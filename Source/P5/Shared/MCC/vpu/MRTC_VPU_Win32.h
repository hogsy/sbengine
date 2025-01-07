
#include "../Mrtc.h"

int VPU_Main(CVPU_JobDefData& _JobDefData)
{
	CVPU_JobInfo JobInfo(&_JobDefData);
	VPU_Worker(_JobDefData.m_JobHash,JobInfo);
	return 0;
}

#define MVPU_MAIN

CVPU_JobInfo::CVPU_JobInfo(CVPU_JobDefData* _pJobData)
:	m_pJobData(_pJobData),
	m_pScratchBuffer(NULL)
{
	m_pScratchBuffer=DNew(uint8) uint8[m_pJobData->m_ScratchSize];
}

CVPU_JobInfo::~CVPU_JobInfo()
{
	delete [] m_pScratchBuffer;;
}

CVPU_CacheBuffer::CVPU_CacheBuffer(CVPU_CacheBufferInfo& _BufferInfo,uint32 _BufferId)
:	m_SysAddr((mint) _BufferInfo.m_SysAddr),
m_VpuBufferSize(_BufferInfo.m_ElementSize*CacheSize),
m_ElementSize(_BufferInfo.m_ElementSize),
m_SysElementCount(_BufferInfo.m_SysElementCount)
{
	m_CacheData=(uint8*) (DNew(vec128) vec128[(m_VpuBufferSize+15)/16]);
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
		m_CacheTags[CacheIndex]=_Index;
		uint8* SysData=(uint8* )((mint)m_SysAddr)+_Index*m_ElementSize;
		memcpy(CacheData,SysData,m_ElementSize);
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
		m_CacheTags[CacheIndex]=_Index;
		uint8* SysData=(uint8* )((mint)m_SysAddr)+_Index*m_ElementSize;
		memcpy(CacheData,SysData,m_ElementSize);
	}
	return CacheData;

}


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
}

CVPU_StreamBuffer::~CVPU_StreamBuffer()
{
}



void* CVPU_StreamBuffer::GetNextBuffer(uint32& _ElementCount)
{
	M_ASSERT(m_IsDoubleBuffered,"Buffer must be double buffered");
	mint pElements=m_SysAddr+m_TransferedElements*m_ElementSize;
	_ElementCount=Min(uint32(m_VpuElementCount),m_SysElementCount-m_TransferedElements);
	m_TransferedElements+=_ElementCount;
	return (void*) pElements;
}

void* CVPU_StreamBuffer::GetBuffer(uint32& _ElementCount)
{
	M_ASSERT(!m_IsDoubleBuffered,"Buffer must be single buffered");
	_ElementCount=m_SysElementCount;
	return (void*) mint(m_SysAddr);
}

void CVPU_StreamBuffer::SetPosition(uint32 _ElementIndex)
{
	M_ASSERT(_ElementIndex<m_SysElementCount,"");
	m_TransferedElements=_ElementIndex;
}

void CVPU_StreamBuffer::Flush()
{
}

