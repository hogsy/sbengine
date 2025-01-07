#ifndef WRPGAmmo_h
#define WRPGAmmo_h

// -------------------------------------------------------------------

#include "PCH.h"

#include "WRPGItem.h"
/*
#include "WRPGChar.h"
#include "../WObj_RPG.h"
#include "../WObj_Weapons/WObj_Spells.h"
#include "../WObj_Weapons/WObj_ClientDebris.h"
#include "MFloat.h"
#include "../WObj_Char.h"
*/

class CRPG_Object_Ammo : public CRPG_Object_Item
{

	MRTC_DECLARE;

protected:

	int32 m_MagazineSize;
	int32 m_NumLoaded;
	int32 m_MaxItems;
	
	CStr m_Spawn;
	int32 m_SpawnType;

public:
	int32 m_iAmmoSurface1;
	int32 m_iAmmoSurface2;

	CRPG_Object_Ammo();
	virtual void OnIncludeClass(const CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual bool OnActivate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject);
	virtual bool MergeItem(int _iObject, CRPG_Object_Item *_pObj);
	virtual void SetMagazineSize(int _Size);
	virtual int RefillMagazine();
	virtual int AddLoaded(int _Num);
	virtual int ConsumeLoaded(int _Num);
	virtual int ComsumeAllLoaded();
	virtual void Unload();
	virtual int AddToTotal(int _Num);
	virtual bool ConsumeFromTotal(int _Num);
	virtual int GetNumTotal();
	virtual int GetMaxTotal();
	virtual int GetNumLoaded();
	virtual int GetMagazineSize();
	virtual	const char* GetSpawnClass();
	virtual int GetSpawnType();

	virtual void OnDeltaSave(CCFile* _pFile, CRPG_Object* _pChar);
	virtual void OnDeltaLoad(CCFile* _pFile, CRPG_Object* _pChar);
};
/*
// -------------------------------------------------------------------
//  CRPG_Object_BurstAmmo
// -------------------------------------------------------------------

class CRPG_Object_BurstAmmo : public CRPG_Object_Ammo
{
	MRTC_DECLARE;

public:

	class CRPG_Object_Bow* GetPrimary(CRPG_Object* _pRoot);

//	void ConsumeLoadedAmmo(int _NumLoadedArrows);
//	void OnReportLoad(int _NumLoadedArrows);

	virtual int RefillMagazine() { return 0; }; // Don't start with filling whole magazine.

	virtual bool Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);

};

// -------------------------------------------------------------------
//  CRPG_Object_SniperAmmo
// -------------------------------------------------------------------

class CRPG_Object_SniperAmmo : public CRPG_Object_Ammo
{
	MRTC_DECLARE;

public:

	CRPG_Object_Bow* GetPrimary(CRPG_Object* _pRoot);

	void SetZoom(int _iObject, int _iPhase, fp32 _ZoomSpeed, fp32 _MaxZoom);
	bool IsZoomed();

	virtual void OnCreate();
	virtual bool Equip(int _iObject, CRPG_Object* _pRoot);
	virtual bool Unequip(int _iObject, CRPG_Object *_pRoot);
	virtual bool Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual bool Deactivate(const CMat4Dfp32 &, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual bool OnProcess(CRPG_Object *_pRoot, const CMat4Dfp32 &_Mat, int _iObject);

};

// -------------------------------------------------------------------
*/
#endif /* WRPGAmmo_h */
