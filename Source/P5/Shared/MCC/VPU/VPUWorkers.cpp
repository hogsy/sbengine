
#define M_COMPILING_ON_VPU
#include "VPU_Platform.h"
#include "MRTC_VPU.h"
#include "../VPUShared/MDA_VPUShared.h"

//#pragma optimize( "", off )
//#pragma inline_depth(0)

#include "VPUWorkerShadow.h"
#include "VPUWorkerBspShadow.h"
#include "VPUWorkerCloth.h"



MVPU_MAIN

MVPU_WORKER_BEGIN
MVPU_WORKER_DECLARE(MHASH2('SHAD','OW'),VPUWorker_Shadow);
MVPU_WORKER_DECLARE(MHASH3('BSPS','HADO','W'),VPUWorker_BspShadow);
MVPU_WORKER_DECLARE(MHASH2('CLOT','H'),VPUWorker_Cloth);
MVPU_WORKER_END



#ifndef PLATFORM_SPU

int VPU_Main(CVPU_ContextData& _ContextData,const CVPU_JobDefData* _pJobDefData,bool _IsAsync)
{
	// Setup memory
	char *pScratch = (char *)MRTC_VPU_Win32__GetScratchBuffer(256*1024);	
	uint32 &Offset = *((uint32*)pScratch);
	Offset = 0;

	M_ASSERT(_ContextData.m_pfnVpuWorker,"NoVpuWorker registered");
	if (!_ContextData.m_pfnVpuWorker)
		return 0;
	if (_IsAsync)
	{
		CVPU_JobDefData* pJobDefData=CVPU_WorkQueue::GetJob(_ContextData);
		if (pJobDefData)
		{
			CVPU_JobInfo JobInfo(pJobDefData);
			_ContextData.m_pfnVpuWorker(pJobDefData->m_JobHash,JobInfo);
			M_EXPORTBARRIER;
			CVPU_WorkQueue::SignalJobComplete(_ContextData,pJobDefData->m_JobId);
		}
	}
	else
	{
		CVPU_JobInfo JobInfo(_pJobDefData);
		_ContextData.m_pfnVpuWorker(_pJobDefData->m_JobHash,JobInfo);
	}


	// clear memory
	MRTC_System_VPU::OS_LocalHeapFreeAll();
	return 0;
}

#endif
