#define CDE_EXCLUDE		// Excludes this file from ToolDocen

#ifndef _INC_MOS_MRTC
#define _INC_MOS_MRTC

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Run-time class system.
								
	Author:			Magnus Högdahl
					
	Copyright:		Starbreeze Studios AB 1996,2001
					
	Contents:		MRTC_ObjectManager
					MRTC_ClassRegistry
					MRTC_ClassContainer
					CCException
					CReferenceCount
					CObj
					TPtr
					Standard types
					Standard constants
					Run-time class macros.
					Memory management macros.
					
					
	Comments:		Core classes for dynamic object creation, class library management, 
					c++ exception handling, string handling and the smart-pointer template TPtr.
					
	History:	
		960901:		Rewrote whole file from scratch.

		960914:		Renamed file for compability reasons

		9610??:		Merged with other files.

		980825:		Changed name from MStructs to MRTC.
					Multi-use of DLL's & use of app's classes from DLL.
					Now there are NO GLOBALS EXCEPT THE OBJECT MANAGER!

		0003??:		Added Fast-string template.

		001015:		Instance counter for dynamically created objects.
					Useful for tracking undeleted objects in cases of premateure
					module unloads.
		
					Classes dumped on module load/unload.

		010131:		Converted documentation format and added comments

		011024:		SCB merged into MCC/MRTC.
					MRTC_SystemInfo
					MCC/MRTC ported to Dolphin.

		011027:		Code split up into more files. Cleaned up stuff.

		011210:		Placement new added to DECLARE/IMPLEMENT_OPERATOR_NEW.

\*____________________________________________________________________________________________*/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯









		   !!! THIS FILE IS STRICTLY OFF-LIMITS FOR EVERYONE EXCEPT THE AUTHOR !!!

		                      !!! READ-ONLY, CAPISHE? !!!









\*____________________________________________________________________________________________*/

#ifdef MRTC_DLL
	#define MRTCDLLEXPORT __declspec(dllexport)
#else
	#define MRTCDLLEXPORT
#endif


#ifdef M_RTM
#undef _DEBUG
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Include compile-settings for target
|__________________________________________________________________________________________________
\*************************************************************************************************/

#include "../Platform/Platform.h"

//#if (defined(PLATFORM_CONSOLE) && !defined(M_RTM)) || defined(PLATFORM_WIN_PC)
#if !defined(M_RTM)
#define M_Profile
#endif

#if defined(M_RTM) && defined(PLATFORM_CONSOLE)
//	#define M_RTMCONSOLE
#endif

#if defined(PLATFORM_PS2)
	#if __ide_target("PS2 Profiler") || __ide_target("PS2 Profiler NoPCH")
		#define MRTC_ENABLE_MSCOPE
	#endif
#endif
#if (!defined( _DEBUG ) || defined(M_MIXEDDEBUG)) && !defined( M_RTM ) && !defined(MRTC_ENABLE_MSCOPE)
	#define MRTC_ENABLE_MSCOPE
#endif

#define MRTC_MULTITHREADED

#ifndef MRTC_DISABLE_REMOTEDEBUGGER

	#ifdef PLATFORM_XENON
	#	define MRTC_ENABLE_REMOTEDEBUGGER
	#elif defined(PLATFORM_XBOX1) && defined(M_Profile)
	#ifndef _DEBUG
	#	define MRTC_ENABLE_REMOTEDEBUGGER
	#endif
	#elif defined(PLATFORM_WIN_PC) && !defined(M_RTM)
	#	define MRTC_ENABLE_REMOTEDEBUGGER
	#endif

#endif // MRTC_DISABLE_REMOTEDEBUGGER

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
//#	define MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCLASSES
#ifdef M_Profile
#	define MRTC_ENABLE_REMOTEDEBUGGER_USESCOPECATEGORY
#endif
#if !defined(M_RTM)
//#	define MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCLASSES
#endif
#ifdef M_Profile
//#	define MRTC_ENABLE_REMOTEDEBUGGER_USESCOPE
//#	define MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTSCOPE
#	define MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCATEGORY
#endif
#	define MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCALLSTACK
#endif

///#if !defined(M_RTM)
//#if defined(PLATFORM_XBOX) && defined(_DEBUG)
#if defined(_DEBUG) && !defined(M_MIXEDDEBUG)
#define M_SUPPORTMEMORYDEBUG
#define MRTC_MEMORYDEBUG
#endif
//#endif

#if (!defined(_DEBUG) || defined(M_MIXEDDEBUG)) && defined (PLATFORM_WIN_PC)
	#define MRTC_DEFAULTMAINHEAP
#endif

#if (!defined(_DEBUG) || defined(M_MIXEDDEBUG)) && defined (PLATFORM_XBOX)
// && !defined(M_Profile)
	#define MRTC_DEFAULTMAINHEAP
#endif

#if defined(M_RTM) && defined(PLATFORM_DOLPHIN)
	#define MRTC_DEFAULTMAINHEAP
#endif

//#if defined(M_RTM) && defined(PLATFORM_PS2)
//	#define MRTC_DEFAULTMAINHEAP
//#endif

#ifdef	PLATFORM_WIN_PC
//#define MRTC_SAFEDISC_LT
//#define MRTC_VERSION_PC_US
#endif	// PLATFORM_WIN_PC
//#define MRTC_XBOXDEMO

#if defined(M_DEMO) && defined(PLATFORM_XBOX1)
#define M_DEMO_XBOX
#endif

#if defined(M_DEMO) && defined(PLATFORM_WIN_PC)
#define M_DEMO_PC
#endif

//#define MRTC_NOVIOLENCE
#define MRTC_SOURCE3

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| ...AND THE STANDARD INCLUDES...
|__________________________________________________________________________________________________
\*************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>

#ifdef COMPILER_GNU
	#include <new>
#else
	#include <new.h>
#endif

#ifdef TARGET_DREAMCAST_SHINOBI
	#include <shinobi.h>
	#include <sg_maloc.h>
	
#elif defined TARGET_DOLPHIN
	#include <dolphin.h>
	#include <errno.h>
	#include <cwchar>
	#include "GC/GC_String.h"
	
#elif defined TARGET_PS2

#elif defined TARGET_MACOS

#else
	#include <memory.h>

#endif

#include <math.h>

#ifdef COMPILER_RTTI
	#ifdef COMPILER_GNU
		#include <typeinfo>
	#else
		#include <typeinfo.h>
	#endif
#endif

#ifdef SSE_MEMCPY
	#pragma function(memcpy)
#endif

// -------------------------------------------------------------------
#ifndef pure
	#define pure = 0
#endif


#define DBit(_x) (1 << (_x))

template <mint t_CurrentNumber>
class TCountBits
{
public:
	enum
	{
		ENumBits = 1 + TCountBits< (t_CurrentNumber >> 1) >::ENumBits
	};
};

template<> 
class TCountBits<0>
{
public:
	enum
	{
		ENumBits = 0 
	};
};

#define DNumBits(_Number) (TCountBits<_Number>::ENumBits)

#define DBitRange(_BitStart, _BitEnd) (((uint(1 << (_BitEnd)) - 1) | (1 << (_BitEnd))) ^ (uint(1 << (_BitStart)) - 1))
#define DBitRangeTyped(_BitStart, _BitEnd, _Type) ((((((_Type)1) << (_BitEnd)) - 1) | (((_Type)1) << (_BitEnd))) ^ ((((_Type)1) << (_BitStart)) - 1))

#define M_BITFIELD1(_Type_, _M1, _S1)				\
union _Type_##_M1##_S1								\
{													\
	struct											\
	{												\
		_Type_ _M1 : _S1;							\
		_Type_ _M2 : _S2;							\
	};												\
	_Type_ m_BitUnion;								\
	void SwapLE()									\
	{												\
		enum {o1 = 0, o2 = _S1, o3 = o1 + _S2 };	\
		_Type_ Tmp = m_BitUnion;					\
		::SwapLE(Tmp);								\
		m_BitUnion = 0;								\
		_M1 = (Tmp >> o1) & DBitRange(0, _S1 - 1);	\
		_M2 = (Tmp >> o2) & DBitRange(0, _S2 - 1);	\
	}												\
}

#define M_BITFIELD2(_Type_, _M1, _S1, _M2, _S2)		\
union _Type_##_M1##_S1##_M2##_S2					\
{													\
	struct											\
	{												\
		_Type_ _M1 : _S1;							\
		_Type_ _M2 : _S2;							\
	};												\
	_Type_ m_BitUnion;								\
	void SwapLE()									\
	{												\
		enum {o1 = 0, o2 = _S1, o3 = o1 + _S2 };	\
		_Type_ Tmp = m_BitUnion;					\
		::SwapLE(Tmp);								\
		m_BitUnion = 0;								\
		_M1 = (Tmp >> o1) & DBitRange(0, _S1 - 1);	\
		_M2 = (Tmp >> o2) & DBitRange(0, _S2 - 1);	\
	}												\
}

#define M_BITFIELD3(_Type_, _M1, _S1, _M2, _S2, _M3, _S3)\
union _Type_##_M1##_S1##_M2##_S2##_M3##_S3\
{\
	struct\
	{\
		_Type_ _M1 : _S1;\
		_Type_ _M2 : _S2;\
		_Type_ _M3 : _S3;\
	};\
	_Type_ m_BitUnion;\
	void SwapLE()\
	{\
		enum {o1 = 0, o2 = o1 + _S1, o3 = o2 + _S2, o4 = o3 + _S3};\
		_Type_ Tmp = m_BitUnion;\
		::SwapLE(Tmp);								\
		m_BitUnion = 0;\
		_M1 = (Tmp >> o1) & DBitRange(0, _S1 - 1);\
		_M2 = (Tmp >> o2) & DBitRange(0, _S2 - 1);\
		_M3 = (Tmp >> o3) & DBitRange(0, _S3 - 1);\
	}\
}

#define M_BITFIELD4(_Type_, _M1, _S1, _M2, _S2, _M3, _S3, _M4, _S4)\
union _Type_##_M1##_S1##_M2##_S2##_M3##_S3##_M4##_S4\
{\
	struct\
	{\
		_Type_ _M1 : _S1;\
		_Type_ _M2 : _S2;\
		_Type_ _M3 : _S3;\
		_Type_ _M4 : _S4;\
	};\
	_Type_ m_BitUnion;\
	void SwapLE()\
	{\
		enum {o1 = 0, o2 = o1 + _S1, o3 = o2 + _S2, o4 = o3 + _S3, o5 = o4 + _S4};\
		_Type_ Tmp = m_BitUnion;\
		::SwapLE(Tmp);								\
		m_BitUnion = 0;\
		_M1 = (Tmp >> o1) & DBitRange(0, _S1 - 1);\
		_M2 = (Tmp >> o2) & DBitRange(0, _S2 - 1);\
		_M3 = (Tmp >> o3) & DBitRange(0, _S3 - 1);\
		_M4 = (Tmp >> o4) & DBitRange(0, _S4 - 1);\
	}\
}

#define M_BITFIELD5(_Type_, _M1, _S1, _M2, _S2, _M3, _S3, _M4, _S4, _M5, _S5)\
union _Type_##_M1##_S1##_M2##_S2##_M3##_S3##_M4##_S4##_M5##_S5\
{\
	struct\
	{\
		_Type_ _M1 : _S1;\
		_Type_ _M2 : _S2;\
		_Type_ _M3 : _S3;\
		_Type_ _M4 : _S4;\
		_Type_ _M5 : _S5;\
	};\
	_Type_ m_BitUnion;\
	void SwapLE()\
	{\
		enum {o1 = 0, o2 = o1 + _S1, o3 = o2 + _S2, o4 = o3 + _S3, o5 = o4 + _S4, o6 = o5 + _S5};\
		_Type_ Tmp = m_BitUnion;\
		::SwapLE(Tmp);								\
		m_BitUnion = 0;\
		_M1 = (Tmp >> o1) & DBitRange(0, _S1 - 1);\
		_M2 = (Tmp >> o2) & DBitRange(0, _S2 - 1);\
		_M3 = (Tmp >> o3) & DBitRange(0, _S3 - 1);\
		_M4 = (Tmp >> o4) & DBitRange(0, _S4 - 1);\
		_M5 = (Tmp >> o5) & DBitRange(0, _S5 - 1);\
	}\
}

#define M_BITFIELD6(_Type_, _M1, _S1, _M2, _S2, _M3, _S3, _M4, _S4, _M5, _S5, _M6, _S6)\
union _Type_##_M1##_S1##_M2##_S2##_M3##_S3##_M4##_S4##_M5##_S5##_M6##_S6\
{\
	struct\
	{\
		_Type_ _M1 : _S1;\
		_Type_ _M2 : _S2;\
		_Type_ _M3 : _S3;\
		_Type_ _M4 : _S4;\
		_Type_ _M5 : _S5;\
		_Type_ _M6 : _S6;\
	};\
	_Type_ m_BitUnion;\
	void SwapLE()\
	{\
		enum {o1 = 0, o2 = o1 + _S1, o3 = o2 + _S2, o4 = o3 + _S3, o5 = o4 + _S4, o6 = o5 + _S5, o7 = o6 + _S6};\
		_Type_ Tmp = m_BitUnion;\
		::SwapLE(Tmp);								\
		m_BitUnion = 0;\
		_M1 = (Tmp >> o1) & DBitRange(0, _S1 - 1);\
		_M2 = (Tmp >> o2) & DBitRange(0, _S2 - 1);\
		_M3 = (Tmp >> o3) & DBitRange(0, _S3 - 1);\
		_M4 = (Tmp >> o4) & DBitRange(0, _S4 - 1);\
		_M5 = (Tmp >> o5) & DBitRange(0, _S5 - 1);\
		_M6 = (Tmp >> o6) & DBitRange(0, _S6 - 1);\
	}\
}

#define M_BITFIELD7(_Type_, _M1, _S1, _M2, _S2, _M3, _S3, _M4, _S4, _M5, _S5, _M6, _S6, _M7, _S7)\
union _Type_##_M1##_S1##_M2##_S2##_M3##_S3##_M4##_S4##_M5##_S5##_M6##_S6##_M7##_S7\
{\
	struct\
	{\
		_Type_ _M1 : _S1;\
		_Type_ _M2 : _S2;\
		_Type_ _M3 : _S3;\
		_Type_ _M4 : _S4;\
		_Type_ _M5 : _S5;\
		_Type_ _M6 : _S6;\
		_Type_ _M7 : _S7;\
	};\
	_Type_ m_BitUnion;\
	void SwapLE()\
	{\
		enum {o1 = 0, o2 = o1 + _S1, o3 = o2 + _S2, o4 = o3 + _S3, o5 = o4 + _S4, o6 = o5 + _S5, o7 = o6 + _S6, o8 = o7 + _S7};\
		_Type_ Tmp = m_BitUnion;\
		::SwapLE(Tmp);								\
		m_BitUnion = 0;\
		_M1 = (Tmp >> o1) & DBitRange(0, _S1 - 1);\
		_M2 = (Tmp >> o2) & DBitRange(0, _S2 - 1);\
		_M3 = (Tmp >> o3) & DBitRange(0, _S3 - 1);\
		_M4 = (Tmp >> o4) & DBitRange(0, _S4 - 1);\
		_M5 = (Tmp >> o5) & DBitRange(0, _S5 - 1);\
		_M6 = (Tmp >> o6) & DBitRange(0, _S6 - 1);\
		_M7 = (Tmp >> o7) & DBitRange(0, _S7 - 1);\
	}\
}

#define M_BITFIELD8(_Type_, _M1, _S1, _M2, _S2, _M3, _S3, _M4, _S4, _M5, _S5, _M6, _S6, _M7, _S7, _M8, _S8)\
union _Type_##_M1##_S1##_M2##_S2##_M3##_S3##_M4##_S4##_M5##_S5##_M6##_S6##_M7##_S7##_M8##_S8\
{\
	struct\
	{\
		_Type_ _M1 : _S1;\
		_Type_ _M2 : _S2;\
		_Type_ _M3 : _S3;\
		_Type_ _M4 : _S4;\
		_Type_ _M5 : _S5;\
		_Type_ _M6 : _S6;\
		_Type_ _M7 : _S7;\
		_Type_ _M8 : _S8;\
	};\
	_Type_ m_BitUnion;\
	void SwapLE()\
	{\
		enum {o1 = 0, o2 = o1 + _S1, o3 = o2 + _S2, o4 = o3 + _S3, o5 = o4 + _S4, o6 = o5 + _S5, o7 = o6 + _S6, o8 = o7 + _S7, o9 = o8 + _S8};\
		_Type_ Tmp = m_BitUnion;\
		::SwapLE(Tmp);								\
		m_BitUnion = 0;\
		_M1 = (Tmp >> o1) & DBitRange(0, _S1 - 1);\
		_M2 = (Tmp >> o2) & DBitRange(0, _S2 - 1);\
		_M3 = (Tmp >> o3) & DBitRange(0, _S3 - 1);\
		_M4 = (Tmp >> o4) & DBitRange(0, _S4 - 1);\
		_M5 = (Tmp >> o5) & DBitRange(0, _S5 - 1);\
		_M6 = (Tmp >> o6) & DBitRange(0, _S6 - 1);\
		_M7 = (Tmp >> o7) & DBitRange(0, _S7 - 1);\
		_M8 = (Tmp >> o8) & DBitRange(0, _S8 - 1);\
	}\
}


// typedef long double			fp10;		MSVC maps 80-bit float to 64-bit float (double)

// typedef int					BOOL;		// Should not be used, use bool

#ifndef FALSE
	#define FALSE				0
#endif

#ifndef TRUE
	#define TRUE				(!FALSE)
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| STANDARD CONSTANTS
|__________________________________________________________________________________________________
\*************************************************************************************************/
#define _FP4_MAX			3.402823466e+38F
#define _FP4_MIN			1.175494351e-38F
#define _FP4_EPSILON		1.192092896e-07F	// Minsta positiva tal sådant att 1.0+x != 1.0
#define _FP8_MAX			1.7976931348623158e+307
#define _FP8_MIN			2.2250738585072014e-308
#define _FP8_EPSILON		2.2204460492503131e-016

#if defined(PLATFORM_CONSOLE)
	#define _PI					3.14159265358979323846264338327950288419716939937510582097494459f
	#define _PI2				6.283185307179586476925286766559f
	#define _PIHALF				1.5707963267948966192313216916398f
	#define _NLOG				2.7182818284590f
	#define _SQRT2				1.4142135623731f
	#define _SQRT3				1.7320508075689f
	#define _SIN30				0.5f
	#define _SIN45				0.7071067812f
	#define _SIN60				0.8660254038f
#else
	#define _PI					3.14159265358979323846264338327950288419716939937510582097494459
	#define _PI2				6.283185307179586476925286766559
	#define _PIHALF				1.5707963267948966192313216916398
	#define _NLOG				2.7182818284590
	#define _SQRT2				1.4142135623731
	#define _SQRT3				1.7320508075689
	#define _SIN30				0.5
	#define _SIN45				0.7071067812
	#define _SIN60				0.8660254038
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Dynamic cast support
|__________________________________________________________________________________________________
\*************************************************************************************************/
#ifdef M_FAKEDYNAMICCAST
int gf_FindObject(class MRTC_CRuntimeClass *_pRuntime, const char *_pObject);
template <typename t_CData, typename t_CDataIn>
M_INLINE t_CData *TDynamicCast(t_CDataIn *_pIn)
{
	if (_pIn && gf_FindObject(_pIn->MRTC_GetRuntimeClass(), t_CData::m_RuntimeClass.m_ClassName))
		return (t_CData *)_pIn;
	return NULL;
}
#else
template <typename t_CData, typename t_CDataIn>
M_INLINE t_CData *TDynamicCast(t_CDataIn *_pIn)
{
	return dynamic_cast<t_CData *>(_pIn);
}
#endif
// -------------------------------------------------------------------
template <typename _t0> 
inline _t0 Min(_t0 a, _t0 b)
{
	return ((a < b) ? a : b);
};

template <typename _t0> 
inline _t0 Max(_t0 a, _t0 b)
{
	return ((a > b) ? a : b);
};

#ifdef PLATFORM_XENON
template <>
M_INLINE fp4 Min<fp4>(fp4 a, fp4 b)
{
	return fpmin(a, b);
}

template <>
M_INLINE fp4 Max<fp4>(fp4 a, fp4 b)
{
	return fpmax(a, b);
}

template <>
M_INLINE fp8 Min<fp8>(fp8 a, fp8 b)
{
	return fpmin(a, b);
}

template <>
M_INLINE fp8 Max<fp8>(fp8 a, fp8 b)
{
	return fpmax(a, b);
}
#endif

#ifdef PLATFORM_PS2
template <>
inline fp4 Min<fp4>(fp4 a, fp4 b)
{
	fp4 ret;
	__asm ( "min.s ret, a, b" );
	
	return ret;
}

template <>
inline fp4 Max<fp4>(fp4 a, fp4 b)
{
	fp4 ret;
	__asm ( "max.s ret, a, b" );
	
	return ret;
}
#endif

#ifdef COMPILER_GREENHILL

	double sqrt( int32 a )
	{
		return sqrt(( double )a );
	}
	double sqrt( int64 a )
	{
		return sqrt(( long double )a );
	}

#endif

// -------------------------------------------------------------------
#ifdef PLATFORM_SHINOBI

//	#include <ptmf.h>
	#include "CW_String.h"
	#include "CW_ProcStuff.h"

	// Does not work on virtuals.  :(
	#define MRTC_GetProcAddress(_This, _ClassName, _ReturnType, _ParameterTypes, _Proc, _RetValue) \
	{ \
		_ReturnType(_ClassName::*pF)_ParameterTypes = &_Proc; \
		PTMF* ptmf = (PTMF*)(&pF); \
		if (ptmf->vtbl_offset >= 0) \
		{ \
			long vtbl = *(long*)((_This) + ptmf->func_data.ventry_offset); \
			_RetValue = *(void**)(vtbl + ptmf->vtbl_offset); \
		} else \
			_RetValue = ptmf->func_data.func_addr; \
	}
	
	

#elif defined(PLATFORM_DOLPHIN)

	#define MRTC_GetProcAddress(_This, _ClassName, _ReturnType, _ParameterTypes, _Proc, _RetValue) \
	{ \
		_ReturnType(_ClassName::*pF)_ParameterTypes = _Proc; \
		void** tmp = (void**)&pF; \
		int nOffset = (int)tmp[1]; \
		if (nOffset == -1) \
			_RetValue = tmp[2]; \
		else \
		{ \
			void** pTab = *(void***)_This; \
			_RetValue = pTab[nOffset>>2]; \
		} \
	}

	// returns address of the code for the function '_FuncPtr'
	template <class T>
		void *MRTC_GetProcAddr(void *_pThis, T _FuncPtr)
	{
		void** tmp = (void**)&_FuncPtr;
		int nOffset = (int)tmp[1]; 
		if (nOffset == -1) 
			return tmp[2]; 
		else 
			return (*(void***)_pThis)[nOffset>>2];
	}
		
	// returns index in vtable for this function (returns -1 if it isn't virtual)
	template<class T>
	int MRTC_GetProcIndex(T _FuncPtr)
	{
		return ((int*)&_FuncPtr)[1] >> 2;
	}
	
#elif defined(PLATFORM_PS2) && defined(COMPILER_GNU)
	

	#if	defined( COMPILER_GNU_2)
	#define MRTC_GetProcAddress(_This, _ClassName, _ReturnType, _ParameterTypes, _Proc, _RetValue) \
	{ \
		_ReturnType(_ClassName::*pF)_ParameterTypes = _Proc; \
		void**tmp = (void**)(&pF); \
		short wOffset = *(((short*)tmp)+1);  /*Extract the index of the function in the vtable list*/ \
		if( wOffset == -1 ) \
			*((int*)&_RetValue) = *((int*)(&pF) + 1); /*If index == -1 then it's not a virtual function*/ \
		else \
		{ \
			int nTableOffset = *(((int*)tmp)+1); /*Extract the offset of the pointer to the class from which we inherited this function*/ \
			void **pTab = ((void***)_This)[nTableOffset>>2]; /*Get the pointer to the vtable*/ \
			*((int*)&_RetValue) = (int)pTab[(wOffset<<1)-1]; /*Extract function from vtable*/ \
		} \
	}

	template <class T>
		void *MRTC_GetProcAddr(void *_pThis, T _FuncPtr)
	{
		T pF = _FuncPtr;
		
		void *pRet;
		
		void**tmp = (void**)(&pF); \
			short wOffset = *(((short*)tmp)+1);  /*Extract the index of the function in the vtable list*/ 
		if( wOffset == -1 ) 
			*((int*)&pRet) = *((int*)(&pF) + 1); /*If index == -1 then it's not a virtual function*/ 
		else 
		{ 
			int nTableOffset = *(((int*)tmp)+1); /*Extract the offset of the pointer to the class from which we inherited this function*/ 
			void **pTab = ((void***)_pThis)[nTableOffset>>2]; /*Get the pointer to the vtable*/ 
			*((int*)&pRet) = (int)pTab[(wOffset<<1)-1]; /*Extract function from vtable*/ 
		}
		
		return (void *)pRet;
	}
	#elif defined( COMPILER_GNU_3 )
		#define MRTC_GetProcAddress(_This, _ClassName, _ReturnType, _ParameterTypes, _Proc, _RetValue) \
		{ \
			_ReturnType(_ClassName::*pF)_ParameterTypes = _Proc; \
			register void**tmp = (void**)(&pF); \
			register uint32 wOffset = *(((uint32*)tmp));\
			if( !( wOffset & 1 ) ) \
			{\
				*((int*)&_RetValue) = *((int*)tmp);\
			}\
			else\
			{ \
				register uint32 nTableOffset = ((uint32*)tmp)[1] >> 2;\
				register void **pTab = ((void***)_This)[nTableOffset];\
				*((int*)&_RetValue) = (int)pTab[wOffset>>2];\
			} \
		}

		template <class T>
			void *MRTC_GetProcAddr(void *_pThis, T _FuncPtr)
		{
			T pF = _FuncPtr;
			
			void *pRet;
			
			register void**tmp = (void**)(&pF); 
			register uint32 wOffset = *(((uint32*)tmp));
			if( !( wOffset & 1 ) ) 
			{
				*((int*)&pRet) = *((int*)tmp);
			}
			else
			{ 
				register uint32 nTableOffset = ((uint32*)tmp)[1] >> 2;
				register void **pTab = ((void***)_pThis)[nTableOffset];
				*((int*)&pRet) = (int)pTab[wOffset>>2];
			} 
			
			return (void *)pRet;
		}
	#elif
	#error "for fuck sake"
	#endif

	
#elif defined(PLATFORM_PS2) && defined(COMPILER_CODEWARRIOR)
	
	#define MRTC_GetProcAddress(_This, _ClassName, _ReturnType, _ParameterTypes, _Proc, _RetValue) \
	{ \
		_ReturnType(_ClassName::*pF)_ParameterTypes = _Proc; \
		void** tmp = (void**)&pF; \
		int nOffset = (int)tmp[1]; \
		if (nOffset == -1) \
			_RetValue = tmp[2]; \
		else \
		{ \
			void** pTab = *(void***)_This; \
			_RetValue = pTab[nOffset>>2]; \
		} \
	}

	template <class T>
		void *MRTC_GetProcAddr(void *_pThis, T _FuncPtr)
	{
		T pF = _FuncPtr;
		
		void *pRet;
		void** tmp = (void**)&pF;
		int nOffset = (int)tmp[1]; 
		if (nOffset == -1) 
			pRet = tmp[2]; 
		else 
		{ 
			void** pTab = *(void***)_pThis;
			pRet = pTab[nOffset>>2]; 
		} 
		
		return (void *)pRet;
	}
	
#elif defined(PLATFORM_MACOS)

	#define MRTC_GetProcAddress(_This, _ClassName, _ReturnType, _ParameterTypes, _Proc, _RetValue) \
	{ \
		_ReturnType(_ClassName::*pF)_ParameterTypes = _Proc; \
		void** tmp = (void**)&pF; \
		int nOffset = (int)tmp[1]; \
		if (nOffset == -1) \
			_RetValue = tmp[2]; \
		else \
		{ \
			void** pTab = *(void***)_This; \
			_RetValue = pTab[nOffset>>2]; \
		} \
	}

	// returns address of the code for the function '_FuncPtr'
	template <class T>
		void *MRTC_GetProcAddr(void *_pThis, T _FuncPtr)
	{
		void** tmp = (void**)&_FuncPtr;
		int nOffset = (int)tmp[1]; 
		if (nOffset == -1) 
			return tmp[2]; 
		else 
			return (*(void***)_pThis)[nOffset>>2];
	}

	// returns index in vtable for this function (returns -1 if it isn't virtual)
	template<class T>
	int MRTC_GetProcIndex(T _FuncPtr)
	{
		return ((int*)&_FuncPtr)[1] >> 2;
	}

#else
	
	template <class T>
		void *MRTC_GetProcAddr(void *_pThis, T _FuncPtr)
	{
		T pF = _FuncPtr;
		void *pRet;
		*((mint*)&pRet) = *((mint*)(&pF));
		return (void *)pRet;
	}
	

	#define MRTC_GetProcAddress(_This, _ClassName, _ReturnType, _ParameterTypes, _Proc, _RetValue) \
	{ \
		_ReturnType(_ClassName::*pF)_ParameterTypes = &_ClassName::_Proc; \
		*((mint*)&_RetValue) = *((mint*)(&pF)); \
	}

#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Memory management
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Template:			TAlignmentOf

	Comments:			M_ALIGNMENTOF(Type) is the compile time constant
						memory alignment requirement for objects of type 'Type'.

						ex:
							M_ALIGNMENTOF(int8) = 00000001
							M_ALIGNMENTOF(int16) = 00000002
							M_ALIGNMENTOF(int32) = 00000004
							M_ALIGNMENTOF(int64) = 00000008
							M_ALIGNMENTOF(fp8) = 00000008
							M_ALIGNMENTOF(vec128) = 00000010
							M_ALIGNMENTOF(CVec3Dfp4) = 00000004
							M_ALIGNMENTOF(CVec4Dfp4) = 00000010
							M_ALIGNMENTOF(CMat4Dfp4) = 00000010
\*____________________________________________________________________*/
#ifdef COMPILER_MSVC4
#pragma warning(push)
#pragma warning(disable:4624)
#endif
template<class T>
class TAlignmentOf
{
public:
	struct CAlign
	{
		uint8 m_Dummy;
		T m_T;
	};

	enum
	{
		ALIGNMENT = sizeof(CAlign) - sizeof(T),
	};
};
#ifdef COMPILER_MSVC4
#pragma warning(pop)
#endif

#define M_ALIGNMENTOF(Type) TAlignmentOf< Type >::ALIGNMENT


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			MRTC_MemAvail
						
	Returns:			Available memory or zero.
						
	Comments:			This function may or may not return available
						memory depending on the target platform. Should 
						only be used for memory consumption monitoring.
						No memory management hueristics should be based
						on the return value of this function.
\*____________________________________________________________________*/
int MRTC_MemAvail();
int MRTC_MemUsed();
int MRTC_MemDelta();

#define M_DEFAULTALIGNMENT 4

void MRTC_Assert(const char* _pMsg, const char* _pFile, int _Line);
void* MRTC_MemAlloc(size_t _nSize, size_t _Align);
void* MRTC_MemRealloc(void* _pMem, size_t _nSize, size_t _Align);
void MRTC_MemFree(void* _pMem);
size_t MRTC_MemSize(void* _pMem);

#ifdef M_SUPPORTMEMORYDEBUG
	void* MRTC_MemAlloc(size_t _nSize, size_t _Align, const char* _File, int _Line);
	void* MRTC_MemRealloc(void* _pMem, size_t _nSize, size_t _Align, const char* _File, int _Line);
#endif
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| new / delete
|__________________________________________________________________________________________________
\*************************************************************************************************/
#ifdef M_SUPPORTMEMORYDEBUG
	#define M_ALLOCDEBUG(_nSize, _File, _Line) MRTC_MemAlloc(_nSize, M_DEFAULTALIGNMENT, _File, _Line)
	#define M_REALLOCDEBUG(_pMem, _nSize, _File, _Line) MRTC_MemRealloc(_pMem, _nSize, M_DEFAULTALIGNMENT, _File, _Line)
	#define M_ALLOC(_nSize) MRTC_MemAlloc(_nSize, M_DEFAULTALIGNMENT, __FILE__, __LINE__)
	#define M_REALLOC(_pMem, _nSize) MRTC_MemRealloc(_pMem, _nSize, M_DEFAULTALIGNMENT, __FILE__, __LINE__)

	#define M_ALLOCDEBUGALIGN(_nSize, _Align, _File, _Line) MRTC_MemAlloc(_nSize, _Align, _File, _Line)
	#define M_REALLOCDEBUGALIGN(_pMem, _nSize, _Align, _File, _Line) MRTC_MemRealloc(_pMem, _nSize, _Align, _File, _Line)
	#define M_ALLOCALIGN(_nSize, _Align) MRTC_MemAlloc(_nSize, _Align, __FILE__, __LINE__)
	#define M_REALLOCALIGN(_pMem, _nSize, _Align) MRTC_MemRealloc(_pMem, _nSize, _Align, __FILE__, __LINE__)

	#define M_MEMALLOC(MemMgr, nSize) MemMgr->AllocDebug(nSize, _NORMAL_BLOCK, __FILE__, __LINE__ )
	#define M_MEMREALLOC(MemMgr, _pMem, _nSize) MemMgr->ReallocDebug(_pMem, _nSize, 1, __FILE__, __LINE__)
#else
	#define M_ALLOCDEBUG(_nSize, _File, _Line) MRTC_MemAlloc(_nSize, M_DEFAULTALIGNMENT)
	#define M_REALLOCDEBUG(_pMem, _nSize, _File, _Line) MRTC_MemRealloc(_pMem, _nSize, M_DEFAULTALIGNMENT)
	#define M_ALLOC(_nSize) MRTC_MemAlloc(_nSize, M_DEFAULTALIGNMENT)
	#define M_REALLOC(_pMem, _nSize) MRTC_MemRealloc(_pMem, _nSize, M_DEFAULTALIGNMENT)
	
	#define M_ALLOCDEBUGALIGN(_nSize, _Align, _File, _Line) MRTC_MemAlloc(_nSize, _Align)
	#define M_REALLOCDEBUGALIGN(_pMem, _nSize, _Align, _File, _Line) MRTC_MemRealloc(_pMem, _nSize, _Align)
	#define M_ALLOCALIGN(_nSize, _Align) MRTC_MemAlloc(_nSize, _Align)
	#define M_REALLOCALIGN(_pMem, _nSize, _Align) MRTC_MemRealloc(_pMem, _nSize, _Align)


	#define M_MEMALLOC(MemMgr, nSize) MemMgr->Alloc(nSize)
	#define M_MEMREALLOC(MemMgr, _pMem, _nSize) MemMgr->Realloc(_pMem, _nSize)

#endif

#ifdef COMPILER_GNU
	void* M_CDECL operator new[](size_t _nSize) throw(std::bad_alloc);
#elif defined(COMPILER_GNU_3)
	void* M_CDECL operator new[](size_t _nSize);
#else
	void* M_CDECL operator new[](mint _nSize);
#endif

#ifdef COMPILER_NEEDOPERATORDELETE
	#ifdef COMPILER_GNU
		void M_CDECL operator delete[](void *) throw();
	#elif defined(COMPILER_GNU_3)
		void M_CDECL operator delete[](void *);
	#else
		void M_CDECL operator delete[] (void * ptr);
	#endif
#endif
/*
#ifdef MRTC_MEMORYDEBUG
	void* M_CDECL operator new(mint _nSize, const char* _pFn, const char* _pFile, int _Line) throw(...);
	#ifdef COMPILER_NEEDOPERATORDELETE
		void M_CDECL operator delete(void* p, const char* _pFn, const char* _pFile, int _Line) throw();
	#endif
#endif
*/
#if 0
#ifdef _DEBUG

	#ifdef PLATFORM_WIN
		#include "crtdbg.h"
	#endif

	void* M_CDECL operator new(mint _nSize, const char* _pFileName, int _Line);
	#ifdef COMPILER_NEEDOPERATORDELETE
		void M_CDECL operator delete(void* p, const char* _pFileName, int _Line);
	#endif

	void* operator new[](mint _nSize, const char* _pFileName, int _Line);

	#define MRTC_DEBUG_NEW new(__FILE__, __LINE__)

	#ifdef	COMPILER_GNU
		#define DECLARE_OPERATOR_NEW \
			void* operator new(mint _Size, const char* _pFileName, int _Line);\
			void* operator new(mint _Size);	\
			void* operator new(mint, void*);

		#define IMPLEMENT_OPERATOR_NEW(Class) \
			void* Class::operator new(mint _Size, const char* _pFileName, int _Line) { return CReferenceCount::operator new(_Size, _pFileName, _Line); };\
			void* Class::operator new(mint _Size) { return CReferenceCount::operator new(_Size); };\
			void* Class::operator new(mint _Size, void* _pAddr) { return ::operator new(_Size, _pAddr); };
	#else	// COMPILER_GNU
		#define DECLARE_OPERATOR_NEW \
			void* operator new(mint _Size, const char* _pFileName, int _Line);\
			void* operator new(mint _Size);	\
			void* operator new(mint, void*);\
			void operator delete(void *);\
			void operator delete(void *, const char* _pFileName, int _Line);\
			void operator delete(void *, void*);

		#define IMPLEMENT_OPERATOR_NEW(Class) \
			void* Class::operator new(mint _Size, const char* _pFileName, int _Line) { return CReferenceCount::operator new(_Size, _pFileName, _Line); };\
			void* Class::operator new(mint _Size) { return CReferenceCount::operator new(_Size); };\
			void* Class::operator new(mint _Size, void* _pAddr) { return ::operator new(_Size, _pAddr); };\
			void Class::operator delete(void *p) { CReferenceCount::operator delete(p); };\
			void Class::operator delete(void *p, const char* _pFileName, int _Line) { CReferenceCount::operator delete(p); };\
			void Class::operator delete(void *, void* _pAddr) { };
	#endif	// COMPILER_GNU

#else

	#ifdef MRTC_MEMORYDEBUG

		void* M_CDECL operator new(mint _nSize, const char* _pFn, const char* _pFile, int _Line) throw(...);
		#ifdef COMPILER_NEEDOPERATORDELETE
			void M_CDECL operator delete(void* p, const char* _pFn, const char* _pFile, int _Line) throw();
		#endif

		void* M_CDECL operator new(mint _nSize) throw(...);
		void M_CDECL operator delete(void* p) throw();


		#define DECLARE_OPERATOR_NEW \
		void* operator new(mint _Size, const char* _pFn, const char* _pFile, int _Line);\
		void* operator new(mint _Size);\
		void* operator new(mint, void*);
		
		#define IMPLEMENT_OPERATOR_NEW(Class) \
			void* Class::operator new(mint _Size, const char* _pFn, const char* _pFile, int _Line) \
			{ return CReferenceCount::operator new(_Size, _pFn, _pFile, _Line); };\
			void* Class::operator new(mint _Size) \
			{ return CReferenceCount::operator new(_Size, "?", "?", 0); };\
			void* Class::operator new(mint _Size, void* _pAddr) { return ::operator new(_Size, _pAddr); };

		#define MRTC_DEBUG_NEW new("?", __FILE__, __LINE__)
//		#define MRTC_DEBUG_NEW new

	#else

		#ifdef M_RTM

			#ifdef	COMPILER_GNU
				#define DECLARE_OPERATOR_NEW \
					void* operator new(mint _Size);\
					void* operator new(mint, void*);
			
				#define IMPLEMENT_OPERATOR_NEW(Class) \
					void* Class::operator new(mint _Size) { return CReferenceCount::operator new(_Size); };\
					void* Class::operator new(mint _Size, void* _pAddr) { return ::operator new(_Size, _pAddr); };
			#else	// COMPILER_GNU
				#ifdef PLATFORM_WIN_PC
					#define DECLARE_OPERATOR_NEW \
						void* operator new(mint _Size);\
						void* operator new(mint, void*);\
						void operator delete(void *);\
						void operator delete(void *, void*);\
						void operator delete(void *, const char* _pFileName, int _Line);\
						void* operator new(mint _Size, const char* _pFileName, int _Line);
				#else
					#define DECLARE_OPERATOR_NEW \
						void* operator new(mint _Size);\
						void* operator new(mint, void*);\
						void operator delete(void *);\
						void operator delete(void *, void*);
				#endif
			
				#ifdef PLATFORM_WIN_PC
					#define IMPLEMENT_OPERATOR_NEW(Class) \
						void* Class::operator new(mint _Size) { return CReferenceCount::operator new(_Size); };\
						void* Class::operator new(mint _Size, void* _pAddr) { return ::operator new(_Size, _pAddr); };\
						void Class::operator delete(void *p) { CReferenceCount::operator delete(p); };\
						void Class::operator delete(void *, void* _pAddr) { };\
						void* Class::operator new(mint _Size, const char* _pFileName, int _Line) { return CReferenceCount::operator new(_Size, _pFileName, _Line); };\
						void Class::operator delete(void *p, const char* _pFileName, int _Line) { CReferenceCount::operator delete(p); };
				#else
					#define IMPLEMENT_OPERATOR_NEW(Class) \
						void* Class::operator new(mint _Size) { return CReferenceCount::operator new(_Size); };\
						void* Class::operator new(mint _Size, void* _pAddr) { return ::operator new(_Size, _pAddr); };\
						void Class::operator delete(void *p) { CReferenceCount::operator delete(p); };\
						void Class::operator delete(void *, void* _pAddr) { };
				#endif
			#endif	// COMPILER_GNU
			#define MRTC_DEBUG_NEW new
		#else

			#ifdef	COMPILER_GNU
				#define DECLARE_OPERATOR_NEW \
					void* operator new(mint _Size);\
					void* operator new(mint _Size, const char* _pFileName, int _Line);\
					void* operator new(mint, void*);
	
				#define IMPLEMENT_OPERATOR_NEW(Class) \
					void* Class::operator new(mint _Size) { return CReferenceCount::operator new(_Size); };\
					void* Class::operator new(mint _Size, const char* _pFileName, int _Line) { return CReferenceCount::operator new(_Size); };\
					void* Class::operator new(mint _Size, void* _pAddr) { return ::operator new(_Size, _pAddr); };
			#else	// COMPILER_GNU
				#define DECLARE_OPERATOR_NEW \
					void* operator new(mint _Size);\
					void* operator new(mint _Size, const char* _pFileName, int _Line);\
					void* operator new(mint, void*);\
					void operator delete(void *);\
					void operator delete(void *, const char* _pFileName, int _Line);\
					void operator delete(void *, void*);
	
				#define IMPLEMENT_OPERATOR_NEW(Class) \
					void* Class::operator new(mint _Size) { return CReferenceCount::operator new(_Size); };\
					void* Class::operator new(mint _Size, const char* _pFileName, int _Line) { return CReferenceCount::operator new(_Size); };\
					void* Class::operator new(mint _Size, void* _pAddr) { return ::operator new(_Size, _pAddr); };\
					void Class::operator delete(void *p) { CReferenceCount::operator delete(p); };\
					void Class::operator delete(void *p, const char* _pFileName, int _Line) { CReferenceCount::operator delete(p); };\
					void Class::operator delete(void *, void* _pAddr) { };
			#endif	// COMPILER_GNU
			#define MRTC_DEBUG_NEW new

		#endif
	#endif

#endif
#else
#define DECLARE_OPERATOR_NEW 
#define IMPLEMENT_OPERATOR_NEW(Class)

class CAlignmentNewDummy
{
public:
};
#ifdef _DEBUG


#	ifdef PLATFORM_WIN
#		include "crtdbg.h"
#	endif

	void* M_CDECL operator new(mint _nSize, const char* _pFileName, int _Line);
	#ifdef COMPILER_NEEDOPERATORDELETE
		void M_CDECL operator delete(void* p, const char* _pFileName, int _Line);
	#endif

	void* operator new[](mint _nSize, const char* _pFileName, int _Line);


	void* M_CDECL operator new(mint _nSize, mint _Alignment, const char* _pFileName, int _Line, CAlignmentNewDummy *_pDummy);
	#ifdef COMPILER_NEEDOPERATORDELETE
		void M_CDECL operator delete(void* p, mint _Alignment, const char* _pFileName, int _Line, CAlignmentNewDummy *_pDummy);
	#endif

	void* operator new[](mint _nSize, mint _Alignment, const char* _pFileName, int _Line, CAlignmentNewDummy *_pDummy);
	#ifdef COMPILER_NEEDOPERATORDELETE
		void M_CDECL operator delete[](void* p, mint _Alignment, const char* _pFileName, int _Line, CAlignmentNewDummy *_pDummy);
	#endif


	#define MRTC_DEBUG_NEW(_Class) new(M_ALIGNMENTOF( _Class ), __FILE__, __LINE__, (CAlignmentNewDummy *)NULL)
	#define MRTC_DEBUG_NEW2(_Class)  new(__FILE__, __LINE__)

#else

	#ifdef MRTC_MEMORYDEBUG

		void* M_CDECL operator new(mint _nSize, mint _Alignment, const char* _pFile, int _Line, CAlignmentNewDummy *_pDummy = NULL) throw();
		#ifdef COMPILER_NEEDOPERATORDELETE
			void M_CDECL operator delete(void* p, mint _Alignment, const char* _pFile, int _Line, CAlignmentNewDummy *_pDummy = NULL) throw();
		#endif

			void* M_CDECL operator new[](mint _nSize, mint _Alignment, const char* _pFile, int _Line, CAlignmentNewDummy *_pDummy = NULL) throw();
		#ifdef COMPILER_NEEDOPERATORDELETE
			void M_CDECL operator delete[](void* p, mint _Alignment, const char* _pFile, int _Line, CAlignmentNewDummy *_pDummy = NULL) throw();
		#endif

		void* M_CDECL operator new(mint _nSize) throw();
		void M_CDECL operator delete(void* p) throw();

		void* M_CDECL operator new(mint _nSize, mint _Alignment, CAlignmentNewDummy *_pDummy) throw();
		#ifdef COMPILER_NEEDOPERATORDELETE
			void M_CDECL operator delete(void* p, mint _Alignment, CAlignmentNewDummy *_pDummy) throw();
		#endif

		void* M_CDECL operator new[](mint _nSize, mint _Alignment, CAlignmentNewDummy *_pDummy) throw();
		#ifdef COMPILER_NEEDOPERATORDELETE
			void M_CDECL operator delete[](void* p, mint _Alignment, CAlignmentNewDummy *_pDummy) throw();
		#endif

		#define MRTC_DEBUG_NEW(_Class) new(M_ALIGNMENTOF( _Class ), __FILE__, __LINE__, (CAlignmentNewDummy *)NULL)
		#define MRTC_DEBUG_NEW2(_Class) new(M_ALIGNMENTOF( _Class ), __FILE__, __LINE__, (CAlignmentNewDummy *)NULL)
	#else

		void* M_CDECL operator new(mint _nSize, mint _Alignment, CAlignmentNewDummy *_pDummy) throw();
		#ifdef COMPILER_NEEDOPERATORDELETE
			void M_CDECL operator delete(void* p, mint _Alignment, CAlignmentNewDummy *_pDummy) throw();
		#endif

		void* M_CDECL operator new[](mint _nSize, mint _Alignment, CAlignmentNewDummy *_pDummy) throw();
		#ifdef COMPILER_NEEDOPERATORDELETE
			void M_CDECL operator delete[](void* p, mint _Alignment, CAlignmentNewDummy *_pDummy) throw();
		#endif


		#ifdef M_RTM

			#define MRTC_DEBUG_NEW(_Class) new(M_ALIGNMENTOF( _Class ), (CAlignmentNewDummy *)NULL)
			#define MRTC_DEBUG_NEW2(_Class) new
		#else
			#define MRTC_DEBUG_NEW(_Class) new(M_ALIGNMENTOF( _Class ), (CAlignmentNewDummy *)NULL)
			#define MRTC_DEBUG_NEW2(_Class) new
		#endif
	#endif

#endif
#endif

#define DNew(_Class) MRTC_DEBUG_NEW(_Class)
#define DNew2(_Class) MRTC_DEBUG_NEW2(_Class)
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Placement new / delete
|__________________________________________________________________________________________________
\*************************************************************************************************/
#ifndef COMPILER_PLACEMENT_NEW
#ifndef __PLACEMENT_NEW_INLINE
//#define __PLACEMENT_NEW_INLINE

	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Function:			placement new operator
	\*____________________________________________________________________*/
#ifdef	COMPILER_GNU
	M_INLINE void* operator new(mint, void*) throw();
#else
	void* operator new(mint, void*);
#endif

	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Function:			placement delete operator
	\*____________________________________________________________________*/
#ifdef	COMPILER_GNU
	M_INLINE void operator delete(void*, void*);
#else
	void operator delete(void*, void*);
#endif

#endif
#endif

#if !defined(M_RTM) || defined(MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCLASSES)
template <typename t_CData>
t_CData *TNew(const char *_pClassName, const char* _pFileName, int _Line)
{
	void* pMem = M_ALLOCDEBUGALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData), _pFileName, _Line);
	t_CData *pNew = new(pMem) t_CData;
	if (pNew)
	{
		pNew->PostConstruct(_pClassName);
	}
	return pNew;
}

template <typename t_CData, typename t_CParam0>
t_CData *TNew(const char *_pClassName, const char* _pFileName, int _Line, t_CParam0 _Param0)
{
	void* pMem = M_ALLOCDEBUGALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData), _pFileName, _Line);
	t_CData *pNew = new(pMem) t_CData(_Param0);
	if (pNew)
	{
		pNew->PostConstruct(_pClassName);
	}
	return pNew;
}

template <typename t_CData, typename t_CParam0, typename t_CParam1>
t_CData *TNew(const char *_pClassName, const char* _pFileName, int _Line, t_CParam0 _Param0, t_CParam1 _Param1)
{
	void* pMem = M_ALLOCDEBUGALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData), _pFileName, _Line);
	t_CData *pNew = new(pMem) t_CData(_Param0, _Param1);
	if (pNew)
	{
		pNew->PostConstruct(_pClassName);
	}
	return pNew;
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2>
t_CData *TNew(const char *_pClassName, const char* _pFileName, int _Line, t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2)
{
	void* pMem = M_ALLOCDEBUGALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData), _pFileName, _Line);
	t_CData *pNew = new(pMem) t_CData(_Param0, _Param1, _Param2);
	if (pNew)
	{
		pNew->PostConstruct(_pClassName);
	}
	return pNew;
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3>
t_CData *TNew(const char *_pClassName, const char* _pFileName, int _Line, t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3)
{
	void* pMem = M_ALLOCDEBUGALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData), _pFileName, _Line);
	t_CData *pNew = new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3);
	if (pNew)
	{
		pNew->PostConstruct(_pClassName);
	}
	return pNew;
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4>
t_CData *TNew(const char *_pClassName, const char* _pFileName, int _Line, t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4)
{
	void* pMem = M_ALLOCDEBUGALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData), _pFileName, _Line);
	t_CData *pNew = new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4);
	if (pNew)
	{
		pNew->PostConstruct(_pClassName);
	}
	return pNew;
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5>
t_CData *TNew(const char *_pClassName, const char* _pFileName, int _Line, t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5)
{
	void* pMem = M_ALLOCDEBUGALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData), _pFileName, _Line);
	t_CData *pNew = new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5);
	if (pNew)
	{
		pNew->PostConstruct(_pClassName);
	}
	return pNew;
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5, typename t_CParam6>
t_CData *TNew(const char *_pClassName, const char* _pFileName, int _Line, t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5, t_CParam6  _Param6)
{
	void* pMem = M_ALLOCDEBUGALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData), _pFileName, _Line);
	t_CData *pNew = new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6);
	if (pNew)
	{
		pNew->PostConstruct(_pClassName);
	}
	return pNew;
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5, typename t_CParam6, typename t_CParam7>
t_CData *TNew(const char *_pClassName, const char* _pFileName, int _Line, t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5, t_CParam6 _Param6, t_CParam7 _Param7)
{
	void* pMem = M_ALLOCDEBUGALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData), _pFileName, _Line);
	t_CData *pNew = new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7);
	if (pNew)
	{
		pNew->PostConstruct(_pClassName);
	}
	return pNew;
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5, typename t_CParam6, typename t_CParam7, typename t_CParam8>
t_CData *TNew(const char *_pClassName, const char* _pFileName, int _Line, t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5, t_CParam6 _Param6, t_CParam7 _Param7, t_CParam8 _Param8)
{
	void* pMem = M_ALLOCDEBUGALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData), _pFileName, _Line);
	t_CData *pNew = new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8);
	if (pNew)
	{
		pNew->PostConstruct(_pClassName);
	}
	return pNew;
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5, typename t_CParam6, typename t_CParam7, typename t_CParam8, typename t_CParam9>
t_CData *TNew(const char *_pClassName, const char* _pFileName, int _Line, t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5, t_CParam6 _Param6, t_CParam7 _Param7, t_CParam8 _Param8, t_CParam9 _Param9)
{
	void* pMem = M_ALLOCDEBUGALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData), _pFileName, _Line);
	t_CData *pNew = new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9);
	if (pNew)
	{
		pNew->PostConstruct(_pClassName);
	}
	return pNew;
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5, typename t_CParam6, typename t_CParam7, typename t_CParam8, typename t_CParam9, typename t_CParam10>
t_CData *TNew(const char *_pClassName, const char* _pFileName, int _Line, t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5, t_CParam6 _Param6, t_CParam7 _Param7, t_CParam8 _Param8, t_CParam9 _Param9, t_CParam10 _Param10)
{
	void* pMem = M_ALLOCDEBUGALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData), _pFileName, _Line);
	t_CData *pNew = new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10);
	if (pNew)
	{
		pNew->PostConstruct(_pClassName);
	}
	return pNew;
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5, typename t_CParam6, typename t_CParam7, typename t_CParam8, typename t_CParam9, typename t_CParam10, typename t_CParam11>
t_CData *TNew(const char *_pClassName, const char* _pFileName, int _Line, t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5, t_CParam6 _Param6, t_CParam7 _Param7, t_CParam8 _Param8, t_CParam9 _Param9, t_CParam10 _Param10, t_CParam11 _Param11)
{
	void* pMem = M_ALLOCDEBUGALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData), _pFileName, _Line);
	t_CData *pNew = new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11);
	if (pNew)
	{
		pNew->PostConstruct(_pClassName);
	}
	return pNew;
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5, typename t_CParam6, typename t_CParam7, typename t_CParam8, typename t_CParam9, typename t_CParam10, typename t_CParam11, typename t_CParam12>
t_CData *TNew(const char *_pClassName, const char* _pFileName, int _Line, t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5, t_CParam6 _Param6, t_CParam7 _Param7, t_CParam8 _Param8, t_CParam9 _Param9, t_CParam10 _Param10, t_CParam11 _Param11, t_CParam12 _Param12)
{
	void* pMem = M_ALLOCDEBUGALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData), _pFileName, _Line);
	t_CData *pNew = new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12);
	if (pNew)
	{
		pNew->PostConstruct(_pClassName);
	}
	return pNew;
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5, typename t_CParam6, typename t_CParam7, typename t_CParam8, typename t_CParam9, typename t_CParam10, typename t_CParam11, typename t_CParam12, typename t_CParam13>
t_CData *TNew(const char *_pClassName, const char* _pFileName, int _Line, t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5, t_CParam6 _Param6, t_CParam7 _Param7, t_CParam8 _Param8, t_CParam9 _Param9, t_CParam10 _Param10, t_CParam11 _Param11, t_CParam12 _Param12, t_CParam13 _Param13)
{
	void* pMem = M_ALLOCDEBUGALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData), _pFileName, _Line);
	t_CData *pNew = new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12, _Param13);
	if (pNew)
	{
		pNew->PostConstruct(_pClassName);
	}
	return pNew;
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5, typename t_CParam6, typename t_CParam7, typename t_CParam8, typename t_CParam9, typename t_CParam10, typename t_CParam11, typename t_CParam12, typename t_CParam13, typename t_CParam14>
t_CData *TNew(const char *_pClassName, const char* _pFileName, int _Line, t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5, t_CParam6 _Param6, t_CParam7 _Param7, t_CParam8 _Param8, t_CParam9 _Param9, t_CParam10 _Param10, t_CParam11 _Param11, t_CParam12 _Param12, t_CParam13 _Param13, t_CParam14 _Param14)
{
	void* pMem = M_ALLOCDEBUGALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData), _pFileName, _Line);
	t_CData *pNew = new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12, _Param13, _Param14);
	if (pNew)
	{
		pNew->PostConstruct(_pClassName);
	}
	return pNew;
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5, typename t_CParam6, typename t_CParam7, typename t_CParam8, typename t_CParam9, typename t_CParam10, typename t_CParam11, typename t_CParam12, typename t_CParam13, typename t_CParam14, typename t_CParam15>
t_CData *TNew(const char *_pClassName, const char* _pFileName, int _Line, t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5, t_CParam6 _Param6, t_CParam7 _Param7, t_CParam8 _Param8, t_CParam9 _Param9, t_CParam10 _Param10, t_CParam11 _Param11, t_CParam12 _Param12, t_CParam13 _Param13, t_CParam14 _Param14, t_CParam15 _Param15)
{
	void* pMem = M_ALLOCDEBUGALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData), _pFileName, _Line);
	t_CData *pNew = new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12, _Param13, _Param14, _Param15);
	if (pNew)
	{
		pNew->PostConstruct(_pClassName);
	}
	return pNew;
}

#define MNew(_ClassName) TNew< _ClassName >(#_ClassName, __FILE__, __LINE__)
#define MNew1(_ClassName, _Param0) TNew< _ClassName >(#_ClassName, __FILE__, __LINE__, _Param0)
#define MNew2(_ClassName, _Param0, _Param1) TNew< _ClassName >(#_ClassName, __FILE__, __LINE__, _Param0, _Param1)
#define MNew3(_ClassName, _Param0, _Param1, _Param2) TNew< _ClassName >(#_ClassName, __FILE__, __LINE__, _Param0, _Param1, _Param2)
#define MNew4(_ClassName, _Param0, _Param1, _Param2, _Param3) TNew< _ClassName >(#_ClassName, __FILE__, __LINE__, _Param0, _Param1, _Param2, _Param3)
#define MNew5(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4) TNew< _ClassName >(#_ClassName, __FILE__, __LINE__, _Param0, _Param1, _Param2, _Param3, _Param4)
#define MNew6(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5) TNew< _ClassName >(#_ClassName, __FILE__, __LINE__, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5)
#define MNew7(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6) TNew< _ClassName >(#_ClassName, __FILE__, __LINE__, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6)
#define MNew8(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7) TNew< _ClassName >(#_ClassName, __FILE__, __LINE__, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7)
#define MNew9(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8) TNew< _ClassName >(#_ClassName, __FILE__, __LINE__, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8)
#define MNew10(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9) TNew< _ClassName >(#_ClassName, __FILE__, __LINE__, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9)
#define MNew11(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10) TNew< _ClassName >(#_ClassName, __FILE__, __LINE__, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10)
#define MNew12(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11) TNew< _ClassName >(#_ClassName, __FILE__, __LINE__, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11)
#define MNew13(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12) TNew< _ClassName >(#_ClassName, __FILE__, __LINE__, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12)
#define MNew14(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12, _Param13) TNew< _ClassName >(#_ClassName, __FILE__, __LINE__, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12, _Param13)
#define MNew15(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12, _Param13, _Param14) TNew< _ClassName >(#_ClassName, __FILE__, __LINE__, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12, _Param13, _Param14)
#define MNew16(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12, _Param13, _Param14, _Param15) TNew< _ClassName >(#_ClassName, __FILE__, __LINE__, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12, _Param13, _Param14, _Param15)

#else
template <typename t_CData>
M_INLINE t_CData *TNew()
{
	void* pMem = M_ALLOCALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData));
	return new(pMem) t_CData;
}

template <typename t_CData, typename t_CParam0>
M_INLINE t_CData *TNew(t_CParam0 _Param0)
{
	void* pMem = M_ALLOCALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData));
	return new(pMem) t_CData(_Param0);
}

template <typename t_CData, typename t_CParam0, typename t_CParam1>
M_INLINE t_CData *TNew(t_CParam0 _Param0, t_CParam1 _Param1)
{
	void* pMem = M_ALLOCALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData));
	return new(pMem) t_CData(_Param0, _Param1);
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2>
M_INLINE t_CData *TNew(t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2)
{
	void* pMem = M_ALLOCALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData));
	return new(pMem) t_CData(_Param0, _Param1, _Param2);
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3>
M_INLINE t_CData *TNew(t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3)
{
	void* pMem = M_ALLOCALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData));
	return new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3);
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4>
M_INLINE t_CData *TNew(t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4)
{
	void* pMem = M_ALLOCALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData));
	return new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4);
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5>
M_INLINE t_CData *TNew(t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5)
{
	void* pMem = M_ALLOCALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData));
	return new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5);
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5, typename t_CParam6>
M_INLINE t_CData *TNew(t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5, t_CParam6 _Param6)
{
	void* pMem = M_ALLOCALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData));
	return new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6);
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5, typename t_CParam6, typename t_CParam7>
M_INLINE t_CData *TNew(t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5, t_CParam6 _Param6, t_CParam7 _Param7)
{
	void* pMem = M_ALLOCALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData));
	return new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7);
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5, typename t_CParam6, typename t_CParam7, typename t_CParam8>
M_INLINE t_CData *TNew(t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5, t_CParam6 _Param6, t_CParam7 _Param7, t_CParam8 _Param8)
{
	void* pMem = M_ALLOCALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData));
	return new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8);
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5, typename t_CParam6, typename t_CParam7, typename t_CParam8, typename t_CParam9>
M_INLINE t_CData *TNew(t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5, t_CParam6 _Param6, t_CParam7 _Param7, t_CParam8 _Param8, t_CParam9 _Param9)
{
	void* pMem = M_ALLOCALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData));
	return new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9);
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5, typename t_CParam6, typename t_CParam7, typename t_CParam8, typename t_CParam9, typename t_CParam10>
M_INLINE t_CData *TNew(t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5, t_CParam6 _Param6, t_CParam7 _Param7, t_CParam8 _Param8, t_CParam9 _Param9, t_CParam10 _Param10)
{
	void* pMem = M_ALLOCALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData));
	return new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10);
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5, typename t_CParam6, typename t_CParam7, typename t_CParam8, typename t_CParam9, typename t_CParam10, typename t_CParam11>
M_INLINE t_CData *TNew(t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5, t_CParam6 _Param6, t_CParam7 _Param7, t_CParam8 _Param8, t_CParam9 _Param9, t_CParam10 _Param10, t_CParam11 _Param11)
{
	void* pMem = M_ALLOCALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData));
	return new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11);
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5, typename t_CParam6, typename t_CParam7, typename t_CParam8, typename t_CParam9, typename t_CParam10, typename t_CParam11, typename t_CParam12>
M_INLINE t_CData *TNew(t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5, t_CParam6 _Param6, t_CParam7 _Param7, t_CParam8 _Param8, t_CParam9 _Param9, t_CParam10 _Param10, t_CParam11 _Param11, t_CParam12 _Param12)
{
	void* pMem = M_ALLOCALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData));
	return new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12);
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5, typename t_CParam6, typename t_CParam7, typename t_CParam8, typename t_CParam9, typename t_CParam10, typename t_CParam11, typename t_CParam12, typename t_CParam13>
M_INLINE t_CData *TNew(t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5, t_CParam6 _Param6, t_CParam7 _Param7, t_CParam8 _Param8, t_CParam9 _Param9, t_CParam10 _Param10, t_CParam11 _Param11, t_CParam12 _Param12, t_CParam13 _Param13)
{
	void* pMem = M_ALLOCALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData));
	return new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12, _Param13);
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5, typename t_CParam6, typename t_CParam7, typename t_CParam8, typename t_CParam9, typename t_CParam10, typename t_CParam11, typename t_CParam12, typename t_CParam13, typename t_CParam14>
M_INLINE t_CData *TNew(t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5, t_CParam6 _Param6, t_CParam7 _Param7, t_CParam8 _Param8, t_CParam9 _Param9, t_CParam10 _Param10, t_CParam11 _Param11, t_CParam12 _Param12, t_CParam13 _Param13, t_CParam14 _Param14)
{
	void* pMem = M_ALLOCALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData));
	return new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12, _Param13, _Param14);
}

template <typename t_CData, typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3, typename t_CParam4, typename t_CParam5, typename t_CParam6, typename t_CParam7, typename t_CParam8, typename t_CParam9, typename t_CParam10, typename t_CParam11, typename t_CParam12, typename t_CParam13, typename t_CParam14, typename t_CParam15>
M_INLINE t_CData *TNew(t_CParam0 _Param0, t_CParam1 _Param1, t_CParam2 _Param2, t_CParam3 _Param3, t_CParam4 _Param4, t_CParam5 _Param5, t_CParam6 _Param6, t_CParam7 _Param7, t_CParam8 _Param8, t_CParam9 _Param9, t_CParam10 _Param10, t_CParam11 _Param11, t_CParam12 _Param12, t_CParam13 _Param13, t_CParam14 _Param14, t_CParam15 _Param15)
{
	void* pMem = M_ALLOCALIGN(sizeof(t_CData), M_ALIGNMENTOF(t_CData));
	return new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12, _Param13, _Param14, _Param15);
}

#define MNew(_ClassName) TNew< _ClassName >()
#define MNew1(_ClassName, _Param0) TNew< _ClassName >(_Param0)
#define MNew2(_ClassName, _Param0, _Param1) TNew< _ClassName >(_Param0, _Param1)
#define MNew3(_ClassName, _Param0, _Param1, _Param2) TNew< _ClassName >(_Param0, _Param1, _Param2)
#define MNew4(_ClassName, _Param0, _Param1, _Param2, _Param3) TNew< _ClassName >(_Param0, _Param1, _Param2, _Param3)
#define MNew5(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4) TNew< _ClassName >(_Param0, _Param1, _Param2, _Param3, _Param4)
#define MNew6(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5) TNew< _ClassName >(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5)
#define MNew7(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6) TNew< _ClassName >(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6)
#define MNew8(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7) TNew< _ClassName >(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7)
#define MNew9(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8) TNew< _ClassName >(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8)
#define MNew10(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9) TNew< _ClassName >(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9)
#define MNew11(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10) TNew< _ClassName >(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10)
#define MNew12(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11) TNew< _ClassName >(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11)
#define MNew13(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12) TNew< _ClassName >(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12)
#define MNew14(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12, _Param13) TNew< _ClassName >(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12, _Param13)
#define MNew15(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12, _Param13, _Param14) TNew< _ClassName >(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12, _Param13, _Param14)
#define MNew16(_ClassName, _Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12, _Param13, _Param14, _Param15) TNew< _ClassName >(_Param0, _Param1, _Param2, _Param3, _Param4, _Param5, _Param6, _Param7, _Param8, _Param9, _Param10, _Param11, _Param12, _Param13, _Param14, _Param15)
#endif



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC - MCC Run-Time Class
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CObj;
class CReferenceCount;
class CReferenceCountMT;

#ifndef M_RTM
	#ifdef PLATFORM_CONSOLE
		#define M_STATIC_ASSERT(expn) typedef char __C_ASSERT__[(expn)?1:-1] 
		#define M_ASSERT(f, Mess) if(!(f)) { M_BREAKPOINT; MRTC_Assert(Mess, __FILE__, __LINE__); }
		#define M_ASSERTHANDLER(f, Mess, Action) if (!(f)) { M_BREAKPOINT; MRTC_Assert(Mess, __FILE__, __LINE__); Action; }
	#else
		#define M_STATIC_ASSERT(expn) typedef char __C_ASSERT__[(expn)?1:-1] 
		#define M_ASSERT(f, Mess) if(!(f)) { MRTC_Assert(Mess, __FILE__, __LINE__); }
		#define M_ASSERTHANDLER(f, Mess, Action) if (!(f)) { MRTC_Assert(Mess, __FILE__, __LINE__); Action; }
	#endif
#else
	#define M_STATIC_ASSERT(expn) ((void)0) 
	#define M_ASSERT(f, Mess) ((void)0)
	#define M_ASSERTHANDLER(f, Mess, Action) ((void)0)
#endif


#include "MDA_IDS_TreeAVL.h"

class MRTC_CRuntimeClass
{
public:
	const char* m_ClassName;
	CReferenceCount*(*m_pfnCreateObject)();
	MRTC_CRuntimeClass* m_pClassBase;

#ifdef M_RTM
#ifdef MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCLASSES
	int AddInstance();
	int DelInstance();
#else
	M_INLINE int AddInstance(){return 0;}
	M_INLINE int DelInstance(){return 0;}
#endif
	M_INLINE int Instances(){return 0;}
#else
	int m_nDynamicInstances;
	int AddInstance();
	int DelInstance();
	int Instances();
#endif
	
	class CTreeCompare
	{
	public:
		M_INLINE static int Compare(const MRTC_CRuntimeClass *_pFirst, const MRTC_CRuntimeClass *_pSecond, void *_pContext);
		M_INLINE static int Compare(const MRTC_CRuntimeClass *_pTest, const char *_Key, void *_pContext);
	};


	DAVLAlignedA_Link(MRTC_CRuntimeClass, m_Link, const char *, CTreeCompare);

	//	int m_Size;
	//	int m_Version;
	//	MRTC_CRuntimeClass* m_pClassLess;		// Less node
	//	MRTC_CRuntimeClass* m_pClassGreater;	// Greater node
	//	MRTC_CRuntimeClass(const char* _ClassName, CReferenceCount*(*_pfnCreateObject)(), MRTC_CRuntimeClass* _pClassBase, const char *File, int Line);
	//	MRTC_CRuntimeClass(const char* _ClassName, CReferenceCount*(*_pfnCreateObject)(), MRTC_CRuntimeClass* _pClassBase);
};

// -------------------------------------------------------------------
class MRTC_CClassInit
{
public:
	MRTC_CClassInit(MRTC_CRuntimeClass* _pRTC);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC Non-dynamic
|__________________________________________________________________________________________________
\*************************************************************************************************/
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
//#define MRTC_CONSTPARAMS MRTC_CRuntimeClass *_pRunTimeClass
//#define MRTC_IMPLEMENTCONST(_BaseClass) : _BaseClass(_pRunTimeClass ? _pRunTimeClass : &m_RuntimeClass)
//#define MRTC_IMPLEMENTCONSTNOINFO(_BaseClass) : _BaseClass(NULL)
#else
//#define MRTC_CONSTPARAMS 
//#define MRTC_IMPLEMENTCONST(_BaseClass)
//#define MRTC_IMPLEMENTCONSTNOINFO(_BaseClass)
#endif

#define MRTC_DECLARE									\
public:														\
	static MRTC_CRuntimeClass m_RuntimeClass;				\
	virtual MRTC_CRuntimeClass* MRTC_GetRuntimeClass() const;	\
private:


#ifdef M_RTM
#	define MRTC_IMPLEMENT_NO_IGNORE(Name, BaseClass)																					\
		MRTC_CRuntimeClass Name::m_RuntimeClass = {#Name, NULL, &BaseClass::m_RuntimeClass};	\
		MRTC_CClassInit g_ClassReg##Name(&Name::m_RuntimeClass);				\
		MRTC_CRuntimeClass* Name::MRTC_GetRuntimeClass() const { return &m_RuntimeClass; };
#else
#	define MRTC_IMPLEMENT_NO_IGNORE(Name, BaseClass)																					\
		MRTC_CRuntimeClass Name::m_RuntimeClass = {#Name, NULL, &BaseClass::m_RuntimeClass, 0};	\
		MRTC_CClassInit g_ClassReg##Name(&Name::m_RuntimeClass);				\
		MRTC_CRuntimeClass* Name::MRTC_GetRuntimeClass() const { return &m_RuntimeClass; };
#endif

#define MRTC_IMPLEMENT(Name, BaseClass) MRTC_IMPLEMENT_NO_IGNORE(Name, BaseClass)

#ifdef M_RTM
#	define MRTC_IMPLEMENT_BASE_NO_IGNORE(Name)																					\
		MRTC_CRuntimeClass Name::m_RuntimeClass = {#Name, NULL, NULL};	\
		MRTC_CClassInit g_ClassReg##Name(&Name::m_RuntimeClass);				\
		MRTC_CRuntimeClass* Name::MRTC_GetRuntimeClass() const { return &m_RuntimeClass; };
#else
#	define MRTC_IMPLEMENT_BASE_NO_IGNORE(Name)																					\
		MRTC_CRuntimeClass Name::m_RuntimeClass = {#Name, NULL, NULL, 0};	\
		MRTC_CClassInit g_ClassReg##Name(&Name::m_RuntimeClass);				\
		MRTC_CRuntimeClass* Name::MRTC_GetRuntimeClass() const { return &m_RuntimeClass; };
#endif

#define MRTC_IMPLEMENT_BASE(Name) MRTC_IMPLEMENT_BASE_NO_IGNORE(Name)

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC Dynamic
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CreateCObjCommon(CReferenceCount* _pObj, MRTC_CRuntimeClass *_pClass);

template <typename t_CCreateClass>
CReferenceCount* CreateCObj()
{
#if !defined(M_RTM) || defined(MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCLASSES)
	t_CCreateClass *pObj = TNew<t_CCreateClass>(t_CCreateClass::m_RuntimeClass.m_ClassName, __FILE__, __LINE__);
#else
	t_CCreateClass *pObj = TNew<t_CCreateClass>();
#endif
	CreateCObjCommon(pObj, &t_CCreateClass::m_RuntimeClass);
	return pObj; 
}
/*
#define MRTC_DECLARE_DYNAMIC(Name)							\
public:														\
	static MRTC_CRuntimeClass m_RuntimeClass;				\
	virtual MRTC_CRuntimeClass* MRTC_GetRuntimeClass() const;	\
private:*/

#ifdef M_RTM
#	define MRTC_IMPLEMENT_DYNAMIC_NO_IGNORE(Name, BaseClass)	\
	MRTC_CRuntimeClass Name::m_RuntimeClass = {#Name, &CreateCObj<Name>, &BaseClass::m_RuntimeClass};	\
		MRTC_CClassInit g_ClassReg##Name(&Name::m_RuntimeClass);				\
		MRTC_CRuntimeClass* Name::MRTC_GetRuntimeClass() const { return &m_RuntimeClass; };
#else
#	define MRTC_IMPLEMENT_DYNAMIC_NO_IGNORE(Name, BaseClass)	\
	MRTC_CRuntimeClass Name::m_RuntimeClass = {#Name, &CreateCObj<Name>, &BaseClass::m_RuntimeClass, 0};	\
		MRTC_CClassInit g_ClassReg##Name(&Name::m_RuntimeClass);				\
		MRTC_CRuntimeClass* Name::MRTC_GetRuntimeClass() const { return &m_RuntimeClass; };

#endif

#define MRTC_IMPLEMENT_DYNAMIC(Name, BaseClass) MRTC_IMPLEMENT_DYNAMIC_NO_IGNORE(Name, BaseClass)


#ifdef M_RTM
#	define MRTC_IMPLEMENT_BASE_DYNAMIC_NO_IGNORE(Name)	\
	MRTC_CRuntimeClass Name::m_RuntimeClass = {#Name, &CreateCObj<Name>, NULL};	\
		MRTC_CClassInit g_ClassReg##Name(&Name::m_RuntimeClass);				\
		MRTC_CRuntimeClass* Name::MRTC_GetRuntimeClass() const { return &m_RuntimeClass; };
#else
#	define MRTC_IMPLEMENT_BASE_DYNAMIC_NO_IGNORE(Name)	\
	MRTC_CRuntimeClass Name::m_RuntimeClass = {#Name, &CreateCObj<Name>, NULL, 0};	\
		MRTC_CClassInit g_ClassReg##Name(&Name::m_RuntimeClass);				\
		MRTC_CRuntimeClass* Name::MRTC_GetRuntimeClass() const { return &m_RuntimeClass; };

#endif

#ifdef MRTC_IGNORE_DYNAMIC
	#define MRTC_IMPLEMENT_BASE_DYNAMIC(Name)
#else
	#define MRTC_IMPLEMENT_BASE_DYNAMIC(Name) MRTC_IMPLEMENT_BASE_DYNAMIC_NO_IGNORE(Name)
#endif

void MRTC_ReferenceSymbol(...);
#	define MRTC_REFERENCE(_Name)																					\
	extern MRTC_CClassInit g_ClassReg##_Name;\
	MRTC_ReferenceSymbol(&g_ClassReg##_Name);

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TPtr, "Smart pointer"
|__________________________________________________________________________________________________
\*************************************************************************************************/
void MCC_TPtrError(char* _pLocation, char* _pMsg);

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	TemplateClass:		Smart-pointer template.
						
	Parameters:			
		T:				Class of which to make to smart-pointer type.
						
	Comments:			Treat smart-pointers as regular pointers in
						all respects EXCEPT:
						
						- NEVER DELETE AN OBJECT HANDLED BY SMARTPOINTERS.

						- If you want to delete an object simply assign NULL
						to the smart-pointer.

						- Never assign an ordinary pointer to a smart-pointer
						EXCEPT when you allocate an objects, OR you know what 
						you are doing. Assignments all other ways are perfectly 
						valid.

						sp = sp		ok
						p = sp		ok
						sp = p		don't!, except if p == NULL.

						
						To the contrary of what you might believe:

						- Smart-pointers do not occupy any more space than an
						ordinary pointer, 4 bytes that is.

						- Dereferencing a smart-pointer is not more costly
						than dereferencing an ordinary pointer.


						The cost of smart-pointers is:

						- An extra 32-bit integer for reference counting
						in all objects inherited for CReferenceCount.

						- Occational increments/decrements of the reference 
						counter when smart-pointers are assigned to
						other smart-pointers or run out of scope.


\*____________________________________________________________________*/

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Run-time class base classes
|__________________________________________________________________________________________________
\*************************************************************************************************/

template <class T> class TPtr;


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Auto-typecast helper function for smart-pointers
						
	Comment:			This macro inserts a smart-pointer operator of 
						type TPtr<T> into this object. 
						This will preserve the automatic pointer 
						typecast functionality achieved by inheritance, 
						which would have been destroyed otherwise.
						A TPtr<TChild> could not act like a TPtr<T> as
						*TChild can act like a *T, if this operator is 
						not provided. 
\*____________________________________________________________________*/

#define MACRO_OPERATOR_TPTR(T)		\
operator TPtr<T> ()					\
{									\
	T* pT = this;					\
	TPtr<T> spT = pT;				\
	return spT;						\
}



/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Base object for run-time class enabled objects.
						
	Comments:			Base object for RTC with basic run-time class 
						functionality, such as retrieving the run-time
						class stucture for the class. 
						(see the MRTC_DECLARE macro for details.)
\*____________________________________________________________________*/

class CObj
{
	MRTC_DECLARE;

#if !defined(M_RTM) || defined(MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCLASSES)
public:
	MRTC_CRuntimeClass *m_pRunTimeClass;
	const char *m_pClassName;
	void PostConstruct(const char *_pClassName);
private:

	void *operator new(size_t _Size) throw()
	{
		// You cannot do new on a CReferenceCounted Object, instead call TNew<>
		return NULL;
	}
	
public:
	void *operator new(size_t _Size, void *_pPlacement)
	{
		return _pPlacement;
	}
#else
public:
#endif

	CObj();
	virtual void MRTC_VirtualCheck() {*((int *)(NULL)) = 0;};
	virtual ~CObj();
	virtual const char* MRTC_ClassName() const;
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				A pure virtual refcount interface.

	Comments:			This is a class for objects who need a 
	         			reference-count interface (TPtr<>-compatible),
	         			but not necessarily implemented the way 
	         			CReferenceCount does it..

\*____________________________________________________________________*/
class CVirtualRefCount
{
public:
	virtual ~CVirtualRefCount() {}
	virtual int MRTC_AddRef() pure;
	virtual int MRTC_DelRef() pure;
	virtual int MRTC_ReferenceCount() const pure;
	virtual void MRTC_Delete();
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Base object for all "Smart-Pointer" 
						enabled objects.
						
	Comments:			Contains a reference counter used by smart-
						pointers. Users should not call any of it's
						member functions.

						Calling MRTC_AddRef, MRTC_DelRef, MRTC_Delete
						will most certainly result in a crash.

						MRTC_ReferenceCount and MRTC_IsDynamic can be
						used safely, but they are only used in very
						rare situations and usualy by system-code.

						Classes inherited from CReferenceCount must
						re-implement copy & copy-constructor operations
						if they are needed. Automatic member-copy
						cannot be used since it would also copy the
						reference-counter. 

\*____________________________________________________________________*/


class CReferenceCount : public CObj
{
	MRTC_DECLARE;
		
	// This make the copy & copy-constructor operations private.
	// CReferenceCount classes cannot be duplicated, only if
	// these are re-implemented as public functions.
	void operator =(const CReferenceCount& _obj);
	CReferenceCount(const CReferenceCount& obj);
	
	union
	{
#ifdef CPU_POWERPC
		struct
		{
			int32 m_MRTC_bDynamic : 1;
			int32 m_MRTC_ReferenceCount : 31;
		};
		int32 m_InterlockedValue;
#else
		struct
		{
			int32 m_MRTC_ReferenceCount : 31;
			int32 m_MRTC_bDynamic : 1;
		};
		int32 m_InterlockedValue;
#endif
	};

public:
	CReferenceCount();
	virtual ~CReferenceCount() {};

	// If you need to use any of these functions you're most certainly doing something wrong.

	M_INLINE void MRTC_SetDynamic() 
	{
		m_MRTC_bDynamic = 1;
	}

	M_INLINE bool MRTC_IsDynamic() const
	{
		return m_MRTC_bDynamic != 0;
	}

	M_INLINE int MRTC_AddRef();

	M_INLINE int MRTC_DelRef();

	M_INLINE int MRTC_ReferenceCount() const 
	{
		return m_MRTC_ReferenceCount;
	}

	M_INLINE void MRTC_RemoveRef()
	{
		if (!MRTC_DelRef()) 
		{ 
			MRTC_Delete();
		}
	}
public:
	DECLARE_OPERATOR_NEW
//	void operator delete(void*);
		
public:
	virtual void MRTC_Delete();
	
	operator TPtr<CReferenceCount> ();

//	MACRO_OPERATOR_TPTR(CReferenceCount)
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TPtrBase
|__________________________________________________________________________________________________
\*************************************************************************************************/
typedef TPtr<CReferenceCount> spCReferenceCount;
#define CRef CReferenceCount

template <class T>
class TPtrBase
{
public:
	
	union
	{
		T *p;
		char *m_cp;
	};
	
	
	void Construct(TPtrBase *_pFrom)
	{
		p = _pFrom->p;
		
		if (p != NULL)
			p->MRTC_AddRef();
	}
	
	void Construct(T *_pFrom)
	{
		p = _pFrom;
		if (p != NULL) 
			p->MRTC_AddRef();
		
	}
	
	void Construct()
	{
		p = NULL;
	}

	void Destruct()
	{
		if (p)
		{
			if (! p->MRTC_DelRef()) 
			{ 
				p->MRTC_Delete(); 
				p = NULL; 
			};
		}
	}

	void CheckP()
	{
		if (!p) 
			MCC_TPtrError("TPtr<?>::operator *", "NULL pointer.");
	}

	void Assign(const TPtrBase& _p)
	{
		if (_p.p)
			_p.p->MRTC_AddRef();
		
		if (p)
			if (p->MRTC_DelRef() == 0)
			{
				p->MRTC_Delete();
				p = NULL;
			};
			
			p = _p.p;
	}

	void Assign(T* _p)
	{
		if (_p != NULL) 
			_p->MRTC_AddRef();
		
		if (p != NULL) 
		{
			if (p->MRTC_DelRef() == 0)  
			{ 
				p->MRTC_Delete(); 
				p = NULL; 
			};
		}
		
		p = _p;
	}

	static void Del()
	{
		MCC_TPtrError("TPtrBase<?>::operator delete", "Delete is not allowed on smart-pointers.");
	};
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TPtr2
|__________________________________________________________________________________________________
\*************************************************************************************************/
template <class T, class TRefBase>
class TPtr2 : public TPtrBase<TRefBase>
{
	
public:

#if !defined( PLATFORM_DOLPHIN ) && !defined(PLATFORM_PS2)
	void operator delete(void *)
	{
		TPtrBase<TRefBase>::Del();
	}
#endif

	TPtr2()
	{
		TPtrBase<TRefBase>::Construct();
	}

	TPtr2(T* _p)
	{
		TPtrBase<TRefBase>::Construct(_p);
	}

	~TPtr2()
	{
		TPtrBase<TRefBase>::Destruct();
	}

	TPtr2(const TPtr2& _p)
	{
		TPtrBase<TRefBase>::Construct(const_cast<TPtr2*>(&_p));
	}

/*
#ifdef COMPILER_CODEWARRIOR
	bool operator== (const void* _p) const { return (void*)p == _p; };
	bool operator!= (const void* _p) const { return (void*)p != _p; };
	bool operator! () const { return p == NULL; };

	bool operator == (const TPtr2& _sp) const { return (void*)p == (void*)_sp.p; };
	bool operator != (const TPtr2& _sp) const { return (void*)p != (void*)_sp.p; };
#endif
*/
	M_INLINE operator T* ()
	{
		return (T*)TPtrBase<TRefBase>::p;
	}
	M_INLINE operator const T* () const
	{ 
		return (T*)TPtrBase<TRefBase>::p; 
	};
	M_INLINE T& operator* ()
	{
		return *((T*)TPtrBase<TRefBase>::p);
	}
	M_INLINE T* operator-> ()
	{
		return (T*)TPtrBase<TRefBase>::p; 
	}
	M_INLINE T const * operator-> (void) const
	{
		return (T*)TPtrBase<TRefBase>::p; 
	}

	TPtr2& operator= (T* _p)
	{
		Assign(_p);
		return *this;
	}
	TPtr2& operator= (const TPtr2& _p)
	{
		Assign(_p);
		return *this;
	}
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TPtr
|__________________________________________________________________________________________________
\*************************************************************************************************/
template <class T>
class TPtr : public TPtrBase<CReferenceCount>
{
	
public:
	typedef TPtrBase<CReferenceCount> CPtrBase;

#if !defined(COMPILER_CODEWARRIOR)
	void operator delete(void *)
	{
		Del();
	}
#endif

	TPtr()
	{
		Construct();
	}

	TPtr(T* _p)
	{
		Construct(_p);
	}

	~TPtr()
	{
		Destruct();
	}

	TPtr(const TPtr& _p)
	{
#if defined(COMPILER_CODEWARRIOR) || defined(COMPILER_GNU)
		Construct(const_cast<CPtrBase*>((const CPtrBase *)&((const CPtrBase&)_p)));
#else
		Construct((CPtrBase *)&((CPtrBase)_p));
#endif
	}

/*
#ifdef COMPILER_CODEWARRIOR
	bool operator== (const void* _p) const { return (void*)p == _p; };
	bool operator!= (const void* _p) const { return (void*)p != _p; };
	bool operator! () const { return p == NULL; };

	bool operator == (const TPtr& _sp) const { return (void*)p == (void*)_sp.p; };
	bool operator != (const TPtr& _sp) const { return (void*)p != (void*)_sp.p; };
#endif
*/
	M_INLINE operator T* ()
	{
		return (T*)p;
	}
	M_INLINE operator const T* () const
	{ 
		return (T*) p; 
	};
	M_INLINE T& operator* ()
	{
		return *((T*)p);
	}
	M_INLINE T* operator-> ()
	{
		return (T*) p; 
	}
	M_INLINE T const * operator-> (void) const
	{
		return (T*) p; 
	}

	TPtr& operator= (T* _p)
	{
		Assign(_p);
		return *this;
	}
	TPtr& operator= (const TPtr& _p)
	{
		Assign(_p);
		return *this;
	}
};



#define M_TPTR(x) typedef TPtr<x> sp##x;



#ifdef COMPILER_MSVC
#pragma warning(disable : 4251 4275)
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TSemiSamrtPtr
|__________________________________________________________________________________________________
\*************************************************************************************************/
template <class T>
class TSemiSmartPtr
{
	
public:

	T *m_pObject;

	TSemiSmartPtr()
	{
		m_pObject = NULL;
	}

	TSemiSmartPtr(T* _p)
	{
		m_pObject = _p;
	}

	~TSemiSmartPtr()
	{
		if (m_pObject)
			delete m_pObject;
	}

	void Clear()
	{
		m_pObject = NULL;
	}
/*
#ifdef COMPILER_CODEWARRIOR
	bool operator== (const void* _p) const { return (void*)m_pObject == _p; };
	bool operator!= (const void* _p) const { return (void*)m_pObject != _p; };
	bool operator! () const { return m_pObject == NULL; };

	bool operator == (const TSemiSmartPtr& _sp) const { return (void*)m_pObject == (void*)_sp.p; };
	bool operator != (const TSemiSmartPtr& _sp) const { return (void*)m_pObject != (void*)_sp.p; };
#endif
*/
	M_INLINE operator T* ()
	{
		return m_pObject;
	}
	M_INLINE operator const T* () const
	{ 
		return m_pObject; 
	};
	M_INLINE T& operator* ()
	{
		return *m_pObject;
	}
	M_INLINE T* operator-> ()
	{
		return m_pObject; 
	}
	M_INLINE T const * operator-> (void) const
	{
		return m_pObject; 
	}

	TSemiSmartPtr& operator= (T* _p)
	{
		if (m_pObject)
			delete m_pObject;
		m_pObject = _p;
		return *this;
	}
	TSemiSmartPtr& operator= (const TSemiSmartPtr& _p)
	{
		if (m_pObject)
			delete m_pObject;
		m_pObject = _p;
		return *this;
	}
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_AutoStrip
|__________________________________________________________________________________________________
\*************************************************************************************************/
#if defined(MRTC_AUTOSTRIPLOGGER)
	// game will dump usage of all functions to logfile
	#define MAUTOSTRIP_VOID
	void MRTC_AutoStripLog(const char *_pFunc);
	#define MAUTOSTRIP(func, ret)         \
		static bool s_bLog##func = false; \
		if (!s_bLog##func)                \
		{                                 \
			s_bLog##func = true;          \
			MRTC_AutoStripLog(#func);     \
		}

#elif defined(MRTC_AUTOSTRIP)
	// game will only use functions dumped from previous log-session
	#define MAUTOSTRIP_VOID
	#include "MRTC_AutoStrip.h"
	#define MAUTOSTRIP(func, ret) MAUTOSTRIPx_##func(ret)

#else
	// game will ignore all autostrip-macros
	#define MAUTOSTRIP(func, ret)

#endif



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_SystemInfo
|__________________________________________________________________________________________________
\*************************************************************************************************/
#include "MRTC_System.h"
#include "MRTC_Protect.h"


M_INLINE int CReferenceCount::MRTC_AddRef()
{
	return (MRTC_SystemInfo::Atomic_Increase(&m_InterlockedValue) & 0x7fffffff) + 1;
}

M_INLINE int CReferenceCount::MRTC_DelRef()
{
	return (MRTC_SystemInfo::Atomic_Decrease(&m_InterlockedValue) & 0x7fffffff) - 1;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_ClassContainer
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Contains all run-time class information for a
						module. (EXE or DLL)
						
	Comments:			Contains a binary seach tree with all
						MRTC_CRuntimeClass objects in a module.
						The class container is created automatically
						when a module is loaded and added to the
						class-registry for the process.
\*____________________________________________________________________*/

class MRTC_ClassContainer		// Must be aggregate (no constr, virtuals, private/protected)
{
public:
//	MRTC_CRuntimeClass* m_pClassRoot;

	DAVLAlignedA_Tree(MRTC_CRuntimeClass, m_Link, const char *, MRTC_CRuntimeClass::CTreeCompare) m_ClassTree;
	typedef DIdsTreeAVLAligned_Iterator(MRTC_CRuntimeClass, m_Link, const char *, MRTC_CRuntimeClass::CTreeCompare) CClassIter;

//	void Insert_r(MRTC_CRuntimeClass* _pNode, MRTC_CRuntimeClass* _pRTC);
//	MRTC_CRuntimeClass* Find_r(MRTC_CRuntimeClass* _pNode, const char* _pName);
//	void FindPrefix_r(MRTC_CRuntimeClass* _pNode, const char *_pName, int _iLen, MRTC_CRuntimeClass **_pList, int _MaxSize, int &_Pos);

	void Insert(MRTC_CRuntimeClass* _pRTC);
	MRTC_CRuntimeClass* Find(const char* _pName);

#ifndef M_RTM
//	void Dump_r(MRTC_CRuntimeClass*, int);
	void Dump(int _Flags);
#endif
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_ClassLibraryInfo
|__________________________________________________________________________________________________
\*************************************************************************************************/
class MRTC_ObjectManager;

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Stores information about an MRTC class-library.
						(i.e, a DLL)
\*____________________________________________________________________*/

class MRTC_ClassLibraryInfo
{
public:
	MRTC_ClassLibraryInfo()
	{
		m_pName = 0;
		m_Handle = 0;
		m_nRef = 0;
		m_pNextCL = 0;
		m_pfnGetClassContainer = NULL;
		m_pfnDLLAttachProcess = NULL;
		m_pfnDLLDetachProcess = NULL;
	}

	~MRTC_ClassLibraryInfo()
	{
		if (m_pName) delete[] m_pName; 
		m_pName = NULL;
	}

	char* m_pName;
	void* m_Handle;
	int m_nRef;
	MRTC_ClassContainer* (*m_pfnGetClassContainer)();
	void(*m_pfnDLLAttachProcess)(MRTC_ObjectManager*);
	void(*m_pfnDLLDetachProcess)(MRTC_ObjectManager*);

	MRTC_ClassLibraryInfo* m_pNextCL;
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_CClassRegistry
|__________________________________________________________________________________________________
\*************************************************************************************************/
class MRTC_ClassContainerLink
{
public:
	MRTC_ClassContainer* m_pClassContainer;
	MRTC_ClassContainerLink* m_pNext;
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Contains references to all class containers
						available and manages class-library IO within
						a process.
\*____________________________________________________________________*/

class MRTC_CClassRegistry : public CReferenceCount
{
	MRTC_ClassContainerLink* m_pFirstContainer;

	MRTC_ClassLibraryInfo* m_pFirstCL;

	MRTC_ClassLibraryInfo* FindClassLibrary(char* _pName);
	
public:
	MRTC_CClassRegistry();
	~MRTC_CClassRegistry();
	void AddClassContainer(MRTC_ClassContainer* _pClassContainer);
	void RemoveClassContainer(MRTC_ClassContainer* _pClassContainer);

	void LoadClassLibrary(char* _pName);
	void UnloadClassLibrary(char* _pName);

	MRTC_CRuntimeClass* GetRuntimeClass(const char* _pName);
	CReferenceCount* CreateObject(const char* _pName);

	int FindPrefix(const char *_pName, MRTC_CRuntimeClass **_pList, int _MaxSize);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CStr
|__________________________________________________________________________________________________
\*************************************************************************************************/
#include "MRTC_String.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Context stack
|__________________________________________________________________________________________________
\*************************************************************************************************/

class MRTC_Context
{
public:
 #ifdef MRTC_ENABLE_MSCOPE
	const char* m_pName;	// Name of the context.
	const char* m_pMemoryCategory;	// Name of the context.
	uint32 m_MemUsedCreated;	// Available memory when the context was created.
	uint64 m_Clocks;
	uint64 m_ClocksWaste;

	void Construct(const char* _pCtx);
 #endif

	MRTC_Context(const char* _pCtx)
	{
	 #ifdef MRTC_ENABLE_MSCOPE
		m_pName = _pCtx;
		m_pMemoryCategory = NULL;
		Construct(_pCtx);
	 #endif
	}

	MRTC_Context(const char* _pCtx, const char* _pMemC)
	{
	 #ifdef MRTC_ENABLE_MSCOPE
		m_pName = _pCtx;
		m_pMemoryCategory = _pMemC;
		Construct(_pCtx);
	 #endif
	}

	~MRTC_Context();
};

// -------------------------------------------------------------------
#define MRTC_CONTEXT_STACK_DEPTH	256

class MRTC_ContextStack : public CReferenceCount
{
public:
	MRTC_Context* m_lpContexts[MRTC_CONTEXT_STACK_DEPTH];
	int m_iContext;

	MRTC_ContextStack();

	CStr GetContextStack();
	CStr GetLastContextStack();

	void PushContext(MRTC_Context* _pCtx);
	void PopContext(MRTC_Context* _pCtx);
};

#ifdef MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCATEGORY
	class MRTC_RemoteDebugCategory
	{
	public:
		const char * m_pName;

		MRTC_RemoteDebugCategory(const char *_pCategory);
		~MRTC_RemoteDebugCategory();
	};
#	define DRDCategory(_Category) MRTC_RemoteDebugCategory RemoteDebugCategory(_Category)
#else
#	define DRDCategory(_Category) ((void)0)
#endif

#ifdef MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTSCOPE
	class MRTC_RemoteDebugScope
	{
	public:
		const char * m_pName;

		MRTC_RemoteDebugScope(const char *_pScope);
		~MRTC_RemoteDebugScope();
	};
#	define DRDScope(_Category) MRTC_RemoteDebugScope RemoteDebugScope(_Category)
#else
#	define DRDScope(_Category) ((void)0)
#endif

#ifdef MRTC_ENABLE_REMOTEDEBUGGER_USESCOPECATEGORY
#	define DRDCategoryMSCOPE(_Category) DRDCategory(_Category)
#else
#	define DRDCategoryMSCOPE(_Category) ((void)0)
#endif

#ifdef MRTC_ENABLE_REMOTEDEBUGGER_USESCOPE
#	define DRDScopeMSCOPE(_Category) DRDScope(_Category)
#else
#	define DRDScopeMSCOPE(_Category) ((void)0)
#endif


// -------------------------------------------------------------------
#ifdef MRTC_ENABLE_MSCOPE

	#define MSCOPESHORT(Ctx)									\
		static const char ContextName[] = #Ctx;			\
		MRTC_Context _Context_(ContextName); \
		DRDScopeMSCOPE(ContextName);

	#define MSCOPE(Ctx, MemC)									\
		static const char ContextName[] = #Ctx;			\
		static const char ContextMemoryName[] = #MemC;			\
		MRTC_Context _Context_(ContextName,ContextMemoryName);	\
		DRDCategoryMSCOPE(ContextMemoryName);							\
		DRDScopeMSCOPE(ContextName);

	#define MSCOPESHORT_STR(pStr)							\
		CStr ContextName = pStr;								\
		MRTC_Context _Context_(ContextName);							\
		DRDScopeMSCOPE(pStr);

	#define MSCOPE_STR(pStr, MemC)							\
		CStr ContextName = pStr;								\
		static const char ContextMemoryName[] = #MemC;			\
		MRTC_Context _Context_(ContextName, ContextMemoryName);					\
		DRDCategoryMSCOPE(ContextMemoryName);									\
		DRDScopeMSCOPE(ContextName);

	#define MPUSH(Ctx)									\
		{												\
			static const char ContextName[] = #Ctx;		\
			MRTC_Context _Context_(ContextName);			\
			DRDScopeMSCOPE(ContextName);

	#define MPOP }

	#define MSCOPE_MEMDELTA	(_Context_.m_MemUsedCreated - MRTC_MemUsed())

#else

	#define MSCOPESHORT(Ctx) DRDScopeMSCOPE(#Ctx)

	#define MSCOPE(Ctx, MemC) DRDCategoryMSCOPE(#MemC); DRDScopeMSCOPE(#Ctx)

#ifdef MRTC_ENABLE_REMOTEDEBUGGER_USESCOPECATEGORY
	#define MSCOPESHORT_STR(pStr)	\
		CStr ContextName = pStr;								\
		DRDScopeMSCOPE(ContextName)
#else
	#define MSCOPESHORT_STR(pStr)	DRDScopeMSCOPE(pStr)
#endif

#if defined(MRTC_ENABLE_REMOTEDEBUGGER_USESCOPE) || defined(MRTC_ENABLE_REMOTEDEBUGGER_USESCOPECATEGORY)
	#define MSCOPE_STR(pStr, MemC)	\
		CStr ContextName = pStr;								\
		static const char ContextMemoryName[] = #MemC;			\
		DRDCategoryMSCOPE(ContextMemoryName); DRDScopeMSCOPE(ContextName)
#else
	#define MSCOPE_STR(pStr, MemC)	DRDCategoryMSCOPE(#MemC); DRDScopeMSCOPE(pStr)
#endif

	#define MPUSH(Ctx)									\
		{												\
			DRDScopeMSCOPE(#Ctx);

	#define MPOP }

	#define MSCOPE_MEMDELTA (int)0

#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_ObjectManager
|__________________________________________________________________________________________________
\*************************************************************************************************/
class MRTC_ObjMgr_Entry;
typedef TPtr<MRTC_ObjMgr_Entry> spMRTC_ObjMgr_Entry;

class MRTC_ObjMgr_Entry : public CReferenceCount
{
public:
	DECLARE_OPERATOR_NEW

	int m_nRef;
	CStr m_ObjectName;
	spCReferenceCount m_spObj;
	spMRTC_ObjMgr_Entry m_spNodeL;
	spMRTC_ObjMgr_Entry m_spNodeG;
	MRTC_ObjMgr_Entry* m_pNodeParent;

	MRTC_ObjMgr_Entry();
	void operator= (const MRTC_ObjMgr_Entry& _Entry);
	bool operator< (const MRTC_ObjMgr_Entry& _Entry) const;

	MRTC_ObjMgr_Entry* Find_r(const char* _pObjName);
	void Insert_r(spMRTC_ObjMgr_Entry _spEntry);
	void InsertTree_r(spMRTC_ObjMgr_Entry _spEntry);
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Contain a registry of process-global
						objects and the class-registry.

						Only executables have this object. DLLs use
						the object-manager of the process they are 
						running in.

						Don't mix up the object-manager and the class-
						registry. The former contains named global
						objects, while the other contain run-time class
						information.
\*____________________________________________________________________*/

#ifdef PLATFORM_WIN_PC
	#define D_MXDFCREATE MRTC_GetObjectManager()->m_bXDFCreate
	#define D_MPLATFORM MRTC_GetObjectManager()->m_Platform
#else
	#define D_MXDFCREATE 0
	#ifdef PLATFORM_XENON
		#define D_MPLATFORM 4
	#elif defined PLATFORM_PS3
		#define D_MPLATFORM 5
	#else
		#error "Implement this"
	#endif
#endif

template <typename t_CType>
class TMRTC_ThreadLocal;

class MRTC_ObjectManager
{
public:
	aint m_ThreadStorageIndex;

	class CDA_MemoryManager* m_pMemoryManager;
	class CByteStreamManager *m_pByteStreamManager;
	MRTC_MemoryProtect m_MemoryProtect;
	int m_ModuleCount;
#if defined(MRTC_AUTOSTRIPLOGGER)
	class CUsageLogger*      m_pAutoStripLogger;
#endif
	uint8 m_bAssertHandler : 1;
	uint8 m_bBreakOnAssert : 1;

protected:
	spMRTC_ObjMgr_Entry m_spRoot;
	MRTC_CClassRegistry m_ClassRegistry;
	bool DllLoading;

public:

#ifdef M_Profile
	class MRTC_CallGraph* m_pCallGraph;
	TMRTC_ThreadLocal<MRTC_ContextStack> *m_ContextStackInternal;
	TMRTC_ThreadLocal<MRTC_ContextStack> &GetContexctStack()
	{
		return *m_ContextStackInternal;
	}
#endif

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	class MRTC_RemoteDebug* m_pRemoteDebugger;
#endif
	
	void * m_hMainThread;
	void* m_hScriptThread;

	class MRTC_ThreadPoolManagerInternal* m_pThreadPoolManagerInternal;
	class MRTC_ScratchPadManagerInternal* m_pScratchPadManagerInternal;

	MRTC_CriticalSection *m_pGlobalStrLock;
	MRTC_CriticalSection *m_pGlobalLock;
	MRTC_CriticalSection *m_pObjMgrLock;

	class CRand_MersenneTwister* m_pRand;

#ifdef PLATFORM_WIN_PC
	int m_Platform;
	int m_bXDFCreate;
#endif

#ifdef M_STATIC
	static MRTC_SystemInfo *m_pSystemInfo;
	static uint64 m_SystemInfoData[(sizeof(MRTC_SystemInfo) + 7) / 8];
#else
	MRTC_SystemInfo m_SystemInfo;
#endif

	void ForgiveDebugNew(int32 _iAdd);
	int32 ForgiveDebugNew();
#ifndef PLATFORM_PS3
	class MRTC_ForgiveDebugNewInternal *m_pForgiveContextInternal;
#endif	// PLATFORM_PS3
	MRTC_ObjectManager();
	~MRTC_ObjectManager();

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
#ifndef MRTC_ENABLE_REMOTEDEBUGGER_STATIC
	class MRTC_RemoteDebug* GetRemoteDebugger();
#endif
#endif

	virtual MRTC_CClassRegistry* GetClassRegistry();										// Get the class-registry.

	// Dynamic object creation
	virtual CReferenceCount* CreateObject(const char* _pClassName);							// Create a run-time class object.
	virtual bool GetDllLoading();
	virtual void SetDllLoading(bool LoadDlls);
	

	// Object registering
	virtual spCReferenceCount CreateObject(const char* _pClassName, const char* _pName);	// Create and register a run-time class object.
	virtual bool RegisterObject(spCReferenceCount _spObj, const char* _pName);				// Register an object in the object-manager.
	virtual bool UnregisterObject(spCReferenceCount _spObj, const char* _pName);			// Unregister an object.
	virtual void UnregisterAll();															// Unregister all objects. DONT USE!
	virtual spCReferenceCount GetRegisteredObject(const char* _pName);						// Get a registered object.

	bool InMainThread();
	
#if defined(MRTC_AUTOSTRIPLOGGER)
	class CUsageLogger* GetAutostripLogger();
#endif
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Get the process's object manager.
\*____________________________________________________________________*/

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_ObjectManager_Container
|__________________________________________________________________________________________________
\*************************************************************************************************/
class MRTC_ObjectManager_Container
{
public:
	MRTC_ObjectManager *m_pManager;
	class CDA_MemoryManager* m_pMemoryManager;

#ifdef PLATFORM_XBOX1
	class CDA_MemoryManager* m_pGraphicsHeap;
/*#ifdef M_Profile
	class CDA_MemoryManager* m_pGraphicsHeapCached;
#endif*/
#endif
	
#if defined(PLATFORM_DOLPHIN) && defined(USE_VIRTUAL_MEMORY)
	CDA_MemoryManager* m_pVirtualHeap;
#endif

#ifndef M_STATICINIT
	int m_bInitialized;

	void* m_hLock;

	void Lock()
	{
		MRTC_SystemInfo::OS_MutexLock(m_hLock);
	}

	void Unlock()
	{
		MRTC_SystemInfo::OS_MutexUnlock(m_hLock);
	}
#endif
};
extern MRTC_ObjectManager_Container g_ObjectManagerContainer;

#ifdef M_STATICINIT
	M_INLINE static MRTC_ObjectManager* MRTC_GetObjectManager() {return g_ObjectManagerContainer.m_pManager;}
	M_INLINE static class CDA_MemoryManager* MRTC_GetMemoryManager(){return g_ObjectManagerContainer.m_pMemoryManager;}
#	ifdef PLATFORM_XBOX1
		M_INLINE static CDA_MemoryManager* MRTC_GetGraphicsHeap(){return g_ObjectManagerContainer.m_pGraphicsHeap;}
		/*
#		ifdef M_Profile
			M_INLINE static CDA_MemoryManager* MRTC_GetGraphicsHeapCached(){return g_ObjectManagerContainer.m_pGraphicsHeapCached;}
#		else
			M_INLINE static CDA_MemoryManager* MRTC_GetGraphicsHeapCached(){return g_ObjectManagerContainer.m_pMemoryManager;}
#		endif*/
#	endif
#else
	extern MRTC_ObjectManager* MRTC_GetObjectManager();
	extern class CDA_MemoryManager* MRTC_GetMemoryManager();
#	ifdef PLATFORM_XBOX1
		extern CDA_MemoryManager* MRTC_GetGraphicsHeap();
//		extern CDA_MemoryManager* MRTC_GetGraphicsHeapCached();
#	endif

#endif
M_INLINE class CRand_MersenneTwister* MRTC_GetRand() {return MRTC_GetObjectManager()->m_pRand;}


#if defined(PLATFORM_DOLPHIN) && defined(USE_VIRTUAL_MEMORY)
extern CDA_MemoryManager* MRTC_GetVirtualHeap();
#endif


// You tend to write MRTC_GetObjectManager quite alot, so...
#define MRTC_GOM MRTC_GetObjectManager

#define MRTC_GETRAND MRTC_GetRand
#define MRTC_RAND   MRTC_GetRand()->GenRand32

#ifdef MRTC_DLL

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Makes a DLL aware that a process is using it
						and supplies it with the process' object manager.
						Called by the object-registry of the process
						loading this DLL.
\*____________________________________________________________________*/
extern void MRTC_DLLAttachProcess(MRTC_ObjectManager* _pObjMgr);

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			The reversal of MRTC_DLLAttachProcess. 
						Called by the object-registry of the process
						loading this DLL.
\*____________________________________________________________________*/
extern void MRTC_DLLDetachProcess(MRTC_ObjectManager* _pObjMgr);

#endif


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Comments:			Helper macro to retrieve an object from the
						object manager with safe typcasting.
\*____________________________________________________________________*/

#define MACRO_GetRegisterObject(Class, Name, RegName)									\
Class *Name;																			\
{																						\
	spCReferenceCount spObj = MRTC_GetObjectManager()->GetRegisteredObject(RegName);	\
	Name = TDynamicCast<Class>((CObj*)spObj);											\
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Helper for creating dynamic objects
\*____________________________________________________________________*/
#ifdef	COMPILER_RTTI

	#ifdef COMPILER_GNU
		#define MRTC_TYPEINFOCLASS std::type_info
	#else
		#define MRTC_TYPEINFOCLASS type_info
	#endif

	void* MRTC_SafeCreateObject_NoEx(const char* _pClassName, const MRTC_TYPEINFOCLASS& _TypeInfo);
	void* MRTC_SafeCreateObject(const char* _pClassName, const MRTC_TYPEINFOCLASS& _TypeInfo);

	// -------------------------------------------------------------------
	#define MRTC_SAFECREATEOBJECT_NOEX(Var, ClassCreate, Class)		TPtr<Class> Var = (Class*)MRTC_SafeCreateObject_NoEx(ClassCreate, typeid(Class))
	#define MRTC_SAFECREATEOBJECT(Var, ClassCreate, Class)			TPtr<Class> Var = (Class*)MRTC_SafeCreateObject(ClassCreate, typeid(Class))

//	#define MRTC_ISKINDOF(Object, ClassName)						(typeid(Object) == typeid(ClassName))
	#define MRTC_ISKINDOF(Object, ClassName)						(Object->MRTC_ClassName() == ClassName::m_RuntimeClass.m_ClassName)

#else

	void* MRTC_SafeCreateObject_NoEx(const char* _pClassName, const MRTC_CRuntimeClass* _TypeInfo);
	void* MRTC_SafeCreateObject(const char* _pClassName, const MRTC_CRuntimeClass* _TypeInfo);

	// -------------------------------------------------------------------
	#define MRTC_SAFECREATEOBJECT_NOEX(Var, ClassCreate, Class)		TPtr<Class> Var = (Class*)MRTC_SafeCreateObject_NoEx(ClassCreate, &Class::m_RuntimeClass)
	#define MRTC_SAFECREATEOBJECT(Var, ClassCreate, Class)			TPtr<Class> Var = (Class*)MRTC_SafeCreateObject(ClassCreate, &Class::m_RuntimeClass)
	#define MRTC_ISKINDOF(Object, ClassName)						(Object->MRTC_ClassName() == ClassName::m_RuntimeClass.m_ClassName)

#endif
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC Headers
|__________________________________________________________________________________________________
\*************************************************************************************************/

#include "MRTC_Exception.h"
#include "MRTC_Misc.h"
#include "MRTC_Thread.h"

template <class T1, class T2>
M_INLINE T1 *safe_cast( T2 _Obj )
{
#ifdef	M_RTM
	return (T1 *)(_Obj);
#else
	T1 *Ret = TDynamicCast<T1>(_Obj);
#ifdef	_DEBUG
	M_ASSERT( ( Ret == (T1 *)(_Obj) ), "Invalid safe_cast" );
#else
	if( Ret != (T1 *)(_Obj) )
		Error_static( "safe_cast", "Invalid safe_cast" );
#endif
	return Ret;
#endif
}

class CVPU_JobDefinition;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_ThreadPoolManager
|__________________________________________________________________________________________________
\*************************************************************************************************/
typedef void (*MRTC_ThreadJobFunction)(int _iObj, void* _pArg);
class MRTC_ThreadPoolManager
{
public:
	static void Create(int _nThreads, int _StackSize);	// Better not be using it while doing this
	static void Enable(bool _bEnable);
	static int GetNumThreads();
	static void ProcessEachInstance(int _nObj, void* _pArg, MRTC_ThreadJobFunction _pfnFunction, const char* _pTaskName = NULL, bint _bSync = false);

	static uint32 VPU_AddTask(CVPU_JobDefinition& _JobDefinition);
	static bool VPU_IsTaskComplete(uint32 _Task);
	static void VPU_BlockOnTask(uint32 _Task);
	static bool VPU_TryBlockUntilIdle();
	static void VPU_BlockUntilIdle();
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_ScratchPadManager
|__________________________________________________________________________________________________
\*************************************************************************************************/
class MRTC_ScratchPadManager
{
public:
	static uint8* Get(uint32 _Size);
};

M_INLINE int MRTC_CRuntimeClass::CTreeCompare::Compare(const MRTC_CRuntimeClass *_pFirst, const MRTC_CRuntimeClass *_pSecond, void *_pContext)
{
	return CStrBase::stricmp(_pFirst->m_ClassName, _pSecond->m_ClassName);
}

M_INLINE int MRTC_CRuntimeClass::CTreeCompare::Compare(const MRTC_CRuntimeClass *_pTest, const char *_Key, void *_pContext)
{
	return CStrBase::stricmp(_pTest->m_ClassName, _Key);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Random number generator
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CRand_MersenneTwister
{
protected:
	enum { N = 624, M = 397, MATRIX_A = 0x9908b0df, UPPER_MASK = 0x80000000, LOWER_MASK = 0x7fffffff};
	uint32 m_mt[N];
	uint32 m_mti;
	NThread::CSpinLock m_Lock;
public:

	CRand_MersenneTwister()
	{
		InitRand(5489);
	}

	void InitRand(uint32 _Seed)
	{
		M_LOCK(m_Lock);
		m_mt[0] = _Seed;
		for(uint32 mti = 1; mti < N; mti++)
			m_mt[mti] = (1812433253 * (m_mt[mti - 1] ^ (m_mt[mti - 1] >> 30)) + mti);
		m_mti = N;
	}

	uint32 GenRand32()
	{
		M_LOCK(m_Lock);
		uint32 y;
		static uint32 mag01[2] = {0, MATRIX_A};
		if(m_mti >= N)
		{
			int kk;
			for(kk = 0; kk < N - M; kk++)
			{
				y = (m_mt[kk] & UPPER_MASK) | (m_mt[kk + 1] & LOWER_MASK);
				m_mt[kk] = m_mt[kk + M] ^ (y >> 1) ^ mag01[y & 1];
			}
			for(;kk < N - 1; kk++)
			{
				y = (m_mt[kk] & UPPER_MASK) | (m_mt[kk + 1] & LOWER_MASK);
				m_mt[kk] = m_mt[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 1];
			}
			y = (m_mt[N - 1] & UPPER_MASK) | (m_mt[0] & LOWER_MASK);
			m_mt[N - 1] = m_mt[M - 1] ^ (y >> 1) ^ mag01[y & 1];
			m_mti = 0;
		}

		y = m_mt[m_mti++];

		// Tempering
		y ^= (y >> 11);
		y ^= (y << 7) & 0x9d2c5680;
		y ^= (y << 15) & 0xefc60000;
		y ^= (y >> 18);

	    return y;
	}

	int32 GenRand31()
	{
		return GenRand32() >> 1;
	}

	// Random 0 to 1 inclusive (can return 0 and 1)
	fp4 GenRand1Inclusive_fp4()
	{
		return GenRand32() * (1.0f/4294967295.0f);
	}

	// Random 0 to 1 exclusive (cannot return 1)
	fp4 GenRand1Exclusive_fp4()
	{
		return GenRand32() * (1.0f/4294967296.0f);
	}

};


#endif // _INC_MOS_MRTC


