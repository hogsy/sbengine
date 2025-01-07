#ifndef WAG2I_Defs_h
#define WAG2I_Defs_h

//--------------------------------------------------------------------------------

#define AG2I_UNDEFINEDTIME	(CMTime::CreateFromSeconds(-1.0f))
#define AG2I_LINKEDTIME		(CMTime::CreateFromSeconds(-2.0f))
#define AG2I_UNDEFINEDTIMEFP32 (-1.0f)

#define AG2I_MAXANIMLAYERS	(16)

//--------------------------------------------------------------------------------

#define AG2I_DEBUGFLAGS_ENTERSTATE_LO_MASK		0x00000003
#define AG2I_DEBUGFLAGS_ENTERSTATE_LO_SHIFT		0x00000000

#define AG2I_DEBUGFLAGS_ENTERSTATE_HI_MASK		0x0000000C
#define AG2I_DEBUGFLAGS_ENTERSTATE_HI_SHIFT		0x00000002

#define AG2I_DEBUGFLAGS_ENTERSTATE_SERVER		0x00000001
#define AG2I_DEBUGFLAGS_ENTERSTATE_CLIENT		0x00000002
#define AG2I_DEBUGFLAGS_ENTERSTATE_PLAYER		0x00000004
#define AG2I_DEBUGFLAGS_ENTERSTATE_NONPLAYERS	0x00000008

//--------------------------------------------------------------------------------

#define AG2I_PACKEDAG2I_RESOURCEINDICES			0x01
#define AG2I_PACKEDAG2I_TOKENS					0x02
#define AG2I_PACKEDAG2I_REMTOKENIDS				0x04
#define AG2I_PACKEDAG2I_OVERLAYANIM				0x08
#define AG2I_PACKEDAG2I_RANDSEED					0x10
#define AG2I_PACKEDAG2I_HASENTERSTATEENTRIES		0x20
#define AG2I_PACKEDAG2I_ISDISABLED				0x40
#define AG2I_PACKEDAG2I_OVERLAYANIMLIPSYNC		0x80

#define AG2I_PACKEDTOKEN_REFRESHGAMETIME			0x01
#define AG2I_PACKEDTOKEN_PLAYINGSTATEINSTANCE	0x02
#define AG2I_PACKEDTOKEN_SIPS					0x04
#define AG2I_PACKEDTOKEN_REMSIIDS				0x08

//--------------------------------------------------------------------------------

#define AG2I_TOKENREFRESHFLAGS_REFRESH			0x01
#define AG2I_TOKENREFRESHFLAGS_PERFORMEDACTION	0x02
#define AG2I_TOKENREFRESHFLAGS_FINISHED			0x04
#define AG2I_TOKENREFRESHFLAGS_FAILED			0x08

//--------------------------------------------------------------------------------

#endif /* WAG2I_Defs_h */
