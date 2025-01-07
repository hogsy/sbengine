#include "MRTC_VPU.h"

class CVec3Bundle
{
	fp4 v[12];

public:
	vec128 getV(uint32 index) const
	{
		return M_VLd(v[3*index],v[3*index+1],v[3*index+2],0.0f);
	}
};
class CTM_Triangle
{
public:
	uint16 m_iV[3];
};
class CTM_EdgeTris
{
public:
	uint16 m_iTri[2];
};



uint32 VPUWorker_Shadow(CVPU_JobInfo& _JobInfo)
{

	CVPU_CacheBufferInfo bTris=_JobInfo.GetCacheBuffer(0);
	CVPU_CacheBufferInfo bVerts=_JobInfo.GetCacheBuffer(1);
	vec128 LightPos=_JobInfo.GetVParam(2);
	CVPU_StreamBufferInfo bTriVis=_JobInfo.GetStreamBuffer(3);
	CVPU_StreamBufferInfo bTriClusterLen=_JobInfo.GetStreamBuffer(4);
	CVPU_StreamBufferInfo bCalcCluster=_JobInfo.GetStreamBuffer(5);
	CVPU_StreamBufferInfo bEdgeClusterLen=_JobInfo.GetStreamBuffer(6);
	CVPU_CacheBufferInfo bEdgeTris=_JobInfo.GetCacheBuffer(7);
	CVPU_StreamBufferInfo bRetData=_JobInfo.GetStreamBuffer(8);

	uint32 DmaChannel=1;
	CVPU_CacheBuffer TriBuffer(bTris,DmaChannel++);
	CVPU_CacheBuffer VertsBuffer(bVerts,DmaChannel++);
	CVPU_StreamBuffer TriVisBuffer(bTriVis,DmaChannel++);
	CVPU_StreamBuffer TriClusterLenBuffer(bTriClusterLen,DmaChannel++);
	CVPU_StreamBuffer CalcClusterBuffer(bCalcCluster,DmaChannel++);
	CVPU_StreamBuffer EdgeClusterLenBuffer(bEdgeClusterLen,DmaChannel++);
	CVPU_CacheBuffer EdgeTrisBuffer(bEdgeTris,DmaChannel++);
	CVPU_StreamBuffer RetDataBuffer(bRetData,DmaChannel++);

//	_JobInfo.GetScratchBufferInfo(pVis,nVis);
	uint32 nClusterLen,nCalcCluster,MaxTriVis,dummy;
	int32* pTriClusterLen=(int32*) TriClusterLenBuffer.GetBuffer(nClusterLen);
	uint8* pCalcCluster=(uint8*) CalcClusterBuffer.GetBuffer(nCalcCluster);
	uint8* pVis=(uint8*) TriVisBuffer.GetBuffer(MaxTriVis);
	uint32* pEdgeClusterLen=(uint32*) EdgeClusterLenBuffer.GetBuffer(dummy);;
	uint32* pRetData=(uint32*) RetDataBuffer.GetBuffer(dummy);
	uint32 nVis=0;
	uint32 TriIndex=0;
	uint32 TriTarget=0;
	uint32 MemReq=0;
	for (uint32 i=0;i<nClusterLen;i++)
	{
		TriTarget= TriIndex+pTriClusterLen[i];
		if (!pCalcCluster[i])
		{
			TriIndex=TriTarget;
			continue;
		}
		while (TriIndex<TriTarget)
		{
			const CTM_Triangle& Tri=((CTM_Triangle*) TriBuffer.GetPntElement(TriIndex/8))[TriIndex%8];
			vec128 v0 = ((const CVec3Bundle*) VertsBuffer.GetPntElement(Tri.m_iV[0]/4))->getV(Tri.m_iV[0]%4);
			vec128 v1 = ((const CVec3Bundle*) VertsBuffer.GetPntElement(Tri.m_iV[1]/4))->getV(Tri.m_iV[1]%4);
			vec128 v2 = ((const CVec3Bundle*) VertsBuffer.GetPntElement(Tri.m_iV[2]/4))->getV(Tri.m_iV[2]%4);
			vec128 n = M_VXpd(M_VSub(v0,v1) , M_VSub(v2,v1));
			vec128 DotProd=M_VDp3(n , M_VSub(LightPos,v1));
			if (M_VGetX_Slow(DotProd)<0)
			{
				pVis[TriIndex]=1;
				nVis++;
				MemReq+=6;
			}
			else
				pVis[TriIndex]=0;
			TriIndex++;
		}
	}
	// Sides
	uint32 EdgeTarget=0;
	uint32 EdgeIndex=0;
	for (uint32 i=0;i<nClusterLen;i++)
	{
		EdgeTarget= EdgeIndex+pEdgeClusterLen[i];
		if (!pCalcCluster[i])
		{
			EdgeIndex=EdgeTarget;
			continue;
		}
		while (EdgeIndex<EdgeTarget)
		{
			const CTM_EdgeTris& EdgeTri=((CTM_EdgeTris*) EdgeTrisBuffer.GetPntElement(EdgeIndex/8))[EdgeIndex%8];
			int t0 = EdgeTri.m_iTri[0];
			int t1 = EdgeTri.m_iTri[1];
			int bT0 = (t0 != 0xffff) ? (pVis[t0]) : 0;
			int bT1 = (t1 != 0xffff) ? (pVis[t1]) : 0;
			if (bT0 ^ bT1)
				MemReq	+= 6;
			EdgeIndex++;
		}
	}
	pRetData[0]=MemReq;
	return 0;
};


