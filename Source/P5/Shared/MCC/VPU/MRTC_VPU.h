
#ifndef	__MRTC_VPU_H_INCLUDED
#define	__MRTC_VPU_H_INCLUDED

#define NULL 0

#include "VPU_Platform.h"
#include "../VPUShared/MRTC_VPUShared.h"
#include "../MRTC_VPUCommonDefs.h"
#include "../MMath_Vec128.h"

#include "../MRTC_StringHash.h"
#include "../MRTC_Bit.h"

#define _FP32_MAX			3.402823466e+38F
#define _FP32_MIN			1.175494351e-38F
#define _FP32_EPSILON		1.192092896e-07F	// Minsta positiva tal sådant att 1.0+x != 1.0
#define _FP64_MAX			1.7976931348623158e+307
#define _FP64_MIN			2.2250738585072014e-308
#define _FP64_EPSILON		2.2204460492503131e-016

template <class T>
void Swap(T &a, T &b)
{
	T c;
	c = a;
	a = b; 
	b = c;
};



template <int N>
struct TrailingZeros{
	enum { value = TrailingZeros<N>::value };
};

template <>struct TrailingZeros<1>	{enum { value = 0 };};
template <>struct TrailingZeros<2>	{enum { value = 1 };};
template <>struct TrailingZeros<4>	{enum { value = 2 };};
template <>struct TrailingZeros<8>	{enum { value = 3 };};
template <>struct TrailingZeros<16>	{enum { value = 4 };};
template <>struct TrailingZeros<32>	{enum { value = 5 };};
template <>struct TrailingZeros<64>	{enum { value = 6 };};
template <>struct TrailingZeros<128>{enum { value = 7 };};




union PointerMix
{
	mint m_mint;
	void* m_pvoid;
	uint32* m_puint32;
	uint16* m_puint16;
	uint8* m_puint8;
	int32* m_pint32;
	int16* m_pint16;
	int8* m_pint8;
};

class CVPU_SystemAddr
{
public:
	PointerMix m_Addr;

	M_FORCEINLINE CVPU_SystemAddr() { m_Addr.m_mint = NULL; }
	M_FORCEINLINE CVPU_SystemAddr(mint _Addr) { m_Addr.m_mint=_Addr; }
	M_FORCEINLINE void SetAddr(mint _Addr) { m_Addr.m_mint=_Addr; }
	M_FORCEINLINE operator mint () const { return m_Addr.m_mint; }
	M_FORCEINLINE PointerMix GetP() { return m_Addr; }
};

template <typename T, int VpuElementCount>
class CVPU_InStreamBuffer
{
private:
	enum {ElementSize=sizeof(T)};
	T* m_pBufferA;
	T* m_pBufferB;
	uint32 m_SysElementCount;
	uint16 m_CurrentElement;
	uint32 m_CurrentElementMax;
	CVPU_SystemAddr m_SysAddr;

	uint32 m_TransferedElements;
	uint32 m_ElementsInLastBuffer;
	uint32 m_DmaTagA;		// TODO: also store preshifted
	uint32 m_DmaTagB;

public:
	M_FORCEINLINE CVPU_InStreamBuffer(const CVPU_InStreamBufferInfo& _BufferInfo);
	M_FORCEINLINE ~CVPU_InStreamBuffer() {}
	M_FORCEINLINE mint GetSysElementCount() const { return m_SysElementCount; }
	M_FORCEINLINE mint GetVpuElementCount() const	{ return VpuElementCount;	}
	M_FORCEINLINE mint GetBufferCount() const { return (m_SysElementCount+VpuElementCount-1)/VpuElementCount; }
	M_FORCEINLINE T* GetNextBuffer(uint32& _ElementCount);
	M_FORCEINLINE T* GetNextElement();
	M_FORCEINLINE void SetPosition(uint32 _ElementIndex);
};

template <typename T, int VpuElementCount>
class CVPU_OutStreamBuffer
{
private:
	enum {ElementSize=sizeof(T)};
	T* m_pBufferA;
	T* m_pBufferB;
	mint m_SysElementCount;
	uint16 m_CurrentElement;
	CVPU_SystemAddr m_SysAddr;

	uint32 m_TransferedElements;
	uint32 m_ElementsInLastBuffer;
	uint32 m_DmaTagA;		// TODO: also store preshifted
	uint32 m_DmaTagB;

public:
	M_FORCEINLINE CVPU_OutStreamBuffer(const CVPU_OutStreamBufferInfo& _BufferInfo);
	M_FORCEINLINE ~CVPU_OutStreamBuffer();
	M_FORCEINLINE mint GetVpuElementCount() const	{ return VpuElementCount;	}
	M_FORCEINLINE mint GetBufferCount() const { return (m_SysElementCount+VpuElementCount-1)/VpuElementCount; }
	M_FORCEINLINE T* GetNextBuffer(uint32& _ElementCount);
	M_FORCEINLINE T* GetNextElement();
	void Flush();
};



template <typename T>
class CVPU_SimpleBuffer
{
	enum {ElementSize=sizeof(T)};
	T* m_pBuffer;
	CVPU_SystemAddr m_SysAddr;
	VPUBufferType m_BufferType;
	uint32 m_DmaTag;
	uint32 m_ElementCount;

public:
	M_FORCEINLINE CVPU_SimpleBuffer(const CVPU_SimpleBufferInfo& _BufferInfo);
	M_FORCEINLINE ~CVPU_SimpleBuffer();
	M_FORCEINLINE mint GetElementCount() const	{ return m_ElementCount;	}
	M_FORCEINLINE T* GetBuffer(uint32& _ElementCount);

	CVPU_SystemAddr GetSysAddr()
	{
		return m_SysAddr;
	}
	void Flush();

};


template <typename T, int ElementsPerCacheLine, int CacheSize>
class CVPU_CacheBuffer
{
private:
	enum {ElementSize=sizeof(T)};
	enum {CacheLineSize=ElementSize*ElementsPerCacheLine};
	//enum {CacheSize=128};
	enum {VpuBufferSize=CacheLineSize*CacheSize};

	CVPU_SystemAddr m_SysAddr;
	uint32 m_CacheTags[CacheSize];
	T* m_CacheData;
	uint32 m_DmaTag;
	uint32 m_SysElementCount;

public:
	M_FORCEINLINE CVPU_CacheBuffer(const CVPU_CacheBufferInfo& _BufferInfo);
	M_FORCEINLINE ~CVPU_CacheBuffer();
	M_FORCEINLINE T GetElement(uint32 _Index);
	M_FORCEINLINE uint32 Len() const { return m_SysElementCount; }

	M_FORCEINLINE void Prefetch(uint32 _Index);
	M_FORCEINLINE void WaitForDma();
	M_FORCEINLINE const T* GetPntElement(uint32 _Index);

	M_FORCEINLINE const T &operator[](int32 _Index) { return *GetPntElement(_Index); }
};


class CVPU_JobInfo
{
private:
	const CVPU_JobDefData* m_pJobData;
	void* m_pScratchBuffer;
public:
	M_FORCEINLINE CVPU_JobInfo(const CVPU_JobDefData* _pJobData);
	M_FORCEINLINE ~CVPU_JobInfo();

	M_FORCEINLINE CVPU_SimpleBufferInfo GetSimpleBuffer(mint _ParamIndex) const	{ return m_pJobData->m_Params[_ParamIndex].m_SimpleBufferData; }
	M_FORCEINLINE CVPU_OutStreamBufferInfo GetOutStreamBuffer(mint _ParamIndex) const { return m_pJobData->m_Params[_ParamIndex].m_OutStreamBufferData; }
	M_FORCEINLINE CVPU_InStreamBufferInfo GetInStreamBuffer(mint _ParamIndex) const { return m_pJobData->m_Params[_ParamIndex].m_InStreamBufferData; }
	M_FORCEINLINE CVPU_CacheBufferInfo GetCacheBuffer(mint _ParamIndex) const { return m_pJobData->m_Params[_ParamIndex].m_CacheBufferData; }
	M_FORCEINLINE void* GetScratchBuffer(uint32 _Size);
	M_FORCEINLINE vec128 GetVParam(mint _ParamIndex) const { return m_pJobData->m_Params[_ParamIndex].m_VecData; }
	M_FORCEINLINE uint32 GetLParam(mint _ParamIndex,mint _SubIndex) const { return m_pJobData->m_Params[_ParamIndex].m_LongData[_SubIndex]; }
	M_FORCEINLINE fp32 GetFParam(mint _ParamIndex,mint _SubIndex) const { return m_pJobData->m_Params[_ParamIndex].m_fp32Data[_SubIndex]; }
	M_FORCEINLINE CVPU_SystemAddr GetPntrParam(mint _ParamIndex,mint _SubIndex) const { return m_pJobData->m_Params[_ParamIndex].m_LongData[_SubIndex]; }
};


class MRTC_System_VPU			// Vector Processing Unit
{
public:
	M_FORCEINLINE static void OS_LocalHeapFreeAll();
	M_FORCEINLINE MRTCDLLEXPORT static void* OS_LocalHeapAlloc(uint32 _Size);								// Will 128-byte align address and size
	M_FORCEINLINE static uint32 OS_LocalHeapMemLeft();
	M_FORCEINLINE static int OS_GetDMAChannel();
	M_FORCEINLINE static void OS_ReleaseAllDMAChannels();
//	static void OS_HeapFree(void *_pMem);
//	static uint32 OS_HeapSize(const void *_pMem);
	M_FORCEINLINE static CVPU_SystemAddr OS_VBAlloc(CVPU_SystemAddr _pAllocPos,CVPU_SystemAddr _pLock,CVPU_SystemAddr _ppHeap,CVPU_SystemAddr _pHeapSize,uint32 _Size);
	M_FORCEINLINE static uint32 OS_ReadData(CVPU_SystemAddr _Addr);
	M_FORCEINLINE static void OS_WriteData(CVPU_SystemAddr _Addr,uint32 _Data);
	M_FORCEINLINE static void OS_Assert(const char*, const char* _pFile = NULL, int _Line = 0);

	M_FORCEINLINE static void OS_DMATransferToSys(CVPU_SystemAddr _pDst, const void* _pSrc, mint _Size);
	M_FORCEINLINE static void OS_DMATransferFromSys(void* _pDst, CVPU_SystemAddr _pSrc, mint _Size);
	M_FORCEINLINE static void OS_DMABlockUntilIdle();
	M_FORCEINLINE static void OS_DMABlockOnTransfer();
	M_FORCEINLINE static void OS_DMABlockOnTransferAll();
	M_FORCEINLINE static bool OS_DMATryBlockUntilIdle();
	M_FORCEINLINE static bool OS_DMATryBlockOnTransfer();
	M_FORCEINLINE static bool OS_DMATryBlockOnTransferAll();
};


class VertexCache
{
public:
	enum { CacheSize=128 };
	VertexCache()
	{
		for (uint i=0;i<CacheSize;i++)
		{
			m_VertexIndex[i]=0xffffffff;
		}
	}
	M_FORCEINLINE bool getVertex(uint32 _Index,vec128& _Vertex)
	{
		const uint32 CacheIndex=_Index & (CacheSize-1);
		if (_Index!=m_VertexIndex[CacheIndex])
			return false;
		else
		{
			_Vertex=m_VertexCache[CacheIndex];
			return true;
		}
	}
	M_FORCEINLINE void setVertex(uint32 _Index,vec128 _Vertex)
	{
		const uint32 CacheIndex=_Index & (CacheSize-1);
		m_VertexCache[CacheIndex]=_Vertex;
		m_VertexIndex[CacheIndex]=_Index;
	}
private:
	uint32 m_VertexIndex[CacheSize];
	vec128 m_VertexCache[CacheSize];
};



uint32 VPU_Worker(uint32 _JobHash,CVPU_JobInfo& _JobInfo);

/*
#define MVPU_WORKER(func)	\
	void cellSpursJobMain(CellSpursJobContext* stInfo)	\
{	\
	CVPU_JobInfo JobInfo(stInfo);	\
	func(JobInfo);					\
}
*/

/*
class CVPU_FStr
{
char m_data[64];
};
*/

#define MVPU_WORKER_BEGIN	\
	uint32 VPU_Worker(uint32 _JobHash,CVPU_JobInfo& _JobInfo)	\
{	\
	switch(_JobHash)	\
{

#define MVPU_WORKER_END \
		default :\
		M_TRACE("Unrecognized job. JobHash: %d\n", _JobHash);	\
}	\
	return 0;	\
}


#define MVPU_WORKER_DECLARE(Name, Func)	\
		case Name : return Func(_JobInfo);


#ifdef PLATFORM_SPU
	#include "MRTC_VPU_PS3.h"
#else 
	#include "MRTC_VPU_WIN32.h"
#endif

#endif
