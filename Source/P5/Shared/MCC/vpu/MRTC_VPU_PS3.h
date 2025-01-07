#include <sys/spu_event.h>
#include <cell/dma.h>

extern "C" char _end[];
uint32 pMem=(uint32)_end;


class CVPU_WorkQueue
{
public:
	static void GetJob(uint32& _JobHash,uint32& _JobId,CVPU_SystemAddr& DataAddr);		// Blocks until message arrive
	static void SignalJobComplete(uint32 _JobId);
};

CVPU_JobInfo::CVPU_JobInfo(CVPU_JobDefData* _pJobData)
:	m_pJobData(_pJobData),
	m_pScratchBuffer(NULL)
{
	m_pScratchBuffer=MRTC_System_VPU::OS_HeapAlloc(_pJobData->m_ScratchSize);
}

CVPU_JobInfo::~CVPU_JobInfo(){}


int VPU_Main()
{
	while(1)
	{
		uint32 JobId;
		uint32 JobHash;
		CVPU_SystemAddr DataAddr;
		CVPU_WorkQueue::GetJob(JobHash,JobId,DataAddr);
		CVPU_JobDefData JobDefData;
		MRTC_System_VPU::OS_DMATransferFromSys(DataAddr,&JobDefData,sizeof(CVPU_JobDefData));
		MRTC_System_VPU::OS_DMABlockUntilIdle();
		CVPU_JobInfo JobInfo(&JobDefData);


		VPU_Worker(JobHash,JobInfo);
		MRTC_System_VPU::OS_HeapFreeAll();
		CVPU_WorkQueue::SignalJobComplete(JobId);
	}
}

#define MVPU_MAIN	\
	int main(void)	\
{	\
	VPU_Main();	\
}







void MRTC_System_VPU::OS_Assert(const char* msg, const char* _pFile, int _Line)
{
	spu_stop(0x3fff);
}

void* MRTC_System_VPU::OS_HeapAlloc(uint32 _Size)
{
	pMem=(pMem+127) & 0xffffff80;
	uint32 ret=pMem;
	pMem+=_Size;
	return (void*) ret;
}

void MRTC_System_VPU::OS_HeapFree(void *_pMem)
{
}


void MRTC_System_VPU::OS_HeapFreeAll()
{
	pMem=(uint32)_end;
}



void MRTC_System_VPU::OS_DMATransferFromSys(CVPU_SystemAddr _pSrc, void* _pDst, mint _Size)
{
	const uint32 DmaTag=0;
	cellDmaLargeGet(_pDst,_pSrc,_Size,DmaTag,0,0);

}

void MRTC_System_VPU::OS_DMABlockUntilIdle()
{
	const uint32 DmaTag=0;
	cellDmaWaitTagStatusAll(1<<DmaTag); 
}


CVPU_CacheBuffer::CVPU_CacheBuffer(CVPU_CacheBufferInfo& _BufferInfo,uint32 _BufferId)
:	m_SysAddr((mint) _BufferInfo.m_SysAddr),
	m_VpuBufferSize(_BufferInfo.m_ElementSize*CacheSize),
	m_ElementSize(_BufferInfo.m_ElementSize),
	m_SysElementCount(_BufferInfo.m_SysElementCount)
{
	m_CacheData=(uint8*) MRTC_System_VPU::OS_HeapAlloc(m_VpuBufferSize);
	for (int i=0;i<CacheSize;i++)
		m_CacheTags[i]=0xffffffff;
}


CVPU_CacheBuffer::~CVPU_CacheBuffer()
{
	delete [] m_CacheData;
}

vec128 CVPU_CacheBuffer::GetVecElement(uint32 _Index)
{
	M_ASSERT(_Index<m_SysElementCount,"Index out of range");
	uint32 CacheIndex=_Index%CacheSize;
	uint8* CacheData=m_CacheData+CacheIndex*m_ElementSize;
	if (m_CacheTags[CacheIndex]!=_Index)
	{
		const uint32 DmaTag=0;
		cellDmaLargeGet(CacheData,m_SysAddr+_Index*m_ElementSize,m_ElementSize,DmaTag,0,0);
		m_CacheTags[CacheIndex]=_Index;
		cellDmaWaitTagStatusAll(1<<DmaTag); 
	}
	return *((vec128 *)(CacheData));
}

const uint8* CVPU_CacheBuffer::GetPntElement(uint32 _Index)
{
	M_ASSERT(_Index<m_SysElementCount,"Index out of range");
	uint32 CacheIndex=_Index%CacheSize;
	uint8* CacheData=m_CacheData+CacheIndex*m_ElementSize;
	if (m_CacheTags[CacheIndex]!=_Index)
	{
		const uint32 DmaTag=0;
		cellDmaLargeGet(CacheData,m_SysAddr+_Index*m_ElementSize,m_ElementSize,DmaTag,0,0);
		m_CacheTags[CacheIndex]=_Index;
		cellDmaWaitTagStatusAll(1<<DmaTag); 
	}
	return CacheData;

}





//extern "C" char _stack_size[], _stack[];
CVPU_StreamBuffer::CVPU_StreamBuffer(CVPU_StreamBufferInfo& _BufferInfo,uint32 _BufferId)
:	m_pBufferA(NULL),
	m_pBufferB(NULL),
	m_SysElementCount(_BufferInfo.m_SysElementCount),
	m_VpuElementCount(_BufferInfo.m_VpuElementCount),
	m_ElementSize(_BufferInfo.m_ElementSize),
	m_SysAddr(mint(_BufferInfo.m_SysAddr)),
	m_BufferType(_BufferInfo.m_VPUBufferType),
	m_TransferedElements(0),
	m_ElementsInLastBuffer(0),
	m_DmaTagA(_BufferId*2),
	m_DmaTagB(_BufferId*2+1),
	m_IsDoubleBuffered(_BufferInfo.m_VpuElementCount<_BufferInfo.m_SysElementCount)
{
	M_ASSERT(_BufferId<=MAX_BUFFER_ID,"");
	m_pBufferA=MRTC_System_VPU::OS_HeapAlloc(m_VpuElementCount*m_ElementSize*2);

	if (m_IsDoubleBuffered)
	{
		m_VpuElementCount/=2;
		m_pBufferB=(uint8*)m_pBufferA+m_VpuElementCount*m_ElementSize;
	}

	if (m_BufferType==VPU_IN_BUFFER)
	{
		m_ElementsInLastBuffer=Min(uint32(m_VpuElementCount),m_SysElementCount);
		cellDmaLargeGet(m_pBufferA,m_SysAddr,m_ElementSize*m_ElementsInLastBuffer,m_DmaTagA,0,0);
		m_TransferedElements+=m_ElementsInLastBuffer;
	}
}

CVPU_StreamBuffer::~CVPU_StreamBuffer()
{
	Flush();
}


void* CVPU_StreamBuffer::GetNextBuffer(uint32& _ElementCount)
{
	M_ASSERT(m_IsDoubleBuffered,"Buffer must be double buffered");
	void* pElements=m_pBufferA;
	if (m_BufferType==VPU_IN_BUFFER)
	{
		uint32 ElementsInNextBuffer=Min(uint32(m_VpuElementCount),m_SysElementCount-m_TransferedElements);
		_ElementCount=m_ElementsInLastBuffer;
		cellDmaWaitTagStatusAll(1<<m_DmaTagA); 
		Swap(m_DmaTagA,m_DmaTagB);
		Swap(m_pBufferA,m_pBufferB);
		cellDmaLargeGet(m_pBufferA,m_SysAddr+m_TransferedElements*m_ElementSize,m_ElementSize*ElementsInNextBuffer,m_DmaTagA,0,0);
		m_TransferedElements+=ElementsInNextBuffer;
		m_ElementsInLastBuffer=ElementsInNextBuffer;
	}
	else if (m_BufferType==VPU_OUT_BUFFER)
	{
		if (m_ElementsInLastBuffer>0)
		{
			cellDmaLargePut(m_pBufferB,m_SysAddr+m_TransferedElements*m_ElementSize,m_ElementSize*m_ElementsInLastBuffer,m_DmaTagB,0,0);
			if (m_TransferedElements>0)
				cellDmaWaitTagStatusAll(1<<m_DmaTagA); 
			m_TransferedElements+=m_ElementsInLastBuffer;
		}
		Swap(m_DmaTagA,m_DmaTagB);
		Swap(m_pBufferA,m_pBufferB);
		uint32 ElementsInNextBuffer=Min(uint32(m_VpuElementCount),m_SysElementCount-m_TransferedElements);
		_ElementCount=ElementsInNextBuffer;
		m_ElementsInLastBuffer=ElementsInNextBuffer;
	}
	return (void*)((char*)pElements);
}

void* CVPU_StreamBuffer::GetBuffer(uint32& _ElementCount)
{
	M_ASSERT(!m_IsDoubleBuffered,"Buffer must be single buffered");
	cellDmaWaitTagStatusAll(1<<m_DmaTagA); 
	_ElementCount=m_SysElementCount;
	m_ElementsInLastBuffer=m_SysElementCount;
	Swap(m_DmaTagA,m_DmaTagB);
	Swap(m_pBufferA,m_pBufferB);
	return m_pBufferB;
}

void CVPU_StreamBuffer::SetPosition(uint32 _ElementIndex)
{
	M_ASSERT(m_BufferType==VPU_IN_BUFFER,"");
	M_ASSERT(!m_IsDoubleBuffered,"Buffer must be single buffered");
	m_TransferedElements=_ElementIndex;
	m_ElementsInLastBuffer=Min(uint32(m_VpuElementCount),m_SysElementCount-m_TransferedElements);
	Swap(m_DmaTagA,m_DmaTagB);
	Swap(m_pBufferA,m_pBufferB);
	cellDmaLargeGet(m_pBufferA,m_SysAddr+m_TransferedElements*m_ElementSize,m_ElementSize*m_ElementsInLastBuffer,m_DmaTagA,0,0);
	m_TransferedElements+=m_ElementsInLastBuffer;
}



void CVPU_StreamBuffer::Flush()
{
	if (m_BufferType==VPU_OUT_BUFFER)
	{
		cellDmaLargePut(m_pBufferB,m_SysAddr+m_TransferedElements*m_ElementSize,m_ElementSize*m_ElementsInLastBuffer,m_DmaTagB,0,0);
		cellDmaWaitTagStatusAll(1<<m_DmaTagB); 
		cellDmaWaitTagStatusAll(1<<m_DmaTagA); 
	}
}


void CVPU_WorkQueue::GetJob(uint32& _JobHash,uint32& _JobId,CVPU_SystemAddr& DataAddr)		// Blocks until message arrive
{
	uint32 data;
	sys_spu_thread_receive_event(SPU_QUEUE_NUMBER, &_JobHash, &_JobId, &data);
	DataAddr.SetAddr(data);
}

void CVPU_WorkQueue::SignalJobComplete(uint32 _JobId)
{
	uint32 dummy;
	sys_spu_thread_send_event(SPU_THREAD_PORT, dummy, _JobId);
	//		sys_spu_thread_throw_event(SPU_THREAD_PORT, dummy, _JobId);
}


