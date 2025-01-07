/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			Post Animation system.

Author:			Fredrik Johansson
				Jakob Ericsson

Copyright:		2005 Starbreeze Studios AB

Contents:		Post-processing of animations.

Comments:		Any post-processing of animations can be put in this class, but it was created 
mainly for IK processing. 

Every char needs to keep a copy of this object, since interpolation values are being
stored as member variables.. 

History:
051207:		Created file
060109:		Rewrote pretty much everything
\*____________________________________________________________________________________________*/
#ifndef __CPostAnimSystem_h__
#define __CPostAnimSystem_h__

#include "../../../Shared/MOS/XR/XRSkeleton.h"
///#include "WObj_Char.h"

#ifndef OGIER
#include "CIKSystem.h"
#endif

#define MULTMAT( M, M2 ) \
{ CMat4Dfp32 Tmp; M2.Multiply(M, Tmp); M = Tmp; }

#define MULTMATMP( M, M2 ) \
{ CMat43fp32 Tmp; M2.Multiply(M, Tmp); M = Tmp; }

#define APPLYQUAT(Q1, Q2, B, Blend) \
{\
	if(Blend > 0)\
	{\
	if(!B)\
	Q2.Unit();\
		Q1.Lerp(Q2, Blend, Q2);\
	}\
	else\
	Q2 = Q1;\
	B = true;\
}

class CPostAnimSystem
{
public:

	enum
	{
		// feettype
		FEET_TYPE_HUMAN_BIPED = 0,
		FEET_TYPE_DARKLING_QUADRO,

		// feet movementtype
		FOOT_MOVEMENT_UNDEFINED = 0,
		FOOT_MOVEMENT_STAIR_UP,
		FOOT_MOVEMENT_STAIR_DOWN,
		FOOT_MOVEMENT_STAIR_DOWN_MOVING, // not used right now
		FOOT_MOVEMENT_ANIMDRIVEN, // no IK after char is raised to animposition
		FOOT_MOVEMENT_FALLING, // not used right now

		// weapon holdtype
		WEAPONHOLD_TYPE_TWOHANDED = 0,
		WEAPONHOLD_TYPE_DUALWIELD,
		WEAPONHOLD_TYPE_RIGHTGUN,
		WEAPONHOLD_TYPE_LEFTGUN,
		WEAPONHOLD_TYPE_ONLY_1_1HGUN, 
		WEAPONHOLD_TYPE_NOWEAPON,

		//m_IKFlags
		IKFLAGS_OVERRIDEPOSITION			=	M_Bit(0),	// IKsystem should lower the player
		IKFLAGS_FRONTLSHOULDCOLLIDE			=	M_Bit(1),	// Animation says that we need a collision for left front foot (darklings etc)
		IKFLAGS_FRONTRSHOULDCOLLIDE			=	M_Bit(2),	// Animation says that we need a collision for right front foot (darklings etc)
		IKFLAGS_LSHOULDCOLLIDE				=	M_Bit(3),	// Animation says that we need a collision for left foot
		IKFLAGS_RSHOULDCOLLIDE				=	M_Bit(4),	// Animation says that we need a collision for right foot
		IKFLAGS_WASWALLCLIMBERLASTFRAME		=	M_Bit(5),	// The darkling has release the wall this frame. Used to setup feet in the transition
		IKFLAGS_LONGDUALWIELDWEAPON_RIGHT	=	M_Bit(6),	// Indicates that we have a one-handed weapon that is very long
		IKFLAGS_LONGDUALWIELDWEAPON_LEFT	=	M_Bit(7),	// Indicates that we have a one-handed weapon that is very long
		IKFLAGS_ISSLOWERTHANRUNNING			=	M_Bit(8),	// The player does not run
		IKFLAGS_STANDINGSTILL				=	M_Bit(9),	// The player moves very slow or is idle
		IKFLAGS_LEFTFRONTFOOTHASCOLLISION	=	M_Bit(10),	// Foot has found a collision point (Left front)
		IKFLAGS_RIGHTFRONTFOOTHASCOLLISION	=	M_Bit(11),	// Foot has found a collision point (Right front)
		IKFLAGS_LEFTBACKFOOTHASCOLLISION	=	M_Bit(12),	// Found a collision point (Left foot)
		IKFLAGS_RIGHTBACKFOOTHASCOLLISION	=	M_Bit(13),	// Found a collision point (Right foot)
		IKFLAGS_FEETSETUPOK					=	M_Bit(14),	// Needed to indicate the all values are reseted to animation position, including interpolation points
		IKFLAGS_RECOILLEFTRUNNING			=	M_Bit(15),	// Running recoil for left gun
		IKFLAGS_RECOILRIGHTRUNNING			=	M_Bit(16),	// Running recoil for right gun
		IKFLAGS_WASHUMAN					=	M_Bit(17),	// Was darkling, is now human or the other way around
		IKFLAGS_CHECKFORDWLEN				=	M_Bit(18),	//
		IKFLAGS_GRABLEFTHAND				=	M_Bit(20),	// Left hand is grabbing
		IKFLAGS_GRABRIGHTHAND				=	M_Bit(21),	// Right hand is grabbing
		IKFLAGS_RELEASELEFT_GRAB			=	M_Bit(22),	// Left hand is releasing grab
		IKFLAGS_RELEASERIGHT_GRAB			=	M_Bit(23),	// Right hand is releasing grab
		IKFLAGS_CROUCHING					=	M_Bit(24),	// Player is crouching
		IKFLAGS_OPENDOOR					=	M_Bit(25),	// Player has collision with a door
		IKFLAGS_OPENDOOR_RIGHTHAND			=	M_Bit(26),	// Door opens to the right (if not set, door opens to the left). Requires OPENDOOR flag to be set
		IKFLAGS_FORCE_AI_LOOKDIR			=	M_Bit(27),	// Forcing AI head-rotation
		IKFLAGS_INWEAPONZOOMMODE			=	M_Bit(28),	// 
		IKFLAGS_DISABLE_LEFTAIM				=	M_Bit(29),	// Disables aiming up weapon to the abs aim point. Left hand
		IKFLAGS_DISABLE_RIGHTAIM			=	M_Bit(30),	// Disables aiming up weapon to the abs aimpoint. Right hand
		IKFLAGS_FACEVERYCLOSETOWALL			=	M_Bit(31),	//

		IKFLAGS2_INWEAPONZOOMOUTMODE		=	M_Bit(0),
		IKFLAGS2_2H_DOORGRABBOOL1			=	M_Bit(1),
		IKFLAGS2_2H_DOORGRABBOOL2			=	M_Bit(2),
		IKFLAGS2_SERVERMODEL0SET			=	M_Bit(3),

		/*
		IKFLAGS2_	=	M_Bit(3),
		IKFLAGS2_	=	M_Bit(4),
		IKFLAGS2_	=	M_Bit(5),
		IKFLAGS2_	=	M_Bit(6),
		IKFLAGS2_	=	M_Bit(7),
		IKFLAGS2_	=	M_Bit(8),
		IKFLAGS2_	=	M_Bit(9),
		IKFLAGS2_	=	M_Bit(10),
		IKFLAGS2_	=	M_Bit(11),
		IKFLAGS2_	=	M_Bit(12),
		IKFLAGS2_	=	M_Bit(13),
		IKFLAGS2_	=	M_Bit(14),
		IKFLAGS2_	=	M_Bit(15),
		IKFLAGS2_	=	M_Bit(16),
		IKFLAGS2_	=	M_Bit(17),
		IKFLAGS2_	=	M_Bit(18),
		IKFLAGS2_	=	M_Bit(20),
		IKFLAGS2_	=	M_Bit(21),
		IKFLAGS2_	=	M_Bit(22),
		IKFLAGS2_	=	M_Bit(23),
		IKFLAGS2_	=	M_Bit(24),
		IKFLAGS2_	=	M_Bit(25),
		IKFLAGS2_	=	M_Bit(26),
		IKFLAGS2_	=	M_Bit(27),
		IKFLAGS2_	=	M_Bit(28),
		IKFLAGS2_	=	M_Bit(29),
		IKFLAGS2_	=	M_Bit(30),
		IKFLAGS2_	=	M_Bit(31),*/

	};

	uint32 m_IKFlags;
	uint32 m_IKFlags2;

	CPostAnimSystem();
	~CPostAnimSystem();

	void ResetValues();

	void EvalAimIK(CXR_SkeletonInstance* _pSkelInstance, fp32 _IPTime, CMat4Dfp32& _MatBody,  CMat4Dfp32& _MatLook, const CVec3Dfp32& _UpVector);
	void RotateBoneAbsolute(const CQuatfp32 &_Q, CXR_SkeletonInstance *_pSkelInstance, int _iRotTrack, int _iParent);

	void EvalHandsOnWeapon(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CWObject_CoreData* _pObj, CXR_Model* _pModel, CWorld_PhysState* _pWPhysState);
	bool EvalHandsForGrab(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CWorld_PhysState* _pWPhysState, class CWO_Character_ClientData *pCD);
	
	void DisableFeetIK(bool _DisbleIt);
	void EvalFeetIK(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CWObject_CoreData* _pObj, CXR_Model* _pModel, 
		CWorld_PhysState* _pWPhysState, uint8 _FeetType, int _PhysType, bool _IsWallClimber);

	void SetGrabPoint(const CVec3Dfp32 *_pAimPointLeft, fp32 _AimTimeLeft, const CVec3Dfp32 *_pAimPointRight, fp32 _AimTimeRight);
	void ReleaseGrabPoint(bool _bReleaseLeft, bool _bReleaseRight);
		
	void UpdateTimer(const CMTime *_pTime);
	void CopyValues(const CPostAnimSystem *_pSource);

	void RotateBoneAroundAxis(fp32 _Angle, const CVec3Dfp32 *_RotAxis, uint8 _iNode, CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CWorld_PhysState* _pWPhysState, bool _bEvalChildNodes = false);

	void CheckStairType(CWorld_PhysState *_pWPhysState, fp32 _SkelScale, fp32 _ColMaxDiff, fp32 _ForwardDistanceForCol);
	void GatherFeetCollisionData(uint8 _FeetType, CWorld_PhysState* _pWPhysState, fp32 _SkelScale);
	bool GatherWeaponWallCollisionData(int8 _WeaponNr, CWorld_PhysState* _pWPhysState, uint16 _WeaponPose = WEAPON_POSETEMPLATE_NORMAL, CSelection *_pSelection = NULL);
	void GatherAimCollisionData(CWorld_PhysState* _pWPhysState, int _iTarget, bool _bIsPlayer);

	void InitRecoil(bool _bIsLeftHand, int _RecoilType, int _RandVal);
	void RecoilModCamera(CMat4Dfp32 *_pCamMat, CWO_Character_ClientData *_pCD);

	void SetHeadLookDir(const CVec3Dfp32 *_pAILookDir);
	void UpdateHeadLookDir(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CWorld_PhysState* _pWPhysState);
	//void OverideAbsAimPoint(const CVec3Dfp32 *_pOverrideAbsPt) {m_AbsAimTargetPos = *_pOverrideAbsPt ;}

	M_INLINE fp32 GetDiffHeight() const { return m_Diffr; }
	//M_INLINE void ZeroDiffHeight() { m_Diffr = 0.0f; }

	M_INLINE bool GetSafeToFirePrim() const { return ((m_WeaponHoldType == WEAPONHOLD_TYPE_TWOHANDED && m_WeaponPoseRight != WEAPON_POSE_GUNSLINGER_DUEL) || m_WeaponHoldType != WEAPONHOLD_TYPE_TWOHANDED); }
	M_INLINE bool GetSafeToFireSec() const { return ((m_WeaponHoldType == WEAPONHOLD_TYPE_TWOHANDED && m_WeaponPoseLeft != WEAPON_POSE_GUNSLINGER_DUEL) || m_WeaponHoldType != WEAPONHOLD_TYPE_TWOHANDED); }
	M_INLINE void SetFeetSetupStatus(bool _bOn) 
	{ 
		if(_bOn)
			m_IKFlags |= IKFLAGS_FEETSETUPOK;
		else 
			m_IKFlags &= ~IKFLAGS_FEETSETUPOK;
	}
	
	M_INLINE bint GetHandIKDisabled() { return m_bDisableHandIK; }
	M_INLINE int8 GetStairType() const { return m_StairMoveType; }
	M_INLINE CVec3Dfp32 GetMainGunAimVec() const { return (m_CurIntrpAimTargetPos -m_MatWeapon.GetRow(3)).Normalize(); }
	M_INLINE CVec3Dfp32 GetSecondGunAimVec() const { return (m_CurIntrpAimTargetPos -m_MatWeapon2.GetRow(3)).Normalize(); }
	M_INLINE CVec3Dfp32 GetAimTargetPos() const { return m_CurIntrpAimTargetPos; }
	M_INLINE int8 GetWeaponHoldType() const { return m_WeaponHoldType; }
	M_INLINE CVec3Dfp32 GetPostanimWeaponPos(bool _bMainWeapon = true) const { return _bMainWeapon ? m_MatWeapon.GetRow(3) : m_MatWeapon2.GetRow(3);}
	//M_INLINE bool GetAutoAimOn() const {return !(m_IKFlags & IKFLAGS_DISABLE_AUTOAIM);}
	M_INLINE bool GetHandIsRetracted(bool _bMainWeapon) const {return _bMainWeapon ? 
																(m_WeaponPoseRight == WEAPON_POSE_GUNSLINGER_DUEL && !m_RightWeaponColInfo.m_bIsPlayerPhys) :
																(m_WeaponPoseLeft == WEAPON_POSE_GUNSLINGER_DUEL && !m_LeftWeaponColInfo.m_bIsPlayerPhys);}

	M_INLINE void SetCameraLookMat(const CMat4Dfp32 *_pSourceMat) {m_CameraLookMat = *_pSourceMat;}
	M_INLINE CVec3Dfp32 GetCameraLookDir() const {return m_CameraLookMat.GetRow(0);}
	M_INLINE CVec3Dfp32 GetCameraLookPos() const {return m_CameraLookMat.GetRow(3);}
	M_INLINE void MarkWeaponChanged() {m_IKFlags |= IKFLAGS_CHECKFORDWLEN;}
	//M_INLINE CMat4Dfp32 GetModel0Mat() const { return m_ModelNode0; }
	//M_INLINE void ZeroDeltaTime() {m_fDeltaTime = 0.0f;}
	//M_INLINE void SetAutoAimOnOff(bool _bSetOn) {if(_bSetOn) m_IKFlags &= ~IKFLAGS_DISABLE_AUTOAIM; else m_IKFlags |= IKFLAGS_DISABLE_AUTOAIM;}
	M_INLINE bool GetOKToWeaponZoom() const {return (m_WeaponHoldType != WEAPONHOLD_TYPE_NOWEAPON && (m_WeaponPoseLeft != WEAPON_POSE_GUNSLINGER_DUEL || m_WeaponPoseRight != WEAPON_POSE_GUNSLINGER_DUEL)) ;}
	M_INLINE void SetModel0Mat(const CMat4Dfp32 *_pMat) {m_ModelNode0 = *_pMat; m_IKFlags2 |= IKFLAGS2_SERVERMODEL0SET;}
	int8 m_Anim_iCurIKLookMode;
	int8 m_Anim_iLastIKLookMode;
	fp32 m_Anim_IKLookBlend;
	int8 m_ForcedAimingMode;
	int8 m_Aim_SkeletonType;

private:

	struct SWeaponPose
	{
		CVec3Dfp32 m_Position;
		CVec3Dfp32 m_Rotation;
		CVec3Dfp32 m_LineCollisionMod;
		fp32 m_SearchLenMod;
	};

	struct SDW_WeaponCollisionInfo
	{
		CVec3Dfp32 m_CollisionPoint;
		fp32 m_LenFromWeaponToColPt;
		fp32 m_UnInterpolLenWeponToPt;

		CVec3Dfp32 m_CollsionNormal;
		fp32 m_NormalDependentRotation;
		fp32 m_BarrelDependentRotation;

		bool m_bIsPlayerPhys;
	};

	void ApplyLookDependantWeaponMotion(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CWorld_PhysState* _pWPhysState);
	
	bool UpdateWeaponMatrices(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CWorld_PhysState* _pWPhysState, CWO_Character_ClientData *_pCDFirst, 
								CMat4Dfp32 &_RightWeapon, CMat4Dfp32 &_pLeftWeapon);

	void SetupNodesForAim(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CWorld_PhysState* _pWPhysState, CWO_Character_ClientData *pCD);
	bool BlendHandNodes(int _IKMode, CMat4Dfp32 *_pTarget, CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance);

	void SetupGunPosePositions(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CWorld_PhysState* _pWPhysState);

	bool CheckOKTorRunFeetIK(CWO_Character_ClientData *_pCD, uint8 _FeetType);
	void GetPointOnVector(const CVec3Dfp32 *_pP1, const CVec3Dfp32 *_pP2, CVec3Dfp32 *_pDest);

	void MoveFootTowardsTarget(bool _FootHasCollision, bool *_SolveLegIK, const CMat4Dfp32 *_pAnimFootMat, CMat4Dfp32 *_FootDestination, 
		CVec3Dfp32 *_CurPAPoint, const CVec3Dfp32 *_DownVec, bool _bIsFalling);
	void AddZMovementForFoot(CWO_Character_ClientData *pCD, int _Dirt, fp32 &_FootVal, CMat4Dfp32 &_OverrideMat, fp32 _Duration, fp32 _HeightMod);

	void UpdateRecoil(const CMat4Dfp32 *_pWeaponMat, const CVec3Dfp32 *_pRotAxis, CMat4Dfp32 *_pHandMat, bool _bIsLeftHand);
	int SetNormalPoseRange(uint16 &_WeaponPose);
	
	void SetupCollisionValues(CVec3Dfp32 &_RetAproxWPos, uint16 _WeaponPose, CVec3Dfp32 &_ColLineStart, CVec3Dfp32 &_ColLineEnd, SWeaponPose *_pPoses, 
								fp32 _SearchLenMod, fp32 _HeightMod, fp32 _WidthMod, fp32 _BackMod, fp32 _CharRotationModMult, const CMat4Dfp32 *_WeaponMat, int8 _LRDirMod = 1);	
	bool SetupCollisionValuesAndCollideBox(uint16 _WeaponPose, SWeaponPose *_pPoses, CCollisionInfo *_pCollisInfo, CWorld_PhysState* _pWPhysState, 
								const CMat4Dfp32 *_pWeaponMat, fp32 _HeightMod, fp32 _WidthMod, bool &_Spec2H2ndItCol, CSelection *_pSelection, bool _bInitSelection);
	void StoreCollisionInfo(const CCollisionInfo *_pCollisInfo, const CVec3Dfp32 *_pColLineStart, const CVec3Dfp32 *_pWeaponPos, 
								CWorld_PhysState* _pWPhysState, SDW_WeaponCollisionInfo *_pWeaponColInfo, bool _bIsMainWeapon, fp32 &_GunBlendVal);
	int GetRandomNormalPose();

	CVec3Dfp32 m_AILookDir;

#ifndef OGIER
	CIKSystem m_IKSolver;
#endif

	enum
	{
		// blendmodes
		WEAPONBLEND_TYPE_NONE = 0,
		WEAPONBLEND_TYPE_IN,
		WEAPONBLEND_TYPE_OUT,
		WEAPONBLEND_TYPE_FULL_IN,
		GRABBLEND_TYPE_IN,
		GRABBLEND_TYPE_OUT,
		GRABBLEND_TYPE_FULL_IN,

		// recoiltypes
		RECOIL_STRENGHT_LIGHT = 0,
		RECOIL_STRENGHT_MEDIUM,
		RECOIL_STRENGHT_HEAVY,
		RECOIL_STRENGHT_SIZE,

		// weaponpose templates. Templates are in prio-order! 
		WEAPON_POSETEMPLATE_NORMAL = 0,
		WEAPON_POSETEMPLATE_NORMAL_1,
		WEAPON_POSETEMPLATE_NORMAL_2,
		WEAPON_POSETEMPLATE_NORMAL_ZOOM,
		WEAPON_POSETEMPLATE_NORMAL_LAST, // if more dual wield normal positions are added, add them BEFORE this one
		WEAPON_POSETEMPLATE_T,
		WEAPON_POSETEMPLATE_GANGSTER,
		WEAPON_POSE_GUNSLINGER_DUEL,
		WEAPON_POSETEMPLATE_SIZE,

		WEAPON_POSETEMPLATE_BLENDFROMLEFT = 0,
		WEAPON_POSETEMPLATE_BLENDFROMRIGHT,
		WEAPON_POSETEMPLATE_SPECIAL_2HCROUCH,
		WEAPON_POSETEMPLATE_SPECIAL_2HDOOR_LEFT,
		WEAPON_POSETEMPLATE_SPECIAL_2HDOOR_RIGHT,
		WEAPON_POSETEMPLATE_EXTRA_SIZE,
	};



	SWeaponPose m_WeaponPoses_1H[WEAPON_POSETEMPLATE_SIZE];
	SWeaponPose m_WeaponPoses_2H[WEAPON_POSETEMPLATE_SIZE];
	SWeaponPose m_WeaponPoses_Extra[WEAPON_POSETEMPLATE_EXTRA_SIZE];

	// a set of "normal" base poses to randomize	
	uint16 m_RandomDWNormalPose;
	int m_DWNormalChangeAtTick;
	//fp32 m_fTimeLeftTilDWNormalChange;

	CVec3Dfp32 m_PosOverride;
	CMat4Dfp32 m_CameraLookMat;

	uint16 m_WeaponPoseLeft;
	uint16 m_WeaponPoseRight;
	uint16 m_LastWeaponPoseLeft;
	uint16 m_LastWeaponPoseRight;
	uint16 m_CurrentDestWeaponPoseLeft;
	uint16 m_CurrentDestWeaponPoseRight;
	fp32 m_GunPoseBlendValueLeft;
	fp32 m_GunPoseBlendValueRight;

	CVec3Dfp32 m_AbsAimTargetPos;
	CVec3Dfp32 m_MovementLineColMod;
	CVec3Dfp32 m_CurIntrpAimTargetPos;

	uint8 m_CurrentWeaponBlendTypeLeft;
	uint8 m_LastWeaponBlendTypeLeft;
	uint8 m_CurrentWeaponBlendTypeRight;
	uint8 m_LastWeaponBlendTypeRight;

	fp32 m_LeftHandBlendVal;
	fp32 m_RightHandBlendVal;
	fp32 m_LeftAimInAndOutBlendVal;
	fp32 m_RightAimInAndOutBlendVal;

	int8 m_WeaponHoldType;

	// for moving weapon around obstacles
	CMTime m_LastTime;
	fp32 m_fDeltaTime;
	fp32 m_Fraction;

	//-----------------------------------------------------
	// foot IK
	CMat4Dfp32 m_RightFootDestination;
	CMat4Dfp32 m_LeftFootDestination;
	
	fp32 m_Diffr;

	CVec3Dfp32 m_CurRFPAPoint;
	CVec3Dfp32 m_CurLFPAPoint;

	CVec3Dfp32  m_LastPos;
	CVec3Dfp32  m_CharSpeed;

	int8 m_StairMoveType;

	fp32 m_FeetBlendVal;
	fp32 m_FrontFeetBlendVal;

	//---------------------------------------------------
	// quadroped foot IK
	CMat4Dfp32 m_RightFrontFootDestination;
	CMat4Dfp32 m_LeftFrontFootDestination;

	CVec3Dfp32 m_CurRFrontFootPAPoint;
	CVec3Dfp32 m_CurLFrontFootPAPoint;

	// test
	fp32 m_LeftFrontFootRotation;
	fp32 m_RightFrontFootRotation;

	// darkling back rotation for convex corners
	//fp32 m_DarklingBackRotation;
	//fp32 m_DarklingBackRotationSpeed;
	//fp32 m_DarklingBackMaxRotation;

	bint m_DisableDarklingIK : 1;
	bint m_DisableHumanFeetIK : 1;
	bint m_bDisableHandIK : 1;

	//----------------------------------------------------
	// Aim point
	fp32 m_fAimAngleHead;
	
	CVec3Dfp32 m_AimPointLeft;
	fp32 m_AimTimeLeft;
	fp32 m_AimTimeStoreLeft;
	fp32 m_AimBlendTimeLeft;
	fp32 m_fAimAngleWeaponLeft;	
	
	CVec3Dfp32 m_AimPointRight;
	fp32 m_AimTimeRight;
	fp32 m_AimTimeStoreRight;
	fp32 m_AimBlendTimeRight;
	fp32 m_fAimAngleWeaponRight;	

	//-----------------------------------------------------
	// variables needed for collision calls
	CMat4Dfp32 m_ModelNode0;

	CVec3Dfp32 m_LeftFrontFootTarget;
	CVec3Dfp32 m_RightFrontFootTarget;
	CVec3Dfp32 m_LeftBackFootTarget;
	CVec3Dfp32 m_RightBackFootTarget;
	
	CMat4Dfp32 m_MatRightFootAnim;
	CMat4Dfp32 m_MatLeftFootAnim;
	CMat4Dfp32 m_MatFrontRightFootAnim;
	CMat4Dfp32 m_MatFrontLeftFootAnim;

	// for precalc weapon col
	CMat4Dfp32 m_MatWeapon;
	CMat4Dfp32 m_MatWeapon2;

	fp32 m_fAdditionalRightWeaponMovement; //  do NOT add in copy function
	fp32 m_fAdditionalLeftWeaponMovement; //  do NOT add in copy function

	// recoil
	fp32 m_RecoilTimerLeft;
	fp32 m_RecoilTimerRight;
	int m_LeftRecoilType;
	int m_RightRecoilType;
	uint8 m_LeftRecoilSubType;
	uint8 m_RightRecoilSubType;
	CVec3Dfp32 m_RecoilCamMod;

	CVec3Dfp32 m_LastRightLookVec;
	fp32 m_fTurningRotAngle1;
	fp32 m_fTurningRotAngle;

	// needed for exclusion in collision
	int m_iPlayerObject;
	fp32 m_TargetBlendOut;

	CVec3Dfp32 m_LastCameraLookVec;
	CVec3Dfp32 m_PlayerRotationDiff;

	SDW_WeaponCollisionInfo m_RightWeaponColInfo;
	SDW_WeaponCollisionInfo m_LeftWeaponColInfo;

	fp32 m_RightFootSin;
	fp32 m_LeftFootSin;
};


#endif
