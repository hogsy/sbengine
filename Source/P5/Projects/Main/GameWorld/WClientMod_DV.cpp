#include "PCH.h"
#include "WClientMod.h"
#include "../../Shared/MOS/XR/XREngineImp.h"
#include "../../Shared/MOS/XR/XREngineVar.h"
#include "../../Shared/MOS/XR/XRVBUtil.h"
#include "../../Shared/MOS/XR/XRCustomModel.h"
#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Game.h"

#include "../GameClasses/WObj_CharMsg.h"

// -------------------------------------------------------------------
CXR_Model_CameraEffects::CXR_Model_CameraEffects()
{
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("Read", "No texture-context available.");

	m_TextureID_OWHSC = pTC->GetTextureID(MHASH4('SPEC','IAL_','HSVT','EST5'));
}

void CXR_Model_CameraEffects::PrepareVisions(const fp32& _CreepingDark, const fp32& _AncientWeapon)
{
	m_CreepingDark = _CreepingDark;
	m_AncientWeapon = _AncientWeapon;
}

void CXR_Model_CameraEffects::PrepareRadialBlur(bool _bRadialBlur, const fp32 _RadialBlur, const fp32 _RadialBlurColorIntensity,
												const CVec3Dfp32& _RadialBlurAffection, const CVec3Dfp32& _RadialBlurColorScale,
												const CVec2Dfp32& _RadialBlurCenter, const CVec2Dfp32& _RadialBlurUVExtra,
												uint _RadialBlurMode, CVec4Dfp32* _pRadialBlurFilterParams, uint _nRadialBlurFilterParams,
												const char* _pRadialBlurFilter)
{
	m_bRadialBlur = _bRadialBlur;
	m_RadialBlur = _RadialBlur;
	m_RadialBlurColorIntensity = _RadialBlurColorIntensity;
	m_RadialBlurAffection = _RadialBlurAffection;
	m_RadialBlurColorScale = _RadialBlurColorScale;
	m_RadialBlurCenter = _RadialBlurCenter;
	m_pRadialBlurFilter = _pRadialBlurFilter;
	m_RadialBlurMode = _RadialBlurMode;
	m_RadialBlurUVExtra = _RadialBlurUVExtra;

	// Setup filter parameters
	m_nRadialBlurFilterParams = _nRadialBlurFilterParams;
	for (uint i = 0; i < _nRadialBlurFilterParams; i++)
		m_lRadialBlurFilterParams[i] = _pRadialBlurFilterParams[i];
}

void CXR_Model_CameraEffects::PrepareSimple()
{
	m_bRadialBlur = false;
}

bool CXR_Model_CameraEffects::ClearScreen(CXR_VBManager* _pVBM, CRC_Viewport* _pVP, CRect2Duint16& _VPRect16, const fp32& _Priority, const uint32& _Color)
{
	CRC_Attributes* pA = _pVBM->Alloc_Attrib();
	CMat4Dfp32* pMat2D = _pVBM->Alloc_M4_Proj2DRelBackPlane(_pVP);
	if(!pA || !pMat2D)
		return false;

	pA->SetDefault();
	pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
	CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(_pVBM, pMat2D, _VPRect16, _Color, _Priority, pA);
	if (!pVB)
		return false;

	_pVBM->AddVB(pVB);
	return true;
}

bool CXR_Model_CameraEffects::RenderQuad(CRC_Viewport* _pVP, CXR_VBManager* _pVBM, const CRect2Duint16& _VPRect16, const int& _bRenderTextureVertFlip, const CVec2Dfp32& _UVMin, const CVec2Dfp32& _UVMax, const uint16& _TextureID, const uint32& _Color, const fp32& _Priority, const uint32 _Enable, const uint32 _Disable, const uint32 _RasterMode)
{
	CVec2Dfp32* pTV = (_bRenderTextureVertFlip) ? 
		CXR_Util::VBM_CreateRectUV_VFlip(_pVBM, _UVMin, _UVMax) :
	CXR_Util::VBM_CreateRectUV(_pVBM, _UVMin, _UVMax);
	CMat4Dfp32* pMat2D = _pVBM->Alloc_M4_Proj2DRelBackPlane(_pVP);
	CRC_Attributes* pA = _pVBM->Alloc_Attrib();
	if (!pTV || !pA || !pMat2D)
		return false;

	pA->SetDefault();
	pA->Attrib_Enable(_Enable);
	pA->Attrib_Disable(_Disable);
	pA->Attrib_TextureID(0, _TextureID);
	pA->Attrib_RasterMode(_RasterMode);
	CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(_pVBM, pMat2D, _VPRect16, _Color, _Priority, pA);
	if (!pVB)
		return false;
	pVB->Geometry_TVertexArray(pTV, 0);
	_pVBM->AddVB(pVB);

	return true;
}

bool CXR_Model_CameraEffects::RenderDownsample(const int& _nDownsamples, const int& _bRenderTextureVertFlip, CRC_Viewport* _pVP, CXR_VBManager* _pVBM, CRect2Duint16 _VPRect16, const fp32& _Priority, const int& _TextureID, const uint16& _CaptureID, const CVec2Dfp32& _UVMin, const CVec2Dfp32& _UVMax, const uint32& _Color)
{
	int TextureID = _TextureID;
	CVec2Dfp32 UVMax = _UVMax;
	CVec2Dfp32 UVMin = _UVMin;
	fp32 Priority = _Priority;

	CMat4Dfp32* pMat2D = _pVBM->Alloc_M4_Proj2DRelBackPlane(_pVP);
	if (!pMat2D)
		return false;

	for(int i = 0; i < _nDownsamples; i++)
	{
		// Downsample
		CRC_Attributes* pA = _pVBM->Alloc_Attrib();
		if(!pA)
			return false;

		CVec2Dfp32* pTV = (_bRenderTextureVertFlip) ? 
			CXR_Util::VBM_CreateRectUV_VFlip(_pVBM, UVMin, UVMax) :
		CXR_Util::VBM_CreateRectUV(_pVBM, UVMin, UVMax);
		if (!pTV)
			return false;

		pA->SetDefault();
		pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
		pA->Attrib_TextureID(0, TextureID);
		CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(_pVBM, pMat2D, _VPRect16, _Color, Priority, pA);
		if(!pVB)
			return false;

		pVB->Geometry_TVertexArray(pTV, 0);
		_pVBM->AddVB(pVB);

		// Capture
		_pVBM->AddCopyToTexture(Priority + 0.001f, CRct(_VPRect16.m_Min[0], _VPRect16.m_Min[1], _VPRect16.m_Max[0], _VPRect16.m_Max[1]), CPnt(_VPRect16.m_Min[0], _VPRect16.m_Min[1]), _CaptureID, false);

		UVMax = UVMax * 0.5f;
		UVMin = UVMin * 0.5f;

		// TextureID is now equal to the captured screen for next downsampling
		TextureID = (int)_CaptureID;
		Priority += 0.002f;

		_VPRect16.m_Max[0] = _VPRect16.m_Min[0] + ((_VPRect16.m_Max[0] - _VPRect16.m_Min[0]) >> 1);
		_VPRect16.m_Max[1] = _VPRect16.m_Min[1] + ((_VPRect16.m_Max[1] - _VPRect16.m_Min[1]) >> 1);
	}

	return true;
}

void CXR_Model_CameraEffects::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
										const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MSCOPESHORT(CXR_Model_CameraEffects::OnRender);

	if (!_pEngine)
		return;

	int CamFXMode = _pAnimState->m_Anim0;
	int bCreepingDark = (CamFXMode & CXR_MODEL_CAMFX_CREEPINGDARK);
	int bDarknessVision = (CamFXMode & CXR_MODEL_CAMFX_DARKNESSVISION);
	int bOtherWorld = (CamFXMode & CXR_MODEL_CAMFX_OTHERWORLD);
	int bRetinaDots = (CamFXMode & CXR_MODEL_CAMFX_RETINADOTS);
	fp32 DVIntensity = Clamp01(MaxMT(m_CreepingDark, _pAnimState->m_AnimAttr0));

	// Fetch texture id's to work with
	int TextureID_Screen = _pEngine->m_TextureID_Screen;
	int TextureID_LastScreen = _pEngine->m_TextureID_PostProcess;

	// These 4 texture id's share the same texture
	uint TextureID_4xTexture = _pEngine->m_TextureID_DeferredDiffuse; // ____ ____
	uint TextureID_BlurEdge = TextureID_4xTexture;				// BE	| BE | SH |
	uint TextureID_BlurScreen = TextureID_4xTexture;			// BS	|____|____|
	uint TextureID_ScreenHalf = TextureID_4xTexture;			// SH	| BS | RB |
	uint TextureID_RadialBlur = TextureID_4xTexture;			// RB	|____|____|

	uint16 TextureID_Depth = (_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_COPYDEPTH) ? _pEngine->m_TextureID_DepthStencil : _pEngine->m_TextureID_Depth;

	CXR_VBManager* pVBM = _pEngine->GetVBM();
	CRC_Viewport* pVP = pVBM->Viewport_Get();
	CMat4Dfp32* pMat2D = _pVBM->Alloc_M4_Proj2DRelBackPlane();
	if (!pMat2D)
		return;

	CRct VPRect = pVP->GetViewRect();;
	CRect2Duint16 VPRect16;
	VPRect16 << VPRect;
	int sw = pVP->GetViewRect().GetWidth();
	int sh = pVP->GetViewRect().GetHeight();
	fp32 swf = fp32(sw);
	fp32 shf = fp32(sh);

	CVec2Dfp32 UVMin(0.0f, 0.0f);
	CVec2Dfp32 UVMax(swf * _pEngine->m_Screen_PixelUV[0], shf * _pEngine->m_Screen_PixelUV[1]);
	CVec2Dfp32 UVTexel = _pEngine->m_Screen_PixelUV;
	CVec2Dfp32 UVMinHalf = UVMin*0.5f;
	CVec2Dfp32 UVMaxHalf = UVMax*0.5f;

	int bRenderTextureVertFlip  = _pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP;

	CVec2Dfp32* pTVWhole = (bRenderTextureVertFlip) ? 
		CXR_Util::VBM_CreateRectUV_VFlip(pVBM, UVMin, UVMax) :
		CXR_Util::VBM_CreateRectUV(pVBM, UVMin, UVMax);
	CVec2Dfp32* pTVHalf = (bRenderTextureVertFlip) ? 
		CXR_Util::VBM_CreateRectUV_VFlip(pVBM, UVMinHalf, UVMaxHalf) :
		CXR_Util::VBM_CreateRectUV(pVBM, UVMinHalf, UVMaxHalf);

	int iLight = 0;
	CXR_Light Light;

	iLight = _pEngine->m_pSceneGraphInstance->SceneGraph_Light_GetIndex(0x2346);
	if (iLight)
		_pEngine->m_pSceneGraphInstance->SceneGraph_Light_Get(iLight, Light);

	fp32 DVPriority = CXR_VBPRIORITY_UNIFIED_MATERIAL + 5000;

	// Darkness vision + creeping dark
	if(bDarknessVision || bCreepingDark || m_bRadialBlur)
	do
	{
		pVBM->AddCopyToTexture(DVPriority + 0.000f, VPRect, VPRect.p0, TextureID_Screen, false);

		// Render edge detection
		if (bDarknessVision)
		{
			CRC_ExtAttributes_FragmentProgram20* pFP = (CRC_ExtAttributes_FragmentProgram20*) pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
			if (!pFP)
				break;
			pFP->Clear();
			pFP->SetProgram("WClientMod_DV3_0", 0);

			CMTime Time = _pEngine->GetEngineTime();

			int nFPParams = 7;
			CVec4Dfp32* pFPParams = pVBM->Alloc_V4(nFPParams);
			if (!pFPParams)
				break;
			fp32 Front = pVP->GetFrontPlane();
			fp32 Back = pVP->GetBackPlane();
			pFPParams[0].k[0] = Front;
			pFPParams[0].k[1] = Back;
//			pFPParams[0].k[2] = fp32(sw) / pVP->GetXScale();
			pFPParams[0].k[2] = 1.0f / pVP->GetBackPlane();
			pFPParams[0].k[3] = shf / pVP->GetYScale();
//			pFPParams[1].k[0] = Back - Front;
			pFPParams[1].k[0] = 2.0f * Back;
			pFPParams[1].k[1] = Back + Front;
			pFPParams[1].k[2] = 2.0f * Back * Front;
			pFPParams[1].k[3] = 2.0f * (Back - Front);
			pFPParams[2].k[0] = UVTexel.k[0];	pFPParams[2].k[1] = 0;	pFPParams[2].k[2] = 0;	pFPParams[2].k[3] = 0;
			pFPParams[3].k[0] = -UVTexel.k[0];	pFPParams[3].k[1] = 0;	pFPParams[3].k[2] = 0;	pFPParams[3].k[3] = 0;
			pFPParams[4].k[0] = 0;	pFPParams[4].k[1] = UVTexel.k[1];	pFPParams[4].k[2] = 0;	pFPParams[4].k[3] = 0;
			pFPParams[5].k[0] = 0;	pFPParams[5].k[1] = -UVTexel.k[1];	pFPParams[5].k[2] = 0;	pFPParams[5].k[3] = 0;
			pFPParams[6][0] = Time.GetTimeModulusScaled(0.01f, 1.0f);
			pFPParams[6][1] = Time.GetTimeModulusScaled(0.0087f, 1.0f);
			pFPParams[6][2] = Time.GetTimeModulusScaled(0.007f, 1.0f);
			pFPParams[6][3] = Time.GetTimeModulusScaled(0.13f, 1.0f);

			pFP->SetParameters(pFPParams, nFPParams);

			CRC_Attributes* pA = pVBM->Alloc_Attrib();
			if (!pA)
				break;

			pA->SetDefault();
			pA->m_pExtAttrib = pFP;
			pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
			pA->Attrib_TextureID(0, TextureID_Screen);
			pA->Attrib_TextureID(1, TextureID_Depth);
			pA->Attrib_TextureID(2, _pEngine->m_pShader->m_TextureID_Rand_03);
			CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(pVBM, pMat2D, VPRect16, 0xffffffff, DVPriority + 0.001f, pA);
			if (!pVB)
				break;

			pVB->Geometry_TVertexArray(pTVWhole, 0);
			pVBM->AddVB(pVB);
		}

		// Copy edge detection
		if (bDarknessVision)
			pVBM->AddCopyToTexture(DVPriority + 0.002f, VPRect, VPRect.p0, TextureID_LastScreen, false);

		CVec2Dfp32* pTVBlur = NULL;

		// Blur edges
		if (bDarknessVision)
		{
			CXR_GaussianBlurParams GP;
			GP.m_Rect = VPRect16;
			GP.m_Bias = 0;
			GP.m_Gamma = 1.0f;
			GP.m_UVCenter = CVec2Dfp32(0.5f, 0.5f);
			GP.m_Scale = 4.0f;
			GP.m_Exp = 4.0f;
			GP.m_SrcUVMin = UVMin;
			GP.m_SrcUVMax = UVMax;
			GP.m_DstUVMin = UVMin;
			GP.m_DstUVMax = UVMax;
			GP.m_SrcPixelUV = _pEngine->m_Screen_PixelUV;
			GP.m_DstPixelUV = _pEngine->m_Screen_PixelUV;
			GP.m_pVBM = pVBM;
			GP.m_Priority = DVPriority + 0.003f;
			GP.m_RenderCaps = _pEngine->m_RenderCaps_Flags;
			GP.m_TextureID_Src = TextureID_LastScreen;
			GP.m_TextureID_Dst = TextureID_BlurEdge;
			GP.m_Flags = 0;
			GP.m_nShrink = 1;
			GP.SetFilter();

			if (!CXR_Util::GaussianBlur(&GP, pTVBlur))
				break;
		}

		CRect2Duint16 VPRectTile11 = VPRect16;
		{
			VPRectTile11.m_Min[0] = VPRect16.m_Min[0] >> 1;
			VPRectTile11.m_Min[1] = VPRect16.m_Min[1] >> 1;
			VPRectTile11.m_Max[0] = VPRect16.m_Max[0] >> 1;
			VPRectTile11.m_Max[1] = VPRect16.m_Max[1] >> 1;
		}

		CRect2Duint16 VPRectTile12;
		{
			VPRectTile12 = VPRect16;
			VPRectTile12.m_Min[0] = VPRectTile11.m_Max[0];
			VPRectTile12.m_Max[1] = VPRectTile11.m_Max[1];

			if (!RenderDownsample(1, bRenderTextureVertFlip , pVP, pVBM, VPRectTile12, DVPriority + 0.100f, TextureID_Screen, TextureID_ScreenHalf, UVMin, UVMax, 0xffffffff))
				break;
		}

		CVec2Dfp32* pTVBlurScreen = NULL;

		// Blur screen
		CRect2Duint16 VPRectTile21;
		if (bDarknessVision)
		{
			VPRectTile21 = VPRectTile11;
			VPRectTile21.m_Min[1] = VPRectTile11.m_Max[1];
			VPRectTile21.m_Max[1] = VPRect16.m_Max[1];

			CXR_GaussianBlurParams GP;
			GP.m_Rect = VPRectTile21;
			GP.m_Bias = 0;
			GP.m_Gamma = 1.0f;
			GP.m_UVCenter = CVec2Dfp32(0.5f, 0.5f);
			GP.m_Scale = 16.0f;
			GP.m_Exp = 1.0f;
			GP.m_SrcUVMin = CVec2Dfp32(UVMaxHalf[0], UVMinHalf[1]);
			GP.m_SrcUVMax = CVec2Dfp32(UVMax[0], UVMaxHalf[1]);
			GP.m_DstUVMin = CVec2Dfp32(UVMinHalf[0], UVMaxHalf[1]);
			GP.m_DstUVMax = CVec2Dfp32(UVMaxHalf[0], UVMax[1]);
			GP.m_SrcPixelUV = _pEngine->m_Screen_PixelUV;
			GP.m_DstPixelUV = _pEngine->m_Screen_PixelUV;
			GP.m_pVBM = pVBM;
			GP.m_Priority = DVPriority + 0.12f;
			GP.m_RenderCaps = _pEngine->m_RenderCaps_Flags;
			GP.m_TextureID_Src = TextureID_ScreenHalf;
			GP.m_TextureID_Dst = TextureID_BlurScreen;
			GP.m_Flags = 0;
			GP.m_nShrink = 2;
			GP.SetFilter();

			if (!CXR_Util::GaussianBlur(&GP, pTVBlurScreen))
				break;
		}

		CRect2Duint16 VPRectTile22;
		CVec2Dfp32* pTVRadialBlur = NULL;
		bool bRadialBlurAdd = (m_RadialBlurMode == CRC_TEXENVMODE_COMBINE_ADD);
		bool bRadialBlurBlend = (bCreepingDark || m_bRadialBlur) && ((bDarknessVision && !bRadialBlurAdd) || !bDarknessVision);
		if (bCreepingDark || m_bRadialBlur)
		{
			VPRectTile22 = VPRect16;
			VPRectTile22.m_Min[0] = VPRectTile11.m_Max[0];
			VPRectTile22.m_Min[1] = VPRectTile11.m_Max[1];

			CXR_RadialBlurParams RB;
			RB.m_Priority = DVPriority + 1.2f;
			RB.m_ColorScale = m_RadialBlurColorScale;
			RB.m_Power = MaxMT(m_CreepingDark, m_RadialBlur);
			RB.m_PowerScale = 6.0f;
			RB.m_PixelScale = 4.0f;
			RB.m_Affection = m_RadialBlurAffection;
			RB.m_ColorIntensity = m_RadialBlurColorIntensity;
			RB.m_RadialCenter = m_RadialBlurCenter;
			RB.m_UVCenter = CVec2Dfp32(0.5f, 0.5f);
			RB.m_UVExtra = m_RadialBlurUVExtra;
			RB.m_RenderCaps = _pEngine->m_RenderCaps_Flags;
			RB.m_TextureID_Src = TextureID_ScreenHalf;
			RB.m_TextureID_Dst = TextureID_RadialBlur;
			RB.m_SrcUVMin = CVec2Dfp32(UVMaxHalf[0], UVMin[1]);
			RB.m_SrcUVMax = CVec2Dfp32(UVMax[0], UVMaxHalf[1]);
			RB.m_Rect = VPRectTile22;
			RB.m_pVBM = pVBM;
			RB.m_Screen_PixelUV = _pEngine->m_Screen_PixelUV;
			RB.SetFilterParams(m_lRadialBlurFilterParams, m_nRadialBlurFilterParams);

			RB.SetFilter(m_pRadialBlurFilter);
			RB.SetBlurSpace();

			if (!CXR_Util::RadialBlur(&RB, pTVRadialBlur))
				break;
		}

		if (bDarknessVision)
		{
			CRC_ExtAttributes_FragmentProgram20* pFP = (CRC_ExtAttributes_FragmentProgram20*) pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
			if (!pFP)
				break;
			pFP->Clear();
			pFP->SetProgram("WClientMod_DV3_1", 0);

			int nFPParams = 1;
			CVec4Dfp32* pFPParams = pVBM->Alloc_V4(nFPParams);

			CRC_Attributes* pA = pVBM->Alloc_Attrib();
			if (!pA)
				break;

			pA->SetDefault();
//			pA->m_pExtAttrib = pFP;
			pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
			pA->Attrib_TextureID(0, TextureID_LastScreen);
			pA->Attrib_TextureID(1, TextureID_Screen);
			pA->Attrib_TextureID(2, TextureID_BlurEdge);
			pA->Attrib_TextureID(3, TextureID_BlurScreen);
			//pA->Attrib_TextureID(4, _pEngine->m_pShader->m_TextureID_Special_ff000000);
			pA->Attrib_TextureID(4, (bRadialBlurAdd && (bCreepingDark || m_bRadialBlur)) ? TextureID_RadialBlur : _pEngine->m_pShader->m_TextureID_Special_ff000000);
			//pA->Attrib_TextureID(4, (bRadialBlurAdd && (bCreepingDark || m_bRadialBlur)) ? TextureID_RadialBlur : _pEngine->m_pShader->m_TextureID_Special_ff000000);

			CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(pVBM, pMat2D, VPRect16, 0xffffffff, DVPriority + 2.000f, pA);
			if (!pVB)
				break;

			pFPParams[0][0] = _pEngine->m_Screen_PixelUV[0];
			pFPParams[0][1] = _pEngine->m_Screen_PixelUV[1];
			pFPParams[0][2] = 0;
			pFPParams[0][3] = DVIntensity*1.25f;
			pFP->SetParameters(pFPParams, nFPParams);
			pA->m_pExtAttrib = pFP;

			pVB->Geometry_TVertexArray(pTVWhole, 0);
			pVB->Geometry_TVertexArray(pTVBlur, 1);
			pVB->Geometry_TVertexArray(pTVBlurScreen, 2);
			pVB->Geometry_TVertexArray(pTVRadialBlur, 3);
			pVBM->AddVB(pVB);

		}

		if (bRadialBlurBlend)
		{
			// No blending with darkness vision, just blend original and streak texture togheter.
			CRC_Attributes* pA = pVBM->Alloc_Attrib();
			if (!pA)
				break;

			CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(pVBM, pMat2D, VPRect16, 0xffffffff, DVPriority + 2.002f, pA);
			if (!pVB)
				break;

#ifdef PLATFORM_XENON
			// Could we support this TexEnvMode on Xenon maybe ?
			if (m_RadialBlurMode == CRC_TEXENVMODE_COMBINE_ADD)
			{
				pA->SetDefault();
				pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
				pA->Attrib_TextureID(0, TextureID_Screen);
				pVB->Geometry_TVertexArray(pTVWhole, 0);
				pVBM->AddVB(pVB);

				pA = pVBM->Alloc_Attrib();
				if (!pA)
					break;

				pVB = CXR_Util::VBM_RenderRect(pVBM, pMat2D, VPRect16, 0xffffffff, DVPriority + 2.004f, pA);
				if (!pVB)
					break;

				pA->SetDefault();
				pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
				pA->Attrib_TextureID(0, TextureID_RadialBlur);
				pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
				pVB->Geometry_TVertexArray(pTVRadialBlur, 0);
				pVBM->AddVB(pVB);
			}
			else
#endif
			{
				pA->SetDefault();
				pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
				pA->Attrib_TextureID(0, TextureID_Screen);
				pA->Attrib_TextureID(1, TextureID_RadialBlur);
				pA->Attrib_TexGen(0, CRC_TEXGENMODE_TEXCOORD, CRC_TEXGENCOMP_ALL);
				pA->Attrib_TexGen(1, CRC_TEXGENMODE_TEXCOORD, CRC_TEXGENCOMP_ALL);
				pA->Attrib_TexEnvMode(1, m_RadialBlurMode);

				pVB->Geometry_TVertexArray(pTVWhole, 0);
				pVB->Geometry_TVertexArray(pTVRadialBlur, 1);

				pVBM->AddVB(pVB);
			}
		}
	}
	while(0);

	// Otherworld grain
	if (bOtherWorld)
	do
	{
		pVBM->AddCopyToTexture(DVPriority + 3.000f, VPRect, VPRect.p0, TextureID_Screen, false);

		{
			CRC_ExtAttributes_FragmentProgram20* pFP = (CRC_ExtAttributes_FragmentProgram20*) pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
			if (!pFP)
				break;
			pFP->Clear();
			pFP->SetProgram("WClientMod_OW1_1", 0);

			int nFPParams = 9;
			CVec4Dfp32* pFPParams = pVBM->Alloc_V4(nFPParams);

			CRC_Attributes* pA = pVBM->Alloc_Attrib();
			if (!pA || !pFPParams)
				break;

			pA->SetDefault();
			//			pA->m_pExtAttrib = pFP;
			pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
			pA->Attrib_TextureID(0, TextureID_Screen);
			pA->Attrib_TextureID(1, _pEngine->m_pShader->m_TextureID_Rand_04);
			pA->Attrib_TextureID(2, TextureID_Depth);

			CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(pVBM, pMat2D, VPRect16, 0xffffffff, DVPriority + 3.001f, pA);
			if (!pVB)
				break;

			pFPParams[0][0] = _pEngine->m_Screen_PixelUV[0];
			pFPParams[0][1] = _pEngine->m_Screen_PixelUV[1];
			pFPParams[0][2] = _pEngine->m_Screen_PixelUV[0];
			pFPParams[0][3] = _pEngine->m_Screen_PixelUV[1];
			CMTime Time = _pEngine->GetEngineTime();
			pFPParams[1][0] = Time.GetTimeModulusScaled(0.01f, 1.0f);
			pFPParams[1][1] = Time.GetTimeModulusScaled(0.0087f, 1.0f);
			pFPParams[1][2] = Time.GetTimeModulusScaled(0.007f, 1.0f);
//			pFPParams[1][3] = Time.GetTimeModulusScaled(0.13f, 1.0f);
			pFPParams[1][3] = 1.0f / _pEngine->m_PostProcess_PrevLevels;

			fp32 Front = pVP->GetFrontPlane();
			fp32 Back = pVP->GetBackPlane();
			pFPParams[2].k[0] = Front;
			pFPParams[2].k[1] = Back;
			pFPParams[2].k[2] = 1.0f / pVP->GetBackPlane();
			pFPParams[2].k[3] = shf / pVP->GetYScale();
			pFPParams[3].k[0] = 2.0f * Back;
			pFPParams[3].k[1] = Back + Front;
			pFPParams[3].k[2] = 2.0f * Back * Front;
			pFPParams[3].k[3] = 2.0f * (Back - Front);
			pFPParams[4].k[0] = 0.0f;
			pFPParams[4].k[1] = 0.0f;
			pFPParams[4].k[2] = swf / pVP->GetXScale();
			pFPParams[4].k[3] = shf / pVP->GetYScale();

			const CMat4Dfp32& V2WMat = _pEngine->GetVC()->m_CameraWMat;
			V2WMat.Transpose(*(CMat4Dfp32*)(pFPParams+5));

			pFP->SetParameters(pFPParams, nFPParams);
			pA->m_pExtAttrib = pFP;

			const fp32 TextureRandSizeRcp = 1.0f / 128.0f;
			pVB->Geometry_TVertexArray(pTVWhole, 0);
			pVB->Geometry_TVertexArray(CXR_Util::VBM_CreateRectUV(pVBM, CVec2Dfp32(0.0f, 0.0f), CVec2Dfp32(swf * TextureRandSizeRcp, shf * TextureRandSizeRcp)), 1);
			pVB->Geometry_TVertexArray(CXR_Util::VBM_CreateRectUV(pVBM, CVec2Dfp32(-1.0f, -1.0f), CVec2Dfp32(1.0f, 1.0f)), 2);
			pVBM->AddVB(pVB);
		}

		pVBM->AddCopyToTexture(DVPriority + 3.002f, VPRect, VPRect.p0, TextureID_LastScreen, false);

		CVec2Dfp32* pTVShrink = NULL;
		{
			CXR_ShrinkParams SP;
			SP.m_SrcUVMin = UVMin;
			SP.m_SrcUVMax = UVMax;
			SP.m_DstUVMin = UVMin;
			SP.m_DstUVMax = UVMax;
			SP.m_nShrink = 1;
			SP.m_SrcPixelUV = _pEngine->m_Screen_PixelUV;
			SP.m_RenderCaps = _pEngine->m_RenderCaps_Flags;
			SP.m_Priority = DVPriority + 3.003f;
			SP.m_pVBM = pVBM;
			SP.m_TextureID_Src = TextureID_LastScreen;
			SP.m_TextureID_Dst = TextureID_LastScreen;
			SP.m_Rect = VPRect16;
			if (!CXR_Util::ShrinkTexture(&SP, pTVShrink))
				break;
		}

		{
			CRC_ExtAttributes_FragmentProgram20* pFP = (CRC_ExtAttributes_FragmentProgram20*) pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
			if (!pFP)
				break;
			pFP->Clear();
			pFP->SetProgram("WClientMod_OW1_2", 0);

			int nFPParams = 1;
			CVec4Dfp32* pFPParams = pVBM->Alloc_V4(nFPParams);

			CRC_Attributes* pA = pVBM->Alloc_Attrib();
			if (!pA || !pFPParams)
				break;

			pA->SetDefault();
			pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
			pA->Attrib_TextureID(0, TextureID_Screen);
			pA->Attrib_TextureID(1, TextureID_LastScreen);

			CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(pVBM, pMat2D, VPRect16, 0xffffffff, DVPriority + 3.05f, pA);
			if (!pVB)
				break;

			pFPParams[0].SetScalar(0.0f);
			pFP->SetParameters(pFPParams, nFPParams);
			pA->m_pExtAttrib = pFP;

			pVB->Geometry_TVertexArray(pTVWhole, 0);
			pVB->Geometry_TVertexArray(pTVShrink, 1);
			pVBM->AddVB(pVB);

		}

		CXR_Engine_PostProcessParams& PPP = _pEngine->m_PostProcessParams_Current;
		PPP.m_GlowBias.SetScalar(-0.05f);
		PPP.m_GlowScale.SetScalar(1.15f);
		PPP.SetHSC(0, 1.0f, 1.0f, m_TextureID_OWHSC);
		PPP.m_nHSC = 1;
	}
	while(0);

	// x06 Screen resolve (Good huh? *sigh* We where supposed to get rid of this!)
//	pVBM->AddCopyToTexture(DVPriority + 3.06f, VPRect, VPRect.p0, TextureID_LastScreen, true);

	// Render retina dots
	if (bRetinaDots)
	do 
	{
		// TODO: Clean up and improve this...

		CXR_VertexBuffer* pVBDots0 = pVBM->Alloc_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES | CXR_VB_TVERTICES0 | CXR_VB_TVERTICES1, 4);
		CXR_VertexBuffer* pVBDots1 = pVBM->Alloc_VB(CXR_VB_TVERTICES1, 4);
		CMat4Dfp32* pMat0 = pVBM->Alloc_M4();
		CMat4Dfp32* pMat1 = pVBM->Alloc_M4();
		
		if (!pVBDots0 || !pVBDots1 || !pMat0 || !pMat1)
			break;

		CXR_VBChain* pVBChain0 = pVBDots0->GetVBChain();
		CXR_VBChain* pVBChain1 = pVBDots1->GetVBChain();
		CRC_Attributes* pA0 = pVBDots0->m_pAttrib;
		
		pVBDots1->Geometry_VertexArray(pVBChain0->m_pV, 4, true);
		pVBDots1->Geometry_TVertexArray(pVBChain0->m_pTV[0], 0, 2);
		pVBDots1->m_pAttrib = pA0;

		int iTextureID_Retina = _pEngine->GetTC()->GetTextureID("Retina");
		int iTextureID_Dots = _pEngine->GetTC()->GetTextureID("Dots");

		pA0->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
		pA0->Attrib_RasterMode(CRC_RASTERMODE_ADD);

		pA0->Attrib_TexGen(0, CRC_TEXGENMODE_TEXCOORD, CRC_TEXGENCOMP_ALL);
		pA0->Attrib_TexGen(1, CRC_TEXGENMODE_TEXCOORD, CRC_TEXGENCOMP_ALL);
		pA0->Attrib_TextureID(0, iTextureID_Retina);
		pA0->Attrib_TextureID(1, iTextureID_Dots);
		pA0->Attrib_TexEnvMode(1, CRC_TEXENVMODE_COMBINE_MULTIPLY2);

		CRect2Duint16 VPRectDouble16;
		uint16 HalfMaxX = VPRect16.m_Max.k[0] / 2;
		uint16 HalfMaxY = VPRect16.m_Max.k[1] / 2;
		VPRectDouble16.m_Min.k[0] = VPRect16.m_Min.k[0] - HalfMaxX;
		VPRectDouble16.m_Min.k[1] = VPRect16.m_Min.k[1] - HalfMaxY;
		VPRectDouble16.m_Max.k[0] = VPRect16.m_Max.k[0] + HalfMaxX;
		VPRectDouble16.m_Max.k[1] = VPRect16.m_Max.k[1] + HalfMaxY;

		// Setup priority, vertices and texture coordinates for retina
		CXR_Util::VBM_CreateRectUV(0.0f, 1.0f, 0, (CVec2Dfp32*)pVBChain0->m_pTV[0]);
		CXR_Util::VBM_CreateRect(pMat2D, VPRect16, pVBChain0->m_pV);
		pVBDots0->m_Priority = DVPriority + 1000.0f;
		pVBDots1->m_Priority = DVPriority + 1000.1f;

		const fp32 Time = _pAnimState->m_AnimTime0.GetTime();

		// Setup texture coordinates for retina dots
		static fp32 sScale0 = 1.0f;
		static fp32 sScale1 = 1.0f;
		static fp32 sUVMax =  2.0f;
		static fp32 sUVMin = -1.0f;
		static fp32 sSinSpeed0 = 0.5f;
		static fp32 sSinSpeed1 = 0.5f;
		static fp32 sScaleSin0 = 2.0f;
		static fp32 sScaleSin1 = 2.0f;
		CVec2Dfp32* pTV0_1 = (CVec2Dfp32*)pVBChain0->m_pTV[1];
		CVec2Dfp32* pTV1_1 = (CVec2Dfp32*)pVBChain1->m_pTV[1];
		CXR_Util::VBM_CreateRectUV(sUVMin * sScale0, sUVMax * sScale0, 0, pTV0_1);
		CXR_Util::VBM_CreateRectUV(sUVMin * sScale1, sUVMax * sScale1, 1, pTV1_1);

		static fp32 sSpeed0 = 0.1f;
		static fp32 sSpeed1 = 0.05f;
		fp32 UVScroll0 = Time * sSpeed0;
		fp32 UVScroll1 = Time * sSpeed1;
		for (uint i = 0; i < 4; i++)
		{
			pTV0_1[i].k[0] += UVScroll0;
			pTV1_1[i].k[0] -= UVScroll1;
		}

		CXR_Util::Init();
		pVBDots0->Render_IndexedTriangles(CXR_Util::m_lQuadParticleTriangles, 2);
		pVBDots1->Render_IndexedTriangles(CXR_Util::m_lQuadParticleTriangles, 2);

		// Add vbs
		pVBM->AddVB(pVBDots0);
		pVBM->AddVB(pVBDots1);
	} while(0);
}

void CXR_Model_CameraEffects::OnPrecache(class CXR_Engine* _pEngine, int _iVariation)
{
	_pEngine->m_pTC->SetTextureParam(m_TextureID_OWHSC, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
}


// -------------------------------------------------------------------
// Misc old unused stuff saved for reference
// -------------------------------------------------------------------
#if 0

/*		else
{
// Blend creepdark texture onto screen
CRC_Attributes* pA = pVBM->Alloc_Attrib();
if (!pA)
break;

pA->SetDefault();
pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
pA->Attrib_TextureID(0, TextureID_Screen);
pA->Attrib_TextureID(1, TextureID_CreepingDark);
pA->Attrib_TexEnvMode(1, CRC_TEXENVMODE_COMBINE_ADD);
CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(pVP, pVBM, VPRect16, 0xffffffff, DVPriority + 2.000f, pA);
if (!pVB)
break;

pVB->Geometry_TVertexArray(pTVWhole, 0);
pVB->Geometry_TVertexArray(pTVHalf, 1);
pVBM->AddVB(pVB);
}*/

// E3 resolve screen
//pVBM->AddCopyToTexture(DVPriority + 2.001f, VPRect, VPRect.p0, TextureID_E3Resolve, false);

#if 0
#ifndef M_RTM
do
{
	if(!(_pAnimState->m_Anim0 & CXR_MODEL_DV_SHOWTEXTURES))
		break;

	CRect2Duint16 VPRectTex = VPRect16;
	if(!(_pAnimState->m_Anim0 & CXR_MODEL_DV_SHOWFULLSCREEN))
	{
		VPRectTex.m_Min[0] >>= 1;
		VPRectTex.m_Min[1] >>= 1;
		VPRectTex.m_Max[0] >>= 1;
		VPRectTex.m_Max[1] >>= 1;
	}

	CVec2Dfp32 UVMinShowTex = UVMin;
	CVec2Dfp32 UVMaxShowTex = UVMax;
	CRC_Attributes* pA = pVBM->Alloc_Attrib();
	if(!pA)
		break;

	int TextureID = 0;
	switch(_pAnimState->m_Anim1)
	{
	case 0:
		TextureID = TextureID_Screen;
		break;
	case 1:
		TextureID = TextureID_LastScreen;
		break;
	case 2:
		if(bDarknessVision)
		{
			TextureID = TextureID_Frame;
			break;
		}

		TextureID = 0;
		break;
	case 3:
		if(bDarknessVision)
		{
			TextureID = TextureID_FinalBlur;
			break;
		}

		TextureID = 0;
		break;
	case 4:
		if(bCreepingDark)
		{
			TextureID = TextureID_CreepingDark;
			UVMinShowTex = UVMinShowTex * 0.5f;
			UVMaxShowTex = UVMaxShowTex * 0.5f;
			break;
		}

		TextureID = 0;
		break;
	}

	CVec2Dfp32* pTV = (bRenderTextureVertFlip) ? 
		CXR_Util::VBM_CreateRectUV_VFlip(pVBM, UVMinShowTex, UVMaxShowTex) :
	CXR_Util::VBM_CreateRectUV(pVBM, UVMinShowTex, UVMaxShowTex);
	if(!pTV)
		break;

	pA->SetDefault();
	pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
	pA->Attrib_TextureID(0, TextureID);
	pA->Attrib_RasterMode(CRC_RASTERMODE_NONE);
	CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(pVBM, pMat2D, VPRectTex, 0xffffffff, PreDestroyScreenPriority + 25.4f, pA);
	if(!pVB)
		break;

	pVB->Geometry_TVertexArray(pTV, 0);
	pVBM->AddVB(pVB);
} while(0);
#endif
#endif

// Ancient weapons vision
/*
do
{
// If creeping dark is active, we will fix color tinting and perturbation in
// that shader, since we have the same effect there also.
if(!bAncientWeapon || bCreepingDark)
break;

CRC_Attributes* pA = pVBM->Alloc_Attrib();
pA->SetDefault();

CRC_ExtAttributes_FragmentProgram20* pFP = (CRC_ExtAttributes_FragmentProgram20*) pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
if(!pFP)
break;

pFP->Clear();
pFP->SetProgram("WClientMod_GV", 0);

int nFPParams = 1;
CVec4Dfp32* pFPParams = pVBM->Alloc_V4(nFPParams);

pFPParams[0][0] = 0.0f;
pFPParams[0][1] = 0.0f + (0.3f * m_AncientWeapon);
pFPParams[0][2] = 0.0f + (0.1f * m_AncientWeapon);
pFPParams[0][3] = 0.0f;

pFP->SetParameters(pFPParams, nFPParams);
pA->m_pExtAttrib = pFP;

CVec2Dfp32* pTV0 = (bRenderTextureVertFlip) ? CXR_Util::VBM_CreateRectUV_VFlip(pVBM, 0, 1) :
CXR_Util::VBM_CreateRectUV(pVBM, 0, 1);
CVec4Dfp32* pTV1 = pVBM->Alloc_V4(4);
CVec4Dfp32* pTV2 = pVBM->Alloc_V4(4);

const fp32 Perturb = _pAnimState->m_AnimAttr1;

pTV1[0] = CVec4Dfp32(Perturb, 0, UVMax[0], 0);
pTV1[1] = CVec4Dfp32(Perturb, 0, 0, 0);
pTV1[2] = CVec4Dfp32(Perturb, 0, 0, 0);
pTV1[3] = CVec4Dfp32(Perturb, 0, UVMax[0], 0);

if(bRenderTextureVertFlip)
{
pTV2[0] = CVec4Dfp32(0, Perturb, 1.0f - UVMax[1], 0);
pTV2[1] = CVec4Dfp32(0, Perturb, 1.0f - UVMax[1], 0);
pTV2[2] = CVec4Dfp32(0, Perturb, 1.0f, 0);
pTV2[3] = CVec4Dfp32(0, Perturb, 1.0f, 0);
}
else
{
pTV2[0] = CVec4Dfp32(0, Perturb, UVMax[1], 0);
pTV2[1] = CVec4Dfp32(0, Perturb, UVMax[1], 0);
pTV2[2] = CVec4Dfp32(0, Perturb, 0, 0);
pTV2[3] = CVec4Dfp32(0, Perturb, 0, 0);
}

pA->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
pA->Attrib_TextureID(0, TextureID_LastScreen);
//		fp32 Prio = 0.0f;
//if(!bDarknessVision)
//{
//	pA->Attrib_RasterMode(CRC_RASTERMODE_NONE);
//	Prio -= 0.002f;
//}
//else
//{
//	pA->Attrib_RasterMode(CRC_RASTERMODE_DESTALPHABLEND);
//	Prio += 0.6f;
//}
if(!bDarknessVision)
{
pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
//			Prio = (-0.002f);
}
else
{
pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
//			Prio = (CXR_VBPRIORITY_MODEL_OPAQUE + CXR_VBPRIORITY_VOLUMETRICFOG + 0.2f);
}

//		pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
//		Prio = (CXR_VBPRIORITY_MODEL_OPAQUE + CXR_VBPRIORITY_VOLUMETRICFOG) + ((bDarknessVision) ? 0.2f : 0.0f);
CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(pVP, pVBM, VPRect16, 0xffffffff, PreDestroyScreenPriority + 0.1, pA);
if(!pVB)
break;

pVB->Geometry_TVertexArray((fp32*)pTV0, 0, 2);
pVB->Geometry_TVertexArray((fp32*)pTV1, 1, 4);
pVB->Geometry_TVertexArray((fp32*)pTV2, 2, 4);

pVBM->AddVB(pVB);

} while(0);
*/


// Create shadow mask for captured screen
//	if (bCreepingDark || bAncientWeapon || bDarknessVision)
/*	{
do
{
if(!RenderQuad(pVP, pVBM, VPRect16, bRenderTextureVertFlip, UVMin, UVMax, 0, 0x0, PreDestroyScreenPriority - 0.05f, CRC_FLAGS_ALPHAWRITE, CRC_FLAGS_COLORWRITE | CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE))
break;

// Capture screen first
{
CXR_PreRenderData_InitiateTexturePrecache* pData = (CXR_PreRenderData_InitiateTexturePrecache*)pVBM->Alloc(sizeof(CXR_PreRenderData_InitiateTexturePrecache));
if (!pData)
break;

pData->m_TextureID = TextureID_Screen;
pVBM->AddCallback(CXR_PreRenderData_InitiateTexturePrecache::InitiateTexturePrecache, pData, PreDestroyScreenPriority + 0.00f);
//	After we capture screen disable useless stuff
pVBM->AddRenderOptions(PreDestroyScreenPriority + 0.02f, CRC_RENDEROPTION_NOZBUFFER | CRC_RENDEROPTION_NOANTIALIAS);
}

// Restore screen after some work is done
CVec2Dfp32* pTV = (bRenderTextureVertFlip) ? 
CXR_Util::VBM_CreateRectUV_VFlip(pVBM, UVMin, UVMax) :
CXR_Util::VBM_CreateRectUV(pVBM, UVMin, UVMax);
CRC_Attributes* pA = pVBM->Alloc_Attrib();
if (!pTV || !pA)
break;

pA->SetDefault();
pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
pA->Attrib_TextureID(0, TextureID_Screen);
CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(pVP, pVBM, VPRect16, 0xffffffff, PreDestroyScreenPriority + 0.05f, pA);
if (!pVB)
break;
pVB->Geometry_TVertexArray(pTV, 0);
pVBM->AddVB(pVB);
}
while (0);
}*/
#endif

