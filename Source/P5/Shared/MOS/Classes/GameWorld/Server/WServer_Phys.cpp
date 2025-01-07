
#include "PCH.h"

#include "WServer_Core.h"
#include "../WPhysState_Hash.h"
#include "../../../XR/Phys/WPhysPCS.h"
#include "../WDynamics.h"
#include "../WObjects/WObj_PhysCluster.h"

#ifndef M_RTM
// #define PHYSSTATE_DEBUG
#endif

#ifdef _DEBUG
	#include "MFloat.h"
	#define DEBUG_CHECK_ROW(v, r)\
		M_ASSERT(!FloatIsInvalid((v).k[r][0]) && \
		         !FloatIsInvalid((v).k[r][1]) && \
		         !FloatIsInvalid((v).k[r][2]) && \
		         !FloatIsInvalid((v).k[r][3]) && \
		         (M_Fabs((v).k[r][0]) + M_Fabs((v).k[r][1]) + M_Fabs((v).k[r][2]) + M_Fabs((v).k[r][2]) < 1000000.0f), "Invalid vector!");
	#define DEBUG_CHECK_MATRIX(m)\
		DEBUG_CHECK_ROW(m, 0)\
		DEBUG_CHECK_ROW(m, 1)\
		DEBUG_CHECK_ROW(m, 2)\
		DEBUG_CHECK_ROW(m, 3)
	#define DEBUG_CHECK_VECTOR(v) \
		M_ASSERT(!FloatIsInvalid((v).k[0]) && \
		         !FloatIsInvalid((v).k[1]) && \
		         !FloatIsInvalid((v).k[2]) && \
		         (M_Fabs((v).k[0]) + M_Fabs((v).k[1]) + M_Fabs((v).k[2]) < 1000000.0f), "Invalid vector!");
	#define DEBUG_CHECK_AXISANGLE(q) \
		M_ASSERT(!FloatIsInvalid((q).m_Axis.k[0]) && \
		         !FloatIsInvalid((q).m_Axis.k[1]) && \
		         !FloatIsInvalid((q).m_Axis.k[2]) && \
		         !FloatIsInvalid((q).m_Angle) && \
		         (M_Fabs((q).m_Axis.k[0]) + M_Fabs((q).m_Axis.k[0]) + M_Fabs((q).m_Axis.k[0]) + M_Fabs((q).m_Angle) <= 1000000.0f), "Invalid axis-angle!");
#else
	#define DEBUG_CHECK_ROW(v, r)
	#define DEBUG_CHECK_MATRIX(m)
	#define DEBUG_CHECK_VECTOR(v)
	#define DEBUG_CHECK_AXISANGLE(q)
#endif

//#pragma optimize( "", off )
//#pragma inline_depth(0)

// #define SERVER_PHYS_COLLIDEALL		// Debug, causes all entities to be tested.

// -------------------------------------------------------------------

bool CWorld_ServerCore::Phys_IntersectWorld( CPotColSet *_pcs, const CWO_PhysicsState& _PhysState, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, int _iExcludeObj, 
		CCollisionInfo* _pCollInfo, int _NotifyFlags, CSelection* _pNotifySelection1 , CSelection* _pNotifySelection2)
{
	MAUTOSTRIP(CWorld_ServerCore_Phys_IntersectWorld_PCS, false);
	MSCOPESHORT(CWorld_ServerCore::Phys_IntersectWorld_PCS);

	int iCInfoObj = -1;
	enum { e_MaxCInfo = 30 };
	CCollisionInfo lCInfo[e_MaxCInfo];

	CMat4Dfp32 WOrigin = _Origin;
	CMat4Dfp32 WDest = _Dest;

	if( !(_PhysState.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION) )
	{
		WOrigin.Unit3x3();
		WDest.Unit3x3();
	}

	const int nPrim = _PhysState.m_nPrim;
	for (int iPrim = 0; iPrim < nPrim; iPrim++)
	{
		CWO_PhysicsPrim	const &PhysPrim = _PhysState.m_Prim[iPrim];

		CMat4Dfp32 WPrimOrigin = WOrigin;
		CMat4Dfp32 WPrimDest = WDest;

		// Add offset to primitive?
		if (_PhysState.m_PhysFlags & OBJECT_PHYSFLAGS_OFFSET)
		{
			CVec3Dfp32 WOffs( PhysPrim.GetOffset() );

			if( _PhysState.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION )
			{
				// ingen rotations hastighet, innebär att väldigt konstiga saker kan uppstå om roteringen inte integreras
				// utan bara sätts mellan 2 frames.
			
				WPrimOrigin.k[3][0] += WOffs[0] * WPrimOrigin.k[0][0] + WOffs[1] * WPrimOrigin.k[0][1] + WOffs[2] * WPrimOrigin.k[0][2];
				WPrimOrigin.k[3][1] += WOffs[0] * WPrimOrigin.k[1][0] + WOffs[1] * WPrimOrigin.k[1][1] + WOffs[2] * WPrimOrigin.k[1][2];
				WPrimOrigin.k[3][2] += WOffs[0] * WPrimOrigin.k[2][0] + WOffs[1] * WPrimOrigin.k[2][1] + WOffs[2] * WPrimOrigin.k[2][2];

				WPrimDest.k[3][0] += WOffs[0] * WPrimDest.k[0][0] + WOffs[1] * WPrimDest.k[0][1] + WOffs[2] * WPrimDest.k[0][2];
				WPrimDest.k[3][1] += WOffs[0] * WPrimDest.k[1][0] + WOffs[1] * WPrimDest.k[1][1] + WOffs[2] * WPrimDest.k[1][2];
				WPrimDest.k[3][2] += WOffs[0] * WPrimDest.k[2][0] + WOffs[1] * WPrimDest.k[2][1] + WOffs[2] * WPrimDest.k[2][2];
			}
			else
			{
				WPrimOrigin.k[3][0] += WOffs[0];
				WPrimOrigin.k[3][1] += WOffs[1];
				WPrimOrigin.k[3][2] += WOffs[2];

				WPrimDest.k[3][0] += WOffs[0];
				WPrimDest.k[3][1] += WOffs[1];
				WPrimDest.k[3][2] += WOffs[2];
			}
		}
		
		lCInfo[0].Clear();
		int nCollisions = 0;

		switch( PhysPrim.m_PrimType )
		{
		case OBJECT_PRIMTYPE_NONE :
			{
				MSCOPESHORT(PCS_NotHandled_NONE);
				// wierd - why should there be any primitives marked OBJECT_PRIMTYPE_NONE??
				continue;
			}

		case OBJECT_PRIMTYPE_PHYSMODEL :
			{
				MSCOPESHORT(PCS_NotHandled_PHYSMODEL);
				// add code to intersect PCS with physmodel
				// physmodels themselfes have to be responsible/implementing for colliding with sets!
				
				// PhysPrim->CollideClosestPCS( _pcs );				
				continue;
			}

		case OBJECT_PRIMTYPE_SPHERE :
			{	// intersect sphere with PCS
				nCollisions = _pcs->CollideSphere(WPrimOrigin.GetRow(3).k, WPrimDest.GetRow(3).k, PhysPrim.GetRadius() + 0.0001f, lCInfo, e_MaxCInfo);
				break;
			}

		case OBJECT_PRIMTYPE_BOX :
			{	// intersect box with PCS

				CBox3Dfp32 BoxOfDoom; // set the box correct damnit!
				BoxOfDoom.m_Max = PhysPrim.GetDim();
				BoxOfDoom.m_Min = -BoxOfDoom.m_Max;

				nCollisions = _pcs->CollideBox(&WPrimOrigin, &WPrimDest, BoxOfDoom.m_Min.k, lCInfo, e_MaxCInfo);
				break;
			}

		case OBJECT_PRIMTYPE_POINT :
			{	// intersect point with PCS, måste göra med WOrigin -> WDest linjen!
				nCollisions = _pcs->CollidePoint( CVec3Dfp32::GetMatrixRow(WOrigin, 3).k, CVec3Dfp32::GetMatrixRow(WDest, 3).k, lCInfo, e_MaxCInfo);
				break;
			}

		default :
			Error("Phys_", CStrF("Invalid primitive type. (%d)", PhysPrim.m_PrimType ));
		}

#ifdef PCSCHECK_INSOLID
		//AR-TEMP-ADD: last round and nothing found? check if inside solid
		if ((nCollisions == 0) && (iCInfoObj == -1) && (iPrim == (nPrim-1)) && _pcs->m_bIsInSolid)
		{
			lCInfo[0].m_IN1N2Flags = _pcs->m_SolidFlags;
			lCInfo[0].m_iObject = _pcs->m_iSolidObj;
			nCollisions = 1;
		}
#endif

		for (int iColl = 0; iColl < nCollisions; iColl++)
		{
			const CCollisionInfo& CInfo = lCInfo[iColl];

			// Add to notify selection1?
			if ((_pNotifySelection1 != NULL) && (CInfo.m_IN1N2Flags & PCS_DO_NOTIFY1))
				Selection_AddObject(*_pNotifySelection1, CInfo.m_iObject);

			// Add to notify selection2?
			if ((_pNotifySelection2 != NULL) && (CInfo.m_IN1N2Flags & PCS_DO_NOTIFY2))
				Selection_AddObject(*_pNotifySelection2, CInfo.m_iObject);

			// Collision info?
			if (CInfo.m_IN1N2Flags & PCS_DO_INTERSECT)
			{
				if (_pCollInfo) 
				{
					_pCollInfo->Improve(CInfo);
				}
				iCInfoObj = CInfo.m_iObject;
			}
		}

	} // for all primitives

	// Finalize collision info
	if (_pCollInfo && _pCollInfo->m_bIsValid)
	{
		CWObject_CoreData* pCObj = ThisClass::Object_GetCD(iCInfoObj);
		if (pCObj)
		{
			CMat4Dfp32 NewMat, VelMat;
			pCObj->GetVelocityMatrix(VelMat);
			pCObj->GetPositionMatrix().Multiply3x3(VelMat, NewMat);
			CVec3Dfp32::GetMatrixRow(NewMat, 3) += pCObj->GetPosition();
			CVec3Dfp32::GetMatrixRow(NewMat, 3) += pCObj->GetMoveVelocity();

			CVec3Dfp32 p1(_pCollInfo->m_LocalPos);
			CVec3Dfp32 p2(_pCollInfo->m_LocalPos);
			p1 *= pCObj->GetPositionMatrix();
			p2 *= NewMat;
			_pCollInfo->m_Velocity = p2 - p1;
		}
	}

	bool bRet = (iCInfoObj != -1);
	return bRet;
}


bool CWorld_ServerCore::Phys_IntersectWorld(const CSelection* _pSelection, const CWO_PhysicsState& _PhysState, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, int _iExcludeObj, 
		CCollisionInfo* _pCollisionInfo, int _NotifyFlags,  CSelection* _pNotifySelection1, CSelection* _pNotifySelection2)
{
	MAUTOSTRIP(CWorld_ServerCore_Phys_IntersectWorld_NoPCS, false);
	MSCOPESHORT(CWorld_ServerCore::Phys_IntersectWorld_NoPCS);

	// Returns false if _PhysState can be placed at _Pos
	// If _pCollisionInfo != NULL the closest point of intersection will be return in *_pCollisionInfo

	if (!_PhysState.m_nPrim)
		return false;

#ifdef	SERVER_STATS
CMTime Time; TStart(Time);
#endif


//	_PhysState.GetMinMaxBox(BMin, BMax);
//	BMin += CVec3Dfp32::GetMatrixRow(_Pos, 3);
//	BMax += CVec3Dfp32::GetMatrixRow(_Pos, 3);
	const CMat4Dfp32* pOrigin = &_Origin;
	const CMat4Dfp32* pDest = &_Dest;
	CMat4Dfp32 NewOrigin, NewDest;

	if (!(_PhysState.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION))
	{
		NewOrigin.Unit();
		NewDest.Unit();
		CVec3Dfp32::GetMatrixRow(NewOrigin, 3) = CVec3Dfp32::GetMatrixRow(_Origin, 3);
		CVec3Dfp32::GetMatrixRow(NewDest, 3) = CVec3Dfp32::GetMatrixRow(_Dest, 3);
		pOrigin = &NewOrigin;
		pDest = &NewDest;
	}

#ifdef SERVER_PHYS_COLLIDEALL
	TSelection<CSelection::LARGE_BUFFER> Selection;
	Selection_AddAll(Selection);
	const CSelection* pSelection=&Selection;
#else
	const CSelection* pSelection=_pSelection;
#endif
	
	const int16* piEnum = NULL;
	int nEnum = 0;

	if (pSelection != NULL)
	{
		nEnum = Selection_Get(*pSelection, &piEnum);
	}
	else
	{
		CWO_EnumParams_Box Params;
		Params.m_ObjectFlags = _PhysState.m_ObjectIntersectFlags | ((_pNotifySelection2 != NULL) ? _NotifyFlags : 0);
		Params.m_ObjectIntersectFlags = _PhysState.m_ObjectFlags;
		Params.m_ObjectNotifyFlags = (_pNotifySelection2 != NULL) ? _PhysState.m_ObjectFlags : 0;

		CBox3Dfp32 BoxSrc;
		Phys_GetMinMaxBox(_PhysState, *pOrigin, BoxSrc);
		Phys_GetMinMaxBox(_PhysState, *pDest, Params.m_Box);
		Params.m_Box.Expand(BoxSrc);
		nEnum = m_spSpaceEnum->EnumerateBox(Params, m_lEnumSpace.GetBasePtr(), m_lEnumSpace.Len());
		piEnum = m_lEnumSpace.GetBasePtr();
	}

	CCollisionInfo CInfo;
	int iCInfoObj = -1;

 	int bDoNotify1 = 0;
	int bDoNotify2 = 0;
	int bCheckObj1 = false;
	int bCheckObj2 = false;
	if (_pNotifySelection1 != NULL) bCheckObj1 = _pNotifySelection1->GetNumElements();
	if (_pNotifySelection2 != NULL) bCheckObj2 = _pNotifySelection2->GetNumElements();

	int bCheckAllForNotify = (_pNotifySelection1 != NULL) || (_pNotifySelection2 != NULL);
	int bComplete = false;

	CMat4Dfp32 TmpObjMat;
	TmpObjMat.Unit();

	int NotifyFlags1 = 0;
	int NotifyFlags2 = 0;

	for(int i = 0; i < nEnum; i++)
	{
		int iObj = piEnum[i];

		if (iObj != _iExcludeObj)
		{
			if (iObj == _PhysState.m_iExclude)
				continue;

			CWObject* pObj = m_lspObjects[iObj];

			if (!pObj)
				continue;

			CWO_PhysicsState* pPhys = &pObj->m_PhysState;

			if (pPhys->m_iExclude == _iExcludeObj)
				continue;

			// Do they have any flags in common?
			int bDoIntersect = 
				!bComplete && 
				(
					(pPhys->m_ObjectIntersectFlags & _PhysState.m_ObjectFlags) ||
					(pPhys->m_ObjectFlags & _PhysState.m_ObjectIntersectFlags) 
				);

			if (_pNotifySelection1 != NULL)
			{
 				bDoNotify1 = pPhys->m_ObjectFlags & _NotifyFlags;
				NotifyFlags1 = (bDoNotify1) ? _NotifyFlags : 0;
			}
			if (_pNotifySelection2 != NULL)
			{
				bDoNotify2 = pObj->m_IntersectNotifyFlags & _PhysState.m_ObjectFlags;
				NotifyFlags2 = (bDoNotify2) ? pObj->m_IntersectNotifyFlags : 0;
			}

			if (!(bDoIntersect || bDoNotify1 || bDoNotify2)) continue;


			int bIntersect = false;

			// Determine intersection order, so that objects treat eachother identically, regardless of the
			// object-order in the function-call. (Phys_IntersectWorld(obj1, obj2) or Phys_IntersectWorld(obj2, obj1) ?)

			bool bOrder = (iObj > _iExcludeObj);

			const CMat4Dfp32* pObjMat = &pObj->m_Pos;
			if (!(pPhys->m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION))
			{
				pObj->GetPosition().SetMatrixRow(TmpObjMat, 3);
				pObjMat = &TmpObjMat;
			}

			if (bDoIntersect && _pCollisionInfo)
			{
				CInfo.Clear();
				CInfo.SetReturnValues(_pCollisionInfo->m_ReturnValues);
				if (bOrder)
				{
					bIntersect = Phys_IntersectStates(*pPhys, _PhysState, *pObjMat, *pObjMat, *pOrigin, *pDest, &CInfo, NotifyFlags2, NotifyFlags1);
					bIntersect = (bIntersect & 1) + ((bIntersect & 2) << 1) + ((bIntersect & 4) >> 1);
				}
				else
				{
					bIntersect = Phys_IntersectStates(_PhysState, *pPhys, *pOrigin, *pDest, *pObjMat, *pObjMat, &CInfo, NotifyFlags1, NotifyFlags2);
					if (bIntersect) CInfo.m_Plane.n = -CInfo.m_Plane.n;
				}
			}
			else
			{
				if (bOrder)
				{
					bIntersect = Phys_IntersectStates(*pPhys, _PhysState, *pObjMat, *pObjMat, *pOrigin, *pDest, NULL, NotifyFlags2, NotifyFlags1);
					bIntersect = (bIntersect & 1) + ((bIntersect & 2) << 1) + ((bIntersect & 4) >> 1);
				}
				else
					bIntersect = Phys_IntersectStates(_PhysState, *pPhys, *pOrigin, *pDest, *pObjMat, *pObjMat, NULL, NotifyFlags1, NotifyFlags2);
			}

			// Intersection?
			if (bIntersect)
			{
				// Add to notify selection1?
				if (bDoNotify1 && (bIntersect & 2))
 				{
					if (bCheckObj1) 
						Selection_AddObject(*_pNotifySelection1, iObj);
					else
						_pNotifySelection1->AddData(iObj);
				}

				// Add to notify selection2?
				if (bDoNotify2 && (bIntersect & 4))
				{
					if (bCheckObj2) 
						Selection_AddObject(*_pNotifySelection2, iObj);
					else
						_pNotifySelection2->AddData(iObj);
				}

				// Collision info?
				if (bDoIntersect && (bIntersect & 1))
				{
					if (_pCollisionInfo) 
					{
						// This one closer?
						CInfo.m_iObject = iObj;
						_pCollisionInfo->Improve(CInfo);
						if (_pCollisionInfo->IsComplete())
						{
							bComplete = true;
							if (!bCheckAllForNotify)
							{
								goto Finish;
							}
						}
					}
					else
					{
						if (!bCheckAllForNotify)
						{
							return true;
						}

						bComplete = true;
						iCInfoObj = iObj;
					}
				}
			}
		}
	}

	// No collision info?, just return true/false.
	if (!_pCollisionInfo)
	{
		return (iCInfoObj != -1);
	}
Finish:
	// Finalize collision info
	if (_pCollisionInfo->m_bIsValid && (_pCollisionInfo->m_ReturnValues & CXR_COLLISIONRETURNVALUE_SURFACEVELOCITY))
	{
		CWObject* pCObj = Object_Get(_pCollisionInfo->m_iObject);
		if (pCObj)
		{
			CMat4Dfp32 NewMat, VelMat;

			pCObj->GetVelocityMatrix(VelMat);
			pCObj->GetPositionMatrix().Multiply3x3(VelMat, NewMat);
			CVec3Dfp32::GetMatrixRow(NewMat, 3) += pCObj->GetPosition();
			CVec3Dfp32::GetMatrixRow(NewMat, 3) += pCObj->GetMoveVelocity();

			CVec3Dfp32 p1(_pCollisionInfo->m_LocalPos);
			CVec3Dfp32 p2(_pCollisionInfo->m_LocalPos);
			p1 *= pCObj->GetPositionMatrix();
			p2 *= NewMat;
			_pCollisionInfo->m_Velocity = p2 - p1;
		}
	}

	return _pCollisionInfo->m_bIsCollision;
}

bool CWorld_ServerCore::Phys_SetPosition(const CSelection& _selection, int _iObj, const CMat4Dfp32& _Pos, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWorld_ServerCore_Phys_SetPosition, false);
	MSCOPESHORT(CWorld_ServerCore::Phys_SetPosition);

	CWObject* pObj = m_lspObjects[_iObj];
	if (!pObj) Error("Phys_SetPosition", "NULL Object.");

#ifdef PHYSSTATE_DEBUG
	if (pObj->GetLocalPosition().Distance(CVec3Dfp32::GetRow(_Pos, 3)) > 128.0f)
	{
		ConOutL("-------------------------------------------------------------------");
		ConOutL(CStrF("§cf80WARNING: (PhysState::SetPos) Teleported from %s to %s", pObj->GetLocalPositionMatrix().GetString().Str(), _Pos.GetString().Str()));
		ConOutL(pObj->Dump(m_spMapData, -1));
		ConOutL("-------------------------------------------------------------------");
	}

#endif

	int NotifyFlags = pObj->m_IntersectNotifyFlags;

	bool bIntersect = false;
	TSelection<CSelection::LARGE_BUFFER> Selection1;
	TSelection<CSelection::LARGE_BUFFER> Selection2;
	{
		if (pObj->m_iObjectParent)
		{
			const CWObject_CoreData* pParent = ThisClass::Object_GetCD(pObj->m_iObjectParent);
			if (pParent)
			{
				CMat4Dfp32 WPos;
				if (pObj->m_ClientFlags & CWO_CLIENTFLAGS_NOROTINHERITANCE)
				{
					WPos = _Pos;
					CVec3Dfp32::GetRow(WPos, 3) += pParent->GetPosition();
				}
				else
				{
					_Pos.Multiply(pParent->GetPositionMatrix(), WPos);
				}
				bIntersect = Phys_IntersectWorld(&_selection, pObj->m_PhysState, WPos, WPos, _iObj, _pCollisionInfo, NotifyFlags, &Selection1, &Selection2);

				if (_pCollisionInfo && _pCollisionInfo->m_bIsValid && (_pCollisionInfo->m_ReturnValues & CXR_COLLISIONRETURNVALUE_POSITION))
				{
					CMat4Dfp32 ParentInv;
					if (pObj->m_ClientFlags & CWO_CLIENTFLAGS_NOROTINHERITANCE)
					{
						ParentInv.Unit();
						CVec3Dfp32::GetRow(ParentInv, 3) = -pParent->GetPosition();
					}
					else
						pParent->GetPositionMatrix().InverseOrthogonal(ParentInv);

					_pCollisionInfo->m_Pos *= ParentInv;
					_pCollisionInfo->m_Plane.Transform(ParentInv);
				}
			}
			else
				bIntersect = Phys_IntersectWorld(&_selection, pObj->m_PhysState, _Pos, _Pos, _iObj, _pCollisionInfo, NotifyFlags, &Selection1, &Selection2);
		}
		else
			bIntersect = Phys_IntersectWorld(&_selection, pObj->m_PhysState, _Pos, _Pos, _iObj, _pCollisionInfo, NotifyFlags, &Selection1, &Selection2);

		if (!bIntersect)
		{
			DEBUG_CHECK_MATRIX(_Pos);
			pObj->m_LocalPos = _Pos;
			Phys_InsertPosition(_iObj, pObj);

			{
				const int16* pSel;
				int nSel = Selection_Get(Selection1, &pSel);
				for(int iSelObj = 0; iSelObj < nSel; iSelObj++)
					MessageQueue_SendToObject(CWObject_Message(OBJSYSMSG_NOTIFY_INTERSECTION, pSel[iSelObj]), _iObj);
			}

			{
				const int16* pSel;
				int nSel = Selection_Get(Selection2, &pSel);
				for(int iSelObj = 0; iSelObj < nSel; iSelObj++)
					MessageQueue_SendToObject(CWObject_Message(OBJSYSMSG_NOTIFY_INTERSECTION, _iObj), pSel[iSelObj]);
			}
		}
	}
	return (!bIntersect);
}




void CWorld_ServerCore::GetMovementBounds( float *_BoxMinMax, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest )
{
	MSCOPESHORT(CWorld_ServerCore::GetMovementBounds);

	// WOrigin and WDest -> defines matrices

	CMat4Dfp32 WOrigin = _Origin;
	CMat4Dfp32 WDest = _Dest;

	CWObject_CoreData* pObj = m_lspObjects[_iObj];

	M_ASSERT( pObj, "GetMovementBounds - NULL Object.");

	if (pObj->m_iObjectParent)
	{
		const CWObject_CoreData* pParent = ThisClass::Object_GetCD(pObj->m_iObjectParent);

		if (pParent)
		{
			if (pObj->m_ClientFlags & CWO_CLIENTFLAGS_NOROTINHERITANCE)
			{
				CVec3Dfp32::GetRow(WDest, 3) += pParent->GetPosition();
				CVec3Dfp32::GetRow(WOrigin, 3) += pParent->GetPosition();
			}
			else
			{
				_Origin.Multiply(pParent->GetPositionMatrix(), WOrigin);
				_Dest.Multiply(pParent->GetPositionMatrix(), WDest);
			}
		}
	}

	// now bound both WOrigin and WDest bounds, into [x,y,z,time] bound

	if (!(pObj->m_PhysState.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION))
	{
		WOrigin.Unit3x3();
		WDest.Unit3x3();
	}

	CBox3Dfp32 BoxSrc;
	CWO_EnumParams_Box Params;

	Phys_GetMinMaxBox(pObj->m_PhysState, WOrigin, BoxSrc);
	Phys_GetMinMaxBox(pObj->m_PhysState, WDest, Params.m_Box);
	Params.m_Box.Expand(BoxSrc);

	_BoxMinMax[0] = Params.m_Box.m_Min[0];
	_BoxMinMax[1] = Params.m_Box.m_Min[1];
	_BoxMinMax[2] = Params.m_Box.m_Min[2];
	_BoxMinMax[3] = Params.m_Box.m_Max[0];
	_BoxMinMax[4] = Params.m_Box.m_Max[1];
	_BoxMinMax[5] = Params.m_Box.m_Max[2];
}





void CWorld_ServerCore::Selection_GetArray( CPotColSet *_pcs, const CSelection* _pSelection, const CWO_PhysicsState &_PhysState, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest)
{
	MAUTOSTRIP(CWorld_ServerCore_Selection_GetArray, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_ServerCore::Selection_GetArray);

	if (!_PhysState.m_nPrim) return;

	// ServerCore always Notify!
	M_ASSERT( m_lspObjects[_iObj], "!" );
	int NotifyFlags = m_lspObjects[_iObj]->m_IntersectNotifyFlags;

	M_ASSERT(m_spSpaceEnum != NULL, "!");

	const int16* piEnum = NULL;
	int i = 0, nEnum = 0;

	{ // Extract Object indices within bounds

		CMat4Dfp32 WOrigin = _Origin;
		CMat4Dfp32 WDest = _Dest;

		if (!(_PhysState.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION))
		{ // Object does not have any rotation, make sure to wipe it out of matrices
			WOrigin.Unit3x3();
			WDest.Unit3x3();
		}

		if (_pSelection != NULL)
		{ // A valid selection was provided in input
			nEnum = Selection_Get(*_pSelection, &piEnum);
		}
		else
		{ // Have to enumerate a new selection		
			CWO_EnumParams_Box Params;

			Params.m_ObjectFlags = _PhysState.m_ObjectIntersectFlags | NotifyFlags;
			Params.m_ObjectIntersectFlags = _PhysState.m_ObjectFlags;
			Params.m_ObjectNotifyFlags = _PhysState.m_ObjectFlags;

			CBox3Dfp32 BoxSrc;
			Phys_GetMinMaxBox(_PhysState, WOrigin, BoxSrc);
			Phys_GetMinMaxBox(_PhysState, WDest, Params.m_Box);
			Params.m_Box.Expand(BoxSrc);
			nEnum = m_spSpaceEnum->EnumerateBox(Params, m_lEnumSpace.GetBasePtr(), m_lEnumSpace.Len());
			piEnum = m_lEnumSpace.GetBasePtr();
		}
	}


	// iterate through all objects and all their primitives

	for( i = 0; i < nEnum; i++)
	{

		int iObj = piEnum[i];

		if( iObj == _iObj || iObj == _PhysState.m_iExclude )
			continue;

		CWObject_CoreData* pObj = ThisClass::Object_GetCD(iObj);

		if (!pObj) continue;

		CWO_PhysicsState* pPhys = &pObj->m_PhysState;

		if (pPhys->m_iExclude == _iObj)
			continue;

		M_ASSERT( m_lspObjects[iObj], "!" );


		int bDoNotify1 = pPhys->m_ObjectFlags & NotifyFlags;
		int bDoNotify2 = m_lspObjects[iObj]->m_IntersectNotifyFlags & _PhysState.m_ObjectFlags;

		int bDoIntersect = 
			(pPhys->m_ObjectIntersectFlags & _PhysState.m_ObjectFlags) ||
			(pPhys->m_ObjectFlags & _PhysState.m_ObjectIntersectFlags);

		int8 IN1N2Flags = 0;

		if( bDoIntersect ) IN1N2Flags |= PCS_DO_INTERSECT;
		if( bDoNotify1 ) IN1N2Flags |= PCS_DO_NOTIFY1;
		if( bDoNotify2 ) IN1N2Flags |= PCS_DO_NOTIFY2;

		if( IN1N2Flags == 0 )
			continue;

		CMat4Dfp32 WObjOrigin = pObj->GetPositionMatrix();

		if (!(pObj->m_PhysState.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION))
		{ // remove rotation from matrix
			WObjOrigin.Unit3x3();
		}


		for(int iPrim = 0; iPrim < pPhys->m_nPrim; iPrim++)
		{
			// All primitives of objects in selection considered as STATIC, and the object itself to be MOVING!

			// for real moving objects should be kept separate so they can be (mathematically) integrated
			// while this set should only have static members.

			CWO_PhysicsPrim	&PhysPrim = pPhys->m_Prim[iPrim];
			CMat4Dfp32 WPrimOrigin = WObjOrigin;

			// Add offset to primitive?
			if (pPhys->m_PhysFlags & OBJECT_PHYSFLAGS_OFFSET)
			{
				CVec3Dfp32 WOffs( PhysPrim.GetOffset() );

				if( pPhys->m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION )
				{
					WPrimOrigin.k[3][0] += WOffs[0] * WPrimOrigin.k[0][0] + WOffs[1] * WPrimOrigin.k[0][1] + WOffs[2] * WPrimOrigin.k[0][2];
					WPrimOrigin.k[3][1] += WOffs[0] * WPrimOrigin.k[1][0] + WOffs[1] * WPrimOrigin.k[1][1] + WOffs[2] * WPrimOrigin.k[1][2];
					WPrimOrigin.k[3][2] += WOffs[0] * WPrimOrigin.k[2][0] + WOffs[1] * WPrimOrigin.k[2][1] + WOffs[2] * WPrimOrigin.k[2][2];
				}
				else
				{
					WPrimOrigin.k[3][0] += WOffs[0];
					WPrimOrigin.k[3][1] += WOffs[1];
					WPrimOrigin.k[3][2] += WOffs[2];
				}
			}


			switch( PhysPrim.m_PrimType )
			{

			case OBJECT_PRIMTYPE_NONE :
				{
					continue;
				}

			case OBJECT_PRIMTYPE_PHYSMODEL :
				{
					CXR_Model* pModel = m_spMapData->GetResource_Model( PhysPrim.m_iPhysModel );
					M_ASSERT( pModel, "Physics, a primitive is lacking a CXR_Model!" );
					CXR_PhysicsModel* pPhysModel = pModel->Phys_GetInterface();
					M_ASSERT( pPhysModel, "Physics, a CXR_Model is lacking an CXR_PhysicsModel interface!" );

					CXR_PhysicsContext PhysContext(WPrimOrigin);
					PhysContext.m_PhysGroupMaskThis = PhysPrim.m_PhysModelMask;
					pPhysModel->Phys_Init(&PhysContext);
					pPhysModel->CollectPCS(&PhysContext, IN1N2Flags, _pcs, iObj, _PhysState.m_MediumFlags );

#ifdef PCSCHECK_INSOLID
					if (_pcs->m_bIsInSolid && _pcs->m_iSolidObj == -1)
					{
						_pcs->m_iSolidObj = iObj;
						_pcs->m_SolidFlags = IN1N2Flags;
					}
#endif
					break;
				}

			case OBJECT_PRIMTYPE_SPHERE :
				{
					// radien + 16.f ???? 
					_pcs->AddSphere( IN1N2Flags, iObj, WPrimOrigin.GetRow(3).k, PhysPrim.GetRadius() + 0.0001f );
					break;
				}

			case OBJECT_PRIMTYPE_BOX :
				{
					// MUPPJOKKO - IMPLEMENT ME! tillåta roterade boxar?

					m_PhysModel_Box.Phys_SetDimensions(PhysPrim.GetDim());

					float fBoxMinMax[6];

					CPhysOBB OBBOrg;
					OBBOrg.SetDimensions(PhysPrim.GetDim());
					OBBOrg.SetPosition( WPrimOrigin );

					fBoxMinMax[0] = OBBOrg.m_C[0] - OBBOrg.m_E[0];
					fBoxMinMax[1] = OBBOrg.m_C[1] - OBBOrg.m_E[1];
					fBoxMinMax[2] = OBBOrg.m_C[2] - OBBOrg.m_E[2];
					fBoxMinMax[3] = OBBOrg.m_C[0] + OBBOrg.m_E[0];
					fBoxMinMax[4] = OBBOrg.m_C[1] + OBBOrg.m_E[1];
					fBoxMinMax[5] = OBBOrg.m_C[2] + OBBOrg.m_E[2];

					_pcs->AddBox( IN1N2Flags, iObj, fBoxMinMax );
					break;
				}

			case OBJECT_PRIMTYPE_POINT :
				{
					_pcs->AddPoint( IN1N2Flags, iObj, CVec3Dfp32::GetMatrixRow(WPrimOrigin, 3).k ); 
					break;
				}

			default :
				Error("Phys_", CStrF("Invalid primitive type. (%d)", PhysPrim.m_PrimType ));
			}
			continue;
		}

	}
}


bool CWorld_ServerCore::Phys_MovePosition(CPotColSet *_pcs, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWorld_ServerCore_Phys_MovePosition_PCS, false);
	MSCOPESHORT(CWorld_ServerCore::Phys_MovePosition_PCS);

	CWObject* pObj = m_lspObjects[_iObj];
	if (!pObj) Error("Phys_MovePosition", "NULL Object.");

	int NotifyFlags = pObj->m_IntersectNotifyFlags;

	bool bIntersect = false;

	TSelection<CSelection::LARGE_BUFFER> Selection1;
	TSelection<CSelection::LARGE_BUFFER> Selection2;
	{
		if (_pCollisionInfo) 
			_pCollisionInfo->Clear();

		if (pObj->m_iObjectParent)
		{
			const CWObject_CoreData* pParent = ThisClass::Object_GetCD(pObj->m_iObjectParent);
			if (pParent)
			{
				CMat4Dfp32 WOrigin;
				CMat4Dfp32 WDest;
				if (pObj->m_ClientFlags & CWO_CLIENTFLAGS_NOROTINHERITANCE)
				{
					WDest = _Dest;
					CVec3Dfp32::GetRow(WDest, 3) += pParent->GetPosition();
					WOrigin = _Origin;
					CVec3Dfp32::GetRow(WOrigin, 3) += pParent->GetPosition();
				}
				else
				{
					_Origin.Multiply(pParent->GetPositionMatrix(), WOrigin);
					_Dest.Multiply(pParent->GetPositionMatrix(), WDest);
				}

				bIntersect = Phys_IntersectWorld(_pcs, pObj->m_PhysState, WOrigin, WDest, _iObj, _pCollisionInfo, NotifyFlags, &Selection1, &Selection2);

				if (_pCollisionInfo && _pCollisionInfo->m_bIsValid && (_pCollisionInfo->m_ReturnValues & CXR_COLLISIONRETURNVALUE_POSITION))
				{
					CMat4Dfp32 ParentInv;
					if (pObj->m_ClientFlags & CWO_CLIENTFLAGS_NOROTINHERITANCE)
					{
						ParentInv.Unit();
						CVec3Dfp32::GetRow(ParentInv, 3) = -pParent->GetPosition();
					}
					else
						pParent->GetPositionMatrix().InverseOrthogonal(ParentInv);

					_pCollisionInfo->m_Pos *= ParentInv;
					_pCollisionInfo->m_Plane.Transform(ParentInv);
				}
			}
			else
			{
				bIntersect = Phys_IntersectWorld(_pcs, pObj->m_PhysState, _Origin, _Dest, _iObj, _pCollisionInfo, NotifyFlags, &Selection1, &Selection2);
			}
		}
		else
		{
			bIntersect = Phys_IntersectWorld(_pcs, pObj->m_PhysState, _Origin, _Dest, _iObj, _pCollisionInfo, NotifyFlags, &Selection1, &Selection2);
		}


		if (!bIntersect)
		{
			int nSel, iSelObj;
			const int16* pSel;

			nSel = Selection_Get(Selection1, &pSel);
			for( iSelObj = 0; iSelObj < nSel; iSelObj++)
			{
				MessageQueue_SendToObject(CWObject_Message(OBJSYSMSG_NOTIFY_INTERSECTION, pSel[iSelObj]), _iObj);
			}

			nSel = Selection_Get(Selection2, &pSel);
			for( iSelObj = 0; iSelObj < nSel; iSelObj++)
			{
				MessageQueue_SendToObject(CWObject_Message(OBJSYSMSG_NOTIFY_INTERSECTION, _iObj), pSel[iSelObj]);
			}
		}


		if (!bIntersect)
		{
			DEBUG_CHECK_MATRIX(_Dest);
			pObj->m_LocalPos = _Dest;
			pObj->m_DirtyMask |= m_DirtyMask_InsertPosition;
			Phys_InsertPosition(_iObj, pObj);

		}
	}

	return (!bIntersect);
}


bool CWorld_ServerCore::Phys_MovePosition(const CSelection* _pSelection, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWorld_ServerCore_Phys_MovePosition_NoPCS, false);
	MSCOPESHORT(CWorld_ServerCore::Phys_MovePosition_NoPCS);

	CWObject* pObj = m_lspObjects[_iObj];
	if (!pObj) Error("Phys_MovePosition", "NULL Object.");

#ifdef PHYSSTATE_DEBUG
	if (CVec3Dfp32::GetRow(_Origin, 3).Distance(CVec3Dfp32::GetRow(_Dest, 3)) > 128.0f)
	{
		ConOutL("-------------------------------------------------------------------");
		ConOutL(CStrF("§cf80WARNING: (PhysState::MovePos) Teleported from %s to %s", _Origin.GetString().Str(), _Dest.GetString().Str()));
		ConOutL(pObj->Dump(m_spMapData, -1));
		ConOutL("-------------------------------------------------------------------");
	}

#endif

	int NotifyFlags = pObj->m_IntersectNotifyFlags;

	bool bIntersect = false;
	TSelection<CSelection::LARGE_BUFFER> Selection1;
	TSelection<CSelection::LARGE_BUFFER> Selection2;
	{
		if (_pCollisionInfo) 
			_pCollisionInfo->Clear();

		if (pObj->m_iObjectParent)
		{
			const CWObject_CoreData* pParent = ThisClass::Object_GetCD(pObj->m_iObjectParent);
			if (pParent)
			{
				CMat4Dfp32 WOrigin;
				CMat4Dfp32 WDest;
				if (pObj->m_ClientFlags & CWO_CLIENTFLAGS_NOROTINHERITANCE)
				{
					WDest = _Dest;
					CVec3Dfp32::GetRow(WDest, 3) += pParent->GetPosition();
					WOrigin = _Origin;
					CVec3Dfp32::GetRow(WOrigin, 3) += pParent->GetPosition();
				}
				else
				{
					_Origin.Multiply(pParent->GetPositionMatrix(), WOrigin);
					_Dest.Multiply(pParent->GetPositionMatrix(), WDest);
				}
				bIntersect = Phys_IntersectWorld(_pSelection, pObj->m_PhysState, WOrigin, WDest, _iObj, _pCollisionInfo, NotifyFlags, &Selection1, &Selection2);

				if (_pCollisionInfo && _pCollisionInfo->m_bIsValid && (_pCollisionInfo->m_ReturnValues & CXR_COLLISIONRETURNVALUE_POSITION))
				{
					CMat4Dfp32 ParentInv;
					if (pObj->m_ClientFlags & CWO_CLIENTFLAGS_NOROTINHERITANCE)
					{
						ParentInv.Unit();
						CVec3Dfp32::GetRow(ParentInv, 3) = -pParent->GetPosition();
					}
					else
						pParent->GetPositionMatrix().InverseOrthogonal(ParentInv);

					_pCollisionInfo->m_Pos *= ParentInv;
					_pCollisionInfo->m_Plane.Transform(ParentInv);
				}
			}
			else
				bIntersect = Phys_IntersectWorld(_pSelection, pObj->m_PhysState, _Origin, _Dest, _iObj, _pCollisionInfo, NotifyFlags, &Selection1, &Selection2);
		}
		else
			bIntersect = Phys_IntersectWorld(_pSelection, pObj->m_PhysState, _Origin, _Dest, _iObj, _pCollisionInfo, NotifyFlags, &Selection1, &Selection2);

		if (!bIntersect)
		{
			DEBUG_CHECK_MATRIX(_Dest);
			pObj->m_LocalPos = _Dest;
			pObj->m_DirtyMask |= m_DirtyMask_InsertPosition;
			Phys_InsertPosition(_iObj, pObj);

			{
				const int16* pSel;
				int nSel = Selection_Get(Selection1, &pSel);
				for(int iSelObj = 0; iSelObj < nSel; iSelObj++)
					MessageQueue_SendToObject(CWObject_Message(OBJSYSMSG_NOTIFY_INTERSECTION, pSel[iSelObj]), _iObj);
			}

			{
				const int16* pSel;
				int nSel = Selection_Get(Selection2, &pSel);
				for(int iSelObj = 0; iSelObj < nSel; iSelObj++)
					MessageQueue_SendToObject(CWObject_Message(OBJSYSMSG_NOTIFY_INTERSECTION, _iObj), pSel[iSelObj]);
			}
		}
	}

	// PHYSLOG:
	/*if (_pCollisionInfo)
	ConOutL(CStrF("Phys_MovePosition %d, Valid %d, Distance %f, Plane ", bIntersect, _pCollisionInfo->m_bIsValid, _pCollisionInfo->m_Distance) + _pCollisionInfo->m_Plane.GetString() + pObj->GetVelocity().m_Move.GetString());
	else
	ConOutL(CStrF("Phys_MovePosition %d", bIntersect));*/

	return (!bIntersect);
}

aint CWorld_ServerCore::Phys_Message_SendToObject(const CWObject_Message& _Msg, int _iObj)
{
	MAUTOSTRIP(CWorld_ServerCore_Phys_Message_SendToObject, 0);
	return ThisClass::Message_SendToObject(_Msg, _iObj);
}

bool CWorld_ServerCore::Phys_MessageQueue_SendToObject(const CWObject_Message& _Msg, int _iObj)
{
	MAUTOSTRIP(CWorld_ServerCore_Phys_MessageQueue_SendToObject, false);
	return ThisClass::MessageQueue_SendToObject(_Msg, _iObj);
}

void CWorld_ServerCore::Phys_MessageQueue_Flush()
{
	MAUTOSTRIP(CWorld_ServerCore_Phys_MessageQueue_Flush, MAUTOSTRIP_VOID);
	ThisClass::MessageQueue_Flush();
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Rigid Body Physics Interface
|__________________________________________________________________________________________________
\*************************************************************************************************/

void CWorld_ServerCore::Phys_SetCollisionPrecision(int _Level)
{
#ifndef WSERVER_DYNAMICSENGINE2
	m_DynamicsWorld.SetCollisionPrecision(_Level);
#endif
}

int CWorld_ServerCore::Phys_GetCollisionPrecision()
{
#ifndef WSERVER_DYNAMICSENGINE2
	return m_DynamicsWorld.GetCollisionPrecision();
#endif
	return 0;
}

void CWorld_ServerCore::Phys_AddImpulse(uint16 _iObject, const CVec3Dfp32& _ApplyAt, const CVec3Dfp32& _Velocity, fp32 _Mass, fp32 _Restitution)
{
	CWObject* pObj = CWorld_ServerCore::Object_Get(_iObject);

#ifdef WSERVER_DYNAMICSENGINE2
	if (pObj && pObj->m_pRigidBody2)
	{
		// TODO: SetStationary is temporary, should be implied if the impulse added is big enough.
		pObj->ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

		pObj->m_pRigidBody2->m_ExternalForce += CVec4Dfp32(_Velocity[0], _Velocity[1], _Velocity[2], 0.0f);
		Phys_SetStationary(_iObject, false);
	}
#else
	if (pObj && pObj->m_pRigidBody)
	{
		// TODO: SetStationary is temporary, should be implied if the impulse added is big enough.
		m_DynamicsWorld.SetStationary(pObj->m_pRigidBody, false);
		pObj->ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

		m_DynamicsWorld.AddImpulse(pObj->m_pRigidBody, _ApplyAt, _Velocity, _Mass, _Restitution);

		Phys_SetStationary(_iObject, false);
	}
#endif
}


static void RigidBodyStateFromPhysClusterObject(CWPhys_ClusterObject &_PO,
												CWD_RigidBodyState& _State,fp32 _GameTickTimeInv)
{
	const fp32 PosScale = 1.0f / 32.0f;
	const fp32 MoveVelScale = (1.0f / 32.0f) * _GameTickTimeInv;		// [units/tick] -> [m/s]
	const fp32 RotVelScale = (-_PI2) * _GameTickTimeInv;				// [angle1/tick] -> [rad/s]

	CWD_RigidBody2 *pRB = _PO.m_pRB;

	CMat4Dfp32 T = _PO.m_Transform;
	const CVec3Dfp32 P = (pRB->m_CenterOfMass * T) * PosScale;
	CQuatfp32 Q;
	T.Transpose3x3();
	Q.Create(T);

	const CVelocityfp32 &Vel = _PO.m_Velocity;
	CVec3Dfp32 v = Vel.m_Move * MoveVelScale;
	CVec3Dfp32 w = Vel.m_Rot.m_Axis * Vel.m_Rot.m_Angle * RotVelScale;

	_State.Create(P,Q,v,w,pRB->m_Mass,pRB->m_InertiaTensor);
}

void CWorld_ServerCore::Phys_AddRigidBodyToSimulation(uint16 _iObject, bool _bActivateConnected)
{
	fp32 GameTickTimeInv = GetGameTicksPerSecond();
	CWObject* pObj = m_lspObjects[_iObject];

	// YES, this assert SHOULD be here!
	M_ASSERT(pObj && (pObj->m_pRigidBody2 || pObj->m_pPhysCluster), "!");
	M_ASSERT(pObj->m_pPhysCluster || pObj->m_pRigidBody2->m_bInSimulation == 0, "Logical error somewhere - duh!");

	if (!(pObj->GetPhysState().m_PhysFlags & OBJECT_PHYSFLAGS_NODYNAMICSUPDATE))
		pObj->ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

	int nAdded = 0;
	if( pObj->m_pPhysCluster )
	{
		int nObj = pObj->m_pPhysCluster->m_lObjects.Len();
		for(int i = 0;i < nObj;i++)
		{
			CWPhys_ClusterObject &PCO = pObj->m_pPhysCluster->m_lObjects[i];
			if( PCO.m_pRB->IsStationary() || PCO.m_pRB->m_bInSimulation ) continue;

			CWD_RigidBodyState State;
			State.Clear();
			RigidBodyStateFromPhysClusterObject(PCO,State,GameTickTimeInv);
			CWD_RigidBody2 * pRB = PCO.m_pRB;
			m_DynamicsWorld2.AddRigidBody(pRB,State);
			pRB->m_bInSimulation = 1;
			pRB->m_bUseOriginalFreezeThreshold = 0;
			nAdded++;
		}

		if(! nAdded ) return;
	}
	else
	{ // RigidBodyStateFromObject(pObj, State, GameTickTimeInv);
		const fp32 PosScale = 1.0f / 32.0f;
		const fp32 MoveVelScale = (1.0f / 32.0f) * GameTickTimeInv;		// [units/tick] -> [m/s]
		const fp32 RotVelScale = (-_PI2) * GameTickTimeInv;				// [angle1/tick] -> [rad/s]

		CWD_RigidBody2* pRB = pObj->m_pRigidBody2;
		CMat4Dfp32 T = pObj->GetPositionMatrix();
		const CVec3Dfp32 P = (pRB->m_CenterOfMass * T) * PosScale;
		CQuatfp32 Q;
		T.Transpose3x3();
		Q.Create(T);

		const CVelocityfp32& ObjVelocity = pObj->GetVelocity();
		CVec3Dfp32 v = ObjVelocity.m_Move * MoveVelScale;
		CVec3Dfp32 w = ObjVelocity.m_Rot.m_Axis * ObjVelocity.m_Rot.m_Angle * RotVelScale;

		fp32 Mass = pRB->GetMass();
		CVec3Dfp32 InertiaTensor = pRB->m_InertiaTensor;

		// Make sure mass ratio of connected objects never exceeds 4:1
		if (pObj->m_liConnectedToConstraint.Len() > 0)
		{
			fp32 SysMassMax, SysMassTotal;
			ThisClass::Phys_GetSystemMass(_iObject, SysMassTotal, SysMassMax);
			Mass = Max(Mass, SysMassMax * 0.25f);
			InertiaTensor *= Mass / pRB->GetMass();
		}

		CWD_RigidBodyState State;
		State.Clear();
		State.Create(P, Q, v, w, Mass, InertiaTensor);

		pRB->m_bInSimulation = 1;
		pRB->m_bUseOriginalFreezeThreshold = 0;

		// Need to insert this index into the bbox, Is this done at the correct time?
		const CWO_PhysicsState &PhysState = pObj->GetPhysState();  
		PhysState.m_RigidBodyID = m_DynamicsWorld2.AddRigidBody(pObj->m_pRigidBody2, State);
	}
	
	m_ObjPoolDynamics.Insert(_iObject);
	

	if (_bActivateConnected)
	{
		TAP<const uint32> piConnectedTo = pObj->m_liConnectedToConstraint;
		for (uint i = 0; i < piConnectedTo.Len(); i++)
		{
			uint32 ConstraintID = piConnectedTo[i];
			CWD_ConstraintDescriptor& ConstraintDesc = *ThisClass::Phys_GetConstraint(ConstraintID);

			if (!ConstraintDesc.InSimulation())
				AddConstraintToSimulation(ConstraintDesc);
		}
	}
}


void CWorld_ServerCore::Phys_AddImpulse(CWD_RigidBody2 *_pRB,const CVec3Dfp32& _ApplyAt, const CVec3Dfp32& _Force)
{
	CWObject *pObj = (CWObject*)_pRB->m_pUserData;
	if( !_pRB->m_pUserData2 )
		return Phys_AddImpulse(pObj->m_iObject,_ApplyAt,_Force);

	CWPhys_ClusterObject *pPO = (CWPhys_ClusterObject*)_pRB->m_pUserData2;
	
	pObj->ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

	CVec3Dfp32 R = _ApplyAt - pPO->m_Transform.GetRow(3);
	R *= 1.0f / 32.0f;

	CVec3Dfp32 tmp = R / _Force;
	//ra.CrossProd(_Force,tmp);

	_pRB->m_ExternalForce += CVec4Dfp32(_Force[0], _Force[1], _Force[2], 0.0f);
	_pRB->m_ExternalTorque += CVec4Dfp32(tmp[0], tmp[1], tmp[2], 0.0f);
	ThisClass::Phys_SetStationary(pObj->m_iObject, false);
}

void CWorld_ServerCore::Phys_AddImpulse(uint16 _iObject, const CVec3Dfp32& _ApplyAt, const CVec3Dfp32& _Force)
{
	CWObject* pObj = ThisClass::Object_Get(_iObject);

#ifdef WSERVER_DYNAMICSENGINE2
	if (pObj && pObj->m_pRigidBody2)
	{
		// TODO: SetStationary is temporary, should be implied if the impulse added is big enough.
		pObj->ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;


		CVec3Dfp32 R = _ApplyAt - Phys_GetCenterOfMassCorrectedTransform(pObj).GetRow(3);
		R *= 1.0f / 32.0f;

		CVec3Dfp32 tmp = R / _Force;
		//ra.CrossProd(_Force,tmp);

		pObj->m_pRigidBody2->m_ExternalForce += CVec4Dfp32(_Force[0], _Force[1], _Force[2], 0.0f);
		pObj->m_pRigidBody2->m_ExternalTorque += CVec4Dfp32(tmp[0], tmp[1], tmp[2], 0.0f);
		ThisClass::Phys_SetStationary(_iObject, false);
	}

#else
	if (pObj && pObj->m_pRigidBody)
	{
		// TODO: SetStationary is temporary, should be implied if the impulse added is big enough.
		m_DynamicsWorld.SetStationary(pObj->m_pRigidBody, false);
		pObj->ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

		m_DynamicsWorld.AddImpulse(pObj->m_pRigidBody, _ApplyAt, _Force);

		CVec3Dfp32 Pos = (pRB->m_CenterOfMass * T) * PosScale;
		CVec3Dfp32 R = _ApplyAt - pObj->GetPosition();
		R *= 1.0f / 32.0f;

		CVec3Dfp32 tmp = R / _Force;
		//ra.CrossProd(_Force,tmp);

		pObj->m_pRigidBody2->m_ExternalForce += CVec4Dfp32(_Force[0], _Force[1], _Force[2], 0.0f);
		pObj->m_pRigidBody2->m_ExternalTorque += CVec4Dfp32(tmp[0], tmp[1], tmp[2], 0.0f);
		ThisClass::Phys_SetStationary(_iObject, false);

//		pObj->m_pRigidBody2->m_ExternalForce += CVec4Dfp32(200.0f, 0.0f, 0.0f, 0.0f);
	}
#endif
}

void CWorld_ServerCore::Phys_AddMassInvariantImpulse(uint16 _iObject, const CVec3Dfp32& _ApplyAt, const CVec3Dfp32& _Force)
{
		CWObject* pObj = CWorld_ServerCore::Object_Get(_iObject);

#ifdef WSERVER_DYNAMICSENGINE2
	if (pObj && pObj->m_pRigidBody2)
	{
		fp32 GameTickTime = GetGameTickTime();
		fp32 GameTickTimeInv = 1.0f / GameTickTime;

		ThisClass::Phys_SetStationary(pObj->m_iObject, false);
		pObj->ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

		Object_SetVelocity(pObj->m_iObject, Object_GetVelocity(pObj->m_iObject) + _Force * 32.0f * GameTickTime);


		const fp32 RotVelScale = (-_PI2) * GameTickTimeInv;				// [angle1/tick] -> [rad/s]
		const fp32 RotVelScaleInv = (-1.0f/_PI2) * GameTickTime;		// [rad/s] -> [angle1/tick]

		CVec3Dfp32 Pos = pObj->m_pRigidBody2->m_CenterOfMass;
		Pos *= pObj->GetPositionMatrix();

		CVec3Dfp32 ra = _ApplyAt - Pos;
		ra *= 1.0f / 32.0f;

		CVec3Dfp32 dw = ra / _Force;

		const CVelocityfp32& ObjVelocity = pObj->GetVelocity();
		CVec3Dfp32 w = ObjVelocity.m_Rot.m_Axis * ObjVelocity.m_Rot.m_Angle * RotVelScale;
		w += dw;

		w *= RotVelScaleInv;
		fp32 m = w.Length();
		if (m > 0.001f)
			w *= (1.0f / m);

		Object_SetRotVelocity(_iObject, CAxisRotfp32(w, m));

	}
#else
	if (pObj && pObj->m_pRigidBody)
	{
		// TODO: SetStationary is temporary, should be implied if the impulse added is big enough.
		m_DynamicsWorld.SetStationary(pObj->m_pRigidBody, false);
		pObj->ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

		m_DynamicsWorld.AddMassInvariantImpulse(pObj->m_pRigidBody, _ApplyAt, _Force);
	}
#endif
	
}

void CWorld_ServerCore::Phys_AddForce(CWD_RigidBody2 *_pRB,const CVec3Dfp32& _Force)
{
	CWObject *pObj = (CWObject*)_pRB->m_pUserData;
	if( !_pRB->m_pUserData2 )
		return Phys_AddForce(pObj->m_iObject,_Force);

	//CWPhys_ClusterObject *pPO = (CWPhys_ClusterObject*)_pRB->m_pUserData2;

	pObj->ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
	_pRB->m_ExternalForce += CVec4Dfp32(_Force[0], _Force[1], _Force[2], 0.0f);
	ThisClass::Phys_SetStationary(pObj->m_iObject, false);
}

void CWorld_ServerCore::Phys_AddForce(uint16 _iObject, const CVec3Dfp32& _Force)
{
	CWObject* pObj = CWorld_ServerCore::Object_Get(_iObject);
#ifdef WSERVER_DYNAMICSENGINE2
	if (pObj && pObj->m_pRigidBody2)
	{
		// TODO: SetStationary is temporary, should be implied if the impulse added is big enough.
		pObj->ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

		pObj->m_pRigidBody2->m_ExternalForce += CVec4Dfp32(_Force[0], _Force[1], _Force[2], 0.0f);
		ThisClass::Phys_SetStationary(_iObject, false);
	}
#else
	if (pObj && pObj->m_pRigidBody)
	{
		// TODO: SetStationary is temporary, should be implied if the impulse added is big enough.
		m_DynamicsWorld.SetStationary(pObj->m_pRigidBody, false);
		pObj->ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

		m_DynamicsWorld.AddForce(pObj->m_pRigidBody, _Force);

		pObj->m_pRigidBody2->m_ExternalForce += CVec4Dfp32(_Force[0], _Force[1], _Force[2], 0.0f);
		Phys_SetStationary(_iObject, false);
	}
#endif
}


void CWorld_ServerCore::Phys_SetStationary(uint16 _iObject, bool _bStationary) 
{
#ifdef WSERVER_DYNAMICSENGINE2
	CWObject* pObj = CWorld_ServerCore::Object_Get(_iObject);

	if (pObj && pObj->m_pRigidBody2)
	{
		if (!_bStationary && (!(pObj->GetPhysState().m_PhysFlags & OBJECT_PHYSFLAGS_NODYNAMICSUPDATE)))
			pObj->ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

		if (_bStationary)
		{
			pObj->m_pRigidBody2->SetStationary(true);
			m_ObjPoolDynamics.Remove(pObj->m_iObject);
		}
		else
		{
			pObj->m_pRigidBody2->SetStationary(false);
			pObj->m_pRigidBody2->m_FreezeCounter = 0;
			m_ObjPoolDynamics.Insert(pObj->m_iObject);
		}
	}
	else if (pObj && pObj->m_pPhysCluster)
	{
		if (!_bStationary && (!(pObj->GetPhysState().m_PhysFlags & OBJECT_PHYSFLAGS_NODYNAMICSUPDATE)))
			pObj->ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
		
		TAP<CWPhys_ClusterObject> pPO = pObj->m_pPhysCluster->m_lObjects;
		for(uint i = 0;i < pPO.Len();i++)
		{
			pPO[i].m_pRB->SetStationary(_bStationary);
			if( !_bStationary ) pPO[i].m_pRB->m_FreezeCounter = 0;
		}
		
		if( _bStationary ) m_ObjPoolDynamics.Remove(pObj->m_iObject);
		else m_ObjPoolDynamics.Insert(pObj->m_iObject);
	}

#else
	CWObject* pObj = CWorld_ServerCore::Object_Get(_iObject);

	if (pObj && pObj->m_pRigidBody)
	{
		m_DynamicsWorld.SetStationary(pObj->m_pRigidBody, _bStationary);

		if (!_bStationary)
			pObj->ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
	}
#endif
}


bool CWorld_ServerCore::Phys_IsStationary(uint16 _iObject)
{
#ifdef WSERVER_DYNAMICSENGINE2
	CWObject* pObj = CWorld_ServerCore::Object_Get(_iObject);
	if (pObj && pObj->m_pRigidBody2)
		return pObj->m_pRigidBody2->IsStationary() != 0;

	if (pObj && pObj->m_pPhysCluster && pObj->m_pPhysCluster->m_lObjects.Len() &&
		pObj->m_pPhysCluster->m_lObjects[0].m_pRB != NULL)
		return pObj->m_pPhysCluster->m_lObjects[0].m_pRB->IsStationary() != 0;

	return true;

#else
	CWObject* pObj = CWorld_ServerCore::Object_Get(_iObject);
	if (pObj && pObj->m_pRigidBody)
		return pObj->m_pRigidBody->IsStationary();

	return true;

#endif
}

int CWorld_ServerCore::Phys_AddBallJointConstraint(uint16 _iObject1, uint16 _iObject2, const CVec3Dfp32& _WorldPos, fp32 _MaxAngle,uint16 _iSub1,uint16 _iSub2)
{
	CWObject* pObj1 = CWorld_ServerCore::Object_Get(_iObject1);
	CWObject* pObj2 = CWorld_ServerCore::Object_Get(_iObject2);

#ifdef WSERVER_DYNAMICSENGINE2
	M_ASSERTHANDLER(pObj1 && (pObj1->m_pRigidBody2 || pObj1->m_pPhysCluster), CStrF("Invalid _iObject1: %d", _iObject1), return -1);
	M_ASSERTHANDLER(pObj2 && (pObj2->m_pRigidBody2 || pObj2->m_pPhysCluster), CStrF("Invalid _iObject2: %d", _iObject2), return -1);

	CMat4Dfp32 InvMat;
	if(pObj1->m_pPhysCluster) pObj1->m_pPhysCluster->m_lObjects[_iSub1].m_Transform.InverseOrthogonal(InvMat);
	else pObj1->GetPositionMatrix().InverseOrthogonal(InvMat);
	CVec3Dfp32 LocalPos1 = _WorldPos;
	LocalPos1 *= InvMat;
	if(!pObj1->m_pPhysCluster) LocalPos1 -= pObj1->m_pRigidBody2->m_CenterOfMass;

	if(pObj2->m_pPhysCluster) pObj2->m_pPhysCluster->m_lObjects[_iSub2].m_Transform.InverseOrthogonal(InvMat);
	else pObj2->GetPositionMatrix().InverseOrthogonal(InvMat);
	CVec3Dfp32 LocalPos2 = _WorldPos;
	LocalPos2 *= InvMat;
	if(!pObj2->m_pPhysCluster) LocalPos2 -= pObj2->m_pRigidBody2->m_CenterOfMass;

	int ID = m_NextConstraintID++;

	CWD_ConstraintDescriptor Constraint(ID);
	Constraint.SetType(CWD_ConstraintDescriptor::BALL);
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::RA, CVec4Dfp32(LocalPos1[0], LocalPos1[1], LocalPos1[2], 1.0f));
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::RB, CVec4Dfp32(LocalPos2[0], LocalPos2[1], LocalPos2[2], 1.0f));
	Constraint.SetConnectedObjects(pObj1->m_iObject, pObj2->m_iObject);
	Constraint.SetConnectedSubObjects(_iSub1, _iSub2);

	if (Phys_DoAddConstraint(pObj1, pObj2, Constraint))
	{
		return ID;
	}
	else
	{
		return -1;
	}

#else
	M_ASSERTHANDLER(pObj1 && pObj1->m_pRigidBody, CStrF("Invalid _iObject1: %d", _iObject1), return -1);
	M_ASSERTHANDLER(pObj2 && pObj2->m_pRigidBody, CStrF("Invalid _iObject2: %d", _iObject2), return -1);

	CMat4Dfp32 InvMat;
	pObj1->GetPositionMatrix().InverseOrthogonal(InvMat);
	CVec3Dfp32 LocalPos1 = _WorldPos;
	LocalPos1 *= InvMat;
	LocalPos1 -= pObj1->m_pRigidBody->m_MassCenter.Getfp32();

	pObj2->GetPositionMatrix().InverseOrthogonal(InvMat);
	CVec3Dfp32 LocalPos2 = _WorldPos;
	LocalPos2 *= InvMat;
	LocalPos2 -= pObj2->m_pRigidBody->m_MassCenter.Getfp32();

	return m_DynamicsWorld.AddBallJointConstraint(pObj1->m_pRigidBody, pObj2->m_pRigidBody, LocalPos1, LocalPos2, _MaxAngle * _PI);
#endif
}

const CCollisionEvent* CWorld_ServerCore::Phys_GetCollisionEvents(CRigidBody *pRigidBody)
{
#ifndef WSERVER_DYNAMICSENGINE2
	TAP<const CCollisionEvent> events = m_DynamicsWorld.GetCollisionEvents();
	for(int i = 0; i < events.Len(); i++)
	{
		const CCollisionEvent& e = events[i];
		if((e.m_pRigidBodyA == pRigidBody || e.m_pRigidBodyB == pRigidBody))
			return &events[i];
	}
#endif
	return NULL;
}

/*
int CWorld_ServerCore::Phys_AddFixedPointConstraint(uint16 _iObject, const CVec3Dfp32& _LocalPosition, const CVec3Dfp32& _WorldPosition)
{
CWObject* pObj1 = CWorld_ServerCore::Object_Get(_iObject);
M_ASSERT(pObj1 && pObj1->m_pRigidBody, "");

CVec3Dfp32 LocalPos = _LocalPosition;
LocalPos -= pObj1->m_pRigidBody->m_MassCenter.Getfp32();

return m_DynamicsWorld.AddFixedPointConstraint(pObj1->m_pRigidBody, LocalPos, _WorldPosition);
}
*/


CMat4Dfp32 CWorld_ServerCore::Phys_GetCenterOfMassCorrectedTransform(const CWObject* _pObj)
{
	CMat4Dfp32 T;
	M_ASSERTHANDLER(_pObj, "Invalid object!", {T.Unit(); return T;});
	T =  _pObj->GetPositionMatrix();
	if (_pObj->m_pRigidBody2)
		T.GetRow(3) = _pObj->m_pRigidBody2->m_CenterOfMass * T;
	return T;
}


bool CWorld_ServerCore::Phys_DoAddConstraint(CWObject_CoreData* _pObj1, CWObject_CoreData* _pObj2, const CWD_ConstraintDescriptor& _Constraint)
{
	bool ok = true;

	if (_pObj1)
	{
		if (_pObj1->m_liConnectedToConstraint.Len() < _pObj1->m_liConnectedToConstraint.GetMax())
		{
			_pObj1->m_liConnectedToConstraint.Add(_Constraint.GetID());
		}
		else
		{
			ConOutL(CStrF("§cf80WARNING: Too many constraints for object"));
			M_TRACE("WARNING: Too many constraints for object\n");
			ok = false;
		}
	}

	if (_pObj2 && (_pObj2 != _pObj1))
	{
		if (_pObj2->m_liConnectedToConstraint.Len() < _pObj2->m_liConnectedToConstraint.GetMax())
		{
			_pObj2->m_liConnectedToConstraint.Add(_Constraint.GetID());
		}
		else
		{
			ConOutL(CStrF("§cf80WARNING: Too many constraints for object"));
			M_TRACE("WARNING: Too many constraints for object\n");
			ok = false;
		}
	}

	if (ok)
		m_lConstraints.Add(_Constraint);

	return ok;
}

int CWorld_ServerCore::Phys_AddBallJointToWorld(uint16 _iObject, const CMat4Dfp32& _WorldPos, fp32 _MaxAngle, uint16 _iSub)
{
	CWObject* pObj = CWorld_ServerCore::Object_Get(_iObject);

#ifdef WSERVER_DYNAMICSENGINE2
	M_ASSERT(pObj && (pObj->m_pRigidBody2 || pObj->m_pPhysCluster), "");


	CMat4Dfp32 T1 = pObj->m_pPhysCluster ? pObj->m_pPhysCluster->m_lObjects[_iSub].m_Transform
		: Phys_GetCenterOfMassCorrectedTransform(pObj);
	CMat4Dfp32 T1Inv;
	T1.InverseOrthogonal(T1Inv);
	CVec3Dfp32 R = _WorldPos.GetRow(3) * T1Inv;

	int ID = m_NextConstraintID++;

	CVec3Dfp32 WorldRef = _WorldPos.GetRow(3);

	CWD_ConstraintDescriptor Constraint(ID);
	Constraint.SetType(CWD_ConstraintDescriptor::BALLWORLD);
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::RA, CVec4Dfp32(R[0], R[1], R[2], 1.0f));
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::WORLDREF, CVec4Dfp32(WorldRef[0], WorldRef[1], WorldRef[2], 1.0f));
	Constraint.SetConnectedObjects(pObj->m_iObject, 0);
	Constraint.SetConnectedSubObjects(_iSub, -1);

	if (Phys_DoAddConstraint(pObj, NULL, Constraint))
	{
		return ID;
	}
	else
	{
		return -1;
	}
#else
	M_ASSERT(pObj && pObj->m_pRigidBody, "");

	CMat4Dfp32 InvMat;
	pObj->GetPositionMatrix().InverseOrthogonal(InvMat);
	CVec3Dfp32 LocalPos = _WorldPos.GetRow(3);
	LocalPos *= InvMat;
	LocalPos -= pObj->m_pRigidBody->m_MassCenter.Get<fp32>();

	// Flip rotation axis if needed
	CVec3Dfp32 Axis = _WorldPos.GetRow(2);
	CVec3Dfp32 MCPos = pObj->m_pRigidBody->m_MassCenter.Get<fp32>();
	MCPos *= pObj->GetPositionMatrix();
	CVec3Dfp32 PosToObj = MCPos - _WorldPos.GetRow(3);
	if ((Axis * PosToObj) < 0.0f)
		Axis *= -1.0f;

	return m_DynamicsWorld.AddBallJointToWorld(pObj->m_pRigidBody, LocalPos, _WorldPos.GetRow(3), Axis, _MaxAngle * _PI);
#endif
}


int CWorld_ServerCore::Phys_AddAxisConstraint(uint16 _iObject, const CMat4Dfp32& _WorldPos, fp32 _AxisLength, fp32 _MaxAngle, uint16 _iSub)
{
#ifdef WSERVER_DYNAMICSENGINE2
	_AxisLength *= 2.0f;

	CWObject* pObj = CWorld_ServerCore::Object_Get(_iObject);
	M_ASSERT(pObj && (pObj->m_pRigidBody2 || pObj->m_pPhysCluster), "");

	CMat4Dfp32 T = pObj->m_pPhysCluster ? pObj->m_pPhysCluster->m_lObjects[_iSub].m_Transform
		: Phys_GetCenterOfMassCorrectedTransform(pObj);
	CMat4Dfp32 TInv;
	T.InverseOrthogonal(TInv);

	CVec3Dfp32 R = _WorldPos.GetRow(3) * TInv;

	int ID = m_NextConstraintID++;

	CVec3Dfp32 WorldRef = _WorldPos.GetRow(3);

	CVec3Dfp32 WorldAxis = _WorldPos.GetRow(2) * _AxisLength;
	const CVec3Dfp32 &Anchor = _WorldPos.GetRow(3);

	CVec3Dfp32 WP1 = Anchor + WorldAxis * 0.5f;
	CVec3Dfp32 WP2 = Anchor - WorldAxis * 0.5f;

	CVec3Dfp32 LP1 = WP1 * TInv;
	CVec3Dfp32 LP2 = WP2 * TInv;
	CVec3Dfp32 LocalAxis = LP1 - LP2;

	CVec3Dfp32 AngleAxis = _WorldPos.GetRow(0);

	CWD_ConstraintDescriptor Constraint(ID);
	Constraint.SetType(CWD_ConstraintDescriptor::HINGEWORLD);
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::RA, CVec4Dfp32(R[0], R[1], R[2], 1.0f));
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::WORLDREF, CVec4Dfp32(WorldRef[0], WorldRef[1], WorldRef[2], 1.0f));
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::WORLDAXIS, CVec4Dfp32(WorldAxis[0], WorldAxis[1], WorldAxis[2], 0.0f));
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::AXISA, CVec4Dfp32(LocalAxis[0], LocalAxis[1], LocalAxis[2], 0.0f));
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::ANGLEAXISA, CVec4Dfp32(AngleAxis[0], AngleAxis[1], AngleAxis[2], 0.0f));
	Constraint.SetScalarParam(CWD_ConstraintDescriptor::MAXANGLE, _MaxAngle);

	Constraint.SetConnectedObjects(pObj->m_iObject, 0);
	Constraint.SetConnectedSubObjects(_iSub, -1);

	if (Phys_DoAddConstraint(pObj, NULL, Constraint))
	{
		return ID;
	}
	else
	{
		return -1;
	}

#else
	CWObject* pObj1 = CWorld_ServerCore::Object_Get(_iObject);
	M_ASSERT(pObj1 && pObj1->m_pRigidBody, "");

	return m_DynamicsWorld.AddAxisConstraint(pObj1->m_pRigidBody, _WorldPos.GetRow(2), _AxisLength, _WorldPos.GetRow(3), _WorldPos.GetRow(0), _MaxAngle * _PI);
#endif

}

static fp32 GetPlanarRotation(const CVec3Dfp32& _From, const CVec3Dfp32& _To, const CVec3Dfp32& _Axis)
{
	CVec3Dfp32 Ref;
	_Axis.CrossProd(_From, Ref);
	fp32 u = _To * _From;
	fp32 v = _To * Ref;
	fp32 Angle = atan2f(v, u);
	return Angle;
}

int CWorld_ServerCore::Phys_AddAxisConstraint2(uint16 _iObject, const CMat4Dfp32& _WorldPos, fp32 _AxisLength, fp32 _MinAngle, fp32 _MaxAngle, bool _bUseOriginalFreezeThreshold, uint16 _iSub)
{
#ifdef WSERVER_DYNAMICSENGINE2
	_AxisLength *= 2.0f;

	CWObject* pObj = CWorld_ServerCore::Object_Get(_iObject);
	M_ASSERT(pObj && (pObj->m_pRigidBody2 || pObj->m_pPhysCluster), "");

	CMat4Dfp32 T = pObj->m_pPhysCluster ? pObj->m_pPhysCluster->m_lObjects[_iSub].m_Transform
		: Phys_GetCenterOfMassCorrectedTransform(pObj);
	CMat4Dfp32 TInv;
	T.InverseOrthogonal(TInv);

	CVec3Dfp32 R = _WorldPos.GetRow(3) * TInv;

	int ID = m_NextConstraintID++;

	CVec3Dfp32 WorldRef = _WorldPos.GetRow(3);

	CVec3Dfp32 WorldAxis = _WorldPos.GetRow(2) * _AxisLength;
	const CVec3Dfp32 &Anchor = _WorldPos.GetRow(3);

	CVec3Dfp32 WP1 = Anchor + WorldAxis * 0.5f;
	CVec3Dfp32 WP2 = Anchor - WorldAxis * 0.5f;

	CVec3Dfp32 LP1 = WP1 * TInv;
	CVec3Dfp32 LP2 = WP2 * TInv;
	CVec3Dfp32 LocalAxis = LP1 - LP2;

	CVec3Dfp32 AngleAxis = _WorldPos.GetRow(0);


	CVec3Dfp32 Dir = T.GetRow(3) - WorldRef;
	Dir.Normalize();

	CVec3Dfp32 Tmp = WorldAxis;
	Tmp.Normalize();
	fp32 RelativeAngle =  GetPlanarRotation(AngleAxis, Dir, Tmp);

	CWD_ConstraintDescriptor Constraint(ID);
	Constraint.SetType(CWD_ConstraintDescriptor::HINGEWORLD2);
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::RA, CVec4Dfp32(R[0], R[1], R[2], 1.0f));
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::WORLDREF, CVec4Dfp32(WorldRef[0], WorldRef[1], WorldRef[2], 1.0f));
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::WORLDAXIS, CVec4Dfp32(WorldAxis[0], WorldAxis[1], WorldAxis[2], 0.0f));
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::AXISA, CVec4Dfp32(LocalAxis[0], LocalAxis[1], LocalAxis[2], 0.0f));
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::ANGLEAXISA, CVec4Dfp32(AngleAxis[0], AngleAxis[1], AngleAxis[2], 0.0f));
	Constraint.SetScalarParam(CWD_ConstraintDescriptor::MAXANGLE, _MaxAngle);
	Constraint.SetScalarParam(CWD_ConstraintDescriptor::MINANGLE, _MinAngle);
	Constraint.SetScalarParam(CWD_ConstraintDescriptor::RELATIVEANGLE, RelativeAngle);
	Constraint.SetUseOriginalFreezeThreshold(_bUseOriginalFreezeThreshold);
	Constraint.SetConnectedObjects(pObj->m_iObject, 0);
	Constraint.SetConnectedSubObjects(_iSub, -1);

	if (Phys_DoAddConstraint(pObj, NULL, Constraint))
	{
		return ID;
	}
	else
	{
		return -1;
	}

#else
	M_ASSERT(false, "NOT SUPPORTED!");
	return -1;
#endif

}

int CWorld_ServerCore::Phys_AddHingeJointConstraint(uint16 _iObject1, uint16 _iObject2, const CMat4Dfp32& _WorldPos, fp32 _AxisLength, fp32 _MaxAngle, uint16 _iSub1, uint16 _iSub2)
{
#ifdef WSERVER_DYNAMICSENGINE2

	_AxisLength *= 2.0f;

	CWObject* pObj1 = CWorld_ServerCore::Object_Get(_iObject1);
	CWObject* pObj2 = CWorld_ServerCore::Object_Get(_iObject2);
	M_ASSERT(pObj1 && (pObj1->m_pRigidBody2 || pObj1->m_pPhysCluster), "");
	M_ASSERT(pObj2 && (pObj2->m_pRigidBody2 || pObj2->m_pPhysCluster), "");

	CMat4Dfp32 T1 = pObj1->m_pPhysCluster ? pObj1->m_pPhysCluster->m_lObjects[_iSub1].m_Transform
		: Phys_GetCenterOfMassCorrectedTransform(pObj1);
	CMat4Dfp32 T2 = pObj2->m_pPhysCluster ? pObj2->m_pPhysCluster->m_lObjects[_iSub2].m_Transform
		: Phys_GetCenterOfMassCorrectedTransform(pObj2);
	CMat4Dfp32 T1Inv, T2Inv;
	T1.InverseOrthogonal(T1Inv);
	T2.InverseOrthogonal(T2Inv);

	CVec3Dfp32 RA = _WorldPos.GetRow(3) * T1Inv;
	CVec3Dfp32 RB = _WorldPos.GetRow(3) * T2Inv;

	int ID = m_NextConstraintID++;

	CVec3Dfp32 WorldAxis = _WorldPos.GetRow(2) * _AxisLength;
	const CVec3Dfp32 &Anchor = _WorldPos.GetRow(3);

	CVec3Dfp32 WP1 = Anchor - WorldAxis * 0.5f;
	CVec3Dfp32 WP2 = Anchor + WorldAxis * 0.5f;

	CVec3Dfp32 LocalAxisA = WP2 * T1Inv - WP1 * T1Inv;
	CVec3Dfp32 LocalAxisB = WP2 * T2Inv - WP1 * T2Inv;

	CVec3Dfp32 Dir1 = T1.GetRow(3) - Anchor;
	Dir1.Normalize();

	CVec3Dfp32 Dir2 = T2.GetRow(3) - Anchor;
	Dir2.Normalize();

	const CVec3Dfp32& AngleAxis = _WorldPos.GetRow(0);
	const CVec3Dfp32& Axis = _WorldPos.GetRow(2);

	fp32 RelativeAngle =  GetPlanarRotation(AngleAxis, Dir2, Axis);
	fp32 RelativeAnchorCenterOfMassAngle = GetPlanarRotation(-Dir1, Dir2, Axis);
	RelativeAngle -= RelativeAnchorCenterOfMassAngle;
	//RelativeAngle *= 0.5f;
//	RelativeAngle *= -1.0f;

//	CAxisRotfp32 AxisRot(Axis, -RelativeAnchorCenterOfMassAngle / (2.0f * 3.14159265f));
 	CAxisRotfp32 AxisRot(Axis, RelativeAngle / (2.0f * 3.14159265f));
//	CAxisRotfp32 AxisRot(Axis, -0.25f);
//	CAxisRotfp32 AxisRot(Axis, -0.28f);
//	CAxisRotfp32 AxisRot(Axis, -RelativeAnchorCenterOfMassAngle / (2.0f * 3.14159265f));
	CMat4Dfp32 AxisRotMatrix;
	AxisRot.CreateMatrix(AxisRotMatrix);
//	AxisRotMatrix.Unit();

	CVec3Dfp32 AngleAxisA = RA * AxisRotMatrix;
	CVec3Dfp32 AngleAxisB = RB;


	CWD_ConstraintDescriptor Constraint(ID);
	Constraint.SetType(CWD_ConstraintDescriptor::HINGE);
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::RA, CVec4Dfp32(RA[0], RA[1], RA[2], 1.0f));
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::RB, CVec4Dfp32(RB[0], RB[1], RB[2], 1.0f));
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::AXISA, CVec4Dfp32(LocalAxisA[0], LocalAxisA[1], LocalAxisA[2], 0.0f));
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::AXISB, CVec4Dfp32(LocalAxisB[0], LocalAxisB[1], LocalAxisB[2], 0.0f));
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::ANGLEAXISA, CVec4Dfp32(AngleAxisA[0], AngleAxisA[1], AngleAxisA[2], 1.0f));
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::ANGLEAXISB, CVec4Dfp32(AngleAxisB[0], AngleAxisB[1], AngleAxisB[2], 1.0f));
	Constraint.SetScalarParam(CWD_ConstraintDescriptor::RELATIVEANGLE, RelativeAngle);
	Constraint.SetScalarParam(CWD_ConstraintDescriptor::MAXANGLE, _MaxAngle);

	Constraint.SetConnectedObjects(pObj1->m_iObject, pObj2->m_iObject);
	Constraint.SetConnectedSubObjects(_iSub1, _iSub2);

	if (Phys_DoAddConstraint(pObj1, pObj2, Constraint))
	{
		return ID;
	}
	else
	{
		return -1;
	}

#else

	CWObject* pObj1 = CWorld_ServerCore::Object_Get(_iObject1);
	CWObject* pObj2 = CWorld_ServerCore::Object_Get(_iObject2);
	M_ASSERT(pObj1 && pObj1->m_pRigidBody, "");
	M_ASSERT(pObj2 && pObj2->m_pRigidBody, "");

	return m_DynamicsWorld.AddHingeConstraint(pObj1->m_pRigidBody, pObj2->m_pRigidBody, _WorldPos.GetRow(2), _WorldPos.GetRow(0), _AxisLength, _WorldPos.GetRow(3), _MaxAngle * _PI);
#endif

}


CWD_ConstraintDescriptor* CWorld_ServerCore::Phys_GetConstraint(int _iConstraint)
{
	TAP<CWD_ConstraintDescriptor> pConstraints = m_lConstraints;
	for (int i = 0; i < pConstraints.Len(); i++)					//TODO: check how much time is spent by this linear search 
	{
		if (pConstraints[i].GetID() == _iConstraint)
			return &pConstraints[i];
	}
	return NULL;
}


void CWorld_ServerCore::Phys_UpdateHingeJoint(int _iConstraint, int _iOldA, int _iOldB, int _iNewA, int _iNewB)
{
	CWD_ConstraintDescriptor& Constraint = *ThisClass::Phys_GetConstraint(_iConstraint);

	CMat4Dfp32 OldA = Phys_GetCenterOfMassCorrectedTransform(ThisClass::Object_Get(_iOldA));
	CMat4Dfp32 OldB = Phys_GetCenterOfMassCorrectedTransform(ThisClass::Object_Get(_iOldB));
	CMat4Dfp32 NewA = Phys_GetCenterOfMassCorrectedTransform(ThisClass::Object_Get(_iNewA));
	CMat4Dfp32 NewB = Phys_GetCenterOfMassCorrectedTransform(ThisClass::Object_Get(_iNewB));

	CVec3Dfp32 WorldRA = ((CVec3Dfp32 &) Constraint.GetVectorParam(CWD_ConstraintDescriptor::RA)) * OldA;
	CVec3Dfp32 WorldRB = ((CVec3Dfp32 &) Constraint.GetVectorParam(CWD_ConstraintDescriptor::RB)) * OldB;

	CMat4Dfp32 NewAInv, NewBInv;
	NewA.InverseOrthogonal(NewAInv);
	NewB.InverseOrthogonal(NewBInv);

	CVec3Dfp32 RA = WorldRA * NewAInv;
	CVec3Dfp32 RB = WorldRB * NewBInv;

	Constraint.SetVectorParam(CWD_ConstraintDescriptor::RA, CVec4Dfp32(RA[0], RA[1], RA[2], 1.0f));
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::RB, CVec4Dfp32(RB[0], RB[1], RB[2], 1.0f));

	CVec3Dfp32 Anchor = (WorldRA + WorldRB) * 0.5f;
	CVec3Dfp32 WorldAxisA, WorldAxisB;
	((CVec3Dfp32 &) Constraint.GetVectorParam(CWD_ConstraintDescriptor::AXISA)).MultiplyMatrix3x3(NewA, WorldAxisA);
	((CVec3Dfp32 &) Constraint.GetVectorParam(CWD_ConstraintDescriptor::AXISB)).MultiplyMatrix3x3(NewB, WorldAxisB);
	WorldAxisA.Normalize();
	WorldAxisB.Normalize();

	CVec3Dfp32 WorldAxis = (WorldAxisA + WorldAxisB) * 0.5f;
	WorldAxis.Normalize();

	CVec3Dfp32 DirAOld = OldA.GetRow(3) - Anchor;
	DirAOld.Normalize();
	CVec3Dfp32 DirBOld = OldB.GetRow(3) - Anchor;
	DirBOld.Normalize();

	CVec3Dfp32 DirANew = NewA.GetRow(3) - Anchor;
	DirANew.Normalize();
	CVec3Dfp32 DirBNew = NewB.GetRow(3) - Anchor;
	DirBNew.Normalize();

	fp32 RelativeAnchorCenterOfMassAngleOld = GetPlanarRotation(-DirAOld, DirBOld, WorldAxis);
	fp32 RelativeAnchorCenterOfMassAngleNew = GetPlanarRotation(-DirANew, DirBNew, WorldAxis);

	fp32 OldRelativeAngle = Constraint.GetScalarParam(CWD_ConstraintDescriptor::RELATIVEANGLE);
	Constraint.SetScalarParam(CWD_ConstraintDescriptor::RELATIVEANGLE, OldRelativeAngle - (RelativeAnchorCenterOfMassAngleOld - RelativeAnchorCenterOfMassAngleNew));
}

int CWorld_ServerCore::Phys_AddHingeJointConstraint2(uint16 _iObject1, uint16 _iObject2, const CMat4Dfp32& _WorldPos, fp32 _AxisLength, fp32 _MinAngle, fp32 _MaxAngle, uint16 _iSub1, uint16 _iSub2) 
{
#ifdef WSERVER_DYNAMICSENGINE2

	_AxisLength *= 2.0f;

	CWObject* pObj1 = CWorld_ServerCore::Object_Get(_iObject1);
	CWObject* pObj2 = CWorld_ServerCore::Object_Get(_iObject2);
	M_ASSERT(pObj1 && (pObj1->m_pRigidBody2 || pObj1->m_pPhysCluster), "");
	M_ASSERT(pObj2 && (pObj2->m_pRigidBody2 || pObj2->m_pPhysCluster), "");

	CMat4Dfp32 T1 = pObj1->m_pPhysCluster ? pObj1->m_pPhysCluster->m_lObjects[_iSub1].m_Transform
		: Phys_GetCenterOfMassCorrectedTransform(pObj1);
	CMat4Dfp32 T2 = pObj2->m_pPhysCluster ? pObj2->m_pPhysCluster->m_lObjects[_iSub2].m_Transform
		: Phys_GetCenterOfMassCorrectedTransform(pObj2);
	CMat4Dfp32 T1Inv, T2Inv;
	T1.InverseOrthogonal(T1Inv);
	T2.InverseOrthogonal(T2Inv);

	CVec3Dfp32 RA = _WorldPos.GetRow(3) * T1Inv;
	CVec3Dfp32 RB = _WorldPos.GetRow(3) * T2Inv;

	int ID = m_NextConstraintID++;

	CVec3Dfp32 WorldAxis = _WorldPos.GetRow(2) * _AxisLength;
	const CVec3Dfp32 &Anchor = _WorldPos.GetRow(3);

	CVec3Dfp32 WP1 = Anchor - WorldAxis * 0.5f;
	CVec3Dfp32 WP2 = Anchor + WorldAxis * 0.5f;

	CVec3Dfp32 LocalAxisA = WP2 * T1Inv - WP1 * T1Inv;
	CVec3Dfp32 LocalAxisB = WP2 * T2Inv - WP1 * T2Inv;

	CVec3Dfp32 Dir1 = T1.GetRow(3) - Anchor;
	Dir1.Normalize();

	CVec3Dfp32 Dir2 = T2.GetRow(3) - Anchor;
	Dir2.Normalize();

	const CVec3Dfp32& AngleAxis = _WorldPos.GetRow(0);
	const CVec3Dfp32& Axis = _WorldPos.GetRow(2);

	fp32 RelativeAngle =  GetPlanarRotation(AngleAxis, Dir2, Axis);
	fp32 RelativeAnchorCenterOfMassAngle = GetPlanarRotation(-Dir1, Dir2, Axis);
	//RelativeAngle -= RelativeAnchorCenterOfMassAngle;
	RelativeAngle = RelativeAnchorCenterOfMassAngle;

	
	//RelativeAngle *= 0.5f;
	//	RelativeAngle *= -1.0f;

	//	CAxisRotfp32 AxisRot(Axis, -RelativeAnchorCenterOfMassAngle / (2.0f * 3.14159265f));
	CAxisRotfp32 AxisRot(Axis, RelativeAngle / (2.0f * 3.14159265f));
	//	CAxisRotfp32 AxisRot(Axis, -0.25f);
	//	CAxisRotfp32 AxisRot(Axis, -0.28f);
	//	CAxisRotfp32 AxisRot(Axis, -RelativeAnchorCenterOfMassAngle / (2.0f * 3.14159265f));
	CMat4Dfp32 AxisRotMatrix;
	AxisRot.CreateMatrix(AxisRotMatrix);
	//	AxisRotMatrix.Unit();

//	CVec3Dfp32 AngleAxisA = RA * AxisRotMatrix;
//	CVec3Dfp32 AngleAxisB = RB;
	CVec3Dfp32 AngleAxisA = RA;
	CVec3Dfp32 AngleAxisB = RB;


	CWD_ConstraintDescriptor Constraint(ID);
	Constraint.SetType(CWD_ConstraintDescriptor::HINGE2);
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::RA, CVec4Dfp32(RA[0], RA[1], RA[2], 1.0f));
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::RB, CVec4Dfp32(RB[0], RB[1], RB[2], 1.0f));
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::AXISA, CVec4Dfp32(LocalAxisA[0], LocalAxisA[1], LocalAxisA[2], 0.0f));
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::AXISB, CVec4Dfp32(LocalAxisB[0], LocalAxisB[1], LocalAxisB[2], 0.0f));
	Constraint.SetScalarParam(CWD_ConstraintDescriptor::RELATIVEANGLE, RelativeAngle);
	Constraint.SetScalarParam(CWD_ConstraintDescriptor::MAXANGLE, _MaxAngle);
	Constraint.SetScalarParam(CWD_ConstraintDescriptor::MINANGLE, _MinAngle);

	Constraint.SetConnectedObjects(pObj1->m_iObject, pObj2->m_iObject);
	Constraint.SetConnectedSubObjects(_iSub1, _iSub2);

	if (Phys_DoAddConstraint(pObj1, pObj2, Constraint))
	{
		return ID;
	}
	else
	{
		return -1;
	}

#else
	M_ASSERT(false, "NOT SUPPORTED!");
	return -1;
#endif

}


void CWorld_ServerCore::Phys_UpdateBallConstraint(int _iConstraint, const CVec3Dfp32& _WorldPos)
{
#ifdef WSERVER_DYNAMICSENGINE2
	CWD_ConstraintDescriptor& Constraint = m_lConstraints[_iConstraint];
	M_ASSERT(Constraint.GetType() == CWD_ConstraintDescriptor::BALLWORLD, "!");

	Constraint.SetVectorParam(CWD_ConstraintDescriptor::WORLDREF, CVec4Dfp32(_WorldPos[0], _WorldPos[1], _WorldPos[2], 1.0f));
#else
	m_DynamicsWorld.UpdateBallConstraint(_iConstraint, _WorldPos.Getfp64());
#endif
}


void CWorld_ServerCore::Phys_UpdateAxisConstraint(int _iConstraint, const CMat4Dfp32& _WorldPos, fp32 _AxisLength)
{
#ifdef WSERVER_DYNAMICSENGINE2
	CVec3Dfp32 WorldRef = _WorldPos.GetRow(3);
	CVec3Dfp32 WorldAxis = _WorldPos.GetRow(2) * _AxisLength;

	CWD_ConstraintDescriptor& Constraint = *ThisClass::Phys_GetConstraint(_iConstraint);
	M_ASSERT(Constraint.GetType() == CWD_ConstraintDescriptor::HINGEWORLD ||
	         Constraint.GetType() == CWD_ConstraintDescriptor::HINGEWORLD2, "Invalid constraint");

	Constraint.SetVectorParam(CWD_ConstraintDescriptor::WORLDREF, CVec4Dfp32(WorldRef[0], WorldRef[1], WorldRef[2], 1.0f));
	Constraint.SetVectorParam(CWD_ConstraintDescriptor::WORLDAXIS, CVec4Dfp32(WorldAxis[0], WorldAxis[1], WorldAxis[2], 0.0f));

#else
	m_DynamicsWorld.UpdateAxisConstraint(_iConstraint, _WorldPos.Getfp64(), _AxisLength);
#endif
}

void CWorld_ServerCore::Phys_UpdateAxisConstraintAngles(int _iConstraint, fp32 _MinAngle, fp32 _MaxAngle)
{
#ifdef WSERVER_DYNAMICSENGINE2
	TAP_RCD<CWD_ConstraintDescriptor> pConstraints = m_lConstraints;
	for (int i = 0; i < pConstraints.Len(); i++)
	{
		if (pConstraints[i].GetID() == _iConstraint)
		{
			M_ASSERT(pConstraints[i].GetType() == CWD_ConstraintDescriptor::HINGEWORLD2, "Invalid constraint type!");

			pConstraints[i].SetScalarParam(CWD_ConstraintDescriptor::MAXANGLE, _MaxAngle);
			pConstraints[i].SetScalarParam(CWD_ConstraintDescriptor::MINANGLE, _MinAngle);
			return;
		}
	}

#endif
}

void CWorld_ServerCore::Phys_RemoveConstraint(int _iConstraint)
{
#ifdef WSERVER_DYNAMICSENGINE2

	int index = -1;
	TAP_RCD<CWD_ConstraintDescriptor> pConstraints = m_lConstraints;
	for (int i = 0; i < pConstraints.Len(); i++)
	{
		if (pConstraints[i].GetID() == _iConstraint)
		{
			index = i;
			break;
			//m_lConstraints.Del(i);
			//return;
		}
	}

	if (index == -1) 
	{
		ConOutL(CStrF("§cf80ERROR: (CWorld_ServerCore::Phys_RemoveConstraint) Constraint doesn't exists!"));
		return;
	}

	CWD_ConstraintDescriptor& Cons = pConstraints[index];

	uint32 iObjectA, iObjectB;
	Cons.GetConnectedObjects(iObjectA, iObjectB);

	CWObject* pObj1 = CWorld_ServerCore::Object_Get(iObjectA);
	if (pObj1)
	{
		for (int i = 0; i < pObj1->m_liConnectedToConstraint.Len(); i++)
		{
			if (pObj1->m_liConnectedToConstraint[i] == _iConstraint)
			{
				pObj1->m_liConnectedToConstraint.Del(i);
				break;
			}
		}
	}

	CWObject* pObj2 = CWorld_ServerCore::Object_Get(iObjectB);
	if (pObj2)
	{
		for (int i = 0; i < pObj2->m_liConnectedToConstraint.Len(); i++)
		{
			if (pObj2->m_liConnectedToConstraint[i] == _iConstraint)
			{
				pObj2->m_liConnectedToConstraint.Del(i);
				break;
			}
		}
	}

	m_lConstraints.Del(index);
	

#else
	m_DynamicsWorld.RemoveRigidConstraint(_iConstraint);

#endif
}


void CWorld_ServerCore::Phys_GetConnectedObjects(int _iConstraint, uint16* _piObject1, uint16* _piObject2)
{
#ifdef WSERVER_DYNAMICSENGINE2
	*_piObject1 = 0;
	*_piObject2 = 0;

	TAP_RCD<CWD_ConstraintDescriptor> pConstraints = m_lConstraints;
	for (int i = 0; i < pConstraints.Len(); i++)
	{
		if (pConstraints[i].GetID() == _iConstraint)
		{
			uint32 tmp1, tmp2;
			pConstraints[i].GetConnectedObjects(tmp1, tmp2);

			*_piObject1 = tmp1;
			*_piObject2 = tmp2;
			return;
		}
	}

#else
	CRigidBody *pRigidBody1, *pRigidBody2;
	m_DynamicsWorld.GetConnectedObjects(_iConstraint, &pRigidBody1, &pRigidBody2);

	*_piObject1 = 0;
	*_piObject2 = 0;

	if (pRigidBody1)
		*_piObject1 = ((CWD_RigidBody *) pRigidBody1)->m_pCoreData->m_iObject;

	if (pRigidBody2)
		*_piObject2 = ((CWD_RigidBody *) pRigidBody2)->m_pCoreData->m_iObject;

#endif
}

void CWorld_ServerCore::Phys_UpdateConnectedObject(int _iConstraint, uint16 _iOldObject, uint16 _iNewObject)
{
#ifdef WSERVER_DYNAMICSENGINE2
	CWObject* pObjOld = ThisClass::Object_Get(_iOldObject);
	CWObject* pObjNew = ThisClass::Object_Get(_iNewObject);
	M_ASSERTHANDLER(pObjOld && pObjOld->m_pRigidBody2, "old object not found!", return);

	TAP_RCD<CWD_ConstraintDescriptor> pConstraints = m_lConstraints;
	for (int i = 0; i < pConstraints.Len(); i++)
	{
		if (pConstraints[i].GetID() == _iConstraint)
		{
			CWD_ConstraintDescriptor& Constraint = pConstraints[i];

			uint32 A, B;
			Constraint.GetConnectedObjects(A, B);
			
			if (A == _iOldObject)
				Constraint.SetConnectedObjects(_iNewObject, B);
			else
				Constraint.SetConnectedObjects(A, _iNewObject);

			uint32 NewA, NewB;
			Constraint.GetConnectedObjects(NewA, NewB);

			if (Constraint.GetType() == CWD_ConstraintDescriptor::HINGE2)
				Phys_UpdateHingeJoint(_iConstraint, A, B, NewA, NewB);

			return;
		}
	}
	M_ASSERTHANDLER(false, "Constraint not found", return);

#else

	CWObject* pObjOld = CWorld_ServerCore::Object_Get(_iOldObject);
	CWObject* pObjNew = CWorld_ServerCore::Object_Get(_iNewObject);
	M_ASSERT(pObjOld && pObjOld->m_pRigidBody, "old object not found!");
	M_ASSERT(pObjNew && pObjNew->m_pRigidBody, "new object not found!");

	m_DynamicsWorld.UpdateConnectedObject(_iConstraint, pObjOld->m_pRigidBody, pObjNew->m_pRigidBody);

#endif
}


void CWorld_ServerCore::Phys_DoGetSystemMass(uint16 _iObject, fp32& _Sum, fp32& _Max, TDummySet2<uint32, 16>& _Set)
{
	if (!_Set.Add(_iObject))
		return;

	const CWObject* pObj = ThisClass::Object_Get(_iObject);
	if (!pObj)
		return;

	fp32 m = pObj->GetMass();
	_Sum += m;
	_Max = Max(m, _Max);

	TAP<const uint32> pConstraintIDs = pObj->m_liConnectedToConstraint;
	for (uint i = 0; i < pConstraintIDs.Len(); i++)
	{
		uint32 iObjA, iObjB;
		const CWD_ConstraintDescriptor& Constraint = *ThisClass::Phys_GetConstraint( pConstraintIDs[i] ); //pConstraints[ piConstraints[i] ];
		Constraint.GetConnectedObjects(iObjA, iObjB);

		if (iObjA && !_Set.Contains(iObjA))
			Phys_DoGetSystemMass(iObjA, _Sum, _Max, _Set);

		if (iObjB && !_Set.Contains(iObjB))
			Phys_DoGetSystemMass(iObjB, _Sum, _Max, _Set);
	}
}


void CWorld_ServerCore::Phys_GetPhysicalProperties(const CWO_PhysicsState& _PhysState, fp32& _Mass, fp32& _Volume)
{
	CWD_DynamicsUtil::SPhysicalProperties tmp;
	CWD_DynamicsUtil::GetPhysicalProperties(m_spMapData, _PhysState, tmp);
	_Mass = tmp.m_Mass;
	_Volume = tmp.m_Volume;
}


void CWorld_ServerCore::Phys_GetSystemMass(uint16 _iObject, fp32& _TotalMass, fp32& _MaxMass)
{
#ifdef WSERVER_DYNAMICSENGINE2
	TDummySet2<uint32, 16> Set;
	_TotalMass = 0.0f;
	_MaxMass = 0.0f;
	Phys_DoGetSystemMass(_iObject, _TotalMass, _MaxMass, Set);

#else
	CWObject* pObj = CWorld_ServerCore::Object_Get(_iObject);
	M_ASSERT(pObj && pObj->m_pRigidBody, "");
	
	return m_DynamicsWorld.GetSystemMass(pObj->m_pRigidBody);

#endif
}

