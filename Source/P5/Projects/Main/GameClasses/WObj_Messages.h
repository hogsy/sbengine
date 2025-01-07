#ifndef __WOBJ_MESSAGES_H
#define __WOBJ_MESSAGES_H

enum
{
	OBJMSGBASE_PRIMITIVES =	0x900,
	OBJMSGBASE_PLAYER = 0x1000,
	OBJMSGBASE_RPGOBJ = 0x2000,
	OBJMSGBASE_SPELLS = 0x3000,
	OBJMSGBASE_MISC = 0x4000,
	OBJMSGBASE_TRIGGER = 0x5000,
	OBJMSGBASE_EMITTER = 0x5500,
	OBJMSGBASE_AI = 0x6000,
	OBJMSGBASE_RPG = 0x7000,
};

enum
{
	// Base for MISC group	( Can't be higher than 0xfff )				// OLD HEX	// MESSAGES			( ALLOW MSGS )		- NOTES
	OBJMSGBASE_MISC_LADDER				= OBJMSGBASE_MISC + 0x0,		// + 0x64,	// 21 messages		( Allows  32 )
	OBJMSGBASE_MISC_ACTIONCUTSCENE		= OBJMSGBASE_MISC + 0x20,		// + 0xC8,	// 27 messages		( Allows  32 )
	OBJMSGBASE_MISC_CREEPINGDARK		= OBJMSGBASE_MISC + 0x40,		// + 0xff,	// 5 messages		( Allows  16 )		- Overwrote chain's messages
	OBJMSGBASE_MISC_CHAIN				= OBJMSGBASE_MISC + 0x50,		// + 0x100,	// 5 messages		( Allows  16 )
	OBJMSGBASE_MISC_LEDGE				= OBJMSGBASE_MISC + 0x60,		// + 0x12C,	// 9 messages		( Allows  16 )
	OBJMSGBASE_MISC_MOTH				= OBJMSGBASE_MISC + 0x70,		// + 0x190,	// 2 messages		( Allows  16 )
	OBJMSGBASE_MISC_RAIN				= OBJMSGBASE_MISC + 0x80,		// + 0x200,	// 1 messages		( Allows  16 )		- Was same as glass
	OBJMSGBASE_MISC_GLASS				= OBJMSGBASE_MISC + 0x90,		// + 0x200,	// 2 messages		( Allows  16 )		- Was same as rain
	OBJMSGBASE_MISC_INPUTENTITY			= OBJMSGBASE_MISC + 0xA0,		// + 0x300,	// 3 messages		( Allows  16 )
	OBJMSGBASE_MISC_ANIMEVENTLISTENER	= OBJMSGBASE_MISC + 0xB0,		// + 0x320,	// 2 messages		( Allows  16 )
	OBJMSGBASE_MISC_OBJECT				= OBJMSGBASE_MISC + 0xC0,		// + 0x400,	// 2 messages		( Allows  16 )
	OBJMSGBASE_MISC_EFFECTSYSTEM		= OBJMSGBASE_MISC + 0xD0,		//			// 1 message(s)		( Allows  16 )
	OBJMSGBASE_MISC_DARKLINGSPAWN		= OBJMSGBASE_MISC + 0xE0,
	OBJMSGBASE_MISC_SWINGDOOR			= OBJMSGBASE_MISC + 0xF0,
};

enum
{
	kColorWhite		= 0x7fffffff,
	kColorRed		= 0x7fff0000,
	kColorGreen		= 0x7f00ff00,
	kColorLightGreen= 0x7f3fff3f,
	kColorBlue		= 0x7f0000ff,
	kColorYellow	= 0x7fffff00,
	kColorPurple	= 0x7fff00ff,
	kColorCyan		= 0x7f00ffff,

	kColorDkWhite	= 0x7f3f3f3f,
	kColorDkRed		= 0x7f00003f,
	kColorDkGreen	= 0x7f003f00,
	kColorDkBlue	= 0x7fff003f,
	kColorDkYellow	= 0x7f3f3f00,
	kColorDkPurple	= 0x7f3f003f,

	kSPColorRoam	= kColorGreen,
	kSPColorSearch	= kColorBlue,
	kSPColorTactical= kColorRed,
	kSPColorCover	= kColorYellow,
	kSPColorTalk	= kColorLightGreen,
	kSPColorLook	= kColorCyan,
	kSPColorDarkling = kColorDkPurple,
};

#endif
