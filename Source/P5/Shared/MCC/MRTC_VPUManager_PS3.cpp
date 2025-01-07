#include "PCH.h"
#include "MRTC_VPUManager.h"
#include "Mrtc.h"


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

#define USE_SPU_PRINTF

#ifdef USE_SPU_PRINTF
#include "PS3/spu_printf_server.h"
#endif

#if 1
#define SPU_PROG_AI (SYS_APP_HOME "/System/PS3Exes/WBlockNavGrid_VPU.elf")
#define SPU_PROG_WORKERS (SYS_APP_HOME "/System/PS3Exes/VPUWorkers.elf")
#define SPU_PROG_SOUND (SYS_APP_HOME "/System/PS3Exes/MSound_Mixer_Worker.elf")
#else
#define SPU_PROG_AI (SYS_APP_HOME "/System/PS3Exes/MSound_Mixer_Worker.elf")
#define SPU_PROG_WORKERS (SYS_APP_HOME "/System/PS3Exes/MSound_Mixer_Worker.elf")
#define SPU_PROG_SOUND (SYS_APP_HOME "/System/PS3Exes/MSound_Mixer_Worker.elf")
#endif

#define PRIORITY 100




class CVPU_Program {
public:
	void Init()
	{
		int ret;
		m_SpuEventQueueAttr.attr_protocol = SYS_SYNC_FIFO;
		m_SpuEventQueueAttr.type = SYS_SPU_QUEUE;
		ret = sys_event_queue_create(&m_SpuEventQueue,&m_SpuEventQueueAttr, SYS_EVENT_QUEUE_LOCAL, EVENT_QUEUE_SIZE);
		M_ASSERT(ret==CELL_OK,"Failed to create Spu event queue.");

		ret = sys_event_port_create(&m_SpuEventPort, SYS_EVENT_PORT_LOCAL, SYS_EVENT_PORT_NO_NAME);
//		ret = sys_event_port_create(&m_SpuEventPort, SYS_EVENT_PORT_LOCAL, m_ContextId+1024);
		M_ASSERT(ret==CELL_OK,"Failed to create event port.");

		ret = sys_event_port_connect_local(m_SpuEventPort, m_SpuEventQueue);
		M_ASSERT(ret==CELL_OK,"Failed to connect event port to event queue.");

		m_PpuEventQueueAttr.attr_protocol = SYS_SYNC_PRIORITY;
		m_PpuEventQueueAttr.type = SYS_PPU_QUEUE;
		ret = sys_event_queue_create(&m_PpuEventQueue, &m_PpuEventQueueAttr, SYS_EVENT_QUEUE_LOCAL, EVENT_QUEUE_SIZE);
		M_ASSERT(ret==CELL_OK,"Failed to create Ppu event queue.");

		// create SPU thread group
		m_ThreadGroupAttr.name = m_ThreadGroupName.Str();
		m_ThreadGroupAttr.nsize = m_ThreadGroupName.Len() + 1;
		m_ThreadGroupAttr.type = SYS_SPU_THREAD_GROUP_TYPE_NORMAL;
		ret = sys_spu_thread_group_create(&m_ThreadGroup, m_ThreadCount, PRIORITY, &m_ThreadGroupAttr);
		M_ASSERT(ret==CELL_OK,"Failed create SPU thread group.");

		if (m_ContextId==VpuAIContext)
			ret = sys_spu_image_open(&m_SpuImage,SPU_PROG_AI);
		if (m_ContextId==VpuWorkersContext)
			ret = sys_spu_image_open(&m_SpuImage,SPU_PROG_WORKERS);
		if (m_ContextId==VpuSoundContext)
			ret = sys_spu_image_open(&m_SpuImage,SPU_PROG_SOUND);
		//	ret = sys_spu_image_import(&m_PlatformData->m_SpuImage,_binary_VPUWorkers_elf_start ,SYS_SPU_IMAGE_DIRECT);
		M_ASSERT(ret==CELL_OK,"Failed to open SPU image.");
	}

	void InitProgram(int& iSpu)
	{
		int ret;
		m_thread_args.arg1 = SYS_SPU_THREAD_ARGUMENT_LET_32(m_SpuEventQueueNumber);
		m_thread_args.arg2 = SYS_SPU_THREAD_ARGUMENT_LET_32(m_SpuThreadPort);
		for (int i = 0; i < m_ThreadCount; i++) 
		{
			// nsegs, segs and entry_point have already been initialized by sys_spu_thread_elf_loader().
			const char* tmp=m_ThreadNames[i].Str();
			m_ThreadAttr[i].name = tmp;
			m_ThreadAttr[i].nsize = m_ThreadNames[i].Len() + 1;
			m_ThreadAttr[i].option = SYS_SPU_THREAD_OPTION_NONE;

			sys_spu_thread_t& SysSpuThread=m_Threads[i];
			ret = sys_spu_thread_initialize(&SysSpuThread,m_ThreadGroup,iSpu++,&m_SpuImage,&m_ThreadAttr[i],&m_thread_args);
			M_ASSERT(ret==CELL_OK,"Failed to initialize SPU thread.");

			ret = sys_spu_thread_bind_queue(SysSpuThread, m_SpuEventQueue, m_SpuEventQueueNumber);
			M_ASSERT(ret==CELL_OK,"Failed to bind SPU thread to eventqueue.");

			ret = sys_spu_thread_connect_event(SysSpuThread, m_PpuEventQueue, SYS_SPU_THREAD_EVENT_USER, m_SpuThreadPort);
			M_ASSERT(ret==CELL_OK,"Failed to connect SPU thread port to eventqueue.");

#ifdef USE_SPU_PRINTF
			ret = spu_printf_server_register(SysSpuThread);
			M_ASSERT(ret==CELL_OK,"spu_printf_server_register failed.");
#endif
		}
		ret = sys_spu_thread_group_start(m_ThreadGroup);
		M_ASSERT(ret==CELL_OK,"Failed to start SPU thread group.");
		M_TRACE("Initialized SPUs\n");
	}
	void Close()
	{
		int ret = sys_event_port_send(m_SpuEventPort, EVENT_TERMINATE, EVENT_TERMINATE,EVENT_TERMINATE);
		M_ASSERT(ret==CELL_OK,"Failed to send Terminate event to SPU.");
		
		int cause, status;
		ret = sys_spu_thread_group_join(m_ThreadGroup, &cause, &status);
		M_ASSERT(ret==CELL_OK,"Failed to terminate SPU thread group.");

		for (int i = 0; i < m_ThreadCount; i++) 
		{
			ret = sys_spu_thread_unbind_queue(m_Threads[i], m_SpuEventQueueNumber);
			M_ASSERT(ret==CELL_OK,"Failed to unbind SPU thread from event queue.");
			ret = sys_spu_thread_disconnect_event(m_Threads[i],	SYS_SPU_THREAD_EVENT_USER, m_SpuEventPort);
			M_ASSERT(ret==CELL_OK,"Failed to disconnect SPU thread port from event queue.");
		}

		ret = sys_spu_thread_group_destroy(m_ThreadGroup);
		M_ASSERT(ret==CELL_OK,"Failed to destroy SPU thread group.");

		ret = sys_event_port_disconnect(m_PpuEventPort);
		M_ASSERT(ret==CELL_OK,"Failed to disconnect eventport.");
		ret = sys_event_port_disconnect(m_SpuEventPort);
		M_ASSERT(ret==CELL_OK,"Failed to disconnect eventport.");

		ret = sys_event_port_destroy(m_PpuEventPort);
		M_ASSERT(ret==CELL_OK,"Failed to destroy eventport.");
		ret = sys_event_port_destroy(m_SpuEventPort);
		M_ASSERT(ret==CELL_OK,"Failed to destroy eventport.");

		ret = sys_event_queue_destroy(m_SpuEventQueue, 0);
		M_ASSERT(ret==CELL_OK,"Failed to destroy eventqueue.");
		ret = sys_event_queue_destroy(m_PpuEventQueue, 0);
		M_ASSERT(ret==CELL_OK,"Failed to destroy eventqueue.");

		ret = sys_spu_image_close(&m_SpuImage);
		M_ASSERT(ret==CELL_OK,"Failed to close SPU image.");
	}


	sys_spu_image_t m_SpuImage;

	sys_event_queue_t m_SpuEventQueue;
	sys_event_queue_t m_PpuEventQueue;
	sys_event_queue_attribute_t m_PpuEventQueueAttr;
	sys_event_queue_attribute_t m_SpuEventQueueAttr;
	sys_event_port_t m_SpuEventPort;
	sys_event_port_t m_PpuEventPort;
	uint32 m_SpuEventQueueNumber;
	uint32 m_SpuThreadPort;

	sys_spu_thread_group_t m_ThreadGroup;					// SPU thread group ID 
	sys_spu_thread_group_attribute_t m_ThreadGroupAttr;		// SPU thread group attribute
	sys_spu_thread_argument_t m_thread_args;
	TArray<sys_spu_thread_attribute_t> m_ThreadAttr;		// SPU thread attribute
	TArray<sys_spu_thread_t> m_Threads;						// SPU thread IDs
	CStr m_ThreadGroupName;
	TArray<CStr> m_ThreadNames;
	int32 m_ThreadCount;
	int m_Prio;
	VpuContextId m_ContextId;
};

enum { 
	SPU_PROGRAM_COUNT=3,
};

class CVPU_ManagerPlatformData {
public:

	//	sys_spu_thread_t m_Threads[MAX_SPU_THREADS];		/* SPU thread IDs */

	CVPU_Program m_SpuPrograms[VpuContextCount];
};


extern char _binary_VPUWorkers_elf_start[];




MRTC_VPUManager::MRTC_VPUManager()
{
	m_PlatformData= new CVPU_ManagerPlatformData;
	InitContexts();

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

	for (int i=0;i<SPU_PROGRAM_COUNT;i++)
		m_PlatformData->m_SpuPrograms[i].Init();

#ifdef USE_SPU_PRINTF
	ret = spu_printf_server_initialize();
	M_ASSERT(ret==CELL_OK,"spu_printf_server_initialize failed.");
#endif
	int iSpu=0;
	for (int i=0;i<SPU_PROGRAM_COUNT;i++)
		m_PlatformData->m_SpuPrograms[i].InitProgram(iSpu);

}

void MRTC_VPUManager::CreateVpuContext(eVpuPrio _Prio,int32 _ThreadCount,char* _ThreadName,VpuContextId _ContextId,uint32 _SpuEventQueueNumber,uint32 _SpuThreadPort)
{
	m_PlatformData->m_SpuPrograms[_ContextId].m_SpuThreadPort=_SpuThreadPort;
	m_PlatformData->m_SpuPrograms[_ContextId].m_SpuEventQueueNumber=_SpuEventQueueNumber;
	m_PlatformData->m_SpuPrograms[_ContextId].m_ThreadCount=_ThreadCount;
	m_PlatformData->m_SpuPrograms[_ContextId].m_Prio=_Prio;
	m_PlatformData->m_SpuPrograms[_ContextId].m_ThreadGroupName=CStr(_ThreadName);
	m_PlatformData->m_SpuPrograms[_ContextId].m_ThreadNames.SetLen(_ThreadCount);
	m_PlatformData->m_SpuPrograms[_ContextId].m_Threads.SetLen(_ThreadCount);
	m_PlatformData->m_SpuPrograms[_ContextId].m_ThreadAttr.SetLen(_ThreadCount);

	for (int i=0;i<_ThreadCount;i++)
	{
		char str[2];
		str[0]=48+i;
		str[1]=0;
		CStr threadstr=CStr(_ThreadName)+CStr("_")+CStr(str);
		m_PlatformData->m_SpuPrograms[_ContextId].m_ThreadNames[i]=threadstr;
	}
	m_PlatformData->m_SpuPrograms[_ContextId].m_ContextId=_ContextId;

	m_Contexts[_ContextId].m_SelfInSysmem=&m_Contexts[_ContextId];
	m_Contexts[_ContextId].Init();
	m_Contexts[_ContextId].m_pJobs= m_NewProbs[_ContextId].m_Jobs;
	m_Contexts[_ContextId].m_pJobDefData= m_NewProbs[_ContextId].m_JobDefData;
	MemSet(m_Contexts[_ContextId].m_pJobs,0,CVPU_ContextData::MaxJobCount);
	m_Contexts[_ContextId].m_pLock= new MRTC_SpinLock;

}


MRTC_VPUManager::~MRTC_VPUManager()
{
	for (int i=0;i<SPU_PROGRAM_COUNT;i++)
		m_PlatformData->m_SpuPrograms[i].Close();

#ifdef USE_SPU_PRINTF
	int ret = spu_printf_server_finalize();
	M_ASSERT(ret==CELL_OK,"spu_printf_server_finalize failed.");
#endif

	delete m_PlatformData;
	for (int i=0;i<VpuContextCount;i++)
	{
		delete (MRTC_SpinLock*) m_Contexts[i].m_pLock;
		delete [] m_Contexts[i].m_pJobs;
		delete [] m_Contexts[i].m_pJobDefData;
	}
}	

uint16 MRTC_VPUManager::AddTask(const CVPU_JobDefinition& _JobDefinition,bool _Async,VpuContextId _ContextId,uint16 _LinkTaskId)
{
	CVPU_ContextData& Data=m_Contexts[_ContextId];
	((MRTC_SpinLock*) (Data.m_pLock))->Lock();
	uint16 JobIndex = Data.PutJob(_JobDefinition.m_Data,_LinkTaskId);
	if (JobIndex==InvalidVpuTask)
		M_BREAKPOINT;

	if (Data.JobCountInQueue()<64)
	{
		int ret = sys_event_port_send(m_PlatformData->m_SpuPrograms[_ContextId].m_SpuEventPort,mint(&Data),_ContextId,0);
		M_ASSERT(ret==CELL_OK,"Failed to send event to SPU.");
	}

	((MRTC_SpinLock*) Data.m_pLock)->Unlock();
	if (_Async==false)
	{
		BlockOnTask(JobIndex,_ContextId, NULL, NULL);
		return InvalidVpuTask;
	}
	return JobIndex;
}

bool MRTC_VPUManager::IsTaskComplete(uint16 _TaskId,VpuContextId _ContextId)
{
	uint16 TaskId=_TaskId;
	while (TaskId!=InvalidVpuTask)
	{
		M_ASSERT(_TaskId<CVPU_ContextData::MaxJobCount,"");
		if (m_Contexts[_ContextId].m_pJobs[_TaskId]==CVPU_ContextData::JobDone)
			TaskId=InvalidVpuTask;
//			TaskId=m_Contexts[_ContextId].m_pGroupLinks[_TaskId];
		else
			return false;
	}
	return true;
}


void MRTC_VPUManager::BlockOnTask(uint16 _TaskId,VpuContextId _ContextId, FVPUCallManager *_pManager, void *_pManagerContext)
{
	while (!IsTaskComplete(_TaskId,_ContextId))
		;

	return;
#if 1
	while (1)
	{
		sys_event_t EventData;
		int ret = sys_event_queue_receive(m_PlatformData->m_SpuPrograms[_ContextId].m_PpuEventQueue,&EventData,1000000);
		switch (ret)
		{
		case CELL_OK:
			{
				uint32 Data0 = EventData.data2 & 0x00FFFFFF;
				uint32 Data1 = EventData.data3;

				if (Data0 & M_Bit(23))
				{
					Data0 &= ~uint32(M_Bit(23));
					// User Message
					uint32 Ret0 = 0;
					uint32 Ret1 = 0;
					if (_pManager)
					{
						_pManager(_pManagerContext, Data0, Data1, Ret0, Ret1);
					}
					int ret = sys_event_port_send(m_PlatformData->m_SpuPrograms[_ContextId].m_SpuEventPort,Ret0,Ret1,1);
				}
				else
				{
					// System Message
					switch (Data0)
					{
					case 0:
						{
							// Job done, do nothing (check if our job is done below)
							M_IMPORTBARRIER;
						}
						break;
					}
				}


				break;
			}
		case ESRCH:
			{
				M_ASSERT(0,"INVALID EVENT QUEUE ID");
				break;
			}
		case EINVAL:
			{
				M_ASSERT(0,"THIS EVENT QUEUE IS FOR SPU");
				break;
			}
		case ETIMEDOUT:
		case EABORT:
			break;
		default:
			{
				M_ASSERT(0,"UNKNOWN ERROR");
				break;
			}
		}	

		if (m_Contexts[_ContextId].m_pJobs[_TaskId]==CVPU_ContextData::JobDone)
			return;
	}
#else
	while (m_Contexts[_ContextId].m_pJobs[_TaskId]!=CVPU_ContextData::JobDone)
		;
#endif
}

void MRTC_VPUManager::BlockUntilIdle(VpuContextId _ContextId)
{
/*	sys_event_t EventData;
	if (m_Contexts[_ContextId].m_JobQueueTail!=m_Contexts[_ContextId].m_JobQueueHead)
	{
		int ret=0xffffffff & sys_event_queue_receive(m_PlatformData->m_SpuPrograms[_ContextId].m_PpuEventQueue,&EventData,1000000);
		switch (ret)
		{
		case CELL_OK:
			{
				if (EventData.data3==0)
				{
					sys_event_queue_drain(m_PlatformData->m_SpuPrograms[_ContextId].m_PpuEventQueue);
					return;
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
	}
	sys_event_queue_drain(m_PlatformData->m_SpuPrograms[_ContextId].m_PpuEventQueue);
*/
}

void MRTC_VPUManager::SetMultiThreaded(bool _MultiThreaded)
{

}



