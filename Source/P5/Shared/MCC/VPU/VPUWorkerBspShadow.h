#include "MRTC_VPU.h"
#include "../../mos/xr/XRVertexBuffer_VPUShared.h"
#include "../../mos/XRModels/Model_BSP/XWCommon_VPUShared.h"
#include "../../mos/XRModels/Model_BSP2/XW2Common_VPUShared.h"


template <int TSize>
uint Align(uint _Size)
{
	return (_Size + TSize - 1) & ~(TSize - 1);
}

void VpuFillData8(void* _pData, uint64 _Format, uint _Data)
{
	uint64* pD = (uint64*)_pData;
	M_ASSERT(!(_Data & 7), "!");
	_Data >>= 3;
	while(_Data > 0)
	{
		*pD++ = _Format;
		_Data--;
	}
}


fp32 ms_lDiameterClasses[] = 
{ 2.0f, 4.0f, 8.0f, 16.0f, 
32.0f, 48.0f, 64.0f, 96.0f, 
128.0f, 192.0f, 256.0f, 384.0f, 
512.0f, 768.0f, 1024.0f, 2048.0f };

M_FORCEINLINE bool VPU_Light_IntersectSphere_CheckFace(const CBSP2_Face& _Face, vec128 _Pos, vec128 _Radius,vec128 _Plane,
												   CVPU_CacheBuffer<CVec3Dfp32Aggr,4,128>& _VertexBuffer,
												   CVPU_CacheBuffer<uint32,4,64>& _VertexIndexBuffer)
{
	// NOTE: This function doesn't guarantee that there was a hit, only that there was NOT a hit.
{
		uint nv = _Face.m_nPhysV;
		uint iiv = _Face.m_iiPhysV;
		_Radius= M_VMul(_Radius,_Radius);
		uint iv1 = _VertexIndexBuffer[iiv + nv - 1];
		uint iv0 = 0;
		vec128 v0=M_VLd_V3_Slow(_VertexBuffer.GetPntElement(iv0));
		vec128 v1=M_VLd_V3_Slow(_VertexBuffer.GetPntElement(iv1));
		for(uint v = 0; v < nv; v++, v1 = v0)
		{
			iv0 = _VertexIndexBuffer[iiv + v];
			v0=M_VLd_V3_Slow(_VertexBuffer.GetPntElement(iv0));
			vec128 ve=M_VSub(v1,v0);

			vec128 Edge_Plane = M_VXpd(ve,_Plane);
			Edge_Plane = M_VSelMsk(M_VConstMsk(1, 1, 1, 0), Edge_Plane, M_VNeg(M_VDp3(Edge_Plane,v0)));
			vec128 d=M_VDp4(Edge_Plane,M_VSetW1(_Pos));
			vec128 rhs = M_VMul(_Radius,M_VDp3(Edge_Plane,Edge_Plane));
			if (M_VCmpAnyGT(M_VMul(d,d), rhs)) 
				return false;
		}

	}
	return true;
}


class Vec128StoreHelper
{
	CVPU_OutStreamBuffer<CVec3Dfp32[4],32>& m_VertsBuffer;
	vec128 m_V[3];
	uint m_Index;

public:
	M_FORCEINLINE Vec128StoreHelper(CVPU_OutStreamBuffer<CVec3Dfp32[4],32>& VertsBuffer): m_VertsBuffer(VertsBuffer), m_Index(0) {}
	M_FORCEINLINE void Add(vec128 v)
	{
		if (m_Index==3)
		{
			M_VSt_V3x4(m_VertsBuffer.GetNextElement(),m_V[0],m_V[1],m_V[2],v);
			m_Index=0;
		}
		else
			m_V[m_Index++]=v;
	}
};


uint32 VPUWorker_BspShadow(CVPU_JobInfo& _JobInfo)
{
#if defined(VPU_NAMEDEVENTS) && defined(SHADOW_NAMEDEVENTS)
	M_NAMEDEVENT("VPUWorker_BspShadow", 0xff800000);
#endif
//	M_TRACE("BSP VPU: Memory left: %d\r\n", MRTC_System_VPU::OS_LocalHeapMemLeft());

	TBitArray<4096> FaceTag;
	TBitArray<4096> EdgeTag;
	TBitArray<4096> CloseFaceTag;
	union PntConv
	{
		mint			m_mint;
		uint32*			m_puint32;
		CVec3Dfp32*		m_pCVec3Dfp32;
		CXR_VBChain*	m_pVBChain;
	};
	CVPU_InStreamBuffer<uint32,4*16> FaceIndexBuffer(_JobInfo.GetInStreamBuffer(0));
	CVPU_CacheBuffer<uint32,4,64> EdgeIndexBuffer(_JobInfo.GetCacheBuffer(1));
	CVPU_CacheBuffer<uint32,4,64> VertexIndexBuffer(_JobInfo.GetCacheBuffer(2));

	CVPU_CacheBuffer<CVec3Dfp32Aggr,4,128> VertexBuffer(_JobInfo.GetCacheBuffer(3));
	CVPU_CacheBuffer<CBSP2_Face,4,128> FaceBuffer(_JobInfo.GetCacheBuffer(4));
	CVPU_CacheBuffer<CPlane3Dfp32,4,128> PlaneBuffer(_JobInfo.GetCacheBuffer(5));
	CVPU_CacheBuffer<CBSP_EdgeFaces,4,128> EdgeFaceBuffer(_JobInfo.GetCacheBuffer(6));
	CVPU_CacheBuffer<CBSP_Edge,4,128> EdgeBuffer(_JobInfo.GetCacheBuffer(7));
	
	const vec128 LightPos = _JobInfo.GetVParam(10);
	
	vec128 LightRange = M_VLdScalar(_JobInfo.GetFParam(11,0));
	vec128 Light_CurrentFrontFaceShadowMul = M_VLdScalar(_JobInfo.GetFParam(11,1));
	
	PntConv pVBChain; pVBChain.m_mint = _JobInfo.GetPntrParam(12,0);


	const uint32 nFaces = FaceIndexBuffer.GetSysElementCount();
	uint32 nEdges = EdgeIndexBuffer.Len();
//	const uint32 niVertices = VertexIndexBuffer.Len();
	const uint32 nVertices = VertexBuffer.Len();
	uint8* pSP = (uint8*)MRTC_System_VPU::OS_LocalHeapAlloc( Align<16>(sizeof(uint16) * nEdges) + Align<16>(sizeof(uint16) * nVertices));
	uint16* pNewiEdges = (uint16*)pSP; pSP += Align<16>(sizeof(uint16) * nEdges);
	uint16* piVertMap = (uint16*)pSP; pSP += Align<16>(sizeof(uint16) * nVertices);

	VpuFillData8(piVertMap, (uint64)0, Align<16>(sizeof(uint16) * nVertices));


	// Get all unique edges from front-facing faces.

	FaceTag.ClearFirstBits(FaceBuffer.Len());
	EdgeTag.ClearFirstBits(EdgeBuffer.Len());
	CloseFaceTag.ClearFirstBits(FaceBuffer.Len());

	uint32 EdgeCount = 0;
	uint nFaceTri = 0;
	uint nFaceVert = 0;
	uint nCloseFaceVert = 0;
	{
		const uint32 nMaxEdges = nEdges;

		{
			for(uint32 f = 0; f < nFaces; f++)
			{
				uint32 iFace = *FaceIndexBuffer.GetNextElement();
				
				const CBSP2_Face& rF = FaceBuffer.GetElement(iFace);
				// Cull faces backfacing the lightsource.
				if (rF.m_iiEdges == XW_FACE2_INVALIDINDEX)
					continue;
				M_ASSERT(rF.m_iiEdges < nEdges, "Edge index out of range");

				nFaceTri += rF.m_nVertices-2;
				nFaceVert += rF.m_nVertices;

				vec128 Plane=PlaneBuffer.GetElement(rF.m_iPlane).v;
				vec128 a=M_VDp4(Plane,LightPos);
				vec128 Dist = M_VAbs(a);
				const fp32 FaceMaxDiam = ms_lDiameterClasses[rF.m_iDiameterClass];
				const vec128 FaceMaxDiamV = M_VLdScalar(FaceMaxDiam);
				if (M_VCmpAllLT(Dist, FaceMaxDiamV)  &&
					VPU_Light_IntersectSphere_CheckFace(rF, LightPos, FaceMaxDiamV, Plane, VertexBuffer, VertexIndexBuffer))
				{
					// Do very slow and safe shadow volume
					nCloseFaceVert += rF.m_nVertices;
					CloseFaceTag.Set1(f);
				}
				else
				{
					// Tag face
					FaceTag.Set1(iFace);
					for(uint32 e = 0; e < rF.m_nVertices; e++)
					{
						uint16 iE = EdgeIndexBuffer[rF.m_iiEdges + e];
						if (EdgeTag.GetThenSet1(iE)) 
							continue;
						if (EdgeCount >= nMaxEdges)
						{
							M_BREAKPOINT;
							return 0;
						}

						pNewiEdges[EdgeCount++] = iE;
					}
				}
			}
		}
	}

	// Remove all edges that have both it's faces front-facing.
	{
		int nEdgesCulled= 0;
		for(uint32 e = 0; e < EdgeCount; e++)
		{
			uint16 iE = pNewiEdges[e];
			const CBSP_EdgeFaces& EF = EdgeFaceBuffer.GetElement(iE);
			int Face0 = EF.IsValid(0) && FaceTag.Get(EF.GetFace(0));
			int Face1 = EF.IsValid(1) && FaceTag.Get(EF.GetFace(1));

			Face0 = (Face0) ? 1 : 0;
			Face1 = (Face1) ? 1 : 0;

			// This should never happen, and that seems to work so we can skip the test.
			//			if (!(Face0 || Face1)) Error("StencilLight_Create", "Internal error.");

			if (Face0 ^ Face1)
				pNewiEdges[nEdgesCulled++] = iE;
		}
		EdgeCount = nEdgesCulled;
	}

	// 0.5773502691896257645091487805008f = Sqrt(1 - (Sqrt(2) / (2*cos(30)))^2)


	{
		// Get vertexcount to VBAlloc
		uint16 nV = 0;
		{
			for(uint32 e = 0; e < EdgeCount; e++)
			{
				const uint16 iE = pNewiEdges[e];
				const CBSP_Edge E = EdgeBuffer.GetElement(iE);
				const CBSP_EdgeFaces EF = EdgeFaceBuffer.GetElement(iE);
				const uint32 iv0 = E.m_iV[0];
				const uint32 iv1 = E.m_iV[1];
				piVertMap[iv0] = 2;
				piVertMap[iv1] = 2;
			}

			FaceIndexBuffer.SetPosition(0);
			for(uint32 f = 0; f < nFaces; f++)
			{
				int iFace = *FaceIndexBuffer.GetNextElement();
				const CBSP2_Face& rF = FaceBuffer.GetElement(iFace);

				if (rF.m_iiEdges == XW_FACE2_INVALIDINDEX)
					continue;

				int nv = rF.m_nVertices;
				for(int v = 0; v < nv; v++)
				{
					uint32 iv0 = VertexIndexBuffer[v + rF.m_iiVertices];
					piVertMap[iv0]=2;
				}
				int bCloseProjection = CloseFaceTag.Get(f);
				nV += bCloseProjection*3*nv;
			}
		}
		for (uint i=0;i<nVertices;i++)
			nV+=piVertMap[i];
		VpuFillData8(piVertMap, ~(uint64)0, Align<16>(sizeof(uint16) * nVertices));

		vec128 ShadowCastRange = M_VMul(LightRange, M_VScalar(1.0f / 0.5773502691896257645091487805008f) );
		vec128 two=M_VTwo();

		int nPrim = 3 * (nFaceTri*2 + nCloseFaceVert*8 + EdgeCount*2);

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
		pAllocPos.m_uint32=_JobInfo.GetPntrParam(13,0);
		pLock.m_uint32=_JobInfo.GetPntrParam(13,1);
		pHeap.m_uint32=_JobInfo.GetPntrParam(14,0);
		pHeapSize.m_uint32=_JobInfo.GetPntrParam(14,1);
		nV=Align<4>(nV);
		CVPU_SystemAddr PrimVBAddr=MRTC_System_VPU::OS_VBAlloc(pAllocPos.m_uint32,pLock.m_uint32,pHeap.m_uint32,pHeapSize.m_uint32,Align<16>(nPrim*sizeof(int16)) + nV*sizeof(CVec3Dfp32));
		CVPU_SystemAddr VertsVBAddr=PrimVBAddr + Align<16>(nPrim*sizeof(int16));
		if (PrimVBAddr == 0)
			return 0;
		if (VertsVBAddr == 0)
			return 0;
		CVPU_OutStreamBufferInfo pbi;
		pbi.m_SysAddr=(void*)mint(PrimVBAddr);
		pbi.m_SysElementCount=nPrim/6;
		CVPU_OutStreamBuffer<uint16[6],64> PrimBuffer(pbi);

		CVPU_OutStreamBufferInfo vbi;
		vbi.m_SysAddr=(void*)mint(VertsVBAddr);
		vbi.m_SysElementCount=nV/4;
		CVPU_OutStreamBuffer<CVec3Dfp32[4],32> VertsBuffer(vbi);

		Vec128StoreHelper StoreHelper(VertsBuffer);

#ifdef CPU_BIGENDIAN
#define PACKPRIM(a,b) ((uint32(a)<<16) | uint32(b))
#else
#define PACKPRIM(a,b) (uint32(a) | (uint32(b)<<16))
#endif


		nV=0;
		{
			for(uint32 e = 0; e < EdgeCount; e++)
			{
				uint16 iE = pNewiEdges[e];
				CBSP_Edge E = EdgeBuffer.GetElement(iE);
				CBSP_EdgeFaces EF = EdgeFaceBuffer.GetElement(iE);

				uint32 iv0 = E.m_iV[0];
				uint32 iv1 = E.m_iV[1];
				uint32 Face0 = FaceTag.Get(EF.GetFace(0));
				if (Face0) Swap(iv0, iv1);

				uint16 iv0vb = piVertMap[iv0];
				if (iv0vb == 0xffff)
				{
					vec128 Vertex=M_VLd_V3_Slow(VertexBuffer.GetPntElement(iv0));
					vec128 dV0Nrm=M_VNrm3_Est(M_VSub(Vertex,LightPos));
					StoreHelper.Add(M_VMAdd(two,dV0Nrm,Vertex));
					StoreHelper.Add(M_VMAdd(ShadowCastRange,dV0Nrm,LightPos));
					piVertMap[iv0] = iv0vb = nV;
					nV += 2;
				}

				uint16 iv1vb = piVertMap[iv1];
				if (iv1vb == 0xffff)
				{
					vec128 Vertex=M_VLd_V3_Slow(VertexBuffer.GetPntElement(iv1));
					vec128 dV0Nrm=M_VNrm3_Est(M_VSub(Vertex,LightPos));
					StoreHelper.Add(M_VMAdd(two,dV0Nrm,Vertex));
					StoreHelper.Add(M_VMAdd(ShadowCastRange,dV0Nrm,LightPos));
					piVertMap[iv1] = iv1vb = nV;
					nV += 2;
				}

				uint32* pPrimTmp=(uint32*) PrimBuffer.GetNextElement();
				pPrimTmp[0] = PACKPRIM(iv0vb,iv1vb+1);
				pPrimTmp[1] = PACKPRIM(iv1vb,iv0vb);
				pPrimTmp[2] = PACKPRIM(iv0vb+1,iv1vb+1);
			}

			FaceIndexBuffer.SetPosition(0);
			for(uint32 f = 0; f < nFaces; f++)
			{
				int iFace = *FaceIndexBuffer.GetNextElement();
				const CBSP2_Face& rF = FaceBuffer.GetElement(iFace);

				if (rF.m_iiEdges == XW_FACE2_INVALIDINDEX)
					continue;

				uint16 liV[64];
				int nv = rF.m_nVertices;
				{
					for(int v = 0; v < nv; v++)
					{
						uint32 iv0 = VertexIndexBuffer[v + rF.m_iiVertices];
						uint16 iv0vb = piVertMap[iv0];
						if (iv0vb == 0xffff)
						{
							vec128 Vertex=M_VLd_V3_Slow(VertexBuffer.GetPntElement(iv0));
							vec128 dV0Nrm=M_VNrm3_Est(M_VSub(Vertex,LightPos));
							StoreHelper.Add(M_VMAdd(two,dV0Nrm,Vertex));
							StoreHelper.Add(M_VMAdd(ShadowCastRange,dV0Nrm,LightPos));
							piVertMap[iv0] = iv0vb = nV;
							nV += 2;
						}
						liV[v] = iv0vb;
					}
				}
				int bCloseProjection = CloseFaceTag.Get(f);
				if (bCloseProjection)
				{
					{
						vec128 N = PlaneBuffer.GetElement(rF.m_iPlane).v;
						uint16 iVPolyBackSide = nV;
						{
							vec128 tmp=M_VMul(Light_CurrentFrontFaceShadowMul,ShadowCastRange);
							for(int v = 0; v < nv; v++)
							{
								int iv0 = VertexIndexBuffer[v + rF.m_iiVertices];
								StoreHelper.Add(M_VMAdd(N, tmp, M_VLd_V3_Slow(VertexBuffer.GetPntElement(iv0))));
							}
							nV+=nv;
						}

						int iVProjEdge = nV;
						{
							vec128 vec0=M_VLd_V3_Slow(VertexBuffer.GetPntElement(VertexIndexBuffer[0 + rF.m_iiVertices]));
							for(int v = 0; v < nv; v++)
							{
								int v1 = v+1;
								if (v1 >= nv)
									v1 = 0;
								uint32 iv1 = VertexIndexBuffer[v1 + rF.m_iiVertices];

								vec128 vec1=M_VLd_V3_Slow(VertexBuffer.GetPntElement(iv1));
								vec128 E=M_VSub(vec1,vec0);
								vec128 L=M_VSub(LightPos,vec0);
								vec128 Proj=M_VMul(M_VDp3(E,L),M_VRcp_Est(M_VDp3(E,E)));
								vec128 Ep=M_VMul(E,Proj);
								vec128 Et=M_VNrm3(M_VSub(Ep,L));
								StoreHelper.Add(M_VMAdd(Et,ShadowCastRange,vec0));
								StoreHelper.Add(M_VMAdd(Et,ShadowCastRange,vec1));
								vec0=vec1;
							}
							nV+=2*nv;
						}

						uint16 iv0vb = liV[0];
						for(int v = 2; v < nv; v++)
						{
							// Front of SV
							uint32* pPrimTmp=(uint32*) PrimBuffer.GetNextElement();
							pPrimTmp[0] = PACKPRIM(iv0vb,liV[v]);
							pPrimTmp[1] = PACKPRIM(liV[v-1],iVPolyBackSide);
							pPrimTmp[2] = PACKPRIM(iVPolyBackSide + v-1,iVPolyBackSide + v);
						}

						uint16 ivb1 = liV[nv-1]+1;
						uint16 ivf1 = liV[nv-1];
						uint16 iw1 = iVPolyBackSide + nv-1;
						uint16 ie0 = iVProjEdge + (nv-1)*2+1;
						uint16 ie1 = iVProjEdge + (nv-1)*2;

						for(int e = 0; e < nv; e++)
						{
							uint16 ivb0 = liV[e]+1;
							uint16 ivf0 = liV[e];
							uint16 iw0 = iVPolyBackSide + e;

							// Front
							uint32* pPrimTmp=(uint32*) PrimBuffer.GetNextElement();
							pPrimTmp[0] = PACKPRIM(ivf1,ivf0);
							pPrimTmp[1] = PACKPRIM(ie0,ivf1);
							pPrimTmp[2] = PACKPRIM(ie0,ie1);

							pPrimTmp=(uint32*) PrimBuffer.GetNextElement();
							pPrimTmp[0] = PACKPRIM(ivf0,ivb0);
							pPrimTmp[1] = PACKPRIM(ie0,ivf1);
							pPrimTmp[2] = PACKPRIM(ie1,ivb1);

							// Back
							pPrimTmp=(uint32*) PrimBuffer.GetNextElement();
							pPrimTmp[0] = PACKPRIM(ie1,ie0);
							pPrimTmp[1] = PACKPRIM(iw0,ie1);
							pPrimTmp[2] = PACKPRIM(iw0,iw1);

							pPrimTmp=(uint32*) PrimBuffer.GetNextElement();
							pPrimTmp[0] = PACKPRIM(ivb0,iw0);
							pPrimTmp[1] = PACKPRIM(ie0,ivb1);
							pPrimTmp[2] = PACKPRIM(ie1,iw1);

							ivb1 = ivb0;
							ivf1 = ivf0;
							iw1 = iw0;
							ie0 = iVProjEdge + e*2+1;
							ie1 = iVProjEdge + e*2;
						}
					}
				}
				else
				{
					uint16 iv0vb = liV[0];
					for(int v = 2; v < nv; v++)
					{
						// Front of SV first 3
						// Back of SV last 3
						uint32* pPrimTmp=(uint32*) PrimBuffer.GetNextElement();
						pPrimTmp[0] = PACKPRIM(iv0vb,liV[v]);
						pPrimTmp[1] = PACKPRIM(liV[v-1],iv0vb+1);
						pPrimTmp[2] = PACKPRIM(liV[v-1]+1,liV[v]+1);
					}
				}
			}

		}

		StoreHelper.Add(M_VZero());		// Force store of cached data
		StoreHelper.Add(M_VZero());
		StoreHelper.Add(M_VZero());
		union  {
			uint32 m_uint32;
			struct {
				uint32 m_nV:16;
				uint32 m_nMWComp:16;		// Weight components per vertex, allowed range is [0..4] (works as m_nTVComp)
			};
		} VBBits;
		union {
			uint32 m_PrimData;
			struct {
				uint16 m_nPrim;
				int16 m_PrimType;
			};
		} VBPrimData;

		M_OFFSET(CXR_VBChain, m_pV,pvOffset);
		M_OFFSET(CXR_VBChain, m_DummyOffset,nvOffset);
		M_OFFSET(CXR_VBChain, m_piPrim,piPrimOffset);
		M_OFFSET(CXR_VBChain, m_PrimData,PrimDataOffset);

		MRTC_System_VPU::OS_WriteData(pVBChain.m_mint+pvOffset, VertsVBAddr);
		VBBits.m_uint32 = MRTC_System_VPU::OS_ReadData(pVBChain.m_mint+nvOffset);
		VBBits.m_nV = nV;
		MRTC_System_VPU::OS_WriteData(pVBChain.m_mint+nvOffset, VBBits.m_uint32);

		VBPrimData.m_nPrim = nPrim / 3;
		VBPrimData.m_PrimType = CRC_RIP_TRIANGLES;
		MRTC_System_VPU::OS_WriteData(pVBChain.m_mint+piPrimOffset, PrimVBAddr);
		MRTC_System_VPU::OS_WriteData(pVBChain.m_mint+PrimDataOffset, VBPrimData.m_PrimData);
//		M_TRACE("BSP VPU: Memory left: %d\r\n", MRTC_System_VPU::OS_LocalHeapMemLeft());
		return 0;
	}
}


