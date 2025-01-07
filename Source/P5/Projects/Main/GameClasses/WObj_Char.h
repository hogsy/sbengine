#ifndef _INC_WOBJ_CHAR
#define _INC_WOBJ_CHAR

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Character world object.
					
	Author:			Magnus Högdahl, Jens Andersson
					
	Copyright:		1997, 2001 Starbreeze Studios AB
					
	Contents:		

		WObj_CharAnim.cpp:	
					OnRefreshAnim
					OnGetAnimState
					Char_EvolveClientEffects

		WObj_CharClientData.cpp:	
					CChar_BlendUnit
					CWO_Character_ClientData

		WObj_CharCreate.cpp:	
					OnCreate
					OnSpawnWorld
					OnSpawnWorld2
					OnEvalKey
					OnFinishEvalKey
					OnInitInstance
					OnIncludeClass
					OnIncludeTemplate
					InitClientObjects

		WObj_CharIO.cpp:
					OnLoad
					OnSave
					OnDeltaLoad
					OnDeltaSave
					OnCreateClientUpdate
					OnClientUpdate
					OnClientLoad
					OnClientSave

		WObj_CharMechanics.cpp:	
					Character game play mechanics, such as RPG-object code, etc.

		WObj_CharMisc.cpp:	
					Miscellaneous helper functions.

		WObj_CharMsg.cpp:	
					OnMessage
					OnClientMessage
					OnClientNetMsg

		WObj_CharPhys.cpp:	
					Char_SetPhysics
					Phys_GetUserVelocity
					Phys_Move
					PhysUtil_GetMediumAccelleration
					OnPhysicsEvent
					OnIntersectLine
					OnClientPreIntersection

		WObj_CharPredict.cpp:	
					Char_ClientProcessControl
					OnPredictPress
					ClientPredictFrame

		WObj_CharRender.cpp:	
					OnClientRender
					OnClientRenderTargetHud
					OnClientRenderStatusBar
					Char_GetCamera
					GetChaseCam
					
		WObj_CharSpectator.cpp:	
					CWObject_Spectator 
					CWObject_SpectatorIntermission


	Comments:		Yeah, right...   

					IMPORTANT STUFF:

					* CWObject_Character code should NEVER use client or server timers. This
						is because characters may run asynchronously with respect to the 
						server simulation. The character's client-data have a timer that 
						should be used instead.


					* ...

					
	History:		
		97????:		Created character class

		010531:		Major clean up. Split up the class into multiple files for the first time.

\*____________________________________________________________________________________________*/


#include "WObj_Player.h"
#include "WObj_CharClientData.h"

#include "WObj_Game/WObj_GameMessages.h"
#include "WRPG/WRPGDef.h"

#include "../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Damage.h"
#include "../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_SimpleMessage.h"
#include "WObj_Misc/WObj_ImpulseContainer.h"
#include "../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_AnimUtils.h"

//All character message declarations have been moved to the WObj_CharMsg.h file
#include "WObj_CharMsg.h"
//#include "CConstraintSystem.h"

// -------------------------------------------------------------------
#define XR_WALLMARK_NOFADE		1 // Added by Mondelore.
#define XR_WALLMARK_FADEALPHA	0 // Added by Mondelore.
#define XR_WALLMARK_FADECOLOR	2

#define LERP(a,b,t) ((a) + ((b) - (a))*t)
#define FRAC(a) ((a) - M_Floor(a))

// WObj_CharPhys.cpp
bool IntersectModel(int _iModel, const CMat4Dfp32& _WMat, const CXR_AnimState& _AnimState, CWorld_PhysState* _pWPhysState, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MediumFlags, CCollisionInfo* _pCollisionInfo);

bool IsAlive(const int _PhysType); // Added by Mondelore.
bool InNoClipMode(const int _PhysType); // Added by Mondelore.
bool InCutscene(const CWObject_CoreData* _pObj); // Added by Mondelore.
bool AssistAim(const CWO_Character_ClientData *_pCD); // Added by Mondelore.
bool AssistCamera(const CWO_Character_ClientData *_pCD); // Added by Mondelore.
int	GetCameraMode(const CWO_Character_ClientData *pCD, const int _PhysType); // Added by Mondelore.
//void Moderate(int& x, int newx, int& xprim, int a);
//void Moderatef(fp32& _x, fp32 _newx, fp32& _xprim, int a);
void ModerateQuat(CQuatfp32& q, const CQuatfp32& newq, CQuatfp32& qprim, int a);
void ModerateVec3f(CVec3Dfp32& q, const CVec3Dfp32& newq, CVec3Dfp32& qprim, int a);
void ModerateVec2f(CVec2Dfp32& q, const CVec2Dfp32& newq, CVec2Dfp32& qprim, int a);
void InterpolateMatrix(const CMat4Dfp32 &_M0, const CMat4Dfp32 &_M1, fp32 _T, CMat4Dfp32 &_Res);
bool GetAttachPos(CWObject_CoreData* _pObj, CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel, int _iRotTrack, int _iAttachPoint, CVec3Dfp32 &_CameraPos, CVec3Dfp32 _Offset=0); // Added by Talbot
bool IsWidescreen(); // Added by Talbot
fp32 GetScreenAspect(); // Added by Talbot
bool IsForcedLook(const CWO_Character_ClientData *_pCD); // Added by Talbot
fp32 GetAnimPhysAldreadyAdjusted(int32 _AnimPhysMoveType); // Added by Phobos
void AdjustTurnCorrection(CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD, int32 _MoveType, CWorld_PhysState* _pWPhys);
// -------------------------------------------------------------------

enum
{
	PLAYER_CLIENTFLAGS_LASERBEAM =			(M_Bit(0)  << PLAYER_CLIENTFLAGS_USERSHIFT),
	PLAYER_CLIENTFLAGS_FLASHLIGHT =			(M_Bit(1)  << PLAYER_CLIENTFLAGS_USERSHIFT),
	PLAYER_CLIENTFLAGS_FORCECROUCH =		(M_Bit(2)  << PLAYER_CLIENTFLAGS_USERSHIFT),
	PLAYER_CLIENTFLAGS_SPECTATOR =			(M_Bit(3)  << PLAYER_CLIENTFLAGS_USERSHIFT),
	PLAYER_CLIENTFLAGS_NOMOVE =				(M_Bit(4)  << PLAYER_CLIENTFLAGS_USERSHIFT),
	PLAYER_CLIENTFLAGS_NOLOOK =				(M_Bit(5)  << PLAYER_CLIENTFLAGS_USERSHIFT),
	PLAYER_CLIENTFLAGS_DIALOGUE =			(M_Bit(6)  << PLAYER_CLIENTFLAGS_USERSHIFT),
	PLAYER_CLIENTFLAGS_WEIGHTLESS =			(M_Bit(7)  << PLAYER_CLIENTFLAGS_USERSHIFT),
	PLAYER_CLIENTFLAGS_BRIEFING =			(M_Bit(8)  << PLAYER_CLIENTFLAGS_USERSHIFT),
	PLAYER_CLIENTFLAGS_CUTSCENE =			(M_Bit(9)  << PLAYER_CLIENTFLAGS_USERSHIFT),
	PLAYER_CLIENTFLAGS_PLAYERSPEAK =		(M_Bit(10) << PLAYER_CLIENTFLAGS_USERSHIFT),
	PLAYER_CLIENTFLAGS_GHOST =				(M_Bit(11) << PLAYER_CLIENTFLAGS_USERSHIFT),
	PLAYER_CLIENTFLAGS_NOGRAVITY = 			(M_Bit(12) << PLAYER_CLIENTFLAGS_USERSHIFT),
	PLAYER_CLIENTFLAGS_NOCROUCH = 			(M_Bit(13) << PLAYER_CLIENTFLAGS_USERSHIFT),
	PLAYER_CLIENTFLAGS_WAIT = 				(M_Bit(14) << PLAYER_CLIENTFLAGS_USERSHIFT),
	// Out of bits (NOTE: We only have 15 bits now)

	PLAYER_EXTRAFLAGS_CAMERAMODE_MASK =		0x00000007, // May change during play.
	PLAYER_EXTRAFLAGS_CAMERAMODE_SHIFT =	0,
	PLAYER_EXTRAFLAGS_ASSISTCAMERA = 		M_Bit(3),  // Per game/player.
	PLAYER_EXTRAFLAGS_ASSISTAIM	=			M_Bit(4),  // Per game/player.
	PLAYER_EXTRAFLAGS_MOUNTEDCAM_NOLOOKUPDATE =		M_Bit(5),
	PLAYER_EXTRAFLAGS_NOSHADOW =			M_Bit(6),
	PLAYER_EXTRAFLAGS_LOCKAIM =				M_Bit(7),  // Per weapon.
	PLAYER_EXTRAFLAGS_STEALTH =				M_Bit(8),  
	PLAYER_EXTRAFLAGS_GODMODE =				M_Bit(9),
	PLAYER_EXTRAFLAGS_NOCOLLISIONSUPPORTED =M_Bit(10),
	PLAYER_EXTRAFLAGS_AIRCRAFT =			M_Bit(11), // Pitch/Turn character in movement direction. Bank when turning.
	PLAYER_EXTRAFLAGS_AIRCRAFT_USELOOK =	M_Bit(12), // Use the look to calculate body angle for airdraft controlled critters.
	PLAYER_EXTRAFLAGS_ANIMREVERSE =			M_Bit(13), 
	PLAYER_EXTRAFLAGS_NONPUSHABLE =			M_Bit(14),
	PLAYER_EXTRAFLAGS_FLIPPED =				M_Bit(15),
	PLAYER_EXTRAFLAGS_STUNNED =				M_Bit(16),
	PLAYER_EXTRAFLAGS_ALWAYS_NOCROUCH =		M_Bit(17),
	PLAYER_EXTRAFLAGS_NOBLINK =				M_Bit(18),
	PLAYER_EXTRAFLAGS_DISABLEHUMANIK =		M_Bit(19),
	PLAYER_EXTRAFLAGS_HOLDANIMATION =		M_Bit(20),
	PLAYER_EXTRAFLAGS_NOATTACH =			M_Bit(21), // OBJMSG_CHAR_ISVALIDATTACH will always return false when this is set
	PLAYER_EXTRAFLAGS_FORCETHIRPERSON =		M_Bit(22), // Debug-flag
	PLAYER_EXTRAFLAGS_THIRDPERSON =			M_Bit(23), // In-game toggle
	PLAYER_EXTRAFLAGS_HIDDEN =				M_Bit(24),
	PLAYER_EXTRAFLAGS_OVERRIDEFOG =			M_Bit(25),
	PLAYER_EXTRAFLAGS_CSNOBORDER =			M_Bit(26),
	PLAYER_EXTRAFLAGS_ROLLBONE =			M_Bit(27),
	PLAYER_EXTRAFLAGS_NORENDERHEAD =		M_Bit(28),
	PLAYER_EXTRAFLAGS_RAGDOLLACTIVE =		M_Bit(29),	// Is character currently in ragdoll-mode?
	PLAYER_EXTRAFLAGS_OTHERWORLD =			M_Bit(30),	// Player is in Otherworld.
	PLAYER_EXTRAFLAGS_FORCEATTACHMATRIX	=	M_Bit(31),	// Used by renderattached

	PLAYER_PHYSFLAGS_NOCHARACTERCOLL =		M_Bit(0),
	PLAYER_PHYSFLAGS_NOWORLDCOLL =			M_Bit(1),
	PLAYER_PHYSFLAGS_NOPROJECTILECOLL =		M_Bit(2),
	PLAYER_PHYSFLAGS_NOGRAVITY =			M_Bit(3),
	PLAYER_PHYSFLAGS_IMMUNE =				M_Bit(4),
	PLAYER_PHYSFLAGS_NOMEDIUMMOVEMENT =		M_Bit(5),
	PLAYER_PHYSFLAGS_IMMOBILE =				M_Bit(6),
	PLAYER_PHYSFLAGS_FORCESETPHYS =			M_Bit(7),
	PLAYER_PHYSFLAGS_NOANIMPHYS	=			M_Bit(8),

	PLAYER_FLAGS_STUNNABLE =				M_Bit(0),
	PLAYER_FLAGS_WAITSPAWN =				M_Bit(1),
	PLAYER_FLAGS_SPAWNDEAD =				M_Bit(2),
	PLAYER_FLAGS_AUTODESTROY =				M_Bit(3),
	PLAYER_FLAGS_TIMELEAP =					M_Bit(4),
	PLAYER_FLAGS_NONPUSHABLE =				M_Bit(5),
	PLAYER_FLAGS_NOCROUCH =					M_Bit(6),
	PLAYER_FLAGS_NOBLINK =					M_Bit(7),
	PLAYER_FLAGS_ALWAYSVISIBLE =			M_Bit(8),
	PLAYER_FLAGS_NOTARGETING =				M_Bit(9),
	PLAYER_FLAGS_FORCETENSION =				M_Bit(10),
	PLAYER_FLAGS_DIALOGUE_NOSPECIAL =		M_Bit(11),
	PLAYER_FLAGS_DIALOGUE_WAIT =			M_Bit(12),
	PLAYER_FLAGS_________ =					M_Bit(13),
	PLAYER_FLAGS_AIR =						M_Bit(14),
	PLAYER_FLAGS_NOGUNKATA =				M_Bit(15),
	PLAYER_FLAGS_SPAWNCROUCHED =			M_Bit(16),
	PLAYER_FLAGS_NODIALOGUETURN =			M_Bit(17),
	PLAYER_FLAGS_MUZZLEVISIBILITY =			M_Bit(18),
	PLAYER_FLAGS_USEDNAWEAPONS =			M_Bit(19),
	PLAYER_FLAGS_NONIGHTVISION =			M_Bit(20),
	PLAYER_FLAGS_RAGDOLL =					M_Bit(21),
	PLAYER_FLAGS_NODEATHSOUND =				M_Bit(22),
	PLAYER_FLAGS_NOAIMASSISTANCE =			M_Bit(23),
	PLAYER_FLAGS_NOAUTOAIMATARGET =			M_Bit(24),
	PLAYER_FLAGS_AUTOUNEQUIPWEAPONS =		M_Bit(25),
	PLAYER_FLAGS_DUALWIELDSUPPORTED =		M_Bit(26),
	PLAYER_FLAGS_PROJECTILEINVULNERABLE =	M_Bit(27),
	PLAYER_FLAGS_ATTACHFORCETRIGGERS	 =	M_Bit(28),
	PLAYER_FLAGS_DISABLEIK =				M_Bit(29),
	PLAYER_FLAGS_VISBOXEXPANDED =			M_Bit(30),


	// Spawn behavior
	PLAYER_SPAWNBEHAVIOR_NOSPAWNMESSAGES =	M_Bit(0),
	PLAYER_SPAWNBEHAVIOR_FROMIO =			M_Bit(1),
};

enum
{
	PLAYER_CUTSCENE_NORMAL = 0,
	PLAYER_CUTSCENE_NOSKIP,
	PLAYER_CUTSCENE_NOBORDER,
};
// -------------------------------------------------------------------
enum
{
	PLAYER_DIALOGUE_LAND = MHASH1('35'),
	PLAYER_DIALOGUE_SLIP = MHASH1('36'),
	PLAYER_DIALOGUE_PAIN0 = MHASH1('25'),
	PLAYER_DIALOGUE_PAIN1 = MHASH1('26'),
	PLAYER_DIALOGUE_DEATH = MHASH1('27'),
	PLAYER_DIALOGUE_DEATH_FALL = MHASH1('28'),
	PLAYER_DIALOGUE_DEATH_GIB = MHASH1('29'),
	PLAYER_DIALOGUE_DEATH_FIRE = MHASH1('30'),
	PLAYER_DIALOGUE_HEARTBEAT01 = MHASH1('8'),
	PLAYER_DIALOGUE_HEARTBEAT02 = MHASH1('9'),
	PLAYER_DIALOGUE_DARKNESS_POWER01 = MHASH2('555','800'),	// This is an evil one.
	PLAYER_DIALOGUE_DARKNESS_POWER02 = MHASH2('555','801'),	// This human is evil.
	PLAYER_DIALOGUE_DARKNESS_POWER03 = MHASH2('555','802'),	// It reeks of evil.
	PLAYER_DIALOGUE_DARKNESS_POWER04 = MHASH2('555','803'),	// Human is bad.
	PLAYER_DIALOGUE_DARKNESS_POWER05 = MHASH2('555','804'),	// We like this one.
	PLAYER_DIALOGUE_DARKNESS_POWER06 = MHASH2('555','805'),	// Look, an evil one.
	PLAYER_DIALOGUE_DARKNESS_POWER07 = MHASH2('555','806'),	// See, a vicious human.
	PLAYER_DIALOGUE_DARKNESS_POWER08 = MHASH2('555','807'),	// That one is strong and vicious.
	PLAYER_DIALOGUE_DARKNESS_POWER09 = MHASH2('555','808'),	// That one we like.
	PLAYER_DIALOGUE_DARKNESS_POWER10 = MHASH2('555','809'),	// Good, an evil-doer.

	PLAYER_CONTROLMODE_FREE = 0,
	PLAYER_CONTROLMODE_LADDER = 1,
	PLAYER_CONTROLMODE_FORCEMOVE = 2,	// like PLAYER_CONTROLMODE_FREE with fixed movement.
	PLAYER_CONTROLMODE_FIGHTING = 3,
	PLAYER_CONTROLMODE_HANGRAIL = 4,
	PLAYER_CONTROLMODE_CARRYBODY = 5,
	PLAYER_CONTROLMODE_LEAN = 6,
	PLAYER_CONTROLMODE_LEDGE = 7,
	//PLAYER_CONTROLMODE_ANIMCONTROLLED = 8,
	PLAYER_CONTROLMODE_ANIMATION = 9,
	PLAYER_CONTROLMODE_ACTIONCUTSCENE = 10,
	PLAYER_CONTROLMODE_EVENT = 11,
	PLAYER_CONTROLMODE_LEDGE2 = 12,
	PLAYER_CONTROLMODE_ANIMSYNC = 13,
	
	PLAYER_MAXARMORMODELS = 4,
	PLAYER_MAXACTIVATEITEMGROUPSOUNDS = 8,
	PLAYER_MAXDEATHSOUNDS = 4,

	PLAYER_PICMIP_CHARACTER = 2,
	PLAYER_PICMIP_PLAYER = 7,

	PLAYER_DATAINDEX_CONTROL = RPG_DATAINDEX_LAST,
	PLAYER_DATAINDEX_LAST,

	PLAYER_DISABLE_TRIGGER = 1,
	PLAYER_DISABLE_ACTIVATE = 2,
	PLAYER_DISABLE_ATTACK = 4,
	PLAYER_DISABLE_MOVE = 8,
	PLAYER_DISABLE_LOOK = 16,
	PLAYER_DISABLE_GRAB = 32,
	PLAYER_DISABLE_DARKNESSPOWERS = 64,
	PLAYER_DISABLE_SWITCHWEAPON = 128,	//For shapeshifter mode

	PLAYER_DARKNESSMODE_POWER_CREEPINGDARK =	M_Bit(0),		// Creeping dark
	PLAYER_DARKNESSMODE_POWER_DEMONARM =		M_Bit(1),		// Demon arm
	PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS =	M_Bit(2),		// Using the ancient darkness guns
	PLAYER_DARKNESSMODE_POWER_BLACKHOLE =		M_Bit(3),		// Black hole
	PLAYER_DARKNESSMODE_POWER_DARKNESSVISION =	M_Bit(4),		// Darkness vision
	PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD =	M_Bit(5),		// Automatic shield when in darkness
	
	PLAYER_DARKNESSMODE_SELECTION =				M_Bit(6),
	PLAYER_DARKNESSMODE_ACTIVE =				M_Bit(7),
	PLAYER_DARKNESSMODE_ACTIVATED =				M_Bit(8),

	PLAYER_DARKNESSMODE_CREEPINGCAM =			M_Bit(9),

	PLAYER_DARKNESSMODE_POWER_DEVOUR =			M_Bit(10),		// Is devouring corpse
	PLAYER_DARKNESSMODE_POWER_______ =			M_Bit(11),		// Free \o/

	PLAYER_DARKNESSMODE_POWER_DRAIN	=			M_Bit(12),		// Replenishing health/darkness

	PLAYER_DARKNESSMODE_STICKMOVED =			M_Bit(13),
	PLAYER_DARKNESSMODE_DARKLINGSPAWNED	= 		M_Bit(14),
	PLAYER_DARKNESSMODE_STICKSELECTED =			M_Bit(15),

	PLAYER_DARKNESSPOWERUP_MAXDARKNESS =		1,
	PLAYER_DARKNESSPOWERUP_COREPOWER =			2,
	PLAYER_DARKNESSPOWERUP_DARKLING =			3,

	PLAYER_DARKNESSMODE_NOREGENMASK = (PLAYER_DARKNESSMODE_POWER_CREEPINGDARK | PLAYER_DARKNESSMODE_POWER_BLACKHOLE | PLAYER_DARKNESSMODE_POWER_DEMONARM /*| PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS*/ | PLAYER_DARKNESSMODE_POWER_DEVOUR),

	PLAYER_DARKNESSMODE_NOUSEMASK = (PLAYER_DARKNESSMODE_POWER_CREEPINGDARK | PLAYER_DARKNESSMODE_POWER_DEMONARM),


	PLAYER_DARKNESSMODE_COREPOWERMASK = (PLAYER_DARKNESSMODE_POWER_CREEPINGDARK | PLAYER_DARKNESSMODE_POWER_BLACKHOLE | PLAYER_DARKNESSMODE_POWER_DEMONARM | PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS),
	PLAYER_DARKNESSMODE_POWERMASK = (PLAYER_DARKNESSMODE_COREPOWERMASK | PLAYER_DARKNESSMODE_POWER_DARKNESSVISION | PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD),
	PLAYER_DARKNESSMODE_AIDARKNESSMASK = (PLAYER_DARKNESSMODE_POWER_CREEPINGDARK | PLAYER_DARKNESSMODE_POWER_BLACKHOLE | PLAYER_DARKNESSMODE_POWER_DEMONARM | PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS | PLAYER_DARKNESSMODE_DARKLINGSPAWNED | PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD),
	PLAYER_DARKNESSMODE_ACTIVATEPOWERMASK = (PLAYER_DARKNESSMODE_POWER_CREEPINGDARK | PLAYER_DARKNESSMODE_POWER_DEMONARM | PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS | PLAYER_DARKNESSMODE_POWER_BLACKHOLE | PLAYER_DARKNESSMODE_POWER_DEVOUR),
	PLAYER_DARKNESSMODE_POWER_STATE_MASK =		(PLAYER_DARKNESSMODE_POWER_DEVOUR | PLAYER_DARKNESSMODE_POWER_DRAIN),

	PLAYER_DARKNESSMODE_SELECTION_NUMPOWERS = 4,

	PLAYER_DARKNESS_DARKLING_ARRAYMAP_UP =		0,
	PLAYER_DARKNESS_DARKLING_ARRAYMAP_DOWN =	1,
	PLAYER_DARKNESS_DARKLING_ARRAYMAP_LEFT =	2,
	PLAYER_DARKNESS_DARKLING_ARRAYMAP_RIGHT =	3,

	//PLAYER_DARKNESSMODE_DARKNESSVISIONSHIELDLIMIT	= 32,
	//PLAYER_DARKNESSMODE_DARKNESSDRAINPERMASSUNIT	= 10,
	// Amount of max darkness (8 -> maxdarkness/8)
	//PLAYER_DARKNESSMODE_DRAINCOST_CREEPINGDARK			= 8,
	//PLAYER_DARKNESSMODE_DRAINCOST_POWERANCIENTWEAPONS	= 4,
	//PLAYER_DARKNESSMODE_DRAINCOST_BLACKHOLE				= 2,
	//PLAYER_DARKNESSMODE_DRAINCOST_DEMONARM				= 16,

	PLAYER_DARKNESS_DRAINCOST_ACTIVATE_CREEPINGDARK =	0,
	PLAYER_DARKNESS_DRAINCOST_ACTIVATE_DEMONARM =		10,
	PLAYER_DARKNESS_DRAINCOST_ACTIVATE_ANCIENTWEAPON =	0,
	PLAYER_DARKNESS_DRAINCOST_ACTIVATE_BLACKHOLE =		0,

	PLAYER_MOUNTEDMODE_FLAG_ORIGINAL		= M_Bit(0),
	PLAYER_MOUNTEDMODE_FLAG_TURRET			= M_Bit(1),
	PLAYER_MOUNTEDMODE_FLAG_INVISIBLE		= M_Bit(2),
	PLAYER_MOUNTEDMODE_FLAG_BLENDANIM		= M_Bit(3),
	PLAYER_MOUNTEDMODE_FLAG_BLENDOUTANIM	= M_Bit(4),
	PLAYER_MOUNTEDMODE_FLAG_RENDERHEAD		= M_Bit(5),

	PLAYER_SAVEPARAM_SAVETOBIN				= 1,
	PLAYER_SAVEPARAM_LOADFROMBIN			= 2,
	PLAYER_INVENTORYSAVEFLAG_HASSAVEDINVENTORY		= M_Bit(0),
	PLAYER_INVENTORYSAVEFLAG_HASTRANSCENDINGITEMS	= M_Bit(1),
	PLAYER_INVENTORYSAVEFLAG_NONORMALINVENTORY		= M_Bit(2),


	FINDSTUFF_SELECTIONMODE_FOCUSFRAME	= M_Bit(0),
	FINDSTUFF_SELECTIONMODE_DEVOURING	= M_Bit(1),

	PLAYER_HITEFFECT_DIR_FRONT			= 0,
	PLAYER_HITEFFECT_DIR_LEFT			= 1,
	PLAYER_HITEFFECT_DIR_BACK			= 2,
	PLAYER_HITEFFECT_DIR_RIGHT			= 3,
	PLAYER_HITEFFECT_DIR_ABOVE			= 4,
	PLAYER_HITEFFECT_DIR_BELOW			= 5,

	PLAYER_HITEFFECT_FLAGS_DAMAGE				= M_Bit(0),
	PLAYER_HITEFFECT_FLAGS_SOUND				= M_Bit(1),
	PLAYER_HITEFFECT_FLAGS_DOTS					= M_Bit(2),
	PLAYER_HITEFFECT_FLAGS_BLUR					= M_Bit(3),
	PLAYER_HITEFFECT_FLAGS_STREAK				= M_Bit(4),

	PLAYER_HITEFFECT_RENDERMASK					= PLAYER_HITEFFECT_FLAGS_STREAK | PLAYER_HITEFFECT_FLAGS_BLUR | PLAYER_HITEFFECT_FLAGS_DOTS,
};

// -------------------------------------------------------------------

enum
{
	PLAYER_ANIM_ONLYTORSO =	 		0x00000001,
	PLAYER_ANIM_HOLDATEND =		 	0x00000002,
};

// -------------------------------------------------------------------

#define PLAYER_CONTROL_WALKMIN			0.03f
#define PLAYER_CONTROL_WALKMAX			0.8f
#define PLAYER_CONTROL_WALK				0.5f
#define PLAYER_CONTROL_WALKLINEARTICKS	20

#define PLAYER_FOOTSTEP_STARTINDEX		31
#define PLAYER_FOOTSTEP_ENDINDEX		42

// -------------------------------------------------------------------
enum
{
	PLAYER_FIGHTDIR_NONE		= 0,
	// Start with 8 to distance ourselves from CONTROLBITS
	PLAYER_FIGHTLOOKDIR_UP		= M_Bit(8),
	PLAYER_FIGHTLOOKDIR_DOWN	= M_Bit(9),
	PLAYER_FIGHTLOOKDIR_LEFT	= M_Bit(10),
	PLAYER_FIGHTLOOKDIR_RIGHT	= M_Bit(11),
	PLAYER_FIGHTMOVEDIR_UP		= M_Bit(12),
	PLAYER_FIGHTMOVEDIR_DOWN	= M_Bit(13),
	PLAYER_FIGHTMOVEDIR_LEFT	= M_Bit(14),
	PLAYER_FIGHTMOVEDIR_RIGHT	= M_Bit(15),

/*	PLAYER_NETMSG_SPAWNBLOOD = 0x20,
	PLAYER_NETMSG_TILT,*/
	PLAYER_NETMSG_RUMBLE = 0x20,
	PLAYER_NETMSG_SETTOKENHOLDER,
	PLAYER_NETMSG_CLEARTOKEN,
	PLAYER_NETMSG_FLASHSCREEN,
	PLAYER_NETMSG_KILLVOICE,
	PLAYER_NETMSG_DECAL,
	PLAYER_NETMSG_HEALTHWOBBLE,
	PLAYER_NETMSG_SETDIALOGUECHOICES,
	PLAYER_NETMSG_SETCROUCHINPUT,
	PLAYER_NETMSG_DISORIENTATION_FLASH,
	PLAYER_NETMSG_DISORIENTATION_BLACKHOLE,
	PLAYER_NETMSG_PLAYSOUND_REPEAT,
	PLAYER_NETMSG_MP_GETTEMPLATES,
	PLAYER_NETMSG_RESET_POSTANIMSYSTEM,
	
	//Crosshair info
	PLAYER_NOT_AIMING = 0x00,
	PLAYER_CROSSHAIR_VISIBLE = 0x01,
	PLAYER_CROSSHAIR_INSIDE_TARGETRING = 0x04,

	// Controller types
	CONTROLLER_TILTASSISTED		= 0x00,
	CONTROLLER_TANK				= 0x01,
	CONTROLLER_FREELOOK			= 0x02,
};
// -------------------------------------------------------------------

//#define LOG_PREDICTION

// #define LOG_AUTHORING
// #define LOG_CLIENTAUTHORING

// #define LOG_SERVERAUTHORING_CMD
// #define LOG_CLIENTAUTHORING_CMD

// -------------------------------------------------------------------

#define PLAYER_CLIENTOBJ_CLIENTDATA		0
#define PLAYER_CLIENTOBJ_SKELINSTANCE	1
#define PLAYER_CLIENTOBJ_AGISTATE			2

#define PLAYER_SELFSPLASH_SCALE 0.4f

#define PLAYER_FLAGS_ISGIBBED 2
#define PLAYER_FLAGS_IMPACTDMGDONE 8

enum
{
	PLAYER_PHYS_VOID		= 0,
	PLAYER_PHYS_STAND		= 1,
	PLAYER_PHYS_CROUCH		= 2,
	PLAYER_PHYS_DEAD		= 3,
	PLAYER_PHYS_NOCLIP		= 4,
	PLAYER_PHYS_SPECTATOR	= 5,
	PLAYER_PHYS_NOCLIP2		= 6,
};

#define PLAYER_PHYS_IDLETICKS	4

#define PLAYER_PHYS_NOCLIP2_SIZE 4

#define PLAYER_CAMERAMAXTILT 0.2f

#define PLAYER_ANIMBLENDSPEED 0.35f
#define PLAYER_ANIMBLENDSPEEDVISUAL 0.15f

#define PLAYER_DIALOGUE_DISTANCE 40.0f

#define PLAYER_FLEETIME 6.0f

#define PLAYER_FALLDAMAGEHEIGHT 20.0f
#define PLAYER_FALLDAMAGESCALE 7.0f
#define PLAYER_FALLDAMAGEMAX 1000.0f

#define PLAYER_FALLTILTHEIGHT 12.0f
#define PLAYER_FALLTILTSCALE 0.3f

#define PLAYER_FALLVELOCITYSOUND0 10.0f
#define PLAYER_FALLVELOCITYSOUND1 19.0f

// Velocity needed to kill a character from above
#define PLAYER_MIN_DROPKILL_VELOCITY 17.0f

#define PLAYER_MIN_KNEEL_VELOCITY 7.0f
#define PLAYER_MAX_KNEEL_VELOCITY 22.0f
#define PLAYER_MIN_KNEEL_HEIGHT 6
#define PLAYER_MAX_KNEEL_HEIGHT 18
#define PLAYER_KNEELDAMPING	320
#define PLAYER_STEPUPDAMPING 320
#define PLAYER_CAMERACHANGE 320
#define PLAYER_CAMERADISTANCECHANGE 160

#define PLAYER_CAMERASHAKE_BLENDINDURATION 0.2f		// How long it takes to fade in shake momentum.
#define PLAYER_CAMERASHAKE_BLENDOUTDURATION 0.7f	// How long it takes to fade out shake momentum.
#define PLAYER_CAMERASHAKE_FREQSCALE 0.1f;
#define PLAYER_CAMERASHAKE_SCALE 0.01f;				// Overall amplitude scaling (do avoid decimals).
#define PLAYER_CAMERASHAKE_TURNSCALE 0.4f			// How much of momentum that is used for turn.
#define PLAYER_CAMERASHAKE_PITCHSCALE 0.6f			// How much of momentum that is used for pitch.
#define PLAYER_CAMERASHAKE_BANKSCALE 0.2f			// How much of momentum that is used for banking.
#define PLAYER_CAMERASHAKE_MOVESCALE 80.0f			// How much of momentum that is used for movement.

#define PLAYER_FIGHT_MINDISTANCE 20.0f

enum CAMERASHAKEFLAG
{
	CAMERASHAKEFLAG_ACTIVE		= 0x01,
	CAMERASHAKEFLAG_RESTART		= 0x02,
};

#define PLAYER_PAINSOUNDDELAY 1.5f

#define PLAYER_AIMASSISTCHANGEX 640
#define PLAYER_AIMASSISTCHANGEY 320
#define PLAYER_TARGETRINGCHANGE 160

#define PLAYER_TARGETRING_LOCKTIME 2.0f
#define PLAYER_TARGETRING_BOOST_THRESHOLD 2.0f
#define PLAYER_TARGETRING_BOOST_FACTOR 1.2f

#define PLAYER_SPAWNANIM_HIDETIME 0.25f

#define PLAYER_TARGETHUD_FADESPEED 160

//Selection timeout duration in seconds
#define PLAYER_WEAPONSELECT_DURATION 3.0f 

#define PLAYER_GRABBED_TIMESPEED 260
#define PLAYER_GRABBED_STATE_GRABBED 1
#define PLAYER_GRABBED_STATE_NOTGRABBED 0

struct CTargetHudData
{
	const char* m_pName;
	uint8		m_Health;
};

//#define PLAYER_UPPER_AUTOTILT_LIMIT -0.06f  //0.08f
//#define PLAYER_LOWER_AUTOTILT_LIMIT -0.04f	//0.04f
#define PLAYER_AUTOTILT_ANGLE		-0.05f

enum
{ 
	PLAYER_CAMERAMODE_FIRSTPERSON	= 0x00,
	PLAYER_CAMERAMODE_NORMAL		= 0x01,
	PLAYER_CAMERAMODE_FAR			= 0x02,
	PLAYER_CAMERAMODE_NEAR 			= 0x03,
};

enum
{
	PLAYER_HITLOCATION_TORSO = 0,
	PLAYER_HITLOCATION_ARMS,
	PLAYER_HITLOCATION_LEGS,
	PLAYER_HITLOCATION_HEAD,
	PLAYER_HITLOCATION_OTHER,
	PLAYER_NUM_HITLOCATIONS,
	
	PLAYER_HITLOCATION_BASE = 10,

	PLAYER_PROTECTION_SKIN = 0,
	PLAYER_PROTECTION_CLOTH,
	PLAYER_PROTECTION_LEATHER,
	PLAYER_PROTECTION_CHAIN,
	PLAYER_PROTECTION_PLATE,
	PLAYER_PROTECTION_OTHER,
	PLAYER_NUM_PROTECTIONS,

	PLAYER_SIMPLEMESSAGE_ONSPAWN = 0,
	PLAYER_SIMPLEMESSAGE_ONDIE,
	PLAYER_SIMPLEMESSAGE_ONTAKEDAMAGE,
	PLAYER_SIMPLEMESSAGE_ONPICKUPDROPWEAPON,
	PLAYER_SIMPLEMESSAGE_ONPICKUPDROPITEM,

	PLAYER_DIALOGUE_AIFLAGS_KEEPBEHAVIOURSPEAKER = 1,
	PLAYER_DIALOGUE_AIFLAGS_KEEPBEHAVIOURLISTENER = 2,
	PLAYER_DIALOGUE_AIFLAGS_NOLOOKSPEAKER = 4,
	PLAYER_DIALOGUE_AIFLAGS_NOLOOKLISTENER = 8,
	PLAYER_DIALOGUE_AIFLAGS_NOCROWD = 16,
	PLAYER_DIALOGUE_AIFLAGS_STOPBEHAVIOURCROWD = 32,
};

// Autotargetting stuff
#define TARGET_FARANGLE 0.998f
#define TARGET_CLOSEANGLE 0.977f
#define TARGET_FARANGLE_LOCKTRESH 1.0f
#define TARGET_CLOSEANGLE_LOCKTRESH 0.995f
#define TARGET_MAXDIST 768.0f

#define TARGET_FARANGLE_LAMP 0.998f		
#define TARGET_CLOSEANGLE_LAMP 0.977f
#define TARGET_FARANGLE_LAMP_LOCKTRESH 1.0f		
#define TARGET_CLOSEANGLE_LAMP_LOCKTRESH 0.995f
#define TARGET_MAXDIST_LAMP 384.0f


#define PLAYER_CAMERAMODE_FAR_BEHINDSCALE 2.0f

#define PLAYER_MAX_STAND_HEAD_HEIGHT 64
#define PLAYER_MAX_CROUCH_HEAD_HEIGHT 50

#define PLAYER_SPECIALROTATION_RANGE 0.25f

#define PLAYER_STEPSIZE 12.5
#define PLAYER_FALLDAMAGE_MINVEL 26.3f

#define CORPSE_DURATION 5.0f

#define CHAR_MAXTARGETINGDISTANCE_DEFAULT 20000.0f
#define OBJECT_MAXTARGETINGDISTANCE_DEFAULT 20000.0f

// #define PLAYER_JUMP_PREINTERSECT

#define PLAYER_CONTROLBITS_NOAUTORELEASE (CONTROLBITS_JUMP)

// The character's turning states
enum
{
	PLAYER_ANIM_NOTTURNING		= 0,
	PLAYER_ANIM_TURNINGRIGHT	= 1,
	PLAYER_ANIM_TURNINGLEFT		= 2,
};

// Sync with FlagsTranslate in CWObject_Trigger_Sneak::OnEvalKey, "AFFILIATIONS"
enum
{
	AFFILIATIONFLAG_GUARDS = 1,
	AFFILIATIONFLAG_GANG1,
	AFFILIATIONFLAG_GANG2,
	AFFILIATIONFLAG_GANG3,
	AFFILIATIONFLAG_OTHER,
};

// ClientDate m_3PI_Mode
enum 
{
	THIRDPERSONINTERACTIVE_MODE_NONE		= 0,
	THIRDPERSONINTERACTIVE_MODE_ACS			= 1,
	THIRDPERSONINTERACTIVE_MODE_DIALOGUE	= 2,
	THIRDPERSONINTERACTIVE_MODE_MASK		= 3,

	THIRDPERSONINTERACTIVE_LIGHT_STATE_ON = 0,
	THIRDPERSONINTERACTIVE_LIGHT_STATE_FADE_IN,
	THIRDPERSONINTERACTIVE_LIGHT_STATE_OFF,
	THIRDPERSONINTERACTIVE_LIGHT_STATE_FADE_OUT,

	THIRDPERSONINTERACTIVE_STATE_IDLE		= 0 << 2,
	THIRDPERSONINTERACTIVE_STATE_ENTERING	= 1 << 2,
	THIRDPERSONINTERACTIVE_STATE_LEAVING	= 2 << 2,
	THIRDPERSONINTERACTIVE_STATE_MASK		= 3 << 2,
};

enum 
{
	WEAPON_ZOOMSTATE_ON =0,
	WEAPON_ZOOMSTATE_IN,
	WEAPON_ZOOMSTATE_OFF,
	WEAPON_ZOOMSTATE_OUT,
};


// Skeleton types
enum
{
	SKELETONTYPE_NONE = 0,			// used for helicopter
	SKELETONTYPE_NORMAL = 1,
	SKELETONTYPE_BUTCHER = 2,		// (hack)
	SKELETONTYPE_3 = 3,				// unused
	SKELETONTYPE_DARKLING = 4,		
};


// Darkness flags
enum
{
	DARKNESS_FLAGS_HEART		= M_Bit(0),		// Has heart left
	DARKNESS_FLAGS_VOICE		= M_Bit(1),		// Voice tagged
};

// Forward declarations
class CWObject_Trigger_Sneak;
class CWObj_ObjLightMeter;



class CGibInfo
{
public:
	CVec3Dint16 m_Pos;
	uint16  m_iModel;
	CVec3Duint8 m_Angles;
	uint8 m_iNode;
};

/*struct SMessageCounter
{
	int m_iMsg;
	int m_ServerCount;
	int m_ClientCount;
	SMessageCounter()
	{
		m_ServerCount = 0;
		m_ClientCount = 0;
	}
};*/


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CGrabbedObject
|
| TODO: move this class somewhere else
|  (merge with tentacle system CObjectGrabber)
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CGrabbedObject
{
public:
	struct PIDInfo
	{
		CVec3Dfp32 m_IntegrateTerm;
		CVec3Dfp32 m_DeriveTerm;
		CVec3Dfp32 m_LastPos;
		CVec3Dfp32 m_LocalGrabPos;
	};
	PIDInfo m_Pid[2];
	uint16  m_iTargetObj;

	void Init(CWorld_PhysState& _WPhysState, uint16 _iObject, const CVec3Dfp32& _Look);
	bool Update(CWorld_Server& _WServer, const CVec3Dfp32& _WantedPos, const CVec3Dfp32& _Look);
	void Release();
	bool IsActive() const { return m_iTargetObj > 0; }
};


class CCompletedMissions
{
public:
	TArray<uint32> m_lCompletedMissions;
	void Clear();
	//void DebugPrint();
	void AddMission(CStr _Str);
	bool MissionExists(CStr _Str);
	void Write(CCFile* _pFile) const;
	void Read(CCFile* _pFile);
	int GetInsertPosition(uint32 _Hash);
};


class CCharDialogueItems
{
public:
	CDialogueLink m_Approach;
	CDialogueLink m_ApproachScared;
	CDialogueLink m_Threaten;
	CDialogueLink m_Ignore;
	CDialogueLink m_Timeout;
	CDialogueLink m_Exit;

	bool Parse(uint32 _KeyHash, const CStr& _KeyValue);
	void Override(const CCharDialogueItems& _Other);
	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile) const;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Character
|__________________________________________________________________________________________________
\*************************************************************************************************/
//
// TODO: Create the following class structure:
//
// CWObject_CharBase
//  |
//  +- CWObject_CharPlayer
//  +- CWObject_CharNPC
//
// CWO_ClientData_CharBase
//  |
//  +- CWO_ClientData_CharPlayer
//  +- CWO_ClientData_CharNPC
//
// The initial step would be to move the existing CWObject_Character -> CWObject_CharBase
// and then gradually move/add Player-specific code+data into CWObject_CharPlayer and NPC-specific
// code+data into CWObject_CharNPC
// 
class CWObject_Character : public CWObject_Player
{
	MRTC_DECLARE_SERIAL_WOBJECT;

protected:
	void OnCharAnimEvent(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD, class CXR_Anim_DataKey* _pKey);
	aint OnCharForceDropWeapon(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD);
	aint OnCharCreateGrabbableItemObject(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD);
	aint OnCharPrecacheMessage(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD);
	aint OnCharTeleport(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD);
	aint OnCharActivateItem(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD);
	aint OnCharSetExtraItem(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD);
	aint OnCharPlayAnim(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD);
	aint OnCharShakeCamera(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD);
	aint OnCharAnimImpulse(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD, CWAG2I* _pWAG2I, CAG2TokenID _iToken = 0);
	aint OnCharSetCameraEffects(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD);
	aint OnCharSetMountedLook(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD);
	aint OnCharSetMountedCamera(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD);
	aint OnCharUpdateItemDesc(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD);

public:
	// Generic struct for storing damagefactors
	// How this is supposed to work:
	// If there are no entries in m_lArmorSurfaces the damage factor is 1.0
	// Add armor by writing *Armor_factor <location>,<types>,<factor>[,<armor>]
	struct SHitloc
	{
		SHitloc()
		{
			m_Factor		= -1.0f;
			m_DamageTypes	= DAMAGETYPE_UNDEFINED;
			m_Hitloc		= HITLOC_ALL;
			m_SurfaceType	= -1;
			m_Armour		= 0;
		};

		enum
		{
			HITLOC_HIGH		= 1,
			HITLOC_MID		= 2,
			HITLOC_LOW		= 4,
			HITLOC_ALL		= (HITLOC_HIGH | HITLOC_MID | HITLOC_LOW),
		};
		fp32 m_Factor;			// Multiplier for the damage
		uint32 m_DamageTypes;	// Bitvector for the kind(s) of damage this entry affects
		uint16 m_Hitloc;			// What hitlocation?
		int16 m_SurfaceType;	// Corresponds to hitlocation/surfacetype
		int m_Armour;			// Damage reduction
	};

	
// *** Will be removed after I finish with CConstraintSystem ***
	static void DebugRenderSkeletonJoints(CWorld_Client* _pWClient,
												const CMat4Dfp32 &_Mat,
												CXR_SkeletonInstance* _pSkelInstance,
												CXR_Skeleton* _pSkel);

	static void CalcMatrices_r(int _iNode,
										const CMat4Dfp32 &_Mat,
										CXR_SkeletonInstance* _pSkelInstance,
										CXR_Skeleton* _pSkel,
										TArray<CMat4Dfp32> &_lMat);

	static CWO_Character_ClientData* GetClientData(CWObject_CoreData* _pObj);
	static const CWO_Character_ClientData* GetClientData(const CWObject_CoreData* _pObj);
	static CXR_SkeletonInstance* GetClientSkelInstance(CWO_Character_ClientData* _pCD, int _iSkelInst);
	static CWAG2I_Mirror* GetAG2IMirror(CWObject_CoreData* _pObj);
	static const CWAG2I_Mirror* GetAG2IMirror(const CWObject_CoreData* _pObj);

	static int InitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);

	class CRPG_Object_Char *Char();
	
	static CVec3Dfp32 GetLook(const CVec3Dfp32& _VFwd);
	static CVec3Dfp32 GetLook(const CMat4Dfp32& _Mat);

	bool Char_BeginTimeLeap(bool _bDestroySounds = true);
	bool Char_EndTimeLeap();

	static int Char_DarknessIsTintSound(CWO_Character_ClientData* _pCD);
	static int Char_DarknessIsTintView(CWO_Character_ClientData* _pCD);
	static bool Char_UpdatePannlampa(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj);
	virtual void Char_RefreshDarkness();
	CVec3Dfp32 Char_GetDarknessPosition();
	int Char_DarknessShieldHit(int _Damage);
	int Char_RefreshDarknessPowers(CWO_Character_ClientData* _pCD, fp32 _Visibility);
	virtual void Char_CheckDarknessActivation(int _ControlPress, int _ControlLastPress, const CVec3Dfp32& _ControlMove);
	bool Char_ActivateDarknessPower(int32 _Power, bool _bDarkEnough, bool _bActivate);
	void Char_TurnOffDarknessPowers();
	void Char_ActivateBlackHole();
	bool Char_DoGunKata(int32 _Type, int32 _iTarget);
	static bool Char_CheckGunKata(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, int32 _Type, int32 _iTarget);
	bool Char_BeRagDoll(bool _bEnable);
	void Char_SpawnDarkling(int _ControlPress, int _ControlLastPress);
	int Char_ExtractDarkling(const CMat4Dfp32& _ExtractMat, const CStr& _DarklingType);
	void CheckDarklingSpawnPoints(const CVec3Dfp32& _SelectPos, fp32 _Radius, int16& _iBest);
	
	bool Char_DevourTarget(int _iTarget);
	bool Char_DevourTarget_PreFinish();
	bool Char_DevourTarget_Finish(int _iTarget);

	void Char_AddDarknessPowerups(const int32& _iFromCorpseObj, CWO_Character_ClientData* _pToCD);
	void Char_AddDarknessPowerups_ShowInfoScreen(int _bShow, const char* _pStr);
	bool Char_IsDarklingAvailable(const CStr& _Type);
	bool Char_AddDarkling(const CStr& _Type, bool _bSendInfoMsg = false);
	CStr Char_GetSpawnDarkling(const CStr& _Type);
	void Char_DarknessFlashBar(int32 _Color);
	void Char_HealthWobble(fp32 Amount);
	bool Char_DropItem(int _iSlot);
	void Char_RemoveDropWeapon(int32 _CurrentIdentifier);
	bool Char_RemoveItem(const char *_pName);
	bool Char_SelectItem(CRPG_Object_Item* _pItem);
	bool Char_SelectItemByName(const char *_pName);
	bool Char_SelectItemByType(int _ItemType);
	bool Char_SelectItemByAnimType(int _ItemType, int _iSlot);
	bool Char_SelectItemByIndex(int _iSlot, int _iIndex);
	void Char_RemoveEmptyItems(int _iSlot = 0);
	bool Char_PickupItem(int _bSelect, int _bInstant, const char *_pName, int _iSender, bool _bNoPickupIcon = false, bool _bDontAutoEquip = false);
	void Char_EquipNewItem(CRPG_Object_Item* _pCurItem, bool _bIsPressLeft, bool _bOnlyWeaponsWithAmmo = false);
	bool Char_AvailablePickup(const CWObject_Message &_Msg);
	void Char_UpdateQuestItems();
	bool Char_UpdateItemDescription(const char *_pName, const char* _pNewName, int16 _iNewIcon);
	int Char_Mount(const char *_pTarget, int _Flags);
	bool Char_SetCutsceneCamera(int _CameraObject);
	void Char_SpawnCutscene(int _CameraObject, bool _bInstant = false, int _Type = 0);
	bool Char_BeginCutscene(int _CameraObject, bool _bInstant = false, int _Type = 0);
	bool Char_EndCutscene();
	bool Char_SkipCutscene();
	void Char_DeathDropItem();

	bool Char_ShowMissionJournal();
	bool Char_ShowMissionMap();
	bool Char_EndShowMissionJournal();
	bool Char_EndShowMissionMap();

	void Char_SpawnDialogue(int _iSpeakerObj, bool _bInstant);
	bool Char_BeginDialogue(int _iSpeakerObj, int _iStartItem);
	bool Char_EndDialogue();
	bool Char_SkipDialogue(void);	//returns true if dialogue is gonna be skipped
	CDialogueLink Char_ParseDialogueChoice(CFStr &_St, int _iSender, int _iOwner);
	bool Char_SetDialogueChoices(const char *_pSt, int _iSender, int _iOwner);
	void Char_SelectDialogueChoice(int _iChoice);
	void Char_SetListener(int _iListener, int _Flags = 0);
	void Char_DestroyDialogue();
	void Char_SendDialogueImpulse(int32 _iImpulse);
	static bool Char_SetDialogueTokenHolder(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, int _iHolder, int _iSender);
	static bool Char_SetDialogueChoices_Client(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, uint16 *_liItems, int _nItems);
	void Char_ActivateDialogueItem(CDialogueLink _iDialogueItem, int _iUser);
	CDialogueLink Char_GetDialogueApproachItem();
	virtual void Char_DebugDialogueInfo(void);

	void Char_SetRpg(TPtr<CRPG_Object> _spRPG) { m_spRPGObject = _spRPG; }
	TPtr<CRPG_Object> Char_GetRpg() { return m_spRPGObject; }
	virtual bool Char_IsClimbing();
	virtual bool Char_IsSwimming();
	static bool Char_IsFighting(CWObject* _pObj);
	static bool Char_IsFighting(CWO_Character_ClientData* _pCD);
	static CVec3Dfp32 GetTargetFightPos(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, fp32 _Offset);
	/*static bool MoveToken(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, 
								   CWO_Character_ClientData* _pCD, const int8& _Token, 
								   const CStr& _StateAction, CMTime _Time = CMTime(), fp32 _ForceOffset = 0.0f, int32 _MaxQueued = 0);*/

	static void CharGetFightCharacter(CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD, CWorld_PhysState* _pWPhys, int32& _iBest, fp32 _SelectRadius);

	// Ragdoll
	bool Char_ActivateRagdoll(CWO_Character_ClientData *_pCD,bool _bCopyState = true);
	void Char_RagdollToCD(CWO_Character_ClientData *_pCD) const;
	void Char_RagdollDeltaWrite(CCFile * _pFile);
	void Char_RagdollDeltaRead(CCFile * _pFile,CWO_Character_ClientData *_pCD);

	// Targeting
	static CVec3Dfp32 Target_GetObjectVector(CWObject_CoreData* _pObjSelf, int _iObj, CWorld_PhysState* _pWPhysState);
	static bool Target_CheckLOS(const CVec3Dfp32& _From, const CVec3Dfp32& _To, CWorld_PhysState* _pWPhysState, CVec3Dfp32* _HitPos = NULL);	
	static fp32 Target_SelectionFitness(fp32 _CosV, fp32 _DistSqr);
	virtual void OnRefresh_Target();
	static void OnRefresh_Predicted_VerifyTarget(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	static void OnRefresh_Predicted_Target(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	
	int GetDialogueLength(int _iDialogue);
	int GetDialogueLength_Hash(uint32 _iDialogue);
	static void OnRefresh_Dialogue(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, int _iNewDialogue = 0, int _Flags = 0, CWRes_Dialogue::CRefreshRes *_pRes = NULL);
	static void OnRefresh_Dialogue_Hash(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, uint32 _iNewDialogue, int _Flags, CWRes_Dialogue::CRefreshRes *_pRes);

	void EvalDialogueLink(const CWRes_Dialogue::CRefreshRes &_Res);
	static void EvalDialogueLink_Client(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, const CWRes_Dialogue::CRefreshRes &_Res);
	void RefreshInteractiveDialogue();

	static void OnRefresh_SoundVolumes(CWObject_Client* _pObj, CWorld_Client* _pWClient);

	void OnRefreshSkeletonMats();
	void OnRefresh_Mechanics();
	void OnRefresh_Stealth();
	static bool OnRefresh_WeaponLights(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, const CMat4Dfp32 &_CharPos, fp32 _IPTime, CMat4Dfp32 *_pMat);
	static bool OnRefresh_WeaponLights_Model(CWO_Character_ClientData* pCD, const uint8& _iAttachModel, const int& _iLight, CWorld_PhysState* _pWPhysState,
											 CXR_SceneGraphInstance* _pSGI, CXR_SkeletonInstance* _pSkelInstance, CXR_Skeleton* _pSkel, const CMTime& _GameTime,
											 CXR_AnimState& _AnimState, CMat4Dfp32* _pMat, const fp32& _IPTime, const bool& _bMuzzleFlash, const bool& _bFlashlight, const int& _bLightningLight);

	virtual CRPG_Object *CreateWorldUseRPGObject(const char *_RpgClass);
	virtual int OnUseInformation(const CWObject_Message &_Msg);

	virtual void OnCreate();
	virtual void OnDestroy();
	virtual void OnShutOffAllLights();
	virtual void OnInitInstance(const aint* _pParam, int _nParam);
	static void OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);
	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server*);
	static void OnClientPrecacheClass(CWorld_Client* _pWClient, CXR_Engine* _pEngine);
	static void OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	
	virtual void SetTemplateName(CStr _TemplateName);

	// -------------------------------------------------------------------
	void Char_SetGameTickDiff(int _Diff);
	static int Char_GetGameTickDiff(CWObject_CoreData *_pObj);
	static void Char_SetControlMode(CWObject_CoreData *_pObj, int _Mode);
	static int Char_GetControlMode(const CWObject_CoreData *_pObj);
	static void Char_SetPhysType(CWObject_CoreData *_pObj, int _PhysType);
	static int Char_GetPhysType(const CWObject_CoreData *_pObj);
	bool IsImmune();

	static bool Char_FindStuff(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD, int32& _iSel, int32& _iCloseSel, int8& _SelType, bool& _bCurrentNoPrio, uint8 _SelectionMode = 0, fp32 _SelectRadius = -1.0f, int32 _SelectType = 0, CVec3Dfp32* _pSelectPos= NULL);
	static bool Char_FindBestOpponent(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, int32& _iSel, fp32 _SelectRadius = -1.0f);
	static bool Char_ActivateStuff(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD, const int32& _iSel, const int8& _SelType);
	static bool Char_GrabLedge(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD);
	bool Char_ShowInFocusFrame(int8 _SelType, int _iObj);
	void Char_UpdateThirdPersonInteractive(CWO_Character_ClientData& _CD);
	void Char_MakePlayerLookAtSpeaker(CWObject_Character* _pPlayer, CWObject_Character* _pSpeaker);
	void Char_TurnDialogueLightOn(CWO_Character_ClientData& _CD, bool _bOn);

	static void Char_SetAnimCode(CWObject_CoreData *_pObj, int _iCode);
	void Char_SetAnimTorsoSequence(int _iSeq, uint32 _StartTick = 0);
	static void Char_SetAnimTorsoSequence(CWObject_CoreData *_pObj, CWorld_PhysState* _pWPhysState, int _iSeq, uint32 _StartTick = 0);
	static void Char_SetAnimSequence(CWObject_CoreData *_pObj, CWorld_PhysState* _pWPhysState, int _iSeq, uint32 _StartTick = 0);
	static void Char_SetAnimStance(CWObject_CoreData *_pObj, int _iSeq);
	static int Char_GetAnimCode(const CWObject_CoreData *_pObj);
	static int Char_GetAnimStance(const CWObject_CoreData *_pObj);
	static int Char_GetAnimTorsoSequence(const CWObject_CoreData *_pObj);

	static bool GetEvaluatedSkeleton(CWObject_CoreData *_pObj, CWorld_PhysState* _pWPhysState, CXR_SkeletonInstance *&_pInstance, CXR_Skeleton *&_pSkeleton, CXR_AnimState& _Anim);
	bool GetEvaluatedSkeleton(CXR_SkeletonInstance *&_pInstance, CXR_Skeleton *&_pSkeleton, CXR_AnimState& _Anim);
	static bool GetEvaluatedPhysSkeleton(CWObject_CoreData *_pObj, CWorld_PhysState* _pWPhysState, CXR_SkeletonInstance *&_pInstance, CXR_Skeleton *&_pSkeleton, CXR_AnimState& _Anim, fp32 _IPTime = 0.0f, const CMat4Dfp32* _pCharPos = NULL);
	bool GetEvaluatedPhysSkeleton(CXR_SkeletonInstance *&_pInstance, CXR_Skeleton *&_pSkeleton, CXR_AnimState& _Anim, fp32 _IPTime = 0.0f, const CMat4Dfp32* _pCharPos = NULL);

	static void Char_SetAnim(CWObject_CoreData*, int _iUnit, int _iSeq, fp32 _Time = -1);
	static int Char_GetAnim(CWObject_CoreData*, int _iUnit);
	static int Char_GetAnimLayers(CWObject_CoreData* _pObj, const CMat4Dfp32 &_Pos, CWorld_PhysState* _pWPhysState, fp32 _IPTime, CXR_AnimLayer* _pLayers, fp32 _Extrapolate = 0);

	static void Client_Anim(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, CWO_Character_ClientData* _pData, int _DebugCalledFrom = 0);

	static bool Char_IsPlayerViewControlled(CWObject_CoreData* _pObj);
	static bool Char_IsPlayerView(CWObject_CoreData* _pObj);
	static bool Char_CanLook(CWObject_CoreData* _pObj);

	bool CheckContinueDialogue();
	virtual bool PlayDialogue_Hash(uint32 _DialogueItemHash, uint _Flags = 0, int _Material = 0, int _StartOffset = 0, uint32 _SyncGroupID = 0, int8 _AttnType = WCLIENT_ATTENUATION_3D);
	static bool Char_PlayDialogue_Hash(CWObject_CoreData* _pObj, CWorld_Client* _pWClient, uint32 _DialogueItemHash, int _AttnType = 0, bool _bNeedGroundMaterial = false);

	static void Char_SetFootstep(CWObject_CoreData* _pObj, CWorld_Client* _pWClient, int _iFoot);

	static void Char_EvolveClientEffects(CWObject_Client* _pObj, CWorld_Client* _pWClient);

	static void OnRefresh_ServerPredicted_Extras(CWO_Character_ClientData *_pCD, CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	static void OnRefresh_ServerPredicted(CWO_Character_ClientData *_pCD, CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	virtual void OnRefresh_ServerOnly(CWO_Character_ClientData *_pCD);

	static void OnClientRefresh_DemoPlayback(CWO_Character_ClientData *_pCD, CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	static void OnClientRefresh_Mirror(CWO_Character_ClientData *_pCD, CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static void OnClientRefresh_TrueClient(CWO_Character_ClientData *_pCD, CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);

	static void OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine*, const CMat4Dfp32& _ParentMat);

	static void RenderSurfaceOverlay(CWorld_Client* _pWClient, CXR_Engine* _pEngine, CRC_Util2D* _pUtil2D, CClipRect& _Clip, uint16 _SurfaceID, int32 _StartTick, uint16 _Duration, int8 _Type, uint32 _WH);

	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static void OnClientRenderTargetHud(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D); // Added by Mondelore.
	static void OnClientRenderTargetHud(int _X, int _Y, CRC_Util2D *_pUtil2D, fp32 _Fade, fp32 _HealthFrac, const char *_pName, CRC_Font *_pFont);
	static void OnClientRenderStatusBar(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D);
	static void OnClientRenderStatusBar_Combobar(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D);
	static void OnClientRenderHealthHud(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D);
	static void OnClientRenderChoices(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D);
	static void OnClientRenderDarklingsChoices(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D);
	static void OnClientRenderHurtEffectAndOverlaySurfaces(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, CRC_Util2D* _pUtil2D);

	// Dialogue camera
	static bool IsValidDialogueCamera(CWorld_Client* _pWClient, CMat4Dfp32& _PrevCamera, CMat4Dfp32& _NewCamera, int _iSpeakerObj, int _iListenerObj, float _FOV);
	//static void GetDialogueCamera_Client(CWO_Character_ClientData* _pCD, CWorld_Client* _pWClient, int _iSpeakerObj, int _iListenerObj, CMat4Dfp32& _Camera, float _IPTime, bool _bSwappedSpeaker = false);
	static void GetDialogueCamera_Client(CWO_Character_ClientData* _pCD, CWorld_Client* _pWClient, const CDialogueInstance* _pDialogueInst, const int *_piObjectIndices, CMat4Dfp32& _Camera, float _IPTime, bool _bSwappedSpeaker = false);
	static fp32 GetDialogueCameraFOV_Client(CWO_Character_ClientData* _pCD, CWorld_Client* _pWClient, float _IPTime);
	static fp32	GetHeadOffset(CWorld_Client* _pWClient, int _iObj, const CMat4Dfp32& _Position, fp32 _IPTime);
	static void Char_ClientProcessControl(const CControlFrame& _Msg, int& _Pos, CWObject_Client* _pObj, CWorld_Client* _pWClient, bool _bLog);
	static void OnPredictPress(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint32& _Press, const uint32& _Released, CPhysUtilParams& _Params);

	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	static void OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg);

	static bool OnGetAnimState(CWObject_CoreData*, CWorld_PhysState*, CXR_Skeleton* _pSkel, int _iCacheSlot, const CMat4Dfp32 &_Pos, fp32 _IPTime, CXR_AnimState& _Anim, CVec3Dfp32 *_pRetPos = NULL, bool _bIgnoreCache = false, fp32 _ClothScaleOverride = -2.0f, CXR_Anim_TrackMask *_pTrackMask = NULL, fp32 _OverrideRagdollHeight = 0.0f);
	static bool OnGetAnimState2(CWObject_CoreData*, CWorld_PhysState*, CXR_Model* _lpModels[CWO_NUMMODELINDICES], const CMat4Dfp32& _Pos, fp32 _IPTime, CXR_AnimState& _Anim, CVec3Dfp32* _pRetPos = NULL, bool _bIgnoreCache = false, fp32 _ClothScaleOverride = -2.0f, fp32 _OverrideRagdollHeight = 0.0f);
	static bool OnIntersectLine(CWO_OnIntersectLineContext& _Context, CCollisionInfo* _pCollisionInfo = NULL);
	static int OnClientPreIntersection(CWObject_Client* _pObj, CWorld_Client* _pWClient, int _iObject, CCollisionInfo *_pInfo);
	
	static void RotateBoneAbsolute(const CQuatfp32 &_Q, CXR_SkeletonInstance *_pSkelInstance, int _iRotTrack, int _iParent);
	static void EvalSkelAndApplyBoneScale(fp32 _Scale, CXR_SkeletonInstance* _pSkelInstance, CXR_Skeleton* _pSkel);

	static int OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo);

	virtual int Char_CheatsEnabled();

	//virtual void Char_AddTilt(const CVec3Dfp32& _Axis, fp32 _Angle);

	int Char_AutoActivate();

	//virtual void OnTimeout();
	
	virtual void OnSpawnWorld();
	virtual void OnSpawnWorld2();
	void SendOnSpawnMessages();

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void EvalDamageFactor(const CRegistry* _pKey,int _iSlot);
	virtual void OnFinishEvalKeys();
	void UnspawnCharacter();
	virtual void SpawnCharacter(int _PhysMode = PLAYER_PHYS_STAND, int _SpawnBehavior = 0, bool _bFindPos = true);
	bool Char_TrySetPosition(int _PhysMode);
	virtual void OnPress();
	virtual int OnPumpControl();
	virtual void OnRefresh();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual int OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const;
	static int OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);

	static void OnClientLoad(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags);
	static void OnClientSave(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags);

	bool CanUse(int _iUser);
	int OnUse(int _iUser, int _Param);

	static CVec3Dfp32 GetCharacterCenter(CWObject_CoreData* _pChar, CWorld_PhysState* _pWPhysState = NULL, bool _bEvalSkeleton = false, fp32 _CenterBias = 0.0f);
	static void GetCameraAimMatrix(CWObject_CoreData* _pObj, CWorld_PhysState* _pPhysState, CMat4Dfp32* _pMat, bool _bApplyBehindOffset = false);
	bool GetActivatePosition(CMat4Dfp32 &_Mat, bool _bPreciseAutoAim = false, uint32 _DamageType = DAMAGETYPE_UNDEFINED);
	static CVec3Dfp32 Char_GetMouthPos(CWObject_CoreData *_pObj);
	void Char_CreateNeckBlood();
	
	static fp32 GetMaxTargetingDistance(CWObject_CoreData* _pObj);
	static fp32 GetTargetRingRadius(CWObject_CoreData* _pObj);

	int Physics_Kill(uint32 _DamageType, int _iSender);
	int Physics_Damage(const CWO_DamageMsg& _Msg, int _iSender);
	int Physics_Shockwave(const CWO_ShockwaveMsg& _Msg, int _iSender);
	int Physics_Preintersection(int _iObject, CCollisionInfo *_pInfo);
	void Physics_AddVelocityImpulse(const CVec3Dfp32& _Impulse);
	int Game_KilledPlayer(int _iPlayer, uint32 _DamageType);
	int Char_Stun(int _Ticks);
	bool Char_DoBloodAtAttach(int32 _iRotTrack, int32 _iAttach, const CVec3Dfp32& _Direction, const CVec3Dfp32& _Offset, int32 _Count = 1, const char* _pEffect = NULL);

	static CVec3Dfp32 Char_ControlMode_Anim2(const CSelection& _Selection, CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, 
		int16& _Flags);

	bool ForceSelectItem(int _iItemType);
	bool ForceActivateItem(int _iItemType);

	static bool Char_PredictActivateItem(CWorld_Client* _pWClient, CWObject_Client* _pObj, CAutoVar_WeaponStatus* _pWeapStat,CAutoVar_WeaponStatus* _pWeapStatFirst, CWO_Character_ClientData* _pCD, CWO_Character_ClientData* _pCDFirst, int _iToken);

	void NextItem(int _iSlot, bool _bReverse, bool _bInstant = false);
	bool SelectItem(int _iItemType, bool _bInstant = false);
	bool SelectItemByIdentifier(int _iSlot, int _Identifier, bool _bInstant = false);
	int OnUpdateItemModel(int _iSlot, int _iTick = -1);

	void ToggleWeaponZoom();

	static CVec3Dfp32 Char_GetPhysRelPos(CWObject_CoreData* _pObj, int _Type);
	static int Char_SetPhysics(CWObject_CoreData* _pObj, CWorld_PhysState* _pPhysState, CWorld_Server* _pWServer, int _Type, int _bTele = false, bool _bReset = false);
	bool Phys_ToggleNoClip(int _PlayerPhys);
	void Char_Die(uint32 _Method, int _iKilledBy, int _iDeathAnim = -1);
	static CVec3Dfp32 GetAdjustedMovement(const CWO_Character_ClientData* _pCD);
	static void Char_UpdateLook(CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD, CWorld_PhysState* _pWPhysState, fp32 _FrameFrac);
	virtual int Char_ProcessControl(const CControlFrame& _Msg, int& _Pos);

	static void Phys_SetForceMove(CWObject_CoreData* _pObj, fp32 Speed, fp32 TotalTime);
	virtual int Phys_GetMediumSamplingPoints(CVec3Dfp32* _pRetV, int _MaxV);
	static CVec3Dfp32 Phys_GetUserAccelleration(const CSelection& _Selection, CWObject_CoreData* _pObj, CWObject_Character* _pChar, CWorld_PhysState* _pPhysState, 
		const CVec3Dfp32& _Move, const CVec3Dfp32& _Look, uint32 _Press, uint32& _Released, 
		const CXR_MediumDesc& _MediumDesc, int16& _Flags);
	static bool Phys_ControlAffectsMovement(const CControlFrame& _Msg, int _Pos, const CWObject_CoreData* _pObj, const CWO_Character_ClientData* _pCD);
	static void Phys_Move(const CSelection& _Selection, CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD, CWorld_PhysState* _pPhysState, fp32 _dTime, const CVec3Dfp32& _UserVel, bool _bPredicted);

	static fp32 GetHeartBeat(CWObject_CoreData *_pObj, CWorld_Client* _pWClient, const CMTime& _Time, bool _bRefresh);

	virtual void OnLoad(CCFile* _pFile);
	virtual void OnSave(CCFile* _pFile);
	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);
	virtual void OnDeltaSave(CCFile* _pFile);
	virtual void OnFinishDeltaLoad();
	void SaveInventory(CCFile* _pFile);
	void SaveInventory_Player(CCFile* _pFile);
	void SaveInventory_PlayerHelper(CCFile* _pFile, bool _bNoTrans, bool _bOnlyTrans, bool _bPrecache);
	void SaveInventory_AI(CCFile* _pFile);
	void LoadInventory(CCFile* _pFile);
	void LoadInventory_Player(CCFile* _pFile);
	void LoadInventory_PlayerHelper(CCFile* _pFile, bool _bRemoveItems);
	void LoadInventory_AI(CCFile* _pFile);
	virtual CStr Dump(CMapData* _pWData, int _DumpFlags);

	int32 Team_Get(int32 _i);
	int32 Team_GetNum();
	int32 Team_Add(int32 _iID);
	bool Team_Remove(int32 _iID);

	static CWObject_Character * IsCharacter(CWObject_CoreData * _pObj);						//Checks if the given object is a character, and returns a character-pointer if so, else NULL
	static CWObject_Character * IsCharacter(int iObj, CWorld_PhysState * _pPhysstate);	//Checks if the given server index points to a charactre on the given server, and returns a character-pointer if so, else NULL

	bool IsBot();						//Checks if the given character is AI-controlled at the moment
	bool IsPlayer();					//Checks if the character has a corresponding player
	
	fp32 GetVisibility(bool _bLightsOut,int32 _PerceptionFlags = 0);	//Get visibility factor of character.
	fp32 GetNoise();	//Get noise factor of character.

	// Returns the hitlocation for the given _CInfo
	int GetHitlocation(const CCollisionInfo* _CInfo);

	// Returns the damage multiplier from a given _DamageType, _Hitlocation and _Surface
	// NOTE: _DamageType cannot be a combined value ie only one bit may be set
	fp32 GetHitlocDamageFactor(uint32 _DamageType, int _HitLoc = SHitloc::HITLOC_ALL,int _SurfaceType = -1, int _Damage = 0, int _iSender = -1);

	bool IsControllerIdle(const int _nTicks = -1);

	// Multiply damage factor for the given hit location and damage type(s) to the given value.
	// (As value originally is 1.0 this will practically be the same as setting the value)
	void SetHitlocDamageFactor(int _iSlot, fp32 _Factor, uint32 _DamageTypes, int _HitLoc = SHitloc::HITLOC_ALL, int _SurfaceType = -1, int _Armour = 0);	

	static bool Aim_IsValidTarget(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, int _iTarget, fp32 &_Angle);
	static int16 Aim_FindTarget(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, int16* _pExcludeObjectList, int _ExcludeListLength);
	void Aim_GetMatrix(CMat4Dfp32& _Matrix);
	static void Aim_Refresh(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);

	// Camera Methods (WObj_CharCamera.cpp)
	static void			Camera_OnRefresh(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);

	static void			Camera_Offset(CWObject_CoreData* _pObj, CWorld_PhysState* _pPhysState, CMat4Dfp32& _Camera, fp32 _IPTime);
	static bool			Camera_Trace(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, bool _bRangedWeapon, CMat4Dfp32& _CameraMatrix, 
												 CVec3Dfp32& _WantedBehindOffset, fp32& _WantedDistance, fp32& _MaxDistance, bool _Rotate=true);
	static CVec3Dfp32	Camera_GetHeadPos(const CWObject_CoreData* _pObj);
	static void			Camera_ShakeItBaby(CWorld_PhysState* _pWPhysState, CMat4Dfp32& _Camera, CWO_Character_ClientData* _pCD, fp32 _IPTime);
	static void			Camera_Get(CWorld_PhysState* _pWPhysState, CMat4Dfp32* _Camera, CWObject_CoreData* _pCamObj, fp32 _IPTime);
	static void			Camera_Get_FirstPerson(CWorld_PhysState* _pWPhysState, CMat4Dfp32* _Camera, CWObject_CoreData* _pCamObj, fp32 _IPTime);

	static int OnGetCamera(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	static void ZeroInLookForDialogueCameras(CWO_Character_ClientData* _pCD);
	void SetWeaponUnequipTimer(bool _bReset = false, fp32 _Seconds = 3.0f);
	void ResetWeaponUnequipTimer();

	// AI-interface
	virtual CAI_Core* AI_GetAI();
	virtual fp32  AI_GetMaxSpeed(int _Speed);

	virtual bool AI_IsSpawned();
	virtual bool AI_IsAlive();
	virtual bool AI_IsBot();
	virtual bool AI_IsWeightLess();

	virtual CRPG_Object_Item* AI_GetRPGItem(int _EquipPlace);
	virtual CRPG_Object_Inventory* AI_GetRPGInventory(int _iSlot);
	virtual int  AI_GetRPGAttribute(int _Attribute, int _Param1 = 0);

	//Return position [matrix] of camera last frame
	virtual CMat4Dfp32 AI_GetLastCamera();			
	virtual CVec3Dfp32 AI_GetLastCameraPos();

	virtual int AI_GetControlMode();
	virtual void AI_SetAnimControlMode(bool _bOn);

	virtual int AI_GetActivationState();
	virtual void AI_SetActivationState(int _State);

	// Returns a randomly chosen object id based on the supplied params
	virtual CWObject* AI_GetSingleTarget(int _iObject, CStr _pName, int _iSender);

	virtual uint16 AI_GetPhysFlags();

	virtual uint AI_GetTeams(uint16* _piResult, uint _MaxElem) const;

	virtual void AI_GetBaseMat(CMat4Dfp32& _RetValue);		// Get character position matrix, but without the look-angles merged into it..
	virtual void AI_GetWeaponMat(CMat4Dfp32& _RetValue);		// Position + direction of weapon muzzle 
	virtual void AI_GetHeadMat(CMat4Dfp32& _RetValue);		// Get position of eyes + direction of look
	virtual void AI_GetActivateMat(CMat4Dfp32& _RetValue);	// Get activate position of agent

	virtual CVec3Dfp32 AI_GetBasePos();
	virtual CVec3Dfp32 AI_GetWeaponPos();
	virtual CVec3Dfp32 AI_GetHeadPos();
	virtual CVec3Dfp32 AI_GetActivatePos();


	static int ResolvePlayerFlags(CStr _Str);
	static int ResolvePhysFlags(CStr _Str);
	static int ResolveDarknessFlags(CStr _Str);
	static int ResolveDarknessSelection(const int8& _Selection);
	static int ResolveDarknessSelectValue(int _DarknessPowerFlag);

	void UpdateVisibilityFlag(bool _bRefresh = false);
	void UpdateVisibility(int *_lpExtraModels = NULL, int _nExtraModels = 0);

	void UpdateDarknessLightMorph(CWO_Character_ClientData* _pCD);

	static void UpdateGroundMaterial(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);

#ifndef M_RTM
	void AI_GetControlFrame(CControlFrame& _Ctrl);
	void GiveOrder(int _Order);		//Gives an order to all controlled bots. Giving a previously given order will cancel that order.
#endif
	
	// Returns true if char is in darkness
	// NOTE Currently, only the player can be considered in darkness, other guys are always
	// returning false here
	bool InDarkness();
	bool HasHeart();
	void SetHeart(bool _bHasHeart);

	M_INLINE void SetGunplayUnsafe(int32 _iTick) { m_GunplayUnsafeTick = Max(m_GunplayUnsafeTick,_iTick); }

	// Description names for focusframe
	CStr m_UseName;
	CStr m_DescName;
	spCRegistry m_spRPGObjectReg;
	int m_Flags;
	int8 m_bHideWhenDead : 1;
	int8 m_bFirst : 1;
	int8 m_bAIControl : 1;			//When true a player controlled character will be controlled by the AI instead
	int8 m_bLastChangeRight : 1;	//Last change of gun left/right hand?
	bool m_bResetControlState : 1;	//When true control state is reset before other commands are issued in OnPumpControl

	CWO_SimpleMessageContainer m_MsgContainer;
	uint32 m_OnTakeDamageTypeMask;		// What damage types that trigger OnTakeDamage (default all on)

	CCharDialogueItems m_DialogueItems;				// these are set in OnEvalKey()    (and replaced by savegame data)
	CCharDialogueItems m_PostLoadDialogueItems;		// these are set in OnEvalKey() and applied to 'm_DialogueItems' after savegame data is loaded
	//TArray<uint16> m_liDialogueChoices;
	TArray<CDialogueLink> m_liDialogueChoices;
	TArray<CWO_SimpleMessage> m_lDialogueImpulses;
	// Carry inventory around between certain sections of the game that don't want the items
	TArray<uint8> m_lSavedInventory;


	TArray<CGibInfo> m_lGibInfos;
	CVec3Dfp32 m_GibExplosionOrigin;
	CVec3Dfp32 m_GibExplosionParams;		// [0] is Force, [1] is radius, [2] is randomness
	fp32 m_GibExplosionRadius;
	fp32 m_GibExplosionRandomness;
	CWO_ImpulseContainer m_ImpulseContainer;
	TPtr<class CRPG_Object> m_spDropItem;
	CStr m_DamageEffect;
	CStr m_DeathEffect;
	CMat4Dfp32 m_OrgMountPos;

	CMat4Dfp32 m_CachedActivatePosition_Mat;
	int m_CachedActivatePosition_Tick;
	uint32 m_CachedActivatePosition_iDamageType;
	bool m_CachedActivatePosition_bPrecise;
	// *** NOTE Very similar to the cached ones above but I don't dare use them yet ***
	CVec3Dfp32 m_ActivatePosition;	// Diff between activate pos and actual activate pos from anim
	int m_ActivatePositionTick;	// When m_ActivateOffset was measured, we accept max 1 tick behind

	//Used in AI_GetAimPosition, set in template. 
	int16 m_AIAimOffset;	 
	int16 m_AICrouchAimOffset;	 

	CCompletedMissions m_CompletedMissions;

	uint16 m_iListener;
	uint16 m_iSpeaker;
	uint32 m_InterruptedItemHash;

	//Team object indices and names (the latter is destroyed after the first is set in OnSpawnWorld)
	TArray<uint16> m_liTeams;
	TArray<CStr> m_lTeamFromKeys;

	int m_StatusInfoCountdown;
	int m_PlayerInfoCountdown;
	int m_GameInfoCountdown;

	int m_iLastUsed;
	int m_LastUsedTick;
	int m_LastNotFjuffed;

	int m_iPendingItemModelUpdate[4];
	int m_PendingTimeLeapTick;

	int m_PendingCutsceneTick;
	int m_PendingCutsceneCamera;
	int8 m_PendingCutsceneType;
	int m_DefaultCutsceneFadeTick;

	int m_ShieldIgnoreDamageEndTick;
	int m_LastHurtSoundTick;

	//Last asked for camera position
	CMat4Dfp32 m_LastCamera;

	int8 m_iSoftSpotCount;
	uint8 m_InitialPhysMode;

	bool m_bForceReleaseAttack2;
	int8 m_GrabDifficulty;
	uint8 m_FightBBSize;
	bool m_bDisableQuicksave;
	int8 m_MountFlags;

	//int m_TimeoutTick;
	
	int m_FallDmgTimeoutTicks;
	//int m_RespawnTimeoutTicks;

	int m_DropItemTick;

	int m_iSearch;	// Path-finding

	// Equipment, last used item..? Items are given a id number (when picked up?) so 
	// we can one item of many of same type
	int32 m_LastUsedItem;
	int32 m_LastUsedItemType;
	// When to "unequip" weapon, reset each time we shoot/equip etc..
	int32 m_WeaponUnequipTimer;


	TArray<SHitloc>	m_lHitlocations;

	//Stealth ability stuff
	int m_iStealthActivateTime; //How long time character must remain unobstrusive before stealth is activated
	int m_iStealthTimer; //How long until stealth mode can be activated

	// Sneak
	uint16 m_RaisedNoiseLevel;
	uint16 m_RaisedVisibility;
	int16 m_GrabbedBody;
	fp32 m_LightIntensity;
	//uint8 m_SpecialGrab;
	fp32 m_AverageLightIntensity; // Used to determine if we're in darkness or in light..

	int8 m_NextGUIScreen;	

	TPtr<CWObj_ObjLightMeter> m_spDarknessLightMeter;	// Light meter used for darkness powers
	
	fp32	m_DarknessDrain;
	uint8	m_DarknessFlags;		// Heart left, voice tagged etc.
	uint8	m_SaveParams;			// Instructions for saving/loading
		
	// Hack to allow non-players to use darkness powers (according to pCD->m_DarknessPowersAvailable 
	// all characters can use all darkness powers currently!) Remove when properly fixed.	
	bool m_bNonPlayerDarknessUser; 
	
	int32 m_LastButton1Release;
	int32 m_LastDarklingSpawnCheck;
	//int8 m_Disable;
	int16 m_iBestDarklingSpawn;
	//The characters security clearance is max of this and what possession of items and membership in teams confer
	int8 m_MinSecurityClearance;
	// Required clearance-level in the zone the character is in (trigger_restrictedzone)
	int8 m_RequiredClearanceLevel;
	// Moved to client data
	

	class CPlayer// Player specific stuff, should be moved to an inherited class or allocated for players only
	{
	public:
		int8 m_bLightSneak : 1;
		int8 m_bSpecialClass : 1; // Keep track on if player has been respawned as a special class
		int m_SneakTriggerCount;

		int16 m_Map_ItemType;
		int m_Map_Visited;
		int m_iTemporaryItemLastTick;
		int m_iTemporaryItemType;
		int m_iTemporaryItemSender;
		CStr m_CheatItems;

		int32 m_LastInfoscreenPress;
	} m_Player;

	// Tranquillizer
	int32 m_TranquillizerOutTime;		// How many ticks should we be tranquillized. From registry. In registry we write it as seconds though.
	int32 m_TranquillizerLevel;
	int32 m_TranquillizerIncoming;
	int32 m_TranquillizerTickCount;

	TThinArray<int16> m_lFlickerLights;	// Keep a list of lights affected by character

	TArray<int> m_lControlledBots;
	int32 m_iBotControl;
	TPtr<class CAI_Core> m_spAI;
	int32 m_iController;

	uint32									m_RagdollHash;
	uint32									m_RagdollIdle;
	TThinArray<CMat4Dfp32>					m_lRagDollOffset;
	CMat4Dfp32								m_OrgMat;

#ifdef INCLUDE_OLD_RAGDOLL
	SConstraintSystemSettings				m_RagdollSettings;
	TPtr<class CConstraintSystem>			m_spRagdoll;
#endif // INCLUDE_OLD_RAGDOLL

	TPtr<class CConstraintRigidObject>		m_spRagdollDropItems[3];

	// Darklings, the one you can extract from this character and the ones 
	// currently available to the player
	struct stDarklingType
	{
		CStr m_DarklingType;
		int32 m_AddedTick;
		bool m_bTaggedForFocus;
	};
	TArray<stDarklingType>		m_lAvailableDarklings;
	
	struct SDarknessPowerup
	{
		SDarknessPowerup(const uint8& _Type, const CStr& _Data)
		{
			m_Type = _Type;
			m_Data = _Data;
		}

		SDarknessPowerup()
		{
			m_Type = 0;
			m_Data.Clear();
		}

		uint8	m_Type;
		CStr	m_Data;
	};
	TArray<SDarknessPowerup>	m_lDarknessPowerups;
	
	CStr			m_Darkness_DarklingSpawn;
	int				m_lDarklingMapping[4];
	// List of input entity objects that wants input from this character
	TArray<int16> m_liInputEntities;
	TArray<int16> m_liAnimEventEntities;

	typedef struct sGunKataMove
	{
		CAG2ImpulseValue m_Impulse;
		// Type, 0 = both, 1 = left, 2 = right
		int8		m_TriggerType;
		// Itemtype, if you want to restrict type
		int8		m_ItemType;
		// Direction, 0 = back, 1 = front
		int8		m_Direction:4;
		// ItemAnimtype, 3 = gun, 6 = rifle
		int8		m_AnimType:4;
		
	} GunKataMove;
	TArray<CAG2ImpulseValue> m_lGunPlayTypes;
	TArray<GunKataMove> m_lGunKataMoves;
	// Last event when gunplay was unsafe again
	int32 m_GunplayUnsafeTick;
	// How long to wait before attempting to gunplay
	int32 m_GunplaySafeDelay;
	int32 m_GunplaySpread;
	int32 m_GunplayReactivateDelay;

	uint16 m_iDarkness_DarklingSpawn;
	uint16 m_iDarkness_BlackHole;
	uint16 m_iDarkness_Drain_2;
	uint16 m_iDarkness_MouthSmoke;
	uint16 m_iDarkness_CreepingDark;

	uint16 m_iChainObj;					// Chain object
	uint16 m_iLastAutoUseObject;
	CGrabbedObject m_GrabbedObject;		// used to grab objects

#ifndef M_RTM
	bool m_bPhysLog;
	fp32 m_NoClipSpeedFactor;
#endif

protected:
	bool m_bHeadMatUpdated;		// Retrieving m_HeadMat and m_WeaponMat requires an update (set to false each refresh)
	CMat4Dfp32 m_BaseMat;		// This is like m_Position, but without the look-angles merged (updated at each refresh)
	CMat4Dfp32 m_HeadMat;		// Global pos+rot of head (updated at each refresh)
	CMat4Dfp32 m_WeaponMat;		// Global pos+rot of weapon muzzle (updated at each refresh)
};



#endif // _INC_WOBJ_CHAR
	
