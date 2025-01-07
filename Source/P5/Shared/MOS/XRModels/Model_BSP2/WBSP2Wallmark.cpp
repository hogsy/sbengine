#include "PCH.h"

#include "WBSP2Model.h"
#include "../../XR/XREngineVar.h"
#include "MFloat.h"
#include "../../Classes/Render/MWireContainer.h"

#include "../../XR/XRShader.h"

CBSP2_WallmarkContext::CBSP2_WallmarkContext()
{
	MAUTOSTRIP(CBSP2_WallmarkContext_ctor, MAUTOSTRIP_VOID);
	m_nV = 0;
	m_pSC = NULL;
}

void CBSP2_WallmarkContext::Create(CXR_Model_BSP2* _pModel, int _nV)
{
	MAUTOSTRIP(CBSP2_WallmarkContext_Create, MAUTOSTRIP_VOID);
	MSCOPE(CBSP2_WallmarkContext::Create, XR_BSPMODEL);

	m_lV.SetLen(_nV);
	m_lN.SetLen(_nV);
	m_lTV.SetLen(_nV);
	m_lTV2.SetLen(_nV);
	m_lCol.SetLen(_nV);
	m_lTangU.SetLen(_nV);
	m_lTangV.SetLen(_nV);
	m_nV = _nV;
	m_iVTail = 0;
	m_iVHead = 0;
	typedef TQueue<CBSP2_Wallmark> TQueue_CBSP2_Wallmark;
	m_spqWM = MNew1(TQueue_CBSP2_Wallmark, _nV / 4);
	if (!m_spqWM) MemError("Create");

	m_spLink = MNew(CBSP2_LinkContext);
	if (!m_spLink) MemError("Create");

	m_spLink->Create(_pModel, _nV / 4, _nV / 3, _pModel->m_nStructureLeaves, false);

	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	if (!pSC) Error("Wallmark_Render", "No surface-context available.");
	m_pSC = pSC;
}

void CBSP2_WallmarkContext::Clear()
{
	MAUTOSTRIP(CBSP2_WallmarkContext_Clear, MAUTOSTRIP_VOID);
	m_iVHead = 0;
	m_iVTail = 0;
	if (!m_spqWM) return;
	while(!m_spqWM->Empty())
	{
		m_spLink->Remove(m_spqWM->GetTailIndex());
		m_spqWM->Get();
	}
}

void CBSP2_WallmarkContext::FreeWallmark()
{
	MAUTOSTRIP(CBSP2_WallmarkContext_FreeWallmark, MAUTOSTRIP_VOID);
	if (!m_spqWM) return;
	int iTail = m_spqWM->GetTailIndex();
	CBSP2_Wallmark WM = m_spqWM->Get();
	CBSP2_Wallmark* pWM = m_spqWM->GetTail();
	m_spLink->Remove(iTail);

	if (!pWM)
	{
		m_iVTail = 0;
		m_iVHead = 0;
		return;
	}
	int iVLast = pWM->m_iV;
	while(m_iVTail != iVLast)
	{
		m_iVTail++;
		if (m_iVTail >= m_nV) m_iVTail = 0;
	}
}

int CBSP2_WallmarkContext::AddWallmark(const CBSP2_Wallmark& _WM)
{
	MAUTOSTRIP(CBSP2_WallmarkContext_AddWallmark, 0);
	if (!m_spqWM) return 0;
	if (!m_spqWM->MaxPut()) FreeWallmark();

	int iHead = m_spqWM->GetHeadIndex();
	m_spqWM->Put(_WM);
	return iHead;
}

int CBSP2_WallmarkContext::MaxPut() const
{
	MAUTOSTRIP(CBSP2_WallmarkContext_MaxPut, 0);
	if (m_iVHead == m_iVTail)
		return m_nV-1;
	else
		return Max(0, ((2*m_nV - (m_iVHead - m_iVTail)) % m_nV) -1);
}


int CBSP2_WallmarkContext::AddVertices(int _nV)
{
	MAUTOSTRIP(CBSP2_WallmarkContext_AddVertices, 0);
	if ((m_iVHead + _nV > m_nV))
	{
		while(m_iVTail > m_iVHead)
			FreeWallmark();

		if (m_iVTail == 0) FreeWallmark();
		m_iVHead = 0;
	}

	while(MaxPut() < _nV+1) FreeWallmark();

	int iv = m_iVHead;
	m_iVHead += _nV;
	if (m_iVHead >= m_nV) m_iVHead = 0;
//ConOutL(CStrF("AddVerts %d, %d, %d, %d", iv, _nV, m_iVHead, m_iVTail));

	if (iv + _nV > m_nV)
		Error("AddVertices", CStrF("Internal error _nV %d, iV %d, nV %d, H %d, T %d", _nV, iv, m_nV, m_iVHead, m_iVTail));

	return iv;
}

void CXR_Model_BSP2::Wallmark_Render(CBSP2_RenderParams* _pRenderParams, CBSP2_WallmarkContext* _pWMC, const CBSP2_Wallmark* _pWM, 
									 CMTime _Time, CXR_VertexBuffer * _pVB,
									 CVec3Dfp32 * _pV,CVec3Dfp32 * _pN, CVec2Dfp32 * _pTV1, CVec2Dfp32 * _pTV2,
									 CVec3Dfp32 * _pTanU,CVec3Dfp32 * _pTanV, CPixel32 * _pCol,uint16 * _piV)
{
	MAUTOSTRIP(CXR_Model_BSP_Wallmark_Render, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP2::Wallmark_Render);

	if(_pWM->m_Flags & XR_WALLMARK_DESTROYED)
	{
		return;
	}

	CXW_Surface* pSurf = _pWMC->m_pSC->GetSurface(_pWM->m_SurfaceID)->GetSurface(_pRenderParams->m_pCurrentEngine->m_SurfOptions, _pRenderParams->m_pCurrentEngine->m_SurfCaps);

	int nv = _pWM->m_nV;
	
	_pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL);
	
	int nPrim = nv+2;
	//*
//	uint16* pPrim = _pRenderParams->m_pVBM->Alloc_Int16(nPrim);
//	if (!pPrim) return;
	
	int iP = 0;
	_piV[iP++] = CRC_RIP_TRIFAN + ((nv+2) << 8);
	_piV[iP++] = nv;
	for(int v = 0; v < nv; v++) _piV[iP++] = nv-1-v;

	if (!_pVB->AllocVBChain(_pRenderParams->m_pVBM, false))
		return;

	_pVB->Geometry_VertexArray(_pV, _pWM->m_nV, true);
	_pVB->Geometry_Normal(_pN);
	_pVB->Geometry_TVertexArray(_pTV1, 0);
	_pVB->Geometry_TVertexArray(_pTV2, 1);
//	if(_pWM->m_Flags & XR_WALLMARK_CREATE_TANGENTS)
	{
		_pVB->Geometry_TVertexArray(_pTanU, 2);
		_pVB->Geometry_TVertexArray(_pTanV, 3);
	}
	_pVB->Geometry_ColorArray(_pCol);
	_pVB->Render_IndexedPrimitives(_piV, nPrim);

	_pVB->m_Priority = CXR_VBPRIORITY_WALLMARK;

	CXW_SurfaceKeyFrame* pSurfKey;
	CMTime Time;
	if(_pWM->m_Flags & XR_WALLMARK_NOANIMATE)
		Time = _pWM->m_SpawnTime;
	else
		Time = _Time - _pWM->m_SpawnTime;

	if(_pWM->m_Flags & XR_WALLMARK_OVERLAPPING)
		pSurfKey = pSurf->GetFrame(1, Time, _pRenderParams->CreateTempSurfKeyFrame(0));
	else
		pSurfKey = pSurf->GetFrame(0, Time, _pRenderParams->CreateTempSurfKeyFrame(0));

	CXR_RenderSurfExtParam Params;
	for(int i = 0; i < XR_WALLMARK_MAXTEXTUREPARAMS; i++)
		Params.m_TextureID[i] = _pWM->m_SurfaceParam_TextureID[i];

	uint8 DummyIndex = ((mint(_pWM)>>4) & 255);
	fp32 Prio = _pRenderParams->m_BasePriority_Opaque + (DummyIndex* 0.0001f);

	if( (pSurf->m_Flags & XW_SURFFLAGS_SHADER) && (pSurfKey->m_lTextures.Len() >= 3) )
	{
		_pVB->Matrix_Set( _pRenderParams->m_pVBMatrixM2V );
		CXR_SurfaceShaderParams SSP;
		CXR_Shader* pShader = _pRenderParams->m_pCurrentEngine->GetShader();
		SSP.Create(pSurf, pSurfKey);
		const CXR_SurfaceShaderParams* pSSP = &SSP;
		CXR_ShaderParams Params;
		Params.Clear();
		pShader->RenderDeferredArray(1, _pVB, &Params, &pSSP, CRC_FLAGS_COLORWRITE | CRC_FLAGS_ALPHAWRITE,CRC_FLAGS_ZWRITE | CRC_FLAGS_STENCIL, Prio - 2059.9f, 1.0f, 1);
	}

	if( !(pSurf->m_Flags & XW_SURFFLAGS_SHADERONLY) || (pSurfKey->m_lTextures.Len() < 3) )
	{
		int Flags = /*RENDERSURFACE_VERTEXFOG | */RENDERSURFACE_MATRIXSTATIC_M2V;
		if( pSurfKey->m_lTextures.Len() >= 3 ) Flags |= RENDERSURFACE_NOSHADERLAYERS;
		CXR_Util::Render_Surface(Flags, Time, pSurf, pSurfKey, _pRenderParams->m_pCurrentEngine, _pRenderParams->m_pVBM, (CMat4Dfp32*)NULL, (CMat4Dfp32*)NULL, _pRenderParams->m_pVBMatrixM2V, _pVB, 
			Prio /* m_RenderInfo.m_BasePriority_Opaque + CXR_VBPRIORITY_WALLMARK*/, 0.0001f, &Params);
	}
}


void CXR_Model_BSP2::Wallmark_Render(CBSP2_RenderParams* _pRenderParams, CBSP2_WallmarkContext* _pWMC, int _iPL)
{
	MAUTOSTRIP(CXR_Model_BSP_Wallmark_Render_2, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP2::Wallmark_Render);
return;
	if (!_pWMC->m_spqWM) return;
	if (!_pRenderParams->m_pCurrentEngine->m_bWallmarks) return;
	CMTime RenderTime = _pRenderParams->m_EngineTime;

	uint32 nBytes = 0;

	if (_iPL < 0)
	{
		CBSP2_Wallmark* pWM = _pWMC->m_spqWM->GetTail();

		//Calculate size, alloc data
		while(pWM)
		{
			nBytes += 68 * pWM->m_nV;	//Vertices
			nBytes += (pWM->m_nV + 2) * sizeof(uint16);	//Indices
			nBytes += sizeof(CXR_VertexBuffer) + sizeof(CRC_Attributes);	//Attrib
			if( pWM->m_nV & 1 ) nBytes += 2;	//Align
			pWM = _pWMC->m_spqWM->GetNext(pWM);
		}
		uint8 *pData = (uint8*)_pRenderParams->m_pVBM->Alloc(nBytes);
		if (!pData)
			return;

		pWM = _pWMC->m_spqWM->GetTail();
		while(pWM)
		{
			const uint16 nSz2 = sizeof(CVec2Dfp32) * pWM->m_nV, nSz3 = sizeof(CVec3Dfp32) * pWM->m_nV;

			CVec3Dfp32 * pV = (CVec3Dfp32*)pData;
			memcpy(pData,&_pWMC->m_lV[pWM->m_iV],nSz3); pData += nSz3;
			CVec3Dfp32 * pN = (CVec3Dfp32*)pData;
			memcpy(pData,&_pWMC->m_lN[pWM->m_iV],nSz3); pData += nSz3;
			CVec2Dfp32 * pTV1 = (CVec2Dfp32*)pData;
			memcpy(pData,&_pWMC->m_lTV[pWM->m_iV],nSz2); pData += nSz2;
			CVec2Dfp32 * pTV2 = (CVec2Dfp32*)pData;
			memcpy(pData,&_pWMC->m_lTV2[pWM->m_iV],nSz2); pData += nSz2;
			CVec3Dfp32 * pTanU = (CVec3Dfp32*)pData;
			memcpy(pData,&_pWMC->m_lTangU[pWM->m_iV],nSz3); pData += nSz3;
			CVec3Dfp32 * pTanV = (CVec3Dfp32*)pData;
			memcpy(pData,&_pWMC->m_lTangV[pWM->m_iV],nSz3); pData += nSz3;
			CPixel32 *pCol = (CPixel32*)pData;
			uint16 *piV = (uint16*)(pData + pWM->m_nV * sizeof(CPixel32));
			memcpy(pData,&_pWMC->m_lCol[pWM->m_iV],sizeof(CPixel32) * pWM->m_nV); 
			pData += pWM->m_nV * (sizeof(CPixel32) + sizeof(uint16)) + 2 * sizeof(uint16);
			if( pWM->m_nV & 1 ) pData += 2;
			CXR_VertexBuffer * pVB = (CXR_VertexBuffer*)pData;
#ifdef M_Profile
			pVB->m_Link.Construct();
#endif
			pVB->Clear();
			pData += sizeof(CXR_VertexBuffer);
			pVB->m_pAttrib = (CRC_Attributes*)pData;
			pVB->m_pAttrib->SetDefault();
			pData += sizeof(CRC_Attributes);

			Wallmark_Render(_pRenderParams, _pWMC, pWM, RenderTime, pVB,pV,pN,pTV1,pTV2,pTanU,pTanV,pCol,piV);
			pWM = _pWMC->m_spqWM->GetNext(pWM);
		}
	}
	else
	{
#ifdef PLATFORM_CONSOLE
		uint16 liWM[4096];
		int nWM = _pWMC->m_spLink->EnumIDs(_iPL, liWM, 4096, NULL, 0);
#else
		uint16 liWM[16384];
		int nWM = _pWMC->m_spLink->EnumIDs(_iPL, liWM, 16384, NULL, 0);
#endif
		if( !nWM ) return;

		//Calculate size, alloc data
		int i;
		for(i = 0;i < nWM;i++)
		{
			const CBSP2_Wallmark &WM = _pWMC->m_spqWM->GetElem(liWM[i]);
			nBytes += 68 * WM.m_nV;	//Vertices;
			nBytes += (WM.m_nV + 2) * sizeof(uint16);	//Indices
			nBytes += sizeof(CXR_VertexBuffer) + sizeof(CRC_Attributes);	//Attrib
			if( WM.m_nV & 1 ) nBytes += 2;	//Align
		}
		uint8 *pData = (uint8*)_pRenderParams->m_pVBM->Alloc(nBytes);
		if (!pData)
			return;

		for(i = 0; i < nWM; i++)
		{
			const CBSP2_Wallmark *pWM = &_pWMC->m_spqWM->GetElem(liWM[i]);
			const uint16 nSz2 = sizeof(CVec2Dfp32) * pWM->m_nV, nSz3 = sizeof(CVec3Dfp32) * pWM->m_nV;

			CVec3Dfp32 * pV = (CVec3Dfp32*)pData;
			memcpy(pData,&_pWMC->m_lV[pWM->m_iV],nSz3); pData += nSz3;
			CVec3Dfp32 * pN = (CVec3Dfp32*)pData;
			memcpy(pData,&_pWMC->m_lN[pWM->m_iV],nSz3); pData += nSz3;
			CVec2Dfp32 * pTV1 = (CVec2Dfp32*)pData;
			memcpy(pData,&_pWMC->m_lTV[pWM->m_iV],nSz2); pData += nSz2;
			CVec2Dfp32 * pTV2 = (CVec2Dfp32*)pData;
			memcpy(pData,&_pWMC->m_lTV2[pWM->m_iV],nSz2); pData += nSz2;
			CVec3Dfp32 * pTanU = (CVec3Dfp32*)pData;
			memcpy(pData,&_pWMC->m_lTangU[pWM->m_iV],nSz3); pData += nSz3;
			CVec3Dfp32 * pTanV = (CVec3Dfp32*)pData;
			memcpy(pData,&_pWMC->m_lTangV[pWM->m_iV],nSz3); pData += nSz3;
			CPixel32 *pCol = (CPixel32*)pData;
			uint16 *piV = (uint16*)(pData + pWM->m_nV * sizeof(CPixel32));
			memcpy(pData,&_pWMC->m_lCol[pWM->m_iV],sizeof(CPixel32) * pWM->m_nV); 
			pData += pWM->m_nV * (sizeof(CPixel32) + sizeof(uint16)) + 2 * sizeof(uint16);
			if( pWM->m_nV & 1 ) pData += 2;
			CXR_VertexBuffer * pVB = (CXR_VertexBuffer*)pData;
#ifdef M_Profile
			pVB->m_Link.Construct();
#endif
			pVB->Clear();
			pData += sizeof(CXR_VertexBuffer);
			pVB->m_pAttrib = (CRC_Attributes*)pData;
			pVB->m_pAttrib->SetDefault();
			pData += sizeof(CRC_Attributes);

			Wallmark_Render(_pRenderParams, _pWMC, pWM, RenderTime, pVB,pV,pN,pTV1,pTV2,pTanU,pTanV,pCol,piV);
		}
	}
}

CXR_WallmarkInterface* CXR_Model_BSP2::Wallmark_GetInterface()
{
	MAUTOSTRIP(CXR_Model_BSP_Wallmark_GetInterface, NULL);
	if (m_lPortalLeaves.Len())
		return this;
	else
		return NULL;
}

int CXR_Model_BSP2::Wallmark_CreateContext(const CXR_WallmarkContextCreateInfo& _CreateInfo)
{
	MAUTOSTRIP(CXR_Model_BSP_Wallmark_CreateContext, 0);
	MSCOPESHORT(CXR_Model_BSP2::Wallmark_CreateContext);
	if (m_spWMC || m_spWMCTemp)
		return -1;

	m_spWMC = MNew(CBSP2_WallmarkContext);
	m_spWMC->Create(this, _CreateInfo.m_CapacityDynamic);

	m_spWMCTemp = MNew(CBSP2_WallmarkContext);
	m_spWMCTemp->Create(this, _CreateInfo.m_CapacityTemp);

	m_spWMCStatic = MNew(CBSP2_WallmarkContext);
	m_spWMCStatic->Create(this, _CreateInfo.m_CapacityStatic);

	return 0;
}

void CXR_Model_BSP2::Wallmark_DestroyContext(int _hContext)
{
	MAUTOSTRIP(CXR_Model_BSP_Wallmark_DestroyContext, MAUTOSTRIP_VOID);
	if (_hContext != 0)
		return;

	m_spWMC = NULL;
	m_spWMCTemp = NULL;
	m_spWMCStatic = NULL;
}

#define XR_WALLMARK_FADECOLOR 2
#define MODEL_BSP_WALLMARKDISTANCE 0.0f
#define XR_WALLMARK_PERPENDICULAR_TREHOLD 0.1f

bool CXR_Model_BSP2::Wallmark_Create(CBSP2_WallmarkContext* _pWMC, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _iPL, const CVec3Dfp32* _pV, int _nV, int _GUID, const CXR_PlaneMapping& _Mapping)
{
	MAUTOSTRIP(CXR_Model_BSP_Wallmark_Create, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP2::Wallmark_Create);

	if (_nV < 3) return false;

	const CVec3Dfp32& U = CVec3Dfp32::GetMatrixRow(_Origin, 0);
	const CVec3Dfp32& V = CVec3Dfp32::GetMatrixRow(_Origin, 1);
	const CVec3Dfp32& N = CVec3Dfp32::GetMatrixRow(_Origin, 2);
	const CVec3Dfp32& O = CVec3Dfp32::GetMatrixRow(_Origin, 3);

	CPlane3Dfp32 Plane;
	Plane.Create(_pV[0], _pV[2], _pV[1]);
	fp32 NDot = N * Plane.n;
	if(!(_Flags & XR_WALLMARK_ALLOW_PERPENDICULAR))
	{	
		if (NDot < XR_WALLMARK_PERPENDICULAR_TREHOLD) return false;
	}
	else
	{
		if (Abs(NDot) < 0.0001f)
			NDot = 0.0001f; 
	}
	fp32 d = Plane.Distance(O);
	if (M_Fabs(d) > _Tolerance) return false;

	CPlane3Dfp32 Planes[34];
	CVec3Dfp32 Verts[32];
	CVec2Dfp32 TVerts[32];
	CVec2Dfp32 TVerts2[32];
	CPixel32 Col[32];

	// Create edge planes:
	int nP = 0;
	{
		for(int p = 0; p < _nV; p++)
		{
			int iv0 = p;
			int iv1 = (p) ? (p-1) : _nV-1;
			Planes[nP++].Create(_pV[iv0], _pV[iv1], _pV[iv1] + Plane.n);
		}

		Planes[nP++].CreateNV(N, O + (N*_Tolerance));
		Planes[nP++].CreateNV(-N, O + (N*(-_Tolerance)));
	}

	CVec3Dfp32 OProj, UProj, VProj;
	O.Combine(N, -d/NDot, OProj);
	OProj += Plane.n * MODEL_BSP_WALLMARKDISTANCE;
	U.Combine(N, -(U*Plane.n) / NDot, UProj);
	V.Combine(N, -(V*Plane.n) / NDot, VProj);

	fp32 Size = _WM.m_Size;
	Verts[0] = OProj;
	OProj.Combine(UProj, -Size*0.5f, Verts[0]);
	Verts[0].Combine(VProj, -Size*0.5f, Verts[0]);
	Verts[0].Combine(UProj, Size, Verts[1]);
	Verts[1].Combine(VProj, Size, Verts[2]);
	Verts[2].Combine(UProj, -Size, Verts[3]);

	TVerts[0] = CVec2Dfp32(1,0);
	TVerts[1] = CVec2Dfp32(0,0);
	TVerts[2] = CVec2Dfp32(0,1);
	TVerts[3] = CVec2Dfp32(1,1);

	int nV = 0;
	{
		MSCOPE(CutFence, XR_BSPMODEL);
		nV = CutFence(Verts, 4, Planes, nP, true, TVerts);
	}
	if (nV <= 2)
		return false;

	int iV = _pWMC->AddVertices(nV);

	fp32 TolerenceRecp = 1.0f / _Tolerance;
	for(int v = 0; v < nV; v++)
	{
		// Ugly texgen, do we still need this?
		TVerts2[v] = CVec2Dfp32(Verts[v][0] / 128.0f, Verts[v][1] / 128.0f);

		if (_WM.m_Flags & XR_WALLMARK_NOFADE)
		{
			Col[v] = 0xffffffff;
		}
		else
		{
			fp32 d = M_Fabs(N*(Verts[v] - O)) * TolerenceRecp;
			d = 1.0f - Clamp01(d);
			d *= 1.0f - Sqr(Sqr(1.0f - NDot));
			int Fade = RoundToInt(d*255.0f);
			if (_WM.m_Flags & XR_WALLMARK_FADECOLOR)
				Col[v] = 0xff000000 + (Fade << 16) + (Fade << 8) + Fade;
			else
				Col[v] = (Fade << 24) + 0xffffff;
		}
	}

	memcpy(&_pWMC->m_lV[iV], Verts, sizeof(CVec3Dfp32)*nV);
	memcpy(&_pWMC->m_lTV[iV], TVerts, sizeof(CVec2Dfp32)*nV);
	memcpy(&_pWMC->m_lTV2[iV], TVerts2, sizeof(CVec2Dfp32)*nV);
	memcpy(&_pWMC->m_lCol[iV], Col, sizeof(CPixel32)*nV);
	{ for(int v = 0; v < nV; v++) _pWMC->m_lN[iV+v] = Plane.n; };

	// Create decal tangent space -> wall tangentspace transform
	{
		CVec2Dfp32 RotU;
		CVec2Dfp32 RotV;
		RotU[0] = V * _Mapping.m_U;
		RotU[1] = V * _Mapping.m_V;
		RotV[0] = U * _Mapping.m_U;
		RotV[1] = U * _Mapping.m_V;
		RotU.Normalize();
 		RotV.Normalize();
		if ((U / V) * N > 0.0f)
		{
			Swap(RotU[0], RotV[1]);
//			RotU = -RotU;
// 			RotV = -RotV;
		}

		for(int v = 0; v < nV; v++)
		{
			_pWMC->m_lTangU[iV + v] = CVec3Dfp32(RotU[0], RotU[1], 0);
			_pWMC->m_lTangV[iV + v] = CVec3Dfp32(RotV[0], RotV[1], 0);
		}
	}

	if(_Flags & XR_WALLMARK_CREATE_TANGENTS && 0)
	{
		CVec3Dfp32 TangU[32];
		CVec3Dfp32 TangV[32];

		for(int v = 0; v < nV; v++)
		{
			TangU[v] = CVec3Dfp32(0,0,0);
			TangV[v] = CVec3Dfp32(0,0,0);
		}

		//CVec3Dfp32 TempTangU[32];
		//CVec3Dfp32 TempTangV[32];
		//int i0 = 0, i1, i2;
		fp32 x1, x2, y1, y2, z1, z2, s1, s2, t1, t2, r;
		for(int v = 2; v < nV; v++)
		{
			const int TempV = v-1;
			//i0 = 0;
			//i1 = v - 1;
			//i2 = v;

			const CVec3Dfp32& v0 = Verts[0];//Verts[i0];
			const CVec3Dfp32& v1 = Verts[TempV];//Verts[i1];
			const CVec3Dfp32& v2 = Verts[v];//Verts[i2];

			const CVec2Dfp32& w0 = TVerts[0];
			const CVec2Dfp32& w1 = TVerts[TempV];
			const CVec2Dfp32& w2 = TVerts[v];

			x1 = v1[0] - v0[0];
			x2 = v2[0] - v0[0];
			y1 = v1[1] - v0[1];
			y2 = v2[1] - v0[1];
			z1 = v1[2] - v0[2];
			z2 = v2[2] - v0[2];

			s1 = w1[0] - w0[0];
			s2 = w2[0] - w0[0];
			t1 = w1[1] - w0[1];
			t2 = w2[1] - w0[1];

			r = 1.0f / (s1 * t2 - s2 * t1);
			const CVec3Dfp32 s((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
			const CVec3Dfp32 t((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);
			TangU[0] += s;
			TangU[TempV] += s;
			TangU[v] += s;

			TangV[0] += t;
			TangV[TempV] += t;
			TangV[v] += t;
		}

		for(int v = 0; v < nV; v++)
		{
			const CVec3Dfp32& N = _pWMC->m_lN[iV+v];
			CVec3Dfp32& TanU = TangU[v];
			CVec3Dfp32& TanV = TangV[v];

			TanU.Normalize();
			TanV.Normalize();
			TanU.Combine(N, -(N * TanU), TanU);
			TanV.Combine(N, -(N * TanV), TanV);
			TanU.Normalize();
			TanV.Normalize();
		}

		memcpy(&_pWMC->m_lTangU[iV], TangU, sizeof(CVec3Dfp32)*nV);
		memcpy(&_pWMC->m_lTangV[iV], TangV, sizeof(CVec3Dfp32)*nV);
	}

	CBSP2_Wallmark WM;
	WM.CXR_WallmarkDesc::operator=(_WM);
	WM.m_iV = iV;
	WM.m_nV = nV;
	WM.m_GUID = _GUID;
	WM.m_Pos = CVec3Dfp32::GetRow(_Origin, 3);
	WM.m_Flags = _Flags;

//ConOut(CStrF("    iPL %d, WM Added: ", _iPL) + N.GetString());
	int iWM = _pWMC->AddWallmark(WM);
	_pWMC->m_spLink->Insert(iWM, _iPL);

	return true;
/*		CWireContainer* pWC = NULL;
	MACRO_GetRegisterObject(CWireContainer, pWCTmp, "GAMECONTEXT.CLIENT.WIRECONTAINER");
	pWC = pWCTmp;
	if (pWC)
	{
		for(int i = 0; i < nV; i++)
			pWC->RenderWire(Verts[i], Verts[(i+1) % nV], 0xff703f00, 0.05f);
	}*/
}

int CXR_Model_BSP2::Wallmark_Create(CBSP2_WallmarkContext* _pWMC, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _Material)
{
	MAUTOSTRIP(CXR_Model_BSP_Wallmark_Create_2, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP2::Wallmark_Create);

	if (!_pWMC->m_spqWM) return false;

	const int MaxFaces = 1024;
	uint32 liFaces[MaxFaces];
	uint16 liFacePL[MaxFaces];

//	const CVec3Dfp32& U = CVec3Dfp32::GetMatrixRow(_Origin, 0);
//	const CVec3Dfp32& V = CVec3Dfp32::GetMatrixRow(_Origin, 1);
	const CVec3Dfp32& N = CVec3Dfp32::GetMatrixRow(_Origin, 2);
	const CVec3Dfp32& O = CVec3Dfp32::GetMatrixRow(_Origin, 3);

	//JK-FIX: What parameters should really be passed here?
//	CXW_Surface *pSurf = _pWMC->m_pSC->GetSurface(_WM.m_SurfaceID)->GetSurface(m_SurfOptions, m_SurfCaps);
	CXW_Surface *pSurf = _pWMC->m_pSC->GetSurface(_WM.m_SurfaceID)->GetSurface(0, 0);
	if(_Flags & XR_WALLMARK_SCANFOROVERLAP && pSurf->GetNumSequences() > 1)
	{
		CBSP2_Wallmark* pWM = _pWMC->m_spqWM->GetTail();

		while(pWM)
		{
			if((pWM->m_Flags & XR_WALLMARK_PRIMARY) && !(pWM->m_Flags & XR_WALLMARK_OVERLAPPING) && (O - pWM->m_Pos).Length() < _WM.m_Size / 3)
			{
				_Flags |= XR_WALLMARK_OVERLAPPING;
				break;
			}
			pWM = _pWMC->m_spqWM->GetNext(pWM);
		}
	}
	_Flags |= XR_WALLMARK_PRIMARY;

	fp32 SH = _WM.m_Size * 0.5f;
	CBox3Dfp32 BoundLocal;
	BoundLocal.m_Min = CVec3Dfp32(-SH, -SH, -_Tolerance);
	BoundLocal.m_Max = CVec3Dfp32(SH, SH, _Tolerance);
	CBox3Dfp32 Bound;
	BoundLocal.Transform(_Origin, Bound);
/*	Bound.m_Min = O;
	Bound.m_Max = O;
	Bound.m_Min -= CVec3Dfp32(_Tolerance*_SQRT2);
	Bound.m_Max += CVec3Dfp32(_Tolerance*_SQRT2);*/


//	fp32 Size = _WM.m_Size;

//	int nFaces = EnumFaces_Sphere(1, liFaces, MaxFaces, ENUM_HQ | ENUM_FACEFLAGS, 0, XW_FACE_VISIBLE, O, Size*0.5f*_SQRT2, liFacePL);
	int nFaces = 0;
	TObjectPoolAllocator<CBSP2_EnumContext> EnumAlloc(m_spEnumContextPool);
	CBSP2_EnumContext* pEnumContext = EnumAlloc.GetObject();
	pEnumContext->Create(m_lFaces.Len(), ENUM_HQ | ENUM_FACEFLAGS, 0, XW_FACE_VISIBLE);
	pEnumContext->SetupFaceEnum(liFaces, MaxFaces);
	pEnumContext->SetupPLEnum(liFacePL, MaxFaces);
	pEnumContext->SetupBoxEnum(Bound);
	CXR_PhysicsContext PhysContext;
//	nFaces = EnumFaces_Box(&PhysContext, pEnumContext, 1, liFaces, MaxFaces, ENUM_HQ | ENUM_FACEFLAGS, 0, XW_FACE_VISIBLE, Bound, liFacePL);
	nFaces = EnumFaces_Box(&PhysContext, pEnumContext, 1);
	pEnumContext->Untag();

//ConOut(CStrF("WMFaces %d, Pos %s", nFaces, (char*)O.GetString() ));

	//JK-NOTE: Non-Threadsafe
	static int GUID = 1;
	int NewGUID = GUID + 1;
	bool bRes = false;
	{
		MSCOPE(Faces, XR_BSPMODEL);
		for(int i = 0; i < nFaces; i++)
		{
			const CBSP2_Face* pF = &m_pFaces[liFaces[i]];
			CXW_Surface* pS = m_lspSurfaces[pF->m_iSurface];
			if (pS->m_Flags & XW_SURFFLAGS_NOWALLMARK)
				continue;

			if(_Material != 0 && pS->GetBaseFrame()->m_MaterialType != _Material)
				continue;

			const CPlane3Dfp32* pP = &m_pPlanes[pF->m_iPlane];
	//ConOut(CStr("    WM Face test: " + N.GetString() +  pP->n.GetString()));
			fp32 NDot = N * pP->n;
			if(!(_Flags & XR_WALLMARK_ALLOW_PERPENDICULAR))
			{	
				if (NDot < XR_WALLMARK_PERPENDICULAR_TREHOLD) continue;
			}
			else
			{
				if (Abs(NDot) < 0.0001f)
					NDot = 0.0001f; 
			}

			fp32 d = pP->Distance(O);
			if (Abs(d) > _Tolerance) continue;

			CVec3Dfp32 Verts[32];
			int iiV = pF->m_iiPhysV;
			const uint32* piV = &m_piVertices[iiV];
			for(int p = 0; p < pF->m_nPhysV; p++)
				Verts[p] = m_pVertices[piV[p]];

			if(Wallmark_Create(_pWMC, _WM, _Origin, _Tolerance, _Flags, liFacePL[i], Verts, pF->m_nPhysV, NewGUID, m_lMappings[pF->m_iMapping]))
			{
				_Flags &= ~XR_WALLMARK_PRIMARY;
				bRes = true;
			}
		}
	}
	
	if(bRes)
		GUID = NewGUID;
	return GUID;
}

int CXR_Model_BSP2::Wallmark_Create(int _hContext, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _Material)
{
	MAUTOSTRIP(CXR_Model_BSP_Wallmark_Create_3, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP2::Wallmark_Create, XR_BSPMODEL);

	if (!m_spWMC)
		return false;

	M_LOCK(m_WallmarkLock);

/*	if (CVec3Dfp32::GetRow(_Origin,3).LengthSqr() < Sqr(200.0f))
	{
		MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
		if (pSC)
		{	
			CXW_Surface* pS = pSC->GetSurface(_WM.m_SurfaceID);
			ConOutL(CStrF("Wallmark close to origo, Pos %s, Tolerance %f, %s", CVec3Dfp32::GetRow(_Origin,3).GetString().Str(), _Tolerance, pS->m_Name.Str() ));
		}
	}*/

	if(_Flags & XR_WALLMARK_NEVERDESTROY)
		return Wallmark_Create(m_spWMCStatic, _WM, _Origin, _Tolerance, _Flags, _Material);
	else if(_Flags & XR_WALLMARK_TEMPORARY)
		return Wallmark_Create(m_spWMCTemp, _WM, _Origin, _Tolerance, _Flags, _Material);
	else
		return Wallmark_Create(m_spWMC, _WM, _Origin, _Tolerance, _Flags, _Material);
}

bool CXR_Model_BSP2::Wallmark_Destroy(int _GUID)
{
	CBSP2_WallmarkContext *pContexts[3] = { m_spWMCTemp, m_spWMC, m_spWMCStatic };

	bool bRes = false;
	for(int i = 0; i < 3; i++)
	{
		CBSP2_Wallmark* pWM = pContexts[i] ? pContexts[i]->m_spqWM->GetTail() : NULL;
		while(pWM)
		{
			if(pWM->m_GUID == _GUID)
			{
				// Remove wallmark
				pWM->m_Flags |= XR_WALLMARK_DESTROYED;
				bRes = true;
				break;
			}
			pWM = pContexts[i]->m_spqWM->GetNext(pWM);
		}
	}
	return bRes;
}
