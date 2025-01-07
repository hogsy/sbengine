#include "PCH.h"

#include "MFloat.h"
#include "WBSP3Model.h"
#include "WBSP3Def.h"
#include "../../Classes/BitString/MBitString.h"

#include "../../XR/XREngineVar.h"
#include "../../XR/XRVBContext.h"

//#define MODEL_BSP_TL_ENABLE_CONDITION m_lPortalLeaves.Len() && m_pCurrentEngine->GetVar(0x2345) != 0
#define MODEL_BSP_TL_ENABLE_CONDITION ms_pCurrentEngine->m_bTLEnableEnabled

#define MODEL_BSP_DYNLIGHTCULLBACKFACE_CONDITION (ms_CurrentRCCaps & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20)


#define MODEL_BSP_DISABLENHF
//#define MODEL_BSP_DISABLEFOGPORTALS

#define MODEL_BSP_BACKFACECULL

//#define MODEL_BSP_TRIANGLEFAN			// Must be enabled, triangle lists doesn't work since all prims go through Render_IndexedPrimitives
#define MODEL_BSP_TRIANGLESTRIP

void Prefetch8(void* _Ptr);

//#define PREFETCH(Addr) Prefetch8(Addr);
#define PREFETCH(Addr)



IMPLEMENT_OPERATOR_NEW(CBSP3_LinkContext);
IMPLEMENT_OPERATOR_NEW(CBSP3_ViewInstance);
IMPLEMENT_OPERATOR_NEW(CBSP3_VertexBuffer);
IMPLEMENT_OPERATOR_NEW(CXR_Model_BSP3);


// -------------------------------------------------------------------
const CXR_AnimState*		CXR_Model_BSP3::m_pCurAnimState;
CXR_MediumDesc*				CXR_Model_BSP3::m_pCurMedium;
CXW_Surface*				CXR_Model_BSP3::m_pCurSurface;
CXW_SurfaceKeyFrame*		CXR_Model_BSP3::m_pCurSurfaceKey;
CXW_SurfaceKeyFrame			CXR_Model_BSP3::m_TmpSurfKeyFrame;

CXR_RenderInfo				CXR_Model_BSP3::ms_RenderInfo(NULL);
CXR_LightInfo				CXR_Model_BSP3::ms_lRenderLightInfo[MODEL_BSP_MAXDYNAMICLIGHTS];
CXR_Light					CXR_Model_BSP3::ms_lRenderLight[MODEL_BSP_MAXDYNAMICLIGHTS];
int							CXR_Model_BSP3::ms_nRenderLightInfo = 0;

CRC_Light*					CXR_Model_BSP3::ms_pRenderVertexLights = NULL;
uint16						CXR_Model_BSP3::ms_nRenderVertexLights = 0;

int							CXR_Model_BSP3::ms_SurfOptions = 0;
int							CXR_Model_BSP3::ms_SurfCaps = 0;

CRC_Attributes*				CXR_Model_BSP3::ms_Render_lpAttribDynLight[MODEL_BSP_MAXDYNAMICLIGHTS] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
CRC_Attributes*				CXR_Model_BSP3::ms_Render_lpAttribDynLight_DestAlphaExpAttn[MODEL_BSP_MAXDYNAMICLIGHTS] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

int							CXR_Model_BSP3::ms_TextureID_AttenuationExp = 0;
int							CXR_Model_BSP3::ms_TextureID_Normalize = 0;
int							CXR_Model_BSP3::ms_TextureID_CubeNoise = 0;
int							CXR_Model_BSP3::ms_TextureID_Specular[8] = { 0,0,0,0,0,0,0,0 };
int							CXR_Model_BSP3::ms_TextureID_DynLightProj = 0;
int							CXR_Model_BSP3::ms_TextureID_DefaultLens = 0;

CBox3Dfp32					CXR_Model_BSP3::ms_Fog_LastBox;

int							CXR_Model_BSP3::ms_LastMedium = 0;
fp32							CXR_Model_BSP3::ms_BasePriority_Opaque = 0;
fp32							CXR_Model_BSP3::ms_BasePriority_Transparent = 0;
CXR_Engine*					CXR_Model_BSP3::ms_pCurrentEngine = NULL;
int							CXR_Model_BSP3::ms_CurrentTLEnable = 0;
int							CXR_Model_BSP3::ms_bCurrentDynLightCullBackFace = 0;
int							CXR_Model_BSP3::ms_CurrentRCCaps = 0;
CXR_VBManager*				CXR_Model_BSP3::ms_pVBM = NULL;
CMat4Dfp32*					CXR_Model_BSP3::ms_pVBMatrixM2V = NULL;
CMat4Dfp32*					CXR_Model_BSP3::ms_pVBMatrixM2W = NULL;
CMat4Dfp32*					CXR_Model_BSP3::ms_pVBMatrixW2V = NULL;
CXR_FogState*				CXR_Model_BSP3::ms_pSceneFog = NULL;

//CMat4Dfp32					CXR_Model_BSP3::ms_PhysWMat;
//CMat4Dfp32					CXR_Model_BSP3::ms_PhysWMatInv;
#ifdef	BSP3_MODEL_PHYS_RENDER
//class CWireContainer*		CXR_Model_BSP3::ms_pPhysWC = NULL;
#endif	// BSP3_MODEL_PHYS_RENDER

#ifdef M_Profile
int							CXR_Model_BSP3::ms_nPortal0 = 0;
int							CXR_Model_BSP3::ms_nPortal1 = 0;
int							CXR_Model_BSP3::ms_nPortal2 = 0;
int							CXR_Model_BSP3::ms_nPortal3 = 0;

int							CXR_Model_BSP3::ms_nFacesChecked = 0;
int							CXR_Model_BSP3::ms_nFacesFront = 0;
int							CXR_Model_BSP3::ms_nFacesVisible = 0;
int							CXR_Model_BSP3::ms_nSurfSwitch = 0;

CMTime						CXR_Model_BSP3::ms_TimeSurf;
CMTime						CXR_Model_BSP3::ms_TimeBrushVB;
CMTime						CXR_Model_BSP3::ms_TimeSplineVB;
CMTime						CXR_Model_BSP3::ms_TimeCull;
CMTime						CXR_Model_BSP3::ms_TimeBuild;

CMTime						CXR_Model_BSP3::ms_Time;
CMTime						CXR_Model_BSP3::ms_TimePortals;
CMTime						CXR_Model_BSP3::ms_TimePortalAnd;
CMTime						CXR_Model_BSP3::ms_TimePortalOr;
CMTime						CXR_Model_BSP3::ms_TimePortal0;
CMTime						CXR_Model_BSP3::ms_TimePortal1;
CMTime						CXR_Model_BSP3::ms_TimePortal2;
CMTime						CXR_Model_BSP3::ms_TimePortal3;
CMTime						CXR_Model_BSP3::ms_TimeRenderLeafList;
CMTime						CXR_Model_BSP3::ms_TimeRenderFaceQueue;
CMTime						CXR_Model_BSP3::ms_TimeRenderPoly1;
CMTime						CXR_Model_BSP3::ms_TimeRenderPoly2;
#endif

CVec2Dfp32					CXR_Model_BSP3::ms_PortalOrMin = 0;
CVec2Dfp32					CXR_Model_BSP3::ms_PortalOrMax = 0;


// XRLight.cpp
int FindMostContributingLights( const int _MaxLights, uint8* _pLightIndices, const CMat4Dfp32& _LTransform, const CXR_RenderInfo* _pRenderInfo );

// -------------------------------------------------------------------
//  Stuff in MImage.cpp
// -------------------------------------------------------------------

void PPA_Mul_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);
void PPA_MulAddSaturate_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);

void PPA_Mul_RGBA32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);

// -------------------------------------------------------------------
//  CBSP3_VBQueue
// -------------------------------------------------------------------
void CBSP3_VBQueue::AddChain(CBSP3_VBChain *_pVB)
{
	if (_pVB->m_bVBIds)
		m_VBQueue.AddChain(_pVB->GetVBIDChain());
	else
		m_VBQueue.AddChain(_pVB->GetVBChain());
}

void CBSP3_VBQueue::AddChain(CXR_VBIDChain *_pVB)
{
	m_VBQueue.AddChain(_pVB);
}

void CBSP3_VBQueue::AddChain(CXR_VBChain *_pVB)
{
	m_VBQueue.AddChain(_pVB);
}


void CBSP3_VBQueue::AddFromVB(CXR_VertexBuffer *_pVB)
{
	if (_pVB->IsVBIDChain())
		m_VBQueue.AddChain(_pVB->GetVBIDChain());
	else
		m_VBQueue.AddChain(_pVB->GetVBChain());
}

void CBSP3_VBQueue::Clear()
{
	MAUTOSTRIP(CBSP2_VBQueue_Clear, MAUTOSTRIP_VOID);
	m_VBQueue.Clear();
}


// -------------------------------------------------------------------
//  CXR_Model_BSP3
// -------------------------------------------------------------------
int CXR_Model_BSP3::ms_Enable = -1 - MODEL_BSP_ENABLE_SPLINEWIRE - MODEL_BSP_ENABLE_FULLBRIGHT - MODEL_BSP_ENABLE_SPLINEBRUSHES /* - MODEL_BSP_ENABLE_MINTESSELATION*/;
//spCXR_WorldLightState CXR_Model_BSP3::m_spTempWLS;

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_BSP3, CXR_PhysicsModel);


// -------------------------------------------------------------------
CXR_Model_BSP3::CXR_Model_BSP3()
{
	MAUTOSTRIP(CXR_Model_BSP_ctor, MAUTOSTRIP_VOID);
	m_iView = 0;
	m_pView = NULL;

	m_nFogTags = 0;
	m_MaxFogTags = 0;
	m_Vis_pEngine = NULL;
	m_VBSBTessLevel = 0.5f;
}

//static bool Validate = false;

CXR_Model_BSP3::~CXR_Model_BSP3()
{
	MAUTOSTRIP(CXR_Model_BSP_dtor, MAUTOSTRIP_VOID);
	VB_FreeVBID();
}

void CXR_Model_BSP3::RenderPortalFence(int _iPortal, CPixel32 _Color)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderPortalFence, MAUTOSTRIP_VOID);
	if (!ms_pVBM) return;
	CBSP3_PortalExt* pP = &m_lPortals[_iPortal];

	int nv = pP->m_nVertices;
	int iiv = pP->m_iiVertices;

	uint32* piV = &m_piVertices[0];
	CVec3Dfp32* pV = &m_pVertices[0];

//	int32 Color = (pP->m_PortalID) ? 0x2fc0c0ff : 0x2fc0ffc0;


	CXR_VertexBuffer* pVB = ms_pVBM->Alloc_VBAttrib();
	if(!pVB)
		return;
	if(!pVB->AllocVBChain(ms_pVBM, false))
		return;
	pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
	pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

	CXR_VBChain* pChain = pVB->GetVBChain();
	pChain->m_piPrim	= (uint16*)ms_pVBM->Alloc(sizeof(uint16) * nv * 2);
	pChain->m_nPrim		= nv * 2;
	pChain->m_pV		= ms_pVBM->Alloc_V3(nv);
	pChain->m_nV		= nv;
	pChain->m_PrimType	= CRC_RIP_WIRES;
	pVB->m_Priority		= 10000000.0f;
	pVB->m_Color		= _Color;

	for(int v = 0; v < nv; v++)
		pChain->m_pV[v]	= pV[piV[iiv + v]];
	int iLast = piV[iiv + nv - 1];
	for(int v = 0; v < nv; v++)
	{
		pChain->m_piPrim[v * 2 + 0]	= iLast;
		pChain->m_piPrim[v * 2 + 1]	= piV[iiv + v];
		iLast	= piV[iiv + v];
	}

	ms_pVBM->AddVB(pVB);
}

void CXR_Model_BSP3::RenderRPortal(const CRC_ClipVolume* _pPortal, CPixel32 _Color)
{
	MAUTOSTRIP(CXR_Model_BSP3_RenderRPortal, MAUTOSTRIP_VOID);
	if (!ms_pVBM) return;

	CXR_VertexBuffer* pVB = ms_pVBM->Alloc_VBAttrib();
	if(!pVB)
		return;
	if(!pVB->AllocVBChain(ms_pVBM, false))
		return;
	pVB->m_pTransform	= ms_pVBM->Alloc_M4(m_pView->m_CurVMat);
	if(!pVB->m_pTransform)
		return;
	pVB->m_Color		= _Color;
	pVB->m_Priority		= 10000000.0f;
	CXR_VBChain* pChain = pVB->GetVBChain();
	pChain->m_pV		= ms_pVBM->Alloc_V3(_pPortal->m_nPlanes);
	pChain->m_nV		= _pPortal->m_nPlanes;
	pChain->m_piPrim	= (uint16*)ms_pVBM->Alloc(sizeof(uint16) * _pPortal->m_nPlanes * 2);
	pChain->m_nPrim		= _pPortal->m_nPlanes * 2;
	pChain->m_PrimType	= CRC_RIP_WIRES;
	if(!pChain->m_pV || !pChain->m_piPrim)
		return;
	memcpy(pChain->m_pV, _pPortal->m_Vertices, sizeof(CVec3Dfp32) * _pPortal->m_nPlanes);
	int iLast = _pPortal->m_nPlanes - 1;
	for(int i = 0; i < _pPortal->m_nPlanes; i++)
	{
		pChain->m_piPrim[i*2 + 0]	= iLast;
		pChain->m_piPrim[i*2 + 1]	= i;
		iLast	= i;
	}

	ms_pVBM->AddVB(pVB);
}

void CXR_Model_BSP3::RenderNodePortals(int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderNodePortals, MAUTOSTRIP_VOID);
	int iPL = m_pNodes[_iNode].m_iPortalLeaf;
	if (!iPL) return;

	const CBSP3_PortalLeafExt* pPL = &m_pPortalLeaves[iPL];

	int np = pPL->m_nPortals;
	int iip = pPL->m_iiPortals;

	for(int p = 0; p < np; p++)
		RenderPortalFence(m_liPortals[iip + p], 0x40ff40);
}


void CXR_Model_BSP3::GetFaceBoundBox(int _iFace, CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_Model_BSP_GetFaceBoundBox, MAUTOSTRIP_VOID);
	CBSP3_Face* pF = &m_lFaces[_iFace];
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

void CXR_Model_BSP3::PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip,
	const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_BSP_PreRender, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP3::PreRender, XR_BSPMODEL);

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

	m_pView = (m_lspViews.ValidPos(m_iView)) ? m_lspViews[m_iView] : (spCBSP3_ViewInstance)NULL;
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
			CBSP3_PortalLeafExt* pPL = &m_lPortalLeaves[iPL];

			for(int iiRefFace = 0; iiRefFace < pPL->m_nRefFaces; iiRefFace++)
			{
				uint32 iFace = m_liPLFaces[pPL->m_iiRefFaces + iiRefFace];
				CBSP3_Face* pF = &m_lFaces[iFace];
#ifdef MODEL_BSP_BACKFACECULL
				if (m_lPlanes[pF->m_iPlane].Distance(m_pView->m_CurLocalVP) < 0.0f) continue;
#endif
				CVec3Dfp32 Verts[64];
				int nVerts = pF->m_nVertices;
				int v;
				for(v = 0; v < nVerts; v++)
					Verts[v] = m_lVertices[m_liVertices[pF->m_iiVertices + v]];

				int bClip;
				nVerts = BSP3Model_CutFace3(Verts, nVerts, m_pView->m_lRPortals[iRP].m_Planes, m_pView->m_lRPortals[iRP].m_nPlanes, bClip);
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
					CBSP3_Face* pF = &m_lFaces[iFace];
#ifdef MODEL_BSP_BACKFACECULL
					if (m_lPlanes[pF->m_iPlane].Distance(m_pView->m_CurLocalVP) < 0.0f) continue;
#endif
					CVec3Dfp32 Verts[64];
					int nVerts = pF->m_nVertices;
					int v;
					for(v = 0; v < nVerts; v++)
						Verts[v] = m_lVertices[m_liVertices[pF->m_iiVertices + v]];

//					int bClip;
//					nVerts = BSP3Model_CutFace3(Verts, nVerts, m_pView->m_lRPortals[iRP].m_Planes, m_pView->m_lRPortals[iRP].m_nPlanes, bClip);
//					for(v = 0; v < nVerts; v++) Verts[v] *= _WMat;
					if (nVerts > 2)
					{
//			if (_pEngine->GetVCDepth()) ConOut("Expand");
						pSkyInterface->Sky_ExpandFrustrum(_pEngine, _WMat, m_pView->m_CurVMat, Verts, nVerts);
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
			CBSP3_Face* pF = &m_lFaces[iFace];
#ifdef MODEL_BSP_BACKFACECULL
			if (m_lPlanes[pF->m_iPlane].Distance(VP) < 0.0f) continue;
#endif
			CVec3Dfp32 Verts[MODEL_BSP_MAXFACEVERTICES];
			int nVerts = pF->m_nVertices;
			int v;
			for(v = 0; v < nVerts; v++)
				Verts[v] = m_lVertices[m_liVertices[pF->m_iiVertices + v]];

			for(v = 0; v < nVerts; v++) Verts[v] *= _WMat;
//			nVerts = BSP3Model_CutFace3(Verts, nVerts, Clip.m_Planes, Clip.m_nPlanes, bClip);

			_pEngine->Render_AddMirror(Verts, nVerts, _WMat, _VMat, 0x407f7f7f, this, (void*) (mint)iFace);
		}
	}
}

void CXR_Model_BSP3::RenderGetPrimCount(const uint32* _piFaces, int _nFaces, int& _nV, int& _nPrim)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderGetPrimCount, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP3::RenderGetPrimCount);

#ifdef MODEL_BSP_TRIANGLEFAN
	int nVert = 0;
	int nPrim = 0;

	int nPolys = 0;
	const uint32* piF = _piFaces;
	for(int f = 0; f < _nFaces; f++)
	{
		PREFETCH(&m_pFaces[piF[f+2]].m_nVertices);

		const CBSP3_Face* pF = &m_pFaces[piF[f]];
		int nv = pF->m_nVertices;
		nVert += nv;
		nPrim += nv + 2;
	}
	_nPrim = nPrim;
	_nV = nVert;

#elif defined(MODEL_BSP_TRIANGLESTRIP)
	int nVert = 0;
	int nPrim = 0;

//	int nPolys = 0;
	const uint32* piF = _piFaces;
	for(int f = 0; f < _nFaces; f++)
	{
		PREFETCH(&m_pFaces[piF[f+2]].m_nVertices);

		const CBSP3_Face* pF = &m_pFaces[piF[f]];
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

		const CBSP3_Face* pF = &m_pFaces[piF[f]];
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

void CXR_Model_BSP3::RenderGetPrimCount_KnitStrip(const uint32* _piFaces, int _nFaces, int& _nV, int& _nPrim)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderGetPrimCount_KnitStrip, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP3::RenderGetPrimCount_KnitStrip);

#ifdef PLATFORM_PS2

	int nVert = 0;
	int nPrim = 2;
	int nPolys = 0;
	int nvStrip = 0;
	const uint32* piF = _piFaces;

	for(int f = 0; f < _nFaces; f++)
	{
		const CBSP3_Face* pF = &m_pFaces[piF[f]];
		int nv = pF->m_nVertices;
		nv = ( nv + 1 ) & ~1;

		if (nv + nvStrip > MODEL_BSP3_MAXKNITSTRIP)
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
		const CBSP3_Face* pF = &m_pFaces[piF[f]];
		int nv = pF->m_nVertices;

#ifdef MODEL_BSP_KNITSTRIPCULLFIX
		nv = (nv + 1) & ~1;
#endif

		if (nv + 2 + nvStrip > MODEL_BSP3_MAXKNITSTRIP)
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

void CXR_Model_BSP3::Tesselate(const uint32* _piFaces, int _nFaces, int _nV, CVec3Dfp32* _pV, CVec2Dfp32* _pTV1, CVec2Dfp32* _pTV2, CPixel32* _pCol, CVec3Dfp32* _pN, CVec3Dfp32* _pTangU, CVec3Dfp32* _pTangV, uint32* _piFaceVertices, CXW_Surface* _pSurf)
{
	MAUTOSTRIP(CXR_Model_BSP_Tesselate, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP3::Tesselate);
	CVec3Dfp32 UVec;
	CVec3Dfp32 VVec;
	fp32 UVecLenSqrInv;
	fp32 VVecLenSqrInv;
	fp32 UOffset;
	fp32 VOffset;
	fp32 TxtWidthInv = 0;
	fp32 TxtHeightInv = 0;

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
		const CBSP3_Face* pF = &m_pFaces[iFace];
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

//			fp32 MidPixelAdjust = 0.5f;
//			int LMWidth = 0;
//			int LMHeight = 0;

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


			CVec2Dfp32 TMid;

#ifdef PLATFORM_PS2
			TMid[0] = TProjMin[0] * TxtWidthInv;
			TMid[1] = TProjMin[1] * TxtHeightInv;
			TMid[0]	= RoundToInt(TMid[0]) - 1.0f;
			TMid[1]	= RoundToInt(TMid[1]) - 1.0f;
#else
			TMid[0] = (TProjMin[0] + TProjMax[0]) * 0.5f * TxtWidthInv;
			TMid[1] = (TProjMin[1] + TProjMax[1]) * 0.5f * TxtHeightInv;
			TMid.k[0] = RoundToInt(TMid.k[0]/16.0f)*16.0f;
			TMid.k[1] = RoundToInt(TMid.k[1]/16.0f)*16.0f;
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

int CXR_Model_BSP3::RenderTesselatePrim(const uint32* _piFaces, int _nFaces, uint16* _pPrim)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderTesselatePrim, 0);
	MSCOPESHORT(CXR_Model_BSP3::RenderTesselatePrim);
#ifdef MODEL_BSP_TRIANGLEFAN
	int nP = 0;

	const uint32* piF = _piFaces;
	for(int i = 0; i < _nFaces; i++)
	{
		int iFace = *(piF++);
		const CBSP3_Face* pF = &m_lFaces[iFace];
		int nv = pF->m_nVertices;
		int iv = pF->m_iiVBVertices;
		_pPrim[nP++] = CRC_RIP_TRIFAN + ((nv+2) << 8);
		_pPrim[nP++] = nv;

		{
			for(int v = 0; v < nv; v++)
				_pPrim[nP++] = iv+v;
		}
	}

	return nP;

#elif defined(MODEL_BSP_TRIANGLESTRIP)
	int nP = 0;

	const uint32* piF = _piFaces;
	for(int i = 0; i < _nFaces; i++)
	{
		int iFace = *(piF++);
		const CBSP3_Face* pF = &m_lFaces[iFace];
		int nv = pF->m_nVertices;
		int iv = pF->m_iiVBVertices;
		_pPrim[nP++] = CRC_RIP_TRISTRIP + ((nv+2) << 8);
		_pPrim[nP++] = nv;

		{
			int iv0 = 0;
			int iv1 = nv - 1;
			int odd = nv & 1;
			nv -= odd;
			for(int v = 0; v < nv; v += 2)
			{
				_pPrim[nP++] = iv + iv1--;
				_pPrim[nP++] = iv + iv0++;
				
			}
			if( odd )
				_pPrim[nP++] = iv + iv1--;
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
		const CBSP3_Face* pF = &m_lFaces[iFace];
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

int CXR_Model_BSP3::RenderTesselatePrim_KnitStrip(const uint32* _piFaces, int _nFaces, uint16* _pPrim)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderTesselatePrim_KnitStrip, false);
	MSCOPESHORT(CXR_Model_BSP3::RenderTesselatePrim_KnitStrip);

#ifdef PLATFORM_PS2

	// Transform TriangleFans into TriangleStrips, and merge them together into one long TriangleStrip,
	// thus greatly reducing overhead in primitivehandling. The ADC bit is used at the weldingpoints.


	int nP = 2;
	int nvStrip = 0;
	int iPCurrentStrip = 0;

	const uint32* piF = _piFaces;

	for(int i = 0; i < _nFaces; i++)
	{
		int iFace = *(piF++);

		const CBSP3_Face* pF = &m_lFaces[iFace];

		int nv = pF->m_nVertices;
		int iv = pF->m_iiVBVertices;
		int odd = nv & 1;
		
		// split if strip is too long!
		if ((nv + odd + nvStrip) > MODEL_BSP3_MAXKNITSTRIP)
		{ 
			_pPrim[iPCurrentStrip] = CRC_RIP_TRISTRIP + (nvStrip+2 << 8);
			_pPrim[iPCurrentStrip+1] = nvStrip;
			nvStrip = 0;
			iPCurrentStrip = nP;
			nP += 2;
		}

		int iv1 = 0;
		int iv2 = nv-1;

		int iFirst = nP;
		
		int oddeven = 0;

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
		
		if( odd )
			_pPrim[nP++] = 0x8000 | ( iv + iv1 - 1 );
		
		// set ADC bit
		_pPrim[iFirst] |= 0x8000;
		_pPrim[iFirst+1] |= 0x8000;
		
		nvStrip += nv + odd;
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
		const CBSP3_Face* pF = &m_lFaces[iFace];
		int nv = pF->m_nVertices; //pF->m_nVBVertices;
		int iv = pF->m_iiVBVertices;

#ifdef MODEL_BSP_KNITSTRIPCULLFIX
		int nvreal = (nv + 1) & ~1;
#else
		int nvreal = nv;
#endif
		if (nvreal + 2 + nvStrip > MODEL_BSP3_MAXKNITSTRIP)
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


bool CXR_Model_BSP3::RenderTesselateVBPrim(const uint32* _piFaces, int _nFaces, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderTesselateVBPrim, false);
	MSCOPESHORT(CXR_Model_BSP3::RenderTesselateVBPrim);
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


bool CXR_Model_BSP3::RenderTesselate(const uint32* _piFaces, int _nFaces, int _TessFlags, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, uint32* _piFaceVertices)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderTesselate, false);
	MSCOPESHORT(CXR_Model_BSP3::RenderTesselate);
	int nVert = 0;
	int nPrim = 0;
	RenderGetPrimCount(_piFaces, _nFaces, nVert, nPrim);

	CVec3Dfp32* pV = _pVBM->Alloc_V3(nVert);
	uint16* pPrim = _pVBM->Alloc_Int16(nPrim);
	if (!pV || !pPrim) return false;

//	uint16* pPrimTmp = pPrim;

	if (!_pVB->AllocVBChain(ms_pVBM, false))
		return false;
	_pVB->Geometry_VertexArray(pV, nVert, true);
//	CVec2Dfp32* pTV = NULL;
//	CVec2Dfp32* pLMTV = NULL;
	CPixel32* pC = NULL;
	CVec3Dfp32* pN = NULL;

	if (_TessFlags & 3)
	{
		ConOut(CStrF("§cf80WARNING: (CXR_Model_BSP3::RenderTesselate) TessFlags %d", _TessFlags));
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
	
#if defined( MODEL_BSP_TRIANGLEFAN ) || defined( MODEL_BSP_TRIANGLESTRIP )
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
		const CBSP3_Face* pF = &m_pFaces[iFace];
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
#elif defined(MODEL_BSP_TRIANGLESTRIP)
		pPrim[nP++] = CRC_RIP_TRISTRIP + ((nv+2) << 8);
		pPrim[nP++] = nv;

		{
			int NV = nv;
			int iv0 = 0;
			int iv1 = NV - 1;
			int odd = NV & 1;
			NV -= odd;
			for(int v = 0; v < NV; v+=2)
			{
				pPrim[nP++] = iVBase + iv1--;
				pPrim[nP++] = iVBase + iv0++;
			}
			
			if( odd )
				pPrim[nP++] = iVBase + iv1--;
			
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

#if defined( MODEL_BSP_TRIANGLEFAN ) || defined( MODEL_BSP_TRIANGLESTRIP )
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

void CXR_Model_BSP3::RenderSurface(CXW_Surface* _pSurf, CXW_SurfaceKeyFrame* _pSurfKey, 
	CXR_VertexBuffer* _pVB, int _LightMapTextureID, bool _bAddLight)
{
	MAUTOSTRIP(CXR_Model_BSP3_RenderSurface, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_PS2
	// Render vertex-buffer chain into z-buffer, black
	{
		CXR_VertexBuffer* pVB = ms_pVBM->Alloc_VBAttrib();
		if (!pVB) 
			return;

		if (_pVB->IsVBIDChain())
			pVB->CopyVBChain(_pVB);
		else
		{
			if (!ms_pVBM->Alloc_VBChainCopy(pVB, _pVB))
				return;

			pVB->SetFlags(CXR_VBFLAGS_TRACKVERTEXUSAGE, _pVB->m_Flags & CXR_VBFLAGS_TRACKVERTEXUSAGE);

			CXR_VBChain* pVBTarget = pVB->GetVBChain();
			CXR_VBChain* pVBSource = _pVB->GetVBChain();
			while(pVBTarget)
			{
				pVBTarget->Clear();
				pVBTarget->m_pV = pVBSource->m_pV;
				pVBTarget->m_nV = pVBSource->m_nV;
				pVBTarget->m_PrimType = pVBSource->m_PrimType;
				pVBTarget->m_piPrim = pVBSource->m_piPrim;
				pVBTarget->m_nPrim = pVBSource->m_nPrim;

				pVBSource = pVBSource->m_pNextVB;
				pVBTarget = pVBTarget->m_pNextVB;
			}
		}

		pVB->Geometry_Color(0);

		pVB->Matrix_Set(ms_pVBMatrixM2V);
		pVB->m_Priority = CXR_VBPRIORITY_UNIFIED_ZBUFFER;

		ms_pVBM->AddVB(pVB);
	}
#endif
return;
}

void CXR_Model_BSP3::VB_CopyGeometry(int _iVB, CBSP3_VBChain* _pVB)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_CopyGeometry, MAUTOSTRIP_VOID);
	CBSP3_VBInfo* pBSPVBI = m_lspVBInfo[_iVB];

	if (ms_CurrentTLEnable && pBSPVBI->m_VBID >= 0)
	{
		CXR_VBIDChain *pChain = ms_pVBM->Alloc_VBIDChain();
		pChain->m_VBID = pBSPVBI->m_VBID;
		_pVB->AddChain(pChain);
	}
	else
	{
		CBSP3_VertexBuffer* pBSPVB = GetVertexBuffer(_iVB);

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

void CXR_Model_BSP3::VB_RenderFaces(int _iVB, const uint32* _piFaces, int _nFaces, CXW_Surface* _pSurf, bool _bAddLight)
{
#ifndef	PLATFORM_PS2
	// Render vertex-buffer chain into z-buffer, black
	{
		CXR_VertexBuffer* pVB = ms_pVBM->Alloc_VB(CXR_VB_ATTRIB);
		if (!pVB) return;

		CBSP3_VBChain Chain;
		VB_CopyGeometry(_iVB, &Chain);
		Chain.SetToVB(pVB);

		if (!RenderTesselateVBPrim(_piFaces, _nFaces, ms_pVBM, pVB))
			return;
		pVB->Geometry_Color(0);
		pVB->Matrix_Set(ms_pVBMatrixM2V);
		pVB->m_Priority = CXR_VBPRIORITY_UNIFIED_ZBUFFER;

		ms_pVBM->AddVB(pVB);
	}
#endif

return;

	MAUTOSTRIP(CXR_Model_BSP_VB_RenderFaces, MAUTOSTRIP_VOID);
	if (!_nFaces) return;
	if (_pSurf->m_Flags & XW_SURFFLAGS_INVISIBLE) return;
	MIncProfile(ms_nSurfSwitch);

	// Set current-surface. No function is using it at the moment though.
	m_pCurSurface = _pSurf;
//	CXW_SurfaceKeyFrame* pSurfKey = _pSurf->GetBaseFrame();
	CXW_SurfaceKeyFrame* pSurfKey = (m_pCurAnimState) 
		?
			_pSurf->GetFrame(
				_pSurf->m_iController ? m_pCurAnimState->m_Anim1 : m_pCurAnimState->m_Anim0, 
				_pSurf->m_iController ? m_pCurAnimState->m_AnimTime1 : m_pCurAnimState->m_AnimTime0, &m_TmpSurfKeyFrame)
		:
			_pSurf->GetBaseFrame();

//	int iFace = _piFaces[0];

	CXR_VertexBuffer VB;

	CBSP3_VBChain Chain;
	VB_CopyGeometry(_iVB, &Chain);
	Chain.SetToVB(&VB);

	if (!RenderTesselateVBPrim(_piFaces, _nFaces, ms_pVBM, &VB))
		return;

	RenderSurface(_pSurf, pSurfKey, &VB, 0, _bAddLight);
}

//#define MODEL_BSP_CULLPORTALPLANES
#define MODEL_BSP_BRUTEFORCE


void CXR_Model_BSP3::VB_ClearQueues()
{
	MAUTOSTRIP(CXR_Model_BSP_VB_ClearQueues, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP3::VB_ClearQueues);
	for(int i = 0; i < m_lVBQueues.Len(); i++)
	{
		m_lVBQueues[i].Clear();
	}
}

int ConvertWLSToCRCLight(CXR_WorldLightState& _WLS, CXR_VBManager& _VBM, CRC_Light*& _pLights, fp32 _LightScale);

static CXW_SurfaceKeyFrame s_lTmpAnimStateSurfKeyFrame[ANIMSTATE_NUMSURF];
static CXW_SurfaceKeyFrame* s_lpTmpAnimStateSurfKeyFrame[ANIMSTATE_NUMSURF];
void CXR_Model_BSP3::VB_RenderQueues()
{

	MAUTOSTRIP(CXR_Model_BSP_VB_RenderQueues, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP3::VB_RenderQueues); //AR-SCOPE

	TMeasureProfile(ms_TimeSurf);

	// Scope local variables (so it wont have to fetch the global ones all the time)
	const CXR_AnimState* pCurAnimState = m_pCurAnimState;
	CXR_VBManager* pVBM = ms_pVBM;

	CXR_RenderSurfExtParam Params;
	if (ms_nRenderVertexLights)
	{
		// FIXME: Broken
//		Params.m_pLights = ms_pRenderVertexLights;
//		Params.m_nLights = ms_nRenderVertexLights;
	}
	Params.m_lUserColors[0] = pCurAnimState->m_Data[0];
	Params.m_lUserColors[1] = pCurAnimState->m_Data[1];

#ifndef	PLATFORM_PS2
	CRC_Attributes* pAZBuffer = pVBM->Alloc_Attrib();
	if (!pAZBuffer)
		return;

	pAZBuffer->SetDefault();

	pAZBuffer->Attrib_Enable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE | CRC_FLAGS_CULL);

//	pAZBuffer->Attrib_Disable(CRC_FLAGS_COLORWRITE | CRC_FLAGS_ALPHAWRITE);
#endif

	CXW_SurfaceKeyFrame** lpTmpAnimStateSurfKeyFrame = s_lpTmpAnimStateSurfKeyFrame;
	CMTime lTmpAnimTime[ANIMSTATE_NUMSURF];

	int nAnimStateSurf = 0;
	for(int iSurf = 0; iSurf < ANIMSTATE_NUMSURF; iSurf++)
	{
		CXW_Surface* pSurf = pCurAnimState->m_lpSurfaces[iSurf];
		if (pSurf)
		{
			nAnimStateSurf = iSurf+1;
			pSurf = pSurf->GetSurface(ms_SurfOptions, ms_SurfCaps);
			int Anim;
			CMTime AnimTime;
			if( pSurf->m_iController )
				Anim = pCurAnimState->m_Anim1, AnimTime = pCurAnimState->m_AnimTime1;
			else
				Anim = pCurAnimState->m_Anim0, AnimTime = pCurAnimState->m_AnimTime0;
			lpTmpAnimStateSurfKeyFrame[iSurf] = pSurf->GetFrame( Anim, AnimTime, &s_lTmpAnimStateSurfKeyFrame[iSurf]);
			lTmpAnimTime[iSurf] = AnimTime;
		}
	}


	CBSP3_VBQueue* plVBQueues = m_lVBQueues.GetBasePtr();
	for(int i = 0; i < m_lVBQueues.Len(); i++)
	{
		CBSP3_VBQueue* pQueue = &plVBQueues[i];

		CBSP3_VBChain *pVBChain = &pQueue->m_VBQueue;
		if (!pVBChain->m_Chain) 
			continue;

		CXW_Surface* pSurf = m_lspSurfaces[pQueue->m_iSurface]->GetSurface(ms_SurfOptions, ms_SurfCaps);
		if (pSurf->m_Flags & XW_SURFFLAGS_INVISIBLE) 
			continue;
#ifndef	PLATFORM_PS2
		// Render vertex-buffer chain into z-buffer, no color/alpha write

		{

			CXR_VertexBuffer* pVB = pVBM->Alloc_VB();
			if (!pVB) 
				return;

			pVBChain->SetToVB(pVB);

			if (!pVB->IsVBIDChain())
			{
				if (!pVBM->Alloc_VBChainCopy(pVB, pVB))
					return;

				pVB->SetFlags(CXR_VBFLAGS_TRACKVERTEXUSAGE, 0);

				CXR_VBChain* pVBTarget = pVB->GetVBChain();
				CXR_VBChain* pVBSource = pVBChain->GetVBChain();
				pVB->Geometry_Color(0);
				while(pVBTarget)
				{
					pVBTarget->Clear();
					pVBTarget->m_pV = pVBSource->m_pV;
					pVBTarget->m_nV = pVBSource->m_nV;
					pVBTarget->m_PrimType = pVBSource->m_PrimType;
					pVBTarget->m_piPrim = pVBSource->m_piPrim;
					pVBTarget->m_nPrim = pVBSource->m_nPrim;

					pVBSource = pVBSource->m_pNextVB;
					pVBTarget = pVBTarget->m_pNextVB;
				}
			}
			pVB->Geometry_Color(0);

			CXW_SurfaceLayer* pLayer = &pSurf->GetBaseFrame()->m_lTextures[0];
			if (pLayer->m_Flags & XW_LAYERFLAGS_ALPHACOMPARE)
			{
				CRC_Attributes* pA = pVBM->Alloc_Attrib();
				*pA = *pAZBuffer;
				pA->Attrib_TextureID(0, pLayer->m_TextureID);
				pA->Attrib_AlphaCompare(pLayer->m_AlphaFunc, pLayer->m_AlphaRef);
				pVB->m_pAttrib = pA;
			}
			else
			{
				pVB->m_pAttrib = pAZBuffer;
			}
			pVB->Matrix_Set(ms_pVBMatrixM2V);
			pVB->m_Priority = CXR_VBPRIORITY_UNIFIED_ZBUFFER;

			pVBM->AddVB(pVB);
		}
#endif
		// Render vertex-buffer chain with surface

#ifdef	PLATFORM_PS2
		int Flags = RENDERSURFACE_NOSHADERLAYERS | /*RENDERSURFACE_MATRIXSTATIC_M2W | RENDERSURFACE_MATRIXSTATIC_W2V |*/ RENDERSURFACE_MATRIXSTATIC_M2V;// | RENDERSURFACE_VERTEXFOG;
#else
		int Flags = RENDERSURFACE_NOSHADERLAYERS | /*RENDERSURFACE_MATRIXSTATIC_M2W | RENDERSURFACE_MATRIXSTATIC_W2V |*/ RENDERSURFACE_MATRIXSTATIC_M2V;// | RENDERSURFACE_VERTEXFOG;
		if (!ms_nRenderVertexLights)
			Flags |= RENDERSURFACE_MODULATELIGHT;
#endif

		{
			fp32 BasePriority = ms_BasePriority_Opaque;
			fp32 Offset = 1.0f;
			if (pSurf->m_Flags & XW_SURFFLAGS_TRANSPARENT)
			{
				BasePriority = ms_BasePriority_Transparent;
				Offset = 0.0001f;
			}

#ifdef	PLATFORM_PS2		
			if (!ms_nRenderVertexLights)
			{
				BasePriority	+= CXR_VBPRIORITY_MODEL_OPAQUE_LM - CXR_VBPRIORITY_MODEL_OPAQUE;
				Offset = BasePriority + (CXR_VBPRIORITY_MODEL_TRANSPARENT - CXR_VBPRIORITY_MODEL_OPAQUE_LM);
//				Offset	+= CXR_VBPRIORITY_MODEL_OPAQUE_LM - CXR_VBPRIORITY_MODEL_OPAQUE;
			}
#endif
/*
			CXW_SurfaceKeyFrame* pSurfKey = (pCurAnimState) 
				?
					pSurf->GetFrame(
						pSurf->m_iController ? pCurAnimState->m_Anim1 : pCurAnimState->m_Anim0, 
						pSurf->m_iController ? pCurAnimState->m_AnimTime1 : pCurAnimState->m_AnimTime0, m_TmpSurfKeyFrame)
				:
					pSurf->GetBaseFrame();
*/
			CXW_SurfaceKeyFrame* pSurfKey;
			CMTime AnimTime;
			if( pCurAnimState == 0 )
				pSurfKey = pSurf->GetBaseFrame();
			else
			{
				int Anim;
				if( pSurf->m_iController )
				{
					Anim = pCurAnimState->m_Anim1;
					AnimTime = pCurAnimState->m_AnimTime1;
				}
				else
				{
					Anim = pCurAnimState->m_Anim0;
					AnimTime = pCurAnimState->m_AnimTime0;
				}
				pSurfKey	= pSurf->GetFrame( Anim, AnimTime, &m_TmpSurfKeyFrame );
			}

			CXR_VertexBuffer VB;
			pVBChain->SetToVB(&VB);

			CXR_Util::Render_Surface(Flags, AnimTime, pSurf, pSurfKey, ms_pCurrentEngine, pVBM, ms_pVBMatrixM2W, ms_pVBMatrixW2V, ms_pVBMatrixM2V, &VB, BasePriority, Offset, &Params);
		}

		if (nAnimStateSurf)
		{
			for(int iSurf = 0; iSurf < nAnimStateSurf; iSurf++)
			{
				CXW_Surface* pSurf = pCurAnimState->m_lpSurfaces[iSurf];
				if (pSurf)
				{
					fp32 BasePriority = ms_BasePriority_Opaque;
					fp32 Offset = 1.0f;
					if (pSurf->m_Flags & XW_SURFFLAGS_TRANSPARENT)
					{
						BasePriority = ms_BasePriority_Transparent;
						Offset = 0.0001f;
					}
#ifdef	PLATFORM_PS2		
					if (!ms_nRenderVertexLights)
					{
						BasePriority	+= CXR_VBPRIORITY_MODEL_OPAQUE_LM - CXR_VBPRIORITY_MODEL_OPAQUE;
						Offset = BasePriority + (CXR_VBPRIORITY_MODEL_TRANSPARENT - CXR_VBPRIORITY_MODEL_OPAQUE_LM);
//						Offset	+= CXR_VBPRIORITY_MODEL_OPAQUE_LM - CXR_VBPRIORITY_MODEL_OPAQUE;
					}
#endif

					CXR_VertexBuffer VB;
					pVBChain->SetToVB(&VB);

					CXR_Util::Render_Surface(Flags, lTmpAnimTime[iSurf], pSurf, lpTmpAnimStateSurfKeyFrame[iSurf], ms_pCurrentEngine, pVBM, ms_pVBMatrixM2W, ms_pVBMatrixW2V, ms_pVBMatrixM2V, &VB, 
						BasePriority, Offset, &Params);
				}
			}
		}
	}
}

	// -------------------------------------------------------------------
void CXR_Model_BSP3::VB_AllocVBID()
{
	MAUTOSTRIP(CXR_Model_BSP_VB_AllocVBID, MAUTOSTRIP_VOID);
/*	if (!m_lPortals.Len())
	{
		VBC_Remove();
		return;
	}*/

	int i;
	for(i = 0; i < m_lspVBInfo.Len(); i++)
	{
		m_lspVBInfo[i]->m_VBID = m_pVBCtx->AllocID(m_iVBC, i);
	}

	if( m_spLightData )
	{
		int Count = m_spLightData->GetSVCount();
		m_spLightData->SetSVVBIDCount( Count );
		for( i = 0; i < Count; i++ )
		{
			int VBID = m_iFirstLightVB + i;
			m_spLightData->m_plSVVBID[i]	= m_pVBCtx->AllocID( m_iVBC, VBID );
		}
	}
}

void CXR_Model_BSP3::VB_FreeVBID()
{
	MAUTOSTRIP(CXR_Model_BSP_VB_FreeVBID, MAUTOSTRIP_VOID);
	if (m_iVBC < 0) return;

	int i;
	if( m_spLightData )
	{
		int Count = m_spLightData->GetSVCount();
		for( i = 0;  i < Count; i++ )
		{
			if( m_spLightData->m_plSVVBID[i] != (uint16)~0 )
				m_pVBCtx->FreeID( m_spLightData->m_plSVVBID[i] );
		}
	}

	for(i = 0; i < m_lVBFacePrim.Len(); i++ )
	{
		if( m_lVBFacePrim[i].m_VBID != 0 )
			m_pVBCtx->FreeID( m_lVBFacePrim[i].m_VBID );
		m_lVBFacePrim[i].m_VBID = 0;
	}

	for(i = 0; i < m_lspVBInfo.Len(); i++)
	{
		if (m_lspVBInfo[i]->m_VBID != (uint16)~0)
			m_pVBCtx->FreeID(m_lspVBInfo[i]->m_VBID);
	}
}


int CXR_Model_BSP3::GetNumLocal()
{
	MAUTOSTRIP(CXR_Model_BSP_GetNumLocal, 0);

	if( m_iFirstLightVB > 0  )
		return m_lspVBInfo.Len() + m_spLightData->GetSVCount();
	else
		return m_lspVBInfo.Len();
}

int CXR_Model_BSP3::GetID(int _iLocal)
{
	MAUTOSTRIP(CXR_Model_BSP_GetID, 0);
	if( m_iFirstLightVB > 0 && _iLocal >= m_iFirstLightVB )
		return m_spLightData->m_plSVVBID[_iLocal - m_iFirstLightVB];
	else
		return m_lspVBInfo[_iLocal]->m_VBID;
}

struct STempPrecacheVB
{
	TThinArray<uint16>		m_lPrimStream;
#ifndef	PLATFORM_PS2
	TThinArray<CVec3Dfp32>	m_lVertex;
	TThinArray<CVec2Dfp32>	m_lTexCoord;
	TThinArray<uint16>		m_lRemapList;
#endif
};

static STempPrecacheVB BSP3Precache;

void CXR_Model_BSP3::Get_SV(int _iLocal, CRC_BuildVertexBuffer& _VB, int _Flags)
{
	CBSP3_LightData* pLD = m_spLightData;

	CBSP3_ShadowVolume& SV = pLD->m_plSV[_iLocal];
	
	_VB.Clear();
	
	if( SV.m_nLMC == 0 )
		return;

	if( pLD->m_ClusterCount == 0 )
		return;

	pLD->GetClusters();
	pLD->GetVertexBuffers();
	
	int PrimLen = 0;
	for( int i = 0; i < SV.m_nLMC; i++ )
	{
		int iLMC = SV.m_iLMC + i;
#ifdef	MODEL_BSP3_USEKNITSTRIP
		PrimLen	+= pLD->GetPrimStreamCount_KnitStrip( iLMC );
#else
		PrimLen	+= pLD->GetPrimStreamCount( iLMC );
#endif
	}
	
	if( PrimLen > BSP3Precache.m_lPrimStream.Len() )	// Only grow if necessary
	{
		BSP3Precache.m_lPrimStream.SetLen( PrimLen );
#ifndef	PLATFORM_PS2
		BSP3Precache.m_lRemapList.SetLen( PrimLen );
#endif	// PLATFORM_PS2
	}
	uint16* pData = BSP3Precache.m_lPrimStream.GetBasePtr();

	for( int i = 0; i < SV.m_nLMC; i++ )
	{
		int iLMC = SV.m_iLMC + i;
#ifdef	MODEL_BSP3_USEKNITSTRIP
		pLD->CreatePrimStream_KnitStrip( iLMC, &pData );
#else
		pLD->CreatePrimStream( iLMC, &pData );
#endif
	}

	int iVB = pLD->m_plLMCLightmap[SV.m_iLMC].m_iVB;
#ifdef	PLATFORM_PS2
	_VB.m_piPrim	= BSP3Precache.m_lPrimStream.GetBasePtr();
	_VB.m_nPrim		= PrimLen;
	_VB.m_PrimType	= CRC_RIP_STREAM;

	_VB.m_pV = pLD->m_lLightVB[iVB].m_lPosition.GetBasePtr();
	_VB.m_nV = pLD->m_lLightVB[iVB].m_lPosition.Len();
	_VB.m_pTV[0] = (fp32*) pLD->m_lLightVB[iVB].m_lTexCoord.GetBasePtr();
	_VB.m_nTVComp[0] = 2;
#else
	{
		// Have to refetch baseptr since CreatePrimStream modifies it
		pData = BSP3Precache.m_lPrimStream.GetBasePtr();
		// Time to create remaplist
		int RemapIndex = 0;
		int StripLen = 0;
		uint16*pRemap = BSP3Precache.m_lRemapList.GetBasePtr();
		for( int i = 0; i < PrimLen; i++ )
		{
			uint16 Index = pData[i];
			if( StripLen == 0 )
			{
				// This should be a new strip declaration
				i++;
				StripLen = pData[i];
				continue;
			}
			StripLen--;
			M_ASSERT( ( Index & 0x8000 ) == 0, "Illegal primstream" );
			int j;
			for( j = 0; j < RemapIndex; j++ )
			{
				if( pRemap[j] == Index )
					break;
			}

			if( j == RemapIndex )
			{
				pRemap[RemapIndex++]	= Index;
			}
		}

		// Should have a decent remap list now... time to build the temporary vertexbuffer
		BSP3Precache.m_lVertex.SetLen( RemapIndex );
		BSP3Precache.m_lTexCoord.SetLen( RemapIndex );
		for( int i = 0; i < RemapIndex; i++ )
		{
			BSP3Precache.m_lVertex[i] = pLD->m_lLightVB[iVB].m_lPosition[pRemap[i]];
			BSP3Precache.m_lTexCoord[i] = pLD->m_lLightVB[iVB].m_lTexCoord[pRemap[i]];
		}

		StripLen = 0;
		for( int i = 0; i < PrimLen; i++ )
		{
			if( StripLen == 0 )
			{
				i++;
				StripLen = pData[i];
				continue;
			}
			StripLen--;

			for( int j = 0; j < RemapIndex; j++ )
			{
				if( pData[i] == pRemap[j] )
				{
					pData[i]	= j;
					break;
				}
			}
		}

		// Ok.. should have a proper vertexbuffer by now...
		_VB.m_piPrim	= BSP3Precache.m_lPrimStream.GetBasePtr();
		_VB.m_nPrim		= PrimLen;
		_VB.m_PrimType	= CRC_RIP_STREAM;

		if (BSP3Precache.m_lVertex.GetBasePtr())
			_VB.Geometry_VertexArray(BSP3Precache.m_lVertex.GetBasePtr());
		_VB.m_nV = BSP3Precache.m_lVertex.Len();
		if (BSP3Precache.m_lTexCoord.GetBasePtr())
			_VB.Geometry_TVertexArray(BSP3Precache.m_lTexCoord.GetBasePtr(), 0);
	}
#endif	// PLATFORM_PS2

}

void CXR_Model_BSP3::Get_FacePrim(int _iLocal, CRC_BuildVertexBuffer& _VB, int _Flags)
{
	CBSP3_VBFacePrim& VBFP = m_lVBFacePrim[_iLocal];
	int iBSPVB = VBFP.m_iVB;
	_VB.Clear();

	CBSP3_VertexBuffer* pBSPVB = GetVertexBuffer(iBSPVB);

	_VB.m_piPrim	= &m_lFacePrim[VBFP.m_iPrim];
	_VB.m_nPrim		= VBFP.m_nPrim;
	_VB.m_PrimType	= CRC_RIP_STREAM;

	if (pBSPVB->m_lV.GetBasePtr())
		_VB.Geometry_VertexArray(pBSPVB->m_lV.GetBasePtr());
	_VB.m_nV = pBSPVB->m_lV.Len();
	if (pBSPVB->m_lN.GetBasePtr())
		_VB.Geometry_NormalArray(pBSPVB->m_lN.GetBasePtr());
	if (pBSPVB->m_lCol.GetBasePtr())
		_VB.Geometry_ColorArray(pBSPVB->m_lCol.GetBasePtr());
	if (pBSPVB->m_lTV1.GetBasePtr())
		_VB.Geometry_TVertexArray(pBSPVB->m_lTV1.GetBasePtr(), 0);
}

void CXR_Model_BSP3::Get(int _iLocal, CRC_BuildVertexBuffer& _VB, int _Flags)
{
	MAUTOSTRIP(CXR_Model_BSP_Get, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP3::Get, XR_BSPMODEL);

	if( m_iFirstFaceVB > 0 && _iLocal >= m_iFirstFaceVB )
	{
		Get_FacePrim( _iLocal - m_iFirstFaceVB, _VB, _Flags);
	}
	else if( m_iFirstLightVB > 0 && _iLocal >= m_iFirstLightVB )
	{
		Get_SV( _iLocal - m_iFirstLightVB, _VB, _Flags);
	}
	else
	{
		CBSP3_VertexBuffer* pBSPVB = GetVertexBuffer(_iLocal);

		if (_Flags & VB_GETFLAGS_FALLBACK)
			m_lspVBInfo[_iLocal]->m_bNoRelease = 1;

		_VB.Clear();

		FillChar(pBSPVB->m_lCol.GetBasePtr(), pBSPVB->m_lCol.Len()*4, -1);

		if (pBSPVB->m_lV.GetBasePtr())
			_VB.Geometry_VertexArray(pBSPVB->m_lV.GetBasePtr());
		_VB.m_nV = pBSPVB->m_lV.Len();
		if (pBSPVB->m_lN.GetBasePtr())
			_VB.Geometry_NormalArray(pBSPVB->m_lN.GetBasePtr());
		if (pBSPVB->m_lCol.GetBasePtr())
			_VB.Geometry_ColorArray(pBSPVB->m_lCol.GetBasePtr());
		if (pBSPVB->m_lTV1.GetBasePtr())
			_VB.Geometry_TVertexArray(pBSPVB->m_lTV1.GetBasePtr(), 0);
	}

	// Calculate boundbox here
}

void CXR_Model_BSP3::Release(int _iLocal)
{
	MAUTOSTRIP(CXR_Model_BSP_Release, MAUTOSTRIP_VOID);
	int nFaceVB = m_lVBFacePrim.Len();
	int nLMCVB = (m_spLightData != NULL) ? m_spLightData->GetSVCount() : 0;
	if (_iLocal > m_lspVBInfo.Len() + nLMCVB + nFaceVB)
		Error("Release", CStrF("Invalid local ID %d", _iLocal));

	if( m_iFirstFaceVB > 0 && nFaceVB && _iLocal >= m_iFirstFaceVB )
	{
		int iFacePrim = _iLocal - m_iFirstFaceVB;
		CBSP3_VBFacePrim& VBFP = m_lVBFacePrim[iFacePrim];
		_iLocal = VBFP.m_iVB;
	}
	else if( m_iFirstLightVB > 0 && nLMCVB && _iLocal >= m_iFirstLightVB )
	{
		// Make sure to release the damn thing
		BSP3Precache.m_lPrimStream.Destroy();
		return;
	}

	if (_iLocal < m_lspVBInfo.Len() && !m_lspVBInfo[_iLocal]->m_bNoRelease)
	{
		m_lspVB[_iLocal] = NULL;
	}
}

// -------------------------------------------------------------------
void CXR_Model_BSP3::RenderNHF(const uint32* _piFaces, int _nFaces, int _iPL)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderNHF, MAUTOSTRIP_VOID);
	if (_nFaces > MODEL_BSP_MAXPORTALLEAFFACES)
	{
		ConOut(CStrF("(CXR_Model_BSP3::RenderNHF) Too many faces. %d/%d", _nFaces, MODEL_BSP_MAXPORTALLEAFFACES));
		return;
	}

	int iBaseV = 0;
	if (_iPL)
	{
		Fog_TraceVertices(_piFaces, _nFaces, _iPL);
		UntagFogVertices();

		const CBSP3_PortalLeafExt* pPL = &m_pPortalLeaves[_iPL];
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

void CXR_Model_BSP3::VB_RenderNHF(int _iVB, const uint16* _piPrim, int _nPrim, CBSP3_PortalLeafExt* _pPL)
{
	MAUTOSTRIP(CXR_Model_BSP_VB_RenderNHF, MAUTOSTRIP_VOID);
	// Note: _piPrim is not CRC_RIP_END terminated.

	CXR_VertexBuffer* pVB = ms_pVBM->Alloc_VB();
	if (pVB)
	{
		CBSP3_VBChain Chain;
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

//#include "../../XR/XRShader.h"

/*
void CXR_Model_BSP3::PrepareShaderQueue()
{
	CBSP3_LightData* pLD = m_spLightData;
	if (!pLD)
		return;

	pLD->m_lpShaderQueueVBChain.SetLen(pLD->m_lShaderQueue.Len());
	FillChar(pLD->m_lpShaderQueueVBChain.GetBasePtr(), pLD->m_lpShaderQueueVBChain.ListSize(), 0);
}
*/

/*
void CXR_Model_BSP3::RenderShaderQueue()
{
	MSCOPESHORT(CXR_Model_BSP3::RenderShaderQueue);
	CBSP3_LightData* pLD = m_spLightData;
	if (!pLD)
		return;

	CBSP3_SceneGraph* pSGI = m_pView->m_pSceneGraph;
	if (!pSGI)
		return;

	int nSQ = pLD->m_lpShaderQueueVBChain.Len();
	CXR_VertexBuffer** lpShaderQueueVBChain = pLD->m_lpShaderQueueVBChain.GetBasePtr();

	const CBSP3_ShaderQueueElement* pSQ = pLD->m_lShaderQueue.GetBasePtr();
	if (pLD->m_lShaderQueue.Len() != nSQ)
		Error("RenderShaderQueue", "Internal error.");

	CXR_Shader* pShader = ms_pCurrentEngine->GetShader();

	for(int i = 0; i< nSQ; i++)
	{
		if (lpShaderQueueVBChain[i])
		{
			CXR_VertexBuffer* pVB = lpShaderQueueVBChain[i];
			pVB->Matrix_Set(ms_pVBMatrixM2V);

			CXW_Surface* pSurf = m_lspSurfaces[pSQ[i].m_iSurface];

			CXR_ShaderParams Params;
			Params.Create(*pSurf->GetBaseFrame());
			pShader->RenderShading(pSGI->m_lLights[pSQ[i].m_iLight], pVB, &Params);

			lpShaderQueueVBChain[i] = NULL;
		}
	}
}
*/
/*
void CXR_Model_BSP3::Render_SV( int _iSV )
{
	MSCOPESHORT(CXR_Model_BSP3::Render_SV);

	CBSP3_LightData* pLD = m_spLightData;

	CBSP3_ShadowVolume& SV = pLD->m_plSV[_iSV];
	CBSP3_SceneGraph* pSGI = m_pView->m_pSceneGraph;

	CXR_Light& Light = pSGI->m_lLights[SV.m_iLight];

	CPixel32 Color = CPixel32( Light.m_Intensity.k[0] * 128.0f, Light.m_Intensity.k[1] * 128.0f, Light.m_Intensity.k[2] * 128.0f );

	CXR_VertexBuffer* pVB = ms_pVBM->Alloc_VB();
	pVB->m_pAttrib = &pLD->m_plAttributes[pLD->m_plLMCLightmap[SV.m_iLMC].m_iTex];
	if (!pVB->AllocVBChain(ms_pVBM, true))
		return;
	pVB->Render_VertexBuffer(pLD->m_plSVVBID[_iSV]);

	pVB->m_Color	= Color;

	pVB->Matrix_Set(ms_pVBMatrixM2V);

	pVB->m_Priority = CXR_VBPRIORITY_LIGHTMAP;

	ms_pVBM->AddVB(pVB);
}
*/
void CXR_Model_BSP3::RenderPortalLeaf(int _iPL)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderPortalLeaf, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP3::RenderPortalLeaf); //AR-SCOPE

	CBSP3_PortalLeafExt* pPL = &m_lPortalLeaves[_iPL];

//	int iNode = m_pPortalLeaves[_iPL].m_iNode;
//	const CBSP3_Node* pN = &m_pNodes[iNode];
//	int iRP = m_pView->m_liLeafRPortals[pN->m_iPortalLeaf];
//	CRC_ClipVolume* pCurRPortal = &m_pView->m_lRPortals[iRP];
	m_nVVertTag = 0;
	m_nFogTags = 0;
//	m_iFaceQueue = 0;
	m_pCurMedium = &m_lMediums[pPL->m_iMedium];
//	int bFog = (m_pCurMedium->m_MediumFlags & XW_MEDIUM_FOG);

//	uint16 liVisFaces[MODEL_BSP_MAXPORTALLEAFFACES];

//	int nVisFaces = 0;

#if !defined(MODEL_BSP_DISABLENHF)
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
//			VB_RenderFaceQueues();
//			VB_Flush();
			VB_RenderQueues();
			VB_ClearQueues();
//			VB_ClearFaceQueues();
#ifndef PLATFORM_PS2
			ms_BasePriority_Opaque += CXR_VBPRIORITY_PORTALLEAFOFFSET;
			ms_BasePriority_Transparent += CXR_VBPRIORITY_PORTALLEAFOFFSET;
#endif
			if (bFog)
				Fog_RenderPortals(_iPL, iFogPortals, nFogPortals);
		}
	}
#endif // MODEL_BSP_DISABLENHF

#ifdef	PLATFORM_PS2
//	const bool bHWNHF = false;
#else
//	const bool bHWNHF = (ms_CurrentRCCaps & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20) && !(m_pCurMedium->m_MediumFlags & XW_MEDIUM_CLIPFOG);
#endif
			
	pPL->m_BasePriority = ms_BasePriority_Opaque;

	if (m_spWMC != NULL) Wallmark_Render(m_spWMC, _iPL);
	if (m_spWMCTemp != NULL) Wallmark_Render(m_spWMCTemp, _iPL);
	if (m_spWMCStatic != NULL) Wallmark_Render(m_spWMCStatic, _iPL);

	int nF = pPL->m_nFaces;
	if (!nF) return;
//	const uint32* piF = &m_liPLFaces[pPL->m_iiFaces];

	CXR_VBManager* pVBM = ms_pVBM;


	{
		MSCOPESHORT(bBruteForce);
		{
			{
				int nVBFP = pPL->m_nVBFacePrims;
				if (nVBFP)
				{
					// Alloc all VB's needed in 1 chunk
					CXR_VBIDChain *pChain = (CXR_VBIDChain *)pVBM->Alloc( sizeof( CXR_VBIDChain ) * nVBFP );
					if( !pChain )	return;
					
					const CBSP3_VBFacePrim* pVBFP = &m_lVBFacePrim[pPL->m_iVBFacePrims];
					for(int i = 0; i < nVBFP; i++)
					{
						pChain->Clear();
						pChain->Render_VertexBuffer(pVBFP->m_VBID);
						m_lVBQueues[pVBFP->m_iVB].AddChain(pChain);
						pVBFP++;
						pChain++;
					}
				}
			}
		}

#if !defined(MODEL_BSP_DISABLENHF)
		if (bHWNHF &&
			(m_pCurMedium->m_MediumFlags & XW_MEDIUM_FOG) &&
			ms_pSceneFog && 
			ms_pSceneFog->NHFEnable())
		{
			int nVBFP = pPL->m_nVBFacePrims;
			if (nVBFP)
			{
				CBSP3_VBFacePrim* pVBFP = &m_lVBFacePrim[pPL->m_iVBFacePrims];
				uint16* pFacePrim = m_lFacePrim.GetBasePtr();
				for(int i = 0; i < nVBFP; i++)
				{
					VB_RenderNHF(pVBFP->m_iVB, &pFacePrim[pVBFP->m_iPrim], pVBFP->m_nPrim, pPL);
					pVBFP++;
				}
			}
		}
#endif
	}

#if !defined(MODEL_BSP_DISABLENHF)

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
				CBSP3_PortalExt* pP = &m_pPortals[iP];
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

	if (m_spLightData != NULL)
	{
		MSCOPESHORT(CXR_Model_BSP3::m_spLightData);
		CBSP3_LightData* pLD = m_spLightData;

		if( pLD->m_spLMTC == NULL || pLD->m_spLMTC->GetNumLocal() == 0 )
			return;

		if( !m_pView->m_pSceneGraph )
			return;

//		int nSV = pLD->GetSVCount();
		int MaxLights = m_pView->m_lLightOcclusion.Len();
		const CXR_LightOcclusionInfo* pLO = m_pView->m_lLightOcclusion.GetBasePtr();
		const CXR_Light* pLights = m_pView->m_pSceneGraph->m_lLights.GetBasePtr();

		int nSVPL = (_iPL >= pLD->m_lnSVPL.Len()) ? 0 : pLD->m_lnSVPL[_iPL];
		int iSVFirstPL = (_iPL >= pLD->m_liSVFirstPL.Len()) ? 0 : pLD->m_liSVFirstPL[_iPL];
		const CBSP3_ShadowVolume* pSV = &pLD->m_plSV[iSVFirstPL];

		const uint16*plSVVBID = &pLD->m_plSVVBID[iSVFirstPL];

		for(int i = 0; i < nSVPL; i++, pSV++)
		{
			if (pSV->m_iPL == _iPL)
			{
				if ( pSV->m_nLMC == 0)
					continue;

				int iLight = pSV->m_iLight;

				if (iLight >= MaxLights ||
					!pLO[iLight].IsVisible() ||
					(pLights[iLight].GetIntensitySqrf() < 0.0001f))
					continue;

//				Render_SV( iSVFirstPL + i );
				{
					const CXR_Light& Light = pLights[iLight];
					CVec4Dfp32 LightColor = Light.GetIntensityv();
					CPixel32 Color = CPixel32((int)(LightColor[0] * 128.0f), (int)(LightColor[1] * 128.0f), (int)(LightColor[2] * 128.0f));

					CXR_VertexBuffer* pVB = pVBM->Alloc_VB();
					pVB->m_pAttrib = &pLD->m_plAttributes[pLD->m_plLMCLightmap[pSV->m_iLMC].m_iTex];
					if (!pVB->AllocVBChain(pVBM, true))
						return;
					pVB->Render_VertexBuffer(plSVVBID[i]);

					pVB->m_Color	= Color;

					pVB->Matrix_Set(ms_pVBMatrixM2V);

					pVB->m_Priority = CXR_VBPRIORITY_LIGHTMAP;

					pVBM->AddVB(pVB);
				}

   			}
		}
	}

//#endif 	// VBQUEUES
}

void CXR_Model_BSP3::RenderLeafList()
{
	MAUTOSTRIP(CXR_Model_BSP_RenderLeafList, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP3::RenderLeafList); //AR-SCOPE

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


void CXR_Model_BSP3::UntagVertices()
{
	MAUTOSTRIP(CXR_Model_BSP_UntagVertices, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_nVVertTag; i++)
	{
		int iv = m_piVVertTag[i];
		m_pVVertMask[iv] = 0;
	}
	m_nVVertTag = 0;
}

void CXR_Model_BSP3::UntagFogVertices()
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
static CRC_Light DummyLight;
void CXR_Model_BSP3::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS_asdf,
			const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_BSP_OnRender, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP3::OnRender, XR_BSPMODEL);

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

	CBox3Dfp32 BoundBox;
	Phys_GetBound_Box(_WMat, BoundBox);

	ms_RenderInfo.Clear(_pEngine);

	// Get bound-box, get the CRC_ClipView
	{
		CRC_Viewport* pVP = ms_pVBM->Viewport_Get();
		m_pView->m_ViewBackPlane = pVP->GetBackPlane();

		if (_pViewClip)
		{
			// Entity
			ms_RenderInfo.m_pLightInfo = ms_lRenderLightInfo;
			ms_RenderInfo.m_MaxLights = MODEL_BSP3_MAX_LIGHTS;
			ms_RenderInfo.m_nLights = 0;

			if (_pAnimState->m_iObject)
			{
				_pViewClip->View_GetClip(_pAnimState->m_iObject, &ms_RenderInfo);
				if ((ms_RenderInfo.m_Flags & CXR_RENDERINFO_INVISIBLE) && !ms_RenderInfo.m_nLights)
					return;
			}
			else
			{
				if (!_pViewClip->View_GetClip_Box(BoundBox.m_Min, BoundBox.m_Max, 0, 0, NULL, &ms_RenderInfo))
					return;
			}

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

	ms_pSceneFog = _pEngine->m_pCurrentFogState;

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

		if (!(_Flags & 1) && m_lPortalLeaves.Len() && !_pViewClip)
		{
			MSCOPESHORT(RPortals);
			int nPL = Max(m_pView->m_liVisPortalLeaves.Len(), m_lPortalLeaves.Len());
			m_pView->m_liVisPortalLeaves.SetLen(nPL);
			m_pView->m_MaxVisLeaves = nPL;
			m_pView->m_nVisLeaves = 0;
	//		m_pView->m_lPVS.SetLen((nPL + 15) >> 3);

			// Initialize nodes RPortal look-up table.
			if (m_pView->m_liLeafRPortals.Len() != m_lPortalLeaves.Len())
				m_pView->m_liLeafRPortals.SetLen(m_lPortalLeaves.Len());
			FillChar(&m_pView->m_liLeafRPortals[0], m_pView->m_liLeafRPortals.ListSize(), 0);

			// Initialize space for RPortals.
			m_pView->m_MaxRPortals = 512;
			m_pView->m_lRPortals.SetLen(m_pView->m_MaxRPortals);
			m_pView->m_liRPortalNext.SetLen(m_pView->m_MaxRPortals);
			m_pView->m_lPortalScissor.SetLen(m_pView->m_MaxRPortals);
			m_pView->m_pRPortals = m_pView->m_lRPortals.GetBasePtr();
			m_pView->m_piRPortalNext = m_pView->m_liRPortalNext.GetBasePtr();
			m_pView->m_nRPortals = 0;
		}

		InitializeListPtrs();

		{
			CMat4Dfp32 WMatInv;
			_WMat.InverseOrthogonal(WMatInv);

			ms_pRenderVertexLights = NULL;
			ms_nRenderVertexLights = 0;

			// If this is the world we need to grab the dynamic lights manually since we didnt run View_GetClip_Box()
			if (m_lPortalLeaves.Len() && m_pView->m_pSceneGraph)
			{
				MSCOPESHORT(Lightmap_Lights);
				CBSP3_SceneGraph* pSG = m_pView->m_pSceneGraph;
				ms_RenderInfo.m_nLights = 0;


				// check if something strage is going on (can crash if this check isn't here)
				if(m_pView->m_lLightOcclusion.Len() != pSG->m_lLights.Len())
				{
					ConOutL("§cf80WARNING: (CXR_Model_BSP3::OnRender) m_pView->m_lLightOcclusion.Len() != pSG->m_lLights.Len() failed.");
					return;
				}

				for(int i = pSG->m_iFirstDynamic; i < pSG->m_lLights.Len(); i++)
				{
					if (ms_RenderInfo.m_nLights >= MODEL_BSP_MAXDYNAMICLIGHTS)
						break;

					if (pSG->m_lLights[i].GetIntensitySqrf() < 0.0001f)
						continue;

					const CXR_LightOcclusionInfo& LO = m_pView->m_lLightOcclusion[i];
					if (LO.IsVisible())
					{
						ms_lRenderLight[ms_RenderInfo.m_nLights] = pSG->m_lLights[i];
						ms_lRenderLight[ms_RenderInfo.m_nLights].Transform(WMatInv);
						ms_lRenderLightInfo[ms_RenderInfo.m_nLights].m_Scissor = LO.m_ScissorVisible;
						ms_RenderInfo.m_nLights++;
					}
				}

				Light_TagDynamics();
			}
			else
			{
				MSCOPESHORT(VertexLight_Lights);
				uint8 aLightIndices[32];
				int nLight = FindMostContributingLights( 3, aLightIndices, WMatInv, &ms_RenderInfo );

				ms_nRenderVertexLights = nLight;
				ms_pRenderVertexLights = ms_pVBM->Alloc_Lights(nLight);

	#if 1 && defined(PLATFORM_PS2)
				for(int i = 0; i < nLight; i++)
				{
					const CXR_Light* pEngineLight = ms_RenderInfo.m_pLightInfo[aLightIndices[i]].m_pLight;
					CVec4Dfp32 LightColor = pEngineLight->GetLightColor();
					CRC_Light* pRenderLight = &ms_pRenderVertexLights[i];
					pEngineLight->GetPosition().MultiplyMatrix(WMatInv, pRenderLight->m_Pos);

	//				fp32 FallOffScaler = 128.0f * Clamp01( 1.0f - ( pRenderLight->m_Pos.LengthSqr() * Sqr(pEngineLight->m_RangeInv) ) );
					fp32 DistScalar = ( pEngineLight->m_Range - pRenderLight->m_Pos.Length() ) * pEngineLight->m_RangeInv;
					DistScalar = M_Cos((1.0f-DistScalar)*_PIHALF);
					fp32 FallOffScaler = 255.0f * Clamp01( DistScalar * 128.0f / ( pRenderLight->m_Pos.Length() + 128 ) );
					

					pRenderLight->m_Color = CPixel32( LightColor[0] * FallOffScaler, LightColor[1] * FallOffScaler, LightColor[2] * FallOffScaler );
					pRenderLight->m_Type = CRC_LIGHTTYPE_PARALLELL;
					pRenderLight->m_Ambient = 0;
					pRenderLight->m_Attenuation[0] = 0;
					pRenderLight->m_Attenuation[1] = pEngineLight->m_RangeInv;
					pRenderLight->m_Attenuation[2] = 0;
					pRenderLight->m_Direction = pRenderLight->m_Pos;
					pRenderLight->m_Direction.Normalize();
				}
	#else
				for(int i = 0; i < nLight; i++)
				{
					const CXR_Light* pEngineLight = ms_RenderInfo.m_pLightInfo[aLightIndices[i]].m_pLight;
					CVec4Dfp32 LightColor = pEngineLight->GetIntensityv();
					CRC_Light* pRenderLight = &ms_pRenderVertexLights[i];
					pEngineLight->GetPosition().MultiplyMatrix(WMatInv, pRenderLight->m_Pos);
					pRenderLight->m_Color = CPixel32((int)(LightColor[0] * 128.0f), (int)(LightColor[1] * 128.0f), (int)(LightColor[2] * 128.0f));
					pRenderLight->m_Type = CRC_LIGHTTYPE_POINT;
					pRenderLight->m_Ambient = 0;
					pRenderLight->m_Direction = 0;
					pRenderLight->m_Attenuation[0] = 0;
					pRenderLight->m_Attenuation[1] = pEngineLight->m_RangeInv;
					pRenderLight->m_Attenuation[2] = 0;
				}
	#endif
				
				if( nLight == 0 )
				{
					DummyLight.m_Color	= 0;
					DummyLight.m_Type = CRC_LIGHTTYPE_PARALLELL;
					ms_pRenderVertexLights	= &DummyLight;
					ms_nRenderVertexLights	= 1;
				}
			}
		}

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

				{

		#ifdef M_Profile
					ms_TimeSplineVB.Reset();
					ms_TimeBrushVB.Reset();
					ms_TimeSurf.Reset();
					ms_TimeCull.Reset();
		#endif

					{
						TMeasureResetProfile(ms_TimeBuild);

						VB_ClearQueues();
						RenderLeafList();
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
			}

			if (m_pPortals && !(ms_Enable & MODEL_BSP_ENABLE_NOPORTALSTATS))
			{
				int iNodePortal = GetPortalLeaf(m_pView->m_CurLocalVP);
				RenderNodePortals(iNodePortal);
			}

		}
		else
		{
	RenderAll:
			{
				if (ms_pSceneFog)
				{
					ms_pSceneFog->SetTransform(&_WMat);
	//				ms_pSceneFog->TraceBound(BoundBox);
				}

				VB_ClearQueues();

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
//					bool bNHF = (m_pCurMedium->m_MediumFlags & XW_MEDIUM_FOG) && ms_pSceneFog &&  ms_pSceneFog->NHFEnable();

					int nVBFP = m_lVBFacePrim.Len();
					const CBSP3_VBFacePrim* pVBFP = m_lVBFacePrim.GetBasePtr();
					CXR_VBIDChain *pChain = (CXR_VBIDChain *)ms_pVBM->Alloc( sizeof( CXR_VBIDChain ) * nVBFP );
					if( !pChain )	return;
					for( int iFace = 0; iFace < nVBFP; iFace++ )
					{
						pChain->Clear();
						pChain->Render_VertexBuffer(pVBFP->m_VBID);
						m_lVBQueues[pVBFP->m_iVB].AddChain(pChain);
						pVBFP++;
						pChain++;
					}

	#ifndef MODEL_BSP_DISABLENHF
					if (m_pCurMedium->m_MediumFlags & XW_MEDIUM_FOG)
					{
						if (ms_pSceneFog && ms_pSceneFog->NHFEnable())
						{
							RenderNHF(liVisFaces, nVisFaces, 0);
						}
					}
	#endif
				}

				VB_RenderQueues();

	//			if (ms_pSceneFog) ms_pSceneFog->TraceBoundRelease();
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
#endif // M_Profile
	}

	m_pCurAnimState = NULL;
}

// -------------------------------------------------------------------
void CXR_Model_BSP3::OnPrecache(CXR_Engine* _pEngine, int _iVariation)
{
	MAUTOSTRIP(CXR_Model_BSP_OnPrecache, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("Read", "No texture-context available.");

	// Precache textures
	{
		// Set precache flags on all textures
/*		ms_TextureID_AttenuationExp = pTC->GetTextureID("SPECIAL_ATTENUATION3");
		ms_TextureID_Normalize = pTC->GetTextureID("SPECIAL_NORMALIZE0");
		ms_TextureID_CubeNoise = pTC->GetTextureID("CUBENOISE_01");
		ms_TextureID_Specular[0] = pTC->GetTextureID("SPECIAL_SPECULAR001_0");
		ms_TextureID_Specular[1] = pTC->GetTextureID("SPECIAL_SPECULAR002_0");
		ms_TextureID_Specular[2] = pTC->GetTextureID("SPECIAL_SPECULAR004_0");
		ms_TextureID_Specular[3] = pTC->GetTextureID("SPECIAL_SPECULAR008_0");
		ms_TextureID_Specular[4] = pTC->GetTextureID("SPECIAL_SPECULAR016_0");
		ms_TextureID_Specular[5] = pTC->GetTextureID("SPECIAL_SPECULAR032_0");
		ms_TextureID_Specular[6] = pTC->GetTextureID("SPECIAL_SPECULAR064_0");
		ms_TextureID_Specular[7] = pTC->GetTextureID("SPECIAL_SPECULAR128_0");*/
//		ms_TextureID_DynLightProj = pTC->GetTextureID("Special_Sbz128");
		ms_TextureID_DefaultLens = pTC->GetTextureID("Special_DefaultLens2D");
		ms_TextureID_AttenuationExp = pTC->GetTextureID("Special_Attn3RGBA");
		
//		extern int _ms_TextureID_AttenuationExp, _ms_TextureID_DefaultLens;
//		_ms_TextureID_AttenuationExp	= ms_TextureID_AttenuationExp;
//		_ms_TextureID_DefaultLens = ms_TextureID_DefaultLens;
		
//		ms_TextureID_AttenuationExp = pTC->GetTextureID("SPECIAL_ATTENUATION3");
//		ms_TextureID_AttenuationExp = pTC->GetTextureID("test_ATTENUATION");


//ms_TextureID_TestBump = pTC->GetTextureID("Cube_20L_0");
/*		pTC->SetTextureParam(ms_TextureID_AttenuationExp, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
		pTC->SetTextureParam(ms_TextureID_Normalize, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
		pTC->SetTextureParam(ms_TextureID_CubeNoise, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
		for(int i = 0; i < 8; i++)
			pTC->SetTextureParam(ms_TextureID_Specular[i], CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);*/
//		pTC->SetTextureParam(ms_TextureID_DynLightProj, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
		pTC->SetTextureParam(ms_TextureID_DefaultLens, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
		pTC->SetTextureParam(ms_TextureID_AttenuationExp, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);

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

	if( m_spLightData )
	{
		spCTextureContainer_Plain spLMTC = m_spLightData->m_spLMTC;
		if( spLMTC )
		{
			int nLMC = spLMTC->GetNumLocal();
			for( int i = 0; i < nLMC; i++ )
			{
				if( spLMTC->GetTextureID(i))
					pTC->SetTextureParam(spLMTC->GetTextureID(i), CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);		// Causes occational calls to BuildInto when pWLS is invalid.
			}
		}
	}
	
	// Set precache flags on all vertex buffers
	{
/*
		for(int i = 0; i < m_lspVBInfo.Len(); i++)
		{
			int VBID = m_lspVBInfo[i]->m_VBID;
			if (VBID)
				_pEngine->m_pVBC->VB_SetFlags(VBID, _pEngine->m_pVBC->VB_GetFlags(VBID) | CXR_VBFLAGS_PRECACHE);
		}
*/
		if( m_spLightData )
		{
			int nSV = m_spLightData->GetSVCount();
			for( int i = 0; i < nSV; i++ )
			{
				if( m_spLightData->m_plSV[i].m_nLMC > 0 )
				{
					int VBID = m_spLightData->m_plSVVBID[i];
					if( VBID )
						_pEngine->m_pVBC->VB_SetFlags( VBID, _pEngine->m_pVBC->VB_GetFlags(VBID) | CXR_VBFLAGS_PRECACHE);
				}
			}
		}
		
		int Len = m_lVBFacePrim.Len();
		for( int i = 0; i < Len; i++ )
		{
			int VBID = m_lVBFacePrim[i].m_VBID;
			if( VBID )
				_pEngine->m_pVBC->VB_SetFlags(VBID, _pEngine->m_pVBC->VB_GetFlags(VBID) | CXR_VBFLAGS_PRECACHE);
		}
	}
}

// -------------------------------------------------------------------
void CXR_Model_BSP3::OnPostPrecache(CXR_Engine* _pEngine)
{
	MAUTOSTRIP( CXR_Model_BSP3_OnPostPrecache, MAUTOSTRIP_VOID );
	BSP3Precache.m_lPrimStream.Destroy();
	if( m_spLightData )
	{
		m_spLightData->OnPostPrecache();
	}
}
// -------------------------------------------------------------------

