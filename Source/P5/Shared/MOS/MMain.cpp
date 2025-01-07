
#include "PCH.h"
#include "MMain.h"

#ifdef PLATFORM_WIN

	#ifdef MRTC_MEMORYDEBUG
		#define MCCNAME "MCCDYN.DLL"
	#else
		#define MCCNAME "MCCDYN.DLL"
	#endif

	#ifdef MRTC_MEMORYDEBUG
		#include "crtdbg.h"
	#endif

#endif



#include "MMain_PS3.cpp"

