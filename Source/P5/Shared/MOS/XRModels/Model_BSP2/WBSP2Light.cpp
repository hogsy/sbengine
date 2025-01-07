#include "PCH.h"

#include "MFloat.h"
#include "WBSP2Model.h"
#include "WBSP2Def.h"
#include "../../../mcc/MRTC_VPUManager.h"

// #define MODEL_BSP_NOATTRSHARING
// -------------------------------------------------------------------
//  Stuff in MImage.cpp
// -------------------------------------------------------------------

extern int aShiftMulTab[];

M_FORCEINLINE void UnAlignedStore(CVec3Dfp32* pv,vec128 v)
{
#if defined(PLATFORM_XENON) || defined(PLATFORM_PS3)
	/*
	vec128 l=__lvlx((void*) pv, 0);
	vec128 r=__lvrx((void*) pv, 16);
	vec128 res= M_VOr(r,l);
	v=M_VSelMsk(M_VConstMsk(0,0,0,1),res,v);
	*/
	__stvlx(v,(void*)pv,0);
	__stvrx(v,(void*)pv,16);
#else
	*pv=M_VGetV3_Slow(v);
#endif
}

void PPA_Mul_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);
void PPA_MulAddSaturate_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);

void PPA_Mul_RGBA32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);





// -------------------------------------------------------------------
//  CXR_Model_BSP2
// -------------------------------------------------------------------
bool CXR_Model_BSP2::Light_IntersectSphere_CheckFace(const CBSP2_Face* _pFace, const CVec3Dfp32& _Pos, fp32 _Radius)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_IntersectSphere_CheckFace, false);
	// NOTE: This function doesn't guarantee that there was a hit, only that there was NOT a hit.

	const CBSP2_Face* pF = _pFace;
	int iPlane = pF->m_iPlane;
	const CPlane3Dfp32* pP = &m_pPlanes[iPlane];
/*	fp32 d = pP->Distance(_Pos);
	if (d < -_Radius) return false;
	if (d > _Radius) return false;
*/
	CPlane3Dfp32 Edge_Plane;

	fp32 RSqr = Sqr(_Radius);

	int nv = pF->m_nPhysV;
	int iiv = pF->m_iiPhysV;

	int iv1 = m_piVertices[iiv + nv - 1];
	int iv0 = 0;
	for(int v = 0; v < nv; v++, iv1 = iv0)
	{
		iv0 = m_piVertices[iiv + v];
		CVec3Dfp32 ve, n;
		m_pVertices[iv1].Sub(m_pVertices[iv0], ve);
		ve.CrossProd(pP->n, Edge_Plane.n);
		Edge_Plane.d = -Edge_Plane.n*m_pVertices[iv0];

		fp32 d = Edge_Plane.Distance(_Pos);
		if (d > 0.0f)
		{
			fp32 NSqrLen = Edge_Plane.n.LengthSqr();
			if (Sqr(d) > RSqr*NSqrLen) return false;
		}

	}

	return true;
}


#include "../../XR/XRShader.h"



bool CXR_Model_BSP2::Light_RenderShading(CBSP2_RenderParams* _pRenderParams, CXR_VertexBuffer* _pVB, const CXR_Light* _pL, CXW_Surface* _pSurf)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_ProjectDynamic_NV20, false);
	// This is for NV20+ really..

	CXW_SurfaceKeyFrame* pSurfKey = (_pRenderParams->m_pCurAnimState)
		?
			_pSurf->GetFrame(
				_pSurf->m_iController ? _pRenderParams->m_pCurAnimState->m_Anim1 : _pRenderParams->m_pCurAnimState->m_Anim0, 
				_pSurf->m_iController ? _pRenderParams->m_pCurAnimState->m_AnimTime1 : _pRenderParams->m_pCurAnimState->m_AnimTime0)
		:
			_pSurf->GetBaseFrame();

	CXR_Shader* pShader = _pRenderParams->m_pCurrentEngine->GetShader();

	CXR_SurfaceShaderParams SSP;
	CXR_ShaderParams Params;
	SSP.Create(_pSurf, pSurfKey);
	Params.Create(_pRenderParams->m_pVBMatrixM2W, _pRenderParams->m_pVBMatrixW2V, pShader);
	const CXR_SurfaceShaderParams* lpSSP[1] = { &SSP };
	pShader->RenderShading(*const_cast<CXR_Light*>(_pL), _pVB, &Params, lpSSP);

	return false;
}

void CXR_Model_BSP2::Light_RenderShading_FaceQueues(CBSP2_RenderParams* _pRenderParams, const CXR_Light* _pL, const CMat4Dfp32* _pWMat, const CMat4Dfp32* _pVMat)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_RenderFaceQueues, MAUTOSTRIP_VOID);

	TMeasureProfile(m_TimeBrushVB);

	CXR_Shader* pShader = _pRenderParams->m_pCurrentEngine->GetShader();

	TAP<CXR_SurfaceShaderParams> lSSP = m_lSurfaceShaderParams;

	{
		CBSP2_VBFaceQueue* pQ = _pRenderParams->m_pVBFaceQueue;
		uint nQ = m_lVBFaceQueues.Len();
		for(int iVB = 0; iVB < nQ; iVB++)
		{
			const uint32* pFQ = pQ[iVB].m_pQueue;
			int nFQ = pQ[iVB].m_nElem;
			if (!nFQ)
				continue;

			pQ[iVB].m_nElem = 0;

			CXR_VertexBuffer VB;
//			CXR_VertexBuffer* pVB = _pRenderParams->m_pVBM->Alloc_VB();
//			if (!pVB) continue;
			VB.Matrix_Set(_pRenderParams->m_pVBMatrixM2V);
			
			CBSP2_VBChain Chain;
			VB_CopyGeometry(_pRenderParams, iVB, &Chain);
			Chain.SetToVB(&VB);

			if (RenderTesselateVBPrim(pFQ, nFQ, _pRenderParams->m_pVBM, &VB))
			{
				const CBSP2_VBInfo* pBSPVBI = &m_lVBInfo[iVB];

				CXR_ShaderParams Params;
//				SSP.Create(m_lspSurfaces[pBSPVBI->m_iSurface]->GetBaseFrame());
				Params.Create(_pWMat, _pVMat, pShader);
				const CXR_SurfaceShaderParams* lpSSP[1] = { &lSSP[pBSPVBI->m_iSurface] };
				pShader->RenderShading(*const_cast<CXR_Light*>(_pL), &VB, &Params, lpSSP);
			}
		}
	}
}


#define MODEL_BSP2_MAX_LIT_FACES 2048

#define MODEL_BSP2_MAXDYNAMICLIGHTFACES 1024
#define MODEL_BSP2_MAX_LIT_EDGES 4096


TThinArray<uint32> g_BSP2liVertMap;
TThinArray<uint32> g_BSP2liVertMapClear;
NThread::CSpinLock ShadowVolumeLock;

template <int TSize>
uint Align(uint _Size)
{
	return (_Size + TSize - 1) & ~(TSize - 1);
}

class CMoominScopeLock
{
	void *m_pLock;
	NThread::FLock *m_pUnlockFunc;
public:

	template <typename t_Lock>
	class TLocker
	{
	public:
		static void Locker(void *_pLock)
		{
			((t_Lock *)_pLock)->Unlock();
		}
	};

	CMoominScopeLock()
	{
		m_pLock = 0;
		m_pUnlockFunc = 0;
	}

	template <typename t_Lock>
	void Create(t_Lock &_Lock)
	{
		_Lock.Lock();
		m_pLock = &_Lock;
		m_pUnlockFunc = TLocker<t_Lock>::Locker;
	}

	void Destroy()
	{
		if(m_pLock)
		{
			m_pUnlockFunc(m_pLock);
			m_pLock = NULL;
		}
	}

	~CMoominScopeLock()
	{
		if(m_pLock)
			m_pUnlockFunc(m_pLock);
	}
};

void FillData8(void* _pData, uint64 _Format, uint _Data)
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




CXR_VertexBuffer* CXR_Model_BSP2::Light_CreateShadowVolume(CBSP2_RenderParams* _pRenderParams, CBSP2_DynamicLightContext* _pDynLightContext, const CXR_Light& _Light, const uint32* _piFaces, int _nFaces)
{
	MSCOPESHORT(Light_CreateShadowVolume);

	if (!_nFaces)
		return NULL;

	if (_Light.m_Type != CXR_LIGHTTYPE_POINT &&
		_Light.m_Type != CXR_LIGHTTYPE_SPOT)
		return NULL;
	CVec3Dfp32 LightPosX = _Light.GetPosition();
	vec128 LightPos = M_VLd_P3_Slow(&LightPosX);

	const uint VpuMemMax = 50*1024;
	if (1 && 
		(VpuMemMax > Align<16>(m_liEdges.Len()*sizeof(uint16)) + Align<16>(m_lVertices.Len() * sizeof(uint16))) &&
		_nFaces < 4096 &&
		m_liEdges.Len() < 4096)
	{
#if defined(SHADOW_NAMEDEVENTS)
		M_NAMEDEVENT("Light_SetupVpuBspShadow", 0xffff0000);
#endif

		CXR_VertexBuffer* pVB = _pRenderParams->m_pVBM->Alloc_VB();
		if (!pVB)
			return NULL;
		if (!pVB->AllocVBChain(_pRenderParams->m_pVBM, false))
			return NULL;
		CXR_VBChain* pVBChain = pVB->GetVBChain();
		pVBChain->m_PrimType = CRC_RIP_TRIANGLES;
		pVBChain->m_nPrim = 0;
		pVBChain->m_nV = 0;
		pVBChain->m_pV = 0;
		pVBChain->m_piPrim = 0;

		CVPU_JobDefinition JobDef;
		JobDef.AddInStreamBuffer(0,(void*) _piFaces,_nFaces);
		JobDef.AddCacheBuffer(1,(void*) m_liEdges.GetBasePtr(),m_liEdges.Len());
		JobDef.AddCacheBuffer(2,(void*) m_liVertices.GetBasePtr(),m_liVertices.Len());

		JobDef.AddCacheBuffer(3,(void*) m_pVertices,m_lVertices.Len());
		JobDef.AddCacheBuffer(4,(void*) m_pFaces,m_lFaces.Len());
		JobDef.AddCacheBuffer(5,(void*) m_pPlanes,m_lPlanes.Len());
		JobDef.AddCacheBuffer(6,(void*) m_lEdgeFaces.GetBasePtr(), m_lEdgeFaces.Len());
		JobDef.AddCacheBuffer(7,(void*) m_lEdges.GetBasePtr(), m_lEdges.Len());

		JobDef.AddVParam(10,LightPos);
		CVPU_ParamData Params;
		Params.m_fp32Data[0] = _Light.m_Range;
		Params.m_fp32Data[1] = _pDynLightContext->m_Light_CurrentFrontFaceShadowMul;
		Params.m_LongData[2] = VpuMemMax;
		JobDef.AddParamData(11,Params);

		JobDef.AddPntParams(12, (void*) pVBChain, NULL);
		union PointersMix
		{
			mint m_mint;
			NThread::CSpinLock* m_SpinLock;
			uint8** m_ppuint8;
			mint* m_pmint;
			int* m_pint;
			void* m_pvoid;
			uint16*** m_pppuint16;
		};
		PointersMix AllocPos,Lock,Heap,HeapSize;
		AllocPos.m_mint=Lock.m_mint=Heap.m_mint=HeapSize.m_mint=0;
		_pRenderParams->m_pVBM->GetAllocParams(AllocPos.m_pint,Lock.m_SpinLock,Heap.m_ppuint8,HeapSize.m_pmint);
		JobDef.AddPntParams(13,AllocPos.m_pvoid,Lock.m_pvoid);
		JobDef.AddPntParams(14,Heap.m_pvoid,HeapSize.m_pvoid);

		JobDef.SetJob(MHASH3('BSPS','HADO','W'));
		uint16 taskId = MRTC_ThreadPoolManager::VPU_AddTask(JobDef,VpuWorkersContext);
		pVB->Render_SetVPUTask(taskId);

//		MRTC_ThreadPoolManager::VPU_BlockOnTask(taskId,VpuWorkersContext);
//		M_TRACEALWAYS("NEW: VBChain %d:\n",pVB->GetVBChain());
//		bThread=true;
		return pVB;
	}
#if defined(SHADOW_NAMEDEVENTS)
	M_NAMEDEVENT("Light_CreateShadow_BigModels", 0xffcc0000);
#endif
	const uint32* lFaces = _piFaces;
	//_nFaces = Min(_nFaces, 0x100);
	int nFaces = _nFaces;

	TObjectPoolAllocator<CBSP2_EnumContext> EnumAlloc(m_spEnumContextPool);
	CBSP2_EnumContext* pEnumContext = EnumAlloc.GetObject();
	pEnumContext->Create(m_lFaces.Len(), 0);
	pEnumContext->CreateEdges(m_lEdges.Len());

	uint8* pET = pEnumContext->m_lEdgeTag.GetBasePtr();;

	const uint32 WorstCaseRequiredMemory =Align<16>((_nFaces + 7) >> 3) + Align<16>(sizeof(uint32) * m_liEdges.Len()) + Align<16>(m_lVertices.Len() * (sizeof(uint32) * 1));

	bool bThread = false;
	const uint ExtraSaveSpace=4;		// Faster to save 16 bytes than 12

	// Make sure there is room for atleast 2048 vertices aswell
	const int TotalMemReq = WorstCaseRequiredMemory + 2048 * sizeof(CVec3Dfp32)+ExtraSaveSpace;

	if(TotalMemReq < 255 * 1024)
		bThread = true;
	

	uint8* pSP = NULL;
	uint8* pTagCloseFace = NULL;
	uint32* liEdges = NULL;
	uint32* piVertMap = NULL;
	uint32* piVertMapClear = NULL;
	CMoominScopeLock ShadowVolumeLocker;
	if(bThread)
	{
		pSP = (uint8*)MRTC_ScratchPadManager::Get(255 * 1024);
		pTagCloseFace = pSP; pSP += Align<16>((_nFaces + 7) >> 3);
		liEdges = (uint32*)pSP; pSP += Align<16>(sizeof(uint32) * m_liEdges.Len());
		piVertMap = (uint32*)pSP; pSP += Align<16>(sizeof(uint32) * m_lVertices.Len());
		//		piVertMapClear = (uint32*)pSP; pSP += Align<16>(sizeof(uint32) * m_lVertices.Len());

		FillData8(piVertMap, ~(uint64)0, Align<16>(sizeof(uint32) * m_lVertices.Len()));
		//		FillChar(piVertMap, sizeof(uint32) * m_lVertices.Len(), 0xff);
		//		FillChar(piVertMapClear, sizeof(uint32) * m_lVertices.Len(), 0);
	}
	else
	{
		ShadowVolumeLocker.Create(ShadowVolumeLock);
		pTagCloseFace = (uint8*)MRTC_ScratchPadManager::Get((_nFaces + 7) >> 3);

		if (g_BSP2liVertMap.Len() < m_lVertices.Len())
		{
			g_BSP2liVertMap.SetLen(m_lVertices.Len());
			g_BSP2liVertMapClear.SetLen(m_lVertices.Len());
			FillChar(g_BSP2liVertMap.GetBasePtr(), g_BSP2liVertMap.ListSize(), 0xff);
			FillChar(g_BSP2liVertMapClear.GetBasePtr(), g_BSP2liVertMapClear.ListSize(), 0);
		}

		piVertMap = g_BSP2liVertMap.GetBasePtr();
		piVertMapClear = g_BSP2liVertMapClear.GetBasePtr();
	}


	// Get all unique edges from front-facing faces.
	if (!pTagCloseFace)
		return NULL;
	FillChar(pTagCloseFace, (_nFaces + 7) >> 3, 0);

	int nEdges = 0;
	int nFaceTri = 0;
	int nFaceVert = 0;
	int nCloseFaceVert = 0;
	{
		CMoominScopeLock AllocLock;
		int nMaxEdges;
		if(!bThread)
		{
			AllocLock.Create(_pRenderParams->m_pVBM->m_AllocLock);
			liEdges = (uint32*)_pRenderParams->m_pVBM->Alloc_Open();
			nMaxEdges = _pRenderParams->m_pVBM->GetAvail() >> 2;
		}
		else
			nMaxEdges = m_liEdges.Len();

		const uint32* piEdges = m_liEdges.GetBasePtr();
		{
			//			int nFacesNew = 0;
			for(int f = 0; f < nFaces; f++)
			{
				int iFace = lFaces[f];
				const CBSP2_Face* pF = &m_pFaces[iFace];

				// Cull faces backfacing the lightsource.
				if (pF->m_iiEdges == XW_FACE2_INVALIDINDEX)
					continue;

				nFaceTri += pF->m_nVertices-2;
				nFaceVert += pF->m_nVertices;

				vec128 plane=m_pPlanes[pF->m_iPlane].v;
				vec128 a=M_VDp4(plane,LightPos);
				vec128 Dist = M_VAbs(a);
				vec128 FaceMaxDiamV = M_VLdScalar(ms_lDiameterClasses[pF->m_iDiameterClass]);
				if (M_VCmpAllLT(Dist, FaceMaxDiamV)  &&
					Light_IntersectSphere_CheckFace(pF, LightPosX, ms_lDiameterClasses[pF->m_iDiameterClass]))
				{
					// Do very slow and safe shadow volume
					nCloseFaceVert += pF->m_nVertices;;
					pTagCloseFace[f >> 3] |= aShiftMulTab[f & 7];
				}
				else
				{
					// Tag face
					int iFaceIdx = iFace >> 3;
					int iFaceMask = aShiftMulTab[iFace & 7];
					if (!pEnumContext->m_pFTag[iFaceIdx])
					{
						if(pEnumContext->m_nUntagEnum >= pEnumContext->m_MaxUntagEnum)
							M_ASSERT(0, "?");
						pEnumContext->m_piFUntag[pEnumContext->m_nUntagEnum++] = iFaceIdx;
					}
					pEnumContext->m_pFTag[iFaceIdx] |= iFaceMask;

					for(int e = 0; e < pF->m_nVertices; e++)
					{
						int iE = piEdges[pF->m_iiEdges + e];
						int eidx=iE >> 3;
						int emask=aShiftMulTab[iE & 7];
						if (pET[eidx] & emask) continue;
						pET[eidx] |= emask;

						if (nEdges >= nMaxEdges)
						{
							ConOut(CStrF("§cf80WARNING: (CXR_Model_BSP2::StencilLight_Create) Too many edges. (Face %d/%d)", f, nFaces));
							{
								// Clear edge tags
								for(int e = 0; e < nEdges; e++)
									pET[liEdges[e] >> 3] = 0;
							}
							pEnumContext->Untag();
							return NULL;
						}

						liEdges[nEdges++] = iE;
					}
				}
			}
		}

		if(!bThread)
		{
			_pRenderParams->m_pVBM->Alloc_Close(nEdges * 4);
		}
	}

	// Clear edge tags
	{
		for(int e = 0; e < nEdges; e++)
			pET[liEdges[e] >> 3] = 0;
	}

	// Remove all edges that have both it's faces front-facing.
	{
		const CBSP_EdgeFaces* pEdgeFaces = m_lEdgeFaces.GetBasePtr();

		int nEdgesCulled= 0;
		for(int e = 0; e < nEdges; e++)
		{
			int iE = liEdges[e];
			pET[iE >> 3] = 0;

			const CBSP_EdgeFaces& EF = pEdgeFaces[iE];
			int Face0 = EF.IsValid(0) && (pEnumContext->m_pFTag[EF.GetFace(0) >> 3] & (aShiftMulTab[EF.GetFace(0) & 7]));
			int Face1 = EF.IsValid(1) && (pEnumContext->m_pFTag[EF.GetFace(1) >> 3] & (aShiftMulTab[EF.GetFace(1) & 7]));

			Face0 = (Face0) ? 1 : 0;
			Face1 = (Face1) ? 1 : 0;

			// This should never happen, and that seems to work so we can skip the test.
			//			if (!(Face0 || Face1)) Error("StencilLight_Create", "Internal error.");

			if (Face0 ^ Face1)
				liEdges[nEdgesCulled++] = iE;
		}
		nEdges = nEdgesCulled;
	}

	int nVertMapClear = 0;

	// Create geometry
	//	fp32 MinDistance = m_BoundBox.GetMinDistance(LightPos);
	//	fp32 ShadowCastRange = (_Light.m_Range - MinDistance) * 2.0f;


	// 0.5773502691896257645091487805008f = Sqrt(1 - (Sqrt(2) / (2*cos(30)))^2)

	vec128 ShadowCastRange = M_VMul(M_VLdScalar(_Light.m_Range), M_VScalar(1.0f / 0.5773502691896257645091487805008f) );
	vec128 two=M_VTwo();
	// + 1.0f * Min(_Light.m_Range, 512.0f);

	int nPrim = 3 * (nFaceTri*2 + nCloseFaceVert*8 + nEdges*2);
	//	int nV = nEdges*4 + nFaceVert*2;

#ifdef CPU_BIGENDIAN
	#define PACKPRIM(a,b) (((a)<<16) | (b))
#else
	#define PACKPRIM(a,b) ((a) | ((b)<<16))
#endif

	uint32* piPrimTri = (uint32*) _pRenderParams->m_pVBM->Alloc_Int16(nPrim);
	uint32* pVertMap = piVertMap;
	uint32* pVertMapClear = piVertMapClear;
	CVec3Dfp32* pV = NULL;
	int nV = 0;
	int nP = 0;
	{
		CMoominScopeLock AllocLock;
		int nMaxV;
		if(!bThread)
		{
			AllocLock.Create(_pRenderParams->m_pVBM->m_AllocLock);
			pV = (CVec3Dfp32*) _pRenderParams->m_pVBM->Alloc_Open();
			nMaxV = (_pRenderParams->m_pVBM->GetAvail() - ExtraSaveSpace)/ sizeof(CVec3Dfp32);
		}
		else
		{
			nMaxV = (255 * 1024 - ExtraSaveSpace - WorstCaseRequiredMemory) / sizeof(CVec3Dfp32);
			pV = (CVec3Dfp32*)pSP; pSP += Align<16>(nMaxV * sizeof(CVec3Dfp32));
		}

		if (!pV || !piPrimTri)
		{
			pEnumContext->Untag();
			return NULL;
		}

		if (nMaxV > 32768)
			nMaxV = 32768;


		{
			const CBSP_EdgeFaces* pEdgeFaces = m_lEdgeFaces.GetBasePtr();
			const CBSP_Edge* pEdges = m_lEdges.GetBasePtr();

			for(int e = 0; e < nEdges; e++)
			{
				int iE = liEdges[e];
				const CBSP_Edge& E = pEdges[iE];
				const CBSP_EdgeFaces& EF = pEdgeFaces[iE];

				int iv0 = E.m_iV[0];
				int iv1 = E.m_iV[1];
				int Face0 = (pEnumContext->m_pFTag[EF.GetFace(0) >> 3] & (aShiftMulTab[EF.GetFace(0) & 7]));
				if (Face0) Swap(iv0, iv1);

				int iv0vb = pVertMap[iv0];
				if (iv0vb == ~0)
				{
					if (nV+2 > nMaxV)
						goto ErrorQuit;
					iv0vb = nV;
					//				pV[iv0vb] = m_pVertices[iv0];
					vec128 Vertex=M_VLd_V3_Slow(&m_pVertices[iv0]);
					vec128 dV0Nrm=M_VNrm3_Est(M_VSub(Vertex,LightPos));
					UnAlignedStore(&pV[iv0vb],  M_VMAdd(two,dV0Nrm,Vertex));
					UnAlignedStore(&pV[iv0vb+1],M_VMAdd(ShadowCastRange,dV0Nrm,LightPos));
					pVertMap[iv0] = nV;
					if(pVertMapClear) pVertMapClear[nVertMapClear++] = iv0;
					nV += 2;
				}

				int iv1vb = pVertMap[iv1];
				if (iv1vb == ~0)
				{
					if (nV+2 > nMaxV)
						goto ErrorQuit;
					iv1vb = nV;
					//				pV[iv1vb] = m_pVertices[iv1];
					vec128 Vertex=M_VLd_V3_Slow(&m_pVertices[iv1]);
					vec128 dV0Nrm=M_VNrm3_Est(M_VSub(Vertex,LightPos));
					UnAlignedStore(&pV[iv1vb],  M_VMAdd(two,dV0Nrm,Vertex));
					UnAlignedStore(&pV[iv1vb+1],M_VMAdd(ShadowCastRange,dV0Nrm,LightPos));
					pVertMap[iv1] = nV;
					if(pVertMapClear) pVertMapClear[nVertMapClear++] = iv1;
					nV += 2;
				}

				piPrimTri[nP++] = PACKPRIM(iv0vb,iv1vb+1);
				piPrimTri[nP++] = PACKPRIM(iv1vb,iv0vb);
				piPrimTri[nP++] = PACKPRIM(iv0vb+1,iv1vb+1);
			}

			//			int iV = nEdges*4;

			for(int f = 0; f < nFaces; f++)
			{
				int iFace = lFaces[f];
				const CBSP2_Face* pF = &m_pFaces[iFace];

				if (pF->m_iiEdges == XW_FACE2_INVALIDINDEX)
					continue;

				const uint32* piV = &m_piVertices[pF->m_iiVertices];

				uint32 liV[64];

				int nv = pF->m_nVertices;
				{
					for(int v = 0; v < nv; v++)
					{
						int iv0 = piV[v];
						int iv0vb = pVertMap[iv0];
						if (iv0vb == ~0)
						{
							if (nV+2 > nMaxV)
								goto ErrorQuit;
							iv0vb = nV;
							//						pV[iv0vb] = m_pVertices[iv0];
							vec128 Vertex=M_VLd_V3_Slow(&m_pVertices[iv0]);
							vec128 dV0Nrm=M_VNrm3_Est(M_VSub(Vertex,LightPos));
							UnAlignedStore(&pV[iv0vb],  M_VMAdd(two,dV0Nrm,Vertex));
							UnAlignedStore(&pV[iv0vb+1],M_VMAdd(ShadowCastRange,dV0Nrm,LightPos));
							pVertMap[iv0] = nV;
							if(pVertMapClear) pVertMapClear[nVertMapClear++] = iv0;
							nV += 2;
						}
						liV[v] = iv0vb;

						if (nV+2 > nMaxV)
							goto ErrorQuit;

					}
				}

				int bCloseProjection = pTagCloseFace[f >> 3] & (aShiftMulTab[f & 7]);

				if (bCloseProjection)
				{
					if (nV+nv*3 > nMaxV)
						goto ErrorQuit;

					{
						vec128 N = m_lPlanes[pF->m_iPlane].v;

						int iVPolyBackSide = nV;
						{
							vec128 tmp=M_VMul(M_VLdScalar(_pDynLightContext->m_Light_CurrentFrontFaceShadowMul),ShadowCastRange);
							for(int v = 0; v < nv; v++)
							{
								int iv0 = piV[v];
								UnAlignedStore(&pV[nV],M_VMAdd(N, tmp, M_VLd_V3_Slow(&m_pVertices[iv0])));
								nV++;
							}
						}

						int iVProjEdge = nV;
						{
							vec128 vec0=M_VLd_V3_Slow(&m_pVertices[piV[0]]);
							for(int v = 0; v < nv; v++)
							{
								int v1 = v+1;
								if (v1 >= nv)
									v1 = 0;
								int iv1 = piV[v1];
								
								vec128 vec1=M_VLd_V3_Slow(&m_pVertices[iv1]);
								vec128 E=M_VSub(vec1,vec0);
								vec128 L=M_VSub(LightPos,vec0);
								vec128 Proj=M_VMul(M_VDp3(E,L),M_VRcp_Est(M_VDp3(E,E)));
								vec128 Ep=M_VMul(E,Proj);
								vec128 Et=M_VNrm3(M_VSub(Ep,L));
								UnAlignedStore(&pV[nV],  M_VMAdd(Et,ShadowCastRange,vec0));
								UnAlignedStore(&pV[nV+1],M_VMAdd(Et,ShadowCastRange,vec1));
								nV += 2;
								vec0=vec1;
							}
						}

						int iv0vb = liV[0];
						for(int v = 2; v < nv; v++)
						{
							// Front of SV
							piPrimTri[nP++] = PACKPRIM(iv0vb,liV[v]);
							piPrimTri[nP++] = PACKPRIM(liV[v-1],iVPolyBackSide);
							piPrimTri[nP++] = PACKPRIM(iVPolyBackSide + v-1,iVPolyBackSide + v);
						}

						int ivb1 = liV[nv-1]+1;
						int ivf1 = liV[nv-1];
						int iw1 = iVPolyBackSide + nv-1;
						int ie0 = iVProjEdge + (nv-1)*2+1;
						int ie1 = iVProjEdge + (nv-1)*2;

						for(int e = 0; e < nv; e++)
						{
							int ivb0 = liV[e]+1;
							int ivf0 = liV[e];
							int iw0 = iVPolyBackSide + e;

							// Front
							piPrimTri[nP++] = PACKPRIM(ivf1,ivf0);
							piPrimTri[nP++] = PACKPRIM(ie0,ivf1);
							piPrimTri[nP++] = PACKPRIM(ie0,ie1);

							piPrimTri[nP++] = PACKPRIM(ivf0,ivb0);
							piPrimTri[nP++] = PACKPRIM(ie0,ivf1);
							piPrimTri[nP++] = PACKPRIM(ie1,ivb1);

							// Back
							piPrimTri[nP++] = PACKPRIM(ie1,ie0);
							piPrimTri[nP++] = PACKPRIM(iw0,ie1);
							piPrimTri[nP++] = PACKPRIM(iw0,iw1);

							piPrimTri[nP++] = PACKPRIM(ivb0,iw0);
							piPrimTri[nP++] = PACKPRIM(ie0,ivb1);
							piPrimTri[nP++] = PACKPRIM(ie1,iw1);

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
					int iv0vb = liV[0];
					for(int v = 2; v < nv; v++)
					{
						// Front of SV first 3
						// Back of SV last 3
						piPrimTri[nP++] = PACKPRIM(iv0vb,liV[v]);
						piPrimTri[nP++] = PACKPRIM(liV[v-1],iv0vb+1);
						piPrimTri[nP++] = PACKPRIM(liV[v-1]+1,liV[v]+1);
					}
				}
			}

			// Check that we didnt write more or less than we planned for in the beginning.
			if (nP*2 != nPrim)
				Error("StencilLight_Create", "Internal error. (3)");
		}

		if(!bThread)
		{
			_pRenderParams->m_pVBM->Alloc_Close(nV*12);
		}
		else
		{
			CVec3Dfp32* pVCopy = _pRenderParams->m_pVBM->Alloc_V3(nV);
			if(!pVCopy)
			{
				pEnumContext->Untag();
				return NULL;
			}
			memcpy(pVCopy, pV, sizeof(CVec3Dfp32) * nV);
			pV = pVCopy;
		}
	}

	if(!bThread)
	{
		// No need to do this when threaded since it'll be trashed anyway
		for(int i = 0; i < nVertMapClear; i++)
			pVertMap[pVertMapClear[i]] = ~0;
	}

	// Clear face tags
	pEnumContext->Untag();
	{
		CXR_VertexBuffer* pVB = _pRenderParams->m_pVBM->Alloc_VB();
		if (!pVB)
			return NULL;
		if (!pVB->AllocVBChain(_pRenderParams->m_pVBM, false))
			return NULL;

		pVB->Geometry_VertexArray(pV, nV, true);
		pVB->Render_IndexedTriangles((uint16*) piPrimTri, 2*nP / 3);
//		M_TRACEALWAYS("OLD: VBChain %d:\n",pVB->GetVBChain());

		return pVB;
	}

ErrorQuit:
	if(!bThread)
	{
		for(int i = 0; i < nVertMapClear; i++)
			pVertMap[pVertMapClear[i]] = ~0;
	}

	// Clear face tags
	pEnumContext->Untag();
	return NULL;
}


void CXR_Model_BSP2::Light_BuildShadowFaces_Point(CBSP2_EnumContext* _pEnumContext, CBSP2_DynamicLightContext* _pDynLightContext, const uint32* _piFaces, int _nFaces)
{
//	ConOut(CStrF("Light_FlagTree, Leaf  nFaces %d, iiFaces %d", nFaces, iiFaces));
	for(int f = 0; f < _nFaces; f++)
	{
		int iFace = _piFaces[f];
		int iFaceIdx = iFace >> 3;
		int iFaceMask = aShiftMulTab[iFace & 7];

		if (_pEnumContext->m_pFTag[iFaceIdx] & iFaceMask) continue;
		if (!_pEnumContext->m_pFTag[iFaceIdx])
		{
			if(_pEnumContext->m_nUntagEnum >= _pEnumContext->m_MaxUntagEnum)
				M_ASSERT(0, "?");
			_pEnumContext->m_piFUntag[_pEnumContext->m_nUntagEnum++] = iFaceIdx;
		}
		_pEnumContext->m_pFTag[iFaceIdx] |= iFaceMask;

		const CBSP2_Face* pF = &m_pFaces[iFace];
		if (!(pF->m_Flags & XW_FACE_SHADOWCASTER))
			continue;
//		int Flags = pF->m_Flags;

/*		CXW_Surface* pSurf = m_lspSurfaces[pF->m_iSurface];

		if (pSurf->m_Flags & XW_SURFFLAGS_INVISIBLE)
			continue;
		if (pSurf->GetBaseFrame()->m_Medium.m_MediumFlags & XW_MEDIUM_SEETHROUGH)
			continue;
*/
		fp32 Dist = _pDynLightContext->m_Light_CurrentFrontFaceShadowMul * m_pPlanes[pF->m_iPlane].Distance(_pDynLightContext->m_Light_CurrentPos);
		if (Dist > 0.0f)
			continue;
		if (Dist < -_pDynLightContext->m_Light_CurrentRange)
			continue;

		if (!Light_IntersectSphere_CheckFace(pF, _pDynLightContext->m_Light_CurrentPos, _pDynLightContext->m_Light_CurrentRange))
			continue;

		if (_pEnumContext->m_nEnum < _pEnumContext->m_MaxEnum)
			_pEnumContext->m_piEnum[_pEnumContext->m_nEnum++] = iFace;
	}

}

void CXR_Model_BSP2::Light_BuildShadowFaces_Point(CBSP2_EnumContext* _pEnumContext, CBSP2_DynamicLightContext* _pDynLightContext, int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_BuildShadowFaces_Point, MAUTOSTRIP_VOID);

	CVec3Dfp32 LightCurrentPos = _pDynLightContext->m_Light_CurrentPos;
	fp32 LightCurrentRange = _pDynLightContext->m_Light_CurrentRange;

	int aWorkStack[256];
	uint32 iWorkPos = 0;
	int iWorkNode = _iNode;

	goto StartOf_Light_BuildShadowFaces_Point_NoAdd;

StartOf_Light_BuildShadowFaces_Point:
	iWorkNode = aWorkStack[--iWorkPos];

StartOf_Light_BuildShadowFaces_Point_NoAdd:

	const CBSP2_Node* pN = &m_pNodes[iWorkNode];

	if (pN->IsLeaf())
	{
		int nFaces = pN->m_nFaces;
		int iiFaces = pN->m_iiFaces;

		Light_BuildShadowFaces_Point(_pEnumContext, _pDynLightContext, &m_piFaces[iiFaces], nFaces);
		if (_pEnumContext->m_nEnum >= _pEnumContext->m_MaxEnum) return;
	}
	else
	{
		int iPlane = pN->m_iPlane;
		fp32 Dist = m_pPlanes[iPlane].Distance(LightCurrentPos);
		if (Dist > -LightCurrentRange)
		{
			if (Dist < LightCurrentRange)
			{
				if (Dist > 0.0f)
				{
					aWorkStack[iWorkPos++] = pN->m_iNodeBack;
					iWorkNode = pN->m_iNodeFront;
					goto StartOf_Light_BuildShadowFaces_Point_NoAdd;
				}
				else
				{
					aWorkStack[iWorkPos++] = pN->m_iNodeFront;
					iWorkNode = pN->m_iNodeBack;
					goto StartOf_Light_BuildShadowFaces_Point_NoAdd;
				}
			}
			else
			{
				iWorkNode = pN->m_iNodeFront;
				goto StartOf_Light_BuildShadowFaces_Point_NoAdd;
			}
		}
		else 
			if (Dist < LightCurrentRange)
			{
				iWorkNode = pN->m_iNodeBack;
				goto StartOf_Light_BuildShadowFaces_Point_NoAdd;
			}
	}

	if(iWorkPos > 0) goto StartOf_Light_BuildShadowFaces_Point;
}

void CXR_Model_BSP2::Light_BuildShadowFaces_Spot(CBSP2_EnumContext* _pEnumContext, CBSP2_DynamicLightContext* _pDynLightContext, const uint32* _piFaces, int _nFaces)
{
//	ConOut(CStrF("Light_FlagTree, Leaf  nFaces %d, iiFaces %d", nFaces, iiFaces));
	for(int f = 0; f < _nFaces; f++)
	{
		int iFace = _piFaces[f];
		int iFaceIdx = iFace >> 3;
		int iFaceMask = aShiftMulTab[iFace & 7];

		if (_pEnumContext->m_pFTag[iFaceIdx] & iFaceMask) continue;
		if (!_pEnumContext->m_pFTag[iFaceIdx])
		{
			if(_pEnumContext->m_nUntagEnum >= _pEnumContext->m_MaxUntagEnum)
				M_ASSERT(0, "?");
			_pEnumContext->m_piFUntag[_pEnumContext->m_nUntagEnum++] = iFaceIdx;
		}
		_pEnumContext->m_pFTag[iFaceIdx] |= iFaceMask;

		const CBSP2_Face* pF = &m_pFaces[iFace];
//		int Flags = pF->m_Flags;
		if (!(pF->m_Flags & XW_FACE_SHADOWCASTER))
			continue;

/*		CXW_Surface* pSurf = m_lspSurfaces[pF->m_iSurface];

		if (pSurf->m_Flags & XW_SURFFLAGS_INVISIBLE)
			continue;
		if (pSurf->GetBaseFrame()->m_Medium.m_MediumFlags & XW_MEDIUM_SEETHROUGH)
			continue;
*/
		fp32 Dist = _pDynLightContext->m_Light_CurrentFrontFaceShadowMul * m_pPlanes[pF->m_iPlane].Distance(_pDynLightContext->m_Light_CurrentPos);
		if (Dist > 0.0f)
			continue;
		if (Dist < -_pDynLightContext->m_Light_CurrentRange)
			continue;

		CBox3Dfp32 FaceBox;
		if (!Light_FaceIntersectSpot(_pDynLightContext, pF, FaceBox))
			continue;

		if (_pEnumContext->m_nEnum < _pEnumContext->m_MaxEnum)
			_pEnumContext->m_piEnum[_pEnumContext->m_nEnum++] = iFace;
	}
}

void CXR_Model_BSP2::Light_BuildShadowFaces_Spot(CBSP2_EnumContext* _pEnumContext, CBSP2_DynamicLightContext* _pDynLightContext, int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_BuildShadowFaces_Point, MAUTOSTRIP_VOID);

	int aWorkStack[256];
	uint32 iWorkPos = 0;
	int iWorkNode = _iNode;

	goto StartOf_Light_BuildShadowFaces_Spot_NoAdd;

StartOf_Light_BuildShadowFaces_Spot:
	iWorkNode = aWorkStack[--iWorkPos];
StartOf_Light_BuildShadowFaces_Spot_NoAdd:
	const CBSP2_Node* pN = &m_pNodes[iWorkNode];

	if (pN->IsLeaf())
	{
		int nFaces = pN->m_nFaces;
		int iiFaces = pN->m_iiFaces;

		Light_BuildShadowFaces_Spot(_pEnumContext, _pDynLightContext, &m_piFaces[iiFaces], nFaces);
	}
	else
	{
		int iPlane = pN->m_iPlane;
		int Mask = m_pPlanes[iPlane].GetArrayPlaneSideMask(_pDynLightContext->m_Light_lCurrentSpotVertices, 5);

		if(Mask == 3)
		{
			aWorkStack[iWorkPos++] = pN->m_iNodeBack;
			iWorkNode = pN->m_iNodeFront;
			goto StartOf_Light_BuildShadowFaces_Spot_NoAdd;
		}
		else if(Mask == 1)
		{
			iWorkNode = pN->m_iNodeFront;
			goto StartOf_Light_BuildShadowFaces_Spot_NoAdd;
		}
		else if(Mask == 2)
		{
			iWorkNode = pN->m_iNodeBack;
			goto StartOf_Light_BuildShadowFaces_Spot_NoAdd;
		}
	}

	if(iWorkPos > 0) goto StartOf_Light_BuildShadowFaces_Spot;
}

void CXR_Model_BSP2::Light_BuildFaceQueuesAndShadowFaces_Point(CBSP2_RenderParams* _pRenderParams, CBSP2_EnumContext* _pEnumContext, CBSP2_DynamicLightContext* _pDynLightContext, const uint32* _piFaces, int _nFaces)
{
//	ConOut(CStrF("Light_FlagTree, Leaf  nFaces %d, iiFaces %d", nFaces, iiFaces));
	for(int f = 0; f < _nFaces; f++)
	{
		int iFace = _piFaces[f];
		int iFaceIdx = iFace >> 3;
		int iFaceMask = aShiftMulTab[iFace & 7];
		if (_pEnumContext->m_pFTag[iFaceIdx] & iFaceMask) continue;
		if (!_pEnumContext->m_pFTag[iFaceIdx])
		{
			if(_pEnumContext->m_nUntagEnum >= _pEnumContext->m_MaxUntagEnum)
				M_ASSERT(0, "?");
			_pEnumContext->m_piFUntag[_pEnumContext->m_nUntagEnum++] = iFaceIdx;
		}
		_pEnumContext->m_pFTag[iFaceIdx] |= iFaceMask;

		const CBSP2_Face* pF = &m_pFaces[iFace];
//		int Flags = pF->m_Flags;
		if (!(pF->m_Flags & XW_FACE_VISIBLE))
			continue;

/*		CXW_Surface* pSurf = m_lspSurfaces[pF->m_iSurface];

		if (pSurf->m_Flags & XW_SURFFLAGS_INVISIBLE)
			continue;
*/
		fp32 Dist = m_pPlanes[pF->m_iPlane].Distance(_pDynLightContext->m_Light_CurrentPos);
		if (M_Fabs(Dist) > _pDynLightContext->m_Light_CurrentRange)
			continue;

		CBox3Dfp32 FaceBox;
		if (!Light_FaceIntersectOmni(_pDynLightContext, pF, FaceBox))
			continue;
//		if (!Light_IntersectSphere_CheckFace(iFace, m_Light_CurrentPos, m_Light_CurrentRange))
//			continue;

		if (Dist * _pDynLightContext->m_Light_CurrentFrontFaceShadowMul < 0.0f)
		{
//			if (!(pSurf->GetBaseFrame()->m_Medium.m_MediumFlags & XW_MEDIUM_SEETHROUGH) &&
			if ((pF->m_Flags & XW_FACE_SHADOWCASTER) &&
			   (_pEnumContext->m_nEnum < _pEnumContext->m_MaxEnum))
				_pEnumContext->m_piEnum[_pEnumContext->m_nEnum++] = iFace;
		}

		if (Dist > 0.0f)
		{
			if (/*(pSurf->m_Flags & XW_SURFFLAGS_LIGHTING) &&*/ m_pPlanes[pF->m_iPlane].Distance(_pRenderParams->m_pViewData->m_CurLocalVP) > -0.001f)
			{
				// Add face to VB face queue.
				_pRenderParams->m_pVBFaceQueue[pF->m_iVB].AddElem(iFace);
				_pDynLightContext->m_Light_CurrentShadedBound.Expand(FaceBox);
			}
		}
	}
}

void CXR_Model_BSP2::Light_BuildFaceQueuesAndShadowFaces_Point(CBSP2_RenderParams* _pRenderParams, CBSP2_EnumContext* _pEnumContext, CBSP2_DynamicLightContext* _pDynLightContext, int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_BuildFaceQueuesAndShadowFaces_Point, MAUTOSTRIP_VOID);

	CVec3Dfp32 LightCurrentPos = _pDynLightContext->m_Light_CurrentPos;
	fp32 LightCurrentRange = _pDynLightContext->m_Light_CurrentRange;

	int aWorkStack[256];
	uint32 iWorkPos = 0;
	int iWorkNode = _iNode;

	goto StartOf_Light_BuildFaceQueuesAndShadowFaces_Point_NoAdd;

StartOf_Light_BuildFaceQueuesAndShadowFaces_Point:
	iWorkNode = aWorkStack[--iWorkPos];
StartOf_Light_BuildFaceQueuesAndShadowFaces_Point_NoAdd:

	const CBSP2_Node* pN = &m_pNodes[iWorkNode];
	if (pN->IsStructureLeaf() && _pRenderParams->m_CurrentIsPortalWorld)
	{
		int iPL = pN->m_iPortalLeaf;
		if (!(_pDynLightContext->m_Light_pPVS && !(_pDynLightContext->m_Light_pPVS[iPL >> 3] & aShiftMulTab[iPL & 7])))
		{
			const CBSP2_PortalLeafExt& PL = m_pPortalLeaves[iPL];

			if (!(m_pView->m_liLeafRPortals[iPL]))
			{
				if (_pDynLightContext->m_Light_CurrentEnableShadows)
					Light_BuildShadowFaces_Point(_pEnumContext, _pDynLightContext, &m_liPLFaces[PL.m_iiFaces], PL.m_nFaces);
			}
			else
			{
				_pDynLightContext->m_Light_CurrentEnableShadows = true;
				Light_BuildFaceQueuesAndShadowFaces_Point(_pRenderParams, _pEnumContext, _pDynLightContext, &m_liPLFaces[PL.m_iiFaces], PL.m_nFaces);
			}
			if (_pEnumContext->m_nEnum >= _pEnumContext->m_MaxEnum) return;
		}
	}
	else if (pN->IsLeaf())
	{
		int nFaces = pN->m_nFaces;
		int iiFaces = pN->m_iiFaces;

		Light_BuildFaceQueuesAndShadowFaces_Point(_pRenderParams, _pEnumContext, _pDynLightContext, &m_piFaces[iiFaces], nFaces);
		if (_pEnumContext->m_nEnum >= _pEnumContext->m_MaxEnum) return;
	}
	else
	{
		int iPlane = pN->m_iPlane;
		fp32 Dist = m_pPlanes[iPlane].Distance(LightCurrentPos);
		if (Dist > -LightCurrentRange)
		{
			if (Dist < LightCurrentRange)
			{
				if (Dist < 0.0f)
				{
					aWorkStack[iWorkPos++] = pN->m_iNodeBack;
					iWorkNode = pN->m_iNodeFront;
					goto StartOf_Light_BuildFaceQueuesAndShadowFaces_Point_NoAdd;
				}
				else
				{
					aWorkStack[iWorkPos++] = pN->m_iNodeFront;
					iWorkNode = pN->m_iNodeBack;
					goto StartOf_Light_BuildFaceQueuesAndShadowFaces_Point_NoAdd;
				}
			}
			else
			{
				iWorkNode = pN->m_iNodeFront;
				goto StartOf_Light_BuildFaceQueuesAndShadowFaces_Point_NoAdd;
			}
		}
		else 
			if (Dist < LightCurrentRange)
			{
				iWorkNode = pN->m_iNodeBack;
				goto StartOf_Light_BuildFaceQueuesAndShadowFaces_Point_NoAdd;
			}
	}

	if(iWorkPos > 0) goto StartOf_Light_BuildFaceQueuesAndShadowFaces_Point;
}

void CXR_Model_BSP2::Light_BuildFaceQueuesAndShadowFaces_Spot(CBSP2_RenderParams* _pRenderParams, CBSP2_EnumContext* _pEnumContext, CBSP2_DynamicLightContext* _pDynLightContext, const uint32* _piFaces, int _nFaces)
{
//	ConOut(CStrF("Light_FlagTree, Leaf  nFaces %d, iiFaces %d", nFaces, iiFaces));
	for(int f = 0; f < _nFaces; f++)
	{
		int iFace = _piFaces[f];
		int iFaceIdx = iFace >> 3;
		int iFaceMask = aShiftMulTab[iFace & 7];
		if (_pEnumContext->m_pFTag[iFaceIdx] & iFaceMask) continue;
		if (!_pEnumContext->m_pFTag[iFaceIdx])
		{
			if(_pEnumContext->m_nUntagEnum >= _pEnumContext->m_MaxUntagEnum)
				M_ASSERT(0, "?");
			_pEnumContext->m_piFUntag[_pEnumContext->m_nUntagEnum++] = iFaceIdx;
		}
		_pEnumContext->m_pFTag[iFaceIdx] |= iFaceMask;

		const CBSP2_Face* pF = &m_pFaces[iFace];
		if (!(pF->m_Flags & XW_FACE_VISIBLE))
			continue;
//		int Flags = pF->m_Flags;

/*		CXW_Surface* pSurf = m_lspSurfaces[pF->m_iSurface];

		if (pSurf->m_Flags & XW_SURFFLAGS_INVISIBLE)
			continue;
*/
		fp32 Dist = m_pPlanes[pF->m_iPlane].Distance(_pDynLightContext->m_Light_CurrentPos);
		if (M_Fabs(Dist) > _pDynLightContext->m_Light_CurrentRange)
			continue;

		CBox3Dfp32 FaceBox;
		if (!Light_FaceIntersectSpot(_pDynLightContext, pF, FaceBox))
			continue;

		if (Dist * _pDynLightContext->m_Light_CurrentFrontFaceShadowMul < 0.0f)
		{
//			if (!(pSurf->GetBaseFrame()->m_Medium.m_MediumFlags & XW_MEDIUM_SEETHROUGH) &&
//			    _pEnumContext->m_nEnum < _pEnumContext->m_MaxEnum)
			if ((pF->m_Flags & XW_FACE_SHADOWCASTER) &&
				(_pEnumContext->m_nEnum < _pEnumContext->m_MaxEnum))
				_pEnumContext->m_piEnum[_pEnumContext->m_nEnum++] = iFace;
		}

		if (Dist > 0.0f)
		{
			if (/*(pSurf->m_Flags & XW_SURFFLAGS_LIGHTING) &&*/ m_pPlanes[pF->m_iPlane].Distance(_pRenderParams->m_pViewData->m_CurLocalVP) > -0.001f)
			{
				// Add face to VB face queue.
				_pRenderParams->m_pVBFaceQueue[pF->m_iVB].AddElem(iFace);
				_pDynLightContext->m_Light_CurrentShadedBound.Expand(FaceBox);
			}
		}
	}
}

void CXR_Model_BSP2::Light_BuildFaceQueuesAndShadowFaces_Spot(CBSP2_RenderParams* _pRenderParams, CBSP2_EnumContext* _pEnumContext, CBSP2_DynamicLightContext* _pDynLightContext, int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_BuildFaceQueuesAndShadowFaces_Spot, MAUTOSTRIP_VOID);

	CVec3Dfp32 LightCurrentPos = _pDynLightContext->m_Light_CurrentPos;

	int aWorkStack[256];
	uint32 iWorkPos = 0;
	int iWorkNode = _iNode;

	goto StartOf_Light_BuildFaceQueuesAndShadowFaces_Spot_NoAdd;

StartOf_Light_BuildFaceQueuesAndShadowFaces_Spot:
	iWorkNode = aWorkStack[--iWorkPos];
StartOf_Light_BuildFaceQueuesAndShadowFaces_Spot_NoAdd:

	const CBSP2_Node* pN = &m_pNodes[iWorkNode];

	if (pN->IsStructureLeaf() && _pRenderParams->m_CurrentIsPortalWorld)
	{
		int iPL = pN->m_iPortalLeaf;
		if (!(_pDynLightContext->m_Light_pPVS && !(_pDynLightContext->m_Light_pPVS[iPL >> 3] & aShiftMulTab[iPL & 7])))
		{
			const CBSP2_PortalLeafExt& PL = m_pPortalLeaves[iPL];

			if (!(m_pView->m_liLeafRPortals[iPL]))
			{
				if (_pDynLightContext->m_Light_CurrentEnableShadows)
					Light_BuildShadowFaces_Spot(_pEnumContext, _pDynLightContext, &m_liPLFaces[PL.m_iiFaces], PL.m_nFaces);
			}
			else
			{
				_pDynLightContext->m_Light_CurrentEnableShadows = true;
				Light_BuildFaceQueuesAndShadowFaces_Spot(_pRenderParams, _pEnumContext, _pDynLightContext, &m_liPLFaces[PL.m_iiFaces], PL.m_nFaces);
			}
		}
	}
	else if (pN->IsLeaf())
	{
		int nFaces = pN->m_nFaces;
		int iiFaces = pN->m_iiFaces;

		Light_BuildFaceQueuesAndShadowFaces_Spot(_pRenderParams, _pEnumContext, _pDynLightContext, &m_piFaces[iiFaces], nFaces);
	}
	else
	{
		int iPlane = pN->m_iPlane;
		CPlane3Dfp32 Plane = m_pPlanes[pN->m_iPlane];
		fp32 Dist = Plane.Distance(LightCurrentPos);
		int Mask = Plane.GetArrayPlaneSideMask(_pDynLightContext->m_Light_lCurrentSpotVertices, 5);

		if (Dist > 0.0f)
		{
			if(Mask == 3)
			{
				aWorkStack[iWorkPos++]	= pN->m_iNodeFront;
				iWorkNode = pN->m_iNodeBack;
				goto StartOf_Light_BuildFaceQueuesAndShadowFaces_Spot_NoAdd;
			}
			else if(Mask == 2)
			{
				iWorkNode = pN->m_iNodeBack;
				goto StartOf_Light_BuildFaceQueuesAndShadowFaces_Spot_NoAdd;
			}
			else if(Mask == 1)
			{
				iWorkNode = pN->m_iNodeFront;
				goto StartOf_Light_BuildFaceQueuesAndShadowFaces_Spot_NoAdd;
			}
		}
		else
		{
			if(Mask == 3)
			{
				aWorkStack[iWorkPos++]	= pN->m_iNodeBack;
				iWorkNode = pN->m_iNodeFront;
				goto StartOf_Light_BuildFaceQueuesAndShadowFaces_Spot_NoAdd;
			}
			else if(Mask == 2)
			{
				iWorkNode = pN->m_iNodeBack;
				goto StartOf_Light_BuildFaceQueuesAndShadowFaces_Spot_NoAdd;
			}
			else if(Mask == 1)
			{
				iWorkNode = pN->m_iNodeFront;
				goto StartOf_Light_BuildFaceQueuesAndShadowFaces_Spot_NoAdd;
			}
		}
	}

	if(iWorkPos > 0) goto StartOf_Light_BuildFaceQueuesAndShadowFaces_Spot;
}


void VBM_Get2DMatrix(CRC_Viewport* _pVB, CMat4Dfp32& _Mat);
void VBM_RenderRect(CRC_Viewport* _pVP, CXR_VBManager* _pVBM, const CRect2Duint16& _Rect, CPixel32 _Color, fp32 _Priority, CRC_Attributes* _pA);
void VBM_RenderRect_AlphaBlend(CRC_Viewport* _pVP, CXR_VBManager* _pVBM, const CRect2Duint16& _Rect, CPixel32 _Color, fp32 _Priority);


#ifdef PLATFORM_XBOX1

void SSE_Box3Dfp32_Expand(CBox3Dfp32& _Target, const CVec3Dfp32* _pSrc, uint32* _piV, int _nV)
{
	__asm
	{
		mov edi, [_Target]
		mov esi, [_pSrc]
		mov ebx, [_piV]
		mov ecx, [_nV]

		// Load target box: xmm0 = m_Min, xmm1 = m_Max
		movups xmm0, [edi]
		movlps xmm1, qword ptr [edi+12]
		movss xmm2, [edi+12+8]
		shufps xmm1, xmm2, 0+4
lp:
		// Load vector: xmm2 = _Src
		dec ecx
		mov eax, [ebx + ecx*4]
		lea eax, [eax + eax*2]

		movlps xmm2, qword ptr [esi+eax*4]
		movss xmm3, [esi+eax*4+8]
		shufps xmm2, xmm3, 0+4

		minps xmm0, xmm2
		maxps xmm1, xmm2

		jnz lp

		// Write xmm0,xmm1 to _Target.m_Min/m_Max
		movhlps xmm4, xmm0
		movhlps xmm5, xmm1
		movlps qword ptr [edi], xmm0
		movss [edi+8], xmm4
		movlps qword ptr [edi+12], xmm1
		movss [edi+12+8], xmm5
	}
}

void SSE_Box3Dfp32_GetMinMax(CBox3Dfp32& _Target, const CVec3Dfp32* _pSrc, int _nV)
{
	__asm
	{
		mov edi, [_Target]
		mov esi, [_pSrc]
		mov ecx, [_nV]

		// Load first vertex

		movlps xmm0, qword ptr [esi]
		movss xmm1, [esi+8]
		dec ecx
		lea esi, [esi+12]
		shufps xmm0, xmm1, 0+4
		movaps xmm1, xmm0
		jz noloop

lp:
		// Load vector: xmm2 = _Src
		movlps xmm2, qword ptr [esi]
		movss xmm3, [esi+8]
		lea esi, [esi+12]
		shufps xmm2, xmm3, 0+4

		dec ecx
		minps xmm0, xmm2
		maxps xmm1, xmm2

		jnz lp
noloop:

		// Write xmm0,xmm1 to _Target.m_Min/m_Max
		movhlps xmm4, xmm0
		movhlps xmm5, xmm1
		movlps qword ptr [edi], xmm0
		movss [edi+8], xmm4
		movlps qword ptr [edi+12], xmm1
		movss [edi+12+8], xmm5
	}
}

void SSE_Box3Dfp32_GetMinMax(CBox3Dfp32& _Target, const CVec3Dfp32* _pSrc, const uint16* _piV, int _nV)
{
	__asm
	{
		mov edi, [_Target]
		mov esi, [_pSrc]
		mov edx, [_piV]
		mov ecx, [_nV]

		// Load first vertex

		movzx eax, word ptr [edx]
		lea eax, [eax*2+eax]
		lea edx, [edx+2]
		movlps xmm0, qword ptr [esi+eax*4]
		movss xmm1, [esi+eax*4+8]
		dec ecx
		shufps xmm0, xmm1, 0+4
		movaps xmm1, xmm0
		jz noloop

lp:
		// Load vector: xmm2 = _Src
		movzx eax, word ptr [edx]
		lea eax, [eax*2+eax]
		lea edx, [edx+2]
		movlps xmm2, qword ptr [esi+eax*4]
		movss xmm3, [esi+eax*4+8]
		shufps xmm2, xmm3, 0+4

		dec ecx
		minps xmm0, xmm2
		maxps xmm1, xmm2

		jnz lp
noloop:

		// Write xmm0,xmm1 to _Target.m_Min/m_Max
		movhlps xmm4, xmm0
		movhlps xmm5, xmm1
		movlps qword ptr [edi], xmm0
		movss [edi+8], xmm4
		movlps qword ptr [edi+12], xmm1
		movss [edi+12+8], xmm5
	}
}


#endif

int CXR_Model_BSP2::Light_FaceIntersectSpot(CBSP2_DynamicLightContext* _pDynLightContext, const CBSP2_Face* _pF, CBox3Dfp32& _FaceBox) const
{
	CVec3Dfp32 lVerts[64];
	int nV = _pF->m_nPhysV;

	for(int v = 0; v < nV; v++)
	{
		lVerts[v] = m_pVertices[m_piVertices[_pF->m_iiPhysV + v]];
	}

	if (_pDynLightContext->m_Light_pCurrentClip)
	{
		const CRC_ClipVolume* pCV = _pDynLightContext->m_Light_pCurrentClip;
		const CPlane3Dfp32* pP = pCV->m_Planes;
		int nP = pCV->m_nPlanes;

		for(int p = 0; p < nP; p++)
		{
			int v = 0;
			for(; v < nV; v++)
				if (pP[p].Distance(lVerts[v]) < 0.0f)
					break;
			if (v == nV)
				return false;
		}
	}
	else
	{
		for(int p = 0; p < 5; p++)
		{
			int v = 0;
			for(; v < nV; v++)
				if (_pDynLightContext->m_Light_lCurrentSpotPlanes[p].Distance(lVerts[v]) < 0.0f)
					break;
			if (v == nV)
				return false;
		}
	}

#ifdef PLATFORM_XBOX1
	SSE_Box3Dfp32_GetMinMax(_FaceBox, lVerts, nV);
#else
	CBox3Dfp32 TempBox;
	TempBox.m_Min = lVerts[0];
	TempBox.m_Max = lVerts[0];
	for(int v = 1; v < nV; v++)
	{
		TempBox.Expand(lVerts[v]);
	}
	_FaceBox = TempBox;
#endif
	return true;
}

int CXR_Model_BSP2::Light_FaceIntersectOmni(CBSP2_DynamicLightContext* _pDynLightContext, const CBSP2_Face* _pF, CBox3Dfp32& _FaceBox) const
{
	int nV = _pF->m_nPhysV;

#ifdef PLATFORM_XBOX1
	SSE_Box3Dfp32_GetMinMax(_FaceBox, m_pVertices, &m_piVertices[_pF->m_iiPhysV], nV);
#else
	const uint32* piVertices = m_piVertices + _pF->m_iiPhysV;
	const CVec3Dfp32& V0 = m_pVertices[*piVertices++];

	fp32 VMin0 = V0.k[0];
	fp32 VMin1 = V0.k[1];
	fp32 VMin2 = V0.k[2];
	fp32 VMax0 = V0.k[0];
	fp32 VMax1 = V0.k[1];
	fp32 VMax2 = V0.k[2];
	for(int v = 1; v < nV; v++)
	{
		const CVec3Dfp32& V = m_pVertices[*piVertices++];
		// This is really a CBox3Dfp32::Expand call but the compiler fucks up the optimization so we do it manually
		VMin0 = Min(VMin0, V.k[0]);
		VMax0 = Max(VMax0, V.k[0]);
		VMin1 = Min(VMin1, V.k[1]);
		VMax1 = Max(VMax1, V.k[1]);
		VMin2 = Min(VMin2, V.k[2]);
		VMax2 = Max(VMax2, V.k[2]);
	}
	_FaceBox.m_Min.k[0]	= VMin0;
	_FaceBox.m_Min.k[1]	= VMin1;
	_FaceBox.m_Min.k[2]	= VMin2;
	_FaceBox.m_Max.k[0]	= VMax0;
	_FaceBox.m_Max.k[1]	= VMax1;
	_FaceBox.m_Max.k[2]	= VMax2;
#endif

	return _FaceBox.IsInside(_pDynLightContext->m_Light_CurrentBoundBox);

/*	for(int p = 0; p < 5; p++)
	{
		int v = 0;
		for(; v < nV; v++)
			if (m_Light_lCurrentSpotPlanes[p].Distance(lVerts[v]) < 0.0f)
				break;
		if (v == nV)
			return false;
	}*/

	return true;
}

void CBSP2_SpotLightClip::CalcSpotPlanes(const CXR_Light* _pL, CPlane3Dfp32* _pP)
{
	CVec3Dfp32 V[5];
	fp32 Width = _pL->m_SpotWidth * _pL->m_Range;
	fp32 Height = _pL->m_SpotHeight * _pL->m_Range;
	CVec3Dfp32 Fwd;
	_pL->GetDirection().Scale(_pL->m_Range, Fwd);

	CVec3Dfp32 LightPos = _pL->GetPosition();
	CVec3Dfp32 LightMRow1 = CVec3Dfp32::GetRow(_pL->m_Pos, 1);
	CVec3Dfp32 LightMRow2 = CVec3Dfp32::GetRow(_pL->m_Pos, 2);

	LightPos.Combine(LightMRow1, Width, V[1]);
	V[1].Combine(LightMRow2, Height, V[1]);
	V[1] += Fwd;

	LightPos.Combine(LightMRow1, -Width, V[2]);
	V[2].Combine(LightMRow2, Height, V[2]);
	V[2] += Fwd;

	LightPos.Combine(LightMRow1, -Width, V[3]);
	V[3].Combine(LightMRow2, -Height, V[3]);
	V[3] += Fwd;

	LightPos.Combine(LightMRow1, Width, V[4]);
	V[4].Combine(LightMRow2, -Height, V[4]);
	V[4] += Fwd;

	V[0] = LightPos;

	_pP[0].Create(V[1], V[0], V[2]);
	_pP[1].Create(V[2], V[0], V[3]);
	_pP[2].Create(V[3], V[0], V[4]);
	_pP[3].Create(V[4], V[0], V[1]);
	_pP[4].Create(V[1], V[2], V[3]);
}


void CBSP2_SpotLightClip::CalcSpotVerticesAndPlanes(const CXR_Light* _pL, CVec3Dfp32* _pV, CPlane3Dfp32* _pP)
{
	CVec3Dfp32 LightPos = _pL->GetPosition();
	CVec3Dfp32 LightMRow1 = CVec3Dfp32::GetRow(_pL->m_Pos, 1);
	CVec3Dfp32 LightMRow2 = CVec3Dfp32::GetRow(_pL->m_Pos, 2);
	fp32 Width = _pL->m_SpotWidth * _pL->m_Range;
	fp32 Height = _pL->m_SpotHeight * _pL->m_Range;
	CVec3Dfp32 Fwd;
	_pL->GetDirection().Scale(_pL->m_Range, Fwd);

	LightPos.Combine(LightMRow1, Width, _pV[1]);
	_pV[1].Combine(LightMRow2, Height, _pV[1]);
	_pV[1] += Fwd;

	LightPos.Combine(LightMRow1, -Width, _pV[2]);
	_pV[2].Combine(LightMRow2, Height, _pV[2]);
	_pV[2] += Fwd;

	LightPos.Combine(LightMRow1, -Width, _pV[3]);
	_pV[3].Combine(LightMRow2, -Height, _pV[3]);
	_pV[3] += Fwd;

	LightPos.Combine(LightMRow1, Width, _pV[4]);
	_pV[4].Combine(LightMRow2, -Height, _pV[4]);
	_pV[4] += Fwd;

	_pV[0] = LightPos;

	if(_pP)
	{
		_pP[0].Create(_pV[1], _pV[0], _pV[2]);
		_pP[1].Create(_pV[2], _pV[0], _pV[3]);
		_pP[2].Create(_pV[3], _pV[0], _pV[4]);
		_pP[3].Create(_pV[4], _pV[0], _pV[1]);
		_pP[4].Create(_pV[1], _pV[2], _pV[3]);
	}
}

void CBSP2_SpotLightClip::Create(const CXR_Light* _pL)
{
	CalcSpotVerticesAndPlanes(_pL, m_lV, m_lPlanes);
}

int CBSP2_SpotLightClip::IntersectBox(const CBox3Dfp32& _Box) const
{
	fp32 Dist0 = m_lPlanes[0].GetBoxMinDistance(_Box.m_Min, _Box.m_Max);
	fp32 Dist1 = m_lPlanes[1].GetBoxMinDistance(_Box.m_Min, _Box.m_Max);
	fp32 Dist2 = m_lPlanes[2].GetBoxMinDistance(_Box.m_Min, _Box.m_Max);
	fp32 Dist3 = m_lPlanes[3].GetBoxMinDistance(_Box.m_Min, _Box.m_Max);
	fp32 Dist4 = m_lPlanes[4].GetBoxMinDistance(_Box.m_Min, _Box.m_Max);

	fp32 Dist = M_FSel(-Dist0, M_FSel(-Dist1, M_FSel(-Dist2, M_FSel(-Dist3, M_FSel(-Dist4, 0.0f, Dist4), Dist3), Dist2), Dist1), Dist0);
	if(Dist > 0.0f) return false;

//	if(Dist0 > 0.0f || Dist1 > 0.0f || Dist2 > 0.0f || Dist3 > 0.0f || Dist4 > 0.0f) return false;

	return true;
}

void CBSP2_SpotLightClip::GetBoundBox(CBox3Dfp32& _Box) const
{
	CVec3Dfp32::GetMinBoundBox(m_lV, _Box.m_Min, _Box.m_Max, 5);
}

CXR_VertexBuffer* CXR_Model_BSP2::Light_CreateFullShadowVolume(CBSP2_RenderParams* _pRenderParams, const CXR_Light& _Light)
{
	if (_Light.m_Type == CXR_LIGHTTYPE_SPOT)
	{
		CXR_VertexBuffer* pVB = _pRenderParams->m_pVBM->Alloc_VB(CXR_VB_VERTICES, 5);
		if (!pVB)
			return NULL;

		CBSP2_SpotLightClip::CalcSpotVerticesAndPlanes(&_Light, pVB->GetVBChain()->m_pV, NULL);

		static uint16 ms_SpotLightTriangles[18] = { 1,2,3, 1,3,4, 0,1,2, 0,2,3, 0,3,4, 0,4,1 };
		pVB->Render_IndexedTriangles(ms_SpotLightTriangles, 6);
		return pVB;
	}
	else if (_Light.m_Type == CXR_LIGHTTYPE_POINT)
	{
		return NULL;
/*		CXR_VertexBuffer* pVB = _pRenderParams->m_pVBM->Alloc_VB(CXR_VB_VERTICES, 8);
		if (!pVB)
			return NULL;

		_Light.CalcBoundBoxFast();
		_Light.m_BoundBox.GetVertices(pVB->m_pV);

		static uint16 ms_PointLightTriangles[18] = {  };
		pVB->Render_IndexedTriangles(ms_PointLightTriangles, 12);
		return pVB;*/
	}
	else
		return NULL;
}

void CXR_Model_BSP2::Light_RenderDynamicLight(CBSP2_RenderParams* _pRenderParams, int _iDynamic, bool _bNoShading, const CMat4Dfp32* _pWMat, const CMat4Dfp32* _pVMat)
{
	MAUTOSTRIP(CXR_Model_BSP_Light_RenderDynamicLight, MAUTOSTRIP_VOID);
	MSCOPESHORT(Light_RenderDynamicLight);
#if defined(SHADOW_NAMEDEVENTS)
	M_NAMEDEVENT("Light_RenderDynamicLight",0xff0000ff);
#endif
	CXR_Light* pL = &_pRenderParams->m_lRenderLight[_iDynamic];
	const CXR_LightInfo* pLI = &_pRenderParams->m_lRenderLightInfo[_iDynamic];

	bool bIsMirrored = _pRenderParams->m_pCurrentEngine->GetVC()->m_bIsMirrored != 0;
	int ColorWriteDisable = (_pRenderParams->m_pCurrentEngine->m_DebugFlags & M_Bit(12)) ? 0 : CRC_FLAGS_COLORWRITE;
	int bShadows = 
		!(pL->m_Flags & CXR_LIGHT_NOSHADOWS) &&
		!(_pRenderParams->m_RenderInfo.m_Flags & CXR_RENDERINFO_NOSHADOWVOLUMES) &&
		!(_pRenderParams->m_pCurrentEngine->m_DebugFlags & M_Bit(13));
	int bSolidShadowVolumes = (_pRenderParams->m_pCurrentEngine->m_DebugFlags & M_Bit(17));

	TObjectPoolAllocator<CBSP2_EnumContext> EnumAlloc(m_spEnumContextPool);
	CBSP2_EnumContext* pEnumContext = EnumAlloc.GetObject();
	CBSP2_DynamicLightContext DynLightContext;
	bool bFullShadow = false;
	{
		M_LOCK(_pRenderParams->m_pVBM->m_AllocLock);
		uint32* piShadowFaces = (uint32*)_pRenderParams->m_pVBM->Alloc_Open();
		int MaxShadowFaces = _pRenderParams->m_pVBM->GetAvail() >> 2;
		DynLightContext.m_Light_pPVS = SceneGraph_PVSLock(0, pL->GetPosition());

		DynLightContext.m_Light_CurrentEnableShadows = (!_pRenderParams->m_CurrentIsPortalWorld);	// Enable from start if this is not a structure model.
	//	DynLightContext.m_Light_CurrentFrontFaceShadow = false;
	//	DynLightContext.m_Light_CurrentFrontFaceShadowMul = 1.0f;
		DynLightContext.m_Light_CurrentFrontFaceShadow = true;
		DynLightContext.m_Light_CurrentFrontFaceShadowMul = -1.0f;

		DynLightContext.m_Light_CurrentRange = pL->m_Range;
		DynLightContext.m_Light_CurrentPos = pL->GetPosition();

		DynLightContext.m_Light_CurrentShadedBound.m_Min = _FP32_MAX;
		DynLightContext.m_Light_CurrentShadedBound.m_Max = -_FP32_MAX;
		DynLightContext.m_Light_pCurrentClip = NULL;

		pEnumContext->Create(m_lFaces.Len(), 0);
		pEnumContext->SetupFaceEnum(piShadowFaces, MaxShadowFaces);

		bFullShadow = !(GetMedium(pL->GetPosition()) & XW_MEDIUM_SEETHROUGH);
		if (bFullShadow)
			goto DrawShadow;


		if (pL->m_Type == CXR_LIGHTTYPE_POINT)
		{
			if (_pRenderParams->m_CurrentIsPortalWorld)
			{
				DynLightContext.m_Light_CurrentFrontFaceShadow = true;
				DynLightContext.m_Light_CurrentFrontFaceShadowMul = -1.0f;
			}
			DynLightContext.m_Light_CurrentBoundBox.m_Min[0] = DynLightContext.m_Light_CurrentPos[0] - DynLightContext.m_Light_CurrentRange;
			DynLightContext.m_Light_CurrentBoundBox.m_Min[1] = DynLightContext.m_Light_CurrentPos[1] - DynLightContext.m_Light_CurrentRange;
			DynLightContext.m_Light_CurrentBoundBox.m_Min[2] = DynLightContext.m_Light_CurrentPos[2] - DynLightContext.m_Light_CurrentRange;
			DynLightContext.m_Light_CurrentBoundBox.m_Max[0] = DynLightContext.m_Light_CurrentPos[0] + DynLightContext.m_Light_CurrentRange;
			DynLightContext.m_Light_CurrentBoundBox.m_Max[1] = DynLightContext.m_Light_CurrentPos[1] + DynLightContext.m_Light_CurrentRange;
			DynLightContext.m_Light_CurrentBoundBox.m_Max[2] = DynLightContext.m_Light_CurrentPos[2] + DynLightContext.m_Light_CurrentRange;

			MSCOPESHORT(Light_BuildFaceQueuesAndShadowFaces_Point);
			if (_bNoShading)
			{
				if (m_lFaces.Len() <= 12)
					Light_BuildShadowFaces_Point(pEnumContext, &DynLightContext, g_IndexRamp32, m_lFaces.Len());
				else
					Light_BuildShadowFaces_Point(pEnumContext, &DynLightContext, 1);
			}
			else
			{
				if (m_lFaces.Len() <= 12)
					Light_BuildFaceQueuesAndShadowFaces_Point(_pRenderParams, pEnumContext, &DynLightContext, g_IndexRamp32, m_lFaces.Len());
				else
					Light_BuildFaceQueuesAndShadowFaces_Point(_pRenderParams, pEnumContext, &DynLightContext, 1);
			}
		}
		else if (pL->m_Type == CXR_LIGHTTYPE_SPOT)
		{
			MSCOPESHORT(Light_BuildFaceQueuesAndShadowFaces_Spot);
			CBSP2_SpotLightClip::CalcSpotVerticesAndPlanes(pL, DynLightContext.m_Light_lCurrentSpotVertices, DynLightContext.m_Light_lCurrentSpotPlanes);

			if (_pRenderParams->m_CurrentIsPortalWorld)
			{
				int iRenderView = m_iView;
				CXR_SceneGraphInstance* pRenderSGI = m_pView->m_pSceneGraph;

				CMat4Dfp32 LightVMat;
				pL->m_Pos.InverseOrthogonal(LightVMat);

				View_InitForLight(m_lspViews.Len()-1, pL->GetPosition(), &DynLightContext.m_Light_lCurrentSpotVertices[1], 4, pL);

				CBSP2_ViewInstance* pLightView = m_pWorldView;

				View_SetCurrent(iRenderView, pRenderSGI);

				DynLightContext.m_Light_CurrentFrontFaceShadow = true;
				DynLightContext.m_Light_CurrentFrontFaceShadowMul = -1.0f;

				bool bShadows = false;
				for(int i = pLightView->m_nVisLeaves-1; i >= 0; i--)
				{
					int iPL = pLightView->m_liVisPortalLeaves[i];

					const CBSP2_PortalLeafExt& PL = m_pPortalLeaves[iPL];

					int iRP = pLightView->m_liLeafRPortals[iPL];
					DynLightContext.m_Light_pCurrentClip = &pLightView->m_lRPortals[iRP];

	//				RenderRPortal(m_Light_pCurrentClip, 0xff400000);

					if (m_pView->m_liLeafRPortals[iPL] == (uint16)~0)
					{
						if (bShadows)
							Light_BuildShadowFaces_Spot(pEnumContext, &DynLightContext, &m_liPLFaces[PL.m_iiFaces], PL.m_nFaces);
					}
					else
					{
						bShadows = true;
						if (_bNoShading)
							Light_BuildShadowFaces_Spot(pEnumContext, &DynLightContext, &m_liPLFaces[PL.m_iiFaces], PL.m_nFaces);
						else
							Light_BuildFaceQueuesAndShadowFaces_Spot(_pRenderParams, pEnumContext, &DynLightContext, &m_liPLFaces[PL.m_iiFaces], PL.m_nFaces);
					}	
				}
				DynLightContext.m_Light_pCurrentClip = NULL;
			}
			else
			{
				if (_bNoShading)
				{
					if (m_lFaces.Len() <= 12)
						Light_BuildShadowFaces_Spot(pEnumContext, &DynLightContext, g_IndexRamp32, m_lFaces.Len());
					else
						Light_BuildShadowFaces_Spot(pEnumContext, &DynLightContext, 1);
				}
				else
				{
					if (m_lFaces.Len() <= 12)
						Light_BuildFaceQueuesAndShadowFaces_Spot(_pRenderParams, pEnumContext, &DynLightContext, g_IndexRamp32, m_lFaces.Len());
					else
						Light_BuildFaceQueuesAndShadowFaces_Spot(_pRenderParams, pEnumContext, &DynLightContext, 1);
				}
			}
		}
		else
		{
			return;
		}

		_pRenderParams->m_pVBM->Alloc_Close(pEnumContext->m_nEnum*4);
	}
	pEnumContext->Untag();

	if (DynLightContext.m_Light_pPVS)
	{
		SceneGraph_PVSRelease(DynLightContext.m_Light_pPVS);
		DynLightContext.m_Light_pPVS = NULL;
	}

	if (!_bNoShading)
	{

#ifdef M_Profile
		if( _pRenderParams->m_pCurrentEngine->m_DebugFlags & XR_DEBUGFLAGS_SHOWLIGHTCOUNT ) 
		{
			CBSP2_VBFaceQueue* pQ = _pRenderParams->m_pVBFaceQueue;
			uint nQ = m_lVBFaceQueues.Len();
			for(uint j = 0; j < nQ; j++)
			{
				const uint32* piF = pQ[j].m_pQueue;
				for(uint i = 0; i < pQ[j].m_nElem; i++)
				{
					uint iFace = *(piF++);
					m_lFaceLights[iFace]++;
				}
			}
		}
#endif

		Light_RenderShading_FaceQueues(_pRenderParams, pL, _pWMat, _pVMat);
	}

	if (_pRenderParams->m_CurrentIsPortalWorld && DynLightContext.m_Light_CurrentShadedBound.m_Min[0] != _FP32_MAX)
	{
		CScissorRect ShadedScissor;
		CalcBoxScissor(m_pView, DynLightContext.m_Light_CurrentShadedBound, ShadedScissor);
		m_pWorldView->m_lLightOcclusion[pL->m_iLight].m_ScissorShaded.Expand(ShadedScissor);
	}

DrawShadow:
	if (bShadows)
	{
		uint32 MinX = Max(0, Min(m_pView->m_VPWidth-1, (int)pLI->m_ShadowScissor.GetMinX()));
		uint32 MinY = Max(0, Min(m_pView->m_VPHeight-1, (int)pLI->m_ShadowScissor.GetMinY()));
		uint32 MaxX = Max(0, Min(m_pView->m_VPWidth, (int)pLI->m_ShadowScissor.GetMaxX()));
		uint32 MaxY = Max(0, Min(m_pView->m_VPHeight, (int)pLI->m_ShadowScissor.GetMaxY()));

		if (MaxX <= MinX)
			MaxX = MinX+1;
		if (MaxY <= MinY)
			MaxY = MinY+1;

		CScissorRect Scissor;
		Scissor.SetRect(MinX, MinY, MaxX, MaxY);

		CXR_VertexBuffer* pVB = NULL;
		if (bFullShadow)
		{
			DynLightContext.m_Light_CurrentFrontFaceShadow = true;
			pVB = Light_CreateFullShadowVolume(_pRenderParams, *pL);
		}
		else
		{	
			pVB = Light_CreateShadowVolume(_pRenderParams, &DynLightContext, *pL, pEnumContext->m_piEnum, pEnumContext->m_nEnum);
		}

		if (!pVB)
			return;

		pVB->Geometry_Color(bFullShadow ? 0xff101000 : 0xff100010);
		pVB->Matrix_Set(_pRenderParams->m_pVBMatrixM2V);

		CXR_VertexBuffer* pVB2 = _pRenderParams->m_pVBM->Alloc_VB();
		if (!pVB2)
			return;

		*pVB2 = *pVB;

		{
			CRC_Attributes* pA = _pRenderParams->m_pVBM->Alloc_Attrib();
			if (!pA)
				return;
			pA->SetDefault();
/*			if (bFullShadow)
			{
				pA->Attrib_RasterMode(CRC_RASTERMODE_NONE);
				pA->Attrib_Enable(CRC_FLAGS_ZWRITE | CRC_FLAGS_COLORWRITE | CRC_FLAGS_CULL);
				pA->Attrib_Disable(CRC_FLAGS_SCISSOR);
			}
			else*/ if (bSolidShadowVolumes)
			{
				pA->Attrib_RasterMode(CRC_RASTERMODE_NONE);
	//			pA->Attrib_ZCompare(CRC_COMPARE_ALWAYS);
				pA->Attrib_Enable(CRC_FLAGS_COLORWRITE | CRC_FLAGS_CULL);
				pA->Attrib_Disable(CRC_FLAGS_SCISSOR | CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_ZWRITE | CRC_FLAGS_STENCIL);
			}
			else
			{
				pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
	//			pA->Attrib_ZCompare(CRC_COMPARE_ALWAYS);
				pA->Attrib_Enable(CRC_FLAGS_SCISSOR | CRC_FLAGS_CULL | CRC_FLAGS_STENCIL);
				pA->Attrib_Disable(ColorWriteDisable | CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_ZWRITE);
			}

			if (DynLightContext.m_Light_CurrentFrontFaceShadow ^ bIsMirrored)
				pA->Attrib_Enable(CRC_FLAGS_CULLCW);

//			if (!bFullShadow)
			{
				pA->Attrib_ZCompare(CRC_COMPARE_GREATEREQUAL);

				if(_pRenderParams->m_CurrentRCCaps & CRC_CAPS_FLAGS_SEPARATESTENCIL)
				{
					pA->Attrib_Enable(CRC_FLAGS_SEPARATESTENCIL);
					pA->Attrib_Disable( CRC_FLAGS_CULL | CRC_FLAGS_CULLCW );
					pA->Attrib_StencilRef( 1, 255 );
					if( !DynLightContext.m_Light_CurrentFrontFaceShadow ^ bIsMirrored)
					{
						pA->Attrib_StencilFrontOp( CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_DEC);
						pA->Attrib_StencilBackOp( CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_INC);
					}
					else
					{
						pA->Attrib_StencilBackOp( CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_DEC);
						pA->Attrib_StencilFrontOp( CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_INC);
					}
				}
				else
				{
					pA->Attrib_StencilRef(1, 255);
					pA->Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_INC);
				}
				pA->Attrib_StencilWriteMask(255);
			}

#ifndef PLATFORM_CONSOLE
			pA->Attrib_PolygonOffset(0.0f, 2.0f);
#endif
			pA->m_Scissor = Scissor;

			pVB->m_pAttrib = pA;
			pVB->m_Priority = pL->m_iLight+0.001f;
			pVB->m_iLight = pL->m_iLight;
			pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;

			_pRenderParams->m_pVBM->AddVB(pVB);
		}

		if(!(_pRenderParams->m_CurrentRCCaps & CRC_CAPS_FLAGS_SEPARATESTENCIL))
		{
			CRC_Attributes* pA = _pRenderParams->m_pVBM->Alloc_Attrib();
			if (!pA)
				return;
			pA->SetDefault();
			pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
	//			pA->Attrib_ZCompare(CRC_COMPARE_ALWAYS
/*			if (bFullShadow)
			{
				pA->Attrib_RasterMode(CRC_RASTERMODE_NONE);
				pA->Attrib_Enable(CRC_FLAGS_ZWRITE | CRC_FLAGS_COLORWRITE | CRC_FLAGS_CULL);
				pA->Attrib_Disable(CRC_FLAGS_SCISSOR);
			}
			else*/ if (bSolidShadowVolumes)
			{
				pA->Attrib_RasterMode(CRC_RASTERMODE_NONE);
	//			pA->Attrib_ZCompare(CRC_COMPARE_ALWAYS);
				pA->Attrib_Enable(CRC_FLAGS_COLORWRITE | CRC_FLAGS_CULL);
				pA->Attrib_Disable(CRC_FLAGS_SCISSOR | CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_ZWRITE | CRC_FLAGS_STENCIL);
			}
			else
			{
				pA->Attrib_Enable(CRC_FLAGS_SCISSOR | CRC_FLAGS_CULL | CRC_FLAGS_STENCIL);
				pA->Attrib_Disable(ColorWriteDisable | CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_ZWRITE);
			}

			if (!DynLightContext.m_Light_CurrentFrontFaceShadow ^ bIsMirrored)
				pA->Attrib_Enable(CRC_FLAGS_CULLCW);

//			if (!bFullShadow)
			{
				pA->Attrib_ZCompare(CRC_COMPARE_GREATEREQUAL);

				pA->Attrib_StencilRef(1, 255);
				pA->Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_DEC);
				pA->Attrib_StencilWriteMask(255);
			}

#ifndef PLATFORM_CONSOLE
			pA->Attrib_PolygonOffset(0.0f, 2.0f);
#endif
			pA->m_Scissor = Scissor;

			pVB2->m_pAttrib = pA;
			pVB2->m_Priority = pL->m_iLight+0.002f;
			pVB2->m_iLight = pL->m_iLight;
			pVB2->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;

			_pRenderParams->m_pVBM->AddVB(pVB2);
		}
	}
}

