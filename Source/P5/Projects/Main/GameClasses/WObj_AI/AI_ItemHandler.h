#ifndef _INC_AI_ITEMHANDLER
#define _INC_AI_ITEMHANDLER

class CAI_AgentInfo;
class CAI_ActionEstimate;

class CAI_WeaponHandler; 
class CAI_ItemHandler; 

class CAI_Weapon;
typedef TPtr<CAI_Weapon> spCAI_Weapon;

class CAI_Item;
typedef TPtr<CAI_Item> spCAI_Item;


//WEAPONS////////////////////////////////////////////////////////////////////////////////////////////

//Base class for handling a specific weapon
class CAI_Weapon : public CReferenceCount
{
protected:
	//Special range bands
	enum {
		INFINITE_RANGE = 1000000,
	};

	//The AI that uses the weapon
	CAI_Core * m_pAI;

	//The corresponding world-item
	class CRPG_Object_Item * m_pItem; 

	//Weapon type
	int m_iType;

	//Counts the number of frames since the item was last used or is -1 if wait is zero
	int m_iUseTimer;

public:
	//Weapon types
	enum {
		UNKNOWN = 0,
		NONWEAPON, //Non-weapon "weapon". Brooms, cigarettes etc

		//Melee weapons
		MELEE,
		FIST,
		DRILL,
		BIGALIEN_HEAD,
		BIGALIEN_TAIL,
		SMALLALIEN_SPURS,
		MAX_MELEE,

		//Ranged weapons
		RANGED,

		RANGED_RAPIDFIRE,

		RANGED_AREAEFFECT,
		GRENADE,
		ROCKET, //Generic non-instant exploding projectile
		PERSISTENT, //Generic instant-hit, persistent area effect weapon
		MAX_RANGED,

		MAX_WEAPON,
	};
	int GetType();

	//Constructor
	CAI_Weapon(CRPG_Object_Item * _pItem = NULL, CAI_Core * _pAI = NULL, int _iType = CAI_Weapon::UNKNOWN);

	//Resets the weapon data
	virtual void Reset();

	//Refreshes the weapon one frame
	virtual void OnRefresh();

	//Returns CActionEstimate values (0.100f) appropriately for taking the given action (or default 
	//action if no action is specified) against the given target right now, taking such factors as 
	//damage per attack, rate of attacks ammo, skill, etc into consideration. One can optionally
	//specify some opponent info to influence the estimate (see specific weapon class for how 
	//this info is handled)
	virtual CAI_ActionEstimate GetEstimate(CAI_AgentInfo* _pTarget, int _Action = 0);

	// Use weapon on target
	virtual void OnUse(CAI_AgentInfo * _pTarget, int _Action = 0);
	// Use weapon at pos
	virtual void OnUse(const CVec3Dfp32& _Pos);

	//Take aim at the target
	virtual void OnAim(CAI_AgentInfo* _pTarget);

	//Adds commands to switch to this weapon if possible
	virtual void OnWield();

	//Checks target is "good" i.e. can be attacked when weapon is ready for this
	enum {
		MODE_DEFAULT,
		MODE_MELEE,
		MODE_RANGED,
	};
	virtual bool IsGoodTarget(CAI_AgentInfo * _pTarget, int _Mode = MODE_DEFAULT);

	//Checks if this is a ranged weapon
	bool IsRangedWeapon();

	//Checks if this is a melee weapon
	bool IsMeleeWeapon();

	//Returns current number of frames since weapon was used
	int GetUseTimer();

	//Checks if the item cannot be used to inflict damage
	virtual bool IsHarmless();

	//Checks if the item is valid (i.e. has it's Item, AI and World pointers set)
	bool IsValid();

	//Checks if this item is wielded
	bool IsWielded();

	//Returns the RPG item
	CRPG_Object_Item * GetItem();

	//Check if given position is within usable arc of given user with given item
	static bool IsInUsableFOV(const CVec3Dfp32& _Position, CAI_Core * _pUserAI, CRPG_Object_Item * _pItem);

/* Deprecated
	//Returns the truncated current aiming score
	virtual int GetAimingScore(CAI_AgentInfo * _pTarget);

	//Returns the current aiming score delta per frame when aiming at the given target
	virtual fp32 GetAimingScoreDelta(CAI_AgentInfo * _pTarget);
*/

	//Returns maximum effective range agaist the specified target for estimation purposes. 
	//If no target is given this simply returns the effective range against a singularity.
	virtual fp32 MaxRange(CAI_AgentInfo * _pTarget = NULL);

	//Returns minimum effective range, agaist the specified target. If no target is given 
	//this simply returns the minimum range against a singularity.
	virtual fp32 MinRange(CAI_AgentInfo * _pTarget = NULL);

	//Change AI user
	void ReInit(CAI_Core * _pAI);

	//Calculate the activateposition matrix for this weapons wielder. This adds some scatter due to lack of skill etc to the given matrix
	virtual void GetActivatePosition(CMat4Dfp32* _pMat);
};

//Base handler for ranged weapons
class CAI_Weapon_Ranged : public CAI_Weapon
{
protected:
	//Maximum uncertainty for an "accurate" shot
	static const fp32 MAX_UNCERTAINTY;

	//The position we should aim at when shooting 
	CVec3Dfp32 m_TargetPos;

	//Did we aim or use our weapon last frame?
	bool m_bAiming;
	
	//Our current target. When switching target, the aiming score is reset
	int m_iTarget;

	//The aiming score is the current bonus to the precision received from aiming. Spending time aiming 
	//improves this score, target movement decreases it. It is reset if we switch target or stops aiming.
	fp32 m_AimingScore;
	fp32 m_StationaryBonus;

	//Checks if we think there is a line of sight to the target
	virtual bool IsLineOfSight(CAI_AgentInfo * pTarget); 

	//Check how much ammo there's left. Check ammo in magazine only if the optional flag is true
	//This returns 100 if weapon does not draw ammo.
	virtual int CheckAmmo(bool _bMagazineOnly = false);

public:
	//Constructor
	CAI_Weapon_Ranged(CRPG_Object_Item * _pItem = NULL, CAI_Core * _pAI = NULL, int _iType = CAI_Weapon::RANGED);

	//Resets the weapon data
	virtual void Reset();

	//Refreshes the weapon one frame
	virtual void OnRefresh();

	//Returns CActionEstimate values (0.100f) appropriately for taking the given action (or default 
	//action if no action is specified) against the given target right now, taking such factors as 
	//damage per attack, rate of attacks ammo, skill, etc into consideration. 
	virtual CAI_ActionEstimate GetEstimate(CAI_AgentInfo* _pTarget, int _Action = 0);

	//Use the weapon if possible
	virtual void OnUse(CAI_AgentInfo * _pTarget, int _Action = 0);
	virtual void OnUse(const CVec3Dfp32& _Pos);

	//Take aim at the target
	virtual void OnAim(CAI_AgentInfo* _pTarget);

	//Checks target is "good" i.e. is in LOS
	virtual bool IsGoodTarget(CAI_AgentInfo * _pTarget, int _Mode = MODE_DEFAULT);

/* Deprecated
	//Returns the truncated current aiming score
	virtual int GetAimingScore(CAI_AgentInfo * _pTarget);

	//Returns the current aiming score delta per frame when aiming at the given target
	virtual fp32 GetAimingScoreDelta(CAI_AgentInfo * _pTarget);
*/

	//Returns maximum effective range agaist the specified target for estimation purposes. 
	//If no target is given this simply returns the effective range against a singularity.
	virtual fp32 MaxRange(CAI_AgentInfo * _pTarget = NULL);

	//Returns minimum effective range, agaist the specified target. If no target is given 
	//this simply returns the minimum range against a singularity.
	virtual fp32 MinRange(CAI_AgentInfo * _pTarget = NULL);

	//Calculate the activateposition matrix for this weapons wielder. This adds some scatter due to lack of skill etc to the given matrix
	virtual void GetActivatePosition(CMat4Dfp32* _pMat);

	//Checks if the item cannot be used to inflict damage
	virtual bool IsHarmless();
};


//Base handler for melee weapons
class CAI_Weapon_Melee : public CAI_Weapon
{
protected:
	//Is the target within striking distance?
	virtual bool InMeleeRange(CAI_AgentInfo * _pTarget);

	//Is the target within minimum melee range?
	virtual bool InMinMeleeRange(CAI_AgentInfo * _pTarget);

	//Get the corresponding melee device action for the given action
	virtual int GetMeleeAction(int _Action);

public:
	//Constructor
	CAI_Weapon_Melee(CRPG_Object_Item * _pItem = NULL, CAI_Core * _pAI = NULL, int _iType = CAI_Weapon::MELEE);

	//Returns CActionEstimate values (0.100f) appropriately for taking the given action (or default 
	//action if no action is specified) against the given target right now, taking such factors as 
	//damage per attack, rate of attacks ammo, skill, etc into consideration. 
	//Opponent info is the action we think opponent is taking.
	virtual CAI_ActionEstimate GetEstimate(CAI_AgentInfo* _pTarget, int _Action = 0);

	//Checks target is "good" i.e. in melee range
	virtual bool IsGoodTarget(CAI_AgentInfo * _pTarget, int _Mode = MODE_DEFAULT);

	//Take the given action if possible
	virtual void OnUse(CAI_AgentInfo * _pTarget, int _Action = 0);

	//Returns maximum effective range agaist the specified target for estimation purposes. 
	//If no target is given this simply returns the effective range against a singularity.
	virtual fp32 MaxRange(CAI_AgentInfo * _pTarget = NULL);

	//Returns minimum effective range, agaist the specified target. If no target is given 
	//this simply returns the minimum range against a singularity.
	virtual fp32 MinRange(CAI_AgentInfo * _pTarget = NULL);

	//Checks if the item cannot be used to inflict damage
	virtual bool IsHarmless();
};


//Weapon handler for fist-fighting
class CAI_Weapon_Melee_Fist : public CAI_Weapon_Melee
{
protected:
	//Get the corresponding melee device action for the given action
	virtual int GetMeleeAction(int _Action);

public:
	//Constructor
	CAI_Weapon_Melee_Fist(CRPG_Object_Item * _pItem = NULL, CAI_Core * _pAI = NULL, int _iType = CAI_Weapon::FIST);

	//Returns CActionEstimate values (0.100f) appropriately for taking the given action (or default 
	//action if no action is specified) against the given target right now, taking such factors as 
	//damage per attack, rate of attacks ammo, skill, etc into consideration. 
	//Opponent info is the action we think opponent is taking.
	virtual CAI_ActionEstimate GetEstimate(CAI_AgentInfo* _pTarget, int _Action = 0, int _OpponentInfo = 0);

	//Checks target is "good" i.e. in melee range
	virtual bool IsGoodTarget(CAI_AgentInfo * _pTarget, int _Mode = MODE_DEFAULT);

	//Returns maximum effective range agaist the specified target for estimation purposes. 
	//If no target is given this simply returns the effective range against a singularity.
	virtual fp32 MaxRange(CAI_AgentInfo * _pTarget = NULL);

	//Returns minimum effective range, agaist the specified target. If no target is given 
	//this simply returns the minimum range against a singularity.
	virtual fp32 MinRange(CAI_AgentInfo * _pTarget = NULL);
};


//Base handler for ranged weapons with area effect
class CAI_Weapon_Ranged_AreaEffect : public CAI_Weapon_Ranged
{
protected:
	//Rough radius of the area of effect
	fp32 m_BlastRadius;

	//Checks if there is a line of sight to the target
	virtual bool IsLineOfSight(CAI_AgentInfo * pTarget); 

public:
	//Constructor
	CAI_Weapon_Ranged_AreaEffect(CRPG_Object_Item * _pItem = NULL, CAI_Core * _pAI=NULL, int _iType = CAI_Weapon::RANGED_AREAEFFECT);

	//Returns CActionEstimate values (0.100f) appropriately for taking the given action (or default 
	//action if no action is specified) against the given target right now, taking such factors as 
	//damage per attack, rate of attacks ammo, skill, etc into consideration. 
	virtual CAI_ActionEstimate GetEstimate(CAI_AgentInfo* _pTarget, int _Action = 0);

	//Returns minimum effective range, agaist the specified target. If no target is given 
	//this simply returns the minimum range against a singularity.
	virtual fp32 MinRange(CAI_AgentInfo * _pTarget = NULL);

	//Aim at feet as default, not focus position
	virtual void GetActivatePosition(CMat4Dfp32* _pMat);
};



//Handler for grenade type weapons
class CAI_Weapon_Grenade : public CAI_Weapon_Ranged_AreaEffect
{
protected:
	//Values for calculating the bomb's trajectory
	fp32 m_ThrowSpeed;
	fp32 m_Gravity;

	//How long since we started using bomb?
	int m_UseCount;

public:
	//HACK! I've just made a quick fix of the bomb since it's been changed to a release weapon and 
	//precision isn't that important
	
	//Constructor
	CAI_Weapon_Grenade(CRPG_Object_Item * _pItem = NULL, CAI_Core * _pAI=NULL, int _iType = CAI_Weapon::GRENADE);

	//Refreshes the weapon one frame. Continually uses weapon until it's time to release, 
	//while aiming towards target
	virtual void OnRefresh();

	//Returns CActionEstimate values (0.100f) appropriately for taking the given action (or default 
	//action if no action is specified) against the given target right now, taking such factors as 
	//damage per attack, rate of attacks ammo, skill, etc into consideration. 
	virtual CAI_ActionEstimate GetEstimate(CAI_AgentInfo* _pTarget, int _Action = 0);

	//Start using bomb
	virtual void OnUse(CAI_AgentInfo * _pTarget, int _Action = 0);

	//Calculate the activateposition matrix for this weapons wielder. This adds some scatter due to lack of skill etc to the given matrix
	virtual void GetActivatePosition(CMat4Dfp32* _pMat);

	//Returns maximum effective range agaist the specified target for estimation purposes. 
	//If no target is given this simply returns the effective range against a singularity.
	virtual fp32 MaxRange(CAI_AgentInfo * _pTarget = NULL);

	//Returns minimum effective range, agaist the specified target. If no target is given 
	//this simply returns the minimum range against a singularity.
	virtual fp32 MinRange(CAI_AgentInfo * _pTarget = NULL);
};


//Handler for rapid fire type ranged weapons
class CAI_Weapon_Ranged_RapidFire : public CAI_Weapon_Ranged
{
protected:
	//Burst countdown (frames the trigger is held down)
	int m_nBurstLeft;

	//The vector direction we're strafing our burst
	// CVec3Dfp32 m_BurstDir;

	//Target position when last shot was fired
	CVec3Dfp32 m_PrevTargetPos;

	//Calculate rate of fire per second
	fp32 ROF();

public:
	//Constructor
	CAI_Weapon_Ranged_RapidFire(CRPG_Object_Item * _pItem = NULL, CAI_Core * _pAI=NULL, int _iType = CAI_Weapon::RANGED_RAPIDFIRE);

	//Resets the weapon data
	virtual void Reset();

	//Refreshes the weapon one frame. During a burst, the weapon will be fired repeatedly, 
	//while strafing the target position across the target.
	virtual void OnRefresh();

	//Same as for ordinary ranged weapons, but offensive value increased a bit. Wielders of rapid fire weapons should always be more trigger happy :)
	virtual CAI_ActionEstimate GetEstimate(CAI_AgentInfo* _pTarget, int _Action = 0);

	//Always fire a burst unless there's very little ammo left
	virtual void OnUse(CAI_AgentInfo * _pTarget, int _Action = 0);
};



//Handles all weapons of a bot
class CAI_WeaponHandler 
{
protected:
	//The AI that uses the item handler
	CAI_Core * m_pAI;

	//Dummy weapon handler when no other weapons are available
	static CAI_Weapon ms_DummyWeapon;

	//The specific weapon handlers
	TArray<spCAI_Weapon> m_lspWeapons;

	//Currently wielded weapon index
	int m_iWielded;	

	//Should we update weapons next refresh?
	bool m_bUpdateWeapons;

	//Constructs a new weapon object from the corresponding world object and returns smart pointer to it
	spCAI_Weapon CreateWeapon(CRPG_Object_Item * _pWeapon); 
		
	//Synch weapon handlers with character weapons
	void OnUpdateWeapons();

	//Return recommended priority for given weapon type
	static int Recommendation(CAI_Weapon * _pWeapon);

	//Hack to enable item handler to add weapons to weapon handler
	friend class CAI_ItemHandler;

public:
	// What class of weapon has bearing on weather our bravery will allow us to attack or not
	int16 m_ArmsClass;
	enum
	{
		AI_ARMSCLASS_INVALID	= -1,	// Nuttin'
		AI_ARMSCLASS_UNARMED	= 0,	// Fist
		AI_ARMSCLASS_MELEE		= 1,	// Chank, club, alien, screamerzapper
		AI_ARMSCLASS_GUN		= 2,	// Pistol, Rifle, Tranq
		AI_ARMSCLASS_HEAVY		= 3,	// Minigun, Rocket
	};

	int16 m_WeaponType;
	enum
	{
		AI_WEAPONTYPE_UNDEFINED		= 0,
		AI_WEAPONTYPE_BAREHANDS		= 1,
		AI_WEAPONTYPE_GUN			= 2,
		AI_WEAPONTYPE_SHOTGUN		= 4,
		AI_WEAPONTYPE_ASSAULT		= 8,
		AI_WEAPONTYPE_MINGUN		= 16,
		AI_WEAPONTYPE_KNUCKLEDUSTER	= 32,
		AI_WEAPONTYPE_SHANK			= 64,
		AI_WEAPONTYPE_CLUB			= 128,
		AI_WEAPONTYPE_GRENADE		= 256,
	};

	//Defines what weapon type corresponds to a given string.
	static int GetWeaponType(CStr _TypeStr);

	//Constructors
	CAI_WeaponHandler(CAI_Core * _pAI = NULL);

	//Initialize weapon handler
	void Init();

	//Refreshes the item handler one frame
	void OnRefresh();

	//Update weapons next refresh
	void UpdateWeapons();

	//Currently wielded weapon
	CAI_Weapon * GetWielded();
	void UpdateWieldedArmsClass();
	int GetWieldedArmsClass();
	int GetWieldedWeaponType();

	//Switch to best weapon against the given target under the given constraints 
	//Succeeds if there's a weapon to switch to
	enum {
		ALL_WEAPONS = 0,
		RANGED_ONLY,
		MELEE_ONLY,
		NONWEAPON_ONLY,
	};
	bool SwitchToBestWeapon(CAI_AgentInfo * pTarget, int _iConstraint = ALL_WEAPONS);

	//Switch to gererally "best" weapon. Succeeds if there's a weapon to switch to
	bool SwitchToBestWeapon(int _iConstraint = ALL_WEAPONS);

	//Change AI user
	void ReInit(CAI_Core * _pAI);

	//Get itemtype of recommended weapon to start with.
	int GetRecommendedWeapon();
};






//ITEMS///////////////////////////////////////////////////////////////////////////////////////////////

//Base class for handling a specific item
class CAI_Item : public CReferenceCount
{
protected:
	//The AI that uses the item
	CAI_Core * m_pAI;

	//The corresponding world-item
	CRPG_Object_Item * m_pItem; 

	//Item type
	int m_iType;

	//Counts the number of frames since the item was last used or is -1 if wait is zero
	int m_iUseTimer;

public:
	//Item types
	enum {
		UNKNOWN = 0,

		//Shields
		SHIELD,
		MAX_SHIELDS,

		//Ammo
		AMMO,
		MAX_AMMO,

		//Potions
		POTION,
		POTION_HEALING,
		MAX_POTIONS,

		//Armour
		ARMOUR,
		MAX_ARMOUR,

		//Miscellaneous
			
		MAX_ITEM,
	};
	int GetType();

	//Constructor
	CAI_Item(CRPG_Object_Item * _pItem = NULL, CAI_Core * _pAI = NULL, int _iType = UNKNOWN);

	//Reset item data
	virtual void Reset();

	//Refreshes the weapon one frame
	virtual void OnRefresh();

	//Returns a value that estimates how suitable the item is to use against the specified 
	//target right now. Most items cannot be used against targets, but will instead return estimate
	//for using item on self.
	virtual CAI_ActionEstimate GetEstimate(CAI_AgentInfo* _pTarget = NULL);

	//Use the item if possible
	virtual void OnUse(CAI_AgentInfo * _pTarget, int _Action = 0);

	//Adds commands to switch to this weapon if possible
	virtual void OnWield();

	//Checks if this is a shield
	bool IsShield();

	//Checks if this is ammo
	bool IsAmmo();

	//Returns current number of frames since weapon was used
	int GetUseTimer();

	//Checks if the item is valid (i.e. has it's Item, Wielder and World pointers set)
	virtual bool IsValid();

	//Checks if this item is wielded
	bool IsWielded();

	//Returns the RPG item
	CRPG_Object_Item * GetItem();

	//Change AI user
	void ReInit(CAI_Core * _pAI);

	//Calculate the activateposition matrix for this weapons wielder. This adds some scatter due to lack of skill etc to the given matrix
	virtual void GetActivatePosition(CMat4Dfp32* _pMat);

	//Get number of items in this item object
	virtual int GetNumItems();
};


//Base class for shields
class CAI_Item_Shield : public CAI_Item
{
protected:
	//Info about when this object last blocked an attack. and how long ago this occurred
	int m_iLastBlockedAttacker;
	int m_iBlockTimer;

public:
	//Constructor
	CAI_Item_Shield(CRPG_Object_Item * _pItem = NULL, CAI_Core * _pAI = NULL, int _iType = SHIELD);

	//Refreshes the shield one frame
	virtual void OnRefresh();

	//Returns a value that estimates how suitable the shield is to use against the specified 
	//target right now, or how suitable it is to use right now in general.
	virtual CAI_ActionEstimate GetEstimate(CAI_AgentInfo* _pTarget = NULL);

	//We just blocked this attackers attack with the item
	void Block(int _iAttacker);

	//Did we block this attacker less than the specified number of frames ago?
	bool BlockedAttack(int _iAttacker, int _iTime);

	//Checks if the given item is a shield and if so returns pointer to shield object
	static CAI_Item_Shield * GetShield(CAI_Item * _pItem);
};


//Handles all item of a bot
class CAI_ItemHandler 
{
protected:
	//The AI that uses the item handler
	CAI_Core * m_pAI;

	//Dummy item handler when no other items are available
	static CAI_Item ms_DummyItem;

	//The specific weapon handlers
	TArray<spCAI_Item> m_lspItems;

	//Currently wielded item index
	int m_iWielded;	

	//Should we update weapons next refresh?
	bool m_bUpdateItems;

	//The item type we want to switch to 
	int m_iWantedItem;

	//Helper to createitem. Defines what weapon type a specific RPG_Object_Item is.
	static int GetItemType(CRPG_Object_Item * _pItem);

	//Constructs a new item object from the corresponding world object and returns smart pointer to it
	spCAI_Item CreateItem(CRPG_Object_Item * _pItem); 
		
	//Synch item handlers with character items
	void OnUpdateItems();

public:
	//Constructors
	CAI_ItemHandler(CAI_Core * _pAI = NULL);

	//Initialize item handler
	void Init();

	//Refreshes the item handler one frame
	void OnRefresh();

	//Update items next refresh
	void UpdateItems();

	//Currently wielded item
	CAI_Item * GetWielded();

	//Switch item to shield if possible
	bool WieldShield();

	//Switch item to given itemtype if possible
	bool WieldItem(int _iType);

	//Get first item of given type (check wielded item first of all)
	CAI_Item * GetItem(int _iType);

	//Change AI user
	void ReInit(CAI_Core * _pAI);

	//Returns corresponding item type of the given string
	static int GetItemType(CStr _ItemType);
};


#endif
