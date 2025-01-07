#ifndef __WSERVER_CORE_H
#define __WSERVER_CORE_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Inherited server for faster compilation and less dependecies

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CMsgQueueElement
					CWServer_ClientInfo
					CWS_GUIDElement
					CWS_GUIDHash
					CWS_TargetSearchTree
					CWS_ObjectPerfGraph
					CWorld_ServerCore
\*____________________________________________________________________________________________*/

#include "WServer.h"
#include "../WObjCore.h"
#include "../WPhysState.h"
#include "../Client/WClient.h"
#include "../../../MSystem/Misc/MRegistry_Compiled.h"
#include "../WObjCore.h"
#include "../WDynamicsEngine.h"
#include "../WDynamicsEngine/WDynamicsEngine2.h"
#include "../WDynamicsEngine/WDynamics2.h"
#include "../WDynamicsEngine/WDynamicsConstraint.h"

//class CWObjectAttributes;

// -------------------------------------------------------------------
#define SERVER_MESSAGEQUEUESIZE		128

#define SERVER_NUMMEDIUMS			12

#define WSERVER_GAMEOBJNAME			"__GAMECONTROL__"

#define WSERVER_MAXCLIENTS			256

#ifdef PLATFORM_CONSOLE
	#define WSERVER_NOLOCALMIRROR
#endif

//#define WCLIENT_REG_GAMESTATE		0

#define WSERVER_DYNAMICSENGINE2

// -------------------------------------------------------------------
class CMsgQueueElement
{
public:
	CWObject_Message m_Msg;
	int m_iDest;

	CMsgQueueElement()
	{
		m_iDest = -1;
	}

	CMsgQueueElement(const CWObject_Message& _Msg, int _iDest) : m_Msg(_Msg), m_iDest(_iDest)
	{
	}
};

// -------------------------------------------------------------------
class CWServer_ClientInfo : public CReferenceCount
{
public:
	CNetMsgPack m_OutMsg;
	CNetMsgPack m_InMsg;

	spCWorld_Client m_spLocalClient;	// (!NULL) ? Local : Remote
	int m_hConnection;					// (m_hConnection == -1) ? Local : Remote
	int m_LastPing;
	int m_iPlayer;
	int m_State;
	int m_ResourceNr;
	int m_ChangeLevelState;
	int m_ServerUpdateMask;
	int m_bEatAllInput : 1;

	int m_iLastPlayerUpdate;

	int m_nClientSimTicks;

	const uint8* m_pPVS;

	// Out-frames
	bool m_bFrameOut;
	bool m_bFrameComplete;
	CWObjectUpdateBuffer m_FrameOut;

	spCRegistry m_spClientReg;
//	TArray<spCRegistry> m_lspNetReg;		// List of server-registries to be replicated to client
	spCRegistry m_spNetReg;

	bool m_bRegFrameOut;
	bool m_bRegFrameComplete;
	CWObjectUpdateBuffer m_RegFrameOut;

	TArray<CWS_ClientObjInfo> m_lObjInfo;	// Parallell array to server's m_lspObjects, containing client-specific data.

	CWServer_ClientInfo();

	bool IsLocal() { return (m_hConnection == -1); };
	bool IsRemote() { return (m_hConnection != -1); };
};

typedef TPtr<CWServer_ClientInfo> spCWServer_ClientInfo;

// -------------------------------------------------------------------
//  CWS_GUIDHash
// -------------------------------------------------------------------
class CWS_GUIDElement
{
public:
	int32 m_GUID;
};

class CWS_GUIDHash : public THash<int16, CWS_GUIDElement>
{
public:
	void Create(int _nIndices);
	void Insert(int _Index, int _GUID);
	int GetIndex(int _GUID);
};

// -------------------------------------------------------------------
/*
class CWS_TargetSearchTree : public TSearchTree<CTargetNameElement, 0>
{
public:
	CTargetNameTreeNode& Insert(const CTargetNameElement& _Elem)
	{
		if (Tree==NULL)
		{
			Tree = DNew(CTargetNameTreeNode) CTargetNameTreeNode(_Elem, NULL);
			if (Tree == NULL) MemError("Insert");
			return *Tree;
		}
		else
			return _Insert(*Tree,_Elem);
	}

	CTargetNameTreeNode& _Insert(CTargetNameTreeNode& Node, const CTargetNameElement& Elem)
	{
		if (Elem < Node.GetElement()) 
		{	
			if (Node.GetChild(0)==NULL) 
			{
				Node.AddChild(0,Elem);
				return *Node.GetChild(0);
			}
			else
				return _Insert(*Node.GetChild(0),Elem);
			
		}
		else if (Elem == Node.GetElement())
		{
			if (Node.GetChild(1)==NULL) 
			{
				Node.AddChild(1,Elem);
				return *Node.GetChild(1);
			}
			else
			{
				CTargetNameTreeNode* pChild1 = Node.GetChild(1);
				Node.SetChild(1, NULL);
				Node.AddChild(1, Elem);
				Node.GetChild(1)->SetChild(1, pChild1);
				
				return *Node.GetChild(1);
			}
		}
		else
		{	
			if (Node.GetChild(1)==NULL) 
			{
				Node.AddChild(1,Elem);
				return *Node.GetChild(1);
			}
			else
				return _Insert(*Node.GetChild(1),Elem);
			
		}
	}
};
*/



class CWS_TargetSearchTree
{
public:
	DIdsTreeAVLAligned_Tree(CTargetNameElement, m_NameLink, CTargetNameElement, CTargetNameElement::CCompare) m_Tree;

	void Clear()
	{
		if (m_Tree.GetRoot())
		{
			LogFile("-------------------------------------------------------------------");

			DIdsTreeAVLAligned_Iterator(CTargetNameElement, m_NameLink, CTargetNameElement, CTargetNameElement::CCompare) Iter = m_Tree;
			int iElem = 0;
			while(Iter)
			{
				CTargetNameElement* pNode = Iter;

				CWObject* pObj = pNode->m_pObj;
				if (pObj)
				{
					ConOutL(CStrF("%.5d, 0x%.8x, pObj %.8x", iElem, pNode->m_NameHash, pObj));

				}

				iElem++;
				++Iter;
			}
			ConOutL("§cf80WARNING: (~CWS_TargetSearchTree) Target name tree not empty on destruction.");
			M_ASSERT(0, "Should be empty.");
		}

		m_Tree.f_DeleteAll();
	}

	~CWS_TargetSearchTree()
	{
		Clear();
	}

	CTargetNameElement* Insert(CTargetNameElement& _Elem)
	{
		CTargetNameElement* pElem = m_Tree.FindEqual(_Elem);
		if (pElem)
			return pElem;
		pElem = DNew(CTargetNameElement) CTargetNameElement(_Elem);
		if (!pElem)
			Error_static(M_FUNCTION, "Insert");
		m_Tree.f_Insert(pElem);
		return pElem;
	}

	void Remove(CTargetNameElement* _pElem)
	{
		m_Tree.f_Remove(_pElem);
		delete _pElem;
	}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWC_ObjectPerfGraph
|__________________________________________________________________________________________________
\*************************************************************************************************/
#ifndef DEF_DISABLE_PERFGRAPH

class CWS_ObjectPerfGraph : public CObj
{
public:
	CWS_ObjectPerfGraph();
	void Create();

	int m_LastTouch_OnRefresh;
	CMTime m_Time_OnRefresh;
	TPtr<CPerfGraph> m_spGraph_OnRefresh;
};

#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TDummySet2
|__________________________________________________________________________________________________
\*************************************************************************************************/
template<typename T, int TSize>
class TDummySet2
{
public:
	TDummySet2()
		: m_Length(0) { }

	bool Add(T _x)
	{
		uint n = m_Length;
		for (uint i = 0; i < n; i++)
		{
			if (m_pV[i] == _x)
				return true;
		}

		if (m_Length >= TSize)
			return false;

		m_pV[m_Length++] = _x;
		return true;
	}

	bool Contains(T _x) const
	{
		uint n = m_Length;
		for (uint i = 0; i < n; i++)
			if (m_pV[i] == _x)
				return true;

		return false;
	}

	T m_pV[TSize];
	uint m_Length;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWorld_Server
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWorld_ServerCore : public CWorld_Server
{
	MRTC_DECLARE;
	typedef CWorld_ServerCore ThisClass;
	friend class CWorld_DeltaGameState;
	friend class CWObject_GameCampaign;

public:
	int32 m_bCharacterPush;

protected:
	bool XDFStart(const char *_pName);
	void XDFStop();

	/*
		Dynamics
	 */
#ifndef WSERVER_DYNAMICSENGINE2

	CWorld m_DynamicsWorld;
#else

	CWD_DynamicsWorld m_DynamicsWorld2;
	CWD_WorldModelCollider m_WorldModelCollider;
	TArray<CWD_ConstraintDescriptor> m_lConstraints;
	int m_NextConstraintID;
#endif


	bool m_bNetShowMsg;
	int m_iRConClient;					// Set before client network commands are executed

	// Object heap & hashing.
	TArray<spCWObject> m_lspObjects;
	TPtr<CIDHeap> m_spObjectHeap;

	TPtr<CWS_GUIDHash> m_spGUIDHash;

#ifndef DEF_DISABLE_PERFGRAPH
	TArray<CWS_ObjectPerfGraph> m_PerfGraph_lObjects;
	int m_PerfGraph;
	int m_PerfGraph_Tick_OnRefresh;
#endif

	// Searchtree for targets
	CWS_TargetSearchTree m_NameSearchTree;
//	TSearchTree<CTargetNameElement, 0> m_NameSearchTree;

	int32 m_Simulate_iNextObject;

#ifdef	SERVER_STATS
	CMTime m_Simulate_CPUTime;
#endif
	fp32 m_Simulate_LocalFrameFraction;

	// World stuff
	int32 m_iWorldModel;

	// Message buffer
	TPtr<TQueue<CMsgQueueElement> > m_spMsgQueue;

	// Used 
//	CFStr m_World_PendingName;
//	int32 m_World_PendingFlags;
	int m_bWorld_PendingChange;
	
#ifdef M_ENABLE_REGISTRYCOMPILED
	CRegistryCompiled m_ServerRegCompiled;
#endif

	spCRegistry m_spGameReg;					// Registry for the game.
	spCRegistry m_spServerReg;					// Registry for this server.
	spCRegistry m_spWorldReg;					// Registry for the current world.
	TArray<spCRegistry> m_lspNetReg;		// List of server-registries that are replicated to clients.
	spCRegistry m_spCurLevelKeys;

	// World-registry variables.
	bool m_bPendingPrecacheDoUnpause;

	spCRegistry m_spTemplateReg;

	// Physics debug-rendering
	int32 m_PhysRenderFlags;
	TPtr<class CRenderContextCapture> m_spCapture;

	// for fast game object fetching
	int m_iGame;
	int32	m_Loading;

	// Load progress helpers
	int32 m_SpawnLen;
	int32 m_SpawnCurrentObject;

	// Used by DeltaGameState to keep MapData resource indices intact when re-entering a level with different spawnmask
	TThinArray<uint32> m_lResourceNameHashes;

public:
	int32 m_NextGUID;

	int32 m_MaxPlayers;
	TArray<spCWorld_Client> m_lspClients;
	TArray<spCWServer_ClientInfo> m_lspClientInfo;

	CWorld_ServerCore();
	~CWorld_ServerCore();
	virtual void Create(int _Mode, spCRegistry _spGameReg, spCWorldData _spWData);
	virtual void OnRegistryChanged();

	virtual void PreDisconnect();

	virtual bool IsLoading()
	{
		return m_Loading != 0;
	}

	void SetGameTick(int _GameTick);

public:
	// Change world
	virtual void World_Change(const CFStr &_WorldName, int _Flags);		// Use only by external force, such as CGameContext
	virtual bool World_ChangeFromGame(CStr _World, int _Flags);	// Use only from within the game (from CWObject:s)

	virtual void World_SetGame(CStr _Name);

	virtual bool World_Pause(bool _bPause);

protected:
	virtual void SetUpdate(int _iClient, int _Mask);
	virtual void SetUpdate_All(int _Mask);
	virtual void World_InitPlayers();
	virtual void World_Init(CStr _WorldName);
	virtual void World_InitGameObject();
	virtual void World_Close();

	virtual void World_DoOnSpawnWorld();

	// Spawning from XW or XTZ
protected:
	TArray<CStr> m_lSpawnTargetNamePrefixStack;

public:
	virtual void World_PushSpawnTargetNamePrefix(CStr _Prefix);
	virtual void World_PopSpawnTargetNamePrefix();
	virtual CStr World_GetSpawnTargetNamePrefix();
	virtual CStr World_MangleTargetName(CStr _TargetName);

	virtual void World_SpawnObjectFromKeys(const CRegistry& _Obj, const CMat4Dfp32* _pTransform);
	virtual void World_SpawnObjectsFromKeys(CRegistryDirectory *_pRegDir, const CMat4Dfp32* _pTransform = NULL, bool _bSkipWorldSpawn = false);
	virtual void World_SpawnObjectsFromWorld(CStr _Name, const CMat4Dfp32* _pTransform = NULL, bool _bSkipWorldSpawn = false);
	virtual CRegistryDirectory *World_LoadObjects(CStr _Name);

	virtual uint32 World_TranslateSpawnFlags(const char* _pFlagsStr) const;
	virtual bool World_TestSpawnFlags(uint32 _Flags, bool _bMaskPrevFlags = false) const;
	virtual bool World_TestSpawnFlags(const char* _pFlagsStr, bool _bMaskPrevFlags = false) const;
	virtual const char* World_GetCurrSpawnFlagsStr(uint32 _IgnoreMask = 0) const { return NULL; }

	virtual void World_CleanResources1(int _Flags);
	virtual void World_CleanResources2(int _Flags);

public:
	virtual void World_Save(CStr _SaveName, bool _bDotest = false) {}
	virtual void World_Load(CStr _SaveName) {}
	virtual bool World_SaveExist(CStr _SaveName) { return false; }
	virtual bool World_DeleteSave(CStr _Name) { return false; }

	virtual fp32 Load_GetProgress();
	virtual void Load_ProgressReset();

protected:
	virtual bool CreateClientUpdate(int _iClient, CWObjectUpdateBuffer* _pDeltaBuffer, int _bAllObjects);
	virtual bool CreateClientRegUpdate(int _iClient, CWObjectUpdateBuffer* _pDeltaBuffer);
	virtual void UnpackServerRegUpdate(int _iClient, CWObjectUpdateBuffer* _pDeltaBuffer);

protected:
	virtual void Client_Init(int _iClient);
	virtual void Client_SetNumRegDirectories(int _iClient, int _nReg);
	virtual CVec3Dfp32 Client_GetPVSPosition(CWServer_ClientInfo* _pCInfo);
	virtual int Client_GetNumLagFrames(int _iClient);
	virtual CWorld_Client* Client_Get(int _iClient);
	virtual CWServer_ClientInfo* Client_GetInfo(int _iClient);

public:
	virtual int Client_Connect(int _hCon);
	virtual int Client_ConnectLocal(spCWorld_Client _spLocal);
	virtual int Client_GetPlayer(int _iClient);
	virtual void Client_SetPlayer(int _iClient, int _iPlayer);
	virtual int Client_GetPing(int _iClient);
	virtual int Client_GetCount();
	virtual uint32 Client_ClientsToWaitFor(int _iClient = -1, int _iNotClient = -1);
	
public:
	virtual void World_SetModel(int _iModel);			// Model to use for scenegraph and navigation.
	virtual CXR_Model* World_GetModel();

	// Network messaging. (only used internally by server)
	virtual bool Net_PutMsg(int _iClient, const CNetMsg& _Msg);
	virtual bool Net_PutMsgEx(int _iClient, const CNetMsg& _Msg, int _PVSNode = -1, int _State = -1, int _MaxLagFrames = -1);
	virtual void Net_PutMsgAll(const CNetMsg& _Msg, int _PVSNode = -1, int _State = -1, int _MaxLagFrames = -1);

	virtual const CNetMsg* Net_GetMsg(int _iClient);
	virtual bool Net_FlushClientMsg(int _iClient);
	virtual bool Net_FlushClientConnection(int _iClient);
	virtual void Net_FlushMessages();

	virtual void Net_CheckPendingConnections();
	virtual void Net_DestroyClient(int _iClient, int _Reason = 0);
	static void Net_RConWriteCallback(const CStr& _Str, void* _pContext);
	virtual void Net_OnMessage(int _iClient, const CNetMsg& _Msg);

	virtual void Net_Refresh();
	virtual bool Net_ClientCommand(int _iClient, const CStr& _Cmd);
	virtual bool Net_ClientCommandAll(const CStr& _Cmd);

	// -------------------------------------------------------------------
	//  Public game interface below. Everything above is system interface.
	// -------------------------------------------------------------------
	virtual int Net_ConOut(const CStr& _s, int _iClient = -1);				// Output to client's consoles.
	virtual void Net_SetClientVar(int _iClient, const CStr& _Key, const CStr& _Value);
/*
	virtual fp32 GetTimeScale();
	virtual fp32 GetGameTickRealTime();
	virtual fp32 GetGameTickTime();
	virtual int GetGameTick();
	virtual CMTime GetGameTime();
*/
	// World physics
	virtual bool Phys_IntersectWorld(const CSelection* _pSelection, const CWO_PhysicsState& _PhysState, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, int _iExcludeObj = -1, CCollisionInfo* _pCollisionInfo = NULL, int _NotifyFlags = 0, CSelection* _pNotifySelection1 = NULL, CSelection* _pNotifySelection2 = NULL);
	virtual bool Phys_IntersectWorld(CPotColSet *_pcs, const CWO_PhysicsState& _PhysState, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, int _iExcludeObj = -1, CCollisionInfo* _pCollisionInfo = NULL, int _NotifyFlags = 0, CSelection* _pNotifySelection1 = NULL, CSelection* _pNotifySelection2 = NULL);

	// Rigid Body Physics:
	virtual void Phys_SetCollisionPrecision(int _Level);
	virtual int Phys_GetCollisionPrecision();

	static CMat4Dfp32 Phys_GetCenterOfMassCorrectedTransform(const CWObject* _pObj);

	virtual void Phys_AddRigidBodyToSimulation(uint16 _iObject, bool _bActivateConnected);
	virtual void Phys_AddImpulse(uint16 _iObject, const CVec3Dfp32& _ApplyAt, const CVec3Dfp32& _Velocity, fp32 _Mass, fp32 _Restitution);
	virtual void Phys_AddImpulse(uint16 _iObject, const CVec3Dfp32& _ApplyAt, const CVec3Dfp32& _Force);
	virtual void Phys_AddImpulse(CWD_RigidBody2 *_pRB,const CVec3Dfp32& _ApplyAt, const CVec3Dfp32& _Force);
	virtual void Phys_AddMassInvariantImpulse(uint16 _iObject, const CVec3Dfp32& _ApplyAt, const CVec3Dfp32& _Force);
	virtual void Phys_AddForce(uint16 _iObject, const CVec3Dfp32& _Force);
	virtual void Phys_AddForce(CWD_RigidBody2 *_pRB,const CVec3Dfp32& _Force);
	virtual void Phys_SetStationary(uint16 _iObject, bool _Stationary);
	virtual bool Phys_IsStationary(uint16 _iObject);

	virtual int Phys_AddBallJointConstraint(uint16 _iObject1, uint16 _iObject2, const CVec3Dfp32& _WorldPos, fp32 _MaxAngle, uint16 _iSub1 = 0xFFFF, uint16 _iSub2 = 0xFFFF);
	virtual int Phys_AddBallJointToWorld(uint16 _iObject, const CMat4Dfp32& _WorldPos, fp32 _MaxAngle = 0.5f, uint16 _iSub = 0xFFFF);
	virtual int Phys_AddAxisConstraint(uint16 _iObject, const CMat4Dfp32& _WorldPos, fp32 _AxisLength, fp32 _MaxAngle = 0.5f, uint16 _iSub = 0xFFFF);
	virtual int Phys_AddAxisConstraint2(uint16 _iObject, const CMat4Dfp32& _WorldPos, fp32 _AxisLength, fp32 _MinAngle, fp32 _MaxAngle, bool _bUseOriginalFreezeThreshold = false, uint16 _iSub = 0xFFFF);
	virtual int Phys_AddHingeJointConstraint(uint16 _iObject1, uint16 _iObject2, const CMat4Dfp32& _WorldPos, fp32 _AxisLength, fp32 _MaxAngle = 0.5f, uint16 _iSub1 = 0xFFFF, uint16 _iSub2 = 0xFFFF);
	virtual int Phys_AddHingeJointConstraint2(uint16 _iObject1, uint16 _iObject2, const CMat4Dfp32& _WorldPos, fp32 _AxisLength, fp32 _MinAngle, fp32 _MaxAngle, uint16 _iSub1 = 0xFFFF, uint16 _iSub2 = 0xFFFF);

	virtual class CWD_ConstraintDescriptor* Phys_GetConstraint(int _iConstraint);
	virtual void Phys_RemoveConstraint(int _iConstraint);
	virtual void Phys_GetConnectedObjects(int _iConstraint, uint16* _piObject1, uint16* _piObject2);
	virtual void Phys_UpdateConnectedObject(int _iConstraint, uint16 _iOldObject, uint16 _iNewObject);
	virtual void Phys_UpdateBallConstraint(int _iConstraint, const CVec3Dfp32& _WorldPos);
	virtual void Phys_UpdateAxisConstraint(int _iConstraint, const CMat4Dfp32& _WorldPos, fp32 _AxisLength);
	virtual void Phys_UpdateAxisConstraintAngles(int _iConstraint, fp32 _MinAngle, fp32 _MaxAngle);

	virtual void Phys_GetPhysicalProperties(const CWO_PhysicsState& _PhysState, fp32& _Mass, fp32& _Volume);
	virtual void Phys_GetSystemMass(uint16 _iObject, fp32& _TotalMass, fp32& _MaxMass);
	virtual const class CCollisionEvent* Phys_GetCollisionEvents(class CRigidBody *pRigidBody);

protected:
	void Phys_UpdateHingeJoint(int _iConstraint, int _iOldA, int _iOldB, int _iNewA, int _iNewB);

	void Phys_DoGetSystemMass(uint16 _iObject, fp32& _Sum, fp32& _Max, TDummySet2<uint32, 16>& _Set);
	bool Phys_DoAddConstraint(CWObject_CoreData *_pObj1, CWObject_CoreData *_pObj2, const CWD_ConstraintDescriptor& _Constraint);

	virtual bool Phys_SetPosition(const CSelection& _selection, int _iObj, const CMat4Dfp32& _Pos, CCollisionInfo* _pCollisionInfo = NULL);
	virtual bool Phys_MovePosition(const CSelection* selection, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, CCollisionInfo* _pCollisionInfo = NULL);
	virtual bool Phys_MovePosition(CPotColSet *_pcs, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, CCollisionInfo* _pCollisionInfo = NULL);

public:
	virtual void GetMovementBounds( float *_BoxMinMax, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest );
	virtual void Selection_GetArray( CPotColSet *pcs, const CSelection* _pSelection, const CWO_PhysicsState &_PhysState, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest);

protected:
	// Overrides from CWorld_PhysState
	// AGMerge
	virtual bool IsServer() const { return true; }
	virtual bool IsClient() const { return false; }

	virtual aint Phys_Message_SendToObject(const CWObject_Message& _Msg, int _iObj);
	virtual void Phys_Message_SendToSelection(const CWObject_Message& _Msg, const CSelection& _Sel);
	virtual bool Phys_MessageQueue_SendToObject(const CWObject_Message& _Msg, int _iObj);
	virtual void Phys_MessageQueue_Flush();

	// Registry management
protected:
	virtual void Registry_ClearWorld();									// Destroys the world-registry and creates a new one.
	virtual void Registry_OnReadWorldKeys();							// Called when registry has been loaded.
	virtual void Registry_OnWriteWorldKeys();							// Called before registry is to be stored.
public:
	virtual CRegistry* Registry_GetGame();
	virtual CRegistry* Registry_GetServer();
	virtual CRegistry* Registry_GetWorld();
	virtual CRegistry* Registry_GetTemplates();
	virtual CRegistry* Registry_GetClientVar(int _iClient);
	virtual CRegistry* Registry_GetClientRoot(int _iClient);
	virtual CRegistry* Registry_GetClient(int _iClient, int _iDirectory);
	virtual CRegistry* Registry_GetLevelKeys(CStr _Section, CStr _Level = "");

	// -------------------------------------------------------------------
	//  Object management
protected:
	virtual int Object_HeapSize();
public:
	virtual CWObject_CoreData* Object_GetCD(int _iObj);
	virtual CWObject* Object_Get(int _iObj);
	virtual CWObject* Object_GetWithGUID(int _ObjGUID);
	virtual CWS_ClientObjInfo* Object_GetPerClientData(int _iObj, int _iClient);

	virtual int Object_GetGUID(int _iObj);								// Translate GUID to index
	virtual int Object_GetIndex(int _ObjGUID);							// Translate index to GUID

	virtual int Object_SetDirty(int _iObj, int _Mask);
	virtual int Object_SetDirtyTree(int _iObj, int _Mask);

	virtual bool Object_SetPhysics_DoNotify(int _iObj, const CWO_PhysicsState& _PhysState, const CMat4Dfp32& _Pos);	// Returns true if successful

	virtual const char* Object_GetName(int _iObj);
	virtual uint32 Object_GetNameHash(uint _iObj);

	virtual void Object_SetName(int _iObj, const char* _pName);
	virtual void Object_SetModel(int _iObj, int _iModel, int _ModelNr = 0);// Helper function
	virtual void Object_SetSound(int _iObj, int _iSound, int _SoundNr = 0);// Helper function

	// Object hierarchy
	virtual bool Object_AddChild(int _iParent, int _iChild);			// Makes iChild a child if iParent, if _iParent is zero, _iChild is unconnected from any parent it might have.

	// Object creation
	virtual void Object_EvalKey(int _iObj, const CStr _Key, const CStr _Value);

protected:
	virtual int Object_Insert(spCWObject _spObj, int _iObj = -1);	// Insert an already created object into the world
	virtual int Object_Create(spCWObject _spObj, const CMat4Dfp32 &_Pos, const CRegistry *_pKeys, int _iOwner, const aint* _pParam, int _nParam, int _iObj = -1, uint32 _GUID = 0);
public:
	virtual int Object_Create(const CRegistry &_Keys, const CMat4Dfp32 *_pPos = NULL, int _iOwner = -1, const aint* _pParam = NULL, int _nParam = 0, int _iObj = -1, uint32 _GUID = 0);
	virtual int Object_Create(const char* _pClassName, CVec3Dfp32 _Pos = 0, int _iOwner = -1, const aint* _pParam = NULL, int _nParam = 0, int _iObj = -1, uint32 _GUID = 0);
	virtual int Object_Create(const char* _pClassName, const CMat4Dfp32& _Pos, int _iOwner = -1, const aint* _pParam = NULL, int _nParam = 0, int _iObj = -1, uint32 _GUID = 0);

	virtual void Object_InitRigidBody(int _iObj, bool _bDebris);

	dllvirtual bool Object_ChangeGUID(int _iObject, int _NewGUID);

protected:
	TArray<uint16> m_liObjectDeferredDestroy;
	virtual void Object_DestroyDeferred_r(int _iObj);
	virtual void Object_CommitDeferredDestruction();		// Destroys all objects that were destroyed with Object_Destroy()

public:
	virtual void Object_Destroy(int _iObj, bool _bForceSafeDestroy = false);					// Safe destruction. Doesn't delete object until end of game tick. Use this if you destroy the object from the object itself.
	virtual void Object_DestroyInstant(int _iObj);			// Destroys object immediately. Use with caution. This method have identical behaviour to that of the original Object_Destroy()

	// Navigation services
	virtual CXR_PathInterface* Path_GetInterface(int _iObj);		// returned in Path_GetInterface.
	virtual CXR_BlockNav* Path_GetBlockNav();
	virtual CXR_BlockNavSearcher* Path_GetBlockNavSearcher();
	virtual CWorld_Navgraph_Pathfinder * Path_GetNavGraph();

	// -------------------------------------------------------------------
	//  Selection management (server-only additions, see CWorld_PhysState for more information)
	virtual void Selection_AddTarget(CSelection& _selection, uint32 _NameHash);
	virtual void Selection_AddTarget(CSelection& _selection, const char* _pName);
	virtual void Selection_AddTransitZone(CSelection& _selection, int _iTZObject);

	// NOTE: The Selection_RemoveOnXxxx family of functions are not fully implemented.
	virtual void Selection_RemoveOnTarget(CSelection& _selection, CStr _TargetName);
	virtual void Selection_RemoveOnNotTarget(CSelection& _selection, CStr _TargetName);


	virtual int Selection_GetSingleTarget(uint32 _NameHash);	// Find a single object. (Randomized if target matches several objects)
	virtual int Selection_GetSingleTarget(const char* _pName);	// Find a single object. (Randomized if target matches several objects)

	// -------------------------------------------------------------------
	//  Message management
	virtual aint Message_SendToObject(const CWObject_Message& _Msg, int _iObj);
	virtual aint Message_SendToObjectGUID(const CWObject_Message& _Msg, int _GUID);
	virtual void Message_SendToTarget(const CWObject_Message& _Msg, uint32 _NameHash);
	virtual void Message_SendToTarget(const CWObject_Message& _Msg, const char* _pName);
	virtual void Message_SendToSelection(const CWObject_Message& _Msg, const CSelection& _selection);

	virtual bool MessageQueue_SendToObject(const CWObject_Message& _Msg, int _iObj);		// false if msg-buffer is full
	virtual void MessageQueue_Flush();

	// -------------------------------------------------------------------
	//  NetMsg management
	virtual void NetMsg_SendToObject(const CNetMsg& _Msg, int _iObj, int _MaxLag = 4);
	virtual void NetMsg_SendToClass(const CNetMsg& _Msg, int _iClass, int _MaxLag = 4);

	virtual void NetMsg_SendToObject_Culled(const CVec3Dfp32& _PVSPos, const CNetMsg& _Msg, int _iObj = 0, int _MaxLag = 4);
	virtual void NetMsg_SendToObject_Culled(const CBox3Dfp32& _PVSPos, const CNetMsg& _Msg, int _iObj = 0, int _MaxLag = 4);

	virtual void NetMsg_SendToClient(const CNetMsg& _Msg, int _iClient = -1, int _MaxLag = 4);
	virtual void NetMsg_SendToClient_Culled(const CVec3Dfp32& _PVSPos, const CNetMsg& _Msg, int _iClient = -1, int _MaxLag = 4);
	virtual void NetMsg_SendToClient_Culled(const CBox3Dfp32& _PVSPos, const CNetMsg& _Msg, int _iClient = -1, int _MaxLag = 4);

	// -------------------------------------------------------------------
	//  Instant-sound services
	virtual void Sound_Global(int _iSound, fp32 _Volume = 1.0f, int _iClient = -1);
	virtual void Sound_GlobalCutscene(int _iSound, fp32 _Volume = 1.0f, int _iClient = -1, fp32 _Delay = 0.0f, uint32 _GroupID = 0);
	virtual void Sound_At(const CVec3Dfp32& _Pos, int _iSound, int _AttnType, uint8 _iMaterial = 0, fp32 _Volume = 1.0f, int _iClient = -1, CVec3Dfp32 _V0 = 0.0f);
	virtual void Sound_On(int16 _iObject, int _iSound, int _AttnType, uint8 _iMaterial=0, fp32 _Volume = 1.0f, int _iClient = -1, const CVec3Dfp32& _V0 = 0, const CVec3Dfp32& _Offset = 0, uint32 _GroupID = 0);
	virtual void Sound_Off(int16 _iObject, int _iSound, uint8 _iMaterial = 0, int _iClient = -1);
	virtual void Sound_GlobalNotClient(int _iClient, int _iSound, fp32 _Volume = 1.0f);
	virtual void Sound_GlobalCutsceneNotClient(int _iClient, int _iSound, fp32 _Volume = 1.0f, fp32 _Delay = 0.0f);
	virtual void Sound_AtNotClient(int _iClient, const CVec3Dfp32& _Pos, int _iSound, int _AttnType, uint8 _iMaterial = 0, fp32 _Volume = 1.0f, CVec3Dfp32 _V0 = 0.0f);
	virtual void Sound_OnNotClient(int _iClient, int16 _iObject, int _iSound, int _AttnType, uint8 _iMaterial=0, fp32 _Volume = 1.0f, const CVec3Dfp32& _V0 = 0, const CVec3Dfp32& _Offset = 0);
	virtual void Sound_OffNotClient(int _iClient, int16 _iObject, int _iSound, uint8 _iMaterial = 0);

	// Internal functions
	void Sound_Play(int16 _iObject, const CVec3Dfp32& _Pos, int _iSound, int _AttnType, uint8 _iMaterial = 0, fp32 _Volume = 1.0f, int _iClient = -1, const CVec3Dfp32 &_V0 = 0.0f, fp32 _Delay = 0, int16 _iNotClient = -1, uint32 _GroupID = 0);

public:
	// -------------------------------------------------------------------
	// GameObj
	virtual void Game_Register(int _iGame);
	virtual void Game_Unregister();
	virtual int Game_GetObjectIndex();
	virtual class CWObject_Game *Game_GetObject();

public:

	// -------------------------------------------------------------------
	//  System stuff, don't even think of calling these!
	// -------------------------------------------------------------------
	virtual void Simulate_SetCurrentLocalFrameFraction(fp32 _LocalFrameFraction);
	virtual void SimulateOnTickUp(CPerfGraph* _pPerfGraph = NULL);
	virtual bool Simulate(CPerfGraph* _pPerfGraph = NULL);
	virtual bool SimulateAhead(CPerfGraph* _pPerfGraph, fp32 _MaxTime);

#if 1
	void AddConstraintToSimulation(CWD_ConstraintDescriptor& _ConstraintDesc);

	virtual void SimulatePhysics();
	virtual void SimulatePhysics2();
#endif

	virtual void Render(CXR_VBManager* _pVBM, const CMat4Dfp32& _Camera, int _Context);	// Debug rendering only.
	virtual CRenderContext* Debug_GetRender();
	virtual CDebugRenderContainer* Debug_GetWireContainer(uint _Flags = 3);

	virtual void ClearStatistics();
	virtual void ClearFrameStatistics();
	virtual void AccumulateFrameStatistics();
	virtual void DumpStatistics();
	virtual void Dump(int _DumpFlags);
	virtual void DumpMemUsage(CStr _FileName);
		
	virtual void Con_svinfo(int _Info);
	virtual void Con_svsay(CStr _s);
	virtual void Con_svvars();
	virtual void Con_svclvars(int _iClient);
	virtual void Con_svgame(CStr _s);
	virtual void Con_svrestart();
	virtual void Con_svclientvar(int _iClient, CStr _key, CStr _value);
	virtual void Con_svset(CStr _key, CStr _value);
	virtual void Con_svphysrender(int _Flags);
	virtual void Con_svperfgraph(int _Flags);
	virtual void Con_svobjdestroy(int _iObj);
	virtual void Con_svcharacterpush(int32 _bPush);

	void Register(CScriptRegisterContext &_RegContext);


	// Statistics:
#ifdef	SERVER_STATS
	CMTime m_TInterpolate;
	CMTime m_TExecute;
	CMTime m_TClients;
	CMTime m_TSimulate;
	int m_NumSimulate;
#endif
};

#endif // _INC_WSERVER_CORE


