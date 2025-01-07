#include "PCH.h"

#include "WModel_CrossHair.h"

#include "MFloat.h"

// -------------------------------------------------------------------
//  CXR_Model_CrossHair
// -------------------------------------------------------------------
void CXR_Model_CrossHair::Create(const char* _pParam)
{
	MAUTOSTRIP(CXR_Model_CrossHair_Create, MAUTOSTRIP_VOID);
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
		
		//			ConOutL(CStrF("(CXR_Model_CrossHair::Create) Params %s, iDefSurface %d, DefSize %f", _pParam, m_iDefSurface, m_DefSize));
	}
}

fp32 CXR_Model_CrossHair::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_CrossHair_GetBound_Sphere, 0.0f);
//	return (_pAnimState) ? _pAnimState->m_Anim0 * _SQRT2 : (m_DefSize * _SQRT2);
	return (_pAnimState) ? _pAnimState->m_Anim0 * _SQRT2 / 2 : (m_DefSize * _SQRT2 / 2);
}

void CXR_Model_CrossHair::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_CrossHair_GetBound_Box, MAUTOSTRIP_VOID);
	fp32 Size = (_pAnimState) ? _pAnimState->m_Anim0 : m_DefSize;
	if(Size == 0)
		Size = m_DefSize;
	Size *= 0.5f;
	_Box.m_Min = -Size;
	_Box.m_Max = Size;
}

void CXR_Model_CrossHair::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat43fp32& _WMat, const CMat43fp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_CrossHair_OnRender, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_CrossHair::OnRender);
	// Anim0 = Size * 1000
	// Anim1 = SizeExpand per second * 1000
	// AnimAttr0 = Start angle
	// AnimAttr1 = Rotation angle per second
	// Colors[0] = Color  (intensity)
	// m_lpSurfaces[0] = Surface
	
	if (!_pRender) return;
	if (!_pAnimState) return;
	
	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	if (!pSC) Error("Create", "No surface-context available.");

	CXW_Surface* pSurf = _pAnimState->m_lpSurfaces[0];
	if (!pSurf)
	{
		pSurf = pSC->GetSurface(m_iDefSurface);
		if (!pSurf)
		{
			if (!pSurf)
				ConOut("(CXR_Model_CrossHair::Render) No Surface.");
			return;
		}
	}
	
	CXR_RenderInfo RenderInfo;
	if (_pViewClip)
	{
		// Get bound-sphere, get the CRC_ClipView
		if (!_pViewClip->View_GetClip_Sphere(CVec3Dfp32::GetMatrixRow(_WMat, 3), GetBound_Sphere(_pAnimState), 0, 0, NULL, &RenderInfo))
			return;
	}
	
	CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
	if (!pVB) return;
	
	fp32 Size = fp32(_pAnimState->m_Anim0) * ( 2.0f / 1000.0f );
	if (!Size) Size = m_DefSize;
	Size += fp32(_pAnimState->m_Anim1) * _pAnimState->m_AnimTime0 / 1000.0f;
	
	CXR_Particle2 Sprite;
	Sprite.m_Pos = CVec3Dfp32::GetMatrixRow(_WMat, 3);
	Sprite.m_Size = Size;
	Sprite.m_Color = _pAnimState->m_Colors[0];
	Sprite.m_Angle = _pAnimState->m_AnimAttr0 + _pAnimState->m_AnimAttr1 * _pAnimState->m_AnimTime0;
	
	//	static bool Render_Particles(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat4Dfp32 &_Mat, const CXR_Particle2* _pParticles, int _nParticles, const CMat4Dfp32* _pAlign = NULL, int _PrimType = CXR_PARTICLETYPE_AUTO);
	
	if (CXR_Util::Render_Particles(_pVBM, pVB, _VMat, &Sprite, 1, NULL, CXR_PARTICLETYPE_QUAD | CXR_PARTICLETYPE_ANGLE))
	{
		pVB->m_Priority = RenderInfo.m_BasePriority_Transparent;
		
		CXW_SurfaceKeyFrame* pSurfKey;
		pSurfKey = pSurf->GetFrame(
			pSurf->m_iController ? _pAnimState->m_Anim1 : _pAnimState->m_Anim0, 
			pSurf->m_iController ? _pAnimState->m_AnimTime1 : _pAnimState->m_AnimTime0, *pSC->m_spTempKeyFrame);

		
		int Flags = RENDERSURFACE_VERTEXFOG;
		CXR_Util::Render_Surface(Flags, pSurf, pSurfKey, _pEngine, _pVBM, NULL, NULL, (CMat43fp32*)NULL, pVB, 
			RenderInfo.m_BasePriority_Transparent, 0.0001f, 0);
	}
}

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_CrossHair, CXR_Model);
