#ifndef _INC_WOBJ_CHARCLIENTDATA
#define _INC_WOBJ_CHARCLIENTDATA

#include "WObj_Player.h"
#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_AutoVar.h"
#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_DialogueCamera.h"
#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_AnimUtils.h"
#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_DialogueCamera.h"
#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_VoCap.h"
#include "../../Shared/MOS/Classes/GameWorld/WAnimGraph2Instance/WAG2I.h"
#include "../../Shared/MOS/MSystem/Sound/LipSync/LipSync.h"
#include "../../Shared/MOS/XR/XRCloth.h"
#include "WObj_AutoVar_AttachModel.h"
#include "WObj_AutoVar_Mod.h"
#include "WObj_CharDialogue.h"
#include "WFixedCharMovement.h"
#include "WAnimPositioning.h"
#include "CConstraintSystemClient.h"
#include "WAG2_ClientData_Game.h"
#include "WObj_Misc/WObj_CameraUtil.h"
#include "WObj_Misc/WObj_Burnable.h"
#include "WObj_Char/WObj_CharBlink.h"
#include "WObj_Char/WObj_CharRagdoll.h"

#include "CPostAnimSystem.h"

class CWO_Character_ClientData;
class CWO_CharPlayer_ClientData;
class CWO_CharNPC_ClientData;
class CWO_CharDarkling_ClientData;

#define ANIMGRAPH2_ENABLED
#ifndef M_RTM
#define WDEBUGCONTROL_ENABLED
#endif
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Character_HealthHudItem
|__________________________________________________________________________________________________
\*************************************************************************************************/
#define PLAYER_MAXHEALTH_HUD		20		// Max 10 plupps (with 2 healthhuditems each) rendered on screen
class CWO_Character_HealthHudItem
{
public:
	uint8 m_Fraction : 4;
	uint8 m_TargetFraction : 4;

	CWO_Character_HealthHudItem();
	void SetFraction(int _Fraction);
	void Refresh(int _GameTick);
};


class CDialogueLink
{
public:
	uint32 m_ItemHash;
	bool	m_bIsPlayer;

	CDialogueLink()
		: m_ItemHash(0)
		, m_bIsPlayer(false) {}

	void Set(const char* _pName, bool _bIsPlayer)
	{
		m_ItemHash = StringToHash(_pName);
		m_bIsPlayer = _bIsPlayer;
	}

	M_FORCEINLINE bool IsValid() const { return IS_VALID_ITEMHASH(m_ItemHash); }

	void Pack(uint8*& _pD) const
	{
		TAutoVar_Pack(m_ItemHash, _pD);
		uint8 bState = m_bIsPlayer ? 1 : 0;
		TAutoVar_Pack(bState, _pD);
	}

	void Unpack(const uint8*& _pD)
	{
		TAutoVar_Unpack(m_ItemHash, _pD);
		uint8 bState;
		TAutoVar_Unpack(bState, _pD);
		m_bIsPlayer = bState ? true : false;
	}
};





// Inventory info class
class CWO_Inventory_Info
{
public:
	enum
	{
		INVENTORYINFO_TYPE_MISSION = M_Bit(0),
		INVENTORYINFO_TYPE_INVENTORY = M_Bit(1),
		INVENTORYINFO_TYPE_PHONEBOOK = M_Bit(2),
		INVENTORYINFO_SUBTYPE_WEAPON_GUN = M_Bit(3),
		INVENTORYINFO_SUBTYPE_WEAPON_RIFLE = M_Bit(4),
	};
	class CInventoryItem
	{
	public:
		// Usually localize strings
		CStr m_ItemName;
		CStr m_ItemDescription;
		// If not -1, show activation surface for (set amount of time?)
		int32 m_ActivationTick;
		// Unique item identifier
		int32 m_Identifier;
		int16 m_NumItems;
		int16 m_iMissionSurface;
		int16 m_iActivationSurface;
		int16 m_Flags;
		int16 m_iJournalImage;
		int16 m_iAmount;

		CInventoryItem() { Clear(); }
		CInventoryItem(const CInventoryItem& _From)
		{
			*this = _From;
		}
		void Clear();
		CInventoryItem& operator= (const CInventoryItem& _Item);
	};
	// Keep everything in one inventory for now
	TArray<CInventoryItem> m_lInventory;
	int16 m_iGUITaggedItem;
	//TArray<CInventoryItem> m_lInventoryMissions;

	// No need to save this, just update after a load
	CWO_Inventory_Info();
	void Clear();
	//void DebugPrint();
	void AddItem(CRPG_Object_Item* _pItem, int32 _Tick = -1, int16 _Flags = 0);
	void CopyFrom(const CWO_Inventory_Info& _CWO_Inventory_Info);
	void Write(CCFile* _pFile) const;
	void Read(CCFile* _pFile);
	int GetInsertPosition(int32 _Identifier);
	// Creates update from mirror and only copying over what's needed instead of copying a bunch of 
	// stuff just because 1 has changed, does however require atleast 1 byte per update unless we can 
	// borrow a dirtymask from autovar...
	int OnCreateClientUpdate(uint8* _pData, const CWO_Inventory_Info& _Mirror, int8 _PackType) const;
	int OnClientUpdate(const uint8* _pData, const uint8& _SharedFlags);
	int OnCreateClientUpdate2(uint8* _pData, const CWO_Inventory_Info& _Mirror, uint8& _SharedFlags) const;
	int16 GetNextActivationSurface(int32& _iStartIndex, int32 iLastTick, int32& _PickupTick);
	// Needed for gui
	dllvirtual int32 GetNumMatches(const int16 _Flags) const;
	dllvirtual const CInventoryItem* GetNth(const int16 _Flags, const int32 _Index) const;
};

class CAutoVar_WeaponStatus
{
public:
	enum
	{
		// 0 - 4 replicated
		WEAPONSTATUS_HASSOUND			= M_Bit(0),
		WEAPONSTATUS_SINGLESHOT			= M_Bit(1),
		WEAPONSTATUS_HASTIMEOUT			= M_Bit(2),
		// When this flag is set, m_Ammoload is darkness drain amount
		WEAPONSTATUS_DARKNESSDRAIN		= M_Bit(3),
		WEAPONSTATUS_MELEEWEAPON		= M_Bit(4),

		// 5 - 7 clientside safe
		WEAPONSTATUS_TRIGGERPRESSED		= M_Bit(5),
	};
	// Depending on weapon, activation times are different
	int32 m_CurrentTimeOut;
	int32 m_LastSoundTick;
	int16 m_FireTimeOut;
	int16 m_iSoundActivatePlayer;
	int16 m_AmmoLoad;
	int8 m_Flags;
	

	CAutoVar_WeaponStatus()	{ Clear(); }

	void Clear();
	void CopyFrom(const CAutoVar_WeaponStatus& _From);
	bool OkToFire(class CWO_Character_ClientData* _pCD, CWorld_PhysState* _pWPhys);
	void Fire(class CWO_Character_ClientData* _pCD, bool _bGodMode);
	void Pack(uint8 *&_pD, CMapData* _pMapData) const;
	void Unpack(const uint8 *&_pD, CMapData* _pMapData);
};

class CAutoVar_DarknessLevel
{
private:
	enum EDarknessLevel
	{
		DARKNESS_INCREASE		= 20,
		HEARTS_LEVEL			= 20,
		HEARTS_MAXLEVEL			= 4,

		MAX_LEVEL				= 1,
		LEVEL_DEMONARM			= (MAX_LEVEL*0),
		LEVEL_CREEPINGDARK		= (MAX_LEVEL*1),
		LEVEL_ANCIENTWEAPONS	= (MAX_LEVEL*2),
		LEVEL_BLACKHOLE			= (MAX_LEVEL*3),
		LEVEL_SHIELD			= (MAX_LEVEL*4),
		LEVEL_VISION			= (MAX_LEVEL*5),
		
		LEVEL_UNKNOWN			= (MAX_LEVEL*6),
	};
	uint16	m_Hearts;
	uint8	m_Levels;

	EDarknessLevel GetLevelEnum(uint _Power);

public:
	CAutoVar_DarknessLevel() { Clear(); }

	void Clear();
	void CopyFrom(const CAutoVar_DarknessLevel& _From);
	void Pack(uint8 *&_pD, CMapData* _pMapData) const;
	void Unpack(const uint8 *&_pD, CMapData* _pMapData);
	void OnDeltaSave(CCFile* _pFile);
	void OnDeltaLoad(CCFile* _pFile);

	void AddHeart();
	void SetLevel(uint _Level);
	void SetPowerLevel(uint _Power, uint _Level);
	
	uint GetMaxDarkness();
	uint GetHearts();
	virtual uint GetLevel();
	uint GetLevelHearts();
	virtual uint GetPowerLevel(uint _Power);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Character_ClientData
|__________________________________________________________________________________________________
\*************************************************************************************************/
#define PLAYER_NUMANIMCACHESLOTS 2

class CWO_Character_ClientData : public CWO_Player_ClientData
{
	MRTC_DECLARE;
	AUTOVAR_SETCLASS(CWO_Character_ClientData, CAutoVarContainer);

public:
	CWorld_PhysState* m_pWPhysState;

public:

	class CWO_Clientdata_Character_AnimGraph2AGI : public CWO_Clientdata_Character_AnimGraph2
	{
	public:
		CWO_Character_ClientData *m_pClientData;
		CWO_Clientdata_Character_AnimGraph2AGI()
		{
			m_pClientData = NULL;
			m_spAG2I = MNew(CWAG2I);
		}
		spCWAG2I m_spAG2I;

		virtual CWAG2I* GetAG2I() { return (m_spAG2I); }
		virtual const CWAG2I* GetAG2I() const { return (m_spAG2I); }

		virtual void AG2_RefreshGlobalProperties(const CWAG2I_Context* _pContext)
		{
			m_pClientData->AG2_RefreshGlobalProperties(_pContext);
		}

	};
	class CWO_Clientdata_WeaponAnim_AnimGraph2AGI : public CWO_Clientdata_WeaponAnim_AnimGraph2
	{
	public:
		CWO_Character_ClientData *m_pClientData;
		CWO_Clientdata_WeaponAnim_AnimGraph2AGI()
		{
			m_pClientData = NULL;
			m_spAG2I = MNew(CWAG2I);
		}
		spCWAG2I m_spAG2I;

		virtual CWAG2I* GetAG2I() { return (m_spAG2I); }
		virtual const CWAG2I* GetAG2I() const { return (m_spAG2I); }

		virtual void AG2_RefreshGlobalProperties(const CWAG2I_Context* _pContext)
		{
			m_pClientData->AG2_RefreshGlobalProperties(_pContext);
		}

	};

	CWO_Clientdata_Character_AnimGraph2AGI m_AnimGraph2;
	CWO_Clientdata_WeaponAnim_AnimGraph2AGI m_WeaponAG2;

	// TraceCamera parameters
	fp32 m_fTraceHistory;			// Stores traced camera distance
	fp32 m_fOldTraceHistory;			// Stores the previously traced camera distance
	int m_iRotationIndex;			// Keeps track of the rotations in TraceCamera
	

	////========================================

	// Character cloth
	TArray<CXR_SkeletonCloth> m_lCharacterCloth;

	spCReferenceCount m_lspClientData[CWO_NUMMODELINDICES];

	CWO_CharBlink m_Blink;
	CWO_EyeAnim m_EyeAnim;

	int32 m_lLastSkeletonInstanceTick[PLAYER_NUMANIMCACHESLOTS];
	fp32 m_lLastSkeletonInstanceIPTime[PLAYER_NUMANIMCACHESLOTS];
	CVec3Dfp32 m_lLastSkeletonInstancePos[PLAYER_NUMANIMCACHESLOTS];
	CXR_Skeleton* m_lLastSkeletonInstanceSkel[PLAYER_NUMANIMCACHESLOTS];	// For cache validation only, don't use this for anything. May be an invalid pointer.
	spCXR_SkeletonInstance m_lspSkelInst[PLAYER_NUMANIMCACHESLOTS];
	MRTC_CriticalSection m_SkelInstCacheLock;
	CVec3Dfp32 m_LastHeadpos;

	CSelection* m_PhysSelection;
	uint8 m_Phys_IdleTicks;
	CPotColSet* m_pPhysPCS;

	// Prediction miss correction
	int m_PredMiss_Tick;
	CVec3Dfp32 m_PredMiss_dPos;

	// Fix for "ag refresh on previous times"
	uint32 m_AGRefreshLastTick;
	fp32 m_AGRefreshLastFrac;


	// Controls
	CVec3Dfp32	m_Control_Move;
	CVec3Dfp32	m_Control_Look;
	CVec3Dfp32	m_Control_Look_Wanted; // Added by Mondelore.
	CVec3Dfp32	m_Control_DeltaLook;
	uint32		m_Control_Press;							// Bits are set for buttons that are being pressed 
	uint32		m_Control_LastPress;						//                               were pressed at last input
	uint32		m_Control_Released;							//                               just got released
	uint32		m_Control_Hold;								//                               have been pressed for a long time period
	uint32		m_Control_LastHold;							//                               had been pressed for a long time period at last update
	uint32		m_Control_Click;							//                               was pressed for a short time period
	uint32		m_Control_Press_Intermediate;
	uint32		m_Control_Press_LastPressAGRefresh;
	int			m_Control_AutoTiltCountDown : 8;
	int			m_Control_IdleTicks;
	uint8		m_Control_PressTicks[16]; // (32 control bits  x  4 bit counter)  used by m_Control_Hold & m_Control_Click

#ifdef WADDITIONALCAMERACONTROL
	int			m_Control_PressAdditional;
	int			m_Control_LastPressAdditional;
	int			m_Control_ReleasedAdditional;
#endif


	int			m_Control_Move_StartTick;
	int			m_PainSoundDelay;

	int			m_SpawnDuration; // May be set to > 0 on character creation and descreased to zero while spawning.
	int			m_iSpawnAnim;
	int			m_iSpawnSound;
	
	int			m_Phys_bInAir : 1;
	int			m_Phys_nInAirFrames : 15;
	int			m_Phys_iLastGroundObject : 16;

	int			m_AnimStance_Sequence;
	fp32			m_AnimStance_LastKeyframeTime;
	int			m_AnimStance_LastKeyframeSeq;

	int32		m_AnimCamBlendInStart;
	int32		m_AnimCamBlendOutStart;

	CControlFrame m_PendingControlFrame;

/*
	CVec3Dfp32	m_ControlSnapshot_Move;
	CVec3Dfp32	m_ControlSnapshot_Look;
	int			m_ControlSnapshot_Press;
	int			m_ControlSnapshot_Released;

	void ControlSnapshot();	
*/

	// repeat sounds (autofire)
	CMTime      m_PlayAhead_LastCreated;
	int         m_lhPlayAheadVoices[3];

	// camera
	bool		m_bCreepingDarkMoveBlock;
	bool		m_Cam_Valid;
	CVec3Dfp32	m_Cam_LastPos;
	CVec3Dfp32	m_Cam_Pos;
	CVec3Dfp32	m_Cam_dPos;
	CQuatfp32	m_Cam_LastRot;
	CQuatfp32	m_Cam_Rot;
	CQuatfp32	m_Cam_dRot;
	fp32			m_Cam_dKneel;
	fp32			m_Cam_Kneel;
	
	// Camera bob settings
	CVec3Dfp32	m_Cam_LastBobMove;
	CVec3Dfp32	m_Cam_BobMove;
	CVec3Dfp32	m_Cam_LastBobWeap;
	CVec3Dfp32	m_Cam_BobWeap;
	
	bool		m_Cam_BobValid;
	CQuatfp32	m_Cam_LastBob;
	CQuatfp32	m_Cam_Bob;
	CAxisRotfp32 m_Cam_Tilt;
	fp32			m_Cam_TiltAnglePrim;
	fp64			m_Cam_TiltTime;
	
	fp32			m_LastStepUpDamper;
	fp32			m_StepUpDamper;
	fp32			m_StepUpDamperPrim;
	
	// Chasecamera distance settings
	fp32			m_ChaseCamDistance;
	fp32			m_dChaseCamDistance;
	fp32			m_LastChaseCamDistance;

	fp32			m_FocusFramePosX;
	fp32			m_FocusFramePosY;

	// Camera effects
	fp32			m_CreepingLens;
	fp32			m_CreepingLensLast;
	fp32			m_AncientLens;
	fp32			m_AncientLensLast;
	fp32			m_HitEffect_Time;
	fp32			m_HitEffect_LastTime;

/*
	// Set in AddChaseCameraOffset to current safetraced chasecamera pos/dir. Used in OnClientRender to trace crosshair.
	CVec3Dfp32	m_ChaseCameraPos;
	CVec3Dfp32	m_ChaseCameraDir;
*/
	
	// Added by Mondelore {START}.
//	CVec3Dfp32	m_Camera_StandHeadOffset; // Offset of head relative feet while standing.
//	CVec3Dfp32	m_Camera_CrouchHeadOffset; // Offset of head relative feet while crouching.
//	CVec3Dfp32	m_Camera_BehindOffset; // Offset of camera behind head.
//	CVec3Dfp32	m_Camera_SpecialOffset; // Special camera offset (weapon and boss overviews).
//	CVec3Dfp32	m_Camera_SpecialRotation; // Special camera rotation (weapon and boss overviews).
	//CVec3Dfp32 m_Camera_FaceOffset; // Offset of camera infront of head.
	CVec3Dfp32	m_Camera_CurHeadOffset; // Current head offset.
	CVec3Dfp32	m_Camera_CurHeadOffsetChange; // Current change in head offset.
	CVec3Dfp32	m_Camera_LastHeadOffset; // Behind offset of previous tick, used for inter frame interpolation.
	CVec3Dfp32	m_Camera_CurBehindOffset; // Current behind head offset (Behind + Special).
	CVec3Dfp32	m_Camera_CurBehindOffsetChange; // Current change in behind offset.
	CVec3Dfp32	m_Camera_LastBehindOffset; // Behind offset of previous tick, used for inter frame interpolation.

/*
	// EXPIRED: SpecialRotation
	CVec3Dfp32	m_Camera_CurSpecialRotation; // Current rotation of chase camera.
	CVec3Dfp32	m_Camera_CurSpecialRotationChange; // Current change in rotation of chase camera.
	CVec3Dfp32	m_Camera_LastSpecialRotation; // Last rotation of chase camera (only used for inter frame interpolation).
	fp32			m_Camera_CurSpecialBlend;
	fp32			m_Camera_CurSpecialBlendChange;
	fp32			m_Camera_LastSpecialBlend;
*/

	// Firstperson Camera settings
	//bool		m_FPCamera_Enable;			// Locks the camera in FP view
	//int		m_FPCamera_Count;			// Counter that prevents the camera from popping in and out of FP view
	//bool		m_FPCamera_ZoomOut;
	//CVec3Dfp32	m_FPCamera_ZoomOffsetChange;
	//CVec3Dfp32	m_FPCamera_ZoomOffset;
	//CVec3Dfp32	m_FPCamera_LastZoomOffset;

	// These settings are used when the camera climb on walls
	//bool		m_Camera_ForceTilt;
	//fp32		m_Camera_CurTilt; 
	//fp32		m_Camera_CurTiltChange;
	//fp32		m_Camera_LastTilt;
	//fp32		m_Camera_OldTilt;
	//fp32		m_Camera_LastTiltBlend;
	//fp32		m_Camera_CurTiltBlend;
	//fp32		m_Camera_CurTiltBlendChange;

	CMTime		m_AutoAim_LastUpdateTime;
	CMTime		m_AutoAim_LastLookMoveTime;

		// Camera shake settings
	uint8		m_CameraShake_Flags;
	int			m_CameraShake_UpdateDelayTicks;
	int			m_CameraShake_Randseed;
	CVec3Dfp32	m_CameraShake_LastMomentum;
	CVec3Dfp32	m_CameraShake_CurMomentum;
	fp32			m_CameraShake_CurBlend;
	fp32			m_CameraShake_LastBlend;
	
	// Camera settings for rotating it when u r dead or in a default cutscene
	fp32			m_Camera_Rotation;
	fp32			m_DefaultCutsceneLaps;


	//Camera filter
	CVec3Dfp32	m_Camera_LastPosition;
	CVec3Dfp32	m_Camera_CharacterOffset;
	CMTime		m_Camera_LastCalcTime;
	fp32			m_Camera_LastSpeedPercent;

		// ForceLook Settings
	CVec2Dfp32	m_ForcedLook;
	CVec2Dfp32	m_LastForcedLook;
	CVec2Dfp32	m_ForcedLookChange;
	fp32			m_ForcedLookBlend;
	fp32			m_LastForcedLookBlend;
	fp32			m_ForcedLookBlendChange;
	
	// ForceTilt Settings
	fp32			m_ForcedTilt;
	fp32			m_LastForcedTilt; 
	fp32			m_ForcedTiltBlend;
	fp32			m_LastForcedTiltBlend;
	fp32			m_ForcedTiltBlendChange; 
	
	// Zoom Settings
	fp32			m_CurZoom;
	fp32			m_LastZoom;
	fp32			m_ZoomSpeed;
	
	int			m_iWeaponFade;

	// Added by Mondelore {END}.

	fp32			m_LastTargetDodgeTime;
	fp32			m_TargetDodgeTime;
	fp32			m_TargetDodgeTimeChange;
	fp32			m_DodgeTime;
	// Health hud
	fp32			m_HealthHudFade;
	fp32			m_LastHealthHudFade;
	fp32			m_HealthHudFadeChange; // Moderated from m_TargetHudState.
	fp32			m_HealthWobble;

	// Angle offset moderation
	fp32			m_ACSLastAngleOffsetX;
	fp32			m_ACSTargetAngleOffsetX;
	fp32			m_ACSAngleOffsetXChange;

	fp32			m_ACSLastAngleOffsetY;
	fp32			m_ACSTargetAngleOffsetY;
	fp32			m_ACSAngleOffsetYChange;

	fp32			m_ACSLastTargetDistance;
	fp32			m_ACSTargetDistance;
	fp32			m_ACSDistanceChange;

	fp32			m_ACSLastTargetRotation;
	fp32			m_ACSTargetRotation;
	fp32			m_ACSRotationChange;

	CVec3Dfp32   m_ACSLastCameraTarget;
	CVec3Dfp32	m_ACSCameraTarget;
	CVec3Dfp32	m_ACSCameraTargetChange;

	// Test new fight moderation
	fp32			m_FightLastAngleX;
	fp32			m_FightTargetAngleX;
	fp32			m_FightAngleXChange;
	fp32			m_FightLastAngleY;
	fp32			m_FightTargetAngleY;
	fp32			m_FightAngleYChange;

	// AnimGraph turncorrection
	/*fp32			m_TurnCorrectionLastAngle;
	fp32			m_TurnCorrectionTargetAngle;*/
	fp32			m_TurnCorrectionAngleChange;

	// Animation rotation velocity
	fp32			m_AnimRotVel;

	/*fp32			m_LastBodyAngleZ;
	fp32			m_TargetBodyAngleZ;
	fp32			m_BodyAngleZChange;*/

	fp32			m_LastDarknessVisibility;
	fp32			m_CurrentDarknessVisibility;
	fp32			m_DarknessVisibilityChange;

	fp32			m_Disorientation_Flash;

	// Ledge "climbon" counter
	int8		m_LedgeClimbOnCounter;
	uint8		m_LedgePhysWidthCached;

	uint16		m_Occlusion;

	int8 m_Anim_iCurIKLookMode;
	int8 m_Anim_iLastIKLookMode;
	fp32 m_Anim_IKLookBlend;

	fp32 m_Anim_LastBodyAngleZ;
	fp32 m_Anim_BodyAngleZ;
	fp32 m_Anim_LastBodyAngleY;
	fp32 m_Anim_LastBodyAngleX;
	CVec3Dfp32 m_LastMove;

	CVec3Dfp32 m_Anim_ModeratedMove;
	CVec3Dfp32 m_Anim_ModeratedMovePrim;

	CQuatfp32 m_Anim_LastLook;
	CQuatfp32 m_Anim_Look;
	
	CLipSync m_LipSync;
	CVoCap m_VoCap;
	CMat4Dfp32 m_VocapOrigin;

	int32 m_WeaponSelectTimeout;

	int32 m_LastGunkataOpponentCheck;
	int16 m_iGunkataOpponent;

	int8 m_GroundMaterial;
	uint8 m_LastFootStepType;
	int32 m_LastFootstepTick;

	// Targeting
	CVec2Dfp32 m_Target_LastAimAngles;
	CVec2Dfp32 m_Target_AimAnglesChange;
	uint16 m_Target_LastObj;

	fp64 m_LastRenderTime;
	CVec3Dfp32 m_LastRenderPos;

	CMTime m_ScreenFlashGameTime;

	CDialogueInstance m_DialogueInstance;
	CDialogueToken m_PlayerDialogueToken;
	CDialogueToken *m_pCurrentDialogueToken;
	CMat4Dfp32 m_PrevDialogueCameraMatrix;
	CWO_DialogueCamera m_DialogueCamera;
	bool m_bFirstFrameOfDialogue;
	CVec3Dfp32 m_DialogCamLook;
	bool m_bWasSwappedSpeaker;
			
	// Choices (Player only)
	int m_nChoices : 4;
	int m_iCurChoice : 4;
	CStr m_Choices;
	TArray<uint32> m_lUsedChoices;
	int16 m_iLastFocusFrameObject;
	int32 m_Go3rdpOnTick;

	fp32 m_ScrollUpTimer;
	fp32 m_ScrollDownTimer;
	fp32 m_LastTime;

	// If we replicate this we end up with a zero on client immediately after it finish and it will
	// cut the effect ugly instead of fading it.
	fp32 m_DarknessVoiceEffect;
#if 1
	bool	m_Debug_ShowHud;
#endif

	// IK
	CPostAnimSystem m_PostAnimSystem;
	bool m_aLegIKIsDirty[4];

	CWO_Character_HealthHudItem m_HealthHudItem[PLAYER_MAXHEALTH_HUD];

	fp32			m_DarknessDrainTime;

	// temp placement 
	CWO_Inventory_Info m_InventoryInfo;

	///////////////////////////////
	// DIRTYMASK_0_0
	///////////////////////////////
	CAUTOVAR_OP(CAutoVar_uint32, m_TargetInfo, DIRTYMASK_0_0);		// Contains target and accuracy, Modified by Target_Get/SetObject/Accuracy
	CAUTOVAR_OP(CAutoVar_uint32, m_TargetAimPos, DIRTYMASK_0_0);		// Modified by Target_Get/SetAimXxxx
	CAUTOVAR_OP(CAutoVar_uint32, m_TargetAimPosVel, DIRTYMASK_0_0);	// Modified by Target_Get/SetAimVelXxxx
	CAUTOVAR_OP(CAutoVar_fp32, m_ActionCutSceneCameraOffsetX, DIRTYMASK_0_0);
	CAUTOVAR_OP(CAutoVar_fp32, m_ActionCutSceneCameraOffsetY, DIRTYMASK_0_0);
	CAUTOVAR_OP(CAutoVar_fp32, m_TurnCorrectionTargetAngleVert, DIRTYMASK_0_0);
	CAUTOVAR_OP(CAutoVar_fp32, m_TurnCorrectionTargetAngle, DIRTYMASK_0_0);
	CAUTOVAR_OP(CAutoVar_fp32, m_TurnCorrectionLastAngle, DIRTYMASK_0_0);
		
	///////////////////////////////
	// DIRTYMASK_0_1
	///////////////////////////////
	CAUTOVAR_OP(CAutoVar_uint8, m_CrosshairInfo, DIRTYMASK_0_1);
	CAUTOVAR_OP(CAutoVar_uint8, m_FreeAimedTargetSurfaceType, DIRTYMASK_0_1);
	CAUTOVAR_OP(CAutoVar_uint8, m_bForceLookOnTarget, DIRTYMASK_0_1);
	CAUTOVAR_OP(CAutoVar_uint8, m_bForceLook, DIRTYMASK_0_1);
	CAUTOVAR_OP(CAutoVar_uint16, m_HudTargetID, DIRTYMASK_0_1);
	CAUTOVAR_OP(CAutoVar_uint8, m_TargetHudState, DIRTYMASK_0_1);
	CAUTOVAR_OP(CAutoVar_uint8, m_Item_ModelFlags, DIRTYMASK_0_1);
	CAUTOVAR_OP(CAutoVar_uint32, m_SelectItemTimeOutTick, DIRTYMASK_0_1);
	CAUTOVAR_OP(CAutoVar_int8, m_iCurSelectedItem, DIRTYMASK_0_1);
	CAUTOVAR_OP(CAutoVar_int8, m_iNextValidSelectItem, DIRTYMASK_0_1);
	CAUTOVAR_OP(CAutoVar_int8, m_iCurEquippedItem, DIRTYMASK_0_1);
	CAUTOVAR_OP(CAutoVar_uint32, m_HealthHudStart, DIRTYMASK_0_1);
	CAUTOVAR_OP(CAutoVar_uint8, m_MoveDir, DIRTYMASK_0_1);
	CAUTOVAR_OP(CAutoVar_CVec3Dfp32, m_EyeLookDir, DIRTYMASK_0_1);
	CAUTOVAR_OP(CAutoVar_int16, m_iEyeLookObj, DIRTYMASK_0_1);

	///////////////////////////////
	// DIRTYMASK_0_2
	///////////////////////////////
	CAUTOVAR_OP(CAutoVar_int32, m_DestroyTick, DIRTYMASK_0_2);
	CAUTOVAR_OP(CAutoVar_int32, m_DeathTick, DIRTYMASK_0_2);
	CAUTOVAR_OP(CAutoVar_uint8, m_Wait, DIRTYMASK_0_2);
	CAUTOVAR_OP(CAutoVar_uint32, m_MuzzleFlashTick0, DIRTYMASK_0_2);
	CAUTOVAR_OP(CAutoVar_uint32, m_MuzzleFlashTick1, DIRTYMASK_0_2);
	CAUTOVAR_OP(CAutoVar_uint8, m_LightningLight, DIRTYMASK_0_2);
	CAUTOVAR_OP(CAutoVar_uint32, m_LightningLightTick0, DIRTYMASK_0_2);
	CAUTOVAR_OP(CAutoVar_uint32, m_LightningLightTick1, DIRTYMASK_0_2);
	CAUTOVAR_OP(CAutoVar_uint8, m_Health, DIRTYMASK_0_2);
	CAUTOVAR_OP(CAutoVar_uint8, m_HitEffect_Flags, DIRTYMASK_0_2);
	CAUTOVAR_OP(CAutoVar_uint16, m_HitEffect_iObject, DIRTYMASK_0_2);
	CAUTOVAR_OP(CAutoVar_int32, m_HitEffect_Duration, DIRTYMASK_0_2);
	CAUTOVAR_OP(CAutoVar_int16, m_HeadGibbed, DIRTYMASK_0_2);
	CAUTOVAR_OP(CAutoVar_uint8, m_bFirstFrameIn3PI, DIRTYMASK_0_2);
	CAUTOVAR_OP(CAutoVar_fp32, m_FightPosTime, DIRTYMASK_0_2);
	CAUTOVAR_OP(CAutoVar_uint8, m_FightModeFlag, DIRTYMASK_0_2);
	CAUTOVAR(CRelativeAnimPos, m_RelAnimPos, DIRTYMASK_0_2);

	///////////////////////////////
	// DIRTYMASK_0_3
	///////////////////////////////	
	CAUTOVAR_OP(CAutoVar_fp32, m_TargetFov, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_fp32, m_FovTime, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_fp32, m_FovTimeScale, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_fp32, m_BaseFov, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_uint32, m_Pickup_StartTick, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_int32, m_FovOverrideStartTick, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_uint16, m_Pickup_iIcon, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_int8, m_Pickup_Magazine_Num, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_uint8, m_FovOverride, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_uint8, m_FovOverridePrevValue, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_int8, m_FovOverrideBlendTime, DIRTYMASK_0_3);
#ifdef WDEBUGCONTROL_ENABLED
	CAUTOVAR_OP(CAutoVar_fp32, m_DebugControlMoveX, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_fp32, m_DebugControlMoveY, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_fp32, m_DebugControlLookH, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_fp32, m_DebugControlLookV, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_fp32, m_DebugControlDLookH, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_fp32, m_DebugControlDLookV, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_int32, m_DebugControlPress, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_int16, m_iDebugControlObject, DIRTYMASK_0_3);
#endif

	///////////////////////////////
	// DIRTYMASK_0_4
	///////////////////////////////
	CAUTOVAR_OP(CAutoVar_int32, m_WeaponUnequipTick, DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_uint16, m_Item0_Num, DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_uint16, m_Item1_Num, DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_uint8, m_ForcedAimingMode, DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_int32, m_WeaponHUDStartTick, DIRTYMASK_0_4);
	CAUTOVAR(CAutoVar_Burnable, m_Burnable, DIRTYMASK_0_4);

	///////////////////////////////
	// DIRTYMASK_0_5
	///////////////////////////////
	CAUTOVAR_OP(CAutoVar_uint8, m_Tension, DIRTYMASK_0_5);
	CAUTOVAR_OP(CAutoVar_uint32, m_LastTeleportTick, DIRTYMASK_0_5);
	CAUTOVAR_OP(CAutoVar_int8, m_FocusFrameType, DIRTYMASK_0_5);
	CAUTOVAR_OP(CAutoVar_int8, m_FocusFrameOffset, DIRTYMASK_0_5);
	CAUTOVAR_OP(CAutoVar_uint8, m_SpecialGrab, DIRTYMASK_0_5);
	CAUTOVAR_OP(CAutoVar_int8, m_Disable, DIRTYMASK_0_5);
	CAUTOVAR_OP(CAutoVar_uint16, m_EquippedItemClass, DIRTYMASK_0_5);
	CAUTOVAR_OP(CAutoVar_uint16, m_EquippedItemType, DIRTYMASK_0_5);
	CAUTOVAR_OP(CAutoVar_int8, m_AnimGripTypeRight, DIRTYMASK_0_5);
	CAUTOVAR_OP(CAutoVar_int8, m_AnimGripTypeLeft, DIRTYMASK_0_5);
	CAUTOVAR_OP(CAutoVar_int16, m_iFocusFrameObject, DIRTYMASK_0_5);
	CAUTOVAR_OP(CAutoVar_uint16, m_iCloseObject, DIRTYMASK_0_5);
	CAUTOVAR(CAutoVar_CFStr, m_FocusFrameUseText, DIRTYMASK_0_5);
	CAUTOVAR(CAutoVar_CFStr, m_FocusFrameDescText, DIRTYMASK_0_5);

	///////////////////////////////
	// DIRTYMASK_0_6
	///////////////////////////////
	CAUTOVAR(CAutoVar_AttachModel, m_Item0_Model, DIRTYMASK_0_6);
	CAUTOVAR_OP(CAutoVar_uint8, m_Item0_Flags, DIRTYMASK_0_6);
	CAUTOVAR_OP(CAutoVar_uint16, m_Item0_AnimSound0, DIRTYMASK_0_6);
	CAUTOVAR_OP(CAutoVar_uint16, m_Item0_AnimSound1, DIRTYMASK_0_6);
	CAUTOVAR_OP(CAutoVar_uint16, m_Item0_AnimSound2, DIRTYMASK_0_6);
	CAUTOVAR_OP(CAutoVar_uint16, m_Item0_AnimSound3, DIRTYMASK_0_6);
	CAUTOVAR_OP(CAutoVar_uint16, m_Item0_Icon, DIRTYMASK_0_6);
	CAUTOVAR(CAutoVar_AttachModel, m_Item1_Model, DIRTYMASK_0_6);
	CAUTOVAR_OP(CAutoVar_uint8, m_Item1_Flags, DIRTYMASK_0_6);
	CAUTOVAR_OP(CAutoVar_uint16, m_Item1_AnimSound0, DIRTYMASK_0_6);
	CAUTOVAR_OP(CAutoVar_uint16, m_Item1_AnimSound1, DIRTYMASK_0_6);
	CAUTOVAR_OP(CAutoVar_uint16, m_Item1_AnimSound2, DIRTYMASK_0_6);
	CAUTOVAR_OP(CAutoVar_uint16, m_Item1_AnimSound3, DIRTYMASK_0_6);
	CAUTOVAR_OP(CAutoVar_uint16, m_Item1_Icon, DIRTYMASK_0_6);
	CAUTOVAR_OP(CAutoVar_uint16, m_AnimSound5, DIRTYMASK_0_6);

	///////////////////////////////
	// DIRTYMASK_1_0
	///////////////////////////////
	CAUTOVAR_OP(CAutoVar_uint8, m_CutsceneFOV, DIRTYMASK_1_0);
	CAUTOVAR_OP(CAutoVar_uint8, m_WeaponZoomState, DIRTYMASK_1_0);
	CAUTOVAR_OP(CAutoVar_uint32, m_WeaponZoomStartTick, DIRTYMASK_1_0);
	CAUTOVAR(TAutoVar_Mod10<uint8>, m_Zoom, DIRTYMASK_1_0);
	CAUTOVAR_OP(CAutoVar_uint32, m_FlashLightColor, DIRTYMASK_1_0);
	CAUTOVAR_OP(CAutoVar_uint16, m_FlashLightRange, DIRTYMASK_1_0);
	CAUTOVAR_OP(CAutoVar_uint32, m_MuzzleLightColor0, DIRTYMASK_1_0);
	CAUTOVAR_OP(CAutoVar_uint32, m_MuzzleLightRange0, DIRTYMASK_1_0);
	CAUTOVAR_OP(CAutoVar_uint32, m_MuzzleLightColor1, DIRTYMASK_1_0);
	CAUTOVAR_OP(CAutoVar_uint32, m_MuzzleLightRange1, DIRTYMASK_1_0);
	CAUTOVAR_OP(CAutoVar_uint32, m_ElectroShockTick, DIRTYMASK_1_0);
	CAUTOVAR_OP(CAutoVar_int32, m_PagerStartTick, DIRTYMASK_1_0);
	CAUTOVAR(CAutoVar_CFStr, m_PagerPhoneNumber, DIRTYMASK_1_0);
	
	///////////////////////////////
	// DIRTYMASK_1_1
	///////////////////////////////
	// Third Person Interactive (3PI):
	CAUTOVAR(TAutoVar_Mod10<uint8>, m_3PI_CameraSlide, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_uint8, m_DarknessVoiceUse, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_uint8, m_3PI_Mode, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_uint8, m_3PI_NoCamera, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_uint8, m_3PI_LightState, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_uint32, m_3PI_LightFadeStart, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_uint8, m_3PI_FocusTicks, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_int32, m_3PI_FocusObject, DIRTYMASK_1_1);
	CAUTOVAR(CCameraUtil, m_CameraUtil, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_int32, m_DarknessBarFlashTick, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_int32, m_DarknessBarFlashColor, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_uint8, m_Darkness, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_int8, m_DarknessSelectedPower, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_int8, m_DarknessLastSelectedPower, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_uint8, m_DarknessPowers, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_uint8, m_DarknessPowersAvailable, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_uint8, m_GUITaggedNewDarknessPower, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_uint8, m_DarknessPowersBackup, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_uint8, m_DarknessVisibility, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_uint16, m_DarknessSelectionMode, DIRTYMASK_1_1);
	//CAUTOVAR_OP(CAutoVar_uint16, m_DarknessDevour, DIRTYMASK_1_1);
	CAUTOVAR(CAutoVar_DarknessLevel, m_DarknessLevel, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_uint16, m_iCreepingDark, DIRTYMASK_1_1);	
	CAUTOVAR(TAutoVar_Mod5<uint8>, m_DarklingSpawn, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_uint32, m_DarklingSpawnRenderChoices, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_uint32, m_DarklingSpawnRenderChoicesStartTick, DIRTYMASK_1_1);
	CAUTOVAR_OP(CAutoVar_int32, m_iFightingCharacter, DIRTYMASK_1_1);

	///////////////////////////////
	// DIRTYMASK_1_2
	///////////////////////////////
	CAUTOVAR(CAutoVar_AttachModel, m_Item2_Model, DIRTYMASK_1_2);
	CAUTOVAR_OP(CAutoVar_uint16, m_nUDMoney, DIRTYMASK_1_2);
	CAUTOVAR_OP(CAutoVar_uint8, m_nMoth, DIRTYMASK_1_2);
	CAUTOVAR(TAutoVar_StaticArray_(CDialogueLink, 6), m_liDialogueChoice, DIRTYMASK_1_2);
	CAUTOVAR_OP(CAutoVar_int32, m_DialogueChoiceTick, DIRTYMASK_1_2);
	CAUTOVAR_OP(CAutoVar_int8,  m_Aim_SkeletonType, DIRTYMASK_1_2);	//This changes when shapeshifting
	CAUTOVAR_OP(CAutoVar_uint8, m_bNightvisionEnabled, DIRTYMASK_1_2);	

	///////////////////////////////
	// DIRTYMASK_1_3
	///////////////////////////////
	//CAUTOVAR(CAutoVar_Inventory, m_PhoneBook, DIRTYMASK_1_3);
	CAUTOVAR_OP(CAutoVar_fp32, m_ControlMode_Param0, DIRTYMASK_1_3);
	CAUTOVAR_OP(CAutoVar_fp32, m_ControlMode_Param1, DIRTYMASK_1_3);
	CAUTOVAR_OP(CAutoVar_fp32, m_ControlMode_Param2, DIRTYMASK_1_3);
	CAUTOVAR_OP(CAutoVar_fp32, m_ControlMode_Param3, DIRTYMASK_1_3);
	CAUTOVAR_OP(CAutoVar_int32, m_ControlMode_Param4, DIRTYMASK_1_3);
	CAUTOVAR_OP(CAutoVar_int32, m_Overviewmap_Visited, DIRTYMASK_1_3);
	CAUTOVAR_OP(CAutoVar_int8, m_Overviewmap_Current, DIRTYMASK_1_3);
	CAUTOVAR_OP(CAutoVar_uint32, m_ExtraFlags, DIRTYMASK_1_3);
	CAUTOVAR_OP(CAutoVar_uint8, m_Analog0, DIRTYMASK_1_3);
	CAUTOVAR_OP(CAutoVar_uint8, m_Analog1, DIRTYMASK_1_3);
	CAUTOVAR_OP(CAutoVar_uint8, m_Analog2, DIRTYMASK_1_3);
	CAUTOVAR_OP(CAutoVar_uint8, m_Analog3, DIRTYMASK_1_3);
	CAUTOVAR_OP(CAutoVar_uint8, m_AnalogMode, DIRTYMASK_1_3);
	CAUTOVAR(CAutoVar_WeaponStatus, m_WeaponStatus0, DIRTYMASK_1_3);
	CAUTOVAR(CAutoVar_WeaponStatus, m_WeaponStatus1, DIRTYMASK_1_3);
	//CAUTOVAR(CAutoVar_Inventory, m_Inventory, DIRTYMASK_1_3);

	///////////////////////////////
	// DIRTYMASK_1_4
	///////////////////////////////
	CAUTOVAR_OP(CAutoVar_int32, m_SpawnTick, DIRTYMASK_1_4);
	CAUTOVAR(TAutoVar_Mod20<uint8>, m_SneakLevel, DIRTYMASK_1_4);
	CAUTOVAR(TAutoVar_Mod20<uint8>, m_CrouchLevel, DIRTYMASK_1_4);
	CAUTOVAR(TAutoVar_Mod20<uint8>, m_SoundLevel, DIRTYMASK_1_4);
	CAUTOVAR(TAutoVar_Mod20<uint8>, m_Opacity, DIRTYMASK_1_4);
	CAUTOVAR(TAutoVar_Mod20<CWObject_WorldSky::CDepthFog>, m_DepthFog, DIRTYMASK_1_4);
	CAUTOVAR_OP(CAutoVar_int16, m_iSurveillanceCamera, DIRTYMASK_1_4);
	CAUTOVAR_OP(CAutoVar_int16, m_AimTarget, DIRTYMASK_1_4);
	CAUTOVAR_OP(CAutoVar_uint32, m_AimTargetStartTick, DIRTYMASK_1_4);
	CAUTOVAR_OP(CAutoVar_uint8, m_AimReleasingTarget, DIRTYMASK_1_4);
	CAUTOVAR_OP(CAutoVar_CVec2Dfp32, m_Creep_Control_Look_Wanted, DIRTYMASK_1_4);
	CAUTOVAR_OP(CAutoVar_CQuatfp32, m_Creep_Orientation, DIRTYMASK_1_4);
	CAUTOVAR_OP(CAutoVar_CQuatfp32, m_Creep_PrevOrientation, DIRTYMASK_1_4);

	///////////////////////////////
	// DIRTYMASK_1_5
	///////////////////////////////
#ifdef INCLUDE_OLD_RAGDOLL
	CAUTOVAR(CConstraintSystemClient, m_RagdollClient, DIRTYMASK_1_5);
#endif // INCLUDE_OLD_RAGDOLL
	CAUTOVAR(CWPhys_RagdollClientData, m_RagdollClientData, DIRTYMASK_1_5);
	
	///////////////////////////////
	// DIRTYMASK_1_6
	///////////////////////////////
	CAUTOVAR_OP(CAutoVar_uint16, m_Cutscene_Camera, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_uint16, m_RenderAttached, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_uint16, m_iAttachJoint, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_int8, m_Anim_IdleStance, DIRTYMASK_1_6); // Replace with Vigilance?
	CAUTOVAR_OP(CAutoVar_uint16, m_DOF_Object, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_uint16, m_DOF_Distance, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_uint8, m_DOF_Amount, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_uint8, m_MotionBlur_Amount, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_uint8, m_MotionBlur_Fuzzyness, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_uint8, m_GlowMotionBlur, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_uint16, m_iDarkness_Drain_1, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_int32, m_LeanStance, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_uint16, m_MaxWeaponRange, DIRTYMASK_1_6); // Could be removed?
	CAUTOVAR_OP(CAutoVar_fp32, m_CameraShake_Amplitude, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_fp32, m_CameraShake_Speed, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_uint32, m_CameraShake_DurationTicks, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_uint32, m_CameraShake_StartTick, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_uint8, m_MountedMode, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_uint16, m_iMountedObject, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_int32, m_MountedStartTick, DIRTYMASK_1_6);
	// Dialogue information
	CAUTOVAR_OP(CAutoVar_uint16, m_iDialogueTargetObject, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_CMat4Dfp32, m_DialoguePlayerPos, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_fp32, m_MaxZoom, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_uint8, m_LiquidTickCountDown, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_uint8, m_Anim_ClimbTickCounter, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_uint16, m_TicksInLiquid, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_uint32, m_Anim_FreezeTick, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_uint32, m_StraightenUpTick, DIRTYMASK_1_6);
	
#ifdef WADDITIONALCAMERACONTROL
	CAUTOVAR_OP(CAutoVar_uint8, m_bAdditionalCameraControl, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_fp32, m_AdditionalCameraControlY, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_fp32, m_AdditionalCameraControlZ, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_CVec3Dfp32, m_AdditionalCameraPos,DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_CVec3Dfp32, m_Control_MoveAdditional,DIRTYMASK_1_6);
#endif
	CAUTOVAR_OP(CAutoVar_int16, m_Map_iSurface, DIRTYMASK_1_6);
	CAUTOVAR_OP(CAutoVar_uint8, m_LookSpeedDamper, DIRTYMASK_1_6);	// 0: normal speed,  128: half speed,  255: freeze

	///////////////////////////////
	// DIRTYMASK_2_0
	// This flag is for things that changes very rarely
	///////////////////////////////
	CAUTOVAR_OP(CAutoVar_uint16, m_iActionCutSceneCameraObject, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_uint16, m_iActionCutSceneCameraTarget, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_int32, m_ActionCutSceneCameraMode, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_fp32, m_ActionCutSceneCameraWantedDistance, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_uint8, m_ActionCutSceneCameraDistanceOffset, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_uint8, m_ActionCutSceneCameraHeightOffset, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_fp32, m_ActionCutSceneCameraAngle, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_uint16, m_Phys_Flags, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_int32, m_iFlagIndex, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_uint8, m_MaxHealth, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_uint8, m_MaxDarkness, DIRTYMASK_2_0);
	CAUTOVAR(CWPhys_RagdollClientMetaData, m_RagdollClientMetaData, DIRTYMASK_2_0);
	//These will change when shapeshifting
	CAUTOVAR_OP(CAutoVar_uint8, m_Phys_DeadRadius, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_uint8, m_Phys_Width, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_uint8, m_Phys_Height, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_uint8, m_Phys_HeightCrouch, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_fp32, m_Speed_WalkForward, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_fp32, m_Speed_Forward, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_fp32, m_Speed_SideStep, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_fp32, m_Speed_Up, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_fp32, m_Speed_Jump, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_uint8, m_ShieldStart, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_int8, m_DemonHeadTargetFade, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_fp32, m_DemonHeadCurrentFade, DIRTYMASK_2_0);

	///////////////////////////////
	// DIRTYMASK_2_1
	// This mask is for things that only changes during creation
	///////////////////////////////
	CAUTOVAR(CAutoVar_CFStr, m_CharName, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_uint16, m_iDarkness_Tentacles, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_int16, m_iPlayer, DIRTYMASK_2_1); //Added by Talbot for Cleaners
	CAUTOVAR_OP(CAutoVar_uint8, m_Phys_SpectatorSize, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_uint16, m_iFacialSetup, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_fp32, m_Camera_FadeOffset, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_fp32, m_MaxTargetingDistance, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_fp32, m_SwimHeight, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_uint16, m_iAnimList, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_uint8, m_AnimOOSBase, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_CVec3Dfp32_Packed32<500>, m_Camera_StandHeadOffset, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_CVec3Dfp32_Packed32<500>, m_Camera_CrouchHeadOffset, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_CVec3Dfp32_Packed32<500>, m_Camera_BehindOffset, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_CVec3Dfp32_Packed32<500>, m_Camera_ShoulderOffset, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_int16, m_Map_PosTop, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_int16, m_Map_PosLeft, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_int16, m_Map_PosBottom, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_int16, m_Map_PosRight, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_fp32, m_CharSkeletonScale, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_fp32, m_CharGlobalScale, DIRTYMASK_2_1); // Global scale parameter
	CAUTOVAR_OP(CAutoVar_int16,  m_AnimLipSync, DIRTYMASK_2_1);
	CAUTOVAR(TAutoVar_StaticArray_(uint16, 5), m_lAnim_VoCap, DIRTYMASK_2_1);
	CAUTOVAR(CAutoVar_CFStr, m_Character, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_fp32, m_ThisAimHeight, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_uint16, m_iLaserBeam, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_uint16, m_iLightCone, DIRTYMASK_2_1);
	CAUTOVAR(CAutoVar_CFStr, m_Overviewmap, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_fp32, m_AutoAimVal, DIRTYMASK_2_1);

	//TODO
	//Check these and make sure they are only changed during creation
	//Might change later?
	CAUTOVAR_OP(CAutoVar_CVec3Dfp32, m_WindForce, DIRTYMASK_2_1);	
	CAUTOVAR_OP(CAutoVar_uint8, m_PannLampa, DIRTYMASK_2_1);		
	CAUTOVAR_OP(CAutoVar_fp32, m_FightPosTimeScale, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_fp32, m_ControlMove_Offset, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_fp32, m_ControlMove_Scale, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_fp32, m_WantedZoomSpeed, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_fp32, m_BackPlane, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_int32, m_ExtraSurfaceColor, DIRTYMASK_2_1);	//not used?
	CAUTOVAR_OP(CAutoVar_int32, m_ExtraSurfaceIndex, DIRTYMASK_2_1);	//not used?
	CAUTOVAR_OP(CAutoVar_fp32, m_ExtraSurfaceTime, DIRTYMASK_2_1);		//not used?
	CAUTOVAR_OP(CAutoVar_fp32, m_CrosshairTime, DIRTYMASK_2_1);			//not used?
	CAUTOVAR_OP(CAutoVar_fp32, m_CrosshairAmount, DIRTYMASK_2_1);		//not used?

	//CAUTOVAR(TAutoVar_StaticArray_(uint16,6), m_liDialogueChoice, DIRTYMASK_1_2);
	
	/*CAUTOVAR_OP(CAutoVar_int32, m_HeadMediumType, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_int32, m_PreviousHeadMediumType, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_int32, m_FeetMediumType, DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_int32, m_PreviousFeetMediumType, DIRTYMASK_2_0);*/
	
	// Added by Phobos: This will be cleaned up! FIXME

	//CAUTOVAR(CFixedCharMovement, m_FCharMovement, DIRTYMASK_1_1);

	// Skeleton scale difference parameter (0.0 == no scaling, 1.0 totally scaled down, -1.0 twice as large)
	//CAUTOVAR_OP(CAutoVar_fp32, m_TurnCorrectionAngleChange, DIRTYMASK_2_1);
	
	/*CAUTOVAR_OP(CAutoVar_fp32, m_LastBodyAngleZ, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_fp32, m_TargetBodyAngleZ, DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_fp32, m_BodyAngleZChange, DIRTYMASK_2_1);*/

	// Auto-aim
//	CAUTOVAR_OP(CAutoVar_CVec3Dfp32_Packed32<500>, CVec3Dfp32, m_ThisAimOffset, DIRTYMASK_2_1);

	// FIXME: MEMOPT
	// Remove and rewrite to FirstPerson instead.

	// Don't think it's used?
	//CAUTOVAR_OP(CAutoVar_CVec3Dfp32_Packed32<500>, CVec3Dfp32, m_Camera_FaceOffset, DIRTYMASK_2_1);


	/*	CAUTOVAR_OP(CAutoVar_uint16, m_Pickup_iQuestIcon0, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_uint16, m_Pickup_iQuestIcon1, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_uint16, m_Pickup_iQuestIcon2, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_uint16, m_Pickup_iQuestIcon3, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_uint16, m_Pickup_iQuestIcon4, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_uint16, m_Pickup_iQuestIcon5, DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_int16, m_Pickup_iQuestCount0, DIRTYMASK_0_3);

	// WHYYYYYYYYYYYYYYYYYYYYYY???????????
	CAUTOVAR(CAutoVar_CFStr, m_Item_Name0, DIRTYMASK_1_4);
	CAUTOVAR(CAutoVar_CFStr, m_Item_Name1, DIRTYMASK_1_4);
	CAUTOVAR(CAutoVar_CFStr, m_Item_Name2, DIRTYMASK_1_4);
	CAUTOVAR(CAutoVar_CFStr, m_Item_Name3, DIRTYMASK_1_4);
	CAUTOVAR(CAutoVar_CFStr, m_Item_Name4, DIRTYMASK_1_4);
	CAUTOVAR(CAutoVar_CFStr, m_Item_Name5, DIRTYMASK_1_4);

	CAUTOVAR(CAutoVar_CFStr, m_Item_Desc0, DIRTYMASK_1_4);
	CAUTOVAR(CAutoVar_CFStr, m_Item_Desc1, DIRTYMASK_1_4);
	CAUTOVAR(CAutoVar_CFStr, m_Item_Desc2, DIRTYMASK_1_4);
	CAUTOVAR(CAutoVar_CFStr, m_Item_Desc3, DIRTYMASK_1_4);
	CAUTOVAR(CAutoVar_CFStr, m_Item_Desc4, DIRTYMASK_1_4);
	CAUTOVAR(CAutoVar_CFStr, m_Item_Desc5, DIRTYMASK_1_4);*/

	AUTOVAR_PACK_BEGIN
	AUTOVAR_PACK_VAR(m_Wait)

	AUTOVAR_PACK_VAR(m_Health)
	AUTOVAR_PACK_VAR(m_MaxHealth)

	AUTOVAR_PACK_VAR(m_bFirstFrameIn3PI)

	AUTOVAR_PACK_VAR(m_DarknessBarFlashTick)
	AUTOVAR_PACK_VAR(m_DarknessBarFlashColor)
	AUTOVAR_PACK_VAR(m_Darkness)
	AUTOVAR_PACK_VAR(m_DarknessSelectedPower)
	AUTOVAR_PACK_VAR(m_DarknessLastSelectedPower)
	AUTOVAR_PACK_VAR(m_DarknessPowers)
	AUTOVAR_PACK_VAR(m_DarknessPowersAvailable)
	AUTOVAR_PACK_VAR(m_GUITaggedNewDarknessPower)
	AUTOVAR_PACK_VAR(m_DarknessPowersBackup)
	AUTOVAR_PACK_VAR(m_MaxDarkness)
	AUTOVAR_PACK_VAR(m_DarknessSelectionMode)
	AUTOVAR_PACK_VAR(m_DarknessLevel)
	AUTOVAR_PACK_VAR(m_PannLampa)
	AUTOVAR_PACK_VAR(m_iCreepingDark)
	AUTOVAR_PACK_VAR(m_DarknessVisibility)

	AUTOVAR_PACK_VAR(m_MaxWeaponRange)
	
	AUTOVAR_PACK_VAR(m_DOF_Object)
	AUTOVAR_PACK_VAR(m_DOF_Distance)
	AUTOVAR_PACK_VAR(m_DOF_Amount)
	AUTOVAR_PACK_VAR(m_MotionBlur_Amount)
	AUTOVAR_PACK_VAR(m_MotionBlur_Fuzzyness)
	AUTOVAR_PACK_VAR(m_GlowMotionBlur)
	AUTOVAR_PACK_VAR(m_iDarkness_Drain_1)
	AUTOVAR_PACK_VAR(m_DemonHeadTargetFade)
	AUTOVAR_PACK_VAR(m_DemonHeadCurrentFade)
	AUTOVAR_PACK_VAR(m_SpawnTick)
	AUTOVAR_PACK_VAR(m_DeathTick)
	AUTOVAR_PACK_VAR(m_WeaponUnequipTick)
	AUTOVAR_PACK_VAR(m_DarklingSpawn)
	AUTOVAR_PACK_VAR(m_DarklingSpawnRenderChoices)
	AUTOVAR_PACK_VAR(m_DarklingSpawnRenderChoicesStartTick)
	AUTOVAR_PACK_VAR(m_DestroyTick)
	AUTOVAR_PACK_VAR(m_Cutscene_Camera)
	AUTOVAR_PACK_VAR(m_RenderAttached)
	AUTOVAR_PACK_VAR(m_iAttachJoint)
	AUTOVAR_PACK_VAR(m_Anim_IdleStance)
	AUTOVAR_PACK_VAR(m_Phys_Flags)
	AUTOVAR_PACK_VAR(m_iFlagIndex)
	AUTOVAR_PACK_VAR(m_MuzzleFlashTick0)
	AUTOVAR_PACK_VAR(m_MuzzleFlashTick1)
	AUTOVAR_PACK_VAR(m_LightningLightTick0)
	AUTOVAR_PACK_VAR(m_LightningLightTick1)
	AUTOVAR_PACK_VAR(m_LightningLight)
#ifdef INCLUDE_OLD_RAGDOLL
	AUTOVAR_PACK_VAR(m_RagdollClient)
#endif // INCLUDE_OLD_RAGDOLL
	AUTOVAR_PACK_VAR(m_RagdollClientData)
	AUTOVAR_PACK_VAR(m_RagdollClientMetaData)
	AUTOVAR_PACK_VAR(m_WeaponHUDStartTick)

/*	AUTOVAR_PACK_VAR(m_Ammo_Surface1)
	AUTOVAR_PACK_VAR(m_Ammo_Surface2)
	AUTOVAR_PACK_VAR(m_Ammo_NumTotal)
	AUTOVAR_PACK_VAR(m_Ammo_NumLoaded)
	AUTOVAR_PACK_VAR(m_Ammo_Extra)*/
	AUTOVAR_PACK_VAR(m_Pickup_iIcon)
	AUTOVAR_PACK_VAR(m_Pickup_StartTick)
	AUTOVAR_PACK_VAR(m_Pickup_Magazine_Num)
	//AUTOVAR_PACK_VAR(m_Inventory)
	//AUTOVAR_PACK_VAR(m_PhoneBook)
/*	AUTOVAR_PACK_VAR(m_Pickup_iQuestIcon0)
	AUTOVAR_PACK_VAR(m_Pickup_iQuestIcon1)
	AUTOVAR_PACK_VAR(m_Pickup_iQuestIcon2)
	AUTOVAR_PACK_VAR(m_Pickup_iQuestIcon3)
	AUTOVAR_PACK_VAR(m_Pickup_iQuestIcon4)
	AUTOVAR_PACK_VAR(m_Pickup_iQuestIcon5)
	AUTOVAR_PACK_VAR(m_Pickup_iQuestCount0)
	AUTOVAR_PACK_VAR(m_Item_Name0)
	AUTOVAR_PACK_VAR(m_Item_Name1)
	AUTOVAR_PACK_VAR(m_Item_Name2)
	AUTOVAR_PACK_VAR(m_Item_Name3)
	AUTOVAR_PACK_VAR(m_Item_Name4)
	AUTOVAR_PACK_VAR(m_Item_Name5)
	AUTOVAR_PACK_VAR(m_Item_Desc0)
	AUTOVAR_PACK_VAR(m_Item_Desc1)
	AUTOVAR_PACK_VAR(m_Item_Desc2)
	AUTOVAR_PACK_VAR(m_Item_Desc3)
	AUTOVAR_PACK_VAR(m_Item_Desc4)
	AUTOVAR_PACK_VAR(m_Item_Desc5)*/
	AUTOVAR_PACK_VAR(m_HitEffect_Flags)
	AUTOVAR_PACK_VAR(m_HitEffect_iObject)
	AUTOVAR_PACK_VAR(m_HitEffect_Duration)
	AUTOVAR_PACK_VAR(m_HeadGibbed)
	AUTOVAR_PACK_VAR(m_Item0_Num)
	AUTOVAR_PACK_VAR(m_Item1_Num)
	AUTOVAR_PACK_VAR(m_ForcedAimingMode)
	AUTOVAR_PACK_VAR(m_Burnable)
	AUTOVAR_PACK_VAR(m_Item_ModelFlags)
	AUTOVAR_PACK_VAR(m_SelectItemTimeOutTick)
	AUTOVAR_PACK_VAR(m_iCurSelectedItem)
	AUTOVAR_PACK_VAR(m_iNextValidSelectItem)
	AUTOVAR_PACK_VAR(m_iCurEquippedItem)
	AUTOVAR_PACK_VAR(m_Item0_Model)
	AUTOVAR_PACK_VAR(m_Item0_Flags)
	AUTOVAR_PACK_VAR(m_Item0_AnimSound0)
	AUTOVAR_PACK_VAR(m_Item0_AnimSound1)
	AUTOVAR_PACK_VAR(m_Item0_AnimSound2)
	AUTOVAR_PACK_VAR(m_Item0_AnimSound3)
	AUTOVAR_PACK_VAR(m_Item0_Icon)
	AUTOVAR_PACK_VAR(m_Item1_Model)
	AUTOVAR_PACK_VAR(m_Item1_Flags)
	AUTOVAR_PACK_VAR(m_Item1_AnimSound0)
	AUTOVAR_PACK_VAR(m_Item1_AnimSound1)
	AUTOVAR_PACK_VAR(m_Item1_AnimSound2)
	AUTOVAR_PACK_VAR(m_Item1_AnimSound3)
	AUTOVAR_PACK_VAR(m_Item1_Icon)
	AUTOVAR_PACK_VAR(m_AnimSound5)
	AUTOVAR_PACK_VAR(m_Item2_Model)
	AUTOVAR_PACK_VAR(m_nUDMoney)
	AUTOVAR_PACK_VAR(m_nMoth)
	AUTOVAR_PACK_VAR(m_liDialogueChoice)
	AUTOVAR_PACK_VAR(m_DialogueChoiceTick)
	AUTOVAR_PACK_VAR(m_ControlMode_Param0)
	AUTOVAR_PACK_VAR(m_ControlMode_Param1)
	AUTOVAR_PACK_VAR(m_ControlMode_Param2)
	AUTOVAR_PACK_VAR(m_ControlMode_Param3)
	AUTOVAR_PACK_VAR(m_ControlMode_Param4)
	AUTOVAR_PACK_VAR(m_Tension)
	AUTOVAR_PACK_VAR(m_LastTeleportTick)
	AUTOVAR_PACK_VAR(m_TargetInfo)
	AUTOVAR_PACK_VAR(m_TargetAimPos)
	AUTOVAR_PACK_VAR(m_CutsceneFOV)
	AUTOVAR_PACK_VAR(m_WeaponZoomState)
	AUTOVAR_PACK_VAR(m_WeaponZoomStartTick)
	AUTOVAR_PACK_VAR(m_Zoom)
	AUTOVAR_PACK_VAR(m_FlashLightColor)
	AUTOVAR_PACK_VAR(m_FlashLightRange)
	AUTOVAR_PACK_VAR(m_MuzzleLightColor0)
	AUTOVAR_PACK_VAR(m_MuzzleLightRange0)
	AUTOVAR_PACK_VAR(m_MuzzleLightColor1)
	AUTOVAR_PACK_VAR(m_MuzzleLightRange1)
	AUTOVAR_PACK_VAR(m_ElectroShockTick)
	AUTOVAR_PACK_VAR(m_CrosshairInfo)
	AUTOVAR_PACK_VAR(m_ShieldStart)
	AUTOVAR_PACK_VAR(m_FreeAimedTargetSurfaceType)
	AUTOVAR_PACK_VAR(m_MountedMode)
	AUTOVAR_PACK_VAR(m_iMountedObject)
	AUTOVAR_PACK_VAR(m_MountedStartTick)
	AUTOVAR_PACK_VAR(m_Phys_DeadRadius)
	AUTOVAR_PACK_VAR(m_Phys_Width)
	AUTOVAR_PACK_VAR(m_Phys_Height)
	AUTOVAR_PACK_VAR(m_Phys_HeightCrouch)
	AUTOVAR_PACK_VAR(m_Phys_SpectatorSize)
	AUTOVAR_PACK_VAR(m_Speed_WalkForward)
	AUTOVAR_PACK_VAR(m_Speed_Forward)
	AUTOVAR_PACK_VAR(m_Speed_SideStep)
	AUTOVAR_PACK_VAR(m_Speed_Up)
	AUTOVAR_PACK_VAR(m_Speed_Jump)
	AUTOVAR_PACK_VAR(m_iAnimList)
	AUTOVAR_PACK_VAR(m_iPlayer)
	AUTOVAR_PACK_VAR(m_Character)
	AUTOVAR_PACK_VAR(m_AnimOOSBase)
	AUTOVAR_PACK_VAR(m_BackPlane)
	AUTOVAR_PACK_VAR(m_iDarkness_Tentacles)
	AUTOVAR_PACK_VAR(m_StraightenUpTick)
	
	AUTOVAR_PACK_VAR(m_Camera_StandHeadOffset)
	AUTOVAR_PACK_VAR(m_Camera_CrouchHeadOffset)
	AUTOVAR_PACK_VAR(m_Camera_BehindOffset)
	AUTOVAR_PACK_VAR(m_Camera_ShoulderOffset)

	AUTOVAR_PACK_VAR(m_Map_iSurface)
	AUTOVAR_PACK_VAR(m_Map_PosTop)
	AUTOVAR_PACK_VAR(m_Map_PosLeft)
	AUTOVAR_PACK_VAR(m_Map_PosBottom)
	AUTOVAR_PACK_VAR(m_Map_PosRight)
	AUTOVAR_PACK_VAR(m_Overviewmap)
	AUTOVAR_PACK_VAR(m_Overviewmap_Visited)
	AUTOVAR_PACK_VAR(m_Overviewmap_Current)

	// EXPIRED: Special Rotation
/*	AUTOVAR_PACK_VAR(m_Camera_SpecialOffset)
	AUTOVAR_PACK_VAR(m_Camera_SpecialRotation)
*/

//	AUTOVAR_PACK_VAR(m_Camera_FaceOffset)

	AUTOVAR_PACK_VAR(m_Camera_FadeOffset)

	AUTOVAR_PACK_VAR(m_CameraShake_Speed)
	AUTOVAR_PACK_VAR(m_CameraShake_Amplitude)
	AUTOVAR_PACK_VAR(m_CameraShake_DurationTicks)
	AUTOVAR_PACK_VAR(m_CameraShake_StartTick)

	// Dialogue
	AUTOVAR_PACK_VAR(m_iDialogueTargetObject)
	AUTOVAR_PACK_VAR(m_DialoguePlayerPos)

	// FIXME: MEMOPT
	// These can be removed and rewriten to NOMOVE, FORCEWALK, etc.
	AUTOVAR_PACK_VAR(m_ControlMove_Offset)
	AUTOVAR_PACK_VAR(m_ControlMove_Scale)

	AUTOVAR_PACK_VAR(m_ExtraFlags)
	AUTOVAR_PACK_VAR(m_Analog0)
	AUTOVAR_PACK_VAR(m_Analog1)
	AUTOVAR_PACK_VAR(m_Analog2)
	AUTOVAR_PACK_VAR(m_Analog3)
	AUTOVAR_PACK_VAR(m_AnalogMode)
	AUTOVAR_PACK_VAR(m_WeaponStatus0)
	AUTOVAR_PACK_VAR(m_WeaponStatus1)

	AUTOVAR_PACK_VAR(m_bForceLookOnTarget)
	AUTOVAR_PACK_VAR(m_bForceLook)

	AUTOVAR_PACK_VAR(m_HudTargetID)
	AUTOVAR_PACK_VAR(m_TargetHudState)

	// FIXME: MEMOPT
	// ZoomSpeed is not used anymore.
	AUTOVAR_PACK_VAR(m_WantedZoomSpeed)

	AUTOVAR_PACK_VAR(m_MaxZoom)

	// FIXME: MEMOPT
	// ExtraSurface functionality not yet implemented, so this may be removed if feature will be left out.
	AUTOVAR_PACK_VAR(m_ExtraSurfaceTime)

	AUTOVAR_PACK_VAR(m_LiquidTickCountDown)
	AUTOVAR_PACK_VAR(m_Anim_ClimbTickCounter)
	AUTOVAR_PACK_VAR(m_TicksInLiquid)
	AUTOVAR_PACK_VAR(m_SwimHeight)	
	/*AUTOVAR_PACK_VAR(m_PreviousHeadMediumType)		
	AUTOVAR_PACK_VAR(m_HeadMediumType)
	AUTOVAR_PACK_VAR(m_PreviousFeetMediumType)		
	AUTOVAR_PACK_VAR(m_FeetMediumType)*/
	AUTOVAR_PACK_VAR(m_Anim_FreezeTick)
	AUTOVAR_PACK_VAR(m_MoveDir)
	AUTOVAR_PACK_VAR(m_EyeLookDir)
	AUTOVAR_PACK_VAR(m_iEyeLookObj)

	// Who we are currently fighting with
	AUTOVAR_PACK_VAR(m_iFightingCharacter)
	AUTOVAR_PACK_VAR(m_FightPosTimeScale)
	AUTOVAR_PACK_VAR(m_FightPosTime)
	AUTOVAR_PACK_VAR(m_TargetFov)
	AUTOVAR_PACK_VAR(m_BaseFov)
	AUTOVAR_PACK_VAR(m_FovTimeScale)
	AUTOVAR_PACK_VAR(m_FovTime)
	AUTOVAR_PACK_VAR(m_FovOverrideStartTick)
	AUTOVAR_PACK_VAR(m_FovOverride)
	AUTOVAR_PACK_VAR(m_FovOverridePrevValue)
	AUTOVAR_PACK_VAR(m_FovOverrideBlendTime)
	AUTOVAR_PACK_VAR(m_HealthHudStart)
	AUTOVAR_PACK_VAR(m_FightModeFlag)
	AUTOVAR_PACK_VAR(m_CharSkeletonScale)
	AUTOVAR_PACK_VAR(m_CharGlobalScale)
	//AUTOVAR_PACK_VAR(m_FCharMovement)
	AUTOVAR_PACK_VAR(m_RelAnimPos)

	AUTOVAR_PACK_VAR(m_LookSpeedDamper)

	AUTOVAR_PACK_VAR(m_ActionCutSceneCameraAngle)
	AUTOVAR_PACK_VAR(m_iActionCutSceneCameraObject)
	AUTOVAR_PACK_VAR(m_iActionCutSceneCameraTarget)
	AUTOVAR_PACK_VAR(m_ActionCutSceneCameraMode)
	AUTOVAR_PACK_VAR(m_ActionCutSceneCameraWantedDistance)
	AUTOVAR_PACK_VAR(m_ActionCutSceneCameraOffsetX)
	AUTOVAR_PACK_VAR(m_ActionCutSceneCameraOffsetY)
	
	AUTOVAR_PACK_VAR(m_TurnCorrectionLastAngle)
	AUTOVAR_PACK_VAR(m_TurnCorrectionTargetAngleVert)
	AUTOVAR_PACK_VAR(m_TurnCorrectionTargetAngle)
//	AUTOVAR_PACK_VAR(m_LastBodyAngleZ)
//	AUTOVAR_PACK_VAR(m_TargetBodyAngleZ)
	//AUTOVAR_PACK_VAR(m_TurnCorrectionAngleChange)

	AUTOVAR_PACK_VAR(m_ActionCutSceneCameraDistanceOffset)
	AUTOVAR_PACK_VAR(m_ActionCutSceneCameraHeightOffset)

	AUTOVAR_PACK_VAR(m_FocusFrameType)
	AUTOVAR_PACK_VAR(m_FocusFrameOffset)
	AUTOVAR_PACK_VAR(m_SpecialGrab)
	AUTOVAR_PACK_VAR(m_Disable)
	AUTOVAR_PACK_VAR(m_iFocusFrameObject)	
	AUTOVAR_PACK_VAR(m_EquippedItemClass)
	AUTOVAR_PACK_VAR(m_EquippedItemType)
	AUTOVAR_PACK_VAR(m_AnimGripTypeRight)
	AUTOVAR_PACK_VAR(m_AnimGripTypeLeft)
	AUTOVAR_PACK_VAR(m_iCloseObject)
	AUTOVAR_PACK_VAR(m_FocusFrameUseText)
	AUTOVAR_PACK_VAR(m_FocusFrameDescText)

	AUTOVAR_PACK_VAR(m_PagerStartTick)
	AUTOVAR_PACK_VAR(m_PagerPhoneNumber)

	AUTOVAR_PACK_VAR(m_LeanStance)

	AUTOVAR_PACK_VAR(m_bNightvisionEnabled)
	
	AUTOVAR_PACK_VAR(m_SneakLevel)
	AUTOVAR_PACK_VAR(m_CrouchLevel)
	AUTOVAR_PACK_VAR(m_SoundLevel)
	AUTOVAR_PACK_VAR(m_Opacity)
	AUTOVAR_PACK_VAR(m_DepthFog)
	AUTOVAR_PACK_VAR(m_iLaserBeam)
	AUTOVAR_PACK_VAR(m_iLightCone)
	AUTOVAR_PACK_VAR(m_iSurveillanceCamera)

	AUTOVAR_PACK_VAR(m_AnimLipSync)
	AUTOVAR_PACK_VAR(m_Aim_SkeletonType)
	AUTOVAR_PACK_VAR(m_lAnim_VoCap)

	AUTOVAR_PACK_VAR(m_CrosshairTime)
	AUTOVAR_PACK_VAR(m_CrosshairAmount)

	AUTOVAR_PACK_VAR(m_AimTargetStartTick)
	AUTOVAR_PACK_VAR(m_AimReleasingTarget)

	AUTOVAR_PACK_VAR(m_Creep_Control_Look_Wanted)
	AUTOVAR_PACK_VAR(m_Creep_Orientation)
	AUTOVAR_PACK_VAR(m_Creep_PrevOrientation)

	AUTOVAR_PACK_VAR(m_AimTarget)
	AUTOVAR_PACK_VAR(m_AutoAimVal)
	AUTOVAR_PACK_VAR(m_ThisAimHeight)

#ifdef WADDITIONALCAMERACONTROL
	AUTOVAR_PACK_VAR(m_bAdditionalCameraControl)
	AUTOVAR_PACK_VAR(m_AdditionalCameraControlY)
	AUTOVAR_PACK_VAR(m_AdditionalCameraControlZ)
	AUTOVAR_PACK_VAR(m_AdditionalCameraPos)
	AUTOVAR_PACK_VAR(m_Control_MoveAdditional)
#endif
//	AUTOVAR_PACK_VAR(m_ThisAimOffset)

	AUTOVAR_PACK_VAR(m_3PI_CameraSlide)
	AUTOVAR_PACK_VAR(m_DarknessVoiceUse)
	AUTOVAR_PACK_VAR(m_3PI_Mode)
	AUTOVAR_PACK_VAR(m_3PI_NoCamera)
	AUTOVAR_PACK_VAR(m_3PI_FocusTicks)
	AUTOVAR_PACK_VAR(m_3PI_FocusObject)
	AUTOVAR_PACK_VAR(m_3PI_LightState)
	AUTOVAR_PACK_VAR(m_3PI_LightFadeStart)
	AUTOVAR_PACK_VAR(m_CameraUtil)

	AUTOVAR_PACK_VAR(m_CharName)

#ifdef WDEBUGCONTROL_ENABLED
	AUTOVAR_PACK_VAR(m_DebugControlMoveX)
	AUTOVAR_PACK_VAR(m_DebugControlMoveY)
	AUTOVAR_PACK_VAR(m_DebugControlLookH)
	AUTOVAR_PACK_VAR(m_DebugControlLookV)
	AUTOVAR_PACK_VAR(m_DebugControlDLookH)
	AUTOVAR_PACK_VAR(m_DebugControlDLookV)
	AUTOVAR_PACK_VAR(m_DebugControlPress)
	AUTOVAR_PACK_VAR(m_iDebugControlObject)
#endif

	AUTOVAR_PACK_VAR(m_WindForce)
	AUTOVAR_PACK_VAR(m_iFacialSetup)
	AUTOVAR_PACK_END

	virtual void Copy(const CWO_Player_ClientData& _CD);
	virtual CWObject_Client* CreateClientObject(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	virtual void CopyClientObject(CWObject_Client* _pObj, CWObject_Client* _pDest);

	virtual int OnClientPredictControlFrame(CWorld_Client* _pWClient, CWObject_Client* pPredict, CControlFrame& _Ctrl, bool _bFullFrame, bool _bLog);
	virtual int OnClientCheckPredictionMisses(CWorld_Client* _pWClient, CWObject_Client* _pObj, CWObject_Client* _pPredict, bool _bLog = false);

	void AddTilt(const CVec3Dfp32& _Axis, fp32 _Angle);
	void ModerateTilt(fp32 _dTime);
	void Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	CWO_Character_ClientData();

	/*

	Wrapper functions for modifying m_TargetInfo
	bits 0..15 contain target object index.
	bits 16..23 contain current accuracy.
	bits 24..31 are unused. (use for testing stuff)

	*/

	void SetItemModelFlag(int _iItem, int _iModel, int _Flag)
	{
		uint8 shift = (_iModel & 3) + ((_iItem & 1) * 4);
		uint8 mask = 1 << shift;
		uint8 flag = _Flag << shift;
		m_Item_ModelFlags = (m_Item_ModelFlags & ~mask) | (flag & mask);
	}

	int GetItemModelFlag(int _iItem, int _iModel)
	{
		uint8 shift = (_iModel & 3) + ((_iItem & 1) * 4);
		uint8 flag = ((m_Item_ModelFlags) >> shift) & 1;
		return flag;
	}

	uint16 Target_GetObject() { return m_TargetInfo & 0xffff; };
	//uint8 Target_GetAccuracy() { return (m_TargetInfo >> 16) & 0xff; };
	void Target_SetObject(int _iObject) { m_TargetInfo = (m_TargetInfo & 0xffff0000) | (_iObject & 0xffff); };
	//void Target_SetAccuracy(int _Accuracy) { m_TargetInfo = (m_TargetInfo & 0xff00ffff) | ((_Accuracy & 0xff) << 16); };

	uint8 Target_GetTestStuff() { return (m_TargetInfo >> 24) & 0xff; };
	void Target_SetTestStuff(int _Val) { m_TargetInfo = (m_TargetInfo & 0x00ffffff) | ((_Val & 0xff) << 24); };
	
	/*

	Aim vectors are stored using polar coordinates, packed into a 32bit integer, each 16-bit word contain an
	angle [0..0x10000] that correspond to [0..1] (or [0..2*PI]) All floating point angles here are in [0..1] range, as usual.

	*/
	CQuatfp32 Target_GetQuatForAngles(const CVec2Dfp32& _Angles) const;		// Convert polar coordinates to quaternion.
	CVec2Dfp32 Target_GetAnglesForQuat(const CQuatfp32& _Quat) const;			// Convert quaternion to polar coordinates.
	static CVec2Dfp32 Target_GetAnglesForVector(const CVec3Dfp32& _Vec);		// Convert vector to polar coordinates.
	static CVec3Dfp32 Target_GetVectorForAngles(const CVec2Dfp32& _Angles);	// Convert polar coordinates to vector.

	void Target_SetAimAngles(fp32 _XAngle, fp32 _YAngle);						// Set aim using polar coordinates.
	void Target_SetAimAnglesVel(fp32 _dXAngle, fp32 _dYAngle);				// Set aim velocity using polar coordinates.
	void Target_GetAimAngles(fp32& _XAngle, fp32& _YAngle) const;				// Get aim in polar coordinates.
	void Target_GetAimAnglesVel(fp32& _dXAngle, fp32& _dYAngle) const;		// Get aim velocity in polar coordinates.
	void Target_SetAimAngles(const CVec2Dfp32& _Angles) { Target_SetAimAngles(_Angles[0], _Angles[1]); };			// Set aim using polar coordinates.
	void Target_SetAimAnglesVel(const CVec2Dfp32& _dAngles) { Target_SetAimAnglesVel(_dAngles[0], _dAngles[1]); };	// Set aim velocity using polar coordinates.
	CVec2Dfp32 Target_GetAimAngles() const { CVec2Dfp32 v; Target_GetAimAngles(v[0], v[1]); return v; };				// Get aim in polar coordinates.
	CVec2Dfp32 Target_GetAimAnglesVel() const { CVec2Dfp32 v; Target_GetAimAnglesVel(v[0], v[1]); return v; };		// Get aim velocity in polar coordinates.

	void Target_AddAimAngles(const CVec2Dfp32&);								// Add to aim using polar coordinates.
	void Target_AddAimAnglesVel(const CVec2Dfp32&);							// Add to aim velocity using polar coordinates.

	void Target_SetAimVector(const CVec3Dfp32& _v);							// Set aim using vector.
	CVec3Dfp32 Target_GetAimVector() const;									// Get aim as a vector.
	void Target_GetAimMatrix(const CVec2Dfp32&, CMat4Dfp32&) const;			// Convert polar coordinates to rotation matrix.
	void Target_GetAimMatrix(CMat4Dfp32&) const;								// Get aim as a rotation matrix.

	static CVec2Dfp32 Target_WrapAngles(const CVec2Dfp32& _ATarget, const CVec2Dfp32& _ACurrent);	// Angle wrapper for interpolation purpose.
	static CVec2Dfp32 Target_LerpAngles(const CVec2Dfp32& _From, const CVec2Dfp32& _To, fp32 _t);	// Interpolate polar coordinates, taking wrapping issues into account.

	void SetEyeLook(const CVec3Dfp32& _EyeLook, const int16 _iObj) { m_EyeLookDir = _EyeLook; m_iEyeLookObj = _iObj; }

	// AGMERGE
	//virtual void AG_RefreshGlobalProperties(const CWAGI_Context* _pContext);
	virtual void AG2_RefreshGlobalProperties(const CWAG2I_Context* _pContext);

	// AGMERGE
	void OnRefresh(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, const CMTime& _GameTime);
	void OnClientRefresh(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, const CMTime& _GameTime);

	virtual void Char_UpdateLook(fp32 _FrameFrac);
	virtual void Char_ProcessControl_Look(const CVec3Dfp32& _dLook);
	virtual void Char_ProcessControl_Move(const CVec3Dfp32& _Move);
	virtual void Phys_Move(const CSelection& _Selection, fp32 _dTime, const CVec3Dfp32& _UserVel, bool _bPredicted);

	void DampDLookForDialogueCameras(CVec3Dfp32 &_dLook);
	void UpdateButtonPressTicks();
	void ResetButtonPressTicks(uint _ControlBits);

	virtual CWO_CharDarkling_ClientData* IsDarkling() { return NULL; }
	virtual CWO_CharNPC_ClientData* IsNPC() { return NULL; }
};

#endif // _INC_WOBJ_CHARCLIENTDATA

