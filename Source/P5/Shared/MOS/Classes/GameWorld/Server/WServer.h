#ifndef __WSERVER_H
#define __WSERVER_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Server baseclass and helpers

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CPlayer
					CWorld_Server
\*____________________________________________________________________________________________*/

#include "../WPhysState.h"
#include "../Client/WClient.h"

//class CWObjectAttributes;

// -------------------------------------------------------------------
#define SERVER_MESSAGEQUEUESIZE		128

#define SERVER_NUMMEDIUMS			12

#define WSERVER_GAMEOBJNAME			"__GAMECONTROL__"

#define WSERVER_MAXCLIENTS			256

//#define WCLIENT_REG_GAMESTATE		0





#define SAVE_BLOCKS				3		// Nr of blocks for a save game  
#define BLOCK_SIZE				16384   // Block size in bytes
#define MAXIMUM_SAVE_SLOTS		10		// Maximum number of slots available
#define MAXIMUM_XBOXSAVE_SLOTS  4096    // Maximum number of saves Microsoft allows on an device
#define LAUNCHMENU_NEWGAME		0x01	// Identifier telling the game to launch to the newgame menu (might be set in the LD_FROM_DASHBOARD on startup)


// -------------------------------------------------------------------
//  CWorld_Server
// -------------------------------------------------------------------
enum
{
	SERVER_CLIENTUPDATE_WORLD			= M_Bit(0),
	SERVER_CLIENTUPDATE_PLAYER			= M_Bit(1),
	SERVER_CLIENTUPDATE_SCORES			= M_Bit(2),
	SERVER_CLIENTUPDATE_DELTAFRAME		= M_Bit(3),
	SERVER_CLIENTUPDATE_DELTAREGISTRY	= M_Bit(4),
	SERVER_CLIENTUPDATE_DOWNLOAD		= M_Bit(5),
	SERVER_CLIENTUPDATE_STATUSBAR		= M_Bit(6),
	SERVER_CLIENTUPDATE_COMPLETEFRAME	= M_Bit(7),
	SERVER_CLIENTUPDATE_ALL				= 0xffffffff,

	SERVER_MODE_RECORD					= M_Bit(0),
	SERVER_MODE_REPLAY					= M_Bit(1),
	SERVER_MODE_SECONDARY				= M_Bit(2),
	SERVER_MODE_CONSOLEREG				= M_Bit(3),
	SERVER_MODE_NOCONNECT				= M_Bit(4),
	SERVER_MODE_SIMULATE				= M_Bit(5),
	SERVER_MODE_COMMITDEFERRED			= M_Bit(6),
	SERVER_MODE_PAUSE					= M_Bit(7),
	SERVER_MODE_SPAWNWORLD				= M_Bit(8),

	SERVER_PLAYMODE_NORMAL				= 0,
	SERVER_PLAYMODE_DEMO				= 1,

	SERVER_CHANGEWORLD_MIGRATION		= M_Bit(0),
	SERVER_CHANGEWORLD_KEEPGAME			= M_Bit(1),
	SERVER_CHANGEWORLD_KEEPRESOURCES	= M_Bit(2),
	SERVER_CHANGEWORLD_DONTKEEPXWRES	= M_Bit(3),
	SERVER_CHANGEWORLD_INITNETWORK		= M_Bit(4),
	
	SERVER_CLS_RESOURCE					= 0,
	SERVER_CLS_GAMESTATE				= 1,
	SERVER_CLS_DOPRECACHE				= 2,
	SERVER_CLS_WAITPRECACHE				= 3,
	SERVER_CLS_ENTERGAME				= 4,

	SERVER_SPAWNFLAGS_DIFFICULTY_BASE	= 0,
	SERVER_SPAWNFLAGS_DIFFICULTY_MASK	= DBitRange(0, 3),
	SERVER_SPAWNFLAGS_GLOBAL_BASE		= 4,
	SERVER_SPAWNFLAGS_GLOBAL_MASK		= DBitRange(4, 11),
	SERVER_SPAWNFLAGS_LAYERS_BASE		= 12,
	SERVER_SPAWNFLAGS_LAYERS_MASK		= DBitRange(12, 19),
	SERVER_SPAWNFLAGS_CUSTOM_BASE		= 20,
	SERVER_SPAWNFLAGS_CUSTOM_MASK		= DBitRange(20, 30),
	SERVER_SPAWNFLAGS_ONCE				= M_Bit(31),

	SERVER_SPAWNFLAGS_XDF_MASK			= (SERVER_SPAWNFLAGS_GLOBAL_MASK|SERVER_SPAWNFLAGS_LAYERS_MASK),

	SERVER_SPAWNFLAGS_GLOBAL = (SERVER_SPAWNFLAGS_DIFFICULTY_MASK | SERVER_SPAWNFLAGS_GLOBAL_MASK),
	SERVER_SPAWNFLAGS_LOCAL  = (SERVER_SPAWNFLAGS_LAYERS_MASK | SERVER_SPAWNFLAGS_CUSTOM_MASK | SERVER_SPAWNFLAGS_ONCE),
};


// -------------------------------------------------------------------
class CPlayer : public CWC_Player
{
public:
	int m_iObject;
	int m_bCtrlValid;

	CPlayer()
	{
		m_iObject = -1;
		m_bCtrlValid = true;
	}
};



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWorld_Server
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWorld_Server : public CWorld_PhysState
{

	MRTC_DECLARE;

public:
	CStr m_ServerName;
	CStr m_WorldName;
	CStr m_ClientClass;
	CStr m_GameType;
	CStr m_NextGameType;
	int m_CurSpawnMask;
	int m_PrevSpawnMask;
	bool m_bSkipSound;

	spCNetwork m_spNetwork;
	int m_hServerSocket;
	int m_ServerMode;
	int m_PlayMode;

#ifndef M_RTM
	TArray<CStr> m_lMessageLog;
	bool m_bLogMessages; // Add messages to message log array for viewing during ogier simulation
	bool m_bConsoleLogMessages; // Log messages to console, for ingame viewing0
#endif

	virtual void Create(int _Mode, spCRegistry _spGameReg, spCWorldData _spWData) pure;
	virtual void OnRegistryChanged() pure;

	virtual void PreDisconnect() pure;
	M_INLINE int GetServerMode() { return m_ServerMode; }
	M_INLINE int GetPlayMode() { return m_PlayMode; }

public:
	virtual bool IsLoading() pure;

	// Change world
	virtual void World_Change(const CFStr &_WorldName, int _Flags) pure;		// Use only by external force, such as CGameContext
	virtual bool World_ChangeFromGame(CStr _World, int _Flags) pure;	// Use only from within the game (from CWObject:s)

	virtual void World_SetGame(CStr _Name) pure;
	virtual bool World_Pause(bool _bPause) pure;

	virtual void SetUpdate(int _iClient, int _Mask) pure;
	virtual void SetUpdate_All(int _Mask) pure;


	// Spawning from XW or XTZ
public:
	virtual void World_PushSpawnTargetNamePrefix(CStr _Prefix) pure;
	virtual void World_PopSpawnTargetNamePrefix() pure;
	virtual CStr World_GetSpawnTargetNamePrefix() pure;
	virtual CStr World_MangleTargetName(CStr _TargetName) pure;

	virtual void World_SpawnObjectFromKeys(const CRegistry& _Obj, const CMat4Dfp32* _pTransform) pure;
	virtual void World_SpawnObjectsFromKeys(CRegistryDirectory *_pRegDir, const CMat4Dfp32* _pTransform = NULL, bool _bSkipWorldSpawn = false) pure;
	virtual void World_SpawnObjectsFromWorld(CStr _Name, const CMat4Dfp32* _pTransform = NULL, bool _bSkipWorldSpawn = false) pure;
	virtual CRegistryDirectory *World_LoadObjects(CStr _Name) pure;

	virtual uint32 World_TranslateSpawnFlags(const char* _pFlagsStr) const pure;
	virtual bool World_TestSpawnFlags(uint32 _Flags, bool _bMaskPrevFlags = false) const pure;
	virtual bool World_TestSpawnFlags(const char* _pFlagsStr, bool _bMaskPrevFlags = false) const pure;
	virtual const char* World_GetCurrSpawnFlagsStr(uint32 _IgnoreMask = 0) const pure;

	virtual void World_CleanResources1(int _Flags) pure;
	virtual void World_CleanResources2(int _Flags) pure;

public:
	virtual void World_Save(CStr _SaveName, bool _bDotest = false) pure;
	virtual void World_Load(CStr _SaveName) pure;
	virtual bool World_SaveExist(CStr _SaveName) pure;

	virtual int Client_Connect(int _hCon) pure;
	virtual int Client_ConnectLocal(spCWorld_Client _spLocal) pure;
	virtual int Client_GetPlayer(int _iClient) pure;
	virtual void Client_SetPlayer(int _iClient, int _iPlayer) pure;
	virtual int Client_GetPing(int _iClient) pure;
	virtual int Client_GetCount() pure;
	virtual uint32 Client_ClientsToWaitFor(int _iClient = -1, int _iNotClient = -1) pure;
	
public:
	virtual void World_SetModel(int _iModel) pure;			// Model to use for scenegraph and navigation.
	virtual CXR_Model* World_GetModel() pure;

	// Network messaging. (only used internally by server)
	virtual bool Net_PutMsg(int _iClient, const CNetMsg& _Msg) pure;
	virtual bool Net_PutMsgEx(int _iClient, const CNetMsg& _Msg, int _PVSNode = -1, int _State = -1, int _MaxLagFrames = -1) pure;
	virtual void Net_PutMsgAll(const CNetMsg& _Msg, int _PVSNode = -1, int _State = -1, int _MaxLagFrames = -1) pure;

	virtual const CNetMsg* Net_GetMsg(int _iClient) pure;
	virtual bool Net_FlushClientMsg(int _iClient) pure;
	virtual void Net_FlushMessages() pure;

	virtual void Net_CheckPendingConnections() pure;
	virtual void Net_DestroyClient(int _iClient, int _Reason = 0) pure;
	virtual void Net_OnMessage(int _iClient, const CNetMsg& _Msg) pure;

	virtual void Net_Refresh() pure;
	virtual bool Net_ClientCommand(int _iClient, const CStr& _Cmd) pure;
	virtual bool Net_ClientCommandAll(const CStr& _Cmd) pure;

	// -------------------------------------------------------------------
	//  Public game interface below. Everything above is system interface.
	// -------------------------------------------------------------------
	virtual int Net_ConOut(const CStr& _s, int _iClient = -1) pure;				// Output to client's consoles.
	virtual void Net_SetClientVar(int _iClient, const CStr& _Key, const CStr& _Value) pure;
/*
	virtual fp32 GetTimeScale() pure;	
	virtual fp32 GetGameTickRealTime() pure;
	virtual fp32 GetGameTickTime() pure;
	virtual int GetGameTick() pure;
	virtual CMTime GetGameTime() pure;
*/
	// World physics
	virtual bool Phys_IntersectWorld(const CSelection* _pSelection, const CWO_PhysicsState& _PhysState, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, int _iExcludeObj = -1, CCollisionInfo* _pCollisionInfo = NULL, int _NotifyFlags = 0, CSelection* _pNotifySelection1 = NULL, CSelection* _pNotifySelection2 = NULL) pure;

	// Rigid Body Physics:
	virtual void Phys_SetCollisionPrecision(int _Level) pure;
	virtual int Phys_GetCollisionPrecision() pure;


	virtual void Phys_AddRigidBodyToSimulation(uint16 _iObject, bool _bActivateConnected) pure;
	virtual void Phys_AddImpulse(uint16 _iObject, const CVec3Dfp32& _ApplyAt, const CVec3Dfp32& _Velocity, fp32 _Mass, fp32 _Restitution) pure;
	virtual void Phys_AddImpulse(uint16 _iObject, const CVec3Dfp32& _ApplyAt, const CVec3Dfp32& _Force) pure;
	virtual void Phys_AddImpulse(CWD_RigidBody2 *_pRB,const CVec3Dfp32& _ApplyAt, const CVec3Dfp32& _Force) pure;
	virtual void Phys_AddMassInvariantImpulse(uint16 _iObject, const CVec3Dfp32& _ApplyAt, const CVec3Dfp32& _Force) pure;
	virtual void Phys_AddForce(uint16 _iObject, const CVec3Dfp32& _Force) pure;
	virtual void Phys_AddForce(CWD_RigidBody2 *_pRB,const CVec3Dfp32& _Force) pure;
	virtual void Phys_SetStationary(uint16 _iObject, bool _Stationary) pure;
	virtual bool Phys_IsStationary(uint16 _iObject) pure;

	virtual int Phys_AddBallJointConstraint(uint16 _iObject1, uint16 _iObject2, const CVec3Dfp32& _WorldPos, fp32 _MaxAngle, uint16 _iSub1 = 0xFFFF, uint16 _iSub2 = 0xFFFF) pure;
	virtual int Phys_AddBallJointToWorld(uint16 _iObject, const CMat4Dfp32& _WorldPos, fp32 _MaxAngle = 0.5f, uint16 _iSub = 0xFFFF) pure;
	virtual int Phys_AddAxisConstraint(uint16 _iObject, const CMat4Dfp32& _WorldPos, fp32 _AxisLength, fp32 _MaxAngle = 0.5f, uint16 _iSub = 0xFFFF) pure;
	virtual int Phys_AddAxisConstraint2(uint16 _iObject, const CMat4Dfp32& _WorldPos, fp32 _AxisLength, fp32 _MinAngle, fp32 _MaxAngle, bool _bUseOriginalFreezeThreshold = false, uint16 _iSub = 0xFFFF) pure;
	virtual int Phys_AddHingeJointConstraint(uint16 _iObject1, uint16 _iObject2, const CMat4Dfp32& _WorldPos, fp32 _AxisLength, fp32 _MaxAngle = 0.5f, uint16 _iSub1 = 0xFFFF, uint16 _iSub2 = 0xFFFF) pure;
	virtual int Phys_AddHingeJointConstraint2(uint16 _iObject1, uint16 _iObject2, const CMat4Dfp32& _WorldPos, fp32 _AxisLength, fp32 _MinAngle, fp32 _MaxAngle, uint16 _iSub1 = 0xFFFF, uint16 _iSub2 = 0xFFFF) pure;

	virtual class CWD_ConstraintDescriptor* Phys_GetConstraint(int _iConstraint) pure;
	virtual void Phys_RemoveConstraint(int _iConstraint) pure;
	virtual void Phys_GetConnectedObjects(int _iConstraint, uint16* _piObject1, uint16* _piObject2) pure;
	virtual void Phys_UpdateConnectedObject(int _iConstraint, uint16 _iOldObject, uint16 _iNewObject) pure;
	virtual void Phys_UpdateBallConstraint(int _iConstraint, const CVec3Dfp32& _WorldPos) pure;
	virtual void Phys_UpdateAxisConstraint(int _iConstraint, const CMat4Dfp32& _WorldPos, fp32 _AxisLength) pure;
	virtual void Phys_UpdateAxisConstraintAngles(int _iConstraint, fp32 _MinAngle, fp32 _MaxAngle) pure;

	virtual void Phys_GetPhysicalProperties(const CWO_PhysicsState& _PhysState, fp32& _Mass, fp32& _Volume) pure;
	virtual void Phys_GetSystemMass(uint16 _iObject, fp32& _TotalMass, fp32& _MaxMass) pure;
	virtual const class CCollisionEvent* Phys_GetCollisionEvents(class CRigidBody *pRigidBody) pure;

	// Registry management
public:
	virtual CRegistry* Registry_GetGame() pure;
	virtual CRegistry* Registry_GetServer() pure;
	virtual CRegistry* Registry_GetWorld() pure;
	virtual CRegistry* Registry_GetTemplates() pure;
	virtual CRegistry* Registry_GetClientVar(int _iClient) pure;
	virtual CRegistry* Registry_GetClientRoot(int _iClient) pure;
	virtual CRegistry* Registry_GetClient(int _iClient, int _iDirectory) pure;
	virtual CRegistry* Registry_GetLevelKeys(CStr _Section, CStr _Level = "") pure;

	// -------------------------------------------------------------------
	//  Object management
public:
	virtual CWObject_CoreData* Object_GetCD(int _iObj) pure;
	virtual CWObject* Object_Get(int _iObj) pure;
	virtual CWObject* Object_GetWithGUID(int _ObjGUID) pure;
	virtual class CWS_ClientObjInfo* Object_GetPerClientData(int _iObj, int _iClient) pure;

	virtual int Object_GetGUID(int _iObj) pure;								// Translate index to GUID
	virtual int Object_GetIndex(int _ObjGUID) pure;							// Translate GUID to index

	virtual int Object_SetDirty(int _iObj, int _Mask) pure;
	virtual int Object_SetDirtyTree(int _iObj, int _Mask) pure;

	virtual const char* Object_GetName(int _iObj) pure;
	virtual uint32 Object_GetNameHash(uint _iObj) pure;

	virtual void Object_SetName(int _iObj, const char* _pName) pure;
	virtual void Object_SetModel(int _iObj, int _iModel, int _ModelNr = 0) pure;// Helper function
	virtual void Object_SetSound(int _iObj, int _iSound, int _SoundNr = 0) pure;// Helper function

	// Object hierarchy
	virtual bool Object_AddChild(int _iParent, int _iChild) pure;			// Makes iChild a child if iParent, if _iParent is zero, _iChild is unconnected from any parent it might have.

	// Object creation
	virtual void Object_EvalKey(int _iObj, const CStr _Key, const CStr _Value) pure;

	virtual int Object_Create(const CRegistry &_Keys, const CMat4Dfp32 *_pPos = NULL, int _iOwner = -1, const aint* _pParam = NULL, int _nParam = 0, int _iObj = -1, uint32 _GUID = 0) pure;
	virtual int Object_Create(const char* _pClassName, CVec3Dfp32 _Pos = 0, int _iOwner = -1, const aint* _pParam = NULL, int _nParam = 0, int _iObj = -1, uint32 _GUID = 0) pure;
	virtual int Object_Create(const char* _pClassName, const CMat4Dfp32& _Pos, int _iOwner = -1, const aint* _pParam = NULL, int _nParam = 0, int _iObj = -1, uint32 _GUID = 0) pure;
	
	virtual void Object_Destroy(int _iObj, bool _bForceSafeDestroy = false) pure;					// Safe destruction. Doesn't delete object until end of game tick. Use this if you destroy the object from the object itself.
	virtual void Object_DestroyInstant(int _iObj) pure;			// Destroys object immediately. Use with caution. This method have identical behaviour to that of the original Object_Destroy()

	virtual void Object_InitRigidBody(int _iObj, bool _bDebris) pure;

	// Navigation services
	virtual CXR_PathInterface* Path_GetInterface(int _iObj) pure;		// returned in Path_GetInterface.
	virtual class CXR_BlockNav* Path_GetBlockNav() pure;
	virtual class CXR_BlockNavSearcher* Path_GetBlockNavSearcher() pure;
	virtual CWorld_Navgraph_Pathfinder * Path_GetNavGraph() pure;

	// -------------------------------------------------------------------
	//  Selection management (server-only additions, see CWorld_PhysState for more information)
	virtual void Selection_AddTarget(CSelection& _selection, uint32 _NameHash) pure;
	virtual void Selection_AddTarget(CSelection& _selection, const char* _pName) pure;
	virtual void Selection_AddTransitZone(CSelection& _selection, int _iTZObject) pure;

	// NOTE: The Selection_RemoveOnXxxx family of functions are not fully implemented.
	virtual void Selection_RemoveOnTarget(CSelection& _selection, CStr _TargetName) pure;
	virtual void Selection_RemoveOnNotTarget(CSelection& _selection, CStr _TargetName) pure;


	virtual int Selection_GetSingleTarget(uint32 _NameHash) pure;	// Find a single object. (Randomized if target matches several objects)
	virtual int Selection_GetSingleTarget(const char* _pName) pure;	// Find a single object. (Randomized if target matches several objects)

	// -------------------------------------------------------------------
	//  Message management
	virtual aint Message_SendToObject(const CWObject_Message& _Msg, int _iObj) pure;
	virtual aint Message_SendToObjectGUID(const CWObject_Message& _Msg, int _GUID) pure;
	virtual void Message_SendToTarget(const CWObject_Message& _Msg, uint32 _NameHash) pure;
	virtual void Message_SendToTarget(const CWObject_Message& _Msg, const char* _pName) pure;
	virtual void Message_SendToSelection(const CWObject_Message& _Msg, const CSelection& _selection) pure;

	virtual bool MessageQueue_SendToObject(const CWObject_Message& _Msg, int _iObj) pure;		// false if msg-buffer is full
	virtual void MessageQueue_Flush() pure;

	// -------------------------------------------------------------------
	//  NetMsg management
	virtual void NetMsg_SendToObject(const CNetMsg& _Msg, int _iObj, int _MaxLag = 4) pure;
	virtual void NetMsg_SendToClass(const CNetMsg& _Msg, int _iClass, int _MaxLag = 4) pure;

	virtual void NetMsg_SendToObject_Culled(const CVec3Dfp32& _PVSPos, const CNetMsg& _Msg, int _iObj = 0, int _MaxLag = 4) pure;
	virtual void NetMsg_SendToObject_Culled(const CBox3Dfp32& _PVSPos, const CNetMsg& _Msg, int _iObj = 0, int _MaxLag = 4) pure;

	virtual void NetMsg_SendToClient(const CNetMsg& _Msg, int _iClient = -1, int _MaxLag = 4) pure;
	virtual void NetMsg_SendToClient_Culled(const CVec3Dfp32& _PVSPos, const CNetMsg& _Msg, int _iClient = -1, int _MaxLag = 4) pure;
	virtual void NetMsg_SendToClient_Culled(const CBox3Dfp32& _PVSPos, const CNetMsg& _Msg, int _iClient = -1, int _MaxLag = 4) pure;

	// -------------------------------------------------------------------
	//  Instant-sound services
	virtual void Sound_Global(int _iSound, fp32 _Volume = 1.0f, int _iClient = -1) pure;
	virtual void Sound_GlobalCutscene(int _iSound, fp32 _Volume = 1.0f, int _iClient = -1, fp32 _Delay = 0, uint32 _GroupID = 0) pure;
	virtual void Sound_At(const CVec3Dfp32& _Pos, int _iSound, int _AttnType, uint8 _iMaterial = 0, fp32 _Volume = 1.0f, int _iClient = -1, CVec3Dfp32 _V0 = 0.0f) pure;
	virtual void Sound_On(int16 _iObject, int _iSound, int _AttnType, uint8 _iMaterial=0, fp32 _Volume = 1.0f, int _iClient = -1, const CVec3Dfp32& _V0 = 0, const CVec3Dfp32& _Offset = 0, uint32 _GroupID = 0) pure;
	virtual void Sound_Off(int16 _iObject, int _iSound, uint8 _iMaterial = 0, int _iClient = -1) pure;
	virtual void Sound_GlobalNotClient(int _iClient, int _iSound, fp32 _Volume = 1.0f) pure;
	virtual void Sound_GlobalCutsceneNotClient(int _iClient, int _iSound, fp32 _Volume = 1.0f, fp32 _Delay = 0) pure;
	virtual void Sound_AtNotClient(int _iClient, const CVec3Dfp32& _Pos, int _iSound, int _AttnType, uint8 _iMaterial = 0, fp32 _Volume = 1.0f, CVec3Dfp32 _V0 = 0.0f) pure;
	virtual void Sound_OnNotClient(int _iClient, int16 _iObject, int _iSound, int _AttnType, uint8 _iMaterial=0, fp32 _Volume = 1.0f, const CVec3Dfp32& _V0 = 0, const CVec3Dfp32& _Offset = 0) pure;
	virtual void Sound_OffNotClient(int _iClient, int16 _iObject, int _iSound, uint8 _iMaterial = 0) pure;

public:
	// -------------------------------------------------------------------
	// GameObj
	virtual void Game_Register(int _iGame) pure;
	virtual void Game_Unregister() pure;
	virtual int Game_GetObjectIndex() pure;
	virtual class CWObject_Game *Game_GetObject() pure;

	// -------------------------------------------------------------------
	//  Script
public:

	// -------------------------------------------------------------------
	//  System stuff, don't even think of calling these!
	// -------------------------------------------------------------------
	virtual void Simulate_SetCurrentLocalFrameFraction(fp32 _LocalFrameFraction) pure;
	virtual bool Simulate(class CPerfGraph* _pPerfGraph = NULL) pure;
	virtual bool SimulateAhead(CPerfGraph* _pPerfGraph, fp32 _MaxTime) pure;

	virtual void Render(CXR_VBManager* _pVBM, const CMat4Dfp32& _Camera, int _Context) pure;	// Debug rendering only.
	virtual CRenderContext* Debug_GetRender() pure;
	virtual CDebugRenderContainer* Debug_GetWireContainer(uint _Flags = 3) pure;

	virtual void ClearStatistics() pure;
	virtual void DumpStatistics() pure;
	virtual void Dump(int _DumpFlags) pure;
};

typedef TPtr<CWorld_Server> spCWorld_Server;

#endif // _INC_WSERVER


