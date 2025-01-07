#include "PCH.h"

#include "MFloat.h"
#include "WBSPModel.h"
#include "WBSPDef.h"
#include "../../Classes/BitString/MBitString.h"

#include "../../XR/XREngineVar.h"
#include "../../XR/XRVBContext.h"


//#define MODEL_BSP_TL_ENABLE_CONDITION m_lPortalLeaves.Len() && m_pCurrentEngine->GetVar(0x2345) != 0
#define MODEL_BSP_TL_ENABLE_CONDITION ms_pCurrentEngine->m_bTLEnableEnabled

#define MODEL_BSP_DYNLIGHTCULLBACKFACE_CONDITION (ms_CurrentRCCaps & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20)


// On by default
// #define MODEL_BSP_SORTENABLE
#define MODEL_BSP_VB

// Off by default
// #define MODEL_BSP_ONELIGHTMAP

//#define MODEL_BSP_DISABLENHF
//#define MODEL_BSP_DISABLEFOGPORTALS

//#define MODEL_BSP_BACKFACECULL

//#define MODEL_BSP_TRIANGLEFAN			// Must be enabled, triangle lists doesn't work since all prims go through Render_IndexedPrimitives

void Prefetch8(void* _Ptr);

//#define PREFETCH(Addr) Prefetch8(Addr);
#define PREFETCH(Addr)



IMPLEMENT_OPERATOR_NEW(CBSP_LinkContext);
IMPLEMENT_OPERATOR_NEW(CBSPModel_ViewInstance);
IMPLEMENT_OPERATOR_NEW(CBSP_VertexBuffer);
IMPLEMENT_OPERATOR_NEW(CXR_Model_BSP);


// -------------------------------------------------------------------
const CXR_AnimState*		CXR_Model_BSP::m_pCurAnimState;
CXR_MediumDesc*				CXR_Model_BSP::m_pCurMedium;
CXW_Surface*				CXR_Model_BSP::m_pCurSurface;
CXW_SurfaceKeyFrame*		CXR_Model_BSP::m_pCurSurfaceKey;
CXW_SurfaceKeyFrame			CXR_Model_BSP::m_TmpSurfKeyFrame;

CXR_RenderInfo				CXR_Model_BSP::ms_RenderInfo(NULL);

int							CXR_Model_BSP::ms_SurfOptions = 0;
int							CXR_Model_BSP::ms_SurfCaps = 0;

CRC_Attributes*				CXR_Model_BSP::ms_Render_lpAttribDynLight[MODEL_BSP_MAXDYNAMICLIGHTS] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
CRC_Attributes*				CXR_Model_BSP::ms_Render_lpAttribDynLight_DestAlphaExpAttn[MODEL_BSP_MAXDYNAMICLIGHTS] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

int							CXR_Model_BSP::ms_TextureID_AttenuationExp = 0;
#if !defined( PLATFORM_PS2 ) && !defined( PLATFORM_DOLPHIN )
int							CXR_Model_BSP::ms_TextureID_Normalize = 0;
int							CXR_Model_BSP::ms_TextureID_CubeNoise = 0;
int							CXR_Model_BSP::ms_TextureID_Specular[8] = { 0,0,0,0,0,0,0,0 };
#endif
int							CXR_Model_BSP::ms_TextureID_DynLightProj = 0;

CBox3Dfp32					CXR_Model_BSP::ms_Fog_LastBox;

int							CXR_Model_BSP::ms_LastMedium = 0;
fp32							CXR_Model_BSP::ms_BasePriority_Opaque = 0;
fp32							CXR_Model_BSP::ms_BasePriority_Transparent = 0;
CXR_Engine*					CXR_Model_BSP::ms_pCurrentEngine = NULL;
int							CXR_Model_BSP::ms_CurrentTLEnable = 0;
int							CXR_Model_BSP::ms_bCurrentDynLightCullBackFace = 0;
int							CXR_Model_BSP::ms_CurrentRCCaps = 0;
CXR_VBManager*				CXR_Model_BSP::ms_pVBM = NULL;
CMat4Dfp32*					CXR_Model_BSP::ms_pVBMatrixM2V = NULL;
CMat4Dfp32*					CXR_Model_BSP::ms_pVBMatrixM2W = NULL;
CMat4Dfp32*					CXR_Model_BSP::ms_pVBMatrixW2V = NULL;
CXR_FogState*				CXR_Model_BSP::ms_pSceneFog = NULL;

//CMat4Dfp32					CXR_Model_BSP::ms_PhysWMat;
//CMat4Dfp32					CXR_Model_BSP::ms_PhysWMatInv;
//class CWireContainer*		CXR_Model_BSP::ms_pPhysWC = NULL;

#ifdef M_Profile
int							CXR_Model_BSP::ms_nPortal0 = 0;
int							CXR_Model_BSP::ms_nPortal1 = 0;
int							CXR_Model_BSP::ms_nPortal2 = 0;
int							CXR_Model_BSP::ms_nPortal3 = 0;

int							CXR_Model_BSP::ms_nFacesChecked = 0;
int							CXR_Model_BSP::ms_nFacesFront = 0;
int							CXR_Model_BSP::ms_nFacesVisible = 0;
int							CXR_Model_BSP::ms_nSurfSwitch = 0;

CMTime						CXR_Model_BSP::ms_TimeSurf;
CMTime						CXR_Model_BSP::ms_TimeBrushVB;
CMTime						CXR_Model_BSP::ms_TimeSplineVB;
CMTime						CXR_Model_BSP::ms_TimeCull;
CMTime						CXR_Model_BSP::ms_TimeBuild;

CMTime						CXR_Model_BSP::ms_Time;
CMTime						CXR_Model_BSP::ms_TimePortals;
CMTime						CXR_Model_BSP::ms_TimePortalAnd;
CMTime						CXR_Model_BSP::ms_TimePortalOr;
CMTime						CXR_Model_BSP::ms_TimePortal0;
CMTime						CXR_Model_BSP::ms_TimePortal1;
CMTime						CXR_Model_BSP::ms_TimePortal2;
CMTime						CXR_Model_BSP::ms_TimePortal3;
CMTime						CXR_Model_BSP::ms_TimeRenderLeafList;
CMTime						CXR_Model_BSP::ms_TimeRenderFaceQueue;
CMTime						CXR_Model_BSP::ms_TimeRenderPoly1;
CMTime						CXR_Model_BSP::ms_TimeRenderPoly2;
#endif

CVec2Dfp32					CXR_Model_BSP::ms_PortalOrMin = 0;
CVec2Dfp32					CXR_Model_BSP::ms_PortalOrMax = 0;



// -------------------------------------------------------------------
//  Stuff in MImage.cpp
// -------------------------------------------------------------------

void PPA_Mul_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);
void PPA_MulAddSaturate_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);

void PPA_Mul_RGBA32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);

// -------------------------------------------------------------------
//  CBSP_VBQueue
// -------------------------------------------------------------------
void CBSP_VBQueue::AddChain(CBSP_VBChain *_pVB)
{
	if (_pVB->m_bVBIds)
		m_VBQueue.AddChain(_pVB->GetVBIDChain());
	else
		m_VBQueue.AddChain(_pVB->GetVBChain());
}

void CBSP_VBQueue::AddChain(CXR_VBIDChain *_pVB)
{
	m_VBQueue.AddChain(_pVB);
}

void CBSP_VBQueue::AddChain(CXR_VBChain *_pVB)
{
	m_VBQueue.AddChain(_pVB);
}


void CBSP_VBQueue::AddFromVB(CXR_VertexBuffer *_pVB)
{
	if (_pVB->IsVBIDChain())
		m_VBQueue.AddChain(_pVB->GetVBIDChain());
	else
		m_VBQueue.AddChain(_pVB->GetVBChain());
}

void CBSP_VBQueue::Clear()
{
	MAUTOSTRIP(CBSP2_VBQueue_Clear, MAUTOSTRIP_VOID);
	m_VBQueue.Clear();
}

// -------------------------------------------------------------------
//  CXR_Model_BSP
// -------------------------------------------------------------------
int CXR_Model_BSP::ms_Enable = -1 - MODEL_BSP_ENABLE_SPLINEWIRE - MODEL_BSP_ENABLE_FULLBRIGHT /* - MODEL_BSP_ENABLE_MINTESSELATION*/;
spCXR_WorldLightState CXR_Model_BSP::m_spTempWLS;

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_BSP, CXR_PhysicsModel);


// -------------------------------------------------------------------
CXR_Model_BSP::CXR_Model_BSP()
{
	MAUTOSTRIP(CXR_Model_BSP_ctor, MAUTOSTRIP_VOID);
	m_iView = 0;
	m_pView = NULL;

	m_nFogTags = 0;
	m_MaxFogTags = 0;
	m_Vis_pEngine = NULL;
	m_VBSBTessLevel = 0.5f;
}

CXR_Model_BSP::~CXR_Model_BSP()
{
	MAUTOSTRIP(CXR_Model_BSP_dtor, MAUTOSTRIP_VOID);

//	MRTC_GetMemoryManager()->m_bValidateHeap	= true;

	m_spLMTC = NULL;
	VB_FreeVBID();
}

void CXR_Model_BSP::RenderPortalFence(int _iPortal, CPixel32 _Color)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderPortalFence, MAUTOSTRIP_VOID);
	if (!ms_pVBM) return;
	CBSP_PortalExt* pP = &m_lPortals[_iPortal];

	int nv = pP->m_nVertices;
	int iiv = pP->m_iiVertices;

	uint32* piV = &m_piVertices[0];
	CVec3Dfp32* pV = &m_pVertices[0];

//	int32 Color = (pP->m_PortalID) ? 0x2fc0c0ff : 0x2fc0ffc0;

	uint16 liV[CRC_MAXPOLYGONVERTICES];
	for(int i = 0; i < nv; i++)
		liV[i] = piV[iiv+i];

	CMat4Dfp32 Unit; Unit.Unit();
	ms_pVBM->RenderWires(Unit, pV, liV, nv, _Color, true);
}

void CXR_Model_BSP::RenderRPortal(const CRC_ClipVolume* _pPortal, CPixel32 _Color)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderRPortal, MAUTOSTRIP_VOID);
	if (!ms_pVBM) return;

	ms_pVBM->RenderWires(m_pView->m_CurVMat, _pPortal->m_Vertices, g_IndexRamp16, _pPortal->m_nPlanes, _Color, true);
}

void CXR_Model_BSP::RenderNodePortals(int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderNodePortals, MAUTOSTRIP_VOID);
	int iPL = m_pNodes[_iNode].m_iPortalLeaf;
	if (!iPL) return;

	const CBSP_PortalLeafExt* pPL = &m_pPortalLeaves[iPL];

	int np = pPL->m_nPortals;
	int iip = pPL->m_iiPortals;

	for(int p = 0; p < np; p++)
		RenderPortalFence(m_liPortals[iip + p], 0x40ff40);
}


void CXR_Model_BSP::GetFaceBoundBox(int _iFace, CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_Model_BSP_GetFaceBoundBox, MAUTOSTRIP_VOID);
	CBSP_Face* pF = &m_lFaces[_iFace];
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

void CXR_Model_BSP::RenderPortalSurface(CXR_Engine* _pEngine, void* _pSurfContext, const CVec3Dfp32* _pVertices, int _nVertices, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderPortalSurface, MAUTOSTRIP_VOID);
//	CXR_ViewContext* pVC = _pEngine->GetVC();
	CRenderContext* pRC = _pEngine->m_pRender;
	if (!_pEngine) return;

	m_pView = m_lspViews[m_iView];
	CMat4Dfp32 Mat; Mat.Unit();
	ms_pSceneFog = _pEngine->GetFogState();
	// URGENTFIXME: ms_PhysWMat WTF!?
	// _pEngine->GetFogState()->SetTransform(&ms_PhysWMat);
	_pEngine->GetFogState()->SetTransform(&Mat);

//	CXR_VBManager* pVBM = _pEngine->GetVBM();

//	if ((m_spLMTC != NULL) && !m_spLMTC->m_spCurrentWLS) return;
	ms_CurrentRCCaps = pRC->Caps_Flags();
	ms_pCurrentEngine = _pEngine;
	ms_pVBM = _pEngine->GetVBM();
//	m_spCurrentWLS = pVC->m_spLightState;
	ms_CurrentTLEnable = MODEL_BSP_TL_ENABLE_CONDITION;
	ms_bCurrentDynLightCullBackFace = MODEL_BSP_DYNLIGHTCULLBACKFACE_CONDITION;
	ms_SurfOptions = _pEngine->m_SurfOptions;
	ms_SurfCaps = _pEngine->m_SurfCaps;

	ms_pVBMatrixM2W = const_cast<CMat4Dfp32*>(&_WMat);
	ms_pVBMatrixW2V = const_cast<CMat4Dfp32*>(&_VMat);

	CXR_MediumDesc Medium;
	Medium.SetAir();
	m_pCurMedium = &Medium;
	ms_RenderInfo.Clear(_pEngine);

	uint32 iFace = (mint) _pSurfContext;
//	ConOut(CStrF("RPS %d", iFace));
	{
		VB_ClearQueues();
		VB_ClearFaceQueues();

//		CBox3Dfp32 Bound;
//		GetFaceBoundBox(iFace, Bound);
		VB_RenderFaces(m_lFaces[iFace].m_iVB, &iFace, 1, m_lspSurfaces[m_lFaces[iFace].m_iSurface]->GetSurface(ms_SurfOptions, ms_SurfCaps), false);

		VB_RenderFaceQueues();
		VB_Flush();
		VB_RenderQueues();
	}
}

void CXR_Model_BSP::PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip,
	const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_BSP_PreRender, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP::PreRender, XR_BSPMODEL);

	if (m_lPortalLeaves.Len())
	{
		CXR_FogState* pFog = _pEngine->GetFogState();
		if (pFog) pFog->AddModel(this, CVec3Dfp32(0));
	}

/*	if (!m_pView)
	{
		InitializeListPtrs();
		m_nPortalFaces = 0;
		m_liPortalFaces.SetLen(CXR_MAXPORTALFACES);
		Portal_AddPortalFaces_r(1);
	}*/

	m_pView = (m_lspViews.ValidPos(m_iView)) ? m_lspViews[m_iView] : (spCBSPModel_ViewInstance)NULL;
	if (m_lPortalLeaves.Len() && m_pView)
	{
		_WMat.Multiply(_VMat, m_pView->m_CurVMat);
		m_pView->m_CurWMat = _WMat;
//		ms_pVBMatrixM2V = _pEngine->GetVBM()->Alloc_M43(m_pView->m_CurVMat);
		ms_pVBMatrixM2V = 0;
		ms_pVBMatrixM2W = const_cast<CMat4Dfp32*>(&_WMat); // _pEngine->GetVBM()->Alloc_M4(m_pView->m_CurWMat);
		ms_pVBMatrixW2V = const_cast<CMat4Dfp32*>(&_VMat); //_pEngine->GetVBM()->Alloc_M4(_VMat);

//		if (!ms_pVBMatrixM2V /*|| !ms_pVBMatrixM2W || !ms_pVBMatrixW2V*/) return;

		// World with portals.
		const uint16* piVisPortalLeaves = m_pView->m_liVisPortalLeaves.GetBasePtr();
		for(int iiPL = 0; iiPL < m_pView->m_nVisLeaves; iiPL++)
		{
			int iPL = piVisPortalLeaves[iiPL];
			int iRP = m_pView->m_liLeafRPortals[iPL];
			if (!iRP) continue;
			CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[iPL];

			for(int iiRefFace = 0; iiRefFace < pPL->m_nRefFaces; iiRefFace++)
			{
				uint32 iFace = m_liPLFaces[pPL->m_iiRefFaces + iiRefFace];
				CBSP_Face* pF = &m_lFaces[iFace];
#ifdef MODEL_BSP_BACKFACECULL
				if (m_lPlanes[pF->m_iPlane].Distance(m_pView->m_CurLocalVP) < 0.0f) continue;
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
					_pEngine->Render_AddMirror(Verts, nVerts, _WMat, _VMat, 0x407f7f7f, this, (void*)(mint)iFace);
				}
			}

//			if (!_pEngine->GetVCDepth()) return;
#ifndef PLATFORM_PS2
			CXR_Model* pSky = _pEngine->GetVC()->GetSky();
			if (pSky && pSky->Sky_GetInterface())
			{
				CXR_SkyInterface* pSkyInterface = pSky->Sky_GetInterface();

//			if (_pEngine->GetVCDepth()) ConOut("PreRender, Adding skyfaces.");
				for(int iiSkyFace = 0; iiSkyFace < pPL->m_nSkyFaces; iiSkyFace++)
				{
					uint32 iFace = m_liPLFaces[pPL->m_iiSkyFaces + iiSkyFace];
					CBSP_Face* pF = &m_lFaces[iFace];
#ifdef MODEL_BSP_BACKFACECULL
					if (m_lPlanes[pF->m_iPlane].Distance(m_pView->m_CurLocalVP) < 0.0f) continue;
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
						pSkyInterface->Sky_ExpandFrustrum(_pEngine, _WMat, m_pView->m_CurVMat, Verts, nVerts);
					}
				}
			}
#endif
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
		CRC_ClipVolume Clip;
		if (!_pViewClip->View_GetClip_Box(Box.m_Min, Box.m_Max, 0, 0, &Clip, NULL)) return;

		// Create front-plane in model-space.
		CMat4Dfp32 VMatInv, WMatInv;
		_VMat.InverseOrthogonal(VMatInv);
		_WMat.InverseOrthogonal(WMatInv);
		CVec3Dfp32 VP(0);
		VP *= VMatInv;
		VP *= WMatInv;

		_WMat.Multiply(_VMat, m_pView->m_CurVMat);
//		ms_pVBMatrixM2V = _pEngine->GetVBM()->Alloc_M43();
		ms_pVBMatrixM2V = 0;
		ms_pVBMatrixM2W = const_cast<CMat4Dfp32*>(&_WMat); // _pEngine->GetVBM()->Alloc_M4(_WMat);
		ms_pVBMatrixW2V = const_cast<CMat4Dfp32*>(&_VMat); // _pEngine->GetVBM()->Alloc_M4(_VMat);

//		if (!ms_pVBMatrixM2V/* || !ms_pVBMatrixM2W || !ms_pVBMatrixW2V*/) return;

		// Entity
		for(int iiRefFace = 0; iiRefFace < m_liRefFaces.Len(); iiRefFace++)
		{
			uint32 iFace = m_liRefFaces[iiRefFace];
			CBSP_Face* pF = &m_lFaces[iFace];
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

void CXR_Model_BSP::RenderGetPrimCount(const uint32* _piFaces, int _nFaces, int& _nV, int& _nPrim)
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

		const CBSP_Face* pF = &m_pFaces[piF[f]];
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

		const CBSP_Face* pF = &m_pFaces[piF[f]];
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

void CXR_Model_BSP::RenderGetPrimCount_KnitStrip(const uint32* _piFaces, int _nFaces, int& _nV, int& _nPrim)
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
		const CBSP_Face* pF = &m_pFaces[piF[f]];
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
		const CBSP_Face* pF = &m_pFaces[piF[f]];
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

void CXR_Model_BSP::Tesselate(const uint32* _piFaces, int _nFaces, int _nV, CVec3Dfp32* _pV, CVec2Dfp32* _pTV1, CVec2Dfp32* _pTV2, CPixel32* _pCol, CVec3Dfp32* _pN, CVec3Dfp32* _pTangU, CVec3Dfp32* _pTangV, uint32* _piFaceVertices, CXW_Surface* _pSurf)
{
	MAUTOSTRIP(CXR_Model_BSP_Tesselate, MAUTOSTRIP_VOID);
	CVec3Dfp32 UVec;
	CVec3Dfp32 VVec;
	fp32 UVecLenSqrInv;
	fp32 VVecLenSqrInv;
	fp32 UOffset;
	fp32 VOffset;
	fp32 TxtWidthInv = 0.0f;
	fp32 TxtHeightInv = 0.0f;
	fp32 MidPixelAdjustLM = 0.0f;
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
	
//	int nP = 0;
	int nV = 0;
//	int iVBase = 0;
	const uint32* piF = _piFaces;
	for(int i = 0; i < _nFaces; i++)
	{
		int iFace = *(piF++);
		const CBSP_Face* pF = &m_pFaces[iFace];
		int iVBase = nV;
		if (_piFaceVertices) _piFaceVertices[i] = iVBase;
		int nv = pF->m_nVertices;
		int iiv = pF->m_iiVertices;

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

			fp32 MidPixelAdjust = 0.5f;
			int LMWidth = 0;
			int LMHeight = 0;
			int bUseLM = (pF->m_Flags & XW_FACE_LIGHTMAP);
			const CBSP_LightMapInfo* pLMI = NULL;
			if (bUseLM && pLMTV)
			{
				int iLM = pF->m_iLightInfo;
				pLMI = &m_lLightMapInfo[iLM];
				int iLMC = m_lLightMapInfo[iLM].m_iLMC;

				LMWidth = pLMI->m_LMCWidthHalf*2;
				LMHeight= pLMI->m_LMCHeightHalf*2;

				fp32 LMScale = 1 << pLMI->m_ScaleShift;
				TxtWidthInvLM = 1.0f / fp32(m_lLMDimensions[iLMC].x);
				TxtHeightInvLM = 1.0f / fp32(m_lLMDimensions[iLMC].y);
				MidPixelAdjustLM = MidPixelAdjust*LMScale;
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
			for(v = 0; v < nv; v++)
			{
				CVec2Dfp32 TVLM;
				TVLM.k[0] = (lUProj[v] - TProjMin[0] + MidPixelAdjustLM) / fp32(1 << pLMI->m_ScaleShift);
				TVLM.k[1] = (lVProj[v] - TProjMin[1] + MidPixelAdjustLM) / fp32(1 << pLMI->m_ScaleShift);
				pLMTV[v][0] = (TVLM[0] + (fp32)pLMI->m_LMCOffsetXHalf*2) * TxtWidthInvLM;
				pLMTV[v][1] = (TVLM[1] + (fp32)pLMI->m_LMCOffsetYHalf*2) * TxtHeightInvLM;
			}

			CVec2Dfp32 TMid;
#ifdef PLATFORM_PS2
			TMid[0] = RoundToInt(TProjMin[0] * TxtWidthInv);
			TMid[1] = RoundToInt(TProjMin[1] * TxtHeightInv);
#else
			TMid[0] = (TProjMin[0] + TProjMax[0]) * 0.5f * TxtWidthInv;
			TMid[1] = (TProjMin[1] + TProjMax[1]) * 0.5f * TxtHeightInv;
			TMid.k[0] = RoundToInt(TMid.k[0]/16.0f)*16.0f;
			TMid.k[1] = RoundToInt(TMid.k[1]/16.0f)*16.0f;
#endif

			for(v = 0; v < nv; v++)
				pTV[v] -= TMid;

			pTV += nv;
			if (pLMTV)
				pLMTV += nv;
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
			if (pF->m_Flags & XW_FACE_LIGHTVERTICES)
			{
				// Lit
				int LightID = m_pLightVerticesInfo[pF->m_iLightInfo].m_LightID;
				int nLVerts = pF->m_nLightInfo;

				if ((nLVerts > 1) || (LightID))
				{
					for(int i = 0; i < nv; pCol[i++] = 0)
					{
					}
					GetLightVertices(iFace, pCol, NULL);
//					if (m_CurColor != 0xffffffff)
//						PPA_Mul_RGBA32(m_CurColor, pCol, pCol, nv);
				}
				else
				{
					int iLVerts = m_pLightVerticesInfo[pF->m_iLightInfo].m_iLightVertices;
/*					if (m_CurColor != 0xffffffff)
					{
						PPA_Mul_RGBA32(m_CurColor, &m_pLightVertices[iLVerts], pCol, nv);
					}
					else*/
					{
						for(int i = 0; i < nv; i++)
							pCol[i] = m_pLightVertices[iLVerts + i];
					}
				}
			}
			else
			{
				for(int v = 0; v < nv; v++) pCol[v] = 0xffffffff;
			}
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

int CXR_Model_BSP::RenderTesselatePrim(const uint32* _piFaces, int _nFaces, uint16* _pPrim)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderTesselatePrim, 0);
#ifdef MODEL_BSP_TRIANGLEFAN
	int nP = 0;

	const uint32* piF = _piFaces;
	for(int i = 0; i < _nFaces; i++)
	{
		int iFace = *(piF++);
		const CBSP_Face* pF = &m_lFaces[iFace];
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
		const CBSP_Face* pF = &m_lFaces[iFace];
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


int CXR_Model_BSP::RenderTesselatePrim_KnitStrip(const uint32* _piFaces, int _nFaces, uint16* _pPrim)
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

		const CBSP_Face* pF = &m_lFaces[iFace];

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
		const CBSP_Face* pF = &m_lFaces[iFace];
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


bool CXR_Model_BSP::RenderTesselateVBPrim(const uint32* _piFaces, int _nFaces, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB)
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


bool CXR_Model_BSP::RenderTesselate(const uint32* _piFaces, int _nFaces, int _TessFlags, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, uint32* _piFaceVertices)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderTesselate, false);
	int nVert = 0;
	int nPrim = 0;
	RenderGetPrimCount(_piFaces, _nFaces, nVert, nPrim);

	CVec3Dfp32* pV = _pVBM->Alloc_V3(nVert);
	uint16* pPrim = _pVBM->Alloc_Int16(nPrim);
	if (!pV || !pPrim) return false;

	uint16* pPrimTmp = pPrim;

	if (!_pVB->AllocVBChain(ms_pVBM, false))
		return false;
	_pVB->Geometry_VertexArray(pV, nVert, true);
//	CVec2Dfp32* pTV = NULL;
//	CVec2Dfp32* pLMTV = NULL;
	CPixel32* pC = NULL;
	CVec3Dfp32* pN = NULL;

	if (_TessFlags & 3)
	{
		ConOut(CStrF("§cf80WARNING: (CXR_Model_BSP::RenderTesselate) TessFlags %d", _TessFlags));
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
		const CBSP_Face* pF = &m_pFaces[iFace];
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
			if (pF->m_Flags & XW_FACE_LIGHTVERTICES)
			{
				// Lit
				int LightID = m_pLightVerticesInfo[pF->m_iLightInfo].m_LightID;
				int nLVerts = pF->m_nLightInfo;

				if ((nLVerts > 1) || (LightID))
				{
					for(int i = 0; i < nv; pCol[i++] = 0)
					{
					}
					GetLightVertices(iFace, pCol, NULL);
//					if (m_CurColor != 0xffffffff)
//						PPA_Mul_RGBA32(m_CurColor, pCol, pCol, nv);
				}
				else
				{
					int iLVerts = m_pLightVerticesInfo[pF->m_iLightInfo].m_iLightVertices;
/*					if (m_CurColor != 0xffffffff)
					{
						PPA_Mul_RGBA32(m_CurColor, &m_pLightVertices[iLVerts], pCol, nv);
					}
					else*/
					{
						for(int i = 0; i < nv; i++)
							pCol[i] = m_pLightVertices[iLVerts + i];
					}
				}
			}
			else
			{
				for(int v = 0; v < nv; v++) pCol[v] = 0xffffffff;
			}
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

void CXR_Model_BSP::RenderSurface(CMTime _AnimTime, CXW_Surface* _pSurf, CXW_SurfaceKeyFrame* _pSurfKey, 
	CXR_VertexBuffer* _pVB, int _LightMapTextureID, bool _bAddLight)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderSurface, MAUTOSTRIP_VOID);
	int Flags = /*RENDERSURFACE_MATRIXSTATIC_M2W | RENDERSURFACE_MATRIXSTATIC_W2V |*/ RENDERSURFACE_MATRIXSTATIC_M2V;// | RENDERSURFACE_VERTEXFOG;
	if (ms_Enable & MODEL_BSP_ENABLE_FULLBRIGHT) Flags |= RENDERSURFACE_FULLBRIGHT;
	if (_bAddLight) Flags |= RENDERSURFACE_ADDLIGHT;

	fp32 BasePriority = ms_BasePriority_Opaque;
	fp32 Offset = CXR_VBPRIORITY_MODEL_LAYEROFFSET;

	if (_pSurf->m_Flags & XW_SURFFLAGS_TRANSPARENT)
		BasePriority = ms_BasePriority_Transparent;

	CXR_RenderSurfExtParam Params;
	Params.m_lUserColors[0] = m_pCurAnimState->m_Data[0];
	Params.m_lUserColors[1] = m_pCurAnimState->m_Data[1];
	Params.m_TextureIDLightMap = _LightMapTextureID;

	CXR_Util::Render_Surface(Flags, _AnimTime, _pSurf, _pSurfKey, ms_pCurrentEngine, ms_pVBM, ms_pVBMatrixM2W, ms_pVBMatrixW2V, ms_pVBMatrixM2V, _pVB, 
		BasePriority, Offset, &Params);
}

void CXR_Model_BSP::VB_CopyGeometry(int _iVB, CBSP_VBChain* _pVB)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_CopyGeometry, MAUTOSTRIP_VOID);
	CBSP_VBInfo* pBSPVBI = m_lspVBInfo[_iVB];

	if (ms_CurrentTLEnable && pBSPVBI->m_VBID >= 0)
	{
		CXR_VBIDChain *pChain = ms_pVBM->Alloc_VBIDChain();
		pChain->m_VBID = pBSPVBI->m_VBID;
		_pVB->AddChain(pChain);
	}
	else
	{
		CBSP_VertexBuffer* pBSPVB = GetVertexBuffer(_iVB);

		CXR_VBChain *pChain = ms_pVBM->Alloc_VBChain();

		_pVB->AddChain(pChain);

		pChain->m_pV = pBSPVB->m_lV.GetBasePtr();
		pChain->m_nV = pBSPVB->m_lV.Len();
		pChain->m_pN = pBSPVB->m_lN.GetBasePtr();
		pChain->m_pCol = pBSPVB->m_lCol.GetBasePtr();
		pChain->m_pTV[0] = (fp32*) pBSPVB->m_lTV1.GetBasePtr();
		pChain->m_pTV[1] = (fp32*) pBSPVB->m_lTV2.GetBasePtr();
		if (!pChain->m_pTV[1])
			pChain->m_pTV[1] = pChain->m_pTV[0];
		pChain->m_pTV[2] = (fp32*) pBSPVB->m_lTangU.GetBasePtr();
		pChain->m_pTV[3] = (fp32*) pBSPVB->m_lTangV.GetBasePtr();
		pChain->m_nTVComp[0] = 2;
		pChain->m_nTVComp[1] = 2;
		pChain->m_nTVComp[2] = 3;
		pChain->m_nTVComp[3] = 3;
	}
}


void CXR_Model_BSP::VB_RenderFaces(int _iVB, const uint32* _piFaces, int _nFaces, CXW_Surface* _pSurf, bool _bAddLight)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_RenderFaces, MAUTOSTRIP_VOID);
	if (!_nFaces) return;
	if (_pSurf->m_Flags & XW_SURFFLAGS_INVISIBLE) return;
	MIncProfile(ms_nSurfSwitch);

//	int bTLEnable = MODEL_BSP_TL_ENABLE_CONDITION;

	// Set current-surface. No function is using it at the moment though.
	m_pCurSurface = _pSurf;
//	CXW_SurfaceKeyFrame* pSurfKey = _pSurf->GetBaseFrame();
	CMTime AnimTime;
	CXW_SurfaceKeyFrame* pSurfKey;
	if (m_pCurAnimState) 
	{
		AnimTime = _pSurf->m_iController ? m_pCurAnimState->m_AnimTime1 : m_pCurAnimState->m_AnimTime0;
		pSurfKey = _pSurf->GetFrame(
				_pSurf->m_iController ? m_pCurAnimState->m_Anim1 : m_pCurAnimState->m_Anim0, 
				AnimTime, &m_TmpSurfKeyFrame);
	}
	else
		pSurfKey = _pSurf->GetBaseFrame();

	int iFace = _piFaces[0];
	int LMTextureID = 0;
	if (m_pFaces[iFace].m_Flags & XW_FACE_LIGHTMAP)
	{
		int iLMC = m_lLightMapInfo[m_pFaces[iFace].m_iLightInfo].m_iLMC;
		LMTextureID = m_spLMTC->GetTextureID(iLMC);
	}

	CXR_VertexBuffer VB;

	CBSP_VBChain Chain;
	VB_CopyGeometry(_iVB, &Chain);
	Chain.SetToVB(&VB);

	if (!RenderTesselateVBPrim(_piFaces, _nFaces, ms_pVBM, &VB))
		return;

	RenderSurface(AnimTime, _pSurf, pSurfKey, &VB, LMTextureID, _bAddLight);
	m_TmpSurfKeyFrame.m_lTextures.Clear();
#ifdef XW_FOGMASK_ENABLE
	m_TmpSurfKeyFrame.m_FogMaskName = "";
#endif
}

void CXR_Model_BSP::VB_ClearFaceQueues()
{
	MAUTOSTRIP(CXR_Model_BSP_VB_ClearFaceQueues, MAUTOSTRIP_VOID);
	CVBFaceQueue* pQ = m_lVBFaceQueues.GetBasePtr();
	for(int i = 0; i < m_lVBFaceQueues.Len(); i++)
	{
		pQ[i].m_nElem = 0;
		pQ[i].m_DynamicLightMask = 0;
	}
}

/*void CXR_Model_BSP::VB_RenderFaces(int _iVB, const uint32* _piFaces, int _nFaces, CXW_Surface* _pSurf)
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

void CXR_Model_BSP::VB_RenderFaceQueues()
{
	MAUTOSTRIP(CXR_Model_BSP_VB_RenderFaceQueues, MAUTOSTRIP_VOID);

	TMeasureProfile(ms_TimeBrushVB);

	const int MaxFaces = 2048;
	uint32 liLitFaces[MaxFaces];
	uint32 liUnlitFaces[MaxFaces];
	uint32 liLitFaces2[MaxFaces];

	{
		CVBFaceQueue* pQ = m_lVBFaceQueues.GetBasePtr();
		for(int iVB = 0; iVB < m_lVBFaceQueues.Len(); iVB++)
		{
			const uint32* pFQ = pQ[iVB].m_pQueue;
			int nFQ = pQ[iVB].m_nElem;

			while(nFQ)
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
				VB_RenderFaces(iVB, liLitFaces, nLit, m_lspSurfaces[m_lspVBInfo[iVB]->m_iSurface]->GetSurface(ms_SurfOptions, ms_SurfCaps), true);

				if (nLit)
				{
					for(int iDynamic = 0; iDynamic < m_spTempWLS->m_nDynamic; iDynamic++)
					{				
						int CurMask = (1 << iDynamic);
						if (TotalMask & CurMask)
						{
							CXR_Light* pL = &m_spTempWLS->m_lDynamic[iDynamic];

							int nLit2 = 0;
							for(int j = 0; j < nLit; j++)
							{
								int iFace = liLitFaces[j];
								if (CurMask & m_lFaceLightMask[iFace])
									liLitFaces2[nLit2++] = iFace;
							}

							if (nLit2)
							{
								CXR_VertexBuffer* pVB = ms_pVBM->Alloc_VB();
								if (!pVB) continue;
								pVB->Matrix_Set(ms_pVBMatrixM2V);
#ifndef PLATFORM_PS2
								if (/*ms_Enable & 1024 &&*/
									ms_pCurrentEngine->m_RenderCaps_nMultiTexture >= 2 ||
									ms_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20)
								{
									CBSP_VBChain Chain;
									VB_CopyGeometry(iVB, &Chain);
									Chain.SetToVB(pVB);
									if (RenderTesselateVBPrim(liLitFaces2, nLit2, ms_pVBM, pVB))
									{
										CBSP_VBInfo* pBSPVBI = m_lspVBInfo[iVB];
										int bAdditive = (pL->m_Flags & CXR_LIGHT_HINT_ADDITIVE);
										pVB->m_Priority = ms_BasePriority_Opaque + fp32(iDynamic) * 0.01f + ((bAdditive) ? CXR_VBPRIORITY_POSTDYNLIGHT : CXR_VBPRIORITY_DYNLIGHT);
										if (ms_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20)
										Light_ProjectDynamic_NV20(pVB, iDynamic, pL, m_lspSurfaces[pBSPVBI->m_iSurface]);
										else
											Light_ProjectDynamic_MultiTexture(pVB, iDynamic, pL, m_lspSurfaces[pBSPVBI->m_iSurface]);
										ms_pVBM->AddVB(pVB);
									}
								}
								else
#endif
								{
									if (Light_ProjectDynamic(nLit2, liLitFaces2, pL, pVB))
									{
#ifdef PLATFORM_PS2
										pVB->m_Priority = ms_BasePriority_Opaque + CXR_VBPRIORITY_DYNLIGHT;
										pVB->m_pAttrib->Attrib_TextureID(0, ms_TextureID_DynLightProj);
										pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_NONE);
										pVB->m_pAttrib->Attrib_TexEnvMode( 0, CRC_PS2_TEXENVMODE_ADD | CRC_PS2_TEXENVMODE_MODULATE | CRC_PS2_TEXENVMODE_COLORBUFFER | CRC_PS2_TEXENVMODE_RGBMASK | CRC_PS2_TEXENVMODE );
										pVB->m_pAttrib->m_nChannels = 1;
#else
										int bAdditive = (pL->m_Flags & CXR_LIGHT_HINT_ADDITIVE);
										pVB->m_Priority = ms_BasePriority_Opaque + ((bAdditive) ? CXR_VBPRIORITY_POSTDYNLIGHT : CXR_VBPRIORITY_DYNLIGHT);
										pVB->m_pAttrib->Attrib_TextureID(0, ms_TextureID_DynLightProj);
										pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ADD);
#endif
										if (ms_pSceneFog) ms_pSceneFog->SetDepthFogBlack(pVB->m_pAttrib);
										ms_pVBM->AddVB(pVB);
									}
								}
							}
						}
					}
				}

				nFQ -= nElem;
				pFQ += nElem;
			}
		}
	}
}

//#define MODEL_BSP_CULLPORTALPLANES
#define MODEL_BSP_BRUTEFORCE


void CXR_Model_BSP::VB_ClearQueues()
{
	MAUTOSTRIP(CXR_Model_BSP_VB_ClearQueues, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_lVBQueues.Len(); i++)
	{
		m_lVBQueues[i].Clear();
	}
}

int ConvertWLSToCRCLight(CXR_WorldLightState& _WLS, CXR_VBManager& _VBM, CRC_Light*& _pLights, fp32 _LightScale);

void CXR_Model_BSP::VB_RenderQueues()
{
	MAUTOSTRIP(CXR_Model_BSP_VB_RenderQueues, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP::VB_RenderQueues); //AR-SCOPE

	TMeasureProfile(ms_TimeSurf);

	CXR_RenderSurfExtParam Params;
	Params.m_lUserColors[0] = m_pCurAnimState->m_Data[0];
	Params.m_lUserColors[1] = m_pCurAnimState->m_Data[1];

	// (m_lPortalLeaves.Len() == 0) removed due to Cannon01.xw -JA
	bool bVertexLighting = 
		/*(m_lPortalLeaves.Len() == 0) && */
		m_pCurAnimState && 
		(m_pCurAnimState->m_AnimAttr0 > 0.0f);

	if (bVertexLighting)
	{
// FIXME: Broken
//		fp32 LightScale = ((ms_pCurrentEngine) ? ms_pCurrentEngine->m_LightScale : 1.0f) * 1.0f;
//		Params.m_nLights = ConvertWLSToCRCLight(*m_spTempWLS, *ms_pVBM, Params.m_pLights, LightScale);
	}

	for(int i = 0; i < m_lVBQueues.Len(); i++)
	{
		CBSP_VBQueue* pQueue = &m_lVBQueues[i];
		CBSP_VBChain *pVBChain = &pQueue->m_VBQueue;
		if (!pVBChain->m_Chain) 
			continue;
		CXW_Surface* pSurf = m_lspSurfaces[pQueue->m_iSurface]->GetSurface(ms_SurfOptions, ms_SurfCaps);
		if (pSurf->m_Flags & XW_SURFFLAGS_INVISIBLE) 
			continue;

		int Flags = /*RENDERSURFACE_MATRIXSTATIC_M2W | RENDERSURFACE_MATRIXSTATIC_W2V |*/ RENDERSURFACE_MATRIXSTATIC_M2V;// | RENDERSURFACE_VERTEXFOG;
		if (ms_Enable & MODEL_BSP_ENABLE_FULLBRIGHT) Flags |= RENDERSURFACE_FULLBRIGHT;
//		if (_bAddLight) Flags |= RENDERSURFACE_ADDLIGHT;

		fp32 BasePriority = ms_BasePriority_Opaque;
		fp32 Offset = 1.0f;
		if (pSurf->m_Flags & XW_SURFFLAGS_TRANSPARENT)
		{
			BasePriority = ms_BasePriority_Transparent;
			Offset = 0.0001f;
		}

		CMTime AnimTime;
		CXW_SurfaceKeyFrame* pSurfKey = NULL;
		if (m_pCurAnimState) 
		{
			AnimTime = (pSurf->m_iController) ? m_pCurAnimState->m_AnimTime1 : m_pCurAnimState->m_AnimTime0;
			pSurfKey = pSurf->GetFrame(
					pSurf->m_iController ? m_pCurAnimState->m_Anim1 : m_pCurAnimState->m_Anim0, 
					AnimTime, &m_TmpSurfKeyFrame);
		}
		else
		{
			pSurfKey = pSurf->GetBaseFrame();
		}

		if (!bVertexLighting)
			Params.m_TextureIDLightMap = (m_spLMTC != NULL) ? m_spLMTC->GetTextureID(pQueue->m_LMTextureID) : 0;
		else
			Params.m_TextureIDLightMap = 0;

		CXR_VertexBuffer VB;
		pVBChain->SetToVB(&VB);
		CXR_Util::Render_Surface(Flags, AnimTime, pSurf, pSurfKey, ms_pCurrentEngine, ms_pVBM, ms_pVBMatrixM2W, ms_pVBMatrixW2V, ms_pVBMatrixM2V, &VB, 
			BasePriority, Offset, &Params);

		m_TmpSurfKeyFrame.m_lTextures.Clear();
#ifdef XW_FOGMASK_ENABLE
		m_TmpSurfKeyFrame.m_FogMaskName = "";
#endif
	}
}

void CXR_Model_BSP::VB_Flush()
{
	MAUTOSTRIP(CXR_Model_BSP_VB_Flush, MAUTOSTRIP_VOID);
	for(int iVB = 0; iVB < m_lspVBInfo.Len(); iVB++)
	{
		CBSP_VBInfo* pBSPVBI = m_lspVBInfo[iVB];
		if (!pBSPVBI->m_pPrim || !pBSPVBI->m_nPrim) continue;

		CBSP_VBChain Chain;
		VB_CopyGeometry(iVB, &Chain);

		int iP = pBSPVBI->m_nPrim;
		M_ASSERT(iP > 1, "!?");
		M_ASSERT(pBSPVBI->m_pPrim[0] != 0x0100, "!?");

		pBSPVBI->m_pPrim[iP++] = CRC_RIP_END + (1 << 8);
		if (Chain.m_bVBIds)
		{
			Chain.GetVBIDChain()->Render_IndexedPrimitives(pBSPVBI->m_pPrim, iP);
			m_lVBQueues[pBSPVBI->m_iVBQueue].AddChain(Chain.GetVBIDChain());
		}
		else
		{
			Chain.GetVBChain()->Render_IndexedPrimitives(pBSPVBI->m_pPrim, iP);
			m_lVBQueues[pBSPVBI->m_iVBQueue].AddChain(Chain.GetVBChain());
		}


		pBSPVBI->Clear();
	}
}

#ifdef CPU_ALIGNED_MEMORY_ACCESS
static inline void WriteUnalignedPtr( void *_pMem, const void *_pPtr )
{
	MAUTOSTRIP(WriteUnalignedPtr, MAUTOSTRIP_VOID);
	memcpy(_pMem, &_pPtr, sizeof(_pPtr));
}

void CXR_Model_BSP::VB_AddPrimitives(int _iVB, const uint16* _pPrim, int _nPrim)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_AddPrimitives, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP::VB_AddPrimitives);
	CBSP_VBInfo* pVBI = m_lspVBInfo[_iVB];

	M_ASSERT(_nPrim > 1, "?!");

	if (!pVBI->m_pPrim)
		if (!pVBI->AllocPrimitives(ms_pVBM)) return;

	if (pVBI->m_nPrim + (2 + sizeof(void *) / 2) > pVBI->m_MaxPrim)
	{
		ConOut(CStrF("(CXR_Model_BSP2::VB_AddPrimitives) Insufficient primitive space. %d+%d > %d", pVBI->m_nPrim, _nPrim, pVBI->m_MaxPrim));
		return;
	}

	pVBI->m_pPrim[pVBI->m_nPrim] = CRC_RIP_PRIMLIST | ((2 + (sizeof(void *) / 2)) << 8);
	WriteUnalignedPtr( (const uint16 **)&pVBI->m_pPrim[pVBI->m_nPrim + 1], _pPrim );
	pVBI->m_pPrim[pVBI->m_nPrim + 1 + (sizeof(void *) / 2)] = _nPrim;
	pVBI->m_nPrim += 2 + (sizeof(void *) / 2);
}
#else
void CXR_Model_BSP::VB_AddPrimitives(int _iVB, const uint16* _pPrim, int _nPrim)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_AddPrimitives_2, MAUTOSTRIP_VOID);
	CBSP_VBInfo* pVBI = m_lspVBInfo[_iVB];

	M_ASSERT(_nPrim > 1, "?!");

	if (!pVBI->m_pPrim)
		if (!pVBI->AllocPrimitives(ms_pVBM)) return;

	if (pVBI->m_nPrim + (2 + sizeof(void *) / 2) > pVBI->m_MaxPrim)
	{
		ConOut(CStrF("(CXR_Model_BSP2::VB_AddPrimitives) Insufficient primitive space. %d+%d > %d", pVBI->m_nPrim, _nPrim, pVBI->m_MaxPrim));
		return;
	}

	pVBI->m_pPrim[pVBI->m_nPrim] = CRC_RIP_PRIMLIST | ((2 + (sizeof(void *) / 2)) << 8);
	*((const uint16 **)&pVBI->m_pPrim[pVBI->m_nPrim + 1]) = _pPrim;
	pVBI->m_pPrim[pVBI->m_nPrim + 1 + (sizeof(void *) / 2)] = _nPrim;
	pVBI->m_nPrim += 2 + (sizeof(void *) / 2);
}
#endif	// CPU_ALIGNED_MEMORY_ACCESS

void CXR_Model_BSP::VB_Tesselate(int _iVB, const uint32* _piFaces, int _nFaces)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_Tesselate, MAUTOSTRIP_VOID);
	if (!_nFaces)
		return;

	int nV, nP;

	RenderGetPrimCount( _piFaces, _nFaces, nV, nP);
	uint16* pPrim = ms_pVBM->Alloc_Int16(nP);
	if (!pPrim)
		return;
	int nPrim = RenderTesselatePrim(_piFaces, _nFaces, pPrim);

	M_ASSERT(nPrim > 0, "!");
	M_ASSERT(nPrim == nP, "!");
	VB_AddPrimitives(_iVB, pPrim, nPrim);

/*	CBSP_VBInfo* pVBI = m_lspVBInfo[_iVB];

	if (!pVBI->m_pPrim)
		if (!pVBI->AllocPrimitives(ms_pVBM)) return;
	
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
void CXR_Model_BSP::VB_AllocVBID()
{
	MAUTOSTRIP(CXR_Model_BSP_VB_AllocVBID, MAUTOSTRIP_VOID);
/*	if (!m_lPortals.Len())
	{
		VBC_Remove();
		return;
	}*/

	for(int i = 0; i < m_lspVBInfo.Len(); i++)
	{
		m_lspVBInfo[i]->m_VBID = m_pVBCtx->AllocID(m_iVBC, i);
	}
}

void CXR_Model_BSP::VB_FreeVBID()
{
	MAUTOSTRIP(CXR_Model_BSP_VB_FreeVBID, MAUTOSTRIP_VOID);
	if (m_iVBC < 0) return;

	for(int i = 0; i < m_lspVBInfo.Len(); i++)
	{
		if (m_lspVBInfo[i]->m_VBID >= 0)
			m_pVBCtx->FreeID(m_lspVBInfo[i]->m_VBID);
	}
}


int CXR_Model_BSP::GetNumLocal()
{
	MAUTOSTRIP(CXR_Model_BSP_GetNumLocal, 0);
//	if (!m_lPortals.Len()) return 0;
	return m_lspVBInfo.Len();
}

int CXR_Model_BSP::GetID(int _iLocal)
{
	MAUTOSTRIP(CXR_Model_BSP_GetID, 0);
	return m_lspVBInfo[_iLocal]->m_VBID;
}

void CXR_Model_BSP::Get(int _iLocal, CRC_BuildVertexBuffer& _VB, int _Flags)
{
	MAUTOSTRIP(CXR_Model_BSP_Get, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP::Get, XR_BSPMODEL);
	CBSP_VertexBuffer* pBSPVB = GetVertexBuffer(_iLocal);

	if (_Flags & VB_GETFLAGS_FALLBACK)
		m_lspVBInfo[_iLocal]->m_bNoRelease = 1;

	_VB.Clear();

	FillChar(pBSPVB->m_lCol.GetBasePtr(), pBSPVB->m_lCol.Len()*4, -1);

	_VB.Geometry_VertexArray(pBSPVB->m_lV.GetBasePtr());
	_VB.m_nV = pBSPVB->m_lV.Len();
	if (pBSPVB->m_lN.GetBasePtr())
		_VB.Geometry_NormalArray(pBSPVB->m_lN.GetBasePtr());
	if (pBSPVB->m_lCol.GetBasePtr())
		_VB.Geometry_ColorArray(pBSPVB->m_lCol.GetBasePtr());

	if (pBSPVB->m_lTV1.GetBasePtr())
		_VB.Geometry_TVertexArray(pBSPVB->m_lTV1.GetBasePtr(), 0);

	if (pBSPVB->m_lTV2.GetBasePtr())
		_VB.Geometry_TVertexArray(pBSPVB->m_lTV2.GetBasePtr(), 1);

	if (!_VB.m_lpVReg[CRC_VREG_TEXCOORD1] && pBSPVB->m_lTV1.GetBasePtr())
		_VB.Geometry_TVertexArray(pBSPVB->m_lTV1.GetBasePtr(), 0);

	if (pBSPVB->m_lTangU.GetBasePtr())
		_VB.Geometry_TVertexArray(pBSPVB->m_lTangU.GetBasePtr(), 2);
	if (pBSPVB->m_lTangV.GetBasePtr())
		_VB.Geometry_TVertexArray(pBSPVB->m_lTangV.GetBasePtr(), 3);

	//AR-ADD: calculate bounding box
//	_VBGetOptions.m_Bound_Vertex = (m_spMaster ? m_spMaster->m_BoundBox : m_BoundBox);

/*	if (_pBoundBox)
		*_pBoundBox = (m_spMaster ? m_spMaster->m_BoundBox : m_BoundBox);*/
}

void CXR_Model_BSP::Release(int _iLocal)
{
	MAUTOSTRIP(CXR_Model_BSP_Release, MAUTOSTRIP_VOID);
	if (!m_lspVBInfo.ValidPos(_iLocal))
		Error("Release", CStrF("Invalid local ID %d", _iLocal));

	if (!m_lspVBInfo[_iLocal]->m_bNoRelease)
	{
		m_lspVB[_iLocal] = NULL;
	}
}

// -------------------------------------------------------------------
void CXR_Model_BSP::RenderNHF(const uint32* _piFaces, int _nFaces, int _iPL)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderNHF, MAUTOSTRIP_VOID);
	if (_nFaces > MODEL_BSP_MAXPORTALLEAFFACES)
	{
		ConOut(CStrF("(CXR_Model_BSP::RenderNHF) Too many faces. %d/%d", _nFaces, MODEL_BSP_MAXPORTALLEAFFACES));
		return;
	}

	int iBaseV = 0;
	if (_iPL)
	{
		Fog_TraceVertices(_piFaces, _nFaces, _iPL);
		UntagFogVertices();

		const CBSP_PortalLeafExt* pPL = &m_pPortalLeaves[_iPL];
		iBaseV = pPL->m_iBaseVertex;
	}
	else
	{
		Fog_TraceVertices(_piFaces, _nFaces);
		UntagFogVertices();
	}

	uint32 liVisFaceVerts[MODEL_BSP_MAXPORTALLEAFFACES];

	CXR_VertexBuffer* pVB = ms_pVBM->Alloc_VB();
	if (pVB)
	{
		if (RenderTesselate(_piFaces, _nFaces, 0, ms_pVBM, pVB, liVisFaceVerts))
		{
			pVB->m_pAttrib = ms_pVBM->Alloc_Attrib();
			if (!pVB->m_pAttrib) return;

			Fog_SetVertices(pVB, _piFaces, _nFaces, liVisFaceVerts, iBaseV);

			pVB->m_pAttrib->SetDefault();
			if (ms_pCurrentEngine) ms_pCurrentEngine->SetDefaultAttrib(pVB->m_pAttrib);
			ms_pSceneFog->SetDepthFogNone(pVB->m_pAttrib);

			if (m_pCurMedium->m_MediumFlags & XW_MEDIUM_FOGADDITIVE)
				pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
			else
				pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

			pVB->m_pAttrib->Attrib_ZCompare(CRC_COMPARE_EQUAL);
			pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
			pVB->m_pAttrib->Attrib_TextureID(0, ms_pSceneFog->m_FogTableTextureID);
			pVB->Matrix_Set(ms_pVBMatrixM2V);
			pVB->m_Priority = ms_BasePriority_Opaque + CXR_VBPRIORITY_VOLUMETRICFOG;
			ms_pVBM->AddVB(pVB);
		}
	}
}

void CXR_Model_BSP::VB_RenderNHF(int _iVB, const uint16* _piPrim, int _nPrim, CBSP_PortalLeafExt* _pPL)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_RenderNHF, MAUTOSTRIP_VOID);
	// Note: _piPrim is not CRC_RIP_END terminated.

	CXR_VertexBuffer* pVB = ms_pVBM->Alloc_VB();
	if (pVB)
	{
		CBSP_VBChain Chain;
		VB_CopyGeometry(_iVB, &Chain);
		Chain.SetToVB(pVB);
		pVB->Render_IndexedPrimitives(const_cast<uint16*>(_piPrim), _nPrim);

		pVB->m_pAttrib = ms_pVBM->Alloc_Attrib();
		if (!pVB->m_pAttrib) return;

		pVB->m_pAttrib->SetDefault();
		if (ms_pCurrentEngine) ms_pCurrentEngine->SetDefaultAttrib(pVB->m_pAttrib);

		if (!Fog_SetAttrib(pVB->m_pAttrib, _pPL, m_pView->m_CurLocalVP))
			return;

		pVB->Matrix_Set(ms_pVBMatrixM2V);
		pVB->m_Priority = ms_BasePriority_Opaque + CXR_VBPRIORITY_VOLUMETRICFOG;
		ms_pVBM->AddVB(pVB);
	}
}

void CXR_Model_BSP::RenderPortalLeaf(int _iPL)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderPortalLeaf, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP::RenderPortalLeaf); //AR-SCOPE

	CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[_iPL];

	int iNode = m_pPortalLeaves[_iPL].m_iNode;
	CBSP_Node* pN = &m_pNodes[iNode];
	CRC_ClipVolume* pCurRPortal = &m_pView->m_lRPortals[m_pView->m_liLeafRPortals[pN->m_iPortalLeaf]];
	m_nVVertTag = 0;
	m_nFogTags = 0;
//	m_iFaceQueue = 0;
	m_pCurMedium = &m_lMediums[pPL->m_iMedium];
	int bFog = (m_pCurMedium->m_MediumFlags & XW_MEDIUM_FOG);

	uint32 liVisFaces[MODEL_BSP_MAXPORTALLEAFFACES];

	int nVisFaces = 0;

	if (ms_pSceneFog && ms_pSceneFog->NHFEnable())
	{
//		int bSwitch = (ms_LastMedium ^ m_pCurMedium->m_MediumFlags) & XW_MEDIUM_FOG;
		int bSwitch = (ms_LastMedium | m_pCurMedium->m_MediumFlags) & XW_MEDIUM_FOG;

		const int MAXFOGPORTALS = 32;
		uint32 iFogPortals[MAXFOGPORTALS];
		int nFogPortals = 0;
		if (bSwitch)
		{
			nFogPortals = Fog_GetVisiblePortals(_iPL, iFogPortals, MAXFOGPORTALS);
			if (!nFogPortals) bSwitch = false;
		}


//		if (m_pCurMedium->m_MediumFlags & XW_MEDIUM_FOG)
		if (bSwitch)
		{
			VB_RenderFaceQueues();
			VB_Flush();
			VB_RenderQueues();
			VB_ClearQueues();
			VB_ClearFaceQueues();
#ifndef PLATFORM_PS2
			ms_BasePriority_Opaque += CXR_VBPRIORITY_PORTALLEAFOFFSET;
			ms_BasePriority_Transparent += CXR_VBPRIORITY_PORTALLEAFOFFSET;
#endif
			if (bFog)
				Fog_RenderPortals(_iPL, iFogPortals, nFogPortals);
		}
	}

	bool bHWNHF = 
		((ms_CurrentRCCaps & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20) ||
		 (ms_CurrentRCCaps & CRC_CAPS_FLAGS_TEXGENMODE_LINEARNHF)) &&
		!(m_pCurMedium->m_MediumFlags & XW_MEDIUM_CLIPFOG);
			
	pPL->m_BasePriority = ms_BasePriority_Opaque;

	if (ms_Enable & MODEL_BSP_ENABLE_SPLINEBRUSHES)
		RenderSplineBrushes(_iPL);

	if (m_spWMC != NULL) Wallmark_Render(m_spWMC, _iPL);
	if (m_spWMCTemp != NULL) Wallmark_Render(m_spWMCTemp, _iPL);
	if (m_spWMCStatic != NULL) Wallmark_Render(m_spWMCStatic, _iPL);

	int nF = pPL->m_nFaces;
	if (!nF) return;
	const uint32* piF = &m_liPLFaces[pPL->m_iiFaces];


#ifdef MODEL_BSP_BRUTEFORCE
	// Don't cull if the view is a portal or NHF is used.
	bool bBruteForce = (m_iView == 0) && (!bFog || bHWNHF);
//	bool bBruteForce = (m_iView == 0) && !(m_pCurMedium->m_MediumFlags & XW_MEDIUM_FOG);

#else
	bool bBruteForce = false;

#endif

	// Find planes intersecting face boundbox.
	CPlane3Dfp32 Planes[16];
	int nP = 0;

	if (!bBruteForce)
	{
		int nPlanes = 0;
		const CPlane3Dfp32* pP = pCurRPortal->m_Planes;
		nP = pCurRPortal->m_nPlanes;

		if (nF > 20)
		{
			CVec3Dfp32 Center;
			pPL->m_FaceBoundBox.m_Min.Lerp(pPL->m_FaceBoundBox.m_Max, 0.5f, Center);

			for(int p = 0; p < nP; p++)
				if (pP[p].Distance(Center) > 0.0f)
					Planes[nPlanes++] = pP[p];
				else
					if (pP[p].GetBoxMaxDistance(pPL->m_FaceBoundBox.m_Min, pPL->m_FaceBoundBox.m_Max) > MODEL_BSP_EPSILON)
						Planes[nPlanes++] = pP[p];

			pP = Planes;
			nP = nPlanes;
		}
		else if (nF > 6)
		{
			CVec3Dfp32 Center;
			pPL->m_FaceBoundBox.m_Min.Lerp(pPL->m_FaceBoundBox.m_Max, 0.5f, Center);

			for(int p = 0; p < nP; p++)
				if (pP[p].Distance(Center) > 0.0f)
					Planes[nPlanes++] = pP[p];

			pP = Planes;
			nP = nPlanes;
		}
		else
			bBruteForce = true;
	}

/*	if (nPlanes)
	{
		ConOut("PortalLeaf skipped.");
		return;
	}*/
//	if (nF > 20) ConOut(CStrF("PL planes %d/%d (nf %d)", nPlanes, pCurRPortal->m_nPlanes, nF));

/*	int i = 0;
	while(i < nF)
	{
		int iSurf = m_pFaces[piF[i]].m_iSurface;
		int iStart = i;
		while(i < nF && m_pFaces[piF[i]].m_iSurface == iSurf)
		{
			Portal_RenderFace(piF[i], Planes, nPlanes);
			i++;
		}
	}*/

	if (!bBruteForce)
	{
	/*	if (nP && (nP < 4))
		{
			for(int i = 0; i < nF; i++)
				Portal_RenderFace(piF[i], pP, nP);
		}
		else
		{
			for(int i = 0; i < nF; i++)
			{
				int _iFace = piF[i];
				CBSP_Face* pFace = &m_pFaces[_iFace];
				int Flags = pFace->m_Flags;
				if (!(Flags & XW_FACE_VISIBLE)) continue;
				m_nFacesChecked++;
				// Backfacing?
				if (m_pPlanes[pFace->m_iPlane].Distance(m_pView->m_CurLocalVP) < 0.0f) continue;
				m_nFacesFront++;
				m_nFacesVisible++;
				if (m_iFaceQueue < m_FaceQueueLen)
					m_piFaceQueue[m_iFaceQueue++] = _iFace;
			}
		}
	*/
		// Cull vertices
		if (nP)
		{
			int nV = pPL->m_nVertices;
			int iVBase = pPL->m_iBaseVertex;

			const CVec3Dfp32* pV = &m_pVertices[iVBase];

			int AndMask = -1;
			int OrMask = 0;
			for(int v = 0; v < nV; v++)
			{
//				int iV = v + iVBase;
				int Mask = 0;
				int m = 1;
				for(int iP = 0; iP < nP; iP++)
				{
					if (Planes[iP].Distance(*pV) > 0.0f) Mask |= m;
					m <<= 1;
				}
				m_pVVertMask[v] = Mask;
				AndMask &= Mask;
				OrMask |= Mask;
				pV++;
			}
			if (AndMask)
			{
	//if (nF > 20) ConOut(CStrF("Leaf culled %d/%d (nf %d)", nPlanes, pCurRPortal->m_nPlanes, nF));
				return;
			}

			if (OrMask)
			{
				for(int i = 0; i < nF; i++)
				{
					int _iFace = piF[i];
					CBSP_Face* pFace = &m_pFaces[_iFace];
					int Flags = pFace->m_Flags;
					if (!(Flags & XW_FACE_VISIBLE)) continue;
					if (m_lspSurfaces[pFace->m_iSurface]->m_Flags & XW_SURFFLAGS_REFLECTION) continue;

					MIncProfile(ms_nFacesChecked);
					// Backfacing?
#ifdef MODEL_BSP_BACKFACECULL
					if (m_pPlanes[pFace->m_iPlane].Distance(m_pView->m_CurLocalVP) < 0.0f) continue;
#endif
					MIncProfile(ms_nFacesFront);

					int iiv = pFace->m_iiVertices;
					int nv = pFace->m_nVertices;
					const uint32* piV = &m_piVertices[iiv];
					int Mask = m_pVVertMask[(*piV) - iVBase];
					for(int v = nv-1; Mask && v; v--)
					{
						piV++;
						Mask &= m_pVVertMask[(*piV) - iVBase];
						if (!Mask) goto Draw;
					}
					if (Mask) continue;
	Draw:
					MIncProfile(ms_nFacesVisible);
					m_lVBFaceQueues[pFace->m_iVB].AddElem(_iFace);

					if (nVisFaces < MODEL_BSP_MAXPORTALLEAFFACES)
						liVisFaces[nVisFaces++] = piF[i];

	//				if (m_iFaceQueue < m_FaceQueueLen)
	//					m_piFaceQueue[m_iFaceQueue++] = _iFace;
				}
			}
			else
			{
				goto RenderAll;
			}
		}
		else
		{
	RenderAll:
	//if (nF > 20) ConOut(CStrF("Face cull skipped %d/%d (nf %d)", nPlanes, pCurRPortal->m_nPlanes, nF));
			for(int i = 0; i < nF; i++)
			{
				int _iFace = piF[i];
				CBSP_Face* pFace = &m_pFaces[_iFace];
				int Flags = pFace->m_Flags;
				if (!(Flags & XW_FACE_VISIBLE)) continue;
				MIncProfile(ms_nFacesChecked);
				// Backfacing?
#ifdef MODEL_BSP_BACKFACECULL
				if (m_pPlanes[pFace->m_iPlane].Distance(m_pView->m_CurLocalVP) < 0.0f) continue;
#endif
				MIncProfile(ms_nFacesFront);
				MIncProfile(ms_nFacesVisible);
				m_lVBFaceQueues[pFace->m_iVB].AddElem(_iFace);

				if (nVisFaces < MODEL_BSP_MAXPORTALLEAFFACES)
					liVisFaces[nVisFaces++] = piF[i];

	//			if (m_iFaceQueue < m_FaceQueueLen)
	//				m_piFaceQueue[m_iFaceQueue++] = _iFace;
			}
		}

	}
	else
	{
		if (!pPL->m_DynLightMask)
		{
			int nVBFP = pPL->m_nVBFacePrims;
			if (nVBFP)
			{
				CBSP_VBFacePrim* pVBFP = &m_lVBFacePrim[pPL->m_iVBFacePrims];
				uint16* pFacePrim = m_lFacePrim.GetBasePtr();
				for(int i = 0; i < nVBFP; i++)
				{
					if (pCurRPortal->BoxInVolume(pVBFP->m_BoundBox))
						VB_AddPrimitives(pVBFP->m_iVB, &pFacePrim[pVBFP->m_iPrim], pVBFP->m_nPrim);
					pVBFP++;
				}
			}
		}
		else
		{
			for(int i = 0; i < nF; i++)
			{
				PREFETCH(&m_pFaces[piF[i+16]].m_iPlane);
				PREFETCH(&m_pPlanes[m_pFaces[piF[i+8]].m_iPlane]);

				int _iFace = piF[i];
				CBSP_Face* pFace = &m_pFaces[_iFace];
				int Flags = pFace->m_Flags;
				if (!(Flags & XW_FACE_VISIBLE)) continue;
				MIncProfile(ms_nFacesChecked);
				// Backfacing?
#ifdef MODEL_BSP_BACKFACECULL
				if (m_pPlanes[pFace->m_iPlane].Distance(m_pView->m_CurLocalVP) < 0.0f) continue;
#endif
				if (m_lspSurfaces[pFace->m_iSurface]->m_Flags & XW_SURFFLAGS_REFLECTION) continue;

				MIncProfile(ms_nFacesFront);
				MIncProfile(ms_nFacesVisible);
				m_lVBFaceQueues[pFace->m_iVB].AddElem(_iFace);

				if (nVisFaces < MODEL_BSP_MAXPORTALLEAFFACES)
					liVisFaces[nVisFaces++] = piF[i];
			}
		}

		if (bHWNHF &&
			(m_pCurMedium->m_MediumFlags & XW_MEDIUM_FOG) &&
			ms_pSceneFog && 
			ms_pSceneFog->NHFEnable())
		{
			int nVBFP = pPL->m_nVBFacePrims;
			if (nVBFP)
			{
				CBSP_VBFacePrim* pVBFP = &m_lVBFacePrim[pPL->m_iVBFacePrims];
				uint16* pFacePrim = m_lFacePrim.GetBasePtr();
				for(int i = 0; i < nVBFP; i++)
				{
					if (pCurRPortal->BoxInVolume(pVBFP->m_BoundBox))
						VB_RenderNHF(pVBFP->m_iVB, &pFacePrim[pVBFP->m_iPrim], pVBFP->m_nPrim, pPL);
					pVBFP++;
				}
			}
		}
	}

#if !defined MODEL_BSP_DISABLENHF

	// NHF?
	if (!bHWNHF &&
		(m_pCurMedium->m_MediumFlags & XW_MEDIUM_FOG) &&
		ms_pSceneFog && 
		ms_pSceneFog->NHFEnable())
		{
			RenderNHF(liVisFaces, nVisFaces, _iPL);

			// Perform the insanely complicated ritual to disable fog-tracing for 
			// volumes that has been covered by fog-portals.
#ifdef NEVER
			for(int p = 0; p < pPL->m_nPortals; p++)
			{
				int iP = m_piPortals[pPL->m_iiPortals + p];
				CBSP_PortalExt* pP = &m_pPortals[iP];
				if (!pP->m_iFogPortal) continue;

				int iNodeOther = (pP->m_iNodeFront == iNode) ? pP->m_iNodeBack : pP->m_iNodeFront;
				int iPLOther = m_pNodes[iNodeOther].m_iPortalLeaf;

				int iRPOther = m_pView->m_liLeafRPortals[iPLOther];
				if (iRPOther)
				{
					// Other PL was visible
//					if (

				}

			}
#endif


		}

#endif
//#endif 	// VBQUEUES
}

void CXR_Model_BSP::RenderLeafList()
{
	MAUTOSTRIP(CXR_Model_BSP_RenderLeafList, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP::RenderLeafList); //AR-SCOPE

	const uint16* piVisPortalLeaves = m_pView->m_liVisPortalLeaves.GetBasePtr();

	if (m_lPortalLeaves.Len())
	{
		// Traverse back-2-front and assign portal-leaves to all splines
		// All splines will be assigned to their front-most portal-leaf.
/*		if (m_spSBLink)
		{
			for(int iiPL = m_pView->m_nVisLeaves-1; iiPL >= 0; iiPL--)
			{
				int iPL = m_pView->m_piVisPortalLeaves[iiPL];
				uint16 liSB[1024];
				int nSB = m_spSBLink->EnumIDs(iPL, liSB, 1024);
				for(int iiSB = 0; iiSB < nSB; iiSB++)
					m_lspSplineBrushes[liSB[iiSB]]->m_CurrentRenderPL = iPL;
			}
		}
*/
		// Update fog
		for(int l = m_pView->m_nVisLeaves-1; l >= 0; l--)
			Fog_BuildFogBox(piVisPortalLeaves[l]);
	}

	ms_BasePriority_Opaque = CXR_VBPRIORITY_MODEL_OPAQUE;
	ms_BasePriority_Transparent = CXR_VBPRIORITY_MODEL_TRANSPARENT;
	for(int l = m_pView->m_nVisLeaves-1; l >= 0; l--)
		RenderPortalLeaf(piVisPortalLeaves[l]);
}


void CXR_Model_BSP::UntagVertices()
{
	MAUTOSTRIP(CXR_Model_BSP_UntagVertices, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_nVVertTag; i++)
	{
		int iv = m_piVVertTag[i];
		m_pVVertMask[iv] = 0;
	}
	m_nVVertTag = 0;
}

void CXR_Model_BSP::UntagFogVertices()
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

// -------------------------------------------------------------------
void CXR_Model_BSP::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
			const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_BSP_OnRender, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP::OnRender, XR_BSPMODEL);

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
	m_pCurAnimState = _pAnimState;

	if ((m_iView < 0) || (m_iView >= m_lspViews.Len()))
		Error("Render", "Invalid view index.");
	m_pView = m_lspViews[m_iView];

	m_CurrentView++;
	m_pView->m_CurrentView = m_CurrentView;

	_WMat.Multiply(_VMat, m_pView->m_CurVMat);
	m_pView->m_CurWMat = _WMat;

#ifdef M_Profile
	ms_Time.Reset();
	ms_TimeRenderPoly1.Reset();
	ms_TimeRenderPoly2.Reset();
	ms_TimeRenderLeafList.Reset();
	ms_TimeRenderFaceQueue.Reset();
#endif
	ms_CurrentRCCaps = _pRender->Caps_Flags();
	ms_pCurrentEngine = _pEngine;
	ms_CurrentTLEnable = MODEL_BSP_TL_ENABLE_CONDITION;
	ms_bCurrentDynLightCullBackFace = MODEL_BSP_DYNLIGHTCULLBACKFACE_CONDITION;

	ms_pVBM = _pVBM;
	ms_pVBMatrixM2V = ms_pVBM->Alloc_M4(m_pView->m_CurVMat);
	ms_pVBMatrixM2W = const_cast<CMat4Dfp32*>(&_WMat); //ms_pVBM->Alloc_M4(m_pView->m_CurWMat);
	ms_pVBMatrixW2V = const_cast<CMat4Dfp32*>(&_VMat); //ms_pVBM->Alloc_M4(_VMat);

	if (!ms_pVBMatrixM2V /* || !ms_pVBMatrixM2W || !ms_pVBMatrixW2V*/) return;
	ms_LastMedium = 0;

	ms_SurfOptions = _pEngine->m_SurfOptions;
	ms_SurfCaps = _pEngine->m_SurfCaps;
//	m_spCurrentWLS = _spWLS;

	m_pView->m_CurVMat.InverseOrthogonal(m_pView->m_CurVMatInv);

	ms_RenderInfo.m_pCurrentEngine = _pEngine;
	CBox3Dfp32 BoundBox;
	Phys_GetBound_Box(_WMat, BoundBox);

	// Get bound-sphere, get the CRC_ClipView
	{
		CRC_Viewport* pVP = ms_pVBM->Viewport_Get();
		m_pView->m_ViewBackPlane = pVP->GetBackPlane();

		if (_pViewClip)
		{
			// Entity
			if (!_pViewClip->View_GetClip_Box(BoundBox.m_Min, BoundBox.m_Max, 0, 0, NULL, &ms_RenderInfo)) return;

			ms_BasePriority_Opaque = ms_RenderInfo.m_BasePriority_Opaque;
			ms_BasePriority_Transparent = ms_RenderInfo.m_BasePriority_Transparent;
		}
		else
		{
			// World
			ms_RenderInfo.Clear(_pEngine);
			ms_BasePriority_Opaque = CXR_VBPRIORITY_MODEL_OPAQUE;
			ms_BasePriority_Transparent = CXR_VBPRIORITY_MODEL_TRANSPARENT;
		}
		m_pView->m_CurLocalVP = CVec3Dfp32::GetRow(m_pView->m_CurVMatInv, 3);
	}

	if (m_lPortalLeaves.Len() ||
		ms_RenderInfo.m_Flags & (CXR_RENDERINFO_VERTEXFOG | CXR_RENDERINFO_NHF))
	{
		ms_pSceneFog = _pEngine->GetFogState();
		if (ms_pSceneFog)
		{
	//		ms_pSceneFog->SetEye(VMatInv);
			ms_pSceneFog->SetTransform(&_WMat);
		}
	}
	else
		ms_pSceneFog = NULL;


	if (!m_spTempWLS)
	{
		m_spTempWLS = MNew(CXR_WorldLightState);
		if (!m_spTempWLS)
			MemError("OnRender");
	}
	if (!m_spTempWLS->m_lStatic.Len())
		m_spTempWLS->Create(256, 16, 4);

	// Get clip mask
/*	{
		fp32 BoundR = GetBound_Sphere();
		int ViewMask = _pRender->Viewport_Get()->SphereInView(CVec3Dfp32::GetMatrixRow(m_pView->m_CurVMat, 3), BoundR);

		// FIXME:
		// If *this doesn't have a pViewClip, it's probably the world. If it's not rendered
		// there will be bugs when objects try to use it's view-interface. 
		// This prevents it from exiting.
		if (!_pViewClip) 
			if (!ViewMask) return;
	}*/

	{
		TMeasureResetProfile(ms_Time);

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

		if (!(_Flags & 1))
		{
			int nPL = Max(m_pView->m_liVisPortalLeaves.Len(), m_lPortalLeaves.Len());
			m_pView->m_liVisPortalLeaves.SetLen(nPL);
			m_pView->m_MaxVisLeaves = m_pView->m_liVisPortalLeaves.Len();
			m_pView->m_nVisLeaves = 0;
	//		m_pView->m_lPVS.SetLen((nPL + 15) >> 3);

			if (m_lPortalLeaves.Len())
			{
				// Initialize nodes RPortal look-up table.
				if (m_pView->m_liLeafRPortals.Len() != m_lPortalLeaves.Len())
					m_pView->m_liLeafRPortals.SetLen(m_lPortalLeaves.Len());
				FillChar(&m_pView->m_liLeafRPortals[0], m_pView->m_liLeafRPortals.ListSize(), 0);

				// Initialize space for RPortals.
				m_pView->m_MaxRPortals = 512;
				m_pView->m_lRPortals.SetLen(m_pView->m_MaxRPortals);
				m_pView->m_liRPortalNext.SetLen(m_pView->m_MaxRPortals);
				m_pView->m_pRPortals = m_pView->m_lRPortals.GetBasePtr();
				m_pView->m_piRPortalNext = m_pView->m_liRPortalNext.GetBasePtr();
				m_pView->m_nRPortals = 0;
			}
			else
			{
	/*			m_pView->m_MaxRPortals = 2;
				m_pView->m_lRPortals.SetLen(m_pView->m_MaxRPortals);
				m_pView->m_liRPortalNext.SetLen(m_pView->m_MaxRPortals);
				m_pView->m_pRPortals = m_pView->m_lRPortals.GetBasePtr();
				m_pView->m_piRPortalNext = m_pView->m_liRPortalNext.GetBasePtr();
				m_pView->m_nRPortals = 0;*/
			}
		}

		InitializeListPtrs();

		// (m_lPortalLeaves.Len() == 0) removed due to Cannon01.xw -JA
		bool bVertexLighting = /*(m_lPortalLeaves.Len() == 0) && */(m_pCurAnimState->m_AnimAttr0 > 0.0f);

		if (_spWLS != NULL && (_spWLS->m_nDynamic || bVertexLighting))
		{
			CMat4Dfp32 WMatInv;
			_WMat.InverseOrthogonal(WMatInv);

	/*		CXR_Light* pL = &_spWLS->m_lDynamic[0];
			for(int iL = 0; iL < _spWLS->m_nDynamic; iL++)
			{
				pL[iL].m_TransformedPos = pL[iL].m_Pos;
				pL[iL].m_TransformedPos *= WMatInv;
			}*/

			// Get lights.
			const CVec3Dfp32& WOrigin = CVec3Dfp32::GetMatrixRow(*ms_pVBMatrixM2W, 3);
			if (_spWLS != NULL)
			{
				int MaxDyn = m_lPortalLeaves.Len() ? 16 : 2;
				m_spTempWLS->CopyAndCull(_spWLS, GetBound_Sphere(), WOrigin, 3, MaxDyn);
			}
			else
				m_spTempWLS->PrepareFrame();

			if (bVertexLighting)
			{
	//			CMat4Dfp32 LTransform;
	//			ms_pVBMatrixM2W->InverseOrthogonal(LTransform);

				m_spTempWLS->AddLightVolume(ms_RenderInfo.m_pLightVolume, WOrigin);
				m_spTempWLS->InitLinks();
				m_spTempWLS->Optimize(WOrigin, GetBound_Sphere(), 0.7f, &WMatInv);
			}
			else
			{
				m_spTempWLS->Transform(WMatInv);
				Light_TagDynamics();
			}
		}

	//	if (m_spLMTC != NULL)
	//		m_spLMTC->PrepareFrame(_spWLS);

		// --------- Rendering is OK from here ------------------
#ifdef M_Profile
		ms_TimePortalAnd.Reset();
		ms_TimePortalOr.Reset();
		ms_TimePortal0.Reset();
		ms_TimePortal1.Reset();
		ms_TimePortal2.Reset();
		ms_TimePortal3.Reset();
		ms_nPortal0 = 0;
		ms_nPortal1 = 0;
		ms_nPortal2 = 0;
		ms_nPortal3 = 0;

		ms_nFacesChecked = 0;
		ms_nFacesVisible = 0;
		ms_nFacesFront = 0;
		ms_nSurfSwitch = 0;
#endif

		if (m_pPortals && !_pViewClip)
		{
			// If (flags::1) then don't calc portals, use the existing view information.
			if (!(_Flags & 1))
			{
				int iNodePortal = GetPortalLeaf(m_pView->m_CurLocalVP);
				if (iNodePortal == -1)
				{
			//		Error("Render", "No portal leaf!");
					ConOut("No portal leaf!");
					goto RenderAll;
				}

				{
					m_pView->m_nRPortals = 1;
					m_pView->m_pRPortals[m_pView->m_nRPortals] = m_pView->m_CurClipVolume;
					m_pView->m_piRPortalNext[m_pView->m_nRPortals] = 0;
					m_pView->m_nRPortals = 2;
					m_pView->m_liLeafRPortals[m_pNodes[iNodePortal].m_iPortalLeaf] = 1;
				}

				{
					TMeasureResetProfile(ms_TimePortals);
					m_pView->m_pPVS = SceneGraph_PVSLock(0, m_lNodes[iNodePortal].m_iPortalLeaf);
					DisableTree(1);
					EnableTreeFromNode(iNodePortal);
					Portal_AddNode(1, 1);
					DisableTree(1);
					SceneGraph_PVSRelease(m_pView->m_pPVS);
				}
			}

			if (!m_pView->m_nVisLeaves)
				goto RenderAll;

			{
				TMeasureResetProfile(ms_TimeRenderLeafList);

#ifdef M_Profile
				ms_TimeSplineVB.Reset();
				ms_TimeBrushVB.Reset();
				ms_TimeSurf.Reset();
				ms_TimeCull.Reset();
#endif
				{
					TMeasureResetProfile(ms_TimeBuild);

					VB_ClearQueues();
					VB_ClearFaceQueues();
					RenderLeafList();
					VB_RenderFaceQueues();
					VB_Flush();
					VB_RenderQueues();

					// Render and clear wallmarks
		//			if (m_spWMC) Wallmark_Render(m_spWMC);
					if (_pEngine->GetVCDepth() == 0 && m_spWMCTemp != NULL)
					{
		//				Wallmark_Render(m_spWMCTemp);
						m_spWMCTemp->Clear();
					}
				}
#ifdef M_Profile
				ms_TimeCull = ms_TimeBuild - ms_TimeBrushVB - ms_TimeSplineVB - ms_TimeSurf;
#endif
			}

			if (m_pPortals && !(ms_Enable & MODEL_BSP_ENABLE_NOPORTALSTATS))
			{
				int iNodePortal = GetPortalLeaf(m_pView->m_CurLocalVP);
				RenderNodePortals(iNodePortal);
			}

	//		T_Stop(ms_TimeRenderFaceQueue);
		}
		else
		{
	RenderAll:
			{
				if (ms_pSceneFog)
				{
					ms_pSceneFog->SetTransform(&_WMat);
					ms_pSceneFog->TraceBound(BoundBox);
				}

				VB_ClearQueues();
				VB_ClearFaceQueues();

				// FIXME: Medium should be enumerated from the engine.
				CXR_MediumDesc Medium;
				Medium.SetAir();
				Medium.m_MediumFlags = ms_RenderInfo.m_MediumFlags;
				m_pCurMedium = &Medium;

				if (m_lPortalLeaves.Len())
				{
					m_pView->m_nRPortals = 1;
					m_pView->m_pRPortals[m_pView->m_nRPortals] = m_pView->m_CurClipVolume;
					m_pView->m_piRPortalNext[m_pView->m_nRPortals] = 0;
					m_pView->m_nRPortals++;
			
					{
						TMeasureProfile(ms_TimeBrushVB);

						for(int iPL = 0; iPL < m_nStructureLeaves; iPL++)
							RenderPortalLeaf(iPL);
					}
				}
				else
				{
					bool bNHF = 
						(m_pCurMedium->m_MediumFlags & XW_MEDIUM_FOG) &&
						ms_pSceneFog && 
						ms_pSceneFog->NHFEnable();

	//				bool bClipNHF = bNHF && (m_pCurMedium->m_MediumFlags & XW_MEDIUM_CLIPFOG);

					if (!bNHF && (bVertexLighting || (m_spTempWLS->m_nDynamic == 0)))
					{
						const CBSP_VBFacePrim* pVBFP = m_lVBFacePrim.GetBasePtr();
						int nVBFP = m_lVBFacePrim.Len();
						const uint16* pFacePrim = m_lFacePrim.GetBasePtr();

						for(int i = 0; i < nVBFP; i++)
						{
							VB_AddPrimitives(pVBFP->m_iVB, &pFacePrim[pVBFP->m_iPrim], pVBFP->m_nPrim);
							pVBFP++;
						}
					}
					else
					{
						uint32 liVisFaces[2048];
						int nVisFaces = 0;

						int nFaces = m_lFaces.Len();
						for(int iFace = 0; iFace < nFaces; iFace++)
						{
							CBSP_Face* pFace = &m_pFaces[iFace];
							if (pFace->m_Flags & XW_FACE_VISIBLE)
							{
								if (m_lspSurfaces[pFace->m_iSurface]->m_Flags & XW_SURFFLAGS_REFLECTION) continue;
		#ifdef MODEL_BSP_BACKFACECULL
								if (m_pPlanes[pFace->m_iPlane].Distance(m_pView->m_CurLocalVP) > 0.0f)
		#endif
								{
									m_lVBFaceQueues[pFace->m_iVB].AddElem(iFace);
									if (nVisFaces < 2048) 
										liVisFaces[nVisFaces++] = iFace;
								}
							}
						}

		#ifndef MODEL_BSP_DISABLENHF
						if (m_pCurMedium->m_MediumFlags & XW_MEDIUM_FOG)
							if (ms_pSceneFog && ms_pSceneFog->NHFEnable())
							{
								RenderNHF(liVisFaces, nVisFaces, 0);
							}
		#endif
					}

				}

				VB_RenderFaceQueues();
				if (ms_Enable & MODEL_BSP_ENABLE_SPLINEBRUSHES)
					RenderSplineBrushes(true);

				VB_Flush();
				VB_RenderQueues();

				if (ms_pSceneFog) ms_pSceneFog->TraceBoundRelease();
			}
		}

		Light_UnTagDynamics();

#ifdef M_Profile
		if (m_pPortals && !(ms_Enable & MODEL_BSP_ENABLE_NOPORTALSTATS))
		{
			fp32 k = 1000.0f;
	//		ConOut(CStrF("%d/%d/%d, surf %d, %.2f (VB %.2f, SVB %.2f, C %.2f)", m_nFacesChecked, m_nFacesFront, m_nFacesVisible, m_nSurfSwitch, ms_TimeBuild*k, ms_TimeBrushVB*k, ms_TimeSplineVB*k, ms_TimeCull*k));
			ConOut(CStrF("%d/%d/%d, surf %d, %.2f (Surf %.2f, VB %.2f, SVB %.2f, C %.2f)", ms_nFacesChecked, ms_nFacesFront, ms_nFacesVisible, ms_nSurfSwitch, ms_TimeBuild.GetTime()*k, ms_TimeSurf.GetTime()*k, ms_TimeBrushVB.GetTime()*k, ms_TimeSplineVB.GetTime()*k, ms_TimeCull.GetTime()*k));
		}
#endif
	}

	m_pCurAnimState = NULL;
}

// -------------------------------------------------------------------
void CXR_Model_BSP::OnPrecache(CXR_Engine* _pEngine, int _iVariation)
{
	MAUTOSTRIP(CXR_Model_BSP_OnPrecache, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("Read", "No texture-context available.");

	// Precache textures
	{
		// Set precache flags on all textures
		ms_TextureID_AttenuationExp = pTC->GetTextureID("SPECIAL_ATTENUATION3");

#if	!defined(PLATFORM_DOLPHIN) && !defined(PLATFORM_PS2)
		ms_TextureID_Normalize = pTC->GetTextureID("SPECIAL_NORMALIZE0");
		ms_TextureID_CubeNoise = pTC->GetTextureID("CUBENOISE_01");
		ms_TextureID_Specular[0] = pTC->GetTextureID("SPECIAL_SPECULAR001_0");
		ms_TextureID_Specular[1] = pTC->GetTextureID("SPECIAL_SPECULAR002_0");
		ms_TextureID_Specular[2] = pTC->GetTextureID("SPECIAL_SPECULAR004_0");
		ms_TextureID_Specular[3] = pTC->GetTextureID("SPECIAL_SPECULAR008_0");
		ms_TextureID_Specular[4] = pTC->GetTextureID("SPECIAL_SPECULAR016_0");
		ms_TextureID_Specular[5] = pTC->GetTextureID("SPECIAL_SPECULAR032_0");
		ms_TextureID_Specular[6] = pTC->GetTextureID("SPECIAL_SPECULAR064_0");
		ms_TextureID_Specular[7] = pTC->GetTextureID("SPECIAL_SPECULAR128_0");
#endif

		ms_TextureID_DynLightProj = pTC->GetTextureID("LIGHT_PHONG1");

//ms_TextureID_TestBump = pTC->GetTextureID("Cube_20L_0");
		pTC->SetTextureParam(ms_TextureID_AttenuationExp, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);



#ifdef	PLATFORM_WIN	//	SS: Never used on PS2 or GC.

		pTC->SetTextureParam(ms_TextureID_Normalize, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
		pTC->SetTextureParam(ms_TextureID_CubeNoise, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
		for(int i = 0; i < 8; i++)
			pTC->SetTextureParam(ms_TextureID_Specular[i], CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);

#endif


		pTC->SetTextureParam(ms_TextureID_DynLightProj, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);

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
		for(int i = 0; i < m_lspVBInfo.Len(); i++)
		{
			int VBID = m_lspVBInfo[i]->m_VBID;
			if (VBID)
				_pEngine->m_pVBC->VB_SetFlags(VBID, _pEngine->m_pVBC->VB_GetFlags(VBID) | CXR_VBFLAGS_PRECACHE);
		}
	}
}

// -------------------------------------------------------------------

