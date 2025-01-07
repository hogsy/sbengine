/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			WObj_SwingDoor.h

Author:			Olle Rosenquist

Copyright:		Copyright O3 Games AB 2004

Contents:		

Comments:		

History:		
041110:		Created File
061007:		(Roger) Made doors physics driven
\*____________________________________________________________________________________________*/

#ifndef _INC_WOBJ_SWINGDOOR
#define _INC_WOBJ_SWINGDOOR

#include "../WObj_Sys/WObj_Physical.h"
#include "../WObj_Messages.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Hook.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_AutoVar.h"
#include "WObj_Object.h"

#define CWObject_SwingDoorParent CWObject_Model

class CWObject_SwingDoor : public CWObject_SwingDoorParent 
{
	MRTC_DECLARE_SERIAL_WOBJECT;
public:
	enum
	{
		SWINGDOORFLAGS_GAMEPHYSICS			= M_Bit(0), // Door object has physics, ie it stops bullets etc
		SWINGDOORFLAGS_BLOCKNAVIGATION		= M_Bit(1), // Door blocks navigation at all times making it impassable by ai even when door is open
		SWINGDOORFLAGS_NOPLAYER				= M_Bit(2), // Door does not swing open for player
		SWINGDOORFLAGS_NOAI					= M_Bit(3), // Door does not swing open for ai
		SWINGDOORFLAGS_NODARKLING			= M_Bit(4), // Door does not swing open for darklings
		SWINGDOORFLAGS_NEVERCLOSE			= M_Bit(5), // Door never closes
		SWINGDOORFLAGS_NODEFAULTTRIGGER		= M_Bit(6), // Ignore built in triggers
		SWINGDOORFLAGS_MESSAGE_ONCE			= M_Bit(7),
		SWINGDOORFLAGS_LOCKED				= M_Bit(8), // Start in locked state
		SWINGDOORFLAGS_NODEFAULTSP			= M_Bit(9), //ignore built in sps
		SWINGDOORFLAGS_NODEFAULTHANDLE		= M_Bit(10), //don't place handles
		SWINGDOORFLAGS_HANDLE_RIGHT_SIDE	= M_Bit(11),
		SWINGDOORFLAGS_NOSP					= M_Bit(12),
		SWINGDOORFLAGS_CONSTRAINT_MODDED	= M_Bit(13),	//Constraint angles are modded so door will close properly
		SWINGDOORFLAGS_LOCK_WHEN_CLOSED		= M_Bit(14),
		SWINGDOORFLAGS_CD_OPENED_DOOR		= M_Bit(15),
		SWINGDOORFLAGS_OPENED_BY_SCRIPT		= M_Bit(16),

		SWINGDOORFLAGS_MESSAGE_SENTOPEN		= M_Bit(30),
		SWINGDOORFLAGS_MESSAGE_SENTCLOSE	= M_Bit(31),


		SWINGDOORFLAGS_MESSAGEMASK_ONCE_OPEN = SWINGDOORFLAGS_MESSAGE_ONCE | SWINGDOORFLAGS_MESSAGE_SENTOPEN,
		SWINGDOORFLAGS_MESSAGEMASK_ONCE_CLOSE = SWINGDOORFLAGS_MESSAGE_ONCE | SWINGDOORFLAGS_MESSAGE_SENTCLOSE,

		OBJMSG_SWINGDOOR_HANDLE_OPEN,		//Time for the handle to open is in m_VecParam0.k[0], same is used for close
		OBJMSG_SWINGDOOR_GETRENDERMATRIX,
		OBJMSG_SWINGDOOR_ADDCHILD,
		OBJMSG_SWINGDOORATTACH_SETPARENT,
		/*OBJMSG_SWINGDOOR_OPENOUT,
		OBJMSG_SWINGDOOR_CLOSE,*/

		SWINGDOOR_ACTION_CLOSEDOOR		= 1,

		SWINGDOOR_STATE_CLOSED			= 1 << 0,
		SWINGDOOR_STATE_OPENING_IN		= 1 << 1,
		SWINGDOOR_STATE_OPENING_OUT		= 1 << 2,
		SWINGDOOR_STATE_OPENING			= 1 << 3,
		SWINGDOOR_STATE_CLOSING			= 1 << 4,
		SWINGDOOR_STATE_PHYSICS			= 1 << 5,	//This means that the door has been hit by an object and has speed from the physics
		SWINGDOOR_STATE_DEMONARM		= 1 << 6,	//So we don't know what the door is going to end up like, wait till speed slow downs and then
													//keep it going till it's in good state(full open/close)

		SWINGDOOR_CLIENTDATA_DIRTYMASK_NORMAL				= 1 << 0,

		SWINGDOOR_HANDLE_TYPE_NONE = 0,
		SWINGDOOR_HANDLE_TYPE_HANDLE,
		SWINGDOOR_HANDLE_TYPE_KNOB,

		SWINGDOOR_HANDLE_STATE_IDLE = 0,
		SWINGDOOR_HANDLE_STATE_OPEN,
		SWINGDOOR_HANDLE_STATE_CLOSE,
	};

protected:
	//
	class CSwingDoorClientData : public CReferenceCount
	{
	public:
		CVec3Dfp32 m_Normal;

		uint8 m_DirtyMask;

		CSwingDoorClientData()
		{
			m_Normal = CVec3Dfp32(0.0f,0.0f,1.0f);
		}

		int OnCreateClientUpdate(uint8* _pData);
		int OnClientUpdate(const uint8* _pData);
	};
	class CChild
	{
	public:
		int32 m_iChild;
		CMat4Dfp32 m_OriginalPosition;

		CChild()
		{
			m_iChild = -1;
		}
		CChild(int32 _iChild, const CMat4Dfp32& _OrgPos)
		{
			m_iChild = _iChild;
			m_OriginalPosition = _OrgPos;
		}
	};
	CWO_SimpleMessage	m_OpenMessage;
	CWO_SimpleMessage	m_CloseMessage;
	TArray<CChild>	m_lChildren;
	TArray<CStr>	m_lChildrenNames;
	CMat4Dfp32 m_OriginalPosition;
	CVec3Dfp32 m_Axis;
	CVec3Dfp32 m_Offset;
	CVec3Dfp32 m_Tangent;
	CVec3Dfp32 m_WallPos;	//position on wall where door is
	CVec3Dfp32 m_DemonArmAttach;
	CStr m_Userportal;
	fp32 m_MaxSwingAngle;	//Normal swing angle
	fp32 m_MaxSwingTime;	//normal swing time
	fp32 m_CloseRadius;
	fp32 m_SwingAngle;		//swing angle used, can be modified by a script open msg, will be reset to m_MaxSwingAngle
	fp32 m_SwingTime;		//swing time used, can be modified by a script open msg, will be reset to m_MaxSwingTime
	fp32 m_LastAngle;
	fp32 m_TangentStart;

	int32 m_ConstraintID;
	int32 m_StartTick;
	int32 m_FailedResetStartTick;
	int32 m_SwingDoorFlags;
	int32 m_iUserPortal;
	int32 m_QueuedAction;
	int32 m_LastOpenRequest;

	int32 m_iSoundOpenStart;		// Play when door starts to open
	int32 m_iSoundOpenStop;			// Play when door has stopped opening
	int32 m_iSoundMove;				// Play when door is moving (non looping)
	int32 m_iSoundMoveLoop;			// Play when door is moving (looping)
	int32 m_iSoundShutStart;		// Play when door is starting to shut
	int32 m_iSoundShutStop;			// Play when door has shut

	int32 m_iActivator;
	int32 m_iLastAI;
	int32 m_LastOpenTick;
	int32 m_AIRequestedSPSTick;		//If an AI already got the SP this tick, all other AIs will not get them

	fp32		m_HandleSideOffset;
	fp32		m_DoorHeightOffset;
	fp32		m_HandleSwingTime;
	int32		m_iBehaviour;
	int32		m_liDoorSPs[2];
	int32		m_iHandle1;
	int32		m_iHandle2;
	CMat4Dfp32	m_Handle1OrgMat;
	CMat4Dfp32	m_Handle2OrgMat;
	CVec3Dfp32	m_DoorBottomPos;
	int32		m_iHandleModel;
	uint32		m_HandleStartTick;
	uint32		m_LastDynamicCollisionTick;
	uint8		m_HandleState;
	uint8		m_HandleType;
	uint8		m_CurrentState;

	void GetSwing(CMat4Dfp32& _Mat, const CMTime& _CurrentTime);

	void UpdateDoorPosition();//CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhys);

	void CloseDoor(bool _bForceClose = false);
	void OpenDoorIn(bool _bForceOpen = false);
	void OpenDoorOut(bool _bForceOpen = false);

	void HandleOpen(void);
	void HandleClose(void);

	void SetDrawToNavgrid(bool _bDrawTo);

	void SetRefresh(bool _bOn);
	void Lock(bool _bLock, bool _bFakeIt = false);

	bool GetSetupValues(CVec3Dfp32& _XDir, fp32& _Thickness, CBox3Dfp32& _InBox);
	void CalcHandlePositions();
	void CreateScenepoints();
	void CreateHandles();
	void CreateTriggers();
	void CreateConstraint();

public:
	virtual void OnCreate();

	virtual void OnSpawnWorld();
	//static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	virtual void OnRefresh();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);

	static void OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);
	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static void GetRenderMatrix(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhys, fp32 _IPTime, CMat4Dfp32* _pMat);
	static int OnPhysicsEvent(CWObject_CoreData*, CWObject_CoreData*, CWorld_PhysState*, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo = NULL);

	void FailedToOpenCloseDoor(CVec3Dfp32 _CollisionPoint, CVec3Dfp32 _ObjectPos);

	static CSwingDoorClientData* GetSwingDoorClientData(CWObject_CoreData* _pObj);
	static const CSwingDoorClientData* GetSwingDoorClientData(const CWObject_CoreData* _pObj);

	int OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pObj, uint8* _pData, int _Flags) const;
	static int OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);

	void OnDeltaSave(CCFile* _pFile);
	void OnDeltaLoad(CCFile* _pFile, int _Flags);
	void OnFinishDeltaLoad();
};

#define CWObject_SwingDoorAttachParent CWObject_Model

class CWObject_SwingDoorAttach : public CWObject_SwingDoorAttachParent 
{
	MRTC_DECLARE_SERIAL_WOBJECT;
protected:
	int32 m_iParent;
	CVec3Dfp32 m_Offset;
	CStr m_Parent;

	void SetParent(int32 _iParent);
	void SetOffset(const CVec3Dfp32& _Offset);
public:
	virtual void OnCreate();
	virtual void OnSpawnWorld();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual aint OnMessage(const CWObject_Message& _Msg);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static int32 GetParent(CWObject_CoreData* _pObj);
	static void GetOffset(CWObject_CoreData* _pObj, CVec3Dfp32& _Offset);
};
#endif
