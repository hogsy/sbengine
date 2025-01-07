#include "MRTC_VPU.h"

uint32 VPUWorker_TestStream(CVPU_JobInfo& _JobInfo)
{
	CVPU_StreamBufferInfo bIn=_JobInfo.GetStreamBuffer(0);
	CVPU_StreamBufferInfo bOut=_JobInfo.GetStreamBuffer(1);
	CVPU_StreamBuffer BufferIn(bIn,0);
	CVPU_StreamBuffer BufferOut(bOut,1);
	/*
	CMat4Dfp4 Mat;
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
		pDst=(vec128*) BufferOut.GetNextBuffer(count);
		pSrc=(vec128*) BufferIn.GetNextBuffer(count);
		for (uint32 i=0;i<count;i++)
		{
			vec128 a = pSrc[i];
			pDst[i] = a;
		}
	}
	BufferOut.Flush();
	return 0;
};

uint32 VPUWorker_TestCache(CVPU_JobInfo& _JobInfo)
{
	CVPU_CacheBufferInfo bIn=_JobInfo.GetCacheBuffer(0);
	CVPU_StreamBufferInfo bOut=_JobInfo.GetStreamBuffer(1);
	CVPU_CacheBuffer BufferIn(bIn,0);
	CVPU_StreamBuffer BufferOut(bOut,1);

	vec128* pDst=NULL; 

	uint32 cc=BufferOut.GetBufferCount();
	uint32 n=0;
	for (uint32 j=0;j<cc;j++)
	{
		uint32 count;
		pDst=(vec128*) BufferOut.GetNextBuffer(count);
		for (uint32 i=0;i<count;i++)
		{
			vec128 a = BufferIn.GetVecElement(n++);
//			vec128* b = BufferIn.GetPntElement(n++);
			pDst[i] = a;
		}
	}
	BufferOut.Flush();
	return 0;
};

