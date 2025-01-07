#include "PCH.h"
#include "WModel_GlassSystem.h"
#include "../WObj_Weapons/WObj_Projectile_SE.h"

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_GlassSystem, CXR_Model_Custom);

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Delaunay2D
|__________________________________________________________________________________________________
\*************************************************************************************************/
bool CXR_Delaunay2D::CDelaunay2DTriangle::CCEncompasses(const CVec2Dfp32& _Point) const
{
	const CVec2Dfp32 Dist = _Point - m_Center;
	const fp32 DistLen = (Dist[0]*Dist[0] + Dist[1]*Dist[1]);
	return (DistLen <= m_R2);
}


bool CXR_Delaunay2D::CDelaunay2DTriangle::TriEncompasses(const CVec2Dfp32* _pPoints2D, const CVec2Dfp32& _Point) const
{
	const CVec2Dfp32& Point0 = _pPoints2D[m_iIndex[0]];
	const CVec2Dfp32& Point1 = _pPoints2D[m_iIndex[1]];
	const CVec2Dfp32& Point2 = _pPoints2D[m_iIndex[2]];

	if(IsLeftOfSegment(_Point, Point0, Point1) >= 0.0f)
	{
		if(IsLeftOfSegment(_Point, Point1, Point2) >= 0.0f &&
		   IsLeftOfSegment(_Point, Point2, Point0) >= 0.0f)
		   return true;
	}
	else
	{
		if(IsLeftOfSegment(_Point, Point1, Point2) < 0.0f &&
		   IsLeftOfSegment(_Point, Point2, Point0) < 0.0f)
		   return true;
	}

	return false;
}


fp32 CXR_Delaunay2D::CDelaunay2DTriangle::IsLeftOfSegment(const CVec2Dfp32& _Point, const CVec2Dfp32& _Point0, const CVec2Dfp32& _Point1) const
{
	const CVec2Dfp32 P0 = _Point - _Point0;
	const CVec2Dfp32 P2 = (_Point1 - _Point0);
	const CVec2Dfp32 P1 = CVec2Dfp32(-P2[1], P2[0]);

	return (P0[0]*P1[0] + P0[1]*P1[1]);
}


void CXR_Delaunay2D::CDelaunay2DTriangle::SetCircumCircle(const CVec2Dfp32* _pPoints)
{
	const CVec2Dfp32& Point0 = _pPoints[m_iIndex[0]];
	const CVec2Dfp32& Point1 = _pPoints[m_iIndex[1]];
	const CVec2Dfp32& Point2 = _pPoints[m_iIndex[2]];
	fp32 x0 = Point0[0]; const fp32& y0 = Point0[1]; fp32 x1 = Point1[0]; const fp32& y1 = Point1[1];
	const fp32& x2 = Point2[0]; const fp32& y2 = Point2[1]; const fp32 y10 = y1 - y0; const fp32 y21 = y2 - y1;
	const bool b21zero = y21 > -_FP32_EPSILON && y21 < _FP32_EPSILON;
	if(y10 > -_FP32_EPSILON && y10 < _FP32_EPSILON)
	{
		if(b21zero)
		{
			if(x1 > x0) { if(x2 > x1) x1 = x2; }
			else		{ if(x2 < x0) x0 = x2; }
			m_Center = CVec2Dfp32((x0 + x1) * 0.5f, y0);
		}
		else
		{
			m_Center[0] = (x0 + x1) * 0.5f;
			m_Center[1] = (- (x2 - x1) / y21) * (m_Center[0] - ((x1 + x2) * 0.5f)) + ((y1 + y2) * 0.5f);
		}
	}
	else if(b21zero)
	{
		m_Center[0] = (x1 + x2) * 0.5f;
		m_Center[1] = (- (x1 - x0) / y10) * (m_Center[0] - ((x0 + x1) * 0.5f)) + ((y0 + y1) * 0.5f);
	}
	else
	{
		fp32 m0 = - (x1 - x0) / y10; fp32 m1 = - (x2 - x1) / y21;
		fp32 mx0 = (x0 + x1) * 0.5f; fp32 my0 = (y0 + y1) * 0.5f;
		m_Center[0] = (m0 * mx0 - m1 * ((x1 + x2) * 0.5f) + ((y1 + y2) * 0.5f) - my0) / (m0 - m1);
		m_Center[1] = m0 * (m_Center[0] - mx0) + my0;
	}

	CVec2Dfp32 t = CVec2Dfp32(x0,y0) - m_Center;
	m_R2 = t[0] * t[0] + t[1] * t[1];
	m_R = M_Sqrt(m_R2) * 1.00001f;
	//m_R = M_Sqrt(m_R2) * 1.000001f;		// Make radius slightly bigger
}


CXR_Delaunay2D::CXR_Delaunay2D(const int32& _Seed)
	: m_Seed(_Seed)
	, m_nBasePoints(0)
{
	m_lOutput.SetLen(0);
	m_lPoints2D.SetLen(0);
	m_liPoints2D.SetLen(0);
}


CXR_Delaunay2D::~CXR_Delaunay2D()
{
	m_lPoints2D.SetLen(0);
	m_lOutput.SetLen(0);
	m_liPoints2D.SetLen(0);
	m_nBasePoints = 0;
}


int16 CXR_Delaunay2D::FindEdge(const CDelaunay2DEdge& _Edge, const TArray<CDelaunay2DEdge>& _lEdgeList)
{
	const int& Len = _lEdgeList.Len();
	const CDelaunay2DEdge* pEdges = _lEdgeList.GetBasePtr();

	for(int16 i = 0; i < Len; i++)
	{
		if(pEdges[i] == _Edge) return i;
	}

	return -1;
}


void CXR_Delaunay2D::HandleEdge(int16& _i0, int16& _i1, TArray<CDelaunay2DEdge>& _lEdgeList)
{
	const CVec2Dfp32* pPoints2D = m_lPoints2D.GetBasePtr();
	const CVec2Dfp32& Point0 = pPoints2D[_i0];
	const CVec2Dfp32& Point1 = pPoints2D[_i1];
	
//	CDelaunay2DEdge edge(_i1,_i0);

//	if((Point0[0] == Point1[0] && Point0[1] < Point1[1])) edge.Set(_i0,_i1);
//	else if((Point0[0] < Point1[0])) edge.Set(_i0,_i1);

	CDelaunay2DEdge edge(_i1,_i0);
	if(Point0[0] < Point1[0]) edge.Set(_i0,_i1);
	else if(Point0[0] == Point1[0] && Point0[1] < Point1[1]) edge.Set(_i0,_i1);
	
	const int16 EdgeIndex = FindEdge(edge, _lEdgeList);
	
	// If edge doesn't exist, we add it, otherwise we remove it
	if(EdgeIndex < 0) _lEdgeList.Add(edge);
	else _lEdgeList.Del(EdgeIndex);
}


void CXR_Delaunay2D::AddBaseTriangleRecalc(const CVec2Dfp32* _pPoints)
{
	const uint32 nPoints = m_lPoints2D.Len();
//	const uint32 niPoints = m_liPoints2D.Len();

	m_lPoints2D.SetGrow(3);
	m_liPoints2D.SetGrow(3);

	// Bias 2d unit points from 0->1 to -1->1
	CVec2Dfp32 Point0 = _pPoints[0] - CVec2Dfp32(0.5f);
	CVec2Dfp32 Point1 = _pPoints[1] - CVec2Dfp32(0.5f);
	CVec2Dfp32 Point2 = _pPoints[2] - CVec2Dfp32(0.5f);

	Point0 = CVec2Dfp32(Point0[0]*2.0f, Point0[1] * -2.0f);
	Point1 = CVec2Dfp32(Point1[0]*2.0f, Point1[1] * -2.0f);
	Point2 = CVec2Dfp32(Point2[0]*2.0f, Point2[1] * -2.0f);

	// Don't handle shared points
//	const uint8 POINT0 = M_Bit(0);
//	const uint8 POINT1 = M_Bit(1);
//	const uint8 POINT2 = M_Bit(2);
	uint8  AddPoint = M_Bit(0) | M_Bit(1) | M_Bit(2);
	uint16 iIndex[3] = {0, 0, 0};

	const CVec2Dfp32* pPoints2DPre = m_lPoints2D.GetBasePtr();
	for(int i = 0; i < nPoints; i++)
	{
		const CVec2Dfp32& TestPoint = pPoints2DPre[i];
		if(Point0[0] == TestPoint[0] && Point0[1] == TestPoint[1]) { iIndex[0] = i; AddPoint &= ~M_Bit(0); }
		if(Point1[0] == TestPoint[0] && Point1[1] == TestPoint[1]) { iIndex[1] = i; AddPoint &= ~M_Bit(1); }
		if(Point2[0] == TestPoint[0] && Point2[1] == TestPoint[1]) { iIndex[2] = i; AddPoint &= ~M_Bit(2); }
	}

	int iPointAdd = nPoints;
	if(AddPoint & M_Bit(0))
	{
		m_lPoints2D.Insert(iPointAdd, Point0);
		iIndex[0] = iPointAdd++;
	}
	if(AddPoint & M_Bit(1))
	{
		m_lPoints2D.Insert(iPointAdd, Point1);
		iIndex[1] = iPointAdd++;
	}
	if(AddPoint & M_Bit(2))
	{
		m_lPoints2D.Insert(iPointAdd, Point2);
		iIndex[2] = iPointAdd;
	}

	const CVec2Dfp32* pPoints2D = m_lPoints2D.GetBasePtr();
	uint16* piPoints2D = m_liPoints2D.GetBasePtr();
	int iPoints2DLen = m_liPoints2D.Len();
	int iListPos = 0;
	int iBasePoints = 0;
    if(AddPoint & M_Bit(0))
	{
		for(iListPos = 0; iListPos < iPoints2DLen; iListPos++)
		{
			const CVec2Dfp32& TestPoint = pPoints2D[piPoints2D[iListPos]];
			
			if(TestPoint[0] == Point0[0] && TestPoint[1] > Point0[1]) break;
			else if(TestPoint[0] > Point0[0]) break;
		}
		m_liPoints2D.Insert(iListPos, nPoints);
		iPoints2DLen++;
		iBasePoints++;
	}

	if(AddPoint & M_Bit(1))
	{
		piPoints2D = m_liPoints2D.GetBasePtr();
		for(iListPos = 0; iListPos < iPoints2DLen; iListPos++)
		{
			const CVec2Dfp32& TestPoint = pPoints2D[piPoints2D[iListPos]];
			
			if(TestPoint[0] == Point1[0] && TestPoint[1] > Point1[1]) break;
			else if(TestPoint[0] > Point1[0]) break;
		}
		m_liPoints2D.Insert(iListPos, nPoints+iBasePoints);
		iPoints2DLen++;
		iBasePoints++;
	}

	if(AddPoint & M_Bit(2))
	{
		piPoints2D = m_liPoints2D.GetBasePtr();
		for(iListPos = 0; iListPos < iPoints2DLen; iListPos++)
		{
			const CVec2Dfp32& TestPoint = pPoints2D[piPoints2D[iListPos]];
			
			if(TestPoint[0] == Point2[0] && TestPoint[1] > Point2[1]) break;
			else if(TestPoint[0] > Point2[0]) break;
		}
		m_liPoints2D.Insert(iListPos, nPoints+iBasePoints);
		iBasePoints++;
	}

	piPoints2D = m_liPoints2D.GetBasePtr();
	m_lBaseTriangle.Add(CDelaunay2DTriangle(pPoints2D, iIndex[0], iIndex[1], iIndex[2]));
	m_nBasePoints += iBasePoints;

//	HandleEdge(iIndex[0], iIndex[1], m_lBaseEdges);
//	HandleEdge(iIndex[1], iIndex[2], m_lBaseEdges);
//	HandleEdge(iIndex[2], iIndex[0], m_lBaseEdges);
}


void CXR_Delaunay2D::AddBaseTriangle(const CVec2Dfp32* _pPoints)
{
	//const uint32 niPoints = m_lPoints2D.SetLen
	const uint32 nPoints = m_lPoints2D.Len();
//	const uint32 niPoints = m_liPoints2D.Len();
	
	m_lPoints2D.SetGrow(3);
	m_liPoints2D.SetGrow(3);

	const CVec2Dfp32& Point0 = _pPoints[0];
	const CVec2Dfp32& Point1 = _pPoints[1];
	const CVec2Dfp32& Point2 = _pPoints[2];

	m_lPoints2D.Insert(nPoints, Point0);
	m_lPoints2D.Insert(nPoints+1, Point1);
	m_lPoints2D.Insert(nPoints+2, Point2);

	const CVec2Dfp32* pPoints2D = m_lPoints2D.GetBasePtr();
	uint16* piPoints2D = m_liPoints2D.GetBasePtr();
	int iPoints2DLen = m_liPoints2D.Len();
	int iListPos = 0;
    for(iListPos = 0; iListPos < iPoints2DLen; iListPos++)
	{
		const CVec2Dfp32& TestPoint = pPoints2D[piPoints2D[iListPos]];
		
		if(TestPoint[0] == Point0[0] && TestPoint[1] > Point0[1]) break;
		else if(TestPoint[0] > Point0[0]) break;
	}
	m_liPoints2D.Insert(iListPos, nPoints);

	iPoints2DLen++;
	piPoints2D = m_liPoints2D.GetBasePtr();
	for(iListPos = 0; iListPos < iPoints2DLen; iListPos++)
	{
		const CVec2Dfp32& TestPoint = pPoints2D[piPoints2D[iListPos]];
		
		if(TestPoint[0] == Point1[0] && TestPoint[1] > Point1[1]) break;
		else if(TestPoint[0] > Point1[0]) break;
	}
	m_liPoints2D.Insert(iListPos, nPoints+1);

	iPoints2DLen++;
    for(iListPos = 0; iListPos < iPoints2DLen; iListPos++)
	{
		const CVec2Dfp32& TestPoint = pPoints2D[piPoints2D[iListPos]];
		
		if(TestPoint[0] == Point2[0] && TestPoint[1] > Point2[1]) break;
		else if(TestPoint[0] > Point2[0]) break;
	}
	m_liPoints2D.Insert(iListPos, nPoints+2);

	m_nBasePoints += 3;
}


void CXR_Delaunay2D::GeneratePoints(const int& _nPoints)
{
	// Get 2d point data
	TThinArray<CVec2Dfp32> lTempPoints;
	lTempPoints.SetLen(_nPoints);
	CVec2Dfp32* pTempPoints = lTempPoints.GetBasePtr();

	const uint32 nPoints = _nPoints + m_nBasePoints;
	m_lPoints2D.Insertx(m_nBasePoints, pTempPoints, _nPoints);
//	const uint32 nPoints = _nPoints + 4;
//	m_lPoints2D.SetLen(nPoints);
	CVec2Dfp32* pPoints2D = m_lPoints2D.GetBasePtr();
	
	lTempPoints.SetLen(0);
	pTempPoints = NULL;
	
	const int32 iGrow = nPoints - m_liPoints2D.Len();//m_AllocLen;
	if(iGrow > 0) m_liPoints2D.SetGrow(iGrow);
//	m_liPoints2D.SetLen(0);
//	m_liPoints2D.SetGrow(nPoints);
		//m_liPoints2D.Insert(m_nBasePoints, _nPoints);
	
	int32 Seed = m_Seed;

	// Setup base points
//	int BasePos = 0;
//	m_liPoints2D.Insert(0, BasePos++);
//	m_liPoints2D.Insert(1, BasePos++);
//	m_liPoints2D.Insert(2, BasePos++);
//	m_liPoints2D.Insert(3, BasePos++);

//	pPoints2D[0] = CVec2Dfp32(-1.0f, -1.0f);
//	pPoints2D[1] = CVec2Dfp32(-1.0f,  1.0f);
//	pPoints2D[2] = CVec2Dfp32( 1.0f, -1.0f);
//	pPoints2D[3] = CVec2Dfp32( 1.0f,  1.0f);

	// Generate random points
	int iPoints2DLen = m_liPoints2D.Len();
//	int iListPos = 0;
	int iListPos = m_nBasePoints;
	uint16* piPoints2D = m_liPoints2D.GetBasePtr();
//	for(int i = 4; i < nPoints; i++)
	for(int i = m_nBasePoints; i < nPoints; i++)
	{
		CVec2Dfp32& Point2D = pPoints2D[i];

		Point2D = CVec2Dfp32(MFloat_GetRand(Seed) - 0.5f, MFloat_GetRand(Seed+1) - 0.5f) * 2.0f;
		Seed += 2;

		// insert into sorted list
		piPoints2D = m_liPoints2D.GetBasePtr();
		for(iListPos = 0; iListPos < iPoints2DLen; iListPos++)
		{
			const CVec2Dfp32& TestPoint = pPoints2D[piPoints2D[iListPos]];
			
			if(TestPoint[0] == Point2D[0] && TestPoint[1] > Point2D[1]) break;
			else if(TestPoint[0] > Point2D[0]) break;
		}
		
		m_liPoints2D.Insert(iListPos, i);
		iPoints2DLen++;
	}
}


// Experimental
bool CXR_Delaunay2D::GeneratePoints(const CVec2Dfp32& _LocalPosition, const int& _nPoints, const fp32& _Area)
{
	// Get 2d point data
	//TArray<uint32> liPoints2D;

	TThinArray<CVec2Dfp32> lTempPoints;
	lTempPoints.SetLen(_nPoints);
	CVec2Dfp32* pTempPoints = lTempPoints.GetBasePtr();

	const uint32 nPoints = _nPoints + m_nBasePoints;
		
	lTempPoints.SetLen(0);
	pTempPoints = NULL;

	const int32 iGrow = nPoints - m_liPoints2D.Len();//m_liPoints2D.m_AllocLen;
	if(iGrow > 0)
	{
		m_liPoints2D.SetGrow(iGrow);
		m_lPoints2D.SetGrow(iGrow);
	}
	
	int32 Seed = m_Seed;

	// Generate random points and make sure each new point is inside one of our base triangles
	CVec2Dfp32 MaxPos = _LocalPosition + CVec2Dfp32(_Area);
	CVec2Dfp32 MinPos = _LocalPosition - CVec2Dfp32(_Area);
	
	MaxPos[0] = MaxMT(1.0f, MaxPos[0]);
	MaxPos[1] = MaxMT(1.0f, MaxPos[1]);

	MinPos[0] = MinMT(-1.0f, MinPos[0]);
	MinPos[1] = MinMT(-1.0f, MinPos[1]);

	const CXR_Delaunay2D::CDelaunay2DTriangle* pBaseTris = m_lBaseTriangle.GetBasePtr();
	const uint32& nBaseTris = m_lBaseTriangle.Len();
	int iInsert = m_nBasePoints;
	CVec2Dfp32* pPoints2D = m_lPoints2D.GetBasePtr();
	for(int i = 0; i < _nPoints; i++)
	{
		CVec2Dfp32 Point2D = _LocalPosition + ((i > 0) ? ((CVec2Dfp32(MFloat_GetRand(Seed) - 0.5f, MFloat_GetRand(Seed+1) - 0.5f) * 2.0f) * _Area) : CVec2Dfp32(0.0f,0.0f));
		
		// Clamp value
		if(Point2D[0] < MinPos[0]) Point2D[0] = MinPos[0];
		else if(Point2D[0] > MaxPos[0]) Point2D[1] = MaxPos[0];

		if(Point2D[1] < MinPos[1]) Point2D[1] = MinPos[1];
		else if(Point2D[1] > MaxPos[1]) Point2D[1] = MaxPos[1];
		
		Seed += 2;

		// Point need to be inside one of the triangles set up as base
		pPoints2D = m_lPoints2D.GetBasePtr();
		for(int j = 0; j < nBaseTris; j++)
		{
			if(pBaseTris[j].TriEncompasses(pPoints2D, Point2D))
			{
				m_lPoints2D.Insert(iInsert++, Point2D);
				break;
			}
		}

		if(i == 0 && iInsert == m_nBasePoints) return false;
	}

	// Add points into sorted list
	int iPoints2DLen = m_liPoints2D.Len();
	int iListPos = m_nBasePoints;
	uint16* piPoints2D = m_liPoints2D.GetBasePtr();
	pPoints2D = m_lPoints2D.GetBasePtr();
	for(int i = m_nBasePoints; i < iInsert; i++)
	{
		CVec2Dfp32& Point2D = pPoints2D[i];

		// insert into sorted list
		piPoints2D = m_liPoints2D.GetBasePtr();
		for(iListPos = 0; iListPos < iPoints2DLen; iListPos++)
		{
			const CVec2Dfp32& TestPoint = pPoints2D[piPoints2D[iListPos]];
			
			if(TestPoint[0] == Point2D[0] && TestPoint[1] > Point2D[1]) break;
			else if(TestPoint[0] > Point2D[0]) break;
		}
		
		m_liPoints2D.Insert(iListPos, i);
		iPoints2DLen++;
	}

	return true;
}
/*
void CXR_Delaunay2D::GeneratePoints(const CVec2Dfp32& _LocalPosition, const int& _nPoints, const fp32& _Area)
{
	// Get 2d point data
	//TArray<uint32> liPoints2D;

	TThinArray<CVec2Dfp32> lTempPoints;
	lTempPoints.SetLen(_nPoints);
	CVec2Dfp32* pTempPoints = lTempPoints.GetBasePtr();

	const uint32 nPoints = _nPoints + m_nBasePoints;
	m_lPoints2D.Insertx(m_nBasePoints, pTempPoints, _nPoints);
	//m_lPoints2D.SetLen(nPoints);
	CVec2Dfp32* pPoints2D = m_lPoints2D.GetBasePtr();
	
	lTempPoints.SetLen(0);
	pTempPoints = NULL;

	const int32 iGrow = nPoints - m_liPoints2D.Len();//m_liPoints2D.m_AllocLen;
	if(iGrow > 0) m_liPoints2D.SetGrow(iGrow);
	//m_liPoints2D.SetLen(0);
	//m_liPoints2D.SetGrow(nPoints);
	//m_liPoints2D.Insert(m_nBasePoints, _nPoints);
	
	int32 Seed = m_Seed;

	// Setup base points
//	int BasePos = 0;
//	m_liPoints2D.Insert(0, BasePos++);
//	m_liPoints2D.Insert(1, BasePos++);
//	m_liPoints2D.Insert(2, BasePos++);
//	m_liPoints2D.Insert(3, BasePos++);

//	pPoints2D[0] = CVec2Dfp32(-1.0f, -1.0f);
//	pPoints2D[1] = CVec2Dfp32(-1.0f,  1.0f);
//	pPoints2D[2] = CVec2Dfp32( 1.0f, -1.0f);
//	pPoints2D[3] = CVec2Dfp32( 1.0f,  1.0f);

	// Generate random points
	int iPoints2DLen = m_liPoints2D.Len();
	int iListPos = m_nBasePoints;
	uint16* piPoints2D = m_liPoints2D.GetBasePtr();
	
	CVec2Dfp32 MaxPos = _LocalPosition + CVec2Dfp32(_Area);
	CVec2Dfp32 MinPos = _LocalPosition - CVec2Dfp32(_Area);
	
	MaxPos[0] = MaxMT(1.0f, MaxPos[0]);
	MaxPos[1] = MaxMT(1.0f, MaxPos[1]);

	MinPos[0] = MinMT(-1.0f, MinPos[0]);
	MinPos[1] = MinMT(-1.0f, MinPos[1]);

	for(int i = m_nBasePoints; i < nPoints; i++)
	{
		CVec2Dfp32& Point2D = pPoints2D[i];
		
		Point2D = _LocalPosition + ((CVec2Dfp32(MFloat_GetRand(Seed) - 0.5f, MFloat_GetRand(Seed+1) - 0.5f) * 2.0f) * _Area);
		
		// Clamp value
		if(Point2D[0] < MinPos[0]) Point2D[0] = MinPos[0];
		else if(Point2D[0] > MaxPos[0]) Point2D[1] = MaxPos[0];

		if(Point2D[1] < MinPos[1]) Point2D[1] = MinPos[1];
		else if(Point2D[1] > MaxPos[1]) Point2D[1] = MaxPos[1];
		
		Seed += 2;

		// insert into sorted list
		piPoints2D = m_liPoints2D.GetBasePtr();
		for(iListPos = 0; iListPos < iPoints2DLen; iListPos++)
		{
			const CVec2Dfp32& TestPoint = pPoints2D[piPoints2D[iListPos]];
			
			if(TestPoint[0] == Point2D[0] && TestPoint[1] > Point2D[1]) break;
			else if(TestPoint[0] > Point2D[0]) break;
		}
		
		m_liPoints2D.Insert(iListPos, i);
		iPoints2DLen++;
	}
}
*/


void CXR_Delaunay2D::Triangulate()
{
	// Get 2d points data
	#ifdef GLASS_SYSTEM_TRACE
		M_TRACEALWAYS("Delaunay2D Triangulation:\n");
	#endif

	TArray<CDelaunay2DTriangle> lWorkTris;

	const TArray<CVec2Dfp32>& lPoints2D = m_lPoints2D;
	const TArray<uint16>& liPoints2D = m_liPoints2D;
	
	const CVec2Dfp32* pPoints2D = lPoints2D.GetBasePtr();
	const uint16* piPoints2D = liPoints2D.GetBasePtr();

	const int& nPoints = lPoints2D.Len();
	
	m_lOutput.SetLen(0);

	#ifdef GLASS_SYSTEM_TRACE
		int niPoints = liPoints2D.Len();
		#ifdef GLASS_SYSTEM_TRACE
		M_TRACEALWAYS("   Listing unsorted points:\n");
		for(int i = 0; i < nPoints; i++)
		{
			const CVec2Dfp32& Point = pPoints2D[i];
			M_TRACEALWAYS("      Point %d:   %f %f\n", i, Point[0], Point[1]);
		}

		M_TRACEALWAYS("   Listing sorted list:\n");
		for(int i = 0; i < niPoints; i++)
		{
			int iPointIndex = piPoints2D[i];
			const CVec2Dfp32& Point = pPoints2D[iPointIndex];
			M_TRACEALWAYS("      List Index %d:   %d   -   %f %f\n", i, iPointIndex, Point[0], Point[1]);
		}
		#endif

		M_TRACEALWAYS("   Searching for duplicated indices\n");
		for(int i = 0; i < niPoints; i++)
		{
			int iPointIndex = piPoints2D[i];
			for(int j = 0; j < niPoints; j++)
			{
				if(j == i) continue;
				if(iPointIndex == piPoints2D[j])
					M_TRACEALWAYS("      Duplicate index found: %d from position %d found at position %d\n", iPointIndex, i, j);
			}
		}
	#endif

	// Create base
	//int nWorkTris = m_nBasePoints / 3;
	//for(int i = 0; i < m_nBasePoints; i += 3)
	//	lWorkTris.Add(CDelaunay2DTriangle(pPoints2D, i+1, i, i+2));
	m_lBaseTriangle.Duplicate(&lWorkTris);
	int nWorkTris = lWorkTris.Len();
	m_lBaseTriangle.SetLen(0);
//	int nWorkTris = 2;
//	lWorkTris.Add(CDelaunay2DTriangle(pPoints2D, 1, 0, 2));
//	lWorkTris.Add(CDelaunay2DTriangle(pPoints2D, 1, 2, 3));

	// Start adding points after base points
	TArray<uint32> liRemove;
	TArray<CDelaunay2DEdge> lEdges;

	for(int i = 0; i < nPoints; i++)
	{
		#ifdef GLASS_SYSTEM_TRACE
			M_TRACEALWAYS("   Updating Point %d\n", i );
		#endif

		const uint32& iPoint2D = piPoints2D[i];
		//if(iPoint2D < 4) continue;
		if(iPoint2D < m_nBasePoints) continue;

		liRemove.QuickSetLen(0);
		lEdges.QuickSetLen(0);
		
		const CVec2Dfp32& Point = pPoints2D[iPoint2D];

		CDelaunay2DTriangle* pWorkTris = lWorkTris.GetBasePtr();
		int iAffected = 0;
		for(int j = 0; j < nWorkTris; j++)
		{
			CDelaunay2DTriangle& WorkTri = pWorkTris[j];
			if(WorkTri.IsLeftOf(Point))
			{
				iAffected++;
				#ifdef GLASS_SYSTEM_TRACE
					M_TRACEALWAYS("      Left Of Triangle: %i\n", j);
				#endif

				//const CVec3Dint16& Index = WorkTri.m_iIndex;
				//if(Index[0] >= 0 && Index[1] >= 0 && Index[2] >= 0)
					m_lOutput.Add(WorkTri);

				liRemove.Add(j);
			}
			else if(WorkTri.CCEncompasses(Point))
			{
				iAffected++;
				#ifdef GLASS_SYSTEM_TRACE
					M_TRACEALWAYS("      Encompassed triangle: %i\n", j);
				#endif
				// Remove triangle if hot
				//if(WorkTri.CCEncompasses(Point))
				//{
					CVec3Dint16& Index = WorkTri.m_iIndex;
					HandleEdge(Index[0], Index[1], lEdges);
					HandleEdge(Index[1], Index[2], lEdges);
					HandleEdge(Index[2], Index[0], lEdges);
					liRemove.Add(j);
				//}
			}
		}

		#ifndef GLASS_SYSTEM_TRACE
			if(iAffected <= 0)
			{
				M_TRACEALWAYS("      Point %d - Index(%d):    Didn't have any affect upon any triangle!!\n", i, iPoint2D);
				iAffected = 0;
			}
		#endif

		// Remove triangles from back to front
		const int& RemoveLen = liRemove.Len();
		const uint32* piRemove = liRemove.GetBasePtr();
		for(int j = RemoveLen; j > 0; j--)
		{
			#ifdef GLASS_SYSTEM_TRACE
				M_TRACEALWAYS("      Removing Triangle: %d\n", piRemove[j-1]);
			#endif
			lWorkTris.Del(piRemove[(j-1)]);
		}

		// Add edges with current index to triangle list
		const int& EdgeLen = lEdges.Len();
		const CDelaunay2DEdge* pEdges = lEdges.GetBasePtr();
		for(int j = 0; j < EdgeLen; j++)
		{
			const CDelaunay2DEdge& Edge = pEdges[j];
			const CVec2Dint16& Index = Edge.m_iIndex;

			//if(Index[0] != iPoint2D && Index[1] != iPoint2D)
			#ifdef GLASS_SYSTEM_TRACE
				M_TRACEALWAYS("      Adding Triangle: %d %d %d\n", Index[0], Index[1], iPoint2D);
			#endif
			lWorkTris.Add(CDelaunay2DTriangle(pPoints2D, Index[0], Index[1], iPoint2D));
		}

		nWorkTris += EdgeLen - RemoveLen;
	}

	int TriLen = lWorkTris.Len();
	#ifdef GLASS_SYSTEM_TRACE
		M_TRACEALWAYS("   Triangled left: %d\n", TriLen);
	#endif
	const CDelaunay2DTriangle* pWorkTris = lWorkTris.GetBasePtr();
	for(int i = 0; i < TriLen; i++)
	{
		const CDelaunay2DTriangle& WorkTri = pWorkTris[i];
		m_lOutput.Add(WorkTri);
	}

	lWorkTris.SetLen(0);
	liRemove.SetLen(0);
	lEdges.SetLen(0);
}


void CXR_Delaunay2D::Triangulate(const uint32* piPoints2D, const int& _i0, const int& _i1, const int& _i2, const int& _i3)
{
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CGlassModelEntity
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CGlassModelEntity::CGlassEntity::PreShatter()
{
	const int& nShards = m_lShards.Len();
	const uint16& Res = m_Resolution;
	const CVec3Dfp32& GlassHor = CVec3Dfp32::GetMatrixRow(m_Pos, 0);
	const CVec3Dfp32& GlassVer = CVec3Dfp32::GetMatrixRow(m_Pos, 1);
	CGlassModelEntity::CGlassShard* pShards = m_lShards.GetBasePtr();

	// Pick user configurated seed from ogier
	int Seed = 1337;

	// Create delaunay object
	CXR_Delaunay2D Delaunay(Seed);

	// Add base triangles to delaunay triangulator
	for(int i = 0; i < nShards; i++)
		Delaunay.AddBaseTriangleRecalc(pShards[i].m_UV);

	// Generate n Resolution points and triangulate the surface
	Delaunay.GeneratePoints(Res);
	Delaunay.Triangulate();

	// Get all points in delaunay list and triangulated entities
	const CXR_Delaunay2D::CDelaunay2DTriangle* pTriangles = Delaunay.GetOutputBasePtr();
	const CVec2Dfp32* pPoints2D = Delaunay.GetPointsBasePtr();
	const int& nTriangles = Delaunay.GetNumTriangles();

	// Set number of shards to number of triangles in delaunay and get the base pointer
	m_lShards.SetLen(nTriangles);
	pShards = m_lShards.GetBasePtr();
	
	// Add all triangles to shards list
	for(int i = 0; i < nTriangles; i++)
	{
		CGlassModelEntity::CGlassShard& Shard = pShards[i];
		const CXR_Delaunay2D::CDelaunay2DTriangle& Triangle = pTriangles[i];
		const CVec3Dint16& piIndex = Triangle.m_iIndex;
		const CVec2Dfp32& Point0 = pPoints2D[piIndex[0]];
		const CVec2Dfp32& Point1 = pPoints2D[piIndex[1]];
		const CVec2Dfp32& Point2 = pPoints2D[piIndex[2]];

		// Calculate shard from 2d triangle
		Shard.m_Position = (GlassHor * Point0[0]) + (GlassVer * Point0[1]);
		
		Shard.m_UV[0] = CVec2Dfp32((Point0[0] * 0.5f) + 0.5f, ((Point0[1] * 0.5f) - 0.5f) * -1.0f);
		Shard.m_UV[1] = CVec2Dfp32((Point1[0] * 0.5f) + 0.5f, ((Point1[1] * 0.5f) - 0.5f) * -1.0f);
		Shard.m_UV[2] = CVec2Dfp32((Point2[0] * 0.5f) + 0.5f, ((Point2[1] * 0.5f) - 0.5f) * -1.0f);
	}
}


void CGlassModelEntity::CGlassEntity::ShatterPoint(const CVec3Dfp32& _CenterPos, const CVec3Dfp32& _Force)
{
	const CVec3Dfp32& GlassHor = CVec3Dfp32::GetMatrixRow(m_Pos, 0);
	const CVec3Dfp32& GlassVer = CVec3Dfp32::GetMatrixRow(m_Pos, 1);
	const CVec3Dfp32& GlassPos = CVec3Dfp32::GetMatrixRow(m_Pos, 3);

	//const uint8& ResWidth = m_Resolution[0];
	//const uint8& ResHeight = m_Resolution[1];
	//const uint16& ResDim = ResWidth*ResHeight;
//	const uint16& Res = m_Resolution;
	
	CVec3Dfp32 NGlassHor = GlassHor;
	CVec3Dfp32 NGlassVer = GlassVer;

	NGlassHor.Normalize();
	NGlassVer.Normalize();

	const CVec3Dfp32 LocalPosition = _CenterPos - GlassPos;
	
	// FIXME: User configurated from Ogier!!!
	int Seed = (int)(_CenterPos.Length() * 1337);//MRTC_RAND();

	CXR_Delaunay2D Delaunay(Seed);

	TArray<CGlassModelEntity::CGlassShard> lSave;

	// Add base triangles to handle point insertion on
	CGlassModelEntity::CGlassShard* pBaseShards = m_lShards.GetBasePtr();
	const int& nBaseShards = m_lShards.Len();
	for(int i = 0; i < nBaseShards; i++)
	{
		const CVec3Dfp32& Vel = pBaseShards[i].m_Velocity;
		
		// Is this triangle disturbed?
		if(Vel[0] == 0.0f && Vel[1] == 0.0f && Vel[2] == 0.0f)
			Delaunay.AddBaseTriangleRecalc(pBaseShards[i].m_UV);
		else
			lSave.Add(pBaseShards[i]);
	}
	
	const fp32 WidthPlane = (LocalPosition * NGlassHor) / GlassHor.Length();
	const fp32 HeightPlane = (LocalPosition * NGlassVer) / GlassVer.Length();
//	const fp32 WidthBias = (WidthPlane * 0.5f) + 0.5f;
//	const fp32 HeightBias = (HeightPlane * 0.5f) + 0.5f;
	
	// Now when we have all the points we want, we run them through our delaunay triangulator
	if(!Delaunay.GeneratePoints(CVec2Dfp32(WidthPlane,HeightPlane), 12, 0.20f))
		return;

	//Delaunay.GeneratePoints(1);
	Delaunay.Triangulate();

	// Get generated points and triangulated data
	CXR_Delaunay2D::CDelaunay2DTriangle* pTriangles = Delaunay.GetOutputBasePtr();
	CVec2Dfp32* pPoints2D = Delaunay.GetPointsBasePtr();
	int nTriangles = Delaunay.GetNumTriangles();

	m_lShards.SetLen(0);
	lSave.Duplicate(&m_lShards);
	m_lShards.SetGrow(nTriangles);

	// Since all this data is generated in 2d all we need to do is actually orientate it according to the glass dimension
    // The triangles might be refered to as a shard, so we just add the shards
	//m_lShards.SetLen(nTriangles);

	#ifdef GLASS_SYSTEM_TRACE
		M_TRACEALWAYS( "Listing Triangulated shards insertion:\n");
	#endif

	for(int i = 0; i < nTriangles; i++)
		m_lShards.Insert(lSave.Len(), CGlassModelEntity::CGlassShard());

	CGlassModelEntity::CGlassShard* pShards = m_lShards.GetBasePtr();
	for(int i = lSave.Len(); i < nTriangles+lSave.Len(); i++)
	{
		CGlassModelEntity::CGlassShard& Shard = pShards[i];
		CXR_Delaunay2D::CDelaunay2DTriangle& Triangle = pTriangles[i-lSave.Len()];
		CVec3Dint16& piIndex = Triangle.m_iIndex;
		CVec2Dfp32& Point0 = pPoints2D[piIndex[0]];
		CVec2Dfp32& Point1 = pPoints2D[piIndex[1]];
		CVec2Dfp32& Point2 = pPoints2D[piIndex[2]];

		#ifdef GLASS_SYSTEM_TRACE
			M_TRACEALWAYS( "   Triangle %i:   %d %d %d   -   %f %f - %f %f - %f %f\n", i-lSave.Len(), piIndex[0], piIndex[1], piIndex[2],
				Point0[0], Point0[1], Point1[0], Point1[1], Point2[0], Point2[1]);

			// ERROR CHECKING!
			if(piIndex[0] == piIndex[1])
				M_TRACEALWAYS( "      Error: Index0 == Index1\n");
			else if(piIndex[0] == piIndex[2])
				M_TRACEALWAYS( "      Error: Index0 == Index2\n");
			else if(piIndex[1] == piIndex[2])
				M_TRACEALWAYS( "      Error: Index1 == Index2\n");

			if(Point0[0] == Point1[0] && Point0[1] == Point1[1])
				M_TRACEALWAYS( "      Error: Point0 == Point1 !!!\n");
			else if(Point0[0] == Point2[0] && Point0[1] == Point2[1])
				M_TRACEALWAYS( "      Error: Point0 == Point2 !!!\n");
			else if(Point2[0] == Point1[0] && Point2[1] == Point1[1])
				M_TRACEALWAYS( "      Error: Point2 == Point1 !!!\n");
			// ERROR CHECKING!
		#endif

//		if(piIndex[0] < 0 || piIndex[1] < 0 || piIndex[2] < 0)
//			int ijk = 0;
//		if(piIndex[0] > ResDim+3 || piIndex[0] > ResDim+3 || piIndex[2] > ResDim+3)
//			int ijk = 0;

		// Calculate shard from 2d data
		Shard.m_Position = (GlassHor * Point0[0]) + (GlassVer * Point0[1]);
//		Shard.m_2DVert[0] = Point0 - Point1;
//		Shard.m_2DVert[1] = Point0 - Point2;
//		Shard.m_Vertices[0] = (GlassHor * Point0[0]) + (GlassVer * Point0[1]);
//		Shard.m_Vertices[1] = (GlassHor * Point1[0]) + (GlassVer * Point1[1]);
//		Shard.m_Vertices[2] = (GlassHor * Point2[0]) + (GlassVer * Point2[1]);
		Shard.m_UV[0] = CVec2Dfp32((Point0[0] * 0.5f) + 0.5f, ((Point0[1] * 0.5f) - 0.5f) * -1.0f);
		Shard.m_UV[1] = CVec2Dfp32((Point1[0] * 0.5f) + 0.5f, ((Point1[1] * 0.5f) - 0.5f) * -1.0f);
		Shard.m_UV[2] = CVec2Dfp32((Point2[0] * 0.5f) + 0.5f, ((Point2[1] * 0.5f) - 0.5f) * -1.0f);

//		if((Point0[0] >= WidthPlane-0.15f && Point0[0] <= WidthPlane+0.15f && Point0[1] >= HeightPlane-0.15f && Point0[1] <= HeightPlane+0.15f) &&
//		   (Point1[0] >= WidthPlane-0.15f && Point1[0] <= WidthPlane+0.15f && Point1[1] >= HeightPlane-0.15f && Point1[1] <= HeightPlane+0.15f) &&
//		   (Point2[0] >= WidthPlane-0.15f && Point2[0] <= WidthPlane+0.15f && Point2[1] >= HeightPlane-0.15f && Point2[1] <= HeightPlane+0.15f))
		if((Point0[0] >= WidthPlane-0.20f && Point0[0] <= WidthPlane+0.20f && Point0[1] >= HeightPlane-0.20f && Point0[1] <= HeightPlane+0.20f) &&
		   (Point1[0] >= WidthPlane-0.20f && Point1[0] <= WidthPlane+0.20f && Point1[1] >= HeightPlane-0.20f && Point1[1] <= HeightPlane+0.20f) &&
		   (Point2[0] >= WidthPlane-0.20f && Point2[0] <= WidthPlane+0.20f && Point2[1] >= HeightPlane-0.20f && Point2[1] <= HeightPlane+0.20f))
		{
			//Shard.m_Velocity = CVec3Dfp32(_Force[0] * (MFloat_GetRand(Seed) * 16.0f), _Force[1] * (MFloat_GetRand(Seed) * 16.0f), _Force[2]);
			Shard.m_Velocity = CVec3Dfp32((MFloat_GetRand(Seed) - 0.5f) * 16.0, (MFloat_GetRand(Seed+1) - 0.5f) * 16.0f, MFloat_GetRand(Seed+2) * 8.0f);
			Shard.m_Velocity = Shard.m_Velocity * _Force;
			
			Shard.m_Velocity = CVec3Dfp32((_Force[0] * ((MFloat_GetRand(Seed+0)) * 8.0f)),
										 (_Force[1] * ((MFloat_GetRand(Seed+1)) * 8.0f)),
										 (_Force[2] * (MFloat_GetRand(Seed+2) * 8.0f)));
			
			Shard.m_Rotations = CVec3Dfp32(MFloat_GetRand(Seed+0), MFloat_GetRand(Seed+1), MFloat_GetRand(Seed+2));
		}

		// Set velocity
//		Shard.m_Velocity = CVec3Dfp32((MFloat_GetRand(Seed) - 0.5f) * 16.0, (MFloat_GetRand(Seed+1) - 0.5f) * 16.0f, MFloat_GetRand(Seed+2) * 8.0f);
		Seed += 3;
	}
}


void CGlassModelEntity::CGlassEntity::SplitConnection(const CGlassShard* _pShards, const int& _Shard)
{
//	const CGlassModelEntity::CGlassShard& Shard = _pShards[_Shard];
//	const CShardConn& Connection = Shard.m_Connection;
//	const int& Up = Connection[SHARD_CONN_U];
//	const int& Down = Connection[SHARD_CONN_D];
//	const int& Left = Connection[SHARD_CONN_L];
//	const int& Right = Connection[SHARD_CONN_R];

//	if(Up > SHARD_BASE)
//	{
//		_pShards[Up].m_Connection[SHARD_CONN_D] = SHARD_HOLE;
//	}
}

void CGlassModelEntity::CGlassEntity::CollapseConnection(const int& _Shard)
{
//	CGlassModelEntity::CGlassShard* pShards = m_lShards.GetBasePtr();
//	const uint16& nShards = m_lShards.Len();
	
//	CGlassModelEntity::CGlassShard& Shard = pShards[_Shard];
	
	// Start by searching downwards
	//if(SearchConnection(_Shard, SHARD_CONN_D, pBase))
	//{
		// if true, this shard follows all the way down to the base frame
	//}
//	bool bConnection = true;
//	if(Shard.m_Connection[SHARD_CONN_D] == SHARD_BASE)
//		bConnection = true;
//	else
}

void CGlassModelEntity::CGlassEntity::SearchConnection(const int& _Shard, const int &_Direction, CGlassShard* pBase)
{
//	CGlassModelEntity::CGlassShard& Shard = pBase[_Shard];
	//if(Shard.m_Connection[_Direction] == SHARD_BASE)
}


CGlassModelEntity::CGlassModelEntity(const int& _Len)
{
	m_lGlasses.SetLen(_Len);
}


void CGlassModelEntity::Create(class CXR_Model* _pModel, const CXR_ModelInstanceContext* _pContext)
{
}


void CGlassModelEntity::OnRefresh(const CXR_ModelInstanceContext* _pContext, int _GameTick, const CMat4Dfp32* _pMat, int _nMat, int _Flags)
{
}


bool CGlassModelEntity::NeedRefresh(class CXR_Model* _pModel, const CXR_ModelInstanceContext* _pContext)
{
	return false;
}


TPtr<CXR_ModelInstance> CGlassModelEntity::Duplicate() const
{
	CXR_ModelInstance* pReturnNew = MNew1(CGlassModelEntity,0);
	CGlassModelEntity* pCastedNew = safe_cast<CGlassModelEntity>(pReturnNew);
	
	// Duplicate data
    //*pNew = *this;
	m_lGlasses.Duplicate(&pCastedNew->m_lGlasses);

	return TPtr<CXR_ModelInstance>(pReturnNew);
}


void CGlassModelEntity::operator = (const CXR_ModelInstance& _Instance)
{
	const CGlassModelEntity& From = *safe_cast<const CGlassModelEntity>(&_Instance);
	*this = From;
}


void CGlassModelEntity::operator = (const CGlassModelEntity& _Instance)
{
	// Reference data
}

void CGlassModelEntity::AllocGlassObj(const int& _Len)
{
	
	m_lGlasses.SetLen(_Len);
}


//void CGlassModelEntity::CreateGlassObj(const int& _iObj, const int& _nV, const CVec3Dfp32* _pV, const uint16* _pI)
void CGlassModelEntity::CreateGlassObj(const int& _iObj, const TThinArray<CVec3Dfp32>& _lV, const int& _SurfaceID, const int& _BrokenSurfaceID, const CMat4Dfp32& _WMat, const uint16& _Resolution, const fp32& _Durability)
{
	CGlassEntity& GlassObj = m_lGlasses[_iObj];
	
    //const int Res = 32;	// Vertex resolution
	GlassObj.m_Resolution = _Resolution;
	GlassObj.m_Pos = _WMat;
	GlassObj.m_lShards.SetLen(2);
	GlassObj.m_Durability = (int8)_Durability;
//	GlassObj.m_liShardsMapping.SetLen(_Resolution[0]*_Resolution[1]);
	//GlassObj.m_lShards.SetLen(Res*Res);
	CGlassModelEntity::CGlassShard* pShards = GlassObj.m_lShards.GetBasePtr();
//	uint16* pShardsMapping = GlassObj.m_liShardsMapping.GetBasePtr();
//	uint16 MappingLen = GlassObj.m_liShardsMapping.Len();
	const CVec3Dfp32* pV = _lV.GetBasePtr();

	// Calculate glass facing, which includes the dimension of the glass (rectangular)
	CVec3Dfp32::GetMatrixRow(GlassObj.m_Pos, 0) = ((pV[2] - pV[1]) / 2);
	CVec3Dfp32::GetMatrixRow(GlassObj.m_Pos, 1) = ((pV[1] - pV[0]) / 2);
	
	// This actually defines the thickness of the glass!
	CVec3Dfp32& Dir = CVec3Dfp32::GetMatrixRow(GlassObj.m_Pos, 2);
	Dir = CVec3Dfp32::GetMatrixRow(GlassObj.m_Pos, 0) / CVec3Dfp32::GetMatrixRow(GlassObj.m_Pos, 1);
	Dir.Normalize();
	Dir *= 4.0f;

	//CVec3Dfp32 Dim = _lV[2] - _lV[0];
	//Dim[0] = M_Fabs(Dim[0]);
	//Dim[1] = M_Fabs(Dim[1]);
	//Dim[2] = M_Fabs(Dim[2]);

	//CVec3Dfp32 HalfDim = Dim / 2;
	const CVec3Dfp32& GlassHor = CVec3Dfp32::GetMatrixRow(GlassObj.m_Pos, 0);
	const CVec3Dfp32& GlassVer = CVec3Dfp32::GetMatrixRow(GlassObj.m_Pos, 1);
//	const CVec3Dfp32& GlassPos = CVec3Dfp32::GetMatrixRow(GlassObj.m_Pos, 3);

	pShards[0].m_Position = GlassHor + GlassVer;//CVec3Dfp32(0,0,0);
	//Shard.m_Position = (GlassHor * Point0[0]) + (GlassVer * Point0[1]);
	pShards[0].m_Position = GlassVer - GlassHor;//CVec3Dfp32(0);//GlassHor * 
	pShards[0].m_UV[0] = CVec2Dfp32(0,0);
	pShards[0].m_UV[1] = CVec2Dfp32(0,1);
	pShards[0].m_UV[2] = CVec2Dfp32(1,1);
//	pShards[0].m_2DVert[0] = CVec2Dfp32(0,-1);
//	pShards[0].m_2DVert[1] = CVec2Dfp32(1,-1);
//	pShards[0].m_Vertices[0] = -GlassHor + GlassVer;
//	pShards[0].m_Vertices[1] = -GlassHor - GlassVer;
//	pShards[0].m_Vertices[2] =  GlassHor - GlassVer;

	pShards[1].m_Position = GlassVer - GlassHor;//CVec3Dfp32(0);//GlassHor + GlassVer;//CVec3Dfp32(0,0,0);
	pShards[1].m_UV[0] = CVec2Dfp32(0,0);
	pShards[1].m_UV[1] = CVec2Dfp32(1,1);
	pShards[1].m_UV[2] = CVec2Dfp32(1,0);
//	pShards[0].m_2DVert[0] = CVec2Dfp32(1,-1);
//	pShards[0].m_2DVert[1] = CVec2Dfp32(1, 0);
//	pShards[1].m_Vertices[0] = -GlassHor + GlassVer;
//	pShards[1].m_Vertices[1] =  GlassHor - GlassVer;
//	pShards[1].m_Vertices[2] =  GlassHor + GlassVer;

//	pShards[0].m_Position = CVec3Dfp32(0,0,0);//CVec3Dfp32::GetMatrixRow(GlassObj.m_Pos, 3);//_lV[0] + CVec3Dfp32((Dim[0]) - HalfDim[0], Dim[1], (-Dim[2] + HalfDim[2]));
//	pShards[0].m_UV = CVec4Dfp32(0,0,1,1);
//	pShards[0].m_Dimension = CVec2Dfp32(1,1);//CVec2Dfp32(HalfDim[0], HalfDim[2]);

	//Dim = Dim / static_cast<fp32>(Res);
	//CVec3Dfp32 Dim2 = Dim / 2;

	//GlassObj.m_HalfWidth = Dim2[0];
	//GlassObj.m_HalfHeight = Dim2[2];

	// Reset whole mapping to use the same shard
//	memset(pShardsMapping, 0x0, sizeof(uint16)*MappingLen);

	GlassObj.m_SurfaceID = _SurfaceID;
	GlassObj.m_BrokenSurfaceID = _BrokenSurfaceID;
}


void CGlassModelEntity::BreakGlass(const int& _iGlassObj)
{

}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_GlassSystem
|__________________________________________________________________________________________________
\*************************************************************************************************/
//void CXR_Model_GlassSystem::Create(const char* _pParam)
//{
//	CXR_Model_Custom::Create(_pParam);
//}


void CXR_Model_GlassSystem::OnCreate(const char* _pParam)
{
	CXR_Model_Custom::OnCreate(_pParam);
}


TPtr<CXR_ModelInstance> CXR_Model_GlassSystem::CreateModelInstance()
{
	return MNew1(CGlassModelEntity,0);
}

fp32 CXR_Model_GlassSystem::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	fp32 Radius = 0.0f;
	if(_pAnimState && _pAnimState->m_pModelInstance)
	{
		const CGlassModelEntity* pData = safe_cast<const CGlassModelEntity>(_pAnimState->m_pModelInstance);
		const CGlassModelEntity::CGlassEntity* pGlasses = pData->m_lGlasses.GetBasePtr();
		const int Len = pData->m_lGlasses.Len();

		for(int i = 0; i < Len; i++)
		{
			const CGlassModelEntity::CGlassEntity& Glass = pGlasses[i];
			const CMat4Dfp32& GlassMat = Glass.m_Pos;
			const CVec3Dfp32& GlassHor = CVec3Dfp32::GetMatrixRow(GlassMat, 0);
			const CVec3Dfp32& GlassVer = CVec3Dfp32::GetMatrixRow(GlassMat, 1);

			Radius += (GlassHor + GlassVer).Length();
		}

		Radius *= 0.5f;
	}
	else
		Radius = 16.0f;

	return Radius;
}

void CXR_Model_GlassSystem::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	if(_pAnimState && _pAnimState->m_pModelInstance)
	{
		const CGlassModelEntity* pData = safe_cast<const CGlassModelEntity>(_pAnimState->m_pModelInstance);
		const CGlassModelEntity::CGlassEntity* pGlasses = pData->m_lGlasses.GetBasePtr();
		const int Len = pData->m_lGlasses.Len();
		
		for(int i = 0; i < Len; i++)
		{
			const CGlassModelEntity::CGlassEntity& Glass = pGlasses[i];
			const CMat4Dfp32& GlassMat = Glass.m_Pos;
            const CVec3Dfp32& GlassHor = CVec3Dfp32::GetMatrixRow(GlassMat, 0);
			const CVec3Dfp32& GlassVer = CVec3Dfp32::GetMatrixRow(GlassMat, 1);
			const CVec3Dfp32& GlassPos = CVec3Dfp32::GetMatrixRow(GlassMat, 3);
			
			_Box.Expand((GlassPos + GlassVer + GlassHor));
			_Box.Expand((GlassPos - GlassVer - GlassHor));
		}

		// Grow bounding box a little extra
		_Box.Grow(32);
	}
	else
		_Box = CBox3Dfp32(-16,16);
}

void CXR_Model_GlassSystem::Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
{
	if(_pAnimState && _pAnimState->m_pModelInstance)
	{
		const CGlassModelEntity* pData = safe_cast<const CGlassModelEntity>(_pAnimState->m_pModelInstance);
		
		const TArray<CGlassModelEntity::CGlassEntity>& lGlasses = pData->m_lGlasses;
		const int Len = lGlasses.Len();
		const fp32& IPTime = _pAnimState->m_AnimAttr0;
        
		for(int i = 0; i < Len; i++)
		{
			const CGlassModelEntity::CGlassEntity& Glass = lGlasses[i];
			const int& nShards = Glass.m_lShards.Len();
			const CGlassModelEntity::CGlassShard* pShards = Glass.m_lShards.GetBasePtr();

			CMat4Dfp32 L2V;
			const CMat4Dfp32& GlassMat = Glass.m_Pos;
			CMat4Dfp32 GlassPos;
			CMat4Dfp32 ShardMat;
			GlassPos.Unit();
			CVec3Dfp32::GetMatrixRow(GlassPos, 3) = CVec3Dfp32::GetMatrixRow(GlassMat, 3);

//			const CVec3Dfp32& GlassMatHor = CVec3Dfp32::GetMatrixRow(GlassMat, 0);
//			const CVec3Dfp32& GlassMatVer = CVec3Dfp32::GetMatrixRow(GlassMat, 1);
			const CVec3Dfp32 GlassThick = CVec3Dfp32::GetMatrixRow(GlassMat, 2) * 0.5f;

			CMat4Dfp32 GlassShardMat;
			//_WMat.Multiply(_VMat, L2V);
						
			//const CVec3Dfp32& WLeft = CVec3Dfp32::GetMatrixRow(_WMat,0);
			//const CVec3Dfp32& WUp = CVec3Dfp32::GetMatrixRow(_WMat,1);
			//const CVec3Dfp32& WDir = CVec3Dfp32::GetMatrixRow(_WMat,2);
			
			//GlassMat.Multiply(_VMat, L2V);
			GlassPos.Multiply(_VMat, L2V);
//			const CVec3Dfp32& WLeft = CVec3Dfp32::GetMatrixRow(GlassMat,0);
//			const CVec3Dfp32& WUp = CVec3Dfp32::GetMatrixRow(GlassMat,1);
//			const CVec3Dfp32& WDir = CVec3Dfp32::GetMatrixRow(GlassMat,2);

			CXR_VertexBuffer* pVB = AllocVB(_pRenderParams);
			CVec3Dfp32* pV = _pRenderParams->m_pVBM->Alloc_V3((nShards * 3)*2);
			CVec2Dfp32* pTV = _pRenderParams->m_pVBM->Alloc_V2((nShards * 3)*2);
			uint16* pI = _pRenderParams->m_pVBM->Alloc_Int16(nShards * 24);
			int SurfaceID = 0;

//			const CVec3Dfp32 Left = (WLeft * Glass.m_HalfWidth);
//			const CVec3Dfp32 Up = (WDir * Glass.m_HalfHeight);
			
			// Go through all shards
			int iVertex = 0;
			int iIndex = 0;
			for(int j = 0; j < nShards; j++)
			{
//				const int Row = j/32;
//				const int Col = j - (Row*32);
				const CGlassModelEntity::CGlassShard& Shard = pShards[j];
//				const CVec3Dfp32& Position = Shard.m_Position;

				//ShardMat.Unit();
				//ShardMat.CreateFrom(GlassMat);
				ShardMat.Unit();
				//CVec3Dfp32::GetMatrixRow(GlassMat, 3).SetMatrixRow(ShardMat, 3);
				
				//GlassShardMat.Multiply(_VMat, L2V);
				//ShardMat.Multiply(_VMat, L2V);

//				const CVec3Dfp32 SLeft = CVec3Dfp32::GetMatrixRow(ShardMat, 0) * Shard.m_Dimension[0];//Glass.m_HalfWidth;
//				const CVec3Dfp32 SUp = CVec3Dfp32::GetMatrixRow(ShardMat, 2) * Shard.m_Dimension[1];//Glass.m_HalfHeight;
				
//				const CVec3Dfp32 SLeft = CVec3Dfp32::GetMatrixRow(GlassMat, 0) * Shard.m_Dimension[0];
//				const CVec3Dfp32 SUp = CVec3Dfp32::GetMatrixRow(GlassMat, 1) * Shard.m_Dimension[1];

				//pV[iVertex]   = (Position - Left + Up) * L2V;
				//pV[iVertex+1] = (Position - Left - Up) * L2V;
				//pV[iVertex+2] = (Position + Left - Up) * L2V;
				//pV[iVertex+3] = (Position + Left + Up) * L2V;
				
				const CVec2Dfp32 UV01 = (Shard.m_UV[0] - Shard.m_UV[1]) * 2.0f;
				const CVec2Dfp32 UV02 = (Shard.m_UV[0] - Shard.m_UV[2]) * 2.0f;

				const CVec3Dfp32 IPVel = (Shard.m_Velocity * IPTime);

				#ifdef GLASS_SYSTEM_FLAT
				pV[iVertex]		= (Shard.m_Position + IPVel) * L2V;//Shard.m_Vertices[0] * L2V;
				pV[iVertex+1]	= (Shard.m_Position + IPVel - (GlassMatHor * UV01[0]) + (GlassMatVer * UV01[1])) * L2V;//Shard.m_Vertices[1] * L2V;
				pV[iVertex+2]	= (Shard.m_Position + IPVel - (GlassMatHor * UV02[0]) + (GlassMatVer * UV02[1])) * L2V;//Shard.m_Vertices[2] * L2V;

				// ORIG
//				pV[iVertex]   = (Position - SLeft + SUp) * L2V;
//				pV[iVertex+1] = (Position - SLeft - SUp) * L2V;
//				pV[iVertex+2] = (Position + SLeft - SUp) * L2V;
//				pV[iVertex+3] = (Position + SLeft + SUp) * L2V;

				//pTV[iVertex]   = CVec2Dfp32(0.0f,0.0f);
				//pTV[iVertex+1] = CVec2Dfp32(0.0f,1.0f);
				//pTV[iVertex+2] = CVec2Dfp32(1.0f,1.0f);
				//pTV[iVertex+3] = CVec2Dfp32(1.0f,0.0f);
				//pTV[iVertex]   = CVec2Dfp32(Col*UOffset, Row*VOffset);
				//pTV[iVertex+1] = CVec2Dfp32(Col*UOffset, (Row+1)*VOffset);
				//pTV[iVertex+2] = CVec2Dfp32((Col+1)*UOffset, (Row+1)*VOffset);
				//pTV[iVertex+3] = CVec2Dfp32((Col+1)*UOffset, Row*VOffset);
				pTV[iVertex]   = Shard.m_UV[0]; //CVec2Dfp32(Shard.m_UV[0], Shard.m_UV[1]);
				pTV[iVertex+1] = Shard.m_UV[1]; //CVec2Dfp32(Shard.m_UV[0], Shard.m_UV[3]);
				pTV[iVertex+2] = Shard.m_UV[2]; //CVec2Dfp32(Shard.m_UV[2], Shard.m_UV[3]);
				//pTV[iVertex+3] = CVec2Dfp32(Shard.m_UV[2], Shard.m_UV[1]);

				pI[iIndex]   = iVertex;
				pI[iIndex+1] = iVertex+1;
				pI[iIndex+2] = iVertex+2;
				#endif
				//pI[iIndex+3] = iVertex;
				//pI[iIndex+4] = iVertex+2;
				//pI[iIndex+5] = iVertex+3;

				#ifdef GLASS_SYSTEM_VOLUME
				
				// Rotations
				const fp32 Time = _pAnimState->m_AnimTime0.GetTime();
				if(Shard.m_Rotations[0] != 0.0f) ShardMat.M_x_RotX(Shard.m_Rotations[0] * Time);
				if(Shard.m_Rotations[1] != 0.0f) ShardMat.M_x_RotY(Shard.m_Rotations[1] * Time);
				if(Shard.m_Rotations[2] != 0.0f) ShardMat.M_x_RotZ(Shard.m_Rotations[2] * Time);
				GlassMat.Multiply(ShardMat, GlassShardMat);
				const CVec3Dfp32& GlassMatHorR = CVec3Dfp32::GetMatrixRow(GlassShardMat, 0);
				const CVec3Dfp32& GlassMatVerR = CVec3Dfp32::GetMatrixRow(GlassShardMat, 1);
				const CVec3Dfp32& GlassThickR = CVec3Dfp32::GetMatrixRow(GlassShardMat, 2);

				// EXTENDED (VOLUME SHARDS)
	                // Front back plane
					pV[iVertex]		= (Shard.m_Position + GlassThickR + IPVel) * L2V;//Shard.m_Vertices[0] * L2V;
					pV[iVertex+1]	= (Shard.m_Position + GlassThickR + IPVel - (GlassMatHorR * UV01[0]) + (GlassMatVerR * UV01[1])) * L2V;//Shard.m_Vertices[1] * L2V;
					pV[iVertex+2]	= (Shard.m_Position + GlassThickR + IPVel - (GlassMatHorR * UV02[0]) + (GlassMatVerR * UV02[1])) * L2V;//Shard.m_Vertices[2] * L2V;
					pV[iVertex+3]	= (Shard.m_Position - GlassThickR + IPVel) * L2V;//Shard.m_Vertices[0] * L2V;
					pV[iVertex+4]	= (Shard.m_Position - GlassThickR + IPVel - (GlassMatHorR * UV01[0]) + (GlassMatVerR * UV01[1])) * L2V;//Shard.m_Vertices[1] * L2V;
					pV[iVertex+5]	= (Shard.m_Position - GlassThickR + IPVel - (GlassMatHorR * UV02[0]) + (GlassMatVerR * UV02[1])) * L2V;//Shard.m_Vertices[2] * L2V;
					
					pTV[iVertex]   = Shard.m_UV[0];//CVec2Dfp32(Shard.m_UV[0], Shard.m_UV[1]);
					pTV[iVertex+1] = Shard.m_UV[1];//CVec2Dfp32(Shard.m_UV[0], Shard.m_UV[3]);
					pTV[iVertex+2] = Shard.m_UV[2];//CVec2Dfp32(Shard.m_UV[2], Shard.m_UV[3]);
					pTV[iVertex+3] = Shard.m_UV[0];//CVec2Dfp32(Shard.m_UV[0], Shard.m_UV[1]);
					pTV[iVertex+4] = Shard.m_UV[1];//CVec2Dfp32(Shard.m_UV[0], Shard.m_UV[3]);
					pTV[iVertex+5] = Shard.m_UV[2];//CVec2Dfp32(Shard.m_UV[2], Shard.m_UV[3]);

					pI[iIndex]   = iVertex;
					pI[iIndex+1] = iVertex+1;
					pI[iIndex+2] = iVertex+2;
					pI[iIndex+3] = iVertex+3;
					pI[iIndex+4] = iVertex+4;
					pI[iIndex+5] = iVertex+5;

					pI[iIndex+6]  = iVertex;
					pI[iIndex+7]  = iVertex+1;
					pI[iIndex+8]  = iVertex+3;
					pI[iIndex+9]  = iVertex+1;
					pI[iIndex+10] = iVertex+3;
					pI[iIndex+11] = iVertex+4;

					pI[iIndex+12] = iVertex;
					pI[iIndex+13] = iVertex+2;
					pI[iIndex+14] = iVertex+5;
					pI[iIndex+15] = iVertex+0;
					pI[iIndex+16] = iVertex+3;
					pI[iIndex+17] = iVertex+5;

					pI[iIndex+18] = iVertex+1;
					pI[iIndex+19] = iVertex+2;
					pI[iIndex+20] = iVertex+4;
					pI[iIndex+21] = iVertex+2;
					pI[iIndex+22] = iVertex+4;
					pI[iIndex+23] = iVertex+5;

					iVertex += 3;
					iIndex += 21;
				// EXTENDED
				#endif

				iVertex += 3;
				iIndex += 3;
			}

			if(Glass.m_Durability != 0)
				SurfaceID = Glass.m_SurfaceID;//GetSurfaceID("glass_test");
			else
				SurfaceID = Glass.m_BrokenSurfaceID;//GetSurfaceID("glass_test_broken");

			// Debug surface
			//SurfaceID = GetSurfaceID("glass_debug_surface");//0;
			//SurfaceID = 0;

			pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL);
			pVB->Geometry_Color(CPixel32(255,255,255,255));
			if(!pVB->AllocVBChain(_pRenderParams->m_pVBM,false))
				return;
			pVB->Geometry_VertexArray(pV, iVertex, true);
			pVB->Geometry_TVertexArray(pTV, 0);

			pVB->Render_IndexedTriangles(pI, iIndex/3);
			Render_Surface(_pRenderParams, SurfaceID, pVB, _pAnimState->m_AnimTime0);

			/*
			const CGlassModelEntity::CGlassEntity& Glass = lGlasses[i];
			const uint16* pIndices = Glass.m_lIndices.GetBasePtr();
			const CVec3Dfp32* pVertices = Glass.m_lVertices.GetBasePtr();
			const int IndicesLen = Glass.m_lIndices.Len();
			const int VerticesLen = Glass.m_lVertices.Len();

			const int Res = VerticesLen / 32;
			const fp32 UAdd = 1.0f / Res;
			const fp32 VAdd = 1.0f / Res;
			for(int j = 0; j < VerticesLen; j++)
			{
				pV[j] = pVertices[j] * L2V;

				const int jVRes = ((j/Res));
				const int jURes = j - (jVRes * 32);
//				const int jResRes = jRes % Res;
				
				pTV[j] = CVec2Dfp32(jURes*UAdd, jVRes*VAdd);
//				if(jResRes == 0) pTV[j] = CVec2Dfp32(0+(jRes*UAdd),0+(jRes*VAdd));
//				else if(jResRes == 1) pTV[j] = CVec2Dfp32(0+(jRes*UAdd),0+((jRes+1)*VAdd));
//				else if(jResRes == 2) pTV[j] = CVec2Dfp32(0+((jRes+1)*UAdd),0+((jRes+1)*VAdd));
//				else if(jResRes == 3) pTV[j] = CVec2Dfp32(0+((jRes+1)*UAdd),0+(jRes*VAdd));
			}
//			pV[0] = pVertices[0] * L2V;
//			pV[1] = pVertices[1] * L2V;
//			pV[2] = pVertices[2] * L2V;
//			pV[3] = pVertices[3] * L2V;

//			pTV[0] = CVec2Dfp32(0,0);
//			pTV[1] = CVec2Dfp32(0,1);
//			pTV[2] = CVec2Dfp32(1,1);
//			pTV[3] = CVec2Dfp32(1,0);
			for(int j = 0; j < IndicesLen; j++)
				pI[j] = pIndices[j];
//			memcpy(pI, pIndices, sizeof(uint16)*(5766));
//			for(int j = 0; j < IndicesLen; j++)
//				pI[j] = pIndices[j];

//			pI[0] = 0;
//			pI[1] = 1;
//			pI[2] = 2;
//			pI[3] = 0;
//			pI[4] = 2;
//			pI[5] = 3;

			SurfaceID = GetSurfaceID("glass_test");
			*/
		}

		/*
		pVB->Geometry_Color(CPixel32(255,255,255,255));
		if(!pVB->SetVBChain(m_pVBM,false))
			return;
		pVB->Geometry_VertexArray(pV, 1024, true);
		pVB->Geometry_TVertexArray(pTV, 0);

		pVB->Render_IndexedTriangles(pI, 1922);
		Render_Surface(SurfaceID, pVB, _pAnimState->m_AnimTime0);
		*/
	}
}

void CXR_Model_GlassSystem::LineIntersectGlass(CPClu_PTCollision& _PTCollision, CGlassModelEntity* _pGlassModelEntity)
{
	TArray<CGlassModelEntity::CGlassEntity>& lGlasses = _pGlassModelEntity->m_lGlasses;
	const int nGlasses = lGlasses.Len();

	for(int i = 0; i < nGlasses; i++)
	{
		CGlassModelEntity::CGlassEntity& Glass = lGlasses[i];

		const CMat4Dfp32& GlassMat = Glass.m_Pos;
		CMat4Dfp32 OrienMat = GlassMat;
		OrienMat.RecreateMatrix(0,1);//GlassMat.CreateFrom(Glass.m_Pos);
		const CVec3Dfp32& GlassHor = CVec3Dfp32::GetMatrixRow(GlassMat, 0);
		const CVec3Dfp32& GlassVer = CVec3Dfp32::GetMatrixRow(GlassMat, 1);
		const CVec3Dfp32& GlassPos = CVec3Dfp32::GetMatrixRow(GlassMat, 3);

		CBox3Dfp32 LocalBoundBox(CVec3Dfp32(-1,-1,-1),CVec3Dfp32(1,1,1));//(-(GlassHor + GlassVer), (GlassHor + GlassVer));
		//CBox3Dfp32 LocalBoundBox(, CVec3Dfp32(Glass.m_HalfWidth*32, 0.5f, Glass.m_HalfHeight*32));
		CBox3Dfp32 WorldBoundBox;
//		CVec3Dfp32 Hit;
		LocalBoundBox.Transform(GlassMat,WorldBoundBox);

		COBBfp32 GlassBox;

		GlassBox.Create(CVec3Dfp32(1*GlassHor.Length(),0,1*GlassVer.Length()), GlassPos, OrienMat);

		//if(WorldBoundBox.IntersectLine(_Pos1, _Pos2, Hit))
		if(_PTCollision.IntersectOBB(GlassBox))
		{
			// If we have a hit from something else, make sure glass is infront of it
			//bool bHitGlass = true;
			//if(_bHit)
			//{
			//	const fp32 GlassDistance = (Hit - _Pos1).Length();
			//	const fp32 OtherDistance = (_pCInfo->m_Pos - _Pos1).Length();
			//	if(OtherDistance < GlassDistance)
			//		bHitGlass = false;
			//}
			const int& nShards = Glass.m_lShards.Len();
			const CGlassModelEntity::CGlassShard* pShards = Glass.m_lShards.GetBasePtr();
//			const CMat4Dfp32& Mat = Glass.m_Pos;
//			const CVec3Dfp32& Dir = CVec3Dfp32::GetMatrixRow(Mat, 2);
//			const CVec3Dfp32& Pos = CVec3Dfp32::GetMatrixRow(Mat, 3);
			CMat4Dfp32 ShardMat =GlassMat;
			int ShardHit = -1;
			for(int i = 0; i < nShards; i++)
			{
				const CGlassModelEntity::CGlassShard& Shard = pShards[i];
                const CVec3Dfp32 ShardPos = GlassPos + Shard.m_Position;//pShards[i].m_Position;
				//CVec3Dfp32::GetMatrixRow(ShardMat, 3) = Pos + ShardPos;

	//			CVec3Dfp32::GetMatrixRow(ShardMat, 0) = GlassHor*Shard.m_Dimension[0];
	//			CVec3Dfp32::GetMatrixRow(ShardMat, 1) = GlassVer*Shard.m_Dimension[1];

	//			const CVec3Dfp32 ShardHor = GlassHor*Shard.m_Dimension[0];
	//			const CVec3Dfp32 ShardVer = GlassVer*Shard.m_Dimension[1];
				//LocalBoundBox = CBox3Dfp32( -(GlassHor*pShards[i].m_Dimension[0] + GlassVer*pShards[i].m_Dimension[1]),
				//							(GlassHor*pShards[i].m_Dimension[0] + GlassVer*pShards[i].m_Dimension[1]));
				LocalBoundBox.Transform(ShardMat, WorldBoundBox);
	//			if(_PTCollision.IntersectBoundBox(WorldBoundBox),false)
				{
					// We don't care about being to accurate about the shard, so we return here
					ShardHit = i;
					break;
					// Check the shard plane
//					ShardMat.RecreateMatrix(0,1);
//					CPlane3Dfp32 Plane(CVec3Dfp32::GetMatrixRow(ShardMat,2), ShardPos.Length());
//					if(_PTCollision.IntersectPlane(Plane))
//						break;
				}
				_PTCollision.IsValidCollision() = false;
			}

			//if(bHitGlass)
			if(_PTCollision.IsInfrontOfCollision())
			{
				//CWO_ProjectileInstance* pProjectile = &_PTCollision.m_pCluster->m_lProjectileInstance[_PTCollision->m_iProjectile];
				const fp32 Damage = _PTCollision.m_pCluster->ResolveDamageRange(_PTCollision.m_pProjectile->m_TravelDistance + _PTCollision.m_Distance);
                
				// We hit the glass
				if(Glass.m_Durability == 0)
				{
					CVec3Dfp32 Force = _PTCollision.m_pProjectile->m_Velocity;
					Force.k[0] *= 0.0333f; // /= 30.0f;
					Force.k[1] *= 0.0333f; // /= 30.0f;
					Force.k[2] *= 0.0333f; // /= 30.0f;
//					Glass.ShatterAt(_PTCollision.m_Position,_PTCollision.m_Direction,Force);
					//Glass.ShatterShard(ShardHit, _PTCollision.m_Direction, Force);
					
					// NEW
					Force = _PTCollision.m_Position - CVec3Dfp32::GetMatrixRow(_PTCollision.m_pProjectile->m_Position, 3);
					Force.Normalize();
					// NEW

					//Force[1] *= -1.0f;

					Glass.ShatterPoint(_PTCollision.m_Position, Force);
					Glass.m_bDisturbed = true;
				}
				else
				{
					Glass.m_Durability = (int8)(Glass.m_Durability - Damage);
					if(Glass.m_Durability < 0)
					{
						Glass.PreShatter();
						Glass.m_Durability = 0;
					}
				}
				
				if(_PTCollision.m_pCluster)
				{
					// A cluster exist, so we can spawn the impact model here
					//_PTCollision.m_SurfaceName = Glass.m_SurfaceName;
					_PTCollision.m_SurfaceName = GetSurfaceContext()->GetSurfaceName(((Glass.m_Durability == 0) ? Glass.m_BrokenSurfaceID : Glass.m_SurfaceID));
					_PTCollision.m_pCluster->OnPassThroughImpact(_PTCollision);
				}
			}
		}
		
	}
}
