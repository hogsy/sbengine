
#include "PCH.h"

#include "WClient_Core.h"
#include "../WPhysState_Hash.h"

int CWorld_ClientCore::IsClientObject(int _iObj)
{
	MAUTOSTRIP(CWorld_ClientCore_IsClientObject, 0);
	return _iObj >= m_ClientObjectBase;
};

int CWorld_ClientCore::Object_HeapSize()
{
	MAUTOSTRIP(CWorld_ClientCore_Object_HeapSize, 0);
	return m_lspObjects.Len();
}

CWObject_CoreData* CWorld_ClientCore::Object_GetCD(int _iObj)
{
	MAUTOSTRIP(CWorld_ClientCore_Object_GetCD, NULL);
	if(_iObj >= m_ClientObjectBase)
	{
		_iObj -= m_ClientObjectBase;
		if (_iObj < 0 || _iObj >= m_lspClientObjects.Len()) return NULL;
		CWObject_CoreData* pObj = m_lspClientObjects[_iObj];
		if (!pObj || !pObj->m_iClass || !pObj->m_iObject) return NULL;
		return pObj;
	}
	else
	{
		if (_iObj < 0 || _iObj >= m_lspObjects.Len()) return NULL;
		CWObject_Client* pObj = m_lspObjects[_iObj];
		if (!pObj) return NULL;
		pObj = pObj->GetActiveClientCopy();
		if (!pObj || !pObj->m_iClass || !pObj->m_iObject) return NULL;
		return pObj;
	}
}

CWObject_Client* CWorld_ClientCore::Object_Get(int _iObj)
{
	MAUTOSTRIP(CWorld_ClientCore_Object_Get, NULL);
	MSCOPESHORT(Object_Get);
	if(_iObj >= m_ClientObjectBase)
	{
		_iObj -= m_ClientObjectBase;
		if (_iObj < 0 || _iObj >= m_lspClientObjects.Len()) return NULL;
		CWObject_Client* pObj = m_lspClientObjects[_iObj];
		if (!pObj || !pObj->m_iClass || !pObj->m_iObject) return NULL;
		return pObj;
	}
	else
	{
		if (_iObj < 0 || _iObj >= m_lspObjects.Len()) return NULL;
		CWObject_Client* pObj = m_lspObjects[_iObj];
		if (!pObj) return NULL;
		pObj = pObj->GetActiveClientCopy();
		if (!pObj || !pObj->m_iClass || !pObj->m_iObject) return NULL;
		return pObj;
	}
};

CWObject_Client* CWorld_ClientCore::Object_GetFirstCopy(int _iObj)
{
	MAUTOSTRIP(CWorld_ClientCore_Object_Get, NULL);
	MSCOPESHORT(Object_Get);
	if(_iObj >= m_ClientObjectBase)
	{
		_iObj -= m_ClientObjectBase;
		if (_iObj < 0 || _iObj >= m_lspClientObjects.Len()) return NULL;
		CWObject_Client* pObj = m_lspClientObjects[_iObj];
		if (!pObj || !pObj->m_iClass || !pObj->m_iObject) return NULL;
		return pObj;
	}
	else
	{
		if (_iObj < 0 || _iObj >= m_lspObjects.Len()) return NULL;
		CWObject_Client* pObj = m_lspObjects[_iObj];
		if (!pObj || !pObj->m_iClass || !pObj->m_iObject) return NULL;
		return pObj;
	}
}

CWObject_Client* CWorld_ClientCore::Object_GetFirstClientType(int _iObj)
{
	if (_iObj < 0 || _iObj >= m_lspObjects.Len()) return NULL;
	CWObject_Client* pObj = m_lspObjects[_iObj];
	if (!pObj || !pObj->m_iClass || !pObj->m_iObject) return NULL;
	return pObj;
}

CWObject_ClientExecute* CWorld_ClientCore::ClientObject_Get(int _iObj)
{
	MAUTOSTRIP(CWorld_ClientCore_ClientObject_Get, NULL);
	if(_iObj >= m_ClientObjectBase)
	{
		_iObj -= m_ClientObjectBase;
		if (_iObj < 0 || _iObj >= m_lspClientObjects.Len()) return NULL;
		CWObject_ClientExecute* pObj = m_lspClientObjects[_iObj];
		if (!pObj || !pObj->m_iClass || !pObj->m_iObject) return NULL;
		return pObj;
	}
	else
		return NULL;
};

void CWorld_ClientCore::Object_SetActiveClientCopy(int _iObj, int _iActiveClientCopy)
{
	MAUTOSTRIP(CWorld_ClientCore_Object_SetActiveClientCopy, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_ClientCore::Object_SetActiveClientCopy); //AR-SCOPE

	if(_iObj >= m_ClientObjectBase)
	{
/*		_iObj -= m_ClientObjectBase;
		if (_iObj < 0 || _iObj >= m_lspClientObjects.Len()) return NULL;
		CWObject_Client* pObj = m_lspClientObjects[_iObj];
		if (!pObj || !pObj->m_iClass || !pObj->m_iObject) return NULL;
		return pObj;*/
		ConOut("§cf80WARNING: (CWorld_ClientCore::Object_SetActiveClientCopy) Is this the intent?");
	}
	else
	{
		if (_iObj < 0 || _iObj >= m_lspObjects.Len()) return;
		CWObject_Client* pObj = m_lspObjects[_iObj];
		if (pObj)
		{
			pObj->m_iActiveClientCopy = _iActiveClientCopy;
			Phys_InsertPosition(_iObj, Object_GetCD(_iObj));
		}
	}
}

// -------------------------------------------------------------------
int CWorld_ClientCore::ClientObject_Create(const char* _pClassName, const CMat4Dfp32& _Pos)
{
	MAUTOSTRIP(CWorld_ClientCore_ClientObject_Create, 0);
	if (m_ClientMode == WCLIENT_MODE_MIRROR) return 0;

	int iClass = m_spMapData->GetResourceIndex_Class(_pClassName);
	MRTC_CRuntimeClass_WObject* pRTC = m_spMapData->GetResource_Class(iClass);
	if (!pRTC) return -1;

	int iObj = m_spClientObjectHeap->AllocID();
	if (iObj < 0)
	{
//		ConOut("Out of client-objects.");
		return -1;
	}

	// "Create" object...
	m_lspClientObjects[iObj] = MNew(CWObject_ClientExecute);
	if (!m_lspClientObjects[iObj])
		MemError("ClientObject_Create");

	CWObject_ClientExecute* pObj = m_lspClientObjects[iObj];

	pObj->Clear();
	pObj->m_pRTC = pRTC;
	pObj->m_iClass = iClass;
	pObj->m_iObject = iObj + m_ClientObjectBase;
	pObj->m_LocalPos = _Pos;
	pObj->m_Pos = _Pos;
	pObj->m_LastPos = _Pos;
	Object_Link(pObj->m_iObject);
	M_TRY
	{
		// Run OnInitClientObjects()
		pRTC->m_pfnOnInitClientObjects(pObj, this);
		pObj = m_lspClientObjects[iObj];
		if (!pObj)
			return -1;

		// Run OnClientCreate()
		pRTC->m_pfnOnClientCreate(pObj, this);
		pObj = m_lspClientObjects[iObj];
		if (!pObj)
			return -1;
	}
	M_CATCH(
	catch(CCException)
	{
		// Create failed, free object.
		Object_Unlink(pObj->m_iObject);
		m_lspClientObjects[iObj] = NULL;
		m_spClientObjectHeap->FreeID(iObj);
		return -1;
	}
	)
	return pObj->m_iObject;
}

int CWorld_ClientCore::ClientObject_Create(const char* _pClassName, const CVec3Dfp32& _Pos)
{
	MAUTOSTRIP(CWorld_ClientCore_ClientObject_Create_2, 0);
	if (m_ClientMode == WCLIENT_MODE_MIRROR) return 0;

	CMat4Dfp32 Pos; Pos.Unit();
	CVec3Dfp32::GetMatrixRow(Pos, 3) = _Pos;
	return ClientObject_Create(_pClassName, Pos);
}

void CWorld_ClientCore::Object_Destroy(int _iObj)
{
	MAUTOSTRIP(CWorld_ClientCore_Object_Destroy, MAUTOSTRIP_VOID);
	CWObject_Client* pObj = Object_Get(_iObj);
	if (!pObj) return;

	if (IsClientObject(_iObj))
	{
		m_spClientObjectHeap->FreeID(_iObj - m_ClientObjectBase);
	}

	Sound_Kill(_iObj);
	Object_Unlink(_iObj);
	pObj->Clear();
	Sound_UpdateObject(_iObj);

	// Free object
	if (IsClientObject(_iObj))
		m_lspClientObjects[_iObj - m_ClientObjectBase] = NULL;
	else
		m_lspObjects[_iObj] = NULL;
}

void CWorld_ClientCore::Object_Link(int _iObj)
{
	MAUTOSTRIP(CWorld_ClientCore_Object_Link, MAUTOSTRIP_VOID);
	MSCOPESHORT(Object_Link);
	if (!m_pSceneGraph) return;
	CWObject_Client* pObj = Object_Get(_iObj);
	if (!pObj) return;

	Phys_InsertPosition(_iObj, pObj);

	if (!(m_ClientMode & WCLIENT_MODE_MIRROR))
	{
		if (pObj->m_ClientFlags & CWO_CLIENTFLAGS_VISIBILITY)
			m_iObjVisPool.Insert(_iObj);
		else
			m_iObjVisPool.Remove(_iObj);

		// Sound Volumes
		if (pObj->m_PhysState.m_ObjectFlags & OBJECT_FLAGS_SOUND)
			m_iObjSoundPool.Insert(_iObj);
		else
			m_iObjSoundPool.Remove(_iObj);
	}

/*	if (pObj->m_ClientFlags & CWO_CLIENTFLAGS_LINKINFINITE)
	{
		m_pSceneGraph->SceneGraph_LinkInfiniteElement(m_hSceneGraph, _iObj);
	}
	else
	{
		CBox3Dfp32 Box;
		Box.m_Min = CVec3Dfp32::GetMatrixRow(pObj->m_Pos, 3);
		Box.m_Max = CVec3Dfp32::GetMatrixRow(pObj->m_Pos, 3);
		Box.m_Min -= 8.0f;
		Box.m_Max += 8.0f;
		m_pSceneGraph->SceneGraph_LinkElement(m_hSceneGraph, _iObj, Box);
	}*/
}

void CWorld_ClientCore::Object_Unlink(int _iObj)
{
	MAUTOSTRIP(CWorld_ClientCore_Object_Unlink, MAUTOSTRIP_VOID);
//	if (IsClientObject(_iObj)) return;
	if (!(m_ClientMode & WCLIENT_MODE_MIRROR))
	{
		m_iObjVisPool.Remove(_iObj);
		if (m_spSpaceEnum) m_spSpaceEnum->Remove(_iObj);

		// Sound Volumes
		m_iObjSoundPool.Remove(_iObj);
	}
	if (m_spSceneGraphInstance != NULL)
		m_spSceneGraphInstance->SceneGraph_UnlinkElement(_iObj);
}

void CWorld_ClientCore::Object_ForcePosition(int _iObj, const CMat4Dfp32& _Pos)
{
	MAUTOSTRIP(CWorld_ClientCore_Object_ForcePosition, MAUTOSTRIP_VOID);
	CWObject_Client* pObj = Object_Get(_iObj);
	if (!pObj) return;

	pObj->m_LocalPos = _Pos;
	Object_Link(_iObj);
}

void CWorld_ClientCore::Object_ForcePosition(int _iObj, const CVec3Dfp32& _Pos)
{
	MAUTOSTRIP(CWorld_ClientCore_Object_ForcePosition_2, MAUTOSTRIP_VOID);
	CWObject_Client* pObj = Object_Get(_iObj);
	if (!pObj) return;

	CVec3Dfp32::GetMatrixRow(pObj->m_LocalPos, 3) = _Pos;
	Object_Link(_iObj);
}

void CWorld_ClientCore::Object_ForcePosition_World(int _iObj, const CMat4Dfp32& _Pos)
{
	MAUTOSTRIP(CWorld_ClientCore_Object_ForcePosition_World, MAUTOSTRIP_VOID);
	CWObject_Client* pObj = Object_Get(_iObj);
	if (!pObj) return;

	if (pObj->GetParent())
	{
		CWObject_CoreData* pParent = Object_GetCD(pObj->GetParent());
		if (!pParent)
			return;
		
		CMat4Dfp32 Local;
		CMat4Dfp32 ParentInv;
		pParent->GetPositionMatrix().InverseOrthogonal(ParentInv);
		_Pos.Multiply(ParentInv, Local);
		
		pObj->m_LocalPos = Local;
	}
	else
	{
		CVec3Dfp32 p = CVec3Dfp32::GetMatrixRow(_Pos, 3);
		pObj->m_LocalPos = _Pos;
	}
	Object_Link(_iObj);
}

void CWorld_ClientCore::Object_SetBox(int _iObj, const CVec3Dfp32& _Min, const CVec3Dfp32& _Max)
{
	MAUTOSTRIP(CWorld_ClientCore_Object_SetBox, MAUTOSTRIP_VOID);
	Error("Object_SetBox", "Not implemented.");
}

int CWorld_ClientCore::Object_SetDirty(int _iObj, int _Mask)
{
	return 0;
}

bool CWorld_ClientCore::Object_Move(int _iObj)
{
	MAUTOSTRIP(CWorld_ClientCore_Object_Move, false);
	CWObject_ClientExecute* pObj = ClientObject_Get(_iObj);
	if (!pObj) return false;

	CMat4Dfp32 NewPos, VelMat;
	NewPos.Unit();
	pObj->GetVelocityMatrix(VelMat);
	VelMat.Multiply3x3(pObj->m_LocalPos, NewPos);
	CVec3Dfp32::GetMatrixRow(NewPos, 3) = 
		CVec3Dfp32::GetMatrixRow(pObj->m_LocalPos, 3) + pObj->GetMoveVelocity();
	Object_ForcePosition(_iObj, NewPos);
	return true;
}

