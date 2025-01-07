#include "PCH.h"

#include "MFloat.h"
#include "WBSP2Model.h"
#include "WBSP2Def.h"
#include "../../Classes/BitString/MBitString.h"

#include "../../XR/XREngineVar.h"
#include "../../XR/XRVBContext.h"
#include "../../XR/XRShader.h"

// #pragma optimize("",off)

enum
{
	e_Platform_PC    = 0,
	e_Platform_Xbox  = 1,
	e_Platform_PS2   = 2,
	e_Platform_GC    = 3,
	e_Platform_Xenon = 4,
	e_Platform_PS3	 = 5,

	e_Platform_Default = e_Platform_PC,
};



enum
{
	BSP_MAX_LIGHTS	= 32
};

//#define MODEL_BSP_TL_ENABLE_CONDITION m_lPortalLeaves.Len() && m_pCurrentEngine->GetVar(0x2345) != 0
#define MODEL_BSP_TL_ENABLE_CONDITION(Engine) Engine->m_bTLEnableEnabled

#define MODEL_BSP_DYNLIGHTCULLBACKFACE_CONDITION (m_CurrentRCCaps & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20)

// On by default
// #define MODEL_BSP_SORTENABLE
#define MODEL_BSP_VB
#define MODEL_BSP_TRACE 1 ? (void)0 : M_TRACEALWAYS
//#define MODEL_BSP_TRACE M_TRACEALWAYS

// Off by default
// #define MODEL_BSP_ONELIGHTMAP

//#define MODEL_BSP_DISABLENHF
//#define MODEL_BSP_DISABLEFOGPORTALS

//#define MODEL_BSP_BACKFACECULL

//#define MODEL_BSP_TRIANGLEFAN			// Must be enabled, triangle lists doesn't work since all prims go through Render_IndexedPrimitives

void Prefetch8(void* _Ptr);

//#define PREFETCH(Addr) Prefetch8(Addr);
#define PREFETCH(Addr)

IMPLEMENT_OPERATOR_NEW(CBSP2_LinkContext);
IMPLEMENT_OPERATOR_NEW(CBSP2_ViewInstance);
IMPLEMENT_OPERATOR_NEW(CBSP2_VertexBuffer);
IMPLEMENT_OPERATOR_NEW(CXR_Model_BSP2);

// -------------------------------------------------------------------
fp32 CXR_Model_BSP2::ms_lDiameterClasses[MODEL_BSP_MAXDIAMETERCLASSES] = 
		{ 2.0f, 4.0f, 8.0f, 16.0f, 
		  32.0f, 48.0f, 64.0f, 96.0f, 
		  128.0f, 192.0f, 256.0f, 384.0f, 
		  512.0f, 768.0f, 1024.0f, 2048.0f };

// -------------------------------------------------------------------
//  Stuff in MImage.cpp
// -------------------------------------------------------------------

void PPA_Mul_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);
void PPA_MulAddSaturate_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);

void PPA_Mul_RGBA32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);




#define XR_COMPAREANDSETATTR(Val, Attr, ChgFlag) { if ((Val) != (Attr)) { (Attr) = (Val); AttrChg |= (ChgFlag); } }

// -------------------------------------------------------------------
//  SHADOWVOLUME
// -------------------------------------------------------------------
class CXR_VirtualAttributes_BSP2ShadowVolumeFront : public CXR_VirtualAttributes
{
public:
	static CRC_Attributes* ms_pBase;

	CScissorRect m_Scissor;

	static void PrepareFrame(CXR_VBManager* _pVBM, int _bSeparateStencil, int _ColorWrite, int _bMirrorCull)
	{
		CRC_Attributes* pA = _pVBM->Alloc_Attrib();
		if (pA)
		{
			pA->SetDefault();

			pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
			pA->Attrib_ZCompare(CRC_COMPARE_GREATEREQUAL);

			int Enable = CRC_FLAGS_SCISSOR | CRC_FLAGS_STENCIL;
			int Disable = CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_ZWRITE;
			if(_bSeparateStencil)
			{
				Enable = CRC_FLAGS_SCISSOR | CRC_FLAGS_STENCIL | CRC_FLAGS_SEPARATESTENCIL;
				Disable = CRC_FLAGS_CULL | CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_ZWRITE;
				pA->Attrib_StencilRef(1, 255);
				if (_bMirrorCull)
				{
					pA->Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_INCWRAP);
					pA->Attrib_StencilBackOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_DECWRAP);
				}
				else
				{
					pA->Attrib_StencilBackOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_INCWRAP);
					pA->Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_DECWRAP);
				}
			}
			else
			{
				Enable = CRC_FLAGS_SCISSOR | CRC_FLAGS_CULL | CRC_FLAGS_STENCIL;
				Disable = CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_ZWRITE | CRC_FLAGS_SEPARATESTENCIL;
				pA->Attrib_StencilRef(1, 255);
				pA->Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_INC);
				if (_bMirrorCull)
					Enable |= CRC_FLAGS_CULLCW;
			}
			if(!_ColorWrite)
				Disable	|= CRC_FLAGS_COLORWRITE;
			pA->Attrib_Enable(Enable);
			pA->Attrib_Disable(Disable);

			pA->Attrib_StencilWriteMask(255);
		}
		ms_pBase = pA;
	}

	void Create()
	{
		m_pBaseAttrib = ms_pBase;
		m_Class = 0x20;
	}

	int OnCompare(const CXR_VirtualAttributes* _pOther)
	{
/*		const CXR_VirtualAttributes_BSP2ShadowVolumeFront* pLast = (CXR_VirtualAttributes_BSP2ShadowVolumeFront*)_pOther;
		XR_COMPAREATTRIBUTE_INT(m_Scissor.m_Min.k[0], pLast->m_Scissor.m_Min.k[0]);
		XR_COMPAREATTRIBUTE_INT(m_Scissor.m_Min.k[1], pLast->m_Scissor.m_Min.k[1]);
		XR_COMPAREATTRIBUTE_INT(m_Scissor.m_Max.k[0], pLast->m_Scissor.m_Max.k[0]);
		XR_COMPAREATTRIBUTE_INT(m_Scissor.m_Max.k[1], pLast->m_Scissor.m_Max.k[1]);*/
		return 0;
	}

	uint OnSetAttributes(CRC_Attributes* _pDest, const CXR_VirtualAttributes* _pLastAttr)
	{
		// If we ran out of VBM during rendering then we can't trust this data
		if(!m_pBaseAttrib)
			return 0;

		int AttrChg = 0;
		if (!_pLastAttr || _pLastAttr->m_Class != m_Class)
		{
			*_pDest = *m_pBaseAttrib;
			AttrChg = -1;
			_pDest->m_Scissor = m_Scissor;
		}
		else
		{
//			CXR_VirtualAttributes_BSP2ShadowVolumeFront *pLast = (CXR_VirtualAttributes_BSP2ShadowVolumeFront *)_pLastAttr;
			if (memcmp(&_pDest->m_Scissor, &m_Scissor, sizeof(m_Scissor)))
			{
				_pDest->m_Scissor = m_Scissor;
				AttrChg |= CRC_ATTRCHG_SCISSOR;
			}
		}

		return AttrChg;
	}
};

CRC_Attributes* CXR_VirtualAttributes_BSP2ShadowVolumeFront::ms_pBase = NULL;

// -------------------------------------------------------------------
class CXR_VirtualAttributes_BSP2ShadowVolumeBack : public CXR_VirtualAttributes
{
public:
	static CRC_Attributes* ms_pBase;

	CScissorRect m_Scissor;

	static void PrepareFrame(CXR_VBManager* _pVBM, int _ColorWrite, int _bMirrorCull)
	{
		CRC_Attributes* pA = _pVBM->Alloc_Attrib();
		if (pA)
		{
			pA->SetDefault();

			pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
			int Enable = CRC_FLAGS_SCISSOR | CRC_FLAGS_CULL | CRC_FLAGS_STENCIL;
			if (!_bMirrorCull)
				Enable |= CRC_FLAGS_CULLCW;
			pA->Attrib_Enable(Enable);
			pA->Attrib_Disable(CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_ZWRITE);
			if (!_ColorWrite)
				pA->Attrib_Disable(CRC_FLAGS_COLORWRITE);

			pA->Attrib_ZCompare(CRC_COMPARE_GREATEREQUAL);

			pA->Attrib_StencilRef(1, 255);
			pA->Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_DEC);
			pA->Attrib_StencilWriteMask(255);
		}
		ms_pBase	= pA;
	}

	void Create()
	{
		m_pBaseAttrib	= ms_pBase;
		m_Class = 0x21;
	}

	int OnCompare(const CXR_VirtualAttributes* _pOther)
	{
/*		const CXR_VirtualAttributes_BSP2ShadowVolume* pLast = (CXR_VirtualAttributes_BSP2ShadowVolume*)_pOther;
		XR_COMPAREATTRIBUTE_INT(m_Scissor.m_Min.k[0], pLast->m_Scissor.m_Min.k[0]);
		XR_COMPAREATTRIBUTE_INT(m_Scissor.m_Min.k[1], pLast->m_Scissor.m_Min.k[1]);
		XR_COMPAREATTRIBUTE_INT(m_Scissor.m_Max.k[0], pLast->m_Scissor.m_Max.k[0]);
		XR_COMPAREATTRIBUTE_INT(m_Scissor.m_Max.k[1], pLast->m_Scissor.m_Max.k[1]);*/
		return 0;
	}

	uint OnSetAttributes(CRC_Attributes* _pDest, const CXR_VirtualAttributes* _pLastAttr)
	{
		// If we ran out of VBM during rendering then we can't trust this data
		if(!m_pBaseAttrib)
			return 0;

		int AttrChg = 0;
		if (!_pLastAttr || _pLastAttr->m_Class != m_Class)
		{
			*_pDest = *m_pBaseAttrib;
			AttrChg = -1;
			_pDest->m_Scissor = m_Scissor;
		}
		else
		{
//			CXR_VirtualAttributes_BSP2ShadowVolumeBack *pLast = (CXR_VirtualAttributes_BSP2ShadowVolumeBack *)_pLastAttr;
			if (memcmp(&_pDest->m_Scissor, &m_Scissor, sizeof(m_Scissor)))
			{
				_pDest->m_Scissor = m_Scissor;
				AttrChg |= CRC_ATTRCHG_SCISSOR;
			}
		}
		return AttrChg | CRC_ATTRCHG_SCISSOR;
	}
};

CRC_Attributes* CXR_VirtualAttributes_BSP2ShadowVolumeBack::ms_pBase = NULL;


// -------------------------------------------------------------------
//  CXR_Model_BSP2
// -------------------------------------------------------------------
int CXR_Model_BSP2::ms_Enable = -1 - MODEL_BSP_ENABLE_SPLINEWIRE - MODEL_BSP_ENABLE_FULLBRIGHT /* - MODEL_BSP_ENABLE_MINTESSELATION*/;

CRC_Attributes CXR_Model_BSP2::m_RenderZBuffer;
CRC_Attributes CXR_Model_BSP2::m_RenderZBufferCullCW;

//spCXR_WorldLightState CXR_Model_BSP2::m_spTempWLS;

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_BSP2, CXR_PhysicsModel);


// -------------------------------------------------------------------
CXR_Model_BSP2::CXR_Model_BSP2()
{
	MAUTOSTRIP(CXR_Model_BSP_ctor, MAUTOSTRIP_VOID);
	m_iView = 0;
	m_pView = NULL;
	m_pWorldView = NULL;
	SetThreadSafe(true);

	m_nFogTags = 0;
	m_MaxFogTags = 0;
//	m_VBSBTessLevel = 0.5f;

//	static bool bInitStaticAttribs = true;

//	if (bInitStaticAttribs)
	{
//		bInitStaticAttribs = false;

		m_RenderZBuffer.SetDefault();
		m_RenderZBuffer.Attrib_Enable(CRC_FLAGS_ZWRITE | CRC_FLAGS_STENCIL | CRC_FLAGS_ZCOMPARE | CRC_FLAGS_CULL);
	#ifdef PLATFORM_CONSOLE
		m_RenderZBuffer.Attrib_Disable(CRC_FLAGS_COLORWRITE | CRC_FLAGS_ALPHAWRITE);
	#endif
		m_RenderZBuffer.Attrib_StencilRef(128, 255);
		m_RenderZBuffer.Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_REPLACE);
		m_RenderZBuffer.Attrib_StencilWriteMask(255);
		m_RenderZBuffer.Attrib_TexCoordSet(1, 0);
	#ifdef XR_DEFAULTPOLYOFFSETSCALE
		m_RenderZBuffer.Attrib_PolygonOffset(XR_DEFAULTPOLYOFFSETSCALE, XR_DEFAULTPOLYOFFSET);
	#endif

		m_RenderZBufferCullCW = m_RenderZBuffer;
		m_RenderZBufferCullCW.Attrib_Enable(CRC_FLAGS_CULLCW);
	}
}

CXR_Model_BSP2::~CXR_Model_BSP2()
{
	MAUTOSTRIP(CXR_Model_BSP_dtor, MAUTOSTRIP_VOID);
	m_spLMTC = NULL;
	VB_FreeVBID();
}

void CXR_Model_BSP2::RenderPortalFence(CBSP2_RenderParams* _pRenderParams, int _iPortal, CPixel32 _Color)
{
	MAUTOSTRIP(CXR_Model_BSP2_RenderPortalFence, MAUTOSTRIP_VOID);
	if (!_pRenderParams->m_pVBM) return;
	CBSP2_PortalExt* pP = &m_lPortals[_iPortal];

	int nv = pP->m_nVertices;
	int iiv = pP->m_iiVertices;

	const uint32* piV = &m_piVertices[0];
	const CVec3Dfp32* pV = &m_pVertices[0];

	int32 Color = (pP->m_PortalID) ? 0x2fc0c0ff : 0x2fc0ffc0;

	_pRenderParams->m_pVBM->RenderWires(_pRenderParams->m_pViewData->m_CurVMat, pV, piV + iiv, nv, Color, true);
}

void CXR_Model_BSP2::RenderRPortal(CBSP2_RenderParams* _pRenderParams, const CRC_ClipVolume* _pPortal, CPixel32 _Color)
{
	MAUTOSTRIP(CXR_Model_BSP2_RenderRPortal, MAUTOSTRIP_VOID);
	if (!_pRenderParams->m_pVBM) return;

	_pRenderParams->m_pVBM->RenderWires(_pRenderParams->m_pViewData->m_CurVMat, _pPortal->m_Vertices, _pPortal->m_nPlanes, _Color, true);
}

void CXR_Model_BSP2::RenderNodePortals(CBSP2_RenderParams* _pRenderParams, int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderNodePortals, MAUTOSTRIP_VOID);
	int iPL = m_pNodes[_iNode].m_iPortalLeaf;
	if (!iPL) return;

	const CBSP2_PortalLeafExt* pPL = &m_pPortalLeaves[iPL];

	int np = pPL->m_nPortals;
	int iip = pPL->m_iiPortals;

	for(int p = 0; p < np; p++)
		RenderPortalFence(_pRenderParams, m_liPortals[iip + p], 0x40ff40);
}


void CXR_Model_BSP2::GetFaceBoundBox(int _iFace, CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_Model_BSP_GetFaceBoundBox, MAUTOSTRIP_VOID);
	CBSP2_Face* pF = &m_lFaces[_iFace];
	int nv = pF->m_nVertices;
	if (!nv)
	{
		_Box.m_Min = 0;
		_Box.m_Max = 0;
		return;
	}
	_Box.m_Min = _FP32_MAX;
	_Box.m_Max = -_FP32_MAX;

	for(int v = 0; v < nv; v++)
	{
		const CVec3Dfp32* pV = &m_lVertices[m_liVertices[pF->m_iiVertices + v]];
		for(int i = 0; i < 3; i++)
		{
			if (pV->k[i] < _Box.m_Min.k[i]) _Box.m_Min.k[i] = pV->k[i];
			if (pV->k[i] > _Box.m_Max.k[i]) _Box.m_Max.k[i] = pV->k[i];
		}
	}
}

void CXR_Model_BSP2::GetFaceListBoundBox(const uint32* _piFaces, int _nFaces, CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_Model_BSP_GetFaceBoundBox, MAUTOSTRIP_VOID);

	if (!_nFaces)
	{
		_Box.m_Min = 0;
		_Box.m_Max = 0;
		return;
	}
	CBox3Dfp32 Box;
	Box.m_Min.SetScalar(_FP32_MAX);
	Box.m_Max.SetScalar(-_FP32_MAX);

	TAP<CBSP2_Face> pFaces = m_lFaces;
	TAP<CVec3Dfp32> pV = m_lVertices;
	TAP<uint32> piV = m_liVertices;

	for(int f = 0; f < _nFaces; f++)
	{
		const CBSP2_Face& Face = pFaces[_piFaces[f]];
		int nv = Face.m_nVertices;
		int iiv = Face.m_iiVertices;
		for(int v = 0; v < nv; v++)
			Box.Expand(pV[piV[iiv + v]]);
	}

	_Box = Box;
}

void CXR_Model_BSP2::RenderPortalSurface(CXR_Engine* _pEngine, void* _pSurfContext, const CVec3Dfp32* _pVertices, int _nVertices, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderPortalSurface, MAUTOSTRIP_VOID);
	if (!_pEngine) return;

//	CXR_ViewContext* pVC = _pEngine->GetVC();

	CBSP2_ViewInstanceData ViewData = m_lViewData[m_iView];

	CBSP2_RenderParams RenderParams;
	RenderParams.m_pViewData = &ViewData;
	RenderParams.m_pCurrentEngine	= _pEngine;
	RenderParams.m_pVBM = _pEngine->m_pVBM;
	RenderParams.m_pSceneFog = _pEngine->m_pCurrentFogState;
	RenderParams.m_RenderInfo.Clear(_pEngine);
	RenderParams.m_pVBMatrixM2W	= const_cast<CMat4Dfp32*>(&_WMat);
	RenderParams.m_pVBMatrixW2V = const_cast<CMat4Dfp32*>(&_VMat);
	RenderParams.m_pVBFaceQueue = (CBSP2_VBFaceQueue*)_pEngine->GetVBM()->Alloc(sizeof(CBSP2_VBFaceQueue) * m_lVBFaceQueues.Len());
	RenderParams.m_pFaceQueueHeap = _pEngine->GetVBM()->Alloc_Int32(m_lFaces.Len());
	RenderParams.m_pSurfQueues = (CBSP2_SurfaceQueueData*)_pEngine->GetVBM()->Alloc(sizeof(CBSP2_SurfaceQueueData) * m_lSurfQueues.Len());

	{
		TAP_RCD<const CBSP2_VBFaceQueueData> lpVBFaceQueues(m_lVBFaceQueues);
		uint nQ = lpVBFaceQueues.Len();
		uint Pos = 0;
		for(uint i = 0; i < nQ; i++)
		{
			if(lpVBFaceQueues[i].m_MaxElem > 0)
			{
				CBSP2_VBFaceQueue& VBFaceQueue = RenderParams.m_pVBFaceQueue[i];
				VBFaceQueue.m_MaxElem = lpVBFaceQueues[i].m_MaxElem;
				VBFaceQueue.m_nElem = 0;
				VBFaceQueue.m_pQueue = &RenderParams.m_pFaceQueueHeap[Pos];
				Pos += VBFaceQueue.m_MaxElem;
			}
		}
		if(Pos != m_lFaces.Len())
			Error_static("CXR_Model_BSP2::RenderPortalSurface", "Internal error");
	}
	{
		TAP_RCD<const CBSP2_SurfaceQueue> lpSurfQueues(m_lSurfQueues);
		uint nQ = lpSurfQueues.Len();
		for(uint i = 0; i < nQ; i++)
		{
			CBSP2_SurfaceQueueData& SurfQueue = RenderParams.m_pSurfQueues[i];
			SurfQueue.Create(lpSurfQueues[i]);
			SurfQueue.Clear();
		}
	}

	m_pView = m_lspViews[m_iView];
//	m_pViewData	= &m_lViewData[m_iView];
	CMat4Dfp32 Mat; Mat.Unit();
	RenderParams.m_pSceneFog = _pEngine->GetFogState();
	// URGENTFIXME: ms_PhysWMat WTF!?
	// _pEngine->GetFogState()->SetTransform(&ms_PhysWMat);
	_pEngine->m_pCurrentFogState->SetTransform(&Mat);

//	if ((m_spLMTC != NULL) && !m_spLMTC->m_spCurrentWLS) return;
	RenderParams.m_CurrentRCCaps = _pEngine->m_pRender->Caps_Flags();
	RenderParams.m_pVBM = _pEngine->GetVBM();
//	m_spCurrentWLS = pVC->m_spLightState;
//	RenderParams.m_CurrentTLEnable = MODEL_BSP_TL_ENABLE_CONDITION(_pEngine);

	CXR_MediumDesc Medium;
	Medium.SetAir();
	RenderParams.m_pCurMedium = &Medium;

	CXR_AnimState AnimState;
	RenderParams.m_pCurAnimState = &AnimState;

	uint32 iFace = (mint) _pSurfContext;
//	ConOut(CStrF("RPS %d", iFace));
	{
		if (!VB_AllocateFaceQueues(RenderParams) ||
			!VB_AllocateSurfaceQueues(RenderParams) ||
			!VB_AllocatePrimitiveQueues(RenderParams))
			return;

		PrepareShaderQueue(&RenderParams);
		VB_ClearQueues(&RenderParams);
		VB_ClearFaceQueues(&RenderParams);

//		CBox3Dfp32 Bound;
//		GetFaceBoundBox(iFace, Bound);

		VB_RenderFaces(&RenderParams, m_lFaces[iFace].m_iVB, &iFace, 1, m_lspSurfaces[m_lFaces[iFace].m_iSurface]->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps), false);

		RenderShaderQueue(&RenderParams);
		VB_RenderFaceQueues(&RenderParams);
		VB_Flush(&RenderParams);
		VB_RenderQueues(&RenderParams);
	}
}

void CXR_Model_BSP2::PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip,
	const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_BSP_PreRender, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP2::PreRender, XR_BSPMODEL);

	if (m_lPortalLeaves.Len())
	{
		CXR_FogState* pFog = _pEngine->m_pCurrentFogState;
		if (pFog) pFog->AddModel(this, CVec3Dfp32(0));
	}

/*	if (!m_pView)
	{
		InitializeListPtrs();
		m_nPortalFaces = 0;
		m_liPortalFaces.SetLen(CXR_MAXPORTALFACES);
		Portal_AddPortalFaces_r(1);
	}*/

	m_pView = (m_lspViews.ValidPos(m_iView)) ? m_lspViews[m_iView] : (spCBSP2_ViewInstance)NULL;
	CBSP2_ViewInstanceData ViewData;
	if (m_lPortalLeaves.Len() && m_pView)
	{
		_WMat.Multiply(_VMat, ViewData.m_CurVMat);
		ViewData.m_CurWMat = _WMat;

		// World with portals.
		const uint16* piVisPortalLeaves = m_pView->m_liVisPortalLeaves.GetBasePtr();
		for(int iiPL = 0; iiPL < m_pView->m_nVisLeaves; iiPL++)
		{
			int iPL = piVisPortalLeaves[iiPL];
			int iRP = m_pView->m_liLeafRPortals[iPL];
			if (!iRP) continue;
			CBSP2_PortalLeafExt* pPL = &m_lPortalLeaves[iPL];

			for(int iiRefFace = 0; iiRefFace < pPL->m_nRefFaces; iiRefFace++)
			{
				uint32 iFace = m_liPLFaces[pPL->m_iiRefFaces + iiRefFace];
				CBSP2_Face* pF = &m_lFaces[iFace];
#ifdef MODEL_BSP_BACKFACECULL
				if (m_lPlanes[pF->m_iPlane].Distance(ViewData.m_CurLocalVP) < 0.0f) continue;
#endif
				CVec3Dfp32 Verts[64];
				int nVerts = pF->m_nVertices;
				int v;
				for(v = 0; v < nVerts; v++)
					Verts[v] = m_lVertices[m_liVertices[pF->m_iiVertices + v]];

				int bClip;
				nVerts = BSPModel_CutFace3(Verts, nVerts, m_pView->m_lRPortals[iRP].m_Planes, m_pView->m_lRPortals[iRP].m_nPlanes, bClip);
				for(v = 0; v < nVerts; v++) Verts[v] *= _WMat;
				if (nVerts > 2)
				{
//					ConOut(CStrF("    Adding mirror, iView %d, %d verts, %d verts post clip, %d clip planes, Face %d", m_iView, nVerts, pF->m_nVertices, m_pView->m_lRPortals[iRP].m_nPlanes, iFace));
					_pEngine->Render_AddMirror(Verts, nVerts, _WMat, _VMat, 0x407f7f7f, this, (void*) (mint)iFace);
				}
			}

//			if (!_pEngine->GetVCDepth()) return;

			CXR_Model* pSky = _pEngine->GetVC()->GetSky();
			if (pSky && pSky->Sky_GetInterface())
			{
				CXR_SkyInterface* pSkyInterface = pSky->Sky_GetInterface();

//			if (_pEngine->GetVCDepth()) ConOut("PreRender, Adding skyfaces.");
				for(int iiSkyFace = 0; iiSkyFace < pPL->m_nSkyFaces; iiSkyFace++)
				{
					uint32 iFace = m_liPLFaces[pPL->m_iiSkyFaces + iiSkyFace];
					CBSP2_Face* pF = &m_lFaces[iFace];
#ifdef MODEL_BSP_BACKFACECULL
					if (m_lPlanes[pF->m_iPlane].Distance(ViewData.m_CurLocalVP) < 0.0f) continue;
#endif
					CVec3Dfp32 Verts[64];
					int nVerts = pF->m_nVertices;
					int v;
					for(v = 0; v < nVerts; v++)
						Verts[v] = m_lVertices[m_liVertices[pF->m_iiVertices + v]];

//					int bClip;
//					nVerts = BSPModel_CutFace3(Verts, nVerts, m_pView->m_lRPortals[iRP].m_Planes, m_pView->m_lRPortals[iRP].m_nPlanes, bClip);
//					for(v = 0; v < nVerts; v++) Verts[v] *= _WMat;
					if (nVerts > 2)
					{
//			if (_pEngine->GetVCDepth()) ConOut("Expand");
						pSkyInterface->Sky_ExpandFrustrum(_pEngine, _WMat, ViewData.m_CurVMat, Verts, nVerts);
					}
				}
			}
		}
	}
	else
	{
		if (!m_liRefFaces.Len()) return;

		m_pView = m_lspViews[m_iView];

		CBox3Dfp32 Box;
		Phys_GetBound_Box(_WMat, Box);
//		Box.m_Min += CVec3Dfp32::GetMatrixRow(_WMat, 3);
//		Box.m_Max += CVec3Dfp32::GetMatrixRow(_WMat, 3);

//		CRC_ClipVolume Clip;
//		if (!_pViewClip->View_GetClip_Box(Box.m_Min, Box.m_Max, 0, 0, &Clip, NULL)) return;
		if (!_pViewClip->View_GetClip_Box(Box.m_Min, Box.m_Max, 0, 0, NULL, NULL))
			return;

		// Create front-plane in model-space.
		CMat4Dfp32 VMatInv, WMatInv;
		_VMat.InverseOrthogonal(VMatInv);
		_WMat.InverseOrthogonal(WMatInv);
		CVec3Dfp32 VP(0);
		VP *= VMatInv;
		VP *= WMatInv;

		_WMat.Multiply(_VMat, ViewData.m_CurVMat);

		// Entity
		for(int iiRefFace = 0; iiRefFace < m_liRefFaces.Len(); iiRefFace++)
		{
			uint32 iFace = m_liRefFaces[iiRefFace];
			CBSP2_Face* pF = &m_lFaces[iFace];
#ifdef MODEL_BSP_BACKFACECULL
			if (m_lPlanes[pF->m_iPlane].Distance(VP) < 0.0f) continue;
#endif
			CVec3Dfp32 Verts[MODEL_BSP_MAXFACEVERTICES];
			int nVerts = pF->m_nVertices;
			int v;
			for(v = 0; v < nVerts; v++)
				Verts[v] = m_lVertices[m_liVertices[pF->m_iiVertices + v]];

			for(v = 0; v < nVerts; v++) Verts[v] *= _WMat;
//			nVerts = BSPModel_CutFace3(Verts, nVerts, Clip.m_Planes, Clip.m_nPlanes, bClip);

			_pEngine->Render_AddMirror(Verts, nVerts, _WMat, _VMat, 0x407f7f7f, this, (void*)(mint)iFace);
		}
	}
}

void CXR_Model_BSP2::RenderGetPrimCount(const uint32* _piFaces, int _nFaces, int& _nV, int& _nPrim)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderGetPrimCount, MAUTOSTRIP_VOID);

#ifdef MODEL_BSP_TRIANGLEFAN
	int nVert = 0;
	int nPrim = 0;

	int nPolys = 0;
	const uint32* piF = _piFaces;
	for(int f = 0; f < _nFaces; f++)
	{
		PREFETCH(&m_pFaces[piF[f+2]].m_nVertices);

		const CBSP2_Face* pF = &m_pFaces[piF[f]];
		int nv = pF->m_nVertices;
		nVert += nv;
		nPrim += nv + 2;
	}
	_nPrim = nPrim;
	_nV = nVert;

#else
	int nVert = 0;
	int nPrim = 0;
	int nP = 2;

	const uint32* piF = _piFaces;
	for(int f = 0; f < _nFaces; f++)
	{
		PREFETCH(&m_pFaces[piF[f+2]].m_nVertices);

		const CBSP2_Face* pF = &m_pFaces[piF[f]];
		int nv = pF->m_nVertices;
		nVert += nv;

		int nPolyPrim = (nv-2)*3;
		if (nPolyPrim+nP > 255)
		{
			// Restart
			nPrim += nP;
			nP = 2;
		}
		nP += nPolyPrim;
	}
	if (nP > 2)
		nPrim += nP;

	_nPrim = nPrim;
	_nV = nVert;
#endif
}

void CXR_Model_BSP2::RenderGetPrimCount_Triangles(const uint32* _piFaces, int _nFaces, int& _nV, int& _nPrim)
{
	int nVert = 0;
	int nPrim = 0;
	int nP = 0;

	const uint32* piF = _piFaces;
	for(int f = 0; f < _nFaces; f++)
	{
		const CBSP2_Face* pF = &m_pFaces[piF[f]];
		int nv = pF->m_nVertices;
		nVert += nv;
		nP += (nv-2)*3;;
	}
	_nPrim = nP;
	_nV = nVert;
}

void CXR_Model_BSP2::RenderGetPrimCount_KnitStrip(const uint32* _piFaces, int _nFaces, int& _nV, int& _nPrim)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderGetPrimCount_KnitStrip, MAUTOSTRIP_VOID);

#ifdef PLATFORM_PS2

	int nVert = 0;
	int nPrim = 2;
	int nPolys = 0;
	int nvStrip = 0;
	const uint32* piF = _piFaces;

	for(int f = 0; f < _nFaces; f++)
	{
		const CBSP2_Face* pF = &m_pFaces[piF[f]];
		int nv = pF->m_nVertices;

		if (nv + nvStrip > MODEL_BSP_MAXKNITSTRIP)
		{
			nPrim += 2;
			nvStrip = 0;
		}

		nVert += nv;
		nPrim += nv;
		nvStrip += nv;
	}
	_nPrim = nPrim;
	_nV = nVert;

#else

	int nVert = 0;
	int nPrim = 2;
//	int nTri = 0;
//	int nPolys = 0;
	int nvStrip = 0;
	const uint32* piF = _piFaces;
	for(int f = 0; f < _nFaces; f++)
	{
		const CBSP2_Face* pF = &m_pFaces[piF[f]];
		int nv = pF->m_nVertices;

#ifdef MODEL_BSP_KNITSTRIPCULLFIX
		nv = (nv + 1) & ~1;
#endif

		if (nv + 2 + nvStrip > MODEL_BSP_MAXKNITSTRIP)
		{
			nPrim += 2;
			nvStrip = 0;
		}

		nVert += nv;
		nPrim += nv + 2;
		nvStrip += nv + 2;
	}
	_nPrim = nPrim;
	_nV = nVert;
	
#endif
	
//	LogFile(CStrF("Count %d faces, %d prim", _nFaces, _nPrim));
}
/*
static void SnapToGrid(CVec3Dfp32& _Pos, const CVec3Dfp32& _UVec, const CVec3Dfp32& _VVec)
{
	CVec3Dfp32 UVecNrm = _UVec; UVecNrm.Normalize();
	CVec3Dfp32 VVecNrm = _VVec; VVecNrm.Normalize();

	CPlane3Dfp32 Plane;
	Plane.Create(_Pos, _Pos + _UVec, _Pos + _VVec);
	CVec3Dfp32 Origo = Plane.n * -Plane.d;
	CVec3Dfp32 Vector = _Pos - Origo;

	fp32 USnap = Vector * UVecNrm;
	fp32 VSnap = Vector * VVecNrm;

	USnap = RoundToInt((USnap - 0.5f) / _UVec.Length()) * _UVec.Length();
	VSnap = RoundToInt((VSnap - 0.5f) / _VVec.Length()) * _VVec.Length();

	_Pos = Origo + UVecNrm * USnap + VVecNrm * VSnap;
}
*/
void CXR_Model_BSP2::Tesselate(const uint32* _piFaces, int _nFaces, int _nV, CVec3Dfp32* _pV, CVec2Dfp32* _pTV1, CVec2Dfp32* _pTV2, CPixel32* _pCol, CVec3Dfp32* _pN, CVec3Dfp32* _pTangU, CVec3Dfp32* _pTangV, fp32* _pLMScale, uint32* _piFaceVertices, CXW_Surface* _pSurf)
{
	MAUTOSTRIP(CXR_Model_BSP_Tesselate, MAUTOSTRIP_VOID);
	CVec3Dfp32 UVec;
	CVec3Dfp32 VVec;
	fp32 UVecLenSqrInv;
	fp32 VVecLenSqrInv;
	fp32 UOffset;
	fp32 VOffset;
	fp32 TxtWidthInv = 0;
	fp32 TxtHeightInv = 0;
	fp32 MidPixelAdjustLM = 0;
	fp32 TxtWidthInvLM = 1.0f / 512.0f;
	fp32 TxtHeightInvLM = 1.0f / 512.0f;

	if (_pTV1 || _pTV2)
	{
		CXW_Surface* pSurf = _pSurf; //m_lspSurfaces[pFace->m_iSurface];
		CVec3Dfp32 ReferenceDim = pSurf->GetTextureMappingReferenceDimensions();

		TxtWidthInv = 1.0f / ReferenceDim[0];
		TxtHeightInv = 1.0f / ReferenceDim[1];

		if (_pTV2 && !_pTV1)
			Error("Tesselate", "Internal error.");
	}



	CVec3Dfp32* pV = _pV;
	CVec2Dfp32* pTV = _pTV1;
	CVec2Dfp32* pLMTV = _pTV2;
	CPixel32* pC = _pCol;
	CVec3Dfp32* pN = _pN;
	CVec3Dfp32* pTangU = _pTangU;
	CVec3Dfp32* pTangV = _pTangV;
	fp32* pLMScale = _pLMScale;
	
//	int nP = 0;
	int nV = 0;
//	int iVBase = 0;
	const uint32* piF = _piFaces;
	for(int i = 0; i < _nFaces; i++)
	{
		int iFace = *(piF++);
		const CBSP2_Face* pF = &m_pFaces[iFace];
		int iVBase = nV;
		if (_piFaceVertices) _piFaceVertices[i] = iVBase;
		int nv = pF->m_nVertices;
		int iiv = pF->m_iiVertices;

		M_ASSERT(nv <= CRC_MAXPOLYGONVERTICES, "!");

		if(pV)
		{
			for(int v = 0; v < nv; v++)
			{
				*pV = m_pVertices[m_piVertices[v + iiv]];
				pV++;
			}
		}

		CVec2Dfp32 TVLM;
		if(pTV)
		{
			int iMapping = pF->m_iMapping;
			UVec = m_lMappings[iMapping].m_U;
			VVec = m_lMappings[iMapping].m_V;
			UVecLenSqrInv = 1.0f/(UVec*UVec);
			VVecLenSqrInv = 1.0f/(VVec*VVec);
			UOffset = m_lMappings[iMapping].m_UOffset;
			VOffset = m_lMappings[iMapping].m_VOffset;

//			fp32 MidPixelAdjust = 0.5f;
			fp32 InvLMScale = 1;
			int LMWidth = 0;
			int LMHeight = 0;
			int bUseLM = (pF->m_Flags & XW_FACE_LIGHTMAP);
			const CBSP2_LightMapInfo* pLMI = NULL;
			if (bUseLM && pLMTV)
			{
				int iLM = pF->m_iLightInfo;
				pLMI = &m_lLightMapInfo[iLM];
				int iLMC = m_lLightMapInfo[iLM].m_iLMC * 4;

				LMWidth = pLMI->m_LMCWidthHalf*2;
				LMHeight= pLMI->m_LMCHeightHalf*2;

				InvLMScale = 1.0f / (1 << pLMI->m_ScaleShift);
				TxtWidthInvLM = 1.0f / fp32(m_lLMDimensions[iLMC].x);
				TxtHeightInvLM = 1.0f / fp32(m_lLMDimensions[iLMC].y);
//				MidPixelAdjustLM = MidPixelAdjust*LMScale;
				MidPixelAdjustLM = 0.5f;
			}

			fp32 lUProj[CRC_MAXPOLYGONVERTICES];
			fp32 lVProj[CRC_MAXPOLYGONVERTICES];

			CVec2Dfp32 TProjMin(_FP32_MAX);
			CVec2Dfp32 TProjMax(-_FP32_MAX);
			int v;
			for(v = 0; v < nv; v++)
			{
				CVec2Dfp32 TV;

				const CVec3Dfp32* pS = &m_pVertices[m_piVertices[v + iiv]];

				fp32 UProj = (pS->k[0]*UVec.k[0] + pS->k[1]*UVec.k[1] + pS->k[2]*UVec.k[2])*UVecLenSqrInv + UOffset;
				fp32 VProj = (pS->k[0]*VVec.k[0] + pS->k[1]*VVec.k[1] + pS->k[2]*VVec.k[2])*VVecLenSqrInv + VOffset;
				lUProj[v] = UProj;
				lVProj[v] = VProj;
				if (UProj > TProjMax.k[0]) TProjMax.k[0] = UProj;
				if (VProj > TProjMax.k[1]) TProjMax.k[1] = VProj;
				if (UProj < TProjMin.k[0]) TProjMin.k[0] = UProj;
				if (VProj < TProjMin.k[1]) TProjMin.k[1] = VProj;
				TV.k[0] = (UProj/* + MidPixelAdjust*/) * TxtWidthInv;
				TV.k[1] = (VProj/* + MidPixelAdjust*/) * TxtHeightInv;
				pTV[v] = TV;
			}

			if(pLMTV)
			{
				fp32 IntensityScale = 1.0f;
				if(pLMI) IntensityScale = pLMI->m_IntensityScale.Getfp32();
				for(v = 0; v < nv; v++)
				{
					if (pLMI)
					{
						CVec2Dfp32 TVLM;
						TVLM.k[0] = (lUProj[v] - TProjMin.k[0]) * InvLMScale;
						TVLM.k[1] = (lVProj[v] - TProjMin.k[1]) * InvLMScale;
						pLMTV[v][0] = (TVLM[0] + (fp32)pLMI->m_LMCOffsetXHalf*2 + MidPixelAdjustLM) * TxtWidthInvLM;
						pLMTV[v][1] = (TVLM[1] + (fp32)pLMI->m_LMCOffsetYHalf*2 + MidPixelAdjustLM) * TxtHeightInvLM;
					}
					else
					{
						pLMTV[v][0] = pTV[v][0];
						pLMTV[v][1] = pTV[v][1];
					}

					if(pLMScale)
					{
						pLMScale[v] = IntensityScale;
					}
				}
			}

			CVec2Dfp32 TMid;
			TMid[0] = (TProjMin[0] + TProjMax[0]) * 0.5f * TxtWidthInv;
			TMid[1] = (TProjMin[1] + TProjMax[1]) * 0.5f * TxtHeightInv;
			TMid.k[0] = RoundToInt(TMid.k[0]/16.0f)*16.0f;
			TMid.k[1] = RoundToInt(TMid.k[1]/16.0f)*16.0f;

#ifdef PLATFORM_PS2
			TMid[0] = TProjMin[0] * TxtWidthInv;
			TMid[1] = TProjMin[1] * TxtHeightInv;
#endif
			for(v = 0; v < nv; v++)
			{
				pTV[v] -= TMid;
#ifdef PLATFORM_PS2
				if(pTV[v][0] < 0) pTV[v][0] = 0;
				if(pTV[v][1] < 0) pTV[v][1] = 0;
#endif
			}

			pTV += nv;
			if (pLMTV)
				pLMTV += nv;
			if(pLMScale)
				pLMScale += nv;
		}

		// Texture coordinates?
/*		if(pTV)
		{
			int itv = pF->m_iTVertices;
			for(int v = 0; v < nv; v++)
			{
				*pTV = m_pTVertices[v + itv];
				pTV++;
			}
		}

		// Lightmap coordinates?
		if(pLMTV)
		{
			int itv = pF->m_iLMTVertices;
			for(int v = 0; v < nv; v++)
			{
				*pLMTV = m_pTVertices[v + itv];
				pLMTV++;
			}
		}
*/
		// Colors?
		if(pC)
		{
			// Light?
			CPixel32* pCol = &pC[nV];
			for(int v = 0; v < nv; v++) 
				pCol[v] = 0xffffffff;
		}

		if(pN)
		{
			if (pF->m_Flags & XW_FACE_SMOOTHNORMALS)
			{
				uint32* piLN = m_liLightNormals.GetBasePtr();

				int iin = pF->m_iiNormals;
				for(int v = 0; v < nv; v++)
				{
					*pN = m_pVertices[piLN[v + iin]];
					pN++;
				}
			}
			else
			{
				const CVec3Dfp32& n = m_pPlanes[pF->m_iPlane].n;
				for(int v = 0; v < nv; v++)
				{
					*pN = n;
					pN++;
				}
			}
		}

		if (_pN && (pTangU || pTangV))
		{
			if (!pTangU || !pTangV)
				Error("Tessellate", "Both U and V tangent arrays be written.");

			const CPlane3Dfp32& Plane = m_pPlanes[pF->m_iPlane];
			const CXR_PlaneMapping& Mapping = m_lMappings[pF->m_iMapping];

			fp32 ULengthRecp = 1.0f / Mapping.m_U.Length();
			fp32 VLengthRecp = 1.0f / Mapping.m_V.Length();

			{
				CVec3Dfp32 TangU;
				Mapping.m_U.Scale(ULengthRecp, TangU);
				TangU.Combine(Plane.n, -(Plane.n * TangU), TangU);
				TangU.Normalize();
				for(int v = 0; v < nv; v++)
				{
					*pTangU = TangU;
					pTangU++;
				}
			}

			{
				CVec3Dfp32 TangV;
				Mapping.m_V.Scale(VLengthRecp, TangV);
				TangV.Combine(Plane.n, -(Plane.n * TangV), TangV);
				TangV.Normalize();
				for(int v = 0; v < nv; v++)
				{
					*pTangV = TangV;
					pTangV++;
				}
			}
		}

/*		pPrim[nP++] = CRC_RIP_TRIFAN + ((nv+2) << 8);
		pPrim[nP++] = nv;

		{
			for(int v = 0; v < nv; v++)
				pPrim[nP++] = iVBase+v;
		}
*/
		nV += nv;
	}

	if (nV != _nV)
		Error("Tesselate", CStrF("Vertex count missmatch. (%d != %d)", nV, _nV));

	// Orthonormalize
	if (_pN && (_pTangU || _pTangV))
	{
		pN = _pN;
		pTangU = _pTangU;
		pTangV = _pTangV;

		for(int v = 0; v < nV; v++)
		{
			const CVec3Dfp32& N = pN[v];
			CVec3Dfp32& TgU = pTangU[v];
			CVec3Dfp32& TgV = pTangV[v];
			TgU.Combine(N, -(N * TgU), TgU);
			TgV.Combine(N, -(N * TgV), TgV);
			TgU.Normalize();
			TgV.Normalize();

			if (M_Fabs(N * TgU) > 0.01f ||
				M_Fabs(N * TgV) > 0.01f)
				LogFile("Fuckade tangenter: " + N.GetString() + TgU.GetString() + TgV.GetString());

		}
	}


//	pPrim[nP++] = CRC_RIP_END + (1 << 8);
//	_pVB->Render_IndexedPrimitives(pPrim, nP);
//	return true;
}

int CXR_Model_BSP2::RenderTesselatePrim(const uint32* _piFaces, int _nFaces, uint16* _pPrim)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderTesselatePrim, 0);
#ifdef MODEL_BSP_TRIANGLEFAN
	int nP = 0;

	const uint32* piF = _piFaces;
	for(int i = 0; i < _nFaces; i++)
	{
		int iFace = *(piF++);
		const CBSP2_Face* pF = &m_lFaces[iFace];
		int nv = pF->m_nVBVertices;
		int iv = pF->m_iiVBVertices;
		_pPrim[nP++] = CRC_RIP_TRIFAN + ((nv+2) << 8);
		_pPrim[nP++] = nv;

		{
			for(int v = 0; v < nv; v++)
				_pPrim[nP++] = iv+v;
		}
	}

	return nP;

#else
	int nPrim = 0;
	int nP = 2;
	int nTri = 0;

	const uint32* piF = _piFaces;
	for(int i = 0; i < _nFaces; i++)
	{
		int iFace = *(piF++);
		const CBSP2_Face* pF = &m_lFaces[iFace];
		int nv = pF->m_nVertices; //pF->m_nVBVertices;
		int iv = pF->m_iiVBVertices;

		int nPolyPrim = (nv-2)*3;
		if (nP + nPolyPrim > 255)
		{
			// Restart
			_pPrim[0] = CRC_RIP_TRIANGLES + (nP << 8);
			_pPrim[1] = nTri;
			nPrim += nP;
			_pPrim += nP;
			nP = 2;
			nTri = 0;
		}

		{
			nv -= 2;
			nTri += nv;
			for(int v = 0; v < nv; v++)
			{
				_pPrim[nP++] = iv;
				_pPrim[nP++] = iv+v+1;
				_pPrim[nP++] = iv+v+2;
			}
		}
	}

	if (nP > 2)
	{
	_pPrim[0] = CRC_RIP_TRIANGLES + (nP << 8);
		_pPrim[1] = nTri;
		nPrim += nP;
	}

	return nPrim;
#endif
}

int CXR_Model_BSP2::RenderTesselatePrim_Triangles(const uint32* _piFaces, int _nFaces, uint16* _pPrim)
{
	int nP = 0;

	const uint32* piF = _piFaces;
	for(int i = 0; i < _nFaces; i++)
	{
		int iFace = *(piF++);
		const CBSP2_Face* pF = &m_lFaces[iFace];
		int nv = pF->m_nVertices; //pF->m_nVBVertices;
		int iv = pF->m_iiVBVertices;

		{
			nv -= 2;
			for(int v = 0; v < nv; v++)
			{
				_pPrim[nP++] = iv;
				_pPrim[nP++] = iv+v+1;
				_pPrim[nP++] = iv+v+2;
			}
		}
	}
	return nP;
}

int CXR_Model_BSP2::RenderTesselatePrim_KnitStrip(const uint32* _piFaces, int _nFaces, uint16* _pPrim)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderTesselatePrim_KnitStrip, false);

#ifdef PLATFORM_PS2

	// Transform TriangleFans into TriangleStrips, and merge them together into one long TriangleStrip,
	// thus greatly reducing overhead in primitivehandling. The ADC bit is used at the weldingpoints.


	int nP = 2;
	int nvStrip = 0;
	int iPCurrentStrip = 0;

	const uint32* piF = _piFaces;

	// keep track of culling (!) and fanconversion
	int	oddeven = 0; 

	for(int i = 0; i < _nFaces; i++)
	{
		int iFace = *(piF++);

		const CBSP2_Face* pF = &m_lFaces[iFace];

		int nv = pF->m_nVertices;
		int iv = pF->m_iiVBVertices;

		// split if strip is too long!
		if (nv + nvStrip > MODEL_BSP_MAXKNITSTRIP)
		{ 
			_pPrim[iPCurrentStrip] = CRC_RIP_TRISTRIP + (nvStrip+2 << 8);
			_pPrim[iPCurrentStrip+1] = nvStrip;
			nvStrip = 0;
			iPCurrentStrip = nP;
			nP += 2;
			oddeven = 0;
		}

		int iv1 = 0;
		int iv2 = nv-1;

		int iFirst = nP;

		while( iv1 <= iv2 )
		{
			if( oddeven )
			{
				_pPrim[nP++] = iv + iv1;
				iv1++;
			}
			else
			{
				_pPrim[nP++] = iv + iv2;
				iv2--;
			}
			
			oddeven ^= 1;
		}
		
		// set ADC bit
		_pPrim[iFirst] |= 0x8000;
		_pPrim[iFirst+1] |= 0x8000;
		
		nvStrip += nv;


	}

	_pPrim[iPCurrentStrip] = CRC_RIP_TRISTRIP + ((nvStrip+2) << 8);
	_pPrim[iPCurrentStrip+1] = nvStrip;

	return nP;



#else // PLATFORM_PS2

	int nP = 0;
	const uint32* piF = _piFaces;

	nP += 2;
	int iPCurrentStrip = 0;

	int nvStrip = 0;
	for(int i = 0; i < _nFaces; i++)
	{
		int iFace = *(piF++);
		const CBSP2_Face* pF = &m_lFaces[iFace];
		int nv = pF->m_nVertices; //pF->m_nVBVertices;
		int iv = pF->m_iiVBVertices;

#ifdef MODEL_BSP_KNITSTRIPCULLFIX
		int nvreal = (nv + 1) & ~1;
#else
		int nvreal = nv;
#endif
		if (nvreal + 2 + nvStrip > MODEL_BSP_MAXKNITSTRIP)
		{
			_pPrim[iPCurrentStrip] = CRC_RIP_TRISTRIP + ((nvStrip+2) << 8);
			_pPrim[iPCurrentStrip+1] = nvStrip;
			nvStrip = 0;
			iPCurrentStrip = nP;
			nP += 2;
		}

		_pPrim[nP++] = iv;
		int iv1 = 0;
		int iv2 = nv-1;
		while(1)
		{
			_pPrim[nP++] = iv + iv1;
			iv1++;
			if (iv1 == iv2)
			{
				_pPrim[nP++] = iv + iv2;
				break;
			}

			_pPrim[nP++] = iv + iv2;
			iv2--;
			if (iv1 == iv2)
			{
				_pPrim[nP++] = iv + iv1;
				break;
			}
		}
#ifdef MODEL_BSP_KNITSTRIPCULLFIX
		if (nv & 1)
		{
			_pPrim[nP] = _pPrim[nP-1];
			nP++;
		}
#endif
		_pPrim[nP] = _pPrim[nP-1];
		nP++;

		nvStrip += nvreal+2;
	}

	_pPrim[iPCurrentStrip] = CRC_RIP_TRISTRIP + ((nvStrip+2) << 8);
	_pPrim[iPCurrentStrip+1] = nvStrip;

	return nP;

#endif
}


bool CXR_Model_BSP2::RenderTesselateVBPrim(const uint32* _piFaces, int _nFaces, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderTesselateVBPrim, false);
	int nVert = 0;
	int nPrim = 0;
	RenderGetPrimCount(_piFaces, _nFaces, nVert, nPrim);
//	nPrim += 1;	// For CRC_RIP_END

	uint16* pPrim = _pVBM->Alloc_Int16(nPrim);
	if (!pPrim) return false;

	int nPrim2 = RenderTesselatePrim(_piFaces, _nFaces, pPrim);

	M_ASSERT(nPrim == nPrim2, "!");

	_pVB->Render_IndexedPrimitives(pPrim, nPrim2);
	return true;
}


bool CXR_Model_BSP2::RenderTesselate(const uint32* _piFaces, int _nFaces, int _TessFlags, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, uint32* _piFaceVertices)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderTesselate, false);
	int nVert = 0;
	int nPrim = 0;
	RenderGetPrimCount(_piFaces, _nFaces, nVert, nPrim);

	CVec3Dfp32* pV = _pVBM->Alloc_V3(nVert);
	uint16* pPrim = _pVBM->Alloc_Int16(nPrim);
	if (!pV || !pPrim) return false;

	uint16* pPrimTmp = pPrim;

	if (!_pVB->AllocVBChain(_pVBM, false))
		return false;
	_pVB->Geometry_VertexArray(pV, nVert, true);
//	CVec2Dfp32* pTV = NULL;
//	CVec2Dfp32* pLMTV = NULL;
	CPixel32* pC = NULL;
	CVec3Dfp32* pN = NULL;

	if (_TessFlags & 3)
	{
		ConOut(CStrF("§cf80WARNING: (CXR_Model_BSP2::RenderTesselate) TessFlags %d", _TessFlags));
	}

	if (_TessFlags & 4)
	{
		pC = _pVBM->Alloc_CPixel32(nVert);
		if (!pC) return false;
		_pVB->Geometry_ColorArray(pC);
	}
	
	if (_TessFlags & 8)
	{
		pN = _pVBM->Alloc_V3(nVert);
		if (!pN) return false;
		_pVB->Geometry_Normal(pN);
	}
	
#ifdef MODEL_BSP_TRIANGLEFAN
	int nP = 0;
#else
	int nTri = 0;
	int nP = 2;
#endif
	int nV = 0;
//	int iVBase = 0;
	const uint32* piF = _piFaces;
	for(int i = 0; i < _nFaces; i++)
	{
		int iFace = *(piF++);
		const CBSP2_Face* pF = &m_pFaces[iFace];
		int iVBase = nV;
		if (_piFaceVertices) _piFaceVertices[i] = iVBase;
		int nv = pF->m_nVertices;
		int iiv = pF->m_iiVertices;

		{
			for(int v = 0; v < nv; v++)
			{
				*pV = m_pVertices[m_piVertices[v + iiv]];
				pV++;
			}
		}

		// Colors?
		if(_TessFlags & 4)
		{
			// Light?
			CPixel32* pCol = &pC[nV];
			for(int v = 0; v < nv; v++) 
				pCol[v] = 0xffffffff;
		}

		if(_TessFlags & 8)
		{
			if (pF->m_Flags & XW_FACE_SMOOTHNORMALS)
			{
				uint32* piLN = m_liLightNormals.GetBasePtr();

				int iin = pF->m_iiNormals;
				for(int v = 0; v < nv; v++)
				{
					*pN = m_pVertices[piLN[v + iin]];
					pN++;
				}
			}
			else
			{
				const CVec3Dfp32& n = m_pPlanes[pF->m_iPlane].n;
				for(int v = 0; v < nv; v++)
				{
					*pN = n;
					pN++;
				}
			}
		}

#ifdef MODEL_BSP_TRIANGLEFAN
		pPrim[nP++] = CRC_RIP_TRIFAN + ((nv+2) << 8);
		pPrim[nP++] = nv;

		{
			for(int v = 0; v < nv; v++)
				pPrim[nP++] = iVBase+v;
		}
#else

		int nPolyPrim = (nv-2)*3;
		if (nP + nPolyPrim > 255)
		{
			// Restart
			pPrimTmp[0] = CRC_RIP_TRIANGLES + (nP << 8);
			pPrimTmp[1] = nTri;
			pPrimTmp += nP;
			nP = 2;
			nTri = 0;
		}

		{
			int nPolyTri = nv-2;
			nTri += nPolyTri;
			for(int v = 0; v < nPolyTri; v++)
			{
				pPrimTmp[nP++] = iVBase;
				pPrimTmp[nP++] = iVBase+v+1;
				pPrimTmp[nP++] = iVBase+v+2;
			}
		}

#endif

		nV += nv;
	}

#ifdef MODEL_BSP_TRIANGLEFAN
	_pVB->Render_IndexedPrimitives(pPrim, nP);
#else
	if (nP > 2)
	{
		pPrimTmp[0] = CRC_RIP_TRIANGLES + (nP << 8);
		pPrimTmp[1] = nTri;
		pPrimTmp += nP;
	}
	_pVB->Render_IndexedPrimitives(pPrim, pPrimTmp-pPrim);
#endif
	return true;
}

//#define MODEL_BSP_NODEPTHFOG

void CXR_Model_BSP2::VB_CopyGeometry(CBSP2_RenderParams* _pRenderParams, int _iVB, CBSP2_VBChain* _pVB)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_CopyGeometry, MAUTOSTRIP_VOID);
	const CBSP2_VBInfo* pBSPVBI = &m_lVBInfo[_iVB];

	M_ASSERT(pBSPVBI->m_VBID != (uint16)~0, "!");

//	if (_pRenderParams->m_CurrentTLEnable && pBSPVBI->m_VBID != (uint16)~0)

	{
		CXR_VBIDChain *pChain = _pRenderParams->m_pVBM->Alloc_VBIDChain();
		if(!pChain)
			return;

		pChain->m_VBID = pBSPVBI->m_VBID;
		_pVB->AddChain(pChain);
	}
/*	else
	{
		CBSP2_VertexBuffer* pBSPVB = GetVertexBuffer(_iVB);

		CXR_VBChain *pChain = _pRenderParams->m_pVBM->Alloc_VBChain();
		if(!pChain)
			return;

		_pVB->AddChain(pChain);

		pChain->m_pV = pBSPVB->m_lV.GetBasePtr();
		pChain->m_nV = pBSPVB->m_lV.Len();
		pChain->m_pN = pBSPVB->m_lN.GetBasePtr();
		pChain->m_pCol = pBSPVB->m_lCol.GetBasePtr();
		pChain->m_pTV[0] = (fp32*) pBSPVB->m_lTV1.GetBasePtr();
		pChain->m_pTV[1] = (fp32*) pBSPVB->m_lTangU.GetBasePtr();
		pChain->m_pTV[2] = (fp32*) pBSPVB->m_lTangV.GetBasePtr();
		pChain->m_pTV[3] = (fp32*) pBSPVB->m_lTV2.GetBasePtr();
		pChain->m_pTV[4] = (fp32*) pBSPVB->m_lLMScale.GetBasePtr();
		pChain->m_nTVComp[0] = 2;
		if(pChain->m_pTV[1]) pChain->m_nTVComp[1] = 3;
		if(pChain->m_pTV[2]) pChain->m_nTVComp[2] = 3;
		if(pChain->m_pTV[3]) pChain->m_nTVComp[3] = 2;
		if(pChain->m_pTV[4]) pChain->m_nTVComp[4] = 1;
	}*/
}

void CXR_Model_BSP2::VB_RenderFaces(CBSP2_RenderParams* _pRenderParams, int _iVB, const uint32* _piFaces, int _nFaces, CXW_Surface* _pSurf, bool _bAddLight)
{
	// Render vertex-buffer chain into z-buffer, black
	{
		uint32 Ambience = _pRenderParams->m_UnifiedAmbience;
		CXR_VertexBuffer* pVB = _pRenderParams->m_pVBM->Alloc_VB();
		if (!pVB) return;

		CBSP2_VBChain Chain;
		VB_CopyGeometry(_pRenderParams, _iVB, &Chain);

		Chain.SetToVB(pVB);

		if (!RenderTesselateVBPrim(_piFaces, _nFaces, _pRenderParams->m_pVBM, pVB))
			return;
		pVB->Geometry_Color(Ambience);
		pVB->Matrix_Set(_pRenderParams->m_pVBMatrixM2V);
		pVB->m_pAttrib = &m_RenderZBuffer;

		pVB->m_Priority = CXR_VBPRIORITY_UNIFIED_ZBUFFER;

		_pRenderParams->m_pVBM->AddVB(pVB);
	}
}

void CXR_Model_BSP2::VB_ClearFaceQueues(CBSP2_RenderParams* _pRenderParams)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_ClearFaceQueues, MAUTOSTRIP_VOID);
	CBSP2_VBFaceQueue* pQ = _pRenderParams->m_pVBFaceQueue;
	uint nQ = m_lVBFaceQueues.Len();
	for(uint i = 0; i < nQ; i++)
	{
		pQ[i].m_nElem = 0;
		pQ[i].m_DynamicLightMask = 0;
	}
}

/*void CXR_Model_BSP2::VB_RenderFaces(int _iVB, const uint32* _piFaces, int _nFaces, CXW_Surface* _pSurf)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_RenderFaces_2, NULL);
	uint32 liFaces[2048];
	uint32 liLFaces[2048];
	int nFaces = 0;
	int nLFaces = 0;

	for(int i = 0; i < _nFaces; i++)
	{
		int iFace = _piFaces[i];
		if (m_lFaceLightMask[iFace])
			liLFaces[nLFaces++] = iFace;
		else
			liFaces[nFaces++] = iFace;
	}

	VB_Tesselate(_iVB, liFaces, nFaces);
//	VB_RenderFaces(_iVB, liFaces, nFaces, _pSurf, Box, -1, false);
	VB_RenderFaces(_iVB, liLFaces, nLFaces, _pSurf, true);
}
*/

void CXR_Model_BSP2::VB_RenderFaceQueues(CBSP2_RenderParams* _pRenderParams)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_RenderFaceQueues, MAUTOSTRIP_VOID);

	TMeasureProfile(m_TimeBrushVB);

/*	const int MaxFaces = 2048;
	uint16 liLitFaces[MaxFaces];
	uint16 liUnlitFaces[MaxFaces];
	uint16 liLitFaces2[MaxFaces];
*/
	{
		CBSP2_VBFaceQueue* pQ = _pRenderParams->m_pVBFaceQueue;
		uint nQ = m_lVBFaceQueues.Len();
		for(uint iVB = 0; iVB < nQ; iVB++)
		{
			const uint32* pFQ = pQ[iVB].m_pQueue;
			int nFQ = pQ[iVB].m_nElem;
			if (!nFQ)
				continue;

			pQ[iVB].m_nElem = 0;

			VB_Tesselate(_pRenderParams, iVB, pFQ, nFQ);

/*			while(nFQ)
			{
				int nLit = 0;
				int nUnlit = 0;
				int TotalMask = 0;
				int nElem = MinMT(nFQ, MaxFaces);
				for(int j = 0; j < nElem; j++)
				{
					int iFace = pFQ[j];
					int Mask = m_lFaceLightMask[iFace];
					if (Mask)
					{
						liLitFaces[nLit++] = iFace;
						TotalMask |= Mask;
					}
					else
					{
						liUnlitFaces[nUnlit++] = iFace;
					}
				}

				VB_Tesselate(iVB, liUnlitFaces, nUnlit);
				VB_RenderFaces(iVB, liLitFaces, nLit, m_lspSurfaces[m_lVBInfo[iVB].m_iSurface]->GetSurface(ms_SurfOptions, ms_SurfCaps), true);

				if (nLit)
				{
					for(int iDynamic = 0; iDynamic < m_RenderInfo.m_nLights; iDynamic++)
					{				
						int CurMask = (1 << iDynamic);
						if (TotalMask & CurMask)
						{
							CXR_Light* pL = &m_lRenderLight[iDynamic];

							int nLit2 = 0;
							for(int j = 0; j < nLit; j++)
							{
								int iFace = liLitFaces[j];
								if (CurMask & m_lFaceLightMask[iFace])
									liLitFaces2[nLit2++] = iFace;
							}

							if (nLit2)
							{
								CXR_VertexBuffer* pVB = m_pVBM->Alloc_VB();
								if (!pVB) continue;
								pVB->Matrix_Set(m_pVBMatrixM2V);

								VB_CopyGeometry(iVB, pVB);
								if (RenderTesselateVBPrim(liLitFaces2, nLit2, m_pVBM, pVB))
								{
									CBSP2_VBInfo* pBSPVBI = &m_lVBInfo[iVB];
									Light_RenderShading(pVB, pL, m_lspSurfaces[pBSPVBI->m_iSurface]);
								}
							}
						}
					}
				}

				nFQ -= nElem;
				pFQ += nElem;
			}*/
		}
	}

}


//#define MODEL_BSP_CULLPORTALPLANES
#define MODEL_BSP_BRUTEFORCE

class CXR_VirtualAttributes_ShaderDefBSP2 : public CXR_VirtualAttributes
{

public:

	uint16 m_TextureID;
	int m_bIsMirrored;
	CRC_ExtAttributes * m_pExtAttrib;

	void Create()
	{
		m_Class = 0x22; // Seeing as the other VAttribs in this file are 0x20 and 0x21
		m_pExtAttrib = NULL;
	}

	int OnCompare(const CXR_VirtualAttributes * _pOther)
	{
		const CXR_VirtualAttributes_ShaderDefBSP2 * pLast = (const CXR_VirtualAttributes_ShaderDefBSP2*)_pOther;
		XR_COMPAREATTRIBUTE_INT(m_TextureID,pLast->m_TextureID);
		return (m_bIsMirrored != pLast->m_bIsMirrored);
	}

	uint OnSetAttributes(CRC_Attributes* _pDest, const CXR_VirtualAttributes* _pLastAttr)
	{
		int AttrChg = 0;

		if (!_pLastAttr || _pLastAttr->m_Class != m_Class)
		{
			AttrChg = -1;
			_pDest->SetDefault();
			_pDest->Attrib_TextureID(0, m_TextureID);
			_pDest->Attrib_Disable(CRC_FLAGS_ZWRITE |CRC_FLAGS_STENCIL);
			_pDest->Attrib_ZCompare(CRC_COMPARE_EQUAL);
			_pDest->m_pExtAttrib = m_pExtAttrib;

			if( !m_bIsMirrored ) _pDest->Attrib_Enable(CRC_FLAGS_CULL);
		}
		else
		{
			const CXR_VirtualAttributes_ShaderDefBSP2 * pLast = (const CXR_VirtualAttributes_ShaderDefBSP2*)_pLastAttr;
			
			if( m_TextureID != pLast->m_TextureID )
			{
				_pDest->Attrib_TextureID(0, m_TextureID);
				AttrChg |= CRC_ATTRCHG_TEXTUREID;
			}

			if( m_pExtAttrib != pLast->m_pExtAttrib )
			{
				_pDest->m_pExtAttrib = m_pExtAttrib;
				AttrChg |= CRC_ATTRCHG_EXATTR;
			}

			if( m_bIsMirrored != pLast->m_bIsMirrored )
			{
				AttrChg |= CRC_ATTRCHG_FLAGS;
				if( m_bIsMirrored )
					_pDest->Attrib_Disable(CRC_FLAGS_CULL);
				else
					_pDest->Attrib_Enable(CRC_FLAGS_CULL);
			}
		}

		return AttrChg;
	}
};

void CXR_Model_BSP2::VB_ClearQueues(CBSP2_RenderParams* _pRenderParams)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_ClearQueues, MAUTOSTRIP_VOID);
	uint nQ = m_lSurfQueues.Len();
	CBSP2_SurfaceQueueData* pQueues = _pRenderParams->m_pSurfQueues;
	for(int i = 0; i < nQ; i++)
	{
		pQueues[i].Clear();
	}
}

int ConvertWLSToCRCLight(CXR_WorldLightState& _WLS, CXR_VBManager& _VBM, CRC_Light*& _pLights, fp32 _LightScale);

static CXR_VBIDChain* AppendVBIDChainCopy(CXR_VBIDChain* _pDestChain, CXR_VBIDChain* _pCopySrc, CXR_VBManager* _pVBM)
{
	uint nSrc = 1;
	CXR_VBIDChain* pChainCount = _pCopySrc;
	for(; pChainCount->m_pNextVB; pChainCount = pChainCount->m_pNextVB, nSrc++) {};
	
	CXR_VBIDChain* pNewChain = (CXR_VBIDChain*)_pVBM->Alloc(sizeof(CXR_VBIDChain) * nSrc);
	if (!pNewChain)
		return NULL;

	int i = 0;
	for(; i < nSrc-1; i++)
	{
		pNewChain[i].CopyData(*_pCopySrc);
		pNewChain[i].m_pNextVB = &pNewChain[i+1];
		_pCopySrc=_pCopySrc->m_pNextVB;
	}
	pNewChain[i].CopyData(*_pCopySrc);
	pNewChain[i].m_pNextVB = _pDestChain;
	return pNewChain;
}

static CXR_VBIDChain* AppendVBIDChainArrayCopy(CXR_VBIDChain* _pDestChain, CXR_VBIDChain** _lpCopySrc, uint _nChains, CXR_VBManager* _pVBM)
{
	// Chains in _lpCopySrc may be null

	uint nSrc = 0;
	uint iChain;
	for(iChain = 0; iChain < _nChains; iChain++)
	{
		CXR_VBIDChain* pChainCount = _lpCopySrc[iChain];
		if (pChainCount)
		{
			nSrc++;
			while(pChainCount->m_pNextVB)
			{
				pChainCount = pChainCount->m_pNextVB;
				nSrc++;
			}
		}
	}
	if (!nSrc)
		return _pDestChain;

	CXR_VBIDChain*M_RESTRICT pNewChain = (CXR_VBIDChain*)_pVBM->Alloc(sizeof(CXR_VBIDChain) * nSrc, true);
	if (!pNewChain)
		return NULL;

	int i = 0;
	for(iChain = 0; iChain < _nChains; iChain++)
	{
		CXR_VBIDChain* pCopySrc = _lpCopySrc[iChain];
		while(pCopySrc)
		{
			pNewChain[i].CopyData(*pCopySrc);
			pNewChain[i].m_pNextVB = &pNewChain[i+1];
			pCopySrc = pCopySrc->m_pNextVB;
			i++;
		}
	}
	pNewChain[i-1].m_pNextVB = _pDestChain;
	return pNewChain;
}

void CXR_Model_BSP2::VB_RenderQueues(CBSP2_RenderParams* _pRenderParams)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_RenderQueues, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP2::VB_RenderQueues); //AR-SCOPE

	TMeasureProfile(m_TimeSurf);

	M_PRECACHE128(0, _pRenderParams->m_pSurfQueues);

	CXR_VBManager* pVBM = _pRenderParams->m_pVBM;

	int bIsMirrored = _pRenderParams->m_pCurrentEngine->GetVC()->m_bIsMirrored;
	CRC_Attributes* pZAttr = (bIsMirrored) ? &m_RenderZBufferCullCW : &m_RenderZBuffer;

	CXW_SurfaceKeyFrame* lpTmpAnimStateSurfKeyFrame[ANIMSTATE_NUMSURF];
	CMTime lTmpAnimTime[ANIMSTATE_NUMSURF];

	CXR_RenderSurfExtParam Params;
	Params.m_lUserColors[0] = _pRenderParams->m_pCurAnimState->m_Data[0];
	Params.m_lUserColors[1] = _pRenderParams->m_pCurAnimState->m_Data[1];

	CXR_Shader* pShader = _pRenderParams->m_pCurrentEngine->GetShader();

	// Create z-fog attribute
	CRC_Attributes* pAFog = NULL;
	CPixel32 FogColor(0xffffffff);
	if (_pRenderParams->m_pSceneFog && _pRenderParams->m_pSceneFog->DepthFogEnable())
	{
		pAFog = pVBM->Alloc_Attrib();
		if (!pAFog)
			return;

		const fp32 FogEnd = _pRenderParams->m_pSceneFog->m_DepthFogEnd;
		const fp32 FogStart = _pRenderParams->m_pSceneFog->m_DepthFogStart;

		CVec4Dfp32* pTexGenAttr = pVBM->Alloc_V4(2);
		if (!pTexGenAttr)
			return;
		const CVec3Dfp32& VFwd = CVec3Dfp32::GetRow(_pRenderParams->m_pViewData->m_CurVMatInv, 2);
		fp32 k = 1.0f / (FogEnd - FogStart);
		pTexGenAttr[0][0] = VFwd[0] * k;
		pTexGenAttr[0][1] = VFwd[1] * k;
		pTexGenAttr[0][2] = VFwd[2] * k;
		pTexGenAttr[0][3] = -(VFwd * _pRenderParams->m_pViewData->m_CurLocalVP)*k - (k * FogStart);
		pTexGenAttr[1] = 0;

		pAFog->SetDefault();
		pAFog->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_STENCIL);
		pAFog->Attrib_Enable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_CULL);
		pAFog->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		pAFog->Attrib_TextureID(0, _pRenderParams->m_pSceneFog->m_DepthFogTableTextureID);
		pAFog->Attrib_TexGen(0, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);
		pAFog->Attrib_TexGenAttr((fp32*)pTexGenAttr);
		pAFog->Attrib_ZCompare(CRC_COMPARE_EQUAL);
#ifdef XR_DEFAULTPOLYOFFSETSCALE
		pAFog->Attrib_PolygonOffset(XR_DEFAULTPOLYOFFSETSCALE, XR_DEFAULTPOLYOFFSET);
#endif

		FogColor = _pRenderParams->m_pSceneFog->m_DepthFogColor | 0xff000000;
	}

	for(int iSurf = 0; iSurf < ANIMSTATE_NUMSURF; iSurf++)
	{
		CXW_Surface* pSurf = _pRenderParams->m_pCurAnimState->m_lpSurfaces[iSurf];
		if (pSurf)
		{
			pSurf = pSurf->GetSurface(_pRenderParams->m_pCurrentEngine->m_SurfOptions, _pRenderParams->m_pCurrentEngine->m_SurfCaps);
			CMTime AnimTime = pSurf->m_iController ? _pRenderParams->m_pCurAnimState->m_AnimTime1 : _pRenderParams->m_pCurAnimState->m_AnimTime0;
			lTmpAnimTime[iSurf] = AnimTime;
			lpTmpAnimStateSurfKeyFrame[iSurf] = pSurf->GetFrame(
				pSurf->m_iController ? _pRenderParams->m_pCurAnimState->m_Anim1 : _pRenderParams->m_pCurAnimState->m_Anim0, 
				AnimTime, _pRenderParams->CreateTempSurfKeyFrame(iSurf));
		}
	}

	CVec4Dfp32 M_ALIGN(16) lLFAxes[6];

	if (_pRenderParams->m_RenderInfo.m_pLightVolume)
	{
		CVec3Dfp32 BoundV[8];
		m_BoundBox.GetVertices(BoundV);
		for(int v = 0; v < 8; v++)
			BoundV[v] *= _pRenderParams->m_pViewData->m_CurWMat;
		_pRenderParams->m_RenderInfo.m_pLightVolume->Light_EvalPointArrayMax(BoundV, 8, lLFAxes);

//		_pRenderParams->m_RenderInfo.m_pLightVolume->Light_EvalPoint(CVec3Dfp32::GetRow(_pRenderParams->m_pViewData->m_CurWMat, 3), lLFAxes);
	}


	uint32 Ambience = _pRenderParams->m_UnifiedAmbience;

	int bDeferredShading = pShader->m_ShaderModeTraits & XR_SHADERMODETRAIT_DEFERRED;


	CXR_VBIDChain* pVBIDChainZbuffer = NULL;
	CXR_VBIDChain* pVBIDChainFog = NULL;

	TAP<CXR_SurfaceShaderParams> lSurfaceShaderParams = m_lSurfaceShaderParams;
	M_ASSERT(lSurfaceShaderParams.Len() == m_lspSurfaces.Len(), "Internal error");

	CBSP2_SurfaceQueueData* pQueues = _pRenderParams->m_pSurfQueues;
	uint nQ = m_lSurfQueues.Len();

	const uint MaxQueueBatch = 32;

	uint iQueue = 0;
	while(iQueue < nQ)
	{
//		uint nInner = Min(MaxQueueBatch, nQ - iQueue);
		M_PRECACHEMEM(&pQueues[iQueue], sizeof(CBSP2_SurfaceQueueData) * MaxQueueBatch * 2);

		// Build 2 queues of batches, one for shading and one for render surface.
		uint liShadingQueue[MaxQueueBatch];
		uint liRSQueue[MaxQueueBatch];
		CXW_Surface* lpRSSurf[MaxQueueBatch];
		uint nRSQueue = 0;
		uint nShadingQueue = 0;

		uint iQueueRestart = iQueue;
		uint iInner;
		for(; (iQueue < nQ) && (nRSQueue < MaxQueueBatch) && (nShadingQueue < MaxQueueBatch); iQueue++)
		{
			CBSP2_SurfaceQueueData*M_RESTRICT pQueue = &pQueues[iQueue];
			CBSP2_VBChain *pVBChain = &pQueue->m_VBChain;
			if (!pVBChain->m_pChain) 
				continue;
			uint32 SurfaceFlags = pQueue->m_SurfaceFlags;
			if (SurfaceFlags & XW_SURFFLAGS_INVISIBLE) 
				continue;
			if (SurfaceFlags & XW_SURFFLAGS_SHADER)
			{
				liShadingQueue[nShadingQueue] = iQueue;
				nShadingQueue++;
			}
			if (!(SurfaceFlags & XW_SURFFLAGS_SHADERONLY))
			{
				CXW_Surface* pSurf = m_lspSurfaces[pQueue->m_iSurface];
				M_PRECACHE128(0, pSurf);

				lpRSSurf[nRSQueue] = pSurf;
				liRSQueue[nRSQueue] = iQueue;
				nRSQueue++;
			}
		}

		// Process render surface queue
		{
			for(iInner = 0; iInner < nRSQueue; iInner++)
			{
				uint iQueue = liRSQueue[iInner];
				CBSP2_SurfaceQueueData*M_RESTRICT pQueue = &pQueues[iQueue];
				CXW_Surface* pSurf = lpRSSurf[iInner];
				if (pSurf->m_pFastPathVA)
					M_PRECACHE128(0, pSurf->m_pFastPathVA);
				else
					M_PRECACHE128(0, pSurf->GetSequence(0));
			}

			for(iInner = 0; iInner < nRSQueue; iInner++)
			{
				uint iQueue = liRSQueue[iInner];
				CBSP2_SurfaceQueueData*M_RESTRICT pQueue = &pQueues[iQueue];
				uint32 SurfaceFlags = pQueue->m_SurfaceFlags;
				CXW_Surface* pSurf = lpRSSurf[iInner];
				if (!(SurfaceFlags & XW_SURFFLAGS_NOVERSIONS))
				{
					pSurf = pSurf->GetSurface(_pRenderParams->m_pCurrentEngine->m_SurfOptions, _pRenderParams->m_pCurrentEngine->m_SurfCaps);
					lpRSSurf[iInner] = pSurf;
					if (!pSurf->m_pFastPathVA)
						M_PRECACHE128(0, pSurf->GetBaseFrame());
				}
				else
				{
					if (!pSurf->m_pFastPathVA)
						M_PRECACHE128(0, pSurf->GetBaseFrame());
				}
			}

			for(iInner = 0; iInner < nRSQueue; iInner++)
			{
				uint iQueue = liRSQueue[iInner];
				CBSP2_SurfaceQueueData*M_RESTRICT pQueue = &pQueues[iQueue];
				CXW_Surface* pSurf = lpRSSurf[iInner];
				if (!pSurf->m_pFastPathVA)
					M_PRECACHE128(0, pSurf->GetBaseFrame()->m_lTextures.GetBasePtr());
			}

			// US = User surface
			uint liUSQueue[MaxQueueBatch];
			uint8 liUSSurfGroup[MaxQueueBatch];
			CXW_Surface* lpUSSurf[MaxQueueBatch];
			uint nUSQueue = 0;

			fp32 BasePriority = _pRenderParams->m_BasePriority_Opaque;
			fp32 Offset = 0.0001f;

			uint nRSQueueCurrent = nRSQueue;
			nRSQueue = 0;
			for(iInner = 0; iInner < nRSQueueCurrent; iInner++)
			{
				uint iQueue = liRSQueue[iInner];
				CBSP2_SurfaceQueueData*M_RESTRICT pQueue = &pQueues[iQueue];

				CXW_Surface* pSurf = lpRSSurf[iInner];
				uint8 iSurfaceGroup = pSurf->m_iGroup;
				CXW_Surface* pUserSurface = _pRenderParams->m_pCurAnimState->m_lpSurfaces[iSurfaceGroup];
				uint8 Mode = (_pRenderParams->m_CurOnRenderFlags >> (CXR_MODEL_ONRENDERFLAGS_SURFMODE_BASE + iSurfaceGroup)) & 1;
				if (!pUserSurface || Mode == 1)
				{
					// Render vertex-buffer chain with surface
					uint32 SurfaceFlags = pQueue->m_SurfaceFlags;
					int Flags = RENDERSURFACE_NOSHADERLAYERS | RENDERSURFACE_MATRIXSTATIC_M2V;
					if (!(SurfaceFlags & XW_SURFFLAGS_SHADER) && !(SurfaceFlags & XW_SURFFLAGS_NOFOG))
						Flags |= RENDERSURFACE_DEPTHFOG;
					if ((SurfaceFlags & XW_SURFFLAGS_TRANSPARENT) || (Flags & RENDERSURFACE_DEPTHFOG) || (pSurf->m_iController != 0))
					{
						fp32 BasePriority = _pRenderParams->m_BasePriority_Opaque;
						fp32 Offset = 0.0001f;
						if (pSurf->m_Flags & XW_SURFFLAGS_TRANSPARENT)
							BasePriority = _pRenderParams->m_BasePriority_Transparent;
						BasePriority += pSurf->m_PriorityOffset;

						CMTime AnimTime = pSurf->m_iController ? _pRenderParams->m_pCurAnimState->m_AnimTime1 : _pRenderParams->m_pCurAnimState->m_AnimTime0;
						CXR_VertexBufferGeometry VB;
						CBSP2_VBChain *pVBChain = &pQueue->m_VBChain;
						VB.Create(pVBChain->m_pChain, NULL);
						CXR_Util::Render_SurfaceArray(Flags, 1, &VB, &lpRSSurf[iInner], AnimTime, _pRenderParams->m_pCurrentEngine, pVBM, _pRenderParams->m_pVBMatrixM2W, _pRenderParams->m_pVBMatrixW2V, _pRenderParams->m_pVBMatrixM2V, 
							&BasePriority, Offset, &Params);
					}
					else
					{
						liRSQueue[nRSQueue] = iQueue;
						lpRSSurf[nRSQueue] = pSurf;
						nRSQueue++;
					}
				}
				if (pUserSurface)
				{
					liUSQueue[nUSQueue] = iQueue;
					lpUSSurf[nUSQueue] = pUserSurface;
					liUSSurfGroup[nUSQueue] = iSurfaceGroup;
					nUSQueue++;
				}
			}

/*			uint nRSQueueCurrent = nRSQueue;
			nRSQueue = 0;
			for(iInner = 0; iInner < nRSQueue; iInner++)
			{
				uint iQueue = liRSQueue[iInner];
				CBSP2_SurfaceQueueData*M_RESTRICT pQueue = &pQueues[iQueue];
				CBSP2_VBChain *pVBChain = &pQueue->m_VBChain;
				uint32 SurfaceFlags = pQueue->m_SurfaceFlags;
				CXW_Surface* pSurf = lpRSSurf[iInner];

				// Render vertex-buffer chain with surface
				int Flags = RENDERSURFACE_NOSHADERLAYERS | RENDERSURFACE_MATRIXSTATIC_M2V;
				if (!(SurfaceFlags & XW_SURFFLAGS_SHADER) && !(SurfaceFlags & XW_SURFFLAGS_NOFOG))
					Flags |= RENDERSURFACE_DEPTHFOG;
				if ((SurfaceFlags & XW_SURFFLAGS_TRANSPARENT) || (Flags & RENDERSURFACE_DEPTHFOG) || (pSurf->m_iController != 0))
				{
					fp32 BasePriority = _pRenderParams->m_BasePriority_Opaque;
					fp32 Offset = 0.0001f;
					if (pSurf->m_Flags & XW_SURFFLAGS_TRANSPARENT)
						BasePriority = _pRenderParams->m_BasePriority_Transparent;

					CMTime AnimTime = pSurf->m_iController ? _pRenderParams->m_pCurAnimState->m_AnimTime1 : _pRenderParams->m_pCurAnimState->m_AnimTime0;
					CXR_VertexBufferGeometry VB;
					VB.Create(pVBChain->m_pChain, NULL);
					CXR_Util::Render_SurfaceArray(Flags, 1, &VB, &lpRSSurf[iInner], AnimTime, _pRenderParams->m_pCurrentEngine, pVBM, _pRenderParams->m_pVBMatrixM2W, _pRenderParams->m_pVBMatrixW2V, _pRenderParams->m_pVBMatrixM2V, 
						BasePriority + pSurf->m_PriorityOffset, Offset, &Params);
				}
				else
				{
					liRSQueue[nRSQueue] = iQueue;
					lpRSSurf[nRSQueue] = pSurf;
					nRSQueue++;
				}
			}*/

			if (nRSQueue)
			{
				CXR_VertexBufferGeometry lVB[MaxQueueBatch];
				fp32 lVBPriority[MaxQueueBatch];

				fp32 BasePriority = _pRenderParams->m_BasePriority_Opaque;
				fp32 Offset = 0.0001f;
				for(iInner = 0; iInner < nRSQueue; iInner++)
				{
					uint iQueue = liRSQueue[iInner];
					CBSP2_SurfaceQueueData*M_RESTRICT pQueue = &pQueues[iQueue];
					CBSP2_VBChain *pVBChain = &pQueue->m_VBChain;
					CXW_Surface* pSurf = lpRSSurf[iInner];
					lVB[iInner].Create(pVBChain->m_pChain, NULL);
					lVBPriority[iInner] = BasePriority + pSurf->m_PriorityOffset;
				}

				int Flags = RENDERSURFACE_NOSHADERLAYERS |  RENDERSURFACE_MATRIXSTATIC_M2V;
				CXR_Util::Render_SurfaceArray(Flags, nRSQueue, lVB, lpRSSurf, _pRenderParams->m_pCurAnimState->m_AnimTime0, _pRenderParams->m_pCurrentEngine, pVBM, 
					_pRenderParams->m_pVBMatrixM2W, _pRenderParams->m_pVBMatrixW2V, _pRenderParams->m_pVBMatrixM2V, 
					lVBPriority, Offset, &Params);
			}

			for(iInner = 0; iInner < nUSQueue; iInner++)
			{
				uint iQueue = liUSQueue[iInner];
				CBSP2_SurfaceQueueData*M_RESTRICT pQueue = &pQueues[iQueue];
				CBSP2_VBChain *pVBChain = &pQueue->m_VBChain;
				uint iSurfaceGroup = liUSSurfGroup[iInner];
				CXW_Surface* pUserSurface = lpUSSurf[iInner];

				uint32 SurfaceFlags = pQueue->m_SurfaceFlags;
				uint32 bDrawShading = SurfaceFlags & XW_SURFFLAGS_SHADER;
				int Flags = RENDERSURFACE_NOSHADERLAYERS |  RENDERSURFACE_MATRIXSTATIC_M2V;
				if (!bDrawShading)
					Flags |= RENDERSURFACE_DEPTHFOG;

				{
					fp32 BasePriority = _pRenderParams->m_BasePriority_Opaque;
					fp32 Offset = 1.0f;
					if (pUserSurface->m_Flags & XW_SURFFLAGS_TRANSPARENT)
					{
						BasePriority = _pRenderParams->m_BasePriority_Transparent;
						Offset = 0.0001f;
					}
					BasePriority += pUserSurface->m_PriorityOffset;
					CXR_VertexBufferGeometry VB;
					VB.Create(pVBChain->m_pChain, NULL);
					CXR_Util::Render_Surface(Flags, lTmpAnimTime[iSurfaceGroup], pUserSurface, lpTmpAnimStateSurfKeyFrame[iSurfaceGroup], _pRenderParams->m_pCurrentEngine, pVBM, _pRenderParams->m_pVBMatrixM2W, _pRenderParams->m_pVBMatrixW2V, _pRenderParams->m_pVBMatrixM2V, &VB, 
						BasePriority + pUserSurface->m_PriorityOffset, Offset, &Params);
				}
			}
		}

		// Process shading queue
		if (nShadingQueue)
		{
			for(iInner = 0; iInner < nShadingQueue; iInner++)
			{
				uint iQueue = liShadingQueue[iInner];
				CBSP2_SurfaceQueueData*M_RESTRICT pQueue = &pQueues[iQueue];
				const CXR_SurfaceShaderParams* pSSP = &lSurfaceShaderParams[pQueue->m_iSurface];
				M_PRECACHE128(0, pSSP);
			}

			// Prepare geometry and surface params
			const CXR_SurfaceShaderParams* lpSSP[MaxQueueBatch];
			CXR_VertexBufferGeometry lVB[MaxQueueBatch];
			CXR_VBIDChain* lpChains[MaxQueueBatch];

			for(iInner = 0; iInner < nShadingQueue; iInner++)
			{
				uint iQueue = liShadingQueue[iInner];
				CBSP2_SurfaceQueueData*M_RESTRICT pQueue = &pQueues[iQueue];
				CBSP2_VBChain *pVBChain = &pQueue->m_VBChain;
				const CXR_SurfaceShaderParams* pSSP = &lSurfaceShaderParams[pQueue->m_iSurface];

				lpChains[iInner] = pVBChain->m_pChain;
				lVB[iInner].Create(pVBChain->m_pChain, _pRenderParams->m_pVBMatrixM2V);
				lpSSP[iInner] = pSSP;
				M_PRECACHE128(0, pVBChain->m_pChain);
			}

			if (bDeferredShading)
			{
				CXR_ShaderParams Params;
				Params.Create(_pRenderParams->m_pVBMatrixM2W, _pRenderParams->m_pVBMatrixW2V, pShader);
				Params.m_nVB = nShadingQueue;
				Params.m_pCurrentWVelMat = _pRenderParams->m_pWVelMat;

				pShader->RenderDeferredArray(nShadingQueue, lVB, &Params, lpSSP, (!bIsMirrored) ? CRC_FLAGS_CULL : 0, CRC_FLAGS_ZWRITE | CRC_FLAGS_STENCIL, CXR_VBPRIORITY_UNIFIED_ZBUFFER + 8.0f, 1.0f);
			}

			CXR_VBIDChain* lpZChainsNoAlpha[MaxQueueBatch];
			uint nZChainsNoAlpha = 0;

			uint liZAlphaQueue[MaxQueueBatch];
			uint nZAlphaQueue = 0;

			for(iInner = 0; iInner < nShadingQueue; iInner++)
			{
				uint iQueue = liShadingQueue[iInner];
				CBSP2_SurfaceQueueData*M_RESTRICT pQueue = &pQueues[iQueue];
				CBSP2_VBChain *pVBChain = &pQueue->m_VBChain;

				uint32 SurfaceFlags = pQueue->m_SurfaceFlags;
				const CXR_SurfaceShaderParams* pSSP = &lSurfaceShaderParams[pQueue->m_iSurface];

				// Z-prepass and fog
				{
					// Render vertex-buffer chain into z-buffer, no color/alpha write
					if (pSSP->m_Flags & XR_SHADERFLAGS_USEZEQUAL)
					{
						liZAlphaQueue[nZAlphaQueue++] = iQueue;
					}
					else
					{
						lpZChainsNoAlpha[nZChainsNoAlpha] = pVBChain->m_pChain;
						nZChainsNoAlpha++;
					}
//						pVBIDChainZbuffer = AppendVBIDChainCopy(pVBIDChainZbuffer, pVBChain->m_pChain, pVBM);

				}
			}

			if (nZAlphaQueue)
			{
				uint VBMem = nZAlphaQueue * (sizeof(CXR_VertexBuffer) + sizeof(CRC_Attributes));
				CRC_Attributes*M_RESTRICT pZAlphaAttr = (CRC_Attributes*) pVBM->Alloc(VBMem, true);
				if (!pZAlphaAttr)
					return;
				CXR_VertexBuffer*M_RESTRICT pZAlphaVBs = (CXR_VertexBuffer*) (pZAlphaAttr + nZAlphaQueue);

				CXR_VertexBuffer* lpVB[MaxQueueBatch];
				for(iInner = 0; iInner < nZAlphaQueue; iInner++)
				{
					pZAlphaVBs[iInner].Construct();
					lpVB[iInner] = pZAlphaVBs+iInner;
				}
				for(iInner = 0; iInner < nZAlphaQueue; iInner++)
					pZAlphaAttr[iInner] = *pZAttr;

				for(iInner = 0; iInner < nZAlphaQueue; iInner++)
				{
					uint iQueue = liZAlphaQueue[iInner];
					CBSP2_SurfaceQueueData*M_RESTRICT pQueue = &pQueues[iQueue];
					CBSP2_VBChain *pVBChain = &pQueue->m_VBChain;

					uint32 SurfaceFlags = pQueue->m_SurfaceFlags;
					const CXR_SurfaceShaderParams* pSSP = &lSurfaceShaderParams[pQueue->m_iSurface];

			//		CXW_Surface* pSurf = m_lspSurfaces[pQueue->m_iSurface];

					CXR_VertexBuffer*M_RESTRICT pVB = pZAlphaVBs+iInner;
					pVBChain->SetToVB(pVB);
					pVB->Geometry_Color(Ambience);

			//		CXW_SurfaceLayer* pLayers = pSurf->GetBaseFrame()->m_lTextures.GetBasePtr();
					CRC_Attributes*M_RESTRICT pA = pZAlphaAttr + iInner;
			//		pA->Attrib_TextureID(0, pLayers->m_TextureID);
			//		pA->Attrib_AlphaCompare(pLayers->m_AlphaFunc, pLayers->m_AlphaRef);
					pA->Attrib_TextureID(0, pSSP->m_lTextureIDs[XR_SHADERMAP_DIFFUSE]);
					pA->Attrib_AlphaCompare(pSSP->m_AlphaFunc, pSSP->m_AlphaRef);
					pVB->m_pAttrib = pA;

					pVB->Matrix_Set(_pRenderParams->m_pVBMatrixM2V);
					pVB->m_Priority = CXR_VBPRIORITY_UNIFIED_ZBUFFER;

					pVB->SetVBEColor(0xff403030);
				}

				pVBM->AddVBArray(lpVB, nZAlphaQueue);
			}

			pVBIDChainZbuffer = AppendVBIDChainArrayCopy(pVBIDChainZbuffer, lpZChainsNoAlpha, nZChainsNoAlpha, pVBM);

			// Render vertex-buffer as z-fog
			if (pAFog)
			{
				pVBIDChainFog = AppendVBIDChainArrayCopy(pVBIDChainFog, lpChains, nShadingQueue, pVBM);
			}

			if (_pRenderParams->m_RenderInfo.m_pLightVolume)
			{
				// Render lightfield shading
				for(iInner = 0; iInner < nShadingQueue; iInner++)
				{
					uint iQueue = liShadingQueue[iInner];
					CBSP2_SurfaceQueueData*M_RESTRICT pQueue = &pQueues[iQueue];
					CBSP2_VBChain *pVBChain = &pQueue->m_VBChain;

					uint32 SurfaceFlags = pQueue->m_SurfaceFlags;
					const CXR_SurfaceShaderParams* pSSP = &lSurfaceShaderParams[pQueue->m_iSurface];

					CXR_ShaderParams_LightField LFParams;
					LFParams.Create(_pRenderParams->m_pVBMatrixM2W, _pRenderParams->m_pVBMatrixW2V, pShader);
					memcpy(&LFParams.m_lLFAxes, &lLFAxes, sizeof(LFParams.m_lLFAxes));
					pShader->RenderShading_LightField(&lVB[iInner], &LFParams, &lpSSP[iInner]);
				}
			}
			else
			{
				iInner = 0;
				while(iInner < nShadingQueue)
				{
					uint nLFMBatch = 1;
					uint iLMTexture = pQueues[liShadingQueue[iInner]].m_iLMTexture;
					while((iInner + nLFMBatch < nShadingQueue) && (iLMTexture == pQueues[liShadingQueue[iInner + nLFMBatch]].m_iLMTexture))
						nLFMBatch++;

					if (iLMTexture != -1)
					{
						CXR_ShaderParams_LightFieldMapping LFMParams;
						LFMParams.CreateLFM(_pRenderParams->m_pVBMatrixM2W, _pRenderParams->m_pVBMatrixW2V, &m_lLMTextureIDs[iLMTexture*4], 3, 4);
						LFMParams.m_nVB = nLFMBatch;

						// Debug
						if (iLMTexture && (_pRenderParams->m_pCurrentEngine->m_DebugFlags & M_Bit(10)))
						{
							int White = 0;
							switch(iLMTexture)
							{
							case 1 : White = _pRenderParams->m_pCurrentEngine->m_pTC->GetTextureID("SPECIAL_FFFFFF"); break;
							case 2 : White = _pRenderParams->m_pCurrentEngine->m_pTC->GetTextureID("SPECIAL_0000FF"); break;
							case 3 : White = _pRenderParams->m_pCurrentEngine->m_pTC->GetTextureID("SPECIAL_00FF00"); break;
							case 4 : White = _pRenderParams->m_pCurrentEngine->m_pTC->GetTextureID("SPECIAL_FF0000"); break;
							case 5 : White = _pRenderParams->m_pCurrentEngine->m_pTC->GetTextureID("SPECIAL_FF8080"); break;
							}
							LFMParams.m_lLFMTextureID[0] = White;
							LFMParams.m_lLFMTextureID[1] = White;
							LFMParams.m_lLFMTextureID[2] = White;
							LFMParams.m_lLFMTextureID[3] = White;
						}

						if(!(_pRenderParams->m_pCurrentEngine->m_DebugFlags & M_Bit(20)))
							pShader->RenderShading_LightFieldMapping(&lVB[iInner], &LFMParams, &lpSSP[iInner]);
					}

					iInner += nLFMBatch;

					{
		/*				{
							int Empty = _pRenderParams->m_pCurrentEngine->m_pTC->GetTextureID("SPECIAL_00000000");
							LFMParams.m_lLFMTextureID[1] = Empty;
							LFMParams.m_lLFMTextureID[2] = Empty;
							LFMParams.m_lLFMTextureID[3] = Empty;
						}
						pShader->RenderShading_LightFieldMapping(&VB, &LFMParams);*/
					}
				}
			}
		}
	}

	if (pVBIDChainZbuffer)
	{
		CXR_VertexBuffer* pVB = pVBM->Alloc_VB();
		if (!pVB) 
			return;

		pVB->SetVBIDChain(pVBIDChainZbuffer);
		pVB->Geometry_Color(Ambience);
		pVB->m_pAttrib = pZAttr;

		pVB->Matrix_Set(_pRenderParams->m_pVBMatrixM2V);
		pVB->m_Priority = CXR_VBPRIORITY_UNIFIED_ZBUFFER;

		pVB->SetVBEColor(0xff403030);
		pVBM->AddVB(pVB);
	}

	if (pVBIDChainFog)
	{
		CXR_VertexBuffer* pVB = pVBM->Alloc_VB();
		if (!pVB) 
			return;

		pVB->SetVBIDChain(pVBIDChainFog);
		pVB->Geometry_Color(FogColor);

		pVB->m_pAttrib = pAFog;
		pVB->Matrix_Set(_pRenderParams->m_pVBMatrixM2V);
		pVB->m_Priority = _pRenderParams->m_BasePriority_Opaque + CXR_VBPRIORITY_DEPTHFOG;

		pVB->SetVBEColor(0xffe0e0ff);
		pVBM->AddVB(pVB);
	}
}

void CXR_Model_BSP2::VB_Flush(CBSP2_RenderParams* _pRenderParams)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_Flush, MAUTOSTRIP_VOID);

	TAP_RCD<CBSP2_VBPrimQueue> pVBPrimQueue = _pRenderParams->m_pVBPrimQueue;

	TAP_RCD<CBSP2_VBInfo> pVBInfo(m_lVBInfo);
	for(int iVB = 0; iVB < pVBInfo.Len(); iVB++)
	{
		CBSP2_VBPrimQueue* pPrimQueue = &pVBPrimQueue[iVB];
		if (!pPrimQueue->m_pPrim || !pPrimQueue->m_nPrim) continue;

		int iP = pPrimQueue->m_nPrim;
		M_ASSERT(iP > 1, "!?");
		M_ASSERT(pPrimQueue->m_pPrim[0] != 0x0100, "!?");


		pPrimQueue->m_pPrim[iP++] = CRC_RIP_END + (1 << 8);

		CBSP2_VBChain Chain;
		VB_CopyGeometry(_pRenderParams, iVB, &Chain);

//		if (Chain.m_bVBIds)
		{
//			CXR_VBIDChain *pChain = _pRenderParams->m_pVBM->Alloc_VBIDChain();
//			if(pChain)
			{
				CXR_VBIDChain* pChain = Chain.GetVBIDChain();
				if (!pChain)
					return;
//				*pChain = *(Chain.GetVBIDChain());
				pChain->m_piPrim = pPrimQueue->m_pPrim;
				pChain->m_nPrim = iP;
				pChain->m_PrimType = CRC_RIP_STREAM;
				_pRenderParams->m_pSurfQueues[pPrimQueue->m_iSurfQueue].AddChain(pChain);
			}
		}
/*		else
		{
			CXR_VBChain *pChain = _pRenderParams->m_pVBM->Alloc_VBChain();
			if(pChain)
			{
				*pChain = *(Chain.GetVBChain());
				pChain->m_piPrim = pBSPVBI->m_pPrim;
				pChain->m_nPrim = iP;
				pChain->m_PrimType = CRC_RIP_STREAM;
				_pRenderParams->m_pSurfQueues[pBSPVBI->m_iSurfQueue].AddChain(pChain);
			}
		}*/

		pPrimQueue->ClearPrimitives();
	}
}

#ifdef CPU_ALIGNED_MEMORY_ACCESS
static M_FORCEINLINE void WriteUnalignedPtr(void *_pMem, const void *_pPtr)
{
	MAUTOSTRIP(WriteUnalignedPtr, MAUTOSTRIP_VOID);
	memcpy(_pMem, &_pPtr, sizeof(_pPtr));
}

void CXR_Model_BSP2::VB_AddPrimitives(CBSP2_RenderParams* _pRenderParams, int _iVB, const uint16* _pPrim, int _nPrim)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_AddPrimitives, MAUTOSTRIP_VOID);
	CBSP2_VBPrimQueue* pVBI = &_pRenderParams->m_pVBPrimQueue[_iVB];

	M_ASSERT(_nPrim > 1, "?!");

	uint16* pPrim = pVBI->m_pPrim;
	if (!pPrim)
	{
		pPrim = _pRenderParams->m_pVBM->Alloc_Int16(pVBI->m_MaxPrim);
		if (!pPrim)
			return;
		pVBI->m_pPrim = pPrim;
	}

	int nVBIPrim = pVBI->m_nPrim;
	int SizeNeeded = (2 + sizeof(void *) / 2);
	if (nVBIPrim + SizeNeeded > pVBI->m_MaxPrim)
	{
		ConOut(CStrF("(CXR_Model_BSP2::VB_AddPrimitives) Insufficient primitive space. %d+%d > %d", nVBIPrim, SizeNeeded, pVBI->m_MaxPrim));
		return;
	}

	pPrim[nVBIPrim] = CRC_RIP_PRIMLIST | ((2 + (sizeof(void *) / 2)) << 8);
	WriteUnalignedPtr( (const uint16 **)&pPrim[nVBIPrim + 1], _pPrim );
	pPrim[nVBIPrim + 1 + (sizeof(void *) / 2)] = _nPrim;
	pVBI->m_nPrim += SizeNeeded;
}
#else
void CXR_Model_BSP2::VB_AddPrimitives(CBSP2_RenderParams* _pRenderParams, int _iVB, const uint16* _pPrim, int _nPrim)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_AddPrimitives_2, MAUTOSTRIP_VOID);
	CBSP2_VBPrimQueue* pVBI = &_pRenderParams->m_pVBPrimQueue[_iVB];

	M_ASSERT(_nPrim > 1, "?!");

	uint16* pPrim = pVBI->m_pPrim;
	if (!pPrim)
	{
		pPrim = _pRenderParams->m_pVBM->Alloc_Int16(pVBI->m_MaxPrim);
		if (!pPrim)
			return;
		pVBI->m_pPrim = pPrim;
	}

	int nVBIPrim = pVBI->m_nPrim;
	int SizeNeeded = (2 + sizeof(void *) / 2);
	if (nVBIPrim + SizeNeeded > pVBI->m_MaxPrim)
	{
		ConOut(CStrF("(CXR_Model_BSP2::VB_AddPrimitives) Insufficient primitive space. %d+%d > %d", nVBIPrim, SizeNeeded, pVBI->m_MaxPrim));
		return;
	}

	pPrim[nVBIPrim] = CRC_RIP_PRIMLIST | ((2 + (sizeof(void *) / 2)) << 8);
	*((const uint16 **)&pPrim[nVBIPrim + 1]) = _pPrim;
	pPrim[nVBIPrim + 1 + (sizeof(void *) / 2)] = _nPrim;
	pVBI->m_nPrim += SizeNeeded;
}
#endif	// CPU_ALIGNED_MEMORY_ACCESS

#ifdef CPU_ALIGNED_MEMORY_ACCESS

void CXR_Model_BSP2::VB_AddPrimitives_LocalVBM(CBSP2_RenderParams* _pRenderParams, int _iVB, const uint16* _pPrim, int _nPrim)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_AddPrimitives, MAUTOSTRIP_VOID);
	CBSP2_VBPrimQueue* pVBI = &_pRenderParams->m_pVBPrimQueue[_iVB];

	M_ASSERT(_nPrim > 1, "?!");

	uint16* pPrim = pVBI->m_pPrim;
	if (!pPrim)
	{
		pPrim = _pRenderParams->m_LocalVBM.Alloc_uint16(pVBI->m_MaxPrim);
		if (!pPrim)
			return;
		pVBI->m_pPrim = pPrim;
	}

	int nVBIPrim = pVBI->m_nPrim;
	int SizeNeeded = (2 + sizeof(void *) / 2);
	if (nVBIPrim + SizeNeeded > pVBI->m_MaxPrim)
	{
		ConOut(CStrF("(CXR_Model_BSP2::VB_AddPrimitives_LocalVBM) Insufficient primitive space. %d+%d > %d", nVBIPrim, SizeNeeded, pVBI->m_MaxPrim));
		return;
	}

	pPrim[nVBIPrim] = CRC_RIP_PRIMLIST | ((2 + (sizeof(void *) / 2)) << 8);
	WriteUnalignedPtr( (const uint16 **)&pPrim[nVBIPrim + 1], _pPrim );
	pPrim[nVBIPrim + 1 + (sizeof(void *) / 2)] = _nPrim;
	pVBI->m_nPrim += SizeNeeded;
}
#else
void CXR_Model_BSP2::VB_AddPrimitives_LocalVBM(CBSP2_RenderParams* _pRenderParams, int _iVB, const uint16* _pPrim, int _nPrim)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_AddPrimitives_2, MAUTOSTRIP_VOID);
	CBSP2_VBPrimQueue* pVBI = &_pRenderParams->m_pVBPrimQueue[_iVB];

	M_ASSERT(_nPrim > 1, "?!");

	uint16* pPrim = pVBI->m_pPrim;
	if (!pPrim)
	{
		pPrim = _pRenderParams->m_LocalVBM.Alloc_uint16(pVBI->m_MaxPrim);
		if (!pPrim)
			return;
		pVBI->m_pPrim = pPrim;
	}

	int nVBIPrim = pVBI->m_nPrim;
	int SizeNeeded = (2 + sizeof(void *) / 2);
	if (nVBIPrim + SizeNeeded > pVBI->m_MaxPrim)
	{
		ConOut(CStrF("(CXR_Model_BSP2::VB_AddPrimitives_LocalVBM) Insufficient primitive space. %d+%d > %d", nVBIPrim, SizeNeeded, pVBI->m_MaxPrim));
		return;
	}

	pPrim[nVBIPrim] = CRC_RIP_PRIMLIST | ((2 + (sizeof(void *) / 2)) << 8);
	*((const uint16 **)&pPrim[nVBIPrim + 1]) = _pPrim;
	pPrim[nVBIPrim + 1 + (sizeof(void *) / 2)] = _nPrim;
	pVBI->m_nPrim += SizeNeeded;
}
#endif	// CPU_ALIGNED_MEMORY_ACCESS

void CXR_Model_BSP2::VB_Tesselate(CBSP2_RenderParams* _pRenderParams, int _iVB, const uint32* _piFaces, int _nFaces)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_Tesselate, MAUTOSTRIP_VOID);
	if (!_nFaces)
		return;

	int nV, nP;
	RenderGetPrimCount( _piFaces, _nFaces, nV, nP);
	uint16* pPrim = _pRenderParams->m_pVBM->Alloc_Int16(nP);
	if (!pPrim)
		return;
	int nPrim = RenderTesselatePrim(_piFaces, _nFaces, pPrim);

	M_ASSERT(nPrim > 0, "!");
	M_ASSERT(nPrim == nP, "!");
	VB_AddPrimitives(_pRenderParams, _iVB, pPrim, nPrim);

/*	CBSP2_VBInfo* pVBI = &m_lVBInfo[_iVB];

	if (!pVBI->m_pPrim)
		if (!pVBI->AllocPrimitives(_pRenderParams->m_pVBM)) return;
	
	int nV, nP;
	RenderGetPrimCount( _piFaces, _nFaces, nV, nP);
	if (pVBI->m_nPrim+nP > pVBI->m_MaxPrim)
	{
		ConOut("(VB_Tesselate) Primitive heap overrun.");
		return;
	}

	int nPrim = RenderTesselatePrim(_piFaces, _nFaces, &pVBI->m_pPrim[pVBI->m_nPrim]);
	pVBI->m_nPrim += nPrim;

	if (pVBI->m_nPrim > pVBI->m_MaxPrim)
		Error("VB_Tesselate", "Primitive heap overrun.");*/
}

	// -------------------------------------------------------------------
void CXR_Model_BSP2::VB_AllocVBID()
{
	MAUTOSTRIP(CXR_Model_BSP_VB_AllocVBID, MAUTOSTRIP_VOID);
/*	if (!m_lPortals.Len())
	{
		VBC_Remove();
		return;
	}*/

	TAP_RCD<CBSP2_VBInfo> pVBInfo(m_lVBInfo);
	for(int i = 0; i < pVBInfo.Len(); i++)
	{
		pVBInfo[i].m_VBID = m_pVBCtx->AllocID(m_iVBC, i);
	}

	if (m_spLightData)
		m_spLightData->AllocVBIds(this);
}

void CXR_Model_BSP2::VB_FreeVBID()
{
	if (m_spLightData)
		m_spLightData->FreeVBIds();

	MAUTOSTRIP(CXR_Model_BSP_VB_FreeVBID, MAUTOSTRIP_VOID);
	if (m_iVBC < 0) return;

	TAP_RCD<CBSP2_VBInfo> pVBInfo(m_lVBInfo);
	for(int i = 0; i < pVBInfo.Len(); i++)
	{
		if (pVBInfo[i].m_VBID != (uint16)~0)
			m_pVBCtx->FreeID(pVBInfo[i].m_VBID);
	}

}

CFStr CXR_Model_BSP2::GetName(int _iLocal)
{	
	if (m_spMaster && m_spMaster->m_spLightData)
		return CFStrF("BSP2:%s:Main(%04d):%04d", m_spMaster->m_spLightData->m_FileName.Str(), m_iSubModel, _iLocal);
	else if (m_spLightData)
		return CFStrF("BSP2:%s:MainNoMaster(%04d):%04d", m_spLightData->m_FileName.Str(), m_iSubModel, _iLocal);
	else
		return CFStrF("BSP2:MainNoFile(%04d):%04d", m_iSubModel, _iLocal);
}

int CXR_Model_BSP2::GetNumLocal()
{
	MAUTOSTRIP(CXR_Model_BSP_GetNumLocal, 0);
//	if (!m_lPortals.Len()) return 0;
	return m_lVBInfo.Len();
}

int CXR_Model_BSP2::GetID(int _iLocal)
{
	MAUTOSTRIP(CXR_Model_BSP_GetID, 0);
	return m_lVBInfo[_iLocal].m_VBID;
}

template <int tPlatform>
static void SetWantTransform(CRC_BuildVertexBuffer& _VB, CXR_Model_BSP2* _pModel, uint _iLocal, CBSP2_VertexBuffer* _pBSPVB);

template <>
static void SetWantTransform<e_Platform_Default>(CRC_BuildVertexBuffer& _VB, CXR_Model_BSP2* _pModel, uint _iLocal, CBSP2_VertexBuffer* _pBSPVB)
{
}

template <>
static void SetWantTransform<e_Platform_Xenon>(CRC_BuildVertexBuffer& _VB, CXR_Model_BSP2* _pModel, uint _iLocal, CBSP2_VertexBuffer* _pBSPVB)
{
	CBSP2_VertexBuffer* pBSPVB = _pBSPVB;
	
	if(pBSPVB->m_lV.GetBasePtr())
	{
		fp32 Snap = 1.0f / 8.0f;
		fp32 MaxRange = _pModel->m_MasterVBScale.k[0];
		MaxRange = Max(MaxRange, _pModel->m_MasterVBScale.k[1]);
		MaxRange = Max(MaxRange, _pModel->m_MasterVBScale.k[2]);
		MaxRange = GetGEPow2(RoundToInt(MaxRange + 1.0f/Snap));

		CRC_VRegTransform Transform;
		Transform.m_Offset = _pModel->m_lVBOffsets[_iLocal];
		Transform.m_Offset.k[3] = 0.0f;
		Transform.m_Scale = _pModel->m_MasterVBScale;
		Transform.m_Scale.k[0] = GetGEPow2(RoundToInt(Transform.m_Scale.k[0] + 1.0f/Snap));
		Transform.m_Scale.k[1] = GetGEPow2(RoundToInt(Transform.m_Scale.k[1] + 1.0f/Snap));
		Transform.m_Scale.k[2] = GetGEPow2(RoundToInt(Transform.m_Scale.k[2] + 1.0f/Snap));

		if (Transform.m_Scale.k[0] <= 2048.0f * Snap &&
			Transform.m_Scale.k[1] <= 2048.0f * Snap &&
			Transform.m_Scale.k[2] <= 1024.0f * Snap
			)
		{
			Transform.m_Scale.CompMul(CVec4Dfp32(1.0f/2048.0f, 1.0f/2048.0f, 1.0f/1024.0f, 1.0f), Transform.m_Scale);
//				Transform.m_Scale.CompMul(CVec4Dfp32(1.0f, 1.0f, 1.0f, 1.0), Transform.m_Scale);
			_VB.Geometry_SetWantFormat(CRC_VREG_POS, CRC_VREGFMT_U3_P32);
			_VB.Geometry_SetWantTransform(CRC_VREG_POS, Transform);
			MODEL_BSP_TRACE("VB Position using CVec3D_11_11_10\n");
		}
		else if (MaxRange <= 65536.0f * Snap)
		{
			Transform.m_Scale.CompMul(CVec4Dfp32(1.0f/65536.0f, 1.0f/65536.0f, 1.0f/65536.0f, 1.0f), Transform.m_Scale);
			_VB.Geometry_SetWantFormat(CRC_VREG_POS, CRC_VREGFMT_V3_U16);
			_VB.Geometry_SetWantTransform(CRC_VREG_POS, Transform);
			MODEL_BSP_TRACE("VB Position using CVec3D_16_16_16\n");
		}
		else
		{
			// Do nothing
			MODEL_BSP_TRACE("VB Position using CVec3D_32_32_32\n");
		}
	}

	if(pBSPVB->m_lN.GetBasePtr())
	{
		_VB.Geometry_SetWantFormat(CRC_VREG_NORMAL, CRC_VREGFMT_NS3_P32);
	}

	if(pBSPVB->m_lTV1.GetBasePtr())
	{
		// Calc boundbox

		CVec2Dfp32 Min2D;
		CVec2Dfp32 Max2D;
		CVec2Dfp32::GetMinBoundRect(pBSPVB->m_lTV1.GetBasePtr(), Min2D, Max2D, _VB.m_nV);
		fp32 MaxRange = Max2D.k[0] - Min2D.k[0];
		MaxRange = Max(MaxRange, Max2D.k[1] - Min2D.k[1]);
		fp32 MinUnitAccurency = 1024.0f;

		if (Min2D.k[0] >= 0.0f && Min2D.k[1] >= 0.0f && Max2D.k[0] <= 1.0f && Max2D.k[1] <= 1.0f)
		{
			_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD0, CRC_VREGFMT_NU3_P32);
			MODEL_BSP_TRACE("VB TexCoord0 using NU3_P32\n");
		}
		else if (Min2D.k[0] >= -1.0f && Min2D.k[1] >= -1.0f && Max2D.k[0] <= 1.0f && Max2D.k[1] <= 1.0f)
		{
			_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD0, CRC_VREGFMT_NS3_P32);
			MODEL_BSP_TRACE("VB TexCoord0 using NS3_P32\n");
		}
		else if (MaxRange < 65536.0f / MinUnitAccurency)
		{
			CRC_VRegTransform Transform;
			Transform.m_Offset.k[0] = Min2D.k[0];
			Transform.m_Offset.k[1] = Min2D.k[1];
			Transform.m_Offset.k[2] = 0.0f;
			Transform.m_Offset.k[3] = 0.0f;
			CVec2Dfp32 Range = (Max2D - Min2D);
			Transform.m_Scale.k[0] = Range.k[0];
			Transform.m_Scale.k[1] = Range.k[1];
			Transform.m_Scale.k[2] = 1.0f;
			Transform.m_Scale.k[3] = 1.0f;

			Transform.m_Scale.CompMul(CVec4Dfp32(1.0f/65535.0f, 1.0f/65535.0f, 1.0f/65535.0f, 1.0f), Transform.m_Scale);
			_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD0, CRC_VREGFMT_V3_U16);
			_VB.Geometry_SetWantTransform(CRC_VREG_TEXCOORD0, Transform);
			MODEL_BSP_TRACE("VB TexCoord0 using V2_I16\n");
		}
		else
			MODEL_BSP_TRACE("VB TexCoord0 using V2_F32\n");
	}

	if(pBSPVB->m_lTangU.GetBasePtr())
	{
		_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD1, CRC_VREGFMT_NS3_P32);
	}

	if(pBSPVB->m_lTangV.GetBasePtr())
	{
		_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD2, CRC_VREGFMT_NS3_P32);
	}

	if(pBSPVB->m_lTV2.GetBasePtr())
	{
		// Calc boundbox
		CVec2Dfp32 Min2D;
		CVec2Dfp32 Max2D;
		CVec2Dfp32::GetMinBoundRect(pBSPVB->m_lTV2.GetBasePtr(), Min2D, Max2D, _VB.m_nV);
		fp32 MaxRange = Max2D.k[0] - Min2D.k[0];
		MaxRange = Max(MaxRange, Max2D.k[1] - Min2D.k[1]);
		fp32 MinUnitAccurency = 1024.0f;

		if (Min2D.k[0] >= 0.0f && Min2D.k[1] >= 0.0f && Max2D.k[0] <= 1.0f && Max2D.k[1] <= 1.0f)
		{
			_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD3, CRC_VREGFMT_NU3_P32);
			MODEL_BSP_TRACE("VB TexCoord3 using NU3_P32\n");
		}
		else if (Min2D.k[0] >= -1.0f && Min2D.k[1] >= -1.0f && Max2D.k[0] <= 1.0f && Max2D.k[1] <= 1.0f)
		{
			_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD3, CRC_VREGFMT_NS3_P32);
			MODEL_BSP_TRACE("VB TexCoord3 using NS3_P32\n");
		}
		else if (MaxRange < 65536.0f / MinUnitAccurency)
		{
			CRC_VRegTransform Transform;
			Transform.m_Offset.k[0] = Min2D.k[0];
			Transform.m_Offset.k[1] = Min2D.k[1];
			Transform.m_Offset.k[2] = 0.0f;
			Transform.m_Offset.k[3] = 0.0f;
			CVec2Dfp32 Range = (Max2D - Min2D);
			Transform.m_Scale.k[0] = Range.k[0];
			Transform.m_Scale.k[1] = Range.k[1];
			Transform.m_Scale.k[2] = 1.0f;
			Transform.m_Scale.k[3] = 1.0f;

			Transform.m_Scale.CompMul(CVec4Dfp32(1.0f/65535.0f, 1.0f/65535.0f, 1.0f/65535.0f, 1.0f), Transform.m_Scale);
			_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD3, CRC_VREGFMT_V2_U16);
			_VB.Geometry_SetWantTransform(CRC_VREG_TEXCOORD3, Transform);
			MODEL_BSP_TRACE("VB TexCoord3 using V2_I16\n");
		}
		else
			MODEL_BSP_TRACE("VB TexCoord3 using V2_F32\n");
	}
}

static void GetMinMax(const fp32* _pValues, fp32& _Min, fp32& _Max, uint _nV)
{
	fp32 MinV = _pValues[0];
	fp32 MaxV = _pValues[0];

	for(uint i = 1; i < _nV; i++)
	{
		fp32 Val = _pValues[1];
		MinV = Min(MinV, Val);
		MaxV = Max(MaxV, Val);
	}
}

template <>
static void SetWantTransform<e_Platform_PS3>(CRC_BuildVertexBuffer& _VB, CXR_Model_BSP2* _pModel, uint _iLocal, CBSP2_VertexBuffer* _pBSPVB)
{
	CBSP2_VertexBuffer* pBSPVB = _pBSPVB;
	
	if(pBSPVB->m_lV.GetBasePtr())
	{
		fp32 Snap = 1.0f / 8.0f;
		fp32 MaxRange = _pModel->m_MasterVBScale.k[0];
		MaxRange = Max(MaxRange, _pModel->m_MasterVBScale.k[1]);
		MaxRange = Max(MaxRange, _pModel->m_MasterVBScale.k[2]);
		MaxRange = GetGEPow2(RoundToInt(MaxRange + 1.0f/Snap));

		CRC_VRegTransform Transform;
		Transform.m_Offset = _pModel->m_lVBOffsets[_iLocal];
		Transform.m_Offset.k[3] = 0.0f;
		Transform.m_Scale = _pModel->m_MasterVBScale;
		Transform.m_Scale.k[0] = GetGEPow2(RoundToInt(Transform.m_Scale.k[0] + 1.0f/Snap));
		Transform.m_Scale.k[1] = GetGEPow2(RoundToInt(Transform.m_Scale.k[1] + 1.0f/Snap));
		Transform.m_Scale.k[2] = GetGEPow2(RoundToInt(Transform.m_Scale.k[2] + 1.0f/Snap));

		if (Transform.m_Scale.k[0] <= 2048.0f * Snap &&
			Transform.m_Scale.k[1] <= 2048.0f * Snap &&
			Transform.m_Scale.k[2] <= 1024.0f * Snap
			)
		{
			Transform.m_Scale.CompMul(CVec4Dfp32(1.0f/2048.0f, 1.0f/2048.0f, 1.0f/1024.0f, 1.0f), Transform.m_Scale);
//				Transform.m_Scale.CompMul(CVec4Dfp32(1.0f, 1.0f, 1.0f, 1.0), Transform.m_Scale);
			_VB.Geometry_SetWantFormat(CRC_VREG_POS, CRC_VREGFMT_U3_P32);
			_VB.Geometry_SetWantTransform(CRC_VREG_POS, Transform);
			MODEL_BSP_TRACE("VB Position using CVec3D_11_11_10\n");
		}
		else if (MaxRange <= 65536.0f * Snap)
		{
			Transform.m_Scale.CompMul(CVec4Dfp32(1.0f/65536.0f, 1.0f/65536.0f, 1.0f/65536.0f, 1.0f), Transform.m_Scale);
			_VB.Geometry_SetWantFormat(CRC_VREG_POS, CRC_VREGFMT_V3_U16);
			_VB.Geometry_SetWantTransform(CRC_VREG_POS, Transform);
			MODEL_BSP_TRACE("VB Position using CVec3D_16_16_16\n");
		}
		else
		{
			// Do nothing
			MODEL_BSP_TRACE("VB Position using CVec3D_32_32_32\n");
		}
	}

	if(pBSPVB->m_lN.GetBasePtr())
	{
		_VB.Geometry_SetWantFormat(CRC_VREG_NORMAL, CRC_VREGFMT_NS3_P32);
	}

	if(pBSPVB->m_lTV1.GetBasePtr())
	{
		// Calc boundbox

		CVec2Dfp32 Min2D;
		CVec2Dfp32 Max2D;
		CVec2Dfp32::GetMinBoundRect(pBSPVB->m_lTV1.GetBasePtr(), Min2D, Max2D, _VB.m_nV);
		fp32 MaxRange = Max2D.k[0] - Min2D.k[0];
		MaxRange = Max(MaxRange, Max2D.k[1] - Min2D.k[1]);
		fp32 MinUnitAccurency = 1024.0f;

		if (Min2D.k[0] >= -1.0f && Min2D.k[1] >= -1.0f && Max2D.k[0] <= 1.0f && Max2D.k[1] <= 1.0f)
		{
			_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD0, CRC_VREGFMT_NS2_I16);
			MODEL_BSP_TRACE("VB TexCoord0 using NS2_I16\n");
		}
		else if (MaxRange < 65536.0f / MinUnitAccurency)
		{
			CRC_VRegTransform Transform;
			Transform.m_Offset.k[0] = Min2D.k[0];
			Transform.m_Offset.k[1] = Min2D.k[1];
			Transform.m_Offset.k[2] = 0.0f;
			Transform.m_Offset.k[3] = 0.0f;
			CVec2Dfp32 Range = (Max2D - Min2D);
			Transform.m_Scale.k[0] = Range.k[0];
			Transform.m_Scale.k[1] = Range.k[1];
			Transform.m_Scale.k[2] = 1.0f;
			Transform.m_Scale.k[3] = 1.0f;

			Transform.m_Scale.CompMul(CVec4Dfp32(1.0f/65535.0f, 1.0f/65535.0f, 1.0f/65535.0f, 1.0f), Transform.m_Scale);
			_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD0, CRC_VREGFMT_V2_U16);
			_VB.Geometry_SetWantTransform(CRC_VREG_TEXCOORD0, Transform);
			MODEL_BSP_TRACE("VB TexCoord0 using V2_I16\n");
		}
		else
			MODEL_BSP_TRACE("VB TexCoord0 using V2_F32\n");
	}

	if(pBSPVB->m_lTangU.GetBasePtr())
	{
		_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD1, CRC_VREGFMT_NS3_P32);
	}

	if(pBSPVB->m_lTangV.GetBasePtr())
	{
		_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD2, CRC_VREGFMT_NS3_P32);
	}

	if(pBSPVB->m_lTV2.GetBasePtr())
	{
		// Calc boundbox
		CVec2Dfp32 Min2D;
		CVec2Dfp32 Max2D;
		CVec2Dfp32::GetMinBoundRect(pBSPVB->m_lTV2.GetBasePtr(), Min2D, Max2D, _VB.m_nV);
		fp32 MaxRange = Max2D.k[0] - Min2D.k[0];
		MaxRange = Max(MaxRange, Max2D.k[1] - Min2D.k[1]);
		fp32 MinUnitAccurency = 1024.0f;

		if (Min2D.k[0] >= -1.0f && Min2D.k[1] >= -1.0f && Max2D.k[0] <= 1.0f && Max2D.k[1] <= 1.0f)
		{
			_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD3, CRC_VREGFMT_NS2_I16);
			MODEL_BSP_TRACE("VB TexCoord3 using NS2_I16\n");
		}
		else if (MaxRange < 65536.0f / MinUnitAccurency)
		{
			CRC_VRegTransform Transform;
			Transform.m_Offset.k[0] = Min2D.k[0];
			Transform.m_Offset.k[1] = Min2D.k[1];
			Transform.m_Offset.k[2] = 0.0f;
			Transform.m_Offset.k[3] = 0.0f;
			CVec2Dfp32 Range = (Max2D - Min2D);
			Transform.m_Scale.k[0] = Range.k[0];
			Transform.m_Scale.k[1] = Range.k[1];
			Transform.m_Scale.k[2] = 1.0f;
			Transform.m_Scale.k[3] = 1.0f;

			Transform.m_Scale.CompMul(CVec4Dfp32(1.0f/65535.0f, 1.0f/65535.0f, 1.0f/65535.0f, 1.0f), Transform.m_Scale);
			_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD3, CRC_VREGFMT_V2_U16);
			_VB.Geometry_SetWantTransform(CRC_VREG_TEXCOORD3, Transform);
			MODEL_BSP_TRACE("VB TexCoord3 using V2_I16\n");
		}
		else
			MODEL_BSP_TRACE("VB TexCoord3 using V2_F32\n");
	}
	if(pBSPVB->m_lLMScale.GetBasePtr())
	{
		CRC_VRegTransform Transform;
		fp32 MinV = 0.0f, MaxV = 0.0f;
		GetMinMax(pBSPVB->m_lLMScale.GetBasePtr(), MinV, MaxV, _VB.m_nV);
		if(MinV >= -1.0f && MaxV <= 1.0f)
		{
			_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD4, CRC_VREGFMT_NS1_I16);
		}
		else if(MinV >= -32768.0f && MaxV <= 32767.0f)
		{
			_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD4, CRC_VREGFMT_V1_I16);
		}
	}
}

void CXR_Model_BSP2::Get(int _iLocal, CRC_BuildVertexBuffer& _VB, int _Flags)
{
	MAUTOSTRIP(CXR_Model_BSP_Get, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP2::Get, XR_BSPMODEL);
	CBSP2_VertexBuffer* pBSPVB = GetVertexBuffer(_iLocal);

	if (_Flags & VB_GETFLAGS_FALLBACK)
		m_lVBInfo[_iLocal].m_bNoRelease = 1;

	_VB.Clear();

	FillChar(pBSPVB->m_lCol.GetBasePtr(), pBSPVB->m_lCol.Len()*4, -1);

	_VB.m_nV = pBSPVB->m_lV.Len();

	if (pBSPVB->m_lV.GetBasePtr())
	{
		_VB.Geometry_VertexArray(pBSPVB->m_lV.GetBasePtr());
	}
	
	if (pBSPVB->m_lN.GetBasePtr())
	{
		_VB.Geometry_NormalArray(pBSPVB->m_lN.GetBasePtr());
	}

	if (pBSPVB->m_lCol.GetBasePtr())
		_VB.Geometry_ColorArray((uint32 *)pBSPVB->m_lCol.GetBasePtr());

	if (pBSPVB->m_lTV1.GetBasePtr())
	{
		_VB.Geometry_TVertexArray(pBSPVB->m_lTV1.GetBasePtr(), 0);
	}

	if (pBSPVB->m_lTangU.GetBasePtr())
	{
		_VB.Geometry_TVertexArray(pBSPVB->m_lTangU.GetBasePtr(), 1);
	}
	if (pBSPVB->m_lTangV.GetBasePtr())
	{
		_VB.Geometry_TVertexArray(pBSPVB->m_lTangV.GetBasePtr(), 2);
	}

	if (pBSPVB->m_lTV2.GetBasePtr())
	{
		_VB.Geometry_TVertexArray(pBSPVB->m_lTV2.GetBasePtr(), 3);
	}

	if (pBSPVB->m_lLMScale.GetBasePtr())
	{
		_VB.Geometry_TVertexArray(pBSPVB->m_lLMScale.GetBasePtr(), 4);
		// Calc boundbox
	}

	switch (D_MPLATFORM)
	{
	case e_Platform_Xenon: SetWantTransform<e_Platform_Xenon>(_VB, this, _iLocal, pBSPVB); break;
	case e_Platform_PS3:   SetWantTransform<e_Platform_PS3>(_VB, this, _iLocal, pBSPVB); break;
	default:               SetWantTransform<e_Platform_Default>(_VB, this, _iLocal, pBSPVB); break;
	}

	// Calculate boundbox here
#if 0
	CVec2Dfp32 Min2D, Max2D;
	CVec3Dfp32 Min3D, Max3D;
	CVec3Dfp32::GetMinBoundBox(_VB.m_pV, _VBGetOptions.m_Bound_Vertex.m_Min, _VBGetOptions.m_Bound_Vertex.m_Max, _VB.m_nV);
	CVec2Dfp32::GetMinBoundRect((CVec2Dfp32*)_VB.m_pTV[0], Min2D, Max2D, _VB.m_nV);
	_VBGetOptions.m_Bound_Texture0.m_Min	= CVec4Dfp32(Min2D.k[0], Min2D.k[1], 0, 0);
	_VBGetOptions.m_Bound_Texture0.m_Max	= CVec4Dfp32(Max2D.k[0], Max2D.k[1], 0, 0);
	CVec3Dfp32::GetMinBoundBox((CVec3Dfp32*)_VB.m_pTV[1], Min3D, Max3D, _VB.m_nV);
	_VBGetOptions.m_Bound_Texture1.m_Min	= CVec4Dfp32(Min3D.k[0], Min3D.k[1], Min3D.k[2], 0);
	_VBGetOptions.m_Bound_Texture1.m_Max	= CVec4Dfp32(Max3D.k[0], Max3D.k[1], Max3D.k[2], 0);
	CVec3Dfp32::GetMinBoundBox((CVec3Dfp32*)_VB.m_pTV[2], Min3D, Max3D, _VB.m_nV);
	_VBGetOptions.m_Bound_Texture2.m_Min	= CVec4Dfp32(Min3D.k[0], Min3D.k[1], Min3D.k[2], 0);
	_VBGetOptions.m_Bound_Texture2.m_Max	= CVec4Dfp32(Max3D.k[0], Max3D.k[1], Max3D.k[2], 0);
	if(_VB.m_pTV[3])
	{
		CVec2Dfp32::GetMinBoundRect((CVec2Dfp32*)_VB.m_pTV[3], Min2D, Max2D, _VB.m_nV);
		_VBGetOptions.m_Bound_Texture3.m_Min	= CVec4Dfp32(Min2D.k[0], Min2D.k[1], 0, 0);
		_VBGetOptions.m_Bound_Texture3.m_Max	= CVec4Dfp32(Max2D.k[0], Max2D.k[1], 0, 0);
	}
#endif
}

void CXR_Model_BSP2::Release(int _iLocal)
{
	MAUTOSTRIP(CXR_Model_BSP_Release, MAUTOSTRIP_VOID);
	if (!m_lVBInfo.ValidPos(_iLocal))
		Error("Release", CStrF("Invalid local ID %d", _iLocal));

	if (!m_lVBInfo[_iLocal].m_bNoRelease)
	{
		m_lspVB[_iLocal] = NULL;
	}
}

// -------------------------------------------------------------------
void CXR_Model_BSP2::RenderNHF(CBSP2_RenderParams* _pRenderParams, const uint32* _piFaces, int _nFaces, int _iPL)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderNHF, MAUTOSTRIP_VOID);
	if (_nFaces > MODEL_BSP_MAXPORTALLEAFFACES)
	{
		ConOut(CStrF("(CXR_Model_BSP2::RenderNHF) Too many faces. %d/%d", _nFaces, MODEL_BSP_MAXPORTALLEAFFACES));
		return;
	}

	int iBaseV = 0;
	if (_iPL)
	{
		Fog_TraceVertices(_piFaces, _nFaces, _iPL);
		UntagFogVertices();

		const CBSP2_PortalLeafExt* pPL = &m_pPortalLeaves[_iPL];
		iBaseV = pPL->m_iBaseVertex;
	}
	else
	{
		Fog_TraceVertices(_piFaces, _nFaces);
		UntagFogVertices();
	}

	uint32 liVisFaceVerts[MODEL_BSP_MAXPORTALLEAFFACES];

	CXR_VertexBuffer* pVB = _pRenderParams->m_pVBM->Alloc_VB();
	if (pVB)
	{
		if (RenderTesselate(_piFaces, _nFaces, 0, _pRenderParams->m_pVBM, pVB, liVisFaceVerts))
		{
			pVB->m_pAttrib = _pRenderParams->m_pVBM->Alloc_Attrib();
			if (!pVB->m_pAttrib) return;

			Fog_SetVertices(_pRenderParams, pVB, _piFaces, _nFaces, liVisFaceVerts, iBaseV);

			pVB->m_pAttrib->SetDefault();
			if (_pRenderParams->m_pCurrentEngine) _pRenderParams->m_pCurrentEngine->SetDefaultAttrib(pVB->m_pAttrib);
			_pRenderParams->m_pSceneFog->SetDepthFogNone(pVB->m_pAttrib);

			if (_pRenderParams->m_pCurMedium->m_MediumFlags & XW_MEDIUM_FOGADDITIVE)
				pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
			else
				pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

			pVB->m_pAttrib->Attrib_ZCompare(CRC_COMPARE_EQUAL);
			pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
			pVB->m_pAttrib->Attrib_TextureID(0, _pRenderParams->m_pSceneFog->m_FogTableTextureID);
			pVB->Matrix_Set(_pRenderParams->m_pVBMatrixM2V);
			pVB->m_Priority = _pRenderParams->m_BasePriority_Opaque + CXR_VBPRIORITY_VOLUMETRICFOG;
			_pRenderParams->m_pVBM->AddVB(pVB);
		}
	}
}

void CXR_Model_BSP2::VB_RenderNHF(CBSP2_RenderParams* _pRenderParams, int _iVB, const uint16* _piPrim, int _nPrim, CBSP2_PortalLeafExt* _pPL)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_RenderNHF, MAUTOSTRIP_VOID);
	// Note: _piPrim is not CRC_RIP_END terminated.

	CXR_VertexBuffer* pVB = _pRenderParams->m_pVBM->Alloc_VB();
	if (pVB)
	{
		CBSP2_VBChain Chain;
		VB_CopyGeometry(_pRenderParams, _iVB, &Chain);
		Chain.SetToVB(pVB);

		pVB->Render_IndexedPrimitives(const_cast<uint16*>(_piPrim), _nPrim);

		pVB->m_pAttrib = _pRenderParams->m_pVBM->Alloc_Attrib();
		if (!pVB->m_pAttrib) return;

		pVB->m_pAttrib->SetDefault();
		if (_pRenderParams->m_pCurrentEngine) _pRenderParams->m_pCurrentEngine->SetDefaultAttrib(pVB->m_pAttrib);

		if (!Fog_SetAttrib(_pRenderParams, pVB->m_pAttrib, _pPL, _pRenderParams->m_pViewData->m_CurLocalVP))
			return;

		pVB->Matrix_Set(_pRenderParams->m_pVBMatrixM2V);
		pVB->m_Priority = _pRenderParams->m_BasePriority_Opaque + CXR_VBPRIORITY_VOLUMETRICFOG;
		_pRenderParams->m_pVBM->AddVB(pVB);
	}
}

#include "../../XR/XRShader.h"

bool CXR_Model_BSP2::PrepareShaderQueue(CBSP2_RenderParams* _pRenderParams)
{
	CBSP2_LightData* pLD = m_spLightData;
	if (!pLD)
		return true;

	int Len = pLD->m_lShaderQueue.Len();
	CBSP2_VBChain* pSQ = (CBSP2_VBChain*) _pRenderParams->m_pVBM->Alloc(sizeof(CBSP2_VBChain) * Len);
	if (!pSQ)
		return false;

	_pRenderParams->m_pShaderQueueVBChain.Set(pSQ, Len);
	FillChar(_pRenderParams->m_pShaderQueueVBChain.GetBasePtr(), _pRenderParams->m_pShaderQueueVBChain.ListSize(), 0);
	return true;
}

void CXR_Model_BSP2::RenderShaderQueue(CBSP2_RenderParams* _pRenderParams)
{
	CBSP2_LightData* pLD = m_spLightData;
	if (!pLD)
		return;

	CBSP2_SceneGraph* pSGI = m_pView->m_pSceneGraph;
	if (!pSGI)
		return;

//	int nSQ = pLD->m_lShaderQueueVBChain.Len();
//	CBSP2_VBChain * lShaderQueueVBChain = pLD->m_lShaderQueueVBChain.GetBasePtr();
	TAP_RCD<CBSP2_VBChain> lShaderQueueVBChain = _pRenderParams->m_pShaderQueueVBChain;

	const CBSP2_ShaderQueueElement* pSQ = pLD->m_lShaderQueue.GetBasePtr();
	M_ASSERT(pLD->m_lShaderQueue.Len() == lShaderQueueVBChain.Len(), "!");
//		Error("RenderShaderQueue", "Internal error.");

	CXR_Shader* pShader = _pRenderParams->m_pCurrentEngine->GetShader();

	TAP<CXR_SurfaceShaderParams> lSSP = m_lSurfaceShaderParams;

	const uint MaxBatch = 32;
	uint liBatchSQ[MaxBatch];
	const CXR_SurfaceShaderParams* lpBatchSSP[MaxBatch];
	CXR_VertexBufferGeometry lBatchGeom[MaxBatch];

	uint iSQBatch = 0;

	while(iSQBatch < lShaderQueueVBChain.Len())
	{
		if (!lShaderQueueVBChain[iSQBatch].GetVBIDChain())
		{
			iSQBatch++;
			continue;
		}

		liBatchSQ[0] = iSQBatch;
		uint iLight = pSQ[iSQBatch].m_iLight;
		uint nSQBatch = 1;
		iSQBatch++;
		while((iSQBatch < lShaderQueueVBChain.Len()) && (iLight == pSQ[iSQBatch].m_iLight))
		{
			if (lShaderQueueVBChain[iSQBatch].GetVBIDChain())
			{
				M_PRECACHE128(0, &lSSP[pSQ[iSQBatch].m_iSurface]);
				liBatchSQ[nSQBatch++] = iSQBatch;
				iSQBatch++;
				if (nSQBatch == MaxBatch)
					break;
			}
			else
				iSQBatch++;
		}

		if (nSQBatch)
		{
			uint iInner;
			for(iInner = 0; iInner < nSQBatch; iInner++)
				lpBatchSSP[iInner] = &lSSP[pSQ[liBatchSQ[iInner]].m_iSurface];

			for(iInner = 0; iInner < nSQBatch; iInner++)
			{
				CBSP2_VBChain& VBC = lShaderQueueVBChain[liBatchSQ[iInner]];
				lBatchGeom[iInner].Create(VBC.m_pChain, _pRenderParams->m_pVBMatrixM2V);
				VBC.Clear();
			}

			CXR_ShaderParams Params;
			Params.Create(_pRenderParams->m_pVBMatrixM2W, _pRenderParams->m_pVBMatrixW2V, pShader);
			Params.m_nVB = nSQBatch;

			M_PRECACHE128(0, &lShaderQueueVBChain[iSQBatch]);

			if(!(_pRenderParams->m_pCurrentEngine->m_DebugFlags & M_Bit(19)))
				pShader->RenderShading(*pSGI->GetLight(iLight), lBatchGeom, &Params, lpBatchSSP);

/*			const CXR_SurfaceShaderParams* pSSP = &lSSP[pSQ[i].m_iSurface];
			{
				CXR_VertexBuffer VB;
				
				lShaderQueueVBChain[i].SetToVB(&VB);

				VB.Matrix_Set(_pRenderParams->m_pVBMatrixM2V);

				CXR_ShaderParams Params;
				Params.Create(_pRenderParams->m_pVBMatrixM2W, _pRenderParams->m_pVBMatrixW2V, pShader);
				const CXR_SurfaceShaderParams* lpSSP[1] = { pSSP };
				if(!(_pRenderParams->m_pCurrentEngine->m_DebugFlags & M_Bit(19)))
					pShader->RenderShading(*pSGI->GetLight(pSQ[i].m_iLight), &VB, &Params, lpSSP);
			}

			lShaderQueueVBChain[i].Clear();*/
		}
	}
}

#define NEW_RENDER_SLC

#ifdef NEW_RENDER_SLC

#ifdef CPU_LITTLEENDIAN
#define Writeu16x2(p, a, b) {	*((uint32*)(p)) = (a) + ((b) << 16); }
#else
#define Writeu16x2(p, a, b) {	*((uint32*)(p)) = (b) + ((a) << 16); }
#endif

#define WriteOddTriangle(_piPrim, _iV, v)	\
{	\
	_piPrim[0] = _iV;	\
	Writeu16x2(_piPrim+1, _iV+v+1, _iV+v+2);	\
	_piPrim += 3;	\
	v++;	\
}

#define WriteEvenTriangle(_piPrim, _iV, v)	\
{	\
	Writeu16x2(_piPrim, _iV, _iV+v+1);	\
	_piPrim[2] = _iV+v+2;	\
	_piPrim += 3;	\
	v++;	\
}

#define WriteDoubleTriangle(_piPrim, _iV, v)	\
{	\
	Writeu16x2(_piPrim+0, _iV, _iV+v+1);	\
	Writeu16x2(_piPrim+2, _iV+v+2, _iV);	\
	Writeu16x2(_piPrim+4, _iV+v+2, _iV+v+3);	\
	_piPrim += 6;	\
	v += 2;	\
}

M_FORCEINLINE void CreateFanTriangles(uint16* _piPrim, uint _iV, uint _nT)
{
	int v = 0;
	if (aint(_piPrim) & 2)
	{
		WriteOddTriangle(_piPrim, _iV, v);
		_nT--;
	}
	while(_nT >= 2)
	{
		WriteDoubleTriangle(_piPrim, _iV, v);
		_nT -= 2;
	}
	if(_nT == 1)
	{
		WriteEvenTriangle(_piPrim, _iV, v);
		_nT -= 1;
	}
}


void CXR_Model_BSP2::Render_SLC(CBSP2_RenderParams*M_RESTRICT _pRenderParams, int _iPL, const CRC_ClipVolume* _pClip, int _iSLC, int _nSLC)
{
	if (!_nSLC)
		return;

	CBSP2_LightData* pLD = m_spLightData;
	if (!pLD)
		return;

	CBSP2_SceneGraph* pSGI = m_pWorldView->m_pSceneGraph;
	if (!pSGI)
		return;

	CXR_VBManager* pVBM = _pRenderParams->m_pVBM;

	// Render_SLC is typically run in a single threaded fashion, but we lock stuff for good measure.
	M_LOCK(pSGI->m_Lock);
	M_ASSERT(pLD->m_lSLC.ValidPos(_iSLC), "!");

	TAP_RCD<uint32> pliSLCFaces(pLD->m_liSLCFaces);
	TAP_RCD<spCXW_Surface> plspSurfaces(m_lspSurfaces);
	TAP_RCD<CXR_Light> plLights(pSGI->m_lLights);
	TAP_RCD<CBSP2_SLCluster> plSLC(pLD->m_lSLC);
	TAP_RCD<CXR_LightOcclusionInfo> plLightOcclusion(m_pWorldView->m_lLightOcclusion);
	TAP_RCD<CBSP2_VBChain> plShaderQueueVBChain(_pRenderParams->m_pShaderQueueVBChain);
	TAP_RCD<CBSP2_VBInfo> pVBInfo(m_lVBInfo);

	CVec3Dfp32 CurLocalVP = _pRenderParams->m_pViewData->m_CurLocalVP;
	CVec4Dfp32 CurLocalVP4;
	CurLocalVP4[0] = CurLocalVP[0];
	CurLocalVP4[1] = CurLocalVP[1];
	CurLocalVP4[2] = CurLocalVP[2];
	CurLocalVP4[3] = 1.0f;

	const int ChainHeapSize = 64;
	int nChainHeapUsed = 0;
	CXR_VBIDChain* pChainHeap = (CXR_VBIDChain*) pVBM->Alloc(sizeof(CXR_VBIDChain) * ChainHeapSize);
	if (!pChainHeap)
		return;

	for(int i = 0; i < _nSLC; i++)
	{
		const CBSP2_SLCluster& SLC = plSLC[i + _iSLC];
		if (!SLC.m_nFaces)
			continue;

		CBox3Dfp32 SLCBoundBox = SLC.m_BoundBox;
		{
			const CPlane3Dfp32* pPlanes = _pClip->m_Planes;

			int nPlanes = _pClip->m_nPlanes;
			fp32 Dist[4];
			Dist[0] = pPlanes[0].GetBoxMinDistance(SLCBoundBox.m_Min, SLCBoundBox.m_Max);
			Dist[1] = pPlanes[1].GetBoxMinDistance(SLCBoundBox.m_Min, SLCBoundBox.m_Max);
			Dist[2] = pPlanes[2].GetBoxMinDistance(SLCBoundBox.m_Min, SLCBoundBox.m_Max);
			Dist[3] = pPlanes[3].GetBoxMinDistance(SLCBoundBox.m_Min, SLCBoundBox.m_Max);

			fp32 d = Max(Max(Dist[0], Dist[1]), Max(Dist[2], Dist[3]));
			if(d > MODEL_BSP_EPSILON)
				continue;

			int p;
			for (p = 4; p < nPlanes; p++)
				if (pPlanes[p].GetBoxMinDistance(SLCBoundBox.m_Min, SLCBoundBox.m_Max) > MODEL_BSP_EPSILON) break;
			if (p < nPlanes) 
				continue;
		}

		if (SLC.m_iiFaces + SLC.m_nFaces > pliSLCFaces.Len())
			continue;
		if (SLC.m_iSurface >= plspSurfaces.Len())
			continue;
		if (SLC.m_iLight >= plLights.Len())
			continue;

		const uint32* piFaces = &pliSLCFaces[SLC.m_iiFaces];
		int nFaces = SLC.m_nFaces;

//		const int MaxFaces = 2048;
//		uint16 liVisFaces[MaxFaces];
//		int nVisFaces = 0;
		const CBSP2_Face* pF = m_pFaces;
		const CPlane3Dfp32* pP = m_pPlanes;

		int f = 0;
		while(f < nFaces)
		{
			M_PRECACHE128(0, pF + piFaces[f]);

			// Lock taken in the beginning of the function
			int MaxTri = pVBM->GetAvail() >> 3;		// Well.. 4 is almost 3  =P
			if (MaxTri < (MODEL_BSP_MAXFACEVERTICES-2))
				return;
			CBSP2_VBChain Chain;
			uint16* piPrim = NULL;
			int nP = 0;
			int nTri = 0;

			int iVBCurrent = -1;
			{
				{
					M_LOCK(pVBM->m_AllocLock);
					piPrim = (uint16*)pVBM->Alloc_Open();
					if(!piPrim)
						return;

					vec128 lvp = CurLocalVP4.v;

					while(f < nFaces)
					{
						const int nBatchFaces = 512;
						uint nFacesInt = Min(f+nBatchFaces, nFaces);

						fp32 M_ALIGN(16) lFaceDist[nBatchFaces];

/*						{
							uint fdist = f;
							uint flocal = 0;
							for(; nFacesInt-fdist >= 4; fdist+=4)
							{
								uint iFace0 = piFaces[fdist];
								uint iFace1 = piFaces[fdist+1];
								uint iFace2 = piFaces[fdist+2];
								uint iFace3 = piFaces[fdist+3];
								M_PRECACHE(&pF[iFace0]);
								M_PRECACHE(&pF[iFace1]);
								M_PRECACHE(&pF[iFace2]);
								M_PRECACHE(&pF[iFace3]);
							}
						}*/
						{
							uint fdist = f;
							uint flocal = 0;
							if(nFacesInt-fdist >= 16)
							{
								const CBSP2_Face*M_RESTRICT pF0 = pF + piFaces[fdist+0];
								M_PRECACHE128(0, &pP[pF0->m_iPlane]);
								const CBSP2_Face*M_RESTRICT pF1 = pF + piFaces[fdist+1];
								M_PRECACHE128(0, &pP[pF1->m_iPlane]);
								const CBSP2_Face*M_RESTRICT pF2 = pF + piFaces[fdist+2];
								M_PRECACHE128(0, &pP[pF2->m_iPlane]);
								const CBSP2_Face*M_RESTRICT pF3 = pF + piFaces[fdist+3];
								M_PRECACHE128(0, &pP[pF3->m_iPlane]);
								const CBSP2_Face*M_RESTRICT pF4 = pF + piFaces[fdist+4];
								M_PRECACHE128(0, &pP[pF4->m_iPlane]);
								const CBSP2_Face*M_RESTRICT pF5 = pF + piFaces[fdist+5];
								M_PRECACHE128(0, &pP[pF5->m_iPlane]);
								const CBSP2_Face*M_RESTRICT pF6 = pF + piFaces[fdist+6];
								M_PRECACHE128(0, &pP[pF6->m_iPlane]);
								const CBSP2_Face*M_RESTRICT pF7 = pF + piFaces[fdist+7];
								M_PRECACHE128(0, &pP[pF7->m_iPlane]);

								M_PRECACHE128(0, pF + piFaces[fdist+8]);
								M_PRECACHE128(0, pF + piFaces[fdist+12]);

								for(; nFacesInt-fdist >= 20; fdist+=8)
								{
/*									const CBSP2_Face* pF8 = pF + piFaces[fdist+8];
									M_PRECACHE128(0, &pP[pF8->m_iPlane]);
									const CBSP2_Face* pF9 = pF + piFaces[fdist+9];
									M_PRECACHE128(0, &pP[pF9->m_iPlane]);
									const CBSP2_Face* pF10 = pF + piFaces[fdist+10];
									M_PRECACHE128(0, &pP[pF10->m_iPlane]);
									const CBSP2_Face* pF11 = pF + piFaces[fdist+11];
									M_PRECACHE128(0, &pP[pF11->m_iPlane]);
									const CBSP2_Face* pF12 = pF + piFaces[fdist+12];
									M_PRECACHE128(0, &pP[pF12->m_iPlane]);
									const CBSP2_Face* pF13 = pF + piFaces[fdist+13];
									M_PRECACHE128(0, &pP[pF13->m_iPlane]);
									const CBSP2_Face* pF14 = pF + piFaces[fdist+14];
									M_PRECACHE128(0, &pP[pF14->m_iPlane]);
									const CBSP2_Face* pF15 = pF + piFaces[fdist+15];
									M_PRECACHE128(0, &pP[pF15->m_iPlane]);*/
									const CBSP2_Face*M_RESTRICT pFLoc = pF;
									const CPlane3Dfp32*M_RESTRICT pPLoc = pP;
									const uint32*M_RESTRICT piFLoc = &piFaces[fdist+8];
									const CBSP2_Face*M_RESTRICT pF8 = pFLoc + *piFLoc; piFLoc++;
									M_PRECACHE128(0, &pPLoc[pF8->m_iPlane]);
									const CBSP2_Face*M_RESTRICT pF9 = pFLoc + *piFLoc; piFLoc++;
									M_PRECACHE128(0, &pPLoc[pF9->m_iPlane]);
									const CBSP2_Face*M_RESTRICT pF10 = pFLoc + *piFLoc; piFLoc++;
									M_PRECACHE128(0, &pPLoc[pF10->m_iPlane]);
									const CBSP2_Face*M_RESTRICT pF11 = pFLoc + *piFLoc; piFLoc++;
									M_PRECACHE128(0, &pPLoc[pF11->m_iPlane]);
									const CBSP2_Face*M_RESTRICT pF12 = pFLoc + *piFLoc; piFLoc++;
									M_PRECACHE128(0, &pPLoc[pF12->m_iPlane]);
									const CBSP2_Face*M_RESTRICT pF13 = pFLoc + *piFLoc; piFLoc++;
									M_PRECACHE128(0, &pPLoc[pF13->m_iPlane]);
									const CBSP2_Face*M_RESTRICT pF14 = pFLoc + *piFLoc; piFLoc++;
									M_PRECACHE128(0, &pPLoc[pF14->m_iPlane]);
									const CBSP2_Face*M_RESTRICT pF15 = pFLoc + *piFLoc; piFLoc++;
									M_PRECACHE128(0, &pPLoc[pF15->m_iPlane]);


									M_PRECACHE128(0, &pFLoc[*piFLoc]); piFLoc += 4;
									M_PRECACHE128(0, &pFLoc[*piFLoc]);

									vec128 dist1 = M_VDp4x4(lvp, pP[pF0->m_iPlane].v, lvp, pP[pF1->m_iPlane].v, lvp, pP[pF2->m_iPlane].v, lvp, pP[pF3->m_iPlane].v);
									vec128 dist2 = M_VDp4x4(lvp, pP[pF4->m_iPlane].v, lvp, pP[pF5->m_iPlane].v, lvp, pP[pF6->m_iPlane].v, lvp, pP[pF7->m_iPlane].v);
									*((vec128*M_RESTRICT)&lFaceDist[flocal]) = dist1;
									*((vec128*M_RESTRICT)&lFaceDist[flocal+4]) = dist2;
									flocal += 8;
									pF0 = pF8; pF1 = pF9; pF2 = pF10; pF3 = pF11;
									pF4 = pF12; pF5 = pF13; pF6 = pF14; pF7 = pF15;
								}
								if(nFacesInt-fdist >= 8)
								{
									vec128 dist1 = M_VDp4x4(lvp, pP[pF0->m_iPlane].v, lvp, pP[pF1->m_iPlane].v, lvp, pP[pF2->m_iPlane].v, lvp, pP[pF3->m_iPlane].v);
									vec128 dist2 = M_VDp4x4(lvp, pP[pF4->m_iPlane].v, lvp, pP[pF5->m_iPlane].v, lvp, pP[pF6->m_iPlane].v, lvp, pP[pF7->m_iPlane].v);
									*((vec128*M_RESTRICT)&lFaceDist[flocal]) = dist1;
									*((vec128*M_RESTRICT)&lFaceDist[flocal+4]) = dist2;
									flocal += 8;
									fdist+=8;
								}
								for(; nFacesInt-fdist >= 4; fdist+=4)
								{
									uint iFace0 = piFaces[fdist];
									uint iFace1 = piFaces[fdist+1];
									uint iFace2 = piFaces[fdist+2];
									uint iFace3 = piFaces[fdist+3];
									const CBSP2_Face& Face0 = pF[iFace0];
									const CBSP2_Face& Face1 = pF[iFace1];
									const CBSP2_Face& Face2 = pF[iFace2];
									const CBSP2_Face& Face3 = pF[iFace3];
									vec128 dist = M_VDp4x4(lvp, pP[Face0.m_iPlane].v, lvp, pP[Face1.m_iPlane].v, lvp, pP[Face2.m_iPlane].v, lvp, pP[Face3.m_iPlane].v);
									*((vec128*M_RESTRICT)&lFaceDist[flocal]) = dist;
									flocal += 4;
								}
							}
							for(; fdist < nFacesInt; fdist++)
							{
								lFaceDist[flocal++] = pP[pF[piFaces[fdist]].m_iPlane].Distance(CurLocalVP);
							}
						}

						
						for(uint flocal = 0; f < nFacesInt; f++, flocal++)
						{
							int iFace = piFaces[f];
							const CBSP2_Face& Face = pF[iFace];

//							if (pP[Face.m_iPlane].Distance(CurLocalVP) > 0.0f)
							if (!(((uint32*)lFaceDist)[flocal] & 0x8000000))
							{
								int iVB = Face.m_iVB;
								if (iVB != iVBCurrent)
								{
									if (iVBCurrent == -1)
										iVBCurrent = iVB;
									else
										goto CommitPrimitives;
								}

								int nv = Face.m_nVertices-2;
								int iv = Face.m_iiVBVertices;

								nTri += nv;
								if (nTri > MaxTri)
								{
									nTri -= nv;
									goto CommitPrimitives;
								}

								CreateFanTriangles(piPrim+nP, iv, nv);
								nP += nv*3;

								// Welcome to the LHS factory
	/*							for(int v = 0; v < nv; v++)
								{
									piPrim[nP++] = iv;
									piPrim[nP++] = iv+v+1;
									piPrim[nP++] = iv+v+2;
								}*/

				//				liVisFaces[nVisFaces++] = iFace;
							}
						}
					}

			CommitPrimitives:
					if (!nTri)
					{
						pVBM->Alloc_Close(0);
						continue;
					}

					pVBM->Alloc_Close(nP*2);
				}

//			int iVB = m_lFaces[piFaces[0]].m_iVB;


/*			CXR_VertexBuffer* pVB = pVBM->Alloc_VB();
			if (!pVB) 
				continue;*/

//				VB_CopyGeometry(_pRenderParams, iVBCurrent, &Chain);

			}

			const CBSP2_VBInfo* pBSPVBI = &pVBInfo[iVBCurrent];
			if (pBSPVBI->m_VBID != (uint16)~0)
			{
				CXR_VBIDChain* pChain = pChainHeap + nChainHeapUsed;
				pChain->m_pNextVB = NULL;
				pChain->m_VBID = pBSPVBI->m_VBID;
				pChain->m_piPrim = piPrim;
				pChain->m_nPrim = nTri;
				pChain->m_PrimType = CRC_RIP_TRIANGLES;
				pChain->m_TaskId = ~0;
				plShaderQueueVBChain[SLC.m_iSQ].AddChain(pChain);

				nChainHeapUsed++;
				if (nChainHeapUsed >= ChainHeapSize)
				{
					pChainHeap = (CXR_VBIDChain*) pVBM->Alloc(sizeof(CXR_VBIDChain) * ChainHeapSize);
					if (!pChainHeap)
						return;
					nChainHeapUsed = 0;
				}
			}
/*

			if (Chain.m_bVBIds)
			{
				CXR_VBIDChain *pChain = pVBM->Alloc_VBIDChain();
				if(!pChain)
					return;

				*pChain = *Chain.GetVBIDChain();
				pChain->m_piPrim = piPrim;
				pChain->m_nPrim = nTri;
				pChain->m_PrimType = CRC_RIP_TRIANGLES;
				plShaderQueueVBChain[SLC.m_iSQ].AddChain(pChain);
			}
			else
			{
				CXR_VBChain *pChain = pVBM->Alloc_VBChain();
				if(!pChain)
					return;

				*pChain = *Chain.GetVBChain();
				pChain->m_piPrim = piPrim;
				pChain->m_nPrim = nTri;
				pChain->m_PrimType = CRC_RIP_TRIANGLES;
				plShaderQueueVBChain[SLC.m_iSQ].AddChain(pChain);
			}*/
		}

		CScissorRect Scissor;
		CalcBoxScissor(m_pWorldView, SLCBoundBox, Scissor);
		plLightOcclusion[SLC.m_iLight].m_ScissorShaded.Expand(Scissor);

	}
}

#else

void CXR_Model_BSP2::Render_SLC(CBSP2_RenderParams*M_RESTRICT _pRenderParams, int _iPL, const CRC_ClipVolume* _pClip, int _iSLC, int _nSLC)
{
	if (!_nSLC)
		return;

	CBSP2_LightData* pLD = m_spLightData;
	if (!pLD)
		return;

	CBSP2_SceneGraph* pSGI = m_pWorldView->m_pSceneGraph;
	if (!pSGI)
		return;

	CXR_VBManager* pVBM = _pRenderParams->m_pVBM;

	M_LOCK(pSGI->m_Lock);
	M_ASSERT(pLD->m_lSLC.ValidPos(_iSLC), "!");

	TAP_RCD<uint32> pliSLCFaces(pLD->m_liSLCFaces);
	TAP_RCD<spCXW_Surface> plspSurfaces(m_lspSurfaces);
	TAP_RCD<CXR_Light> plLights(pSGI->m_lLights);
	TAP_RCD<CBSP2_SLCluster> plSLC(pLD->m_lSLC);
	TAP_RCD<CXR_LightOcclusionInfo> plLightOcclusion(m_pWorldView->m_lLightOcclusion);
	TAP_RCD<CBSP2_VBChain> plShaderQueueVBChain(_pRenderParams->m_pShaderQueueVBChain);

	for(int i = 0; i < _nSLC; i++)
	{
		const CBSP2_SLCluster& SLC = plSLC[i + _iSLC];
		if (!SLC.m_nFaces)
			continue;

		CBox3Dfp32 SLCBoundBox = SLC.m_BoundBox;
		{
			const CPlane3Dfp32* pPlanes = _pClip->m_Planes;

			int nPlanes = _pClip->m_nPlanes;
			fp32 Dist[4];
			Dist[0] = pPlanes[0].GetBoxMinDistance(SLCBoundBox.m_Min, SLCBoundBox.m_Max);
			Dist[1] = pPlanes[1].GetBoxMinDistance(SLCBoundBox.m_Min, SLCBoundBox.m_Max);
			Dist[2] = pPlanes[2].GetBoxMinDistance(SLCBoundBox.m_Min, SLCBoundBox.m_Max);
			Dist[3] = pPlanes[3].GetBoxMinDistance(SLCBoundBox.m_Min, SLCBoundBox.m_Max);

			fp32 d = Max(Max(Dist[0], Dist[1]), Max(Dist[2], Dist[3]));
			if(d > MODEL_BSP_EPSILON)
				continue;

			int p;
			for (p = 4; p < nPlanes; p++)
				if (pPlanes[p].GetBoxMinDistance(SLCBoundBox.m_Min, SLCBoundBox.m_Max) > MODEL_BSP_EPSILON) break;
			if (p < nPlanes) 
				continue;
		}

		if (SLC.m_iiFaces + SLC.m_nFaces > pliSLCFaces.Len())
			continue;
		if (SLC.m_iSurface >= plspSurfaces.Len())
			continue;
		if (SLC.m_iLight >= plLights.Len())
			continue;

		const uint32* piFaces = &pliSLCFaces[SLC.m_iiFaces];
		int nFaces = SLC.m_nFaces;

		//		const int MaxFaces = 2048;
		//		uint16 liVisFaces[MaxFaces];
		//		int nVisFaces = 0;
		const CBSP2_Face* pF = m_pFaces;
		const CPlane3Dfp32* pP = m_pPlanes;
		CVec3Dfp32 CurLocalVP = _pRenderParams->m_pViewData->m_CurLocalVP;


		int f = 0;
		while(f < nFaces)
		{
			// JK-NOTE: Not really valid to call GetAvail here since we don't own the alloclock
			int MaxTri = pVBM->GetAvail() >> 3;		// Well.. 4 is almost 3  =P
			if (MaxTri < (MODEL_BSP_MAXFACEVERTICES-2))
				return;
			CBSP2_VBChain Chain;
			uint16* piPrim = NULL;
			int nP = 0;
			int nTri = 0;

			int iVBCurrent = -1;
			{
				{
					M_LOCK(pVBM->m_AllocLock);
					piPrim = (uint16*)pVBM->Alloc_Open();
					if(!piPrim)
						return;
					for(; f < nFaces; f++)
					{
						int iFace = piFaces[f];
						const CBSP2_Face& Face = pF[iFace];

						if (pP[Face.m_iPlane].Distance(CurLocalVP) > 0.0f)
						{
							int iVB = Face.m_iVB;
							if (iVB != iVBCurrent)
							{
								if (iVBCurrent == -1)
									iVBCurrent = iVB;
								else
									goto CommitPrimitives;
							}

							int nv = Face.m_nVertices-2;
							int iv = Face.m_iiVBVertices;

							nTri += nv;
							if (nTri > MaxTri)
							{
								nTri -= nv;
								break;
							}

							for(int v = 0; v < nv; v++)
							{
								piPrim[nP++] = iv;
								piPrim[nP++] = iv+v+1;
								piPrim[nP++] = iv+v+2;
							}

							//				liVisFaces[nVisFaces++] = iFace;
						}
					}

CommitPrimitives:
					if (!nTri)
					{
						pVBM->Alloc_Close(0);
						continue;
					}

					pVBM->Alloc_Close(nP*2);
				}

				//			int iVB = m_lFaces[piFaces[0]].m_iVB;


				/*			CXR_VertexBuffer* pVB = pVBM->Alloc_VB();
				if (!pVB) 
				continue;*/

				VB_CopyGeometry(_pRenderParams, iVBCurrent, &Chain);
			}

			if (Chain.m_bVBIds)
			{
				CXR_VBIDChain *pChain = pVBM->Alloc_VBIDChain();
				if(!pChain)
					return;

				*pChain = *Chain.GetVBIDChain();
				pChain->m_piPrim = piPrim;
				pChain->m_nPrim = nTri;
				pChain->m_PrimType = CRC_RIP_TRIANGLES;
				plShaderQueueVBChain[SLC.m_iSQ].AddChain(pChain);
			}
			else
			{
				CXR_VBChain *pChain = pVBM->Alloc_VBChain();
				if(!pChain)
					return;

				*pChain = *Chain.GetVBChain();
				pChain->m_piPrim = piPrim;
				pChain->m_nPrim = nTri;
				pChain->m_PrimType = CRC_RIP_TRIANGLES;
				plShaderQueueVBChain[SLC.m_iSQ].AddChain(pChain);
			}
		}

		CScissorRect Scissor;
		CalcBoxScissor(m_pWorldView, SLCBoundBox, Scissor);
		plLightOcclusion[SLC.m_iLight].m_ScissorShaded.Expand(Scissor);

	}
}


#endif

M_FORCEINLINE vec128 M_VPlaneBoxMinDistance_x4(vec128 _p0, vec128 _p1, vec128 _p2, vec128p _p3, vec128p _boxmin, vec128p _boxmax)
{
	// Box must be { xmin, ymin, zmin, 1.0f } { xmax, ymax, zmax, 1.0f }
	vec128 z = M_VZero();
	vec128 box0comp = M_VSelMsk(M_VCmpGTMsk(_p0, z), _boxmin, _boxmax);
	vec128 box1comp = M_VSelMsk(M_VCmpGTMsk(_p1, z), _boxmin, _boxmax);
	vec128 box2comp = M_VSelMsk(M_VCmpGTMsk(_p2, z), _boxmin, _boxmax);
	vec128 box3comp = M_VSelMsk(M_VCmpGTMsk(_p3, z), _boxmin, _boxmax);
	vec128 dist = M_VDp4x4(box0comp, _p0, box1comp, _p1, box2comp, _p2, box3comp, _p3);
	return dist;
}

M_FORCEINLINE vec128 M_VPlaneBoxMinDistance(vec128 _p0, vec128p _boxmin, vec128p _boxmax)
{
	// Box must be { xmin, ymin, zmin, 1.0f } { xmax, ymax, zmax, 1.0f }
	vec128 z = M_VZero();
	vec128 box0comp = M_VSelMsk(M_VCmpGTMsk(_p0, z), _boxmin, _boxmax);
	vec128 dist = M_VDp4(box0comp, _p0);
	return dist;
}

M_FORCEINLINE vec128 M_VScissorExpand(vec128 _a, vec128 _b)
{
	vec128 vsel = M_VConstMsk(1,0,1,0);
	vec128 vmin = M_VMin_u16(_a, _b);
	vec128 vmax = M_VMax_u16(_a, _b);
	vec128 vret = M_VSelMsk(vsel, vmin, vmax);
	return vret;
}

M_FORCEINLINE void M_VBoxInVolume(vec128 _boxmin, vec128 _boxmax, const CPlane3Dfp32* _pPlanes, uint _nPlanes, uint32*M_RESTRICT _pbResult)
{
	// Box must be { xmin, ymin, zmin, 1.0f } { xmax, ymax, zmax, 1.0f }
	M_ASSERT(_nPlanes >= 4, "Unexpected plane count.");

	vec128 dist = M_VPlaneBoxMinDistance_x4(_pPlanes[0].v, _pPlanes[1].v, _pPlanes[2].v, _pPlanes[3].v, _boxmin, _boxmax);
	vec128 culled = M_VCmpGTMsk(dist, M_VZero());
	culled = M_VOr(M_VOr(M_VSplatX(culled), M_VSplatY(culled)), M_VOr(M_VSplatZ(culled), M_VSplatW(culled)));
	for (int p = 4; p < _nPlanes; p++)
		culled = M_VOr(culled, M_VCmpGTMsk(M_VPlaneBoxMinDistance(_pPlanes[p].v, _boxmin, _boxmax), M_VZero()));

	M_VStAny32(culled, _pbResult);
}

void CXR_Model_BSP2::Render_SLC2(CBSP2_RenderParams*M_RESTRICT _pRenderParams, int _iPL, const CRC_ClipVolume* _pClip, int _iSLC, int _nSLC)
{
	if (!_nSLC)
		return;

	uint32 lbSLCCulled[256];
	if (_nSLC > 256)
		_nSLC = 256;

	CBSP2_LightData* pLD = m_spLightData;
	if (!pLD)
		return;

	TAP_RCD<CBSP2_SLCluster> plSLC(pLD->m_lSLC);


	CBSP2_SceneGraph* pSGI = m_pWorldView->m_pSceneGraph;
	if (!pSGI)
		return;

	CXR_LocalVBManager* pVBM = &_pRenderParams->m_LocalVBM;

	M_ASSERT(pLD->m_lSLC.ValidPos(_iSLC), "!");

	TAP_RCD<uint32> pliSLCFaces(pLD->m_liSLCFaces);
	TAP_RCD<spCXW_Surface> plspSurfaces(m_lspSurfaces);
	TAP_RCD<CXR_Light> plLights(pSGI->m_lLights);
	TAP_RCD<CBSP2_SLCVBInfo> plSLCVBI(pLD->m_lSLCVBI);
	TAP_RCD<CXR_LightOcclusionInfo> plLightOcclusion(m_pWorldView->m_lLightOcclusion);
	TAP_RCD<CBSP2_VBChain> plShaderQueueVBChain(_pRenderParams->m_pShaderQueueVBChain);

	const CPlane3Dfp32* pPlanes = _pClip->m_Planes;
	int nPlanes = _pClip->m_nPlanes;

	M_PRECACHEMEM(&plSLCVBI[_iSLC], sizeof(CBSP2_SLCVBInfo)*_nSLC);

	uint i;
	for(i = 0; i < _nSLC; i++)
	{
		const CBSP2_SLCluster& SLC = plSLC[i + _iSLC];
		if (!SLC.m_nFaces)
		{
			lbSLCCulled[i] = 1;
			continue;
		}

//		CBox3Dfp32 SLCBoundBox = SLC.m_BoundBox;

		vec128 vboxmin = M_VLd_P3_Slow(&SLC.m_BoundBox.m_Min);
		vec128 vboxmax = M_VLd_P3_Slow(&SLC.m_BoundBox.m_Max);
		M_VBoxInVolume(vboxmin, vboxmax, pPlanes, nPlanes, &lbSLCCulled[i]);

#if 0
		{
//			fp32 Dist[4];
			vec128 dist = M_VPlaneBoxMinDistance_x4(pPlanes[0].v, pPlanes[1].v, pPlanes[2].v, pPlanes[3].v, vboxmin, vboxmax);
//			if (M_VCmpAnyGT(dist, M_VZero()))
//				continue;
			vec128 culled = M_VCmpGTMsk(dist, M_VZero());
			culled = M_VOr(M_VOr(M_VSplatX(culled), M_VSplatY(culled)), M_VOr(M_VSplatZ(culled), M_VSplatW(culled)));

			for (int p = 4; p < nPlanes; p++)
				culled = M_VOr(culled, M_VCmpGTMsk(M_VPlaneBoxMinDistance(pPlanes[p].v, vboxmin, vboxmax), M_VZero()));

			M_VStAny32(culled, &lbSLCCulled[i]);

/*			Dist[0] = pPlanes[0].GetBoxMinDistance(SLCBoundBox.m_Min, SLCBoundBox.m_Max);
			Dist[1] = pPlanes[1].GetBoxMinDistance(SLCBoundBox.m_Min, SLCBoundBox.m_Max);
			Dist[2] = pPlanes[2].GetBoxMinDistance(SLCBoundBox.m_Min, SLCBoundBox.m_Max);
			Dist[3] = pPlanes[3].GetBoxMinDistance(SLCBoundBox.m_Min, SLCBoundBox.m_Max);

			fp32 d = Max(Max(Dist[0], Dist[1]), Max(Dist[2], Dist[3]));
			if(d > MODEL_BSP_EPSILON)
				continue;

			int p;
			for (p = 4; p < nPlanes; p++)
				if (pPlanes[p].GetBoxMinDistance(SLCBoundBox.m_Min, SLCBoundBox.m_Max) > MODEL_BSP_EPSILON) break;
			if (p < nPlanes) 
				continue;*/
		}
#endif
	}

	uint nVis = 0;
	for(uint i = 0; i < _nSLC; i++)
	{
		if (!lbSLCCulled[i])
		{
			nVis++;
			const CBSP2_SLCluster& SLC = plSLC[i + _iSLC];
			M_PRECACHE128(0, &plShaderQueueVBChain[SLC.m_iSQ]);
		}
	}

	if (!nVis)
		return;

	uint8*M_RESTRICT pVBMem = (uint8*M_RESTRICT)pVBM->Alloc(nVis * sizeof(CXR_VBIDChain));
	if (!pVBMem)
		return;

	for(uint i = 0; i < _nSLC; i++)
	{
		if (lbSLCCulled[i])
			continue;

		const CBSP2_SLCluster& SLC = plSLC[i + _iSLC];
		const CBSP2_SLCVBInfo& SLCVBI = plSLCVBI[i + _iSLC];

		{
			CXR_VBIDChain*M_RESTRICT pChain = (CXR_VBIDChain*)pVBMem; pVBMem += sizeof(CXR_VBIDChain);
			pChain->m_pNextVB = NULL;
			pChain->m_piPrim = NULL;
			pChain->m_TaskId = ~0;
			pChain->Render_VertexBuffer_IndexBufferTriangles(SLCVBI.m_VBID, SLCVBI.m_IBID, SLCVBI.m_nPrimTri, SLCVBI.m_iPrimTri);
			plShaderQueueVBChain[SLC.m_iSQ].AddChain(pChain);
		}

//		CScissorRect Scissor;
		vec128 vboxmin = M_VLd_P3_Slow(&SLC.m_BoundBox.m_Min);
		vec128 vboxmax = M_VLd_P3_Slow(&SLC.m_BoundBox.m_Max);
		vec128 vscissor = CalcBoxScissor(m_pWorldView, vboxmin, vboxmax);
		CScissorRect*M_RESTRICT pScissor = &plLightOcclusion[SLC.m_iLight].m_ScissorShaded;
		M_VStAny64(M_VScissorExpand(vscissor, M_VLd64(pScissor)), pScissor);

//		plLightOcclusion[SLC.m_iLight].m_ScissorShaded.Expand(Scissor);
	}
}

void CXR_Model_BSP2::RenderPortalLeaf(CBSP2_RenderParams*M_RESTRICT _pRenderParams, int _iPL)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderPortalLeaf, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP2::RenderPortalLeaf); //AR-SCOPE

//	const CBSP2_PortalLeafExt* pPL = &m_lPortalLeaves[_iPL];
	const CBSP2_PortalLeaf_VBFP* pPLVBFP = &m_lPortalLeaves_VBFP[_iPL];

	int iRP = m_pWorldView->m_liLeafRPortals[_iPL];
	const CRC_ClipVolume* pClip = &m_pWorldView->m_lRPortals[iRP];
	M_PRECACHE128(0, pClip);
	m_nVVertTag = 0;
	m_nFogTags = 0;
//	m_iFaceQueue = 0;

#if 0
	_pRenderParams->m_pCurMedium = &m_lMediums[pPL->m_iMedium];
	int bFog = (_pRenderParams->m_pCurMedium->m_MediumFlags & XW_MEDIUM_FOG);
	if (_pRenderParams->m_pSceneFog && _pRenderParams->m_pSceneFog->NHFEnable())
	{
//		int bSwitch = (m_LastMedium ^ m_pCurMedium->m_MediumFlags) & XW_MEDIUM_FOG;
//		int bSwitch = (m_LastMedium | m_pCurMedium->m_MediumFlags) & XW_MEDIUM_FOG;
		int bSwitch = (_pRenderParams->m_pCurMedium->m_MediumFlags) & XW_MEDIUM_FOG;

		const int MAXFOGPORTALS = 32;
		uint32 iFogPortals[MAXFOGPORTALS];
		int nFogPortals = 0;
		if (bSwitch)
		{
			nFogPortals = Fog_GetVisiblePortals(_pRenderParams, _iPL, iFogPortals, MAXFOGPORTALS);
			if (!nFogPortals) bSwitch = false;
		}


//		if (m_pCurMedium->m_MediumFlags & XW_MEDIUM_FOG)
		if (bSwitch)
		{
			VB_RenderFaceQueues(_pRenderParams);
			VB_Flush(_pRenderParams);
			VB_RenderQueues(_pRenderParams);
			VB_ClearQueues(_pRenderParams);
			VB_ClearFaceQueues(_pRenderParams);
#ifndef PLATFORM_PS2
			_pRenderParams->m_BasePriority_Opaque += CXR_VBPRIORITY_PORTALLEAFOFFSET;
			_pRenderParams->m_BasePriority_Transparent += CXR_VBPRIORITY_PORTALLEAFOFFSET;
#endif
			if (bFog)
				Fog_RenderPortals(_iPL, iFogPortals, nFogPortals);
		}
	}
#endif

	// Precache VBFP
	{
		uint nVBFP = pPLVBFP->m_nVBFacePrims;
		if (nVBFP)
		{
			const CBSP2_VBFacePrim* pVBFP = &m_lVBFacePrim[pPLVBFP->m_iVBFacePrims];
			M_PRECACHEMEM(pVBFP, sizeof(CBSP2_VBFacePrim) * nVBFP);
/*			for(int i = 0; i < nVBFP; i += 3)
			{
				M_PRECACHE128(0, pVBFP+i);
			}*/
		}
	}
			
//	pPL->m_BasePriority = _pRenderParams->m_BasePriority_Opaque;

	if (m_spWMC != NULL) Wallmark_Render(_pRenderParams, m_spWMC, _iPL);
	if (m_spWMCTemp != NULL) Wallmark_Render(_pRenderParams, m_spWMCTemp, _iPL);
	if (m_spWMCStatic != NULL) Wallmark_Render(_pRenderParams, m_spWMCStatic, _iPL);


	CBSP2_LightData_Ext*M_RESTRICT pLD = m_spLightData;
	if (pLD)
	{
		uint nSVPL = (_iPL >= pLD->m_lnSVPL.Len()) ? 0 : pLD->m_lnSVPL[_iPL];
/*		const CBSP2_ShadowVolume* pSV = pLD->m_lSV.GetBasePtr();
		int nSVPL = (_iPL >= pLD->m_lnSVPL.Len()) ? 0 : pLD->m_lnSVPL[_iPL];
		int iSVFirstPL = (_iPL >= pLD->m_liSVFirstPL.Len()) ? 0 : pLD->m_liSVFirstPL[_iPL];
		M_PRECACHE128(0, pSV+iSVFirstPL);
		M_PRECACHE128(128, pSV+iSVFirstPL);*/
		if (nSVPL)
		{
			const CBSP2_ShadowVolume_Thin* pSVThin = pLD->m_lSVThin.GetBasePtr();
			uint Size = nSVPL * sizeof(CBSP2_ShadowVolume_Thin);
			uint iSVFirst = pLD->m_liSVFirstPL[_iPL];
			M_PRECACHEMEM(pSVThin + iSVFirst, Size);
		}

	}

	{
		uint nVBFP = pPLVBFP->m_nVBFacePrims;
		if (nVBFP)
		{
			const CBSP2_VBFacePrim* pVBFP = &m_lVBFacePrim[pPLVBFP->m_iVBFacePrims];
			uint16* pFacePrim = m_lFacePrim.GetBasePtr();
			for(int i = 0; i < nVBFP; i++)
			{
				VB_AddPrimitives_LocalVBM(_pRenderParams, pVBFP->m_iVB, &pFacePrim[pVBFP->m_iPrim], pVBFP->m_nPrim);
				pVBFP++;
			}
		}
	}

	while(pLD)
	{
		uint nSVPL = (_iPL >= pLD->m_lnSVPL.Len()) ? 0 : pLD->m_lnSVPL[_iPL];
		if (!nSVPL)
			break;

//		M_LOCK(m_pWorldView->m_pSceneGraph->m_Lock); This lock is taken in RenderLeafList

		//		int nSV = pLD->m_lSV.Len();
//		TAP<const CBSP2_ShadowVolume> pSV = pLD->m_lSV;
//		const CBSP2_ShadowVolume* pSVThick = pLD->m_lSV.GetBasePtr();
		TAP<CBox3Dfp32> pSVBoundBoxSV = pLD->m_lSVBoundBoxSV;
		const CBSP2_ShadowVolume_Thin* pSVThin = pLD->m_lSVThin.GetBasePtr();
		const CBSP2_LightData::CLightDataVBID *pVBIds = pLD->m_lVBIds.GetBasePtr();
		int MaxLights = m_pWorldView->m_lLightOcclusion.Len();
		CXR_LightOcclusionInfo*M_RESTRICT pLO = m_pWorldView->m_lLightOcclusion.GetBasePtr();
		TAP<const CXR_Light> pLights = m_pWorldView->m_pSceneGraph->m_lLights;
		TAP<CScissorRect> pPortalScissor = m_pWorldView->m_lPortalScissor;
		TAP_RCD<CBSP2_SLCluster> plSLC(pLD->m_lSLC);

//		int ColorWriteDisable = (_pRenderParams->m_pCurrentEngine->m_DebugFlags & M_Bit(12)) ? 0 : CRC_FLAGS_COLORWRITE;

		uint iSVFirstPL = (_iPL >= pLD->m_liSVFirstPL.Len()) ? 0 : pLD->m_liSVFirstPL[_iPL];

		const uint MaxSV = 256;
		uint32 lbSVCulled[MaxSV];
		if (nSVPL > MaxSV)
		{
			M_TRACEALWAYS("(CXR_Model_BSP2::RenderPortalLeaf) Too many SV in PL. Need code fix. (%d > %d)\n", nSVPL, MaxSV);
			nSVPL = MaxSV;
		}
		uint32 liSVRenderSLC[MaxSV];
		uint nSVRenderSLC = 0;

		for(uint i = 0; i < nSVPL; i++)
		{
//			M_PRECACHE128(0, pSV+iSVFirstPL+2);
			uint iSV = i + iSVFirstPL;
			const CBSP2_ShadowVolume_Thin& SVThin = pSVThin[iSV];

			lbSVCulled[i] = 1;
			if (SVThin.m_iPL == _iPL)
			{
				int iLight = SVThin.m_iLight;
				if (iLight >= MaxLights || !pLO[iLight].IsVisible())
					continue;

				if (!(pLights[iLight].m_Flags & CXR_LIGHT_ENABLED))
					continue;

				if (!SVThin.m_nSLC)
				{
					M_ASSERT(m_pWorldView->m_lActiveSV.Len() << 3 >= pLD->m_lSV.Len(), "!");
					if (!(m_pWorldView->m_lActiveSV[iSV >> 3] & M_BitD(iSV & 7)))
						continue;
				}

				if (SVThin.m_iSLC >= pLD->m_lSLC.Len())
					continue;

				liSVRenderSLC[nSVRenderSLC++] = iSV;

				{
					void* pPrecache = &plSLC[SVThin.m_iSLC];
					M_PRECACHEMEM(pPrecache, SVThin.m_nSLC*sizeof(CBSP2_SLCluster));
				}

				if (pVBIds[iSV].m_VBID == -1)
					continue;

				lbSVCulled[i] = 0;
				M_PRECACHE128(0, &pSVBoundBoxSV[iSV]);

/*
				const CBox3Dfp32& SVBox = pSV[iSV].m_BoundBoxSV;
				int p;
				for (p = 0; p < pClip->m_nPlanes; p++)
					if (pClip->m_Planes[p].GetBoxMinDistance(SVBox.m_Min, SVBox.m_Max) > MODEL_BSP_EPSILON) break;
				if (p != pClip->m_nPlanes) 
					continue;*/
			}
		}

		{
			for(uint i = 0; i < nSVPL; i++)
			{
				if (!lbSVCulled[i])
				{
					uint iSV = i + iSVFirstPL;
					vec128 vboxmin = M_VLd_P3_Slow(&pSVBoundBoxSV[iSV].m_Min);
					vec128 vboxmax = M_VLd_P3_Slow(&pSVBoundBoxSV[iSV].m_Max);
					M_VBoxInVolume(vboxmin, vboxmax, pClip->m_Planes, pClip->m_nPlanes, &lbSVCulled[i]);
				}
			}
		}

		if (_pRenderParams->m_pCurrentEngine->m_BSPDebugFlags & 1)
		{
			for(uint j = 0; j < nSVRenderSLC; j++)
			{
				uint iSV = liSVRenderSLC[j];
				Render_SLC(_pRenderParams, _iPL, pClip, pSVThin[iSV].m_iSLC, pSVThin[iSV].m_nSLC);
			}
		}
		else
		{
			for(uint j = 0; j < nSVRenderSLC; j++)
			{
				uint iSV = liSVRenderSLC[j];
				Render_SLC2(_pRenderParams, _iPL, pClip, pSVThin[iSV].m_iSLC, pSVThin[iSV].m_nSLC);
			}
		}


		int bShadows = !(_pRenderParams->m_pCurrentEngine->m_DebugFlags & M_Bit(13));
		if (bShadows)
		{
			uint i;
			uint nVis = 0;
			for(i = 0; i < nSVPL; i++)
			{
				if (!lbSVCulled[i])
					nVis++;
			}
			if (nVis)
			{
				uint bSepStencil = _pRenderParams->m_CurrentRCCaps & CRC_CAPS_FLAGS_SEPARATESTENCIL;
				uint VBMemSize = nVis*(sizeof(CXR_VertexBuffer) + sizeof(CXR_VBIDChain) + sizeof(CXR_VirtualAttributes_BSP2ShadowVolumeFront));
				if (!bSepStencil)
					VBMemSize *= 2;

				uint8*M_RESTRICT pVBMem = (uint8*M_RESTRICT)_pRenderParams->m_LocalVBM.Alloc(VBMemSize);
				if (!pVBMem)
					return;
				uint8*M_RESTRICT pVBMemStart = pVBMem;

				for(i = 0; i < nSVPL; i++)
				{
					uint iSV = i + iSVFirstPL;

					if (lbSVCulled[i])
						continue;

					// Safety thingy
	#if 0
					if (pSV[iSV].m_iTriBase + pSV[iSV].m_nTriangles > pLD->m_liSVPrim.Len())
					{
						ConOut(CStrF("PL %d, SV %d, Invalid triangle references", _iPL, iSV));
						continue;
					}
	#endif

					{
						CXR_VertexBuffer*M_RESTRICT pVB = _pRenderParams->m_LocalVBM.Construct_VB_IDChain(pVBMem, pVBMem+sizeof(CXR_VertexBuffer));
						pVBMem += sizeof(CXR_VertexBuffer) + sizeof(CXR_VBIDChain);

						pVB->Matrix_Set(_pRenderParams->m_pVBMatrixM2V);
						pVB->Geometry_Color(0xff100010);

						uint iLight = pSVThin[iSV].m_iLight;

						CScissorRect*M_RESTRICT pDestScissor = &pLO[iLight].m_ScissorShadow;
						M_VStAny64(M_VScissorExpand(M_VLd64(pDestScissor), M_VLd64(&pPortalScissor[iRP])), pDestScissor);

//						pLO[iLight].m_ScissorShadow.Expand(m_pWorldView->m_lPortalScissor[iRP]);

						pVB->Render_VertexBuffer(pVBIds[iSV].m_VBID);
						pVB->m_Priority = pLights[iLight].m_iLightf + 0.001f;
						pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
						pVB->m_iLight = iLight;
						pVB->SetVBEColor(0xff304040);

						CXR_VirtualAttributes_BSP2ShadowVolumeFront *M_RESTRICT pVA = new((void*)pVBMem) CXR_VirtualAttributes_BSP2ShadowVolumeFront;
						pVBMem += sizeof(CXR_VirtualAttributes_BSP2ShadowVolumeFront);
						pVA->Create();
						pVA->m_Scissor = m_pWorldView->m_lPortalScissor[iRP];
						pVB->SetVirtualAttr(pVA);

						_pRenderParams->m_LocalVBM.AddVB(pVB);
					}
				}

				if(!bSepStencil)
				{
					for(i = 0; i < nSVPL; i++)
					{
						uint iSV = i + iSVFirstPL;

						if (lbSVCulled[i])
							continue;

						{
							CXR_VertexBuffer*M_RESTRICT pVB = _pRenderParams->m_LocalVBM.Construct_VB_IDChain(pVBMem, pVBMem+sizeof(CXR_VertexBuffer));
							pVBMem += sizeof(CXR_VertexBuffer) + sizeof(CXR_VBIDChain);

							pVB->Matrix_Set(_pRenderParams->m_pVBMatrixM2V);
							pVB->Geometry_Color(0xff100010);

							void* pVAMem = _pRenderParams->m_LocalVBM.Alloc(sizeof(CXR_VirtualAttributes_BSP2ShadowVolumeBack));
							if(!pVAMem)
								return;
					
							CXR_VirtualAttributes_BSP2ShadowVolumeBack *M_RESTRICT pVA = new((void*)pVBMem) CXR_VirtualAttributes_BSP2ShadowVolumeBack;
							pVBMem += sizeof(CXR_VirtualAttributes_BSP2ShadowVolumeFront);
					
							pVA->Create();
							pVA->m_Scissor = m_pWorldView->m_lPortalScissor[iRP];
							pVB->SetVirtualAttr(pVA);

							uint iLight = pSVThin[iSV].m_iLight;
							pLO[iLight].m_ScissorShadow.Expand(m_pWorldView->m_lPortalScissor[iRP]);

							pVB->Render_VertexBuffer(pVBIds[iSV].m_VBID);
							pVB->m_Priority = pLights[iLight].m_iLightf + 0.002f;
							pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
							pVB->m_iLight = iLight;

							pVB->SetVBEColor(0xff304040);
							_pRenderParams->m_LocalVBM.AddVB(pVB);
						}
					}
				}

				M_ASSERT((pVBMem - pVBMemStart) == VBMemSize, "VB memory missmatch.");
			}
		}
		break;
	}

//#endif 	// VBQUEUES
}

#ifdef PLATFORM_XENON
#include "xtl.h"
#include "tracerecording.h"
#endif

void CXR_Model_BSP2::RenderLeafList(CBSP2_RenderParams* _pRenderParams)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderLeafList, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP2::RenderLeafList);
	M_NAMEDEVENT("RenderLeafList", 0xffff8000);

	CBSP2_SceneGraph* pSGI = m_pWorldView->m_pSceneGraph;
	if (!pSGI)
		return;
	M_LOCK(pSGI->m_Lock);

	_pRenderParams->m_LocalVBM.Begin(_pRenderParams->m_pVBM, 16384);

	int ColorWrite = _pRenderParams->m_pCurrentEngine->m_DebugFlags & M_Bit(12);
	int bIsMirrored = _pRenderParams->m_pCurrentEngine->GetVC()->m_bIsMirrored;
	CXR_VirtualAttributes_BSP2ShadowVolumeFront::PrepareFrame(_pRenderParams->m_pVBM, _pRenderParams->m_CurrentRCCaps & CRC_CAPS_FLAGS_SEPARATESTENCIL, ColorWrite, bIsMirrored);
	CXR_VirtualAttributes_BSP2ShadowVolumeBack::PrepareFrame(_pRenderParams->m_pVBM, ColorWrite, bIsMirrored);


	const uint16* piVisPortalLeaves = m_pView->m_liVisPortalLeaves.GetBasePtr();

#ifdef PLATFORM_XENON
	static volatile uint ms_bDoTrace = false;
	uint bTrace = ms_bDoTrace;
	if (bTrace)
		XTraceStartRecording( "cache:\\TraceRecord_RenderLeafList.bin" );

	CMTime T;
	TStart(T);
#endif
	_pRenderParams->m_BasePriority_Opaque = CXR_VBPRIORITY_MODEL_OPAQUE;
	_pRenderParams->m_BasePriority_Transparent = CXR_VBPRIORITY_MODEL_TRANSPARENT;

//	TAP_RCD<CBSP2_PortalLeafExt> lPortalLeaves = m_lPortalLeaves;
	TAP_RCD<CBSP2_PortalLeaf_VBFP> lPortalLeaves_VBFP = m_lPortalLeaves_VBFP;


	uint DebugFlags = _pRenderParams->m_pCurrentEngine->m_BSPDebugFlags;

	int l = m_pView->m_nVisLeaves-1;
	if (DebugFlags & 2)
		l = Min(l, 1);
	if (DebugFlags & 4)
		l = Min(l, 0);

	while(l >= 1)
	{
		M_PRECACHE128(0, &lPortalLeaves_VBFP[piVisPortalLeaves[l-1]]);
		RenderPortalLeaf(_pRenderParams, piVisPortalLeaves[l]);
		l--;
	}
	while(l >= 0)
	{
		RenderPortalLeaf(_pRenderParams, piVisPortalLeaves[l]);
		l--;
	}

//	for(int l = m_pView->m_nVisLeaves-1; l >= 0; l--)
//		RenderPortalLeaf(_pRenderParams, piVisPortalLeaves[l]);

#ifdef PLATFORM_XENON
	TStop(T);
	if (bTrace)
	{
		XTraceStopRecording();
		ms_bDoTrace = false;
	}
//	M_TRACEALWAYS("RenderLeafList: %.3f ms, %d leaves.\n", T.GetTime() * 1000.0, m_pView->m_nVisLeaves);
#endif

	_pRenderParams->m_LocalVBM.End();
}

bool CXR_Model_BSP2::VB_AllocateFaceQueues(CBSP2_RenderParams& _RenderParams)
{
	// Allocate and initialize face queues
	_RenderParams.m_pVBFaceQueue = (CBSP2_VBFaceQueue*)_RenderParams.m_pVBM->Alloc(sizeof(CBSP2_VBFaceQueue) * m_lVBFaceQueues.Len());
	_RenderParams.m_pFaceQueueHeap = _RenderParams.m_pVBM->Alloc_Int32(m_lFaces.Len());
	if(!_RenderParams.m_pVBFaceQueue || !_RenderParams.m_pFaceQueueHeap)
		return false;

	TAP_RCD<const CBSP2_VBFaceQueueData> lpVBFaceQueues(m_lVBFaceQueues);
	uint nQ = lpVBFaceQueues.Len();
	uint Pos = 0;
	for(uint i = 0; i < nQ; i++)
	{
		if(lpVBFaceQueues[i].m_MaxElem > 0)
		{
			CBSP2_VBFaceQueue& VBFaceQueue = _RenderParams.m_pVBFaceQueue[i];
			VBFaceQueue.m_MaxElem = lpVBFaceQueues[i].m_MaxElem;
			VBFaceQueue.m_nElem = 0;
			VBFaceQueue.m_pQueue = &_RenderParams.m_pFaceQueueHeap[Pos];
			Pos += VBFaceQueue.m_MaxElem;
		}
	}
	if(Pos != m_lFaces.Len())
		Error_static("CXR_Model_BSP2::VB_AllocateFaceQueues", "Internal error");
	return true;
}

bool CXR_Model_BSP2::VB_AllocateSurfaceQueues(CBSP2_RenderParams& _RenderParams)
{
	// Allocate and initialize suerface queues

	_RenderParams.m_pSurfQueues = (CBSP2_SurfaceQueueData*)_RenderParams.m_pVBM->Alloc(sizeof(CBSP2_SurfaceQueueData) * m_lSurfQueues.Len());
	if(!_RenderParams.m_pSurfQueues)
		return false;
	TAP_RCD<const CBSP2_SurfaceQueue> lpSurfQueues(m_lSurfQueues);
	uint nQ = lpSurfQueues.Len();
	for(uint i = 0; i < nQ; i++)
	{
		CBSP2_SurfaceQueueData& SurfQueue = _RenderParams.m_pSurfQueues[i];
		SurfQueue.Create(lpSurfQueues[i]);
		SurfQueue.m_VBChain.Clear();
	}
	return true;
}

bool CXR_Model_BSP2::VB_AllocatePrimitiveQueues(CBSP2_RenderParams& _RenderParams)
{
	// Allocate and initialize VB primitive queues
	int Len = m_lVBInfo.Len();
	CBSP2_VBPrimQueue* pPQ = (CBSP2_VBPrimQueue*)_RenderParams.m_pVBM->Alloc(sizeof(CBSP2_VBPrimQueue) * Len);
	if (!pPQ)
		return false;

	_RenderParams.m_pVBPrimQueue.Set(pPQ, Len);
	TAP_RCD<CBSP2_VBInfo> pVBI(m_lVBInfo);
	for(int i = 0; i < Len; i++)
	{
		pPQ[i].Create(pVBI[i]);
	}
	return true;
}



void CXR_Model_BSP2::UntagVertices()
{
	MAUTOSTRIP(CXR_Model_BSP_UntagVertices, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_nVVertTag; i++)
	{
		int iv = m_piVVertTag[i];
		m_pVVertMask[iv] = 0;
	}
	m_nVVertTag = 0;
}

void CXR_Model_BSP2::UntagFogVertices()
{
	MAUTOSTRIP(CXR_Model_BSP_UntagFogVertices, MAUTOSTRIP_VOID);
	int8* pFogTags = m_lFogTags.GetBasePtr();
	uint32* piFogTags = m_liFogTags.GetBasePtr();
	
	for(int i = 0; i < m_nFogTags; i++)
	{
		int iv = piFogTags[i];
		pFogTags[iv] = 0;
	}
	m_nFogTags = 0;
}

void CXR_Model_BSP2::RenderWire(CBSP2_RenderParams* _pRenderParams, CPixel32 _Color)
{
	const CBSP_Edge* pE = m_lEdges.GetBasePtr();
	int nE = m_lEdges.Len();

	const CVec3Dfp32* pV = m_lVertices.GetBasePtr();

	CRC_Attributes* pA = _pRenderParams->m_pVBM->Alloc_Attrib();
	if (!pA)
		return;

	pA->SetDefault();
	pA->Attrib_Disable(CRC_FLAGS_ZWRITE);

	int iE = 0;
	while(iE < nE)
	{
		const int BytesPerEdge = 12*2 + 2*2;
		int nBatch = Min(32767, nE-iE);

		uint8* pData = (uint8*)_pRenderParams->m_pVBM->Alloc(nBatch * BytesPerEdge + sizeof(CXR_VertexBuffer));
		if (!pData)
			return;

		uint8* pD = pData;
		CXR_VertexBuffer* pVB = (CXR_VertexBuffer*)pD;	pD += sizeof(CXR_VertexBuffer);
		CVec3Dfp32* pVE = (CVec3Dfp32*)pD; pD += sizeof(CVec3Dfp32)*2*nBatch;
		uint16* pPrim = (uint16*)pD;

		pVB->Construct();
		if (!pVB->AllocVBChain(_pRenderParams->m_pVBM, false))
			return;

		CXR_VBChain* pVBC = pVB->GetVBChain();
		pVBC->m_pV = pVE;
		pVBC->m_nV = 2*nBatch;
		pVB->m_Color = _Color;
		pVBC->Render_IndexedWires(pPrim, nBatch*2);
		pVB->m_pAttrib = pA;
		pVB->Matrix_Set(_pRenderParams->m_pVBMatrixM2V);

		CVec3Dfp32* pDestV = pVE;
		for(int i = 0; i < nBatch; i++)
		{
			*pDestV = pV[pE[iE].m_iV[0]]; pDestV++;
			*pDestV = pV[pE[iE].m_iV[1]]; pDestV++;
			pPrim[i*2] = i*2;
			pPrim[i*2+1] = i*2+1;
			iE++;
		}

		_pRenderParams->m_pVBM->AddVB(pVB);
	}

	{
		CBSP2_Face* pF = m_lFaces.GetBasePtr();
		int nF = m_lFaces.Len();

		int nVBE = 0;
		for(int iFace = 0; iFace < nF; iFace++)
		{
			if (pF[iFace].m_iiEdges == 0xffffff)
			{
				int nV = pF[iFace].m_nVertices;
				nVBE += nV;
			}
		}

		if(nVBE > 32767)					// Will render nothing when out of space
			return;

		int nAlloc = sizeof(CXR_VertexBuffer) + nVBE * (2 * sizeof(uint16) + sizeof(CVec3Dfp32));
		uint8* pVBData = (uint8*)_pRenderParams->m_pVBM->Alloc(nAlloc);
		if (!pVBData)
			return;

		CXR_VertexBuffer* pVB = (CXR_VertexBuffer*)pVBData; pVBData += sizeof(CXR_VertexBuffer);
		CVec3Dfp32* pVE = (CVec3Dfp32*)pVBData; pVBData += nVBE * sizeof(CVec3Dfp32);
		uint16* pPrim = (uint16*)pVBData; pVBData += nVBE * 2 * sizeof(uint16);

		const uint32* piVertices = m_piVertices;

		int iE = 0;
		for(int iFace = 0; iFace < nF; iFace++)
		{
			if (pF[iFace].m_iiEdges == 0xffffff)
			{
				int nV = pF[iFace].m_nVertices;
				int iiV = pF[iFace].m_iiVertices;
				const uint32* piV = &piVertices[iiV];

				uint16 iV0 = iE+nV-1;
				for(int v = 0; v < nV; v++)
				{
					pVE[iE] = pV[piV[v]];
					uint16 iV1 = iE;
					pPrim[iE*2 + 0] = iV0;
					pPrim[iE*2 + 1] = iV1;
					iV0 = iV1;
					iE++;
				}
			}
		}


		pVB->Construct();
		if (!pVB->AllocVBChain(_pRenderParams->m_pVBM, false))
			return;

		CXR_VBChain* pVBC = pVB->GetVBChain();
		pVBC->m_pV = pVE;
		pVBC->m_nV = iE;
		pVB->m_Color = _Color;
		pVBC->Render_IndexedWires(pPrim, iE*2);
		pVB->m_pAttrib = pA;
		pVB->Matrix_Set(_pRenderParams->m_pVBMatrixM2V);
		_pRenderParams->m_pVBM->AddVB(pVB);
	}
}

#ifdef M_Profile

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function: Counts Non-dynamic lights per face
Parameters:
	_pRenderParams:	Render parameters
Comments: Basically just copied from RenderLeafList, RenderPortalLeaf
		  and Render_SLC
\*____________________________________________________________________*/
void CXR_Model_BSP2::LightCountSLC(CBSP2_RenderParams * _pRenderParams)
{
	TAP_RCD<uint8> pFaceLights = m_lFaceLights;

	for(int k = m_pView->m_nVisLeaves-1; k >= 0; k--)
	{
		int iVPL = m_pView->m_liVisPortalLeaves[k];
		CBSP2_LightData* pLD = m_spLightData;

		const CBSP2_ShadowVolume* pSV = pLD->m_lSV.GetBasePtr();
		int MaxLights = m_pView->m_lLightOcclusion.Len();
		const CXR_LightOcclusionInfo* pLO = m_pView->m_lLightOcclusion.GetBasePtr();
		
		int nSVPL = (iVPL >= pLD->m_lnSVPL.Len()) ? 0 : pLD->m_lnSVPL[iVPL];
		int iSVFirstPL = (iVPL >= pLD->m_liSVFirstPL.Len()) ? 0 : pLD->m_liSVFirstPL[iVPL];
		const CXR_Light* pLights = m_pView->m_pSceneGraph->m_lLights.GetBasePtr();

		for(int l = iSVFirstPL;l < iSVFirstPL + nSVPL; l++)
		{
			if( (pSV[l].m_iPL != iVPL) || (pSV[l].m_iLight >= MaxLights) ||
				(!pLO[pSV[l].m_iLight].IsVisible()) || (pSV[l].m_iSLC >= pLD->m_lSLC.Len()) ||
				(pLights[pSV[l].m_iLight].GetIntensitySqrf() < 0.0001f) ) continue;

			CBSP2_SLCluster * pCls = m_spLightData->m_lSLC.GetBasePtr();
			uint32 * piFcs = m_spLightData->m_liSLCFaces.GetBasePtr();
			for(int i = 0;i < pSV[l].m_nSLC;i++)
			{
				CBSP2_SLCluster &Cl = pCls[i+ pSV[l].m_iSLC];
				uint32 * piF = &piFcs[Cl.m_iiFaces];
				for(int j = 0;j < Cl.m_nFaces;j++)
				{
					pFaceLights[piF[j]] ++;
				}
			}
		}
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function: Render a collection of faces with a color key
Parameters:
	_pRenderParams:	Render parameters
	_pFaces:		Facelist
	_nFaces:		Length of facelist
	_pAttr:			Attributes
	_iVB:			Index of VB
	_Color:			Color of facecollection
\*____________________________________________________________________*/
void CXR_Model_BSP2::RenderLightColorKeyFaceList(CBSP2_RenderParams * _pRenderParams,uint32 * _pFaces,
												 uint32 _nFaces,CRC_Attributes * _pAttr,int16 _iVB,CPixel32 _Color)
{
	CXR_VBManager * pVBM = _pRenderParams->m_pVBM;

	int nV,nP;
	RenderGetPrimCount(_pFaces,_nFaces,nV,nP);

	uint16 * pPrim = pVBM->Alloc_Int16(nP);
	CXR_VertexBuffer * pVB = pVBM->Alloc_VB();
	if(!pPrim || !pVB)
		return;
	int nPrim2 = RenderTesselatePrim(_pFaces,_nFaces,pPrim);
//	pVB = new(pVB) CXR_VertexBuffer;

	CBSP2_VBChain Chain;
	VB_CopyGeometry(_pRenderParams, _iVB, &Chain);
	Chain.SetToVB(pVB);

	pVB->m_pAttrib = _pAttr;
	pVB->m_Color = _Color;
	pVB->Matrix_Set(_pRenderParams->m_pVBMatrixM2V);
	pVB->Render_IndexedPrimitives(pPrim,nPrim2);
	pVB->m_Priority = CXR_VBPRIORITY_MODEL_TRANSPARENT + 1.0f;

	pVBM->AddVB(pVB);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function: Render Color-keyed lightcount information
Parameters:
	_pRenderParams:	Render parameters
Comments: Might be able to optimize this more if a more clever grouping
		  function for VBs is devised. if lColors is moved somewhere
		  else, also update in WTriMesh.cpp, 
		  CXR_Model_TriangleMesh::VB_RenderUnified
\*____________________________________________________________________*/
void CXR_Model_BSP2::RenderLightColorKey(CBSP2_RenderParams * _pRenderParams)
{
	const CPixel32 lColors[BSP_MAX_LIGHTS] = {
		CPixel32(0x00,0x00,0xFF,0x80),CPixel32(0x00,0x80,0x80,0x80),
		CPixel32(0x00,0xFF,0x00,0x80),CPixel32(0x80,0xFF,0x00,0x80),
		CPixel32(0xFF,0xFF,0x00,0x80),CPixel32(0xFF,0x80,0x00,0x80),
		CPixel32(0xFF,0x00,0x00,0x80),CPixel32(0xFF,0x00,0xFF,0x80),
		CPixel32(0x80,0x80,0x80,0x80),CPixel32(0xFF,0xFF,0xFF,0x80)
	};

	/*
	lColors[0] = CPixel32(0x00,0x00,0xFF,0x80);
	lColors[1] = CPixel32(0x00,0x80,0x80,0x80);
	lColors[2] = CPixel32(0x00,0xFF,0x00,0x80);
	lColors[3] = CPixel32(0x40,0xA0,0x00,0x80);
	lColors[4] = CPixel32(0xFF,0xFF,0x00,0x80);
	lColors[5] = CPixel32(0xA0,0x40,0x00,0x80);
	lColors[6] = CPixel32(0xFF,0x00,0x00,0x80);
	lColors[7] = CPixel32(0xFF,0x00,0xFF,0x80);
	lColors[8] = CPixel32(0xFF,0xFF,0xFF,0x80);
	*/

	CVec3Dfp32* pV = m_lVertices.GetBasePtr();
	CXR_VBManager * pVBM = _pRenderParams->m_pVBM;

	CRC_Attributes* pA = _pRenderParams->m_pVBM->Alloc_Attrib();
	if (!pA)
		return;

	pA->SetDefault();
	pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	pA->Attrib_Enable(CRC_TEXENVMODE_BLEND);
	pA->Attrib_SourceBlend(CRC_BLEND_SRCALPHA);
	pA->Attrib_DestBlend(CRC_BLEND_INVSRCALPHA);
	pA->Attrib_AlphaCompare(CRC_COMPARE_ALWAYS,0);

	uint8 *pFL = m_lFaceLights.GetBasePtr();

	uint32 liFaces[1000];
	uint8 lnFaces[10] = {0};
	int16 liVB[10];
	
	uint32 i;
	for(i = 0;i < m_lFaces.Len();i++)
	{
		if( pFL[i] == 0 ) continue;
		uint8 iClr = pFL[i]-1;

		//No batching of faces with 10+ lights
		if( iClr > 9 )
		{
			RenderLightColorKeyFaceList(_pRenderParams,&i,1,pA,m_lFaces[i].m_iVB,lColors[iClr]);
			continue;
		}

		//Render full buffers
		if( (lnFaces[iClr]>=100) || 
			((m_lFaces[i].m_iVB != liVB[iClr]) && (lnFaces[iClr] > 0)) )
		{
			RenderLightColorKeyFaceList(_pRenderParams,liFaces+100*iClr,lnFaces[iClr],pA,liVB[iClr],lColors[iClr]);
			lnFaces[iClr] = 0;
		}

		//Add buffer
		liVB[iClr] = m_lFaces[i].m_iVB;
		liFaces[100*iClr + lnFaces[iClr]] = i;
		lnFaces[iClr]++;
	}

	//Render remaining data
	for(i = 0;i < 10;i++)
	{
		if( lnFaces[i] ) RenderLightColorKeyFaceList(_pRenderParams,liFaces+100*i,lnFaces[i],pA,liVB[i],lColors[i]);
	}
}

#endif


#define CXR_RENDERINFO_INVISIBLE 32

// -------------------------------------------------------------------
void CXR_Model_BSP2::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS_asdf,
							  const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	CMat4Dfp32 WVelMat;
	WVelMat.Unit();
	CXR_Model_BSP2::OnRender2(_pEngine, _pRender, _pVBM, _pViewClip, _spWLS_asdf, _pAnimState, _WMat, _VMat, WVelMat, _Flags);
}

void CXR_Model_BSP2::OnRender2(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
							   const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _WVelMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_BSP_OnRender, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP2::OnRender, XR_BSPMODEL);

	/*
	AnimState
		AnimTime
			Texture-anim time
			Explode-time if Explode
		Anim0
		Anim1
			1 : Explode

	*/
	if (!_pAnimState) return;
	if (!_pEngine) return;

	CBSP2_ViewInstanceData ViewData;
	CBSP2_View_Params ViewParam;
	ViewParam.m_pViewData = &ViewData;

	if ((m_iView < 0) || (m_iView >= m_lspViews.Len()))
		Error("Render", "Invalid view index.");
	m_pView = m_lspViews[m_iView];
	m_pWorldView = NULL;
//	m_pViewData	= &m_lViewData[m_iView];

//	m_CurrentView++;
//	m_pViewData->m_CurrentView = m_CurrentView;

	_WMat.Multiply(_VMat, ViewData.m_CurVMat);
	ViewData.m_CurWMat = _WMat;

#ifdef M_Profile
	m_Time.Reset();
	m_TimeRenderPoly1.Reset();
	m_TimeRenderPoly2.Reset();
	m_TimeRenderLeafList.Reset();
	m_TimeRenderFaceQueue.Reset();
#endif

	ViewParam.m_pCurrentEngine	= _pEngine;
	ViewParam.m_pSceneFog = _pEngine->m_pCurrentFogState;

	CBSP2_RenderParams RenderParams;
	RenderParams.m_pViewData = &ViewData;
	RenderParams.m_pCurrentEngine	= _pEngine;
	RenderParams.m_pCurAnimState	= _pAnimState;
	RenderParams.m_CurOnRenderFlags	= _Flags;

	RenderParams.m_CurrentRCCaps = _pRender->Caps_Flags();
	RenderParams.m_CurrentIsPortalWorld = m_lPortalLeaves.Len() && !_pViewClip;
//	RenderParams.m_CurrentTLEnable = MODEL_BSP_TL_ENABLE_CONDITION(_pEngine);

	RenderParams.m_pVBM = _pVBM;
	RenderParams.m_pVBMatrixM2V = _pVBM->Alloc_M4(ViewData.m_CurVMat);
	RenderParams.m_pVBMatrixM2W = const_cast<CMat4Dfp32*>(&_WMat); //m_pVBM->Alloc_M4(m_pViewData->m_CurWMat);
	RenderParams.m_pVBMatrixW2V = const_cast<CMat4Dfp32*>(&_VMat); //m_pVBM->Alloc_M4(_VMat);
	RenderParams.m_pWVelMat = &_WVelMat;

	if (!RenderParams.m_pVBMatrixM2V) return;
//	m_LastMedium = 0;

	CBSP2_RenderParams* _pRenderParams = &RenderParams;
	ViewData.m_CurVMat.InverseOrthogonal(ViewData.m_CurVMatInv);

	CBox3Dfp32 BoundBox;
	Phys_GetBound_Box(_WMat, BoundBox);

	_pRenderParams->m_RenderInfo.Clear(_pEngine);

	// Get bound-box, get the CRC_ClipView
	{
//		CRC_Viewport* pVP = _pVBM->Viewport_Get();
//		m_pView->m_ViewBackPlane = pVP->GetBackPlane();

		if (_pViewClip)
		{
			// Entity
			if (!(_Flags & CXR_MODEL_ONRENDERFLAGS_NODYNAMICLIGHT))
			{
				_pRenderParams->m_RenderInfo.m_pLightInfo = _pRenderParams->m_lRenderLightInfo;
				_pRenderParams->m_RenderInfo.m_MaxLights = BSP_MAX_LIGHTS;
				_pRenderParams->m_RenderInfo.m_nLights = 0;
			}

			if (_pAnimState->m_iObject)
			{
				_pViewClip->View_GetClip(_pAnimState->m_iObject, &_pRenderParams->m_RenderInfo);
				if ((_pRenderParams->m_RenderInfo.m_Flags & CXR_RENDERINFO_INVISIBLE) && !_pRenderParams->m_RenderInfo.m_nLights)
					return;
			}
			else
			{
				if (!_pViewClip->View_GetClip_Box(BoundBox.m_Min, BoundBox.m_Max, 0, 0, NULL, &_pRenderParams->m_RenderInfo))
					return;
			}

			_pRenderParams->m_BasePriority_Opaque = _pRenderParams->m_RenderInfo.m_BasePriority_Opaque;
			_pRenderParams->m_BasePriority_Transparent = _pRenderParams->m_RenderInfo.m_BasePriority_Transparent;

			// Check if designer wants lightmap rendering for this model
			if(_pAnimState && (_pAnimState->m_AnimAttr0 == 0))
				_pRenderParams->m_RenderInfo.m_pLightVolume = NULL;
		}
		else
		{
			// World
			m_pWorldView = m_lspViews[m_iView];
			_pRenderParams->m_RenderInfo.Clear(_pEngine);
			_pRenderParams->m_BasePriority_Opaque = CXR_VBPRIORITY_MODEL_OPAQUE;
			_pRenderParams->m_BasePriority_Transparent = CXR_VBPRIORITY_MODEL_TRANSPARENT;
		}
		ViewData.m_CurLocalVP = CVec3Dfp32::GetRow(ViewData.m_CurVMatInv, 3);
	}

	_pRenderParams->m_pSceneFog = _pEngine->m_pCurrentFogState;
/*	if (m_lPortalLeaves.Len() ||
		m_RenderInfo.m_Flags & (CXR_RENDERINFO_VERTEXFOG | CXR_RENDERINFO_NHF))
	{
		m_pSceneFog = _pEngine->GetFogState();
		if (m_pSceneFog)
		{
	//		m_pSceneFog->SetEye(VMatInv);
			m_pSceneFog->SetTransform(&_WMat);
		}
	}
	else
		m_pSceneFog = NULL;*/


/*	if (!m_spTempWLS)
	{
		m_spTempWLS = DNew(CXR_WorldLightState) CXR_WorldLightState;
		if (!m_spTempWLS)
			MemError("OnRender");
	}
	if (!m_spTempWLS->m_lStatic.Len())
		m_spTempWLS->Create(256, 16, 4);
*/

	// Get clip mask
/*	{
		fp32 BoundR = GetBound_Sphere();
		int ViewMask = _pRender->Viewport_Get()->SphereInView(CVec3Dfp32::GetMatrixRow(m_pViewData->m_CurVMat, 3), BoundR);

		// FIXME:
		// If *this doesn't have a pViewClip, it's probably the world. If it's not rendered
		// there will be bugs when objects try to use it's view-interface. 
		// This prevents it from exiting.
		if (!_pViewClip) 
			if (!ViewMask) return;
	}*/

#ifdef M_Profile
	if( _pRenderParams->m_pCurrentEngine->m_DebugFlags & XR_DEBUGFLAGS_SHOWLIGHTCOUNT )
	{
		m_lFaceLights.SetLen(m_lFaces.Len());
		FillChar(m_lFaceLights.GetBasePtr(),m_lFaceLights.ListSize(),0);
	}
#endif

	if (_Flags & CXR_MODEL_ONRENDERFLAGS_WIRE)
	{
		RenderWire(_pRenderParams, _pAnimState->m_Data[0]);
	}
	else
	{
		TMeasureResetProfile(m_Time);

		// Initialize view vertex and mask lists.
		if (m_lPortalLeaves.Len())
		{
			if (m_lVVertMask.Len() != m_lVertices.Len())
			{
				m_lVVertMask.SetLen(m_lVertices.Len());
				FillChar(&m_lVVertMask[0], m_lVertices.Len(), 0);
				m_liVVertTag.SetLen(512);
				m_MaxVVertTag = 512;
			}
			m_nVVertTag = 0;
		}

		if (!(_Flags & 1) && _pRenderParams->m_CurrentIsPortalWorld)
		{
			int nPL = Max(m_pWorldView->m_liVisPortalLeaves.Len(), m_lPortalLeaves.Len());
			m_pWorldView->m_liVisPortalLeaves.SetLen(nPL);
			m_pWorldView->m_MaxVisLeaves = m_pWorldView->m_liVisPortalLeaves.Len();
			m_pWorldView->m_nVisLeaves = 0;
	//		m_pWorldView->m_lPVS.SetLen((nPL + 15) >> 3);

			// Initialize nodes RPortal look-up table.
			if (m_pWorldView->m_liLeafRPortals.Len() != m_lPortalLeaves.Len())
				m_pWorldView->m_liLeafRPortals.SetLen(m_lPortalLeaves.Len());
			FillChar(&m_pWorldView->m_liLeafRPortals[0], m_pWorldView->m_liLeafRPortals.ListSize(), 0);

			// Initialize space for RPortals.
			m_pWorldView->m_MaxRPortals = Min(512, m_lPortals.Len());
			m_pWorldView->m_lRPortals.SetLen(m_pWorldView->m_MaxRPortals);
			m_pWorldView->m_liRPortalNext.SetLen(m_pWorldView->m_MaxRPortals);
			m_pWorldView->m_lPortalScissor.SetLen(m_pWorldView->m_MaxRPortals);
			m_pWorldView->m_pRPortals = m_pWorldView->m_lRPortals.GetBasePtr();
			m_pWorldView->m_piRPortalNext = m_pWorldView->m_liRPortalNext.GetBasePtr();
			m_pWorldView->m_nRPortals = 0;
		}

		if (!VB_AllocateFaceQueues(RenderParams) ||
			!VB_AllocateSurfaceQueues(RenderParams) ||
			!VB_AllocatePrimitiveQueues(RenderParams))
			return;

		InitializeListPtrs();

		{
			CMat4Dfp32 WMatInv;
			_WMat.InverseOrthogonal(WMatInv);

			// If this is the world we need to grab the dynamic lights manually since we didnt run View_GetClip_Box()
			if (m_lPortalLeaves.Len() && m_pView->m_pSceneGraph)
			{
				CBSP2_SceneGraph* pSG = m_pView->m_pSceneGraph;
				_pRenderParams->m_RenderInfo.m_nLights = 0;
				M_LOCK(pSG->m_Lock);

				// check if something strage is going on (can crash if this check isn't here)
				if(m_pView->m_lLightOcclusion.Len() != pSG->m_lLights.Len())
				{
					ConOutL("§cf80WARNING: (CXR_Model_BSP2::OnRender) m_pView->m_lLightOcclusion.Len() != pSG->m_lLights.Len() failed.");
					return;
				}

				for(int i = pSG->m_iFirstDynamic; i < pSG->m_lLights.Len(); i++)
				{
					if (_pRenderParams->m_RenderInfo.m_nLights >= MODEL_BSP_MAXDYNAMICLIGHTS)
						break;

					if (pSG->m_lLights[i].GetIntensitySqrf() < 0.0001f)
						continue;

					const CXR_LightOcclusionInfo& LO = m_pView->m_lLightOcclusion[i];
					if (LO.IsVisible())
					{
						_pRenderParams->m_lRenderLight[_pRenderParams->m_RenderInfo.m_nLights] = pSG->m_lLights[i];
						_pRenderParams->m_lRenderLight[_pRenderParams->m_RenderInfo.m_nLights].Transform(WMatInv);
						_pRenderParams->m_lRenderLightInfo[_pRenderParams->m_RenderInfo.m_nLights].m_ShadowScissor = LO.m_ScissorVisible; // FIXME: LO.m_ScissorShadow;
						_pRenderParams->m_lRenderLightInfo[_pRenderParams->m_RenderInfo.m_nLights].m_Scissor = LO.m_ScissorVisible;
						_pRenderParams->m_RenderInfo.m_nLights++;
					}
				}

			}
			else
			{
				uint nLights = 0;
				uint16 liLights[MODEL_BSP_MAXDYNAMICLIGHTS];
				CScissorRect M_ALIGN(16) Scissors[MODEL_BSP_MAXDYNAMICLIGHTS * 2];

				int nMaxLights = 0;

				if (_pEngine->m_pViewClip)
					nMaxLights = _pEngine->m_pViewClip->View_Light_GetOcclusionSize();

				CScissorRect BoundScissor;
				CalcBoxScissor(m_pView, BoundBox, BoundScissor);

#ifndef M_RTM
				if(_pRenderParams->m_RenderInfo.m_nLights > MODEL_BSP_MAXDYNAMICLIGHTS)
				{
					ConOutL(CStrF("Dynamic BSP model called with more than 32 lights applied (%d)", _pRenderParams->m_RenderInfo.m_nLights));
					_pRenderParams->m_RenderInfo.m_nLights = 32;
				}
#endif

				for(int i = 0; i < _pRenderParams->m_RenderInfo.m_nLights; i++)
				{
					_pRenderParams->m_lRenderLight[i] = *(_pRenderParams->m_lRenderLightInfo[i].m_pLight);
					_pRenderParams->m_lRenderLight[i].Transform(WMatInv);

					uint16 iLight = _pRenderParams->m_lRenderLight[i].m_iLight;
					if (iLight < nMaxLights)
					{
						liLights[nLights] = iLight;
						Scissors[nLights * 2 + 0] = BoundScissor;
						Scissors[nLights * 2 + 1] = _pRenderParams->m_lRenderLightInfo[i].m_ShadowScissor;
						nLights++;
					}
				}

				if(nLights > 0)
				{
					M_ASSERT(_pEngine->m_pViewClip, "!");
					_pEngine->m_pViewClip->View_Light_ApplyOcclusionArray_ShadowShaded(nLights, liLights, Scissors);
				}
			}

	/*		Light_TagDynamics();

			// Render dynamic shadows
	#if 1
			for(int i = 0; i < m_RenderInfo.m_nLights; i++)
				if (!(m_lRenderLight[i].m_Flags & CXR_LIGHT_NOSHADOWS))
					Light_RenderDynamicShadowVolume(i);
	#endif*/

			bool bNoShading = (_pRenderParams->m_RenderInfo.m_Flags & CXR_RENDERINFO_INVISIBLE) != 0;

#ifdef M_Profile
			if (_pRenderParams->m_CurrentIsPortalWorld)
			{
				M_NAMEDEVENT("VB_RenderQueues", 0xffff8000);
				for(int i = 0; i < _pRenderParams->m_RenderInfo.m_nLights; i++)
					Light_RenderDynamicLight(_pRenderParams, i, bNoShading, &_WMat, &_VMat);
			}
			else
#endif
			{
				for(int i = 0; i < _pRenderParams->m_RenderInfo.m_nLights; i++)
					Light_RenderDynamicLight(_pRenderParams, i, bNoShading, &_WMat, &_VMat);
			}
#ifdef M_Profile

//		if( _pRenderParams->m_pCurrentEngine->m_DebugFlags & XR_DEBUGFLAGS_SHOWLIGHTCOUNT ) 
#endif

			if (bNoShading)
				return;
		}

		// --------- Rendering is OK from here ------------------
	#ifdef M_Profile
		m_TimePortalAnd.Reset();
		m_TimePortalOr.Reset();
		m_TimePortal0.Reset();
		m_TimePortal1.Reset();
		m_TimePortal2.Reset();
		m_TimePortal3.Reset();
		m_nPortal0 = 0;
		m_nPortal1 = 0;
		m_nPortal2 = 0;
		m_nPortal3 = 0;

		m_nFacesChecked = 0;
		m_nFacesVisible = 0;
		m_nFacesFront = 0;
		m_nSurfSwitch = 0;
	#endif

		if (_pRenderParams->m_CurrentIsPortalWorld)
		{
			_pRenderParams->m_EngineTime = _pEngine->GetEngineTime();
//			_pRenderParams->m_UnifiedAmbience = _pRenderParams->m_pCurrentEngine->GetVar(XR_ENGINE_UNIFIED_AMBIENCE);

			// If (flags::1) then don't calc portals, use the existing view information.
			if (!(_Flags & 1))
			{
				int iNodePortal = GetPortalLeaf(m_pViewClipData->m_CurLocalVP);
				if (iNodePortal == -1)
				{
			//		Error("Render", "No portal leaf!");
					ConOut("No portal leaf!");
					goto RenderAll;
				}

				{
					m_pWorldView->m_nRPortals = 1;
					m_pWorldView->m_pRPortals[m_pWorldView->m_nRPortals] = m_pViewClipData->m_CurClipVolume;
					m_pWorldView->m_piRPortalNext[m_pWorldView->m_nRPortals] = 0;
					m_pWorldView->m_nRPortals = 2;
					m_pWorldView->m_liLeafRPortals[m_pNodes[iNodePortal].m_iPortalLeaf] = 1;
				}

				{
					TMeasureResetProfile(m_TimePortals);
					m_pWorldView->m_pPVS = SceneGraph_PVSLock(0, m_lNodes[iNodePortal].m_iPortalLeaf);
						DisableTree(1);
						EnableTreeFromNode(iNodePortal);
						Portal_AddNode(&ViewParam, 1, 1);
						DisableTree(1);
					SceneGraph_PVSRelease(m_pWorldView->m_pPVS);
				}
			}

			if (!m_pWorldView->m_nVisLeaves)
				goto RenderAll;

			{
				TMeasureResetProfile(m_TimeRenderLeafList);

		#ifdef M_Profile
					m_TimeBrushVB.Reset();
					m_TimeSurf.Reset();
					m_TimeCull.Reset();
		#endif

					{
						TMeasureResetProfile(m_TimeBuild);

						{
							M_NAMEDEVENT("QueuePrep", 0xffff8000);
							PrepareShaderQueue(_pRenderParams);
							VB_ClearQueues(_pRenderParams);
							VB_ClearFaceQueues(_pRenderParams);
						}
#ifdef M_Profile
						if( _pRenderParams->m_pCurrentEngine->m_DebugFlags & XR_DEBUGFLAGS_SHOWLIGHTCOUNT ) 
							LightCountSLC(_pRenderParams);
#endif
						RenderLeafList(_pRenderParams);

						{
							M_NAMEDEVENT("RenderShaderQueue", 0xffff8000);
#ifdef PLATFORM_XENON
							CMTime T;
							TStart(T);

							static volatile uint ms_bDoTrace = false;
							uint bTrace = ms_bDoTrace;
							if (bTrace)
								XTraceStartRecording( "cache:\\TraceRecord_RenderShaderQueue.bin" );
#endif
							RenderShaderQueue(_pRenderParams);
#ifdef PLATFORM_XENON
							if (bTrace)
							{
								XTraceStopRecording();
								ms_bDoTrace = false;
							}

							TStop(T);
//							M_TRACEALWAYS("RenderShaderQueue: %.3f ms \n", T.GetTime() * 1000.0);
#endif
						}
						{
							M_NAMEDEVENT("VB_RenderFaceQueues", 0xffff8000);
							VB_RenderFaceQueues(_pRenderParams);
						}
						{
							M_NAMEDEVENT("VB_Flush", 0xffff8000);
							VB_Flush(_pRenderParams);
						}
						{
							M_NAMEDEVENT("VB_RenderQueues", 0xffff8000);

							VB_RenderQueues(_pRenderParams);
						}

						// Render and clear wallmarks
			//			if (m_spWMC) Wallmark_Render(m_spWMC);
						if (_pEngine->GetVCDepth() == 0 && m_spWMCTemp != NULL)
						{
			//				Wallmark_Render(m_spWMCTemp);
							m_spWMCTemp->Clear();
						}
					}
		#ifdef M_Profile
					m_TimeCull = m_TimeBuild - m_TimeBrushVB - m_TimeSurf;
		#endif
			}

			if (m_pPortals && !(ms_Enable & MODEL_BSP_ENABLE_NOPORTALSTATS))
			{
				_pVBM->ScopeBegin("RenderNodePortals", false);
				int iNodePortal = GetPortalLeaf(m_pViewClipData->m_CurLocalVP);
				RenderNodePortals(_pRenderParams, iNodePortal);
				_pVBM->ScopeEnd();
			}

	/*		_pRender->Attrib_Push();
			_pRender->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
			for(int i = 1; i < m_pView->m_nRPortals; i++)
				RenderRPortal(&m_pView->m_pRPortals[i], 0xffffff7f);
			_pRender->Attrib_Pop();
	*/

//			_pRender->Matrix_Pop();

	//		T_Stop(m_TimeRenderFaceQueue);
		}
		else
		{
	RenderAll:
	//		_pRender->Matrix_PushMultiply(m_pViewData->m_CurVMat);
	//		_pRender->Attrib_Push();
			{
				if (_pRenderParams->m_pSceneFog)
				{
					_pRenderParams->m_pSceneFog->SetTransform(&_WMat);
	//				m_pSceneFog->TraceBound(BoundBox);
				}

				PrepareShaderQueue(_pRenderParams);
				VB_ClearQueues(_pRenderParams);
				VB_ClearFaceQueues(_pRenderParams);

				// FIXME: Medium should be enumerated from the engine.
				CXR_MediumDesc Medium;
				Medium.SetAir();
				Medium.m_MediumFlags = _pRenderParams->m_RenderInfo.m_MediumFlags;
				_pRenderParams->m_pCurMedium = &Medium;

				/*if (m_lPortalLeaves.Len())
				{
					m_pView->m_nRPortals = 1;
					m_pView->m_pRPortals[m_pView->m_nRPortals] = m_pViewData->m_CurClipVolume;
					m_pView->m_piRPortalNext[m_pView->m_nRPortals] = 0;
					m_pView->m_nRPortals++;
			
					{
						TMeasureProfile(m_TimeBrushVB);

						for(int iPL = 0; iPL < m_nStructureLeaves; iPL++)
							RenderPortalLeaf(iPL);
					}
				}
				else*/
				{
//					bool bNHF = (_pRenderParams->m_pCurMedium->m_MediumFlags & XW_MEDIUM_FOG) && _pRenderParams->m_pSceneFog &&  _pRenderParams->m_pSceneFog->NHFEnable();

	//				bool bClipNHF = bNHF && (m_pCurMedium->m_MediumFlags & XW_MEDIUM_CLIPFOG);

	/*				if (!bNHF && (bVertexLighting || (m_spTempWLS->m_nDynamic == 0)))
					{
						const CBSP2_VBFacePrim* pVBFP = m_lVBFacePrim.GetBasePtr();
						int nVBFP = m_lVBFacePrim.Len();
						const uint16* pFacePrim = m_lFacePrim.GetBasePtr();

						for(int i = 0; i < nVBFP; i++)
						{
							VB_AddPrimitives(pVBFP->m_iVB, &pFacePrim[pVBFP->m_iPrim], pVBFP->m_nPrim);
							pVBFP++;
						}
					}
					else */
					{
						uint32 liVisFaces[2048];
						int nVisFaces = 0;

						int nFaces = m_lFaces.Len();
						for(int iFace = 0; iFace < nFaces; iFace++)
						{
							const CBSP2_Face* pFace = &m_pFaces[iFace];
							if (pFace->m_Flags & XW_FACE_VISIBLE)
							{
								if (m_lspSurfaces[pFace->m_iSurface]->m_Flags & XW_SURFFLAGS_REFLECTION) continue;
		#ifdef MODEL_BSP_BACKFACECULL
								if (m_pPlanes[pFace->m_iPlane].Distance(ViewData.m_CurLocalVP) > 0.0f)
		#endif
								{
									_pRenderParams->m_pVBFaceQueue[pFace->m_iVB].AddElem(iFace);
									if (nVisFaces < 2048) 
										liVisFaces[nVisFaces++] = iFace;
								}
							}
						}

		#ifndef MODEL_BSP_DISABLENHF
						if (_pRenderParams->m_pCurMedium->m_MediumFlags & XW_MEDIUM_FOG)
							if (_pRenderParams->m_pSceneFog && _pRenderParams->m_pSceneFog->NHFEnable())
							{
								RenderNHF(_pRenderParams, liVisFaces, nVisFaces, 0);
							}
		#endif
					}

				}

				RenderShaderQueue(_pRenderParams);
				VB_RenderFaceQueues(_pRenderParams);

				VB_Flush(_pRenderParams);
				VB_RenderQueues(_pRenderParams);

	//			if (m_pSceneFog) 
	//				m_pSceneFog->TraceBoundRelease();
			}
	//		_pRender->Attrib_Pop();
	//		_pRender->Matrix_Pop();
		}

#ifdef M_Profile
		if (m_pPortals && !(ms_Enable & MODEL_BSP_ENABLE_NOPORTALSTATS))
		{
			fp32 k = 1000.0f;
			ConOut(CStrF("%d/%d/%d, surf %d, %.2f (Surf %.2f, VB %.2f, C %.2f)", m_nFacesChecked, m_nFacesFront, m_nFacesVisible, m_nSurfSwitch, m_TimeBuild.GetTime()*k, m_TimeSurf.GetTime()*k, m_TimeBrushVB.GetTime()*k, m_TimeCull.GetTime()*k));
		}
#endif

	}

#ifdef M_Profile
	if( _pRenderParams->m_pCurrentEngine->m_DebugFlags & XR_DEBUGFLAGS_SHOWLIGHTCOUNT ) 
	{
		RenderLightColorKey(_pRenderParams);

		//Entity
		if( !m_lPortalLeaves.Len() )
		{
			CVec3Dfp32 lV[BSP_MAX_LIGHTS*2];
			CPixel32 lC[BSP_MAX_LIGHTS*2];

			int nV = 0;
			for(int i = 0;i < _pRenderParams->m_RenderInfo.m_nLights;i++)
			{
				const CXR_Light * pL = _pRenderParams->m_RenderInfo.m_pLightInfo[i].m_pLight;
				
				if( pL->GetIntensitySqrf() < 0.0001f )
					continue;

				lV[nV] = _WMat.GetRow(3);
				lV[nV+1] = pL->GetPosition();
				lC[nV] = 0xff201000;
				lC[nV+1] = 0xffff8000;
				nV += 2;

//				_pRenderParams->m_pVBM->RenderWire(_VMat,_WMat.GetRow(3),
//					pL->GetPosition(),CPixel32(255,255,255,255));
			}

			if (nV)
				_pRenderParams->m_pVBM->RenderWires(_WMat, lV, g_IndexRamp32, nV, lC, false);
		}
	}

	if( m_lOctaAABBNodes.Len() && (_pRenderParams->m_pCurrentEngine->m_DebugFlags & XR_DEBUGFLAGS_SHOWOCTAAABB) && (m_lPortalLeaves.Len()) )
	{
		//Modifier from uint16-space
		CMat4Dfp32 FromU16;
		FromU16.Unit();
		FromU16.k[0][0] = (m_OctaAABBBox.m_Max[0] - m_OctaAABBBox.m_Min[0]) / 65535.0;
		FromU16.k[1][1] = (m_OctaAABBBox.m_Max[1] - m_OctaAABBBox.m_Min[1]) / 65535.0;
		FromU16.k[2][2] = (m_OctaAABBBox.m_Max[2] - m_OctaAABBBox.m_Min[2]) / 65535.0;
		FromU16.k[3][0] = m_OctaAABBBox.m_Min[0];
		FromU16.k[3][1] = m_OctaAABBBox.m_Min[1];
		FromU16.k[3][2] = m_OctaAABBBox.m_Min[2];

		CPixel32 lColors[9];
		lColors[0] = CPixel32(0xFF,0x00,0x00,0xFF);
		lColors[1] = CPixel32(0x00,0xFF,0x00,0xFF);
		lColors[2] = CPixel32(0x00,0x00,0xFF,0xFF);
		lColors[3] = CPixel32(0x7F,0x7F,0x00,0xFF);
		lColors[4] = CPixel32(0x7F,0x00,0x7F,0xFF);
		lColors[5] = CPixel32(0x00,0x7F,0x7F,0xFF);
		lColors[6] = CPixel32(0x7F,0x7F,0x7F,0xFF);
		lColors[7] = CPixel32(0x3F,0x3F,0x3F,0xFF);
		lColors[8] = CPixel32(0xFF,0xFF,0xFF,0xFF);
//		BSPOctaAABB_DrawTree_r(m_lOctaAABBNodes.GetBasePtr(),0,m_lPortalLeaves.GetBasePtr(),
//			_pRenderParams->m_pVBM,*_pRenderParams->m_pVBMatrixM2V,
//			FromU16,lColors,8,true,false,lColors+8);

		for(int k = m_pView->m_nVisLeaves-1; k >= 0; k--)
		{
			int iVPL = m_pView->m_liVisPortalLeaves[k];
			CBSP2_PortalLeafExt &PL = m_lPortalLeaves[iVPL];

			uint32 nNodes = BSPOctaAABB_CountChildren(m_lOctaAABBNodes.GetBasePtr(),PL.m_iOctaAABBNode);

			void * pData = _pRenderParams->m_pVBM->Alloc(sizeof(CXR_VertexBuffer) + sizeof(CRC_Attributes) + sizeof(CMat4Dfp32) +
				nNodes * (8 * (sizeof(CVec3Dfp32) + sizeof(CPixel32)) + sizeof(uint16) * 24));

			if( !pData ) break;

			CRC_Attributes*M_RESTRICT pA = (CRC_Attributes*)pData;
			pA->SetDefault();

			CXR_VertexBuffer * pVB = (CXR_VertexBuffer*)(pA+1);
#ifdef M_Profile
			pVB->m_Link.Construct();
#endif
			pVB->Clear();
			pVB->m_pAttrib = pA;
			*((CMat4Dfp32*)(pVB + 1)) = *_pRenderParams->m_pVBMatrixM2V;	// why allocate transform?
			pVB->m_pTransform = (CMat4Dfp32*)(pVB + 1);

			if (!pVB->AllocVBChain(_pRenderParams->m_pVBM,false))
				break;
			pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
			pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			pVB->m_Priority		= 10000000.0f;
			CXR_VBChain* pChain = pVB->GetVBChain();
			pChain->m_pV = (CVec3Dfp32*)(pVB->m_pTransform + 1);
			pChain->m_pCol = (CPixel32*)(pChain->m_pV + nNodes * 8);
			pChain->m_nV = nNodes * 8;

			pChain->m_PrimType = CRC_RIP_WIRES;
			pChain->m_piPrim = (uint16*)(pChain->m_pCol + nNodes * 8);
			pChain->m_nPrim = nNodes * 24;

			BSPOctaAABB_DrawTree(m_lOctaAABBNodes.GetBasePtr(),PL.m_iOctaAABBNode,lColors,9,FromU16,pChain->m_pV,pChain->m_piPrim,pChain->m_pCol);

			_pRenderParams->m_pVBM->AddVB(pVB);
		}
		
	}
#endif

}

// -------------------------------------------------------------------
void CXR_Model_BSP2::OnPrecache(CXR_Engine* _pEngine, int _iVariation)
{
	MAUTOSTRIP(CXR_Model_BSP_OnPrecache, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("Read", "No texture-context available.");

	// Precache textures
	{
		// Set precache flags on all textures

//ms_TextureID_TestBump = pTC->GetTextureID("Cube_20L_0");

//		pTC->SetTextureParam(pTC->GetTextureID("SPECIAL_DEFAULTLENS"), CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);

		// Set precache flags on all surface textures
		for(int iSurf = 0; iSurf < m_lspSurfaces.Len(); iSurf++)
		{
			CXW_Surface* pS = m_lspSurfaces[iSurf];
			pS = pS->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
			if (!pS) continue;

			pS->InitTextures(false);	// Don't report failures.
			if (!(pS->m_Flags & XW_SURFFLAGS_INVISIBLE))
				pS->SetTextureParam(CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
		}
	}

	// Set precache flags on all lightmap textures
	if (m_spLMTC != NULL)
	{
		int nLMC = m_spLMTC->GetNumLocal();
		for(int i = 0; i < nLMC; i++)
		{
			if (m_spLMTC->GetTextureID(i))
				pTC->SetTextureParam(m_spLMTC->GetTextureID(i), CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);		// Causes occational calls to BuildInto when pWLS is invalid.
		}
	}
	
	// Set precache flags on all vertex buffers
	{
		TAP_RCD<CBSP2_VBInfo> pVBInfo(m_lVBInfo);
		for(int i = 0; i < pVBInfo.Len(); i++)
		{
			int VBID = pVBInfo[i].m_VBID;
			if (VBID)
				_pEngine->m_pVBC->VB_SetFlags(VBID, _pEngine->m_pVBC->VB_GetFlags(VBID) | CXR_VBFLAGS_PRECACHE);
		}
	}

	// Shadow volume buffers
	if (m_spLightData != NULL)
	{
		CBSP2_LightData* pLD = m_spLightData;
		for(int i = 0; i < pLD->m_lVBIds.Len(); i++)
		{
			int VBID = pLD->m_lVBIds[i].m_VBID;
			if (VBID != -1)
				_pEngine->m_pVBC->VB_SetFlags(VBID, _pEngine->m_pVBC->VB_GetFlags(VBID) | CXR_VBFLAGS_PRECACHE);
		}

		{
			for(int i = 0; i < pLD->m_lSLCIBI.Len(); i++)
			{
				int IBID = pLD->m_lSLCIBI[i].m_IBID;
				if (IBID != 0)
					_pEngine->m_pVBC->VB_SetFlags(IBID, _pEngine->m_pVBC->VB_GetFlags(IBID) | CXR_VBFLAGS_PRECACHE);
			}
		}
	}
}

void CXR_Model_BSP2::OnRefreshSurfaces()
{
	for(int s = 0; s < m_lspSurfaces.Len(); s++)
	{
		m_lspSurfaces[s]->Init();
		m_lspSurfaces[s]->InitTextures(false);
	}
}

void CXR_Model_BSP2::CleanStatic()
{
	extern TThinArray<uint32> g_BSP2liVertMap;
	extern TThinArray<uint32> g_BSP2liVertMapClear;
	g_BSP2liVertMap.Destroy();
	g_BSP2liVertMapClear.Destroy();
}
