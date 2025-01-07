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
| CXR_Model_BSP4Glass::Phys_IntersectLine
|__________________________________________________________________________________________________
\*************************************************************************************************/
bool CXR_Model_BSP4Glass::Phys_IntersectLine(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::Phys_IntersectLine, XR_BSP4GLASS);
	if (!m_piFaceInstance)
		return false; // To prevent crash...

	CVec3Dfp32 v0, v1;
	_v0.MultiplyMatrix(_pPhysContext->m_WMatInv, v0);
	_v1.MultiplyMatrix(_pPhysContext->m_WMatInv, v1);

	CPhysLineContext LineContext;
	LineContext.m_V0 = v0;
	LineContext.m_V1 = v1;
	LineContext.m_InvV1V0 = 1.0f * v1.DistanceInv(v0);

	if (_pCollisionInfo && (_pCollisionInfo->m_ReturnValues & CXR_COLLISIONRETURNVALUE_TIME))
	{
		_pCollisionInfo->m_LocalNode = -1;

		bool bHit = __PhysGlass_TraceLine_r(_pPhysContext, 1, v0, v1, _MediumFlags, &LineContext, _pCollisionInfo);
		if(_pCollisionInfo->IsComplete())
			return true;

		if(_pCollisionInfo->m_bIsValid)
		{
			_pCollisionInfo->m_Plane.Transform(_pPhysContext->m_WMat);
			_pCollisionInfo->m_Pos = _pCollisionInfo->m_LocalPos;
			_pCollisionInfo->m_Distance = _pCollisionInfo->m_Plane.Distance(v1);
			_pCollisionInfo->m_Time = v0.Distance(_pCollisionInfo->m_Pos) / v0.Distance(v1);
			_pCollisionInfo->m_Pos *= _pPhysContext->m_WMat;

			return true;
		}
	}
	else
	{
		if(__PhysGlass_TraceLine_r(_pPhysContext, 1, v0, v1, _MediumFlags, &LineContext))
		{
			if(_pCollisionInfo)
			{
				_pCollisionInfo->m_bIsCollision = true;
				_pCollisionInfo->m_bIsValid = false;
				_pCollisionInfo->m_LocalNode = -1;
			}
			return true;
		}
	}

	return false;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_BSP4Glass::__PhysGlass_TraceLine_r
|__________________________________________________________________________________________________
\*************************************************************************************************/
bool CXR_Model_BSP4Glass::__PhysGlass_TraceLine_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MediumFlags, CPhysLineContext* _pLineContext) const
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::__PhysGlass_TraceLine_r_Simple, XR_BSP4GLASS);

	if (!_iNode)
		return true;

	const CBSP4_Node* pN = &m_pNodes[_iNode];

	if (pN->IsLeaf())
		return ((m_pMediums[pN->m_iMedium].m_MediumFlags & _MediumFlags) != 0) ? true : false;

	if(pN->IsNode())
	{
		const CPlane3Dfp32* pP = &m_pPlanes[pN->m_iPlane];
		const fp32 d1 = pP->Distance(_p0);
		const fp32 d2 = pP->Distance(_p1);

		// Masks
		const int Side1 = ((d1 < -0.01f) ? 2 : ((d1 > 0.01f) ? 1 : 4));
		const int Side = (Side1 | ((d2 < -0.01f) ? 2 : ((d2 > 0.01f) ? 1 : 4)));

		if(Side == 4)
		{
			return
				__PhysGlass_TraceLine_r(_pPhysContext, pN->m_iNodeBack, _p0, _p1, _MediumFlags, _pLineContext) ||
				__PhysGlass_TraceLine_r(_pPhysContext, pN->m_iNodeFront, _p0, _p1, _MediumFlags, _pLineContext);
		}

		if((Side & 3) == 1)
			return __PhysGlass_TraceLine_r(_pPhysContext, pN->m_iNodeFront, _p0, _p1, _MediumFlags, _pLineContext);
		if((Side & 3) == 2)
			return __PhysGlass_TraceLine_r(_pPhysContext, pN->m_iNodeBack, _p0, _p1, _MediumFlags, _pLineContext);

		const fp32 s = d2 - d1;
		if(s == 0.0f) return false;
		CVec3Dfp32 CutPoint0;
		_p0.Lerp(_p1, -d1/s, CutPoint0);

		if (Side1 & 1)
		{
			return 
				__PhysGlass_TraceLine_r(_pPhysContext, pN->m_iNodeFront, _p0, CutPoint0, _MediumFlags, _pLineContext) ||
				__PhysGlass_TraceLine_r(_pPhysContext, pN->m_iNodeBack, CutPoint0, _p1, _MediumFlags, _pLineContext);
		}
		else
		{
			return 
				__PhysGlass_TraceLine_r(_pPhysContext, pN->m_iNodeBack, _p0, CutPoint0, _MediumFlags, _pLineContext) ||
				__PhysGlass_TraceLine_r(_pPhysContext, pN->m_iNodeFront, CutPoint0, _p1, _MediumFlags, _pLineContext);
		}
	}

	return false;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_BSP4Glass::__PhysGlass_TraceLine_r (CollisionInfo)
|__________________________________________________________________________________________________
\*************************************************************************************************/
bool CXR_Model_BSP4Glass::__PhysGlass_TraceLine_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MediumFlags, CPhysLineContext* _pLineContext, CCollisionInfo* _pCollisionInfo) const
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::__PhysGlass_TraceLine_r_CollisionInfo, XR_BSP4GLASS);
	if(!_iNode)
		return false;

	const CBSP4_Node* pN = &m_pNodes[_iNode];

	bool bHit = false;
	if(pN->IsLeaf())
	{
		const int nf = pN->m_nFaces;
		const int iif = pN->m_iiFaces;
		for(int f = 0; f < nf; f++)
		{
			const int iFace = m_piFaces[iif + f];
			const CBSP4_Face* pF = &m_pFaces[iFace];
			const int iMedium = pF->m_iBackMedium;
			if(!(m_pMediums[iMedium].m_MediumFlags & _MediumFlags)) continue;

			const int nv = pF->m_nVertices;
			const int iiv = pF->m_iiVertices;
			int iv0 = m_piVertices[iiv + nv - 1];

			const CPlane3Dfp32& P = m_pPlanes[pF->m_iPlane];
			const fp32 d1 = P.Distance(_p0);
			if(d1 < 0.001f) continue;
			const fp32 d2 = P.Distance(_p1);
			if(d2 > -0.001f) continue;

			int v;
			for(v = 0; v < nv; v++)
			{
				const int iv1 = m_piVertices[iiv + v];
				CVec3Dfp32 Edge, p0v1, Normal;
				m_pVertices[iv1].Sub(m_pVertices[iv0], Edge);
				m_pVertices[iv1].Sub(_p0, p0v1);
				Edge.CrossProd(p0v1, Normal);
				const CPlane3Dfp32 Plane(Normal, _p0);
				if(Plane.Distance(_p1) > 0.001f) break;

				iv0 = iv1;
			}

			if(v == nv)
			{
				// Calculate hit position
				CVec3Dfp32 HitPos;
				const fp32 t = -d1 / (d2-d1);
				_p0.Lerp(_p1, t, HitPos);

				// Glass might have been hit here, check instance
				if(_pPhysContext && _pPhysContext->m_pAnimState && _pPhysContext->m_pAnimState->m_pModelInstance)
				{
					CXR_Model_BSP4Glass_Instance* pGlassInstance = (CXR_Model_BSP4Glass_Instance*)_pPhysContext->m_pAnimState->m_pModelInstance;
					const int32 GameTick = _pPhysContext->m_pAnimState->m_Data[GLASS_DATA_GAMETICK];

					bHit = pGlassInstance->Phys_LineIntersect((int32)m_piFaceInstance[iFace], _p0, _p1, &HitPos, GameTick, &_pPhysContext->m_WMat);
				}
				else
					bHit = false;

				if(bHit)
				{
					const fp32 Time = _pLineContext->m_V0.Distance(HitPos) * _pLineContext->m_InvV1V0;

					if(_pCollisionInfo->IsImprovement(Time))
					{
						_pCollisionInfo->m_bIsCollision = true;
						_pCollisionInfo->m_bIsValid = true;
						_pCollisionInfo->m_LocalPos = HitPos;
						_pCollisionInfo->m_Time = Time;
						_pCollisionInfo->m_Distance = P.Distance(_pLineContext->m_V1);
						_pCollisionInfo->m_Plane = P;
						_pCollisionInfo->m_pSurface = m_lspSurfaces[pF->m_iSurface];
						_pCollisionInfo->m_LocalNode = m_piFaceInstance[iFace];

						if(Time > 1.1f)
							ConOut(CStrF("(__PhysGlass_TraceLine_r) Invalid T %f", t));

						if(_pCollisionInfo->IsComplete())
							return true;
					}
				}
			}
		}
	}

	if(pN->IsNode())
	{
		const CPlane3Dfp32* pP = &m_pPlanes[pN->m_iPlane];
		const fp32 d1 = pP->Distance(_p0);
		const fp32 d2 = pP->Distance(_p1);

		// Masks
		const int Side1 = ((d1 < -0.01f) ? 2 : ((d1 > 0.01f) ? 1 : 4));
		const int Side2 = ((d2 < -0.01f) ? 2 : ((d2 > 0.01f) ? 1 : 4));

		const int iFront = pN->m_iNodeFront;
		const int iBack = pN->m_iNodeBack;

		if((Side1 | Side2) == 4)
		{
			bHit |= __PhysGlass_TraceLine_r(_pPhysContext, pN->m_iNodeBack, _p0, _p1, _MediumFlags, _pLineContext, _pCollisionInfo);
			bHit |= __PhysGlass_TraceLine_r(_pPhysContext, pN->m_iNodeFront, _p0, _p1, _MediumFlags, _pLineContext, _pCollisionInfo);
			return bHit;
		}

		if(((Side1 | Side2) & 3) == 3)
		{
			if(!(iFront || iBack))
				return bHit;

			const fp32 s = d2 - d1;
			if(s == 0.0f) return bHit;
			const fp32 vscale = -d1 / s;

			CVec3Dfp32 v, CutPoint0, CutPoint1;
			_p1.Sub(_p0, v);
			_p0.Combine(v, vscale+0.01f, CutPoint0);
			_p0.Combine(v, vscale-0.01f, CutPoint1);

			if(Side1 & 2)
			{
				bHit |= __PhysGlass_TraceLine_r(_pPhysContext, iBack, _p0, CutPoint0, _MediumFlags, _pLineContext, _pCollisionInfo);
				bHit |= __PhysGlass_TraceLine_r(_pPhysContext, iFront, CutPoint1, _p1, _MediumFlags, _pLineContext, _pCollisionInfo);
				return bHit;
			}
			else
			{
				bHit |= __PhysGlass_TraceLine_r(_pPhysContext, iFront, _p0, CutPoint0, _MediumFlags, _pLineContext, _pCollisionInfo);
				bHit |= __PhysGlass_TraceLine_r(_pPhysContext, iBack, CutPoint1, _p1, _MediumFlags, _pLineContext, _pCollisionInfo);
				return bHit;
			}
		}
		else
		{
			if((Side1 & 1) || (Side2 & 1))
				bHit |= __PhysGlass_TraceLine_r(_pPhysContext, iFront, _p0, _p1, _MediumFlags, _pLineContext, _pCollisionInfo);
			else
				bHit |= __PhysGlass_TraceLine_r(_pPhysContext, iBack, _p0, _p1, _MediumFlags, _pLineContext, _pCollisionInfo);
		}
	}

	return bHit;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_BSP4Glass_Instance::Phys_LineIntersect
|__________________________________________________________________________________________________
\*************************************************************************************************/
bool CXR_Model_BSP4Glass_Instance::Phys_LineIntersect(const int32 _iInstance, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, const CVec3Dfp32* _pHitPos, const int32 _Tick, const CMat4Dfp32* _pWMat)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass_Instance::Phys_LineIntersect, XR_BSP4GLASS);

	if(_iInstance >= (uint16)-1)
		return false;

	CGlassModel* pModels = m_lModels.GetBasePtr();
	CGlassModel& Model = pModels[_iInstance];
	const int nFrontP = Model.m_lFrontP.Len();

	if(Model.m_Attrib.Attrib_NoPhys())
	{
		if(Model.m_Attrib.Attrib_TimeBreaker())
		{
			// Might have hit some glass pieces... check them properly here...
			CBSP4Glass_Grid& GlassGrid = Model.m_Grid;
			if(GlassGrid.IsValid() && GlassGrid.Phys_LineIntersectGrid(*_pHitPos, *_pWMat).IsValid())
			{
				return true;
			}
		}

		return false;
	}

	if(nFrontP < 3)
		return false;

	const uint16* pFrontP = Model.m_lFrontP.GetBasePtr();
	TAP_RCD<const CVec3Dfp32> pV = Model.m_lV;

	for(int i = 0; i < nFrontP;)
	{
		const CVec3Dfp32& V0 = pV[pFrontP[i++]];
		const CVec3Dfp32& V1 = pV[pFrontP[i++]];
		const CVec3Dfp32& V2 = pV[pFrontP[i++]];

		if(	CPlane3Dfp32((V0-V2)/(V0-_p0), _p0).Distance(_p1) <= 0.001f &&
			CPlane3Dfp32((V1-V0)/(V1-_p0), _p0).Distance(_p1) <= 0.001f &&
			CPlane3Dfp32((V2-V1)/(V2-_p0), _p0).Distance(_p1) <= 0.001f )
#ifndef M_RTM
		{
			Phys_LineIntersect_RenderHit(_pWMat, V0, V1, V2, _pHitPos);
			return true;
		}
#else
			return true;
#endif
	}

	// Ignoring thickness, make a false plane from the other direction and test it
	if(!Model.m_Attrib.Attrib_UseThickness())
	{
		for(int i = 0; i < nFrontP;)
		{
			const CVec3Dfp32& V0 = pV[pFrontP[i++]];
			const CVec3Dfp32& V2 = pV[pFrontP[i++]];
			const CVec3Dfp32& V1 = pV[pFrontP[i++]];
			if(	CPlane3Dfp32((V0-V2)/(V0-_p0), _p0).Distance(_p1) <= 0.001f &&
				CPlane3Dfp32((V1-V0)/(V1-_p0), _p0).Distance(_p1) <= 0.001f &&
				CPlane3Dfp32((V2-V1)/(V2-_p0), _p0).Distance(_p1) <= 0.001f )
#ifndef M_RTM
			{
				Phys_LineIntersect_RenderHit(_pWMat, V0, V1, V2, _pHitPos);
				return true;
			}
#else
				return true;
#endif
		}
	}

	return false;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_Grid::Phys_LineIntersectGrid
|__________________________________________________________________________________________________
\*************************************************************************************************/
CBSP4Glass_Grid_CollisionInfo CBSP4Glass_Grid::Phys_LineIntersectGrid(const CVec3Dfp32& _Position, const CMat4Dfp32& _WMat)
{
	GLASS_MSCOPE(CBSP4Glass_Grid::Phys_LineIntersectGrid, XR_BSP4GLASS);
	const TAP_RCD<CVec3Dfp32> lGridPoints = m_lGridPoints;

	const CVec3Dfp32 Position = _Position * _WMat;
	const CVec3Dfp32 Pos0 = Position + m_PlaneN;
	const CVec3Dfp32 Pos1 = Position - m_PlaneN;

	CBSP4Glass_Grid_PhysInfo PhysInfo;
	Phys_SetupIntersectGrid(_Position, NULL, _WMat, PhysInfo);

	const int32 Start0 = PhysInfo.m_Start.k[0];
	const int32 Start1 = PhysInfo.m_Start.k[1];
	const int32 End0   = PhysInfo.m_End.k[0];
	const int32 End1   = PhysInfo.m_End.k[1];

	// Make sure position is inside any of the grid quads and that they are valid
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
				const CVec3Dfp32& GridPoint0  = lGridPoints[iPoint0]   * _WMat;
				const CVec3Dfp32& GridPoint01 = lGridPoints[iPoint0+1] * _WMat;
				const CVec3Dfp32& GridPoint1  = lGridPoints[iPoint1]   * _WMat;
				const CVec3Dfp32& GridPoint11 = lGridPoints[iPoint1+1] * _WMat;

				// Caluclate edges
				const CVec3Dfp32 Edge0 = GridPoint1  - GridPoint0;
				const CVec3Dfp32 Edge1 = GridPoint11 - GridPoint1;
				const CVec3Dfp32 Edge2 = GridPoint01 - GridPoint11;
				const CVec3Dfp32 Edge3 = GridPoint0  - GridPoint01;

				// Check
				if ((CPlane3Dfp32(Edge0 / (GridPoint1  - Pos0), Pos0).Distance(Pos1) > 0.001f) ||
					(CPlane3Dfp32(Edge1 / (GridPoint11 - Pos0), Pos0).Distance(Pos1) > 0.001f) ||
					(CPlane3Dfp32(Edge2 / (GridPoint01 - Pos0), Pos0).Distance(Pos1) > 0.001f) ||
					(CPlane3Dfp32(Edge3 / (GridPoint0  - Pos0), Pos0).Distance(Pos1) > 0.001f))
					continue;

#ifdef GLASS_DEBUG_CRUSH
				{
					MACRO_GetRegisterObject(CWireContainer, pWC, "GAMECONTEXT.CLIENT.WIRECONTAINER");
					if(pWC)
					{
						pWC->RenderWire(GridPoint0, GridPoint1, CPixel32(255,0,0,255), 5.0f, false);
						pWC->RenderWire(GridPoint1, GridPoint11, CPixel32(255,0,0,255), 5.0f, false);
						pWC->RenderWire(GridPoint11, GridPoint01, CPixel32(255,0,0,255), 5.0f, false);
						pWC->RenderWire(GridPoint01, GridPoint0, CPixel32(255,0,0,255), 5.0f, false);
					}
				}
#endif

				return CBSP4Glass_Grid_CollisionInfo(true, i, x, y);
			}
		}
	}

	return CBSP4Glass_Grid_CollisionInfo();
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_BSP4Glass_Instance::Phys_LineIntersect_RenderHit
|__________________________________________________________________________________________________
\*************************************************************************************************/
#ifndef M_RTM
void CXR_Model_BSP4Glass_Instance::Phys_LineIntersect_RenderHit(const CMat4Dfp32* _pWMat, const CVec3Dfp32& _V0, const CVec3Dfp32& _V1, const CVec3Dfp32& _V2, const CVec3Dfp32* _pHitPos)
{
	// Render hit triangle
	MACRO_GetRegisterObject(CWireContainer, pWC, "GAMECONTEXT.CLIENT.WIRECONTAINER");
	if(pWC)
	{
		CMat4Dfp32 Mat;
		Mat.Unit();
		if(_pWMat)
			Mat = *_pWMat;

		// Render hit triangle!
		const CVec3Dfp32 R0 = _V0 * Mat;
		const CVec3Dfp32 R1 = _V1 * Mat;
		const CVec3Dfp32 R2 = _V2 * Mat;

		if(_pHitPos)
			pWC->RenderVertex((*_pHitPos) * Mat, 0xFF7F7F7F, 0.0f);
		pWC->RenderWire(R0, R1, 0xFF7F0000, 0.0f, false);
		pWC->RenderWire(R1, R2, 0xFF7F0000, 0.0f, false);
		pWC->RenderWire(R2, R0, 0xFF7F0000, 0.0f, false);
	}
}
#endif


