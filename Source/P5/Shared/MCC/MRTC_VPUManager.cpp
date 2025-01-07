#include "PCH.h"
#include "MRTC_VPUManager.h"
#include "Mrtc.h"


CVPU_JobDefinition::CVPU_JobDefinition()
{	
	m_Data.m_JobHash=0;
	m_Data.m_JobId=0;
	m_Data.m_ScratchSize=0;
}


void CVPU_JobDefinition::AddCacheBuffer(uint32 paramIndex,void* _Addr,mint _SysElementCount,uint16 _ElementSize)
{
#define CacheSize 128
#ifdef PLATFORM_PS3
	M_ASSERT(mint(_Addr)%16==0,"System address must be aligned on 16 bytes");
#endif
	M_ASSERT(_ElementSize%16==0,"Elementsize must be a multiple of 16 bytes");
	const uint16 VpuSize=CacheSize*_ElementSize;
	M_ASSERT(m_TotalVpuSize+VpuSize<MAX_TOTAL_VPU_SIZE,"Out of VPU memory");
	m_TotalVpuSize+=VpuSize;
	m_Data.m_Params[paramIndex].m_CacheBufferData.m_SysAddr=_Addr;
	m_Data.m_Params[paramIndex].m_CacheBufferData.m_SysElementCount=_SysElementCount;
	m_Data.m_Params[paramIndex].m_CacheBufferData.m_ElementSize=_ElementSize;
}


void CVPU_JobDefinition::AddStreamBuffer(uint32 paramIndex,void* _Addr,mint _SysElementCount,uint16 _VpuElementCount,uint16 _ElementSize,VPUBufferType BufferType)
{
#ifdef PLATFORM_PS3
	M_ASSERT(mint(_Addr)%16==0,"System address must be aligned on 16 bytes");
#endif
	const uint16 VpuSize=_VpuElementCount*_ElementSize;
	M_ASSERT(m_TotalVpuSize+VpuSize<MAX_TOTAL_VPU_SIZE,"Out of VPU memory");
	m_TotalVpuSize+=VpuSize;
	m_Data.m_Params[paramIndex].m_StreamBufferData.m_VPUBufferType=BufferType;
	m_Data.m_Params[paramIndex].m_StreamBufferData.m_SysAddr=_Addr;
	m_Data.m_Params[paramIndex].m_StreamBufferData.m_SysElementCount=_SysElementCount;
	m_Data.m_Params[paramIndex].m_StreamBufferData.m_VpuElementCount=_VpuElementCount;
	m_Data.m_Params[paramIndex].m_StreamBufferData.m_ElementSize=_ElementSize;
}

void CVPU_JobDefinition::SetScratchBuffer(uint32 _VpuBufferSize)
{
	M_ASSERT(m_TotalVpuSize+_VpuBufferSize<MAX_TOTAL_VPU_SIZE,"Out of VPU memory");
	m_TotalVpuSize+=_VpuBufferSize;
	m_Data.m_ScratchSize=_VpuBufferSize;
}


void CVPU_JobDefinition::AddVParam(uint32 paramIndex,vec128 _Param)
{
	m_Data.m_Params[paramIndex].m_VecData=_Param;
}


void CVPU_JobDefinition::AddLParams(uint32 paramIndex,uint32 _Param0,uint32 _Param1,uint32 _Param2,uint32 _Param3)
{
	m_Data.m_Params[paramIndex].m_LongData[0]=_Param0;
	m_Data.m_Params[paramIndex].m_LongData[1]=_Param1;
	m_Data.m_Params[paramIndex].m_LongData[2]=_Param2;
	m_Data.m_Params[paramIndex].m_LongData[3]=_Param3;
}


#ifdef PLATFORM_PS3

enum { MAX_PHYSICAL_SPU=6 };
enum { MAX_RAW_SPU=0 };
enum { EVENT_QUEUE_SIZE=127 };

#include <sys/spu_initialize.h>
#include <sys/spu_image.h>
#include <sys/spu_thread.h>
#include <sys/spu_thread_group.h>
#include <sys/spu_utility.h>
#include <sys/event.h>
#include <sys/paths.h>

#define SPU_PROG (SYS_APP_HOME "/VPUWorkers.elf")

#define PRIORITY 100

class MRTC_VPUManagerData {
public:
	enum { MAX_SPU_THREADS=1 };
	sys_spu_thread_group_t m_ThreadGroup;				/* SPU thread group ID */
	sys_spu_thread_group_attribute_t m_ThreadGroupAttr;	/* SPU thread group attribute*/
	sys_spu_thread_t m_Threads[MAX_SPU_THREADS];		/* SPU thread IDs */
	sys_spu_thread_attribute_t m_ThreadAttr;			/* SPU thread attribute */
	sys_spu_image_t m_SpuImage;

	sys_event_queue_t m_SpuEventQueue;
	sys_event_queue_t m_PpuEventQueue;
	sys_event_queue_attribute_t m_PpuEventQueueAttr;
	sys_event_queue_attribute_t m_SpuEventQueueAttr;
	sys_event_port_t m_SpuEventPort;
	sys_event_port_t m_PpuEventPort;
};

const char *ThreadGroupName = "Group";
const char *ThreadNames[7] = {"SPU Thread 0","SPU Thread 1","SPU Thread 2","SPU Thread 3","SPU Thread 4","SPU Thread 5","SPU Thread 6"};

extern char _binary_VPUWorkers_elf_start[];

MRTC_VPUManager::MRTC_VPUManager()
{
	m_Data= new MRTC_VPUManagerData;
	int ret;
	M_TRACE("Initializing SPUs\n");
	ret = 0xffffffff & sys_spu_initialize(MAX_PHYSICAL_SPU, MAX_RAW_SPU);
	switch (ret) 
	{
	case CELL_OK:
		break;
	case uint(EBUSY):
		M_TRACE("SPUs have already been initialized.\n");
		break;
	default:
		M_TRACE("sys_spu_initialize failed: %d\n", ret);
		M_ASSERT(0,"");
		break;
	}

	// create event queue and event port
	m_Data->m_SpuEventQueueAttr.attr_protocol = SYS_SYNC_PRIORITY;
	m_Data->m_SpuEventQueueAttr.type = SYS_SPU_QUEUE;
	ret = sys_event_queue_create(&m_Data->m_SpuEventQueue, &m_Data->m_SpuEventQueueAttr, SYS_EVENT_QUEUE_LOCAL, 
		EVENT_QUEUE_SIZE);
	M_ASSERT(ret==CELL_OK,"Failed to create Spu event queue.");

	ret = sys_event_port_create(&m_Data->m_SpuEventPort, SYS_EVENT_PORT_LOCAL, SYS_EVENT_PORT_NO_NAME);
	M_ASSERT(ret==CELL_OK,"Failed to create event port.");

	ret = sys_event_port_connect_local(m_Data->m_SpuEventPort, m_Data->m_SpuEventQueue);
	M_ASSERT(ret==CELL_OK,"Failed to connect event port to event queue.");

	m_Data->m_PpuEventQueueAttr.attr_protocol = SYS_SYNC_PRIORITY;
	m_Data->m_PpuEventQueueAttr.type = SYS_PPU_QUEUE;
	ret = sys_event_queue_create(&m_Data->m_PpuEventQueue, &m_Data->m_PpuEventQueueAttr, SYS_EVENT_QUEUE_LOCAL, EVENT_QUEUE_SIZE);
	M_ASSERT(ret==CELL_OK,"Failed to create Ppu event queue.");

	// create SPU thread group
	m_Data->m_ThreadGroupAttr.name = ThreadGroupName;
	m_Data->m_ThreadGroupAttr.nsize = strlen(m_Data->m_ThreadGroupAttr.name) + 1;
	m_Data->m_ThreadGroupAttr.type = SYS_SPU_THREAD_GROUP_TYPE_NORMAL;
	ret = sys_spu_thread_group_create(&m_Data->m_ThreadGroup, MRTC_VPUManagerData::MAX_SPU_THREADS, PRIORITY, &m_Data->m_ThreadGroupAttr);
	M_ASSERT(ret==CELL_OK,"Failed create SPU thread group.");

//	ret = sys_spu_image_open(&m_Data->m_SpuImage,SPU_PROG);
	ret = sys_spu_image_import(&m_Data->m_SpuImage,_binary_VPUWorkers_elf_start ,SYS_SPU_IMAGE_DIRECT);
	M_ASSERT(ret==CELL_OK,"Failed to open SPU image.");

	for (int i = 0; i < MRTC_VPUManagerData::MAX_SPU_THREADS; i++) 
	{
		sys_spu_thread_argument_t thread_args;
		// nsegs, segs and entry_point have already been initialized by sys_spu_thread_elf_loader().
		m_Data->m_ThreadAttr.name = ThreadNames[i];
		m_Data->m_ThreadAttr.nsize = strlen(ThreadNames[i]) + 1;
		m_Data->m_ThreadAttr.option = SYS_SPU_THREAD_OPTION_NONE;

		thread_args.arg1 = SYS_SPU_THREAD_ARGUMENT_LET_32(SPU_QUEUE_NUMBER);
		ret = sys_spu_thread_initialize(&m_Data->m_Threads[i],m_Data->m_ThreadGroup,i,&m_Data->m_SpuImage,&m_Data->m_ThreadAttr,&thread_args);
		M_ASSERT(ret==CELL_OK,"Failed to initialize SPU thread.");
		
	
		ret = sys_spu_thread_bind_queue(m_Data->m_Threads[i], m_Data->m_SpuEventQueue, SPU_QUEUE_NUMBER);
		M_ASSERT(ret==CELL_OK,"Failed to bind SPU thread to eventqueue.");

		ret = sys_spu_thread_connect_event(m_Data->m_Threads[i], m_Data->m_PpuEventQueue, SYS_SPU_THREAD_EVENT_USER, SPU_THREAD_PORT);
		M_ASSERT(ret==CELL_OK,"Failed to connect SPU thread port to eventqueue.");

	}
	ret = sys_spu_thread_group_start(m_Data->m_ThreadGroup);
	M_ASSERT(ret==CELL_OK,"Failed to start SPU thread group.");
	m_JobsSent=0; 
	m_JobsRecived=0;
	M_TRACE("Initialized SPUs\n");
}


MRTC_VPUManager::~MRTC_VPUManager()
{
	for (int i = 0; i < MRTC_VPUManagerData::MAX_SPU_THREADS; i++) 
	{
		int ret = sys_event_port_send(m_Data->m_SpuEventPort, EVENT_TERMINATE, EVENT_TERMINATE,EVENT_TERMINATE);
		M_ASSERT(ret==CELL_OK,"Failed to send Terminate event to SPU.");
	}

	int cause, status;
	int ret = sys_spu_thread_group_join(m_Data->m_ThreadGroup, &cause, &status);
	M_ASSERT(ret==CELL_OK,"Failed to terminate SPU thread group.");

	for (int i = 0; i < MRTC_VPUManagerData::MAX_SPU_THREADS; i++) {
		ret = sys_spu_thread_unbind_queue(m_Data->m_Threads[i], SPU_QUEUE_NUMBER);
		M_ASSERT(ret==CELL_OK,"Failed to unbind SPU thread from event queue.");
		ret = sys_spu_thread_disconnect_event(m_Data->m_Threads[i],	SYS_SPU_THREAD_EVENT_USER, SPU_THREAD_PORT);
		M_ASSERT(ret==CELL_OK,"Failed to disconnect SPU thread port from event queue.");
	}

	ret = sys_spu_thread_group_destroy(m_Data->m_ThreadGroup);
	M_ASSERT(ret==CELL_OK,"Failed to destroy SPU thread group.");

	ret = sys_event_port_disconnect(m_Data->m_PpuEventPort);
	M_ASSERT(ret==CELL_OK,"Failed to disconnect eventport.");
	ret = sys_event_port_disconnect(m_Data->m_SpuEventPort);
	M_ASSERT(ret==CELL_OK,"Failed to disconnect eventport.");

	ret = sys_event_port_destroy(m_Data->m_PpuEventPort);
	M_ASSERT(ret==CELL_OK,"Failed to destroy eventport.");
	ret = sys_event_port_destroy(m_Data->m_SpuEventPort);
	M_ASSERT(ret==CELL_OK,"Failed to destroy eventport.");

	ret = sys_event_queue_destroy(m_Data->m_SpuEventQueue, 0);
	M_ASSERT(ret==CELL_OK,"Failed to destroy eventqueue.");
	ret = sys_event_queue_destroy(m_Data->m_PpuEventQueue, 0);
	M_ASSERT(ret==CELL_OK,"Failed to destroy eventqueue.");

	ret = sys_spu_image_close(&m_Data->m_SpuImage);
	M_ASSERT(ret==CELL_OK,"Failed to close SPU image.");
}	

uint32 MRTC_VPUManager::AddTask(CVPU_JobDefinition& _JobDefinition)
{
	_JobDefinition.m_Data.m_JobId=1000+m_JobsSent++;
	uint64 data0=_JobDefinition.m_Data.m_JobHash;
	uint64 data1=_JobDefinition.m_Data.m_JobId;
	uint64 data2=uint32(&_JobDefinition.m_Data);
	int ret = sys_event_port_send(m_Data->m_SpuEventPort, data0, data1, data2);
	M_ASSERT(ret==CELL_OK,"Failed to send event to SPU.");
	return _JobDefinition.m_Data.m_JobId;
}
bool IsComplete(uint32 _Task,sys_event_queue_t& _SysEventQueue,usecond_t _Timeout)
{
	sys_event_t EventData;
	int ret=0xffffffff & sys_event_queue_receive(_SysEventQueue,&EventData,_Timeout);
	switch (ret)
	{
	case CELL_OK:
		{
			if (EventData.data3==_Task)
			{
				return true;
			}
			break;
		}
	case uint(ESRCH):
		{
			M_ASSERT(0,"INVALID EVENT QUEUE ID");
			break;
		}
	case uint(EINVAL):
		{
			M_ASSERT(0,"THIS EVENT QUEUE IS FOR SPU");
			break;
		}
	case uint(ETIMEDOUT):
		{
			M_ASSERT(0,"TIMEOUT");
			break;
		}
	default:
		{
			M_ASSERT(0,"UNKNOWN ERROR");
			break;
		}
	}	
	M_ASSERT(0,"DUMMY SHOULD NEVER GET HERE");
	return false;

}

bool MRTC_VPUManager::IsTaskComplete(uint32 _Task)
{
	if (IsComplete(_Task,m_Data->m_PpuEventQueue,SYS_NO_TIMEOUT))
	{
		m_JobsRecived++;
		return true;
	}
	return false;
}


void MRTC_VPUManager::BlockOnTask(uint32 _Task)
{
	sys_event_t EventData;
	uint TimeOut=20000000; //microsec
	if (IsComplete(_Task,m_Data->m_PpuEventQueue,TimeOut))
		m_JobsRecived++;
}

#else

MRTC_VPUManager::MRTC_VPUManager()
{
}

MRTC_VPUManager::~MRTC_VPUManager()
{
}

int VPU_Main(CVPU_JobDefData& _JobDefData);

uint32 MRTC_VPUManager::AddTask(CVPU_JobDefinition& _JobDefinition)
{
	VPU_Main(_JobDefinition.m_Data);
	return 0;
}

bool MRTC_VPUManager::IsTaskComplete(uint32 _Task)
{
	return true;
}

void MRTC_VPUManager::BlockOnTask(uint32 _Task)
{
}

bool MRTC_VPUManager::TryBlockUntilIdle()
{
	return true;
}

void MRTC_VPUManager::BlockUntilIdle()
{
}



#endif



