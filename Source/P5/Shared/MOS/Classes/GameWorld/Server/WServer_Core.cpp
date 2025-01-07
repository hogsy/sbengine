#include "PCH.h"
#include "WServer_Core.h"
#include "../Client/WClient_Core.h"
#include "../WPackets.h"
#include "../WMapData.h"
#include "../../../XR/XRBlockNav.h"
#include "../../Render/MRenderCapture.h"
#include "MFloat.h"
#include "../../../MSystem/Misc/MPerfGraph.h"
#include "../WDynamics.h"

#ifdef MRTC_MEMORYDEBUG
	#define CHECKMEMORY(s) { if (!_CrtCheckMemory()) Error(s, "Memory check failure."); }
#else
	#define CHECKMEMORY(s)
#endif

//#pragma optimize("", off)
//#pragma inline_depth(0)

//#define SERVER_DISABLE_SCENEGRAPH

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWS_GUIDHash
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CWS_GUIDHash::Create(int _nIndices)
{
	MAUTOSTRIP(CWS_GUIDHash_Create, MAUTOSTRIP_VOID);
	THash<int16, CWS_GUIDElement>::Create(_nIndices, SERVER_NUM_GUID_HASH_SLOTS, false);
}

void CWS_GUIDHash::Insert(int _Index, int _GUID)
{
	MAUTOSTRIP(CWS_GUIDHash_Insert, MAUTOSTRIP_VOID);
	{
		CHashIDInfo* pID = &m_pIDInfo[_Index];
		if(pID->m_Flags)
			M_BREAKPOINT;
	}
//	THash<int16, CWS_GUIDElement>::Remove(_Index);	// Prevent dual-insertions by removing it first.
	THash<int16, CWS_GUIDElement>::Insert(_Index, _GUID & SERVER_NUM_GUID_HASH_AND);
	CHashIDInfo* pID = &m_pIDInfo[_Index];
	pID->m_GUID = _GUID;
}

int CWS_GUIDHash::GetIndex(int _GUID)
{
	MAUTOSTRIP(CWS_GUIDHash_GetIndex, 0);
	int ID = m_pHash[_GUID & SERVER_NUM_GUID_HASH_AND];
	while(ID != -1)
	{
		if (m_pIDInfo[ID].m_GUID == _GUID) return ID;
		ID = m_pIDInfo[ID].m_iNext;
	}

	return 0;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWC_ObjectPerfGraph
|__________________________________________________________________________________________________
\*************************************************************************************************/
#ifndef DEF_DISABLE_PERFGRAPH

CWS_ObjectPerfGraph::CWS_ObjectPerfGraph()
{
	MAUTOSTRIP(CWS_ObjectPerfGraph_ctor, MAUTOSTRIP_VOID);
}

void CWS_ObjectPerfGraph::Create()
{
	MAUTOSTRIP(CWS_ObjectPerfGraph_Create, MAUTOSTRIP_VOID);
	m_spGraph_OnRefresh = (CPerfGraph*) MRTC_GOM()->CreateObject("CPerfGraph");
	if (!m_spGraph_OnRefresh)
		Error("Create", "Could not create perf-graphs.");
	m_spGraph_OnRefresh->Create(32, 0, 0.001f, 16, 16, 2);			// -500..500ms

	m_LastTouch_OnRefresh = 0;
	m_Time_OnRefresh.Reset();
}

#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWServer_ClientInfo
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWServer_ClientInfo::CWServer_ClientInfo()
{
	MAUTOSTRIP(CWServer_ClientInfo_ctor, MAUTOSTRIP_VOID);
	m_hConnection = -1;
	m_LastPing = 0;
	m_iPlayer = -1;
	m_State = WCLIENT_STATE_VOID;
	m_ResourceNr = 0;
	m_ChangeLevelState = 0;
	m_iLastPlayerUpdate = 0;
	m_nClientSimTicks = 0;
	m_pPVS = NULL;
	m_ServerUpdateMask = 0;
	m_bEatAllInput = 0;
	m_bFrameOut = false;
	m_bFrameComplete = false;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWorld_Server
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_DYNAMIC(CWorld_ServerCore, CWorld_Server);

CWorld_ServerCore::CWorld_ServerCore()
{
	MAUTOSTRIP(CWorld_ServerCore_ctor, MAUTOSTRIP_VOID);
	m_Loading = 0;
	m_ServerMode = 0;
	m_PlayMode = SERVER_PLAYMODE_NORMAL;
	m_bNetShowMsg = 0;
	m_iRConClient = -1;
	m_TimeScale = 1.0f;
	m_SimulationTick = 0;
//	m_SimulationTime = 0;
	m_Simulate_iNextObject = 0;
	m_Simulate_LocalFrameFraction = 0;
	m_MaxPlayers = 8;
	m_iWorldModel = -1;
	m_pSceneGraph = 0;
	m_bWorld_PendingChange = false;
//	m_World_PendingName = "";
//	m_World_PendingFlags = 0;
	m_NextGUID = 1;
	m_PhysRenderFlags = 0;
	m_CurSpawnMask = 1;
	m_PrevSpawnMask = 0;
	m_bSkipSound = false;
#ifndef M_RTM
	m_bLogMessages = false;
	m_bConsoleLogMessages = false;
#endif

#ifdef	SERVER_STATS
//	m_Simulate_CPUTime = 0;
//	m_TInterpolate = 0;
//	m_TExecute = 0;
//	m_TClients = 0;
//	m_TSimulate = 0;
//	m_NumSimulate = 0;
#endif
	m_bPendingPrecacheDoUnpause = false;

	m_iGame = -1;

	m_SpawnLen = 0;
	m_SpawnCurrentObject = 0;

#ifndef DEF_DISABLE_PERFGRAPH
	m_PerfGraph = 0;
	m_PerfGraph_Tick_OnRefresh = 0;
#endif

	m_NextConstraintID = 0;
	m_bCharacterPush = 1;
}


void CWorld_ServerCore::Create(int _Mode, spCRegistry _spGameReg, spCWorldData _spWData)
{
	MAUTOSTRIP(CWorld_ServerCore_Create, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_ServerCore::Create);
	m_ServerMode = _Mode;
	m_SimulationTick = 0;
	m_SimulationTime.Reset();
	m_Simulate_iNextObject = 0;
	m_spGameReg = _spGameReg;
	m_MaxPlayers = 8;
	m_spWData = _spWData;
	m_spMapData = MNew(CMapData);
	if(!m_spMapData)
		MemError("Create");
	m_spMapData->Create(m_spWData);

	/*
		Dynamics
	 */
#ifndef WSERVER_DYNAMICSENGINE2
//	m_DynamicsWorld.SetWorldCollider(StarDynamics::CWorldCollider::GetInstance());
	m_DynamicsWorld.SetWorldCollider(CWO_DynamicsCollider::GetInstance());
	m_DynamicsWorld.SetGravity(CVec3Dfp64(0,0,-9.8));
	m_DynamicsWorld.SetScaleFactor(1.0/32.0);
//	m_DynamicsWorld.SetScaleFactor(1.0);
#endif

	m_lConstraints.QuickSetLen(0);
	m_DynamicsWorld2.SetCollider(&m_WorldModelCollider);
	m_DynamicsWorld2.SetGravity(CVec3Dfp32(0,0,-9.8f));
	m_NextConstraintID = 0;

	if (!(m_ServerMode & SERVER_MODE_SECONDARY))
	{
		AddToConsole();
		m_ServerMode |= SERVER_MODE_CONSOLEREG;
	}
	
	m_ClientClass = m_spGameReg->GetValue("CLIENTCLASS", "CWorld_Client");

	m_NextGameType = m_spGameReg->GetValue("DEFAULTGAME", "Debug");

	OnRegistryChanged();

	Registry_ClearWorld();
}

CWorld_ServerCore::~CWorld_ServerCore()
{
	MAUTOSTRIP(CWorld_ServerCore_dtor, MAUTOSTRIP_VOID);
//	M_TRY
//	{
		ConOut("Destroying server.");
		LogFile("(CWorld_ServerCore::~CWorld_Server)");

		m_spMsgQueue = NULL;
		m_spCapture = NULL;

//		Dump(WDUMP_SERVER);
		World_Close();

		for(int iC = 0; iC < m_lspClients.Len(); iC++)
			Net_DestroyClient(iC);

		if (m_spWireContainer)
			MRTC_GOM()->UnregisterObject((CReferenceCount*)m_spWireContainer, "GAMECONTEXT.SERVER.WIRECONTAINER");
		m_spWireContainer = NULL;

		m_spGameReg->DeleteDir("SERVER");

//		LogFile("(CWorld_ServerCore::~CWorld_Server) Done.");

		if (m_ServerMode & SERVER_MODE_CONSOLEREG)
			RemoveFromConsole();

		if (m_spWData)
			m_spWData->Resource_WorldCloseServer();

/*	}
	M_CATCH(
	catch(CCException)
	{
	}
	)*/
}

void CWorld_ServerCore::PreDisconnect()
{
	MAUTOSTRIP(CWorld_ServerCore_PreDisconnect, MAUTOSTRIP_VOID);
	if(m_bPendingPrecacheDoUnpause)
	{
		MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
		if (pCon)
		{
			pCon->ExecuteString("pause(0)");
			m_bPendingPrecacheDoUnpause = false;
		}
	}

	World_Close();

	for(int i = 0; i < m_lspClientInfo.Len(); i++)
	{
		if(m_lspClientInfo[i] && m_spNetwork)
		{
			m_spNetwork->Connection_Close(m_lspClientInfo[i]->m_hConnection);
		}
	}

	m_lspClients.Destroy();
	m_lspClientInfo.Destroy();

	World_CleanResources1(0);
	World_CleanResources2(0);
}

static void DumpReg(CRegistry* _pReg, int _nLevel = 0)
{
	CStr Name = _pReg->GetThisName();
	CStr Value = _pReg->GetThisValue();
	M_TRACEALWAYS("%*s*%s %s\n", _nLevel*4, "", Name.Str(), Value.Str());

	if (_nLevel < 2)
	{
		uint nCh = _pReg->GetNumChildren();
		for (uint i = 0; i < nCh; i++)
		{
			CRegistry* pChild = _pReg->GetChild(i);
			DumpReg(pChild, _nLevel+1);
		}
	}
}

void CWorld_ServerCore::OnRegistryChanged()
{
	MAUTOSTRIP(CWorld_ServerCore_OnRegistryChanged, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_ServerCore::OnRegistryChanged);
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	CRegistry* pReg = (pSys) ? pSys->GetEnvironment() : NULL;

	m_bNetShowMsg = pReg->GetValuei("SV_SHOWMSG", 0) != 0;

	M_TRACEALWAYS("CWorld_ServerCore::OnRegistryChanged, m_spGameReg = \n");
	static bool bDump = false;
	if (bDump)
		DumpReg(m_spGameReg);

	m_spServerReg = m_spGameReg->CreateDir("SERVER");
	m_spTemplateReg = m_spServerReg->Find("TEMPLATES");

	M_ASSERT(m_spServerReg != NULL, "!");
//	if (m_spTemplateReg != NULL)
//		m_spTemplateReg->EnableAutoHashing(false);

	m_TimeScale = Registry_GetServer()->GetValuef("TIMESCALE", 1.0f, 0);
	m_TickRealTime = m_TickTime / m_TimeScale;
	for(int i = 0; i < m_lspClients.Len(); i++)
	{
		CRegistry* pReg = Registry_GetClient(i, 0);
		if (pReg)
		{
			pReg->SetValuef("TIMESCALE", m_TimeScale);
			SetUpdate(i, SERVER_CLIENTUPDATE_DELTAREGISTRY);
		}
	}
}

void CWorld_ServerCore::SetUpdate(int _iClient, int _Mask)
{
	MAUTOSTRIP(CWorld_ServerCore_SetUpdate, MAUTOSTRIP_VOID);
	if (m_lspClientInfo[_iClient] != NULL)
		m_lspClientInfo[_iClient]->m_ServerUpdateMask |= _Mask;
}

void CWorld_ServerCore::SetUpdate_All(int _Mask)
{
	MAUTOSTRIP(CWorld_ServerCore_SetUpdate_All, MAUTOSTRIP_VOID);
	int nc = m_lspClients.Len();
	for(int i = 0; i < nc; i++) SetUpdate(i, _Mask);
}
/*

fp32 CWorld_ServerCore::GetTimeScale()
{
	MAUTOSTRIP(CWorld_ServerCore_GetTimeScale, 0.0);
	return m_TimeScale;
}

fp32 CWorld_ServerCore::GetGameTickRealTime()
{
	MAUTOSTRIP(CWorld_ServerCore_GetGameTickRealTime, 0.0);
	return GetGameTickTime() / m_TimeScale;
}

fp32 CWorld_ServerCore::GetGameTickTime()
{
	MAUTOSTRIP(CWorld_ServerCore_GetGameTickTime, 0.0);
	return SERVER_TIMEPERFRAME;
}

int CWorld_ServerCore::GetGameTick()
{
	MAUTOSTRIP(CWorld_ServerCore_GetGameTick, 0);
	return m_SimulationTick;
}

CMTime CWorld_ServerCore::GetGameTime()
{
	MAUTOSTRIP(CWorld_ServerCore_GetGameTime, 0.0);
	return m_SimulationTime;
}
*/

// -------------------------------------------------------------------
//  Registry management
// -------------------------------------------------------------------
void CWorld_ServerCore::Registry_ClearWorld()
{
	MAUTOSTRIP(CWorld_ServerCore_Registry_ClearWorld, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ServerCore::Registry_ClearWorld, WORLD_SERVER);
	m_spWorldReg = NULL;
	Registry_GetServer()->DeleteDir("WORLD");
	m_spWorldReg = Registry_GetServer()->CreateDir("WORLD");

	//m_NextGUID = 1;
}

void CWorld_ServerCore::Registry_OnReadWorldKeys()
{
	MAUTOSTRIP(CWorld_ServerCore_Registry_OnReadWorldKeys, MAUTOSTRIP_VOID);
	m_NextGUID = m_spWorldReg->GetValuei("NEXTGUID", 1);
	m_WorldName = m_spWorldReg->GetValue("WORLD", "$$$NoWorldName$$$");
}

void CWorld_ServerCore::Registry_OnWriteWorldKeys()
{
	MAUTOSTRIP(CWorld_ServerCore_Registry_OnWriteWorldKeys, MAUTOSTRIP_VOID);
	m_spWorldReg->SetValue("WORLD", (char*)m_WorldName);
	m_spWorldReg->SetValuei("NEXTGUID", m_NextGUID);
}

CRegistry* CWorld_ServerCore::Registry_GetGame()
{
	MAUTOSTRIP(CWorld_ServerCore_Registry_GetGame, NULL);
	if (m_spGameReg == NULL) Error("Registry_GetGame", "No game-registry created.");
	return m_spGameReg;
}

CRegistry* CWorld_ServerCore::Registry_GetServer()
{
	MAUTOSTRIP(CWorld_ServerCore_Registry_GetServer, NULL);
	if (m_spServerReg == NULL) Error("Registry_GetGame", "No server-registry created.");
	return m_spServerReg;
}

CRegistry* CWorld_ServerCore::Registry_GetWorld()
{
	MAUTOSTRIP(CWorld_ServerCore_Registry_GetWorld, NULL);
	if (m_spWorldReg == NULL) Error("Registry_GetGame", "No world-registry created.");
	return m_spWorldReg;
}

CRegistry* CWorld_ServerCore::Registry_GetTemplates()
{
	MAUTOSTRIP(CWorld_ServerCore_Registry_GetTemplates, NULL);
	if(m_spTemplateReg == NULL) Error("Registry_GetTemplates", "No template-registry created.");
	return m_spTemplateReg;
}

CRegistry* CWorld_ServerCore::Registry_GetClientVar(int _iClient)
{
	MAUTOSTRIP(CWorld_ServerCore_Registry_GetClientVar, NULL);
	if (!m_lspClientInfo.ValidPos(_iClient)) return NULL;

	CWServer_ClientInfo* pCI = m_lspClientInfo[_iClient];
	if (!pCI) return NULL;

	if (!pCI->m_spClientReg)
	{
		pCI->m_spClientReg = REGISTRY_CREATE;
		if(!pCI->m_spClientReg)
			MemError("Registry_GetClientVar");
	}

	return pCI->m_spClientReg;
}

CRegistry* CWorld_ServerCore::Registry_GetClientRoot(int _iClient)
{
	MAUTOSTRIP(CWorld_ServerCore_Registry_GetClientRoot, NULL);
	if (!m_lspClientInfo.ValidPos(_iClient)) return NULL;

	CWServer_ClientInfo* pCI = m_lspClientInfo[_iClient];
	if (!pCI->m_spNetReg) Error("Registry_GetClientRoot", "NULL registry.");
	return pCI->m_spNetReg;
}

CRegistry* CWorld_ServerCore::Registry_GetClient(int _iClient, int _iDirectory)
{
	MAUTOSTRIP(CWorld_ServerCore_Registry_GetClient, NULL);
	if (!m_lspClientInfo.ValidPos(_iClient)) return NULL;

	CWServer_ClientInfo* pCI = m_lspClientInfo[_iClient];
	if(!pCI->m_spNetReg)
		return NULL;
	
	if (_iDirectory >= pCI->m_spNetReg->GetNumChildren())
		Client_SetNumRegDirectories(_iClient, _iDirectory+1);
	if (_iDirectory < 0) return NULL;
	return pCI->m_spNetReg->GetChild(_iDirectory);

/*	if (!pCI->m_lspNetReg.ValidPos(_iDirectory))
		Error("Registry_GetClient", "Failed to init client-registries.");
	return pCI->m_lspNetReg[_iDirectory];*/
}


CRegistry* CWorld_ServerCore::Registry_GetLevelKeys(CStr _Section, CStr _Level)
{
	if (m_spCurLevelKeys)
	{
		if (_Section != "")
			return m_spCurLevelKeys->FindChild(_Section);
		else
			return m_spCurLevelKeys;
	}

	CRegistry *pLevelKeys = Registry_GetServer()->Find("LEVELKEYS");
	if (!pLevelKeys)
		return NULL;

	CRegistry* pBase = pLevelKeys->FindChild("BASE");
	if (pBase)
		m_spCurLevelKeys = pBase->Duplicate();
	else
		m_spCurLevelKeys = DNew(CRegistry_Dynamic) CRegistry_Dynamic;

	CStr Level = _Level.IsEmpty() ? m_WorldName : _Level;
	CRegistry* pLevel = pLevelKeys->FindChild(Level);
	if (!pLevel)
		pLevel = pLevelKeys->FindChild("default");

	if (pLevel)
	{
		// find correct script layer
		bool bOk = false;
		int nScriptLayers = pLevel->GetNumChildren();
		for (int i = 0; i < nScriptLayers; i++)
		{
			CRegistry *pReg = pLevel->GetChild(i)->Find("spawnmask");
			if (pReg && World_TestSpawnFlags(pReg->GetThisValue()))
			{
				m_spCurLevelKeys->CopyDir(pLevel->GetChild(i));
				bOk = true;
			}
		}

		// if no entries with "spawnmask" key were found, let's use the level root as a layer  (backwards compatible)
		if (!bOk)
			m_spCurLevelKeys->CopyDir(pLevel);
	}

	if (_Section.IsEmpty())
		return m_spCurLevelKeys;

	return m_spCurLevelKeys->FindChild(_Section);
}

// -------------------------------------------------------------------
void CWorld_ServerCore::World_SetModel(int _iModel)
{
	MAUTOSTRIP(CWorld_ServerCore_World_SetModel, MAUTOSTRIP_VOID);
	m_iWorldModel = _iModel;
	CXR_Model* m_pWorld = m_spMapData->GetResource_Model(m_iWorldModel);
	if (!m_pWorld) Error("World_SetModel", CStrF("Invalid scenegraph model. (Rc %d)", _iModel));
	
#ifndef SERVER_DISABLE_SCENEGRAPH
	m_pSceneGraph = m_pWorld->SceneGraph_GetInterface();
	if (!m_pSceneGraph)
		Error("World_SetModel", "Model was not a scene-graph model.");
	m_spSceneGraphInstance = m_pSceneGraph->SceneGraph_CreateInstance();
	if (!m_spSceneGraphInstance)
		Error("World_SetModel", "Unable to create scene graph instance.");
	m_spSceneGraphInstance->Create(m_lspObjects.Len(), 64);		// 64 dynamic lights
	m_SceneGraphDeferredLinkage.Create(m_lspObjects.Len());
#endif
//	LogFile(CStrF("(CWorld_ServerCore::World_SetModel) Model %d, hSceneGraph %d", _iModel, m_hSceneGraph));
}

CXR_Model* CWorld_ServerCore::World_GetModel()
{
	MAUTOSTRIP(CWorld_ServerCore_World_GetModel, NULL);
	if (m_iWorldModel < 0) return NULL;
	return(m_spMapData->GetResource_Model(m_iWorldModel));
};

// -------------------------------------------------------------------
// Navigation services
// -------------------------------------------------------------------
CXR_PathInterface* CWorld_ServerCore::Path_GetInterface(int _iObj)
{
	MAUTOSTRIP(CWorld_ServerCore_Path_GetInterface, NULL);
	// iObj might be used later to determine which world should be used for navigation, should there be 
	// several world-models in this server context.

	if (m_iWorldModel < 0) return NULL;
	return m_spMapData->GetResource_Model(m_iWorldModel)->Path_GetInterface();
}

CXR_BlockNav* CWorld_ServerCore::Path_GetBlockNav()
{
	MAUTOSTRIP(CWorld_ServerCore_Path_GetBlockNav, NULL);
	return m_spBlockNav;
}

CXR_BlockNavSearcher* CWorld_ServerCore::Path_GetBlockNavSearcher()
{
	MAUTOSTRIP(CWorld_ServerCore_Path_GetBlockNavSearcher, NULL);
	return m_spBlockNavSearcher;
}

CWorld_Navgraph_Pathfinder * CWorld_ServerCore::Path_GetNavGraph()
{
	MAUTOSTRIP(CWorld_ServerCore_Path_GetNavGraph, NULL);
	return m_spGraphPathfinder;
};


// -------------------------------------------------------------------
//  Selection management
// -------------------------------------------------------------------
void CWorld_ServerCore::Selection_AddTarget(CSelection& _selection, uint32 _NameHash)
{
	MAUTOSTRIP(CWorld_ServerCore_Selection_AddTarget, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_ServerCore::Selection_AddTarget);
	int bCheckObj = _selection.GetNumElements();

	if (!_NameHash)
		return;

	CTargetNameElement Elem;
	Elem.m_NameHash = _NameHash;

#ifdef	SERVER_STATS
	CMTime Time; TStart(Time);
#endif	// SERVER_STATS

/*	if (m_NameSearchTree.m_Tree.GetRoot())
	{
		LogFile("-------------------------------------------------------------------");

		DIdsTreeAVLAligned_Iterator(CTargetNameElement, m_NameLink, CTargetNameElement, CTargetNameElement::CCompare) Iter = m_NameSearchTree.m_Tree;
		int iElem = 0;
		while(Iter)
		{
			CTargetNameElement* pNode = Iter;

			CWObject* pObj = pNode->m_pObj;
			if (pObj)
			{
				LogFile(CStrF("%.5d, 0x%.8x, pObj %.8x, %s", iElem, pNode->m_NameHash, pObj, pObj->GetName()));

			}

			iElem++;
			++Iter;
		}
	}*/

	if (m_NameSearchTree.m_Tree.GetRoot())
	{
		DIdsTreeAVLAligned_Iterator(CTargetNameElement, m_NameLink, CTargetNameElement, CTargetNameElement::CCompare) Iter;
		Iter.SetRoot(m_NameSearchTree.m_Tree);
		Iter.FindSmallestGreaterThanEqualForward(Elem);
		while(Iter)
		{
			CTargetNameElement* pNode = Iter;
			if (pNode->m_NameHash != Elem.m_NameHash)
				break;

			CWObject* pObj = pNode->m_pObj;
			if (pObj)
			{
	//			if (pObj) ConOut(CStrF("Sel_AddTrgt %s, %s", _pName, pObj->GetName()));

				if (bCheckObj) 
					Selection_AddObject(_selection, pObj->m_iObject);
				else
					_selection.AddData(pObj->m_iObject);
			}

			++Iter;
		}
	}


/*	CTargetNameTreeNode* pNode = m_NameSearchTree.Find(Elem);

	while (pNode)
	{
		if (pNode->GetElement().m_NameHash != Elem.m_NameHash)
			break;

		CWObject* pObj = pNode->GetElement().m_pObj;
		if (pObj)
		{
//			if (pObj) ConOut(CStrF("Sel_AddTrgt %s, %s", _pName, pObj->GetName()));

			if (bCheckObj) 
				Selection_AddObject(_iSelection, pObj->m_iObject);
			else
				m_lSelectionStack[_iSelection][m_lSelectNumElems[_iSelection]++] = pObj->m_iObject;
		}

		pNode = pNode->GetChild(1);
	}*/

#ifdef	SERVER_STATS
	TStop(Time); m_TFunc_Selection += Time;  m_nFunc_Selection++;
#endif	// SERVER_STATS
}

void CWorld_ServerCore::Selection_AddTarget(CSelection& _Selection, const char* _pName)
{
	MAUTOSTRIP(CWorld_ServerCore_Selection_AddTarget, MAUTOSTRIP_VOID);
	if (!_pName)
		return;

	ThisClass::Selection_AddTarget(_Selection, StringToHash(_pName));
}

int CWorld_ServerCore::Selection_GetSingleTarget(uint32 _NameHash)
{
	MSCOPESHORT(CWorld_ServerCore::Selection_GetSingleTarget);
	MAUTOSTRIP(CWorld_ServerCore_Selection_GetSingleTarget, 0);
	int iObj = 0;
	TSelection<CSelection::LARGE_BUFFER> Selection;
	ThisClass::Selection_AddTarget(Selection, _NameHash);
	const int16* pRet;
	int nRet =  Selection_Get(Selection, &pRet);
	if (nRet) iObj = pRet[Min(int(Random*nRet), nRet-1)];
	return iObj;
}

int CWorld_ServerCore::Selection_GetSingleTarget(const char* _pName)
{
	return ThisClass::Selection_GetSingleTarget(StringToHash(_pName));
}

void CWorld_ServerCore::Selection_AddTransitZone(CSelection& _Selection, int _iTZObject)
{
/*	MAUTOSTRIP(CWorld_ServerCore_Selection_AddTransitZone, MAUTOSTRIP_VOID);
	int bCheckObj = m_lSelectNumElems[_iSelection];

	int nObj = m_lspObjects.Len();
	for(int iObj = 0; iObj < nObj; iObj++)
		if (m_lspObjects[iObj] != NULL)
			if (m_lspObjects[iObj]->m_iTZObject == _iTZObject)
			{
				if (bCheckObj)
					Selection_AddObject(_iSelection, iObj);
				else
					m_lSelectionStack[_iSelection][m_lSelectNumElems[_iSelection]++] = iObj;
			}*/
}


// -------------------------------------------------------------------
void CWorld_ServerCore::Selection_RemoveOnTarget(CSelection& _Selection, CStr _TargetName)
{
	MAUTOSTRIP(CWorld_ServerCore_Selection_RemoveOnTarget, MAUTOSTRIP_VOID);
	int16* piSel = _Selection.GetData();
	int nSel = _Selection.GetNumElements();
	int nSelNew = 0;
	for(int i = 0; i < nSel; i++)
	{
		const CWObject* pObj = ThisClass::Object_Get(piSel[i]);
		if (!(pObj && (_TargetName.CompareNoCase(pObj->GetName()) == 0)))
			piSel[nSelNew++] = piSel[i];
	}
	_Selection.SetNumElements(nSelNew);
}

void CWorld_ServerCore::Selection_RemoveOnNotTarget(CSelection& _Selection, CStr _TargetName)
{
	MAUTOSTRIP(CWorld_ServerCore_Selection_RemoveOnNotTarget, MAUTOSTRIP_VOID);
	int16* piSel = _Selection.GetData();
	int nSel = _Selection.GetNumElements();
	int nSelNew = 0;
	for(int i = 0; i < nSel; i++)
	{
		const CWObject* pObj = ThisClass::Object_Get(piSel[i]);
		if (pObj && (_TargetName.CompareNoCase(pObj->GetName()) == 0))
			piSel[nSelNew++] = piSel[i];
	}

	_Selection.SetNumElements(nSelNew);
}

// -------------------------------------------------------------------
//  Message management
// -------------------------------------------------------------------
aint CWorld_ServerCore::Message_SendToObject(const CWObject_Message& _Msg, int _iObj)
{
	MAUTOSTRIP(CWorld_ServerCore_Message_SendToObject, 0);
	MSCOPESHORT(CWorld_ServerCore::Message_SendToObject); //AR-SCOPE

	CWObject* pObj = ThisClass::Object_Get(_iObj);
	if (pObj) return pObj->OnMessage(_Msg);
	return 0;
}

aint CWorld_ServerCore::Message_SendToObjectGUID(const CWObject_Message& _Msg, int _GUID)
{
	MAUTOSTRIP(CWorld_ServerCore_Message_SendToObjectGUID, 0);
	CWObject* pObj = ThisClass::Object_GetWithGUID(_GUID);
	if (pObj) return pObj->OnMessage(_Msg);
	return 0;
}

void CWorld_ServerCore::Message_SendToTarget(const CWObject_Message& _Msg, uint32 _NameHash)
{
	MAUTOSTRIP(CWorld_ServerCore_Message_SendToTarget, MAUTOSTRIP_VOID);
	TSelection<CSelection::LARGE_BUFFER> Selection;
	{
		ThisClass::Selection_AddTarget(Selection, _NameHash);
		ThisClass::Message_SendToSelection(_Msg, Selection);
	}
}

void CWorld_ServerCore::Message_SendToTarget(const CWObject_Message& _Msg, const char* _pName)
{
	ThisClass::Message_SendToTarget(_Msg, StringToHash(_pName));
}

void CWorld_ServerCore::Message_SendToSelection(const CWObject_Message& _Msg, const CSelection& _Selection)
{
	MAUTOSTRIP(CWorld_ServerCore_Message_SendToSelection, MAUTOSTRIP_VOID);
	int nElem = _Selection.GetNumElements();
	const int16* pSel = _Selection.GetData();
	for (int i = 0; i < nElem; i++)
		ThisClass::Message_SendToObject(_Msg, pSel[i]);
}

void CWorld_ServerCore::Phys_Message_SendToSelection(const CWObject_Message& _Msg, const CSelection& _Sel)
{
	ThisClass::Message_SendToSelection(_Msg, _Sel);
}


// -------------------------------------------------------------------
bool CWorld_ServerCore::MessageQueue_SendToObject(const CWObject_Message& _Msg, int _iObj)
{
	MAUTOSTRIP(CWorld_ServerCore_MessageQueue_SendToObject, false);
	return m_spMsgQueue->Put(CMsgQueueElement(_Msg, _iObj));
}

void CWorld_ServerCore::MessageQueue_Flush()
{
	MAUTOSTRIP(CWorld_ServerCore_MessageQueue_Flush, MAUTOSTRIP_VOID);
	while(!m_spMsgQueue->Empty())
	{
		CMsgQueueElement Elem = m_spMsgQueue->Get();
		ThisClass::Message_SendToObject(Elem.m_Msg, Elem.m_iDest);
	}
}

// -------------------------------------------------------------------
//  Sound services
// -------------------------------------------------------------------
void CWorld_ServerCore::Sound_Global(int _iSound, fp32 _Volume, int _iClient)
{
	Sound_Play(-1, CVec3Dfp32(0), _iSound, WCLIENT_ATTENUATION_2D, 0, _Volume, _iClient);
}

void CWorld_ServerCore::Sound_GlobalCutscene(int _iSound, fp32 _Volume, int _iClient, fp32 _Delay, uint32 _GroupID)
{
	Sound_Play(-1, CVec3Dfp32(0), _iSound, WCLIENT_ATTENUATION_2D_CUTSCENE, 0, _Volume, _iClient, CVec3Dfp32(0), _Delay, -1, _GroupID);

}

void CWorld_ServerCore::Sound_At(const CVec3Dfp32& _Pos, int _iSound, int _AttnType, uint8 _iMaterial, fp32 _Volume,
								int _iClient, CVec3Dfp32 _V0)
{
	Sound_Play(-1, _Pos, _iSound, _AttnType, _iMaterial, _Volume, _iClient, _V0);
}

void CWorld_ServerCore::Sound_On(int16 _iObject, int _iSound, int _AttnType, uint8 _iMaterial, fp32 _Volume, int _iClient, const CVec3Dfp32& _V0, const CVec3Dfp32& _Offset, uint32 _GroupID)
{
	Sound_Play(_iObject, _Offset, _iSound, _AttnType, _iMaterial, _Volume, _iClient, _V0, 0.0f, -1, _GroupID);
}

void CWorld_ServerCore::Sound_Off(int16 _iObject, int _iSound, uint8 _iMaterial, int _iClient)
{
	CNetMsg Msg(WPACKET_SOUND);
	Msg.AddInt16(_iSound);
	Msg.AddInt16(_iObject);
	Msg.AddInt8(_iMaterial);
	Msg.AddInt8(WCLIENT_ATTENUATION_OFF);	// Attenuation == -1, indicates that we want the sound removed

	if (_iClient == -1)
	{
		for(int32 i = 0; i < m_lspClientInfo.Len(); i++)
			if (m_lspClientInfo[i] != NULL && m_lspClientInfo[i]->m_State == WCLIENT_STATE_INGAME)
				Net_PutMsg(i, Msg);
	}
	else
	{
		Net_PutMsg(_iClient, Msg);
	}
}

void CWorld_ServerCore::Sound_Play(int16 _iObject, const CVec3Dfp32& _Pos, int _iSound, int _AttnType,
                                   uint8 _iMaterial, fp32 _Volume, int _iClient, const CVec3Dfp32 &_V0, 
                                   fp32 _Delay, int16 _iNotClient, uint32 _GroupID)
{
	if (m_bSkipSound)
		return;
	
	CNetMsg Msg(WPACKET_SOUND);
	Msg.AddInt16(_iSound);
	Msg.AddInt16(_iObject);
	Msg.AddInt8(_iMaterial);
	Msg.AddInt8(_AttnType);

	if (_AttnType != WCLIENT_ATTENUATION_OFF)
	{
		Msg.AddInt16(int16(Max(0.0f, Min(16.0f, _Volume))*2047.0f));
		Msg.AddVecInt16_Max32768(_Pos);

		if(_AttnType == WCLIENT_ATTENUATION_LRP)
			Msg.AddVecInt16_Max32768(_V0);
		else if(_AttnType == WCLIENT_ATTENUATION_2D_CUTSCENE)
			Msg.Addfp32(_Delay);
		else if(_AttnType == WCLIENT_ATTENUATION_3D_OVERRIDE)
		{
			M_TRACEALWAYS(CFStrF("CWorld_ServerCore::Sound_Play(WCLIENT_ATTENUATION_3D_OVERRIDE), _iObject(%d), _iSound(%d)\n", _iObject, _iSound));
			Msg.AddVecInt16_Max32768(_V0);
		}

		Msg.AddInt32(_GroupID);			// TODO: add some flags to make this optional  (use bits from _iSound or _iObject...)
	}


	uint32 ClientsToWaitFor = 0; // (wait for max 32 clients...)
	TAP<spCWServer_ClientInfo> pClients = m_lspClientInfo;

	for (int i = 0; i < pClients.Len(); i++)
	{
		if (i == _iNotClient)
			continue;

		if ((i == _iClient) || (_iClient == -1 && pClients[i] != NULL && pClients[i]->m_State == WCLIENT_STATE_INGAME))
		{
			ThisClass::Net_PutMsg(i, Msg);
			ClientsToWaitFor |= M_BitD(i);
		}
	}

	// Notify sync object about which clients to wait for
	if (_GroupID != 0)
	{
		int iSyncObj = ThisClass::Selection_GetSingleTarget(_GroupID);
		if (iSyncObj > 0)
			ThisClass::Message_SendToObject(CWObject_Message(OBJSYSMSG_SOUNDSYNC, 0, ClientsToWaitFor), iSyncObj);
	}
}


void CWorld_ServerCore::Sound_GlobalNotClient(int _iClient, int _iSound, fp32 _Volume)
{
	Sound_Play(-1, CVec3Dfp32(0), _iSound, WCLIENT_ATTENUATION_2D, 0, _Volume,-1,CVec3Dfp32(0.0f),_iClient);
}

void CWorld_ServerCore::Sound_GlobalCutsceneNotClient(int _iClient, int _iSound, fp32 _Volume, fp32 _Delay)
{
	Sound_Play(-1, CVec3Dfp32(0), _iSound, WCLIENT_ATTENUATION_2D_CUTSCENE, 0, _Volume, -1,CVec3Dfp32(0.0f), _Delay,_iClient);

}

void CWorld_ServerCore::Sound_AtNotClient(int _iClient, const CVec3Dfp32& _Pos, int _iSound, int _AttnType, uint8 _iMaterial, fp32 _Volume,
								 CVec3Dfp32 _V0)
{
	Sound_Play(-1, _Pos, _iSound, _AttnType, _iMaterial, _Volume, -1, _V0,0.0f, _iClient);
}

void CWorld_ServerCore::Sound_OnNotClient(int _iClient, int16 _iObject, int _iSound, int _AttnType, uint8 _iMaterial, fp32 _Volume, const CVec3Dfp32& _V0, const CVec3Dfp32& _Offset)
{
	Sound_Play(_iObject, _Offset, _iSound, _AttnType, _iMaterial, _Volume, -1,_V0),0.0f, _iClient;
}

void CWorld_ServerCore::Sound_OffNotClient(int _iClient, int16 _iObject, int _iSound, uint8 _iMaterial)
{
	CNetMsg Msg(WPACKET_SOUND);
	Msg.AddInt16(_iSound);
	Msg.AddInt16(_iObject);
	Msg.AddInt8(_iMaterial);
	Msg.AddInt8(WCLIENT_ATTENUATION_OFF); // Attenuation == -1, indicates that we want the sound removed

	for(int32 i = 0; i < m_lspClientInfo.Len(); i++)
		if (_iClient != i && m_lspClientInfo[i] != NULL && m_lspClientInfo[i]->m_State == WCLIENT_STATE_INGAME)
			Net_PutMsg(i, Msg);
}

//
//
void CWorld_ServerCore::Render(CXR_VBManager* _pVBM, const CMat4Dfp32& _Camera, int _Context)
{
	MAUTOSTRIP(CWorld_ServerCore_Render, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_ServerCore::Render); //AR-SCOPE
	if (_Context)
		return;

#ifndef DEF_DISABLE_PERFGRAPH
	if (!m_spCapture && !m_spWireContainer && !m_PerfGraph) return;
#else
	if (!m_spCapture && !m_spWireContainer) return;
#endif

	_pVBM->ScopeBegin("CWorld_ServerCore::Render", false);
	CMat4Dfp32 VMat;
	_Camera.InverseOrthogonal(VMat);
	{

		if (m_spCapture != NULL)
		{
//			CCaptureBuffer* pCapture = m_spCapture->GetCaptureBuffer();
			CRC_Viewport DummyVP;
//			pCapture->Render(_pRC, &DummyVP);
		}

		if (m_spWireContainer != NULL)
			m_spWireContainer->Render(_pVBM, VMat);

#ifndef DEF_DISABLE_PERFGRAPH
		if (m_PerfGraph)
		{
			CRC_ClipVolume Clip;
			_pVBM->Viewport_Get()->GetClipVolume(Clip, &_Camera);

			CRC_Font* pFont = m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("MONOPRO"));
			if (pFont)
			{
				CMat4Dfp32 VMat;
				const CMat4Dfp32& IVMat = _Camera;
				_Camera.InverseOrthogonal(VMat);

	//			fp64 FreqRecp = 1.0f / GetCPUFrequency();
				for(int iObj = 0; iObj < m_PerfGraph_lObjects.Len(); iObj++)
				{
					CWS_ObjectPerfGraph& ObjPerf = m_PerfGraph_lObjects[iObj];

//					if (ObjPerf.m_LastTouch_OnClientRender != m_PerfGraph_Tick_OnClientRender)
//						continue;

					CWObject_CoreData* pObj = Object_GetCD(iObj);
					if (!pObj) continue;

					if (CVec3Dfp32::GetRow(_Camera, 3).DistanceSqr(pObj->GetPosition()) > Sqr(512))
						continue;

					CBox3Dfp32 VisBox;
					pObj->GetAbsVisBoundBox(VisBox);

					if (!Clip.BoxInVolume(VisBox))
						continue;


					int Col = 0x6fff8000;
					CFStr St;
					if (m_PerfGraph & 2)
						St = CFStrF("%i: %.0f", iObj, ObjPerf.m_Time_OnRefresh.GetTime() * 1000000.0);
					else
						St = CFStrF("%.0f", ObjPerf.m_Time_OnRefresh.GetTime() * 1000000.0);
	//				CFStr St = CFStrF("%4s", St2.Str());

					CVec2Dfp32 V(8, 8);
					CVec3Dfp32 Down = CVec3Dfp32::GetRow(IVMat, 1);//(0, 0, -1);
					CVec3Dfp32 Dir = CVec3Dfp32::GetRow(IVMat, 0);
					CBox3Dfp32 BoundW;
					CVec3Dfp32 Pos;
					
					pObj->GetAbsVisBoundBox(BoundW);

					BoundW.m_Min.Lerp(BoundW.m_Max, 0.5, Pos);
					Pos[2] = BoundW.m_Max[2] + 8.0f;
					if (pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER)
						Pos[2] += 32;

					Pos.Combine(Dir, -16, Pos);

					CXR_VertexBuffer* pVB = _pVBM->Alloc_VB(CXR_VB_ATTRIB);
					if(!pVB)
						break;	// No point in continuing since VBM is out of memory
					pVB->m_pTransform = _pVBM->Alloc_M4(VMat);
					if(!pVB->m_pTransform)
						break;	// No point in continuing since VBM is out of memory
					
					pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
					pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
					if (pFont->Write(_pVBM, pVB, Pos, Dir, Down, St.Str(), V, Col))
						_pVBM->AddVB(pVB);

					if (CVec3Dfp32::GetRow(_Camera, 3).DistanceSqr(Pos) < Sqr(384))
					{
						CMat4Dfp32 WMat;
						WMat.Unit();
						Pos.SetRow(WMat, 3);

						CMat4Dfp32 L2VMat;
						WMat.Multiply(VMat, L2VMat);
						L2VMat.Unit3x3();

						CXR_VertexBuffer* pVB = ObjPerf.m_spGraph_OnRefresh->Render(_pVBM, CVec2Dfp32(0, -16), L2VMat);
						if (pVB)
							_pVBM->AddVB(pVB);
					}
				}
			}
		}
#endif
	}
	_pVBM->ScopeEnd();
}

CRenderContext* CWorld_ServerCore::Debug_GetRender()
{
	MAUTOSTRIP(CWorld_ServerCore_Debug_GetRender, NULL);
	return m_spCapture;
}

CDebugRenderContainer* CWorld_ServerCore::Debug_GetWireContainer(uint _Flags)
{
	MAUTOSTRIP(CWorld_ServerCore_Debug_GetWireContainer, NULL);
	if (m_PhysRenderFlags & _Flags)
		return m_spWireContainer;
	return NULL;
}


void CWorld_ServerCore::SetGameTick(int _GameTick)
{
	m_SimulationTick = _GameTick;
	m_SimulationTime = CMTime::CreateFromTicks(m_SimulationTick, GetGameTickTime());
}
