#ifndef __WPACKETS_H
#define __WPACKETS_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Global packet enums

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		
\*____________________________________________________________________________________________*/

enum
{
	WPACKET_VOID =				0,
	WPACKET_CONTROLS,
	WPACKET_PLAYERINFO,
	WPACKET_DELTAFRAME,
	WPACKET_STATUSBARINFO,
	WPACKET_SOUND,

	WPACKET_LARGESIZE =			8,		// Not yet used.

	WPACKET_WORLD =				16,
	WPACKET_FILEREQUEST,
	WPACKET_FILEFRAGMENT,
	WPACKET_COMMAND,
	WPACKET_RESOURCEID,
	WPACKET_ENTERGAME,
	WPACKET_DELTAREGISTRY,
	WPACKET_SETVAR,
	WPACKET_OBJNETMSG,
	WPACKET_NULLOBJNETMSG,
	WPACKET_SAY,
	WPACKET_DONEPRECACHE,				// From client to server
	WPACKET_DOPRECACHE,
	WPACKET_PAUSE,
	WPACKET_SINGLESTEP,
	WPACKET_LOCALFRAMEFRACTION,
	WPACKET_SOUNDSYNC,					// Client <-> Server

	WPACKET_LAST,
};

#endif
