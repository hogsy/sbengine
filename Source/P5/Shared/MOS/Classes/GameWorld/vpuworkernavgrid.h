
//
#include "../../xr/XRBlockNav.h"
//#include "../../xr/XRBlockNavParam.h"
#include "../../xr/XRBlockNavGrid_Shared.h"

#include "WPhysState_Hash.h"

#include "VPUWorkerNavGrid_Params.h"

/*
#ifndef PLATFORM_SPU
	//#define VPU_BLOCKNAV_DEBUG
#endif
	*/


//
// Object hash specialization
//
static CVPU_JobInfo *g_pJobInfo = 0x0; // used for auto-init for the cache buffers

template <typename T, int ElementsPerCacheLine, int CacheSize, int ParamIndex>
class TVPU_CacheBufferAutoInit : public CVPU_CacheBuffer<T, ElementsPerCacheLine, CacheSize>
{
public:
	TVPU_CacheBufferAutoInit()
	: CVPU_CacheBuffer<T,ElementsPerCacheLine,CacheSize>(g_pJobInfo->GetCacheBuffer(ParamIndex))
	{}
};

class CSpaceEnumSpecialization_VPU
{
public:

	class CHashSpacializationBase
	{
	public:
		class CElementMixin
		{
		public: 
			CMat4Dfp32 m_Position;
			CBox3Dfp32 m_Bounding;

			inline void Assign(class CWObject_CoreData *_pObj) {}
			inline void Release() {}

			inline const CBox3Dfp32 *GetBox() const
			{
				static CBox3Dfp32 box, temp;
				temp = m_Bounding;
				temp.Transform(m_Position, box);
				return &box;
			}

			inline bool BoxEnumCheck(const CWO_EnumParams_Box& _Params, const CWO_Hash_Settings &_Settings) const { return true; }
			inline const CElementMixin &GetReturnValue(int ID) const { return *this; }
		};

		typedef CElementMixin ElementMixinType;
		typedef ElementMixinType EnumerateReturnType;
	};

	typedef CHashSpacializationBase::ElementMixinType EnumerateReturnType;

	// tag management
	class CStorage_TagManagement
	{
	public:
		uint32 *m_pTags;
		int m_NumTags;
		int m_NumTagged;

		CStorage_TagManagement()
		{
			m_pTags = 0;
		}

		void TagInit(int _MaxIDs)
		{
			m_NumTags = (_MaxIDs+32)/32;
			//M_TRACEALWAYS("%d tags, %d ids\n", m_NumTags, _MaxIDs);
			m_pTags = (uint32*)MRTC_System_VPU::OS_LocalHeapAlloc(m_NumTags * sizeof(uint32));
			for(int i = 0; i < m_NumTags; i++)
				m_pTags[i] = 0;
			m_NumTagged = 0;
		}

		inline void TagElement(int _ID)
		{
			m_pTags[_ID/32] |= 1<<(_ID%32);
			m_NumTagged++;
		}

		inline bool IsElementTagged(int _ID)
		{
			//return false;
			return (m_pTags[_ID/32] & (1<<(_ID%32))) != 0;
		}

		inline void FinalizeEnum(EnumerateReturnType *_pRetValues, int _NumValues)
		{
			if(m_NumTagged)
			{
				for(int i = 0; i < m_NumTags; i++)
					m_pTags[i] = 0;
				m_NumTagged = 0;
			}
		}
	};

	class CHashSpecialization1 : public CHashSpacializationBase
	{
	public:
		class CStorage : public CStorage_TagManagement
		{
		public:
			CStorage()
			{
				TagInit(m_lElements.Len());
			}

			// expose the arrays so we can send them off to the VPU
			TVPU_CacheBufferAutoInit<CWO_HashLink,8,8,VPU_NAVGRID_PARAM_HASH1_LINKS> m_lLinks;
			TVPU_CacheBufferAutoInit<TWO_HashElement<CHashSpacializationBase::CElementMixin>,4,4,VPU_NAVGRID_PARAM_HASH1_ELEMENTS> m_lElements;
			TVPU_CacheBufferAutoInit<int16,16,8,VPU_NAVGRID_PARAM_HASH1_IDS> m_lHash;

			inline const CWO_HashLink &GetLink(int index) { return m_lLinks[index]; }
			inline const TWO_HashElement<ElementMixinType> &GetElement(int index) { return m_lElements[index]; }
			inline int16 GetHash(int index) { return m_lHash[index]; }
		};

		typedef CStorage StorageType;
	};

	class CHashSpecialization2 : public CHashSpacializationBase
	{
	public:
		class CStorage : public CStorage_TagManagement
		{
		public:
			CStorage()
			{
				TagInit(m_lElements.Len());
			}

			// expose the arrays so we can send them off to the VPU
			TVPU_CacheBufferAutoInit<CWO_HashLink,8,8,VPU_NAVGRID_PARAM_HASH2_LINKS> m_lLinks;
			TVPU_CacheBufferAutoInit<TWO_HashElement<CHashSpacializationBase::CElementMixin>,4,4,VPU_NAVGRID_PARAM_HASH2_ELEMENTS> m_lElements;
			TVPU_CacheBufferAutoInit<int16,16,8,VPU_NAVGRID_PARAM_HASH2_IDS> m_lHash;

			inline const CWO_HashLink &GetLink(int index) { return m_lLinks[index]; }
			inline const TWO_HashElement<ElementMixinType> &GetElement(int index) { return m_lElements[index]; }
			inline int16 GetHash(int index) { return m_lHash[index]; }
		};

		typedef CStorage StorageType;
	};

	typedef TWO_Hash_RO<CHashSpecialization1> HashType1;
	typedef TWO_Hash_RO<CHashSpecialization2> HashType2;
};

class CWO_SpaceEnum_RO_VPU : public TWO_SpaceEnum_RO<CSpaceEnumSpecialization_VPU>
{
public:
	CWO_Hash_Settings &GetHash1Settings()
	{
		return TWO_SpaceEnum_RO<CSpaceEnumSpecialization_VPU>::m_Hash1;
	}
	CWO_Hash_Settings &GetHash2Settings()
	{
		return TWO_SpaceEnum_RO<CSpaceEnumSpecialization_VPU>::m_Hash2;
	}

	CSpaceEnumSpecialization_VPU::HashType1 &GetHash1()
	{
		return TWO_SpaceEnum_RO<CSpaceEnumSpecialization_VPU>::m_Hash1;
	}
	CSpaceEnumSpecialization_VPU::HashType2 &GetHash2()
	{
		return TWO_SpaceEnum_RO<CSpaceEnumSpecialization_VPU>::m_Hash2;
	}
};

//
class CXR_BlockNav_Grid_VPU : public CXR_BlockNav_GridProvider
{
public:
	/////// SETTINGS //////
	enum
	{
		//MAX_DECOMPRESSED_TILES = 8, // 4k*X = Total mem
		MAX_DECOMPRESSED_TILES = 8,
	};

	struct SBitPlane
	{
		uint8 m_lBits[XR_NAVTILE_DIM*XR_NAVTILE_DIM*XR_NAVTILE_DIM / 8];
	};

	struct SDecompressedTile
	{	
		int m_AccessCounter;
		int m_PosX;
		int m_PosY;
		int m_PosZ;
		int m_NumBitPlanes;
		CXR_BlockNav_BitPlane m_lBitPlanes[XR_CELL_MAXBITPLANES]; // 4k!
	};

	//
	SDecompressedTile *m_lDecompressedTiles;
	uint8 *m_pDecompessOutput;
	int m_AccessCounter;
	int m_DecompressCount;

	// used for rendering
	CVec3Dfp32 m_CurrentTileWPos;
	SDecompressedTile *m_pCurrentRenderTile;

	// streams
	TVPU_CacheBufferAutoInit<uint16,8,8,VPU_NAVGRID_PARAM_TILETILEINDEX	> m_TileTileIndexes;
	TVPU_CacheBufferAutoInit<uint16,8,8,VPU_NAVGRID_PARAM_TILETILES		> m_TileTiles;
	TVPU_CacheBufferAutoInit<uint16,8,8,VPU_NAVGRID_PARAM_TILES			> m_Tiles;
	TVPU_CacheBufferAutoInit<uint16,8,8,VPU_NAVGRID_PARAM_BITPLANES		> m_BitPlanes;
	TVPU_CacheBufferAutoInit<uint32,4,8,VPU_NAVGRID_PARAM_BITPLANESDATA	> m_BitPlanesData;
	TVPU_CacheBufferAutoInit<uint8,32,8,VPU_NAVGRID_PARAM_COMPRESSEDDATA	> m_CompressedData; // TODO: should use prefetching or be an InStream

	// spaceenum (contains cachebuffers aswell
	CWO_SpaceEnum_RO_VPU m_SpaceEnum;

	//
	CXR_BlockNav_Grid_VPU()
	{
		// setup spaceenum hash
		{
			CVPU_SimpleBufferInfo HashSettingsInfo = g_pJobInfo->GetSimpleBuffer(VPU_NAVGRID_PARAM_HASH_SETTINGS);
			CVPU_SimpleBuffer<CWO_Hash_Settings> HashSettingsBuf(HashSettingsInfo);
			uint32 numelements = 2;
			m_SpaceEnum.GetHash1Settings() = HashSettingsBuf.GetBuffer(numelements)[0];
			m_SpaceEnum.GetHash2Settings() = HashSettingsBuf.GetBuffer(numelements)[1];
		}

		// allocate tiles
		m_lDecompressedTiles = (SDecompressedTile*)MRTC_System_VPU::OS_LocalHeapAlloc(MAX_DECOMPRESSED_TILES*sizeof(SDecompressedTile));

		// init tiles
		m_DecompressCount = 0;
		m_AccessCounter = 0;
		for(int i = 0; i < MAX_DECOMPRESSED_TILES; i++)
		{
			m_lDecompressedTiles[i].m_AccessCounter = -1;
			m_lDecompressedTiles[i].m_PosX = -1;
			m_lDecompressedTiles[i].m_PosY = -1;
			m_lDecompressedTiles[i].m_PosZ = -1;
			m_lDecompressedTiles[i].m_NumBitPlanes = 0;
		}
	}

	virtual ~CXR_BlockNav_Grid_VPU()
	{
	}


	inline int GetBitAt(int _Bit)
	{
		return (m_CompressedData.GetElement(_Bit >> 3) & (1 << (_Bit & 7))) ? 1 : 0;
	}

	inline void SetBitAt(int x, int y, int z, int _Bit)
	{
		int Pos = x + ((y + (z << XR_NAVTILE_DIMSHIFT)) << XR_NAVTILE_DIMSHIFT);
		m_pDecompessOutput[Pos >> 3] = (m_pDecompessOutput[Pos >> 3] & ~(1 << (Pos & 7))) | ((_Bit & 1) << (Pos & 7));
	}

	int Decompress_r(int _Pos, int x, int y, int z, int _Size)
	{
		int Pos = _Pos;
		if (_Size == 1)
		{
			SetBitAt(x, y, z, GetBitAt(Pos++));
		}
		else
		{
			if (GetBitAt(Pos++))
			{
				// Subdivide
				_Size >>= 1;
				Pos += Decompress_r(Pos, x		,y		,z		,	_Size);
				Pos += Decompress_r(Pos, x+_Size,y		,z		,	_Size);
				Pos += Decompress_r(Pos, x		,y+_Size,z		,	_Size);
				Pos += Decompress_r(Pos, x+_Size,y+_Size,z		,	_Size);
				Pos += Decompress_r(Pos, x		,y		,z+_Size,	_Size);
				Pos += Decompress_r(Pos, x+_Size,y		,z+_Size,	_Size);
				Pos += Decompress_r(Pos, x		,y+_Size,z+_Size,	_Size);
				Pos += Decompress_r(Pos, x+_Size,y+_Size,z+_Size,	_Size);
			}
			else
			{
				// Fill
				int FillValue = GetBitAt(Pos++);
				for(int ix = 0; ix < _Size; ix++)
					for(int iy = 0; iy < _Size; iy++)
						for(int iz = 0; iz < _Size; iz++)
							SetBitAt(x+ix, y+iy, z+iz, FillValue);
			}
		}

		return Pos - _Pos;
	}

	//
	int Decompress(uint32 byte_offset, uint8 *output)
	{
		// clear the output
		m_pDecompessOutput = output;
		for(int i = 0; i < 512; i++)
			m_pDecompessOutput[i] = 0xff;

		// offset is in bits
		Decompress_r(byte_offset<<3, 0,0,0, XR_NAVTILE_DIM);
		return 0;
	}

	// ttN = Tile tile pos
	// twN = Tile wrapped pos
	inline int GetTile(int ttx, int tty, int ttz, int twx, int twy, int twz)
	{
		int TileTileIndex = m_TileTileIndexes.GetElement(ttx + ((tty + ttz*m_TileTileGridDim[1])*m_TileTileGridDim[0]));
		int TileIndex = twx + ((twy + twz*XR_NAVTILETILE_DIM)*XR_NAVTILETILE_DIM);
		int Offset = TileTileIndex*XR_NAVTILETILE_NUMCELLS + TileIndex;
		return m_TileTiles.GetElement(Offset);
	}

	//
	//
	//
	uint GetAt(const CVec3Dint32& _Pos)
	{
		int x = _Pos[0];
		int y = _Pos[1];
		int z = _Pos[2];

		// clamp the values
		if (x < 0) z = 0;
		if (y < 0) y = 0;
		if (z < 0) z = 0;
		if (x >= m_CellGridDim[0]) x = m_CellGridDim[0]-1;
		if (y >= m_CellGridDim[1]) y = m_CellGridDim[1]-1;
		if (z >= m_CellGridDim[2]) z = m_CellGridDim[2]-1;

		// shift to get the tilepos
		int tpos_x = x >> XR_NAVTILE_DIMSHIFT;
		int tpos_y = y >> XR_NAVTILE_DIMSHIFT;
		int tpos_z = z >> XR_NAVTILE_DIMSHIFT;

		// look in decompressed cache // TODO: some sort of hash thingie
		SDecompressedTile *pDCTile = (SDecompressedTile *)0x0;
		for(int i = 0; i < MAX_DECOMPRESSED_TILES; i++)
			if(m_lDecompressedTiles[i].m_PosX == tpos_x && m_lDecompressedTiles[i].m_PosY == tpos_y && m_lDecompressedTiles[i].m_PosZ == tpos_z)
			{
				pDCTile = &m_lDecompressedTiles[i];
				break;
			}

		if(!pDCTile)
		{
			// decompress tile
			const int TileTileShift = XR_NAVTILETILE_DIMSHIFT + XR_NAVTILE_DIMSHIFT;
			int ttpos_x = x >> TileTileShift;
			int ttpos_y = y >> TileTileShift;
			int ttpos_z = z >> TileTileShift;
			int tposwrapped_x = (x >> XR_NAVTILE_DIMSHIFT) & XR_NAVTILETILE_DIMAND;
			int tposwrapped_y = (y >> XR_NAVTILE_DIMSHIFT) & XR_NAVTILETILE_DIMAND;
			int tposwrapped_z = (z >> XR_NAVTILE_DIMSHIFT) & XR_NAVTILETILE_DIMAND;

			int index = GetTile(ttpos_x, ttpos_y, ttpos_z, tposwrapped_x, tposwrapped_y, tposwrapped_z);

			int NumBitPlanes = m_Tiles.GetElement(index*2);
			int iBitPlanes = m_Tiles.GetElement(index*2+1);

			// selected tile to cache, aswell as save the posititon
			pDCTile = &m_lDecompressedTiles[0];
			for(int i = 1; i < MAX_DECOMPRESSED_TILES; i++)
			{
				if(m_lDecompressedTiles[i].m_AccessCounter < pDCTile->m_AccessCounter)
					pDCTile = &m_lDecompressedTiles[i];
			}
			
			// set tile info
			pDCTile->m_NumBitPlanes = NumBitPlanes;
			pDCTile->m_PosX = tpos_x;
			pDCTile->m_PosY = tpos_y;
			pDCTile->m_PosZ = tpos_z;

			// decompress data into the buffer
			for(int i = 0; i < NumBitPlanes; i++)
			{
				uint16 iBitPlane = m_BitPlanes.GetElement(iBitPlanes+i);
				uint32 DataOffset = m_BitPlanesData.GetElement(iBitPlane);
				Decompress(DataOffset, pDCTile->m_lBitPlanes[i].m_lBits);
			}

			// render dynamic models into the grid
			m_pCurrentRenderTile = pDCTile;

			CBox3Dfp32 Box;
			Box.m_Min[0] = fp32(m_pCurrentRenderTile->m_PosX) * m_UnitsPerTile + m_Origin[0];
			Box.m_Min[1] = fp32(m_pCurrentRenderTile->m_PosY) * m_UnitsPerTile + m_Origin[1];
			Box.m_Min[2] = fp32(m_pCurrentRenderTile->m_PosZ) * m_UnitsPerTile + m_Origin[2];
			Box.m_Max[0] = fp32(m_pCurrentRenderTile->m_PosX+1) * m_UnitsPerTile + m_Origin[0];
			Box.m_Max[1] = fp32(m_pCurrentRenderTile->m_PosY+1) * m_UnitsPerTile + m_Origin[1];
			Box.m_Max[2] = fp32(m_pCurrentRenderTile->m_PosZ+1) * m_UnitsPerTile + m_Origin[2];
			m_CurrentTileWPos = Box.m_Min;

			//
			CWO_EnumParams_Box Params;
			Params.m_Box = Box;
			Params.m_ObjectFlags = 0; // the flags are not in use
			Params.m_ObjectNotifyFlags = 0;
			Params.m_ObjectIntersectFlags = 0;
			CSpaceEnumSpecialization_VPU::EnumerateReturnType lBoxes[32];
			int NumBoxes = m_SpaceEnum.EnumerateBox(Params, lBoxes, 32);

			// render the objects into the grid
			for(int i = 0; i < NumBoxes; i++)
				RenderOBB(lBoxes[i].m_Bounding, lBoxes[i].m_Position);

			// increase count
			m_DecompressCount++;
		}

		//
		pDCTile->m_AccessCounter = m_AccessCounter++;

		// construct value and return
		CVec3Dint32 Cell(x & XR_NAVTILE_DIMAND, y & XR_NAVTILE_DIMAND, z & XR_NAVTILE_DIMAND);
		int ret = CXR_BlockNav_BitPlane::GetAt(Cell, pDCTile->m_lBitPlanes, pDCTile->m_NumBitPlanes);
		return ret;
	}


	void RenderOBB(const CBox3Dfp32& _Box, const CMat4Dfp32& _Pos)
	{
		//M_TRACEALWAYS("Rendering OOB\r\n");
		CBox3Dfp32 Bound; // = _Box;
		_Box.Transform(_Pos, Bound);

		int x0 = TruncToInt((Bound.m_Min[0] - m_CurrentTileWPos[0]) * m_CellsPerUnit);
		int y0 = TruncToInt((Bound.m_Min[1] - m_CurrentTileWPos[1]) * m_CellsPerUnit);
		int z0 = TruncToInt((Bound.m_Min[2] - m_CurrentTileWPos[2]) * m_CellsPerUnit);
		int x1 = TruncToInt((Bound.m_Max[0] - m_CurrentTileWPos[0]) * m_CellsPerUnit + 1.0f - _FP32_EPSILON);
		int y1 = TruncToInt((Bound.m_Max[1] - m_CurrentTileWPos[1]) * m_CellsPerUnit + 1.0f - _FP32_EPSILON);
		int z1 = TruncToInt((Bound.m_Max[2] - m_CurrentTileWPos[2]) * m_CellsPerUnit + 1.0f - _FP32_EPSILON);
		if (x0 < 0) x0 = 0; else if(x0 >= XR_NAVTILE_DIM) return;
		if (x1 >= XR_NAVTILE_DIM) x1 = XR_NAVTILE_DIM-1; else if(x1 < 0) return;
		if (y0 < 0) y0 = 0; else if(y0 >= XR_NAVTILE_DIM) return;
		if (y1 >= XR_NAVTILE_DIM) y1 = XR_NAVTILE_DIM-1; else if(y1 < 0) return;
		if (z0 < 0) z0 = 0; else if(z0 >= XR_NAVTILE_DIM) return;
		if (z1 >= XR_NAVTILE_DIM) z1 = XR_NAVTILE_DIM-1; else if(z1 < 0) return;

		CVec3Dfp32 TraceOrigin(m_CurrentTileWPos);
		TraceOrigin[0] -= m_UnitsPerTile*8.0f;
		TraceOrigin[1] += m_UnitsPerCell*0.5f;
		TraceOrigin[2] += m_UnitsPerCell*0.5f;
		CVec3Dfp32 TraceX(m_UnitsPerTile*16.0f, 0, 0);
		CVec3Dfp32 TraceY(0, m_UnitsPerCell, 0);
		CVec3Dfp32 TraceZ(0, 0, m_UnitsPerCell);

		CMat4Dfp32 PosInv;
		_Pos.InverseOrthogonal(PosInv);
		TraceOrigin.MultiplyMatrix(PosInv);
		TraceX.MultiplyMatrix3x3(PosInv);
		TraceY.MultiplyMatrix3x3(PosInv);
		TraceZ.MultiplyMatrix3x3(PosInv);

		fp32 xscale = Sqr(1.0f / (m_UnitsPerTile * 16.0f)) * 16.0f * fp32(XR_NAVTILE_DIM);
		fp32 xadd = -8.0f*fp32(XR_NAVTILE_DIM);

		CXR_BlockNav_BitPlane* pBitPlanes = m_pCurrentRenderTile->m_lBitPlanes;
		int nBitPlanes = m_pCurrentRenderTile->m_NumBitPlanes;

		for(int z = z0; z <= z1; z++)
			for(int y = y0; y <= y1; y++)
			{
				CVec3Dfp32 TracePos0, TracePos1;
				TraceOrigin.Combine(TraceY, fp32(y), TracePos0);
				TracePos0.Combine(TraceZ, fp32(z), TracePos0);
				TracePos0.Add(TraceX, TracePos1);

				CVec3Dfp32 HitPos0, HitPos1;
				if (_Box.IntersectLine(TracePos0, TracePos1, HitPos0))
					if (_Box.IntersectLine(TracePos1, TracePos0, HitPos1))
					{
						HitPos0 -= TraceOrigin;
						HitPos1 -= TraceOrigin;
						fp32 fx0 = (HitPos0*TraceX) * xscale + xadd;
						fp32 fx1 = (HitPos1*TraceX) * xscale + xadd;
						if (fx0 > fx1)
							Swap(fx0, fx1);

						int x0 = TruncToInt(fx0);
						if (x0 < 0) x0 = 0; else if(x0 >= XR_NAVTILE_DIM) continue;
						int x1 = TruncToInt(fx1 + 1.0f - _FP32_EPSILON);
						if (x1 >= XR_NAVTILE_DIM) x1 = XR_NAVTILE_DIM-1; else if(x1 < 0) continue;

						for(int x = x0; x <= x1; x++)
							CXR_BlockNav_BitPlane::SetAt(CVec3Dint32(x,y,z), pBitPlanes, nBitPlanes, 0);

					}
			}
	}
};

//
//
uint32 VPUWorker_NavGrid(CVPU_JobInfo &_JobInfo)
{
#if defined(VPU_NAMEDEVENTS)
	M_NAMEDEVENT("VPUWorker_NavGrid", 0xff800000);
#endif
	//M_TRACE("VPU: Memory left: %d\r\n", MRTC_System_VPU::OS_LocalHeapMemLeft());

	//
	g_pJobInfo = &_JobInfo;

	// result values
	int result = 0;
	int numnodes = 0;
	int cost = 0;

	// NOTE: Check so that CXBN_SearchNode doesn't align upto 16 bytes and become 32 bytes per instance instead of 24 -kma

	// fetch streams
	CVPU_SimpleBufferInfo SearchParamsInfo = _JobInfo.GetSimpleBuffer(VPU_NAVGRID_PARAM_SEARCHPARAMS);
	CVPU_SimpleBufferInfo GridLayoutInfo = _JobInfo.GetSimpleBuffer(VPU_NAVGRID_PARAM_GRIDLAYOUT);

	CVPU_SimpleBufferInfo ResultInfo = _JobInfo.GetSimpleBuffer(VPU_NAVGRID_PARAM_RESULTNODES);
		
	//return 0;

	// create grid
	{
		CVPU_SimpleBuffer<CXBN_SearchResultNode> ResultBuffer(ResultInfo);

		//
		CXR_BlockNav_Grid_VPU NavGrid;
		/* REMOVE: TileTilesIndexInfo, TileTilesInfo, TilesInfo, BitPlanesInfo, BitPlanesDataInfo, CompressedDataInfo, ModelsInfo*/
		CXR_BlockNav_Grid_VPU *pNavGrid = &NavGrid; //new(0,VPU_ALLOC_DETAIL()) CXR_BlockNav_Grid_VPU();
		CXR_BlockNav BlockNav;
		CXR_BlockNav *pBlockNav = &BlockNav;
		//CXBN_SearchInstance Search;

		// TODO: NASTY!! set layout
		{
			uint32 numelements = 1;
			CVPU_SimpleBuffer<CXR_BlockNav_GridLayout> GridLayout(GridLayoutInfo);
			CXR_BlockNav_GridLayout Layout = *(CXR_BlockNav_GridLayout *)GridLayout.GetBuffer(numelements);
			pNavGrid->Create(Layout);
		}

		#ifdef VPU_BLOCKNAV_DEBUG
			int DebugCmd = _JobInfo.GetLParam(VCPU_NAVGRID_PARAM_DEBUG, 0);
			int DebugX = _JobInfo.GetLParam(VCPU_NAVGRID_PARAM_DEBUG, 1);
			int DebugY = _JobInfo.GetLParam(VCPU_NAVGRID_PARAM_DEBUG, 2);
			int DebugZ = _JobInfo.GetLParam(VCPU_NAVGRID_PARAM_DEBUG, 3);

			if(DebugCmd)
			{
				if(DebugCmd == 1)
				{
					CVPU_SimpleBufferInfo DebugOutInfo = _JobInfo.GetSimpleBuffer(VCPU_NAVGRID_PARAM_DEBUG_OUTPUT);
					CVPU_SimpleBuffer<int32> DebugOut(DebugOutInfo);

					uint32 numelements = 4;
					CVec3Dint32 v(DebugX, DebugY, DebugZ);
					*DebugOut.GetBuffer(numelements) = NavGrid.GetAt(v);
					return 0;
				}
				return 0;
			}
		#endif

		// init blocknav
		pBlockNav->Create(pNavGrid); // should be 1

		// TODO: NASTY!! fetch search parameters
		uint32 numelements = 1;
		CVPU_SimpleBuffer<CXBN_SearchParams> SearchParams(SearchParamsInfo);
		CXBN_SearchParams Params = *(CXBN_SearchParams *)SearchParams.GetBuffer(numelements);

		// start the search
		CXBN_SearchInstance Search;
		Search.Create(pNavGrid, pBlockNav, 0, 1024*4);
		Search.Search_Create(Params);
		//int iSearch = pBlockNav->Search_Create(Params);
		int nodes = 0;
		result = CXR_BlockNavSearcher::SEARCH_IN_PROGRESS;

		while(result == CXR_BlockNavSearcher::SEARCH_IN_PROGRESS)
		{
			nodes = 0;
			result = Search.Search_Execute(nodes, 0);
			//result = pBlockNav->Search_Execute(iSearch, nodes, 0);
			switch(result)
			{
			case CXR_BlockNavSearcher::SEARCH_NO_PATH:
				break;
			case CXR_BlockNavSearcher::SEARCH_DONE:
				{
					CXBN_SearchResultCore Result;
					Search.Search_Get(&Result, false);
					//pBlockNav->Search_Get(iSearch, &Result, false);
					cost = Result.GetCost();
					uint32 MaxNodes;
					CXBN_SearchResultNode *pNodes = ResultBuffer.GetBuffer(MaxNodes);
					numnodes = Result.GetLength();
					if(Result.GetLength() >= (int)MaxNodes)
						M_TRACEALWAYS("[VPU SEARCH] Result buffer too small!!\n");

					for(int i = 0; i < Result.GetLength() && i < (int)MaxNodes; i++)
					{
						CXBN_SearchResultNode Node = Result.GetNode(i);
						pNodes[i] = Node;
						CVec3Dint16 p = Node.GetGridPosition();
					}
				} break;

			case CXR_BlockNavSearcher::SEARCH_IN_PROGRESS: // no message for this
				break;

			default: // failure
				/*
				switch(result)
				{
				case CXR_BlockNav::SEARCH_DONE: M_TRACE("VPU Trace: SEARCH_DONE\r\n"); break;
				case CXR_BlockNav::SEARCH_IN_PROGRESS: M_TRACE("VPU Trace: SEARCH_IN_PROGRESS\r\n"); break;
				case CXR_BlockNav::SEARCH_SOURCE_INTRAVERSABLE: M_TRACE("VPU Trace: SEARCH_SOURCE_INTRAVERSABLE\r\n"); break;
				case CXR_BlockNav::SEARCH_DESTINATION_INTRAVERSABLE: M_TRACE("VPU Trace: SEARCH_DESTINATION_INTRAVERSABLE\r\n"); break;
				case CXR_BlockNav::SEARCH_SOURCE_OUTOFBOUNDS: M_TRACE("VPU Trace: SEARCH_SOURCE_OUTOFBOUNDS\r\n"); break;
				case CXR_BlockNav::SEARCH_DESTINATION_OUTOFBOUNDS: M_TRACE("VPU Trace: SEARCH_DESTINATION_OUTOFBOUNDS\r\n"); break;
				case CXR_BlockNav::SEARCH_NO_PATH: M_TRACE("VPU Trace: SEARCH_NO_PATH\r\n"); break;
				case CXR_BlockNav::SEARCH_NODE_ALLOC_FAIL: M_TRACE("VPU Trace: SEARCH_NODE_ALLOC_FAIL\r\n"); break;
				case CXR_BlockNav::SEARCH_INSTANCE_INVALID: M_TRACE("VPU Trace: SEARCH_INSTANCE_INVALID\r\n"); break;
				case CXR_BlockNav::SEARCH_INSTANCE_DESTROYED: M_TRACE("VPU Trace: SEARCH_INSTANCE_DESTROYED\r\n"); break;
				case CXR_BlockNav::SEARCH_NO_NAVGRID: M_TRACE("VPU Trace: SEARCH_NO_NAVGRID\r\n"); break;
				}*/
				break;
			}
		}
	}

	// 
	//M_TRACEALWAYS("[VPU SEARCH] END!\r\n");
	//return 0;

	// write back the result
	MRTC_System_VPU::OS_WriteData(_JobInfo.GetLParam(VPU_NAVGRID_PARAM_RESULT, 0), result);
	MRTC_System_VPU::OS_WriteData(_JobInfo.GetLParam(VPU_NAVGRID_PARAM_RESULT, 1), numnodes);
	MRTC_System_VPU::OS_WriteData(_JobInfo.GetLParam(VPU_NAVGRID_PARAM_RESULT, 2), cost);
	return 0;
};
