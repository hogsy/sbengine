#include "MRTC_VPU.h"



uint32 VPUWorker_Vec128Test(CVPU_JobInfo& _JobInfo)
{

	vec128 v0,v1,v2,v3,v4,v5,v6,v7,v8,v9;
	v0=M_VLd(1.0f,2.0f,3.0f,4.0f);
	v1=M_VLd(4.0f,3.0f,2.0f,1.0f);
	v2=M_VLd(-1.0f,-2.0f,-3.0f,-4.0f);
	v3=M_VLd(1.0f,2.0f,3.0f,4.0f);

	v4=M_VMrgXY(v0,v2);
	v5=M_VMrgZW(v0,v2);
	M_ASSERT(M_VCmpAllEq_u32(v4,v4),"");
	M_ASSERT(M_VCmpAllEq_u32(v4,M_VLd(1.0f,-1.0f,2.0f,-2.0f)),"");
	M_ASSERT(M_VCmpAllEq(v5,M_VLd(3.0f,-3.0f,4.0f,-4.0f)),"");


	v4=M_VShuf(v0,M_VSHUF(0,1,2,3));
	v5=M_VShuf(v0,M_VSHUF(3,2,1,0));
	v6=M_VShuf(v0,M_VSHUF(0,0,0,0));
	v7=M_VShuf(v0,M_VSHUF(1,1,1,1));
	v8=M_VShuf(v0,M_VSHUF(2,2,2,2));
	v9=M_VShuf(v0,M_VSHUF(3,3,3,3));
	M_ASSERT(M_VCmpAllEq_u32(v4,v0),"");
	M_ASSERT(M_VCmpAllEq_u32(v5,v1),"");
	return 0;
};


