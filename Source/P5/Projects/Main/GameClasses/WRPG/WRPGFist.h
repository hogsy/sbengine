/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WRPGFist.h
					
	Author:			Olle Rosenquist
					
	Copyright:		Copyright O3 Games AB 2002
					
	Contents:		
		WRPGFist.cpp
				CRPG_Object_Fist()
				OnCreate()
				OnEvalKey()
				Equip()
				UnEquip()
				OnActivate()
				Deactivate()
				OnProcess()
				DoCollisionTest()
				OnHit()
					
	Comments:		
					
	History:		
		020730:		Olle Rosenquist, Created File
\*____________________________________________________________________________________________*/

#include "WRPGWeapon.h"

#ifndef __WRPG_OBJECT_FIST
#define __WRPG_OBJECT_FIST

#ifndef __MACROINLINEACCESS
#define __MACROINLINEACCESS
#define MACRO_INLINEACCESS_RW(name, type) \
M_INLINE const type& Get##name() const { return m_##name; } \
M_INLINE type& Get##name() { return m_##name; }

#define MACRO_INLINEACCESS_R(name, type) \
M_INLINE const type& Get##name() const { return m_##name; }

#define MACRO_INLINEACCESS_RWEXT(name, variable, type) \
M_INLINE const type& Get##name() const { return variable; } \
M_INLINE type& Get##name() { return variable; }

#define MACRO_INLINEACCESS_REXT(name, variable, type) \
M_INLINE const type& Get##name() const { return variable; }
#endif

#define ACTIONDELAYCOUNTDOWNVALUE (20)

#define FIGHTMODE_INITIATERADIUS (40.0f)
#define FIGHTMODE_LEAVERADIUS (70.0f)
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TEMPORARY PLACEMENT OF FIST ANIMATION SCHEDULE THINGY
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*enum TimeMode
{
	ANIMATION_TIMEMODE_FINITE,
	ANIMATION_TIMEMODE_INFINITE,
	ANIMATION_TIMEMODE_ENDOFANIMATION,
	ANIMATION_TIMEMODE_HALFANIMATION,
};*/

// Only here because AI still uses it, should be removed!!
enum FistActions
	{
		FIST_ACTION_NONE			= 0,
		FIST_ACTION_PUNCHLEFT		= 1 << 0,
		FIST_ACTION_PUNCHRIGHT		= 1 << 1,
		FIST_ACTION_LEFTDODGEPUNCH	= 1 << 2,
		FIST_ACTION_RIGHTDODGEPUNCH = 1 << 3,
		
		FIST_MOVE_ENTERDODGELEFT	= 1 << 4,
		FIST_MOVE_ENTERDODGERIGHT	= 1 << 5,
		FIST_MOVE_ENTERBLOCK		= 1 << 6,
		FIST_MOVE_LEAVEDODGELEFT	= 1 << 7,
		FIST_MOVE_LEAVEDODGERIGHT	= 1 << 8,
		FIST_MOVE_LEAVEBLOCK		= 1 << 9,
		
		FIST_HURT_MIDDLE			= 1 << 10,
		FIST_HURT_LEFT				= 1 << 11,
		FIST_HURT_RIGHT				= 1 << 12,
		
		FIST_ACTION_GRAB			= 1 << 13,
		FIST_ACTION_BREAKGRAB		= 1 << 14,
		FIST_ACTION_SNEAKGRAB		= 1 << 15,
		FIST_ACTION_PICKUPITEM		= 1 << 16,
		FIST_ACTION_PICKUPBODY		= 1 << 17,
		FIST_ACTION_DROPBODY		= 1 << 18,
		FIST_MOVE_GRABINAIR			= 1 << 19,
		
		FIST_HURT_BLOCKLEFT			= 1 << 20,
		FIST_HURT_BLOCKRIGHT		= 1 << 21,
		FIST_HURT_BLOCKDODGELEFT	= 1 << 22,
		FIST_HURT_BLOCKDODGERIGHT	= 1 << 23,

		FIST_HURT_GENERIC			= 1 << 24,

		FIST_ACTION_NUMBEROFACTIONS = 25,

		FIST_STANCE_NUMBEROFSTANCES	= 6,
	};

class CWorld_Server;

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CStanceBlock, holds information for the different
					stances, ie what can be done in them and so on
					
	Comments:		
\*____________________________________________________________________*/
// Typedef smartpointer
/*class CStanceSwitch;
typedef TPtr<CStanceSwitch> spCStanceSwitch;*/

enum FightNew
{
	// Stances
	// Old stances that should be removed
	FIST_STANCE_MIDDLE			= 1 << 25,
	FIST_STANCE_LEFT			= 1 << 26,
	FIST_STANCE_RIGHT			= 1 << 27,
	FIST_STANCE_BLOCK			= 1 << 28,
	FIST_STANCE_GENERIC			= 1 << 29,
	FIST_STANCE_CARRYBODY		= 1 << 30,
	FIST_STANCE_NONE			= 0,
};

#define MACRO_ISINRANGE(_A,_B,_C) ((_C >= _A) ? (_C <= _B) : false)

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CRPG_Object_Fist
|__________________________________________________________________________________________________
\*************************************************************************************************/
#define MAXFIGHTATTACKS (10)
#define PLAYER_FIGHTING_DISTANCE (32)

#define CRPG_Object_Fist_Parent CRPG_Object_Weapon

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class: CRPG_Object_Fist
					
	Comments: Class for fist weapon
\*____________________________________________________________________*/

#define CRPG_Object_Fist_ItemType 0
#define CRPG_Object_KnuckleDuster_ItemType 8
#define CRPG_Object_Shank_ItemType 0x11
#define CRPG_Object_Club_ItemType 0x12
/*class CRPG_Object_Fist : public CRPG_Object_Weapon
{
	MRTC_DECLARE;
private:
	bool CanActivate(int _Pos, int _iObject, int _iOpponent, int _GameTick);

protected:
	friend class CAnimationSchedule;
	friend class CAnimationAction;
	uint32		m_LastActivatedTick;
	uint32		m_LastProcessedTick;
	//int			m_FightAttack;
	//int			m_ActiveStance;

	CMat4Dfp32	m_PunchMatrix;
	int			m_PunchTick;
	// Sounds to play when hit by hand/knee/kick respectivly
	int			m_iSound_HitHand;
	int			m_iSound_HitKnee;
	int			m_iSound_HitKick;
	int			m_iSound_HitSurf;

	int			m_HitObjectDelayCountdown;
	int			m_HitCharacterDelayCountdown;
	int			m_HitWorldDelayCountdown;
	
	// When exiting fight mode, make the character wait for a short while
	int			m_ActionDelay;

	// If the character has picked up a body it will lie here....
	int			m_iPickupBody;

	bool		m_bShouldBeDead;

	//spCAnimationBlock	m_lspAnimationBlock[FIST_ACTION_NUMBEROFACTIONS];
	
	//spCAnimationBlock	m_lspAnimationBlockMove[FIGHT_MOVE_NUMBEROFMOVES];

	//spCBlockActivation	m_spDodgeTest;
	
	//int					m_NumberOfBlocks;
	//CAnimationSchedule	m_AnimationSchedule;

	int			m_Input;

	fp32			m_HitRadius;

	// TEST FIXME REMAKE
	//CStanceBall		m_StanceBall;

	// Damage Effect /kma
	CStr m_DamageEffect;

	bool m_bIsActivatable;
	
	//spCAnimationManager m_spAnimationManager;

	//CAnimationSchedule* GetAnimationSchedule(int _iSender, int _iObject);
	
	//int GetPositionFromAction(int _FistAction);
	//int GetActionFromPosition(int _iPosition);

public:

	CRPG_Object_Fist();

	virtual void OnCreate();

	void OnIncludeClass(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);

	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	
	virtual bool Equip(int _iObject, CRPG_Object *_pRoot, int _iPrecedingUnequipAnim = 0, bool _bInstant = false, bool _bHold = false);

	virtual bool Unequip(int _iObject, CRPG_Object *_pRoot, int _iSucceedingEquipAnim = 0, bool _bInstant = false, bool _bHold = false);
	
	// Activate fist when entering fight mode
	bool ActivateFist(int _iObject, int _iOpponent);
	bool ActivateFistSneak(int _iObject);
	virtual bool OnActivate(const CMat4Dfp32 &, CRPG_Object *_pRoot, int _iObject, int _Input);

	//virtual bool Block(bool _bAutoBlock, const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _iSender);

	bool OnProcess(CRPG_Object *_pRoot, const CMat4Dfp32 &_Mat, int _iObject);

	//bool ActivateFistAction(FistActions _FistAction, int _iObject, int _iOpponent, int _StartTime = -1);
	//bool ActivateFistActionNoOpponent(FistActions _FistAction, int _iObject);
	//bool SetStanceAnimation(int _Stance, int _iObject, bool bReplace = false);

	//bool ActivateMoveActionNew(int _MoveAction, int _iObject, int _iOpponent, 
	//	int _StartTime = -1);
	//bool ActivateMoveActionNewNoOpponent(int _MoveAction, int _iObject);

	void DoCollisionTest(CWObject *_pObj, CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkelChar, const CVec3Dfp32 &_Force, bool _bSpawnSpecial);
	void OnHit(CCollisionInfo &_Info, int _iObject, const CVec3Dfp32 &_SplatterDir, const CVec3Dfp32 &_Force);

	virtual bool IsMeleeWeapon();

	//CWorld_Server* GetWorldServer();

	int GetManageableActions(int _GameTick, int _iObject, int _iOpponent);
	//static int ResolveFistAction(CStr _Str);

	// Adds used input
	void UpdateInput(int _Input);
	fp32 GetMoveLeftRight(int _iObject);
	fp32 GetMoveUpDown(int _iObject);
	int GetActiveStance(int _iObject);
	//void ExitFightMode(int _iObject);
	//bool EnterFightMode(int _iObject, int _Input);
	bool CharIsPlayer(int _iObject);
	bool CharIsPlayer(CWO_Character_ClientData* pCD);
//	void ResetStanceBall(int _iObject);
//	void ResetStanceBall(CWO_Character_ClientData* pCD);
//	void UpdateStanceBall(int _iObject, CWO_Character_ClientData* _pCDServer);

	//void ResetDirectAnimSeq(int _iObject);

	virtual bool IsEquippable(CRPG_Object* _pRoot) { return m_bIsActivatable; }
	virtual bool IsActivatable(CRPG_Object* _pRoot) { return m_bIsActivatable; }

	bool CanEnterFightMode();

	bool ShouldBeDead();

	//int GetSoundIndex(int _Type);
	void SetShouldBeDead() { m_bShouldBeDead = true; }
	
	MACRO_INLINEACCESS_RW(ActionDelay,int)
	MACRO_INLINEACCESS_REXT(SoundHand,m_iSound_HitHand,int)
	MACRO_INLINEACCESS_REXT(SoundKnee,m_iSound_HitKnee,int)
	MACRO_INLINEACCESS_REXT(SoundKick,m_iSound_HitKick,int)
	MACRO_INLINEACCESS_REXT(SoundSurf,m_iSound_HitSurf,int)
	MACRO_INLINEACCESS_RWEXT(Activatable,m_bIsActivatable,bool);
};
typedef TPtr<CRPG_Object_Fist> spCRPG_Object_Fist;*/

// TEST
#ifndef M_DISABLE_TODELETE
#define CRPG_Object_KnuckleDusterParent CRPG_Object_Weapon
class CRPG_Object_KnuckleDuster : public CRPG_Object_KnuckleDusterParent
{
	MRTC_DECLARE;
protected:
	int32		m_DamageModifier;
public:
	virtual void OnCreate();
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	MACRO_INLINEACCESS_R(DamageModifier,int32);
};
#endif

// TEST
enum
{
	MELEE_ATTACK_RIGHT,
	MELEE_ATTACK_LEFT,
	MELEE_ATTACK_MIDDLE,
	MELEE_ATTACK_SPECIAL,
	MELEE_ATTACK_NUMATTACKS,
};

#define CRPG_Object_MeleeParent CRPG_Object_Item
class CRPG_Object_Melee : public CRPG_Object_MeleeParent
{
	MRTC_DECLARE;
protected:
	int32		m_iSound_HitRight;
	int32		m_iSound_HitLeft;
	int32		m_iSound_HitMiddle;
	int32		m_iSound_HitSpecial;
	int32		m_iSound_HitSurface;
	int32		m_iSound_HitBlock;

	//int32		m_DamageModifier;

	uint8		m_AttackDamage[MELEE_ATTACK_NUMATTACKS];

	void ResolveAttackDamage(const CStr& _Key, const CStr& _Value);
public:

	virtual void OnCreate();
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	void OnIncludeClass(const CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);

	virtual int32 GetSoundIndex(int32 _Type);
	virtual int32 GetDamage(int32 _AttackIndex);

	MACRO_INLINEACCESS_REXT(SoundRight,m_iSound_HitRight,int32)
	MACRO_INLINEACCESS_REXT(SoundLeft,m_iSound_HitLeft,int32)
	MACRO_INLINEACCESS_REXT(SoundMiddle,m_iSound_HitMiddle,int32)
	MACRO_INLINEACCESS_REXT(SoundSpecial,m_iSound_HitSpecial,int32)
	MACRO_INLINEACCESS_REXT(SoundSurface,m_iSound_HitSurface,int32)
	//MACRO_INLINEACCESS_R(DamageModifier,int32);
};
#endif
