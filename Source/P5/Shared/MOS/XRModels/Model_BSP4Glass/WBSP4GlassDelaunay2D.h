#ifndef __WBSP4GlassDelaunay2D_h__
#define __WBSP4GlassDelaunay2D_h__

//#include "../../Classes/Render/MWireContainer.h"
//#include "../Model_BSP4/XW4Common.h"
//#include "WBSP4GlassCommon.h"


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_Delaunay2D
|
| To be removed, not working properly
|__________________________________________________________________________________________________
\*************************************************************************************************/
/*
enum
{
	GLASS_DELAUNAY_TYPE_INT64			= 0,
	GLASS_DELAUNAY_TYPE_INT32,
	GLASS_DELAUNAY_TYPE_INT16,
	GLASS_DELAUNAY_TYPE_INT8,
	GLASS_DELAUNAY_TYPE_RATIONAL,
	GLASS_DELAUNAY_TYPE_FLOAT,

	// Errors
	GLASS_DELAUNAY_ERROR_ALLCLEAR		= 0,
	GLASS_DELAUNAY_ERROR_MAXVERTICES,
	GLASS_DELAUNAY_ERROR_NOTYPE,
	GLASS_DELAUNAY_ERROR_MAXOUTTRIANGLES,
	GLASS_DELAUNAY_ERROR_MAXREMOVEINDICES,
	GLASS_DELAUNAY_ERROR_NOATTRIBUTE,
};

// Glass clamping integers

// Real min/max values for int64's
#define GLASS_REAL_INT64_MAX		 9223372036854775807LL	// 0x7FFF FFFF FFFF FFFF
#define GLASS_REAL_INT64_MIN_		-9223372036854775808LL	// 0x8000 0000 0000 0000
#define GLASS_REAL_INT64_MIN		-9223372036854775807LL	// 0x8000 0000 0000 0001

// Allowed min/max values for int64's (Because of super vertices)
//#define GLASS_INT64_MAX				 2305843009213693951LL	// 1FFF FFFF FFFF FFFF
//#define GLASS_INT64_MIN				-2305843009213693951LL	// E000 0000 0000 0001

#define GLASS_INT64_MAX				 1879048191				// 0x6FFF FFFF
#define GLASS_INT64_MIN				-1879048191				// 0x9000 0001

// Real min/max values for int32's
#define GLASS_REAL_INT32_MAX		 2147483647				// 0x7FFF FFFF
#define GLASS_REAL_INT32_MIN_		-2147483648				// 0x8000 0000
#define GLASS_REAL_INT32_MIN		-2147483647				// 0x8000 0001

// Allowed min/max values for int32's (Because of super vertices)
#define GLASS_INT32_MAX				 536870911				// 0x1FFF FFFF
#define GLASS_INT32_MIN				-536870911				// 0xE000 0001

// Real min/max values for int16's
#define GLASS_REAL_INT16_MAX		 32767					// 0x7FFF
#define GLASS_REAL_INT16_MIN_		-32768					// 0x8000
#define GLASS_REAL_INT16_MIN		-32767					// 0x8001

// Allowed min/max values for int16's (Because of super vertices)
#define GLASS_INT16_MAX				 8191					// 0x1FFF
#define GLASS_INT16_MIN				-8191					// 0xE001

// Real min/max values for int8's
#define GLASS_REAL_INT8_MAX			 127					// 0x7F
#define GLASS_REAL_INT8_MIN_		-128					// 0x80
#define GLASS_REAL_INT8_MIN			-127					// 0x81

// Allowed min/max values for int8's (Because of super vertices)
#define GLASS_INT8_MAX				 31						// 0x1F
#define GLASS_INT8_MIN				-31						// 0xE1


class CBSP4Glass_Delaunay2D
{
	typedef TVector2<int64>		_CVec2Dint64;

	class CDel2DEdge
	{
	public:
		CDel2DEdge() {}
		
		M_INLINE void Set(const int16& _i0, const int16& _i1)
		{
			m_iIndex[0] = _i0;
			m_iIndex[1] = _i1;
		}

		M_INLINE void SwapIndices()
		{
			Swap(m_iIndex[0], m_iIndex[1]);
		}

		// Same edge info ?
		M_INLINE bool operator == (const CDel2DEdge& _Edge) const
		{
			if(_Edge.m_iIndex[0] == m_iIndex[0] && _Edge.m_iIndex[1] == m_iIndex[1])
				return true;
			return false;
		}

		// Shared edge?
		M_INLINE bool operator == (const CVec2Dint16& _iIndex) const
		{
			if((_iIndex[0] == m_iIndex[0] && _iIndex[1] == m_iIndex[1]) ||
			   (_iIndex[1] == m_iIndex[0] && _iIndex[0] == m_iIndex[1]))
			   return true;
			return false;
		}

		M_INLINE void operator = (const CDel2DEdge& _Edge)
		{
			m_iIndex[0] = _Edge.m_iIndex[0];
			m_iIndex[1] = _Edge.m_iIndex[1];
		}

		M_INLINE void operator = (const CVec2Dint16& _iIndex)
		{
			m_iIndex[0] = _iIndex[0];
			m_iIndex[1] = _iIndex[1];
		}

		int16 m_iIndex[2];
	};

public:
	class CDel2DTri
	{
	public:
		CDel2DTri() {}
		~CDel2DTri() {}

		M_INLINE void Set(const uint16 _i0, const uint16 _i1, const uint16 _i2)
		{
			m_iIndex[0] = _i0;
			m_iIndex[1] = _i1;
			m_iIndex[2] = _i2;
			m_iWind[0] = 0;
			m_iWind[1] = 1;
			m_iWind[2] = 2;
		}

		M_INLINE void Set(const uint16 _i0, const uint16 _i1, const uint16 _i2, const uint16 _w0, const uint16 _w1, const uint16 _w2)
		{
			m_iIndex[0] = _i0;
			m_iIndex[1] = _i1;
			m_iIndex[2] = _i2;
			m_iWind[0] = _w0;
			m_iWind[1] = _w1;
			m_iWind[2] = _w2;
		}

		void operator = (const CDel2DTri& _Tri)
		{
			m_iIndex[0] = _Tri.m_iIndex[0];
			m_iIndex[1] = _Tri.m_iIndex[1];
			m_iIndex[2] = _Tri.m_iIndex[2];
			m_iWind[0] = _Tri.m_iWind[0];
			m_iWind[1] = _Tri.m_iWind[1];
			m_iWind[2] = _Tri.m_iWind[2];
			m_CenterInt = _Tri.m_CenterInt;
			m_IntR2	= _Tri.m_IntR2;
			m_IntR = _Tri.m_IntR;
		}

		M_INLINE bool IsLeftOf(_CVec2Dint64& _PntInt)
		{
			return (_PntInt.k[0] > (m_CenterInt.k[0] + m_IntR));
			//return true;
			//return _Point[0] > (m_Center[0] + m_R);
		}

		M_INLINE bool IsCCW(CVec3Dfp32* _pVert, const CVec3Dfp32& _PlaneN)
		{
			const CVec3Dfp32& P0 = _pVert[m_iIndex[0]];
			const CVec3Dfp32& P1 = _pVert[m_iIndex[1]];
			const CVec3Dfp32& P2 = _pVert[m_iIndex[2]];

			const CVec3Dfp32 N = ((P1 - P0) / (P2 - P0)).Normalize();
			const fp32 Dot = (N * _PlaneN);
			return (Dot > 0) ? true : false;
		}

		void SetCircumCircle(const _CVec2Dint64* _pPntsInt);
		bool CCEncompasses(const _CVec2Dint64& _PntInt);

		uint16	m_iIndex[3];	// Point index
		uint8	m_iWind[3];		// Point index winding

		// Integer data types
		_CVec2Dint64	m_CenterInt;
		int64			m_IntR2;
		int64			m_IntR;
	};

public:
	CBSP4Glass_Delaunay2D(uint8 _iType, CGlassAttrib* _pAttrib, int32* _pSeed = NULL, CMat4Dfp32* _pWMat = NULL);

	// Upload original mesh information to delaunay
	bool UploadBaseGeometry(CVec3Dfp32* _pVertices, uint32 _nVertices, uint16* _piPrim, uint32 _nPrim, uint32 _nGen, bool _bOwner);

	// Generate random points on surface
	//void GeneratePoints(const uint32 _nPoints);

	// Triangulate
	bool	Triangulate(TThinArray<CVec3Dfp32>& _lOutputVertices);

	// Get generated triangles
	const CDel2DTri* GetOutputTriangles() const
	{
		return m_lTriOut.GetBasePtr();
	}

	// Get number of generated triangles
	int GetNumTriangles() const
	{
		return m_nTriOut;
	}

private:

	// Triangulate using int mapping
	void	TriangulateInt();
	void	CalcIntMapping();
	void	RestoreMappingFromInt(TThinArray<CVec3Dfp32>& _lOutputVertices);

	// Point genereration in int mapping space
	void	GeneratePointsInt(const uint32 _nPoints);

	// Error logging
	bool	ErrorCode(const uint32 _ErrorCode);

	// Edge handling
	void	HandleEdge(CDel2DTri& _Tri, CDel2DEdge* _pEdges, int& _nEdges);
	int		FindEdge(CDel2DEdge& _Edge, CDel2DEdge* _pEdges, int& _nEdges);

	// Input
	CVec3Dfp32*	m_pVert;		// Original 3d vertices
	//CVec3Dfp32*	m_pGenVert;		// Genereated 3d vertices
	uint32		m_nVert;		// Number of vertices
	uint16*		m_piPrim;		// Primitive indices
	uint32		m_nPrim;		// Number of primitives
	bool		m_bOwner;		// Does delaunay own the vertices?
    uint8		m_iType;		// Mapping type
	bool		m_bCCW;			// Counter clockwise geometry
	uint32		m_nGen;			// Number of generated points
	int			m_nTriOut;		// Number of valid triangles in output
	
	CGlassAttrib*	m_pAttrib;	// Glass attributes
	CMat4Dfp32		m_WMat;
	int32			m_Seed;		// Seeding value

	// Max / Min values
	int64		m_Max_Int;
	int64		m_Min_Int;

	// Int64 vertices
	TThinArray<_CVec2Dint64>	m_lVertInt64;

	// Delaunay triangles
	TThinArray<CDel2DTri>		m_lTri;
	TThinArray<CDel2DTri>		m_lTriOut;
};
*/























/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Glass_Delaunay2D
|__________________________________________________________________________________________________
\*************************************************************************************************/
/*
class CXR_Glass_Delaunay2D
{
public:
	// Delaunay2D edge class used in triangulation routine
	class CDelaunay2DEdge
	{
	public:
		CVec2Dint16				m_iIndex;	// Index of point

		CDelaunay2DEdge() : m_iIndex(-1) { }
		CDelaunay2DEdge(const int16& _i0, const int16& _i1) : m_iIndex(_i0,_i1) {}
		
		void Set(const int16& _i0, const int16& _i1)
		{
			m_iIndex = CVec2Dint16(_i0, _i1);
		}

		bool operator == (const CDelaunay2DEdge& _Edge) const
		{
			const CVec2Dint16& iIndex = _Edge.m_iIndex;
			
			if(iIndex[0] == m_iIndex[0] && iIndex[1] == m_iIndex[1])
				return true;
			return false;
		}

		bool operator == (const CVec2Dint16& _iIndex) const
		{
			if((_iIndex.k[0] == m_iIndex.k[0] && _iIndex.k[1] == m_iIndex.k[1]) ||
			   (_iIndex.k[1] == m_iIndex.k[0] && _iIndex.k[0] == m_iIndex.k[1]))
			   return true;
			return false;
		}

		void operator = (const CDelaunay2DEdge& _Edge)
		{
			m_iIndex = _Edge.m_iIndex;
		}
	};


public:
	// Delaunay2D triangle class used in triangulation routine
	class CDelaunay2DTriangle
	{
	protected:
		fp32					m_R, m_R2;

	public:
		CVec3Dint16			m_iIndex;
		CVec2Dfp32			m_Center;

		CDelaunay2DTriangle() : m_R(0), m_R2(0), m_iIndex(0) { }
		CDelaunay2DTriangle(const CVec2Dfp32* _pPoints, const int16 _i0 = -1, const int16& _i1 = -2, const int16& _i2 = -3)// : m_iIndex(_i0, _i1, _i2)
		{
			uint8 index[3] =  { _i0, _i1, _i2 };
			if((_pPoints[index[1]].k[0] < _pPoints[index[0]].k[0]) || (_pPoints[index[1]].k[0] == _pPoints[index[0]].k[0] && _pPoints[index[1]].k[1] < _pPoints[index[0]].k[1]))
				Swap(index[0], index[1]);
			if((_pPoints[index[2]].k[0] < _pPoints[index[0]].k[0]) || (_pPoints[index[2]].k[0] == _pPoints[index[0]].k[0] && _pPoints[index[2]].k[1] < _pPoints[index[0]].k[1]))
				Swap(index[0], index[2]);
			if((_pPoints[index[2]].k[0] < _pPoints[index[1]].k[0]) || (_pPoints[index[2]].k[0] == _pPoints[index[1]].k[0] && _pPoints[index[2]].k[1] < _pPoints[index[1]].k[1]))
				Swap(index[1], index[2]);
			m_iIndex = CVec3Dint16(index[0], index[1], index[2]);

			SetCircumCircle(_pPoints);
		}

		CDelaunay2DTriangle(const CVec2Dfp32* _pPoints, const CVec3Dint16 _iIndex)// : m_iIndex(_iIndex)
		{
			uint8 index[3] =  { _iIndex.k[0], _iIndex.k[1], _iIndex.k[2] };
			if((_pPoints[index[1]].k[0] < _pPoints[index[0]].k[0]) || (_pPoints[index[1]].k[0] == _pPoints[index[0]].k[0] && _pPoints[index[1]].k[1] < _pPoints[index[0]].k[1]))
				Swap(index[0], index[1]);
			if((_pPoints[index[2]].k[0] < _pPoints[index[0]].k[0]) || (_pPoints[index[2]].k[0] == _pPoints[index[0]].k[0] && _pPoints[index[2]].k[1] < _pPoints[index[0]].k[1]))
				Swap(index[0], index[2]);
			if((_pPoints[index[2]].k[0] < _pPoints[index[1]].k[0]) || (_pPoints[index[2]].k[0] == _pPoints[index[1]].k[0] && _pPoints[index[2]].k[1] < _pPoints[index[1]].k[1]))
				Swap(index[1], index[2]);
			m_iIndex = CVec3Dint16(index[0], index[1], index[2]);

			SetCircumCircle(_pPoints);
		}

		CDelaunay2DTriangle(const CVec2Dfp32* _pPoints, const CVec2Dint16 _iIndex01, const int16& _i2)// : m_iIndex(_iIndex01.k[0], _iIndex01.k[1], _i2)
		{
			uint8 index[3] =  { _iIndex01.k[0], _iIndex01.k[1], _i2 };
			if((_pPoints[index[1]].k[0] < _pPoints[index[0]].k[0]) || (_pPoints[index[1]].k[0] == _pPoints[index[0]].k[0] && _pPoints[index[1]].k[1] < _pPoints[index[0]].k[1]))
				Swap(index[0], index[1]);
			if((_pPoints[index[2]].k[0] < _pPoints[index[0]].k[0]) || (_pPoints[index[2]].k[0] == _pPoints[index[0]].k[0] && _pPoints[index[2]].k[1] < _pPoints[index[0]].k[1]))
				Swap(index[0], index[2]);
			if((_pPoints[index[2]].k[0] < _pPoints[index[1]].k[0]) || (_pPoints[index[2]].k[0] == _pPoints[index[1]].k[0] && _pPoints[index[2]].k[1] < _pPoints[index[1]].k[1]))
				Swap(index[1], index[2]);
			m_iIndex = CVec3Dint16(index[0], index[1], index[2]);

			SetCircumCircle(_pPoints);
		}

		bool CCEncompasses(const CVec2Dfp32& _Point) const;
		bool IsLeftOf(const CVec2Dfp32& _Point) const
		{
			return _Point[0] > (m_Center[0] + m_R);
		}

		bool TriEncompasses(const CVec2Dfp32* _pPoints2D, const CVec2Dfp32& _Point) const;
		fp32 IsLeftOfSegment(const CVec2Dfp32& _Point, const CVec2Dfp32& _Point0, const CVec2Dfp32& _Point1) const;

		void operator = (const CDelaunay2DTriangle& _Triangle)
		{
			m_R = _Triangle.m_R;
			m_R2 = _Triangle.m_R2;
			m_iIndex = _Triangle.m_iIndex;
			m_Center = _Triangle.m_Center;
		}

	protected:
		void SetCircumCircle(const CVec2Dfp32* _pPoints);
	};

public:
	// Point indices after sort
	TThinArray<uint16>		m_liPoints2D;

protected:
	int32				m_Seed;
	uint16				m_nBasePoints;
	
	TThinArray<CVec2Dfp32>		m_lPoints2D;
	TThinArray<CDelaunay2DTriangle>	m_lTriangles;

public:
	CXR_Glass_Delaunay2D(const int32& _Seed = 0);
	~CXR_Glass_Delaunay2D();

	// Build base triangulated primitive to handle point insertions on
	//void SetBasePrimitiveRecalc(const CVec2Dfp32* _pPoints, const uint32& _nPoints, const uint16* _piPrim, const uint32& _nPrim);
	void SetBasePrimitiveRecalc(const CVec3Dfp32* _pVerts, const uint32& _nVerts, const CGlassAttrib& _Attrib, const uint16* _piPrim, const uint32& _nPrim);
	
	// Base edge handling, for adding and removing shared edges
	//void AddBaseEdge(CDelaunay2DBaseEdge* _pEdges, int& _nBaseEdges, int& _nShared, const uint16& _i1, const uint16& _i2, const CVec2Dfp32* _pPoints2D);
	//void RemoveSharedBaseEdges(CDelaunay2DBaseEdge* _pEdges, int& _nBaseEdges, int& _nShared);

	// Point scattering, over whole surface and around certain positions
	void GeneratePoints(const int& _nPoints);
	void GeneratePoints(const int& _nPoints, const CVec2Dfp32& _LocalPosition);
	bool GeneratePoints(const CVec2Dfp32& _LocalPosition, const int& _nPoints, const fp32& _Area);

	// Point scattering, over whole surface and around certain position and surface edges
	void GenerateSurfacePoints(const int& _nPoints, const CVec2Dfp32& _LocalPosition);

	// Takes care of triangulation of a 2d shape
	void Triangulate();
	void Triangulate(const uint32* piPoints2D, const int& _i0, const int& _i1, const int& _i2, const int& _i3);

	// Edge finding and adding
	int16 FindEdge(const CDelaunay2DEdge& _Edge, const CDelaunay2DEdge* _pEdges, const uint32& _nEdges);
	void HandleEdge(const CVec3Dint16& _iIndex, const CVec2Dfp32* _pPoints2D, CDelaunay2DEdge* _pEdges, uint32& _nEdges);

	void SelectTriangles(const CVec2Dfp32& _LocalPosition, const fp32& _Area, TThinArray<uint8>& _lTris, uint16& _nTris, TThinArray<uint16>& _lPnts, uint16& _nPnts);

	// Get functions
	CDelaunay2DTriangle*	GetTrianglesBasePtr()	{ return m_lTriangles.GetBasePtr(); }
	//CDelaunay2DTriangle**	GetTrianglesBasePtr()	{ return m_lpTriangles.GetBasePtr(); }
	CVec2Dfp32*				GetPointsBasePtr()		{ return m_lPoints2D.GetBasePtr(); }

	//int						GetNumTriangles()		{ return m_lpTriangles.Len(); }
	int						GetNumTriangles()		{ return m_lTriangles.Len(); }
	int						GetNumPoints()			{ return m_lPoints2D.Len(); }
};
*/

#endif
