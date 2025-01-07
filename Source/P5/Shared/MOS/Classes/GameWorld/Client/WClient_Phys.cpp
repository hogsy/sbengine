
#include "PCH.h"

#include "WClient_Core.h"
#include "../WPhysState_Hash.h"
#include "../../../XR/Phys/WPhysPCS.h"



bool CWorld_ClientCore::Phys_IntersectWorld(CPotColSet *_pcs, const CWO_PhysicsState& _PhysState, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, int _iExcludeObj, 
		CCollisionInfo* _pCollInfo, int _NotifyFlags, CSelection* _pNotifySelection1 , CSelection* _pNotifySelecion2)
{
	MAUTOSTRIP(CWorld_ClientCore_Phys_IntersectWorld_PCS, false);
	MSCOPESHORT(CWorld_ClientCore::Phys_IntersectWorld_PCS);

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

	int nPrim = _PhysState.m_nPrim;
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
				// ingen rotations hastighet, innebär att vädiga konstiga saker kan uppstå om roteringen inte integreras
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

		switch (PhysPrim.m_PrimType)
		{
		case OBJECT_PRIMTYPE_NONE:
			{
				MSCOPESHORT(PCS_NotHandled_NONE);
				// wierd - why should there be any primitives marked OBJECT_PRIMTYPE_NONE??
				continue;
			}

		case OBJECT_PRIMTYPE_PHYSMODEL:
			{
				MSCOPESHORT(PCS_NotHandled_PHYSMODEL);
				// add code to intersect PCS with physmodel
				// physmodels themselfes have to be responsible/implementing for colliding with sets!
				
				// PhysPrim->CollideClosestPCS( _pcs );				
				continue;
			}

		case OBJECT_PRIMTYPE_SPHERE:
			{	// intersect sphere with PCS
				nCollisions = _pcs->CollideSphere( WPrimOrigin.GetRow(3).k, WPrimDest.GetRow(3).k, PhysPrim.GetRadius() + 0.0001f, lCInfo, e_MaxCInfo);
				break;
			}

		case OBJECT_PRIMTYPE_BOX:
			{	// intersect box with PCS

				CBox3Dfp32 BoxOfDoom; // set the box correct damnit!
				BoxOfDoom.m_Max = PhysPrim.GetDim();
				BoxOfDoom.m_Min = -BoxOfDoom.m_Max;

				nCollisions = _pcs->CollideBox(&WPrimOrigin, &WPrimDest, BoxOfDoom.m_Min.k, lCInfo, e_MaxCInfo);
				break;
			}

		case OBJECT_PRIMTYPE_POINT:
			{	// intersect point with PCS, måste göra med WOrigin -> WDest linjen!
				nCollisions = _pcs->CollidePoint( CVec3Dfp32::GetMatrixRow(WOrigin, 3).k, CVec3Dfp32::GetMatrixRow(WDest, 3).k, lCInfo, e_MaxCInfo);
				break;
			}

		default:
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

		// uppdatera _pCollInfo om ny kollision är "bättre".
		// kanske borde trycka in detta i Collide<primitive> funktionerna istället!
		
		for (int iColl = 0; iColl < nCollisions; iColl++)
		{
			const CCollisionInfo& CInfo = lCInfo[iColl];
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
	}


	// Finalize collision info
	if (_pCollInfo && _pCollInfo->m_bIsValid && (_pCollInfo->m_ReturnValues & CXR_COLLISIONRETURNVALUE_SURFACEVELOCITY))
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



bool CWorld_ClientCore::Phys_IntersectWorld(const CSelection* _pSelection, const CWO_PhysicsState& _PhysState, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, int _iExcludeObj, 
		CCollisionInfo* _pCollisionInfo, int _NotifyFlags, CSelection* _pNotifySelection1 , CSelection* _pNotifySelecion2)
{
	MAUTOSTRIP(CWorld_ClientCore_Phys_IntersectWorld_NoPCS, false);
	MSCOPESHORT(CWorld_ClientCore::Phys_IntersectWorld_NoPCS);

	// Returns false if _PhysState can be placed at _Pos
	// If _pCollisionInfo != NULL the closest point of intersection will be return in *_pCollisionInfo

	if (!_PhysState.m_nPrim) return false;

	M_ASSERT(m_spSpaceEnum != NULL, "!");
#ifdef	SERVER_STATS
CMTime Time; TStart(Time);
#endif


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

	const int16* piEnum = NULL;
	int nEnum = 0;

	if (_pSelection != NULL)
	{
		nEnum = ThisClass::Selection_Get(*_pSelection, &piEnum);
	}
	else
	{
		CWO_EnumParams_Box Params;
		Params.m_ObjectFlags = _PhysState.m_ObjectIntersectFlags;
		Params.m_ObjectIntersectFlags = _PhysState.m_ObjectFlags;
		Params.m_ObjectNotifyFlags = 0;

		CBox3Dfp32 BoxSrc;
		Phys_GetMinMaxBox(_PhysState, *pOrigin, BoxSrc);
		Phys_GetMinMaxBox(_PhysState, *pDest, Params.m_Box);
		Params.m_Box.Expand(BoxSrc);
		nEnum = m_spSpaceEnum->EnumerateBox(Params, m_lEnumSpace.GetBasePtr(), m_lEnumSpace.Len());
		piEnum = m_lEnumSpace.GetBasePtr();
	}

	CCollisionInfo CInfo;
//	int iCInfoObj = -1;

//	int bCheckAll = (_pCollisionInfo != NULL);

	CMat4Dfp32 ObjOrigin;
	ObjOrigin.Unit();

	for(int i = 0; i < nEnum; i++)
	{
		int iObj = piEnum[i];

		if( iObj == _iExcludeObj )
			continue;

		if (iObj == _PhysState.m_iExclude)
			continue;
		
		CWObject_CoreData* pObj = ThisClass::Object_GetCD(iObj);
		CWObject_Client* pObjClient = ThisClass::Object_Get(iObj);
		
		if (!pObj)
			continue;
			
		CWO_PhysicsState* pPhys = &pObj->m_PhysState;
		
		// Fill phys state with modelinstance data
		// NOTE: This is handled from CWObject_Client and shouldn't be a problem for us...
		/*
		if(pObjClient)
		{
			for(int i = 0; i < CWO_NUMMODELINDICES; i++)
				pPhys->m_lModelInstances[i] = pObjClient->m_lModelInstances[i];
		}
		else
		{
			for(int i = 0; i < CWO_NUMMODELINDICES; i++)
				pPhys->m_lModelInstrances[i] = NULL;
		}
		*/

		if (pPhys->m_iExclude == _iExcludeObj)
			continue;

		// Do they have any flags in common?
		if(!((pPhys->m_ObjectIntersectFlags & _PhysState.m_ObjectFlags) ||
			 (pPhys->m_ObjectFlags & _PhysState.m_ObjectIntersectFlags)))
			continue;

#ifdef	SERVER_STATS
CMTime Time2; TStart(Time2);
#endif
		// Intersect physics-states...

		int bIntersect = false;

		// Determine intersection order, so that objects treat eachother identically, regardless of the
		// object-order in the function-call. (Phys_IntersectWorld(obj1, obj2) or Phys_IntersectWorld(obj2, obj1) ?)

		bool bOrder = (iObj > _iExcludeObj);

		const CMat4Dfp32* pObjOrigin = &pObj->GetPositionMatrix();
		if (!(pObj->m_PhysState.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION))
		{
			CVec3Dfp32::GetMatrixRow(ObjOrigin, 3) = pObj->GetPosition();
			pObjOrigin = &ObjOrigin;
		}

		CInfo.Clear();
		if (bOrder)
			bIntersect = Phys_IntersectStates(*pPhys, _PhysState, *pObjOrigin, *pObjOrigin, *pOrigin, *pDest, (_pCollisionInfo) ? &CInfo : NULL, 0, 0);
		else
		{
			bIntersect = Phys_IntersectStates(_PhysState, *pPhys, *pOrigin, *pDest, *pObjOrigin, *pObjOrigin, (_pCollisionInfo) ? &CInfo : NULL, 0, 0);
			if (bIntersect) CInfo.m_Plane.n = -CInfo.m_Plane.n;
		}

		if (bIntersect)
		{
			if (_pCollisionInfo) 
			{
				CInfo.m_iObject = iObj;
				_pCollisionInfo->Improve(CInfo);
				if (_pCollisionInfo->IsComplete())
					return true;	
			}
			else
				return true;
		}
	}


	// No collision info?, just return true/false.
	if (!_pCollisionInfo)
		return false;

	// Finalize collision info
	if (_pCollisionInfo->m_bIsValid && (_pCollisionInfo->m_ReturnValues & CXR_COLLISIONRETURNVALUE_SURFACEVELOCITY))
	{
		CWObject_CoreData* pCObj = Object_GetCD(_pCollisionInfo->m_iObject);
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

	return _pCollisionInfo->m_bIsCollision != 0;
}


bool CWorld_ClientCore::Phys_SetPosition(const CSelection* _pSelection, int _iObj, const CMat4Dfp32& _Pos, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWorld_ClientCore_Phys_SetPosition, false);
	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (!pObj) Error("Phys_SetPosition", "NULL Object.");

	bool bIntersect = false;
	{
		if (pObj->m_iObjectParent)
		{
			const CWObject_CoreData* pParent = Object_GetCD(pObj->m_iObjectParent);
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
				bIntersect = Phys_IntersectWorld(_pSelection, pObj->m_PhysState, WPos, WPos, _iObj, _pCollisionInfo, 0, NULL, NULL);

				if (_pCollisionInfo && _pCollisionInfo->m_bIsValid)
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
				bIntersect = Phys_IntersectWorld(_pSelection, pObj->m_PhysState, _Pos, _Pos, _iObj, _pCollisionInfo, 0, NULL, NULL);
		}
		else
			bIntersect = Phys_IntersectWorld(_pSelection, pObj->m_PhysState, _Pos, _Pos, _iObj, _pCollisionInfo, 0, NULL, NULL);

		if (!bIntersect)
		{
			pObj->m_LocalPos = _Pos;
			Phys_InsertPosition(_iObj, pObj);
		}
	}
	return (!bIntersect);
}



void CWorld_ClientCore::GetMovementBounds( float *_BoxMinMax, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest )
{
	// WOrigin and WDest -> defines matrices
	
	CMat4Dfp32 WOrigin = _Origin;
	CMat4Dfp32 WDest = _Dest;

	CWObject_CoreData* pObj = ThisClass::Object_GetCD(_iObj);

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




void CWorld_ClientCore::Selection_GetArray( CPotColSet *_pcs, const CSelection* _pSelection, const CWO_PhysicsState &_PhysState, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest)
{
	MAUTOSTRIP(CWorld_ServerCore_Selection_GetArray, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_ClientCore::Selection_GetArray);
	
	if (!_PhysState.m_nPrim) return;

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


		if (_pSelection != 0)
		{ // A valid selection was provided in input
			nEnum = Selection_Get(*_pSelection, &piEnum);
		}
		else
		{ // Have to enumerate a new selection		
			CWO_EnumParams_Box Params;
			Params.m_ObjectFlags = _PhysState.m_ObjectIntersectFlags;
			Params.m_ObjectIntersectFlags = _PhysState.m_ObjectFlags;
			Params.m_ObjectNotifyFlags = 0;

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

		int bDoIntersect = 
			(pPhys->m_ObjectIntersectFlags & _PhysState.m_ObjectFlags) ||
			(pPhys->m_ObjectFlags & _PhysState.m_ObjectIntersectFlags);

		if ( !bDoIntersect )
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
					pPhysModel->CollectPCS(&PhysContext, PCS_DO_INTERSECT, _pcs, iObj, _PhysState.m_MediumFlags);
					break;
				}

			case OBJECT_PRIMTYPE_SPHERE :
				{
					// radien + 16.f ???? 
					_pcs->AddSphere( PCS_DO_INTERSECT, iObj, WPrimOrigin.GetRow(3).k, PhysPrim.GetRadius() + 0.0001f );
					break;
				}

			case OBJECT_PRIMTYPE_BOX :
				{
					// MUPPJOKKO - IMPLEMENT ME! - tillåta roterade boxar?
				
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

					_pcs->AddBox( PCS_DO_INTERSECT, iObj, fBoxMinMax );
					break;
				}

			case OBJECT_PRIMTYPE_POINT :
				{
					_pcs->AddPoint( PCS_DO_INTERSECT, iObj, CVec3Dfp32::GetMatrixRow(WPrimOrigin, 3).k ); 
					break;
				}

			default :
				Error("Phys_", CStrF("Invalid primitive type. (%d)", PhysPrim.m_PrimType ));
			}
			continue;
		}
			
	}
}




bool CWorld_ClientCore::Phys_MovePosition(CPotColSet *_pcs, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWorld_ClientCore_Phys_MovePosition_PCS, false);
	MSCOPESHORT(CWorld_ClientCore::Phys_MovePosition_PCS);

	CWObject_CoreData* pObj = ThisClass::Object_GetCD(_iObj);

	M_ASSERT( pObj, "Phys_MovePosition, NULL Object" );
	

	bool bIntersect = false;
	{
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
				bIntersect = Phys_IntersectWorld( _pcs, pObj->m_PhysState, WOrigin, WDest, _iObj, _pCollisionInfo, 0, NULL, NULL);

				if (_pCollisionInfo && _pCollisionInfo->m_bIsValid)
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
				bIntersect = Phys_IntersectWorld( _pcs, pObj->m_PhysState, _Origin, _Dest, _iObj, _pCollisionInfo, 0, NULL, NULL);
		}
		else
			bIntersect = Phys_IntersectWorld( _pcs, pObj->m_PhysState, _Origin, _Dest, _iObj, _pCollisionInfo, 0, NULL, NULL);

		if (!bIntersect)
		{
			pObj->m_LocalPos = _Dest;
			Phys_InsertPosition(_iObj, pObj);
		}
	}

	return (!bIntersect);
}


bool CWorld_ClientCore::Phys_MovePosition(const CSelection* _pSelection, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWorld_ClientCore_Phys_MovePosition_NoPCS, false);
	MSCOPESHORT(CWorld_ClientCore::Phys_MovePosition_NoPCS);

	CWObject_CoreData* pObj = ThisClass::Object_GetCD(_iObj);
	if (!pObj) Error("Phys_MovePosition", "NULL Object.");

	bool bIntersect = false;
	{
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
				bIntersect = Phys_IntersectWorld(_pSelection, pObj->m_PhysState, WOrigin, WDest, _iObj, _pCollisionInfo, 0, NULL, NULL);

				if (_pCollisionInfo && _pCollisionInfo->m_bIsValid)
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
				bIntersect = Phys_IntersectWorld(_pSelection, pObj->m_PhysState, _Origin, _Dest, _iObj, _pCollisionInfo, 0, NULL, NULL);
		}
		else
			bIntersect = Phys_IntersectWorld(_pSelection, pObj->m_PhysState, _Origin, _Dest, _iObj, _pCollisionInfo, 0, NULL, NULL);

		if (!bIntersect)
		{
			pObj->m_LocalPos = _Dest;
			Phys_InsertPosition(_iObj, pObj);
		}
	}

// PHYSLOG:
/*if (_pCollisionInfo)
	ConOutL(CStrF("Phys_MovePosition %d, Valid %d, Distance %f, Plane ", bIntersect, _pCollisionInfo->m_bIsValid, _pCollisionInfo->m_Distance) + _pCollisionInfo->m_Plane.GetString() + pObj->GetVelocity().m_Move.GetString());
else
	ConOutL(CStrF("Phys_MovePosition %d", bIntersect));*/

	return (!bIntersect);
}

aint CWorld_ClientCore::Phys_Message_SendToObject(const CWObject_Message& _Msg, int _iObj)
{
	MAUTOSTRIP(CWorld_ClientCore_Phys_Message_SendToObject, 0);
	return ThisClass::ClientMessage_SendToObject(_Msg, _iObj);
}

void CWorld_ClientCore::Phys_Message_SendToSelection(const CWObject_Message& _Msg, const CSelection& _Sel)
{
	int nElem = _Sel.GetNumElements();
	const int16* pSel = _Sel.GetData();
	for (int i = 0; i < nElem; i++)
		ThisClass::ClientMessage_SendToObject(_Msg, pSel[i]);
}


bool CWorld_ClientCore::Phys_MessageQueue_SendToObject(const CWObject_Message& _Msg, int _iObj)
{
	MAUTOSTRIP(CWorld_ClientCore_Phys_MessageQueue_SendToObject, false);
	return false;
}

void CWorld_ClientCore::Phys_MessageQueue_Flush()
{
	MAUTOSTRIP(CWorld_ClientCore_Phys_MessageQueue_Flush, MAUTOSTRIP_VOID);
}

