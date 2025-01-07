#include "PCH.h"
#include "WObj_TentacleSystem_ClientData.h"

#include "../WObj_Char/WObj_CharPlayer_ClientData.h"
#include "../WObj_Char/WObj_CharPlayer.h"
#include "../Models/WModel_EffectSystem.h"
#include "../../../../Shared/MOS/XR/XREngineImp.h"
#include "../../../../Shared/MOS/XR/XREngineVar.h"

#define sizeof_buffer(buf) (sizeof(buf)/sizeof(buf[0]))


#define TENTACLE_DEBUG_USE_STATIC 0
#if TENTACLE_DEBUG_USE_STATIC
	static bool DEBUG_RENDER_BONE_HIERARCHY = 0;
	static bool DEBUG_RENDER_HEART = 1;
	static bool DEBUG_RENDER_SPLINE_ORIGINAL = 1;
	static bool DEBUG_RENDER_SPLINE_MODIFIED = 1;
	static bool HIDE_MODELS_WHEN_RENDERING_WIREFRAME = 0;
#else
	#define DEBUG_RENDER_BONE_HIERARCHY 0
	#define DEBUG_RENDER_HEART 0
	#define DEBUG_RENDER_SPLINE_ORIGINAL 1
	#define DEBUG_RENDER_SPLINE_MODIFIED 1
	#define HIDE_MODELS_WHEN_RENDERING_WIREFRAME 0
#endif


enum
{
	class_CharPlayer =			MHASH6('CWOb','ject','_','Char','Play','er'),
	class_CharNPC =				MHASH5('CWOb','ject','_','Char','NPC'),
};


// TODO: Precalc this, instead of fetching in each render call
struct CTemplateMesh
{
	CXR_Model* pMesh;
	CXR_Skeleton* pSkeleton;
	uint nBones;
	uint iStartBone, iEndBone;
	fp32 Length;

	bool Init(uint16 _iResource, CMapData* _pMapData)
	{
		pMesh = _pMapData->GetResource_Model(_iResource);
		if (!pMesh)
			return false;

		pSkeleton = pMesh->GetSkeleton();
		if (!pSkeleton)
			return false;

		nBones = pSkeleton->m_lNodes.Len();
		if (nBones < 2)
			return false;

		fp32 xMin = _FP32_MAX, xMax = -_FP32_MAX;
		iStartBone = iEndBone = 0;
		for (uint i = 0; i < nBones; i++)
		{
			fp32 x = pSkeleton->m_lNodes[i].m_LocalCenter.k[0];
			if (x < xMin) 
			{
				iStartBone = i;
				xMin = x;
			}
			if (x > xMax)
			{
				iEndBone = i;
				xMax = x;
			}
		}
		Length = xMax - xMin;
		return true;
	}
};


struct SPowerStreak
{
	CXR_Model*		m_lpModel[NUM_POWERSTREAKS];		// Tentacle model
	CMat4Dfp32		m_lMat[NUM_POWERSTREAKS];			// Position
	CXR_AnimState	m_lAnimState[NUM_POWERSTREAKS];		// Anim state for tattoo rendering
	CBox3Dfp32		m_lBoundBoxW[NUM_POWERSTREAKS];		// Bounding box for model
	fp32			m_PowerFade;						// Fade value inbetween powers
	fp32			m_Darkness;
	int8			m_iPower;							// Selected power (or 'tattoo')
//	int8			m_iLastPower;						// Last selected power (or 'tattoo')
	uint8			m_Power;							// Amount of darkness power left
	uint8			m_MaxPower;							// Max amount of darkness power

	SPowerStreak()
	{
		for (uint i = 0; i < NUM_POWERSTREAKS; i++)
			m_lpModel[i] = NULL;
	}

	bool IsValid(uint _iSlot) const
	{
		return (m_lpModel[_iSlot] != NULL);
	}

	void Setup(uint _iSlot, CXR_Model* _pModel, const CMat4Dfp32& _Mat, const CXR_AnimState& _AnimState, const CBox3Dfp32& _BoundBoxW)
	{
		m_lpModel[_iSlot] = _pModel;
		m_lMat[_iSlot] =_Mat;
		m_lAnimState[_iSlot] = _AnimState;
		m_lBoundBoxW[_iSlot] = _BoundBoxW;
	}

	void Setup(uint _iPower, fp32 _PowerFade, fp32 _Darkness, uint8 _Power, uint8 _MaxPower)
	{
		m_iPower = _iPower;
	//	m_iLastPower = _iLastPower;
		m_PowerFade = _PowerFade;
		m_Power = _Power;
		m_MaxPower = _MaxPower;
		m_Darkness = _Darkness;
	}

	void Render(CXR_Engine* _pEngine, CXR_Model_PowerStreaks* _pPowerStreaksModel, fp32 _GameTickTime, fp32 _IPTime, CWireContainer* _pWC);   // would be nice if const, but m_lAnimState[] breaks that...
};




static void DebugRenderSpline(const CSpline_Tentacle& _Spline, CWireContainer& _WC, bool _bModified)
{
	uint32 ColorChachePoint = CPixel32(0,255,0,255);
	uint32 ColorPoint = CPixel32(128,0,128,255);
	uint32 ColorSpline = CPixel32(255,0,255,255);
	if (_bModified)
	{
		ColorChachePoint = CPixel32(0,0,255,255);
		ColorPoint = CPixel32(128,128,0,255);
		ColorSpline = CPixel32(255,255,0,255);
	}

	// Examine animation up vectors
	if (!_bModified)
	{
		if (_Spline.m_lSegments.Len())
		{
			CMat4Dfp32 ResMat;
			CVec3Dfp32 CurrUp = _Spline.m_EndMat.GetRow(2);
			const CSpline_Tentacle::Segment& s = _Spline.m_lSegments[_Spline.m_lSegments.Len() - 1];
			for (int i = _Spline.m_nCachePoints - 1; i >= 0; i--)
			{
				_Spline.CalcMat(s.m_Cache[i].t, ResMat, CurrUp);
				const CVec3Dfp32& Pos = ResMat.GetRow(3);
				_WC.RenderWire(Pos, Pos + ResMat.GetRow(0), CPixel32(255,0,0,255), 0.0f, false);
				_WC.RenderWire(Pos, Pos + ResMat.GetRow(1), CPixel32(0,255,0,255), 0.0f, false);
				_WC.RenderWire(Pos, Pos + ResMat.GetRow(2), CPixel32(0,0,255,255), 0.0f, false);
			}
		}
	}

	for (uint iSeg = 0; iSeg < _Spline.m_lSegments.Len(); iSeg++)
	{
		const CSpline_Tentacle::Segment& s = _Spline.m_lSegments[iSeg];
		CVec3Dfp32 Pos, PrevPos;
		for (uint i = 0; i < _Spline.m_nCachePoints; i++)
		{
			_Spline.CalcPos(s.m_Cache[i].t, Pos);
			//_WC.RenderVertex(Pos, 0xff00ff00, 0.0f, false);
			_WC.RenderWire(Pos - CVec3Dfp32(0.5f,0,0), Pos + CVec3Dfp32(0.5f,0,0), ColorChachePoint, 0.0f, false);
			_WC.RenderWire(Pos - CVec3Dfp32(0,0.5f,0), Pos + CVec3Dfp32(0,0.5f,0), ColorChachePoint, 0.0f, false);
			_WC.RenderWire(Pos - CVec3Dfp32(0,0,0.5f), Pos + CVec3Dfp32(0,0,0.5f), ColorChachePoint, 0.0f, false);

			if (i > 0)
				_WC.RenderWire(PrevPos, Pos, ColorSpline, 0.0f, false);
			PrevPos = Pos;
		}
	}

	for (uint iPoint = 0; iPoint < _Spline.m_lPoints.Len(); iPoint++)
	{
		const CSpline_Tentacle::Point& p = _Spline.m_lPoints[iPoint];
		_WC.RenderVertex(p.m_Pos, ColorPoint, 0.0f, false);
//		_WC.RenderVector(p.m_Pos, -p.m_TangentIn, 0xff808080, 0.0f, false);
//		_WC.RenderVector(p.m_Pos, p.m_TangentOut, 0xff808080, 0.0f, false);
	}
}


CXR_Model_PowerStreaks::CXR_Model_PowerStreaks()
{
}


void CXR_Model_PowerStreaks::OnRender(CXR_Engine *_pEngine, CRenderContext *_pRender, CXR_VBManager *_pVBM, CXR_ViewClipInterface *_pViewClip,
								 spCXR_WorldLightState _spWLS, const CXR_AnimState *_pAnimState, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat, int _Flags)
{
	// Only rendered when power is full
	CRC_Viewport* pVP = _pVBM->Viewport_Get();
	CTextureContainer_Screen* pTCScreen = safe_cast<CTextureContainer_Screen>(_pEngine->GetInterface(XR_ENGINE_TCSCREEN));
	aint RenderType = _pAnimState->m_Data[0];
	CXR_Model* pModel = (CXR_Model*)_pAnimState->m_Data[1];

	if (!pVP || !pTCScreen)
		return;

	// Calculate amount of vb memory needed
	CFXVBMAllocUtil AllocUtil;
	AllocUtil.Alloc_M4();

	if (RenderType < POWERSTREAKS_RENDER_MODEL_MAX)
	{
		if (!pModel)
			return;

		AllocUtil.Alloc_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES, 4);
		AllocUtil.Alloc_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES, 4);
		AllocUtil.Alloc_FragmentProgram20(2);
		AllocUtil.Alloc_CopyToTexture(1);
		AllocUtil.Alloc_V2(4);
	}
	else if (RenderType == POWERSTREAKS_RENDER_SCREEN)
	{
#ifdef PLATFORM_XENON
		AllocUtil.Alloc_CopyToTexture(1);
		AllocUtil.Alloc_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES, 4);
#endif
		AllocUtil.Alloc_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES, 4);
		AllocUtil.Alloc_V2(4);
	}

	// Allocate and run effect
	if (AllocUtil.Alloc(_pVBM))
	{
		CMat4Dfp32* pMat2D = AllocUtil.Get_M4();
		pVP->Get2DMatrix_RelBackPlane(*pMat2D);

		// Set start priority for model effects
		fp32 ScreenPrio = CXR_VBPRIORITY_UNIFIED_MATERIAL + 4999;									// 13191.0
		fp32 PowerPriority = (CXR_VBPRIORITY_MODEL_TRANSPARENT - 1) + (0.1f * fp32(RenderType));	// 2062.1 / 2062.2 / 2062.4

		// Get screen setting
		CRct VPRect = pVP->GetViewRect();
		int bRenderTextureVertFlip  = _pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP;
		CRect2Duint16 VPRect16(CVec2Duint16(VPRect.p0.x, VPRect.p0.y), CVec2Duint16(VPRect.p1.x, VPRect.p1.y));
		CPnt Dst(VPRect.p0);
		fp32 swfp32 = fp32(VPRect.GetWidth());
		fp32 shfp32 = fp32(VPRect.GetHeight());

		// Get screen width/height
		
		
		// Calculate bounding box for model
		if (RenderType < POWERSTREAKS_RENDER_MODEL_MAX)
		{
			CVec2Dfp32 UVTexelHalfRcp = _pEngine->m_Screen_PixelUV * 2.0f;
			
			// TextureID to work with
			int TextureID_Temp = (RenderType == POWERSTREAKS_RENDER_MODEL_1) ? _pEngine->m_TextureID_DeferredDiffuse : _pEngine->m_TextureID_DeferredSpecular;

			// Calculate rectangle on screen that the effect takes up
			CBox3Dfp32 BoundBoxW = *(CBox3Dfp32*)_pAnimState->m_Data[2];
			CScissorRect Scissor;
			uint32 MinX, MinY, MaxX, MaxY;
			CFXSysUtil::CalcBoxScissor(_pEngine, pVP, &_VMat, BoundBoxW, Scissor);
			Scissor.GetRect(MinX, MinY, MaxX, MaxY);
			CPnt ScissorDst(MinX, MinY);
			CRct ScissorRect(ScissorDst, CPnt(MaxX, MaxY));
			CPnt ScissorDstExp(MaxMT(int32(MinX) - 16, VPRect.p0.x), MaxMT(int32(MinY) - 16, VPRect.p0.y));
			CRct ScissorRectExp(ScissorDstExp, CPnt(MinMT(int32(MaxX) + 16, VPRect.p1.x), MinMT(int32(MaxY) + 16, VPRect.p1.y)));
			CPnt ScissorDstExpHalf(ScissorDstExp.x >> 1, ScissorDstExp.y >> 1);
			CRct ScissorRectExpHalf(ScissorDstExpHalf, CPnt(ScissorRectExp.p1.x >> 1, ScissorRectExp.p1.y >> 1));

			CRect2Duint16 ScissorRectExp16(CVec2Duint16(uint16(ScissorDstExp.x), uint16(ScissorDstExp.y)), CVec2Duint16(uint16(ScissorRectExp.p1.x), uint16(ScissorRectExp.p1.y)));

			CVec2Dfp32 UVMinScissor = CVec2Dfp32(_pEngine->m_Screen_PixelUV.k[0] * fp32(ScissorRectExp.p0.x), _pEngine->m_Screen_PixelUV.k[1] * fp32(ScissorRectExp.p0.y));
			CVec2Dfp32 UVMaxScissor = CVec2Dfp32(_pEngine->m_Screen_PixelUV.k[0] * fp32(ScissorRectExp.p1.x), _pEngine->m_Screen_PixelUV.k[1] * fp32(ScissorRectExp.p1.y));
			
			// Create texture coordinates
			CVec2Dfp32* pTV = AllocUtil.Get_V2(4);
			CXR_Util::VBM_CreateRectUV(UVMinScissor, UVMaxScissor, bRenderTextureVertFlip, pTV);

			// Clear wanted area where add model is going to be rendered
			CXR_VertexBuffer* pVBClear0 = AllocUtil.Get_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES, 4);
			pVBClear0->m_pAttrib->SetDefault();
			pVBClear0->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
			CXR_Util::VBM_RenderRect(_pVBM, pMat2D, pVBClear0, ScissorRectExp16, 0x0, PowerPriority - 0.01f);
			
			// Capture rendered tatto model
			AllocUtil.AddCopyToTexture(PowerPriority + 0.01f, ScissorRectExp, ScissorDstExp, TextureID_Temp, true);
			
			// Shrink mask texture
			/*
			CXR_VertexBuffer* pVBShrink = AllocUtil.Get_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES, 4);
			pVBShrink->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
			pVBShrink->m_pAttrib->Attrib_Enable(CRC_FLAGS_SCISSOR);
			pVBShrink->m_pAttrib->Attrib_TextureID(0, TextureID_Full);
			pVBShrink->Geometry_TVertexArray(pTVWhole, 0);
			pVBShrink->m_pAttrib->m_Scissor.SetRect(ScissorRectExpHalf.p0.x, ScissorRectExpHalf.p0.y, ScissorRectExpHalf.p1.x, ScissorRectExpHalf.p1.y);
			CXR_Util::VBM_RenderRect(_pVBM, pMat2D, pVBShrink, VPRectHalf16, 0xffffffff, PowerPriority + 0.02f);
			
			// Copy shrinked mask texture
			// NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW
			// OLD //	AllocUtil.AddCopyToTexture(PowerPriority + 0.03f, ScissorRectExpHalf, ScissorDstExpHalf, TextureID_Temp, false);
			// OLD //	//_pVBM->AddCopyToTexture(PowerPriority + 0.003f, ScissorRectExpHalf, ScissorDstExpHalf, TextureID_Temp, false);
			AllocUtil.AddCopyToTexture(PowerPriority + 0.03f, ScissorRectExpHalf, ScissorDstExpHalf, TextureID_Temp, true);
			// NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW NEW
			*/

			// Run streak shader on tattos
			{
				CRC_ExtAttributes_FragmentProgram20* pFP = AllocUtil.Get_FragmentProgram20(2, "WClientMod_Power", 0);
				CVec4Dfp32* pFPParams = pFP->m_pParams;

				CMat4Dfp32 ProjMat, InvProjMat;
				_VMat.Multiply(pVP->GetProjectionMatrix(), ProjMat);
				ProjMat.Inverse(InvProjMat);

				CVec3Dfp32 WPos = _WMat.GetRow(3);
				CVec4Dfp32 PosTmp;
				PosTmp.k[0] = WPos.k[0];
				PosTmp.k[1] = WPos.k[1];
				PosTmp.k[2] = WPos.k[2];
				PosTmp.k[3] = 1.0f;
				CVec4Dfp32 PosH = PosTmp * ProjMat;
				fp32 w = PosH.k[3];
				PosH.k[0] = PosH.k[0] / w;		
				PosH.k[1] = PosH.k[1] / w;		

				static fp32 FP02 = -0.085f;
				static fp32 FP03 = 0.0f;
				
				static fp32 FP12 = 1.0f;
				static fp32 FP13 = 2.0f;

				PosTmp.k[0] = (PosH.k[0] / swfp32) + 0.5f;
				PosTmp.k[1] = (PosH.k[1] / shfp32) + 0.5f;
				PosTmp.k[0] = Max(-1.0f, PosTmp.k[0]);
				PosTmp.k[1] = Max(0.0f, PosTmp.k[1]);
				PosTmp.k[0] = Min(2.0f, PosTmp.k[0]);
				PosTmp.k[1] = Min(1.0f, PosTmp.k[1]);

				pFPParams[0].k[0] = PosTmp.k[0];
				pFPParams[0].k[1] = bRenderTextureVertFlip ? PosTmp.k[1] : 1.0f - PosTmp.k[1];
				pFPParams[0][2] = FP02;
				pFPParams[0][3] = FP03;

				pFPParams[1][0] = UVTexelHalfRcp.k[0];
				pFPParams[1][1] = UVTexelHalfRcp.k[1];
				pFPParams[1][2] = FP12;
				pFPParams[1][3] = FP13;

				CXR_VertexBuffer* pVBStreaks = AllocUtil.Get_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES, 4);

				pVBStreaks->m_pAttrib->SetDefault();
				pVBStreaks->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
				pVBStreaks->m_pAttrib->Attrib_TextureID(0, TextureID_Temp);
				pVBStreaks->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ADD);
				pVBStreaks->m_pAttrib->m_pExtAttrib = pFP;

				pVBStreaks->Geometry_TVertexArray(pTV, 0);
				CXR_Util::VBM_RenderRect(_pVBM, pMat2D, pVBStreaks, ScissorRectExp16, 0xffffffff, ScreenPrio + 0.1f);

				// Render scissor debug
				/*
				{
					CPixel32 RenderColor = (RenderType == POWERSTREAKS_RENDER_MODEL_0) ? CPixel32(0, 96, 0, 96) : CPixel32(0, 0, 96, 96);
					CRC_Attributes* pAScissor = _pVBM->Alloc_Attrib();
					pAScissor->SetDefault();
					pAScissor->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
					pAScissor->Attrib_RasterMode(CRC_RASTERMODE_ADD);
					CXR_VertexBuffer* pVBScissor = CXR_Util::VBM_RenderRect(_pVBM, pMat2D, ScissorRect16, RenderColor, AfterScreenPrio + 0.002f, pAScissor);
					_pVBM->AddVB(pVBScissor);
				}
				*/
			}
		}

		// Copy screen texture first time
		if (RenderType == POWERSTREAKS_RENDER_SCREEN)
		{
			int TextureID_Screen = _pEngine->m_TextureID_Screen;
			int TextureID_ResolveScreen = _pEngine->m_TextureID_ResolveScreen;

#ifdef PLATFORM_XENON
			// End tiling, resolve and prepare for streak shader
			AllocUtil.AddCopyToTexture(ScreenPrio, VPRect, CPnt(0,0), TextureID_Screen, false);
#endif

			// Calculate min/max uv coordinates
			CVec2Dfp32 UVMin = 0.0f;
			CVec2Dfp32 UVMax = CVec2Dfp32(swfp32 * _pEngine->m_Screen_PixelUV.k[0], shfp32 * _pEngine->m_Screen_PixelUV.k[1]);
			
			// Setup texture coordinates
			CVec2Dfp32* pTVWhole = AllocUtil.Get_V2(4);
			CXR_Util::VBM_CreateRectUV(UVMin, UVMax, bRenderTextureVertFlip, pTVWhole);

			// Restore resolve screen
			CXR_VertexBuffer* pVBRestore = AllocUtil.Get_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES, 4);
			pVBRestore->m_pAttrib->SetDefault();
			pVBRestore->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
			pVBRestore->m_pAttrib->Attrib_TextureID(0, TextureID_ResolveScreen);
			pVBRestore->Geometry_TVertexArray(pTVWhole, 0);
			CXR_Util::VBM_RenderRect(_pVBM, pMat2D, pVBRestore, VPRect16, 0xffffffff, PowerPriority);

#ifdef PLATFORM_XENON
			// Restore end tile capture
			CXR_VertexBuffer* pVBBlend = AllocUtil.Get_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES, 4);
			pVBBlend->m_pAttrib->SetDefault();
			pVBBlend->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
			pVBBlend->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_NONE);
			pVBBlend->m_pAttrib->Attrib_TextureID(0, TextureID_Screen);
			pVBBlend->Geometry_TVertexArray(pTVWhole, 0);
			CXR_Util::VBM_RenderRect(_pVBM, pMat2D, pVBBlend, VPRect16, 0xffffffff, ScreenPrio + 0.05f);
#endif
		}
	}
}


void SPowerStreak::Render(CXR_Engine* _pEngine, CXR_Model_PowerStreaks* _pPowerStreaksModel, fp32 _GameTickTime, fp32 _IPTime, CWireContainer* _pWC)
{
	// Check if we need rendering
	bool bPowerStreaks = (m_Power == m_MaxPower);
	uint nModels = 0;
	bool bVisModel[NUM_POWERSTREAKS] = { 0 };
	CXR_RenderInfo RenderInfo(_pEngine);
	for (uint iSlot = 0; iSlot < NUM_POWERSTREAKS; iSlot++)
	{
		if (IsValid(iSlot))
		{
			if (bVisModel[iSlot] = _pEngine->View_GetClip_Box(m_lBoundBoxW[iSlot].m_Min, m_lBoundBoxW[iSlot].m_Max, 0, 0, NULL, &RenderInfo))
				nModels++;
		}
	}
	if (!nModels)
		return;

	 // Figure out variation
	int8 iPowerFade0 = (int8)Clamp(TruncToInt(m_PowerFade), 0, 4);
	int8 iPowerFade1 = (int8)Clamp(TruncToInt(m_PowerFade + 0.999999f), 0, 4);
	int8 iPower0 = (iPowerFade0 == 4) ? 0 : iPowerFade0;
	int8 iPower1 = (iPowerFade1 == 4) ? 0 : iPowerFade1;
	int8 iPower = m_iPower;
			
	fp32 TimeDst = Clamp01(1.0f - (fp32(m_Power) / fp32(m_MaxPower)));
	fp32 Time = Clamp01(CFXSysUtil::LerpMT(m_Darkness, TimeDst, (_GameTickTime * 2.0f) * _IPTime));
	fp32 LevelTime = (fp32)((m_MaxPower - 20) / 20);
	CBox3Dfp32* plBoundBoxW = (CBox3Dfp32*)_pEngine->GetVBM()->Alloc(sizeof(CBox3Dfp32) * nModels);
	uint iModel = 0;

	if (iPower0 == iPower1)
	{
		for (uint iSlot = 0; iSlot < NUM_POWERSTREAKS; iSlot++)
		{
			if (bVisModel[iSlot])
			{
				CXR_AnimState& Anim = m_lAnimState[iSlot];
				Anim.m_Data[0] = ~0;

				// Power tatto
				Anim.m_AnimTime0 = CMTime::CreateFromSeconds(Time);
				Anim.m_Variation = iPower0 + 1;
				_pEngine->Render_AddModel(m_lpModel[iSlot], m_lMat[iSlot], Anim, XR_MODEL_STANDARD, CXR_MODEL_ONRENDERFLAGS_NOSHADOWS);

				// Level tatto
				Anim.m_AnimTime0 = CMTime::CreateFromSeconds(LevelTime);
				Anim.m_Variation += 4;
				_pEngine->Render_AddModel(m_lpModel[iSlot], m_lMat[iSlot], Anim, XR_MODEL_STANDARD, CXR_MODEL_ONRENDERFLAGS_NOSHADOWS);

				aint RenderModel = POWERSTREAKS_RENDER_MODEL + iSlot + 1;
				if (bPowerStreaks && RenderModel < POWERSTREAKS_RENDER_MODEL_MAX)
				{
					plBoundBoxW[iModel] = m_lBoundBoxW[iSlot];

					Anim.m_Data[0] = RenderModel;
					Anim.m_Data[1] = (aint)m_lpModel[iSlot];
					Anim.m_Data[2] = (aint)&plBoundBoxW[iModel++];
					Anim.m_Variation = 0;
					_pEngine->Render_AddModel(_pPowerStreaksModel, m_lMat[iSlot], Anim, XR_MODEL_STANDARD, CXR_MODEL_ONRENDERFLAGS_NOCULL);
					Anim.m_Data[1] = ~0;
					Anim.m_Data[2] = ~0;

					#ifndef M_RTM
						if (_pWC)
							_pWC->RenderAABB(plBoundBoxW[iSlot], CPixel32(0,255,0,255), 0.0f, false);
					#endif
				}
			}
		}
	}
	else
	{
		// Make sure iPower1 is always wanted power
		if (iPower0 == iPower)
		{
			Swap(iPower0, iPower1);
			Swap(iPowerFade0, iPowerFade1);
		}

		// Render tattoos we're fading from and to
		int FadeOut = Clamp(TruncToInt(M_Fabs(fp32(iPowerFade1) - m_PowerFade) * 255.0f), 0, 255);
		int FadeIn = 255 - FadeOut;

		for (int iSlot = 0; iSlot < NUM_POWERSTREAKS; iSlot++)
		{
			if (bVisModel[iSlot])
			{
				CXR_AnimState& Anim = m_lAnimState[iSlot];
				Anim.m_AnimTime0 = CMTime::CreateFromSeconds(Time);

				Anim.m_Variation = iPower0 + 1;
				Anim.m_Data[0] = CPixel32(FadeOut, FadeOut, FadeOut, 255);
				_pEngine->Render_AddModel(m_lpModel[iSlot], m_lMat[iSlot], Anim, XR_MODEL_STANDARD, CXR_MODEL_ONRENDERFLAGS_NOSHADOWS);

				Anim.m_Variation += 4;
				Anim.m_AnimTime0 = CMTime::CreateFromSeconds(LevelTime);
				_pEngine->Render_AddModel(m_lpModel[iSlot], m_lMat[iSlot], Anim, XR_MODEL_STANDARD, CXR_MODEL_ONRENDERFLAGS_NOSHADOWS);

				Anim.m_Variation = iPower1 + 1;
				Anim.m_AnimTime0 = CMTime::CreateFromSeconds(Time);
				Anim.m_Data[0] = CPixel32(FadeIn, FadeIn, FadeIn, 255);
				_pEngine->Render_AddModel(m_lpModel[iSlot], m_lMat[iSlot], Anim, XR_MODEL_STANDARD, CXR_MODEL_ONRENDERFLAGS_NOSHADOWS);

				Anim.m_Variation += 4;
				Anim.m_AnimTime0 = CMTime::CreateFromSeconds(LevelTime);
				_pEngine->Render_AddModel(m_lpModel[iSlot], m_lMat[iSlot], Anim, XR_MODEL_STANDARD, CXR_MODEL_ONRENDERFLAGS_NOSHADOWS);

				aint RenderModel = POWERSTREAKS_RENDER_MODEL + iSlot + 1;
				if (bPowerStreaks && RenderModel < POWERSTREAKS_RENDER_MODEL_MAX)
				{
					plBoundBoxW[iModel] = m_lBoundBoxW[iSlot];

					Anim.m_Data[0] = RenderModel;
					Anim.m_Data[1] = (aint)m_lpModel[iSlot];
					Anim.m_Data[2] = (aint)&plBoundBoxW[iModel++];
					Anim.m_Variation = 0;
					_pEngine->Render_AddModel(_pPowerStreaksModel, m_lMat[iSlot], Anim, XR_MODEL_STANDARD, CXR_MODEL_ONRENDERFLAGS_NOCULL);
					Anim.m_Data[1] = ~0;
					Anim.m_Data[2] = ~0;

					#ifndef M_RTM
						if (_pWC)
							_pWC->RenderAABB(plBoundBoxW[iSlot], CPixel32(0,255,0,255), 0.0f, false);
					#endif
				}
			}
		}
	}

	if (bPowerStreaks)
	{
		CMat4Dfp32 Unit; Unit.Unit();
		CXR_AnimState Anim; Anim.Clear();
		Anim.m_Data[0] = POWERSTREAKS_RENDER_SCREEN;
		_pEngine->Render_AddModel(_pPowerStreaksModel, Unit, Anim, XR_MODEL_STANDARD, CXR_MODEL_ONRENDERFLAGS_NOCULL);

		_pEngine->GetVC()->m_bNeedResolve_TempFixMe = true;
	}
}


void CWO_TentacleSystem_ClientData::OnRender(CWorld_Client* _pWClient, CXR_Engine* _pEngine)
{
	uint nArms = m_lArmSetup.Len();
	if (nArms == 0)
		return;

	fp32 IPTime = _pWClient->GetRenderTickFrac();
	fp32 TickTime = _pWClient->GetGameTickTime();
	fp32 TicksSec = _pWClient->GetGameTicksPerSecond();
	int GameTick = _pWClient->GetGameTick();
	CMTime RenderTime = _pWClient->GetRenderTime();
	CMTime GameTime = _pWClient->GetGameTime();

	CWObject_Client* pOwner = _pWClient->Object_Get(m_iOwner);
	bool bOwnerIsCharacter = false;
	bool bOwnerIsInCreepingDark = false;
	bool bOwnerIsDraining = false;
	fp32 DrainTime = 0.0f;
	if (pOwner)
	{
		// Check if snakes should be hidden (3rd person mode)
		if (pOwner->IsClass(class_CharPlayer) || pOwner->IsClass(class_CharNPC))
		{
			bOwnerIsCharacter = true;
			pOwner = CWObject_Player::Player_GetLastValidPrediction(pOwner);

			CWO_Character_ClientData* pOwnerCD = CWObject_Character::GetClientData(pOwner);
			bool bCutSceneView = (pOwner->m_ClientFlags & (PLAYER_CLIENTFLAGS_CUTSCENE | PLAYER_CLIENTFLAGS_DIALOGUE)) != 0;
			bool bThirdPerson = (pOwnerCD->m_ExtraFlags & (PLAYER_EXTRAFLAGS_THIRDPERSON | PLAYER_EXTRAFLAGS_FORCETHIRPERSON)) != 0;
			bool bNoClip = (CWObject_Character::Char_GetPhysType(pOwner) == PLAYER_PHYS_NOCLIP);
			bOwnerIsInCreepingDark = (pOwnerCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK) != 0;

			if (bThirdPerson || bCutSceneView || bNoClip)
				return;

			if ((pOwnerCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_MASK) != THIRDPERSONINTERACTIVE_MODE_NONE)
			{
				uint Slide = pOwnerCD->m_3PI_CameraSlide.Get(pOwnerCD->m_GameTick, pOwnerCD->m_PredictFrameFrac);
				if (Slide > 64)
					return;
			}

			DrainTime = pOwnerCD->m_DarknessDrainTime;
			bOwnerIsDraining = (pOwnerCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DRAIN) != 0;
			DrainTime = (bOwnerIsDraining) ? DrainTime : 3.0f - DrainTime;
			bOwnerIsDraining = !AlmostEqual(DrainTime, 0.0f, 0.0001f);
		}
	}

	uint RenderFlags = 0;
	if (!_pWClient->m_Render_CharacterShadows)
		RenderFlags |= CXR_MODEL_ONRENDERFLAGS_NOSHADOWS;

	CWireContainer* pWC = _pWClient->Debug_GetWireContainer();

	uint32 UserColor0 = 0;
	uint8 Darkness = 0;
	uint8 MaxDarkness = 0;
	int8 iPower = 0;
	fp32 Fade = 0.0f;

	if (bOwnerIsCharacter)
	{
		CWObject_Message OwnerMsg(OBJMSG_CHAR_GETDARKNESS);
		Darkness = (uint8)_pWClient->ClientMessage_SendToObject(OwnerMsg, m_iOwner);

		OwnerMsg.m_Msg = OBJMSG_CHAR_GETMAXDARKNESS;
		MaxDarkness = (uint8)_pWClient->ClientMessage_SendToObject(OwnerMsg, m_iOwner);

		int8 iLastPower = 0;
		Fade = GetDemonHeadFade(_pWClient, pOwner, &iPower, &iLastPower);
		if (Fade != -1.0f)
		{
			uint32 liColor[] = { 0xffb7b56a, 0xffff0000, 0xff9fcae1, 0xffb7b56a, 0 };
			UserColor0 = CFXSysUtil::LerpColor(liColor[TruncToInt(Fade + 0.001f)], liColor[TruncToInt(Fade + 0.999f)], Fade - fp32(TruncToInt(Fade + 0.001f)));
		}
	}
	SPowerStreak PowerStreaks;
	PowerStreaks.Setup(iPower, Fade, m_Darkness, Darkness, MaxDarkness);

	// Get and create anim graph information
	CXR_AnimLayer AnimLayers[4];
	CWAG2I* pAGI = m_AnimGraph.m_spAGI;
	CWAG2I_Context AGIContext(m_pObj, m_pWPhysState, RenderTime);
	
	// Render each tentacle arm
	for (uint iArm = 0; iArm < nArms; iArm++)
	{
		// Skip if retracted
		const CTentacleArmState& ArmState = m_lArmState[iArm];
		const CTentacleArmSetup& ArmSetup = m_lArmSetup[iArm];
		if (ArmState.m_State == TENTACLE_STATE_IDLE)
			continue;

		// In creeping dark, hide the other tentacle stuff
		bool bCreepingDark = (ArmState.m_Task == TENTACLE_TASK_TRAIL) && (ArmState.m_State == TENTACLE_STATE_TRAIL);
		if (bOwnerIsInCreepingDark && !bCreepingDark && (iArm != TENTACLE_CREEPINGDARK_SQUID))
			continue;

		const bool bDemonHead = (iArm == TENTACLE_DEMONHEAD1 || iArm == TENTACLE_DEMONHEAD2);
		const bool bDemonArm = (iArm == TENTACLE_DEMONARM);

		// Lars wanted to remove shadows from the tentacle system. But for now, let the good ol' Demon Arm have shadows...
		if (!bDemonArm)
			RenderFlags |= CXR_MODEL_ONRENDERFLAGS_NOSHADOWS;

		// Get some template-mesh information
		CTemplateMesh Model0, Model1;
		bool b0 = Model0.Init(ArmSetup.m_liTemplateModels[0], _pWClient->GetMapData());
		bool b1 = Model1.Init(ArmSetup.m_liTemplateModels[1], _pWClient->GetMapData());
		if (!b0 && !b1)
			continue;

		// Build spline
		CSpline_Tentacle Spline;
		GetSpline(iArm, Spline, true);
		
		// Calculate arm length
		fp32 ArmLength = ArmState.m_Length + IPTime * ArmState.m_Speed;
		ArmLength = Clamp(ArmLength, 0.0f, Spline.m_Length);
		if (M_Fabs(ArmState.m_Speed - TENTACLE_SPEED_SNAPTOTARGET) < 0.1f)
			ArmLength = Spline.m_Length;

		const fp32 Scale = ArmSetup.m_Scale;
		const fp32 InvModifier = (ArmSetup.m_bMirrorAnim) ? -1.0f : 1.0f;
		fp32 PosOnSpline = ArmLength;
		fp32 TotalSpin = 0.0f;
		
		// Find end position on spline
		CSpline_Tentacle::SplinePos SpEndPos;
		Spline.FindPos(PosOnSpline, SpEndPos, Spline.m_EndMat.GetRow(2));
		CMat4Dfp32	EndPos = SpEndPos.mat,
					StartPos_Render = EndPos,
					StartPos, AttachModelPos[2], ScaleMat, Inv, Mat;

		// Setup scale matrix
		ScaleMat.Unit();
		ScaleMat.Multiply(Scale);
		
		TAP<const uint16> liBones = ArmSetup.m_liSplineBones;

		// Debug render spline we created earlier
		if (DEBUG_RENDER_SPLINE_ORIGINAL && pWC)
		{
			DebugRenderSpline(Spline, *pWC, false);
			pWC->RenderMatrix(SpEndPos.mat, 0.0f, false);
		}

		// loop through spline and place tile segments, from end to start
		for (uint i = 0; PosOnSpline >= 0.0f; i++)
		{
			bool bEndSegment = (i == 0);
			const CTemplateMesh& Model = bEndSegment ? Model1 : Model0;

			// Skip if we have no mesh or if we're not rendering end segment in creeping dark mode
			if ((Model.pMesh == NULL) || (bCreepingDark && !bEndSegment))
				break;

			// Setup animstate and create skeleton instance data
			CXR_AnimState Anim;
			Anim.m_iObject = m_pObj->m_iObject;
			Anim.m_pSkeletonInst = CXR_VBMHelper::Alloc_SkeletonInst(_pEngine->GetVBM(), Model.nBones, Model.pSkeleton->m_nUsedRotations, Model.pSkeleton->m_nUsedMovements);
			if (!Anim.m_pSkeletonInst)
				return;

			// Fetch some pointers
			CXR_Skeleton* pSkeleton = Model.pSkeleton;
			CXR_SkeletonInstance* pSkeletonInst = Anim.m_pSkeletonInst;
			TAP_RCD<CXR_SkeletonNode> lNodes = Model.pSkeleton->m_lNodes;
			TAP_RCD<CMat4Dfp32> lBoneTransform(Anim.m_pSkeletonInst->m_pBoneTransform, Anim.m_pSkeletonInst->m_nBoneTransform);
			TAP_RCD<CMat4Dfp32> lBoneLocalPos(Anim.m_pSkeletonInst->m_pBoneLocalPos, Anim.m_pSkeletonInst->m_nBoneLocalPos);
			
			// Animate end segments
			bool bAnimated = false;
			if (bEndSegment)
			{
				int nAnimLayers = sizeof_buffer(AnimLayers);
				pAGI->GetAnimLayersFromToken(&AGIContext, ArmSetup.m_iAGToken, AnimLayers, nAnimLayers, 0);
				if (nAnimLayers > 0)
				{
					if (Spline.IsEmpty())
					{
						// this is just an animated model (used for on-screen tentacles)
						pSkeleton->EvalAnim(AnimLayers, nAnimLayers, pSkeletonInst, Spline.m_EndMat);
					}
					else
					{
						CMat4Dfp32 UnitMat;
						UnitMat.Unit();
						pSkeleton->EvalAnim(AnimLayers, nAnimLayers, pSkeletonInst, UnitMat);
					}
					bAnimated = true;
				}
			}

			if (Spline.IsEmpty())
			{
				// this is just an animated model (used for on-screen tentacles)
				_pEngine->Render_AddModel(Model.pMesh, Spline.m_EndMat, Anim, XR_MODEL_STANDARD, RenderFlags);
				break;
			}

			const fp32 Length = Model.Length * Scale;
			const fp32 SpinModifier = (bEndSegment) ? 0.0f : 1.0f;
			const CVec3Dfp32& Origo = lNodes[Model.iStartBone].m_LocalCenter;

			// After animation has been evaluated we know where the heart bone is, so offset everything
			//const CVec3Dfp32 Offset = GetOffset();
			CVec3Dfp32 Offset = 0.0f;
			if (bEndSegment && ArmState.IsTask_DevourTarget() && ArmState.m_iTarget)
			{
				CWObject_CoreData* pTarget = m_pWPhysState->Object_GetCD(ArmState.m_iTarget);
				if (pTarget)
				{
					CMat4Dfp32 RealHeartMat;
					GetCharBoneMat(*pTarget, TENTACLE_DEVOUR_ROTTRACK_HEART, RealHeartMat, IPTime);

					// Get bone in worldspace
					CMat4Dfp32 BoneTransform = Anim.m_pSkeletonInst->m_pBoneTransform[46];
					{
						CSpline_Tentacle::SplinePos sp;
						CVec3Dfp32 CurrUp = EndPos.GetRow(2);
						const CVec3Dfp32& LocalCenter = lNodes[46].m_LocalCenter;
						
						fp32 CurrX = (LocalCenter.k[0] - Origo.k[0]) * Scale;
						fp32 d = PosOnSpline - Length + CurrX;
						d = Max(0.0f, d);
						Spline.FindPos(d, sp, CurrUp);
						CurrUp = sp.mat.GetRow(2);
						sp.mat.GetRow(3) += sp.mat.GetRow(1) * ((LocalCenter.k[1] - Origo.k[1]) * Scale);
						sp.mat.GetRow(3) += sp.mat.GetRow(2) * ((LocalCenter.k[2] - Origo.k[2]) * Scale);

						Inv.SetXRotation(0.0f);
						Inv.GetRow(3) = -LocalCenter;

						CMat4Dfp32 MatScale;
						MatScale.Unit();
						MatScale.Multiply(Scale);
						MatScale.Multiply(BoneTransform, BoneTransform);

						BoneTransform.Multiply(Inv, BoneTransform);
						BoneTransform.Multiply(sp.mat, BoneTransform);
						BoneTransform.GetRow(3) = LocalCenter * BoneTransform;
					}

					// Specify how much offset we want
					fp32 DevourOffsetTime = GetDevourBlendTime(ArmSetup.m_iAGToken, IPTime, TENTACLE_DEVOURBLEND_TIME_HEADHEARTOFFSET);
					Offset = CFXSysUtil::LerpMT(CVec3Dfp32(0.0f), RealHeartMat.GetRow(3) - BoneTransform.GetRow(3), DevourOffsetTime);
				}
			}

			// Setup modifiers
					
			// Evaluate each bone or from list
			
			CSpline_Tentacle::SplinePos sp;
			CVec3Dfp32 LastPos = StartPos_Render.GetRow(3);
			CVec3Dfp32 CurrUp = EndPos.GetRow(2);
			
			// Get rid of this ?!
			//CMat4Dfp32 TempAttach[4];
			//CMat4Dfp32* pAttachModelMat = NULL;
			CBox3Dfp32 BoundBoxW(_FP32_MAX, -_FP32_MAX);

			bool bBoneList = (bEndSegment && liBones.Len() > 0);
			int nBones = (bBoneList) ? liBones.Len() : Model.nBones;
			for (int j = nBones - 1; j >= 0; j--)
			{
				// Get bone index, it's local center and bone transform
				int iBone = (bBoneList) ? liBones[j] : j;
				const CVec3Dfp32& LocalCenter = lNodes[iBone].m_LocalCenter;
				CMat4Dfp32& BoneTransform = lBoneTransform[iBone];
				
				// Find spline position, apply offset and store current up vector
				fp32 CurrX = (LocalCenter.k[0] - Origo.k[0]) * Scale;
				fp32 d = PosOnSpline - Length + CurrX;
				d = Max(0.0f, d);
				Spline.FindPos(d, sp, CurrUp);
				sp.mat.GetRow(3) += Offset;
				sp.mat.GetRow(3) += sp.mat.GetRow(1) * ((LocalCenter.k[1] - Origo.k[1]) * Scale);
				sp.mat.GetRow(3) += sp.mat.GetRow(2) * ((LocalCenter.k[2] - Origo.k[2]) * Scale);
				CurrUp = sp.mat.GetRow(2);


				// Calculate spining
				fp32 Spin = (TotalSpin + (Length - CurrX)/Scale * ArmSetup.m_Spin) * SpinModifier;
				Inv.SetXRotation(Spin);
				Inv.GetRow(3) = -LocalCenter;

				// Create mirroring
				Inv.k[0][1] *= InvModifier;
				Inv.k[1][1] *= InvModifier;
				Inv.k[2][1] *= InvModifier;

				// Scale
				ScaleMat.Multiply(BoneTransform, BoneTransform);

				if (bAnimated)
					BoneTransform.Multiply(Inv, Mat);
				else
					Mat = Inv;

				// This is quite bad in a bone loop
				if (bDemonArm)
				{
					// add some wormy wave stuff (should be placed in the template. remove spin?)
					fp32 EndDistance = ArmLength - Model1.Length;
					if (d < EndDistance) // Zero wave at end point
					{
						fp32 StartDistance = Spline.GetDistance(1.0f);
						fp32 Amount = 1.0f;
						if (d < StartDistance) // Less waves near the start point (player's back)
						{
							Amount = d * (1.0f / StartDistance);
							if (d < 16.0f && (iBone == Model.iStartBone))
								Amount = 0.0f; // snap start point
						}
						fp32 Wave = d - StartDistance;
						fp32 t = RenderTime.GetTime(); //EEEK! (should do modulus before converting to float precision)
						fp32 w1 = Amount * 20.0f * M_Sin(Wave * 0.06f - t*0.71f + 1.0f);
						fp32 w2 = Amount * 10.0f * M_Sin(Wave * 0.07f + t*0.63f);
						sp.mat.GetRow(3) += sp.mat.GetRow(1) * w1;
						sp.mat.GetRow(3) += sp.mat.GetRow(2) * w2;
					}
				}

				// Create bone matrix
				bool bEndOfTileSegment = (!bEndSegment) && (iBone == Model.iEndBone);
				if (bEndOfTileSegment)
					Mat.Multiply(EndPos, BoneTransform); // force snap
				else
					Mat.Multiply(sp.mat, BoneTransform);

				// Calculate actual world position
				// Place this somewhere else
				Mat = BoneTransform;
				CVec3Dfp32 WorldPos = LocalCenter * Mat;
				if (bEndSegment)
				{
					/*
					for (uint iAttachModel = 0; iAttachModel < 2; iAttachModel++)
					{
						if (ArmSetup.m_lAttachModels[iAttachModel].GetModel(0))
						{
							if (iBone == ArmSetup.m_lAttachModels[iAttachModel].GetAttachNode())
							{
								TempAttach[iAttachModel] = Mat;
								TempAttach[iAttachModel].GetRow(3) = (LocalCenter + ArmSetup.m_lAttachModels[iAttachModel].m_Offset) * Mat;
								//TempAttach[iAttachModel].GetRow(3) = (LocalCenter + CVec3Dfp32(0.297f, 0.1f,0.773f)) * Mat;
							}
						}
					}
					*/

					BoundBoxW.Expand(WorldPos);
				}
				Mat.GetRow(3) = WorldPos;


				if (iBone == Model.iStartBone)
				{
					// Store current matrix to get correct render matrix
					StartPos = sp.mat;
					StartPos_Render = Mat;
				}

#ifndef M_RTM
				// Debug render bone hierarchy
				if (DEBUG_RENDER_BONE_HIERARCHY && !bBoneList)
				{
					const CVec3Dfp32& BPos = Mat.GetRow(3);
					if (pWC)
						pWC->RenderVector(BPos, LastPos - BPos, 0xffffff00, 0.0f, false);
					LastPos = BPos;
				}
#endif
				// if (pWC)
				// 	pWC->RenderMatrix(Mat, 0.0f, false);
			}

			if (bBoneList)
			{
				// Fix local pos in bone list
				CMat4Dfp32 Inv;
				CVec3Dfp32 LocalPos;
				CMat4Dfp32* pBoneLocalPos = Anim.m_pSkeletonInst->m_pBoneLocalPos;
				for (int i = 0; i < nBones; i++)
				{
					uint16 iBone = liBones[i];
					LocalPos = pBoneLocalPos[iBone].GetRow(3);
					lBoneTransform[lNodes[iBone].m_iNodeParent].InverseOrthogonal(Inv);
					lBoneTransform[iBone].Multiply3x3(Inv, lBoneLocalPos[iBone]);
					lBoneLocalPos[iBone].GetRow(3) = LocalPos;
				}

				// Re-evaluate nodes
				CMat4Dfp32 EvalMat = lBoneTransform[lNodes[0].m_iNodeParent];
				pSkeleton->EvalNode(0, &EvalMat, pSkeletonInst);

				// Update bounding box
				CBox3Dfp32 UpdateBoundBoxW(_FP32_MAX, -_FP32_MAX);
#ifndef M_RTM
				if (DEBUG_RENDER_BONE_HIERARCHY && pWC)
				{
					LastPos = StartPos_Render.GetRow(3);
					for (int iBone = Model.nBones - 1; iBone >= 0; iBone--)
					{
						CVec3Dfp32 BPos = (lNodes[iBone].m_LocalCenter * lBoneTransform[iBone]);
						UpdateBoundBoxW.Expand(BPos);
						
						pWC->RenderVector(BPos, LastPos - BPos, 0xffffff00, 0.0f, false);
						LastPos = BPos;
					}
				}
				else
#endif
				{
					for (int iBone = Model.nBones - 1; iBone >= 0; iBone--)
						UpdateBoundBoxW.Expand((lNodes[iBone].m_LocalCenter * lBoneTransform[iBone]));
				}
				BoundBoxW = UpdateBoundBoxW;

				// Store current matrix to get correct render matrix
				StartPos_Render = lBoneTransform[Model.iStartBone];
				StartPos_Render.GetRow(3) = lNodes[Model.iStartBone].m_LocalCenter * StartPos_Render;
				StartPos = StartPos_Render;
			}

#ifndef M_RTM
			if (pWC)
				pWC->RenderMatrix(StartPos_Render, 0.0f, false);

			if (!(pWC && HIDE_MODELS_WHEN_RENDERING_WIREFRAME))
#endif
			{
				// Demon Head model
				fp32 BloodTime = 1.0f - Clamp01((CMTime::CreateFromTicks((GameTick - ArmState.m_BloodTick), TickTime, IPTime)).GetTime() * (1.0f / 60.0f));
				if (ArmState.m_BloodTick == 0)
					BloodTime = 0.0f;

				Anim.m_AnimTime0 = CMTime::CreateFromSeconds(BloodTime);
				Anim.m_Data[0] = UserColor0;

				_pEngine->Render_AddModel(Model.pMesh, StartPos_Render, Anim, XR_MODEL_STANDARD, RenderFlags);

				if (bEndSegment)
				{
					// Add animated heart model
					if (ArmState.IsTask_DevourTarget() && ArmState.m_iTarget)
					{
						fp32 HeartVis = GetDevourBlendTime(ArmSetup.m_iAGToken, IPTime, TENTACLE_DEVOURBLEND_TIME_HEART);
						CWObject_CoreData* pTarget = _pWClient->Object_GetCD(ArmState.m_iTarget);
						if (pTarget && AlmostEqual(HeartVis, 1.0f, 0.0001f))
						{
							CMat4Dfp32 RenderHeartMat;
							RenderHeartMat = lBoneTransform[46];
							RenderHeartMat.GetRow(3) = lNodes[46].m_LocalCenter * RenderHeartMat;

							CXR_AnimState HeartAnimState;
							int iHeartModel = _pWClient->GetMapData()->GetResourceIndex_Model("Items/heart_01");
							CXR_Model* pHeartModel = (iHeartModel) ? _pWClient->GetMapData()->GetResource_Model(iHeartModel) : NULL;
							if (pHeartModel)
							{
								HeartAnimState.Clear();

								// try to evaluate heart animation
								CXR_Skeleton* pHeartSkeleton = pHeartModel->GetSkeleton();
								CMat4Dfp32 ReEvalMat = RenderHeartMat;
								if (pHeartSkeleton)
								{
									HeartAnimState.m_iObject = m_pObj->m_iObject;
									HeartAnimState.m_pSkeletonInst = CXR_VBMHelper::Alloc_SkeletonInst(_pEngine->GetVBM(), pHeartSkeleton->m_lNodes.Len(), pHeartSkeleton->m_nUsedRotations, pHeartSkeleton->m_nUsedMovements);
									if (HeartAnimState.m_pSkeletonInst)
									{
										int nAnimLayers = sizeof_buffer(AnimLayers);
										pAGI->GetAnimLayersFromToken(&AGIContext, TENTACLE_AG2_TOKEN_HEART, AnimLayers, nAnimLayers, 0);

										if (nAnimLayers > 0)
										{
											CMat4Dfp32 UnitMat; UnitMat.Unit();
											pHeartSkeleton->EvalAnim(AnimLayers, nAnimLayers, HeartAnimState.m_pSkeletonInst, UnitMat);
											pHeartSkeleton->EvalNode(0, &RenderHeartMat, HeartAnimState.m_pSkeletonInst);
										}
									}
								}
								
								if (DEBUG_RENDER_HEART && pWC)
								{
									pWC->RenderSphere(RenderHeartMat, 2.0f, CPixel32(255,255,0,255), 0.0f, false);
									pWC->RenderMatrix(RenderHeartMat, 0.0f, false);
								}

								_pEngine->Render_AddModel(pHeartModel, RenderHeartMat, HeartAnimState);
							}
						}
					}

					// Setup power streaks
					Anim.m_AnimTime0 = CMTime::CreateFromSeconds(Fade);
					if (bDemonHead)
					{
						PowerStreaks.Setup((iArm - TENTACLE_DEMONHEAD1), Model.pMesh, StartPos_Render, Anim, BoundBoxW);

						// Add drain model
						if (bOwnerIsDraining)
						{
							CTentacleArmSetup& ArmSetupModelInstance = m_lArmSetup[iArm];

							const CXR_SkeletonAttachPoint* pAttach = pSkeleton->GetAttachPoint(ArmSetup.m_DrainModel.GetAttachNode());
							pAttach->m_LocalPos.Multiply(lBoneTransform[pAttach->m_iNode], Mat);

							CXR_AnimState DrainAnimState;
							CXR_Model* pDrainModel = _pWClient->GetMapData()->GetResource_Model(ArmSetup.m_DrainModel.GetModel(0));
							CXR_ModelInstance* pModelInstance = ArmSetupModelInstance.m_DrainModel.GetModelInstance(0);
							if (pDrainModel && pModelInstance)
							{
								DrainAnimState.Clear();
								DrainAnimState.m_AnimTime0 = RenderTime;
								DrainAnimState.m_AnimTime1 = CMTime::CreateFromSeconds(DrainTime);
								DrainAnimState.m_pModelInstance = pModelInstance;
								_pEngine->Render_AddModel(pDrainModel, Mat, DrainAnimState);
							}
						}
					}

					// Add attach models
					Anim.Clear();
					Anim.m_AnimTime0 = GameTime;
					Anim.m_AnimTime1.Reset();
					for (uint iAttachModel = 0; iAttachModel < 2; iAttachModel++)
					{
						CXR_Model* pModel = (ArmSetup.m_lAttachModels[iAttachModel].GetModel(0)) ? _pWClient->GetMapData()->GetResource_Model(ArmSetup.m_lAttachModels[iAttachModel].GetModel(0)) : NULL;
						if (pModel)
						{
							const CXR_SkeletonAttachPoint* pAttach = pSkeleton->GetAttachPoint(ArmSetup.m_lAttachModels[iAttachModel].GetAttachNode());
							pAttach->m_LocalPos.Multiply(lBoneTransform[pAttach->m_iNode], Mat);
							_pEngine->Render_AddModel(pModel, Mat, Anim, XR_MODEL_STANDARD);

							//Interpolate2(TempAttach[iAttachModel*2], TempAttach[(iAttachModel*2)+1], AttachModelPos[iAttachModel], 0.5f);
							//_pEngine->Render_AddModel(pModel, TempAttach[iAttachModel], Anim, XR_MODEL_STANDARD);
						}
					}

					// Add breath models
					if (GameTick >= ArmState.m_BreathCtrl)
					{
						CWObject_Client* pClObj = _pWClient->Object_Get(m_pObj->m_iObject);

						Anim.Clear();
						Anim.m_AnimTime0 = CMTime::CreateFromTicks(GameTick - ArmState.m_BreathCtrl, TickTime, IPTime);
						Anim.m_AnimTime1 = CMTime::CreateFromSeconds(4.0f);

						uint BreathFlags = GetBreathFlags(ArmState.m_BreathCtrl);
						CXR_Model* pBreathModels[] = { NULL, NULL, NULL, NULL };
						if (BreathFlags & TENTACLE_BREATH_MODEL0) pBreathModels[0] = _pWClient->GetMapData()->GetResource_Model(ArmSetup.m_BreathModels.GetModel(0));
						if (BreathFlags & TENTACLE_BREATH_MODEL1) pBreathModels[1] = _pWClient->GetMapData()->GetResource_Model(ArmSetup.m_BreathModels.GetModel(1));
						if (BreathFlags & TENTACLE_BREATH_MODEL2) pBreathModels[2] = _pWClient->GetMapData()->GetResource_Model(ArmSetup.m_BreathModels.GetModel(2));
						
						const CXR_SkeletonAttachPoint* pAttach = pSkeleton->GetAttachPoint(ArmSetup.m_BreathModels.GetAttachNode());
						if (pAttach)
						{
							M_VMatMul(pAttach->m_LocalPos, lBoneTransform[pAttach->m_iNode], Mat);

							//Mat = lBoneTransform[ArmSetup.m_BreathModels.GetAttachNode()];
							CTentacleArmSetup& ArmSetupModelInstance = m_lArmSetup[iArm];
							for (uint i = 0; i < 3; i++)
							{
								if (pBreathModels[i])
								{
									Anim.m_pModelInstance = ArmSetupModelInstance.m_BreathModels.GetModelInstance(i);
									_pEngine->Render_AddModel(pBreathModels[i], Mat, Anim, XR_MODEL_STANDARD);
								}
							}
						}
					}
				}
			}

			if (bEndSegment && bDemonHead)
			{
				// head model can be animated - modify the spline to make it pass through the animated root position.
				int iPoint = Max(0, Spline.m_lPoints.Len() - 2);
				CSpline_Vec3Dfp32::Point& p2 = Spline.m_lPoints[iPoint];		// we want to modify the second last spline point
				p2.m_Pos = StartPos_Render.GetRow(3);							// update position
				p2.m_Tangent = StartPos_Render.GetRow(0);						// update direction
				
				// If devouring, change tangent at third last point, (first point that hasen't anything to do with the head)
				if (ArmState.IsTask_DevourTarget() && ArmState.m_iTarget)
				{
					fp32 DevourExtrudeTime = GetDevourBlendTime(ArmSetup.m_iAGToken, IPTime);
					if (!AlmostEqual(DevourExtrudeTime, 0.0f, 0.0001f))
					{
						// Fix devour points
						CSpline_Vec3Dfp32::Point& p1 = Spline.m_lPoints[Max(0, Spline.m_lPoints.Len() - 1)];
						CSpline_Vec3Dfp32::Point& p3 = Spline.m_lPoints[Max(0, Spline.m_lPoints.Len() - 3)];
						CSpline_Vec3Dfp32::Point& p4 = Spline.m_lPoints[Max(0, Spline.m_lPoints.Len() - 4)];
						CSpline_Vec3Dfp32::Point& p5 = Spline.m_lPoints[Max(0, Spline.m_lPoints.Len() - 5)];

						// Fix devour points
						p3.m_Pos = p2.m_Pos - (p2.m_Tangent * 3.0f);
						p4.m_Pos = CFXSysUtil::LerpMT(p3.m_Pos, p5.m_Pos, 0.25f);

						p3.m_Tangent = p2.m_Tangent;
						p4.m_Tangent = (p1.m_Pos - p5.m_Pos).Normalize();
					}
				}
				
				Spline.Finalize(Spline.m_nCachePoints);							// re-calc spline
				PosOnSpline = Spline.GetDistance((fp32)iPoint) + Length;		// calc new 'PosOnSpline'
				StartPos = StartPos_Render;										// make sure segments snaps together

				if (DEBUG_RENDER_SPLINE_MODIFIED && pWC)
					DebugRenderSpline(Spline, *pWC, true);
			}

			EndPos = StartPos;
			PosOnSpline -= Length;
			if (!bEndSegment)
				TotalSpin += Length * ArmSetup.m_Spin;
		}
	}
	
#ifndef M_RTM
	if (!(pWC && HIDE_MODELS_WHEN_RENDERING_WIREFRAME))
#endif
	{
		PowerStreaks.Render(_pEngine, &m_PowerStreaks, _pWClient->GetGameTickTime(), IPTime, pWC);
	}

	m_LastRender = RenderTime;
}


void CWO_TentacleSystem_ClientData::OnRender_AnimateTounge_TEMP(CWorld_Client* _pWClient, CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInst, uint _Seed)
{
	enum
	{
		DH_TOUNGE_ROOT	= 26,
		DH_TOUNGE_1		= 27,
		DH_TOUNGE_2		= 28,
		DH_TOUNGE_3		= 29,
		DH_TOUNGE_4		= 30,
	};

	CMat4Dfp32& DhToungeRootMat = _pSkelInst->m_pBoneTransform[DH_TOUNGE_ROOT];
	CMat4Dfp32& DhTounge1Mat = _pSkelInst->m_pBoneTransform[DH_TOUNGE_1];
	CMat4Dfp32& DhTounge2Mat = _pSkelInst->m_pBoneTransform[DH_TOUNGE_2];
	CMat4Dfp32& DhTounge3Mat = _pSkelInst->m_pBoneTransform[DH_TOUNGE_3];
	CMat4Dfp32& DhTounge4Mat = _pSkelInst->m_pBoneTransform[DH_TOUNGE_4];

	fp32 SinVal0 = M_Sin(_pWClient->GetGameTime().GetTime() * 2.00f);
	fp32 SinVal1 = M_Sin(_pWClient->GetGameTime().GetTime() * 1.50f);
	fp32 SinVal2 = M_Sin(_pWClient->GetGameTime().GetTime() * 1.00f);
	fp32 SinVal3 = M_Sin(_pWClient->GetGameTime().GetTime() * 0.75f);
	fp32 SinVal4 = M_Sin(_pWClient->GetGameTime().GetTime() * 0.40f);

	DhToungeRootMat.GetRow(3) += (DhToungeRootMat.GetRow(2) * 0.125f * SinVal0);
	DhTounge1Mat.GetRow(3) += (DhTounge1Mat.GetRow(2) * 0.001f * SinVal1);
	DhTounge2Mat.GetRow(3) += (DhTounge2Mat.GetRow(2) * 0.020f * SinVal2);
	DhTounge3Mat.GetRow(3) += (DhTounge3Mat.GetRow(2) * 0.075f * SinVal3);
	DhTounge4Mat.GetRow(3) += (DhTounge4Mat.GetRow(2) * 0.075f * SinVal4);

	DhTounge1Mat.M_x_RotY(0.001f * SinVal1);
	DhTounge2Mat.M_x_RotY(0.020f * SinVal2);
	DhTounge3Mat.M_x_RotY(0.075f * SinVal3);
	DhTounge4Mat.M_x_RotY(0.075f * SinVal4);
}
