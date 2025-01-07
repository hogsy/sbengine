#include "PCH.h"
#include "WBSP4Glass.h"

#include "../../Classes/Render/MWireContainer.h"


#if GLASS_OPTIMIZE_OFF
#pragma xrMsg("optimize off!")
#pragma optimize("", off)
#pragma inline_depth(0)
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_BSP4Glass::Phys_IntersectSphere
|__________________________________________________________________________________________________
\*************************************************************************************************/
bool CXR_Model_BSP4Glass::Phys_IntersectSphere(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	CVec3Dfp32 v0, v1;
	_Origin.MultiplyMatrix(_pPhysContext->m_WMatInv, v0);
	_Dest.MultiplyMatrix(_pPhysContext->m_WMatInv, v1);

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

	bool bImpact = __PhysGlass_IntersectSphere_r(_pPhysContext, 1, v0, v1, _Radius, _MediumFlags, _pCollisionInfo);

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


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_BSP4Glass::__PhysGlass_IntersectSphere_r
|__________________________________________________________________________________________________
\*************************************************************************************************/
bool CXR_Model_BSP4Glass::__PhysGlass_IntersectSphere_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo) const
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::__PhysGlass_IntersectSphere_r, XR_BSP4GLASS);

	if(!_iNode)
		return false;

	CBSP4_Node* pN = &m_pNodes[_iNode];
//	CCollisionInfo CInfo;
	bool bIntersect = false;

	if(pN->IsLeaf())
	{
		if(m_pMediums[pN->m_iMedium].m_MediumFlags & _MediumFlags)
			if(!_pCollisionInfo) return true;

		int nFaces = pN->m_nFaces;
		int iiFaces = pN->m_iiFaces;

		for(int i = 0; i < nFaces; i++)
		{
			int iFace = m_piFaces[iiFaces + i];

			CBSP4_Face* pF = &m_pFaces[iFace];
			if(!(pF->m_Flags & XW_FACE_PHYSICAL)) continue;
			if(pF->m_Flags & XW_FACE_VOLUMEINSIDE) continue;
			else
				if(!(m_pMediums[pF->m_iBackMedium].m_MediumFlags & _MediumFlags)) continue;

			if (_pPhysContext && _pPhysContext->m_pAnimState && _pPhysContext->m_pAnimState->m_pModelInstance)
			{
				CXR_Model_BSP4Glass_Instance* pGlassInstance = (CXR_Model_BSP4Glass_Instance*)_pPhysContext->m_pAnimState->m_pModelInstance;
				bIntersect = __PhysGlass_IntersectSphere(_pPhysContext, iFace, (int32)m_piFaceInstance[iFace], _Origin, _Dest, _Radius, _pCollisionInfo);
			}
			else
			{
				M_ASSERT(0, "CXR_Model_BSP4Glass::__PhysGlass_IntersectSphere_r: Something has gone terribly wrong!");
				/*
				if(__PhysGlass_IntersectSphere_CheckFace(iFace, true, _Origin, _Dest, _Radius, &CInfo))
				{
					bIntersect = true;
					if(!_pCollisionInfo) return true;
					_pCollisionInfo->Improve(CInfo);
				}
				if(__PhysGlass_IntersectSphere_CheckFace(iFace, false, _Origin, _Dest, _Radius, &CInfo))
				{
					bIntersect = true;
					if(!_pCollisionInfo) return true;
					_pCollisionInfo->Improve(CInfo);
				}
				*/
			}

			if (bIntersect)
			{
				if (!_pCollisionInfo)
					return true;

//				if (_pCollisionInfo->Improve(CInfo))
//					_pCollisionInfo->m_LocalNode = m_piFaceInstance[iFace];
			}
		}

		if(m_pMediums[pN->m_iMedium].m_MediumFlags & _MediumFlags) return true;
		return bIntersect;
	}
	else
	{
		int iPlane = m_pNodes[_iNode].m_iPlane;
		if(!iPlane) Error("__PhysGlass_IntersectSphere_r", "Internal error.");
		CPlane3Dfp32* pP = &m_pPlanes[iPlane];
		fp32 d = pP->Distance(_Dest);

		bool bIntersect1 = false;
		bool bIntersect2 = false;
		if(d > -_Radius)
			if((bIntersect1 = __PhysGlass_IntersectSphere_r(_pPhysContext, m_pNodes[_iNode].m_iNodeFront, _Origin, _Dest, _Radius, _MediumFlags, _pCollisionInfo)))
				if(!_pCollisionInfo) return true;

		if(d < _Radius)
			if((bIntersect2 = __PhysGlass_IntersectSphere_r(_pPhysContext, m_pNodes[_iNode].m_iNodeBack, _Origin, _Dest, _Radius, _MediumFlags, _pCollisionInfo)))
				if(!_pCollisionInfo) return true;

		if(!_pCollisionInfo) return false;
		return (bIntersect1 || bIntersect2);
	}
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_BSP4Glass::__PhysGlass_IntersectSphere
|__________________________________________________________________________________________________
\*************************************************************************************************/
bool CXR_Model_BSP4Glass::__PhysGlass_IntersectSphere(CXR_PhysicsContext* _pPhysContext, uint _iFace, int32 _iInstance, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, CCollisionInfo* _pCollisionInfo) const
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::__PhysGlass_IntersectSphere_CheckFace, XR_BSP4GLASS);
	if(_iInstance >= (uint16)-1)
		return false;

	CXR_Model_BSP4Glass_Instance* pGlassInstance = (CXR_Model_BSP4Glass_Instance*)_pPhysContext->m_pAnimState->m_pModelInstance;
	CGlassModel& Model = pGlassInstance->m_lModels[_iInstance];
	if (Model.m_Attrib.Attrib_NoPhys())
	{
		if (pGlassInstance->Phys_SphereIntersect(_iInstance, _Origin, _Dest, _Radius, _pCollisionInfo, _pPhysContext->m_WMat))
		{
			if (_pCollisionInfo)
			{
				_pCollisionInfo->m_pSurface = m_lspSurfaces[pGlassInstance->GetSurface(_iInstance)];
				_pCollisionInfo->m_LocalNode = _iInstance;
			}

			return true;
		}
		return false;
	}

	return __PhysGlass_IntersectSphere_CheckFace(_iFace, true, _Origin, _Dest, _Radius, _pCollisionInfo);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_BSP4Glass::__PhysGlass_IntersectSphere_CheckFace
|__________________________________________________________________________________________________
\*************************************************************************************************/
bool CXR_Model_BSP4Glass::__PhysGlass_IntersectSphere_CheckFace(uint _iFace, bool _bTrueFace, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, CCollisionInfo* _pCollisionInfo) const
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::__PhysGlass_IntersectSphere_CheckFace, XR_BSP4GLASS);
	CBSP4_Face* pF = &m_pFaces[_iFace];

	uint iPlane = pF->m_iPlane;
	CPlane3Dfp32 Plane = (_bTrueFace) ? m_pPlanes[iPlane] : CPlane3Dfp32(-m_pPlanes[iPlane].n, m_pMediums[pF->m_iBackMedium].m_Thickness - m_pPlanes[iPlane].d);
	fp32 d = Plane.Distance(_Dest);

	if (d < -_Radius) return false;
	if (d > _Radius) return false;

	CPlane3Dfp32 Edge_Plane(CVec3Dfp32(0, 0, 1), 0);
	uint Edge_bCollided = false;
	fp32 Edge_Nearest = _Radius*2.0f;
	CVec3Dfp32 Edge_Pos = 0;
	CVec3Dfp32 VertexOffset = 0;

	uint nv = pF->m_nVertices;
	int iv = 0;
	uint iiv = pF->m_iiVertices;

	uint InsideBit = 1;
	uint bAllInside = nv;
	uint InsideMask = 0;

	uint ivL = iiv + nv - 1;
	if (!_bTrueFace)
	{
		Swap(ivL, iiv);
		iv = -(int)(nv-1);
		VertexOffset = Plane.n * m_pMediums[pF->m_iBackMedium].m_Thickness;
	}

	uint iv1 = m_piVertices[ivL];
	uint iv0 = 0;
	for(uint v = 0; v < nv; v++, iv1 = iv0)
	{
		iv0 = m_piVertices[iiv + (iv + v)];

		const CVec3Dfp32 iv0Vert = m_pVertices[iv0] + VertexOffset;
		const CVec3Dfp32 iv1Vert = m_pVertices[iv1] + VertexOffset;

		CVec3Dfp32 vs = _Dest - iv0Vert;
		CVec3Dfp32 ve = iv1Vert - iv0Vert;
		fp32 len_e = ve.Length();
		CVec3Dfp32 n = (vs/ve);
		fp32 dist_e = n.Length() / len_e;

		fp32 Side = n*Plane.n;
		if (Side > 0.0f)
		{
			bAllInside = false;
			fp32 t = (vs*ve) / (ve*ve);
			if (t < 0.0f)
			{
				fp32 l = vs.Length();
				if (l > _Radius) continue;
				if (!_pCollisionInfo) return true;
				if (l < Edge_Nearest)
				{
					Edge_Nearest = l;
					Edge_Plane.CreateNV(vs.Normalize(), iv0Vert);
					Edge_Pos = iv0Vert;
				}
				Edge_bCollided = true;
			}
			else if (t > 1.0f)
			{
				CVec3Dfp32 vs2 = _Dest - iv1Vert;
				fp32 l = vs2.Length();
				if (l > _Radius) continue;
				if (!_pCollisionInfo) return true;
				if (l < Edge_Nearest)
				{
					Edge_Nearest = l;
					Edge_Plane.CreateNV(vs2.Normalize(), iv1Vert);
					Edge_Pos = iv1Vert;
				}
				Edge_bCollided = true;
			}
			else 
			{
				fp32 l = Abs(dist_e);
				if (l > _Radius) return false;
				if (!_pCollisionInfo) return true;
				if (l < Edge_Nearest)
				{
					Edge_Nearest = l;

					CVec3Dfp32 vsproj;
					vs.Project(ve, vsproj);
					Edge_Plane.CreateNV((vs - vsproj).Normalize(), iv0Vert);
					Edge_Pos = iv0Vert + ve*t;
				}
				Edge_bCollided = true;
			}
		}
		else
			InsideMask |= InsideBit;

		InsideBit <<= 1;
	}

	if (!_pCollisionInfo)
	{
		return Edge_bCollided || bAllInside;
	}

	CCollisionInfo CInfo;
	if (bAllInside)
	{
		CInfo.m_bIsValid = true;
		CInfo.m_bIsCollision = true;
		CInfo.m_Plane = Plane;
		CInfo.m_Distance = d - _Radius;
		CInfo.m_LocalPos = _Dest - CInfo.m_Plane.n*d;
		CInfo.m_Velocity = 0;
		CInfo.m_pSurface = m_lspSurfaces[pF->m_iSurface];
	}
	else
	{
		if (!Edge_bCollided) return false;
		CInfo.m_bIsValid = true;
		CInfo.m_bIsCollision = true;
		CInfo.m_Plane = Edge_Plane;
		CInfo.m_Distance = Edge_Nearest - _Radius;
		CInfo.m_LocalPos = Edge_Pos;
		CInfo.m_Velocity = 0;
		CInfo.m_pSurface = m_lspSurfaces[pF->m_iSurface];
	}

	{
		fp32 Time = 0.0f;
		const CVec3Dfp32& n = CInfo.m_Plane.n;
		CVec3Dfp32 RelOrig, Dir;
		_Dest.Sub(_Origin, Dir);
		fp32 Den = n * Dir;
		if (M_Fabs(Den) > 0.001f)
		{
			fp32 d = -_Radius;						// Plane in collision point, offseted by radius
			_Origin.Sub(CInfo.m_LocalPos, RelOrig);
			Time = -(d + n * RelOrig) / Den;
		}
		CInfo.m_Time = Clamp01(Time);
		if (_pCollisionInfo->Improve(CInfo))
			_pCollisionInfo->m_LocalNode = m_piFaceInstance[_iFace];
	}
	return true;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_BSP4Glass_Instance::Phys_SphereIntersect
|__________________________________________________________________________________________________
\*************************************************************************************************/
bool CXR_Model_BSP4Glass_Instance::Phys_SphereIntersect(const int32 _iInstance, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, CCollisionInfo* _pCollisionInfo, const CMat4Dfp32& _WMat)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass_Instance::Phys_SphereIntersect, XR_BSP4GLASS);
	
	if (_iInstance >= GLASS_MAX_INSTANCE)
		return false;

	CGlassModel* pModels = m_lModels.GetBasePtr();
	CGlassModel& Model = pModels[_iInstance];
	if (!Model.m_Grid.IsValid())
		return false;

	CBSP4Glass_Grid_CollisionInfo GCInfo = Model.m_Grid.Phys_SphereIntersectGrid(Model.m_Attrib.m_Plane, _Origin, _Dest, _Radius, _pCollisionInfo, _WMat);
	if (_pCollisionInfo)
		return GCInfo.IsValid();

	return GCInfo.m_bValid;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_Grid::Phys_SphereIntersectGrid
|__________________________________________________________________________________________________
\*************************************************************************************************/
CBSP4Glass_Grid_CollisionInfo CBSP4Glass_Grid::Phys_SphereIntersectGrid(const CPlane3Dfp32& _Plane, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, const fp32 _Radius, CCollisionInfo* _pCollisionInfo, const CMat4Dfp32& _WMat)
{
	GLASS_MSCOPE(CBSP4Glass_Grid::Phys_SphereIntersectGrid, XR_BSP4GLASS);
	
	bool bTrueFace = (_Plane.Distance(_Dest) >= 0.0f) ? true : false;
	
	CPlane3Dfp32 Plane = (bTrueFace) ? _Plane : CPlane3Dfp32(-_Plane.n, 0.0f - _Plane.d);
	fp32 d = Plane.Distance(_Dest);
	if (M_Fabs(d) > _Radius)
		return CBSP4Glass_Grid_CollisionInfo();

	CVec3Dfp32 Origin = _Origin;
	CVec3Dfp32 Dest = _Dest;

	const TAP_RCD<CVec3Dfp32> lGridPoints = m_lGridPoints;
	CBSP4Glass_Grid_PhysInfo PhysInfo;
	CBSP4Glass_Grid_CollisionInfo GCInfo;
	Phys_SetupIntersectGrid(Dest, &_Radius, _WMat, PhysInfo);
	
	const int32 Start0 = PhysInfo.m_Start.k[0];
	const int32 Start1 = PhysInfo.m_Start.k[1];
	const int32 End0   = PhysInfo.m_End.k[0];
	const int32 End1   = PhysInfo.m_End.k[1];

	const uint32 nPointsX = m_nQuadsX + 1;
	for(uint32 y = Start1; y < End1; y++)
	{
		uint32 i = (y * m_nQuadsX) + Start0;
		uint32 iPoint0 = (y * nPointsX) + Start0;

		for(uint32 x = Start0; x < End0; x++, i++, iPoint0++)
		{
			// Check if grid is valid
			if(m_GridData.Get(i,m_GridData.GetLastBit()))
			{
				uint bAllInside = 4;
				uint InsideMask = 0;
				uint InsideBit = 1;

				CPlane3Dfp32 Edge_Plane(CVec3Dfp32(0, 0, 1), 0);
				uint Edge_bCollided = false;
				fp32 Edge_Nearest = _Radius*2.0f;
				CVec3Dfp32 Edge_Pos = 0;
				CVec3Dfp32 VertexOffset = 0;

				const uint32 iPoint1 = iPoint0 + nPointsX;
				const uint liVertices0[] = { iPoint0, iPoint1, iPoint1+1, iPoint0+1, NULL };
				const uint liVertices1[] = { iPoint0, iPoint0+1, iPoint1+1, iPoint1, NULL };
				const uint* piVertices = (bTrueFace) ? liVertices0 : liVertices1;

				uint iv1 = 3;
				uint iv0 = 0;
				for(uint v = 0; v < 4; v++, iv1 = iv0)
				{
					iv0 = v;

					const CVec3Dfp32 iv0Vert = (lGridPoints[piVertices[iv0]]);
					const CVec3Dfp32 iv1Vert = (lGridPoints[piVertices[iv1]]);

					CVec3Dfp32 vs = Dest - iv0Vert;
					CVec3Dfp32 ve = iv1Vert - iv0Vert;
					fp32 len_e = ve.Length();
					CVec3Dfp32 n = (vs/ve);
					fp32 dist_e = n.Length() / len_e;

					fp32 Side = n*Plane.n;
					if (Side > 0.0f)
					{
						bAllInside = false;
						fp32 t = (vs*ve) / (ve*ve);
						if (t < 0.0f)
						{
							fp32 l = vs.Length();
							if (l > _Radius) continue;
							if (!_pCollisionInfo) return CBSP4Glass_Grid_CollisionInfo(true, -1, -1, -1);
							if (l < Edge_Nearest)
							{
								Edge_Nearest = l;
								Edge_Plane.CreateNV(vs.Normalize(), iv0Vert);
								Edge_Pos = iv0Vert;
							}
							Edge_bCollided = true;
						}
						else if (t > 1.0f)
						{
							CVec3Dfp32 vs2 = Dest - iv1Vert;
							fp32 l = vs2.Length();
							if (l > _Radius) continue;
							if (!_pCollisionInfo) return CBSP4Glass_Grid_CollisionInfo(true, -1, -1, -1);
							if (l < Edge_Nearest)
							{
								Edge_Nearest = l;
								Edge_Plane.CreateNV(vs2.Normalize(), iv1Vert);
								Edge_Pos = iv1Vert;
							}
							Edge_bCollided = true;
						}
						else 
						{
							fp32 l = Abs(dist_e);
							if (l > _Radius) break;
							if (!_pCollisionInfo) return CBSP4Glass_Grid_CollisionInfo(true, -1, -1, -1);
							if (l < Edge_Nearest)
							{
								Edge_Nearest = l;

								CVec3Dfp32 vsproj;
								vs.Project(ve, vsproj);
								Edge_Plane.CreateNV((vs - vsproj).Normalize(), iv0Vert);
								Edge_Pos = iv0Vert + ve*t;
							}
							Edge_bCollided = true;
						}
					}
					else
						InsideMask |= InsideBit;

					InsideBit <<= 1;
				}

				if (!_pCollisionInfo)
				{
					if (Edge_bCollided || bAllInside) return CBSP4Glass_Grid_CollisionInfo(true, -1, -1, -1);
					continue;
				}

				CCollisionInfo CInfo;
				if (bAllInside)
				{
					CInfo.m_bIsValid = true;
					CInfo.m_bIsCollision = true;
					CInfo.m_Plane = Plane;
					CInfo.m_Distance = d - _Radius;
					CInfo.m_LocalPos = _Dest - CInfo.m_Plane.n*d;
					CInfo.m_Velocity = 0;
					//CInfo.m_pSurface = m_lspSurfaces[pF->m_iSurface];
				}
				else
				{
					if (!Edge_bCollided) continue;
					CInfo.m_bIsValid = true;
					CInfo.m_bIsCollision = true;
					CInfo.m_Plane = Edge_Plane;
					CInfo.m_Distance = Edge_Nearest - _Radius;
					CInfo.m_LocalPos = Edge_Pos;
					CInfo.m_Velocity = 0;
					//CInfo.m_pSurface = m_lspSurfaces[pF->m_iSurface];
				}

				{
					fp32 Time = 0.0f;
					const CVec3Dfp32& n = CInfo.m_Plane.n;
					CVec3Dfp32 RelOrig, Dir;
					Dest.Sub(Origin, Dir);
					fp32 Den = n * Dir;
					if (M_Fabs(Den) > 0.001f)
					{
						fp32 d = -_Radius;						// Plane in collision point, offseted by radius
						Origin.Sub(CInfo.m_LocalPos, RelOrig);
						Time = -(d + n * RelOrig) / Den;
					}
					CInfo.m_Time = Clamp01(Time);
					if (_pCollisionInfo->Improve(CInfo))
					{
						GCInfo.m_bValid = true;
						GCInfo.m_iInside = i;
						GCInfo.m_iHitX = x;
						GCInfo.m_iHitY = y;
					}
				}
			}
		}
	}

	return GCInfo;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_Grid::Phys_EnumSphere
|__________________________________________________________________________________________________
\*************************************************************************************************/
uint32 CBSP4Glass_Grid::Phys_EnumSphere(const CVec3Dfp32& _LocalPos, fp32 _Radius, const CMat4Dfp32& _WMat, TThinArray<uint32>& _lReturn)
{
	GLASS_MSCOPE(CBSP4Glass_Grid::Phys_EnumSphere, XR_BSP4GLASS);

	uint32 nResult = 0;
	CPlane3Dfp32 _Plane(m_PlaneN, m_PlaneD);
	bool bTrueFace = (_Plane.Distance(_LocalPos) >= 0.0f) ? true : false;

	CPlane3Dfp32 Plane = (bTrueFace) ? _Plane : CPlane3Dfp32(-_Plane.n, 0.0f - _Plane.d);
	fp32 d = Plane.Distance(_LocalPos);
	if (M_Fabs(d) > _Radius)
		return nResult;

	const TAP_RCD<CVec3Dfp32> lGridPoints = m_lGridPoints;
	CBSP4Glass_Grid_PhysInfo PhysInfo;
	Phys_SetupIntersectGrid(_LocalPos, &_Radius, _WMat, PhysInfo);

	const int32 Start0 = PhysInfo.m_Start.k[0];
	const int32 Start1 = PhysInfo.m_Start.k[1];
	const int32 End0   = PhysInfo.m_End.k[0];
	const int32 End1   = PhysInfo.m_End.k[1];

	int32 nMaxReturn = MaxMT(0, End0 - Start0) * MaxMT(0, End1 - Start1);
	_lReturn.SetLen(nMaxReturn);
	uint32* pResult = _lReturn.GetBasePtr();

	const uint32 nPointsX = m_nQuadsX + 1;
	for(uint32 y = Start1; y < End1; y++)
	{
		uint32 i = (y * m_nQuadsX) + Start0;
		uint32 iPoint0 = (y * nPointsX) + Start0;

		for(uint32 x = Start0; x < End0; x++, i++, iPoint0++)
		{
			// Check if grid is valid
			if(m_GridData.Get(i,m_GridData.GetLastBit()))
			{
				const uint32 iPoint1 = iPoint0 + nPointsX;
				const uint liVertices0[] = { iPoint0, iPoint1, iPoint1+1, iPoint0+1, NULL };
				const uint liVertices1[] = { iPoint0, iPoint0+1, iPoint1+1, iPoint1, NULL };
				const uint* piVertices = (bTrueFace) ? liVertices0 : liVertices1;

				uint iv1 = 3;
				uint iv0 = 0;
				for(uint v = 0; v < 4; v++, iv1 = iv0)
				{
					iv0 = v;

					const CVec3Dfp32 iv0Vert = (lGridPoints[piVertices[iv0]]);
					const CVec3Dfp32 iv1Vert = (lGridPoints[piVertices[iv1]]);

					CVec3Dfp32 vs = _LocalPos - iv0Vert;
					CVec3Dfp32 ve = iv1Vert - iv0Vert;
					CVec3Dfp32 n = (vs/ve);

					fp32 Side = n*Plane.n;
					if (Side > 0.0f)
					{
						fp32 t = (vs*ve) / (ve*ve);
						if (t < 0.0f)
						{
							fp32 l = vs.Length();
							if (l > _Radius) continue;
							pResult[nResult++] = i;
						}
						else if (t > 1.0f)
						{
							CVec3Dfp32 vs2 = _LocalPos - iv1Vert;
							fp32 l = vs2.Length();
							if (l > _Radius) continue;
							pResult[nResult++] = i;
						}
						else 
						{
							fp32 l = Abs(n.Length() / ve.Length());
							if (l > _Radius) break;
							pResult[nResult++] = i;
						}
					}
				}
			}
		}
	}

	return nResult;
}

