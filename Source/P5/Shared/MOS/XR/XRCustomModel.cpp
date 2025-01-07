#include "PCH.h"

#include "XRCustomModel.h"
#include "MFloat.h"
#include "XREngine.h"
#include "XRFog.h"
#include "XRVBManager.h"
#include "XRSurfaceContext.h"
#include "XREngineVar.h"
#include "XRSkeleton.h"

//CXR_WorldLightState CXR_Model_Custom::m_WLS;

CXR_Model_Custom::CXR_Model_Custom()
{
	m_BoundRadius = 16.0f;
//	m_FadeTime.Reset();
//	m_InvFadeTime = 0;
//	m_pEngine = NULL;
//	m_WLS.Create(256, 4, 4);
}

CXR_Model_Custom::~CXR_Model_Custom()
{
}

fp32 CXR_Model_Custom::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	return m_BoundRadius;
}

void CXR_Model_Custom::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	_Box.m_Max = m_BoundRadius;
	_Box.m_Min = -m_BoundRadius;
}

//----------------------------------------------------------------------

void CXR_Model_Custom::ParseKeys(const char* _Keys)
{
	if (_Keys != NULL)
	{
		int iKey = 0;

		// FIXME: CStr is slower than CFStr, but otherwise CStr::Capture() fails in RenameThisKey, I think =).
		CStr keys(_Keys);
		CStr key = keys.GetStrSep(",");
		CStr name, value;

		key.Trim();
		
		while ((keys != "") || (key != ""))
		{
			if (key != "")
			{
				name = key.GetStrSep("=");
				name.MakeUpperCase();

				if (name == "")
					name = CStrF("%d",iKey);

				name.Trim();
				key.Trim();

				CRegistry_Dynamic regkey;
				regkey.SetThisName(name.Str());
				regkey.SetThisValue(key.Str());
				OnEvalKey(&regkey);
			}

			key = keys.GetStrSep(",");
			iKey++;
		}
	}
}

//----------------------------------------------------------------------

void CXR_Model_Custom::OnEvalKey(const CRegistry *_pReg)
{
}

//----------------------------------------------------------------------

void CXR_Model_Custom::Create(const char *_pParam)
{
	MACRO_GetRegisterObject(CTextureContext, pTContext, "SYSTEM.TEXTURECONTEXT");
	if(!pTContext)
		Error("-", "No texture-context available.");
	m_pTextureContext = pTContext;

	MACRO_GetRegisterObject(CXR_SurfaceContext, pSContext, "SYSTEM.SURFACECONTEXT");
	if(!pSContext)
		Error("-", "No surface-context available.");
	m_pSurfaceContext = pSContext;

	OnCreate(_pParam);
}

void CXR_Model_Custom::OnPrecache(class CXR_Engine* _pEngine, int _iVariation)
{
	for(int i = 0; i < m_lRequestedSurfaces.Len(); i++)
	{
		CXW_Surface *pSurf = m_pSurfaceContext->GetSurface(m_lRequestedSurfaces[i]);
		if(pSurf)
		{
			CXW_Surface *pVer = pSurf->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
			if(pVer)
			{
				pSurf->InitTextures(false);	// Don't report failures.
				pSurf->SetTextureParam(CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
			}
		}
	}

	for(int t = 0; t < m_lRequestedTextures.Len(); t++)
		m_pTextureContext->SetTextureParam(m_lRequestedTextures[t], CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
}

CXR_VertexBuffer *CXR_Model_Custom::AllocVB(CXR_Model_Custom_RenderParams* _pRenderParams, CRC_Attributes *_pAttrib)
{
	CXR_VertexBuffer *pVB = _pRenderParams->m_pVBM->Alloc_VB();
	if(!pVB)
		return NULL;

	CRC_Attributes *pAttrib = _pAttrib;
	if(!pAttrib)
	{
		pAttrib = _pRenderParams->m_pVBM->Alloc_Attrib();
		if(!pAttrib)
			return NULL;
		pAttrib->SetDefault();
	}

	pVB->m_pAttrib = pAttrib;
	pVB->m_Priority = _pRenderParams->m_RenderInfo.m_BasePriority_Transparent;
	return pVB;
}

CRC_Attributes *CXR_Model_Custom::AllocAttrib(CXR_Model_Custom_RenderParams* _pRenderParams)
{
	CRC_Attributes *pAttrib = _pRenderParams->m_pVBM->Alloc_Attrib();
	if(!pAttrib)
		return NULL;
	
	pAttrib->SetDefault();
	return pAttrib;
}

fp32 CXR_Model_Custom::GetCorrectedDistance(CXR_Engine* _pEngine, fp32 _Distance)
{
	if (_pEngine)
	{
		_Distance += _pEngine->m_LODOffset;
		_Distance *= _pEngine->m_LODScale;
	}
	return _Distance;
}

//void CXR_Model_Custom::PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
//{
//}

void CXR_Model_Custom::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MSCOPESHORT(CXR_Model_Custom::OnRender);

	CXR_Model_Custom_RenderParams RenderParams;
	RenderParams.m_pEngine	= _pEngine;
	RenderParams.m_RenderInfo.m_pCurrentEngine = _pEngine;
	RenderParams.m_pVBM = _pVBM;

	if (_pViewClip && !(_Flags & CXR_MODEL_ONRENDERFLAGS_NOCULL))
	{
		// Get bound-sphere, get the CRC_ClipView
		CBox3Dfp32 Box, BoxWorld;
		GetBound_Box(Box, _pAnimState);
		Box.Transform(_WMat, BoxWorld);

#ifndef M_RTM
		if (M_Fabs(BoxWorld.m_Max[0] - BoxWorld.m_Min[0]) > 2000.0f ||
			M_Fabs(BoxWorld.m_Max[1] - BoxWorld.m_Min[1]) > 2000.0f ||
			M_Fabs(BoxWorld.m_Max[2] - BoxWorld.m_Min[2]) > 2000.0f)
		{
			static bool bWarnOnce = true;
			if(bWarnOnce)
			{
//				ConOutL(CStrF("(0x%.8x) Extremely large boundingbox on custom-model: %s", this, BoxWorld.GetString().Str() ));
				ConOutL(CStrF("Extremely large boundingbox on custom-model: %s", BoxWorld.GetString().Str() ));
				bWarnOnce = false;
			}
		}
#endif
		bool bClipRes;
/*		if( _pAnimState->m_iObject )
		{
			_pViewClip->View_GetClip(_pAnimState->m_iObject, &m_RenderInfo);
			bClipRes = (m_RenderInfo.m_Flags & CXR_RENDERINFO_INVISIBLE) == 0;
		}
		else*/
		{
			bClipRes = _pViewClip->View_GetClip_Box(BoxWorld.m_Min, BoxWorld.m_Max, 0, 0, NULL, &RenderParams.m_RenderInfo);
		}

		if (!bClipRes && !RenderParams.m_RenderInfo.m_nLights)
			return;

//		m_RenderInfo.m_pCurrentEngine = _pEngine;
//		if( !_pViewClip->View_GetClip_Box(BoxWorld.m_Min, BoxWorld.m_Max, 0, 0, NULL, &m_RenderInfo) )
//			return;

//	ConOutL(CStrF("(%.8x) Prior %f, Box %s", this, m_RenderInfo.m_BasePriority_Transparent, BoxWorld.GetString().Str()));

		
/*		fp32 Radius = GetBound_Sphere(_pAnimState);
		if (!_pViewClip->View_GetClip_Sphere(CVec3Dfp32::GetMatrixRow(_WMat, 3), Radius, 0, 0, NULL, &m_RenderInfo))
			return;*/
	}

	Render(&RenderParams, _pAnimState, _WMat, _VMat);
}

void CXR_Model_Custom::Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
{
	CXR_VertexBuffer *pVB = AllocVB(_pRenderParams);
	if(pVB)
	{
		if(RenderVB(pVB, _pAnimState, _WMat, _VMat))
			_pRenderParams->m_pVBM->AddVB(pVB);	
	}
}

fp32 CXR_Model_Custom::GetRandInterpolated(int _iBaseRand, fp32 _Time)
{
	int Time = (int)_Time;
	fp32 Frac = _Time - Time;

	float r0 = MFloat_GetRand(_iBaseRand + Time);
	float r1 = MFloat_GetRand(_iBaseRand + Time + 1);
	return r0 + (r1 - r0) * Frac;
}

void CXR_Model_Custom::SetSplineWeights(fp32 _Frac, fp32 _b[4])
{
	//Beizer:	11 mul, 7 add
	//Cubic:	10 mul, 6 add

	fp32 t = _Frac;
	fp32 t2 = t * t;
	fp32 t3 = t * t * t;

	//Beizer
/*	const fp32 Mat[4][4] = {
		{-1.0f/6,	0.5,   -0.5,	1.0f/6},
		{ 0.5,	   -1,		0,		2.0f/3},
		{-0.5,		0.5,	0.5,	1.0f/6},
		{ 1.0f/6,	0,		0,		0}};*/

	//Cubic
	const fp32 Mat[4][4] = {
		{-0.5,	1,	 -0.5,	0},
		{ 1.5, -2.5,  0,	1},
//		{-1.5,  2,    -0.5,	0},
		{-1.5,  2,    0.5,	0},	//TODO: Try the line with -0.5. I must have read wrong somewhere
		{ 0.5, -0.5,  0,	0}};

	_b[0] = (Mat[0][3] + Mat[0][2] * t + Mat[0][1] * t2 + Mat[0][0] * t3);
	_b[1] = (Mat[1][3] + Mat[1][2] * t + Mat[1][1] * t2 + Mat[1][0] * t3);
	_b[2] = (Mat[2][3] + Mat[2][2] * t + Mat[2][1] * t2 + Mat[2][0] * t3);
	_b[3] = (Mat[3][3] + Mat[3][2] * t + Mat[3][1] * t2 + Mat[3][0] * t3);

}

fp32 CXR_Model_Custom::GetRandSplineInterpolated(const fp32 _b[4], int _iBaseRand, fp32 _Time)
{
	int Time = TruncToInt(_Time);
//	int Time = _Time;

	float r0 = MFloat_GetRand(_iBaseRand + Time + 0);
	float r1 = MFloat_GetRand(_iBaseRand + Time + 1);
	float r2 = MFloat_GetRand(_iBaseRand + Time + 2);
	float r3 = MFloat_GetRand(_iBaseRand + Time + 3);

	return _b[0] * r0 + _b[1] * r1 + _b[2] * r2 + _b[3] * r3;
}

/*
fp32 CXR_Model_Custom::GetFade(const CXR_AnimState *_pAnimState)
{
	fp32 Fade;
	
	if (m_FadeTime > 0)
	{
		fp32 Time = _pAnimState->m_AnimTime0;
		fp32 Duration = _pAnimState->m_AnimTime1;
		fp32 RevTime = Duration - Time;
		fp32 FadeIn, FadeOut;

		if (Time > 0)
		{
			if (Time < m_FadeTime)
			{
				FadeIn = Time * m_InvFadeTime;
			}
			else
			{
				FadeIn = 1.0f;
			}
		}
		else
		{
			FadeIn = 0.0f;
		}

		if (Duration > 0)
		{
			if (RevTime > 0)
			{
				if (RevTime < m_FadeTime)
				{
					FadeOut = RevTime * m_InvFadeTime;
				}
				else
				{
					FadeOut = 1.0f;
				}
			}
			else
			{
				FadeOut = 0.0f;
			}
		}
		else
		{
			FadeOut = 1.0f;
		}

		Fade = Min(FadeIn, FadeOut);
	}
	else
	{
		Fade = 1.0f;
	}

	return Fade;
}
*/
fp32 CXR_Model_Custom_RenderParams::GetFade(const CXR_AnimState *_pAnimState, float _Boost)
{
	fp32 Fade;
	
	if (!m_FadeTime.IsReset())
	{
		CMTime Time = _pAnimState->m_AnimTime0.Scale(_Boost);
		CMTime Duration = _pAnimState->m_AnimTime1;
		CMTime RevTime = Duration - Time;
		fp32 FadeIn, FadeOut;

		if (Time.Compare(0) > 0)
		{
			if (Time.Compare(m_FadeTime) < 0)
			{
				FadeIn = Time.GetTime() * m_InvFadeTime;
			}
			else
			{
				FadeIn = 1.0f;
			}
		}
		else
		{
			FadeIn = 0.0f;
		}

		if (Duration.Compare(0) > 0)
		{
			if (RevTime.Compare(0) > 0)
			{
				if (RevTime.Compare(m_FadeTime) < 0)
				{
					FadeOut = RevTime.GetTime() * m_InvFadeTime;
				}
				else
				{
					FadeOut = 1.0f;
				}
			}
			else
			{
				FadeOut = 0.0f;
			}
		}
		else
		{
			FadeOut = 1.0f;
		}

		Fade = Min(FadeIn, FadeOut);
	}
	else
	{
		Fade = 1.0f;
	}

	return Fade;
}

fp32 CXR_Model_Custom_RenderParams::GetFadeDown(const CXR_AnimState *_pAnimState)
{
	if(_pAnimState->m_AnimTime1.Compare(0) == 0)
		return 1.0f;

	if(_pAnimState->m_AnimTime0.Compare(_pAnimState->m_AnimTime1 - m_FadeTime) > 0)
	{
		if(_pAnimState->m_AnimTime0.Compare(_pAnimState->m_AnimTime1) >= 0)
			return 0.0f;
		else
			return (_pAnimState->m_AnimTime1 - _pAnimState->m_AnimTime0).GetTime() * m_InvFadeTime;
	}
	return 1.0f;
}

fp32 CXR_Model_Custom_RenderParams::GetFadeUp(const CXR_AnimState *_pAnimState)
{
	if(_pAnimState->m_AnimTime0.Compare(m_FadeTime) < 0)
		return _pAnimState->m_AnimTime0.GetTime() * m_InvFadeTime;
	
	return 1.0f;
}
/*
CPixel32 CXR_Model_Custom::GetCurrentFog(CXR_Model_Custom_RenderParams* _pRenderParams, const CMat4Dfp32 &_WMat)
{
	CPixel32 Fog;
	CXR_Engine* pEngine = _pRenderParams->m_pEngine;
	if((m_RenderInfo.m_Flags & CXR_RENDERINFO_VERTEXFOG) && pEngine->GetFogState())
	{
		pEngine->GetFogState()->SetTransform(NULL);
		pEngine->GetFogState()->VertexFog_Eval(1, &CVec3Dfp32::GetMatrixRow(_WMat, 3), NULL, &Fog, 0, 0);
	}
	else
		Fog = 0;
	
	return Fog;
}

fp32 CXR_Model_Custom::GetFogFade(CXR_Model_Custom_RenderParams* _pRenderParams, const CMat4Dfp32 &_WMat)
{
	CXR_Engine* pEngine = _pRenderParams->m_pEngine;
	CXR_FogState* pFogState = pEngine->GetFogState();

	if (!pFogState->m_DepthFogEnable)
		return 1.0f;

	const CVec3Dfp32& CameraPos = CVec3Dfp32::GetMatrixRow(pFogState->m_Eye, 3);
	const CVec3Dfp32& CameraFwd = CVec3Dfp32::GetMatrixRow(pFogState->m_Eye, 2);
	const CVec3Dfp32& SamplePoint = CVec3Dfp32::GetMatrixRow(_WMat, 3);
	CVec3Dfp32 DistanceVector = SamplePoint - CameraPos;
	fp32 Distance = DistanceVector * CameraFwd;

	if (Distance < 0)
		return 0.0f;

	fp32 Fade;
	Fade = 1.0f - Clamp01((Distance - pFogState->m_DepthFogStart) / (pFogState->m_DepthFogEnd - pFogState->m_DepthFogStart));
	Fade *= pFogState->m_DepthFogDensity;

	return Fade;
}
*/
void CXR_Model_Custom::Render_Surface(CXR_Model_Custom_RenderParams* _pRenderParams, int _iSurface, CXR_VertexBuffer *_pVB, const CMTime& _Time, int _Flags)
{
	CXW_Surface *pSurface = m_pSurfaceContext->GetSurfaceVersion(_iSurface, _pRenderParams->m_pEngine);
	Render_Surface(_pRenderParams, pSurface, _pVB, _Time, _Flags);
}

void CXR_Model_Custom::Render_Surface(CXR_Model_Custom_RenderParams* _pRenderParams, CXW_Surface *_pSurface, CXR_VertexBuffer *_pVB, const CMTime& _Time, int _Flags)
{
	CXR_Engine* pEngine = _pRenderParams->m_pEngine;
	CXW_Surface *pSurface = _pSurface->GetSurface(pEngine->m_SurfOptions, pEngine->m_SurfCaps);
	CXW_SurfaceKeyFrame *pKeyFrame = pSurface->GetFrame(0, _Time, &m_pSurfaceContext->GetTempSurfaceKeyFrame());

	_pVB->m_Priority += pSurface->m_PriorityOffset;
	
	int Flags = _Flags;
	if(_pVB->m_pTransform)
	{
		Flags |= RENDERSURFACE_MATRIXSTATIC_M2V;
	}
	CXR_Util::Render_Surface(Flags, _Time, pSurface, pKeyFrame, pEngine, _pRenderParams->m_pVBM, (CMat4Dfp32*) NULL, (CMat4Dfp32*) NULL, _pVB->m_pTransform, _pVB, _pVB->m_Priority, 0.0001f);
}

void CXR_Model_Custom::CalcPositionsFromSkeleton(CXR_Skeleton *_pSkel, const CMat4Dfp32 &_Mat, CVec3Dfp32 &_Pos0, int _Attach0)
{
	const CXR_SkeletonAttachPoint *pBegin = _pSkel->GetAttachPoint(_Attach0);
	if(pBegin)
		_Pos0 = pBegin->m_LocalPos.GetRow(3) * _Mat;
}

void CXR_Model_Custom::CalcPositionsFromSkeleton(CXR_Skeleton *_pSkel, const CMat4Dfp32 &_Mat, CVec3Dfp32 &_Pos0, CVec3Dfp32 &_Pos1, int _Attach0, int _Attach1)
{
	const CXR_SkeletonAttachPoint *pBegin = _pSkel->GetAttachPoint(_Attach0);
	if(pBegin)
		_Pos0 = pBegin->m_LocalPos.GetRow(3) * _Mat;
	const CXR_SkeletonAttachPoint *pEnd = _pSkel->GetAttachPoint(_Attach1);
	if(pEnd)
		_Pos1 = pEnd->m_LocalPos.GetRow(3) * _Mat;
}

int CXR_Model_Custom::GetSurfaceID(const char *_pSurface)
{
	int iSurf = m_pSurfaceContext->GetSurfaceID(_pSurface);
	if(iSurf > 0)
		m_lRequestedSurfaces.Add(iSurf);
	
	return iSurf;
}

int CXR_Model_Custom::GetTextureID(const char *_pTexture)
{
	int iTexture = m_pTextureContext->GetTextureID(_pTexture);
	if(iTexture > 0)
		m_lRequestedTextures.Add(iTexture);

	return iTexture;
}

