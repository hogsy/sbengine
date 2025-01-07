/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030606:		Added Comments
\*_____________________________________________________________________________________________*/

#ifndef _INC_MMAIN
#define _INC_MMAIN

#include "MCC.h"
#include "MOS.h"

#ifdef PLATFORM_WIN_PC
	int MOSInit(void* this_inst, void* prev_inst, char* cmdline, int cmdshow, const char* _pAppClassName, const char *_pSystemClassName = NULL);
	void MOSDestroy();
#endif


#ifdef PLATFORM_XBOX

	int Win32_Main(void* this_inst, void* prev_inst, char* cmdline, int cmdshow, const char* _pAppClassName, const char *_pSystemClassName = NULL);

	#define MACRO_MAIN(AppClassName)													\
	int M_CDECL main()																	\
	{																					\
		return Win32_Main(NULL, NULL, NULL, 0, AppClassName);							\
	};

	#define MACRO_MAINEX(AppClassName, SystemClassName)								\
	int M_CDECL main()																	\
	{																					\
		return Win32_Main(NULL, NULL, NULL, 0, AppClassName, SystemClassName);			\
	};

#elif defined PLATFORM_WIN

	int Win32_Main(void* this_inst, void* prev_inst, char* cmdline, int cmdshow, const char* _pAppClassName, const char *_pSystemClassName = NULL);

	#define MACRO_MAIN(AppClassName)													\
	int __stdcall WinMain(HINSTANCE__* this_inst, HINSTANCE__* prev_inst, char* cmdline, int cmdshow)	\
	{																					\
		return Win32_Main(this_inst, prev_inst, cmdline, cmdshow, AppClassName);		\
	};

	#define MACRO_MAINEX(AppClassName, SystemClassName)													\
	int __stdcall WinMain(HINSTANCE__* this_inst, HINSTANCE__* prev_inst, char* cmdline, int cmdshow)	\
	{																					\
		return Win32_Main(this_inst, prev_inst, cmdline, cmdshow, AppClassName, SystemClassName);		\
	};

#elif defined PLATFORM_SHINOBI

	void Shinobi_Main( const char* _pAppClassName, const char *_pSystemClassName = NULL);
	extern "C" void MoveStack(long newlocation);

	#define MACRO_MAIN(AppClassName) \
	void main( void ) \
	{ \
		long newptr = 0x8d000000;	\
		MoveStack(newptr);			\
		Shinobi_Main(AppClassName);	\
	};

	#define MACRO_MAINEX(AppClassName, SystemClassName) \
	void main( void ) \
	{ \
		long newptr = 0x8d000000;	\
		MoveStack(newptr);			\
		Shinobi_Main(AppClassName, SystemClassName);	\
	};

#elif defined PLATFORM_DOLPHIN

	int Dolphin_Main(const char* _pAppClassName);

	#define MACRO_MAIN(AppClassName)										\
	void main()																\
	{																		\
		Dolphin_Main(AppClassName);											\
	};

	#define MACRO_MAINEX(AppClassName, SystemClassName)						\
	void main()																\
	{																		\
		Dolphin_Main(AppClassName);											\
	};

#elif defined PLATFORM_PS2 && defined(COMPILER_GNU)
	int PS2_Main( const char* _pAppClassName );

	#define MACRO_MAIN(AppClassName)\
	void main()\
	{\
		PS2_Main( AppClassName );\
	};

#elif defined(PLATFORM_PS2)	&& defined(COMPILER_CODEWARRIOR)
	#include "mwUtils.h"

	int  PS2_Main(const char* _pAppClassName);

	#define MACRO_MAIN(AppClassName)		\
	void main()								\
	{										\
		mwInit();							\
		PS2_Main( AppClassName );			\
		mwExit();							\
	}

#elif defined(PLATFORM_PS3)
	int PS3_Main(const char* _pAppClassName);

	#define MACRO_MAIN(AppClassName)\
	int main(int argc, char** argv)\
	{\
		PS3_Main(AppClassName);\
	};
#else

#error "Implement this"

#endif

#endif
