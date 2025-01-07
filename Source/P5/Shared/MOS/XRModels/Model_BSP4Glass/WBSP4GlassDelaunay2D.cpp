#include "PCH.h"

//#include "WBSP4Glass.h"
//#include "WBSP4GlassDelaunay2D.h"
//#include "MFloat.h"


//#pragma xrMsg("optimize off!")
//#pragma optimize("", off)
//#pragma inline_depth(0)


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_Delaunay2D
|__________________________________________________________________________________________________
\*************************************************************************************************/
/*
void CBSP4Glass_Delaunay2D::CDel2DTri::SetCircumCircle(const _CVec2Dint64* _pPntsInt)
{
	const _CVec2Dint64& P0 = _pPntsInt[m_iIndex[0]];
	const _CVec2Dint64& P1 = _pPntsInt[m_iIndex[0]];
	const _CVec2Dint64& P2 = _pPntsInt[m_iIndex[0]];

		  int64  x0 = P0.k[0];
	const int64& y0 = P0.k[1];
		  int64  x1 = P1.k[0];
	const int64& y1 = P1.k[1];
	const int64& x2 = P2.k[0];
	const int64& y2 = P2.k[1];
	const int64 y10 = y1 - y0;
	const int64 y21 = y2 - y1;
	if(y10 == 0)
	{
		if(y21 == 0)
		{
			if(x1 > x0) { if(x2 > x1) x1 = x2; }
			else		{ if(x2 < x0) x0 = x2; }
			m_CenterInt.k[0] = (x0 + x1) >> 1;
			m_CenterInt.k[1] = y0;
		}
		else
		{
			m_CenterInt.k[0] = (x0 + x1) >> 1;
			m_CenterInt.k[1] = (-(x2 - x1) / y21) * (m_CenterInt.k[0] - ((x1 + x2) >> 1)) + ((y1 + y2) >> 1);
		}
	}
	else if(y21 == 0)
	{
		m_CenterInt.k[0] = (x1 + x2) >> 1;
		m_CenterInt.k[1] = (- (x1 - x0) / y10) * (m_CenterInt.k[0] - ((x0 + x1) >> 1)) + ((y0 + y1) >> 1);
	}
	else
	{
		int64 m0 = - (x1 - x0) / y10;
		int64 m1 = - (x2 - x1) / y21;
		int64 mx0 = (x0 + x1) >> 1;
		int64 my0 = (y0 + y1) >> 1;
		m_CenterInt.k[0] = (m0 * mx0 - m1 * ((x1 + x2) >> 1) + ((y1 + y2) >> 1) - my0) / (m0 - m1);
		m_CenterInt.k[1] = m0 * (m_CenterInt.k[0] - mx0) + my0;
	}
    
	_CVec2Dint64 t = _CVec2Dint64(x0, y0) - m_CenterInt;
	m_IntR2 = t.k[0] * t.k[0] + t.k[1] * t.k[1];
	m_IntR = (int64)M_Sqrt((fp8)m_IntR2) + 1;
	//m_IntR = M_Sqrt(m_IntR2) + 1;
}


bool CBSP4Glass_Delaunay2D::CDel2DTri::CCEncompasses(const _CVec2Dint64& _PntInt)
{
	const _CVec2Dint64 Dist = _PntInt - m_CenterInt;
	int64 DistLen = Sqr(Dist.k[0]) + Sqr(Dist.k[1]);
	return (DistLen <= m_IntR2);
}


CBSP4Glass_Delaunay2D::CBSP4Glass_Delaunay2D(uint8 _iType, CGlassAttrib* _pAttrib, int32* _pSeed, CMat4Dfp32* _pWMat)
	: m_pVert(NULL)
	, m_nVert(0)
	, m_piPrim(NULL)
	, m_nPrim(0)
	, m_bOwner(false)
    , m_iType(_iType)
	, m_bCCW(false)
	, m_nGen(0)
	, m_pAttrib(_pAttrib)
{
	m_lVertInt64.Clear();
	m_lTri.Clear();

	// Setup matrix
	if(_pWMat)	m_WMat.CreateFrom(*_pWMat);
	else		m_WMat.Unit();

	// Setup seed
	if(_pSeed)	m_Seed = *_pSeed;
	else		m_Seed = (int32)MRTC_GetRand()->GenRand32();
}


bool CBSP4Glass_Delaunay2D::ErrorCode(const uint32 _ErrorCode)
{
	#ifndef M_RTM
		M_TRACEALWAYS("CBSP4Glass_Delaunay(ERROR): ");
		switch(_ErrorCode)
		{
			case GLASS_DELAUNAY_ERROR_ALLCLEAR:
			{
				M_TRACEALWAYS("Failed even thought there where no errors! WTF?!");
				break;
			}

			case GLASS_DELAUNAY_ERROR_MAXVERTICES:
			{
				M_TRACEALWAYS("Exceeding maximum allowed vertices of 65535");
				break;
			}

			case GLASS_DELAUNAY_ERROR_NOTYPE:
			{
				M_TRACEALWAYS("Invalid or unsupported mapping type specified!");
				break;
			}

			case GLASS_DELAUNAY_ERROR_MAXOUTTRIANGLES:
			{
				M_TRACEALWAYS("Too few triangles in output list!");
				break;
			}

			case GLASS_DELAUNAY_ERROR_MAXREMOVEINDICES:
			{
				M_TRACEALWAYS("Too few remove indices in temporary list!");
				break;
			}

			case GLASS_DELAUNAY_ERROR_NOATTRIBUTE:
			{
				M_TRACEALWAYS("No glass attributes specified!");
				break;
			}

			default:
			{
				M_TRACEALWAYS("Unknown error!");
				break;
			}
		}

		M_TRACEALWAYS("\n");
	#endif

	return false;
}


bool CBSP4Glass_Delaunay2D::UploadBaseGeometry(CVec3Dfp32* _pVertices, uint32 _nVertices, uint16* _piPrim, uint32 _nPrim, uint32 _nGen, bool _bOwner)
{
	if(!m_pAttrib)
		return ErrorCode(GLASS_DELAUNAY_ERROR_NOATTRIBUTE);

	m_nVert = _nVertices;
	m_pVert = _pVertices;
	m_nPrim = _nPrim;
	m_piPrim = _piPrim;
	m_bOwner = _bOwner;
	m_nGen = _nGen;

	if(m_nVert > 65535)
		return ErrorCode(GLASS_DELAUNAY_ERROR_MAXVERTICES);

	// Send vertices into mapped space
	switch(m_iType)
	{
		case GLASS_DELAUNAY_TYPE_INT64:
		{
			m_Max_Int = GLASS_INT64_MAX;
			m_Min_Int = GLASS_INT64_MIN;
			break;
		}

		case GLASS_DELAUNAY_TYPE_INT32:
		{
			m_Max_Int = GLASS_INT32_MAX;
			m_Min_Int = GLASS_INT32_MIN;
			break;
		}

		case GLASS_DELAUNAY_TYPE_INT16:
		{
			m_Max_Int = GLASS_INT16_MAX;
			m_Min_Int = GLASS_INT16_MIN;
			break;
		}

		case GLASS_DELAUNAY_TYPE_INT8:
		{
			m_Max_Int = GLASS_INT8_MAX;
			m_Min_Int = GLASS_INT8_MIN;
			break;
		}

		default:
		{
			return ErrorCode(GLASS_DELAUNAY_ERROR_NOTYPE);
		}
	}

	// Done uploading geometry
	return true;
}
*/


/* -----------------------------------------------------------------------
void CBSP4Glass_Delaunay2D::GeneratePoints(const uint32 _nPoints)
{
	switch(m_iType)
	{
		case GLASS_DELAUNAY_TYPE_INT64:
		case GLASS_DELAUNAY_TYPE_INT32:
		case GLASS_DELAUNAY_TYPE_INT16:
		case GLASS_DELAUNAY_TYPE_INT8:
		{
			GeneratePointsInt(_nPoints);
			break;
		}

		default:
		{
			ErrorCode(GLASS_DELAUNAY_ERROR_NOTYPE);
			break;
		}
	}
}
   ------------------------------------------------------------------------ */


/*
void CBSP4Glass_Delaunay2D::GeneratePointsInt(const uint32 _nPoints)
{
	_CVec2Dint64* pVertInt = m_lVertInt64.GetBasePtr() + m_nVert;

	// Generate random points in mapping space
    for(int i = 0, iRand = 0; i < m_nGen; i++)
	{
		const fp32 RandX = (-1.0f + (2.0f * MFloat_GetRand(m_Seed + iRand++)));
		const fp32 RandY = (-1.0f + (2.0f * MFloat_GetRand(m_Seed + iRand++)));
		pVertInt[i].k[0] = Clamp((int64)(RandX * m_Max_Int), -m_Max_Int, m_Max_Int);
		pVertInt[i].k[1] = Clamp((int64)(RandY * m_Max_Int), -m_Max_Int, m_Max_Int);
	}
}


void CBSP4Glass_Delaunay2D::CalcIntMapping()
{
	const CVec2Dfp32& DimRcp = m_pAttrib->m_DimRcp;
	const fp32 Half = 0.5f;
	const CVec2Dfp32& Min = m_pAttrib->m_Min;
	const uint8& iComp0 = m_pAttrib->m_liComp[0];
	const uint8& iComp1 = m_pAttrib->m_liComp[1];
	const int nV = m_nVert + m_nGen;

	// Allocate storage for the vertices and triangles
	m_lVertInt64.SetLen(nV);
	m_lTri.SetLen(m_nPrim + (m_nGen * 3));
	_CVec2Dint64* pVert = m_lVertInt64.GetBasePtr();

	// Map uploaded points
	for(int i = 0; i < m_nVert; i++)
	{
		// Send 3d vertex down to 2d formated -1 -> 1 space
		const fp32 InX = (((m_pVert[i].k[iComp0] - Min.k[0]) * DimRcp.k[0]) - Half) * 2.0f;
		const fp32 InY = (((m_pVert[i].k[iComp1] - Min.k[1]) * DimRcp.k[1]) - Half) * 2.0f;

		// Convert 2d -1 -> 1 vertex to int64 -m_Max_Int -> m_Max_Int
		pVert[i].k[0] = (int64)(Clamp(InX, -1.0f, 1.0f) * m_Max_Int);
		pVert[i].k[1] = (int64)(Clamp(InY, -1.0f, 1.0f) * m_Max_Int);
	}

	// Generate random points
	GeneratePointsInt(m_nGen);

	// Setup triangle information
	CDel2DTri* pTris = m_lTri.GetBasePtr();
	for(int i = 0, j = 0; i < m_nPrim; i++, j += 3)
		pTris[i].Set(m_piPrim[j], m_piPrim[j+1], m_piPrim[j+2]);

	// Check glass winding
	if(m_nPrim)
		m_bCCW = pTris[0].IsCCW(m_pVert, m_pAttrib->m_Plane.n);
}


void CBSP4Glass_Delaunay2D::RestoreMappingFromInt(TThinArray<CVec3Dfp32>& _lOutputVertices)
{
	int nV = m_lVertInt64.Len();
	_CVec2Dint64* pVertInt64 = m_lVertInt64.GetBasePtr();

	// Output double sided
	_lOutputVertices.SetLen(nV * 2);
	CVec3Dfp32* pOutVerts = _lOutputVertices.GetBasePtr();

	// Get some attributes
	const CVec2Dfp32& DimRcp = m_pAttrib->m_DimRcp;
	const CVec2Dfp32 Dim = CVec2Dfp32(1.0f / DimRcp.k[0], 1.0f / DimRcp.k[1]);
	const uint8& iComp0 = m_pAttrib->m_liComp[0];
	const uint8& iComp1 = m_pAttrib->m_liComp[1];
	const uint8& iComp2 = 3 - (iComp0 + iComp1);
	const CVec3Dfp32& Middle = m_pAttrib->m_MiddlePoint;
	const fp32& Thickness = m_pAttrib->m_Thickness;
	const fp32& Distance = m_pAttrib->m_Plane.d;
	const CVec3Dfp32& PlaneN	= m_pAttrib->m_Plane.n;
	const fp8 MaxIntFp4 = (fp32)m_Max_Int;

	// Setup output vertices
	for(int i = 0; i < nV; i++)
	{
		CVec3Dfp32& VPos0 = pOutVerts[i];
		CVec3Dfp32& VPos1 = pOutVerts[nV+i];
		const _CVec2Dint64& PntInt = pVertInt64[i];

        VPos0.k[iComp0] = Middle.k[iComp0] + ((((fp32)PntInt.k[0] / MaxIntFp4) * 0.5f) * Dim.k[0]);
		VPos0.k[iComp1] = Middle.k[iComp1] + ((((fp32)PntInt.k[1] / MaxIntFp4) * 0.5f) * Dim.k[1]);
		VPos0.k[iComp2] = 0;
		VPos0 -= (PlaneN * Distance);

		VPos1 = (VPos0 + (-PlaneN * Thickness)) * m_WMat;
		VPos0 = VPos0 * m_WMat;
	}
}


bool CBSP4Glass_Delaunay2D::Triangulate(TThinArray<CVec3Dfp32>& _lOutputVertices)
{
	if(!m_pAttrib)
		return ErrorCode(GLASS_DELAUNAY_ERROR_NOATTRIBUTE);

	// Triangulate using correct mapping type
	if(m_iType == GLASS_DELAUNAY_TYPE_INT64)
	{
		CalcIntMapping();
		TriangulateInt();
		RestoreMappingFromInt(_lOutputVertices);
	}
	else
		return ErrorCode(GLASS_DELAUNAY_ERROR_NOTYPE);

	return true;
}


void CBSP4Glass_Delaunay2D::TriangulateInt()
{
	int nBase = m_nVert;
	int nTotalPoints = nBase + m_nGen;

	int nWorkTris = m_nPrim;//m_lTri.Len();
	CDel2DTri* pWorkTris = m_lTri.GetBasePtr();
	
	int nOutTris = ((nTotalPoints - 3) * 2) + 1;
	m_lTriOut.SetLen(nOutTris);
	CDel2DTri* pOutTris = m_lTriOut.GetBasePtr();
	int iOutTri = 0;
	
	TThinArray<CDel2DEdge> lEdges;
	int nOutEdges = nOutTris * 3;
	lEdges.SetLen(nOutEdges);
	CDel2DEdge* pEdges = lEdges.GetBasePtr();
	int iOutEdge = 0;

	TThinArray<uint32> liRemove;
	int iRemove = 0;
	int nRemove = nOutTris;
	liRemove.SetLen(nRemove);
	uint32* piRemove = liRemove.GetBasePtr();
	

	_CVec2Dint64* pPntInt = m_lVertInt64.GetBasePtr();
	for(int i = nBase; i < nTotalPoints; i++)
	{
		_CVec2Dint64& PntInt = pPntInt[i];

		// Find which triangle this point belongs to
		for(int j = 0; j < nWorkTris; j++)
		{
			CDel2DTri& WorkTri = pWorkTris[j];
			if(WorkTri.IsLeftOf(PntInt))
			{
				#ifndef M_RTM
				{
					if(iOutTri + 1 > nOutTris)
					{
						// Grow list or report error?
						ErrorCode(GLASS_DELAUNAY_ERROR_MAXOUTTRIANGLES);
						break;
					}

					if(iRemove + 1 > nRemove)
					{
						// Grow list or report error?
						ErrorCode(GLASS_DELAUNAY_ERROR_MAXREMOVEINDICES);
						break;
					}
				}
				#endif

				pOutTris[iOutTri++] = WorkTri;
				piRemove[iRemove++] = j;
			}
			else if(WorkTri.CCEncompasses(PntInt))
			{
				// Remove hot triangle
				HandleEdge(WorkTri, pEdges, iOutEdge);
				piRemove[iRemove++] = j;
			}
		}

		// Remove triangles
		if(iRemove >= nWorkTris)
			nWorkTris = 0;
		else
		{
			// Shrink work tri array
			for(int j = 0; j < iRemove; j++)
				pWorkTris[piRemove[iRemove - 1 - j]] = pWorkTris[nWorkTris - 1 - j];
			
			nWorkTris -= iRemove;
		}
		iRemove = 0;

		// Add edges to triangle list
		for(int j = 0; j < iOutEdge; j++)
		{
			pWorkTris[nWorkTris+j].Set(pEdges[j].m_iIndex[0], pEdges[j].m_iIndex[1], i);
			pWorkTris[nWorkTris+j].SetCircumCircle(pPntInt);
		}
		nWorkTris += iOutEdge;
		iOutEdge = 0;

		// Remap point to original space. (Only do this on generated points ??)
	}

	// Copy remaning work triangles to output
	for(int i = 0; i < nWorkTris; i++)
		pOutTris[iOutTri++] = pWorkTris[i];

	m_nTriOut = iOutTri;

	// Free up some memory
	lEdges.Clear();
	liRemove.Clear();
	m_lTri.Clear();
}


void CBSP4Glass_Delaunay2D::HandleEdge(CDel2DTri& _Tri, CDel2DEdge* _pEdges, int& _nEdges)
{
	_CVec2Dint64* pVerts = m_lVertInt64.GetBasePtr();
	for(int i = 0; i < 3; i++)
	{
		const uint8 i2 = (i + 1) % 3;
		const _CVec2Dint64& Pnt0 = pVerts[_Tri.m_iIndex[_Tri.m_iWind[i]]];
		const _CVec2Dint64& Pnt1 = pVerts[_Tri.m_iIndex[_Tri.m_iWind[i2]]];

		CDel2DEdge Edge;
		Edge.Set(_Tri.m_iIndex[_Tri.m_iWind[i2]], _Tri.m_iIndex[_Tri.m_iWind[i]]);
		if((Pnt0.k[0] < Pnt1.k[0]) || (Pnt0.k[0] == Pnt1.k[0] && Pnt0.k[1] < Pnt1.k[1]))
			Edge.SwapIndices();

		const int16 iEdge = FindEdge(Edge, _pEdges, _nEdges);
		if(iEdge < 0) _pEdges[_nEdges++] = Edge;
		else if(_nEdges > 0) _pEdges[iEdge] = _pEdges[--_nEdges];
	}
}


int CBSP4Glass_Delaunay2D::FindEdge(CDel2DEdge& _Edge, CDel2DEdge* _pEdges, int& _nEdges)
{
	for(int i = 0; i < _nEdges; i++)
	{
		if(_pEdges[i] == _Edge)
			return i;
	}

	return -1;
}
*/





/*
void CXR_Model_BSP4Glass_Instance::Client_CrushGlass(const CMat4Dfp32& _WMat, const int32& _iInstance, const int32& _Seed, const CVec3Dfp32& _Position, const CVec3Dfp32& _LocalPosition)
{
	if(_iInstance >= (uint16)-1)
		return;

	SGlassModel* pModels = m_lModels.GetBasePtr();
	SGlassModel& Model = pModels[_iInstance];

	// Retrive some attributes from the glass instance
	const CGlassAttrib& Attrib = Model.m_Attrib;
	const CVec2Dfp32& DimRcp = Attrib.m_DimRcp;
	const uint8& iComp0 = Attrib.m_liComp[0];
	const uint8& iComp1 = Attrib.m_liComp[1];

	const CVec2Dfp32 LocalPosition = CVec2Dfp32(_LocalPosition.k[iComp0], _LocalPosition.k[iComp1]) - Attrib.m_Min;
	const CVec2Dfp32 LP = (CVec2Dfp32(LocalPosition.k[0] * DimRcp.k[0], LocalPosition.k[1] * DimRcp.k[1]) - CVec2Dfp32(0.5f)) * 2.0f;

	// Send geometry through the Delaunay triangulator
	CXR_Glass_Delaunay2D Delaunay(_Seed);

	int nV = Model.m_lV.Len() / 2;
	int nT = Model.m_nRenderMainPrim;//m_lFrontP.Len() / 3;
	Delaunay.SetBasePrimitiveRecalc(Model.m_lV.GetBasePtr(), nV, Attrib, Model.m_lFrontP.GetBasePtr(), nT);
	if(!Delaunay.GeneratePoints(LP, 12, 0.2f))
		return;

	Delaunay.Triangulate();

	// Select triangles around hit location and fetch number of solid vertices
	TThinArray<uint8> lTriTag;
	TThinArray<uint16> lPntsTag;
	uint16 nPntsTag = 0;
	uint16 nTriTag = 0;
	
	Delaunay.SelectTriangles(LP, 0.1f, lTriTag, nTriTag, lPntsTag, nPntsTag);
	uint8* pTriTag = lTriTag.GetBasePtr();
	uint16* pPntsTag = lPntsTag.GetBasePtr();

	// Get triangulated data
	CXR_Glass_Delaunay2D::CDelaunay2DTriangle* pTriangles = Delaunay.GetTrianglesBasePtr();
	CVec2Dfp32* pPoints2D = Delaunay.GetPointsBasePtr();
	
	// Get number of triangles and points left in base mesh
	nV = nPntsTag;
	nT = Delaunay.GetNumTriangles();
	
	// Get model attributes
	const CVec3Dfp32& PlaneN	= Attrib.m_Plane.n;
	const CVec3Dfp32& Middle = Attrib.m_MiddlePoint;
	const CVec2Dfp32 Dim = CVec2Dfp32(1.0f / DimRcp.k[0], 1.0f / DimRcp.k[1]);
	const fp32& Thickness = Attrib.m_Thickness;
	const fp32& Distance = Attrib.m_Plane.d;//M_Fabs(Attrib.m_Plane.d);
	const fp32& TxtWidthInv = Attrib.m_TxtWidthInv;
	const fp32& TxtHeightInv = Attrib.m_TxtHeightInv;
		
	// Setup texture coordinates data
	CVec3Dfp32 TangU1, TangU2, TangV1, TangV2;
	CXR_PlaneMapping* pMapping = Model.m_pMapping;

	CVec2Dfp32 TProjMin(_FP4_MAX);
	CVec2Dfp32 TProjMax(-_FP4_MAX);

	const CVec3Dfp32& UVec = pMapping->m_U;
	const CVec3Dfp32& VVec = pMapping->m_V;
	const fp32& UOffset = pMapping->m_UOffset;
	const fp32& VOffset = pMapping->m_VOffset;

	const fp32 UVecLenSqrInv = 1.0f / (UVec * UVec);
	const fp32 VVecLenSqrInv = 1.0f / (VVec * VVec);

	const fp32 ULengthRecp = 1.0f / pMapping->m_U.Length();
	const fp32 VLengthRecp = 1.0f / pMapping->m_V.Length();

	// Tangent setups
	{
		UVec.Scale(ULengthRecp, TangU1);
		VVec.Scale(VLengthRecp, TangV1);
		TangU2 = TangU1;
		TangV2 = TangV1;

		TangU1.Combine(PlaneN, -(PlaneN * TangU1), TangU1);
		TangV1.Combine(PlaneN, -(PlaneN * TangV1), TangV1);
		TangU2.Combine(-(PlaneN), PlaneN * TangU2, TangU2);
		TangV2.Combine(-(PlaneN), PlaneN * TangV2, TangV2);
			
		TangU1.Normalize();
		TangV1.Normalize();
		TangU2.Normalize();
		TangV2.Normalize();
	}

	const fp32 NTangU1 = -( PlaneN * TangU1);
	const fp32 NTangV1 = -( PlaneN * TangV1);
	const fp32 NTangU2 = -(-PlaneN * TangU2);
	const fp32 NTangV2 = -(-PlaneN * TangV2);

	CVec3Dfp32* pV = NULL;
	CPixel32*  pC = NULL;
	CVec3Dfp32* pN = NULL;
	CVec3Dfp32* pTangU = NULL;
	CVec3Dfp32* pTangV = NULL;
	CVec2Dfp32* pTV = NULL;

	// Shard setup
	{
		// First of we set up the shards, this is in world space
		const int nShardModels = m_lShardModels.Len();
		const int nShardsV = nTriTag * 6;
		m_lShardModels.SetLen(nShardModels+1);
		CGlassShardModel& ShardModel = m_lShardModels[nShardModels];

		ShardModel.m_iSurface = Model.m_iSurface;
		ShardModel.m_nShards = nTriTag;
		ShardModel.m_SpawnTick = -1;

		// Allocate some new space for the shard pieces
		ShardModel.m_lV.SetLen(nShardsV);
		ShardModel.m_lN.SetLen(nShardsV);
		ShardModel.m_lC.SetLen(nShardsV);
		ShardModel.m_lTV.SetLen(nShardsV);
		ShardModel.m_lTangU.SetLen(nShardsV);
		ShardModel.m_lTangV.SetLen(nShardsV);
		ShardModel.m_lP.SetLen(nTriTag * 24);
		ShardModel.m_lVelocity.SetLen(nTriTag);

		pV		= ShardModel.m_lV.GetBasePtr();
		pC		= ShardModel.m_lC.GetBasePtr();
		pN		= ShardModel.m_lN.GetBasePtr();
		pTangU	= ShardModel.m_lTangU.GetBasePtr();
		pTangV	= ShardModel.m_lTangV.GetBasePtr();
		pTV		= ShardModel.m_lTV.GetBasePtr();
		uint16* pP = ShardModel.m_lP.GetBasePtr();
		CVec3Dfp32* pVelocity = ShardModel.m_lVelocity.GetBasePtr();

		// Reconstruct all shards from tags
		int iV = 0;
		nT = Delaunay.GetNumTriangles();
		int iT = 0;
		int iP = 0;
		for(int i = 0; i < nT; i++)
		{
			if(!pTriTag[i]) continue;

			CXR_Glass_Delaunay2D::CDelaunay2DTriangle& Triangle = pTriangles[i];
			
			// Generate shard indices
			pP[iP++] = iV;
			pP[iP++] = iV+1;
			pP[iP++] = iV+2;

			pP[iP++] = iV+3;
			pP[iP++] = iV+3+1;
			pP[iP++] = iV+3+2;

			pP[iP++] = iV;
			pP[iP++] = iV+1;
			pP[iP++] = iV+3+1;
			pP[iP++] = iV;
			pP[iP++] = iV+3;
			pP[iP++] = iV+3+1;

			pP[iP++] = iV;
			pP[iP++] = iV+2;
			pP[iP++] = iV+3+2;
			pP[iP++] = iV;
			pP[iP++] = iV+3;
			pP[iP++] = iV+3+2;

			pP[iP++] = iV+1;
			pP[iP++] = iV+2;
			pP[iP++] = iV+3+2;
			pP[iP++] = iV+1;
			pP[iP++] = iV+3+1;
			pP[iP++] = iV+3+2;

			// Reconstruct shard render data
			for(int j = 0; j < 3; j++)
			{
				//const uint16 iPnt = pPntsTag[Triangle.m_iIndex.k[j]];
				const uint16 iPnt2 = Triangle.m_iIndex.k[j];

				// Get point
				const int inV = iV+3;
				CVec2Dfp32& Point = pPoints2D[iPnt2];

				// Vertex creation
				{
					CVec3Dfp32 VertexPosition(0);

					VertexPosition.k[iComp0] = Middle.k[iComp0] + ((Point.k[0] * 0.5f) * Dim.k[0]);
					VertexPosition.k[iComp1] = Middle.k[iComp1] + ((Point.k[1] * 0.5f) * Dim.k[1]);

					const CVec3Dfp32 V2WPos = (VertexPosition - (PlaneN * Distance));
					pV[iV] = V2WPos * _WMat;
					pV[inV] = (V2WPos + (-PlaneN * Thickness)) * _WMat;
				}

				// Normals
				{
					pN[iV] = PlaneN;
					pN[inV] = -PlaneN;
				}

				// Colors
				{
					pC[iV] = CPixel32(255,255,255,255);
					pC[inV] = CPixel32(255,255,255,255);
				}

				// Texture coordinates
				{
					const CVec3Dfp32& V = pV[iV];
					const fp32 UProj = (V.k[0]*UVec.k[0] + V.k[1]*UVec.k[1] + V.k[2]*UVec.k[2])*UVecLenSqrInv + UOffset;
					const fp32 VProj = (V.k[0]*VVec.k[0] + V.k[1]*VVec.k[1] + V.k[2]*VVec.k[2])*VVecLenSqrInv + VOffset;
					if(UProj > TProjMax.k[0]) TProjMax.k[0] = UProj;
					if(VProj > TProjMax.k[1]) TProjMax.k[1] = VProj;
					if(UProj < TProjMin.k[0]) TProjMin.k[0] = UProj;
					if(VProj < TProjMin.k[1]) TProjMin.k[1] = VProj;

					pTV[iV].k[0] = pTV[inV].k[0] = UProj * TxtWidthInv;
					pTV[iV].k[1] = pTV[inV].k[1] = VProj * TxtHeightInv;
				}

				// Tangents
				{
					TangU1.Combine(PlaneN, NTangU1, pTangU[iV]);
					TangV1.Combine(PlaneN, NTangV1, pTangV[iV]);

					TangU2.Combine(-PlaneN, NTangU2, pTangU[inV]);
					TangV2.Combine(-PlaneN, NTangV2, pTangV[inV]);

					pTangU[iV].Normalize();
					pTangV[iV].Normalize();

					pTangU[inV].Normalize();
					pTangV[inV].Normalize();
				}
				
				iV++;
			}

			iV += 3;

			// Set velocity for shard
            pVelocity[iT] = CVec3Dfp32(0);
			iT++;
		}

		{
			#ifdef PLATFORM_PS2
				const CVec2Dfp32 TMid = CVec2Dfp32(TProjMin[0] * TxtWidthInv, TProjMin[1] * TxtHeightInv);
				const int nv = nShardsV;// * 2;
				for(int i = 0; i < nv; i++)
				{
					pTV[i] -= TMid;
					pTV[i].k[0] = MaxMT(pTV[i].k[0], 0);
					pTV[i].k[1] = MaxMT(pTV[i].k[1], 0);
				}
			#else
				const CVec2Dfp32 TMid = CVec2Dfp32(RoundToInt(((TProjMin[0] + TProjMax[0]) * 0.5f * TxtWidthInv ) / 16.0f) * 16.0f,
												RoundToInt(((TProjMin[1] + TProjMax[1]) * 0.5f * TxtHeightInv) / 16.0f) * 16.0f);
				if(TMid.k[0] != 0 || TMid.k[0] != 0)
				{
					const int nv = nShardsV;// * 2;
					for(int i = 0; i < nv; i++)
						pTV[i] -= TMid;
				}
			#endif
		}
	}

	// Allocate some new space for the glass model
	Model.m_lV.SetLen(nV * 2);
	Model.m_lC.SetLen(nV * 2);
	Model.m_lN.SetLen(nV * 2);
	Model.m_lTangU.SetLen(nV * 2);
	Model.m_lTangV.SetLen(nV * 2);
	Model.m_lTV.SetLen(nV * 2);

	pV		= Model.m_lV.GetBasePtr();
	pC		= Model.m_lC.GetBasePtr();
	pN		= Model.m_lN.GetBasePtr();
	pTangU	= Model.m_lTangU.GetBasePtr();
	pTangV	= Model.m_lTangV.GetBasePtr();
	pTV		= Model.m_lTV.GetBasePtr();

	// Reconstruct solids
	nV = Delaunay.GetNumPoints();
	for(int iV = 0, i = 0; iV < nV; iV++)
	{
		// Make sure this is a solid point
		if(!pPntsTag[iV])
			continue;

		// Get point
		const int inV = i+nPntsTag;
		CVec2Dfp32& Point = pPoints2D[iV];
		
		// Vertex creation
		{
			CVec3Dfp32 VertexPosition(0);

			VertexPosition.k[iComp0] = Middle.k[iComp0] + ((Point.k[0] * 0.5f) * Dim.k[0]);
			VertexPosition.k[iComp1] = Middle.k[iComp1] + ((Point.k[1] * 0.5f) * Dim.k[1]);

			pV[i] = VertexPosition - (PlaneN * Distance);
			pV[inV] = pV[i] + (-PlaneN * Thickness);
		}

		// Normals
		{
			pN[i] = PlaneN;
			pN[inV] = -PlaneN;
		}

		// Colors
		{
			pC[i] = CPixel32(255,255,255,255);
			pC[inV] = CPixel32(255,255,255,255);
		}

		// Texture coordinates
		{
			const CVec3Dfp32& V = pV[i];
			const fp32 UProj = (V.k[0]*UVec.k[0] + V.k[1]*UVec.k[1] + V.k[2]*UVec.k[2])*UVecLenSqrInv + UOffset;
			const fp32 VProj = (V.k[0]*VVec.k[0] + V.k[1]*VVec.k[1] + V.k[2]*VVec.k[2])*VVecLenSqrInv + VOffset;
			if(UProj > TProjMax.k[0]) TProjMax.k[0] = UProj;
			if(VProj > TProjMax.k[1]) TProjMax.k[1] = VProj;
			if(UProj < TProjMin.k[0]) TProjMin.k[0] = UProj;
			if(VProj < TProjMin.k[1]) TProjMin.k[1] = VProj;

			pTV[i].k[0] = pTV[inV].k[0] = UProj * TxtWidthInv;
			pTV[i].k[1] = pTV[inV].k[1] = VProj * TxtHeightInv;
		}

		// Tangents
		{
			TangU1.Combine(PlaneN, NTangU1, pTangU[i]);
			TangV1.Combine(PlaneN, NTangV1, pTangV[i]);

			TangU2.Combine(-PlaneN, NTangU2, pTangU[inV]);
			TangV2.Combine(-PlaneN, NTangV2, pTangV[inV]);

			pTangU[i].Normalize();
			pTangV[i].Normalize();

			pTangU[inV].Normalize();
			pTangV[inV].Normalize();
		}
		
		i++;
	}

	{
		#ifdef PLATFORM_PS2
			const CVec2Dfp32 TMid = CVec2Dfp32(TProjMin[0] * TxtWidthInv, TProjMin[1] * TxtHeightInv);
			const int nv = nPntsTag * 2;
			for(int i = 0; i < nv; i++)
			{
				pTV[i] -= TMid;
				pTV[i].k[0] = MaxMT(pTV[i].k[0], 0);
				pTV[i].k[1] = MaxMT(pTV[i].k[1], 0);
			}
		#else
			const CVec2Dfp32 TMid = CVec2Dfp32(RoundToInt(((TProjMin[0] + TProjMax[0]) * 0.5f * TxtWidthInv ) / 16.0f) * 16.0f,
											 RoundToInt(((TProjMin[1] + TProjMax[1]) * 0.5f * TxtHeightInv) / 16.0f) * 16.0f);
			if(TMid.k[0] != 0 || TMid.k[0] != 0)
			{
				const int nv = nPntsTag * 2;
				for(int i = 0; i < nv; i++)
					pTV[i] -= TMid;
			}
		#endif
	}

	nT = Delaunay.GetNumTriangles();
	int nP = (nT - nTriTag) * 3;
	int iP = 0;

	Model.m_lFrontP.SetLen(nP*2);
	//Model.m_lBackP.SetLen(nP);
	Model.m_nRenderMainPrim = (nT - nTriTag);
	
	uint16* pFrontP = Model.m_lFrontP.GetBasePtr();
	uint16* pBackP = pFrontP + nP;
	//Model.m_pBackP = Model.m_lBackP.GetBasePtr();

	CBSP4Glass_BuildEdge Edges;
	Edges.PrepareBuild(nT - nTriTag);

	for(int i = 0, j = 0; i < nT; i++)
	{
		if(pTriTag[i]) continue;

		CXR_Glass_Delaunay2D::CDelaunay2DTriangle& Triangle = pTriangles[i];

		const uint16 iPnt0 = pPntsTag[Triangle.m_iIndex.k[0-j]] - 1;
		const uint16 iPnt1 = pPntsTag[Triangle.m_iIndex.k[1]] - 1;
		const uint16 iPnt2 = pPntsTag[Triangle.m_iIndex.k[2+j]] - 1;

		j = -j - 2;

		Edges.AddEdge(iPnt0, iPnt1);
		Edges.AddEdge(iPnt1, iPnt2);
		Edges.AddEdge(iPnt2, iPnt0);

		pFrontP[iP] = iPnt0;
		pBackP[iP] = iPnt0 + nPntsTag; iP++;

		pFrontP[iP] = iPnt1;
		pBackP[iP] = iPnt1 + nPntsTag; iP++;
		
		pFrontP[iP] = iPnt2;
		pBackP[iP] = iPnt2 + nPntsTag; iP++;
	}

	const uint16& nTotalEdges = Edges.NumEdges();
	const uint16& nEdges = Edges.NumRealEdges();
	nP = nEdges * 6;
	Model.m_lEdgeP.SetLen(nP);
	Model.m_nRenderEdgePrim = nEdges * 2;
	uint16* pEdgeP = Model.m_lEdgeP.GetBasePtr();
	CBSP4Glass_BuildEdgeData* pEdgeData = Edges.GetBasePtr();

	iP = 0;
	for(int i = 0; i < nTotalEdges; i++)
	{
		// Add only valid edges
		if(pEdgeData[i].m_n < 2)
		{
			const uint16& i1 = pEdgeData[i].m_i1;
			const uint16& i2 = pEdgeData[i].m_i2;

			pEdgeP[iP++] = i1;
			pEdgeP[iP++] = i2;
			pEdgeP[iP++] = i2 + nPntsTag;

			pEdgeP[iP++] = i1;
			pEdgeP[iP++] = i1 + nPntsTag;
			pEdgeP[iP++] = i2 + nPntsTag;
		}
	}
}
*/






















/*
void CXR_Model_BSP4Glass_Instance::Client_CrushGlassSurface(const CMat4Dfp32& _WMat, const int32& _iInstance, const int32& _Seed, const CVec3Dfp32& _Position, const CVec3Dfp32& _LocalPosition, const CVec3Dfp32& _Force, const fp32& _ForceScale)
{
	if(_iInstance >= (uint16)-1)
		return;

	SGlassModel* pModels = m_lModels.GetBasePtr();
	SGlassModel& Model = pModels[_iInstance];

	// Retrive some attributes from the glass instance
	const CGlassAttrib& Attrib = Model.m_Attrib;
	const CVec2Dfp32& DimRcp = Attrib.m_DimRcp;
	const uint8& iComp0 = Attrib.m_liComp[0];
	const uint8& iComp1 = Attrib.m_liComp[1];

	const CVec2Dfp32 LocalPosition = CVec2Dfp32(_LocalPosition.k[iComp0], _LocalPosition.k[iComp1]) - Attrib.m_Min;
	const CVec2Dfp32 LP = (CVec2Dfp32(LocalPosition.k[0] * DimRcp.k[0], LocalPosition.k[1] * DimRcp.k[1]) - CVec2Dfp32(0.5f)) * 2.0f;

	// Send geometry through the Delaunay triangulator
	CXR_Glass_Delaunay2D Delaunay(_Seed);

	int nV = Model.m_lV.Len() / 2;
	int nT = Model.m_nRenderMainPrim;//m_lFrontP.Len() / 3;
	Delaunay.SetBasePrimitiveRecalc(Model.m_pV, nV, Attrib, Model.m_pFrontP, nT);
	//Delaunay.GeneratePoints(40 + TruncToInt((MFloat_GetRand(_Seed) * 25.0f)), LP);
	Delaunay.GenerateSurfacePoints(40 + TruncToInt((MFloat_GetRand(_Seed) * 25.0f)), LP);
	Delaunay.Triangulate();

	// Get triangulated data
	CXR_Glass_Delaunay2D::CDelaunay2DTriangle* pTriangles = Delaunay.GetTrianglesBasePtr();
	CVec2Dfp32* pPoints2D = Delaunay.GetPointsBasePtr();
	
	// Get number of triangles and points left in base mesh
	nV = Delaunay.GetNumPoints();
	nT = Delaunay.GetNumTriangles();
	
	// Get model attributes
	const CVec3Dfp32& PlaneN	= Attrib.m_Plane.n;
	const CVec3Dfp32& Middle = Attrib.m_MiddlePoint;
	const CVec2Dfp32 Dim = CVec2Dfp32(1.0f / DimRcp.k[0], 1.0f / DimRcp.k[1]);
	const fp32& Thickness = Attrib.m_Thickness;
	const fp32& Distance = Attrib.m_Plane.d;//M_Fabs(Attrib.m_Plane.d);
	const fp32& TxtWidthInv = Attrib.m_TxtWidthInv;
	const fp32& TxtHeightInv = Attrib.m_TxtHeightInv;
		
	// Setup texture coordinates data
	CVec3Dfp32 TangU1, TangU2, TangV1, TangV2;
	CXR_PlaneMapping* pMapping = Model.m_pMapping;

	CVec2Dfp32 TProjMin(_FP4_MAX);
	CVec2Dfp32 TProjMax(-_FP4_MAX);

	const CVec3Dfp32& UVec = pMapping->m_U;
	const CVec3Dfp32& VVec = pMapping->m_V;
	const fp32& UOffset = pMapping->m_UOffset;
	const fp32& VOffset = pMapping->m_VOffset;

	const fp32 UVecLenSqrInv = 1.0f / (UVec * UVec);
	const fp32 VVecLenSqrInv = 1.0f / (VVec * VVec);

	const fp32 ULengthRecp = 1.0f / pMapping->m_U.Length();
	const fp32 VLengthRecp = 1.0f / pMapping->m_V.Length();

	// Tangent setups
	{
		UVec.Scale(ULengthRecp, TangU1);
		VVec.Scale(VLengthRecp, TangV1);
		TangU2 = TangU1;
		TangV2 = TangV1;

		TangU1.Combine(PlaneN, -(PlaneN * TangU1), TangU1);
		TangV1.Combine(PlaneN, -(PlaneN * TangV1), TangV1);
		TangU2.Combine(-(PlaneN), PlaneN * TangU2, TangU2);
		TangV2.Combine(-(PlaneN), PlaneN * TangV2, TangV2);
			
		TangU1.Normalize();
		TangV1.Normalize();
		TangU2.Normalize();
		TangV2.Normalize();
	}

	const fp32 NTangU1 = -( PlaneN * TangU1);
	const fp32 NTangV1 = -( PlaneN * TangV1);
	const fp32 NTangU2 = -(-PlaneN * TangU2);
	const fp32 NTangV2 = -(-PlaneN * TangV2);

	CVec3Dfp32* pV = NULL;
	CPixel32*  pC = NULL;
	CVec3Dfp32* pN = NULL;
	CVec3Dfp32* pTangU = NULL;
	CVec3Dfp32* pTangV = NULL;
	CVec2Dfp32* pTV = NULL;

	// Shard setup
	{
		// First of we set up the shards, this is in world space
		const int nShardModels = m_lShardModels.Len();
		const int nShardsV = nT * 6;
		m_lShardModels.SetLen(nShardModels+1);
		CGlassShardModel& ShardModel = m_lShardModels[nShardModels];

		ShardModel.m_iSurface = Model.m_iSurface;
		ShardModel.m_nShards = nT;
		ShardModel.m_SpawnTick = -1;

		// Allocate some new space for the shard pieces
		ShardModel.m_lV.SetLen(nShardsV);
		ShardModel.m_lN.SetLen(nShardsV);
		ShardModel.m_lC.SetLen(nShardsV);
		ShardModel.m_lTV.SetLen(nShardsV);
		ShardModel.m_lTangU.SetLen(nShardsV);
		ShardModel.m_lTangV.SetLen(nShardsV);
		ShardModel.m_lP.SetLen(nT * 24);
		ShardModel.m_lVelocity.SetLen(nT);

		pV		= ShardModel.m_lV.GetBasePtr();
		pC		= ShardModel.m_lC.GetBasePtr();
		pN		= ShardModel.m_lN.GetBasePtr();
		pTangU	= ShardModel.m_lTangU.GetBasePtr();
		pTangV	= ShardModel.m_lTangV.GetBasePtr();
		pTV		= ShardModel.m_lTV.GetBasePtr();
		uint16* pP = ShardModel.m_lP.GetBasePtr();
		CVec3Dfp32* pVelocity = ShardModel.m_lVelocity.GetBasePtr();

		// Reconstruct all shards from tags
		int iV = 0;
		int iT = 0;
		int iP = 0;
		
		for(int i = 0; i < nT; i++)
		{
			CXR_Glass_Delaunay2D::CDelaunay2DTriangle& Triangle = pTriangles[i];
			
			// Reconstruct shard render data
			for(int j = 0; j < 3; j++)
			{
				//const uint16 iPnt = pPoints2D[Triangle.m_iIndex.k[j]];
				const uint16 iPnt2 = Triangle.m_iIndex.k[j];

				// Get point
				const int inV = iV+3;
				CVec2Dfp32& Point = pPoints2D[iPnt2];

				// Set velocity for shard
				pVelocity[iT].k[iComp0] = (Point.k[0] - LP.k[0]) * _ForceScale;// * (_Force.k[iComp0] * 2.5f);
				pVelocity[iT].k[iComp1] = (Point.k[0] - LP.k[0]) * _ForceScale;// * (_Force.k[iComp1] * 2.5f);
				pVelocity[iT].k[3 - (iComp0 + iComp1)] = _ForceScale * MFloat_GetRand(iT) * 2.5f;// * (_Force.k[3] - (iComp0 + iComp1));

				// Vertex creation
				{
					CVec3Dfp32 VertexPosition(0);

					VertexPosition.k[iComp0] = Middle.k[iComp0] + ((Point.k[0] * 0.5f) * Dim.k[0]);
					VertexPosition.k[iComp1] = Middle.k[iComp1] + ((Point.k[1] * 0.5f) * Dim.k[1]);

					const CVec3Dfp32 V2WPos = (VertexPosition - (PlaneN * Distance));
					pV[iV] = V2WPos * _WMat;
					pV[inV] = (V2WPos + (-PlaneN * Thickness)) * _WMat;
				}

				// Normals
				{
					pN[iV] = PlaneN;
					pN[inV] = -PlaneN;
				}

				// Colors
				{
					pC[iV] = CPixel32(255,255,255,255);
					pC[inV] = CPixel32(255,255,255,255);
				}

				// Texture coordinates
				{
					const CVec3Dfp32& V = pV[iV];
					const fp32 UProj = (V.k[0]*UVec.k[0] + V.k[1]*UVec.k[1] + V.k[2]*UVec.k[2])*UVecLenSqrInv + UOffset;
					const fp32 VProj = (V.k[0]*VVec.k[0] + V.k[1]*VVec.k[1] + V.k[2]*VVec.k[2])*VVecLenSqrInv + VOffset;
					if(UProj > TProjMax.k[0]) TProjMax.k[0] = UProj;
					if(VProj > TProjMax.k[1]) TProjMax.k[1] = VProj;
					if(UProj < TProjMin.k[0]) TProjMin.k[0] = UProj;
					if(VProj < TProjMin.k[1]) TProjMin.k[1] = VProj;

					pTV[iV].k[0] = pTV[inV].k[0] = UProj * TxtWidthInv;
					pTV[iV].k[1] = pTV[inV].k[1] = VProj * TxtHeightInv;
				}

				// Tangents
				{
					TangU1.Combine(PlaneN, NTangU1, pTangU[iV]);
					TangV1.Combine(PlaneN, NTangV1, pTangV[iV]);

					TangU2.Combine(-PlaneN, NTangU2, pTangU[inV]);
					TangV2.Combine(-PlaneN, NTangV2, pTangV[inV]);

					pTangU[iV].Normalize();
					pTangV[iV].Normalize();

					pTangU[inV].Normalize();
					pTangV[inV].Normalize();
				}
				
				iV++;
			}

			// Generate shard indices
			iV-=3;
			//const fp32 X = (pPoints2D[Triangle.m_iIndex.k[1]]-pPoints2D[Triangle.m_iIndex.k[0]]) / (pPoints2D[Triangle.m_iIndex.k[2]]-pPoints2D[Triangle.m_iIndex.k[0]]);
			const fp32 Or = PlaneN * ((pV[iV-2]-pV[iV-3])/(pV[iV-1]-pV[iV-3]));
			if(Or >= 0)
			{
				pP[iP++] = iV; pP[iP++] = iV+1; pP[iP++] = iV+2;
				pP[iP++] = iV+3; pP[iP++] = iV+3+1; pP[iP++] = iV+3+2;

				pP[iP++] = iV; pP[iP++] = iV+3+1; pP[iP++] = iV+1;
				pP[iP++] = iV; pP[iP++] = iV+3; pP[iP++] = iV+3+1;

				pP[iP++] = iV; pP[iP++] = iV+2; pP[iP++] = iV+3+2;
				pP[iP++] = iV; pP[iP++] = iV+3+2; pP[iP++] = iV+3;

				pP[iP++] = iV+1; pP[iP++] = iV+3+2; pP[iP++] = iV+2;
				pP[iP++] = iV+1; pP[iP++] = iV+3+1; pP[iP++] = iV+3+2;
			}
			else
			{
				pP[iP++] = iV; pP[iP++] = iV+2; pP[iP++] = iV+1;
				pP[iP++] = iV+3; pP[iP++] = iV+3+2; pP[iP++] = iV+3+1;

				pP[iP++] = iV; pP[iP++] = iV+1; pP[iP++] = iV+3+1;
				pP[iP++] = iV; pP[iP++] = iV+3+1; pP[iP++] = iV+3;

				pP[iP++] = iV; pP[iP++] = iV+3+2; pP[iP++] = iV+2;
				pP[iP++] = iV; pP[iP++] = iV+3; pP[iP++] = iV+3+2;

				pP[iP++] = iV+1; pP[iP++] = iV+2; pP[iP++] = iV+3+2;
				pP[iP++] = iV+1; pP[iP++] = iV+3+2; pP[iP++] = iV+3+1;
			}

			iV += 6;
			iT++;
		}

		{
			#ifdef PLATFORM_PS2
				const CVec2Dfp32 TMid = CVec2Dfp32(TProjMin[0] * TxtWidthInv, TProjMin[1] * TxtHeightInv);
				const int nv = nShardsV;// * 2;
				for(int i = 0; i < nv; i++)
				{
					pTV[i] -= TMid;
					pTV[i].k[0] = MaxMT(pTV[i].k[0], 0);
					pTV[i].k[1] = MaxMT(pTV[i].k[1], 0);
				}
			#else
				const CVec2Dfp32 TMid = CVec2Dfp32(RoundToInt(((TProjMin[0] + TProjMax[0]) * 0.5f * TxtWidthInv ) / 16.0f) * 16.0f,
												RoundToInt(((TProjMin[1] + TProjMax[1]) * 0.5f * TxtHeightInv) / 16.0f) * 16.0f);
				if(TMid.k[0] != 0 || TMid.k[0] != 0)
				{
					const int nv = nShardsV;// * 2;
					for(int i = 0; i < nv; i++)
						pTV[i] -= TMid;
				}
			#endif
		}
	}

	// Disable all collision testing with this model on client also
	Model.m_Attrib.Attrib_SetPhys(false);

	// Allocate some new space for the glass model
	Model.m_lV.Clear();
	Model.m_lC.Clear();
	Model.m_lN.Clear();
	Model.m_lTangU.Clear();
	Model.m_lTangV.Clear();
	Model.m_lTV.Clear();
	Model.m_lFrontP.Clear();
	Model.m_lEdgeP.Clear();

	Model.m_pV		= NULL;
	Model.m_pC		= NULL;
	Model.m_pN		= NULL;
	Model.m_pTangU	= NULL;
	Model.m_pTangV	= NULL;
	Model.m_pTV		= NULL;
	Model.m_pFrontP	= NULL;
	Model.m_pEdgeP	= NULL;
	
	Model.m_nRenderMainPrim = 0;
	Model.m_nRenderEdgePrim = 0;
}
*/























/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Glass_Delaunay2D
|__________________________________________________________________________________________________
\*************************************************************************************************/
/*
bool CXR_Glass_Delaunay2D::CDelaunay2DTriangle::CCEncompasses(const CVec2Dfp32& _Point) const
{
	const CVec2Dfp32 Dist = _Point - m_Center;
	const fp32 DistLen = (Dist[0]*Dist[0] + Dist[1]*Dist[1]);
	return (DistLen <= m_R2);
}

bool CXR_Glass_Delaunay2D::CDelaunay2DTriangle::TriEncompasses(const CVec2Dfp32* _pPoints2D, const CVec2Dfp32& _Point) const
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


fp32 CXR_Glass_Delaunay2D::CDelaunay2DTriangle::IsLeftOfSegment(const CVec2Dfp32& _Point, const CVec2Dfp32& _Point0, const CVec2Dfp32& _Point1) const
{
	const CVec2Dfp32 P0 = _Point - _Point0;
	const CVec2Dfp32 P2 = (_Point1 - _Point0);
	const CVec2Dfp32 P1 = CVec2Dfp32(-P2[1], P2[0]);

	return (P0[0]*P1[0] + P0[1]*P1[1]);
}


void CXR_Glass_Delaunay2D::CDelaunay2DTriangle::SetCircumCircle(const CVec2Dfp32* _pPoints)
{
	const CVec2Dfp32& Point0 = _pPoints[m_iIndex[0]];
	const CVec2Dfp32& Point1 = _pPoints[m_iIndex[1]];
	const CVec2Dfp32& Point2 = _pPoints[m_iIndex[2]];
	fp32 x0 = Point0[0]; const fp32& y0 = Point0[1]; fp32 x1 = Point1[0]; const fp32& y1 = Point1[1];
	const fp32& x2 = Point2[0]; const fp32& y2 = Point2[1]; const fp32 y10 = y1 - y0; const fp32 y21 = y2 - y1;
	const bool b21zero = y21 > -_FP4_EPSILON && y21 < _FP4_EPSILON;
	if(y10 > -_FP4_EPSILON && y10 < _FP4_EPSILON)
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


CXR_Glass_Delaunay2D::CXR_Glass_Delaunay2D(const int32& _Seed)
	: m_Seed(_Seed)
	, m_nBasePoints(0)
{
	m_lPoints2D.Clear();
	m_lTriangles.Clear();
}


CXR_Glass_Delaunay2D::~CXR_Glass_Delaunay2D()
{
	m_nBasePoints = 0;
	m_lPoints2D.Clear();
	m_lTriangles.Clear();
}

int16 CXR_Glass_Delaunay2D::FindEdge(const CDelaunay2DEdge& _Edge, const CDelaunay2DEdge* _pEdges, const uint32& _nEdges)
{
	for(int16 i = 0; i < _nEdges; i++)
	{
		if(_pEdges[i] == _Edge) return i;
	}

	return -1;
}

void CXR_Glass_Delaunay2D::HandleEdge(const CVec3Dint16& _iIndex, const CVec2Dfp32* _pPoints2D, CDelaunay2DEdge* _pEdges, uint32& _nEdges)
{
	static uint8 iComp[4] = { 0, 1, 2, 0 };

	for(int i = 0; i < 3; i++)
	{
		const CVec2Dfp32& Point0 = _pPoints2D[_iIndex.k[iComp[i]]];
		const CVec2Dfp32& Point1 = _pPoints2D[_iIndex.k[iComp[i+1]]];

        CDelaunay2DEdge Edge(_iIndex.k[iComp[i+1]], _iIndex.k[iComp[i]]);
		if((Point0.k[0] < Point1.k[0]) || (Point0.k[0] == Point1.k[0] && Point0.k[1] < Point1.k[1]))
			Edge.Set(_iIndex.k[iComp[i]], _iIndex.k[iComp[i+1]]);

		const int16 iEdge = FindEdge(Edge, _pEdges, _nEdges);

		if(iEdge < 0) _pEdges[_nEdges++] = Edge;
		else if(_nEdges > 0) _pEdges[iEdge] = _pEdges[--_nEdges];
	}
}

void CXR_Glass_Delaunay2D::SetBasePrimitiveRecalc(const CVec3Dfp32* _pVerts, const uint32& _nVerts, const CGlassAttrib& _Attrib, const uint16* _piPrim, const uint32& _nPrim)
{
	if(_nPrim <= 0)
		return;

	m_lPoints2D.SetLen(_nVerts);
	
	CVec2Dfp32* pPoints = m_lPoints2D.GetBasePtr();
	
	// Send vertices into 2D -1 -> 1 space
	const CVec2Dfp32 Half(0.5f);
	const CVec2Dfp32& DimRcp = _Attrib.m_DimRcp;
	const CVec2Dfp32& Min = _Attrib.m_Min;
	const uint8& iComp0 = _Attrib.m_liComp[0];
	const uint8& iComp1 = _Attrib.m_liComp[1];

	for(int i = 0; i < _nVerts; i++)
	{
		const CVec2Dfp32 Vert = CVec2Dfp32(_pVerts[i].k[iComp0], _pVerts[i].k[iComp1]) - Min;
		pPoints[i] = (CVec2Dfp32(Vert.k[0] * DimRcp.k[0], Vert.k[1] * DimRcp.k[1]) - Half) * 2.0f;
	}
    
	// Add base triangles
	m_lTriangles.SetLen(_nPrim);
	CDelaunay2DTriangle* pBaseTris = m_lTriangles.GetBasePtr();
	
	// Edge code
//	m_lBaseEdges.SetLen(_nPrim * 3);
//	CDelaunay2DBaseEdge* pBaseEdges = m_lBaseEdges.GetBasePtr();
//	int nBaseEdges = 0;
//	int nShared = 0;

	for(int i = 0, j = 0; i < _nPrim; i++, j += 3)
	{
		pBaseTris[i] = CDelaunay2DTriangle(pPoints, _piPrim[j], _piPrim[j+1], _piPrim[j+2]);
		
		// Add base edges
//		AddBaseEdge(pBaseEdges, nBaseEdges, nShared, _piPrim[j], _piPrim[j+1], pPoints);
//		AddBaseEdge(pBaseEdges, nBaseEdges, nShared, _piPrim[j+1], _piPrim[j+2], pPoints);
//		AddBaseEdge(pBaseEdges, nBaseEdges, nShared, _piPrim[j+2], _piPrim[j], pPoints);
	}

	// Edge code, remove shared edges and set new length
//	RemoveSharedBaseEdges(pBaseEdges, nBaseEdges, nShared);
//	m_lBaseEdges.SetLen(nBaseEdges);

	// Set number of base points and return
	m_nBasePoints = _nVerts;
}
*/

/*
void CXR_Glass_Delaunay2D::AddBaseEdge(CDelaunay2DBaseEdge* _pEdges, int& _nBaseEdges, int& _nShared, const uint16& _i1, const uint16& _i2, const CVec2Dfp32* _pPoints2D)
{
	// Check if base edge exist already
	const CVec2Dint16 iIndex(_i1, _i2);
	for(int i = 0; i < _nBaseEdges; i++)
	{
		if(_pEdges[i] == iIndex)
		{
			_pEdges[i].m_nShared++;
			return;
		}
	}

	_pEdges[_nBaseEdges] = iIndex;
	_pEdges[_nBaseEdges++] = _pPoints2D;
}

void CXR_Glass_Delaunay2D::RemoveSharedBaseEdges(CDelaunay2DBaseEdge* _pEdges, int& _nBaseEdges, int& _nShared)
{
	// Remove all shared edges
	int i = 0;
	while(_nShared > 0)
	{
		if(_pEdges[i].m_nShared > 1)
		{
			_pEdges[i] = _pEdges[_nBaseEdges-1];
			_nBaseEdges--;
			_nShared--;
			continue;
		}

		i++;
	}
}
*/

/*
void CXR_Glass_Delaunay2D::GeneratePoints(const int& _nPoints)
{
	// Temporary solve
	TThinArray<CVec2Dfp32> lAddPoints;

	lAddPoints.SetLen(_nPoints);
	CVec2Dfp32* pPoints = lAddPoints.GetBasePtr();
	CVec2Dfp32* pPoints2D = m_lPoints2D.GetBasePtr();

	const CDelaunay2DTriangle* pBaseTri = m_lTriangles.GetBasePtr();
	const int nBaseTri = m_lTriangles.Len();

	int iInsert = 0;
	for(int i = 0; i < _nPoints; i++, m_Seed += 2)
	{
		CVec2Dfp32 Point2D = CVec2Dfp32(((MFloat_GetRand(m_Seed) * 2.0f) - 1.0f), ((MFloat_GetRand(m_Seed+1) * 2.0f) - 1.0f));

		// Make sure point is inside one of the triangles set up as base triangle
		for(int j = 0; j < nBaseTri; j++)
		{
			if(pBaseTri[j].TriEncompasses(pPoints2D, Point2D))
			{
				pPoints[iInsert++] = Point2D;
				break;
			}
		}
	}

	if(iInsert == 0)
		return;

	// Temporary, copy points
	int CurLen = m_lPoints2D.Len();
	{
		m_lPoints2D.SetLen(CurLen + iInsert);
		pPoints2D = m_lPoints2D.GetBasePtr();

		for(int i = 0; i < iInsert; i++)
			pPoints2D[CurLen+i] = pPoints[i];
	}

    // Here we should actually add setup sorted indices

	// Sort points
	pPoints = m_lPoints2D.GetBasePtr();
	for(int i = CurLen; i < iInsert+CurLen; i++)
	{
		for(int j = i+1; j < iInsert+CurLen; j++)
		{
			// Swap
			if((pPoints[i].k[0] > pPoints[j].k[0]) || (pPoints[i].k[0] == pPoints[j].k[0] && pPoints[i].k[1] > pPoints[i].k[2]))
			{
				CVec2Dfp32 Point = pPoints[i];
				pPoints[i] = pPoints[j];
				pPoints[j] = Point;
			}
		}
	}
}

void CXR_Glass_Delaunay2D::GeneratePoints(const int& _nPoints, const CVec2Dfp32& _LocalPosition)
{
	// Temporary solve
	TThinArray<CVec2Dfp32> lAddPoints;

	lAddPoints.SetLen(_nPoints);
	CVec2Dfp32* pPoints = lAddPoints.GetBasePtr();
	CVec2Dfp32* pPoints2D = m_lPoints2D.GetBasePtr();

	const CDelaunay2DTriangle* pBaseTri = m_lTriangles.GetBasePtr();
	const int nBaseTri = m_lTriangles.Len();

	fp32 Half = 1.0f / ((fp32)_nPoints / 3.0f);

	int iInsert = 0;
	for(int i = 0; i < _nPoints; i++, m_Seed += 2)
	{
		CVec2Dfp32 Point2D = CVec2Dfp32(_LocalPosition.k[0] + ((MFloat_GetRand(m_Seed) * 2.0f - 1.0f) * MinMT((i * Half),1.0f)),
									  _LocalPosition.k[1] + ((MFloat_GetRand(m_Seed+1) * 2.0f - 1.0f) * MinMT((i * Half),1.0f)));
		//CVec2Dfp32 Point2D = CVec2Dfp32(((, ((MFloat_GetRand(m_Seed+1) * 2.0f) - 1.0f));

		// Make sure point is inside one of the triangles set up as base triangle
		for(int j = 0; j < nBaseTri; j++)
		{
			if(pBaseTri[j].TriEncompasses(pPoints2D, Point2D))
			{
				pPoints[iInsert++] = Point2D;
				break;
			}
		}
	}

	if(iInsert == 0)
		return;

	// Temporary, copy points
	int CurLen = m_lPoints2D.Len();
	{
		m_lPoints2D.SetLen(CurLen + iInsert);
		pPoints2D = m_lPoints2D.GetBasePtr();

		for(int i = 0; i < iInsert; i++)
			pPoints2D[CurLen+i] = pPoints[i];
	}

    // Here we should actually add setup sorted indices

	// Sort points
	pPoints = m_lPoints2D.GetBasePtr();
	for(int i = CurLen; i < iInsert+CurLen; i++)
	{
		for(int j = i+1; j < iInsert+CurLen; j++)
		{
			// Swap
			if((pPoints[i].k[0] > pPoints[j].k[0]) || (pPoints[i].k[0] == pPoints[j].k[0] && pPoints[i].k[1] > pPoints[i].k[2]))
			{
				CVec2Dfp32 Point = pPoints[i];
				pPoints[i] = pPoints[j];
				pPoints[j] = Point;
			}
		}
	}
}

void CXR_Glass_Delaunay2D::SelectTriangles(const CVec2Dfp32& _LocalPosition, const fp32& _Area, TThinArray<uint8>& _lTris, uint16& _nTris, TThinArray<uint16>& _lPnts, uint16& _nPnts)
{
	const CDelaunay2DTriangle* pTris = m_lTriangles.GetBasePtr();
	const CVec2Dfp32* pPoints = m_lPoints2D.GetBasePtr();
	const int nT = m_lTriangles.Len();
	const int nP = m_lPoints2D.Len();
	_nTris = 0;
	_nPnts = 0;

	if(nT <= 0)
		return;

	_lTris.SetLen(nT);
	_lPnts.SetLen(nP);
	
	uint8* _pTris = _lTris.GetBasePtr();
	uint16* _pPnts = _lPnts.GetBasePtr();
	
	FillW(_pPnts, nP, 0);
	FillChar(_pTris, nT, 0);
	
	const CVec2Dfp32 PosMin = _LocalPosition - CVec2Dfp32(_Area);
	const CVec2Dfp32 PosMax = _LocalPosition + CVec2Dfp32(_Area);
	for(int i = 0; i < nT; i++)
	{
		const CVec3Dint16& iTriIndex = pTris[i].m_iIndex;
		const CVec2Dfp32& P0 = pPoints[iTriIndex.k[0]];
		const CVec2Dfp32& P1 = pPoints[iTriIndex.k[1]];
		const CVec2Dfp32& P2 = pPoints[iTriIndex.k[2]];

		// Either tag a triangle as hot or the points as solid
		if(	(P0.k[0] >= PosMin.k[0] && P0.k[0] <= PosMax.k[0] && P0.k[1] >= PosMin.k[1] && P0.k[1] <= PosMax.k[1]) ||
			(P1.k[0] >= PosMin.k[0] && P1.k[0] <= PosMax.k[0] && P1.k[1] >= PosMin.k[1] && P1.k[1] <= PosMax.k[1]) ||
			(P2.k[0] >= PosMin.k[0] && P2.k[0] <= PosMax.k[0] && P2.k[1] >= PosMin.k[1] && P2.k[1] <= PosMax.k[1]) )
		{
			_pTris[i] = 1;
			_nTris++;
		}
		else
			_pPnts[iTriIndex.k[0]] = _pPnts[iTriIndex.k[1]] = _pPnts[iTriIndex.k[2]] = 1;
	}

	// Get number of solid points (vertices), and set their indices into the points list
	for(int16 i = 0; i < nP; i++)
	{
		if(_pPnts[i])
			_pPnts[i] = 1 + _nPnts++;
	}
}

// Experimental
bool CXR_Glass_Delaunay2D::GeneratePoints(const CVec2Dfp32& _LocalPosition, const int& _nPoints, const fp32& _Area)
{
	// Temporary solve
	TThinArray<CVec2Dfp32> lAddPoints;

	lAddPoints.SetLen(_nPoints);
	CVec2Dfp32* pPoints = lAddPoints.GetBasePtr();
	CVec2Dfp32* pPoints2D = m_lPoints2D.GetBasePtr();

	const CVec2Dfp32 MaxPos = CVec2Dfp32(MinMT(_LocalPosition.k[0] + _Area,  1.0f), MinMT(_LocalPosition.k[1] + _Area,  1.0f));
	const CVec2Dfp32 MinPos = CVec2Dfp32(MaxMT(_LocalPosition.k[0] - _Area, -1.0f), MaxMT(_LocalPosition.k[1] - _Area, -1.0f));

	const CDelaunay2DTriangle* pBaseTri = m_lTriangles.GetBasePtr();
	const int nBaseTri = m_lTriangles.Len();

	int iInsert = 0;
	for(int i = 0; i < _nPoints; i++, m_Seed += 2)
	{
		CVec2Dfp32 Point2D = _LocalPosition + ((i > 0) ? ((CVec2Dfp32(MFloat_GetRand(m_Seed) - 0.5f, MFloat_GetRand(m_Seed+1) - 0.5f) * 2.0f) * _Area) : CVec2Dfp32(0));

		Point2D[0] = MinMT(MaxMT(Point2D[0], MinPos[0]), MaxPos[0]);
		Point2D[1] = MinMT(MaxMT(Point2D[1], MinPos[1]), MaxPos[1]);

		// Make sure point is inside one of the triangles set up as base triangle
		for(int j = 0; j < nBaseTri; j++)
		{
			if(pBaseTri[j].TriEncompasses(pPoints2D, Point2D))
			{
				pPoints[iInsert++] = Point2D;
				break;
			}
		}
	}

	if(iInsert == 0)
		return false;

	// Temporary, copy points
	int CurLen = m_lPoints2D.Len();
	{
		m_lPoints2D.SetLen(CurLen + iInsert);
		pPoints2D = m_lPoints2D.GetBasePtr();

		for(int i = 0; i < iInsert; i++)
			pPoints2D[CurLen+i] = pPoints[i];
	}

    // Here we should actually add setup sorted indices

	// Sort points
	pPoints = m_lPoints2D.GetBasePtr();
	for(int i = CurLen; i < iInsert+CurLen; i++)
	{
		for(int j = i+1; j < iInsert+CurLen; j++)
		{
			// Swap
			if((pPoints[i].k[0] > pPoints[j].k[0]) || (pPoints[i].k[0] == pPoints[j].k[0] && pPoints[i].k[1] > pPoints[i].k[2]))
			{
				CVec2Dfp32 Point = pPoints[i];
				pPoints[i] = pPoints[j];
				pPoints[j] = Point;
			}
		}
	}

	return true;
}

void CXR_Glass_Delaunay2D::GenerateSurfacePoints(const int& _nPoints, const CVec2Dfp32& _LocalPosition)
{
	// Temporary solve
	TThinArray<CVec2Dfp32> lAddPoints;

	const int nEdgePoints = _nPoints / 4;
	lAddPoints.SetLen(nEdgePoints * 8);
	CVec2Dfp32* pPoints = lAddPoints.GetBasePtr();
	CVec2Dfp32* pPoints2D = m_lPoints2D.GetBasePtr();

	const CDelaunay2DTriangle* pBaseTri = m_lTriangles.GetBasePtr();
	const int nBaseTri = m_lTriangles.Len();

	fp32 Half = 1.0f / ((fp32)_nPoints / 3.0f);
	int iInsert = 0;

	// Insert points around edges
	for(int i = 0; i < nEdgePoints; i++)
	{
		// Insert random points
		for(int j = 0; j < 4; j++, m_Seed += 2)
		{
			CVec2Dfp32 Point2D = CVec2Dfp32(_LocalPosition.k[0] + ((MFloat_GetRand(m_Seed) * 2.0f - 1.0f) * MinMT((i * Half),1.0f)),
										  _LocalPosition.k[1] + ((MFloat_GetRand(m_Seed+1) * 2.0f - 1.0f) * MinMT((i * Half),1.0f)));
			for(int k = 0; k < nBaseTri; k++)
			{
				if(pBaseTri[k].TriEncompasses(pPoints2D, Point2D))
				{
					pPoints[iInsert++] = Point2D;
					break;
				}
			}
		}

		// Insert edge points
		for(int j = 0; j < 4; j++)
		{
			CVec2Dfp32 Points2DEdge;
			int iComp0 = j % 2;
			int iComp1 = (iComp0 + 1) % 2;
			Points2DEdge.k[iComp0] = MFloat_GetRand(m_Seed++) * 2.0f - 1.0f;
			Points2DEdge.k[iComp1] = (j < 2) ? -1.0f : 1.0f;
			for(int k = 0; k < nBaseTri; k++)
			{
				if(pBaseTri[k].TriEncompasses(pPoints2D, Points2DEdge))
				{
					pPoints[iInsert++] = Points2DEdge;
					break;
				}
			}
		}
	}

	if(iInsert == 0)
		return;

	// Temporary, copy points
	int CurLen = m_lPoints2D.Len();
	{
		m_lPoints2D.SetLen(CurLen + iInsert);
		pPoints2D = m_lPoints2D.GetBasePtr();

		for(int i = 0; i < iInsert; i++)
			pPoints2D[CurLen+i] = pPoints[i];
	}

    // Here we should actually add setup sorted indices

	// Sort points
	pPoints = m_lPoints2D.GetBasePtr();
	for(int i = CurLen; i < iInsert+CurLen; i++)
	{
		for(int j = i+1; j < iInsert+CurLen; j++)
		{
			// Swap
			if((pPoints[i].k[0] > pPoints[j].k[0]) || (pPoints[i].k[0] == pPoints[j].k[0] && pPoints[i].k[1] > pPoints[i].k[2]))
				Swap(pPoints[i], pPoints[j]);
		}
	}
}

void CXR_Glass_Delaunay2D::Triangulate()
{
	TThinArray<CDelaunay2DTriangle>	lWorkTris;
	TThinArray<uint32>				liRemove;
	TThinArray<CDelaunay2DEdge>		lEdges;
	
	uint32* piRemove = NULL;
	CDelaunay2DEdge* pEdges = NULL;
	uint32 nEdges = 0;
	uint32 niRemove = 0;
	uint32 nOutput = 0;
	uint32 OutputLen = 0;

	const CVec2Dfp32* pPoints2D = m_lPoints2D.GetBasePtr();

	const int nPoints = m_lPoints2D.Len();
	int nWorkTris = m_lTriangles.Len();

	// This is quite impossible, set up a triangle if anything ...
	if(nPoints <= 3)
		return;

	// Copy base triangle list into working triangle list
	lWorkTris.Clear();
	lWorkTris.Add(m_lTriangles);
	m_lTriangles.Clear();
		
	CDelaunay2DTriangle* pWorkTris = lWorkTris.GetBasePtr();
	CDelaunay2DTriangle* pOutput = m_lTriangles.GetBasePtr();

	fp32 CWCCW = 0.0f;
	if(nWorkTris > 0)
		CWCCW = (pPoints2D[pWorkTris[0].m_iIndex.k[1]]-pPoints2D[pWorkTris[0].m_iIndex.k[0]])/(pPoints2D[pWorkTris[0].m_iIndex.k[2]]-pPoints2D[pWorkTris[0].m_iIndex.k[0]]);

	// Add all generated or set points after base points
	for(int i = m_nBasePoints; i < nPoints; i++)
	{
		const CVec2Dfp32& Point = pPoints2D[i];
		
		liRemove.SetLen(nWorkTris);
		piRemove = liRemove.GetBasePtr();
		niRemove = 0;
		
		lEdges.SetLen(nWorkTris*3);		// How should we do this?
		nEdges = 0;
		pEdges = lEdges.GetBasePtr();

		for(int j = 0; j < nWorkTris; j++)
		{
			CDelaunay2DTriangle& WorkTri = pWorkTris[j];
			if(WorkTri.IsLeftOf(Point))
			{
				// Expand output if needed
				if(nOutput + 1 > OutputLen)
				{
					m_lTriangles.SetLen(OutputLen + 32);
					pOutput = m_lTriangles.GetBasePtr();
					OutputLen += 32;
				}

				pOutput[nOutput++] = WorkTri;
				piRemove[niRemove++] = j;
			}
			else if(WorkTri.CCEncompasses(Point))
			{
				// Remove hot triangle
				HandleEdge(WorkTri.m_iIndex, pPoints2D, pEdges, nEdges);
				piRemove[niRemove++] = j;
			}
		}

		// Remove triangles, by replacing taged triangle with last one and decreasing triangle count
		if(niRemove >= nWorkTris)
			nWorkTris = 0;
		else
		{
			// Shrink work tri array
			for(int j = 0; j < niRemove; j++)
				pWorkTris[piRemove[niRemove - 1 - j]] = pWorkTris[nWorkTris - 1 - j];
			
			nWorkTris -= niRemove;
		}

		// Set new length
		lWorkTris.SetLen(nWorkTris + nEdges);
		pWorkTris = lWorkTris.GetBasePtr();

		// Add edges with current index to triangle list
		for(int j = 0; j < nEdges; j++)
			pWorkTris[nWorkTris+j] = CDelaunay2DTriangle(pPoints2D, pEdges[j].m_iIndex, i);

		nWorkTris += nEdges;
	}

	// Copy down remaining triangles to output
	m_lTriangles.SetLen(nOutput);
	m_lTriangles.Add(lWorkTris);

	// Clear arrays
	lWorkTris.Clear();
	liRemove.Clear();
	lEdges.Clear();
}


void CXR_Glass_Delaunay2D::Triangulate(const uint32* piPoints2D, const int& _i0, const int& _i1, const int& _i2, const int& _i3)
{
}
*/

