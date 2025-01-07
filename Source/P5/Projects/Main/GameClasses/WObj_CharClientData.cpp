#include "PCH.h"

#include "WObj_Char.h"
#include "../GameWorld/WClientMod_Defines.h"
#include "WObj_AI/AI_DeviceHandler.h"
#include "WObj_Misc/WObj_CreepingDark.h"
#include "WObj_Misc/WObj_ActionCutscenecamera.h"
#include "WObj_Misc/WObj_Turret.h"
#include "WObj_Char/WObj_CharDarkling_ClientData.h"


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Character client data					
\*____________________________________________________________________________________________*/

MRTC_IMPLEMENT(CWO_Character_ClientData, CWO_Player_ClientData);

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Character_ClientData
|__________________________________________________________________________________________________
\*************************************************************************************************/

CWO_Character_ClientData::CWO_Character_ClientData()
{
	MAUTOSTRIP(CWO_Character_ClientData_ctor, MAUTOSTRIP_VOID);

	m_AnimGraph2.m_pClientData = this;
	m_AnimGraph2.GetAG2I()->SetEvaluator(&m_AnimGraph2);
	m_WeaponAG2.m_pClientData = this;
	m_WeaponAG2.GetAG2I()->SetEvaluator(&m_WeaponAG2);
	INVALIDATE_POS(m_EyeLookDir);

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	m_AutoAimVal = pSys->GetOptions()->GetValuef("CONTROLLER_AUTOAIM", 0.5f);
}


void CWO_Character_ClientData::Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	MAUTOSTRIP(CWO_Character_ClientData_Clear, MAUTOSTRIP_VOID);
	CWO_Player_ClientData::Clear(_pObj);
	m_pWPhysState = _pWPhysState;

	m_AnimGraph2.Clear();
	m_WeaponAG2.Clear();
	m_CameraUtil.Clear();
	m_lCharacterCloth.SetLen(0);

	for(int i = 0; i < PLAYER_NUMANIMCACHESLOTS; i++)
	{
		m_lLastSkeletonInstanceTick[i] = -1;
		m_lLastSkeletonInstanceIPTime[i] = -1;
		m_lLastSkeletonInstanceSkel[i] = NULL;
	}

	m_PhysSelection=NULL;
	m_Phys_IdleTicks = 0;

	m_3PI_LightState = THIRDPERSONINTERACTIVE_LIGHT_STATE_OFF;
	m_3PI_LightFadeStart = 0;
	m_3PI_NoCamera = 0;

	m_PredMiss_Tick = 0;
	m_PredMiss_dPos = 0;

	m_AGRefreshLastTick = 0;
	m_AGRefreshLastFrac = 0.0f;

	m_Camera_LastSpeedPercent = 0;
	for (int iModel = 0; iModel < CWO_NUMMODELINDICES; iModel++)
		m_lspClientData[iModel] = NULL;

//	m_BloodColor = 0xFFC02020; // Added by Mondelore

	m_ExtraFlags = 0; // Added by Mondelore

	m_ExtraSurfaceIndex = 0;
	m_ExtraSurfaceTime = 0.0f;

	m_DarknessBarFlashTick = 0;
	m_DarknessBarFlashColor = 0;
	m_Darkness = 0;
	m_MaxDarkness = 0;
	m_DarknessSelectionMode = 0;
	m_DarknessPowersAvailable = 0;
	m_GUITaggedNewDarknessPower = 0;
	m_PannLampa = 0;
	m_iCreepingDark = 0;
	m_DarknessVisibility = 0;

	m_DemonHeadTargetFade = -1;
	m_DemonHeadCurrentFade = 0.0f;
	
	m_HitEffect_Duration = 0;
	m_HitEffect_Flags = 0;
	m_HitEffect_LastTime = 0;
	m_HitEffect_Time = 0;

	m_DarklingSpawnRenderChoices = 0;
	m_DarklingSpawnRenderChoicesStartTick = 0;

	m_DarknessPowersAvailable = PLAYER_DARKNESSMODE_COREPOWERMASK;
	m_DarknessPowers = m_DarknessPowersAvailable;

	m_iFlagIndex = -1;
	m_ShieldStart = 2;
	m_SpawnTick = -1;
	m_DeathTick = -1;
	m_iDarkness_Tentacles = 0;
	m_Control_Press = 0;
	m_Control_LastPress = 0;
	m_Control_Released = ~0;
	m_Control_Hold = 0;
	m_Control_LastHold = 0;
	m_Control_Click	= 0;
	m_Control_Press_Intermediate = 0;
	m_Control_Press_LastPressAGRefresh = 0;
	m_Control_AutoTiltCountDown = 0;
	m_Control_IdleTicks = 0;
	memset(m_Control_PressTicks, 0, sizeof(m_Control_PressTicks));

	m_bWasSwappedSpeaker = false;
	m_DialogCamLook = 0;
#ifdef WADDITIONALCAMERACONTROL
	m_Control_PressAdditional = 0;
	m_Control_LastPressAdditional = 0;
	m_Control_ReleasedAdditional = -1;
#endif

	m_Phys_bInAir = 0;
	m_Phys_nInAirFrames = 0;
	m_Phys_iLastGroundObject = 0;

	m_Control_Move = 0;
	m_Control_Look = 0;
	m_Control_Look_Wanted = 0; // Added by Mondelore.
	m_Control_DeltaLook = 0;
	m_Creep_Control_Look_Wanted = 0;
	m_Creep_Orientation.m_Value.Unit();
	m_Creep_PrevOrientation.m_Value.Unit();

	m_ForcedTilt = 0; // Added by Mondelore.
	m_LastForcedTilt = 0; // Added by Mondelore.
	m_ForcedTiltBlend = 0; // Added by Mondelore.
	m_LastForcedTiltBlend = 0; // Added by Mondelore.
	m_ForcedTiltBlendChange = 0; // Added by Mondelore.

	m_bForceLookOnTarget = 0;
	m_ForcedLook = 0;
	m_LastForcedLook = 0;
	m_ForcedLookChange = 0;
	m_ForcedLookBlend = 0;
	m_LastForcedLookBlend = 0;
	m_ForcedLookBlendChange = 0;

	m_PainSoundDelay = 0;

	m_SpawnDuration = 0;
	m_iSpawnAnim = 0;
	m_iSpawnSound = 0;
	m_LastMove = 0;

	m_Camera_CharacterOffset = 0;
	m_CameraShake_Flags = 0;
	m_CameraShake_CurBlend = 0;
	m_CameraShake_LastBlend = 0;
	m_CameraShake_CurMomentum = 0;
	m_CameraShake_LastMomentum = 0;
	m_CameraShake_UpdateDelayTicks = 0;
	m_CameraShake_Randseed = 0;

	m_CameraShake_Amplitude = 0;
	m_CameraShake_Speed = 0;
	m_CameraShake_DurationTicks = 0;
	m_CameraShake_StartTick = 0;

	m_DestroyTick = 0;

	m_ControlMove_Offset = 0; // Added by Mondelore.
	m_ControlMove_Scale = 1; // Added by Mondelore.

	m_bCreepingDarkMoveBlock = 0;

	m_Cam_Valid = false;
	m_Cam_LastPos = 0;
	m_Cam_Pos = 0;
	m_Cam_dPos = 0;
	m_Cam_Rot.Unit();
	m_Cam_dRot.Unit();	
	m_Cam_BobValid = false;
	m_Cam_LastBob.Unit();
	m_Cam_BobMove = 0;
	m_Cam_LastBobMove = 0;
	m_Cam_BobWeap = 0;
	m_Cam_LastBobWeap = 0;
	m_Cam_dKneel = 0;
	m_Cam_Kneel = 0;

	m_LastStepUpDamper = 0;
	m_StepUpDamper = 0;
	m_StepUpDamperPrim = 0;

	m_ChaseCamDistance = 0;
	m_dChaseCamDistance = 0;
	m_LastChaseCamDistance = 0;

	m_FocusFramePosX = 0;
	m_FocusFramePosY = 0;

	m_Cam_Tilt.Unit();
	m_Cam_TiltAnglePrim = 0;
	m_Cam_TiltTime = 0;

	m_Anim_LastLook.Unit();
	m_Anim_Look.Unit();
	m_Anim_BodyAngleZ = 0;
	m_Anim_LastBodyAngleZ = 0;
	m_Anim_LastBodyAngleY = 0;
	m_Anim_LastBodyAngleX = 0;


	m_Anim_ModeratedMove = 0;
	m_Anim_ModeratedMovePrim = 0;

	m_AnimStance_Sequence = 0;
	m_AnimStance_LastKeyframeTime = 0;
	m_AnimStance_LastKeyframeSeq = -1;

	m_AnimCamBlendInStart = 0;
	m_AnimCamBlendOutStart = 0;

	m_Camera_StandHeadOffset	= CVec3Dfp32(0, 0, 64);
	m_Camera_CrouchHeadOffset	= CVec3Dfp32(0, 0, 32);
	m_Camera_BehindOffset		= CVec3Dfp32(0, -20, -80);
	m_Camera_ShoulderOffset		= CVec3Dfp32(10, -10, 0);
	m_Camera_FadeOffset			= 50.f;
	m_Camera_CurHeadOffset = m_Camera_StandHeadOffset;
	m_Camera_CurBehindOffset = m_Camera_BehindOffset;
	m_Camera_LastHeadOffset = m_Camera_CurHeadOffset;
	m_Camera_LastBehindOffset = m_Camera_CurBehindOffset;
	m_Camera_CurHeadOffsetChange = 0;
	m_Camera_CurBehindOffsetChange = 0;
	
	m_Camera_LastPosition = 0;

	m_MaxTargetingDistance = CHAR_MAXTARGETINGDISTANCE_DEFAULT;

	m_WantedZoomSpeed = 0.0f;
	m_MaxZoom = 1.0f;
	m_ZoomSpeed = 0.0f;
	m_CurZoom = m_MaxZoom;
	m_LastZoom = m_CurZoom;
	// Added by Mondelore {END}.
	m_BaseFov = 70.0f;
	m_TargetFov = 70.0f;
	m_FovTime = -1.0f;
	m_FightPosTime = -1.0f;
	m_FovTimeScale = 1.0f;
	m_FightPosTimeScale = 1.0f;
	//m_FightModeRadius = 50.0f;
	//m_TargetFightingPosition = 0;

	m_GroundMaterial = 0;
	m_LastFootstepTick = -1;
	m_LastFootStepType = ~0;
	m_WeaponSelectTimeout = 0;

	// Default all object-indexes to 0 if possible, since then we can remove this clear - JA
	m_iFightingCharacter = -1;
	m_iActionCutSceneCameraObject = 0;
	m_iActionCutSceneCameraTarget = 0;

	m_DodgeTime = 0.0f;
	m_LastTargetDodgeTime = 0.0f;
	m_TargetDodgeTime = 0.0f;
	m_TargetDodgeTimeChange = 0.0f;

	// Char skeleton scale difference
	m_CharSkeletonScale = 0.0f;

	// Char global scale
	m_CharGlobalScale = 1.0f;

	m_LedgeClimbOnCounter = 0;
	m_LedgePhysWidthCached = 0;

	m_HealthHudStart = 0;
	m_HealthHudFade = 0;
	m_LastHealthHudFade = 0;
	m_HealthHudFadeChange = 0;
	m_HealthWobble = 0;

	m_ACSLastAngleOffsetX = 0.0f;
	m_ACSTargetAngleOffsetX = 0.0f;
	m_ACSAngleOffsetXChange = 0.0f;

	m_ACSLastAngleOffsetY = 0.0f;
	m_ACSTargetAngleOffsetY = 0.0f;
	m_ACSAngleOffsetYChange = 0.0f;

	m_ACSLastTargetDistance = 0.0f;
	m_ACSTargetDistance = 0.0f;
	m_ACSDistanceChange = 0.0f;

	m_ACSLastTargetRotation = 0.0f;
	m_ACSTargetRotation = 0.0f;
	m_ACSRotationChange = 0.0f;

	m_ACSLastCameraTarget = 0.0f;;
	m_ACSCameraTarget = 0.0f;
	m_ACSCameraTargetChange = 0.0f;

	m_FightLastAngleX = 0.0f;
	m_FightTargetAngleX = 0.0f;
	m_FightAngleXChange = 0.0f;
	m_FightLastAngleY = 0.0f;
	m_FightTargetAngleY = 0.0f;
	m_FightAngleYChange = 0.0f;

	m_TurnCorrectionLastAngle = 0.0f;
	m_TurnCorrectionTargetAngleVert = 0.0f;
	m_TurnCorrectionTargetAngle = 0.0f;
	m_TurnCorrectionAngleChange = 0.0f;

	m_AnimRotVel = 0.0f;

	m_LastDarknessVisibility = 0.0f;
	m_CurrentDarknessVisibility = 0.0f;
	m_DarknessVisibilityChange = 0.0f;

	m_CreepingLens = 0.0f;
	m_CreepingLensLast = 0.0f;

	m_AncientLens = 0.0f;
	m_AncientLensLast = 0.0f;

	m_HeadGibbed = 0;

	/*m_LastBodyAngleZ = 0.0f;
	m_TargetBodyAngleZ = 0.0f;
	m_BodyAngleZChange = 0.0f;*/

	m_Target_LastAimAngles = 0;
	m_Target_AimAnglesChange = 0;
	m_Target_LastObj = 0;

	m_LastRenderTime = 0;
	m_LastRenderPos = 0;

	m_ScreenFlashGameTime.Reset();

#define PHYSSTATE_CONVERTFROM20HZ(x) ((x) * 20.0f * _pWPhysState->GetGameTickTime())
	m_Speed_Forward = PHYSSTATE_CONVERTFROM20HZ(12.0f * 0.50f);
	m_Speed_WalkForward = m_Speed_Forward * 0.5f;
	m_Speed_SideStep = PHYSSTATE_CONVERTFROM20HZ(12.0f * 0.50f);
	m_Speed_Up = PHYSSTATE_CONVERTFROM20HZ(16.0f * 0.50f);
	m_Speed_Jump = PHYSSTATE_CONVERTFROM20HZ(13.0f);
	m_iPlayer = -1;

	m_CutsceneFOV = 90;
	m_WeaponZoomState = WEAPON_ZOOMSTATE_OFF;
	m_CrosshairInfo = 0;

	m_DialogueChoiceTick = -1;

	m_Control_Move_StartTick = 0;
	m_ControlMode_Param0 = 0;
	m_ControlMode_Param1 = 0;
	m_ControlMode_Param2 = 0;
	m_ControlMode_Param3 = 0;
	m_ControlMode_Param4 = 0;

/*	m_HeadMediumType = XW_MEDIUM_AIR;
	m_PreviousHeadMediumType = XW_MEDIUM_AIR;*/

	m_FocusFrameOffset = 64;
	m_SpecialGrab = 0;
	m_Disable = 0;
	m_iFocusFrameObject = -1;
	m_EquippedItemClass = 0;
	m_EquippedItemType = 0;
	m_AnimGripTypeRight = -1;
	m_AnimGripTypeLeft = -1;
	m_iCloseObject = 0;

	// TraceCamera parameters
	m_fTraceHistory = 1.0f;
	m_fOldTraceHistory = 1.0f;
	m_iRotationIndex = 0;

	// Character fade
	m_iWeaponFade = 255;
	m_Camera_Rotation = 0;
	m_DefaultCutsceneLaps = 0;

	m_MountedMode = 0;

	// Dialogue system
	m_DialogueInstance.Reset(CMTime(), 0, 0.0f, 0);
	m_PlayerDialogueToken.Clear();
	m_pCurrentDialogueToken = NULL;
	m_PrevDialogueCameraMatrix.Unit();
	m_DialogueCamera.Init(_pWPhysState->GetMapData());
	m_DialogueCamera.Clear();
	m_iCurChoice = 0;
	
	m_ScrollUpTimer = 0.0f;
	m_ScrollDownTimer = 0.0f;
	m_LastTime = 0.0f;

	m_nChoices = 0;
	m_lUsedChoices.Clear();
	m_lUsedChoices.SetGrow(128);
	m_iLastFocusFrameObject = -1;
	m_Go3rdpOnTick = 0;

	m_WeaponHUDStartTick = 0;
	m_iSurveillanceCamera = 0;

	m_MuzzleLightColor0 = 0xff000000;
	m_MuzzleLightColor1 = 0xff000000;
	m_MuzzleLightRange0 = 0;
	m_MuzzleLightRange1 = 0;

	m_FlashLightColor = 0xffb2d8ff;
	m_FlashLightRange = 512;

	m_LipSync.Clear();
	m_Occlusion = 0;
#ifdef WADDITIONALCAMERACONTROL
	m_bAdditionalCameraControl = 0;
	m_Control_MoveAdditional = 0.0f;
	m_AdditionalCameraControlY = 0.0f;
	m_AdditionalCameraControlZ = 0.0f;
	m_AdditionalCameraPos = 0.0f;
#endif

	m_InventoryInfo.Clear();

	CWAG2I_Context AG2Context(_pObj, _pWPhysState, CMTime());
	m_AnimGraph2.GetAG2I()->SetEvaluator(&m_AnimGraph2);
	m_AnimGraph2.SetAG2I(m_AnimGraph2.GetAG2I());
	m_AnimGraph2.SetInitialProperties(&AG2Context);
	m_WeaponAG2.GetAG2I()->SetEvaluator(&m_WeaponAG2);
	m_WeaponAG2.SetAG2I(m_WeaponAG2.GetAG2I());
#ifndef M_RTM
	//m_AnimGraph2.GetAG2I()->m_bDisableDebug = true;
	m_WeaponAG2.GetAG2I()->m_bDisableDebug = true;
#endif
	m_Item0_Model.SetAG2I(m_WeaponAG2.GetAG2I());
	m_Item1_Model.SetAG2I(m_WeaponAG2.GetAG2I());
	m_Item1_Model.m_iWeaponToken = 1;

	m_iDarkness_Drain_1 = 0;
	//m_DarknessDrainTick = 0;
	m_DarknessDrainTime = 0.0f;
	m_WindForce = 0;
	m_iFacialSetup = 0;

	m_Disorientation_Flash = 0;

	m_PlayAhead_LastCreated = CMTime::CreateFromSeconds(0.0f);
	for (uint i = 0; i < 3; i++)
		m_lhPlayAheadVoices[i] = 0;

	m_DarknessVoiceEffect = 0.0f;
	m_DarknessVoiceUse = 0;

	m_aLegIKIsDirty[0] = false;
	m_aLegIKIsDirty[1] = false;
	m_aLegIKIsDirty[2] = false;
	m_aLegIKIsDirty[3] = false;

	INVALIDATE_POS(m_EyeLookDir);

#if 1
	m_Debug_ShowHud = false;
#endif
}


void CWO_Character_ClientData::Copy(const CWO_Player_ClientData& _CDPlayer)
{
	MAUTOSTRIP(CWO_Character_ClientData_Copy, MAUTOSTRIP_VOID);
	MSCOPE(CWO_Character_ClientData::Copy, CHARACTER);
	CWO_Player_ClientData::Copy(_CDPlayer);

	//JK-FIX: Multiple inheritance junk, forces us to use a dynamic_cast
	const CWO_Character_ClientData* pCD = TDynamicCast<const CWO_Character_ClientData>(&_CDPlayer);
	if (!pCD) 
		Error("Copy", "Object was not of class CWO_Character_ClientData.");

	const CWO_Clientdata_Character_AnimGraph2* pCDAG2 = &pCD->m_AnimGraph2;
	m_AnimGraph2.Copy(*pCDAG2);
	m_AnimGraph2.GetAG2I()->CopyFrom(pCD->m_AnimGraph2.GetAG2I());
	m_AnimGraph2.GetAG2I()->SetEvaluator(&m_AnimGraph2);
	m_AnimGraph2.SetAG2I(m_AnimGraph2.GetAG2I());
	const CWO_Clientdata_WeaponAnim_AnimGraph2AGI* pCDWAG2 = &pCD->m_WeaponAG2;
	m_WeaponAG2.Copy(*pCDWAG2);
	m_WeaponAG2.GetAG2I()->CopyFrom(pCD->m_WeaponAG2.GetAG2I());
	m_WeaponAG2.GetAG2I()->SetEvaluator(&m_WeaponAG2);
	m_WeaponAG2.SetAG2I(m_WeaponAG2.GetAG2I());

	const CWO_Character_ClientData& _CD = *pCD;

	AutoVar_CopyFrom(_CD);
	
	memcpy(&m_HealthHudItem, &_CD.m_HealthHudItem, sizeof(m_HealthHudItem));
	m_InventoryInfo.CopyFrom(_CD.m_InventoryInfo);
	

	for (int iModel = 0; iModel < CWO_NUMMODELINDICES; iModel++)
		m_lspClientData[iModel] = _CD.m_lspClientData[iModel];

	m_iFlagIndex = _CD.m_iFlagIndex;

//	m_BloodColor = _CD.m_BloodColor; // Added by Mondelore.
	
	m_AGRefreshLastTick = _CD.m_AGRefreshLastTick;
	m_AGRefreshLastFrac = _CD.m_AGRefreshLastFrac;

	m_Blink = _CD.m_Blink;

	for(int i = 0; i < PLAYER_NUMANIMCACHESLOTS; i++)
	{
		m_lLastSkeletonInstanceTick[i] = _CD.m_lLastSkeletonInstanceTick[i];
//		m_lLastSkeletonInstanceTick[i] = -1; //_CD.m_lLastSkeletonInstanceTick[i];
//		m_lLastSkeletonInstanceIPTime[i] = _CD.m_lLastSkeletonInstanceIPTime[i];
//		m_lLastSkeletonInstanceSkel[i] = _CD.m_lLastSkeletonInstanceSkel[i];
	}
	if (_CD.m_PhysSelection)
	{
		if (m_PhysSelection==NULL)
			m_PhysSelection=DNew(TSelection<CSelection::LARGE_BUFFER>) TSelection<CSelection::LARGE_BUFFER>();
		CSelection::copy(*m_PhysSelection,*_CD.m_PhysSelection);
	}
	m_Phys_IdleTicks = _CD.m_Phys_IdleTicks;
	m_Control_Move = _CD.m_Control_Move;
	m_Control_Look = _CD.m_Control_Look;
	m_Control_Look_Wanted = _CD.m_Control_Look_Wanted; // Added by Mondelore.
	m_Control_Press = _CD.m_Control_Press;
	m_Control_LastPress = _CD.m_Control_LastPress;
	m_Control_Released = _CD.m_Control_Released;
	m_Control_Hold = _CD.m_Control_Hold;
	m_Control_LastHold = _CD.m_Control_LastHold;
	m_Control_Click = _CD.m_Control_Click;
	m_Control_Press_LastPressAGRefresh = _CD.m_Control_Press_LastPressAGRefresh;
	m_Control_AutoTiltCountDown = _CD.m_Control_AutoTiltCountDown;
	memcpy(m_Control_PressTicks, _CD.m_Control_PressTicks, sizeof(m_Control_PressTicks));

#ifdef WADDITIONALCAMERACONTROL
	m_Control_PressAdditional = _CD.m_Control_PressAdditional;
	m_Control_LastPressAdditional = _CD.m_Control_LastPressAdditional;
	m_Control_ReleasedAdditional = _CD.m_Control_ReleasedAdditional;
#endif

	m_Control_Move_StartTick = _CD.m_Control_Move_StartTick;

	m_ForcedTilt = _CD.m_ForcedTilt; // Added by Mondelore.
	m_LastForcedTilt = _CD.m_LastForcedTilt; // Added by Mondelore.
	m_ForcedTiltBlend = _CD.m_ForcedTiltBlend; // Added by Mondelore.
	m_LastForcedTiltBlend = _CD.m_LastForcedTiltBlend; // Added by Mondelore.
	m_ForcedTiltBlendChange = _CD.m_ForcedTiltBlendChange; // Added by Mondelore.

	m_ForcedLook = _CD.m_ForcedLook;
	m_LastForcedLook = _CD.m_LastForcedLook;
	m_ForcedLookChange = _CD.m_ForcedLookChange;
	m_ForcedLookBlend = _CD.m_ForcedLookBlend;
	m_LastForcedLookBlend = _CD.m_LastForcedLookBlend;
	m_ForcedLookBlendChange = _CD.m_ForcedLookBlendChange;

	m_Phys_bInAir = _CD.m_Phys_bInAir;
	m_Phys_nInAirFrames = _CD.m_Phys_nInAirFrames;
	m_Phys_iLastGroundObject = _CD.m_Phys_iLastGroundObject;

/*
	m_ControlSnapshot_Move = _CD.m_ControlSnapshot_Move;
	m_ControlSnapshot_Look = _CD.m_ControlSnapshot_Look;
	m_ControlSnapshot_Press = _CD.m_ControlSnapshot_Press;
	m_ControlSnapshot_Released = _CD.m_ControlSnapshot_Released;
*/
	m_iDarkness_Tentacles = _CD.m_iDarkness_Tentacles;
	m_bCreepingDarkMoveBlock = _CD.m_bCreepingDarkMoveBlock;
	m_Cam_Valid = _CD.m_Cam_Valid;
	m_Cam_LastPos = _CD.m_Cam_LastPos;
	m_Cam_Pos = _CD.m_Cam_Pos;
	m_Cam_dPos = _CD.m_Cam_dPos;
	m_Cam_LastRot = _CD.m_Cam_LastRot;
	m_Cam_Rot = _CD.m_Cam_Rot;
	m_Cam_dRot = _CD.m_Cam_dRot;
	m_Cam_dKneel = _CD.m_Cam_dKneel;
	m_Cam_Kneel = _CD.m_Cam_Kneel;
	m_Cam_BobValid = _CD.m_Cam_BobValid;
	m_Cam_LastBob = _CD.m_Cam_LastBob;
	m_Cam_Bob = _CD.m_Cam_Bob;
	m_Cam_Tilt = _CD.m_Cam_Tilt;
	m_Cam_TiltAnglePrim = _CD.m_Cam_TiltAnglePrim;
	m_Cam_TiltTime = _CD.m_Cam_TiltTime;
	m_Cam_LastBobMove = _CD.m_Cam_LastBobMove;
	m_Cam_BobMove = _CD.m_Cam_BobMove;
	m_Cam_LastBobWeap = _CD.m_Cam_LastBobWeap;
	m_Cam_BobWeap = _CD.m_Cam_BobWeap;
	m_LastStepUpDamper = _CD.m_LastStepUpDamper;
	m_StepUpDamper = _CD.m_StepUpDamper;
	m_StepUpDamperPrim = _CD.m_StepUpDamperPrim;
	m_ShieldStart = _CD.m_ShieldStart;

	m_HitEffect_Duration = _CD.m_HitEffect_Duration;
	m_HitEffect_Flags = _CD.m_HitEffect_Flags;
	m_HitEffect_LastTime = _CD.m_HitEffect_LastTime;
	m_HitEffect_Time = _CD.m_HitEffect_Time;

	m_DemonHeadTargetFade = _CD.m_DemonHeadTargetFade;
	m_DemonHeadCurrentFade = _CD.m_DemonHeadCurrentFade;

	m_PainSoundDelay = _CD.m_PainSoundDelay;

	m_Camera_CharacterOffset = _CD.m_Camera_CharacterOffset;
	m_CameraShake_Flags = _CD.m_CameraShake_Flags;
	m_CameraShake_CurBlend = _CD.m_CameraShake_CurBlend;
	m_CameraShake_LastBlend = _CD.m_CameraShake_LastBlend;
	m_CameraShake_CurMomentum = _CD.m_CameraShake_CurMomentum;
	m_CameraShake_LastMomentum = _CD.m_CameraShake_LastMomentum;
	m_CameraShake_UpdateDelayTicks = _CD.m_CameraShake_UpdateDelayTicks;
	m_CameraShake_Randseed = _CD.m_CameraShake_Randseed;

/*
	m_ChaseCameraPos = _CD.m_ChaseCameraPos;
	m_ChaseCameraDir = _CD.m_ChaseCameraDir;
*/

	m_ChaseCamDistance = _CD.m_ChaseCamDistance;
	m_dChaseCamDistance = _CD.m_dChaseCamDistance;
	m_LastChaseCamDistance = _CD.m_LastChaseCamDistance;

	m_FocusFramePosX = _CD.m_FocusFramePosX;
	m_FocusFramePosY = _CD.m_FocusFramePosY;

	m_Camera_CurHeadOffset = _CD.m_Camera_CurHeadOffset;
	m_Camera_LastHeadOffset = _CD.m_Camera_LastHeadOffset;
	m_Camera_CurBehindOffset = _CD.m_Camera_CurBehindOffset;
	m_Camera_LastBehindOffset = _CD.m_Camera_LastBehindOffset;
	m_Camera_CurHeadOffsetChange = _CD.m_Camera_CurHeadOffsetChange;
	m_Camera_CurBehindOffsetChange = _CD.m_Camera_CurBehindOffsetChange;

	m_AutoAim_LastUpdateTime = _CD.m_AutoAim_LastUpdateTime;
	m_AutoAim_LastLookMoveTime = _CD.m_AutoAim_LastLookMoveTime;

	m_ZoomSpeed = _CD.m_ZoomSpeed;
	m_CurZoom = _CD.m_CurZoom;
	m_LastZoom = _CD.m_LastZoom;

	m_MaxTargetingDistance = _CD.m_MaxTargetingDistance;

	m_Anim_LastBodyAngleZ = _CD.m_Anim_LastBodyAngleZ;
	m_Anim_BodyAngleZ = _CD.m_Anim_BodyAngleZ;
	m_Anim_LastBodyAngleY = _CD.m_Anim_LastBodyAngleY;
	m_Anim_LastBodyAngleX = _CD.m_Anim_LastBodyAngleX;

	m_Anim_ModeratedMove = _CD.m_Anim_ModeratedMove;
	m_Anim_ModeratedMovePrim = _CD.m_Anim_ModeratedMovePrim;
	m_Anim_LastLook = _CD.m_Anim_LastLook;
	m_Anim_Look = _CD.m_Anim_Look;
	m_Target_LastAimAngles = _CD.m_Target_LastAimAngles;
	m_Target_AimAnglesChange = _CD.m_Target_AimAnglesChange;
	m_Target_LastObj = _CD.m_Target_LastObj;
	m_LastRenderTime = _CD.m_LastRenderTime;
	m_LastRenderPos = _CD.m_LastRenderPos;
	m_ScreenFlashGameTime = _CD.m_ScreenFlashGameTime;
	
	m_AnimStance_Sequence = _CD.m_AnimStance_Sequence;
	m_AnimStance_LastKeyframeSeq = _CD.m_AnimStance_LastKeyframeSeq;

	m_AnimCamBlendInStart = _CD.m_AnimCamBlendInStart;
	m_AnimCamBlendOutStart = _CD.m_AnimCamBlendOutStart;

	// Animation manager
	//m_spAnimManager = _CD.m_spAnimManager;

	// Firstperson camera
	//m_FPCamera_Enable	= _CD.m_FPCamera_Enable;
	//m_FPCamera_ZoomOut = _CD.m_FPCamera_ZoomOut;
	//m_FPCamera_Pos = _CD.m_FPCamera_Pos;
	//m_FPCamera_LastPos = _CD.m_FPCamera_LastPos;
	//m_FPCamera_Count = _CD.m_FPCamera_Count;
	//m_FPCamera_ZoomOffsetChange = _CD.m_FPCamera_ZoomOffsetChange;
	//m_FPCamera_ZoomOffset = _CD.m_FPCamera_ZoomOffset;
	//m_FPCamera_LastZoomOffset = _CD.m_FPCamera_LastZoomOffset;
	
	// TraceCamera parameters
	m_fTraceHistory = _CD.m_fTraceHistory;
	m_fOldTraceHistory = _CD.m_fOldTraceHistory;
	m_iRotationIndex = _CD.m_iRotationIndex;

	m_DarklingSpawnRenderChoices = _CD.m_DarklingSpawnRenderChoices;
	m_DarklingSpawnRenderChoicesStartTick = _CD.m_DarklingSpawnRenderChoicesStartTick;

	// Character fade
	m_iWeaponFade = _CD.m_iWeaponFade;
	m_Camera_Rotation = _CD.m_Camera_Rotation;
	m_DefaultCutsceneLaps = _CD.m_DefaultCutsceneLaps;

	m_WeaponHUDStartTick = _CD.m_WeaponHUDStartTick;
	m_GroundMaterial = _CD.m_GroundMaterial;
	m_LastFootstepTick = _CD.m_LastFootstepTick;
	m_LastFootStepType = _CD.m_LastFootStepType;
	m_WeaponSelectTimeout = _CD.m_WeaponSelectTimeout;

	m_LastGunkataOpponentCheck = _CD.m_LastGunkataOpponentCheck;
	m_iGunkataOpponent = _CD.m_iGunkataOpponent;

	// Dialogue system
	m_DialogueInstance = _CD.m_DialogueInstance;
	m_PlayerDialogueToken = _CD.m_PlayerDialogueToken;
	m_pCurrentDialogueToken = _CD.m_pCurrentDialogueToken;
	m_DialogueCamera = _CD.m_DialogueCamera;
	m_PrevDialogueCameraMatrix = _CD.m_PrevDialogueCameraMatrix;
	m_iCurChoice = _CD.m_iCurChoice;
	m_nChoices = _CD.m_nChoices;
	m_Choices = _CD.m_Choices;
	m_lUsedChoices = _CD.m_lUsedChoices;
	m_iLastFocusFrameObject = _CD.m_iLastFocusFrameObject;
	m_Go3rdpOnTick = _CD.m_Go3rdpOnTick;
	
	m_bWasSwappedSpeaker = _CD.m_bWasSwappedSpeaker;
	m_DialogCamLook = _CD.m_DialogCamLook;
	
	// Fight stuff
//	m_FightModeRadius = _CD.m_FightModeRadius;
	m_LastTargetDodgeTime = _CD.m_LastTargetDodgeTime;
	m_TargetDodgeTime = _CD.m_TargetDodgeTime;
	m_TargetDodgeTimeChange = _CD.m_TargetDodgeTimeChange;
	m_DodgeTime = _CD.m_DodgeTime;
	//m_FCharMovement.Copy(_CD.m_FCharMovement);

	// Health hud
	m_HealthHudFade = _CD.m_HealthHudFade;
	m_LastHealthHudFade = _CD.m_LastHealthHudFade;
	m_HealthHudFadeChange = _CD.m_HealthHudFadeChange;

	m_ACSLastAngleOffsetX = _CD.m_ACSLastAngleOffsetX;
	m_ACSTargetAngleOffsetX = _CD.m_ACSTargetAngleOffsetX;
	m_ACSAngleOffsetXChange = _CD.m_ACSAngleOffsetXChange;

	m_ACSLastAngleOffsetY = _CD.m_ACSLastAngleOffsetY;
	m_ACSTargetAngleOffsetY = _CD.m_ACSTargetAngleOffsetY;
	m_ACSAngleOffsetYChange = _CD.m_ACSAngleOffsetYChange;

	m_ACSLastTargetDistance = _CD.m_ACSLastTargetDistance;
	m_ACSTargetDistance = _CD.m_ACSTargetDistance;
	m_ACSDistanceChange = _CD.m_ACSDistanceChange;

	m_ACSLastTargetRotation = _CD.m_ACSLastTargetRotation;
	m_ACSTargetRotation = _CD.m_ACSTargetRotation;
	m_ACSRotationChange = _CD.m_ACSRotationChange;
	
	m_ACSLastCameraTarget = _CD.m_ACSLastCameraTarget;
	m_ACSCameraTarget = _CD.m_ACSCameraTarget;
	m_ACSCameraTargetChange = _CD.m_ACSCameraTargetChange;

	m_FightLastAngleX = _CD.m_FightLastAngleX;
	m_FightTargetAngleX = _CD.m_FightTargetAngleX;
	m_FightAngleXChange = _CD.m_FightAngleXChange;

	m_FightLastAngleY = _CD.m_FightLastAngleY;
	m_FightTargetAngleY = _CD.m_FightTargetAngleY;
	m_FightAngleYChange = _CD.m_FightAngleYChange;

	/*m_TurnCorrectionLastAngle = _CD.m_TurnCorrectionLastAngle;
	m_TurnCorrectionTargetAngle = _CD.m_TurnCorrectionTargetAngle;*/
	m_TurnCorrectionAngleChange = _CD.m_TurnCorrectionAngleChange;

	m_AnimRotVel = _CD.m_AnimRotVel;

	m_LastDarknessVisibility = _CD.m_LastDarknessVisibility;
	m_CurrentDarknessVisibility = _CD.m_CurrentDarknessVisibility;
	m_DarknessVisibilityChange = _CD.m_DarknessVisibilityChange;
	
	m_CreepingLens = _CD.m_CreepingLens;
	m_CreepingLensLast = _CD.m_CreepingLensLast;

	m_AncientLens = _CD.m_AncientLens;
	m_AncientLensLast = _CD.m_AncientLensLast;

	m_HeadGibbed = _CD.m_HeadGibbed;

	/*m_LastBodyAngleZ = _CD.m_LastBodyAngleZ;
	m_TargetBodyAngleZ = _CD.m_TargetBodyAngleZ;
	m_BodyAngleZChange = _CD.m_BodyAngleZChange;*/

	m_LedgeClimbOnCounter = _CD.m_LedgeClimbOnCounter;
	m_LedgePhysWidthCached = _CD.m_LedgePhysWidthCached;
	
	m_LipSync = _CD.m_LipSync;
	m_VoCap = _CD.m_VoCap;

	m_3PI_LightState = _CD.m_3PI_LightState;
	m_3PI_LightFadeStart = _CD.m_3PI_LightFadeStart;
	m_3PI_NoCamera = _CD.m_3PI_NoCamera;
	m_DarknessVoiceEffect = _CD.m_DarknessVoiceEffect;
	m_DarknessVoiceUse = _CD.m_DarknessVoiceUse;

	m_Occlusion = _CD.m_Occlusion;

	m_lCharacterCloth = _CD.m_lCharacterCloth;

	m_iDarkness_Drain_1 = _CD.m_iDarkness_Drain_1;
	//m_DarknessDrainTick = _CD.m_DarknessDrainTick;
	m_DarknessDrainTime = _CD.m_DarknessDrainTime;
	m_DarknessPowersAvailable = _CD.m_DarknessPowersAvailable;

	m_WindForce = _CD.m_WindForce;
	
	m_Disorientation_Flash = _CD.m_Disorientation_Flash;

	// IK stuff
	m_aLegIKIsDirty[0] = _CD.m_aLegIKIsDirty[0];
	m_aLegIKIsDirty[1] = _CD.m_aLegIKIsDirty[1];
	m_aLegIKIsDirty[2] = _CD.m_aLegIKIsDirty[2];
	m_aLegIKIsDirty[3] = _CD.m_aLegIKIsDirty[3];

	m_PostAnimSystem.CopyValues(&_CD.m_PostAnimSystem);

#if 1
	m_Debug_ShowHud = _CD.m_Debug_ShowHud;
#endif
}


CWObject_Client* CWO_Character_ClientData::CreateClientObject(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWO_Character_ClientData_CreateClientObject, NULL);
	if (!_pObj->GetNext())
	{
		spCWObject_Client spObj = MNew(CWObject_Client);
		if (!spObj) Error_static("CWO_Character_ClientData::CreateClientObject", "Could not allocate copy.");
		spObj->LinkAfter(_pObj);
		_pObj->m_pRTC->m_pfnOnInitClientObjects(spObj, _pWClient);

		CWO_Character_ClientData *pCDBase = CWObject_Character::GetClientData(_pObj);
		CWO_Character_ClientData *pCDPred = CWObject_Character::GetClientData(spObj);

		if (!pCDBase || !pCDPred)
			Error_static("CWO_Character_ClientData::CreateClientObject", "NULL client-data.");
	}

	if (!_pObj->GetNext())
		Error_static("CWO_Character_ClientData::CreateClientObject", "Internal error.");

	return _pObj->GetNext();
}


void CWO_Character_ClientData::CopyClientObject(CWObject_Client* _pObj, CWObject_Client* _pDest)
{
	MAUTOSTRIP(CWO_Character_ClientData_CopyClientObject, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWO_Character_ClientData::CopyClientObject);
	CWO_Player_ClientData *pCDBase = CWObject_Player::GetCD(_pObj);
	CWO_Player_ClientData *pCDDest = CWObject_Player::GetCD(_pDest);

	// Copy object
	*_pDest = *_pObj;

	// Copy skeletoninstance (Share, actually)
	_pDest->m_lspClientObj[1] = _pObj->m_lspClientObj[1];

	// Copy client-data
	pCDDest->Copy(*pCDBase);

	pCDDest->m_bIsValidPrediction = false;
	pCDDest->m_PredictLastCmdID = -1;
}


void CWO_Character_ClientData::AddTilt(const CVec3Dfp32& _Axis, fp32 _Angle)
{
	MAUTOSTRIP(CWO_Character_ClientData_AddTilt, MAUTOSTRIP_VOID);
	CVec3Dfp32 Axis = _Axis;;
	Axis.Normalize();
	CAxisRotfp32 Tilt(Axis, _Angle);
	CAxisRotfp32 Res;
	m_Cam_Tilt.Multiply(Tilt);
	m_Cam_TiltAnglePrim = 0;
	//	ConOut(CStrF("AddTilt: %s, %f, %s", (char*) _Axis.GetString(), _Angle, (char*) m_Cam_Tilt.GetString() ));
}


void CWO_Character_ClientData::ModerateTilt(fp32 _dTime)
{
	MAUTOSTRIP(CWO_Character_ClientData_ModerateTilt, MAUTOSTRIP_VOID);
	fp32 Time = Max(0.0f, Min(0.040f, _dTime));
	m_Cam_Tilt.m_Angle *= 100.0f;
	Moderatef(m_Cam_Tilt.m_Angle, 0, m_Cam_TiltAnglePrim, (int)(20 * 256 * Time));
	m_Cam_Tilt.m_Angle /= 100.0f;
	
	//void Moderatef(fp32& _x, fp32 _newx, fp32& _xprim, int a)
}


CQuatfp32 CWO_Character_ClientData::Target_GetQuatForAngles(const CVec2Dfp32& _Angles) const
{
	CVec3Dfp32 Angles(0, _Angles[0], _Angles[1]);

	CMat4Dfp32 Mat;
	Angles.CreateMatrixFromAngles(0, Mat);

	CQuatfp32 Q;
	Q.Create(Mat);

	return Q;
}


CVec2Dfp32 CWO_Character_ClientData::Target_GetAnglesForQuat(const CQuatfp32& _Quat) const
{
	MAUTOSTRIP(CWO_Character_ClientData_Target_GetAnglesForQuat, CVec2Dfp32());
	CMat4Dfp32 Mat;
	_Quat.CreateMatrix(Mat);

	return Target_GetAnglesForVector(CVec3Dfp32::GetRow(Mat, 0));
}


CVec2Dfp32 CWO_Character_ClientData::Target_GetAnglesForVector(const CVec3Dfp32& _Vec)
{
	MAUTOSTRIP(CWO_Character_ClientData_Target_GetAnglesForVector, CVec2Dfp32());
	CVec3Dfp32 Look = CWObject_Character::GetLook(_Vec);
	return CVec2Dfp32(Look[1], Look[2]);
}


CVec3Dfp32 CWO_Character_ClientData::Target_GetVectorForAngles(const CVec2Dfp32& _Angles)
{
	MAUTOSTRIP(CWO_Character_ClientData_Target_GetVectorForAngles, CVec3Dfp32());
	CVec3Dfp32 Angles(0, _Angles[0], _Angles[1]);

	CMat4Dfp32 Mat;
	Angles.CreateMatrixFromAngles(0, Mat);

	return CVec3Dfp32::GetRow(Mat, 0);
}


void CWO_Character_ClientData::Target_SetAimAngles(fp32 _XAngle, fp32 _YAngle)
{
	MAUTOSTRIP(CWO_Character_ClientData_Target_SetAimAngles, MAUTOSTRIP_VOID);
	uint16 x = int(_XAngle*65536.0f) & 0xffff;
	uint16 y = int(_YAngle*65536.0f) & 0xffff;
	m_TargetAimPos = x + (y << 16);
}


void CWO_Character_ClientData::Target_SetAimAnglesVel(fp32 _dXAngle, fp32 _dYAngle)
{
	MAUTOSTRIP(CWO_Character_ClientData_Target_SetAimAnglesVel, MAUTOSTRIP_VOID);
	uint16 dx = int(_dXAngle*65536.0f) & 0xffff;
	uint16 dy = int(_dYAngle*65536.0f) & 0xffff;
	m_TargetAimPosVel = dx + (dy << 16);
}


void CWO_Character_ClientData::Target_GetAimAngles(fp32& _XAngle, fp32& _YAngle) const
{
	MAUTOSTRIP(CWO_Character_ClientData_Target_GetAimAngles, MAUTOSTRIP_VOID);
	int TargetAimPos = m_TargetAimPos;
	_XAngle = fp32(TargetAimPos & 0xffff) / 65536.0f;
	_YAngle = fp32((TargetAimPos >> 16) & 0xffff) / 65536.0f;
}


void CWO_Character_ClientData::Target_GetAimAnglesVel(fp32& _dXAngle, fp32& _dYAngle) const
{
	MAUTOSTRIP(CWO_Character_ClientData_Target_GetAimAnglesVel, MAUTOSTRIP_VOID);
	int TargetAimPosVel = m_TargetAimPosVel;
	_dXAngle = fp32(int16(TargetAimPosVel & 0xffff)) / 65536.0f;
	_dYAngle = fp32(int16((TargetAimPosVel >> 16) & 0xffff)) / 65536.0f;
}


void CWO_Character_ClientData::Target_AddAimAngles(const CVec2Dfp32& _Vel)
{
	MAUTOSTRIP(CWO_Character_ClientData_Target_AddAimAngles, MAUTOSTRIP_VOID);
	Target_SetAimAngles(_Vel + Target_GetAimAngles());
}


void CWO_Character_ClientData::Target_AddAimAnglesVel(const CVec2Dfp32& _Acc)
{
	MAUTOSTRIP(CWO_Character_ClientData_Target_AddAimAnglesVel, MAUTOSTRIP_VOID);
	Target_SetAimAnglesVel(_Acc + Target_GetAimAnglesVel());
}


void CWO_Character_ClientData::Target_SetAimVector(const CVec3Dfp32& _v)
{
	MAUTOSTRIP(CWO_Character_ClientData_Target_SetAimVector, MAUTOSTRIP_VOID);
	CVec2Dfp32 Angles = Target_GetAnglesForVector(_v);
	Target_SetAimAngles(Angles[0], Angles[1]);
}


CVec3Dfp32 CWO_Character_ClientData::Target_GetAimVector() const
{
	MAUTOSTRIP(CWO_Character_ClientData_Target_GetAimVector, CVec3Dfp32());
	// Hardly the most efficient method, but it works..

	CVec2Dfp32 Angles(0);
	Target_GetAimAngles(Angles[0],Angles[1]);

	return Target_GetVectorForAngles(Angles);
}


void CWO_Character_ClientData::Target_GetAimMatrix(const CVec2Dfp32& _Angles, CMat4Dfp32& _Mat) const
{
	MAUTOSTRIP(CWO_Character_ClientData_Target_GetAimMatrix, MAUTOSTRIP_VOID);
	CVec3Dfp32 Angles(0, _Angles[0], _Angles[1]);
	Angles.CreateMatrixFromAngles(0, _Mat);
}


void CWO_Character_ClientData::Target_GetAimMatrix(CMat4Dfp32& _Mat) const
{
	MAUTOSTRIP(CWO_Character_ClientData_Target_GetAimMatrix_2, MAUTOSTRIP_VOID);
	Target_GetAimMatrix(Target_GetAimAngles(), _Mat);
}


CVec2Dfp32 CWO_Character_ClientData::Target_WrapAngles(const CVec2Dfp32& _ATarget, const CVec2Dfp32& _ACurrent)
{
	MAUTOSTRIP(CWO_Character_ClientData_Target_WrapAngles, CVec2Dfp32());
	// Return a "wrapped" _ATarget so that it's angles are in [-0.5..0.5] range relative to _ACurrent

	CVec2Dfp32 ATarget(_ATarget);

	// Wrap target
	for(int i = 0; i < 2; i++)
	{
		if (_ACurrent[i] - ATarget[i] > 0.5f)
			ATarget[i] += 1.0f;
		if (_ACurrent[i] - ATarget[i] < -0.5f)
			ATarget[i] -= 1.0f;
	}

	return ATarget;
}


CVec2Dfp32 CWO_Character_ClientData::Target_LerpAngles(const CVec2Dfp32& _From, const CVec2Dfp32& _To, fp32 _t)
{
	MAUTOSTRIP(CWO_Character_ClientData_Target_LerpAngles, CVec2Dfp32());
	CVec2Dfp32 To = Target_WrapAngles(_To, _From);
	CVec2Dfp32 Ret;
	_From.Lerp(To, _t, Ret);
	return Ret;
}


//--------------------------------------------------------------------------------

// Should draw Move velocity, local move velocity, move control
/*void DebugDrawController(const CWAGI_Context* _pContext, CVec3Dfp32 _MoveVelocity, CVec3Dfp32 _LocalMoveVel, CVec3Dfp32 _MoveControl, fp32 _Size)
{
	// Draw circle, player direction, controller direction/length
	CWireContainer* pWC = _pContext->m_pWPhysState->Debug_GetWireContainer();
	if (pWC)
	{
		CVec3Dfp32 Pos = _pContext->m_pObj->GetPosition() + CVec3Dfp32(0,0,3);
		pWC->RenderWire(Pos, Pos + _MoveVelocity * _Size, 0xffff0000, 0.1f, false);
		pWC->RenderWire(Pos, Pos + _LocalMoveVel * _Size, 0xff00ff00, 0.1f, false);
		//pWC->RenderWire(Pos, Pos + _LocalVelocity, 0xff00ffff, 0.1f, false);
		CVec3Dfp32 CharDir = CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),0);
		CharDir *= _Size * 5;
		_pContext->m_pWPhysState->Debug_RenderWire(Pos, Pos + CharDir, 0xffffffff,0.1f, false);

		// Get Controller dir
		fp32 CharAngle = CVec3Dfp32::AngleFromVector(CharDir.k[0], CharDir.k[1]);
		fp32 ControllerAngle = _ControllerAngle - CharAngle;
		ControllerAngle = MACRO_ADJUSTEDANGLE(GetRealAngle(ControllerAngle));
		CVec3Dfp32 ControllerDir = VectorFromAngles(0, ControllerAngle);
		ControllerDir *= _MoveRadius * Size;
		_pContext->m_pWPhysState->Debug_RenderWire(Pos, Pos + ControllerDir, 0xffff0000,0.1f, false);

		//ConOut(CStrF("ControllerAngle: %f CamControlAngle. %f CharAngle: %f",_ControllerAngle, _CamControlAngle, CharAngle));

		// Cam relative controllerdir
		ControllerAngle = MACRO_ADJUSTEDANGLE(GetRealAngle(_CamControlAngle));
		ControllerDir = VectorFromAngles(0, ControllerAngle);
		ControllerDir *= _MoveRadius * Size;
		_pContext->m_pWPhysState->Debug_RenderWire(Pos, Pos + ControllerDir, 0xff0000ff,0.1f, false);

		// Camera direction
		ControllerAngle = MACRO_ADJUSTEDANGLE(_CameraAngle);
		ControllerDir = VectorFromAngles(0, ControllerAngle);
		ControllerDir *= Size;
		_pContext->m_pWPhysState->Debug_RenderWire(Pos, Pos + ControllerDir, 0xff00ff00,0.1f, false);
	}
}*/

#define MACRO_ADJUSTANDSETANGLE(p) if (p < 0.0f) p += 1.0f + M_Floor(-p); else if (p > 1.0f) p -= M_Floor(p);
#define CPI					3.14159265358979323846264338327950288419716939937510582097494459f
#define CPI2				6.283185307179586476925286766559f
void DebugDrawCirle(const CWAG2I_Context* _pContext, CVec3Dfp32 _StartPos, int _Res, fp32 _Size, 
			   int _Color = 0xffffffff, bool bRenderCharDir = false)
{
	CVec3Dfp32 Dir(0,0,0);
	
	fp32 Frag = CPI2 / _Res;
	//CVec3Dfp32 CharPos = _pContext->m_pObj->GetPosition() + CVec3Dfp32(0,0,65);
	CVec3Dfp32 PrevPos = _StartPos + CVec3Dfp32(1,0,0) * _Size;
	
	for (int i = 0; i < _Res; i++)
	{
		fp32 Angle = Frag * i;
		Dir.k[0] = M_Cos(Angle);
		Dir.k[1] = M_Sin(Angle);
		
		CVec3Dfp32 Pos = Dir * _Size + _StartPos;
		_pContext->m_pWPhysState->Debug_RenderWire(PrevPos, Pos, _Color, 0.1f, false);

		PrevPos = Pos;
	}

	_pContext->m_pWPhysState->Debug_RenderWire(PrevPos, _StartPos + CVec3Dfp32(1,0,0) * _Size, _Color, 0.1f, false);
	if (bRenderCharDir)
	{
		CVec3Dfp32 CharDir = CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(), 0);
		_pContext->m_pWPhysState->Debug_RenderWire(_StartPos, _StartPos + CharDir * _Size, _Color, 0.1f, false);
	}
}

CVec3Dfp32 VectorFromAngles(fp32 _Pitch, fp32 _Yaw)
{
	CVec3Dfp32 Dir;
	Dir.k[2] = M_Sin(_Pitch * CPI2);
	Dir.k[0] = M_Cos(_Yaw * CPI2);
	Dir.k[1] = M_Sin(_Yaw * CPI2);

	Dir.Normalize();

	return Dir;
}

void DebugDrawController(const CWAG2I_Context* _pContext, fp32 _MoveVelAngle, fp32 _MoveVelocity, 
						 fp32 _CharMoveDir, fp32 _SomeOtherDir)
{
	// Draw circle, player direction, controller direction/length
	CWireContainer* pWC = _pContext->m_pWPhysState->Debug_GetWireContainer();
	fp32 Size = 25;
	if (pWC)
	{
		CVec3Dfp32 Pos = _pContext->m_pObj->GetPosition() + CVec3Dfp32(0,0,3);
		DebugDrawCirle(_pContext, Pos, 30, Size);
		CVec3Dfp32 CharDir = CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),0);
		CharDir *= Size;
		pWC->RenderWire(Pos, Pos + CharDir, 0xffffffff,0.1f, false);

		// Get Move velocity direction
		CVec3Dfp32 MoveVelAngleDir = VectorFromAngles(0, _MoveVelAngle);
		MoveVelAngleDir *= _MoveVelocity * Size;
		_pContext->m_pWPhysState->Debug_RenderWire(Pos, Pos + MoveVelAngleDir, 0xffff0000,0.1f, false);

		// Char velocity direction
		MoveVelAngleDir = VectorFromAngles(0, _CharMoveDir);
		MoveVelAngleDir *= _MoveVelocity * Size;
		_pContext->m_pWPhysState->Debug_RenderWire(Pos, Pos + MoveVelAngleDir, 0xff00ff00,0.1f, false);

		// Some other dir
		MoveVelAngleDir = VectorFromAngles(0, _SomeOtherDir);
		MoveVelAngleDir *= _MoveVelocity * Size;
		_pContext->m_pWPhysState->Debug_RenderWire(Pos, Pos + MoveVelAngleDir, 0xff0000ff,0.1f, false);

		//ConOut(CStrF("MovAngle: %f CharMoveAngle. %f Vel: %f",_MoveVelAngle, _CharMoveDir, _MoveVelocity));
	}
}

void CWO_Character_ClientData::AG2_RefreshGlobalProperties(const CWAG2I_Context* _pContext)
{
	if (_pContext == NULL)
		return;
	
	m_AnimGraph2.m_SyncAnimScale = Max(Min(_pContext->m_pObj->GetMoveVelocity().LengthSqr() / (m_Speed_Forward * m_Speed_Forward),1.0f),0.0f);

	CVec3Dfp32 GroundVelocity = 0;
	if (m_Phys_iLastGroundObject != 0)
		GroundVelocity = _pContext->m_pWPhysState->Object_GetVelocity(m_Phys_iLastGroundObject);

	// Set supported stances if they aren't set yet...
	m_AnimGraph2.SetSupportedStances(_pContext, 0);

	CVec3Dfp32 Control = m_Control_Move;
	Control.k[2] = 0.0f;
	CVec3Dfp32 MoveVeloVectorWorld = _pContext->m_pObj->GetMoveVelocity() - GroundVelocity;
	// Recalculate movevelovector world to 20hz speed (should be changed to / sec, but meh
	// will need to sync code/content)
	MoveVeloVectorWorld *= _pContext->m_pWPhysState->GetGameTicksPerSecond() / 20.0f;
	m_AnimGraph2.CWO_Clientdata_Character_AnimGraph2::AG2_RefreshGlobalProperties(_pContext);
	m_AnimGraph2.SetPropertyFloat(PROPERTY_FLOAT_MOVERADIUSCONTROL, Control.Length());
	fp32 MoveAngleUnit = CVec2Dfp32(m_Control_Move[0], -m_Control_Move[1]).GetAngle();
	fp32 MoveAngle = MoveAngleUnit * 360.0f;
	// Hmm, have remade it a bit, seems to work fine now... should fix it up a bit though

	const CMat4Dfp32& PosMat = _pContext->m_pObj->GetPositionMatrix();
	CVec3Dfp32 MoveVeloLocal;
	fp32 NewMoveAngle;
	CWO_CharDarkling_ClientData* pDarklingCD = IsDarkling();
	if (pDarklingCD)
	{
		const CVec3Dfp32& Gravity = pDarklingCD->m_Gravity;
		CMat4Dfp32 BaseMat = PosMat;
		BaseMat.GetRow(2) = -Gravity;		// Align with ground
		BaseMat.RecreateMatrix(2, 0);		// --

		MoveVeloLocal.k[0] = MoveVeloVectorWorld * BaseMat.GetRow(0);
		MoveVeloLocal.k[1] = MoveVeloVectorWorld * BaseMat.GetRow(1);
		MoveVeloLocal.k[2] = MoveVeloVectorWorld * BaseMat.GetRow(2);

		NewMoveAngle = -CVec2Dfp32(MoveVeloLocal.k[0], MoveVeloLocal.k[1]).GetAngle();
		NewMoveAngle = MACRO_ADJUSTEDANGLE(NewMoveAngle);
	}
	else
	{
		MoveVeloLocal = MoveVeloVectorWorld;
		const CVec3Dfp32& CharDir = PosMat.GetRow(0);
		fp32 CharAngle = CVec2Dfp32(CharDir.k[0], CharDir.k[1]).GetAngle();
		fp32 MoveDirWorld = CVec2Dfp32(MoveVeloVectorWorld.k[0], MoveVeloVectorWorld.k[1]).GetAngle();
		NewMoveAngle = MACRO_ADJUSTEDANGLE((CharAngle - MoveDirWorld));
	}

	m_AnimGraph2.SetPropertyFloat(PROPERTY_FLOAT_MOVEANGLECONTROL, MoveAngle);
	m_AnimGraph2.SetPropertyFloat(PROPERTY_FLOAT_MOVEANGLESCONTROL, ((MoveAngleUnit < 0.5f) ? MoveAngleUnit : (MoveAngleUnit - 1.0f)) * 360.0f);
	m_AnimGraph2.SetPropertyFloat(PROPERTY_FLOAT_MOVEANGLEUNITCONTROL, MoveAngleUnit);
	m_AnimGraph2.SetPropertyFloat(PROPERTY_FLOAT_MOVEANGLEUNITSCONTROL, ((MoveAngleUnit < 0.5f) ? MoveAngleUnit : (MoveAngleUnit - 1.0f)));
	fp32 MoveVel = CVec2Dfp32(MoveVeloLocal[0], MoveVeloLocal[1]).Length();
	m_AnimGraph2.SetPropertyFloat(PROPERTY_FLOAT_MOVEVELOCITY, MoveVel);
	m_AnimGraph2.SetAdaptiveTimeScale(MoveVel * 20.0f / m_AnimGraph2.GetPropertyFloat(PROPERTY_FLOAT_ANIMMOVELENGTH));
	m_AnimGraph2.SetPropertyFloat(PROPERTY_FLOAT_MOVEVELOCITYVERTICAL, MoveVeloLocal[2]);
	m_AnimGraph2.SetPropertyFloat(PROPERTY_FLOAT_MOVEANGLE, NewMoveAngle * 360.0f);
	m_AnimGraph2.SetPropertyFloat(PROPERTY_FLOAT_MOVEANGLES,((NewMoveAngle < 0.5f) ? NewMoveAngle : (NewMoveAngle - 1.0f)) * 360.0f);
	m_AnimGraph2.SetPropertyFloat(PROPERTY_FLOAT_MOVEANGLEUNIT, NewMoveAngle);
	m_AnimGraph2.SetPropertyFloat(PROPERTY_FLOAT_MOVEANGLEUNITS, ((NewMoveAngle < 0.5f) ? NewMoveAngle : (NewMoveAngle - 1.0f)));
	m_AnimGraph2.SetPropertyFloat(PROPERTY_FLOAT_MOVEHCONTROL, -m_Control_Move.k[1]);
	m_AnimGraph2.SetPropertyFloat(PROPERTY_FLOAT_MOVEVCONTROL, m_Control_Move.k[0]);
	// TEMP REMOVE
	//m_AnimGraph2.SetPropertyFloat(PROPERTY_FLOAT_VIGILANCE, 0.0f);
	//m_AnimGraph2.SetPropertyFloat(PROPERTY_FLOAT_LEFTTRIGGER, 
	//	PROPERTY_FLOAT_RIGHTTRIGGER = 28,
	m_AnimGraph2.SetPropertyFloat(PROPERTY_FLOAT_HEALTH, (fp32)m_Health);
	m_AnimGraph2.SetPropertyFloat(PROPERTY_FLOAT_ANGLEDIFFS, m_TurnCorrectionTargetAngle);

	// Int properties
	int32 ButtonPress = ((m_Control_Press & (CONTROLBITS_BUTTON0 | CONTROLBITS_BUTTON1 | 
		CONTROLBITS_BUTTON2 | CONTROLBITS_BUTTON3)) >> BUTTONPRESS_SHIFT) | 
		(m_Control_Press & (CONTROLBITS_PRIMARY | CONTROLBITS_SECONDARY | CONTROLBITS_JUMP));
	m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BUTTONPRESS,ButtonPress);
	m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BUTTONPRESSED,m_AnimGraph2.GetPropertyInt(PROPERTY_INT_BUTTONPRESSED) & ButtonPress);
	m_AnimGraph2.SetPropertyInt(PROPERTY_INT_JOYPAD, (m_Control_Press & (CONTROLBITS_DPAD_UP | CONTROLBITS_DPAD_DOWN | CONTROLBITS_DPAD_LEFT | CONTROLBITS_DPAD_RIGHT)) >> DIGIPAD_SHIFT);
	if ((int32)(M_Fabs(m_TurnCorrectionTargetAngle) * 360) > m_AnimGraph2.m_IdleTurnThreshold)
		m_AnimGraph2.m_IdleTurnTimer++;
	else
		m_AnimGraph2.m_IdleTurnTimer = 0;
	m_AnimGraph2.SetPropertyInt(PROPERTY_INT_IDLETURNTIMER, m_AnimGraph2.m_IdleTurnTimer);
	m_AnimGraph2.SetPropertyInt(PROPERTY_INT_EQUIPPEDITEMCLASS, m_EquippedItemClass);
	m_AnimGraph2.SetPropertyInt(PROPERTY_INT_EQUIPPEDITEMTYPE, m_EquippedItemType);
	m_AnimGraph2.SetPropertyInt(PROPERTY_INT_ANIMGRIPTYPERIGHT, m_AnimGripTypeRight);
	m_AnimGraph2.SetPropertyInt(PROPERTY_INT_ANIMGRIPTYPELEFT, m_AnimGripTypeLeft);
	m_AnimGraph2.SetPropertyInt(PROPERTY_INT_RAND255, (int32)(MFloat_GetRand(m_AnimGraph2.m_iRandseed) * 255.0f));
	m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_ISPLAYER, m_iPlayer != -1);

	if (m_iMountedObject && (m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_TURRET))
	{
		CVec2Dfp32 Angles;
		CWObject_Message Msg(CWObject_Turret::OBJMSG_TURRET_GETRELATIVEANGLES);
		Msg.m_VecParam0.k[1] = m_ActionCutSceneCameraOffsetY;
		Msg.m_VecParam0.k[2] = m_ActionCutSceneCameraOffsetX;
		Msg.m_pData = &Angles;
		Msg.m_DataSize = sizeof(Angles);

		_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg,m_iMountedObject);
		m_AnimGraph2.SetPropertyFloat(PROPERTY_FLOAT_TURRETHORIZONTAL, Angles.k[0]);
		m_AnimGraph2.SetPropertyFloat(PROPERTY_FLOAT_TURRETVERTICAL, Angles.k[1]);
	}


	// Check if we should stop
	/*if (m_AnimGraph2.GetPropertyBool(PROPERTY_BOOL_WALKSTOPENABLED) && m_AnimGraph2.m_MDestination.GetRow(3).k[0] != _FP32_MAX)
	{
		// Check distance from current position (get length from current state...?)
		fp32 StopLength = m_AnimGraph2.GetStopLength();
		CVec3Dfp32 Dir = m_AnimGraph2.m_MDestination.GetRow(3) - _pContext->m_pObj->GetPosition();
		CVec3Dfp32 FaceDir = CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),0);
		fp32 LenSqr = Dir.LengthSqr();
		int32 StopMode = AG2_STOPMODE_NONE;
		if (LenSqr < Sqr(StopLength))
		{
			// Oki then, might have to stop, check further if it's in out current direction
			if (LenSqr > 0.0f)
			{
				fp32 Len = M_Sqrt(LenSqr);
				Dir = Dir / Len;
				if (Dir * FaceDir > 0.9f)
				{
					StopMode = Len < (StopLength*0.7f) ? AG2_STOPMODE_EMERGENCY : AG2_STOPMODE_NORMAL;
				}
			}
			else
			{
				StopMode = AG2_STOPMODE_EMERGENCY;
			}
		}
		m_AnimGraph2.SetPropertyInt(PROPERTY_INT_STOPMODE,StopMode);
	}
	else
	{
		m_AnimGraph2.SetPropertyInt(PROPERTY_INT_STOPMODE,AG2_STOPMODE_NONE);
	}*/


	// Set weaponclass....
	m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WEAPONCLASS,m_EquippedItemClass);

	// Bool properties
	m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_ISINAIR,m_Phys_bInAir != 0);
	m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_ISCROUCHING, CWObject_Character::Char_GetPhysType(_pContext->m_pObj) == PLAYER_PHYS_CROUCH);
	m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_ISDEAD,CWObject_Character::Char_GetPhysType(_pContext->m_pObj) == PLAYER_PHYS_DEAD);

#ifdef AG2_RECORDPROPERTYCHANGES
	m_AnimGraph2.m_PropertyRecorder.Update(_pContext,m_TurnCorrectionTargetAngle);
#endif
}
//--------------------------------------------------------------------------------

void CWO_Character_ClientData::OnRefresh(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, const CMTime& _GameTime)
{
	m_Control_Press_LastPressAGRefresh = m_Control_Press | m_Control_Press_Intermediate;
}

//--------------------------------------------------------------------------------

void CWO_Character_ClientData::OnClientRefresh(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, const CMTime& _GameTime)
{
	m_Blink.Update( m_pWPhysState->GetGameTickTime() );

	// Update drain time
	fp32 GameTickTime = _pWPhysState->GetGameTickTime();
	if(m_DarknessDrainTime > -0.001f && m_DarknessDrainTime < 1.001f)
	{
		if(m_DarknessSelectionMode.m_Value & PLAYER_DARKNESSMODE_POWER_DRAIN)
			m_DarknessDrainTime = Clamp01(m_DarknessDrainTime + GameTickTime);
		else
			m_DarknessDrainTime = Clamp((3.0f - m_DarknessDrainTime), 2.0f, 3.0f);
	}
	else if(m_DarknessDrainTime > 1.999f && m_DarknessDrainTime < 3.001f)
	{
		if(m_DarknessSelectionMode.m_Value & PLAYER_DARKNESSMODE_POWER_DRAIN)
			m_DarknessDrainTime = Clamp01((3.0f - m_DarknessDrainTime));
		else
			m_DarknessDrainTime = Clamp(m_DarknessDrainTime + GameTickTime, 2.0f, 3.0f);
	}

	// It's updated from OnGetAnimState (Not so nice), so after it has been finished, we tone it down to zero
	if (!m_DarknessVoiceUse.m_Value)
		m_DarknessVoiceEffect = Clamp01(m_DarknessVoiceEffect - GameTickTime);
}

void CWO_Character_ClientData::Char_UpdateLook(fp32 _FrameFrac)
{
	CWObject_Character::Char_UpdateLook(m_pObj, this, m_pWPhysState, _FrameFrac);
}


void CWO_Character_ClientData::Phys_Move(const CSelection& _Selection, fp32 _dTime, const CVec3Dfp32& _UserVel, bool _bPredicted)
{
	// For regular characters (CharPlayer & CharNPC), let's just jump to the old code.
	// (other characters, like CharDarkling, will do the movement code in their ClientData class)
	CWObject_Character::Phys_Move(_Selection, m_pObj, this, m_pWPhysState, _dTime, _UserVel, _bPredicted);
}

#define MAXVIEWANGLEDLGCAM 0.03f
#define VALFORDAMPDLGCAM 0.005f

void CWO_Character_ClientData::DampDLookForDialogueCameras(CVec3Dfp32 &_dLook)
{
	_dLook[2] *= 0.333f;

	if((m_DialogCamLook.k[1] > VALFORDAMPDLGCAM) && _dLook[1] > 0.0f)
	{
		fp32 Diff = (m_DialogCamLook.k[1] - VALFORDAMPDLGCAM);

		// wont overshoot MaxVal anyways
		Diff = Min(Diff, (MAXVIEWANGLEDLGCAM-VALFORDAMPDLGCAM));
		_dLook[1] *= (1.0f - (Diff / (MAXVIEWANGLEDLGCAM-VALFORDAMPDLGCAM)));
	}
	else if((m_DialogCamLook.k[1] < -VALFORDAMPDLGCAM) && _dLook[1] < 0.0f)
	{
		fp32 Diff = (-VALFORDAMPDLGCAM - m_DialogCamLook.k[1]);

		// wont overshoot MaxVal anyways
		Diff = Min(Diff, (MAXVIEWANGLEDLGCAM-VALFORDAMPDLGCAM));
		_dLook[1] *= (1.0f - (Diff / (MAXVIEWANGLEDLGCAM-VALFORDAMPDLGCAM)));
	}

	if((m_DialogCamLook.k[2] > VALFORDAMPDLGCAM) && _dLook[2] > 0.0f)
	{
		fp32 Diff = (m_DialogCamLook.k[2] - VALFORDAMPDLGCAM);

		// wont overshoot MaxVal anyways
		Diff = Min(Diff, (MAXVIEWANGLEDLGCAM-VALFORDAMPDLGCAM));
		_dLook[2] *= (1.0f - (Diff / (MAXVIEWANGLEDLGCAM-VALFORDAMPDLGCAM)));
	}
	else if((m_DialogCamLook.k[2] < -VALFORDAMPDLGCAM) && _dLook[2] < 0.0f)
	{
		fp32 Diff = (-VALFORDAMPDLGCAM - m_DialogCamLook.k[2]);

		// wont overshoot MaxVal anyways
		Diff = Min(Diff, (MAXVIEWANGLEDLGCAM-VALFORDAMPDLGCAM));
		_dLook[2] *= (1.0f - (Diff / (MAXVIEWANGLEDLGCAM-VALFORDAMPDLGCAM)));
	}

}

void CWO_Character_ClientData::Char_ProcessControl_Look(const CVec3Dfp32& _dLook)
{
	MSCOPESHORT(CWO_Character_ClientData::Char_ProcessControl_Look);
	CWO_Character_ClientData* pCD = this;

	CVec3Dfp32 dLook = _dLook;
	
	if(dLook.Length() > 0.00001f)
		pCD->m_AutoAim_LastLookMoveTime = pCD->m_GameTime;

	// Adjust look speed when zooming 
	{
		fp32 IPTime = pCD->m_PredictFrameFrac;
		fp32 Zoom = LERP(pCD->m_LastZoom, pCD->m_CurZoom, IPTime);
		dLook *= 1.0f / Zoom;
	}

	// Adjust look speed according to damper value
	if (m_LookSpeedDamper > 0)
		dLook *= 1.0f - (m_LookSpeedDamper * (1.0f / 255.0f));

	if (!(pCD->m_WeaponZoomState & WEAPON_ZOOMSTATE_OFF))
	{
		dLook *= 0.4f;
	}


	//-------------------------------------------------------------------------------------------------
	// Willbo: Quick Fix. I know, I know... This shouldn't be here, just pretend you didn't see it ok? =)	
	if(pCD->m_pWPhysState && pCD->m_pWPhysState->IsServer())
		pCD->m_Control_IdleTicks = pCD->m_pWPhysState->GetGameTick();

	//-------------------------------------------------------------------------------------------------
	// Dialogue contoldampener etc
	
	fp32 SlideTime = pCD->m_3PI_CameraSlide.Get(pCD->m_GameTick, pCD->m_PredictFrameFrac) * (1.0f / 255.0f);
	if(((pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_MASK) == THIRDPERSONINTERACTIVE_MODE_DIALOGUE) && SlideTime > 0)
	{
		// if speaking or have choices, run with normal stuff
		// store m_control_look for the first frame
		pCD->DampDLookForDialogueCameras(dLook);
		m_DialogCamLook.k[1] += dLook[1];
		m_DialogCamLook.k[2] += dLook[2];
		return;
	}
	else
		m_bWasSwappedSpeaker = false;
		

	if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_CREEPINGCAM)
	{
		// Update look for creepingdark
		CWObject_CoreData* pCreep = m_pWPhysState->Object_GetCD(pCD->m_iCreepingDark);
		CWObject_CreepingDark::UpdateCameraLook(m_pWPhysState, pCreep, pCD->m_Creep_Control_Look_Wanted.m_Value, dLook);
		pCD->m_Creep_Control_Look_Wanted.MakeDirty();
		return;
	}

	if (pCD->m_iMountedObject != 0)
	{
		if (pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_TURRET)
		{
			// Get min max from the mounted object
			CVec3Dfp32 MaxYZ;
			CWObject_Message Msg(CWObject_Turret::OBJMSG_TURRET_GETMINMAXYZ);
			Msg.m_pData = &MaxYZ;
			Msg.m_DataSize = sizeof(CVec3Dfp32);
			if (m_pWPhysState->Phys_Message_SendToObject(Msg,pCD->m_iMountedObject))
			{
				dLook *= MaxYZ[0];
				if (MaxYZ[1] > 0.45f)
				{	// Move towards zero when outside FOV
					if (pCD->m_ActionCutSceneCameraOffsetY.m_Value + dLook[1] < -MaxYZ[1])
					{
						dLook[1] = Abs(dLook[1]);
					}
					else if (pCD->m_ActionCutSceneCameraOffsetY.m_Value + dLook[1] > MaxYZ[1])
					{
						dLook[1] = -Abs(dLook[1]);
					}
				}
				if (MaxYZ[2] > 0.45f)
				{	// Move towards zero when outside FOV
					if (pCD->m_ActionCutSceneCameraOffsetX.m_Value + dLook[2] < -MaxYZ[2])
					{
						dLook[2] = Abs(dLook[2]);
					}
					else if (pCD->m_ActionCutSceneCameraOffsetX.m_Value + dLook[2] > MaxYZ[2])
					{
						dLook[2] = -Abs(dLook[2]);
					}
				}
				pCD->m_ActionCutSceneCameraOffsetY = Clamp(pCD->m_ActionCutSceneCameraOffsetY + dLook[1], -MaxYZ[1], MaxYZ[1]);
				if (MaxYZ[2] > 2.99f)
					pCD->m_ActionCutSceneCameraOffsetX = M_FMod(pCD->m_ActionCutSceneCameraOffsetX + dLook[2] + 2.0f, 1.0f);
				pCD->m_ActionCutSceneCameraOffsetX = Clamp(pCD->m_ActionCutSceneCameraOffsetX + dLook[2], -MaxYZ[2], MaxYZ[2]);
			}
		}
		else if (pCD->m_iPlayer == -1)
		{
			CWObject_CoreData* pMounted = m_pWPhysState->Object_GetCD(pCD->m_iMountedObject);
			CWO_Character_ClientData* pCDMounted = (pMounted ? CWObject_Character::GetClientData(pMounted) : NULL);
			if (pCDMounted)
			{
				fp32 Before = pCDMounted->m_Control_Look_Wanted[2];
				pCDMounted->m_Control_Look_Wanted[2] = M_FMod(pCDMounted->m_Control_Look_Wanted[2] + dLook[2] + 2.0f, 1.0f);
				pCDMounted->Char_UpdateLook(pCDMounted->m_PredictFrameFrac);
				if (Sqr(Before - pCDMounted->m_Control_Look_Wanted[2]) > Sqr(0.00001f))
				{
					CMat4Dfp32 Mat;
					pCDMounted->m_Control_Look_Wanted.CreateMatrixFromAngles(0, Mat);
					m_pWPhysState->Object_SetRotation(pMounted->m_iObject, Mat);
				}
			}
		}
	}

	pCD->m_Control_DeltaLook = dLook;

//	fp32 ControlLookCacheZ = pCD->m_Control_Look_Wanted[2];

	if (pCD->m_CameraUtil.IsActive(CAMERAUTIL_MODE_MOUNTEDCAM))
	{
		pCD->m_CameraUtil.UpdateMountedLook(pCD->m_pWPhysState,pCD,dLook);
		if (pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_MOUNTEDCAM_NOLOOKUPDATE)
			return;
	}

	const int PhysType = CWObject_Character::Char_GetPhysType(m_pObj);

	int32 StateFlagsLo = pCD->m_AnimGraph2.GetStateFlagsLo();
	if(InNoClipMode(PhysType))
	{
		pCD->m_Control_Look_Wanted[1] = Min(0.249f, Max(-0.249f, pCD->m_Control_Look_Wanted[1] + dLook[1]));
		pCD->m_Control_Look_Wanted[2] = M_FMod(pCD->m_Control_Look_Wanted[2] + dLook[2] + 2.0f, 1.0f);
	}
	else if (!(StateFlagsLo & AG2_STATEFLAG_APPLYFIGHTCORRECTION))
	{
		if (IsAlive(PhysType) &&
		((!(AssistCamera(pCD) && AssistAim(pCD))) ||
		((AssistCamera(pCD) && AssistAim(pCD)) && (!(pCD->m_CrosshairInfo & PLAYER_CROSSHAIR_INSIDE_TARGETRING)))) &&
		(!IsForcedLook(pCD)))//pCD->m_bForceLookOnTarget == false))
			pCD->m_Control_Look_Wanted[1] = Min(PLAYER_CAMERAMAXTILT, Max(-PLAYER_CAMERAMAXTILT, pCD->m_Control_Look_Wanted[1] + dLook[1]));

		if (IsAlive(PhysType) && (!IsForcedLook(pCD)))//(pCD->m_bForceLookOnTarget == false))
			pCD->m_Control_Look_Wanted[2] = M_FMod(pCD->m_Control_Look_Wanted[2] + dLook[2] + 2.0f, 1.0f);
	}

	Char_UpdateLook(pCD->m_PredictFrameFrac);

	// Update Actioncutscene camera variables
	int32 CameraMode = pCD->m_ActionCutSceneCameraMode;
	if (((pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_MASK) == THIRDPERSONINTERACTIVE_MODE_NONE) &&	// don't offset the camera in interactive mode
		(((CameraMode & CActionCutsceneCamera::ACS_CAMERAMODE_ACTIVE) && 
		(pCD->m_ExtraFlags & (PLAYER_EXTRAFLAGS_THIRDPERSON))) ||
		(CWObject_Character::Char_GetControlMode(m_pObj) == PLAYER_CONTROLMODE_LEAN)))
	{
		pCD->m_ActionCutSceneCameraOffsetY = Clamp(pCD->m_ActionCutSceneCameraOffsetY + dLook[1], -ACSCAMERA_MAXPITCH, ACSCAMERA_MAXPITCH);
		pCD->m_ActionCutSceneCameraOffsetX = Clamp(pCD->m_ActionCutSceneCameraOffsetX + dLook[2], -ACSCAMERA_MAXYAW, ACSCAMERA_MAXYAW);
		// Update to camutil
		pCD->m_CameraUtil.SetCamOffsets(pCD->m_ActionCutSceneCameraOffsetX, pCD->m_ActionCutSceneCameraOffsetY);
	}

	// Update angular error
	
	if (!(m_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOLOOK))
	{
		int32 AnimPhysMoveType = (int32)pCD->m_AnimGraph2.GetAnimPhysMoveType();
		if ((AnimPhysMoveType == 0) && 
			(StateFlagsLo & AG2_STATEFLAG_USETURNCORRECTION) && 
			!m_AnimGraph2.GetPropertyBool(PROPERTY_BOOL_DISABLETURNCORRECTION))
		{
			fp32 NewAngle = pCD->m_TurnCorrectionTargetAngle - dLook[2];
			fp32 MaxOffset = fp32(pCD->m_AnimGraph2.GetMaxBodyOffset()) * (1.0f / 360.0f);
//			fp32 Diff = pCD->m_TurnCorrectionTargetAngle;
			pCD->m_TurnCorrectionTargetAngle = Clamp(NewAngle, -MaxOffset, MaxOffset);

			// Vert test (for tunnel mount thingie..)
			NewAngle = pCD->m_TurnCorrectionTargetAngleVert - dLook[1];
			pCD->m_TurnCorrectionTargetAngleVert = Clamp(NewAngle, -0.2f, 0.2f);
		}
	}

	{
/*
		fp32 LastSpecialRotation = ((CVec3Dfp32&)pCD->m_Camera_LastSpecialRotation).k[0];
		fp32 CurSpecialRotation = ((CVec3Dfp32&)pCD->m_Camera_CurSpecialRotation).k[0];
		
		fp32 SpecialRotation = LERP(LastSpecialRotation, CurSpecialRotation, pCD->m_PredictFrameFrac);
		fp32 SpecialTilt = (SpecialRotation * PLAYER_SPECIALROTATION_RANGE);
*/
	
//							if ((M_Fabs(dLook[1]) > 0.001f) || (M_Fabs(pCD->m_Control_Look[1]) < PLAYER_AUTOTILT_LIMIT))
		
		if ((M_Fabs(dLook[1]) > 0.001f) || 
			((pCD->m_Control_Look[1] < PLAYER_AUTOTILT_ANGLE + 0.005f) &&
			(pCD->m_Control_Look[1] > PLAYER_AUTOTILT_ANGLE - 0.005f)))
			pCD->m_Control_AutoTiltCountDown = 10;
	}
}


void CWO_Character_ClientData::Char_ProcessControl_Move(const CVec3Dfp32& _Move)
{
	MSCOPESHORT(CWO_Character_ClientData::Char_ProcessControl_Look);
	CWO_Character_ClientData* pCD = this;
	pCD->m_Control_Move = _Move;

	// Make the player move slower in 3rd person interactive mode
	//if (pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_DIALOGUE)
	if (!(pCD->m_WeaponZoomState & WEAPON_ZOOMSTATE_OFF))
	{
		pCD->m_Control_Move[0] = Min(pCD->m_Control_Move[0], 0.4f);
		pCD->m_Control_Move[0] = Max(pCD->m_Control_Move[0], -0.4f);
		pCD->m_Control_Move[1] = Min(pCD->m_Control_Move[1], 0.4f);
		pCD->m_Control_Move[1] = Max(pCD->m_Control_Move[1], -0.4f);
	}


	fp32 Dir = 0.0f;
	if(pCD->m_Control_Move[0] != 0.0f && pCD->m_Control_Move[0] != 0.0f)
		Dir = CVec3Dfp32::AngleFromVector(pCD->m_Control_Move[0], pCD->m_Control_Move[1]);
	else
	{
		fp32 LookZ = pCD->m_Control_Look[2];
		LookZ = FRAC(LookZ);
		if (LookZ > 0.5f)
			LookZ -= 1.0f;

		CVec3Dfp32 Move;
		Move = m_pObj->GetMoveVelocity();
		if (Sqr(Move.k[0]) + Sqr(Move.k[0]) > 0.0f)
		{
			fp32 MoveAngleZ =  -(CVec3Dfp32::AngleFromVector(Move.k[0], Move.k[1]));
			MoveAngleZ = FRAC(MoveAngleZ);
			if (MoveAngleZ > 0.5f)
				MoveAngleZ -= 1.0f;

			Dir = FRAC(2.0f + MoveAngleZ -  LookZ);
		}
		else
		{
			Dir = FRAC(2.0f - LookZ);
		}
	}
	pCD->m_MoveDir = (uint8)(Dir * 256);
}


void CWO_Character_ClientData::UpdateButtonPressTicks()
{
	MSCOPESHORT(CWO_Character_ClientData::UpdateButtonPressTicks);
	uint nTicksToHold = RoundToInt(0.3f * m_pWPhysState->GetGameTicksPerSecond());

	m_Control_LastHold = m_Control_Hold;

	// (32 control bits  x  4 bit counter)
	uint32 Pressed = m_Control_Press;
	for (uint i = 0, CurrBit = 1; i < 16; i++)
	{
		uint c[2] = { m_Control_PressTicks[i] & 15, m_Control_PressTicks[i] >> 4 };
		for (uint j = 0; j < 2; j++, CurrBit <<= 1)
		{
			m_Control_Click &= ~CurrBit;
			if (Pressed & CurrBit)
			{
				c[j] = MinMT(c[j] + 1, 15);
				if (c[j] >= nTicksToHold)
				{
				//	if (m_pWPhysState->IsServer() && !(m_Control_Hold & CurrBit))
				//		M_TRACEALWAYS("[%d] hold detected: %d\n", m_GameTick, i*2+j);
					m_Control_Hold |= CurrBit;
				}
			}
			else
			{
				m_Control_Hold &= ~CurrBit;
				if (c[j] > 0 && c[j] < nTicksToHold)
				{
				//	if (m_pWPhysState->IsServer())
				//		M_TRACEALWAYS("[%d] click detected: %d\n", m_GameTick, i*2+j);
					m_Control_Click |= CurrBit;
				}
				c[j] = 0;
			}
		}
		m_Control_PressTicks[i] = c[0] | (c[1] << 4);
	}
}

void CWO_Character_ClientData::ResetButtonPressTicks(uint _ControlBits)
{
	for (uint i = 0; i < 16; i++)
	{
		if (_ControlBits & 3)
		{
			uint c0 = ((_ControlBits & 1) ? ~0 : (m_Control_PressTicks[i] >> 0)) & 15;
			uint c1 = ((_ControlBits & 2) ? ~0 : (m_Control_PressTicks[i] >> 4)) & 15;
			m_Control_PressTicks[i] = c0 | (c1 << 4);
		}
		_ControlBits >>= 2;
	}
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Temp placement of CWO_Inventory_Info
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWO_Inventory_Info::CWO_Inventory_Info()
{
	Clear();
	m_iGUITaggedItem = -1;
}

void CWO_Inventory_Info::Clear()
{
	m_lInventory.Clear();
}

void CWO_Inventory_Info::CopyFrom(const CWO_Inventory_Info& _Info)
{
	TAP_RCD<const CInventoryItem> pOtherInventory = _Info.m_lInventory;
	m_lInventory.SetLen(pOtherInventory.Len());
	TAP_RCD<CInventoryItem> pInventory = m_lInventory;
	for (int32 i = 0; i < pOtherInventory.Len(); i++)
	{
		pInventory[i] = pOtherInventory[i];
	}
	m_iGUITaggedItem = _Info.m_iGUITaggedItem;
}

// First test, just add....
int CWO_Inventory_Info::GetInsertPosition(int32 _Identifier)
{
	// Find position to insert into
	TAP_RCD<const CInventoryItem> pInventory = m_lInventory;
	int32 iSearchStart = 0;
	int32 iSearchEnd = pInventory.Len() - 1;
	bool bAdd = true;
	while (iSearchEnd >= iSearchStart)
	{
		int32 Middle = (iSearchEnd - iSearchStart);
		if (Middle > 1)
		{
			Middle = Middle / 2;
			int32 Identifier = pInventory[iSearchStart + Middle].m_Identifier - _Identifier;
			if (Identifier == 0)
			{
				// Already have this item in the list so just break
				bAdd = false;
				break;
			}
			else if (Identifier < 0)
			{
				// Given identifier is larger than middle
				iSearchStart += Middle + 1;
			}
			else
			{
				// Given identifier is less than middle
				iSearchEnd = iSearchStart + Middle - 1;
			}
		}
		else
		{
			// Only 1 - 2 left
			int32 Identifier = pInventory[iSearchStart].m_Identifier - _Identifier;
			if (iSearchStart == iSearchEnd)
			{
				bAdd = Identifier != 0;
				// If we're adding check where, if larger add after
				if (bAdd && Identifier < 0)
					iSearchStart++;
				break;
			}
			if (Identifier == 0)
			{
				bAdd = false;
				break;
			}
			// Check if less than searchstart, then insert there
			if (Identifier > 0)
				break;
			if (pInventory[iSearchEnd].m_Identifier == _Identifier)
			{
				bAdd = false;
				break;
			}
			// Add after end
			iSearchStart = pInventory[iSearchEnd].m_Identifier - _Identifier < 0 ? iSearchEnd + 1 : iSearchEnd;
			break;
		}
	}

	return bAdd ? iSearchStart : -1;
}

void CWO_Inventory_Info::AddItem(CRPG_Object_Item* _pItem, int32 _Tick, int16 _Flags)
{
	// Sort by identifier
	if (!_pItem)
		return;

	int iPos = GetInsertPosition(_pItem->m_Identifier);

	if (iPos != -1)
	{
		CInventoryItem Item;
		Item.m_ItemName = _pItem->GetItemName();
		Item.m_ItemDescription = _pItem->GetItemDesc();
		Item.m_ActivationTick = _Tick;
		Item.m_Identifier = _pItem->m_Identifier;
		Item.m_NumItems = _pItem->m_NumItems;
		Item.m_iMissionSurface = _pItem->m_iIconSurface;
		Item.m_iActivationSurface = _pItem->m_iActivationSurface;
		Item.m_Flags = _Flags;
		Item.m_iJournalImage = _pItem->m_JournalImage;
		Item.m_iAmount = _pItem->GetAmmo(NULL) + (_pItem->GetMagazines() * _pItem->GetMaxAmmo());

		m_lInventory.Insert(iPos,Item);
	}
	return;
}

/*void CWO_Inventory_Info::DebugPrint()
{
	TAP_RCD<const CInventoryItem> pInventory = m_lInventory;
	CStr InvItems("Items: ");
	for (int32 i = 0; i < pInventory.Len(); i++)
	{
		InvItems += CStrF(" %d",pInventory[i].m_Identifier);
	}
	ConOutL(InvItems);
}*/

int CWO_Inventory_Info::OnCreateClientUpdate2(uint8* _pData, const CWO_Inventory_Info& _Mirror, uint8& _SharedFlags) const
{
	// Mirror should be identical, first remove any item's that are already in here (256 max for now..)
	uint8* pStart = _pData;
	TArray<uint8> lRemove;
	TArray<uint8> lAdd;
	TAP_RCD<const CInventoryItem> pMirrorInventory = _Mirror.m_lInventory;
	TAP_RCD<const CInventoryItem> pInventory = m_lInventory;
	int32 iOffset = 0;
	int32 i = 0;
	for (i = 0; i < pInventory.Len(); i++)
	{
		// For each item in the inventory check if it exists in the mirror
		while (true)
		{
			int32 iMirror = i + iOffset;
			if (iMirror < 0 || iMirror >= pMirrorInventory.Len())
			{
				// Needs to be added
				lAdd.Add(i);
				break;
			}
			else
			{
				int32 Identifier = pMirrorInventory[iMirror].m_Identifier - pInventory[i].m_Identifier;
				if (Identifier == 0)
				{
					// this one is ok, break
					break;
				}
				else if (Identifier < 0)
				{
					// Current mirror is less than the current it means that that item needs to be removed
					lRemove.Add(iMirror);
					iOffset++;
					continue;
				}
				else
				{
					// Current mirror identifier is larger than current, add it
					lAdd.Add(i);
					iOffset--;
					break;
				}
			}
		}
	}
	// Remove stuff outside range
	for (int32 j = i + iOffset; j < pMirrorInventory.Len(); j++)
		lRemove.Add(j);

	TAP_RCD<const uint8> pWork = lRemove;
	if (pWork.Len())
	{
		_SharedFlags |= M_Bit(6);
		uint8 Len = pWork.Len();
		PTR_PUTUINT8(_pData,Len);
		for (int32 i = 0; i < pWork.Len(); i++)
		{
			PTR_PUTUINT8(_pData,pWork[i]);
		}
	}
	pWork = lAdd;
	if (pWork.Len())
	{
		_SharedFlags |= M_Bit(7);
		uint8 Len = pWork.Len();
		PTR_PUTUINT8(_pData,Len);
		for (int32 i = 0; i < pWork.Len(); i++)
		{
			PTR_PUTSTR(_pData,pInventory[pWork[i]].m_ItemName);
			PTR_PUTSTR(_pData,pInventory[pWork[i]].m_ItemDescription);
			PTR_PUTINT32(_pData,pInventory[pWork[i]].m_ActivationTick);
			PTR_PUTINT32(_pData,pInventory[pWork[i]].m_Identifier);
			PTR_PUTINT16(_pData,pInventory[pWork[i]].m_NumItems);
			PTR_PUTINT16(_pData,pInventory[pWork[i]].m_iMissionSurface);
			PTR_PUTINT16(_pData,pInventory[pWork[i]].m_iActivationSurface);
			PTR_PUTINT16(_pData,pInventory[pWork[i]].m_Flags);
		}
	}

	return _pData - pStart;
}

int CWO_Inventory_Info::OnClientUpdate(const uint8* _pData, const uint8& _SharedFlags)
{
	// Since this won't change at all on the client we don't have to worry about order
	const uint8* pStart = _pData;
	if (_SharedFlags & M_Bit(6))
	{
		// Got some stuff to remove
		uint8 RemoveLen;
		PTR_GETUINT8(_pData,RemoveLen);
		for (int32 i = 0; i < RemoveLen; i++)
		{
			uint8 iRemove;
			PTR_GETUINT8(_pData,iRemove);
			m_lInventory.Del(iRemove - i);
		}
	}

	if (_SharedFlags & M_Bit(7))
	{
		// Some items to add
		uint8 AddLen;
		PTR_GETUINT8(_pData,AddLen);
		for (int32 i = 0; i < AddLen; i++)
		{
			CInventoryItem Item;
			// Add it
			PTR_GETSTR(_pData,Item.m_ItemName);
			PTR_GETSTR(_pData,Item.m_ItemDescription);
			PTR_GETINT32(_pData,Item.m_ActivationTick);
			PTR_GETINT32(_pData,Item.m_Identifier);
			PTR_GETINT16(_pData,Item.m_NumItems);
			PTR_GETINT16(_pData,Item.m_iMissionSurface);
			PTR_GETINT16(_pData,Item.m_iActivationSurface);
			PTR_GETINT16(_pData,Item.m_Flags);
			int iPos = GetInsertPosition(Item.m_Identifier);
			if (iPos != -1)
				m_lInventory.Insert(iPos, Item);
		}
	}

	return _pData - pStart;
}

int32 CWO_Inventory_Info::GetNumMatches(const int16 _Flags) const
{
	int32 Len = 0;
	TAP_RCD<const CInventoryItem> pInventory = m_lInventory;
	for (int32 i = 0; i < pInventory.Len(); i++)
	{
		if (pInventory[i].m_Flags & _Flags)
			Len++;
	}

	return Len;
}

const CWO_Inventory_Info::CInventoryItem* CWO_Inventory_Info::GetNth(const int16 _Flags, const int32 _Index) const
{
	int32 Len = 0;
	TAP_RCD<const CInventoryItem> pInventory = m_lInventory;
	for (int32 i = 0; i < pInventory.Len(); i++)
	{
		if (pInventory[i].m_Flags & _Flags)
		{
			if (Len == _Index)
				return &pInventory[i];
			Len++;
		}
	}

	return NULL;
}

int16 CWO_Inventory_Info::GetNextActivationSurface(int32& _iStartIndex, int32 iLastTick, int32& _PickupTick)
{
	// Search from given startindex forward until we find an active surface above lasttick (which is currenttick - duration)
	TAP_RCD<const CInventoryItem> pInventory = m_lInventory;
	if (_iStartIndex >= pInventory.Len())
		return 0;

	for (; _iStartIndex < pInventory.Len(); _iStartIndex++)
	{
		if (pInventory[_iStartIndex].m_iActivationSurface && pInventory[_iStartIndex].m_ActivationTick > iLastTick)
		{
			_PickupTick = pInventory[_iStartIndex].m_ActivationTick;
			return pInventory[_iStartIndex++].m_iActivationSurface;
		}
	}
	return 0;
}


void CWO_Inventory_Info::CInventoryItem::Clear()
{
	m_ItemName.Clear();
	m_ItemDescription.Clear();
	m_ActivationTick = -1;
	m_Identifier = -1;
	m_NumItems = 0;
	m_iMissionSurface = 0;
	m_iActivationSurface = 0;
	m_iJournalImage = 0;
	m_iAmount = 0;
	m_Flags = 0;
}

CWO_Inventory_Info::CInventoryItem& CWO_Inventory_Info::CInventoryItem::operator= (const CInventoryItem& _Item)
{
	m_ItemName = _Item.m_ItemName;
	m_ItemDescription = _Item.m_ItemDescription;
	m_ActivationTick = _Item.m_ActivationTick;
	m_Identifier = _Item.m_Identifier;
	m_NumItems = _Item.m_NumItems;
	m_iMissionSurface = _Item.m_iMissionSurface;
	m_iActivationSurface = _Item.m_iActivationSurface;
	m_Flags = _Item.m_Flags;
	m_iJournalImage = _Item.m_iJournalImage;
	m_iAmount = _Item.m_iAmount;
	return *this;
}

void CWO_Inventory_Info::Write(CCFile* _pFile) const
{
	// Save all inventory items
	TAP_RCD<const CInventoryItem> lItems = m_lInventory;
	// Write num items
	_pFile->WriteLE((int32)lItems.Len());
	for (int32 i = 0; i < lItems.Len(); i++)
	{
		lItems[i].m_ItemName.Write(_pFile);
		lItems[i].m_ItemDescription.Write(_pFile);
		// If not -1, show activation surface for (set amount of time?)
		_pFile->WriteLE(lItems[i].m_ActivationTick);
		// Unique item identifier
		_pFile->WriteLE(lItems[i].m_Identifier);
		_pFile->WriteLE(lItems[i].m_NumItems);
		_pFile->WriteLE(lItems[i].m_iMissionSurface);
		_pFile->WriteLE(lItems[i].m_iActivationSurface);
		_pFile->WriteLE(lItems[i].m_Flags);
		_pFile->WriteLE(lItems[i].m_iJournalImage);
		_pFile->WriteLE(lItems[i].m_iAmount);
	}
}

void CWO_Inventory_Info::Read(CCFile* _pFile)
{
	// Load all inventory items
	int32 Len;
	// Read num items
	_pFile->ReadLE(Len);
	m_lInventory.SetLen(Len);
	TAP_RCD<CInventoryItem> lItems = m_lInventory;
	for (int32 i = 0; i < lItems.Len(); i++)
	{
		lItems[i].m_ItemName.Read(_pFile);
		lItems[i].m_ItemDescription.Read(_pFile);
		// If not -1, show activation surface for (set amount of time?)
		_pFile->ReadLE(lItems[i].m_ActivationTick);
		// Unique item identifier
		_pFile->ReadLE(lItems[i].m_Identifier);
		_pFile->ReadLE(lItems[i].m_NumItems);
		_pFile->ReadLE(lItems[i].m_iMissionSurface);
		_pFile->ReadLE(lItems[i].m_iActivationSurface);
		_pFile->ReadLE(lItems[i].m_Flags);
		_pFile->ReadLE(lItems[i].m_iJournalImage);
		_pFile->ReadLE(lItems[i].m_iAmount);
	}
}

void CAutoVar_WeaponStatus::Clear()
{
	m_CurrentTimeOut = 0;
	m_LastSoundTick = 0;
	m_FireTimeOut = 0;
	m_iSoundActivatePlayer = 0;
	m_AmmoLoad = 0;
	m_Flags = 0;
}

bool CAutoVar_WeaponStatus::OkToFire(CWO_Character_ClientData* _pCD, CWorld_PhysState* _pWPhys)
{
	bool bAmmoOk = (m_Flags & WEAPONSTATUS_MELEEWEAPON) || (m_Flags & WEAPONSTATUS_DARKNESSDRAIN) ? _pCD->m_Darkness >= m_AmmoLoad : m_AmmoLoad > 0;
	return ((((m_Flags & WEAPONSTATUS_SINGLESHOT) && !(m_Flags & WEAPONSTATUS_TRIGGERPRESSED)) || 
		!(m_Flags & WEAPONSTATUS_SINGLESHOT)) && bAmmoOk && _pCD->m_GameTick > m_CurrentTimeOut);
}

void CAutoVar_WeaponStatus::Fire(CWO_Character_ClientData* _pCD, bool _bGodMode)
{
	m_CurrentTimeOut = _pCD->m_GameTick + m_FireTimeOut;
	if (!_bGodMode)
	{
		// Decrease darkness instead
		if (!(m_Flags & WEAPONSTATUS_DARKNESSDRAIN))
			m_AmmoLoad--;
	}
}

void CAutoVar_WeaponStatus::Pack(uint8 *&_pD, CMapData* _pMapData) const
{
	uint8& Flags = _pD[0];
	_pD++;
	Flags = m_Flags;
	if (m_Flags & WEAPONSTATUS_HASSOUND)
		PTR_PUTINT16(_pD,m_iSoundActivatePlayer);
	if (m_CurrentTimeOut > 0)
	{
		PTR_PUTINT32(_pD,m_CurrentTimeOut);
		Flags |= WEAPONSTATUS_HASTIMEOUT;
	}
	PTR_PUTINT16(_pD,m_FireTimeOut);
	PTR_PUTINT16(_pD,m_AmmoLoad);
}

void CAutoVar_WeaponStatus::Unpack(const uint8 *&_pD, CMapData* _pMapData)
{
	int8 Flags;
	PTR_GETINT8(_pD,Flags);
	m_Flags = Flags | (m_Flags & 0xe0);
	if (m_Flags & WEAPONSTATUS_HASSOUND)
	{
		PTR_GETINT16(_pD,m_iSoundActivatePlayer);
	}
	else
	{
		m_iSoundActivatePlayer = 0;
	}
	if (m_Flags & WEAPONSTATUS_HASTIMEOUT)
		PTR_GETINT32(_pD,m_CurrentTimeOut);

	PTR_GETINT16(_pD,m_FireTimeOut);
	PTR_GETINT16(_pD,m_AmmoLoad);
}

void CAutoVar_WeaponStatus::CopyFrom(const CAutoVar_WeaponStatus& _From)
{
	m_CurrentTimeOut = _From.m_CurrentTimeOut;
	m_LastSoundTick = _From.m_LastSoundTick;
	m_FireTimeOut = _From.m_FireTimeOut;
	m_iSoundActivatePlayer = _From.m_iSoundActivatePlayer;
	m_AmmoLoad = _From.m_AmmoLoad;
	m_Flags = _From.m_Flags;
}


void CAutoVar_DarknessLevel::Clear()
{
	m_Hearts = 0;
	m_Levels = 0;
}

void CAutoVar_DarknessLevel::CopyFrom(const CAutoVar_DarknessLevel& _From)
{
	m_Hearts = _From.m_Hearts;
	m_Levels = _From.m_Levels;
}

void CAutoVar_DarknessLevel::Pack(uint8 *&_pD, CMapData* _pMapData) const
{
	PTR_PUTINT16(_pD, m_Hearts);
	PTR_PUTINT8(_pD, m_Levels);
}

void CAutoVar_DarknessLevel::Unpack(const uint8 *&_pD, CMapData* _pMapData)
{
	PTR_GETINT16(_pD, m_Hearts);
	PTR_GETINT8(_pD, m_Levels);
}

void CAutoVar_DarknessLevel::OnDeltaSave(CCFile* _pFile)
{
	_pFile->WriteLE(m_Hearts);
	_pFile->WriteLE(m_Levels);
}

void CAutoVar_DarknessLevel::OnDeltaLoad(CCFile* _pFile)
{
	_pFile->ReadLE(m_Hearts);
	_pFile->ReadLE(m_Levels);
}

void CAutoVar_DarknessLevel::AddHeart()
{
	m_Hearts++;
}

void CAutoVar_DarknessLevel::SetLevel(uint _Level)
{
	m_Hearts = _Level * 20;
}

CAutoVar_DarknessLevel::EDarknessLevel CAutoVar_DarknessLevel::GetLevelEnum(uint _Power)
{
	if (_Power & PLAYER_DARKNESSMODE_POWER_DEMONARM)
		return LEVEL_DEMONARM;
	else if (_Power & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK)
		return LEVEL_CREEPINGDARK;
	else if (_Power & PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS)
		return LEVEL_ANCIENTWEAPONS;
	else if (_Power & PLAYER_DARKNESSMODE_POWER_BLACKHOLE)
		return LEVEL_BLACKHOLE;
	else if (_Power & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD)
		return LEVEL_SHIELD;
	else if (_Power & PLAYER_DARKNESSMODE_POWER_DARKNESSVISION)
		return LEVEL_VISION;
	else
		return LEVEL_UNKNOWN;
}

void CAutoVar_DarknessLevel::SetPowerLevel(uint _Power, uint _Level)
{
	uint Level = MinMT(uint(MAX_LEVEL), _Level);
	EDarknessLevel LevelEnum = GetLevelEnum(_Power);

	if (LevelEnum != LEVEL_UNKNOWN)
	{
		m_Levels &= ~(MAX_LEVEL << LevelEnum);
		m_Levels |= (Level << LevelEnum);
	}
}

uint CAutoVar_DarknessLevel::GetMaxDarkness()
{
	return uint(DARKNESS_INCREASE) + (GetLevel() * uint(DARKNESS_INCREASE));
}

uint CAutoVar_DarknessLevel::GetHearts()
{
	return m_Hearts;
}

uint CAutoVar_DarknessLevel::GetLevel()
{
	return MinMT(uint(HEARTS_MAXLEVEL), m_Hearts / uint(HEARTS_LEVEL));
}

uint CAutoVar_DarknessLevel::GetLevelHearts()
{
	return (m_Hearts - (GetLevel() * HEARTS_LEVEL));
}

uint CAutoVar_DarknessLevel::GetPowerLevel(uint _Power)
{
	EDarknessLevel LevelEnum = GetLevelEnum(_Power);
	if (LevelEnum != LEVEL_UNKNOWN)
		return (m_Levels >> LevelEnum) & ((1 << MAX_LEVEL)-1);
	return 0;
}
