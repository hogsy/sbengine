
#ifndef _INC_VPUTARGET_SPU
#define _INC_VPUTARGET_SPU


#define NULL 0
#define M_ALIGN(_Align) __attribute__((aligned(_Align)))
#define M_ARGLISTCALL
#define PLATFORM_SPU
#define CPU_BIGENDIAN 0x0001
#define M_FORCEINLINE inline
#define M_INLINE inline
#define COMPILER_GNU
#define M_IMPORTBARRIER
#define M_EXPORTBARRIER
#define M_BREAKPOINT spu_stop(0x3fff)
#define M_SCOPESHORT(str)
//#define M_ASSERT(f, Mess) if(!(f)) { MRTC_System_VPU::OS_Assert(Mess, __FILE__, __LINE__); }
#define M_TRY
#define M_CATCH(_ToCatch)

#define M_OFFSET(_Class, _Member, _Dest) aint _Dest;\
			{\
			const _Class *pPtr = 0;\
			_Dest = (aint)(void *)(&((pPtr)->_Member));\
			}


#define M_RESTRICT __restrict
#define M_PREZERO128(a, b)
//#define M_ZERO128(a, b)
#define M_ZERO128(_x, _y) \
{\
	mint Addr = ((mint)_x + (mint)_y) & (~mint(15)); \
	vec128 Zero = spu_splats(0.0f);\
	((vec128 *)(Addr))[0] = Zero;\
	((vec128 *)(Addr))[1] = Zero;\
	((vec128 *)(Addr))[2] = Zero;\
	((vec128 *)(Addr))[3] = Zero;\
	((vec128 *)(Addr))[4] = Zero;\
	((vec128 *)(Addr))[5] = Zero;\
	((vec128 *)(Addr))[6] = Zero;\
	((vec128 *)(Addr))[7] = Zero;\
}

#define M_PRECACHE128(a, b)
#define USE_SPU_PRINTF

typedef signed char				int8;
typedef unsigned char			uint8;
typedef signed short			int16;
typedef unsigned short			uint16;
typedef signed int				int32;
typedef unsigned int			uint32;
typedef float					fp32;
typedef double					fp64;
typedef int						bint;
typedef char ch8;

typedef vector float			vec128;


#if defined(__LP32__)
	typedef signed long long	int64;
	typedef unsigned long long	uint64;
	typedef signed int			aint;
	typedef unsigned long		auint;
	typedef long unsigned int	mint;
#elif defined(__LP64__)
	typedef signed long			int64;
	typedef unsigned long		uint64;
	typedef signed long			aint;
	typedef unsigned long		auint;
	typedef long unsigned int	mint;
#else
#error	"moo"
#endif

#include "spu_intrinsics.h"

typedef unsigned int			uint;

#ifdef USE_SPU_PRINTF
	#include <spu_printf.h>
#else
#endif

#endif

