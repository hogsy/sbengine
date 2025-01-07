#ifndef __WRPGDEF_H
#define __WRPGDEF_H

#include "../WObj_Messages.h"

enum
{
	OBJMSG_RPGRET_CONTINUE = 0,
	OBJMSG_RPGRET_DELETE = -1,

	OBJMSG_RPGRET_YES = 1,
	OBJMSG_RPGRET_NO = 2,

	OBJMSG_STAFF_GETATTACKMANACOST = OBJMSGBASE_RPG,
	OBJMSG_STAFF_GETSHIELDMANACOST,
	OBJMSG_STAFF_GETSHOCKWAVEMANACOST,
	OBJMSG_STAFF_GETSUMMONMANACOST,
	OBJMSG_STAFF_GETSUMMONTIME,

	OBJMSG_BOW_GETBURSTCURVELOCITY,
	OBJMSG_BOW_GETMAXLOADABLE,
	OBJMSG_BOW_GETCURRENTLYLOADED,

	OBJMSG_RPG_GETINITPARAMS,

	OBJMSG_DRAGONBREATH_GETDURATION,

	OBJMSG_MELEE_AUTOCOMBO,			//Will cause melee weapon to automatically perform next Param0 number of combos
	OBJMSG_MELEE_STOP_AUTOCOMBO,	//Will cancel any upcoming autocombos

	OBJMSG_SUMMON_SPAWNPROJECTILES,
	OBJMSG_SUMMON_LOCKLINKEDFIRE,
	OBJMSG_SUMMON_REMOVEPROJECTILES,
};

enum
{
	// Make sure to update this Ogier string in nodetype if changing any values below
	// Blow=0x00000001,Pierce=0x00000002,Tranquillizer=0x00000004,Pistol=0x00000008,Rifle=0x00000010,Darkness=0x00000020,Fall=0x00000040,Blast=0x00000080,AP=0x00000100,Physics=0x00000200,DarknessGun=00000x0400,Light=0x00000800,Flash=0x00001000,BlackHole=0x00002000,Blockable=0x00004000,Fire=0x00008000,=0x00010000,=0x00020000,=0x00040000,=0x00080000,=0x00100000,=0x00200000,=0x00400000,=0x00800000,=0x01000000,=0x02000000,=0x04000000,=0x08000000,=0x10000000,=0x20000000,=0x40000000,NoCritical=0x80000000

	DAMAGETYPE_UNDEFINED		= 0,

	// Damage types
	DAMAGETYPE_BLOW				= M_Bit(0),		// 0x0001
	DAMAGETYPE_PIERCE			= M_Bit(1),		// 0x0002
	DAMAGETYPE_TRANQUILLIZER	= M_Bit(2),		// 0x0004
	DAMAGETYPE_PISTOL			= M_Bit(3),		// 0x0008
	DAMAGETYPE_RIFLE			= M_Bit(4),		// 0x0010
	DAMAGETYPE_DARKNESS			= M_Bit(5),		// 0x0020
	DAMAGETYPE_FALL				= M_Bit(6),		// 0x0040
	DAMAGETYPE_BLAST			= M_Bit(7),		// 0x0080
	DAMAGETYPE_AP				= M_Bit(8),		// 0x0100
	DAMAGETYPE_PHYSICS			= M_Bit(9),		// 0x0200
	DAMAGETYPE_DARKNESS_GUN		= M_Bit(10),		// 0x0400
	DAMAGETYPE_LIGHT			= M_Bit(11),		// 0x0800
	DAMAGETYPE_FLASH			= M_Bit(12),		// 0x1000
	DAMAGETYPE_BLACKHOLE		= M_Bit(13),		// 0x2000
	DAMAGETYPE_BLOCKABLE		= M_Bit(14),		// 0x4000
	DAMAGETYPE_FIRE				= M_Bit(15),		// 0x8000

	// Type masks
	DAMAGETYPE_MELEE_MASK		= DAMAGETYPE_BLOW | DAMAGETYPE_PIERCE | DAMAGETYPE_DARKNESS | DAMAGETYPE_FALL,
	DAMAGETYPE_DISORIENT_MASK	= DAMAGETYPE_FLASH,

	// Damge flags
	DAMAGEFLAG_NO_CRIT			= M_Bit(31),	// If this flag is set the damage factor cannot go above 1.0f
	
	// Flag masks
	DAMAGEFLAG_MASK				= DAMAGEFLAG_NO_CRIT,
	DAMAGETYPE_MASK				= ~DAMAGEFLAG_MASK,

	NUM_DAMAGETYPES				= 17,
	MAX_DAMAGETYPES				= 32,
};

#endif
