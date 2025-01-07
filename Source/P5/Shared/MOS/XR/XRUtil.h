
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

class CXR_VertexBuffer;

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
	CXR_PARTICLETYPE_NOPRIMITIVES	= DBit(2),
	CXR_PARTICLETYPE_ANGLE			= DBit(3),

	CXR_DEBUGVB_VERTICES			= DBit(0),
	CXR_DEBUGVB_TRIANGLES			= DBit(1),
	CXR_DEBUGVB_NORMALS				= DBit(2),
	CXR_DEBUGVB_TEXCOORDS			= DBit(3),

	CXR_BEAMFLAGS_EDGEFADE			= DBit(0),
	CXR_BEAMFLAGS_BEGINCHAIN		= DBit(1),
	CXR_BEAMFLAGS_AUTOTEXOFFSET		= DBit(2),
	CXR_BEAMFLAGS_TEXFROMOFFSET		= DBit(3),

	CXR_QUADFLAGS_SPLITX2			= DBit(0),
	CXR_QUADFLAGS_SPLITX4			= DBit(1),
	CXR_QUADFLAGS_SPLITX8			= DBit(2),
};

class CRC_Particle
{
public:
	CVec3Dfp4 m_Pos;
	//int32 m_Color;
	CPixel32  m_Color;  //  SS.
};

typedef CRC_Particle CXR_Particle;

class CXR_Particle2		// 24 bytes
{
public:
	CVec3Dfp4 m_Pos;
	fp4 m_Angle;		// Angle = [0..1]
	fp4 m_Size;
//	int32 m_Color;
	CPixel32  m_Color;  //  SS.
};

class CXR_Particle3
{
public:
	CVec3Dfp4 m_Pos;
	fp4 m_Angle;
	fp4 m_Size;
	int32 m_Color;
	fp4 m_Time;
};

class CXR_Beam
{
public:
	CVec3Dfp4 m_Pos;
	CVec3Dfp4 m_Dir;
	fp4 m_Width;
//	int32 m_Color0;
//	int32 m_Color1;
	CPixel32  m_Color0; //  SS.
	CPixel32  m_Color1; //  SS.
};

class CXR_BeamStrip
{
public:
	CVec3Dfp4 m_Pos;
//	int32 m_Color;
	CPixel32  m_Color;  //  SS.
	fp4 m_Width;
	fp4 m_TextureYOfs;
	int32 m_Flags;
};

class CXR_QuadStrip
{
public:
	CVec3Dfp4 m_Pos0;
	CVec3Dfp4 m_Pos1;
//	int32 m_Color0;
//	int32 m_Color1;
	CPixel32  m_Color0; //  SS.
	CPixel32  m_Color1; //  SS.
	fp4 m_TextureYOfs;
};

enum
{
	RENDERSURFACE_ADDLIGHT			= DBit(0),
	RENDERSURFACE_LIGHTMAP			= DBit(1),
	RENDERSURFACE_FULLBRIGHT		= DBit(2),
	RENDERSURFACE_NODETAIL			= DBit(3),
	RENDERSURFACE_NOENVIRONMENT		= DBit(4),
	RENDERSURFACE_MATRIXSTATIC_M2W	= DBit(5),
	RENDERSURFACE_MATRIXSTATIC_W2V	= DBit(6),
	RENDERSURFACE_MATRIXSTATIC_M2V	= DBit(7),
	RENDERSURFACE_DEPTHFOG			= DBit(8),
//	RENDERSURFACE_VERTEXFOG			= DBit(9),
	RENDERSURFACE_HINT_TRANSPARENT	= DBit(10),
	RENDERSURFACE_MODULATELIGHT		= DBit(11),
	RENDERSURFACE_ALPHABLEND		= DBit(12),
	RENDERSURFACE_NOSHADERLAYERS	= DBit(13),

	RENDERSURFACE_MAXTEXTUREPARAMS = XR_WALLMARK_MAXTEXTUREPARAMS,
};

class CXR_RenderSurfExtParam
{
public:
	uint16 m_TextureIDLightMap;
	uint16 m_TextureIDReflectionMap;
	uint16 m_TextureIDRefractionMap;
	fp4 m_DepthFogScale;
	uint16 m_TextureID[RENDERSURFACE_MAXTEXTUREPARAMS];
	CPixel32 m_lUserColors[2];						// Used in conjunction with RENDERSURFACE_ALPHABLEND or XW_LAYERFLAGS_USERCOLOR on surface layer.

	CXR_Light* m_pLights;
	int m_nLights;

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
	}

	CXR_RenderSurfExtParam()
	{
		Clear();
	}
};

class SYSTEMDLLEXPORT CXR_Util
{
public:
	enum
	{
		MAXBEAMS = 256,
		MAXQUADS = 256,
		MAXPARTICLES = 512,
	};

	static void Render_Text(CXR_VBManager* _pVBM, CRC_Font* _pFont, const CVec3Dfp4& _Position, const CMat43fp4& _VMat, const char* _TextString, const fp4& _Size = 8.0f, const CPixel32& _Color = CPixel32(255,255,255,255));
//	static void CalcEnvironmentMapping(const CVec3Dfp4 &_Pos, const CMat43fp4& _Mat, int _nV, const uint16* _piV, const CVec3Dfp4& _N, const CVec3Dfp4* _pV, CVec2Dfp4* _pTV);
//	static void CalcEnvironmentMapping(const CVec3Dfp4 &_Pos, const CMat43fp4& _Mat, int _nV, const uint16* _piV, const CVec3Dfp4* _pN, const uint32* _piN, const CVec3Dfp4* _pV, CVec2Dfp4* _pTV);
//	static void CalcEnvironmentMapping(const CVec3Dfp4 &_Pos, const CMat43fp4& _Mat, int _nV, const uint16* _piV, const CVec3Dfp4* _pN, const CVec3Dfp4* _pV, CVec2Dfp4* _pTV);
	static void CalcEnvironmentMapping(const CVec3Dfp4 &_Pos, int _nV, const uint16* _piV, const CVec3Dfp4* _pN, const CVec3Dfp4* _pV, CVec2Dfp4* _pTV);

	// New, builds vertex-buffer
	static bool Render_Particles(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat43fp4 &_Mat, const CXR_Particle* _pParticles, int _nParticles, fp4 _Size, const CMat43fp4* _pAlign = NULL, int _PrimType = CXR_PARTICLETYPE_AUTO);
	static bool Render_Particles(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat43fp4 &_Mat, const CXR_Particle2* _pParticles, int _nParticles, const CMat43fp4* _pAlign = NULL, int _PrimType = CXR_PARTICLETYPE_AUTO, uint32* _piParticles = NULL);
	static bool Render_Particles(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat43fp4 &_Mat, const CXR_Particle3* _pParticles, int _nParticles, int _nFramesX, int _nFramesY, int _nTotalFrames, fp4 _Duration);

#ifdef PLATFORM_PS2
	static void Render_ApplyColorBufferPreRender(CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext);
	static void Render_ClearColorBufferPreRender(CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext);
	static void Render_GlowFilter(CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext);
#endif

	static void Render_FlaresPreRender(CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext, CXR_VBMScope* _pScope, int _Flags);
	static bool Render_Flares(CRenderContext* _pRC, CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat43fp4 &_Mat, const CXR_Particle2* _pParticles, int _nParticles, fp4 _Attenuation, fp4 _DepthOffset = 0.0f, int _nSamplesSqr = 3, bool _b3DProj = false);

	static bool Render_Beams(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat43fp4 &_WMat, const CMat43fp4 &_VMat, const CXR_Beam *_pBeams, int _nBeams, int _Flags = 0);
	static bool Render_BeamStrip(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat43fp4 &_WMat, const CMat43fp4 &_VMat, const CXR_BeamStrip *_pBeams, int _nBeams, int _Flags = 0);
	static bool Render_BeamStrip2(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat43fp4 &_WMat, const CMat43fp4 &_VMat, const CXR_BeamStrip *_pBeams, int _nBeams, int _Flags = 0);
	static bool Render_QuadStrip(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat43fp4 &_WMat, const CMat43fp4 &_VMat, const CXR_QuadStrip *_pBeams, int _nBeams, int _Flags = 0);

	static bool ApplyLightingRenderState(CXR_Engine* _pEngine, CXR_VBManager *_pVBM, CRC_Attributes* _pA, const CXR_Light* _pLights, int _nLights, const CVec4Dfp4& _IntensityScale);

	static bool Render_Surface(int _Flags, CXW_Surface* _pSurface, CXW_SurfaceKeyFrame* _pSurfKey, CXR_Engine* _pEngine, CXR_VBManager *_pVBM, 
		const CMat43fp4* _pModel2World, const CMat43fp4* _pWorld2View, const CMat43fp4* _pModel2View, 
		CXR_VertexBuffer* _pVB, fp4 _BasePriority = 0, fp4 _PriorityOffset = 0.0001f, CXR_RenderSurfExtParam* _pParams = NULL);

#ifndef DEFINE_MAT43_IS_MAT4D
	static bool Render_Surface(int _Flags, CXW_Surface* _pSurface, CXW_SurfaceKeyFrame* _pSurfKey, CXR_Engine* _pEngine, CXR_VBManager *_pVBM, 
		const CMat43fp4* _pModel2World, const CMat43fp4* _pWorld2View, const CMat4Dfp4* _pModel2View, 
		CXR_VertexBuffer* _pVB, fp4 _BasePriority = 0, fp4 _PriorityOffset = 0.0001f, CXR_RenderSurfExtParam* _pParams = NULL);
#endif

	static CXR_VertexBuffer *Create_Box(CXR_VBManager *_pVBM, const CMat43fp4 &_L2V, const CBox3Dfp4 &_Box, CPixel32 _Color);
	static CXR_VertexBuffer *Create_Sphere(CXR_VBManager *_pVBM, const CMat43fp4 &_L2V, fp4 _Radius, CPixel32 _Color);
	static CXR_VertexBuffer *Create_Star(CXR_VBManager *_pVBM, const CMat43fp4 &_L2V, fp4 _Radius, CPixel32 _Color);
	static CXR_VertexBuffer *Create_SOR(CXR_VBManager *_pVBM, const CMat43fp4 &_L2V, const CVec3Dfp4 *_pEdge, int _nEdgeVertices, fp4 _NumSegments, fp4 _StartAngle = 0, int _iMappingType = 0, float _MappingRadius = 1.0f);
	static CXR_VertexBuffer *Create_SOR(CXR_VBManager *_pVBM, const CMat43fp4 &_L2V, const CVec3Dfp4 *_pEdge, const CPixel32 *_pColors, int _nEdgeVertices, fp4 _NumSegments, fp4 _StartAngle = 0);
	static CXR_VertexBuffer *Create_Cylinder(CXR_VBManager *_pVBM, const CMat43fp4 &_L2V, int _Axis, fp4 _Height, fp4 _Radius, CPixel32 _Color, int _nSegments = 16, int _nSlices = 16);

	static void VB_ScrollTexture(CXR_VertexBuffer *_pVB, const CVec2Dfp4 &_Ofs, int _iTextureBank = 0);
	static void VB_RenderDebug(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVBSource, int _iType, int _Color = 0xffffffff);

	// -------------------------------------------------------------------
	// Light-weight 2D helpers
	static void VBM_Get2DMatrix(CRC_Viewport* _pVB, CMat4Dfp4& _Mat, fp4 _Z);
	static void VBM_Get2DMatrix_RelBackPlane(CRC_Viewport* _pVB, CMat4Dfp4& _Mat, fp4 _BackPlaneOfs = -16.0f);
	static CXR_VertexBuffer* VBM_RenderRect(CRC_Viewport* _pVP, CXR_VBManager* _pVBM, const CRect2Duint16& _Rect, CPixel32 _Color, fp4 _Priority, CRC_Attributes* _pA);
	static CVec2Dfp4* VBM_CreateRectUV(CXR_VBManager* _pVBM, const CVec2Dfp4& _UVMin, const CVec2Dfp4& _UVMax);
	static CVec2Dfp4* VBM_CreateRectUV_VFlip(CXR_VBManager* _pVBM, const CVec2Dfp4& _UVMin, const CVec2Dfp4& _UVMax);

	// -------------------------------------------------------------------
	static void Init();
	static bool m_bInit;

	static CVec2Dfp4 m_lBeamTVertices[4 * MAXBEAMS];
	static uint16 m_lBeamTriangles[6 * MAXBEAMS];

	static CVec2Dfp4 m_lBeamStripTVertices[2 * MAXBEAMS];
	static uint16 m_lBeamStripTriangles[6 * MAXBEAMS];

	static bool m_bQuadParticleInit;
	static CVec2Dfp4 m_lQuadParticleTVertices[4 * MAXPARTICLES];
	static uint16 m_lQuadParticleTriangles[6 * MAXPARTICLES];

	static CVec2Dfp4 m_lTriParticleTVertices[3 * MAXPARTICLES];
	static uint16 m_lTriParticleTriangles[3 * MAXPARTICLES];

	static CRC_ExtAttributes_FragmentProgram20 ms_lFPLighting[4];
};

#endif //_INC_XRUTIL

