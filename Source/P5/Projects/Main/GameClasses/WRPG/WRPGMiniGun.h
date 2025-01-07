#ifndef __WRPGMINIGUN_H
#define __WRPGMINIGUN_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WRPGMiniGun
					
	Author:			Jens Andersson
					
	Copyright:		
					
	Contents:		
					
	History:		
		021125:		Created File
\*____________________________________________________________________________________________*/

#include "WRPGWeapon.h"


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CRPG_Object_MiniGun
					
	Comments:		
\*____________________________________________________________________*/

class CRPG_Object_MiniGun : public CRPG_Object_Weapon
{
	MRTC_DECLARE;

public:
	enum
	{
		MINIGUN_AGSTATE_IDLE		= 0,
		MINIGUN_AGSTATE_WINDUP		= 1,
		MINIGUN_AGSTATE_LOOPING		= 2,
		MINIGUN_AGSTATE_WINDDOWN	= 3,

		AGIMPULSE_WINDUP			= 100,
		AGIMPULSE_WINDDOWN			= 200,
	};
	CStr m_Secondary_Spawn;
	CStr m_ForcedPickup;
	int m_Secondary_WaitTicks;
	int m_Secondary_AttachPoint[3];
	int m_Secondary_RotTrack;
	int m_iCurAttachPoint;
	int m_BuildUpTick;
	int16 m_BuildUpDelayTicks;
	int16 m_Secondary_DelayTicks;
	bool m_bSkipMuzzle;
	bool m_bSecondaryFire;

	int m_iSound_Start;
	int m_iSound_Loop;
	int m_iSound_Stop;

	int m_iAnim_Start;
	int m_iAnim_Loop;
	int m_iAnim_Stop;

	//Keep track of which input is currently used to determine how to deactivate item properly.
	int8 m_CurrentInput;
	int8 m_AGState;

	virtual void OnCreate();
	void OnIncludeClass(const CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual bool DrawAmmo(int _Num, const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, bool _bApplyFireTimeout = true);
	virtual bool OnProcess(CRPG_Object *_pRoot, int _iObject);
	virtual bool Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual bool Deactivate(const CMat4Dfp32 &, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual bool CanPickup() { return false; }
	virtual bool MergeItem(int _iObject, CRPG_Object_Item *_pObj);
	virtual TPtr<class CRPG_Object_Item> GetForcedPickup();

	//Get the number of extra magazines to show. Return -1 if we don't need/use extra magazines.
	//Minigun needs ammo and can possibly have extra mags, but never show it
	virtual int ShowNumMagazines(){ return -1; };
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Class:			CRPG_Object_Drill

Comments:		AO: Inherits from minigun mainly due to animation
				and mechanics similarities, i.e. because I'm lazy :)
				The secondary fire support does mean that you could
				have a combined melee/ranged fire weapon though...
				
\*____________________________________________________________________*/

class CRPG_Object_Drill : public CRPG_Object_MiniGun
{
	MRTC_DECLARE;
protected:
	int8 m_lMelee_AttachPoints[2];

public:
	virtual void OnCreate();
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual bool IsActivatable(CRPG_Object* _pRoot,int _iObject,int _Input = 1);
	virtual bool Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual void PerformMeleeAttack(const CMat4Dfp32 &_Mat, int _iObject);
	int FindMeleeAttackSelection(const CVec3Dfp32 &_Pos, const CVec3Dfp32 &_Size, int _iObject, const int16** _ppRetList);
};

#endif
