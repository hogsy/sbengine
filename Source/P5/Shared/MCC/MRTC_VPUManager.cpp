#include "PCH.h"
#include "MRTC_VPUManager.h"
#include "Mrtc.h"

CVPU_JobDefinition::CVPU_JobDefinition()
{	
	m_Data.m_JobHash=0;
	m_Data.m_JobId=0;
}


void CVPU_JobDefinition::AddCacheBuffer(uint32 paramIndex,void* _Addr,mint _SysElementCount)
{
	M_ASSERT(paramIndex<CVPU_JobDefData::MAX_PARAM_COUNT,"Param out of range");
#ifdef PLATFORM_PS3
	M_ASSERT(mint(_Addr)%16==0,"System address must be aligned on 16 bytes");
#endif
	m_Data.m_Params[paramIndex].m_CacheBufferData.m_SysAddr=_Addr;
	m_Data.m_Params[paramIndex].m_CacheBufferData.m_SysElementCount=_SysElementCount;
}

void CVPU_JobDefinition::AddInStreamBuffer(uint32 paramIndex,void* _Addr,mint _SysElementCount)
{
	M_ASSERT(paramIndex<CVPU_JobDefData::MAX_PARAM_COUNT,"Param out of range");
#ifdef PLATFORM_PS3
	M_ASSERT(mint(_Addr)%16==0,"System address must be aligned on 16 bytes");
#endif
	m_Data.m_Params[paramIndex].m_InStreamBufferData.m_SysAddr=_Addr;
	m_Data.m_Params[paramIndex].m_InStreamBufferData.m_SysElementCount=_SysElementCount;
}

void CVPU_JobDefinition::AddOutStreamBuffer(uint32 paramIndex,void* _Addr,mint _SysElementCount)
{
	M_ASSERT(paramIndex<CVPU_JobDefData::MAX_PARAM_COUNT,"Param out of range");
#ifdef PLATFORM_PS3
	M_ASSERT(mint(_Addr)%16==0,"System address must be aligned on 16 bytes");
#endif
	m_Data.m_Params[paramIndex].m_OutStreamBufferData.m_SysAddr=_Addr;
	m_Data.m_Params[paramIndex].m_OutStreamBufferData.m_SysElementCount=_SysElementCount;
}
void CVPU_JobDefinition::AddSimpleBuffer(uint32 paramIndex,void* _Addr,mint _ElementCount,VPUBufferType BufferType)
{
	M_ASSERT(paramIndex<CVPU_JobDefData::MAX_PARAM_COUNT,"Param out of range");
#ifdef PLATFORM_PS3
	M_ASSERT(mint(_Addr)%16==0,"System address must be aligned on 16 bytes");
#endif
	m_Data.m_Params[paramIndex].m_SimpleBufferData.m_VPUBufferType=BufferType;
	m_Data.m_Params[paramIndex].m_SimpleBufferData.m_SysAddr=_Addr;
	m_Data.m_Params[paramIndex].m_SimpleBufferData.m_ElementCount=_ElementCount;
}




void CVPU_JobDefinition::AddVParam(uint32 paramIndex,vec128 _Param)
{
	M_ASSERT(paramIndex<CVPU_JobDefData::MAX_PARAM_COUNT,"Param out of range");
	m_Data.m_Params[paramIndex].m_VecData=_Param;
}


void CVPU_JobDefinition::AddLParams(uint32 paramIndex,uint32 _Param0,uint32 _Param1,uint32 _Param2,uint32 _Param3)
{
	M_ASSERT(paramIndex<CVPU_JobDefData::MAX_PARAM_COUNT,"Param out of range");
	m_Data.m_Params[paramIndex].m_LongData[0]=_Param0;
	m_Data.m_Params[paramIndex].m_LongData[1]=_Param1;
	m_Data.m_Params[paramIndex].m_LongData[2]=_Param2;
	m_Data.m_Params[paramIndex].m_LongData[3]=_Param3;
}


void CVPU_JobDefinition::AddPntParams(uint32 paramIndex,void* _Param0,void* _Param1)
{
	M_ASSERT(paramIndex<CVPU_JobDefData::MAX_PARAM_COUNT,"Param out of range");
	m_Data.m_Params[paramIndex].m_PntrData[0]=mint(_Param0);
	m_Data.m_Params[paramIndex].m_PntrData[1]=mint(_Param1);
}


void CVPU_JobDefinition::AddParamData(uint32 paramIndex,const CVPU_ParamData& _Param)
{
	M_ASSERT(paramIndex<CVPU_JobDefData::MAX_PARAM_COUNT,"Param out of range");
	m_Data.m_Params[paramIndex]=_Param;
}





#ifndef PLATFORM_PS3

class CVPU_ProgramData
{
public:
	NThread::CEventAggregate m_VpuIdleEvent;
	int m_Prio;
};

class CVPU_ManagerPlatformData
{
public:
	bool m_MultiThreaded;
	CVPU_ProgramData m_ProgramData[VpuContextCount];
};


MRTC_VPUManager::MRTC_VPUManager()
{
	m_PlatformData= DNew(CVPU_ManagerPlatformData) CVPU_ManagerPlatformData;
	InitContexts();

	m_PlatformData->m_MultiThreaded=false;
	for (int i=0;i<VpuContextCount;i++)
		m_PlatformData->m_ProgramData[i].m_VpuIdleEvent.Construct();
}

uint32 VPU_Worker(uint32 _JobHash,CVPU_JobInfo& _JobInfo);

void MRTC_VPUManager::CreateVpuContext(eVpuPrio _Prio,int32 _ThreadCount,char* _ThreadName,VpuContextId _ContextId,uint32 _SpuEventQueueNumber,uint32 _SpuThreadPort)
{
	CVPU_ContextData& Data= m_Contexts[_ContextId];
	m_PlatformData->m_ProgramData[_ContextId].m_Prio=_Prio;
	Data.m_SelfInSysmem=&Data;
	Data.Init();
	Data.m_pJobs= m_NewProbs[_ContextId].m_Jobs;
	Data.m_pJobDefData= m_NewProbs[_ContextId].m_JobDefData;
	MemSet(Data.m_pJobs,0,CVPU_ContextData::MaxJobCount);
	Data.m_pLock= DNew(MRTC_SpinLock) MRTC_SpinLock;
	Data.m_pfnVpuWorker=NULL;
	if (_ContextId==VpuWorkersContext)
		RegisterContext(VpuWorkersContext,VPU_Worker);
}

void MRTC_VPUManager::RegisterContext(VpuContextId _ContextId,VpuWorkerFunction _pfnVpuWorker)
{
	m_Contexts[_ContextId].m_pfnVpuWorker=_pfnVpuWorker;
}


MRTC_VPUManager::~MRTC_VPUManager()
{
	for (int i=0;i<VpuContextCount;i++)
	{
		m_PlatformData->m_ProgramData[i].m_VpuIdleEvent.Destruct();
		delete (MRTC_SpinLock*) m_Contexts[i].m_pLock;
	}
	delete m_PlatformData;

}	

int VPU_Main(CVPU_ContextData& _ContextData,const CVPU_JobDefData* _pJobDefData,bool _IsAsync);

uint16 MRTC_VPUManager::AddTask(const CVPU_JobDefinition& _JobDefinition,bool _Async,VpuContextId _ContextId,uint16 _LinkTaskId)
{
	if (m_PlatformData->m_MultiThreaded && _Async)
	{
		CVPU_ContextData& Data= m_Contexts[_ContextId];
		((MRTC_SpinLock*) (Data.m_pLock))->Lock();
		uint16 JobIndex=Data.PutJob(_JobDefinition.m_Data,_LinkTaskId);
		((MRTC_SpinLock*) Data.m_pLock)->Unlock();
		if (JobIndex==InvalidVpuTask)
			M_BREAKPOINT;
		return JobIndex;
	}
	else
	{
		VPU_Main(m_Contexts[_ContextId],(CVPU_JobDefData*)&(_JobDefinition.m_Data),false);
		return InvalidVpuTask;
	}
}

void MRTC_VPUManager::SetMultiThreaded(bool _MultiThreaded)
{
	m_PlatformData->m_MultiThreaded=_MultiThreaded;
}

void MRTC_VPUManager::RunTask(VpuContextId _ContextId)
{
	if (m_PlatformData->m_MultiThreaded)
	{
		VPU_Main(m_Contexts[_ContextId],NULL,true);
		if (!IsTaskWaiting(_ContextId))
			m_PlatformData->m_ProgramData[_ContextId].m_VpuIdleEvent.SetSignaled();
	}
}


bool MRTC_VPUManager::IsTaskComplete(uint16 _TaskId,VpuContextId _ContextId)
{
	if (m_PlatformData->m_MultiThreaded)
	{
		uint16 TaskId=_TaskId;
		while (TaskId!=InvalidVpuTask)
		{
			M_ASSERT(_TaskId<CVPU_ContextData::MaxJobCount,"");
			if (m_Contexts[_ContextId].m_pJobs[_TaskId]==CVPU_ContextData::JobDone)
				TaskId=InvalidVpuTask;
//				TaskId=m_Contexts[_ContextId].m_pGroupLinks[_TaskId];
			else
			{
				M_IMPORTBARRIER;
				return false;
			}
		}
	}
	return true;
}


void MRTC_VPUManager::BlockOnTask(uint16 _TaskId,VpuContextId _ContextId, FVPUCallManager *_pManager, void *_pManagerContext)
{
	// TODO: Implement VPU call manager
	while (!IsTaskComplete(_TaskId,_ContextId))
	{
		if (IsTaskWaiting(VpuWorkersContext))
		{
			RunTask(VpuWorkersContext);
		}
		
	}
}


void MRTC_VPUManager::BlockUntilIdle(VpuContextId _ContextId)
{
	if (m_PlatformData->m_MultiThreaded)
	{
		m_PlatformData->m_ProgramData[_ContextId].m_VpuIdleEvent.TryWait();
		if (IsTaskWaiting(_ContextId))
		{
			m_PlatformData->m_ProgramData[_ContextId].m_VpuIdleEvent.Wait();
		}
	}
}

#endif

#define SPU_AI_QUEUE_NUMBER 16850944
#define SPU_WORKER_QUEUE_NUMBER 16850945
#define SPU_SOUND_QUEUE_NUMBER 16850946

#define SPU_THREAD_PORT_AI 58
#define SPU_THREAD_PORT_WORKER 59
#define SPU_THREAD_PORT_SOUND 60

void MRTC_VPUManager::InitContexts()
{
	CreateVpuContext(VpuPrioLow,1,"AI",VpuAIContext,SPU_AI_QUEUE_NUMBER,SPU_THREAD_PORT_AI);
	CreateVpuContext(VpuPrioNormal,3,"Workers",VpuWorkersContext,SPU_WORKER_QUEUE_NUMBER,SPU_THREAD_PORT_WORKER);
	CreateVpuContext(VpuPrioHigh,1,"Sound",VpuSoundContext,SPU_SOUND_QUEUE_NUMBER,SPU_THREAD_PORT_SOUND);
}


bool MRTC_VPUManager::IsTaskWaiting(VpuContextId _ContextId)
{
	return !m_Contexts[_ContextId].IsEmpty();
}



