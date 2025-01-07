
#ifndef _INC_MOS_MRTC_VPUCOMMONDEFS
#define _INC_MOS_MRTC_VPUCOMMONDEFS

#define EVENT_TERMINATE 0xfee1deadUL

typedef void (FVPUCallManager)(void *_pManagerContext, uint32 _Message, uint32 _Context, uint32 &_Ret0, uint32 &_Ret1);

#ifndef PLATFORM_PS3
class CVPU_JobInfo;
typedef uint32 (*VpuWorkerFunction)(uint32, CVPU_JobInfo&);
#endif

enum VPUBufferType
{
	VPU_IN_BUFFER,
	VPU_OUT_BUFFER,
	VPU_INOUT_BUFFER
};

class M_ALIGN(16) CVPU_OutStreamBufferInfo
{
public:
	void* m_SysAddr;
	uint32 m_SysElementCount;
};

class M_ALIGN(16) CVPU_InStreamBufferInfo
{
public:
	void* m_SysAddr;
	uint32 m_SysElementCount;
};

class M_ALIGN(16) CVPU_SimpleBufferInfo
{
public:
	VPUBufferType m_VPUBufferType;
	void* m_SysAddr;
	uint32 m_ElementCount;
};



class M_ALIGN(16) CVPU_CacheBufferInfo
{
public:
	void* m_SysAddr;
	uint32 m_SysElementCount;
};



union CVPU_ParamData
{
	CVPU_InStreamBufferInfo m_InStreamBufferData;
	CVPU_OutStreamBufferInfo m_OutStreamBufferData;
	CVPU_SimpleBufferInfo m_SimpleBufferData;
	CVPU_CacheBufferInfo m_CacheBufferData;
	vec128 m_VecData;
	uint32 m_LongData[4];
	fp32 m_fp32Data[4];
	mint m_PntrData[2];
};
/*
M_STATIC_ASSERT(sizeof(CVPU_SimpleBufferInfo)==16);
M_STATIC_ASSERT(sizeof(CVPU_InStreamBufferInfo)==16);
M_STATIC_ASSERT(sizeof(CVPU_OutStreamBufferInfo)==16);
M_STATIC_ASSERT(sizeof(CVPU_CacheBufferInfo)==16);

M_STATIC_ASSERT(sizeof(CVPU_ParamData)==16);
*/

class M_ALIGN(128) CVPU_JobDefData
{
public:
	enum { MAX_PARAM_COUNT=31 };
	uint32 m_JobHash;
	uint32 m_JobId;
	uint32 m_Pad1;
	uint32 m_Pad2;
	CVPU_ParamData m_Params[MAX_PARAM_COUNT];
};

class M_ALIGN(128) CVPU_ContextData
{
public:
	enum { MaxJobCount=16384};
	enum { JobQueueSize=2048};

	void* m_pLock;
	CVPU_JobDefData* m_pJobDefData;
	uint8 *volatile m_pJobs;
	CVPU_ContextData* m_SelfInSysmem;
	uint16 m_JobQueueHead;
	uint16 m_JobQueueTail;
	uint16 m_JobIndex;

	M_FORCEINLINE uint16 PutJob(const CVPU_JobDefData& Data,uint16 _LinkTaskId)
	{
		if (m_JobQueueTail == (m_JobQueueHead+1)%JobQueueSize)
			return InvalidVpuTask;
		m_pJobDefData[m_JobQueueHead]=Data;
		m_pJobDefData[m_JobQueueHead].m_JobId=m_JobIndex;
		m_JobQueueHead=(m_JobQueueHead+1)%JobQueueSize;

		uint16 ret=m_JobIndex;
		m_pJobs[m_JobIndex]=JobWaiting;
		m_JobIndex=(m_JobIndex+1)%MaxJobCount;
		return ret;
	}
	M_FORCEINLINE uint16 JobCountInQueue()
	{
		uint16 Headwrap=m_JobQueueHead;
		if (Headwrap<m_JobQueueTail)
			Headwrap+=CVPU_ContextData::JobQueueSize;
		return Headwrap-m_JobQueueTail;
	}

	M_FORCEINLINE CVPU_JobDefData* Get()
	{
		if (IsEmpty()) 
			return NULL;
		CVPU_JobDefData* ret=&m_pJobDefData[m_JobQueueTail];
		m_JobQueueTail=(m_JobQueueTail+1)%JobQueueSize;
		return ret;
	}

	M_FORCEINLINE bool IsEmpty() const { return m_JobQueueHead==m_JobQueueTail; }

	void Init()
	{
		m_JobQueueTail=0;
		m_JobQueueHead=0;
		m_JobIndex=0;
	}
#if !defined(PLATFORM_PS3) && !defined(PLATFORM_SPU)
	VpuWorkerFunction m_pfnVpuWorker;
#endif

	enum JobStatusFlags
	{
		NoJobInitialized=0,
		JobWaiting,
		JobProcessing,
//		JobBlocked,
		JobDone=NoJobInitialized
	};
};

class M_ALIGN(128) ProblemsWithNewForcesThisClass
{
public:
	uint8 m_Jobs[CVPU_ContextData::MaxJobCount];
	CVPU_JobDefData m_JobDefData[CVPU_ContextData::JobQueueSize];
};

//M_STATIC_ASSERT(sizeof(CVPU_ContextData)==128);


#endif

