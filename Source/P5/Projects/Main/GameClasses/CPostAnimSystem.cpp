/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			Post Animation system.

Author:			Jakob Ericsson

Copyright:		2005 Starbreeze Studios AB

Contents:		Post-processing of animations.

Comments:		

History:
051207:		Created file
060109:		Rewrote pretty much everything

\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "CPostAnimSystem.h"
#include "WObj_Char/WObj_CharDarkling_ClientData.h"
#include "WObj_CharClientData.h"
#include "WObj_Char.h"
#include "WObj_AI/AICore.h"

// This file should be moved to shared

// large weapon (two handed) values
#define LONGWEAPONLENGTH		30.0f
#define LONGWEAPONWIDTH			1.1f
#define LONGWEAPONHEIGHT		3.0f

// small weapon values
#define SHORTWEAPONLENGTH		30.0f
#define SHORTWEAPONHEIGHT		2.5f
#define SHORTWEAPONWIDTH		0.0f

#define HOVERVALUE				3.666f // the distance from the foot-node to the ground (rough)
#define PLAYERLOWERSPEED		25.0f

#define HANDIKBLENDINSPEED			2.5f
#define HANDIKBLENDINSPEED_FAST		5.0f
#define HANDIKBLENDINSPEED_POSE_1H	2.0f
#define HANDIKBLENDINSPEED_POSE_2H	1.75f // 2.3
 
// recoil defines
#define RECOILMOVMAXLIGHT_1		0.5f
#define RECOILROTMAXLIGHT_1		0.0175f
#define RECOILMOVUPLIGHT_1		0.0f
#define RECOILMOVMAXLIGHT_2		0.25f
#define RECOILROTMAXLIGHT_2		0.0175f
#define RECOILMOVUPLIGHT_2		0.65f
#define RECOILMOVMAXLIGHT_3		0.35f
#define RECOILROTMAXLIGHT_3		0.0075f
#define RECOILMOVUPLIGHT_3		0.3f
#define RECOILTIMELIGHT			0.025f
#define RECOILRECOVETIMELIGHT	0.1f

#define RECOILMOVMAXMEDIUM_1	1.0f
#define RECOILROTMAXMEDIUM_1	0.025f
#define RECOILMOVUPMEDIUM_1		0.1f
#define RECOILMOVMAXMEDIUM_2	0.5f
#define RECOILROTMAXMEDIUM_2	0.035f
#define RECOILMOVUPMEDIUM_2		0.5f
#define RECOILMOVMAXMEDIUM_3	0.7f
#define RECOILROTMAXMEDIUM_3	0.015f
#define RECOILMOVUPMEDIUM_3		0.75f
#define RECOILTIMEMEDIUM		0.025f
#define RECOILRECOVETIMEMEDIUM	0.2f

#define RECOILMOVMAXHEAVY_1		1.6f
#define RECOILROTMAXHEAVY_1		0.020f
#define RECOILMOVUPHEAVY_1		-0.1f
#define RECOILMOVMAXHEAVY_2		1.75f
#define RECOILROTMAXHEAVY_2		0.025f
#define RECOILMOVUPHEAVY_2		0.5f
#define RECOILMOVMAXHEAVY_3		1.0f
#define RECOILROTMAXHEAVY_3		0.025f
#define RECOILMOVUPHEAVY_3		1.25f
#define RECOILTIMEHEAVY			0.025f
#define RECOILRECOVETIMEHEAVY	0.25f

#define AIMROTATIONSPEEDWEAPON 0.5f
#define AIMROTATIONSPEEDHEAD 0.1f

#define CHANGEDWNORMALPOSETICKS 300
//#pragma optimize( "", off )
//#pragma inline_depth(0)
//#define USEJAKOBSDEBUGINFO

#define FEETBLENDSPEED 1.5f

enum
{
	class_Object_Lamp =			MHASH6('CWOb','ject','_','Obje','ct_','Lamp'),
	class_SwingDoor =           MHASH6('CWOb','ject','_','Swin','g','Door'),
	class_CreepingDarkEntity =MHASH7('CWOb','ject','_Cre','epin', 'gDar', 'kEnt','ity'),	
};

CPostAnimSystem::CPostAnimSystem()
{
	MACRO_GetSystemEnvironment(pEnv);
	m_DisableHumanFeetIK = pEnv->GetValuei("DISABLE_CHAR_PLAYER_IK", 0);
	m_DisableDarklingIK = pEnv->GetValuei("DISABLE_CHAR_DARKLING_IK", 0);
	m_bDisableHandIK = pEnv->GetValuei("DISABLE_HAND_IK", 0);

	ResetValues();
	//---------------------------------------------------------------------------------------------
	// Gun poses

	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL].m_Position = 0;
	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL].m_Rotation = 0;
	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL].m_LineCollisionMod = CVec3Dfp32(-3.0f, 0.0f, -2.0f);
	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL].m_SearchLenMod = 30.0f;

	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL_1].m_Position = CVec3Dfp32(0.0f, 3.0f, 2.0f);
	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL_1].m_Rotation = CVec3Dfp32(0.06f, 0.0f, 0.0f);
	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL_1].m_LineCollisionMod = CVec3Dfp32(-3.0f, 1.3f, -4.0f);
	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL_1].m_SearchLenMod = 30.0f;

	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL_2].m_Position = CVec3Dfp32(-1.5f, 2.5f, 1.25f);
	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL_2].m_Rotation = CVec3Dfp32(0.04f, 0.0f, 0.0f);
	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL_2].m_LineCollisionMod = CVec3Dfp32(-3.0f, 1.3f, -3.0f);
	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL_2].m_SearchLenMod = 30.0f;

	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL_ZOOM].m_Position = CVec3Dfp32(0.5f, 7.0f, 2.5f);
	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL_ZOOM].m_Rotation = CVec3Dfp32(0.01f, 0.0f, 0.0f);
	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL_ZOOM].m_LineCollisionMod = CVec3Dfp32(-3.0f, -0.5f, -4.0f);
	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL_ZOOM].m_SearchLenMod = 30.0f;

	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL_LAST].m_Position = CVec3Dfp32(-0.5f, 2.25f, 0.0f);
	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL_LAST].m_Rotation = CVec3Dfp32(0.01f, 0.0f, 0.0f);
	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL_LAST].m_LineCollisionMod = CVec3Dfp32(-3.0f, 0.2f, -2.0f);
	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL_LAST].m_SearchLenMod = 30.0f;

	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_T].m_Position = CVec3Dfp32(-4.0f, 8.85f, 4.0f);
	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_T].m_Rotation = CVec3Dfp32(0.10f, 0.0f, 0.0f);
	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_T].m_LineCollisionMod = CVec3Dfp32(-3.0f, -1.2f, -6.2f);
	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_T].m_SearchLenMod = 0;

	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_GANGSTER].m_Position = CVec3Dfp32(-4.5f, 6.0f, 7.0f);
	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_GANGSTER].m_Rotation = CVec3Dfp32(0.08f, 0.0f, 0.0f);
	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_GANGSTER].m_LineCollisionMod = CVec3Dfp32(-3.0f, 0.0f, -3.0f);
	m_WeaponPoses_1H[WEAPON_POSETEMPLATE_GANGSTER].m_SearchLenMod = 8.0f;

	m_WeaponPoses_1H[WEAPON_POSE_GUNSLINGER_DUEL].m_Position = CVec3Dfp32(-6.2f, 0.0f, 0.0f);
	m_WeaponPoses_1H[WEAPON_POSE_GUNSLINGER_DUEL].m_Rotation = CVec3Dfp32(0.20f, -0.25f, 0.0f);
	m_WeaponPoses_1H[WEAPON_POSE_GUNSLINGER_DUEL].m_LineCollisionMod = 0;
	m_WeaponPoses_1H[WEAPON_POSE_GUNSLINGER_DUEL].m_SearchLenMod = 0;

	// ------------------------------------------------------------------------------------------------------

	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_NORMAL].m_Position = CVec3Dfp32(0.0f, 0.0f, 0.0f);
	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_NORMAL].m_Rotation = 0;
	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_NORMAL].m_LineCollisionMod =  CVec3Dfp32(50.0f, 0.0f, 0.0f);
	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_NORMAL].m_SearchLenMod = 40;

	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_NORMAL_1].m_Position = CVec3Dfp32(0.5f, 1.0f, 0.5f);
	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_NORMAL_1].m_Rotation = CVec3Dfp32(0.0f, 0.0f, 0.0f);
	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_NORMAL_1].m_LineCollisionMod =  CVec3Dfp32(50.0f, 0.0f, 0.0f);
	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_NORMAL_1].m_SearchLenMod = 40;

	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_NORMAL_2].m_Position = CVec3Dfp32(1.0f, 1.0f, 0.5f);
	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_NORMAL_2].m_Rotation = CVec3Dfp32(0.001f, 0.0f, 0.0f);
	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_NORMAL_2].m_LineCollisionMod =  CVec3Dfp32(50.0f, 0.0f, 0.0f);
	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_NORMAL_2].m_SearchLenMod = 40;

	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_NORMAL_ZOOM].m_Position = CVec3Dfp32(2.0f, 2.5f, 0.5f);
	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_NORMAL_ZOOM].m_Rotation = CVec3Dfp32(0.0f, 0.0f, 0.0f);
	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_NORMAL_ZOOM].m_LineCollisionMod =  CVec3Dfp32(50.0f, 0.0f, 0.0f);
	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_NORMAL_ZOOM].m_SearchLenMod = 40;

	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_NORMAL_LAST].m_Position = CVec3Dfp32(1.0f, 1.2f, 1.0f);
	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_NORMAL_LAST].m_Rotation = CVec3Dfp32(0.015f, 0.0f, 0.0f);
	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_NORMAL_LAST].m_LineCollisionMod =  CVec3Dfp32(50.0f, 0.0f, 0.0f);
	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_NORMAL_LAST].m_SearchLenMod = 40;

	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_T].m_Position = CVec3Dfp32(3.0f, 7.5f, 0.0f);
	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_T].m_Rotation = CVec3Dfp32(0.02f, 0.0f, 0.0f);
	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_T].m_LineCollisionMod = CVec3Dfp32(50.0f, 0.0f, 0.0f);
	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_T].m_SearchLenMod =50;

	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_GANGSTER].m_Position = CVec3Dfp32(-5.0f, -2.5f, 8.5f);
	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_GANGSTER].m_Rotation = CVec3Dfp32(0.175, 0.0f, 0.0f);
	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_GANGSTER].m_LineCollisionMod = CVec3Dfp32(20.0f, 2.25f, -2.25f);
	m_WeaponPoses_2H[WEAPON_POSETEMPLATE_GANGSTER].m_SearchLenMod = 20.0f;

	m_WeaponPoses_2H[WEAPON_POSE_GUNSLINGER_DUEL].m_Position = CVec3Dfp32(-2.0f, 0.0f, -6.0f);
	m_WeaponPoses_2H[WEAPON_POSE_GUNSLINGER_DUEL].m_Rotation = CVec3Dfp32(0.0, 0.15f, 0.15f);
	m_WeaponPoses_2H[WEAPON_POSE_GUNSLINGER_DUEL].m_LineCollisionMod = 0;
	m_WeaponPoses_2H[WEAPON_POSE_GUNSLINGER_DUEL].m_SearchLenMod = 0;

	// special crouch position for 2 handed weapons

	m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_SPECIAL_2HCROUCH].m_Position = CVec3Dfp32(-3.0f, -3.0f, -5.0f);
	m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_SPECIAL_2HCROUCH].m_Rotation = CVec3Dfp32(0.05f, -0.01f, 0.15f);
	m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_SPECIAL_2HCROUCH].m_LineCollisionMod = 0;
	m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_SPECIAL_2HCROUCH].m_SearchLenMod = 0;

	// 2h opening door. left door and right door
	m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_SPECIAL_2HDOOR_RIGHT].m_Position = CVec3Dfp32(6.0f, 0.0f, -2.0f);
	m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_SPECIAL_2HDOOR_RIGHT].m_Rotation = CVec3Dfp32(0.0f, -0.075f, 0.20f);
	m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_SPECIAL_2HDOOR_RIGHT].m_LineCollisionMod = 0;
	m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_SPECIAL_2HDOOR_RIGHT].m_SearchLenMod = 0;

	m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_SPECIAL_2HDOOR_LEFT].m_Position = CVec3Dfp32(4.0f, 2.0f, -4.0f);
	m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_SPECIAL_2HDOOR_LEFT].m_Rotation = CVec3Dfp32(-0.0f, -0.19f, 0.0f);
	m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_SPECIAL_2HDOOR_LEFT].m_LineCollisionMod = 0;
	m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_SPECIAL_2HDOOR_LEFT].m_SearchLenMod = 0;

	//---------------------------------------------------------------------------------------------
}

void CPostAnimSystem::ResetValues()
{
	m_fDeltaTime = 100000.0f;
	m_LastTime = CMTime();

	m_IKFlags = 0;
	m_IKFlags2 = 0;

	//----------------------------------------------------------------
	// feet Ik

	m_Diffr = 0.0f; // for lowering characters

	m_CurRFPAPoint = 0.0f;
	m_CurLFPAPoint = 0.0f;
	m_CurLFrontFootPAPoint = 0.0f;
	m_CurRFrontFootPAPoint = 0.0f;

	m_RightFootDestination.Unit();
	m_LeftFootDestination.Unit();
	m_RightFrontFootDestination.Unit();
	m_LeftFrontFootDestination.Unit();

	m_MatRightFootAnim.Unit();
	m_MatLeftFootAnim.Unit();
	m_MatFrontRightFootAnim.Unit();
	m_MatFrontLeftFootAnim.Unit();

	m_StairMoveType = FOOT_MOVEMENT_UNDEFINED;

	m_LastPos = 0.0f;
	m_CharSpeed = 0.0f;

	m_LeftFrontFootRotation = 0.0f;
	m_RightFrontFootRotation = 0.0f;
	
	// for darkling back rotation over convex corners
	//m_DarklingBackRotation = 0.0f;
	//m_DarklingBackRotationSpeed = 0.5;
	//m_DarklingBackMaxRotation = 0.25f;

	//-------------------------------------------------------------
	// hand IK
	
	m_WeaponHoldType = WEAPONHOLD_TYPE_NOWEAPON;

	m_AimPointLeft = 0;
	m_AimTimeLeft = 0.0f;
	m_AimTimeStoreLeft = 0.0f;
	m_AimBlendTimeLeft = 0.75f;
	m_fAimAngleWeaponLeft = 0.0f;	

	m_AimPointRight = 0;
	m_AimTimeRight = 0.0f;
	m_AimTimeStoreRight = 0.0f;
	m_AimBlendTimeRight = 0.75f;
	m_fAimAngleWeaponRight = 0.0f;	

	m_fAimAngleHead = 0.0f;

	// collision detection store-values
	m_LeftFrontFootTarget = 0;
	m_RightFrontFootTarget = 0;
	
	m_LeftBackFootTarget = 0;
	m_RightBackFootTarget = 0;
	
	// stored weapon collision info
	m_RightWeaponColInfo.m_CollisionPoint = 0.0f;
	m_RightWeaponColInfo.m_LenFromWeaponToColPt = 0.0f;
	m_RightWeaponColInfo.m_CollsionNormal = 0.0f;
	m_RightWeaponColInfo.m_UnInterpolLenWeponToPt = 0.0f;
	m_RightWeaponColInfo.m_NormalDependentRotation = 0.0f;
	m_RightWeaponColInfo.m_BarrelDependentRotation = 0.0f;

	m_LeftWeaponColInfo.m_CollisionPoint = 0.0f;
	m_LeftWeaponColInfo.m_LenFromWeaponToColPt = 0.0f;
	m_LeftWeaponColInfo.m_CollsionNormal = 0.0f;
	m_LeftWeaponColInfo.m_UnInterpolLenWeponToPt = 0.0f;
	m_LeftWeaponColInfo.m_NormalDependentRotation = 0.0f;
	m_LeftWeaponColInfo.m_BarrelDependentRotation = 0.0f;

	m_CameraLookMat.Unit();

	m_MatWeapon.Unit();
	m_MatWeapon2.Unit();

	m_LastRightLookVec = 0.0f;
	m_fTurningRotAngle = 0.0f;
	m_fTurningRotAngle1 = 0.0f;
	m_RecoilTimerRight = 0.0f;
	m_RecoilTimerLeft = 0.0f;
	m_LeftRecoilSubType = 0;
	m_RightRecoilSubType = 0;
	m_RecoilCamMod = 0;
	
	m_CurrentWeaponBlendTypeLeft = WEAPONBLEND_TYPE_NONE;
	m_LastWeaponBlendTypeLeft = WEAPONBLEND_TYPE_NONE;
	m_CurrentWeaponBlendTypeRight = WEAPONBLEND_TYPE_NONE;
	m_LastWeaponBlendTypeRight = WEAPONBLEND_TYPE_NONE;
	m_GunPoseBlendValueLeft = 0.0f;
	m_GunPoseBlendValueRight = 0.0f;
	m_LeftAimInAndOutBlendVal = 1.0f;
	m_RightAimInAndOutBlendVal = 1.0f;

	m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_BLENDFROMRIGHT] = m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL];
	m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_BLENDFROMLEFT] = m_WeaponPoses_1H[WEAPON_POSETEMPLATE_NORMAL];

	m_WeaponPoseLeft = WEAPON_POSETEMPLATE_NORMAL;
	m_WeaponPoseRight = WEAPON_POSETEMPLATE_NORMAL;
	m_LastWeaponPoseLeft = WEAPON_POSETEMPLATE_NORMAL;
	m_LastWeaponPoseRight = WEAPON_POSETEMPLATE_NORMAL;
	m_CurrentDestWeaponPoseLeft = WEAPON_POSETEMPLATE_NORMAL;
	m_CurrentDestWeaponPoseRight = WEAPON_POSETEMPLATE_NORMAL;

	m_DWNormalChangeAtTick = 0;
	m_RandomDWNormalPose = 0;

	m_LeftHandBlendVal = 0.0f;
	m_RightHandBlendVal = 0.0f;

	m_AbsAimTargetPos = 0;
    m_CurIntrpAimTargetPos = 0;
	m_MovementLineColMod = 0;
	m_AILookDir = 0;	

	m_iPlayerObject = -1;

	m_LastCameraLookVec = CVec3Dfp32(0.0, 0.0f, -1.0f);
	m_PlayerRotationDiff = CVec3Dfp32(0.0, 0.0f, -1.0f);
        
    m_TargetBlendOut = 1.0f;	
    m_Anim_iCurIKLookMode = 0;
	m_Anim_iLastIKLookMode = 0;
	m_Anim_IKLookBlend = 0;
	m_ForcedAimingMode = 0;	

	m_FeetBlendVal = 0.0f;
	m_FrontFeetBlendVal = 0.0f;

	m_RightFootSin = 0.0f;
	m_LeftFootSin = 0.0f;
}

CPostAnimSystem::~CPostAnimSystem()
{
}

static fp32 Sinc(fp32 _x)
{
	MAUTOSTRIP(Sinc, 0.0f);
	return M_Sin((_x - 0.5f)*_PI)*0.5f + 0.5f;
}

void CPostAnimSystem::EvalAimIK(CXR_SkeletonInstance* _pSkelInstance, fp32 _IPTime, CMat4Dfp32& _MatBody,  CMat4Dfp32& _MatLook, const CVec3Dfp32& _UpVector)
{
	CMat4Dfp32 IMB, RH;
	_MatBody.InverseOrthogonal(IMB);
	CQuatfp32 QUnit, QRel, Q;
	QUnit.Unit();

	CQuatfp32 QRelIK[5];
	bool lbQRelIK[5];
	memset(lbQRelIK, 0, sizeof(lbQRelIK));

	const fp32 BlendValue = (m_Anim_IKLookBlend - _IPTime * PLAYER_ANIMBLENDSPEEDVISUAL);
	for(int i = 0; i < 2; i++)
	{
		int iMode;
		if(i == 0)
		{
			iMode = m_Anim_iCurIKLookMode;
			if(m_ForcedAimingMode != 0)
				iMode = m_ForcedAimingMode - 1;
		}
		else
			iMode = m_Anim_iLastIKLookMode;

		fp32 Blend = 0;
		if(i > 0)
			Blend = Sinc(Max(1.0f - BlendValue, 0.0f));
		else
			Blend = Sinc(Max(BlendValue, 0.0f));

		switch(iMode)
		{
		case AIMING_NORMAL:
			// From spine and up
			{
				CMat4Dfp32 ZLook = _MatLook;
				_UpVector.SetRow(ZLook, 2);
				ZLook.RecreateMatrix(2, 0); // Z Look-rotation
				ZLook.Multiply(IMB, RH);
				QRel.Create(RH); // QRel == Z Look-rotation

				CQuatfp32 Q2, Q3;
				_MatLook.Multiply(IMB, RH);
				Q2.Create(RH); // Q2 = Look-rotation

				QRel.Lerp(Q2, 0.75f, Q3); // 25% Z Look-rotation + 75% Look-rotation
				QUnit.Lerp(Q3, 1.0f / 4, Q); // Split over 4 bones

				APPLYQUAT(Q, QRelIK[0], lbQRelIK[0], Blend); // Apply on the two spine-bones
				APPLYQUAT(Q, QRelIK[1], lbQRelIK[1], Blend);

				//									QUnit.Interpolate(QRel, Q, 1.0f / 2);
				Q.Multiply(Q, Q);
				Q.Inverse();
				Q2.Multiply(Q, QRel);
				QUnit.Lerp(QRel, 1.0f / 2, Q); // Split rest of rotation over 2 bones
				APPLYQUAT(Q, QRelIK[2], lbQRelIK[2], Blend);
				APPLYQUAT(Q, QRelIK[3], lbQRelIK[3], Blend);
				break;
			}
		case AIMING_BODY:
			// Only spine (for aiming)
			_MatLook.Multiply(IMB, RH);
			QRel.Create(RH);

			QUnit.Lerp(QRel, 1.0f / 2, Q);
			APPLYQUAT(Q, QRelIK[0], lbQRelIK[0], Blend);
			APPLYQUAT(Q, QRelIK[1], lbQRelIK[1], Blend);
			break;

		case AIMING_CROUCHBODY:
			_MatLook.Multiply(IMB, RH);
			QRel.Create(RH);
			APPLYQUAT(QRel, QRelIK[1], lbQRelIK[1], Blend);
			break;

		case AIMING_HEAD:
			// Only neck and head
			{
				_MatLook.Multiply(IMB, RH);
				QRel.Create(RH);

				QUnit.Lerp(QRel, 1.0f / 2, Q);
				APPLYQUAT(Q, QRelIK[2], lbQRelIK[2], Blend);
				APPLYQUAT(Q, QRelIK[3], lbQRelIK[3], Blend);
				break;
			}
		case AIMING_FULLBODY:
			// Whole body (no aiming)
			{
				_MatLook.Multiply(IMB, RH);
				QRel.Create(RH);
				APPLYQUAT(QRel, QRelIK[4], lbQRelIK[4], Blend);
				break;
			}
		case AIMING_MINIGUN:
			// Only spine (for aiming)
			{
				CMat4Dfp32 Look = _MatLook;
				Look.k[0][2] = Max(Look.k[0][2], -0.1f);
				Look.RecreateMatrix(0, 2);
				Look.Multiply(IMB, RH);
				QRel.Create(RH);

				QUnit.Lerp(QRel, 1.0f / 2, Q);
				APPLYQUAT(Q, QRelIK[0], lbQRelIK[0], Blend);
				APPLYQUAT(Q, QRelIK[1], lbQRelIK[1], Blend);

				CQuatfp32 Q2, Q3;
				_MatLook.Multiply(IMB, RH);
				Q2.Create(RH);

				Q = QRel;
				Q.Inverse();
				Q2.Multiply(Q, QRel);
				QUnit.Lerp(QRel, 1.0f / 2, Q);
				APPLYQUAT(Q, QRelIK[2], lbQRelIK[2], Blend);
				APPLYQUAT(Q, QRelIK[3], lbQRelIK[3], Blend);
				break;
			}
		case AIMING_CARRYBODY:
			{
				CMat4Dfp32 ZLook = _MatLook;
				CVec3Dfp32(0,0,1).SetRow(ZLook, 2);
				ZLook.RecreateMatrix(2, 0); // Z Look-rotation
				ZLook.Multiply(IMB, RH);
				QRel.Create(RH); // QRel == Z Look-rotation

				// Divide z rotation on the spine
				QUnit.Lerp(QRel, 0.5f, Q);
				APPLYQUAT(Q, QRelIK[0], lbQRelIK[0], Blend);
				APPLYQUAT(Q, QRelIK[1], lbQRelIK[1], Blend);

				// Put the y look on the head/neck bones
				CQuatfp32 QYLook,Q;
				_MatLook.Multiply(IMB, RH);
				QYLook.Create(RH);
				QRel.Inverse();
				QYLook.Multiply(QRel,Q);
				QUnit.Lerp(Q, 0.5f, QRel);
				APPLYQUAT(QRel, QRelIK[2], lbQRelIK[2], Blend);
				APPLYQUAT(QRel, QRelIK[3], lbQRelIK[3], Blend);
				break;
			}
		case AIMING_UNARMED:
			{
				CMat4Dfp32 ZLook = _MatLook;
				_UpVector.SetRow(ZLook, 2);
				ZLook.RecreateMatrix(2, 0); // Z Look-rotation
				ZLook.Multiply(IMB, RH);
				QRel.Create(RH); // QRel == Z Look-rotation

				CQuatfp32 Q2, Q3;
				_MatLook.Multiply(IMB, RH);
				Q2.Create(RH); // Q2 = Look-rotation

				QUnit.Lerp(QRel, 1.0f / 4, Q); // Split over 4 bones

				APPLYQUAT(Q, QRelIK[0], lbQRelIK[0], Blend); // Apply on the two spine-bones
				APPLYQUAT(Q, QRelIK[1], lbQRelIK[1], Blend);

				Q.Multiply(Q, Q);
				Q.Inverse();
				Q2.Multiply(Q, QRel);
				QUnit.Lerp(QRel, 1.0f / 2, Q); // Split rest of rotation over 2 bones
				APPLYQUAT(Q, QRelIK[2], lbQRelIK[2], Blend);
				APPLYQUAT(Q, QRelIK[3], lbQRelIK[3], Blend);

				//									QUnit.Interpolate(QRel, Q, 1.0f / 2);
				/*CQuatfp32 QYLook,Q;
				_MatLook.Multiply(IMB, RH);
				QYLook.Create(RH);
				QRel.Inverse();
				QYLook.Multiply(QRel,Q);
				QUnit.Lerp(Q, 0.5f, QRel);
				APPLYQUAT(Q, QRelIK[2], lbQRelIK[2], Blend);
				APPLYQUAT(Q, QRelIK[3], lbQRelIK[3], Blend);*/
				break;
			}
		default:
			// None
			break;
		}

		if(m_Anim_iCurIKLookMode == m_Anim_iLastIKLookMode || m_Anim_IKLookBlend <= 0)
			break;
	}

	{
		if(lbQRelIK[4])
			RotateBoneAbsolute(QRelIK[4], _pSkelInstance, PLAYER_ROTTRACK_ROOT, 0);
		if(lbQRelIK[0])
			RotateBoneAbsolute(QRelIK[0], _pSkelInstance, PLAYER_ROTTRACK_SPINE, PLAYER_ROTTRACK_ROOT);
		if(lbQRelIK[1])
			RotateBoneAbsolute(QRelIK[1], _pSkelInstance, PLAYER_ROTTRACK_TORSO, PLAYER_ROTTRACK_SPINE);
		if(lbQRelIK[2])
			RotateBoneAbsolute(QRelIK[2], _pSkelInstance, PLAYER_ROTTRACK_NECK, PLAYER_ROTTRACK_TORSO);
		if(lbQRelIK[3])
			RotateBoneAbsolute(QRelIK[3], _pSkelInstance, PLAYER_ROTTRACK_HEAD, PLAYER_ROTTRACK_NECK);
	}
}


void CPostAnimSystem::RotateBoneAbsolute(const CQuatfp32 &_Q, CXR_SkeletonInstance *_pSkelInstance, int _iRotTrack, int _iParent)
{
	CMat4Dfp32 IParent, Rot, dParent, IdParent, M0, M1, M2;
	_pSkelInstance->GetBoneTransform(_iParent ).InverseOrthogonal(IParent);
	_pSkelInstance->GetBoneTransform(0).Multiply(IParent, dParent);
	dParent.InverseOrthogonal(IdParent);

	CMat4Dfp32 Local = _pSkelInstance->GetBoneLocalPos(_iRotTrack);

	// Rotate Quat in Parent space and apply to Local bone
	_Q.CreateMatrix(Rot);
	IdParent.Multiply3x3(Rot, M0);
	Local.Multiply3x3(M0, M1);
	M1.Multiply3x3(dParent, M2);

	CVec3Dfp32::GetRow(M2, 0).SetRow(Local, 0);
	CVec3Dfp32::GetRow(M2, 1).SetRow(Local, 1);
	CVec3Dfp32::GetRow(M2, 2).SetRow(Local, 2);

	_pSkelInstance->SetBoneLocalPos(_iRotTrack, &Local);
}


void CPostAnimSystem::EvalHandsOnWeapon(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CWObject_CoreData* _pObj, CXR_Model* _pModel, CWorld_PhysState* _pWPhysState)
{
#ifndef OGIER

	if (m_bDisableHandIK)
		return;

	CWO_Character_ClientData *pCDFirst;
	pCDFirst =  CWObject_Character::GetClientData(_pObj);
	m_iPlayerObject = _pObj->m_iObject;
	if(!pCDFirst)
		return;

	if(_pSkel && _pSkelInstance)
	{
		// matrix set in charanim.cpp
		m_CameraLookMat.GetRow(3) = _pSkel->m_lNodes[PLAYER_ROTTRACK_CAMERA].m_LocalCenter * _pSkelInstance->m_pBoneTransform[_pSkel->m_lNodes[PLAYER_ROTTRACK_CAMERA].m_iNodeParent];
		m_PlayerRotationDiff = m_CameraLookMat.GetRow(0) - m_LastCameraLookVec;
	}
	
	// store zoom mode since CD is not passed to all functions
	if(pCDFirst->m_WeaponZoomState == WEAPON_ZOOMSTATE_IN || pCDFirst->m_WeaponZoomState == WEAPON_ZOOMSTATE_ON)
	{
		m_IKFlags |= IKFLAGS_INWEAPONZOOMMODE;
		m_IKFlags2 &= ~IKFLAGS2_INWEAPONZOOMOUTMODE;
	}
	else if(pCDFirst->m_WeaponZoomState == WEAPON_ZOOMSTATE_OUT)
	{
		m_IKFlags &= ~IKFLAGS_INWEAPONZOOMMODE;
		m_IKFlags2 |= IKFLAGS2_INWEAPONZOOMOUTMODE;
	}
	else
	{
		m_IKFlags &= ~IKFLAGS_INWEAPONZOOMMODE;
		m_IKFlags2 &= ~IKFLAGS2_INWEAPONZOOMOUTMODE;
	}

	// ---------------------------------------------------------------------------------------------------

	CMat4Dfp32 MatWeapon;
	CXR_Model* pModelWeapon;
	CXR_AnimState AnimStateWeapon;

	// is he holding any weapon?
	if(!(pCDFirst->m_Item0_Model.GetModel0_RenderInfo(_pWPhysState->GetMapData(), NULL, _pSkelInstance, _pSkel, m_LastTime, 
		AnimStateWeapon, MatWeapon, pModelWeapon, NULL)))
	{
		m_WeaponHoldType = WEAPONHOLD_TYPE_NOWEAPON;
		m_IKFlags &= ~IKFLAGS_RELEASELEFT_GRAB;
		m_IKFlags &= ~IKFLAGS_RELEASERIGHT_GRAB;
		m_RandomDWNormalPose = 0;
		return;
	}

	CXR_Skeleton* pSkelItem = (CXR_Skeleton*)pModelWeapon->GetParam(MODEL_PARAM_SKELETON);
	if(!pSkelItem)
		return;

	const CXR_SkeletonAttachPoint *pAttachWeapon = pSkelItem->GetAttachPoint(4);

	
	//----------------------------------------------------------------------------------------
	// check what type of weapon the char is holding

	if(pAttachWeapon && !pCDFirst->m_Item1_Flags) // weapon has second hand-attach point, that means, is two handed weapon
	{
		m_WeaponHoldType = WEAPONHOLD_TYPE_TWOHANDED;

		m_IKFlags &= ~IKFLAGS_RELEASELEFT_GRAB;
		m_IKFlags &= ~IKFLAGS_RELEASERIGHT_GRAB;		
	}
 	else if(pCDFirst->m_iPlayer == -1 && !pCDFirst->m_Item1_Flags) // is AI and has no second gun
	{
		m_WeaponHoldType = WEAPONHOLD_TYPE_ONLY_1_1HGUN;
	}
	else if(pCDFirst->m_Item1_Flags && pCDFirst->m_iPlayer != -1) // is Player and has second gun
	{
		//TODO: this needs to be set in draw weapon animation
		MarkWeaponChanged();
		m_WeaponHoldType = WEAPONHOLD_TYPE_DUALWIELD;
	}
	else
		return; // something wrong

	//--------------------------------------------------------------------------------------
	// blend in and blend out of hand IK is set via animgraph

	if(pCDFirst->m_AnimGraph2.GetStateFlagsHiCombined() & AG2_STATEFLAGHI_IKSYSTEM_IGNOREHANDS)
	{
		m_IKFlags |= IKFLAGS_DISABLE_RIGHTAIM;
		m_IKFlags |= IKFLAGS_DISABLE_LEFTAIM;
		m_RandomDWNormalPose = 0;
		m_WeaponPoseLeft = WEAPON_POSETEMPLATE_NORMAL;
		m_WeaponPoseRight = WEAPON_POSETEMPLATE_NORMAL;
		
		if(m_CurrentWeaponBlendTypeLeft != WEAPONBLEND_TYPE_NONE)
			m_CurrentWeaponBlendTypeLeft = WEAPONBLEND_TYPE_OUT;
		
		if(m_CurrentWeaponBlendTypeRight != WEAPONBLEND_TYPE_NONE)
			m_CurrentWeaponBlendTypeRight = WEAPONBLEND_TYPE_OUT;

		if(m_CurrentWeaponBlendTypeLeft == WEAPONBLEND_TYPE_NONE && m_CurrentWeaponBlendTypeRight == WEAPONBLEND_TYPE_NONE)
		{
			m_WeaponHoldType = WEAPONHOLD_TYPE_NOWEAPON;
			return;
		}
	}
	else if(pCDFirst->m_iPlayer == -1) // is AI Player
	{
		// a bit of a special case when holding 1_1HGun. We can always have full in on that one. Wont effect reloading
		if((pCDFirst->m_AnimGraph2.GetStateFlagsHiCombined() & AG2_STATEFLAGHI_IKSYSTEM_ENABLELEFTHANDIK) || m_WeaponHoldType == WEAPONHOLD_TYPE_ONLY_1_1HGUN)
		{
			if(m_WeaponHoldType == WEAPONHOLD_TYPE_ONLY_1_1HGUN)
			{
				if(m_CurrentWeaponBlendTypeLeft != WEAPONBLEND_TYPE_NONE)
					m_CurrentWeaponBlendTypeLeft = WEAPONBLEND_TYPE_OUT;
			}
			else
			{
				if(m_CurrentWeaponBlendTypeLeft != WEAPONBLEND_TYPE_FULL_IN)
					m_CurrentWeaponBlendTypeLeft = WEAPONBLEND_TYPE_IN;
			}
			

			if(m_CurrentWeaponBlendTypeRight != WEAPONBLEND_TYPE_FULL_IN)
				m_CurrentWeaponBlendTypeRight = WEAPONBLEND_TYPE_IN;
		}
		else
		{
			if(m_CurrentWeaponBlendTypeLeft != WEAPONBLEND_TYPE_NONE)
				m_CurrentWeaponBlendTypeLeft = WEAPONBLEND_TYPE_OUT;

			if(m_CurrentWeaponBlendTypeRight != WEAPONBLEND_TYPE_NONE)
				m_CurrentWeaponBlendTypeRight = WEAPONBLEND_TYPE_OUT;

			if(m_CurrentWeaponBlendTypeLeft == WEAPONBLEND_TYPE_NONE && m_CurrentWeaponBlendTypeRight == WEAPONBLEND_TYPE_NONE)
			{
				m_WeaponHoldType = WEAPONHOLD_TYPE_NOWEAPON;
				return;
			}
		}		
	}
	else if(pCDFirst->m_iPlayer != -1)
	{
		// a players left hand is toggled with "AG2_STATEFLAGHI_IKSYSTEM_IGNOREHANDS"
		if(m_CurrentWeaponBlendTypeLeft != WEAPONBLEND_TYPE_FULL_IN)
			m_CurrentWeaponBlendTypeLeft = WEAPONBLEND_TYPE_IN;

		if(m_CurrentWeaponBlendTypeRight != WEAPONBLEND_TYPE_FULL_IN)
			m_CurrentWeaponBlendTypeRight = WEAPONBLEND_TYPE_IN;
	}


	//------------------------------------------------------------------------
	// get all the collsion data (player and client only. AI gets it from the refresh calls)
	if(pCDFirst->m_iPlayer != -1 && _pWPhysState->IsClient() && m_CurrentWeaponBlendTypeRight != WEAPONBLEND_TYPE_OUT)
	{	
		if(m_WeaponHoldType != WEAPONHOLD_TYPE_TWOHANDED) // for 2h, this is done via char refresh
			GatherWeaponWallCollisionData(-1, _pWPhysState);
		GatherAimCollisionData(_pWPhysState, pCDFirst->m_AimTarget, (pCDFirst->m_iPlayer != -1));
	}		
		
	//---------------------------------------------------------------------------------------------------
	// chaange armrotations when looking around to get it less staic

	m_fAdditionalRightWeaponMovement = 0.0f;
	m_fAdditionalLeftWeaponMovement = 0.0f;

	if(((_pObj->m_Data[PLAYER_DATAINDEX_CONTROL] & 0x0000f000) >> 12) == 2) // PLAYER_PHYS_CROUCH = 2, PLAYER_PHYS_DEAD = 3
		m_IKFlags |= IKFLAGS_CROUCHING;
	else
		m_IKFlags &= ~IKFLAGS_CROUCHING;
	
	if(m_PlayerRotationDiff.LengthSqr() > 0.000625f) // 0.025f
		m_LastCameraLookVec = m_CameraLookMat.GetRow(0);

	if(pCDFirst->m_iPlayer != -1 && !(m_IKFlags & IKFLAGS_OPENDOOR))
		ApplyLookDependantWeaponMotion(_pSkel, _pSkelInstance, _pWPhysState);
		
	//------------------------------------------------------------------------------------------------------
	// Setup the nodes according to the collision information
	bool bAdjustLeft = true;
	bool bAdjustRight = true;

	if(!UpdateWeaponMatrices(_pSkel, _pSkelInstance, _pWPhysState, pCDFirst, m_MatWeapon, m_MatWeapon2))
	{
		bAdjustLeft = false;
		bAdjustRight = false;
	}
	
	// if theres nothing to do, just setup the left hand and return
	if(!bAdjustLeft && !bAdjustRight && !(m_IKFlags & IKFLAGS_RECOILLEFTRUNNING) && !(m_IKFlags & IKFLAGS_RECOILRIGHTRUNNING))
	{
		m_CurrentWeaponBlendTypeLeft  = WEAPONBLEND_TYPE_NONE;
		m_CurrentWeaponBlendTypeRight = WEAPONBLEND_TYPE_NONE;
		return;
	}

	if(pCDFirst->m_iPlayer != -1 && ((m_WeaponPoseLeft > WEAPON_POSETEMPLATE_NORMAL_LAST) || (m_WeaponPoseRight > WEAPON_POSETEMPLATE_NORMAL_LAST)))
		pCDFirst->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_GUNPLAYDISABLED, true); // Disable gunplay while IK-rotating guns
	else if(!(m_IKFlags & IKFLAGS_RECOILRIGHTRUNNING) && !(m_IKFlags & IKFLAGS_RECOILLEFTRUNNING))
		pCDFirst->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_GUNPLAYDISABLED, false);
	
	if(pCDFirst->m_iPlayer != -1)
		SetupGunPosePositions(_pSkel, _pSkelInstance, _pWPhysState);
	
	//------------------------------------------------------------------------
	// apply aim to weapon, if supplied
	
	// if AI and in fighting mode, target player
	if(pCDFirst->m_iPlayer == -1)
	{
		pCDFirst->m_AimTarget = pCDFirst->m_iEyeLookObj;
		if((pCDFirst->m_AnimGraph2.GetStateFlagsLo()  &AG2_STATEFLAG_BEHAVIORACTIVE) && pCDFirst->m_AimTarget != 0)
		{
			m_IKFlags &= ~IKFLAGS_DISABLE_LEFTAIM;
			m_IKFlags &= ~IKFLAGS_DISABLE_RIGHTAIM;
		}
		else
		{
			m_IKFlags |= IKFLAGS_DISABLE_LEFTAIM;
			m_IKFlags |= IKFLAGS_DISABLE_RIGHTAIM;
		}
	}
	
	// this could maybe be done better
	// We actually need the unupdated weaponmatrices
	{
		CMat4Dfp32 MatRightTemp;
		CMat4Dfp32 MatLeftTemp;
		MatRightTemp = m_MatWeapon;
		MatLeftTemp = m_MatWeapon2;
		UpdateWeaponMatrices(_pSkel, _pSkelInstance, _pWPhysState, pCDFirst, m_MatWeapon, m_MatWeapon2);
		SetupNodesForAim(_pSkel, _pSkelInstance, _pWPhysState, pCDFirst);
		m_MatWeapon = MatRightTemp;
		m_MatWeapon2 = MatLeftTemp;
	}
	
	//------------------------------------------------------------------------------------------------------
	// Do the IK

	bool bIsTacShotgun = false;

	// need to get these values again, since rotations and movement has occured
	if(pCDFirst->m_Item0_Model.GetModel0_RenderInfo(_pWPhysState->GetMapData(), NULL, _pSkelInstance, _pSkel, m_LastTime, 
		AnimStateWeapon, MatWeapon, pModelWeapon, _pWPhysState))
	{
		CXR_Skeleton* pSkelItem = (CXR_Skeleton*)pModelWeapon->GetParam(MODEL_PARAM_SKELETON);
		if(pSkelItem)
		{	
			CMat4Dfp32 MatLeftHandTarget;
			CMat4Dfp32 MatRightHandTarget;

			MatRightHandTarget = _pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_RHAND];
			MatRightHandTarget.GetRow(3)= _pSkel->m_lNodes[PLAYER_ROTTRACK_RHANDATTACH].m_LocalCenter * _pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_RHAND];

			if(m_WeaponHoldType == WEAPONHOLD_TYPE_TWOHANDED)
			{
				const CXR_SkeletonAttachPoint *pAttachWeapon = pSkelItem->GetAttachPoint(4);

				if(pAttachWeapon)
				{
					// match left hand rotations with right hand rotations
					RotateBoneAroundAxis(-m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_BLENDFROMRIGHT].m_Rotation.k[0], &MatWeapon.GetRow(0), PLAYER_ROTTRACK_LHAND, _pSkel, _pSkelInstance, _pWPhysState, false);
					RotateBoneAroundAxis(m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_BLENDFROMRIGHT].m_Rotation.k[1], &MatWeapon.GetRow(1), PLAYER_ROTTRACK_LHAND, _pSkel, _pSkelInstance, _pWPhysState, false);
					RotateBoneAroundAxis(m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_BLENDFROMRIGHT].m_Rotation.k[2], &MatWeapon.GetRow(2), PLAYER_ROTTRACK_LHAND, _pSkel, _pSkelInstance, _pWPhysState, false);
					RotateBoneAroundAxis(m_fTurningRotAngle,  &m_CameraLookMat.GetRow(2), PLAYER_ROTTRACK_LHAND, _pSkel, _pSkelInstance, _pWPhysState, false);
					
					if (AnimStateWeapon.m_pSkeletonInst && pAttachWeapon->m_iNode && (pAttachWeapon->m_iNode < AnimStateWeapon.m_pSkeletonInst->m_nBoneTransform))
					{
						// this is the only way to know if im holding a shotgun! :(
						bIsTacShotgun = true;

						MatLeftHandTarget = _pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_LHAND];
						const CMat4Dfp32& WeaponAttachMat = AnimStateWeapon.m_pSkeletonInst->GetBoneTransform( pAttachWeapon->m_iNode );
						// TODO: this only affects the position, not the rotation...
						MatLeftHandTarget.GetRow(3) = pAttachWeapon->m_LocalPos.GetRow(3) * WeaponAttachMat;
					}
					else
					{
						MatLeftHandTarget = _pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_LHAND];
						MatLeftHandTarget.GetRow(3) = pAttachWeapon->m_LocalPos.GetRow(3) * MatWeapon;
					}
				}
				else // failsafe
				{
					MatLeftHandTarget = _pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_LHAND];
					MatLeftHandTarget.GetRow(3) = MatWeapon.GetRow(3);
				}
			}
			else 
			{
				MatLeftHandTarget = _pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_LHAND];
				MatLeftHandTarget.GetRow(3)= _pSkel->m_lNodes[PLAYER_ROTTRACK_LHANDATTACH].m_LocalCenter * _pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_LHAND];
			}

			//-------------------------------------------------------------------------------------
			// Recoil
			if(m_IKFlags & IKFLAGS_RECOILLEFTRUNNING)
			{
				CMat4Dfp32 CurWeaponMat;
				CXR_Model* pModelWeapon;
				CXR_AnimState AnimStateWeapon;
				if(pCDFirst->m_Item1_Model.GetModel0_RenderInfo(_pWPhysState->GetMapData(), NULL, _pSkelInstance, _pSkel, m_LastTime, 
					AnimStateWeapon, CurWeaponMat, pModelWeapon, NULL))
				{
					m_CurrentWeaponBlendTypeLeft = WEAPONBLEND_TYPE_FULL_IN;
					UpdateRecoil(&CurWeaponMat, &CurWeaponMat.GetRow(1), &MatLeftHandTarget, true);
					bAdjustLeft = true;
				}
			}

			if(m_IKFlags & IKFLAGS_RECOILRIGHTRUNNING)
			{
				CMat4Dfp32 CurWeaponMat;
				CXR_Model* pModelWeapon;
				CXR_AnimState AnimStateWeapon;
				if(pCDFirst->m_Item0_Model.GetModel0_RenderInfo(_pWPhysState->GetMapData(), NULL, _pSkelInstance, _pSkel, m_LastTime, 
					AnimStateWeapon, CurWeaponMat, pModelWeapon, NULL))
				{
					m_CurrentWeaponBlendTypeRight = WEAPONBLEND_TYPE_FULL_IN;
					UpdateRecoil(&CurWeaponMat, &CurWeaponMat.GetRow(1), &MatRightHandTarget, false);
					bAdjustRight = true;
				}					
			}
			
			//-------------------------------------------------------------------------------------
			// do the IK-calls
			
			// well, we always want the elbow to point downwards somewhat, and when the arm is straight, there is
			// a problem with retrieving the actual direction of the elbow, so this can be passed on the the IK system
			// as a modifier

			if(bIsTacShotgun)
			{
				CVec3Dfp32 MoveBack = MatWeapon.GetRow(0) * -3.0f;
				MatLeftHandTarget.GetRow(3) += MoveBack;
				MatRightHandTarget.GetRow(3) += MoveBack;
			}

 			if(bAdjustLeft)
			{
				if(BlendHandNodes(CIKSystem::IK_MODE_LEFT_HAND, &MatLeftHandTarget, _pSkel, _pSkelInstance))
				{
					m_IKSolver.SetIKMode(CIKSystem::IK_MODE_LEFT_HAND);

					// usually too straight armd in dualwield to determin elbow direction
					CVec3Dfp32 ElbowDirLeft = 0;
					if((m_WeaponHoldType != WEAPONHOLD_TYPE_TWOHANDED))
					{
						ElbowDirLeft = m_CameraLookMat.GetRow(2) * -100.0f + m_CameraLookMat.GetRow(1) * 50.0f;
						m_IKSolver.DoDualHandIK(_pSkel, _pSkelInstance, &MatLeftHandTarget, _pWPhysState, pCDFirst, &ElbowDirLeft);
					}
					else
					{
						ElbowDirLeft = m_CameraLookMat.GetRow(2) * -100.0f + m_CameraLookMat.GetRow(1) * 50.0f;
						m_IKSolver.DoDualHandIK(_pSkel, _pSkelInstance, &MatLeftHandTarget, _pWPhysState, pCDFirst, &ElbowDirLeft);

						// this is a ful-hack. Since we have only handnode to rotate the hand with, and no extra bone between elbow and hand,
						// we need to hardcode som rotation on the elbow to make the skinning around the hand look acceptable.
						// I don't like it anymore than you do... /Jakob

						// Store hand matrix, Rotate elbow, Restore hand matrix
						CMat4Dfp32 StoredHandMat = _pSkelInstance->m_pBoneLocalPos[PLAYER_ROTTRACK_LHAND];
						CVec3Dfp32 ElbowPos;
						ElbowPos = _pSkel->m_lNodes[PLAYER_ROTTRACK_LELBOW].m_LocalCenter * _pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_LARM];
						CVec3Dfp32 RotationAxis = (MatLeftHandTarget.GetRow(3) - ElbowPos);
						RotateBoneAroundAxis(-0.075f, &RotationAxis, PLAYER_ROTTRACK_LELBOW, _pSkel, _pSkelInstance, _pWPhysState, false);
						_pSkelInstance->m_pBoneLocalPos[PLAYER_ROTTRACK_LHAND] = StoredHandMat;			

					}
				}
			}

			if(bAdjustRight)
			{
				if(BlendHandNodes(CIKSystem::IK_MODE_RIGHT_HAND, &MatRightHandTarget, _pSkel, _pSkelInstance))
				{
					m_IKSolver.SetIKMode(CIKSystem::IK_MODE_RIGHT_HAND);

					CVec3Dfp32 ElbowDirRight = 0;
					if((m_WeaponHoldType == WEAPONHOLD_TYPE_DUALWIELD))
					{
						ElbowDirRight = m_CameraLookMat.GetRow(2) * -100.0f + m_CameraLookMat.GetRow(1) * -50.0f;
						m_IKSolver.DoDualHandIK(_pSkel, _pSkelInstance, &MatRightHandTarget, _pWPhysState, pCDFirst, &ElbowDirRight);
					}
					else
						m_IKSolver.DoDualHandIK(_pSkel, _pSkelInstance, &MatRightHandTarget, _pWPhysState, pCDFirst, NULL);	
				}
			}
		}
	}
#endif
}


//---------------------------------------------------------------------------
// set weapon to move when camera moves
//
void CPostAnimSystem::ApplyLookDependantWeaponMotion(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CWorld_PhysState* _pWPhysState)
{
#ifndef OGIER

	fp32 Speeder = 4.0f;
	fp32 SensSnap = 0.001f;

	if(m_IKFlags & IKFLAGS_CROUCHING) // crouching...
		SensSnap = 0.005f;

	// camera is VERY unsteady when crouching and walking forward. This means the aimvector will
	// alter too much to get a good estimate turnspeed. Might have to go with controller input insted
	// of aimvectors

#ifdef PLATFORM_WIN // should be "if gampad"
	Speeder = 2.0f;
#endif

	CVec3Dfp32 AimVector = m_CameraLookMat.GetRow(0);
	CVec3Dfp32 VecUp = _pSkelInstance->m_pBoneTransform[0].GetRow(2);

	fp32 fVertAngle = M_ACos(AimVector * VecUp);
	fVertAngle = fVertAngle / (_PI2); // convert to 0->1 angle

	fVertAngle -= 0.25;
	if(FloatIsNAN(fVertAngle))
		fVertAngle = 0.0f;

	if(fVertAngle < 0.0)
	{
		if(m_WeaponHoldType == WEAPONHOLD_TYPE_TWOHANDED)
		{
			m_fAdditionalRightWeaponMovement = fVertAngle * 10.0f;
		}
		else
		{
			m_fAdditionalRightWeaponMovement = fVertAngle * 15.0f;
			m_fAdditionalLeftWeaponMovement = fVertAngle * 15.0f;
		}
	}
	else if(fVertAngle > 0.0f)
	{
		if(m_WeaponHoldType == WEAPONHOLD_TYPE_TWOHANDED)
		{
			m_fAdditionalRightWeaponMovement = fVertAngle * -2.5f;
		}
		else
		{
			m_fAdditionalRightWeaponMovement = fVertAngle * -15.0f;
			m_fAdditionalLeftWeaponMovement = fVertAngle * -15.0f;
		}			
	}

	//if(!(m_IKFlags & IKFLAGS_RECOILLEFTRUNNING) && !(m_IKFlags & IKFLAGS_RECOILRIGHTRUNNING))
	{
		// aim-delay
		AimVector.k[2] = 0.0f;
		AimVector = AimVector.Normalize();
		m_LastRightLookVec.k[2] = 0.0f;
		m_LastRightLookVec = m_LastRightLookVec .Normalize();

		fp32 fRotAngle = M_ACos(AimVector * m_LastRightLookVec );
		fRotAngle = fRotAngle / (_PI2); // convert to 0->1 angle

		fRotAngle -=  0.25f; // to get +/- angles, check angle between current forward and last right
		if(FloatIsNAN(fRotAngle) || (fRotAngle > -SensSnap && fRotAngle < SensSnap))
			fRotAngle = 0.0f;

		if(fRotAngle < 0.0f) // turning right
		{
			if(m_fTurningRotAngle > 0.0f)
			{
				m_fTurningRotAngle -= m_fDeltaTime * 0.1f; //  speed up movement to center if changed direction
				m_fTurningRotAngle = Max(m_fTurningRotAngle, 0.0f);
			}

			m_fTurningRotAngle -= m_fDeltaTime * (-fRotAngle) * Speeder;
			m_fTurningRotAngle = Max(m_fTurningRotAngle, -0.025f);

			m_fAdditionalLeftWeaponMovement += (m_fTurningRotAngle * 50.0f); 
			m_fAdditionalRightWeaponMovement += (m_fTurningRotAngle * 100.0f);				
		}
		else if(fRotAngle > 0.0f)  // turning left
		{
			if(m_fTurningRotAngle < 0.0f)
			{
				m_fTurningRotAngle += m_fDeltaTime * 0.1f; //  speed up movement to center if changed direction
				m_fTurningRotAngle = Min(m_fTurningRotAngle, 0.0f);
			}

			m_fTurningRotAngle += m_fDeltaTime * fRotAngle * Speeder;
			m_fTurningRotAngle = Min(m_fTurningRotAngle, 0.025f);

			m_fAdditionalLeftWeaponMovement += (-m_fTurningRotAngle * 100.0f);
			m_fAdditionalRightWeaponMovement += (-m_fTurningRotAngle * 50.0f);
		}
		else
		{
			if(m_fTurningRotAngle < -0.001f) // recover from right turn
			{
				m_fTurningRotAngle += m_fDeltaTime * (-m_fTurningRotAngle * 5.0f);
				m_fTurningRotAngle = Min(m_fTurningRotAngle, 0.0f);

				m_fAdditionalLeftWeaponMovement += (m_fTurningRotAngle * 100.0f);
				m_fAdditionalLeftWeaponMovement = Min(m_fAdditionalLeftWeaponMovement, 0.0f);

				m_fAdditionalRightWeaponMovement += (m_fTurningRotAngle * 50.0f);
				m_fAdditionalRightWeaponMovement = Min(m_fAdditionalRightWeaponMovement, 0.0f);
			}
			else if(m_fTurningRotAngle > 0.001f) // recover from left turn
			{
				m_fTurningRotAngle -= m_fDeltaTime * (m_fTurningRotAngle * 5.0f);
				m_fTurningRotAngle = Max(m_fTurningRotAngle, 0.0f);

				m_fAdditionalLeftWeaponMovement += (-m_fTurningRotAngle * 100.0f);
				m_fAdditionalLeftWeaponMovement = Min(m_fAdditionalLeftWeaponMovement, 0.0f);
				m_fAdditionalRightWeaponMovement += (-m_fTurningRotAngle * 50.0f);
				m_fAdditionalRightWeaponMovement = Min(m_fAdditionalRightWeaponMovement, 0.0f);
			}
		}

		if(m_WeaponHoldType == WEAPONHOLD_TYPE_TWOHANDED)
		{
			RotateBoneAroundAxis(m_fTurningRotAngle,  &m_CameraLookMat.GetRow(2), PLAYER_ROTTRACK_RARM, _pSkel, _pSkelInstance, _pWPhysState, true);
		}
		else if(m_WeaponHoldType == WEAPONHOLD_TYPE_DUALWIELD)// 
		{
			RotateBoneAroundAxis(m_fTurningRotAngle,  &m_CameraLookMat.GetRow(2), PLAYER_ROTTRACK_RARM, _pSkel, _pSkelInstance, _pWPhysState, true);
			RotateBoneAroundAxis(m_fTurningRotAngle,  &m_CameraLookMat.GetRow(2), PLAYER_ROTTRACK_LARM, _pSkel, _pSkelInstance, _pWPhysState, true);
		}
	}

	VecUp.CrossProd(AimVector, m_LastRightLookVec );
	m_LastRightLookVec = m_LastRightLookVec .Normalize();
#endif
}

bool CPostAnimSystem::UpdateWeaponMatrices(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CWorld_PhysState* _pWPhysState, 
										   CWO_Character_ClientData *_pCDFirst, CMat4Dfp32 &_RightWeapon, CMat4Dfp32 &_LeftWeapon)
{
#ifndef OGIER

	CXR_Model* pModelWeapon;
	CXR_AnimState AnimStateWeapon;
	CXR_Model* pModelWeapon1;
	CXR_AnimState AnimStateWeapon1;

	if(_pCDFirst->m_Item0_Model.GetModel0_RenderInfo(_pWPhysState->GetMapData(), NULL, _pSkelInstance, _pSkel, m_LastTime, 
		AnimStateWeapon, _RightWeapon, pModelWeapon, NULL))
	{
		if(m_IKFlags & IKFLAGS_CHECKFORDWLEN)
		{
			CXR_Skeleton* pSkelItem = (CXR_Skeleton*)pModelWeapon->GetParam(MODEL_PARAM_SKELETON);
			if(pSkelItem)
			{
				// need to check if we're dualwielding long weapons
				const CXR_SkeletonAttachPoint *pAttachWeapon = pSkelItem->GetAttachPoint(1);
				if(pAttachWeapon)
				{
					if(pAttachWeapon->m_LocalPos.GetRow(3).k[0] > 8.0f)
						m_IKFlags |= IKFLAGS_LONGDUALWIELDWEAPON_RIGHT;
					else
						m_IKFlags &= ~IKFLAGS_LONGDUALWIELDWEAPON_RIGHT;
				}	
			}
		}

		// then check the left hand weapon, and react and setup
		if(!_pCDFirst->m_Item1_Model.GetModel0_RenderInfo(_pWPhysState->GetMapData(), NULL, _pSkelInstance, _pSkel, m_LastTime, 
			AnimStateWeapon1, _LeftWeapon, pModelWeapon1, NULL))
		{
			_LeftWeapon = _RightWeapon;
			_LeftWeapon.GetRow(3)= _pSkel->m_lNodes[PLAYER_ROTTRACK_LHANDATTACH].m_LocalCenter * _pSkelInstance->m_pBoneTransform[_pSkel->m_lNodes[PLAYER_ROTTRACK_LHANDATTACH].m_iNodeParent];
		}
		else
		{
			if(m_IKFlags & IKFLAGS_CHECKFORDWLEN)
			{
				CXR_Skeleton* pSkelItem = (CXR_Skeleton*)pModelWeapon1->GetParam(MODEL_PARAM_SKELETON);
				if(pSkelItem)
				{
					// need to check if we're dualwielding long weapons
					const CXR_SkeletonAttachPoint *pAttachWeapon = pSkelItem->GetAttachPoint(1);
					if(pAttachWeapon)
					{
						if(pAttachWeapon->m_LocalPos.GetRow(3).k[0] > 8.0f)
							m_IKFlags |= IKFLAGS_LONGDUALWIELDWEAPON_LEFT;
						else
							m_IKFlags &= ~IKFLAGS_LONGDUALWIELDWEAPON_LEFT;

						m_IKFlags &= ~IKFLAGS_CHECKFORDWLEN;
					}	
				}
			}
		}

		return true;
	}
#endif
	return false;
}

// help function
int CPostAnimSystem::GetRandomNormalPose()
{
	if(m_IKFlags & IKFLAGS_INWEAPONZOOMMODE)
		return WEAPON_POSETEMPLATE_NORMAL_ZOOM;

	if(m_RandomDWNormalPose < 2)
		return 0;

	int RetVal = int(MFloat_GetRand(m_RandomDWNormalPose)*5);

	// use the orig pose more often than the others
	if(RetVal == WEAPON_POSETEMPLATE_NORMAL_ZOOM)
		RetVal = WEAPON_POSETEMPLATE_NORMAL;
	else if(RetVal > WEAPON_POSETEMPLATE_NORMAL_LAST) 
		RetVal = WEAPON_POSETEMPLATE_NORMAL;

	return RetVal;
}
//--------------------------------------------------------------------------------------------
// Setting up hands for different poses
//
void CPostAnimSystem::SetupGunPosePositions(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CWorld_PhysState* _pWPhysState)
{
	CVec3Dfp32 UsePos;
	CVec3Dfp32 UseRot;
	uint16 *piLastWeaponPose;
	uint16 iCurWeaponPose;
	uint16 *piCurDestinationPose;
	fp32 *pGunBlendVal;
	int RotTrack;
	int RotTrackAtt;
	CMat4Dfp32 WeaponMat;
	SWeaponPose UseWeaponPoses[WEAPON_POSETEMPLATE_SIZE];
	SWeaponPose *pBlendFromPose;
	int8 Direct;
	fp32 BlendSpeed = HANDIKBLENDINSPEED;
	bool bPreventCameraClipping = false;
	SDW_WeaponCollisionInfo *pDW_WeaponColInfo;
	int iNumberOfIterations = 0;

	int UseRandomNormalPose = GetRandomNormalPose();

	switch(m_WeaponHoldType)
	{
	case WEAPONHOLD_TYPE_DUALWIELD:
		{			
			iNumberOfIterations = 2;
			for(int j = 0; j < WEAPON_POSETEMPLATE_SIZE; j++)
				UseWeaponPoses[j] = m_WeaponPoses_1H[j];
			
			// both hands in T mean looking into wall.
			if(m_WeaponPoseRight == WEAPON_POSETEMPLATE_T && m_WeaponPoseLeft == WEAPON_POSETEMPLATE_T)
			{
				if(m_IKFlags & IKFLAGS_OPENDOOR)
				{
					m_WeaponPoseLeft = WEAPON_POSE_GUNSLINGER_DUEL;
					m_WeaponPoseRight = WEAPON_POSE_GUNSLINGER_DUEL;
				}
				else
				{
					if(m_RightWeaponColInfo.m_UnInterpolLenWeponToPt == 0.0f)
						m_WeaponPoseRight = WEAPON_POSETEMPLATE_NORMAL;

					if(m_LeftWeaponColInfo.m_UnInterpolLenWeponToPt == 0.0f)
						m_WeaponPoseLeft = WEAPON_POSETEMPLATE_NORMAL;
				}
			}
			else
			{
				//-----------------------------------------------------------------------------------------------
				// random normal posing				

				if(_pWPhysState->GetGameTick() > m_DWNormalChangeAtTick && m_WeaponPoseRight != WEAPON_POSE_GUNSLINGER_DUEL && m_WeaponPoseLeft != WEAPON_POSE_GUNSLINGER_DUEL)
				{
					m_DWNormalChangeAtTick = _pWPhysState->GetGameTick() + (CHANGEDWNORMALPOSETICKS + TruncToInt(MFloat_GetRand(m_RandomDWNormalPose*0xCAB)*5.0f));
					m_RandomDWNormalPose++;
					UseRandomNormalPose = GetRandomNormalPose();
				}
			}

			if(m_WeaponPoseRight < WEAPON_POSETEMPLATE_NORMAL_LAST+1)
				m_WeaponPoseRight = UseRandomNormalPose;

			if(m_WeaponPoseLeft < WEAPON_POSETEMPLATE_NORMAL_LAST+1)
				m_WeaponPoseLeft = UseRandomNormalPose;

			if(m_IKFlags & IKFLAGS_CROUCHING)
			{
				for(int k = WEAPON_POSETEMPLATE_NORMAL_LAST+1; k < WEAPON_POSETEMPLATE_SIZE; k++)
					UseWeaponPoses[k].m_Position.k[2] *= 0.75f;

				UseWeaponPoses[WEAPON_POSETEMPLATE_NORMAL_ZOOM].m_Position.k[2] *= 0.5f;
			}

		}
		break;
	
	case WEAPONHOLD_TYPE_TWOHANDED:
		{
			iNumberOfIterations = 1;

			//-----------------------------------------------------------------------------------------------
			// random normal posing

			if(_pWPhysState->GetGameTick() > m_DWNormalChangeAtTick && m_WeaponPoseRight != WEAPON_POSE_GUNSLINGER_DUEL && m_WeaponPoseLeft != WEAPON_POSE_GUNSLINGER_DUEL)
			{
				m_DWNormalChangeAtTick = _pWPhysState->GetGameTick() + (CHANGEDWNORMALPOSETICKS + TruncToInt(MFloat_GetRand(m_RandomDWNormalPose*0xCAB)*5.0f));
				m_RandomDWNormalPose++;
				UseRandomNormalPose = GetRandomNormalPose();
			}
			
			//-----------------------------------------------------------------------------------------------

			// dont prevent camera clipping of going from one normal to another
			if(m_IKFlags & IKFLAGS_OPENDOOR)
				m_WeaponPoseRight = WEAPON_POSE_GUNSLINGER_DUEL;
			else if(m_WeaponPoseRight < WEAPON_POSETEMPLATE_NORMAL_LAST+1)
				m_WeaponPoseRight = UseRandomNormalPose;
			else if(m_WeaponPoseRight == WEAPON_POSETEMPLATE_GANGSTER && (m_RightWeaponColInfo.m_UnInterpolLenWeponToPt != 0.0f))
				m_WeaponPoseRight = UseRandomNormalPose;
			else
				bPreventCameraClipping = true;
				
			for(int j = 0; j < WEAPON_POSETEMPLATE_SIZE; j++)
				UseWeaponPoses[j] = m_WeaponPoses_2H[j];

			if( m_IKFlags & IKFLAGS_CROUCHING)
				UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL] = m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_SPECIAL_2HCROUCH];
				
		}
	    break;

	case WEAPONHOLD_TYPE_ONLY_1_1HGUN:
		// no support yet
	    return;
	
	default:
	    return;
	}
	
	//TODO: this is getting ugly...
	fp32 StoredK1Rotation = UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Rotation.k[1];
	fp32 StoredK0Position = UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Position.k[0];
	fp32 Addturn;

	for(int i = 0; i < iNumberOfIterations; i++)
	{
		BlendSpeed = HANDIKBLENDINSPEED_POSE_1H;

		if(i == 0)
		{
			piLastWeaponPose = &m_LastWeaponPoseRight;
			iCurWeaponPose = m_WeaponPoseRight;
			piCurDestinationPose = &m_CurrentDestWeaponPoseRight;
			pGunBlendVal = &m_GunPoseBlendValueRight;
			RotTrack = PLAYER_ROTTRACK_RHAND;
			RotTrackAtt = PLAYER_ROTTRACK_RHANDATTACH;
			WeaponMat = m_MatWeapon;
			Direct = -1;
			pBlendFromPose = &m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_BLENDFROMRIGHT];
			pDW_WeaponColInfo = &m_RightWeaponColInfo;
			
			if(iCurWeaponPose < WEAPON_POSETEMPLATE_NORMAL_LAST+1)
				UseWeaponPoses[iCurWeaponPose].m_Position.k[0] += m_fAdditionalRightWeaponMovement;

			Addturn = 0.5f;
		}
		else
		{
			UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Rotation.k[1] = StoredK1Rotation;
			UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Position.k[0] = StoredK0Position;

			piLastWeaponPose = &m_LastWeaponPoseLeft;
			iCurWeaponPose = m_WeaponPoseLeft;
			piCurDestinationPose = &m_CurrentDestWeaponPoseLeft;
			pGunBlendVal = &m_GunPoseBlendValueLeft;
			RotTrack = PLAYER_ROTTRACK_LHAND;
			RotTrackAtt = PLAYER_ROTTRACK_LHANDATTACH;
			WeaponMat = m_MatWeapon2;
			Direct = 1;
			pBlendFromPose = &m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_BLENDFROMLEFT];
			pDW_WeaponColInfo = &m_LeftWeaponColInfo;

			if(iCurWeaponPose < WEAPON_POSETEMPLATE_NORMAL_LAST+1)
				UseWeaponPoses[iCurWeaponPose].m_Position.k[0] += m_fAdditionalLeftWeaponMovement;
			
			Addturn = 0.0f;
		}
 
		//----------------------------------------------------------------------------------------------------
		// for grabbing door

		if(m_WeaponHoldType == WEAPONHOLD_TYPE_TWOHANDED)
		{
			if(m_IKFlags & IKFLAGS_OPENDOOR)
			{
				if(m_IKFlags & IKFLAGS_OPENDOOR_RIGHTHAND)
					UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL] = m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_SPECIAL_2HDOOR_RIGHT];
				else
				{
					UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL] = m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_SPECIAL_2HDOOR_LEFT];

					if(m_CameraLookMat.GetRow(0).k[2] < -0.35f)
					{
						bPreventCameraClipping = false;
						UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL] = m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_SPECIAL_2HDOOR_RIGHT];
						if(!(m_IKFlags2 & IKFLAGS2_2H_DOORGRABBOOL1))
						{
							*pGunBlendVal = 0.01f;
							m_IKFlags2 |= IKFLAGS2_2H_DOORGRABBOOL1;
						}
					}
					else
					{
						if(m_IKFlags2 & IKFLAGS2_2H_DOORGRABBOOL1)
						{
							*pGunBlendVal = 0.01f;
							m_IKFlags2 &= ~IKFLAGS2_2H_DOORGRABBOOL1;
						}
					}
				}
			}
			else
			{
				if(m_CameraLookMat.GetRow(0).k[2] > 0.2f && iCurWeaponPose == WEAPON_POSE_GUNSLINGER_DUEL)
				{
					bPreventCameraClipping = false;
					
					// value from -0.75 for k[2] == 1.0f and 0.15 for k[2] == -1.0f ?
					UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Position = CVec3Dfp32(4.0f, 0.0f, -6.0f);
					UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Rotation = CVec3Dfp32(0.0, -0.075f, 0.15f);
					if(!(m_IKFlags2 & IKFLAGS2_2H_DOORGRABBOOL2))
					{
						*pGunBlendVal = 0.01f;
						m_IKFlags2 |= IKFLAGS2_2H_DOORGRABBOOL2;
					}
				}
				else
				{
					if(m_IKFlags2 & IKFLAGS2_2H_DOORGRABBOOL2)
					{
						*pGunBlendVal = 0.01f;
						m_IKFlags2 &= ~IKFLAGS2_2H_DOORGRABBOOL2;
					}
				}
			}
		}

		//----------------------------------------------------------------------------------------------------
		// for grabbing door and wall

		else if(m_WeaponHoldType == WEAPONHOLD_TYPE_DUALWIELD && iCurWeaponPose == WEAPON_POSE_GUNSLINGER_DUEL)
		{		
			UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Position = m_WeaponPoses_1H[UseRandomNormalPose].m_Position;

			// blend if pulling hand front, snap otherwise (ok now with collision every frame)
			if(pDW_WeaponColInfo->m_LenFromWeaponToColPt > pDW_WeaponColInfo->m_UnInterpolLenWeponToPt)
			{
				fp32 LenDiff = pDW_WeaponColInfo->m_LenFromWeaponToColPt - pDW_WeaponColInfo->m_UnInterpolLenWeponToPt;
				pDW_WeaponColInfo->m_LenFromWeaponToColPt -= m_fDeltaTime * Max(Min(LenDiff * HANDIKBLENDINSPEED_FAST, 15.0f), 0.3f);
				pDW_WeaponColInfo->m_LenFromWeaponToColPt = Max(pDW_WeaponColInfo->m_LenFromWeaponToColPt, pDW_WeaponColInfo->m_UnInterpolLenWeponToPt);						
			}
			else
				pDW_WeaponColInfo->m_LenFromWeaponToColPt = pDW_WeaponColInfo->m_UnInterpolLenWeponToPt;

			UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Position.k[0] = -pDW_WeaponColInfo->m_LenFromWeaponToColPt;		

			
			//--------------------------------------------------------------------------------------------------------
			// special case when we really REALLY want to be able to fire straight forward even in this case

			if(pDW_WeaponColInfo->m_bIsPlayerPhys)
			{
				UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Rotation = 0.0f;
			}
			else
			{
				//----------------------------------------------------------------------------------------------------
				// apply Barreldir depending rotations and positions

				CVec3Dfp32 BarrelDir;
				m_CameraLookMat.GetRow(1).CrossProd(pDW_WeaponColInfo->m_CollsionNormal, BarrelDir);
				BarrelDir = BarrelDir.Normalize();
				fp32 fBarrelDirAngleMod = M_ACos(m_CameraLookMat.GetRow(0) * BarrelDir);
				fBarrelDirAngleMod = fBarrelDirAngleMod / (_PI2); // convert to 0->1 angle

				fp32 AngleDiff = pDW_WeaponColInfo->m_BarrelDependentRotation - fBarrelDirAngleMod;
				if(AngleDiff > 0.0f)
				{
					pDW_WeaponColInfo->m_BarrelDependentRotation -= m_fDeltaTime *  Max(AngleDiff, 0.015f) * 10.0f;
					pDW_WeaponColInfo->m_BarrelDependentRotation = Max(pDW_WeaponColInfo->m_BarrelDependentRotation, fBarrelDirAngleMod);
				}
				else if(AngleDiff < 0.0f)
				{
					pDW_WeaponColInfo->m_BarrelDependentRotation += m_fDeltaTime * Max(-AngleDiff, 0.015f) * 13.0f;
					pDW_WeaponColInfo->m_BarrelDependentRotation = Min(pDW_WeaponColInfo->m_BarrelDependentRotation, fBarrelDirAngleMod);
				}

				// add a small rotation forward when colliding with walls
				fp32 WallRotationAddFWD = 0.03f * (1.0f - pDW_WeaponColInfo->m_CollsionNormal.k[2]);

				UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Rotation.k[1] = -(pDW_WeaponColInfo->m_BarrelDependentRotation - WallRotationAddFWD);
				UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Rotation.k[1] = Max(UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Rotation.k[1], -0.32f);
				UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Position.k[0] -= Max((pDW_WeaponColInfo->m_BarrelDependentRotation-0.32f), 0.0f) * 40.0f;

				//----------------------------------------------------------------------------------------------------
				// apply rotations around the barreldirection

				// set weapon flat against wall
				CVec3Dfp32 Right = m_CameraLookMat.GetRow(1);
				fp32 fRightAngleMod = M_ACos(pDW_WeaponColInfo->m_CollsionNormal * Right); // todo: - right for left hand?
				fRightAngleMod = fRightAngleMod / (_PI2); // convert to 0->1 angle
				
				if(Direct == -1)
					fRightAngleMod = Max(fRightAngleMod, 0.35f);
				else
					fRightAngleMod = Min(fRightAngleMod, 0.15f);

				AngleDiff = pDW_WeaponColInfo->m_NormalDependentRotation - fRightAngleMod;
				if(AngleDiff > 0.0f)
				{
					pDW_WeaponColInfo->m_NormalDependentRotation -= m_fDeltaTime *  Max(AngleDiff, 0.025f) * 10.0f;
					pDW_WeaponColInfo->m_NormalDependentRotation = Max(pDW_WeaponColInfo->m_NormalDependentRotation, fRightAngleMod);
				}
				else if(AngleDiff < 0.0f)
				{
					pDW_WeaponColInfo->m_NormalDependentRotation += m_fDeltaTime * Max(-AngleDiff, 0.025f) * 10.0f;
					pDW_WeaponColInfo->m_NormalDependentRotation = Min(pDW_WeaponColInfo->m_NormalDependentRotation, fRightAngleMod);
				}

				UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Rotation.k[0] = pDW_WeaponColInfo->m_NormalDependentRotation  * Direct + Addturn;
				UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Position.k[0] = Min(UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Position.k[0], m_WeaponPoses_1H[UseRandomNormalPose].m_Position.k[0]);			

				// need a fast blendspeed to get the guns rotated out the the way
				BlendSpeed = HANDIKBLENDINSPEED_FAST * ((0.0766 - Abs(fRightAngleMod - 0.25)) / 0.0766);
				BlendSpeed = Max(BlendSpeed, HANDIKBLENDINSPEED);

				// additional moveback for guns if looking downwards so barrles wont clip
				UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Position.k[0] -= (Min((m_CameraLookMat.GetRow(0).k[2] + 0.25f), 0.0f) * -5.0f);

				if(m_IKFlags & IKFLAGS_OPENDOOR)
				{
					if(m_IKFlags & IKFLAGS_OPENDOOR_RIGHTHAND)
					{
						if(i == 0)
							UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Position.k[1] = 6.2f;
						else
						{
							UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Position.k[0] = -6.5f;
							UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Rotation.k[0] = 0.0f;
						}
					}
					else
					{
						if(i == 0)
						{
							UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Position.k[0] = -6.5f;
							UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Rotation.k[0] = 0.0f;
						}
						else
							UseWeaponPoses[WEAPON_POSE_GUNSLINGER_DUEL].m_Position.k[1] = 6.2f;
					}
				}
			}
		}
		else if(pDW_WeaponColInfo->m_LenFromWeaponToColPt != 0.0f)
                {
			pDW_WeaponColInfo->m_LenFromWeaponToColPt = 0.0f;
		}
				
		//----------------------------------------------------------------------------------------------------
		// Blend slow if going from a normal position to another normal.
		if((pBlendFromPose->m_Rotation.k[1] == 0.0f) &&
			(iCurWeaponPose < WEAPON_POSETEMPLATE_NORMAL_LAST+1 && 
			*piLastWeaponPose < WEAPON_POSETEMPLATE_NORMAL_LAST+1 && 
			*piCurDestinationPose < WEAPON_POSETEMPLATE_NORMAL_LAST+1) &&
			!(m_IKFlags & IKFLAGS_INWEAPONZOOMMODE) && 
			*piLastWeaponPose != WEAPON_POSETEMPLATE_NORMAL_ZOOM &&
			!(m_IKFlags2 & IKFLAGS2_INWEAPONZOOMOUTMODE))
		{

			BlendSpeed = HANDIKBLENDINSPEED * 0.15f;
		}
		else if(m_WeaponHoldType != WEAPONHOLD_TYPE_TWOHANDED && iCurWeaponPose == WEAPON_POSE_GUNSLINGER_DUEL)
			BlendSpeed = HANDIKBLENDINSPEED_FAST * 1.5f;
		
		/*
		// play wallscrapesound when got wallcollsioion
		else if(iCurWeaponPose != *piLastWeaponPose && _pWPhysState->IsClient())
		{
			if(Random < 0.15f)
			{
				MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
				CStr cmd = CStrF("stream_play(\"%s\", \"\", 1)", "entersoundnamehere");
				pCon->ExecuteString(cmd);
			}
		}
		*/		

		//----------------------------------------------------------------------------------------------------
		// Blend in and out of poses

		if(*piLastWeaponPose != iCurWeaponPose || *pGunBlendVal != 0.0f)
		{
			if(*piCurDestinationPose != iCurWeaponPose)
			{
				*pGunBlendVal = 0.0f;
				*piCurDestinationPose = iCurWeaponPose;			
			}

			// setup a blend-value
			*pGunBlendVal += m_fDeltaTime * BlendSpeed;
			*pGunBlendVal = Min(*pGunBlendVal, 1.0f);
			fp32 SincBlend = Sinc(*pGunBlendVal);

			// finished blending, all OK
			if(*pGunBlendVal == 1.0f) 
			{
				*piLastWeaponPose = iCurWeaponPose;
				UsePos = UseWeaponPoses[iCurWeaponPose].m_Position;
				UseRot = UseWeaponPoses[iCurWeaponPose].m_Rotation;
				*pGunBlendVal = 0.0f;
			}
			else
			{
				// blend from current BlendFromPose to iCurWeaponPose
				UsePos = pBlendFromPose->m_Position + 
					((UseWeaponPoses[iCurWeaponPose].m_Position - pBlendFromPose->m_Position) * SincBlend);

				for(int ii = 0; ii < 3; ii++)
					UseRot.k[ii] = pBlendFromPose->m_Rotation.k[ii] + 
					((UseWeaponPoses[iCurWeaponPose].m_Rotation.k[ii] - pBlendFromPose->m_Rotation.k[ii]) * SincBlend);

				// to prevent camera clipping
				if(bPreventCameraClipping)
				{
					fp32 Blendval = 0;
					Blendval = M_Sin(*pGunBlendVal * _PI);
					UsePos += CVec3Dfp32(1.5f, 0.0f, -1.0f) * Blendval;
				}
			}
		}
		else //pGunBlendVal == 0.0f// finished blending, all OK
		{
			*piCurDestinationPose = iCurWeaponPose;
			UseRot = UseWeaponPoses[iCurWeaponPose].m_Rotation;
			UsePos = UseWeaponPoses[iCurWeaponPose].m_Position;
		}

		// store current positions
		pBlendFromPose->m_Position = UsePos;
		pBlendFromPose->m_Rotation = UseRot;
			
		//-------------------------------------------------------------------------------------------------------------------------------
		// apply the gunpose position

		RotateBoneAroundAxis(UseRot.k[0] * Direct, &m_CameraLookMat.GetRow(0), RotTrack, _pSkel, _pSkelInstance, _pWPhysState, false);
		RotateBoneAroundAxis(UseRot.k[1], &m_CameraLookMat.GetRow(1), RotTrack, _pSkel, _pSkelInstance, _pWPhysState, false);
		RotateBoneAroundAxis(UseRot.k[2], &m_CameraLookMat.GetRow(2), RotTrack, _pSkel, _pSkelInstance, _pWPhysState, false);

		CMat4Dfp32 TmpMat;
		int iParentNode = _pSkel->m_lNodes[RotTrack].m_iNodeParent;
		TmpMat = _pSkelInstance->m_pBoneTransform[iParentNode];
		TmpMat.GetRow(3) = 0.0f; //ignore translation

		CMat4Dfp32 ParentInvMat;
		TmpMat.InverseOrthogonal(ParentInvMat);

		CVec3Dfp32 WeaponMoveDir = (m_CameraLookMat.GetRow(0) * -UsePos.k[0]) + 
			(m_CameraLookMat.GetRow(1) * (Direct * UsePos.k[1])) + 
			(m_CameraLookMat.GetRow(2) * -UsePos.k[2]);

		CVec3Dfp32 WeaponMoveDirLocal;
		WeaponMoveDirLocal = WeaponMoveDir * ParentInvMat;
		_pSkelInstance->m_pBoneLocalPos[RotTrack].GetRow(3) += WeaponMoveDirLocal;

		_pSkel->EvalNode_IK_Special(RotTrack, &_pSkelInstance->m_pBoneTransform[iParentNode], _pSkelInstance, 0);

		iParentNode = _pSkel->m_lNodes[RotTrackAtt].m_iNodeParent;
		_pSkel->EvalNode_IK_Special(RotTrackAtt, &_pSkelInstance->m_pBoneTransform[iParentNode], _pSkelInstance, 0);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------
// returns whether or not we should run IK, nad blend in or out of IK mode.
// also updates current blend mode
bool CPostAnimSystem::BlendHandNodes(int _IKMode, CMat4Dfp32 *_pTarget, CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance)
{
#ifndef OGIER
	bool bRunIK = true; // the return bool
	uint8 *pCurrentBlendMode;
	fp32 *pCurrentBlendValue;
	int NodeId = -1;
	int NodeId2 = -1; // tmp
	fp32 SincBlendval;

	switch(_IKMode) {
	case CIKSystem::IK_MODE_LEFT_HAND:

		pCurrentBlendMode  = &m_CurrentWeaponBlendTypeLeft;
		pCurrentBlendValue = &m_LeftHandBlendVal;
		NodeId  = PLAYER_ROTTRACK_LHANDATTACH;
		NodeId2 = PLAYER_ROTTRACK_LHAND;
		break;

	case CIKSystem::IK_MODE_RIGHT_HAND:
		pCurrentBlendMode  = &m_CurrentWeaponBlendTypeRight;
		pCurrentBlendValue = &m_RightHandBlendVal;
		NodeId  = PLAYER_ROTTRACK_RHANDATTACH;
		NodeId2 = PLAYER_ROTTRACK_RHAND;
		break;

	default:
		// not implemented yet
		return false;
	}

	
	//-------------------------------------------------------------------------------------
	// blend towards target
	switch(*pCurrentBlendMode) 
	{
	case WEAPONBLEND_TYPE_NONE:
		// no blend at all
		bRunIK = false;
		*pCurrentBlendValue = 0.0f;
		break;

	case WEAPONBLEND_TYPE_IN:

		*pCurrentBlendValue += HANDIKBLENDINSPEED_FAST * m_fDeltaTime;// * Max(M_Sin(*pCurrentBlendValue), 0.05f);
		
		*pCurrentBlendValue  = Min(*pCurrentBlendValue, 1.0f);
		SincBlendval = Sinc(*pCurrentBlendValue);

		if(*pCurrentBlendValue == 1.0f)
		{
			*pCurrentBlendMode = WEAPONBLEND_TYPE_FULL_IN;
		}
		else
		{
			CVec3Dfp32 Tmp;
			Tmp =  _pSkel->m_lNodes[NodeId].m_LocalCenter * 
				_pSkelInstance->m_pBoneTransform[NodeId2];

			CVec3Dfp32 AnimToIK = _pTarget->GetRow(3) - Tmp;

			_pTarget->GetRow(3) = Tmp + (AnimToIK * SincBlendval);
		}

		break;

	case WEAPONBLEND_TYPE_OUT:

		*pCurrentBlendValue -= HANDIKBLENDINSPEED_FAST * m_fDeltaTime;// * Max(M_Sin(*pCurrentBlendValue), 0.05f);

		*pCurrentBlendValue  = Max(*pCurrentBlendValue, 0.0f);
		SincBlendval = Sinc(*pCurrentBlendValue);

		if(*pCurrentBlendValue == 0.0f) // snap the rest
		{
			bRunIK = false;
			*pCurrentBlendMode = WEAPONBLEND_TYPE_NONE;
		}
		else
		{
			CVec3Dfp32 Tmp;
			Tmp =  _pSkel->m_lNodes[NodeId].m_LocalCenter * 
				_pSkelInstance->m_pBoneTransform[NodeId2];

			CVec3Dfp32 IKToAnim = _pTarget->GetRow(3) - Tmp;

			_pTarget->GetRow(3) = Tmp + (IKToAnim * SincBlendval);
		}
		break;

	case WEAPONBLEND_TYPE_FULL_IN: 
		*pCurrentBlendValue = 1.0f;
		// target is ok!
		break;

	default:
		break; // ?? error!
	}

	return bRunIK;
#else
	return false;
#endif
}

void CPostAnimSystem::InitRecoil(bool _bIsLeftHand, int _RecoilType, int _RandVal)
{
	if(_bIsLeftHand)// && !(m_IKFlags & IKFLAGS_RECOILLEFTRUNNING))
	{
		m_RecoilTimerLeft = 0.0f;
		m_IKFlags |= IKFLAGS_RECOILLEFTRUNNING;
		
		m_LeftRecoilType = _RecoilType;
		m_LeftRecoilSubType = int(MFloat_GetRand(_RandVal)*RECOIL_STRENGHT_SIZE);
		
		if(m_LeftRecoilSubType > (RECOIL_STRENGHT_SIZE-1))
			m_LeftRecoilSubType = (RECOIL_STRENGHT_SIZE-1);

#ifdef USEJAKOBSDEBUGINFO
		M_TRACEALWAYS(CStrF("LeftRecoilSubType %d\n", m_LeftRecoilSubType));
#endif

	}
	else if(!_bIsLeftHand)// && !(m_IKFlags & IKFLAGS_RECOILRIGHTRUNNING))
	{
		m_RecoilTimerRight = 0.0f;
		m_IKFlags |= IKFLAGS_RECOILRIGHTRUNNING;
		
		m_RightRecoilType = _RecoilType;
		m_RightRecoilSubType = int(MFloat_GetRand(_RandVal*0xCAB)*RECOIL_STRENGHT_SIZE);
		
		if(m_RightRecoilSubType > (RECOIL_STRENGHT_SIZE-1))
			m_RightRecoilSubType = (RECOIL_STRENGHT_SIZE-1);

#ifdef USEJAKOBSDEBUGINFO
		M_TRACEALWAYS(CStrF("LeftRecoilSubType %d\n", m_RightRecoilSubType));
#endif

	}
}

// apply recoil to camera

void CPostAnimSystem::RecoilModCamera(CMat4Dfp32 *_pCamMat, CWO_Character_ClientData *_pCD)
{
	if(_pCD->m_iPlayer != -1 && (m_IKFlags & IKFLAGS_RECOILRIGHTRUNNING || (m_IKFlags & IKFLAGS_RECOILLEFTRUNNING)))
	{
		//m_RecoilCamMod.k[0] = mov;
		//m_RecoilCamMod.k[1] = rot;

		CVec3Dfp32 ModIt = m_CameraLookMat.GetRow(0) * -m_RecoilCamMod.k[0];
		//_pCamMat->GetRow(3) += ModIt * 1.0f;//CAMERACERCOILMOD;
		_pCamMat->RotX_x_M(m_RecoilCamMod.k[1] * 0.02f);

		if(m_IKFlags & IKFLAGS_RECOILRIGHTRUNNING && m_RightRecoilSubType > 1)
			_pCamMat->RotY_x_M(m_RecoilCamMod.k[1] * 0.02f);
		else if(m_IKFlags & IKFLAGS_RECOILLEFTRUNNING && m_LeftRecoilSubType > 1)
			_pCamMat->RotY_x_M(m_RecoilCamMod.k[1] * -0.02f);
	}
}

void CPostAnimSystem::UpdateRecoil(const CMat4Dfp32 *_pWeaponMat, const CVec3Dfp32 *_pRotAxis, CMat4Dfp32 *_pHandMat, bool _bIsLeftHand)
{
	// if in rotation, dont apply rotation todo:
	int RecoilType = 0;
	uint8 RecoilSubType = 0;
	fp32 RecoilTimer = 0.0f;
	fp32 RecoilMovMax = 0.0f;
	fp32 RecoilMovUpVal = 0.0f;
	fp32 RecoilRotMax = 0.0f;
	fp32 RecoilTime = 0.0f;
	fp32 RecoilRecoveryTime = 0.0f;
	fp32 AdditionalMovement = 0;
	
	if(_bIsLeftHand)
	{
		RecoilType = m_LeftRecoilType;
		RecoilSubType = m_LeftRecoilSubType;
		RecoilTimer = m_RecoilTimerLeft;
		AdditionalMovement = m_fAdditionalLeftWeaponMovement;
	}
	else
	{
		RecoilType = m_RightRecoilType;
		RecoilSubType = m_RightRecoilSubType;
		RecoilTimer = m_RecoilTimerRight;
		AdditionalMovement = m_fAdditionalRightWeaponMovement;
	}

	switch(RecoilType) 
	{
	case RECOIL_STRENGHT_LIGHT:

		switch(RecoilSubType) {
		case 0:
			RecoilMovMax = RECOILMOVMAXLIGHT_1;
			RecoilRotMax = RECOILROTMAXLIGHT_1;
			RecoilMovUpVal = RECOILMOVUPLIGHT_1;
			break;
		case 1:
			RecoilMovMax = RECOILMOVMAXLIGHT_2;
			RecoilRotMax = RECOILROTMAXLIGHT_2;
			RecoilMovUpVal = RECOILMOVUPLIGHT_2;
			break;
		case 2:
			RecoilMovMax = RECOILMOVMAXLIGHT_3;
			RecoilRotMax = RECOILROTMAXLIGHT_3;
			RecoilMovUpVal = RECOILMOVUPLIGHT_3;
			break;
		default:
			break;
		}
		
		RecoilTime = RECOILTIMELIGHT;
		RecoilRecoveryTime = RECOILRECOVETIMELIGHT;
		break;

	case RECOIL_STRENGHT_MEDIUM:

		switch(RecoilSubType) {
		case 0:
			RecoilMovMax = RECOILMOVMAXMEDIUM_1;
			RecoilRotMax = RECOILROTMAXMEDIUM_1;
			RecoilMovUpVal = RECOILMOVUPMEDIUM_1;
			break;
		case 1:
			RecoilMovMax = RECOILMOVMAXMEDIUM_2;
			RecoilRotMax = RECOILROTMAXMEDIUM_2;
			RecoilMovUpVal = RECOILMOVUPMEDIUM_2;
			break;
		case 2:
			RecoilMovMax = RECOILMOVMAXMEDIUM_3;
			RecoilRotMax = RECOILROTMAXMEDIUM_3;
			RecoilMovUpVal = RECOILMOVUPMEDIUM_3;
			break;
		default:
			break;
		}

		RecoilTime = RECOILTIMEMEDIUM;
		RecoilRecoveryTime = RECOILRECOVETIMEMEDIUM;
		break;

	case RECOIL_STRENGHT_HEAVY:

		switch(RecoilSubType) {
		case 0:
			RecoilMovMax = RECOILMOVMAXHEAVY_1;
			RecoilRotMax = RECOILROTMAXHEAVY_1;
			RecoilMovUpVal = RECOILMOVUPHEAVY_1;
			break;
		case 1:
			RecoilMovMax = RECOILMOVMAXHEAVY_2;
			RecoilRotMax = RECOILROTMAXHEAVY_2;
			RecoilMovUpVal = RECOILMOVUPHEAVY_2;
			break;
		case 2:
			RecoilMovMax = RECOILMOVMAXHEAVY_3;
			RecoilRotMax = RECOILROTMAXHEAVY_3;
			RecoilMovUpVal = RECOILMOVUPHEAVY_3;
			break;
		default:
			break;
		}
		
		RecoilTime = RECOILTIMEHEAVY;
		RecoilRecoveryTime = RECOILRECOVETIMEHEAVY;
		break;

	default:
		break;
	}

	CVec3Dfp32 MovDir;
	MovDir = (_pWeaponMat->GetRow(0) + (-_pWeaponMat->GetRow(2) * RecoilMovUpVal)).Normalize();

	if(m_fDeltaTime > RecoilTime && RecoilTimer == 0.0f) // instant snap-kick
	{
		// movement
		RecoilTimer += m_fDeltaTime;

		// rotation
		CVec3Dfp32 StorePos = _pHandMat->GetRow(3);
		CMat4Dfp32 MatPlaneRotation;
		CQuatfp32(*_pRotAxis, RecoilRotMax).CreateMatrix3x3(MatPlaneRotation);
		MatPlaneRotation.GetRow(3) = 0.0f;

		CMat4Dfp32 Temp;
		Temp = *_pHandMat;
		CMat4Dfp32 ResultRot;
		Temp.Multiply3x3(MatPlaneRotation, ResultRot);
		ResultRot.GetRow(3) = StorePos - MovDir * RecoilMovMax;

		*_pHandMat = ResultRot;

		m_RecoilCamMod.k[0] = RecoilMovMax;
		m_RecoilCamMod.k[1] = RecoilRotMax;
	}
	else
	{
		RecoilTimer += m_fDeltaTime;
		fp32 TotalTime = RecoilTime + RecoilRecoveryTime;

		if(RecoilTimer > TotalTime)
		{
			// finished
			if(_bIsLeftHand)
				m_IKFlags &= ~IKFLAGS_RECOILLEFTRUNNING;
			else
				m_IKFlags &= ~IKFLAGS_RECOILRIGHTRUNNING;
		}
		else if(RecoilTimer > RecoilTime)
		{
			// run rebound
			fp32 ReboundVal    = RecoilMovMax - (Sinc((RecoilTimer - RecoilTime) / RecoilRecoveryTime) * RecoilMovMax);
			fp32 ReboundRotVal = RecoilRotMax - (Sinc((RecoilTimer - RecoilTime) / RecoilRecoveryTime) * RecoilRotMax);

			if(ReboundVal > 0.0f && ReboundRotVal > 0.0f)
			{
				CVec3Dfp32 StorePos = _pHandMat->GetRow(3);

				CMat4Dfp32 MatPlaneRotation;
				CQuatfp32(*_pRotAxis, ReboundRotVal).CreateMatrix3x3(MatPlaneRotation);
				MatPlaneRotation.GetRow(3) = 0.0f;

				CMat4Dfp32 ResultRot;
				_pHandMat->Multiply3x3(MatPlaneRotation, ResultRot);
				ResultRot.GetRow(3) = StorePos - MovDir * ReboundVal;

				*_pHandMat = ResultRot;

				m_RecoilCamMod.k[0] = ReboundVal;
				m_RecoilCamMod.k[1] = ReboundRotVal;

			}
			else
			{
				if(_bIsLeftHand)
					m_IKFlags &= ~IKFLAGS_RECOILLEFTRUNNING;
				else
					m_IKFlags &= ~IKFLAGS_RECOILRIGHTRUNNING;
			}
		}
		else
		{
			// run recoil kick. The kick is a linear motion
			fp32 KickVal = Sinc(RecoilTimer / RecoilTime) * RecoilMovMax;
			fp32 RotVal  = Sinc(RecoilTimer / RecoilTime) * RecoilRotMax;

			if(KickVal > 0.0f && RotVal > 0.0f)
			{
				CVec3Dfp32 StorePos = _pHandMat->GetRow(3);			
				
				CMat4Dfp32 MatPlaneRotation;
				CQuatfp32(*_pRotAxis, RotVal).CreateMatrix3x3(MatPlaneRotation);
				MatPlaneRotation.GetRow(3) = 0.0f;

				CMat4Dfp32 ResultRot;
				_pHandMat->Multiply3x3(MatPlaneRotation, ResultRot);
				ResultRot.GetRow(3) = StorePos - MovDir * KickVal;

				*_pHandMat = ResultRot;

				m_RecoilCamMod.k[0] = KickVal;
				m_RecoilCamMod.k[1] = RotVal;
			}
			else
			{
				if(_bIsLeftHand)
					m_IKFlags &= ~IKFLAGS_RECOILLEFTRUNNING;
				else
					m_IKFlags &= ~IKFLAGS_RECOILRIGHTRUNNING;
			}
		}
	}

	if(_bIsLeftHand)
	{
		m_RecoilTimerLeft = RecoilTimer;
	}
	else
	{
		m_RecoilTimerRight = RecoilTimer;
	}	
}

void CPostAnimSystem::UpdateTimer(const CMTime *_pTime)//, fp32 _TimeFraction)
{
	m_fDeltaTime = _pTime->GetTime() - m_LastTime.GetTime();
	m_fDeltaTime = Min(0.1f, Max(m_fDeltaTime, 0.0f));
	m_LastTime = *_pTime;
	//m_TimeFraction = _TimeFraction;
}

void CPostAnimSystem::DisableFeetIK(bool _DisbleIt)
{
	m_DisableDarklingIK = _DisbleIt;
	m_DisableHumanFeetIK = _DisbleIt;

}

bool CPostAnimSystem::CheckOKTorRunFeetIK(CWO_Character_ClientData *_pCD, uint8 _FeetType)
{
#ifndef OGIER
	bool bJustSpawned = false;
	bool bJustShapeShifted = false;

	if(_FeetType == FEET_TYPE_HUMAN_BIPED && !(m_IKFlags & IKFLAGS_WASHUMAN))
	{
		bJustShapeShifted = true;
		m_IKFlags |= IKFLAGS_WASHUMAN;
	}
	else if(_FeetType == FEET_TYPE_DARKLING_QUADRO)
	{
		if((m_IKFlags & IKFLAGS_WASHUMAN))
		{
			bJustShapeShifted = true;
			m_IKFlags &= ~IKFLAGS_WASHUMAN;
		}
		else if(!(m_IKFlags & IKFLAGS_FEETSETUPOK))
		{
			CWO_CharDarkling_ClientData* pDarklingCD = _pCD->IsDarkling();
			if(pDarklingCD->Char_IsSpawning())
				bJustSpawned = true;
		}
	}

	if(_pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_DISABLEHUMANIK)
	{
		m_IKFlags &= ~IKFLAGS_FEETSETUPOK;
		m_DisableHumanFeetIK = true;
		m_DisableDarklingIK = true;
		return false;
	}

	if(_pCD->m_AnimGraph2.GetStateFlagsHiCombined() & AG2_STATEFLAGHI_IKSYSTEM_IGNOREFEET ||	// don't do if ignore-flag is on
		(_pCD->m_iPlayer != -1 && _FeetType == FEET_TYPE_HUMAN_BIPED) ||					// don't do for the player (right now anyways)
		(_pCD->m_iPlayer == -1 && (_pCD->m_AnimGraph2.GetStateFlagsLo()  &AG2_STATEFLAG_BEHAVIORACTIVE)) || // dont't do while in behavior
		(_pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOANIMPHYS))  // don't do when no-anim-physing
	{
		if(bJustShapeShifted || bJustSpawned)
		{
			m_FeetBlendVal = 0.0f;
			m_FrontFeetBlendVal = 0.0f;
		}
		else
		{
			if(!(_pCD->m_AnimGraph2.GetStateFlagsHiCombined() &AG2_STATEFLAGHI_IKSYSTEM_FORCEBACKFEET))
			{
				if(m_FeetBlendVal > 0.0f)
				{
					m_FeetBlendVal -= m_fDeltaTime * FEETBLENDSPEED;
					m_FeetBlendVal = Max(m_FeetBlendVal, 0.0f);
				}
			}
			else
			{
				if(m_FeetBlendVal < 1.0f)
				{
					m_FeetBlendVal += m_fDeltaTime * FEETBLENDSPEED;
					m_FeetBlendVal = Min(m_FeetBlendVal, 1.0f);
				}
			}

			if(!(_pCD->m_AnimGraph2.GetStateFlagsHiCombined() &AG2_STATEFLAGHI_IKSYSTEM_FORCEFRONTFEET))
			{
				if(m_FrontFeetBlendVal > 0.0f)
				{
					m_FrontFeetBlendVal -= m_fDeltaTime * FEETBLENDSPEED;
					m_FrontFeetBlendVal = Max(m_FrontFeetBlendVal, 0.0f);
				}
			}
			else
			{
				if(m_FrontFeetBlendVal < 1.0f)
				{
					m_FrontFeetBlendVal += m_fDeltaTime * FEETBLENDSPEED;
					m_FrontFeetBlendVal = Min(m_FrontFeetBlendVal, 1.0f);
				}
			}
		}
	}
	else
	{
		if(bJustShapeShifted || bJustSpawned)
		{
			m_FeetBlendVal = 0.0f;
			m_FrontFeetBlendVal = 0.0f;
		}
		else
		{
			if(m_FeetBlendVal < 1.0f)
			{
				m_FeetBlendVal += m_fDeltaTime * FEETBLENDSPEED;
				m_FeetBlendVal = Min(m_FeetBlendVal, 1.0f);
			}

			if(m_FrontFeetBlendVal < 1.0f)
			{
				m_FrontFeetBlendVal += m_fDeltaTime * FEETBLENDSPEED;
				m_FrontFeetBlendVal = Min(m_FrontFeetBlendVal, 1.0f);
			}
		}
	}

	if(m_FeetBlendVal == 0.0f && m_FrontFeetBlendVal == 0.0f)
	{
		m_IKFlags &= ~IKFLAGS_FEETSETUPOK;
		return false;
	}
#endif
	return true;
}

void CPostAnimSystem::GetPointOnVector(const CVec3Dfp32 *_pTarget, const CVec3Dfp32 *_pAnimpoint, CVec3Dfp32 *_pDest)
{
	CVec3Dfp32 VecDir = m_ModelNode0.GetRow(2);
	VecDir *= VecDir * (*_pTarget  - *_pAnimpoint);
	*_pDest  = *_pAnimpoint + VecDir;
}

void CPostAnimSystem::EvalFeetIK(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CWObject_CoreData* _pObj, 
								 CXR_Model* _pModel, CWorld_PhysState* _pWPhysState, uint8 _FeetType, int _PhysType, bool _IsWallClimber)
{
#ifndef OGIER

	CWO_Character_ClientData *pCDFirst;
	pCDFirst =  _pObj ? CWObject_Character::GetClientData(_pObj) : NULL;
	if(!pCDFirst)
		return;
	
	//-------------------------------------------------------------------------------------------
	// Get the current speed
	m_ModelNode0 = _pSkelInstance->m_pBoneTransform[0];
	m_CharSpeed = (m_ModelNode0.GetRow(3) - m_LastPos);
	m_LastPos = m_ModelNode0.GetRow(3);

	if(!CheckOKTorRunFeetIK(pCDFirst, _FeetType))
	{
		m_StairMoveType = FOOT_MOVEMENT_ANIMDRIVEN;
		return;
	}
	
	//-------------------------------------------------------------------------------------------------
	// depending on character, we setup the feets diffrent
	bool bInAir = false;

	// for jumping only, N/A on AI players, so...
	//if((pCDFirst->m_Phys_nInAirFrames > 6))
	//	bInAir = true;

	CVec3Dfp32 DownVec(0.0f, 0.0f, 1.0f);

	int8 Node_RBF = -1; // Right Back Foot
	int8 Node_RBK = -1; // Right Back Knee
	int8 Node_RBL = -1; // Right Back Leg

	int8 Node_LBF = -1;
	int8 Node_LBK = -1;
	int8 Node_LBL = -1;

	int8 Node_RFF = -1;
	int8 Node_RFK = -1;
	int8 Node_RFL = -1; 

	int8 Node_LFF = -1;
	int8 Node_LFK = -1;
	int8 Node_LFL = -1;

	switch(_FeetType) 
	{
	case FEET_TYPE_HUMAN_BIPED:

		if(m_DisableHumanFeetIK)
			return;

		Node_RBF = PLAYER_ROTTRACK_RFOOT;
		Node_RBK = PLAYER_ROTTRACK_RKNEE;
		Node_RBL = PLAYER_ROTTRACK_RLEG;

		Node_LBF = PLAYER_ROTTRACK_LFOOT;
		Node_LBK = PLAYER_ROTTRACK_LKNEE;
		Node_LBL = PLAYER_ROTTRACK_LLEG;

		//-------------------------------------------------------------------------------------------------
		// depending on some states we want some values changed

		if(_PhysType == 2)
		{
			m_Diffr = 0.0f;
			return;
		}

		break;

	case FEET_TYPE_DARKLING_QUADRO:

		if(m_DisableDarklingIK) // TODO: blend in and out of this
			return;

		// do not alter animation if is in a behaviour
		if((pCDFirst->m_AnimGraph2.GetStateFlagsLo()  &AG2_STATEFLAG_BEHAVIORACTIVE))
			return;

		m_Diffr = 0.0f;

		Node_RBF = PLAYER_ROTTRACK_RFOOT;
		Node_RBK = CIKSystem::DARKLING_ROTTRACK_RIGHT_LOWLEG;
		Node_RBL = CIKSystem::DARKLING_ROTTRACK_RIGHT_MIDLEG;

		Node_LBF = PLAYER_ROTTRACK_LFOOT;
		Node_LBK = CIKSystem::DARKLING_ROTTRACK_LEFT_LOWLEG;
		Node_LBL = CIKSystem::DARKLING_ROTTRACK_LEFT_MIDLEG;


		Node_LFF = PLAYER_ROTTRACK_LHAND;
		Node_LFK = PLAYER_ROTTRACK_LELBOW;
		Node_LFL = PLAYER_ROTTRACK_LARM; 

		Node_RFF = PLAYER_ROTTRACK_RHAND;
		Node_RFK = PLAYER_ROTTRACK_RELBOW;
		Node_RFL = PLAYER_ROTTRACK_RARM;

		break;

	default:
		return; // error...
	}

	if(_FeetType == FEET_TYPE_HUMAN_BIPED) // we only raise and lower humans, and that value counts as being idle
		m_CharSpeed.k[2] = 0.0f;

	DownVec = m_ModelNode0.GetRow(2);

	//-------------------------------------------------------------------------------------------
	// Get the current positions according to the animation. This is positions BEFORE the character is lowered

	m_MatRightFootAnim = _pSkelInstance->m_pBoneTransform[Node_RBF];
	m_MatRightFootAnim.GetRow(3) = _pSkel->m_lNodes[Node_RBF].m_LocalCenter * _pSkelInstance->m_pBoneTransform[Node_RBK];

	m_MatLeftFootAnim = _pSkelInstance->m_pBoneTransform[Node_LBF];
	m_MatLeftFootAnim.GetRow(3) = _pSkel->m_lNodes[Node_LBF].m_LocalCenter * _pSkelInstance->m_pBoneTransform[Node_LBK];

	if(_FeetType == FEET_TYPE_DARKLING_QUADRO)
	{
		m_MatFrontRightFootAnim = _pSkelInstance->m_pBoneTransform[Node_RFF];
		m_MatFrontRightFootAnim.GetRow(3) = _pSkel->m_lNodes[Node_RFF].m_LocalCenter * _pSkelInstance->m_pBoneTransform[Node_RFK];

		m_MatFrontLeftFootAnim = _pSkelInstance->m_pBoneTransform[Node_LFF];
		m_MatFrontLeftFootAnim.GetRow(3) = _pSkel->m_lNodes[Node_LFF].m_LocalCenter * _pSkelInstance->m_pBoneTransform[Node_LFK];
	}

	//-------------------------------------------------------------------------------------------

	bool bSolveLeftLeg  = true;
	bool bSolveRightLeg = true;

	// and for quadroped		
	bool bFrontSolveLeftLeg  = true;
	bool bFrontSolveRightLeg = true;

	//-------------------------------------------------------------------------------------------
	// if moving slowly or not moving at all, feet should collide

	fp32 StandingStillval = 30.0f * m_fDeltaTime;
	fp32 IdleValue = 100.0f * m_fDeltaTime;
	if(_FeetType == FEET_TYPE_DARKLING_QUADRO)
		IdleValue = 50.0f * m_fDeltaTime;

	if(m_CharSpeed.LengthSqr() < Sqr(IdleValue) || (pCDFirst->m_AnimGraph2.GetStateFlagsHiCombined() & AG2_STATEFLAGHI_IKSYSTEM_IGNOREFEET)) // stand still
	{
		if(m_CharSpeed.LengthSqr() < Sqr(StandingStillval))
			m_IKFlags |= IKFLAGS_STANDINGSTILL;
		else
			m_IKFlags &= ~IKFLAGS_STANDINGSTILL;

		m_IKFlags |= IKFLAGS_ISSLOWERTHANRUNNING;
		m_IKFlags &= ~IKFLAGS_LSHOULDCOLLIDE;
		m_IKFlags &= ~IKFLAGS_RSHOULDCOLLIDE;

		//pCDFirst->m_aLegIKIsDirty[1] = false; // 1 = left;
		//pCDFirst->m_aLegIKIsDirty[0] = false; // 0 = right;

		if(_FeetType == FEET_TYPE_DARKLING_QUADRO)
		{
			if(m_StairMoveType != FOOT_MOVEMENT_STAIR_DOWN && m_StairMoveType != FOOT_MOVEMENT_STAIR_DOWN )
			{
				m_IKFlags &= ~IKFLAGS_FRONTLSHOULDCOLLIDE;
				m_IKFlags &= ~IKFLAGS_FRONTRSHOULDCOLLIDE;
				pCDFirst->m_aLegIKIsDirty[2] = false; // 2 = Frontleft;
				pCDFirst->m_aLegIKIsDirty[3] = false; // 3 = Frontright;
			}
		}
	}
	else
	{
		m_IKFlags &= ~IKFLAGS_ISSLOWERTHANRUNNING;
		m_IKFlags &= ~IKFLAGS_LSHOULDCOLLIDE;
		m_IKFlags &= ~IKFLAGS_RSHOULDCOLLIDE;

		if(m_StairMoveType == FOOT_MOVEMENT_STAIR_DOWN || m_StairMoveType == FOOT_MOVEMENT_STAIR_UP)
		{
			/*
			if(pCDFirst->m_aLegIKIsDirty[1])
				m_IKFlags |= IKFLAGS_LSHOULDCOLLIDE; // 1 = left;
			else 
				m_IKFlags &= ~IKFLAGS_LSHOULDCOLLIDE;

			if(pCDFirst->m_aLegIKIsDirty[0])
				m_IKFlags |= IKFLAGS_RSHOULDCOLLIDE; // 0 = right;
			else 
				m_IKFlags &= ~IKFLAGS_RSHOULDCOLLIDE;
			*/

			bSolveRightLeg = true;
			bSolveLeftLeg =  true;

			// setup for running down stairs
			m_MatLeftFootAnim.k[3][2] -= m_Diffr;
			m_MatRightFootAnim.k[3][2] -= m_Diffr;

			if(_FeetType == FEET_TYPE_DARKLING_QUADRO)
			{
				if(pCDFirst->m_aLegIKIsDirty[2])
					m_IKFlags |= IKFLAGS_FRONTLSHOULDCOLLIDE; 
				else
					m_IKFlags &= ~IKFLAGS_FRONTLSHOULDCOLLIDE;

				if(pCDFirst->m_aLegIKIsDirty[3])
					m_IKFlags |= IKFLAGS_FRONTRSHOULDCOLLIDE;
				else
					m_IKFlags &= ~IKFLAGS_FRONTRSHOULDCOLLIDE;
			}
		}		
	}

	if(_FeetType == FEET_TYPE_DARKLING_QUADRO)
	{		
		if(_IsWallClimber &&  (m_IKFlags & IKFLAGS_FEETSETUPOK))
		{
			// at the moment of switch, the values are un-initialized
			if(!(m_IKFlags & IKFLAGS_WASWALLCLIMBERLASTFRAME))
			{
				m_CurLFPAPoint = m_MatLeftFootAnim.GetRow(3);
				m_CurRFPAPoint = m_MatRightFootAnim.GetRow(3);
				m_CurLFrontFootPAPoint = m_MatFrontLeftFootAnim.GetRow(3);
				m_CurRFrontFootPAPoint = m_MatFrontRightFootAnim.GetRow(3);

				m_LeftFrontFootDestination = m_MatFrontLeftFootAnim;
				m_RightFrontFootDestination = m_MatFrontRightFootAnim;
				m_LeftFootDestination = m_MatLeftFootAnim;
				m_RightFootDestination = m_MatRightFootAnim;
			}
			else
			{
				m_LeftFrontFootDestination = m_MatFrontLeftFootAnim;
				if(m_IKFlags & IKFLAGS_LEFTFRONTFOOTHASCOLLISION)
					GetPointOnVector(&m_LeftFrontFootTarget, &m_MatFrontLeftFootAnim.GetRow(3), &m_LeftFrontFootDestination.GetRow(3));

				m_RightFrontFootDestination = m_MatFrontRightFootAnim;
				if(m_IKFlags & IKFLAGS_RIGHTFRONTFOOTHASCOLLISION)
					GetPointOnVector(&m_RightFrontFootTarget, &m_MatFrontRightFootAnim.GetRow(3), &m_RightFrontFootDestination.GetRow(3));

				m_LeftFootDestination = m_MatLeftFootAnim;
				if(m_IKFlags & IKFLAGS_LEFTBACKFOOTHASCOLLISION)
					GetPointOnVector(&m_LeftBackFootTarget, &m_MatLeftFootAnim.GetRow(3), &m_LeftFootDestination.GetRow(3));

				m_RightFootDestination = m_MatRightFootAnim;
				if(m_IKFlags & IKFLAGS_RIGHTBACKFOOTHASCOLLISION)
					GetPointOnVector(&m_RightBackFootTarget, &m_MatRightFootAnim.GetRow(3), &m_RightFootDestination.GetRow(3));

				// the only modified movement of the foot we want the movement i the darklings UP-direction
				GetPointOnVector(&m_CurLFPAPoint, &m_LeftFootDestination.GetRow(3), &m_CurLFPAPoint);
				GetPointOnVector(&m_CurRFPAPoint, &m_RightFootDestination.GetRow(3), &m_CurRFPAPoint);
				GetPointOnVector(&m_CurLFrontFootPAPoint, &m_LeftFrontFootDestination.GetRow(3), &m_CurLFrontFootPAPoint);
				GetPointOnVector(&m_CurRFrontFootPAPoint, &m_RightFrontFootDestination.GetRow(3), &m_CurRFrontFootPAPoint);

				/*
				// never ever go through floor (rather snap)
				m_CurLFPAPoint.k[2] = Max(m_CurLFPAPoint.k[2], m_LeftBackFootTarget.k[2]);
				m_CurRFPAPoint.k[2] = Max(m_CurRFPAPoint.k[2], m_RightBackFootTarget.k[2]);
				m_CurLFrontFootPAPoint.k[2] = Max(m_CurLFrontFootPAPoint.k[2], m_LeftFrontFootTarget.k[2]);
				m_CurRFrontFootPAPoint.k[2] = Max(m_CurRFrontFootPAPoint.k[2], m_RightFrontFootTarget.k[2]);
				*/

			}
		}
		else if(m_IKFlags & IKFLAGS_FEETSETUPOK)
		{
			m_LeftFrontFootDestination = m_MatFrontLeftFootAnim;
			if(m_IKFlags & IKFLAGS_LEFTFRONTFOOTHASCOLLISION)
				m_LeftFrontFootDestination.k[3][2] = m_LeftFrontFootTarget.k[2];

			m_RightFrontFootDestination = m_MatFrontRightFootAnim;
			if(m_IKFlags & IKFLAGS_RIGHTFRONTFOOTHASCOLLISION)
				m_RightFrontFootDestination.k[3][2] = m_RightFrontFootTarget.k[2];

			m_LeftFootDestination = m_MatLeftFootAnim;
			if(m_IKFlags & IKFLAGS_LEFTBACKFOOTHASCOLLISION)
				m_LeftFootDestination.k[3][2] = m_LeftBackFootTarget.k[2];

			m_RightFootDestination= m_MatRightFootAnim;
			if(m_IKFlags & IKFLAGS_RIGHTBACKFOOTHASCOLLISION)
				m_RightFootDestination.k[3][2] = m_RightBackFootTarget.k[2];

			m_CurLFPAPoint.k[0] = m_MatLeftFootAnim.k[3][0];
			m_CurLFPAPoint.k[1] = m_MatLeftFootAnim.k[3][1];

			m_CurRFPAPoint.k[0] = m_MatRightFootAnim.k[3][0];
			m_CurRFPAPoint.k[1] = m_MatRightFootAnim.k[3][1];

			m_CurLFrontFootPAPoint.k[0] = m_MatFrontLeftFootAnim.k[3][0];
			m_CurLFrontFootPAPoint.k[1] = m_MatFrontLeftFootAnim.k[3][1];

			m_CurRFrontFootPAPoint.k[0] = m_MatFrontRightFootAnim.k[3][0];
			m_CurRFrontFootPAPoint.k[1] = m_MatFrontRightFootAnim.k[3][1];

			// never ever go through floor (rather snap)
			m_CurLFPAPoint.k[2] = Max(m_CurLFPAPoint.k[2], m_LeftBackFootTarget.k[2]);
			m_CurRFPAPoint.k[2] = Max(m_CurRFPAPoint.k[2], m_RightBackFootTarget.k[2]);
			m_CurLFrontFootPAPoint.k[2] = Max(m_CurLFrontFootPAPoint.k[2], m_LeftFrontFootTarget.k[2]);
			m_CurRFrontFootPAPoint.k[2] = Max(m_CurRFrontFootPAPoint.k[2], m_RightFrontFootTarget.k[2]);
		}
		else // feet setup not OK
		{
			m_CurLFPAPoint = m_MatLeftFootAnim.GetRow(3);
			m_CurRFPAPoint= m_MatRightFootAnim.GetRow(3);
			m_CurLFrontFootPAPoint = m_MatFrontLeftFootAnim.GetRow(3);
			m_CurRFrontFootPAPoint = m_MatFrontRightFootAnim.GetRow(3);

			m_LeftFrontFootDestination = m_MatFrontLeftFootAnim;
			m_RightFrontFootDestination = m_MatFrontRightFootAnim;
			m_LeftFootDestination = m_MatLeftFootAnim;
			m_RightFootDestination = m_MatRightFootAnim;

			m_LeftBackFootTarget = m_MatLeftFootAnim.GetRow(3);
			m_RightBackFootTarget = m_MatRightFootAnim.GetRow(3);

			m_IKFlags |= IKFLAGS_FEETSETUPOK;
		}

		if(_IsWallClimber)
			m_IKFlags |= IKFLAGS_WASWALLCLIMBERLASTFRAME;
		else
			m_IKFlags &= ~IKFLAGS_WASWALLCLIMBERLASTFRAME;
	}
	else // is biped
	{
		bFrontSolveLeftLeg  = false;
		bFrontSolveRightLeg = false;

		m_LeftFootDestination = m_MatLeftFootAnim;
		if(m_IKFlags & IKFLAGS_LEFTBACKFOOTHASCOLLISION)
			m_LeftFootDestination.k[3][2] = m_LeftBackFootTarget.k[2];

		m_RightFootDestination = m_MatRightFootAnim;
		if(m_IKFlags & IKFLAGS_RIGHTBACKFOOTHASCOLLISION)
			m_RightFootDestination.k[3][2] = m_RightBackFootTarget.k[2];


		// the only modified movement of the foot we want is the horizontal movement
		m_CurLFPAPoint.k[0] = m_MatLeftFootAnim.k[3][0];
		m_CurLFPAPoint.k[1] = m_MatLeftFootAnim.k[3][1];

		m_CurRFPAPoint.k[0] = m_MatRightFootAnim.k[3][0];
		m_CurRFPAPoint.k[1] = m_MatRightFootAnim.k[3][1];

		if(!(m_IKFlags & IKFLAGS_FEETSETUPOK))
		{
			m_CurLFPAPoint.k[2] = m_MatLeftFootAnim.k[3][2];
			m_CurRFPAPoint.k[2] = m_MatRightFootAnim.k[3][2];
			m_LeftBackFootTarget.k[2] = m_MatLeftFootAnim.k[3][2];
			m_RightBackFootTarget.k[2] = m_MatRightFootAnim.k[3][2];
			m_LeftFootDestination = m_MatLeftFootAnim;
			m_RightFootDestination = m_MatRightFootAnim;
			m_IKFlags |= IKFLAGS_FEETSETUPOK;
		}
		else
		{
			if(!(m_IKFlags & IKFLAGS_STANDINGSTILL) && m_StairMoveType != FOOT_MOVEMENT_STAIR_UP) // need to be able to go through geometry a bit in idle mode
			{
				// never ever go through floor (rather snap)
				m_CurLFPAPoint.k[2] = Max(m_CurLFPAPoint.k[2], m_LeftBackFootTarget.k[2]);
				m_CurRFPAPoint.k[2] = Max(m_CurRFPAPoint.k[2], m_RightBackFootTarget.k[2]);
			}
		}
	}



	//----------------------------------------------------------------------------------
	// conditions
	if((_FeetType == FEET_TYPE_HUMAN_BIPED && 
		(m_StairMoveType == FOOT_MOVEMENT_ANIMDRIVEN || m_StairMoveType == FOOT_MOVEMENT_UNDEFINED)))
	{
		if(m_Diffr == 0.0f)
		{
			// these needs to be set since catchup value is very low in idle-mode, and we don't want the lagging behind
			m_CurLFPAPoint = m_MatLeftFootAnim.GetRow(3);
			m_CurRFPAPoint = m_MatRightFootAnim.GetRow(3);

			//pCDFirst->m_aLegIKIsDirty[1] = false; // 1 = left;
			//pCDFirst->m_aLegIKIsDirty[0] = false; // 0 = right;

			bSolveLeftLeg  = false;
			bSolveRightLeg = false;

			// when these two are false, the player will rise to the physbox-pos
			m_IKFlags &= ~IKFLAGS_RIGHTBACKFOOTHASCOLLISION; 
			m_IKFlags &= ~IKFLAGS_LEFTBACKFOOTHASCOLLISION;
		}
		else
		{
			m_CurLFPAPoint.k[2] = m_LeftBackFootTarget.k[2]; //  never ever go through floor (rather snap)
			m_CurRFPAPoint.k[2] = m_RightBackFootTarget.k[2]; //never ever go through floor
		}

	}


	//------------------------------------------------------------------------------------
	// Adjust target, so movement happens over time
	CVec3Dfp32 RightFinalDestination = m_RightFootDestination.GetRow(3);
	CVec3Dfp32 LeftFinalDestination  = m_LeftFootDestination.GetRow(3);

	if(bSolveLeftLeg)
	{
		// special for blending in out for darkling
			
		if(m_IKFlags & IKFLAGS_LEFTBACKFOOTHASCOLLISION)
			MoveFootTowardsTarget(true, &bSolveLeftLeg, &m_MatLeftFootAnim, &m_LeftFootDestination, &m_CurLFPAPoint, &DownVec, bInAir);
		else
			MoveFootTowardsTarget(false, &bSolveLeftLeg, &m_MatLeftFootAnim, &m_LeftFootDestination, &m_CurLFPAPoint, &DownVec, bInAir);
	}

	if(bSolveRightLeg)
	{
		if(m_IKFlags & IKFLAGS_RIGHTBACKFOOTHASCOLLISION)
			MoveFootTowardsTarget(true, &bSolveRightLeg, &m_MatRightFootAnim, &m_RightFootDestination, &m_CurRFPAPoint, &DownVec, bInAir);
		else
			MoveFootTowardsTarget(false, &bSolveRightLeg, &m_MatRightFootAnim, &m_RightFootDestination, &m_CurRFPAPoint, &DownVec, bInAir);
	}

	if(bFrontSolveLeftLeg)
	{
		if((m_IKFlags & IKFLAGS_LEFTFRONTFOOTHASCOLLISION) && !(_FeetType == FEET_TYPE_DARKLING_QUADRO && (pCDFirst->m_AnimGraph2.GetStateFlagsHiCombined() & AG2_STATEFLAGHI_IKSYSTEM_IGNOREHANDS)))
			MoveFootTowardsTarget(true, &bFrontSolveLeftLeg, &m_MatFrontLeftFootAnim, &m_LeftFrontFootDestination, &m_CurLFrontFootPAPoint, &DownVec, bInAir);
		else
			MoveFootTowardsTarget(false, &bFrontSolveLeftLeg, &m_MatFrontLeftFootAnim, &m_LeftFrontFootDestination, &m_CurLFrontFootPAPoint, &DownVec, bInAir);

	}

	if(bFrontSolveRightLeg)
	{
		if((m_IKFlags & IKFLAGS_RIGHTFRONTFOOTHASCOLLISION) && !(_FeetType == FEET_TYPE_DARKLING_QUADRO && (pCDFirst->m_AnimGraph2.GetStateFlagsHiCombined() & AG2_STATEFLAGHI_IKSYSTEM_IGNOREHANDS)))
			MoveFootTowardsTarget(true, &bFrontSolveRightLeg, &m_MatFrontRightFootAnim, &m_RightFrontFootDestination, &m_CurRFrontFootPAPoint, &DownVec, bInAir);
		else
			MoveFootTowardsTarget(false, &bFrontSolveRightLeg, &m_MatFrontRightFootAnim, &m_RightFrontFootDestination, &m_CurRFrontFootPAPoint, &DownVec, bInAir);

	}


	//----------------------------------------------------------------------------------
	// alter body position if needed
	if(_FeetType == FEET_TYPE_HUMAN_BIPED)
	{
		if((m_IKFlags & IKFLAGS_RIGHTBACKFOOTHASCOLLISION) || (m_IKFlags & IKFLAGS_LEFTBACKFOOTHASCOLLISION))
		{
			int8 iParentNode = _pSkel->m_lNodes[Node_RBL].m_iNodeParent;
			CVec3Dfp32 RLegPointWrld = _pSkel->m_lNodes[Node_RBL].m_LocalCenter * _pSkelInstance->m_pBoneTransform[iParentNode];

			iParentNode = _pSkel->m_lNodes[Node_LBL].m_iNodeParent;
			CVec3Dfp32 LLegPointWrld = _pSkel->m_lNodes[Node_LBL].m_LocalCenter * _pSkelInstance->m_pBoneTransform[iParentNode];

			iParentNode = _pSkel->m_lNodes[Node_RBK].m_iNodeParent;
			CVec3Dfp32 RKneePointWrld = _pSkel->m_lNodes[Node_RBK].m_LocalCenter * _pSkelInstance->m_pBoneTransform[iParentNode];

			fp32 CombinedBoneLength = (RKneePointWrld - RLegPointWrld).Length() + (m_MatRightFootAnim.GetRow(3)  - RKneePointWrld).Length() + 0.5f;

			// needs to move down? how much? who will decide?
			bool RightDecides = false;
			bool LeftDecides  = false;		

			fp32 RHipToFootTargetLength = (RightFinalDestination - RLegPointWrld).Length();
			fp32 LHipToFootTargetLength = (LeftFinalDestination - LLegPointWrld).Length();

			if(RHipToFootTargetLength > LHipToFootTargetLength && (RHipToFootTargetLength > CombinedBoneLength))
			{
				RightDecides = true;
				LeftDecides = false;
			}
			else if(LHipToFootTargetLength > RHipToFootTargetLength && (LHipToFootTargetLength > CombinedBoneLength))
			{
				LeftDecides = true;
				RightDecides = false;
			}	

			//--------------------------------------------------------------------------
			// special case: if running down a stair, it's important what foot decides
			// the player height
			/*
			if((m_StairMoveType == FOOT_MOVEMENT_STAIR_DOWN || m_StairMoveType == FOOT_MOVEMENT_STAIR_UP) && !(m_IKFlags & IKFLAGS_ISSLOWERTHANRUNNING))
			{
				LeftDecides = false;
				RightDecides = false;

				if(pCDFirst->m_aLegIKIsDirty[1])
					LeftDecides = true;
				else if(pCDFirst->m_aLegIKIsDirty[0])
					RightDecides = true;
			}

			if(LeftDecides && RightDecides) // special case: when both legs are out of reach
			{
				LeftDecides = false;
				RightDecides = false;

				if(LHipToFootTargetLength > RHipToFootTargetLength)
					LeftDecides = true;
				else
					RightDecides = true;
			}
			*/

			if(!LeftDecides && !RightDecides && m_Diffr != 0.0f) // noone decides
			{
				// when running down a stair, we need to bounce back up very slow to avoid jerky motion
				if(m_IKFlags & IKFLAGS_ISSLOWERTHANRUNNING)
				{
					if(m_StairMoveType == FOOT_MOVEMENT_STAIR_UP || m_StairMoveType == FOOT_MOVEMENT_STAIR_DOWN)
						m_Diffr -= PLAYERLOWERSPEED * m_fDeltaTime * 1.01f;
					else
						m_Diffr -= PLAYERLOWERSPEED * m_fDeltaTime;	
				} 

				m_Diffr = Max(m_Diffr, 0.0f);
			}
			else
			{
				// this is the Magic. first foot goes to target over time, but also player is lower over time 
				// depending on the foot distance
				fp32 Diffr = 0.0f;
				if(LeftDecides)
					Diffr = m_MatLeftFootAnim.k[3][2]  - m_LeftFootDestination.k[3][2];
				else if(RightDecides)
					Diffr = m_MatRightFootAnim.k[3][2] - m_RightFootDestination.k[3][2];

				Diffr = Max(Diffr, 0.0f);
				Diffr = Min(Diffr, 15.0f);

				if(Diffr < m_Diffr)
				{
					m_Diffr -= PLAYERLOWERSPEED * m_fDeltaTime * 1.01f;
					m_Diffr = Max(m_Diffr, Diffr);
				}
				else if(Diffr > m_Diffr)
				{
					m_Diffr += PLAYERLOWERSPEED * m_fDeltaTime * 1.5f;
					m_Diffr = Min(m_Diffr, Diffr);
				}
			}			
		}
		else if(m_Diffr != 0.0f) // no feet collisions at all. move back towards orig pos
		{
			if(m_StairMoveType == FOOT_MOVEMENT_STAIR_UP)
				m_Diffr -= PLAYERLOWERSPEED * m_fDeltaTime * 1.01f;
			else
				m_Diffr -= PLAYERLOWERSPEED * m_fDeltaTime;

			m_Diffr = Max(m_Diffr, 0.0f);	
		}

		//----------------------------------------------------------------------------------
		// override physbox if needed
		m_IKFlags |= IKFLAGS_OVERRIDEPOSITION;
		if(m_Diffr == 0.0f)
			m_IKFlags &= ~IKFLAGS_OVERRIDEPOSITION;
		else
			m_PosOverride = m_ModelNode0.GetRow(3) - (DownVec * m_Diffr);

		// raise/lower player
		if(m_IKFlags & IKFLAGS_OVERRIDEPOSITION)
		{
			// Lower the player as much as necessary
			CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(_pSkelInstance->m_pBoneTransform[0],3);
			CMat4Dfp32 GlobalScale;
			GlobalScale.Unit();
			fp32 TestScale = pCDFirst->m_CharGlobalScale;
			GlobalScale.k[0][0] *= TestScale;
			GlobalScale.k[1][1] *= TestScale;
			GlobalScale.k[2][2] *= TestScale;
			Pos = Pos - Pos * TestScale;
			Pos.SetMatrixRow(GlobalScale,3);
			CMat4Dfp32 Temp;
			_pSkelInstance->m_pBoneTransform[0].Multiply(GlobalScale,Temp);
			Temp.GetRow(3) = m_PosOverride;
			_pSkel->EvalNode(PLAYER_ROTTRACK_ROOT, &Temp, _pSkelInstance);
		}

		// ---------------------------------------------------------------------
		// Solve the IK

		if(bSolveRightLeg)
		{
			if(m_FeetBlendVal != 1.0f)
			{
				CVec3Dfp32 MoveDir = (m_RightFootDestination.GetRow(3) - m_MatRightFootAnim.GetRow(3)); // could interpolate w/ quaternions
				MoveDir *= m_FeetBlendVal;
				m_RightFootDestination.GetRow(3) = m_MatRightFootAnim.GetRow(3) + MoveDir;
			}

			// this is ONLY for foot movement in stairs (no other animation has those flags set)
			CMat4Dfp32 OverrideMat = m_RightFootDestination;
			AddZMovementForFoot(pCDFirst, 0, m_RightFootSin, OverrideMat, 0.35f, 5.0f);

			m_IKSolver.SetIKMode(CIKSystem::IK_MODE_RIGHT_FOOT);
			m_IKSolver.DoDualHandIK(_pSkel, _pSkelInstance, &OverrideMat, _pWPhysState, pCDFirst);
		}
		else
		{
			m_RightFootDestination = m_MatRightFootAnim;
			m_CurRFPAPoint = m_MatRightFootAnim.GetRow(3);
		}

		if(bSolveLeftLeg)
		{
			if(m_FeetBlendVal != 1.0f)
			{
				CVec3Dfp32 MoveDir = (m_LeftFootDestination.GetRow(3) - m_MatLeftFootAnim.GetRow(3)); // could interpolate w/ quaternions
				MoveDir *= Sinc(m_FeetBlendVal);
				m_LeftFootDestination.GetRow(3) = m_MatLeftFootAnim.GetRow(3) + MoveDir;
			}


			CMat4Dfp32 OverrideMat = m_LeftFootDestination;
			AddZMovementForFoot(pCDFirst, 1, m_LeftFootSin, OverrideMat, 0.35f, 5.0f);

			m_IKSolver.SetIKMode(CIKSystem::IK_MODE_LEFT_FOOT);
			m_IKSolver.DoDualHandIK(_pSkel, _pSkelInstance, &OverrideMat, _pWPhysState, pCDFirst);
		}
		else
		{
			m_LeftFootDestination = m_MatLeftFootAnim;
			m_CurLFPAPoint = m_MatLeftFootAnim.GetRow(3);
		}

	}
	else // is quadroped
	{
		// ---------------------------------------------------------------------
		// Solve the IK

		/*
		if(!(pCDFirst->m_AnimGraph2.GetStateFlagsHiCombined() & AG2_STATEFLAGHI_IKSYSTEM_IGNOREHANDS))
		{
			bFrontSolveRightLeg = true;
			bFrontSolveLeftLeg  = true;
		}
		else
		{
			bFrontSolveRightLeg = false;
			bFrontSolveLeftLeg  = false;
		}
		if(!(m_IKFlags & IKFLAGS_ISSLOWERTHANRUNNING))
		{
		//bSolveLeftLeg = false;
		//bSolveRightLeg = false;
		}
		*/
		

		// the catch.. if stairs up/down, these are animdriven so... hmm.. need to fix.
		//bSolveLeftLeg = true;
		//bSolveRightLeg = true;

		bFrontSolveRightLeg = true;
		bFrontSolveLeftLeg  = true;

		if(bSolveRightLeg)
		{
			if(m_FeetBlendVal != 1.0f)
			{
				CVec3Dfp32 MoveDir = (m_RightFootDestination.GetRow(3) - m_MatRightFootAnim.GetRow(3)); // could interpolate w/ quaternions
				MoveDir *= Sinc(m_FeetBlendVal);
				m_RightFootDestination.GetRow(3) = m_MatRightFootAnim.GetRow(3) + MoveDir;
			}
			m_IKSolver.SetIKMode(CIKSystem::IK_MODE_DARKLING_RIGHT_BACK_FOOT);
			m_IKSolver.DoDualHandIK(_pSkel, _pSkelInstance, &m_RightFootDestination, _pWPhysState, pCDFirst);
		}

		if(bSolveLeftLeg)
		{
			if(m_FeetBlendVal != 1.0f)
			{
				CVec3Dfp32 MoveDir = (m_LeftFootDestination.GetRow(3) - m_MatLeftFootAnim.GetRow(3)); // could interpolate w/ quaternions
				MoveDir *= Sinc(m_FeetBlendVal);
				m_LeftFootDestination.GetRow(3) = m_MatLeftFootAnim.GetRow(3) + MoveDir;
			}
			m_IKSolver.SetIKMode(CIKSystem::IK_MODE_DARKLING_LEFT_BACK_FOOT);
			m_IKSolver.DoDualHandIK(_pSkel, _pSkelInstance, &m_LeftFootDestination, _pWPhysState, pCDFirst);
		}


		if(bFrontSolveRightLeg)
		{
			if(m_FrontFeetBlendVal != 1.0f)
			{
				CVec3Dfp32 MoveDir = (m_RightFrontFootDestination.GetRow(3) - m_MatFrontRightFootAnim.GetRow(3)); // could interpolate w/ quaternions
				MoveDir *= Sinc(m_FrontFeetBlendVal);
				m_RightFrontFootDestination.GetRow(3) = m_MatFrontRightFootAnim.GetRow(3) + MoveDir;
			}
			m_IKSolver.SetIKMode(CIKSystem::IK_MODE_DARKLING_RIGHT_FRONT_FOOT);
			m_IKSolver.DoDualHandIK(_pSkel, _pSkelInstance, &m_RightFrontFootDestination, _pWPhysState, pCDFirst);
		}

		if(bFrontSolveLeftLeg)
		{
			if(m_FrontFeetBlendVal != 1.0f)
			{
				CVec3Dfp32 MoveDir = (m_LeftFrontFootDestination.GetRow(3) - m_MatFrontLeftFootAnim.GetRow(3)); // could interpolate w/ quaternions
				MoveDir *= Sinc(m_FrontFeetBlendVal);
				m_LeftFrontFootDestination.GetRow(3) = m_MatFrontLeftFootAnim.GetRow(3) + MoveDir;
			}
			m_IKSolver.SetIKMode(CIKSystem::IK_MODE_DARKLING_LEFT_FRONT_FOOT);
			m_IKSolver.DoDualHandIK(_pSkel, _pSkelInstance, &m_LeftFrontFootDestination, _pWPhysState, pCDFirst);
		}

		// back rotation for corners
		//RotateBoneAroundAxis(m_DarklingBackRotation, &_pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_SPINE].GetRow(1), 
		//	PLAYER_ROTTRACK_SPINE, _pSkel, _pSkelInstance, _pWPhysState);

		// lastly fix hands. only apply if can't reach ground
		if(bFrontSolveLeftLeg)
		{
			int8 iParentNode = _pSkel->m_lNodes[Node_LFL].m_iNodeParent;
			CVec3Dfp32 LLegPointWrld = _pSkel->m_lNodes[Node_LFL].m_LocalCenter * _pSkelInstance->m_pBoneTransform[iParentNode];

			iParentNode = _pSkel->m_lNodes[Node_LFK].m_iNodeParent;
			CVec3Dfp32 LKneePointWrld = _pSkel->m_lNodes[Node_LFK].m_LocalCenter * _pSkelInstance->m_pBoneTransform[iParentNode];

			fp32 ArmElbowHand = (LKneePointWrld - LLegPointWrld).Length() + (m_MatFrontLeftFootAnim.GetRow(3) - LKneePointWrld).Length();

			fp32 TravelLength = (m_LeftFrontFootDestination.GetRow(3) - LLegPointWrld).Length();
			if((ArmElbowHand + 3.0f) < TravelLength +10)
			{

			}
			else
			{
				m_LeftFrontFootRotation -= m_fDeltaTime * 0.20f;
				m_LeftFrontFootRotation = Max(m_LeftFrontFootRotation, 0.0f);
			}

			CVec3Dfp32 RotAxis = -m_LeftFrontFootDestination.GetRow(0);
			RotateBoneAroundAxis(m_LeftFrontFootRotation * Sinc(m_FrontFeetBlendVal), &RotAxis, PLAYER_ROTTRACK_LHAND, _pSkel, _pSkelInstance, _pWPhysState, true);
		}
		if(bFrontSolveRightLeg)
		{
			int8 iParentNode = _pSkel->m_lNodes[Node_RFL].m_iNodeParent;
			CVec3Dfp32 RLegPointWrld = _pSkel->m_lNodes[Node_RFL].m_LocalCenter * _pSkelInstance->m_pBoneTransform[iParentNode];

			iParentNode = _pSkel->m_lNodes[Node_RFK].m_iNodeParent;
			CVec3Dfp32 RKneePointWrld = _pSkel->m_lNodes[Node_RFK].m_LocalCenter * _pSkelInstance->m_pBoneTransform[iParentNode];

			fp32 ArmElbowHand = (RKneePointWrld - RLegPointWrld).Length() + (m_MatFrontRightFootAnim.GetRow(3) - RKneePointWrld).Length();

			fp32 TravelLength = (m_RightFrontFootDestination.GetRow(3) - RLegPointWrld).Length();
			if((ArmElbowHand + 3.0f) < TravelLength +10)
			{		

			}
			else
			{
				m_RightFrontFootRotation -= m_fDeltaTime * 0.20f;
				m_RightFrontFootRotation = Max(m_RightFrontFootRotation, 0.0f);
			}

			CVec3Dfp32 RotAxis = m_ModelNode0.GetRow(1);
			RotateBoneAroundAxis(m_RightFrontFootRotation * Sinc(m_FrontFeetBlendVal), &RotAxis, PLAYER_ROTTRACK_RHAND, _pSkel, _pSkelInstance, _pWPhysState, true);
		}
	}
#endif
}

//------------------------------------------------------------------------
// modifies position of foot (half sinus-curve) so the foot wont clip through
// the stairs. this is done because the foot actually follows the ground.
// The bools are set in the animation.
//
void CPostAnimSystem::AddZMovementForFoot(CWO_Character_ClientData *pCD, int _Dirt, fp32 &_FootVal, CMat4Dfp32 &_OverrideMat, fp32 _Duration, fp32 _HeightMod)
{
	if(pCD->m_aLegIKIsDirty[_Dirt])
	{
		_FootVal += m_fDeltaTime;
		_FootVal = Min(_FootVal, _Duration);
		fp32 Val = (_FootVal / _Duration); // val is from 0 to 1
		Val = M_Sin(Val*_PI) * _HeightMod;
		_OverrideMat.GetRow(3).k[2] += Val;
	}
	else
		_FootVal = 0.0f;
}
// -----------------------------------------------------------------------------------------------------
// Sets the foottarget to go from current foot position towards destination
// notes:
// bool _FootHasCollision		- Set if the foot not is in mid-air (almost always set)
// bool *_SolveLegIK			- Used as a return value. Set to false if the foot is at the animation point
// CMat4Dfp32 *_pAnimFootMat		- The foot position according to the animation
// CMat4Dfp32 *_FootDestination	- Used as a return value. this sets the interpolated point that the IK uses to set the foot position
// CVec3Dfp32 *_CurPAPoint		- The current footpos (animpos + towards target value)
// CVec3Dfp32 *_DownVec			- Characters downvector (humans always -Z)
// bool _bIsFalling				- Set if the chars physbox has been in-air for 6 frames
// bool _bIsIdle				- Set if the char is almost or completely idle
//
void CPostAnimSystem::MoveFootTowardsTarget(bool _FootHasCollision, bool *_SolveLegIK, const CMat4Dfp32 *_pAnimFootMat, 
											CMat4Dfp32 *_FootDestination, CVec3Dfp32 *_CurPAPoint, const CVec3Dfp32 *_DownVec, 
											bool _bIsFalling)
{
	CVec3Dfp32 TravelVec		= 0;
	fp32 fTravelLength		= 0.0f;
	fp32 CachUpValue			= 0.0f;
	fp32 CurToAnimOffset		= 0.0f;
	fp32 IgnoreIfLowerThan	= 0.01f;
	fp32 CatchUpSpeed		= 15.f;

	if(((m_IKFlags & IKFLAGS_ISSLOWERTHANRUNNING)|| (m_IKFlags & IKFLAGS_STANDINGSTILL)) && m_StairMoveType != FOOT_MOVEMENT_STAIR_UP)
		CatchUpSpeed = 4.5;

	// If it has a collision, move it towards collision, else move it towards animation point
	if(_FootHasCollision)
		TravelVec = (_FootDestination->GetRow(3) - *_CurPAPoint);
	else // move foot back towards orig position
		TravelVec = (_pAnimFootMat->GetRow(3) - *_CurPAPoint);

	fTravelLength = TravelVec.Length();
	CachUpValue = m_fDeltaTime * CatchUpSpeed * (fTravelLength * 2.0f); // faster if distance is great
	CachUpValue = Max(CachUpValue, 0.33f); // not too slow

	// reached target
	CachUpValue = Min(CachUpValue, fTravelLength);

	*_CurPAPoint += TravelVec.Normalize() * CachUpValue;
	_FootDestination->GetRow(3) = *_CurPAPoint;

	// do we need to IK-solve this foot?
	/*if(m_Diffr == 0.0f)
	{
	CurToAnimOffset = (_FootDestination->GetRow(3) - (_pAnimFootMat->GetRow(3))).Length();
	if(CachUpValue < IgnoreIfLowerThan)
	*_SolveLegIK = false;
	}*/
}

//----------------------------------------------------------------------------------------
//
// if aim point supplied, use that one to aim the weapon(s), or grab a certain point (sharing times and point member variables)
// can be used to release BOTH aimpoints ONLY as well, but use "CPostAnimSystem::ReleaseAimPoint" insteadn of that
//
void CPostAnimSystem::SetGrabPoint(const CVec3Dfp32 *_pAimPointLeft, fp32 _AimTimeLeft, const CVec3Dfp32 *_pAimPointRight, fp32 _AimTimeRight)
{
	if(!_pAimPointLeft && !_pAimPointRight)
		ReleaseGrabPoint(true, true);

	if(_pAimPointLeft)
	{
		m_IKFlags |= IKFLAGS_GRABLEFTHAND;
		m_IKFlags &= ~IKFLAGS_RELEASELEFT_GRAB;

		m_AimPointLeft = *_pAimPointLeft;
		m_AimTimeLeft = _AimTimeLeft;
		m_AimTimeStoreLeft = _AimTimeLeft;
		m_AimBlendTimeLeft = 0.75f;
	}

	if(_pAimPointRight)
	{
		m_IKFlags |= IKFLAGS_GRABRIGHTHAND;
		m_IKFlags &= ~IKFLAGS_RELEASERIGHT_GRAB;

		m_AimPointRight = *_pAimPointRight;
		m_AimTimeRight = _AimTimeRight;
		m_AimTimeStoreRight = _AimTimeRight;
		m_AimBlendTimeRight = 0.75f;	
	}
}

void CPostAnimSystem::ReleaseGrabPoint(bool _bReleaseLeft, bool _bReleaseRight)
{
	if(_bReleaseLeft && (m_IKFlags & IKFLAGS_GRABLEFTHAND))
	{
		m_IKFlags &= ~IKFLAGS_GRABLEFTHAND;
		m_IKFlags |= IKFLAGS_RELEASELEFT_GRAB;
		m_AimTimeLeft = 0;
	}

	if(_bReleaseRight && (m_IKFlags & IKFLAGS_GRABRIGHTHAND))
	{
		m_IKFlags &= ~IKFLAGS_GRABRIGHTHAND;
		m_IKFlags |= IKFLAGS_RELEASERIGHT_GRAB;
		m_AimTimeRight = 0;
	}
}

//----------------------------------------------------------------------------------------------------------
//
// This sets up the hands for grabbing certain points
// returns true if still calculating and setting up hands for grab, false if all done
//
bool CPostAnimSystem::EvalHandsForGrab(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CWorld_PhysState* _pWPhysState, CWO_Character_ClientData *_pCD)
{
#ifndef OGIER
	if(!(m_IKFlags & IKFLAGS_GRABLEFTHAND) && !(m_IKFlags & IKFLAGS_GRABRIGHTHAND))
		return false;

	if(m_IKFlags & IKFLAGS_GRABLEFTHAND)
	{	
		if(m_CurrentWeaponBlendTypeLeft != WEAPONBLEND_TYPE_FULL_IN)
			m_CurrentWeaponBlendTypeLeft = WEAPONBLEND_TYPE_IN;
	}
	else if(m_CurrentWeaponBlendTypeLeft != WEAPONBLEND_TYPE_NONE)
	{
			m_CurrentWeaponBlendTypeLeft = WEAPONBLEND_TYPE_OUT;		
	}

	if(m_IKFlags & IKFLAGS_GRABRIGHTHAND)
	{
		if(m_CurrentWeaponBlendTypeRight != WEAPONBLEND_TYPE_FULL_IN)
			m_CurrentWeaponBlendTypeRight = WEAPONBLEND_TYPE_IN;
	}
	else if(m_CurrentWeaponBlendTypeRight != WEAPONBLEND_TYPE_NONE)
	{
		m_CurrentWeaponBlendTypeRight = WEAPONBLEND_TYPE_OUT;		
	}

	//-------------------------------------------------------------------------------------
	// blend towards target
	bool bStillProcessing = false;
	CMat4Dfp32 MatHandTarget;
	
	if((m_IKFlags & IKFLAGS_GRABLEFTHAND) || (m_IKFlags & IKFLAGS_RELEASELEFT_GRAB))
	{
		MatHandTarget = _pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_LHAND];
		MatHandTarget.GetRow(3) = m_AimPointLeft;

		if(BlendHandNodes(CIKSystem::IK_MODE_LEFT_HAND, &MatHandTarget, _pSkel, _pSkelInstance))
		{
			m_LeftHandBlendVal = Max(0.0f, m_LeftHandBlendVal);
			m_LeftHandBlendVal = Min(1.0f, m_LeftHandBlendVal);
			m_IKSolver.SetIKMode(CIKSystem::IK_MODE_LEFT_HAND);

			// in extreme formations, elbow will be twisted wrong, this locks it.
			CVec3Dfp32 ElbowDirLeft = m_CameraLookMat.GetRow(2) * -100.0f + m_CameraLookMat.GetRow(1) * 50.0f;
			m_IKSolver.DoDualHandIK(_pSkel, _pSkelInstance, &MatHandTarget, _pWPhysState, _pCD, &ElbowDirLeft);

			bStillProcessing = true;
		}
		else
			m_IKFlags &= ~IKFLAGS_RELEASELEFT_GRAB;
	}
	
	if((m_IKFlags & IKFLAGS_GRABRIGHTHAND) || (m_IKFlags & IKFLAGS_RELEASERIGHT_GRAB))
	{
		MatHandTarget = _pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_RHAND];
		MatHandTarget.GetRow(3) = m_AimPointRight;

		if(BlendHandNodes(CIKSystem::IK_MODE_RIGHT_HAND, &MatHandTarget, _pSkel, _pSkelInstance))
		{
			m_RightHandBlendVal = Max(0.0f, m_RightHandBlendVal);
			m_RightHandBlendVal = Min(1.0f, m_RightHandBlendVal);
			m_IKSolver.SetIKMode(CIKSystem::IK_MODE_RIGHT_HAND);

			// in extreme formations, elbow will be twisted wrong, this locks it.
			CVec3Dfp32 ElbowDirRight = m_CameraLookMat.GetRow(2) * -100.0f + m_CameraLookMat.GetRow(1) * -50.0f;
			m_IKSolver.DoDualHandIK(_pSkel, _pSkelInstance, &MatHandTarget, _pWPhysState, _pCD, &ElbowDirRight);

			bStillProcessing = true;
		}
		else
			m_IKFlags &= ~IKFLAGS_RELEASERIGHT_GRAB;
	}

	return bStillProcessing;
#else
	return false;
#endif
}

void CPostAnimSystem::GatherAimCollisionData(CWorld_PhysState *_pWPhysState, int _iTarget, bool _bIsPlayer)
{
#ifndef OGIER
	if(m_WeaponHoldType == WEAPONHOLD_TYPE_NOWEAPON)
		return;

	if(!_bIsPlayer && m_RightAimInAndOutBlendVal == 0.0f && m_LeftAimInAndOutBlendVal == 0.0f)
		return;

	//int iTarget =  _pCD->m_AimTarget;
	CWObject_CoreData *pTarget = NULL;
	CVec3Dfp32 TarPos=0;
	if(_iTarget)
	{
		pTarget = _pWPhysState->Object_GetCD(_iTarget);
		if(pTarget)
			TarPos = pTarget->GetPosition();
	}

	CCollisionInfo CollisInfo;
	CollisInfo.Clear();
	CollisInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
	CollisInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_POSITION);
	CVec3Dfp32 ColLineStart;
	CVec3Dfp32 ColLineEnd;

	bool bDoLineCollisionCheck = false;
	bool bIsHumanTarget = false;
	// if there is no target, just get an aim point
	if(!pTarget || (m_IKFlags & IKFLAGS_INWEAPONZOOMMODE))
	{
		if(_bIsPlayer) // AI players keep their abs aimpoint
		{
			ColLineStart = m_CameraLookMat.GetRow(3);
			ColLineEnd = ColLineStart + (m_CameraLookMat.GetRow(0) * 2048.0f);
			bDoLineCollisionCheck = true;
		}
	}
	else
	{
		uint ObjectFlags = pTarget->GetPhysState().m_ObjectFlags;
		bint bObject = (ObjectFlags & OBJECT_FLAGS_OBJECT);
		bint bCharacter = (ObjectFlags & OBJECT_FLAGS_CHARACTER);
		aint Ret = 0;

		//if (pTarget->IsClass(class_Object_Lamp) || !_bIsPlayer ) // IA always aims at aimpoint
		if (bObject || !_bIsPlayer)
		{
			aint Ret =_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETAUTOAIMOFFSET), _iTarget);
			if(Ret)
			{
				TarPos = CVec3Dfp32().Unpack32(Ret, 256.0f);
				if(TarPos.k[0] != 256.0f)// && TarPos.LengthSqr() > 1.0f) //  TODO: remove this heavy failsafe
					m_AbsAimTargetPos = TarPos * pTarget->GetPositionMatrix();				
			}
			else
			{
				m_AbsAimTargetPos = TarPos;
			}
		}
		else if (bCharacter && (CWObject_Character::Char_GetPhysType(pTarget) != PLAYER_PHYS_DEAD))
		{
			CWO_Character_ClientData *pTargetCD = CWObject_Character::GetClientData(pTarget);
			CVec3Dfp32 CharUp;
			if(pTargetCD)
			{
				CVec3Dfp32 TargetHeadPos = pTargetCD->m_PostAnimSystem.GetCameraLookPos();
				CharUp = (TargetHeadPos - TarPos).Normalize();
			}
			else
				CharUp = CVec3Dfp32(0.0f, 0.0f, 1.0f); // failsafe

			ColLineStart = m_CameraLookMat.GetRow(3);
			CVec3Dfp32 ModDirection = TarPos - ColLineStart;
			ModDirection = ModDirection.Normalize();

			// todo! check modirection is NAN
			CVec3Dfp32 TargetDirRight;	
			ModDirection.CrossProd(CharUp, TargetDirRight);

			TargetDirRight = TargetDirRight.Normalize();
			CVec3Dfp32 CamAtProj = TargetDirRight * (TargetDirRight * m_CameraLookMat.GetRow(0));
			ModDirection = (-CamAtProj + m_CameraLookMat.GetRow(0)).Normalize();
			ColLineEnd = ColLineStart + (ModDirection * 2048.0f);

			bDoLineCollisionCheck = true;
			bIsHumanTarget = true;
		}
	}

	if(bDoLineCollisionCheck)
	{		 
		if(_pWPhysState->Phys_IntersectLine(ColLineStart, ColLineEnd, OBJECT_FLAGS_PHYSOBJECT,  // OBJECT_FLAGS_PROJECTILE collides w characters
			OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_PHYSMODEL, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS,
			&CollisInfo, m_iPlayerObject,true) && CollisInfo.m_bIsValid)
		{
			// not too close
			CVec3Dfp32 AimDir = (CollisInfo.m_Pos - ColLineStart);

			fp32 MaxSquareDist = 2500.0f;
			fp32 MaxDist = 50.0f;
			if(m_WeaponHoldType == WEAPONHOLD_TYPE_TWOHANDED)
			{
				MaxSquareDist = 5600.0f;
				MaxDist = 80.0f;
			}

			if(AimDir.LengthSqr() > MaxSquareDist)
			{
				m_AbsAimTargetPos = CollisInfo.m_Pos;
				m_IKFlags &= ~IKFLAGS_FACEVERYCLOSETOWALL;
			}
			else
			{
				m_AbsAimTargetPos = ColLineStart + (AimDir.Normalize() * MaxDist);
				m_IKFlags |= IKFLAGS_FACEVERYCLOSETOWALL;
			}

			if(bIsHumanTarget)
				m_AbsAimTargetPos.k[2] = Max(m_AbsAimTargetPos.k[2], TarPos.k[2]); // no lower than feet
		}
		else
			m_AbsAimTargetPos = ColLineEnd;
	}
#endif
}

void CPostAnimSystem::SetupNodesForAim(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CWorld_PhysState* _pWPhysState,  CWO_Character_ClientData *_pCD)
{
#ifndef OGIER
	
	 if(m_IKFlags & IKFLAGS_DISABLE_RIGHTAIM)
	 {
		 m_RightAimInAndOutBlendVal -= m_fDeltaTime * HANDIKBLENDINSPEED;
		 m_RightAimInAndOutBlendVal = Max(m_RightAimInAndOutBlendVal, 0.0f);
	 }
	 else
	 {
		 m_RightAimInAndOutBlendVal += m_fDeltaTime * HANDIKBLENDINSPEED;
		 m_RightAimInAndOutBlendVal = Min(m_RightAimInAndOutBlendVal, 1.0f);
	 }

	 if(m_IKFlags & IKFLAGS_DISABLE_LEFTAIM)
	 {
		 m_LeftAimInAndOutBlendVal -= m_fDeltaTime * HANDIKBLENDINSPEED;
		 m_LeftAimInAndOutBlendVal = Max(m_LeftAimInAndOutBlendVal, 0.0f);
	 }
	 else
	 {
		 m_LeftAimInAndOutBlendVal += m_fDeltaTime * HANDIKBLENDINSPEED;
		 m_LeftAimInAndOutBlendVal = Min(m_LeftAimInAndOutBlendVal, 1.0f);
	 }

	 if(_pCD->m_iPlayer == -1 && m_RightAimInAndOutBlendVal == 0.0f && m_LeftAimInAndOutBlendVal == 0.0f)
		 return;

	//---------------------------------------------------------------------------------------------------------------
	// overwrite with autoaimpoint

	fp32 MaxAngle;
	int8 iNrOfIterations;
	fp32 Speeder;

	switch(m_WeaponHoldType)
	{
	case WEAPONHOLD_TYPE_DUALWIELD:
		iNrOfIterations = 2;
		MaxAngle = 0.07f;
		Speeder = 8.0f;
		break;

	case WEAPONHOLD_TYPE_ONLY_1_1HGUN:
		iNrOfIterations = 1;
		MaxAngle = 0.07f;
		Speeder = 8.0f;
	    break;

	case WEAPONHOLD_TYPE_TWOHANDED:
		iNrOfIterations = 1;
		MaxAngle = 0.05f;
		Speeder = 6.0f;
		break;

	default:
		return; // error
	}


	CVec3Dfp32 LastCurIntTargPos = m_CurIntrpAimTargetPos;
	if(_pCD->m_AimTarget && _pCD->m_AimTarget != -1 && !(m_IKFlags & IKFLAGS_INWEAPONZOOMMODE))
	{
		m_TargetBlendOut = 0.0f;
		CVec3Dfp32 LaggDir = (m_AbsAimTargetPos - m_CurIntrpAimTargetPos);
		fp32 CathcUpValue = m_fDeltaTime * Speeder;
		CathcUpValue = Min(CathcUpValue, 1.0f);
		m_CurIntrpAimTargetPos += LaggDir *  CathcUpValue;
	}
	else
	{
		m_TargetBlendOut += m_fDeltaTime * HANDIKBLENDINSPEED;
		if(m_TargetBlendOut > 1.0f)
		{
			m_TargetBlendOut = 1.0f;

			// intepolate depth
			CVec3Dfp32 AimToABS = m_AbsAimTargetPos - m_CameraLookMat.GetRow(3);
			CVec3Dfp32 CamToCureDir = m_CurIntrpAimTargetPos - m_CameraLookMat.GetRow(3);

			fp32 CamToAbsLen = AimToABS.Length();
			fp32 CamToCureDirLen = CamToCureDir.Length();
			fp32 LenDiff = CamToAbsLen - CamToCureDirLen;
			
			fp32 NewCurLen = CamToCureDirLen + (LenDiff * m_fDeltaTime * Speeder);

			m_CurIntrpAimTargetPos = m_CameraLookMat.GetRow(3) + ((AimToABS).Normalize() * NewCurLen);

			//m_CurIntrpAimTargetPos = m_AbsAimTargetPos;
		}
		else
		{
			CVec3Dfp32 LaggDir = (m_AbsAimTargetPos - m_CurIntrpAimTargetPos);
			m_CurIntrpAimTargetPos += LaggDir *  Sinc(m_TargetBlendOut);
		}
	}
	
	if(_pWPhysState->IsServer())
		m_CurIntrpAimTargetPos = m_AbsAimTargetPos;

#ifdef USEJAKOBSDEBUGINFO
	if(_pWPhysState->IsServer())
	{
		CMat4Dfp32 DebugMat;
		DebugMat.Unit();
		DebugMat.GetRow(0) *= 2.5f;
		DebugMat.GetRow(1) *= 2.5f;
		DebugMat.GetRow(2) *= 2.5f;
		DebugMat.GetRow(3) = m_AbsAimTargetPos;
		_pWPhysState->Debug_RenderMatrix(DebugMat, true);
	}
#endif

	// modify line collision
	//if(m_WeaponPoseRight != WEAPON_POSE_GUNSLINGER_DUEL && m_WeaponPoseLeft != WEAPON_POSE_GUNSLINGER_DUEL)
	{
		CVec3Dfp32 LastCamToCureDir = (LastCurIntTargPos - m_CameraLookMat.GetRow(3)).Normalize();
		CVec3Dfp32 CamToCureDir = (m_CurIntrpAimTargetPos - m_CameraLookMat.GetRow(3)).Normalize();

		CVec3Dfp32 Dir1 =  (CamToCureDir - LastCamToCureDir);
		if(Dir1.LengthSqr() > 0.0001f) //(0.01 * 0.01)
			m_MovementLineColMod = Dir1.Normalize();
	}

	//---------------------------------------------------------------------------------------------------------------
	// apply rotations

	CVec3Dfp32 WeaponToTarget;
	CVec3Dfp32 RotAxis;
	CVec3Dfp32 StoredHandPos;
	CVec3Dfp32 ActualGunPos;
	CMat4Dfp32 WeaponMat;
	int8 RotTrack;
	int8 RotTrackAtt;
	fp32 *pAimAngle;
	fp32 AimInOutBlendVal;
	fp32 BlendVal2;

	for (int i= 0; i < iNrOfIterations; i++)
	{
		if(i == 0 && m_RightAimInAndOutBlendVal != 0.0f)
		{
			AimInOutBlendVal = Sinc(m_RightAimInAndOutBlendVal);
			WeaponMat = m_MatWeapon;
			RotTrack = PLAYER_ROTTRACK_RHAND;
			RotTrackAtt = PLAYER_ROTTRACK_RHANDATTACH;	
			pAimAngle = &m_fAimAngleWeaponRight;
			BlendVal2 = m_RightHandBlendVal;
		}
		else if(i == 1 && m_LeftAimInAndOutBlendVal != 0.0f)
		{
			AimInOutBlendVal = Sinc(m_LeftAimInAndOutBlendVal);
			WeaponMat = m_MatWeapon2;
			RotTrack = PLAYER_ROTTRACK_LHAND;
			RotTrackAtt = PLAYER_ROTTRACK_LHANDATTACH;	
			pAimAngle = &m_fAimAngleWeaponLeft;

			if(m_WeaponHoldType == WEAPONHOLD_TYPE_ONLY_1_1HGUN)
				BlendVal2 = 1.0f;
			else
				BlendVal2 = m_LeftHandBlendVal;
		}
		else
			continue;

		if(BlendVal2 == 0.0f)
			continue;

		StoredHandPos = _pSkelInstance->m_pBoneLocalPos[RotTrack].GetRow(3);
		ActualGunPos = _pSkel->m_lNodes[RotTrackAtt].m_LocalCenter * _pSkelInstance->m_pBoneTransform[RotTrack];
		ActualGunPos += WeaponMat.GetRow(2) * 3.0f;

		WeaponToTarget = (m_CurIntrpAimTargetPos - ActualGunPos);
		if(WeaponToTarget.LengthSqr() > 100.0f)
		{
			//_pWPhysState->Debug_RenderWire(ActualGunPos, m_CurIntrpAimTargetPos , 0xfffff666);

			WeaponToTarget = WeaponToTarget.Normalize();
			WeaponMat.GetRow(0).CrossProd(WeaponToTarget, RotAxis);
			RotAxis = RotAxis.Normalize();

			*pAimAngle = M_ACos(WeaponToTarget * WeaponMat.GetRow(0));
			*pAimAngle = *pAimAngle / (_PI2); // convert to 0->1 angle
			*pAimAngle = Min(*pAimAngle, MaxAngle);

			// use left since that's the one that's often toggles on/off
			*pAimAngle *= BlendVal2;
			
			//*pAimAngle = Min(Max(*pAimAngle, -MaxAngle), MaxAngle);
			RotateBoneAroundAxis(*pAimAngle * AimInOutBlendVal, &RotAxis, RotTrack, _pSkel, _pSkelInstance,_pWPhysState, true);

			int iParentNode = _pSkel->m_lNodes[RotTrack].m_iNodeParent;
			_pSkelInstance->m_pBoneLocalPos[RotTrack].GetRow(3) = StoredHandPos;
			
			_pSkel->EvalNode_IK_Special(RotTrack, &_pSkelInstance->m_pBoneTransform[iParentNode], _pSkelInstance, 0);

			// this is needed actually for the left hand indirectly
			if(m_WeaponHoldType == WEAPONHOLD_TYPE_TWOHANDED)
				_pSkel->EvalNode_IK_Special(PLAYER_ROTTRACK_RHANDATTACH, &_pSkelInstance->m_pBoneTransform[_pSkel->m_lNodes[PLAYER_ROTTRACK_RHANDATTACH].m_iNodeParent], _pSkelInstance,0);
		}
		else
			m_CurIntrpAimTargetPos = m_AbsAimTargetPos;
	}
#endif
}

void CPostAnimSystem::SetHeadLookDir(const CVec3Dfp32 *_pAILookDir)
{
	if (_pAILookDir)
	{	
		CVec3Dfp32 Dir = *_pAILookDir;
		if(VALID_POS(Dir))
		{
			m_IKFlags |= IKFLAGS_FORCE_AI_LOOKDIR;
			m_AILookDir =  *_pAILookDir;
			if(m_fAimAngleHead == 0.0f)
				m_fAimAngleHead = 0.00001f;
		}
		else
			m_IKFlags &= ~IKFLAGS_FORCE_AI_LOOKDIR;
	}
	else
	{
		m_IKFlags &= ~IKFLAGS_FORCE_AI_LOOKDIR;
	}	
}

void CPostAnimSystem::UpdateHeadLookDir(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CWorld_PhysState* _pWPhysState)
{
	if((m_IKFlags & IKFLAGS_FORCE_AI_LOOKDIR) || m_fAimAngleHead != 0.0f)
	{
		//---------------------------------------------------------------------------------
		// Head stuff
		int iParentNode;
		iParentNode = _pSkel->m_lNodes[PLAYER_ROTTRACK_NECK].m_iNodeParent;
		CVec3Dfp32 NeckPos = _pSkel->m_lNodes[PLAYER_ROTTRACK_NECK].m_LocalCenter * _pSkelInstance->m_pBoneTransform[iParentNode];

		CVec3Dfp32 NeckToTarget = m_AILookDir;

		//fp32 LengthToTarget = NeckToTarget.Length();
		CVec3Dfp32 LookVector = m_CameraLookMat.GetRow(0); 

		CVec3Dfp32 RotAxis;
		if (LookVector * NeckToTarget >= 0.999f)
		{	// LookVector and NeckToTarget are almost identical.
			// No need to turn head and the CrossProd would produce zero vector which doesn't like being normalized
			m_fAimAngleHead = 0.0f;
			return;
		}

		LookVector.CrossProd(NeckToTarget, RotAxis);
		RotAxis = RotAxis.Normalize();

		fp32 fAngle = M_ACos(NeckToTarget * LookVector);
		fAngle = fAngle / (_PI2); // convert to 0->1 angle
		if(fAngle > 0.2f)
		{
			m_IKFlags &= ~IKFLAGS_FORCE_AI_LOOKDIR;
		}

		fAngle = Min(fAngle, 0.17f);

		if(!(m_IKFlags & IKFLAGS_FORCE_AI_LOOKDIR))
		{
			m_fAimAngleHead -= m_fDeltaTime * AIMROTATIONSPEEDHEAD;
			m_fAimAngleHead = Max(m_fAimAngleHead, 0.0f);
		}
		else
		{
			m_fAimAngleHead += m_fDeltaTime * AIMROTATIONSPEEDHEAD;
			m_fAimAngleHead = Min(m_fAimAngleHead, fAngle);
		}

		fp32 ActualtTurnAngle;
		ActualtTurnAngle = 0.17f * Sinc(m_fAimAngleHead / 0.17f);
		RotateBoneAroundAxis(ActualtTurnAngle, &RotAxis, PLAYER_ROTTRACK_NECK, _pSkel, _pSkelInstance, _pWPhysState, true);
	}
}
//--------------------------------------------------------------------------------------------------
// called from OnClientRefresh()
void CPostAnimSystem::GatherFeetCollisionData(uint8 _FeetType, CWorld_PhysState* _pWPhysState, fp32 _SkelScale)
{
#ifndef OGIER
	// reset values
	m_IKFlags &= ~IKFLAGS_LEFTFRONTFOOTHASCOLLISION;
	m_IKFlags &= ~IKFLAGS_RIGHTFRONTFOOTHASCOLLISION;
	m_IKFlags &= ~IKFLAGS_LEFTBACKFOOTHASCOLLISION;
	m_IKFlags &= ~IKFLAGS_RIGHTBACKFOOTHASCOLLISION;	

	//------------------------------------------------------------------------
	// the server does not run feet IK, but needs to know if the char is in a 
	// stair going down or up, to be able to set the right animation

	if(_pWPhysState->IsServer())
	{		
		if(!(m_IKFlags2 & IKFLAGS2_SERVERMODEL0SET))
			return;

		CCollisionInfo	CollisInfo;
		CVec3Dfp32		ColLineStart;
		CVec3Dfp32		ColLineEnd;

		// just check if this is a stair
		ColLineStart = m_ModelNode0.GetRow(3) + (m_ModelNode0.GetRow(2) * 15.0f);
		ColLineEnd   = ColLineStart - (m_ModelNode0.GetRow(2) * 30.0f);
		CollisInfo.Clear();
		CollisInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_POSITION | CXR_COLLISIONRETURNVALUE_SURFACE);
		if(_pWPhysState->Phys_IntersectLine(ColLineStart, ColLineEnd, OBJECT_FLAGS_WORLD, OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_CREEPING_DARK, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &CollisInfo))
		{

			CWObject_CoreData *pColObj = CollisInfo.m_iObject ? _pWPhysState->Object_GetCD(CollisInfo.m_iObject) : NULL;
			if(pColObj)
			{
				if(pColObj->IsClass(class_CreepingDarkEntity))
				{
					CVec3Dfp32 CharDir = m_ModelNode0.GetRow(0);
					fp32	fAngle = M_ACos(CharDir * CollisInfo.m_Plane.n);
					fAngle = fAngle / (_PI2); // convert to 0->1 angle
					if(fAngle > 0.27f)
						m_StairMoveType = FOOT_MOVEMENT_STAIR_UP;
					else if(fAngle < 0.23f)
						m_StairMoveType = FOOT_MOVEMENT_STAIR_DOWN;
					else
					{
						m_StairMoveType = FOOT_MOVEMENT_ANIMDRIVEN;
					}
				}
				else
				{
					m_StairMoveType = FOOT_MOVEMENT_ANIMDRIVEN;
				}
			}
		}
		return;
	}

	//--------------------------------------------------------------------------

	if(!(m_IKFlags & IKFLAGS_FEETSETUPOK))
	{
		m_StairMoveType = 0;
		return;
	}

	// values for deciding what kind of ground is ahead
	uint8 iNrOfIterations       = 0;
	fp32	  ColMaxDiff	        = 0.0f;
	fp32	  ForwardDistanceForCol	= 0.0f;
	fp32	  HoverValue = HOVERVALUE * _SkelScale;
	switch(_FeetType) 
	{
	case FEET_TYPE_DARKLING_QUADRO:

		if(m_DisableDarklingIK)
			return;

		iNrOfIterations = 4;						// check four feet
		if(m_IKFlags & IKFLAGS_STANDINGSTILL)
			m_StairMoveType = FOOT_MOVEMENT_STAIR_DOWN;
		else
			m_StairMoveType	= FOOT_MOVEMENT_UNDEFINED;	// not yet set
		ColMaxDiff		= 10.0f;					// A bit to play with when bouncing
		ForwardDistanceForCol = 30.0f;				

		//m_StairMoveType	= FOOT_MOVEMENT_STAIR_DOWN; // always this for darklings
		break;

	case FEET_TYPE_HUMAN_BIPED:

		if(m_DisableHumanFeetIK)
			return;

		iNrOfIterations = 2; 						// check two feet
		if(m_IKFlags & IKFLAGS_STANDINGSTILL)
			m_StairMoveType = FOOT_MOVEMENT_STAIR_DOWN;
		else
			m_StairMoveType	= FOOT_MOVEMENT_UNDEFINED;	// not yet set
		ColMaxDiff		= 10.0f;					// A bit to play with when bouncing
 		ForwardDistanceForCol = 30.0f;				// the line that finds ground
		break;

	default:
		return;
	}

	bool		GroundCheckCol			= false;
	bool		bFootCollisionFound		= false;
	bool		bShouldCollide			= false;

	CVec3Dfp32	StoredColPos;
	CVec3Dfp32	StoredFootPos;
	CMat4Dfp32	AnimFootMatrix;
	CVec3Dfp32	HealToToe;

	// check collisions for all feet
	uint8 IKMode = 0;
	for(uint8 i = 0; i < iNrOfIterations; i++)
	{
		//-----------------------------------------------------------
		// Setup values that are foot specific
		switch(_FeetType) 
		{
		case FEET_TYPE_DARKLING_QUADRO:
			switch(i)
			{
			case 0:
				IKMode = CIKSystem::IK_MODE_DARKLING_LEFT_BACK_FOOT;
				AnimFootMatrix = m_MatLeftFootAnim;
				HealToToe = m_ModelNode0.GetRow(0) * 7.0f;
				break;

			case 1:
				IKMode = CIKSystem::IK_MODE_DARKLING_RIGHT_BACK_FOOT;
				AnimFootMatrix = m_MatRightFootAnim;	
				if(m_IKFlags & IKFLAGS_ISSLOWERTHANRUNNING)
					HealToToe = 0;
				else
					HealToToe = m_ModelNode0.GetRow(0) * 7.0f;

				break;

			case 2:
				IKMode = CIKSystem::IK_MODE_DARKLING_LEFT_FRONT_FOOT;
				AnimFootMatrix = m_MatFrontLeftFootAnim;
				HealToToe = m_ModelNode0.GetRow(0) * 7.0f;
				break;

			case 3:
				IKMode = CIKSystem::IK_MODE_DARKLING_RIGHT_FRONT_FOOT;
				AnimFootMatrix = m_MatFrontRightFootAnim;				
				HealToToe = m_ModelNode0.GetRow(0) * 7.0f;
				break;

			default:
				break;
			}
			break;

		case FEET_TYPE_HUMAN_BIPED:
			switch(i)
			{
			case 0:
				IKMode = CIKSystem::IK_MODE_LEFT_FOOT;
				AnimFootMatrix = m_MatLeftFootAnim;				
				HealToToe = AnimFootMatrix.GetRow(0) * 7.0f + AnimFootMatrix.GetRow(1) * 2.0f;
				if(m_IKFlags & IKFLAGS_LSHOULDCOLLIDE)
					bShouldCollide = true;
				else
					bShouldCollide = false;
				break;

			case 1:
				IKMode = CIKSystem::IK_MODE_RIGHT_FOOT;
				AnimFootMatrix = m_MatRightFootAnim;
				HealToToe = AnimFootMatrix.GetRow(0) * 7.0f - AnimFootMatrix.GetRow(1) * 2.0f;
				
				if(m_IKFlags & IKFLAGS_RSHOULDCOLLIDE)
					bShouldCollide = true;
				else
					bShouldCollide = false;

				break;

			default:
				break;
			}
			break;

		default:
			return;
		}


		//-----------------------------------------------------------
		// Proceed with collision detection
		CCollisionInfo	CollisInfo;
		CVec3Dfp32		ColLineStart;
		CVec3Dfp32		ColLineEnd;

		// decide what kind of floor we're dealing with
		if(m_StairMoveType == FOOT_MOVEMENT_UNDEFINED)
		{
			ColLineStart = m_ModelNode0.GetRow(3) + (m_ModelNode0.GetRow(0) * ForwardDistanceForCol) + (m_ModelNode0.GetRow(2)* 25.0f * _SkelScale);
			ColLineEnd   = ColLineStart - (m_ModelNode0.GetRow(2) * 30.0f * _SkelScale); // we DO want about 5 units of penetration
			CollisInfo.Clear();
			CollisInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_POSITION);
			if(_pWPhysState->Phys_IntersectLine(ColLineStart, ColLineEnd, OBJECT_FLAGS_WORLD, OBJECT_FLAGS_PHYSMODEL, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &CollisInfo))
			{
				fp32 ColDff = ((ColLineStart - ColLineEnd).Length() - (CollisInfo.m_Pos - ColLineStart).Length());

				if(ColDff > ColMaxDiff)		
				{
					m_StairMoveType = FOOT_MOVEMENT_STAIR_UP; // FOOT_MOVEMENT_STAIR_DOWN
				}
				else
				{
					m_StairMoveType = FOOT_MOVEMENT_ANIMDRIVEN;
				}
			}
			else
			{
				m_StairMoveType = FOOT_MOVEMENT_STAIR_DOWN;
			}
		}


	// always IK the front feet
	int tempMoveType = m_StairMoveType;
	if(IKMode == CIKSystem::IK_MODE_DARKLING_LEFT_FRONT_FOOT || IKMode == CIKSystem::IK_MODE_DARKLING_RIGHT_FRONT_FOOT)
		tempMoveType = FOOT_MOVEMENT_STAIR_DOWN;
		//---------------------------------------------------------
		// Use the movement-type to decide what kind of collisions we want (heal or toe or both, length etc.)
	
	switch(tempMoveType)
		{
		case FOOT_MOVEMENT_STAIR_UP: //upwards a stair, check from toe (short int line)

			if(m_IKFlags & IKFLAGS_ISSLOWERTHANRUNNING) // not if running...
			{
				// trace from toe			
				ColLineStart = AnimFootMatrix.GetRow(3) + HealToToe - (m_ModelNode0.GetRow(2) * -20.0f);
				ColLineEnd = (ColLineStart + (m_ModelNode0.GetRow(2) * -60.0f));

				CollisInfo.Clear();
				CollisInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_POSITION);
				if(_pWPhysState->Phys_IntersectLine(ColLineStart, ColLineEnd, OBJECT_FLAGS_WORLD,
					OBJECT_FLAGS_PHYSMODEL,	XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &CollisInfo))
				{
					bFootCollisionFound = true;
					StoredColPos = CollisInfo.m_Pos;
				}
				else
					bFootCollisionFound = false;

				HealToToe -=  m_ModelNode0.GetRow(2);
			}
			
			break;

		case FOOT_MOVEMENT_STAIR_DOWN:

			if((m_IKFlags & IKFLAGS_ISSLOWERTHANRUNNING )|| _FeetType == FEET_TYPE_DARKLING_QUADRO) // always for darklings, only in idle mode for Biped
			{
				bool ToeIntFound	= false;
				fp32 ToeColLen		= 0.0f;
				CVec3Dfp32 ToeIntPt	= 0;

				bool HealIntFound	= false;
				fp32 HealColLen		= 0.0f;
				CVec3Dfp32 HealIntPt = 0;

				//------------------------------------------------
				// check for Toe Collision

				ColLineStart = AnimFootMatrix.GetRow(3) + HealToToe - (m_ModelNode0.GetRow(2) * -20.0f);
				ColLineEnd = (ColLineStart + (m_ModelNode0.GetRow(2) * -60.0f));

				CollisInfo.Clear();
				CollisInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_POSITION);
				if(_pWPhysState->Phys_IntersectLine(ColLineStart, ColLineEnd, OBJECT_FLAGS_WORLD,
					OBJECT_FLAGS_PHYSMODEL, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &CollisInfo))
				{
					ToeIntFound = true;
					ToeIntPt	= CollisInfo.m_Pos;
					ToeColLen	= (ColLineStart - CollisInfo.m_Pos).LengthSqr();
				}

				//------------------------------------------------
				// check for Heal Collision
				// darkling rightbackfoot is a bit special in idle animation. 
				// only one line collision for that one

				if(IKMode != CIKSystem::IK_MODE_DARKLING_RIGHT_BACK_FOOT)
				{
					ColLineStart = AnimFootMatrix.GetRow(3) - (m_ModelNode0.GetRow(2) * -20.0f);
					ColLineEnd = (ColLineStart + (m_ModelNode0.GetRow(2) * -60.0f));

					CollisInfo.Clear();
					CollisInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_POSITION);
					if(_pWPhysState->Phys_IntersectLine(ColLineStart, ColLineEnd, OBJECT_FLAGS_WORLD,
						OBJECT_FLAGS_PHYSMODEL,	XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &CollisInfo))
					{
						HealIntFound = true;
						HealIntPt = CollisInfo.m_Pos;
						HealColLen = (ColLineStart - CollisInfo.m_Pos).LengthSqr();
					}
				}

				//------------------------------------------------
				// Wich of the points should be used for foot position

				if(HealIntFound && ToeIntFound)
				{
					if(ToeColLen < HealColLen) 
						StoredColPos = (ToeIntPt - HealToToe); // use toe as height adjuster
					else
						StoredColPos = HealIntPt;				// use heal as height adjuster

					//-----------------------------------------------------------------------------------------------------------
					// rotate hands so that fingers point towards ground
					if(IKMode == CIKSystem::IK_MODE_DARKLING_LEFT_FRONT_FOOT || IKMode == CIKSystem::IK_MODE_DARKLING_RIGHT_FRONT_FOOT)
					{
						CVec3Dfp32 WristToFingerInt = (ToeIntPt - AnimFootMatrix.GetRow(3)).Normalize();
						CVec3Dfp32 WristToFinger = ((AnimFootMatrix.GetRow(3) + HealToToe) - AnimFootMatrix.GetRow(3)).Normalize();

						fp32	fAngle = M_ACos(WristToFinger * WristToFingerInt);
						fAngle = fAngle / (_PI2); // convert to 0->1 angle
			
						fp32 HardCodedHandRotationSpeed = 0.25f;
						if(IKMode == CIKSystem::IK_MODE_DARKLING_RIGHT_FRONT_FOOT)
						{

							if(m_RightFrontFootRotation < fAngle)
							{
								m_RightFrontFootRotation += m_fDeltaTime * HardCodedHandRotationSpeed;
								m_RightFrontFootRotation = Min(m_RightFrontFootRotation, fAngle);
							}
							else
							{
								m_RightFrontFootRotation -= m_fDeltaTime * HardCodedHandRotationSpeed;
								m_RightFrontFootRotation = Max(m_RightFrontFootRotation, fAngle);
							}
						}
						else if(IKMode == CIKSystem::IK_MODE_DARKLING_LEFT_FRONT_FOOT)
						{
							if(m_LeftFrontFootRotation < fAngle)
							{
								m_LeftFrontFootRotation += m_fDeltaTime * HardCodedHandRotationSpeed;
								m_LeftFrontFootRotation = Min(m_LeftFrontFootRotation, fAngle);
							}
							else
							{
								m_LeftFrontFootRotation -= m_fDeltaTime * HardCodedHandRotationSpeed;
								m_LeftFrontFootRotation = Max(m_LeftFrontFootRotation, fAngle);
							}
						}
					}
					//-----------------------------------------------------------------------------------------------------------
				}
				else if(HealIntFound)
				{
					StoredColPos = HealIntPt;
				}
				else if(ToeIntFound)
				{
					StoredColPos = (ToeIntPt - HealToToe);
				}

				bFootCollisionFound = (HealIntFound || ToeIntFound);
			}

			else if(!(m_IKFlags & IKFLAGS_ISSLOWERTHANRUNNING) && _FeetType == FEET_TYPE_HUMAN_BIPED)
			{
				// if should collide, reach for footstep
				if(bShouldCollide)
				{
					// trace from heal
					ColLineStart = AnimFootMatrix.GetRow(3) + (m_ModelNode0.GetRow(2) * 20.0f);
					ColLineEnd = (ColLineStart + (m_ModelNode0.GetRow(2) * -120.0f));

					CollisInfo.Clear();
					CollisInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_POSITION);
					if(_pWPhysState->Phys_IntersectLine(ColLineStart, ColLineEnd, OBJECT_FLAGS_WORLD,
						OBJECT_FLAGS_PHYSMODEL,	XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &CollisInfo))
					{
						bFootCollisionFound = true;
						StoredColPos = CollisInfo.m_Pos;
					}
					else
						bFootCollisionFound = false;
				}

			}

			HealToToe = 0;
			break;

		case FOOT_MOVEMENT_ANIMDRIVEN: // animdriven means no target at all
			HealToToe = 0;
			break;
		}

		if(bFootCollisionFound)
		{
			// Set values and adjust with hack to fix strange idle position of right back foot
			switch(IKMode)
			{
			case CIKSystem::IK_MODE_DARKLING_LEFT_BACK_FOOT:
				m_LeftBackFootTarget = StoredColPos + (m_ModelNode0.GetRow(2) * (HoverValue - 0.5f));
				m_IKFlags |= IKFLAGS_LEFTBACKFOOTHASCOLLISION;
				break;

			case CIKSystem::IK_MODE_DARKLING_RIGHT_BACK_FOOT:
				if(m_IKFlags & IKFLAGS_ISSLOWERTHANRUNNING)
					m_RightBackFootTarget = StoredColPos + (m_ModelNode0.GetRow(2) * (HoverValue + 3.0f));
				else
					m_RightBackFootTarget = StoredColPos + (m_ModelNode0.GetRow(2) * (HoverValue + 0.5f));

				m_IKFlags |= IKFLAGS_RIGHTBACKFOOTHASCOLLISION;
				break;

			case CIKSystem::IK_MODE_DARKLING_LEFT_FRONT_FOOT:
				m_LeftFrontFootTarget = StoredColPos + (m_ModelNode0.GetRow(2) * HoverValue);
				m_IKFlags |= IKFLAGS_LEFTFRONTFOOTHASCOLLISION;
				break;

			case CIKSystem::IK_MODE_DARKLING_RIGHT_FRONT_FOOT:
				m_RightFrontFootTarget = StoredColPos + (m_ModelNode0.GetRow(2) * HoverValue);
				m_IKFlags |= IKFLAGS_RIGHTFRONTFOOTHASCOLLISION;
				break;

			case CIKSystem::IK_MODE_LEFT_FOOT:
				m_LeftBackFootTarget = StoredColPos + (m_ModelNode0.GetRow(2) * (HoverValue - HealToToe.k[2]));
				m_IKFlags |= IKFLAGS_LEFTBACKFOOTHASCOLLISION;
				break;

			case CIKSystem::IK_MODE_RIGHT_FOOT:
				m_RightBackFootTarget = StoredColPos + (m_ModelNode0.GetRow(2) * (HoverValue - HealToToe.k[2]));
				m_IKFlags |= IKFLAGS_RIGHTBACKFOOTHASCOLLISION;
				break;

			default:
				break;
			}
		}
		bFootCollisionFound =false;
	} // end of for loop
#endif
}

// -------------------------------------------------------------------------------------------------
// helpfunction for GatherWeaponWallCollisionData
//
void CPostAnimSystem::SetupCollisionValues(CVec3Dfp32 &_RetAproxWPos, uint16 _WeaponPose, CVec3Dfp32 &_ColLineStart, CVec3Dfp32 &_ColLineEnd, SWeaponPose *_pPoses, 
										   fp32 _SearchLenMod, fp32 _HeightMod, fp32 _WidthMod, fp32 _BackMod, fp32 _CharRotationModMult, const CMat4Dfp32 *_WeaponMat, int8 _LRDirMod)
{
	fp32 SearchLen = _SearchLenMod + _pPoses[_WeaponPose].m_SearchLenMod;
	
	if((m_IKFlags & IKFLAGS_LONGDUALWIELDWEAPON_RIGHT) || (m_IKFlags & IKFLAGS_LONGDUALWIELDWEAPON_LEFT))
		SearchLen += 5.0f;

	_RetAproxWPos = _WeaponMat->GetRow(3) + (m_CameraLookMat.GetRow(1) * _WidthMod * _LRDirMod) + (m_CameraLookMat.GetRow(2) * _HeightMod);
	_ColLineStart = _RetAproxWPos - (m_CameraLookMat.GetRow(0) * _BackMod);

	_ColLineStart += (m_CameraLookMat.GetRow(0) * _pPoses[_WeaponPose].m_Position.k[0] + 
		m_CameraLookMat.GetRow(1) * _pPoses[_WeaponPose].m_Position.k[1] * _LRDirMod + 
		m_CameraLookMat.GetRow(2) * _pPoses[_WeaponPose].m_Position.k[2]);

	// slight mod for line collision tweaking
	_ColLineStart += (m_CameraLookMat.GetRow(0) * _pPoses[_WeaponPose].m_LineCollisionMod.k[0] + 
		m_CameraLookMat.GetRow(1) * _pPoses[_WeaponPose].m_LineCollisionMod.k[1] * _LRDirMod + 
		m_CameraLookMat.GetRow(2) * _pPoses[_WeaponPose].m_LineCollisionMod.k[2]);

	_ColLineEnd = (_ColLineStart + (m_CameraLookMat.GetRow(0) * SearchLen));
	
	//---------------------------------------------------------------------------
	// floating linecollision depending on player movement
	
	// if we're stanging with each hand on the sides of something like a pole or vertical beam
	// the lines needs to be drawn together
	if((m_IKFlags & IKFLAGS_FACEVERYCLOSETOWALL) && _WeaponPose < WEAPON_POSETEMPLATE_NORMAL_LAST+1)
	{
		_ColLineStart += (m_CameraLookMat.GetRow(1) * _CharRotationModMult * _LRDirMod);
		_ColLineEnd += (m_CameraLookMat.GetRow(1) * _CharRotationModMult * _LRDirMod);
	}
	else
	{
		_ColLineStart += (m_MovementLineColMod * _CharRotationModMult);
		_ColLineEnd += (m_MovementLineColMod * _CharRotationModMult);
	}
}

// -------------------------------------------------------------------------------------------------
// helpfunction for GatherWeaponWallCollisionData
//
int CPostAnimSystem::SetNormalPoseRange(uint16 &_WeaponPose)
{
	int UseRandomNormalPose = GetRandomNormalPose();
	if(_WeaponPose < UseRandomNormalPose)
		_WeaponPose = UseRandomNormalPose;
	else if(_WeaponPose > UseRandomNormalPose && _WeaponPose < WEAPON_POSETEMPLATE_NORMAL_LAST+1)
		_WeaponPose = WEAPON_POSETEMPLATE_NORMAL_LAST+1;

	return UseRandomNormalPose;
}

void CPostAnimSystem::StoreCollisionInfo(const CCollisionInfo *_pCollisInfo, const CVec3Dfp32 *_pColLineStart, const CVec3Dfp32 *_pWeaponPos, 
										 CWorld_PhysState* _pWPhysState, SDW_WeaponCollisionInfo *_pWeaponColInfo, bool _bIsMainWeapon, fp32 &_GunBlendVal)
{
	// check if it's a door, player phys (count a character as playerphys) or glass (count as playerphys)
	bool IsPlayerPhys = false;
	CWObject_CoreData *pColObj = _pCollisInfo->m_iObject ? _pWPhysState->Object_GetCD(_pCollisInfo->m_iObject) : NULL;
	if(pColObj)
	{
		if(pColObj->IsClass(class_SwingDoor))
		{
			m_IKFlags |= IKFLAGS_OPENDOOR;			
			if(_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_SWINGDOOR_GETOPENDIR, 0, 0, m_iPlayerObject), _pCollisInfo->m_iObject) == 1)
				m_IKFlags |= IKFLAGS_OPENDOOR_RIGHTHAND;
			else
				m_IKFlags &= ~IKFLAGS_OPENDOOR_RIGHTHAND;

			if(_bIsMainWeapon)
				m_IKFlags |= IKFLAGS_DISABLE_RIGHTAIM;
			else
				m_IKFlags |= IKFLAGS_DISABLE_LEFTAIM;
		}
		else if(pColObj && (pColObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER))
		{
			IsPlayerPhys = true;
		}
		else if(_pCollisInfo->m_pSurface)
		{
			const CXW_SurfaceKeyFrame *pBaseFrame = _pCollisInfo->m_pSurface->GetBaseFrame();
			if(pBaseFrame)
				IsPlayerPhys = (pBaseFrame->m_Medium.m_MediumFlags & (XW_MEDIUM_GLASS | XW_MEDIUM_DYNAMICSSOLID)) ? true : false;
		}
	}

	if(_pWeaponColInfo->m_bIsPlayerPhys != IsPlayerPhys)
		_GunBlendVal = 0.001f; // force a blend

	_pWeaponColInfo->m_bIsPlayerPhys = IsPlayerPhys;
	_pWeaponColInfo->m_CollsionNormal = _pCollisInfo->m_Plane.n;
	if(IsPlayerPhys)
		_pWeaponColInfo->m_CollisionPoint = _pCollisInfo->m_Pos - (m_CameraLookMat.GetRow(0) * 8.0f);
	else
		_pWeaponColInfo->m_CollisionPoint = _pCollisInfo->m_Pos - (m_CameraLookMat.GetRow(0) * 2.0f);

	_pWeaponColInfo->m_UnInterpolLenWeponToPt = 
		Max((*_pColLineStart - *_pWeaponPos).Length() - (*_pColLineStart - _pWeaponColInfo->m_CollisionPoint).Length(), 0.0f);
}

bool CPostAnimSystem::SetupCollisionValuesAndCollideBox(uint16 _WeaponPose, SWeaponPose *_pPoses, CCollisionInfo *_pCollisInfo, CWorld_PhysState* _pWPhysState, 
														const CMat4Dfp32 *_pWeaponMat, fp32 _HeightMod, fp32 _WidthMod, bool &_Spec2H2ndItCol, CSelection *_pSelection, bool _bInitSelection)
{
	bool bGotCollision = false;
	CVec3Dfp32 WeaponCenter;
	
	//------------------------------------------------------------

	// get line-collision depending on current pose
	WeaponCenter = m_MatWeapon.GetRow(3) + 
				(m_CameraLookMat.GetRow(0) * -_pPoses[_WeaponPose].m_SearchLenMod) + 
				(m_CameraLookMat.GetRow(2) * LONGWEAPONHEIGHT * 0.5f);

	WeaponCenter += (	m_CameraLookMat.GetRow(0) * _pPoses[_WeaponPose].m_Position.k[0]+ 
						m_CameraLookMat.GetRow(1) * _pPoses[_WeaponPose].m_Position.k[1] + 
						m_CameraLookMat.GetRow(2) * _pPoses[_WeaponPose].m_Position.k[2]);

	// slight mod for collision tweaking
	WeaponCenter += (	m_CameraLookMat.GetRow(0) * _pPoses[_WeaponPose].m_LineCollisionMod.k[0]+ 
						m_CameraLookMat.GetRow(1) * _pPoses[_WeaponPose].m_LineCollisionMod.k[1] + 
						m_CameraLookMat.GetRow(2) * _pPoses[_WeaponPose].m_LineCollisionMod.k[2]);

	//-------------------------------------------------------------
	// do boxcollision test
	CWO_PhysicsState PhysState;
	PhysState.m_nPrim = 1;
	PhysState.m_PhysFlags = OBJECT_PHYSFLAGS_ROTATION;
	PhysState.m_MediumFlags =  XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS | XW_MEDIUM_DYNAMICSSOLID;
	PhysState.m_ObjectFlags = 0;
	PhysState.m_ObjectIntersectFlags = OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PLAYERPHYSMODEL | OBJECT_FLAGS_CHARACTER;

	CVec3Dfp32 PhysDimension = CVec3Dfp32(_pPoses[_WeaponPose].m_SearchLenMod * 0.5f, _WidthMod , _HeightMod);
	CVec3Dfp32 Offset = CVec3Dfp32(0,0,0);
	PhysState.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, PhysDimension, Offset);

	CMat4Dfp32 SourcePos;
	SourcePos = *_pWeaponMat;
	SourcePos.GetRow(0) = m_CameraLookMat.GetRow(0);
	SourcePos.RecreateMatrix(0,2);
	SourcePos.GetRow(3) = WeaponCenter;
	SourcePos.RotX_x_M(-_pPoses[_WeaponPose].m_Rotation.k[0]); // rotation for thoes special tweaked poses

	CMat4Dfp32 DestPos;
	DestPos = SourcePos;
	DestPos.GetRow(3) += DestPos.GetRow(0);

	// setup selection
	if(_bInitSelection)
	{
		CWObject_CoreData *pPlayer = m_iPlayerObject ? _pWPhysState->Object_GetCD(m_iPlayerObject) : NULL;
		const CBox3Dfp32 *pPlayerABsBbox =  pPlayer ? pPlayer->GetAbsBoundBox() :NULL;
		if(pPlayerABsBbox)
		{
			if(!_pSelection)
				return false;
			
			CVec3Dfp32 SelBoxMin = pPlayer->GetPosition() + m_CameraLookMat.GetRow(1) * 6.0f;
			CVec3Dfp32 SelBoxMax = (PhysDimension * SourcePos) + (SourcePos.GetRow(0) * _pPoses[_WeaponPose].m_SearchLenMod) + SourcePos.GetRow(1) * 2.0f * -_WidthMod;

			CBox3Dfp32 colbox;
			colbox.m_Max = SelBoxMax;
			colbox.m_Min = SelBoxMin;

			CBox3Dfp32 BoxSrc;
			_pWPhysState->Phys_GetMinMaxBox(PhysState, SourcePos, BoxSrc);
			colbox.Expand(BoxSrc);
			CMat4Dfp32 temp;
			temp = SourcePos;
			temp.GetRow(3)+=(SourcePos.GetRow(0) * _pPoses[_WeaponPose].m_SearchLenMod);
			_pWPhysState->Phys_GetMinMaxBox(PhysState, temp, BoxSrc);
			colbox.Expand(BoxSrc);
			
#ifdef USEJAKOBSDEBUGINFO
		_pWPhysState->Debug_RenderAABB(colbox);
#endif
			int iObjFlags = OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PLAYERPHYSMODEL | OBJECT_FLAGS_CHARACTER;
			_pWPhysState->Selection_AddBoundBox(*_pSelection, iObjFlags, colbox.m_Min, colbox.m_Max);
		}
		else
			_pSelection = NULL;		
	}
	



	uint8 iCollisionSteps = 2;
	for(uint8 i = 0; i < iCollisionSteps; i++)
	{
		_pCollisInfo->Clear();
		_pCollisInfo->m_CollisionType = CXR_COLLISIONTYPE_BOUND;
		//CollisInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_POSITION | CXR_COLLISIONRETURNVALUE_SURFACE);

		if(_pWPhysState->Phys_IntersectWorld(_pSelection, PhysState, SourcePos, DestPos, m_iPlayerObject, _pCollisInfo))
		{
			bGotCollision = true;
			if(i==1)
				_Spec2H2ndItCol = true;
		}

		//----------------------------------------------------------------------
#ifdef USEJAKOBSDEBUGINFO
		if(bGotCollision)
		{
			if(_pCollisInfo->m_bIsValid)
			{
				CMat4Dfp32 ColPos;
				ColPos.Unit();
				ColPos.GetRow(3) = _pCollisInfo->m_Pos;
				_pWPhysState->Debug_RenderSphere(ColPos, 3.0f);
				_pWPhysState->Debug_RenderOBB(DestPos, PhysDimension, 0xff00ff00);
			}
			else
				_pWPhysState->Debug_RenderOBB(DestPos, PhysDimension, 0xffff0000);

			break; // got collision, no need to go on.
		}
		else
			_pWPhysState->Debug_RenderOBB(DestPos, PhysDimension, 0xff0000ff);
#endif
		//----------------------------------------------------------------------

		if(bGotCollision)
			break;

		DestPos.GetRow(3) += (SourcePos.GetRow(0) * _pPoses[_WeaponPose].m_SearchLenMod);
	}

	return bGotCollision;
}

//--------------------------------------------------------------------------------------------------
// called from OnClientRefresh()
// returns false if there is no collision and all is OK.
bool CPostAnimSystem::GatherWeaponWallCollisionData(int8 _WeaponNr, CWorld_PhysState* _pWPhysState, uint16 _WeaponPose, CSelection *_pSelection)
{ 
	if(m_WeaponHoldType == WEAPONHOLD_TYPE_NOWEAPON)
		return false;

	CCollisionInfo CollisInfo;
	CVec3Dfp32 ColLineStart;
	CVec3Dfp32 ColLineEnd;

	SetNormalPoseRange(_WeaponPose);

	bool bInitSelection = false;
	if(_WeaponNr == -1)
	{
		bInitSelection = true;
		m_IKFlags &= ~IKFLAGS_OPENDOOR;

		_WeaponNr = m_WeaponHoldType;
		m_IKFlags &= ~IKFLAGS_DISABLE_RIGHTAIM;
		m_IKFlags &= ~IKFLAGS_DISABLE_LEFTAIM;

		m_WeaponPoseLeft = WEAPON_POSETEMPLATE_NORMAL;
		m_WeaponPoseRight =WEAPON_POSETEMPLATE_NORMAL;

		m_RightWeaponColInfo.m_UnInterpolLenWeponToPt = 0.0f;
		m_LeftWeaponColInfo.m_UnInterpolLenWeponToPt = 0.0f;
	}

	bool bGotCollision = false;
	bool b2HTposeIt2HasCol = false;
	bool bAquireColPlaneNormal = false;

	CVec3Dfp32 KindofWeaponPos = 0; // TODO: Make nice. not like this
	switch(_WeaponNr) {
		case WEAPONHOLD_TYPE_TWOHANDED:	
		bGotCollision = SetupCollisionValuesAndCollideBox(_WeaponPose, m_WeaponPoses_2H, &CollisInfo, _pWPhysState, &m_MatWeapon, LONGWEAPONHEIGHT, LONGWEAPONWIDTH, b2HTposeIt2HasCol, _pSelection, bInitSelection);		
		//SetupCollisionValues(KindofWeaponPos, _WeaponPose, ColLineStart, ColLineEnd, m_WeaponPoses_2H, LONGWEAPONLENGTH, LONGWEAPONHEIGHT, LONGWEAPONWIDTH, 0.0f, 1.0f, &m_MatWeapon);
		m_WeaponPoseRight = _WeaponPose;
		break;

	case WEAPONHOLD_TYPE_DUALWIELD:
		GatherWeaponWallCollisionData(WEAPONHOLD_TYPE_RIGHTGUN, _pWPhysState, WEAPON_POSETEMPLATE_NORMAL, _pSelection);
		GatherWeaponWallCollisionData(WEAPONHOLD_TYPE_LEFTGUN, _pWPhysState, WEAPON_POSETEMPLATE_NORMAL, _pSelection);
		return false; // return whatever.. doesn't mater

	case WEAPONHOLD_TYPE_RIGHTGUN:
	case WEAPONHOLD_TYPE_ONLY_1_1HGUN: 
		SetupCollisionValues(KindofWeaponPos, _WeaponPose, ColLineStart, ColLineEnd, m_WeaponPoses_1H, SHORTWEAPONLENGTH, SHORTWEAPONHEIGHT, 0.0f, 15.0f, 1.4f, &m_MatWeapon);
		m_WeaponPoseRight = _WeaponPose;
		break;

	case WEAPONHOLD_TYPE_LEFTGUN:
		SetupCollisionValues(KindofWeaponPos, _WeaponPose, ColLineStart, ColLineEnd, m_WeaponPoses_1H, SHORTWEAPONLENGTH, SHORTWEAPONHEIGHT, 0.0f, 15.0f, 1.4f, &m_MatWeapon2, -1);
		m_WeaponPoseLeft = _WeaponPose;
		break;

	default:
		return false;
	}

	//--------------------------------------------------------------------------------------------------------------
	// old linestyle collision
	if(_WeaponNr != WEAPONHOLD_TYPE_TWOHANDED)
	{
		CollisInfo.Clear();
		CollisInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE; // OBJECT_FLAGS_PROJECTILE  will include collisions at player
		CollisInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_POSITION | CXR_COLLISIONRETURNVALUE_SURFACE);
		int ObjectFlags = OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PLAYERPHYSMODEL;
		int MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS | XW_MEDIUM_DYNAMICSSOLID;
		bGotCollision = _pWPhysState->Phys_IntersectLine(ColLineStart, ColLineEnd, OBJECT_FLAGS_PHYSOBJECT, ObjectFlags, MediumFlags, &CollisInfo, m_iPlayerObject);
	}

#ifdef USEJAKOBSDEBUGINFO
	//if(_WeaponNr == WEAPONHOLD_TYPE_RIGHTGUN || _WeaponNr == WEAPONHOLD_TYPE_TWOHANDED)
	{
		switch(_WeaponPose)
		{
		case 0:
		case 1:
		case 2:
		case 3:
			_pWPhysState->Debug_RenderWire(ColLineStart,ColLineEnd, 0xff0000ff);
			break;
		case 4:
			_pWPhysState->Debug_RenderWire(ColLineStart,ColLineEnd, 0xff00ff00);
			break;
		case 5:
			_pWPhysState->Debug_RenderWire(ColLineStart,ColLineEnd, 0xffff0000);
			break;
		default:
			_pWPhysState->Debug_RenderWire(ColLineStart,ColLineEnd, 0xffffffff);
			break;
		}
	}
#endif

	//--------------------------------------------------------------------------------------------------------------

	_WeaponPose++;
	SetNormalPoseRange(_WeaponPose);
	//----------------------------------------------------------------------------
	
	if(bGotCollision)
	{
		// store collisionpoint from the dualwield position
		if((_WeaponPose-1) < WEAPON_POSETEMPLATE_NORMAL_LAST+1)
		{
			switch(_WeaponNr) {
			case WEAPONHOLD_TYPE_TWOHANDED:
			case WEAPONHOLD_TYPE_ONLY_1_1HGUN:
			case WEAPONHOLD_TYPE_RIGHTGUN:
				StoreCollisionInfo(&CollisInfo, &ColLineStart, &KindofWeaponPos, _pWPhysState, &m_RightWeaponColInfo, true, m_GunPoseBlendValueRight);
				break;

			case WEAPONHOLD_TYPE_LEFTGUN:
				StoreCollisionInfo(&CollisInfo, &ColLineStart, &KindofWeaponPos, _pWPhysState, &m_LeftWeaponColInfo, false, m_GunPoseBlendValueLeft);
				break;
			default:
				break;
			}
		}
		else if(_WeaponNr == WEAPONHOLD_TYPE_TWOHANDED && b2HTposeIt2HasCol && ((_WeaponPose-1) == WEAPON_POSETEMPLATE_NORMAL_LAST+1))
		{
			// with the dualwield, we can check second gun first collision to determin corner or wall. Not with 2h so... ->
			m_RightWeaponColInfo.m_CollsionNormal = CollisInfo.m_Plane.n;
			m_RightWeaponColInfo.m_CollisionPoint = CollisInfo.m_Pos - (m_CameraLookMat.GetRow(0)* 2.0f);
			m_RightWeaponColInfo.m_UnInterpolLenWeponToPt = 1.0f;
		}

		// try to find a position so the bullet trajectory is not hindered
		// OK-Pose found. Store pose ID
		if((_WeaponPose < WEAPON_POSETEMPLATE_SIZE-1) && !(GatherWeaponWallCollisionData(_WeaponNr, _pWPhysState, _WeaponPose, _pSelection)))
			return false;

		switch(_WeaponNr) {
			case WEAPONHOLD_TYPE_TWOHANDED:
				//if(m_RightWeaponColInfo.m_bIsPlayerPhys)
				
				m_IKFlags |= IKFLAGS_DISABLE_RIGHTAIM;
				m_WeaponPoseRight = WEAPON_POSE_GUNSLINGER_DUEL;
				break;

			case WEAPONHOLD_TYPE_RIGHTGUN:
				if(!m_RightWeaponColInfo.m_bIsPlayerPhys)
					m_IKFlags |= IKFLAGS_DISABLE_RIGHTAIM;
				m_WeaponPoseRight = WEAPON_POSE_GUNSLINGER_DUEL;	
				break;

			case WEAPONHOLD_TYPE_LEFTGUN:
				if(!m_LeftWeaponColInfo.m_bIsPlayerPhys)
					m_IKFlags |= IKFLAGS_DISABLE_LEFTAIM;
				m_WeaponPoseLeft = WEAPON_POSE_GUNSLINGER_DUEL;
				break;

			case WEAPONHOLD_TYPE_ONLY_1_1HGUN:
				if(!m_RightWeaponColInfo.m_bIsPlayerPhys)
					m_IKFlags |= IKFLAGS_DISABLE_RIGHTAIM;
				m_WeaponPoseRight = WEAPON_POSE_GUNSLINGER_DUEL;	
				break;

			default:
				break;
		}
	
		return true;
	}

	// no collision found, ok to use this pose	
	return false;
}

void CPostAnimSystem::CopyValues(const CPostAnimSystem *_pSource)
{
	m_IKFlags = _pSource->m_IKFlags;
    m_IKFlags2 = _pSource->m_IKFlags2;
	m_WeaponHoldType = _pSource->m_WeaponHoldType;

	// timer
	m_LastTime = _pSource->m_LastTime;
	m_fDeltaTime = _pSource->m_fDeltaTime;

	//-----------------------------------------------------
	// foot IK
	m_CurRFPAPoint = _pSource->m_CurRFPAPoint;
	m_CurLFPAPoint = _pSource->m_CurLFPAPoint;

	// Front feet
	m_CurRFrontFootPAPoint = _pSource->m_CurRFrontFootPAPoint;
	m_CurLFrontFootPAPoint = _pSource->m_CurLFrontFootPAPoint;
	m_Diffr = _pSource->m_Diffr;
	m_LastPos = _pSource->m_LastPos;
	m_CharSpeed = _pSource->m_CharSpeed;
	m_StairMoveType = _pSource->m_StairMoveType;
	m_PosOverride = _pSource->m_PosOverride;

	// darkling foot rotation
	m_LeftFrontFootRotation = _pSource->m_LeftFrontFootRotation;
	m_RightFrontFootRotation = _pSource->m_RightFrontFootRotation;

	// darkling back rotation for convex corners
	//m_DarklingBackRotation = _pSource->m_DarklingBackRotation;
	//m_DarklingBackRotationSpeed = _pSource->m_DarklingBackRotationSpeed;
	//m_DarklingBackMaxRotation = _pSource->m_DarklingBackMaxRotation;

	m_DisableDarklingIK = _pSource->m_DisableDarklingIK;
	m_DisableHumanFeetIK = _pSource->m_DisableHumanFeetIK;

	// aim point
	m_AimPointLeft = _pSource->m_AimPointLeft;
	m_AimTimeLeft = _pSource->m_AimTimeLeft;
	m_AimTimeStoreLeft = _pSource->m_AimTimeStoreLeft;
	m_AimBlendTimeLeft = _pSource->m_AimBlendTimeLeft;
	m_fAimAngleWeaponLeft = _pSource->m_fAimAngleWeaponLeft;

	m_AimPointRight = _pSource->m_AimPointRight;
	m_AimTimeRight = _pSource->m_AimTimeRight;
	m_AimTimeStoreRight = _pSource->m_AimTimeStoreRight;
	m_AimBlendTimeRight = _pSource->m_AimBlendTimeRight;
	m_fAimAngleWeaponRight = _pSource->m_fAimAngleWeaponRight;

	m_fAimAngleHead = _pSource->m_fAimAngleHead; // TODO: should not be copied

	// used to store values retrieved by collision-detection calls
	m_ModelNode0 = _pSource->m_ModelNode0;

	m_LeftFrontFootTarget = _pSource->m_LeftFrontFootTarget;
	m_RightFrontFootTarget = _pSource->m_RightFrontFootTarget;
	m_LeftBackFootTarget = _pSource->m_LeftBackFootTarget;
	m_RightBackFootTarget = _pSource->m_RightBackFootTarget;

	m_MatRightFootAnim = _pSource->m_MatRightFootAnim;
	m_MatLeftFootAnim = _pSource->m_MatLeftFootAnim;
	m_MatFrontRightFootAnim = _pSource->m_MatFrontRightFootAnim;
	m_MatFrontLeftFootAnim = _pSource->m_MatFrontLeftFootAnim;

	m_CameraLookMat = _pSource->m_CameraLookMat;
	
	m_MatWeapon = _pSource->m_MatWeapon;
	m_MatWeapon2 = _pSource->m_MatWeapon2;
	//m_LastAbsAimTargetPos = _pSource->m_LastAbsAimTargetPos;
	m_AbsAimTargetPos = _pSource->m_AbsAimTargetPos;
			
	m_WeaponPoseLeft = _pSource->m_WeaponPoseLeft;
	m_WeaponPoseRight = _pSource->m_WeaponPoseRight;
	m_LastWeaponPoseLeft = _pSource->m_LastWeaponPoseLeft;
	m_LastWeaponPoseRight = _pSource->m_LastWeaponPoseRight;
	m_GunPoseBlendValueLeft = _pSource->m_GunPoseBlendValueLeft;
	m_GunPoseBlendValueRight = _pSource->m_GunPoseBlendValueRight;
	m_CurrentDestWeaponPoseLeft = _pSource->m_CurrentDestWeaponPoseLeft;
	m_CurrentDestWeaponPoseRight = _pSource->m_CurrentDestWeaponPoseRight;
	
	m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_BLENDFROMRIGHT] = _pSource->m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_BLENDFROMRIGHT];
	m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_BLENDFROMLEFT] = _pSource->m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_BLENDFROMLEFT];
	m_RightAimInAndOutBlendVal = _pSource->m_RightAimInAndOutBlendVal;
	m_LeftAimInAndOutBlendVal = _pSource->m_LeftAimInAndOutBlendVal;

	m_CurrentWeaponBlendTypeLeft = _pSource->m_CurrentWeaponBlendTypeLeft;
	m_LastWeaponBlendTypeLeft = _pSource->m_LastWeaponBlendTypeLeft;
	m_CurrentWeaponBlendTypeRight = _pSource->m_CurrentWeaponBlendTypeRight;
	m_LastWeaponBlendTypeRight = _pSource->m_LastWeaponBlendTypeRight;

	m_LeftHandBlendVal = _pSource->m_LeftHandBlendVal;
	m_RightHandBlendVal = _pSource->m_RightHandBlendVal;

	// recoil values
	m_RecoilTimerLeft = _pSource->m_RecoilTimerLeft;
	m_RecoilTimerRight = _pSource->m_RecoilTimerRight;
	m_LeftRecoilType = _pSource->m_LeftRecoilType;
	m_RightRecoilType = _pSource->m_RightRecoilType;
	m_RecoilCamMod = _pSource->m_RecoilCamMod;
	
	// weapon delay
	m_LastRightLookVec = _pSource->m_LastRightLookVec;
	m_fTurningRotAngle = _pSource->m_fTurningRotAngle;
	m_fTurningRotAngle1 = _pSource->m_fTurningRotAngle1;

	m_LeftRecoilSubType  = _pSource->m_LeftRecoilSubType;
	m_RightRecoilSubType = _pSource->m_RightRecoilSubType;

	m_CurIntrpAimTargetPos = _pSource->m_CurIntrpAimTargetPos;
	// m_pModelInstanceWeapon = _pSource->m_pModelInstanceWeapon;

	m_iPlayerObject = _pSource->m_iPlayerObject;

	m_DWNormalChangeAtTick =_pSource-> m_DWNormalChangeAtTick;
	m_RandomDWNormalPose = _pSource->m_RandomDWNormalPose;

	m_LastCameraLookVec = _pSource->m_LastCameraLookVec;
	m_MovementLineColMod = _pSource->m_MovementLineColMod;

	// Aimingmode
	m_Anim_iCurIKLookMode = _pSource->m_Anim_iCurIKLookMode;
	m_Anim_iLastIKLookMode = _pSource->m_Anim_iLastIKLookMode;
	m_Anim_IKLookBlend = _pSource->m_Anim_IKLookBlend;
	m_ForcedAimingMode = _pSource->m_ForcedAimingMode;
	m_Aim_SkeletonType = _pSource->m_Aim_SkeletonType;

	m_RightWeaponColInfo = _pSource->m_RightWeaponColInfo;
	m_LeftWeaponColInfo = _pSource->m_LeftWeaponColInfo;

	m_TargetBlendOut = _pSource->m_TargetBlendOut;

	m_FeetBlendVal = _pSource->m_FeetBlendVal;
	m_FrontFeetBlendVal = _pSource->m_FrontFeetBlendVal;

	m_RightFootSin = _pSource->m_RightFootSin;
	m_LeftFootSin = _pSource->m_LeftFootSin;

	m_AILookDir = _pSource->m_AILookDir;
}

void CPostAnimSystem::RotateBoneAroundAxis(fp32 _Angle, const CVec3Dfp32 *_RotAxis, uint8 _iNode, CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CWorld_PhysState* _pWPhysState, bool _bEvalChildNodes)
{
	if(_Angle == 0.0f)
		return;

	CMat4Dfp32 MatPlaneRotation;
	CQuatfp32(*_RotAxis, -_Angle).CreateMatrix3x3(MatPlaneRotation);

	CMat4Dfp32 TmpMat;
	TmpMat = _pSkelInstance->m_pBoneTransform[_iNode];
	TmpMat.GetRow(3) = 0.0f;

	CMat4Dfp32 ResultRot;
	TmpMat.Multiply3x3(MatPlaneRotation, ResultRot);

	// ResultRot to localspace
	uint8 iParentNode = _pSkel->m_lNodes[_iNode].m_iNodeParent;
	TmpMat = _pSkelInstance->m_pBoneTransform[iParentNode];
	TmpMat.GetRow(3) = 0.0f; //ignore translation

	CMat4Dfp32 ParentInvMat;
	TmpMat.InverseOrthogonal(ParentInvMat);

	CMat4Dfp32 ResultRotLocal;
	ResultRot.Multiply3x3(ParentInvMat, ResultRotLocal);

	ResultRotLocal.GetRow(3) = 0.0f;
	_pSkelInstance->m_pBoneLocalPos[_iNode] = ResultRotLocal;

	if(_bEvalChildNodes)
	{
		_pSkel->EvalNode(_iNode, &_pSkelInstance->m_pBoneTransform[iParentNode], _pSkelInstance);
	}
	else
	{
		_pSkel->EvalNode_IK_Special(_iNode, &_pSkelInstance->m_pBoneTransform[iParentNode], _pSkelInstance, 0);
	}
}

