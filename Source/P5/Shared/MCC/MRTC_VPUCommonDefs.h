
#ifndef _INC_MOS_MRTC_VPUCOMMONDEFS
#define _INC_MOS_MRTC_VPUCOMMONDEFS

#define SPU_QUEUE_NUMBER 16850944
#define SPU_THREAD_PORT 58
#define EVENT_TERMINATE 0xfee1deadUL

enum VPUBufferType
{
	VPU_IN_BUFFER,
	VPU_OUT_BUFFER
};

class M_ALIGN(16) CVPU_StreamBufferInfo
{
public:
	VPUBufferType m_VPUBufferType;
	void* m_SysAddr;
	uint32 m_SysElementCount;
	uint16 m_VpuElementCount;
	uint16 m_ElementSize;
};


class M_ALIGN(16) CVPU_CacheBufferInfo
{
public:
	void* m_SysAddr;
	uint32 m_SysElementCount;
	uint16 m_ElementSize;
	uint16 m_pad[3];
};


union CVPU_ParamData
{ 
	CVPU_StreamBufferInfo m_StreamBufferData;
	CVPU_CacheBufferInfo m_CacheBufferData;
	vec128 m_VecData;
	uint32 m_LongData[4];
};

class M_ALIGN(128) CVPU_JobDefData
{
public:
	uint32 m_JobHash;
	uint32 m_JobId;
	uint32 m_Pad1;
	uint32 m_ScratchSize;
	uint32 m_Deps[4];
	CVPU_ParamData m_Params[14];
};



#endif

