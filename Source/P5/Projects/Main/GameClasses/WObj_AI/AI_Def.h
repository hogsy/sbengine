#ifndef _INC_AI_DEF
#define _INC_AI_DEF

#include "../WObj_Messages.h"

//Common AI-related definitions
#define AI_GAME_P5

//AI related classes messagebases (Assume AI messages spans 0x1000 slots)
enum {
	OBJMSGBASE_BEHAVIOUR	= OBJMSGBASE_AI + 0x400,
	OBJMSGBASE_TEAM			= OBJMSGBASE_AI + 0x450,
	OBJMSGBASE_AREAINFO		= OBJMSGBASE_AI + 0x500,
	OBJMSGBASE_NAVNODE		= OBJMSGBASE_AI + 0x550,
	OBJMSGBASE_AIQUERY		= OBJMSGBASE_AI + 0x600,
	// 0x700 reserved
};

//Enum bases
enum{
	ACTION_OVERRIDE_ENUM_BASE		= 64, //The starting number of any behaviour type enumerator id
	PERSONALITY_ENUM_BASE			= 96, //The starting number of any personality type enumerator id
};

//AI-controlled objects should be able to handle these messages
enum {
	OBJMSG_AIQUERY_GETAI = OBJMSGBASE_AIQUERY,	//Set the CAI_Core pointer that data points at to the agent's AI pointer.  
	OBJMSG_AIQUERY_GETACTIVATEPOSITION,		    //Set the CMat4Dfp32 that data points at to activate position of agent
	OBJMSG_AIQUERY_ISALIVE,						//Check if the agent is considered to be "alive" i.e. functional, not no-clipping etc.
	OBJMSG_AIQUERY_GETMAXSPEED,					//Set the fp32 that data points at to maximum speed in the direction specified as param0, see enum below
//	OBJMSG_AIQUERY_GETAIMPOSITION,				//Set the CVec3Dfp32 that data points at to position from which agent aims
	OBJMSG_AIQUERY_STEALTHMODE,					//Check if agent is in stealth mode
	OBJMSG_AIQUERY_RPGATTRIBUTE,				//Return value of rpg attribute specified in the param0, see enum below.
	OBJMSG_AIQUERY_GETBODYANGLEZ,				//Set the fp32 that data points at to the body heading of an agent whose "body" isn't automatically rotated along with his look
	OBJMSG_AIQUERY_GETRPGITEM,					//Set the CRPG_Object_Item pointer that data points at to currently wielded item of type given in param0 (RPG_CHAR_INVENTORY_WEAPONS etc)
	OBJMSG_AIQUERY_GETRPGINVENTORY,				//Set the CRPG_Object_Inventory pointer that data points at to inventory of the type given in param0 (RPG_CHAR_INVENTORY_WEAPONS etc)
	OBJMSG_AIQUERY_GETVULNERABLEPOS,			//Set the CVec3Dfp32 that the data points at to the vulnerable position of the agent (e.g. head for characters)
//	OBJMSG_AIEFFECT_SETMOVEMODE,				//Notifies object of preferred movement mode.
	OBJMSG_AIEFFECT_STARTOFBEHAVIOR,			//Notifies the game that a particular object has started a particular behaviour anim
	OBJMSG_AIEFFECT_ENDOFBEHAVIOR,				//AG signals that a behaviour has ended
	OBJMSG_AIEFFECT_SETFACIAL,					//Tell ai what facial to use
	OBJMSG_AIEFFECT_SETHURT,					//Play a (very) specific hurt anim on the target, no damage is actually inflicted
	OBJMSG_AIEFFECT_RETARGET,					//AI should pick a new target
};

//The param0 of a OBJMSG_AIQUERY_GETMAXSPEED-message
enum
{
	AIQUERY_GETMAXSPEED_GENERAL = 0,
	AIQUERY_GETMAXSPEED_FORWARD,
	AIQUERY_GETMAXSPEED_BACKWARD,
	AIQUERY_GETMAXSPEED_SIDESTEP,
	AIQUERY_GETMAXSPEED_JUMP,
	AIQUERY_GETMAXSPEED_UP,
	AIQUERY_GETMAXSPEED_DOWN,
	AIQUERY_GETMAXSPEED_WALK,
};

//The param0 of a OBJMSG_AIQUERY_RPGATTRIBUTE-message
enum
{
	AIQUERY_RPGATTRIBUTE_HEALTH = 0,
	AIQUERY_RPGATTRIBUTE_MAXHEALTH,
	AIQUERY_RPGATTRIBUTE_WAIT,
	AIQUERY_RPGATTRIBUTE_MANA,
	AIQUERY_RPGATTRIBUTE_MAXMANA,
	AIQUERY_RPGATTRIBUTE_AMMO,			//Param1 is weapon type to associate ammo with,  Return -1 if weapon does not draw ammo.
	AIQUERY_RPGATTRIBUTE_LOADEDAMMO,	//Ammo in magazine. Otherwise as above.
};

// m_Reason tells us why we entered fightmode (OBJMSG_CHAR_ENTERFIGHTMODE)
#define ENTERFIGHTMODE_MELEE				0
#define ENTERFIGHTMODE_ENTERBREAKNECK		1
#define ENTERFIGHTMODE_EXITBREAKNECK		2
#define ENTERFIGHTMODE_ENTERBREAKNECK_LOUD	3
#define ENTERFIGHTMODE_EXITBREAKNECK_KILL	4
#define ENTERFIGHTMODE_BREAKNECK			255


// Used to get number of elements of a static array
#ifndef sizeof_buffer
# define sizeof_buffer(buf) (sizeof(buf)/sizeof(buf[0]))
#endif

#endif
