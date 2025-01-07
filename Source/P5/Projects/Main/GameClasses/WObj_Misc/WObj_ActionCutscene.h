/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			WObj_ActionCutscene.h

Author:			Olle Rosenquist

Copyright:		Copyright O3 Games AB 2002

Contents:		

Comments:		

History:		
021014:		Created File
\*____________________________________________________________________________________________*/

#ifndef _INC_WOBJ_ACTIONCUTSCENE
#define _INC_WOBJ_ACTIONCUTSCENE

#include "../WObj_Sys/WObj_Physical.h"
#include "../WObj_Messages.h"
#include "WObj_ActionCutsceneCamera.h"
//#include "../WRPG/WRPGFist.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_SimpleMessage.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Hook.h"

#define ACTIONCUTSCENEDELAYCOMPENSATION (3)

enum
{
	OBJMSG_ACTIONCUTSCENE_ACTIVATE = OBJMSGBASE_MISC_ACTIONCUTSCENE,
	//OBJMSG_ACTIONCUTSCENE_GETSTARTPOSITION,
	OBJMSG_ACTIONCUTSCENE_GETRELATIVESTARTPOSITION,
	OBJMSG_ACTIONCUTSCENE_GETENDPOSITION,
	OBJMSG_ACTIONCUTSCENE_GETSUCCESSANIM,
	OBJMSG_ACTIONCUTSCENE_GETFAILANIM,
	OBJMSG_ACTIONCUTSCENE_ISACTIONCUTSCENE,
	OBJMSG_ACTIONCUTSCENE_ADDCUTSCENECAMERA,
	OBJMSG_ACTIONCUTSCENE_GETNEWCUTSCENECAMERA,
	OBJMSG_ACTIONCUTSCENE_GETACTIVECUTSCENECAMERA,
	OBJMSG_ACTIONCUTSCENE_ONENDACS,
	OBJMSG_ACTIONCUTSCENE_GETTYPE,
	OBJMSG_ACTIONCUTSCENE_CANENDACS,
	OBJMSG_ACTIONCUTSCENE_ISWAITSPAWNED,
	OBJMSG_ACTIONCUTSCENE_DOTRIGGER,
	OBJMSG_ACTIONCUTSCENE_CANACTIVATE,
	OBJMSG_ACTIONCUTSCENEPICKUP_SETDEFAULTS,
	OBJMSG_ACTIONCUTSCENE_LEVER_SETSTATE,
	OBJMSG_ACTIONCUTSCENE_LEVER_GETSTATE,
	OBJMSG_ACTIONCUTSCENE_VALVE_ONCHANGESTATE,
	OBJMSG_ACTIONCUTSCENE_VALVE_CANTURN,
	OBJMSG_ACTIONCUTSCENE_DOTRIGGERMIDDLE,
	OBJMSG_ACTIONCUTSCENE_SHOULDPAUSEALLAI,
	OBJMSG_ACTIONCUTSCENE_DOTRIGGERREFILL,
	OBJMSG_ACTIONCUTSCENE_GETPICKUPITEMTYPE,
	OBJMSG_ACTIONCUTSCENE_GETFOCUSPOS,
	OBJMSG_ACTIONCUTSCENE_USE3PI,

	OBJMSG_ACTIONCUTSCENE_TELEPHONE_SELECTCHOICE,
	OBJMSG_ACTIONCUTSCENE_GETFOCUSCUTSCENECAMERA,
	OBJMSG_ACTIONCUTSCENE_LEAVEFOCUSCUTSCENECAMERA,
	OBJMSG_ACTIONCUTSCENE_REUSEFOCUSCUTSCENECAMERA,
	OBJMSG_ACTIONCUTSCENE_ISTELEPHONE,
	//OBJMSG_ACTIONCUTSCENE_RIOTACS_SETHEALTH,


	CLIENTFLAGS_ATTACHED = (CWO_CLIENTFLAGS_USERBASE << 0),
	ITEM_PHYS_IDLETICKS = 4,

	ACTIONCUTSCENE_MAXCAMERAS = 6,

	ACTIONCUTSCENE_TYPE_ACS								= 1,
	ACTIONCUTSCENE_TYPE_LADDER,
	ACTIONCUTSCENE_TYPE_HANGRAIL,
	ACTIONCUTSCENE_TYPE_LEDGE,
	ACTIONCUTSCENE_TYPE_PICKUP,
	ACTIONCUTSCENE_TYPE_RIOT,

	// Operation flags.....
	// Flags that can be set in ogier, do not change these values!
	ACTIONCUTSCENE_FLAGS_ROTATE							= 1 << 0,
	ACTIONCUTSCENE_FLAGS_TELEPHONEACTIVE				= 1 << 1,
	ACTIONCUTSCENE_FLAGS_WAITSPAWN						= 1 << 2,
	ACTIONCUTSCENE_OPERATION_CROUCHATEND				= 1 << 3,
	ACTIONCUTSCENE_OPERATION_DISABLEPHYSICS				= 1 << 4,
	ACTIONCUTSCENE_OPERATION_PLAYSUCCESSANIMATION		= 1 << 5,
	ACTIONCUTSCENE_OPERATION_PLAYFAILANIMATION			= 1 << 6,
	ACTIONCUTSCENE_OPERATION_USETHIRDPERSON				= 1 << 7,
	ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESS				= 1 << 8,
	ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESSEND			= 1 << 9,
	ACTIONCUTSCENE_OPERATION_TRIGGERFAIL				= 1 << 10,
	ACTIONCUTSCENE_OPERATION_TRIGGERFAILEND				= 1 << 11,
	ACTIONCUTSCENE_OPERATION_DEPENDSONITEM				= 1 << 12,
	ACTIONCUTSCENE_OPERATION_SUCCESSSETSTARTPOSITION	= 1 << 13,
	ACTIONCUTSCENE_OPERATION_SUCCESSSETENDPOSITION		= 1 << 14,
	ACTIONCUTSCENE_OPERATION_FAILSETSTARTPOSITION		= 1 << 15,
	ACTIONCUTSCENE_OPERATION_FAILSETENDPOSITION			= 1 << 16,
	ACTIONCUTSCENE_OPERATION_LOCKED						= 1 << 17,
	ACTIONCUTSCENE_OPERATION_DISABLEONSUCCESS			= 1 << 18,
	ACTIONCUTSCENE_FLAGS_PAUSEALLAI						= 1 << 19,

	ACS_CLIENTFLAG_ROTATE								= (CWO_CLIENTFLAGS_USERBASE << 13),

	//Internal flags, values can be changed
	ACTIONCUTSCENE_FLAGS_ISSUCCESS						= 1 << 20,
	ACTIONCUTSCENE_FLAGS_ISFAIL							= 1 << 21,
	ACTIONCUTSCENE_FLAGS_USEFLOATINGSTART				= 1 << 22,
	ACTIONCUTSCENE_FLAGS_HASDEFINEDDIRECTION			= 1 << 23,
	ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESSMIDDLE		= 1 << 24,
	ACTIONCUTSCENE_OPERATION_TRIGGERFAILMIDDLE			= 1 << 25,
	ACTIONCUTSCENE_OPERATION_USEPERFECTPOS				= 1 << 26,
	ACTIONCUTSCENE_OPERATION_DISABLE3PI					= 1 << 27,
	ACTIONCUTSCENE_OPERATION_LINKACS					= 1 << 28,
	ACTIONCUTSCENE_OPERATION_USEORGPOS					= 1 << 29,
	ACTIONCUTSCENE_OPERATION_SAVESEQPOS					= 1 << 30,
	ACTIONCUTSCENE_OPERATION_SETNOREFRESH				= 1 << 31,


	ACTIONCUTSCENE_OPERATION_SUCCESSMASK				= (ACTIONCUTSCENE_OPERATION_USETHIRDPERSON |
	ACTIONCUTSCENE_OPERATION_SUCCESSSETSTARTPOSITION |
	ACTIONCUTSCENE_OPERATION_SUCCESSSETENDPOSITION |
	ACTIONCUTSCENE_OPERATION_DISABLEPHYSICS |
	ACTIONCUTSCENE_OPERATION_CROUCHATEND |
	ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESSEND |
	ACTIONCUTSCENE_OPERATION_DISABLEONSUCCESS |
	ACTIONCUTSCENE_FLAGS_HASDEFINEDDIRECTION |
	ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESSMIDDLE |
	ACTIONCUTSCENE_OPERATION_USEPERFECTPOS|
	ACTIONCUTSCENE_OPERATION_USEORGPOS|
	ACTIONCUTSCENE_OPERATION_SAVESEQPOS),

	ACTIONCUTSCENE_OPERATION_SETSTARTPOSMASK			= (ACTIONCUTSCENE_OPERATION_SUCCESSSETSTARTPOSITION |
	ACTIONCUTSCENE_OPERATION_FAILSETSTARTPOSITION),
	ACTIONCUTSCENE_OPERATION_SETENDPOSMASK			= (ACTIONCUTSCENE_OPERATION_SUCCESSSETENDPOSITION |
	ACTIONCUTSCENE_OPERATION_FAILSETENDPOSITION),

	ACTIONCUTSCENE_OPERATION_FAILMASK					= (ACTIONCUTSCENE_OPERATION_USETHIRDPERSON |
	ACTIONCUTSCENE_OPERATION_FAILSETSTARTPOSITION |
	ACTIONCUTSCENE_OPERATION_FAILSETENDPOSITION |
	ACTIONCUTSCENE_OPERATION_DISABLEPHYSICS |
	ACTIONCUTSCENE_OPERATION_TRIGGERFAILEND |
	ACTIONCUTSCENE_FLAGS_HASDEFINEDDIRECTION |
	ACTIONCUTSCENE_OPERATION_TRIGGERFAILMIDDLE |
	ACTIONCUTSCENE_OPERATION_USEPERFECTPOS|
	ACTIONCUTSCENE_OPERATION_USEORGPOS|
	ACTIONCUTSCENE_OPERATION_SAVESEQPOS),

	ACTIONCUTSCENE_IMPULSE_UNLOCK						= 2,
	ACTIONCUTSCENE_IMPULSE_LOCK							= 3,
	ACTIONCUTSCENE_IMPULSE_SENDSUCCESSMESSAGES			= 4,
	ACTIONCUTSCENE_IMPULSE_FORCEACTIVATE				= 5,

	// Seems we must have fixed types of action cutscenes (or atleast the types of animations that
	// we have)
	ACTIONCUTSCENE_TYPE_HATCHINVERTICALHIGH = 1,
	ACTIONCUTSCENE_TYPE_HATCHOUTVERTICALHIGH,
	ACTIONCUTSCENE_TYPE_HATCHINVERTICALLOW,
	ACTIONCUTSCENE_TYPE_HATCHOUTVERTICALLOW,
	ACTIONCUTSCENE_TYPE_HATCHINHORIZONTAL,
	ACTIONCUTSCENE_TYPE_HATCHOUTHORIZONTAL,
	ACTIONCUTSCENE_TYPE_PICKUPITEMGROUND,
	ACTIONCUTSCENE_TYPE_PICKUPITEMTABLE,
	ACTIONCUTSCENE_TYPE_USEKEYPAD,
	ACTIONCUTSCENE_TYPE_USEMEDSTATION,
	ACTIONCUTSCENE_TYPE_USEPOWERCELL,
	ACTIONCUTSCENE_TYPE_JUSTACTIVATE,
	ACTIONCUTSCENE_TYPE_USEBUTTON,
	ACTIONCUTSCENE_TYPE_LEVER,
	ACTIONCUTSCENE_TYPE_VALVE,
	ACTIONCUTSCENE_TYPE_USEDNA,
	// Don't use when scripting please
	ACTIONCUTSCENE_TYPE_RECHARGEMEDSTATION,
	ACTIONCUTSCENE_TYPE_USEEYESCANNER,
	ACTIONCUTSCENE_TYPE_USETERMINAL,
	ACTIONCUTSCENE_TYPE_ENTERHEAVY,
	ACTIONCUTSCENE_TYPE_GETMINIFROMPOD,
	ACTIONCUTSCENE_TYPE_USEHEALTHSTATION,
	ACTIONCUTSCENE_TYPE_USESECURITYCARD,
	ACTIONCUTSCENE_TYPE_KICKGRATE,
	ACTIONCUTSCENE_TYPE_CROUCH_PICKUPITEMGROUND,
	ACTIONCUTSCENE_TYPE_CROUCH_PICKUPITEMTABLE,
	ACTIONCUTSCENE_TYPE_USEBUTTON_LONG,
	ACTIONCUTSCENE_TYPE_ENTERRIOT,
	ACTIONCUTSCENE_TYPE_TELEPHONE,
	ACTIONCUTSCENE_TYPE_PERISCOPE,
	//ACTIONCUTSCENE_TYPE_STEALCRYOBOX,

	ACTIONCUTSCENE_TYPE_NUMTYPES = ACTIONCUTSCENE_TYPE_PERISCOPE,

	ACTIONCUTSCENE_TYPE_UNDEFINED = 0,

	ACS_AGCRAPMODE_SUCCSENT			= 1 << 0,
	ACS_AGCRAPMODE_SUCCENDSENT		= 1 << 1,
	ACS_AGCRAPMODE_FAILSENT			= 1 << 2,
	ACS_AGCRAPMODE_FAILENDSENT		= 1 << 3,
	ACS_AGCRAPMODE_MIDDLESUCCSENT	= 1 << 4,
	ACS_AGCRAPMODE_MIDDLEFAILSENT	= 1 << 5,
};

enum
{

	ACS_INITSTRING_CAMERAINITPARAMS = 0,
	ACS_INITSTRING_STARTPOSITION,
	ACS_INITSTRING_ENDPOSITION,
	ACS_INITSTRING_CAMERAV2CONTROLLED,
	ACS_INITSTRING_CAMERAV2CONTROLLEDONCHAR,
	ACS_INITSTRING_FOCUSCAMERA,
	ACS_INITSTRING_LINKACS,
	ACS_INITSTRING_NEEDED,
};

class CACSOffsetEntry
{
public:
	CMat4Dfp32 m_Mat;
	CACSOffsetEntry(CVec3Dfp32 _Offset, fp32 _RotX, fp32 _RotY, fp32 _RotZ)
	{
		m_Mat.Unit();
		m_Mat.SetZRotation3x3(_RotZ);
		CMat4Dfp32 Temp;
		Temp.Unit();
		Temp.SetYRotation3x3(_RotY);
		Temp.Multiply(m_Mat, m_Mat);
		Temp.SetYRotation3x3(_RotX);
		Temp.Multiply(m_Mat, m_Mat);
		_Offset.SetMatrixRow(m_Mat,3);
	}
};

#define CWObject_ActionCutsceneParent CWObject_Engine_Path
class CWObject_ActionCutscene : public CWObject_ActionCutsceneParent
{
	MRTC_DECLARE_SERIAL_WOBJECT;
protected:
	//HOOK_DECLARE_WOBJECT(CWObject_ActionCutscene, CWObject_Engine_Path, CAttachClientData_Engine_Path);
	// Need this info on client if we are to make a smooth camera
	class CACSClientData : public CReferenceCount
	{
	public:
		enum
		{
			TELEPHONE_CURRENTSELECTION_DIRTY = 1 << 0,
			TELEPHONE_CURRENTNUMBER_DIRTY = 1 << 1,
			TELEPHONE_CHOICESTRING_DIRTY = 1 << 2,
			TELEPHONE_CUTSCENEFLAGS_DIRTY = 1 << 3,
			TELEPHONE_DISABLED_DIRTY = 1 << 4,
			TELEPHONE_TELEPHONEREGISTRY_DIRTY = 1 << 5,
			ACS_ACTIVATIONARC_DIRTY = 1 << 6,
			ACS_CACHEDMAT_DIRTY = 1 << 7,
			ACS_RETRYCOUNTDOWN_DIRTY = 1 << 8,

			TELEPHONE_DIRTYMASK_ALL = (TELEPHONE_CURRENTSELECTION_DIRTY | TELEPHONE_CURRENTNUMBER_DIRTY | TELEPHONE_CHOICESTRING_DIRTY | TELEPHONE_CUTSCENEFLAGS_DIRTY | TELEPHONE_DISABLED_DIRTY | TELEPHONE_TELEPHONEREGISTRY_DIRTY|ACS_ACTIVATIONARC_DIRTY|ACS_RETRYCOUNTDOWN_DIRTY),
		};

		// Selection coord [0,0] -> [3,4] (representing the telephone keypad)
		CVec2Dint32 m_CurrentSelection;
		CMat4Dfp32 m_CachedSeqMat;
		CStr m_CurrentNumber;
		CStr m_ChoiceString;
		fp32 m_ActivationArc;
		int32 m_CutsceneFlags;
		int32 m_TelePhoneActivationTick;
		int32 m_ActionRetryCountdown;
		uint16 m_iTelephoneRegistry;
		uint16 m_CopyFlags;
		uint8 m_bDisabled;

		CACSClientData()
		{
			Clear();
		}

		void Clear()
		{
			m_CurrentSelection = 0;
			m_CopyFlags = 0;
			m_CurrentNumber.Clear();
			m_ChoiceString.Clear();
			m_CutsceneFlags = ACTIONCUTSCENE_OPERATION_DISABLEPHYSICS | ACTIONCUTSCENE_FLAGS_PAUSEALLAI;
			m_bDisabled = false;
			m_iTelephoneRegistry = 0;
			m_ActivationArc = -1.0f;
			m_CachedSeqMat.Unit();
			m_TelePhoneActivationTick = 0;
			m_ActionRetryCountdown = 0;
		}

		int OnCreateClientUpdate(uint8* _pData);
		int OnClientUpdate(const uint8* _pData);
		void SetCurrentSelection(const CVec2Dint32& _Selection)
		{
			if ((m_CurrentSelection.k[0] != _Selection.k[0]) || 
				(m_CurrentSelection.k[1] != _Selection.k[1]))
			{
				m_CurrentSelection = _Selection;
				m_CopyFlags |= TELEPHONE_CURRENTSELECTION_DIRTY;
			}
		}

		void SetCurrentNumber(CStr _Number)
		{
			if (m_CurrentNumber != _Number)
			{
				m_CurrentNumber = _Number;
				m_CopyFlags |= TELEPHONE_CURRENTNUMBER_DIRTY;
			}
		}

		void SetChoiceString(CStr _Choice)
		{
			if (m_ChoiceString != _Choice)
			{
				m_ChoiceString = _Choice;
				m_CopyFlags |= TELEPHONE_CHOICESTRING_DIRTY;
			}
		}
		void SetCutsceneFlags(int32 _CutsceneFlags)
		{
			if (m_CutsceneFlags != _CutsceneFlags)
			{
				m_CutsceneFlags = _CutsceneFlags;
				m_CopyFlags |= TELEPHONE_CUTSCENEFLAGS_DIRTY;
			}
		}

		void SetDisabled(uint8 _bDisabled)
		{
			if (m_bDisabled != _bDisabled)
			{
				m_bDisabled = _bDisabled;
				m_CopyFlags |= TELEPHONE_DISABLED_DIRTY;
			}
		}

		void SetTelephoneRegistry(uint16 _iTelephoneRegistry)
		{
			if (m_iTelephoneRegistry != _iTelephoneRegistry)
			{
				m_iTelephoneRegistry = _iTelephoneRegistry;
				m_CopyFlags |= TELEPHONE_TELEPHONEREGISTRY_DIRTY;
			}
		}

		void SetActivationArc(fp32 _ActivationArc)
		{
			if (m_ActivationArc != _ActivationArc)
			{
				m_ActivationArc = _ActivationArc;
				m_CopyFlags |= ACS_ACTIVATIONARC_DIRTY;
			}
		}

		void SetCachedMat(const CMat4Dfp32& _Mat)
		{
			if (!m_CachedSeqMat.AlmostEqual(_Mat,0.001f))
			{
				m_CachedSeqMat = _Mat;
				m_CopyFlags |= ACS_CACHEDMAT_DIRTY;
			}
		}

		void SetRetryCountDown(int32 _RetryCountDown)
		{
			if (_RetryCountDown != m_ActionRetryCountdown)
			{
				m_ActionRetryCountdown = _RetryCountDown;
				m_CopyFlags |= ACS_RETRYCOUNTDOWN_DIRTY;
			}
		}

		void SetDirtyAll()
		{
			m_CopyFlags = TELEPHONE_DIRTYMASK_ALL;
		}
	};

	//CFStr m_ItemTemplate;
	CVec3Dfp32 m_StartPositionVector;
	CVec3Dfp32 m_StartRotationVector;
	CMTime m_CachedRunTime;
	int m_iStartPosition;
	int m_iEndPosition;


	int m_ActionRetryCountdownValue;
	int m_iLinkACS;
	int32 m_SetNoRefreshTick;

	// Hmm, should remake these, or atleast find a good way to select animations in Ogier
	uint16 m_ActionCutsceneTypeSuccess;
	uint16 m_ActionCutsceneTypeFail;

	int8 m_FocusFrameOffset;

	uint8 m_AGCrapMode;

	// What type of item to depend on
	CStr m_DependItem;
	CWO_SimpleMessage m_DependMessage;

	// Temporary string init parameters (clears in "OnSpawnWorld")
	TThinArray<CStr>	m_lInitParams;

	//Hmm, is it better to use cfstr or cstr...
	CStr				m_UseName;
	CStr				m_DescName;

	// Messages sent at the beginning/end of the acs
	// Multiple messages for the acs start
	TThinArray<CWO_SimpleMessage> m_lMsg_TriggerSuccess;
	TThinArray<CWO_SimpleMessage> m_lMsg_TriggerSuccessMiddle;
	TThinArray<CWO_SimpleMessage> m_lMsg_TriggerSuccessEnd;
	TThinArray<CWO_SimpleMessage> m_lMsg_TriggerFail;
	TThinArray<CWO_SimpleMessage> m_lMsg_TriggerFailMiddle;
	TThinArray<CWO_SimpleMessage> m_lMsg_TriggerFailEnd;
	TThinArray<CWO_SimpleMessage> m_lMsg_OnEnterFocusCamera;
	TThinArray<CWO_SimpleMessage> m_lMsg_OnLeaveFocusCamera;

	// Active cutscene camera
	spCActionCutsceneCamera m_spActiveCutsceneCamera;

	// How long time the Actioncutscene is activated (minimum)
	fp32 m_MinActivationTime;
	int	m_MinActivationTimer;

	// Defined direction
	fp32	m_DefinedDirection;

	// Array of camera configs
	CActionCutsceneCamera::ConfigBlock	m_lCameraConfigs[ACTIONCUTSCENE_MAXCAMERAS];
	int m_CameraCount;
	CActionCutsceneCamera::ConfigBlock m_FocusCameraConfig;

	virtual spCActionCutsceneCamera GetActionCutsceneCamera(int _iObject, int _iCharacter, 
		int _Specific = -1);

	virtual bool GetActionCutsceneCamera(int _iObject, int _iCharacter, CCameraUtil* _pCamUtil);
	virtual bool GetFocusCutsceneCamera(int _iObject, int _iCharacter, CCameraUtil* _pCamUtil, bool _bIgnorePrev = false);

	// Ok then, have a static array for animations offsets
	/*static CACSOffsetEntry m_slStartPosition[ACTIONCUTSCENE_TYPE_NUMTYPES];
	static CACSOffsetEntry m_slEndPosition[ACTIONCUTSCENE_TYPE_NUMTYPES];*/

	static CVec3Dfp32 m_slStartPosition[ACTIONCUTSCENE_TYPE_NUMTYPES];
	static CVec3Dfp32 m_slEndPosition[ACTIONCUTSCENE_TYPE_NUMTYPES];

	virtual bool CanActivate(int _iCharacter);
	static bool CanActivateClient(CWorld_PhysState* pWPhys, CWObject_CoreData* _pObj, int _iCharacter);

	virtual bool DoAction(int _iCharacter);
	virtual bool DoActionSuccess(int _iCharacter);
	virtual bool DoActionFail(int _iCharacter);
	virtual bool OnEndACS(int _iCharacter);
	virtual bool IsACS() { return true;	}
	void SetStartPosition(int32 _iCharacter);
	virtual void OnEnterFocusCamera(int32 _iChar);
	virtual void OnLeaveFocusCamera(int32 _iChar);
public:
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual void OnSpawnWorld();
	virtual void OnRefresh();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	static void OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);
	static void OnClientPrecacheClass(CWorld_Client* _pWClient, CXR_Engine* _pEngine);

	virtual void TagAnimationsForPrecache(class CWAGI_Context* _pContext, class CWAGI* _pAgi, TArray<int32>& _liACS);
	virtual void TagAnimationsForPrecache(CWAG2I_Context* _pContext, CWAG2I* _pAGI, TArray<int32>& _liACS);

	//static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);

	//static bool OnIntersectLine(CWObject_CoreData*, CWorld_PhysState*, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _ObjectFlags, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);
	static uint16 ResolveActionCutsceneType(const CStr _Value);

	static bool GetStartPosition(CMat4Dfp32& _StartPos, CWO_Character_ClientData* pCD, CWorld_PhysState* _pWPhysState);
	static bool GetEndPosition(CMat4Dfp32& _EndPos, CWO_Character_ClientData* pCD, CWorld_PhysState* _pWPhysState);

	static bool GetAnimPosition(CMat4Dfp32& _AnimPos, CWO_Character_ClientData* pCD, CWorld_PhysState* _pWPhysState);


	static CACSClientData* GetACSClientData(CWObject_CoreData* _pObj);
	static const CACSClientData* GetACSClientData(const CWObject_CoreData* _pObj);

	int OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pObj, uint8* _pData, int _Flags) const;
	static int OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);

	void OnDeltaSave(CCFile* _pFile);
	void OnDeltaLoad(CCFile* _pFile, int _Flags);
	CMat4Dfp32 GetRenderMatrix(CWorld_PhysState *_pWPhysState, const CMTime& _Time, int32 _GameTick, fp32 _TickFraction);
	virtual void Run(int _iType);
};

#define CWObject_LeverActionCutsceneParent CWObject_ActionCutscene
class CWObject_LeverActionCutscene : public CWObject_LeverActionCutsceneParent
{
	MRTC_DECLARE_SERIAL_WOBJECT;
protected:
	uint8	m_LeverState;

	void OnChangeState(int32 _iTarget);
public:
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual bool DoActionSuccess(int _iCharacter);

	enum
	{
		LEVERSTATE_UP,
		LEVERSTATE_DOWN,
	};
};

#define CWObject_ValveActionCutsceneParent CWObject_ActionCutscene
class CWObject_ValveActionCutscene : public CWObject_ValveActionCutsceneParent
{
	MRTC_DECLARE_SERIAL_WOBJECT;
protected:
	int8	m_ValveFlags;
	//int8	m_ValveState;
	//int8	m_ValveStateActivateLeft;
	//int8	m_ValveStateActivateRight;

	void OnChangeState(int32 _iTarget);
public:
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual bool DoActionSuccess(int _iCharacter);

	enum
	{
		VALVESTATE_CHANGELEFT	= -1,
		VALVESTATE_CHANGERIGHT	= 1,

		VALVESTATE_STARTLEFT	= 2,
		VALVESTATE_STARTRIGHT	= 3,

		VALVESTATE_CANACTIVATELEFT		= 1 << 0,
		VALVESTATE_CANACTIVATERIGHT		= 1 << 1,
		VALVESTATE_ISRIGHT				= 1 << 2,
	};
};

#define CWObject_ActionCutsceneRiotParent CWObject_ActionCutscene
class CWObject_ActionCutsceneRiot : public CWObject_ActionCutsceneRiotParent
{
	MRTC_DECLARE_SERIAL_WOBJECT;
protected:
	//static int32 m_siCurrentActive;
	static CStr m_sRiotAcsName;
	static TPtr<CRPG_Object> m_sspRPGObject;
	static bool m_bNeedRespawn;
	static int32 m_sMaxHealth;
	static int32 m_sCharHealth;
	//static int32 m_sRiotHealth;
	static int32 m_sRiotHealth;
public:
	virtual void OnCreate();
	aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnFinishEvalKeys();
	static void OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);
	virtual bool DoActionSuccess(int _iCharacter);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static void CharSave(CWorld_Server* _pWServer, CCFile* _pFile, int32 _iChar, CWO_Character_ClientData* _pCD);
	static void CharLoad(CWorld_Server* _pWServer, CCFile* _pFile, int32 _iChar);
	static void CheckRespawn(CWorld_Server* _pWServer, int32 _iChar);

	static int32 RespawnCharacter(CWorld_Server* _pWServer, int32 _iChar);
};


#define CWObject_DCActioncutsceneParent CWObject_ActionCutscene
class CWObject_DCActioncutscene : public CWObject_DCActioncutsceneParent
{
	MRTC_DECLARE_SERIAL_WOBJECT;
protected:
	enum
	{
		DCACS_ROTATEPOS = 4,
		DCACS_PAUSEPOS = 5,
		DCACS_CONTROLPOS = 6,
		DCACS_DATAPOS = 7,

		DCACS_CONTROL_START = 1,
		DCACS_CONTROL_PAUSE = 2,
		DCACS_CONTROL_CONTINUE = 3,

		DCACS_PLAYBACKMODE_NORMAL = 0,
		DCACS_PLAYBACKMODE_FORCED = 1,
	};
	static int32 m_sVolumeLock;
	CStr m_Sound;
	CStr m_SyncedEntity;
	int32 m_StartTick;
	// Reset when not paused
	int32 m_PauseStartTick;
	int32 m_iActivator;
	int32 m_iSyncedEntity;
	int8 m_PlayBackMode;
	void Pause();
	void Continue();
	void NormalizeVolume();
	void RaiseVolume();
public:
	virtual void OnCreate();
	aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	//virtual void OnFinishEvalKeys();
	void OnSpawnWorld();
	virtual void OnRefresh();
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	virtual bool DoAction(int _iCharacter);
	virtual bool DoActionSuccess(int _iCharacter);
	virtual bool OnEndACS(int _iCharacter);
	//virtual bool DoActionFail(int _iCharacter);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
};

#define CWObject_ActionCutscenePeriscopeParent CWObject_ActionCutscene
class CWObject_ActionCutscenePeriscope : public CWObject_ActionCutscenePeriscopeParent
{
	MRTC_DECLARE_SERIAL_WOBJECT;
protected:
	fp32 m_FOV;
public:
	virtual void OnCreate();
	//aint OnMessage(const CWObject_Message& _Msg);
	virtual bool DoActionSuccess(int _iCharacter);
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	virtual spCActionCutsceneCamera GetActionCutsceneCamera(int _iObject, int _iCharacter, 
		int _Specific = -1);
};

/*#define CWObject_LightSwitchActionCutsceneParent CWObject_ActionCutscene
class CWObject_LightSwitchActionCutscene : public CWObject_LightSwitchActionCutsceneParent
{
MRTC_DECLARE_SERIAL_WOBJECT;
protected:
public:
virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
};*/

#endif
