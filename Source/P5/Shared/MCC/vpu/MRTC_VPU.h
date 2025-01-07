
#ifndef	__MRTC_VPU_H_INCLUDED
#define	__MRTC_VPU_H_INCLUDED

#include "VPU_Platform.h"
#include "../MRTC_VPUCommonDefs.h"
#ifdef PLATFORM_SPU
#include "../MRTC_StringHash.h"
template <class T>
void Swap(T &a, T &b)
{
	T c;
	c = a;
	a = b; 
	b = c;
};

template <typename T> 
inline T Max(T a, T b)
{
	return ((a > b) ? a : b);
};

template <typename T> 
inline T Min(T a, T b)
{
	return ((a < b) ? a : b);
};


#endif



class CVPU_SystemAddr
{
public:
	mint m_Addr;

	CVPU_SystemAddr() { m_Addr = NULL; }
	CVPU_SystemAddr(mint _Addr) : m_Addr(_Addr) {}
	void SetAddr(mint _Addr) { m_Addr=_Addr; }
	operator mint () const { return m_Addr; }
};




class CVPU_StreamBuffer
{
private:
	enum { MAX_BUFFER_ID=15 };
	void* m_pBufferA;
	void* m_pBufferB;
	mint m_SysElementCount;
	uint16 m_VpuElementCount;
	uint16 m_ElementSize;
	CVPU_SystemAddr m_SysAddr;
	VPUBufferType m_BufferType;

	uint32 m_TransferedElements;
	uint32 m_ElementsInLastBuffer;
	uint32 m_DmaTagA;		// TODO: also store preshifted
	uint32 m_DmaTagB;
	bool m_IsDoubleBuffered;


public:
	CVPU_StreamBuffer(CVPU_StreamBufferInfo& _BufferInfo,uint32 _BufferId);
	~CVPU_StreamBuffer();
	mint CVPU_StreamBuffer::GetVpuElementCount() const	{ return m_VpuElementCount;	}
	mint GetBufferCount() const { return (m_SysElementCount+m_VpuElementCount-1)/m_VpuElementCount; }
	void* GetNextBuffer(uint32& _ElementCount);
	void* GetBuffer(uint32& _ElementCount);
	void SetPosition(uint32 _ElementIndex);
	void Flush();

};


class CVPU_CacheBuffer
{
private:
	enum {CacheSize=128};
	CVPU_SystemAddr m_SysAddr;
	uint32 m_CacheTags[CacheSize];
	uint8* m_CacheData;
	uint32 m_VpuBufferSize;
	uint32 m_DmaTag;
	uint32 m_ElementSize;
	uint32 m_SysElementCount;

public:
	CVPU_CacheBuffer(CVPU_CacheBufferInfo& _BufferInfo,uint32 _BufferId);
	~CVPU_CacheBuffer();
	vec128 GetVecElement(uint32 _Index);
	const uint8* GetPntElement(uint32 _Index);

};


class CVPU_JobInfo
{
private:
	CVPU_JobDefData* m_pJobData;
	void* m_pScratchBuffer;
public:
	CVPU_JobInfo(CVPU_JobDefData* _pJobData);
	~CVPU_JobInfo();

	CVPU_StreamBufferInfo GetStreamBuffer(mint _ParamIndex) { return m_pJobData->m_Params[_ParamIndex].m_StreamBufferData; }
	CVPU_CacheBufferInfo GetCacheBuffer(mint _ParamIndex) { return m_pJobData->m_Params[_ParamIndex].m_CacheBufferData; }
	void GetScratchBufferInfo(void*& _Addr,uint32& _Size) 	{ _Size=m_pJobData->m_ScratchSize; _Addr=m_pScratchBuffer; }
	vec128 GetVParam(mint _ParamIndex) { return m_pJobData->m_Params[_ParamIndex].m_VecData; }
	uint32 GetLParam(mint _ParamIndex,mint _SubIndex) { return m_pJobData->m_Params[_ParamIndex].m_LongData[_SubIndex]; }

	mint GetInOutBufferSize();
	mint GetInputBufferSize();
	mint GetOutputBufferSize();
	void SendDownOutputData();
};


class MRTC_System_VPU			// Vector Processing Unit
{
public:
	static void OS_HeapFreeAll();
	static void* OS_HeapAlloc(uint32 _Size);								// Will 128-byte align address and size
	static void OS_HeapFree(void *_pMem);
	static uint32 OS_HeapSize(const void *_pMem);

	static void OS_Assert(const char*, const char* _pFile = NULL, int _Line = 0);
	static void M_ARGLISTCALL OS_Trace(const char *, ...);
	static void OS_TraceRaw(const char *);


	static void OS_DMATransferToSys(void* _pSrc, CVPU_SystemAddr _pDst, mint _Size);
	static void OS_DMATransferFromSys(CVPU_SystemAddr _pSrc, void* _pDst, mint _Size);
	static void OS_DMABlockUntilIdle();
	static void OS_DMABlockOnTransfer();
	static void OS_DMABlockOnTransferAll();
	static bool OS_DMATryBlockUntilIdle();
	static bool OS_DMATryBlockOnTransfer();
	static bool OS_DMATryBlockOnTransferAll();
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
		M_TRACE("Unrecognized job. JobHash: %d", _JobHash);	\
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
