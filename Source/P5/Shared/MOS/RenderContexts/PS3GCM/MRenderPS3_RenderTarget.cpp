
#include "PCH.h"
#include "MRenderPS3_Context.h"
#include "MRenderPS3_RenderTarget.h"

namespace NRenderPS3GCM
{
void CContext_RenderTarget::Create()
{
}

void CRCPS3GCM::RenderTarget_Clear(CRct _ClearRect, int _WhatToClear, CPixel32 _Color, fp32 _ZBufferValue, int _StencilValue)
{
	MSCOPE(CRCPS3GCM::RenderTarget_Clear, RENDER_PS3);

	if(_ClearRect.p1.x == -1)
	{
		CDCPS3GCM::ms_This.ClearFrameBuffer(_WhatToClear, _Color);
		return;
	}

	// Set clear scissor
	if(_ClearRect.p1.x == -1)
	{
		// Entire thing..
		gcmSetScissor(0, 0, CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_Width, CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_Height);
	}
	else
		gcmSetScissor(_ClearRect.p0.x, _ClearRect.p0.y, _ClearRect.GetWidth(), _ClearRect.GetHeight());

	gcmSetClearDepthStencil((((uint32&)_ZBufferValue) & ~0xff) | (_StencilValue & 0xff));

	uint ClearFlags = 0;
	if (_WhatToClear & CDC_CLEAR_COLOR)
	{
		gcmSetClearColor(_Color);
		gcmSetColorMask(CELL_GCM_TRUE);
		ClearFlags |= CELL_GCM_CLEAR_RGBA;
	}
	if (_WhatToClear & CDC_CLEAR_ZBUFFER)
	{
		gcmSetDepthMask(CELL_GCM_TRUE);
		ClearFlags |= CELL_GCM_CLEAR_Z;
	}
	if (_WhatToClear & CDC_CLEAR_STENCIL)
	{
		ClearFlags |= CELL_GCM_CLEAR_S;
		gcmSetStencilMask(0xff);
	}

	gcmSetClearSurface(ClearFlags);

	// Restore renderstate
	{
		uint32 MinX = 0, MinY = 0, MaxX = 4095, MaxY = 4095;
		if(m_CurrentAttrib.m_Scissor.IsValid())
		{
			m_CurrentAttrib.m_Scissor.GetRect(MinX, MinY, MaxX, MaxY);
			MaxX &= 0xfff;
			MaxY &= 0xfff;

		}

		uint w = MaxX - MinX;
		uint h = MaxY - MinY;
		gcmSetScissor(MinX, MinY, w, h);
	}
	//JK-NOTE: Removed the if's and always write these, will probably be faster.
	if (_WhatToClear & CDC_CLEAR_STENCIL)
	{
		gcmSetStencilMask(m_CurrentAttrib.m_StencilWriteMask);
	}
	if (_WhatToClear & CDC_CLEAR_COLOR)
	{
		uint32 Mask = 0;
		if(m_CurrentAttrib.m_Flags & CRC_FLAGS_COLORWRITE)
			Mask |= CELL_GCM_COLOR_MASK_R | CELL_GCM_COLOR_MASK_G | CELL_GCM_COLOR_MASK_B;
		if(m_CurrentAttrib.m_Flags & CRC_FLAGS_ALPHAWRITE)
			Mask |= CELL_GCM_COLOR_MASK_A;
		gcmSetColorMask(Mask);
	}
	if (_WhatToClear & CDC_CLEAR_ZBUFFER)
	{
		if(m_CurrentAttrib.m_Flags & CRC_FLAGS_ZWRITE)
			gcmSetDepthMask(CELL_GCM_TRUE);
		else
			gcmSetDepthMask(CELL_GCM_FALSE);
	}
}

void CRCPS3GCM::RenderTarget_Copy(CRct _SrcRect, CPnt _Dest, int _CopyType)
{
	MSCOPE(CRCPS3GCM::RenderTarget_Copy, RENDER_PS3);
	M_BREAKPOINT;
}

void CRCPS3GCM::RenderTarget_CopyToTexture(int _TextureID, CRct _SrcRect, CPnt _Dest, bint _bContinueTiling, uint16 _Slice, int _iMRT)
{
	MSCOPE(CRCPS3GCM::RenderTarget_CopyToTexture, RENDER_PS3);
	m_ContextTexture.RenderTarget_CopyToTexture(_TextureID, _SrcRect, _Dest, _bContinueTiling, _Slice, _iMRT);
}

void CRCPS3GCM::RenderTarget_SetEnableMasks(uint32 _And)
{
	MSCOPE(CRCPS3GCM::RenderTarget_SetEnableMasks, RENDER_PS3);
	CRC_RenderTargetDesc RenderTarget;
	RenderTarget.Clear();
	uint iSrc = 0, iDst = 0;
	for(uint iSrc = 0; iSrc < CRC_MAXMRT; iSrc++)
	{
		if((1 << iSrc) & _And)
		{
			RenderTarget.SetRenderTarget(iDst, m_CurrentMRT.m_lColorTextureID[iSrc], m_CurrentMRT.m_lColorClearColor[iSrc], m_CurrentMRT.m_lTextureFormats[iSrc], m_CurrentMRT.m_lSlice[iSrc]);
			iDst++;
		}
	}
	if(iDst == 0)
	{
		M_BREAKPOINT;
	}
	RenderTarget.m_ClearRect = m_CurrentMRT.m_ClearRect;
	RenderTarget.m_ClearBuffers = m_CurrentMRT.m_ClearBuffers;
	RenderTarget.m_ClearValueStencil = m_CurrentMRT.m_ClearValueStencil;
	RenderTarget.m_Options = m_CurrentMRT.m_Options;
	RenderTarget.m_ClearValueZ = m_CurrentMRT.m_ClearValueZ;
	RenderTarget.m_ResolveRect = m_CurrentMRT.m_ResolveRect;
	CDCPS3GCM::ms_This.MRT_SetRenderTarget(RenderTarget);
}

void CRCPS3GCM::RenderTarget_SetNextClearParams(CRct _ClearRect, int _WhatToClear, CPixel32 _Color, fp32 _ZBufferValue, int _StecilValue)
{
	MSCOPE(CRCPS3GCM::RenderTarget_SetNextClearParams, RENDER_PS3);
}

void CRCPS3GCM::RenderTarget_SetRenderTarget(const CRC_RenderTargetDesc& _RenderTarget)
{
	MSCOPE(CRCPS3GCM::RenderTarget_SetRenderTarget, RENDER_PS3);
	m_CurrentMRT = _RenderTarget;
	CDCPS3GCM::ms_This.MRT_SetRenderTarget(_RenderTarget);
}

};
