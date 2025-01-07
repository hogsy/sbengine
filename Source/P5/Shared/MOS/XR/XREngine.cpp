#include "PCH.h"

#include "XREngine.h"
#include "XREngineImp.h"
#include "XRFog.h"
#include "XRVertexBuffer.h"
#include "XRVBManager.h"
#include "XRVBUtil.h"
#include "MFloat.h"
#include "XRUtil.h"
#include "XRSurfaceContext.h"

#include "XREngineVar.h"
#include "../Classes/Render/MWireContainer.h"
#include "../XRModels/Model_Sky/WSky.h"

#ifdef PLATFORM_PS2
#include "MDispPS2.h"
#include "MRndrPS2_DmaEngine.h"
#include "MRndrPS2_DmaChain.h"
#endif


void CXR_Engine_PostProcessParams::SetDefault()
{
	for(int i = 0; i < XR_PPMAXHSC; i++)
	{
		m_lHSC_Intensity[i] = 1.0f;
		m_lHSC_Blend[i] = 1.0f;
		m_lHSC_TextureID[i] = 0;
	}
	m_nHSC = 0;

	m_DebugFlags = 0;
	m_GlowAttenuationExp = 2;
	m_GlowBias = CVec4Dfp32(-0.1f, -0.1f, -0.1f, 0.0f);
	m_GlowCenter = 0.5f;
	m_GlowScale = 1;
	m_GlowGamma = 1;
	m_pGlowFilter = NULL;
	m_bDynamicExposure = true;
//	m_ExposureExp.SetScalar(1.1f);
	m_ExposureExp.Set(1.05f, 1.05, 1.2f);
	m_ExposureScale.SetScalar(0.75f);
	m_ExposureContrast = 1.0f;
	m_ExposureSaturation = 1.0f;
	m_ExposureBlackLevel = 0.0f;
	m_ViewFlags = 0;
	m_AspectChange = 1.0f;
}

// -------------------------------------------------------------------
//  Move to MMath.h, RAUS !!!!!!
// -------------------------------------------------------------------
#define MACRO_ISMIRRORED(Mat)	\
	((CVec3Dfp32::GetMatrixRow(Mat, 0) / CVec3Dfp32::GetMatrixRow(Mat, 1)) * CVec3Dfp32::GetMatrixRow(Mat, 2) < 0.0f)

static int RemoveColinearPoints(CVec3Dfp32* _pV, CVec3Dfp32* _pDest, int _nV)
{
	MAUTOSTRIP(RemoveColinearPoints, 0);
	CVec3Dfp32 Temp[64];

	bool bNoDest = false;
	if (!_pDest)
	{
		_pDest = Temp;
		bNoDest = true;
	}

	int nVerts = 0;
	for(int v = 0; v < _nV; v++)
	{
		int iv0 = (v + _nV-1) % _nV;
		int iv1 = v;
		int iv2 = (v+1) % _nV;
		CVec3Dfp32 e0, e1, n;
		_pV[iv2].Sub(_pV[iv1], e1);
		_pV[iv1].Sub(_pV[iv0], e0);
		if (e0.LengthSqr() < Sqr(0.1f)) continue;	// Removes points that are on top of each-other
		e1.CrossProd(e0, n);
		if (n.LengthSqr() > 0.01f)
			_pDest[nVerts++] = _pV[iv1];
	}

	if (bNoDest)
		memcpy(_pV, Temp, nVerts*sizeof(CVec3Dfp32));

	return nVerts;
}

static int RemoveColinearPoints(CVec3Dfp64* _pV, CVec3Dfp64* _pDest, int _nV)
{
	MAUTOSTRIP(RemoveColinearPoints_2, 0);
	CVec3Dfp64 Temp[64];

	bool bNoDest = false;
	if (!_pDest)
	{
		_pDest = Temp;
		bNoDest = true;
	}

	int nVerts = 0;
	for(int v = 0; v < _nV; v++)
	{
		int iv0 = (v + _nV-1) % _nV;
		int iv1 = v;
		int iv2 = (v+1) % _nV;
		CVec3Dfp64 e0, e1, n;
		_pV[iv2].Sub(_pV[iv1], e1);
		_pV[iv1].Sub(_pV[iv0], e0);
		if (e0.LengthSqr() < Sqr(0.1f)) continue;	// Removes points that are on top of each-other
		e1.CrossProd(e0, n);
		if (n.LengthSqr() > 0.01f)
			_pDest[nVerts++] = _pV[iv1];
	}

	if (bNoDest)
		memcpy(_pV, Temp, nVerts*sizeof(CVec3Dfp64));

	return nVerts;
}

// Move to MCC/MMisc.h
static int GetLEPow2(int _a)
{
	MAUTOSTRIP(GetLEPow2, 0);
	int x = 1;
	while (x+x <= _a) x += x;
	return x;
};

static CPlane3Dfp64 ConvertPlane8(const CPlane3Dfp32& _p)
{
	MAUTOSTRIP(ConvertPlane8, CPlane3Dfp64());
	CPlane3Dfp64 P;
	P.n = _p.n.Getfp64();
	P.d = _p.d;
	return P;
}


// -------------------------------------------------------------------
//  Move to CRC_ClipVolume, RAUS !!!!!!
// -------------------------------------------------------------------
void TransformClipVolume(CRC_ClipVolume& _Clip, const CMat4Dfp32& _Mat);
void TransformClipVolume(CRC_ClipVolume& _Clip, const CMat4Dfp32& _Mat)
{
	MAUTOSTRIP(TransformClipVolume, MAUTOSTRIP_VOID);
	_Clip.m_POV *= _Mat;

	for(int i = 0; i < _Clip.m_nPlanes; i++)
	{
		_Clip.m_Vertices[i] *= _Mat;
		_Clip.m_Planes[i].Transform(_Mat);
	}
}

void InvertClipVolume(CRC_ClipVolume& _Clip);
void InvertClipVolume(CRC_ClipVolume& _Clip)
{
	MAUTOSTRIP(InvertClipVolume, MAUTOSTRIP_VOID);
	CRC_ClipVolume Tmp;
	Tmp.Copy(_Clip);

	for(int i = 0; i < _Clip.m_nPlanes; i++)
	{
		int iDst = _Clip.m_nPlanes-1-i;
		_Clip.m_Vertices[iDst] = Tmp.m_Vertices[i];
		_Clip.m_Planes[iDst] = Tmp.m_Planes[i];
		_Clip.m_Planes[iDst].Inverse();
	}
}
/*
static void RC_RenderRect(CRenderContext* _pRC, CRC_Viewport* _pVP, const CRect2Duint16& _Rect, CPixel32 _Color)
{
	Error_static("RC_RenderRect", "Does not work since it uses CRenderContext");
	CVec3Dfp32 lV[4];

	CMat4Dfp32 Mat;
	CXR_Util::VBM_Get2DMatrix_RelBackPlane(_pVP, Mat);

	CVec3Dfp32* pV = lV;

	CVec3Dfp32(_Rect.m_Min[0], _Rect.m_Min[1], 0).MultiplyMatrix(Mat, pV[0]);
	CVec3Dfp32(_Rect.m_Max[0], _Rect.m_Min[1], 0).MultiplyMatrix(Mat, pV[1]);
	CVec3Dfp32(_Rect.m_Max[0], _Rect.m_Max[1], 0).MultiplyMatrix(Mat, pV[2]);
	CVec3Dfp32(_Rect.m_Min[0], _Rect.m_Max[1], 0).MultiplyMatrix(Mat, pV[3]);

	CXR_Util::Init();
	_pRC->Geometry_VertexArray(pV, 4);
	_pRC->Geometry_Color(_Color);
	_pRC->Render_IndexedTriangles(CXR_Util::m_lQuadParticleTriangles, 2);
	_pRC->Geometry_VertexArray(NULL, 0);
	_pRC->Geometry_Color(0xffffff);
}
*/
static void ClearViewport(CRenderContext* _pRC, CXR_VBManager* _pVBM, CPixel32 _ColorValue = 0, fp32 _ZValue = 1.0f, int _StencilValue = 0, int _ClearFlags = -1)
{
	MAUTOSTRIP(ClearViewport, MAUTOSTRIP_VOID);
	MSCOPE(ClearViewport, XR);

	CRC_Attributes Attrib;
	Attrib.SetDefault();
	if (_ClearFlags & 1)
		Attrib.Attrib_Enable(CRC_FLAGS_COLORWRITE);
	else
		Attrib.Attrib_Disable(CRC_FLAGS_COLORWRITE);
		
	if (_ClearFlags & 2)
		Attrib.Attrib_Enable(CRC_FLAGS_ALPHAWRITE);
	else
		Attrib.Attrib_Disable(CRC_FLAGS_ALPHAWRITE);

	if (_ClearFlags & 4)
		Attrib.Attrib_Enable(CRC_FLAGS_ZWRITE);
	else
		Attrib.Attrib_Disable(CRC_FLAGS_ZWRITE);
		
	if (_ClearFlags & 8)
		Attrib.Attrib_Enable(CRC_FLAGS_STENCIL);
	else
		Attrib.Attrib_Disable(CRC_FLAGS_STENCIL);

	Attrib.Attrib_Disable(/*CRC_FLAGS_ZCOMPARE |*/ CRC_FLAGS_CULL);
	Attrib.Attrib_ZCompare(CRC_COMPARE_ALWAYS);
	Attrib.Attrib_StencilRef(_StencilValue, -1);
	Attrib.Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_REPLACE, CRC_STENCILOP_REPLACE);
	Attrib.Attrib_StencilWriteMask(-1);

	CRC_Viewport VP;
	if(_pVBM)
		VP = *_pVBM->Viewport_Get();
	else
		VP = *_pRC->Viewport_Get();
	const CVec3Dfp32* pVV = VP.GetViewVertices();
	M_ASSERT(VP.GetNumViewVertices() == 5, "!");

	CVec3Dfp32 lVerts[4];
	for(int i = 0; i < 4; i++)
	{
		pVV[i+1].Scale(_ZValue, lVerts[i]);
/*		lVerts[i][0] *= 0.5;
		lVerts[i][1] *= 0.5;*/
	}

	uint16 lTriPrim[6] = { 0,1,2,0,2,3 };
	if(_pVBM)
	{
		CXR_VertexBuffer* pVB = _pVBM->Alloc_VB(CXR_VB_ATTRIB);
		if(!pVB)
			return;
		if (!pVB->AllocVBChain(_pVBM, false))
			return;

		CXR_VBChain *pChain = pVB->GetVBChain();
		pChain->m_pV	= _pVBM->Alloc_V3(4);
		pChain->m_piPrim	= _pVBM->Alloc_Int16(6);
		if(!pChain->m_pV || !pChain->m_piPrim)
			return;

		pChain->m_nPrim		= 2;
		pChain->m_nV		= 4;
		*pVB->m_pAttrib	= Attrib;
		pVB->m_Color	= _ColorValue;
		pChain->m_PrimType	= CRC_RIP_TRIANGLES;
		memcpy(pChain->m_pV, lVerts, sizeof(CVec3Dfp32) * 4);
		memcpy(pChain->m_piPrim, lTriPrim, sizeof(uint16) * 6);

		_pVBM->AddVB(pVB);
	}
	else
	{
		_pRC->Attrib_Push();
		_pRC->Matrix_Push();

		CMat4Dfp32 Unit;
		Unit.Unit();
		_pRC->Matrix_SetMode(CRC_MATRIX_MODEL);
		_pRC->Matrix_Set(Unit);

		_pRC->Attrib_Set(Attrib);

	//	_pRC->Viewport_Update();

		_pRC->Geometry_Clear();
		_pRC->Geometry_Color(_ColorValue);
		_pRC->Geometry_VertexArray(lVerts, 4, true);

		_pRC->Render_IndexedTriangles(lTriPrim, 2);
		
		_pRC->Matrix_Pop();
		_pRC->Attrib_Pop();
	}
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_PortalTexture
|__________________________________________________________________________________________________
\*************************************************************************************************/

// -------------------------------------------------------------------
//#define TEXTUREPORTAL_SIZE 256
MRTC_IMPLEMENT(CTextureContainer_Render, CTextureContainer);

void CTextureContainer_Render::Create(int _nTextures, CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CTextureContainer_Render_Create, MAUTOSTRIP_VOID);
	m_pEngine = _pEngine;
	m_lTxtInfo.SetLen(_nTextures);

	for(int i = 0; i < m_lTxtInfo.Len(); i++)
	{
		m_lTxtInfo[i].m_TextureID = m_pTC->AllocID(m_iTextureClass, i, (const char *)NULL);
		m_lTxtInfo[i].m_Width = 256;
		m_lTxtInfo[i].m_Height = 256;
	}
}

CTextureContainer_Render::~CTextureContainer_Render()
{
	MAUTOSTRIP(CTextureContainer_Render_dtor, MAUTOSTRIP_VOID);
	if (m_pTC)
	{
		for(int i = 0; i < m_lTxtInfo.Len(); i++)
		{
			if (m_lTxtInfo[i].m_TextureID) m_pTC->FreeID(m_lTxtInfo[i].m_TextureID);
			m_lTxtInfo[i].m_TextureID = 0;
		}
	}
}

void CTextureContainer_Render::PrepareFrame(CPnt _TextureSize)
{
	MAUTOSTRIP(CTextureContainer_Render_PrepareFrame, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_lTxtInfo.Len(); i++)
	{
		if (m_lTxtInfo[i].m_Width != _TextureSize.x ||
			m_lTxtInfo[i].m_Height != _TextureSize.y)
			m_pTC->MakeDirty(m_lTxtInfo[i].m_TextureID);

		m_lTxtInfo[i].m_Width = _TextureSize.x;
		m_lTxtInfo[i].m_Height = _TextureSize.y;

	}
}

int CTextureContainer_Render::GetNumLocal()
{
	MAUTOSTRIP(CTextureContainer_Render_GetNumLocal, 0);
	return m_lTxtInfo.Len();
}

int CTextureContainer_Render::GetTextureID(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Render_GetTextureID, 0);
	return m_lTxtInfo[_iLocal].m_TextureID;
}

int CTextureContainer_Render::GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps)
{
	MAUTOSTRIP(CTextureContainer_Render_GetTextureDesc, 0);
	CRenderTextureInfo* pInfo = &m_lTxtInfo[_iLocal];

	_Ret_nMipmaps = 1;
	if (_pTargetImg) 
		_pTargetImg->CreateVirtual(pInfo->m_Width, pInfo->m_Height, pInfo->m_Format, IMAGE_MEM_VIDEO | IMAGE_MEM_TEXTURE);

	return 0;		// CTC_TEXTURE_xxx flags
}

void CTextureContainer_Render::GetTextureProperties(int _iLocal, CTC_TextureProperties& _Prop)
{
	MAUTOSTRIP(CTextureContainer_Render_GetTextureProperties, MAUTOSTRIP_VOID);

	CTC_TextureProperties Prop;
	Prop.m_Flags |= 
		CTC_TEXTUREFLAGS_NOMIPMAP | 
		CTC_TEXTUREFLAGS_NOPICMIP | 
		CTC_TEXTUREFLAGS_CLAMP_U | 
		CTC_TEXTUREFLAGS_CLAMP_V | 
		CTC_TEXTUREFLAGS_HIGHQUALITY | 
		CTC_TEXTUREFLAGS_NOCOMPRESS | 
		CTC_TEXTUREFLAGS_RENDER |
		CTC_TEXTUREFLAGS_RENDERUSEBACKBUFFERFORMAT;

	Prop.m_MinFilter = CTC_MINFILTER_LINEAR;
	Prop.m_MagFilter = CTC_MAGFILTER_LINEAR;
	Prop.m_MIPFilter = CTC_MIPFILTER_NEAREST;

	_Prop = Prop;
}

void CTextureContainer_Render::BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _TextureVersion, int _ConvertType, int _iStartMip, uint32 _BulidFlags)
{
	MAUTOSTRIP(CTextureContainer_Render_BuildInto, MAUTOSTRIP_VOID);
	Error("BuildInto", CStrF("Invalid call. This texture is a render-texture. Local %d, TextureID %d", _iLocal, GetTextureID(_iLocal) ));
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_EnginePortals
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_DYNAMIC(CTextureContainer_EnginePortals, CTextureContainer_Render);

CTextureContainer_EnginePortals::CTextureContainer_EnginePortals()
{
	MAUTOSTRIP(CTextureContainer_EnginePortals_ctor, MAUTOSTRIP_VOID);
	m_iLocalEnabled = -1;
}

void CTextureContainer_EnginePortals::BuildInto(int _iLocal, class CRenderContext* _pRC)
{
	MAUTOSTRIP(CTextureContainer_EnginePortals_BuildInto, MAUTOSTRIP_VOID);
	MSCOPESHORT(CTextureContainer_EnginePortals::BuildInto);
	// Set is to ignore any and all texture build requests except 
	// those issued from CXR_Engine::Engine_RVC_RenderPortals.
	if (_iLocal == m_iLocalEnabled)
		m_pEngine->Engine_RenderViewContext();
}

void CTextureContainer_EnginePortals::GetTextureProperties(int _iLocal, CTC_TextureProperties& _Prop)
{
	CTextureContainer_Render::GetTextureProperties(_iLocal, _Prop);

	// Make sure portals render using same format as backbuffer
	_Prop.m_Flags |= CTC_TEXTUREFLAGS_RENDERUSEBACKBUFFERFORMAT;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_ShadowDecals
|__________________________________________________________________________________________________
\*************************************************************************************************/
#ifdef XR_SUPPORT_SHADOWDECALS

MRTC_IMPLEMENT_DYNAMIC(CTextureContainer_ShadowDecals, CTextureContainer_Render);

CTextureContainer_ShadowDecals::CTextureContainer_ShadowDecals()
{
	MAUTOSTRIP(CTextureContainer_ShadowDecals_ctor, MAUTOSTRIP_VOID);
	m_iNextTexture = 0;
	m_iLastTexture = 0;
	m_bFull = false;
	m_Tick = 0;
}

void CTextureContainer_ShadowDecals::PrepareFrame(CPnt _TextureSize)
{
	MAUTOSTRIP(CTextureContainer_ShadowDecals_PrepareFrame, MAUTOSTRIP_VOID);
	CTextureContainer_Render::PrepareFrame(_TextureSize);
	m_iLastTexture = m_iNextTexture;
	m_bFull = false;

	if (m_lShadows.Len() != m_lTxtInfo.Len())
	{
		m_lShadows.Clear();
		m_lShadows.SetLen(m_lTxtInfo.Len());
	}

	m_Tick++;

//	m_iNextTexture = 0;
}

int CTextureContainer_ShadowDecals::Shadow_Begin(const CMat4Dfp32& _CameraWMat, fp32 _ProjSize, int _GUID, int _Update)
{
	MAUTOSTRIP(CTextureContainer_ShadowDecals_Shadow_Begin, 0);
	// Only allow decals for recursion level 0 since decals can only 
	// be rendered before anything else is rendered.
	if (m_pEngine->GetVCDepth() != 0) return 0;
	if (m_bFull) return 0;

	int iShadow = 0;

	// Look for the same GUID
	for(;iShadow < m_lShadows.Len(); iShadow++)
	{
		if (m_lShadows[iShadow].m_GUID == _GUID)
		{
			if (m_Tick - m_lShadows[iShadow].m_TickRendered < _Update)
			{
				m_lShadows[iShadow].m_TickUsed = m_Tick;
				m_bIgnoreRender = true;
				return iShadow+1;
			}
			break;
		}
	}

	if (iShadow == m_lShadows.Len())
	{
		// Look for an unused GUID.
		iShadow = 0;
		for(;iShadow < m_lShadows.Len(); iShadow++)
		{
			if (m_Tick - m_lShadows[iShadow].m_TickUsed > 1)
				break;
		}
	}

	if (iShadow == m_lShadows.Len())
	{
		m_bFull = true;
		return 0;
	}

	m_bIgnoreRender = false;


//	if (m_iNextTexture >= m_lTxtInfo.Len()) return 0;

	// Get display size (FIXME: Main viewport != display size)
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(!pSys->m_spDisplay)
		return 0;
	CImage* pImg = pSys->m_spDisplay->GetFrameBuffer();
	if (!pImg) return 0;

	int TextureID = m_lTxtInfo[iShadow].m_TextureID;
	CRC_Viewport* pVP= m_pEngine->m_pRender->Viewport_Get();

	// Get texture description
	CImage Desc;
	int nMip = 0;
	m_pEngine->m_pRender->Texture_GetTC()->GetTextureDesc(TextureID, &Desc, nMip);

//	int ww = pImg->GetWidth();
//	int wh = pImg->GetHeight();

	// Set viewport matching the texture area.
	CRC_Viewport VP;
	VP = *m_pEngine->m_pRender->Viewport_Get();
	VP.SetView(CClipRect(0, 0, Desc.GetWidth(), Desc.GetHeight()), CRct(0,0,Desc.GetWidth(), Desc.GetHeight()));
	VP.SetFOV(1);
	VP.SetBackPlane(100000);


	fp32 SizeX = _ProjSize;
//	fp32 SizeY = _ProjSize;

	fp32 Fov = VP.GetFOV() / 180 * _PI;
	fp32 Dist = SizeX * M_Cos(Fov) / M_Sin(Fov);
//	fp32 DistY = SizeY * M_Cos(Fov) / M_Sin(Fov);

	fp32 d = Dist;

//	fp32 Dist = Max(SizeX / m_Pos.GetWidth(), SizeY / m_Pos.GetHeight());

	const CVec3Dfp32& CamZ = CVec3Dfp32::GetRow(_CameraWMat, 2);
	CMat4Dfp32 Cam(_CameraWMat);
	CVec3Dfp32& CamPos = CVec3Dfp32::GetRow(Cam, 3);

	CamPos.Combine(CamZ, -d, CamPos);

//	m_Camera.k[3][0] = Max(Dist, DistY) * 1.0f;
//	m_Camera.k[3][2] = 0;

	// Frontplane
	CPlane3Dfp32 FrontPlaneW;
	FrontPlaneW.CreateND(CVec3Dfp32(0,0,1), -pVP->GetBackPlane());
	FrontPlaneW.Transform(Cam);

	// Clip
	CRC_ClipVolume Clip;
	pVP->GetClipVolume(Clip);

	m_pEngine->Engine_PushViewContext(Cam, CVec3Dfp32::GetRow(Cam, 3), Clip, FrontPlaneW, XR_SCENETYPE_SHADOWDECAL);
	m_lTxtInfo[iShadow].m_iVC = m_pEngine->GetVCDepth();


//	VP.SetAspectRatio(fp32(ww) / fp32(wh*1.05f) * VPCurrent.GetAspectRatio());

	m_pEngine->m_pRender->Viewport_Push();
	m_pEngine->m_pRender->Viewport_Set(&VP);

	m_lShadows[iShadow].m_TickUsed = m_Tick;
	m_lShadows[iShadow].m_TickRendered = m_Tick;
	m_lShadows[iShadow].m_GUID = _GUID;

/*	int hShadow = 1 + m_iNextTexture;
	m_iNextTexture++;
	if (m_iNextTexture >= m_lTxtInfo.Len())
		m_iNextTexture = 0;

	if (m_iNextTexture == m_iLastTexture)
		m_bFull = true;

  */
	return iShadow+1;
}

void CTextureContainer_ShadowDecals::Shadow_AddModel(int _hShadow, CXR_Model* _pModel, const CMat4Dfp32& _Pos, const CXR_AnimState& _Anim)
{
	MAUTOSTRIP(CTextureContainer_ShadowDecals_Shadow_AddModel, MAUTOSTRIP_VOID);
	if (m_bIgnoreRender)
		return;

	_hShadow--;
	m_pEngine->Render_AddModel(_pModel, _Pos, _Anim);
}

void CTextureContainer_ShadowDecals::Shadow_End(int _hShadow)
{
	MAUTOSTRIP(CTextureContainer_ShadowDecals_Shadow_End, MAUTOSTRIP_VOID);
	if (m_bIgnoreRender)
		return;

	_hShadow--;
	int TextureID = m_lTxtInfo[_hShadow].m_TextureID;
	int iVC = m_lTxtInfo[_hShadow].m_iVC;
	if (iVC != m_pEngine->GetVCDepth())
		Error("Shadow_End", "Recursion level missmatch.");


	m_pEngine->GetTC()->MakeDirty(TextureID);
	m_pEngine->m_pRender->Texture_Precache(TextureID);

	m_pEngine->Engine_PopViewContext();
	m_pEngine->m_pRender->Viewport_Pop();
}

int CTextureContainer_ShadowDecals::Shadow_GetTextureID(int _hShadow)
{
	MAUTOSTRIP(CTextureContainer_ShadowDecals_Shadow_GetTextureID, 0);
	_hShadow--;
	return m_lTxtInfo[_hShadow].m_TextureID;
}

int CTextureContainer_ShadowDecals::Shadow_GetNumUsed()
{
	MAUTOSTRIP(CTextureContainer_ShadowDecals_Shadow_GetNumUsed, 0);
	return m_iNextTexture;
}

void CTextureContainer_ShadowDecals::BuildInto(int _iLocal, class CRenderContext* _pRC)
{
	MAUTOSTRIP(CTextureContainer_ShadowDecals_BuildInto, MAUTOSTRIP_VOID);
	MSCOPE(CTextureContainer_ShadowDecals::BuildInto, XR);

	// Just to make sure the framebuffer is OK.
/*	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	spCImage spImg = pSys->m_spDisplay->GetFrameBuffer();
//	if (!spImg) return;

	glDepthMask(1);
	CPixel32 _Col = 0x00ffff00;
	glClearDepth(1.0);
	glClearColor(fp32(_Col.GetR()) / 255.0f, fp32(_Col.GetG()) / 255.0f, fp32(_Col.GetB()) / 255.0f, 0);
//	glDepthFunc(GL_LEQUAL);
//	GLErr("ClearFrameBuffer");
//	int Flags = 0; 
//	if (_Buffers & CDC_CLEAR_COLOR) Flags |= GL_COLOR_BUFFER_BIT;
//	if (_Buffers & CDC_CLEAR_ZBUFFER) Flags |= GL_DEPTH_BUFFER_BIT;
//	Flags |= GL_STENCIL_BUFFER_BIT;
//	glClearStencil(128);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GLErr("ClearFrameBuffer");
*/

	// FIXME: ONLY CLEAR THE VIEWPORT AREA!
//	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
//	if (pSys->m_spDisplay != NULL) pSys->m_spDisplay->ClearFrameBuffer(-1, 0x00ffff00);

	ClearViewport(_pRC, 0, 0x00000000, 0.99f, 0, 1+2+4);


	m_pEngine->Engine_RenderViewContext();
}

#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_Screen
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_DYNAMIC(CTextureContainer_Screen, CTextureContainer_Render);

void CTextureContainer_Screen::SetTextureProperties(int _iLocal, CTC_TextureProperties& _Prop)
{
	m_lProperties[_iLocal] = _Prop;
}

void CTextureContainer_Screen::SetTextureFormat(int _iLocal, uint32 _Format)
{
	CRenderTextureInfo& Info = m_lTxtInfo[_iLocal];
	if (Info.m_Format == _Format)
		return;

	Info.m_Format = _Format;
	m_pTC->MakeDirty(Info.m_TextureID);
}

CTextureContainer_Screen::CTextureContainer_Screen()
{
	MAUTOSTRIP(CTextureContainer_Screen_ctor, MAUTOSTRIP_VOID);
}

void CTextureContainer_Screen::Create(int _nTextures, class CXR_Engine* _pEngine)
{
	CTextureContainer_Render::Create(_nTextures, _pEngine);

	CTC_TextureProperties Prop;
	Prop.m_Flags |= 
		CTC_TEXTUREFLAGS_NOMIPMAP | 
		CTC_TEXTUREFLAGS_NOPICMIP | 
		CTC_TEXTUREFLAGS_CLAMP_U | 
		CTC_TEXTUREFLAGS_CLAMP_V | 
		CTC_TEXTUREFLAGS_HIGHQUALITY | 
		CTC_TEXTUREFLAGS_NOCOMPRESS | 
		CTC_TEXTUREFLAGS_RENDER |
		CTC_TEXTUREFLAGS_BACKBUFFER | 
		CTC_TEXTUREFLAGS_BACKBUFFERDISCARDOLD;

	Prop.m_MinFilter = CTC_MINFILTER_LINEAR;
	Prop.m_MagFilter = CTC_MAGFILTER_LINEAR;
	Prop.m_MIPFilter = CTC_MIPFILTER_NEAREST;

	m_lProperties.SetLen(_nTextures);
	m_lFormats.SetLen(_nTextures);
	for(int i = 0; i < _nTextures; i++)
	{
		m_lProperties[i] = Prop;
	}
}

void CTextureContainer_Screen::PrepareFrame(CPnt _TextureSize)
{
	MAUTOSTRIP(CTextureContainer_Screen_PrepareFrame, MAUTOSTRIP_VOID);

	
	CTextureContainer_Render::PrepareFrame(_TextureSize);
}

void CTextureContainer_Screen::GetSnapshot(int _iScreen, CRenderContext *_pRC)
{
	MAUTOSTRIP(CTextureContainer_Screen_GetSnapshot, MAUTOSTRIP_VOID);

	int TextureID = m_lTxtInfo[_iScreen].m_TextureID;
	
	if(_pRC)
	{
		_pRC->Texture_GetTC()->MakeDirty(TextureID);
		_pRC->Texture_Precache(TextureID);

	}
	else if(m_pEngine)
	{
		m_pEngine->GetTC()->MakeDirty(TextureID);
		m_pEngine->m_pRender->Texture_Precache(TextureID);
	}
}

int CTextureContainer_Screen::GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps)
{
	MAUTOSTRIP(CTextureContainer_Render_GetTextureDesc, 0);
	CRenderTextureInfo* pInfo = &m_lTxtInfo[_iLocal];

	_Ret_nMipmaps = 1;
	if (_pTargetImg) 
		_pTargetImg->CreateVirtual(pInfo->m_Width, pInfo->m_Height, pInfo->m_Format, IMAGE_MEM_VIDEO | IMAGE_MEM_TEXTURE);
//		_pTargetImg->CreateVirtual(pInfo->m_Width, pInfo->m_Height, IMAGE_FORMAT_BGRA8, IMAGE_MEM_VIDEO | IMAGE_MEM_TEXTURE);

	return 0;		// CTC_TEXTURE_xxx flags
}

void CTextureContainer_Screen::GetTextureProperties(int _iLocal, CTC_TextureProperties& _Prop)
{
	MAUTOSTRIP(CTextureContainer_Render_GetTextureProperties, MAUTOSTRIP_VOID);
//	CRenderTextureInfo* pInfo = &m_lTxtInfo[_iLocal];


	_Prop = m_lProperties[_iLocal];
}

void CTextureContainer_Screen::BuildInto(int _iLocal, class CRenderContext* _pRC)
{
	MAUTOSTRIP(CTextureContainer_Screen_BuildInto, MAUTOSTRIP_VOID);

	// Nada...
}


#ifdef _DEBUG_TEXTUREID_SCREEN
int CTextureContainer_Screen::GetTextureID(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Screen_GetTextureID, 0);

	if (_iLocal == 6)
	{
		// Why are we picking out ID 6!? We're trying to remove this!
		M_BREAKPOINT;
	}

	return CTextureContainer_Render::GetTextureID(_iLocal);
}
#endif


MRTC_IMPLEMENT_DYNAMIC(CTextureContainer_ScreenCopy, CTextureContainer_Screen);
void CTextureContainer_ScreenCopy::GetTextureProperties(int _iLocal, CTC_TextureProperties& _Prop)
{
	MAUTOSTRIP(CTextureContainer_Render_GetTextureProperties, MAUTOSTRIP_VOID);
//	CRenderTextureInfo* pInfo = &m_lTxtInfo[_iLocal];

	CTC_TextureProperties Prop;
	Prop.m_Flags |= 
		CTC_TEXTUREFLAGS_NOMIPMAP | 
		CTC_TEXTUREFLAGS_NOPICMIP | 
		CTC_TEXTUREFLAGS_CLAMP_U | 
		CTC_TEXTUREFLAGS_CLAMP_V | 
		CTC_TEXTUREFLAGS_HIGHQUALITY | 
		CTC_TEXTUREFLAGS_NOCOMPRESS | 
		CTC_TEXTUREFLAGS_RENDER |
		CTC_TEXTUREFLAGS_BACKBUFFER;

	Prop.m_MinFilter = CTC_MINFILTER_LINEAR;
	Prop.m_MagFilter = CTC_MAGFILTER_LINEAR;
	Prop.m_MIPFilter = CTC_MIPFILTER_NEAREST;

	_Prop = Prop;
}



MRTC_IMPLEMENT_DYNAMIC(CTextureContainer_RenderCallback, CTextureContainer_Render);
CTextureContainer_RenderCallback::CTextureContainer_RenderCallback()
{
	m_pfnCallback = NULL;
}


void CTextureContainer_RenderCallback::BuildInto(int _iLocal, class CRenderContext* _pRC)
{
	MSCOPESHORT(CTextureContainer_RenderCallback::BuildInto);
	if(m_pfnCallback)
		m_pfnCallback(_iLocal, _pRC, m_pData);
}


// -------------------------------------------------------------------
//  CXR_Portal
// -------------------------------------------------------------------
void CXR_Portal::Clear()
{
	MAUTOSTRIP(CXR_Portal_Clear, MAUTOSTRIP_VOID);
	m_pSurfRender = NULL;
	m_pSurfContext = NULL;
	m_TextureID = 0;
	m_iLocalTexture = -1;
}

CXR_Portal::CXR_Portal()
{
	MAUTOSTRIP(CXR_Portal_ctor, MAUTOSTRIP_VOID);
	Clear();
}

const CXR_Portal& CXR_Portal::operator= (const CXR_Portal& _Src)
{
	MAUTOSTRIP(CXR_Portal_operator_assign, *((CXR_Portal*)NULL));
//	Error("CXR_Portal::operator=", "Don't use this.");
	m_Plane = _Src.m_Plane;
	m_PlaneDest = _Src.m_PlaneDest;
	m_Portal = _Src.m_Portal;
	m_PortalDest = _Src.m_PortalDest;
	m_WCam = _Src.m_WCam;
	m_pSurfRender = _Src.m_pSurfRender;
	m_pSurfContext = _Src.m_pSurfContext;
	m_SortZ = _Src.m_SortZ;
	m_SurfColor = _Src.m_SurfColor;
	m_VisPos = _Src.m_VisPos;
	m_ModelVMat = _Src.m_ModelVMat;
	m_ModelWMat = _Src.m_ModelWMat;
	return *this;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_ViewContext
|__________________________________________________________________________________________________
\*************************************************************************************************/
/*void CXR_ViewContext::ReflectMatrix(const CPlane3Dfp32& _Plane, const CMat4Dfp32& _Mat, CMat4Dfp32& _RefMat)
{
	MAUTOSTRIP(CXR_ViewContext_ReflectMatrix, MAUTOSTRIP_VOID);
	// Reflects _Mat in _Plane and puts the result in _RefMat
	_RefMat.Unit();
	CVec3Dfp32::GetMatrixRow(_Mat, 0).Reflect(_Plane.n, CVec3Dfp32::GetMatrixRow(_RefMat, 0));
	CVec3Dfp32::GetMatrixRow(_Mat, 1).Reflect(_Plane.n, CVec3Dfp32::GetMatrixRow(_RefMat, 1));
	CVec3Dfp32::GetMatrixRow(_Mat, 2).Reflect(_Plane.n, CVec3Dfp32::GetMatrixRow(_RefMat, 2));
	CVec3Dfp32 Pos(CVec3Dfp32::GetMatrixRow(_Mat, 3));
	Pos -= _Plane.n*(2.0f*_Plane.Distance(Pos));
	CVec3Dfp32::GetMatrixRow(_RefMat, 3) = Pos;
}*/
void CXR_ViewContext::ClearReferences()
{
	for(int i = 0; i < m_lObjects.Len(); i++)
		m_lObjects[i].m_Anim.Clear();

	m_spLightState = NULL;
}

CXR_WorldLightState *CXR_ViewContext::GetLightState()
{
	if(!m_spLightState)
	{
		MSCOPESHORT(AllocWLS);
		m_spLightState = MNew(CXR_WorldLightState);
		if (!m_spLightState)
			MemError("Create");
		m_spLightState->Create(1, 32, 1);
	}
	return m_spLightState;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_ViewContextImpl
|__________________________________________________________________________________________________
\*************************************************************************************************/

CXR_ViewContextImpl::CXR_ViewContextImpl()
{
	MAUTOSTRIP(CXR_ViewContextImpl_ctor, MAUTOSTRIP_VOID);
	m_pSky = NULL;
	m_MaxObjects = 0;
	m_nObjects = 0;
}

CXR_ViewContextImpl::~CXR_ViewContextImpl()
{
	MAUTOSTRIP(CXR_ViewContextImpl_dtor, MAUTOSTRIP_VOID);
}

void CXR_ViewContextImpl::Create(CXR_Engine* _pEngine, int _MaxObjects, int _Depth, int _MaxPortals)
{
	MAUTOSTRIP(CXR_ViewContextImpl_Create, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_ViewContextImpl::Create);
	
	m_pEngine = _pEngine;
	m_RecursionDepth = _Depth;

	{
		MSCOPESHORT(AllocPortals);
		m_lPortals.SetLen(_MaxPortals);
		m_lpPortalsSorted.SetLen(_MaxPortals);
	}

	{
		MSCOPESHORT(AllocObjects);
		m_lObjects.SetLen(_MaxObjects);
	}
	m_pObjects = m_lObjects.GetBasePtr();
	m_MaxObjects = _MaxObjects;
	m_nObjects = 0;

	m_W2VMat.Unit();

/*	int VBSize = 1536;

 	MACRO_GetSystemEnvironment(pEnv);
	if (pEnv) VBSize = pEnv->GetValuef("XR_VBHEAP", 1536, 0);
	VBSize = Max(16, Min(1024*64, VBSize));
	
	m_pVBM = DNew(CXR_VBManager) CXR_VBManager;
	if (!m_pVBM) MemError("-");
	m_pVBM->Create(VBSize*1024, 1024);
*/
	{
		MSCOPESHORT(AllocFogState);
		m_spFogState = MNew(CXR_FogState);
		if (!m_spFogState) MemError("-");
		m_spFogState->Create(_pEngine);
	}

/*	MRTC_SAFECREATEOBJECT_NOEX(spSky, "CXR_Model_Sky", CXR_Model_Sky);
	m_spSky = spSky;
	if (!m_spSky) ConOutL("§cf80WARNING: (CXR_ViewContextImpl::CXR_ViewContextImpl) Could not create sky-object.");
	m_spSky->Create("Skytrack.txt");*/

	CMat4Dfp32 Mat; Mat.Unit();
	CPlane3Dfp32 Plane(CVec3Dfp32(0,0,1), 0);
	Clear(Mat, Mat, Plane, Plane);
}

void CXR_ViewContextImpl::OnPrecache()
{
	MAUTOSTRIP(CXR_ViewContextImpl_OnPrecache, MAUTOSTRIP_VOID);
	if (m_spFogState != NULL)
		m_spFogState->OnPrecache();
}

void CXR_ViewContextImpl::Clear(const CMat4Dfp32& _CameraWMat, const CMat4Dfp32& _CameraWVelMat, const CPlane3Dfp32& _FrontPlaneW, const CPlane3Dfp32& _BackPlaneW)
{
	MAUTOSTRIP(CXR_ViewContextImpl_Clear, MAUTOSTRIP_VOID);
	if (((CXR_EngineImpl*)m_pEngine)->m_bFreeze)
		return;

	m_FrontPlaneW = _FrontPlaneW;
#ifndef	CPU_SOFTWARE_FP64
	m_FrontPlaneW8 = ConvertPlane8(_FrontPlaneW);
#endif
	m_BackPlaneW = _BackPlaneW;
	m_bWorldFound = false;
	m_World.m_pModel = NULL;
	m_pViewClip = NULL;
	m_nObjects = 0;
//	m_W2VMat = _W2VMat;
	m_bNeedResolve_TempFixMe = 0;

	m_nPortals = 0;
	m_nPortalsSorted = 0;
	memset(m_lpPortalsSorted.GetBasePtr(), 0, m_lpPortalsSorted.ListSize());

/*	M_TRACEALWAYS("CamWVel = %s\n", _CameraWVelMat.GetString().Str() );

	{
		CMat4Dfp32 _W2VMat;
		_CameraWMat.InverseOrthogonal(_W2VMat);
		m_CameraWMat.Multiply(_W2VMat, m_dW2VMat);	// Camera should be last frame's w2vmat-inverse (hack)
		M_TRACEALWAYS("old m_dW2VMat = %s\n", m_dW2VMat.GetString().Str() );
	}*/

	m_CameraWMat = _CameraWMat;
	m_dCameraWMat = _CameraWVelMat;
	m_CameraWMat.InverseOrthogonal(m_W2VMat);
	{
		CMat4Dfp32 M, MInv;
		m_CameraWMat.Multiply(m_dCameraWMat, M);
		M.InverseOrthogonal(MInv);
		m_CameraWMat.Multiply(MInv, m_dW2VMat);

//		m_dCameraWMat.Multiply(m_CameraWMat, M1);
//		m_dCameraWMat.InverseOrthogonal(m_dW2VMat);
	}

	m_bIsMirrored = MACRO_ISMIRRORED(m_W2VMat);

//	m_CameraWMat.Multiply(_W2VMat, m_dW2VMat);	// Camera should be last frame's w2vmat-inverse (hack)

//	_W2VMat.InverseOrthogonal(m_CameraWMat);
//	m_dW2VMat.InverseOrthogonal(m_dCameraWMat);

//	M_TRACEALWAYS("new m_dW2VMat = %s\n", m_dW2VMat.GetString().Str() );

	GetLightState()->PrepareFrame();

	if(m_spFogState != NULL)
		m_spFogState->PrepareFrame(this, m_FrontPlaneW);

	m_pSky = NULL;
}

void CXR_ViewContextImpl::AddModel(CXR_Model* _pModel, const CMat4Dfp32& _Pos, const CXR_AnimState& _Anim, int _OnRenderFlags)
{
	MAUTOSTRIP(CXR_ViewContextImpl_AddModel, MAUTOSTRIP_VOID);
	// NOTE: _RenderPass is ignored.

	if (m_nObjects >= m_MaxObjects) return;
	int RenderPass = _pModel->GetRenderPass(&_Anim);
	m_pObjects[m_nObjects].Create(_pModel, _Pos, _Anim, RenderPass, _OnRenderFlags);
#ifdef M_Profile
	m_pObjects[m_nObjects].m_Time.Reset();
#endif

	m_nObjects++;
}

void CXR_ViewContextImpl::AddWorld(CXR_Model* _pModel, const CMat4Dfp32& _Pos, const CXR_AnimState& _Anim, int _OnRenderFlags)
{
	MAUTOSTRIP(CXR_ViewContextImpl_AddWorld, MAUTOSTRIP_VOID);
	if (!_pModel)
	{
		ConOut("(CXR_ViewContextImpl::AddWorld) NULL world.");
		return;
	}

	m_World = CXR_VCModelInstance(_pModel, _Pos, _Anim, _OnRenderFlags);
#ifdef M_Profile
	m_World.m_Time.Reset();
#endif
	m_pViewClip = _pModel->View_GetInterface();
	if (!m_pViewClip)
		Error("AddWorld", "No view-interface.");

	m_bWorldFound = true;
}

void CXR_ViewContextImpl::AddSky(CXR_Model_Sky* _pModel, const CMat4Dfp32& _Pos, const CXR_AnimState& _Anim)
{
	MAUTOSTRIP(CXR_ViewContextImpl_AddSky, MAUTOSTRIP_VOID);
	m_pSky = _pModel;
	if(m_pSky) 
	{
		m_SkyAnimState = _Anim;
		CXR_SkyInterface* pSI = m_pSky->Sky_GetInterface();
		if (pSI) pSI->Sky_PrepareFrame(m_pEngine);
	}
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Engine
|__________________________________________________________________________________________________
\*************************************************************************************************/

CXR_Engine::CXR_Engine()
{
	MAUTOSTRIP(CXR_Engine_ctor, MAUTOSTRIP_VOID);
	m_LightScale = 1.0f;
	m_LightScaleRecp = 1.0f / m_LightScale;
	m_pSystem = NULL;
	m_pTC = NULL;
	m_pSC = NULL;
	m_pVBC = NULL;
	m_pVBM = NULL;
	m_pSceneGraphInstance = NULL;
	m_pViewClip = NULL;
	m_pRender = NULL;
	m_pShader = NULL;
	m_pCurrentFogState = NULL;
	MemSet(m_lEngineUserColors, 0, sizeof(m_lEngineUserColors));
	m_RenderSurfaceLightScale = 1;

	m_TextureID_Screen = 0;
	m_TextureID_ResolveScreen = 0;
	m_TextureID_PostProcess = 0;		// Should be removed
	m_TextureID_Depth = 0;
	m_TextureID_DeferredNormal = 0;
	m_TextureID_DepthStencil = 0;
	m_TextureID_ShadowMask = 0;
	m_TextureID_DefaultEnvMap = 0;

	m_PostProcessParams.SetDefault();
	m_PostProcess_PrevLevels = 1.0f;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_EngineImpl
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_DYNAMIC(CXR_EngineImpl, CXR_Engine);

CXR_EngineImpl::CXR_EngineImpl()
{
	MAUTOSTRIP(CXR_EngineImpl_ctor, MAUTOSTRIP_VOID);
	m_iCurrentVC = 0;
	m_EngineMode = XR_ENGINE_MODE_LIGHTMAP;
	m_DebugFlags = 0;
	m_LightDebugFlags = 0;
	m_BSPDebugFlags = 0;
	m_bFlares = true;
	m_bDynLight = true;
	m_bFastLight = false;
	m_bWallmarks = true;
	m_bShadowDecals = true;
	m_LODOffset = 0;
	m_LODScale = 1.0f;
	m_VBSkip = 0;
	m_bShowBoundings = false;
	m_bWorldOnly = false;
	m_bObjectsOnly = false;
	m_bPortalsOnly = false;
	m_bSky = true;
	m_ShowTiming = 0;
	m_bShowFogInfo = false;
	m_bShowRecurse = false;
	m_bShowPortalFence = false;
	m_bAllowDepthFog = true;
	m_bAllowVertexFog = true;
	m_bAllowNHF = true;
	m_bCurrentVCMirrored = false;
	m_RenderCaps_Flags = 0;
	m_RenderCaps_nMultiTexture = 1;
	m_RenderCaps_nMultiTextureEnv = 0;
	m_RenderCaps_nMultiTextureCoords = 0;
	m_SurfOptions = 0;
	m_SurfCaps = 0;
	m_bStencilShadows = 0;
	m_PortalTextureSize = 256;
	m_ShadowDecalTextureSize = 128;
	m_bNoRenderToTexture = false;
	m_bTLEnableEngine = false;
	m_bTLEnableRenderer = true;
	m_bTLEnableEnabled = false;
	m_bSyncOnRender = false;


	m_ShowVBTypes = 0;
	m_bGeneratedSurfaces = true;
	m_DrawMode = false;
	m_bFreeze = false;
	m_bClearViewport = true;
	m_FogCullOffset = 0;
	m_FogCullOffsetOverride = 0;
	m_Unified_Clear = 128;
	m_Unified_Ambience = 0xff000000;
	m_PBNightVisionLight = 0;
	m_VBMFlags = 0;
	m_pClient = NULL;

	m_iiTV0 = 1; m_diiTV0 = 1;
	m_iiTV1 = 3; m_diiTV1 = 1;
	m_iiTV2 = 2; m_diiTV2 = 1;
	m_iHexagonColor = 1; m_dHexagonColor = -1;

	m_TextureID_MotionMap = 0;
	m_PostProcess_MBDebugFlags = 0;
	m_PostProcess_MBMaxRadius = 0.015;


	MACRO_GetSystemEnvironment(pEnv);
	m_nMaxVB = pEnv->GetValuei("XR_MAXVBCOUNT", 16384);

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	m_bPendingRecordVBHeap = false;
#endif
}


CXR_EngineImpl::~CXR_EngineImpl()
{
	MAUTOSTRIP(CXR_EngineImpl_dtor, MAUTOSTRIP_VOID);

//	M_TRY
//	{
		fp_UpdateEnvironment();
		RemoveFromConsole();
/*	}
	M_CATCH(
	catch(CCException)
	{
	}
	)*/
}

void CXR_EngineImpl::fp_UpdateEnvironment()
{
	MACRO_GetSystemEnvironment(pReg);
	if (pReg)
	{
		if (pReg->GetValuei("XR_FLARES", 0) != m_bFlares)
			pReg->SetValuei("XR_FLARES", m_bFlares);
		else
			pReg->DeleteKey("XR_FLARES");

		if (pReg->GetValuei("XR_DLIGHT", 1) != m_bDynLight)
			pReg->SetValuei("XR_DLIGHT", m_bDynLight);
		else
			pReg->DeleteKey("XR_DLIGHT");

		if (pReg->GetValuei("XR_FASTLIGHT", 0) != m_bFastLight)
			pReg->SetValuei("XR_FASTLIGHT", m_bFastLight);
		else
			pReg->DeleteKey("XR_FASTLIGHT");

		if (pReg->GetValuei("XR_WALLMARKS", 1) != m_bWallmarks)
			pReg->SetValuei("XR_WALLMARKS", m_bWallmarks);
		else
			pReg->DeleteKey("XR_WALLMARKS");

		if (pReg->GetValuei("XR_SHADOWDECALS", 1) != m_ShadowDecals)
			pReg->SetValuei("XR_SHADOWDECALS", m_ShadowDecals);
		else
			pReg->DeleteKey("XR_SHADOWDECALS");

		if (pReg->GetValuef("XR_LODOFFSET", 0) != m_LODOffset)
			pReg->SetValuef("XR_LODOFFSET", m_LODOffset);
		else
			pReg->DeleteKey("XR_LODOFFSET");

		if (pReg->GetValuef("XR_LODSCALE", 1.0f) != m_LODScaleBias)
			pReg->SetValuef("XR_LODSCALE", m_LODScaleBias);
		else
			pReg->DeleteKey("XR_LODSCALE");

		if (pReg->GetValuei("XR_SURFOPTIONS", XW_SURFOPTION_HQ0) != m_SurfOptions)
			pReg->SetValuei("XR_SURFOPTIONS", m_SurfOptions);
		else
			pReg->DeleteKey("XR_SURFOPTIONS");

		if (pReg->GetValuei("XR_STENCILSHADOWS", 0) != m_bStencilShadows)
			pReg->SetValuei("XR_STENCILSHADOWS", m_bStencilShadows);
		else
			pReg->DeleteKey("XR_STENCILSHADOWS");

		if (pReg->GetValuei("XR_PORTALTEXTURESIZE", 512) != m_PortalTextureSize)
			pReg->SetValuei("XR_PORTALTEXTURESIZE", m_PortalTextureSize);
		else
			pReg->DeleteKey("XR_PORTALTEXTURESIZE");

		if (pReg->GetValuei("XR_SHADOWDECALTEXTURESIZE", 256) != m_ShadowDecalTextureSize)
			pReg->SetValuei("XR_SHADOWDECALTEXTURESIZE", m_ShadowDecalTextureSize);
		else
			pReg->DeleteKey("XR_SHADOWDECALTEXTURESIZE");

	}
}

void CXR_EngineImpl::Create(int _MaxRecurse, int _CreateFlags)
{
	MAUTOSTRIP(CXR_EngineImpl_Create, MAUTOSTRIP_VOID);
	MSCOPE(Create, XR_ENGINE);
	
	//m_pClient = _pClient;
	m_iCurrentVC = 0;
	m_lspVC.Clear();
	for(int i = 0; i < _MaxRecurse; i++)
	{
		spCXR_ViewContextImpl spVC = MNew(CXR_ViewContextImpl);
		if (!spVC) MemError("Create");
		spVC->Create(this, (i) ? 128 : 1024, i, (i) ? 4 : 8);
		m_lspVC.Add(spVC);
	}

	m_iCurrentVBMDepth = -1;

	// Register
	if (!(_CreateFlags & XR_ENGINECREATE_NOCONSOLE))
		AddToConsole();

	// Get system
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) Error("Create", "No system.");
	m_pSystem = pSys;

	// Get texture context
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("Engine_Render", "No surface-context in system.");
	m_pTC = pTC;

	// Get surface context
	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	if (!pSC) Error("Engine_Render", "No surface-context in system.");
	m_pSC = pSC;

	// Get VB context
	MACRO_GetRegisterObject(CXR_VBContext, pVBC, "SYSTEM.VBCONTEXT");
	if (!pVBC) Error("Engine_Render", "No VB-context in system.");
	m_pVBC = pVBC;

	MACRO_GetSystemEnvironment(pReg);
	if (pReg)
	{
		m_bFlares = pReg->GetValuei("XR_FLARES", 1);
		m_bDynLight = pReg->GetValuei("XR_DLIGHT", 1);
		m_bFastLight = pReg->GetValuei("XR_FASTLIGHT", 0);
		m_bWallmarks = pReg->GetValuei("XR_WALLMARKS", 1);
		m_ShadowDecals = pReg->GetValuei("XR_SHADOWDECALS", 1);
		m_LODOffset = pReg->GetValuef("XR_LODOFFSET", 0);
#ifdef PLATFORM_CONSOLE
		m_LODScaleBias = pReg->GetValuef("XR_LODSCALE", 2.0f);
#else
		m_LODScaleBias = pReg->GetValuef("XR_LODSCALE", 1.0f);
#endif
		m_SurfOptions = pReg->GetValuei("XR_SURFOPTIONS", XW_SURFOPTION_HQ0);
		m_bAllowDepthFog = pReg->GetValuei("XR_ZFOG", 1) != 0;
		m_bAllowVertexFog = pReg->GetValuei("XR_VFOG", 1) != 0;
		m_bAllowNHF = pReg->GetValuei("XR_NHFOG", 1) != 0;
		m_bStencilShadows = pReg->GetValuei("XR_STENCILSHADOWS", 0);
		m_PortalTextureSize = GetLEPow2(Max(1, Min(2048, (int)pReg->GetValuei("XR_PORTALTEXTURESIZE", 512))));
		m_ShadowDecalTextureSize = GetLEPow2(Max(1, Min(2048, (int)pReg->GetValuei("XR_SHADOWDECALTEXTURESIZE", 256))));
		m_bTLEnableEngine = pReg->GetValuei("XR_TLENABLE", 1) != 0;
		m_bGeneratedSurfaces = pReg->GetValuei("XR_GENERATESURFACES", 1) != 0;
//		m_DrawMode = pReg->GetValuei("XR_DRAWMODE", 0);
		m_bClearViewport = pReg->GetValuei("XR_CLEARVIEWPORT", 1);
	}

#ifndef M_RTM
	m_bObjectsOnly	= pSys->GetEnvironment()->GetValuei("XR_OBJECTSONLY", 0) != 0;
	m_bWorldOnly	= pSys->GetEnvironment()->GetValuei("XR_WORLDONLY", 0) != 0;
	m_bShadowDecals	= pSys->GetEnvironment()->GetValuei("XR_SHADOWDECALS", 1) != 0;
#endif
	
	Create_PortalTextureContainer();
	Create_ShadowDecalTextureContainer();
	Create_ScreenTextureContainer();
	Create_ColorCorrectionTextureContainer();

	// Create shader
	{
		m_Shader.Create(this);
		if (!(_CreateFlags & XR_ENGINECREATE_NOCONSOLE))
			m_Shader.Register();
		m_pShader = &m_Shader;
	}

	m_TempLightState.Create(256, 16, 128);
}

void CXR_EngineImpl::OnPrecache()
{
	MAUTOSTRIP(CXR_EngineImpl_OnPrecache, MAUTOSTRIP_VOID);
	for(int iVC = 0; iVC < m_lspVC.Len(); iVC++)
		m_lspVC[iVC]->OnPrecache();

	m_Shader.OnPrecache();

	m_pTC->SetTextureParam(m_pTC->GetTextureID("SPECIAL_WIREGREEN"), CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	m_pTC->SetTextureParam(m_pTC->GetTextureID("SPECIAL_FFFFFF"), CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);

	//JK-NOTE: Removed resident for X06
	m_pTC->SetTextureParam(m_spTCScreen->GetTextureID(0), CTC_TEXTUREPARAM_FLAGS, 0 /*| CTC_TXTIDFLAGS_PRECACHE | CTC_TXTIDFLAGS_RESIDENT*/);
	m_pTC->SetTextureParam(m_spTCScreen->GetTextureID(1), CTC_TEXTUREPARAM_FLAGS, 0 /*| CTC_TXTIDFLAGS_PRECACHE | CTC_TXTIDFLAGS_RESIDENT*/);
	m_pTC->SetTextureParam(m_spTCScreen->GetTextureID(2), CTC_TEXTUREPARAM_FLAGS, 0 /*| CTC_TXTIDFLAGS_PRECACHE | CTC_TXTIDFLAGS_RESIDENT*/);
	m_pTC->SetTextureParam(m_spTCScreen->GetTextureID(3), CTC_TEXTUREPARAM_FLAGS, 0 /*| CTC_TXTIDFLAGS_PRECACHE | CTC_TXTIDFLAGS_RESIDENT*/);
	m_pTC->SetTextureParam(m_spTCScreen->GetTextureID(4), CTC_TEXTUREPARAM_FLAGS, 0 /*| CTC_TXTIDFLAGS_PRECACHE | CTC_TXTIDFLAGS_RESIDENT*/);
	m_pTC->SetTextureParam(m_spTCScreen->GetTextureID(5), CTC_TEXTUREPARAM_FLAGS, 0 /*| CTC_TXTIDFLAGS_PRECACHE | CTC_TXTIDFLAGS_RESIDENT*/);
	m_pTC->SetTextureParam(m_spTCScreen->GetTextureID(6), CTC_TEXTUREPARAM_FLAGS, 0 /*| CTC_TXTIDFLAGS_PRECACHE | CTC_TXTIDFLAGS_RESIDENT*/);
}

void CXR_EngineImpl::Create_PortalTextureContainer()
{
	MAUTOSTRIP(CXR_EngineImpl_Create_PortalTextureContainer, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_EngineImpl::Create_PortalTextureContainer);

	m_spTCPortals = MNew(CTextureContainer_EnginePortals);
	if (!m_spTCPortals) MemError("Create");
	m_spTCPortals->Create(8, this);
	m_iNextPortalTexture = 0;

	m_spTCPortals->PrepareFrame(CPnt(m_PortalTextureSize, m_PortalTextureSize));
}

void CXR_EngineImpl::SetDebugFont(spCRC_Font _spFont)
{
	MAUTOSTRIP(CXR_EngineImpl_SetDebugFont, MAUTOSTRIP_VOID);
	m_spDebugFont = _spFont;
}

void CXR_EngineImpl::Create_ShadowDecalTextureContainer()
{
	MAUTOSTRIP(CXR_EngineImpl_Create_ShadowDecalTextureContainer, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_EngineImpl::Create_ShadowDecalTextureContainer);

#ifdef XR_SUPPORT_SHADOWDECALS
	m_spTCShadowDecals = MNew(CTextureContainer_ShadowDecals);
	if (!m_spTCShadowDecals) MemError("Create");
	m_spTCShadowDecals->Create(8, this);

	m_spTCShadowDecals->PrepareFrame(CPnt(m_ShadowDecalTextureSize, m_ShadowDecalTextureSize));
#endif

}

void CXR_EngineImpl::Create_ScreenTextureContainer()
{
	MAUTOSTRIP(CXR_EngineImpl_Create_ScreenTextureContainer, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_EngineImpl::Create_ScreenTextureContainer);

	{
		m_spTCScreen = MNew(CTextureContainer_Screen);
		if (!m_spTCScreen) MemError("Create");
		m_spTCScreen->Create(8, this);

		m_spTCScreen->SetTextureFormat(0, IMAGE_FORMAT_BGRA8);		// Screen (BackBufferFormat)
		m_spTCScreen->SetTextureFormat(1, IMAGE_FORMAT_ZBUFFER);	// Depth
		m_spTCScreen->SetTextureFormat(2, IMAGE_FORMAT_BGRA8);		// Last screen (BackBufferFormat)
#ifdef PLATFORM_XENON
		m_spTCScreen->SetTextureFormat(3, IMAGE_FORMAT_BGR10A2);		// Deferred normal
#else
		m_spTCScreen->SetTextureFormat(3, IMAGE_FORMAT_BGRA8);		// Deferred normal
#endif
		m_spTCScreen->SetTextureFormat(4, IMAGE_FORMAT_BGRA8);		// Deferred diffuse
		m_spTCScreen->SetTextureFormat(5, IMAGE_FORMAT_BGRA8);		// Deferred specular
		m_spTCScreen->SetTextureFormat(6, IMAGE_FORMAT_BGRA8);		// Postprocessing buffer (BackBufferFormat)
		m_spTCScreen->SetTextureFormat(7, IMAGE_FORMAT_BGRA8);		// Motion map

		CTC_TextureProperties Prop;
		m_spTCScreen->GetTextureProperties(0, Prop);
		Prop.m_Flags |= CTC_TEXTUREFLAGS_RENDERUSEBACKBUFFERFORMAT; // | CTC_TEXTUREFLAGS_DOUBLEBUFFER | CTC_TEXTUREFLAGS_RENDEROLDBUFFER;
		m_spTCScreen->SetTextureProperties(0, Prop);
		m_spTCScreen->SetTextureProperties(6, Prop);

		m_spTCScreen->GetTextureProperties(2, Prop);
		Prop.m_Flags |= CTC_TEXTUREFLAGS_CONTINUETILING | CTC_TEXTUREFLAGS_RENDERUSEBACKBUFFERFORMAT; // | CTC_TEXTUREFLAGS_DOUBLEBUFFER | CTC_TEXTUREFLAGS_RENDEROLDBUFFER;
		m_spTCScreen->SetTextureProperties(2, Prop);

		m_spTCScreen->GetTextureProperties(1, Prop);
		Prop.m_Flags |= CTC_TEXTUREFLAGS_CONTINUETILING;
		Prop.m_MagFilter = CTC_MAGFILTER_NEAREST;
		Prop.m_MinFilter = CTC_MINFILTER_NEAREST;
		Prop.m_MIPFilter = CTC_MIPFILTER_NEAREST;
		m_spTCScreen->SetTextureProperties(1, Prop);
		Prop.m_Flags |= CTC_TEXTUREFLAGS_CLEARWHENCONTINUETILING | CTC_TEXTUREFLAGS_RENDERTARGET; // | CTC_TEXTUREFLAGS_DOUBLEBUFFER | CTC_TEXTUREFLAGS_RENDEROLDBUFFER;
		m_spTCScreen->SetTextureProperties(3, Prop);
		Prop.m_MagFilter = CTC_MAGFILTER_LINEAR;
		Prop.m_MinFilter = CTC_MINFILTER_LINEAR;
		Prop.m_MIPFilter = CTC_MIPFILTER_NEAREST;
		m_spTCScreen->SetTextureProperties(4, Prop);
		m_spTCScreen->SetTextureProperties(5, Prop);

	}

	// ----------------------------------------------------------------
	{
		m_spTCDepthStencil = MNew(CTextureContainer_Screen);
		if (!m_spTCDepthStencil) MemError("Create");
		m_spTCDepthStencil->Create(2, this);

		CTC_TextureProperties Prop;
		m_spTCDepthStencil->GetTextureProperties(0, Prop);
		Prop.m_MagFilter = CTC_MAGFILTER_NEAREST;
		Prop.m_MagFilter = CTC_MINFILTER_NEAREST;
		Prop.m_MIPFilter = CTC_MIPFILTER_NEAREST;
		m_spTCDepthStencil->SetTextureProperties(0, Prop);
	}
}

void CXR_EngineImpl::Create_ColorCorrectionTextureContainer()
{
	MSCOPESHORT(CXR_EngineImpl::Create_ColorCorrectionTextureContainer);

	{
		m_spTCColorCorrection = MNew(CTextureContainer_Screen);
		if (!m_spTCColorCorrection) MemError("Create_ColorCorrectionTextureContainer");
		CTextureContainer_Screen* pTC = m_spTCColorCorrection;

		const int nCubes = 4;
		pTC->Create(nCubes, this);

		CTC_TextureProperties Prop;
		m_spTCScreen->GetTextureProperties(0, Prop);
		Prop.m_Flags |= CTC_TEXTUREFLAGS_RENDERUSEBACKBUFFERFORMAT | CTC_TEXTUREFLAGS_CUBEMAP;

		for(int iCube = 0; iCube < nCubes; iCube++)
		{
			pTC->SetTextureFormat(iCube, IMAGE_FORMAT_BGRA8);
			pTC->SetTextureProperties(iCube, Prop);
		}
		pTC->PrepareFrame(CPnt(128, 128));
	}

	{
		m_spTCColorCorrectionTemp = MNew(CTextureContainer_Screen);
		if (!m_spTCColorCorrectionTemp) MemError("Create_ColorCorrectionTextureContainer");
		CTextureContainer_Screen* pTC = m_spTCColorCorrectionTemp;

		pTC->Create(1, this);

		CTC_TextureProperties Prop;
		m_spTCScreen->GetTextureProperties(0, Prop);
		Prop.m_Flags |= CTC_TEXTUREFLAGS_RENDERUSEBACKBUFFERFORMAT;

		pTC->SetTextureFormat(0, IMAGE_FORMAT_BGRA8);
		pTC->SetTextureProperties(0, Prop);
		pTC->PrepareFrame(CPnt(256,256));
	}
}

void CXR_EngineImpl::RenderModel(CXR_VCModelInstance* _pObjInfo, CXR_ViewClipInterface* _pViewClip, int _Flags)
{
	MAUTOSTRIP(CXR_EngineImpl_RenderModel, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_EngineImpl::RenderModel); //AR-SCOPE

	if (!_pObjInfo->m_pModel) return;
	CXR_ViewContextImpl* pVC = m_lspVC[m_iCurrentVC];
#ifdef M_Profile
	if (m_ShowTiming)
		_pObjInfo->m_Time.Start();
#endif
//	_pObjInfo->m_pModel->OnRender(this, m_pRender, GetVBM(), _pViewClip, pVC->m_spLightState, &_pObjInfo->m_Anim, _pObjInfo->m_Pos, pVC->m_W2VMat, _Flags | _pObjInfo->m_OnRenderFlags);
	_pObjInfo->m_pModel->OnRender2(this, m_pRender, GetVBM(), _pViewClip, pVC->m_spLightState, &_pObjInfo->m_Anim, _pObjInfo->m_Pos, pVC->m_W2VMat, _pObjInfo->m_Velocity, _Flags | _pObjInfo->m_OnRenderFlags);
#ifdef M_Profile
	if (m_ShowTiming)
		_pObjInfo->m_Time.Stop();
#endif
}


#ifndef	CPU_SOFTWARE_FP64
static int GetWindingFromPlane(const CPlane3Dfp64& _Plane, CVec3Dfp64* _pV)
{
	MAUTOSTRIP(GetWindingFromPlane, 0);
	CVec3Dfp64 v;
	if (M_Fabs(_Plane.n.k[0]) > 1-0.01)
	{
		v.k[0] = 0;
		v.k[1] = 0;
		v.k[2] = 1;
	}
	else
	{
		v.k[0] = 1;
		v.k[1] = 0;
		v.k[2] = 0;
	}

	CVec3Dfp64 dv0, dv1, p;
	v.CrossProd(_Plane.n, dv0);
	dv0.Normalize();
	dv0.CrossProd(_Plane.n, dv1);
	p = _Plane.GetPointInPlane();

	dv0 *= 100000.0;
	dv1 *= 100000.0;

	_pV[0] = p + dv0;
	_pV[1] = p + dv1;
	_pV[2] = p - dv0;
	_pV[3] = p - dv1;
	return 4;
}
#else
static int GetWindingFromPlane(const CPlane3Dfp32& _Plane, CVec3Dfp32* _pV)
{
	MAUTOSTRIP(GetWindingFromPlane, 0);
	CVec3Dfp32 v;
	if (M_Fabs(_Plane.n.k[0]) > 1-0.01f)
	{
		v.k[0] = 0;
		v.k[1] = 0;
		v.k[2] = 1;
	}
	else
	{
		v.k[0] = 1;
		v.k[1] = 0;
		v.k[2] = 0;
	}

	CVec3Dfp32 dv0, dv1, p;
	v.CrossProd(_Plane.n, dv0);
	dv0.Normalize();
	dv0.CrossProd(_Plane.n, dv1);
	p = _Plane.GetPointInPlane();

	dv0 *= 100000.0f;
	dv1 *= 100000.0f;

	_pV[0] = p + dv0;
	_pV[1] = p + dv1;
	_pV[2] = p - dv0;
	_pV[3] = p - dv1;
	return 4;
}
#endif


static int CreateConvexPolygon(const CPlane3Dfp32& _Plane, CVec3Dfp32* _pV, int _nV, int _MaxV)
{
	MAUTOSTRIP(CreateConvexPolygon, 0);
#ifndef	CPU_SOFTWARE_FP64
	// Creates a convex polygon from an arbitrary bunch of vertices.
	CVec3Dfp64 VSrc[64];

	CVec3Dfp64 N = _Plane.n.Getfp64();
	for(int i = 0; i < _nV; i++)
		VSrc[i] = _pV[i].Getfp64();

	CPlane3Dfp64 Plane;
	Plane.n = _Plane.n.Getfp64();
	Plane.d = _Plane.d;

	CVec3Dfp64 V[64];
	int nV = GetWindingFromPlane(Plane, V);

	CWireContainer* pWC = NULL;

//	if (m_bShowPortalFence && !GetVCDepth())
	{
		MACRO_GetRegisterObject(CWireContainer, pWCTmp, "GAMECONTEXT.CLIENT.WIRECONTAINER");
		pWC = pWCTmp;
	}


	// Find a vertex that creates an edge plane with all other vertices on it's back-side
	for(int v = 0; v < _nV-1; v++)
	{
		for(int v2 = v+1; v2 < _nV; v2++)
		{
			if (VSrc[v].DistanceSqr(VSrc[v2]) < Sqr(0.1f)) continue;

			CPlane3Dfp64 P;
			P.Create(VSrc[v], VSrc[v2], VSrc[v2] + N);
			if (P.n.LengthSqr() < Sqr(0.1f)) continue;

			int Mask = P.GetArrayPlaneSideMask_Epsilon(VSrc, _nV, 0.01f) & 3;


			if (pWC && (Mask == 2 || Mask == 1))
			{
//				for(int i = 0; i < nVerts; i++)
				{
					CVec3Dfp64 V;
					VSrc[v2].Sub(VSrc[v], V);
					V.Normalize();
					V *= 64.0f;
					pWC->RenderWire((VSrc[v] - V).Getfp32(), (VSrc[v2] + V).Getfp32(), 0xff002000, 0.1f);
				}
			}

			int bClip;
			if (Mask == 2)
			{
				nV = CutFence(V, nV, &P, 1, bClip);
//				nV = RemoveColinearPoints(V, NULL, nV);
			}
			else if (Mask == 1)
			{
				P.Inverse();
				nV = CutFence(V, nV, &P, 1, bClip);
//				nV = RemoveColinearPoints(V, NULL, nV);
			}
		}

		if (nV >= 254) break;
	}

	if (pWC)
	{
//				for(int i = 0; i < nVerts; i++)

		for(int i = 0; i < nV; i++)
			pWC->RenderWire(V[i].Getfp32(), V[(i+1)%nV].Getfp32(), 0xff000060, 0.1f);
	}


//	CVec3Dfp64 V2[256];
//	nV = RemoveColinearPoints(V, V2, nV);

	if (nV > _MaxV)
	{
		ConOut("§cf80WARNING: (::CreateConvexPolygon) Polygon truncated.");
		nV = _MaxV;
	}

	{
		for(int i = 0; i < nV; i++)
			_pV[i] = V[i].Getfp32();

//		memcpy(_pV, V, nV*sizeof(CVec3Dfp32));
	}

#else	// CPU_SOFTWARE_FP64
	// Creates a convex polygon from an arbitrary bunch of vertices.
	CVec3Dfp32 VSrc[64];

	CVec3Dfp32 N = _Plane.n.Getfp32();
	for(int i = 0; i < _nV; i++)
		VSrc[i] = _pV[i].Getfp32();

	CPlane3Dfp32 Plane;
	Plane.n = _Plane.n.Getfp32();
	Plane.d = _Plane.d;

	CVec3Dfp32 V[64];
	int nV = GetWindingFromPlane(Plane, V);

	CWireContainer* pWC = NULL;

//	if (m_bShowPortalFence && !GetVCDepth())
	{
		MACRO_GetRegisterObject(CWireContainer, pWCTmp, "GAMECONTEXT.CLIENT.WIRECONTAINER");
		pWC = pWCTmp;
	}


	// Find a vertex that creates an edge plane with all other vertices on it's back-side
	for(int v = 0; v < _nV-1; v++)
	{
		for(int v2 = v+1; v2 < _nV; v2++)
		{
			if (VSrc[v].DistanceSqr(VSrc[v2]) < Sqr(0.1f)) continue;

			CPlane3Dfp32 P;
			P.Create(VSrc[v], VSrc[v2], VSrc[v2] + N);
			if (P.n.LengthSqr() < Sqr(0.1f)) continue;

			int Mask = P.GetArrayPlaneSideMask_Epsilon(VSrc, _nV, 0.01f) & 3;


			if (pWC && (Mask == 2 || Mask == 1))
			{
//				for(int i = 0; i < nVerts; i++)
				{
					CVec3Dfp32 V;
					VSrc[v2].Sub(VSrc[v], V);
					V.Normalize();
					V *= 64.0;
					pWC->RenderWire((VSrc[v] - V).Getfp32(), (VSrc[v2] + V).Getfp32(), 0xff002000, 0.1f);
				}
			}

			int bClip;
			if (Mask == 2)
			{
				nV = CutFence(V, nV, &P, 1, bClip);
//				nV = RemoveColinearPoints(V, NULL, nV);
			}
			else if (Mask == 1)
			{
				P.Inverse();
				nV = CutFence(V, nV, &P, 1, bClip);
//				nV = RemoveColinearPoints(V, NULL, nV);
			}
		}

		if (nV >= 254) break;
	}

	if (pWC)
	{
//				for(int i = 0; i < nVerts; i++)

		for(int i = 0; i < nV; i++)
			pWC->RenderWire(V[i].Getfp32(), V[(i+1)%nV].Getfp32(), 0xff000060, 0.1f);
	}


//	CVec3Dfp32 V2[256];
//	nV = RemoveColinearPoints(V, V2, nV);

	if (nV > _MaxV)
	{
		ConOut("§cf80WARNING: (::CreateConvexPolygon) Polygon truncated.");
		nV = _MaxV;
	}

	{
		for(int i = 0; i < nV; i++)
			_pV[i] = V[i].Getfp32();

//		memcpy(_pV, V, nV*sizeof(CVec3Dfp32));
	}

#endif	// CPU_SOFTWARE_FP64
	return nV;
}


int CXR_EngineImpl::Render_AddPortal(const CVec3Dfp32* _pPortal, int _nVertices, const CMat4Dfp32& _dWCam, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat,
	CPixel32 _SurfColor, CXR_PortalSurfaceRender* _pSurfaceRender, void* _pSurfContext)
{
	MAUTOSTRIP(CXR_EngineImpl_Render_AddPortal, 0);
	M_LOCK(m_RenderAddLock);
#ifndef	CPU_SOFTWARE_FP64
	if (m_iCurrentVC+1 >= m_lspVC.Len()) return -1;
	CXR_ViewContextImpl* pVC = m_lspVC[m_iCurrentVC];
	if (!pVC->m_pViewClip) return -1;
	if (pVC->m_nPortals >= pVC->m_lPortals.Len()) return -1;

	bool bDebug = (m_bShowPortalFence & 2) != 0;

	CVec3Dfp64 PortalSrc[32];
	{ for(int v = 0; v < _nVertices; v++) _pPortal[v].Assignfp64(PortalSrc[v]); };

	CVec3Dfp64 PortalFace[32];
	CVec3Dfp32 PortalFace4[32];

	// Copy and clean-up input portal-fence.
	int nVerts = RemoveColinearPoints(PortalSrc, PortalFace, _nVertices);
	if (bDebug) ConOut(CStrF("    (1) nVerts %d -> %d", _nVertices, nVerts));

	if (nVerts < 3) return -1;
	CMat4Dfp32 WCam;
	pVC->m_CameraWMat.Multiply(_dWCam, WCam);

	// Calc normal
	CVec3Dfp64 e0, e1, n;
	PortalFace[2].Sub(PortalFace[1], e1);
	PortalFace[1].Sub(PortalFace[0], e0);
	e1.CrossProd(e0, n);
	if (n.LengthSqr() < 0.001f) return -1;	// Nonsense normal, abort.
//		Error("Render_AddPortal", "Can't calculate normal.");
	n.Normalize();

	int bClip;
	// Cut to view-context volume
	nVerts = CutFence(PortalFace, nVerts, pVC->m_ClipPlanesW8, pVC->m_ClipW.m_nPlanes, bClip);
	if (bDebug) ConOut(CStrF("    (2) nVerts %d -> %d", _nVertices, nVerts));

	nVerts = RemoveColinearPoints(PortalFace, NULL, nVerts);

	// Cut to view-context front-plane
	nVerts = CutFence(PortalFace, nVerts, &pVC->m_FrontPlaneW8, 1, bClip);
	if (bDebug) ConOut(CStrF("    (3) nVerts %d -> %d", _nVertices, nVerts));

	nVerts = RemoveColinearPoints(PortalFace, NULL, nVerts);

	// Check if portal is on view-context front-plane
	if (pVC->m_FrontPlaneW8.GetArrayPlaneSideMask_Epsilon(PortalFace, nVerts, 1.0f) == 4) return -1;

	CXR_Portal* pPortal = &pVC->m_lPortals[pVC->m_nPortals];
	pPortal->Clear();

	// Create portal-plane
	pPortal->m_Plane.CreateNV(n.Getfp32(), PortalFace[0].Getfp32());

	// Check camera distance to portal-plane
	if (pPortal->m_Plane.Distance(CVec3Dfp32::GetMatrixRow(pVC->m_CameraWMat, 3)) < 0.0f) return -1;

	if (bDebug) ConOut(CStr("    (4) Progress"));

	if (nVerts < 3) return -1;
	if (bDebug) ConOut(CStr("    (5) Progress"));
	if (nVerts > CRC_CLIPVOLUME_MAXPLANES-1) nVerts = CRC_CLIPVOLUME_MAXPLANES-1;
	if (bDebug) ConOut(CStr("    (6) Progress"));

	CVec3Dfp64 VMid(0);
	{ 
		for(int v = 0; v < nVerts; v++)
		{
			VMid += PortalFace[v]; 
			PortalFace[v].Assignfp32(PortalFace4[v]);
		}
	}
	VMid *= 1.0f/nVerts;

	pPortal->m_Portal.Init(0);
	pPortal->m_Portal.GetVertices(&PortalFace4[0], nVerts, CVec3Dfp32(CVec3Dfp32::GetMatrixRow(pVC->m_CameraWMat, 3)), false /*!MACRO_ISMIRRORED(_WCam)*/);

	pPortal->m_PortalDest.CopyAndTransform(pPortal->m_Portal, _dWCam);
	pPortal->m_VisPos = 0;
	{ for(int v = 0; v < nVerts; v++)
		pPortal->m_VisPos += pPortal->m_PortalDest.m_Vertices[v]; }
	pPortal->m_VisPos *= 1.0f / fp32(nVerts);

	pPortal->m_PlaneDest = pPortal->m_Plane;
	pPortal->m_PlaneDest.Transform(_dWCam);
	pPortal->m_SurfColor = _SurfColor;
	pPortal->m_pSurfRender = _pSurfaceRender;
	pPortal->m_pSurfContext = _pSurfContext;
	pPortal->m_WCam = WCam;
	pPortal->m_dCam = _dWCam;
	pPortal->m_SortZ = (VMid.Getfp32() - CVec3Dfp32::GetMatrixRow(pVC->m_CameraWMat, 3)).LengthSqr();
	pPortal->m_ModelWMat	= _WMat;
	pPortal->m_ModelVMat	= _VMat;

	pVC->m_nPortals++;
	if (bDebug) ConOut(CStrF("    (7) Complete, iPortal %d, IsMirr %d, CamFwd %s", pVC->m_nPortals-1, MACRO_ISMIRRORED(pPortal->m_WCam), (const char*)CVec3Dfp32::GetRow(pPortal->m_WCam, 2).GetString()) );

	// DEBUG: Render all portals from recursion level 0
/*	if (m_bShowPortalFence && !GetVCDepth())
	{
		MACRO_GetRegisterObject(CWireContainer, pWC, "GAMECONTEXT.CLIENT.WIRECONTAINER");
		if (pWC)
		{
			for(int i = 0; i < nVerts; i++)
			{
				pWC->RenderWire(PortalFace4[i], PortalFace4[(i + 1) % nVerts], 0xff002000);
			}
		}
	}*/


#else	// CPU_SOFTWARE_FP64
	if (m_iCurrentVC+1 >= m_lspVC.Len()) return -1;
	CXR_ViewContextImpl* pVC = m_lspVC[m_iCurrentVC];
	if (!pVC->m_pViewClip) return -1;
	if (pVC->m_nPortals >= pVC->m_lPortals.Len()) return -1;

	bool bDebug = (m_bShowPortalFence & 2) != 0;

	CVec3Dfp32 PortalSrc[32];
	{ for(int v = 0; v < _nVertices; v++) _pPortal[v].Assignfp32(PortalSrc[v]); };

	CVec3Dfp32 PortalFace[32];
	CVec3Dfp32 PortalFace4[32];

	// Copy and clean-up input portal-fence.
	int nVerts = RemoveColinearPoints(PortalSrc, PortalFace, _nVertices);
	if (bDebug) ConOut(CStrF("    (1) nVerts %d -> %d", _nVertices, nVerts));

	if (nVerts < 3) return -1;
	CMat4Dfp32 WCam;
	pVC->m_CameraWMat.Multiply(_dWCam, WCam);

	// Calc normal
	CVec3Dfp32 e0, e1, n;
	PortalFace[2].Sub(PortalFace[1], e1);
	PortalFace[1].Sub(PortalFace[0], e0);
	e1.CrossProd(e0, n);
	if (n.LengthSqr() < 0.001f) return -1;	// Nonsense normal, abort.
//		Error("Render_AddPortal", "Can't calculate normal.");
	n.Normalize();

	int bClip;
	// Cut to view-context volume
	nVerts = CutFence(PortalFace, nVerts, pVC->m_ClipW.m_Planes, pVC->m_ClipW.m_nPlanes, bClip);
	if (bDebug) ConOut(CStrF("    (2) nVerts %d -> %d", _nVertices, nVerts));

	nVerts = RemoveColinearPoints(PortalFace, NULL, nVerts);

	// Cut to view-context front-plane
	nVerts = CutFence(PortalFace, nVerts, &pVC->m_FrontPlaneW, 1, bClip);
	if (bDebug) ConOut(CStrF("    (3) nVerts %d -> %d", _nVertices, nVerts));

	nVerts = RemoveColinearPoints(PortalFace, NULL, nVerts);

	// Check if portal is on view-context front-plane
	if (pVC->m_FrontPlaneW.GetArrayPlaneSideMask_Epsilon(PortalFace, nVerts, 1.0f) == 4) return -1;

	CXR_Portal* pPortal = &pVC->m_lPortals[pVC->m_nPortals];
	pPortal->Clear();

	// Create portal-plane
	pPortal->m_Plane.CreateNV(n.Getfp32(), PortalFace[0].Getfp32());

	// Check camera distance to portal-plane
	if (pPortal->m_Plane.Distance(CVec3Dfp32::GetMatrixRow(pVC->m_CameraWMat, 3)) < 0.0f) return -1;

	if (bDebug) ConOut(CStr("    (4) Progress"));

	if (nVerts < 3) return -1;
	if (bDebug) ConOut(CStr("    (5) Progress"));
	if (nVerts > CRC_CLIPVOLUME_MAXPLANES-1) nVerts = CRC_CLIPVOLUME_MAXPLANES-1;
	if (bDebug) ConOut(CStr("    (6) Progress"));

	CVec3Dfp32 VMid(0);
	{ 
		for(int v = 0; v < nVerts; v++)
		{
			VMid += PortalFace[v]; 
			PortalFace[v].Assignfp32(PortalFace4[v]);
		}
	}
	VMid *= 1.0f/nVerts;

	pPortal->m_Portal.Init(0);
	pPortal->m_Portal.GetVertices(&PortalFace4[0], nVerts, CVec3Dfp32(CVec3Dfp32::GetMatrixRow(pVC->m_CameraWMat, 3)), false /*!MACRO_ISMIRRORED(_WCam)*/);

	pPortal->m_PortalDest.CopyAndTransform(pPortal->m_Portal, _dWCam);
	pPortal->m_VisPos = 0;
	{ for(int v = 0; v < nVerts; v++)
		pPortal->m_VisPos += pPortal->m_PortalDest.m_Vertices[v]; }
	pPortal->m_VisPos *= 1.0f / fp32(nVerts);

	pPortal->m_PlaneDest = pPortal->m_Plane;
	pPortal->m_PlaneDest.Transform(_dWCam);
	pPortal->m_SurfColor = _SurfColor;
	pPortal->m_pSurfRender = _pSurfaceRender;
	pPortal->m_pSurfContext = _pSurfContext;
	pPortal->m_WCam = WCam;
	pPortal->m_dCam = _dWCam;
	pPortal->m_SortZ = (VMid.Getfp32() - CVec3Dfp32::GetMatrixRow(pVC->m_CameraWMat, 3)).LengthSqr();
	pPortal->m_ModelWMat	= _WMat;
	pPortal->m_ModelVMat	= _VMat;

	pVC->m_nPortals++;
	if (bDebug) ConOut(CStrF("    (7) Complete, iPortal %d, IsMirr %d, CamFwd %s", pVC->m_nPortals-1, MACRO_ISMIRRORED(pPortal->m_WCam), (const char*)CVec3Dfp32::GetRow(pPortal->m_WCam, 2).GetString()) );

	// DEBUG: Render all portals from recursion level 0
/*	if (m_bShowPortalFence && !GetVCDepth())
	{
		MACRO_GetRegisterObject(CWireContainer, pWC, "GAMECONTEXT.CLIENT.WIRECONTAINER");
		if (pWC)
		{
			for(int i = 0; i < nVerts; i++)
			{
				pWC->RenderWire(PortalFace4[i], PortalFace4[(i + 1) % nVerts], 0xff002000);
			}
		}
	}*/


#endif	// CPU_SOFTWARE_FP64
	return pVC->m_nPortals-1;
}

int CXR_EngineImpl::Render_AddMirror(const CVec3Dfp32* _pPortal, int _nVertices, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat,
	CPixel32 _SurfColor, CXR_PortalSurfaceRender* _pSurfaceRender, void* _pSurfContext)
{
	MAUTOSTRIP(CXR_EngineImpl_Render_AddMirror, 0);
	if (_nVertices < 3) return -1;

	M_LOCK(m_RenderAddLock);
	CXR_ViewContext* pVC = GetVC();

	CVec3Dfp32 Portal[32];

	int nV = RemoveColinearPoints(const_cast<CVec3Dfp32*>(_pPortal), Portal, _nVertices);

	CPlane3Dfp32 Plane;
	Plane.Create(Portal[0], Portal[2], Portal[1]);

	CMat4Dfp32 WRef;
	Plane.ReflectMatrix(pVC->m_CameraWMat, WRef); //CXR_ViewContext::ReflectMatrix(Plane, pVC->m_CameraWMat, WRef);

	CMat4Dfp32 dCam, SrcInv;
	pVC->m_CameraWMat.InverseOrthogonal(SrcInv);
	SrcInv.Multiply(WRef, dCam);

	return Render_AddPortal(Portal, nV, dCam, _WMat, _VMat, _SurfColor, _pSurfaceRender, _pSurfContext);
}

int CXR_EngineImpl::Render_AddPortal_Texture(const CVec3Dfp32* _pPortal, int _nVertices, const CMat4Dfp32& _WCam, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
{
	MAUTOSTRIP(CXR_EngineImpl_Render_AddPortal_Texture, 0);
//ConOut(CStr("(Render_AddPortal_Texture) ..."));
	M_LOCK(m_RenderAddLock);
	if (m_bNoRenderToTexture) return -1;
	if (m_iNextPortalTexture >= m_spTCPortals->GetNumLocal()) return -1;

	int iP = Render_AddPortal(_pPortal, _nVertices, _WCam, _WMat, _VMat);
	if (iP < 0) return -1;

	CXR_ViewContext* pVC = m_lspVC[m_iCurrentVC];
	CXR_Portal* pPortal = &pVC->m_lPortals[iP];

	// Look for identical portal
	int p = 0;
	for(; p < iP; p++)
	{
		CXR_Portal* pP = &pVC->m_lPortals[p];
		if (pP->IsTexture())
		{
			if ((pP->m_Plane.n.Distance(pPortal->m_Plane.n) < 0.001f) &&
				(M_Fabs(pP->m_Plane.d - pPortal->m_Plane.d) < 0.001f) &&
				(pP->m_PlaneDest.n.Distance(pPortal->m_PlaneDest.n) < 0.001f) &&
				(M_Fabs(pP->m_PlaneDest.d - pPortal->m_PlaneDest.d) < 0.001f) &&
				(M_Fabs(pP->m_WCam.k[3][0] - pPortal->m_WCam.k[3][0]) < 0.001f) &&
				(M_Fabs(pP->m_WCam.k[3][1] - pPortal->m_WCam.k[3][1]) < 0.001f) &&
				(M_Fabs(pP->m_WCam.k[3][2] - pPortal->m_WCam.k[3][2]) < 0.001f))
			{
				break;
			}
		}
	}
	//JK-FIX: 2005-08-18, Not sure if this is valid since portals can have diffrent matrices now...
	if (p < iP)
	{
		// Found match
		CXR_Portal* pPNew = &pVC->m_lPortals[p];

		// Combine portals into one convex portal
		CVec3Dfp32 Verts[64];
		memcpy(Verts, pPNew->m_Portal.m_Vertices, pPNew->m_Portal.m_nPlanes * sizeof(CVec3Dfp32));
		memcpy(&Verts[pPNew->m_Portal.m_nPlanes], pPortal->m_Portal.m_Vertices, pPortal->m_Portal.m_nPlanes * sizeof(CVec3Dfp32));

		int nV = pPNew->m_Portal.m_nPlanes + pPortal->m_Portal.m_nPlanes;
		nV = ::CreateConvexPolygon(pPNew->m_Plane, Verts, nV, CRC_CLIPVOLUME_MAXPLANES);

		iP = p;
		pPortal = &pVC->m_lPortals[iP];

		pPortal->m_Portal.Init(0);
		pPortal->m_Portal.GetVertices(&Verts[0], nV, CVec3Dfp32(CVec3Dfp32::GetMatrixRow(pVC->m_CameraWMat, 3)), false /*!MACRO_ISMIRRORED(_WCam)*/);
		pPortal->m_PortalDest.CopyAndTransform(pPortal->m_Portal, pPortal->m_dCam);

		// Remove the created portal.
		pVC->m_nPortals--;

//ConOut(CStrF("(Render_AddPortal_Texture) Recycling portal %d, TxtID %d, iLocal %d", iP, pPortal->m_TextureID, pPortal->m_iLocalTexture));
	}
	else
	{
		pPortal->m_iLocalTexture = m_iNextPortalTexture;
		pPortal->m_TextureID = m_spTCPortals->GetTextureID(pPortal->m_iLocalTexture);
		pPortal->m_SortZ = -1000000000.0f; // To ensure that all render-to-texture portals are processed before all ordinary portals.
		m_iNextPortalTexture++;
//ConOut(CStrF("(Render_AddPortal_Texture) TxtID %d, iLocal %d", pPortal->m_TextureID, pPortal->m_iLocalTexture));
	}
	return iP;
}

int CXR_EngineImpl::Render_AddMirror_Texture(const CVec3Dfp32* _pPortal, int _nVertices, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
{
	MAUTOSTRIP(CXR_EngineImpl_Render_AddMirror_Texture, 0);
	if (_nVertices < 3) return -1;
	M_LOCK(m_RenderAddLock);
	if (m_bNoRenderToTexture) return -1;
	CXR_ViewContext* pVC = GetVC();

	CVec3Dfp32 Portal[32];

	int nV = RemoveColinearPoints(const_cast<CVec3Dfp32*>(_pPortal), Portal, _nVertices);

	CPlane3Dfp32 Plane;
	Plane.Create(Portal[0], Portal[2], Portal[1]);

	CMat4Dfp32 WRef;
	Plane.ReflectMatrix(pVC->m_CameraWMat, WRef); //CXR_ViewContext::ReflectMatrix(Plane, pVC->m_CameraWMat, WRef);

	CMat4Dfp32 dCam, SrcInv;
	pVC->m_CameraWMat.InverseOrthogonal(SrcInv);
	SrcInv.Multiply(WRef, dCam);

	return Render_AddPortal_Texture(Portal, nV, dCam, _WMat, _VMat);
}

bool CXR_EngineImpl::Render_AddModel(CXR_Model* _pModel, const CMat4Dfp32& _Pos, const CXR_AnimState& _Anim, int _ModelType, int _OnRenderFlags)
{
	MAUTOSTRIP(CXR_EngineImpl_Render_AddModel, MAUTOSTRIP_VOID);
	if (!_pModel) return false;
	
	M_LOCK(m_RenderAddLock);
	switch(_ModelType)
	{
	case XR_MODEL_STANDARD :
		{
/*
			if(!(_OnRenderFlags & CXR_MODEL_ONRENDERFLAGS_NOCULL))
			{
				CBox3Dfp32 BoxLocal, BoxWorld;
				_pModel->GetBound_Box(BoxLocal, &_Anim);
				BoxLocal.Transform(_Pos, BoxWorld);

				CXR_ViewContext* pVC = m_lspVC[m_iCurrentVC];
				CRC_ClipVolume* pClip = &pVC->m_ClipW;

				for(int p = 0; p < pClip->m_nPlanes; p++)
				{
					fp32 d = pClip->m_Planes[p].GetBoxMinDistance(BoxWorld.m_Min, BoxWorld.m_Max);
					if (d > 0.0f) return false;
				}
				if (pVC->m_BackPlaneW.GetBoxMinDistance(BoxWorld.m_Min, BoxWorld.m_Max) > 0.0f) return false;
			}
*/

/*			fp32 BoundR = _pModel->GetBound_Sphere(&_Anim);
			CXR_ViewContext* pVC = m_lspVC[m_iCurrentVC];
			CRC_ClipVolume* pClip = &pVC->m_ClipW;

			for(int p = 0; p < pClip->m_nPlanes; p++)
			{
				fp32 d = pClip->m_Planes[p].Distance(CVec3Dfp32::GetMatrixRow(_Pos, 3));
				if (d > BoundR) return;
			}
			if (pVC->m_BackPlaneW.Distance(CVec3Dfp32::GetMatrixRow(_Pos, 3)) > BoundR) return;
*/
			m_lspVC[m_iCurrentVC]->AddModel(_pModel, _Pos, _Anim, _OnRenderFlags);
		}
		break;

	case XR_MODEL_WORLD :
		{
			m_lspVC[m_iCurrentVC]->AddWorld(_pModel, _Pos, _Anim, _OnRenderFlags);
		}
		break;

	case XR_MODEL_SKY :
		{
			CXR_Model_Sky* pSky = safe_cast<CXR_Model_Sky>(_pModel);
			if (pSky)
				m_lspVC[m_iCurrentVC]->AddSky(pSky, _Pos, _Anim);
			else
			{
				ConOut("§cf80WARNING: (CXR_EngineImpl::Render_AddModel) Model supplied as type XR_MODEL_SKY was not an object of class CXR_Model_Sky.");
				return false;
			}
	}
		break;

	default :
		{
			ConOut(CStrF("§cf80WARNING: (CXR_EngineImpl::Render_AddModel) Invalid model-type. (%d)", _ModelType));
			return false;
		}
	}
	return true;
}

void CXR_EngineImpl::Render_Light_AddDynamic(const CXR_Light& _Light, int _Priority)
{
	MAUTOSTRIP(CXR_EngineImpl_Render_Light_AddDynamic, MAUTOSTRIP_VOID);
	M_LOCK(m_RenderAddLock);
	if (!m_bDynLight && !_Priority) return;

	CXR_ViewContext* pVC = GetVC();
	const CRC_ClipVolume* pClip = &pVC->m_ClipW;

	for(int iP = 0; iP < pClip->m_nPlanes; iP++)
		if (pClip->m_Planes[iP].Distance(_Light.GetPosition()) > _Light.m_Range) return;

/*	if (m_EngineMode == XR_ENGINE_MODE_UNIFIED)
		pVC->GetLightState()->AddStatic(_Light);
	else*/
		pVC->GetLightState()->AddDynamic(_Light);
}

void CXR_EngineImpl::Render_Light_AddDynamic(const CVec3Dfp32& _Pos, const CVec3Dfp32& _Intensity, fp32 _Range, int _Flags, int _Type, int _Priority)
{
	MAUTOSTRIP(CXR_EngineImpl_Render_Light_AddDynamic_2, MAUTOSTRIP_VOID);
	M_LOCK(m_RenderAddLock);
	if (!m_bDynLight && !_Priority) return;

	CXR_ViewContext* pVC = GetVC();
	const CRC_ClipVolume* pClip = &pVC->m_ClipW;

	for(int iP = 0; iP < pClip->m_nPlanes; iP++)
		if (pClip->m_Planes[iP].Distance(_Pos) > _Range) return;

/*	if (m_EngineMode == XR_ENGINE_MODE_UNIFIED)
		pVC->GetLightState()->AddStatic(0, _Pos, _Intensity, _Range, _Flags, _Type);
	else*/
		pVC->GetLightState()->AddDynamic(_Pos, _Intensity, _Range, _Flags, _Type);
}

void CXR_EngineImpl::Render_Light_AddStatic(const CXR_Light& _Light)
{
	MAUTOSTRIP(CXR_EngineImpl_Render_Light_AddStatic, MAUTOSTRIP_VOID);
	M_LOCK(m_RenderAddLock);
	CXR_ViewContext* pVC = GetVC();
	const CRC_ClipVolume* pClip = &pVC->m_ClipW;

	CVec3Dfp32 ViewPos;
	ViewPos = _Light.GetPosition();
	ViewPos *= pVC->m_W2VMat;

	fp32 Back = m_pVBM->Viewport_Get()->GetBackPlane();
	if (ViewPos.k[2] > Back + _Light.m_Range) return;

	for(int iP = 0; iP < pClip->m_nPlanes; iP++)
		if (pClip->m_Planes[iP].Distance(_Light.GetPosition()) > _Light.m_Range) return;

	if (pVC->GetLightState()->m_nStatic >= 96) return;

	pVC->GetLightState()->AddStatic(_Light);
}

void CXR_EngineImpl::Render_Light_AddStatic(int _LightID, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Intensity, fp32 _Range, int _Flags, int _Type)
{
	MAUTOSTRIP(CXR_EngineImpl_Render_Light_AddStatic_2, MAUTOSTRIP_VOID);
	// IGNORE!
	return;

	M_LOCK(m_RenderAddLock);
	CXR_ViewContext* pVC = GetVC();
	const CRC_ClipVolume* pClip = &pVC->m_ClipW;

	CVec3Dfp32 ViewPos;
	ViewPos = _Pos;
	ViewPos *= pVC->m_W2VMat;

	fp32 Back = m_pVBM->Viewport_Get()->GetBackPlane();
	if (ViewPos.k[2] > Back + _Range) return;

	for(int iP = 0; iP < pClip->m_nPlanes; iP++)
		if (pClip->m_Planes[iP].Distance(_Pos) > _Range) return;

	if (pVC->GetLightState()->m_nStatic >= 96) return;

	pVC->GetLightState()->AddStatic(_LightID, _Pos, _Intensity, _Range, _Flags, _Type);
}

void CXR_EngineImpl::Render_Light_State(int _LightID, const CVec3Dfp32& _Intensity)
{
	MAUTOSTRIP(CXR_EngineImpl_Render_Light_State, MAUTOSTRIP_VOID);
	GetVC()->GetLightState()->Set(_LightID, _Intensity);
}

CXR_VBManager* CXR_EngineImpl::GetVBM()
{
	MAUTOSTRIP(CXR_EngineImpl_GetVBM, NULL);
	M_ASSERT(m_pVBM, "No VBM");
	return m_pVBM; /*m_lspVC[m_iCurrentVC]->m_pVBM;*/
}

CXR_ViewContext* CXR_EngineImpl::GetVC()
{
	MAUTOSTRIP(CXR_EngineImpl_GetVC, NULL);
	return m_lspVC[m_iCurrentVC];
}

CXR_WorldLightState* CXR_EngineImpl::GetWLS()
{
	MAUTOSTRIP(CXR_EngineImpl_GetWLS, NULL);
	return m_lspVC[m_iCurrentVC]->GetLightState();
}

TPtr<CXR_FogState> CXR_EngineImpl::GetFogState()
{
	MAUTOSTRIP(CXR_EngineImpl_GetFogState, NULL);
	if (!m_lspVC.ValidPos(m_iCurrentVC))
		return NULL;

	return m_lspVC[m_iCurrentVC]->m_spFogState;
}

CXR_Portal* CXR_EngineImpl::GetPortal(int _iPortal)
{
	MAUTOSTRIP(CXR_EngineImpl_GetPortal, NULL);
	CXR_ViewContext* pVC = GetVC();

	if (!pVC->m_lPortals.ValidPos(_iPortal)) return NULL;
	if (_iPortal >= pVC->m_nPortals) return NULL;
	return &pVC->m_lPortals[_iPortal];
}

CXR_SurfaceContext* CXR_EngineImpl::GetSC()
{
	MAUTOSTRIP(CXR_EngineImpl_GetSC, NULL);
	return m_pSC;
}

CTextureContext* CXR_EngineImpl::GetTC()
{
	MAUTOSTRIP(CXR_EngineImpl_GetTC, NULL);
	return m_pTC;
}

void CXR_EngineImpl::VBM_End()
{
	MAUTOSTRIP(CXR_EngineImpl_VBM_End, MAUTOSTRIP_VOID);
	M_ASSERT(m_iCurrentVBMDepth == m_iCurrentVC, CStrF("Invalid operation. %d != %d", m_iCurrentVBMDepth, m_iCurrentVC));

	M_LOCK(m_RenderAddLock);

	int nMaxLights = 0;
	if (m_pViewClip)
	{
		CXR_LightOcclusionInfo* pLO = m_pViewClip->View_Light_GetOcclusionArray(nMaxLights);
		if(pLO && nMaxLights > 0)
			GetVBM()->ScopeSetLightOcclusionMask(pLO, nMaxLights);
	}
	GetVBM()->ScopeEnd();
	VBM_ClearDependencies();

	m_iCurrentVBMDepth = -1;

	m_Shader.OnEndFrame();
//	m_Shader.InitAttributeCache(0);
}

void CXR_EngineImpl::VBM_Begin(int _nMaxVB)
{
	MAUTOSTRIP(CXR_EngineImpl_VBM_Begin, MAUTOSTRIP_VOID);
	M_ASSERT(m_iCurrentVBMDepth == -1, CStrF("Invalid operation. %d != -1", m_iCurrentVBMDepth));

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	if (m_bPendingRecordVBHeap)
	{
		GetVBM()->RecordFrame();
		--m_bPendingRecordVBHeap;
	}
#endif
	GetVBM()->ScopeBegin("CXR_EngineImpl::VBM_Begin", true, _nMaxVB);
	GetVBM()->ScopeSetBufferskip(GetVCDepth() ? 0 : m_VBSkip);
	GetVBM()->ScopeSetFlags(VBMSCOPE_FL_RENDERWIRE);

	m_iCurrentVBMDepth = m_iCurrentVC;

//	int nMaxLights = (m_pSceneGraphInstance) ? m_pSceneGraphInstance->SceneGraph_Light_GetMaxIndex() : 0;
//	m_Shader.InitAttributeCache(nMaxLights);
}

void CXR_EngineImpl::VBM_ClearDependencies()
{
	MAUTOSTRIP(CXR_EngineImpl_VBM_ClearDependencies, MAUTOSTRIP_VOID);
	GetFogState()->ClearVBMDependencies();
}


bool CXR_EngineImpl::View_GetClip_Sphere(const CVec3Dfp32& _Origin, fp32 _Radius, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume, CXR_RenderInfo* _pRenderInfo)
{
	MAUTOSTRIP(CXR_EngineImpl_View_GetClip_Sphere, false);
	CXR_ViewContext* pVC = GetVC();
	if (!pVC->m_pViewClip)
		Error("View_GetClip_Sphere", "Invalid call.");

	return pVC->m_pViewClip->View_GetClip_Sphere(_Origin, _Radius, _MediumFlags, _ObjectMask, _pClipVolume, _pRenderInfo);
}

bool CXR_EngineImpl::View_GetClip_Box(const CVec3Dfp32& _BoxMin, const CVec3Dfp32& _BoxMax, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume, CXR_RenderInfo* _pRenderInfo)
{
	MAUTOSTRIP(CXR_EngineImpl_View_GetClip_Box, false);
	CXR_ViewContext* pVC = GetVC();
	if (!pVC->m_pViewClip)
		Error("View_GetClip_Sphere", "Invalid call.");

	return pVC->m_pViewClip->View_GetClip_Box(_BoxMin, _BoxMax, _MediumFlags, _ObjectMask, _pClipVolume, _pRenderInfo);
}

void CXR_EngineImpl::Engine_PushViewContext(const CMat4Dfp32& _CameraWMat, const CMat4Dfp32& _CameraWVelMat, const CVec3Dfp32& _VisPos, const CRC_ClipVolume& _Clip, const CPlane3Dfp32& _FrontPlaneW, int _SceneType)
{
	MAUTOSTRIP(CXR_EngineImpl_Engine_PushViewContext, MAUTOSTRIP_VOID);
	MSCOPE(CXR_EngineImpl::Engine_PushViewContext, XR_ENGINE);
	
	m_nRecurse++;
	m_iCurrentVC++;

	CXR_ViewContextImpl* pVC = m_lspVC[m_iCurrentVC];
	pVC->m_Clip.Copy(_Clip);
	pVC->m_ClipW.Copy(_Clip);
	TransformClipVolume(pVC->m_ClipW, _CameraWMat);

#ifndef	CPU_SOFTWARE_FP64
	for(int p = 0; p < pVC->m_ClipW.m_nPlanes; p++)
		pVC->m_ClipPlanesW8[p] = ConvertPlane8(pVC->m_ClipW.m_Planes[p]);
#endif

	pVC->m_SceneType = _SceneType;

//	CMat4Dfp32 W2VMat;
//	_CameraWMat.InverseOrthogonal(W2VMat);

	CPlane3Dfp32 BackPlane;
	BackPlane.CreateND(CVec3Dfp32(0,0,1), -m_pVBM->Viewport_Get()->GetBackPlane() );
	BackPlane.Transform(_CameraWMat);

	pVC->Clear(_CameraWMat, _CameraWVelMat, _FrontPlaneW, BackPlane);
	pVC->m_VisPos = _VisPos;
	pVC->m_LODScale = m_LODScaleBias * m_pVBM->Viewport_Get()->GetFOV() / 90.0f;

	Engine_SetCurrentVC(m_iCurrentVC);
}

void CXR_EngineImpl::Engine_PopViewContext()
{
	MAUTOSTRIP(CXR_EngineImpl_Engine_PopViewContext, MAUTOSTRIP_VOID);
	m_iCurrentVC--;
	CXR_ViewContextImpl* pVC = m_lspVC[m_iCurrentVC];
	if (pVC->m_pViewClip)
		pVC->m_pViewClip->View_SetCurrent(m_iCurrentVC, m_pSceneGraphInstance);

	Engine_SetCurrentVC(m_iCurrentVC);
}

void CXR_EngineImpl::Engine_BuildViewContext(const CMat4Dfp32& _CameraWMat, const CMat4Dfp32& _CameraWVelMat, const CVec3Dfp32& _VisPos, const CRC_ClipVolume& _Clip, const CPlane3Dfp32& _FrontPlaneW, int _SceneType)
{
	MAUTOSTRIP(CXR_EngineImpl_Engine_BuildViewContext, MAUTOSTRIP_VOID);
	MSCOPE(CXR_EngineImpl::Engine_BuildViewContext, XR_ENGINE);

	if (m_bFreeze)
		return;

	m_nRecurse++;

	CXR_ViewContextImpl* pVC = m_lspVC[m_iCurrentVC];
	pVC->m_Clip.Copy(_Clip);
	pVC->m_ClipW.Copy(_Clip);
	TransformClipVolume(pVC->m_ClipW, _CameraWMat);

#ifndef	CPU_SOFTWARE_FP64
	for(int p = 0; p < pVC->m_ClipW.m_nPlanes; p++)
		pVC->m_ClipPlanesW8[p] = ConvertPlane8(pVC->m_ClipW.m_Planes[p]);
#endif

	pVC->m_SceneType = _SceneType;
	pVC->m_bNeedResolve_TempFixMe = 0;

	CMat4Dfp32 W2VMat;
	_CameraWMat.InverseOrthogonal(W2VMat);

	CPlane3Dfp32 BackPlane;
	BackPlane.CreateND(CVec3Dfp32(0,0,1), -m_pVBM->Viewport_Get()->GetBackPlane() );
	BackPlane.Transform(_CameraWMat);

	pVC->Clear(_CameraWMat, _CameraWVelMat, _FrontPlaneW, BackPlane);
	pVC->m_VisPos = _VisPos;
	pVC->m_LODScale = m_LODScaleBias * m_pVBM->Viewport_Get()->GetFOV() / 90.0f;

	Engine_SetCurrentVC(m_iCurrentVC);
	m_pViewClip = NULL;

	// -------------------------------------------------------------------
	// Enumerate models that affect occlusion culling first
	if (m_pClient) 
		m_pClient->EngineClient_EnumerateView(this, m_iCurrentVC, CXR_ENGINE_EMUMVIEWTYPE_VIEWCLIP);
	else
		ConOut("(CXR_EngineImpl::Engine_BuildViewContext) No engine-client.");

	// -------------------------------------------------------------------
	// Initialize view clip interface.
	
//GetFogState()->DepthFog_Init(500, 1000, 0);

	if (pVC->m_pViewClip)
	{
		MSCOPE(if(pVC->m_pViewClip), XR_ENGINE);
		M_NAMEDEVENT("View_Init", 0xffff0000);

		CXR_AnimState Anim;

		CXR_ViewContext* pVC = m_lspVC[m_iCurrentVC];
		CMat4Dfp32 ViewClipPos; ViewClipPos.Unit();

		TProfileDef(Time);

		{
			TMeasureProfile(Time);
			pVC->m_pViewClip->View_SetCurrent(m_iCurrentVC, m_pSceneGraphInstance);

			if (!m_iCurrentVC)
				pVC->m_pViewClip->View_Init(m_iCurrentVC, this, m_pRender, NULL, &Anim, ViewClipPos, pVC->m_W2VMat);
			else
			{
				CRC_ClipVolume Portal;
				Portal.Copy(pVC->m_Portal);
				pVC->m_pViewClip->View_Init(m_iCurrentVC, this, m_pRender, Portal.m_Vertices, Portal.m_nPlanes, &Anim, ViewClipPos, pVC->m_W2VMat);
			}
		}
//ConOut("ViewInit " + T_String(" ", Time));
		m_pViewClip = pVC->m_pViewClip;
	}

//GetFogState()->DepthFog_Init(500, 1000, 0);

	// -------------------------------------------------------------------
	// Enumerate all other models
	if (m_pClient) 
		m_pClient->EngineClient_EnumerateView(this, m_iCurrentVC, CXR_ENGINE_EMUMVIEWTYPE_NORMAL);
	else
		ConOut("(CXR_EngineImpl::Engine_BuildViewContext) No engine-client.");

	if (m_bShowRecurse) 
		ConOut(CStr(char(32), m_iCurrentVC*4) + CStrF("RenderVC %d, %d/%d Models", m_iCurrentVC, pVC->m_nObjects, pVC->m_lObjects.Len() ));

	if (pVC->m_nObjects == pVC->m_lObjects.Len())
		ConOut("§cf80WARNING: (CXR_EngineImpl::Engine_BuildViewContext) Too many models.");

//GetFogState()->DepthFog_Init(500, 1000, 0);
}

void RenderZPolygon(CRenderContext* _pRC, fp32 _ZDepth, const CVec3Dfp32* _pV, const CMat4Dfp32* _pMat, int _nv);
void RenderZPolygon(CRenderContext* _pRC, fp32 _ZDepth, const CVec3Dfp32* _pV, const CMat4Dfp32* _pMat, int _nv)
{
	MAUTOSTRIP(RenderZPolygon, MAUTOSTRIP_VOID);
	Error_static("RenderZPolygon", "FixMe: Using legacy rendering API.");

	_pRC->Attrib_Push();
	_pRC->Attrib_RasterMode(CRC_RASTERMODE_NONE);
	_pRC->Attrib_SourceBlend(CRC_BLEND_ZERO);
	_pRC->Attrib_DestBlend(CRC_BLEND_ONE);
	_pRC->Attrib_Enable(CRC_FLAGS_BLEND | CRC_FLAGS_ZWRITE);
	_pRC->Attrib_Disable(CRC_FLAGS_ZCOMPARE);

	CVec3Dfp32 VV[16];
	if (_nv > 16) 
	{
		_pRC->Attrib_Pop();
		return;
	}
//	const uint32 IndexRamp[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

	for(int v = 0; v < _nv; v++)
	{
		VV[v] = _pV[v];
		if (_pMat) VV[v] *= (*_pMat);

		fp32 s = _ZDepth / VV[v].k[2];
		VV[v] *= s;
	}
//	_pRC->Render_Polygon(_nv, VV, (CVec2Dfp32*) &VV[0], IndexRamp, IndexRamp, CPixel32(0));
	_pRC->Attrib_Pop();
}

void CXR_EngineImpl::Engine_SetCurrentVC(int _iVC)
{
	MAUTOSTRIP(CXR_EngineImpl_Engine_SetCurrentVC, MAUTOSTRIP_VOID);
	m_iCurrentVC = _iVC;
	CXR_ViewContextImpl* pVC = m_lspVC[_iVC];
	m_LODScale = pVC->m_LODScale;
	m_SceneType = pVC->m_SceneType;
	m_pViewClip = pVC->m_pViewClip;
	m_pCurrentFogState = pVC->m_spFogState;
	m_bCurrentVCMirrored = pVC->m_bIsMirrored;
}

void CXR_EngineImpl::Engine_RVC_RenderSky(CXR_ViewContextImpl* _pVC)
{
	if (m_bSky && _pVC->GetSky())
	{
		MSCOPE(Sky, XR_ENGINE);

//ConOut(CStrF("RenderSky (2), Depth %d", GetVCDepth()));
		CXR_AnimState Anim = _pVC->m_SkyAnimState;
		Anim.m_Anim1 = 0x0100;
		CMat4Dfp32 Mat; Mat.Unit();
//if (GetVCDepth()) pVC->GetSky()->OpenFrustrum();
		_pVC->GetSky()->PreRender(this, _pVC->m_pViewClip, &Anim, Mat, _pVC->m_W2VMat, 0);
		_pVC->GetSky()->OnRender(this, m_pRender, GetVBM(), _pVC->m_pViewClip, _pVC->GetLightState(), &Anim, Mat, _pVC->m_W2VMat, 0);
	}
}

void CXR_EngineImpl::Engine_RVC_PrePortal(CXR_ViewContextImpl* _pVC)
{
	MAUTOSTRIP(CXR_EngineImpl_Engine_RVC_PrePortal, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_EngineImpl::Engine_RVC_PrePortal); //AR-SCOPE

	// Render all "Pass < 0" objects
	//		FogVolume
	//		LightObstructors
	if (GetVCDepth() || !m_bPortalsOnly)
	{
		VBM_Begin();
		{
			for(int iObj = 0; iObj < _pVC->m_nObjects; iObj++)
			{
				if (_pVC->m_pObjects[iObj].m_RenderPass < 0)
					RenderModel(&_pVC->m_lObjects[iObj], _pVC->m_pViewClip, 0);
			}
		}

//		Engine_RVC_RenderSky(_pVC);

		VBM_End();
	}
}

struct pCXR_Portal { CXR_Portal* m_p; };

#define Align(size, align) (((size) + (align) - 1) & ((align) - 1))

void CXR_EngineImpl::Engine_RVC_RenderPortals(CXR_ViewContextImpl* _pVC)
{
	MAUTOSTRIP(CXR_EngineImpl_Engine_RVC_RenderPortals, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_EngineImpl::Engine_RVC_RenderPortals); //AR-SCOPE

	if (!_pVC->m_nPortals)
		return;

	bool bPrePortalDone = false;

	// -------------------------------------------------------------------
	// Render portals
	// -------------------------------------------------------------------
	{


/*		pCXR_Portal pP[CXR_MAXPORTALS];
		{ for(int i = 0; i < _pVC->m_nPortals; i++)
			pP[i].m_p = &_pVC->m_lPortals[i];
		}

		// Bubble-sort portals (there shouldn't be too many.)
		{
			for(int i = 0; i < _pVC->m_nPortals-1; i++)
				for(int j = i+1; j < _pVC->m_nPortals; j++)
					if (pP[i].m_p->m_SortZ > pP[j].m_p->m_SortZ) 
						Swap(pP[i].m_p, pP[j].m_p);
		}*/
		TAP_RCD<CXR_Portal*> lpPortals = _pVC->m_lpPortalsSorted;
		int nPSort = 0;

		for(int iPortal = 0; iPortal < _pVC->m_nPortals; iPortal++)
		{
			CXR_Portal* pPortal = &_pVC->m_lPortals[iPortal];

			// Check if the portal is actually visible.
			{
				int nVerts = pPortal->m_Portal.m_nPlanes;

				CVec3Dfp32 VMin(_FP32_MAX);
				CVec3Dfp32 VMax(-_FP32_MAX);
				for(int v = 0; v < nVerts; v++)
				{
					const CVec3Dfp32& VPos = pPortal->m_Portal.m_Vertices[v];
					for(int i = 0; i < 3; i++)
					{
						if (VPos.k[i] > VMax.k[i]) VMax.k[i] = VPos.k[i];
						if (VPos.k[i] < VMin.k[i]) VMin.k[i] = VPos.k[i];
					}
				}
				//				CRC_ClipVolume ViewClip;
				if (_pVC->m_pViewClip)
					if (!_pVC->m_pViewClip->View_GetClip_Box(VMin, VMax, 0, 0, NULL, NULL))
					{
						continue;
					}
			}

			lpPortals[nPSort++] = pPortal;
		}
		_pVC->m_nPortalsSorted = nPSort;
		if (!nPSort)
			return;

		// Bubble-sort portals (there shouldn't be too many.)
		{
			for(int i = 0; i < nPSort-1; i++)
				for(int j = i+1; j < nPSort; j++)
					if (lpPortals[i]->m_SortZ > lpPortals[j]->m_SortZ)
						Swap(lpPortals[i], lpPortals[j]);
//						Swap(pP[i].m_p, pP[j].m_p);
		}

		for(int iPortal = 0; iPortal < nPSort; iPortal++)
		{
			CXR_Portal* pPortal = lpPortals[iPortal];

/*			if (!bPrePortalDone && !pPortal->IsTexture())
			{
				Engine_RVC_PrePortal(_pVC);
				bPrePortalDone = true;
			}*/


			int nVerts = pPortal->m_Portal.m_nPlanes;


			if (m_bShowPortalFence & 4)
			{
				CStr s;
				for(int v = 0; v < nVerts; v++)
					s += pPortal->m_Portal.m_Vertices[v].GetString();
				ConOut(CStrF("VC %d, Rndr portal, %d, ", m_iCurrentVC, nVerts) + s);
			}

			CRC_ClipVolume Clip;
			CMat4Dfp32 Transf;
			pPortal->m_WCam.InverseOrthogonal(Transf);

			Clip.GetVertices(pPortal->m_PortalDest.m_Vertices, nVerts, CVec3Dfp32::GetMatrixRow(pPortal->m_WCam, 3), false);
			TransformClipVolume(Clip, Transf);

			CPlane3Dfp32 PlaneV(pPortal->m_Plane);
			PlaneV.Transform(_pVC->m_W2VMat);
			PlaneV.Inverse();
			PlaneV.Translate(-PlaneV.n*0.1);

			if (pPortal->IsTexture())
				PlaneV.Translate(PlaneV.n * -8.0f);

			CPlane3Dfp32 VBackPlane;
			VBackPlane.n = CVec3Dfp32(0,0,1);
			VBackPlane.d = m_pVBM->Viewport_Get()->GetBackPlane();
			if (_pVC->GetDepth()+1 < m_lspVC.Len())
			{
				m_iCurrentVC = _pVC->GetDepth()+1;	// Not doing Engine_SetCurrentVC because Engine_BuildViewContext will do it.
				CXR_ViewContext* pNextVC = m_lspVC[m_iCurrentVC];
				
				pNextVC->m_Portal.Copy(pPortal->m_PortalDest);
//				pNextVC->m_FrontPlaneW = pPortal->m_PlaneDest;
				Clip.CreateFromVertices3(VBackPlane);

				if (MACRO_ISMIRRORED(pPortal->m_WCam))
					InvertClipVolume(Clip);

				if (m_bShowRecurse) 
					ConOut(CStr(char(32), _pVC->GetDepth()*4) + CStrF("Rendering portal %d, IsMirr %d, CamFwd %s", iPortal, MACRO_ISMIRRORED(pPortal->m_WCam), (const char*)CVec3Dfp32::GetRow(pPortal->m_WCam, 2).GetString()) );

//				m_pRender->Attrib_Push();
//				m_pRender->Clip_Push();

					// Enable render-clipping
//					m_pRender->Attrib_Enable(CRC_FLAGS_CLIP);
					// Add all planes to clipping
//					m_pRender->Clip_Clear();

				CPlane3Dfp32 lClipPlanes[CXR_CLIPMAXPLANES];
				int nClipPlanes = 0;
				lClipPlanes[nClipPlanes++] = PlaneV;
/*					if (!pPortal->IsTexture())
					{
						for(int p = 0; p < nVerts; p++)
						{
							CPlane3Dfp32 Plane = Clip.m_Planes[p]; Plane.Inverse();
							Plane.Translate(-PlaneV.n*0.1f);
							m_pRender->Clip_AddPlane(Plane, NULL);
						}
					}*/
				{

					int SceneType = pPortal->IsTexture() ? XR_SCENETYPE_TEXTUREPORTAL : XR_SCENETYPE_PORTAL;
//					Engine_BuildViewContext(pPortal->m_WCam, pPortal->m_VisPos, Clip, pPortal->m_PlaneDest, SceneType);

					if (pPortal->IsTexture())
					{
						CRC_Viewport VPCurrent;
						VPCurrent = *m_pVBM->Viewport_Get();
//						m_pRender->Viewport_Push();
						{
							CImage Desc;
							int nMip = 0;
							m_pRender->Texture_GetTC()->GetTextureDesc(pPortal->m_TextureID, &Desc, nMip);

							// Set viewport matching the texture area.
							CRC_Viewport VP;
							VP = VPCurrent;
							VP.SetView(CClipRect(0, 0, Desc.GetWidth(), Desc.GetHeight()), CRct(0,0,Desc.GetWidth(), Desc.GetHeight()));
//							VP.SetFOV(VPCurrent.GetFOV());
//							VP.SetFOVAspect(VPCurrent.GetFOVAspect());

							VP.SetScale(2.0f*VPCurrent.GetScale() / fp32(VPCurrent.GetViewArea().GetWidth()) * fp32(Desc.GetWidth()) );

//							VP.SetScale(VPCurrent.GetScale());
//							VP.SetAspectRatio(fp32(ww) / fp32(wh) * VPCurrent.GetAspectRatio());
							pPortal->m_Viewport = VP;
//							m_pRender->Viewport_Set(&VP);
							if (m_pVBM->Viewport_Push(&VP))
							{
								CMat4Dfp32 CameraVelocity; CameraVelocity.Unit();
								Engine_BuildViewContext(pPortal->m_WCam, CameraVelocity, pPortal->m_VisPos, Clip, pPortal->m_PlaneDest, SceneType);
								// Force texture-update which will invoke CTextureContainer_EnginePortals::BuildInto
								if (m_pVBM->Clip_Push(lClipPlanes, nClipPlanes))
								{
									Engine_RenderViewContext();
									m_pVBM->Clip_Pop();
								}

								// Copy to texture
								{
									m_pVBM->ScopeBegin(false, 1);

									m_pVBM->AddCopyToTexture(0, CRct(0, 0, Desc.GetWidth(), Desc.GetHeight()), CPnt(0, 0), pPortal->m_TextureID, false);

									m_pVBM->ScopeEnd();
								}


	/*							m_spTCPortals->m_iLocalEnabled = pPortal->m_iLocalTexture;
								m_pRender->Texture_GetTC()->MakeDirty(pPortal->m_TextureID);
								m_pRender->Texture_Precache(pPortal->m_TextureID);
								m_spTCPortals->m_iLocalEnabled = -1;*/

			//					RenderZPolygon(m_pRender, 1024, pPortal->m_Portal.m_Vertices, &_pVC->m_W2VMat, nVerts);
								m_pVBM->Viewport_Pop();
							}
						}
//						m_pRender->Viewport_Pop();

						// FIXME: Can we remove color buffer clear?
//						m_pSystem->m_spDisplay->ClearFrameBuffer(CDC_CLEAR_ZBUFFER | CDC_CLEAR_COLOR | CDC_CLEAR_STENCIL, 0xff00ff00);
//						ClearViewport(m_pRender, m_pVBM, GetFogState()->m_DepthFogColor & 0x00ffffff, 0.99f, 0, 1+2+4+8);
						m_pVBM->ScopeBegin(false, 1);
						ClearViewport(m_pRender, m_pVBM, 0x0000ff00, 0.99f, 0, 1+2+4+8);
						m_pVBM->ScopeEnd();
					}
					else
					{
						for(int p = 0; p < nVerts; p++)
						{
							CPlane3Dfp32 Plane = Clip.m_Planes[p]; Plane.Inverse();
							Plane.Translate(-PlaneV.n*0.1f);
							lClipPlanes[nClipPlanes++] = Plane;
							if (nClipPlanes >= CXR_CLIPMAXPLANES)
								break;
//							m_pRender->Clip_AddPlane(Plane, NULL);
						}
						CMat4Dfp32 CameraVelocity; CameraVelocity.Unit();
						Engine_BuildViewContext(pPortal->m_WCam, CameraVelocity, pPortal->m_VisPos, Clip, pPortal->m_PlaneDest, SceneType);
						if (m_pVBM->Clip_Push(lClipPlanes, nClipPlanes))
						{
							Engine_RenderViewContext();
							m_pVBM->Clip_Pop();
						}
					}
				}


//				m_pRender->Clip_Pop();
	//			m_pRender->Attrib_Pop();
				Engine_SetCurrentVC(_pVC->GetDepth());
			}

			if (_pVC->m_pViewClip)
				_pVC->m_pViewClip->View_SetCurrent(_pVC->GetDepth(), m_pSceneGraphInstance);
#if 0
			if (!pPortal->IsTexture())
			{
				if (pPortal->m_pSurfRender)
				{
					VBM_Begin();
					pPortal->m_pSurfRender->RenderPortalSurface(this, pPortal->m_pSurfContext, pPortal->m_Portal.m_Vertices, nVerts, pPortal->m_ModelWMat, pPortal->m_ModelVMat);
					VBM_End();
				}
				else
				{
					VBM_Begin(1);
					uint8* pVBMData = (uint8*)m_pVBM->Alloc(Align(sizeof(CXR_VertexBuffer), 4) + Align(sizeof(CRC_Attributes),4) + Align(sizeof(CXR_VBChain), 4) + Align(sizeof(CVec3Dfp32) * nVerts + sizeof(CMat4Dfp32), 4));
					if(pVBMData)
					{
						CXR_VertexBuffer* pVB = (CXR_VertexBuffer*)pVBMData; pVBMData += sizeof(CXR_VertexBuffer);
						CRC_Attributes* pA = (CRC_Attributes*)pVBMData; pVBMData += sizeof(CRC_Attributes);
						CXR_VBChain* pChain = (CXR_VBChain*)pVBMData; pVBMData += sizeof(CXR_VBChain);
						CMat4Dfp32* pMat = (CMat4Dfp32*)pVBMData; pVBMData += sizeof(CMat4Dfp32);
						CVec3Dfp32* pV = (CVec3Dfp32*)pVBMData; pVBMData += sizeof(CVec3Dfp32) * nVerts;

						memcpy(pV, pPortal->m_Portal.m_Vertices, sizeof(CVec3Dfp32) * nVerts);
						memcpy(pMat, &_pVC->m_W2VMat, sizeof(CMat4Dfp32));

						pA->SetDefault();
						pA->Attrib_Enable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
						pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

						pVB->Construct();
						pVB->m_pAttrib = pA;
						pVB->SetVBChain(pChain);

						pVB->m_Color	= pPortal->m_SurfColor;
						pVB->m_pTransform	= pMat;
						pChain->m_pV = pV;
						pChain->m_nV = nVerts;
						pChain->m_piPrim = const_cast<uint16*>(g_IndexRamp16);
						pChain->m_nPrim = nVerts;

						m_pVBM->AddVB(pVB);
/*
						m_pRender->Attrib_Push();
						{
							m_pRender->Matrix_Set(_pVC->m_W2VMat);
							m_pRender->Attrib_Enable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
			//				m_pRender->Attrib_TextureID(0, m_pRender->Texture_GetTC()->GetTextureID("MARBLE2"));
							m_pRender->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

							m_pRender->Geometry_VertexArray(pPortal->m_Portal.m_Vertices, nVerts, true);
							m_pRender->Geometry_Color(pPortal->m_SurfColor);
							m_pRender->Render_IndexedPolygon(const_cast<uint16*>(g_IndexRamp16), nVerts);
							m_pRender->Geometry_Clear();

	//						m_pRender->Render_Polygon(nVerts, pPortal->m_Portal.m_Vertices, (CVec2Dfp32*) pPortal->m_Portal.m_Vertices, g_IndexRamp32, g_IndexRamp32, pPortal->m_SurfColor);
						}
						m_pRender->Attrib_Pop();
*/
					}
					VBM_End();
				}
			}
#endif
		}

/*		if (!bPrePortalDone)
		{
			Engine_RVC_PrePortal(_pVC);
			bPrePortalDone = true;
		}*/

		VBM_Begin(1);
		CRct Area = m_pVBM->Viewport_Get()->GetViewArea();
		m_pVBM->AddCopyToTexture(0.0f, Area, Area.p0, m_TextureID_ResolveScreen, true);
		VBM_End();
	}

	Engine_InitRenderTextureProjection();
}

void CXR_EngineImpl::Engine_RVC_RestorePortals(CXR_ViewContextImpl* _pVC)
{
	MSCOPESHORT(CXR_EngineImpl::Engine_RVC_RestorePortals);

	if (!_pVC->m_nPortals)
		return;

	TAP<CXR_Portal*> lpPortals = _pVC->m_lpPortalsSorted;
	uint nPortals = _pVC->m_nPortalsSorted;
	for(int iPortal = 0; iPortal < nPortals; iPortal++)
	{
		CXR_Portal* pPortal = lpPortals[iPortal];
		uint nVerts = pPortal->m_Portal.m_nPlanes;

		if (!pPortal->IsTexture())
		{
/*			if (pPortal->m_pSurfRender)
			{
				VBM_Begin();
				pPortal->m_pSurfRender->RenderPortalSurface(this, pPortal->m_pSurfContext, pPortal->m_Portal.m_Vertices, nVerts, pPortal->m_ModelWMat, pPortal->m_ModelVMat);
				VBM_End();
			}
			else*/
			{
//				VBM_Begin(1);
				const uint nTexGen = 4;
				const uint AllocSize = 
					sizeof(CVec4Dfp32)*nTexGen + 
					sizeof(CMat4Dfp32) +
					sizeof(CXR_VertexBuffer) + 
					sizeof(CRC_Attributes) + 
					sizeof(CXR_VBChain) + 
					sizeof(CVec3Dfp32) * nVerts +
					sizeof(CRC_ExtAttributes_FragmentProgram20);
				uint8* pVBMDataBegin = (uint8*)m_pVBM->Alloc(AllocSize);
				uint8* pVBMData = pVBMDataBegin;

				if(pVBMData)
				{
					CVec4Dfp32* pTexGenAttr = (CVec4Dfp32*)pVBMData; pVBMData += sizeof(CVec4Dfp32)*nTexGen;
					CMat4Dfp32* pMat = (CMat4Dfp32*)pVBMData; pVBMData += sizeof(CMat4Dfp32);
					CRC_Attributes* pA = (CRC_Attributes*)pVBMData; pVBMData += sizeof(CRC_Attributes);
					CXR_VertexBuffer* pVB = (CXR_VertexBuffer*)pVBMData; pVBMData += sizeof(CXR_VertexBuffer);
					CXR_VBChain* pChain = (CXR_VBChain*)pVBMData; pVBMData += sizeof(CXR_VBChain);
					CVec3Dfp32* pV = (CVec3Dfp32*)pVBMData; pVBMData += sizeof(CVec3Dfp32) * nVerts;
					CRC_ExtAttributes_FragmentProgram20* pFP = (CRC_ExtAttributes_FragmentProgram20*) pVBMData; pVBMData += sizeof(CRC_ExtAttributes_FragmentProgram20);

					M_ASSERT((pVBMData - pVBMDataBegin) == AllocSize, "!");

					pVB->Construct();
					pVB->Clear();
					pChain->Clear();
					pFP->Clear();
					pFP->SetProgram("TexEnvProj1",MHASH3('TexE','nvPr','oj1'));

					memcpy(pV, pPortal->m_Portal.m_Vertices, sizeof(CVec3Dfp32) * nVerts);
					memcpy(pMat, &_pVC->m_W2VMat, sizeof(CMat4Dfp32));

					pA->SetDefault();
					pA->m_pExtAttrib = pFP;
					pA->Attrib_Enable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
					pA->Attrib_Disable(CRC_FLAGS_CULL);
					pA->Attrib_RasterMode(CRC_RASTERMODE_NONE);

					pA->Attrib_TexGen(0, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_ALL);
					pA->Attrib_TexGenAttr((fp32*)pTexGenAttr);
					pA->Attrib_TextureID(0, m_TextureID_ResolveScreen);

					CMat4Dfp32& DestMat(*(CMat4Dfp32*)pTexGenAttr);
					_pVC->m_W2VMat.Multiply(m_Screen_TextureProjection, DestMat);
					DestMat.Transpose();

					pVB->m_pAttrib = pA;
					pVB->SetVBChain(pChain);

//					pVB->m_Color	= pPortal->m_SurfColor;
					pVB->m_Color = 0xff80ff80;
					pVB->m_pTransform	= pMat;
					pVB->m_Priority = CXR_VBPRIORITY_UNIFIED_ZBUFFER + 2048.0f + fp32(iPortal)*0.01f;
					pChain->m_pV = pV;
					pChain->m_nV = nVerts;
					pChain->Render_IndexedTriangles(CXR_Util::m_lTriFanTriangles, nVerts-2);

					m_pVBM->AddVB(pVB);

					/*
					m_pRender->Attrib_Push();
					{
					m_pRender->Matrix_Set(_pVC->m_W2VMat);
					m_pRender->Attrib_Enable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
					//				m_pRender->Attrib_TextureID(0, m_pRender->Texture_GetTC()->GetTextureID("MARBLE2"));
					m_pRender->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

					m_pRender->Geometry_VertexArray(pPortal->m_Portal.m_Vertices, nVerts, true);
					m_pRender->Geometry_Color(pPortal->m_SurfColor);
					m_pRender->Render_IndexedPolygon(const_cast<uint16*>(g_IndexRamp16), nVerts);
					m_pRender->Geometry_Clear();

					//						m_pRender->Render_Polygon(nVerts, pPortal->m_Portal.m_Vertices, (CVec2Dfp32*) pPortal->m_Portal.m_Vertices, g_IndexRamp32, g_IndexRamp32, pPortal->m_SurfColor);
					}
					m_pRender->Attrib_Pop();
					*/
				}
//				VBM_End();
			}
		}
	}
}

class CThreadPoolOnRenderArg
{
public:
	CXR_EngineImpl* m_pEngine;
	CXR_ViewContextImpl* m_pVC;
	CThreadPoolOnRenderArg(CXR_EngineImpl* _pEngine, CXR_ViewContextImpl* _pVC) : m_pEngine(_pEngine), m_pVC(_pVC) {}
};

void Thread_OnRender(int _iObj, void* _pArg)
{
	CThreadPoolOnRenderArg* pArg = (CThreadPoolOnRenderArg*)_pArg;
	CXR_ViewContextImpl* pVC = pArg->m_pVC;

	if (pVC->m_pObjects[_iObj].m_RenderPass >= 0)
	{
		M_TRY
		{
			CXR_Model* pModel = pVC->m_lObjects[_iObj].m_pModel;
			if(pModel->GetThreadSafe())
				pArg->m_pEngine->RenderModel(&pVC->m_lObjects[_iObj], pVC->m_pViewClip, 0);
		}
		M_CATCH(
		catch(CCException _Ex)
		{
			throw _Ex;
		}
		)
	}
}

#ifdef M_Profile
void Thread_OnRender_Perf(int _iObj, void* _pArg)
{
	CThreadPoolOnRenderArg* pArg = (CThreadPoolOnRenderArg*)_pArg;
	CXR_ViewContextImpl* pVC = pArg->m_pVC;

	CXR_VCModelInstance* pObj = &pVC->m_pObjects[_iObj];

	if (pObj->m_RenderPass >= 0)
	{
		M_TRY
		{
			CXR_EngineImpl* pEngine = pArg->m_pEngine;
			CXR_VBManager* pVBM = pEngine->m_pVBM;

			TThinArray<CXR_VertexBuffer *> Buffers;
			Buffers.SetLen(32768);
			CXR_VBManager::CVBAddCollector VBCollector(Buffers.GetBasePtr(), 32768);

			CXR_Model* pModel = pObj->m_pModel;
			{
				pVBM->SetVBAddCollector(&VBCollector);
				pArg->m_pEngine->RenderModel(pObj, pVC->m_pViewClip, 0);
				pVBM->SetVBAddCollector(NULL);
			}


			int nVB = 0;
			int nVBShader = 0;
			int nVBStencil = 0;
			int nVBVirtualAttrib = 0;
			int nVBOther = 0;
			int nImmPrim = 0;
			int nImmVert = 0;

			for(int iVB = 0; iVB < VBCollector.m_iIndex; ++iVB)
			{
				CXR_VertexBuffer *pVB = VBCollector.m_pVBs[iVB];
				if (pVB)
				{
					nVB++;
					if (pVB->m_Flags & CXR_VBFLAGS_VIRTUALATTRIBUTES)
					{
						nVBVirtualAttrib++;
					}
					else
					{
						CRC_Attributes* pA = pVB->m_pAttrib;
						if (pA)
						{
							if (!(pA->m_Flags & CRC_FLAGS_COLORWRITE) && (pA->m_StencilWriteMask))
								nVBStencil++;
							else if (pA->m_pExtAttrib)
								nVBShader++;
							else
								nVBOther++;
						}
					}

					if (pVB->m_Flags & CXR_VBFLAGS_VBCHAIN)
					{
						CXR_VBChain* pChain = pVB->GetVBChain();
						while(pChain)
						{
							nImmVert += pChain->m_nV;
							if (pChain->m_PrimType == CRC_RIP_TRIANGLES)
								nImmPrim += pChain->m_nPrim*3;
							else
								nImmPrim += pChain->m_nPrim;
							pChain = pChain->m_pNextVB;
						}
					}
				}
			}

			CMat4Dfp32 IVMat;
			pVC->m_W2VMat.InverseOrthogonal(IVMat);

			CVec2Dfp32 V(8, 8);
			CVec3Dfp32 Down = CVec3Dfp32::GetRow(IVMat, 1);//(0, 0, -1);
			CVec3Dfp32 Dir = CVec3Dfp32::GetRow(IVMat, 0);
			CBox3Dfp32 Bound, BoundW;
			pModel->GetBound_Box(Bound);
			Bound.Transform(pObj->m_Pos, BoundW);
			CVec3Dfp32 Pos;
			BoundW.m_Min.Lerp(BoundW.m_Max, 0.5, Pos);
			Pos[2] = BoundW.m_Max[2] + 10.0f;

			CMat4Dfp32 *pMat = pVBM->Alloc_M4(pVC->m_W2VMat);

			CRC_Attributes *pA = pVBM->Alloc_Attrib();
			if (!pA || !pMat) return;
			pA->SetDefault();
			pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			pA->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
			pA->Attrib_TextureID(0, pEngine->m_spDebugFont->m_spTC->GetTextureID(0));

			CVec3Dfp32 CullPos = Pos;

			fp32 Priority = CXR_VBPRIORITY_WINDOWS + 1.0f - CVec3Dfp32::GetRow(IVMat, 3).Distance(CullPos) / pVBM->Viewport_Get()->GetBackPlane();

			int Col = 0xff208020;
			{
				CFStr St2 = CFStrF("%.0f", pObj->m_Time.GetTime() * 1000000.0);
				CFStr St = CFStrF("%4s", St2.Str());

				CXR_VertexBuffer *pVB = pVBM->Alloc_VB();
				if (!pVB) return;

				pEngine->m_spDebugFont->Write(pVBM, pVB, Pos, Dir, Down, St.Str(), V, Col);
				pVB->Matrix_Set(pMat);
				pVB->m_pAttrib = pA;
				pVB->m_Priority = Priority;

				pVBM->AddVB(pVB);

				Pos += Down*V[1];
			}

			if (CVec3Dfp32::GetRow(IVMat, 3).DistanceSqr(CullPos) < Sqr(384))
			{
				{
					CXR_VertexBuffer *pVB = pVBM->Alloc_VB();
					if (!pVB) return;
					pEngine->m_spDebugFont->Write(pVBM, pVB, Pos, Dir, Down, pModel->MRTC_ClassName()+10, V, Col);
					pVB->Matrix_Set(pMat);
					pVB->m_pAttrib = pA;
					pVB->m_Priority = Priority;
					pVBM->AddVB(pVB);

					Pos += Down*V[1];
				}

				{
					CFStr St = CFStrF("VB %d+%d+%d+%d=%d", nVBStencil, nVBShader, nVBVirtualAttrib, nVBOther, nVB);

					CXR_VertexBuffer *pVB = pVBM->Alloc_VB();
					if (!pVB) return;
					pEngine->m_spDebugFont->Write(pVBM, pVB, Pos, Dir, Down, St.Str(), V, Col);
					pVB->Matrix_Set(pMat);
					pVB->m_pAttrib = pA;
					pVB->m_Priority = Priority;
					pVBM->AddVB(pVB);

					Pos += Down*V[1];
				}

				if (nImmVert || nImmPrim)
				{
					CFStr St = CFStrF("Imm %d/%d", nImmVert, nImmPrim);

					CXR_VertexBuffer *pVB = pVBM->Alloc_VB();
					if (!pVB) return;
					pEngine->m_spDebugFont->Write(pVBM, pVB, Pos, Dir, Down, St.Str(), V, Col);
					pVB->Matrix_Set(pMat);
					pVB->m_pAttrib = pA;
					pVB->m_Priority = Priority;
					pVBM->AddVB(pVB);

					Pos += Down*V[1];
				}

			}
		}
		M_CATCH(
		catch(CCException _Ex)
		{
			throw _Ex;
		}
		)
	}
}
#endif

// -------------------------------------------------------------------
void CXR_EngineImpl::Engine_RenderViewContext()
{
	MAUTOSTRIP(CXR_EngineImpl_Engine_RenderViewContext, MAUTOSTRIP_VOID);
	MSCOPE(CXR_EngineImpl::Engine_RenderViewContext, XR_ENGINE);

	CXR_ViewContextImpl* pVC = m_lspVC[m_iCurrentVC];
//	if (!pVC->m_bWorldFound) return;

	Engine_InitRenderTextureProjection();

	if (!GetVCDepth() && m_bClearViewport)
	{
//		ClearViewport(m_pRender, GetFogState()->m_DepthFogColor, 0.99f, m_Unified_Clear, 1+2+4+8);
		ClearViewport(m_pRender, m_pVBM, m_pCurrentFogState->m_DepthFogColor & 0x00ffffff, 0.99f, 0, 1+2+4+8);

		// FIXME: UGLY WAY TO CLEAR
/*		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if (pSys->m_spDisplay != NULL) 
			pSys->m_spDisplay->ClearFrameBuffer(CDC_CLEAR_COLOR | CDC_CLEAR_ZBUFFER | CDC_CLEAR_STENCIL, GetFogState()->m_DepthFogColor);
*/	}

	
//CStr Indent(char(32), _iVC*3);

	// Set global culling-direction.
/*	if (MACRO_ISMIRRORED(pVC->m_W2VMat))
		m_pRender->Attrib_Enable(CRC_FLAGS_CULLCW);
	else
		m_pRender->Attrib_Disable(CRC_FLAGS_CULLCW);

*/
	// -------------------------------------------------------------------
	// Cull models to fog end-plane, if vertex-fog is used
	{
		MSCOPE(Fogculling, XR_ENGINE);

		if (pVC->m_spFogState != NULL && pVC->m_spFogState->m_VtxFog_Enable)
		{
			for(int iObj = 0; iObj < pVC->m_nObjects; iObj++)
			{
				CXR_VCModelInstance* pObjInfo = &pVC->m_pObjects[iObj];
				CXR_Model* pModel = pObjInfo->m_pModel;
				fp32 BoundR = pModel->GetBound_Sphere(&pObjInfo->m_Anim);
				if (pVC->m_spFogState->m_VtxFog_EndPlane.Distance(CVec3Dfp32::GetMatrixRow(pObjInfo->m_Pos, 3)) > BoundR)
					pObjInfo->m_pModel = NULL;
			}
		}
	}


	// -------------------------------------------------------------------
	// Run PreRender on all objects
	if (pVC->m_bWorldFound)
	{
		MSCOPE(if(pVC->m_bWorldFound), XR_ENGINE);
		CXR_VCModelInstance* pObj = &pVC->m_World;
#ifdef M_Profile
		if (m_ShowTiming)
			pObj->m_Time.Start();
#endif
		pObj->m_pModel->PreRender(this, pVC->m_pViewClip, &pObj->m_Anim, pObj->m_Pos, pVC->m_W2VMat, pObj->m_OnRenderFlags);
#ifdef M_Profile
		if (m_ShowTiming)
			pObj->m_Time.Stop();
#endif
	}

int nObj = 0;
	{
		MSCOPE(PreRender, XR_ENGINE);
#ifdef M_Profile
		if (m_ShowTiming)
		{
			for(int iObj = 0; iObj < pVC->m_nObjects; iObj++)
			{
				CXR_VCModelInstance* pObj = &pVC->m_lObjects[iObj];
				if (pObj->m_pModel)
				{
					pObj->m_Time.Start();
					pObj->m_pModel->PreRender(this, pVC->m_pViewClip, &pObj->m_Anim, pObj->m_Pos, pVC->m_W2VMat, pObj->m_OnRenderFlags);
					pObj->m_Time.Stop();
					nObj++;
				}
			}
		}
		else
#endif
		{
			for(int iObj = 0; iObj < pVC->m_nObjects; iObj++)
			{
				CXR_VCModelInstance* pObj = &pVC->m_lObjects[iObj];
				if (pObj->m_pModel)
				{
					pObj->m_pModel->PreRender(this, pVC->m_pViewClip, &pObj->m_Anim, pObj->m_Pos, pVC->m_W2VMat, pObj->m_OnRenderFlags);
					nObj++;
				}
			}
		}
	}

	if (m_bShowRecurse) ConOut(CStr(char(32), m_iCurrentVC*4) + CStrF("RenderVC %d, main", m_iCurrentVC));

	// -------------------------------------------------------------------
	// Render portals
	{
		MSCOPE(Engine_RVC_RenderPortals(pVC), XR_ENGINE);
		Engine_RVC_RenderPortals(pVC);
	}

	// -------------------------------------------------------------------

// URGENTFIXEME: Should this be enabled?
//	pVC->m_LightState.CopyAndCull(&m_LightState, pVC->m_pViewClip);
//ConOut(CStrF("%d, %d", pVC->m_LightState.m_nDynamic, pVC->m_LightState.m_nStatic));

	if (!GetVCDepth() && m_bPortalsOnly) return;

	VBM_Begin(m_nMaxVB);


#ifdef PLATFORM_PS2
	if( CDisplayContextPS2::isEnabledColorBuffer() )
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VB();
		CXR_VertexBuffer_PreRender* pPreRender = (CXR_VertexBuffer_PreRender*)m_pVBM->Alloc(sizeof(CXR_VertexBuffer_PreRender));
		pPreRender->Create(NULL, CXR_Util::Render_ApplyColorBufferPreRender);
		pVB->m_Flags |= CXR_VBFLAGS_COLORBUFFER;
		pVB->m_Priority = CXR_VBPRIORITY_APPLY_COLORBUFFER;
		pVB->m_pPreRender = pPreRender;
		pVB->m_pVBChain = 0;
		m_pVBM->AddVB( pVB );

		pVB = m_pVBM->Alloc_VB();
		pPreRender = (CXR_VertexBuffer_PreRender*)m_pVBM->Alloc(sizeof(CXR_VertexBuffer_PreRender));
		pPreRender->Create(NULL, CXR_Util::Render_ClearColorBufferPreRender);
		pVB->m_Flags |= CXR_VBFLAGS_COLORBUFFER;
		pVB->m_Priority = CXR_VBPRIORITY_CLEAR_COLORBUFFER;
		pVB->m_pPreRender = pPreRender;
		pVB->m_pVBChain = 0;
		m_pVBM->AddVB( pVB );
	}
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VB();
		CXR_VertexBuffer_PreRender* pPreRender = (CXR_VertexBuffer_PreRender*)m_pVBM->Alloc(sizeof(CXR_VertexBuffer_PreRender));
		pPreRender->Create(NULL, CXR_Util::Render_GlowFilter);
		pVB->m_Flags |= CXR_VBFLAGS_COLORBUFFER;
		pVB->m_Priority = CXR_VBPRIORITY_GLOWFILTER;
		pVB->m_pPreRender = pPreRender;
		pVB->m_pVBChain = 0;
		m_pVBM->AddVB( pVB );
	}
#endif

// FIXME: REMOVE, THIS IS A TEST!
//MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
//if (pSys->m_spDisplay != NULL) pSys->m_spDisplay->ClearFrameBuffer(CDC_CLEAR_ZBUFFER);

	// Render world
	if (pVC->m_pViewClip) 
		pVC->m_pViewClip->View_SetCurrent(m_iCurrentVC, m_pSceneGraphInstance);
//	pVC->m_LightState.CopyAndCull(&m_LightState, NULL);		// Copy and cull with NULL is just a copy.

	if (pVC->m_pViewClip && m_EngineMode == XR_ENGINE_MODE_UNIFIED)
	{
		m_TempLightState.CopyAndCull(pVC->GetLightState(), pVC->m_pViewClip);
		pVC->GetLightState()->CopyAndCull(&m_TempLightState, NULL);
	}

	// Render sky
	Engine_RVC_RenderSky(pVC);

	TProfileDef(TWorld);
	TProfileDef(TModels);

	// -------------------------------------------------------------------
	// Render all models
	if (!m_bWorldOnly)
	{
		MSCOPE(RenderModel, XR_ENGINE);
		M_NAMEDEVENT("OnRender", 0xffc0ffff);

		TMeasureProfile(TModels);

#ifdef M_Profile
		if (m_ShowTiming & 2)
		{
			CThreadPoolOnRenderArg Arg(this, pVC);
			MRTC_ThreadPoolManager::ProcessEachInstance(pVC->m_nObjects, &Arg, Thread_OnRender_Perf, "OnRender", true);
		}
		else
#endif
		{
			// First do all non-threadsafe objects
			for(int iObj = 0; iObj < pVC->m_nObjects; iObj++)
			{
				if (pVC->m_pObjects[iObj].m_RenderPass >= 0)
				{
					M_TRY
					{
						CXR_Model* pModel = pVC->m_lObjects[iObj].m_pModel;
						if(!pModel->GetThreadSafe())
							RenderModel(&pVC->m_lObjects[iObj], pVC->m_pViewClip, 0);
					}
					M_CATCH(
						catch(CCException _Ex)
					{
						throw _Ex;
					}
					)
				}
			}

			CThreadPoolOnRenderArg Arg(this, pVC);
			MRTC_ThreadPoolManager::ProcessEachInstance(pVC->m_nObjects, &Arg, Thread_OnRender, "OnRender", m_bSyncOnRender);
		}
	}

	// -------------------------------------------------------------------
	// Render World. (This need to be done after all models have been rendered due to the mechanics with View_GetClip_xxx and shadow volume culling.)
	{
		TMeasureProfile(TWorld);
		if (pVC->m_bWorldFound && !m_bObjectsOnly)
		{
			MSCOPE(RenderModel(&pVC->m_World,NULL,1), XR_ENGINE);
			M_NAMEDEVENT("World::OnRender", 0xffc0c0ff);
			RenderModel(&pVC->m_World, NULL, 1);
		}
	}

	// -------------------------------------------------------------------
	// DEBUG: Render all model boundings
	if (!m_bWorldOnly && m_bShowBoundings)
	{
		for(int iObj = 0; iObj < pVC->m_nObjects; iObj++)
		{
			CXR_VCModelInstance* pObj = &pVC->m_lObjects[iObj];
			if (!pObj->m_pModel) continue;

			CBox3Dfp32 Box;
			pObj->m_pModel->GetBound_Box(Box, &pObj->m_Anim);
			CMat4Dfp32 Move, L2V;
			Move.Unit();
			CVec3Dfp32::GetMatrixRow(pObj->m_Pos, 3).SetMatrixRow(Move, 3);

			pObj->m_Pos.Multiply(pVC->m_W2VMat, L2V);

			{
				CXR_VertexBuffer *pVB = CXR_Util::Create_Box(GetVBM(), L2V, Box, 0xff5050ff);
				if(pVB)
					GetVBM()->AddVB(pVB);
			}

			{
				CXR_VertexBuffer *pVB = CXR_Util::Create_Sphere(GetVBM(), L2V, pObj->m_pModel->GetBound_Sphere(&pObj->m_Anim), 0xffff2020);
				if(pVB)
					GetVBM()->AddVB(pVB);
			}
		}
	}

	// -------------------------------------------------------------------
	// DEBUG: Render all portals from recursion level 0
	if (m_bShowPortalFence && !GetVCDepth())
	{
		for(int iPortal = 0; iPortal < pVC->m_nPortals; iPortal++)
		{
			CXR_Portal* pPortal = &GetVC()->m_lPortals[iPortal];

			m_pVBM->RenderWires(GetVC()->m_W2VMat, pPortal->m_Portal.m_Vertices, g_IndexRamp16, pPortal->m_Portal.m_nPlanes, 0xff2f0000, true);
		}
	}

	// -------------------------------------------------------------------
	Engine_RVC_RestorePortals(pVC);

	// -------------------------------------------------------------------
	if (m_EngineMode == XR_ENGINE_MODE_UNIFIED)
	{
		CXR_ViewClipInterface* pView = pVC->m_pViewClip;
		CRC_Viewport* pVP = m_pVBM->Viewport_Get();
		CMat4Dfp32* pMat2D = m_pVBM->Alloc_M4_Proj2DRelBackPlane();

		if (pView && m_pSceneGraphInstance && pMat2D)
		{
			int nLightsUsed = 0;
			const uint16* piLightsUsed = pView->View_Light_GetVisible(nLightsUsed);
			
			for(int i = 0; i < nLightsUsed; i++)
			{
				int iLight = piLightsUsed[i];
				const CXR_LightOcclusionInfo* pLO = pView->View_Light_GetOcclusion(iLight);
				if (pLO)
				{
#if defined(PLATFORM_XBOX1)
					CRect2Duint16 ClearArea;

					if (!i)
					{
						CRct Area = pVP->GetViewRect();;
						ClearArea.m_Min[0] = Area.p0.x;
						ClearArea.m_Min[1] = Area.p0.y;
						ClearArea.m_Max[0] = Area.p1.x;
						ClearArea.m_Max[1] = Area.p1.y;
					}
					else
					{
						ClearArea = pLO->m_ScissorShadow;
						ClearArea.Expand(pLO->m_ScissorShaded);
					}

					if (ClearArea.m_Min[0] < ClearArea.m_Max[0] &&
						ClearArea.m_Min[1] < ClearArea.m_Max[1])
					{
						// Clear
						{

							CXR_VertexBuffer *pVB = m_pVBM->Alloc_VB();
							if (!pVB)
								continue;
							CXR_PreRenderData_RenderTarget_Clear *pData = (CXR_PreRenderData_RenderTarget_Clear *)m_pVBM->Alloc(sizeof(CXR_PreRenderData_RenderTarget_Clear));
							if (!pData)
								continue;
							
							pData->m_ClearArea = CRct(ClearArea.m_Min.k[0], ClearArea.m_Min.k[1], ClearArea.m_Max.k[0], ClearArea.m_Max.k[1]);
							pData->m_StencilClearTo = m_Unified_Clear;
							pData->m_WhatToClear = CDC_CLEAR_STENCIL;

							pVB->m_pPreRender = m_pVBM->Alloc_PreRender(CXR_PreRenderData_RenderTarget_Clear::ClearRenderTarget, pData);
							pVB->m_Priority = iLight - 0.001f;
							pVB->m_Flags |= CXR_VBFLAGS_PRERENDER;

							m_pVBM->AddVB(pVB);
						}

						// Flush graphics pipeline between render passes
						{

							CXR_VertexBuffer *pVB = m_pVBM->Alloc_VB();
							if (!pVB)
								continue;
							
							pVB->m_pPreRender = m_pVBM->Alloc_PreRender(CFlushGraphicsPipelinePreRenderData::CallBack, NULL);
							pVB->m_Priority = iLight + 0.305f;
							pVB->m_Flags |= CXR_VBFLAGS_PRERENDER;

							m_pVBM->AddVB(pVB);
						}
					}



#else

					CScissorRect ClearArea;
#ifndef	PLATFORM_PS2
					if (!i)
					{
						CRct Area = pVP->GetViewRect();;
						ClearArea.SetRect(Area.p0.x, Area.p0.y, Area.p1.x, Area.p1.y);
//						ClearArea.m_Min[0] = Area.p0.x;
//						ClearArea.m_Min[1] = Area.p0.y;
//						ClearArea.m_Max[0] = Area.p1.x;
//						ClearArea.m_Max[1] = Area.p1.y;
					}
					else
					{
						ClearArea = pLO->m_ScissorShadow;
						ClearArea.Expand(pLO->m_ScissorShaded);
// FIXME: Should be able to use AND here. Oh why won't it work?!  =((((
//						ClearArea.Expand(pLO->m_ScissorShaded);
					}
//					ClearArea = pLO->m_ScissorShadow;*/
#endif	// PLATFORM_PS2
					if (ClearArea.IsValid())
					{
						if (m_Shader.m_ShaderMode != XR_SHADERMODE_FRAGMENTPROGRAM20SS)
						{
							// Clear stencil
							CRC_Attributes* pA = m_pVBM->Alloc_Attrib();
							if (!pA)
								continue;
							pA->SetDefault();
							pA->Attrib_Enable(CRC_FLAGS_STENCIL);
							pA->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_COLORWRITE | CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_ZWRITE);
							pA->Attrib_StencilRef(m_Unified_Clear, -1);
							pA->Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_REPLACE, CRC_STENCILOP_REPLACE);
							pA->Attrib_StencilWriteMask(-1);
							pA->Attrib_RasterMode(CRC_RASTERMODE_NONE);

							m_pVBM->AddVB(CXR_Util::VBM_RenderRect(m_pVBM, pMat2D, ClearArea, 0x00808000, iLight - 0.001f, pA));
						}
						else
						{
							// Clear stencil
							{
								CRC_Attributes* pA = m_pVBM->Alloc_Attrib();
								if (!pA)
									continue;
								pA->SetDefault();
								pA->Attrib_Enable(CRC_FLAGS_STENCIL | CRC_FLAGS_ALPHAWRITE);
								pA->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_COLORWRITE | CRC_FLAGS_ZWRITE);
								pA->Attrib_StencilRef(m_Unified_Clear, -1);
								pA->Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_REPLACE, CRC_STENCILOP_REPLACE);
								pA->Attrib_StencilWriteMask(-1);
								pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

								m_pVBM->AddVB(CXR_Util::VBM_RenderRect(m_pVBM, pMat2D, ClearArea, 0x00808000, iLight - 0.001f, pA));
							}
							// Clear alpha
							{
								CRC_Attributes* pA = m_pVBM->Alloc_Attrib();
								if (!pA)
									continue;
								pA->SetDefault();
								pA->Attrib_Enable(CRC_FLAGS_ALPHAWRITE);
								pA->Attrib_Disable(CRC_FLAGS_COLORWRITE | CRC_FLAGS_ZCOMPARE | CRC_FLAGS_CULL | CRC_FLAGS_ZWRITE);
								pA->Attrib_RasterMode(CRC_RASTERMODE_NONE);
								m_pVBM->AddVB(CXR_Util::VBM_RenderRect(m_pVBM, pMat2D, ClearArea, 0xff000000, iLight + 0.10f, pA));
							}

							// Render stencil mask
							{
								CRC_Attributes* pA = m_pVBM->Alloc_Attrib();
								if (!pA)
									continue;
								pA->SetDefault();
								pA->Attrib_Enable(CRC_FLAGS_STENCIL | CRC_FLAGS_ALPHAWRITE);
								pA->Attrib_Disable(CRC_FLAGS_COLORWRITE | CRC_FLAGS_ZCOMPARE | CRC_FLAGS_CULL | CRC_FLAGS_ZWRITE);
								pA->Attrib_StencilRef(128, 255);
								pA->Attrib_StencilFrontOp(CRC_COMPARE_LESSEQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
								pA->Attrib_StencilWriteMask(0);
								pA->Attrib_RasterMode(CRC_RASTERMODE_NONE);

//								int Color = 0x20 << ((iLight & 3) * 8);
								int Color = 0x00000000;

								CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(m_pVBM, pMat2D, ClearArea, Color, iLight + 0.11f, pA);
								if (!pVB)
									continue;
								m_pVBM->AddVB(pVB);
							}

							// Copy to texture
							{
								uint32 MinX, MinY, MaxX, MaxY;
								ClearArea.GetRect(MinX, MinY, MaxX, MaxY);
								m_pVBM->AddCopyToTexture(iLight + 0.12f, CRct(MinX, MinY, MaxX, MaxY), CPnt(MinX, MinY), m_TextureID_ShadowMask, false);
							}
						}
					}
#endif
				}
			}
		}
		else
		{
			// Clear stencil for each light, offseted with the occlusion count for each light.

			MSCOPE(UnifiedStencilClear, XR_ENGINE);

			pVC->GetLightState()->InitLinks();

			int iLight = 0;

			for(CXR_Light* pL = pVC->GetLightState()->GetFirst(); pL; pL = pL->m_pNext)
			{
				CXR_VBManager* pVBM = GetVBM();
				CXR_VertexBuffer* pVB = pVBM->Alloc_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES, 4);
				uint16* pPrim = pVBM->Alloc_Int16(6);
				if (pVB && pPrim)
				{
					int OcclusionCount = 0;
	//ConOut(CStrF("Clearing Light GUID %d with occlusion count %d", pL->m_LightGUID, OcclusionCount));

					CRC_Attributes* pA = pVB->m_pAttrib;
					pA->Attrib_Enable(CRC_FLAGS_STENCIL);
					pA->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_COLORWRITE | CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_ZWRITE);
					pA->Attrib_StencilRef(m_Unified_Clear-OcclusionCount, -1);
					pA->Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_REPLACE, CRC_STENCILOP_REPLACE);
					pA->Attrib_StencilWriteMask(-1);
					pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
					CXR_VBChain *pChain = pVB->GetVBChain();
					CVec3Dfp32* pV = pChain->m_pV;
					pV[0] = CVec3Dfp32(-1000, -1000, 32);
					pV[1] = CVec3Dfp32(1000, -1000, 32);
					pV[2] = CVec3Dfp32(1000, 1000, 32);
					pV[3] = CVec3Dfp32(-1000, 1000, 32);
					pPrim[0] = 0;
					pPrim[1] = 1;
					pPrim[2] = 2;
					pPrim[3] = 0;
					pPrim[4] = 2;
					pPrim[5] = 3;
	//				pVB->Geometry_Color(0x7f008000);
					pVB->m_Priority = pL->m_iLight;
					pVB->Render_IndexedTriangles(pPrim, 2);
					pVBM->AddVB(pVB);

					iLight++;
				}
			}
			
		}

#if 0 // Todo, only enable this if we are rendering portal
		// Restore render mode at end of game
		m_pVBM->AddRenderOptions(1000000, CRC_RENDEROPTION_CONTINUETILING, ~0);
#endif


		// Copy deferred components to their textures
		if (m_Shader.m_ShaderModeTraits & XR_SHADERMODETRAIT_DEFERRED)
		{
			CRct ScreenRect = pVP->GetViewArea();
			// Restore at end of frame

			if (m_Shader.m_ShaderModeTraits & XR_SHADERMODETRAIT_MRT)
			{
				CRC_RenderTargetDesc RT;
				RT.Clear();
//				RT.SetOptions(CRC_RENDEROPTION_PRESERVEZ | CRC_RENDEROPTION_CONTINUETILING);
	#ifdef PLATFORM_XENON
				RT.SetRenderTarget(0, m_TextureID_DeferredNormal, CVec4Dfp32(1.0f, 0.5f, 0.5f, 0), IMAGE_FORMAT_BGR10A2);
	#else
				RT.SetRenderTarget(0, m_TextureID_DeferredNormal, CVec4Dfp32(1.0f, 0.5f, 0.5f, 0), IMAGE_FORMAT_BGRA8);
	#endif
				RT.SetRenderTarget(1, m_TextureID_DeferredDiffuse, 0, IMAGE_FORMAT_BGRA8);
				RT.SetRenderTarget(2, m_TextureID_DeferredSpecular, 0, IMAGE_FORMAT_BGRA8);
				RT.SetResolveRect(ScreenRect);
//				RT.SetClear(CDC_CLEAR_COLOR0, ScreenRect, 0, 0); 

				m_pVBM->AddSetRenderTarget(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 6.5f, RT);
//				m_pVBM->AddClearRenderTarget(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 6.51f, CDC_CLEAR_COLOR, CPixel32(255, 127, 127), 1.0f, 128);

				m_pVBM->AddRenderTargetEnableMasks(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 7.5f, 1);
				m_pVBM->AddRenderTargetEnableMasks(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 8.5f, 2);
				m_pVBM->AddRenderTargetEnableMasks(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 9.5f, 4);

				m_pVBM->AddRenderTargetEnableMasks(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 10.499f, 7);

				RT.Clear();
			//	RT.SetOptions(CRC_RENDEROPTION_PRESERVEZ | CRC_RENDEROPTION_CONTINUETILING);
				m_pVBM->AddSetRenderTarget(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 10.5f, RT);

				m_pVBM->AddNextClearParams(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 10.51f, 0, 0x00000000, 0.0f, 0);
			}
			else
			{
	#ifdef PLATFORM_XENON
				m_pVBM->AddRenderOptions(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 7.5f, CRC_RENDEROPTION_PRESERVEZ | CRC_RENDEROPTION_CONTINUETILING, IMAGE_FORMAT_BGR10A2);
				m_pVBM->AddRenderOptions(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 8.501f, CRC_RENDEROPTION_PRESERVEZ | CRC_RENDEROPTION_CONTINUETILING, IMAGE_FORMAT_BGRA8);
	#else
				m_pVBM->AddRenderOptions(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 7.5f, CRC_RENDEROPTION_PRESERVEZ | CRC_RENDEROPTION_CONTINUETILING, IMAGE_FORMAT_BGRA8);
	#endif

				m_pVBM->AddNextClearParams(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 8.49f, CDC_CLEAR_COLOR, 0x00000000, 0.0f, 0);
				m_pVBM->AddCopyToTexture(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 8.5f, ScreenRect, ScreenRect.p0, m_TextureID_DeferredNormal, true);
	//			m_pVBM->AddNextClearParams(9.49f, CDC_CLEAR_COLOR, 0x00000000, 0.0f, 0);
				m_pVBM->AddCopyToTexture(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 9.5f, ScreenRect, ScreenRect.p0, m_TextureID_DeferredDiffuse, true);
	//			m_pVBM->AddNextClearParams(10.49f, CDC_CLEAR_COLOR, 0x00000000, 0.0f, 0);

				m_pVBM->AddNextClearParams(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 10.49f, CDC_CLEAR_COLOR, 0xff8080ff, 0.0f, 0);
				m_pVBM->AddCopyToTexture(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 10.5f, ScreenRect, ScreenRect.p0, m_TextureID_DeferredSpecular, true);

				if (m_Shader.m_ShaderModeTraits & XR_SHADERMODETRAIT_MOTIONMAP)
				{
					m_pVBM->AddCopyToTexture(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 11.5f, ScreenRect, ScreenRect.p0, m_TextureID_MotionMap, true);
				}


				m_pVBM->AddClearRenderTarget(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 7.51f, CDC_CLEAR_COLOR, CPixel32(255, 127, 127), 1.0f, 128);
	//			m_pVBM->AddClearRenderTarget(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 9.6f, CDC_CLEAR_COLOR, 0, 1.0f, 128);

				m_pVBM->AddRenderOptions(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 11.6f, CRC_RENDEROPTION_PRESERVEZ | CRC_RENDEROPTION_CONTINUETILING, ~0);
			}
		}

		if (pVC->m_bNeedResolve_TempFixMe)
		{
			CRct ScreenRect = pVP->GetViewArea();
			m_pVBM->AddCopyToTexture(CXR_VBPRIORITY_MODEL_TRANSPARENT - 1.0f, ScreenRect, ScreenRect.p0, m_TextureID_ResolveScreen, true);
		}

		if (m_RenderCaps_Flags & CRC_CAPS_FLAGS_COPYDEPTH)
		{
			// Copy depth to color
			CRct SrcRect = pVP->GetViewArea();
			CPnt Dst = SrcRect.p0;
			do
			{
				CXR_VertexBuffer *pVB = m_pVBM->Alloc_VB();
				if (!pVB)
					break;
				CXR_PreRenderData_RenderTarget_Copy* pData = (CXR_PreRenderData_RenderTarget_Copy*)m_pVBM->Alloc(sizeof(CXR_PreRenderData_RenderTarget_Copy));
				if (!pData)
					break;

				pData->m_SrcRect = SrcRect;
				pData->m_Dst = Dst;
				pData->m_CopyType = CRC_COPYTYPE_DEPTHTOCOLOR;

				pVB->m_pPreRender = m_pVBM->Alloc_PreRender(CXR_PreRenderData_RenderTarget_Copy::RenderTarget_Copy, pData);
				pVB->m_Priority = CXR_VBPRIORITY_UNIFIED_ZBUFFER + 12;
				pVB->m_Flags |= CXR_VBFLAGS_PRERENDER;

				m_pVBM->AddVB(pVB);
			} while(0);

			// Copy to depth-stencil texture
			m_pVBM->AddCopyToTexture(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 12.1f, SrcRect, Dst, m_TextureID_DepthStencil, true);
		}
		else
		{
			CRct SrcRect = pVP->GetViewArea();
			CPnt Dst = SrcRect.p0;
			m_pVBM->AddCopyToTexture(CXR_VBPRIORITY_UNIFIED_ZBUFFER + 1.0f, SrcRect, Dst, m_TextureID_Depth, true);
		}


		if (m_bClearViewport || (m_Shader.m_ShaderModeTraits & XR_SHADERMODETRAIT_DEFERRED))
		{
			// Clear shading area
			do
			{
				CRC_Attributes* pA = m_pVBM->Alloc_Attrib();
				if (!pA)
					break;
				pA->SetDefault();
				pA->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
				pA->Attrib_Enable(CRC_FLAGS_COLORWRITE | CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_STENCIL);
				pA->Attrib_RasterMode(CRC_RASTERMODE_NONE);
				pA->Attrib_StencilRef(128, 255);
				pA->Attrib_StencilFrontOp(CRC_COMPARE_EQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
				pA->Attrib_StencilWriteMask(0);

				CRect2Duint16 ClearArea;
				CRct Area = pVP->GetViewRect();;
				ClearArea.m_Min[0] = Area.p0.x;
				ClearArea.m_Min[1] = Area.p0.y;
				ClearArea.m_Max[0] = Area.p1.x;
				ClearArea.m_Max[1] = Area.p1.y;

				CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(m_pVBM, pMat2D, ClearArea, m_Unified_Ambience & 0x00ffffff, CXR_VBPRIORITY_UNIFIED_ZBUFFER + 12.2f, pA);
				if (!pVB)
					break;
				m_pVBM->AddVB(pVB);
			} while(0);

			// Clear background
			do
			{
				CRC_Attributes* pA = m_pVBM->Alloc_Attrib();
				if (!pA)
					break;
				pA->SetDefault();
				pA->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
				pA->Attrib_Enable(CRC_FLAGS_COLORWRITE | CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_STENCIL);
				pA->Attrib_RasterMode(CRC_RASTERMODE_NONE);
				pA->Attrib_StencilRef(128, 255);
				pA->Attrib_StencilFrontOp(CRC_COMPARE_NOTEQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
				pA->Attrib_StencilWriteMask(0);

				CRect2Duint16 ClearArea;
				CRct Area = pVP->GetViewRect();;
				ClearArea.m_Min[0] = Area.p0.x;
				ClearArea.m_Min[1] = Area.p0.y;
				ClearArea.m_Max[0] = Area.p1.x;
				ClearArea.m_Max[1] = Area.p1.y;

				CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(m_pVBM, pMat2D, ClearArea, m_pCurrentFogState->m_DepthFogColor, CXR_VBPRIORITY_UNIFIED_ZBUFFER + 12.2f, pA);
				if (!pVB)
					break;
				m_pVBM->AddVB(pVB);
			} while(0);
		}
	}


/*	{
		CXR_VBManager* pVBM = GetVBM();
		CXR_VertexBuffer* pVB = pVBM->Alloc_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES, 4);
		if (pVB)
		{
			CRC_Attributes* pA = pVB->m_pAttrib;
			pA->Attrib_Enable(CRC_FLAGS_STENCIL);
			pA->Attrib_Disable(CRC_FLAGS_CULL);
			pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
			pA->Attrib_StencilFunction(CRC_COMPARE_GREATER, 0, -1);
			pA->Attrib_StencilOp(CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
			pA->Attrib_StencilWriteMask(-1);
			CVec3Dfp32* pV = pVB->m_pV;
			pV[0] = CVec3Dfp32(-1000, -1000, 32);
			pV[1] = CVec3Dfp32(1000, -1000, 32);
			pV[2] = CVec3Dfp32(1000, 1000, 32);
			pV[3] = CVec3Dfp32(-1000, 1000, 32);
			pVB->Geometry_Color(0xff004000);
			pVB->m_Priority = 10000;
			pVBM->AddVB(pVB);
		}
	}
*/
#ifdef M_Profile
	if (m_ShowTiming & 1) 
		ConOut(CStrF("World:  %f ms,  %d/%d Models %f ms, %d/%d Lights", TWorld.GetTime()*1000.0, nObj, pVC->m_nObjects, TModels.GetTime()*1000.0, pVC->GetLightState()->m_nStatic, pVC->GetLightState()->m_nDynamic));
#endif
	if (m_bShowFogInfo) 
		ConOut(CStrF("VertexFog count:  %d", m_pCurrentFogState->m_VtxFog_VertexCount));

	VBM_End();

#if 0
	if (m_ShowTiming & 2 && m_spDebugFont != NULL) 
	{
		VBM_Begin(pVC->m_nObjects);

		for(int iObj = 0; iObj < pVC->m_nObjects; iObj++)
		{
			CXR_VCModelInstance* pObj = &pVC->m_lObjects[iObj];
			if (pObj->m_pModel)
			{
				int Col = 0xff104010;
				CFStr St2 = CFStrF("%.0f", pObj->m_Time.GetTime() * 1000000.0);
				CFStr St = CFStrF("%4s", St2.Str());

				CMat4Dfp32 IVMat;
				pVC->m_W2VMat.InverseOrthogonal(IVMat);
				CXR_VertexBuffer *pVB = m_pVBM->Alloc_VB(CXR_VB_ATTRIB);
				if (!pVB) break;

				pVB->m_pAttrib->SetDefault();

				CVec2Dfp32 V(8, 8);
				CVec3Dfp32 Down = CVec3Dfp32::GetRow(IVMat, 1);//(0, 0, -1);
				CVec3Dfp32 Dir = CVec3Dfp32::GetRow(IVMat, 0);
				CBox3Dfp32 Bound, BoundW;
				pObj->m_pModel->GetBound_Box(Bound);
				Bound.Transform(pObj->m_Pos, BoundW);
				CVec3Dfp32 Pos;
				BoundW.m_Min.Lerp(BoundW.m_Max, 0.5, Pos);
				Pos[2] = BoundW.m_Max[2] + 10.0f;

				m_spDebugFont->Write(m_pVBM, pVB, Pos, Dir, Down, St.Str(), V, Col);
				CMat4Dfp32 *pMat = m_pVBM->Alloc_M4(pVC->m_W2VMat);
				pVB->Matrix_Set(pMat);
				pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
				pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);

				m_pVBM->AddVB(pVB);
			}
		}
		VBM_End();
	}
#endif
}

void CXR_EngineImpl::Engine_CopyHexagonToCube(CXR_VBManager* _pVBM, CVec3Dfp32* _pVHexagon, int _SrcTextureID, int _DstTextureID)
{
	CMat4Dfp32* pMat2D = _pVBM->Alloc_M4_Proj2DRelBackPlane();
	if (!pMat2D)
		return;

	uint bFlip = m_RenderCaps_Flags & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP;

	fp32 HexagonPixelUV = 1.0f / 256.0f;

	CVec2Dfp32 lTVHexagon[7];
	if (bFlip)
	{
		for(int v = 0; v < 7; v++)
			lTVHexagon[v].Set(_pVHexagon[v][0] * HexagonPixelUV, 1.0f - _pVHexagon[v][1] * HexagonPixelUV);
	}
	else
	{
		for(int v = 0; v < 7; v++)
			lTVHexagon[v].Set(_pVHexagon[v][0] * HexagonPixelUV, _pVHexagon[v][1] * HexagonPixelUV);
	}

	CXR_VertexBuffer* pVB0 = _pVBM->Alloc_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES | CXR_VB_TVERTICES0, 4);
	CXR_VertexBuffer* pVB1 = _pVBM->Alloc_VB(CXR_VB_TVERTICES0, 4);
	CXR_VertexBuffer* pVB2 = _pVBM->Alloc_VB(CXR_VB_TVERTICES0, 4);
	if (!pMat2D || !pVB0 || !pVB1 || !pVB2)
		return;

	CRC_Attributes* pA = pVB0->m_pAttrib;
	pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_CULL);
	pA->Attrib_TextureID(0, _SrcTextureID);
	pVB1->m_pAttrib = pA;
	pVB2->m_pAttrib = pA;
	pVB0->Matrix_Set(pMat2D);
	pVB1->Matrix_Set(pMat2D);
	pVB2->Matrix_Set(pMat2D);
	CXR_VBChain* pChain0 = pVB0->GetVBChain();
	CXR_VBChain* pChain1 = pVB1->GetVBChain();
	CXR_VBChain* pChain2 = pVB2->GetVBChain();
	CVec3Dfp32* pV0 = pChain0->m_pV;
	if (bFlip)
	{
		pV0[0].Set(0, 0, 0);
		pV0[1].Set(128, 0, 0);
		pV0[2].Set(128, 128, 0);
		pV0[3].Set(0, 128, 0);
	}
	else
	{
		pV0[0].Set(0, 128, 0);
		pV0[1].Set(128, 128, 0);
		pV0[2].Set(128, 0, 0);
		pV0[3].Set(0, 0, 0);
	}
	pChain1->m_nV = pChain0->m_nV;
	pChain1->m_pV = pV0;
	pChain2->m_nV = pChain0->m_nV;
	pChain2->m_pV = pV0;
	pChain0->Render_IndexedTriangles(CXR_Util::m_lQuadParticleTriangles, 2);
	pChain1->Render_IndexedTriangles(CXR_Util::m_lQuadParticleTriangles, 2);
	pChain2->Render_IndexedTriangles(CXR_Util::m_lQuadParticleTriangles, 2);

	CVec2Dfp32* pTV0 = (CVec2Dfp32*) pChain0->m_pTV[0];
	CVec2Dfp32* pTV1 = (CVec2Dfp32*) pChain1->m_pTV[0];
	CVec2Dfp32* pTV2 = (CVec2Dfp32*) pChain2->m_pTV[0];
	static uint8 liTV0[4] = { 0, 1, 2, 3 };
	static uint8 liTV1[4] = { 0, 3, 4, 5 };
	static uint8 liTV2[4] = { 0, 5, 6, 1 };
	int iiTV0 = m_iiTV0; int diiTV0 = m_diiTV0;
	pTV0[0] = lTVHexagon[liTV0[iiTV0 & 3]]; iiTV0 += diiTV0;
	pTV0[1] = lTVHexagon[liTV0[iiTV0 & 3]]; iiTV0 += diiTV0;
	pTV0[2] = lTVHexagon[liTV0[iiTV0 & 3]]; iiTV0 += diiTV0;
	pTV0[3] = lTVHexagon[liTV0[iiTV0 & 3]]; iiTV0 += diiTV0;
	int iiTV1 = m_iiTV1; int diiTV1 = m_diiTV1;
	pTV1[0] = lTVHexagon[liTV1[iiTV1 & 3]]; iiTV1 += diiTV1;
	pTV1[1] = lTVHexagon[liTV1[iiTV1 & 3]]; iiTV1 += diiTV1;
	pTV1[2] = lTVHexagon[liTV1[iiTV1 & 3]]; iiTV1 += diiTV1;
	pTV1[3] = lTVHexagon[liTV1[iiTV1 & 3]]; iiTV1 += diiTV1;
	int iiTV2 = m_iiTV2; int diiTV2 = m_diiTV2;
	pTV2[0] = lTVHexagon[liTV2[iiTV2 & 3]]; iiTV2 += diiTV2;
	pTV2[1] = lTVHexagon[liTV2[iiTV2 & 3]]; iiTV2 += diiTV2;
	pTV2[2] = lTVHexagon[liTV2[iiTV2 & 3]]; iiTV2 += diiTV2;
	pTV2[3] = lTVHexagon[liTV2[iiTV2 & 3]]; iiTV2 += diiTV2;

//	pTV0[0] = lTVHexagon[0];	pTV0[1] = lTVHexagon[1];	pTV0[2] = lTVHexagon[2];	pTV0[3] = lTVHexagon[3];
//	pTV1[0] = lTVHexagon[0];	pTV1[1] = lTVHexagon[3];	pTV1[2] = lTVHexagon[4];	pTV1[3] = lTVHexagon[5];
//	pTV2[0] = lTVHexagon[0];	pTV2[1] = lTVHexagon[5];	pTV2[2] = lTVHexagon[6];	pTV2[3] = lTVHexagon[1];

	_pVBM->AddVB(pVB0);
	_pVBM->AddCopyToTexture(0.0f, CRct(0,0,128,128), CPnt(0,0), _DstTextureID, false, 0);
	_pVBM->AddVB(pVB1);
	_pVBM->AddCopyToTexture(0.0f, CRct(0,0,128,128), CPnt(0,0), _DstTextureID, false, 2);
	_pVBM->AddVB(pVB2);
	_pVBM->AddCopyToTexture(0.0f, CRct(0,0,128,128), CPnt(0,0), _DstTextureID, false, 4);
}

void CXR_EngineImpl::Engine_CreateColorCorrection(CXR_VBManager* _pVBM, const CXR_Engine_PostProcessParams *M_RESTRICT _pParams, int _TempTextureID, uint16 _lDstTextureID[4])
{
	static fp32 lHexagon[6][2] = 
	{
		{ 1.0f, 0.0f },
		{ 0.5f, 0.86602540f },
		{ -0.5f, 0.86602540f },
		{ -1.0f, 0.0f },
		{ -0.5f, -0.86602540f },
		{ 0.5f, -0.86602540f }
	};

	static uint16 lHexagonTriangles[18] =
	{
		0,1,2,
		0,2,3,
		0,3,4,
		0,4,5,
		0,5,6,
		0,6,1
	};

	static uint32 lHexagonColors[7] =
	{
		0xffffffff,
		0xffff0000,
		0xffff00ff,
		0xff0000ff,
		0xff00ffff,
		0xff00ff00,
		0xffffff00
	};



	fp32 Radius = 127.0f;
	CVec2Dfp32 Center(128.0f, 128.0f);
	CVec2Dfp32 Pos(000.0f, 0.0f);
	CPnt Posi(000, 0);

	CMat4Dfp32* pMat2D = _pVBM->Alloc_M4_Proj2DRelBackPlane();
	CXR_VertexBuffer* pVB = _pVBM->Alloc_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES | CXR_VB_COLORS, 7);
	if (!pMat2D || !pVB)
		return;

	pVB->Matrix_Set(pMat2D);
	CXR_VBChain* pChain = pVB->GetVBChain();
	CVec3Dfp32* pV = pChain->m_pV;
	CPixel32* pC = pChain->m_pCol;

	pV[0].Set(Pos[0] + Center[0], Pos[1] + Center[1], 0);
	pC[0] = lHexagonColors[0];
	int iC = m_iHexagonColor;
	for(int v = 0; v < 6; v++)
	{
		pV[v+1].Set(Pos[0] + Center[0] + lHexagon[v][0]*Radius, Pos[1] + Center[1] + lHexagon[v][1]*Radius, 0);
		pC[v+1] = lHexagonColors[iC+1]; iC += m_dHexagonColor; if (iC >= 6) iC = 0; if (iC < 0) iC = 5;
	}

//	pChain->m_pCol = (CPixel32*) lHexagonColors;
	pChain->Render_IndexedTriangles(lHexagonTriangles, 6);

	CRC_Attributes* pA = pVB->m_pAttrib;
	pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_CULL);

	_pVBM->AddVB(pVB);

	if (m_spHexagonTest != NULL)
	{
		CRC_Attributes* pA = _pVBM->Alloc_Attrib();
		pA->SetDefault();
		pA->Attrib_TextureID(0, m_spHexagonTest->GetTextureID(0));
		pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_CULL);
		CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(_pVBM, pMat2D, CRect2Duint16(Posi.x, Posi.y, Posi.x+256, Posi.y+256), 0xffffffff, 0.0f, pA);
		if (!pVB)
			return;
		pVB->Geometry_TVertexArray(CXR_Util::VBM_CreateRectUV(_pVBM, CVec2Dfp32(0.0f, 0.0f), CVec2Dfp32(1.0f, 1.0f)), 0);
		_pVBM->AddVB(pVB);
	}
	else
	{
		int nHSC = _pParams->m_nHSC;
		if (nHSC)
		{
			uint8 lBlend[4];
			uint8 lIntensity[4];
			M_VSt_V4f32_u8(M_VMul(M_VScalar(255.0f), M_VLd(&_pParams->m_lHSC_Blend)), (uint32*)&lBlend);
			M_VSt_V4f32_u8(M_VMul(M_VScalar(255.0f), M_VLd(&_pParams->m_lHSC_Intensity)), (uint32*)&lIntensity);

			CRC_Attributes* pA = (CRC_Attributes*)_pVBM->Alloc(sizeof(CRC_Attributes) * nHSC);
			for(int i = 0; i < nHSC; i++)
			{
				pA[i].SetDefault();
				pA[i].Attrib_TextureID(0, _pParams->m_lHSC_TextureID[i]);
				pA[i].Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_CULL);
				pA[i].Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			}
			CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(_pVBM, pMat2D, CRect2Duint16(Posi.x, Posi.y, Posi.x+256, Posi.y+256), 0xffffffff, 0.0f, pA);
			if (!pVB)
				return;
			pVB->Geometry_TVertexArray(CXR_Util::VBM_CreateRectUV(_pVBM, CVec2Dfp32(0.0f, 0.0f), CVec2Dfp32(1.0f, 1.0f)), 0);
			uint Int = lIntensity[0];
			uint Bl = lBlend[0];
			pVB->m_Color = (Bl << 24) + ((Int + (Int << 8) + (Int << 16)));
			_pVBM->AddVB(pVB);

			if (nHSC > 1)
			{
				CXR_VertexBuffer* pVBCopy = (CXR_VertexBuffer*)_pVBM->Alloc(sizeof(CXR_VertexBuffer) * (nHSC-1));
				for(int i = 1; i < nHSC; i++)
				{
					CXR_VertexBuffer* pVB2 = pVBCopy + (i-1);
					*pVB2 = *pVB;
					pVB2->m_pAttrib = pA + i;
					uint Int = lIntensity[i];
					uint Bl = lBlend[i];
					pVB->m_Color = (Bl << 24) + ((Int + (Int << 8) + (Int << 16)));
					_pVBM->AddVB(pVB2);
				}
			}
		}
	}

	_pVBM->AddCopyToTexture(0.0f, CRct(Posi, CPnt(Posi.x+256, Posi.y+256)), Posi, _TempTextureID, false);

	Engine_CopyHexagonToCube(_pVBM, pV, _TempTextureID, _lDstTextureID[0]);
}

static M_FORCEINLINE CRect2Duint16 ShiftRect(const CRect2Duint16& _Rect, int _Shift)
{
	CRect2Duint16 Ret;
	if(_Shift >= 0)
	{
		Ret.m_Min[0] = _Rect.m_Min[0] << _Shift;
		Ret.m_Min[1] = _Rect.m_Min[1] << _Shift;
		Ret.m_Max[0] = _Rect.m_Max[0] << _Shift;
		Ret.m_Max[1] = _Rect.m_Max[1] << _Shift;
	}
	else
	{
		Ret.m_Min[0] = _Rect.m_Min[0] >> -_Shift;
		Ret.m_Min[1] = _Rect.m_Min[1] >> -_Shift;
		Ret.m_Max[0] = _Rect.m_Max[0] >> -_Shift;
		Ret.m_Max[1] = _Rect.m_Max[1] >> -_Shift;
	}
	return Ret;
}

void CXR_EngineImpl::Engine_PostProcess(CXR_VBManager* _pVBM, CRC_Viewport& _3DVP, const CXR_Engine_PostProcessParams *M_RESTRICT _pParams)
{
	CXR_VBManager* pVBM = _pVBM;
	CXR_Engine* pEngine = this;

	CTextureContainer_Screen* pTCScreen = safe_cast<CTextureContainer_Screen>(pEngine->GetInterface(XR_ENGINE_TCSCREEN));
	if (!pTCScreen)
		return;

	int TextureID_Screen = pTCScreen->GetTextureID(0);				// Screen
	int TextureID_PostProcess = m_spTCScreen->GetTextureID(6);	// Postprocess screen
	
	int bRenderTextureVertFlip = m_RenderCaps_Flags & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP;
	int ShaderModeTraits = pEngine->GetShader()->GetShaderModeTraits();

	CRC_Viewport* pVP = &_3DVP;
	CRC_Viewport* pVPScreen = pVBM->Viewport_Get();
	CMat4Dfp32* pMat2D = pVBM->Alloc_M4_Proj2DRelBackPlane();
	if (!pMat2D)
		return;

	bool bNeedUpScale = false;
	CRct ScreenArea = pVPScreen->GetViewArea();
	CPnt ScreenSize(ScreenArea.GetWidth(), ScreenArea.GetHeight());
	int sw = pVP->GetViewRect().GetWidth();
	int sh = pVP->GetViewRect().GetHeight();
	if (ScreenSize.x != sw || ScreenSize.y != sh)
		bNeedUpScale = true;

	fp32 swf = fp32(sw);
	fp32 shf = fp32(sh);
	CVec2Dfp32 UVMin(0.0f, 0.0f);
	CVec2Dfp32 UVMax(swf * m_Screen_PixelUV[0], shf * m_Screen_PixelUV[1]);

	CRct VPRect = pVP->GetViewRect();;
	CRect2Duint16 VPRect16;
	VPRect16 << VPRect;

	CRct VPRectScreen(CPnt(0, 0), ScreenSize);
	if (_pParams->m_ViewFlags & XR_VIEWFLAGS_WIDESCREEN)
	{
		fp32 WideScreen = (1.0 - 1.0 / _pParams->m_AspectChange) * 0.5;

		int Borders = int(M_Ceil(ScreenSize.y * WideScreen));
		VPRectScreen.p0.y = Borders;
		VPRectScreen.p1.y = VPRectScreen.p1.y - Borders;
	}

	CRect2Duint16 VPRectScreen16;
	VPRectScreen16 << VPRectScreen;

	pVBM->ScopeBegin("CXR_Engine::Engine_PostProcess", false);

	do
	{
		int DebugFlags = _pParams->m_DebugFlags;

		bool bDoGlow = (DebugFlags & XR_PPDEBUGFLAGS_NOGLOW) == 0;
		bool bDoFinal = (DebugFlags & XR_PPDEBUGFLAGS_NOFINAL) == 0;
		//		bool bDoGlowX = (DebugFlags & XR_PPDEBUGFLAGS_NOGAUSS_X) == 0;
		//		bool bDoGlowY = (DebugFlags & XR_PPDEBUGFLAGS_NOGAUSS_Y) == 0;
		bool bShowFinalGlow = (DebugFlags & XR_PPDEBUGFLAGS_SHOWGLOW) != 0;

		// -------------------------------------------------------------------
		// Stretch to fullscreen
		// -------------------------------------------------------------------
		if (bNeedUpScale || bDoGlow || _pParams->m_bDynamicExposure)
		{
			int TextureID_BlurScreen = m_spTCScreen->GetTextureID(6);		// Postprocess screen

			// Copy screen
			pVBM->AddCopyToTexture(0, VPRect, VPRect.p0, TextureID_Screen, false);

			if ((m_Shader.m_ShaderModeTraits & XR_SHADERMODETRAIT_MOTIONMAP) && !(m_PostProcess_MBDebugFlags & 1))
			{
				uint16 TextureID_MotionMapBlur = m_spTCScreen->GetTextureID(4);	// Deferred diffuse

				// Shrink motion map
				CVec2Dfp32* pTVBlur = NULL;
				{
					CXR_GaussianBlurParams GP;
					GP.m_Rect = VPRect16;
					GP.m_Bias.SetScalar(0.0f);
					GP.m_Gamma.SetScalar(1.0f);
					GP.m_UVCenter = (UVMin+UVMax) * 0.5f;
					GP.m_Scale.SetScalar(1.0f);
					GP.m_Exp = 1.0f;
					GP.m_SrcUVMin = UVMin;
					GP.m_SrcUVMax = UVMax;
					GP.m_DstUVMin = GP.m_SrcUVMin;
					GP.m_DstUVMax = GP.m_SrcUVMax;
					GP.m_SrcPixelUV = pEngine->m_Screen_PixelUV;
					GP.m_DstPixelUV = pEngine->m_Screen_PixelUV;
					GP.m_pVBM = pVBM;
					GP.m_Priority = 0;
					GP.m_RenderCaps = pEngine->m_RenderCaps_Flags;
					GP.m_TextureID_Src = m_TextureID_MotionMap;
					GP.m_TextureID_Dst = TextureID_MotionMapBlur;
					GP.m_Flags = DebugFlags & (XR_PPDEBUGFLAGS_NOGAUSS_X | XR_PPDEBUGFLAGS_NOGAUSS_Y);
					GP.m_nShrink = 2;
					GP.SetFilter(_pParams->m_pGlowFilter);

					if (!CXR_Util::GaussianBlur(&GP, pTVBlur))
						break;
				}

				{
					CVec2Dfp32* pTVShrink = NULL;
					CXR_ShrinkParams SP;
					SP.m_SrcUVMin = UVMin;
					SP.m_SrcUVMax = UVMax;
					SP.m_DstUVMin = UVMin;
					SP.m_DstUVMax = UVMax;
					SP.m_nShrink = 2;
					SP.m_SrcPixelUV = pEngine->m_Screen_PixelUV;
					SP.m_RenderCaps = pEngine->m_RenderCaps_Flags;
					SP.m_Priority = 0;
					SP.m_pVBM = pVBM;
					SP.m_TextureID_Src = TextureID_Screen;
					SP.m_TextureID_Dst = TextureID_BlurScreen;
					SP.m_Rect = VPRect16;
					if (!CXR_Util::ShrinkTexture(&SP, pTVShrink))
						break;
				}

				{
					CRC_Attributes* pA = _pVBM->Alloc_Attrib();
					if (!pA)
						break;
					pA->SetDefault();
					pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_CULL);
					pA->Attrib_TextureID(0, TextureID_Screen);
					pA->Attrib_TextureID(1, m_TextureID_MotionMap);
					pA->Attrib_TextureID(2, TextureID_MotionMapBlur);
					pA->Attrib_TextureID(3, TextureID_BlurScreen);
					
					CRC_ExtAttributes_FragmentProgram20* pFP = (CRC_ExtAttributes_FragmentProgram20*)_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
					CVec4Dfp32* pFPParams = _pVBM->Alloc_V4(5);
					pFP->Clear();
					pFP->SetProgram("XREngine_MotionBlur", MHASH5('XREN','GINE','_MOT','IONB','LUR'));
					pFP->SetParameters(pFPParams, 5);
					pFPParams[0].Set(m_Screen_PixelUV[0], m_Screen_PixelUV[1], m_Screen_PixelUV[0], m_Screen_PixelUV[1]);
					fp32 mbr = swf * m_PostProcess_MBMaxRadius;
					pFPParams[1].Set(m_Screen_PixelUV[0] * mbr * 0.25f, m_Screen_PixelUV[1] * mbr * 0.25f, 0, 0);
					pFPParams[2].Set(m_Screen_PixelUV[0] * mbr * 0.50f, m_Screen_PixelUV[1] * mbr * 0.50f, 0, 0);
					pFPParams[3].Set(m_Screen_PixelUV[0] * mbr * 0.75f, m_Screen_PixelUV[1] * mbr * 0.75f, 0, 0);
					pFPParams[4].Set(m_Screen_PixelUV[0] * mbr * 1.00f, m_Screen_PixelUV[1] * mbr * 1.00f, 0, 0);

					pA->m_pExtAttrib = pFP;

					CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(_pVBM, pMat2D, VPRect16, 0xffffffff, 0.0f, pA);
					if (!pVB)
						break;

					CVec2Dfp32* pTV = (bRenderTextureVertFlip) ?
						CXR_Util::VBM_CreateRectUV_VFlip(pVBM, UVMin, UVMax) :
						CXR_Util::VBM_CreateRectUV(pVBM, UVMin, UVMax);
					pVB->Geometry_TVertexArray(pTV, 0);
					pVB->Geometry_TVertexArray(pTVBlur, 1);
					_pVBM->AddVB(pVB);
				}


				pVBM->AddCopyToTexture(0, VPRect, VPRect.p0, TextureID_Screen, false);
			}

			const int nHistograms = 8;
			fp32 M_ALIGN(16) lVisibleRatio[nHistograms];

			fp32 Levels = 1.0f;
			CVec3Dfp32 Exposure;
			Exposure.SetScalar(1.0f);

			// Shrink screen
			{
				int nShrink = 2;

				CVec2Dfp32* pTVShrink = NULL;
				if (bDoGlow || _pParams->m_bDynamicExposure)
				{
					CXR_ShrinkParams SP;
					SP.m_SrcUVMin = UVMin;
					SP.m_SrcUVMax = UVMax;
					SP.m_DstUVMin = UVMin;
					SP.m_DstUVMax = UVMax;
					SP.m_nShrink = nShrink;
					SP.m_SrcPixelUV = pEngine->m_Screen_PixelUV;
					SP.m_RenderCaps = pEngine->m_RenderCaps_Flags;
					SP.m_Priority = 0;
					SP.m_pVBM = pVBM;
					SP.m_TextureID_Src = TextureID_Screen;
					SP.m_TextureID_Dst = TextureID_BlurScreen;
					SP.m_Rect = VPRect16;
					if (!CXR_Util::ShrinkTexture(&SP, pTVShrink))
						break;
				}

				// Render histogram
				if (_pParams->m_bDynamicExposure)
				{
					fp32 BucketSize = 1.0f/64.0f;
					fp32 BucketOffset = 0.0f;

					CRC_Attributes* pAHist = (CRC_Attributes*) pVBM->Alloc(sizeof(CRC_Attributes) * nHistograms);
					CXR_PreRenderData_Histogram* pPreRender = (CXR_PreRenderData_Histogram*) pVBM->Alloc(sizeof(CXR_PreRenderData_Histogram) * nHistograms);
					CXR_PreRenderData_HistogramFeedback* pPreRenderFB = (CXR_PreRenderData_HistogramFeedback*) pVBM->Alloc(sizeof(CXR_PreRenderData_HistogramFeedback) );
					CXR_VertexBuffer_PreRender* pVBPreRender = (CXR_VertexBuffer_PreRender*) pVBM->Alloc(sizeof(CXR_VertexBuffer_PreRender) * nHistograms);
					CRC_ExtAttributes_FragmentProgram20* pFPHist = (CRC_ExtAttributes_FragmentProgram20*) pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20) * nHistograms);
					CVec4Dfp32* pFPParams = pVBM->Alloc_V4(nHistograms);
					if (!pAHist || !pPreRender || !pVBPreRender || !pPreRenderFB || !pFPHist || !pFPParams)
						break;

					CRect2Duint16 VPRectShrunk;
					VPRectShrunk.m_Min[0] = VPRect16.m_Min[0] >> nShrink;
					VPRectShrunk.m_Min[1] = VPRect16.m_Min[1] >> nShrink;
					VPRectShrunk.m_Max[0] = VPRect16.m_Max[0] >> nShrink;
					VPRectShrunk.m_Max[1] = VPRect16.m_Max[1] >> nShrink;

					static uint32 lnVisibleWriteBack[nHistograms] = { 0, 0, 0, 0, 0, 0, 0, 0 };	// Static so that this buffer can't disappear if the client gets zapped.

					uint32 M_ALIGN(16) lnVisible[nHistograms];

					for(int i = 0; i < nHistograms; i++)
					{
						pPreRender[i].m_QueryID = 0x80000000 + i;
						pVBPreRender[i].Create(pPreRender+i, CXR_PreRenderData_Histogram::RenderHistogram_PreRender, CXR_PreRenderData_Histogram::RenderHistogram_PostRender);

						CRC_Attributes* pA = pAHist + i;
						CRC_ExtAttributes_FragmentProgram20* pFP = pFPHist+i;
						pFP->Clear();
						pFP->SetProgram("XREngine_Histogram", MHASH5('XREN','GINE','_HIS','TOGR','AM'));
						pFP->SetParameters(pFPParams+i, 1);
						pA->SetDefault();
						pA->Attrib_ZCompare(CRC_COMPARE_ALWAYS);
						pA->Attrib_TextureID(0, TextureID_BlurScreen);
						pA->Attrib_AlphaCompare(CRC_COMPARE_GREATER, 0);
						pA->m_pExtAttrib = pFP;

						pFPParams[i].Set(BucketOffset, BucketSize, 0, 0);
						BucketOffset += BucketSize;
						BucketSize *= 2.0f;

						CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(pVBM, pMat2D, VPRectShrunk, 0xffffffff, 1, pA);
						if (!pVB)
							break;
						pVB->Geometry_TVertexArray(pTVShrink, 0);
						pVB->m_pPreRender = pVBPreRender+i;
						pVB->m_pAttrib = pA;
						pVB->SetVBEColor(0xff0080ff);
						pVBM->AddVB(pVB);

						//						lnVisible[i] = pEngine->m_pRender->OcclusionQuery_GetVisiblePixelCount(0x80000000+i);
					}

					pPreRenderFB->m_QueryID = 0x80000000;
					pPreRenderFB->m_nQueries = nHistograms;
					pPreRenderFB->m_pnVisiblePixels = lnVisibleWriteBack;
					pVBM->AddCallback(CXR_PreRenderData_HistogramFeedback::PreRender, pPreRenderFB, 0.0f, 0xff00ffff);

					{
						for(int i = 0; i < nHistograms; i++)
							lnVisible[i] = lnVisibleWriteBack[i];
					}

					uint32 nPixels = (VPRectShrunk.m_Max[0] - VPRectShrunk.m_Min[0]) * (VPRectShrunk.m_Max[1] - VPRectShrunk.m_Min[1]);
					{
						int Prev = 0;
						int Sum = 0;
						for(int i = nHistograms-1; i >= 0; i--)
						{
							int PrevTemp = lnVisible[i];
							lnVisible[i] = Max(0, int(lnVisible[i] - Prev));
							Prev = PrevTemp;
							Sum += lnVisible[i];
						}
						int nZero = nPixels - Sum;
						lnVisible[0] += nZero;

						vec128 vnpixelsrcp = M_VRcp(M_VCnv_i32_f32(M_VLdScalar_u32(nPixels)));

						for(int i = 0; i < nHistograms; i += 4)
						{
							vec128 ratio = M_VMul(vnpixelsrcp, M_VCnv_i32_f32(M_VLd(lnVisible+i)));
							M_VSt(ratio, lVisibleRatio+i);
						}
					}

					// Find power for x percent cut.
					{
						fp32 Cut = 0.0f;
						int nCutPixels = (nPixels * 38) >> 8;	// ~15%
						for(int i = nHistograms-1; i >= 0; i--)
						{
							if (lnVisible[i] >= nCutPixels)
							{
								Cut = fp32(i) + fp32(lnVisible[i]-nCutPixels) / fp32(lnVisible[i]);
								break;
							}
							nCutPixels -= lnVisible[i];
						}

						Levels = M_Pow(2.0f, Cut-6.0f);
						Exposure[0] = _pParams->m_ExposureScale[0] * M_Pow(Levels, -_pParams->m_ExposureExp[0]);
						Exposure[1] = _pParams->m_ExposureScale[1] * M_Pow(Levels, -_pParams->m_ExposureExp[1]);
						Exposure[2] = _pParams->m_ExposureScale[2] * M_Pow(Levels, -_pParams->m_ExposureExp[2]);
					}

					if (DebugFlags & XR_PPDEBUGFLAGS_TRACEHISTOGRAM)
						M_TRACEALWAYS("Histogram Pixels: %8d, %8d, %8d, %8d,%8d, %8d, %8d, %8d, Levels %f, %f\n", lnVisible[0], lnVisible[1], lnVisible[2], lnVisible[3], lnVisible[4], lnVisible[5], lnVisible[6], lnVisible[7], Levels, Exposure[0]);

					Levels -= _pParams->m_ExposureBlackLevel;
					m_PostProcess_PrevLevels = Levels;
				}
				else
				{
					// Dynamic exposure disabled
					m_PostProcess_PrevLevels = 0.5f;
				}
			}

			// Glow
			CVec2Dfp32* pTVBlur = NULL;
			if (bDoGlow)
			{
				CXR_GaussianBlurParams GP;
				GP.m_Rect = ShiftRect(VPRect16, -2);
				GP.m_Bias = _pParams->m_GlowBias;
				GP.m_Gamma = _pParams->m_GlowGamma;
				GP.m_UVCenter = _pParams->m_GlowCenter;
				GP.m_Scale = _pParams->m_GlowScale;
				GP.m_Exp = _pParams->m_GlowAttenuationExp;;
				GP.m_SrcUVMin = UVMin * 0.25f;
				GP.m_SrcUVMax = UVMax * 0.25f;
				GP.m_DstUVMin = GP.m_SrcUVMin;
				GP.m_DstUVMax = GP.m_SrcUVMax;
				GP.m_SrcPixelUV = pEngine->m_Screen_PixelUV;
				GP.m_DstPixelUV = pEngine->m_Screen_PixelUV;
				GP.m_pVBM = pVBM;
				GP.m_Priority = 0;
				GP.m_RenderCaps = pEngine->m_RenderCaps_Flags;
				GP.m_TextureID_Src = TextureID_BlurScreen;
				GP.m_TextureID_Dst = TextureID_BlurScreen;
				GP.m_Flags = DebugFlags & (XR_PPDEBUGFLAGS_NOGAUSS_X | XR_PPDEBUGFLAGS_NOGAUSS_Y);
				GP.m_nShrink = 0;
				GP.SetFilter(_pParams->m_pGlowFilter);

				if (!CXR_Util::GaussianBlur(&GP, pTVBlur))
					break;
			}
			else
				TextureID_BlurScreen = 0;

			// Color correction
			uint16 lTextureIDColorCorrection[4];
			uint16 TextureIDColorCorrectionTemp = m_spTCColorCorrectionTemp->GetTextureID(0);
			lTextureIDColorCorrection[0] = m_spTCColorCorrection->GetTextureID(0);
			lTextureIDColorCorrection[1] = m_spTCColorCorrection->GetTextureID(1);
			lTextureIDColorCorrection[2] = m_spTCColorCorrection->GetTextureID(2);
			lTextureIDColorCorrection[3] = m_spTCColorCorrection->GetTextureID(3);
			//
			{
				Engine_CreateColorCorrection(_pVBM, _pParams, TextureIDColorCorrectionTemp, lTextureIDColorCorrection);
			}

			// Reposition/stretch screen
			if (bDoFinal)
			{
				CVec2Dfp32* pTV = (bRenderTextureVertFlip) ? 
					CXR_Util::VBM_CreateRectUV_VFlip(pVBM, UVMin, UVMax) :
				CXR_Util::VBM_CreateRectUV(pVBM, UVMin, UVMax);
				CRC_Attributes* pA = pVBM->Alloc_Attrib();
				if (!pTV || !pA)
					break;

				const int nFPParams = 2;
				uint8* pFPMem = (uint8*) pVBM->Alloc(2 * (sizeof(CRC_ExtAttributes_FragmentProgram20) + sizeof(CVec4Dfp32)*nFPParams));

				CVec4Dfp32* pFPParams = (CVec4Dfp32*) pFPMem; pFPMem += nFPParams*sizeof(CVec4Dfp32);
				CRC_ExtAttributes_FragmentProgram20* pFP = (CRC_ExtAttributes_FragmentProgram20*) pFPMem; pFPMem += sizeof(CRC_ExtAttributes_FragmentProgram20);

				pA->SetDefault();
				pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
				CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(pVBM, pMat2D, VPRectScreen16, 0xffffffff, 1, pA);
				if (!pVB)
					break;

				if (bShowFinalGlow)
				{
					pVB->Geometry_TVertexArray(pTVBlur, 0);
					pA->Attrib_TextureID(0, TextureID_BlurScreen);
				}
				else
				{
					pFPParams[0][0] = Exposure[0];
					pFPParams[0][1] = Exposure[1];
					pFPParams[0][2] = Exposure[2];
					pFPParams[0][3] = Levels;
					pFPParams[1][0] = _pParams->m_ExposureContrast;
					pFPParams[1][1] = _pParams->m_ExposureSaturation;
					pFPParams[1][2] = _pParams->m_ExposureBlackLevel;
					pFPParams[1][3] = 0;

					pFP->Clear();
					if (pEngine->GetShader()->GetShaderModeTraits() & XR_SHADERMODETRAIT_LINEARCOLORSPACE)
						pFP->SetProgram("XREngine_FinalLinear", 0);
					else
					{
						if (_pParams->m_bDynamicExposure)
							pFP->SetProgram("XREngine_Final", 0);
						else
							pFP->SetProgram("XREngine_FinalNoExposure", 0);
					}
					pFP->SetParameters(pFPParams, nFPParams);
					pA->m_pExtAttrib = pFP;

					pA->Attrib_TextureID(0, TextureID_Screen);
					if (bDoGlow)
						pA->Attrib_TextureID(1, TextureID_BlurScreen);
					else
						pA->Attrib_TextureID(1, pEngine->GetShader()->m_TextureID_Special_ff000000);

					pA->Attrib_TextureID(2, lTextureIDColorCorrection[0]);

					pVB->Geometry_TVertexArray(pTV, 0);
					pVB->Geometry_TVertexArray(pTVBlur, 1);
				}
				pVBM->AddVB(pVB);
			}

			// Render borders
			if (_pParams->m_ViewFlags & XR_VIEWFLAGS_WIDESCREEN)
			{
				CRect2Duint16 VPRectTop(VPRectScreen16);
				CRect2Duint16 VPRectBottom(VPRectScreen16);
				VPRectTop.m_Max[1] = VPRectTop.m_Min[1];
				VPRectTop.m_Min[1] = 0;
				VPRectBottom.m_Min[1] = VPRectBottom.m_Max[1];
				VPRectBottom.m_Max[1] = ScreenSize.y;

				CRC_Attributes* pA = pVBM->Alloc_Attrib();
				if (!pA)
					break;

				pA->SetDefault();
				pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
				CXR_VertexBuffer* pVB1 = CXR_Util::VBM_RenderRect(pVBM, pMat2D, VPRectTop, 0, 1, pA);
				if (!pVB1)
					break;
				pVBM->AddVB(pVB1);

				CXR_VertexBuffer* pVB2 = CXR_Util::VBM_RenderRect(pVBM, pMat2D, VPRectBottom, 0, 1, pA);
				if (!pVB2)
					break;
				pVBM->AddVB(pVB2);
			}

			// Render histogram
#if 1
			if (DebugFlags & XR_PPDEBUGFLAGS_EXPOSUREDEBUG)
			{
				CRC_Font* pF = m_spDebugFont;
				if (!pF)
					break;

				CRC_Attributes* pA = pVBM->Alloc_Attrib(2);
				if (!pA)
					break;

				pA->SetDefault();
				pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
				pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
				CRC_Attributes* pAText = pA+1;
				*pAText = *pA;
				CXR_Util::Text_PatchAttribute(pF, pAText);

				{
					int posx = VPRect.p1.x - 194 - 20;
					int posy = VPRect.p1.y - 130 - 36 - 20;
					fp32 posxf = posx;
					fp32 posyf = posy;

					CClipRect cr(VPRect);

					CXR_Util::VBM_RenderRect(pVBM, pMat2D, CRect2Duint16(posx-2, posy-2, posx+198, posy+130+36+4), 0x80000000, 0.0f, pA);
					CFStr Text;
					if (_pParams->m_bDynamicExposure)
						Text = CFStrF("Exposure: %.4f, %.4f, %.4f", Exposure[0], Exposure[1], Exposure[2]);
					else
						Text = CFStrF("Exposure: Disabled");
					CXR_Util::Text_Add(pVBM, pF, pMat2D, CVec2Dfp32(posxf+4.0f, posyf+2.0f), Text.Str(), 0xff80ff80, pAText);
					Text = CFStrF("Levels: %f", Levels);
					CXR_Util::Text_Add(pVBM, pF, pMat2D, CVec2Dfp32(posxf+4.0f, posyf+12.0f), Text.Str(), 0xff80ff80, pAText);
					CXR_Util::VBM_RenderRect(pVBM, pMat2D, CRect2Duint16(posx, posy+64, posx+RoundToInt(Levels*64.0f), posy+128), 0x80008080, 0.0f, pA);
					

					const int GraphSamples = 192;	// Must be 16n
					const int MaxLines = GraphSamples*2 + 10;
					CVec2Dfp32 lp0[MaxLines];
					CVec2Dfp32 lp1[MaxLines];
					CPixel32 lCol0[MaxLines];
					CPixel32 lCol1[MaxLines];
					int nLines = 0;

					fp32 M_ALIGN(16) lExp[GraphSamples];
					fp32 M_ALIGN(16) lExpGamma[GraphSamples];
					{
						fp32 fx = 0.0f;
						for(int x = 0; x < GraphSamples; x++)
						{
							lExp[x] = 1.0f - M_Exp(-fx * Exposure[0]);
							fx += 2.0f / 128.0f;
						}
					}
					{
						for(int x = 0; x < GraphSamples; x += 16)
						{
							vec128 r0 = M_VSqrt(M_VLd(lExp + x));
							vec128 r1 = M_VSqrt(M_VLd(lExp + x + 4));
							vec128 r2 = M_VSqrt(M_VLd(lExp + x + 8));
							vec128 r3 = M_VSqrt(M_VLd(lExp + x + 12));
							M_VSt(r0, lExpGamma + x);
							M_VSt(r1, lExpGamma + x + 4);
							M_VSt(r2, lExpGamma + x + 8);
							M_VSt(r3, lExpGamma + x + 12);
						}
					}
					{
						lp0[nLines].Set(posxf, posyf);			lp1[nLines].Set(posxf, posyf+128.0f);
						lCol0[nLines] = 0x40ffffff;				lCol1[nLines] = 0x40ffffff;
						nLines++;

						lp0[nLines].Set(posxf, posyf+128.0f);		lp1[nLines].Set(posxf + 196.0f, posyf+128.0f);
						lCol0[nLines] = 0x40ffffff;				lCol1[nLines] = 0x40ffffff;
						nLines++;

						lp0[nLines].Set(posxf, posyf+128.0f);		lp1[nLines].Set(posxf + 128.0f, posyf);
						lCol0[nLines] = 0x40ffffff;				lCol1[nLines] = 0x40ffffff;
						nLines++;

						lp0[nLines].Set(posxf, posyf+64.0f);		lp1[nLines].Set(posxf + 160.0f, posyf+64.0f);
						lCol0[nLines] = 0x40ffffff;				lCol1[nLines] = 0x40ffffff;
						nLines++;

						lp0[nLines].Set(posxf+64.0f, posyf+32.0f);lp1[nLines].Set(posxf+64.0f, posyf+128.0f);
						lCol0[nLines] = 0x40ffffff;				lCol1[nLines] = 0x40ffffff;
						nLines++;

						if (_pParams->m_bDynamicExposure)
						{
							fp32 fx = 0.0f;
							for(int x = 0; x < GraphSamples-1; x++)
							{
								lp0[nLines].Set(posxf + fx, posyf + 64.0f*(2.0f - lExpGamma[x]));
								lp1[nLines].Set(posxf + fx + 1.0f, posyf + 64.0f*(2.0f - lExpGamma[x+1]));
								lCol0[nLines] = 0xff0080c0;
								lCol1[nLines] = 0xff0080c0;
								nLines++;

								lp0[nLines].Set(posxf + fx, posyf + 64.0f*(2.0f - lExp[x]));
								lp1[nLines].Set(posxf + fx + 1.0f, posyf + 64.0f*(2.0f - lExp[x+1]));
								lCol0[nLines] = 0xffff8000;
								lCol1[nLines] = 0xffff8000;
								nLines++;
								fx += 1;
							}
						}
					}

					CXR_VertexBuffer* pVBLines = CXR_Util::VBM_RenderWires2D(pVBM, cr, lp0, lp1, lCol0, lCol1, nLines, pA);
					if (pVBLines)
						pVBM->AddVB(pVBLines);


					if (_pParams->m_bDynamicExposure)
					{
						int x = posx;
						int y = posy+128+4;
						int w = 1;

						for(int i = 0; i < nHistograms; i++)
						{
							CRect2Duint16 Rect;
							Rect.m_Min[0] = x;
							Rect.m_Max[0] = x + w;
							Rect.m_Min[1] = y + RoundToInt(32.0f * (1.0f - lVisibleRatio[i]));
							Rect.m_Max[1] = y + 32;
							uint32 Color = (i & 1) ? 0xffff8000 : 0xffc06000;
							CXR_VertexBuffer* pVB1 = CXR_Util::VBM_RenderRect(pVBM, pMat2D, Rect, Color, 1, pA);
							if (!pVB1)
								break;
							pVBM->AddVB(pVB1);

							Rect.m_Max[1] = Rect.m_Min[1]-1;
							Rect.m_Min[1] = y;
							CXR_VertexBuffer* pVB2 = CXR_Util::VBM_RenderRect(pVBM, pMat2D, Rect, 0x7f000000, 1, pA);
							if (!pVB2)
								break;
							pVBM->AddVB(pVB2);

							x += w;
							w *= 2;
						}
						{
							int x = posx+192 - nHistograms*4;
							int y = posy+128+4;
							for(int i = 0; i < nHistograms; i++)
							{
								CRect2Duint16 Rect;
								Rect.m_Min[0] = x + i*4;
								Rect.m_Max[0] = x + i*4 + 3;
								Rect.m_Min[1] = y + RoundToInt(32.0f * (1.0f - lVisibleRatio[i]));
								Rect.m_Max[1] = y + 32;
								CXR_VertexBuffer* pVB1 = CXR_Util::VBM_RenderRect(pVBM, pMat2D, Rect, 0x8000ffff, 1, pA);
								if (!pVB1)
									break;
								pVBM->AddVB(pVB1);
							}
						}
					}
				}
			}
#endif
		}

	}
	while(0);

	pVBM->ScopeEnd();
}


void CXR_EngineImpl::Engine_SetRenderContext(CRenderContext* _pRC)
{
	MAUTOSTRIP(CXR_EngineImpl_Engine_SetRenderContext, MAUTOSTRIP_VOID);
	m_pRender = _pRC;
	if (!m_pRender)
		Error("Engine_Render", "Invalid render target.");

	m_RenderCaps_Flags = _pRC->Caps_Flags();
	m_RenderCaps_nMultiTexture = _pRC->Attrib_GlobalGetVar(CRC_GLOBALVAR_NUMTEXTURES);
	m_RenderCaps_nMultiTextureEnv =  _pRC->Attrib_GlobalGetVar(CRC_GLOBALVAR_NUMTEXTUREENV);
	m_RenderCaps_nMultiTextureCoords =  _pRC->Attrib_GlobalGetVar(CRC_GLOBALVAR_NUMTEXTURECOORDS);

	m_SurfCaps = 0;
	m_SurfCaps |= (m_RenderCaps_nMultiTexture >= 2) ? XW_SURFREQ_MULTITEX2 : 0;
	m_SurfCaps |= (m_RenderCaps_nMultiTexture >= 3) ? XW_SURFREQ_MULTITEX3 : 0;
	m_SurfCaps |= (m_RenderCaps_nMultiTexture >= 4) ? XW_SURFREQ_MULTITEX4 : 0;

	if (m_RenderCaps_Flags & CRC_CAPS_FLAGS_GAMECUBE)
		m_SurfCaps |= XW_SURFREQ_GAMECUBE;

	if (m_RenderCaps_Flags & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV10)
		m_SurfCaps |= XW_SURFREQ_NV10 | XW_SURFREQ_TEXENVMODE2;

	if (m_RenderCaps_Flags & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20)
		m_SurfCaps |= XW_SURFREQ_NV20;

	if (m_RenderCaps_Flags & CRC_CAPS_FLAGS_CUBEMAP)
		m_SurfCaps |= XW_SURFREQ_CUBEMAP;

	if (m_RenderCaps_Flags & CRC_CAPS_FLAGS_TEXENVMODE2)
		m_SurfCaps |= XW_SURFREQ_TEXENVMODE2;

	if (m_bGeneratedSurfaces)
		m_SurfCaps |= XW_SURFREQ_GENERATED;

	if( m_EngineMode == 2 || m_EngineMode == 0 )
		m_SurfOptions |= XW_SURFOPTION_LIGHTMAP;
	else
		m_SurfOptions &= ~XW_SURFOPTION_LIGHTMAP;

	if( m_EngineMode == 1 )
		m_SurfOptions |= XW_SURFOPTION_SHADER;
	else
		m_SurfOptions &= ~XW_SURFOPTION_SHADER;
}

void CXR_EngineImpl::Engine_InitRenderTextures()
{
	CRC_Viewport* pVP = m_pVBM->Viewport_Get();

	m_iNextPortalTexture = 0;
	int MaxSquarePortal;
	int MaxSquareShadowDecal;
	
	m_bTLEnableRenderer = true; //!(_pRC->Caps_Flags() & CRC_CAPS_FLAGS_SLOWTL);
	m_bTLEnableEnabled = m_bTLEnableEngine & m_bTLEnableRenderer;

	CPnt ScrSize = m_pRender->GetDC()->GetScreenSize();

	if (m_RenderCaps_Flags & CRC_CAPS_FLAGS_OFFSCREENTEXTURERENDER)
	{
		MaxSquarePortal = m_PortalTextureSize;
		MaxSquareShadowDecal = m_ShadowDecalTextureSize;
	}
	else
	{
		MaxSquarePortal = Min(m_PortalTextureSize, GetLEPow2(Min(ScrSize.x, ScrSize.y)));
		MaxSquareShadowDecal = Min(m_ShadowDecalTextureSize, GetLEPow2(Min(ScrSize.x, ScrSize.y)));
	}

	CPnt TxtSize = m_pRender->GetDC()->GetMaxWindowSize();
//TxtSize = CPnt(1600,1200);
//	int TxtW;
//	int TxtH;
	if (m_RenderCaps_Flags & CRC_CAPS_FLAGS_ARBITRARY_TEXTURE_SIZE)
	{
//		TxtW = ScrSize.x;
//		TxtH = ScrSize.y;
	}
	else
	{
		TxtSize.x = GetGEPow2(TxtSize.x);
		TxtSize.y = GetGEPow2(TxtSize.y);
	}

	m_Screen_Size.Set(ScrSize.x, ScrSize.y);
	m_Screen_TextureSize.Set(TxtSize.x, TxtSize.y);
	m_spTCPortals->PrepareFrame(CPnt(MaxSquarePortal, MaxSquarePortal));	
#ifdef XR_SUPPORT_SHADOWDECALS
	m_spTCShadowDecals->PrepareFrame(CPnt(MaxSquareShadowDecal, MaxSquareShadowDecal));
#endif
	m_spTCScreen->PrepareFrame(TxtSize);
	m_spTCDepthStencil->PrepareFrame(TxtSize);
	m_spTCColorCorrection->PrepareFrame(CPnt(128, 128));
	m_spTCColorCorrectionTemp->PrepareFrame(CPnt(256,256));

	m_TextureID_Screen = m_spTCScreen->GetTextureID(0);
	m_TextureID_Depth = m_spTCScreen->GetTextureID(1);
	m_TextureID_ResolveScreen = m_spTCScreen->GetTextureID(2);
	m_TextureID_DeferredNormal = m_spTCScreen->GetTextureID(3);
	m_TextureID_DeferredDiffuse = m_spTCScreen->GetTextureID(4);
	m_TextureID_DeferredSpecular = m_spTCScreen->GetTextureID(5);
	m_TextureID_PostProcess = m_spTCScreen->GetTextureID(6);
	m_TextureID_MotionMap = m_spTCScreen->GetTextureID(7);

	m_TextureID_DepthStencil = m_spTCDepthStencil->GetTextureID(0);
	m_TextureID_ShadowMask = m_spTCDepthStencil->GetTextureID(1);

	m_TextureID_DefaultEnvMap = m_pTC->GetTextureID(MHASH6('SPEC','IAL_','DEFA','ULTE','NVMA','P'));

	Engine_InitRenderTextureProjection();
}

void CXR_EngineImpl::Engine_InitRenderTextureProjection()
{
	// Init texture projection matrix and pixel UV scale
	CRC_Viewport* pVP = m_pVBM->Viewport_Get();

	{
		CRct Area = pVP->GetViewArea();
		fp32 sw = fp32(m_Screen_Size[0]);
		fp32 sh = fp32(m_Screen_Size[1]);
		fp32 xs = 0.5 * pVP->GetXScale() / sw;
		fp32 ys = 0.5 * pVP->GetYScale() / sh;

		m_Screen_PixelUV[0] = 1.0f / fp32(m_Screen_TextureSize[0]);
		m_Screen_PixelUV[1] = 1.0f / fp32(m_Screen_TextureSize[1]);

		xs *= (sw * m_Screen_PixelUV[0]);
		ys *= (sh * m_Screen_PixelUV[1]);

		fp32 umaxvp = fp32(Area.GetWidth()) * m_Screen_PixelUV[0];
		fp32 vmaxvp = fp32(Area.GetHeight()) * m_Screen_PixelUV[1];
		fp32 xofs = -(1.0f - umaxvp)*0.5;
		fp32 yofs = -(1.0f - vmaxvp)*0.5;

		if (m_RenderCaps_Flags & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP)
		{
			yofs = -yofs;
			ys = -ys;
		}

		CMat4Dfp32& Proj = m_Screen_TextureProjection;
		Proj.k[0][0] = xs;		Proj.k[0][1] = 0.0;		Proj.k[0][2] = 0.0;		Proj.k[0][3] = 0.0;
		Proj.k[1][0] = 0.0;		Proj.k[1][1] = ys;		Proj.k[1][2] = 0.0;		Proj.k[1][3] = 0.0;
		Proj.k[2][0] = 0.5+xofs;Proj.k[2][1] = 0.5+yofs;Proj.k[2][2] = 0;		Proj.k[2][3] = 1.0;
		Proj.k[3][0] = 0;		Proj.k[3][1] = 0.0;		Proj.k[3][2] = 0.0f;	Proj.k[3][3] = 0.0;
	}
}

void CXR_EngineImpl::Engine_ClearRenderTextures()
{
	m_TextureID_Screen = 0;
	m_TextureID_Depth = 0;
	m_TextureID_ResolveScreen = 0;
	m_TextureID_PostProcess = 0;			// This should be removed
	m_TextureID_DeferredNormal = 0;			// Exp, N.g, N.b, N.alpha
	m_TextureID_DeferredDiffuse = 0;		// Diff.r, Diff.g, Diff.b, Diff.blend
	m_TextureID_DeferredSpecular = 0;		// Spec.r, Spec.g, Spec.b, Spec.blend

	m_TextureID_DepthStencil = 0;
	m_TextureID_ShadowMask = 0;
}

void CXR_EngineImpl::Engine_Render(CXR_EngineClient *_pClient, CXR_VBManager* _pVBM, CRenderContext* _pRC, const CMat4Dfp32& _CameraWMat, const CMat4Dfp32& _CameraWVelMat, CXR_SceneGraphInstance* _pSceneGraphInstance, class CXR_TriangleMeshDecalContainer* _pTMDC)
{
	MAUTOSTRIP(CXR_EngineImpl_Engine_Render, MAUTOSTRIP_VOID);
	MSCOPE(CXR_EngineImpl::Engine_Render, XR_ENGINE);
	M_NAMEDEVENT("Engine_Render", 0xffff0000);

	m_pVBM	= _pVBM;
	m_pClient = _pClient;

	Engine_SetRenderContext(_pRC);
	m_pSceneGraphInstance = _pSceneGraphInstance;
	m_pTMDC = _pTMDC;
	m_PostProcessParams_Current = m_PostProcessParams;

	m_iCurrentVC = 0;
	m_nRecurse = 0;
	m_bCurrentVCMirrored = false;

	CRC_Viewport* pVP = m_pVBM->Viewport_Get();

	CPlane3Dfp32 FrontPlaneW;
	FrontPlaneW.CreateND(CVec3Dfp32(0,0,1), -pVP->GetBackPlane());
	FrontPlaneW.Transform(_CameraWMat);

	CRC_ClipVolume Clip;
	pVP->GetClipVolume(Clip);

	Engine_InitRenderTextures();

	if (!m_Shader.OnPrepareFrame() && (m_EngineMode == XR_MODE_UNIFIED))
	{
		ConOutL("§cf80WARNING: Insufficient renderer feature set to prepare shader.");
	}
	else
	{
		Engine_BuildViewContext(_CameraWMat, _CameraWVelMat, CVec3Dfp32::GetMatrixRow(_CameraWMat, 3), Clip, FrontPlaneW, XR_SCENETYPE_MAIN);

#ifdef XR_SUPPORT_SHADOWDECALS
		if (m_spTCShadowDecals != NULL && m_spTCShadowDecals->Shadow_GetNumUsed() > 0)
		{
			MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
			if (pSys->m_spDisplay != NULL) pSys->m_spDisplay->ClearFrameBuffer();
		}
#endif

		Engine_RenderViewContext();
	}

	Engine_ClearRenderTextures();

	m_pSceneGraphInstance = NULL;
	m_pTMDC = NULL;
	m_pViewClip = NULL;

	if (GetVC()->m_pViewClip)
		GetVC()->m_pViewClip->View_SetCurrent(0, NULL);

	// -------------------------------------------------------------------

	if (m_bShowRecurse) 
		ConOut(CStrF("RenderComplete, %d passes, %d texture-portals", m_nRecurse, m_iNextPortalTexture));

	m_pVBM	= NULL;
	m_pClient = NULL;
	m_bCurrentVCMirrored = false;
}

void CXR_EngineImpl::Engine_ClearReferences()
{
	for(int i = 0; i < m_lspVC.Len(); i++)
		m_lspVC[i]->ClearReferences();
}

/*
void CXR_EngineImpl::SetDefaultAttrib(CRC_Attributes* _pAttr)
{
	MAUTOSTRIP(CXR_EngineImpl_SetDefaultAttrib, MAUTOSTRIP_VOID);
	if (GetVC()->m_bIsMirrored)
		_pAttr->Attrib_Enable(CRC_FLAGS_CULLCW);
	else
		_pAttr->Attrib_Disable(CRC_FLAGS_CULLCW);
}*/

CObj* CXR_EngineImpl::GetInterface(int _Var)
{
	MAUTOSTRIP(CXR_EngineImpl_GetInterface, NULL);
	switch(_Var)
	{
#ifdef XR_SUPPORT_SHADOWDECALS
	case XR_ENGINE_TCSHADOWDECALS :			return (m_bNoRenderToTexture || !m_bShadowDecals) ? (CObj*)NULL : (CObj*)m_spTCShadowDecals;
#endif
	case XR_ENGINE_TCSCREEN :				return m_spTCScreen;

	default : return NULL;
	}
}

int CXR_EngineImpl::GetVar(int _Var)
{
	MAUTOSTRIP(CXR_EngineImpl_GetVar, 0);
	switch(_Var)
	{
	case XR_ENGINE_MODE : return m_EngineMode;
	case XR_ENGINE_FLARES : return m_bFlares;
	case XR_ENGINE_DYNLIGHT : return m_bDynLight;
	case XR_ENGINE_FASTLIGHT : return m_bFastLight;
	case XR_ENGINE_WALLMARKS : return m_bWallmarks;
	case XR_ENGINE_LODOFFSET : return (int)m_LODOffset;
	case XR_ENGINE_LODSCALE : return (int)m_lspVC[m_iCurrentVC]->m_LODScale;
	case XR_ENGINE_VBSKIP : return m_VBSkip;
	case XR_ENGINE_ALLOWDEPTHFOG : return m_bAllowDepthFog;
	case XR_ENGINE_ALLOWVERTEXFOG : return m_bAllowVertexFog;
	case XR_ENGINE_ALLOWNHF : return m_bAllowNHF;
	case XR_ENGINE_SURFOPTIONS : return m_SurfOptions;
	case XR_ENGINE_SURFCAPS : return m_SurfCaps;
	case XR_ENGINE_STENCILSHADOWS : return m_bStencilShadows;
	case XR_ENGINE_SHADOWDECALS : return m_ShadowDecals;
	case XR_ENGINE_NORENDERTOTEXTURE : return m_bNoRenderToTexture;
	case XR_ENGINE_UNIFIED_AMBIENCE : return m_Unified_Ambience;
	case XR_ENGINE_PBNIGHTVISIONLIGHT : return m_PBNightVisionLight;
	case XR_ENGINE_FOGCULLOFFSET : return (int)((m_FogCullOffsetOverride) ? m_FogCullOffsetOverride : m_FogCullOffset);
	case XR_ENGINE_CLEARVIEWPORT : return m_bClearViewport;
#ifdef M_Profile
	case XR_ENGINE_MINAVAILABLEVBHEAP: return m_pVBM ? m_pVBM->GetMinAvail() : 0; 
#endif

	case 0x2345 : return m_bTLEnableEnabled;
	case 0x2346 : return m_ShowVBTypes;

	default : return 0;
	}
}

fp32 CXR_EngineImpl::GetVarf(int _Var)
{
	MAUTOSTRIP(CXR_EngineImpl_GetVarf, 0.0f);
	switch(_Var)
	{
	case XR_ENGINE_LIGHTSCALE :	return m_LightScale;
	case XR_ENGINE_LODOFFSET : return m_LODOffset;
	case XR_ENGINE_LODSCALE : return m_lspVC[m_iCurrentVC]->m_LODScale;
	case XR_ENGINE_FOGCULLOFFSET : return (m_FogCullOffsetOverride) ? m_FogCullOffsetOverride : m_FogCullOffset;
	default : return GetVar(_Var);
	}
}

void CXR_EngineImpl::SetVar(int _Var, aint _Value)
{
	MAUTOSTRIP(CXR_EngineImpl_SetVar, MAUTOSTRIP_VOID);
	switch(_Var)
	{
	case XR_ENGINE_MODE : m_EngineMode = _Value; break;
	case XR_ENGINE_FLARES : m_bFlares = _Value; break;
	case XR_ENGINE_DYNLIGHT : m_bDynLight = _Value;  break;
	case XR_ENGINE_FASTLIGHT : m_bFastLight = _Value; break;
	case XR_ENGINE_WALLMARKS : m_bWallmarks = _Value; break;
	case XR_ENGINE_SHADOWDECALS : m_ShadowDecals = _Value; break;
	case XR_ENGINE_LODOFFSET : m_LODOffset = _Value; break;
//	case XR_ENGINE_LODSCALE : m_LODScale =  _Value; break;
	case XR_ENGINE_VBSKIP : m_VBSkip = MaxMT(0, _Value);  break;
	case XR_ENGINE_ALLOWDEPTHFOG : m_bAllowDepthFog = _Value != 0; break;
	case XR_ENGINE_ALLOWVERTEXFOG : m_bAllowVertexFog = _Value != 0; break;
	case XR_ENGINE_ALLOWNHF : m_bAllowNHF = _Value != 0; break;
	case XR_ENGINE_SURFOPTIONS : m_SurfOptions = _Value; break;
	case XR_ENGINE_SURFCAPS : m_SurfCaps = _Value; break;
	case XR_ENGINE_STENCILSHADOWS : m_bStencilShadows = _Value; break;
	case XR_ENGINE_SETENGINECLIENT : m_pClient = (CXR_EngineClient *)_Value; break;
	case XR_ENGINE_NORENDERTOTEXTURE : m_bNoRenderToTexture = _Value != 0; break;
	case XR_ENGINE_FOGCULLOFFSET : m_FogCullOffset = _Value; break;
	case XR_ENGINE_CLEARVIEWPORT : m_bClearViewport = _Value; break;
	case XR_ENGINE_PBNIGHTVISIONLIGHT : m_PBNightVisionLight = _Value; break;
			
	case XR_ENGINE_LIGHTSCALE :
		{
			m_LightScale = _Value;
			m_LightScaleRecp = 1.0f / m_LightScale;
			break;
		}

	case XR_ENGINE_UNIFIED_ADDLIGHTOCCLUSION :
		{
			ConOut(CStr("§cf80WARNING: (CXR_EngineImpl::SetVar XR_ENGINE_UNIFIED_ADDLIGHTOCCLUSION) NOP"));

			break;
		}
	}

//	fp_UpdateEnvironment();
}

void CXR_EngineImpl::SetVarf(int _Var, fp32 _Value)
{
	MAUTOSTRIP(CXR_EngineImpl_SetVarf, MAUTOSTRIP_VOID);
	switch(_Var)
	{
	case XR_ENGINE_LIGHTSCALE :
		{
			m_LightScale = _Value;
			m_LightScaleRecp = 1.0f / m_LightScale;
			break;
		}
	case XR_ENGINE_FOGCULLOFFSET : m_FogCullOffset = _Value; break;
	default :
		SetVar(_Var, (aint)_Value);
	}

//	fp_UpdateEnvironment();
}


CMTime CXR_EngineImpl::GetEngineTime()
{
	return m_RenderTime;
}

void CXR_EngineImpl::SetEngineTime(CMTime &_Time)
{
	m_RenderTime = _Time;
}


void CXR_EngineImpl::Con_DebugFlags(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_DebugFlags, MAUTOSTRIP_VOID);
	m_DebugFlags = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_LightDebugFlags(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_LightDebugFlags, MAUTOSTRIP_VOID);
	m_LightDebugFlags = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_Flares(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_Flares, MAUTOSTRIP_VOID);
	m_bFlares = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_DLight(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_DLight, MAUTOSTRIP_VOID);
	m_bDynLight = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_FastLight(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_FastLight, MAUTOSTRIP_VOID);
	m_bFastLight = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_Wallmarks(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_Wallmarks, MAUTOSTRIP_VOID);
	m_bWallmarks = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_ShadowDecals(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_ShadowDecals, MAUTOSTRIP_VOID);
	m_ShadowDecals = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_LODOffset(fp32 _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_LODOffset, MAUTOSTRIP_VOID);
	m_LODOffset = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_LODScaleBias(fp32 _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_LODScaleBias, MAUTOSTRIP_VOID);
	m_LODScaleBias = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_ShowBoundings(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_ShowBoundings, MAUTOSTRIP_VOID);
	m_bShowBoundings = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_WorldOnly(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_WorldOnly, MAUTOSTRIP_VOID);
	m_bWorldOnly = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_ObjectsOnly(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_ObjectsOnly, MAUTOSTRIP_VOID);
	m_bObjectsOnly = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_PortalsOnly(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_PortalsOnly, MAUTOSTRIP_VOID);
	m_bPortalsOnly = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_Sky(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_Sky, MAUTOSTRIP_VOID);
	m_bSky = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_ShowTiming(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_ShowTiming, MAUTOSTRIP_VOID);
	m_ShowTiming = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_ShowVBTime(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_ShowVBTime, MAUTOSTRIP_VOID);
	m_VBMFlags = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_ShowFogInfo(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_ShowFogInfo, MAUTOSTRIP_VOID);
	m_bShowFogInfo = _v != 0;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_ShowRecurse(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_ShowRecurse, MAUTOSTRIP_VOID);
	m_bShowRecurse = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_ShowPortalFence(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_ShowPortalFence, MAUTOSTRIP_VOID);
	m_bShowPortalFence = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_VertexFog(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_VertexFog, MAUTOSTRIP_VOID);
	m_bAllowVertexFog = _v != 0;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_DepthFog(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_DepthFog, MAUTOSTRIP_VOID);
	m_bAllowDepthFog = _v != 0;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_NHF(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_NHF, MAUTOSTRIP_VOID);
	m_bAllowNHF = _v != 0;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_SurfOptions(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_SurfOptions, MAUTOSTRIP_VOID);
	m_SurfOptions = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_StencilShadows(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_StencilShadows, MAUTOSTRIP_VOID);
	m_bStencilShadows = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_PortalTextureSize(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_PortalTextureSize, MAUTOSTRIP_VOID);
	_v = GetLEPow2(Max(1, Min(1024, _v)));
	m_PortalTextureSize = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_ShadowDecalTextureSize(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_ShadowDecalTextureSize, MAUTOSTRIP_VOID);
	_v = GetLEPow2(Max(1, Min(1024, _v)));
	m_ShadowDecalTextureSize = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_UnifiedClear(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_UnifiedClear, MAUTOSTRIP_VOID);
	m_Unified_Clear = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_UnifiedAmbience(int _r, int _g, int _b)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_UnifiedAmbience, MAUTOSTRIP_VOID);
	m_Unified_Ambience = CPixel32(_r, _g, _b, 255);

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_TempTLEnable(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_TempTLEnable, MAUTOSTRIP_VOID);
	m_bTLEnableEngine = _v != 0;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_ShowVBTypes(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_ShowVBTypes, MAUTOSTRIP_VOID);
	m_ShowVBTypes = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_GeneratedSurfaces(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_GeneratedSurfaces, MAUTOSTRIP_VOID);
	m_bGeneratedSurfaces = _v != 0;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_DrawMode(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_DrawMode, MAUTOSTRIP_VOID);
	m_DrawMode = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_Freeze(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_Freeze, MAUTOSTRIP_VOID);
	m_bFreeze = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_ClearViewport(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_ClearViewport, MAUTOSTRIP_VOID);
	m_bClearViewport = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_FogCullOffset(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_FogCullOffset, MAUTOSTRIP_VOID);
	m_FogCullOffsetOverride = _v;

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_XRModeOverride(int _v)
{
	MAUTOSTRIP(CXR_EngineImpl_Con_XRModeOverride, MAUTOSTRIP_VOID);

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	CRegistry* pReg = (pSys) ? pSys->GetEnvironment() : NULL;
	if( pReg )
	{
		int val = (_v==2)?2:1;
		pReg->SetValue( "XR_MODEOVERRIDE", CStrF("%d", val ) );
		ConOut(CStrF("XR_MODEOVERRIDE changed to %d", val ));
	}

	fp_UpdateEnvironment();
}

void CXR_EngineImpl::Con_SyncOnRefresh(int _v)
{
	m_bSyncOnRender = _v != 0;
	ConOutL(CStrF("OnRender calls now %s", _v?"Sync":"ASync"));
}

void CXR_EngineImpl::Con_PPDebugFlags(int _v)
{
	m_PostProcessParams.m_DebugFlags = _v;
}

void CXR_EngineImpl::Con_PPGlowExp(fp32 _Exp)
{
	m_PostProcessParams.m_GlowAttenuationExp = _Exp;
}

void CXR_EngineImpl::Con_PPGlowBias(fp32 _r, fp32 _g, fp32 _b, fp32 _a)
{
	m_PostProcessParams.m_GlowBias = CVec4Dfp32(_r, _g, _b, _a);
}

void CXR_EngineImpl::Con_PPGlowScale(fp32 _r, fp32 _g, fp32 _b)
{
	m_PostProcessParams.m_GlowScale = CVec3Dfp32(_r, _g, _b);
}

void CXR_EngineImpl::Con_PPGlowGamma(fp32 _r, fp32 _g, fp32 _b)
{
	m_PostProcessParams.m_GlowGamma = CVec3Dfp32(_r, _g, _b);
}

void CXR_EngineImpl::Con_PPGlowCenter(fp32 _u, fp32 _v)
{
	m_PostProcessParams.m_GlowCenter = CVec2Dfp32(_u, _v);
}

void CXR_EngineImpl::Con_PPExposureExp(fp32 _r, fp32 _g, fp32 _b)
{
	m_PostProcessParams.m_ExposureExp.Set(_r, _g, _b);
}

void CXR_EngineImpl::Con_PPExposureScale(fp32 _r, fp32 _g, fp32 _b)
{
	m_PostProcessParams.m_ExposureScale.Set(_r, _g, _b);
}

void CXR_EngineImpl::Con_PPExposureContrast(fp32 _Contrast)
{
	m_PostProcessParams.m_ExposureContrast = _Contrast;
}

void CXR_EngineImpl::Con_PPExposureSaturation(fp32 _Saturation)
{
	m_PostProcessParams.m_ExposureSaturation = _Saturation;
}

void CXR_EngineImpl::Con_PPExposureBlackLevel(fp32 _BlackLevel)
{
	m_PostProcessParams.m_ExposureBlackLevel = _BlackLevel;
}

void CXR_EngineImpl::Con_PPToggleExposureDebug()
{
	m_PostProcessParams.m_DebugFlags ^= XR_PPDEBUGFLAGS_EXPOSUREDEBUG;
}

void CXR_EngineImpl::Con_PPExposureDebug(int _v)
{
	if (_v)
		m_PostProcessParams.m_DebugFlags |= XR_PPDEBUGFLAGS_EXPOSUREDEBUG;
	else
		m_PostProcessParams.m_DebugFlags &= ~XR_PPDEBUGFLAGS_EXPOSUREDEBUG;
}

void CXR_EngineImpl::Con_PPToggleDynamicExposure()
{
	m_PostProcessParams.m_bDynamicExposure ^= true;
}

void CXR_EngineImpl::Con_PPMBMaxRadius(fp32 _v)
{
	m_PostProcess_MBMaxRadius = _v;
}

void CXR_EngineImpl::Con_PPMBDebugFlags(int _v)
{
	m_PostProcess_MBDebugFlags = _v;
}

void CXR_EngineImpl::Con_HexCubeTV0(int _iiTV0, int _diiTV0)
{
	m_iiTV0 = _iiTV0;
	m_diiTV0 = _diiTV0;
}
void CXR_EngineImpl::Con_HexCubeTV1(int _iiTV1, int _diiTV1)
{
	m_iiTV1 = _iiTV1;
	m_diiTV1 = _diiTV1;
}
void CXR_EngineImpl::Con_HexCubeTV2(int _iiTV2, int _diiTV2)
{
	m_iiTV2 = _iiTV2;
	m_diiTV2 = _diiTV2;
}

void CXR_EngineImpl::Con_HexagonColor(int _iCol, int _dCol)
{
	m_iHexagonColor = _iCol;
	m_dHexagonColor = _dCol;
}

void CXR_EngineImpl::Con_HexagonTest(CStr _Name)
{
#ifndef PLATFORM_CONSOLE
	spCImage spImg = MNew(CImage);
	if (!spImg)
		MemError("Con_HexagonTest");

	if (_Name == "")
	{
		if (m_spHexagonTest != NULL)
		{
			m_spHexagonTest->Clear();
			m_spHexagonTest = NULL;
		}
		return;
	}

	CStr FileName = m_pSystem->m_ExePath + "Content\\Dev\\" + _Name;
	if (_Name.GetFilenameExtenstion() == "")
		FileName += ".png";

	if (!CDiskUtil::FileExists(FileName))
	{
		ConOut(CStrF("'%s' doesn't exist.", FileName.Str()) );
		return;
	}

	CTC_TextureProperties Prop;
	Prop.m_Flags |= CTC_TEXTUREFLAGS_NOMIPMAP | CTC_TEXTUREFLAGS_NOPICMIP | CTC_TEXTUREFLAGS_HIGHQUALITY | CTC_TEXTUREFLAGS_NOCOMPRESS;

	spImg->Read(FileName, IMAGE_MEM_IMAGE);
	if (!m_spHexagonTest)
	{
		MRTC_SAFECREATEOBJECT(spTC, "CTextureContainer_Plain", CTextureContainer_Plain);
		m_spHexagonTest = spTC;
		m_spHexagonTest->AddTexture(spImg, Prop, "XREngine_TestHexagon");
	}
	else
	{
		m_spHexagonTest->SetTexture(0, spImg, Prop);
	}
#endif
}


// -------------------------------------------------------------------
//  Script commands
// -------------------------------------------------------------------
void CXR_EngineImpl::Con_BSPDebugFlags(int _Flags)
{
	m_BSPDebugFlags = _Flags;
}

void CXR_EngineImpl::Con_BSPToggleDebugFlags(int _Flags)
{
	m_BSPDebugFlags = m_BSPDebugFlags ^ _Flags;
}

void CXR_EngineImpl::Register(CScriptRegisterContext & _RegContext)
{
	MAUTOSTRIP(CXR_EngineImpl_Register, MAUTOSTRIP_VOID);
	_RegContext.RegFunction("xr_debugflags", this, &CXR_EngineImpl::Con_DebugFlags);
	_RegContext.RegFunction("xr_lightdebugflags", this, &CXR_EngineImpl::Con_LightDebugFlags);
	_RegContext.RegFunction("xr_flares", this, &CXR_EngineImpl::Con_Flares);
	_RegContext.RegFunction("xr_dlight", this, &CXR_EngineImpl::Con_DLight);
	_RegContext.RegFunction("xr_fastlight", this, &CXR_EngineImpl::Con_FastLight);
	_RegContext.RegFunction("xr_wallmarks", this, &CXR_EngineImpl::Con_Wallmarks);
	_RegContext.RegFunction("xr_shadowdecals", this, &CXR_EngineImpl::Con_ShadowDecals);
	_RegContext.RegFunction("xr_lodoffset", this, &CXR_EngineImpl::Con_LODOffset);
	_RegContext.RegFunction("xr_lodscale", this, &CXR_EngineImpl::Con_LODScaleBias);
	_RegContext.RegFunction("xr_showbounding", this, &CXR_EngineImpl::Con_ShowBoundings);
	_RegContext.RegFunction("xr_worldonly", this, &CXR_EngineImpl::Con_WorldOnly);
	_RegContext.RegFunction("xr_objectsonly", this, &CXR_EngineImpl::Con_ObjectsOnly);
	_RegContext.RegFunction("xr_portalsonly", this, &CXR_EngineImpl::Con_PortalsOnly);
	_RegContext.RegFunction("xr_sky", this, &CXR_EngineImpl::Con_Sky);
	_RegContext.RegFunction("xr_showtiming", this, &CXR_EngineImpl::Con_ShowTiming);
	_RegContext.RegFunction("xr_showvbtime", this, &CXR_EngineImpl::Con_ShowVBTime);
	_RegContext.RegFunction("xr_showfoginfo", this, &CXR_EngineImpl::Con_ShowFogInfo);
	_RegContext.RegFunction("xr_showrecurse", this, &CXR_EngineImpl::Con_ShowRecurse);
	_RegContext.RegFunction("xr_showportalfence", this, &CXR_EngineImpl::Con_ShowPortalFence);
	_RegContext.RegFunction("xr_vfog", this, &CXR_EngineImpl::Con_VertexFog);
	_RegContext.RegFunction("xr_zfog", this, &CXR_EngineImpl::Con_DepthFog);
	_RegContext.RegFunction("xr_nhfog", this, &CXR_EngineImpl::Con_NHF);
	_RegContext.RegFunction("xr_surfoptions", this, &CXR_EngineImpl::Con_SurfOptions);
	_RegContext.RegFunction("xr_stencilshadows", this, &CXR_EngineImpl::Con_StencilShadows);
	_RegContext.RegFunction("xr_portaltexturesize", this, &CXR_EngineImpl::Con_PortalTextureSize);
	_RegContext.RegFunction("xr_shadowdecaltexturesize", this, &CXR_EngineImpl::Con_ShadowDecalTextureSize);
	_RegContext.RegFunction("xr_unifiedclear", this, &CXR_EngineImpl::Con_UnifiedClear);
	_RegContext.RegFunction("xr_unifiedambience", this, &CXR_EngineImpl::Con_UnifiedAmbience);
	_RegContext.RegFunction("xr_tlenable", this, &CXR_EngineImpl::Con_TempTLEnable);
	_RegContext.RegFunction("xr_showvbtypes", this, &CXR_EngineImpl::Con_ShowVBTypes);
	_RegContext.RegFunction("xr_optimizedsurfaces", this, &CXR_EngineImpl::Con_GeneratedSurfaces);
//	_RegContext.RegFunction("xr_drawmode", this, &CXR_EngineImpl::Con_DrawMode);
	_RegContext.RegFunction("xr_freeze", this, &CXR_EngineImpl::Con_Freeze);
	_RegContext.RegFunction("xr_clearviewport", this, &CXR_EngineImpl::Con_ClearViewport);
	_RegContext.RegFunction("xr_fogculloffset", this, &CXR_EngineImpl::Con_FogCullOffset);
	_RegContext.RegFunction("xr_modeoverride", this, &CXR_EngineImpl::Con_XRModeOverride);
	_RegContext.RegFunction("xr_synconrender", this, &CXR_EngineImpl::Con_SyncOnRefresh);

	_RegContext.RegFunction("xr_ppdebugflags", this, &CXR_EngineImpl::Con_PPDebugFlags);
	_RegContext.RegFunction("xr_ppglowexp", this, &CXR_EngineImpl::Con_PPGlowExp);
	_RegContext.RegFunction("xr_ppglowbias", this, &CXR_EngineImpl::Con_PPGlowBias);
	_RegContext.RegFunction("xr_ppglowscale", this, &CXR_EngineImpl::Con_PPGlowScale);
	_RegContext.RegFunction("xr_ppglowgamma", this, &CXR_EngineImpl::Con_PPGlowGamma);
	_RegContext.RegFunction("xr_ppglowcenter", this, &CXR_EngineImpl::Con_PPGlowCenter);
	_RegContext.RegFunction("xr_ppexposureexp", this, &CXR_EngineImpl::Con_PPExposureExp);
	_RegContext.RegFunction("xr_ppexposurescale", this, &CXR_EngineImpl::Con_PPExposureScale);
	_RegContext.RegFunction("xr_ppexposurecontrast", this, &CXR_EngineImpl::Con_PPExposureContrast);
	_RegContext.RegFunction("xr_ppexposuresaturation", this, &CXR_EngineImpl::Con_PPExposureSaturation);
	_RegContext.RegFunction("xr_ppexposureblacklevel", this, &CXR_EngineImpl::Con_PPExposureBlackLevel);
	_RegContext.RegFunction("xr_pptoggleexposuredebug", this, &CXR_EngineImpl::Con_PPToggleExposureDebug);
	_RegContext.RegFunction("xr_pptoggledynamicexposure", this, &CXR_EngineImpl::Con_PPToggleDynamicExposure);
	_RegContext.RegFunction("xr_ppexposuredebug", this, &CXR_EngineImpl::Con_PPExposureDebug);

	_RegContext.RegFunction("xr_ppmbmaxradius", this, &CXR_EngineImpl::Con_PPMBMaxRadius);
	_RegContext.RegFunction("xr_ppmbdebugflags", this, &CXR_EngineImpl::Con_PPMBDebugFlags);

	_RegContext.RegFunction("xr_bspdebugflags", this, &CXR_EngineImpl::Con_BSPDebugFlags);
	_RegContext.RegFunction("xr_bsptoggledebugflags", this, &CXR_EngineImpl::Con_BSPToggleDebugFlags);

	_RegContext.RegFunction("xr_hexcubetv0", this, &CXR_EngineImpl::Con_HexCubeTV0);
	_RegContext.RegFunction("xr_hexcubetv1", this, &CXR_EngineImpl::Con_HexCubeTV1);
	_RegContext.RegFunction("xr_hexcubetv2", this, &CXR_EngineImpl::Con_HexCubeTV2);
	_RegContext.RegFunction("xr_hexagoncolor", this, &CXR_EngineImpl::Con_HexagonColor);
	_RegContext.RegFunction("xr_hexagontest", this, &CXR_EngineImpl::Con_HexagonTest);

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	_RegContext.RegFunction("xr_recordvbheap", this, &CXR_EngineImpl::Con_RecordVBHeap);
#endif

}
