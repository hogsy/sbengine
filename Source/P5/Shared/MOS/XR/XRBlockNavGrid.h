#ifndef _INC_XRBlockNavGrid
#define _INC_XRBlockNavGrid

#define XR_NAVGRID_BYTECELL

//#include "MCC.h"

/*
// +-----------------------------------------------------------------+
// | XR Block Navigation				                             |
// +-----------------------------------------------------------------+
// | Creator:          Magnus Högdahl                                |
// | Created:          2000-11-02                                    |
// | Last modified:    2000-__-__                                    |
// |                                                                 |
// | Description:                                                    |
// |                                                                 |
// | ...                                                             |
// |                                                                 |
// +-----------------------------------------------------------------+
// | Copyright Starbreeze Studios AB 2000							 |
// +-----------------------------------------------------------------+
*/

#ifdef PLATFORM_CONSOLE
# define xwc_virtual
#else
# define xwc_virtual virtual
#endif

// alot of defines in this files
#include "XRBlockNavGrid_Shared.h"

//

class CXR_BlockNav_CellExt
{
public:
	uint16 m_Pos;				// Position in the block.
	uint16 m_Flags;

	CXR_BlockNav_CellExt* m_pNext;
};


// -------------------------------------------------------------------
//  CXR_BlockNav_BitPlaneCompressed
// -------------------------------------------------------------------
/*
	// Compression using octtree. A leaf is a volume that can be filled with single value.

[BitplaneNode]
{
	1 bit
		0 :
			// Fill node with value
			1 bit, fill value	

		1 :
			// Split. 8 child nodes follow
			[BitplaneNode] * 8
}

*/

// -------------------------------------------------------------------
class CXR_BlockNav_Grid;

class CXR_BlockNav_BitPlaneContainer : public CReferenceCount
{
public:
	TArray<uint8> m_lData;
	TArray<uint32> m_liBitPlaneData;

	void AddBitPlane(const CXR_BlockNav_BitPlane& _BitPlane);
	void GetBitPlane(int _iBitPlane, CXR_BlockNav_BitPlane& _BitPlane);
	const uint8* GetBitPlaneCompressedData(int _iBitPlane)
	{ 
		int iData = m_liBitPlaneData[_iBitPlane];
		return &m_lData[iData];
	}

	void Read(CCFile* _pFile, int _Ver);
	void Write(CCFile* _pFile);
};

// -------------------------------------------------------------------
class CXR_BlockNav_Tile		// "Tile"
{
public:
	uint16 m_nBitPlanes;
	uint16 m_iiBitPlanes;

	CXR_BlockNav_Tile();

	bool IsEqual(const CXR_BlockNav_Tile&) const;

	void Read(CCFile* _pFile, int _Ver);
	void Write(CCFile* _pFile);
};

// -------------------------------------------------------------------
class CXR_BlockNav_TileTile
{
public:
	uint16 m_liTiles[XR_NAVTILETILE_NUMCELLS];

	CXR_BlockNav_TileTile();

	int GetIndex(const CVec3Dint32& _TilePos) const
	{
		return _TilePos[0] + ((_TilePos[1] + _TilePos[2]*XR_NAVTILETILE_DIM)*XR_NAVTILETILE_DIM);
	}

	void SetTile(const CVec3Dint32& _TilePos, int _iTile)
	{
		m_liTiles[GetIndex(_TilePos)] = _iTile;
	}

	int GetTile(const CVec3Dint32& _TilePos) const
	{
		return m_liTiles[GetIndex(_TilePos)];;
	}

	bool IsEqual(const CXR_BlockNav_TileTile&) const;

	void Read(CCFile* _pFile, int _Ver);
	void Write(CCFile* _pFile);
};

// -------------------------------------------------------------------
//  CXR_BlockNav_GridLayout
// -------------------------------------------------------------------

// This is a structure holding a grid size, scale and origin.
/*
class CXR_BlockNav_GridLayout
{
protected:
public:		// protected was, so, so annoying, especially since there was tons of code already assuming that.
	CVec3Dint32						m_TileTileGridDim;	// Dimension in tile tiles
	CVec3Dint32						m_CellGridDim;		// Dimension in cells
	CVec3Dfp32						m_Origin;		// world-space origin of grid pos (0,0,0)
	fp32								m_UnitsPerCell;
	fp32								m_UnitsPerTile;
	fp32								m_UnitsPerTileTile;
	fp32								m_CellsPerUnit;
	fp32								m_TilesPerUnit;
	fp32								m_TileTilesPerUnit;

public:
	CXR_BlockNav_GridLayout();
	void Create(const CBox3Dfp32& _BoundBox, fp32 _UnitsPerCell);
	void Create(const CXR_BlockNav_GridLayout& _Grid);
};*/

// -------------------------------------------------------------------
//  CXR_BlockNav_Grid
// -------------------------------------------------------------------
class CXR_BlockNav_Grid : public CReferenceCount, public CXR_BlockNav_GridProvider
{
	MRTC_DECLARE;

public:
	class CDecompressedTile
	{
	public:
#ifdef XR_NAVGRID_BYTECELL
		CVec128Access m_lCellRows[XR_NAVTILE_DIM*XR_NAVTILE_DIM];
		static vec128 ms_lFillAnd[4*16];	// 64*16 = 1k

#else
		int m_nBitPlanes;
		CXR_BlockNav_BitPlane m_lBitPlanes[XR_CELL_MAXBITPLANES];

#endif

		int16 m_iHash;
		int16 m_iHashPrev;
		int16 m_iHashNext;

		CVec3Dint32 m_TilePos;
		int m_LastAccessCount;

		CDecompressedTile()
		{
			m_iHash = -1;
			m_iHashPrev = -1;
			m_iHashNext = -1;
			m_LastAccessCount = 0;
#ifndef XR_NAVGRID_BYTECELL
			m_nBitPlanes = 0;
#endif
			m_TilePos = -1;
		}

		// Before rendering into an uncompressed bitplane make sure there are
		// enough bitplanes to hold the values you intend to write.
#ifdef XR_NAVGRID_BYTECELL
		void Reset()
		{
			m_LastAccessCount = 0;
			m_TilePos = -1;
		}

		void Clear()
		{
			memset(&m_lCellRows, 0, sizeof(m_lCellRows));
		};

		void SetMinBitPlanes(int _nBitPlanes)
		{
			M_ASSERT(_nBitPlanes <= XR_CELL_MAXBITPLANES, "Too many bitplanes.");
			// Do nada
		}

		void ClearBitPlane(uint _iBit)
		{
			uint And8 = (~M_BitD(_iBit)) & 0xff;
			uint32 And32 = And8 + (And8 << 8)  + (And8 << 16) + (And8 << 24);
			vec128 v = M_VLdScalar_u32(And32);
			for(int i = 0; i < XR_NAVTILE_DIM*XR_NAVTILE_DIM; i++)
				m_lCellRows[i].v = M_VAnd(m_lCellRows[i].v, v);
		}

		void M_FORCEINLINE OrAt(const CVec3Dint32& _Pos, uint _Value)
		{
			m_lCellRows[_Pos[1] + XR_NAVTILE_DIM*_Pos[2]].ku8[_Pos[0]] |= _Value;
		}

		vec128 M_FORCEINLINE GetRow(uint _y, uint _z)
		{
			return m_lCellRows[_y + XR_NAVTILE_DIM*_z].v;
		}

		void M_FORCEINLINE OrRow(uint _y, uint _z, vec128 _Value)
		{
			m_lCellRows[_y + XR_NAVTILE_DIM*_z].v = M_VOr(m_lCellRows[_y + XR_NAVTILE_DIM*_z].v, _Value);
		}

		void M_FORCEINLINE SetAt(uint _x, uint _y, uint _z, uint _Value)
		{
			m_lCellRows[_y + XR_NAVTILE_DIM*_z].ku8[_x] = _Value;
		}

		uint M_FORCEINLINE GetAt(uint _x, uint _y, uint _z)
		{
			return m_lCellRows[_y + XR_NAVTILE_DIM*_z].ku8[_x];
		}

		void M_FORCEINLINE SetAt(const CVec3Dint32& _Pos, uint _Value)
		{
			m_lCellRows[_Pos[1] + XR_NAVTILE_DIM*_Pos[2]].ku8[_Pos[0]] = _Value;
		}

		uint M_FORCEINLINE GetAt(const CVec3Dint32& _Pos)
		{
			return m_lCellRows[_Pos[1] + XR_NAVTILE_DIM*_Pos[2]].ku8[_Pos[0]];
		}

		void Decompress(const uint8* _pBuffer, uint _Value);
		static void CreateFillAndTable();

#else
		void Reset()
		{
			m_LastAccessCount = 0;
			m_nBitPlanes = 0;
			m_TilePos = -1;
		}

		void SetMinBitPlanes(int _nBitPlanes)
		{
			M_ASSERT(_nBitPlanes <= XR_CELL_MAXBITPLANES, "Too many bitplanes.");

			if (m_nBitPlanes < _nBitPlanes)
			{
				for(int i = m_nBitPlanes; i < _nBitPlanes; i++)
					m_lBitPlanes[i].Clear(0);
				m_nBitPlanes = _nBitPlanes;
			}
		}

		void ClearBitPlane(uint _iBit)
		{
			if (m_nBitPlanes > _iBit)
				m_lBitPlanes[_iBit].Clear(0);
		}

#endif
	};

	TArray<uint16>					m_liBitPlanes;
	TArray<uint16>					m_liTileTiles;
	TArray<CXR_BlockNav_TileTile>	m_lTileTiles;
	TArray<CXR_BlockNav_Tile>		m_lTiles;
	CXR_BlockNav_BitPlaneContainer	m_BitPlaneContainer;

	TThinArray<int16>				m_liHashFirstDT;
	TThinArray<CDecompressedTile>	m_lDecompressedTiles;

	int m_AccessCount;

	CXR_BlockNav_Grid();
	~CXR_BlockNav_Grid();
	virtual void Create(int _nCachedTiles);

	virtual void OnPostDecompressTile(CDecompressedTile& _Tile);

protected:
	static M_INLINE int Hash_GetKey(const CVec3Dint32& _Pos)
	{
		return (_Pos[0] & 3) + ((_Pos[1] & 3) << 2) + ((_Pos[2] & 3) << 4);
	}

	void Hash_Clear()
	{
		m_liHashFirstDT.SetLen(1 << 12);
		FillW(m_liHashFirstDT.GetBasePtr(), m_liHashFirstDT.Len(), -1);
	}

	M_INLINE void Hash_InsertDT(int _iDT)
	{
		CDecompressedTile& Tile = m_lDecompressedTiles[_iDT];
		int iHashKey = Hash_GetKey(Tile.m_TilePos);
		Tile.m_iHash = iHashKey;
		Tile.m_iHashPrev = -1;
		Tile.m_iHashNext = m_liHashFirstDT[iHashKey];
		if (Tile.m_iHashNext != -1)
			m_lDecompressedTiles[Tile.m_iHashNext].m_iHashPrev = _iDT;
		m_liHashFirstDT[iHashKey] = _iDT;
	}

	M_INLINE void Hash_RemoveDT(int _iDT)
	{
		CDecompressedTile& Tile = m_lDecompressedTiles[_iDT];
		if (Tile.m_iHash >= 0)
		{
			if (Tile.m_iHashPrev != -1)
				m_lDecompressedTiles[Tile.m_iHashPrev].m_iHashNext = Tile.m_iHashNext;
			else
				m_liHashFirstDT[Tile.m_iHash] = Tile.m_iHashNext;

			if (Tile.m_iHashNext != -1)
				m_lDecompressedTiles[Tile.m_iHashNext].m_iHashPrev = Tile.m_iHashPrev;

			Tile.m_iHash = -1;
			Tile.m_iHashNext = -1;
			Tile.m_iHashPrev = -1;
		}
	}

	M_INLINE int Hash_GetDT(const CVec3Dint32& _Pos) const
	{
		int iHash = Hash_GetKey(_Pos);
		int iDT = m_liHashFirstDT.GetBasePtr()[iHash];
		const CDecompressedTile* pTiles = m_lDecompressedTiles.GetBasePtr();
		while(iDT >= 0)
		{
			const CDecompressedTile& Tile = pTiles[iDT];
			if (Tile.m_TilePos[0] == _Pos[0] &&
				Tile.m_TilePos[1] == _Pos[1] &&
				Tile.m_TilePos[2] == _Pos[2])
			{
				// ConOut(CStrF("Hash hit %d, %d, %d, %d", pN->m_iNode, _x, _y, _z));
				return iDT;
			}
			iDT = Tile.m_iHashNext;
		}
		return -1;
	}

	int GetTileTile(const CVec3Dint32& _TileTilePos) const;
	void SetTileTile(const CVec3Dint32& _TileTilePos, int _iTileTile);

	void RewindAccessCounters();
	int DecompressTile(int _iTile, const CVec3Dint32& _TilePos);

public:
	xwc_virtual uint GetAt(const CVec3Dint32& _Pos);

	int ReadStruct(CCFile* _pFile);
	void WriteStruct(CCFile* _pFile);

	virtual void Read(CDataFile* _pDFile);
	virtual void Write(CDataFile* _pDFile);
};

// -------------------------------------------------------------------
//  CXR_BlockNav_GridBuilder
// -------------------------------------------------------------------
class CXR_BlockNav_GridBuilder : public CXR_BlockNav_Grid
{
	MRTC_DECLARE;

public:
	CVec3Dint32						m_TileGridDim;	// Dimension in tiles
	TArray<uint16>					m_liTiles;
	TArray<CXR_BlockNav_BitPlane>	m_lBitPlanes;

	CXR_BlockNav_GridBuilder();
	virtual void InitFromGridLayout();
	virtual void Create(const CBox3Dfp32& _BoundBox, fp32 _UnitsPerCell);
	virtual void Create(const CXR_BlockNav_GridLayout& _Grid);

protected:
	int AddTileTile(const CXR_BlockNav_TileTile&);

public:
	virtual void CompileTileTiles();

	// Construction
	virtual int AddBitPlane(const CXR_BlockNav_BitPlane& _BitPlane);
	virtual int AddTile(const CXR_BlockNav_Tile& _Tile);

	virtual int GetTile(const CVec3Dint32& _Pos);
	virtual void SetTile(const CVec3Dint32& _Pos, int _iTile);

	virtual int SetTile(const CVec3Dint32& _Pos, const CXR_BlockNav_BitPlane* _pBitPlanes, int _nBitPlanes);

	virtual int GetTile(const CVec3Dint32& _Pos, CXR_BlockNav_Tile& _Tile, CXR_BlockNav_BitPlane* _pBitPlanes, int _MaxBitPlanes);

	virtual CBox3Dint GetTileDomain(const CBox3Dfp32& _Box);			// Get the tile domain covered by _Box
	virtual CBox3Dfp32 GetTileExtents(const CVec3Dint32& _Pos);
	virtual CVec3Dint32 GetTileCellBase(const CVec3Dint32& _Pos);

	virtual CVec3Dfp32 GetCellExtents();

protected:
	void GetPosTileCellClamped(const CVec3Dint32& _Pos, CVec3Dint32& _Tile, CVec3Dint32& _Cell);
public:
	virtual uint GetAt(const CVec3Dint32& _Pos);
//	virtual void SetAt(const CVec3Dint32& _Pos, int _NewValue);
};

typedef TPtr<CXR_BlockNav_GridBuilder> spCXR_BlockNav_GridBuilder;
typedef TPtr<CXR_BlockNav_Grid> spCXR_BlockNav_Grid;

// -------------------------------------------------------------------



#endif // _INC_XRBlockNavGrid


