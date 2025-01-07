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
	uint16 m_TotalVpuSize;
	void SetJobId(uint32 _JobId);

public:
	MRTCDLLEXPORT	CVPU_JobDefinition();
	MRTCDLLEXPORT	void SetJob(uint32 _JobHash) { m_Data.m_JobHash=_JobHash; }
	MRTCDLLEXPORT	void AddDependency(uint32 _JobId);
	MRTCDLLEXPORT	void AddCacheBuffer(uint32 paramIndex,void* _Addr,mint _SysElementCount,uint16 _ElementSize);
	MRTCDLLEXPORT	void AddStreamBuffer(uint32 paramIndex,void* _Addr,mint _SysElementCount,uint16 _VpuElementCount,uint16 _ElementSize,VPUBufferType BufferType);
	MRTCDLLEXPORT	void SetScratchBuffer(uint32 _VpuBufferSize);
	MRTCDLLEXPORT	void AddVParam(uint32 paramIndex,vec128 _Param);
	MRTCDLLEXPORT	void AddLParams(uint32 paramIndex,uint32 _Param0,uint32 _Param1,uint32 _Param2,uint32 _Param3);
};


class MRTC_VPUManagerData;
class MRTC_VPUManager
{
public:

private:
	MRTC_VPUManagerData* m_Data;

	uint m_JobsSent;
	uint m_JobsRecived;

public:
MRTCDLLEXPORT	MRTC_VPUManager();
MRTCDLLEXPORT	~MRTC_VPUManager();
MRTCDLLEXPORT	uint32 AddTask(CVPU_JobDefinition& _JobDefinition);
MRTCDLLEXPORT	bool IsTaskComplete(uint32 _Task);
MRTCDLLEXPORT	void BlockOnTask(uint32 _Task);
MRTCDLLEXPORT	bool TryBlockUntilIdle();
MRTCDLLEXPORT	void BlockUntilIdle();

};



#endif


