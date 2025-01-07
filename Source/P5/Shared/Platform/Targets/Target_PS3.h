
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030606:		Added Comments
\*_____________________________________________________________________________________________*/

#ifndef __INC_TARGET_PS3
#define __INC_TARGET_PS3

#ifndef	TARGET_PS3
#define TARGET_PS3
#endif	// TARGET_PS2

// -------------------------------------------------------------------
//  COMPILE SETTINGS
// -------------------------------------------------------------------
//#define CPU_LITTLEENDIAN 0x0001				// Def. byte ordn.
#define CPU_BIGENDIAN 0x0001

//#define CPU_IA64						// Intel Merced
//#define CPU_INTELP5						// Intel Pentium
//#define CPU_X86
//#define CPU_ALPHA21164				// DEC, Alpha
//#define CPU_MIPSR4400					// MIPS RISC
//#define CPU_MIPSR3000					// MIPS för sur-playstation
#define CPU_POWERPC					// IBM/Motorolja, PowerPC
//#define CPU_SH2						// Hitachi RISC, SH2
//#define CPU_SH4						// Hitachi RISC, SH4
//#define	CPU_MIPS_PS2					// MIPS R5900 for PS2

//#define PLATFORM_WIN64				// x86, IA64, Alpha, SPARC
//#define PLATFORM_WIN32					// x86, IA64, Alpha, [MIPS, PPC]
//#define PLATFORM_WIN32_PC				// x86, IA64, Alpha, [MIPS, PPC]
//#define PLATFORM_LINUX				// x86
//#define PLATFORM_DOS					// x86
//#define PLATFORM_WIN32MAC				// PPC
//#define PLATFORM_MACOS				// PPC
//#define PLATFORM_BEOS					// BeOS, x86
//#define PLATFORM_DREAMCAST			// SH4
//#define PLATFORM_WIN32CONSOLE			// <- Once implemented, now it's broken!!!
//#define PLATFORM_PS2
#define PLATFORM_PS3
#define PLATFORM_CONSOLE

//#define COMPILER_MSVC					// Microsoft Visual C++ v4.0+
//#define COMPILER_MSVC_ICC24			// Intel C++ MSVC Plug-in compiler v2.4+
#define COMPILER_GNU					// gcc
//# define COMPILER_GNU_2				// gcc 2.95
#define COMPILER_GNU_3					// gcc 3.1
#define COMPILER_CPP_EXCEPTIONS			// C++ exception handling is supported
#define COMPILER_NEEDOPERATORDELETE
//#define COMPILER_PLACEMENT_NEW
#define __PLACEMENT_NEW_INLINE

#define CPU_INT32						// An 'int' is 32 bits
//#define CPU_INT64						//
//#define CPU_INT128					//
//#define CPU_PTR32						// A pointer is 32 bits
#define CPU_PTR64						//
//#define CPU_PTR128					//
//#define CPU_PTRMIX					//

//#define CPU_FEATURE_P6			1
//#define CPU_FEATURE_MMX			2
//#define CPU_FEATURE_SSE			4
//#define CPU_FEATURE_3DNOW		8

//#define	CPU_SOFTWARE_FP8						// CPU does doubles in software (avoid at all cost!)
#define CPU_SUPPORT_FP8							// CPU supports double
#define CPU_QWORD_ALIGNMENT						// CPU requires QuadWord alignment for some datatypes
#define CPU_ALIGNED_MEMORY_ACCESS				// All datatypes must be correctly aligned

#define	NO_INLINE_FILESYSTEM

#define	DEF_DISABLE_PERFGRAPH

#define	UNICODE_WORKAROUND

// no 43 matrices
//#define DEFINE_MAT43_IS_MAT4D
// -------------------------------------------------------------------
//  DEBUG COMPILER SETTINGS
// -------------------------------------------------------------------
#ifndef M_RTM
#define DEBUG_LOGFILEENBLE
#define DEBUG_ERRORFILEENBLE
#endif

#define	dllvirtual

//#define COMPILER_RTTI					// RTTI is supported
#define M_FAKEDYNAMICCAST
#define M_EXCEPTIONS 0
#define M_FILEERROR M_BREAKPOINT

#define IMAGE_IO_NOJPG
#define IMAGE_IO_NOPCX
#define IMAGE_IO_NOGIF
#define	IMAGE_IO_NOS3TC
#define IMAGE_IO_NOVORBIS
#define SOUND_IO_NOCOMPRESS
#define	SOUND_IO_NODECOMPRESS
#define IMAGE_IO_PNG

#define M_DISABLE_CURRENTPROJECT
#define M_DISABLE_TODELETE
#define M_DISABLE_CWOBJECT_TRIGGER_TELEPORT
#define M_DISABLE_CWOBJECT_FUNC_LOD
#define M_DISABLE_CWOBJECT_FUNC_MIRROR
#define M_DISABLE_CWOBJECT_FUNC_PORTAL
#define M_DISABLE_CWOBJECT_SYSTEM


#define M_STATICINIT
#define M_STATIC
//#define M_STATIC_RENDERER

#define M_ARGLISTCALL
#define M_INLINE	inline
#define M_CDECL
#define M_STDCALL
void DebugBreak();
#define M_BREAKPOINT DebugBreak()
#define M_FORCEINLINE inline

#define M_RESTRICT 
#define M_ALIGN(_Align) __attribute__((aligned(_Align)))

#define M_ATOMICALIGNMENT 128

#define M_IMPORTBARRIER asm volatile("lwsync\n");
#define M_EXPORTBARRIER asm volatile("lwsync\n");
#define M_THREADSPINCOUNT 400


//#define MRTC_THREADLOCAL __thread
#define MRTC_THREADLOCAL

#define M_TRY
#define M_CATCH(_ToCatch)

//#define M_SIZEOF(_Class, _Member) sizeof(_Class::_Member)
//#define M_OFFSETOF(_Class, _Member) __offsetof__(_Class::_Member)

#define M_OFFSET(_Class, _Member, _Dest) aint _Dest;\
			{\
				const static _Class *pPtr = 0;\
				_Dest = (aint)(void *)(&((pPtr)->_Member));\
			}
#define _MAX_PATH 256

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| STANDARD TYPES
|__________________________________________________________________________________________________
\*************************************************************************************************/

typedef signed char				int8;
typedef unsigned char			uint8;
typedef signed short			int16;
typedef unsigned short			uint16;
typedef signed int				int32;
typedef unsigned int			uint32;
typedef float					fp4;
typedef double					fp8;

typedef vector float			vec128;

//#define M_SEPARATETYPE_int

#if defined(__LP32__)
	typedef signed long long	int64;
	typedef unsigned long long	uint64;
	typedef signed int			aint;
	typedef unsigned long		auint;
	typedef unsigned int		mint;
	typedef	signed int			smint;
#elif defined(__LP64__)
	typedef signed long			int64;
	typedef unsigned long		uint64;
	typedef signed long			aint;
	typedef unsigned long		auint;
	typedef unsigned long		mint;
	typedef	signed long			smint;
#else
#error	"moo"
#endif

typedef unsigned int			uint;
typedef int64					fint;

typedef uint16					wchar;

#endif	// __INC_TARGET_PS3
