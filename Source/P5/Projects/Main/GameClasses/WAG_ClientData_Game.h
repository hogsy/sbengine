#ifndef AnimGraphClientData_Templar_h
#define AnimGraphClientData_Templar_h

//--------------------------------------------------------------------------------

#include "../../Shared/MOS/Classes/GameWorld/WAnimGraphInstance/WAG_ClientData.h"
#include "../../Shared/MOS/Classes/GameWorld/WAnimGraphInstance/WAGI.h"
#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Damage.h"

#include "WRPG/WRPGDef.h"
//#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_AutoVar.h"

//--------------------------------------------------------------------------------

enum
{
	ONHIT_BODYPART_NONE			= 0,
	ONHIT_BODYPART_HEAD			= 1,
	ONHIT_BODYPART_TORSO		= 2,
	ONHIT_BODYPART_ABDOMEN		= 3,
	ONHIT_BODYPART_PELVIS		= 4,
	ONHIT_BODYPART_LEFTLEG		= 5,
	ONHIT_BODYPART_RIGHTLEG		= 6,
	ONHIT_BODYPART_LEFTARM		= 7,
	ONHIT_BODYPART_RIGHTARM		= 8,
	ONHIT_BODYPART_WEAPON		= 9,
	ONHIT_BODYPART_SHIELD		= 10,
};

enum
{
	ONHIT_DIRECTION_NONE		= 0,
	ONHIT_DIRECTION_FWD			= 1,
	ONHIT_DIRECTION_BWD			= 2,
	ONHIT_DIRECTION_LEFT		= 3,
	ONHIT_DIRECTION_RIGHT		= 4,
};

enum
{
	ONHIT_FORCE_NONE			= 0,
	ONHIT_FORCE_LIGHT			= 1,
	ONHIT_FORCE_MEDIUM			= 2,
	ONHIT_FORCE_HEAVY			= 3,
};

/* We use bitfield from WRPGDefs.h instead
enum
{
	ONHIT_DAMAGETYPE_NONE		= 0,
	ONHIT_DAMAGETYPE_PIERCE		= 1,
	ONHIT_DAMAGETYPE_CUT		= 2,
	ONHIT_DAMAGETYPE_CRUSH		= 3,
};
*/

#define MACRO_GETBUTTONFROMINDEX(i) (1 << i)

enum
{
	BUTTONPRESS_PRIMARY			= 1 << 0,
	BUTTONPRESS_SECONDARY		= 1 << 1,
	BUTTONPRESS_JUMP			= 1 << 2,
	BUTTONPRESS_A				= 1 << 3,
	BUTTONPRESS_B				= 1 << 4,
	BUTTONPRESS_X				= 1 << 5,
	BUTTONPRESS_Y				= 1 << 6,
	BUTTONPRESS_SHIFT			= 1,
	//OSV....
	DIGIPAD_UP					= 1 << 0,
	DIGIPAD_DOWN				= 1 << 1,
	DIGIPAD_LEFT				= 1 << 2,
	DIGIPAD_RIGHT				= 1 << 3,
	DIGIPAD_SHIFT				= 8,

	BUTTONTEST_USED				= 0,
	BUTTONTEST_NOTUSED			= 1,


	DIGIPAD_LEANMASK_LEFT		= (DIGIPAD_LEFT),
	DIGIPAD_LEANMASK_RIGHT		= (DIGIPAD_RIGHT),
	DIGIPAD_LEANMASK_OVERBOX	= (DIGIPAD_UP),
	DIGIPAD_LEANMASK_UNDERBOX	= (DIGIPAD_DOWN),
	DIGIPAD_LEANMASK_EDGEDOWN	= (DIGIPAD_UP | DIGIPAD_LEFT),
	DIGIPAD_LEANMASK_EDGEUP		= (DIGIPAD_UP | DIGIPAD_RIGHT),
	
	DIGIPAD_LEANMASK_GOOD		= (DIGIPAD_LEANMASK_LEFT | DIGIPAD_LEANMASK_RIGHT |
									DIGIPAD_LEANMASK_OVERBOX | DIGIPAD_LEANMASK_UNDERBOX |
									DIGIPAD_LEANMASK_EDGEDOWN | DIGIPAD_LEANMASK_EDGEUP),

	DIGIPAD_LEANMASK_BAD1		= (DIGIPAD_DOWN | DIGIPAD_LEFT),
	DIGIPAD_LEANMASK_BAD2		= (DIGIPAD_DOWN | DIGIPAD_RIGHT),
};

// Actioncutscene types
enum
{
	AG_ACSACTIONTYPE_CROUCH						= 1,
	AG_ACSACTIONTYPE_DISABLEPHYSICS				= 2,
	AG_ACSACTIONTYPE_ENABLEPHYSICS				= 3,
	AG_ACSACTIONTYPE_SETTHIRDPERSON				= 4,
	AG_ACSACTIONTYPE_SETFIRSTPERSON				= 5,
	AG_ACSACTIONTYPE_SUCCESSSETSTARTPOSITION	= 6,
	AG_ACSACTIONTYPE_SUCCESSSETENDPOSITION		= 7,

	AG_ACSACTIONTYPE_STARTANIMATION				= 8,
	AG_ACSACTIONTYPE_ENDANIMATION				= 9,
	AG_ACSACTIONTYPE_DOTRIGGER					= 10,

	AG_ACSACTIONTYPE_PICKUPITEM					= 11,

	AG_ACSACTIONTYPE_SETLEVERSTATE				= 12,
	AG_ACSACTIONTYPE_ONCHANGEVALVESTATE			= 13,

	AG_ACSACTIONTYPE_USEACSCAMERA				= 14,

	AG_ACSACTIONTYPE_DOTRIGGERMIDDLE			= 15,
	AG_ACSACTIONTYPE_DOTRIGGERREFILL			= 16,
};

enum
{
	AG_AIMELEE_ATTACK_RIGHT			= 1 << 0,
	AG_AIMELEE_ATTACK_LEFT			= 1 << 1,
	AG_AIMELEE_ATTACK_MIDDLE		= 1 << 2,
	AG_AIMELEE_ATTACK_SPECIAL		= 1 << 3,
	AG_AIMELEE_ATTACK_COMBO_RIGHT	= 1 << 4,
	AG_AIMELEE_ATTACK_COMBO_LEFT	= 1 << 5,
	AG_AIMELEE_ATTACK_COMBO_MIDDLE	= 1 << 6,
	AG_AIMELEE_ATTACK_COMBO_SPECIAL	= 1 << 7,

	AG_AIMELEE_NUM_ATTACKS			= 4,

	AG_AIMELEE_BLOCK				= 1 << 8,


	AG_AIMELEE_MASK_COMBO			= (AG_AIMELEE_ATTACK_COMBO_RIGHT | AG_AIMELEE_ATTACK_COMBO_LEFT | AG_AIMELEE_ATTACK_COMBO_MIDDLE | AG_AIMELEE_ATTACK_COMBO_SPECIAL),
	AG_AIMELEE_MASK_NORMALATTACK	= (AG_AIMELEE_ATTACK_RIGHT | AG_AIMELEE_ATTACK_LEFT | AG_AIMELEE_ATTACK_MIDDLE | AG_AIMELEE_ATTACK_SPECIAL),
};

enum
{
	RPG_ITEMTYPE_FIST = 0,
	RPG_ITEMTYPE_GUN = 3,
	RPG_ITEMTYPE_ANCIENTWEAPONS = 0xf,
};

//#define HEAVYGUARD_MAXANGLEOFFSET	(0.13f)
//#define HEAVYGUARD_MAXTILT			(0.075f)
#define HEAVYGUARDAIM_MAXCONTROLLOOK	(0.007f)
#define HEAVYGUARDAIM_INVMAXCONTROLLOOK	(1.0f/HEAVYGUARDAIM_MAXCONTROLLOOK)
#define HEAVYGUARDAIM_360VAL			(0.002777777f)
//#define ATTACKDROID_MAXANGLEOFFSET	(0.22f)
//#define ATTACKDROID_MAXTILT			(0.075f)
enum
{
	CHAR_WANTEDBEHAVIORFLAG_MASKTYPE	= 0x8000,


	CHAR_STATEFLAG_AIIDLE		= 0x01000000,

	CHAR_STATEFLAG_BEHAVIORACTIVE		= 0x00000001,
	
	CHAR_STATEFLAG_USETURNCORRECTION	= 0x00000002,
	CHAR_STATEFLAG_APPLYTURNCORRECTION	= 0x00000004,
	CHAR_STATEFLAG_APPLYFIGHTCORRECTION	= 0x00000008,

	CHAR_STATEFLAG_NOCHARCOLL			= 0x00000010,
	CHAR_STATEFLAG_ATTACHNOREL			= 0x00000020,
	CHAR_STATEFLAG_CANDISABLEREFRESH	= 0x00000040,
	CHAR_STATEFLAG_HURTACTIVE			= 0x00000080,
	CHAR_STATEFLAG_EFFECTOKENACTIVE		= 0x00000100,
	CHAR_STATEFLAG_BLOCKACTIVE			= 0x00000200,
	CHAR_STATEFLAG_EQUIPPING			= 0x00000400,
	CHAR_STATEFLAG_NOLOOK				= 0x00000800,
	CHAR_STATEFLAG_SYNCHACK				= 0x00001000,
	CHAR_STATEFLAG_HEAVYGUARDAIM		= 0x00002000,
	// When not in controlmode animation
	CHAR_STATEFLAG_FORCEROTATE			= 0x00004000,
	CHAR_STATEFLAG_MOVADVADJUST			= 0x00008000,
	// We want special handling of this animation
	CHAR_STATEFLAG_WALLCOLLISION		= 0x00010000,
	CHAR_STATEFLAG_FORCELAYERVELOCITY	= 0x00020000,
	CHAR_STATEFLAG_LADDERSTEPADJUST		= 0x00040000,
	CHAR_STATEFLAG_LAYERNOVELOCITY		= 0x00080000,
	CHAR_STATEFLAG_DISABLEBREAKOUT		= 0x00100000,
	CHAR_STATEFLAG_FIGHT_UNBLOCKABLE	= 0x00200000,
	CHAR_STATEFLAG_FIGHT_EXPANDBBOX		= 0x00400000,
	CHAR_STATEFLAG_LAYERADDITIVEBLEND	= 0x00800000,
	CHAR_STATEFLAG_LAYERADJUSTFOROFFSET	= 0x01000000,
	CHAR_STATEFLAG_NOGRAVITY			= 0x02000000,
	CHAR_STATEFLAG_TAGUSEANIMCAMERA		= 0x04000000,
	CHAR_STATEFLAG_AIMELEE_CANATTACK	= 0x08000000,
	CHAR_STATEFLAG_VELOCITYOVERRIDE		= 0x10000000,
	CHAR_STATEFLAG_DONTTOUCHCONTROLMODE	= 0x20000000,
	CHAR_STATEFLAG_NOPHYSFLAG			= 0x40000000,
	CHAR_STATEFLAG_NOITEMRENDER			= 0x80000000,

	CHAR_STATEFLAG_FORCESUBSEQUENTTO8	= CXR_ANIMLAYER_FORCESUBSEQUENTTO8,
	CHAR_STATEFLAG_SKIPFORCEKEEP		= 0x00004000,

	CHAR_STATEFLAG_EXCLUSIVEATTACK		= 0x01000000,
	CHAR_STATEFLAG_NOCOLLISION			= 0x00000001
};

// Weaponclasses (TEMPORARY)
//
// These aren't actually used. They are scripted in
// the registry files. Check out ms_AnimProperty in
// WRPGItem.cpp for a complete list of keys available
// for scripting.
enum
{
	AG_EQUIPPEDITEMCLASS_UNDEFINED	= 0,
	AG_EQUIPPEDITEMCLASS_BAREHANDS	= 1,
	AG_EQUIPPEDITEMCLASS_GUN		= 2,
	AG_EQUIPPEDITEMCLASS_SHOTGUN	= 4,
	AG_EQUIPPEDITEMCLASS_ASSAULT	= 8,
	AG_EQUIPPEDITEMCLASS_MINGUN		= 16,
	AG_EQUIPPEDITEMCLASS_KNUCKLEDUSTER	= 32,
	AG_EQUIPPEDITEMCLASS_SHANK		= 64,
	AG_EQUIPPEDITEMCLASS_CLUB		= 128,
	AG_EQUIPPEDITEMCLASS_GRENADE	= 256,
	AG_EQUIPPEDITEMCLASS_TRANQ		= 512,

	AG_EQUIPPEDITEMCLASS_NUMCLASSES	= 10,
};

// JUST HELPERS
enum
{
	AG_ITEMSLOT_NONE		= -1,
	AG_ITEMSLOT_WEAPONS		= 0,
	AG_ITEMSLOT_ITEMS		= 1,
	AG_ITEMSLOT_UNARMED		= 2,  // humm?

	AG_ITEMACTIVATION_NORMAL	= 0,
	AG_ITEMACTIVATION_FAIL		= 1,
	AG_ITEMACTIVATION_RELOAD	= 2,
};

enum
{
	//AG_FIGHTMODE_ATTACKACTIVE		= 1 << 0,
	//AG_FIGHTMODE_HURTACTIVE		= 1 << 1, // ie the character can be hurt from opponent
	//AG_FIGHTMODE_BLOCKACTIVE		= 1 << 2,
	AG_FIGHTMODE_CANINTERRUPT		= 1 << 3,
	AG_FIGHTMODE_CANBLOCK			= 1 << 4,
	AG_FIGHTMODE_CANGUARD			= 1 << 5,
	AG_FIGHTMODE_CANDONORMALMOVE	= 1 << 6,
	AG_FIGHTMODE_CANATTACK			= 1 << 7,
	AG_FIGHTMODE_CANDODGEBACK		= 1 << 8,
	AG_FIGHTMODE_CANDODGELEFT		= 1 << 9,
	AG_FIGHTMODE_CANDODGERIGHT		= 1 << 10,
	

	AG_FIGHTMODE_PREPHASEMASK		= (AG_FIGHTMODE_CANINTERRUPT  |
									   AG_FIGHTMODE_CANBLOCK | AG_FIGHTMODE_CANGUARD | 
									   AG_FIGHTMODE_CANDODGEBACK |AG_FIGHTMODE_CANDODGELEFT | 
									   AG_FIGHTMODE_CANDODGERIGHT),
	AG_FIGHTMODE_POSTPHASEMASK		= 0,


	AG_FIGHTMODE_ATTACKMOVEMASK		= (AG_FIGHTMODE_CANINTERRUPT | AG_FIGHTMODE_CANBLOCK | 
									AG_FIGHTMODE_CANGUARD | AG_FIGHTMODE_CANDODGEBACK |
									AG_FIGHTMODE_CANDODGELEFT | AG_FIGHTMODE_CANDODGERIGHT),
	
	AG_FIGHTMODE_OTHERFLAGS			= (AG_FIGHTMODE_CANDONORMALMOVE | AG_FIGHTMODE_CANATTACK),

	AG_FIGHTMODE_DAMAGELIGHT			= 1,
	AG_FIGHTMODE_DAMAGEMEDIUM			= 2,
	AG_FIGHTMODE_DAMAGEHEAVY			= 3,

	AG_FIGHTMODE_STAMINADRAIN_LOW		= 1,
	AG_FIGHTMODE_STAMINADRAIN_MEDIUM	= 2,
	AG_FIGHTMODE_STAMINADRAIN_HIGH		= 3,

	AG_FIGHTMODE_STAMINATARGET_SELF		= 0,
	AG_FIGHTMODE_STAMINATARGET_OPPONENT	= 1,


	AG_MESSAGESENDER_SETGHOSTMODE			= 1,
	AG_MESSAGESENDER_KILLYOURSELF			= 2,
	AG_MESSAGESENDER_SWITCHLEDGE			= 3,
	AG_MESSAGESENDER_FISTSETSHOULDBEDEAD	= 4,
	AG_MESSAGESENDER_SETFORCECROUCH			= 5,
	AG_MESSAGESENDER_INVALIDATEMOVE			= 6,
	// 7-9... removed
	AG_MESSAGESENDER_KILLME					= 10,
	AG_MESSAGESENDER_DROPBODY				= 11,
	AG_MESSAGESENDER_SETCROUCH				= 12,
	AG_MESSAGESENDER_DRAGBODYSOUND			= 13,
	AG_MESSAGESENDER_TERMINATETOKENS		= 14,
	AG_MESSAGESENDER_DROPWEAPONS			= 15,
	AG_MESSAGESENDER_SWITCHWEAPONS			= 16,
	AG_MESSAGESENDER_CROUCH					= 17,
	AG_MESSAGESENDER_RESETUNEQUIPCOUNTER	= 18,

	AG_LEDGEMODE_TYPEHIGH				= 0,
	AG_LEDGEMODE_TYPEMEDIUM				= 1,
	AG_LEDGEMODE_TYPELOW				= 2,
	AG_LEDGEMODE_TYPEENTERFROMABOVE		= 3,
	AG_LEDGETYPE_BADLEDGEENTERDIRECT	= 4,

	AG_PICKUP_TYPEUSE					= 0,
	AG_PICKUP_TYPEFIGHT					= 1,
	AG_PICKUP_TYPELEDGE					= 2,
	
	AG_FIGHTMODE_STANCEMOVETYPEFWD		= 1,
	AG_FIGHTMODE_STANCEMOVETYPEBWD		= 2,
	AG_FIGHTMODE_STANCEMOVETYPENONE		= 0,

	AG_FIGHTMODE_SOUNDTYPE_RIGHT		= 1,
	AG_FIGHTMODE_SOUNDTYPE_LEFT			= 2,
	AG_FIGHTMODE_SOUNDTYPE_MIDDLE		= 3,
	AG_FIGHTMODE_SOUNDTYPE_SPECIAL		= 4,
	AG_FIGHTMODE_SOUNDTYPE_SURFACE		= 5,
	AG_FIGHTMODE_SOUNDTYPE_BLOCK		= 6,

	

	AG_FIGHTMODE_INTERRUPT_MOVESTANCECLOSE		= 1,
	AG_FIGHTMODE_INTERRUPT_MOVESTANCENEUTRAL	= 2,
	AG_FIGHTMODE_INTERRUPT_MOVESTANCESAFE		= 3,
	AG_FIGHTMODE_INTERRUPT_HURTHEAD				= 4,
	AG_FIGHTMODE_INTERRUPT_HURTBODY				= 5,
	AG_FIGHTMODE_INTERRUPT_BREAKGRAB			= 6,

	AG_FIGHTMODE_MODE_NOOPPONENT				= 0,
	AG_FIGHTMODE_MODE_CLOSE						= 1,
	AG_FIGHTMODE_MODE_MEDIUM					= 2,
	AG_FIGHTMODE_MODE_SNEAK						= 3,

	AG_LADDERENDPOINT_MODE1						= 1,
	AG_LADDERENDPOINT_LEFT						= 4,
	AG_LADDERENDPOINT_RIGHT						= 5,

	AG_TOKEN_MAIN								= 0,
	AG_TOKEN_RESPOSE							= 1,
	AG_TOKEN_EFFECT								= 2,
	AG_TOKEN_UNEQUIP							= 3,
	AG_TOKEN_WALLCOLLISION						= 4,
	AG_TOKEN_DIALOG								= 8,


	AG_FIGHTSIGNAL_DAMAGE						= 1,
	AG_FIGHTSIGNAL_COUNTER						= 2,
	AG_FIGHTSIGNAL_SNEAKGRAB					= 3,
	AG_FIGHTSIGNAL_WEAPONMELEE					= 4,
	AG_FIGHTSIGNAL_WEAPONEXCHANGE				= 5,
	AG_FIGHTSIGNAL_RESETCOMBOCOUNT				= 6,
	AG_FIGHTSIGNAL_INCREASECOMBOCOUNT			= 7,
	AG_FIGHTSIGNAL_BREAKOFFGRAB					= 8,
	AG_FIGHTSIGNAL_CLEARAIMELEE					= 9,
	AG_FIGHTSIGNAL_CLEARAIMELEECOMBO			= 10,
	AG_FIGHTSIGNAL_PUSHDRAGDOLL					= 11,
	AG_FIGHTSIGNAL_RUMBLE						= 12,
	AG_FIGHTSIGNAL_GRENADEMELEE					= 13,
	AG_FIGHTSIGNAL_BLOOD						= 14,
	AG_FIGHTSIGNAL_BLOOD2						= 15,
	AG_FIGHTSIGNAL_BLOOD3						= 16,

	AG_FIGHT_COMBOSNEEDEDFORCOUNTER				= 3,

	AG_RUMBLE_SNEAK_RESPONSE					= 35,

	AG_COUNTERTYPE_UNARMED_RIGHT				= 5,
	AG_COUNTERTYPE_UNARMED_LEFT					= 9,
	AG_COUNTERTYPE_UNARMED_MIDDLE				= 13,
	AG_COUNTERTYPE_CLUB_MIDDLE					= 14,
	AG_COUNTERTYPE_SHANK_MIDDLE					= 15,
	AG_COUNTERTYPE_PISTOL						= 16,
	AG_COUNTERTYPE_ASSAULT						= 17,

	AG_FIGHTDAMAGE_TYPE_HURTLEFT				= 6,
	AG_FIGHTDAMAGE_TYPE_HURTRIGHT				= 7,
	AG_FIGHTDAMAGE_TYPE_HURTMIDDLE				= 8,
	AG_FIGHTDAMAGE_TYPE_HURTSPECIALUNARMED		= 9,
	AG_FIGHTDAMAGE_TYPE_HURTSPECIALSHANK		= 10,
	AG_FIGHTDAMAGE_TYPE_HURTSPECIALCLUB			= 11,

	AG_FIGHTDAMAGE_TYPE_ABUSEBODYKICK			= 12,
	AG_FIGHTDAMAGE_TYPE_ABUSEBODYSTOMP			= 13,
	AG_FIGHTDAMAGE_TYPE_ABUSEBODYCLUB			= 14,
	AG_FIGHTDAMAGE_TYPE_ABUSEBODYPOKE			= 15,


	AG_CLEARINTERRUPT_TYPE_PENDING				= 0,
	AG_CLEARINTERRUPT_TYPE_SENT					= 1,

	AG_FIGHTATTACK_TYPE_UNARMEDLEFT				= 1,
	AG_FIGHTATTACK_TYPE_UNARMEDRIGHT,
	AG_FIGHTATTACK_TYPE_UNARMEDMIDDLE,
	AG_FIGHTATTACK_TYPE_UNARMEDSPECIAL,
	AG_FIGHTATTACK_TYPE_SHANKLEFT,
	AG_FIGHTATTACK_TYPE_SHANKRIGHT,
	AG_FIGHTATTACK_TYPE_SHANKMIDDLE,
	AG_FIGHTATTACK_TYPE_SHANKSPECIAL,
	AG_FIGHTATTACK_TYPE_CLUBLEFT,
	AG_FIGHTATTACK_TYPE_CLUBRIGHT,
	AG_FIGHTATTACK_TYPE_CLUBMIDDLE,
	AG_FIGHTATTACK_TYPE_CLUBSPECIAL,
	AG_FIGHTATTACK_TYPE_PISTOL,
	AG_FIGHTATTACK_TYPE_ASSAULT,

	AG_PHYSWIDTH_FIGHTING						= 18,

};

enum
{
	AG_RESPONSE_TYPE_DAMAGE						= 1 << 0,
	AG_RESPONSE_TYPE_DEATH						= 1 << 1,
	AG_RESPONSE_TYPE_CONTROLMODE				= 1 << 2,
	AG_RESPONSE_TYPE_BEHAVIOR					= 1 << 3,
	AG_RESPONSE_TYPE_PERSISTENT_0				= 1 << 4,
	AG_RESPONSE_TYPE_PERSISTENT_1				= 1 << 5,
	AG_RESPONSE_TYPE_PERSISTENT_2				= 1 << 6,
	AG_RESPONSE_TYPE_PERSISTENT_3				= 1 << 7,
};

enum
{
	AG_BEHAVIOR_SIGNAL_ENDOFBEHAVIOR			= 1,
	AG_BEHAVIOR_SIGNAL_STARTOFBEHAVIOR			= 2,
};

enum
{
	SELECTION_NONE = 0,
	SELECTION_CHAR,
	SELECTION_PICKUP,
	SELECTION_ACTIONCUTSCENE,
	SELECTION_LADDER,
	SELECTION_LEDGE,
	SELECTION_HANGRAIL,
	SELECTION_DEADCHAR,
	SELECTION_ACTIONCUTSCENELOCKED,
	SELECTION_STEALTHTARGET,
	SELECTION_USABLEOBJECT,

	SELECTION_MASK_TYPE = 0x1f,
	SELECTION_MASK_TYPEINVALID = 0x3f,

	SELECTION_FLAG_INVALID = 0x20,
	SELECTION_FLAG_PROXY = 0x40, // Can't use 0x80 until converted SelectionType to uint8
};

enum
{
	REGISTERBEHAVIOR_RESET		= 0,
	REGISTERBEHAVIOR_ADD		= 1,
	REGISTERBEHAVIOR_REMOVE		= 2,
};
enum
{
	ANIMPHYSMOVETYPE_NONE		= 0,
	ANIMPHYSMOVETYPE_FORWARD	= 1,
	ANIMPHYSMOVETYPE_BACKWARD	= 2,
	ANIMPHYSMOVETYPE_LEFT		= 3,
	ANIMPHYSMOVETYPE_RIGHT		= 4,
	ANIMPHYSMOVETYPE_RESET		= 5,

	ANIMPHYSMOVETYPECONSTANTID	= 4,
};

enum
{
	EXPLORE_ACTIVE_UNDEFINED	= 0,
	EXPLORE_ACTIVE_FWD			= 1,
	EXPLORE_ACTIVE_LEFT45		= 2,
	EXPLORE_ACTIVE_LEFT90		= 3,
	EXPLORE_ACTIVE_LEFT135		= 4,
	EXPLORE_ACTIVE_LEFT180		= 5,

	EXPLORE_ACTIVE_RIGHT45		= 6,
	EXPLORE_ACTIVE_RIGHT90		= 7,
	EXPLORE_ACTIVE_RIGHT135		= 8,
	EXPLORE_ACTIVE_RIGHT180		= 9,
};

enum
{
	WALKPROPERTY_NONE			= 0,
	WALKPROPERTY_FORWARD		= 1,
	WALKPROPERTY_BACKWARD		= 2,
	WALKPROPERTY_STRAFELEFT		= 3,
	WALKPROPERTY_STRAFERIGHT	= 4,
};

enum
{
	AG_CONSTANT_ANIMMOVESCALE		= 0,
	AG_CONSTANT_ANIMROTSCALE		= 1,
	//AG_CONSTANT_DONTAPPLYGRAVITY	= 2,
	
	AG_CONSTANT_ANIMPHYSMOVETYPE	= 4,
	AG_CONSTANT_MAXBODYOFFSET		= 5,
	AG_CONSTANT_TURNSCALE			= 7,
	AG_CONSTANT_AIMINGTYPE			= 9,
	AG_CONSTANT_TURNTHRESHOLD		= 10,
	AG_CONSTANT_CANCOUNTER			= 11,
	AG_CONSTANT_CURRENTATTACK		= 12,
	AG_CONSTANT_MAXLOOKANGLEZ		= 13,
	AG_CONSTANT_MAXLOOKANGLEY		= 14,
	AG_CONSTANT_MAXTURNANGLE		= 15,
};

#define Explore_TurnAngleFwd			(0.0f)
#define Explore_TurnAngleLeft45			(0.875f)
#define Explore_TurnAngleLeft90			(0.75f)
#define Explore_TurnAngleLeft135		(0.625f)
#define Explore_TurnAngleLeft180		(0.5f)
#define Explore_TurnAngleMaxRangeLeft	(0.125f)
#define Explore_HalfTurnAngleMaxRangeLeft (0.0625f)
#define Explore_HalfTurnAngleMaxRangeLeft4 (0.125f)


#define Explore_TurnAngleRight45		(0.125f)
#define Explore_TurnAngleRight90		(0.25f)
#define Explore_TurnAngleRight135		(0.375f)
#define Explore_TurnAngleMaxRangeRight	(0.125f)
#define Explore_HalfTurnAngleMaxRangeRight (0.0625f)
#define Explore_HalfTurnAngleMaxRangeRight4 (0.125f)

// Some predefined factors for m_Vigilance
// (Please, pay no attention to their similarity to alertness states :))
// VIGILANCE_HOSTILE: Searching, wary, hostile
// VIGILANCE_COMBAT: Combat, panic
#define VIGILANCE_OBLIVIOUS			0.00f
#define VIGILANCE_DROWSY			0.25f
#define VIGILANCE_IDLE				0.50f
//#define VIGILANCE_SEARCH			0.60f
#define VIGILANCE_WARY				0.75f
#define VIGILANCE_COMBAT			1.00f
//#define VIGILANCE_FRIGHTENED		2.00f

#define FP32TRUE 1.0f
#define FP32FALSE 0.0f

#define MOVEANGLETHRESHOLD			(0.08f)
#define MELEEDAMAGERANGE			(50.0f)

#define MACRO_ADJUSTEDANGLE(p) (p < 0.0f ? p + 1.0f + M_Floor(-p) : (p > 1.0f ? p - M_Floor(p) : p))
#ifndef __MACROINLINEACCESS
#define __MACROINLINEACCESS
#define MACRO_INLINEACCESS_RW(name, type) \
	M_INLINE const type& Get##name() const { return m_##name; } \
	M_INLINE type& Get##name() { return m_##name; }

#define MACRO_INLINEACCESS_R(name, type) \
	M_INLINE const type& Get##name() const { return m_##name; }

#define MACRO_INLINEACCESS_RWEXT(name, variable, type) \
	M_INLINE const type& Get##name() const { return variable; } \
	M_INLINE type& Get##name() { return variable; }

#define MACRO_INLINEACCESS_REXT(name, variable, type) \
	M_INLINE const type& Get##name() const { return variable; }
#endif

//--------------------------------------------------------------------------------

#define MAX_ANIMGRAPH_CONDITIONS	110
#define MAX_ANIMGRAPH_PROPERTIES	110
#define MAX_ANIMGRAPH_OPERATORS		8
#define MAX_ANIMGRAPH_EFFECTS		32

//--------------------------------------------------------------------------------

class CFightDamageQueue
{
	int32 m_Tick;
	int32 m_DamageType;
public:
	CFightDamageQueue();
	void Create(int32 _Tick, int32 _DamageType);
	M_INLINE void Invalidate()
	{
		m_Tick = -1;
	}
	void Refresh(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, int32 _CurrentTick);
};
class CWO_Character_ClientData;
class CWO_Clientdata_Character_AnimGraph : public CWO_ClientData_AnimGraphInterface
{
	friend class CWO_Character_ClientData;
	public:
		//CHAR_ATTACKTYPE -> CONTROLBITS map
		static int GetControlBits(int _AttackType);

	private:

		// Make type checking work
		typedef bool (CWO_Clientdata_Character_AnimGraph::*PFN_ANIMGRAPH_CONDITION)(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams, int _iOperator, const CAGVal &_Constant, fp32& _TimeFraction);
		typedef CAGVal (CWO_Clientdata_Character_AnimGraph::*PFN_ANIMGRAPH_PROPERTY)(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		typedef bool (CWO_Clientdata_Character_AnimGraph::*PFN_ANIMGRAPH_OPERATOR)(const CWAGI_Context* _pContext, const CAGVal&, const CAGVal&);
		typedef void (CWO_Clientdata_Character_AnimGraph::*PFN_ANIMGRAPH_EFFECT)(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		static PFN_ANIMGRAPH_CONDITION ms_lpfnConditions_Server[MAX_ANIMGRAPH_CONDITIONS];
		static PFN_ANIMGRAPH_PROPERTY ms_lpfnProperties_Server[MAX_ANIMGRAPH_PROPERTIES];
		static PFN_ANIMGRAPH_OPERATOR ms_lpfnOperators_Server[MAX_ANIMGRAPH_OPERATORS];
		static PFN_ANIMGRAPH_EFFECT ms_lpfnEffects_Server[MAX_ANIMGRAPH_EFFECTS];

	public:
		
		void AG_RegisterCallbacks(CWorld_PhysState* _pWPhysState)
		{
			if (_pWPhysState->IsServer())
				AG_RegisterCallbacks2((::PFN_ANIMGRAPH_CONDITION *)ms_lpfnConditions_Server, MAX_ANIMGRAPH_CONDITIONS,
									  (::PFN_ANIMGRAPH_PROPERTY *)ms_lpfnProperties_Server, MAX_ANIMGRAPH_PROPERTIES,
									  (::PFN_ANIMGRAPH_OPERATOR *)ms_lpfnOperators_Server, MAX_ANIMGRAPH_OPERATORS,
									  (::PFN_ANIMGRAPH_EFFECT *)ms_lpfnEffects_Server, MAX_ANIMGRAPH_EFFECTS);
			else
				AG_RegisterCallbacks2((::PFN_ANIMGRAPH_CONDITION *)ms_lpfnConditions_Server, MAX_ANIMGRAPH_CONDITIONS,
									  (::PFN_ANIMGRAPH_PROPERTY *)ms_lpfnProperties_Server, MAX_ANIMGRAPH_PROPERTIES,
									  (::PFN_ANIMGRAPH_OPERATOR *)ms_lpfnOperators_Server, MAX_ANIMGRAPH_OPERATORS,
									  (::PFN_ANIMGRAPH_EFFECT *)ms_lpfnEffects_Server, MAX_ANIMGRAPH_EFFECTS);
		}

	protected:

		// FIX FOR AG bug? (Calls function twice sometimes in the olden days)
		//CFightDamageQueue		m_FightDamageQueue;
		int32					m_LastFightDamage;
		int32					m_LastCrouchAttempt;

		fp32						m_MoveVeloHoriz, m_MoveVeloVert, m_MoveAngleUnit, m_MoveAngleUnitS, m_LastMoveSpeedSqr;
		fp32						m_MoveRadiusControl, m_MoveAngleUnitControl, m_MoveAngleUnitSControl;
		fp32						m_MoveHControl, m_MoveVControl;

		int32					m_iTarget;
		fp32						m_DistanceToTarget;
		fp32						m_TurnAngleToTarget;

		fp32						m_AGMaxHealth;
		fp32						m_AGHealth;

		// Stateflags
		int32					m_StateFlagsLo;
		int32					m_StateFlagsHi;
		int32					m_StateFlagsLoToken2;
		int32					m_StateFlagsLoToken3;

		// A value from 0.0 - 1.0 showing the alertness of the character, used by animgraph to
		// determine stances etc. NOT the same as AI alertness.
		fp32						m_Vigilance;
		fp32						m_EventTrigger;
		fp32						m_LeftTrigger;
		fp32						m_RightTrigger;

		fp32						m_OnHit_Damage;

		uint16					m_OnHit_DamageType;
		// General purpuse AI<->AG bitfield
		uint16					m_GPBitField;
		// The AI's wanted behavior
		uint16					m_WantedBehavior;

		int8					m_ForcedAimingType;
		uint8					m_AIMoveMode;

		uint8					m_UsedButtonPress;
		uint8					m_ButtonPress;
		uint8					m_JoyPad;
		uint8					m_OnHit_BodyPart;
		uint8					m_OnHit_Direction;
		uint8					m_OnHit_Force;
		
		uint8					m_PendingFightInterrupt;
		uint8					m_LastSentFightInterrupt;
		uint8					m_CanCounterType;
		uint8					m_CurrentAttack;
		uint8					m_ConstantMaxLookZ;
		uint8					m_ConstantMaxLookY;

		// Animphysmovetype
		uint8					m_AnimphysMoveType;

		uint8					m_IdleTurnTimer;
		uint8					m_IdleTurnThreshold;

		// Maxbody angle offset from look
		uint8					m_MaxBodyOffset;

		int8					m_ActionPressCount;
		// Counts amount of fight combos, at 3 -> automatic counter
		uint8					m_ComboCount;

		// Bitfield for various flags.
		uint8					m_bIsSleeping : 1;
		uint8					m_DisableRefresh : 2;
		uint8					m_bUseSmallHurt : 1;
		uint8					m_bQueueWeaponSelect : 1;

		//The invantory index of the weapon we should switch to, or -1 if we shouldn't switch at all
		int32					m_WeaponIdentifier;

	public:

		CWO_Clientdata_Character_AnimGraph()
		{
			Clear();
		}

		void Clear()
		{
			CWO_ClientData_AnimGraphInterface::Clear();

			m_MoveVeloHoriz = 0;
			m_MoveVeloVert = 0;
			m_MoveAngleUnit = 0;
			m_MoveAngleUnitS = 0;
			m_LastMoveSpeedSqr = 0;

			m_MoveRadiusControl = 0;
			m_MoveAngleUnitControl = 0;
			m_MoveAngleUnitSControl = 0;
			m_MoveHControl = 0;
			m_MoveVControl = 0;
		
			m_iTarget = 0;
			m_DistanceToTarget = 0;
			m_TurnAngleToTarget = 0;
			m_AIMoveMode = 0;

			m_AGHealth = 0;
			m_AGMaxHealth = 1;
			m_Vigilance = VIGILANCE_OBLIVIOUS;
			m_StateFlagsLo = 0;
			m_StateFlagsHi = 0;
			m_StateFlagsLoToken2 = 0;
			m_StateFlagsLoToken3 = 0;

			m_EventTrigger = 0;

			m_UsedButtonPress = 0;
			m_ButtonPress = 0;
			m_JoyPad = 0;
			m_LeftTrigger = 0;
			m_RightTrigger = 0;

			m_OnHit_BodyPart = 0;
			m_OnHit_Direction = 0;
			m_OnHit_Force = 0;
			m_OnHit_DamageType = 0;
			m_OnHit_Damage = 0;

			m_PendingFightInterrupt = 0;
			m_LastSentFightInterrupt = 0;
			m_CanCounterType = 0;
			m_CurrentAttack = 0;
			m_ConstantMaxLookZ = 0;
			m_ConstantMaxLookY = 0;

			m_WantedBehavior = 0;
			m_AnimphysMoveType = 0;
			m_ForcedAimingType = -1;
			m_IdleTurnTimer = 0;
			m_IdleTurnThreshold = 0;

			m_GPBitField = 0;
			m_MaxBodyOffset = 0;
			m_ActionPressCount = 0;

			m_LastFightDamage = 0;
			m_LastCrouchAttempt = 0;

			m_bIsSleeping = 0;
			m_DisableRefresh = 0;
			m_bUseSmallHurt = 0;
			m_bQueueWeaponSelect = 0;

			m_WeaponIdentifier = 0;
		}

		void Copy(const CWO_Clientdata_Character_AnimGraph& _CD)
		{
			MSCOPESHORT(CWO_Clientdata_Character_AnimGraph::Copy);
			CWO_ClientData_AnimGraphInterface::Copy(_CD);

			//m_FightDamageQueue = _CD.m_FightDamageQueue;
			m_MoveVeloHoriz = _CD.m_MoveVeloHoriz;
			m_MoveVeloVert = _CD.m_MoveVeloVert;
			m_MoveAngleUnit = _CD.m_MoveAngleUnit;
			m_MoveAngleUnitS = _CD.m_MoveAngleUnitS;
			m_LastMoveSpeedSqr = _CD.m_LastMoveSpeedSqr;

			m_MoveRadiusControl = _CD.m_MoveRadiusControl;
			m_MoveAngleUnitControl = _CD.m_MoveAngleUnitControl;
			m_MoveAngleUnitSControl = _CD.m_MoveAngleUnitSControl;

			m_MoveHControl = _CD.m_MoveHControl;
			m_MoveVControl = _CD.m_MoveVControl;

			m_iTarget = _CD.m_iTarget;
			m_DistanceToTarget = _CD.m_DistanceToTarget;
			m_TurnAngleToTarget = _CD.m_TurnAngleToTarget;
			m_AIMoveMode = _CD.m_AIMoveMode;
			
			m_AGHealth = _CD.m_AGHealth;
			m_AGMaxHealth = _CD.m_AGMaxHealth;
			m_Vigilance = _CD.m_Vigilance;
			m_StateFlagsLo = _CD.m_StateFlagsLo;
			m_StateFlagsHi = _CD.m_StateFlagsHi;
			m_StateFlagsLoToken2 = _CD.m_StateFlagsLoToken2;
			m_StateFlagsLoToken3 = _CD.m_StateFlagsLoToken3;
			m_EventTrigger = _CD.m_EventTrigger;

			m_UsedButtonPress = _CD.m_UsedButtonPress;
			m_ButtonPress = _CD.m_ButtonPress;
			m_JoyPad = _CD.m_JoyPad;
			m_LeftTrigger = _CD.m_LeftTrigger;
			m_RightTrigger = _CD.m_RightTrigger;

			m_OnHit_BodyPart = _CD.m_OnHit_BodyPart;
			m_OnHit_Direction = _CD.m_OnHit_Direction;
			m_OnHit_Force = _CD.m_OnHit_Force;
			m_OnHit_DamageType = _CD.m_OnHit_DamageType;
			m_OnHit_Damage = _CD.m_OnHit_Damage;

			m_PendingFightInterrupt = _CD.m_PendingFightInterrupt;
			m_LastSentFightInterrupt = _CD.m_LastSentFightInterrupt;
			m_CanCounterType = _CD.m_CanCounterType;
			m_CurrentAttack = _CD.m_CurrentAttack;
			m_ConstantMaxLookZ = _CD.m_ConstantMaxLookZ;
			m_ConstantMaxLookY = _CD.m_ConstantMaxLookY;

			m_WantedBehavior = _CD.m_WantedBehavior;
			m_AnimphysMoveType = _CD.m_AnimphysMoveType;
			m_ForcedAimingType = _CD.m_ForcedAimingType;
			m_IdleTurnTimer = _CD.m_IdleTurnTimer;
			m_IdleTurnThreshold = _CD.m_IdleTurnThreshold;

			m_GPBitField = _CD.m_GPBitField;
			m_MaxBodyOffset = _CD.m_MaxBodyOffset;

			m_ActionPressCount = _CD.m_ActionPressCount;

			m_LastFightDamage = _CD.m_LastFightDamage;
			m_LastCrouchAttempt = _CD.m_LastCrouchAttempt;

			m_bIsSleeping = _CD.m_bIsSleeping;
			m_DisableRefresh = _CD.m_DisableRefresh;
			m_bUseSmallHurt = _CD.m_bUseSmallHurt;
			m_bQueueWeaponSelect = _CD.m_bQueueWeaponSelect;

			m_WeaponIdentifier = _CD.m_WeaponIdentifier;
		}

		virtual void AG_RefreshStateInstanceProperties(const CWAGI_Context* _pContext, const CWAGI_StateInstance* _pStateInstance);
		virtual void AG_OnEnterState(const CWAGI_Context* _pContext, CAGTokenID _TokenID, CAGStateIndex _iState, CAGActionIndex _iEnterAction);

		virtual CWAGI* GetAGI() pure;
		virtual const CWAGI* GetAGI() const pure;
	public:
		void RefreshPress(uint32 _PressedBits);

		void OnHit(const CWO_DamageMsg& _DX, CWObject_CoreData* _pObj);
		void OnHit_Clear();

		M_INLINE int GetTargetID() { return m_iTarget; }
		M_INLINE uint16 GetWantedBehavior() { return m_WantedBehavior; }
		void SetWantedBehavior(uint16 _WantedBehavior) { m_WantedBehavior = _WantedBehavior; m_ImportantAGEvent = 0; }
		void SetDisableRefresh(uint8 _Disable) { m_DisableRefresh = _Disable; }
		M_INLINE uint8 GetDisableRefresh() { return m_DisableRefresh; }
		void SetPendingFightInterrupt(uint8 _Interrupt) { m_PendingFightInterrupt = _Interrupt; m_ImportantAGEvent = 0; }
		void SetLastSentFightInterrupt(uint8 _Interrupt) { m_LastSentFightInterrupt = _Interrupt; }
		void SetAIMoveMode(uint8 _Mode) { m_AIMoveMode = _Mode; }
		M_INLINE void ResetActionPressCount() { m_ActionPressCount = 0; }
		M_INLINE void IncreaseActionPressCount() { if (m_ActionPressCount < 100) m_ActionPressCount++; }
		M_INLINE int8 GetActionPressCount() { return m_ActionPressCount; }
		M_INLINE void SetActionPressCount(int8 _Count) { m_ActionPressCount = _Count; }
		M_INLINE void SetWeaponIdentifier(int8 _Identifier = 0) { m_WeaponIdentifier = _Identifier; };
		M_INLINE int8 GetWeaponIdentifier() { return m_WeaponIdentifier; };
		M_INLINE bool GetQueueWeaponSelect() { return m_bQueueWeaponSelect; };

		void AddHandledResponse(uint8 _Response) { m_HandledResponse |= _Response; }

		M_INLINE int32 GetStateFlagsLo() { return m_StateFlagsLo; }
		M_INLINE int32 GetStateFlagsHi() { return m_StateFlagsHi; }
		M_INLINE int32 GetStateFlagsLoToken2() { return m_StateFlagsLoToken2; }
		M_INLINE int32 GetStateFlagsLoToken3() { return m_StateFlagsLoToken3; }
		M_INLINE uint8 GetAnimPhysMoveType() { return m_AnimphysMoveType; }
		M_INLINE int8 GetForcedAimingType() { return m_ForcedAimingType; }
		M_INLINE void SetVigilance(fp32 _Vigilance) { if (m_Vigilance != _Vigilance) { m_Vigilance = _Vigilance; m_ImportantAGEvent = 0;}}
		M_INLINE fp32 GetVigilance() { return m_Vigilance; }
		M_INLINE uint8 GetCanCounterType() { return m_CanCounterType; }
		M_INLINE uint8 GetCurrentAttack() { return m_CurrentAttack; }
		M_INLINE uint8 GetConstantMaxLookZ() { return m_ConstantMaxLookZ; }
		M_INLINE uint8 GetConstantMaxLookY() { return m_ConstantMaxLookY; }

		M_INLINE const uint16& GetGPBitField() const { return m_GPBitField; }
		M_INLINE uint16& GetGPBitField() { return m_GPBitField; }
		M_INLINE void SetGPBitField(uint16 _Field) { m_GPBitField = _Field; }

		M_INLINE uint8& GetButtonPress() { return m_ButtonPress; }
		M_INLINE const uint8& GetButtonPress() const { return m_ButtonPress; }

		M_INLINE void QueueWeaponSelect(bool _bQueue = true) { m_bQueueWeaponSelect = _bQueue; }


		M_INLINE const uint8& GetMaxBodyOffset() const { return m_MaxBodyOffset; }

		M_INLINE uint8 GetIsSleeping() const { return m_bIsSleeping; }
		
		M_INLINE bool CanEquip() const { return !((m_StateFlagsLo |m_StateFlagsLoToken2 | m_StateFlagsLoToken3) & CHAR_STATEFLAG_EQUIPPING); }

		M_INLINE uint8 GetUseSmallHurt() const { return m_bUseSmallHurt; }
		M_INLINE void SetUseSmallHurt(uint8 _bUse) { m_bUseSmallHurt = _bUse; }
		M_INLINE fp32 GetRelativeHealth() const { return m_AGHealth/m_AGMaxHealth; }
		//M_INLINE const CFightDamageQueue& GetFightDamageQueue() const { return m_FightDamageQueue; }
		//M_INLINE CFightDamageQueue& GetFightDamageQueue() { return m_FightDamageQueue; }

		bool GetStateConstantsRequestPtrs(CWAGI*& _pAGI, const CXRAG_State*& _pState);

		const fp32& GetMoveAngleUnitControl() const { return m_MoveAngleUnitControl; }
		const fp32& GetMoveAngleUnit() const { return m_MoveAngleUnit; }
		
		CAGVal Property_MoveVeloHoriz(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_MoveVeloHoriz); }
		CAGVal Property_MoveVeloVert(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_MoveVeloVert); }
		CAGVal Property_MoveAngle(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_MoveAngleUnit * 360.0f); }
		CAGVal Property_MoveAngleS(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From((m_MoveAngleUnitS * 360.0f)); }
		CAGVal Property_MoveAngleUnit(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_MoveAngleUnit); }
		CAGVal Property_MoveAngleUnitS(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_MoveAngleUnitS); }
		CAGVal Property_LastMoveSpeedSqr(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_LastMoveSpeedSqr); }

		CAGVal Property_MoveRadiusControl(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_MoveRadiusControl); }
		CAGVal Property_MoveAngleControl(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_MoveAngleUnitControl * 360.0f); }
		CAGVal Property_MoveAngleSControl(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From((m_MoveAngleUnitSControl * 360.0f)); }
		CAGVal Property_MoveAngleUnitControl(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_MoveAngleUnitControl); }
		CAGVal Property_MoveAngleUnitSControl(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_MoveAngleUnitSControl); }
		// Finds fixed angles in a circle divided into eight sections
		CAGVal Property_FindFixedMoveAngle8(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_FindFixedMoveAngle4(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_FindFixedMoveAngleControl4(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_FindFixedMoveAngleControl8(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		// Independent values from the joystick
		// Property_MoveHControl: Horizontal value (positive to the right [-1.0..1.0])
		// Property_MoveVControl: Vertical value (positive forward [-1.0..1.0])
		CAGVal Property_MoveHControl(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_MoveHControl); }
		CAGVal Property_MoveVControl(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_MoveVControl); }
		CAGVal Property_AIMoveMode(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_AIMoveMode); }

		CAGVal Property_MoveAngleHeavyGuard(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_WallCollision(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_IsServer(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(_pContext->m_pWPhysState->IsServer()); }

		// If lean is active
		CAGVal	Property_LeanActive(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_JoyPad(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_LeftTrigger(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_LeftTrigger); }
		CAGVal Property_RightTrigger(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_RightTrigger); }

		CAGVal	Property_IsFighting(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_IsCrouching(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_Health(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_AGHealth); }
		CAGVal Property_RelativeHealth(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_AGHealth/m_AGMaxHealth); }
		CAGVal Property_IsDead(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_IsSleeping(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_LedgeType(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		// Returns state of alertness: Oblivious/Dead=0, Sleep=0.25f, Drowsy=0.5f, Idle=0.75f, Watchful=1.0f, Jumpy=1.0f
		// NOTE: Currently they return Dead=0.0f, IDLESTANCE_DEFAULT= 0.5f, IDLESTANCE_WARY=0.75f, IDLESTANCE_HOSTILE=1.0f
		CAGVal Property_Vigilance(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_Vigilance); }

		CAGVal Property_HasTarget(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From((m_iTarget != 0)); };
		CAGVal Property_DistanceToTarget(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_DistanceToTarget); };
		CAGVal Property_TurnAngleToTarget(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_TurnAngleToTarget); };

		CAGVal Property_EventTrigger(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_EventTrigger); };

		CAGVal Property_GetControlmode(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_ActionCutsceneType(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_LadderEndPoint(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_HangrailCanGoForward(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_IsInAir(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_OnHit_BodyPart(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From((fp32)m_OnHit_BodyPart); }
		CAGVal Property_OnHit_Direction(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From((fp32)m_OnHit_Direction); }
		CAGVal Property_OnHit_Force(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From((fp32)m_OnHit_Force); }
		CAGVal Property_OnHit_DamageType(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From((fp32)m_OnHit_DamageType); }
		CAGVal Property_OnHit_Damage(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From((fp32)(m_OnHit_Damage)); }
		
		CAGVal Property_EquippedItemClass(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
	
		CAGVal Property_CanActivateItem(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_NeedReload(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_ButtonTest(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_PendingFightInterrupt(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_PendingFightInterrupt); }
		CAGVal Property_LastSentFightInterrupt(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_LastSentFightInterrupt); }

		CAGVal Property_CanBlock(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_CanEndACS(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_CanClimbUpLedge(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_NewFightTestMode(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_WantedBehavior(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_WantedBehaviorType(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_AngleDiff(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_AngleDiffS(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_FindFixedMoveAngleDiff8(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_FindFixedMoveAngleDiff5(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		// Which animation to use when walking
		CAGVal Property_FixedWalkAngle4(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		// Actioncutscene Lever state
		CAGVal Property_LeverState(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_ValveState(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_CanPlayEquip(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_IdleTurnTimer(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From((fp32)m_IdleTurnTimer); }

		// If fight target is active (focusframe object exists)
		CAGVal Property_HasFightTarget(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_HasTranqTarget(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_CanGotoFight(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_HangRailCheckHeight(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_LadderStepoffType(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_EffectPlaying(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		
		CAGVal Property_CanCounter(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_RelAnimQueued(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_CanPlayFightFakeIdle(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_GPBitFieldBit(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_FixedFightAngle4(const CWAGI_Context* _pContext,const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_BreakOffGrab(const CWAGI_Context* _pContext,const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_CanSwitchWeapon(const CWAGI_Context* _pContext,const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_CanFightStandUp(const CWAGI_Context* _pContext,const CXRAG_ICallbackParams* _pParams);

		CAGVal Property_MeleeeAttack(const CWAGI_Context* _pContext,const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_UseSmallHurt(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From((fp32)m_bUseSmallHurt); }

		void Effect_SelectItem(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		void Effect_ActivateItem(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		void Effect_DeactivateItem(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		// Pickup items, enter fightmode.....
		void Effect_PickupStuff(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		void Effect_RegisterUsedInput(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		void Effect_UnRegisterUsedInput(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		void Effect_SetControlMode(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		//void Effect_FightMove(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		void Effect_ErrorMsg(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		void Effect_ActionCutsceneSwitch(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		void Effect_SendImpulse(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		void Effect_DrainStamina(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		void Effect_MessageSender(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		void Effect_LedgeMove(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		void Effect_LadderMove(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		void Effect_ClearFightInterrupt(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		void Effect_ClearHit(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		void Effect_ResetControlmodeParams(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		void Effect_UnregisterHandledResponse(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		void Effect_AIBehaviorSignal(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		void Effect_AIRegisterBehavior(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		// Signal relanimmove (start/stop)
		void Effect_RelAnimMoveSignal(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		void Effect_FightSignal(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		void Effect_KillTokens(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		void Effect_ResetActionPressCount(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { m_ActionPressCount = 0; }
		
		// TEMP
		//void Effect_SendFightDamageNew(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		void Effect_PauseTokens(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

};

//--------------------------------------------------------------------------------

#endif /* AnimGraphClientData_Templar_h */
