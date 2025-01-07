#include "MRTC_VPU.h"
#include "VPUWorkerTransformBone.h"


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

class CTM_Edge
{
public:
	uint16 m_iV[2];
};

class CRenderData
{
public:
	uint16 m_iShadowCluster;
	uint16 m_iRenderCluster;
};

/*
void Write(uint8 _data)
{
	uint8vec128 d=si_lqa(_adr);
	d=spu_insert(data,d,_adr);
	si_stqa(d,_adr);
}
*/

uint32 VPUWorker_Shadow(CVPU_JobInfo& _JobInfo)
{
#if defined(VPU_NAMEDEVENTS) && defined(SHADOW_NAMEDEVENTS)
	M_NAMEDEVENT("VPUWorker_TriMeshShadow", 0xff800000);
#endif
//	M_TRACE("SDW VPU: Memory left: %d\r\n", MRTC_System_VPU::OS_LocalHeapMemLeft());

	void* pScratch = MRTC_System_VPU::OS_LocalHeapAlloc(32768);
	M_ASSERT(pScratch!=0,"Failed to get scratchbuffer");

	CVPU_InStreamBuffer<CTM_Triangle,256> TriBuffer(_JobInfo.GetInStreamBuffer(0));
	CVPU_CacheBuffer<CVec3Dfp32,4,128> VertsBuffer(_JobInfo.GetCacheBuffer(1));
	CVPU_SystemAddr pCTM_Cluster=_JobInfo.GetPntrParam(2,0);
	uint CTM_ClusterSize=_JobInfo.GetLParam(2,2);
	uint CTM_ClusterDataOffset=_JobInfo.GetLParam(2,3);;


	CVPU_SimpleBuffer<int32> TriClusterLenBuffer(_JobInfo.GetSimpleBuffer(4));
	CVPU_SimpleBuffer<uint8> CalcClusterBuffer(_JobInfo.GetSimpleBuffer(5));
	CVPU_SimpleBuffer<int32> EdgeClusterLenBuffer(_JobInfo.GetSimpleBuffer(6));
	CVPU_InStreamBuffer<CTM_EdgeTris,256> EdgeTrisBuffer(_JobInfo.GetInStreamBuffer(7));
	
	CVPU_SimpleBuffer<CRenderData> RenderDataBuffer(_JobInfo.GetSimpleBuffer(10));
	CVPU_SystemAddr pIndexLists=_JobInfo.GetPntrParam(11,0);
	const uint32 DoTrans=_JobInfo.GetLParam(11,2);
	const uint32 LightCount=_JobInfo.GetLParam(11,3);
	CVPU_SimpleBuffer<int> nClusterVerticesBuffer(_JobInfo.GetSimpleBuffer(12));
	CVPU_CacheBuffer<CTM_Edge,8,128> EdgeBuffer(_JobInfo.GetCacheBuffer(13));
	CVPU_CacheBuffer<CTM_Triangle,8,128> ShadowTriBuffer(_JobInfo.GetCacheBuffer(14));

	VertexTransformer vertexTransformer;
	CVPU_CacheBuffer<CTM_BDVertexInfo,16,128> VertexInfoBuffer(_JobInfo.GetCacheBuffer(16));
	CVPU_SimpleBuffer<CMat4Dfp32> MatrixPaletteBuffer(_JobInfo.GetSimpleBuffer(15));
	CVPU_CacheBuffer<CTM_BDInfluence,16,128> InfluenceBuffer(_JobInfo.GetCacheBuffer(17));

	CVPU_SystemAddr pnVBVerticies=_JobInfo.GetPntrParam(18,0);

	uint32 nMatrixPal=0;
	if (DoTrans)
	{
		CMat4Dfp32* pMatrixPal=(CMat4Dfp32*)MatrixPaletteBuffer.GetBuffer(nMatrixPal);
		vertexTransformer.Create(&InfluenceBuffer,pMatrixPal,nMatrixPal);
	}
	uint8* pVis=(uint8*) pScratch;

	uint32 nClusterLen,nCalcCluster,dummy;
	int32* pTriClusterLen=(int32*) TriClusterLenBuffer.GetBuffer(nClusterLen);
	uint8* pCalcCluster=(uint8*) CalcClusterBuffer.GetBuffer(nCalcCluster);

	uint32 TriIndex=0;
	uint32 MemReq=0;
	VertexCache vt;
	for (uint32 i=0;i<nClusterLen;i++)
	{
		const uint32 TriTarget= TriIndex+pTriClusterLen[i];
		if (!pCalcCluster[i])
		{
			TriIndex=TriTarget;
			continue;
		}
		TriBuffer.SetPosition(TriIndex);
		while (TriIndex<TriTarget)
		{
			pVis[TriIndex]=0;
			const CTM_Triangle& Tri=*((const CTM_Triangle*) TriBuffer.GetNextElement());
			vec128 v0,v1,v2;
			if (!vt.getVertex(Tri.m_iV[0],v0))
			{
				v0 = M_VLd_P3_Slow(VertsBuffer.GetPntElement(Tri.m_iV[0]));
				if (DoTrans)
					v0=vertexTransformer.TransformVertex(v0,VertexInfoBuffer.GetElement(Tri.m_iV[0]));
				vt.setVertex(Tri.m_iV[0],v0);
			}
			if (!vt.getVertex(Tri.m_iV[1],v1))
			{
				v1 = M_VLd_P3_Slow(VertsBuffer.GetPntElement(Tri.m_iV[1]));
				if (DoTrans)
					v1=vertexTransformer.TransformVertex(v1,VertexInfoBuffer.GetElement(Tri.m_iV[1]));
				vt.setVertex(Tri.m_iV[1],v1);
			}
			if (!vt.getVertex(Tri.m_iV[2],v2))
			{
				v2 = M_VLd_P3_Slow(VertsBuffer.GetPntElement(Tri.m_iV[2]));
				if (DoTrans)
					v2=vertexTransformer.TransformVertex(v2,VertexInfoBuffer.GetElement(Tri.m_iV[2]));
				vt.setVertex(Tri.m_iV[2],v2);
			}
			vec128 n = M_VXpd(M_VSub(v0,v1) , M_VSub(v2,v1));
			for (uint32 j=0;j<LightCount;j++)
			{
				vec128 LightPos=_JobInfo.GetVParam(19+j);
				vec128 DotProd=M_VDp3(n , M_VSub(LightPos,v1));
				if (M_VCmpAnyLT(DotProd,M_VZero()))
				{
					pVis[TriIndex]|=1<<j;
					MemReq+=6*2;
				}
			}
			TriIndex++;
		}
	}

	uint32* pEdgeClusterLen=(uint32*) EdgeClusterLenBuffer.GetBuffer(dummy);
	// Sides
	uint32 EdgeIndex=0;
	for (uint32 i=0;i<nClusterLen;i++)
	{
		const uint32 EdgeTarget= EdgeIndex+pEdgeClusterLen[i];
		if (!pCalcCluster[i])
		{
			EdgeIndex=EdgeTarget;
			continue;
		}
		EdgeTrisBuffer.SetPosition(EdgeIndex);
		while (EdgeIndex<EdgeTarget)
		{
			const CTM_EdgeTris& EdgeTri=*((const CTM_EdgeTris*) EdgeTrisBuffer.GetNextElement());
			const uint16 t0 = EdgeTri.m_iTri[0];
			const uint16 t1 = EdgeTri.m_iTri[1];
			for (uint32 j=0;j<LightCount;j++)
			{
				const uint8 bT0 = (t0 != 0xffff) ? (pVis[t0]& (1<<j)) : 0;
				const uint8 bT1 = (t1 != 0xffff) ? (pVis[t1]& (1<<j)) : 0;
				if (bT0 ^ bT1)
					MemReq	+= 6*2;
			}
			EdgeIndex++;
		}
	}
	MemReq+=24*LightCount;
	union PointersMix
	{
		mint m_mint;
		uint32 m_uint32;
		int* m_pLock;
		uint8** m_ppuint8;
		mint* m_pmint;
		int* m_pint;
	};
	M_STATIC_ASSERT(sizeof(PointersMix)==4);
	PointersMix pAllocPos,pLock,pHeap,pHeapSize;
	pAllocPos.m_uint32=_JobInfo.GetPntrParam(3,0);
	pLock.m_uint32=_JobInfo.GetPntrParam(3,1);
	pHeap.m_uint32=_JobInfo.GetPntrParam(9,0);
	pHeapSize.m_uint32=_JobInfo.GetPntrParam(9,1);
	CVPU_SystemAddr vbAddr=MRTC_System_VPU::OS_VBAlloc(pAllocPos.m_uint32,pLock.m_uint32,pHeap.m_uint32,pHeapSize.m_uint32,MemReq);

	if (vbAddr == 0)
		return 0;

	union PntConv
	{
		mint m_mint;
		uint16* m_puint16;
		uint16*** m_pppuint16;
	};
	PntConv tmp; tmp.m_mint=vbAddr;
	uint16* pPrim=tmp.m_puint16;

	CVPU_OutStreamBufferInfo bi;
	bi.m_SysAddr=(void*)mint(vbAddr);
	bi.m_SysElementCount=MemReq/12;
	CVPU_OutStreamBuffer<uint16[6],1024> PrimBuffer(bi);

	uint32 nDummy=0;
	int* pnClusterVertices=(int*) nClusterVerticesBuffer.GetBuffer(nDummy);
	const CRenderData* pRenderData=(const CRenderData*)	RenderDataBuffer.GetBuffer(nDummy);
	for (uint32 j=0;j<LightCount;j++)
	{
		// Tri-cap
		uint32 TriIndex = 0;
		uint32 EdgeIndex = 0;
		uint32 PPIndex = 0;
		const uint32 iL=_JobInfo.GetLParam(19+j,3);
		const uint8 LightMask=1<<j;
		for (uint32 i=0;i<nClusterLen;i++)
		{
			const uint32 TriTarget= TriIndex+pTriClusterLen[i];
			const uint32 EdgeTarget= EdgeIndex+pEdgeClusterLen[i];
			if (!pCalcCluster[i])
			{
				TriIndex=TriTarget;
				EdgeIndex=EdgeTarget;
				continue;
			}
			const int PPStart = PPIndex;
			const uint16 iRealCluster = pRenderData[i].m_iShadowCluster;
			const CVPU_SystemAddr baseAddr=MRTC_System_VPU::OS_ReadData(pIndexLists+sizeof(void*)*iRealCluster);
			union CTM_Cluster_Info {
				uint32 m_uint32[2];
				struct {
					uint16 m_nIBPrim;
					uint16 m_iVBVert;
					uint32 m_iVB:10;
					uint32 m_iIB:10;
					uint32 m_Flags:4;
					uint32 m_OcclusionIndex:8;
				};
			};
			CTM_Cluster_Info bits;	
			bits.m_uint32[0]= MRTC_System_VPU::OS_ReadData(pCTM_Cluster+CTM_ClusterSize*iRealCluster+CTM_ClusterDataOffset+0);
			bits.m_uint32[1]= MRTC_System_VPU::OS_ReadData(pCTM_Cluster+CTM_ClusterSize*iRealCluster+CTM_ClusterDataOffset+4);
			uint iRealClusterOffset = bits.m_iVBVert;
			uint nRealVBV = MRTC_System_VPU::OS_ReadData(pnVBVerticies+bits.m_iVB*sizeof(int));
			
			PntConv pStart; pStart.m_puint16=pPrim+PPIndex;
			const uint16 nRealV = pnClusterVertices[i];
			while (TriIndex<TriTarget)
			{
				if(pVis[TriIndex]& LightMask)
				{
					const CTM_Triangle& Tri=*(const CTM_Triangle*) ShadowTriBuffer.GetPntElement(TriIndex);
					M_ASSERT(Tri.m_iV[0] < nRealV, "Error");
					M_ASSERT(Tri.m_iV[1] < nRealV, "Error");
					M_ASSERT(Tri.m_iV[2] < nRealV, "Error");
					uint16* pPrimTmp=(uint16*) PrimBuffer.GetNextElement();
					pPrimTmp[0]= iRealClusterOffset+Tri.m_iV[0]+nRealVBV;
					pPrimTmp[1]= iRealClusterOffset+Tri.m_iV[1]+nRealVBV;
					pPrimTmp[2]= iRealClusterOffset+Tri.m_iV[2]+nRealVBV;
					pPrimTmp[3]= iRealClusterOffset+Tri.m_iV[0];
					pPrimTmp[4]= iRealClusterOffset+Tri.m_iV[2];
					pPrimTmp[5]= iRealClusterOffset+Tri.m_iV[1];
					PPIndex+=6;
				}
				TriIndex++;
			}
			// Edges

			{
				EdgeTrisBuffer.SetPosition(EdgeIndex);
				while (EdgeIndex < EdgeTarget)
				{
					const CTM_EdgeTris& EdgeTri=*((const CTM_EdgeTris*) EdgeTrisBuffer.GetNextElement());
					const uint16 t0 = EdgeTri.m_iTri[0];
					const uint16 t1 = EdgeTri.m_iTri[1];
					const uint8 bT0 = (t0 != 0xffff) ? (pVis[t0]& LightMask) : 0;
					const uint8 bT1 = (t1 != 0xffff) ? (pVis[t1]& LightMask) : 0;
					if (bT0 ^ bT1)
					{ 
						const CTM_Edge& Edge = *(const CTM_Edge*) EdgeBuffer.GetPntElement(EdgeIndex);
						const uint16 iv0 = iRealClusterOffset+Edge.m_iV[0];
						const uint16 iv1 = iRealClusterOffset+Edge.m_iV[1];

						uint16* pPrimTmp=(uint16*) PrimBuffer.GetNextElement();
						if (bT0)
						{
							pPrimTmp[0]= iv0;
							pPrimTmp[1]= iv1;
							pPrimTmp[2]= iv1+nRealVBV;
							pPrimTmp[3]= iv0;
							pPrimTmp[4]= iv1+nRealVBV;
							pPrimTmp[5]= iv0+nRealVBV;
						}
						else
						{
							pPrimTmp[0]= iv0;
							pPrimTmp[1]= iv1+nRealVBV;
							pPrimTmp[2]= iv1;
							pPrimTmp[3]= iv0;
							pPrimTmp[4]= iv0+nRealVBV;
							pPrimTmp[5]= iv1+nRealVBV;
						}
						PPIndex+=6;
					}
					EdgeIndex++;
				}

			}
			// Save number of indices used
			//			(((mint *)(pIndexLists[iRealCluster][iL]))[1]) = (iP1 - iP1Start) / 3;
			{
				CVPU_SystemAddr c=MRTC_System_VPU::OS_ReadData(baseAddr+sizeof(void*)*iL);
				MRTC_System_VPU::OS_WriteData(c,1);
				MRTC_System_VPU::OS_WriteData(c+sizeof(void*),(PPIndex-PPStart)/3);
				MRTC_System_VPU::OS_WriteData(c+sizeof(void*)*2,pStart.m_mint);
			}
		}
/*
		while (PPIndex & 7)
		{
			PPIndex+=6;
			PrimBuffer.GetNextElement();
		}
*/
		pPrim+=PPIndex;
		MemReq-=PPIndex*2;
	}
//	M_TRACEALWAYS("%d\n", MemReq);
//	spu_write_decrementer(0xffffffffU);
//	uint32 time=0xffffffffU-spu_read_decrementer();


//	M_TRACE("SDW VPU: Memory left: %d\r\n", MRTC_System_VPU::OS_LocalHeapMemLeft());
	return 0;
};


