#include "PCH.h"

#include "WBSP4Model.h"
#include "WBSP4Def.h"
#include "../../Classes/Render/MWireContainer.h"

#include "../../XR/Phys/GjkCollision.h"
#include "../../Classes/GameWorld/WDynamicsConvexCollision.h"
#include "../../XR/Phys/SeparatingAxis.h"

//#pragma optimize( "", off )
//#pragma inline_depth(0)

extern int aShiftMulTab[];


// -------------------------------------------------------------------
// Physics internal functions
// -------------------------------------------------------------------
void CXR_Model_BSP4::__Phys_RenderFace(int _iFace, const CMat4Dfp32& _WMat, CWireContainer* _pWC, int _Col) const
{
	MAUTOSTRIP(CXR_Model_BSP4___Phys_RenderFace, MAUTOSTRIP_VOID);

	const CBSP4_Face* pF = &m_pFaces[_iFace];

	int iLast = m_piVertices[pF->m_iiVertices + pF->m_nVertices - 1];
	CVec3Dfp32 p0 = m_pVertices[iLast] * _WMat;
	for(int i = 0; i < pF->m_nVertices; i++)
	{
		int iVertex = m_piVertices[pF->m_iiVertices + i];
		CVec3Dfp32 p1 = m_pVertices[iVertex] * _WMat;
		_pWC->RenderWire(p0, p1, _Col, 0.0f, false);
		p0	= p1;
	}
}

bool CXR_Model_BSP4::__Phys_IntersectSphere_Polygon(
	const CVec3Dfp32* _pV, 
	const uint32* _piV, 
	int _nV,
	const CPlane3Dfp32& _Plane, 
	const CVec3Dfp32& _Pos, 
	fp32 _Radius, 
	CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_IntersectSphere_Polygon, false);

	fp32 d = _Plane.Distance(_Pos);
	if (d < -_Radius) return false;
	if (d > _Radius) return false;

	CPlane3Dfp32 Edge_Plane(CVec3Dfp32(0, 0, 1), 0);
	uint Edge_bCollided = false;
	fp32 Edge_Nearest = _Radius*2.0f;
	CVec3Dfp32 Edge_Pos;

	uint bAllInside = _nV;
	uint InsideBit = 1;
	uint NextInsideBit = 1;
	uint InsideMask = 0;
	uint iv1 = _piV[_nV - 1];
	uint iv0 = 0;
	for(int v = 0; v < _nV; v++, iv1 = iv0, InsideBit = NextInsideBit)
	{
		NextInsideBit = InsideBit << 1;
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
			bAllInside = false;
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
				fp32 l = Abs(dist_e);
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
		}
		else
			InsideMask |= InsideBit;
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
		_pCollisionInfo->m_Distance = Abs(d) - _Radius;
		_pCollisionInfo->m_LocalPos = _Pos - _pCollisionInfo->m_Plane.n*d;
		_pCollisionInfo->m_Velocity = 0;
	}
	else
	{
		if (!Edge_bCollided)
			return false;
		_pCollisionInfo->m_bIsValid = true;
		_pCollisionInfo->m_bIsCollision = true;
		_pCollisionInfo->m_Plane = Edge_Plane;
		_pCollisionInfo->m_Distance = Edge_Nearest - _Radius;
		_pCollisionInfo->m_LocalPos = Edge_Pos;
		_pCollisionInfo->m_Velocity = 0;
	}
	return true;
}


bool CXR_Model_BSP4::__Phys_IntersectSphere_CheckFace(int _iFace, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, CCollisionInfo* _pCollisionInfo) const
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_IntersectSphere_CheckFace, false);
	CBSP4_Face* pF = &m_pFaces[_iFace];

	int iPlane = pF->m_iPlane;
	CPlane3Dfp32* pP = &m_pPlanes[iPlane];
	fp32 d = pP->Distance(_Dest);

//ConOut(CStrF("(1) iF %d", _iFace));
	if (d < -_Radius) return false;
	if (d > _Radius) return false;

//	fp32 RSqr = Sqr(_Radius);

	CPlane3Dfp32 Edge_Plane(CVec3Dfp32(0, 0, 1), 0);
	uint Edge_bCollided = false;
	fp32 Edge_Nearest = _Radius*2.0f;
	CVec3Dfp32 Edge_Pos;

	uint nv = pF->m_nVertices;
	uint iiv = pF->m_iiVertices;

	uint bAllInside = nv;
	uint InsideBit = 1;
	uint NextInsideBit = 1;
	uint InsideMask = 0;
	uint iv1 = m_piVertices[iiv + nv - 1];
//ConOut(CStrF("(2) iF %d, nv %d", _iFace, nv));
	uint iv0 = 0;
	for(int v = 0; v < nv; v++, iv1 = iv0, InsideBit = NextInsideBit)
	{
		NextInsideBit = InsideBit << 1;
		iv0 = m_piVertices[iiv + v];
		CVec3Dfp32 vs = _Dest - m_pVertices[iv0];
		CVec3Dfp32 ve = m_pVertices[iv1] - m_pVertices[iv0];
		fp32 len_e = ve.Length();
		CVec3Dfp32 n = (vs/ve);
		fp32 dist_e = n.Length() / len_e;
		
		fp32 Side = n*pP->n;
		if (Side > 0.0f)
		{
			bAllInside = false;
			fp32 t = (vs*ve) / (ve*ve);
			if (t < 0.0f)
			{
				fp32 l = vs.Length();
				if (l > _Radius) continue;
				if (!_pCollisionInfo) return true;
				if (_pCollisionInfo && (l < Edge_Nearest))
				{
					Edge_Nearest = l;
					Edge_Plane.CreateNV(vs.Normalize(), m_pVertices[iv0]);
					Edge_Pos = m_pVertices[iv0];
				}
				Edge_bCollided = true;
			}
			else if (t > 1.0f)
			{
				CVec3Dfp32 vs2 = _Dest - m_pVertices[iv1];
				fp32 l = vs2.Length();
				if (l > _Radius) continue;
				if (!_pCollisionInfo) return true;
				if (_pCollisionInfo && (l < Edge_Nearest))
				{
					Edge_Nearest = l;
					Edge_Plane.CreateNV(vs2.Normalize(), m_pVertices[iv1]);
					Edge_Pos = m_pVertices[iv1];
				}
				Edge_bCollided = true;
			}
			else 
			{
				fp32 l = Abs(dist_e);
				if (l > _Radius) return false;
				if (!_pCollisionInfo) return true;
				if (_pCollisionInfo && (l < Edge_Nearest))
				{
					Edge_Nearest = l;

					CVec3Dfp32 vsproj;
					vs.Project(ve, vsproj);
					Edge_Plane.CreateNV((vs - vsproj).Normalize(), m_pVertices[iv0]);
					Edge_Pos = m_pVertices[iv0] + ve*t;
				}
				Edge_bCollided = true;
			}
		}
		else
			InsideMask |= InsideBit;

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
		_pCollisionInfo->m_Distance = d - _Radius;
		_pCollisionInfo->m_LocalPos = _Dest - _pCollisionInfo->m_Plane.n*d;
		_pCollisionInfo->m_Velocity = 0;
		_pCollisionInfo->m_pSurface = m_lspSurfaces[pF->m_iSurface];
//LogFile(CStrF("Setting plane. %d, %f, ", _iFace, _pCollisionInfo->m_Distance) + _pCollisionInfo->m_Plane.n.GetString());

		// Calculate 'time' by using the returned collision plane
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
	{
		fp32 Time = 0.0f;
		const CVec3Dfp32& n = _pCollisionInfo->m_Plane.n;
		CVec3Dfp32 RelOrig, Dir;
		_Dest.Sub(_Origin, Dir);
		fp32 Den = n * Dir;
		if (M_Fabs(Den) > 0.001f)
		{
			fp32 d = -_Radius;						// Plane in collision point, offseted by radius
			_Origin.Sub(_pCollisionInfo->m_LocalPos, RelOrig);
			Time = -(d + n * RelOrig) / Den;
		}
		_pCollisionInfo->m_Time = Clamp01(Time);
	}
	return true;
}

bool CXR_Model_BSP4::__Phys_IntersectSphere_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo) const
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_IntersectSphere_r, false);
	if (!_iNode) 
	{
//ConOut(CStr("Solid node!"));
//		if (_pCollisionInfo) _pCollisionInfo->m_bIsValid = false;
		return false;
	}

	CCollisionInfo CInfo;
	bool bIntersect = false;

#ifdef BSP_USE_OCTAAABB

	//The crazy medium-stuff was lost in this since Oct nodes does not store such information
	if( m_lOctaAABBNodes.Len() )
	{
		TAP_RCD<const CXR_MediumDesc> pMediums = m_lMediums;

		bool bIntersect = false;
		CBox3Dfp32 Bnd(_Dest,_Dest);
		Bnd.m_Min -= CVec3Dfp32(_Radius,_Radius,_Radius);
		Bnd.m_Max += CVec3Dfp32(_Radius,_Radius,_Radius);
		//CBSP_OctaAABBIterator<CXR_Model_BSP4,CBSP_PortalLeaf> Itr(this,NULL,Bnd);
		uint32 nHit;
		uint32 liHit[256];
		nHit = BSPOctaAABB_EnumLeaf(this, liHit, 256, Bnd);

		for(int i = 0;i < nHit;i++)
		{
			uint32 iFace = liHit[i];

			// Face Tag skipped since OctaAABB only contains one instance of each face

			CBSP4_Face * pF = m_pFaces + iFace;

			//Cull unused
			if (!(pF->m_Flags & XW_FACE_PHYSICAL) || (pF->m_Flags & XW_FACE_VOLUMEINSIDE)) continue;
			const CXR_MediumDesc& Medium = pMediums[pF->m_iBackMedium];
			if (!(Medium.m_MediumFlags & _MediumFlags)) continue;
			if (!(M_BitD(Medium.m_iPhysGroup) & _pPhysContext->m_PhysGroupMaskThis)) continue;

			if( __Phys_IntersectSphere_CheckFace(iFace, _Origin, _Dest, _Radius, &CInfo) )
			{
				bIntersect = true;
				if (!_pCollisionInfo) return true;

				_pCollisionInfo->Improve(CInfo);
			}
		}

		return bIntersect;
	}

#endif

	CBSP4_Node* pN = &m_pNodes[_iNode];

	if (pN->IsLeaf())
	{
		if (m_pMediums[pN->m_iMedium].m_MediumFlags & _MediumFlags)
			if (!_pCollisionInfo) return true;

		// Leaf
		uint nFaces = pN->m_nFaces;
		uint iiFaces = pN->m_iiFaces;
//ConOut(CStrF("iN %d, nF %d", _iNode, nFaces));
		for(int i = 0; i < nFaces; i++)
		{
			uint iFace = m_piFaces[iiFaces + i];

			// Check medium...
			CBSP4_Face* pF = &m_pFaces[iFace];
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
				const CXR_MediumDesc& Medium = m_pMediums[pF->m_iBackMedium];
				if (!(Medium.m_MediumFlags & _MediumFlags)) continue;
				if (!(M_BitD(Medium.m_iPhysGroup) & _pPhysContext->m_PhysGroupMaskThis)) continue;
			}

			// Do intersection...
			if (__Phys_IntersectSphere_CheckFace(iFace, _Origin, _Dest, _Radius, &CInfo))
			{
				bIntersect = true;
				if (!_pCollisionInfo) return true;

				_pCollisionInfo->Improve(CInfo);
			}
		}

		if (m_pMediums[pN->m_iMedium].m_MediumFlags & _MediumFlags) return true;
		return bIntersect;
	}
	else
	{
		// Node, recurse...
		uint iPlane = m_pNodes[_iNode].m_iPlane;
		if (!iPlane) Error("__Phys_IntersectSphere_r", "Internal error.");
		CPlane3Dfp32* pP = &m_pPlanes[iPlane];
		fp32 d = pP->Distance(_Dest);

		bool bIntersect1 = false;
		bool bIntersect2 = false;
		if (d > -_Radius) 
			if ((bIntersect1 = __Phys_IntersectSphere_r(_pPhysContext, m_pNodes[_iNode].m_iNodeFront, _Origin, _Dest, _Radius, _MediumFlags, _pCollisionInfo)))
				if (!_pCollisionInfo) return true;

		if (d < _Radius) 
			if ((bIntersect2 = __Phys_IntersectSphere_r(_pPhysContext, m_pNodes[_iNode].m_iNodeBack, _Origin, _Dest, _Radius, _MediumFlags, _pCollisionInfo)))
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

bool CXR_Model_BSP4::__Phys_IntersectSphere_Triangle(const CVec3Dfp32& _V0, const CVec3Dfp32& _V1, const CVec3Dfp32& _V2, const CVec3Dfp32& _Pos, fp32 _Radius, CCollisionInfo* _pCollisionInfo) const
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

// -------------------------------------------------------------------
// Physics interface overrides
// -------------------------------------------------------------------
void CXR_Model_BSP4::Phys_GetBound_Sphere(const CMat4Dfp32& _Pos, CVec3Dfp32& _v0, fp32& _Radius)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_GetBound_Sphere, MAUTOSTRIP_VOID);
	_Radius = GetBound_Sphere();
	_v0 = CVec3Dfp32::GetMatrixRow(_Pos, 3);
}

void CXR_Model_BSP4::Phys_GetBound_Box(const CMat4Dfp32& _Pos, CBox3Dfp32& _Box)
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

void CXR_Model_BSP4::Phys_Init(CXR_PhysicsContext* _pPhysContext)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_Init, MAUTOSTRIP_VOID);
}

bool CXR_Model_BSP4::__Phys_TraceLine_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MediumFlags, CPhysLineContext* _pLineContext, CCollisionInfo* _pCollisionInfo) const
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_TraceLine_r, false);
	// Note: _pCollisionInfo must be valid here.


	if (!_iNode) return false;
//	if (!_iNode) return true;
	const CBSP4_Node* pN = &m_pNodes[_iNode];

	bool bHit = false;

	if (pN->IsLeaf())
	{
		uint nf = pN->m_nFaces;
		uint iif = pN->m_iiFaces;
		for(int f = 0; f < nf; f++)
		{
			uint iFace = m_piFaces[iif + f];
			const CBSP4_Face* pF = &m_pFaces[iFace];
			uint iMedium = pF->m_iBackMedium;
			const CXR_MediumDesc& Medium = m_pMediums[iMedium];
			if (!(Medium.m_MediumFlags & _MediumFlags)) continue;
			if (!(M_BitD(Medium.m_iPhysGroup) & _pPhysContext->m_PhysGroupMaskThis)) continue;

			uint nv = pF->m_nVertices;
			uint iiv = pF->m_iiVertices;
			uint iv0 = m_piVertices[iiv + nv - 1];

			const CPlane3Dfp32& P = m_pPlanes[pF->m_iPlane];
//			fp32 d1 = P.Distance(_p0);
//			fp32 d2 = P.Distance(_p1);
//			if (d1 > 1.1f && d2 > 1.1f) continue;
//			if (d1 < -0.1f && d2 < -0.1f) continue;
//			if (d1 < 0.2f) continue;
//			fp32 d2 = P.Distance(_p1);
//			if (d2 > 0.1f) continue;	// If positive, the points must be on the same side.
			fp32 d1 = P.Distance(_p0);
			if (d1 < 0.001f) continue;
			fp32 d2 = P.Distance(_p1);
			if (d2 > -0.001f) continue;	// If positive, the points must be on the same side.

			uint v;
			for(v = 0; v < nv; v++)
			{
				uint iv1 = m_piVertices[iiv + v];
				CVec3Dfp32 Edge, p0v1, Normal;
				m_pVertices[iv1].Sub(m_pVertices[iv0], Edge);
				m_pVertices[iv1].Sub(_p0, p0v1);
				Edge.CrossProd(p0v1, Normal);
				CPlane3Dfp32 Plane(Normal, _p0);
				if (Plane.Distance(_p1) > 0.001f) break;

				iv0 = iv1;
			}

			if (v == nv)
			{
				bHit = true;
				CVec3Dfp32 HitPos;
				fp32 t = -d1/(d2-d1);
				_p0.Lerp(_p1, t, HitPos);
				fp32 Time = _pLineContext->m_V0.Distance(HitPos) * _pLineContext->m_InvV1V0;

				if (_pCollisionInfo->IsImprovement(Time))
				{
					_pCollisionInfo->m_bIsCollision = true;
					_pCollisionInfo->m_bIsValid = true;
					_pCollisionInfo->m_LocalPos = HitPos;
					_pCollisionInfo->m_Time = Time;
					_pCollisionInfo->m_Distance = P.Distance(_pLineContext->m_V1);
					_pCollisionInfo->m_Plane = P;
					_pCollisionInfo->m_pSurface = m_lspSurfaces[pF->m_iSurface];

					if (Time > 1.1f)
						ConOut(CStrF("(__Phys_TraceLine_r) Fuckat T %f", t));

					if (_pCollisionInfo->IsComplete())
						return true;
				}
			}
		}
	}

	if (pN->IsNode())
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

		uint iFront = pN->m_iNodeFront;
		uint iBack = pN->m_iNodeBack;

		if ((Side1 | Side2) == 4)
		{
			bHit |= __Phys_TraceLine_r(_pPhysContext, pN->m_iNodeBack, _p0, _p1, _MediumFlags, _pLineContext, _pCollisionInfo);
			bHit |= __Phys_TraceLine_r(_pPhysContext, pN->m_iNodeFront, _p0, _p1, _MediumFlags, _pLineContext, _pCollisionInfo);
			return bHit;
		}

		if (((Side1 | Side2) & 3) == 3)
		{
			if (!(iFront || iBack)) return bHit;

			fp32 s = d2 - d1;
			if (s == 0.0f) return bHit;	// Impossible.
			fp32 vscale = -d1/s;

			CVec3Dfp32 v, CutPoint0, CutPoint1;
			_p1.Sub(_p0, v);
			_p0.Combine(v, vscale+0.01f, CutPoint0);	// Make sure the new lines still intersects the plane
			_p0.Combine(v, vscale-0.01f, CutPoint1);	// since the lines must still be intersected to the actual polygons.

			if (Side1 & 2)
			{
				bHit |= __Phys_TraceLine_r(_pPhysContext, iBack, _p0, CutPoint0, _MediumFlags, _pLineContext, _pCollisionInfo);
				bHit |= __Phys_TraceLine_r(_pPhysContext, iFront, CutPoint1, _p1, _MediumFlags, _pLineContext, _pCollisionInfo);
				return bHit;
			}
			else
			{
				bHit |= __Phys_TraceLine_r(_pPhysContext, iFront, _p0, CutPoint0, _MediumFlags, _pLineContext, _pCollisionInfo);
				bHit |= __Phys_TraceLine_r(_pPhysContext, iBack, CutPoint1, _p1, _MediumFlags, _pLineContext, _pCollisionInfo);
				return bHit;
			}
		}
		else
			if ((Side1 & 1) || (Side2 & 1))
				bHit |= __Phys_TraceLine_r(_pPhysContext, iFront, _p0, _p1, _MediumFlags, _pLineContext, _pCollisionInfo);
			else
				bHit |= __Phys_TraceLine_r(_pPhysContext, iBack, _p0, _p1, _MediumFlags, _pLineContext, _pCollisionInfo);

	}
	return bHit;
}

bool CXR_Model_BSP4::__Phys_TraceLine_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MediumFlags, CPhysLineContext* _pLineContext) const
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_TraceLine_r_2, false);
	MSCOPESHORT(CXR_Model_BSP4::__Phys_TraceLine_r_2);

	if (!_iNode)
		return true;
	const CBSP4_Node* pN = &m_pNodes[_iNode];

	if (pN->IsLeaf())
	{
		return (m_pMediums[pN->m_iMedium].m_MediumFlags & _MediumFlags) != 0;
	}

	if (pN->IsNode())
	{
		const CPlane3Dfp32* pP = &m_pPlanes[pN->m_iPlane];
		fp32 d1 = pP->Distance(_p0);
		fp32 d2 = pP->Distance(_p1);

		// Mask1
		uint Side1, Side2;
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

		uint Side = Side1 | Side2;
		if (Side == 4)
			return 
				__Phys_TraceLine_r(_pPhysContext, pN->m_iNodeFront, _p0, _p1, _MediumFlags, _pLineContext) ||
				__Phys_TraceLine_r(_pPhysContext, pN->m_iNodeBack, _p0, _p1, _MediumFlags, _pLineContext);

		if ((Side & 3) == 1)
			return __Phys_TraceLine_r(_pPhysContext, pN->m_iNodeFront, _p0, _p1, _MediumFlags, _pLineContext);
		if ((Side & 3) == 2)
			return __Phys_TraceLine_r(_pPhysContext, pN->m_iNodeBack, _p0, _p1, _MediumFlags, _pLineContext);

		fp32 s = d2 - d1;
		if (s == 0.0f) return FALSE;	// Impossible.
		CVec3Dfp32 CutPoint0;
		_p0.Lerp(_p1, -d1/s, CutPoint0);

		if (Side1 & 1)
		{
			return 
				__Phys_TraceLine_r(_pPhysContext, pN->m_iNodeFront, _p0, CutPoint0, _MediumFlags, _pLineContext) ||
				__Phys_TraceLine_r(_pPhysContext, pN->m_iNodeBack, CutPoint0, _p1, _MediumFlags, _pLineContext);
		}
		else
		{
			return 
				__Phys_TraceLine_r(_pPhysContext, pN->m_iNodeBack, _p0, CutPoint0, _MediumFlags, _pLineContext) ||
				__Phys_TraceLine_r(_pPhysContext, pN->m_iNodeFront, CutPoint0, _p1, _MediumFlags, _pLineContext);
		}
	}

	return false;
}

bool CXR_Model_BSP4::Phys_IntersectLine(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_IntersectLine, false);
	MSCOPESHORT(CXR_Model_BSP4::Phys_IntersectLine);

	CVec3Dfp32 v0, v1;
	_v0.MultiplyMatrix(_pPhysContext->m_WMatInv, v0);
	_v1.MultiplyMatrix(_pPhysContext->m_WMatInv, v1);

	CPhysLineContext LineContext;
	LineContext.m_V0 = v0;
	LineContext.m_V1 = v1;
	LineContext.m_InvV1V0 = 1.0f * v1.DistanceInv(v0);

	if (_pCollisionInfo && (_pCollisionInfo->m_ReturnValues & CXR_COLLISIONRETURNVALUE_TIME))
	{
		__Phys_TraceLine_r(_pPhysContext, 1, v0, v1, _MediumFlags, &LineContext, _pCollisionInfo);
		if (_pCollisionInfo->IsComplete())
			return true;

		if (_pCollisionInfo->m_bIsValid)
		{
			_pCollisionInfo->m_Plane.Transform(_pPhysContext->m_WMat);
			_pCollisionInfo->m_Pos = _pCollisionInfo->m_LocalPos;
			_pCollisionInfo->m_Distance = _pCollisionInfo->m_Plane.Distance(_v1);
			_pCollisionInfo->m_Time = v0.Distance(_pCollisionInfo->m_Pos) / v0.Distance(v1);
			_pCollisionInfo->m_Pos *= _pPhysContext->m_WMat;

			return true;
		}
		return false;
	}
	else
	{
		if (__Phys_TraceLine_r(_pPhysContext, 1, v0, v1, _MediumFlags, &LineContext))
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

bool CXR_Model_BSP4::Phys_IntersectSphere(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_IntersectSphere, false);

	uint MediumDest = Phys_GetMedium(_pPhysContext, _Dest);
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

	CVec3Dfp32 v0, v1;
	_Origin.MultiplyMatrix(_pPhysContext->m_WMatInv, v0);
	_Dest.MultiplyMatrix(_pPhysContext->m_WMatInv, v1);
	bool bImpact = __Phys_IntersectSphere_r(_pPhysContext, 1, v0, v1, _Radius, _MediumFlags, _pCollisionInfo);

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

//		if (_pPhysContext->m_pWC)
//			_pPhysContext->m_pWC->RenderVector(_pCollisionInfo->m_Pos, _pCollisionInfo->m_Plane.n, 0xff006060, 0.5f, true);
	}

	return bImpact;
}

bool CXR_Model_BSP4::__Phys_IntersectOBB_CheckFace(int _iFace, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, CCollisionInfo* _pCollisionInfo) const
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_IntersectOBB_CheckFace, false);
	CBSP4_Face* pF = &m_pFaces[_iFace];
	uint nv = pF->m_nVertices;
	uint iiv = pF->m_iiVertices;

	CVec3Dfp32 Tri[3];

	CCollisionInfo CInfo;
	bool bIntersect = false;

	Tri[0] = m_pVertices[m_piVertices[iiv]];
	for(uint v = 2; v < nv; v++)
	{
		Tri[1] = m_pVertices[m_piVertices[iiv + v -1]];
		Tri[2] = m_pVertices[m_piVertices[iiv + v]];

		CInfo.m_bIsCollision = false;
		if (Phys_Intersect_TriOBB(Tri, _BoxStart, _Box, false, (_pCollisionInfo) ? &CInfo : NULL) )
		{
			if (!_pCollisionInfo)
				return true;
			bIntersect = true;
			CInfo.m_pSurface = m_lspSurfaces[pF->m_iSurface];

			if (_pCollisionInfo->Improve(CInfo))
				if (_pCollisionInfo->IsComplete())
					return true;
		}
	}

	return bIntersect;
}

#define ENUMFACES

bool CXR_Model_BSP4::__Phys_IntersectOBB_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, const CBox3Dfp32 _Bound, int _MediumFlags, CCollisionInfo* _pCollisionInfo) const
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_IntersectOBB_r, false);
	MSCOPESHORT(CXR_Model_BSP4::__Phys_IntersectOBB_r); //AR-SCOPE

#ifdef ENUMFACES
	const int MaxFaces = 2048;
	uint32 liFaces[MaxFaces];

#ifndef M_RTM
	CWireContainer* pOldWC = _pPhysContext->m_pWC;
	if (!_pCollisionInfo)
		_pPhysContext->m_pWC = NULL;
#endif

	TObjectPoolAllocator<CBSP4_EnumContext> AllocObject(m_spEnumContextPool);
	CBSP4_EnumContext* pEnumContext = AllocObject.GetObject();
	pEnumContext->Create(m_lFaces.Len(), ENUM_HQ | ENUM_FACEFLAGS | ENUM_MEDIUMFLAGS, _MediumFlags, XW_FACE_PHYSICAL);
	pEnumContext->SetupFaceEnum(liFaces, MaxFaces);
	pEnumContext->m_EnumBox	= _Bound;
	int nFaces = EnumFaces_Box(_pPhysContext, pEnumContext, _iNode);
	pEnumContext->Untag();
#ifndef M_RTM
	_pPhysContext->m_pWC = pOldWC;
#endif

	if (nFaces)
	{
		bool bIntersect = false;
		CCollisionInfo CInfo;
		for(int i = 0; i < nFaces; i++)
		{
			uint iFace = liFaces[i];

			if (_pPhysContext->m_pWC && _pCollisionInfo) 
				__Phys_RenderFace(iFace, _pPhysContext->m_WMat, _pPhysContext->m_pWC, 0xff006060);

			CInfo.m_bIsCollision = false;
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

	return false;

#else

	if (!_iNode)
	{
		return false;
	}

	CBSP4_Node* pN = &m_pNodes[_iNode];
	CCollisionInfo CInfo;
	bool bIntersect = false;

	if (pN->IsLeaf())
	{
		uint nf = pN->m_nFaces;
		uint iif = pN->m_iiFaces;
//ConOut(CStrF("LEAF %d, Faces %d", _iNode, nf ));
//CStr s;
		for(uint i = 0; i < nf; i++)
		{
			uint iFace = m_piFaces[iif + i];
			const CBSP4_Face* pF = &m_pFaces[iFace];
			if (!(pF->m_Flags & XW_FACE_PHYSICAL)) continue;
			if (pF->m_Flags & XW_FACE_VOLUMEINSIDE) continue;
			const CXR_MediumDesc& Medium = m_pMediums[pF->m_iBackMedium];
			if (!(Medium.m_MediumFlags & _MediumFlags)) continue;
			if (!(M_Bit(Medium.m_iPhysGroup) & _pPhysContext->m_PhysGroupMaskThis)) continue;

			uint nv = pF->m_nPhysV;
			uint iiv = pF->m_iiPhysV;

			// Get bound-box for polygon.
			CBox3Dfp32 PBox;
			PBox.m_Min = _FP32_MAX;
			PBox.m_Max = -_FP32_MAX;
			for(int v = 0; v < nv; v++)
			{
				const CVec3Dfp32& V = m_pVertices[m_piVertices[iiv + v]];
				for(int k = 0; k < 3; k++)
				{
					if (V.k[k] < PBox.m_Min.k[k]) PBox.m_Min.k[k] = V.k[k];
					if (V.k[k] > PBox.m_Max.k[k]) PBox.m_Max.k[k] = V.k[k];
				}
			}

			// Intersection?
			if (!PBox.IsInside(_Bound)) continue;

//	s += CStrF("%d, ", iFace);
//	ConOutL(CStrF("          tested %d, P %s, Pos %s", iFace, (char*)CInfo.m_Plane.GetString(), (char*)CInfo.m_LocalPos.GetString() ));

			CInfo.m_bIsCollision = false;
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
	
		return bIntersect;
	}
	else
	{
		// Node, recurse...
		uint iPlane = m_pNodes[_iNode].m_iPlane;
		if (!iPlane) Error("__Phys_IntersectSphere_r", "Internal error.");
		CPlane3Dfp32* pP = &m_pPlanes[iPlane];
		uint Side = pP->GetBoxPlaneSideMask(_Bound.m_Min, _Bound.m_Max);

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

#ifdef PLATFORM_XBOX1
	void SSE_Box3Dfp32_GetMinMax(CBox3Dfp32& _Target, const CVec3Dfp32* _pSrc, const uint16* _piV, int _nV);


bool SSE_Box3Dfp32_IntersectingMinMax(const CBox3Dfp32& _Bound, const CVec3Dfp32* _pSrc, const uint16* _piV, int _nV)
{
	bool bRet;
	__asm
	{
		mov edi, [_Bound]
		mov esi, [_pSrc]
		mov edx, [_piV]
		mov ecx, [_nV]

		// Load first vertex

		movzx eax, word ptr [edx]
		lea eax, [eax*2+eax]
		lea edx, [edx+2]
		movlps xmm0, qword ptr [esi+eax*4]
		movss xmm1, [esi+eax*4+8]
		dec ecx
		shufps xmm0, xmm1, 0+4
		movaps xmm1, xmm0
		jz noloop

lp:
		// Load vector: xmm2 = _Src
		movzx eax, word ptr [edx]
		lea eax, [eax*2+eax]
		lea edx, [edx+2]
		movlps xmm2, qword ptr [esi+eax*4]
		movss xmm3, [esi+eax*4+8]
		shufps xmm2, xmm3, 0+4

		dec ecx
		minps xmm0, xmm2
		maxps xmm1, xmm2

		jnz lp
noloop:

		// Load source box: xmm2 = m_Min, xmm3 = m_Max
		movlps xmm2, qword ptr [edi]
		movss xmm4, [edi+8]
		shufps xmm2, xmm4, 0+4

		movlps xmm3, qword ptr [edi+12]
		movss xmm4, [edi+12+8]
		shufps xmm3, xmm4, 0+4

		cmpltps xmm1, xmm2
		cmpltps xmm3, xmm0
		movmskps eax, xmm1
		movmskps ebx, xmm3
		or eax, ebx
		mov ecx, 1
		xor ebx, ebx
		cmp eax, 0
		cmove ebx, ecx
		mov bRet, bl
	}
	return bRet;
}

#endif


bool CXR_Model_BSP4::__Phys_IntersectOBB_i(CXR_PhysicsContext* _pPhysContext, int _iNode, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, const CBox3Dfp32& _Bound, int _MediumFlags, CCollisionInfo* _pCollisionInfo) 
{
	MAUTOSTRIP(CXR_Model_BSP4___Phys_IntersectOBB_i, false);
	MSCOPESHORT(CXR_Model_BSP4::__Phys_IntersectOBB_i);
/*
	CMTime OctTm;
	CMTime NormTm;

	OctTm.Reset();
	NormTm.Reset();
*/
#ifdef BSP_USE_OCTAAABB

	if( m_lOctaAABBNodes.Len() )
	{

		// OctTm.Start();
		TAP_RCD<const CXR_MediumDesc> pMediums = m_lMediums;

		bool bIntersect = false;
		uint32 nHit;
		uint32 liHit[256];
		nHit = BSPOctaAABB_EnumLeaf(this, liHit, 256, _Bound);
		//CBSP_OctaAABBIterator<CXR_Model_BSP4,CBSP_PortalLeaf> Itr(this,NULL,_Bound);

		for(int i = 0;i < nHit;i++)
		{
			uint32 iFace = liHit[i];

			// Face Tag skipped since OctaAABB only contains one instance of each face

			CBSP4_Face * pF = m_pFaces + iFace;

			//Cull unused
			if (!(pF->m_Flags & XW_FACE_PHYSICAL) || (pF->m_Flags & XW_FACE_VOLUMEINSIDE)) continue;
			const CXR_MediumDesc& Medium = pMediums[pF->m_iBackMedium];
			if (!(Medium.m_MediumFlags & _MediumFlags)) continue;
			if (!(M_BitD(Medium.m_iPhysGroup) & _pPhysContext->m_PhysGroupMaskThis)) continue;

			//Cull plane
			const CPlane3Dfp32& PolyPlane = m_pPlanes[pF->m_iPlane];
			fp32 MinDist = PolyPlane.GetBoxMinDistance(_Bound.m_Min, _Bound.m_Max);
			if (MinDist > MODEL_BSP_EPSILON) continue;

			uint32 nv = pF->m_nVertices;
			uint32 iiv = pF->m_iiVertices;
			const uint32* piVert = m_piVertices + iiv;

			if( _pCollisionInfo )
			{
				CCollisionInfo CInfo;

				if (_pPhysContext->m_pWC) 
					__Phys_RenderFace(iFace, _pPhysContext->m_WMat, _pPhysContext->m_pWC, 0xff00ff00);

				if( Phys_Intersect_PolyOBB( m_pVertices, piVert, nv, PolyPlane, _BoxStart, _Box, false, &CInfo ) )
				{
					if (_pPhysContext->m_pWC) 
						__Phys_RenderFace(iFace, _pPhysContext->m_WMat, _pPhysContext->m_pWC, 0xffffffff);

					CInfo.m_pSurface	= m_lspSurfaces[pF->m_iSurface];
					bIntersect = true;
					_pCollisionInfo->Improve(CInfo);
				}
			}
			else
			{
				if( Phys_Intersect_PolyOBB( m_pVertices, piVert, nv, PolyPlane, _BoxStart, _Box, false, NULL ) )
				{
					return true;
				}
			}
		}

		return bIntersect;
	}

#endif

	// NormTm.Start();

	if (!_iNode)
	{
		return false;
	}

	bool bIntersect = false;
	uint32 aWorkingSet[128];
	uint WorkingStack = 0;
	aWorkingSet[WorkingStack++] = _iNode;

	TObjectPoolAllocator<CBSP4_EnumContext> EnumAlloc(m_spEnumContextPool);
	CBSP4_EnumContext* pEnumContext = EnumAlloc.GetObject();
	pEnumContext->Create(m_lFaces.Len(), 0);

StartOf_Phys_IntersectOBB_i:
	uint32 WorkingNode = aWorkingSet[--WorkingStack];

StartOf_Phys_IntersectOBB_i_NoAdd:
	const CBSP4_Node* pN = &m_pNodes[WorkingNode];

	if (pN->IsLeaf())
	{
		uint nf = pN->m_nFaces;
		uint iif = pN->m_iiFaces;

		const uint32* piFaces = &m_piFaces[iif];
		for(uint i = 0; i < nf; i++)
		{
			int iFace = piFaces[i];
			{
				uint32 iFaceIdx = iFace >> 3;
				uint8 iFaceMask = aShiftMulTab[iFace & 7];
				if (pEnumContext->m_pFTag[iFaceIdx] & iFaceMask) continue;
				if(!pEnumContext->m_pFTag[iFaceIdx])
					pEnumContext->m_pFUntag[pEnumContext->m_nUntagEnum++]	= iFaceIdx;
				pEnumContext->m_pFTag[iFaceIdx] |= iFaceMask;
			}
			const CBSP4_Face* pF = &m_pFaces[iFace];

			if (_pPhysContext->m_pWC) 
				__Phys_RenderFace(iFace, _pPhysContext->m_WMat, _pPhysContext->m_pWC, 0xffff0000);

			if (!(pF->m_Flags & XW_FACE_PHYSICAL) || (pF->m_Flags & XW_FACE_VOLUMEINSIDE)) continue;
			const CXR_MediumDesc& Medium = m_pMediums[pF->m_iBackMedium];
			if (!(Medium.m_MediumFlags & _MediumFlags)) continue;
			if (!(M_BitD(Medium.m_iPhysGroup) & _pPhysContext->m_PhysGroupMaskThis)) continue;

			const CPlane3Dfp32& PolyPlane = m_pPlanes[pF->m_iPlane];
			fp32 MinDist = PolyPlane.GetBoxMinDistance(_Bound.m_Min, _Bound.m_Max);
			if (MinDist > MODEL_BSP_EPSILON) continue;

			uint32 nv = pF->m_nVertices;
			uint32 iiv = pF->m_iiVertices;

			// Get bound-box for polygon.
			const uint32* piVert = m_piVertices + iiv;
			CBox3Dfp32 PBox;

#if 0 && defined PLATFORM_XBOX
			if (!SSE_Box3Dfp32_IntersectingMinMax(_Bound, m_pVertices, piVert, nv))
				continue;

//			SSE_Box3Dfp32_GetMinMax(PBox, m_pVertices, piVert, nv);
#else
			PBox.m_Min = m_pVertices[piVert[0]];
			PBox.m_Max = m_pVertices[piVert[0]];
			
			for( uint32 v = 1; v < nv; v++ )
			{
				const CVec3Dfp32& V = m_pVertices[piVert[v]];
				PBox.m_Min.k[0] = Min( PBox.m_Min.k[0], V.k[0] );
				PBox.m_Max.k[0] = Max( PBox.m_Max.k[0], V.k[0] );
				PBox.m_Min.k[1] = Min( PBox.m_Min.k[1], V.k[1] );
				PBox.m_Max.k[1] = Max( PBox.m_Max.k[1], V.k[1] );
				PBox.m_Min.k[2] = Min( PBox.m_Min.k[2], V.k[2] );
				PBox.m_Max.k[2] = Max( PBox.m_Max.k[2], V.k[2] );
			}
			if( _Bound.m_Min.k[0] > PBox.m_Max.k[0] ||
				_Bound.m_Min.k[1] > PBox.m_Max.k[1] ||
				_Bound.m_Min.k[2] > PBox.m_Max.k[2] ||
				_Bound.m_Max.k[0] < PBox.m_Min.k[0] ||
				_Bound.m_Max.k[1] < PBox.m_Min.k[1] ||
				_Bound.m_Max.k[2] < PBox.m_Min.k[2] )
				continue;
#endif

			if( _pCollisionInfo )
			{
				CCollisionInfo CInfo;
			//	NormTm.Stop();

				if (_pPhysContext->m_pWC) 
					__Phys_RenderFace(iFace, _pPhysContext->m_WMat, _pPhysContext->m_pWC, 0xff00ff00);

				if( Phys_Intersect_PolyOBB( m_pVertices, piVert, nv, PolyPlane, _BoxStart, _Box, false, &CInfo ) )
				{
					if (_pPhysContext->m_pWC) 
						__Phys_RenderFace(iFace, _pPhysContext->m_WMat, _pPhysContext->m_pWC, 0xffffffff);

					CInfo.m_pSurface	= m_lspSurfaces[pF->m_iSurface];
					bIntersect = true;
					_pCollisionInfo->Improve(CInfo);
				}
			//	NormTm.Start();
			}
			else
			{
				if( Phys_Intersect_PolyOBB( m_pVertices, piVert, nv, PolyPlane, _BoxStart, _Box, false, NULL ) )
				{
					pEnumContext->Untag();
					return true;
				}
			}
		}
	}
	else if( pN->IsNode() )
	{
		// Node, recurse...
		const CPlane3Dfp32* pP = &m_pPlanes[pN->m_iPlane];
		uint Side = pP->GetBoxPlaneSideMask(_Bound.m_Min, _Bound.m_Max);

		uint32 iFront = pN->m_iNodeFront;
		uint32 iBack = pN->m_iNodeBack;
		if( Side == 1 )
		{
			if( iFront )
			{
				WorkingNode = iFront;
				goto StartOf_Phys_IntersectOBB_i_NoAdd;
			}
		}
		else if( Side == 2 )
		{
			if( iBack )
			{
				WorkingNode = iBack;
				goto StartOf_Phys_IntersectOBB_i_NoAdd;
			}
		}
		else
		{
			if( iFront && iBack )
			{
				if( iFront )
					aWorkingSet[WorkingStack++]	= iFront;

				if( iBack )
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

	pEnumContext->Untag();
/*
	NormTm.Stop();
	m_OctTotal += OctTm.GetTimefp64();
	m_NormTotal += NormTm.GetTimefp64();
	m_nOctTests++;
	if( m_nOctTests >= 1000 )
	{
#ifdef BSP_USE_OCTAAABB
		if( m_lOctaAABBNodes.Len() )
			ConOut(CStrF("[Using OctaAABB] Avg time for 1000 nodes: %0.7f",(m_OctTotal / m_nOctTests)).Str());
#endif
		ConOut(CStrF("[Using BSP] Avg time for 1000 nodes: %0.7f",(m_NormTotal / m_nOctTests)).Str());

		m_nOctTests = 0;
		m_OctTotal = 0;
		m_NormTotal = 0;
	}
*/

	return bIntersect;
}

void CXR_Model_BSP4::CollectPCS(CXR_PhysicsContext* _pPhysContext, const uint8 _IN1N2Flags, CPotColSet *_pcs, const int _iObj, const int _MediumFlags )
{
	MSCOPESHORT(CXR_Model_BSP4::CollectPCS);
	CBox3Dfp32 _Box; 	//oh this is just soo stupid!
	CBox3Dfp32 TransformedBox;
	CBoxInterleaved TransformedBoxInterleaved;
	CVec3Dfp32 TransformedCenter;

	_pcs->GetBox( _Box.m_Min.k ); // and this is a very ugly way to set the bloody template box

	// Transform PCS into BSP local space
	TransformedCenter = *(CVec3Dfp32*)_pcs->m_fCenter;
	TransformedCenter.MultiplyMatrix(_pPhysContext->m_WMatInv);
	_Box.Transform(_pPhysContext->m_WMatInv, TransformedBox);
	TransformedBoxInterleaved.Init(TransformedBox);

	TObjectPoolAllocator<CBSP4_EnumContext> EnumAlloc(m_spEnumContextPool);
	CBSP4_EnumContext* pEnumContext = EnumAlloc.GetObject();
	pEnumContext->Create(m_lFaces.Len(), ENUM_HQ | ENUM_FACEFLAGS | ENUM_MEDIUMFLAGS, _MediumFlags, XW_FACE_PHYSICAL);
	pEnumContext->SetupPCSEnum(_pcs, _iObj, _IN1N2Flags, TransformedCenter, &TransformedBox, &TransformedBoxInterleaved);

	CollectPCS_i(_pPhysContext, pEnumContext, 1);
	pEnumContext->Untag();

#ifdef PCSCHECK_INSOLID
	//AR-TEMP-ADD: Get medium in middle of box
	CVec3Dfp32 Center;
	_Box.m_Min.Lerp(_Box.m_Max, 0.5f, Center);
	int Medium = CXR_Model_BSP4::Phys_GetMedium(_pPhysContext, Center);
	if (Medium & _MediumFlags)
		_pcs->m_bIsInSolid = true;
#endif
}


void CXR_Model_BSP4::CollectPCS_r(CXR_PhysicsContext* _pPhysContext, CBSP4_EnumContext* _pEnumContext, int _iNode) const
{
	if (!_iNode)
	{ // empty tree
		return;
	}
	
	const CBSP4_Node* pN = &m_pNodes[_iNode];

//	const CMat4Dfp32 &T = _pPhysContext->m_WMat; // syntaktiskt socker!

	
	if (pN->IsLeaf())
	{
		CollectPCS_IsLeaf(_pPhysContext, _pEnumContext, pN);
	}

	if (pN->IsNode())
	{
		const CPlane3Dfp32& W = m_pPlanes[pN->m_iPlane];
		const fp32 d = W.Distance(_pEnumContext->m_TransformedCenter);
		const fp32 R = _pEnumContext->m_pPCS->m_fRadius;

		if (d > R)
		{ // positive side of partition plane
			CollectPCS_r(_pPhysContext, _pEnumContext, pN->m_iNodeFront);
			return;
		}	
		else if (d < -R)
		{ // negative side of partition plane
			CollectPCS_r(_pPhysContext, _pEnumContext, pN->m_iNodeBack);
			return;
		}

		// bound-sphere is split by plane - better check with the box..
		fp32 MinDist, MaxDist;
		GetBoxMinMaxDistance(W, *_pEnumContext->m_pTransformedBoxInterleaved, MinDist, MaxDist);

		if (MinDist < MODEL_BSP_EPSILON)
			CollectPCS_r(_pPhysContext, _pEnumContext, pN->m_iNodeBack);

		if (MaxDist > -MODEL_BSP_EPSILON)
			CollectPCS_r(_pPhysContext, _pEnumContext, pN->m_iNodeFront);
	}
}

void CXR_Model_BSP4::CollectPCS_i(CXR_PhysicsContext* _pPhysContext, CBSP4_EnumContext* _pEnumContext, int _iNode) const
{
	MSCOPESHORT( CXR_Model_BSP4::CollectPCS_i );
	if (!_iNode)
	{ // empty tree
		return;
	}

//	const CMat4Dfp32 &T = _pPhysContext->m_WMat; // syntaktiskt socker!
	const CVec3Dfp32& C = _pEnumContext->m_TransformedCenter;
	const CBoxInterleaved& B = *_pEnumContext->m_pTransformedBoxInterleaved;
	const fp32 Radius = _pEnumContext->m_pPCS->m_fRadius;

	uint aWorkingStack[256];
	uint iWorkPos = 0;
	aWorkingStack[iWorkPos++]	= _iNode;
StartOf_CollectPCS_i:
	uint iWorkNode = aWorkingStack[--iWorkPos];

StartOf_CollectPCS_i_NoAdd:
	const CBSP4_Node* pN = &m_pNodes[iWorkNode];

	if (pN->IsLeaf())
	{
		CollectPCS_IsLeaf(_pPhysContext, _pEnumContext, pN);
	}
	else if (pN->IsNode())
	{
		const CPlane3Dfp32& W = m_pPlanes[pN->m_iPlane];
		const fp32 d = W.Distance(C);

		uint iBack = pN->m_iNodeBack;
		uint iFront = pN->m_iNodeFront;

		if (d > Radius)
		{ // positive side of partition plane
			if( iFront )
			{
				iWorkNode	= iFront;
				goto StartOf_CollectPCS_i_NoAdd;
			}
		}	
		else if (d < -Radius)
		{ // negative side of partition plane
			if( iBack )
			{
				iWorkNode	= iBack;
				goto StartOf_CollectPCS_i_NoAdd;
			}
		}
		else
		{
			// bound-sphere is split by plane - better check with the box..
			fp32 MinDist, MaxDist;
			GetBoxMinMaxDistance(W, B, MinDist, MaxDist);

			if (iBack && (MinDist < MODEL_BSP_EPSILON))
			{
				aWorkingStack[iWorkPos++]	= iBack;
			}

			if (iFront && (MaxDist > -MODEL_BSP_EPSILON))
			{
				iWorkNode	= iFront;
				goto StartOf_CollectPCS_i_NoAdd;
			}
		}
	}

	if( iWorkPos > 0 )
		goto StartOf_CollectPCS_i;
}


void CXR_Model_BSP4::CollectPCS_IsLeaf(CXR_PhysicsContext* _pPhysContext, CBSP4_EnumContext* _pEnumContext, const CBSP4_Node* _pNode) const
{
	MSCOPESHORT(CXR_Model_BSP4::CollectPCS_IsLeaf);

	uint nFaces = _pNode->m_nFaces;
	uint iiFaces = _pNode->m_iiFaces;

	const uint32* piFaces = &m_piFaces[iiFaces];

	for (uint f = 0; f < nFaces; f++)
	{
		uint iFace = piFaces[f];
		
		uint iFaceByte = iFace >> 3;
		uint iFaceMask = aShiftMulTab[iFace & 7];
		if (_pEnumContext->m_pFTag[iFaceByte] & iFaceMask) // check if already processed
			continue;
		if(!_pEnumContext->m_pFTag[iFaceByte]) _pEnumContext->m_pFUntag[_pEnumContext->m_nUntagEnum++] = iFaceByte;
			
		_pEnumContext->m_pFTag[iFaceByte] |= iFaceMask;	// tag face as processed

		const CBSP4_Face* const pF = &m_pFaces[iFace];

		if ((_pEnumContext->m_EnumQuality & ENUM_FACEFLAGS) && 
			!(_pEnumContext->m_EnumFaceFlags & pF->m_Flags))
			continue;

		const CXR_MediumDesc& Medium = m_pMediums[pF->m_iBackMedium];
		if (!(aShiftMulTab[Medium.m_iPhysGroup] & _pPhysContext->m_PhysGroupMaskThis))
			continue;

		if ((_pEnumContext->m_EnumQuality & ENUM_MEDIUMFLAGS) && 
			!(Medium.m_MediumFlags & _pEnumContext->m_EnumMedium))
			continue;

		uint iPlane = pF->m_iPlane;
		const CPlane3Dfp32& WOrig = m_pPlanes[iPlane];

		fp32 MinDist = WOrig.GetBoxMinDistance(_pEnumContext->m_pTransformedBox->m_Min, _pEnumContext->m_pTransformedBox->m_Max);
		if (MinDist > MODEL_BSP_EPSILON)
			continue;

		const int nv = pF->m_nVertices;
		const uint32* piVerts = m_piVertices + pF->m_iiVertices;

		// check face bound min/max
		if(_pEnumContext->m_EnumQuality & ENUM_HQ)
		{
			for (uint k = 0; k < 3; k++)
			{
				fp32 PMin(_FP32_MAX), PMax(-_FP32_MAX);
				for (int v = 0; v < nv; v++)
				{
					const CVec3Dfp32& V = m_pVertices[piVerts[v]];
					PMin = Min(PMin, V.k[k]);
					PMax = Max(PMax, V.k[k]);
				}

				if (_pEnumContext->m_pTransformedBox->m_Min.k[k] > PMax) goto Continue;
				if (_pEnumContext->m_pTransformedBox->m_Max.k[k] < PMin) goto Continue;
			}
		}

		// Add faces
		{
			const CMat4Dfp32& T = _pPhysContext->m_WMat;
			CPlane3Dfp32 W = WOrig;
			W.Transform(T);

			CVec3Dfp32 tri[3];
			tri[0] = m_pVertices[piVerts[0]] * T;
			tri[1] = m_pVertices[piVerts[1]] * T;

			for (uint v = 2; v < nv; v++)
			{ // extract all triangles and add them to PCS
				tri[2] = m_pVertices[piVerts[v]] * T;

				if (_pEnumContext->m_pPCS->ValidFace((const float *)&W, (const float *)tri))
					_pEnumContext->m_pPCS->AddFace( _pEnumContext->m_bIN1N2Flags, _pEnumContext->m_iObj, (const float *)&W, (const float *)tri, m_lspSurfaces[pF->m_iSurface] );

				tri[1] = tri[2];
			}
		}				

Continue:
		continue;
	}
}


bool CXR_Model_BSP4::Phys_IntersectBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _BoxOrigin, const CPhysOBB& _BoxDest, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_IntersectBox, false);
	MSCOPESHORT(CXR_Model_BSP4::Phys_IntersectBox); //AR-SCOPE

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


	if (_pPhysContext->m_pWC)
	{
		_pPhysContext->m_pWC->RenderAABB(_pPhysContext->m_WMat, BoundBoxOrigin, 0xff3030ff, 0.0f, false);

		CBox3Dfp32 Box;
		GetBound_Box(Box);

		_pPhysContext->m_pWC->RenderAABB(_pPhysContext->m_WMat, Box, 0xff300000, 0.0f, false);
	}


	uint MediumDest = Phys_GetMedium(_pPhysContext, _BoxDest.m_C);
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
		bool bImpact = __Phys_IntersectOBB_i(_pPhysContext, 1, LBoxOrigin, LBoxDest, BoundBoxOrigin, _MediumFlags, _pCollisionInfo);
		


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

		if(__Phys_IntersectOBB_i(_pPhysContext, 1, LBoxOrigin, LBoxDest, BoundBoxOrigin, _MediumFlags, _pCollisionInfo))
			return true;
	}

	return false;
}

int CXR_Model_BSP4::Phys_CollideBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _Box, int _MediumFlags, CCollisionInfo* _pCollisionInfo, int _nMaxCollisions)
{
	CPhysOBB LBox;
	_Box.Transform(_pPhysContext->m_WMatInv, LBox);
	
	CBox3Dfp32 BoundBox;
	LBox.GetBoundBox(BoundBox);
	BoundBox.m_Min -= CVec3Dfp32(1);
	BoundBox.m_Max += CVec3Dfp32(1);

	return __Phys_CollideBox_i(_pPhysContext,1,LBox,BoundBox,_MediumFlags,_pCollisionInfo,_nMaxCollisions);
}

int CXR_Model_BSP4::Phys_CollideCapsule(CXR_PhysicsContext* _pPhysContext, const CPhysCapsule& _Capsule,int _MediumFlags, CCollisionInfo* _pCollisionInfo, int _nMaxCollisions)
{
	CPhysCapsule LCap;
	_Capsule.Transform(_pPhysContext->m_WMatInv, LCap);

	CBox3Dfp32 BoundBox;
	LCap.GetBoundBox(BoundBox);
	BoundBox.m_Min -= CVec3Dfp32(1);
	BoundBox.m_Max += CVec3Dfp32(1);

	return __Phys_CollideCapsule_i(_pPhysContext,1,LCap,BoundBox,_MediumFlags,_pCollisionInfo,_nMaxCollisions);
}

int CXR_Model_BSP4::Phys_CollideBSP2(CXR_PhysicsContext* _pPhysContext, class CXR_IndexedSolidContainer32 *_pSolidContainer, const CMat4Dfp32& _BSP2Transform, const CVec3Dfp32& _Offset, int _MediumFlags, CCollisionInfo* _pCollisionInfo, int _nMaxCollisions)
{
	//CXR_IndexedSolidContainer32 *pSolidContainer = _pBSP2->m_spIndexedSolids;
	if (_pSolidContainer == NULL) return 0;

	TAP<const CXR_IndexedSolid32> pSolids = _pSolidContainer->m_lSolids;
	TAP<const CXR_MediumDesc> pMediums = _pSolidContainer->m_lMediums;

	// Räknas även ut i nästa metod...
	CMat4Dfp32 Transform;
	_BSP2Transform.Multiply(_pPhysContext->m_WMatInv, Transform);

	uint nTotCollisions = 0;
	for (uint i = 0; i < pSolids.Len(); i++)
	{
		uint iMedium = pSolids[i].m_iMedium;
		const CXR_MediumDesc& MediumDesc = pMediums[iMedium];

		if (!(MediumDesc.m_MediumFlags & _MediumFlags)) continue;
		if (!(_pPhysContext->m_PhysGroupMaskCollider & M_BitD(MediumDesc.m_iPhysGroup))) continue;

		CXR_IndexedSolidDesc32 soliddesr;
		_pSolidContainer->GetSolid(i, soliddesr);

		CBox3Dfp32 BoundBox(CVec3Dfp32(_FP32_MAX), CVec3Dfp32(-_FP32_MAX));
		BoundBox = pSolids[i].m_BoundBox;
		BoundBox.m_Min += _Offset;
		BoundBox.m_Max += _Offset;

		uint nVertices = pSolids[i].m_nVertices;

		BoundBox.m_Min -= CVec3Dfp32(1);
		BoundBox.m_Max += CVec3Dfp32(1);
		CBox3Dfp32 BoundBoxTransformed;
		BoundBox.Transform(Transform, BoundBoxTransformed);

		/*
		if (_pPhysContext->m_pWC)
		{
			CVec3Dfp32 Center;
			BoundBox.GetCenter(Center);
			CVec3Dfp32 Extents = (BoundBox.m_Max - BoundBox.m_Min) * 0.5f;
			CMat4Dfp32 tmp = _BSP2Transform;
			tmp.GetRow(3) = Center * tmp;
			_pPhysContext->m_pWC->RenderOBB(tmp, Extents, 0xff7f7f7f, 1.0f/20.0f, false);
		}
		*/

		if (_pCollisionInfo)
		{
			/*
			CCollideSolidBSP SolidBsp(_pPhysContext, _BSP2Transform, _pCollisionInfo, _nMaxCollisions);
			TBSP4Iterator<CCollideSolidBSP, CCollideSolidBSP> iter;
			iter.Iterate(this, _pPhysContext, 0, BoundBoxTransformed, _MediumFlags, &SolidBsp);
			nTotCollisions += SolidBsp.m_nTotCollisions;
*/
			uint nCollisions = __Phys_CollideSolid_i(_pPhysContext, 1, soliddesr, nVertices, _BSP2Transform, _Offset, BoundBoxTransformed, _MediumFlags, &_pCollisionInfo[nTotCollisions], Max(int(_nMaxCollisions - nTotCollisions), 0));
			nTotCollisions += nCollisions;
		}
	}
	return nTotCollisions;
}

int CXR_Model_BSP4::__Phys_CollideSolid_i(CXR_PhysicsContext* _pPhysContext, int _iNode, const CXR_IndexedSolidDesc32& _SolidDescr, int _nVertices, const CMat4Dfp32& _BSP2Transform, const CVec3Dfp32& _Offset, const CBox3Dfp32& _Bound, int _MediumFlags, CCollisionInfo* _pCollisionInfo, int _nMaxCollisions) const
{
	MAUTOSTRIP(CXR_Model_BSP4__Phys_CollideSolid_i, false);
	MSCOPESHORT(CXR_Model_BSP4::__Phys_CollideSolid_i);

	CMat4Dfp32 Transform, BSP2TransformInv;
	_BSP2Transform.InverseOrthogonal(BSP2TransformInv);
	_BSP2Transform.Multiply(_pPhysContext->m_WMatInv, Transform); 
	Transform.GetRow(3) = _Offset * Transform;

	int nTotCollisions = 0;

#ifdef BSP_USE_OCTAAABB

	if( m_lOctaAABBNodes.Len() )
	{
		TAP_RCD<const CXR_MediumDesc> pMediums = m_lMediums;
		TAP_RCD<const CBSP4_Face> pFaces = m_lFaces;

		//CBSP_OctaAABBIterator<CXR_Model_BSP4,CBSP_PortalLeaf> Itr(this,NULL,_Bound);
		uint32 nHit;
		uint32 liHit[256];
		nHit = BSPOctaAABB_EnumLeaf(this, liHit, 256, _Bound);

		for(int i = 0;i < nHit;i++)
		{
			uint32 iFace = liHit[i];

			// Face Tag skipped since OctaAABB only contains one instance of each face

			const CBSP4_Face* M_RESTRICT pF = &pFaces[iFace];

			//Cull unused
			if (!(pF->m_Flags & XW_FACE_PHYSICAL) || (pF->m_Flags & XW_FACE_VOLUMEINSIDE)) continue;
			const CXR_MediumDesc& Medium = pMediums[pF->m_iBackMedium];
			if (!(Medium.m_MediumFlags & _MediumFlags)) continue;
			if (!(M_BitD(Medium.m_iPhysGroup) & _pPhysContext->m_PhysGroupMaskThis)) continue;

			//Distance cull
			const CPlane3Dfp32& PolyPlane = m_pPlanes[pF->m_iPlane];
			fp32 MinDist = PolyPlane.GetBoxMinDistance(_Bound.m_Min, _Bound.m_Max);
			if (MinDist > MODEL_BSP_EPSILON) continue;

			//Save typing
			uint32 nv = pF->m_nVertices;
			uint32 iiv = pF->m_iiVertices;
			const uint32* piVert = m_piVertices + iiv;

			//Collision
			CSolidPolyhedronSeparating Solid((CXR_IndexedSolidDesc32 * ) &_SolidDescr, _nVertices);
			CSolidPolyhedronIndirectIndexedSeparating BSPFace(m_pVertices, piVert, nv, PolyPlane);

			CMat4Dfp32 Transform2;
			Transform2.Unit();
			Transform2.GetRow(3) = BSPFace.GetCenter();

			CBox3Dfp32 Box1 = _SolidDescr.m_BoundBox;
			CVec3Dfp32 Center1 = (Box1.m_Max + Box1.m_Min) * 0.5f;
			Center1 *= Transform;
			CVec3Dfp32 Center2 = Transform2.GetRow(3);

			CVec3Dfp32 CenterCenterAxis = (Center2 - Center1);
			CenterCenterAxis.Normalize();

			const int MaxCollisions = 100;
			CVec3Dfp32 PointOfCollisions[MaxCollisions];
			CVec3Dfp32 Normals[MaxCollisions];
			fp32 Depths[MaxCollisions];
			int MaxRealCollisions = Min (MaxCollisions, _nMaxCollisions-nTotCollisions);

#if !defined(M_RTM) && 0
			if (_pPhysContext->m_pWC)
			{
				CMat4Dfp32 T = _BSP2Transform;
				T.GetRow(3) = _Offset * T;
				for (uint iV = 0; iV < _nVertices; iV++)
					_pPhysContext->m_pWC->RenderVertex(Solid.GetVertex(iV) * T, 0xff7f007f, 1.0f / 20.0f, false);

				uint nCLF = Solid.GetColinearFaceDirectionCount();
				for (uint iCLF = 0; iCLF < nCLF; iCLF++)
				{
					bool slask;
					CVec3Dfp32 V = Solid.GetColinearFaceDirection(iCLF, T, slask);
					_pPhysContext->m_pWC->RenderVector(T.GetRow(3), V * 32.0f, 0xff7f007f, 1.0f / 20.0f, false);
				}
			}

#endif

			uint nC = TPolyhedraPolyhedraCollider<CSolidPolyhedronSeparating, CSolidPolyhedronIndirectIndexedSeparating, fp32>
				::Collide(&Solid, Transform, &BSPFace, Transform2, CenterCenterAxis, PointOfCollisions, Normals, Depths, MaxRealCollisions);

			if (nTotCollisions < _nMaxCollisions)
			{
				if (nTotCollisions >= _nMaxCollisions)
					return nTotCollisions;

				const CXW_Surface *pSurf = (const CXW_Surface *) m_lspSurfaces[pF->m_iSurface];
				fp32 FaceFriction = pSurf->GetBaseFrame()->m_Friction;

				for (uint i = 0; i < nC; i++)
				{
					_pCollisionInfo[nTotCollisions].m_bIsCollision = true;
					_pCollisionInfo[nTotCollisions].m_Pos = PointOfCollisions[i] * _pPhysContext->m_WMat;
					_pCollisionInfo[nTotCollisions].m_Plane.n = -Normals[i];
					_pCollisionInfo[nTotCollisions].m_Distance = M_Fabs(Depths[i]);
					_pCollisionInfo[nTotCollisions].m_Friction = Min(FaceFriction, _SolidDescr.m_Friction);
					nTotCollisions += 1;
				}
			}
			
		}

		return nTotCollisions;
	}

#endif

	if (!_iNode)
	{
		return 0;
	}

	TObjectPoolAllocator<CBSP4_EnumContext> EnumAlloc(m_spEnumContextPool);
	CBSP4_EnumContext* pEnumContext = EnumAlloc.GetObject();
	pEnumContext->Create(m_lFaces.Len(), ENUM_MEDIUMFLAGS, _MediumFlags);

	//bool bIntersect = false;
	uint32 aWorkingSet[128];
	uint WorkingStack = 0;
	aWorkingSet[WorkingStack++] = _iNode;

StartOf_Phys_IntersectOBB_i:
	uint32 WorkingNode = aWorkingSet[--WorkingStack];

StartOf_Phys_IntersectOBB_i_NoAdd:
	const CBSP4_Node* pN = &m_pNodes[WorkingNode];

	if (pN->IsLeaf())
	{
		uint nf = pN->m_nFaces;
		uint iif = pN->m_iiFaces;

		TAP_RCD<const CXR_MediumDesc> pMediums = m_lMediums;
		const uint32* piFaces = &m_piFaces[iif];
		for(int i = 0; i < nf; i++)
		{
			int iFace = piFaces[i];
			{
				uint iFaceIdx = iFace >> 3;
				uint8 iFaceMask = aShiftMulTab[iFace & 7];
				if (pEnumContext->m_pFTag[iFaceIdx] & iFaceMask) continue;
				if (!pEnumContext->m_pFTag[iFaceIdx]) pEnumContext->m_pFUntag[pEnumContext->m_nUntagEnum++] = iFaceIdx;
				pEnumContext->m_pFTag[iFaceIdx] |= iFaceMask;
			}
			const CBSP4_Face* pF = &m_pFaces[iFace];

			if (!(pF->m_Flags & XW_FACE_PHYSICAL) || (pF->m_Flags & XW_FACE_VOLUMEINSIDE)) continue;
			const CXR_MediumDesc& Medium = pMediums[pF->m_iBackMedium];
			if (!(Medium.m_MediumFlags & _MediumFlags)) continue;
			if (!(M_BitD(Medium.m_iPhysGroup) & _pPhysContext->m_PhysGroupMaskThis)) continue;

			const CPlane3Dfp32& PolyPlane = m_pPlanes[pF->m_iPlane];
			fp32 MinDist = PolyPlane.GetBoxMinDistance(_Bound.m_Min, _Bound.m_Max);
			if (MinDist > MODEL_BSP_EPSILON) continue;

			uint32 nv = pF->m_nVertices;
			uint32 iiv = pF->m_iiVertices;

			// Get bound-box for polygon.
			const uint32* piVert = m_piVertices + iiv;
			CBox3Dfp32 PBox;

#if 0 && defined PLATFORM_XBOX
			if (!SSE_Box3Dfp32_IntersectingMinMax(_Bound, m_pVertices, piVert, nv))
				continue;

			//			SSE_Box3Dfp32_GetMinMax(PBox, m_pVertices, piVert, nv);
#else
			PBox.m_Min = m_pVertices[piVert[0]];
			PBox.m_Max = m_pVertices[piVert[0]];

			for( uint32 v = 1; v < nv; v++ )
			{
				const CVec3Dfp32& V = m_pVertices[piVert[v]];
				PBox.m_Min.k[0] = Min( PBox.m_Min.k[0], V.k[0] );
				PBox.m_Max.k[0] = Max( PBox.m_Max.k[0], V.k[0] );
				PBox.m_Min.k[1] = Min( PBox.m_Min.k[1], V.k[1] );
				PBox.m_Max.k[1] = Max( PBox.m_Max.k[1], V.k[1] );
				PBox.m_Min.k[2] = Min( PBox.m_Min.k[2], V.k[2] );
				PBox.m_Max.k[2] = Max( PBox.m_Max.k[2], V.k[2] );
			}
			if( _Bound.m_Min.k[0] > PBox.m_Max.k[0] ||
				_Bound.m_Min.k[1] > PBox.m_Max.k[1] ||
				_Bound.m_Min.k[2] > PBox.m_Max.k[2] ||
				_Bound.m_Max.k[0] < PBox.m_Min.k[0] ||
				_Bound.m_Max.k[1] < PBox.m_Min.k[1] ||
				_Bound.m_Max.k[2] < PBox.m_Min.k[2] )
				continue;
#endif

			if( _pCollisionInfo )
			{

// GJK-Collision
#if 0
				CSolidPolyhedron2 Polyhedron1((CXR_IndexedSolidDesc32 * ) &_SolidDescr, _nVertices, Transform, _Offset);
				CVec3Dfp32 BspNormal = (m_pVertices[piVert[1]] - m_pVertices[piVert[0]]) / (m_pVertices[piVert[2]] - m_pVertices[piVert[0]]);
				BspNormal.Normalize();
				BspNormal *= 1.0f;

				CExtrudedSolidPolyhedronIndirectIndexed Polyhedron2(m_pVertices, piVert, nv, BspNormal);

				CMat4Dfp32 ident;
				ident.Unit();
				CVec3Dfp32 P1, P2;
				bool isColliding = TGJK<CSolidPolyhedron2, CExtrudedSolidPolyhedronIndirectIndexed>::MarginPenetrationDepth(Polyhedron1, ident, Polyhedron2, ident, P1, P2, 32*0.01f, 32*0.01f);

				if (isColliding)
				{
					if (nTotCollisions < _nMaxCollisions)
					{
						_pCollisionInfo[nTotCollisions].m_bIsCollision = true;
						_pCollisionInfo[nTotCollisions].m_Pos = P2 * _pPhysContext->m_WMat;
						_pCollisionInfo[nTotCollisions].m_Plane.n = (P1 - P2);
						//_pCollisionInfo[nTotCollisions].m_Plane.n = BspNormal;
						_pCollisionInfo[nTotCollisions].m_Plane.n.Normalize();
						_pCollisionInfo[nTotCollisions].m_Distance = P1.Distance(P2);
						nTotCollisions += 1;
					}
				}
#else


				CSolidPolyhedronSeparating Solid((CXR_IndexedSolidDesc32 * ) &_SolidDescr, _nVertices);
				CSolidPolyhedronIndirectIndexedSeparating BSPFace(m_pVertices, piVert, nv, PolyPlane);

				CMat4Dfp32 Transform2;
				Transform2.Unit();
				Transform2.GetRow(3) = BSPFace.GetCenter();

				CBox3Dfp32 Box1 = _SolidDescr.m_BoundBox;
				CVec3Dfp32 Center1 = (Box1.m_Max + Box1.m_Min) * 0.5f;
				Center1 *= Transform;
				CVec3Dfp32 Center2 = Transform2.GetRow(3);

				CVec3Dfp32 CenterCenterAxis = (Center2 - Center1);
				CenterCenterAxis.Normalize();

				const int MaxCollisions = 100;
				CVec3Dfp32 PointOfCollisions[MaxCollisions];
				CVec3Dfp32 Normals[MaxCollisions];
				fp32 Depths[MaxCollisions];
				int MaxRealCollisions = Min(MaxCollisions, _nMaxCollisions-nTotCollisions);

#if !defined(M_RTM) && 0
				if (_pPhysContext->m_pWC)
				{
					CMat4Dfp32 T = _BSP2Transform;
					T.GetRow(3) = _Offset * T;
					for (uint iV = 0; iV < _nVertices; iV++)
						_pPhysContext->m_pWC->RenderVertex(Solid.GetVertex(iV) * T, 0xff7f007f, 1.0f / 20.0f, false);
					
					uint nCLF = Solid.GetColinearFaceDirectionCount();
					for (uint iCLF = 0; iCLF < nCLF; iCLF++)
					{
						bool slask;
						CVec3Dfp32 V = Solid.GetColinearFaceDirection(iCLF, T, slask);
						_pPhysContext->m_pWC->RenderVector(T.GetRow(3), V * 32.0f, 0xff7f007f, 1.0f / 20.0f, false);
					}					
				}

#endif
				uint nC = TPolyhedraPolyhedraCollider<CSolidPolyhedronSeparating, CSolidPolyhedronIndirectIndexedSeparating, fp32>
					::Collide(&Solid, Transform, &BSPFace, Transform2, CenterCenterAxis, PointOfCollisions, Normals, Depths, MaxRealCollisions);

				if (nTotCollisions < _nMaxCollisions)
				{
					if (nTotCollisions >= _nMaxCollisions)
						return nTotCollisions;

					const CXW_Surface *pSurf = (const CXW_Surface *) m_lspSurfaces[pF->m_iSurface];
					fp32 FaceFriction = pSurf->GetBaseFrame()->m_Friction;

					for (uint i = 0; i < nC; i++)
					{
						_pCollisionInfo[nTotCollisions].m_bIsCollision = true;
						_pCollisionInfo[nTotCollisions].m_Pos = PointOfCollisions[i] * _pPhysContext->m_WMat;
						_pCollisionInfo[nTotCollisions].m_Plane.n = -Normals[i];
						_pCollisionInfo[nTotCollisions].m_Distance = M_Fabs(Depths[i]);
						_pCollisionInfo[nTotCollisions].m_Friction = Min(FaceFriction, _SolidDescr.m_Friction);
						nTotCollisions += 1;
					}
				}
#endif
			}
		}
	}
	else if( pN->IsNode() )
	{
		// Node, recurse...
		const CPlane3Dfp32* pP = &m_pPlanes[pN->m_iPlane];
		uint Side = pP->GetBoxPlaneSideMask(_Bound.m_Min, _Bound.m_Max);

		uint32 iFront = pN->m_iNodeFront;
		uint32 iBack = pN->m_iNodeBack;
		if( Side == 1 )
		{
			if( iFront )
			{
				WorkingNode = iFront;
				goto StartOf_Phys_IntersectOBB_i_NoAdd;
			}
		}
		else if( Side == 2 )
		{
			if( iBack )
			{
				WorkingNode = iBack;
				goto StartOf_Phys_IntersectOBB_i_NoAdd;
			}
		}
		else
		{
			if( iFront && iBack )
			{
				if( iFront )
					aWorkingSet[WorkingStack++]	= iFront;

				if( iBack )
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

	pEnumContext->Untag();

	return nTotCollisions;

}


int CXR_Model_BSP4::__Phys_CollideBox_i(CXR_PhysicsContext* _pPhysContext, int _iNode, const CPhysOBB& _Box, const CBox3Dfp32& _Bound, 
										int _MediumFlags, CCollisionInfo* _pCollisionInfo, int _nMaxCollisions) const
{
	MAUTOSTRIP(CXR_Model_BSP4__Phys_CollideBox_i, false);
	MSCOPESHORT(CXR_Model_BSP4::__Phys_CollideBox_i);

//	CMat4Dfp32 Transform, BSP2TransformInv;
//	_BSP2Transform.InverseOrthogonal(BSP2TransformInv);
//	_BSP2Transform.Multiply(_pPhysContext->m_WMatInv, Transform); 
//	Transform.GetRow(3) = _Offset * Transform;

	CMat4Dfp32 Transform;
	_Box.GetPosition(Transform);

	uint nTotCollisions = 0;

#ifdef BSP_USE_OCTAAABB

	if( m_lOctaAABBNodes.Len() )
	{
		TAP_RCD<const CXR_MediumDesc> pMediums = m_lMediums;
		TAP_RCD<const CBSP4_Face> pFaces = m_lFaces;

		//CBSP_OctaAABBIterator<CXR_Model_BSP4,CBSP_PortalLeaf> Itr(this,NULL,_Bound);
		uint32 nHit;
		uint32 liHit[256];
		nHit = BSPOctaAABB_EnumLeaf(this, liHit, 256, _Bound);

		for(int i = 0;i < nHit;i++)
		{
			uint32 iFace = liHit[i];

			// Face Tag skipped since OctaAABB only contains one instance of each face

			const CBSP4_Face* M_RESTRICT pF = &pFaces[iFace];

			//Cull unused
			if (!(pF->m_Flags & XW_FACE_PHYSICAL) || (pF->m_Flags & XW_FACE_VOLUMEINSIDE)) continue;
			const CXR_MediumDesc& Medium = pMediums[pF->m_iBackMedium];
			if (!(Medium.m_MediumFlags & _MediumFlags)) continue;
			if (!(M_BitD(Medium.m_iPhysGroup) & _pPhysContext->m_PhysGroupMaskThis)) continue;

			//Distance cull
			const CPlane3Dfp32& PolyPlane = m_pPlanes[pF->m_iPlane];
			fp32 MinDist = PolyPlane.GetBoxMinDistance(_Bound.m_Min, _Bound.m_Max);
			if (MinDist > MODEL_BSP_EPSILON) continue;

			//Save typing
			uint32 nv = pF->m_nVertices;
			uint32 iiv = pF->m_iiVertices;
			const uint32* piVert = m_piVertices + iiv;

			//Collision
			CSolidBoxSeparating BoxSp(_Box.m_E);
//			CSolidPolyhedronSeparating Solid((CXR_IndexedSolidDesc32 * ) &_SolidDescr, _nVertices);
			CSolidPolyhedronIndirectIndexedSeparating BSPFace(m_pVertices, piVert, nv, PolyPlane);

			CMat4Dfp32 Transform2;
			Transform2.Unit();
			Transform2.GetRow(3) = BSPFace.GetCenter();
/*
			CBox3Dfp32 Box1 = _SolidDescr.m_BoundBox;
			CVec3Dfp32 Center1 = (Box1.m_Max + Box1.m_Min) * 0.5f;
			Center1 *= Transform;
*/
			CVec3Dfp32 Center2 = Transform2.GetRow(3);

			CVec3Dfp32 CenterCenterAxis = (Center2 - _Box.m_C);
			CenterCenterAxis.Normalize();

			const int MaxCollisions = 100;
			CVec3Dfp32 PointOfCollisions[MaxCollisions];
			CVec3Dfp32 Normals[MaxCollisions];
			fp32 Depths[MaxCollisions];

			uint nC = TPolyhedraPolyhedraCollider<CSolidBoxSeparating, CSolidPolyhedronIndirectIndexedSeparating, fp32>
				::Collide(&BoxSp, Transform, &BSPFace, Transform2, CenterCenterAxis, PointOfCollisions, Normals, Depths,MaxCollisions);

			if (nTotCollisions < _nMaxCollisions)
			{
				if (nTotCollisions >= _nMaxCollisions)
					return nTotCollisions;

				const CXW_Surface *pSurf = (const CXW_Surface *) m_lspSurfaces[pF->m_iSurface];
				fp32 FaceFriction = pSurf->GetBaseFrame()->m_Friction;

				for (uint i = 0; i < nC; i++)
				{
					_pCollisionInfo[nTotCollisions].m_bIsCollision = true;
					_pCollisionInfo[nTotCollisions].m_Pos = PointOfCollisions[i] * _pPhysContext->m_WMat;
					_pCollisionInfo[nTotCollisions].m_Plane.n = -Normals[i];
					_pCollisionInfo[nTotCollisions].m_Distance = M_Fabs(Depths[i]);
					_pCollisionInfo[nTotCollisions].m_Friction = FaceFriction;
					nTotCollisions += 1;
				}
			}

		}

		return nTotCollisions;
	}

#endif

	if (!_iNode)
	{
		return 0;
	}

	TObjectPoolAllocator<CBSP4_EnumContext> EnumAlloc(m_spEnumContextPool);
	CBSP4_EnumContext* pEnumContext = EnumAlloc.GetObject();
	pEnumContext->Create(m_lFaces.Len(), ENUM_MEDIUMFLAGS, _MediumFlags);

	//bool bIntersect = false;
	uint32 aWorkingSet[128];
	uint WorkingStack = 0;
	aWorkingSet[WorkingStack++] = _iNode;

StartOf_Phys_IntersectOBB_i:
	uint32 WorkingNode = aWorkingSet[--WorkingStack];

StartOf_Phys_IntersectOBB_i_NoAdd:
	const CBSP4_Node* pN = &m_pNodes[WorkingNode];

	if (pN->IsLeaf())
	{
		uint nf = pN->m_nFaces;
		uint iif = pN->m_iiFaces;

		TAP_RCD<const CXR_MediumDesc> pMediums = m_lMediums;
		const uint32* piFaces = &m_piFaces[iif];
		for(int i = 0; i < nf; i++)
		{
			int iFace = piFaces[i];
			{
				uint iFaceIdx = iFace >> 3;
				uint8 iFaceMask = aShiftMulTab[iFace & 7];
				if (pEnumContext->m_pFTag[iFaceIdx] & iFaceMask) continue;
				if (!pEnumContext->m_pFTag[iFaceIdx]) pEnumContext->m_pFUntag[pEnumContext->m_nUntagEnum++] = iFaceIdx;
				pEnumContext->m_pFTag[iFaceIdx] |= iFaceMask;
			}
			const CBSP4_Face* pF = &m_pFaces[iFace];

			if (!(pF->m_Flags & XW_FACE_PHYSICAL) || (pF->m_Flags & XW_FACE_VOLUMEINSIDE)) continue;
			const CXR_MediumDesc& Medium = pMediums[pF->m_iBackMedium];
			if (!(Medium.m_MediumFlags & _MediumFlags)) continue;
			if (!(M_BitD(Medium.m_iPhysGroup) & _pPhysContext->m_PhysGroupMaskThis)) continue;

			const CPlane3Dfp32& PolyPlane = m_pPlanes[pF->m_iPlane];
			fp32 MinDist = PolyPlane.GetBoxMinDistance(_Bound.m_Min, _Bound.m_Max);
			if (MinDist > MODEL_BSP_EPSILON) continue;

			uint32 nv = pF->m_nVertices;
			uint32 iiv = pF->m_iiVertices;

			// Get bound-box for polygon.
			const uint32* piVert = m_piVertices + iiv;
			CBox3Dfp32 PBox;

			PBox.m_Min = m_pVertices[piVert[0]];
			PBox.m_Max = m_pVertices[piVert[0]];

			for( uint32 v = 1; v < nv; v++ )
			{
				const CVec3Dfp32& V = m_pVertices[piVert[v]];
				PBox.m_Min.k[0] = Min( PBox.m_Min.k[0], V.k[0] );
				PBox.m_Max.k[0] = Max( PBox.m_Max.k[0], V.k[0] );
				PBox.m_Min.k[1] = Min( PBox.m_Min.k[1], V.k[1] );
				PBox.m_Max.k[1] = Max( PBox.m_Max.k[1], V.k[1] );
				PBox.m_Min.k[2] = Min( PBox.m_Min.k[2], V.k[2] );
				PBox.m_Max.k[2] = Max( PBox.m_Max.k[2], V.k[2] );
			}
			if( _Bound.m_Min.k[0] > PBox.m_Max.k[0] ||
				_Bound.m_Min.k[1] > PBox.m_Max.k[1] ||
				_Bound.m_Min.k[2] > PBox.m_Max.k[2] ||
				_Bound.m_Max.k[0] < PBox.m_Min.k[0] ||
				_Bound.m_Max.k[1] < PBox.m_Min.k[1] ||
				_Bound.m_Max.k[2] < PBox.m_Min.k[2] )
				continue;

			if( _pCollisionInfo )
			{

				CSolidBoxSeparating BoxSp(_Box.m_E);
//				CSolidPolyhedronSeparating Solid((CXR_IndexedSolidDesc32 * ) &_SolidDescr, _nVertices);
				CSolidPolyhedronIndirectIndexedSeparating BSPFace(m_pVertices, piVert, nv, PolyPlane);

				CMat4Dfp32 Transform2;
				Transform2.Unit();
				Transform2.GetRow(3) = BSPFace.GetCenter();
/*
				CBox3Dfp32 Box1 = _SolidDescr.m_BoundBox;
				CVec3Dfp32 Center1 = (Box1.m_Max + Box1.m_Min) * 0.5f;
				Center1 *= Transform;
*/
				CVec3Dfp32 Center2 = Transform2.GetRow(3);

				CVec3Dfp32 CenterCenterAxis = (Center2 - _Box.m_C);
				CenterCenterAxis.Normalize();

				const int MaxCollisions = 100;
				CVec3Dfp32 PointOfCollisions[MaxCollisions];
				CVec3Dfp32 Normals[MaxCollisions];
				fp32 Depths[MaxCollisions];

				uint nC = TPolyhedraPolyhedraCollider<CSolidBoxSeparating, CSolidPolyhedronIndirectIndexedSeparating, fp32>
					::Collide(&BoxSp, Transform, &BSPFace, Transform2, CenterCenterAxis, PointOfCollisions, Normals, Depths, MaxCollisions);

				if (nTotCollisions < _nMaxCollisions)
				{
					if (nTotCollisions >= _nMaxCollisions)
						return nTotCollisions;

					const CXW_Surface *pSurf = (const CXW_Surface *) m_lspSurfaces[pF->m_iSurface];
					fp32 FaceFriction = pSurf->GetBaseFrame()->m_Friction;

					for (uint i = 0; i < nC; i++)
					{
						_pCollisionInfo[nTotCollisions].m_bIsCollision = true;
						_pCollisionInfo[nTotCollisions].m_Pos = PointOfCollisions[i] * _pPhysContext->m_WMat;
						_pCollisionInfo[nTotCollisions].m_Plane.n = -Normals[i];
						_pCollisionInfo[nTotCollisions].m_Distance = M_Fabs(Depths[i]);
						_pCollisionInfo[nTotCollisions].m_Friction = FaceFriction;
						nTotCollisions += 1;
					}
				}
			}
		}
	}
	else if( pN->IsNode() )
	{
		// Node, recurse...
		const CPlane3Dfp32* pP = &m_pPlanes[pN->m_iPlane];
		uint Side = pP->GetBoxPlaneSideMask(_Bound.m_Min, _Bound.m_Max);

		uint32 iFront = pN->m_iNodeFront;
		uint32 iBack = pN->m_iNodeBack;
		if( Side == 1 )
		{
			if( iFront )
			{
				WorkingNode = iFront;
				goto StartOf_Phys_IntersectOBB_i_NoAdd;
			}
		}
		else if( Side == 2 )
		{
			if( iBack )
			{
				WorkingNode = iBack;
				goto StartOf_Phys_IntersectOBB_i_NoAdd;
			}
		}
		else
		{
			if( iFront && iBack )
			{
				if( iFront )
					aWorkingSet[WorkingStack++]	= iFront;

				if( iBack )
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

	pEnumContext->Untag();

	return nTotCollisions;
}


int CXR_Model_BSP4::__Phys_CollideCapsule_i(CXR_PhysicsContext* _pPhysContext, int _iNode, const CPhysCapsule& _Capsule, const CBox3Dfp32& _Bound, 
											int _MediumFlags, CCollisionInfo* _pCollisionInfo, int _nMaxCollisions) const
{

	MAUTOSTRIP(CXR_Model_BSP4__Phys_CollideCapsule_i, false);
	MSCOPESHORT(CXR_Model_BSP4::__Phys_CollideCapsule_i);

	CMat4Dfp32 Transform;
	_Capsule.GetPosition(Transform);

	vec128 CaCen = M_VLd_V3_Slow(&_Capsule.m_Pos.GetRow(3));
	vec128 CaDir = M_VLd_V3_Slow(&_Capsule.m_Pos.GetRow(0));
	vec128 CaExt = M_VLd_V3_Slow(&_Capsule.m_E);

	uint nTotCollisions = 0;

#ifdef BSP_USE_OCTAAABB

	if( m_lOctaAABBNodes.Len() )
	{
		TAP_RCD<const CXR_MediumDesc> pMediums = m_lMediums;
		TAP_RCD<const CBSP4_Face> pFaces = m_lFaces;

		//CBSP_OctaAABBIterator<CXR_Model_BSP4,CBSP_PortalLeaf> Itr(this,NULL,_Bound);
		uint32 nHit;
		uint32 liHit[256];
		nHit = BSPOctaAABB_EnumLeaf(this, liHit, 256, _Bound);

		for(int i = 0;i < nHit;i++)
		{
			uint32 iFace = liHit[i];

			// Face Tag skipped since OctaAABB only contains one instance of each face

			const CBSP4_Face* M_RESTRICT pF = &pFaces[iFace];

			//Cull unused
			if (!(pF->m_Flags & XW_FACE_PHYSICAL) || (pF->m_Flags & XW_FACE_VOLUMEINSIDE)) continue;
			const CXR_MediumDesc& Medium = pMediums[pF->m_iBackMedium];
			if (!(Medium.m_MediumFlags & _MediumFlags)) continue;
			if (!(M_BitD(Medium.m_iPhysGroup) & _pPhysContext->m_PhysGroupMaskThis)) continue;

			//Distance cull
			const CPlane3Dfp32& PolyPlane = m_pPlanes[pF->m_iPlane];
			fp32 MinDist = PolyPlane.GetBoxMinDistance(_Bound.m_Min, _Bound.m_Max);
			if (MinDist > MODEL_BSP_EPSILON) continue;

			//Save typing
			uint32 nv = pF->m_nVertices;
			uint32 iiv = pF->m_iiVertices;
			const uint32* piVert = m_piVertices + iiv;
/*
			//Collision
			CSolidBoxSeparating BoxSp(_Box.m_E);
			CSolidPolyhedronIndirectIndexedSeparating BSPFace(m_pVertices, piVert, nv, PolyPlane);

			CMat4Dfp32 Transform2;
			Transform2.Unit();
			Transform2.GetRow(3) = BSPFace.GetCenter();
			CVec3Dfp32 Center2 = Transform2.GetRow(3);

			CVec3Dfp32 CenterCenterAxis = (Center2 - _Box.m_C);
			CenterCenterAxis.Normalize();
*/
/*
			const int MaxCollisions = 2;
			CVec3Dfp32 PointOfCollisions[MaxCollisions];
			CVec3Dfp32 Normals[MaxCollisions];
			fp32 Depths[MaxCollisions];

			uint nC = Phys_Collide_CapsuleFace(_Capsule,m_pVertices,piVert,nv,PolyPlane,
				PointOfCollisions,Normals,Depths);
*/
			vec128 Vtx[2],Nrm,Dpt;
			vec128 lVerts[32];
			vec128 Pln;
			CVec4Dfp32 Plane(PolyPlane.n.k[0],PolyPlane.n.k[1],PolyPlane.n.k[2],PolyPlane.d);
			for(uint i = 0;i < nv;i++)
			{
				lVerts[i] = M_VLd_V3_Slow(m_pVertices + piVert[i]);
			}
			Pln = Plane.v;
			uint nC = Phys_Collide_CapsuleFace(CaCen,CaDir,CaExt,Pln,lVerts,nv,Vtx,Nrm,Dpt);

			if ( (nTotCollisions < _nMaxCollisions) && (nC) )
			{
				const CXW_Surface *pSurf = (const CXW_Surface *) m_lspSurfaces[pF->m_iSurface];
				fp32 FaceFriction = pSurf->GetBaseFrame()->m_Friction;

				CVec3Dfp32 Norm = -M_VGetV3_Slow(Nrm);
				fp32 Depth = M_Fabs(M_VGetX_Slow(Dpt));

				for (uint i = 0; i < nC; i++)
				{
					_pCollisionInfo[nTotCollisions].m_bIsCollision = true;
					_pCollisionInfo[nTotCollisions].m_Pos = M_VGetV3_Slow(Vtx[i]) * _pPhysContext->m_WMat;
					_pCollisionInfo[nTotCollisions].m_Plane.n = Norm;
					_pCollisionInfo[nTotCollisions].m_Distance = Depth;
					_pCollisionInfo[nTotCollisions].m_Friction = FaceFriction;
					nTotCollisions += 1;
				}
			}
		}

		return nTotCollisions;
	}

#endif

	if (!_iNode)
	{
		return 0;
	}

	TObjectPoolAllocator<CBSP4_EnumContext> EnumAlloc(m_spEnumContextPool);
	CBSP4_EnumContext* pEnumContext = EnumAlloc.GetObject();
	pEnumContext->Create(m_lFaces.Len(), ENUM_MEDIUMFLAGS, _MediumFlags);

	//bool bIntersect = false;
	uint32 aWorkingSet[128];
	uint WorkingStack = 0;
	aWorkingSet[WorkingStack++] = _iNode;

StartOf_Phys_IntersectOBB_i:
	uint32 WorkingNode = aWorkingSet[--WorkingStack];

StartOf_Phys_IntersectOBB_i_NoAdd:
	const CBSP4_Node* pN = &m_pNodes[WorkingNode];

	if (pN->IsLeaf())
	{
		uint nf = pN->m_nFaces;
		uint iif = pN->m_iiFaces;

		TAP_RCD<const CXR_MediumDesc> pMediums = m_lMediums;
		const uint32* piFaces = &m_piFaces[iif];
		for(int i = 0; i < nf; i++)
		{
			int iFace = piFaces[i];
			{
				uint iFaceIdx = iFace >> 3;
				uint8 iFaceMask = aShiftMulTab[iFace & 7];
				if (pEnumContext->m_pFTag[iFaceIdx] & iFaceMask) continue;
				if (!pEnumContext->m_pFTag[iFaceIdx]) pEnumContext->m_pFUntag[pEnumContext->m_nUntagEnum++] = iFaceIdx;
				pEnumContext->m_pFTag[iFaceIdx] |= iFaceMask;
			}
			const CBSP4_Face* pF = &m_pFaces[iFace];

			if (!(pF->m_Flags & XW_FACE_PHYSICAL) || (pF->m_Flags & XW_FACE_VOLUMEINSIDE)) continue;
			const CXR_MediumDesc& Medium = pMediums[pF->m_iBackMedium];
			if (!(Medium.m_MediumFlags & _MediumFlags)) continue;
			if (!(M_BitD(Medium.m_iPhysGroup) & _pPhysContext->m_PhysGroupMaskThis)) continue;

			const CPlane3Dfp32& PolyPlane = m_pPlanes[pF->m_iPlane];
			fp32 MinDist = PolyPlane.GetBoxMinDistance(_Bound.m_Min, _Bound.m_Max);
			if (MinDist > MODEL_BSP_EPSILON) continue;

			uint32 nv = pF->m_nVertices;
			uint32 iiv = pF->m_iiVertices;

			// Get bound-box for polygon.
			const uint32* piVert = m_piVertices + iiv;
			CBox3Dfp32 PBox;

			PBox.m_Min = m_pVertices[piVert[0]];
			PBox.m_Max = m_pVertices[piVert[0]];

			for( uint32 v = 1; v < nv; v++ )
			{
				const CVec3Dfp32& V = m_pVertices[piVert[v]];
				PBox.m_Min.k[0] = Min( PBox.m_Min.k[0], V.k[0] );
				PBox.m_Max.k[0] = Max( PBox.m_Max.k[0], V.k[0] );
				PBox.m_Min.k[1] = Min( PBox.m_Min.k[1], V.k[1] );
				PBox.m_Max.k[1] = Max( PBox.m_Max.k[1], V.k[1] );
				PBox.m_Min.k[2] = Min( PBox.m_Min.k[2], V.k[2] );
				PBox.m_Max.k[2] = Max( PBox.m_Max.k[2], V.k[2] );
			}
			if( _Bound.m_Min.k[0] > PBox.m_Max.k[0] ||
				_Bound.m_Min.k[1] > PBox.m_Max.k[1] ||
				_Bound.m_Min.k[2] > PBox.m_Max.k[2] ||
				_Bound.m_Max.k[0] < PBox.m_Min.k[0] ||
				_Bound.m_Max.k[1] < PBox.m_Min.k[1] ||
				_Bound.m_Max.k[2] < PBox.m_Min.k[2] )
				continue;

			if( _pCollisionInfo )
			{
/*
				CSolidBoxSeparating BoxSp(_Box.m_E);
				CSolidPolyhedronIndirectIndexedSeparating BSPFace(m_pVertices, piVert, nv, PolyPlane);

				CMat4Dfp32 Transform2;
				Transform2.Unit();
				Transform2.GetRow(3) = BSPFace.GetCenter();
				CVec3Dfp32 Center2 = Transform2.GetRow(3);

				CVec3Dfp32 CenterCenterAxis = (Center2 - _Box.m_C);
				CenterCenterAxis.Normalize();
*/
/*				
				const int MaxCollisions = 2;
				CVec3Dfp32 PointOfCollisions[MaxCollisions];
				CVec3Dfp32 Normals[MaxCollisions];
				fp32 Depths[MaxCollisions];

				uint nC = Phys_Collide_CapsuleFace(_Capsule,m_pVertices,piVert,nv,PolyPlane,
					PointOfCollisions,Normals,Depths);
*/
				vec128 Vtx[2],Nrm,Dpt;
				vec128 lVerts[32];
				vec128 Pln;
				CVec4Dfp32 Plane(PolyPlane.n.k[0],PolyPlane.n.k[1],PolyPlane.n.k[2],PolyPlane.d);
				for(uint i = 0;i < nv;i++)
				{
					lVerts[i] = M_VLd_V3_Slow(m_pVertices + piVert[i]);
				}
				Pln = Plane.v;
				uint nC = Phys_Collide_CapsuleFace(CaCen,CaDir,CaExt,Pln,lVerts,nv,Vtx,Nrm,Dpt);
				
				if ( (nTotCollisions < _nMaxCollisions) && (nC) )
				{
					const CXW_Surface *pSurf = (const CXW_Surface *) m_lspSurfaces[pF->m_iSurface];
					fp32 FaceFriction = pSurf->GetBaseFrame()->m_Friction;

					CVec3Dfp32 Norm = -M_VGetV3_Slow(Nrm);
					fp32 Depth = M_Fabs(M_VGetX_Slow(Dpt));

					for (uint i = 0; i < nC; i++)
					{
						_pCollisionInfo[nTotCollisions].m_bIsCollision = true;
						_pCollisionInfo[nTotCollisions].m_Pos = M_VGetV3_Slow(Vtx[i]) * _pPhysContext->m_WMat;
						_pCollisionInfo[nTotCollisions].m_Plane.n = Norm;
						_pCollisionInfo[nTotCollisions].m_Distance = Depth;
						_pCollisionInfo[nTotCollisions].m_Friction = FaceFriction;
						nTotCollisions += 1;
					}
				}
			}
		}
	}
	else if( pN->IsNode() )
	{
		// Node, recurse...
		const CPlane3Dfp32* pP = &m_pPlanes[pN->m_iPlane];
		uint Side = pP->GetBoxPlaneSideMask(_Bound.m_Min, _Bound.m_Max);

		uint32 iFront = pN->m_iNodeFront;
		uint32 iBack = pN->m_iNodeBack;
		if( Side == 1 )
		{
			if( iFront )
			{
				WorkingNode = iFront;
				goto StartOf_Phys_IntersectOBB_i_NoAdd;
			}
		}
		else if( Side == 2 )
		{
			if( iBack )
			{
				WorkingNode = iBack;
				goto StartOf_Phys_IntersectOBB_i_NoAdd;
			}
		}
		else
		{
			if( iFront && iBack )
			{
				if( iFront )
					aWorkingSet[WorkingStack++]	= iFront;

				if( iBack )
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

	pEnumContext->Untag();

	return nTotCollisions;
}


int CXR_Model_BSP4::Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_GetMedium, 0);

	CVec3Dfp32 v0;
	_v0.MultiplyMatrix(_pPhysContext->m_WMatInv, v0);
	uint iLeaf = GetLeaf(v0);
	if (!iLeaf) return XW_MEDIUM_SOLID;
	return m_pMediums[m_pNodes[iLeaf].m_iMedium].m_MediumFlags;
}

void CXR_Model_BSP4::Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, CXR_MediumDesc& _RetMedium)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_GetMedium_2, MAUTOSTRIP_VOID);

	CVec3Dfp32 v0;
	_v0.MultiplyMatrix(_pPhysContext->m_WMatInv, v0);
	uint iLeaf = GetLeaf(v0);
	if (!iLeaf)
	{
		_RetMedium.SetSolid();
		return;
	}

	_RetMedium = m_pMediums[m_pNodes[iLeaf].m_iMedium];
}

int CXR_Model_BSP4::__Phys_GetCombinedMediumFlags_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CBox3Dfp32* _pBox) const
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_GetCombinedMediumFlags_r, 0);
	const CBSP4_Node* pN = &m_pNodes[_iNode];

	if (pN->IsLeaf())
	{
		return m_pMediums[pN->m_iMedium].m_MediumFlags;
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

int CXR_Model_BSP4::__Phys_GetCombinedMediumFlags_i(CXR_PhysicsContext* _pPhysContext, int _iNode, const CBox3Dfp32* _pBox) const
{
	MAUTOSTRIP(CXR_Model_BSP4___Phys_GetCombinedMediumFlags_i, 0);

	uint MediumFlags = 0;
	uint aWorkStack[256];
	uint iStackPos = 0;
	aWorkStack[iStackPos++] = _iNode;
StartOf__Phys_GetCombinedMediumFlags_i:
	_iNode = aWorkStack[--iStackPos];
StartOf__Phys_GetCombinedMediumFlags_i_NoPush:
	M_ASSERT(iStackPos < 256, "!");
	const CBSP4_Node* pN = &m_pNodes[_iNode];

	if (pN->IsLeaf())
	{
		const CXR_MediumDesc& Medium = m_lMediums[pN->m_iMedium];
		if (_pPhysContext->m_PhysGroupMaskThis & M_BitD(Medium.m_iPhysGroup))
			MediumFlags |= Medium.m_MediumFlags;
	}
	else
	{
		CPlane3Dfp32 Plane = m_pPlanes[pN->m_iPlane];
		Plane.Transform(_pPhysContext->m_WMat);	// We're doing this backwards to what we usually do, i.e transforming model data to world space instead of transforming input to model space.

		fp32 MinDist, MaxDist;
		Plane.GetBoxMinAndMaxDistance(_pBox->m_Min, _pBox->m_Max, MinDist, MaxDist);
		if(MinDist < -0.01f && MaxDist > 0.01f)
		{
			aWorkStack[iStackPos++] = pN->m_iNodeFront;

			_iNode = pN->m_iNodeBack;
			goto StartOf__Phys_GetCombinedMediumFlags_i_NoPush;
		}
		else if(MinDist < -0.01f)
		{
			_iNode = pN->m_iNodeBack;
			goto StartOf__Phys_GetCombinedMediumFlags_i_NoPush;
		}
		else if(MaxDist > 0.01f)
		{
			_iNode = pN->m_iNodeFront;
			goto StartOf__Phys_GetCombinedMediumFlags_i_NoPush;
		}
	}

	if(iStackPos > 0)
		goto StartOf__Phys_GetCombinedMediumFlags_i;

	return MediumFlags;
}

int CXR_Model_BSP4::Phys_GetCombinedMediumFlags(CXR_PhysicsContext* _pPhysContext, const CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_GetCombinedMediumFlags, 0);
	return __Phys_GetCombinedMediumFlags_i(_pPhysContext, 1, &_Box);
}


