#include "PCH.h"

#include "WModel_Flare.h"

#include "MFloat.h"

// -------------------------------------------------------------------
//  CXR_Model_Flare
// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Flare, CXR_Model);

CXR_Model_Flare::CXR_Model_Flare()
{
	MAUTOSTRIP(CXR_Model_Flare_ctor, MAUTOSTRIP_VOID);
	SetThreadSafe(true);
//	int i = 0;
	m_nSample = 10;
/*	m_DefSize = 64*3;
	m_Fade = 1.0f/2048.0f;*/

/*	while((TextureID = pTC->GetTextureID(CStrF("FLARE%d", i))) )
	{
		m_lFlareTxtID.Add(TextureID);
		i++;
	}*/

	m_TVerts[0].k[0] = 0;
	m_TVerts[0].k[1] = 0;
	m_TVerts[1].k[0] = 1;
	m_TVerts[1].k[1] = 0;
	m_TVerts[2].k[0] = 1;
	m_TVerts[2].k[1] = 1;
	m_TVerts[3].k[0] = 0;
	m_TVerts[3].k[1] = 1;

	m_iPrim[0] = CRC_RIP_TRIFAN + (6 << 8);
	m_iPrim[1] = 4;
	m_iPrim[2] = 0;
	m_iPrim[3] = 1;
	m_iPrim[4] = 2;
	m_iPrim[5] = 3;
	m_iPrim[6] = CRC_RIP_END + (1 << 8);
}

void CXR_Model_Flare::Create(const char* _pParam)
{
	MAUTOSTRIP(CXR_Model_Flare_Create, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("-", "No texture-context available.");

	if (_pParam)
	{
		CStr Param(_pParam);
/*		m_nSample = Max(1.0f, Min(10.0f, Param.Getfp64Sep(",")));
		m_DefSize = Param.Getfp64Sep(",");
		m_Fade = 1.0f / Param.Getfp64Sep(","); */
		m_lFlareTxtID.Add(pTC->GetTextureID(Param.GetStrSep(",")));

/*		MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
		if (!pTC) Error("Create", "No texture-context available.");
		m_lFlareTxtID.Clear();
		m_lFlareTxtID.Add(pTC->GetTextureID(Param.GetStrSep(",")));
*/
//		LogFile(CStrF("nSmp %d, DefSize %f, Fade %d", m_nSample, m_DefSize, 1.0f / m_Fade));
	}

	if (m_lFlareTxtID.Len() == 0)
		m_lFlareTxtID.Add(pTC->GetTextureID("SPECIAL_DEFAULTFLARE"));
}

int CXR_Model_Flare::GetTextureID(int _iFlare)
{
	MAUTOSTRIP(CXR_Model_Flare_GetTextureID, 0);
	if (_iFlare < 0) _iFlare = 0;
	if (_iFlare >= m_lFlareTxtID.Len()) _iFlare = 0;

	return m_lFlareTxtID[_iFlare];
}

// Bounding volumes in model-space
fp32 CXR_Model_Flare::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_Flare_GetBound_Sphere, 0.0f);
	return 32;
}

void CXR_Model_Flare::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_Flare_GetBound_Box, MAUTOSTRIP_VOID);
	_Box.m_Min = -32;
	_Box.m_Max = 32;
}

// typedef void(*PFN_VERTEXBUFFER_PRERENDER)(CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext);

void CXR_Model_Flare::VBPreRender(CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext, CXR_VBMScope* _pScope, int _Flags)
{
	MAUTOSTRIP(CXR_Model_Flare_VBPreRender, MAUTOSTRIP_VOID);
	fp32* pData = (fp32*) _pContext;

//	fp32 x2d = pData[0];
//	fp32 y2d = pData[1];
	fp32 DepthOffset = pData[2];
	int m_nSample = (int)pData[3];
	fp32 Z = pData[4];

	CVec2Dfp32* pPos2D = (CVec2Dfp32*) _pContext;

	fp32 Vis = 0;
	{
		const int MaxPixels = 10*10;
		fp32 Depth[MaxPixels];
		int nSmp = Max(1, Min(m_nSample, 10));
		int Disp = (nSmp-1) >> 1;
		int nPixels = Sqr(nSmp);
		if (_pRender->ReadDepthPixels((int)(pPos2D->k[0] * 0.5f - Disp), (int)(pPos2D->k[1] * 0.5f - 2 * Disp), nSmp, nSmp, Depth))
		{
			// W = -Z * 2*far*near/(far - near) ?

			// zd = (zw - 0.5f) * 2;
			// actual_z = 2fn/(zd*(f-n) - (f+n));

			const CRC_Viewport* pVP = _pVBM->Viewport_Get();
			fp32 f = pVP->GetBackPlane();
			fp32 n = pVP->GetFrontPlane() * 0.5f;
//ConOut(CStrF("Flare Z %f", Z));

			for(int i = 0; i < nPixels; i++)
			{
				fp32 zw = Depth[i];
				fp32 zd = (zw - 0.5f) * 2.0f;
				fp32 z = -2.0f * f * n / (zd*(f - n) - (f + n));


//				fp32 z = n * f / ((n - f)*(zw + f/(n - f)));
//				if (!i) ConOut(CStrF("Flare zbuff %f, zw %f", z, zw));

				z += DepthOffset;
				if (z >= Z) Vis++;
			}

/*			const CRC_Viewport* pVP =_pRender->Viewport_Get();
			fp32 zn = pVP->GetFrontPlane() / pVP->GetBackPlane();
			for(int i = 0; i < nPixels; i++)
			{
				fp32 z = 2.0f / (1.0f-(Depth[i]-0.5f*zn));// + pVP->GetFrontPlane();
				z += DepthOffset;
				if (z >= Z) Vis++;
			}*/
			
		}

		if (!Vis)
		{
			CXR_VBChain *pChain = _pVB->GetVBChain();
			pChain->m_nPrim = 0;
			return;
		}
		Vis /= fp32(nPixels);
	}

	CPixel32 Col = _pVB->m_Color;
	_pVB->Geometry_Color(CPixel32(Col.GetR(), Col.GetG(), Col.GetB(), (int)(Col.GetA()*Vis)));
}


void CXR_Model_Flare::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
	const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_Flare_OnRender, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_Flare::OnRender);
	// Anim0 = Size
	// Anim1 = Fadeout dist.
	// Colors[0] = Color  (intensity)
	// Colors[1] = Depth offset.
	// Colors[3] = Flare ID				(!!!!!!!!!!!!!!!EACH FLARE MUST HAVE A UNIQUE ID!!!!!!!!!!!!!!!)

//ConOutL("Flare begin");

// FIXME:!!!!!!!!!!!!!
return;


	if (!_pRender) return;
	if (!_pVBM) return;
	if (!_pAnimState) return;
	if (_pEngine && !_pEngine->GetVar(XR_ENGINE_FLARES)) return;
//*
	CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
	CRC_Attributes* pA = _pVBM->Alloc_Attrib();
	if (!pVB || !pA) return;
	pVB->m_pAttrib = pA;
	pA->SetDefault();
	pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
	pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	pA->Attrib_TextureID(0, GetTextureID(0));

	CXR_Particle4 Flare;
	Flare.m_Pos = CVec3Dfp32::GetMatrixRow(_WMat, 3);
	Flare.m_Dimensions.k[0] = fp32(_pAnimState->m_Anim0) * 2.0f;
	Flare.m_Dimensions.k[1] = fp32(_pAnimState->m_Data[2]) * 2.0f;
	Flare.m_Color = _pAnimState->m_Data[0];
	Flare.m_iObject = _pAnimState->m_iObject;

	fp32 Attenuation = _pAnimState->m_Anim1;
	fp32 DepthOffset = _pAnimState->m_Data[1];

	if (CXR_Util::Render_Flares(_pRender, _pVBM, pVB, _VMat, &Flare, 1, Attenuation, DepthOffset, m_nSample, false))
	{
		pVB->m_Priority = CXR_VBPRIORITY_FLARE;
		_pVBM->AddVB(pVB);
	}


/*/	
	CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(_WMat, 3);
	CVec3Dfp32 PosV = Pos;
	PosV *= _VMat;

	CRC_Viewport* pVP = _pRender->Viewport_Get();
	if (PosV.k[2] <= pVP->GetFrontPlane()) return;
	if (PosV.k[2] > pVP->GetBackPlane()) return;

//	fp32 Scale = (_pAnimState) ? 1.0f / fp32(_pAnimState->m_Anim1) : m_Fade;
	fp32 Scale = (_pAnimState) ? 1.0f / fp32(_pAnimState->m_Anim1) : 1.0f;
	fp32 Fade = 1.0f - PosV.k[2] * Scale;
	if (Fade < 0.0f) return;


	CMat4Dfp32 VInv;
	_VMat.InverseOrthogonal(VInv);

	fp32 x2d = PosV.k[0] * pVP->GetXScale() / PosV.k[2];
	fp32 y2d = PosV.k[1] * pVP->GetYScale() / PosV.k[2];
//	fp32 DepthOffset = _pAnimState->m_Colors[1];
	fp32 DepthOffset = _pAnimState->m_Data[1];

//	fp32 Size = (_pAnimState) ? fp32(_pAnimState->m_Anim0) : m_DefSize;
	fp32 Size = (_pAnimState) ? fp32(_pAnimState->m_Anim0) : 128.0f;
	fp32 wh = Size;
	fp32 hh = Size;
	fp32 z = pVP->GetXScale();

	CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
	CRC_Attributes* pA = _pVBM->Alloc_Attrib();
	if (!pVB || !pA) return;
	pVB->m_pAttrib = pA;
	pA->SetDefault();
	pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
	pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	pA->Attrib_TextureID(0, GetTextureID(0));

	CVec3Dfp32* pV = _pVBM->Alloc_V3(4);
	if (!pV) return;
	pV[0].k[0] = x2d - wh;
	pV[0].k[1] = y2d - hh;
	pV[0].k[2] = z;
	pV[1].k[0] = x2d + wh;
	pV[1].k[1] = y2d - hh;
	pV[1].k[2] = z;
	pV[2].k[0] = x2d + wh;
	pV[2].k[1] = y2d + hh;
	pV[2].k[2] = z;
	pV[3].k[0] = x2d - wh;
	pV[3].k[1] = y2d + hh;
	pV[3].k[2] = z;

//	CPixel32 Col = (_pAnimState) ? _pAnimState->m_Colors[0] : 0xffffffff;
	CPixel32 Col = (_pAnimState) ? _pAnimState->m_Data[0] : 0xffffffff;
	pVB->SetVBChain(_pVBM,false);
	pVB->Geometry_VertexArray(pV, 4,true);
	pVB->Geometry_TVertexArray(m_TVerts, 0);
	pVB->Geometry_Color(CPixel32(Col.GetR(), Col.GetG(), Col.GetB(), Col.GetA()*Fade));
	pVB->Render_IndexedPrimitives(m_iPrim, 7);
	pVB->m_Priority = CXR_VBPRIORITY_FLARE;

	fp32* pData = (fp32*) _pVBM->Alloc(5*sizeof(fp32));
	if (!pData) return;
	pData[0] = x2d;
	pData[1] = y2d;
	pData[2] = DepthOffset;
	pData[3] = m_nSample;
	pData[4] = PosV.k[2];

	//	pVB->m_pPreRenderContext = pData;
	// pVB->m_pfnPreRender = VBPreRender;
	CXR_VertexBuffer_PreRender * pVBpre = (CXR_VertexBuffer_PreRender*)_pVBM->Alloc(sizeof(CXR_VertexBuffer_PreRender));
	pVBpre->m_pfnPreRender = VBPreRender;
	pVBpre->m_pPreRenderContext = pData;
	pVB->m_pPreRender = pVBpre;

	_pVBM->AddVB(pVB);
//*/
}

void CXR_Model_Flare::OnPrecache(CXR_Engine* _pEngine, int _iVariation)
{
	CTextureContext* pTC = _pEngine->m_pTC;
	for(int i = 0; i < m_lFlareTxtID.Len(); i++)
		pTC->SetTextureParam(m_lFlareTxtID[i], CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
}


// -------------------------------------------------------------------
//  CXR_Model_Sprite
// -------------------------------------------------------------------
CXR_Model_Sprite::CXR_Model_Sprite()
{
	SetThreadSafe(false);
}

void CXR_Model_Sprite::Create(const char* _pParam)
{
	MAUTOSTRIP(CXR_Model_Sprite_Create, MAUTOSTRIP_VOID);
	m_iDefSurface = 0;
	m_DefSize = 128;
	if (_pParam)
	{
		CStr Param(_pParam);
		
		MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
		if (!pSC) Error("Create", "No surface-context available.");
		m_iDefSurface = pSC->GetSurfaceID(Param.GetStrSep(","));
		
		if (Param.Len())
			m_DefSize = Param.Getfp64Sep(",");
		
		//			ConOutL(CStrF("(CXR_Model_Sprite::Create) Params %s, iDefSurface %d, DefSize %f", _pParam, m_iDefSurface, m_DefSize));
	}
}

fp32 CXR_Model_Sprite::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_Sprite_GetBound_Sphere, 0.0f);
//	return (_pAnimState) ? _pAnimState->m_Anim0 * _SQRT2 : (m_DefSize * _SQRT2);
	return (_pAnimState) ? _pAnimState->m_Anim0 * _SQRT2 / 2 : (m_DefSize * _SQRT2 / 2);
}

void CXR_Model_Sprite::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_Sprite_GetBound_Box, MAUTOSTRIP_VOID);
	fp32 Size = (_pAnimState) ? _pAnimState->m_Anim0 : m_DefSize;
	if(Size == 0)
		Size = m_DefSize;
	Size /= 2;
	_Box.m_Min = -Size;
	_Box.m_Max = Size;
}

void CXR_Model_Sprite::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_Sprite_OnRender, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_Sprite::OnRender);
	// Anim0 = Size
	// Anim1 = SizeExpand per second
	// AnimAttr0 = Start angle
	// AnimAttr1 = Rotation angle per second
	// Colors[0] = Color  (intensity)
	// m_lpSurfaces[0] = Surface
	
	if (!_pRender) return;
	if (!_pVBM) return;
	if (!_pAnimState) return;

	CXW_Surface* pSurf = _pAnimState->m_lpSurfaces[0];
	if (!pSurf)
	{
		MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
		if (!pSC) Error("Create", "No surface-context available.");
		pSurf = m_iDefSurface ? pSC->GetSurface(m_iDefSurface) : NULL;
		if (!pSurf)
		{
			ConOut("(CXR_Model_Sprite::Render) No Surface.");
			return;
		}
	}
	
	CXR_RenderInfo RenderInfo(_pEngine);
	if (_pViewClip)
	{
		// Get bound-sphere, get the CRC_ClipView
		if (!_pViewClip->View_GetClip_Sphere(CVec3Dfp32::GetMatrixRow(_WMat, 3), GetBound_Sphere(_pAnimState), 0, 0, NULL, &RenderInfo))
			return;
	}
	
	CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
	if (!pVB) return;
	
	fp32 Size = fp32(_pAnimState->m_Anim0) * 2.0f;
	Size += fp32(_pAnimState->m_Anim1) * _pAnimState->m_AnimTime0.GetTime() * 2.0f;
	if (!Size) Size = m_DefSize;
	
	CXR_Particle2 Sprite;
	Sprite.m_Pos = CVec3Dfp32::GetMatrixRow(_WMat, 3);
	Sprite.m_Size = Size;
	Sprite.m_Color = _pAnimState->m_Data[0];
	Sprite.m_Angle = _pAnimState->m_AnimAttr0 + _pAnimState->m_AnimAttr1 * _pAnimState->m_AnimTime0.GetTime();
	
	//	static bool Render_Particles(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat4Dfp32 &_Mat, const CXR_Particle2* _pParticles, int _nParticles, const CMat4Dfp32* _pAlign = NULL, int _PrimType = CXR_PARTICLETYPE_AUTO);
	
	if (CXR_Util::Render_Particles(_pVBM, pVB, _VMat, &Sprite, 1, NULL, CXR_PARTICLETYPE_QUAD | CXR_PARTICLETYPE_ANGLE))
	{
		pVB->m_Priority = RenderInfo.m_BasePriority_Transparent;
		
		CMTime AnimTime = pSurf->m_iController ? _pAnimState->m_AnimTime1 : _pAnimState->m_AnimTime0;
		CXW_SurfaceKeyFrame* pSurfKey = pSurf->GetFrame(
			pSurf->m_iController ? _pAnimState->m_Anim1 : _pAnimState->m_Anim0, 
			AnimTime);

		
		int Flags = 0;//RENDERSURFACE_VERTEXFOG;
		CXR_Util::Render_Surface(Flags, AnimTime, pSurf, pSurfKey, _pEngine, _pVBM,(CMat4Dfp32*) NULL,(CMat4Dfp32*) NULL, (CMat4Dfp32*)NULL, pVB, 
			RenderInfo.m_BasePriority_Transparent, 0.0001f, 0);
	}
}

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Sprite, CXR_Model);


// -------------------------------------------------------------------
//  CXR_Model_SphereSprite
// -------------------------------------------------------------------
void CXR_Model_SphereSprite::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_SphereSprite_OnRender, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_SphereSprite::OnRender);
	// Anim0 = Size
	// Anim1 = SizeExpand per second
	// AnimAttr0 = Start angle
	// AnimAttr1 = Rotation angle per second
	// Colors[0] = Color  (intensity)
	// m_lpSurfaces[0] = Surface
	
	if (!_pRender) return;
	if (!_pVBM) return;
	if (!_pAnimState) return;
	
	CXW_Surface* pSurf = _pAnimState->m_lpSurfaces[0];
	if (!pSurf)
	{
		MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
		if (!pSC) Error("Create", "No surface-context available.");
		pSurf = pSC->GetSurface(m_iDefSurface);
		if (!pSurf)
		{
			if (!pSurf)
				ConOut("(CXR_Model_Sprite::Render) No Surface.");
			return;
		}
	}
	
	CXR_RenderInfo RenderInfo(_pEngine);
	if (_pViewClip)
	{
		// Get bound-sphere, get the CRC_ClipView
		if (!_pViewClip->View_GetClip_Sphere(CVec3Dfp32::GetMatrixRow(_WMat, 3), GetBound_Sphere(_pAnimState), 0, 0, NULL, &RenderInfo))
			return;
	}
	
	fp32 Size = fp32(int16(_pAnimState->m_Anim0 & 0x7fff)) * 2.0f;
	if (!Size) Size = m_DefSize;
	Size += fp32(_pAnimState->m_Anim1) * _pAnimState->m_AnimTime0.GetTime();
	
	CVec3Dfp32 Edge[8];
	for(int e = 0; e < 6; e++)
	{
		//			Edge[e] = CVec3Dfp32((float(e) / 8) * Time * 40, 0, Sqr(Time) * 10 * e);
		fp32 s, c;
		QSinCos(fp32(e) / 7.0f * 0.5f * _PI, s, c);
		Edge[e] = CVec3Dfp32(s*Size, 0, -c*Size + Size * 0.75f);
	}
	
	CMat4Dfp32 Mat, Mat0, L2V;
	
	_WMat.Multiply(_VMat, L2V);
	if (!(_pAnimState->m_Anim0 & 0x8000))
		L2V.Unit3x3();
	
	fp32 Angle = _pAnimState->m_AnimAttr0 + _pAnimState->m_AnimTime0.GetTimeModulusScaled(_pAnimState->m_AnimAttr1, _PI2);
	CXR_VertexBuffer *pVB = CXR_Util::Create_SOR(_pVBM, L2V, Edge, 6, 8, Angle, 1, Size * 0.65f);
	if(pVB)
	{
		pVB->m_Priority = RenderInfo.m_BasePriority_Transparent;
		
		CMTime AnimTime = pSurf->m_iController ? _pAnimState->m_AnimTime1 : _pAnimState->m_AnimTime0;
		CXW_SurfaceKeyFrame* pSurfKey = pSurf->GetFrame(
			pSurf->m_iController ? _pAnimState->m_Anim1 : _pAnimState->m_Anim0, 
			AnimTime);
		
		int Flags = 0;// RENDERSURFACE_VERTEXFOG;
		CXR_Util::Render_Surface(Flags, AnimTime, pSurf, pSurfKey, _pEngine, _pVBM,(CMat4Dfp32*) NULL,(CMat4Dfp32*) NULL, (CMat4Dfp32*)NULL, pVB, 
			RenderInfo.m_BasePriority_Transparent, 0.0001f, 0);
	}
}

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_SphereSprite, CXR_Model);

// -------------------------------------------------------------------
//  CXR_Model_ConcaveSprite
// -------------------------------------------------------------------
void CXR_Model_ConcaveSprite::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_ConcaveSprite_OnRender, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_ConcaveSprite::OnRender);
	// Anim0 = Size, 0x8000 => Not sprite
	// Anim1 = SizeExpand per second
	// AnimAttr0 = Start angle
	// AnimAttr1 = Rotation angle per second
	// Colors[0] = Color  (intensity)
	// m_lpSurfaces[0] = Surface
	
	if (!_pRender) return;
	if (!_pVBM) return;
	if (!_pAnimState) return;
	
	CXW_Surface* pSurf = _pAnimState->m_lpSurfaces[0];
	if (!pSurf)
	{
		MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
		if (!pSC) Error("Create", "No surface-context available.");
		pSurf = pSC->GetSurface(m_iDefSurface);
		if (!pSurf)
		{
			if (!pSurf)
				ConOut("(CXR_Model_Sprite::Render) No Surface.");
			return;
		}
	}
	
	CXR_RenderInfo RenderInfo(_pEngine);
	if (_pViewClip)
	{
		// Get bound-sphere, get the CRC_ClipView
		if (!_pViewClip->View_GetClip_Sphere(CVec3Dfp32::GetMatrixRow(_WMat, 3), GetBound_Sphere(_pAnimState), 0, 0, NULL, &RenderInfo))
			return;
	}
	
	fp32 Size = fp32(int16(_pAnimState->m_Anim0 & 0x7fff)) * 2.0f;
	if (!Size) Size = m_DefSize;
	Size += fp32(_pAnimState->m_Anim1) * _pAnimState->m_AnimTime0.GetTime();
	
	CVec3Dfp32 Edge[8];
	for(int e = 0; e < 6; e++)
	{
		//			Edge[e] = CVec3Dfp32((float(e) / 8) * Time * 40, 0, Sqr(Time) * 10 * e);
		fp32 s, c;
		QSinCos(fp32(e) / 7.0f * 0.5f * _PI, s, c);
		Edge[e] = CVec3Dfp32(s*Size, 0, (1.0f - c)*Size*0.5f);
	}
	
	CMat4Dfp32 Mat, Mat0, L2V;
	Mat.Unit();
	
	_WMat.Multiply(_VMat, L2V);
	CVec3Dfp32::GetMatrixRow(Mat, 3) = CVec3Dfp32::GetMatrixRow(L2V, 3);
	//		if (!(_pAnimState->m_Anim0 & 0x8000))
	//			L2V.Unit3x3();
	
	fp32 Angle = _pAnimState->m_AnimAttr0 + _pAnimState->m_AnimTime0.GetTimeModulusScaled(_pAnimState->m_AnimAttr1, _PI2);
	CXR_VertexBuffer *pVB = CXR_Util::Create_SOR(_pVBM, L2V, Edge, 6, 8, Angle, 1, Size * 0.65f);
	if(pVB)
	{
		pVB->m_Priority = RenderInfo.m_BasePriority_Transparent;
		
		CMTime AnimTime = pSurf->m_iController ? _pAnimState->m_AnimTime1 : _pAnimState->m_AnimTime0;
		CXW_SurfaceKeyFrame* pSurfKey = pSurf->GetFrame(
			pSurf->m_iController ? _pAnimState->m_Anim1 : _pAnimState->m_Anim0, 
			AnimTime);
		
		int Flags = 0;// RENDERSURFACE_VERTEXFOG;
		CXR_Util::Render_Surface(Flags, AnimTime, pSurf, pSurfKey, _pEngine, _pVBM,(CMat4Dfp32*) NULL,(CMat4Dfp32*) NULL, (CMat4Dfp32*)NULL, pVB, 
			RenderInfo.m_BasePriority_Transparent, 0.0001f, 0);
	}
}

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_ConcaveSprite, CXR_Model);


// -------------------------------------------------------------------
//  CXR_Model_SpotLightVolume
// -------------------------------------------------------------------
void CXR_Model_SpotLightVolume::Create(const char* _pParam)
{
	MAUTOSTRIP(CXR_Model_SpotLightVolume_Create, MAUTOSTRIP_VOID);

	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	if (!pSC) Error("Create", "No surface-context available.");

	m_iDefSurface = pSC->GetSurfaceID("SPECIAL_LCV");
	m_DefSize = 128;

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("-", "No texture-context available.");
	m_iFlareTexture = pTC->GetTextureID("Effect_Flare01");
/*	if (_pParam)
	{
		CStr Param(_pParam);
		
		m_iDefSurface = pSC->GetSurfaceID(Param.GetStrSep(","));
		
		if (Param.Len())
			m_DefSize = Param.Getfp64Sep(",");
		
		//			ConOutL(CStrF("(CXR_Model_SpotLightVolume::Create) Params %s, iDefSurface %d, DefSize %f", _pParam, m_iDefSurface, m_DefSize));
	}*/
}


void CXR_Model_SpotLightVolume::OnPrecache(CXR_Engine* _pEngine, int _iVariation)
{
	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	if (!pSC) Error("Create", "No surface-context available.");
	CXW_Surface* pS = pSC->GetSurface(m_iDefSurface);
	pS = pS->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);

	pS->InitTextures(false);	// Don't report failures.
	if (!(pS->m_Flags & XW_SURFFLAGS_INVISIBLE))
		pS->SetTextureParam(CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);

	_pEngine->m_pTC->SetTextureParam(m_iFlareTexture, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
}

fp32 CXR_Model_SpotLightVolume::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_SpotLightVolume_GetBound_Sphere, 0.0f);
//	return (_pAnimState) ? _pAnimState->m_Anim0 * _SQRT2 : (m_DefSize * _SQRT2);
//	return (_pAnimState) ? _pAnimState->m_Anim0 * _SQRT2 / 2 : (m_DefSize * _SQRT2 / 2);
	return m_DefSize * _SQRT2 / 2;
}

void CXR_Model_SpotLightVolume::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_SpotLightVolume_GetBound_Box, MAUTOSTRIP_VOID);
	fp32 Size = /*(_pAnimState) ? _pAnimState->m_Anim0 :*/ m_DefSize;
	if(Size == 0)
		Size = m_DefSize;
	Size /= 2;
	_Box.m_Min = -Size;
	_Box.m_Max = Size;
}

void CXR_Model_SpotLightVolume::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_SpotLightVolume_OnRender, MAUTOSTRIP_VOID);
	// Anim0 = Size
	// Anim1 = SizeExpand per second
	// AnimAttr0 = Start angle
	// AnimAttr1 = Rotation angle per second
	// Colors[0] = Color  (intensity)
	// m_lpSurfaces[0] = Surface
	
	if (!_pRender) return;
	if (!_pVBM) return;
	if (!_pAnimState) return;
	
	CXW_Surface* pSurf = _pAnimState->m_lpSurfaces[0];
	if (!pSurf)
	{
		MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
		if (!pSC) Error("Create", "No surface-context available.");
		pSurf = pSC->GetSurface(m_iDefSurface);
		if (!pSurf)
		{
			if (!pSurf)
				ConOut("(CXR_Model_SpotLightVolume::Render) No Surface.");
			return;
		}
//	pSurf = pSC->GetSurface("SPECIAL_LCV");
	}
//ConOut(CStrF("pSurf->m_Name = %s", pSurf->m_Name.Str()));
	CXR_RenderInfo RenderInfo(_pEngine);
	if (_pViewClip)
	{
		// Get bound-sphere, get the CRC_ClipView
		if (!_pViewClip->View_GetClip_Sphere(CVec3Dfp32::GetMatrixRow(_WMat, 3), GetBound_Sphere(_pAnimState), 0, 0, NULL, &RenderInfo))
			return;
	}
	
	CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
	if (!pVB) return;
	
	fp32 Size = fp32(_pAnimState->m_Anim0) * 2.0f;
	if (!Size) Size = m_DefSize;
	Size += fp32(_pAnimState->m_Anim1) * _pAnimState->m_AnimTime0.GetTime();
	

	CXR_Particle2 lSprites[16];

	const CVec3Dfp32& Fwd = CVec3Dfp32::GetMatrixRow(_WMat, 0);

	int nSprites = 8;
	for(int i = 0; i < nSprites; i++)
	{
		int k = i;
		CVec3Dfp32::GetMatrixRow(_WMat, 3).Combine(Fwd, k*4, lSprites[i].m_Pos);
		lSprites[i].m_Size = 16 + k*6;
//		CVec3Dfp32::GetMatrixRow(_WMat, 3).Combine(Fwd, k*8, lSprites[i].m_Pos);
//		lSprites[i].m_Size = 32 + k*12;
		lSprites[i].m_Color = CPixel32(255-k*16, 255-k*16, 255-k*16, 255);

		fp32 r = (fp32(i & 1)-0.5f) * 0.0323f;
		lSprites[i].m_Angle = _pAnimState->m_AnimTime0.GetTimeModulusScaled(r, _PI2);
	}

	//	static bool Render_Particles(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat4Dfp32 &_Mat, const CXR_Particle2* _pParticles, int _nParticles, const CMat4Dfp32* _pAlign = NULL, int _PrimType = CXR_PARTICLETYPE_AUTO);
	
	if (CXR_Util::Render_Particles(_pVBM, pVB, _VMat, lSprites, nSprites, NULL, CXR_PARTICLETYPE_QUAD | CXR_PARTICLETYPE_ANGLE))
	{
		pVB->m_Priority = RenderInfo.m_BasePriority_Transparent;
		
		CMTime AnimTime = pSurf->m_iController ? _pAnimState->m_AnimTime1 : _pAnimState->m_AnimTime0;
		CXW_SurfaceKeyFrame* pSurfKey = pSurf->GetFrame(
			pSurf->m_iController ? _pAnimState->m_Anim1 : _pAnimState->m_Anim0, 
			AnimTime);

		
		int Flags = 0; //RENDERSURFACE_VERTEXFOG;
		CXR_Util::Render_Surface(Flags, AnimTime, pSurf, pSurfKey, _pEngine, _pVBM, (CMat4Dfp32*)NULL, (CMat4Dfp32*)NULL, (CMat4Dfp32*)NULL, pVB, 
			RenderInfo.m_BasePriority_Transparent, 0.0001f, 0);
	}

	do
	{
/*		CVec3Dfp32 VCam;
		CVec3Dfp32::GetMatrixRow(_pEngine->GetVC()->m_CameraWMat, 3).Sub(CVec3Dfp32::GetMatrixRow(_WMat, 3), VCam);
		fp32 VCamLen = VCam.Length();

		fp32 s = CVec3Dfp32::GetMatrixRow(_WMat, 0) * VCam;
		if(s < 0.0f)
			break;

		fp32 Range =	768.0f;

		if (VCamLen > Range)
			break;

		fp32 SizeNear = 192.0f;
		fp32 SizeFar = 128.0f;
		fp32 Size = SizeNear * 64.0f / Max(1.0f, VCamLen);

		s *= 1.0f / VCamLen;
		s = 1.0f - Sqr(1.0f - s);

*/
		CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
		CRC_Attributes* pA = _pVBM->Alloc_Attrib();
		if (!pVB || !pA)
			break;
		pVB->m_pAttrib = pA;
		pA->SetDefault();
		pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
		pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
		pA->Attrib_TextureID(0, m_iFlareTexture);

		CXR_Particle2 Flare;
		Flare.m_Pos = CVec3Dfp32::GetRow(_WMat, 3) + CVec3Dfp32::GetRow(_WMat, 0) * 2;
//		Flare.m_Size = Size;
		Flare.m_Size = 48;

//		int Alpha = RoundToInt(Clamp01(s) * 255.0f);
		int Alpha = 128;
		Flare.m_Color = 0xffffff + (Alpha << 24);

//		fp32 Attenuation = _pAnimState->m_Anim1;
//		fp32 DepthOffset = _pAnimState->m_Data[1];

//		if (CXR_Util::Render_Flares(_pRender, _pVBM, pVB, _VMat, &Flare, 1, Range, 8, 5, false))
		if (CXR_Util::Render_Particles(_pVBM, pVB, _VMat, &Flare, 1, NULL, CXR_PARTICLETYPE_QUAD | CXR_PARTICLETYPE_ANGLE))
		{
//			pVB->m_Priority = CXR_VBPRIORITY_FLARE;
			pVB->m_Priority = RenderInfo.m_BasePriority_Transparent;
			_pVBM->AddVB(pVB);
		}
	}
	while(0);
}

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_SpotLightVolume, CXR_Model);


