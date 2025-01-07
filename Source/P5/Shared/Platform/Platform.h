
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 1996,2001,2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		010301:		Moved platform definitions here to be independent of MRTC
		030606:		Updated Comments
\*_____________________________________________________________________________________________*/

#ifndef _INC_PLATFORM
#define _INC_PLATFORM

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Include compile-settings for target
|__________________________________________________________________________________________________
\*************************************************************************************************/

#ifdef TARGET_DREAMCAST_SHINOBI
	#include "Targets/Target_Dreamcast_Shinobi.h"

#elif defined TARGET_DREAMCAST_CE
	#include "Targets/Target_Dreamcast_WinCE.h"

#elif defined TARGET_WIN32_X86
	#include "Targets/Target_Win32_x86.h"

#elif defined TARGET_WIN32_X86_SSE
	#include "Targets/Target_Win32_x86_SSE.h"

#elif defined TARGET_WIN32_X86_SSE2
	#include "Targets/Target_Win32_x86_SSE2.h"

#elif defined TARGET_WIN64_AMD64
	#include "Targets/Target_Win64_AMD64.h"

#elif defined TARGET_XBOX
	#include "Targets/Target_XBox.h"

#elif defined TARGET_XENON
	#include "Targets/Target_Xenon.h"

#elif defined TARGET_DOLPHIN
	#include "Targets/Target_Dolphin.h"

#elif defined TARGET_PS2
	#include "Targets/Target_PS2.h"

#elif defined(TARGET_PS3) || defined(SN_TARGET_PS3)
	#include "Targets/Target_PS3.h"

#else
	#error "No target platform defined."

#endif

#endif // _INC_PLATFORM


