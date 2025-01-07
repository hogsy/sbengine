#include <sys/spu_event.h>
#include <cell/dma.h>
#include <cell/atomic.h>



extern "C" char _end[];
extern "C" char __stack[];

uint32 pMem=(uint32)_end;
uint32 pMemOriginal=(uint32)_end;
uint32 DmaChannel=1;
uint32 g_SPUEventPort = 0;
uint32 g_SPUEventQueueNumber = 0;
uint32 g_StackSize = 16*1024;

void SpinLockLock(CVPU_SystemAddr _pLock)
{
	uint32 dummybuf[128/sizeof(uint32)] __attribute__((aligned(128)));
	uint32 ret=1;
	while (ret!=0) {
		ret=cellAtomicCompareAndSwap32(dummybuf,_pLock,0,1);
	}
}

void SpinLockUnlock(CVPU_SystemAddr _pLock)
{
	uint32 dummybuf[128/sizeof(uint32)] __attribute__((aligned(128)));
	cellAtomicDecr32(dummybuf,_pLock);
}

class CVPU_WorkQueue
{
public:
	static void GetEvent(CVPU_ContextData& _ContextData,uint32 _SpuEventQueueNumber);
	static void SignalVpuIdle(CVPU_ContextData& _Data);
	static bool GetJob(CVPU_ContextData& _ContextData,CVPU_JobDefData& _JobDefData,uint32 _SpuEventQueueNumber);
	static void SignalJobComplete(CVPU_ContextData& _Data,uint16 _JobIndex);
	static void CallPPU(uint32 _Message, uint32 _Context, uint32 &_Ret0, uint32 &_Ret1);
};

CVPU_JobInfo::CVPU_JobInfo(const CVPU_JobDefData* _pJobData)
:	m_pJobData(_pJobData)
{
}

void* CVPU_JobInfo::GetScratchBuffer(uint32 _Size)
{
	return MRTC_System_VPU::OS_LocalHeapAlloc(_Size);
}

CVPU_JobInfo::~CVPU_JobInfo(){}

int VPU_Main(uint32 _SpuEventQueueNumber,uint32 _SpuEventPort)
{
	g_SPUEventPort = _SpuEventPort;
	g_SPUEventQueueNumber = _SpuEventQueueNumber;
	CVPU_ContextData ManagerData;
	CVPU_JobDefData JobDefData;
	while(1)
	{
		CVPU_WorkQueue::GetEvent(ManagerData,_SpuEventQueueNumber);
		while (CVPU_WorkQueue::GetJob(ManagerData,JobDefData,_SpuEventQueueNumber))
		{
			CVPU_JobInfo JobInfo(&JobDefData);
			VPU_Worker(JobDefData.m_JobHash,JobInfo);
			MRTC_System_VPU::OS_LocalHeapFreeAll();
			MRTC_System_VPU::OS_ReleaseAllDMAChannels();
			CVPU_WorkQueue::SignalJobComplete(ManagerData,JobDefData.m_JobId);
		}
		CVPU_WorkQueue::SignalVpuIdle(ManagerData);
	}
	return 0;
}

#define MVPU_MAIN \
int main(uint32 _SpuEventQueueNumber,uint32 _SpuEventPort)	\
{	\
	VPU_Main(_SpuEventQueueNumber,_SpuEventPort);	\
}

int32 MRTC_SystemInfo::Atomic_Increase(volatile int32 *_pDest)
{
	uint32 M_ALIGN(128) dummybuf[32];
	return cellAtomicIncr32(dummybuf,(uint64_t)_pDest);
}

int32 MRTC_SystemInfo::Atomic_Decrease(volatile int32 *_pDest)
{
	uint32 M_ALIGN(128) dummybuf[32];
	return cellAtomicDecr32(dummybuf,(uint64_t)_pDest);
}

int32 MRTC_SystemInfo::Atomic_Exchange(volatile int32 *_pDest, int32 _SetTo)
{
	uint32 M_ALIGN(128) dummybuf[32];
	return cellAtomicDecr32(dummybuf,(uint64_t)_pDest);
}

int32 MRTC_SystemInfo::Atomic_Add(volatile int32 *_pDest, int32 _Add)
{
	uint32 M_ALIGN(128) dummybuf[32];
	return cellAtomicAdd32(dummybuf, (uint64_t)_pDest, _Add);
}

int32 MRTC_SystemInfo::Atomic_IfEqualExchange(volatile int32 *_pDest, int32 _CompareTo, int32 _SetTo)
{
	uint32 M_ALIGN(128) dummybuf[32];
	return cellAtomicCompareAndSwap32(dummybuf, (uint64_t)_pDest, _CompareTo, _SetTo);
}

void MRTC_SystemInfo::OS_Assert(const char*, const char* _pFile, int _Line)
{
	spu_stop(0x3fff);
}

void M_ARGLISTCALL MRTC_SystemInfo::OS_Trace(const char *_Format, ...)
{
}

void MRTC_SystemInfo::OS_TraceRaw(const char *_pToTrace)
{
#ifdef USE_SPU_PRINTF
	spu_printf("%s", _pToTrace);
#endif
}




void MRTC_System_VPU::OS_Assert(const char* msg, const char* _pFile, int _Line)
{
	spu_stop(0x3fff);
}

void* MRTC_System_VPU::OS_LocalHeapAlloc(uint32 _Size)
{
	M_ASSERT(_Size<OS_LocalHeapMemLeft(),"Out of memory");
	if(_Size>OS_LocalHeapMemLeft())
		M_TRACE("Out of memory!!\r\n", OS_LocalHeapMemLeft(), _Size);
	pMem=(pMem+127) & ~127;
	uint32 ret=pMem;
	pMem+=_Size;
	return (void*) ret;
}


void MRTC_System_VPU::OS_LocalHeapFreeAll()
{
	pMem=(uint32)_end;
}


uint32 MRTC_System_VPU::OS_LocalHeapMemLeft()
{
	return (uint32)240*1024-(uint32)pMem;
}

int MRTC_System_VPU::OS_GetDMAChannel()
{
	return DmaChannel++;
}

void MRTC_System_VPU::OS_ReleaseAllDMAChannels()
{
	DmaChannel=1;
}


void MRTC_System_VPU::OS_DMABlockUntilIdle()
{
	const uint32 DmaTag=0;
	cellDmaWaitTagStatusAll(1<<DmaTag); 
}


CVPU_SystemAddr MRTC_System_VPU::OS_VBAlloc(CVPU_SystemAddr _pAllocPos,CVPU_SystemAddr _pLock,CVPU_SystemAddr _ppHeap,CVPU_SystemAddr _pHeapSize,uint32 _Size)
{

	SpinLockLock(_pLock);
	_Size = (_Size + 15) & ~15;

	int32 AllocPosData[4];
	int32& AllocPos=AllocPosData[(_pAllocPos>>2) & 3];
	cellDmaSmallGet(&AllocPos,_pAllocPos,4,0,0,0);

	int32 pHeapData[4];
	int32& pHeap=pHeapData[(_ppHeap>>2) & 3];
	cellDmaSmallGet(&pHeap,_ppHeap,4,0,0,0);

	int32 HeapSizeData[4];
	int32& HeapSize=HeapSizeData[(_pHeapSize>>2) & 3];
	cellDmaSmallGet(&HeapSize,_pHeapSize,4,0,0,0);
	cellDmaWaitTagStatusAll(1<<0); 


	CVPU_SystemAddr ret;
	if (int32(AllocPos + _Size) <= HeapSize)
	{
		ret=CVPU_SystemAddr(pHeap+AllocPos);
		AllocPos += _Size;
		cellDmaSmallPut(&AllocPos,_pAllocPos,4,0,0,0);
		cellDmaWaitTagStatusAll(1<<0); 
	}
	else
		ret=0;
	SpinLockUnlock(_pLock);
	return ret;
}

uint32 MRTC_System_VPU::OS_ReadData(CVPU_SystemAddr _Addr)
{
	int32 DataBlock[4];
	int32& RealData=DataBlock[(_Addr>>2) & 3];
	cellDmaSmallGet(&RealData,_Addr,4,0,0,0);
	cellDmaWaitTagStatusAll(1<<0);
	return RealData;
}


void MRTC_System_VPU::OS_WriteData(CVPU_SystemAddr _Addr,uint32 _Data)
{
	int32 DataBlock[4];
	int32& RealData=DataBlock[(_Addr>>2) & 3];
	RealData=_Data;
	cellDmaSmallPut(&RealData,_Addr,4,0,0,0);
	cellDmaWaitTagStatusAll(1<<0);
}


void MRTC_System_VPU::OS_DMATransferToSys(CVPU_SystemAddr _pDst, const void* _pSrc, mint _Size)
{
	cellDmaLargePut(_pSrc,_pDst,_Size,0,0,0);
	cellDmaWaitTagStatusAll(1<<0);
}

void MRTC_System_VPU::OS_DMATransferFromSys(void* _pDst, CVPU_SystemAddr _pSrc, mint _Size)
{
	cellDmaLargeGet(_pDst, _pSrc,_Size,0,0,0);
	cellDmaWaitTagStatusAll(1<<0);
}

template <typename T, int ElementsPerCacheLine, int CacheSize> 
CVPU_CacheBuffer<T,ElementsPerCacheLine,CacheSize>::CVPU_CacheBuffer(const CVPU_CacheBufferInfo& _BufferInfo)
:	m_SysAddr((mint) _BufferInfo.m_SysAddr),
	m_SysElementCount(_BufferInfo.m_SysElementCount)
{
	m_CacheData=(T*) MRTC_System_VPU::OS_LocalHeapAlloc(VpuBufferSize);
	M_ASSERT(m_CacheData,"Out of LS memory");
	for (int i=0;i<CacheSize;i++)
		m_CacheTags[i]=0xffffffff;
}


template <typename T, int ElementsPerCacheLine, int CacheSize> 
CVPU_CacheBuffer<T,ElementsPerCacheLine,CacheSize>::~CVPU_CacheBuffer()
{
}

template <typename T, int ElementsPerCacheLine, int CacheSize> 
T CVPU_CacheBuffer<T,ElementsPerCacheLine,CacheSize>::GetElement(uint32 _Index)
{
	return *(T*)(GetPntElement(_Index));
}
/*
template <typename T, int ElementsPerCacheLine> 
vec128 CVPU_CacheBuffer<vec128,ElementsPerCacheLine>::GetElement(uint32 _Index)
{
	return M_VLdMem((void*)GetPntElement(_Index));
}
*/


template <typename T, int ElementsPerCacheLine, int CacheSize> 
const T* CVPU_CacheBuffer<T,ElementsPerCacheLine,CacheSize>::GetPntElement(uint32 _Index)
{
	M_ASSERT(_Index<m_SysElementCount,"Index out of range");
	const uint32 CacheLineIndex=_Index >> TrailingZeros<ElementsPerCacheLine>::value;
	const uint32 CacheIndex=CacheLineIndex&(CacheSize-1);
	uint8* CacheData=(uint8*)m_CacheData+CacheIndex*CacheLineSize;
	if (m_CacheTags[CacheIndex]!=CacheLineIndex)
	{
		const uint32 DmaTag=0;
		cellDmaLargeGetf(CacheData,m_SysAddr+CacheLineIndex*CacheLineSize,CacheLineSize,DmaTag,0,0);
		m_CacheTags[CacheIndex]=CacheLineIndex;
		cellDmaWaitTagStatusAll(1<<DmaTag); 
	}
	return (T*)(CacheData+ElementSize*(_Index & (ElementsPerCacheLine-1)));
}


/*
void CVPU_CacheBuffer::Prefetch(uint32 _Index)
{
	M_ASSERT(_Index<m_SysElementCount,"Index out of range");
	const uint32 CacheLineIndex=_Index/(m_CacheLineSize/m_ElementSize);
	const uint32 CacheIndex=CacheLineIndex&(CacheSize-1);
	uint8* CacheData=m_CacheData+CacheIndex*m_CacheLineSize;
	if (m_CacheTags[CacheIndex]!=_Index)
	{
		const uint32 DmaTag=0;
		cellDmaLargeGetf(CacheData,m_SysAddr+CacheLineIndex*m_CacheLineSize,m_CacheLineSize,DmaTag,0,0);
		m_CacheTags[CacheIndex]=CacheLineIndex;
	}
}

void CVPU_CacheBuffer::WaitForDma()
{
	const uint32 DmaTag=0;
	cellDmaWaitTagStatusAll(1<<DmaTag); 
}
*/



template <typename T, int VpuElementCount>
CVPU_InStreamBuffer<T,VpuElementCount>::CVPU_InStreamBuffer(const CVPU_InStreamBufferInfo& _BufferInfo)
:	m_pBufferA(NULL),
	m_pBufferB(NULL),
	m_SysElementCount(_BufferInfo.m_SysElementCount),
	m_CurrentElement(0),
	m_CurrentElementMax(0),
	m_SysAddr(mint(_BufferInfo.m_SysAddr)),
	m_TransferedElements(0),
	m_ElementsInLastBuffer(0),
	m_DmaTagA(MRTC_System_VPU::OS_GetDMAChannel()),
	m_DmaTagB(MRTC_System_VPU::OS_GetDMAChannel())
{
	m_pBufferA=(T*) MRTC_System_VPU::OS_LocalHeapAlloc(VpuElementCount*ElementSize*2);
	M_ASSERT(m_pBufferA,"Out of LS memory");
	m_pBufferB=m_pBufferA+VpuElementCount;
	m_ElementsInLastBuffer=Min(uint32(VpuElementCount),m_SysElementCount);
	cellDmaLargeGet(m_pBufferA,m_SysAddr,ElementSize*m_ElementsInLastBuffer,m_DmaTagA,0,0);
	m_TransferedElements=m_ElementsInLastBuffer;
}

template <typename T, int VpuElementCount>
T* CVPU_InStreamBuffer<T,VpuElementCount>::GetNextBuffer(uint32& _ElementCount)
{
	T* pElements=m_pBufferA;
	const uint32 ElementsInNextBuffer=Min(uint32(VpuElementCount),m_SysElementCount-m_TransferedElements);
	_ElementCount=m_ElementsInLastBuffer;
	cellDmaWaitTagStatusAll(1<<m_DmaTagA); 
	Swap(m_DmaTagA,m_DmaTagB);
	Swap(m_pBufferA,m_pBufferB);
	if (ElementsInNextBuffer)
		cellDmaLargeGet(m_pBufferA,m_SysAddr+m_TransferedElements*ElementSize,ElementSize*ElementsInNextBuffer,m_DmaTagA,0,0);
	m_TransferedElements+=ElementsInNextBuffer;
	m_ElementsInLastBuffer=ElementsInNextBuffer;
	return (T*)((char*)pElements);
}


template <typename T, int VpuElementCount>
void CVPU_InStreamBuffer<T,VpuElementCount>::SetPosition(uint32 _ElementIndex)
{
	const uint32 subIndex=_ElementIndex&(VpuElementCount-1);

	m_TransferedElements=_ElementIndex-subIndex;
	m_ElementsInLastBuffer=Min(uint32(VpuElementCount),m_SysElementCount-m_TransferedElements);
	cellDmaLargeGetf(m_pBufferA,m_SysAddr+m_TransferedElements*ElementSize,ElementSize*m_ElementsInLastBuffer,m_DmaTagA,0,0);
	m_TransferedElements+=m_ElementsInLastBuffer;
	m_CurrentElement=subIndex;
	if (m_CurrentElement)
		GetNextBuffer(m_CurrentElementMax);
}

template <typename T, int VpuElementCount>
T* CVPU_InStreamBuffer<T,VpuElementCount>::GetNextElement()
{
	if (m_CurrentElement==m_CurrentElementMax)
		m_CurrentElement=0;
	if (m_CurrentElement==0)
		GetNextBuffer(m_CurrentElementMax);
	return m_pBufferB+m_CurrentElement++;
}


template <typename T, int VpuElementCount>
CVPU_OutStreamBuffer<T,VpuElementCount>::CVPU_OutStreamBuffer(const CVPU_OutStreamBufferInfo& _BufferInfo)
:	m_pBufferA(NULL),
	m_pBufferB(NULL),
	m_SysElementCount(_BufferInfo.m_SysElementCount),
	m_CurrentElement(0),
	m_SysAddr(mint(_BufferInfo.m_SysAddr)),
	m_TransferedElements(0),
	m_ElementsInLastBuffer(0),
	m_DmaTagA(MRTC_System_VPU::OS_GetDMAChannel()),
	m_DmaTagB(MRTC_System_VPU::OS_GetDMAChannel())
{
	m_pBufferA=(T*) MRTC_System_VPU::OS_LocalHeapAlloc(VpuElementCount*ElementSize*2);
	M_ASSERT(m_pBufferA,"Out of LS memory");
	m_pBufferB=m_pBufferA+VpuElementCount;
}

template <typename T, int VpuElementCount>
CVPU_OutStreamBuffer<T,VpuElementCount>::~CVPU_OutStreamBuffer()
{
	Flush();
}

template <typename T, int VpuElementCount>
T* CVPU_OutStreamBuffer<T,VpuElementCount>::GetNextElement()
{
	if (m_CurrentElement==m_ElementsInLastBuffer)
		m_CurrentElement=0;
	if (m_CurrentElement==0)
	{
		uint32 dummy;
		GetNextBuffer(dummy);
	}
	return m_pBufferB+m_CurrentElement++;
}

template <typename T, int VpuElementCount>
T* CVPU_OutStreamBuffer<T,VpuElementCount>::GetNextBuffer(uint32& _ElementCount)
{
	void* pElements=m_pBufferA;
	if (m_ElementsInLastBuffer>0)
	{
		cellDmaLargePut(m_pBufferB,m_SysAddr+m_TransferedElements*ElementSize,ElementSize*m_ElementsInLastBuffer,m_DmaTagB,0,0);
		if (m_TransferedElements>0)
			cellDmaWaitTagStatusAll(1<<m_DmaTagA); 
		m_TransferedElements+=m_ElementsInLastBuffer;
	}
	Swap(m_DmaTagA,m_DmaTagB);
	Swap(m_pBufferA,m_pBufferB);
	uint32 ElementsInNextBuffer=Min(uint32(VpuElementCount),uint32(m_SysElementCount-m_TransferedElements));
	_ElementCount=ElementsInNextBuffer;
	m_ElementsInLastBuffer=ElementsInNextBuffer;
	return (T*)((char*)pElements);
}


template <typename T, int VpuElementCount>
void CVPU_OutStreamBuffer<T,VpuElementCount>::Flush()
{
	cellDmaLargePut(m_pBufferB,m_SysAddr+m_TransferedElements*ElementSize,ElementSize*m_ElementsInLastBuffer,m_DmaTagB,0,0);
	cellDmaWaitTagStatusAll(1<<m_DmaTagB); 
	cellDmaWaitTagStatusAll(1<<m_DmaTagA); 
}

template <typename T> 
CVPU_SimpleBuffer<T>::CVPU_SimpleBuffer(const CVPU_SimpleBufferInfo& _BufferInfo)
:	m_pBuffer(NULL),
	m_SysAddr(mint(_BufferInfo.m_SysAddr)),
	m_BufferType(_BufferInfo.m_VPUBufferType),
	m_DmaTag(MRTC_System_VPU::OS_GetDMAChannel()),
	m_ElementCount(_BufferInfo.m_ElementCount)
{
	m_pBuffer=(T*) MRTC_System_VPU::OS_LocalHeapAlloc(m_ElementCount*ElementSize);
	M_ASSERT(m_pBuffer,"Out of LS memory");
	if (m_BufferType==VPU_IN_BUFFER || m_BufferType==VPU_INOUT_BUFFER)
		cellDmaLargeGet(m_pBuffer,m_SysAddr,ElementSize*m_ElementCount,m_DmaTag,0,0);
}

template <typename T> 
CVPU_SimpleBuffer<T>::~CVPU_SimpleBuffer()
{
	Flush();
}


template <typename T> 
T* CVPU_SimpleBuffer<T>::GetBuffer(uint32& _ElementCount)
{
	if (m_BufferType==VPU_IN_BUFFER || m_BufferType==VPU_INOUT_BUFFER)
		cellDmaWaitTagStatusAll(1<<m_DmaTag); 
	_ElementCount=m_ElementCount;
	return m_pBuffer;
}


template <typename T> 
void CVPU_SimpleBuffer<T>::Flush()
{
	if (m_BufferType==VPU_OUT_BUFFER || m_BufferType==VPU_INOUT_BUFFER)
	{
		cellDmaLargePut(m_pBuffer,m_SysAddr,ElementSize*m_ElementCount,m_DmaTag,0,0);
		cellDmaWaitTagStatusAll(1<<m_DmaTag); 
	}
}

void CVPU_WorkQueue::GetEvent(CVPU_ContextData& _ContextData,uint32 _SpuEventQueueNumber)
{
	uint32 data0,data1,data2;
	sys_spu_thread_receive_event(_SpuEventQueueNumber, &data0, &data1, &data2);
	if (data2==0)
	{
		cellDmaLargeGet(&_ContextData,data0,sizeof(CVPU_ContextData),0,0,0);
		cellDmaWaitTagStatusAll(1<<0); 
	}
}


void CVPU_WorkQueue::SignalVpuIdle(CVPU_ContextData& _ContextData)
{
//	uint32 dummy=0;
//	sys_spu_thread_send_event(SPU_THREAD_PORT, dummy, 0);
}



bool CVPU_WorkQueue::GetJob(CVPU_ContextData& _ContextData,CVPU_JobDefData& _JobDefData,uint32 _SpuEventQueueNumber)
{
	SpinLockLock(CVPU_SystemAddr(mint(_ContextData.m_pLock)));
	uint32 dummy0,dummy1,dummy2;
	sys_spu_thread_tryreceive_event(_SpuEventQueueNumber,&dummy0,&dummy1,&dummy2);
	cellDmaLargeGet(&_ContextData,mint(_ContextData.m_SelfInSysmem),sizeof(CVPU_ContextData),0,0,0);
	cellDmaWaitTagStatusAll(1<<0); 

	bool ret=true;
	if (_ContextData.IsEmpty())
		ret=false;
	else
	{
		cellDmaLargeGet(&_JobDefData,mint(&_ContextData.m_pJobDefData[_ContextData.m_JobQueueTail]),sizeof(CVPU_JobDefData),0,0,0);
		_ContextData.m_JobQueueTail=(_ContextData.m_JobQueueTail+1)%CVPU_ContextData::JobQueueSize;
		CVPU_ContextData* pTmp=_ContextData.m_SelfInSysmem;
		cellDmaPutUint16(_ContextData.m_JobQueueTail,mint(&pTmp->m_JobQueueTail),0,0,0);
	}
	SpinLockUnlock(CVPU_SystemAddr(mint(_ContextData.m_pLock)));
	return ret;
}


void CVPU_WorkQueue::SignalJobComplete(CVPU_ContextData& _ContextData,uint16 _JobIndex)
{
	SpinLockLock(CVPU_SystemAddr(mint(_ContextData.m_pLock)));
	cellDmaLargeGet(&_ContextData,mint(_ContextData.m_SelfInSysmem),sizeof(CVPU_ContextData),0,0,0);
	cellDmaWaitTagStatusAll(1<<0); 

	cellDmaPutUint8(CVPU_ContextData::JobDone,mint(&_ContextData.m_pJobs[_JobIndex]),0,0,0);

	SpinLockUnlock(CVPU_SystemAddr(mint(_ContextData.m_pLock)));

	sys_spu_thread_send_event(g_SPUEventPort, 0, 0);
}

// Currently this only works if you don't queue several jobs
// _Message is 23 bit only
void CVPU_WorkQueue::CallPPU(uint32 _Message, uint32 _Context, uint32 &_Ret0, uint32 &_Ret1)
{
	sys_spu_thread_send_event(g_SPUEventPort, M_Bit(23)| _Message, _Context);


	uint32 data0,data1,data2 = 0;
	sys_spu_thread_receive_event(g_SPUEventQueueNumber, &data0, &data1, &data2);
	if (data2==1)
	{
		_Ret0 = data0;
		_Ret1 = data1;
	}
	else
		M_BREAKPOINT; // This is a job, this currently does not work
}

/*
void CVPU_WorkQueue::GetJob(uint32& _JobHash,uint32& _JobId,CVPU_SystemAddr& DataAddr)		// Blocks until message arrive
{
	uint32 data;
	sys_spu_thread_receive_event(SPU_QUEUE_NUMBER, &_JobHash, &_JobId, &data);
	DataAddr.SetAddr(data);
}

*/

