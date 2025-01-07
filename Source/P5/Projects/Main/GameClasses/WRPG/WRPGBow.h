#ifndef WRPGBow_h
#define WRPGBow_h

// -------------------------------------------------------------------

/*

	Burst
		Reload
		Raise Charge Lower
		Release Lower

	Sniper
		Reload
		Raise Charge

		ChargeLoop

		Release Lower/Reload
		Release Lower

*/

// -------------------------------------------------------------------

#include "PCH.h"

#include "WRPGCore.h"
#include "WRPGItem.h"
#include "WRPGSummon.h"
#include "WRPGAmmo.h"

/*
#include "WRPGChar.h"
#include "../WObj_RPG.h"
#include "../WObj_Weapons/WObj_Spells.h"
#include "../WObj_Weapons/WObj_ClientDebris.h"
#include "MFloat.h"
#include "../WObj_Char.h"
*/

// -------------------------------------------------------------------

enum SNIPERANIM
{
	SNIPERANIM_NULL						= 0xFF,
	SNIPERANIM_RELOAD					= 0x00,
	SNIPERANIM_RAISECHARGE				= 0x01,
	SNIPERANIM_CHARGELOOP				= 0x02,
	SNIPERANIM_RELEASELOWERRELOAD		= 0x03,
	SNIPERANIM_RELEASELOWER				= 0x04,
	SNIPERANIM_FASTSHOTRELOAD			= 0x05,
	SNIPERANIM_FASTSHOT					= 0x06,
	NUM_SNIPERANIMS						= 0x07,
};

enum SNIPERSOUND
{
	SNIPERSOUND_NULL					= 0xFF,
	SNIPERSOUND_RELOAD					= 0x00,
	SNIPERSOUND_CHARGE					= 0x01,
	SNIPERSOUND_RELEASE					= 0x02,
	NUM_SNIPERSOUNDS					= 0x03,
};

enum
{
	ANIMMODE_STAND = 0,
	ANIMMODE_CROUCH = 1,
	NUM_ANIMMODES = 2,
};

// -------------------------------------------------------------------

class ConflictVariables
{

	public:

		CFStr m_KeyPrefix;
		int m_FireTimeout;
		int m_NumItems;
		int m_Flags;
		int	m_iSound_Equip;
		int	m_iSound_Unequip;
		int m_iAnimEquip;

	public:

		CRPG_Object_Summon* m_Base;

		ConflictVariables()
		{
			m_FireTimeout = 0;
			m_NumItems = 0;
			m_iSound_Equip = -1;
			m_iSound_Unequip = -1;
			m_iAnimEquip = -1;
			m_Flags = 0;
		}

		void UpdateState(CRPG_Object_Summon* _pSummon)
		{
			_pSummon->m_FireTimeout = m_FireTimeout;
			_pSummon->m_NumItems = m_NumItems;
			_pSummon->m_Flags &= RPG_ITEM_FLAGS_EQUIPPED | RPG_ITEM_FLAGS_ACTIVATED/* | RPG_ITEM_FLAGS_EVOLVECLIENTTRAIL*/;
			_pSummon->m_Flags |= m_Flags;
			_pSummon->m_iSound_Equip = m_iSound_Equip;
			_pSummon->m_iSound_Unequip = m_iSound_Unequip;
			_pSummon->m_iAnimEquip = m_iAnimEquip;
		}

		virtual void OnIncludeClass(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
		{
			m_Base->IncludeAnimFromKey(m_KeyPrefix + "ANIM_EQUIP", _pReg, _pMapData);
		}

		virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
		{
			if (_pKey->GetThisName() == (m_KeyPrefix + "RELOADTIME"))
			{
				if(_pKey->GetThisValue().CompareNoCase("anim") == 0)
					m_FireTimeout = -1;
				else
					m_FireTimeout = _pKey->GetThisValuef() * SERVER_TICKSPERSECOND;
			}

			else if (_pKey->GetThisName() == (m_KeyPrefix + "NUMITEMS"))
				m_NumItems = _pKey->GetThisValuei();

			else if (_pKey->GetThisName() == (m_KeyPrefix + "TEST_NUMITEMS"))
				m_NumItems = _pKey->GetThisValuei();

			else if (_pKey->GetThisName() == (m_KeyPrefix + "FLAGS"))
				m_Flags = _pKey->GetThisValue().TranslateFlags(CRPG_Object_Item::ms_FlagsStr);

			else if (_pKey->GetThisName() == (m_KeyPrefix + "ANIM_EQUIP"))
				
				m_iAnimEquip = m_Base->ResolveAnimHandle(_pKey->GetThisValue());

			else if (_pKey->GetThisName() == (m_KeyPrefix + "SOUND_EQUIP"))
				m_iSound_Equip = m_Base->m_pWServer->GetMapData()->GetResourceIndex_Sound(_pKey->GetThisValue());
			else if (_pKey->GetThisName() == (m_KeyPrefix + "SOUND_UNEQUIP"))
				m_iSound_Unequip = m_Base->m_pWServer->GetMapData()->GetResourceIndex_Sound(_pKey->GetThisValue());

			else
				return false;
		
			return true;
		}
		
};

// -------------------------------------------------------------------

class CBurstBow : public ConflictVariables
{
private:

	CWO_Damage m_Damage;

	fp32 m_XOffset, m_YOffset;
	fp32 m_XSpread, m_YSpread;
	fp32 m_SpreadUnity;
	fp32 m_ColumnSpread;
	fp32 m_VelocitySpread;

	int m_iCurAnim;
	int m_ElapsedAnimTicks;
	int m_AnimDurationTicks;
	
	int m_iReloadingAmmoItemType;

	int m_iDefaultAnim;
	int m_iChargeAnim[NUM_ANIMMODES];
	int m_iReleaseAnim[NUM_ANIMMODES];
	int m_iReloadAnim[NUM_ANIMMODES];

	fp32 m_SubChargeAnimStart;
	fp32 m_SubChargeAnimHold;
	fp32 m_SubChargeAnimEnd;

	fp32 m_QuickShot_ReleaseTime;
	fp32 m_QuickShot_VelocityFraction;
	bool m_bQuickShot;

	int m_iChargeSound;
	int m_iReleaseSound;
	int m_iReloadSound;
	bool m_ChargeSoundActive;

private:

	virtual int GetAnimMode(int _iObject);
	virtual void PlaySound(int _iObject, int _iSound);
	virtual void PlayAnim(int _iObject, int _iAnim, int _iSound);
	virtual bool IsAnimPlaying(int* _iAnim);
	virtual int SummonWithoutDrawAmmo(const char *_pClass, int32 _Param0, int32 _Param1, const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, CCollisionInfo *_pInfo);
	virtual void FireArrows(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject);
	virtual CRPG_Object_BurstAmmo* GetBurstAmmo(CRPG_Object *_pRoot);

public:

	CBurstBow();
	virtual void OnCreate();
	virtual void OnIncludeClass(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual aint OnMessage(CRPG_Object* _pParent, const CWObject_Message& _Msg, int _iOwner);
	virtual bool Equip(int _iObject, CRPG_Object *_pRoot);
	virtual bool Unequip(int _iObject, CRPG_Object *_pRoot);
	virtual bool EquipSecondary(int _iObject, CRPG_Object *_pRoot);
	virtual bool UnequipSecondary(int _iObject, CRPG_Object *_pRoot);
	virtual bool Activate(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual bool Deactivate(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual bool ActivateSecondary(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject);
	virtual bool DeactivateSecondary(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject);
	virtual bool OnProcess(CRPG_Object *_pRoot, const CMat43fp32 &_Mat, int _iObject);

};

// -------------------------------------------------------------------

class CSniperBow : public ConflictVariables
{

private:

	int m_Range;
	fp32 m_MoveBack;

	CWO_Damage m_Damage;

	fp32	m_Zoom1;
	fp32	m_Zoom2;
	int m_iPhase;
	bool m_bSecondaryPressed;

	SNIPERANIM m_CurAnim;
	SNIPERSOUND m_CurSound;

	bool m_bFastShot;

	int m_iAnim[NUM_SNIPERANIMS][NUM_ANIMMODES];
	int m_iSound[NUM_SNIPERSOUNDS];

	fp32 m_AnimTime; // Time elapsed since current animation started playing.
	fp32 m_AnimDuration; // Duration of current animation (until it ends or starts looping).

private:

	virtual bool IsZoomed();
	virtual void SetZoom(int _iObject, int _iPhase, fp32 _ZoomSpeed, fp32 _MaxZoom);

	virtual int GetAnimMode(int _iObject);
	virtual void PlaySound(int _iObject, SNIPERSOUND _iSound);
	virtual void PlayAnim(int _iObject, SNIPERANIM _iAnim);
	virtual int SummonWithoutDrawAmmo(const char *_pClass, int32 _Param0, int32 _Param1, const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, CCollisionInfo *_pInfo);
	virtual void FireArrow(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject);
	virtual CRPG_Object_SniperAmmo* GetSniperAmmo(CRPG_Object *_pRoot);

public:

	CSniperBow();
	virtual void OnCreate();
	virtual void OnIncludeClass(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual	aint OnMessage(CRPG_Object* _pParent, const CWObject_Message& _Msg, int _iOwner);
	virtual bool Equip(int _iObject, CRPG_Object *_pRoot);
	virtual bool Unequip(int _iObject, CRPG_Object *_pRoot);
	virtual bool EquipSecondary(int _iObject, CRPG_Object *_pRoot);
	virtual bool UnequipSecondary(int _iObject, CRPG_Object *_pRoot);
	virtual bool Activate(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual bool ActivateSecondary(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject);
	virtual bool DeactivateSecondary(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject);
	virtual bool OnProcess(CRPG_Object *_pRoot, const CMat43fp32 &_Mat, int _iObject);
};

// -------------------------------------------------------------------

enum EvalKeyMode
{
	EVALKEYMODE_BOW,
	EVALKEYMODE_BURSTBOW,
	EVALKEYMODE_SNIPERBOW,
};

enum BOWMODE
{
	BOWMODE_NONE,
	BOWMODE_BURST,
	BOWMODE_SNIPER,
};

// -------------------------------------------------------------------

class CRPG_Object_Bow : public CRPG_Object_Summon
{

	MRTC_DECLARE;

private:

	EvalKeyMode			m_EvalKeyMode;
	BOWMODE				m_Mode;

	CBurstBow			m_BurstBow;
	CSniperBow			m_SniperBow;

private:

	BOWMODE GetMode(int _iObject, CRPG_Object *_pRoot, bool _bForceUsableItem);

public:

	virtual void OnCreate();
	virtual aint OnMessage(CRPG_Object* _pParent, const CWObject_Message& _Msg, int _iOwner);
	virtual void OnIncludeClass(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual bool Equip(int _iObject, CRPG_Object *_pRoot, int _iPrecedingUnequipAnim = 0, bool _bInstant = false, bool _bHold = false);
	virtual bool Unequip(int _iObject, CRPG_Object *_pRoot, int _iSucceedingEquipAnim = 0, bool _bInstant = false, bool _bHold = false);
	virtual bool EquipSecondary(int _iObject, CRPG_Object *_pRoot);
	virtual bool UnequipSecondary(int _iObject, CRPG_Object *_pRoot);
	virtual bool Activate(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual bool Deactivate(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual bool ActivateSecondary(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject);
	virtual bool DeactivateSecondary(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject);
	virtual bool OnProcess(CRPG_Object *_pRoot, const CMat43fp32 &_Mat, int _iObject);

};

// -------------------------------------------------------------------

#endif