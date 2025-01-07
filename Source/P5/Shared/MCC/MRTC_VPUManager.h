#define CDE_EXCLUDE		// Excludes this file from ToolDocen

#ifndef _INC_MOS_MRTC_VPUMANAGER
#define _INC_MOS_MRTC_VPUMANAGER


#include "../Platform/Platform.h"
#include "MRTC_VPUCommonDefs.h"




class M_ALIGN(128) CVPU_JobDefinition
{
	friend class MRTC_VPUManager;
	enum { MAX_TOTAL_VPU_SIZE=256*1024 };

	CVPU_JobDefData m_Data;
	void SetJobId(uint32 _JobId);

public:
	MRTCDLLEXPORT	CVPU_JobDefinition();
	MRTCDLLEXPORT	void SetJob(uint32 _JobHash) { m_Data.m_JobHash=_JobHash; }
	MRTCDLLEXPORT	void AddDependency(uint32 _JobId);
	MRTCDLLEXPORT	void AddCacheBuffer(uint32 paramIndex,void* _Addr,mint _SysElementCount);
	MRTCDLLEXPORT	void AddInStreamBuffer(uint32 paramIndex,void* _Addr,mint _SysElementCount);
	MRTCDLLEXPORT	void AddOutStreamBuffer(uint32 paramIndex,void* _Addr,mint _SysElementCount);
	MRTCDLLEXPORT	void AddSimpleBuffer(uint32 paramIndex,void* _Addr,mint _ElementCount,VPUBufferType BufferType);
	MRTCDLLEXPORT	void AddVParam(uint32 paramIndex,vec128 _Param);
	MRTCDLLEXPORT	void AddLParams(uint32 paramIndex,uint32 _Param0,uint32 _Param1,uint32 _Param2,uint32 _Param3);
	MRTCDLLEXPORT   void AddPntParams(uint32 paramIndex,void* _Param0,void* _Param1);
	MRTCDLLEXPORT   void AddParamData(uint32 paramIndex,const CVPU_ParamData& _Param);
};


class CVPU_ManagerPlatformData;


class MRTC_VPUManager
{
public:

private:
	enum eVpuPrio {
		VpuPrioLow,
		VpuPrioNormal,
		VpuPrioHigh
	};
	CVPU_ManagerPlatformData* m_PlatformData;
	CVPU_ContextData m_Contexts[VpuContextCount];
	ProblemsWithNewForcesThisClass m_NewProbs[VpuContextCount];
	uint32 m_ContextCount;
	bool m_IsInitialized;
	MRTCDLLEXPORT	void CreateVpuContext(eVpuPrio _Prio,int32 _ThreadCount,char* _ThreadName,VpuContextId _ContextIdu,uint32 _SpuEventQueueNumber,uint32 _SpuThreadPort);
	MRTCDLLEXPORT	void InitContexts();
	
public:
	MRTCDLLEXPORT	MRTC_VPUManager();
	MRTCDLLEXPORT	~MRTC_VPUManager();
#ifndef PLATFORM_PS3
	MRTCDLLEXPORT	void RegisterContext(VpuContextId _ContextId,VpuWorkerFunction _pfnVpuWorker);
#endif
	MRTCDLLEXPORT	uint16 AddTask(const CVPU_JobDefinition& _JobDefinition,bool _Async,VpuContextId _ContextId,uint16 _LinkTaskId);
	MRTCDLLEXPORT	bool IsTaskComplete(uint16 _TaskId,VpuContextId _ContextId);
	MRTCDLLEXPORT	void BlockOnTask(uint16 _TaskId,VpuContextId _ContextId, FVPUCallManager *_pManager, void *_pManagerContext);
	MRTCDLLEXPORT	bool TryBlockUntilIdle(VpuContextId _ContextId);
	MRTCDLLEXPORT	void BlockUntilIdle(VpuContextId _ContextId);
	MRTCDLLEXPORT	void SetMultiThreaded(bool _MultiThreaded);
	MRTCDLLEXPORT	bool IsTaskWaiting(VpuContextId _ContextId);
#ifndef PLATFORM_PS3
	MRTCDLLEXPORT	void RunTask(VpuContextId _ContextId);
#endif
};



#endif


