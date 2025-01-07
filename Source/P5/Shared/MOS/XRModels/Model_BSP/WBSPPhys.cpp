#include "PCH.h"

#include "WBSPModel.h"
#include "WBSPDef.h"
#include "../../XR/Phys/WPhysPCS.h"
#include "../../Classes/Render/MWireContainer.h"

#ifdef PLATFORM_PS2
#include "eeregs.h"
#endif


// -------------------------------------------------------------------
// Physics internal functions
// -------------------------------------------------------------------
void CXR_Model_BSP::__Phys_RenderFace(int _iFace, const CMat4Dfp32& _WMat, CWireContainer* _pWC, int _Col)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_RenderFace, MAUTOSTRIP_VOID);

	const CBSP_Face* pF = &m_lFaces[_iFace];

	int iLast = m_liVertices[pF->m_iiVertices + pF->m_nVertices - 1];
	for(int i = 0; i < pF->m_nVertices; i++)
	{
		int iVertex = m_liVertices[pF->m_iiVertices + i];
		_pWC->RenderWire(m_lVertices[iLast], m_lVertices[iVertex], _Col);
		iLast	= iVertex;
	}
}

bool CXR_Model_BSP::__Phys_IntersectSphere_Polygon(
	const CVec3Dfp32* _pV, 
	const uint32* _piV, 
	int _nV,
	const CPlane3Dfp32& _Plane, 
	const CVec3Dfp32& _Pos, 
	fp32 _Radius, 
	CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_IntersectSphere_Polygon, false);

//	CBSP_Face* pF = &m_lFaces[_iFace];
//	int iPlane = pF->m_iPlane;
//	CPlane3Dfp32* pP = &m_pPlanes[iPlane];
	fp32 d = _Plane.Distance(_Pos);
	if (d < -_Radius) return false;
	if (d > _Radius) return false;

	CPlane3Dfp32 Edge_Plane(CVec3Dfp32(0, 0, 1), 0);
	uint Edge_bCollided = false;
	fp32 Edge_Nearest = _Radius*2.0f;
	CVec3Dfp32 Edge_Pos;

	uint32 InsideBit = 1;
	uint32 InsideMask = 0;
	uint bAllInside = true;
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


bool CXR_Model_BSP::__Phys_IntersectSphere_CheckFace(int _iFace, const CVec3Dfp32& _Pos, fp32 _Radius, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_IntersectSphere_CheckFace, false);
	CBSP_Face* pF = &m_lFaces[_iFace];

	uint iPlane = pF->m_iPlane;
	CPlane3Dfp32* pP = &m_pPlanes[iPlane];
	fp32 d = pP->Distance(_Pos);

//ConOut(CStrF("(1) iF %d", _iFace));
	if (d < -_Radius) return false;
	if (d > _Radius) return false;

//	fp32 RSqr = Sqr(_Radius);

	CPlane3Dfp32 Edge_Plane(CVec3Dfp32(0, 0, 1), 1);
	uint Edge_bCollided = false;
	fp32 Edge_Nearest = _Radius*2.0f;
	CVec3Dfp32 Edge_Pos;

	uint nv = pF->m_nPhysV;
	uint iiv = pF->m_iiPhysV;

	uint bAllInside = true;
	uint InsideBit = 1;
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
				fp32 l = Abs(dist_e);
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

	ConOutL("(CXR_Model_BSP::__Phys_IntersectSphere_CheckFace) FIXME: Hay, no m_Time is calculated. /mh");

//	LogFile(CStrF("Mask %d, %d", InsideMask, (1 << nv) -1));
	if (bAllInside)
	{
//ConOut(CStrF("Plane %f", 
		_pCollisionInfo->m_bIsValid = true;
		_pCollisionInfo->m_bIsCollision = true;
		_pCollisionInfo->m_Plane = *pP;
		_pCollisionInfo->m_Distance = Abs(d) - _Radius;
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

bool CXR_Model_BSP::__Phys_IntersectSphere_r(int _iNode, const CVec3Dfp32& _Pos, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_IntersectSphere_r, false);
	if (!_iNode) 
	{
//ConOut(CStr("Solid node!"));
//		if (_pCollisionInfo) _pCollisionInfo->m_bIsValid = false;
		return false;
	}

	CBSP_Node* pN = &m_pNodes[_iNode];
	CCollisionInfo CInfo;
	bool bIntersect = false;

	if (pN->IsLeaf())
	{
		if (m_lMediums[pN->m_iMedium].m_MediumFlags & _MediumFlags)
			if (!_pCollisionInfo) return true;

		// Leaf
		int nFaces = pN->m_nFaces;
		int iiFaces = pN->m_iiFaces;
//ConOut(CStrF("iN %d, nF %d", _iNode, nFaces));
		for(int i = 0; i < nFaces; i++)
		{
			int iFace = m_liFaces[iiFaces + i];

			// Check medium...
			CBSP_Face* pF = &m_lFaces[iFace];
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
				if (!(m_lMediums[pF->m_iBackMedium].m_MediumFlags & _MediumFlags)) continue;
			}

			// Do intersection...
			if (__Phys_IntersectSphere_CheckFace(iFace, _Pos, _Radius, &CInfo))
			{
				if (!_pCollisionInfo) return true;

				if (_pCollisionInfo->Improve(CInfo))
				{
					if (_pCollisionInfo->IsComplete())
						return true;
				}

				bIntersect = true;
			}
		}

		if (m_lMediums[pN->m_iMedium].m_MediumFlags & _MediumFlags) return true;
		return bIntersect;
	}
	else
	{
		// Node, recurse...
		int iPlane = m_pNodes[_iNode].m_iPlane;
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

bool SphereInBoundBox(const CVec3Dfp32& _v, fp32 _Radius, const CVec3Dfp32& _BoxMin, const CVec3Dfp32& _BoxMax);
bool SphereInBoundBox(const CVec3Dfp32& _v, fp32 _Radius, const CVec3Dfp32& _BoxMin, const CVec3Dfp32& _BoxMax)
{
	MAUTOSTRIP(SphereInBoundBox, false);
	for(int i = 0; i < 3; i++)
	{
		if(_v.k[i] + _Radius < _BoxMin.k[i]) return false;
		if(_v.k[i] - _Radius > _BoxMax.k[i]) return false;
	}
	return true;
}

bool CXR_Model_BSP::__Phys_IntersectSphere_Triangle(const CVec3Dfp32& _V0, const CVec3Dfp32& _V1, const CVec3Dfp32& _V2, const CVec3Dfp32& _Pos, fp32 _Radius, CCollisionInfo* _pCollisionInfo)
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

bool CXR_Model_BSP::__Phys_IntersectSphere_SB(const CVec3Dfp32& _Pos, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_IntersectSphere_SB, false);
	MSCOPESHORT(CXR_Model_BSP::__Phys_IntersectSphere_SB);

	uint32 ST[CSB_TESS_PHYSICS*CSB_TESS_PHYSICS];

	CBox3Dfp32 Box;
	for(int i = 0; i < 3; i++)
	{
		Box.m_Min.k[i] = _Pos.k[i] - _Radius;
		Box.m_Max.k[i] = _Pos.k[i] + _Radius;
	}

	if (m_spSBLink!=NULL)
	{
		uint16 SBBuffer[1024];
		int nSB = m_spSBLink->EnumBox(Box, SBBuffer, 1024, NULL, 0);
		for(int i = 0; i < nSB; i++)
		{
			int iSB = SBBuffer[i];
			CSplineBrush* pSB = m_lspSplineBrushes[iSB];

			if (!SphereInBoundBox(_Pos, _Radius, pSB->m_BoundBox.m_Min, pSB->m_BoundBox.m_Max)) continue;

			for(int iFace = 0; iFace < pSB->m_lFaces.Len(); iFace++)
			{
				CSplineBrush_Face* pF = &pSB->m_lFaces[iFace];
				if (!SphereInBoundBox(_Pos, _Radius, pF->m_BoundBox.m_Min, pF->m_BoundBox.m_Max)) continue;

				int nS = 1 << pF->m_PhysTessShiftS;
				int nT = 1 << pF->m_PhysTessShiftT;
				fp32 sStep = 1.0f / fp32(nS);
				fp32 tStep = 1.0f / fp32(nT);

	//	LogFile(CStrF("iSB %d, iF %d, nS %d, nT %d", iSB, iFace, nS, nT));

				int nST = pSB->EnumBoundNodes(iFace, Box, ST);
	//if (nST) ConOut(CStrF("nST %d, iSB %d, iF %d", nST, iSB, iFace));
				for(int iST = 0; iST < nST; iST++)
				{
					fp32 fS = fp32(ST[iST] & 0x7fff) * sStep;
					fp32 fT = fp32(ST[iST] >> 16) * tStep;
					CVec3Dfp32 Verts[4];
					pSB->EvalXYZ(iFace, fS, fT, Verts[0]);
					pSB->EvalXYZ(iFace, fS + sStep, fT, Verts[1]);
					pSB->EvalXYZ(iFace, fS + sStep, fT + tStep, Verts[2]);
					pSB->EvalXYZ(iFace, fS, fT + tStep, Verts[3]);

	//		ConOut(CStrF("      %f, %f", fS, fT));
					CCollisionInfo CInfo;
					// Check triangle1
					if (__Phys_IntersectSphere_Triangle(Verts[0], Verts[1], Verts[2], _Pos, _Radius, &CInfo))
					{
						if (!_pCollisionInfo)
							return true;

						CInfo.m_pSurface = m_lspSurfaces[pF->m_iSurface];
						if (_pCollisionInfo->Improve(CInfo))
						{
							if (_pCollisionInfo->IsComplete())
								return true;
						}
					}

					// Check triangle2
					if (__Phys_IntersectSphere_Triangle(Verts[0], Verts[2], Verts[3], _Pos, _Radius, &CInfo))
					{
						if (!_pCollisionInfo) return true;

						CInfo.m_pSurface = m_lspSurfaces[pF->m_iSurface];
						if (_pCollisionInfo->Improve(CInfo))
						{
							if (_pCollisionInfo->IsComplete())
								return true;
						}
					}
				}
			}
		}
	}

	if (_pCollisionInfo)
		return _pCollisionInfo->m_bIsCollision != 0;
	else
		return false;
}

// -------------------------------------------------------------------
// Physics interface overrides
// -------------------------------------------------------------------
void CXR_Model_BSP::Phys_GetBound_Sphere(const CMat4Dfp32& _Pos, CVec3Dfp32& _v0, fp32& _Radius)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_GetBound_Sphere, MAUTOSTRIP_VOID);
	_Radius = GetBound_Sphere();
	_v0 = CVec3Dfp32::GetMatrixRow(_Pos, 3);
}

void CXR_Model_BSP::Phys_GetBound_Box(const CMat4Dfp32& _Pos, CBox3Dfp32& _Box)
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

void CXR_Model_BSP::Phys_Init(CXR_PhysicsContext* _pPhysContext)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_Init, MAUTOSTRIP_VOID);
}

bool CXR_Model_BSP::__Phys_TraceLine_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MediumFlags, CPhysLineContext* _pLineContext, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_TraceLine_r, false);
	// Note: _pCollisionInfo must be valid here.


	if (!_iNode) return false;
//	if (!_iNode) return true;
	const CBSP_Node* pN = &m_pNodes[_iNode];

	bool bHit = false;

	if (pN->IsLeaf())
	{
//		if (m_lMediums[pN->m_iMedium].m_MediumFlags & _MediumFlags)
//			if (!_pCollisionInfo) return true;

		#ifdef PLATFORM_PS2
			// Setup some registers
			asm volatile ("
			
				    lq			$2,	0x00(%0)		# Load/align _p0
				    lq			$3, 0x10(%0)
				    mtsab		%0,	0
				    qfsrv		$2,	$3,	$2

				    qmtc2		%2,	vf21			# vf21 == 0.001f

				    lq			$4,	0x00(%1)		# Load/align _p1
				    lq			$5, 0x10(%1)
				    mtsab		%1,	0
				    qfsrv		$4,	$5,	$4

				    qmtc2		$2,	vf03			# vf03 == _p0
				    qmtc2		$4,	vf04			# vf04 == _p1

				    vmr32.w   	vf21, vf21
					vaddw.x		vf20, vf00, vf00w	# vf20.x == 1
					vsub		vf10, vf04, vf03	# vf10 = _p1 - _p0
				"
				:
				: "r" (&_p0), "r" (&_p1), "r" (0.001f)
				: "cc", "$2", "$3", "$4", "$5", "memory"
			);
		#endif
		
		int nf = pN->m_nFaces;
		int iif = pN->m_iiFaces;
		uint32* piFaces = &m_piFaces[iif];
		for(int f = 0; f < nf; f++)
		{
			int iFace = *piFaces++;
			const CBSP_Face* pF = &m_pFaces[iFace];
			int iMedium = pF->m_iBackMedium;
			if (!(m_lMediums[iMedium].m_MediumFlags & _MediumFlags)) continue;

			int nv = pF->m_nPhysV;
			int iiv = pF->m_iiPhysV;
			int iv0 = m_piVertices[iiv + nv - 1];

			const CPlane3Dfp32& P = m_pPlanes[pF->m_iPlane];
			fp32 d1 = P.Distance(_p0);
			if (d1 < 0.001f) continue;
			fp32 d2 = P.Distance(_p1);
			if (d2 > -0.001f) continue;	// If positive, the points must be on the same side.

		#ifdef PLATFORM_PS2
			asm volatile ("
			
				    lq			$2,	0x00(%3)    	# Load/align m_pVertices[iv0]
				    lq			$3, 0x10(%3)
				    mtsab		%3,	0
				    qfsrv		$2,	$3,	$2

				    qmtc2		$2,	vf01			# vf01 == m_pVertices[iv0]
				    
				vloop:

					lh			$3, 0x00(%0)		# $3 == iv1
					addi		%0, %0, 2			# m_piVertices++
					sll			$3, $3, 2  			# iv1 * sizeof(CVec3Dfp32)
					add			$4, $3, $3
					add			$3, $4, $3
					
					add			$2, %1, $3
					
				    lq			$4,	0x00($2)    	# Load/align m_pVertices[iv1]
				    lq			$5, 0x10($2)
				    mtsab		$2,	0
				    qfsrv		$3,	$5,	$4
				    qmtc2		$3,	vf02			# vf02 == m_pVertices[iv1]
				    
					vsub		vf05, vf02, vf01    # vf05 == Edge
					vsub		vf06, vf02, vf03    # vf06 == p0v1

					vopmula.xyz	ACC, vf05, vf06
					vopmsub.xyz	vf07, vf06, vf05    # vf07 == Normal

					vmul.xyz	vf08, vf07, vf10	# vf07.x = Plane.Distance(_p1)
					vadday.x 	ACC, vf08, vf08y
					vmaddz.x 	vf07, vf20, vf08z
					
					vmove		vf01, vf02
					
					vsub.x		vf07, vf07, vf21
					
					cfc2		$3, vi17			# get MAC flags
					andi		$3, $3, 0x80
					beqz		$3, floop
				    addi		%2, %2, -1
					
					bnez	 	%2, vloop
				"
				:
				: "r" (&m_piVertices[iiv]), "r" (m_pVertices), "r" (nv), "r" (&m_pVertices[iv0])
				: "cc", "$2", "$3", "$4", "$5", "memory"
			);
		#else
			int v;
			for(v = 0; v < nv; v++)
			{
				int iv1 = m_piVertices[iiv + v];
				CVec3Dfp32 Edge, p0v1, Normal;
				m_pVertices[iv1].Sub(m_pVertices[iv0], Edge);
				m_pVertices[iv1].Sub(_p0, p0v1);
				Edge.CrossProd(p0v1, Normal);
				CPlane3Dfp32 Plane(Normal, _p0);
				if (Plane.Distance(_p1) > 0.001f) break;

				iv0 = iv1;
			}

			if (v == nv)
		#endif
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
		#ifdef PLATFORM_PS2
			asm volatile ( "floop:" );
		#endif
		}
	}

	if (pN->IsStructureLeaf() && (m_spSBLink!=NULL))
	{
		// Test all spline-brushes in this leaf. The same splinebrush might be tested several times if it is inside
		// several leaves that are traced. To avoid this we can't clip the line to the bsp-splits and that would
		// probably have a greater performance impact, so we just stick to this solution. Besides, it's easier to
		// ignore duplicate traces than keeping track of which splines have been done.

		int iPL = pN->m_iPortalLeaf;
		static uint16 g_liSB[512];
		int nSB = m_spSBLink->EnumIDs(iPL, g_liSB, sizeof(g_liSB)/sizeof(g_liSB[0]), NULL, 0);
		for(int iiSB = 0; iiSB < nSB; iiSB++)
		{
			int iSB = g_liSB[iiSB];
			CSplineBrush* pSB = m_lspSplineBrushes[iSB];
			bHit |= pSB->Phys_IntersectLine(_pLineContext->m_V0, _p1, _MediumFlags, _pCollisionInfo);
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

		int iFront = pN->m_iNodeFront;
		int iBack = pN->m_iNodeBack;

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

bool CXR_Model_BSP::__Phys_TraceLine_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MediumFlags, CPhysLineContext* _pLineContext)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_TraceLine_r_2, false);
	MSCOPESHORT(CXR_Model_BSP::__Phys_TraceLine_r_2);

	if (!_iNode) 
		return true;
	const CBSP_Node* pN = &m_pNodes[_iNode];

	if (pN->IsLeaf())
	{
//		if ((m_lMediums[pN->m_iMedium].m_MediumFlags & _MediumFlags) != 0)
//			ConOut(CStrF("HitMedium %.8x at leaf %d", m_lMediums[pN->m_iMedium].m_MediumFlags, _iNode));
		return (m_lMediums[pN->m_iMedium].m_MediumFlags & _MediumFlags) != 0;
	}

	if (pN->IsStructureLeaf() && (m_spSBLink!=NULL))
	{
		// Test all spline-brushes in this leaf. The same splinebrush might be tested several times if it is inside
		// several leaves that are traced. To avoid this we can't clip the line to the bsp-splits and that would
		// probably have a greater performance impact, so we just stick to this solution. Besides, it's easier to
		// ignore duplicate traces than keeping track of which splines have been done.

		int iPL = pN->m_iPortalLeaf;
		static uint16 g_liSB[512];
		int nSB = m_spSBLink->EnumIDs(iPL, g_liSB, sizeof(g_liSB)/sizeof(g_liSB[0]), NULL, 0);
		for(int iiSB = 0; iiSB < nSB; iiSB++)
		{
			int iSB = g_liSB[iiSB];
			CSplineBrush* pSB = m_lspSplineBrushes[iSB];
			if (pSB->Phys_IntersectLine(_pLineContext->m_V0, _p1, _MediumFlags)) 
				return true;
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

		int Side = Side1 | Side2;
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

bool CXR_Model_BSP::Phys_IntersectLine(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_IntersectLine, false);
	MSCOPESHORT(CXR_Model_BSP::Phys_IntersectLine);

	CVec3Dfp32 v0, v1;
	_v0.MultiplyMatrix(_pPhysContext->m_WMatInv, v0);
	_v1.MultiplyMatrix(_pPhysContext->m_WMatInv, v1);
	m_pPlanes = m_lPlanes.GetBasePtr();
	m_pNodes = m_lNodes.GetBasePtr();

	CPhysLineContext LineContext;
	LineContext.m_V0 = v0;
	LineContext.m_V1 = v1;
	LineContext.m_InvV1V0 = 1.0f / v1.Distance(v0);

	if (_pCollisionInfo && (_pCollisionInfo->m_ReturnValues & CXR_COLLISIONRETURNVALUE_TIME))
	{
		bool bHit = __Phys_TraceLine_r(_pPhysContext, 1, v0, v1, _MediumFlags, &LineContext, _pCollisionInfo);
		if (!m_spSBLink)
			for(int iSB = 0; iSB < m_lspSplineBrushes.Len(); iSB++)
				bHit |= m_lspSplineBrushes[iSB]->Phys_IntersectLine(v0, v1, _MediumFlags, _pCollisionInfo);

		if (bHit)
		{
			_pCollisionInfo->m_Plane.Transform(_pPhysContext->m_WMat);
			_pCollisionInfo->m_Pos = _pCollisionInfo->m_LocalPos;
			_pCollisionInfo->m_Distance = _pCollisionInfo->m_Plane.Distance(_v1);
			_pCollisionInfo->m_Time = v0.Distance(_pCollisionInfo->m_Pos) / v0.Distance(v1);
			_pCollisionInfo->m_Pos *= _pPhysContext->m_WMat;
			_pCollisionInfo->m_bIsValid = true;
			_pCollisionInfo->m_bIsCollision = true;
//	ConOut(CStrF("HitBSP: Distance %f, Pos ", _pCollisionInfo->m_Distance) + _pCollisionInfo->m_Pos.GetString());
//			if (_pCollisionInfo->m_Time > 1.1f) ConOut(CStrF("(Phys_IntersectLine) Fuckat T %f", _pCollisionInfo->m_Time));
			return true;
		}
		return false;
	}
	else
	{
		if (__Phys_TraceLine_r(_pPhysContext, 1, v0, v1, _MediumFlags, &LineContext))
		{
			if(!_pCollisionInfo)
				return true;

			_pCollisionInfo->m_bIsCollision = true;
			_pCollisionInfo->m_bIsValid = false;
			return true;
		}

		if (!m_spSBLink)
		{
			const int nBrushes = m_lspSplineBrushes.Len();
			for(int iSB = 0; iSB < nBrushes; iSB++)
				if (m_lspSplineBrushes[iSB]->Phys_IntersectLine(v0, v1, _MediumFlags))
				{
					if(!_pCollisionInfo)
						return true;

					_pCollisionInfo->m_bIsCollision = true;
					_pCollisionInfo->m_bIsValid = false;
					return true;
				}
		}
		return false;
	}
}

bool CXR_Model_BSP::Phys_IntersectSphere(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_IntersectSphere, false);
	CVec3Dfp32 v0;
	_Dest.MultiplyMatrix(_pPhysContext->m_WMatInv, v0);
	m_pPlanes = m_lPlanes.GetBasePtr();
	m_pNodes = m_lNodes.GetBasePtr();
	if (_pCollisionInfo) _pCollisionInfo->m_bIsValid = false;

	int Medium = m_lMediums[m_pNodes[GetLeaf(v0)].m_iMedium].m_MediumFlags;
	if (Medium & _MediumFlags)
	{
		return true;
	}

	bool bImpact = __Phys_IntersectSphere_r(1, v0, _Radius, _MediumFlags, _pCollisionInfo);
	if (_pCollisionInfo || !bImpact)
		bImpact |= __Phys_IntersectSphere_SB(v0, _Radius, _MediumFlags, _pCollisionInfo);

	if (bImpact && _pCollisionInfo)
	{
		_pCollisionInfo->m_Plane.Transform(_pPhysContext->m_WMat);
		_pCollisionInfo->m_Pos = _pCollisionInfo->m_LocalPos;
		_pCollisionInfo->m_Pos *= _pPhysContext->m_WMat;
	}

	return bImpact;
}

bool CXR_Model_BSP::__Phys_IntersectOBB_CheckFace(int _iFace, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_IntersectOBB_CheckFace, false);
	CBSP_Face* pF = &m_lFaces[_iFace];
	int nv = pF->m_nPhysV;
	int iiv = pF->m_iiPhysV;

	CVec3Dfp32 Tri[3];

	CCollisionInfo CInfo;
	bool bIntersect = false;

	Tri[0] = m_pVertices[m_piVertices[iiv]];
	for(int v = 2; v < nv; v++)
	{
		Tri[1] = m_pVertices[m_piVertices[iiv + v -1]];
		Tri[2] = m_pVertices[m_piVertices[iiv + v]];

		CInfo.m_bIsCollision = false;
		if (Phys_Intersect_TriOBB(Tri, _BoxStart, _Box, false, (_pCollisionInfo) ? &CInfo : NULL) )
		{
			if (!_pCollisionInfo) return true;
			bIntersect = true;
			CInfo.m_pSurface = m_lspSurfaces[pF->m_iSurface];

			if (_pCollisionInfo->Improve(CInfo))
			{
				if (_pCollisionInfo->IsComplete())
					return true;
			}
		}
	}

	return bIntersect;
}


bool CXR_Model_BSP::__Phys_IntersectOBB(CXR_PhysicsContext* _pPhysContext, int _iNode, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, const CBox3Dfp32 _Bound, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	const int MaxFaces = 2048;
	uint32 liFaces[MaxFaces];

	int nFaces = EnumFaces_Box(_pPhysContext, _iNode, liFaces, MaxFaces, ENUM_HQ | ENUM_FACEFLAGS | ENUM_MEDIUMFLAGS, _MediumFlags, XW_FACE_PHYSICAL, _Bound);

	if (nFaces)
	{
		bool bIntersect = false;
		CCollisionInfo CInfo;

		for(int i = 0; i < nFaces; i++)
		{
			int iFace = liFaces[i];

			CInfo.m_bIsCollision = false;
			if (__Phys_IntersectOBB_CheckFace(iFace, _BoxStart, _Box, (_pCollisionInfo) ? &CInfo : NULL))
			{
				if (!_pCollisionInfo)
					return true;

				bIntersect = true;

				if (_pCollisionInfo->Improve(CInfo))
				{
					if (_pCollisionInfo->IsComplete())
						return true;
				}
			}
		}

		return bIntersect;
	}

	return false;
}

bool CXR_Model_BSP::__Phys_IntersectOBB_SB(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _BoxStart, const CPhysOBB& _BoxDest, const CBox3Dfp32 _Bound, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_IntersectOBB_SB, false);
	uint32 ST[CSB_TESS_PHYSICS*CSB_TESS_PHYSICS];

	if (m_spSBLink!=NULL)
	{
		uint16 SBBuffer[1024];
		int nSB = m_spSBLink->EnumBox(_Bound, SBBuffer, 1024, NULL, 0);
		for(int i = 0; i < nSB; i++)
		{
			int iSB = SBBuffer[i];
			CSplineBrush* pSB = m_lspSplineBrushes[iSB];
			bool bIsInside = pSB->m_BoundBox.IsInside(_Bound);

			if (_pPhysContext->m_pWC)
			{
				_pPhysContext->m_pWC->RenderAABB(_pPhysContext->m_WMat, pSB->m_BoundBox, (bIsInside) ? 0xff300030 : 0xff003000);
			}
			if (!bIsInside) continue;

			for(int iFace = 0; iFace < pSB->m_lFaces.Len(); iFace++)
			{
				CSplineBrush_Face* pF = &pSB->m_lFaces[iFace];
				if (!(m_lspSurfaces[pF->m_iSurface]->GetBaseFrame()->m_Medium.m_MediumFlags & _MediumFlags)) continue;

				if (!pF->m_BoundBox.IsInside(_Bound)) continue;

				int nS = 1 << pF->m_PhysTessShiftS;
				int nT = 1 << pF->m_PhysTessShiftT;
				fp32 sStep = 1.0f / fp32(nS);
				fp32 tStep = 1.0f / fp32(nT);

	//	LogFile(CStrF("iSB %d, iF %d, nS %d, nT %d", iSB, iFace, nS, nT));

				int nST = pSB->EnumBoundNodes(iFace, _Bound, ST);
	//if (nST) ConOut(CStrF("nST %d, iSB %d, iF %d", nST, iSB, iFace));
				for(int iST = 0; iST < nST; iST++)
				{
					fp32 fS = fp32(ST[iST] & 0x7fff) * sStep;
					fp32 fT = fp32(ST[iST] >> 16) * tStep;
					CVec3Dfp32 Verts[4];
					pSB->EvalXYZ(iFace, fS, fT, Verts[0]);
					pSB->EvalXYZ(iFace, fS + sStep, fT, Verts[1]);
					pSB->EvalXYZ(iFace, fS + sStep, fT + tStep, Verts[2]);
					pSB->EvalXYZ(iFace, fS, fT + tStep, Verts[3]);
/*
					if (ms_pPhysRC)
					{
						uint16 Wires[12] = { 0,1,1,2,2,0,0,2,2,3,3,0 };
						ms_pPhysRC->Matrix_Push();
						ms_pPhysRC->Matrix_Set(_pPhysContext->m_WMat);

						ms_pPhysRC->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
						ms_pPhysRC->Geometry_VertexArray(Verts, 4, true);
						ms_pPhysRC->Geometry_Color(0xff006060);				
						ms_pPhysRC->Render_IndexedWires(Wires, 12);
						ms_pPhysRC->Geometry_Clear();

						ms_pPhysRC->Matrix_Pop();
//						__Phys_RenderFace(iFace, _pPhysContext->m_WMat, ms_pPhysRC, 0xff006060);
					}
*/

	//		ConOut(CStrF("      %f, %f", fS, fT));
					CCollisionInfo CInfo;

					// Check triangle1
					// FIXME: Unnecessary data-copy.
					CVec3Dfp32 Tri[3];
					Tri[2] = Verts[0];
					Tri[1] = Verts[1];
					Tri[0] = Verts[2];

					CInfo.m_bIsCollision = false;
					if (Phys_Intersect_TriOBB(Tri, _BoxStart, _BoxDest, false, (_pCollisionInfo) ? &CInfo : NULL) )
					{
						if (!_pCollisionInfo) return true;

						CInfo.m_pSurface = m_lspSurfaces[pF->m_iSurface];
						if (_pCollisionInfo->Improve(CInfo))
						{
							if (_pCollisionInfo->IsComplete())
								return true;
						}
					}

					// Check triangle2
					// FIXME: Unnecessary data-copy.
					Tri[2] = Verts[0];
					Tri[1] = Verts[2];
					Tri[0] = Verts[3];

					CInfo.m_bIsCollision = false;
					if (Phys_Intersect_TriOBB(Tri, _BoxStart, _BoxDest, false, (_pCollisionInfo) ? &CInfo : NULL) )
					{
						if (!_pCollisionInfo) return true;

						CInfo.m_pSurface = m_lspSurfaces[pF->m_iSurface];
						if (_pCollisionInfo->Improve(CInfo))
						{
							if (_pCollisionInfo->IsComplete())
								return true;
						}
					}
				}
			}
		}
	}

	if (_pCollisionInfo)
		return _pCollisionInfo->m_bIsCollision != 0;
	else
		return false;
}

static CVec3Dfp32 ms_pcs_TransformedCenter;
static CBox3Dfp32 ms_pcs_TransformedBox;
static CBSP_BoxInterleaved ms_pcs_TransformedBoxInterleaved;
static fp32 ms_pcs_Radius;


void CXR_Model_BSP::CollectPCS(CXR_PhysicsContext* _pPhysContext, const uint8 _IN1N2Flags, CPotColSet *_pcs, const int _iObj, const int _MediumFlags )
{
	MSCOPESHORT(CXR_Model_BSP::CollectPCS);
	CBox3Dfp32 _Box; 	//oh this is just soo stupid!

	_pcs->GetBox( _Box.m_Min.k ); // and this is a very ugly way to set the bloody template box
	
	// set a bunch of static class variables!  (ms_ )
	ms_EnumBox = _Box;
	ms_EnumQuality = ENUM_HQ | ENUM_FACEFLAGS | ENUM_MEDIUMFLAGS;
	ms_EnumMedium = _MediumFlags;
	ms_EnumFaceFlags = XW_FACE_PHYSICAL;
	ms_iObj = _iObj;
	ms_pcs = _pcs;
	ms_bIN1N2Flags = _IN1N2Flags;

	// Transform PCS into BSP local space
	ms_pcs_TransformedCenter = *(CVec3Dfp32*)ms_pcs->m_fCenter;
	ms_pcs_TransformedCenter.MultiplyMatrix(_pPhysContext->m_WMatInv);
	ms_EnumBox.Transform(_pPhysContext->m_WMatInv, ms_pcs_TransformedBox);
	ms_pcs_TransformedBoxInterleaved.Init(ms_pcs_TransformedBox);
	ms_pcs_Radius = ms_pcs->m_fRadius;

	// recursevly DFS
	CollectPCS_r(_pPhysContext, 1);

	EnumFaces_Untag(_pPhysContext);


#ifdef PCSCHECK_INSOLID
	//AR-TEMP-ADD: Get medium in middle of box
	CVec3Dfp32 Center;
	_Box.m_Min.Lerp(_Box.m_Max, 0.5f, Center);
	int Medium = CXR_Model_BSP::Phys_GetMedium(_pPhysContext, Center);
	if (Medium & _MediumFlags)
		_pcs->m_bIsInSolid = true;
#endif
}


void CXR_Model_BSP::CollectPCS_r(CXR_PhysicsContext* _pPhysContext, int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_CollectPCS_r, MAUTOSTRIP_VOID);
	if (!_iNode)
		return; // empty tree

	const CBSP_Node* pN = &m_pNodes[_iNode];

	if (pN->IsLeaf())
	{
		CollectPCS_IsLeaf(_pPhysContext, pN);
	}


	if (pN->IsStructureLeaf() && (m_spSBLink!=NULL))
	{
		CollectPCS_IsStructureLeaf(_pPhysContext, pN->m_iPortalLeaf);
	}

	if (pN->IsNode())
	{
		const CPlane3Dfp32& W = m_pPlanes[pN->m_iPlane];
		const CVec3Dfp32& C = ms_pcs_TransformedCenter;
		const fp32 d = W.Distance(C);
		const fp32 R = ms_pcs_Radius;

		if (d > R)
		{ // positive side of partition plane
			CollectPCS_r(_pPhysContext, pN->m_iNodeFront);
			return;
		}	
		else if (d < -R)
		{ // negative side of partition plane
			CollectPCS_r(_pPhysContext, pN->m_iNodeBack);
			return;
		}

		// bound-sphere is split by plane - better check with the box..
		const CBSP_BoxInterleaved& B = ms_pcs_TransformedBoxInterleaved;
		fp32 MinDistance, MaxDistance;
		GetBoxMinMaxDistance(W, B, MinDistance, MaxDistance);

		if (MinDistance < MODEL_BSP_EPSILON)
			CollectPCS_r(_pPhysContext, pN->m_iNodeBack);

		if (MaxDistance > -MODEL_BSP_EPSILON)
			CollectPCS_r(_pPhysContext, pN->m_iNodeFront);
	}
}


void CXR_Model_BSP::CollectPCS_IsLeaf(CXR_PhysicsContext* _pPhysContext, const CBSP_Node* const _pNode)
{
	MSCOPESHORT(CXR_Model_BSP::CollectPCS_IsLeaf);

	int nFaces = _pNode->m_nFaces;
	int iiFaces = _pNode->m_iiFaces;

	uint8* pFTag = m_lFaceTagList.GetBasePtr();
	uint32* piFUntag = m_lFaceUntagList.GetBasePtr();
	const CXR_MediumDesc* pMediums = m_lMediums.GetBasePtr();
	const uint32* piFaces = &m_piFaces[iiFaces];

	for (int f = 0; f < nFaces; f++)
	{
		int iFace = piFaces[f];
		
		if (pFTag[iFace >> 3] & (1 << (iFace & 7))) // check if already processed
			continue;
			
		if (!pFTag[iFace >> 3])	// before we tag, we must mark it for later cleanup
			piFUntag[ms_nEnumUntag++] = iFace;
		
		pFTag[iFace >> 3] |= 1 << (iFace & 7);	// tag face as processed

		const CBSP_Face* const pF = &m_pFaces[iFace];

		if (!(ms_EnumFaceFlags & pF->m_Flags))
			continue;

		if ((ms_EnumQuality & ENUM_MEDIUMFLAGS) && !(pMediums[pF->m_iBackMedium].m_MediumFlags & ms_EnumMedium))
			continue;

		int iPlane = pF->m_iPlane;
		const CPlane3Dfp32& WOrig = m_pPlanes[iPlane];

		fp32 MinDist = WOrig.GetBoxMinDistance(ms_pcs_TransformedBox.m_Min, ms_pcs_TransformedBox.m_Max);
		if (MinDist > MODEL_BSP_EPSILON)
			continue;

		const int nv = pF->m_nVertices;
		const uint32* piVerts = m_piVertices + pF->m_iiVertices;

		// check face bound min/max
		{
			for (int k = 0; k < 3; k++)
			{
				fp32 PMin(_FP32_MAX), PMax(-_FP32_MAX);
				for (int v = 0; v < nv; v++)
				{
					const CVec3Dfp32& V = m_pVertices[piVerts[v]];
					PMin = Min(PMin, V.k[k]);
					PMax = Max(PMax, V.k[k]);
				}

				if (ms_pcs_TransformedBox.m_Min.k[k] > PMax) goto Continue;
				if (ms_pcs_TransformedBox.m_Max.k[k] < PMin) goto Continue;
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

			for (int v = 2; v < nv; v++)
			{ // extract all triangles and add them to PCS
				tri[2] = m_pVertices[piVerts[v]] * T;

				if (ms_pcs->ValidFace((const float *)&W, (const float *)tri))
					ms_pcs->AddFace( ms_bIN1N2Flags, ms_iObj, (const float *)&W, (const float *)tri, m_lspSurfaces[pF->m_iSurface] );

				tri[1] = tri[2];
			}
		}				

Continue:
		continue;
	}
}


void CXR_Model_BSP::CollectPCS_IsStructureLeaf(CXR_PhysicsContext* _pPhysContext, int _iPL)
{
	MSCOPESHORT(CXR_Model_BSP::CollectPCS_IsStructureLeaf);
	// may re-iterate splines! unacceptable -> mark done splinebrushes.

	static uint16 g_liSB[512];
	static uint32 ST[CSB_TESS_PHYSICS*CSB_TESS_PHYSICS];
	int nSB = m_spSBLink->EnumIDs(_iPL, g_liSB, sizeof(g_liSB)/sizeof(g_liSB[0]), NULL, 0);

	const CBox3Dfp32& Box = ms_EnumBox;

	for(int iiSB = 0; iiSB < nSB; iiSB++)
	{
		int iSB = g_liSB[iiSB];
		CSplineBrush* pSB = m_lspSplineBrushes[iSB];

		MSCOPESHORT(CollectPCS_SplineBrush);

		if (!pSB->m_BoundBox.IsInside(Box))
			continue;

		MPUSH(CollectPCS_SplineBrush_Inside); MPOP

		const int nFaces = pSB->m_lFaces.Len();
		const CSplineBrush_Face* const pFaces = pSB->m_lFaces.GetBasePtr();

		for(int iFace = 0; iFace < nFaces; iFace++)
		{
			const CSplineBrush_Face* const pF = &pFaces[iFace];

			if (!(m_lspSurfaces[pF->m_iSurface]->GetBaseFrame()->m_Medium.m_MediumFlags & ms_EnumMedium))
				continue;

			if (!pF->m_BoundBox.IsInside(Box))
				continue;

			const CMat4Dfp32& T = _pPhysContext->m_WMat;

			int nS = 1 << pF->m_PhysTessShiftS;
			int nT = 1 << pF->m_PhysTessShiftT;

			const fp32 sStep = 1.0f / fp32(nS);
			const fp32 tStep = 1.0f / fp32(nT);

			int nST = pSB->EnumBoundNodes(iFace, Box, ST);

			for(int iST = 0; iST < nST; iST++)
			{
				fp32 fS = fp32(ST[iST] & 0x7fff) * sStep;
				fp32 fT = fp32(ST[iST] >> 16) * tStep;

				CVec3Dfp32 Verts[4];
				float fPlaneEq[2][4];

				{
					pSB->EvalXYZ(iFace, fS, fT, Verts[0]);
					pSB->EvalXYZ(iFace, fS + sStep, fT, Verts[1]);
					pSB->EvalXYZ(iFace, fS + sStep, fT + tStep, Verts[2]);
					pSB->EvalXYZ(iFace, fS, fT + tStep, Verts[3]);

					for (int iV=0; iV<4; iV++)
						Verts[iV] *= T;
				}

				{
					CVec3Dfp32 u, v;

					v = Verts[2] - Verts[0];
					u = Verts[1] - Verts[0];

					u.CrossProd( v, *(CVec3Dfp32*)fPlaneEq[0] );
					((CVec3Dfp32*)fPlaneEq[0])->Normalize();
					fPlaneEq[0][3] = -(Verts[0] * *(CVec3Dfp32 *)fPlaneEq[0]);

					v = Verts[3] - Verts[0];
					u = Verts[1] - Verts[0];

					u.CrossProd( v, *(CVec3Dfp32*)fPlaneEq[1] );
					((CVec3Dfp32*)fPlaneEq[1])->Normalize();
					fPlaneEq[1][3] = -(Verts[0] * *(CVec3Dfp32 *)fPlaneEq[1]);
				}

				if (ms_pcs->ValidPlane(fPlaneEq[0]))
				{
					CVec3Dfp32 x[3];
					x[0] = Verts[2]; x[1] = Verts[1]; x[2] = Verts[0];
					
					if (ms_pcs->ValidFace( fPlaneEq[0], (float *)x ))
					{
						ms_pcs->AddFace( ms_bIN1N2Flags, ms_iObj, fPlaneEq[0], (float *)x, m_lspSurfaces[pF->m_iSurface] );
					}
				}

				if (ms_pcs->ValidPlane( fPlaneEq[1] ))
				{
					CVec3Dfp32 x[3];
					x[0] = Verts[3]; x[1] = Verts[2]; x[2] = Verts[0];

					if( ms_pcs->ValidFace( fPlaneEq[1], (float *)x ) )
					{
						ms_pcs->AddFace( ms_bIN1N2Flags, ms_iObj, fPlaneEq[1], (float *)x, m_lspSurfaces[pF->m_iSurface] );
					}
				}
			}
		}
	}
}




bool CXR_Model_BSP::Phys_IntersectBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _BoxOrigin, const CPhysOBB& _BoxDest, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_IntersectBox, false);
	MSCOPESHORT(CXR_Model_BSP::Phys_IntersectBox); //AR-SCOPE

	CPhysOBB LBoxOrigin, LBoxDest;

	_BoxOrigin.Transform( _pPhysContext->m_WMatInv, LBoxOrigin );
	_BoxDest.Transform( _pPhysContext->m_WMatInv, LBoxDest );
	
	m_pPlanes = m_lPlanes.GetBasePtr();
	m_pNodes = m_lNodes.GetBasePtr();

	
	if (_pCollisionInfo)
		_pCollisionInfo->m_bIsCollision = false;


	// Get bounding box for both OBBs.
	CBox3Dfp32 BoundBoxOrigin, BoundBoxDest;

	LBoxOrigin.GetBoundBox( BoundBoxOrigin );
	LBoxDest.GetBoundBox( BoundBoxDest );

	BoundBoxOrigin.Expand( BoundBoxDest );
	BoundBoxOrigin.m_Min -= CVec3Dfp32( 1.f );
	BoundBoxOrigin.m_Max += CVec3Dfp32( 1.f );

	bool bImpact = __Phys_IntersectOBB(_pPhysContext, 1, LBoxOrigin, LBoxDest, BoundBoxOrigin, _MediumFlags, _pCollisionInfo);

	if (_pCollisionInfo || !bImpact)
	{
		bImpact |= __Phys_IntersectOBB_SB(_pPhysContext, LBoxOrigin, LBoxDest, BoundBoxOrigin, _MediumFlags, _pCollisionInfo);
	}


	if (!bImpact)
	{
		CVec3Dfp32 Center;

		BoundBoxDest.m_Min.Lerp(BoundBoxDest.m_Max, 0.5f, Center);

		int iLeaf = GetLeaf(Center);

		int Medium = (!iLeaf) ? XW_MEDIUM_SOLID : m_lMediums[m_lNodes[iLeaf].m_iMedium].m_MediumFlags;

		if (Medium & _MediumFlags)
			bImpact = true;
	}


	if (_pCollisionInfo)
	{
		if (_pCollisionInfo->m_bIsValid)
		{
			_pCollisionInfo->m_Plane.Transform(_pPhysContext->m_WMat);
			_pCollisionInfo->m_Pos = _pCollisionInfo->m_LocalPos;
			_pCollisionInfo->m_Pos *= _pPhysContext->m_WMat;
		}
	}

	return bImpact;
}



int CXR_Model_BSP::Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_GetMedium, 0);

	CVec3Dfp32 v0;
	_v0.MultiplyMatrix(_pPhysContext->m_WMatInv, v0);
	int iLeaf = GetLeaf(v0);
	if (!iLeaf) return XW_MEDIUM_SOLID;
	return m_lMediums[m_lNodes[iLeaf].m_iMedium].m_MediumFlags;
}

void CXR_Model_BSP::Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, CXR_MediumDesc& _RetMedium)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_GetMedium_2, MAUTOSTRIP_VOID);

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

int CXR_Model_BSP::__Phys_GetCombinedMediumFlags_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CBox3Dfp32* _pBox)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_GetCombinedMediumFlags_r, 0);
	const CBSP_Node* pN = &m_pNodes[_iNode];

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

int CXR_Model_BSP::Phys_GetCombinedMediumFlags(CXR_PhysicsContext* _pPhysContext, const CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_GetCombinedMediumFlags, 0);
	return __Phys_GetCombinedMediumFlags_r(_pPhysContext, 1, &_Box);
}


