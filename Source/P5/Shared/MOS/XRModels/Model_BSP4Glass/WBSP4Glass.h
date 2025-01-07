#ifndef _INC_WBSP4GLASSMODEL
#define _INC_WBSP4GLASSMODEL

#include "../Model_BSP4/WBSP4Model.h"
#include "../Model_BSP4/WBSP4Def.h"

#include "../MCC/MCC.h"
#include "../../XR/XRClass.h"
#include "../../XR/XRShader.h"
//#include "../../XR/XRSurf.h"

#include "WBSP4GlassCommon.h"
#include "WBSP4GlassDelaunay2D.h"


//#pragma optimize("", off)
//#pragma inline_depth(0)


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Defines
|__________________________________________________________________________________________________
\*************************************************************************************************/
#define GLASS_VERSION_100	0x0100
#define GLASS_VERSION_110	0x0110
#define GLASS_VERSION_120	0x0120


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Enums
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum
{
	GLASS_MSG_NA		= 0,
	GLASS_MSG_CRUSH_POINT,
	GLASS_MSG_CRUSH_SPHERE,
	GLASS_MSG_CRUSH_CUBE,
	GLASS_MSG_WALLMARK,
	GLASS_MSG_RESTORE,
	GLASS_MSG_RESTORE_ALL,
	GLASS_MSG_ACTIVE,

	GLASS_CRUSH_POINT	= 0,
	GLASS_CRUSH_CUBE,
	GLASS_CRUSH_SPHERE,
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Predefined classes
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Model_BSP4Glass;
class CXR_Model_BSP4Glass_Instance;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Velocity callback functions
|__________________________________________________________________________________________________
\*************************************************************************************************/
typedef void(*CBSP4Glass_VelocityCallback)(CVec3Dfp32& _Velocity, const CVec3Dfp32& _Vertex, const CBSP4Glass_CrushData& _CrushData);
void GlassDefaultVelocityCB(CVec3Dfp32& _Velocity, const CVec3Dfp32& _Vertex, const CBSP4Glass_CrushData& _CrushData);
void GlassVelocityFromPointCB(CVec3Dfp32& _Velocity, const CVec3Dfp32& _Vertex, const CBSP4Glass_CrushData& _CrushData);
void GlassVelocityZeroCB(CVec3Dfp32& _Velocity, const CVec3Dfp32& _Vertex, const CBSP4Glass_CrushData& _CrushData);


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_RenderParam
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CBSP4Glass_RenderParam
{
public:
	// Render parameters...
	CXR_ViewClipInterface*			m_pViewClip;
	CXR_LightOcclusionInfo*			m_pLO;
	int								m_nMaxLO;
	CXR_VBManager*					m_pVBM;
	CXR_Engine*						m_pEngine;
	bool							m_bDirtyLO;
	CXR_RenderInfo*					m_pRenderInfo;
	CXR_VBChain*					m_pLastVBChain;
	CRenderContext*					m_pRenderContext;
	CXR_AnimState*					m_pCurAnimState;
	CXR_WorldLightState*			m_pWLS;
	CXR_Model_BSP4Glass_Instance*	m_pGlassInstance;
	CMat4Dfp32						m_M2W;				// Model to world (WMat)
	CMat4Dfp32						m_W2V;				// World to view  (VMat)
	CMat4Dfp32						m_M2V;				// Model to view  (WMat * VMat)
	CMat4Dfp32						m_InvM2W;			// Inverse model to view (InvOrt WMat * VMat)
	CVec3Dfp32						m_MVPos;			// Modelspace view position

	TThinArray<CBSP4Glass_Chain>	m_lpRenderChain;
	TAP<CBSP4Glass_Chain>			m_lpTAPChain;
	CXR_VertexBuffer*				m_pVBList;
	CXR_VertexBuffer*				m_pCurVBList;
	uint32							m_nVBSingleListLen;
	CMat4Dfp32*						m_pVBMat;
	CMat4Dfp32*						m_pVBMatM2V;
	CRC_Attributes*					m_pMaskAttr;
	CMat4Dfp32*						m_pMPMats;

	CPlane3Dfp32						m_FrontPlaneW;
	fp32								m_BackPlaneInv;

	uint8 m_plTmpSurfKeyFrameData[sizeof(CXW_SurfaceKeyFrame) * ANIMSTATE_NUMSURF];
	CXW_SurfaceKeyFrame*			m_plTmpSurfKeyFrame[ANIMSTATE_NUMSURF];
	
	CBSP4Glass_RenderParam()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_RenderParam::CBSP4Glass_RenderParam, XR_BSP4GLASS);
		for(int i = 0; i < ANIMSTATE_NUMSURF; i++)
			m_plTmpSurfKeyFrame[i] = NULL;
	}

	~CBSP4Glass_RenderParam()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_RenderParam::~CBSP4Glass_RenderParam, XR_BSP4GLASS);
		for(int i = 0; i < ANIMSTATE_NUMSURF; i++)
		{
			if(m_plTmpSurfKeyFrame[i])
			{
				// Run destructor on created temp objects
				m_plTmpSurfKeyFrame[i]->CXW_SurfaceKeyFrame::~CXW_SurfaceKeyFrame();
				m_plTmpSurfKeyFrame[i] = NULL;
			}
		}
	}

	M_INLINE CXW_SurfaceKeyFrame& CreateTempSurfKeyFrame(int _iTemp)
	{
		M_ASSERT(_iTemp >= 0 && _iTemp < ANIMSTATE_NUMSURF, "CBSP4Glass_RenderParam::CreateTempSurfKeyFrame: Out of range!");
		
		if(m_plTmpSurfKeyFrame[_iTemp] == NULL)
			m_plTmpSurfKeyFrame[_iTemp] = new (m_plTmpSurfKeyFrameData + (_iTemp * sizeof(CXW_SurfaceKeyFrame))) CXW_SurfaceKeyFrame;

		return *m_plTmpSurfKeyFrame[_iTemp];
	}

	M_INLINE void ClearChain()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_RenderParam::ClearChain, XR_BSP4GLASS);
		m_lpRenderChain.Clear();
		m_lpTAPChain = m_lpRenderChain;
		m_pVBList = NULL;
	}

	M_INLINE void SetVBMat_M43(CMat4Dfp32* _pVBMat, const CMat43fp32& _Mat)
	{
		m_pVBMat = _pVBMat;
		m_pVBMat->CreateFrom(_Mat);
	}

	M_INLINE void SetVBMat_M4D(CMat4Dfp32* _pVBMat, const CMat4Dfp32& _Mat)
	{
		m_pVBMat = _pVBMat;
		*m_pVBMat= _Mat;
	}

	M_INLINE void SetVBMat_MP(CMat4Dfp32* _pVBMat, const CMat43fp32& _Mat)
	{
		m_pVBMat = _pVBMat;
		m_pVBMat->CreateFrom(_Mat);
	}

	M_INLINE CMat4Dfp32* GetVBMat()
	{
		return m_pVBMat;
	}

	M_INLINE void SetMaskAttr(CRC_Attributes* _pA)
	{
		m_pMaskAttr = _pA;
	}

	M_INLINE CRC_Attributes* GetMaskAttr()
	{
		return m_pMaskAttr;
	}

	M_INLINE void SetChainLen(const uint32 _Len)
	{
		m_lpRenderChain.SetLen(_Len);
		m_lpTAPChain = m_lpRenderChain;

		// Clear everything
		MemSet(m_lpRenderChain.GetBasePtr(), 0x0, sizeof(CBSP4Glass_Chain) * _Len);
		m_nVBSingleListLen = _Len;
	}

	M_INLINE uint32 GetChainLen()
	{
		return (uint32)m_lpRenderChain.Len();
	}

	M_INLINE CBSP4Glass_Chain* GetChain(const uint32 _i)
	{
		return &m_lpTAPChain[_i];
	}

	M_INLINE uint32 SetChain(const uint32 _i, CXR_VBChain* _pChain, CRC_MatrixPalette* _pMP)
	{
		m_lpTAPChain[_i].Set(_pChain, _pMP);
		return (_i + 1);
	}

	M_INLINE void RestartVBList()
	{
		m_pCurVBList = m_pVBList;
	}

	M_INLINE void SetVBList(CXR_VertexBuffer* _pVBList)
	{
		m_pVBList = _pVBList;
		m_pCurVBList = _pVBList;
	}

	M_INLINE void SetMPMatrices(CMat4Dfp32* _pMPMats)
	{
		m_pMPMats = _pMPMats;
	}

	M_INLINE CXR_VertexBuffer* GetVBList()
	{
		CXR_VertexBuffer* pVBList = m_pCurVBList;
		m_pCurVBList += m_nVBSingleListLen;
		return pVBList;
	}

	M_INLINE void Clear()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_RenderParam::Clear, XR_BSP4GLASS);

		m_pViewClip = NULL;
		m_pLO = NULL;
		m_nMaxLO = 0;
		m_pVBM = NULL;
		m_pEngine = NULL;
		m_bDirtyLO = false;
		m_pRenderInfo = NULL;
		m_pLastVBChain = NULL;
		m_pRenderContext = NULL;
		m_pCurAnimState = NULL;
		m_pWLS = NULL;
	}

	M_INLINE void InitializeRender(CXR_ViewClipInterface* _pViewClip, CXR_VBManager* _pVBM, CXR_Engine* _pEngine, CXR_RenderInfo* _pRenderInfo, CRenderContext* _pRender, CXR_AnimState* _pAnimState, CXR_WorldLightState* _pWLS)
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_RenderParam::InitializeRender, XR_BSP4GLASS);

		M_ASSERT(_pAnimState, "NULL animstate");

		m_pViewClip = _pViewClip;
		if(_pViewClip)
		{
			m_pLO = _pViewClip->View_Light_GetOcclusionArray(m_nMaxLO);
		}
		else
		{
			m_pLO = NULL;
			m_nMaxLO = 0;
		}
		m_pVBM = _pVBM;
		m_pEngine = _pEngine;
		m_bDirtyLO = false;
		m_pRenderInfo = _pRenderInfo;
		m_pLastVBChain = NULL;
		m_pRenderContext = _pRender;
		m_pCurAnimState = _pAnimState;
		m_pWLS = _pWLS;

		if(m_pCurAnimState->m_pModelInstance)
		{
			m_pGlassInstance = safe_cast<CXR_Model_BSP4Glass_Instance>(m_pCurAnimState->m_pModelInstance);
		}

		m_FrontPlaneW = _pEngine->GetVC()->m_FrontPlaneW;
		m_BackPlaneInv = 1.0f / _pEngine->GetVBM()->Viewport_Get()->GetBackPlane();
	}

	M_INLINE void SetupMatrices(const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_RenderParam::SetupMatrices, XR_BSP4GLASS);

		m_M2W = _WMat;
		m_W2V = _VMat;
		m_M2W.Multiply(m_W2V, m_M2V);
		m_M2W.InverseOrthogonal(m_InvM2W);

		m_MVPos.k[0] = -(m_M2V.k[3][0] * m_M2V.k[0][0] + m_M2V.k[3][1] * m_M2V.k[0][1] + m_M2V.k[3][2] * m_M2V.k[0][2]);
		m_MVPos.k[1] = -(m_M2V.k[3][0] * m_M2V.k[1][0] + m_M2V.k[3][1] * m_M2V.k[1][1] + m_M2V.k[3][2] * m_M2V.k[1][2]);
		m_MVPos.k[2] = -(m_M2V.k[3][0] * m_M2V.k[2][0] + m_M2V.k[3][1] * m_M2V.k[2][1] + m_M2V.k[3][2] * m_M2V.k[2][2]);
	}

	M_INLINE void RecalcMatrices(const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, bool _bKeepM2W = false, bool _bKeepW2V = false, bool _bKeepM2V = true, bool _bKeepInvM2W = true, bool _bKeepMVPos = true)
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_RenderParam::RecalcMatrices, XR_BSP4GLASS);

		// Recalc given matrices
		if(!_bKeepM2W)
			m_M2W = _WMat;

		if(!_bKeepW2V)
			m_W2V = _VMat;

		if(!_bKeepM2V)
			m_M2W.Multiply(m_W2V, m_M2V);

		if(!_bKeepInvM2W)
			m_M2W.InverseOrthogonal(m_InvM2W);

		if(!_bKeepMVPos)
		{
			m_MVPos.k[0] = -(m_M2V.k[3][0] * m_M2V.k[0][0] + m_M2V.k[3][1] * m_M2V.k[0][1] + m_M2V.k[3][2] * m_M2V.k[0][2]);
			m_MVPos.k[1] = -(m_M2V.k[3][0] * m_M2V.k[1][0] + m_M2V.k[3][1] * m_M2V.k[1][1] + m_M2V.k[3][2] * m_M2V.k[1][2]);
			m_MVPos.k[2] = -(m_M2V.k[3][0] * m_M2V.k[2][0] + m_M2V.k[3][1] * m_M2V.k[2][1] + m_M2V.k[3][2] * m_M2V.k[2][2]);
		}
	}

	void CommitRender()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_RenderParam::CommitRender, XR_BSP4GLASS);

		if(m_bDirtyLO)
		{
			m_pViewClip->View_Light_ApplyOcclusionArray(m_nMaxLO,NULL,m_pLO );
		}
	}

	void ExpandLightOcclusion(const int& _iLO, const CRect2Duint16& _Scissor)
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_RenderParam::ExpandLightOcclusion, XR_BSP4GLASS);

		if(_iLO < m_nMaxLO)
		{
			m_pLO[_iLO].m_ScissorShaded.Expand(_Scissor);
			m_pLO[_iLO].m_ScissorShadow.Expand(_Scissor);
			m_pLO[_iLO].m_ScissorVisible.Expand(_Scissor);
			m_bDirtyLO = true;
		}
	}

	M_INLINE bool IsValid_LightScissor(const int& _iLO)
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_RenderParam::IsValid_LightScissor, XR_BSP4GLASS);

		if(_iLO < m_nMaxLO)
			return m_pLO[_iLO].IsVisible();

		return false;
	}

	M_INLINE bool IsValid_AnimState()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_RenderParam::IsValid_AnimState, XR_BSP4GLASS);
		return (m_pCurAnimState->m_pModelInstance != NULL);
	}

	M_INLINE bool IsValid_Engine()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_RenderParam::IsValid_Engine, XR_BSP4GLASS);
		return (m_pEngine) ? true : false;
	}

	M_INLINE bool IsValid_ViewClip()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_RenderParam::IsValid_ViewClip, XR_BSP4GLASS);
		return (m_pViewClip) ? true : false;
	}

	M_INLINE bool IsValid_VBM()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_RenderParam::IsValid_VBM, XR_BSP4GLASS);
		return (m_pVBM) ? true : false;
	}

	M_INLINE bool IsValid_RenderInfo()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_RenderParam::IsValid_RenderInfo, XR_BSP4GLASS);
		return (m_pRenderInfo) ? true : false;
	}

	M_INLINE bool IsValid_RenderContext()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_RenderParam::IsValid_RenderContext, XR_BSP4GLASS);
		return (m_pRenderContext) ? true : false;
	}

	M_INLINE bool IsValid()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_RenderParam::IsValid, XR_BSP4GLASS);
		return (m_pCurAnimState->m_pModelInstance && m_pEngine && m_pViewClip && m_pVBM && m_pRenderInfo && m_pRenderContext);
	}

	M_INLINE CXR_Model_BSP4Glass_Instance* GetGlassInstance()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_RenderParam::GetGlassInstance, XR_BSP4GLASS);
		return m_pGlassInstance;
	}

	M_INLINE CXR_Shader* GetShader()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_RenderParam::GetShader, XR_BSP4GLASS);
		return m_pEngine->GetShader();
	}

	M_INLINE CXR_SceneGraphInstance* GetSceneGraphInstance()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_RenderParam::GetSceneGraphInstance, XR_BSP4GLASS);
		return m_pEngine->m_pSceneGraphInstance;
	}

	M_INLINE CXW_SurfaceKeyFrame* GetSurfaceKey(CXW_Surface* _pSurf, CXW_SurfaceKeyFrame& _SurfaceKey)
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_RenderParam::GetSurfaceKey, XR_BSP4GLASS);
		if(_pSurf->m_iController)
			return _pSurf->GetFrame(m_pCurAnimState->m_Anim1, m_pCurAnimState->m_AnimTime1, &_SurfaceKey);
		else
			return _pSurf->GetFrame(m_pCurAnimState->m_Anim0, m_pCurAnimState->m_AnimTime0, &_SurfaceKey);
	}

	M_INLINE CMTime GetSurfaceTime(CXW_Surface* _pSurf)
	{
		if(_pSurf->m_iController)
			return m_pCurAnimState->m_AnimTime1;
		else
			return m_pCurAnimState->m_AnimTime0;
	}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_BuildEdge
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CBSP4Glass_BuildEdge
{
public:

	struct SBSP4Glass_EdgeData
	{
		uint16	m_i1;
		uint16	m_i2;
		uint8	m_n;
	};

	TThinArray<SBSP4Glass_EdgeData>	m_lEdgeData;
	SBSP4Glass_EdgeData* m_pEdgeData;
	uint16 m_nEdges;
	uint16 m_nRealEdges;

	CBSP4Glass_BuildEdge()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_BuildEdge::CBSP4Glass_BuildEdge, XR_BSP4GLASS);
		m_lEdgeData.Clear();
	}

	~CBSP4Glass_BuildEdge()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_BuildEdge::~CBSP4Glass_BuildEdge, XR_BSP4GLASS);
		m_lEdgeData.Clear();
	}

	void PrepareBuild(const uint16& _nTris)
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_BuildEdge::PrepareBuild, XR_BSP4GLASS);
		m_lEdgeData.SetLen(_nTris * 3);
		m_pEdgeData = m_lEdgeData.GetBasePtr();
		m_nEdges = 0;
		m_nRealEdges = 0;
	}

	void AddEdge(const uint16& _i1, const uint16& _i2)
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_BuildEdge::AddEdge, XR_BSP4GLASS);
		uint16 i = 0;
		for(; i < m_nEdges; i++)
		{
			if( m_pEdgeData[i].m_n < 2 &&
			   ((m_pEdgeData[i].m_i1 == _i1 && m_pEdgeData[i].m_i2 == _i2) ||
			   (m_pEdgeData[i].m_i1 == _i2 && m_pEdgeData[i].m_i2 == _i1)))
			{
				m_nRealEdges--;
				m_pEdgeData[i].m_n++;
				break;
			}
		}

		if(i == m_nEdges)
		{
			m_nRealEdges++;
			m_pEdgeData[m_nEdges].m_i1 = _i1;
			m_pEdgeData[m_nEdges].m_i2 = _i2;
			m_pEdgeData[m_nEdges++].m_n = 1;
		}
	}

	void OptimizeMemory()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_BuidleEdge::OptimizeMemory, XR_BSP4GLASS);
		m_lEdgeData.SetLen(m_nEdges);
		m_pEdgeData = m_lEdgeData.GetBasePtr();
	}

	SBSP4Glass_EdgeData* GetBasePtr()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_BuildEdge::GetBasePtr, XR_BSP4GLASS);
		return m_pEdgeData;
	}

	uint16 NumEdges()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_BuildEdge::NumEdges, XR_BSP4GLASS);
		return m_nEdges;
	}

	uint16 NumRealEdges()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_BuildEdge::NumRealEdges, XR_BSP4GLASS);
		return m_nRealEdges;
	}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_Edge
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CBSP4Glass_Edge
{
public:
	struct SBSP4Glass_EdgeData
	{
		int m_i1;
		int m_i2;
		int m_n;
	};

	TArray<SBSP4Glass_EdgeData> m_lEdgeData;

	CBSP4Glass_Edge()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_Edge::CBSP4Glass_Edge, XR_BSP4GLASS);
		m_lEdgeData.Clear();
	}

	~CBSP4Glass_Edge()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_Edge::~CBSP4Glass_Edge, XR_BSP4GLASS);
		m_lEdgeData.Clear();
	}

	void AddEdge(const int& _i1, const int& _i2)
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_Edge::AddEdge, XR_BSP4GLASS);

		const int& nEdgeData = m_lEdgeData.Len();
		SBSP4Glass_EdgeData* pEdgeData = m_lEdgeData.GetBasePtr();
		
		bool bAddEdge = true;
		for(int i = 0; i < nEdgeData; i++)
		{
			if((pEdgeData[i].m_i1 == _i1 && pEdgeData[i].m_i2 == _i2) ||
			   (pEdgeData[i].m_i1 == _i2 && pEdgeData[i].m_i2 == _i1))
			{
				pEdgeData[i].m_n++;
				bAddEdge = false;
				break;
			}
		}

		if(bAddEdge)
		{
			SBSP4Glass_EdgeData EdgeData;
			EdgeData.m_i1 = _i1;
			EdgeData.m_i2 = _i2;
			EdgeData.m_n = 1;

			m_lEdgeData.Add(EdgeData);
		}
	}

	int NumEdges()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_Edge::NumEdges, XR_BSP4GLASS);
		return m_lEdgeData.Len();
	}

	void RemoveSharedEdges()
	{
		GLASS_MSCOPE_ALL(CBSP4Glass_Edge::RemoveSharedEdges, XR_BSP4GLASS);

		const int nEdgeData = m_lEdgeData.Len();
		for(int i = nEdgeData-1; i >= 0; i--)
		{
			if(m_lEdgeData[i].m_n > 1)
				m_lEdgeData.Del(i);
		}
	}

	CBSP4Glass_Edge& operator = (const CBSP4Glass_Edge& _Edge)
	{
		m_lEdgeData = _Edge.m_lEdgeData;
		return *this;
	}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CGlassCreateInfo
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CGlassCreateInfo
{
public:
	CGlassCreateInfo()
	{
		GLASS_MSCOPE_ALL(CGlassCreateInfo::CGlassCreateInfo, XR_BSP4GLASS);
		m_lModels.Clear();
	}

	~CGlassCreateInfo()
	{
		GLASS_MSCOPE_ALL(CGlassCreateInfo::~CGlassCreateInfo, XR_BSP4GLASS);
		m_lModels.Clear();
	}

	struct SGlassCreateInfo
	{
		CXR_PlaneMapping*		m_pMapping;
		TThinArray<CVec3Dfp32>	m_lV;

		// Triangle indices for front, back and edges of glass
		TThinArray<uint16>		m_lFrontP;
		//TThinArray<uint16>	m_lBackP;
		TThinArray<uint16>		m_lEdgeP;

		uint32					m_nRenderMainPrim;//FrontBackPrim;
		uint32					m_nRenderEdgePrim;
		CPlane3Dfp32			m_Plane;
		fp32					m_TxtWidthInv;
		fp32					m_TxtHeightInv;
		fp32					m_Thickness;
		uint16					m_iSurface;
		CBSP4Glass_Edge			m_Edges;
		uint32					m_Durability;
		uint32					m_UserFlags;
		bool					m_bValid;

		SGlassCreateInfo& operator = (const SGlassCreateInfo& _Info)
		{
			m_pMapping = _Info.m_pMapping;
			m_lV = _Info.m_lV;
			m_lFrontP = _Info.m_lFrontP;
			//m_lBackP = _Info.m_lBackP;
			m_lEdgeP = _Info.m_lEdgeP;
			m_nRenderMainPrim = _Info.m_nRenderMainPrim;
			m_nRenderEdgePrim = _Info.m_nRenderEdgePrim;
			m_Plane = _Info.m_Plane;
			m_TxtWidthInv = _Info.m_TxtWidthInv;
			m_TxtHeightInv = _Info.m_TxtHeightInv;
			m_Thickness = _Info.m_Thickness;
			m_iSurface = _Info.m_iSurface;
			m_Edges = _Info.m_Edges;
			m_Durability = _Info.m_Durability;
			m_UserFlags = _Info.m_UserFlags;
			m_bValid = _Info.m_bValid;
			return *this;
		}
	};

	TArray<SGlassCreateInfo>	m_lModels;
};
typedef CGlassCreateInfo::SGlassCreateInfo CSGlassCreateInfo;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_BSP4Glass_Instance
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Model_BSP4Glass_Instance : public CXR_ModelInstance
{
public:
	class CBSP4Glass_Model;

	// A glass shard holding data
	class CBSP4Glass_ModelShard : public CReferenceCount
	{
	public:
		// Vertices and texture coordinates
		TThinArray<CVec3Dfp32>	m_lV;
		TThinArray<CVec2Dfp32>	m_lTV;

		// This data will stay the same
		CPixel32				m_lC[GLASS_NUM_COLORS];
		CVec3Dfp32				m_lN[GLASS_NUM_NORMALS];
		CVec3Dfp32				m_lTangU[GLASS_NUM_TANGENTS];
		CVec3Dfp32				m_lTangV[GLASS_NUM_TANGENTS];

		TThinArray<CVec4Dfp32>	m_lVelocity;
		TThinArray<CVec3Dfp32>	m_lSrcPos;

		int32					m_SpawnTick;
		uint16					m_nShards;
		uint32					m_iModel;

		int32					m_iSurface;

		CBox3Dfp32				m_BBox;

		CBSP4Glass_ModelShard()
		{
			GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass_Instance::CGlassShardModel::CGlassShardModel, XR_BSP4GLASS);
			ClearMemory();
		}
		~CBSP4Glass_ModelShard()
		{
			GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass_Instance::CGlassShardModel::~CGlassShardModel, XR_BSP4GLASS);
			ClearMemory();
		}

		void ClearMemory()
		{
			GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass_Instance::CGlassShardModel::ClearMemory, XR_BSP4GLASS);
			m_lV.Clear();
			m_lTV.Clear();
			m_lVelocity.Clear();
		}

		M_INLINE CVec3Dfp32 GetRandomPoint(uint32& _iRand, const CVec3Dfp32& _V0, const CVec3Dfp32& _V1, const CVec3Dfp32& _V2)
		{
			_iRand += 2;
			return _V0 + (((_V1 - _V0) * ((MFloat_GetRand(_iRand-2)-0.5f) * 2.0f)) + ((_V2 - _V0) * ((MFloat_GetRand(_iRand-1)-0.5f) * 2.0f)));
		}

		M_INLINE CVec3Dfp32 GetRandomPointHalf(uint32& _iRand, const CVec3Dfp32& _V0, const CVec3Dfp32& _V1, const CVec3Dfp32& _V2)
		{
			_iRand += 2;
			return _V0 + (((_V1 - _V0) * ((MFloat_GetRand(_iRand-2)) * 0.5f)) + ((_V2 - _V0) * ((MFloat_GetRand(_iRand-1)) * 0.5f)));
		}

		void UpdateVelocities(const int32 _iHitIndex, CBSP4Glass_Grid& _Grid, const int32 _nRefresh, const CVec3Dfp32& _Force);
		void UpdateMesh(int32 _iSurface, CGlassAttrib& _Attrib, CBSP4Glass_Grid& _Grid, CBSP4Glass_TangentSetup& _Tang, CBSP4Glass_MappingSetup& _Mapping, const CMat4Dfp32& _WMat);
		void RefreshMesh(const int32 _GameTick, CBSP4Glass_Grid& _Grid, const CMat4Dfp32& _WMat);

		void GetBound_Box(CBox3Dfp32& _BBox, const CMat4Dfp32* _pMat = NULL);
		void Render(CBSP4Glass_RenderParam* _pRenderParam, CXW_Surface* _pSurface, const CMat4Dfp32& _WMat, CXW_Surface* _pSurfaceCrush);

		// Debugging
		void Debug_Render(CWireContainer* _pWC);

	private:
		void AllocMeshData(const uint32 _nShards, const uint32 _nV);
		void RenderMask(CBSP4Glass_RenderParam* _pRenderParam, CXW_SurfaceKeyFrame* _pSurfKey, const fp32& _Priority, CXW_Surface* _pSurfaceCrush);
		void RenderShading(CBSP4Glass_RenderParam* _pRenderParam, const CXR_Light& _Light, const CXR_ShaderParams& _ShaderParams, const CXR_SurfaceShaderParams* _pSurfParams);

		void VB_CreateChain(CXR_RenderInfo& _RenderInfo, CBSP4Glass_Model* _pModel, CXR_VBManager* _pVBM, CBSP4Glass_RenderParam* _pRenderParam);
	};

	class CBSP4Glass_Model
	{
	public:
		TThinArray<CVec3Dfp32>	m_lV;		// Vertices for solid glass (glass still sitting in frame)
		TThinArray<CVec2Dfp32>	m_lTV;		// Texture coordinates
		TThinArray<CPixel32>	m_lC;		// Vertex colors, this could be redone
		TThinArray<CVec3Dfp32>	m_lN;		// All normals for solids and shards
		TThinArray<CVec3Dfp32>	m_lTangU;	// U tangent for all vertices
		TThinArray<CVec3Dfp32>	m_lTangV;	// V tangent for all vertices
		
		TThinArray<uint16>		m_lFrontP;	// Front render primitive
		TThinArray<uint32>		m_lPhysFrontP;
		TThinArray<uint16>		m_lEdgeP;	// Edge render primitive

		uint32					m_nRenderMainPrim;		// No. of primitives in front and back list
		uint32					m_nRenderEdgePrim;		// No. of primitives in edge list

		uint16					m_iSurface;

		CXR_PlaneMapping*		m_pMapping;
		
		CGlassAttrib			m_Attrib;

		CBSP4Glass_Grid			m_Grid;

		CVec3Dfp32				m_Center;

		TPtr<CBSP4Glass_ModelShard> m_spShardModel;
		#if GLASS_DEBUG
			uint8				m_Debug_nRendered;
		#endif

		CBSP4Glass_Model()
		{
			Clear();
		}

		~CBSP4Glass_Model()
		{
			Clear();
		}

		void Clear()
		{
			m_spShardModel = NULL;
		}
	};


	// The actuall glass models
	TThinArray<CBSP4Glass_Model>		m_lModels;
	TArray<CBSP4Glass_ModelShard*>		m_lpShardModels;
	TPtr<CBSP4Glass_WallmarkContext>	m_spWMC;

	// Skinning data (same through whole system, never changes)
	static uint16		m_lP[GLASS_NUM_INDICES];
	static uint32		m_lMI[GLASS_NUM_MATRIXINDICES];
	static fp32			m_lMW[GLASS_NUM_MATRIXWEIGHTS];
	static CMat4Dfp32	m_lRandRotMat[GLASS_ROT_MAT_MAX];
	static bool			m_bStaticInit;

	#ifndef M_RTM
		CRC_Font*		m_pDebug_Font;
	#endif


	CXR_Model_BSP4Glass_Instance(const CGlassCreateInfo& _GlassInfo);
	CXR_Model_BSP4Glass_Instance();
	~CXR_Model_BSP4Glass_Instance();

	virtual void Create(CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context);
	virtual void CreateVertexBufferData(const CGlassCreateInfo& _GlassInfo);
	
	virtual void OnRefresh(const CXR_ModelInstanceContext& _Context, const CMat4Dfp32* _pMat = NULL, int _nMat = 0, int _Flags = 0);
	virtual bool NeedRefresh(CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context);
	
	virtual TPtr<CXR_ModelInstance> Duplicate() const;
	virtual void operator = (const CXR_ModelInstance& _Instance);
	virtual void operator = (const CXR_Model_BSP4Glass_Instance& _Instance);

	virtual bool Phys_LineIntersect(const int32 _iInstance, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, const CVec3Dfp32* _pHitPos, const int32 _Tick, const CMat4Dfp32* _pWMat);
	virtual bool Phys_SphereIntersect(const int32 _iInstance, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, CCollisionInfo* _pCollisionInfo, const CMat4Dfp32& _WMat);
	virtual bool Phys_BoxIntersect(const CXR_PhysicsContext* _pPhysContext, const int32 _iInstance, const CPlane3Dfp32& _PolyPlane, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, CCollisionInfo* _pCollisionInfo);

	#ifndef M_RTM
		virtual void Phys_LineIntersect_RenderHit(const CMat4Dfp32* _pWMat, const CVec3Dfp32& _V0, const CVec3Dfp32& _V1, const CVec3Dfp32& _V2, const CVec3Dfp32* _pHitPos);
		virtual void Debug_RenderWire(CWireContainer* _pWC, CPixel32 _Color, const CMat4Dfp32& _WMat);
	#endif

	// Server side functions
	virtual uint8 Server_OnDamage(const int32& _iInstance, const uint32& _Damage);
	virtual bool  Server_CrushGlassSurface(uint8 _CrushType, int32 _iInstance, const CVec3Dfp32& _LocalPosition, fp32 _Radius, const CVec3Dfp32& _BoxSize, int32 _GameTick, const CMat4Dfp32& _WMat, uint8 _CrushFlags);
	virtual void  Server_Restore(const uint16 _iInstance);
	virtual void  Server_RestoreAll();
	virtual void  Server_Inactive(const uint16 _iInstance, bool _bInactive);

	// Client side functions
	virtual int8 Client_CrushGlassSurface(uint8 _CrushType, const CMat4Dfp32& _WMat, int32 _GameTick, int32 _iInstance, int32 _Seed, const CVec3Dfp32& _LocalPosition, const CVec3Dfp32& _Force, fp32 _ForceScale, fp32 _Radius, const CVec3Dfp32& _BoxSize, uint8 _CrushFlags);
	virtual void Client_Restore(const uint16 _iInstance);
	virtual void Client_RestoreAll();
	virtual void Client_Inactive(const uint16 _iInstance, bool _bInactive);
	virtual bool Wallmark_Create(CXR_Model_BSP4Glass* _pModelGlass, const CMat4Dfp32& _GlassMat, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _Material);

	// Load save functions
	virtual void OnDeltaLoad(CCFile* _pFile);
	virtual void OnDeltaSave(CCFile* _pFile);

	TPtr<CBSP4Glass_ModelShard> AllocShardModel(CBSP4Glass_Model* _pModel, const int32 _iInstance, const int32 _Seed, const CMat4Dfp32& _WMat);
	void RemoveShardModel(CBSP4Glass_Model* _pModel);
	
	static CMat4Dfp32 GetRandRotMat(const uint32 _i)
	{
		return m_lRandRotMat[_i & GLASS_ROT_MAT_AND];
	}

	int32 GetSurface(uint16 _iInstance);
	virtual CVec3Dfp32 GetLocalPosFrom01(uint16 _iInstance, fp32 _x01, fp32 _y01);
	virtual bool GetAttrib_NoPhys(uint16 _iInstance);
};

typedef CXR_Model_BSP4Glass_Instance::CBSP4Glass_Model		CGlassModel;
typedef CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard	CGlassModelShard;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_BSP4Glass
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Model_BSP4Glass
	: public CXR_Model_BSP4
{
public:
	DECLARE_OPERATOR_NEW

	MRTC_DECLARE;

	TThinArray<CXR_PlaneMapping>	m_lMappings;
	TThinArray<uint16>				m_liFaceMapping;
	TThinArray<CBSP4Glass_Index>	m_lGlassIndices;
	TThinArray<uint16>				m_liFaceInstance;

	const CXR_PlaneMapping*			m_pMappings;
	const uint16*					m_piFaceMapping;
	const uint16*					m_piFaceInstance;
	const CBSP4Glass_Index*			m_pGlassIndices;

	TPtr<CBSP4Glass_LinkContext>	m_spLinkContext;
	TPtr<CXR_Model_BSP2>			m_spMaster;

protected:
	virtual void InitializeListPtrs();

	// Z-Buffer attributes
	//static CRC_Attributes ms_UnifiedZBuffer;

	// -------------------------------------------------------------------
	// Debugging
#if GLASS_DEBUG
	void	ClearRenderCalls(const CXR_AnimState* _pAnimState, const uint16* _piRet = NULL, const int& _nVisible = 0);
#endif

	// -------------------------------------------------------------------
	// Post create
	void	PostCreate();
	
	// -------------------------------------------------------------------
	// Private rendering calls
	CXR_VBChain* VB_CreateChain(CGlassModel& _GlassInfo, CXR_VBManager* _pVBM, const CMat4Dfp32& _M2W, const CMat4Dfp32& _W2V, const bool _bEdge, const bool _bFrontSide);
	//void OnRender_IndexZBuffer(const CSGlassInfo& _GlassInfo, CXR_VBManager* _pVBM, const CXR_Engine* _pEngine, const CMat4Dfp32& _M2V, const CXW_SurfaceLayer* _pLayers, const bool _bEdge);
	
	virtual void OnRender_Index(const CBSP4Glass_Index& _Index, CBSP4Glass_RenderParam* _pRenderParam, const uint16& _iID);
	virtual void OnRender_IndexShading(CGlassModel& _GlassInfo, CBSP4Glass_RenderParam* _pRenderParam, const CXR_ShaderParams& _ShaderParams, const CXR_SurfaceShaderParams* _pSurfParams, const CXR_Light& _Light, const bool _bEdge, const bool _bFrontSide);
	virtual void OnRender_IndexMask(CGlassModel& _GlassInfo, CBSP4Glass_RenderParam* _pRenderParam, CXW_SurfaceKeyFrame* _pSurfKey, const fp32& _Priority, const bool _bEdge, const bool _bFrontSide);

public:
	CXR_Model_BSP4Glass();
	~CXR_Model_BSP4Glass();


	// -------------------------------------------------------------------
	// Creation
	virtual void Create(const char* _pParam, CDataFile* _pDFile, CCFile* _pFile, const CBSP_CreateInfo& _CreateInfo);
	TPtr<CXR_ModelInstance> CreateModelInstance();
	void GetGlassCreateInfo(CGlassCreateInfo& _GlassInfo);
	virtual void CreateLinkContext(const CMat4Dfp32& _WMat);


	// -------------------------------------------------------------------
	virtual uint16 GetInstanceFromHash(uint32 _NameHash);


	// -------------------------------------------------------------------
	// Render precache
	virtual void PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip,
			const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0);
	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
			const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags);
	virtual void OnPrecache(CXR_Engine* _pEngine, int _iVariation);
	void PrecacheSurface(CXR_Engine* _pEngine, CXR_SurfaceContext* _pSurfCtx, const char* _pSurfaceName);


	// -------------------------------------------------------------------
	// Physics overrides
	virtual CXR_PhysicsModel* Phys_GetInterface() { return this; };

	// Full glass model bounding
	virtual void  Phys_GetBound_Sphere(const CMat4Dfp32& _Pos, CVec3Dfp32& _v0, fp32& _Radius);
	virtual void  Phys_GetBound_Box(const CMat4Dfp32& _Pos, CBox3Dfp32& _Box);

	// Per glass instance bounding
	virtual void  Phys_GetBound_BoxInstance(const uint32& _iInstance, const CMat4Dfp32& _Pos, CBox3Dfp32& _Box);

	virtual void  Phys_Init(CXR_PhysicsContext* _pPhysContext);
	virtual bool  Phys_IntersectLine(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);
	virtual bool  Phys_IntersectSphere(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);
	virtual bool  Phys_IntersectBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _BoxOrigin, const CPhysOBB& _BoxDest, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);

	virtual void  CollectPCS(CXR_PhysicsContext* _pPhysContext, const uint8 _IN1N2Flags, CPotColSet *_pcs, const int _iObj, const int _MediumFlags );

	virtual int   Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0);
	virtual void  Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, CXR_MediumDesc& _RetMedium);
	virtual int   Phys_GetCombinedMediumFlags(CXR_PhysicsContext* _pPhysContext, const CBox3Dfp32& _Box);

	void __PhysGlass_RenderFace(int _iFace, const CMat4Dfp32& _WMat, CWireContainer* _pWC, int _Col) const;
	bool __PhysGlass_TraceLine_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MediumFlags, CPhysLineContext* _pLineContext, CCollisionInfo* _pCollisionInfo) const;
	bool __PhysGlass_TraceLine_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MediumFlags, CPhysLineContext* _pLineContext) const;

	bool __PhysGlass_IntersectSphere_CheckFace(uint _iFace, bool _bTrueFace, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, CCollisionInfo* _pCollisionInfo) const;
	bool __PhysGlass_IntersectSphere_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo) const;
	bool __PhysGlass_IntersectSphere(CXR_PhysicsContext* _pPhysContext, uint _iFace, int32 _iInstance, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, CCollisionInfo* _pCollisionInfo) const;

	bool __PhysGlass_IntersectOBB_i(CXR_PhysicsContext* _pPhysContext, int _iNode, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, const CBox3Dfp32& _Bound, int _MediumFlags, CCollisionInfo* _pCollisionInfo) const;
	bool __PhysGlass_Intersect_PolyOBB(const bool& _bTrueFace, const fp32& _Thickness, const CVec3Dfp32* _pVertices, const uint32* _pVertIndices, const int _nVertexCount, const CPlane3Dfp32& _PolyPlane, const CPhysOBB& _BoxStart, const CPhysOBB& _BoxDest, bool _bOrder, CCollisionInfo* _pCollisionInfo = NULL);

	void __Glass_CollectPCS_r(CXR_PhysicsContext* _pPhysContext, CBSP4_EnumContext* _pEnumContext, int _iNode) const;
	void __Glass_CollectPCS_i(CXR_PhysicsContext* _pPhysContext, CBSP4_EnumContext* _pEnumContext, int _iNode) const;
	void __Glass_CollectPCS_IsLeaf(CXR_PhysicsContext* _pPhysContext, CBSP4_EnumContext* _pEnumContext, const CBSP4_Node* _pNode) const;

	static void CalcBoxScissor(CXR_VBManager* _pVBM, const CMat4Dfp32& _VMat, const CBox3Dfp32& _Box, CRect2Duint16& _Scissor);

	bool Wallmark_Create(CXR_Model_BSP4Glass_Instance* _pGlass, const CMat4Dfp32& _GlassMat, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _Material);
	//bool Wallmark_Create(CXR_AnimState* _pAnimState, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _Material);

private:
	void ReadFaceMapping(const int& _nFaces, CCFile* _pFile, int _Version);

	void CreateInstance_Clear(CSGlassCreateInfo& _CreateInfo);
	void CreateInstance_GetPrimCount(CSGlassCreateInfo& _CreateInfo, const CBSP4_Face& _Face, const bool& _bAlloc = false);
	void CreateInstance_CollectFaceData(CSGlassCreateInfo& _CreateInfo, const CBSP4_Face& _Face);
	void CreateInstance_CollectRenderPrim(CSGlassCreateInfo& _CreateInfo, const CBSP4_Face& _Face);
	void CreateInstance_CollectSurfaceData(CSGlassCreateInfo& _CreateInfo, const int& _iSurface);

	bool Wallmark_CreateOnModel(CBSP4Glass_WallmarkContext* _pWMC, const CMat4Dfp32& _GlassMat, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, CGlassModel& _GlassModel);
	bool Wallmark_Destroy(CXR_AnimState* _pAnimState, const int _GUID);

	uint8* Wallmark_AllocVBMem(CBSP4Glass_RenderParam* _pRenderParam, CBSP4Glass_WallmarkContext* _pWMC, const uint16* _piWM, const uint32 _nWM);
	void   Wallmark_Render(CBSP4Glass_RenderParam* _pRenderParams, CBSP4Glass_WallmarkContext* _pWMC, uint16* _piID, uint32 _nID);
	uint8* Wallmark_RenderWM(uint8* _pVBMem, CBSP4Glass_RenderParam* _pRenderParam, CBSP4Glass_WallmarkContext* _pWMC, const CBSP4Glass_Wallmark* _pWM, const CMTime& _Time);

	//int GlassCutFence(CVec3Dfp32* _pVerts, int _nv, const CPlane3Dfp32* _pPlanes, int _np, bool _bInvertPlanes, CVec2Dfp32* _pTVerts1 = NULL, CVec2Dfp32* _pTVerts2 = NULL, CVec2Dfp32* _pTVerts3 = NULL);
public:
	M_INLINE CXR_Model_BSP2* GetMaster()
	{
		return m_spMaster;
	}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| typedefs
|__________________________________________________________________________________________________
\*************************************************************************************************/
typedef CBSP4Glass_BuildEdge::SBSP4Glass_EdgeData	CBSP4Glass_BuildEdgeData;
typedef CBSP4Glass_Edge::SBSP4Glass_EdgeData CSBSP4Glass_Edge;
typedef TPtr<CXR_Model_BSP4Glass> spCXR_Model_BSP4Glass;


#endif

