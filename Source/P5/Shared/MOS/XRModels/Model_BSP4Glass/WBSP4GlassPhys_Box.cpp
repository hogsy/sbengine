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
| CXR_Model_BSP4Glass::Phys_IntersectBox
|__________________________________________________________________________________________________
\*************************************************************************************************/
bool CXR_Model_BSP4Glass::Phys_IntersectBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _BoxOrigin, const CPhysOBB& _BoxDest, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::Phys_IntersectBox, XR_BSP4GLASS);

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

	if (_pPhysContext->m_pWC)
	{
		_pPhysContext->m_pWC->RenderAABB(_pPhysContext->m_WMat, BoundBoxOrigin, 0xff3030ff, 0.0f, false);

		CBox3Dfp32 Box;
		GetBound_Box(Box);

		_pPhysContext->m_pWC->RenderAABB(_pPhysContext->m_WMat, Box, 0xff300000, 0.0f, false);
	}


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
		bool bImpact = __PhysGlass_IntersectOBB_i(_pPhysContext, 1, LBoxOrigin, LBoxDest, BoundBoxOrigin, _MediumFlags, _pCollisionInfo);

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

		if(__PhysGlass_IntersectOBB_i(_pPhysContext, 1, LBoxOrigin, LBoxDest, BoundBoxOrigin, _MediumFlags, _pCollisionInfo))
			return true;
	}

	return false;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_BSP4Glass::__PhysGlass_IntersectOBB_i
|__________________________________________________________________________________________________
\*************************************************************************************************/
bool CXR_Model_BSP4Glass::__PhysGlass_IntersectOBB_i(CXR_PhysicsContext* _pPhysContext, int _iNode, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, const CBox3Dfp32& _Bound, int _MediumFlags, CCollisionInfo* _pCollisionInfo) const
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::__PhysGlass_IntersectOBB_i, XR_BSP4GLASS);
	if (!_iNode)
	{
		return false;
	}

	bool bIntersect = false;
	uint32 aWorkingSet[128];
	int WorkingStack = 0;
	aWorkingSet[WorkingStack++] = _iNode;

	TObjectPoolAllocator<CBSP4_EnumContext> EnumAlloc(m_spEnumContextPool);
	CBSP4_EnumContext* pEnumContext = EnumAlloc.GetObject();
	pEnumContext->Create(m_lFaces.Len(), 0);

	CXR_ModelInstance* pModelInstance = (_pPhysContext && _pPhysContext->m_pAnimState) ? _pPhysContext->m_pAnimState->m_pModelInstance : NULL;
	CXR_Model_BSP4Glass_Instance* pGlassInstance = (CXR_Model_BSP4Glass_Instance*)pModelInstance;

StartOf_Phys_IntersectOBB_i:
	uint32 WorkingNode = aWorkingSet[--WorkingStack];

StartOf_Phys_IntersectOBB_i_NoAdd:
	const CBSP4_Node* pN = &m_pNodes[WorkingNode];

	if (pN->IsLeaf())
	{
		int nf = pN->m_nFaces;
		int iif = pN->m_iiFaces;

		const uint32* piFaces = &m_piFaces[iif];
		for(int i = 0; i < nf; i++)
		{
			int iFace = piFaces[i];
			{
				uint32 iFaceIdx = iFace >> 3;
				uint8 iFaceMask = 1 << (iFace & 0x7);
				if (pEnumContext->m_pFTag[iFaceIdx] & iFaceMask) continue;
				if(!pEnumContext->m_pFTag[iFaceIdx])
					pEnumContext->m_pFUntag[pEnumContext->m_nUntagEnum++]	= iFaceIdx;
				pEnumContext->m_pFTag[iFaceIdx] |= iFaceMask;
			}
			const CBSP4_Face* pF = &m_pFaces[iFace];

			if (_pPhysContext->m_pWC) 
				__PhysGlass_RenderFace(iFace, _pPhysContext->m_WMat, _pPhysContext->m_pWC, 0xffff0000);

			if (!(pF->m_Flags & XW_FACE_PHYSICAL) || (pF->m_Flags & XW_FACE_VOLUMEINSIDE)) continue;
			if (!(m_pMediums[pF->m_iBackMedium].m_MediumFlags & _MediumFlags)) continue;

			const CPlane3Dfp32& PolyPlane = m_pPlanes[pF->m_iPlane];
			const CPlane3Dfp32 PolyPlaneM = CPlane3Dfp32(-PolyPlane.n, m_pMediums[pF->m_iBackMedium].m_Thickness - PolyPlane.d);
			fp32 MinDist = PolyPlane.GetBoxMinDistance(_Bound.m_Min, _Bound.m_Max);
			fp32 MinDistM = PolyPlaneM.GetBoxMinDistance(_Bound.m_Min, _Bound.m_Max);
			if (MinDist > MODEL_BSP_EPSILON && MinDistM > MODEL_BSP_EPSILON) continue;

			uint32 nv = pF->m_nVertices;
			uint32 iiv = pF->m_iiVertices;

			// Get bound-box for polygon.
			const uint32* piVert = m_piVertices + iiv;
			CBox3Dfp32 PBox;

			PBox.m_Min = m_pVertices[piVert[0]];
			PBox.m_Max = m_pVertices[piVert[0]];

			const CVec3Dfp32 VertexOffset = PolyPlaneM.n * m_pMediums[pF->m_iBackMedium].m_Thickness;
			for( uint32 v = 1; v < nv; v++ )
			{
				const CVec3Dfp32& V = m_pVertices[piVert[v]];
				const CVec3Dfp32 VO = V + VertexOffset;
				PBox.m_Min.k[0] = Min( PBox.m_Min.k[0], V.k[0] );
				PBox.m_Min.k[0] = Min( PBox.m_Min.k[0], VO.k[0] );
				PBox.m_Max.k[0] = Max( PBox.m_Max.k[0], V.k[0] );
				PBox.m_Max.k[0] = Max( PBox.m_Max.k[0], VO.k[0] );
				PBox.m_Min.k[1] = Min( PBox.m_Min.k[1], V.k[1] );
				PBox.m_Min.k[1] = Min( PBox.m_Min.k[1], VO.k[1] );
				PBox.m_Max.k[1] = Max( PBox.m_Max.k[1], V.k[1] );
				PBox.m_Max.k[1] = Max( PBox.m_Max.k[1], VO.k[1] );
				PBox.m_Min.k[2] = Min( PBox.m_Min.k[2], V.k[2] );
				PBox.m_Min.k[2] = Min( PBox.m_Min.k[2], VO.k[2] );
				PBox.m_Max.k[2] = Max( PBox.m_Max.k[2], V.k[2] );
				PBox.m_Max.k[2] = Max( PBox.m_Max.k[2], VO.k[2] );
			}
			if( _Bound.m_Min.k[0] > PBox.m_Max.k[0] ||
				_Bound.m_Min.k[1] > PBox.m_Max.k[1] ||
				_Bound.m_Min.k[2] > PBox.m_Max.k[2] ||
				_Bound.m_Max.k[0] < PBox.m_Min.k[0] ||
				_Bound.m_Max.k[1] < PBox.m_Min.k[1] ||
				_Bound.m_Max.k[2] < PBox.m_Min.k[2] )
				continue;

			if (pGlassInstance)
			{
				uint16 iInstance = m_piFaceInstance[iFace];
				if (iInstance >= GLASS_MAX_INSTANCE)
					continue;

				if (pGlassInstance->m_lModels[iInstance].m_Attrib.Attrib_NoPhys())
				{
					if (pGlassInstance->Phys_BoxIntersect(_pPhysContext, iInstance, PolyPlane, _BoxStart, _Box, _pCollisionInfo))
					{
						if (_pCollisionInfo)
						{
							_pCollisionInfo->m_pSurface = m_lspSurfaces[pF->m_iSurface];
							_pCollisionInfo->m_LocalNode = iInstance;
							bIntersect = true;
						}
						else
						{
							// No collision info, untag and return
							pEnumContext->Untag();
							return true;
						}
					}

					continue;
				}
			}

			// No glass model found, try to run the collision code on bsp model
			if (_pCollisionInfo)
			{
				CCollisionInfo CInfo;

				if (_pPhysContext->m_pWC) 
					__PhysGlass_RenderFace(iFace, _pPhysContext->m_WMat, _pPhysContext->m_pWC, 0xff00ff00);

				if (Phys_Intersect_PolyOBB(m_pVertices, piVert, nv, PolyPlane, _BoxStart, _Box, false, &CInfo ))
				{
					if (_pPhysContext->m_pWC) 
						__PhysGlass_RenderFace(iFace, _pPhysContext->m_WMat, _pPhysContext->m_pWC, 0xffffffff);

					CInfo.m_pSurface	= m_lspSurfaces[pF->m_iSurface];
					bIntersect = true;
					if (_pCollisionInfo->Improve(CInfo))
						_pCollisionInfo->m_LocalNode = m_piFaceInstance[iFace];
				}
			}
			else
			{
				if (Phys_Intersect_PolyOBB(m_pVertices, piVert, nv, PolyPlane, _BoxStart, _Box, false, NULL))
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
		int Side = pP->GetBoxPlaneSideMask(_Bound.m_Min, _Bound.m_Max);

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

	return bIntersect;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_BSP4Glass_Instance::Phys_BoxIntersect
|__________________________________________________________________________________________________
\*************************************************************************************************/
bool CXR_Model_BSP4Glass_Instance::Phys_BoxIntersect(const CXR_PhysicsContext* _pPhysContext, const int32 _iInstance, const CPlane3Dfp32& _PolyPlane, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, CCollisionInfo* _pCollisionInfo)
{
	if (_iInstance >= GLASS_MAX_INSTANCE)
		return false;

	CBSP4Glass_Model& Model = m_lModels[_iInstance];
	if (Model.m_Grid.IsValid())
	{
		CBSP4Glass_Grid_CollisionInfo GCInfo = Model.m_Grid.Phys_BoxIntersectGrid(_pPhysContext, _PolyPlane, _BoxStart, _Box, _pCollisionInfo);
		if (_pCollisionInfo)
			return GCInfo.IsValid();

		return GCInfo.m_bValid;
	}

	return false;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_Grid::Phys_BoxIntersectGrid
|__________________________________________________________________________________________________
\*************************************************************************************************/
CBSP4Glass_Grid_CollisionInfo CBSP4Glass_Grid::Phys_BoxIntersectGrid(const CXR_PhysicsContext* _pPhysContext, const CPlane3Dfp32& _PolyPlane, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, CCollisionInfo* _pCollisionInfo)
{
	const CMat4Dfp32& WMat = _pPhysContext->m_WMat;

	const CVec3Dfp32 Middle = (_Box.m_C - _BoxStart.m_C) * 0.5f;
	const CVec3Dfp32 Position = _BoxStart.m_C + Middle;
	const fp32 Radius = M_Sqrt(Middle.LengthSqr() + MaxMT(_Box.GetRadiusSquared(), _BoxStart.GetRadiusSquared()));

	CBSP4Glass_Grid_PhysInfo PhysInfo;
	Phys_SetupIntersectGrid(Position, &Radius, WMat, PhysInfo);

	const int32 Start0 = PhysInfo.m_Start.k[0];
	const int32 Start1 = PhysInfo.m_Start.k[1];
	const int32 End0   = PhysInfo.m_End.k[0];
	const int32 End1   = PhysInfo.m_End.k[1];

	CBSP4Glass_Grid_CollisionInfo GCInfo;

	// Fetch vertices
	CVec3Dfp32* pGridPoints = m_lGridPoints.GetBasePtr();
	const uint32 nPointsX = m_nQuadsX + 1;
	for(uint32 y = Start1; y < End1; y++)
	{
		uint32 i = (y * m_nQuadsX) + Start0;
		uint32 iPoint0 = (y * nPointsX) + Start0;

		//iHitY = y;
		for(uint32 x = Start0; x < End0; x++, i++, iPoint0++)
		{
			// Check if grid is valid
			if(m_GridData.Get(i,m_GridData.GetLastBit()))
			{
				const uint32 iPoint1 = iPoint0 + nPointsX;
				const uint32 liVertices[] = { iPoint0, iPoint1, iPoint1+1, iPoint0+1, NULL };
				
				if (_pCollisionInfo)
				{
					CCollisionInfo CInfo;
					if (Phys_Intersect_PolyOBB(pGridPoints, liVertices, 4, _PolyPlane, _BoxStart, _Box, false, &CInfo))
					{
						if (_pCollisionInfo->Improve(CInfo))
						{
							GCInfo.m_bValid = true;
							GCInfo.m_iInside = i;
							GCInfo.m_iHitX = x;
							GCInfo.m_iHitY = y;
						}
					}
				}
				else
				{
					if (Phys_Intersect_PolyOBB(pGridPoints, liVertices, 4, _PolyPlane, _BoxStart, _Box, false, NULL))
						return CBSP4Glass_Grid_CollisionInfo(true, -1, -1, -1);
				}
			}
		}
	}

	return GCInfo;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_Grid::Phys_EnumBox
|__________________________________________________________________________________________________
\*************************************************************************************************/
uint32 CBSP4Glass_Grid::Phys_EnumBox(const CPhysOBB& _Box, const CMat4Dfp32& _WMat, TThinArray<uint32>& _lReturn)
{
	const fp32 Radius = M_Sqrt(_Box.GetRadiusSquared());
	const CVec3Dfp32& LocalPos = _Box.m_C;
	CPlane3Dfp32 PolyPlane(m_PlaneN, m_PlaneD);

	CBSP4Glass_Grid_PhysInfo PhysInfo;
	Phys_SetupIntersectGrid(LocalPos, &Radius, _WMat, PhysInfo);

	const int32 Start0 = PhysInfo.m_Start.k[0];
	const int32 Start1 = PhysInfo.m_Start.k[1];
	const int32 End0   = PhysInfo.m_End.k[0];
	const int32 End1   = PhysInfo.m_End.k[1];

	int32 nReturn = 0;
	int32 nMaxReturn = MaxMT(End0 - Start0, 0) * MaxMT(End1 - Start1, 0);
	_lReturn.SetLen(nMaxReturn);
	uint32* pReturn = _lReturn.GetBasePtr();

	// Fetch vertices
	CVec3Dfp32* pGridPoints = m_lGridPoints.GetBasePtr();
	const uint32 nPointsX = m_nQuadsX + 1;
	uint8 iLastBit = m_GridData.GetLastBit();
	for(uint32 y = Start1; y < End1; y++)
	{
		uint32 i = (y * m_nQuadsX) + Start0;
		uint32 iPoint0 = (y * nPointsX) + Start0;

		for(uint32 x = Start0; x < End0; x++, i++, iPoint0++)
		{
			// Check if grid is valid
			if(m_GridData.Get(i,iLastBit))
			{
				const uint32 iPoint1 = iPoint0 + nPointsX;
				const uint32 liVertices[] = { iPoint0, iPoint1, iPoint1+1, iPoint0+1, NULL };
				
				if (Phys_Intersect_PolyOBB(pGridPoints, liVertices, 4, PolyPlane, _Box, _Box, false, NULL))
					pReturn[nReturn++] = i;
			}
		}
	}

	return nReturn;
}

