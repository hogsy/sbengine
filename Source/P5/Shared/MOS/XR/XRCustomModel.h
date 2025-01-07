#ifndef __XRCUSTOMMODEL_H
#define __XRCUSTOMMODEL_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Base class for custom models

	Author:			Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CXR_Model_Custom
					CCritters
\*____________________________________________________________________________________________*/

#include "XRClass.h"
#include "XRVBManager.h"
#include "XRVertexBuffer.h"
#include "MFloat.h"
#include "XRUtil.h"

class CXR_Model_Custom_RenderParams
{
protected:
	CMTime	m_FadeTime;
	fp32	m_InvFadeTime;
public:
	CXR_Engine* m_pEngine;
	CXR_RenderInfo m_RenderInfo;
	CXR_VBManager* m_pVBM;

	CXR_Model_Custom_RenderParams() : m_RenderInfo(NULL)
	{
		m_FadeTime.Reset();
		m_InvFadeTime = 0;
		m_pEngine = NULL;
		m_pVBM = NULL;
	}
	void SetFadeTime(fp32 _Seconds) {m_FadeTime = CMTime::CreateFromSeconds(_Seconds); m_InvFadeTime = 1.0f / _Seconds;}
	fp32 GetFadeTime() const {return m_FadeTime.GetTime();};
	fp32 GetFade(const CXR_AnimState *_pAnimState, float _Boost = 1.0f);
	fp32 GetFadeDown(const CXR_AnimState *_pAnimState);
	fp32 GetFadeUp(const CXR_AnimState *_pAnimState);
};

// -------------------------------------------------------------------
//  CXR_Model_Custom
// -------------------------------------------------------------------
class CXR_Model_Custom : public CXR_Model
{
private:
	class CTextureContext *m_pTextureContext;
	class CXR_SurfaceContext *m_pSurfaceContext;
	TArray<int> m_lRequestedSurfaces;
	TArray<int> m_lRequestedTextures;

protected:
	fp32 m_BoundRadius;		// Defaulting to 16
//	fp32 m_FadeTime;
//	CMTime m_FadeTime;
//	fp32 m_InvFadeTime;
//	CXR_Engine *m_pEngine;
//	CXR_RenderInfo m_RenderInfo;
	
//	spCXR_WorldLightState m_pWLS;
//	static CXR_WorldLightState m_WLS;
//	CXR_VBManager *m_pVBM;

public:
	CXR_Model_Custom();
	~CXR_Model_Custom();

	void ParseKeys(const char *_Keys);
	virtual void OnEvalKey(const CRegistry *_pReg);

	virtual void Create(const char* _pParam);

	virtual void OnPrecache(class CXR_Engine* _pEngine, int _iVariation);

	virtual void ModifyEntry(CMat4Dfp32& _Matrix, fp32& _Time) {};


	virtual fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState = NULL);
	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState);

//	virtual void PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip, const CXR_AnimState* _pAnimState, 
//						   const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags);
	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
			const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags);

	//Overridables
	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat);
	virtual bool RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat) { return false; }
	virtual void OnCreate(const char* _pParam) {}

	fp32 GetCorrectedDistance(CXR_Engine* _pEngine, fp32 _Distance);

	//Helpers

	void Render_Surface(CXR_Model_Custom_RenderParams* _pRenderParams, int _iSurface, CXR_VertexBuffer *_pVB, const CMTime& _Time, int _Flags = RENDERSURFACE_DEPTHFOG); //  | RENDERSURFACE_VERTEXFOG
	void Render_Surface(CXR_Model_Custom_RenderParams* _pRenderParams, CXW_Surface *_pSurface, CXR_VertexBuffer *_pVB, const CMTime& _Time, int _Flags = RENDERSURFACE_DEPTHFOG); //  | RENDERSURFACE_VERTEXFOG

	void CalcPositionsFromSkeleton(CXR_Skeleton *_pSkel, const CMat4Dfp32 &_Mat, CVec3Dfp32 &_Pos0, CVec3Dfp32 &_Pos1, int _Attach0, int _Attach1);
	void CalcPositionsFromSkeleton(CXR_Skeleton *_pSkel, const CMat4Dfp32 &_Mat, CVec3Dfp32 &_Pos0, int _Attach0);

	int GetSurfaceID(const char *_pSurface);
	int GetTextureID(const char *_pTexture);

//	CPixel32 GetCurrentFog(CXR_Model_Custom_RenderParams* _pRenderParams, const CMat4Dfp32 &_WMat);
//	fp32 GetFogFade(CXR_Model_Custom_RenderParams* _pRenderParams, const CMat4Dfp32 &_WMat);

	CXR_VertexBuffer *AllocVB(CXR_Model_Custom_RenderParams* _pRenderParams, CRC_Attributes *_pAttrib = NULL);
	CRC_Attributes *AllocAttrib(CXR_Model_Custom_RenderParams* _pRenderParams);

	CXR_SurfaceContext *GetSurfaceContext() { return m_pSurfaceContext; }
	CTextureContext *GetTextureContext() { return m_pTextureContext; }

	static fp32 GetRandInterpolated(int _iBaseRand, fp32 _Time);
	static fp32 GetRandSplineInterpolated(const fp32 _b[4], int _iBaseRand, fp32 _Time);
	static void SetSplineWeights(fp32 _Frac, fp32 _b[4]);


	fp32 InvSqrInv(fp32 _Value)
	{
		return (1.0f - Sqr(1.0f - _Value));
	}

	fp32 Sawtooth(fp32 _Time)
	{
		if(_Time >= 0.5)
			return 1.0f - (_Time - 0.5f) * 2;
		else
			return _Time * 2;
	}

	fp32 Tooth(fp32 _Time)
	{
		return 4 * (_Time - Sqr(_Time));
	}
};

// -------------------------------------------------------------------
//  CCritter
// -------------------------------------------------------------------
class CCritters
{
private:
	int m_nCritters;
	fp32 m_LifeSpan;
	fp32 m_InvCritters;
	fp32 m_InvLifeSpan;

	int m_TimeBase;
	fp32 m_TimeFrac;

protected:
	CCritters()
	{
		m_nCritters = 10;
		m_InvCritters = 1.0f / m_nCritters;
		m_LifeSpan = 1.0f;
		m_InvLifeSpan = 1.0f;
	}

	void Critters_Create(int _nCritters, fp32 _LifeSpan)
	{
		m_nCritters = _nCritters;
		m_InvCritters = 1.0f / m_nCritters;
		m_LifeSpan = _LifeSpan;
		m_InvLifeSpan = 1.0f / m_LifeSpan;
	}

	void Critters_SetTime(float _Time)
	{
		float fBase = (_Time * m_nCritters) * m_InvLifeSpan;
		m_TimeBase = TruncToInt(fBase) + 1;
		m_TimeFrac = (fBase - m_TimeBase) * m_InvCritters;
	}

	fp32 Critters_GetTime(int _iCritter)
	{
		return 1.0f - (fp32(_iCritter) * m_InvCritters) + m_TimeFrac;
	}

	int Critters_GetID(int _iCritter)
	{
		return m_TimeBase + _iCritter;
	}
};

#endif
