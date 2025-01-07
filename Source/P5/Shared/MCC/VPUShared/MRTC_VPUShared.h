
#ifndef MACRO_INC_MRTC_VPUShared_h
#define MACRO_INC_MRTC_VPUShared_h

#if defined(TARGET_PS3_SPU) || defined(SN_TARGET_PS3_SPU)
	#include "../VPU/VPU_Platform.h"
#else
	#include "../../Platform/Platform.h"
#endif

#ifndef pure
	#define pure = 0
#endif

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Template:			TAlignmentOf

	Comments:			M_ALIGNMENTOF(Type) is the compile time constant
						memory alignment requirement for objects of type 'Type'.

						ex:
							M_ALIGNMENTOF(int8) = 00000001
							M_ALIGNMENTOF(int16) = 00000002
							M_ALIGNMENTOF(int32) = 00000004
							M_ALIGNMENTOF(int64) = 00000008
							M_ALIGNMENTOF(fp64) = 00000008
							M_ALIGNMENTOF(vec128) = 00000010
							M_ALIGNMENTOF(CVec3Dfp32) = 00000004
							M_ALIGNMENTOF(CVec4Dfp32) = 00000010
							M_ALIGNMENTOF(CMat4Dfp32) = 00000010
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


enum { InvalidVpuTask=0xffff };

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

// ----------------------------------------------------------------
#if defined(USE_SPU_PRINTF) && defined(PLATFORM_SPU)
#if !defined(M_RTM) || defined(M_Profile)
	#define M_TRACEALWAYS(fmt, args...) spu_printf(fmt, ## args)
#else
	#define M_TRACEALWAYS(fmt, args...) 
#endif

#ifndef M_RTM
	#define M_TRACE(fmt, args...) spu_printf(fmt, ## args)
#else
	#define M_TRACE(fmt, args...) 
#endif

#else

#if !defined(M_RTM) || defined(M_Profile)
	#define M_TRACEALWAYS              MRTC_SystemInfo::OS_Trace
#else
	#define M_TRACEALWAYS              1 ? (void)0 : MRTC_SystemInfo::OS_Trace
#endif

#ifndef M_RTM
	#define M_TRACE              MRTC_SystemInfo::OS_Trace
#else
	#define M_TRACE              1 ? (void)0 : MRTC_SystemInfo::OS_Trace
#endif

#endif

// ----------------------------------------------------------------
#ifndef M_RTM
	#ifdef PLATFORM_CONSOLE
		#define M_STATIC_ASSERT(expn) typedef char __C_ASSERT__[(expn)?1:-1] 
		#define M_ASSERT(f, Mess) if(!(f)) { M_BREAKPOINT; MRTC_SystemInfo::OS_Assert(Mess, __FILE__, __LINE__); }
		#define M_ASSERTHANDLER(f, Mess, Action) if (!(f)) { M_BREAKPOINT; MRTC_SystemInfo::OS_Assert(Mess, __FILE__, __LINE__); Action; }
	#else
		#define M_STATIC_ASSERT(expn) typedef char __C_ASSERT__[(expn)?1:-1] 
		#define M_ASSERT(f, Mess) if(!(f)) { MRTC_SystemInfo::OS_Assert(Mess, __FILE__, __LINE__); }
		#define M_ASSERTHANDLER(f, Mess, Action) if (!(f)) { MRTC_SystemInfo::OS_Assert(Mess, __FILE__, __LINE__); Action; }
	#endif
#else
	#define M_STATIC_ASSERT(expn) ;
	#define M_ASSERT(f, Mess) ((void)0)
	#define M_ASSERTHANDLER(f, Mess, Action) ((void)0)
#endif

// -------------------------------------------------------------------
M_FORCEINLINE void M_PRECACHEMEM(const void* _pVBFP, uint _Bytes)
{
	uint nCL = (_Bytes+127) >> 7;
	for(uint i = 0; i < nCL; i++)
		M_PRECACHE128(i << 7, _pVBFP);
}

// ----------------------------------------------------------------
#ifdef M_COMPILING_ON_VPU 

#define ThrowError(_Msg)  M_BREAKPOINT

#ifdef	COMPILER_GNU
M_INLINE void* operator new(mint, void* _pPtr) throw()
{
	return _pPtr;
}
#else
M_INLINE void* operator new(mint, void* _pPtr)
{
	return _pPtr;
}
#endif

#ifdef	COMPILER_GNU
M_INLINE void operator delete(void*, void*)
{
}
#else
M_INLINE void operator delete(void*, void*)
{
}

#endif

#endif

#ifdef MRTC_DLL
	#define MRTCDLLEXPORT __declspec(dllexport)
#else
	#define MRTCDLLEXPORT
#endif


// ----------------------------------------------------------------
template <typename t_CData>
static void DataCopy(t_CData *_pDest, const t_CData *_pSrc, mint _NumberToCopy)
{
	for (uint32 i = 0; i < _NumberToCopy; ++i)
	{
		_pDest[i] = _pSrc[i];
	}
}


M_INLINE void ModerateFloat(fp32& _x, fp32 _newx, fp32& _xprim, fp32 a)
{
	fp32 xbiss = (-fp32(a)*fp32(_xprim) - fp32(_x-_newx)*(fp32(a)*fp32(a))*0.25f);
	_xprim += xbiss;
	_x += _xprim;
}



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

template<class T, class T2, class T3>
T Clamp(T _Value, T2 _Min, T3 _Max)
{
	if(_Value > _Max)
		return _Max;
	if(_Value < _Min)
		return _Min;

	return _Value;
}

template <typename t_CType>
M_INLINE t_CType AlignUp(t_CType _Value, mint _Alignment)
{
	return (t_CType)(((mint)_Value + (_Alignment - 1)) & (~(_Alignment - 1)));
}

template <typename t_CType>
M_INLINE t_CType AlignDown(t_CType _Value, mint _Alignment)
{
	return (t_CType)(((mint)_Value) & (~(_Alignment - 1)));
}



#if defined(TARGET_PS3_SPU) || defined(SN_TARGET_PS3_SPU)
	#include "../VPU/MRTC_System_VPU.h"
#else
	#include "../MRTC_System.h"
#endif

#include "MRTC_VPUShared_Thread.h"

// ----------------------------------------------------------------
#if !defined(PLATFORM_SPU) && defined(M_Profile) && !defined(_DEBUG)

class MRTC_NamedEvent
{
public:
	M_FORCEINLINE MRTC_NamedEvent(const char* _pName, uint32 _Color)
	{
		MRTC_SystemInfo::OS_NamedEvent_Begin(_pName, _Color);
	}

	M_FORCEINLINE ~MRTC_NamedEvent()
	{
		MRTC_SystemInfo::OS_NamedEvent_End();
	}
};

//#define VPU_NAMEDEVENTS
//#define SHADOW_NAMEDEVENTS
//#define CLOTH_NAMEDEVENTS
#define THREADPOOL_NAMEDEVENTS

#define M_NAMEDEVENT(_Name, _Color) MRTC_NamedEvent NamedEvent##__LINE__(_Name, _Color)

#else
#define M_NAMEDEVENT(_Name, _Color) ((void)0)

#endif

// ----------------------------------------------------------------
#endif //MACRO_INC_MRTC_VPUShared_h

