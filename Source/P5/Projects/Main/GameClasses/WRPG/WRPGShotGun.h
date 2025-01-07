#ifndef __WRPGSHOTGUN_H
#define __WRPGSHOTGUN_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WRPGShotGun
					
	Author:			Jens Andersson
					
	Copyright:		
					
	Contents:		
					
	History:		
		021125:		Created File
\*____________________________________________________________________________________________*/

#include "WRPGWeapon.h"


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CRPG_Object_ShotGun
					
	Comments:		
\*____________________________________________________________________*/

class CRPG_Object_ShotGun : public CRPG_Object_Weapon
{
	MRTC_DECLARE;

public:
	int m_iSound_FlashlightOn;
	int m_iSound_FlashlightOff;
	
	int32 m_Flashlight_Color;
	int16 m_Flashlight_Range;
	bool m_bTriggerPressed;

	virtual void OnCreate();
	virtual void OnIncludeClass(const CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey);
	virtual bool Equip(int _iObject, CRPG_Object *_pRoot, bool _bInstant = false, bool _bHold = false);
	virtual void CycleMode(int _iObject);
	virtual bool DrawAmmo(int _Num, const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, bool _bApplyFireTimeout = true);
	virtual bool OnProcess(CRPG_Object *_pRoot, int _iObject);
	virtual void OnDeltaLoad(CCFile* _pFile, CRPG_Object* _pCharacter);
	virtual void OnDeltaSave(CCFile* _pFile, CRPG_Object* _pCharacter);
};

#endif
