#include "PCH.h"

#include "MDisplayPS3.h"
#include "MRenderPS3_Context.h"
#include "MRenderPS3_Def.h"
#include "MFloat.h"

#include "../../XR/XRVBContext.h"

// #define INTERNAL_GL_VERTEX

#define CRCGL_CONVERT_PRIMITIVES_TO_TRIANGLES

void CRenderContextPS3::Geometry_Color(CPixel32 _Col)
{
	if (_Col != CRenderContextPS3::ms_This.m_GeomColor)
	{
		m_VPConstRegisters[m_VP_iConstantColor] = CVec4Dfp4(fp4(_Col.GetR()) / 255.0f, fp4(_Col.GetG()) / 255.0f, fp4(_Col.GetB()) / 255.0f, fp4(_Col.GetA()) / 255.0f);
		m_bUpdateVPConst = true;
	}

	CRC_Core::Geometry_Color(_Col);
}

void CRenderContextPS3::Geometry_PrecacheFlush( )
{
	VB_DeleteAll();
}

void CRenderContextPS3::Geometry_PrecacheBegin( int _Count )
{
	ConOutL("(CRenderContextPS3::Geometry_PrecacheBegin)");
}

void CRenderContextPS3::Geometry_Precache(int _VBID)
{
	if (m_pVBCtx->VB_GetFlags(_VBID) & CXR_VBFLAGS_ALLOCATED)
	{
		CRC_VBIDInfo& IDInfo = m_lVBIDInfo[_VBID];
		if (!(IDInfo.m_Fresh & 1))
			VB_Create(_VBID);
	}
}


//----------------------------------------------------------------
void CRenderContextPS3::Internal_RenderPolygon(int _nV, const CVec3Dfp4* _pV, const CVec3Dfp4* _pN, const CVec4Dfp4* _pCol, 
											  const CVec4Dfp4* _pSpec, //const fp4* _pFog,
		const CVec4Dfp4* _pTV0, const CVec4Dfp4* _pTV1, const CVec4Dfp4* _pTV2, const CVec4Dfp4* _pTV3, int _Color)
{
	DebugBreak();
};

void CRenderContextPS3::Internal_IndxTriangles(uint16* _pIndices, int _nTriangles, const CRC_VertexBuffer& _VB, int _bAllUsed)
{
	Internal_VA_SetArrays(_VB);

	if(_bAllUsed)
	{
		glnDrawElements(GL_TRIANGLES, _nTriangles * 3, GL_UNSIGNED_SHORT, _pIndices);
		GLErr("Internal_IndxTriangles (glnDrawElements)");
	}
	else
	{
		uint16 MinVal, MaxVal;
		GetMinMax(_pIndices, _nTriangles * 3, MinVal, MaxVal);
		glDrawRangeElements(GL_TRIANGLES, MinVal, MaxVal, _nTriangles * 3, GL_UNSIGNED_SHORT, _pIndices);
		GLErr("Internal_IndxTriangles (glDrawRangeElements)");
	}
}

void CRenderContextPS3::Internal_IndxWires(uint16* _pIndices, int _Len, const CRC_VertexBuffer& _VB, int _bAllUsed)
{
	Internal_VA_SetArrays(_VB);
	if(_bAllUsed)
	{
		DebugNop();
		glnDrawElements(GL_LINES, _Len, GL_UNSIGNED_SHORT, _pIndices);
		GLErr("Internal_IndxWires (glnDrawElements)");
	}
	else
	{
		uint16 MinVal, MaxVal;
		GetMinMax(_pIndices, _Len, MinVal, MaxVal);
		glDrawRangeElements(GL_LINES, MinVal, MaxVal, _Len, GL_UNSIGNED_SHORT, _pIndices);
		GLErr("Internal_IndxWires (glDrawRangeElements)");
	}
}

void CRenderContextPS3::Internal_IndxPrimitives(uint16* _pPrimStream, int _StreamLen, const CRC_VertexBuffer& _VB, int _bAllUsed)
{
	if (!_pPrimStream) return;

	Internal_VA_SetArrays(_VB);

	uint16 lTriIndices[1024*3];
	CRCPrimStreamIterator StreamIterate(_pPrimStream, _StreamLen);

	while(StreamIterate.IsValid())
	{
		int nTriIndices = 1024*3;
		bool bDone = Geometry_BuildTriangleListFromPrimitives(StreamIterate, lTriIndices, nTriIndices);
		if (nTriIndices)
		{
			int nTri = nTriIndices/3;
			m_nVertices += nTriIndices;
			m_nTriangles += nTri;
			m_nTriTotal += nTri;
			Internal_VAIndxTriangles(lTriIndices, nTri);
		}

		if (bDone) break;
	}
}

//----------------------------------------------------------------
void CRenderContextPS3::Render_IndexedTriangles(uint16* _pIndices, int _nTriangles)
{
	MSCOPE(CRenderContextPS3::Render_IndexedTriangles, RENDER_GL);

	if (_nTriangles == 0xffff)
	{
		// Recursively call this function
		mint *pList = ((mint *)_pIndices);;
		int nLists = *pList;
		++pList;
		for (int i = 0; i < nLists; ++i)
		{
			int nTri = *pList;
			++pList;
			Render_IndexedTriangles(*((uint16 **)pList), nTri);
			++pList;
		}
		return;
	}


	if (m_GeomVBID)
	{
		Internal_VBIndxTriangles(m_GeomVBID, _pIndices, _nTriangles);
		m_nTriangles += _nTriangles;
		m_nTriTotal += _nTriangles;
		return;
	}

	if (!m_Geom.m_pV) return;
	if (m_AttribChanged) Attrib_Update();
	if (m_MatrixChanged) Matrix_Update();
	if (m_bUpdateVPConst) VP_Update();

	Internal_IndxTriangles(_pIndices, _nTriangles, m_Geom, m_bGeomArrayAllUsed);

	m_nTriangles += _nTriangles;
	m_nTriTotal += _nTriangles;
}

void CRenderContextPS3::Render_IndexedWires(uint16* _pIndices, int _Len)
{
	MSCOPE(CRenderContextPS3::Render_IndexedWires, RENDER_GL);

	if (m_AttribChanged) Attrib_Update();
	if (m_MatrixChanged) Matrix_Update();
	if (m_bUpdateVPConst) VP_Update();

	Internal_IndxWires(_pIndices, _Len, m_Geom, m_bGeomArrayAllUsed);

	m_nWires += _Len >> 1;
}

void CRenderContextPS3::Render_IndexedPrimitives(uint16* _pPrimStream, int _StreamLen)
{
	MSCOPE(CRenderContextPS3::Render_IndexedPrimitives, RENDER_GL);

	if (m_GeomVBID)
	{
		Internal_VBIndxPrimitives(m_GeomVBID, _pPrimStream, _StreamLen);
		return;
	}

	{
		if (m_AttribChanged) Attrib_Update();
		if (m_MatrixChanged) Matrix_Update();
		if (m_bUpdateVPConst) VP_Update();

		Internal_IndxPrimitives(_pPrimStream, _StreamLen, m_Geom, m_bGeomArrayAllUsed);
	}
}

//----------------------------------------------------------------
void CRenderContextPS3::Render_VertexBuffer(int _VBID)
{
	MSCOPE(CRenderContextPS3::Render_VertexBuffer, RENDER_GL);

	if (m_AttribChanged) Attrib_Update();
	if (m_MatrixChanged) Matrix_Update();
	if (m_bUpdateVPConst) VP_Update();

	{
		CRC_VBIDInfo& IDInfo = m_lVBIDInfo[_VBID];
		if (!(IDInfo.m_Fresh & 1))
			VB_Create(_VBID);
		
		if (!m_lspVB[_VBID])
		{
			ConOutL("(CRenderContextPS3::Render_VertexBuffer) Unable to create VB.");
			return;
		}
		
		CRCGL_VBInfo& VBI = *m_lspVB[_VBID];
		
		if (!VBI.IsEmpty())
		{
			VBO_RenderVB(_VBID);
		}
	}
}

//----------------------------------------------------------------
void CRenderContextPS3::Render_Wire(const CVec3Dfp4& _v0, const CVec3Dfp4& _v1, CPixel32 _Color)
{
	DebugBreak();
}

void CRenderContextPS3::Render_WireStrip(const CVec3Dfp4* _pV, const uint16* _piV, int _nVertices, CPixel32 _Color)
{
	DebugBreak();
}

void CRenderContextPS3::Render_WireLoop(const CVec3Dfp4* _pV, const uint16* _piV, int _nVertices, CPixel32 _Color)
{
	DebugBreak();
}


bool CRenderContextPS3::ReadDepthPixels(int _x, int _y, int _w, int _h, fp4* _pBuffer)
{
	DebugBreak();
	return false;
}

void CRenderContextPS3::RenderTarget_Clear(CRct _ClearRect, int _WhatToClear, CPixel32 _Color, fp4 _ZBufferValue, int _StecilValue)
{
	int Flags = m_CurrentAttrib.m_Flags;

	int Left = Min(_ClearRect.p0.x, _ClearRect.p1.x);
	int Right = Max(_ClearRect.p0.x, _ClearRect.p1.x);
	int Bottom = Min(_ClearRect.p0.y, _ClearRect.p1.y);
	int Top = Max(_ClearRect.p0.y, _ClearRect.p1.y);
	glScissor(Left, Bottom, Right - Left, Bottom - Top);
	glEnable(GL_SCISSOR_TEST);

	int ClearFlags = 0; 
	if (_WhatToClear & CDC_CLEAR_COLOR)
	{
		glClearColor(fp4(_Color.GetR()) / 255.0f, fp4(_Color.GetG()) / 255.0f, fp4(_Color.GetB()) / 255.0f, 0);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		ClearFlags |= GL_COLOR_BUFFER_BIT;
	}
	if (_WhatToClear & CDC_CLEAR_ZBUFFER)
	{
		ClearFlags |= GL_DEPTH_BUFFER_BIT;
		glClearDepthf(_ZBufferValue);
		glDepthMask(1);
	}
	if (_WhatToClear & CDC_CLEAR_STENCIL)
	{
		ClearFlags |= GL_STENCIL_BUFFER_BIT;
		glClearStencil(_StecilValue);
		glStencilMask(~0);
	}
	glClear(ClearFlags);

	{
		int w = CRenderContextPS3::ms_This.m_CurrentAttrib.m_Scissor.m_Max[0] - CRenderContextPS3::ms_This.m_CurrentAttrib.m_Scissor.m_Min[0];
		int h = CRenderContextPS3::ms_This.m_CurrentAttrib.m_Scissor.m_Max[1] - CRenderContextPS3::ms_This.m_CurrentAttrib.m_Scissor.m_Min[1];
		if (w < 0)
			w = 0;
		if (h < 0)
			h = 0;
		glScissor(CRenderContextPS3::ms_This.m_CurrentAttrib.m_Scissor.m_Min[0], CDisplayContextPS3::ms_This.m_CurrentBackbufferContext.m_Setup.m_Height - CRenderContextPS3::ms_This.m_CurrentAttrib.m_Scissor.m_Max[1], w, h);

		if (_WhatToClear & CDC_CLEAR_STENCIL)
			glStencilMask(CRenderContextPS3::ms_This.m_CurrentAttrib.m_StencilWriteMask);
		if (_WhatToClear & CDC_CLEAR_COLOR)
		{
			int ColorMask = (Flags & CRC_FLAGS_COLORWRITE) ? 1 : 0;
			int AlphaMask = (Flags & CRC_FLAGS_ALPHAWRITE) ? 1 : 0;
			glnColorMask(ColorMask, ColorMask, ColorMask, AlphaMask);
		}
		if((_WhatToClear & CDC_CLEAR_ZBUFFER) && !(Flags & CRC_FLAGS_ZWRITE))
			glDepthMask(0);
		if(!(Flags & CRC_FLAGS_SCISSOR))
			glDisable(GL_SCISSOR_TEST);
	}
}


void CRenderContextPS3::RenderTarget_Copy(CRct _SrcRect, CPnt _Dest, int _CopyType)
{
	DebugBreak();
}

void CRenderContextPS3::RenderTarget_CopyToTexture(int _TextureID, CRct _SrcRect, CPnt _Dest, bint _bContinueTiling, uint16 _Slice)
{
	CTC_TextureProperties Properties;
	m_pTC->GetTextureProperties(_TextureID, Properties);

	if (!(Properties.m_Flags & (CTC_TEXTUREFLAGS_PROCEDURAL | CTC_TEXTUREFLAGS_RENDER)))
		return;

	CRC_IDInfo &IDInfo = m_spTCIDInfo->m_pTCIDInfo[_TextureID];
	if (!(IDInfo.m_Fresh & 1))
	{
		// Unbind whatever texture is bound to texunit
		GL_SetTexture(0, 0);
		GL_BuildTexture(_TextureID, _TextureID, Properties);
		m_CurrentAttrib.m_TextureID[0] = _TextureID;
		// Unbind whatever texture got bount to texunit
		GL_SetTexture(0, 0);
//		m_CurrentAttrib.m_TextureID[0] = 0;
	}

	Attrib_Push();
	CRC_Attributes Attrib;
	Attrib.SetDefault();
	CRC_Core::Attrib_Set(Attrib);
	Attrib_Update();

	CImage Desc;
	int nMip;
	m_pTC->GetTextureDesc(_TextureID, &Desc, nMip);

	CRC_Viewport* pVP = Viewport_Get();
//	_SrcRect += pVP->GetViewArea().p0;

	int w = _SrcRect.GetWidth();
	int h = _SrcRect.GetHeight();
	int ww = CDisplayContextPS3::ms_This.m_CurrentBackbufferContext.m_Setup.m_Width;
	int wh = CDisplayContextPS3::ms_This.m_CurrentBackbufferContext.m_Setup.m_Height;
	Swap(_SrcRect.p0.y, _SrcRect.p1.y);
	_SrcRect.p0.y = wh - _SrcRect.p0.y;
	_SrcRect.p1.y = wh - _SrcRect.p1.y;
	_Dest.y = wh - h - _Dest.y + Desc.GetHeight()-wh;

	//JK-TODO: 3d texture support?
	int ImageTarget = GL_TEXTURE_2D;
	int TextureTarget = GL_TEXTURE_2D;
	if(Properties.m_Flags & (CTC_TEXTUREFLAGS_CUBEMAP | CTC_TEXTUREFLAGS_CUBEMAPCHAIN))
	{
		if(_Slice > 5)
		{
			LogFile("Cubemap textures only support slice 0-5");
			_Slice = 0;
		}
		TextureTarget = GL_TEXTURE_CUBE_MAP;
		ImageTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + _Slice;
	}
	glnBindTexture(TextureTarget, _TextureID);
//	glnCopyTexSubImage2D(ImageTarget, 0, _Dest.x, _Dest.y, _SrcRect.p0.x, _SrcRect.p0.y, w, h);
	glnCopyTexSubImage2D(ImageTarget, 0, 0, 0, _SrcRect.p0.x, _SrcRect.p0.y, w, h);
	glnBindTexture(TextureTarget, 0);

	Attrib_Pop();
}
