#include "PCH.h"

#include "WBSP3Model.h"
#include "WBSP3Def.h"
#include "../../Classes/Render/MWireContainer.h"

// These defines can be used to find certain bugs but at a performance cost.. Remove for RTM
#if defined(MODEL_BSP3_BOUNDCHECK_STACK)
#ifdef	PLATFORM_PS2
#warning "There are some defines enabled which cause performance drops."
#else
#endif
#endif

#ifdef	MODEL_BSP3_BOUNDCHECK_STACK
int Phys_TraceLine_i_MaxDepth = 0;
int Phys_TraceLine_i_MaxDepth_LastPeak = 0;
int Phys_TraceLine_i2_MaxDepth = 0;
int Phys_TraceLine_i2_MaxDepth_LastPeak = 0;
int CollectPCS_i_MaxDepth = 0;
int CollectPCS_i_MaxDepth_LastPeak = 0;
#endif	// MODEL_BSP3_BOUNDCHECK_STACK


static const CXR_MediumDesc* s_pMediums;
static uint8* s_pFTag;
static uint16* s_piFUntag;
static spCXW_Surface* s_lspSurfaces;




// -------------------------------------------------------------------
// Physics internal functions
// -------------------------------------------------------------------
void CXR_Model_BSP3::__Phys_RenderFace(int _iFace, const CMat4Dfp32& _WMat, CWireContainer* _pWC, int _Col) const
{
	MAUTOSTRIP(CXR_Model_BSP3___Phys_RenderFace, MAUTOSTRIP_VOID);

	const CBSP3_Face* pF = &m_lFaces[_iFace];

	int iLast = m_liVertices[pF->m_iiVertices + pF->m_nVertices - 1];
	for(int i = 0; i < pF->m_nVertices; i++)
	{
		int iVertex = m_liVertices[pF->m_iiVertices + i];
		_pWC->RenderWire(m_lVertices[iLast], m_lVertices[iVertex], _Col);
		iLast	= iVertex;
	}
}

bool CXR_Model_BSP3::__Phys_IntersectSphere_Polygon(
	const CVec3Dfp32* _pV, 
	const uint32* _piV, 
	int _nV,
	const CPlane3Dfp32& _Plane, 
	const CVec3Dfp32& _Pos, 
	fp32 _Radius, 
	CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_IntersectSphere_Polygon, false);

//	CBSP3_Face* pF = &m_lFaces[_iFace];
//	int iPlane = pF->m_iPlane;
//	CPlane3Dfp32* pP = &m_pPlanes[iPlane];
	fp32 d = _Plane.Distance(_Pos);
	if( d > _Radius || d < -_Radius ) return false;
//	if (d < -_Radius) return false;
//	if (d > _Radius) return false;

	CPlane3Dfp32 Edge_Plane(CVec3Dfp32(0, 0, 1), 0);
	uint Edge_bCollided = false;
	fp32 Edge_Nearest = _Radius*2.0f;
	CVec3Dfp32 Edge_Pos;

	uint bAllInside = true;
	uint InsideBit = 1;
	uint InsideMask = 0;
	uint iv1 = _piV[_nV - 1];
	uint iv0 = 0;
	for(int v = 0; v < _nV; v++, iv1 = iv0)
	{
		iv0 = _piV[v];
		CVec3Dfp32 vs, ve, n;
		_Pos.Sub(_pV[iv0], vs);
		_pV[iv1].Sub(_pV[iv0], ve);
		fp32 len_e = ve.Length();
		vs.CrossProd(ve, n);
		fp32 dist_e = n.Length() / len_e;
		
		fp32 Side = n*_Plane.n;
		if (Side > 0.0f)
		{
			fp32 t = (vs*ve) / (ve*ve);
			if (t < 0.0f)
			{
				fp32 l = vs.Length();
				if (l > _Radius) continue;
				if (!_pCollisionInfo) return true;
				if (_pCollisionInfo && (l < Edge_Nearest))
				{
					Edge_Nearest = l;
					Edge_Plane.CreateNV(vs.Normalize(), _pV[iv0]);
					Edge_Pos = _pV[iv0];
				}
				Edge_bCollided = true;
			}
			else if (t > 1.0f)
			{
				CVec3Dfp32 vs2 = _Pos-_pV[iv1];
				fp32 l = vs2.Length();
				if (l > _Radius) continue;
				if (!_pCollisionInfo) return true;
				if (_pCollisionInfo && (l < Edge_Nearest))
				{
					Edge_Nearest = l;
					Edge_Plane.CreateNV(vs2.Normalize(), _pV[iv1]);
					Edge_Pos = _pV[iv1];
				}
				Edge_bCollided = true;
			}
			else 
			{
				fp32 l = M_Fabs(dist_e);
				if (l > _Radius) return false;
				if (!_pCollisionInfo) return true;
				if (_pCollisionInfo && (l < Edge_Nearest))
				{
					Edge_Nearest = l;

					CVec3Dfp32 vsproj;
					vs.Project(ve, vsproj);
					Edge_Plane.CreateNV((vs - vsproj).Normalize(), _pV[iv0]);
					Edge_Pos = _pV[iv0] + ve*t;
				}
				Edge_bCollided = true;
			}
			bAllInside = false;
		}
		else
			InsideMask |= InsideBit;

		InsideBit <<= 1;
	}

	if (!_pCollisionInfo)
	{
		return Edge_bCollided || bAllInside;
	}

	if (bAllInside)
	{
		_pCollisionInfo->m_bIsValid = true;
		_pCollisionInfo->m_bIsCollision = true;
		_pCollisionInfo->m_Plane = _Plane;
		_pCollisionInfo->m_Distance = M_Fabs(d) - _Radius;
		_pCollisionInfo->m_LocalPos = _Pos - _pCollisionInfo->m_Plane.n*d;
		_pCollisionInfo->m_Velocity = 0;
	}
	else
	{
		if (!Edge_bCollided) return false;
		_pCollisionInfo->m_bIsValid = true;
		_pCollisionInfo->m_bIsCollision = true;
		_pCollisionInfo->m_Plane = Edge_Plane;
		_pCollisionInfo->m_Distance = Edge_Nearest - _Radius;
		_pCollisionInfo->m_LocalPos = Edge_Pos;
		_pCollisionInfo->m_Velocity = 0;
	}
	return true;
}


bool CXR_Model_BSP3::__Phys_IntersectSphere_CheckFace(int _iFace, const CVec3Dfp32& _Pos, fp32 _Radius, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_IntersectSphere_CheckFace, false);
	CBSP3_Face* pF = &m_lFaces[_iFace];

	int iPlane = pF->m_iPlane;
	CPlane3Dfp32* pP = &m_pPlanes[iPlane];
	fp32 d = pP->Distance(_Pos);

//ConOut(CStrF("(1) iF %d", _iFace));
	if (d < -_Radius) return false;
	if (d > _Radius) return false;

//	fp32 RSqr = Sqr(_Radius);

	CPlane3Dfp32 Edge_Plane(CVec3Dfp32(0, 0, 1), 0);
	uint Edge_bCollided = false;
	fp32 Edge_Nearest = _Radius*2.0f;
	CVec3Dfp32 Edge_Pos;

	uint nv = pF->m_nPhysV;
	uint iiv = pF->m_iiPhysV;

	uint InsideBit = 1;
	uint bAllInside = true;
	uint InsideMask = 0;
	uint iv1 = m_liVertices[iiv + nv - 1];
	uint iv0 = 0;
//ConOut(CStrF("(2) iF %d, nv %d", _iFace, nv));
	for(int v = 0; v < nv; v++, iv1 = iv0)
	{
		iv0 = m_liVertices[iiv + v];
		CVec3Dfp32 vs = _Pos - m_lVertices[iv0];
		CVec3Dfp32 ve = m_lVertices[iv1] - m_lVertices[iv0];
		fp32 len_e = ve.Length();
		CVec3Dfp32 n = (vs/ve);
		fp32 dist_e = n.Length() / len_e;
		
		fp32 Side = n*pP->n;
		if (Side > 0.0f)
		{
			fp32 t = (vs*ve) / (ve*ve);
			if (t < 0.0f)
			{
				fp32 l = vs.Length();
				if (l > _Radius) continue;
				if (!_pCollisionInfo) return true;
				if (_pCollisionInfo && (l < Edge_Nearest))
				{
					Edge_Nearest = l;
					Edge_Plane.CreateNV(vs.Normalize(), m_lVertices[iv0]);
					Edge_Pos = m_lVertices[iv0];
				}
				Edge_bCollided = true;
			}
			else if (t > 1.0f)
			{
				CVec3Dfp32 vs2 = _Pos-m_lVertices[iv1];
				fp32 l = vs2.Length();
				if (l > _Radius) continue;
				if (!_pCollisionInfo) return true;
				if (_pCollisionInfo && (l < Edge_Nearest))
				{
					Edge_Nearest = l;
					Edge_Plane.CreateNV(vs2.Normalize(), m_lVertices[iv1]);
					Edge_Pos = m_lVertices[iv1];
				}
				Edge_bCollided = true;
			}
			else 
			{
				fp32 l = M_Fabs(dist_e);
				if (l > _Radius) return false;
				if (!_pCollisionInfo) return true;
				if (_pCollisionInfo && (l < Edge_Nearest))
				{
					Edge_Nearest = l;

					CVec3Dfp32 vsproj;
					vs.Project(ve, vsproj);
					Edge_Plane.CreateNV((vs - vsproj).Normalize(), m_lVertices[iv0]);
					Edge_Pos = m_lVertices[iv0] + ve*t;
				}
				Edge_bCollided = true;
			}
			bAllInside = false;
		}
		else
			InsideMask |= InsideBit;

		InsideBit <<= 1;

//ConOut(CStrF("iF %d, e %d, nSide %f, %f", _iFace, v, Side, dist_e));
//LogFile(CStrF("iF %d, e %d, nSide %f, %f", _iFace, v, Side, dist_e));

	}

	if (!_pCollisionInfo)
	{
		return Edge_bCollided || bAllInside;
	}

//	LogFile(CStrF("Mask %d, %d", InsideMask, (1 << nv) -1));
	if (bAllInside)
	{
//ConOut(CStrF("Plane %f", 
		_pCollisionInfo->m_bIsValid = true;
		_pCollisionInfo->m_bIsCollision = true;
		_pCollisionInfo->m_Plane = *pP;
		_pCollisionInfo->m_Distance = M_Fabs(d) - _Radius;
		_pCollisionInfo->m_LocalPos = _Pos - _pCollisionInfo->m_Plane.n*d;
		_pCollisionInfo->m_Velocity = 0;
		_pCollisionInfo->m_pSurface = m_lspSurfaces[pF->m_iSurface];
//LogFile(CStrF("Setting plane. %d, %f, ", _iFace, _pCollisionInfo->m_Distance) + _pCollisionInfo->m_Plane.n.GetString());
	}
	else
	{
		if (!Edge_bCollided) return false;
//			Error("__Phys_IntersectSphere_CheckFace", "Internal error.");
		_pCollisionInfo->m_bIsValid = true;
		_pCollisionInfo->m_bIsCollision = true;
		_pCollisionInfo->m_Plane = Edge_Plane;
		_pCollisionInfo->m_Distance = Edge_Nearest - _Radius;
		_pCollisionInfo->m_LocalPos = Edge_Pos;
		_pCollisionInfo->m_Velocity = 0;
		_pCollisionInfo->m_pSurface = m_lspSurfaces[pF->m_iSurface];
		
//LogFile(CStrF("Setting edge. %d, %f, ", _iFace, _pCollisionInfo->m_Distance) + _pCollisionInfo->m_Plane.n.GetString());
//		_pCollisionInfo->m_Plane.p = 0;
//		_pCollisionInfo->m_Plane.n = CVec3Dfp32(1,0,0);
	}
	return true;
}

bool CXR_Model_BSP3::__Phys_IntersectSphere_r(int _iNode, const CVec3Dfp32& _Pos, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_IntersectSphere_r, false);
	if (!_iNode) 
	{
//ConOut(CStr("Solid node!"));
//		if (_pCollisionInfo) _pCollisionInfo->m_bIsValid = false;
		return true;
	}

	CBSP3_Node* pN = &m_pNodes[_iNode];
	CCollisionInfo CInfo;
	bool bIntersect = false;

	if (pN->IsLeaf())
	{
		CXR_MediumDesc* pMediums = m_lMediums.GetBasePtr();
		if (pMediums[pN->m_iMedium].m_MediumFlags & _MediumFlags)
			if (!_pCollisionInfo) return true;

		// Leaf
		uint nFaces = pN->m_nFaces;
		uint iiFaces = pN->m_iiFaces;
//ConOut(CStrF("iN %d, nF %d", _iNode, nFaces));
		for(uint i = 0; i < nFaces; i++)
		{
			uint iFace = m_liFaces[iiFaces + i];

			// Check medium...
			CBSP3_Face* pF = &m_lFaces[iFace];
			if (!(pF->m_Flags & XW_FACE_PHYSICAL)) continue;

			if (pF->m_Flags & XW_FACE_VOLUMEINSIDE)
			{
//				LogFile(CStrF("InsideOfMedium %.4x vs %.4x", pF->m_InsideOfMedium, _MediumFlags));
//				if ((pF->m_InsideOfMedium & _MediumFlags)) continue;
				continue;
			}
			else
			{
//				LogFile(CStrF("BackMedium %.4x vs %.4x", pF->m_BackMedium, _MediumFlags));
				if (!(pMediums[pF->m_iBackMedium].m_MediumFlags & _MediumFlags)) continue;
			}

			// Do intersection...
			if (__Phys_IntersectSphere_CheckFace(iFace, _Pos, _Radius, &CInfo))
			{
				bIntersect = true;
				if (!_pCollisionInfo) return true;

				if (_pCollisionInfo->Improve(CInfo))
					if (_pCollisionInfo->IsComplete())
						return true;
			}
		}

		if (pMediums[pN->m_iMedium].m_MediumFlags & _MediumFlags) return true;
		return bIntersect;
	}
	else
	{
		// Node, recurse...
		uint iPlane = m_pNodes[_iNode].m_iPlane;
		if (!iPlane) Error("__Phys_IntersectSphere_r", "Internal error.");
		CPlane3Dfp32* pP = &m_pPlanes[iPlane];
		fp32 d = pP->Distance(_Pos);

		bool bIntersect1 = false;
		bool bIntersect2 = false;
		if (d > -_Radius) 
			if ((bIntersect1 = __Phys_IntersectSphere_r(m_pNodes[_iNode].m_iNodeFront, _Pos, _Radius, _MediumFlags, _pCollisionInfo)))
				if (!_pCollisionInfo) return true;

		if (d < _Radius) 
			if ((bIntersect2 = __Phys_IntersectSphere_r(m_pNodes[_iNode].m_iNodeBack, _Pos, _Radius, _MediumFlags, _pCollisionInfo)))
				if (!_pCollisionInfo) return true;

		if (!_pCollisionInfo) return false;
		return (bIntersect1 || bIntersect2);
	}
}
/*
static bool SphereInBoundBox(const CVec3Dfp32& _v, fp32 _Radius, const CVec3Dfp32& _BoxMin, const CVec3Dfp32& _BoxMax);
static bool SphereInBoundBox(const CVec3Dfp32& _v, fp32 _Radius, const CVec3Dfp32& _BoxMin, const CVec3Dfp32& _BoxMax)
{
	MAUTOSTRIP(SphereInBoundBox, false);
	for(int i = 0; i < 3; i++)
	{
		if(_v.k[i] + _Radius < _BoxMin.k[i]) return false;
		if(_v.k[i] - _Radius > _BoxMax.k[i]) return false;
	}
	return true;
}
*/
bool CXR_Model_BSP3::__Phys_IntersectSphere_Triangle(const CVec3Dfp32& _V0, const CVec3Dfp32& _V1, const CVec3Dfp32& _V2, const CVec3Dfp32& _Pos, fp32 _Radius, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_IntersectSphere_Triangle, false);
	CVec3Dfp32 V[3];
	CVec3Dfp32 e0, e1, N;
	_V1.Sub(_V0, e0);
	_V2.Sub(_V0, e1);
	e0.CrossProd(e1, N);
	N.Normalize();
	CPlane3Dfp32 Plane(N, _V0);
	V[0] = _V0;
	V[1] = _V2;
	V[2] = _V1;
	return __Phys_IntersectSphere_Polygon(V, g_IndexRamp32, 3, Plane, _Pos, _Radius, _pCollisionInfo);
}

bool CXR_Model_BSP3::__Phys_IntersectSphere_SB(const CVec3Dfp32& _Pos, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_IntersectSphere_SB, false);
	return false;
}

// -------------------------------------------------------------------
// Physics interface overrides
// -------------------------------------------------------------------
void CXR_Model_BSP3::Phys_GetBound_Sphere(const CMat4Dfp32& _Pos, CVec3Dfp32& _v0, fp32& _Radius)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_GetBound_Sphere, MAUTOSTRIP_VOID);
	_Radius = GetBound_Sphere();
	_v0 = CVec3Dfp32::GetMatrixRow(_Pos, 3);
}

void CXR_Model_BSP3::Phys_GetBound_Box(const CMat4Dfp32& _Pos, CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_GetBound_Box, MAUTOSTRIP_VOID);
	/*
	NOTE: 
		This function in only optimal when the box return by GetBound_Box is symetrical.
	*/
	GetBound_Box(_Box);

	CVec3Dfp32 E;
	CVec3Dfp32 E2;
	CVec3Dfp32 C;
	_Box.m_Max.Lerp(_Box.m_Min, 0.5f, C);
	_Box.m_Max.Sub(C, E);

	E2[0] = M_Fabs(E[0] * _Pos.k[0][0]) + M_Fabs(E[1] * _Pos.k[1][0]) + M_Fabs(E[2] * _Pos.k[2][0]);
	E2[1] = M_Fabs(E[0] * _Pos.k[0][1]) + M_Fabs(E[1] * _Pos.k[1][1]) + M_Fabs(E[2] * _Pos.k[2][1]);
	E2[2] = M_Fabs(E[0] * _Pos.k[0][2]) + M_Fabs(E[1] * _Pos.k[1][2]) + M_Fabs(E[2] * _Pos.k[2][2]);

	C *= _Pos;
	C.Add(E2, _Box.m_Max);
	C.Sub(E2, _Box.m_Min);
}

void CXR_Model_BSP3::Phys_Init(CXR_PhysicsContext* _pPhysContext)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_Init, MAUTOSTRIP_VOID);
}

bool CXR_Model_BSP3::__Phys_TraceLine_i(CXR_PhysicsContext* _pPhysContext, int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MediumFlags, CPhysLineContext* _pLineContext, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_TraceLine_i, false);
	MSCOPESHORT(CXR_Model_BSP3::__Phys_TraceLine_i);
	// Note: _pCollisionInfo must be valid here.

	if (!_iNode) return false;

	const CXR_MediumDesc* pMediums = m_lMediums.GetBasePtr();

	bool bHit = false;
	fp32 InvV1V0 = 1.0f * _p1.DistanceInv(_p0);

	struct JobItem
	{
		uint32 m_iNode;
		CVec3Dfp32 m_p0, m_p1;
	};

#ifdef	PLATFORM_PS2
	JobItem* aWorkingSet = (JobItem*)ScratchPad_Alloc( sizeof(JobItem) * 128 );
#else
	JobItem aWorkingSet[128];
#endif

	int WorkingStack = 0;
	
	aWorkingSet[WorkingStack].m_p0 = _p0;
	aWorkingSet[WorkingStack].m_p1 = _p1;
	aWorkingSet[WorkingStack].m_iNode = _iNode;
	WorkingStack++;

StartOf_Phys_TraceLine_i:
#ifdef	MODEL_BSP3_BOUNDCHECK_STACK
	Phys_TraceLine_i_MaxDepth	= Max( Phys_TraceLine_i_MaxDepth, WorkingStack );
#endif	// MODEL_BSP3_BOUNDCHECK_STACK
	
	--WorkingStack;
	uint32 WorkingNode	= aWorkingSet[WorkingStack].m_iNode;
	CVec3Dfp32 p0 = aWorkingSet[WorkingStack].m_p0;
	CVec3Dfp32 p1 = aWorkingSet[WorkingStack].m_p1;

StartOf_Phys_TraceLine_i_NoAdd:
	const CBSP3_Node* pN = &m_pNodes[WorkingNode];

	if (pN->IsLeaf())
	{
		int nf = pN->m_nFaces;
		int iif = pN->m_iiFaces;
		for(int f = 0; f < nf; f++)
		{
			int iFace = m_piFaces[iif + f];
			const CBSP3_Face* pF = &m_pFaces[iFace];
			int iMedium = pF->m_iBackMedium;
			if (!(pMediums[iMedium].m_MediumFlags & _MediumFlags)) continue;


			const CPlane3Dfp32& P = m_pPlanes[pF->m_iPlane];
//			fp32 d1 = P.Distance(p0);
//			fp32 d2 = P.Distance(p1);
//			if (d1 > 1.1f && d2 > 1.1f) continue;
//			if (d1 < -0.1f && d2 < -0.1f) continue;
//			if (d1 < 0.2f) continue;
//			fp32 d2 = P.Distance(p1);
//			if (d2 > 0.1f) continue;	// If positive, the points must be on the same side.
			fp32 d1 = P.Distance(p0);
			if (d1 < 0.001f) continue;
			fp32 d2 = P.Distance(p1);
			if (d2 > -0.001f) continue;	// If positive, the points must be on the same side.

			int nv = pF->m_nPhysV;
			int iiv = pF->m_iiPhysV;
			int iv0 = m_piVertices[iiv + nv - 1];

			int v;
			for(v = 0; v < nv; v++)
			{
				int iv1 = m_piVertices[iiv + v];
				CVec3Dfp32 Edge, p0v1, Normal;
				m_pVertices[iv1].Sub(m_pVertices[iv0], Edge);
				m_pVertices[iv1].Sub(p0, p0v1);
				Edge.CrossProd(p0v1, Normal);
				CPlane3Dfp32 Plane(Normal, p0);
				if (Plane.Distance(p1) > 0.001f) break;

				iv0 = iv1;
			}

			if (v == nv)
			{
				bHit = true;
				CVec3Dfp32 HitPos;
				fp32 t = -d1/(d2-d1);
				p0.Lerp(p1, t, HitPos);
				fp32 Time = _p0.Distance(HitPos) * InvV1V0;

				if (_pCollisionInfo->IsImprovement(Time))
				{
					_pCollisionInfo->m_bIsCollision = true;
					_pCollisionInfo->m_bIsValid = true;
					_pCollisionInfo->m_LocalPos = HitPos;
					_pCollisionInfo->m_Time = Time;
					_pCollisionInfo->m_Distance = P.Distance(_p1);
					_pCollisionInfo->m_Plane = P;
					_pCollisionInfo->m_pSurface = m_lspSurfaces[pF->m_iSurface];

					if (Time > 1.1f)
						ConOut(CStrF("(__Phys_TraceLine_r) Fuckat T %f", t));

					if (_pCollisionInfo->IsComplete())
					{
#ifdef	PLATFORM_PS2
						ScratchPad_Free( sizeof( JobItem ) * 128 );
#endif
						return true;
					}
				}
			}
		}
		
		if( bHit )
		{
#ifdef	PLATFORM_PS2
			ScratchPad_Free( sizeof( JobItem ) * 128 );
#endif
			return true;
		}
	}
	else if (pN->IsNode())
	{
		const CPlane3Dfp32* pP = &m_pPlanes[pN->m_iPlane];
		fp32 d1 = pP->Distance(p0);
		fp32 d2 = pP->Distance(p1);

		// Mask1
		int Side1, Side2;
		if (d1 < -0.01f)
			Side1 = 2;
		else if (d1 > 0.01f)
			Side1 = 1;
		else
			Side1 = 4;

		// Mask2
		if (d2 < -0.01f)
			Side2 = 2;
		else if (d2 > 0.01f)
			Side2 = 1;
		else
			Side2 = 4;

		int iFront = pN->m_iNodeFront;
		int iBack = pN->m_iNodeBack;
		
		int Side1or2 = Side1 | Side2;

		if ((Side1or2) == 4)
		{
			if( iBack )
			{
				aWorkingSet[WorkingStack].m_p0	= p0;
				aWorkingSet[WorkingStack].m_p1	= p1;
				aWorkingSet[WorkingStack].m_iNode	= iBack;
				WorkingStack++;
			}
			if( iFront )
			{
				// Cheap iteration.. same positions, just new node
				WorkingNode = iFront;
				goto StartOf_Phys_TraceLine_i_NoAdd;
			}
		}
		else if (((Side1or2) & 3) == 3)
		{
			if (!(iFront || iBack))
				goto Continue;

			fp32 s = d2 - d1;
			if (s == 0.0f)
				goto Continue;

			fp32 vscale = -d1/s;

			CVec3Dfp32 v, CutPoint0, CutPoint1;
			p1.Sub(p0, v);
			p0.Combine(v, vscale+0.01f, CutPoint0);	// Make sure the new lines still intersects the plane
			p0.Combine(v, vscale-0.01f, CutPoint1);	// since the lines must still be intersected to the actual polygons.

			if (Side1 & 2)
			{
				if( iFront )
				{
					aWorkingSet[WorkingStack].m_p0		= CutPoint1;
					aWorkingSet[WorkingStack].m_p1		= p1;
					aWorkingSet[WorkingStack].m_iNode	= iFront;
					WorkingStack++;
				}

				if( iBack )
				{
					aWorkingSet[WorkingStack].m_p0		= p0;
					aWorkingSet[WorkingStack].m_p1		= CutPoint0;
					aWorkingSet[WorkingStack].m_iNode	= iBack;
					WorkingStack++;
				}
			}
			else
			{
				if( iBack )
				{
					aWorkingSet[WorkingStack].m_p0	= CutPoint1;
					aWorkingSet[WorkingStack].m_p1	= p1;
					aWorkingSet[WorkingStack].m_iNode	= iBack;
					WorkingStack++;
				}

				if( iFront )
				{
					aWorkingSet[WorkingStack].m_p0	= p0;
					aWorkingSet[WorkingStack].m_p1	= CutPoint0;
					aWorkingSet[WorkingStack].m_iNode	= iFront;
					WorkingStack++;
				}
			}
		}
		else
			if ((Side1 & 1) || (Side2 & 1))
			{
				if( iFront )
				{
					// Cheap iteration.. same positions, just new node
					WorkingNode = iFront;
					goto StartOf_Phys_TraceLine_i_NoAdd;
				}
			}
			else
			{
				if( iBack )
				{
					// Cheap iteration.. same positions, just new node
					WorkingNode = iBack;
					goto StartOf_Phys_TraceLine_i_NoAdd;
				}
			}

	}

Continue:
	
	if( WorkingStack > 0 )
		goto StartOf_Phys_TraceLine_i;
	
#ifdef	PLATFORM_PS2
	ScratchPad_Free( sizeof( JobItem ) * 128 );
#endif
	return bHit;
}

bool CXR_Model_BSP3::__Phys_TraceLine_i(CXR_PhysicsContext* _pPhysContext, int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MediumFlags, CPhysLineContext* _pLineContext)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_TraceLine_i_2, false);

	struct JobItem
	{
		uint32 m_iNode;
		CVec3Dfp32 m_p0, m_p1;
	};

#ifdef	PLATFORM_PS2
	JobItem* aWorkingSet = (JobItem*)ScratchPad_Alloc( sizeof(JobItem) * 128 );
#else
	JobItem aWorkingSet[128];
#endif

	int WorkingStack = 0;
	
	aWorkingSet[WorkingStack].m_iNode = _iNode;
	aWorkingSet[WorkingStack].m_p0 = _p0;
	aWorkingSet[WorkingStack].m_p1 = _p1;
	WorkingStack++;
	
	const CXR_MediumDesc* pMediums = m_lMediums.GetBasePtr();

StartOf_Phys_TraceLine_i2:
#ifdef	MODEL_BSP3_BOUNDCHECK_STACK
	Phys_TraceLine_i2_MaxDepth	= Max( Phys_TraceLine_i2_MaxDepth, WorkingStack );
#endif	// MODEL_BSP3_BOUNDCHECK_STACK

	--WorkingStack;

	uint32 WorkingNode = aWorkingSet[WorkingStack].m_iNode;
	CVec3Dfp32 p0 = aWorkingSet[WorkingStack].m_p0;
	CVec3Dfp32 p1 = aWorkingSet[WorkingStack].m_p1;

StartOf_Phys_TraceLine_i2_NoAdd:
	if( WorkingNode == 0 )
	{
#ifdef	PLATFORM_PS2
		ScratchPad_Free( sizeof( JobItem ) * 128 );
#endif
		return true;
	}

	const CBSP3_Node* pN = &m_pNodes[WorkingNode];

	if (pN->IsLeaf())
	{
		if( (pMediums[pN->m_iMedium].m_MediumFlags & _MediumFlags) != 0 )
		{
#ifdef	PLATFORM_PS2
			ScratchPad_Free( sizeof( JobItem ) * 128 );
#endif
			return true;
		}
	}
	else if (pN->IsNode())
	{
		const CPlane3Dfp32* pP = &m_pPlanes[pN->m_iPlane];
		fp32 d1 = pP->Distance(p0);
		fp32 d2 = pP->Distance(p1);

		// Mask1
		int Side1, Side2;
		if (d1 < -0.01f)
			Side1 = 2;
		else if (d1 > 0.01f)
			Side1 = 1;
		else
			Side1 = 4;

		// Mask2
		if (d2 < -0.01f)
			Side2 = 2;
		else if (d2 > 0.01f)
			Side2 = 1;
		else
			Side2 = 4;

		int Side1or2 = Side1 | Side2;
		uint32 iFront = pN->m_iNodeFront;
		uint32 iBack = pN->m_iNodeBack;
		if (Side1or2 == 4)
		{
			{
				aWorkingSet[WorkingStack].m_iNode	= iBack;
				aWorkingSet[WorkingStack].m_p0		= p0;
				aWorkingSet[WorkingStack].m_p1		= p1;
				WorkingStack++;
			}

			// Cheapo recursion.. using same points but diffrent node
			WorkingNode = iFront;
			goto StartOf_Phys_TraceLine_i2_NoAdd;
		}
		else if((Side1or2 & 3) == 1 )
		{
			// Cheapo recursion.. using same points but diffrent node
			WorkingNode = iFront;
			goto StartOf_Phys_TraceLine_i2_NoAdd;
		}
		else if((Side1or2 & 3) == 2 )
		{
			// Cheapo recursion.. using same points but diffrent node
			WorkingNode = iBack;
			goto StartOf_Phys_TraceLine_i2_NoAdd;
		}
		else
		{
			fp32 s = d2 - d1;
			if (s != 0.0f)
			{
				CVec3Dfp32 CutPoint;
				p0.Lerp(p1, -d1/s, CutPoint);

				if (Side1 & 1)
				{
					{
						aWorkingSet[WorkingStack].m_p0		= CutPoint;
						aWorkingSet[WorkingStack].m_p1		= p1;
						aWorkingSet[WorkingStack].m_iNode	= iBack;
						WorkingStack++;
					}
					{
						aWorkingSet[WorkingStack].m_p0		= p0;
						aWorkingSet[WorkingStack].m_p1		= CutPoint;
						aWorkingSet[WorkingStack].m_iNode	= iFront;
						WorkingStack++;
					}
				}
				else
				{
					{
						aWorkingSet[WorkingStack].m_p0		= CutPoint;
						aWorkingSet[WorkingStack].m_p1		= p1;
						aWorkingSet[WorkingStack].m_iNode	= iFront;
						WorkingStack++;
					}
					{
						aWorkingSet[WorkingStack].m_p0		= p0;
						aWorkingSet[WorkingStack].m_p1		= CutPoint;
						aWorkingSet[WorkingStack].m_iNode	= iBack;
						WorkingStack++;
					}
				}
			}
		}
	}

	if( WorkingStack > 0 )
		goto StartOf_Phys_TraceLine_i2;

#ifdef	PLATFORM_PS2
	ScratchPad_Free( sizeof( JobItem ) * 128 );
#endif
	return false;
}

/*
bool CXR_Model_BSP3::__Phys_TraceLine_r(int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MediumFlags)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_TraceLine_r_2, false);

	if (!_iNode)
		return true;
	const CBSP3_Node* pN = &m_pNodes[_iNode];

	if (pN->IsLeaf())
	{
//		if ((m_lMediums[pN->m_iMedium].m_MediumFlags & _MediumFlags) != 0)
//			ConOut(CStrF("HitMedium %.8x at leaf %d", m_lMediums[pN->m_iMedium].m_MediumFlags, _iNode));

		return (m_lMediums[pN->m_iMedium].m_MediumFlags & _MediumFlags) != 0;
	}
	else if (pN->IsNode())
	{
		const CPlane3Dfp32* pP = &m_pPlanes[pN->m_iPlane];
		fp32 d1 = pP->Distance(_p0);
		fp32 d2 = pP->Distance(_p1);

		// Mask1
		int Side1, Side2;
		if (d1 < -0.01f)
			Side1 = 2;
		else if (d1 > 0.01f)
			Side1 = 1;
		else
			Side1 = 4;

		// Mask2
		if (d2 < -0.01f)
			Side2 = 2;
		else if (d2 > 0.01f)
			Side2 = 1;
		else
			Side2 = 4;

		int Side = Side1 | Side2;
		if (Side == 4)
			return 
				__Phys_TraceLine_r(pN->m_iNodeFront, _p0, _p1, _MediumFlags) ||
				__Phys_TraceLine_r(pN->m_iNodeBack, _p0, _p1, _MediumFlags);

		if (Side == 1) return __Phys_TraceLine_r(pN->m_iNodeFront, _p0, _p1, _MediumFlags);
		if (Side == 2) return __Phys_TraceLine_r(pN->m_iNodeBack, _p0, _p1, _MediumFlags);

		fp32 s = d2 - d1;
		if (s == 0.0f) return FALSE;	// Impossible.
		CVec3Dfp32 CutPoint0;
		_p0.Lerp(_p1, -d1/s, CutPoint0);

		if (Side1 & 1)
		{
			return 
				__Phys_TraceLine_r(pN->m_iNodeFront, _p0, CutPoint0, _MediumFlags) ||
				__Phys_TraceLine_r(pN->m_iNodeBack, CutPoint0, _p1, _MediumFlags);
		}
		else
		{
			return 
				__Phys_TraceLine_r(pN->m_iNodeBack, _p0, CutPoint0, _MediumFlags) ||
				__Phys_TraceLine_r(pN->m_iNodeFront, CutPoint0, _p1, _MediumFlags);
		}
	}

	return false;
}
*/
bool CXR_Model_BSP3::Phys_IntersectLine(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_IntersectLine, false);
	MSCOPESHORT(CXR_Model_BSP3::Phys_IntersectLine);

	CVec3Dfp32 v0, v1;
	_v0.MultiplyMatrix(_pPhysContext->m_WMatInv, v0);
	_v1.MultiplyMatrix(_pPhysContext->m_WMatInv, v1);

	CPhysLineContext LineContext;
	LineContext.m_V0	= v0;
	LineContext.m_V1	= v1;
	LineContext.m_InvV1V0 = 1.0f * v1.DistanceInv(v0);

	if (_pCollisionInfo && (_pCollisionInfo->m_ReturnValues & CXR_COLLISIONRETURNVALUE_TIME))
	{
		__Phys_TraceLine_i(_pPhysContext, 1, v0, v1, _MediumFlags, &LineContext, _pCollisionInfo);
		if (_pCollisionInfo->IsComplete())
			return true;

#ifdef	MODEL_BSP3_BOUNDCHECK_STACK
		if( Phys_TraceLine_i_MaxDepth > Phys_TraceLine_i_MaxDepth_LastPeak )
		{
			Phys_TraceLine_i_MaxDepth_LastPeak	= Phys_TraceLine_i_MaxDepth;
#ifdef	PLATFORM_PS2
			scePrintf( "Phys_TraceLine_i::New PeakStack %d\n", Phys_TraceLine_i_MaxDepth_LastPeak );
#else
			ConOutL( CStrF( "Phys_TraceLine_i::New PeakStack %d\n", Phys_TraceLine_i_MaxDepth_LastPeak ).Str() );
#endif
		}
#endif	// MODEL_BSP3_BOUNDCHECK_STACK

		if (_pCollisionInfo->m_bIsValid)
		{
			_pCollisionInfo->m_Plane.Transform(_pPhysContext->m_WMat);
			_pCollisionInfo->m_Pos = _pCollisionInfo->m_LocalPos;
			_pCollisionInfo->m_Distance = _pCollisionInfo->m_Plane.Distance(_v1);	// Can this be removed?
			_pCollisionInfo->m_Time = v0.Distance(_pCollisionInfo->m_Pos) / v0.Distance(v1); // Can this be removed?
			_pCollisionInfo->m_Pos *= _pPhysContext->m_WMat;

//	ConOut(CStrF("HitBSP: Distance %f, Pos ", _pCollisionInfo->m_Distance) + _pCollisionInfo->m_Pos.GetString());
//			if (_pCollisionInfo->m_Time > 1.1f) ConOut(CStrF("(Phys_IntersectLine) Fuckat T %f", _pCollisionInfo->m_Time));
			return true;
		}
		return false;
	}
	else
	{
		bool bColl;

		{
			MSCOPESHORT( CXR_Model_BSP3::__Phys_TraceLine_i2 );
			bColl = __Phys_TraceLine_i(_pPhysContext, 1, v0, v1, _MediumFlags, &LineContext);
		}
		{
//			MSCOPESHORT( CXR_Model_BSP3::__Phys_TraceLine_r2 );
//			bColl = __Phys_TraceLine_r(1, v0, v1, _MediumFlags);
		}
	
#ifdef	MODEL_BSP3_BOUNDCHECK_STACK
		if( Phys_TraceLine_i2_MaxDepth > Phys_TraceLine_i2_MaxDepth_LastPeak )
		{
			Phys_TraceLine_i2_MaxDepth_LastPeak	= Phys_TraceLine_i2_MaxDepth;
#ifdef	PLATFORM_PS2
			scePrintf( "Phys_TraceLine_i2::New PeakStack %d\n", Phys_TraceLine_i2_MaxDepth_LastPeak );
#else
			ConOutL( CStrF("Phys_TraceLine_i2::New PeakStack %d\n", Phys_TraceLine_i2_MaxDepth_LastPeak).Str() );
#endif
		}
#endif	// MODEL_BSP3_BOUNDCHECK_STACK

		if (bColl)
		{
			if (_pCollisionInfo)
			{
				_pCollisionInfo->m_bIsCollision = true;
				_pCollisionInfo->m_bIsValid = false;
			}
			return true;
		}

		return false;
	}
}

bool CXR_Model_BSP3::Phys_IntersectSphere(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_IntersectSphere, false);
	MSCOPESHORT(CXR_Model_BSP3::Phys_IntersectSphere);
	CVec3Dfp32 v0, v0src;
	_Dest.MultiplyMatrix(_pPhysContext->m_WMatInv, v0);

	int MediumDest = Phys_GetMedium(_pPhysContext, _Dest);
	if (MediumDest & _MediumFlags)
	{
		if (!_pCollisionInfo)
			return true;

		int MediumOrigin = Phys_GetMedium(_pPhysContext, _Origin);
		if (MediumOrigin & _MediumFlags)
		{
			if (_pCollisionInfo)
			{
				_pCollisionInfo->m_bIsCollision = true;
				_pCollisionInfo->m_bIsValid = false;
			}
			return true;
		}
	}

	bool bImpact = __Phys_IntersectSphere_r(1, v0, _Radius, _MediumFlags, _pCollisionInfo);
	if (_pCollisionInfo || !bImpact)
		bImpact |= __Phys_IntersectSphere_SB(v0, _Radius, _MediumFlags, _pCollisionInfo);

	if (!bImpact && (MediumDest & _MediumFlags))
	{
		if (_pCollisionInfo)
		{
			_pCollisionInfo->m_bIsValid = false;
			_pCollisionInfo->m_bIsCollision = true;
		}
		return true;
	}

	if (bImpact && _pCollisionInfo && _pCollisionInfo->m_bIsValid)
	{
		_pCollisionInfo->m_Plane.Transform(_pPhysContext->m_WMat);
		_pCollisionInfo->m_Pos = _pCollisionInfo->m_LocalPos;
		_pCollisionInfo->m_Pos *= _pPhysContext->m_WMat;
	}

	return bImpact;
}

bool CXR_Model_BSP3::__Phys_IntersectOBB_CheckFace(int _iFace, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, CCollisionInfo* _pCollisionInfo) const
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_IntersectOBB_CheckFace, false);
	MSCOPESHORT(CXR_Model_BSP3::__Phys_IntersectOBB_CheckFace);
	const CBSP3_Face* pF = &m_pFaces[_iFace];
	int nv = pF->m_nPhysV;
	int iiv = pF->m_iiPhysV;

	if( Phys_Intersect_PolyOBB( m_pVertices, m_piVertices + iiv, nv, m_pPlanes[pF->m_iPlane], _BoxStart, _Box, false, _pCollisionInfo ) )
	{
		if( _pCollisionInfo )
		{
			_pCollisionInfo->m_pSurface	= s_lspSurfaces[pF->m_iSurface];
		}
		return true;
	}
	return false;
}

#define ENUMFACES

bool CXR_Model_BSP3::__Phys_IntersectOBB_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, const CBox3Dfp32& _Bound, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_IntersectOBB_r, false);
	MSCOPESHORT(CXR_Model_BSP3::__Phys_IntersectOBB_r); //AR-SCOPE

#ifdef ENUMFACES
	const int MaxFaces = 2048;
	uint32 liFaces[MaxFaces];

#ifdef	BSP3_MODEL_PHYS_RENDER
	CWireContainer* pOldWC = _pPhysContext->m_pWC;
	if (!_pCollisionInfo)
		_pPhysContext->m_pWC = NULL;
#endif

	int nFaces = EnumFaces_Box(_pPhysContext, _iNode, liFaces, MaxFaces, ENUM_HQ | ENUM_FACEFLAGS | ENUM_MEDIUMFLAGS, _MediumFlags, XW_FACE_PHYSICAL, _Bound);
	EnumFaces_Untag(_pPhysContext);
//if (nFaces) ConOut(CStrF("(__Phys_IntersectOBB_r) %d Faces", nFaces));
#ifdef	BSP3_MODEL_PHYS_RENDER
	_pPhysContext->m_pWC = pOldWC;
#endif

	if (nFaces)
	{
		bool bIntersect = false;
		CCollisionInfo CInfo;
		CCollisionInfo* pCInfo = (_pCollisionInfo)?&CInfo:NULL;;
		for(int i = 0; i < nFaces; i++)
		{
			int iFace = liFaces[i];

#ifdef	BSP3_MODEL_PHYS_RENDER
			if (_pPhysContext->m_pWC && _pCollisionInfo) 
				__Phys_RenderFace(iFace, _pPhysContext->m_WMat, _pPhysContext->m_pWC, 0xff006060);
#endif	// BSP3_MODEL_PHYS_RENDER

			CInfo.m_bIsCollision = false;

			const CBSP3_Face* pF = &m_pFaces[iFace];
			int nv = pF->m_nPhysV;
			int iiv = pF->m_iiPhysV;

			if( Phys_Intersect_PolyOBB( m_pVertices, m_piVertices + iiv, nv, m_pPlanes[pF->m_iPlane], _BoxStart, _Box, false, pCInfo ) )
			{
				if( !_pCollisionInfo ) return true;
				bIntersect = true;
				_pCollisionInfo->Improve(CInfo);
				_pCollisionInfo->m_pSurface	= s_lspSurfaces[pF->m_iSurface];
			}
		}

		return bIntersect;
	}

	return false;

#else

	if (!_iNode)
	{
		return false;
	}

	CBSP3_Node* pN = &m_pNodes[_iNode];
	CCollisionInfo CInfo;
	bool bIntersect = false;

	if (pN->IsLeaf())
	{
		int nf = pN->m_nFaces;
		int iif = pN->m_iiFaces;
//ConOut(CStrF("LEAF %d, Faces %d", _iNode, nf ));
//CStr s;
		for(int i = 0; i < nf; i++)
		{
			int iFace = m_liFaces[iif + i];
			const CBSP3_Face* pF = &m_lFaces[iFace];
			if (!(pF->m_Flags & XW_FACE_PHYSICAL)) continue;
			if (pF->m_Flags & XW_FACE_VOLUMEINSIDE) continue;
			if (!(m_lMediums[pF->m_iBackMedium].m_MediumFlags & _MediumFlags)) continue;

			int nv = pF->m_nPhysV;
			int iiv = pF->m_iiPhysV;

			// Get bound-box for polygon.
			CBox3Dfp32 PBox;
			PBox.m_Min = _FP32_MAX;
			PBox.m_Max = -_FP32_MAX;
			for(int v = 0; v < nv; v++)
			{
				const CVec3Dfp32& V = m_lVertices[m_liVertices[iiv + v]];
				for(int k = 0; k < 3; k++)
				{
//					if (V.k[k] < PBox.m_Min.k[k]) PBox.m_Min.k[k] = V.k[k];
//					if (V.k[k] > PBox.m_Max.k[k]) PBox.m_Max.k[k] = V.k[k];
					PBox.m_Min.k[k] = Min( PBox.m_Min.k[k], V.k[k] );
					PBox.m_Max.k[k] = Max( PBox.m_Max.k[k], V.k[k] );
				}
			}

			// Intersection?
			if (!PBox.IsInside(_Bound)) continue;

//	s += CStrF("%d, ", iFace);
//	ConOutL(CStrF("          tested %d, P %s, Pos %s", iFace, (char*)CInfo.m_Plane.GetString(), (char*)CInfo.m_LocalPos.GetString() ));

			if (__Phys_IntersectOBB_CheckFace(iFace, _BoxStart, _Box, (_pCollisionInfo) ? &CInfo : NULL))
			{
//if (_pCollisionInfo)
//	ConOut(CStrF("        FACE HIT %d, P %s, Pos %s", iFace, (char*)CInfo.m_Plane.GetString(), (char*)CInfo.m_LocalPos.GetString() ));

				bIntersect = true;
				if (!_pCollisionInfo) return true;

				if (_pCollisionInfo->Improve(CInfo))
					if (_pCollisionInfo->IsComplete())
						return true;
			}

		}
//	ConOut(CStrF("          Tested %s", (char*) s ));
	
/*		if (!bIntersect)
		{
			if (m_lMediums[pN->m_iMedium].m_MediumFlags & _MediumFlags) bIntersect = true;
		}*/

		return bIntersect;
	}
	else
	{
		// Node, recurse...
		int iPlane = m_pNodes[_iNode].m_iPlane;
		if (!iPlane) Error("__Phys_IntersectSphere_r", "Internal error.");
		CPlane3Dfp32* pP = &m_pPlanes[iPlane];
		int Side = pP->GetBoxPlaneSideMask(_Bound.m_Min, _Bound.m_Max);

		bool bIntersect1 = false;
		bool bIntersect2 = false;
		if (Side & 1) 
			if (bIntersect1 = __Phys_IntersectOBB_r(m_pNodes[_iNode].m_iNodeFront, _BoxStart, _Box, _Bound, _MediumFlags, _pCollisionInfo))
				if (!_pCollisionInfo) return true;

		if (Side & 2) 
			if (bIntersect2 = __Phys_IntersectOBB_r(m_pNodes[_iNode].m_iNodeBack, _BoxStart, _Box, _Bound, _MediumFlags, _pCollisionInfo))
				if (!_pCollisionInfo) return true;

//		if (!_pCollisionInfo) return false;
		return (bIntersect1 || bIntersect2);
	}
#endif
}

bool CXR_Model_BSP3::__Phys_IntersectOBB_i(CXR_PhysicsContext* _pPhysContext, int _iNode, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, const CBox3Dfp32& _Bound, int _MediumFlags, CCollisionInfo* _pCollisionInfo) const
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_IntersectOBB_i, false);
	MSCOPESHORT(CXR_Model_BSP3::__Phys_IntersectOBB_i);

	if (!_iNode)
	{
		return false;
	}

	bool bIntersect = false;
	uint32 aWorkingSet[128];
	int WorkingStack = 0;
	aWorkingSet[WorkingStack++] = _iNode;
	
StartOf_Phys_IntersectOBB_i:
	uint32 WorkingNode = aWorkingSet[--WorkingStack];

StartOf_Phys_IntersectOBB_i_NoAdd:
	const CBSP3_Node* pN = &m_pNodes[WorkingNode];

	if (pN->IsLeaf())
	{
		int nf = pN->m_nFaces;
		int iif = pN->m_iiFaces;

		const uint32* piFaces = &m_piFaces[iif];
		for(int i = 0; i < nf; i++)
		{
			int iFace = piFaces[i];
			{
				int iFaceIdx = iFace >> 3;
				uint8 iFaceMask = 1 << (iFace & 0x7);
				if (s_pFTag[iFaceIdx] & iFaceMask) continue;
				if (!s_pFTag[iFaceIdx]) s_piFUntag[ms_nEnumUntag++] = iFace;
				s_pFTag[iFaceIdx] |= iFaceMask;
			}
			const CBSP3_Face* pF = &m_pFaces[iFace];

#ifdef	BSP3_MODEL_PHYS_RENDER
			if (_pPhysContext->m_pWC) 
				__Phys_RenderFace(iFace, _pPhysContext->m_WMat, _pPhysContext->m_pWC, 0xffff0000);
#endif	// BSP3_MODEL_PHYS_RENDER

			if (!(pF->m_Flags & XW_FACE_PHYSICAL) || (pF->m_Flags & XW_FACE_VOLUMEINSIDE)) continue;
			if (!(s_pMediums[pF->m_iBackMedium].m_MediumFlags & _MediumFlags)) continue;

			fp32 MinDist = m_pPlanes[pF->m_iPlane].GetBoxMinDistance(_Bound.m_Min, _Bound.m_Max);
			if (MinDist > MODEL_BSP_EPSILON) continue;

			uint32 nv = pF->m_nPhysV;
			uint32 iiv = pF->m_iiPhysV;

			// Get bound-box for polygon.
			const uint32* piVert = m_piVertices + iiv;
			CVec3Dfp32 PMin = m_pVertices[piVert[0]];
			CVec3Dfp32 PMax = PMin;
			
			for( uint32 v = 1; v < nv; v++ )
			{
				const CVec3Dfp32& V = m_pVertices[piVert[v]];
				PMin.k[0] = Min( PMin.k[0], V.k[0] );
				PMax.k[0] = Max( PMax.k[0], V.k[0] );
				PMin.k[1] = Min( PMin.k[1], V.k[1] );
				PMax.k[1] = Max( PMax.k[1], V.k[1] );
				PMin.k[2] = Min( PMin.k[2], V.k[2] );
				PMax.k[2] = Max( PMax.k[2], V.k[2] );
			}
			
			if( _Bound.m_Min.k[0] > PMax.k[0] ||
				_Bound.m_Min.k[1] > PMax.k[1] ||
				_Bound.m_Min.k[2] > PMax.k[2] ||
				_Bound.m_Max.k[0] < PMin.k[0] ||
				_Bound.m_Max.k[1] < PMin.k[1] ||
				_Bound.m_Max.k[2] < PMin.k[2] )
				continue;

			if( _pCollisionInfo )
			{
				CCollisionInfo CInfo;

#ifdef	BSP3_MODEL_PHYS_RENDER
				if (_pPhysContext->m_pWC) 
					__Phys_RenderFace(iFace, _pPhysContext->m_WMat, _pPhysContext->m_pWC, 0xff00ff00);
#endif	// BSP3_MODEL_PHYS_RENDER
				if( Phys_Intersect_PolyOBB( m_pVertices, piVert, nv, m_pPlanes[pF->m_iPlane], _BoxStart, _Box, false, &CInfo ) )
				{
#ifdef	BSP3_MODEL_PHYS_RENDER
					if (_pPhysContext->m_pWC) 
						__Phys_RenderFace(iFace, _pPhysContext->m_WMat, _pPhysContext->m_pWC, 0xffffffff);
#endif	// BSP3_MODEL_PHYS_RENDER
					CInfo.m_pSurface	= s_lspSurfaces[pF->m_iSurface];
					bIntersect = true;
					_pCollisionInfo->Improve(CInfo);
				}
			}
			else
			{
				if( Phys_Intersect_PolyOBB( m_pVertices, piVert, nv, m_pPlanes[pF->m_iPlane], _BoxStart, _Box, false, NULL ) )
				{
					return true;
				}
			}

		}
	}
	else if( pN->IsNode() )
	{
		// Node, recurse...
		const CPlane3Dfp32* pP = &m_pPlanes[pN->m_iPlane];
		int Side = pP->GetBoxPlaneSideMask(_Bound.m_Min, _Bound.m_Max);

		uint32 iFront = pN->m_iNodeFront;
		uint32 iBack = pN->m_iNodeBack;
		if( Side == 1 )
		{
			if( iFront > 0 )
			{
				WorkingNode = iFront;
				goto StartOf_Phys_IntersectOBB_i_NoAdd;
			}
		}
		else if( Side == 2 )
		{
			if( iBack > 0 )
			{
				WorkingNode = iBack;
				goto StartOf_Phys_IntersectOBB_i_NoAdd;
			}
		}
		else
		{
			if( iFront && iBack )
			{
				if( iFront > 0 )
					aWorkingSet[WorkingStack++]	= iFront;

				if( iBack > 0 )
				{
					WorkingNode = iBack;
					goto StartOf_Phys_IntersectOBB_i_NoAdd;
				}
			}
			else
			{
				if( iFront )
				{
					WorkingNode = iFront;
					goto StartOf_Phys_IntersectOBB_i_NoAdd;
				}
				if( iBack )
				{
					WorkingNode = iBack;
					goto StartOf_Phys_IntersectOBB_i_NoAdd;
				}
			}
		}
	}
	
	if( WorkingStack > 0 )
		goto StartOf_Phys_IntersectOBB_i;
	
	return bIntersect;

/*
	if (!_iNode)
	{
		return false;
	}

	const CBSP3_Node* pN = &m_pNodes[_iNode];

	if (pN->IsLeaf())
	{
		int nf = pN->m_nFaces;
		int iif = pN->m_iiFaces;
		bool bIntersect = false;
		CCollisionInfo CInfo;

		for(int i = 0; i < nf; i++)
		{
			int iFace = m_liFaces[iif + i];
			const CBSP3_Face* pF = &m_lFaces[iFace];
			if (!(pF->m_Flags & XW_FACE_PHYSICAL)) continue;
			if (pF->m_Flags & XW_FACE_VOLUMEINSIDE) continue;
			if (!(s_pMediums[pF->m_iBackMedium].m_MediumFlags & _MediumFlags)) continue;

			int nv = pF->m_nPhysV;
			int iiv = pF->m_iiPhysV;

			// Get bound-box for polygon.
			CVec3Dfp32 PMin = m_pVertices[m_piVertices[iiv]];
			CVec3Dfp32 PMax = PMin;
			
			for( int v = 1; v < nv; v++ )
			{
				const CVec3Dfp32& V = m_pVertices[m_piVertices[iiv+v]];
				PMin.k[0] = Min( PMin.k[0], V.k[0] );
				PMax.k[0] = Max( PMax.k[0], V.k[0] );
				PMin.k[1] = Min( PMin.k[1], V.k[1] );
				PMax.k[1] = Max( PMax.k[1], V.k[1] );
				PMin.k[2] = Min( PMin.k[2], V.k[2] );
				PMax.k[2] = Max( PMax.k[2], V.k[2] );
			}
			
			if( _Bound.m_Min.k[0] > PMax.k[0] ||
				_Bound.m_Min.k[1] > PMax.k[1] ||
				_Bound.m_Min.k[2] > PMax.k[2] ||
				_Bound.m_Max.k[0] < PMin.k[0] ||
				_Bound.m_Max.k[1] < PMin.k[1] ||
				_Bound.m_Max.k[2] < PMin.k[2] )
				continue;

			if (__Phys_IntersectOBB_CheckFace(iFace, _BoxStart, _Box, (_pCollisionInfo) ? &CInfo : NULL))
			{

				bIntersect = true;
				if (!_pCollisionInfo) return true;

				if (_pCollisionInfo->Improve(CInfo))
					if (_pCollisionInfo->IsComplete())
						return true;
			}

		}

		return bIntersect;
	}
	else if( pN->IsNode() )
	{
		// Node, recurse...
		int iPlane = m_pNodes[_iNode].m_iPlane;
		CPlane3Dfp32* pP = &m_pPlanes[iPlane];
		int Side = pP->GetBoxPlaneSideMask(_Bound.m_Min, _Bound.m_Max);

		bool bIntersect = false;
		if (Side & 1) 
			if (bIntersect |= __Phys_IntersectOBB_i(m_pNodes[_iNode].m_iNodeFront, _BoxStart, _Box, _Bound, _MediumFlags, _pCollisionInfo))
				if (!_pCollisionInfo) return true;

		if (Side & 2) 
			if (bIntersect |= __Phys_IntersectOBB_i(m_pNodes[_iNode].m_iNodeBack, _BoxStart, _Box, _Bound, _MediumFlags, _pCollisionInfo))
				if (!_pCollisionInfo) return true;

		return (bIntersect);
	}
	
	return false;
*/
}

class CPhysPCSContext
{
public:
	const CBox3Dfp32* m_pcs_TransformedBox;
	const CBSP3_BoxInterleaved* m_pcs_TransformedBoxInterleaved;
	const CVec3Dfp32* m_pcs_TransformedCenter;
	const CBox3Dfp32* m_pEnumBox;
	fp32 m_pcs_Radius;
	uint8* m_pFTag;
	uint16* m_piFUntag;
	int m_nEnumUntag;
	int m_EnumFaceFlags;
	int m_EnumMedium;
	const CXR_MediumDesc* m_pMediums;
	CPotColSet* m_pPCS;
	int m_iObj;
	int m_bIN1N2Flags;
	spCXW_Surface* m_lspSurfaces;
};

void CXR_Model_BSP3::CollectPCS(CXR_PhysicsContext* _pPhysContext, const uint8 _IN1N2Flags, CPotColSet *_pcs, const int _iObj, const int _MediumFlags )
{
	MSCOPESHORT(CXR_Model_BSP3::CollectPCS);
	CBox3Dfp32 _Box; 	//oh this is just soo stupid!
	CBox3Dfp32 EnumBox; 	//oh this is just soo stupid!
	CBox3Dfp32 TransformedBox;
	CBSP3_BoxInterleaved TransformedBoxInterleaved;
	CVec3Dfp32 TransformedCenter;

	_pcs->GetBox( _Box.m_Min.k ); // and this is a very ugly way to set the bloody template box

	// set a bunch of static class variables!  (ms_ )
	EnumBox = _Box;

	// Transform PCS into BSP local space
	TransformedCenter = *(CVec3Dfp32*)_pcs->m_fCenter;
	TransformedCenter.MultiplyMatrix(_pPhysContext->m_WMatInv);
	EnumBox.Transform(_pPhysContext->m_WMatInv, TransformedBox);
	TransformedBoxInterleaved.Init(TransformedBox);

	CPhysPCSContext PCSArg;
	PCSArg.m_pcs_TransformedBox = &TransformedBox;
	PCSArg.m_pcs_TransformedBoxInterleaved = &TransformedBoxInterleaved;
	PCSArg.m_pcs_TransformedCenter = &TransformedCenter;
	PCSArg.m_pEnumBox = &EnumBox;
	PCSArg.m_pcs_Radius = _pcs->m_fRadius;
	PCSArg.m_pFTag = m_lFaceTagList.GetBasePtr();
	PCSArg.m_piFUntag = m_lFaceUntagList.GetBasePtr();
	PCSArg.m_EnumFaceFlags = XW_FACE_PHYSICAL;
	PCSArg.m_EnumMedium = _MediumFlags;
	PCSArg.m_pMediums = m_lMediums.GetBasePtr();
	PCSArg.m_pPCS = _pcs;
	PCSArg.m_iObj = _iObj;
	PCSArg.m_bIN1N2Flags = _IN1N2Flags;
	PCSArg.m_lspSurfaces = m_lspSurfaces.GetBasePtr();
	PCSArg.m_nEnumUntag = 0;

	// recursevly DFS
	CollectPCS_i(_pPhysContext, 1, &PCSArg);
	ms_nEnumUntag = PCSArg.m_nEnumUntag;

	EnumFaces_Untag(_pPhysContext);

#ifdef PCSCHECK_INSOLID
	//AR-TEMP-ADD: Get medium in middle of box
	CVec3Dfp32 Center;
	_Box.m_Min.Lerp(_Box.m_Max, 0.5f, Center);
	int Medium = CXR_Model_BSP3::Phys_GetMedium(_pPhysContext, Center);
	if (Medium & _MediumFlags)
		_pcs->m_bIsInSolid = true;
#endif
}


void CXR_Model_BSP3::CollectPCS_r(CXR_PhysicsContext* _pPhysContext, int _iNode, CPhysPCSContext* _pPCSArg) const
{
	if (!_iNode)
	{ // empty tree
		return;
	}
	
	const CBSP3_Node* pN = &m_pNodes[_iNode];

	if (pN->IsLeaf())
	{
		CollectPCS_IsLeaf(_pPhysContext, pN, _pPCSArg);
	}
	else if (pN->IsNode())
	{
		const CPlane3Dfp32& W = m_pPlanes[pN->m_iPlane];
		const CVec3Dfp32& C = *_pPCSArg->m_pcs_TransformedCenter;
		const fp32 d = W.Distance(C);
		const fp32 R = _pPCSArg->m_pcs_Radius;

		if (d > R)
		{ // positive side of partition plane
			CollectPCS_r(_pPhysContext, pN->m_iNodeFront, _pPCSArg);
			return;
		}	
		else if (d < -R)
		{ // negative side of partition plane
			CollectPCS_r(_pPhysContext, pN->m_iNodeBack, _pPCSArg);
			return;
		}

		// bound-sphere is split by plane - better check with the box..
		const CBSP3_BoxInterleaved& B = *_pPCSArg->m_pcs_TransformedBoxInterleaved;
		fp32 MinDist, MaxDist;
		GetBoxMinMaxDistance(W, B, MinDist, MaxDist);

		if (MinDist < MODEL_BSP_EPSILON)
			CollectPCS_r(_pPhysContext, pN->m_iNodeBack, _pPCSArg);

		if (MaxDist > -MODEL_BSP_EPSILON)
			CollectPCS_r(_pPhysContext, pN->m_iNodeFront, _pPCSArg);
	}
}

void CXR_Model_BSP3::CollectPCS_IsLeaf(CXR_PhysicsContext* _pPhysContext, const CBSP3_Node* _pNode, CPhysPCSContext* _pPCSArg ) const
{
	const uint16 nFaces = _pNode->m_nFaces;
	const uint32 iiFaces = _pNode->m_iiFaces;

	const uint32* piFaces = &m_piFaces[iiFaces];

	const CBox3Dfp32* pcs_TransformedBox = _pPCSArg->m_pcs_TransformedBox;
	uint8* pFTag = _pPCSArg->m_pFTag;
	uint16* piFUnTag = _pPCSArg->m_piFUntag;
	const CMat4Dfp32& PhysWMat = _pPhysContext->m_WMat;
	int nEnumUntag = _pPCSArg->m_nEnumUntag;
	const int EnumFaceFlags = _pPCSArg->m_EnumFaceFlags;
	const int EnumMedium = _pPCSArg->m_EnumMedium;
	const CXR_MediumDesc* pMediums = _pPCSArg->m_pMediums;
	CPotColSet* pPCS = _pPCSArg->m_pPCS;
	const int iObj = _pPCSArg->m_iObj;
	const int bIN1N2Flags = _pPCSArg->m_bIN1N2Flags;
	spCXW_Surface* lspSurfaces = _pPCSArg->m_lspSurfaces;
	for (uint16 f = 0; f < nFaces; f++)
	{
		const uint32 iFace = piFaces[f];
		
//		const uint8 TagByte = pFTag[iFace >> 3];
		if (pFTag[iFace >> 3] & (1 << (iFace & 7))) // check if already processed
			continue;
		
		if (!pFTag[iFace >> 3])	// before we tag, we must mark it for later cleanup
			piFUnTag[nEnumUntag++] = iFace;
		
		pFTag[iFace >> 3] |= 1 << (iFace & 7);	// tag face as processed

		const CBSP3_Face* pF = &m_pFaces[iFace];

		if (!(EnumFaceFlags & pF->m_Flags))
			continue;

		if (!(pMediums[pF->m_iBackMedium].m_MediumFlags & EnumMedium))
			continue;

		const CPlane3Dfp32& WOrig = m_pPlanes[pF->m_iPlane];

		fp32 MinDist, MaxDist;
		WOrig.GetBoxMinAndMaxDistance( pcs_TransformedBox->m_Min, pcs_TransformedBox->m_Max, MinDist, MaxDist );
		if (MinDist > MODEL_BSP_EPSILON || MaxDist < -MODEL_BSP_EPSILON)
			continue;

		const uint32 nv = pF->m_nPhysV;
		const uint32* piVerts = m_piVertices + pF->m_iiPhysV;
		{
			// check face bound min/max
			{
				CVec3Dfp32 PMin = m_pVertices[piVerts[0]];
				CVec3Dfp32 PMax = PMin;
				for( uint32 v = 1; v < nv; v++ )
				{
					const CVec3Dfp32& V = m_pVertices[piVerts[v]];
					PMin.k[0]	= Min( PMin.k[0], V.k[0] );
					PMax.k[0]	= Max( PMax.k[0], V.k[0] );
					PMin.k[1]	= Min( PMin.k[1], V.k[1] );
					PMax.k[1]	= Max( PMax.k[1], V.k[1] );
					PMin.k[2]	= Min( PMin.k[2], V.k[2] );
					PMax.k[2]	= Max( PMax.k[2], V.k[2] );
				}

				if (pcs_TransformedBox->m_Min.k[0] > PMax.k[0] ||
					pcs_TransformedBox->m_Min.k[1] > PMax.k[1] ||
					pcs_TransformedBox->m_Min.k[2] > PMax.k[2] ||
					pcs_TransformedBox->m_Max.k[0] < PMin.k[0] ||
					pcs_TransformedBox->m_Max.k[1] < PMin.k[1] ||
					pcs_TransformedBox->m_Max.k[2] < PMin.k[2])
					continue;
			}
		}

		// Add faces
		{
			CPlane3Dfp32 W = WOrig;
			W.Transform(PhysWMat);

			CVec3Dfp32 tri[3];
			tri[0] = m_pVertices[piVerts[0]] * PhysWMat;
			tri[1] = m_pVertices[piVerts[1]] * PhysWMat;

			for (uint32 v = 2; v < nv; v++)
			{ // extract all triangles and add them to PCS
				tri[2] = m_pVertices[piVerts[v]] * PhysWMat;

				if (pPCS->ValidFace((const float *)&W, (const float *)tri))
					pPCS->AddFace( bIN1N2Flags, iObj, (const float *)&W, (const float *)tri, lspSurfaces[pF->m_iSurface] );

				tri[1] = tri[2];
			}
		}				
	}
	_pPCSArg->m_nEnumUntag = nEnumUntag;
}

void CXR_Model_BSP3::CollectPCS_i(CXR_PhysicsContext* _pPhysContext, int _iNode, CPhysPCSContext* _pPCSArg) const
{
	if (!_iNode)
	{ // empty tree
		return;
	}

	uint32 aWorkingSet[256];
	int WorkingStack = 0;
	
	aWorkingSet[WorkingStack++] = _iNode;

StartOf_CollectPCS_i:
	uint32 WorkingNode = aWorkingSet[--WorkingStack];
	
StartOf_CollectPCS_i_NoAdd:
	const CBSP3_Node* pN = &m_lNodes[WorkingNode];

	if (pN->IsLeaf())
	{
		CollectPCS_IsLeaf(_pPhysContext, pN, _pPCSArg);
	}
	else if (pN->IsNode())
	{
		const CPlane3Dfp32& W = m_pPlanes[pN->m_iPlane];
		const fp32 d = W.Distance(*_pPCSArg->m_pcs_TransformedCenter);
		const fp32 R = _pPCSArg->m_pcs_Radius;

		uint32 iFront = pN->m_iNodeFront;
		uint32 iBack = pN->m_iNodeBack;

		if (d > R)
		{ // positive side of partition plane
			if( iFront > 0 )
			{
				WorkingNode = iFront;
				goto StartOf_CollectPCS_i_NoAdd;
			}
		}	
		else if (d < -R)
		{ // negative side of partition plane
			if( iBack > 0 )
			{
				WorkingNode = iBack;
				goto StartOf_CollectPCS_i_NoAdd;
			}
		}
		else
		{

			// bound-sphere is split by plane - better check with the box..
			const CBSP3_BoxInterleaved& B = *_pPCSArg->m_pcs_TransformedBoxInterleaved;
			fp32 MinDist, MaxDist;
			GetBoxMinMaxDistance(W, B, MinDist, MaxDist);

			if (iBack > 0 && MinDist < MODEL_BSP_EPSILON)
				aWorkingSet[WorkingStack++]	= iBack;

			if (iFront > 0 && MaxDist > -MODEL_BSP_EPSILON)
			{
				WorkingNode = iFront;
				goto StartOf_CollectPCS_i_NoAdd;
			}
		}
	}

	if( WorkingStack > 0 )
		goto StartOf_CollectPCS_i;
}



bool CXR_Model_BSP3::Phys_IntersectBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _BoxOrigin, const CPhysOBB& _BoxDest, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP3_Phys_IntersectBox, false);
	MSCOPESHORT(CXR_Model_BSP3::Phys_IntersectBox); //AR-SCOPE

	CPhysOBB LBoxOrigin, LBoxDest;
	_BoxOrigin.Transform(_pPhysContext->m_WMatInv, LBoxOrigin);
	_BoxDest.Transform(_pPhysContext->m_WMatInv, LBoxDest);

	// Get bounding box for both OBBs.
	CBox3Dfp32 BoundBoxOrigin, BoundBoxDest;
	LBoxOrigin.GetBoundBox(BoundBoxOrigin);
	LBoxDest.GetBoundBox(BoundBoxDest);
	BoundBoxOrigin.Expand(BoundBoxDest);
	BoundBoxOrigin.m_Min -= CVec3Dfp32(1);
	BoundBoxOrigin.m_Max += CVec3Dfp32(1);

//	if (!BoundBoxOrigin.IsInside(m_BoundBox))
//		return false;

#ifdef	BSP3_MODEL_PHYS_RENDER
	if (_pPhysContext->m_pWC)
	{
		_pPhysContext->m_pWC->RenderAABB(_pPhysContext->m_WMat, BoundBoxOrigin, 0xff3030ff);

		CBox3Dfp32 Box;
		GetBound_Box(Box);

		_pPhysContext->m_pWC->RenderAABB(_pPhysContext->m_WMat, Box, 0xff300000);
	}
#endif	// BSP3_MODEL_PHYS_RENDER

	s_pFTag = m_lFaceTagList.GetBasePtr();
	s_piFUntag = m_lFaceUntagList.GetBasePtr();
	s_pMediums = m_lMediums.GetBasePtr();
	s_lspSurfaces = m_lspSurfaces.GetBasePtr();

	int MediumDest = Phys_GetMedium(_pPhysContext, _BoxDest.m_C);
	if (MediumDest & _MediumFlags)
	{
		if (!_pCollisionInfo)
			return true;

		int MediumOrigin = Phys_GetMedium(_pPhysContext, _BoxOrigin.m_C);
		if (MediumOrigin & _MediumFlags)
		{
			if (_pCollisionInfo)
			{
				_pCollisionInfo->m_bIsCollision = true;
				_pCollisionInfo->m_bIsValid = false;
			}
			return true;
		}
	}

	if (_pCollisionInfo)
	{
//		bool bImpact = __Phys_IntersectOBB_r(1, LBoxOrigin, LBoxDest, BoundBoxOrigin, _MediumFlags, _pCollisionInfo);
		bool bImpact = __Phys_IntersectOBB_i(_pPhysContext, 1, LBoxOrigin, LBoxDest, BoundBoxOrigin, _MediumFlags, _pCollisionInfo);
		EnumFaces_Untag(_pPhysContext);

		if (!bImpact && (MediumDest & _MediumFlags))
		{
			if (_pCollisionInfo)
			{
				_pCollisionInfo->m_bIsValid = false;
				_pCollisionInfo->m_bIsCollision = true;
			}
			return true;
		}

		if (_pCollisionInfo->m_bIsValid)
		{
			_pCollisionInfo->m_Plane.Transform(_pPhysContext->m_WMat);
			_pCollisionInfo->m_Pos = _pCollisionInfo->m_LocalPos;
			_pCollisionInfo->m_Pos *= _pPhysContext->m_WMat;
		}

		return bImpact;
	}
	else
	{
//		bool bImpact = __Phys_IntersectOBB_r(1, LBoxOrigin, LBoxDest, BoundBoxOrigin, _MediumFlags, NULL);
		bool bImpact = __Phys_IntersectOBB_i(_pPhysContext, 1, LBoxOrigin, LBoxDest, BoundBoxOrigin, _MediumFlags, NULL);
		EnumFaces_Untag(_pPhysContext);

		if( bImpact )
		{
			return true;
		}
	}

	return false;
}



int CXR_Model_BSP3::Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_GetMedium, 0);
	MSCOPESHORT(CXR_Model_BSP3::Phys_GetMedium_1);

	CVec3Dfp32 v0;
	_v0.MultiplyMatrix(_pPhysContext->m_WMatInv, v0);
	int iLeaf = GetLeaf(v0);
	if (!iLeaf) return XW_MEDIUM_SOLID;
	return m_lMediums[m_lNodes[iLeaf].m_iMedium].m_MediumFlags;
}

void CXR_Model_BSP3::Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, CXR_MediumDesc& _RetMedium)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_GetMedium_2, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP3::Phys_GetMedium_2);

	CVec3Dfp32 v0;
	_v0.MultiplyMatrix(_pPhysContext->m_WMatInv, v0);
	int iLeaf = GetLeaf(v0);
	if (!iLeaf)
	{
		_RetMedium.SetSolid();
		return;
	}

	_RetMedium = m_lMediums[m_lNodes[iLeaf].m_iMedium];
}

int CXR_Model_BSP3::__Phys_GetCombinedMediumFlags_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CBox3Dfp32* _pBox)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_GetCombinedMediumFlags_r, 0);
	const CBSP3_Node* pN = &m_pNodes[_iNode];

	if (pN->IsLeaf())
	{
		return m_lMediums[pN->m_iMedium].m_MediumFlags;
	}
	else
	{
		CPlane3Dfp32 Plane = m_pPlanes[pN->m_iPlane];
		Plane.Transform(_pPhysContext->m_WMat);	// We're doing this backwards to what we usually do, i.e transforming model data to world space instead of transforming input to model space.

		int MediumFlags = 0;
		if (Plane.GetBoxMinDistance(_pBox->m_Min, _pBox->m_Max) < -0.01f)
			MediumFlags |= __Phys_GetCombinedMediumFlags_r(_pPhysContext, pN->m_iNodeBack, _pBox);
		if (Plane.GetBoxMaxDistance(_pBox->m_Min, _pBox->m_Max) > 0.01f)
			MediumFlags |= __Phys_GetCombinedMediumFlags_r(_pPhysContext, pN->m_iNodeFront, _pBox);

		return MediumFlags;
	}
}

int CXR_Model_BSP3::Phys_GetCombinedMediumFlags(CXR_PhysicsContext* _pPhysContext, const CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_GetCombinedMediumFlags, 0);
	MSCOPESHORT(CXR_Model_BSP3::Phys_GetCombinedMediumFlags);
	return __Phys_GetCombinedMediumFlags_r(_pPhysContext, 1, &_Box);
}


