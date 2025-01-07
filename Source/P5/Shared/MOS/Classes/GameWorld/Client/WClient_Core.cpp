#include "PCH.h"
#include "WClient_Core.h"
#include "../WPhysState_Hash.h"
#include "../WPackets.h"
#include "../WMapData.h"
#include "MFloat.h"
#include "../../Render/MRenderCapture.h"
#include "../WObjects/WObj_Game.h"
#include "../../../MSystem/Misc/MPerfGraph.h"
#include "../../../XRModels/Model_TriMesh/WTriMeshDC.h"
#include "../WDataRes_XW.h"

#ifdef MRTC_MEMORYDEBUG
	#define CHECKMEMORY(s) { if (!_CrtCheckMemory()) Error(s, "Memory check failure."); }
#else
	#define CHECKMEMORY(s)
#endif

#if defined(PLATFORM_DOLPHIN)
	#include "../../../MSystem/Sound/dolphin/MSnd_Dolphin.h"
	#define DEF_DISABLE_PERFGRAPH //AR-TEMP
	extern void GameCube_DestroyStuff();
#elif defined(PLATFORM_PS2)
	extern void PS2_DestroyStuff();
#endif



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWC_ObjectPerfGraph
|__________________________________________________________________________________________________
\*************************************************************************************************/
#ifndef DEF_DISABLE_PERFGRAPH

CWC_ObjectPerfGraph::CWC_ObjectPerfGraph()
{
	MAUTOSTRIP(CWC_ObjectPerfGraph_ctor, MAUTOSTRIP_VOID);
}

void CWC_ObjectPerfGraph::Create()
{
	MAUTOSTRIP(CWC_ObjectPerfGraph_Create, MAUTOSTRIP_VOID);
#ifndef	DEF_DISABLE_PERFGRAPH
	m_spGraph_OnClientRender = (CPerfGraph*) MRTC_GOM()->CreateObject("CPerfGraph");
	m_spGraph_OnClientRefresh = (CPerfGraph*) MRTC_GOM()->CreateObject("CPerfGraph");
	if (!m_spGraph_OnClientRender || !m_spGraph_OnClientRefresh)
		Error("Create", "Could not create perf-graphs.");
	m_spGraph_OnClientRender->Create(32, 0, 0.001f, 16, 16, 2);			// -500..500ms
	m_spGraph_OnClientRefresh->Create(32, 0, 0.001f, 16, 16, 2);			// -500..500ms
#else
	m_spGraph_OnClientRender	= NULL;
	m_spGraph_OnClientRefresh	= NULL;
#endif

	m_LastTouch_OnClientRender = 0;
	m_LastTouch_OnClientRefresh = 0;
	m_Time_OnClientRender.Reset();
	m_Time_OnClientRefresh.Reset();
}

#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWC_SoundSyncGroups
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWC_SoundSyncGroups::SGroup::SGroup(uint32 _GroupID)
	: m_GroupID(_GroupID)
	, m_lVoices()
	, m_bReadyToGo(false)
{ }

int CWC_SoundSyncGroups::Find(uint32 _GroupID) const
{
	TAP<const SGroup> pGroups = m_lGroups;
	for (int i = 0; i < pGroups.Len(); i++)
		if (pGroups[i].m_GroupID == _GroupID)
			return i;
	return -1;
}

CWC_SoundSyncGroups::SGroup& CWC_SoundSyncGroups::Get(uint32 _GroupID)
{
	int iGroup = Find(_GroupID);
	if (iGroup < 0)
		iGroup = m_lGroups.Add( SGroup(_GroupID) );
	return m_lGroups[iGroup];
}

void CWC_SoundSyncGroups::Remove(uint32 _GroupID)
{
	int iGroup = Find(_GroupID);
	if (iGroup >= 0)
		m_lGroups.Del(iGroup);
}

TAP<CWC_SoundSyncGroups::SGroup> CWC_SoundSyncGroups::GetAll()
{
	return TAP<SGroup>(m_lGroups);
}





/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWorld_ClientCore
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_DYNAMIC(CWorld_ClientCore, CWorld_Client);

CWorld_ClientCore::CWorld_ClientCore()
{
	MAUTOSTRIP(CWorld_ClientCore_ctor, MAUTOSTRIP_VOID);

	m_Render_CharacterShadows = 1;
	m_pClientRef = NULL;
	m_ClientMode = 0;
	m_ClientState = WCLIENT_STATE_VOID;
	m_ChangeLevelState = 0;
	m_LastRcvRc = 0;
	m_ClientDrawDisable = 0;
	m_bShowObjPhys = 0;
	m_ShowGraphs = 0;
	m_bPhysRender = 0;
	m_HeadLightRange = 512;
	m_ForcedHeadLightRange = 0;
	m_HeadLightIntensity = 0;
	m_LastRenderCamera.Unit();

	m_MaxRecurseDepth = 2;

	m_Precache = false;
	m_Precache_iItem = 0;
	m_Precache_nItems = 0;
	m_Precache_nItemsDone = 0;
	m_Precache_ResourceID = 0;
	m_PrecacheProgress = 0;
	m_LoadStatus = 0;

	m_RecMode = WCLIENT_RECMODE_NORMAL;
	{ for(int i = 0; i < WCLIENT_NUMCHANNELS; i++) m_hChannels[i] = -1; }

	m_hConnection = -1;

//	m_LastCmdTime = 0;
	m_PauseTime.Reset();

	m_ClientUpdateMask = WCLIENT_SERVERUPDATE_PLAYERINFO;

	m_SimulationTick = 0;
	m_TimeScale = 1.0;
//	m_RenderTime = 0;
	m_RenderTickFrac = 0;
	m_LocalFrameFraction = 0;

//	m_iLocalPlayer = -1;
	m_FramePeriod = GetGameTickTime();
	m_FramePeriodPrim = 0;

	m_bCaptureNext = false;

	m_bNetLoad = 0;
	m_bNetShowMsg = 0;
	m_NetLoadLastFrameAverage = 0;
	m_NetLoadLastSecondTotal = 0;
//	m_NetLoadSampleTime = 0;
	m_NetLoadSecondTotal = 0;
	m_NetLoadSecondTotal = 0;

	m_iWorldModel = 0;
	m_pWorld = NULL;
	m_pPhysicsPrim = NULL;

	m_iPlayerObj = -1;
	m_iGameObj = -1;

	m_bSound_PosSet = false;
	m_bSyncOnClientRender	= false;

	m_Sound_VolumeAmbient = 1.0f;
	m_Sound_VolumeSfx = 1.0f;
	m_Sound_VolumeVoice = 1.0f;
	m_Sound_VolumeGameMaster = 1.0f;
	m_Sound_VolumeSoundVolume = 1.0f;

	m_Sound_LastestNumHit = 0;

	m_pSceneGraph = NULL;


	m_hWallmarkContext = -1;
	m_pWallmarkInterface = NULL;

	m_CameraTweak.k[0] = 0.0f;
	m_CameraTweak.k[1] = 0.0f;
	m_CameraTweak.k[2] = 0.0f;

#ifndef DEF_DISABLE_PERFGRAPH
	m_PerfGraph = 0;
	m_PerfGraph_Tick_OnClientRender = 0;
	m_PerfGraph_Tick_OnClientRefresh = 0;
#endif

	Render_ResetCameraVelocity();

//ConOutL(CStrF("(CWorld_ClientCore::CWorld_Client) this %.8x", this));
}

void CWorld_ClientCore::Create(CWC_CreateInfo& _CreateInfo)
{
	m_spEngine = _CreateInfo.m_spEngine;

	MAUTOSTRIP(CWorld_ClientCore_Create, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ClientCore::Create, WORLD_CLIENT);

	m_ClientMode = _CreateInfo.m_Mode;
	m_ClientState = WCLIENT_STATE_VOID;
	m_pClientRef = _CreateInfo.m_pClientRef;
	if (m_pClientRef)
		m_ClientMode |= WCLIENT_MODE_REFERENCE;
	m_ClientPlayerNr = _CreateInfo.m_PlayerNr;

//ConOutL(CStrF("(CWorld_ClientCore::Create) Mode %.8x, ClientRef %.8x, ClientPlayerNr %d", m_ClientMode, m_pClientRef, m_ClientPlayerNr));
	if (!m_ClientPlayerNr)
	{
		m_CmdPrefix = "cl_";
		m_CmdPrefix_Controls = CStr(' ', 0);
	}
	else
	{
		m_CmdPrefix = CStrF("cl%d_", m_ClientPlayerNr+1);
		m_CmdPrefix_Controls = CStrF("pl%d_", m_ClientPlayerNr+1);
	}

	m_ChangeLevelState = 0;
	m_LastRcvRc = 0;

	m_spGameReg = _CreateInfo.m_spGameReg;
	m_spUserReg = _CreateInfo.m_spGameReg->CreateDir("USER");

	if (_CreateInfo.m_spMapData != NULL)
	{
		m_spMapData = _CreateInfo.m_spMapData;
		m_spMapData->SetState(WMAPDATA_STATE_NOCREATE);
		m_spWData = m_spMapData->m_spWData;
	}
	else
		m_spWData = _CreateInfo.m_spWData;

	// Create resource mapping for GUI.
	m_spGUIData = MNew(CMapData);
	if (!m_spGUIData) MemError("Create");
	m_spGUIData->Create(m_spWData);

	// Create client registry mirrors/server-replica.
	{
		m_spNetReg = REGISTRY_CREATE;
		if (!m_spNetReg) MemError("Create");
		m_spNetReg->SetNumChildren(WCLIENT_REG_NUM);
		for(int i = 0; i < WCLIENT_REG_NUM; i++)
			m_spNetReg->SetValue(i, CFStrF("NETREG%d", i).Str());
	}

	// Moved from CWorld_ClientCore::EngineClient_EnumerateView
	// we wanted to initialize this memory block early, and not during first render
	{
		const int MaxObj = 4096;
		m_liEnumObj.SetLen(MaxObj);
	}
	
	// Get environment registry pointer
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	m_pEnv = (pSys) ? pSys->GetEnvironment() : NULL;
	if (!m_pEnv) Error("Create", "No environment-registry found.");

	m_MaxRecurseDepth = MaxMT(1, m_pEnv->GetValuei("XR_MAXRECURSE", 4));

	// Read registry
	OnRegistryChange();

	// Register this client's commands?
	if (m_ClientMode & WCLIENT_MODE_PRIMARY)
		AddToConsole();

	if (m_ClientMode & WCLIENT_MODE_PRIMARY)
	{
#ifndef	DEF_DISABLE_PERFGRAPH
		m_lspGraphs.SetLen(WCLIENT_GRAPH_MAX);
		for(int i = 0; i < m_lspGraphs.Len(); i++)
		{
			CPerfGraph* NewGraph = (CPerfGraph*) MRTC_GOM()->CreateObject("CPerfGraph");
			m_lspGraphs[i] = NewGraph;

			if (!m_lspGraphs[i]) Error("Create", "Unable to create performance graph");
		}

		m_lspGraphs[WCLIENT_GRAPH_LAGTIME]->Create(64, -0.5, 0.5f, 64, 64);			// -500..500ms
		m_lspGraphs[WCLIENT_GRAPH_RENDERTIME]->Create(64, 0, 0.100f, 64, 64);		// 0..200 ms
		m_lspGraphs[WCLIENT_GRAPH_NETBANDWIDTH]->Create(64, 0, 500, 64, 64);		// 0..500 bytes
#ifdef PLATFORM_CONSOLE
		m_lspGraphs[WCLIENT_GRAPH_SERVERTIME]->Create(64, 0, 0.020f, 64, 128, 5);	// 0..20 ms
#else
		m_lspGraphs[WCLIENT_GRAPH_SERVERTIME]->Create(64, 0, 0.010f, 64, 64, 5);		// 0..10 ms
#endif
		m_lspGraphs[WCLIENT_GRAPH_CLIENTTIME]->Create(64, 0, 0.010f, 64, 64);		// 0..10 ms
		m_lspGraphs[WCLIENT_GRAPH_NETTIME]->Create(64, 0, 0.010f, 64, 64);			// 0..10 ms
		m_lspGraphs[WCLIENT_GRAPH_PREDICTTIME]->Create(64, 0, 0.010f, 64, 64);		// 0..10 ms
		m_lspGraphs[WCLIENT_GRAPH_SOUNDTIME]->Create(64, 0, 0.200f, 64, 64);			// 0..200 ms
#endif
	}

	pSys->System_Add(this);
}

CWorld_ClientCore::~CWorld_ClientCore()
{
	MAUTOSTRIP(CWorld_ClientCore_dtor, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ClientCore::~CWorld_ClientCore, WORLD_CLIENT);

//	M_TRY
//	{
		
		CDiskUtil::XDF_Stop();
#ifdef PLATFORM_CONSOLE
		if(m_spSound) 
			m_spSound->MultiStream_Stop(false);
#endif
		Sound_SetSoundContext(NULL);

		// Shut down
		MACRO_RemoveSubSystem(this);

		if (!(m_ClientMode & WCLIENT_MODE_MIRROR))
			ConOut("Destroying client.");
		World_Close();

		if (m_ClientMode & WCLIENT_MODE_PRIMARY)
			RemoveFromConsole();

		if (m_spWData)
			m_spWData->Resource_WorldCloseClient();


		if (m_RecMode == WCLIENT_RECMODE_RECORDING) 
			Demo_Stop();

		if ((m_spNetwork != NULL) && (m_hConnection >= 0))
		{
			LogFile("(CWorld_ClientCore::~CWorld_Client) Closing connection.");
			m_spNetwork->Connection_Close(m_hConnection);
			LogFile("(CWorld_ClientCore::~CWorld_Client) Ok.");
			m_hConnection = -1;
			m_spNetwork = NULL;
		}
		
		if (m_spWireContainer)
			MRTC_GOM()->UnregisterObject((CReferenceCount*)m_spWireContainer, "GAMECONTEXT.CLIENT.WIRECONTAINER");
		m_spWireContainer = NULL;
/*	}
	M_CATCH(
	catch(CCException)
	{
	}
	)*/
}

aint CWorld_ClientCore::OnMessage(const CSS_Msg& _Msg)
{
	MAUTOSTRIP(CWorld_ClientCore_OnMessage, 0);
	switch(_Msg.m_Msg)
	{
	case CSS_MSG_PRECHANGEDISPLAYMODE :
	case CSS_MSG_POSTCHANGEDISPLAYMODE :
	case CSS_MSG_PRECACHEHINT :
	case CSS_MSG_PRECACHEHINT_SOUNDS :
	case CSS_MSG_PRECACHEHINT_TEXTURES :
		{
			MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
			if(pSys->GetEnvironment()->GetValue("rs_preload_textures", "1").Val_int() == 1)
				Precache_Init();
		}
		return 0;

	default :
		return CSubSystem::OnMessage(_Msg);
	}
}

bool CWorld_ClientCore::Execute(CStr _Script)
{
	MAUTOSTRIP(CWorld_ClientCore_Execute, false);
	MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
	if (!pCon) return false;
	bool bException = false;
	M_TRY
	{
		pCon->ExecuteString(_Script);
	}
	M_CATCH(
	catch (CCException)
	{
		bException = true;
	}
	)
	return !bException;
}

int CWorld_ClientCore::GetSplitScreenMode()
{
	MAUTOSTRIP(CWorld_ClientCore_GetSplitScreenMode, 0);
	if (m_ClientState == WCLIENT_STATE_CHANGELEVEL || m_Precache)
		return WCLIENT_SPLITSCREEN_SHARE;
	else
		return WCLIENT_SPLITSCREEN_REQUIRED;
}

CStr CWorld_ClientCore::GetClientInfo()
{
	MAUTOSTRIP(CWorld_ClientCore_GetClientInfo, CStr());
	return CStrF("Client %s, S %d, CWS %d, Pl %s", (m_ClientMode & WCLIENT_MODE_PRIMARY) ? "PRIMARY" : "MIRROR", m_ClientState, m_ChangeLevelState, (char*)m_LocalPlayer.GetName());
}

bool CWorld_ClientCore::CanRenderGame()
{
	MAUTOSTRIP(CWorld_ClientCore_CanRenderGame, false);

	CWObject_CoreData* pObj = Object_GetCD(Player_GetLocalObject());
	if (!pObj) return false;

	bool bInGame = 
		(m_ClientState == WCLIENT_STATE_INGAME || (m_RecMode == WCLIENT_RECMODE_PLAYBACK)) &&
		(m_spMapData != NULL) &&
		m_lspObjects.Len();

	return bInGame && !m_Precache;
}

bool CWorld_ClientCore::CanRenderWorld()
{
	MAUTOSTRIP(CWorld_ClientCore_CanRenderWorld, false);
	// Returns wether the client has completed the connection sequence and is able to render the game.
	CWObject_CoreData* pObj = Object_GetCD(Player_GetLocalObject());
	if (!pObj) return false;

	bool bInGame = 
		(m_ClientState == WCLIENT_STATE_INGAME || (m_RecMode == WCLIENT_RECMODE_PLAYBACK)) &&
		(m_spMapData != NULL) &&
		m_lspObjects.Len();

//	return true;
	return bInGame;
}

uint32 CWorld_ClientCore::GetViewFlags()
{
	return 0;
}

int CWorld_ClientCore::GetInteractiveState()
{
	MAUTOSTRIP(CWorld_ClientCore_GetInteractiveState, 0);
	if (GetClientMode() & WCLIENT_MODE_GUI)
		return 0;
	if (GetClientState() != WCLIENT_STATE_INGAME)
		return 0;

	int iGameObj = Game_GetObjectIndex();
	if(iGameObj > 0 && ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_NEVERRENDERWORLD), iGameObj) != 0)
		return 0;

	return 1;
}

#ifdef M_Profile
CPerfGraph* CWorld_ClientCore::GetGraph(int _iGraph)
{
	MAUTOSTRIP(CWorld_ClientCore_GetGraph, NULL);
#ifdef	DEF_DISABLE_PERFGRAPH
	return NULL;
#else
	return (m_lspGraphs.ValidPos(_iGraph)) ? m_lspGraphs[_iGraph] : (TPtr<CPerfGraph>)NULL;
#endif
}

void CWorld_ClientCore::AddGraphPlot(int _iGraph, fp32 _Value, uint32 _Color)
{
	MAUTOSTRIP(CWorld_ClientCore_AddGraphPlot, MAUTOSTRIP_VOID);
#ifndef	DEF_DISABLE_PERFGRAPH
	MSCOPESHORT(CWorld_ClientCore::AddGraphPlot);
	if (m_lspGraphs.ValidPos(_iGraph))
		m_lspGraphs[_iGraph]->Plot(_Value, _Color);
#endif
}
#endif


CMTime CWorld_ClientCore::GetTime()
{
	MAUTOSTRIP(CWorld_ClientCore_GetTime, 0.0);
	CMTime Time;
	Time.Snapshot();
	return Time;
//	return CMTime::GetCPU();
}

/*
fp32 CWorld_ClientCore::GetTimeScale()
{
	MAUTOSTRIP(CWorld_ClientCore_GetTimeScale, 0.0);
	return m_TimeScale;
}

fp32 CWorld_ClientCore::GetGameTickRealTime()
{
	MAUTOSTRIP(CWorld_ClientCore_GetGameTickRealTime, 0.0);
	return GetGameTickTime() / m_TimeScale;
}

fp32 CWorld_ClientCore::GetGameTickTime()
{
	MAUTOSTRIP(CWorld_ClientCore_GetGameTickTime, 0.0);
//	return CWCLIENT_TIMEPERFRAME;
	return (1.0f / 20.0f);
}

int CWorld_ClientCore::GetGameTick()
{
	MAUTOSTRIP(CWorld_ClientCore_GetGameTick, 0);
	return m_SimulationTick;
}

CMTime CWorld_ClientCore::GetGameTime()
{
	MAUTOSTRIP(CWorld_ClientCore_GetGameTime, 0.0);
	
	return CMTime::CreateFromTicks(m_SimulationTick, GetGameTickTime());
}
*/
/*fp32 CWorld_ClientCore::GetRenderTickFrac()
{
	MAUTOSTRIP(CWorld_ClientCore_GetRenderTickFrac, 0.0f);
	if (m_RecMode != 2 && GetClientMode() & WCLIENT_MODE_LOCAL)
		return m_LocalFrameFraction;
	else
		return m_RenderTickFrac;
}*/

CMTime CWorld_ClientCore::GetRenderTime()
{
	MAUTOSTRIP(CWorld_ClientCore_GetRenderTime, 0.0);
	return m_RenderTime;
}

CMTime CWorld_ClientCore::GetLastSnapshotTime()
{
	if (m_ClientMode & WCLIENT_MODE_PAUSE)
		return m_PauseTime;
	else
		return m_LastUnpackTime;
};

CMTime CWorld_ClientCore::GetLastCmdTime()
{
	return m_LastCmdTime;
};

fp32 CWorld_ClientCore::GetInterpolateTime()
{
	MAUTOSTRIP(CWorld_ClientCore_GetInterpolateTime, 0.0f);
	return GetRenderTickFrac() * GetGameTickTime();
}

fp32 CWorld_ClientCore::GetModeratedFramePeriod()
{
	MAUTOSTRIP(CWorld_ClientCore_GetModeratedFramePeriod, 0.0);
	return m_FramePeriod;
}

// -------------------------------------------------------------------
/*void ConvMatrix(const CMat4Dfp32& _Src, CMat4Dfp64& _Dest);
void ConvMatrix(const CMat4Dfp32& _Src, CMat4Dfp64& _Dest)
{
	MAUTOSTRIP(ConvMatrix, MAUTOSTRIP_VOID);
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; j++)
			_Dest.k[i][j] = _Src.k[i][j];
}

void ConvMatrix(const CMat4Dfp64& _Src, CMat4Dfp32& _Dest);
void ConvMatrix(const CMat4Dfp64& _Src, CMat4Dfp32& _Dest)
{
	MAUTOSTRIP(ConvMatrix_2, MAUTOSTRIP_VOID);
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; j++)
			_Dest.k[i][j] = _Src.k[i][j];
}*/

// -------------------------------------------------------------------
CWC_Player* CWorld_ClientCore::Player_GetLocal()
{
	MAUTOSTRIP(CWorld_ClientCore_Player_GetLocal, NULL);
	return &m_LocalPlayer;
};

int CWorld_ClientCore::Player_GetLocalObject()
{
	MAUTOSTRIP(CWorld_ClientCore_Player_GetLocalObject, 0);
	return m_iPlayerObj;
/*
	CRegistry* pReg = Registry_GetNonConst(WCLIENT_REG_GAMESTATE);
	if (pReg)
	{
		return pReg->GetValuei("PLAYEROBJ", -1, 0);
	}
	else
		return -1;*/
}

// -------------------------------------------------------------------
int CWorld_ClientCore::Game_GetObjectIndex()
{
	MAUTOSTRIP(CWorld_ClientCore_Game_GetObjectIndex, 0);
	return m_iGameObj;
/*	CRegistry* pReg = Registry_GetNonConst(WCLIENT_REG_GAMESTATE);
	if (pReg)
	{
		return pReg->GetValuei("GAMEOBJ", -1, 0);
	}
	else
		return -1;*/
}

// -------------------------------------------------------------------
void CWorld_ClientCore::OnRegistryChange()
{
	MAUTOSTRIP(CWorld_ClientCore_OnRegistryChange, MAUTOSTRIP_VOID);
	if(m_pEnv)
	{
		m_bNetShowMsg = m_pEnv->GetValuei("CL_SHOWMSG", 0);
	}

	CRegistry* pReg = Registry_GetNonConst(WCLIENT_REG_GAMESTATE);
	m_iPlayerObj = pReg ? pReg->GetValuei("PLAYEROBJ", -1, 0) : -1;
	m_iGameObj = pReg ? pReg->GetValuei("GAMEOBJ", -1, 0) : -1;
}

int CWorld_ClientCore::Registry_GetNum()
{
	MAUTOSTRIP(CWorld_ClientCore_Registry_GetNum, 0);
	if(!m_spNetReg)
		return 0;

	return m_spNetReg->GetNumChildren();
}

CRegistry* CWorld_ClientCore::Registry_GetNonConst(int _iDirectory)
{
	MAUTOSTRIP(CWorld_ClientCore_Registry_GetNonConst, NULL);
	if (m_spNetReg != NULL && m_spNetReg->ValidChild(_iDirectory))
		return m_spNetReg->GetChild(_iDirectory);
	else
		return NULL;
}

const CRegistry* CWorld_ClientCore::Registry_Get(int _iDirectory)
{
	MAUTOSTRIP(CWorld_ClientCore_Registry_Get, NULL);
	if (m_spNetReg != NULL && m_spNetReg->ValidChild(_iDirectory))
		return m_spNetReg->GetChild(_iDirectory);
	else
		return NULL;
}

CRegistry* CWorld_ClientCore::Registry_GetUser()
{
	return m_spUserReg;
}

// Talbot
CRegistry* CWorld_ClientCore::Registry_GetEnvironment()
{
	return m_pEnv;
}

CRegistry* CWorld_ClientCore::Registry_GetGame()
{
	return m_spGameReg;
}


const CRegistry* CWorld_ClientCore::Registry_GetRoot()
{
	MAUTOSTRIP(CWorld_ClientCore_Registry_GetRoot, NULL);
	if (!m_spNetReg) Error("Registry_GetRoot", "NULL registry.");
	return m_spNetReg;
}

CRegistry* CWorld_ClientCore::Registry_GetRootNonConst()
{
	MAUTOSTRIP(CWorld_ClientCore_Registry_GetRootNonConst, NULL);
	if (!m_spNetReg) Error("Registry_GetRoot", "NULL registry.");
	return m_spNetReg;
}

// -------------------------------------------------------------------

aint CWorld_ClientCore::ClientMessage_SendToObject(const CWObject_Message& _Msg, int _iObj)
{
	MAUTOSTRIP(CWorld_ClientCore_ClientMessage_SendToObject, 0);
	MSCOPESHORT(CWorld_ClientCore::ClientMessage_SendToObject); //AR-SCOPE

	CWObject_Client* pObj = ThisClass::Object_Get(_iObj);
	if (!pObj) return 0;

	M_ASSERT(pObj->m_pRTC, "!");
	return pObj->m_pRTC->m_pfnOnClientMessage(pObj, this, _Msg);
}

// -------------------------------------------------------------------
void CWorld_ClientCore::World_Close()
{
	MAUTOSTRIP(CWorld_ClientCore_World_Close, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWorld_ClientCore::World_Close);
	
	m_spSceneGraphInstance = NULL;
	m_pSceneGraph = NULL;
	m_SceneGraphDeferredLinkage.Clear();

	if (m_hWallmarkContext >= 0 && m_pWallmarkInterface)
	{
		m_pWallmarkInterface->Wallmark_DestroyContext(m_hWallmarkContext);
		m_pWallmarkInterface = NULL;
		m_hWallmarkContext = -1;
	}
	m_spTMDC = NULL;

	if(!(m_ClientMode & WCLIENT_MODE_MIRROR))
	{
		Render_GetEngine()->Engine_ClearReferences();
	}

#ifdef PLATFORM_CONSOLE
	MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
	if(pCon)
	{
		// Clearing the console just to tidy up in the memory dump
		pCon->ClearHistory();
		pCon->ClearOutput();
		pCon->FreeTempAllocations();
	}
#endif

	Sound_KillVoices();

#ifdef PLATFORM_CONSOLE
	if(m_spSound) 
		m_spSound->MultiStream_Stop(false);
#endif

	ClientMessage_SendToObject(CWObject_Message(OBJSYSMSG_GAME_CLOSEWORLD), Game_GetObjectIndex());

	int i;
	for(i = 0; i < m_lspObjects.Len(); i++)
	{
		CWObject_Client *pObj = m_lspObjects[i];
		if (pObj && m_spSpaceEnum)
		{
			m_spSpaceEnum->Remove(i);		
			pObj->Clear();
		}
		m_lspObjects[i] = NULL;
	}
	m_lspObjects.Clear();
	m_spWorldSpawnKeys = NULL;

	m_iWorldModel = 0;
	m_pWorld = 0;

	m_spMapData = NULL;

	m_spClientObjectHeap = NULL;
	m_lspClientObjects.Clear();

	m_Util2D.ClearTempSurface();

	m_lspFrameIn.Clear();
	m_lFrameInTime.Clear();

	m_spSpaceEnum = NULL;
	m_lEnumSpace.Clear();

	m_iObjVisPool.Clear();

	m_iObjSoundPool.Clear();
	m_ObjPoolDynamics.Clear();

	m_spNetReg = NULL;

	// Clean up all soundvolumes
	Sound_Clean();

#if defined(PLATFORM_DOLPHIN)
	GameCube_DestroyStuff();
	MACRO_GetRegisterObject(CSoundContext_Dolphin, pSound, "SYSTEM.SOUND");
	if (pSound)
		pSound->DestroyAllSounds(true); // even streams

#elif defined(PLATFORM_PS2)
	PS2_DestroyStuff();
#endif
}

void CWorld_ClientCore::World_Init(int _nObj, int _nRc, CStr _WName, int32 _SpawnMask)
{
	MAUTOSTRIP(CWorld_ClientCore_World_Init, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ClientCore::World_Init, WORLD_CLIENT);
	
//	ConOutL(CStrF("(CWorld_ClientCore::World_Init) %s, %d, %d", (char*) _WName, _nObj, _nRc));

	int bIsMirror = m_ClientMode & WCLIENT_MODE_MIRROR;

	{
		MSCOPESHORT(Objects);
		if (_nObj == -1)
			_nObj = m_lspObjects.Len();
		else
		{
			m_lspObjects.Clear();
			m_lspObjects.SetLen(_nObj);

			// Create client registry mirrors/server-replica.
			{
				m_spNetReg = REGISTRY_CREATE;
				if (!m_spNetReg) MemError("Create");
				m_spNetReg->SetNumChildren(WCLIENT_REG_NUM);
				for(int i = 0; i < WCLIENT_REG_NUM; i++)
					m_spNetReg->SetValue(i, CFStrF("NETREG%d", i).Str());
			}
		}
	}
	m_ClientObjectBase = m_lspObjects.Len();

	// Setup enum-space.
	int nClObj = m_lspObjects.Len() + m_lspClientObjects.Len();
	m_lEnumSpace.SetLen(nClObj);

#ifndef DEF_DISABLE_PERFGRAPH
	if (!bIsMirror && GetClientMode() & WCLIENT_MODE_PRIMARY)
	{
		MSCOPE(PerfGraph, IGNORE);
		m_PerfGraph_lObjects.Clear();
		m_PerfGraph_Tick_OnClientRender = 0;
		m_PerfGraph_Tick_OnClientRefresh = 0;
	}
#endif

	// Setup space-enumerator.
	if (!bIsMirror)
	{
		MSCOPE(SpaceEnum, WORLD_CLIENT);
		m_spSpaceEnum = MNew(CWO_SpaceEnum);
		if (m_spSpaceEnum == NULL) MemError("-");
		m_spSpaceEnum->Create(SERVER_HASH1_SIZE, SERVER_HASH1_BUCKET_SHIFT_SIZE, SERVER_HASH2_SIZE, SERVER_HASH2_BUCKET_SHIFT_SIZE, nClObj, false);

		m_iObjVisPool.Create(nClObj);

		// Sound Volumes
		m_iObjSoundPool.Create(nClObj);

		// Dynamics pool
		m_ObjPoolDynamics.Create(nClObj);
	}

	if (_WName != "" && m_spMapData == NULL)
	{
		MSCOPE(WorldData, WORLD_CLIENT);

		m_spMapData = MNew(CMapData);
		if (!m_spMapData) MemError("Net_OnMessage");
		m_spMapData->Create(m_spWData);
		m_spMapData->SetNumResources(_nRc);
		m_spMapData->SetWorld(_WName);
		m_spMapData->SetState(WMAPDATA_STATE_NOCREATE);
	}

	if (!bIsMirror)
	{
		m_lspFrameIn.Clear();
		m_lFrameInTime.Clear();
		Net_InitUpdateBuffers();
	}

	m_SimulationTick = 0;
	m_TimeScale = 1.0;
	m_TickRealTime = m_TickTime / m_TimeScale;
	m_RenderTime.Reset();
	m_RenderTickFrac = 0;
	m_bSound_PosSet = false;
	m_Sound_VolumeGameMaster = 1.0f;
	m_Sound_LastestNumHit = 0;
	m_WorldName = _WName.GetFilenameNoExt().UpperCase();
	m_SpawnMask = _SpawnMask;
	m_bShowObjPhys = false;

	ClearStatistics();

	{
		int iIndex = m_spWData->GetResourceIndex(CStrF("ATR:%s", m_WorldName.Str()), NULL);
		if (iIndex > 0)
		{
			CWResource *pRes = m_spWData->GetResource(iIndex);
			if (pRes)
			{
				CWRes_ObjectAttribs *pObjAttr = TDynamicCast<CWRes_ObjectAttribs>(pRes);
				if (pObjAttr)
				{
					CRegistryDirectory *pRegDir = pObjAttr->GetRegDir();
					int nObjAttribsObjects = pRegDir->NumReg();
					if (nObjAttribsObjects < 1)
						Error("World_Init", "No objects.");

					CRegistry *pReg = pRegDir->GetReg(0);
					if (pReg->GetValuei("EVALED", 0))
					{
						pReg = pReg->FindChild("TEMPLATES");
						if (pReg && pReg->GetNumChildren())
							pReg = pReg->GetChild(0);
					}

					m_spWorldSpawnKeys = pReg;
				}
				else
					Error("World_Init", "No objects.");
			}
			else
				Error("World_Init", "No objects.");
		}
		else
			Error("World_Init", "No objects.");


#if 0
		CStr FileName = CStrF("WORLDS\\%s.XW", m_WorldName.Str());
		FileName = m_spWData->ResolveFileName(FileName);

		//ConOutL("(CWorld_ServerCore::World_LoadObjects) Spawning objects from " + _FileName);

		CDataFile DFile;
		DFile.Open(FileName);

		// Read Object-Attributes
		if (!DFile.GetNext("OBJECTATTRIBS"))
		{
			Error("World_Init", "No OBJECTATTRIBS entry.");
		}

		int nObjects = DFile.GetUserData();
		int Version = DFile.GetUserData2();
		if (nObjects < 1)
			Error("World_Init", "No objects.");

		spCRegistry spObj = REGISTRY_CREATE;
		//		spCWObjectAttributes spObj = DNew(CWObjectAttributes) CWObjectAttributes;
		if (spObj == NULL) MemError("Read");
		spObj->Read(DFile.GetFile(), Version);

		m_spWorldSpawnKeys = spObj;

		DFile.Close();
#endif
	}

#ifdef M_Profile
	int iPhysicsPrim = m_spGUIData->GetResourceIndex_Model("PHYSICSPRIM");
	if(iPhysicsPrim)
		m_pPhysicsPrim = m_spGUIData->GetResource_Model(iPhysicsPrim);
#endif

	// Relink all objects.
	for(int i = 0; i < m_lspObjects.Len() + m_lspClientObjects.Len(); i++)
		Object_Link(i);

	Render_ResetCameraVelocity();
}

void CWorld_ClientCore::World_InitClientExecuteObjects()
{
	MAUTOSTRIP(CWorld_ClientCore_World_InitClientExecuteObjects, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ClientCore::World_InitClientExecuteObjects, WORLD_CLIENT);
	
	// Init storage for client-execute objects.

	m_ClientObjectBase = 0;
	int nClObj = 512;		// Should be defined elsewhere, dynamically. Perhaps defined by a server registry variable?
	m_spClientObjectHeap = MNew1(CIDHeap, nClObj);
	if (!m_spClientObjectHeap) MemError("World_InitClientExecuteObjects");
	m_lspClientObjects.Clear();
	m_lspClientObjects.SetLen(nClObj);
}

bool CWorld_ClientCore::HideHud()
{
	return false;
}

void CWorld_ClientCore::World_Pause(bool _bPause)
{
	MAUTOSTRIP(CWorld_ClientCore_World_Pause, MAUTOSTRIP_VOID);
	// Check if pause state is going to change
	if ((_bPause && (m_ClientMode & WCLIENT_MODE_PAUSE)) ||
		(!_bPause && !(m_ClientMode & WCLIENT_MODE_PAUSE)))
		return;

	m_ClientMode &= ~WCLIENT_MODE_PAUSE;
	CMTime Time;
	Time.Snapshot();
	m_LastCmdTime = Time;
	m_PauseTime = Time;
	m_LastUnpackTime = Time;

	if (_bPause)
	{
		// Pause on
		m_ClientMode |= WCLIENT_MODE_PAUSE;

		if (m_spSound != NULL)
		{
			for(int i = 0; i < WCLIENT_NUMCHANNELS; i++)
				if (m_hChannels[i] >= 0)
					m_spSound->Chn_Pause(m_hChannels[i]);
		}
	}
	else
	{
		// Pause off
		if (m_spSound != NULL)
		{
			for(int i = 0; i < WCLIENT_NUMCHANNELS; i++)
				if (m_hChannels[i] >= 0)
					m_spSound->Chn_Play(m_hChannels[i]);
		}
	}
}

int CWorld_ClientCore::World_EnumerateVisible(const CVec3Dfp32& _Pos, uint16* _piRetObj, int _MaxObj)
{
	MAUTOSTRIP(CWorld_ClientCore_World_EnumerateVisible, 0);
	const uint8* pPVS = (m_pSceneGraph) ? m_pSceneGraph->SceneGraph_PVSLock(0, _Pos) : NULL;
	if (pPVS && m_pSceneGraph && m_spSceneGraphInstance != NULL)
	{
		World_CommitDeferredSceneGraphLinkage();

		int nObj = m_spSceneGraphInstance->SceneGraph_EnumeratePVS(pPVS, _piRetObj, _MaxObj);
		m_pSceneGraph->SceneGraph_PVSRelease(pPVS);
		return nObj;
	}
	else
	{
		int nObj = m_lspObjects.Len();
		if (m_lspObjects.Len() > _MaxObj)
			Error("World_EnumeratePVS", "Not enough enumeration space.");
		for(int i = 0; i < nObj; i++)
			_piRetObj[i] = i;
		return nObj;
	}
}

void CWorld_ClientCore::World_SetModel(int _iModel, CXR_WallmarkContextCreateInfo* _pWCCI)
{
	MAUTOSTRIP(CWorld_ClientCore_World_SetModel, MAUTOSTRIP_VOID);
	if (_iModel == m_iWorldModel)
		return;
	MSCOPESHORT(CWorld_ClientCore::World_SetModel);

	m_pWorld = NULL;

	m_spSceneGraphInstance = NULL;
	m_pSceneGraph = NULL;

	if (m_hWallmarkContext >= 0 && m_pWallmarkInterface)
	{
		m_pWallmarkInterface->Wallmark_DestroyContext(m_hWallmarkContext);
		m_pWallmarkInterface = NULL;
		m_hWallmarkContext = -1;
	}
	m_spTMDC = NULL;

	m_iWorldModel = _iModel;
	if (m_iWorldModel)
	{
		m_pWorld = m_spMapData->GetResource_Model(m_iWorldModel);
		if (!m_pWorld)
			Error("World_SetModel", "Unable to acquire world-model.");

		m_pSceneGraph = m_pWorld->SceneGraph_GetInterface();
		if (!m_pSceneGraph) 
			Error("World_SetModel", "Model 0 was not a scene-graph model.");
		int nClObj = m_lspObjects.Len() + m_lspClientObjects.Len();

		m_spSceneGraphInstance = m_pSceneGraph->SceneGraph_CreateInstance();
		if (!m_spSceneGraphInstance)
			Error("World_SetModel", "Unable to create scene graph instance.");
		m_spSceneGraphInstance->Create(nClObj, 64);		// 64 Dynamic lights
		m_SceneGraphDeferredLinkage.Create(nClObj);

		if (GetClientMode() & WCLIENT_MODE_PRIMARY)
		{
			m_pWallmarkInterface = m_pWorld->Wallmark_GetInterface();
			if (m_pWallmarkInterface)
			{
				if (_pWCCI)
					m_hWallmarkContext = m_pWallmarkInterface->Wallmark_CreateContext(*_pWCCI);
				else
				{
					CXR_WallmarkContextCreateInfo CreateInfo;
					CreateInfo.m_CapacityDynamic = 384;
					CreateInfo.m_CapacityTemp = 200;
	#ifdef PLATFORM_CONSOLE
					CreateInfo.m_CapacityStatic = 384*64;
	#else
					CreateInfo.m_CapacityStatic = 384;
	#endif
					m_hWallmarkContext = m_pWallmarkInterface->Wallmark_CreateContext(CreateInfo);
				}
			}

			MRTC_SAFECREATEOBJECT_NOEX(spTMDC, "CXR_TriangleMeshDecalContainer", CXR_TriangleMeshDecalContainer);
			if (spTMDC != NULL)
			{
				spTMDC->Create(8192, 256);
				m_spTMDC = spTMDC;
			}
		}
	}
	else
		return;
//		Error("World_InitObjects", "Unable to acquire world-model.");

	// Relink all objects.
	for(int i = 0; i < m_lspObjects.Len() + m_lspClientObjects.Len(); i++)
		Object_Link(i);
}

CXR_Model* CWorld_ClientCore::World_GetModel()
{
	MAUTOSTRIP(CWorld_ClientCore_World_GetModel, NULL);
	if (!m_pWorld) Error("World_GetModel", "No world model available.");
	return m_pWorld;
}

bool CWorld_ClientCore::ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
{
	MAUTOSTRIP(CWorld_ClientCore_ProcessKey, false);
	// Default: Process no scankeys

/*	switch(_Key.GetKey9())
	{
	case SKEY_ESC : 
		{
			bla...
		}
	}*/
	return false;
}

void CWorld_ClientCore::PreRefresh()
{
	// Send some stuff back..
	if (m_RecMode != WCLIENT_RECMODE_PLAYBACK)
	{
		Net_SendClientVars();
//		if (CanRenderWorld() && !m_Precache)
		Net_SendControls();
	}
	Net_FlushMessages();
	Net_FlushConnection();
}


void CWorld_ClientCore::Refresh()
{
	MAUTOSTRIP(CWorld_ClientCore_Refresh, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ClientCore::Refresh, WORLD_CLIENT);
	M_NAMEDEVENT("Client::Refresh", 0xff000080);
	// Poll all incomming network messages.
	const CNetMsg* pMsg = NULL;
	while((pMsg = Net_GetMsg()))
		Net_OnMessage(*pMsg);

	// Check if any sound sync groups are ready to go.
	Sound_UpdateSyncGroups();

	//
	Net_UnpackGameFrames();
	Net_FlushMessages();
	Net_FlushConnection();
}


void CWorld_ClientCore::Simulate(int _bCalcInterpolation)
{
	MAUTOSTRIP(CWorld_ClientCore_Simulate, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ClientCore::Simulate, WORLD_CLIENT);

	if (m_spMapData == NULL)
	{
		ConOutL("(CWorld_ClientCore::Simulate) No WorldData");
		//Error("Simulate", "No WorldData.");
		return;
	}

	if (m_bPhysRender && (m_ClientMode != WCLIENT_MODE_MIRROR))
	{
#ifndef	PLATFORM_PS2
/*
		if (!m_spPhysCapture)
		{
			MRTC_SAFECREATEOBJECT_NOEX(spRC, "CRenderContextCapture", CRenderContextCapture);
			m_spPhysCapture = spRC;
			if (!m_spPhysCapture)
			{
				ConOut("Unable to create capture-renderer.");
				m_bPhysRender = 0;
			}
			else
			{
				m_spPhysCapture->Create(NULL, NULL);
				CRC_Viewport VP;
				m_spPhysCapture->BeginScene(&VP);
			}
		}
*/
		if (!m_spWireContainer)
		{
			MRTC_SAFECREATEOBJECT_NOEX(spWC, "CDebugRenderContainer", CDebugRenderContainer);
			m_spWireContainer = spWC;
			if (!m_spWireContainer)
			{
				ConOut("Unable to create wire-container.");
			}
			else
			{
				CRC_Font* pFont = m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("MONOPRO"));
				m_spWireContainer->Create(8192, 64, pFont);
				MRTC_GOM()->RegisterObject((CReferenceCount*)m_spWireContainer, "GAMECONTEXT.CLIENT.WIRECONTAINER");
			}
		}
/*
		if (m_spPhysCapture)
		{
			m_spPhysCapture->EndScene();
			CRC_Viewport VP;
			m_spPhysCapture->BeginScene(&VP);
		}
*/
		m_spWireContainer->Refresh(GetGameTickTime());
#endif
	}
	else
	{
		m_spPhysCapture = NULL;
		if (m_spWireContainer)
			MRTC_GOM()->UnregisterObject((CReferenceCount*)m_spWireContainer, "GAMECONTEXT.CLIENT.WIRECONTAINER");
		m_spWireContainer = NULL;
	}

#if defined(M_Profile) && !defined(DEF_DISABLE_PERFGRAPH)
	// -------------------------------------------------------------------
	//  Plot network stats
	{
//		CMTime T = CMTime::GetCPU();
		CMTime T;
		T.Snapshot();
		if ((T - m_NetLoadSampleTime).GetTime() > 1.0f)
		{
			m_NetLoadLastFrameAverage = (int)(m_NetLoadSecondTotal * GetGameTickTime());
			m_NetLoadLastSecondTotal = m_NetLoadSecondTotal;
			m_NetLoadSampleTime = T;
			m_NetLoadSecondTotal = 0;
			m_NetLoadSecondTotal = 0;
		}

		if (m_bNetLoad)
			ConOut(CStrF("NetLoad %d, %d, %d", m_NetLoadFrameTotal, m_NetLoadLastFrameAverage, m_NetLoadLastSecondTotal));

		AddGraphPlot(WCLIENT_GRAPH_NETBANDWIDTH, m_NetLoadFrameTotal, (m_NetLoadFrameTotal > 150) ? 0xffff0000 : 0xff00a0ff);

		m_NetLoadFrameTotal = 0;
	}
#endif

	m_SimulationTick++;
	m_SimulationTime += CMTime::CreateFromSeconds(GetGameTickTime());

	TProfileDef(TObj);
	{
		TMeasureProfile(TObj);


		m_NumSimulate++;	// For stats

		bool bUsePVS = true;

		const int MaxObj = 4096;
		uint16 liObj[MaxObj];
		if (m_lspObjects.Len() > MaxObj)
			Error("Render_EnumerateView", "Too many objects.");

		const uint16* piObj = NULL;
		int nObj = 0;

		CMat4Dfp32 PVSPos;
		Render_GetLastRenderCamera(PVSPos);

		const uint8* pPVS = (m_pSceneGraph && m_spSceneGraphInstance != NULL) ? 
			m_pSceneGraph->SceneGraph_PVSLock(0, CVec3Dfp32::GetRow(PVSPos, 3)) : NULL;

		if (bUsePVS && pPVS)
		{
			// -------------------------------------------------------------------
			//  Get PVS objects
			World_CommitDeferredSceneGraphLinkage();

//			spCWObject_Client* lpObj = m_lspObjects.GetBasePtr();
			nObj = m_spSceneGraphInstance->SceneGraph_EnumeratePVS(pPVS, liObj, MaxObj);
			piObj = liObj;
		}
		else
		{
			// OBS!
			nObj = m_lspObjects.Len() + m_lspClientObjects.Len();
		}

		if(pPVS)
			m_pSceneGraph->SceneGraph_PVSRelease(pPVS);
		
		{
			// -------------------------------------------------------------------
			//  Run OnClientRefresh on all objects
			int nVoices = 0;
	#ifndef DEF_DISABLE_PERFGRAPH
			m_PerfGraph_Tick_OnClientRefresh++;
//			fp64 CPUFreqRecp = 1.0 / GetCPUFrequency();
	#endif

			for(int iiObj = 0; iiObj < nObj; iiObj++)
			{
				int iObj = (piObj) ? piObj[iiObj] : iiObj;
				CWObject_CoreData* pObj = Object_GetCD(iObj);
				if (pObj)
				{
					if (!(pObj->m_ClientFlags & CWO_CLIENTFLAGS_NOREFRESH))
					{
	#ifndef DEF_DISABLE_PERFGRAPH
						CMTime T; TStart(T);
	#endif
						CMat4Dfp32 LastPos = pObj->m_Pos;

						if (IsClientObject(iObj))
							pObj->m_pRTC->m_pfnOnClientExecute((CWObject_ClientExecute*)pObj, this);
						else
							pObj->m_pRTC->m_pfnOnClientRefresh((CWObject_Client*)pObj, this);

						pObj = Object_GetCD(iObj);
						if (pObj)
							pObj->m_LastPos = LastPos;

	#ifndef DEF_DISABLE_PERFGRAPH
						int iClass = pObj->m_iClass;
						TStop(T);

						CWorldData::CWD_ClassStatistics* pCS = m_spMapData->GetResource_ClassStatistics(iClass);
						if (pCS)
						{
							pCS->m_ClientExecuteTime += T;
							pCS->m_nClientExecute++;
						}

						// Plot OnClientRefresh execution time
						if (m_PerfGraph)
						{
							if (m_PerfGraph_lObjects.Len() == 0)
							{
								MSCOPE(PerfGraph, IGNORE);
								int nClObj = m_lspObjects.Len() + m_lspClientObjects.Len();
								m_PerfGraph_lObjects.SetLen(nClObj);
								for(int i = 0; i < nClObj; i++)
									m_PerfGraph_lObjects[i].Create();
								m_PerfGraph_Tick_OnClientRender = 0;
								m_PerfGraph_Tick_OnClientRefresh = 0;
							}

							CWC_ObjectPerfGraph& ObjPerf = m_PerfGraph_lObjects[iObj];
							ObjPerf.m_Time_OnClientRefresh = T;
							ObjPerf.m_spGraph_OnClientRefresh->Plot2(T.GetTime(), Clamp01(0.001f - T.GetTime()), 0x6fffff00, 0x3f000000);
							ObjPerf.m_LastTouch_OnClientRefresh = m_PerfGraph_Tick_OnClientRefresh;
						}
	#endif

						nVoices += Sound_UpdateObject(iObj);
					}
					else
					{					
						if (pObj->GetParent())
							pObj->m_LastPos = pObj->m_Pos;
						nVoices += Sound_UpdateObject(iObj);
					}
				}
			}

	#ifndef DEF_DISABLE_PERFGRAPH
			// Plot zero-time for all object's that were not visible.
			if (m_PerfGraph)
			{
				for(int i = 0; i < m_PerfGraph_lObjects.Len(); i++)
				{
					CWC_ObjectPerfGraph& ObjPerf = m_PerfGraph_lObjects[i];
					if (ObjPerf.m_LastTouch_OnClientRefresh != m_PerfGraph_Tick_OnClientRefresh)
					{
						ObjPerf.m_Time_OnClientRefresh.Reset();
						ObjPerf.m_spGraph_OnClientRefresh->Plot(0, 0xff101060);
					}
				}

			}
			if (nVoices > WCLIENT_NVOICES_SFXLOOPING)
				ConOut(CStrF("(CWorld_ClientCore::Sound_UpdateObject) nVoices %d/%d", nVoices, WCLIENT_NVOICES_SFXLOOPING));				
	#endif
		}
	}
	TAddProfile(m_TExecute, TObj);
	

/*	fp64 TClObj;
	T_Start(TClObj);

	// -------------------------------------------------------------------
	//  Run OnClientExecute on all client-execute objects.
	{
		int nObj = m_lspClientObjects.Len();
		if (!nObj) return;
		TPtr<CWObject_ClientExecute>* lpObj = m_lspClientObjects.GetBasePtr();

		for(int iObj = 0; iObj < nObj; iObj++)
		{
			CWObject_ClientExecute* pObj = lpObj[iObj];
			if (pObj)
			{
				if (pObj->m_iClass && !(pObj->m_ClientFlags & CWO_CLIENTFLAGS_NOREFRESH))
				{
					CMat4Dfp32 LastPos = pObj->m_Pos;
					MRTC_CRuntimeClass_WObject* pRTC = m_spMapData->GetResource_Class(pObj->m_iClass);
					if (pRTC) pRTC->m_pfnOnClientExecute(pObj, this);
					pObj->m_LastPos = LastPos;
				}
				else 
					if (pObj->GetParent())
						pObj->m_LastPos = pObj->m_Pos;
			}
		}
	}
	T_Stop(TClObj);
	m_TExecuteClObj += TClObj;*/

#if defined(M_Profile) && !defined(DEF_DISABLE_PERFGRAPH)
	// -------------------------------------------------------------------
	//  Plot timing..
	CPerfGraph* pGraph = GetGraph(WCLIENT_GRAPH_CLIENTTIME);
	if (pGraph) pGraph->Plot(
		TObj.GetTime(), 0xff700030);
#endif
}

void CWorld_ClientCore::ClearStatistics()
{
	MAUTOSTRIP(CWorld_ClientCore_ClearStatistics, MAUTOSTRIP_VOID);
	m_spWData->ClearClassStatistics();

#ifdef M_Profile
	m_TRender.Reset();
	m_TPVS.Reset();
	m_TExecute.Reset();
	m_TExecuteClObj.Reset();
#endif
	m_NumSimulate = 0;
	m_NumRender = 0;

#ifdef	SERVER_STATS
	m_nFunc_IntersectPrim = 0;
	m_nFunc_MovePhysical = 0;
	m_nFunc_MovePhysicalQuick = 0;
	m_nFunc_IntersectWorld = 0;

	m_TFunc_IntersectPrim.Reset();
	m_TFunc_MovePhysical.Reset();
	m_TFunc_IntersectWorld.Reset();
#endif	// SERVER_STATS
}

void CWorld_ClientCore::DumpStatistics()
{
	MAUTOSTRIP(CWorld_ClientCore_DumpStatistics, MAUTOSTRIP_VOID);
	if ((m_NumSimulate || m_NumRender) && (m_spMapData!=NULL))
	{
		fp32 Mult = 1000.0;

		LogFile("-------------------------------------------------------------------");
		LogFile(" CLIENT STATISTICS");
		LogFile("-------------------------------------------------------------------");
		LogFile(CStrF("Number of ticks:              %6d", m_NumSimulate));
		LogFile(CStrF("Number of renders:            %6d", m_NumRender));
		LogFile(CStrF("Object heap:                  %6d", m_lspObjects.Len()));
		LogFile(" ");

		LogFile(CStr("TOTALS:                              Time"));
		LogFile(CStrF("Render:                       %8.2f ms, %8.2f ms per frame", m_TRender.GetTime()*Mult, m_TRender.GetTime()*Mult / m_NumSimulate));
		LogFile(CStrF("  PVS:                        %8.2f ms, %8.2f ms per frame", m_TPVS.GetTime()*Mult, m_TPVS.GetTime()*Mult / m_NumSimulate));
		LogFile(CStrF("Execution:                    %8.2f ms, %8.2f ms per tick", m_TExecute.GetTime()*Mult, m_TExecute.GetTime()*Mult / m_NumSimulate));
		LogFile(" ");

		LogFile(     "FUNCTIONS:                           Time     Time/Exe    Time/Tick      nX   nX/Tick    Quick%%");
#ifdef	SERVER_STATS
		LogFile(CStrF("MovePhysical:                 %8.2f ms, %8.2f ms, %8.2f ms, %6.d, %8.2f, %8.2f", 
			m_TFunc_MovePhysical.GetTime()*Mult, 
			m_TFunc_MovePhysical.GetTime()*Mult / m_nFunc_MovePhysical,
			m_TFunc_MovePhysical.GetTime()*Mult / m_NumSimulate,
			m_nFunc_MovePhysical, fp32(m_nFunc_MovePhysical) / fp32(m_NumSimulate),
			100*fp32(m_nFunc_MovePhysicalQuick) / fp32(m_nFunc_MovePhysical)));

		LogFile(CStrF("IntersectWorld:               %8.2f ms, %8.2f ms, %8.2f ms, %6.d, %8.2f", 
			m_TFunc_IntersectWorld.GetTime()*Mult,
			m_TFunc_IntersectWorld.GetTime()*Mult / m_nFunc_IntersectWorld,
			m_TFunc_IntersectWorld.GetTime()*Mult/m_NumSimulate,
			m_nFunc_IntersectWorld, fp32(m_nFunc_IntersectWorld) / fp32(m_NumSimulate)));

		LogFile(CStrF("IntersectPrim:                %8.2f ms, %8.2f ms, %8.2f ms, %6.d, %8.2f", 
			m_TFunc_IntersectPrim.GetTime()*Mult,
			m_TFunc_IntersectPrim.GetTime()*Mult / m_nFunc_IntersectPrim,
			m_TFunc_IntersectPrim.GetTime()*Mult/m_NumSimulate,
			m_nFunc_IntersectPrim, fp32(m_nFunc_IntersectPrim) / fp32(m_NumSimulate)));
#endif	// SERVER_STATS
		LogFile(" ");
//               CWObject                                0,     0,     0,      0     -   0.00 ms,  -
		LogFile("CLASS-STATISTICS:                       nX  X/Sim          Time        T/Tick        Time/X       nR  R/Frame        Time       T/Frame        Time/R");

		int nRc = m_spMapData->GetNumResources();
		for(int iRc = 0; iRc < nRc; iRc++)
		{
			MRTC_CRuntimeClass_WObject* pRTC = m_spMapData->GetResource_Class(iRc);
			CWorldData::CWD_ClassStatistics* pCS = m_spMapData->GetResource_ClassStatistics(iRc);
			if (pCS && pRTC)
			{
				CStr sTimePerExe = (pCS->m_nClientExecute) ? CStrF("%9.2f", pCS->m_ClientExecuteTime.GetTime()*Mult / fp32(pCS->m_nClientExecute)).GetStr() : "        -";
				CStr sExeTime = CStrF("%9.2f", 1000.0f*pCS->m_ClientExecuteTime.GetTime());
				CStr sExeTimePerTick = CStrF("%9.2f", 1000.0*pCS->m_ClientExecuteTime.GetTime() / fp32(m_NumSimulate));

				CStr sTimePerR = (pCS->m_nClientRender) ? CStrF("%9.2f", pCS->m_ClientRenderTime.GetTime() * Mult / fp32(pCS->m_nClientRender)).GetStr() : "        -";
				CStr sRTime = CStrF("%9.2f", 1000.0*pCS->m_ClientRenderTime.GetTime());
				CStr sRTimePerFrame = CStrF("%9.2f", 1000.0*pCS->m_ClientRenderTime.GetTime() / (m_NumRender));

				LogFile(CStrF("%-32s   %7d, %5d, %s ms, %s ms, %s ms, %7d, %5d, %s ms, %s ms, %s ms", 
					(char*)pRTC->m_ClassName,
					pCS->m_nClientExecute, 
					pCS->m_nClientExecute / m_NumSimulate,
					sExeTime.Str(), sExeTimePerTick.Str(), sTimePerExe.Str(),
					pCS->m_nClientRender, 
					pCS->m_nClientRender / m_NumRender,
					sRTime.Str(), sRTimePerFrame.Str(), sTimePerR.Str()
					
					)
					);
			}
		}

		LogFile("-------------------------------------------------------------------");
	}

	ClearStatistics();
}

// -------------------------------------------------------------------
void CWorld_ClientCore::Dump(int _DumpFlags)
{
	MAUTOSTRIP(CWorld_ClientCore_Dump, MAUTOSTRIP_VOID);
/*	LogFile("Client LocalPlayer");
	LogFile("------------------");
	LogFile(m_LocalPlayer.Dump());
	LogFile("");
	LogFile("");*/

	if (_DumpFlags & WDUMP_OBJECTS)
	{
		LogFile("Client Game-Objects");
		LogFile("-------------------");
		for(int iObj = 0; iObj < m_lspObjects.Len(); iObj++)
		{
			CWObject_Client* pObj = m_lspObjects[iObj];
			if (pObj && pObj->m_iClass) LogFile(pObj->Dump(m_spMapData, 0));
		}
		LogFile("");
		LogFile("");
	}

	if (_DumpFlags & WDUMP_NETREG && m_spNetReg != NULL)
	{
		LogFile("Client Net-Registry");
		LogFile("-------------------");
		m_spNetReg->XRG_LogDump(1);
		LogFile("");
		LogFile("");
	}

	if (_DumpFlags & WDUMP_WDATA && m_spMapData != NULL)
	{
		LogFile("Client MapData");
		LogFile("--------------");
		m_spMapData->Dump(_DumpFlags);
		LogFile("");
		LogFile("");
	}
}

// -------------------------------------------------------------------
#define DTIME 0

