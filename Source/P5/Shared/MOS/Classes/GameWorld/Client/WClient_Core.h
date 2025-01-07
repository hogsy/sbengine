#ifndef __WCLIENT_CORE_H
#define __WCLIENT_CORE_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Inherited client for faster compilation and less dependecies

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWO_IndexPoolEntry
					CWO_IndexPool
					CWC_ObjectPerfGraph
					CWorld_ClientCore
\*____________________________________________________________________________________________*/

#include "WClient.h"

enum
{
	// -------------------------------------------------------------------
	//  Flags used with UnpackClientUpdate()
	WCLIENT_UNPACKUPD_IP				= 1,
	WCLIENT_UNPACKUPD_ADDIP				= 2,
	WCLIENT_UNPACKUPD_SIMULATE			= 4,

	// -------------------------------------------------------------------
	//  Sound system max values
	WCLIENT_CHANNEL_AMBIENT				= 0,
	WCLIENT_CHANNEL_SFX					= 1,
	WCLIENT_CHANNEL_SFXLOOPING			= 2,
	WCLIENT_CHANNEL_VOICE				= 3,
	WCLIENT_CHANNEL_CUTSCENE			= 4,

	WCLIENT_NUMCHANNELS					= 5,

	WCLIENT_NVOICES_AMBIENT				= 24,
	WCLIENT_NVOICES_SFX					= 32,
	WCLIENT_NVOICES_SFXLOOPING			= 24,
	WCLIENT_NVOICES_VOICE				= 14,
	WCLIENT_NVOICES_CUTSCENE			= 2,

	WCLIENT_ATTENUATION_3D				= 0,
	WCLIENT_ATTENUATION_2D				= 1,
	WCLIENT_ATTENUATION_LRP				= 2,
	WCLIENT_ATTENUATION_3D_OVERRIDE		= 3,	// Attenuation range is taken from _V0  (min = k[0], max = k[1])
	WCLIENT_ATTENUATION_OFF				= -1,	// Turn off tracking sound on object
	WCLIENT_ATTENUATION_LOOP			= -2,	// Play 3D sound looping
	WCLIENT_ATTENUATION_2D_CUTSCENE		= 4,	// Used by Play2DSound (gets its own channel)
	WCLIENT_ATTENUATION_FORCE_3D_LOOP	= 5,
	WCLIENT_ATTENUATION_2D_POS			= 6,

	// -------------------------------------------------------------------
	//  m_ClientUpdateMask flags
	WCLIENT_SERVERUPDATE_PLAYERINFO		= 1,
	WCLIENT_SERVERUPDATE_CONTROLS		= 2,

	WCLIENT_RECMODE_NORMAL				= 0,
	WCLIENT_RECMODE_RECORDING			= 1,
	WCLIENT_RECMODE_PLAYBACK			= 2,

	PRECACHE_DONE = 0,
	PRECACHE_INIT,
	PRECACHE_ONPRECACHE,
	PRECACHE_ONPRECACHECLASS,
	PRECACHE_RESOURCES,
	PRECACHE_TEXTURES,
	PRECACHE_VERTEXBUFFERS,
	PRECACHE_SOUND,
	PRECACHE_ASYNCCACHEBLOCK,

	PRECACHE_PERFORMPOSTPRECACHE,
	PRECACHE_NUMSTEPS,

};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWC_ObjectPerfGraph
|__________________________________________________________________________________________________
\*************************************************************************************************/
#ifndef DEF_DISABLE_PERFGRAPH

class CWC_ObjectPerfGraph : public CObj
{
public:
	CWC_ObjectPerfGraph();
	void Create();

	int m_LastTouch_OnClientRender;
	int m_LastTouch_OnClientRefresh;
	CMTime m_Time_OnClientRender;
	CMTime m_Time_OnClientRefresh;
	TPtr<CPerfGraph> m_spGraph_OnClientRender;
	TPtr<CPerfGraph> m_spGraph_OnClientRefresh;
};

#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWC_SoundSyncGroups
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWC_SoundSyncGroups
{
public:
	struct SGroup
	{
		uint32 m_GroupID;
		TArray<int> m_lVoices;
		bool m_bReadyToGo;

		SGroup(uint32 _GroupID = 0);
	};
	TArray<SGroup> m_lGroups;
	int Find(uint32 _GroupID) const;

public:
	SGroup& Get(uint32 _GroupID);
	void Remove(uint32 _GroupID);
	TAP<SGroup> GetAll();
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWorld_ClientCore
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWorld_ClientCore : public CWorld_Client
{
	typedef CWorld_ClientCore ThisClass;
	MRTC_DECLARE;

protected: 
	CWorld_Client* m_pClientRef;	// Client providing the gamestate if this client is in WCLIENT_MODE_REFERENCE mode.
	CStr m_CmdPrefix;
	CStr m_CmdPrefix_Controls;

	int m_ClientMode;
	int m_ClientState;
	int m_ChangeLevelState;
	int m_LastRcvRc;
	int m_ClientPlayerNr;			// Split-screen player nr, affects command registering.

	int m_iPlayerObj;
	int m_iGameObj;

	int m_ClientDrawDisable;
	int m_bShowObjPhys;
	int m_ShowGraphs;
	int m_bPhysRender;
	fp32 m_HeadLightRange;
	fp32 m_ForcedHeadLightRange;
	CVec3Dfp32 m_HeadLightIntensity;
	
	CVec3Dfp32 m_CameraTweak;				// Use to tweak third-person(dialogue) camera positions

	CMTime m_LastEngineRefresh;
	spCXR_Engine m_spEngine;
	int m_MaxRecurseDepth;

	int m_Precache;							// Perform a texture precache the next frame.
	int m_Precache_iItem;
	int m_Precache_nItemsDone;
	int m_Precache_nItems;
	int m_Precache_ResourceID;
	int m_Precache_Sound_ID;
	int m_Precache_OldSurfOptions;
	TArray<uint16> m_Precache_lPrecacheOrder;

	CRegistry* m_pEnv;							// Environment (environment.cfg)
	spCRegistry m_spGameReg;
	spCRegistry m_spUserReg;					// User settings, i.e sbz1\\user.cfg
	spCRegistry m_spNetReg;						// Registry replicated from server
	CWObjectUpdateBuffer m_NetRegIn;
	CWObjectUpdateBuffer m_NetRegOut;

	int m_iWorldModel;
	CXR_Model* m_pWorld;
	CXR_Model* m_pPhysicsPrim;

	uint32 m_bSyncOnClientRender : 1;

public:
	// Sound stuff
	TPtr<class CSoundContext> m_spSound;
	int m_hChannels[WCLIENT_NUMCHANNELS];
	bool m_bSound_PosSet;

	fp32 m_Sound_VolumeAmbient;
	fp32 m_Sound_VolumeSfx;
	fp32 m_Sound_VolumeVoice;
	fp32 m_Sound_VolumeGameMaster;
	
	fp32 m_Sound_VolumeSoundVolume;
	int32 m_Sound_LastestNumHit;

	CWC_SoundSyncGroups m_SoundSyncGroups;

protected:
	// Render stuff
	CRC_Util2D m_Util2D;
	bool m_bCaptureNext;
	CMat4Dfp32 m_LastRenderCamera;
	CVec3Dfp32 m_LastRenderCameraVelocity;
	CRC_Viewport m_LastViewport;

	CMat4Dfp32 m_CamWVel;
	CMat4Dfp32 m_CamWVel_LastCamera;
	CMTime m_CamWVel_LastGameTime;
	CMTime m_CamWVel_LastCmdTime;

#ifndef DEF_DISABLE_PERFGRAPH
	TArray<CWC_ObjectPerfGraph> m_PerfGraph_lObjects;
	int m_PerfGraph;
	int m_PerfGraph_Tick_OnClientRender;
	int m_PerfGraph_Tick_OnClientRefresh;
#endif

	// Graphs
#ifndef	DEF_DISABLE_PERFGRAPH
	TArray<TPtr<class CPerfGraph> > m_lspGraphs;
#endif

#ifdef M_Profile
	CMTime m_LastRenderTime;
#endif

	// Local player info
	CWC_Player m_LocalPlayer;
	CMTime m_LastCmdTime;

	int m_ClientUpdateMask;

	// In-frames:
	TArray<spCWObjectUpdateBuffer> m_lspFrameIn;
	TArray<CMTime> m_lFrameInTime;
	bool m_bInHead;
	int m_iFrameInHead;
	int m_iFrameInTail;
	CMTime m_LastFrameTime;
	CMTime m_LastUnpackTime;
	CMTime m_PauseTime;
	fp32 m_FramePeriod;
	fp32 m_FramePeriodPrim;

	// Net rcon
	CStr m_RCon_Password;

	// Net state
	int m_bNetShowMsg;
	int m_bNetLoad;
	CMTime m_NetLoadSampleTime;
	int m_NetLoadSecondTotal;
	int m_NetLoadFrameTotal;
	int m_NetLoadLastFrameAverage;
	int m_NetLoadLastSecondTotal;

	CMTime m_RenderTime;
	CMTime m_LastRenderFrameTime;
	fp32 m_RenderTickFrac;

	fp32 m_LocalFrameFraction;

	int m_NumSimulate;	// For stats only
	int m_NumRender;	// For stats only
#ifdef M_Profile
	CMTime m_TRender;
	CMTime m_TPVS;
	CMTime m_TExecute;
	CMTime m_TExecuteClObj;
#endif

	// Network I/O message packages.
	CNetMsgPack m_OutMsg;
	CNetMsgPack m_InMsg;

	// Msg-buffer for local clients. Net_FlushMessages uses this if the client doesn't have a network connection.
	TPtr<TQueue<CNetMsgPack> > m_spLocalInQueue;
	TPtr<TQueue<CNetMsgPack> > m_spLocalOutQueue;

public:
	//
	// a directional sound is a sound that have no origin but it have a direction.
	// it's emulated by moving point sounds with the camera.
	// it's used by the sound volumes
	//
	class CDirectionalSound
	{
	public:
		CDirectionalSound();

		struct 
		{
			uint8 m_Delete:1;
		} m_Flags;

		int32 m_ObjectSoundNumber;
		int32 m_iObject;
		int32 m_iVoice;
		CVec3Dfp32 m_Direction;
	};

	// Sound pool
	CIndexPool16 m_iObjSoundPool;

	// Current playing sounds
	TArray<CDirectionalSound> m_lDirectionalSounds;

	//
	// an object sound is a sound that follows an object.
	//
	class CTrackingSound
	{
	public:
		CTrackingSound();
		CVec3Dfp32 m_Offset;
		int32 m_iVoice;
		int16 m_iObject;
		int16 m_iWave;
	};

	TArray<CTrackingSound> m_lTrackingSounds;

	// returns false if the sound should be removed
	// if _FirstUpdate is set the sound will not be removed becouse the sound isn't playing
	bool UpdateTrackingSound(CTrackingSound &_TrackSound);

private:
	// XDF Usage
	void XDFStart(const char* _Name);
	void XDFStop();

	void NextPrecacheStep(); // Resets alot of precache variables

public:
	CIndexPool16 m_iObjVisPool;
	TThinArray<uint16> m_liEnumObj;
	int m_nEnumObj;

	// CXR_EngineClient overrides
	virtual void EngineClient_EnumerateView(CXR_Engine* _pEngine, int _iVC, int _EnumViewType);
	virtual void EngineClient_Refresh();

protected:
	// This stuff should be privitized.
	TArray<TPtr<class CWObject_Client> > m_lspObjects;

	TPtr<CIDHeap> m_spClientObjectHeap;
	TArray<TPtr<CWObject_ClientExecute> > m_lspClientObjects;
	int m_ClientObjectBase;

public:
	spCNetwork m_spNetwork;									// Network, shared throughout the app.
	int m_hConnection;										// Network connection, -1 if none.

	CFStr m_WorldName;
	int32 m_SpawnMask;
	spCRegistry m_spWorldSpawnKeys;

	CTextureContainer *m_pLastPrecacheTC;
	int32 m_PrecacheProgress;
	bool  m_LoadStatus;

	CWorld_ClientCore();
	~CWorld_ClientCore();
	virtual void Create(CWC_CreateInfo& _CreateInfo);

	// CSubSystem override:
	virtual aint OnMessage(const CSS_Msg& _Msg);

	virtual bool Execute(CStr _Script);						// calls CConsole::ExecuteString(), returns true if no exeption occured.

	uint32 m_ViewFlags;
	fp32 m_AspectChange;
	virtual void SetViewFlags(uint32 _Flags, fp32 _AspectChange)
	{
		m_AspectChange = _AspectChange;
		m_ViewFlags = _Flags;
	}

	virtual uint32 GetViewFlags();

	virtual void Precache_Init();
	virtual void Precache_Perform(CRenderContext* _pRC);
	virtual void Precache_Perform(CRenderContext* _pRC, fp32 _dTime);
	virtual CFStr Precache_GetItemName();
	virtual fp32 Precache_GetProgress();
	virtual int Precache_Status();

	virtual fp32 Load_GetProgress();
	virtual void Load_ProgressReset();

	// -------------------------------------------------------------------
	//  Various info
	virtual int GetClientMode() { return m_ClientMode; };	// Used to check if the client is a mirror or a 'primary' (ie, 'real' client.)
	virtual int GetClientState() { return m_ClientState; };
	virtual int GetRecMode() { return m_RecMode; };
	virtual int GetSplitScreenMode();
	virtual CStr GetClientInfo();							// Used to get debuginfo about the client
	virtual bool CanRenderGame();							// 'true' 
	virtual bool CanRenderWorld();							// 'true' indicates the client has completed connection sequence and is now capable of rendering the game.
	virtual CMTime GetLastSnapshotTime();
	virtual CMTime GetLastCmdTime();
	virtual int GetInteractiveState();

	// -------------------------------------------------------------------
	//  Debug graph
#ifdef M_Profile
	virtual CPerfGraph* GetGraph(int _iGraph);
	virtual void AddGraphPlot(int _iGraph, fp32 _Value, uint32 _Color = 0xffffffff);
#endif

	// -------------------------------------------------------------------
	//  Time stuff
	virtual CMTime GetTime();						// Unscaled system time
//	virtual fp32 GetRenderTickFrac();				// Game tick fraction. [0..1], only valid when called from OnClientRender
	virtual CMTime GetRenderTime();					// GetGameTime + GetRenderTickFrac*GetGameTickTime, only valid when called from OnClientRender
	virtual fp32 GetInterpolateTime();
	virtual fp32 GetModeratedFramePeriod();

	// -------------------------------------------------------------------
	// Player
	virtual CWC_Player* Player_GetLocal();
	virtual int Player_GetLocalObject();
	virtual bool HideHud();

	// -------------------------------------------------------------------
	// GameObj
	virtual int Game_GetObjectIndex();
	
	// -------------------------------------------------------------------
	// Registry
	virtual void OnRegistryChange();						// This function will be called upon registry changes.

	virtual int Registry_GetNum();								// Returns number of registry directories
	virtual const CRegistry* Registry_Get(int _iDirectory);		// Returns a registry directory.
	virtual const CRegistry* Registry_GetRoot();				// Returns the entire client registry
	virtual CRegistry* Registry_GetUser();
	virtual CRegistry* Registry_GetGame();
	virtual CRegistry* Registry_GetEnvironment(); // Added by Talbot
	
protected:
	virtual CRegistry* Registry_GetNonConst(int _iDirectory);	// Returns a non-const registry directory.
	virtual CRegistry* Registry_GetRootNonConst();				// Returns the entire client registry

public:
	// -------------------------------------------------------------------
	// Overrides from CWorld_PhysState

	// AGMerge
	virtual bool IsServer() const { return false; }
	virtual bool IsClient() const { return true; }

	virtual bool Phys_IntersectWorld(const CSelection* _pSelection, const CWO_PhysicsState& _PhysState, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, int _iExcludeObj, 
			CCollisionInfo* _pCollisionInfo, int _NotifyFlags, CSelection* _pNotifySelection1 , CSelection* _pNotifySelecion2);
	virtual bool Phys_IntersectWorld(CPotColSet *_pcs, const CWO_PhysicsState& _PhysState, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, int _iExcludeObj, 
			CCollisionInfo* _pCollisionInfo, int _NotifyFlags, CSelection* _pNotifySelection1 , CSelection* _pNotifySelecion2);
	virtual bool Phys_SetPosition(const CSelection* _pSelection, int _iObj, const CMat4Dfp32& _Pos, CCollisionInfo* _pCollisionInfo);
	virtual bool Phys_MovePosition(const CSelection* _pSelection, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, CCollisionInfo* _pCollisionInfo);
	virtual bool Phys_MovePosition(CPotColSet *_pcs, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, CCollisionInfo* _pCollisionInfo = NULL);
	virtual void GetMovementBounds( float *_BoxMinMax, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest );
	virtual void Selection_GetArray( CPotColSet *pcs, const CSelection* _pSelection, const CWO_PhysicsState &_PhysState, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest);



	virtual aint Phys_Message_SendToObject(const CWObject_Message& _Msg, int _iObj);
	virtual void Phys_Message_SendToSelection(const CWObject_Message& _Msg, const CSelection& _Sel);
	virtual bool Phys_MessageQueue_SendToObject(const CWObject_Message& _Msg, int _iObj);
	virtual void Phys_MessageQueue_Flush();

	// -------------------------------------------------------------------
	// Client object-management
	virtual int IsClientObject(int _iObj);

protected:
	virtual int Object_HeapSize();
public:
	virtual CWObject_CoreData* Object_GetCD(int _iObj);
	virtual CWObject_Client* Object_Get(int _iObj);
	virtual CWObject_Client* Object_GetFirstCopy(int _iObj);
	virtual CWObject_ClientExecute* ClientObject_Get(int _iObj);
	virtual CWObject_Client* Object_GetFirstClientType(int _iObj);
	
	virtual void Object_SetActiveClientCopy(int _iObj, int _iActiveClientCopy);		// Used for object version control in conjunction with prediction.

protected:
	virtual TArray<TPtr<CWObject_Client> > Object_GetArray() { return m_lspObjects; }	// Only accessible by CWorld_Server

public:
	virtual int ClientObject_Create(const char* _pClassName, const CMat4Dfp32& _Pos);
	virtual int ClientObject_Create(const char* _pClassName, const CVec3Dfp32& _Pos);
	virtual void Object_Destroy(int _iObj);

protected:
	virtual void Object_Link(int _iObj);			// Used internally for scene-graph linkage.
	virtual void Object_Unlink(int _iObj);

public:
	virtual void Object_ForcePosition(int _iObj, const CMat4Dfp32& _Pos);
	virtual void Object_ForcePosition(int _iObj, const CVec3Dfp32& _Pos);
	virtual void Object_ForcePosition_World(int _iObj, const CMat4Dfp32& _Pos);

	virtual void Object_SetBox(int _iObj, const CVec3Dfp32& _Min, const CVec3Dfp32& _Max);					// N/A

	virtual int Object_SetDirty(int _iObj, int _Mask);

	// Client-objects only:
	virtual bool Object_Move(int _iObj);						// Move object without any collision. (moderately useful..)

	virtual aint ClientMessage_SendToObject(const CWObject_Message& _Msg, int _iObj);

	// -------------------------------------------------------------------
	// Sound services
protected:
	virtual void Sound_UpdateVolume();
public:
	virtual void Sound_SetSoundContext(TPtr<class CSoundContext> _spSC);
	virtual void Sound_SetVolume(fp32 _VolumeAmbient, fp32 _VolumeSfx, fp32 _VolumeVoice);
	virtual void Sound_SetVolume(int _hVoice, fp32 _Volume);
	virtual void Sound_SetGameMasterVolume(fp32 _Volume);
	virtual void Sound_Reset();
	virtual void Sound_Mute(bool _bMuted);
	virtual void Sound_KillVoices();
	virtual int Sound_Global(int _iChannel, int _iSound, fp32 _Volume = 1.0f, bool _bLoop = false, fp32 _Delay = 0.0f);
	virtual int Sound_At(int _iChannel, const CVec3Dfp32& _Pos, int _iSound, int _AttnType, uint8 _iMaterial=0, fp32 _Volume = 1.0f, const CVec3Dfp32& _V0 = 0, fp32 _Delay = 0.0f, uint32 _SyncGroupID = 0);
	virtual int Sound_At(int _iChannel, const CVec3Dfp32& _Pos, class CSC_SFXDesc *_pSound, int _AttnType, uint8 _iMaterial=0, fp32 _Volume = 1.0f, const CVec3Dfp32& _V0 = 0, fp32 _Delay = 0.0f, uint32 _SyncGroupID = 0);
	virtual int Sound_On(int _iChannel, int16 _iObject, int _iSound, int _AttnType, uint8 _iMaterial=0, fp32 _Volume = 1.0f, const CVec3Dfp32& _V0 = 0, fp32 _Delay = 0.0f);
	virtual void Sound_Off(int16 _iObject, int _iSound, uint8 _iMaterial=0);
	virtual void Sound_Kill(int _iObj);
	dllvirtual bool Sound_IsActive();

	// Internal sound functions
	int Sound_Play(int _iChannel, int16 _iObject, const CVec3Dfp32& _Pos, CSC_SFXDesc *_pSound, int _AttnType, uint8 _iMaterial=0, fp32 _Volume = 1.0f, const CVec3Dfp32& _V0 = 0, fp32 _Delay = 0.0f, bool _bLoop = false, uint32 _SyncGroupID = 0);
	int Sound_Play(int _iChannel, int16 _iObject, const CVec3Dfp32& _Pos, int _iSound, int _AttnType, uint8 _iMaterial=0, fp32 _Volume = 1.0f, const CVec3Dfp32& _V0 = 0, fp32 _Delay = 0.0f, bool _bLoop = false, uint32 _SyncGroupID = 0);
//	int Sound_CreateVoice3D(CSC_SFXDesc *_pDesc, int16 _Wave, int _hChn, const CSC_3DProperties& _Properties, fp32 _Volume = 1.0f, fp32 _Delay = 0.0f, bool _bLoop = false);
	dllvirtual int Sound_CreateVoice(CSC_SFXDesc *_pDesc, int16 _Wave, CSC_VoiceCreateParams &_Params);

	dllvirtual void Sound_AddVoiceToSyncGroup(int _hVoice, uint32 _GroupID);
	void Sound_UpdateSyncGroups();

	virtual bool Sound_MultiStream_Play(int *_pMusic, int _nMusic);
	virtual void Sound_MultiStream_Volumes(fp32 *_pVolume, int _nVolumes, fp32 *_pFadeSpeeds);

	virtual void Sound_UpdateSoundVolumes(const CVec3Dfp32 &_Pos);
	virtual void Sound_SetSoundVolumesVolume(fp32 _Volume);
	virtual void Sound_UpdateTrackSounds();
	virtual void Sound_Clean();

	virtual void Sound_SetSoundChannelVolume(int _Channel, fp32 _Volume);
	virtual fp32 Sound_GetSoundChannelVolume(int _Channel);

protected:
	dllvirtual int Sound_UpdateObject(int _iObj);
 
public:
	virtual int IsFullScreenGUI() { return false; };
	virtual int IsCutscene() { return false; }
	virtual CXR_Engine* Render_GetEngine();
	virtual void Render_SetViewport(CXR_VBManager* _pVBM);
	virtual void Render_GUI(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP);
	virtual void Render_GetCamera(CMat4Dfp32& _Camera);			// NOTE: This function invokes client-prediction and is therefore VERY costly.
	virtual void Render_GetLastRenderCamera(CMat4Dfp32& _Camera);// Use this to get the last predicted camera position.

	virtual void Render_ResetCameraVelocity();
	virtual void Render_GetCameraVelocity(const CMat4Dfp32& _WCam, CMat4Dfp32& _WCamVelocity);

	virtual void Render_ModifyViewport(CRC_Viewport& _Viewport);
	virtual void Render_GetLastRenderViewport(CRC_Viewport& _Viewport);	// Use this to get the viewport last returned by Render_GetViewport
	virtual CVec2Dfp32 Render_GetViewScaleMultiplier();
	virtual bool Render_World_AllowTextureLoad() { return false; };
	virtual void Render_World(CXR_VBManager* _pVBM, CRenderContext* _pRender, const CMat4Dfp32& _Camera, const CMat4Dfp32& _CameraVelocity, fp32 _InterpolationTime);
	virtual void Render_ObjectPerfGraphs(CXR_VBManager* _pVBM, CRenderContext* _pRender, const CMat4Dfp32& _Camera);
	virtual void Render(CXR_VBManager* _pVBM, CRenderContext* _pRender, CRC_Viewport& _GUIVP, fp32 _InterpolationTime, int _Context);

	CXR_Engine*	 GetEngine() {return m_spEngine;}; // added by Talbot
	// -------------------------------------------------------------------
	virtual CRenderContext* Debug_GetRender();
	virtual CDebugRenderContainer* Debug_GetWireContainer(uint _Flags = 3);
	// -------------------------------------------------------------------
	// Wallmark
protected:
	int m_hWallmarkContext;
	CXR_WallmarkInterface* m_pWallmarkInterface;
	TPtr<class CXR_TriangleMeshDecalContainer> m_spTMDC;

public:

	virtual int Wallmark_Create(const CMat4Dfp32 &_Pos, fp32 _Size, fp32 _Tolerance, CMTime _Time, const char* _pSurfName, int _Flags);
	virtual int Wallmark_Create(const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags);
	virtual int Wallmark_Create(CXR_Model* _pModel, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags);
	virtual bool Wallmark_Destroy(int _GUID);

public:
	virtual void World_Close();
	virtual void World_Init(int _nObj, int _nRc, CStr _WName, int32 _SpawnMask);
	virtual void World_InitClientExecuteObjects();

	virtual void World_Pause(bool _bPause);

	virtual int World_EnumerateVisible(const CVec3Dfp32& _Pos, uint16* _piRetObj, int _MaxObj);

	virtual void World_SetModel(int _iModel, CXR_WallmarkContextCreateInfo* _pWCCI = NULL);
	virtual CXR_Model* World_GetModel();

	// -------------------------------------------------------------------
	// Local messaging.
protected:
	virtual void Local_InitQueues();
public:
	virtual bool Local_OutQueueEmpty();
	virtual const CNetMsgPack* Local_GetOutMsg();
	virtual bool Local_PutInMsg(const CNetMsgPack& _MsgPack);
	virtual CCmdQueue* Local_GetCmdQueue();
	virtual void Local_ClearCmdQueue();

	// -------------------------------------------------------------------
	// Network stuff
protected:
	virtual void Net_InitUpdateBuffers();
	virtual void Net_UnpackClientUpdate(CWObjectUpdateBuffer* _pDeltaBuffer, int _Flags = WCLIENT_UNPACKUPD_IP | WCLIENT_UNPACKUPD_SIMULATE);
	virtual void Net_FlushGameFrames();
	virtual void Net_UnpackGameFrame();
	virtual void Net_UnpackGameFrames();

	virtual bool Net_CreateServerRegUpdate(CWObjectUpdateBuffer* _pDeltaBuffer);
	virtual bool Net_UnpackClientRegUpdate(CWObjectUpdateBuffer* _pDeltaBuffer);

public:
	virtual bool Net_PutMsg(const CNetMsg& _Packet);					// Send netmsg to server, false if queue is full.
	virtual const CNetMsg* Net_GetMsg();								// Poll message from incomming queue. NULL if empty.
	virtual bool Net_FlushMessages();									// Flush outgoing queue.
	virtual bool Net_FlushConnection();									// Flush outgoing queue.

	virtual void Net_OnMessage_Sound(const CNetMsg& _Msg);
	virtual void Net_OnMessage_World(const CNetMsg& _Msg);
	virtual void Net_OnMessage_ResourceID(const CNetMsg& _Msg);
	virtual void Net_OnMessage_DoPrecache(const CNetMsg& _Msg);
	virtual void Net_OnMessage_EnterGame(const CNetMsg& _Msg);
	virtual void Net_OnMessage_DeltaFrame(const CNetMsg& _Msg);
	virtual void Net_OnMessage_DeltaRegistry(const CNetMsg& _Msg);
	virtual void Net_OnMessage_Command(const CNetMsg& _Msg);
	virtual void Net_OnMessage_ObjNetMsg(const CNetMsg& _Msg);
	virtual void Net_OnMessage_NullObjNetMsg(const CNetMsg& _Msg);
	virtual void Net_OnMessage_Pause(const CNetMsg& _Msg);
	virtual void Net_OnMessage_SingleStep(const CNetMsg& _Msg);
	virtual void Net_OnMessage_LocalFrameFraction(const CNetMsg& _Msg);
	virtual void Net_OnMessage_SoundSync(const CNetMsg& _Msg);

	virtual int Net_OnProcessMessage(const CNetMsg& _Msg);				// Called for each incomming netmsg, calls Net_OnMessage_xxxx
	virtual void Net_OnMessage(const CNetMsg& _Packet);					// Called for each incomming netmsg, calls Net_OnProcessMessage
	virtual bool Net_SetClientVar(CStr _Key, CStr _Value);

	virtual void Net_AddIdleCommand();
	virtual void Net_SendClientVars();
	virtual void Net_SendControls(bool _bForced = false);

	virtual void Net_SetConnection(spCNetwork _spNetwork, int _hConnection);
	virtual int Net_GetConnection();
	virtual uint32 Net_GetConnectionStatus();

	virtual	bool ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey);
	virtual void PreRefresh();
	virtual void Refresh();
	virtual void Simulate(int _bCalcInterpolation = 1);

	// -------------------------------------------------------------------
	// Demo-recording/playback
protected:
	CStr m_RecordingName;
	CDataFile m_RecFile;
	int m_RecMode;
	int m_RecNumMsg;
	CDemoMsgStream m_RecStream;

public:
	virtual void Demo_Start(CStr _FileName);
	virtual void Demo_Stop();
	virtual void Demo_Get(CStr _FileName);
	virtual int Demo_PlayTick();

protected:
	virtual void Demo_ReadInitialState(CDataFile* _pDFile);
	virtual void Demo_WriteInitialState(CDataFile* _pDFile);
	virtual void Demo_ReadMsgStream(CDataFile* _pDFile);
	virtual void Demo_StoreMessage(const CNetMsg& _Msg, fp32 _TimeStamp);

public:
	// Dump
	void ClearStatistics();
	void DumpStatistics();
	
	virtual void Dump(int _DumpFlags);
	virtual void DumpTextureUsage(CStr _Path, bool _bAppend, bool _bIncludeNonPrecached = false);
	virtual void DumpSoundUsage(CStr _Path, bool _bAppend);
	virtual void DumpGeometryUsage(CStr _Path, bool _bAppend);
	friend class CWorld_Server;

protected:
	virtual fp32 GetCmdDeltaTime();
	virtual bool AddCmd(const CCmd& _Cmd);
	virtual bool AddCmd_Forced(const CCmd& _Cmd);

	// -------------------------------------------------------------------
public:
	virtual void Con_Cmd(int _cmd);						//  Some generic client player-commands
	virtual void Con_Cmd2(int _cmd, int d0);
	virtual void Con_Cmd3(int _cmd, int d0, int d1);
	virtual void Con_Cmd4(int _cmd, int d0, int d1, int d2);

	virtual void Con_Cmd_Forced(int _cmd);				//  Some generic client player-commands
	virtual void Con_Cmd2_Forced(int _cmd, int d0);
	virtual void Con_Cmd3_Forced(int _cmd, int d0, int d1);
	virtual void Con_Cmd4_Forced(int _cmd, int d0, int d1, int d2);

	// -------------------------------------------------------------------
	virtual void Con_NetLoad(int _v);					// Display network traffic statistics
	virtual void Con_NetShowMsg(int _v);				// Display network messages
	virtual void Con_NetRate(int _Rate);				// Set new network rate
	virtual void Con_NoDraw(int _v);					// Various flags, 1=Render Ent 1 only, 2=Disable GUI rendering, 4=DisablePVS for OnClientRender, 16=DisplayPVSInfo, 32=Display EnumView time.
	virtual void Con_Say(CStr _s);
	virtual void Con_SayTeam(CStr _s);
	virtual void Con_ShowObjPhys(int _v);				// Render physics-primitives in wireframe.
	virtual void Con_ShowGraphs(int _v);				// Display various graphs. ServerTime, ClientTime, Lagometer, Network load, etc..
	virtual void Con_PerfGraph(int _v);					// Display object perf graphs.
	virtual void Con_PhysRender(int _v);				// Render debugging wires.
	virtual void Con_HeadLightRange(fp32 _v);
	virtual void Con_HeadLightIntensity(fp32 _r, fp32 _g, fp32 _b);
	virtual void Con_TimeScale(fp32 _v);					// Change client time-scale. Works only for demo-playback.

	virtual void Con_RCon_Password(CStr _s);			// Set remote-control password
	virtual void Con_RCon(CStr _s);					// Send remote-control command

	virtual void Con_AddBufferSkip(int _v);				// Increase/Decrease number of vertex-buffers to not render.

	virtual void Con_Set(CStr _k, CStr _v); 		// Set client-variable
	virtual void Con_OpenGameMenu();					// Opens up the in-game menu
	virtual void Con_SetXYZCameraValues(fp32 _x, fp32 _y, fp32 _z); 

	virtual void Con_SyncOnClientRender(int _v);

	// Wrappers
	virtual void Con_Name(CStr _s);					// Set "name" client-variable
	virtual void Con_Team(CStr _s);					// Set "team" client-variable

	void Register(CScriptRegisterContext &_RegContext);

public:
	virtual void OnClientRender(CXR_Engine* _pEngine, int _EnumViewType, int _iObj, CWObject_Client* _pObj, const CMat4Dfp32& _BaseMat);
};

#endif
