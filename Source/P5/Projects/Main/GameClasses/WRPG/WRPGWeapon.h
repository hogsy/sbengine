#ifndef __WRPGWEAPON_H
#define __WRPGWEAPON_H

#include "WRPGSummon.h"

//-------------------------------------------------------------------
//- CRPG_Object_Weapon ----------------------------------------------
//-------------------------------------------------------------------
class CRPG_Object_Weapon : public CRPG_Object_Summon
{
	MRTC_DECLARE;

public:

	CWO_Damage m_Damage;
	CStr m_DamageEffect;
	int m_MuzzleFlameOffTick;
	uint32 m_MuzzleLightColor;
	uint16 m_MuzzleLightRange;
	bool m_bLightningLight;
	fp32 m_Range;
	fp32 m_PlayerScatter;
	
	int32 m_LastShotTick;
	int32 m_ScatterDecreaseTime;
	fp32 m_ScatterIncreaseAmount;
	fp32 m_LastScatter;
	fp32 m_ScatterFrequencyPenalty;

	uint8 m_bTriggerPressed : 1;
	uint8 m_bForceReload : 1;
	uint8 m_bDefaultRicochet : 1;
	uint8 m_bNoFlashLight : 1;
	int m_nMagazines;
	int m_MaxMagazines;
	
	int m_iSound_Reload;
	int m_iSound_OutOfAmmo;
	int m_iSound_Merge;

	virtual void OnCreate();
	virtual void OnIncludeClass(const CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual CWO_Damage * GetDamage();
	virtual int Summon(const char* _pClass, int _iObject, const CMat4Dfp32& _Mat, CRPG_InitParams* _pInitParams, int _NumDrawAmmo = 1, bool _bApplyWait = true, bool _bFinalPos = false);

	virtual bool NeedReload(int _iObject, bool _bCanReload);
	virtual void Reload(int _iObject);
	virtual bool MergeItem(int _iObject, CRPG_Object_Item *_pObj);
	virtual void ForceReload(int _iObject);
	virtual bool CanPickup();
	virtual bool Equip(int _iObject, CRPG_Object *_pRoot, bool _bInstant = false, bool _bHold = false);
	virtual int GetAmmo(CRPG_Object* _pRoot, int _iType = 0);
	virtual int GetMagazines(void);
	virtual void SetAmmo(int _Num);
	virtual void AddMagazine(int _Num);

	virtual void OnDeltaLoad(CCFile* _pFile, CRPG_Object* _pCharacter);
	virtual void OnDeltaSave(CCFile* _pFile, CRPG_Object* _pCharacter);

	virtual bool Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual bool Deactivate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual bool IsActivatable(CRPG_Object* _pRoot,int _iObject,int _Input = 1);

	virtual bool OnProcess(CRPG_Object* _pRoot, int _iObject);

	void FireMuzzle(int _iObject);
	void RefreshMuzzle(int _iObject);

	//Get the number of extra magazines to show. Return -1 if we don't need/use extra magazines.
	virtual int ShowNumMagazines();

	virtual bool DrawAmmo(int _Num, const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, bool _bApplyFireTimeout = true);
	
	virtual void GetMuzzleProperties(uint32& _MuzzleLightColor, uint32& _MuzzleLightRange);
	virtual bool HasLightningLight();
};

class CRPG_Object_DualSupport
{
public:
	CRPG_Object_DualSupport();
	int32 m_PrevCloneIdentifier;
	void LoadDualSupport(CCFile* _pFile, CRPG_Object* _pCharacter);
	void SaveDualSupport(CCFile* _pFile, CRPG_Object* _pCharacter);
	void EquipDualSupport(int _iObject, CRPG_Object *_pRoot, bool _bInstant, CRPG_Object_Weapon* _pHost);
	void UnequipDualSupport(int _iObject, CRPG_Object *_pRoot, bool _bInstant, CRPG_Object_Weapon* _pHost);
};

//-------------------------------------------------------------------

#endif /* WRPGWeapon_h */
