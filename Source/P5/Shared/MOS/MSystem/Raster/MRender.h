
#ifndef _INC_MRender
#define _INC_MRender

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Render context interface.
					
	Author:			Magnus Högdahl
					
	Copyright:		1996-2001 Starbreeze Studios AB
	
	History:
		96xxxx:		Inception.

		97xxxx:		Changed interface a couple of times.

		98xxxx:		Changed interface a couple of times.

		99xxxx:		Changed interface a couple of times.

		00xxxx:		Minor changes to interface.

		010826:		Fog coordinate support has been deliberatily disabled due to:
						1) GeForce doesn't support AGP pulling of fogcoordinates
						2) We should not be calculating any fogcoords on the CPU in the first place.
						3) Fog coordinates are rarely useful because they are usualy not 
							interpolated with perspective correction. Because of this, when CPU
							calculated fog is necessary, alphablending with a fog-table texture
							would most likely be needed .
\*____________________________________________________________________________________________*/

#include "MImage.h"
#include "../Misc/MConsole.h"
#include "MRender_Classes.h"

#if !defined(PLATFORM_CONSOLE) || defined(PLATFORM_XBOX1)
#define SUPPORT_REGISTERCOMBINERS
#endif

#ifdef M_STATIC_RENDERER
#define DRenderPre static
#define DRenderPost 
#define DRenderConst 
#define DRenderTopClass CRenderContext::
#else
#define DRenderPre virtual
#define DRenderPost pure
#define DRenderConst const
#define DRenderTopClass 
#endif
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CRenderContext
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CTextureContext;
class CXR_VBContext;
class CDisplayContext;
class CXR_VBManager;
class CXR_VBMContainer;

class SYSTEMDLLEXPORT CRenderContext
	: public CConsoleClient
{
public:
	DRenderPre void Create(CObj* _pContext, const char* _pParams) DRenderPost;
	DRenderPre CDisplayContext *GetDC() DRenderPost;

	DRenderPre const char * GetRenderingStatus(int _iStatus) DRenderPost;
	DRenderPre const char * GetRenderingStatus() DRenderPost;

	DRenderPre void BeginScene(CRC_Viewport* _pVP) DRenderPost;
	DRenderPre void EndScene() DRenderPost;

	DRenderPre CRC_Statistics Statistics_Get() DRenderPost;
	DRenderPre void Flip_SetInterval(int _nFrames) DRenderPost;

	// Viewport
	DRenderPre void Viewport_Update() DRenderPost;
	DRenderPre CRC_Viewport* Viewport_Get() DRenderPost;
	DRenderPre void Viewport_Set(CRC_Viewport* _pVP) DRenderPost;
	DRenderPre void Viewport_Push() DRenderPost;
	DRenderPre void Viewport_Pop() DRenderPost;

	// Render target
	DRenderPre void RenderTarget_Clear(CRct _ClearRect, int _WhatToClear, CPixel32 _Color, fp4 _ZBufferValue, int _StecilValue) DRenderPost;
	DRenderPre void RenderTarget_SetNextClearParams(CRct _ClearRect, int _WhatToClear, CPixel32 _Color, fp4 _ZBufferValue, int _StecilValue) DRenderPost;
	DRenderPre void RenderTarget_Copy(CRct _SrcRect, CPnt _Dest, int _CopyType) DRenderPost;
	DRenderPre void RenderTarget_CopyToTexture(int _TextureID, CRct _SrcRect, CPnt _Dest, bint _bContinueTiling, uint16 _Slice) DRenderPost;

	// Caps
	DRenderPre int Caps_Flags() DRenderPost;
	DRenderPre int Caps_TextureFormats() DRenderPost;
	DRenderPre int Caps_DisplayFormats() DRenderPost;
	DRenderPre int Caps_StencilDepth() DRenderPost;

	// Render Precache flush
	DRenderPre void Render_PrecacheFlush( ) DRenderPost;
	DRenderPre void Render_PrecacheEnd() DRenderPost;

	DRenderPre void Render_SetRenderOptions(uint32 _Options, uint32 _Format = 0xFFFFFFFF) DRenderPost;


	DRenderPre void Render_EnableHardwareMemoryRegion(void *_pMemStart, mint _Size) DRenderPost;
	DRenderPre void Render_EnableHardwareMemoryRegion(CXR_VBManager *_pManager, CXR_VBMContainer *_pContainer) DRenderPost;
	DRenderPre void Render_DisableHardwareMemoryRegion() DRenderPost;


	// Texture stuff
	DRenderPre CTextureContext* Texture_GetTC() DRenderPost;
	DRenderPre class CRC_TCIDInfo* Texture_GetTCIDInfo() DRenderPost;
	DRenderPre void Texture_PrecacheFlush( ) DRenderPost;
	DRenderPre void Texture_Precache(int _TextureID) DRenderPost;
	DRenderPre void Texture_PrecacheBegin( int _Count ) DRenderPost;
	DRenderPre void Texture_PrecacheEnd() DRenderPost;
	DRenderPre void Texture_Copy(int _SourceTexID, int _DestTexID, CRct _SrcRgn, CPnt _DstPos) DRenderPost;	// Not implemented.
	DRenderPre CRC_TextureMemoryUsage Texture_GetMem(int _TextureID) DRenderPost;
	DRenderPre int Texture_GetPicmipFromGroup(int _iPicmip) DRenderPost;

	DRenderPre int Texture_GetBackBufferTextureID() DRenderPost;
	DRenderPre int Texture_GetZBufferTextureID() DRenderPost;
	DRenderPre int Texture_GetFrontBufferTextureID() DRenderPost;

	DRenderPre void Texture_BlockUntilStreamingTexturesDone() DRenderPost;

	// VertexBuffer stuff
	DRenderPre CXR_VBContext* VB_GetVBContext() DRenderPost;
	DRenderPre class CRC_VBIDInfo* VB_GetVBIDInfo() DRenderPost;

	// Attribute
	DRenderPre void Attrib_Push() DRenderPost;
	DRenderPre void Attrib_Pop() DRenderPost;

	DRenderPre void Attrib_Set(const CRC_Attributes& _Attrib) DRenderPost;
	DRenderPre void Attrib_Get(CRC_Attributes& _Attrib) DRenderConst DRenderPost;

	DRenderPre CRC_Attributes* Attrib_Begin() DRenderPost;
	DRenderPre void Attrib_End(uint _ChgFlags) DRenderPost;

	DRenderPre void Attrib_Lock(int _Flags) DRenderPost;
	DRenderPre void Attrib_LockFlags(int _Flags) DRenderPost;

	DRenderPre void Attrib_Enable(int _Flags) DRenderPost;
	DRenderPre void Attrib_Disable(int _Flags) DRenderPost;
	DRenderPre void Attrib_Switch(int _Flags) DRenderPost;
	DRenderPre void Attrib_ZCompare(int _Compare) DRenderPost;
	DRenderPre void Attrib_AlphaCompare(int _Compare, int _AlphaRef) DRenderPost;
	DRenderPre void Attrib_StencilRef(int _Ref, int _FuncAnd) DRenderPost;
	DRenderPre void Attrib_StencilWriteMask(int _Mask) DRenderPost;
	DRenderPre void Attrib_StencilFrontOp(int _Func, int _OpFail, int _OpZFail, int _OpZPass) DRenderPost;
	DRenderPre void Attrib_StencilBackOp(int _Func, int _OpFail, int _OpZFail, int _OpZPass) DRenderPost;
	DRenderPre void Attrib_RasterMode(int _Mode) DRenderPost;			// RasterMode preceeds Blend enable, SrcBlend & DestBlend

	DRenderPre void Attrib_SourceBlend(int _Blend) DRenderPost;
	DRenderPre void Attrib_DestBlend(int _Blend) DRenderPost;
	DRenderPre void Attrib_FogColor(CPixel32 _FogColor) DRenderPost;
	DRenderPre void Attrib_FogStart(fp4 _FogStart) DRenderPost;
	DRenderPre void Attrib_FogEnd(fp4 _FogEnd) DRenderPost;
	DRenderPre void Attrib_FogDensity(fp4 _FogDensity) DRenderPost;
	DRenderPre void Attrib_PolygonOffset(fp4 _Scale, fp4 _Offset) DRenderPost;
	DRenderPre void Attrib_Scissor(const CRect2Duint16& _Scissor) DRenderPost;

	// Light state
	DRenderPre void Attrib_Lights(const CRC_Light* _pLights, int _nLights) DRenderPost;	// _pLights must be valid as long as the light-state is in use.

	// Texture state
	DRenderPre void Attrib_TextureID(int _iTxt, int _TextureID) DRenderPost;
	DRenderPre void Attrib_TexEnvMode(int _iTxt, int _TexEnvMode) DRenderPost;
//	DRenderPre void Attrib_TexEnvColor(int _iTxt, CPixel32 _TexEnvColor) DRenderPost;

	DRenderPre void Attrib_TexGen(int _iTxt, int _TexGen, int _Comp) DRenderPost;
	DRenderPre void Attrib_TexGenAttr(fp4* _pAttr) DRenderPost;

	// Global attributes
	DRenderPre void Attrib_GlobalEnable(int _Flags) DRenderPost;
	DRenderPre void Attrib_GlobalDisable(int _Flags) DRenderPost;
	DRenderPre void Attrib_GlobalSwitch(int _Flags) DRenderPost;
	DRenderPre void Attrib_GlobalSetVar(int _Var, int _Value) DRenderPost;
	DRenderPre void Attrib_GlobalSetVarfv(int _Var, const fp4* _pValues) DRenderPost;
	DRenderPre int Attrib_GlobalGetVar(int _Var) DRenderPost;
	DRenderPre int Attrib_GlobalGetVarfv(int _Var, fp4* _pValues) DRenderPost;

	// Transform
	DRenderPre void Matrix_SetMode(int _iMode) DRenderPost;
	DRenderPre void Matrix_Push() DRenderPost;
	DRenderPre void Matrix_Pop() DRenderPost;
	DRenderPre void Matrix_SetUnit() DRenderPost;
	DRenderPre void Matrix_Set(const CMat4Dfp4& _Matrix) DRenderPost;
	DRenderPre void Matrix_Get(CMat4Dfp4& _Matrix) DRenderPost;
	DRenderPre void Matrix_Multiply(const CMat4Dfp4& _Matrix) DRenderPost;
	DRenderPre void Matrix_MultiplyInverse(const CMat4Dfp4& _Matrix) DRenderPost;
	DRenderPre void Matrix_PushMultiply(const CMat4Dfp4& _Matrix) DRenderPost;
	DRenderPre void Matrix_PushMultiplyInverse(const CMat4Dfp4& _Matrix) DRenderPost;
	DRenderPre void Matrix_SetPalette(const CRC_MatrixPalette* _pMatrixPaletteArgs) DRenderPost;

#ifndef DEFINE_MAT43_IS_MAT4D
	DRenderPre void Matrix_Set(const CMat43fp4& _Matrix) DRenderPost;
	DRenderPre void Matrix_Multiply(const CMat43fp4& _Matrix) DRenderPost;
	DRenderPre void Matrix_MultiplyInverse(const CMat43fp4& _Matrix) DRenderPost;
	DRenderPre void Matrix_PushMultiply(const CMat43fp4& _Matrix) DRenderPost;
	DRenderPre void Matrix_PushMultiplyInverse(const CMat43fp4& _Matrix) DRenderPost;
#endif

	// Clipping
	DRenderPre void Clip_Push() DRenderPost;
	DRenderPre void Clip_Pop() DRenderPost;
	DRenderPre void Clip_Clear() DRenderPost;
	DRenderPre void Clip_Set(const CPlane3Dfp4* _pPlanes, int _nPlanes) DRenderPost;
	DRenderPre void Clip_AddPlane(const CPlane3Dfp4& _Plane, const CMat4Dfp4* _pTransform = NULL, bool _bClipBack = true) DRenderPost;

	// Index-array rendering.
	DRenderPre void Geometry_VertexArray(const CVec3Dfp4* _pV, int _nVertices = 0, int _bAllUsed = true) DRenderPost;
	DRenderPre void Geometry_NormalArray(const CVec3Dfp4* _pN) DRenderPost;
	DRenderPre void Geometry_TVertexArray(const fp4* _pTV, int _TxtChannel = 0, int _nComp = 2) DRenderPost;
	DRenderPre void Geometry_TVertexArray(const CVec2Dfp4* _pTV, int _TxtChannel = 0) DRenderPost;
	DRenderPre void Geometry_TVertexArray(const CVec3Dfp4* _pTV, int _TxtChannel = 0) DRenderPost;
	DRenderPre void Geometry_TVertexArray(const CVec4Dfp4* _pTV, int _TxtChannel = 0) DRenderPost;
	DRenderPre void Geometry_ColorArray(const CPixel32* _pCol) DRenderPost;
	DRenderPre void Geometry_SpecularArray(const CPixel32* _pCol) DRenderPost;
//	DRenderPre void Geometry_FogArray(const fp4* _pFog) DRenderPost;					// No longer supported. /mh
	DRenderPre void Geometry_MatrixIndexArray(const uint32* _pMI) DRenderPost;
	DRenderPre void Geometry_MatrixWeightArray(const fp4* _pMW, int _nComp) DRenderPost;
	DRenderPre void Geometry_Color(CPixel32 _Col) DRenderPost;

	DRenderPre void Geometry_VertexBuffer(const CRC_VertexBuffer& _VB, int _bAllUsed) DRenderPost;
	DRenderPre void Geometry_VertexBuffer(int _VBID, int _bAllUsed) DRenderPost;

	DRenderPre void Geometry_Clear() DRenderPost;

	DRenderPre void Geometry_PrecacheFlush( ) DRenderPost;
	DRenderPre void Geometry_PrecacheBegin( int _Count ) DRenderPost;
	DRenderPre void Geometry_Precache(int _VBID) DRenderPost;
	DRenderPre void Geometry_PrecacheEnd() DRenderPost;

	DRenderPre int Geometry_GetVBSize(int _VBID) DRenderPost;

	DRenderPre void Render_IndexedTriangles(uint16* _pTriVertIndices, int _nTriangles) DRenderPost;
 	DRenderPre void Render_IndexedTriangleStrip(uint16* _pIndices, int _Len) DRenderPost;
 	DRenderPre void Render_IndexedWires(uint16* _pIndices, int _Len) DRenderPost;
 	DRenderPre void Render_IndexedPolygon(uint16* _pIndices, int _Len) DRenderPost;

 	DRenderPre void Render_IndexedPrimitives(uint16* _pPrimStream, int _StreamLen) DRenderPost;

	DRenderPre void Render_VertexBuffer(int _VBID) DRenderPost;

	// Wire
	DRenderPre void Render_Wire(const CVec3Dfp4& _v0, const CVec3Dfp4& _v1, CPixel32 _Color) DRenderPost;
	DRenderPre void Render_WireStrip(const CVec3Dfp4* _pV, const uint16* _piV, int _nVertices, CPixel32 _Color) DRenderPost;
	DRenderPre void Render_WireLoop(const CVec3Dfp4* _pV, const uint16* _piV, int _nVertices, CPixel32 _Color) DRenderPost;

	// Occlusion query
	DRenderPre void OcclusionQuery_Begin(int _QueryID) DRenderPost;
	DRenderPre void OcclusionQuery_End() DRenderPost;
	DRenderPre int OcclusionQuery_GetVisiblePixelCount(int _QueryID) DRenderPost;			// This may be a high latency operation and it is expected that this operation is performed one frame after OcclusionQuery_End has been performed.

	// Occlusion query helper
	DRenderPre int OcclusionQuery_Rect(int _QueryID, CRct _Rct, fp4 _Depth) DRenderPost;

	// Depth-buffer read
	DRenderPre bool ReadDepthPixels(int _x, int _y, int _w, int _h, fp4* _pBuffer) DRenderPost;

//	DRenderPre void Register(CScriptRegisterContext & _RegContext) DRenderPost;
};
typedef TPtr<CRenderContext> spCRenderContext;

// -------------------------------------------------------------------
#endif // _INC_MRender

