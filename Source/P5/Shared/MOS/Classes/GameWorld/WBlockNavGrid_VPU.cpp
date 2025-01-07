#define M_COMPILING_ON_VPU
#include "VPU/VPU_Platform.h"
#include "VPU/MRTC_VPU.h"


//uint32 aivputest(CVPU_JobInfo& _JobInfo){return 0;}
namespace NNavGridVPU {
uint32 VPUWorker_NavGrid(CVPU_JobInfo &_JobInfo);
};

uint32 VPU_Worker_NavGrid(uint32 _JobHash, CVPU_JobInfo &_JobInfo)
{
	return NNavGridVPU::VPUWorker_NavGrid(_JobInfo);
}

#ifdef PLATFORM_SPU
MVPU_MAIN
MVPU_WORKER_BEGIN
MVPU_WORKER_DECLARE(MHASH2('NAV','GRID'), NNavGridVPU::VPUWorker_NavGrid);
MVPU_WORKER_END
#endif

#include <math.h>

namespace NNavGridVPU {

	#define BLOCKNAV_VPU_COMPILE
	#define BLOCKNAV_VPU_NUMNODES (1024*4)

	#undef new

	#define MAUTOSTRIP(a,b)
	#define MSCOPESHORT(x)
	#define MRTC_DECLARE

	// TODO: Fix these -kma
	template <typename T> inline T Abs(T x) { return (x<0?-x:x); }
	#define Max(a,b) (a>b?a:b)
	#define Max3(a,b,c) (a>b?(a>c?a:c):(b>c?b:c))

	#define _FP32_MAX			3.402823466e+38F
	#define _FP32_MIN			1.175494351e-38F
	#define _FP32_EPSILON		1.192092896e-07F	// Minsta positiva tal sådant att 1.0+x != 1.0
	#define _FP64_MAX			1.7976931348623158e+307
	#define _FP64_MIN			2.2250738585072014e-308
	#define _FP64_EPSILON		2.2204460492503131e-016

	inline const fp32 M_Fabs(fp32 _Val) { return fabsf(_Val); }
	inline fp32 M_Sqrt(fp32 _Val) { return sqrtf(_Val); }

	template <class T> inline T Sqr(T a) { T tmp(a); return tmp*tmp; }

	template <class T>
	T Length2(T a, T b)
	{
		return (T)M_Sqrt(Sqr(a) + Sqr(b));
	};

	template <class T>
	T Length3(T a, T b, T c)
	{
		return (T)M_Sqrt(Sqr(a) + Sqr(b) + Sqr(c));
	};

	inline int RoundToInt( fp32 _Val )
	{
		return (int)(( _Val < 0.0f )? ( _Val - 0.5f ) : ( _Val + 0.5f ));
	}

	inline int TruncToInt( fp32 _Val )
	{
		return (int)_Val;
	}

	inline fp32 Fraction(fp32 _Val)
	{
		return _Val - TruncToInt(_Val);
	}

	inline fp64 Fraction(fp64 _Val)
	{
		return _Val - TruncToInt(_Val);
	}

	inline fp32 M_Floor( fp32 _Val )
	{
		fp32 Value = TruncToInt(_Val);
		fp32 f = Fraction(_Val);

		if( f < 0 )
		{
			Value	-= 1.0f;
		}
		
		return Value;
	}

	inline fp32 M_Ceil( fp32 _Val )
	{
		fp32 Value = TruncToInt(_Val);
		fp32 f = Fraction(_Val);

		if( f > 0 )
		{
			Value	+= 1.0f;
		}
		
		return Value;
	}

	class CReferenceCount {};


	// the actual source
	#ifdef PLATFORM_SPU
		#include <MMath_Vec128.h>
	#endif


	// 
	template<typename TTYPE, int TSIZE>
	class TThinArray_VPU
	{
		TTYPE *m_lData;
		int m_Size;
	public:

		//(SDecompressedTile*)MRTC_System_VPU::OS_LocalHeapAlloc(MAX_DECOMPRESSED_TILES*sizeof(SDecompressedTile));
		TThinArray_VPU()
		{
			m_lData = (TTYPE*)MRTC_System_VPU::OS_LocalHeapAlloc(TSIZE*sizeof(TTYPE));
			m_Size = 0;
		}
		void SetLen(int s) { m_Size = s; }
		int Len() const { return m_Size; }
		void Clear() { m_Size = 0; }
		bool ValidPos(int index) const { return index >= 0 && index < m_Size; }

		TTYPE &operator [] (int index) { return m_lData[index]; }
		const TTYPE &operator [] (int index) const { return m_lData[index]; }

		TTYPE *GetBasePtr() { return m_lData; }
		const TTYPE *GetBasePtr() const { return m_lData; }

		void Delx(int index, int count)
		{
			m_Size -= count;
			for(int i = index; i < m_Size; i++)
				m_lData[i] = m_lData[i+count];
		}
	};


	#include <MDA_PQueue.h>
	#include "VPUWorkerNavGrid.h"
	#include "../../XR/XRBlockNav.inl"
	#include "../../XR/XRBlockNavResult.inl"
}
