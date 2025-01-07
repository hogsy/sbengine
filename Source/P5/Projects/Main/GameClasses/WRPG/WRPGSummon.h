#ifndef __WRPGSUMMON_H
#define __WRPGSUMMON_H

#include "WRPGItem.h"
#include "WRPGInitParams.h"

enum
{
	// Load/save mask shift indexes
	LOADSAVEMASK_SUMMON_INDEX_GUID = LOADSAVEMASK_INDEX_DERIVEDBASE,
	LOADSAVEMASK_SUMMON_INDEX_DERIVEDBASE,

	// Load/save masks
	LOADSAVEMASK_SUMMON_GUID = (1 << LOADSAVEMASK_SUMMON_INDEX_GUID),
	LOADSAVEMASK_SUMMON_DERIVEDBASE = (1 << LOADSAVEMASK_SUMMON_INDEX_DERIVEDBASE),
};

//-------------------------------------------------------------------
//- CRPG_Object_Summon ----------------------------------------------
//-------------------------------------------------------------------

class CRPG_Object_Summon : public CRPG_Object_Item
{
	MRTC_DECLARE;

protected:
	int8 m_MeleeDelay;				// Read from register. Delay from user trigger to actual attack being performed
	int8 m_PendingMeleeAttack;		// How many ticks are there until next delayed melee-attack will be performed?
	int8 m_MeleeDamage_Frontal;		// How much damage to give when hit from framifrån, typ
	int8 m_MeleeDamage_FromBehind;	// How much damage to give when hitting from behind
	fp32 m_MeleeForce;				// Read from register. Multiplied with user look-vector to get force vector to knock target with
	uint8 m_MeleeFwdOffset;			// Read from register. The distance the melee attack selection is offset forwards.

	//These three values are used when you want to customize the melee attack selection size
	uint8 m_MeleeHalfWidth;		
	uint8 m_MeleeHalfDepth;
	uint8 m_MeleeMinHeight;

	int32 m_iMeleeHitSound;			// Sound played when melee strike hits
	int32 m_MeleeSoundPeriod;		// Min time in ticks between sounds
#define SUMMON_MELEE_HITSOUND_PERIOD	10
	int32 m_MeleeSoundTick;			// At what gametick did the melee sound start

	int16 m_LinkedFireItem;			// Quick activate this item when we activate
	bool m_bLockedLinkedFire;		// Is linked fire allowed this tick? (Use to avoid recursive linked fire)

public:
	CStr m_Spawn;
	CStr m_SpawnEffect;
	CStr m_GiveOnPickup;
	uint32 m_SpawnedObjectGUID;

	fp32 m_StartOffset;	

	int m_SummonDelayTicks;
	int m_CurSummonDelayTicks;

	virtual void OnCreate();
	virtual void OnIncludeClass(const CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual bool OnProcess(CRPG_Object *_pRoot, int _iObject);
	virtual aint OnMessage(CRPG_Object* _pParent, const CWObject_Message& _Msg, int _iOwner);

	virtual bool IsActivatable(CRPG_Object* _pRoot,int _iObject,int _Input = 1);

	virtual int Summon(const char* _pClass, int _iObject, const CMat4Dfp32& _Mat, CRPG_InitParams* _pInitParams, int _NumDrawAmmo = 1, bool _bApplyWait = true, bool _bFinalPos = false);
	int Summon(const CMat4Dfp32& _Mat, CRPG_Object *_pRoot, int _iObject, int _NumDrawAmmo = 1, bool _bApplyWait = true, int m_DelayTicks = 0);

	virtual bool DrawAmmo(int _Num, const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, bool _bApplyFireTimeout = true);

	virtual bool IsUnequip() { return (m_bUnequipWhenEmpty && m_AmmoLoad <= 0); }

	// Note: Activate/OnActivate doesn't go further up (down?) the class hierarchy!
	// OnActivate is only called when Activate has actually spawned it's summon and
	// not when hitting a reload mark or whatever. Because this is hacked into the
	// class hierarchy att this level you will have to call OnActivate yourself if
	// you overload the Activate method and it was executed sucecssfully. - Rune
	virtual bool Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual bool OnActivate(const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, int _Input);
	virtual bool Deactivate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);

	virtual void SetPendingMeleeAttackTicks(int _Ticks);
	fp32 GetMeleeAttackAngle(const CMat4Dfp32& _AttackPos, const CMat4Dfp32& _VictimPos);
	virtual void PerformMeleeAttack(const CMat4Dfp32 &_Mat, int _iObject);
	int FindMeleeAttackSelection(const CVec3Dfp32 &_Pos, int _iObject, const int16** _ppRetList);

	//Variant of the above, which uses template values to define attack selection box, instead of hardcoded ones.
	int FindMeleeAttackSelection(const CMat4Dfp32& _Mat, int _iObject, const int16** _ppRetList);

	// Load/Save
	virtual void OnDeltaLoad(CCFile* _pFile, CRPG_Object* _pCharacter);
	virtual void OnDeltaSave(CCFile* _pFile, CRPG_Object* _pCharacter);

	virtual bool Equip(int _iObject, CRPG_Object *_pRoot, bool _bInstant = false, bool _bHold = false);
	virtual bool Unequip(int _iObject, CRPG_Object *_pRoot, bool _bInstant = false, bool _bHold = false);

	virtual bool MergeItem(int _iObject, CRPG_Object_Item *_pObj);
	virtual int OnPickup(int _iObject, CRPG_Object *_pRoot, bool _bNoSound, int _iSender, bool _bNoPickupIcon = false);
	virtual int GetAmmo(CRPG_Object* _pRoot, int _iType = 0);
	virtual int GetMaxAmmo() { return m_MaxAmmo; }
};

/*
//-------------------------------------------------------------------

#define CRPG_Object_ThresholdSummon_Parent CRPG_Object_Summon
class CRPG_Object_ThresholdSummon : public CRPG_Object_ThresholdSummon_Parent
{
	MRTC_DECLARE;

private:

	static const char* ms_lpAnimStr[];

	enum TSANIM
	{
		TSANIM_NULL			= 0xFF,
		TSANIM_CHARGE		= 0x00,
		TSANIM_LONGTHROW	= 0x01,
		TSANIM_SHORTTHROW	= 0x02,
		TSANIM_RELOAD		= 0x03,
		NUM_TSANIMS			= 0x04,
	};

	enum TSSTATE
	{
		TSSTATE_NULL,
		TSSTATE_WAITBRANCH,
		TSSTATE_CHARGE,
		TSSTATE_LONGTHROW,
		TSSTATE_SHORTTHROW,
		TSSTATE_RELOAD,
	};


	int m_iState;
	
	fp32 m_VelocityFraction;

	int m_QuickSummonDelayTicks;

	fp32	m_LongThrowThreshold;

	int	m_iSound_Charge;

	int	m_iAnim_Charge;
	int	m_iAnim_Throw;
	int	m_iAnim_ShortThrow;

	fp32 m_AnimTime;
	fp32	m_AnimDuration;

	int	m_CurAnim;

	int m_iAnim[NUM_TSANIMS];

public:

	void PlayAnim(TSANIM _Anim, int _iObject);

	virtual void OnCreate();
	virtual void OnIncludeClass(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	virtual bool OnProcess(CRPG_Object *_pRoot, int _iObject);
	virtual aint OnMessage(CRPG_Object* _pParent, const CWObject_Message& _Msg, int _iOwner);
	virtual bool Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual bool Deactivate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
};

//-------------------------------------------------------------------
//- CRPG_Object_ReleaseSummon ---------------------------------------
//-------------------------------------------------------------------

#define CRPG_Object_ReleaseSummon_Parent CRPG_Object_Summon
class CRPG_Object_ReleaseSummon : public CRPG_Object_ReleaseSummon_Parent
{
	MRTC_DECLARE;

private:

	static const char* ms_lpAnimStr[];

	enum RSANIM
	{
		RSANIM_NULL			= 0xFF,
		RSANIM_CHARGE		= 0x00,
		RSANIM_LONGTHROW	= 0x01,
		RSANIM_SHORTTHROW	= 0x02,
		RSANIM_RELOAD		= 0x03,
		NUM_RSANIMS			= 0x04,
	};

	enum RSSTATE
	{
		RSSTATE_NULL,
		RSSTATE_WAITBRANCH,
		RSSTATE_CHARGE,
		RSSTATE_LONGTHROW,
		RSSTATE_SHORTTHROW,
		RSSTATE_RELOAD,
	};


	int m_iState;
	
	fp32 m_Charge_CurTime;
	fp32	m_Charge_StartTime;
	fp32	m_Charge_HoldTime;
	fp32	m_Charge_EndTime;

	int m_QuickSummonDelayTicks;

	fp32	m_LongThrowThreshold;

	int	m_iSound_Charge;

	int	m_iAnim_Charge;
	int	m_iAnim_Throw;
	int	m_iAnim_ShortThrow;

	fp32 m_AnimTime;
	fp32	m_AnimDuration;

	int	m_CurAnim;

	int m_iAnim[NUM_RSANIMS];

public:

	void PlayAnim(RSANIM _Anim, int _iObject);

	virtual void OnCreate();
	virtual void OnIncludeClass(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	virtual bool OnProcess(CRPG_Object *_pRoot, int _iObject);
	virtual aint OnMessage(CRPG_Object* _pParent, const CWObject_Message& _Msg, int _iOwner);
	virtual bool Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual bool Deactivate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
};

//-------------------------------------------------------------------
//- CRPG_Object_TargetSummon ----------------------------------------
//-------------------------------------------------------------------

class CRPG_Object_TargetSummon : public CRPG_Object_Summon
{
	MRTC_DECLARE;

public:

	int m_Range;

	virtual void OnCreate();
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual bool Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
};


//-------------------------------------------------------------------
//- CRPG_Object_FloorSummon -----------------------------------------
//-------------------------------------------------------------------

class CRPG_Object_FloorSummon : public CRPG_Object_TargetSummon
{
	MRTC_DECLARE;

public:
	void OnCreate();
	virtual bool Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
};

//-------------------------------------------------------------------

//-------------------------------------------------------------------
//- CRPG_Object_MultiSpreadSummon -----------------------------------
//-------------------------------------------------------------------

class CRPG_Object_MultiSpreadSummon : public CRPG_Object_Summon
{
	MRTC_DECLARE;

public:
	
	fp32 m_HeadingSpread;
	fp32 m_PitchSpread;
	int m_MinProjectiles;
	int m_MaxProjectiles;

	virtual void OnCreate();
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	virtual int Summon(const char* _pClass, const CMat4Dfp32& _Mat, CRPG_InitParams* _pInitParams, int _NumDrawAmmo = 1, bool _bApplyWait = true);
};
*/

class CRPG_Object_Throw : public CRPG_Object_Summon
{
	MRTC_DECLARE;
protected:
	uint8 m_HideModelDelay;
	uint8 m_ShowModelDelay;
	uint8 m_ActiveTimer;

	enum {
		TIMER_MAX = 254,
		TIMER_INVALID = 255,
	};

public:
	virtual void OnCreate();
	virtual bool Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual bool OnProcess(CRPG_Object *_pRoot, int _iObject);
	virtual bool Unequip(int _iObject, CRPG_Object *_pRoot, bool _bInstant = false, bool _bHold = false);

	//Get the number of extra magazines to show. Return -1 if we don't need/use extra magazines.
	virtual int ShowNumMagazines();
};


#endif /* WRPGSummon_h */
