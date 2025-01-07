#include "PCH.h"
#include "CSurfaceKey.h"

void CSurfaceKey::Create(const int& _Seq, CXR_SurfaceContext *_pSurfaceContext, CXR_Engine *_pEngine, const CMTime& _Time, const int& _iSurface)
{
	m_SurfaceTime = _Time;
	m_pSurface = _pSurfaceContext->GetSurfaceVersion(_iSurface, _pEngine);
	m_pKeyFrame = m_pSurface->GetFrame(_Seq, _Time);
	m_ColorRGB = m_pKeyFrame->m_Medium.m_Color & 0x00FFFFFF;
	m_ColorAlpha = ((m_pKeyFrame->m_Medium.m_Color >> 24) & 0xFF);
}

void CSurfaceKey::Create(CXR_SurfaceContext *_pSurfaceContext, CXR_Engine *_pEngine, const CXR_AnimState* _pAnimState, int _iSurface)
{
	m_SurfaceTime = _pAnimState->m_AnimTime0;
	m_pSurface = _pSurfaceContext->GetSurfaceVersion(_iSurface, _pEngine);
	m_pKeyFrame = m_pSurface->GetFrame(0, _pAnimState->m_AnimTime0);
	m_ColorRGB = m_pKeyFrame->m_Medium.m_Color & 0x00FFFFFF;
	m_ColorAlpha = ((m_pKeyFrame->m_Medium.m_Color >> 24) & 0xFF);
}

bool CSurfaceKey::Render(CXR_VertexBuffer* _pVB, CXR_VBManager* _pVBM, CXR_Engine *_pEngine, CXR_RenderSurfExtParam* _pParams,int _ExtraFlags /* = 0 */)
{
	int flags;
	
	flags = RENDERSURFACE_DEPTHFOG | _ExtraFlags;// | RENDERSURFACE_VERTEXFOG;
	if (_pVB->m_pTransform)
		flags |= RENDERSURFACE_MATRIXSTATIC_M2V;
	
	return CXR_Util::Render_Surface(flags, m_SurfaceTime, m_pSurface, m_pKeyFrame, _pEngine, _pVBM, 
									(CMat4Dfp32*) NULL, (CMat4Dfp32*) NULL, _pVB->m_pTransform, 
									_pVB, _pVB->m_Priority, 0.0001f, _pParams);
}

