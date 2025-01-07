
#include "../MRTC_VPUCommonDefs.h"
#include "MRTC_VPU_Win32_Helper.h"


class CVPU_WorkQueue
{
public:
	inline static void GetEvent(CVPU_ContextData& _ContextData);
	inline static CVPU_JobDefData* GetJob(CVPU_ContextData& _ContextData);
	inline static void SignalJobComplete(CVPU_ContextData& _Data,uint32 _JobIndex);
};

CVPU_JobDefData* CVPU_WorkQueue::GetJob(CVPU_ContextData& _ContextData)
{
	MRTC_VPU_Win32__MRTC_SpinLock_Lock(_ContextData.m_pLock);
	CVPU_JobDefData* ret=_ContextData.Get();
	MRTC_VPU_Win32__MRTC_SpinLock_Unlock(_ContextData.m_pLock);
	return ret;
}


void CVPU_WorkQueue::SignalJobComplete(CVPU_ContextData& _ContextData,uint32 _JobIndex)
{
	MRTC_VPU_Win32__MRTC_SpinLock_Lock(_ContextData.m_pLock);
	_ContextData.m_pJobs[_JobIndex]=CVPU_ContextData::JobDone;
	M_EXPORTBARRIER;
	MRTC_VPU_Win32__MRTC_SpinLock_Unlock(_ContextData.m_pLock);
}


CVPU_SystemAddr MRTC_System_VPU::OS_VBAlloc(CVPU_SystemAddr _pAllocPos,CVPU_SystemAddr _pLock,CVPU_SystemAddr _ppHeap,CVPU_SystemAddr _pHeapSize,uint32 _Size)
{
	MRTC_VPU_Win32__NThread_CSpinLock_Lock(_pLock.GetP().m_pvoid);
	CVPU_SystemAddr ret=0;
	int32& AllocPos=*_pAllocPos.GetP().m_pint32;
	_Size = (_Size + 15) & ~15;
	if (int32(AllocPos + _Size) <= *_pHeapSize.GetP().m_pint32)
	{
		ret=CVPU_SystemAddr((*_ppHeap.GetP().m_pint32)+AllocPos);
		AllocPos += _Size;
	}
	MRTC_VPU_Win32__NThread_CSpinLock_Unlock(_pLock.GetP().m_pvoid);
	return ret;
}


#define MVPU_MAIN

CVPU_JobInfo::CVPU_JobInfo(const CVPU_JobDefData* _pJobData)
:	m_pJobData(_pJobData)
{
}

CVPU_JobInfo::~CVPU_JobInfo()
{
}


uint32 MRTC_System_VPU::OS_ReadData(CVPU_SystemAddr _Addr)
{
	return *((uint32*)mint(_Addr));
}


void MRTC_System_VPU::OS_WriteData(CVPU_SystemAddr _Addr,uint32 _Data)
{
	*(uint32*)mint(_Addr)=_Data;
}

uint32 MRTC_System_VPU::OS_LocalHeapMemLeft()
{
	return 0;
}

void* MRTC_System_VPU::OS_LocalHeapAlloc(uint32 _Size)
{
	// TODO: Add check for out of memory

	/*
	M_ASSERT(_Size<OS_LocalHeapMemLeft(),"Out of memory");
	if(_Size>OS_LocalHeapMemLeft())
		M_TRACE("Out of memory!!\r\n", OS_LocalHeapMemLeft(), _Size);
	*/

	char *pScratch = (char *)MRTC_VPU_Win32__GetScratchBuffer(256*1024);
	uint32 &Offset = *((uint32*)pScratch);
	uint32 SizeNeeded = (_Size+127) & ~127;
	char *pRet = pScratch+128+Offset;
	Offset += SizeNeeded;
	if(Offset > 256*1024-128)
		M_BREAKPOINT;
	return (void*)pRet;
}


void MRTC_System_VPU::OS_LocalHeapFreeAll()
{
	char *pScratch = (char *)MRTC_VPU_Win32__GetScratchBuffer(256*1024);
	*((uint32*)pScratch) = 0;
	//pMem=(uint32)_end;
}


template <typename T, int ElementsPerCacheLine, int CacheSize> 
CVPU_CacheBuffer<T,ElementsPerCacheLine, CacheSize>::CVPU_CacheBuffer(const CVPU_CacheBufferInfo& _BufferInfo)
:	m_SysAddr((mint) _BufferInfo.m_SysAddr),
	m_SysElementCount(_BufferInfo.m_SysElementCount)
{
}

template <typename T, int ElementsPerCacheLine, int CacheSize> 
CVPU_CacheBuffer<T,ElementsPerCacheLine, CacheSize>::~CVPU_CacheBuffer()
{
}

template <typename T, int ElementsPerCacheLine, int CacheSize> 
T CVPU_CacheBuffer<T,ElementsPerCacheLine, CacheSize>::GetElement(uint32 _Index)
{
	return *((T*)(m_SysAddr+_Index*sizeof(T)));
}


template <typename T, int ElementsPerCacheLine, int CacheSize> 
const T* CVPU_CacheBuffer<T,ElementsPerCacheLine, CacheSize>::GetPntElement(uint32 _Index)
{
	return (T*)(m_SysAddr+_Index*ElementSize);
}

template <typename T, int ElementsPerCacheLine, int CacheSize> 
void CVPU_CacheBuffer<T,ElementsPerCacheLine, CacheSize>::Prefetch(uint32 _Index)
{
}

template <typename T, int ElementsPerCacheLine, int CacheSize> 
void CVPU_CacheBuffer<T,ElementsPerCacheLine, CacheSize>::WaitForDma()
{
}


template <typename T, int VpuElementCount>
CVPU_InStreamBuffer<T,VpuElementCount>::CVPU_InStreamBuffer(const CVPU_InStreamBufferInfo& _BufferInfo)
:	m_pBufferA(NULL),
	m_pBufferB(NULL),
	m_SysElementCount(_BufferInfo.m_SysElementCount),
	m_CurrentElement(0),
	m_SysAddr(mint(_BufferInfo.m_SysAddr)),
	m_TransferedElements(0),
	m_ElementsInLastBuffer(0)
{
}

template <typename T, int VpuElementCount>
T* CVPU_InStreamBuffer<T,VpuElementCount>::GetNextBuffer(uint32& _ElementCount)
{
	mint pElements=m_SysAddr+m_TransferedElements*ElementSize;
	_ElementCount=Min(uint32(VpuElementCount),m_SysElementCount-m_TransferedElements);
	m_TransferedElements+=_ElementCount;
	return (T*) pElements;
}

template <typename T, int VpuElementCount>
void CVPU_InStreamBuffer<T,VpuElementCount>::SetPosition(uint32 _ElementIndex)
{
	M_ASSERT(_ElementIndex<m_SysElementCount,"");
	m_CurrentElement=_ElementIndex;
}

template <typename T, int VpuElementCount>
T* CVPU_InStreamBuffer<T,VpuElementCount>::GetNextElement()
{
	return (T*)(m_SysAddr+(m_CurrentElement++)*ElementSize);
}



template <typename T, int VpuElementCount>
CVPU_OutStreamBuffer<T,VpuElementCount>::CVPU_OutStreamBuffer(const CVPU_OutStreamBufferInfo& _BufferInfo)
:	m_pBufferA(NULL),
	m_pBufferB(NULL),
	m_SysElementCount(_BufferInfo.m_SysElementCount),
	m_CurrentElement(0),
	m_SysAddr(mint(_BufferInfo.m_SysAddr)),
	m_TransferedElements(0),
	m_ElementsInLastBuffer(0)
{
}

template <typename T, int VpuElementCount>
CVPU_OutStreamBuffer<T,VpuElementCount>::~CVPU_OutStreamBuffer()
{
}



template <typename T, int VpuElementCount>
T* CVPU_OutStreamBuffer<T,VpuElementCount>::GetNextBuffer(uint32& _ElementCount)
{
	mint pElements=m_SysAddr+m_TransferedElements*ElementSize;
	_ElementCount=Min(uint32(VpuElementCount),m_SysElementCount-m_TransferedElements);
	m_TransferedElements+=_ElementCount;
	return (T*) pElements;
}

template <typename T, int VpuElementCount>
T* CVPU_OutStreamBuffer<T,VpuElementCount>::GetNextElement()
{
	M_ASSERT(m_CurrentElement<m_SysElementCount,"Trying to write outside buffer");
	return (T*)(m_SysAddr+(m_CurrentElement++)*ElementSize);
}


template <typename T, int VpuElementCount>
void CVPU_OutStreamBuffer<T,VpuElementCount>::Flush()
{
}


template <typename T> 
CVPU_SimpleBuffer<T>::CVPU_SimpleBuffer(const CVPU_SimpleBufferInfo& _BufferInfo)
:	m_pBuffer(NULL),
	m_SysAddr(mint(_BufferInfo.m_SysAddr)),
	m_BufferType(_BufferInfo.m_VPUBufferType),
	m_ElementCount(_BufferInfo.m_ElementCount)
{
}

template <typename T> 
CVPU_SimpleBuffer<T>::~CVPU_SimpleBuffer()
{
}


template <typename T> 
T* CVPU_SimpleBuffer<T>::GetBuffer(uint32& _ElementCount)
{
	_ElementCount=m_ElementCount;
	return (T*) mint(m_SysAddr);
}


