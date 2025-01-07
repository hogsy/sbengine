
#include "PCH.h"

#ifdef M_STATIC_RENDERER
void CRenderContext::Create(CObj* _pContext, const char* _pParams)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Create(_pContext, _pParams);
}

CDisplayContext* CRenderContext::GetDC()
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.GetDC();
}

const char* CRenderContext::GetRenderingStatus(int _iStatus)
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.GetRenderingStatus(_iStatus);
}

const char* CRenderContext::GetRenderingStatus()
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.GetRenderingStatus();
}

void CRenderContext::BeginScene(CRC_Viewport* _pVP)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.BeginScene(_pVP);
}

void CRenderContext::EndScene()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.EndScene();
}

CRC_Statistics CRenderContext::Statistics_Get()
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.Statistics_Get();
}

void CRenderContext::Flip_SetInterval(int _nFrames)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Flip_SetInterval(_nFrames);
}

// Performance query
uint CRenderContext::PerfQuery_GetStride()
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.PerfQuery_GetStride();
}

void CRenderContext::PerfQuery_Begin(int _MaxQueries, void* _pMem)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.PerfQuery_Begin(_MaxQueries, _pMem);
}

void CRenderContext::PerfQuery_Next()
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.PerfQuery_Next();
}

uint CRenderContext::PerfQuery_End()
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.PerfQuery_End();
}

// Viewport
void CRenderContext::Viewport_Update()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Viewport_Update();
}

CRC_Viewport* CRenderContext::Viewport_Get()
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.Viewport_Get();
}

void CRenderContext::Viewport_Set(CRC_Viewport* _pVP)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Viewport_Set(_pVP);
}

void CRenderContext::Viewport_Push()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Viewport_Push();
}

void CRenderContext::Viewport_Pop()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Viewport_Pop();
}

// MRT
void CRenderContext::RenderTarget_SetRenderTarget(const CRC_RenderTargetDesc& _RenderTarget)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.RenderTarget_SetRenderTarget(_RenderTarget);
}

void CRenderContext::RenderTarget_Clear(CRct _ClearRect, int _WhatToClear, CPixel32 _Color, fp32 _ZBufferValue, int _StecilValue)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.RenderTarget_Clear(_ClearRect, _WhatToClear, _Color, _ZBufferValue, _StecilValue);
}

void CRenderContext::RenderTarget_SetNextClearParams(CRct _ClearRect, int _WhatToClear, CPixel32 _Color, fp32 _ZBufferValue, int _StecilValue)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.RenderTarget_SetNextClearParams(_ClearRect, _WhatToClear, _Color, _ZBufferValue, _StecilValue);
}

void CRenderContext::RenderTarget_Copy(CRct _SrcRect, CPnt _Dest, int _CopyType)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.RenderTarget_Copy(_SrcRect, _Dest, _CopyType);
}

void CRenderContext::RenderTarget_CopyToTexture(int _TextureID, CRct _SrcRect, CPnt _Dest, bint _bContinueTiling, uint16 _Slice, int _iMRT /* = 0 */)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.RenderTarget_CopyToTexture(_TextureID, _SrcRect, _Dest, _bContinueTiling, _Slice, _iMRT);
}

void CRenderContext::RenderTarget_SetEnableMasks(uint32 _And)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.RenderTarget_SetEnableMasks(_And);
}

// Caps

int CRenderContext::Caps_Flags()
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.Caps_Flags();
}

int CRenderContext::Caps_TextureFormats()
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.Caps_TextureFormats();
}

int CRenderContext::Caps_DisplayFormats()
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.Caps_DisplayFormats();
}

int CRenderContext::Caps_StencilDepth()
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.Caps_StencilDepth();
}

// Render precache

void CRenderContext::Render_SetOnlyAllowExternalMemory(bint _bEnabled)
{
}
void CRenderContext::Render_SetMainMemoryOverride(bint _bEnabled)
{
}


void CRenderContext::Render_PrecacheFlush()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Render_PrecacheFlush();
}

void CRenderContext::Render_PrecacheEnd()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Render_PrecacheEnd();
}

void CRenderContext::Render_SetRenderOptions(uint32 _Options, uint32 _Format /* = 0xFFFFFFFF */)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Render_SetRenderOptions(_Options, _Format);
}

void CRenderContext::Render_EnableHardwareMemoryRegion(void *_pMemStart, mint _Size)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Render_EnableHardwareMemoryRegion(_pMemStart, _Size);
}

void CRenderContext::Render_EnableHardwareMemoryRegion(CXR_VBManager *_pManager, CXR_VBMContainer *_pContainer)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Render_EnableHardwareMemoryRegion(_pManager, _pContainer);
}

void CRenderContext::Render_DisableHardwareMemoryRegion()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Render_DisableHardwareMemoryRegion();
}

// Texture stuff

CTextureContext* CRenderContext::Texture_GetTC()
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.Texture_GetTC();
}

class CRC_TCIDInfo* CRenderContext::Texture_GetTCIDInfo()
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.Texture_GetTCIDInfo();
}

void CRenderContext::Texture_PrecacheFlush()
{
}

void CRenderContext::Texture_PrecacheBegin(int _Count)
{
}

void CRenderContext::Texture_PrecacheEnd()
{
}

void CRenderContext::Texture_Precache(int _TextureID)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Texture_Precache(_TextureID);
}

void CRenderContext::Texture_Copy(int _SourceTexID, int _DestTexID, CRct _SrcRgn, CPnt _DstPos)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Texture_Copy(_SourceTexID, _DestTexID, _SrcRgn, _DstPos);
}

CRC_TextureMemoryUsage CRenderContext::Texture_GetMem(int _TextureID)
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.Texture_GetMem(_TextureID);
}

int CRenderContext::Texture_GetPicmipFromGroup(int _iPicmip)
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.Texture_GetPicmipFromGroup(_iPicmip);
}

void CRenderContext::Texture_Flush(int _TextureID)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Texture_Flush(_TextureID);
}

int CRenderContext::Texture_GetBackBufferTextureID()
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.Texture_GetBackBufferTextureID();
}

int CRenderContext::Texture_GetFrontBufferTextureID()
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.Texture_GetFrontBufferTextureID();
}

int CRenderContext::Texture_GetZBufferTextureID()
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.Texture_GetZBufferTextureID();
}

void CRenderContext::Texture_BlockUntilStreamingTexturesDone()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Texture_BlockUntilStreamingTexturesDone();
}

// Vertexbuffer stuff

CXR_VBContext* CRenderContext::VB_GetVBContext()
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.VB_GetVBContext();
}

class CRC_VBIDInfo* CRenderContext::VB_GetVBIDInfo()
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.VB_GetVBIDInfo();
}

// Attrib stuff

void CRenderContext::Attrib_Push()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_Push();
}

void CRenderContext::Attrib_Pop()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_Pop();
}

void CRenderContext::Attrib_Set(const CRC_Attributes& _Attrib)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.CRC_Core::Attrib_Set(_Attrib);
}

void CRenderContext::Attrib_Get(CRC_Attributes& _Attrib)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_Get(_Attrib);
}

CRC_Attributes* CRenderContext::Attrib_Begin()
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_Begin();
}

void CRenderContext::Attrib_End(uint _ChgFlags)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_End(_ChgFlags);
}

void CRenderContext::Attrib_Lock(int _Flags)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_Lock(_Flags);
}

void CRenderContext::Attrib_LockFlags(int _Flags)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_LockFlags(_Flags);
}


void CRenderContext::Attrib_Enable(int _Flags)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_Enable(_Flags);
}
void CRenderContext::Attrib_Disable(int _Flags)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_Disable(_Flags);
}
void CRenderContext::Attrib_Switch(int _Flags)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_Switch(_Flags);
}
void CRenderContext::Attrib_ZCompare(int _Compare)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_ZCompare(_Compare);
}
void CRenderContext::Attrib_AlphaCompare(int _Compare, int _AlphaRef)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_AlphaCompare(_Compare, _AlphaRef);
}
void CRenderContext::Attrib_StencilRef(int _Ref, int _FuncAnd)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_StencilRef(_Ref, _FuncAnd);
}
void CRenderContext::Attrib_StencilWriteMask(int _Mask)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_StencilWriteMask(_Mask);
}
void CRenderContext::Attrib_StencilFrontOp(int _Func, int _OpFail, int _OpZFail, int _OpZPass)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_StencilFrontOp(_Func, _OpFail, _OpZFail, _OpZPass);
}
void CRenderContext::Attrib_StencilBackOp(int _Func, int _OpFail, int _OpZFail, int _OpZPass)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_StencilBackOp(_Func, _OpFail, _OpZFail, _OpZPass);
}
void CRenderContext::Attrib_RasterMode(int _Mode)			// RasterMode preceeds Blend enable, SrcBlend & DestBlend
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_RasterMode(_Mode);
}

void CRenderContext::Attrib_SourceBlend(int _Blend)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_SourceBlend(_Blend);
}
void CRenderContext::Attrib_DestBlend(int _Blend)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_DestBlend(_Blend);
}
void CRenderContext::Attrib_FogColor(CPixel32 _FogColor)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_FogColor(_FogColor);
}
void CRenderContext::Attrib_FogStart(fp32 _FogStart)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_FogStart(_FogStart);
}
void CRenderContext::Attrib_FogEnd(fp32 _FogEnd)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_FogEnd(_FogEnd);
}
void CRenderContext::Attrib_FogDensity(fp32 _FogDensity)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_FogDensity(_FogDensity);
}
void CRenderContext::Attrib_PolygonOffset(fp32 _Scale, fp32 _Offset)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_PolygonOffset(_Scale, _Offset);
}
//void CRenderContext::Attrib_Scissor(const CRect2Duint16& _Scissor)
void CRenderContext::Attrib_Scissor(const CScissorRect& _Scissor)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_Scissor(_Scissor);
}

	// Light state
void CRenderContext::Attrib_Lights(const CRC_Light* _pLights, int _nLights)	// _pLights must be valid as long as the light-state is in use.
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_Lights(_pLights, _nLights);
}

	// Texture state
void CRenderContext::Attrib_TextureID(int _iTxt, int _TextureID)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_TextureID(_iTxt, _TextureID);
}
void CRenderContext::Attrib_TexEnvMode(int _iTxt, int _TexEnvMode)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_TexEnvMode(_iTxt, _TexEnvMode);
}
//void CRenderContext::Attrib_TexEnvColor(int _iTxt, CPixel32 _TexEnvColor)

void CRenderContext::Attrib_TexGen(int _iTxt, int _TexGen, int _Comp)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_TexGen(_iTxt, _TexGen, _Comp);
}
void CRenderContext::Attrib_TexGenAttr(fp32* _pAttr)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_TexGenAttr(_pAttr);
}

	// Global attributes
void CRenderContext::Attrib_GlobalEnable(int _Flags)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_GlobalEnable(_Flags);
	if(_Flags & CRC_GLOBALFLAGS_WIRE)
	{
		gcmSetFrontPolygonMode(CELL_GCM_POLYGON_MODE_LINE);
		gcmSetBackPolygonMode(CELL_GCM_POLYGON_MODE_LINE);
	}
}
void CRenderContext::Attrib_GlobalDisable(int _Flags)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_GlobalDisable(_Flags);
	if(_Flags & CRC_GLOBALFLAGS_WIRE)
	{
		gcmSetFrontPolygonMode(CELL_GCM_POLYGON_MODE_FILL);
		gcmSetBackPolygonMode(CELL_GCM_POLYGON_MODE_FILL);
	}
}
void CRenderContext::Attrib_GlobalSwitch(int _Flags)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_GlobalSwitch(_Flags);
}
void CRenderContext::Attrib_GlobalSetVar(int _Var, int _Value)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_GlobalSetVar(_Var, _Value);
}
void CRenderContext::Attrib_GlobalSetVarfv(int _Var, const fp32* _pValues)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_GlobalSetVarfv(_Var, _pValues);
}
int CRenderContext::Attrib_GlobalGetVar(int _Var)
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_GlobalGetVar(_Var);
}
int CRenderContext::Attrib_GlobalGetVarfv(int _Var, fp32* _pValues)
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_GlobalGetVarfv(_Var, _pValues);
}

	// Transform
void CRenderContext::Matrix_SetMode(int _iMode, uint32 _ModeMask)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Matrix_SetMode(_iMode, _ModeMask);
}
void CRenderContext::Matrix_Push()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Matrix_Push();
}
void CRenderContext::Matrix_Pop()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Matrix_Pop();
}

void CRenderContext::Matrix_SetUnit()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Matrix_SetUnit();
}
void CRenderContext::Matrix_SetUnitInternal(uint _iMode, uint _ModeMask)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Matrix_SetUnitInternal(_iMode, _ModeMask);
}

void CRenderContext::Matrix_Set(const CMat4Dfp32& _Matrix)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Matrix_Set(_Matrix);
}
void CRenderContext::Matrix_SetInternal(const CMat4Dfp32& _Matrix, uint _iMode, uint _ModeMask)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Matrix_SetInternal(_Matrix, _iMode, _ModeMask);
}

void CRenderContext::Matrix_SetModelAndTexture(const CMat4Dfp32* _pModel, const CMat4Dfp32** _ppTextures)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Matrix_SetModelAndTexture(_pModel, _ppTextures);
}

void CRenderContext::Matrix_Get(CMat4Dfp32& _Matrix)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Matrix_Get(_Matrix);
}
void CRenderContext::Matrix_Multiply(const CMat4Dfp32& _Matrix)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Matrix_Multiply(_Matrix);
}
void CRenderContext::Matrix_MultiplyInverse(const CMat4Dfp32& _Matrix)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Matrix_MultiplyInverse(_Matrix);
}
void CRenderContext::Matrix_PushMultiply(const CMat4Dfp32& _Matrix)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Matrix_PushMultiply(_Matrix);
}
void CRenderContext::Matrix_PushMultiplyInverse(const CMat4Dfp32& _Matrix)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Matrix_PushMultiplyInverse(_Matrix);
}
void CRenderContext::Matrix_SetPalette(const CRC_MatrixPalette* _pMatrixPaletteArgs)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Matrix_SetPalette(_pMatrixPaletteArgs);
}

	// Clipping
void CRenderContext::Clip_Push()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Clip_Push();
}
void CRenderContext::Clip_Pop()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Clip_Pop();
}
void CRenderContext::Clip_Clear()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Clip_Clear();
}
void CRenderContext::Clip_Set(const CPlane3Dfp32* _pPlanes, int _nPlanes)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Clip_Set(_pPlanes, _nPlanes);
}
void CRenderContext::Clip_AddPlane(const CPlane3Dfp32& _Plane, const CMat4Dfp32* _pTransform, bool _bClipBack)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Clip_AddPlane(_Plane, _pTransform, _bClipBack);
}

	// Index-array rendering.
void CRenderContext::Geometry_VertexArray(const CVec3Dfp32* _pV, int _nVertices, int _bAllUsed)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Geometry_VertexArray(_pV, _nVertices, _bAllUsed);
}
void CRenderContext::Geometry_NormalArray(const CVec3Dfp32* _pN)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Geometry_NormalArray(_pN);
}
void CRenderContext::Geometry_TVertexArray(const fp32* _pTV, int _TxtChannel, int _nComp)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Geometry_TVertexArray(_pTV, _TxtChannel, _nComp);
}
void CRenderContext::Geometry_TVertexArray(const CVec2Dfp32* _pTV, int _TxtChannel)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Geometry_TVertexArray(_pTV, _TxtChannel);
}
void CRenderContext::Geometry_TVertexArray(const CVec3Dfp32* _pTV, int _TxtChannel)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Geometry_TVertexArray(_pTV, _TxtChannel);
}
void CRenderContext::Geometry_TVertexArray(const CVec4Dfp32* _pTV, int _TxtChannel)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Geometry_TVertexArray(_pTV, _TxtChannel);
}
void CRenderContext::Geometry_ColorArray(const CPixel32* _pCol)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Geometry_ColorArray(_pCol);
}
void CRenderContext::Geometry_SpecularArray(const CPixel32* _pCol)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Geometry_SpecularArray(_pCol);
}
void CRenderContext::Geometry_MatrixIndexArray(const uint32* _pMI)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Geometry_MatrixIndexArray(_pMI);
}
void CRenderContext::Geometry_MatrixWeightArray(const fp32* _pMW, int _nComp)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Geometry_MatrixWeightArray(_pMW, _nComp);
}
void CRenderContext::Geometry_Color(uint32 _Col)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Geometry_Color(_Col);
}

void CRenderContext::Geometry_VertexBuffer(const CRC_VertexBuffer& _VB, int _bAllUsed)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Geometry_VertexBuffer(_VB, _bAllUsed);
}
void CRenderContext::Geometry_VertexBuffer(int _VBID, int _bAllUsed)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Geometry_VertexBuffer(_VBID, _bAllUsed);
}

void CRenderContext::Geometry_Clear()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Geometry_Clear();
}

void CRenderContext::Geometry_PrecacheFlush( )
{
}
void CRenderContext::Geometry_PrecacheBegin( int _Count )
{
}
void CRenderContext::Geometry_Precache(int _VBID)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Geometry_Precache(_VBID);
}
void CRenderContext::Geometry_PrecacheEnd()
{
}

int CRenderContext::Geometry_GetVBSize(int _VBID)
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.Geometry_GetVBSize(_VBID);
}

void CRenderContext::Render_IndexedTriangles(uint16* _pTriVertIndices, int _nTriangles)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Render_IndexedTriangles(_pTriVertIndices, _nTriangles);
}
void CRenderContext::Render_IndexedTriangleStrip(uint16* _pIndices, int _Len)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Render_IndexedTriangleStrip(_pIndices, _Len);
}
void CRenderContext::Render_IndexedWires(uint16* _pIndices, int _Len)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Render_IndexedWires(_pIndices, _Len);
}
void CRenderContext::Render_IndexedPolygon(uint16* _pIndices, int _Len)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Render_IndexedPolygon(_pIndices, _Len);
}

void CRenderContext::Render_IndexedPrimitives(uint16* _pPrimStream, int _StreamLen)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Render_IndexedPrimitives(_pPrimStream, _StreamLen);
}

void CRenderContext::Render_VertexBuffer(int _VBID)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Render_VertexBuffer(_VBID);
}

void CRenderContext::Render_VertexBuffer_IndexBufferTriangles(uint _VBID, uint _IBID, uint _nTriangles, uint _PrimOffset)
{ 
	NRenderPS3GCM::CRCPS3GCM::ms_This.Render_VertexBuffer_IndexBufferTriangles(_VBID, _IBID, _nTriangles, _PrimOffset);
	// Todo...
}

	// Wire
void CRenderContext::Render_Wire(const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, CPixel32 _Color)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Render_Wire(_v0, _v1, _Color);
}
void CRenderContext::Render_WireStrip(const CVec3Dfp32* _pV, const uint16* _piV, int _nVertices, CPixel32 _Color)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Render_WireStrip(_pV, _piV, _nVertices, _Color);
}
void CRenderContext::Render_WireLoop(const CVec3Dfp32* _pV, const uint16* _piV, int _nVertices, CPixel32 _Color)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Render_WireLoop(_pV, _piV, _nVertices, _Color);
}

	// Occlusion query
void CRenderContext::OcclusionQuery_Begin(int _QueryID)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.OcclusionQuery_Begin(_QueryID);
}
void CRenderContext::OcclusionQuery_End()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.OcclusionQuery_End();
}
int CRenderContext::OcclusionQuery_GetVisiblePixelCount(int _QueryID)			// This may be a high latency operation and it is expected that this operation is performed one frame after OcclusionQuery_End has been performed.
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.OcclusionQuery_GetVisiblePixelCount(_QueryID);
}

	// Occlusion query helper
int CRenderContext::OcclusionQuery_Rect(int _QueryID, CRct _Rct, fp32 _Depth)
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.OcclusionQuery_Rect(_QueryID, _Rct, _Depth);
}

	// Depth-buffer read
bool CRenderContext::ReadDepthPixels(int _x, int _y, int _w, int _h, fp32* _pBuffer)
{
	return NRenderPS3GCM::CRCPS3GCM::ms_This.ReadDepthPixels(_x, _y, _w, _h, _pBuffer);
}

void CRC_CoreVirtualImp::Internal_RenderPolygon(int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN, const CVec4Dfp32* _pCol, const CVec4Dfp32* _pSpec, 
		const CVec4Dfp32* _pTV0, const CVec4Dfp32* _pTV1, const CVec4Dfp32* _pTV2, const CVec4Dfp32* _pTV3, int _Color)
{
}

void CRC_CoreVirtualImp::PreEndScene()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.PreEndScene();
}
void CRC_CoreVirtualImp::Texture_MakeAllDirty(int _iPicMip)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Texture_MakeAllDirty(_iPicMip);
}

	// Internals, don't use.
void CRC_CoreVirtualImp::Attrib_Update()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_Update();
}
void CRC_CoreVirtualImp::Attrib_GlobalUpdate()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_GlobalUpdate();
}
void CRC_CoreVirtualImp::Attrib_Set(CRC_Attributes* _pAttrib)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_Set(_pAttrib);
}
void CRC_CoreVirtualImp::Attrib_SetAbsolute(CRC_Attributes* _pAttrib)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Attrib_SetAbsolute(_pAttrib);
} 

void CRC_CoreVirtualImp::Matrix_SetRender(int _iMode, const CMat4Dfp32* _pMatrix)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Matrix_SetRender(_iMode, _pMatrix);
}
void CRC_CoreVirtualImp::Matrix_Update()
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Matrix_Update();
}

void CRC_CoreVirtualImp::Internal_IndexedTriangles2Wires(uint16* _pPrim, int _nTri)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Internal_IndexedTriangles2Wires(_pPrim, _nTri);
}
void CRC_CoreVirtualImp::Internal_IndexedTriStrip2Wires(uint16* _pPrim, int _Len)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Internal_IndexedTriStrip2Wires(_pPrim, _Len);
}
void CRC_CoreVirtualImp::Internal_IndexedTriFan2Wires(uint16* _pPrim, int _Len)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Internal_IndexedTriFan2Wires(_pPrim, _Len);
}
void CRC_CoreVirtualImp::Internal_IndexedPrimitives2Wires(uint16* _pPrimStream, int _StreamLen)
{
	NRenderPS3GCM::CRCPS3GCM::ms_This.Internal_IndexedPrimitives2Wires(_pPrimStream, _StreamLen);
}

#endif // M_STATIC_RENDERER
