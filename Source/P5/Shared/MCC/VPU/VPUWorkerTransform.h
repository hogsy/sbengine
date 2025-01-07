#include "MRTC_VPU.h"

uint32 VPUWorker_TestStream(CVPU_JobInfo& _JobInfo)
{
	CVPU_InStreamBufferInfo bIn=_JobInfo.GetInStreamBuffer(0);
	CVPU_OutStreamBufferInfo bOut=_JobInfo.GetOutStreamBuffer(1);
	CVPU_InStreamBuffer<vec128,64> BufferIn(bIn);
	CVPU_OutStreamBuffer<vec128,64> BufferOut(bOut);
	/*
	CMat4Dfp32 Mat;
	Mat.r[0] = _Msg.m_lVParam[0];
	Mat.r[1] = _Msg.m_lVParam[1];
	Mat.r[2] = _Msg.m_lVParam[2];
	Mat.r[3] = _Msg.m_lVParam[3];
	*/

	vec128* pSrc=NULL;
	vec128* pDst=NULL; 

	uint32 cc=BufferIn.GetBufferCount();
	for (uint32 j=0;j<cc;j++)
	{
		uint32 count;
		pSrc=(vec128*) BufferIn.GetNextBuffer(count);
		for (uint32 i=0;i<count;i++)
		{
			pDst=(vec128*) BufferOut.GetNextElement();
			vec128 a = pSrc[i];
			pDst[0] = a;
		}
	}
	BufferOut.Flush();
	return 0;
};

uint32 VPUWorker_TestCache(CVPU_JobInfo& _JobInfo)
{
	CVPU_CacheBufferInfo bIn=_JobInfo.GetCacheBuffer(0);
	CVPU_OutStreamBufferInfo bOut=_JobInfo.GetOutStreamBuffer(1);
	CVPU_CacheBuffer<float,1> BufferIn(bIn);
	CVPU_OutStreamBuffer<vec128,64> BufferOut(bOut);

	vec128* pDst=NULL; 

	uint32 cc=BufferOut.GetBufferCount();
	uint32 n=0;
	for (uint32 j=0;j<cc;j++)
	{
		uint32 count;
		pDst=(vec128*) BufferOut.GetNextBuffer(count);
		pDst[0]=M_VNaN();
		for (uint32 i=0;i<count;i++)
		{
//			vec128 a = BufferIn.GetVecElement(n++);
//			vec128* b = BufferIn.GetPntElement(n++);
//			pDst[i] = a;
			n++;
		}
	}
	BufferOut.Flush();
	
	return 0;

};

