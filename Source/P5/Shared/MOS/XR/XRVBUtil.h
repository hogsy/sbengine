#ifndef _INC_XRVBUtil
#define _INC_XRVBUtil

#include "XRVBManager.h"

// -------------------------------------------------------------------
class CXR_PreRenderData_RenderTarget_Clear
{
public:
	CRct m_ClearArea;
	int m_WhatToClear;
	CPixel32 m_ColorClearTo;
	fp32 m_ZClearTo;
	int m_StencilClearTo;
	
	static void ClearRenderTarget(CRenderContext* _pRC, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext, CXR_VBMScope* _pScope, int _Flags)
	{
		CXR_PreRenderData_RenderTarget_Clear *pData = (CXR_PreRenderData_RenderTarget_Clear *)_pContext;

		_pRC->RenderTarget_Clear(pData->m_ClearArea, pData->m_WhatToClear, pData->m_ColorClearTo, pData->m_ZClearTo, pData->m_StencilClearTo);
	}

	static void SetNextClearParams(CRenderContext* _pRC, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext, CXR_VBMScope* _pScope, int _Flags)
	{
		CXR_PreRenderData_RenderTarget_Clear *pData = (CXR_PreRenderData_RenderTarget_Clear *)_pContext;

		_pRC->RenderTarget_SetNextClearParams(pData->m_ClearArea, pData->m_WhatToClear, pData->m_ColorClearTo, pData->m_ZClearTo, pData->m_StencilClearTo);
	}
};


// -------------------------------------------------------------------
class CXR_PreRenderData_RenderTarget_Copy
{
public:
	CRct m_SrcRect;
	CPnt m_Dst;
	int m_CopyType;
	
	static void RenderTarget_Copy(CRenderContext* _pRC, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext, CXR_VBMScope* _pScope, int _Flags)
	{
		CXR_PreRenderData_RenderTarget_Copy*pData = (CXR_PreRenderData_RenderTarget_Copy*)_pContext;

		_pRC->RenderTarget_Copy(pData->m_SrcRect, pData->m_Dst, pData->m_CopyType);
	}
};

// -------------------------------------------------------------------
class CXR_PreRenderData_RenderTarget_CopyToTexture1
{
public:
	CRct m_SrcRect;
	CPnt m_Dst;
	bint m_bContinueTiling;
	uint16 m_TextureID;
	uint16 m_Slice;	// Which slice, 0 for 1D/2D, 0-5 for Cube, 0-n for 3D texture
					// 1D/2D:
					//  Slice0 - Only valid slice
					// Cube:
					//  Slice0 - Positive X
					//  Slice1 - Negative X
					//  Slice2 - Positive Y
					//  Slice3 - Negative Y
					//  Slice4 - Positive Z
					//  Slice5 - Negative Z

	uint32 m_iMRT;
	
	static void RenderTarget_CopyToTexture(CRenderContext* _pRC, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext, CXR_VBMScope* _pScope, int _Flags)
	{
		CXR_PreRenderData_RenderTarget_CopyToTexture1* pData = (CXR_PreRenderData_RenderTarget_CopyToTexture1*)_pContext;

		_pRC->RenderTarget_CopyToTexture(pData->m_TextureID, pData->m_SrcRect, pData->m_Dst, pData->m_bContinueTiling, pData->m_Slice, pData->m_iMRT);
	}
};

// -------------------------------------------------------------------
class CXR_PreRenderData_Histogram
{
public:
	uint32 m_QueryID;

	static void RenderHistogram_PreRender(CRenderContext* _pRC, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext, CXR_VBMScope* _pScope, int _Flags)
	{
		CXR_PreRenderData_Histogram *pData = (CXR_PreRenderData_Histogram *)_pContext;
		_pRC->OcclusionQuery_Begin(pData->m_QueryID);
	}

	static void RenderHistogram_PostRender(CRenderContext* _pRC, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext, CXR_VBMScope* _pScope, int _Flags)
	{
		_pRC->OcclusionQuery_End();
	}
};

class CXR_PreRenderData_HistogramFeedback
{
public:
	uint32 m_QueryID;
	uint32 m_nQueries;
	uint32* m_pnVisiblePixels;

	static void PreRender(CRenderContext* _pRC, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext, CXR_VBMScope* _pScope, int _Flags)
	{
		CXR_PreRenderData_HistogramFeedback*pData = (CXR_PreRenderData_HistogramFeedback*)_pContext;

		uint32 nQ = pData->m_nQueries;
		uint32 IDBase = pData->m_QueryID;
		uint32* pnVisible = pData->m_pnVisiblePixels;

		for(int i = 0; i < nQ; i++)
		{
			pnVisible[i] = _pRC->OcclusionQuery_GetVisiblePixelCount(IDBase + i);
		}
	}
};

// -------------------------------------------------------------------
#ifdef PLATFORM_XBOX1

class CXR_PreRenderData_FlushGraphicsPipeline
{
public:
	static void PreRender(CRenderContext* _pRC, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext, CXR_VBMScope* _pScope, int _Flags)
	{
		g_XRender.FlushGraphicsPipeline();
	}
};

#endif


#endif // _INC_XRVBUtil

