#ifndef _INC_XRBlockNavGrid_Shared
#define _INC_XRBlockNavGrid_Shared

// -------------------------------------------------------------------
//  Structure versions
#define XR_NAVBITPLANE_VERSION			0x0200
#define XR_NAVBITPLANEINDEX_VERSION		0x0200
#define XR_NAVTILE_VERSION				0x0200
#define XR_NAVGRID_VERSION				0x0200
#define XR_NAVTILETILE_VERSION			0x0200
#define XR_NAVCODEBOOK_VERSION			0x0200

// -------------------------------------------------------------------
#define XR_NAVTILE_DIM 16
#define XR_NAVTILE_DIMAND (XR_NAVTILE_DIM-1)
#define XR_NAVTILE_DIMSHIFT 4

#define XR_NAVTILETILE_DIM 4
#define XR_NAVTILETILE_DIMAND (XR_NAVTILETILE_DIM-1)
#define XR_NAVTILETILE_DIMSHIFT 2
#define XR_NAVTILETILE_NUMCELLS (XR_NAVTILETILE_DIM*XR_NAVTILETILE_DIM*XR_NAVTILETILE_DIM)

// -------------------------------------------------------------------
//  Tile flags

#define XR_CELL_AIR				1
#define XR_CELL_TRAVERSABLE		2	// (Air) Cell is traversable (see below)
#define XR_CELL_SLIP			4	// (Air) Slip: Movement is permitted only downwards, Jumping not allowed
#define XR_CELL_DIE				8	// (Air)
#define XR_CELL_WALL			16	// Wall cell, slightly expensive and useable for wallclimbers
#define XR_CELL_EXPENSIVE		32	// Cell is traversible but (very) expensive, used inside dynamic objects

#define XR_CELL_MAXBITPLANES	8

//Traversability dimensions. A position is considered traversable if all cells in the cube
//posX-X..posX+X, posY-Y, PosY+Y, posZ..posZ+Z is air.
#define	XR_TRAVERSABILITY_X		1  
#define	XR_TRAVERSABILITY_Y		1
#define	XR_TRAVERSABILITY_Z		4


// this is abit ugly -kma

struct VPU_ALLOC_DETAIL {};
struct VPU_ALLOC_DETAIL_BASE {};

#ifdef PLATFORM_VPU
	#define VPU_ALLOCATABLE public: void *operator new(size_t size, int dummy, const VPU_ALLOC_DETAIL v) { return MRTC_System_VPU::OS_LocalHeapAlloc((uint32)size); } private:
#else
	#define VPU_ALLOCATABLE public: void *operator new(size_t size, int dummy, const VPU_ALLOC_DETAIL v) { return ::operator new(size); } private:
//#define VPU_NEW new(0, VPU_ALLOC_DETAIL())
#endif
/*
#ifdef PLATFORM_VPU
	inline void *operator new(size_t size, int dummy, const VPU_ALLOC_DETAIL_BASE &v)
	{
		return MRTC_System_VPU::OS_LocalHeapAlloc((uint32)size);
	}
#else
	inline void *operator new(size_t size, int dummy, const VPU_ALLOC_DETAIL_BASE &v)
	{
		return ::operator new(size);
	}
#endif
*/
// stub class for vectors
/*
	template<typename T>
	class TVec3D
	{
	public:
		T k[3];
		TVec3D() {}
		TVec3D(T a, T b, T c)
		{
			k[0] = a;
			k[1] = b;
			k[2] = c;
		}

		T operator [](const int i) const { return k[i]; }
	};

	typedef TVec3D<int16> CVec3Dint16;
	typedef TVec3D<fp32> CVec3Dfp32;*/

//
//
//
class M_ALIGN(16) CXR_BlockNav_GridLayout
{
public:
	CVec3Dint32						m_TileTileGridDim;	// Dimension in tile tiles
	CVec3Dint32						m_CellGridDim;		// Dimension in cells
	CVec3Dfp32						m_Origin;		// world-space origin of grid pos (0,0,0)
	fp32							m_UnitsPerCell;
	fp32							m_UnitsPerTile;
	fp32							m_UnitsPerTileTile;
	fp32							m_CellsPerUnit;
	fp32							m_TilesPerUnit;
	fp32							m_TileTilesPerUnit;

	CXR_BlockNav_GridLayout()
	{
		m_TileTileGridDim = 0;
		m_CellGridDim = 0;
		m_Origin = 0;
		m_UnitsPerCell = 0;
		m_UnitsPerTile = 0;
		m_UnitsPerTileTile = 0;
		m_CellsPerUnit = 0;
		m_TilesPerUnit = 0;
		m_TileTilesPerUnit = 0;
	}

	void Create(const CBox3Dfp32 &_BoundBox, fp32 _UnitsPerCell)
	{
		//((CXR_BlockNav_GridLayout_Create, MAUTOSTRIP_VOID);
		m_UnitsPerCell = _UnitsPerCell;
		m_UnitsPerTile = _UnitsPerCell * XR_NAVTILE_DIM;
		m_UnitsPerTileTile = _UnitsPerCell * XR_NAVTILE_DIM * XR_NAVTILETILE_DIM;
		m_CellsPerUnit = 1.0f / _UnitsPerCell;
		m_TilesPerUnit = 1.0f / m_UnitsPerTile;
		m_TileTilesPerUnit = 1.0f / m_UnitsPerTileTile;

		m_Origin[0] = M_Floor(_BoundBox.m_Min[0] * m_TileTilesPerUnit);
		m_Origin[1] = M_Floor(_BoundBox.m_Min[1] * m_TileTilesPerUnit);
		m_Origin[2] = M_Floor(_BoundBox.m_Min[2] * m_TileTilesPerUnit);
		m_TileTileGridDim[0] = int32(fp32(M_Floor(_BoundBox.m_Max[0] * m_TileTilesPerUnit)) -  fp32(m_Origin[0]));
		m_TileTileGridDim[1] = int32(fp32(M_Floor(_BoundBox.m_Max[1] * m_TileTilesPerUnit)) -  fp32(m_Origin[1]));
		m_TileTileGridDim[2] = int32(fp32(M_Floor(_BoundBox.m_Max[2] * m_TileTilesPerUnit)) -  fp32(m_Origin[2]));
		m_Origin *= m_UnitsPerTileTile;
		m_TileTileGridDim += 1;

		m_CellGridDim = m_TileTileGridDim;
		m_CellGridDim *= XR_NAVTILE_DIM * XR_NAVTILETILE_DIM;

		//LogFile("Creating navgrid.");
		//LogFile(CStr("        Bound: ") + _BoundBox.GetString());
		//LogFile(CStrF("        UnitsPerCell:     %f", m_UnitsPerCell));
		//LogFile(CStrF("        UnitsPerTile:     %f", m_UnitsPerTile));
		//LogFile(CStrF("        UnitsPerTileTile: %f", m_UnitsPerTileTile));
		//LogFile(CStrF("        CellsPerUnit:     %f", m_CellsPerUnit));
		//LogFile(CStrF("        TilesPerUnit:     %f", m_TilesPerUnit));
		//LogFile(CStrF("        TileTilesPerUnit: %f", m_TileTilesPerUnit));
		//LogFile(CStrF("        TileTileGridDim:  %d, %d, %d", m_TileTileGridDim[0], m_TileTileGridDim[1], m_TileTileGridDim[2] ));
		//LogFile(CStrF("        CellGridDim:      %d, %d, %d", m_CellGridDim[0], m_CellGridDim[1], m_CellGridDim[2] ));
		//LogFile(CStr("        Origin:           ") + m_Origin.GetString());
	}

	void Create(const CXR_BlockNav_GridLayout& _Grid)
	{
		MAUTOSTRIP(CXR_BlockNav_GridLayout_Create_2, MAUTOSTRIP_VOID);
		m_TileTileGridDim = _Grid.m_TileTileGridDim;
		m_CellGridDim = _Grid.m_CellGridDim;
		m_Origin = _Grid.m_Origin;
		m_UnitsPerCell = _Grid.m_UnitsPerCell;
		m_UnitsPerTile = _Grid.m_UnitsPerTile;
		m_UnitsPerTileTile = _Grid.m_UnitsPerTileTile;
		m_CellsPerUnit = _Grid.m_CellsPerUnit;
		m_TilesPerUnit = _Grid.m_TilesPerUnit;
		m_TileTilesPerUnit = _Grid.m_TileTilesPerUnit;
	}

	CVec3Dfp32 GetCellPosition(const CVec3Dint32& _Pos)
	{
		return CVec3Dfp32(
			fp32(_Pos[0]) * m_UnitsPerCell,
			fp32(_Pos[1]) * m_UnitsPerCell,
			fp32(_Pos[2]) * m_UnitsPerCell);
	}

};



class CXR_BlockNav_GridProvider : public CXR_BlockNav_GridLayout
{
public:
	// To get the compiler to stop nagging for now.
	// TODO: remove this when the virtuals are gone! -kma
	virtual ~CXR_BlockNav_GridProvider() {}

	// TODO: Must skip these virtuals somehow -kma
	virtual uint GetAt(const CVec3Dint32& _Pos) pure;
};


// -------------------------------------------------------------------
class CXR_BlockNav_BitPlane		// 16*16*16 = 4096 bit cube.  512 Bytes
{
public:
	uint8 m_lBits[XR_NAVTILE_DIM*XR_NAVTILE_DIM*XR_NAVTILE_DIM / 8];

	void Clear(int _Value);
	bool IsEqual(const CXR_BlockNav_BitPlane&) const;
	bool IsZero() const;

protected:
	static void GetPosByteShift(const CVec3Dint32& _Pos, int& _Byte, int& _Shift)
	{
#if XR_NAVTILE_DIM == 8
		_Byte = _Pos[1] + (_Pos[2] << XR_NAVTILE_DIMSHIFT);
		_Shift = _Pos[0];
#else
		int i = _Pos[0] + ((_Pos[1] + (_Pos[2] << XR_NAVTILE_DIMSHIFT)) << XR_NAVTILE_DIMSHIFT);
		_Byte = i >> 3;
		_Shift = (i & 7);
#endif
	}


public:

#if XR_NAVTILE_DIM == 8
	// Get value of bitplane 0
	int GetAt(int x, int y, int z) const
	{
		// NOTE: Written for TILEDIM==8
		int Pos = y + (z << XR_NAVTILE_DIMSHIFT);
		return (m_lBits[Pos] >> x) & 1;
	}

	// Get value of bitplane 0
	void SetAt(int x, int y, int z, int _Bit)
	{
		// NOTE: Written for TILEDIM==8
		int Pos = y + (z << XR_NAVTILE_DIMSHIFT);
		m_lBits[Pos] = (m_lBits[Pos] & ~(1 << x)) | ((_Bit & 1) << x);
	}
#else
	// Get value of bitplane 0
	int GetAt(int x, int y, int z) const
	{
		// NOTE: Written for TILEDIM==8
		int Pos = x + ((y + (z << XR_NAVTILE_DIMSHIFT)) << XR_NAVTILE_DIMSHIFT);
		return (m_lBits[Pos >> 3] >> (Pos & 7)) & 1;
	}

	// Get value of bitplane 0
	void SetAt(int x, int y, int z, int _Bit)
	{
		// NOTE: Written for TILEDIM==8
		int Pos = x + ((y + (z << XR_NAVTILE_DIMSHIFT)) << XR_NAVTILE_DIMSHIFT);
		m_lBits[Pos >> 3] = (m_lBits[Pos >> 3] & ~(1 << (Pos & 7))) | ((_Bit & 1) << (Pos & 7));
	}
#endif

	int GetAt(const CVec3Dint32& _Pos) const
	{
		return GetAt(_Pos[0], _Pos[1], _Pos[2]);
	}

	void SetAt(const CVec3Dint32& _Pos, int _Bit)
	{
		SetAt(_Pos[0], _Pos[1], _Pos[2], _Bit);
	}

	static int GetAt(const CVec3Dint32& _Pos, const CXR_BlockNav_BitPlane* _pBitPlanes, int _nBitPlanes)
	{
		int iByte, ByteShift;
		GetPosByteShift(_Pos, iByte, ByteShift);

		int Value = 0;
		for(int iBP = 0; iBP < _nBitPlanes; iBP++)
		{
			Value += ((_pBitPlanes[iBP].m_lBits[iByte] >> ByteShift) & 1) << iBP;
		}

		return Value;
	}

	static int GetAt(const CVec3Dint32& _Pos, const CXR_BlockNav_BitPlane* _pBitPlanes, int _nBitPlanes, uint16* _piBitPlanes)
	{
		int iByte, ByteShift;
		GetPosByteShift(_Pos, iByte, ByteShift);

		int Value = 0;
		for(int iiBP = 0; iiBP < _nBitPlanes; iiBP++)
		{
			int iBP = _piBitPlanes[iiBP];
			Value += ((_pBitPlanes[iBP].m_lBits[iByte] >> ByteShift) & 1) << iiBP;
		}

		return Value;
	}

	static void SetAt(const CVec3Dint32& _Pos, CXR_BlockNav_BitPlane* _pBitPlanes, int _nBitPlanes, int _Value)
	{
		int iByte, ByteShift;
		GetPosByteShift(_Pos, iByte, ByteShift);

		int Value = _Value;
		for(int iBP = 0; iBP < _nBitPlanes; iBP++)
		{
			_pBitPlanes[iBP].m_lBits[iByte] =
				(_pBitPlanes[iBP].m_lBits[iByte] & ~(1 << ByteShift)) | ((Value & 1) << ByteShift);
			Value >>= 1;
		}
	}

protected:
	bool BoxIsValue(const CVec3Dint32& _Origin, const CVec3Dint32& _Size, int _Value) const;
	int GetBoxValues(const CVec3Dint32& _Origin, const CVec3Dint32& _Size) const;

	int CompressOcttree_r(uint8* _pCompressBuffer, int _Pos, const CVec3Dint32& _Origin, int _Size) const;
	int CompressBintree_r(uint8* _pCompressBuffer, int _Pos, const CVec3Dint32& _Origin, const CVec3Dint32& _Size) const;

public:
	static int GetMaxCompressedSize();				// Returns maximum required space in _pCompressBuffer by Compress()
	int CompressOcttree(uint8* _pCompressBuffer) const;	// Returns compressed size in bytes. Compressed data is written to _pCompressBuffer
	int CompressBintree(uint8* _pCompressBuffer) const;	// Returns compressed size in bytes. Compressed data is written to _pCompressBuffer
	void Decompress(const uint8* _pCompressBuffer);

//	void Read(CCFile* _pFile);
//	void Write(CCFile* _pFile);
};

#endif // _INC_XRBlockNavGrid_Shared
