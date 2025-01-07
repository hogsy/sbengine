#ifndef WRPGItem2_h
#define WRPGItem2_h

//-------------------------------------------------------------------
#include "MRTC.h"
#include "WRPGCore.h"
//#include "WRPGChar.h"
#include "../WObj_AutoVar_AttachModel2.h"
#include "../Models/WModels_Misc.h"

//-------------------------------------------------------------------
//- CRPG_Object_Item2 -----------------------------------------------
//-------------------------------------------------------------------

class CRPG_Object_Item2 : public CRPG_Object
{
	MRTC_DECLARE;

public:
	CStr m_ItemName_Debug;	// For debugging purposes only

	static const char* ms_FlagsStr[];
	static const char* ms_RenderFlagsStr[];

	int m_WeaponGroup;
	int m_FireTimeout;
	
	int m_iItemType;
	
	int m_iSound_Cast;
	int m_iSound_End;
	int m_iSound_Equip;
	int m_iSound_Unequip;
	int m_iSound_Pickup;
	int m_iSound_Dropped;
	int m_iMuzzleAttachPoint;
	
	int m_iAnimHold;
	int m_iAnimAttack;
	int m_iAnimEquip;
	int m_iAnimUnequip;
	int m_iAnimNoAmmoHold;
	int m_iAnimNoAmmo;
	int m_iAnimReload;
	int m_iAnimReloadCrouch;

	int32 m_AnimProperty;
	CAutoVar_AttachModel2	m_Model;
	CAutoVar_AttachModel2*	m_pEquippedModel;

	int m_NumItems;
	int m_MaxItems;
	int m_PrimaryMagazineSize;

	// Offset to regular aimposition in modelspace coordinates
	CVec3Dfp32 m_AimOffset;

	// CVec3Dfp32 m_PickupOfs;
	int m_iLastActivator;
	int m_iClientHelperObject;
	int m_Flags;

	int m_PendingEquipUpdate;

	int m_lLinkedItemTypes[RPG_ITEM_MAXLINKS];
	int m_NumLinks;
	int m_iCurLink;

	int16 m_iIconSurface;
	
	int16 m_PickupMsg_iDialogue;
	int8 m_PickupMsg_iDialogueID;

	// These two have to do with the triggerzones
	// When activating the item these two will be passed
	// to the activator object in order to raise noise
	// or visiblity so they cannot be used without
	// drawing attention of the AI. Note: Then can
	// only raise the noise and visibility, not
	// lower it.
	fp32 m_NoiseLevel;
	fp32 m_Visibility;

	CStr m_lRumble_Activate[RPG_ITEM_MAXACTIVATERUMBLE];
	int m_nRumble_Activate;


	//AI type string. Interpreted by the AI weapon/itemhandlers
	CStr m_AIType;
	// The angle centered on a characters look direction within which an item is considered useful. Fractions, default is 1.0f i.e. always useful. 
	fp32 m_AIUseFOV;

	// Animation sets needed to use this item, will be used in Characters OnSpawnWorld
	CStr m_AnimSetsNeeded;

	//Security clearance required to wield weapon.
	int m_RequiredClearance;

	// Security clearance given by this item
	int m_GivenClearanceLevel;

protected:
	//
	// Save system
	//
	// The SaveMask will tell the 'this' object that it needs to be saved.
	// The mask is supposed to be used by subclasses of CRPG_Object_Item2,
	// that way we can maintain a viable class-hierarchy by letting methods
	// of a class set or clear flags in this save-mask, and still compress data.
	// That way we know that we don't accidentally save the same data twice
	// the further up the hierarchy we go. The reason to maintain a viable
	// hierarchi is to keep in sync with the classes derrived from this class.
	// The LoadMask is filled in by CRPG_Object_Item2::OnDeltaLoad, and should
	// be obeyed by derived classes.
	uint32 m_SaveMask;
	uint32 m_LoadMask;

private:
	void FindLaserBeamModelIndex();

public:

	virtual void OnCreate();
	virtual void OnIncludeClass(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);

	virtual int GetType() const				{ return TYPE_ITEM; }
	virtual CFStr GetItemName();
	CFStr GetItemDesc();
	uint32 GetRenderFlags();
	
	virtual bool Activate(const CMat43fp32 &, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual bool OnActivate(const CMat43fp32 &, CRPG_Object *_pRoot, int _iObject, int _Input) { return true; }
	virtual bool Deactivate(const CMat43fp32 &, CRPG_Object *_pRoot, int _iObject, int _Input) { return true; }
	virtual void Interrupt(const CMat43fp32 &, CRPG_Object *_pRoot, int _iObject, int _Input) {}
	virtual bool OnProcess(CRPG_Object *_pRoot, int);
	virtual bool MergeItem(int _iObject, CRPG_Object_Item2 *_pObj);
	virtual bool WorldUse(int _iObject, int _iTrigger, CRPG_Object *_pRoot) { return false; }
	virtual int OnPickup(int _iObject, CRPG_Object *_pRoot, bool _bNoSound, int _iSender);
	virtual void CycleMode(int _iObject) {}
	virtual void ForceReload(int _iObject) {}
	virtual bool CanPickup();

	// Load / save
	virtual void OnDeltaLoad(CCFile* _pFile, CRPG_Object* _pCharacter);
	virtual void OnDeltaSave(CCFile* _pFile, CRPG_Object* _pCharacter);
	bool NeedsDeltaSave(CRPG_Object* _pCharacter);

	// Called from CRPG_Object_Char, which is called from CWObject_Character, when that character dies.
	// This is needed, since OnProcess is not run when characters are dead (performance issue), so we need some way to know that the character has died.
	virtual void OnCharDeath(int _iObject, CRPG_Object* _pRoot) {}

	// NOTE: IF THESE ARE CHANGED, THEY MUST BE UPDATED FOR ALL SUBCLASSES TOO!
	// Shield, Staff, Bow, etc...
	virtual bool Equip(int _iObject, CRPG_Object *_pRoot, int _iPrecedingUnequipAnim = 0, bool _bInstant = false, bool _bHold = false);
	virtual bool Unequip(int _iObject, CRPG_Object *_pRoot, int _iSucceedingEquipAnim = 0, bool _bInstant = false, bool _bHold = false);

	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();

	virtual bool CheckAmmo(CRPG_Object *_pRoot);
	virtual bool NeedReload() { return false; }
	virtual int GetAmmo(CRPG_Object* _pRoot, int _iType = 0);
	virtual void SetAmmo(int _Num) {}
	virtual int GetActivateAnim(int _iObject);
	virtual bool DrawAmmo(int _Num, const CMat43fp32 &, CRPG_Object *_pRoot, int _iObject, bool _bApplyFireTimeout = true);
	virtual void Reload(int _iObject) {}
	virtual bool UpdateAmmoBarInfo(CRPG_Object *_pRoot, uint16 &_Surface1, uint16 &_Surface2, uint8 &_NumTotal, uint8 &_NumLoaded, uint32 &_Extra);

	// Tag animations for precache given a character object
	virtual void TagAnimationsForPrecache(CWAGI_Context* _pContext, CWAGI* _pAgi);

	virtual bool CanUse(int32 _iChar);
	
//	static bool GetItemRenderMatrix(CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel, CXR_Model *_pModel, int _iRotTrack, int _iAttachPoint, CMat43fp32 &_Mat);
//	bool GetAttachMatrix(CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel, CMat43fp32 &_Mat, int _AttachPoint, int _CharAttachpoint = -1, int _CharRottrack = -1);
	
	struct CAttachInfo
	{
		CXR_SkeletonInstance *m_pSkelInstance;
		CXR_Skeleton *m_pCharSkeleton;
		CXR_Skeleton *m_pItemSkeleton;
		int m_iCharRotTrack;
		int m_iCharAttachPoint;
		int m_iItemAttachPoint;
	};

	static CMat43fp32 GetAttachOnCharMatrix(CAttachInfo &_AttachInfo);
	static CMat43fp32 GetItemOnCharMatrix(CAttachInfo &_AttachInfo);
	static CMat43fp32 GetAttachOnItemMatrix(CAttachInfo &_AttachInfo);

	CMat43fp32 GetCurAttachOnItemMatrix(int _iObject);
	CMat43fp32 GetAttachMatrix(int _iObject, int _iCharRottrack, int _iCharAttachPoint, int _iItemAttachPoint = -1);

	virtual uint8 GetRefreshFlags(uint8 _Flags);
	virtual uint16 GetActivateAnim() { return -1; }

	class CRPG_Object_Char2* GetChar(CRPG_Object* _pRoot);

	int GetCurLinkItemType();
	//class CRPG_Object_Ammo* GetAssociatedAmmo(CRPG_Object *_pRoot);
	CRPG_Object_Item2* GetCurLinkedItem(CRPG_Object *_pRoot);
	bool HasLinkedItemType(int _ItemType);

	virtual bool IsEquippable(CRPG_Object* _pRoot);
	virtual bool IsActivatable(CRPG_Object* _pRoot,int _iObject,int _Input = 1);

	// Character wrappers
	bool ApplyWait(int _iObject, int _Ticks, fp32 _NoiseLevel, fp32 _Visibility);
	bool SetWait(int _iObject, int _Ticks);
	int SendDamage(int _iObject, const CVec3Dfp32& _Pos, int _iSender, int _Damage, int _DamageType, int _SurfaceType = 0, const CVec3Dfp32 *_pForce = NULL, const CVec3Dfp32 *_pSplatterDir = NULL, CCollisionInfo* _pCInfo = NULL, const char* _pDamageEffect = NULL, int _StunTicks = -1);

	// Skeleton animated item
	int32 PlayWeaponAnimFromType(int32 _Type, int32 _iObject, uint16 _Flags);
	int PlayWeaponAnim(int _iAnim, int _iObject, uint16 _Flags);

	int ResolveAnimHandle(const char *_pSequence);

	virtual void UpdateAmmoList(CRPG_Object *_pRoot);

	//Get the number of extra magazines to show. Return -1 if we shoudn't show extra magazines.
	virtual int ShowNumMagazines();

	// Helpers
	int GetGameTick(int _iObject);

	void Line(const CVec3Dfp32& _Pos0, const CVec3Dfp32& _Pos1, int _Col = 0xffffffff);

	//Returns the speed (in world units per server frame) of any projectiles the item can summon, or
	//_FP32_MAX if these projectiles are instantaneous. If the object does not summon projectiles, 0 is returned.
	virtual fp32 GetProjectileSpeed();

	virtual CWO_Damage * GetDamage();

	//Check if this is a melee weapon
	virtual bool IsMeleeWeapon();

	int GetRequiredClearanceLevel();
	int GetGivenClearanceLevel();
};

//-------------------------------------------------------------------

#endif /* WRPGItem_h */
