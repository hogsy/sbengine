#ifndef __WRPGRIFLE_H
#define __WRPGRIFLE_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WRPGRifle
					
	Author:			Jens Andersson
					
	Copyright:		
					
	Contents:		
					
	History:		
		021001:		Created File
\*____________________________________________________________________________________________*/

#include "WRPGWeapon.h"

class CRPG_Object_Rifle : public CRPG_Object_Weapon, public CRPG_Object_DualSupport
{
	MRTC_DECLARE;

public:
	int m_iSound_FlashlightOn;
	int m_iSound_FlashlightOff;
	int m_iElectroSurface;
	int m_iSound_AccessDenied;

	CPixel32 m_Flashlight_Color;
	int16 m_Flashlight_Range;

	virtual void OnCreate();
	virtual void OnIncludeClass(const CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey);
	//virtual int OnPickup(int _iObject, CRPG_Object *_pRoot, bool _bNoSound, int _iSender, bool _bNoPickupIcon = false);
	virtual bool Equip(int _iObject, CRPG_Object *_pRoot, bool _bInstant = false, bool _bHold = false);
	virtual bool Unequip(int _iObject, CRPG_Object *_pRoot, bool _bInstant = false, bool _bHold = false);
	virtual void CycleMode(int _iObject);
	virtual bool DrawAmmo(int _Num, const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, bool _bApplyFireTimeout = true);
	virtual bool OnProcess(CRPG_Object *_pRoot, int _iObject);
	virtual bool Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);

	virtual bool CanUse(int32 _iChar);
	virtual void OnDeltaLoad(CCFile* _pFile, CRPG_Object* _pCharacter);
	virtual void OnDeltaSave(CCFile* _pFile, CRPG_Object* _pCharacter);
	virtual void SetCloneIdentifier(int32 _CloneIdentifier){ m_PrevCloneIdentifier = _CloneIdentifier; };
};

// Sucks darkness juice instead of ammo
class CRPG_Object_Ancient : public CRPG_Object_Rifle
{
	MRTC_DECLARE;

public:
	int m_DarknessDrainAmount;

	virtual void OnCreate();
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey);
	virtual void OnFinishEvalKeys();
	//virtual int OnPickup(int _iObject, CRPG_Object *_pRoot, bool _bNoSound, int _iSender, bool _bNoPickupIcon = false);
	virtual bool DrawAmmo(int _Num, const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, bool _bApplyFireTimeout = true);
	virtual int GetAmmoDraw() { return m_DarknessDrainAmount; }
	virtual bool Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);

	virtual bool IsActivatable(CRPG_Object* _pRoot,int _iObject,int _Input = 1);
};

/*enum
{
	RIFLESPAWN_FULLMETALJACKET = 0,
	RIFLESPAWN_TRANQUILLIZER,
	RIFLESPAWN_APPENDAGE,
	RIFLESPAWN_NUMBEROF,

	// Load/save mask shift indexes
	LOADSAVEMASK_RIFLE_INDEX_DUMMY = LOADSAVEMASK_INDEX_DERIVEDBASE,
	LOADSAVEMASK_RIFLE_INDEX_DERIVEDBASE,

	// Load/save masks
	LOADSAVEMASK_RIFLE_DUMMY = (1 << LOADSAVEMASK_RIFLE_INDEX_DUMMY),
};

class CWO_Rifle_Functionality
{
public:
	CWO_Rifle_Functionality();

	// Basic settings
	CStr m_SummonClass;
	int m_ReloadTime;
	int m_AmmoLoad;

	// These attachmodels are sent to CRPG_Object_Rifle or parent when this ammo-type has been selected
	int m_iAttachModel[ATTACHMODEL_NUMMODELS];
	int m_AttachModelPoint[ATTACHMODEL_NUMMODELS];

	// Sound effects to play when ammo-type has been selected or fired
	int m_iSound_Select;
	int m_iSound_Activate;

	// For AI to detect when ammo-type has been used
	int m_NoiseLevel;
	int m_Visibility;
};

class CRPG_Object_Rifle : public CRPG_Object_Weapon
{
	MRTC_DECLARE;

private:
	void ParseRifleChunk(const CRegistry* _pReg, CWO_Rifle_Functionality* _pRifleChunk);
	void IncludeRifleChunk(CRegistry* _pParent, CMapData* _pMapData);
	int GetIndexFromAttachmodelKey(const CRegistry* _pKey);
	int GetAttachPointFromAttachmodelKey(const CRegistry* _pKey);
	CStr GetModelFromAttachmodelKey(const CRegistry* _pKey);
	void CycleCurrentAmmoType(int _iOwner);

protected:
	int m_iAnimMeleeAttack;
	int m_PendingHit;
	fp32 m_Force;
	int m_PreviousSecondaryActivate;
	int m_CurrentSelectedAmmo;
	bool m_bSecondaryBounceback;

	CWO_Rifle_Functionality m_Spawn[RIFLESPAWN_NUMBEROF];	// Full metal jacket(0), tranquillizer(1) and appendage(2)

	int GetCurrentAmmoType(){return m_CurrentSelectedAmmo;};

public:
	virtual void OnCreate();
	virtual void OnIncludeClass(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual bool Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual bool Deactivate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual bool Equip(int _iObject, CRPG_Object *_pRoot, int _iPrecedingUnequipAnim = 0, bool _bInstant = false, bool _bHold = false);
	virtual bool MergeItem(CRPG_Object_Item *_pObj);
	virtual bool IsActivatable(CRPG_Object* _pRoot);
	virtual void RefreshAmmo(int _iObject);
	virtual void SelectAmmoType(int _iOwner, int _NewAmmoType);
	virtual bool OnProcess(CRPG_Object *_pRoot, const CMat4Dfp32 &_Mat, int _iObject);
	virtual int GetAmmo(CRPG_Object* _pRoot);
	virtual void OnDeltaLoad(CCFile* _pFile, CRPG_Object* _pCharacter);
	virtual void OnDeltaSave(CCFile* _pFile, CRPG_Object* _pCharacter);
	virtual void CycleMode(int _iObject);
	virtual int OnPickup(int _iObject, CRPG_Object *_pRoot, bool _bNoSound, int _iSender);
};*/

#endif
