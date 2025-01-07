/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			WModel_BloodEffect.h

Author:			Patrik Willbo

Copyright:		2006 Starbreeze Studios AB

Contents:		CXR_Model_BloodEffect

Comments:

History:
\*____________________________________________________________________________________________*/
#ifndef __INC__WModel_BloodEffect_h__
#define __INC__WModel_BloodEffect_h__


#include "WModel_EffectSystem.h"


enum
{
	BLOODEFFECT_LIGHTING =		M_Bit(0),
	BLOODEFFECT_LIGHTS_MAX =	3,
};


typedef CSpline_Beam CSpline_BloodSpurt;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBloodEffectModelParam
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CBloodEffectModelParam
{
public:
	enum
	{
		MaxNumPoints = 4,
	};

	CVec3Dfp32*		m_pEntrance;
	CVec3Dfp32*		m_pExit;
	CVec3Dfp32*		m_pPointData;
	fp32*			m_pLength;
	uint8			m_nPoints[MaxNumPoints];
	uint8			m_bCloudEntrance : 1;
	uint8			m_bCloudExit : 1;
	
	CBloodEffectModelParam()
		: m_pEntrance(NULL)
		, m_pExit(NULL)
		, m_pPointData(NULL)
		, m_pLength(NULL)
		, m_bCloudEntrance(0)
		, m_bCloudExit(0)
	{
		FillChar(m_nPoints, MaxNumPoints, 0x0);
	}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBloodEffectRenderParam
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CBloodEffectRenderParam
{
public:
	CXR_Engine*				m_pEngine;
	CXR_VBManager*			m_pVBM;
	CXR_SurfaceContext*		m_pSC;
	CMat4Dfp32				m_M2W;
	CMat4Dfp32				m_W2V;
	CXR_RenderInfo			m_RenderInfo;
	CXR_AnimState*			m_pAnimState;
	CBloodEffectModelParam*	m_pModelParam;

	// Lighting render parameters
	CBox3Dfp32				m_BBox;
	CXR_RenderSurfExtParam	m_Params;
	uint					m_Flags;
	CXR_Light				m_lLights[BLOODEFFECT_LIGHTS_MAX];

	CBloodEffectRenderParam() : m_pEngine(NULL), m_pVBM(NULL), m_pSC(NULL), m_RenderInfo(NULL), m_pModelParam(NULL), m_Flags(0) {}

	bool Init(CXR_VBManager* _pVBM, CXR_Engine* _pEngine, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _M2W, const CMat4Dfp32& _W2V)
	{
		m_pVBM = _pVBM;
		m_pEngine = _pEngine;
		m_pSC = (_pEngine) ? _pEngine->GetSC() : NULL;

		m_M2W = _M2W;
		m_W2V = _W2V;

		if(_pAnimState)
		{
			m_pAnimState = const_cast<CXR_AnimState*>(_pAnimState);
			m_pModelParam = (_pAnimState->m_Data[0] != -1) ? (CBloodEffectModelParam*)_pAnimState->m_Data[0] : NULL;
		}
		else
		{
			m_pAnimState = NULL;
			m_pModelParam = NULL;
		}

		m_RenderInfo.Clear(_pEngine);

		return IsValid();
	}

	bool IsValid()
	{
		return (m_pEngine && m_pVBM && m_pSC && m_pModelParam && m_pAnimState);
	}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_BloodEffect
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Model_BloodEffect : public CXR_Model_Custom
{
	MRTC_DECLARE;

	enum
	{
		MAXSURFACES =				8,
	};

	struct SBloodSurfaces
	{
		uint16	m_lSurfaceID[MAXSURFACES];
		uint8	m_nSurfaces;

		void Clear()
		{
			m_nSurfaces = 0;
			FillW(m_lSurfaceID, MAXSURFACES, 0x0);
		}
	};

	SBloodSurfaces	m_CloudSurfaces;
	SBloodSurfaces	m_SpurtSurfaces;
	fp32			m_SpurtWidth1;
	fp32			m_SpurtWidth2;
	fp32			m_SpurtLength1;
	fp32			m_SpurtLength2;
	fp32			m_CloudRadius1;
	fp32			m_CloudRadius2;
	fp32			m_CloudHeight1;
	fp32			m_CloudHeight2;

	// Blood effect 'components' creation
	void	OnCreate_Spurt(CFXVBMAllocUtil* _pVBMHelp, CSpline_BloodSpurt* _pSpurtSpline, CBloodEffectModelParam* _pModelParam, const int32 _Seed);
	void	OnCreate_Cloud(CFXVBMAllocUtil* _pVBMHelp, CBloodEffectModelParam* _pModelParam, const int32 _Seed);

	// Blood effect 'components' rendering
	void	OnRender_SetupLights(CBloodEffectRenderParam* _pRP);
	void	OnRender_Spurt(CFXVBMAllocUtil* _pVBMHelp, CSpline_BloodSpurt* _pSpurtSpline, CBloodEffectRenderParam* _pRP);
	void	OnRender_Cloud(CFXVBMAllocUtil* _pVBMHelp, CBloodEffectRenderParam* _pRP, const int32 _Seed);

public:
	CXR_Model_BloodEffect();

	virtual void OnCreate(const char* _pParam);
	virtual void OnRender(CXR_Engine *_pEngine, CRenderContext *_pRender, CXR_VBManager *_pVBM, CXR_ViewClipInterface *_pViewClip,
						  spCXR_WorldLightState _spWLS, const CXR_AnimState *_pAnimState, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat, int _Flags);
};


#endif

