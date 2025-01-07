#ifndef WAGI_Defs_h
#define WAGI_Defs_h

//--------------------------------------------------------------------------------

#define AGI_UNDEFINEDTIME	(CMTime::CreateFromSeconds(-1.0f))
#define AGI_LINKEDTIME		(CMTime::CreateFromSeconds(-2.0f))
#define AGI_UNDEFINEDTIMEFP32 (-1.0f)

#define AGI_MAXANIMLAYERS	(16)

//--------------------------------------------------------------------------------

#define AGI_DEBUGFLAGS_ENTERSTATE_LO_MASK		0x00000003
#define AGI_DEBUGFLAGS_ENTERSTATE_LO_SHIFT		0x00000000

#define AGI_DEBUGFLAGS_ENTERSTATE_HI_MASK		0x0000000C
#define AGI_DEBUGFLAGS_ENTERSTATE_HI_SHIFT		0x00000002

#define AGI_DEBUGFLAGS_ENTERSTATE_SERVER		0x00000001
#define AGI_DEBUGFLAGS_ENTERSTATE_CLIENT		0x00000002
#define AGI_DEBUGFLAGS_ENTERSTATE_PLAYER		0x00000004
#define AGI_DEBUGFLAGS_ENTERSTATE_NONPLAYERS	0x00000008

//--------------------------------------------------------------------------------

#define AGI_PACKEDAGI_RESOURCEINDICES			0x01
#define AGI_PACKEDAGI_TOKENS					0x02
#define AGI_PACKEDAGI_REMTOKENIDS				0x04
#define AGI_PACKEDAGI_OVERLAYANIM				0x08
#define AGI_PACKEDAGI_RANDSEED					0x10
#define AGI_PACKEDAGI_HASENTERSTATEENTRIES		0x20
#define AGI_PACKEDAGI_ISDISABLED				0x40
#define AGI_PACKEDAGI_OVERLAYANIMLIPSYNC		0x80

#define AGI_PACKEDTOKEN_REFRESHGAMETIME			0x01
#define AGI_PACKEDTOKEN_PLAYINGSTATEINSTANCE	0x02
#define AGI_PACKEDTOKEN_SIPS					0x04
#define AGI_PACKEDTOKEN_REMSIIDS				0x08

//--------------------------------------------------------------------------------

#define AGI_TOKENREFRESHFLAGS_REFRESH			0x01
#define AGI_TOKENREFRESHFLAGS_PERFORMEDACTION	0x02
#define AGI_TOKENREFRESHFLAGS_FINISHED			0x04
#define AGI_TOKENREFRESHFLAGS_FAILED			0x08

//--------------------------------------------------------------------------------

#endif /* WAGI_Defs_h */
