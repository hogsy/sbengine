
#ifndef _INC_VPUTARGET_SPU
#define _INC_VPUTARGET_SPU


#define NULL 0
#define M_ALIGN(_Align) __attribute__((aligned(_Align)))
#define M_ARGLISTCALL
#define M_TRACE(a,b)
#define PLATFORM_SPU
#define M_ASSERT(f, Mess) if(!(f)) { spu_stop(0x3ff); }
#define M_FORCEINLINE inline
//#define M_ASSERT(f, Mess) if(!(f)) { MRTC_System_VPU::OS_Assert(Mess, __FILE__, __LINE__); }


typedef signed char				int8;
typedef unsigned char			uint8;
typedef signed short			int16;
typedef unsigned short			uint16;
typedef signed int				int32;
typedef unsigned int			uint32;
typedef float					fp4;
typedef double					fp8;

typedef vector float			vec128;


#if defined(__LP32__)
	typedef signed long long	int64;
	typedef unsigned long long	uint64;
	typedef signed int			aint;
	typedef unsigned long		auint;
	typedef unsigned int		mint;
#elif defined(__LP64__)
	typedef signed long			int64;
	typedef unsigned long		uint64;
	typedef signed long			aint;
	typedef unsigned long		auint;
	typedef unsigned long		mint;
#else
#error	"moo"
#endif



#endif
