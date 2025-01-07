#ifndef __WOBJ_TRIGGER_H
#define __WOBJ_TRIGGER_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Misc triggers

	Author:			Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWObject_Trigger_Ext
					CWObject_Trigger_Message
					CWObject_Trigger_Damage
					CWObject_Trigger_SoftSpot
					CWObject_Trigger_SneakZone
					CWObject_Trigger_RestrictedZone
					CWObject_Trigger_HideVolume
\*____________________________________________________________________________________________*/


#include "../../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_System.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_SimpleMessage.h"
#include "../../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_Hook.h"
#include "../WObj_Messages.h"

class CWO_DamageMsg;

enum
{
	OBJMSG_MESSAGEPIPE_SETSTATE = OBJMSGBASE_TRIGGER, // Ogier Reserved ID 0x5000
	OBJMSG_TELEPORTCONTROLLER_TELEPORTOBJECT = 0x5001, // Ogier Reserved ID 0x5001
	
	//OBJMSG_OBJANIM_TRIGGER,
	OBJMSG_TRIGGER_WASDESTROYED,
	OBJMSG_TRIGGER_CONFIGURE,
	OBJMSG_TRIGGER_PICKUP_SETTICKSLEFT,
	OBJMSG_TRIGGER_GET_INTERSECT_NOTIFYFLAGS,
	OBJMSG_TRIGGER_SET_INTERSECT_NOTIFYFLAGS,
};

class CWObject_Trigger_Ext : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	
protected:
	TArray<CStr> m_lLinkedTriggers;
	
	int32 m_DelayTicks;
	int32 m_DamageDelayTicks;
	
	int32 m_CurDelay;
	
	int32 m_Flags;
	int32 m_iLastTriggerer;

	int32 m_iBSPModel;

	int16 m_Hitpoints;
	int16 m_MaxHitpoints;
	int16 m_MinDamage;
	int16 m_MaxDamage;
	int16 m_iMaxDamageType;
	int16 m_SoundLevel;
	uint16 m_CharFlags; // what character types to intersect with (default: all)
	
	CStr m_Use_RPGObject;
	TArray<CNameHash> m_lIntersectObjects;			// if non-empty, only objects that exists in this list will be tested.
	TArray<CNameHash> m_lExcludedObjects;		// objects that exists in this list will not be tested.

	CWO_SimpleMessageContainer m_MsgContainer;

	struct CTimedEntry
	{
		uint16 m_iActivator;
		int16  m_nTicksLeft;
	};
	TThinArray<CTimedEntry> m_lTriggeredDelayedLeaveMessageTimers;

	struct CIntersectingObject
	{
		uint16 m_iObject;
		int32 m_LastTick;
	};

	TArray<CIntersectingObject> m_lCurIntersectingObjects;
	
	int m_SpecialDamage;

	uint8 m_bTrigging : 1;
	uint8 m_bRunTriggerOnLeave : 1;
	uint8 m_bForceRefresh : 1;

	int16 m_DealDamage;
	uint32 m_DealDamageType;

	enum
	{
		TRIGGER_FLAGS_SOUND			= M_Bit(0),
		TRIGGER_FLAGS_VISIBLE		= M_Bit(1),
		TRIGGER_FLAGS_DEALDAMAGE	= M_Bit(2),
	//	TRIGGER_FLAGS_0x0008		= M_Bit(3),
		TRIGGER_FLAGS_ONCE			= M_Bit(4),
		TRIGGER_FLAGS_RANDOM		= M_Bit(5),
		TRIGGER_FLAGS_DESTROYABLE	= M_Bit(6),
		TRIGGER_FLAGS_WAITSPAWN		= M_Bit(7),
		TRIGGER_FLAGS_CORPSE		= M_Bit(8),

		TRIGGER_CHARFLAGS_PLAYER	= M_Bit(0),
		TRIGGER_CHARFLAGS_DARKLING	= M_Bit(1),
		TRIGGER_CHARFLAGS_CIVILIAN	= M_Bit(2),
		TRIGGER_CHARFLAGS_BADGUY	= M_Bit(3),
		TRIGGER_CHARFLAGS_TOUGHGUY	= M_Bit(4),
		TRIGGER_CHARFLAGS_SOLDIER	= M_Bit(5),
		TRIGGER_CHARFLAGS_MEATFACE	= M_Bit(6),

		TRIGGER_MAXDAMAGETYPE_CLIP		= 0,
		TRIGGER_MAXDAMAGETYPE_SPLIT		= 1,
		TRIGGER_MAXDAMAGETYPE_TRIGGER	= 2,
		TRIGGER_MAXDAMAGETYPE_DESTROY	= 3,

		TRIGGER_MSG_INSTANT = 0,
		TRIGGER_MSG_DELAYED,
		TRIGGER_MSG_ENTER,
	//	TRIGGER_MSG_ENTER_DELAYED,
		TRIGGER_MSG_LEAVE,
		TRIGGER_MSG_LEAVE_DELAYED,
		TRIGGER_MSG_DAMAGED,
	};

	void SendRandomMessage(int _Event, int _iSender);
	bool TestIntersection(const CWO_PhysicsState& _This, uint _iObject);
	void UpdateMessageQueue(TThinArray<CTimedEntry>& _List, uint _Event);
	int FindIntersectingObject(uint _iObject) const; // returns index to m_lCurIntersectingObjects
	void AddIntersectingObject(uint _iObject);
	uint UpdateIntersectingObjects();	// returns object index

public:
	CWObject_Trigger_Ext();
	
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual void Spawn(bool _bSpawn);
	static void OnIncludeClass(CMapData *_pWData, CWorld_Server *_pWServer);
	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer);
	virtual void OnSpawnWorld();

	virtual aint OnMessage(const CWObject_Message& _Msg);
	static aint OnClientMessage(CWObject_Client *_pObj, CWorld_Client *_pWClient, const CWObject_Message &_Msg);

	virtual void OnRefresh();

	int ApplyDamage(const CWO_DamageMsg &_Damage, int _iSender);
	virtual void Trigger(int _iSender);
	virtual void TriggerOnLeave(int _iSender);
	virtual void TriggerOnEnter(int _iSender);
	void SendUseNotification(int _iPlayer);

	virtual void UpdateNoRefreshFlag();
	
	void SetStrInData(CStr _St);
	static char *GetStrFromData(CWObject_CoreData *_pObj);

	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);
	virtual void OnDeltaSave(CCFile* _pFile);
	virtual void OnFinishDeltaLoad();
};

#ifndef M_DISABLE_TODELETE

// -------------------------------------------------------------------
// Trigger_Message
// -------------------------------------------------------------------
class CWObject_Trigger_Message : public CWObject_Trigger
{
public:
	CWObject_Trigger_Message();
	MRTC_DECLARE_SERIAL_WOBJECT;

	CStr m_Msg;
	int m_Timer;
	int m_Wait;

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual aint OnMessage(const CWObject_Message& _Msg);
};

#endif


// -------------------------------------------------------------------
//  TRIGGER_DAMAGE
// -------------------------------------------------------------------
class CWObject_Trigger_Damage : public CWObject_Trigger
{
	MRTC_DECLARE_SERIAL_WOBJECT;
public:
	int16 m_Damage;			//Damage per sec
	int16 m_FramesPerHit;
	uint32 m_DamageType;
	int16 m_LastFrameSent;

	CWObject_Trigger_Damage();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual aint OnMessage(const CWObject_Message& _Msg);
	/*	virtual void OnLoad(CCFile* _pFile);
	virtual void OnSave(CCFile* _pFile);*/
};


#if !defined(M_DISABLE_TODELETE) || 1
// -------------------------------------------------------------------
// Trigger_SoftSpot
// -------------------------------------------------------------------
class CWObject_Trigger_SoftSpot : public CWObject_Trigger_Ext
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	CWObject_Trigger_SoftSpot();
	virtual void TriggerOnEnter(int _iSender);
	virtual void TriggerOnLeave(int _iSender);
};

// -------------------------------------------------------------------
// Trigger_Sneak
// -------------------------------------------------------------------
class CWObject_Trigger_SneakZone : public CWObject_Trigger_Ext
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	bool m_bOccupied;

public:
	CWObject_Trigger_SneakZone();
	virtual void Trigger(int _iSender);
	virtual void TriggerOnLeave(int _iSender);
};
#endif

// -------------------------------------------------------------------
// Trigger_RestrictedZone
// -------------------------------------------------------------------
class CWObject_Trigger_RestrictedZone : public CWObject_Trigger_Ext
{
		MRTC_DECLARE_SERIAL_WOBJECT;

	int m_ClearanceLevel;

public:
	CWObject_Trigger_RestrictedZone();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void Trigger(int _iSender);
	virtual void TriggerOnLeave(int _iSender);
};

// -------------------------------------------------------------------
//  TRIGGER_ACCPAD
// -------------------------------------------------------------------
class CWObject_Trigger_AccPad : public CWObject_Trigger
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	CVec3Dfp32 m_Vel;
	int32 m_TimeOut;

public:
	CWObject_Trigger_AccPad();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnRefresh();
};

// -------------------------------------------------------------------
//  Trigger_Pickup
// -------------------------------------------------------------------
class CWObject_Trigger_Pickup : public CWObject_Trigger_Ext
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:	
	enum
	{
		PICKUP_HEALTH,
		PICKUP_AMMO,
		PICKUP_WEAPON,
		PICKUP_FLAG,
		PICKUP_HEALTH_BOOST,
		PICKUP_SPEED_BOOST,
		PICKUP_DAMAGE_BOOST,
		PICKUP_SHIELD,
		PICKUP_INVISIBILITY,

		PICKUP_UNKNOWN,

		PICKUP_SPAWNFLAG_DM					= M_Bit(0),
		PICKUP_SPAWNFLAG_TDM				= M_Bit(1),
		PICKUP_SPAWNFLAG_CTF				= M_Bit(2),
		PICKUP_SPAWNFLAG_SURVIVOR			= M_Bit(3),
		PICKUP_SPAWNFLAG_LASTHUMAN			= M_Bit(4),
		PICKUP_SPAWNFLAG_SHAPESHIFTER		= M_Bit(5),
		PICKUP_SPAWNFLAG_DARKLINGS			= M_Bit(6),
		PICKUP_SPAWNFLAG_DVH				= M_Bit(7),
	};

	virtual void OnCreate();
	virtual void Spawn(bool _bSpawn);
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual void Trigger(int _iSender);
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnRefresh();
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static int OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo = NULL);
	virtual void UpdateNoRefreshFlag();

	uint32		m_SpawnModeFlags;
	uint8		m_Type;
	uint32		m_SpawnTime;
	int			m_Param0;	//amount of health, amount of ammo and so on form pickup
	int			m_iSound;
	int			m_Duration;
	int			m_TicksLeft;

	fp32		m_Multiplier;

	CStr		m_WeaponTemplate;
	CStr		m_ModelName;

	TPtr<class CRPG_Object_Item>	m_spDummyObject;
	CWO_SimpleMessage				m_Msg_OnPickup;
};

// -------------------------------------------------------------------
//  Trigger_Look
// -------------------------------------------------------------------
// Delayed messages sent as usual, only "messages" are affected
#define CWObject_Trigger_Look_Parent CWObject_Trigger_Ext
class CWObject_Trigger_Look : public CWObject_Trigger_Look_Parent
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	struct LookObject
	{
		int32 m_iLookObject;
		int32 m_LookTick;
	};
	TArray<CWO_SimpleMessage> m_lLookLeaveMessages;
	TArray<struct LookObject> m_lLookActiveObjects;
	CStr m_LookObject;
	int32 m_iLookObject;
	
	// How tight we must look at the object
	fp32 m_LookAccuracy;

	// Time we started to be inside lookangle
	CMTime m_LookTime;
	int32 m_LastLookTick;
	// How long we need to stay inside look before triggering
	fp32 m_NeededLookTime;

	void AddLookActive(int16 _iObject, int32 _LookTick);
	void CheckLookActive(int32 _CurrentTick, bool _bForce = false, int _iForceSend = -1);
public:
	virtual void OnCreate();
	virtual void OnSpawnWorld();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnRefresh();

	virtual void Trigger(int _iSender);
	virtual void TriggerOnLeave(int _iSender);
	bool CheckLook(int32 _iObject);
};

// -------------------------------------------------------------------
//  TRIGGER_HIDEVOLUME
// -------------------------------------------------------------------
class CWObject_Trigger_HideVolume : public CWObject_Trigger_Ext
{
	enum
	{
		FLAGS_HIDDEN =		M_Bit(0),
		FLAGS_CHARACTERS =	M_Bit(1),
		FLAGS_OBJECTS =		M_Bit(2),
	};

	MRTC_DECLARE_SERIAL_WOBJECT;

	int m_HideFlags;
	bool m_bHiding:1;
	TArray<int> m_lCurrObjectsInside;

public:
	CWObject_Trigger_HideVolume();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnSpawnWorld2(void);
	virtual void OnRefresh(void);
	virtual aint OnMessage(const CWObject_Message& _Msg);
	
	virtual void UpdateNoRefreshFlag();
	bool TryToAddObject(int _iObj);
	void InitObjectsInside();
	void UpdateObjectsInside(void);
	bool PointIsInsideRoom(const CVec3Dfp32& _Pos) const;
};


#endif

