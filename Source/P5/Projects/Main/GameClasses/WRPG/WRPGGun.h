#ifndef __WRPGGUN_H
#define __WRPGGUN_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WRPGGun
					
	Author:			Jens Andersson
					
	Copyright:		
					
	Contents:		
					
	History:		
		021125:		Created File
\*____________________________________________________________________________________________*/

#include "WRPGWeapon.h"


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CRPG_Object_Gun
					
	Comments:		
\*____________________________________________________________________*/

class CRPG_Object_Gun : public CRPG_Object_Weapon, public CRPG_Object_DualSupport
{
	MRTC_DECLARE;

public:
	bool m_bTriggerPressed;

	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnCreate();
	virtual bool DrawAmmo(int _Num, const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, bool _bApplyFireTimeout = true);
	virtual bool OnProcess(CRPG_Object *_pRoot, int _iObject);
	int ShowNumMagazines();
	virtual bool CanPickup();
	virtual bool Equip(int _iObject, CRPG_Object *_pRoot, bool _bInstant = false, bool _bHold = false);
	virtual bool Unequip(int _iObject, CRPG_Object *_pRoot, bool _bInstant = false, bool _bHold = false);
	virtual void OnDeltaLoad(CCFile* _pFile, CRPG_Object* _pCharacter);
	virtual void OnDeltaSave(CCFile* _pFile, CRPG_Object* _pCharacter);
	virtual void SetCloneIdentifier(int32 _CloneIdentifier){ m_PrevCloneIdentifier = _CloneIdentifier; };

};

#endif
