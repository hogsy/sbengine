#ifndef WRPGItem_h
#define WRPGItem_h

//-------------------------------------------------------------------
#include "MRTC.h"
#include "WRPGCore.h"
//#include "WRPGChar.h"
#include "../WObj_AutoVar_AttachModel.h"
#include "../Models/WModels_Misc.h"

//-------------------------------------------------------------------
//- CRPG_Object_Item ------------------------------------------------
//-------------------------------------------------------------------

enum
{
	// Note, only the first 8 flags are replicated to the client.
	RPG_ITEM_FLAGS_WALLCOLLISION		= 1 << 0,
	RPG_ITEM_FLAGS_NORENDERMODEL		= 1 << 1,
	RPG_ITEM_FLAGS_LASERBEAM			= 1 << 2,
	RPG_ITEM_FLAGS_EQUIPPED				= 1 << 3,
	RPG_ITEM_FLAGS_RPGMODELFLAGS		= 1 << 4,
	RPG_ITEM_FLAGS_AIMING				= 1 << 5,
	RPG_ITEM_FLAGS_FLASHLIGHT			= 1 << 6,
	RPG_ITEM_FLAGS_FORCEWALK			= 1 << 7,

	RPG_ITEM_FLAGS_UNSELECTABLE			= 1 << 8,
	RPG_ITEM_FLAGS_AUTOAIM				= 1 << 9,
	RPG_ITEM_FLAGS_AUTOEQUIP			= 1 << 10,	//If set, this item should be equipped when picked up
	RPG_ITEM_FLAGS_AUTOACTIVATE			= 1 << 11,
	RPG_ITEM_FLAGS_USELESS				= 1 << 12,
	RPG_ITEM_FLAGS_ACTIVATED			= 1 << 13,
	RPG_ITEM_FLAGS_NEEDAMMO				= 1 << 14,
	RPG_ITEM_FLAGS_NEEDLINK				= 1 << 15,
	RPG_ITEM_FLAGS_NEEDEQUIPPEDLINK		= 1 << 16,
	RPG_ITEM_FLAGS_KEEPAFTERMISSION		= 1 << 17,
	RPG_ITEM_FLAGS_DROPONUNEQUIP		= 1 << 18,
	RPG_ITEM_FLAGS_REMOVED				= 1 << 19,
	RPG_ITEM_FLAGS_RENDERQUANTITY		= 1 << 20,
	RPG_ITEM_FLAGS_QUESTITEM			= 1 << 21,
	RPG_ITEM_FLAGS_FORCESELECTED		= 1 << 22,
	RPG_ITEM_FLAGS_TWOHANDED			= 1 << 23,
	RPG_ITEM_FLAGS_CONSUME				= 1 << 24, // This should really replace as inverse of NOCONSUME.
	RPG_ITEM_FLAGS_USEABLE				= 1 << 25,
	RPG_ITEM_FLAGS_NOPICKUP				= 1 << 26,
	RPG_ITEM_FLAGS_MERGEONLY			= 1 << 27,
	RPG_ITEM_FLAGS_WEAPONTARGETING		= 1 << 28,
	RPG_ITEM_FLAGS_SINGLESHOT			= 1 << 29,
	RPG_ITEM_FLAGS_REMOVEINSTANT		= 1 << 30,
	RPG_ITEM_FLAGS_NOMERGE				= 1 << 31,

	RPG_ITEM_FLAGS2_UNIQUE				= 1 << 0,
	RPG_ITEM_FLAGS2_THROWAWAYONEMPTY	= 1 << 1,
	RPG_ITEM_FLAGS2_NOLASTHROWAWAY		= 1 << 2,
	RPG_ITEM_FLAGS2_NOTSELECTABLE		= 1 << 3,
	RPG_ITEM_FLAGS2_CLONEATTACHITEM		= 1 << 4,
	RPG_ITEM_FLAGS2_NOMUZZLE			= 1 << 5,
	RPG_ITEM_FLAGS2_NOAMMODRAW			= 1 << 6,
	RPG_ITEM_FLAGS2_TAGGEDFORRELOAD		= 1 << 7,
	RPG_ITEM_FLAGS2_FORCESHELLEMIT		= 1 << 8,
	RPG_ITEM_FLAGS2_GRABBABLEOBJECT		= 1 << 9,
	RPG_ITEM_FLAGS2_FORCEEQUIPRIGHT		= 1 << 10,
	RPG_ITEM_FLAGS2_FORCEEQUIPLEFT		= 1 << 11,
	RPG_ITEM_FLAGS2_PERMANENT			= 1 << 12,
	RPG_ITEM_FLAGS2_DRAINSDARKNESS		= 1 << 13,
	RPG_ITEM_FLAGS2_AUTOFJUFF			= 1 << 14,
	RPG_ITEM_FLAGS2_TRANSCENDIVENTORY	= 1 << 15,



	// only set at runtime
	RPG_ITEM_FLAGS2_IAMACLONE			= 1 << 31,


	RPG_ITEM_MAXLINKS = 6,
	RPG_ITEM_NUMANIMSOUNDS = 4,

	RPG_ITEM_MAXACTIVATERUMBLE = 3,

	// Load/save mask indexes. When creating own flags for
	// derived classes, be sure that your first index is
	// LOADSAVEMASK_INDEX_DERIVEDBASE so your flags don't
	// coincide with these flags.
	LOADSAVEMASK_INDEX_NUMITEMS			= 0,
	LOADSAVEMASK_INDEX_ASSOCIATEDAMMO,
	LOADSAVEMASK_INDEX_DERIVEDBASE,

	// Save mask flags
	LOADSAVEMASK_NUMITEMS				= 1 << LOADSAVEMASK_INDEX_NUMITEMS,			// Save m_NumItems from class CRPG_Object_Item
	LOADSAVEMASK_ASSOCIATEDAMMO			= 1 << LOADSAVEMASK_INDEX_ASSOCIATEDAMMO,	// Save associated ammo class


	// Animtypes
	RPG_ITEM_ANIM_HOLD							= 0,
	RPG_ITEM_ANIM_ATTACK						= 1,
	RPG_ITEM_ANIM_RELOAD						= 2,
	RPG_ITEM_ANIM_EQUIP							= 3,
	RPG_ITEM_ANIM_NOAMMOHOLD					= 4,
	RPG_ITEM_ANIM_NOAMMO						= 5,
	RPG_ITEM_ANIM_RELOAD_CROUCH					= 6,
	//RPG_ITEM_ANIM_CROUCH						= 2,
	//RPG_ITEM_ANIM_CROUCH_FWD					= 3,
	//RPG_ITEM_ANIM_CROUCH_BWD					= 4,
	//RPG_ITEM_ANIM_WALKFWD						= 5,
	//RPG_ITEM_ANIM_WALKBWD						= 6,
	//RPG_ITEM_ANIM_RUNFWD						= 7,
	//RPG_ITEM_ANIM_RUNBWD						= 8,
	//RPG_ITEM_ANIM_RELOADCROUCH					= 10,
	//RPG_ITEM_ANIM_RELOADCROUCHFWD				= 11,
};

//-------------------------------------------------------------------

#define LOADMASK_SETBIT(_Bit) (m_LoadMask |= (1<<_Bit))
#define LOADMASK_CLRBIT(_Bit) (m_LoadMask &= ~(1<<_Bit))
#define LOADMASK_TESTBIT(_Bit) ((m_LoadMask & (1<<_Bit)) == (1<<_Bit))
//-------------------------------------------------------------------
#define SAVEMASK_SETBIT(_Bit) (m_SaveMask |= (1<<_Bit))
#define SAVEMASK_CLRBIT(_Bit) (m_SaveMask &= ~(1<<_Bit))
#define SAVEMASK_TESTBIT(_Bit) ((m_SaveMask & (1<<_Bit)) == (1<<_Bit))

//-------------------------------------------------------------------

class CRPG_Object_Item : public CRPG_Object
{
	MRTC_DECLARE;

public:
	CFStr m_ItemName_Debug;	// For debugging purposes only
	CFStr m_ItemDescription; // usually not set unless changed by script

	static const char* ms_FlagsStr[];
	static const char* ms_FlagsStr2[];
	static const char* ms_RenderFlagsStr[];

	int m_Price;
	int m_WeaponGroup;
	int m_FireTimeout;

	int m_iSpellIndex;
	int m_iItemType;
	int m_GroupType;
	int m_AnimType;
	int m_iSkill;
	int m_iSound_Learned;
	int m_iSound_Cast;
	int m_iSound_Cast_Player;
	int m_iSound_Cast_LeftItem_Player;
	int m_iSound_Hold;
	int m_iSound_End;
	int m_iSound_Equip;
	int m_iSound_Unequip;
	int m_iSound_Pickup;
	int m_iSound_Dropped;
	int m_iSound_Anim[RPG_ITEM_NUMANIMSOUNDS];
	int m_iMuzzleAttachPoint;
	int16	m_AmmoLoad;
	int16	m_MaxAmmo;
	int16	m_ShotsFired;
	int16	m_iShellType;
	int8	m_iShellAttach;
	int8	m_AnimGripType;
	int8	m_iFirstAttachPoint;

	// How long (in ticks) it takes to unequip an item
	int m_NumEquipTicks;
	int m_NumUnequipTicks;

	int32 m_AnimProperty;
	CAutoVar_AttachModel m_Model;
	CAutoVar_AttachModel *m_pEquippedModel;

	int m_NumItems;
	int m_MaxItems;
	int m_PrimaryMagazineSize;
	int m_AmmoDraw;
	int m_iExtraActivationWait;

	// Offset to regular aimposition in modelspace coordinates
	CVec3Dfp32 m_AimOffset;

	// CVec3Dfp32 m_PickupOfs;
	int m_iLastActivator;
	int m_iClientHelperObject;
	int m_Flags;
	int m_Flags2;

	int m_PendingEquipUpdate;

	int m_lLinkedItemTypes[RPG_ITEM_MAXLINKS];
	int m_NumLinks;
	int m_iCurLink;
	int m_iPhysModel;

	// Unique item identifier
	int m_Identifier;
	int m_LastEquipped;

	// Animgraph resource
	int16 m_iAnimGraph;

	int16 m_iIconSurface;
	// When picking up mission items for instance
	int16 m_iActivationSurface;

	// the imgae that appears in the journal
	int16 m_JournalImage;
	
	int16 m_PickupMsg_iDialogue;
	int8 m_PickupMsg_iDialogueID;

	int8 m_FocusFrameOffset;
	int16 m_DropItemOffset;

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


	// AI type string. Interpreted by the AI weapon/itemhandlers
	CStr m_AIType;
	// The angle centered on a characters look direction within which an item is considered useful. Fractions, default is 1.0f i.e. always useful. 
	fp32 m_AIUseFOV;

	//Security clearance required to wield weapon.
	int m_RequiredClearance;

	// Security clearance given by this item
	int m_GivenClearanceLevel;


	// Grr, no flag space left :/
	uint8 m_bUnequipWhenEmpty : 1;
	uint8 m_bNoPlayerPickup : 1;
	uint8 m_bReplaceOnSameType : 1;
	uint8 m_bNoLoadedAmmoMergeOnPickup : 1;

protected:
	//
	// Save system
	//
	// The SaveMask will tell the 'this' object that it needs to be saved.
	// The mask is supposed to be used by subclasses of CRPG_Object_Item,
	// that way we can maintain a viable class-hierarchy by letting methods
	// of a class set or clear flags in this save-mask, and still compress data.
	// That way we know that we don't accidentally save the same data twice
	// the further up the hierarchy we go. The reason to maintain a viable
	// hierarchi is to keep in sync with the classes derrived from this class.
	// The LoadMask is filled in by CRPG_Object_Item::OnDeltaLoad, and should
	// be obeyed by derived classes.
	uint32 m_SaveMask;
	uint32 m_LoadMask;

private:
	void FindLaserBeamModelIndex();

public:

	virtual void OnCreate();
	virtual void OnIncludeClass(const CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer);

	virtual int GetType() const				{ return TYPE_ITEM; }
	virtual CFStr GetItemName() const;
	CFStr GetItemDesc() const;
	uint32 GetRenderFlags();
	
	virtual bool Activate(const CMat4Dfp32 &, CRPG_Object *_pRoot, int _iObject, int _Input);
	virtual bool OnActivate(const CMat4Dfp32 &, CRPG_Object *_pRoot, int _iObject, int _Input) { return true; }
	virtual bool Deactivate(const CMat4Dfp32 &, CRPG_Object *_pRoot, int _iObject, int _Input) { return true; }
	virtual bool Block(bool _bAutoBlock, const CMat4Dfp32 &, CRPG_Object *_pRoot, int _iObject, int _iSender = 0);
	virtual void Interrupt(const CMat4Dfp32 &, CRPG_Object *_pRoot, int _iObject, int _Input) {}
	virtual bool OnProcess(CRPG_Object *_pRoot, int);
	virtual bool MergeItem(int _iObject, CRPG_Object_Item *_pObj);
	virtual bool WorldUse(int _iObject, int _iTrigger, CRPG_Object *_pRoot) { return false; }
	virtual int OnPickup(int _iObject, CRPG_Object *_pRoot, bool _bNoSound, int _iSender, bool _bNoPickupIcon = false);
	virtual void CycleMode(int _iObject) {}
	virtual void ForceReload(int _iObject) {}
	virtual bool CanPickup();
	virtual bool IsUnequip() { return false; }

	virtual TPtr<class CRPG_Object_Item> GetForcedPickup() { return NULL; }
	int16 GetDropItemOffset() { return m_DropItemOffset; }
	void SetItemIdentifier(int _Identifier) { m_Identifier = _Identifier; }
	int GetItemIdentifier() { return m_Identifier; }

	// Load / save
	virtual void OnDeltaLoad(CCFile* _pFile, CRPG_Object* _pCharacter);
	virtual void OnDeltaSave(CCFile* _pFile, CRPG_Object* _pCharacter);
	bool NeedsDeltaSave(CRPG_Object* _pCharacter);

	// Called from CRPG_Object_Char, which is called from CWObject_Character, when that character dies.
	// This is needed, since OnProcess is not run when characters are dead (performance issue), so we need some way to know that the character has died.
	virtual void OnCharDeath(int _iObject, CRPG_Object* _pRoot) {}

	// NOTE: IF THESE ARE CHANGED, THEY MUST BE UPDATED FOR ALL SUBCLASSES TOO!
	// Shield, Staff, Bow, etc...
	virtual bool Equip(int _iObject, CRPG_Object *_pRoot, bool _bInstant = false, bool _bHold = false);
	virtual bool Unequip(int _iObject, CRPG_Object *_pRoot, bool _bInstant = false, bool _bHold = false);

	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	int ResolveAnimHandle(const char *_pSequence);

	virtual void SetCloneIdentifier(int32 _CloneIdentifier){};
	virtual bool CheckAmmo(CRPG_Object *_pRoot);
	virtual bool NeedReload(int _iObject, bool bCanReload = false) { return false; }
	virtual int GetAmmo(CRPG_Object* _pRoot, int _iType = 0);
	virtual int GetMaxAmmo() { return 0; }
	virtual int GetMagazines(void);
	virtual void SetAmmo(int _Num) {}
	virtual int GetAmmoDraw() { return m_AmmoDraw; }
	virtual bool DrawAmmo(int _Num, const CMat4Dfp32 &, CRPG_Object *_pRoot, int _iObject, bool _bApplyFireTimeout = true);
	virtual void Reload(int _iObject) { m_ShotsFired = 0; } // Reset shots fired
	virtual bool UpdateAmmoBarInfo(CRPG_Object *_pRoot, uint16 &_Surface1, uint16 &_Surface2, uint8 &_NumTotal, uint8 &_NumLoaded, uint32 &_Extra);


	// Tag animations for precache given a character object
	virtual void TagAnimationsForPrecache(class CWAG2I_Context* _pContext, class CWAG2I* _pAgi);

	virtual bool CanUse(int32 _iChar);
	
//	static bool GetItemRenderMatrix(CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel, CXR_Model *_pModel, int _iRotTrack, int _iAttachPoint, CMat4Dfp32 &_Mat);
//	bool GetAttachMatrix(CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel, CMat4Dfp32 &_Mat, int _AttachPoint, int _CharAttachpoint = -1, int _CharRottrack = -1);
	
	struct CAttachInfo
	{
		CXR_SkeletonInstance *m_pSkelInstance;
		CXR_Skeleton *m_pCharSkeleton;
		CXR_Skeleton *m_pItemSkeleton;
		int m_iCharRotTrack;
		int m_iCharAttachPoint;
		int m_iItemAttachPoint;
	};

	static CMat4Dfp32 GetAttachOnCharMatrix(CAttachInfo &_AttachInfo);
	static CMat4Dfp32 GetItemOnCharMatrix(CAttachInfo &_AttachInfo);
	static CMat4Dfp32 GetAttachOnItemMatrix(CAttachInfo &_AttachInfo);

	CMat4Dfp32 GetCurAttachOnItemMatrix(int _iObject, int _iAttachPoint);
	CMat4Dfp32 GetCurAttachOnItemMatrix(int _iObject);
	CMat4Dfp32 GetAttachMatrix(int _iObject, int _iCharRottrack, int _iCharAttachPoint, int _iItemAttachPoint = -1);

	virtual void GetShellSpawnData(int _iObject, class CWO_Shell_SpawnData* _pSpawnData);

	virtual uint8 GetRefreshFlags(uint8 _Flags);
	virtual uint16 GetActivateAnim() { return ~0; }

	class CRPG_Object_Char* GetChar(CRPG_Object* _pRoot);

	int GetCurLinkItemType();
	class CRPG_Object_Ammo* GetAssociatedAmmo(CRPG_Object *_pRoot);
	CRPG_Object_Item* GetCurLinkedItem(CRPG_Object *_pRoot);
	bool HasLinkedItemType(int _ItemType);

	virtual bool IsEquippable(CRPG_Object* _pRoot);
	virtual bool IsActivatable(CRPG_Object* _pRoot,int _iObject,int _Input = 1);

	// Character wrappers
	bool ApplyWait(int _iObject, int _Ticks, fp32 _NoiseLevel, fp32 _Visibility);
	bool SetWait(int _iObject, int _Ticks);
	int PlayAnimSequence(int _iObject, int _iAnim, int _Flags = 0); // Return duration of animation in ticks
	void SetAnimStance(int _iObject, int _iAnim);
	int SendDamage(int _iObject, const CVec3Dfp32& _Pos, int _iSender, int _Damage, uint32 _DamageType, int _SurfaceType = 0, const CVec3Dfp32 *_pForce = NULL, const CVec3Dfp32 *_pSplatterDir = NULL, CCollisionInfo* _pCInfo = NULL, const char* _pDamageEffect = NULL, int _StunTicks = -1);

	int16 GetShotsFired() { return m_ShotsFired; }

	// Skeleton animated item
	//int32 PlayWeaponAnimFromType(int32 _Type, int32 _iObject, uint16 _Flags);
	//int PlayWeaponAnim(int _iAnim, int _iObject, uint16 _Flags);

	virtual void UpdateAmmoList(CRPG_Object *_pRoot);

	//Get the number of extra magazines to show. Return -1 if we shoudn't show extra magazines.
	virtual int ShowNumMagazines();

	// Helpers
	int GetGameTick(int _iObject);

	void Line(const CVec3Dfp32& _Pos0, const CVec3Dfp32& _Pos1, int _Col = 0xffffffff);

	//Returns the speed (in world units per server frame) of any projectiles the item can summon, or
	//_FP32_MAX if these projectiles are instantaneous. If the object does not summon projectiles, 0 is returned.
	virtual fp32 GetProjectileSpeed();
	virtual CVec3Duint8 GetMuzzleColor();
	virtual void GetMuzzleProperties(uint32& _MuzzleLightColor, uint32& _MuzzleLightRange) {}
	virtual bool HasLightningLight() { return false; }

	virtual CWO_Damage * GetDamage();

	//Check if this is a melee weapon
	virtual bool IsMeleeWeapon();

	int GetRequiredClearanceLevel();
	int GetGivenClearanceLevel();
};

//-------------------------------------------------------------------

#endif /* WRPGItem_h */
