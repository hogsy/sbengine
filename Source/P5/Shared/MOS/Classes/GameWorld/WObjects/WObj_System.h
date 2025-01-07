#ifndef __WOBJ_SYSTEM_H
#define __WOBJ_SYSTEM_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Misc shared gameobjects

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWObject_Info_Player_Start
					CWObject_Info_Intermission
					CWObject_Info_Teleport_Destination
					CWObject_Info_ChangeWorld_Destination
					CWObject_Model
					CWObject_Sound
					CWObject_AnimModel
					CWObject_Static
					CWObject_Null
					CWObject_WorldSpawn
					CWObject_WorldSky
					CWObject_Func_UserPortal
					CWObject_Func_Portal
					CWObject_Func_Mirror
					CWObject_Func_LOD
					CWObject_FogVolume
					CWObject_Flare
					CWObject_Trigger
					CWObject_Trigger_Teleport
					CWObject_Trigger_TransitZone
					CWObject_Trigger_ChangeWorld
					CWObject_System
\*____________________________________________________________________________________________*/


#include "../Server/WServer.h"
#include "../WObjCore.h"

#include "WObj_AutoVar.h"
#include "WObj_Packer.h"

// jakob
#include "WObj_SimpleMessage.h"
// -------------------------------------------------------------------
//  Messages
// -------------------------------------------------------------------

// NOTE: OBJMSG_GAME_xxxxxx starts at 0x80
enum
{
	// rail
	OBJMSG_RAIL_REGISTER					= 0x0300,
	OBJMSG_RAIL_SPAWN						= 0x0301,
	OBJMSG_RAIL_NUMBYTYPE					= 0x0302,
	OBJMSG_RAIL_NUMBYNAME					= 0x0303,
	OBJMSG_RAIL_POWER						= 0x0304,
	OBJMSG_RAIL_ENEMY						= 0x0305,
	OBJMSG_RAIL_UNFRIENDLY					= 0x0306,
	OBJMSG_RAIL_ATTACK						= 0x0307,

	OBJMSG_MODEL_SETFLAGS					= 0x1012, // Don't change. Ogier nodetype specified. Compiled into levels.

	OBJMSG_RADIALSHOCKWAVE					= 0x0101,
	OBJMSG_DAMAGE							= 0x0102,
	
	//OBJMSG_FLICKER_IMPULSE				= 0x0103,
	
	OBJMSG_PHYSICS_KILL						= 0x0106,
	OBJMSG_IMPULSE							= 0x0110,
	OBJMSG_SCRIPTIMPULSE					= 0x0111,
	OBJMSG_LIGHT_IMPULSE					= 0x0112,
	OBJMSG_DEPTHFOG_SET						= 0x0113,
	OBJMSG_LIGHT_MORPH						= 0x0114,
	OBJMSG_SWINGDOOR_OPEN					= 0x0163,
	OBJMSG_SWINGDOOR_LOCK					= 0x0164,
	OBJMSG_SWINGDOOR						= 0x0165,
	OBJMSG_HOOK_WAITIMPULSE					= 0x0211,
	OBJMSG_HOOK_WAITPARAM					= 0x0212,
	OBJMSG_PUSH_RIGID						= 0x0213, // param0 is the pusher, vec0 is its pos
	OBJMSG_LIGHT_SETPROJMAP					= 0x0214,
	OBJMSG_THROW_RIGID						= 0x0215, // param0 is the thrower, vec0 is the pos, vec1 is force
	OBJMSG_HOOK_GETSEQUENCE					= 0x0216,
	OBJMSG_USE								= 0x0217, // sent when player presses the Use-button
	OBJMSG_ROOM_BROADCAST					= 0x0218, // used to send messages to some/all objects in a room
	OBJMSG_ROOM_GETNUMINSIDE				= 0x0219, // get number of objects in a room (used in conditions)
	OBJMSG_EXT_STATIC_IMPULSE				= 0x0220,
	OBJMSG_OBJECT_BREAK						= 0x0221, // break (remove) a trigger/part/constraint inside a destructable object
	OBJMSG_OBJECT_SPAWN						= 0x0222, // tell a destructable object to spawn an internal sub-entity (model / constraint / part / trigger..)
	OBJMSG_TELEVISION_SETCHANNEL			= 0x0223, // param0 is the channel number
	OBJMSG_OBJECT_SETVISIBILITYMASK			= 0x0224, // update visibilitymask (for rendering, not for physics)
	OBJMSG_OBJECT_PLAYANIM					= 0x0225, // play animation on CWObject_Object
	OBJMSG_OBJECT_SETDYNAMIC				= 0x0226, // Turn on/off dynamics engine control
	OBJMSG_ENGINEPATH_DETACH_PHYSICS_OBJECT	= 0x0227, // Detach object linked to physics driven EP
	OBJMSG_ENGINEPATH_ATTACH_PHYSICS_OBJECT	= 0x0229, // Attaches and object to an Physics driven EP
	OBJMSG_GLASS_IMPULSE					= 0x0228, // Send impulses to glass world spawn object
	OBJMSG_HIDEMODEL						= 0x1066, // Hide model	 (moved from CharMsg.h)
	OBJMSG_OBJECT_SWITCH_AUTOAIM			= 0x0231, // Switch on/off autoaim for object

	OBJMSG_CANUSE,
	OBJMSG_HOOK_GETDURATION,
	OBJMSG_PRIMITIVES_LINE,
	OBJMSG_FUNCPORTAL_SETSTATE,
	OBJMSG_FUNCPORTAL_SETCONTROLLER,
	OBJMSG_FUNC_GETWAIT,	
	OBJMSG_HOOK_GETRENDERMATRIX,
	OBJMSG_HOOK_GETORGMATRIX,
	OBJMSG_HOOK_GETCONTROLLEDTIME,
	OBJMSG_HOOK_SETCONTROLLER,
	OBJMSG_HOOK_GETTIME,
    OBJMSG_ENGINEPATH_PROPELPATH,
	OBJMSG_ENGINEPATH_SETTIME,		// Sets the enginepath time and if later than current time, fires relevant messages
	OBJMSG_ENGINEPATH_CHECKFALLOFF,	//Checks if the object connected the the EP should fall off
	OBJMSG_OBJECT_SETUP,			//Lets the object know which EP is controling it
	OBJMSG_HOOK_GETCURRENTMATRIX,
	OBJMSG_HOOK_ALLOCATEATTACHDATA,
	OBJMSG_ENGINEPATH_GETDURATION,
	OBJMSG_DEPTHFOG_GET,
	OBJMSG_TRIGGER_INTERSECTION,
	OBJMSG_LIGHT_GETINDEX,
	OBJMSG_LIGHT_INITPROJMAP,		// Used to set projection map from Trigger_Lamp
	OBJMSG_SCENEPOINT_ADD,
	OBJMSG_GIB_ADDPART,
	OBJMSG_GIB_EXPLOSION,
	OBJMSG_GIB_INITIALIZE,
	OBJMSG_RAIL_FORGETENEMIES,
	OBJMSG_SPAWN_WALLMARK,			// client message
	OBJMSG_LIGHT_GETSTATUS,
	OBJMSG_SWINGDOOR_GETOPENDIR,	//1 is right, 2 is left
	OBJMSG_SWINGDOOR_CANDARKLINGOPEN,
	OBJMSG_GETSPAWNMASK,
	OBJMSG_GLASS_CRUSH,

	//These messages should be handled by any objects that can spawn other objects in runtime in 
	//order for scenepoints etc to work correctly in conjunction with the spawned objects
	OBJMSG_SPAWNER_CANSPAWN,		//Return 1 if object can spawn any other objects
	OBJMSG_SPAWNER_CANSPAWNNAMED,	//Return 1 if any object this object can spawn will get the given targetname (m_pData)
	OBJMSG_SPAWNER_ONLOADOWNED,		//This message should be sent by all run-time spawned objects to owner (spawner) during their OnDeltaLoad. Currently this message is only sent by characters.

	OBJMSG_GLASSPHYS_GLASSINSTANCE_IMPACT,

	INFOPLAYERSTART_FLAGS_RESETDIRECTION = 1,
	INFOPLAYERSTART_FLAGS_DEFAULTSTART = 2,
	INFOPLAYERSTART_FLAGS_STARTCROUCHED = 4,
	INFOPLAYERSTART_FLAGS_CLEARPLAYER = 8,
	INFOPLAYERSTART_FLAGS_NIGHTVISION = 16,
	INFOPLAYERSTART_FLAGS_RELATIVEPOS = 32,
	INFOPLAYERSTART_FLAGS_CLEARSAVE = 64,
	INFOPLAYERSTART_FLAGS_CLEARMISSIONS = 128,

	INFOPLAYERSTART_FLAGS_OGR_NOCLIP = 0x1000,
	INFOPLAYERSTART_FLAGS_OGR_SPECIALCLASS = 0x2000,
};

// -------------------------------------------------------------------
//  INFO_PLAYER_START
// -------------------------------------------------------------------
class CWObject_Info_Player_Start : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;
public:
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnCreate();
	virtual void OnSpawnWorld();

	TArray<CWO_SimpleMessage> m_lMsg_OnSpawn;		// Messages sent on spawn
};

#ifdef M_Profile
// -------------------------------------------------------------------
//  INFO_INTERMISSION
// -------------------------------------------------------------------
class CWObject_Info_Intermission : public CWObject_Info_Player_Start
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	virtual void OnFinishEvalKeys();
};
#endif

#ifndef M_DISABLE_TODELETE

// -------------------------------------------------------------------
//  INFO_TELEPORT_DESTINATION
// -------------------------------------------------------------------
class CWObject_Info_Teleport_Destination: public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;
public:
	CWObject_Info_Teleport_Destination();
};

// -------------------------------------------------------------------
//  INFO_CHANGEWORLD_DESTINATION
// -------------------------------------------------------------------
class CWObject_Info_ChangeWorld_Destination: public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;
public:
	CWObject_Info_ChangeWorld_Destination();
};

#endif

// -------------------------------------------------------------------
//  MODEL
// -------------------------------------------------------------------
class CWObject_Model : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:

	void Sound(int _iSound);								
	void SoundLRP(int _iSound, CVec3Dfp32 _P1, CVec3Dfp32 _P2);

//	void Sound(const char *_Name)										{ m_pWServer->Sound_At(GetPosition(), m_pWServer->GetMapData()->GetResourceIndex_Sound(_Name), 0); }
//	void Sound(const char *_Name, const CVec3Dfp32 &_Pos)				{ m_pWServer->Sound_At(_Pos, m_pWServer->GetMapData()->GetResourceIndex_Sound(_Name), 0); }
//	int Spawn(const char *_Name,const CMat4Dfp32 &_Mat,int _iOwner = -1)	{ return m_pWServer->Object_Create(_Name, _Mat, _iOwner == -1 ? m_iOwner : _iOwner); }
//	int Spawn(const char *_Name, int _iOwner = -1)						{ return Spawn(_Name, GetPositionMatrix(), _iOwner); }

	void Destroy()														{ m_iSound[0] = 0; m_iSound[1] = 0; m_pWServer->Object_Destroy(m_iObject); }

	int GetModel(const char *_Name)									{ return m_pWServer->GetMapData()->GetResourceIndex_Model(_Name); }
	int GetSound(const char *_Name)									{ return m_pWServer->GetMapData()->GetResourceIndex_Sound(_Name); }

	const CVec3Dfp32 &GetForward()								{ return CVec3Dfp32::GetMatrixRow(GetPositionMatrix(), 0); }
	bool SetPosition(const CVec3Dfp32 &_Pos)						{ return m_pWServer->Object_SetPosition(m_iObject, _Pos); }
	bool SetPosition(const CMat4Dfp32 &_Mat)						{ return m_pWServer->Object_SetPosition(m_iObject, _Mat); }
	void SetVelocity(const CVec3Dfp32 &_Vel)						{ m_pWServer->Object_SetVelocity(m_iObject, _Vel); }
	void SetVelocity(const CVelocityfp32 &_Vel)					{ m_pWServer->Object_SetVelocity(m_iObject, _Vel); }
	void SetRotVelocity(const CAxisRotfp32 &_Vel)				{ m_pWServer->Object_SetRotVelocity(m_iObject, _Vel); }
	void SetDirty(int _Mask)									{ m_pWServer->Object_SetDirty(m_iObject, _Mask); }

public:
	virtual void Model_SetPhys(int iModel, bool _bAdd = false, int _ObjectFlags = OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT, int _PhysFlags = OBJECT_PHYSFLAGS_PUSHER, bool _bNoPhysReport = true);
	virtual void Model_Set(int _iPos, int _iModel, bool _bAutoSetPhysics = true);
	virtual void Model_Set(int _iPos, const char *_pName, bool _bAutoSetPhysics = true);


	CWObject_Model();
	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server*);
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();

	virtual aint OnMessage(const CWObject_Message& _Msg);
	
	virtual void OnRefresh();

	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);
	virtual void OnDeltaSave(CCFile* _pFile);

	static void RefreshStandardModelInstances(CWObject_Client* _pObj, CWorld_Client* _pWClient, int _ModelFlags);
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWorld);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine*, const CMat4Dfp32& _ParentMat);
};


// -------------------------------------------------------------------
//  MODEL
// -------------------------------------------------------------------
class CWObject_Sound : public CWObject_Model
{
public:
	CWObject_Sound();
	MRTC_DECLARE_SERIAL_WOBJECT;
public:
	enum
	{
		MAXRANDOMSOUNDS = 3,
	};

	int m_iRealSound[2];
	bool m_bWaitSpawn;

	int m_RandomSound_iSound[MAXRANDOMSOUNDS];
	int m_RandomSound_MaxTick[MAXRANDOMSOUNDS];
	int m_RandomSound_MinTick[MAXRANDOMSOUNDS];

	int m_RandomSound_NextTick[MAXRANDOMSOUNDS];
	
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual void OnRefresh();
	virtual aint OnMessage(const CWObject_Message &_Msg);

	virtual void Spawn();
	virtual void Unspawn();
};

//#ifdef M_Profile
// -------------------------------------------------------------------
//  ANIMMODEL
// -------------------------------------------------------------------
class CWObject_AnimModel : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	CWObject_AnimModel();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine*, const CMat4Dfp32& _ParentMat);
};
//#endif

// -------------------------------------------------------------------
//  STATIC
// -------------------------------------------------------------------
class CWObject_Static : public CWObject_Model
{
public:
	CWObject_Static();
	MRTC_DECLARE_SERIAL_WOBJECT;

	int m_iBSPModelIndex;
	int m_iPhysModel;
	int m_iRealModel[3];
	int m_iRealSound[2];
	bool m_bWaitSpawn;
	
public:
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual aint OnMessage(const CWObject_Message &_Msg);
	virtual void OnFinishEvalKeys();

	void Spawn();
	
	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);
	virtual void OnDeltaSave(CCFile* _pFile);

	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine*, const CMat4Dfp32& _ParentMat);
};

// -------------------------------------------------------------------
//  EXT_STATIC
// -------------------------------------------------------------------
class CWO_Ext_Static_ClientData : public CReferenceCount, public CAutoVarContainer
{
public:
	CWObject_CoreData*	m_pObj;

public:
	AUTOVAR_SETCLASS(CWO_Ext_Static_ClientData, CAutoVarContainer);
	CAUTOVAR_OP(CAutoVar_int32,		m_SurfaceResourceID,	DIRTYMASK_0_0);
		
	AUTOVAR_PACK_BEGIN
	AUTOVAR_PACK_VAR(m_SurfaceResourceID)
	AUTOVAR_PACK_END

public:
	CWO_Ext_Static_ClientData();

	void Clear(CWObject_CoreData* _pObj);
};

class CWObject_Ext_Static : public CWObject_Static
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	static const CWO_Ext_Static_ClientData& GetClientData(const CWObject_CoreData* _pObj);
	static       CWO_Ext_Static_ClientData& GetClientData(      CWObject_CoreData* _pObj);

public:
	CWObject_Ext_Static();

	virtual void OnCreate();

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual aint OnMessage(const CWObject_Message &_Msg);
	virtual void OnRefresh();
	
	static void  OnIncludeClass(CMapData* _pWData, CWorld_Server* _pWServer);
	
	virtual int  OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const;
	
	static void  OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	static int   OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);
	static void  OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine*, const CMat4Dfp32& _ParentMat);

	static void  OnClientPrecache(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine);
	
	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);
	virtual void OnDeltaSave(CCFile* _pFile);
};

// -------------------------------------------------------------------
//  NULL
// -------------------------------------------------------------------
class CWObject_Null : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
};

// -------------------------------------------------------------------
//  WORLDSPAWN
// -------------------------------------------------------------------
class CWObject_WorldSpawn : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	int m_XR_Mode;
	int m_ModelIndex;

public:
	CWObject_WorldSpawn();
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();

	virtual void OnRefresh();
	static void OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static void OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);

	static int OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);
	static void OnClientLoad(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags);
};

// -------------------------------------------------------------------
//  WORLDPLAYERPHYS
// -------------------------------------------------------------------
class CWObject_WorldPlayerPhys : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	int m_ModelIndex;

public:
	CWObject_WorldPlayerPhys();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();

	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
};


// -------------------------------------------------------------------
//  WORLDGLASSSPAWN
// -------------------------------------------------------------------
class CWO_Glass_MsgData
{
public:
	CVec3Dfp32	m_BoxSize;
	CVec3Dfp32	m_LocalPosition;
	CVec3Dfp32	m_ForceDir;
	CVec2Dfp32	m_ForceRange;
	fp32		m_ForceScale;
	fp32		m_Radius;
	uint16		m_iInstance;
	uint8		m_CrushType;

	CWO_Glass_MsgData() { Clear(); }

	void Clear()
	{
		m_BoxSize = 1.0f;
		m_LocalPosition = 0.0f;
		m_ForceDir = 0.0f;
		m_ForceRange = 1.0f;
		m_ForceScale = 1.0f;
		m_Radius = 1.0f;
		m_iInstance = 0xFFFF;
		m_CrushType = 0;
	}

	static const CWO_Glass_MsgData* GetSafe(const CWObject_Message& _Msg)
	{
		if (_Msg.m_pData && _Msg.m_DataSize == sizeof(CWO_Glass_MsgData))
			return (const CWO_Glass_MsgData*)_Msg.m_pData;

		return NULL;
	}

	static void SetData(CWObject_Message& _Msg, const CWO_Glass_MsgData& _MsgData)
	{
		_Msg.m_pData = (void*)&_MsgData;
		_Msg.m_DataSize = sizeof(CWO_Glass_MsgData);
	}
};


class CWObject_WorldGlassSpawn : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

protected:
	enum
	{
		GLASS_MODEL_INDEX				= 0,

		// Don't change order, it will screw up nodetype in Ogier
		GLASS_IMPULSE_NEEDINSTANCE		= 0,		// All messages below needs a glass instance index
		GLASS_IMPULSE_RESTORE,						// Restore glass
		GLASS_IMPULSE_CRUSH,						// Crush glass
		GLASS_IMPULSE_ACTIVE,						// Activate/deactivate render of glass

		GLASS_IMPULSE_NOINSTANCE		= 0x8000,	// All messages below don't need instance indices
		GLASS_IMPULSE_GLOBAL_RESTORE,				// Restore all windows

		CWO_GLASS_CLIENTFLAGS_NOLINK	= M_Bit(0) << CWO_CLIENTFLAGS_USERSHIFT,
		CWO_GLASS_CLIENTFLAGS_DYNAMIC	= M_Bit(1) << CWO_CLIENTFLAGS_USERSHIFT,
	};

	class CWO_GlassClientData : public CReferenceCount
	{
	public:
		CWO_GlassClientData();
		~CWO_GlassClientData();
		TPtr<CXR_ModelInstance>	m_spModelInstance;
	};

	int m_ModelIndex;
	
	static		 CWO_GlassClientData& GetClientData(	  CWObject_CoreData* _pObj);
	static const CWO_GlassClientData& GetClientData(const CWObject_CoreData* _pObj);

	static CXR_ModelInstance* GetModelInstance(CWO_GlassClientData& _CD, CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	static class CXR_Model_BSP4Glass_Instance* GetGlassModelInstance(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	
	uint8 EvaluateDamageType(uint32 _DamageType);
	void  Impulse(uint _Impulse, const char* _pImpulseData, int16 _iSender);

	static uint8 TranslateCrushMsg(uint8 _CrushMsg);
	static uint8 TranslateCrushType(uint8 _CrushType);

public:

	CWObject_WorldGlassSpawn();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();

	virtual void OnRefresh();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	
	static void  OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);

	static aint  OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	static void  OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg);
	static int   OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);
	static void  OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static void  OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static void  OnClientRenderGlass(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat, const CMat4Dfp32& _WMat);
	
	static bool  OnIntersectLine(CWO_OnIntersectLineContext& _Context, CCollisionInfo* _pCollisionInfo = 0);

	static void  OnIncludeClass(CMapData* _pMapData, CWorld_Server* _pWServer);
	
	virtual void OnLoad(CCFile* _pFile);
	virtual void OnSave(CCFile* _pFile);
	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);
	virtual void OnDeltaSave(CCFile* _pFile);
	virtual void OnFinishDeltaLoad();

	// Server & Client
	static int OnPhysicsEvent(CWObject_CoreData*, CWObject_CoreData*, CWorld_PhysState*, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo = NULL);
};


// -------------------------------------------------------------------
//  DYNAMIC GLASS
// -------------------------------------------------------------------
class CWObject_Glass_Dynamic : public CWObject_WorldGlassSpawn
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	CWObject_Glass_Dynamic();

	virtual void OnFinishEvalKeys();
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
};


// -------------------------------------------------------------------
//  WORLDSKY
// -------------------------------------------------------------------
class CWObject_WorldSky : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	class CDepthFog
	{
	public:
		int32 m_Near;
		int32 m_Far;
		CVec3Dint8 m_Color;
		int16 m_Density;

		CDepthFog() {}
		CDepthFog(int _Val) { *this = _Val; }
		CDepthFog(const CDepthFog &_Fog) { m_Near = _Fog.m_Near; m_Far = _Fog.m_Far; m_Color = _Fog.m_Color; m_Density = _Fog.m_Density; }
		void Parse(const char *_pSt);

		bool operator !=(const CDepthFog &_Fog) const;
		void operator =(int _Val);
		CDepthFog operator +(const CDepthFog &_Fog) const;
		CDepthFog operator -(const CDepthFog &_Fog) const;
		CDepthFog operator *(fp32 _Val) const;

		void Pack(uint8*& _pD) const;
		void Unpack(const uint8*& _pD);
	};

	CWObject_WorldSky();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual void OnCreate();
	static void OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static void OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
};


// -------------------------------------------------------------------
//  SIDESCENE
// -------------------------------------------------------------------
class CWObject_SideScene : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
};

// -------------------------------------------------------------------
//  FUNC_USERPORTAL
// -------------------------------------------------------------------
class CWObject_Func_UserPortal : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	enum
	{
		MAXCONTROLLERS = 4,
	};

	uint16 m_lControllers[MAXCONTROLLERS];
	uint8 m_lControllerState[MAXCONTROLLERS];
	uint8 m_NumControllers;
	CStr m_Target;

public:
	CWObject_Func_UserPortal();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnDeltaSave(CCFile* _pFile);
	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWorld);
	static void OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
};

#ifndef M_DISABLE_CWOBJECT_FUNC_PORTAL
// -------------------------------------------------------------------
//  FUNC_PORTAL
// -------------------------------------------------------------------
class CWObject_Func_Portal : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;

protected:
	TArray<CVec3Dfp32> m_lVPortal;
//	virtual void GetCamera(CWorld_Client* _pWorld, CMat4Dfp32& _Dst);
	CStr m_Target;

public:
	CWObject_Func_Portal();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnRefresh();
	virtual void OnLoad(CCFile* _pFile);
	virtual void OnSave(CCFile* _pFile);
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWorld);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
};
#endif

// -------------------------------------------------------------------
//  FUNC_MIRROR
// -------------------------------------------------------------------
#ifndef M_DISABLE_CWOBJECT_FUNC_MIRROR
class CWObject_Func_Mirror : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
};
#endif

#ifndef M_DISABLE_CWOBJECT_FUNC_LOD
// -------------------------------------------------------------------
//  FUNC_LOD
// -------------------------------------------------------------------
class CWObject_Func_LOD : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	CWObject_Func_LOD();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
};
#endif

#ifndef M_DISABLE_TODELETE
// -------------------------------------------------------------------
//  FOGVOLUME
// -------------------------------------------------------------------
class CWObject_FogVolume : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	fp32 m_Radius;
	fp32 m_Thickness;
	CPixel32 m_Color;

public:
	CWObject_FogVolume();
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual void OnLoad(CCFile* _pFile);
	virtual void OnSave(CCFile* _pFile);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
};
#endif

// -------------------------------------------------------------------
//  FLARE
// -------------------------------------------------------------------
class CWObject_Flare : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	CWObject_Flare();
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
};

// -------------------------------------------------------------------
//  TRIGGER
// -------------------------------------------------------------------
class CWObject_Trigger : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;
protected:
	int32 m_TriggerObjFlags;
	int32 m_Mode;
	int16 m_bActive;
	int32 m_InitObjectFlags;
	int32 m_InitObjectIntersectFlags;
	CStr m_Target;

public:
	CWObject_Trigger();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnRefresh();
	virtual void OnLoad(CCFile* _pFile);
	virtual void OnSave(CCFile* _pFile);

	virtual void Trigger(int _iSender = -1);
};

#ifndef M_DISABLE_CWOBJECT_TRIGGER_TELEPORT
// -------------------------------------------------------------------
//  TRIGGER_TELEPORT
// -------------------------------------------------------------------
class CWObject_Trigger_Teleport : public CWObject_Trigger
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	int32 m_State;

public:
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnCreate();
	bool Teleport(int _iObj, const CMat4Dfp32& _Pos);
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnRefresh();
	virtual void OnFinishEvalKeys();
	virtual void OnLoad(CCFile* _pFile);
	virtual void OnSave(CCFile* _pFile);
	static void OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);
};
#endif

// -------------------------------------------------------------------
//  TRIGGER_TRANSITZONE
// -------------------------------------------------------------------
/*class CWObject_Trigger_TransitZone : public CWObject_Trigger
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	CStr m_TZName;

public:
	CWObject_Trigger_TransitZone();

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnLoad(CCFile* _pFile);
	virtual void OnSave(CCFile* _pFile);
	virtual void OnSpawnWorld();
};*/

// -------------------------------------------------------------------
//  TRIGGER_CHANGELEVEL
// -------------------------------------------------------------------
class CWObject_Trigger_ChangeWorld : public CWObject_Trigger
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	CStr m_World;
	int m_Flags;

	enum {
		FLAGS_INSTANT = M_Bit(0),
		FLAGS_WAITSPAWN = M_Bit(1),
	};

public:
	CWObject_Trigger_ChangeWorld();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnLoad(CCFile* _pFile);
	virtual void OnSave(CCFile* _pFile);
	virtual void OnFinishEvalKeys();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void Trigger(int _iSender = -1);

	void Spawn(bool _bSpawn);
};


#ifndef M_DISABLE_CWOBJECT_SYSTEM
/////////////////////////////////////////////////////////////////////////////
// CWObject_System
class CWObject_System : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnCreate();
};
#endif

#endif // _INC_WOBJ_SYSTEM
