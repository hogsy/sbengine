#ifndef CSurfaceKey_h
#define CSurfaceKey_h

//----------------------------------------------------------------------

//----------------------------------------------------------------------
// CSurfaceKey
//----------------------------------------------------------------------

class CSurfaceKey
{
public:
	class CXW_Surface*			m_pSurface;
	class CXW_SurfaceKeyFrame*	m_pKeyFrame;
	
	int32					m_ColorRGB;
	int32					m_ColorAlpha;
	CMTime					m_SurfaceTime;
	
public:
	
	CSurfaceKey()
	{
	}
	
	void Create(const int& _Seq, class CXR_SurfaceContext *_pSurfaceContext, class CXR_Engine *_pEngine, const CMTime& _Time, const int& _iSurface);
	void Create(class CXR_SurfaceContext *_pSurfaceContext, class CXR_Engine *_pEngine, const class CXR_AnimState* _pAnimState, int _iSurface);
	bool Render(class CXR_VertexBuffer* _pVB, class CXR_VBManager* _pVBM, class CXR_Engine *_pEngine, CXR_RenderSurfExtParam* _pParams = NULL,int _ExtraFlags = 0);
	
	int32 GetFlags()
	{
		return m_pSurface->m_Flags;
	}
	
	/*
	uint8 GetRasterMode()
	{
	for (int i = 0; i < m_pKeyFrame->m_lTextures.Len(); i++)
				if (m_pKeyFrame->m_lTextures[i].m_RasterMode & )
				}
	*/
	
};

//----------------------------------------------------------------------

#endif /* CSurfaceKey_h */
