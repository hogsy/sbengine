#include "PCH.h"
#include "WPhysState.h"
#include "WPhysState_Hash.h"
#include "../Render/MRenderCapture.h"



// -------------------------------------------------------------------
//  Selection management
// -------------------------------------------------------------------
void CWorld_PhysState::Selection_AddAll(CSelection& _Selection)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_AddAll, MAUTOSTRIP_VOID);

	int nObj = Object_HeapSize();
	if (nObj > _Selection.GetCapacity())
		Error("Selection_AddAll", "Invalid selection capacity.");

	// This is hardly the most efficient solution, it is, however, highly unlikely that 
	// anybody in a hurry would use this selection method.
	int nElem = 0;
	int16* pSel = _Selection.GetData();
	for(int i = 0; i < nObj; i++)
	{
		CWObject_CoreData* pObj = Object_GetCD(i);
		if (pObj)
			pSel[nElem++] = i;
	}
	_Selection.SetNumElements(nElem);
}

void CWorld_PhysState::Selection_AddObject(CSelection& _Selection, int _iObject)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_AddObject, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_PhysState::Selection_AddObject);

	if (!Object_GetCD(_iObject)) return;

	int nElem = _Selection.GetNumElements();
	int16* pSel = _Selection.GetData();
	for(int i = 0; i < nElem; i++)
		if (pSel[i] == _iObject) return;

	if (_Selection.IsBufferFull())
		Error("Selection_AddObject", "Selection buffer full.");

	pSel[nElem] = _iObject;
	_Selection.IncreaseNumElements(1);
}

void CWorld_PhysState::Selection_AddObjectChildren(CSelection& _Selection, int _iObject, bool _bRecursive)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_AddObjectChildren, MAUTOSTRIP_VOID);
	Error("Selection_AddObjectChildren", "Not implemented.");
}

void CWorld_PhysState::Selection_AddObjects(CSelection& _Selection, int16* _piObjects, int _nObjects)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_AddObjects, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_PhysState::Selection_AddObjects);

#ifdef	SERVER_STATS
	CMTime Time; TStart(Time);
#endif	// SERVER_STATS

	int nElem = _Selection.GetNumElements();
	int16* pSel = _Selection.GetData();
	for(int iiObj = 0; iiObj < _nObjects; iiObj++)
	{
		int iObj = _piObjects[iiObj];
		for(int i = 0; i < nElem; i++)
			if (pSel[i] == iObj) continue;
		if (_Selection.IsBufferFull())
			Error("Selection_AddObjects", "Selection buffer full.");

		pSel[nElem] = iObj;
		_Selection.IncreaseNumElements(1);
	}

#ifdef	SERVER_STATS
	TStop(Time); m_TFunc_Selection += Time;  m_nFunc_Selection++;
#endif	// SERVER_STATS
}

void CWorld_PhysState::Selection_AddClass(CSelection& _Selection, int _iClass)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_AddClass, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_PhysState::Selection_AddClass);

	if (!_iClass) return;
	int bCheckObj = _Selection.GetNumElements();

#ifdef	SERVER_STATS
	CMTime Time; TStart(Time);
#endif	// SERVER_STATS

	int nObj = Object_HeapSize();
	for(int iObj = 0; iObj < nObj; iObj++)
	{
		const CWObject_CoreData* pObj = Object_GetCD(iObj);
		if (pObj && pObj->m_iClass == _iClass)
		{
			if (bCheckObj)
				Selection_AddObject(_Selection, iObj);
			else
				_Selection.AddData(iObj);
		}
	}

#ifdef	SERVER_STATS
	TStop(Time); m_TFunc_Selection += Time;  m_nFunc_Selection++;
#endif	// SERVER_STATS
}

void CWorld_PhysState::Selection_AddClass(CSelection& _Selection, const char* _pClassName)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_AddClass_2, MAUTOSTRIP_VOID);
	int iClass = m_spMapData->GetResourceIndex_Class(_pClassName);
	if (iClass > 0) Selection_AddClass(_Selection, iClass);
}


void CWorld_PhysState::Selection_AddOnFlagsSet(CSelection& _Selection, int _ObjectFlags)
{
	if (!_ObjectFlags) return;
	int bCheckObj = _Selection.GetNumElements();

#ifdef	SERVER_STATS
	CMTime Time; TStart(Time);
#endif	// SERVER_STATS

	int nObj = Object_HeapSize();
	for(int iObj = 0; iObj < nObj; iObj++)
	{
		const CWObject_CoreData* pObj = Object_GetCD(iObj);
		if (pObj && (pObj->m_PhysState.m_ObjectFlags & _ObjectFlags))
		{
			if (bCheckObj)
				Selection_AddObject(_Selection, iObj);
			else
				_Selection.AddData(iObj);
		}
	}

#ifdef	SERVER_STATS
	TStop(Time); m_TFunc_Selection += Time;  m_nFunc_Selection++;
#endif	// SERVER_STATS
}


void CWorld_PhysState::Selection_AddBoundSphere(CSelection& _Selection, int _ObjectFlags, const CVec3Dfp32& _Center, fp32 _Radius)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_AddBoundSphere, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_PhysState::Selection_AddBoundSphere);

	M_ASSERT(m_spSpaceEnum != NULL, "!");
#ifdef	SERVER_STATS
	CMTime Time; TStart(Time);
#endif	// SERVER_STATS

/*	int nObj = m_lspObjects.Len();
	for(int iObj = 0; iObj < nObj; iObj++)
	{
		CWObject* pObj = m_lspObjects[iObj];
		if (pObj && (pObj->m_iClass))
		{
			if ((pObj->GetPosition() - _Center).Length() <= _Radius)
				if (pObj->m_PhysState.m_Flags & _ObjectFlags)
					if (bCheckObj)
						Selection_AddObject(_iSelection, iObj);
					else
						m_lSelectionStack[_iSelection][m_lSelectNumElems[_iSelection]++] = iObj;
		}
	}*/

	int bCheckObj = _Selection.GetNumElements();

	CWO_EnumParams_Box Params;
	Params.m_ObjectFlags = _ObjectFlags;
	Params.m_ObjectIntersectFlags = 0;
	Params.m_ObjectNotifyFlags = 0;
	Params.m_Box.m_Min = CVec3Dfp32(_Center.k[0] - _Radius, _Center.k[1] - _Radius, _Center.k[2] - _Radius);
	Params.m_Box.m_Max = CVec3Dfp32(_Center.k[0] + _Radius, _Center.k[1] + _Radius, _Center.k[2] + _Radius);

	int iDisabled = m_iObject_DisabledLinkage;
	if(iDisabled)
		Object_EnableLinkage(iDisabled);

	int nEnum = m_spSpaceEnum->EnumerateBox(Params, m_lEnumSpace.GetBasePtr(), m_lEnumSpace.Len());

	if(iDisabled)
		Object_DisableLinkage(iDisabled);
	m_iObject_DisabledLinkage = iDisabled;

	if (bCheckObj)
		for(int i = 0; i < nEnum; i++)
			Selection_AddObject(_Selection, m_lEnumSpace[i]);
	else
	{
		for(int i = 0; i < nEnum; i++)
			_Selection.GetData()[i] = m_lEnumSpace.GetBasePtr()[i];
		_Selection.IncreaseNumElements(nEnum);
	}

#ifdef	SERVER_STATS
	TStop(Time); m_TFunc_Selection += Time;  m_nFunc_Selection++;
#endif	// SERVER_STATS
}

void CWorld_PhysState::Selection_AddBoundBox(CSelection& _Selection, int _ObjectFlags, const CVec3Dfp32& _BoxMin, const CVec3Dfp32& _BoxMax)
{
	MSCOPESHORT(CWorld_PhysState::Selection_AddBoundBox);
	MAUTOSTRIP(CWorld_PhysState_Selection_AddBoundBox, MAUTOSTRIP_VOID);
	M_ASSERT(m_spSpaceEnum != NULL, "!");
#ifdef	SERVER_STATS
	CMTime Time; TStart(Time);
#endif	// SERVER_STATS

	CWO_EnumParams_Box Params;
	Params.m_ObjectFlags = _ObjectFlags;
	Params.m_ObjectIntersectFlags = _ObjectFlags;
	Params.m_ObjectNotifyFlags = _ObjectFlags;
	Params.m_Box.m_Min = _BoxMin;
	Params.m_Box.m_Max = _BoxMax;
	
	int iDisabled = m_iObject_DisabledLinkage;
	if(iDisabled)
		Object_EnableLinkage(iDisabled);

	int bCheckObj = _Selection.GetNumElements();
	if (bCheckObj)
	{
		int nEnum = m_spSpaceEnum->EnumerateBox(Params, m_lEnumSpace.GetBasePtr(), m_lEnumSpace.Len());
		for(int i = 0; i < nEnum; i++)
			Selection_AddObject(_Selection, m_lEnumSpace[i]);
	}
	else
	{
		_Selection.SetNumElements(m_spSpaceEnum->EnumerateBox(Params, _Selection.GetData(), _Selection.GetCapacity() ));
	}

	if(iDisabled)
		Object_DisableLinkage(iDisabled);
	m_iObject_DisabledLinkage = iDisabled;

#ifdef	SERVER_STATS
	TStop(Time); m_TFunc_Selection += Time;  m_nFunc_Selection++;
#endif	// SERVER_STATS
}


void CWorld_PhysState::Selection_AddOriginInside(CSelection& _Selection, const CMat4Dfp32& _Pos, const CWO_PhysicsState& _PhysState)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_AddOriginInside, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_PhysState::Selection_AddOriginInside);
	// NOTE:	Will only use primitive 0 of _PhysState for inside-test.
	//			Only physics-model is supported.
	//			XW_MEDIUM_SOLID medium-flag set counts for inside.

	M_ASSERT(m_spSpaceEnum != NULL, "!");
//ConOutL("(CWorld_PhysState::Selection_AddOriginInside) 1");
	if (!_PhysState.m_nPrim) return;

	int bCheckObj = _Selection.GetNumElements();
	CWO_EnumParams_Box Params;
	Params.m_ObjectFlags = ~0; //OBJECT_FLAGS_PHYSMODEL
	Params.m_ObjectIntersectFlags = 0;
	Params.m_ObjectNotifyFlags = 0;	
	Phys_GetMinMaxBox(_PhysState, _Pos, Params.m_Box);

	int iDisabled = m_iObject_DisabledLinkage;
	if(iDisabled)
		Object_EnableLinkage(iDisabled);

	int nEnum = m_spSpaceEnum->EnumerateBox(Params, m_lEnumSpace.GetBasePtr(), m_lEnumSpace.Len());

	if(iDisabled)
		Object_DisableLinkage(iDisabled);
	m_iObject_DisabledLinkage = iDisabled;

//ConOutL(CStrF("(CWorld_PhysState::Selection_AddOriginInside) nEnum %d", nEnum));
	for(int iPrim = 0; iPrim < MinMT(1, _PhysState.m_nPrim); iPrim++)
	{
		const CWO_PhysicsPrim& PhysPrim = _PhysState.m_Prim[iPrim];

//ConOutL("(CWorld_PhysState::Selection_AddOriginInside) 2");
		if (_PhysState.m_Prim[iPrim].m_PrimType == OBJECT_PRIMTYPE_PHYSMODEL)
		{
//ConOutL("(CWorld_PhysState::Selection_AddOriginInside) 3");
			CXR_Model* pModel = m_spMapData->GetResource_Model(_PhysState.m_Prim[iPrim].m_iPhysModel);
//ConOutL("(CWorld_PhysState::Selection_AddOriginInside) 4");
			if (!pModel) continue;
//ConOutL("(CWorld_PhysState::Selection_AddOriginInside) 5");
			CXR_PhysicsModel* pPhysModel = pModel->Phys_GetInterface();
//ConOutL("(CWorld_PhysState::Selection_AddOriginInside) 6");
			if (!pPhysModel) continue;
			CXR_AnimState Anim;
			CXR_PhysicsContext PhysContext(_Pos, &Anim, m_spWireContainer);
			PhysContext.m_PhysGroupMaskThis = PhysPrim.m_PhysModelMask;
			pPhysModel->Phys_Init(&PhysContext);

			for(int i = 0; i < nEnum; i++)
			{
				int iObj = m_lEnumSpace[i];

//				if (iObj == _PhysState.m_iExclude) continue;
				const CWObject_CoreData* pObj = Object_GetCD(iObj);
				if (!pObj) continue;

				// Do they have any flags in common?
//				if (!(pObj->m_PhysState.m_ObjectIntersectFlags & _PhysState.m_ObjectFlags) &&
//					!(pObj->m_PhysState.m_ObjectFlags & _PhysState.m_ObjectIntersectFlags)) continue;

				// Sample medium
				CXR_MediumDesc TmpMedium;
				pPhysModel->Phys_GetMedium(&PhysContext, pObj->GetPosition(), TmpMedium);
				int m = TmpMedium.m_MediumFlags;
				if (m & XW_MEDIUM_SOLID)
				{
//				ConOutL(CStrF("    (CWorld_PhysState::Selection_AddOriginInside) Adding object %d", iObj));
					if (bCheckObj)
						Selection_AddObject(_Selection, iObj);
					else
						_Selection.AddData(iObj);
				}
			}
		}
	}

//ConOutL("(CWorld_PhysState::Selection_AddOriginInside) 7");
}

void CWorld_PhysState::Selection_AddIntersection(CSelection& _Selection, const CMat4Dfp32& _Pos, const CWO_PhysicsState& _PhysState)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_AddIntersection, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_PhysState::Selection_AddIntersection);
	if (!_PhysState.m_nPrim) return;

	// NOTE: The OBJECT_PHYSFLAGS_ROTATION flag of _PhysState.m_PhysFlags is ignored.
	// _PhysState is assumed to be rotated. (i.e, treated as if OBJECT_PHYSFLAGS_ROTATION were set)

	M_ASSERT(m_spSpaceEnum != NULL, "!");
#ifdef	SERVER_STATS
	CMTime Time; TStart(Time);
#endif	// SERVER_STATS

	int bCheckObj = _Selection.GetNumElements();

	CWO_EnumParams_Box Params;
	Params.m_ObjectFlags = _PhysState.m_ObjectIntersectFlags;
	Params.m_ObjectIntersectFlags = _PhysState.m_ObjectFlags;
	Params.m_ObjectNotifyFlags = 0;	

	Params.m_ObjectFlags = ~0;
	Params.m_ObjectIntersectFlags = ~0;
	Params.m_ObjectNotifyFlags = ~0;
	Phys_GetMinMaxBox(_PhysState, _Pos, Params.m_Box);

	int iDisabled = m_iObject_DisabledLinkage;
	if(iDisabled)
		Object_EnableLinkage(iDisabled);

	int nEnum = m_spSpaceEnum->EnumerateBox(Params, m_lEnumSpace.GetBasePtr(), m_lEnumSpace.Len());

	if(iDisabled)
		Object_DisableLinkage(iDisabled);
	m_iObject_DisabledLinkage = iDisabled;

	for(int i = 0; i < nEnum; i++)
	{
		int iObj = m_lEnumSpace[i];

		if (iObj == _PhysState.m_iExclude) continue;
		const CWObject_CoreData* pObj = Object_GetCD(iObj);
		if (!pObj) continue;

		const CWO_PhysicsState& ObjPhysState = pObj->GetPhysState();

		// Do they have any flags in common?
		if (!(ObjPhysState.m_ObjectIntersectFlags & _PhysState.m_ObjectFlags) &&
			!(ObjPhysState.m_ObjectFlags & _PhysState.m_ObjectIntersectFlags)) continue;

		// Intersect physics-states...
		int bIntersect = false;
		
		if (ObjPhysState.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION)
			bIntersect = Phys_IntersectStates(ObjPhysState, _PhysState, pObj->m_Pos, pObj->m_Pos, _Pos, _Pos, NULL, 0, 0);
		else
		{
			CMat4Dfp32 ObjPos;
			ObjPos.Unit();
			pObj->GetPosition().SetRow(ObjPos, 3);
			bIntersect = Phys_IntersectStates(ObjPhysState, _PhysState, ObjPos, ObjPos, _Pos, _Pos, NULL, 0, 0);
		}

		// Intersection?
		if (bIntersect)
		{
			if (bCheckObj)
				Selection_AddObject(_Selection, iObj);
			else
				_Selection.AddData(iObj);
		}
	}

#ifdef	SERVER_STATS
	TStop(Time); m_TFunc_Selection += Time;  m_nFunc_Selection++;
#endif	// SERVER_STATS
}
	

void CWorld_PhysState::Selection_AddIntersection(CSelection& _Selection, const CVec3Dfp32& _Pos, const CWO_PhysicsState& _PhysState)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_AddIntersection_2, MAUTOSTRIP_VOID);
	CMat4Dfp32 Mat; Mat.Unit();
	CVec3Dfp32::GetMatrixRow(Mat, 3) = _Pos;
	Selection_AddIntersection(_Selection, Mat, _PhysState);
}


void CWorld_PhysState::Selection_AddListeners(CSelection& _Selection, uint _iObject, uint16 _EventMask)
{
	MSCOPESHORT(CWorld_PhysState::Selection_AddListeners);

	int nElem = _Selection.GetNumElements();
	if (nElem)
	{
		// need to use m_lEnumSpace as temp buffer
		uint16* piRet = (uint16*)m_lEnumSpace.GetBasePtr();
		uint nRet = Min(m_lEnumSpace.Len(), _Selection.GetCapacity() - nElem);
		nRet = m_Listeners.EnumLinks(_iObject, 0, piRet, nRet, _EventMask);
		for (uint i = 0; i < nRet; i++)
			Selection_AddObject(_Selection, _iObject);
	}
	else
	{
		// can enumerate directly into selection buffer
		uint16* piRet = (uint16*)_Selection.GetData();
		uint nRet = m_Listeners.EnumLinks(_iObject, 0, piRet, _Selection.GetCapacity(), _EventMask);
		_Selection.IncreaseNumElements(nRet);
	}
}


void CWorld_PhysState::Object_NotifyListeners(uint _iObject, uint16 _EventMask)
{
	if (Object_HasListeners(_iObject))
	{
		TSelection<CSelection::SMALL_BUFFER> Sel;
		Selection_AddListeners(Sel, _iObject, _EventMask);
		CWObject_Message Msg(OBJSYSMSG_LISTENER_EVENT, _EventMask, 0, _iObject);
		Phys_Message_SendToSelection(Msg, Sel);
	}
}



// -------------------------------------------------------------------
void CWorld_PhysState::Selection_RemoveOnIndex(CSelection& _Selection, int _iObject)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_RemoveOnIndex, MAUTOSTRIP_VOID);
	int16* piSel = _Selection.GetData();
	int nSel = _Selection.GetNumElements();
	int nSelNew = 0;
	for(int i = 0; i < nSel; i++)
	{
		if (piSel[i] != _iObject)
			piSel[nSelNew++] = piSel[i];
	}

	_Selection.SetNumElements(nSelNew);
}


void CWorld_PhysState::Selection_RemoveOnFlagsSet(CSelection& _Selection, int _ObjectFlags)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_RemoveOnFlagsSet, MAUTOSTRIP_VOID);
	if (!_ObjectFlags) return;

	int16* piSel = _Selection.GetData();
	int nSel = _Selection.GetNumElements();
	int nSelNew = 0;
	for (int i = 0; i < nSel; i++)
	{
		const CWObject_CoreData* pObj = Object_GetCD(piSel[i]);
		if (!(pObj && (pObj->m_PhysState.m_ObjectFlags & _ObjectFlags)))
			piSel[nSelNew++] = piSel[i];
	}
	_Selection.SetNumElements(nSelNew);
}


void CWorld_PhysState::Selection_RemoveOnNotFlagsSet(CSelection& _Selection, int _ObjectFlags)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_RemoveOnNotFlagsSet, MAUTOSTRIP_VOID);

	int16* piSel = _Selection.GetData();
	int nSel = _Selection.GetNumElements();
	int nSelNew = 0;
	for (int i = 0; i < nSel; i++)
	{
		const CWObject_CoreData* pObj = Object_GetCD(piSel[i]);
		if (!(pObj && !(pObj->m_PhysState.m_ObjectFlags & _ObjectFlags)))
			piSel[nSelNew++] = piSel[i];
	}
	_Selection.SetNumElements(nSelNew);
}


void CWorld_PhysState::Selection_RemoveOnClass(CSelection& _Selection, int _iClass)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_RemoveOnClass, MAUTOSTRIP_VOID);
	int16* piSel = _Selection.GetData();
	int nSel = _Selection.GetNumElements();
	int nSelNew = 0;
	for(int i = 0; i < nSel; i++)
	{
		const CWObject_CoreData* pObj = Object_GetCD(piSel[i]);
		if (!(pObj && (pObj->m_iClass == _iClass)))
			piSel[nSelNew++] = piSel[i];
	}

	_Selection.SetNumElements(nSelNew);
}

void CWorld_PhysState::Selection_RemoveOnClass(CSelection& _Selection, const char* _pClassName)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_RemoveOnClass_2, MAUTOSTRIP_VOID);
	int iClass = m_spMapData->GetResourceIndex_Class(_pClassName);
	if (iClass > 0) Selection_RemoveOnClass(_Selection, iClass);
}

void CWorld_PhysState::Selection_RemoveOnNotClass(CSelection& _Selection, int _iClass)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_RemoveOnNotClass, MAUTOSTRIP_VOID);
	int16* piSel = _Selection.GetData();
	int nSel = _Selection.GetNumElements();
	int nSelNew = 0;
	for(int i = 0; i < nSel; i++)
	{
		const CWObject_CoreData* pObj = Object_GetCD(piSel[i]);
		if (!(pObj && (pObj->m_iClass != _iClass)))
			piSel[nSelNew++] = piSel[i];
	}

	_Selection.SetNumElements(nSelNew);
}

void CWorld_PhysState::Selection_RemoveOnNotClass(CSelection& _Selection, const char* _pClassName)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_RemoveOnNotClass_2, MAUTOSTRIP_VOID);
	int iClass = m_spMapData->GetResourceIndex_Class(_pClassName);
	if (iClass > 0) Selection_RemoveOnNotClass(_Selection, iClass);
}

void CWorld_PhysState::Selection_RemoveObjects(CSelection& _Selection, const int16* _piObjects, int _nObjects)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_RemoveObjects, MAUTOSTRIP_VOID);

#ifdef	SERVER_STATS
	CMTime Time; TStart(Time);
#endif	// SERVER_STATS

	for (int iiObj = 0; iiObj < _nObjects; iiObj++)
	{
		Selection_RemoveOnIndex(_Selection, _piObjects[iiObj]);
	}

#ifdef	SERVER_STATS
	TStop(Time); m_TFunc_Selection += Time;  m_nFunc_Selection++;
#endif	// SERVER_STATS
}

// -------------------------------------------------------------------
bool CWorld_PhysState::Selection_ContainsObject(const CSelection& _Selection, int _iObject)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_ContainsObject, false);
	const int16* piObj;
	int nObj = Selection_Get(_Selection, &piObj);
	for(int i = 0; i < nObj; i++)
		if (piObj[i] == _iObject) return true;
	return false;
}

bool CWorld_PhysState::Selection_IntersectLine(const CSelection& _Selection, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _ObjectFlags, int _MediumFlags, CCollisionInfo* _pCollisionInfo, int _iExclude)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_IntersectLine, false);
	const int16* piObj;
	int nObj = Selection_Get(_Selection, &piObj);
	bool bImpact = false;
	for(int i = 0; i < nObj; i++)
	{
		int iObj = piObj[i];
		if (iObj == _iExclude) continue;
		if (Object_IntersectLine(iObj, _p0, _p1, 0, _ObjectFlags, _MediumFlags, _pCollisionInfo))
		{
			if (!_pCollisionInfo) return true;
			bImpact = true;
		}
	}
	return bImpact;
}

int CWorld_PhysState::Selection_Get(const CSelection& _Selection, const int16** _ppRetList)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_Get, 0);

	if (_ppRetList) (*_ppRetList) = _Selection.GetData();
	return _Selection.GetNumElements();
}


void CWorld_PhysState::Selection_GetArray( CPotColSet *_pcs, const CSelection* _pSelection, const CWO_PhysicsState &_PhysState, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_GetArray, MAUTOSTRIP_VOID);
	
	Error("Selection_GetArray", "internal error while building PCS");
}



int CWorld_PhysState::Selection_GetSingleClass(const char* _pClass)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_GetSingleClass, 0);
	int iObj = -1;
	TSelection<CSelection::LARGE_BUFFER> Selection;
		Selection_AddClass(Selection, _pClass);
		const int16* pRet;
		int nRet =  Selection_Get(Selection, &pRet);
		if (nRet) iObj = pRet[Min(int(Random*nRet), nRet-1)];
	return iObj;
}


void CWorld_PhysState::Selection_Dump(const CSelection& _Selection, CStr _DumpName)
{
	MAUTOSTRIP(CWorld_PhysState_Selection_Dump, MAUTOSTRIP_VOID);
	const int16* piObj = 0;
	int nObj = Selection_Get(_Selection, &piObj);

	LogFile("-------------------------------------------------------------------");
	LogFile(CStrF("SELECTION DUMP (") + _DumpName + ")");
//	LogFile(CStrF("SELECTION DUMP (", _Selection) + _DumpName + ")");
	LogFile("-------------------------------------------------------------------");
	if (piObj)
		for(int i = 0; i < nObj; i++)
		{
			CWObject_CoreData* pObj = Object_GetCD(piObj[i]);
			if (pObj) LogFile(pObj->Dump(m_spMapData, 0));
		}
	LogFile("-------------------------------------------------------------------");
}

