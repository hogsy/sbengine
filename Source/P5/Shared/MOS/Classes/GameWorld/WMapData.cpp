#include "PCH.h"
#include "WMapData.h"
#include "../../XR/XRBlockNavGrid.h"

#include "WDataRes_Core.h"
#include "WDataRes_MiscMedia.h"
#include "WDataRes_Anim.h"
#include "WDataRes_Models.h"
#include "WDataRes_XW.h"
#include "WDataRes_Sound.h"
#include "WDataRes_FacialSetup.h"
#include "WObjCore.h"

#include "Server/WServer.h"

extern void StrFixFilename(const char* _pStr, char* _pDest, uint _nMax);

//#pragma optimize("",off)

//#define DebugLog(s) LogFile(s)
#define DebugLog(s)


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMapData::CIndexMap
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CMapData::CIndexMap::Create(uint _nMaxIDs)
{
	CMap32::Create(_nMaxIDs, 8);
	CMap32::Insert(0, 0);							// Reserve 0 for null-resources.
	m_nUsed = 1;
}

uint CMapData::CIndexMap::GetNewID()
{
	if (m_nUsed >= GetMaxIDs())
		Resize(Max(512, GetMaxIDs()*2));
	return m_nUsed++;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMapData
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_DYNAMIC(CMapData, CReferenceCount);

void CMapData::Create(spCWorldData _spWData)
{
	MAUTOSTRIP(CMapData_Create, MAUTOSTRIP_VOID);
	m_spWData = _spWData;
	m_lspResources.Clear();
	m_lspResources.SetGrow(1024);
	m_lspResources.Add(spCWResource(NULL));			// Reserve 0 for null-resources.
	m_IndexMap.Create(512);
}

CMapData::CMapData()
{
	MAUTOSTRIP(CMapData_ctor, MAUTOSTRIP_VOID);
//	m_Hash.Create(0, false);
	m_State = 0;
	m_pWServer = NULL;
}

CMapData::~CMapData()
{
	MAUTOSTRIP(CMapData_dtor, MAUTOSTRIP_VOID);
//	Dump(-1);
}


// -------------------------------------------------------------------
void CMapData::SetIndexMap(const TThinArray<uint32>& _lResourceNameHashes)
{
	TAP<const uint32> pList = _lResourceNameHashes;
	for (uint i = 1; i < pList.Len(); i++)
	{
		int iRc = m_IndexMap.GetNewID();
		M_ASSERT(iRc == i, "IndexMap internal error");
		m_IndexMap.Insert(iRc, pList[i]);
	}
}

void CMapData::GetIndexMap(TThinArray<uint32>& _lResourceNameHashes) const
{
	uint n = m_IndexMap.GetMaxIDs();
	_lResourceNameHashes.SetLen(n);
	for (uint i = 0; i < n; i++)
	{
		uint32 NameHash;
		if (!m_IndexMap.GetValue(i, NameHash))
		{
			_lResourceNameHashes.SetLen(i);
			return;
		}
		_lResourceNameHashes[i] = NameHash;
	}
}


// -------------------------------------------------------------------
int CMapData::Resource_GetCreateFlags()
{
	MAUTOSTRIP(CMapData_Resource_GetCreateFlags, 0);
	M_ASSERT(m_spWData != NULL, "!");
	return m_spWData->Resource_GetCreateFlags();
}

void CMapData::Resource_PushCreateFlags(int _Flags)
{
	MAUTOSTRIP(CMapData_Resource_PushCreateFlags, MAUTOSTRIP_VOID);
	M_ASSERT(m_spWData != NULL, "!");
	m_spWData->Resource_PushCreateFlags(_Flags);
}

void CMapData::Resource_PopCreateFlags()
{
	MAUTOSTRIP(CMapData_Resource_PopCreateFlags, MAUTOSTRIP_VOID);
	M_ASSERT(m_spWData != NULL, "!");
	m_spWData->Resource_PopCreateFlags();
}

// -------------------------------------------------------------------
int CMapData::GetWorldResourceIndex(int _iRc)
{
	MAUTOSTRIP(CMapData_GetWorldResourceIndex, 0);
	if (_iRc < 0 || _iRc >= m_lspResources.Len()) return 0;
	spCWResource* lpRc = m_lspResources.GetBasePtr();
	CWResource* pRC = lpRc[_iRc];
	return (pRC) ? pRC->m_iRc : 0;
}

void CMapData::PostCreateResource(int _iRc)
{
	MAUTOSTRIP(CMapData_PostCreateResource, MAUTOSTRIP_VOID);
	MSCOPE(CMapData::PostCreateResource, WORLD_CWOBJECT);

	CWResource* pRc = GetResourceAsync(_iRc);
	if (!pRc) return;

	{
		int KeepFlags = WRESOURCE_LOADED | WRESOURCE_ASYNCLOADING | WRESOURCE_ASYNCLOADHIGHPRIOR;
		int SetFlags = m_spWData->Resource_GetCreateFlags() & ~KeepFlags;

		M_LOCK(*(MRTC_GOM()->m_pGlobalLock));
		pRc->m_Flags = (pRc->m_Flags & KeepFlags) | SetFlags;
	}
	
	pRc->OnRegisterMapData(m_spWData, this);

	int RcClass = pRc->GetClass();

#ifndef MDISABLE_CLIENT_SERVER_RES
	if (RcClass == WRESOURCE_CLASS_WOBJECTCLASS)
	{
//		MSCOPE(OnIncludeClass, MAPDATA);

		DebugLog("(CMapData::GetResourceIndex) OnIncludeClass, " + CStr(_pName));

		m_spWData->Resource_LoadSync(pRc->m_iRc);
		CWRes_Class* pRCCls = (CWRes_Class*) pRc;
		M_ASSERT(m_pWServer, "!");
		pRCCls->GetRuntimeClass()->m_pfnOnIncludeClass(this, m_pWServer);

		DebugLog("(CMapData::GetResourceIndex) OnIncludeClass done., " + CStr(_pName));
	}

	else if(RcClass == WRESOURCE_CLASS_TEMPLATE)
	{
//		MSCOPE(OnIncludeTemplate, MAPDATA);
		
		m_spWData->Resource_LoadSync(pRc->m_iRc);
		CWRes_Template *pTpl = (CWRes_Template *)pRc;

		for(int i = 0; i < pTpl->m_lspObjectReg.Len(); i++)
		{
			CRegistry *pClassName = pTpl->m_lspObjectReg[i]->FindChild("CLASSNAME");
			if (!pClassName)
				return;

			int iClass = GetResourceIndex_Class(pClassName->GetThisValue());
			if(iClass <= 0)
				return;

			CWResource *pRC = GetResource(iClass);
			if(!pRC)
				return;

			if(pRC->GetClass() != WRESOURCE_CLASS_WOBJECTCLASS)
				Error("PostCreateResource", "Internal error.");

			CWRes_Class *pCls = (CWRes_Class*) pRC;

//				LogFile(CStr("CWRes_Template::Create, ") + CStr(_pName[4]) + CStr(": ") + pClassName->GetThisValue());

			M_ASSERT(m_pWServer, "!");
			pCls->GetRuntimeClass()->m_pfnOnIncludeTemplate(pTpl->m_lspObjectReg[i], this, m_pWServer);
		}
	}
#endif
}

// -------------------------------------------------------------------
int CMapData::GetNumResources()
{
	MAUTOSTRIP(CMapData_GetNumResources, 0);
	return m_lspResources.Len();
}

void CMapData::SetNumResources(int _Rc)
{
	MAUTOSTRIP(CMapData_SetNumResources, MAUTOSTRIP_VOID);
	m_lspResources.Clear();
	m_lspResources.SetLen(_Rc);
	m_spHash = NULL;
	m_IndexMap.Create(_Rc);
//	FillChar(m_lResourceIDs.GetBasePtr(), m_lResourceIDs.ListSize(), 0);
}

CStr CMapData::GetResourceName(int _iRc)
{
	MAUTOSTRIP(CMapData_GetResourceName, CStr());
	CWResource* pRC = GetResourceAsync(_iRc);
	if (!pRC) return CStr();
	return pRC->GetName();
}

int CMapData::GetResourceClass(int _iRc)
{
	MAUTOSTRIP(CMapData_GetResourceClass, 0);
	CWResource* pRC = GetResourceAsync(_iRc);
	if (!pRC) return WRESOURCE_CLASS_NULL;
	return pRC->GetClass();
}

int CMapData::SetResource(int _iRc, const char* _pName, int _RcClass)
{
	MAUTOSTRIP(CMapData_SetResource, 0);
	if (_iRc < 0 || _iRc >= m_lspResources.Len())
		return false;

	if (_RcClass != WRESOURCE_CLASS_NULL)
	{
		// IMPORTANT: Spooky stuff going on here..
		//
		// Must get the resource index BEFORE getting the reference into the list.
		// The list might be resized during GetResourceIndex and thereby invalidating
		// the reference to m_lResourceIDs[_iRc]

		int iWRc = m_spWData->GetResourceIndex(_pName, _RcClass, this);
		spCWResource spRc = m_spWData->GetResourceRef(iWRc);
		m_lspResources[_iRc] = spRc;
	}
	else
		m_lspResources[_iRc] = NULL;

	CWResource* pRc = GetResourceAsync(_iRc);
	if (!pRc)
	{
		if(_iRc != 0)
			ConOutL(CStrF("(CMapData::SetResource) Failed creating resource %d, Name %s, Class %d", _iRc, _pName, _RcClass));
		m_lspResources[_iRc] = NULL;
	}
	else
	{
		Hash_Insert(_iRc);
	}

	return (m_lspResources[_iRc] != 0);
}

void CMapData::SetState(int _State)
{
	MAUTOSTRIP(CMapData_SetState, MAUTOSTRIP_VOID);
	m_State = _State;
}

int CMapData::GetState()
{
	MAUTOSTRIP(CMapData_GetState, 0);
	return m_State;
}

void CMapData::SetWorld(CStr _FileName)
{
	MAUTOSTRIP(CMapData_SetWorld, MAUTOSTRIP_VOID);
	if (_FileName.GetFilenameExtenstion() == "") _FileName += ".XW";
	m_WorldName = _FileName;
}

CStr CMapData::GetWorld()
{
	MAUTOSTRIP(CMapData_GetWorld, CStr());
	return m_WorldName;
}

#ifndef MDISABLE_CLIENT_SERVER_RES

CWObject* CMapData::CreateObject(const char* _pClassName)
{
	MAUTOSTRIP(CMapData_CreateObject, NULL);
	MSCOPE(CMapData::CreateObject, WORLD_CWOBJECT);

	int iRc = GetResourceIndex_Class(_pClassName);
	if (!iRc) return NULL;
	CWResource* pRC = GetResource(iRc);
	if (!pRC) return NULL;

	if (pRC->GetClass() != WRESOURCE_CLASS_WOBJECTCLASS)
		Error("CreateObject", "Resource was not a class.");

	CWRes_Class* pCls = (CWRes_Class*) pRC;
	CWObject* pObject = (CWObject*) pCls->GetRuntimeClass()->m_pfnCreateObject();
	if (!pObject) 
		MemError("CreateObject");
	pObject->m_pRTC = pCls->GetRuntimeClass();
	pObject->m_iClass = iRc;
	return pObject;
}

const TThinArray<spCRegistry> *CMapData::GetObjectTemplates(const char *_pClassName, const TThinArray<CMat4Dfp32> **_pplMat)
{
	MAUTOSTRIP(CMapData_GetObjectTemplates, NULL);
	int iRc = GetResourceIndex_Template(_pClassName);
	if(!iRc)
		return NULL;

	CWResource* pRC = GetResource(iRc);
	if(!pRC)
		return NULL;

	if(pRC->GetClass() != WRESOURCE_CLASS_TEMPLATE)
		Error("CreateObject", "Resource was not a template.");

	CWRes_Template *pTpl = (CWRes_Template *)pRC;

	if(_pplMat)
		*_pplMat = &pTpl->m_lObjectMat;

	return &pTpl->m_lspObjectReg;
}

#endif

// -------------------------------------------------------------------
void CMapData::Hash_Insert(int _iRc)
{
	MAUTOSTRIP(CMapData_Hash_Insert, MAUTOSTRIP_VOID);
	if (!m_lspResources.ValidPos(_iRc)) return;

	if (!m_spHash)
	{
		m_spHash = MNew(CStringHash);
		if (!m_spHash) MemError("UpdateHashTable");
	}

	if (_iRc < m_spHash->GetMaxIDs())
	{
		m_spHash->Insert(_iRc, GetResourceName(_iRc));
		/*
		CWResource* pRC = GetResourceAsync(_iRc);
		if (!pRC) 
			Error("CMapData::Hash_Insert", "Resource does not exist");
		m_Hash.Insert(_iRc, GetResourceName(_iRc), pRC->m_HashLinkMapData);*/

	}
	else
	{
		if (m_spHash->GetMaxIDs() < m_lspResources.Len())
		{
//			m_spHash = DNew(CStringHash) CStringHash;
			m_spHash->Create(Max(512, m_lspResources.Len()*2), false);
		}

		for(int iRc = 0; iRc < m_lspResources.Len(); iRc++)
			m_spHash->Insert(iRc, GetResourceName(iRc));
	}
}

int CMapData::GetResourceIndex(const char* _pName, int _RcClass)
{
	MAUTOSTRIP(CMapData_GetResourceIndex, 0);
//	MSCOPE(CMapData::GetResourceIndex_C, MAPDATA);

	char TmpName[300];
	StrFixFilename((char*)_pName, TmpName, sizeof(TmpName));

	int iRc = (m_spHash != NULL) ? m_spHash->GetIndex(TmpName) : 0;
//	int iRc = m_Hash.GetIndex(_pName);

	if (iRc > 0) return iRc;
	if (m_State & WMAPDATA_STATE_NOCREATE)
	{
		if (_RcClass != WRESOURCE_CLASS_WOBJECTCLASS)
			ConOutL(CStrF("§cf00ERROR: (CMapData::GetResourceIndex) Non-existing resource requested: %s", TmpName));
		return 0;
	}

	int iWRc = m_spWData->GetResourceIndex(TmpName, _RcClass, this);
	if (iWRc)
	{
		spCWResource spRc = m_spWData->GetResourceRef(iWRc);
		uint32 NameHash = spRc->GetName().StrHash();
		int iRc = m_IndexMap.GetIndex(NameHash);
		if (iRc < 0)
		{
			iRc = m_IndexMap.GetNewID();
			m_IndexMap.Insert(iRc, NameHash);
		}
		m_lspResources.SetMinLen(iRc + 1);
		m_lspResources[iRc] = spRc;
		Hash_Insert(iRc);
		PostCreateResource(iRc);
		return iRc;
	}
	else
		return 0;
}

int CMapData::GetResourceIndex(const char* _pRcName)
{
	MAUTOSTRIP(CMapData_GetResourceIndex_2, 0);
//	MSCOPE(CMapData::GetResourceIndex, MAPDATA);

	char TmpName[300];
	StrFixFilename((char*)_pRcName, TmpName, sizeof(TmpName));

	int iRc = (m_spHash != NULL) ? m_spHash->GetIndex(TmpName) : 0;
//	int iRc = m_Hash.GetIndex(_pRcName);
	if (iRc > 0) return iRc;
	if (m_State & WMAPDATA_STATE_NOCREATE)
	{
		ConOutL(CStrF("§cf00ERROR: (CMapData::GetResourceIndex) Non-existing resource requested: %s", TmpName));
		return 0;
	}

	int iWRc = m_spWData->GetResourceIndex(TmpName, this);
	if (iWRc)
	{
		spCWResource spRc = m_spWData->GetResourceRef(iWRc);
		uint32 NameHash = spRc->GetName().StrHash();
		int iRc = m_IndexMap.GetIndex(NameHash);
		if (iRc < 0)
		{
			iRc = m_IndexMap.GetNewID();
			m_IndexMap.Insert(iRc, NameHash);
		}
		m_lspResources.SetMinLen(iRc + 1);
		m_lspResources[iRc] = spRc;
		Hash_Insert(iRc);
		PostCreateResource(iRc);
		return iRc;
	}
	else
		return 0;
}

/*
CWResource* CMapData::GetResource(int _iRc)
{
	MAUTOSTRIP(CMapData_GetResource, NULL);
	if (_iRc < 0 || _iRc >= m_lspResources.Len()) return 0;
	spCWResource* lpRc = m_lspResources.GetBasePtr();
	CWResource* pRc = lpRc[_iRc];
	if (pRc && !pRc->IsLoaded())
		m_spWData->Resource_LoadSync(pRc->m_iRc);
	return pRc;
}

CWResource* CMapData::GetResourceAsync(int _iRc)
{
	MAUTOSTRIP(CMapData_GetResourceAsync, NULL);
	if (_iRc < 0 || _iRc >= m_lspResources.Len()) return 0;
	spCWResource* lpRc = m_lspResources.GetBasePtr();
	CWResource* pRc = lpRc[_iRc];
	return pRc;
}
*/

#ifndef MDISABLE_CLIENT_SERVER_RES
// -------------------------------------------------------------------
int CMapData::GetResourceIndex_Class(const char* _pName)
{
	MSCOPE(CMapData::GetResourceIndex_Class, RES_CLASS);
	MAUTOSTRIP(CMapData_GetResourceIndex_Class, 0);
	int iClass = GetResourceIndex(CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_WOBJECTCLASS), _pName), WRESOURCE_CLASS_WOBJECTCLASS);
	if(iClass > 0)
		return iClass;

	// Make sure each template gets a resource index
	GetResourceIndex_Template(_pName);
	return 0;
}
#endif

int CMapData::GetResourceIndex_Model(const char* _pName)
{
	MSCOPE(CMapData::GetResourceIndex_Model, RES_MODEL);
	MAUTOSTRIP(CMapData_GetResourceIndex_Model, 0);
//LogFile(CStrF("(CMapData::GetResourceIndex_Model) %s", _pName));
	CFStr Model(_pName);
	Model.MakeUpperCase();
	if (Model.Find(".XW") >= 0)
	{
		//CFStr Name = CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_MODEL_XW2), _pName);
		return GetResourceIndex_BSPModel(_pName);
	}
	else
	{
		CFStr SystemRegistry(_pName);
		// Check if the model is a custom-model.
		if (MRTC_GOM()->GetClassRegistry()->GetRuntimeClass(CFStrF("%s%s", "CXR_Model_", (char*) CFStr(_pName).GetStrSep(":"))))
		{
			// Fetch registry for effect system models
			if (SystemRegistry.GetStrSep(":").UpperCase() == "EFFECTSYSTEM")
				GetResourceIndex_Registry(CFStrF("Registry\\%s", SystemRegistry.GetStrSep(",").GetStr()));
			else
			{
				SystemRegistry = _pName;
				if (SystemRegistry.GetStrSep(":").UpperCase() == "MULTITRIMESH")
				{
					// SystemRegistry = "MultiTriMesh:Models\\" + SystemRegistry;
					SystemRegistry = "MultiTriMesh:" + SystemRegistry;
					return GetResourceIndex(CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_MODEL_CUSTOM_FILE),SystemRegistry.Str()), 
											WRESOURCE_CLASS_MODEL_CUSTOM_FILE);
				}
			}
			
			return GetResourceIndex(CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_MODEL_CUSTOM), _pName), WRESOURCE_CLASS_MODEL_CUSTOM);
		}
		else
		{
			if (SystemRegistry.GetStrSep(":").UpperCase() == "MULTITRIMESH")
			{
				// SystemRegistry = "MultiTriMesh:Models\\" + SystemRegistry;
				SystemRegistry = "MultiTriMesh:" + SystemRegistry;
				return GetResourceIndex(CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_MODEL_CUSTOM_FILE),SystemRegistry.Str()), 
					WRESOURCE_CLASS_MODEL_CUSTOM_FILE);
			}
			
			return GetResourceIndex(CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_MODEL_XMD), _pName), WRESOURCE_CLASS_MODEL_XMD);
		}
	}
}

int CMapData::GetResourceIndex_BSPModel(const char* _pName)
{
	MSCOPE(CMapData::GetResourceIndex_BSPModel, RES_BSPMODEL);
	MAUTOSTRIP(CMapData_GetResourceIndex_BSPModel, 0);

	int XRMode = (m_pWServer) ? m_pWServer->Registry_GetGame()->GetValuei("XR_MODE", 0, 0) : 1;

	int iModel;
	if( XRMode == 1 )
		iModel = GetResourceIndex_BSPModel2(_pName);
	else if( XRMode == 2 )
		iModel = GetResourceIndex_BSPModel3(_pName);
	else
		iModel = GetResourceIndex_BSPModel1(_pName);
		
	return iModel;
}

int CMapData::GetResourceIndex_BSPModel1(const char* _pName)
{
	MSCOPE(CMapData::GetResourceIndex_BSPModel1, RES_BSPMODEL1);
	MAUTOSTRIP(CMapData_GetResourceIndex_BSPModel1, 0);
	CStr Name(_pName);
	if (Name.Find("$WORLD") >= 0)
	{
		CStr NewName = Name.GetStrSep(":");
		int ModelNr = Name.Val_int();
		Name = CFStrF("%s:%s:%d", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_MODEL_XW), (const char*) m_WorldName, ModelNr);
	}
	else
		Name = CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_MODEL_XW), _pName);

	return GetResourceIndex(Name, WRESOURCE_CLASS_MODEL_XW);
}

int CMapData::GetResourceIndex_BSPModel2(const char* _pName)
{
	MSCOPE(CMapData::GetResourceIndex_BSPModel2, RES_BSPMODEL2);
	MAUTOSTRIP(CMapData_GetResourceIndex_BSPModel2, 0);
	CStr Name(_pName);
	if (Name.Find("$WORLD") >= 0)
	{
		CStr NewName = Name.GetStrSep(":");
		int ModelNr = Name.Val_int();
		Name = CFStrF("%s:%s:%d", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_MODEL_XW2), (const char*) m_WorldName, ModelNr);
	}
	else
		Name = CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_MODEL_XW2), _pName);

	return GetResourceIndex(Name, WRESOURCE_CLASS_MODEL_XW2);
}

int CMapData::GetResourceIndex_BSPModel3(const char* _pName)
{
	MSCOPE(CMapData::GetResourceIndex_BSPModel3, RES_BSPMODEL3);
	MAUTOSTRIP(CMapData_GetResourceIndex_BSPModel3, 0);
	CStr Name(_pName);
	if (Name.Find("$WORLD") >= 0)
	{
		CStr NewName = Name.GetStrSep(":");
		int ModelNr = Name.Val_int();
		Name = CFStrF("%s:%s:%d", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_MODEL_XW3), (const char*) m_WorldName, ModelNr);
	}
	else
		Name = CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_MODEL_XW3), _pName);

	return GetResourceIndex(Name, WRESOURCE_CLASS_MODEL_XW3);
}

int CMapData::GetResourceIndex_BSPModel4(const char* _pName)
{
	MSCOPE(CMapData::GetResourceIndex_BSPModel4, RES_BSPMODEL4);
	MAUTOSTRIP(CMapData_GetResourceIndex_BSPModel4, 0);
	CStr Name(_pName);
	if (Name.Find("$WORLD") >= 0)
	{
		CStr NewName = Name.GetStrSep(":");
		int ModelNr = Name.Val_int();
		Name = CFStrF("%s:%s:%d", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_MODEL_XW4), (const char*) m_WorldName, ModelNr);
	}
	else
		Name = CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_MODEL_XW4), _pName);

	return GetResourceIndex(Name, WRESOURCE_CLASS_MODEL_XW4);
}

int CMapData::GetResourceIndex_CustomModel(const char* _pName)
{
	MSCOPE(CMapData::GetResourceIndex_CustomModel, RES_CUSTOMMODEL);
	MAUTOSTRIP(CMapData_GetResourceIndex_CustomModel, 0);
LogFile("(CMapData::GetResourceIndex_CustomModel) This function is obsolete. Use CMapData::GetResourceIndex_Model() instead.");
	return GetResourceIndex(CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_MODEL_CUSTOM), _pName), WRESOURCE_CLASS_MODEL_CUSTOM);
}

/*int CMapData::GetResourceIndex_CustomModelFile(const char* _pName)
{
	MAUTOSTRIP(CMapData_GetResourceIndex_CustomModelFile, NULL);
LogFile("(CMapData::GetResourceIndex_CustomModel) This function is obsolete. Use CMapData::GetResourceIndex_Model() instead.");
	return GetResourceIndex(CFStrF("%s%s", g_lResourcePrefixes[WRESOURCE_CLASS_MODEL_CUSTOM_FILE], _pName), WRESOURCE_CLASS_MODEL_CUSTOM_FILE);
}*/
int CMapData::GetResourceIndex_Wave(const char* _pName)
{
	MSCOPE(CMapData::GetResourceIndex_Wave, RES_WAVE);
	MAUTOSTRIP(CMapData_GetResourceIndex_Wave, 0);
	return GetResourceIndex(CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_WAVE), _pName), WRESOURCE_CLASS_WAVE);
}

int CMapData::GetResourceIndex_Sound(const char* _pName)
{
	MSCOPE(CMapData::GetResourceIndex_Sound, RES_SOUND);
	MAUTOSTRIP(CMapData_GetResourceIndex_Sound, 0);

	// If the sound starts on a $ it's a waveid and not a soundid
	if(_pName && _pName[0] == '$')
		return GetResourceIndex_Wave(_pName);

	return GetResourceIndex(CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_SOUND), _pName), WRESOURCE_CLASS_SOUND);
}

int CMapData::GetResourceIndex_Anim(const char* _pName)
{
	MSCOPE(CMapData::GetResourceIndex_Anim, RES_ANIM);
	MAUTOSTRIP(CMapData_GetResourceIndex_Anim, 0);
	return GetResourceIndex(CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_XSA), _pName), WRESOURCE_CLASS_XSA);
}

#ifndef M_DISABLE_TODELETE
int CMapData::GetResourceIndex_AnimList(const char* _pName)
{
	MAUTOSTRIP(CMapData_GetResourceIndex_AnimList, 0);
	return GetResourceIndex(CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_ANIMLIST), _pName), WRESOURCE_CLASS_ANIMLIST);
}
#endif

int CMapData::GetResourceIndex_AnimGraph2(const char* _pName)
{
	MSCOPE(CMapData::GetResourceIndex_AnimGraph2, RES_ANIMGRAPH2);
	MAUTOSTRIP(CMapData_GetResourceIndex_AnimGraph2, 0);
	return GetResourceIndex(CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_XAH), _pName), WRESOURCE_CLASS_XAH);
}

int CMapData::GetResourceIndex_Dialogue(const char* _pName)
{
	MSCOPE(CMapData::GetResourceIndex_Dialogue, RES_DIALOG);
	MAUTOSTRIP(CMapData_GetResourceIndex_Dialogue, 0);
	return GetResourceIndex(CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_DIALOGUE), _pName), WRESOURCE_CLASS_DIALOGUE);
}

int CMapData::GetResourceIndex_Registry(const char* _pName)
{
	MSCOPE(CMapData::GetResourceIndex_Registry, RES_REGISTRY);
	MAUTOSTRIP(CMapData_GetResourceIndex_Registry, 0);
	return GetResourceIndex(CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_REGISTRY), _pName), WRESOURCE_CLASS_REGISTRY);
}

int CMapData::GetResourceIndex_Surface(const char* _pName)
{
	MSCOPE(CMapData::GetResourceIndex_Surface, RES_SURFACE);
	MAUTOSTRIP(CMapData_GetResourceIndex_Surface, 0);
	return GetResourceIndex(CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_SURFACE), _pName), WRESOURCE_CLASS_SURFACE);
}

int CMapData::GetResourceIndex_Font(const char* _pName)
{
	MSCOPE(CMapData::GetResourceIndex_Font, RES_FONT);
	MAUTOSTRIP(CMapData_GetResourceIndex_Font, 0);
	return GetResourceIndex(CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_XFC), _pName), WRESOURCE_CLASS_XFC);
}

int CMapData::GetResourceIndex_XWData(const char* _pName)
{
	MSCOPE(CMapData::GetResourceIndex_XWData, RES_XW_DATA);
	MAUTOSTRIP(CMapData_GetResourceIndex_XWData, 0);
	CStr Name(_pName);
	if (Name.Find("$WORLD") >= 0)
	{
		CStr NewName = Name.GetStrSep(":");
		int ModelNr = Name.Val_int();
		Name = CFStrF("%s:%s:%d", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_XWDATA), (const char*) m_WorldName, ModelNr);
	}
	else
		Name = CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_XWDATA), _pName);

	return GetResourceIndex(Name, WRESOURCE_CLASS_XWDATA);
}

#ifndef MDISABLE_CLIENT_SERVER_RES
int CMapData::GetResourceIndex_Template(const char* _pName)
{
	MSCOPE(CMapData::GetResourceIndex_Template, RES_TEMPLATE);
	MAUTOSTRIP(CMapData_GetResourceIndex_Template, 0);
	return GetResourceIndex(CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_TEMPLATE), _pName), WRESOURCE_CLASS_TEMPLATE);
}
#endif

int CMapData::GetResourceIndex_XWNavGrid(const char* _pName)
{
	MSCOPE(CMapData::GetResourceIndex_XWNavGrid, RES_XW_NAVGRID);
	MAUTOSTRIP(CMapData_GetResourceIndex_XWNavGrid, 0);
	CStr Name(_pName);
	if (Name.Find("$WORLD") >= 0)
	{
		CStr NewName = Name.GetStrSep(":");
		int ModelNr = Name.Val_int();
		Name = CFStrF("%s:%s:%d", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_XWNAVGRID), (const char*) m_WorldName, ModelNr);
	}
	else
		Name = CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_XWNAVGRID), _pName);

	return GetResourceIndex(Name, WRESOURCE_CLASS_XWNAVGRID);
}

int CMapData::GetResourceIndex_XWNavGraph(const char* _pName)
{
	MSCOPE(CMapData::GetResourceIndex_XWNavGraph, RES_XW_NAVGRAF);
	MAUTOSTRIP(CMapData_GetResourceIndex_XWNavGraph, 0);
	CStr Name(_pName);
	if (Name.Find("$WORLD") >= 0)
	{
		CStr NewName = Name.GetStrSep(":");
		int ModelNr = Name.Val_int();
		Name = CFStrF("%s:%s:%d", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_XWNAVGRAPH), (const char*) m_WorldName, ModelNr);
	}
	else
		Name = CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_XWNAVGRAPH), _pName);

	return GetResourceIndex(Name, WRESOURCE_CLASS_XWNAVGRAPH);
}

int CMapData::GetResourceIndex_ObjectAttribs(const char* _pName)
{
	MAUTOSTRIP(CMapData_GetResourceIndex_ObjectAttribs, 0);
	MSCOPESHORT(CMapData::GetResourceIndex_ObjectAttribs);

	CFStr Name = CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_OBJECTATTRIBS), _pName);
	return GetResourceIndex(Name, WRESOURCE_CLASS_OBJECTATTRIBS);
}


int CMapData::GetResourceIndex_FacialSetup(const char* _pName)
{
	MAUTOSTRIP(CMapData_GetResourceIndex_FacialSetup, 0);
	MSCOPESHORT(CMapData::GetResourceIndex_ObjectAttribs);

	CFStr Name = CFStrF("%s:%s", m_spWData->GetResourceClassPrefix(WRESOURCE_CLASS_FACIALSETUP), _pName);
	return GetResourceIndex(Name, WRESOURCE_CLASS_FACIALSETUP);
}




// -------------------------------------------------------------------
void CMapData::GetResourceIndex_Model_Directory_r(const CStr _BasePath, const CStr _Path)
{
	MAUTOSTRIP(CMapData_GetResourceIndex_Model_Directory_r, MAUTOSTRIP_VOID);
	CDirectoryNode Node;
	Node.ReadDirectory(_BasePath + _Path + "\\*");
	int nFiles = Node.GetFileCount();

	for(int f = 0; f < nFiles; f++)
	{
		CDir_FileRec *pRec = Node.GetFileRec(f);
		pRec->m_Ext.MakeUpperCase();
		if(pRec->IsDirectory() && pRec->m_Name != ".." && pRec->m_Name != ".")
			GetResourceIndex_Model_Directory_r(_BasePath, CStrF("%s\\%s", (const char*)_Path, (const char*)pRec->m_Name));
		else if(pRec->m_Ext == "XMD")
			GetResourceIndex_Model(_Path + "\\" + pRec->m_Name.GetFilenameNoExt());
	}
}

void CMapData::GetResourceIndex_Anim_Directory_r(const CStr _BasePath, const CStr _Path)
{
	MAUTOSTRIP(CMapData_GetResourceIndex_Anim_Directory_r, MAUTOSTRIP_VOID);
	CDirectoryNode Node;
	Node.ReadDirectory(_BasePath + _Path + "\\*");
	int nFiles = Node.GetFileCount();

	for(int f = 0; f < nFiles; f++)
	{
		CDir_FileRec *pRec = Node.GetFileRec(f);
		pRec->m_Ext.MakeUpperCase();
		if(pRec->IsDirectory() && pRec->m_Name != ".." && pRec->m_Name != ".")
			GetResourceIndex_Anim_Directory_r(_BasePath, CStrF("%s\\%s", (const char*)_Path, (const char*)pRec->m_Name));
		else if(pRec->m_Ext == "XSA")
			GetResourceIndex_Anim(_Path + "\\" + pRec->m_Name.GetFilenameNoExt());
	}
}

void CMapData::GetResourceIndex_Model_Directory(const CStr _Path)
{
	MAUTOSTRIP(CMapData_GetResourceIndex_Model_Directory, MAUTOSTRIP_VOID);
	GetResourceIndex_Model_Directory_r(m_spWData->GetBasePath() + "MODELS\\", _Path);
}

void CMapData::GetResourceIndex_Anim_Directory(const CStr _Path)
{
	MAUTOSTRIP(CMapData_GetResourceIndex_Anim_Directory, MAUTOSTRIP_VOID);
	GetResourceIndex_Anim_Directory_r(m_spWData->GetBasePath() + "ANIM\\", _Path);
}

#ifndef MDISABLE_CLIENT_SERVER_RES
// -------------------------------------------------------------------
MRTC_CRuntimeClass_WObject* CMapData::GetResource_Class(int _iClass)
{
	MAUTOSTRIP(CMapData_GetResource_Class, NULL);
//	MSCOPE(CMapData::GetResource_Class, MAPDATA);
	CWResource* pRC = GetResource(_iClass);
	if (!pRC) return NULL;
	if (pRC->GetClass() != WRESOURCE_CLASS_WOBJECTCLASS) return NULL;

	CWRes_Class* pRCCls = (CWRes_Class*) pRC;
	return pRCCls->GetRuntimeClass();
}

CWorldData::CWD_ClassStatistics* CMapData::GetResource_ClassStatistics(int _iClass)
{
	MAUTOSTRIP(CMapData_GetResource_ClassStatistics, NULL);
//	MSCOPE(CMapData::GetResource_Class, MAPDATA);
	CWResource* pRC = GetResource(_iClass);
	if (!pRC) return NULL;
	if (pRC->GetClass() != WRESOURCE_CLASS_WOBJECTCLASS) return NULL;

	CWRes_Class* pRCCls = (CWRes_Class*) pRC;
	return pRCCls->GetStats();
}
#endif

CWRes_Dialogue *CMapData::GetResource_Dialogue(int _iDialogue)
{
	MAUTOSTRIP(CMapData_GetResource_Dialogue, NULL);
	CWResource* pRC = GetResource(_iDialogue);
	if(!pRC)
		return NULL;

	if(pRC->GetClass() == WRESOURCE_CLASS_DIALOGUE)
		return (CWRes_Dialogue *)pRC;
	else
		return NULL;
}

CRegistry* CMapData::GetResource_Registry(int _iRc)
{
	MAUTOSTRIP(CMapData_GetResource_Registry, NULL);
	CWResource* pRC = GetResource(_iRc);
	if (!pRC) return NULL;

	if (pRC->GetClass() == WRESOURCE_CLASS_REGISTRY)
	{
		CWRes_Registry* pRCReg = (CWRes_Registry*) pRC;
		return pRCReg->GetRegistry();
	}
	else
		return NULL;
}

CXW_Surface* CMapData::GetResource_Surface(int _iRc)
{
	MAUTOSTRIP(CMapData_GetResource_Surface, NULL);
	CWResource* pRC = GetResource(_iRc);
	if (!pRC) return NULL;

	if (pRC->GetClass() == WRESOURCE_CLASS_SURFACE)
	{
		CWRes_Surface* pRCSurf = (CWRes_Surface*) pRC;
		return pRCSurf->GetSurface();
	}
	else
		return NULL;
}

int CMapData::GetResource_SurfaceID(int _iRc)
{
	MAUTOSTRIP(GetResource_SurfaceID, NULL);
	CWResource* pRC = GetResource(_iRc);
	if (!pRC) return NULL;
	
	if (pRC->GetClass() == WRESOURCE_CLASS_SURFACE)
	{
		CWRes_Surface* pRCSurf = (CWRes_Surface*) pRC;
		return pRCSurf->GetSurfaceID();
	}
	else
		return 0;
}

CRC_Font* CMapData::GetResource_Font(int _iRc)
{
	MAUTOSTRIP(CMapData_GetResource_Font, NULL);
	CWResource* pRC = GetResource(_iRc);
	if (!pRC) return NULL;

	if (pRC->GetClass() == WRESOURCE_CLASS_XFC)
	{
		CWRes_XFC* pRCReg = (CWRes_XFC*) pRC;
		return pRCReg->GetFont();
	}
	else
		return NULL;
}

TAP<const uint8> CMapData::GetResource_XWData(int _iRc, uint _iXWResource)
{
	MAUTOSTRIP(CMapData_GetResource_XWData, NULL);
	CWResource* pRC = GetResource(_iRc);

	if (!pRC || pRC->GetClass() != WRESOURCE_CLASS_XWDATA)
		return TAP<const uint8>(); // return empty data

	CWRes_XWResource* pXWData = (CWRes_XWResource*)pRC;
	return pXWData->GetResourceData(_iXWResource);
}

CXR_BlockNav_Grid_GameWorld* CMapData::GetResource_XWNavGrid(int _iModel)
{
	MAUTOSTRIP(CMapData_GetResource_XWNavGrid, NULL);
	CWResource* pRC = GetResource(_iModel);
	if (!pRC) return NULL;
	if (pRC->GetClass() != WRESOURCE_CLASS_XWNAVGRID) return NULL;

	CWRes_XWNavGrid* pRCGrid = (CWRes_XWNavGrid*) pRC;
	return pRCGrid->GetNavGrid();
}

CXR_NavGraph* CMapData::GetResource_XWNavGraph(int _iModel)
{
	MAUTOSTRIP(CMapData_GetResource_XWNavGraph, NULL);
	CWResource* pRC = GetResource(_iModel);
	if (!pRC) return NULL;
	if (pRC->GetClass() != WRESOURCE_CLASS_XWNAVGRAPH) return NULL;

	CWRes_XWNavGraph* pRCGrid = (CWRes_XWNavGraph*) pRC;
	return pRCGrid->GetNavGraph();
}

CRegistryDirectory *CMapData::GetResource_ObjectAttribs(int _iModel)
{
	MAUTOSTRIP(CMapData_GetResource_ObjectAttribs, TArray<spCRegistry>());
	CWResource* pRC = GetResource(_iModel);
	class CEmpty : public CRegistryDirectory
	{
	public:
		virtual int NumReg()
		{
			return 0;
		}
		virtual CRegistry *GetReg(int _iReg)
		{
			return 0;
		}
	};
	static CEmpty Empty;
	if (!pRC)
	{
		return &Empty;
	}
	if (pRC->GetClass() != WRESOURCE_CLASS_OBJECTATTRIBS) return &Empty;

	CWRes_ObjectAttribs* pRCObjectAttribs = safe_cast<CWRes_ObjectAttribs>(pRC);
	return pRCObjectAttribs->GetRegDir();
}

CFacialSetup* CMapData::GetResource_FacialSetup(int _iRc)
{
	MAUTOSTRIP(CMapData_GetResource_FacialSetup, NULL);
	CWResource* pRC = GetResource(_iRc);
	if (!pRC || pRC->GetClass() != WRESOURCE_CLASS_FACIALSETUP)
		return NULL;

	CWRes_FacialSetup* pRCFacialSetup = safe_cast<CWRes_FacialSetup>(pRC);
	return pRCFacialSetup->GetData();
}


// -------------------------------------------------------------------
void CMapData::Write(CDataFile* _pDFile)
{
	MAUTOSTRIP(CMapData_Write, MAUTOSTRIP_VOID);
	CCFile* pF = _pDFile->GetFile();
	_pDFile->BeginEntry("NAME");
	m_WorldName.Write(pF);
	_pDFile->EndEntry(0);

	_pDFile->BeginEntry("RESOURCES");
	int nRc = GetNumResources();
	for(int i = 0; i < nRc; i++)
	{
		uint32 Class = GetResourceClass(i);
		CStr Name = GetResourceName(i);
		CWResource *pRes = GetResource(i);
		uint32 Flags = 0;
		if (pRes)
			Flags = pRes->m_Flags;
		pF->WriteLE(Flags);		
		pF->WriteLE(Class);
		Name.Write(pF);
	}
	_pDFile->EndEntry(nRc);
}

void CMapData::Read(CDataFile* _pDFile)
{
	MAUTOSTRIP(CMapData_Read, MAUTOSTRIP_VOID);
	CCFile* pF = _pDFile->GetFile();

	_pDFile->PushPosition();
	if (!_pDFile->GetNext("NAME")) Error("Read", "Corrupt demo-file. (NAME)");
	m_WorldName.Read(pF);
	_pDFile->PopPosition();

	_pDFile->PushPosition();
	{
		if (!_pDFile->GetNext("RESOURCES")) Error("Read", "Corrupt demo-file. (RESOURCES)");

		int nRc = _pDFile->GetUserData();
		SetNumResources(nRc);

		for(int i = 0; i < nRc; i++)
		{
			uint32 Class;
			CStr Name;
			uint32 Flags;
			pF->ReadLE(Flags);
			pF->ReadLE(Class);
			Name.Read(pF);
LogFile(CStrF("(CMapData::Read) SetResource %d, %s", i, (char*)Name));
			SetResource(i, Name, Class);
			CWResource *pRes = GetResource(i);
			if (pRes)
				pRes->m_Flags |= Flags & WRESOURCE_PRECACHE;
		}
	}
	_pDFile->PopPosition();

}

// -------------------------------------------------------------------
void CMapData::LogUsed(CStr _Filename)
{
	CCFile Output;
	Output.Open(_Filename, CFILE_WRITE|CFILE_BINARY);
	if(!Output.IsOpen())
		return;

	int32 nResources = m_lspResources.Len();
	Output.WriteLE(nResources);
	for(int i = 0; i < nResources; i++)
	{
		CWResource *pRC = m_lspResources[i];
		if(pRC && pRC->IsLoaded())
			pRC->GetName().Ansi().LowerCase().Write(&Output);
	}

}

void CMapData::Dump(int _DumpFlags)
{
	MAUTOSTRIP(CMapData_Dump, MAUTOSTRIP_VOID);
/*	LogFile("----------------------------------------------------------------------------------");
	LogFile(" MAPDATA RESOURCE-MAPPING");
	LogFile("----------------------------------------------------------------------------------");*/
	for(int i = 0; i < m_lspResources.Len(); i++)
	{
		CWResource* pR = GetResourceAsync(i);
		if (pR)
		{
			LogFile(CStrF("%4.d, %4.d, %s", i, pR->m_iRc, (char*) pR->GetName()));
		}
		else
			LogFile(CStrF("%4.d,  -", i));
	}
//	LogFile("----------------------------------------------------------------------------------");
}

