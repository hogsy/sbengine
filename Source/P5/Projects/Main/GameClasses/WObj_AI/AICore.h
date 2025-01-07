#ifndef _INC_AICORE
#define _INC_AICORE

class CWObject_Character;
class CAI_ResourceHandler;

class CAI_Core;
typedef TPtr<CAI_Core> spCAI_Core;

class CAI_Behaviour;
typedef TPtr<CAI_Behaviour> spCAI_Behaviour;

class CAI_Personality;
typedef TPtr<CAI_Personality> spCAI_Personality;

class CAI_FollowPathData;
typedef TPtr<CAI_FollowPathData> spCAI_FollowPathData;

class CWObject_ScenePointManager;
#define CWO_ScenePointManager CWObject_ScenePointManager	// TODO: rename properly



#include "../WObj_Messages.h"
#include "AI_Def.h"

//Include auxiliaries
#include "AI_Auxiliary.h"
#include "AI_Action/AI_Action.h"
#include "AI_ItemHandler.h"
#include "AI_Personality.h"
#include "AI_ScriptHandler.h" 
#include "AI_EventHandler.h"
#include "AI_DeviceHandler.h"
#include "AI_Pathfinder.h"
#include "AI_KnowledgeBase.h"
#include "WObj_Aux/WObj_Team.h"
#include "../Wrpg/WRPGWeapon.h"

// Macros for checking if a CVec3Dfp32 is invalid or not (not stack object, no constructor call, one check)
#define INVALID_POS(p) (p[0] == _FP32_MAX)
#define VALID_POS(p) (p[0] != _FP32_MAX)
#define INVALIDATE_POS(p) (p = CVec3Dfp32(_FP32_MAX))

// ========================================================================================
// CWObj_LightMeter
// ========================================================================================
// A simple class that is used to sample lightlevels
// Usage:
// Create an instance with a CWObject* as an argument (it is used to access other parts of
// the engine and to get the object ID to exclude when doing tracings)
// Call Measure() to update lightdata
// Call GetIntensity() to get the current sum of light intensities

// LIGHTMETER_MAXLIGHTS is the maximum number of lights that will be considered
// LIGHTMETER_SLOP is the distance limit on how far _Pos may differ from earlier calls
// without a new collection of lights.
// LIGHTMETER_SNEAK_THRESHOLD
#define LIGHTMETER_MAXLIGHTS		8
#define LIGHTMETER_SLOP				4
#define LIGHTMETER_SNEAK_THRESHOLD	0.35f
#define LIGHTMETER_CORPSE_MIN		0.05f
#define SNEAK_MAXMOVE_THRESHOLD		5
class CWObj_LightMeter : public CReferenceCount
{
public:
	CWObj_LightMeter(CAI_Core* _pAI);
	~CWObj_LightMeter();
	// Measure() measures the light intersecting a box at _Pos with dimensions _Width, _Height
	// The _nLights tells us the number of lights to sample this call (-1 will sample all lights)
	// Returns true if all lights have been sampled at least once and false if not
	fp32 MeasureFakePointLight(const CVec3Dfp32& _BoxPos, const CVec3Dfp32& _MeasurePos, fp32 _Width, fp32 _Height, const CXR_Light& _Light, const bool _TraceLine, const int _iExclude0, const int _iExclude1);
	bool Measure(const CVec3Dfp32& _BoxPos,const CVec3Dfp32& _MeasurePos,fp32 _Width,fp32 _Height,int _nLights,bool _bTraceLines,int _iSkipObj);
	// Returns the current light intensity (this may be too dark if the last Measure() call returned false)
	fp32 GetIntensity(int _iObj);
	fp32 GetLightmapIntensity() {return(m_LightMapIntensity);}
	fp32 GetIntensityDelta() {return(m_LightIntensity - m_PrevLightIntensity);}
	// Returns true if the current itensity value is valid ie all lights have been measured for the current
	// position.
	bool IntensityValid();
	int	m_LastMeasureTick; 

protected:
	// Updates the m_LightIntensity and m_bIntensityValid flags
	void UpdateIntensity();

	CAI_Core* m_pAI;
	fp32 m_LightIntensity;		// Light measurement from 0 to 1.0
	fp32 m_PrevLightIntensity;	// Light measurement from 0 to 1.0
	int16 m_iOwnerFlashLight;	// Who is the owner of the flash
	fp32 m_FlashlightIntensity;	// Intensity of flash
	int16 m_nLights;		// Current number of valid lights in m_lpLights
	const CXR_Light *m_lpLights[LIGHTMETER_MAXLIGHTS];
	fp32 m_lpLightIntensities[LIGHTMETER_MAXLIGHTS];	// Intensities of lights in m_lpLights
	fp32 m_LightMapIntensity;
	bool m_bIntensityValid;	// Have all lights been measured
	CVec3Dfp32 m_Pos;		// Current position
};
typedef TPtr<CWObj_LightMeter>	spCWObj_LightMeter;


// ========================================================================================
// Engine path following
// ========================================================================================
// To reduce the amount of duplicated code for engine path following it resides in CAICore
class CAI_FollowPathData
{
public:
	//Following mode
	enum {
	FOLLOW_NORMAL	= 0,
	FOLLOW_FORCE	= 0x1,
	FOLLOW_FLY		= 0x2,
	FOLLOW_RELATIVE = 0x4,
	FOLLOW_TELEPORT = 0x8,
	FOLLOW_TRUE		= 0x10,
	};

	CAI_FollowPathData()
	{
		Clear();
	};

	void Clear()
	{
		m_iPath = 0;
		m_iFollowMode = FOLLOW_NORMAL;
		m_bFallenBehind = false;
		m_PathPosition = CVec3Dfp32(_FP32_MAX);
		m_bFlying = false;
		m_bTurnOnGravity = true;
		m_bTurnOnAnimControl = true;
		m_MovePosRel = CVec3Dfp32(_FP32_MAX);
		m_LastPropelTick = 0;
		m_bStop = false;
		m_LookVector = CVec3Dfp32(_FP32_MAX);
	};

	//Server index to engine path to follow.
	int m_iPath;
	int m_iFollowMode;
	//Keeps track of when we need to navigate our way back to the path
	bool m_bFallenBehind;
	//Keeps track of the position to go to when following path
	CVec3Dfp32 m_PathPosition;
	//Are we flying?
	bool m_bFlying;

	// *** Are these scripting specific members? ***
	//Should we turn on gravity when we change path following or clear script?
	bool m_bTurnOnGravity;
	//Should we turn on animation control when we change path following or clear script?
	bool m_bTurnOnAnimControl;
	// *** ?

	//The position to go to when following relative
	CVec3Dfp32 m_MovePosRel;
	//The last tick we propelled a path
	int m_LastPropelTick;
	//Are we at a stop in path?
	bool m_bStop;
	//Look vector when following path
	CVec3Dfp32 m_LookVector;
};

//Core AI functionality
class CAI_Core : public CReferenceCount
{
	MRTC_DECLARE;
public:
	//Constants

	// This constant is used situations where a phys state is not available
	// (typically in constructors).
	enum
	{
		AI_TICKS_PER_SECOND = 30,
	};
#define AI_TICK_DURATION	0.033333333333333333333333333333333f
// #define AI_PATHFIND_MICROSECONDS	1000.0f
#define AI_PATHFIND_MICROSECONDS	10000.0f
	//Behaviour types
	enum {
		PATROL		= ACTION_OVERRIDE_ENUM_BASE+1,
		BEHAVIOUR	= ACTION_OVERRIDE_ENUM_BASE+2,
		EXPLORE		= ACTION_OVERRIDE_ENUM_BASE+4,
		HOLD		= ACTION_OVERRIDE_ENUM_BASE+5,
		ENGAGE		= ACTION_OVERRIDE_ENUM_BASE+8,
	};

	//Action modes
	enum {
		CROUCHING	= 0x1,
		AIRBORNE	= 0x2,
		JUMPING		= 0x4,
		LOOKING		= 0x8,
		TAUNTING	= 0x10
	};

	// Movement modes
	enum {
		MOVEMODE_CROUCH = 0,
		MOVEMODE_STAND,
		MOVEMODE_WALK,
		MOVEMODE_LEDGE,
		MOVEMODE_HANGRAIL,
		MOVEMODE_LADDER,
		MOVEMODE_RUN
	};
	//Extra search statuses
	enum {
		//SEARCH_TIMED_OUT			= -1,
		//SEARCH_RESULT_INVALID		= -2,
		//SEARCH_INSTANCE_UNAVAILABLE	= -3,
		SEARCH_NO_PATH_POSITION		= -4,
		SEARCH_FOUND_PARTIAL_PATH	= -5,
		SEARCH_NONE					= -6,
	};

	//Arguments to "IsAttackingUs"
	enum {
		MELEE_ONLY = 1,
		RANGED_ONLY,
	};

	//Arguments to "PrecisionModifier"
	enum {
		ALL_MODIFIERS   = 0,
		TARGET_MOVEMENT = 0x1,
		OWN_MOVEMENT	= 0x2,
		VISIBILITY		= 0x4,
	};

	//Impulses (The values are specified in the nodetype.txt file for ogier, and must not be changed, for backwards compability reasons)
	enum {
		IMPULSE_RELEASE				= 0,//Bot is released from any script control
		IMPULSE_PAUSE				= 2,//Bot does nothing at all until otherwise ordered or released
			
		IMPULSE_FOLLOW_ME			= 1,//Bot follows sender until released
		IMPULSE_FORCE_FOLLOW_ME		= 3,//Bot always moves directly towards the sender
		IMPULSE_FLY_FOLLOW			= 8,//Bot moves normally to position of engine path, then jumps and flies along the engine path
		IMPULSE_RELATIVE_FORCE_FOLLOW = 9,//Bot moves in the same fashion as the engine path, but ignores the exakt position of the path
		IMPULSE_TELEPORT_FOLLOW		= 16,//Bot moves to path position every frame
		IMPULSE_FOLLOW_PATH			= 17,//Bot follows given path until released.
		IMPULSE_FLY_FOLLOW_TRUE		= 18,//Bot follows path exactl instead of following path object

		IMPULSE_DIALOGFLAGS			= 4,//Set the bots KEEPBEHAVIOUR or NOLOOK flags

		IMPULSE_ANIMATION			= 5,//Bot preforms given full body animation.
		IMPULSE_TORSO_ANIMATION		= 6,//Bot preforms given torso animation
		IMPULSE_RUN_BEHAVIOUR		= 12,//Bot starts running the given behaviour
		IMPULSE_LOOP_BEHAVIOUR		= 129,//Run behaviour looping
		IMPULSE_STOP_BEHAVIOUR		= 13,//Bot tells behaviour to stop running, then he waits for confirmation

		IMPULSE_DAMAGE				= 7,//Bot receives the given amount of damage
		IMPULSE_DIE					= 62,//Bot dies
		IMPULSE_PUSH				= 14,//Bots velocity is set to the given amount, in the direction of the sending object or given object (if any)

		IMPULSE_GHOSTMODE			= 15,//Bot becomes a ghost, physics-wise

		IMPULSE_AI_VOICE			= 10,//Bot triggers given speech sound 
		IMPULSE_DETECT				= 11,//Bot DETECTS given objects
		IMPULSE_UNSPOT_ALL			= 37,//Bot clears all enemy character info

		IMPULSE_SOFTLOOK			= 19,//Bot continuously looks at given object, or as far in that direction as possible without having to strafe or move backwards, even when taking certain actions
		IMPULSE_UNPAUSE				= 20,//Is unpaused, keeping any scripting
		IMPULSE_LOOK				= 21,//Bot continuously looks at given object, even when taking certain actions

		IMPULSE_CROUCH				= 22,//Bot crouches until another crouch impulse is sent or bot is released
		IMPULSE_JUMP				= 23,//Bot jumps once
		IMPULSE_SWITCH_WEAPON		= 24,//Bot switches to given weapon or to next weapon if no weapon index is given
		IMPULSE_SWITCH_ITEM			= 25,//Bot switches to given item or to next item if no item index is given
		IMPULSE_USE_WEAPON			= 26,//Bot uses wepon once as soon as it can
		IMPULSE_USE_ITEM			= 27,//Bot uses item once as soon as it can

		IMPULSE_MOVE_TO				= 28,//Bot navigates to position of given object, offset by any given offset, until it reaches that position or is released
		IMPULSE_LOOK_AT				= 29,//Bot tracks given object's focus position, until it gets another look command or is released
		IMPULSE_SNAP_LOOK_AT		= 30,//Bot looks instantly at given object's focus position, until it gets another look command or is released
		IMPULSE_ATTACK              = 31,//Bot aims until weapon is ready, then attacks given object once
		IMPULSE_AIM				    = 32,//Bot aims at object until otherwise ordered.

		IMPULSE_ESCALATE			= 33,//Starts escalating for the given time
		IMPULSE_FORCE_BEHAVIOUR		= 34,//Start running the behaviour RIGHT NOW!
		IMPULSE_FORCE_SHOOT			= 35,//DEPRECATED Bot makes an attack immediately regardless of any normal restrictions towards the given target without changing heading. This is only effective for ranged weapons, of course.
		
		IMPULSE_AGGRESSIVE			= 36,//Bot becomes aggressive and will attack the given target, or any valid target, without moving
		
		IMPULSE_PROJ_INVULNERABLE	= 38,// Invulnerable to projectile attacks
		IMPULSE_INVESTIGATE_OBJ		= 39,
		IMPULSE_NOTICE_PLAYER_AT	= 41,//Notice player at pos if awareness NOTICE or less

		IMPULSE_ADD_TEAM			= 40,//Bot is added to team
		IMPULSE_REMOVE_TEAM			= 47,//Bot is removed from team

		IMPULSE_AWARENESS			= 42,//Bot changes awareness
		IMPULSE_SIGHTRANGE			= 43,//Bot changes sight range
		IMPULSE_FLANK				= 44,//Bot tries to flank the player
		IMPULSE_TARGET				= 45,//Bot will attack this target before others
		IMPULSE_WALLCLIMB			= 46,//Turns wallclimbing on/off

		IMPULSE_ACTIVATERANGE		= 50,//Bot changes activate range
		IMPULSE_HITRATIO			= 52,//Bot changes hit ratio
		IMPULSE_HEARINGRANGE		= 53,//Bot changes hearing range
		IMPULSE_IDLEMAXSPEED		= 54,//Bot changes idle max speed
		IMPULSE_HEALTH				= 55,//Bot changes health value
		IMPULSE_ALERTNESS			= 56,//Bot changes alertness
		IMPULSE_DEBUG				= 57,//General debugging impulse, use as you wish!
		IMPULSE_UNDEAD_STUN			= 58,//Put to sleep/wake undead

		// Restriction related impulses
		IMPULSE_RESTRICT_ACTIVE		= 59,//Restrict turned on or off (1 or 0)
		IMPULSE_RESTRICT_RANGE		= 60,//Restrict maxrange
		IMPULSE_RESTRICT_OBJECT		= 61,//Set object to restrict around

		IMPULSE_SECURITY_TOLERANCE	= 63,//Change security tolerance
		IMPULSE_ALLOW_ATTACKS		= 110,//Bot will attack or not (1 or 0)
		IMPULSE_CHARGE				= 111,//Bot will charge the enemy for patience ticks

		//Action related stuff
		IMPULSE_TURRET_MOUNT		= 109,
		IMPULSE_SCENEPOINTOVERRIDE  = 112,//Bot will try to take given scenepoint next time a scenepoint of matching type is required.
		IMPULSE_MOVE_TO_SCENEPOINT	= 113,//Bot will move towards the given scenepoint and stay there unntil its duration
		// IMPULSE_CHAR_RENDERED		= 114,//Received when character was potentially rendered (Obsolete)
		IMPULSE_USEFLAGS			= 115,//Received when we want to change the AIs useflags

		//The below enums are not actually used, but rather the actual behaviour enums are used instead.
		IMPULSE_ACTION_PATROL = ACTION_OVERRIDE_ENUM_BASE+1,		//Bot switches to patrol action, with the given engine path as patrol route, if any
		IMPULSE_ACTION_EXPLORE = ACTION_OVERRIDE_ENUM_BASE+4,		//Bot switches to explore action (Roam most likely)
		IMPULSE_ACTION_HOLD = ACTION_OVERRIDE_ENUM_BASE+5,			//Bot switches to hold action, with the position of the given object as hold-point, if any

		IMPULSE_SPECIAL				= 120,//Bot performs special script, according to specific custom AI
		IMPULSE_SET_ENEMY_RELATION	= 121,// Set the given object(s) to enemy
		IMPULSE_SET_FRIEND_RELATION	= 122,// Set the given object(s) to friend
		IMPULSE_SET_DEFAULT_RELATION	= 123,// Set the given object(s) (back) to default
		IMPULSE_HOSTILE_RELATION		= 124,// Make AI Hostile

		IMPULSE_COVER =					125, // Bot will cover for patience ticks
		IMPULSE_SET_MAX_STANCE = 126,		// Set the maximum stance
		IMPULSE_SET_DRAGOBJECT	= 127,		//Set object to drag
		IMPULSE_SET_MIN_STANCE	= 128,		// Set the minimum stance
		IMPULSE_SET_NEXTOFKIN = 101,		// Set nextofkin flag
		IMPULSE_TELEPORT_TO_SP = 102,		// Teleport ai to named object (pick random if multiple)
		IMPULSE_JUMP_TO_SP = 103,			// Jump to object (pick random if multiple), sets scenepointoverride as well
		IMPULSE_RELEASE_SP = 104,			// Releases currently held scenepoint(s)
		IMPULSE_DARKNESSPOWER = 130			// Use darkness power
	};
	
	// Bits for the various character types to die (Used by IMPULSE_DIE)
	enum
	{
		DIE_FLAG_PLAYER = 1,
		DIE_FLAG_CIV = 2,
		DIE_FLAG_TOUGHGUY = 4,
		DIE_FLAG_BADGUY = 8,
		DIE_FLAG_DARKLING = 16,
		DIE_FLAG_UNDEAD = 32,
		DIE_FLAG_ALL = 64,
	};

	// Push types
	enum
	{
		PUSH_DIRECTION = 0,
		PUSH_AWAY = 1,
	};

	//Message target modes
	enum {
		MSGMODE_ALARM,					//All teammembers within hearing range will get the message	
		MSGMODE_ALL,					//All teammembers will get the message			
	};

	//AI messages
	enum {
		OBJMSG_ENEMYSPOTTED = OBJMSGBASE_AI,				
		OBJMSG_IM_ALIIIVE,
		OBJMSG_BLOCKED_ATTACK,
		OBJMSG_RELEASEPATH,
		OBJMSG_DIE,
	};

	//Some interval values
	enum {
		II_PERCEPTION_HEARING	= 10,
	};

	// Indices for the perception factors
	// Prefix SOUND_ refers to the object making the sound
	// Prefix LISTEN_ refers to the object listening
	// Prefix SIGHT_ refers to the object being observed
	// Prefix VIEW_ refers to the object observing
#define PERCEPTIONFACTOR_SOUND_CROUCH				0.0f
#define PERCEPTIONFACTOR_SOUND_STAND				0.0f
#define PERCEPTIONFACTOR_SOUND_WALK					0.25f
#define PERCEPTIONFACTOR_SOUND_RUN					1.0f
#define PERCEPTIONFACTOR_SOUND_LOS_BLOCKED			0.25f
#define PERCEPTIONFACTOR_SOUND_LOS_BLOCKED_GUN		0.75f
#define PERCEPTIONFACTOR_LISTEN_STAND				1.0f
#define PERCEPTIONFACTOR_LISTEN_WALK				1.0f
#define PERCEPTIONFACTOR_LISTEN_RUN					0.5f
#define PERCEPTIONFACTOR_LISTEN_SLEEPING			0.25f
#define PERCEPTIONFACTOR_LISTEN_DROWSY				0.5f
#define PERCEPTIONFACTOR_LISTEN_IDLE				0.75f
#define PERCEPTIONFACTOR_LISTEN_WATCHFUL			1.0f
#define PERCEPTIONFACTOR_SIGHT_CROUCH				0.75f
#define PERCEPTIONFACTOR_SIGHT_STAND				0.75f
#define PERCEPTIONFACTOR_SIGHT_WALK					1.0f
#define PERCEPTIONFACTOR_SIGHT_RUN					1.0f
#define PERCEPTIONFACTOR_SIGHT_DARKNESS				0.5f
#define PERCEPTIONFACTOR_SIGHT_LOS_BLOCKED			0.0f
#define PERCEPTIONFACTOR_VIEW_STAND					1.0f
#define PERCEPTIONFACTOR_VIEW_WALK					1.0f
#define PERCEPTIONFACTOR_VIEW_RUN					0.5f
#define PERCEPTIONFACTOR_VIEW_SLEEPING				0.0f
#define PERCEPTIONFACTOR_VIEW_DROWSY				0.5f
#define PERCEPTIONFACTOR_VIEW_IDLE					0.75f
#define PERCEPTIONFACTOR_VIEW_WATCHFUL				1.0f
#define PERCEPTIONFACTOR_VIEW_WIDE_FOV				0.25f
#define PERCEPTIONFACTOR_THRESHOLD_SPOT				0.75f
#define PERCEPTIONFACTOR_VERTICAL_MULTIPLIER		4.0f
#define PERCEPTIONFACTOR_MIN_SNEAK_RANGE			40.0f
#define PERCEPTIONFACTOR_MIN_LIGHT					0.2f
#define PERCEPTIONFACTOR_CREEP_SPOT					0.15f
#define PERCEPTIONFACTOR_CREEP_DETECT				0.3f
#define PERCEPTIONFACTOR_CREEP_NOTICE				0.45f

	// Some perception flags
#define PERC_NIGHTVISION_FLAG				(1 << 0)
#define PERC_XRAYVISION_FLAG				(1 << 1)
#define PERC_HEARTHROUGHWALLS_FLAG			(1 << 2)

	// Enums for AG2
	enum
	{
		BEHAVIOUR_FLAGS_LEFT	= 0,		// For completeness
		BEHAVIOUR_FLAGS_FAST	= 1,		// Fast transition to/from behaviour main
		BEHAVIOUR_FLAGS_DIRECT	= 2,		// Go to/from behaviour main, "...do not collect any money"	
		BEHAVIOUR_FLAGS_RIGHT	= 4,		// Right version behaviour, also used as second version in two behaviours
		BEHAVIOUR_FLAGS_TOP		= 8,		// Top/fwd version of behaviour
		BEHAVIOUR_FLAGS_BOTTOM	= 16,		// Bottom/rear version of behaviour
		BEHAVIOUR_FLAGS_LOOP	= 32,		// Loop continuous
		BEHAVIOUR_FLAGS_EXIT_MH	= 64,		// Exit to MH
		BEHAVIOUR_FLAGS_PP		= 128,		// Set perfect placement
		BEHAVIOUR_FLAGS_PO		= 256,		// Set perfect origin
		BEHAVIOUR_FLAGS_JUMPOK	= 512,		// Behaviours whilejumping OK
	};

	enum
	{
		BEHAVIOUR_STOP_NONE		= 0,		// No stop
		BEHAVIOUR_STOP_NORMAL	= 1,		// Plays exit animation, may fail
		BEHAVIOUR_STOP_FAST		= 2,		// Skips exit, blends instead
		BEHAVIOUR_STOP_DIRECT	= 3,		// Blends right out to stance ignoring MH etc
	};
	enum
	{
		BEHAVIOUR_COMBAT_MIN	= 4000,
		BEHAVIOUR_COMBAT_MAX	= 5999,
	};

// Add this to behaviour nbr when equipped with rifle
#define BEHAVIOUR_COMBAT_RIFLE_OFFSET	400
#define GESTURE_COMBAT_RIFLE_OFFSET		200
	// Predefined behaviour values
	enum {
		BEHAVIOUR_NBR_STOPPED = 0,				// Should NOT be used except as a response to OBJMSG_AIEFFECT_ENDOFBEHAVIOR
		BEHAVIOUR_NBR_SEIZURE = 17,
		BEHAVIOUR_NBR_IDLE_SPEAK = 100,			// Speak animation (talking into headset etc)
		BEHAVIOUR_NBR_DODGE_LEFT = 73,			// Various dodge behaviours (cannot be interrupted, autoexpires)
		BEHAVIOUR_NBR_DODGE_RIGHT = 74,
		BEHAVIOUR_NBR_ROLL_LEFT = 75,
		BEHAVIOUR_NBR_ROLL_RIGHT = 76,
		BEHAVIOUR_NBR_CROUCH_ROLL_LEFT = 77,
		BEHAVIOUR_NBR_CROUCH_ROLL_RIGHT = 78,
		BEHAVIOUR_NBR_FIND_BODY = 101,			// Deprecated
		BEHAVIOUR_NBR_SURPRISE_FRONT = 102,
		BEHAVIOUR_NBR_SURPRISE_BACK = 103,

		BEHAVIOUR_NBR_STOP = 126,				// Exit gracefully out of the behaviour
		BEHAVIOUR_NBR_STOP_FAST = 127,			// We need to exit in a hurry
		BEHAVIOUR_NBR_STOP_DIRECT = 128,		// We need to exit in a hurry

		BEHAVIOUR_NBR_TURNING = 7633,			// AG is turning body

		// Combat behaviours
		CB_CROUCH_PISTOL_MH								= 4000,	
		CB_STAND_PISTOL_MH								= 4100,	// Reversed look behaviour
		
		CB_CROUCH_CHARGE_PISTOL							= 4001,	// Charge out fro, crouched cover
		
		BEHAVIOUR_OPENDOOR_IN							= 160,
		BEHAVIOUR_OPENDOOR_IN_KNOB						= 161,
		BEHAVIOUR_OPENDOOR_OUT							= 162,
		BEHAVIOUR_OPENDOOR_OUT_KNOB						= 163,

		BEHAVIOUR_COMBAT_BLACKHOLE_STAND				= 893,
		BEHAVIOUR_COMBAT_BLACKHOLE_SUCKEDIN				= 894,

		BEHAVIOUR_COMBAT_GESTURE_SCARED					= 892,

		BEHAVIOUR_LIE_IN_AGONY							= 7394,

		// Synch behaviours, use BEHAVIOUR_FLAGS_RIGHT for second version
		BEHAVIOUR_DARKLING_BORED					= 171,
		BEHAVIOUR_DARKLING_BACKFLIP					= 270,
		BEHAVIOUR_DARKLING_NONO						= 230,
		BEHAVIOUR_DARKLING_GROWL					= 273,
		BEHAVIOUR_DARKLING_FUCKOFF					= 278,
		BEHAVIOUR_DARKLING_SUCCESS					= 282,
		BEHAVIOUR_DARKLING_PISS						= 283,
		BEHAVIOUR_DARKLING_BLINDED					= 288,
		BEHAVIOUR_DARKLING_SCARE					= 298,
		BEHAVIOUR_DARKLING_SCARE2					= 299,
		BEHAVIOUR_DARKLING_HANDSUP					= 305,
		BEHAVIOUR_DARKLING_JACKIESHOOT				= 318,
		BEHAVIOUR_DARKLING_JACKIESHOOT_SHORT		= 319,
		BEHAVIOUR_DARKLING_THUMB_DOWN				= 428,
		BEHAVIOUR_DARKLING_THUMB_UP					= 302,
	
		BEHAVIOUR_DARKLING_JUMPATTACK				= 350,

		BEHAVIOUR_MEATFACE_PLAYER_PISTOL			= 353,
		BEHAVIOUR_MEATFACE_PLAYER_RIFLE				= 354,
		BEHAVIOUR_MEATFACE_CLOSE_PLAYER_PISTOL		= 351,
		BEHAVIOUR_MEATFACE_CLOSE_PLAYER_RIFLE		= 352,
		BEHAVIOUR_MEATFACE_THROW					= 379,

		// These are the same on human darklings (the type that is, anims are different...)
		BEHAVIOUR_DARKLING_BERZERKER_AXE			= 180,
		BEHAVIOUR_DARKLING_BERZERKER_AXE_SHORT		= 181, 
		BEHAVIOUR_DARKLING_BERZERKER_BAT			= 182,
		BEHAVIOUR_DARKLING_BERZERKER_BAT_SHORT		= 183,	// 84 units range
		BEHAVIOUR_DARKLING_BERZERKER_HAMMER			= 184,
		BEHAVIOUR_DARKLING_BERZERKER_HAMMER_SHORT	= 185,	// 54 units
		BEHAVIOUR_DARKLING_BERZERKER_MACHETE		= 186,
		BEHAVIOUR_DARKLING_BERZERKER_MACHETE_SHORT	= 187,
		BEHAVIOUR_DARKLING_BERZERKER_PISTOL			= 188,
		BEHAVIOUR_DARKLING_BERZERKER_SAW			= 189,
		BEHAVIOUR_DARKLING_BERZERKER_SAW_SHORT		= 190,
		BEHAVIOUR_DARKLING_BERZERKER_SLEDGEHAMMER	= 191,
		BEHAVIOUR_DARKLING_BERZERKER_SLEDGEHAMMER_SHORT	= 192,
		BEHAVIOUR_DARKLING_BERZERKER_TEETH			= 193,
		BEHAVIOUR_DARKLING_BERZERKER_TEETH_SHORT	= 194,
		BEHAVIOUR_DARKLING_BERZERKER_INTERROGATE	= 195,
		BEHAVIOUR_DARKLING_BERZERKER_LIGHTKILLER	= 196,
		BEHAVIOUR_DARKLING_BERZERKER_LIGHTKILLER_SHORT = 197,
		BEHAVIOUR_DARKLING_BERZERKER_GRUMPY			= 198,
		BEHAVIOUR_BERZERKER_MIN						= 180,
		BEHAVIOUR_BERZERKER_MAX						= 198,
		BEHAVIOUR_DARKLING_KAMIKAZE					= 205,
		BEHAVIOUR_DARKLING_KAMIKAZE_SHORT			= 206,
		BEHAVIOUR_DARKLING_KAMIKAZE_QUICK			= 207,


		BEHAVIOUR_ANGELUS_THROWBALL					= 904,
		BEHAVIOUR_ANGELUS_BREAKBLEECHER				= 905,

		BEHAVIOUR_ANGELUS_FLASHBANG					= 907,
		BEHAVIOUR_ANGELUS_TENTACLE_HITOBJECT		= 908,
		BEHAVIOUR_ANGELUS_TENTACLE_HITTARGET		= 909,
		BEHAVIOUR_ANGELUS_EMBRACE					= 910,

		GESTURE_DARKLING_BUMP						= 1,
		GESTURE_HUMAN_CHECK_LEFTRIGHT				= 6,
		GESTURE_MEATFACE_CHECK_THROW				= 50,
	};	

	// Offsets from root for CB leanout/popup etc
#define CB_OFF_CROUCH	28.0f
#define CB_OFF_STAND	50.0f
#define CB_OFF_LEAN		18.0f
#define CB_OFF_POPUP	48.0f

// Aproximately 1 degree of arc
#define AI_LOOK_ANGLE_THRESHOLD		0.003f
#define MIN_SPEAK_BEHAVIOUR_RANGE	256.0f


	//The AI types
	enum {
		TYPE_CHARACTER,
		TYPE_TURRET,
		TYPE_DARKLING,
	};


public: //public for simplicity...
	//Attributes

	//Do we have a direct ground path to our current destination?
	bool m_bDirectPath;

	//Position the previous server frame
	CVec3Dfp32 m_PrevPos;
	
	//True if character normally isn't affected by gravity (updated in OnRefresh)
	bool m_bWeightless;

	//The agent we control
	CWObject_Interface_AI* m_pGameObject;

	//The server we operate in
	CWorld_Server* m_pServer;

	// TODO: Use m_pCD that we get whenever it's NULL, simplifies code
	// **** CWO_Character_ClientData* m_pCD;

	//The AI resource handler
	CAI_ResourceHandler* m_pAIResources;

	//The status of any pathfinding.
	int m_SearchStatus;

	//Keeps track of whether we have reached our path destination or not
	bool m_bReachedPathDestination;

	//Keeps track of the actions we are performing
	int m_ActionFlags;

	// Members dealing with the current running behaviour
	int32 m_BehaviourStopIssued;					// Can be any of BEHAVIOUR_STOP_NONE, BEHAVIOUR_STOP_NORMAL, BEHAVIOUR_STOP_FAST or BEHAVIOUR_STOP_DIRECT
	int16 m_StopDir;								// 0=Fwd, 1 = left, -1 = right (we really want the default to be 0)
	uint32 m_BehaviourStopFlags;					// Flags from last explicit stop call
	bool m_bBehaviourRunning;						// We told the AG to run a behaviour
	bool m_bBehaviourMainRunning;					// AG told us the behaviours is running
	bool m_bBehaviourLooping;						// We told AG the behaviour should loop
	int32 m_iBehaviourRunning;
	int32 m_BehaviourPrioClass;						// Prio class for the behaviour
	int32 m_iPendingBehaviour;						// Qeued behaviour
	int32 m_PendingBehaviourPrioClass;				// Prioclass of above
	CWO_ScenePoint* m_pBehaviourScenePoint;			// Scenepoint to hog when running the behaviour
	int32 m_BehaviourExpirationTick;				// When the behaviour should expire (0 = never, -1 = now)
	
	// Members handled by OnRefreshBehaviours()
	int32 m_iCurBehaviour;							// Behaviour held this tick
	int32 m_iCurMainBehaviour;						// Set to m_iCurBehaviour when not entering or exiting
													// this is also when OnStartBehaviour event is triggered
	int32 m_iPrevBehaviour;							// Behaviour held last refresh
	bool m_bBehaviourEntering;						// true when entering a behaviour (given by m_iCurBehaviour)
	bool m_bBehaviourExiting;						// true when exiting a behaviour (given by m_iCurBehaviour)

	int32 m_GestureTimer;							// Cannot play another gesture until timer is zero

	// Stuff dealing with combat behaviours goes here
	// (Does this sound like another animgraph-ish implementation or what?)
	// We implement a 'simple' list of S_CombatBehaviour
	// Each S_CombatBehaviour has a m_iBehaviour: The behaviour nbr to play
	// They may also have m_iPrev and m_iNext
	// To start select randomly any m_iPrev == 0 from the list and play its m_iBehaviour
	// When m_Duration is up:
	//	Randomly pick from list all items whose m_iBehaviour is m_iPrevBehaviour or m_iNextBehaviour
	struct S_CombatBehaviour
	{
		S_CombatBehaviour()
		{
			Clear();
		}

		void Set(int16 _iBehaviour, int16 _iPrev, int16 _iNext, int16 _TacFlags, int16 _Duration, int16 _Flags)
		{
			m_iBehaviour = _iBehaviour;
			m_iPrev = _iPrev;
			m_iNext = _iNext;
			m_TacFlags = _TacFlags;
			m_Duration = _Duration;
			m_UseFlags = _Flags;
		}
		void Clear()
		{
			m_iBehaviour = 0;
			m_iPrev = 0;
			m_iNext = 0;
			m_TacFlags = 0;
			m_Duration = 0;
			m_UseFlags = 0;
		};

#ifndef M_RTM
		CStr m_DbgName;		// Name of CB behaviour
#endif
		int16 m_iBehaviour;	// The behaviour nbr to play, may not be zero
		int16 m_iPrev;		// 0 means no sub behaviour, exit to m_iPrev only
		int16 m_iNext;		// 0 means no sub behaviour, exit to m_iPrev only
		int16 m_TacFlags;	// What CWO_ScenePoint::m_TacFlags the behaviour requires
		int16 m_Duration;	// Duration, actual duration will be 50% to  150% of m_Duration, 0 means until finished
		int16 m_UseFlags;	// Usage flags
	};

	enum
	{
		CBFLAG_FASTEXIT = 1,
		CBFLAG_WIDEMOVE	= 2,
	};

	S_CombatBehaviour m_PrevCB;
	S_CombatBehaviour m_CurCB;

	int32 m_CurCBTarget;			// Current target for CB
	int32 m_CurCBTimeout;			// CAI_Core::m_Timer for when a new CB should be chosen
	int32 m_ScenepointCBTimeout;	// CAI_Core::m_Timer for scenepoint as a whole
	TArray<S_CombatBehaviour> m_lCombatBehaviour;
#define MAX_CB_PICKCOUNT	16
	int8 m_li_CBChoices[MAX_CB_PICKCOUNT];
	// Debug flag to avoid spamming
#ifndef M_RTM
	// Return the pos looked at AI
	CVec3Dfp32 GetLookAtPos(int _iPlayer);
	// Returns the bot object ID that is nearest FOV centerline and closest according to the following heuristic: TBD
	int32 FindBotInFOV(CVec3Dfp32* _pLook = NULL, CVec3Dfp32* _pPos = NULL);
	void DebugDrawNavgrid(int32 _iObject = 0);
	void DebugDrawCheckDead(CVec3Dfp32& _ViewPos,CVec3Dfp32& _VictimPos,int _Awareness);
	void DebugDrawKnowledge();
	void DebugDrawLight();
#endif

	//Weapon and item handler
	CAI_WeaponHandler m_Weapon; 
	CAI_ItemHandler m_Item;


	//Pathfinding handler
	CAI_Pathfinder m_PathFinder;

	//Current path
	CAI_Path m_Path;

	//Behaviour type from key
	int m_ActionFromKey;
	
	//Behaviour parameter keys
	TArray<CNameValue> m_lActionParamKeys;

	//The current personality
	CAI_Personality m_Personality;

	//The actions handler
	CAI_ActionHandler m_AH;

	//Haven't we refreshed the bot before in this round?
	bool m_bFirstRefresh;

	// If this flag is true we can be added to someones knowledgebase
	bool m_bAgent;

	//How far the bot autodetects enemy in any direction
	fp32 m_Awareness;

	//How far away the bot can detect enemy in it's field of view
	fp32 m_SightRange;
	
	//How far away the bot can hear enemy
	fp32 m_HearingRange;
	fp32 m_VerticalMultiplier;	// Multiplies vertical distance

	//The bots field of view in fractions of a all around view (i.e. between 0 and 1)
	fp32 m_FOV;
	fp32 m_FOVWide;
	fp32 m_FakeFOVLight;
	fp32 m_TurnPerTick;

	// Flagfield for various abilities that modify perception
	// These only affect OUR perception capabilities, not how others perceive us
	int32 m_PerceptionFlags;

	// Multiplicative factor for how loud/big etc we are
	// Will only affect THEIR perception of us, not how we perceive others
	fp32 m_PerceptionFactor;
	fp32 m_LoudMovement;
	
	//The bots ability to reason about uncertain information. 0..100.
	int m_Reasoning;

	// Action ranges
	// Various ranges used by actions to simplify template editing
	// Roam action
	fp32 m_RoamMinRange;
	fp32 m_RoamMaxRange;
	// Search action
	fp32 m_SearchMinRange;
	fp32 m_SearchMaxRange;

	// Ranged fire max and min
	fp32 m_RangedMinRange;
	fp32 m_RangedMaxRange;
	// Melee max, there is no min to that
	fp32 m_MeleeMaxRange;
	
	// Close action
	fp32 m_CloseMinRange;
	fp32 m_CloseMaxRange;
	// Combat action
	fp32 m_CombatMinRange;
	fp32 m_CombatMaxRange;
	fp32 m_CombatTargetMinRange;
	fp32 m_CombatTargetMaxRange;
	// Cover
	fp32 m_CoverMinRange;
	fp32 m_CoverMaxRange;
	fp32 m_CoverTargetMinRange;
	fp32 m_CoverTargetMaxRange;
	// Jumping
	fp32 m_JumpMinRange;
	fp32 m_JumpMaxRange;

	enum
	{
		BRAVERY_NEVER		= 0,	// Never fight
		BRAVERY_BETTER		= 1,	// Fight only when better armed (fist<melee<ranged<heavy)
		BRAVERY_EQUAL		= 2,	// Fight when eqal or better armed
		BRAVERY_ONE_LESS	= 3,	// Fights when no more than 1 less in weapon class
		BRAVERY_ALWAYS		= 4,	// Fight always
	};
	//The bots courage or lack thereof
	int m_DefaultBravery;			// The bravery value given by template
	int m_Bravery;					// Current bravery modified by scripts
	int m_MeleeRelationIncrease;	// Bot raises relation with agents that recently swung melee blows
	int m_RelationChangeAwareness;

	enum
	{
		ORDER_NONE			= 0,
		ORDER_CHARGE		= 1,
		ORDER_COVER			= 2,
		ORDER_FLANK			= 3,
		ORDER_ESCALATE		= 4,
	};
	int m_Order;					// Can be any of ORDER_NONE, ORDER_CHARGE, ORDER_COVER or ORDER_FLANK
	int m_OrderTicksRemaining;		// How many ticks remain of the order

	//The bots ability to fight well (aiming correctly and choosing the right manoeuvres in melee)
	int m_Skill;

	//The patience of the bot when searching for threats etc in server ticks
	//Default value is 200 ticks (10.0 seconds)
	int m_Patience;

	//The chance that a "hitting shot" is not overridden and turned into a miss. Ha! Suck on that formulation!
	int m_HitRatio;
	fp32 m_FriendlyFireFactor;

	//The aiming bonus per frame against stationary targets
	fp32 m_StationaryBonus;

	//When this value is non-zero, a penalty applies when the agent wants to fire at a player that hasn't
	//spotted him yet. When the first shot is fired hitratio will be at 0, LERPing to 100 when this duration is up
	int m_SniperPenalty;

	//This time in server frames is used to calculate the time necessary to raise awareness. It's also
	//the awareness delay imposed when raising awareness in certain alertness modes and the delay
	//before a bot raises an alarm.
	int m_Reaction;

	int16 m_PressDuration;	// How long do we press the trigger? Default 0
	int16 m_ShootPeriod;	// How long do we wait to fire again in UsePeriodic? Default 20

	// Local value, to ease the CPU load of constantly asking this expensive items
	int m_StealthTension;
	int m_StealthTensionTime;	// Last time m_StealthTension was set

	bool m_bCanWallClimb;		// When TRUE m_bWallClimb can be set to true
	bool m_bWallClimb;			// TRUE if char should wallclimb/jump
	int32 m_ClimbEnableTicks;	// When == 0 switch off wallclimbing
	int32 m_ClimbDisableTicks;	// When != 0 TempEnableWallClimb is disabled

	bool m_bCanJump;			// When TRUE bot can jump whenever m_bJump and m_JumpFlags allow it
	bool m_bJump;				// 
	int32 m_JumpFlags;			// Flags for under which circumstances the bot can jump, still given by m_bCanJump above
	int32 m_JumpDisableTicks;	// Nbr of ticks of no jumping
	enum
	{
		JUMPREASON_IDLE			= 1,	// Bot may jump from IDLE actions
		JUMPREASON_COMBAT		= 2,	// Bot can jump from COMBAT actions
		JUMPREASON_AVOID		= 4,	// Bot can jump when avoiding player
		JUMPREASON_RETARGET		= 8,	// Bot can jump when retargeted
		JUMPREASON_STUCK		= 16,	// Bot can jump when stuck
		JUMPREASON_NOGRID		= 32,	// Bot can jump when there's no navgrid
		JUMPREASON_SCRIPT		= 64,	// Bot can jump when scripted to do so
	};
	enum
	{
		JUMP_FROM_AIR	= 1,	// Jump from air
		JUMP_TO_AIR		= 2,	// Land on air
		JUMP_CHECK_COLL = 64,	// Check for collisions
	};
	bool m_bWorldspaceMovement;	// Typically false, when true we send desired moves in
								// normalized worldspace coords

	// Darkness related members
	uint16 m_SeenDarknessPowers;	// What darknesspowers we have seen so far
	bool m_bFearDarkness;			// True if the bot fears darkness
	int m_CharacterClass;			// CLASS_PLAYER,CLASS_CIV,CLASS_TOUGHGUY,CLASS_BADGUY,CLASS_DARKLING,CLASS_UNDEAD
	enum
	{
		CLASS_DEFAULT	= 0,	// Player etc
		CLASS_CIV		= 1,	// Coward window dressing
		CLASS_TOUGHGUY	= 2,	// Friendly badass
		CLASS_BADGUY	= 3,	// Potential enemy
		CLASS_DARKLING	= 4,	// Soul eating mischievious critter
		CLASS_UNDEAD	= 5,	// Zombie style guy, can only be permanently killed by darkness guns
	};
	int m_Importance;			// How important the character is
	enum
	{
		IMPORTANCE_UNIMPORTANT	= 0,	// Riff raff, window dressing, respawn, noname
		IMPORTANCE_NORMAL		= 1,	// Regular NPC, not very important
		IMPORTANCE_IMPORTANT	= 2,	// Has special dialogue
		IMPORTANCE_CRITICAL		= 3,	// Should NOT be killed, extremely important to the game
	};
	
	// The m_UndeadXXX members specifically deals with CLASS_UNDEAD bots
	//==================================================================
	// 0: Is already awake
	// -1: Never wakeup, keep playing the behaviour
	// >0: When to wake up based on CAI_Core::m_Timer
	int m_UndeadSleepTicks;		// Duration of undead sleep, -1 means forever
	int m_UndeadDieBehaviour;	// Behaviour to play when resurrectably dead
#define AI_UNDEAD_SLEEPTIME		60

	uint16 m_AgonyBehaviour;
	uint16 m_SecondaryWeaponAnimType;	// Precache knifemoves too

	// When a character dies for whatever reason he should be reported as dead to other bots
	// if m_bNextOfKin is true. CAI_Action_Investigate and CAI_Action_InvestigateDeath may then
	// use the dead data to look for dead bodies etc.
	bool m_bNextOfKin;	// Only report death when this is true

	// If m_Timer >= m_PendingAlarmTime and m_PendingAlarmTime != -1 m_PendingAlarmMsg will be sent
	// Hack, should be some kind of queu instead but as we only support RaiseAlarm and RaiseAwareness it's OK
	int m_PendingAlarmTime;
	CWObject_Message m_PendingAlarmMsg;

	//If this is not -1, bot will treat any agents who violate the clearance level as hostile
	//and treat any agents that violate the clearance level by more than this number as enemy
	int m_SecurityTolerance;

	//The angles we want to look at (handled by RefreshLook)
	CVec3Dfp32 m_LookAngles;
	//The direction we want to look at (handled by RefreshLook)
	CVec3Dfp32 m_LookDir;
	//The time (number of frames) remaining to change look direction to m_LookAngles
	int16 m_LookTime;

	//Continuous look at object, and flag if we should look "softly" or not
	int16 m_iLookAtObject;
	bool m_bLookSoft;

	//Object to drag
	int16 m_iDragObject;

	//The pos and direction for hold, restrict etc
	CVec3Dfp32 m_HoldPosition;
	CVec3Dfp32 m_HoldLookDir;
	//The general direction we should look; we will never change our look direction by more than 
	//45 degrees from this in OnIdle

	//General purpose timers
	//Each real server tick:
	//Add servertick duration (relative ai tick) to m_ServerTickFraction
	//If m_ServerTickFraction >= 1.0
	//	Subtract 1.0 from m_ServerTickFraction
	//	Increment m_Timer and m_Timer
	//	Perform OnRefresh
	int32 m_Timer;
	int32 m_ServerTick;			// Refreshed 20 times a second no matter what the damned server rate is
	fp32 m_ServerTickFraction;	// When less than 1.0f, the next full server tick has not yet ticked
	bool m_bServerTickAdvanced;

	//Pathfinding resource priority
	int16 m_PathPrio; //0..255, otherwise invalid

	//Hostile presence object id and flag for new hostility state
	int m_iHostile; 
	bool m_bNewHostility;

	//The world coordinates we should find our way to
	CVec3Dfp32 m_PathDestination;

	//The current sub-destination
	CVec3Dfp32 m_SubDest;

	//The script control object, that takes over actions when we're controlled by a script
	friend class CAI_ScriptHandler;
	CAI_ScriptHandler m_Script;

	//The eventhandler
	CAI_EventHandler m_EventHandler;

	//The devices
	CAI_Device_Look			m_DeviceLook;
	CAI_Device_Move			m_DeviceMove;
	CAI_Device_Jump			m_DeviceJump;
	CAI_Device_Sound		m_DeviceSound;
	CAI_Device_Weapon		m_DeviceWeapon;
	CAI_Device_Item			m_DeviceItem;
	CAI_Device_Stance		m_DeviceStance;
	CAI_Device_Animation	m_DeviceAnimation;	// Deprecated?
	CAI_Device_Melee		m_DeviceMelee;		// Deprecated?
	CAI_Device_Facial		m_DeviceFacial;
	CAI_Device_Darkness		m_DeviceDarkness;
	
	//List, for easier access
	CAI_Device* m_lDevices[CAI_Device::NUM_DEVICE];

	//The control handler
	CAI_ControlHandler m_Controlhandler;

	//The world knowledge base
	// m_UseLightmeter
	// -1: Don't use the lightmeter, ignore broken lights
	// 0: Don't use the lightmeter, react to broken lights
	// 1: Use it without shadows, react to broken lights
	// 2: Use it with shadows, react to broken lights
	int m_UseLightmeter;	// use the lightmeter when nonzero
	spCWObj_LightMeter m_spLightMeter;
	bool m_bFlashlight;
	fp32 m_FlashThresholdOverride;	// Used as alt threshold, forces HandleFlashlight call, clears
	int m_FlashThresholdOverrideTimeout;	// When should we clear the m_FlashThresholdOverride?
	CAI_KnowledgeBase m_KB;
#define AI_FLASH_LIGHT_THRESHOLD	0.3f
#define AI_KB_REFRESH_PERIOD		10

	//Were we alive last frame
	bool m_bWasAlive;

	//Were we valid last frame
	bool m_bWasValid;

	// Physical dimensions
	// NOTE These are NOT the same as those in CWO_Character_ClientData
	// Our values are not radii as those in CWO_Character_ClientData are
	// Our values are also rounded up from the above as we need some leeway whan pathfinding
#define AI_DEFAULT_BASESIZE			24
#define AI_DEFAULT_HEIGHT			64
#define AI_DEFAULT_HEIGHT_CROUCH	32
#define AI_DEFAULT_STEP_HEIGHT		12
#define AI_DEFAULT_JUMP_HEIGHT		48
#define AI_DEFAULT_WALK_STEPLENGTH	24
#define AI_DEFAULT_RUN_STEPLENGTH	48
	int16 m_PF_BaseSize;		// CWO_Character_ClientData->m_Phys_Width * 2
	int16 m_PF_Height;			// CWO_Character_ClientData->m_Phys_Height * 2
	int16 m_PF_HeightCrouch;	// CWO_Character_ClientData->m_Phys_HeightCrouch * 2
	int16 m_PF_StepHeight;		// CWObject_Character.m_PhysAttrib.m_StepSize
	int16 m_PF_JumpHeight;
	int16 m_PF_WalkStepLength;	
	int16 m_PF_RunStepLength;

protected:
	enum
	{	// Enumds for m_MDestState
		DEST_NONE	= 0,	// INVALIDATE_POS(m_MDest.GetRow(3)), there should be no stops. Use CAI_Core::NoStops() to set, walkstops true
		DEST_NORMAL = 1,	// There is a stop nearby. Use CAI_Core::SetDestination() to set, walkstops true
		DEST_STOP	= 2,	// Emergency stop. Use CAI_Core::Stop() to set, walkstops true
		DEST_PP		= 3,	// Perfect placement. Use CAI_Core::SetPerfectPlacement() , walkstops false
		DEST_PO		= 4,	// Perfect origin. Use CAI_Core::SetPerfectPlacement() , walkstops false
	};
	bool		m_bMDestDirty;	// Update AG when set
	int			m_MDestState;
	CMat4Dfp32	m_MDest;		// Row 0 is our preferred look dir, row 3 is our cur destination
	fp32			m_MDestSpeed;

#define DARKLING_PF_OFFSET	8.0f
	CVec3Dfp32	m_HeadOff;			// Last valid refresh of headoffset from base
	CVec3Dfp32	m_WeaponOff;		// Last valid weaponoffset from base
	CMat4Dfp32	m_LastBaseMat;
	CMat4Dfp32	m_CurBaseMat;
	CMat4Dfp32	m_InvBaseMat;
	CMat4Dfp32	m_LastHeadMat;
	CMat4Dfp32	m_CurHeadMat;
	CVec3Dfp32	m_CurLookPos;		// Almost the same as m_CurHeadMat.GetRow(3);
	CVec3Dfp32	m_LastLookPos;		// Almost the same as m_LastHeadMat.GetRow(3);
	CVec3Dfp32	m_CurLookDir;		// Almost the same as m_LastHeadMat.GetRow(0);
	fp32		m_CurHeight;

	// Members dealing with turning objColl on or off
	CVec3Dfp32 m_LastValidGridPos;	// THe last pos we could stand in with m_bObjColl == true
	bool m_bObjColl;				// When WE have turned off obj coll this flag is false (when using the sitflag for example)
	bool m_bObjCollRequested;		// We want m_bObjColl to be true again but cannot do so until our position is valid to stand in


public:
	// Logical movement style, fallbacks to default when a certain type is lacking
	int32 m_iMoveVariant;
	enum
	{
		MOVEVARIATION_NORMAL			=0,
		MOVEVARIATION_ALKY				=1,
		MOVEVARIATION_BUM				=2,
		MOVEVARIATION_FEMALE1_1			=3,
		MOVEVARIATION_FEMALE1_2			=4,
		MOVEVARIATION_FEMALE2_1			=5,
		MOVEVARIATION_FEMALE2_2			=6,
		MOVEVARIATION_FEMALE3_1			=7,
		MOVEVARIATION_FEMALE3_2			=8,
		MOVEVARIATION_FEMALE4_1			=9,
		MOVEVARIATION_FEMALE4_2			=10,
		MOVEVARIATION_FEMALE5_1			=11,
		MOVEVARIATION_FEMALE5_2			=12,
		MOVEVARIATION_GANGSTER1			=13,
		MOVEVARIATION_GANGSTER2			=14,
		MOVEVARIATION_GANGSTER3			=15,
		MOVEVARIATION_HURT				=16,
		MOVEVARIATION_MALE1_1			=17,
		MOVEVARIATION_MALE1_2			=18,
		MOVEVARIATION_MALE2_1			=19,
		MOVEVARIATION_MALE2_2			=20,
		MOVEVARIATION_MALE3_1			=21,
		MOVEVARIATION_MALE3_2			=22,
		MOVEVARIATION_MALE4_1			=23,
		MOVEVARIATION_MALE4_2			=24,
		MOVEVARIATION_MALE5_1			=25,
		MOVEVARIATION_MALE5_2			=26,
		MOVEVARIATION_MALE6_1			=27,
		MOVEVARIATION_MALE6_2			=28,
		MOVEVARIATION_OLDGUY1			=29,
		MOVEVARIATION_OLDGUY2			=30,
		MOVEVARIATION_OLDLADY1			=31,
		MOVEVARIATION_OLDLADY2			=32,
		MOVEVARIATION_HORSESLEEP		=33,
		MOVEVARIATION_LIMP				=34,
		MOVEVARIATION_BUTCHER			=35,
		MOVEVARIATION_SWAT				=36,
};

	bool m_bWalkStarts;	// When true we can do walkstarts
	int8 m_LastStairsType;

	// Physical speed values
	fp32		m_PrevSpeed;	// Last movelength
	CVec3Dfp32 m_PrevMoveDir;	// Normalized last dir (world)

	//The maximum speed the character will move at when not under stress (i.e. when no enemies are known)
	fp32 m_IdleMaxSpeed;
	//The maximum speed the character will move at when backing
	fp32 m_MaxEvadeSpeed;
	//Maximum speed when not scripted
	fp32 m_ReleaseMaxSpeed;
	fp32 m_ReleaseIdleSpeed;
	fp32 m_ReleaseFlightSpeed;
	fp32 m_CurFlightSpeedFactor;	// [0..1] fraction of m_ReleaseFlightSpeed used
	//Max speed
	fp32 m_MaxSpeed;

	//Minimum propel step when following a propelled path
	fp32 m_MinPropelStep;
	//Squared ranges when we consider ourselves to have fallen behind/caught up with a propelled path
	fp32 m_FallenBehindRangeSqr;
	fp32 m_CaughtUpRangeSqr;
	int m_CollisionRank;	// Lower values avoid higher, 0 means ignore checking (yes, behaves as highest rank of all)
	//Behaviours are only activated if we've come within activate range of a player, have come under attack, 
	//spotted enemy in other fashion or is script released. Behaviours may become inactivated if so suggested
	//(i.e. flag below set) and there are suitable conditions.
	enum {
		STATE_INACTIVE = 0,
		STATE_DEACTIVATED,

		//States below are active states, those above inactive
		STATE_ACTIVE,
		STATE_SUGGEST_DEACTIVATION,
		STATE_ALWAYSACTIVE,
	};
	int m_ActivationState;
	fp32 m_ActivateRangeSqr;
	fp32 m_DestroyOnDeactivationRangeSqr;
	int m_ActivationTick;
	int m_ActivationTime;

	//Own activity score, to be reported to activity counter at end of refresh.
	int m_ActivityScore;

	//Recommended activity level (optimization, independent of the activation states above)
	enum {
		ACTIVITY_NONE,
		ACTIVITY_LOW,
		ACTIVITY_HIGH,
		ACTIVITY_FULL,
	};
	int m_ActivityLevel;
	int32 m_iFacialGroup;
	int32 m_iMountParam;	// Object we're mounted on

	//Activity level can normally only change when we're at this gametick or later
	int m_ActivityTick;

	//Released bots are paused during cutscenes, unless this flag is true
	bool m_bIgnoreCutscenes;

	//Did we take damage during last frame?
	bool m_bTookDamage;

	int m_LastMeleeTarget;
	int m_LastMeleeAttackTick;	// AI timer when last melee/breakneck was attempted (in GameTicks!)
	int m_LastHurtTimer;		// Last timertick we got hurt *** SAVE? ***

	bool m_bHostileFromBlowDmg;	// When true we onlcy increase hostility each time we take blow dmg
	bool m_bPlayerOnlyMeleeDmg;	// Only meleedmg from PLAYER can hurt us
	fp32	m_DamageFromPlayerFactor;	// Damage given from player gets multiplied by this
									// (Ugly PB hack to keep screamers alive yet fragile)
	bool m_bDeathspots;

	//Shouldn't we use escape sequence moves?
	bool m_bNoEscapeSeqMoves;
	//The position we should move towards when making an escape-sequence move
	CVec3Dfp32 m_EscSeqPos;
	//Do we need to make an escape-sequence move?
	bool m_bIsStuck;
	//For how long are we allowed to be static before considering ourselves stuck?
	uint8 m_StuckThreshold;
	//How many frames have we been static?
	uint8 m_StuckCounter; 
	//Squared distance to current destination
	fp32 m_CurDestDistSqr;

	//Are we adding extra speed when doing a long jump?
	bool m_bJumpCheat;

	//If the life timer isn't negative, this is counted down, and when zero, the bot loses the 
	//specified life drain amount points of health every lifedrain interval frames
	int m_LifeTimer;
	int m_LifeDrainAmount;
	int m_LifeDrainInterval;

	//If non-negative will destroy bot after this amount of frames
	int m_DestroyTimer;	

	//If this value is more than 0, this is the minimum health the bot can be reduced to under normal circumstances
	int m_MinHealth;

	//Should bot do nothing if behaviour controlled?
	bool m_PauseActions;
	// m_StunTimeout is a counter that will count down to 0 whereupon the bot will wake up
	// If m_StunTimeout == -1 the stun lasts forever (yes, constructs like if (m_StunTimeout) are still eligible)
	int m_StunTimeout;

	bool m_bLookPartner;
	// m_iDialoguePartner indicates who we are speaking to or listening to (doesn't matter really)
	uint16 m_iDialoguePartner;
	// m_iCrowdFocus indicates who we as onlooker is looking at
	uint16 m_iCrowdFocus;
	int m_CurPriorityClass;

	//Keeps track of whether we should use some miscellaneous feature
	enum {
		USE_CLUMSY			= M_Bit(0),
		USE_FLYATPATHSPEED	= M_Bit(1),
		USE_UNSPOTFAST		= M_Bit(2),
		USE_SAVEPLAYERINFO	= M_Bit(3),
		USE_SWITCHWEAPON	= M_Bit(4),
		USE_ALWAYSBLOCK		= M_Bit(5),
		USE_SCRIPTMUTE		= M_Bit(6),
		USE_ALWAYSAGGRESSIVE= M_Bit(7),
		USE_NOPATHFINDING	= M_Bit(8),
		USE_NOROOMSELECT	= M_Bit(9),
		USE_DOORSPS			= M_Bit(10),
		USE_STAIRS			= M_Bit(11),
		USE_NO_TURN_IN		= M_Bit(12),
		USE_DRAWAMMO		= M_Bit(13),
		USE_SOFTLOOKPITCH	= M_Bit(14),
		USE_TELEPORTER		= M_Bit(15),
		USE_NONE			= 0x0, 
		USE_ALL				= 0xffff,
#ifdef AI_PB_PC
		USE_RESETPLAYERINFO = 0x9,
#endif
	};
	int16 m_UseFlags;

public: //public for simplicity...
	//Methods

	//Methods for getting the maximum speed of the character in the given directions
#define AI_MOVE_THRESHOLD_WALKSLOW	0.15f
#define AI_MOVE_THRESHOLD_WALK		0.25f
#define AI_MOVE_THRESHOLD_WALKFAST	0.35f
#define AI_MOVE_THRESHOLD_RUN		0.75f

	virtual fp32 GetMaxSpeedForward();
	virtual fp32 GetMaxSpeedSideStep();
	virtual fp32 GetMaxSpeedUp();
	virtual fp32 GetMaxSpeedJump();
	virtual fp32 GetMaxSpeedWalk();

	//Methods for getting some of the bots physical values
	virtual fp32 GetBaseSize();
	virtual fp32 GetHeight();
	virtual fp32 GetCrouchHeight();
	virtual fp32 GetCurHeight();
	virtual fp32 GetStepHeight();
	virtual fp32 GetJumpHeight();
	virtual fp32 GetWalkStepLength();
	virtual fp32 GetRunStepLength();

	// Setters to the above
	void SetBaseSize(fp32 _Val);
	void SetHeight(fp32 _Val);
	void SetCrouchHeight(fp32 _Val);
	void SetStepHeight(fp32 _Val);
	void SetJumpHeight(fp32 _Val);
	void SetWalkStepLength(fp32 _Val);
	void SetRunStepLength(fp32 _Val);
	void SetCharacterCollisions(bool _bSet);

	// Handles AI vs AI collision somehow
#define AI_COLL_AVOID_RANGE		128
	bool CheckCollisions();
	// Checks if we're stuck from lack of navgrid and if that is the case handles us moving out of the navgrid
	// Returns true if we really was stuck and false otherwise, called from within CheckCollisions() above
	bool CheckNavGridStuck();

	// Returns true if we should avoid _iCollider
	// Rules (descending priority)
	// Our collision rank is 0:					Don't avoid
	// Our collision rank is -1:				Avoid (yes, -1 coll rank avoid each other)
	// Either one is dead:						Don't avoid
	// Enemy relation with him:					Don't avoid
	// He is running a behaviour:				Avoid
	// We are running a behaviour:				Don't avoid
	// We are scripted
	//		He is not scripted:					Don't avoid
	//		He is scripted:						Avoid
	// We are crouching:						Don't avoid
	// He is crouching							Avoid (ie if both crouch nobody avoids)
	// He is angrier than we:					Avoid
	// We are angrier than he:					Don't avoid
	// We have higher collision rank than he:	Don't avoid
	// We have equal collision rank
	//		He has higher obj nbr:				Avoid
	// Nothing									Avoid
	bool ShouldAvoid(int _iCollider);
	// Handle collision between us and _iCollider
	// Returns true if we somehow took this into account and false if not
	bool HandleCollision(uint16 _iColl,uint16 _iColl2,fp32 _RangeSqr);
	bool HandleCollisionNew(uint16 _iColl,uint16 _iColl2,fp32 _RangeSqr);
	// Helpers for HandleCollisionNew
	int GetOctantFromOff(CVec3Dfp32 _Off);
	CVec3Dfp32 GetOffFromOctant(int _iOctant);

	bool MoveCollision();
	// Returns true when an AG2 effect is active
	bool MoveEffectAG2();
	int GetCollider();	// Returns the active collider if any

	bool m_bCharColl;				// When false, no character collisions occur
	bool m_bCollAvoidanceBlocked;	// When true we cannot move to avoid as we want, equal collRank should avoid too
	bool m_bCollCWBlocked;			
	bool m_bCollCCWBlocked;
	CVec3Dfp32 m_CollAvoidancePos;
	uint16 m_CollisionAvoidanceCount;	// The number of collision avoidances in a row

	// Turn on/off obj collision
	void SetObjColl(bool _bFlag);

	// Handle m_bWallClimb flag
	// Returns true if _bActivate was true and we really could set the flag
	bool SetWallclimb(bool _bActivate);
	bool TempEnableWallClimb(int32 _DurationTicks);
	bool TempDisableWallClimb(int32 _DurationTicks);
	// Disable jumping for whatever reason under _DurationTicks
	bool TempDisableJump(int32 _DurationTicks);

	// Set velocity to given amount in given direction, if possible
	void Push(fp32 _Speed, const CVec3Dfp32& _Direction);

	//Perception check of awareness attribute, Returns agent info awareness value.
	virtual int Perception_Awareness(int _iObj);

	// Measures the level of light on m_pGameObject
	void MeasureLightIntensity(bool _bTracelines = false);
	// Measures light at _Pos, skipping _iSkipObj
	fp32 MeasureLightIntensityAt(const CVec3Dfp32& _Pos,bool _bTracelines,int _iSkipObj);

	// Returns the light intensity from the last call to MeasureLightIntensity();
	fp32 GetLightmapIntensity();
	fp32 GetLightIntensity(int _iObj);
	fp32 GetCorpseLightIntensity(int _Interval);
	fp32 GetLightIntensityDelta();
	void HandleFlashlight(int _Duration);
	void HandleBrokenLights(int _Duration);
	// Returns true if we're allowed to use flash
	bool UseFlash(int _Duration, int _Prio);
	// Overrides the light threshold for turning on the flash for _Duration ticks
	// (Don't call this too often as it is pretty expensive, especially if m_UseLightmeter == 2)
	void OverrideFlashThreshold(fp32 _Threshold, int _Duration);

	// Returns the vision factor (0..1) visavi _iObj
	// That is, how well WE see _iObj
	fp32 GetVisionFactor(int _iObj);

	// Returns the hearing  factor (0..1) visavi _iObj
	// That is, how well WE hear _iObj
	// If _ListenerNoise is an optional noisefactor for the target
	// It is used instead of the targets noise due to movement
	fp32 GetHearingFactor(int _iObj);

	// Returns CAI_AgentInfo::SPOTTED when inside vision range and FOV
	// Returns CAI_AgentInfo::NOTICED when inside 2 * vision range and wide FOV
	// Returns CAI_AgentInfo::NONE otherwise
	int32 InsideFOV(CVec3Dfp32& _Pos);

	//Perception check of how well we see the given object with given basic visibility.
	//Returns agent info awareness value.
	// If _bObjectOnly is true we ignore any agent related sideeffects
	// If _Vis >= 0.0f we use that value as object illumination
	virtual int Perception_Sight(int _iObj,bool _bObjectOnly = false, fp32 _Vis = -1.0f);

	//Perception check of how well we hear the given object with given basic noise level, 
	//Returns agent info awareness value.
	//If the _bInArea flag is set, then this returns SPOTTED if the agent is in noise-modified
	//hearing range, effectively passing all hearing rolls automatically.
	virtual int Perception_Hearing(int _iObj, int _OwnLoudness, bool _bInArea);

	CVec3Dfp32 GetBasePos();
	CVec3Dfp32 GetBasePos(CWObject* _pObj);
	CVec3Dfp32 GetBasePos(int _iObj);
	CVec3Dfp32 GetBaseDir();
	CVec3Dfp32 GetLastBasePos();
	CVec3Dfp32 GetLastHeadPos();
	CVec3Dfp32 GetUp(); 
	CVec3Dfp32 GetLastUp();
	CVec3Dfp32 GetSideDir();
	CVec3Dfp32 GetLastSideDir();
	CVec3Dfp32 GetForwardDir();
	CVec3Dfp32 GetLastForwardDir();
	void GetBaseMat(CMat4Dfp32& _RetValue);


	// Timing stuff
	bool RefreshTicks();
	M_FORCEINLINE int32 GetGlobalAITick()
	{
		return(m_pAIResources ? m_pAIResources->GetAITick() : 0L);
	};

	M_FORCEINLINE int32 GetAITick() { return(m_ServerTick); };
	M_FORCEINLINE int32 GetAITicksPerSecond() { return(AI_TICKS_PER_SECOND); };
	M_FORCEINLINE fp32 GetAITickDuration() { return(AI_TICK_DURATION); };

	//Get corresponding object pointer
	M_FORCEINLINE CWObject* GetObject()
	{
		return(m_pGameObject);
	};

	//Hmm, I wonder what this method does?
	M_FORCEINLINE int32 GetObjectID()
	{
		if (m_pGameObject)
		{
			return(m_pGameObject->m_iObject);
		}
		else
		{
			return(0);
		}
	};

	//Get type of AI
	virtual int GetType();

	// Do class specific post script cleanup
	virtual void OnPreScriptAction(int _iAction, bool _bTake) { return; };

	//Get pointer to the AI of an object, or NULL if there isn't any
	static CAI_Core * GetAI(int _iObj, CWorld_Server* _pServer);
	CAI_Core * GetAI(int _iObj);

	//Get pointer to the agent's current item of given type or NULL if agent doesn't have an item of that type
	class CRPG_Object_Item * GetRPGItem(int _iObj, int _iType);

	//Get pointer to the agent's inventory of given type (RPG_CHAR_INVENTORY_WEAPONS etc) or NULL if agent doesn't have an inventory of that type
	class CRPG_Object_Inventory * GetRPGInventory(int _iObj, int _iType);

	// GetPrevSpeed() returns the length of last move
	// GetPrevVelocity() returns the last move
	// GetWantedWorldMove() returns the current frames move if any (just by collision handling)
	void UpdatePrevSpeed();
	fp32 GetPrevSpeed();
	CVec3Dfp32 GetPrevMoveDir();
	CVec3Dfp32 GetPrevVelocity();
	CVec3Dfp32 GetWantedWorldMove();

	//Free the search instance
	void ReleasePathSearch();
	
	//Resets some pathfinding stuff
	virtual void ResetPathSearch();

	// Returns the unit angle diff from our look at _Pos
	// If _bBelowValid then we only measure angle horizontally if _Pos is below
	fp32 AngleOff(const CVec3Dfp32& _Pos,bool _bValidBelow);

	//Returns the horizontal angle of the line between the given "from"- and "to"-positions, relative 
	//to the x coordinate axis or the given _Angle. If the _Interval argument is a value between 0 and 1, 
	//the result will be given in the interval [_Interval-1.._Interval]
	fp32 HeadingToPosition(const CVec3Dfp32& _FromPos, const CVec3Dfp32& _ToPos, fp32 _Interval = _FP32_MAX, fp32 _Angle = 0.0f);

	//Returns the vertical angle of the line between the given "from"- and "to"-positions, relative 
	//to the x coordinate axis or the given _Angle. If the _Interval argument is a value between 0 and 1, 
	//the result will be given in the interval [_Interval-1.._Interval]
	fp32 PitchToPosition(const CVec3Dfp32& _FromPos, const CVec3Dfp32& _ToPos, fp32 _Interval = _FP32_MAX, fp32 _Angle = 0.0f);

	fp32 HeadingToPosition2(const CVec3Dfp32& _FromPos, const CVec3Dfp32& _ToPos);
	fp32 PitchToPosition2(const CVec3Dfp32& _FromPos, const CVec3Dfp32& _ToPos);

	//Current heading of given object in fractions
	fp32 GetHeading(CWObject * _pObj);
	fp32 GetHeading();
	
	//Get cosine of first objects heading and it's heading to second object
	fp32 CosHeadingDiff(CWObject* _pObj1, CWObject* _pObj2, bool _bHeadingOnly = true);
	fp32 CosHeadingDiff(int32 _iTarget, bool _bHeadingOnly = true);

	//Checks if the position is path-traversable for the bot
	virtual bool IsTraversable(const CVec3Dfp32& _Pos);

	//Gets the position from which the character aims, the "activateposition"
	virtual CVec3Dfp32 GetTrueAimPosition();
	CVec3Dfp32 GetAimPosition();
	// Gets the position from which the agent aims, the "activateposition"
	CVec3Dfp32 GetAimPosition(CAI_AgentInfo* _pAgent);
	//Gets the position from which the given character aims, the "activateposition". This is incorrect for non-humanoid characters...
	CVec3Dfp32 GetAimPosition(CWObject * _pChar);
	CVec3Dfp32 GetAimPosition(uint16 _iObj);

	//Gets the transformation matrix of the engine path of the given server id at the given time 
	//(or current time as default), or CMat4Dfp32(_FP32_MAX) if the object is not an engine path
	CMat4Dfp32 GetEnginePathMatrix(int _iPath,int _Time = -1);
	
	//Gets the position of the engine path of the given server id at the given time 
	//(or current time as default), or CVec3Dfp32(_FP32_MAX) if the object is not an engine path
	CVec3Dfp32 GetEnginePathPosition(int _iPath,int _Time = -1);
	
	//Gets the center position of the objects bounding box, or CVec3Dfp32(_FP32_MAX) if this fails
	CVec3Dfp32 GetCenterPosition(CWObject * _pObj);

	//Gets the dimensions of the objects bounding box
	CVec3Dfp32 GetBoundBoxDimensions(CWObject * _pObj);

	//Gets the position to focus on, which is activateposition for characters and center of bounding box for other objects
	CVec3Dfp32 GetFocusPosition(CWObject* _pObj);

	//Get believed focus position
	CVec3Dfp32 GetFocusPosition(CAI_AgentInfo * _pObj);

	//A bit above center position of bounding box (center position is often between the legs of characters)
	CVec3Dfp32 GetTorsoPosition(CWObject * _pObj);

	//Get believed torso position
	CVec3Dfp32 GetTorsoPosition(CAI_AgentInfo * _pObj);

	//Face position or a bit below aim position for characters
	CVec3Dfp32 GetVulnerablePosition(CWObject * _pObj);

	// Returns the actual position of the eyes of the character
	virtual CVec3Dfp32 GetHeadPos();
	CVec3Dfp32 GetHeadPos(int _iObj);
	CVec3Dfp32 GetHeadPos(CWObject* _pObj);
	void GetHeadMat(CMat4Dfp32& _RetValue);

	// Returns the pos to use for look calculations. This is NOT the same as GetHeadPos()
	CVec3Dfp32 GetLookPos();

	// Returns the current look direction
	CVec3Dfp32 GetLookDir();
	CVec3Dfp32 GetLookDir(CWObject* _pObj);

	//Check LOS between given positions. Succeeds if we hit something other than optionally excluded object.
	//If given, the optional collision info is set appropriately.
	bool IntersectLOS(const CVec3Dfp32& _Start, const CVec3Dfp32& _End, int _iExclude = 0, CCollisionInfo * _pInfo = NULL);

	//Checks if there is LOS from the given position to the target. The positions actually checked for LOS are
	//those specified in the target offsets array added to the targets position. If an object is to be excluded 
	//from the intersection checking, the server index to that object should be given. Returns the first position 
	//found where there is LOS, or CVec3Dfp32(_FP32_MAX) if there is no LOS. 
	CVec3Dfp32 CheckLOS(const CVec3Dfp32& _Pos, CWObject * pTarget, const CVec3Dfp32 *_TargetOffsets, int _nTargets, int _iExclude = 0);

	//Default LOS check, with object pointer as argument. Checks LOS from character activateposition to a few 
	//positions on the target
	CVec3Dfp32 CheckLOS(CWObject* _pTarget);

	//Default LOS check, with object server index as argument. Checks LOS from character activateposition to a 
	//few positions on the target
	CVec3Dfp32 CheckLOS(int _iTarget);

	// Faster version of CheckLOS that doesn't bother with where the LOS was stopped
	// It basically returns true if _Start can see _End
	bool FastCheckLOS(const CVec3Dfp32& _Start,const CVec3Dfp32& _End,int _iExclude0, int _iExclude1);
	bool FastCheckLOS(CWObject* _pTarget);
	bool FastCheckLOS(int _iObj);
	
	//Get health/maximum health of given agent (or own agent if no param is given)
	int Health(int _iObj);
	int Health();
	int MaxHealth(int _iObj);
	int MaxHealth();

	//Check if we're below the given fraction of our maximum health
	bool IsHurt(fp32 _Threshold = 1.0f);
	//Returns the fractional amount of health left (Health/MaxHealth)
	fp32 GetHealthFrac();

	//Check if given object is a character
	bool IsCharacter(int _iObj);

	// Checks if _iObj is a valid target
	virtual bool IsValidTarget(int _iTarget);

	//Checks if the given agent is "alive", not no-clipping and not the bot himself
	bool IsValidAgent(CWObject * _pObj);
	
	//Checks if the given server index points to an agent that is "alive", not no-clipping and
	//not character self. Returns a pointer to the object if so, NULL otherwise
	CWObject * IsValidAgent(int _iObj);

	bool HasOrder(int _Order = ORDER_NONE)
	{
		return((m_Order == _Order) ? true : false);
	};
	// Receive the order, return true if we can handle it
	bool GiveOrder(int16 _Order, int _DurationTicks);

	// Returns, as a fractional value, the nbr of mags fired (0.0 means gun is full)
	fp32 GetMagsFired();

	// Tries to do a reload action, retuns true if successful
	// _MagFrac is the (fractional) nbr of mags fired for a reload to occurs
	bool Reload(fp32 _MagFrac = 1.0f);

	//Check if the given object is AI-controlled
	bool IsBot(int _iObj);

	//Checks if the object of the given server index is a player
	bool IsPlayer(int _iObj);

	//Checks if 'this' is a player
	bool IsPlayer();

	// If true we can add this ai as an agent
	bool CanAddAgent();

	//Check if given character is an hostile+
	// When _DeadOK is true we only base the retuen on the relation
	bool IsHostileOrWorse(CWObject* _pChar,bool _DeadOK = false);

	//Checks if character pointed at by given server index is a hostile+. 
	// When _DeadOK is true we only base the retuen on the relation
	CWObject* IsHostileOrWorse(int _iObj,bool _DeadOK = false);

	//Check if given character is an enemy
	// When _DeadOK is true we only base the retuen on the relation
	bool IsEnemy(CWObject* _pChar,bool _DeadOK = false);

	//Checks if character pointed at by given server index is an enemy. 
	//Returns object pointer if so, else NULL. 
	CWObject* IsEnemy(int _iObj,bool _DeadOK = false);

	//Check if given character is a friend
	bool IsFriend(CWObject * _pChar);

	//Checks if character pointed at by given server index is an ally.. 
	//Returns object pointer if so, else NULL. 
	CWObject * IsFriend(int iObj);

	//Check if the given position is in the given area info
#ifndef M_DISABLE_TODELETE
	bool IsInArea(int _iArea, const CVec3Dfp32& _Pos);
#endif

	//Check if the given position is within the given objects bounding box
	bool IsInBoundBox(CWObject * _pObj, const CVec3Dfp32& _Pos);

	//Adds commands that changes the characters look direction to look at the given position.
	//This can change both the heading and pitch of the characters look direction.
	//This change of looking will occur over _Time
	virtual void AddAimAtPositionCmd(const CVec3Dfp32& _Pos,int _Time = -1);

	//Adds commands that changes the movement direction of the character to 
	//head towards the given position
	virtual void AddFacePositionCmd(const CVec3Dfp32& _Pos,int _Time = -1);

	//Adds commands to move towards the given position, at the given speed or full speed by default. 
	//Optionally we can specify that we want to get to the exact given xy-position. This will override animgraph control.
	virtual bool AddMoveTowardsPositionCmd(const CVec3Dfp32& _Pos, fp32 _Speed = -1.0f);

	//Adds commands to jump towards the given position.
	//REturns true if a jump is possible and false if not
	virtual bool AddJumpTowardsPositionCmd(const CMat4Dfp32* _pMat, int _Reason, int _Flags);

	//Checks if we've gotten stuck, given the destination
	virtual bool IsStuck(const CVec3Dfp32& _Dest);
	void ResetStuckInfo();
	
	//Check if we are perfectly placed at given xy-position. Optionally we can demand that look is 
	//perfect as well and change the height diff tolerancy
	bool IsPerfectlyPlaced(CVec3Dfp32 _Pos, CVec3Dfp32 _LookVec = CVec3Dfp32(_FP32_MAX), fp32 _HeightDiffToleracy = 64.0f);

	// Returns the movemode the character is currently using
	// Returns MOVEMODE_CROUCH, MOVEMODE_STAND, MOVEMODE_WALK or MOVEMODE_RUN
	virtual int GetMoveMode();

	//Add commands to follow the path at given speed, optionally with perfect placement
	virtual void OnFollowPath(fp32 _Speed);

	//Make moves to escape from a "stuck" position
	//_iPath is an optional supplied path which will be propelled a bit when we're stuck
	virtual void OnEscapeSequenceMove(fp32 _Speed);

	// Refreshes targets, hostiles and friends to see if they are still valid
	void RefreshInteresting();
	int32 RefreshInteresting(int32 _iObject,int32 _Type);

	// Switch to a newly detected target?
	// If _bReplace we automatically swicth, if not we evaluate the best target of the two
	virtual bool SwitchTarget(int32 _iTarget,bool _bReplace = true);
	// Same as for _iTarget above but for hostiles
	virtual bool SwitchHostile(int32 _iTarget,bool _bReplace = true);
	// Same as for SwitchTarget but with friends
	virtual bool SwitchFriendly(int32 _iFriend, bool _bReplace = true);
	// Same as for SwitchTarget but with NOTICED enemies, valid for searches
	virtual bool SwitchInvestigate(int32 _iFriend, bool _bReplace = true);
	
	// GetInterestingObject
	// Returns true if _Pos is an interesting object
	enum
	{
		INTERESTING_NOTHING = -1,
		INTERESTING_LOOK = 0,
		INTERESTING_FRIEND = 1,
		INTERESTING_HOSTILE = 2,
		INTERESTING_ENEMY = 3,
	};
	int32 GetInterestingObjects(int32& _Type, CVec3Dfp32& _Pos, CVec3Dfp32& _Pos2);
	
	//Causes the bot to move towards it's current destination or the new destination if 
	//such is given at given speed or maximum speed by default.
	//Succeed if we can (try) to move or is pathfinding, fail otherwise
	enum
	{
		MOVE_PATHFIND_FAILURE = -4,		// Other pathfind failure (timeout, missing pathfinder etc)
		MOVE_SRC_INVALID = -3,			// Startposition invalid
		MOVE_DST_INVALID = -2,			// Endposition invalid
		MOVE_DEVICE_BUSY = -1,			// Move device busy (or bot is jumping)
		MOVE_WAIT_NO_PATHFINDER = 0,	// Waiting for a pathfinder (Pathfinding is working for someone else)
		MOVE_WAIT_PATHFINDING = 1,		// Pathfinding is working (for us)
		MOVE_OK = 2,					// Move OK
		MOVE_DONE = 3,					// Close enough to do PP
	};
	virtual int32 OnMove(const CVec3Dfp32& _NewDestination, fp32 _Speed, bool _bFollowPartial, bool _bPerfectPlacement, CWO_ScenePoint* _pScenePoint);
	bool CheckMoveresult(int32 _MoveResult, CWO_ScenePoint* _pScenePoint);

	bool CheckWalkStop(CVec3Dfp32 _Dest, fp32 _Range = 32.0f);
	bool CheckWalkStart(CVec3Dfp32 _Dest, fp32 _Range = 48.0f);
	void SetDestination(CVec3Dfp32 _Dest = CVec3Dfp32(_FP32_MAX),fp32 _Velocity = 0.0f);
	void SetPerfectPlacement(const CMat4Dfp32* _pMDest,fp32 _Velocity = 0.0f);
	void SetPerfectPlacement(CWO_ScenePoint* _pScenePoint = NULL, bool _bReversedLook = false);
	void SetPerfectOrigin(const CMat4Dfp32* _pMDest);
	void SetPerfectOrigin(CWO_ScenePoint* _pScenePoint);
	// Same as SetPerfectOrigin(NULL) but unambigious
	void ClearPerfectOrigin();
	void StopNow();
	void NoStops();
	// Stop using DEST_PP or DEST_PO
	void NoPerfectPlacement();

	// Call whenever a pathfind fails
	// If _bJump is true we try to jump to m_Pathfinder.GetDestination();
	void PathfindFailed(bool _bJump);

	// Sets up a path to follow using _iFollowMode
	// Call OnGroundFollowPath to actually follow the data
	void SetPath(CAI_FollowPathData* _pFollowData, int _iPath, int _iFollowMode = CAI_FollowPathData::FOLLOW_NORMAL);
	void OnGroundFollowPath(CAI_FollowPathData* _pFollowData);

	// *** Temporary solution until the action and objective systems are fully implemented ***
	// Returns true if we can attack the target ie attack angles and possibly ranges are OK
	// Behaviour drive bots will always return true
	virtual bool CanAttack(CAI_AgentInfo* _pObj);

	//Causes the bot to attack the given target
	virtual void OnAttack(CAI_AgentInfo * _pObj);

	//Causes the bot to aim at the given target
	virtual void OnAim(CAI_AgentInfo * _pObj);

	// Handle being mounted
	enum
	{
		MOUNTFLAGS_WORLDTRACKING	= 1,
		MOUNTFLAGS_BURST			= 2,
	};
	bool HandleMount(uint32 _Flags,int32 _iPrimary,CVec3Dfp32* _pTargetPos);

	//Get random traversable position given position matrix (which defines the search origin and direction)
	//and constraints on distance, density of search positions and deviation. Returns CVec3Dfp32(_FP32_MAX) on failure.
	virtual CVec3Dfp32 FindRandomPosition(const CMat4Dfp32& _Mat, fp32 _Distance, fp32 _Deviation = 0.5f, fp32 _Density = 16.0f, fp32 _MaxDev = 0.25f);

	//Convenient overload; uses characters current position matrix
	virtual CVec3Dfp32 FindRandomPosition(fp32 _Distance, fp32 _Deviation = 0.5f, fp32 _Density = 16.0f, fp32 _MaxDev = 0.25f);

	//Attempts to find a ground-traversable position (i.e. one that can be reached by travelling in a straight line 
	//along the ground) in the given heading at the given distance. If this fails, it will try to find the closest 
	//position at that distance or closer, trying new heading with the given angle interval (90 degrees as default). 
	//If the _angle argument is specified it will only try to find positions within the arc of that angle, centered 
	//around the heading and if the _MinDist argument is given it will only try to find closer positions down to 
	//this minimum distance. Returns CVec3Dfp32(_FP32_MAX) if no position is found. If the nGroundCells argument is specified 
	//the position must also have at least that many cround cells.
	CVec3Dfp32 FindGroundPosition(fp32 _Heading, fp32 _Distance, fp32 _MinDist = 0.0f, fp32 _Interval = 0.25f, fp32 _Angle = 1.0f, int nGroundCells = 1);

	//Causes the bot to take fast, short-distance evasive actions, preferrably stepping out of enemy LOS. If a move 
	//position is specified, bot will try to move towards that general direction while maintaining evasive manoeuvres 
	//If a face position is specified the bot will face that position while evading. Fails if there is nothing to evade
	//or evasive manoeuvres are impossible and we don't have a movement position to move towards. The _pbIsTrapped pointer 
	//points to an aditional result variable that is set to true if we've been cornered and cannot evade, or false otherwise.
	// virtual bool OnEvade(const CVec3Dfp32& _FacePos = CVec3Dfp32(_FP32_MAX), const CVec3Dfp32& _MovePos = CVec3Dfp32(_FP32_MAX), bool * _pbIsTrapped = NULL);

	//Bot makes a normal move to follow a target, taking evasive actions if feeling threatened
	// virtual void OnFollowTarget(CAI_AgentInfo * _pTarget);
	//Same as above but no evasions or anything
	virtual void OnBasicFollowTarget(CAI_AgentInfo * _pTarget,fp32 _Speed = -1.0f);
	// Same as above but for investigating phenomena instead
	virtual void OnBasicInvestigatePos(CVec3Dfp32& _Pos,fp32 _Speed,fp32 _GoalRange = 48.0f);

	// All MakeNoise et al calls goes through these method first
	// So all CAI_Core subclasses can override sound behaviours
	// Returns the variant used if a sound was actually played and -1 if none was played
	int UseRandom(CStr _Reason, int _iType, int _Prio);
	int UseRandom(CStr _Reason, int _iType, int _Prio, int _iVariant);
	// Delayed speech utterance
	void UseRandomDelayed(CStr _Reason, int _iType, int _Prio, int _DelayTicks);

#ifndef M_RTM
	void DebugPrintSoundReason(CStr _Reason, int _iType, int _iVariant = -1);
	// void DebugPrintSoundReason(CStr _Reason, CStr _sSound);
#endif

	//Raise an alarm about the given object. Optionally you can specify alarm information level and 
	//the relation recipients of the alarm should assume versus the cause
	void RaiseAlarm(int _iCause, int _Relation, bool _RangeBased);

	// Raise an alarm about finding a corpse _iVictim killed by _iPerp at _Pos
	void RaiseFoundCorpse(int _iPerp, int _iVictim, CVec3Dfp32 _Pos);

	//Check if any team friend can ses _iCause and if they can; set relation to _Relation
	void RaiseFight(int _iCause, int _Relation);

	//Raise or refresh the awareness of an object
	//iCause is the object for the awareness, _Pos its position according to the raiser and
	//_Awareness is the awareness level
	void RaiseAwareness(int _iCause, CVec3Dfp32 _Pos,int _Awareness,int _Relation = CAI_AgentInfo::RELATION_INVALID);

	// Send message to all teammembers forcing their knwowledge of to go to nothing
	// if NOTICED
	void RaiseSearchOff(int _iSuspect);

	//Sends a message to an object, all friends, all allies, all enemies or all characters.
	void SendMessage(int _iMode, const CWObject_Message& _Msg, int _iObject = 0);

	// Returns the best communication available between _iObject and us
	// If -1 is returned _iObject isn't even a teammember
	int GetBestCommunication(int _iObject);

	//Get non-harmless weapon, and initialize weapon handler. If the optional param is true, then 
	//we don't care if the current weapon is non-harmless or inappropriate.
	void GetWeapon(bool bCurrentOnly = false);

	//Get current item
	void GetItem();

	// Looks at dialogue partner if that is the case
	bool HandleDialoguePartner();

	//Chooses the actions the bot should take at the current time.
	virtual void OnTakeAction();

	// Stops all kinds of dialogues
	bool ShutUp(int _Prio = CAI_Action::PRIO_COMBAT);
	// _bWeAreBumping == true we bumped them, otherwise they bumped us
	// _iObj what object bumped/bumping
	// _pCollisionInfo To get actual impact pos etc
	virtual void OnBumped(bool _bWeAreBumping,int _iObj,CVec3Dfp32& CollPos);

	//Handles the event of us taking damage
	virtual void OnTakeDamage(int _iAttacker, int _Damage = 0, uint32 _DamageType = DAMAGETYPE_UNDEFINED, int _Hitloc = CWObject_Character::SHitloc::HITLOC_MID);
	void OnTakeDamageAG2(int _iAttacker, int _Damage, uint32 _iDamageType, int _Hitloc);
	void OnBumpedAG2(int _iAttacker, CVec3Dfp32 _Dir = CVec3Dfp32(_FP32_MAX));
	
	//Change the direction (heading and pitch) the bot looks at. The change will take place gradually 
	//over the course of the given number of frames.
	virtual void OnLook(fp32 _Heading, fp32 _Pitch = _FP32_MAX, int _Time = 1, bool _bLookSoft = false);

	//Track position with given time delay. Optionally we only look horizontally and optionally we 
	//only look as far as we can given movement direction without having to strafe or move backwards
	virtual void OnTrackPos(const CVec3Dfp32& _Pos, int _Time,bool bHeadingOnly, bool _bLookSoft);
	virtual void OnTrackObj(int _iObj,int _Time,bool bHeadingOnly, bool _bLookSoft);
	virtual void OnTrackAgent(CAI_AgentInfo* pAgent,int _Time,bool bHeadingOnly = false, bool _bLookSoft = false);
	// Similar to OnTrackPos above but takes a direction vector instead
	virtual void OnTrackDir(CVec3Dfp32 _Dir,int _iObj, int _Time, bool bHeadingOnly, bool _bLookSoft);
	// Update look for time delayed tracking (previously handled by OnLook)
	void RefreshLook();
	//Tracks the targets head if any, returning iether the current look dir or the new lookdir
	// Retreats backwards if target is inside _RetreatRng
	CVec3Dfp32 OnWeaponTrackHead(CAI_AgentInfo* _pTarget,int _TrackTicks = 0,fp32 _RetreatRng = 0.0f, bool _bTrack = true);

	// General methods for aligning with a scenepoint/matrix over time
	bool OnAlign(CWO_ScenePoint* _pScenePoint, int _iObj, int _Time, bool _bReverseDir = false);
	bool OnAlign(const CMat4Dfp32* _pMDest, int _iObj, int _Time);
	// General methods for aligning with a scenepoint/position over time
	// Returns true when we are sufficiently close to do PP
	bool OnNudge(CWO_ScenePoint* _pScenePoint, fp32 _Speed, bool _bUseOffset = true);
	bool OnNudge(CVec3Dfp32 _Pos, fp32 _Speed);

	//Scratching our butt...
	virtual void OnIdle();

	//Act frustrated
	virtual void OnFrustration();

	//Create and return default behaviour
	// virtual spCAI_Behaviour DefaultBehaviour();

	//Sets/creates action from given action type, parameter target name, distance and max distance parameters(if any)
	virtual bool SetAction(int _iAction, CStr _sParamName = "", fp32 _DistParam = -1.0f, fp32 _MaxDistParam = -1.0f, int _iActivator = 0);

	//Sets behaviour from keys
	virtual void SetActionFromKeys();

	//Helper to OnImpulse; gets single random object to use as parameter to some impulses. Will prefer characters
	//before other objects and valid characters before other characters
	CWObject * GetSingleTarget(int _iObject, CStr _pName, int _iSender);

	//Get index of closest human player
	int GetClosestPlayer();
	//Get index of closest teammate with _ActionType within _MaxRange
	int GetClosestActionUser(int _ActionType,fp32 _MaxRange = _FP32_MAX, bool _bActive = false);
	int GetClosestActionUserWithSkiplist(int _ActionType,fp32 _MaxRange, bool _bActive, int* _pSkipArray, int _nArraySize);

	//Gets animation resource index from the string on the format <FILEHANDLE>;<ANIM ID>
	int ResolveAnimHandle(const char *_pSequence);
	
	// Turns animgraph refresh on or off according to _Deactivate
	void ActivateAnimgraph();
	int m_bCanDeactivateAG;
	void DeactivateAnimgraph();
	void SetAnimgraphDeactivation(bool _Deactivate);

	bool SetWantedGesture(uint32 _iGesture, int32 _DurationTicks = AI_TICKS_PER_SECOND * 2);
	bool SetWantedMove(uint32 _iMove, int32 _DurationTicks = AI_TICKS_PER_SECOND * 2);

	M_FORCEINLINE bool IsEnteringBehaviour() { return(m_bBehaviourEntering); }
	M_FORCEINLINE bool IsExitingBehaviour() { return(m_bBehaviourExiting); }

	int32 GetCurrentBehaviour();

	// SetWantedBehaviour2:
	// _iBehaviour:		The behaviour nbr to be played
	// _Prio:			Priority of the behavior
	// _Flags:
	// BEHAVIOUR_FLAGS_LEFT		= 0,		// For completeness
	// BEHAVIOUR_FLAGS_FAST		= 1,		// Fast transition to/from behaviour main
	// BEHAVIOUR_FLAGS_DIRECT	= 2,		// Go to/from behaviour main, "...do not collect any money"	
	// BEHAVIOUR_FLAGS_RIGHT	= 4,		// Right version behaviour, also used as second version in two behaviours
	// BEHAVIOUR_FLAGS_TOP		= 8,		// Top/fwd version of behaviour
	// BEHAVIOUR_FLAGS_BOTTOM	= 16,		// Bottom/rear version of behaviour
	// BEHAVIOUR_FLAGS_LOOP		= 32,		// Loop
	// BEHAVIOUR_FLAGS_EXIT_MH	= 64,		// Exit to MH
	// BEHAVIOUR_FLAGS_PP		= 128,		// Set perfect placement
	// BEHAVIOUR_FLAGS_PO		= 256,		// Set perfect origin
	// _Duration:		Duration of behaviour, i <= 0 the behaviour is looping
	// _pScenePoint:	Optional scenepoint to play the behaviour at
	// _pMPerfectPlacement:	Perfect placement matrix when _pScenePoint was not given
	bool SetWantedBehaviour2(uint32 _iBehaviour,
							uint32 _Prio,
							uint32 _Flags,
							int32 _Duration,
							CWO_ScenePoint* _pScenePoint = NULL,
							CMat4Dfp32* _pMPerfectPlacement = NULL);

	// StopBehaviour: Tries to stop a running behaviour
	// _HowFast: Can be any of BEHAVIOUR_STOP_NORMAL, BEHAVIOUR_STOP_FAST or BEHAVIOUR_STOP_DIRECT
	// _Prio: Is the prio of the stop, prio must be EQUAL or higher to stop a running behaviour
	void StopBehaviour(int32 _HowFast = BEHAVIOUR_STOP_NORMAL, uint32 _Prio = CAI_Action::PRIO_IDLE, int16 _StopDir = 0);
	// void StopWantedBehaviour2(int32 _HowFast = BEHAVIOUR_STOP_NORMAL, bool _bForceSend = false);
	void OnBehaviourFinished(int _iBehaviour);
	void OnBehaviourMainStarted(int _iBehaviour);
	bool IsBehaviourSupported(int _iBehaviour);

	// Reset behaviour stuff. Should be called when we know we're not in behaviour state anymore.
	void ResetBehaviour();

	// InitCombatBehaviours: Setup ai to start handling combat behaviours
	bool InitCombatBehaviours(CWO_ScenePoint* _pScenePoint, int32 _Prio, int32 _iTarget);
	// Call repeatedly to service combat behaviours
	bool HandleCombatBehaviours();
	// _StopDir: 0 = Fwd, 1 = Left, -1 = Right
	void ClearCombatBehaviours(int16 _StopDir = 0);

	//Drag the given object, if possible. Currently only usable on dead ragdoll characters.
	void DragObject(int _iObj); 

	//Handles impulses.
	virtual bool OnImpulse(int _iImpulse, int _iSender = 0, int _iParam = 0, const CVec3Dfp32& _VecParam = 0, void * _pData = NULL);

	//Calculate pathfinding prio
	virtual uint8 GetPathPrio();

	//Forces the bots' body direction to align towards it's look direction 
	//at the given fraction of the direction change needed 
	void ForceAlignBody(fp32 _Amount = 1.0f);

	//Check if body is aligned with look direction, within a given fraction
	bool IsBodyAligned(fp32 _Epsilon = 0.0f);

	//How long do we have to wait before an item or weapon can be activáted?
	int Wait();

	//How long do the given agent have to wait before an item or weapon can be activáted?
	int Wait(int _iObj);

	// Draws the supplied ep 
	void DebugDrawPath(int32 _iPath,int32 _Steps, fp32 _Duration = 0.0f);

	//Propel an engine path given distance etc. If given, result is the new position of the engine path.
	void PropelPath(int _iPath, fp32 _Distance, bool _bAllowStops = true, bool _bXYOnly = false, bool _bPropel = true, CMat4Dfp32 * _pRes = NULL);
	// Sets the time of _iPath to _Time, sending messagges if _bSendMessages is true, returns pos in _pRes if supplied
	void SetTimePath(int _iPath, fp32 _Time, bool _bSendMessages = true, CMat4Dfp32 * _pRes = NULL);
	// Returns the pos of path _iPath at _TimeAhead seconds in the future of _iPath
	bool GetTimeAheadPath(int _iPath, fp32 _TimeAhead, CMat4Dfp32 * _pRes);

	//Is given agent using a weapon?
	bool UsingWeapon(int _iObj);

	// Returns the index of the nearest teammember to us
	// If _bComm is true only teams with voice or direct communications are considered
	// _iIgnore is a member indexs to skip (pass 0 if none should be skipped)
	// _RangeSqr will hold the squared range to the returned member 
	int GetClosestTeammember(bool _bComm,int _iIgnore,fp32* _RangeSqr);
	bool IsTeammember(int _iObj);

	//Get best security clearance of any team given agent belongs to
	int GetSecurityClearance(int _iObj);

	//Get positions/objects squared distance to us
	fp32 GridSqrDistanceToUs(const CVec3Dfp32& _Pos);
	fp32 SqrDistanceToUs(const CVec3Dfp32& _Pos);
	fp32 SqrDistanceToUs(CWObject * _pObj);
	fp32 SqrDistanceToUs(int _iObj);

	// Projects _Pos onto baseplane of us
	CVec3Dfp32 Proj2Base(CVec3Dfp32 _Pos);
	fp32 SqrFlatDistance(CVec3Dfp32 _Pos);

	//Get AI resource handler (the hard way)
	CAI_ResourceHandler * GetAIResources();

	//Get scenepoint manager
	CWObject_ScenePointManager* GetScenePointManager();

	//Add indices of all human players to given array
	void AddPlayers(CWorld_Server * _pServer, TArray<int>& _lResult);
	void AddPlayers(CWorld_Server * _pServer, TArray<int16>& _lResult);
	
public:
	//Constructor and destructor
	CAI_Core();
	~CAI_Core();

	//Copy constructor (use with caution)
	CAI_Core(CAI_Core * _pAI_Core);

	//Initializes the AI, setting it up for action...
	virtual void Init(CWObject * _pCharacter, CWorld_Server * _pServer);

	//Destroys the AI, releasing any held resources. Optionally we might want to refrain from raising the OnDie event.
	virtual void Destroy(bool _bRaiseOnDie = true);

	//Arrgh! Oh what a cruel world! Call when dying normally (i.e. not when script-destroyed)
	virtual void OnDie(bool _bRaiseOndie = true);

	//Is the AI valid, i.e.does it have a server, character, character RPG object, behaviour, personality etc.
	//If so specified the AI is not considered valid if it's dead, noclipping or immobile, also.
	virtual bool IsValid(bool _bStrict = false);

	virtual bool CheckScenepoint(CWO_ScenePoint* _pSP); 

	// Returns true if the characters is alive and not stunned nor oblivious
	bool IsConscious();

	// If _StunTime == -1 the undead will be stunned forever
	// If _StunTime > 0 the undead will be stunned _StunTicks ticks
	// If _StunTime == 0 the undead will wake up next tick
	// Returns true if the call was valid for the bot
	bool SetUndeadStun(int _StunTicks, bool _bDirect = false);
	bool SetAgonyStun();

	//Updates the bot one frame. If the _bPassive flag is true, no actions are considered or generated, 
	//but information is gathered.
	virtual void OnRefresh(bool _bPassive = false);
	void OnRefreshDestination();
	void OnRefreshPosMats();
	void OnRefreshClimb();
	// Save last behaviour, get new currently running behaviour
	void OnRefreshBehaviours();

	bool IsMounted(CMat4Dfp32& _Mat);

	//Handles incoming messages
	virtual aint OnMessage(const CWObject_Message& _Msg);

	//Gets the AI control frame, containing the commands that the bot should currently perform
	virtual const CControlFrame* GetControlFrame();

	//Handles any registry keys of a character that are AI-related
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();

	// Adds all used behaviours to _liBehaviours, returns the nbr added
	virtual int GetUsedBehaviours(TArray<int16>& _liBehaviours) const;
	virtual int GetUsedGesturesAndMoves(TArray<int16>& _liGestures, TArray<int16>& _liMoves) const;
	virtual int GetUsedAcs(TArray<int16>& _liAcs) const;

	//Should run when world has been spawned
	virtual void OnSpawnWorld();

	// Load/save character status
	virtual void OnDeltaLoad(CCFile* _pFile);
	virtual void OnDeltaSave(CCFile* _pFile);
	virtual void OnPostWorldLoad();

	//The more "action" the player thinks there currently is, the higher value is returned. 
	//Currently this is 1 when bot thinks an enemy has spotted it, and 0 otherwise
	virtual fp32 GetTension();
#define GET_TENSION_BATTLE	1.0f
//#define GET_TENSION_TENSION_SEARCH		0.5f
#define GET_TENSION_TENSION_HOSTILE_2	0.3f
#define GET_TENSION_TENSION_HOSTILE_1	0.2f
#define GET_TENSION_TENSION_HOSTILE		0.1f

	bool HighTension(int _CalmTime = 100);	// Returns true if Tension has been low for atleast _CalmTime ticks

	//Tension for stealth stuff; returns any of the following values
	enum {
		TENSION_NONE = 0,	//Bots are scratching their collective butt
		// TENSION_SEARCHING,
		TENSION_HOSTILE,	//Searching, or angry but not outright fighting
		TENSION_COMBAT,		//Guards are fighting you 
	};

	// Returns TENSION_NONE, TENSION_HOSTILE or TENSION_COMBAT
	// If _UpdateFlag == true we do a real update from knowledge base etc
	virtual int GetStealthTension(bool _UpdateFlag = false);
	void SetMinStealthTension(int _Tension, bool _bRightNow = false);
	void SetStealthTension(int _Tension, bool _bRightNow = false);
	// Returns the index of the player if we're scared of darkness and it is active
	// 
	int GetPlayerDarkness(int32* _piCreepDark, CVec3Dfp32* _pCreepDarkPos);
	void SetPlayerDarkness(uint16 _DarknessType);
	
	// Setting/getting and clearing tentacle globals
	void SetDarknessPos(CVec3Dfp32 _Pos);
	void OnRefreshDarkness();
	

	//Get itemtype of recommended weapon to start with.
	virtual int GetRecommendedWeapon();

	//Calculate the activateposition matrix for the bot. This is the character position matrix,
	//possibly modified by the wielded weapon.
	virtual void GetActivatePosition(CMat4Dfp32* _pMat);

	// Helper method for getting the player look pos and dir
	// Returns true if _Look and _EyePos are valid
	virtual bool GetPlayerLookAndPos(CVec3Dfp32& _LookDir,CVec3Dfp32& _EyePos);
	// Returns true when player look is within _CosAngle
	bool IsPlayerLookWithinCosAngle(fp32 _CosAngle);
	// Returns true when player look is within _Angle
	bool IsPlayerLookWithinAngle(fp32 _Angle);
	bool IsPlayerLookWithinAngle(fp32 _Angle,const CVec3Dfp32& _TargetPos);
	// Somewhat expensive generalized FOV method
	bool IsLookWithinAngle(fp32 _Angle,int _iBot);

	//Is bot active?
	virtual bool IsActive();

	//Set activation state to given state if possible
	virtual bool SetActivationState(int _State);

	//Checks if the bot should be activated, and sets activate flag if so. 
	virtual bool OnActivationCheck();

	//Checks if the bot should be deactivated
	virtual bool OnDeactivationCheck();

	//Pause/Release behavior-controlled bot. Scripting will still work
	void PauseBehaviour();
	void ReleaseBehaviour();
	//Pause/Release behavior-controlled bot. Scripting will still work
	void PauseScripts();
	void ReleaseScripts();

	//Sets activity stuff
	void SetActivityScore(int _Score);
	void SetActivityLevel(int _Level, bool _bForce = false);
	void OnActivityCheck();	

	void SetPathDestination(const CVec3Dfp32& _Pos);
	
	//Custom OnIncludeSound method, to take special sound syntax in consideration
	static void IncludeSoundFromKey(CStr _Key, CRegistry * _pReg, CMapData * _pMapData);

	//Should run when including a template
	static void OnIncludeTemplate(CRegistry * _pReg, CMapData * _pMapData);

	//Construct a new AI core object, of the type denoted by the given string, from the given one
	static spCAI_Core GetAIFromString(CStr _Str, CAI_Core * _pAI, CWorld_Server * pServer,  CWObject * _pChar);

	//Gets all object server indices with the specified target name on the specified server and 
	//appends them to end of the given array
	static void GetTargetIndices(CStr _sTargetName, CWorld_Server * _pServer, TArray<int16>& _lResult);

	//The distance in the horizontal plane only between two points
	static fp32 XYDistance(const CVec3Dfp32& _p1, const CVec3Dfp32& _p2);
	
	//The squared distance in the horizontal plane only between two points
	static fp32 XYDistanceSqr(const CVec3Dfp32& _p1, const CVec3Dfp32& _p2);

	//Creates a CVec3Dfp32 from a string if it is of the (<X>,<Y>,<Z>) format, or CVec3Dfp(_FP32_MAX) otherwise
	static CVec3Dfp32 GetVectorFromString(CStr _Str, char _cSeparator = ',');

	//Get given object's local (tilt, pitch, heading) vector in fractions
	// If reverse is true we get the reverse angles
	CVec3Dfp32 GetLookAngles(CWObject* _pObj,bool _Reverse = false);
	CVec3Dfp32 GetLookAngles(bool _Reverse = false);

	// Converts Lookangles to direction _Look[Tilt,Pitch,Heading] to [x,y,z] ignoring tilt
	CVec3Dfp32 LookAngles2LookDir(CVec3Dfp32 _Look);

	//Returns the difference in fractions between two angles. Return value is in the interval 0..0.5, of course
	static fp32 AngleDiff(fp32 _a1, fp32 _a2);

	//As above, but absolute diff
	static fp32 AbsAngleDiff(fp32 _a1, fp32 _a2);

	//Checks if given target name is reserved
	static bool IsReservedTargetName(CStr _Name);

	//Get pointer to NonZeroList with all spawned agents indices.
	//Do not add or remove agents outside of knowledge base!
	CSimpleIntList * GetAgents();

	// Flags for _pFlags below
#define AI_BEYOND_DIALOGUERANGE		64.0f
#define AI_WAYBEYOND_DIALOGUERANGE	96.0f
	enum
	{
		PLAYER_RELATION_SEENDARKNESS = 1,	// The character has seen Jackies darkness
		PLAYER_RELATION_SCARED = 2,			// This CIV is a bit scared (maybe Jackie has his gun out)
		PLAYER_RELATION_PANICKED = 4,		// This is a CIV that will do anything to get away
		PLAYER_RELATION_COULDATTACK = 8,	// Bot has enough bravery and weaponry to dare attack Jackie if need be
		PLAYER_RELATION_IN_FOV = 16,		// Currently in Jackie's FOV
		PLAYER_RELATION_WAYBEYOND_FOV = 32,	// More than 90 degress off FOV
		PLAYER_RELATION_BEYOND_RANGE = 64,	// Beyond normal dialogue range (2m)
		PLAYER_RELATION_WAYBEYOND_RANGE = 128,	// Way beyond normal dialogue range (3m+)
		PLAYER_RELATION_LONGTIME_RESPONSE = 256, // Player took his time
		PLAYER_RELATION_WAYLONGTIME_RESPONSE = 512, // Player took a very long time
		PLAYER_RELATION_GUNOUT = 1024,		// Player has his gun out
		PLAYER_RELATION_GUNAIM = 2048,		// Player is aiming his gun at the bot
	};

	// Returns the relation the bot has with the player and various flags are set in _pFlags if supplied
	int GetPlayerRelation(int* _pioFlags);

	int GetCurrentPriorityClass();

////////////////////////////////////////////////////////

public:
	//Debugging stuff...
	bool DebugRender()
	{
		if ((m_pAIResources)&&(m_pAIResources->ms_bDebugRender))
		{
			return(true);
		}
		else
		{
			return(false);
		}
	};

	bool DebugTargetSet()
	{
		if ((m_pAIResources)&&(m_pAIResources->ms_bDebugRender)&&(m_pAIResources->ms_iDebugRenderTarget))
		{
			return(true);
		}
		else
		{
			return(false);
		}
	}

	bool DebugTarget()
	{
		if ((m_pAIResources)&&(m_pAIResources->ms_bDebugRender)&&(m_pAIResources->ms_iDebugRenderTarget == GetObjectID()))
		{
			return(true);
		}
		else
		{
			return(false);
		}
	}

	CWObject* SetDebugTarget(int32 _iDbgTarget)
	{
		if ((m_pServer)&&(m_pAIResources))
		{
			CWObject* pObj = m_pServer->Object_Get(_iDbgTarget);
			if (pObj)
			{
				m_pAIResources->ms_iDebugRenderTarget = _iDbgTarget;
				m_pServer->Registry_GetServer()->SetValue("dbgctrlobj",pObj->GetName());
				m_pServer->Registry_GetServer()->SetValue("agdbgobj",CStrF("%d",pObj->m_iObject));
				return(pObj);
			}
			else
			{
				m_pAIResources->ms_iDebugRenderTarget = 0;
				m_pServer->Registry_GetServer()->SetValue("dbgctrlobj",CStr("0"));
				m_pServer->Registry_GetServer()->SetValue("agdbgobj",CStr("0"));
			}
		}
		return(NULL);
	}

	//Debug renderwire if flag is set
	void Debug_RenderWire(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int32 _Color = 0xffffffff, fp32 _Duration = 1.0f);
	void Debug_RenderVector(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int32 _Color = 0xffffffff, fp32 _Duration = 1.0f);
	void Debug_RenderVertex(const CVec3Dfp32& _p0, int32 _Color = 0xffffffff, fp32 _Duration = 1.0f);

	//Sets debug message
	void SetDebugMsg(int _Msg);

#ifndef M_RTM
	//Do some every-frame debug stuff
	void OnDebugStuff();
	void OnDebugStuff2();

	//Get debug name of object
	virtual CStr GetDebugName(CWObject * _pObj);
	virtual CStr GetDebugName(int _iObj);

	//Get some debug info about the bot
	virtual CStr GetDebugString();

	static int DebugMsg;
	static CMTime AITime;
	static CMTime MaxTime;
	static bool ms_bBenchMark;
	static bool ms_bRemoveFunctionality;
	static bool ms_bRenderNavGrid;
	static bool ms_bMovementVerbose;

	bool m_bAIOut;
	bool m_bRenderFOV;

	CMTime Time;
	CMTime cmFrameTime;

	static CMTime ms_Pathfinds;
	static CMTime ms_DirectMoves;
	static CMTime ms_LengthFailedPathFinds;
	static CMTime ms_PositionFailedPathFinds;
	static CMTime ms_DelayedPathfinds;
	static CMTime ms_SearchTimeouts;
	static CMTime ms_MiscFailedPathFinds;
	bool m_bJustStarted;
	
	fp32 m_fPFTime;
	static CMTime ms_PFTime;
	static int ms_nPFs;
	static fp32 ms_fMaxPFTime;
	static fp32 ms_fMinPFTime;

	static bool ms_bPFReset; 
	static int ms_nPFFrames; 
	static fp32 ms_fMaxPFTimePerFrame;
	static fp32 ms_fMinPFTimePerFrame;

	bool Clear;
	CVec3Dfp32 BotCoord;

	CStr m_DebugInfo;

	static TPtr<CWorld_Navgraph_Pathfinder> ms_spGraphPF;

	static CVec3Dfp32 ms_PathDest;
	static bool ms_bNewPath;

	static fp32 ms_fFakeFrameTime;

	//Frame time in ms
	fp32 GetFrameTime();

	class CBenchMarker
	{
	public:
		CMTime m_Time;
		CStr m_Text;
		CMTime m_OverHead;
		CBenchMarker(CMTime _Time = CMTime(), CStr _Text = "")
		{
			m_Time = _Time;
			m_Text = _Text;
			m_OverHead.Reset();
		};
	};

	class CBMStack
	{
	private:
		TThinArray<CBenchMarker> m_lBMStack;
		int m_iTop;
	public:
		//Total overhead
		CMTime m_TotalOverhead;

		void Push(CMTime _Time, CStr _Text)
		{
			if (m_iTop >= m_lBMStack.Len())
				m_lBMStack.SetLen(m_iTop + 8);
			m_lBMStack[m_iTop] = CBenchMarker(_Time, _Text);
			m_iTop++;
		};
		int Pop(CMTime& _Time, CStr& _Text, CMTime& _OverHead)
		{
			if (m_iTop == 0)
				return 0;
			else
			{
				m_iTop--;
				_Time = m_lBMStack[m_iTop].m_Time; 
				_Text = m_lBMStack[m_iTop].m_Text; 
				_OverHead = m_lBMStack[m_iTop].m_OverHead; 
				return m_iTop + 1;
			}
		};
		CBMStack()
		{
			m_iTop = 0;
		}
		void AddOverHead(CMTime _OverHead)
		{
			if (m_iTop > 0)
				m_lBMStack[m_iTop - 1].m_OverHead += _OverHead;
			else
				m_TotalOverhead += _OverHead;
		}
	};

	CBMStack m_BMStack;
	CStr m_BenchmarkInfo;

	void StartBM(CStr _Text);
	void StopBM(CStr _Text = "");

	//Bot-control flags 
	enum {
		BOT_MOVE_FORWARD			= 0x1,
		BOT_MOVE_BACKWARD			= 0x2,
		BOT_MOVE_LEFT				= 0x4,
		BOT_MOVE_RIGHT				= 0x8,
		BOT_MOVE_UP					= 0x10,
		BOT_MOVE_DOWN				= 0x20,
		BOT_JUMP					= 0x40,
		BOT_CROUCH					= 0x80,
		BOT_LOOK_LEFT				= 0x100,
		BOT_LOOK_RIGHT				= 0x200,
		BOT_LOOK_UP					= 0x400,
		BOT_LOOK_DOWN				= 0x800,
		BOT_PRIMARY_ATTACK			= 0x1000,
		BOT_SECONDARY_ATTACK		= 0x2000,
		BOT_RUN						= 0x4000,
		BOT_WALK					= 0x8000,
		BOT_AI_CONTROL				= 0x10000,
		BOT_SWITCH_WEAPON			= 0x20000,
		BOT_FOLLOW_PATH				= 0x40000,
		BOT_TEST_STUFF				= 0x80000,
	};	

	//ConOuts stuff if the ms_bAIOut variable is true
	void AIOut(CStr Str);
	static void DebugRenderCell(int Val, int ValBelow, int ValAbove, CVec3Dfp32 Pos, CWorld_Server * pServer, fp32 Time);

	//Testing routine, remove later...
	//Adds commands according to the given bot control flags.
	void OnBotControl(CControlFrame& _Ctrl, int32 _iBotControl, CWObject * _pController);
#endif
};



#endif
