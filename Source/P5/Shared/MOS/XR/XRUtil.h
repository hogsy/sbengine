
#ifndef _INC_XRUTIL
#define _INC_XRUTIL

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			GameContext base class

	Author:			Magnus Högdahl & Jens Andersson

	Maintainer:		*.*

	Copyright:		Starbreeze Studios 1997, 2005

	Contents:		Various helpers for creating vertex buffer data

\*____________________________________________________________________________________________*/


#include "XREngine.h"
#include "XRVertexBuffer.h"

class CXR_VertexBuffer;
class CXR_VBChain;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Util
|__________________________________________________________________________________________________
\*************************************************************************************************/

enum
{
	CXR_PARTICLETYPE_AUTO			= 0,
	CXR_PARTICLETYPE_TRIANGLE		= 1,
	CXR_PARTICLETYPE_QUAD			= 2,
	CXR_PARTICLETYPE_TYPE_AND		= DBitRange(0, 1),
	CXR_PARTICLETYPE_NOPRIMITIVES	= M_Bit(2),
	CXR_PARTICLETYPE_ANGLE			= M_Bit(3),

	CXR_DEBUGVB_VERTICES			= M_Bit(0),
	CXR_DEBUGVB_TRIANGLES			= M_Bit(1),
	CXR_DEBUGVB_NORMALS				= M_Bit(2),
	CXR_DEBUGVB_TEXCOORDS			= M_Bit(3),

	CXR_BEAMFLAGS_EDGEFADE			= M_Bit(0),
	CXR_BEAMFLAGS_BEGINCHAIN		= M_Bit(1),
	CXR_BEAMFLAGS_AUTOTEXOFFSET		= M_Bit(2),
	CXR_BEAMFLAGS_TEXFROMOFFSET		= M_Bit(3),

	CXR_QUADFLAGS_SPLITX2			= M_Bit(0),
	CXR_QUADFLAGS_SPLITX4			= M_Bit(1),
	CXR_QUADFLAGS_SPLITX8			= M_Bit(2),
};

class CRC_Particle
{
public:
	CVec3Dfp32 m_Pos;
	//int32 m_Color;
	CPixel32  m_Color;  //  SS.
};

typedef CRC_Particle CXR_Particle;

class CXR_Particle2		// 24 bytes
{
public:
	CVec3Dfp32 m_Pos;
	fp32 m_Angle;		// Angle = [0..1]
	fp32 m_Size;
//	int32 m_Color;
	CPixel32  m_Color;  //  SS.
};

class CXR_Particle3
{
public:
	CVec3Dfp32 m_Pos;
	fp32 m_Angle;
	fp32 m_Size;
	int32 m_Color;
	fp32 m_Time;
};

class CXR_Particle4		// 28 bytes - angle unsupported for now
{
public:
	CVec3Dfp32	m_Pos;
	CVec2Dfp32	m_Dimensions;
	CPixel32	m_Color;
	uint32		m_iObject;
};

class CXR_Beam
{
public:
	CVec3Dfp32 m_Pos;
	CVec3Dfp32 m_Dir;
	fp32 m_Width;
//	int32 m_Color0;
//	int32 m_Color1;
	CPixel32  m_Color0; //  SS.
	CPixel32  m_Color1; //  SS.
};

class CXR_BeamStrip
{
public:
	CVec3Dfp32 m_Pos;
//	int32 m_Color;
	CPixel32  m_Color;  //  SS.
	fp32 m_Width;
	fp32 m_TextureYOfs;
	int32 m_Flags;
};

class CXR_QuadStrip
{
public:
	CVec3Dfp32 m_Pos0;
	CVec3Dfp32 m_Pos1;
//	int32 m_Color0;
//	int32 m_Color1;
	CPixel32  m_Color0; //  SS.
	CPixel32  m_Color1; //  SS.
	fp32 m_TextureYOfs;
};

enum
{
	RENDERSURFACE_ADDLIGHT			= M_Bit(0),
	RENDERSURFACE_LIGHTMAP			= M_Bit(1),
	RENDERSURFACE_FULLBRIGHT		= M_Bit(2),
	RENDERSURFACE_NODETAIL			= M_Bit(3),
	RENDERSURFACE_NOENVIRONMENT		= M_Bit(4),
	RENDERSURFACE_MATRIXSTATIC_M2W	= M_Bit(5),
	RENDERSURFACE_MATRIXSTATIC_W2V	= M_Bit(6),
	RENDERSURFACE_MATRIXSTATIC_M2V	= M_Bit(7),
	RENDERSURFACE_DEPTHFOG			= M_Bit(8),
//	RENDERSURFACE_VERTEXFOG			= M_Bit(9),
	RENDERSURFACE_HINT_TRANSPARENT	= M_Bit(10),
	RENDERSURFACE_MODULATELIGHT		= M_Bit(11),
	RENDERSURFACE_ALPHABLEND		= M_Bit(12),
	RENDERSURFACE_NOSHADERLAYERS	= M_Bit(13),
	RENDERSURFACE_LIGHTNONORMAL		= M_Bit(14),

	RENDERSURFACE_MAXTEXTUREPARAMS = XR_WALLMARK_MAXTEXTUREPARAMS,
};

class CXR_RenderSurfExtParam
{
public:
	uint16 m_TextureIDLightMap;
	uint16 m_TextureIDReflectionMap;
	uint16 m_TextureIDRefractionMap;
	fp32 m_DepthFogScale;
	uint16 m_TextureID[RENDERSURFACE_MAXTEXTUREPARAMS];
	CPixel32 m_lUserColors[2];						// Used in conjunction with RENDERSURFACE_ALPHABLEND or XW_LAYERFLAGS_USERCOLOR on surface layer.

	const CXR_Light* m_pLights;
	int m_nLights;
	CVec4Dfp32 * m_pLightFieldAxes;

	void Clear()
	{
		m_TextureIDLightMap = 0;
		m_TextureIDReflectionMap = 0;
		m_TextureIDRefractionMap = 0;
		m_DepthFogScale = 1.0f;
		for(int i = 0; i < RENDERSURFACE_MAXTEXTUREPARAMS; i++)
			m_TextureID[i] = 0;
		m_lUserColors[0] = 0xffffffff;
		m_lUserColors[1] = 0xffffffff;

		m_pLights = NULL;
		m_nLights = 0;

		m_pLightFieldAxes = NULL;
	}

	CXR_RenderSurfExtParam()
	{
		Clear();
	}
};

class CXR_ShrinkParams
{
public:
	CRect2Duint16 m_Rect;
	CVec2Dfp32 m_SrcUVMin;
	CVec2Dfp32 m_SrcUVMax;
	CVec2Dfp32 m_DstUVMin;
	CVec2Dfp32 m_DstUVMax;
	CVec2Dfp32 m_SrcPixelUV;
	CXR_VBManager* m_pVBM;
	fp32 m_Priority;
	uint m_RenderCaps;
	uint16 m_TextureID_Src;
	uint16 m_TextureID_Dst;
	uint m_nShrink;
};

enum
{
	XR_GAUSSFLAGS_NOBLUR_X = 1,
	XR_GAUSSFLAGS_NOBLUR_Y = 2,
};

class CXR_GaussianBlurParams
{
public:
	CRect2Duint16 m_Rect;
	CVec4Dfp32 m_Bias;
	CVec3Dfp32 m_Gamma;
	CVec3Dfp32 m_Scale;
	CVec2Dfp32 m_UVCenter;		// Unscaled (0->1)
	fp32 m_Exp;
	CVec2Dfp32 m_SrcUVMin;
	CVec2Dfp32 m_SrcUVMax;
	CVec2Dfp32 m_DstUVMin;
	CVec2Dfp32 m_DstUVMax;
	CVec2Dfp32 m_SrcPixelUV;
	CVec2Dfp32 m_DstPixelUV;
	CXR_VBManager* m_pVBM;
	fp32 m_Priority;
	uint m_RenderCaps;
	uint16 m_TextureID_Src;
	uint16 m_TextureID_Dst;
	uint m_Flags;
	uint m_nShrink;
	const char* m_pFilterShader;
	uint16 m_TextureID_Filter01;
	uint16 m_TextureID_Filter02;
	uint16 m_TextureID_Filter03;

	void SetFilter(const char* _pFilter = NULL, uint16 _TxtID01 = 0, uint16 _TxtID02 = 0, uint16 _TxtID03 = 0)
	{
		m_pFilterShader = _pFilter;
		m_TextureID_Filter01 = _TxtID01;
		m_TextureID_Filter02 = _TxtID02;
		m_TextureID_Filter03 = _TxtID03;
	}
};

enum
{
	XR_RADIALBLUR_MAX_PARAMS	= 1,
};

class CXR_RadialBlurParams
{
public:
	fp32 m_Power;
	fp32 m_PowerScale;
	fp32 m_PixelScale;
	fp32 m_BlurSpace[8];
	fp32 m_ColorIntensity;
	CVec3Dfp32 m_ColorScale;
	CVec2Dfp32 m_RadialCenter;	// Unscaled (0->1)
	CVec2Dfp32 m_UVCenter;		// Unscaled (0->1)
	CVec2Dfp32 m_UVExtra;		// Unscaled (0->1)
	CVec3Dfp32 m_Affection;
	uint m_RenderCaps;
	fp32 m_Priority;
	CVec2Dfp32 m_SrcUVMin;
	CVec2Dfp32 m_SrcUVMax;
	uint16 m_TextureID_Src;
	uint16 m_TextureID_Dst;
	CRect2Duint16 m_Rect;
	CXR_VBManager* m_pVBM;
	CVec2Dfp32 m_Screen_PixelUV;
	const char* m_pFilterShader;
	CVec4Dfp32 m_lFilterParams[XR_RADIALBLUR_MAX_PARAMS];
	uint m_nFilterParams;
	
	CXR_RadialBlurParams()
	{
		m_nFilterParams = 0;
	}

	void SetFilter(const char* _pFilter = NULL)
	{
		m_pFilterShader = _pFilter;
	}

	void SetFilterParams(const CVec4Dfp32* _pFilterParams, const uint _nFilterParams)
	{
		m_nFilterParams = _nFilterParams;
		for (uint i = 0; i < _nFilterParams; i++)
			m_lFilterParams[i] = _pFilterParams[i];
	}

	void SetBlurSpace(fp32 _Space0 = 1.0f, fp32 _Space1 = 3.0f, fp32 _Space2 = 5.0f, fp32 _Space3 = 7.0f, fp32 _Space4 = 9.0f, fp32 _Space5 = 11.0f, fp32 _Space6 = 13.0f, fp32 _Space7 = 15.0f)
	{
		m_BlurSpace[0] = _Space0;
		m_BlurSpace[1] = _Space1;
		m_BlurSpace[2] = _Space2;
		m_BlurSpace[3] = _Space3;
		m_BlurSpace[4] = _Space4;
		m_BlurSpace[5] = _Space5;
		m_BlurSpace[6] = _Space6;
		m_BlurSpace[7] = _Space7;
	}

	void SetBlurSpaceStep(fp32 _Space0, fp32 _Step)
	{
		m_BlurSpace[0] = _Space0;
		m_BlurSpace[1] = _Space0 + _Step;
		m_BlurSpace[2] = _Space0 + (_Step * 2.0f);
		m_BlurSpace[3] = _Space0 + (_Step * 3.0f);
		m_BlurSpace[4] = _Space0 + (_Step * 4.0f);
		m_BlurSpace[5] = _Space0 + (_Step * 5.0f);
		m_BlurSpace[6] = _Space0 + (_Step * 6.0f);
		m_BlurSpace[7] = _Space0 + (_Step * 7.0f);
	}
};

enum
{
	CXR_Util_Align_Size	= 16,
};

#define TUtilAlign						_TUtilAlign<CXR_Util_Align_Size>
#define TUtilAlignElem(_Size, _nElem)	_TUtilAlign<CXR_Util_Align_Size>(_Size) * _nElem
template <uint _tAlign>
M_FORCEINLINE uint _TUtilAlign(uint _Size)
{
	return ((_Size + _tAlign - 1) & ~(_tAlign - 1));
}

template <class _tType>
M_FORCEINLINE _tType* TUtilGetAlign(uint8*& _pVBMem, uint _Num = 1)
{
	_tType* pReturn = (_tType*)_pVBMem;
	_pVBMem += TUtilAlign(sizeof(_tType) * _Num);
	return pReturn;
}

template <class _tType>
M_FORCEINLINE _tType* TUtilGetAlignElem(uint8*& _pVBMem, uint _Num = 1)
{
	_tType* pReturn = (_tType*)_pVBMem;
	_pVBMem += TUtilAlign(sizeof(_tType)) * _Num;
	return pReturn;
}

class CXR_VirtualAttributes_SurfaceBase : public CXR_VirtualAttributes
{
public:
	CVec4Dfp2 m_HDRColor;
};


class SYSTEMDLLEXPORT CXR_Util
{
public:
	enum
	{
		MAXBEAMS = 256,
		MAXQUADS = 256,
		MAXPARTICLES = 512,
		MAXTRIFAN = 64,
		MAXSTRCONVERT = 512,
	};

	static void Render_Text(CXR_VBManager* _pVBM, CRC_Font* _pFont, const CVec3Dfp32& _Position, const CMat4Dfp32& _VMat, const char* _TextString, const fp32& _Size = 8.0f, const CPixel32& _Color = CPixel32(255,255,255,255));
//	static void CalcEnvironmentMapping(const CVec3Dfp32 &_Pos, const CMat4Dfp32& _Mat, int _nV, const uint16* _piV, const CVec3Dfp32& _N, const CVec3Dfp32* _pV, CVec2Dfp32* _pTV);
//	static void CalcEnvironmentMapping(const CVec3Dfp32 &_Pos, const CMat4Dfp32& _Mat, int _nV, const uint16* _piV, const CVec3Dfp32* _pN, const uint32* _piN, const CVec3Dfp32* _pV, CVec2Dfp32* _pTV);
//	static void CalcEnvironmentMapping(const CVec3Dfp32 &_Pos, const CMat4Dfp32& _Mat, int _nV, const uint16* _piV, const CVec3Dfp32* _pN, const CVec3Dfp32* _pV, CVec2Dfp32* _pTV);
	static void CalcEnvironmentMapping(const CVec3Dfp32 &_Pos, int _nV, const uint16* _piV, const CVec3Dfp32* _pN, const CVec3Dfp32* _pV, CVec2Dfp32* _pTV);

	// New, builds vertex-buffer
	static bool Render_Particles(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat4Dfp32 &_Mat, const CXR_Particle* _pParticles, int _nParticles, fp32 _Size, const CMat4Dfp32* _pAlign = NULL, int _PrimType = CXR_PARTICLETYPE_AUTO);
	static bool Render_Particles(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat4Dfp32 &_Mat, const CXR_Particle2* _pParticles, int _nParticles, const CMat4Dfp32* _pAlign = NULL, int _PrimType = CXR_PARTICLETYPE_AUTO, uint32* _piParticles = NULL);
	static bool Render_Particles(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat4Dfp32 &_Mat, const CXR_Particle3* _pParticles, int _nParticles, int _nFramesX, int _nFramesY, int _nTotalFrames, fp32 _Duration);

#ifdef PLATFORM_PS2
	static void Render_ApplyColorBufferPreRender(CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext);
	static void Render_ClearColorBufferPreRender(CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext);
	static void Render_GlowFilter(CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext);
#endif

	static void Render_FlaresPreRender(CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext, CXR_VBMScope* _pScope, int _Flags);
	static bool Render_Flares(CRenderContext* _pRC, CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat4Dfp32 &_Mat, const CXR_Particle4* _pParticles, int _nParticles, fp32 _Attenuation, fp32 _DepthOffset = 0.0f, int _nSamplesSqr = 3, bool _b3DProj = false);

	static bool Render_Beams(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat, const CXR_Beam *_pBeams, int _nBeams, int _Flags = 0);
	static bool Render_BeamStrip(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat, const CXR_BeamStrip *_pBeams, int _nBeams, int _Flags = 0);
	static bool Render_BeamStrip2(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat, const CXR_BeamStrip *_pBeams, int _nBeams, int _Flags = 0);
	static bool Render_QuadStrip(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat, const CXR_QuadStrip *_pBeams, int _nBeams, int _Flags = 0);

	static bool ApplyLightingRenderState(CXR_Engine* _pEngine, CXR_VBManager *_pVBM, CRC_Attributes* _pA, const CXR_Light* _pLights, int _nLights, const CVec4Dfp32& _IntensityScale,bool _bNoNormal,const CVec4Dfp32 * _pLightFieldAxes,const CMat4Dfp32 * _pModel2World);

	static bool Render_Surface(int _Flags, CMTime _Time, CXW_Surface* _pSurface, CXW_SurfaceKeyFrame* _pSurfKey, CXR_Engine* _pEngine, CXR_VBManager *_pVBM, 
		const CMat4Dfp32* _pModel2World, const CMat4Dfp32* _pWorld2View, const CMat4Dfp32* _pModel2View, 
		CXR_VertexBufferGeometry* _pVB, fp32 _BasePriority = 0, fp32 _PriorityOffset = 0.0001f, CXR_RenderSurfExtParam* _pParams = NULL);

	static bool Render_Surface(int _Flags, CMTime _Time, CXW_Surface* _pSurface, CXW_SurfaceKeyFrame* _pSurfKey, CXR_Engine* _pEngine, CXR_VBManager *_pVBM, 
		const CMat43fp32* _pModel2World, const CMat43fp32* _pWorld2View, const CMat4Dfp32* _pModel2View, 
		CXR_VertexBufferGeometry* _pVB, fp32 _BasePriority = 0, fp32 _PriorityOffset = 0.0001f, CXR_RenderSurfExtParam* _pParams = NULL);

	// Prefered path:
	static bool Render_SurfaceArray(int _Flags, uint _nVB, CXR_VertexBufferGeometry* _lVB, CXW_Surface** _lpSurface, CMTime _Time, CXR_Engine* _pEngine, CXR_VBManager *_pVBM, 
		const CMat4Dfp32* _pModel2World, const CMat4Dfp32* _pWorld2View, const CMat4Dfp32* _pModel2View, 
		fp32* _lBasePriority, fp32 _PriorityOffset = 0.0001f, CXR_RenderSurfExtParam* _pParams = NULL);

	static CXR_VertexBuffer *Create_Box(CXR_VBManager *_pVBM, const CMat4Dfp32 &_L2V, const CBox3Dfp32 &_Box, CPixel32 _Color);
	static CXR_VertexBuffer *Create_Sphere(CXR_VBManager *_pVBM, const CMat4Dfp32 &_L2V, fp32 _Radius, CPixel32 _Color);
	static CXR_VertexBuffer *Create_Star(CXR_VBManager *_pVBM, const CMat4Dfp32 &_L2V, fp32 _Radius, CPixel32 _Color);
	static CXR_VertexBuffer *Create_SOR(CXR_VBManager *_pVBM, const CMat4Dfp32 &_L2V, const CVec3Dfp32 *_pEdge, int _nEdgeVertices, fp32 _NumSegments, fp32 _StartAngle = 0, int _iMappingType = 0, float _MappingRadius = 1.0f);
	static CXR_VertexBuffer *Create_SOR(CXR_VBManager *_pVBM, const CMat4Dfp32 &_L2V, const CVec3Dfp32 *_pEdge, const CPixel32 *_pColors, int _nEdgeVertices, fp32 _NumSegments, fp32 _StartAngle = 0);
	static CXR_VertexBuffer *Create_Cylinder(CXR_VBManager *_pVBM, const CMat4Dfp32 &_L2V, int _Axis, fp32 _Height, fp32 _Radius, CPixel32 _Color, int _nSegments = 16, int _nSlices = 16);

	static void VB_ScrollTexture(CXR_VertexBuffer *_pVB, const CVec2Dfp32 &_Ofs, int _iTextureBank = 0);
	static void VB_RenderDebug(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVBSource, int _iType, int _Color = 0xffffffff);

	// -------------------------------------------------------------------
	// Light-weight 2D helpers
	static void VBM_RenderRect(CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, CXR_VBChain* _pVBChain, CRC_Attributes* _pA, CPixel32 _Color, fp32 _Priority);
	static void VBM_RenderRect(CXR_VBManager* _pVBM, const CMat4Dfp32 *M_RESTRICT _pMat, CXR_VertexBuffer* _pVB, const CRect2Duint16& _Rect, CPixel32 _Color, fp32 _Priority);
	static CXR_VertexBuffer* VBM_RenderRect(CXR_VBManager* _pVBM, const CMat4Dfp32 *M_RESTRICT _pMat, const CRect2Duint16& _Rect, CPixel32 _Color, fp32 _Priority, CRC_Attributes* _pA);
	static CXR_VertexBuffer* VBM_RenderRect(CXR_VBManager* _pVBM, const CMat4Dfp32 *M_RESTRICT _pMat, const CScissorRect& _Rect, CPixel32 _Color, fp32 _Priority, CRC_Attributes* _pA);
	CVec3Dfp32* VBM_CreateRect(CXR_VBManager* _pVBM, const CMat4Dfp32 *M_RESTRICT _pMat, const CRect2Duint16& _Rect);
	static void VBM_CreateRect(const CMat4Dfp32 *M_RESTRICT _pMat, const CRect2Duint16& _Rect, CVec3Dfp32* _pV);
	static void VBM_CreateRectUV(const CVec2Dfp32& _UVMin, const CVec2Dfp32& _UVMax, int _bVFlip, CVec2Dfp32* _pTV);
	static CVec2Dfp32* VBM_CreateRectUV(CXR_VBManager* _pVBM, const CVec2Dfp32& _UVMin, const CVec2Dfp32& _UVMax);
	static CVec2Dfp32* VBM_CreateRectUV_VFlip(CXR_VBManager* _pVBM, const CVec2Dfp32& _UVMin, const CVec2Dfp32& _UVMax);
	static CXR_VertexBuffer* VBM_RenderWires2D(CXR_VBManager* _pVBM, const CClipRect& _Clip, const CVec2Dfp32 *_p0, const CVec2Dfp32 *_p1, const CPixel32 *_Color0, const CPixel32 *_Color1, int _nLines, CRC_Attributes* _pA = NULL);
	
	static CVec2Dfp32 VBM_Convert3DTo2DPosition(CRC_Viewport* _pVP, const CMat4Dfp32& _VProjMat, const CVec3Dfp32& _3DPos, bool _bUVSpace = false);
	static CVec2Dfp32 VBM_Convert3DTo2DPosition(CRC_Viewport* _pVP, const CVec3Dfp32& _3DPos, const CMat4Dfp32& _VMat, bool _bUVSpace = false);
	static CVec2Dfp32 VBM_Convert3DTo2DPosition(CXR_VBManager* _pVBM, const CMat4Dfp32& _VMat, const CVec3Dfp32& _3DPos, bool _bUVSpace = false);
	static CVec2Dfp32 VBM_Convert3DTo2DPosition(CXR_VBManager* _pVBM, const CMat4Dfp32& _VMat, const CMat4Dfp32& _3DPos, bool _bUVSpace = false);

	// -------------------------------------------------------------------
	static void Text_PatchAttribute(CRC_Font* _pF, CRC_Attributes* _pA);					// Patch attribute (sets font texture id.)
	static void Text_SetAttribute(CRC_Font* _pF, CRC_Attributes* _pA);						// Set full attribute
	static CRC_Attributes* Text_CreateAttribute(CXR_VBManager* _pVBM, CRC_Font* _pF);		// Allocate and set full attribute

	static CXR_VertexBuffer* Text_Create(CXR_VBManager* _pVBM, CRC_Font* _pF, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const wchar* _pStr, 
		const CVec2Dfp32& _Size, CPixel32 _Color, CRC_Attributes* _pA = NULL, const CVec2Dfp32& _MinLimit = 0, const CVec2Dfp32& _MaxLimit = 10000);
	static CXR_VertexBuffer* Text_Create(CXR_VBManager* _pVBM, CRC_Font* _pF, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const char* _pStr, 
		const CVec2Dfp32& _Size, CPixel32 _Color, CRC_Attributes* _pA = NULL, const CVec2Dfp32& _MinLimit = 0, const CVec2Dfp32& _MaxLimit = 10000);
	static CXR_VertexBuffer* Text_Create(CXR_VBManager* _pVBM, CRC_Font* _pF, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, CStr& _Str, 
		const CVec2Dfp32& _Size, CPixel32 _Color, CRC_Attributes* _pA = NULL, const CVec2Dfp32& _MinLimit = 0, const CVec2Dfp32& _MaxLimit = 10000);

	static CXR_VertexBuffer* Text_Create(CXR_VBManager* _pVBM, CRC_Font* _pF, CMat4Dfp32* _pMat, const CVec2Dfp32& _Pos, const char* _pText, uint32 _Color, CRC_Attributes* _pA = NULL);
	static CXR_VertexBuffer* Text_Create(CXR_VBManager* _pVBM, CRC_Font* _pF, CMat4Dfp32* _pMat, const CVec2Dfp32& _Pos, const wchar* _pText, uint32 _Color, CRC_Attributes* _pA = NULL);
	static bool Text_Add(CXR_VBManager* _pVBM, CRC_Font* _pF, CMat4Dfp32* _pMat, const CVec2Dfp32& _Pos, const char* _pText, uint32 _Color, CRC_Attributes* _pA = NULL);
	static bool Text_Add(CXR_VBManager* _pVBM, CRC_Font* _pF, CMat4Dfp32* _pMat, const CVec2Dfp32& _Pos, const wchar* _pText, uint32 _Color, CRC_Attributes* _pA = NULL);

	// -------------------------------------------------------------------
	static bool ShrinkTexture(const CXR_ShrinkParams *M_RESTRICT _pParams, CVec2Dfp32*& _pRetTVShrink);
	static bool GaussianBlur(const CXR_GaussianBlurParams *M_RESTRICT _pParams, CVec2Dfp32*& _pRetTVBlur);
	static bool RadialBlur(const CXR_RadialBlurParams *M_RESTRICT _pParams, CVec2Dfp32*& _pRetTVRadialBlur);

	// -------------------------------------------------------------------
	static void Init();
	static bool m_bInit;

	static CVec2Dfp32 m_lBeamTVertices[4 * MAXBEAMS];
	static uint16 m_lBeamTriangles[6 * MAXBEAMS];

	static CVec2Dfp32 m_lBeamStripTVertices[2 * MAXBEAMS];
	static uint16 m_lBeamStripTriangles[6 * MAXBEAMS];

	static bool m_bQuadParticleInit;
	static CVec2Dfp32 m_lQuadParticleTVertices[4 * MAXPARTICLES];
	static uint16 m_lQuadParticleTriangles[6 * MAXPARTICLES];

	static CVec2Dfp32 m_lTriParticleTVertices[3 * MAXPARTICLES];
	static uint16 m_lTriParticleTriangles[3 * MAXPARTICLES];

	static uint16 m_lTriFanTriangles[3 * MAXTRIFAN];

	static CRC_ExtAttributes_FragmentProgram20 ms_lFPLighting[4];
};

#endif //_INC_XRUTIL

