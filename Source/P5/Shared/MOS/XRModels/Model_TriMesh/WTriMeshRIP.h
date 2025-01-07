#ifndef _INC_WTRIMESHRIP
#define _INC_WTRIMESHRIP

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			RenderInstanceParameters of Trimesh model

Author:			mh..?

Copyright:		Copyright Starbreeze Studios AB 2002

Contents:		implementation of the class used to contain information
				about trimesh rendering

History:		
2005-12-15:		ae, created this file and moved class here from 
				WTriMesh.cpp
\*____________________________________________________________________*/

enum
{
	CTM_MAX_MERGECLUSTERS	= 32
};

class CTriMesh_RenderInstanceParamters
{
public:
	CXR_Engine* m_pCurrentEngine;
	CXR_VBManager*	m_pVBM;
	const CXR_AnimState*	m_pCurAnimState;
	CXR_RenderInfo m_RenderInfo;
	CXR_VertexBuffer m_RenderVB;
	CVec3Dfp32* m_pRenderV;
	CVec3Dfp32* m_pRenderN;
	CVec3Dfp32* m_pRenderTangU;
	CVec3Dfp32* m_pRenderTangV;
	CVec2Dfp32* m_pRenderTV;
	int m_RenderSurfOptions;
	int m_RenderSurfCaps;

	const CMat4Dfp32* m_pVMat;
	// Align 
	CMat4Dfp32 m_CurWorld2VertexMat;
	CMat4Dfp32 m_CurVertex2WorldMat;
	CMat4Dfp32 m_CurWorld2LocalMat;
	CMat4Dfp32 m_CurVertex2LocalMat;
	CMat4Dfp32 m_CurL2VMat;
	const CMat4Dfp32* m_pCurWVelMat;
	CVec3Dfp32 m_CurViewVP;
	CVec3Dfp32 m_CurWorldVP;
	CVec3Dfp32 m_CurWorldPosition;

	bool m_bRender_Unified;
	bool m_bRender_ProjLight;
	int m_bRenderTempTLEnable;
	int m_bRenderFastLight;
	CPixel32 m_FastLight;
	int m_OnRenderFlags;
	int m_bIsMirrored;

	class CXR_FogState* m_pSceneFog;
	CXR_WorldLightState m_CurrentWLS;

	class CRC_Attributes* m_pLightAttrib[32];

	uint8 M_ALIGN(16) m_plTmpSurfaceKeyFramesData[sizeof(CXW_SurfaceKeyFrame) * TRIMESH_NUMSURF];
	CXW_SurfaceKeyFrame* m_lpTmpSurfaceKeyFrames[TRIMESH_NUMSURF];
	CXW_SurfaceKeyFrame* m_lpSurfaceKeyFrames[TRIMESH_NUMSURF];
	CXW_Surface* m_lpSurfaces[TRIMESH_NUMSURF];
	CMTime m_lSurfaceTime[TRIMESH_NUMSURF];

	CXR_Light * m_pRenderLights;
	int m_nRenderLights;
	CVec4Dfp32 m_lLightFieldAxes[6];

	CBox3Dfp32 m_RenderFogBox;
	CPixel32 m_RenderFog[8];
	CMTime m_Time;
	CScissorRect m_RenderBoundScissor;
	CXR_LightInfo m_RenderLightInfo[CTM_MAX_LIGHTS];
	bool m_bRenderFogWorldSpace;

	CXR_LightOcclusionInfo m_pLO[CTM_MAX_LIGHTS];
	uint16 m_piDstLO[CTM_MAX_LIGHTS];
	uint16 m_piSrcLO[CTM_MAX_LIGHTS];
	TBitArray<CTM_MAX_LIGHTS> m_LOUsage;								// One bit per light
	uint m_nLOMaxLights;
	uint m_nLO;

	uint16*** m_pppShadowPrimData;
	uint32 m_pVpuShadowTaskId;

	CTM_Cluster* m_lpCluster[CTM_MAX_MERGECLUSTERS];
	uint16 m_iCluster[CTM_MAX_MERGECLUSTERS];
	uint32 m_nClusters;

	CScissorRect m_lLightScissors[CTM_MAX_LIGHTS];
	CVec4Dfp32* m_plLightParams;
	uint32 m_plLightCulled[CTM_MAX_LIGHTS];
	CRC_Attributes* m_lpShadowLightAttributes[CTM_MAX_LIGHTS];
	CRC_Attributes* m_lpShadowLightAttributesBackside[CTM_MAX_LIGHTS];
	fp32 m_PrioCoverageMin;
	fp32 m_PrioCoverageMax;
	fp32 m_PriorityOverride;

	CTriMesh_RenderInstanceParamters(CXR_Engine* _pEngine, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState) : m_pCurrentEngine(_pEngine), m_pVBM(_pVBM), m_pCurAnimState(_pAnimState), m_RenderInfo(_pEngine)
	{
		m_RenderVB.Clear();
		m_pRenderV = 0;
		m_pRenderN = 0;
		m_pRenderTangU = 0;
		m_pRenderTangV = 0;
		m_pRenderTV = 0;
		m_RenderSurfOptions = 0;
		m_RenderSurfCaps = 0;

		m_bRender_Unified	= false;
		m_bRender_ProjLight	= false;
		m_bRenderTempTLEnable	= false;
		m_bRenderFastLight	= false;
		m_FastLight	= 0xffffffff;
		m_OnRenderFlags	= 0;
		m_pCurWVelMat = NULL;

		m_pSceneFog	= 0;

		m_pRenderLights	= NULL;
		m_nRenderLights	= 0;
		vec128 z = M_VZero();
		m_lLightFieldAxes[0].v = z;
		m_lLightFieldAxes[1].v = z;
		m_lLightFieldAxes[2].v = z;
		m_lLightFieldAxes[3].v = z;
		m_lLightFieldAxes[4].v = z;
		m_lLightFieldAxes[5].v = z;

		m_bRenderFogWorldSpace	= false;

		m_nLOMaxLights = 0;
		m_LOUsage.Clear();
		m_pppShadowPrimData = NULL;
		m_pVpuShadowTaskId = 0xffffffff;
		m_bIsMirrored = (_pEngine) ? _pEngine->GetVC()->m_bIsMirrored : false;
		for(int i = 0; i < TRIMESH_NUMSURF; i++)
			m_lpTmpSurfaceKeyFrames[i] = NULL;

		m_nClusters = 0;
		m_plLightParams = 0;

		m_PrioCoverageMin = _FP32_MAX;
		m_PrioCoverageMax = -_FP32_MAX;
		m_PriorityOverride = 0.0f;
	}

	~CTriMesh_RenderInstanceParamters()
	{
		for(int i = 0; i < TRIMESH_NUMSURF; i++)
		{
			if(m_lpTmpSurfaceKeyFrames[i])
			{
				m_lpTmpSurfaceKeyFrames[i]->CXW_SurfaceKeyFrame::~CXW_SurfaceKeyFrame();
				m_lpTmpSurfaceKeyFrames[i] = NULL;
			}
		}
	}

	CXW_SurfaceKeyFrame* CreateTempSurfKeyFrame(int _iTemp)
	{
		M_ASSERT(_iTemp >= 0 && _iTemp < TRIMESH_NUMSURF, "!");
		if(m_lpTmpSurfaceKeyFrames[_iTemp] == NULL)
		{
			m_lpTmpSurfaceKeyFrames[_iTemp] = new (m_plTmpSurfaceKeyFramesData +(_iTemp * sizeof(CXW_SurfaceKeyFrame))) CXW_SurfaceKeyFrame;
		}

		return m_lpTmpSurfaceKeyFrames[_iTemp];
	}

	void ClearPointers()
	{
		MAUTOSTRIP(CTriMesh_RenderInstanceParamters, MAUTOSTRIP_VOID);
		m_pRenderV = NULL;
		m_pRenderN = NULL;
		m_pRenderTangU = NULL;
		m_pRenderTangV = NULL;
		m_pRenderTV = NULL;
		m_RenderVB.Clear();
	}
};

#endif
