
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/

#ifndef __INC_MCCINC
#define __INC_MCCINC

#ifdef MCCDLL

	#ifndef MRTC_DLL
		#define MRTC_DLL
	#endif


	// Both has to be dllexport here to make linking to exporting libraries work
	#ifdef MCCDLLINTERFACE
		#define MCCDLLEXPORT __declspec(dllexport)
	#else
		#define MCCDLLEXPORT __declspec(dllexport)
//		#define MCCDLLEXPORT __declspec(dllimport)
	#endif

	#pragma warning(disable : 4275) // non dll-interface class 'xxx' used as base for dll-interface class
	#pragma warning(disable : 4251) // 'xxx' needs to have dll-interface to be used by clients of class 'yyy'

#else
	#define MCCDLLEXPORT

#endif
#endif /// __INC_MCCINC

#include "MRTC.h"

