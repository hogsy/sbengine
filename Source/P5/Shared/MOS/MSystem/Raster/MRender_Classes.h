
#ifndef _INC_MRender_Classes
#define _INC_MRender_Classes

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Classes related to the render context interface.

	Author:			Magnus Högdahl

	Copyright:		1996-2001 Starbreeze Studios AB

	History:
		010827:		Created file. Moved stuff from MRender.h

\*____________________________________________________________________________________________*/

#include "MRender_Classes_VPUShared.h"


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CRC_ClipVolume
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum 
{
	CRC_RENDEROPTION_NOZBUFFER = M_Bit(0),
	CRC_RENDEROPTION_NOANTIALIAS = M_Bit(1),
	CRC_RENDEROPTION_PRESERVEZ = M_Bit(2),
	CRC_RENDEROPTION_CONTINUETILING = M_Bit(3),
	CRC_RENDEROPTION_ = M_Bit(4),
};

class CRC_RenderTargetDesc
{
public:
	CVec4Dfp32 m_lColorClearColor[CRC_MAXMRT];
	uint32 m_lTextureFormats[CRC_MAXMRT];
	uint16 m_lColorTextureID[CRC_MAXMRT];
	uint8 m_lSlice[CRC_MAXMRT];

	CRct m_ClearRect;
	uint32 m_ClearBuffers:8;
	uint32 m_ClearValueStencil:8;
	uint32 m_Options:16;
	fp32 m_ClearValueZ;
	CRct m_ResolveRect;

	void Clear();
	void SetRenderTarget(int _iBuffer, int _TextureID, const CVec4Dfp32& _ClearColor, uint32 _Format = 0, uint32 _Slice = 0);
	void SetClear(int _Buffers, CRct _Rect, fp32 _ClearZ, int _ClearStencil);
	void SetOptions(int _Options);
	void SetResolveRect(CRct _Rect);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CRC_ClipVolume
|__________________________________________________________________________________________________
\*************************************************************************************************/
#define CRC_CLIPVOLUME_MAXPLANES 16

class SYSTEMDLLEXPORT CRC_ClipVolume:public CReferenceCount
{
public:

	DECLARE_OPERATOR_NEW

	CVec3Dfp32 m_POV;
	int m_nPlanes;
	CPlane3Dfp32 m_Planes[CRC_CLIPVOLUME_MAXPLANES];
	CVec3Dfp32 m_Vertices[CRC_CLIPVOLUME_MAXPLANES];

	CRC_ClipVolume();
	const CRC_ClipVolume& operator = (const CRC_ClipVolume &ToCopy)
	{
#ifdef	PLATFORM_PS2
		PS2WordCopy( this, &ToCopy, sizeof(*this) / 4 );
#else
		memcpy(this, &ToCopy, sizeof(*this));
#endif

		return *this;
	}

	CRC_ClipVolume(const CRC_ClipVolume &ToCopy)
	{
#ifdef	PLATFORM_PS2
		PS2WordCopy( this, &ToCopy, sizeof(*this) / 4 );
#else
		memcpy(this, &ToCopy, sizeof(*this));
#endif
	}


	void Init(const CVec3Dfp32& _POV);
	void GetVertices(const CVec3Dfp32* _pVertices, int _nVertices, const CVec3Dfp32& _POV, bool _bFlipDirection);
	void CreateFromVertices();
	void CreateFromVertices2();
	void CreateFromVertices3(const CPlane3Dfp32& _ProjPlane);

	void Invert();
	void Transform(const CMat4Dfp32& _Mat);

	void Copy(const CRC_ClipVolume& _SrcClip);
	void CopyAndTransform(const CRC_ClipVolume& _SrcClip, const CMat4Dfp32& _pMat);

	int BoxInVolume(const CBox3Dfp32& _Box) const;
	int SphereInVolume(const CVec3Dfp32& _v, fp32 _r) const;
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CRC_Light
|__________________________________________________________________________________________________
\*************************************************************************************************/
#define CRC_LIGHTTYPE_POINT			0
#define CRC_LIGHTTYPE_SPOT			1
#define CRC_LIGHTTYPE_PARALLELL		2
#define CRC_LIGHTTYPE_AMBIENT		3

class CRC_Light		// 12*4 = 48 bytes
{
public:
	int32 m_Type;
	CPixel32 m_Color;
	CPixel32 m_Ambient;
	CVec3Dfp32 m_Pos;
	CVec3Dfp32 m_Direction;
	fp32 m_Attenuation[3];		// Constant, Linear and Quadratic attenuation.

	CRC_Light()
	{
		m_Type = CRC_LIGHTTYPE_POINT;
		m_Color = 0xffffffff;
		m_Ambient = 0x00000000;
		m_Pos = 0;
		m_Direction = 0;
		m_Attenuation[0] = 0;
		m_Attenuation[1] = 0;
		m_Attenuation[2] = 0;
	}
};


/***************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CRC_MatrixPalette
|___________________________________________________________________________________________________
\***************************************************************************************************/

enum ERC_MatrixPaletteFlags
{
	 ERC_MatrixPaletteFlags_SpecialCubePosition = M_Bit(0)
	,ERC_MatrixPaletteFlags_SpecialCubeTexture = M_Bit(1)
	,ERC_MatrixPaletteFlags_SpecialCubeTextureScaleByZ = M_Bit(2)
	,ERC_MatrixPaletteFlags_SpecialCubeTextureScale2 = M_Bit(3)
	,ERC_MatrixPaletteFlags_DoNotCache = M_Bit(4)
	,ERC_MatrixPaletteFlags_SpecialCubeFlags = ERC_MatrixPaletteFlags_SpecialCubePosition | ERC_MatrixPaletteFlags_SpecialCubeTexture | ERC_MatrixPaletteFlags_SpecialCubeTextureScale2 | ERC_MatrixPaletteFlags_SpecialCubeTextureScaleByZ
};

class SYSTEMDLLEXPORT CRC_MatrixPalette
{
public:
	CRC_MatrixPalette()
	{
		m_VpuTaskId=InvalidVpuTask;
	}
    const void* m_pMatrices;
    const uint16* m_piMatrices;
    uint32 m_nMatrices;
	uint16 m_Flags;
	uint16 m_VpuTaskId;

	const CMat43fp32 &Index(int _Index) const
	{
		if (m_piMatrices)
			return ((const CMat43fp32*)m_pMatrices)[m_piMatrices[_Index]];
		else
			return ((const CMat43fp32*)m_pMatrices)[_Index];
	}
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CRC_Viewport
|__________________________________________________________________________________________________
\*************************************************************************************************/
class SYSTEMDLLEXPORT CRC_Viewport
{
	// Big stuff first
	CMat4Dfp32 m_PMat;					// Projection matrix
	CVec3Dfp32 m_VViewVertices[8];		// Viewport space by vertices
	CPlane3Dfp32 m_VViewPlanes[4];		// Viewport space
	CVec2Dfp32 m_PixelAspect;
	CClipRect m_Clip;					// The clipping area.

	enum
	{
		VP_MODE_FOV = 0,
		VP_MODE_SCALE,
	};

	int m_Mode;
	fp32 m_FOV;
	fp32 m_FOVAspect;
	fp32 m_Scale;
	fp32 m_AspectRatio;
	fp32 m_xScale;
	fp32 m_yScale;
	fp32 m_FrontPlane;
	fp32 m_BackPlane;
	CRct m_Rect;						// The desired view, before clipping.
	// Viewport geometry, in view-space
	int m_nViewVertices;
	bool m_bVPChanged;

	void Update();

public:


	DECLARE_OPERATOR_NEW

	CRC_Viewport();
	~CRC_Viewport();
	
	const CRC_Viewport& operator = (const CRC_Viewport &ToCopy)
	{
		memcpy(this, &ToCopy, sizeof(CRC_Viewport));

		return *this;
	}

	CRC_Viewport(const CRC_Viewport &ToCopy)
	{
		memcpy(this, &ToCopy, sizeof(CRC_Viewport));
	}


	void SetBorder(CVec2Dfp32 _Border, bool _bRemove = false); // Adds a border to the viewport, (0, 0) removes it
		
	void SetFOV(fp32 _fp);
	void SetFOVAspect(fp32 _Aspect);
	void SetScale(fp32 _Scale);
	void SetAspectRatio(fp32 _Aspect);
	void SetPixelAspect(fp32 _AspectX, fp32 _AspectY);
	void SetFrontPlane(fp32 _FrontPlane);
	void SetBackPlane(fp32 _BackPlane);
	void SetView(const CClipRect& clip, const CRct& rect);
	void SetView(CImage* pImg);

	const CMat4Dfp32& GetProjectionMatrix();
	void GetClipVolume(CRC_ClipVolume& _Clip, const CMat4Dfp32* _pViewWMat = NULL);

	int GetNumViewVertices();
	const CVec3Dfp32* GetViewVertices();
	const CPlane3Dfp32* GetViewPlanes();

	static void DebugViewControl(CMat4Dfp32& mat, fp32 Ctrl_x, fp32 Ctrl_z, fp32 Ctrl_y, fp32 Ctrl_xaxis, fp32 Ctrl_yaxis, fp32 _apan1);

	M_FORCEINLINE fp32 GetFOV() const { return m_FOV; };
	M_FORCEINLINE fp32 GetFOVAspect() const { return m_FOVAspect; };
	M_FORCEINLINE fp32 GetScale() { Update(); return m_Scale; };
	M_FORCEINLINE fp32 GetAspectRatio() const { return m_AspectRatio; };
	M_FORCEINLINE fp32 GetFrontPlane() const { return m_FrontPlane; };
	M_FORCEINLINE fp32 GetBackPlane() const { return m_BackPlane; };
	M_FORCEINLINE fp32 GetXScale() { Update(); return m_xScale; };
	M_FORCEINLINE fp32 GetYScale() { Update(); return m_yScale; };
	M_FORCEINLINE const CClipRect& GetViewClip() const { return m_Clip; };
	M_FORCEINLINE const CRct& GetViewRect() const { return m_Rect; };
	CRct GetViewArea() const;

	void Get2DMatrix(CMat4Dfp32& _Mat, fp32 _Z);
	void Get2DMatrix_RelBackPlane(CMat4Dfp32& _Mat, fp32 _BackPlaneOfs = -16.0f);

	int SphereInView(const CVec3Dfp32& p, fp32 _Radius);
};

// -------------------------------------------------------------------
//  Caps & GlobalVar enums
// -------------------------------------------------------------------
//#if !defined(PLATFORM_CONSOLE) || defined(PLATFORM_XENON) || defined()
#define SUPPORT_FRAGMENTPROGRAM
//#endif

enum
{
	CRC_GLOBALFLAGS_WIRE		= 1,
	CRC_GLOBALFLAGS_TEXTURE		= 2,

	CRC_GLOBALVAR_FILTER		= 0,
	CRC_GLOBALVAR_GAMMARAMPSCALE = 1,
	CRC_GLOBALVAR_GAMMARAMPADD	= 2,
	CRC_GLOBALVAR_GAMMA			= 3,
	CRC_GLOBALVAR_WIRE			= 4,
	CRC_GLOBALVAR_ALLOWTEXTURELOAD = 5,
	CRC_GLOBALVAR_GAMMARAMP		= 6,

	CRC_GLOBALVAR_NUMTEXTURES	= 10,				// Number of multitexture units
	CRC_GLOBALVAR_NUMAUTOMIP	= 11,
	CRC_GLOBALVAR_NUMTEXTUREENV	= 12,				// Number of texture environments
	CRC_GLOBALVAR_NUMTEXTURECOORDS = 13,			// Number of texture coordinates
	CRC_GLOBALVAR_TOTALTRIANGLES = 20, 

	CRC_GLOBALFILTER_POINT		= 0,
	CRC_GLOBALFILTER_BILINEAR	= 1,
	CRC_GLOBALFILTER_TRILINEAR	= 2,

	// -------------------------------------------------------------------
	CRC_CAPS_FLAGS_HWAPI					= M_Bit(0),
//	CRC_CAPS_FLAGS_FULLBLENDING				= M_Bit(1),		// I don't think this is relevant any longer  :)   -mh
	CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP= M_Bit(1),
	CRC_CAPS_FLAGS_GAMMARAMP				= M_Bit(2),
	CRC_CAPS_FLAGS_SPECULARCOLOR			= M_Bit(3),
	CRC_CAPS_FLAGS_COPYDEPTH				= M_Bit(4),
	CRC_CAPS_FLAGS_CUBEMAP					= M_Bit(5),
	CRC_CAPS_FLAGS_MATRIXPALETTE			= M_Bit(6),
	CRC_CAPS_FLAGS_OFFSCREENTEXTURERENDER	= M_Bit(7),
	CRC_CAPS_FLAGS_OCCLUSIONQUERY			= M_Bit(8),
	CRC_CAPS_FLAGS_READDEPTH				= M_Bit(9),
	CRC_CAPS_FLAGS_TEXGENMODE_ENV			= M_Bit(10),
	CRC_CAPS_FLAGS_TEXGENMODE_ENV2			= M_Bit(11),
	CRC_CAPS_FLAGS_TEXGENMODE_LINEARNHF     = M_Bit(12),
	CRC_CAPS_FLAGS_TEXGENMODE_BOXNHF        = M_Bit(13),
	CRC_CAPS_FLAGS_EXTATTRIBUTES_TEXENV		= M_Bit(14), // 65536
	CRC_CAPS_FLAGS_EXTATTRIBUTES_NV10		= M_Bit(15),
	CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20		= M_Bit(16),
	CRC_CAPS_FLAGS_GAMECUBE					= M_Bit(17),
	CRC_CAPS_FLAGS_TEXENVMODE_COMBINE		= M_Bit(18),
	CRC_CAPS_FLAGS_TEXENVMODE2	 			= M_Bit(19),
	CRC_CAPS_FLAGS_VBOPERATOR_WAVE			= M_Bit(20),
	CRC_CAPS_FLAGS_ARBITRARY_TEXTURE_SIZE	= M_Bit(21),
	CRC_CAPS_FLAGS_BACKBUFFERASTEXTURE		= M_Bit(22),
	CRC_CAPS_FLAGS_FRONTBUFFERASTEXTURE		= M_Bit(23),
	CRC_CAPS_FLAGS_SEPARATESTENCIL 			= M_Bit(24),
	CRC_CAPS_FLAGS_FRAGMENTPROGRAM11	 	= M_Bit(25),
	CRC_CAPS_FLAGS_FRAGMENTPROGRAM14		= M_Bit(26),
	CRC_CAPS_FLAGS_FRAGMENTPROGRAM20		= M_Bit(27),
	CRC_CAPS_FLAGS_FRAGMENTPROGRAM30 		= M_Bit(28),
	CRC_CAPS_FLAGS_PRIMITIVERESTART			= M_Bit(29),
	CRC_CAPS_FLAGS_MRT						= M_Bit(30),

};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CRC_Attributes
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*
	if "RasterMode" != CRC_RASTERMODE_NONE
		Attemps to modify the following attributes are ignored;

	Flag: CRC_FLAGS_BLEND
	Flag: CRC_FLAGS_ALPHA
	SourceBlend,
	DestBlend
*/

#ifdef CPU_BIGENDIAN
#define MAKE_SOURCEDEST_BLEND(_Src_, _Dest_) (((_Src_) << 8) | (_Dest_))
#else
#define MAKE_SOURCEDEST_BLEND(_Src_, _Dest_) (((_Dest_) << 8) | (_Src_))
#endif
// --- FLAGS ---

enum
{
	CRC_FLAGS_ALPHATOCOVERAGE		= 0x00000001,	// <- Free enum
	CRC_FLAGS_ZCOMPARE				= 0x00000002,
	CRC_FLAGS_ZWRITE				= 0x00000004,
	CRC_FLAGS_BLEND					= 0x00000008,
	CRC_FLAGS_SMOOTH				= 0x00000010,
	CRC_FLAGS_SCISSOR				= 0x00000020,
	CRC_FLAGS_FOG					= 0x00000040,
	CRC_FLAGS_SEPARATESTENCIL		= 0x00000080,	// m_StencilBackXxx attributes should be used
	CRC_FLAGS_ALPHATOCOVERAGEDITHER	= 0x00000100,
	CRC_FLAGS_PERSPECTIVE			= 0x00000200,
	CRC_FLAGS_CULL					= 0x00000800,	// Face-culling
	CRC_FLAGS_CULLCW				= 0x00001000,	// Cull counter clockwise.
	CRC_FLAGS_CLIP					= 0x00002000,	// Clip all geometry to clipping planes..
	CRC_FLAGS_STENCIL				= 0x00004000,
	CRC_FLAGS_SECONDARYCOLOR		= 0x00010000,
//	CRC_FLAGS_FOGCOORD				= 0x00020000,
	CRC_FLAGS_POLYGONOFFSET			= 0x00040000,
	CRC_FLAGS_LIGHTING				= 0x00080000,	// Enable vertex lighting using lights provided with Attrib_SetLights. Normals must be supplied.

	CRC_FLAGS_COLORWRITE			= 0x00100000,
	CRC_FLAGS_COLORWRITE0			= 0x00100000,
	CRC_FLAGS_COLORWRITE1			= 0x00200000,
	CRC_FLAGS_COLORWRITE2			= 0x00400000,
	CRC_FLAGS_COLORWRITE3			= 0x00800000,
	CRC_FLAGS_COLORWRITEALL			= CRC_FLAGS_COLORWRITE0 | CRC_FLAGS_COLORWRITE1 | CRC_FLAGS_COLORWRITE2 | CRC_FLAGS_COLORWRITE3,

	CRC_FLAGS_ALPHAWRITE			= 0x01000000,
	CRC_FLAGS_ALPHAWRITE0			= CRC_FLAGS_ALPHAWRITE,
	CRC_FLAGS_ALPHAWRITE1			= 0x02000000,
	CRC_FLAGS_ALPHAWRITE2			= 0x04000000,
	CRC_FLAGS_ALPHAWRITE3			= 0x08000000,
	CRC_FLAGS_ALPHAWRITEALL			= CRC_FLAGS_ALPHAWRITE0 | CRC_FLAGS_ALPHAWRITE1 | CRC_FLAGS_ALPHAWRITE2 | CRC_FLAGS_ALPHAWRITE3,

	// --- RASTERMODES ---
	CRC_RASTERMODE_NONE				= 0,		//  D = S,
	CRC_RASTERMODE_ALPHABLEND		= 1,		//  D = D*S.InvAlpha + S*S.Alpha,
	CRC_RASTERMODE_LIGHTMAPBLEND	= 2,		//  D = D*S.InvColor
	CRC_RASTERMODE_MULTIPLY			= 3,		//  D = D*S,
	CRC_RASTERMODE_ADD				= 4,		//  D = D + S,
	CRC_RASTERMODE_ALPHAADD			= 5,		//  D = D + S*S.Alpha,
	CRC_RASTERMODE_ALPHAMULTIPLY	= 6,		//  D = D*S.InvAlpha + S*D,
	CRC_RASTERMODE_MULTIPLY2		= 7,		//  D = D*S + S*D,
	CRC_RASTERMODE_MULADD			= 8,		//  D = D + S*D,
	CRC_RASTERMODE_ONE_ALPHA		= 9,		//  D = S + D*S.Alpha,
	CRC_RASTERMODE_ONE_INVALPHA		= 10,		//  D = S + D*S.InvAlpha,
	CRC_RASTERMODE_DESTALPHABLEND	= 11,		//  D = D*D.InvAlpha + S*D.Alpha,
	CRC_RASTERMODE_DESTADD			= 12,		//  D = D + D*S.Alpha
	CRC_RASTERMODE_MULADD_DESTALPHA = 13,		//  D = D + S*D.Alpha

	// Clear What
/*	CRC_CLEAR_COLOR					= M_Bit(0),
	CRC_CLEAR_ZBUFFER				= M_Bit(1),
	CRC_CLEAR_STENCIL				= M_Bit(2),
*/
	// _CopyType enums for RenderTarget_Copy
	CRC_COPYTYPE_NORMAL				= 0,
	CRC_COPYTYPE_DEPTHTOCOLOR		= 1,

	// --- BLENDINGMODES ---
	CRC_BLEND_ZERO				= 1,
	CRC_BLEND_ONE				= 2,
	CRC_BLEND_SRCCOLOR			= 3,
	CRC_BLEND_INVSRCCOLOR		= 4,
	CRC_BLEND_SRCALPHA			= 5,
	CRC_BLEND_INVSRCALPHA		= 6,
	CRC_BLEND_DESTALPHA			= 7,
	CRC_BLEND_INVDESTALPHA		= 8,
	CRC_BLEND_DESTCOLOR			= 9,
	CRC_BLEND_INVDESTCOLOR		= 10,
	CRC_BLEND_SRCALPHASAT		= 11,

	// --- Z/ALPHA-COMPARES ---
	CRC_COMPARE_NEVER			= 1,
	CRC_COMPARE_LESS			= 2,
	CRC_COMPARE_EQUAL			= 3,
	CRC_COMPARE_LESSEQUAL		= 4,
	CRC_COMPARE_GREATER			= 5,
	CRC_COMPARE_NOTEQUAL		= 6,
	CRC_COMPARE_GREATEREQUAL	= 7,
	CRC_COMPARE_ALWAYS			= 8,

	// --- TEXTURE-ENVIRONMENT MODES ---
	CRC_TEXENVMODE_MULTIPLY		= 0,
	CRC_TEXENVMODE_BLEND		= 1,
	CRC_TEXENVMODE_REPLACE		= 2,
	CRC_TEXENVMODE_COMBINE_ADD		= 3,			// CRC_CAPS_FLAGS_TEXENVMODE_COMBINE required (Note: Supported on GameCube)
	CRC_TEXENVMODE_COMBINE_SUBTRACT	= 4,			// CRC_CAPS_FLAGS_TEXENVMODE_COMBINE required (Note: Supported on GameCube)
	CRC_TEXENVMODE_COMBINE_DOT3		= 5,			// CRC_CAPS_FLAGS_TEXENVMODE_COMBINE required
	CRC_TEXENVMODE_COMBINE_LERP		= 6,			// CRC_CAPS_FLAGS_TEXENVMODE_COMBINE required, Src0*Src2 + Src1*(1-Src2)
	CRC_TEXENVMODE_COMBINE_MULTIPLY2 = 7,			// CRC_CAPS_FLAGS_TEXENVMODE_COMBINE required
	CRC_TEXENVMODE_COMBINE_SUBTRACTREVERSE= 8,		// CRC_CAPS_FLAGS_TEXENVMODE_COMBINE required

	// Combine default setting:
	//   Source0 = PREVIOUS
	//   Source1 = TEXTURE
	//   Source2 = CONSTANT
	//

	// Combine mode modifiers:						// CRC_CAPS_FLAGS_TEXENVMODE_COMBINE required for modifiers
	CRC_TEXENVMODE_RGB_SOURCE0_ALPHA			= 0x0080,
	CRC_TEXENVMODE_RGB_SOURCE1_ALPHA			= 0x0100,
	CRC_TEXENVMODE_RGB_SOURCE2_ALPHA			= 0x0200,
	CRC_TEXENVMODE_ALPHA_SOURCE0_COMPLEMENT		= 0x0400,
	CRC_TEXENVMODE_ALPHA_SOURCE1_COMPLEMENT		= 0x0800,
	CRC_TEXENVMODE_ALPHA_SOURCE2_COMPLEMENT		= 0x1000,
	CRC_TEXENVMODE_SOURCE1_CONSTANT				= 0x2000,
	CRC_TEXENVMODE_SOURCE1_PRIMARY				= 0x4000,
	CRC_TEXENVMODE_SOURCE2_PRIMARY				= 0x8000,

	/* The diffrent texturechannels in TEXENVMODE2
	CRC_TEXENVMODE2_NONE0		= 6,			// Out.rgb = Texture.rgb * Primary.rgb;
	CRC_TEXENVMODE2_NONE1		= 7,			// Out.rgb = Previus.rgb; Out.a = Texture.a * Primary.a;
	CRC_TEXENVMODE2_ALPHABLEND0	= 8,			// Out.rgb = Texture.rgb * Primary.rgb; Out.a = Texture.a * Primary.a;
	CRC_TEXENVMODE2_ALPHABLEND1	= 9,			// Tmp.rgb = Texture.rgb * Primary.rgb; Tmp.a = Texture.a * Primary.a;
	CRC_TEXENVMODE2_ALPHABLEND2	= 10,			// Current.rgb = Tmp.rgb*(1-Current.a) + Current.rgb*Current.a;
	CRC_TEXENVMODE2_MULADD0		= 11,			// Tmp.rgb = Texture.rgb;
	CRC_TEXENVMODE2_MULADD1		= 12,			// Out.rgb = Texture.rgb * Primary.rgb; Out.a = Texture.a * Primary.a;
	CRC_TEXENVMODE2_MULADD2		= 13,			// Out.rgb = Current.rgb + Current.rgb * Tmp.rgb; Out.a = Current.a
	CRC_TEXENVMODE2_MULTIPLY2_0	= 14,			// Out.rgb = Texture.rgb * Primary.rgb; 
	CRC_TEXENVMODE2_MULTIPLY2_1	= 15,			// Out.rgb = Texture.rgb * Current.rgb * 2; Out.a = Texture.a * Primary.a;
	*/
	CRC_TEXENVMODE2_NONE		= 12,			// Avail only if the CRC_CAPS_FLAGS_TEXENVMODE2 capability is set.
	CRC_TEXENVMODE2_ALPHABLEND	= 13,			// Avail only if the CRC_CAPS_FLAGS_TEXENVMODE2 capability is set.
	CRC_TEXENVMODE2_MULADD		= 14,			// Avail only if the CRC_CAPS_FLAGS_TEXENVMODE2 capability is set.
	CRC_TEXENVMODE2_MULTIPLY2	= 15,			// Avail only if the CRC_CAPS_FLAGS_TEXENVMODE2 capability is set.

	
	

	// --- PS2 TEXTURE ENVIRONMENT/RASTER MODES ---
	CRC_PS2_TEXENVMODE_REPLACE			= 0,	// bit 0-2 	-> TexEnvMode		
	CRC_PS2_TEXENVMODE_ADD				= 1,
	CRC_PS2_TEXENVMODE_BLEND			= 2,
	CRC_PS2_TEXENVMODE_ADDALPHA			= 3,
	CRC_PS2_TEXENVMODE_DESTADD			= 4,
	CRC_PS2_TEXENVMODE_ONEALPHA			= 5,
	CRC_PS2_TEXENVMODE_MULADD_DESTALPHA = 6,	// Dest = Dest.RGB + Source.RGB * Dest.A
	CRC_PS2_TEXENVMODE_INVISIBLE		= 7,
	
	CRC_PS2_TEXENVMODE_COLORBUFFER	= 8,		// bit 3 	-> mask/adr to FRAMEBUFFER/COLORBUFFER

	CRC_PS2_TEXENVMODE_RGBMASK		= 16,		// bit 4	-> bits 0-23 will be affected
	CRC_PS2_TEXENVMODE_ALPHAMASK	= 32,		// bit 5	-> bits 24-30 will be affected
	CRC_PS2_TEXENVMODE_STENCILMASK	= 64,		// bit 6	-> bit 31 will be affected

	CRC_PS2_TEXENVMODE_ADTEST		= 128,		// bit 7	-> alpha destination test is on
	CRC_PS2_TEXENVMODE_ASTEST		= 256,		// bit 8	-> alpha source test is on
	CRC_PS2_TEXENVMODE_ZWRITE		= 512,		// bit 9	-> zbuffer is written to

	CRC_PS2_TEXENVMODE_MODULATE		= 1024,		// bit 5	-> DECAL / MODULATE
	CRC_PS2_TEXENVMODE				= 0x8000,	// bit 15	-> TexEnvMode is a PS2 format

	// syntactic sugar
	CRC_PS2_TEXENVMODE_RGBAMASK		= CRC_PS2_TEXENVMODE_RGBMASK | CRC_PS2_TEXENVMODE_ALPHAMASK,
	CRC_PS2_TEXENVMODE_RGBASMASK	= CRC_PS2_TEXENVMODE_RGBMASK | CRC_PS2_TEXENVMODE_ALPHAMASK | CRC_PS2_TEXENVMODE_STENCILMASK,

	// --- TEXTURE-GENERATION MODES ---
	CRC_TEXGENMODE_TEXCOORD			= 0,		// TexCoord = TV
	CRC_TEXGENMODE_LINEAR			,			// VP-Mode only, TexCoord = V * M, modelspace
	CRC_TEXGENMODE_LINEARNHF		,			// VP-Mode only, TexCoord = ...
	CRC_TEXGENMODE_BOXNHF			,			// VP-Mode only, TexCoord = ...
	CRC_TEXGENMODE_VOID				,			// VP-Mode only, TexCoord is not written.
	CRC_TEXGENMODE_ENV2				,			// VP-Mode only, Q3 style dual-hemisphere-fake-env?

	CRC_TEXGENMODE_SCREEN			,			// GAMECUBE SPECIFIC TEXGENMODE.
	CRC_TEXGENMODE_SHADOWVOLUME2	,			// VP-Mode only
	CRC_TEXGENMODE_MSPOS			,			// VP-Mode only, Modelspace pixel position
	CRC_TEXGENMODE_CONSTANT			,			// VP-Mode only, TexCoord = Constant

	// Modes from here on use normal
	CRC_TEXGENMODE_NORMALMAP		,			// VP-Mode only, TexCoord = N, modelspace
	CRC_TEXGENMODE_REFLECTION		,			// VP-Mode only, TexCoord = V - 2*N*(V dot N), viewspace
	CRC_TEXGENMODE_ENV				,			// VP-Mode only, TexCoord.x = N.x, TexCoord.y = -N.y, viewspace
	CRC_TEXGENMODE_SHADOWVOLUME		,			// VP-Mode only
	CRC_TEXGENMODE_LIGHTING			,			// VP-Mode only, TexCoord.rgb = Diffuse lighting, 6 TexGenAttr
	CRC_TEXGENMODE_LIGHTING_NONORMAL,			// VP-Mode only, lighting for billboards
	CRC_TEXGENMODE_LIGHTFIELD		,

	// Modes from here on use tangents
	CRC_TEXGENMODE_BUMPCUBEENV		,			// VP-Mode only, TexCoord = <insert lots of math here>
	CRC_TEXGENMODE_TSREFLECTION		,			// VP-Mode only, TSREFLECTION = Tangent-space reflection vector, modelspace:
	CRC_TEXGENMODE_TSLV			    ,			// VP-Mode only, TSLV = Tangent-space light vector, modelspace:
                								//		TexCoord.x = (L - V) dot TangU
                								//		TexCoord.y = (L - V) dot TangV
                								//		TexCoord.z = (L - V) dot N
	CRC_TEXGENMODE_PIXELINFO		,			// VP-Mode only, Creates 4 sets of texture coordinates, Mat is typically a model-space to world transform.
                								//	this + 0: AnimatedVertexPosition * Mat
                								//	this + 1: Normal * Mat
                								//	this + 2: TangentU * Mat
                								//	this + 3: TangentV * Mat

	CRC_TEXGENMODE_DEPTHOFFSET		,			// VP-Mode only, Creates depth offseted texture coordinates
												//		TexCoord.x = TexCoord.x + EyeVecTangU * OffsetU / EyeVecN
												//		TexCoord.y = TexCoord.y + EyeVecTangV * OffsetV / EyeVecN
	CRC_TEXGENMODE_TANG_U			,			// VP-Mode only, TexCoord = TangU, Modelspace
	CRC_TEXGENMODE_TANG_V			,			// VP-Mode only, TexCoord = TangV, Modelspace
	CRC_TEXGENMODE_NULL			    ,			// VP-Mode only, TexCoord is written with 0
	CRC_TEXGENMODE_DECALTSTRANSFORM	,			// Texparam0..1 are transformed to tangent-space and used as tangents

	CRC_TEXGENMODE_MAX				,

	CRC_TEXGENMODE_FIRSTUSENORMAL = CRC_TEXGENMODE_NORMALMAP,
	CRC_TEXGENMODE_FIRSTUSETANGENT = CRC_TEXGENMODE_BUMPCUBEENV,

	CRC_TEXGENCOMP_U				= 1,
	CRC_TEXGENCOMP_V				= 2,
	CRC_TEXGENCOMP_W				= 4,
	CRC_TEXGENCOMP_Q				= 8,
	CRC_TEXGENCOMP_ALL				= 15,
	CRC_TEXGENCOMP_MASK				= 15,

	CRC_TEXSAMPLERMODE_LINEAR		= 0,
	CRC_TEXSAMPLERMODE_SRGB			= 1,

/*	CRC_TEXGENMODE_MASK			= 15,
	CRC_TEXGENMODE_SHIFT_U		= 0,
	CRC_TEXGENMODE_SHIFT_V		= 4,
	CRC_TEXGENMODE_SHIFT_W		= 8,
	CRC_TEXGENMODE_SHIFT_Q		= 12,
	CRC_TEXGENMODE_MASK_U		= (CRC_TEXGENMODE_MASK << CRC_TEXGENMODE_SHIFT_U),
	CRC_TEXGENMODE_MASK_V		= (CRC_TEXGENMODE_MASK << CRC_TEXGENMODE_SHIFT_V),
	CRC_TEXGENMODE_MASK_W		= (CRC_TEXGENMODE_MASK << CRC_TEXGENMODE_SHIFT_W),
	CRC_TEXGENMODE_MASK_Q		= (CRC_TEXGENMODE_MASK << CRC_TEXGENMODE_SHIFT_Q),*/

	// --- MATRIX MODES ---
	CRC_MATRIX_MODEL			= 0,
	CRC_MATRIX_PROJECTION		= 1,

	CRC_MATRIX_TEXTURE0			= 2, CRC_MATRIX_TEXTURE = 2,
	CRC_MATRIX_TEXTURE1			= 3,
	CRC_MATRIX_TEXTURE2			= 4,
	CRC_MATRIX_TEXTURE3			= 5,
#if	(DEF_CRC_MAXTEXTURES==4)
	CRC_MATRIX_PALETTE			= 6,			// Not in matrix stack, shift value used for update mask.
#else
	CRC_MATRIX_TEXTURE4			= 6,
	CRC_MATRIX_TEXTURE5			= 7,
	CRC_MATRIX_TEXTURE6			= 8,
	CRC_MATRIX_TEXTURE7			= 9,

	CRC_MATRIX_PALETTE			= 10,			// Not in matrix stack, shift value used for update mask.
#endif

	// --- STENCIL OPERATIONS ---
	CRC_STENCILOP_NONE			= 0,
	CRC_STENCILOP_ZERO			= 1,
	CRC_STENCILOP_REPLACE		= 2,
	CRC_STENCILOP_INC			= 3,
	CRC_STENCILOP_DEC			= 4,
	CRC_STENCILOP_INVERT		= 5,
	CRC_STENCILOP_INCWRAP		= 6,
	CRC_STENCILOP_DECWRAP		= 7,
};

//----------------------------------------------------------------
enum
{
//	CRC_ATTRCHG_RASTERMODE		= M_Bit(0),
	CRC_ATTRCHG_BLEND			= M_Bit(1),
	CRC_ATTRCHG_ZCOMPARE		= M_Bit(2),
	CRC_ATTRCHG_TEXTUREID		= M_Bit(3),
	CRC_ATTRCHG_TEXENVMODE		= M_Bit(4),
	CRC_ATTRCHG_TEXENVCOLOR		= M_Bit(5),
	CRC_ATTRCHG_ALPHACOMPARE	= M_Bit(6),
	CRC_ATTRCHG_FOG				= M_Bit(7),
	CRC_ATTRCHG_POLYGONOFFSET	= M_Bit(8),
	CRC_ATTRCHG_STENCIL			= M_Bit(9),
	CRC_ATTRCHG_TEXGEN			= M_Bit(10),
	CRC_ATTRCHG_MATRIXPALETTE	= M_Bit(11),
	CRC_ATTRCHG_MATRIX			= M_Bit(12),	// Any transform
	CRC_ATTRCHG_SCISSOR			= M_Bit(13),
	CRC_ATTRCHG_EXATTR			= M_Bit(14),
	CRC_ATTRCHG_TEXGENCOMP		= M_Bit(15),
	CRC_ATTRCHG_TEXGENATTR		= M_Bit(16),
	CRC_ATTRCHG_TEXCOORDSET		= M_Bit(17),
	CRC_ATTRCHG_LIGHTING		= M_Bit(18),
	CRC_ATTRCHG_FLAG_FOG		= M_Bit(19),
	CRC_ATTRCHG_FLAG_STENCIL	= M_Bit(20),
	CRC_ATTRCHG_FLAG_ALHPAWRITE = M_Bit(21),
	CRC_ATTRCHG_FLAG_BLEND		= M_Bit(22),
	CRC_ATTRCHG_VB_FLAGS		= M_Bit(23),
	CRC_ATTRCHG_FLAG_OTHER		= M_Bit(24),
	CRC_ATTRCHG_TEXSAMPLERMODE	= M_Bit(25),

	CRC_ATTRCHG_OTHER			= CRC_ATTRCHG_EXATTR | CRC_ATTRCHG_TEXGENCOMP | CRC_ATTRCHG_TEXGENATTR | CRC_ATTRCHG_TEXCOORDSET | CRC_ATTRCHG_LIGHTING,

	CRC_ATTRCHG_FLAGS			= CRC_ATTRCHG_FLAG_FOG | CRC_ATTRCHG_FLAG_STENCIL | CRC_ATTRCHG_FLAG_ALHPAWRITE | CRC_ATTRCHG_FLAG_BLEND | CRC_ATTRCHG_FLAG_OTHER,
};

// -------------------------------------------------------------------
//  CRC_ExtAttributes
// -------------------------------------------------------------------
enum
{
	CRC_ATTRIBTYPE_INVALID				= 0,
	CRC_ATTRIBTYPE_NV10					= 1,
	CRC_ATTRIBTYPE_NV20					= 2,

	CRC_ATTRIBTYPE_FP11					= 3,
	CRC_ATTRIBTYPE_FP14					= 4,
	CRC_ATTRIBTYPE_FP20					= 5,
	CRC_ATTRIBTYPE_FP30					= 6,
};

class SYSTEMDLLEXPORT CRC_ExtAttributes
{
public:
	int m_AttribType;
};

// -------------------------------------------------------------------
//  CRC_ExtAttributes_FragmentProgram
// -------------------------------------------------------------------
class CRC_ExtAttributes_FragmentProgram11 : public CRC_ExtAttributes
{
public:
	const char *m_pProgramName;
	CPixel32 m_Colors[16];

	void Clear()
	{
		m_AttribType = CRC_ATTRIBTYPE_FP11;
		m_pProgramName = NULL;
	}
};

class CRC_ExtAttributes_FragmentProgram14 : public CRC_ExtAttributes
{
public:
	const char *m_pProgramName;
	uint32 m_ProgramNameHash;
	CVec4Dfp32* m_pParams;
	int m_nParams;

	void Clear()
	{
		m_AttribType = CRC_ATTRIBTYPE_FP14;
		m_pProgramName = NULL;
		m_ProgramNameHash = 0;
		m_pParams = NULL;
		m_nParams = 0;
	}

	void SetProgram(const char* _pProgramName, uint32 _ProgramNameHash)
	{
		m_pProgramName = _pProgramName;
#ifndef M_RTM
		{
			uint32 Hash = StringToHash(_pProgramName);
			if(_ProgramNameHash && Hash != _ProgramNameHash)
			{
				LogFile(CStrF("Program '%s' added with wrong HASH (0x%.8X vs 0x%.8X), this will cause bugs!!!!!", _pProgramName, _ProgramNameHash, Hash));
				_ProgramNameHash = Hash;
			}
		}
#endif
		m_ProgramNameHash = _ProgramNameHash;
	}

	void SetParameters(CVec4Dfp32* _pParams, int _nParams)
	{
		m_pParams = _pParams;
		m_nParams = _nParams;
	}
};

class CRC_ExtAttributes_FragmentProgram20 : public CRC_ExtAttributes_FragmentProgram14
{
public:
	void Clear()
	{
		CRC_ExtAttributes_FragmentProgram14::Clear();
		m_AttribType = CRC_ATTRIBTYPE_FP20;
	}
};

class CRC_ExtAttributes_FragmentProgram30 : public CRC_ExtAttributes_FragmentProgram14
{
public:
	void Clear()
	{
		CRC_ExtAttributes_FragmentProgram14::Clear();
		m_AttribType = CRC_ATTRIBTYPE_FP30;
	}
};


//----------------------------------------------------------------
class M_ALIGN(16) SYSTEMDLLEXPORT CRC_Attributes	// 0xa0 bytes, this shit needs shrinking
{
	struct CRasterModeBlend
	{
		uint8 m_SrcBlend, m_DstBlend;
	};

	static CRasterModeBlend ms_lRasterModeBlend[];

	class CAttributeCheck { public: CAttributeCheck(); };
	static CAttributeCheck ms_AttribCheck;

	class CAttributeDefaultInit { public: CAttributeDefaultInit(); };
	static CAttributeDefaultInit ms_AttribDefaultInit;
	static CRC_Attributes M_ALIGN(16) ms_AttribDefault;

public:
	union
	{
		struct
		{
			CRC_ExtAttributes* m_pExtAttrib;
			const CRC_Light* m_pLights;

			uint16 m_TextureID[CRC_MAXTEXTURES];
			// Texture-state
			uint16 m_TexSamplerMode;
			uint16 m_nLights;
		#if	DEF_CRC_MAXTEXTURES == 4
			uint16 m_TexGenComp;																// 4-bit per texture coord, 4 * CRC_MAXTEXTURES = 16
		#else
			uint32 m_TexGenComp : (CRC_NUMTEXCOORDCOMP*CRC_MAXTEXCOORDS);						// 4-bit per texture coord, 4 * CRC_MAXTEXCOORDS = 32
		#endif
			uint8 m_lTexGenMode[CRC_MAXTEXCOORDS];
			uint8 m_iTexCoordSet[CRC_MAXTEXCOORDS];
		#ifdef PLATFORM_PS2
			uint16 m_TexEnvMode[CRC_MAXTEXTUREENV];
		#else
			uint16 m_TexEnvMode[CRC_MAXTEXTUREENV];
			CPixel32* m_pTexEnvColors;
		#endif
		//	fp32* m_pTexGenParams[CRC_MAXTEXTURES][CRC_NUMTEXCOORDCOMP];
			fp32* m_pTexGenAttr;
			uint32 m_AttribLock;
			uint32 m_AttribLockFlags;

			uint32 m_VBFlags;

			uint32 m_Flags;
			uint8 m_ZCompare;
			uint8 m_AlphaCompare;
			uint16 m_AlphaRef;

			fp32 m_PolygonOffsetScale;
			fp32 m_PolygonOffsetUnits;

		#ifndef PLATFORM_PS2
		//	CRect2Duint16 m_Scissor;
			CScissorRect m_Scissor;

			// Stencil: 32bit + 16bit
			union
			{
				struct
				{
					uint8 m_StencilFrontFunc : 4;
					uint8 m_StencilFrontOpFail : 4;
					uint8 m_StencilFrontOpZFail : 4;
					uint8 m_StencilFrontOpZPass : 4;
					uint8 m_StencilBackFunc : 4;
					uint8 m_StencilBackOpFail : 4;
					uint8 m_StencilBackOpZFail : 4;
					uint8 m_StencilBackOpZPass : 4;
				};
				uint32 m_StencilDWord1;
			};
			uint8 m_StencilRef;
			uint8 m_StencilFuncAnd;
			uint8 m_StencilWriteMask;

		#endif

			CPixel32Aggr m_FogColor;
			fp32 m_FogStart;
			fp32 m_FogEnd;
			fp32 m_FogDensity;

		#ifdef PLATFORM_PS2
			uint8 m_nChannels;
		#endif

			union
			{
				struct
				{
					uint8 m_SourceBlend;
					uint8 m_DestBlend;
				};
				uint16 m_SourceDestBlend;
			};
		};

		struct
		{
			vec128 m_v128[0x0a];
		};
	};


	CRC_Attributes();
	void Clear();
	void SetDefault();		// Copies from ms_AttribDefault

	void SetDefaultReal();	// Initializer for ms_AttribDefault

	M_FORCEINLINE const CRC_Attributes& operator= (const CRC_Attributes& _Src)
	{
		if((mint(&_Src) & 0xf) || (mint(this) & 0xf))
			M_BREAKPOINT;
		vec128 v0 = _Src.m_v128[0];
		vec128 v1 = _Src.m_v128[1];
		vec128 v2 = _Src.m_v128[2];
		vec128 v3 = _Src.m_v128[3];
		vec128 v4 = _Src.m_v128[4];
		vec128 v5 = _Src.m_v128[5];
		vec128 v6 = _Src.m_v128[6];
		vec128 v7 = _Src.m_v128[7];
		vec128 v8 = _Src.m_v128[8];
		vec128 v9 = _Src.m_v128[9];
		m_v128[0] = v0;
		m_v128[1] = v1;
		m_v128[2] = v2;
		m_v128[3] = v3;
		m_v128[4] = v4;
		m_v128[5] = v5;
		m_v128[6] = v6;
		m_v128[7] = v7;
		m_v128[8] = v8;
		m_v128[9] = v9;
		return *this;
	}

	void ClearRasterModeSettings();

	int Compare(const CRC_Attributes& _Attr) const;

	// Attributes
	M_FORCEINLINE void Attrib_Lock(int _Flags)										{ m_AttribLock = _Flags; };
	M_FORCEINLINE void Attrib_LockFlags(int _Flags)									{ m_AttribLockFlags = _Flags; };

	M_FORCEINLINE void Attrib_Default()												{ m_Flags = CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_COLORWRITE | CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE | CRC_FLAGS_SMOOTH | CRC_FLAGS_PERSPECTIVE; }
	M_FORCEINLINE void Attrib_Enable(int _Flags)									{ m_Flags |= _Flags; };
	M_FORCEINLINE void Attrib_Disable(int _Flags)									{ m_Flags &= ~_Flags; };
	M_FORCEINLINE void Attrib_Switch(int _Flags)									{ m_Flags ^= _Flags; };
	M_FORCEINLINE void Attrib_ZCompare(int _Compare)								{ m_ZCompare = _Compare; };
	M_FORCEINLINE void Attrib_AlphaCompare(int _Compare, int _AlphaRef)				{ m_AlphaCompare = _Compare; m_AlphaRef = _AlphaRef; };

#ifndef PLATFORM_PS2
	M_FORCEINLINE void Attrib_StencilWriteMask(int _Mask)							{ m_StencilWriteMask = _Mask; };
	M_FORCEINLINE void Attrib_StencilRef(int _Ref, int _FuncAnd)					{ m_StencilRef = _Ref; m_StencilFuncAnd = _FuncAnd; };
	M_FORCEINLINE void Attrib_StencilFrontOp(int _Func, int _OpFail, int _OpZFail, int _OpZPass) { m_StencilFrontFunc = _Func; m_StencilFrontOpFail = _OpFail; m_StencilFrontOpZFail = _OpZFail; m_StencilFrontOpZPass = _OpZPass; };
	M_FORCEINLINE void Attrib_StencilBackOp(int _Func, int _OpFail, int _OpZFail, int _OpZPass) { m_StencilBackFunc = _Func; m_StencilBackOpFail = _OpFail; m_StencilBackOpZFail = _OpZFail; m_StencilBackOpZPass = _OpZPass; };
#endif	// PLATFORM_PS2

#ifndef PLATFORM_CONSOLE
	void Attrib_RasterMode(uint _Mode);
	static uint GetSourceDestBlend(uint _RasterMode);
#else
	M_FORCEINLINE void Attrib_RasterMode(uint _Mode)
	{
		m_SourceDestBlend = MAKE_SOURCEDEST_BLEND(ms_lRasterModeBlend[_Mode].m_SrcBlend, ms_lRasterModeBlend[_Mode].m_DstBlend);
		m_Flags |= CRC_FLAGS_BLEND;
	}

	M_FORCEINLINE static uint GetSourceDestBlend(uint _RasterMode) { return MAKE_SOURCEDEST_BLEND(ms_lRasterModeBlend[_RasterMode].m_SrcBlend, ms_lRasterModeBlend[_RasterMode].m_DstBlend); };
#endif

	M_FORCEINLINE void Attrib_SourceBlend(int _Blend)								{ m_SourceBlend = _Blend; };
	M_FORCEINLINE void Attrib_DestBlend(int _Blend)									{ m_DestBlend = _Blend; };
	M_FORCEINLINE void Attrib_FogColor(CPixel32 _FogColor)							{ m_FogColor = _FogColor; };
	M_FORCEINLINE void Attrib_FogStart(fp32 _FogStart)								{ m_FogStart = _FogStart; };
	M_FORCEINLINE void Attrib_FogEnd(fp32 _FogEnd)									{ m_FogEnd = _FogEnd; };
	M_FORCEINLINE void Attrib_FogDensity(fp32 _FogDensity)							{ m_FogDensity = _FogDensity; };
	M_FORCEINLINE void Attrib_PolygonOffset(fp32 _Scale, fp32 _Offset)				{ m_PolygonOffsetScale = _Scale; m_PolygonOffsetUnits = _Offset; m_Flags |= CRC_FLAGS_POLYGONOFFSET; };

	M_FORCEINLINE void Attrib_Lights(const CRC_Light* _pLights, int _nLights)		{ m_pLights = _pLights; m_nLights = _nLights; };

	// Texture-state
	M_FORCEINLINE void Attrib_TextureID(int _iTxt, int _TextureID)					{ m_TextureID[_iTxt] = _TextureID; };
	M_FORCEINLINE void Attrib_TexCoordSet(int _iTxt, int _iTexCoordSet)				{ m_iTexCoordSet[_iTxt] = _iTexCoordSet; };
	M_FORCEINLINE void Attrib_TexEnvMode(int _iTxt, int _TexEnvMode)				{ m_TexEnvMode[_iTxt] = _TexEnvMode; };
//	M_FORCEINLINE void Attrib_TexEnvColor(int _iTxt, CPixel32 _TexEnvColor)			{ m_TexEnvColor[_iTxt] = _TexEnvColor; };
	M_FORCEINLINE void Attrib_TexEnvColors(CPixel32* _pTexEnvColors)				{ m_pTexEnvColors = _pTexEnvColors; };

	M_FORCEINLINE void Attrib_TexSamplerMode(int _iTxt, int _SamplerMode)			{ m_TexSamplerMode = (m_TexSamplerMode & ~(M_BitD(_iTxt))) | (_SamplerMode << _iTxt); };
	M_FORCEINLINE void Attrib_TexSamplerModeAll(int _SamplerMode)					{ m_TexSamplerMode = (_SamplerMode == CRC_TEXSAMPLERMODE_SRGB) ? 0xffff : 0; };
	M_FORCEINLINE int GetTexSamplerMode(int _iTxt)									{ return (m_TexSamplerMode >> _iTxt) & 1; };

	M_FORCEINLINE void Attrib_TexGen(int _iTxt, int _TexGen, int _CompMask)			{ m_TexGenComp = (m_TexGenComp & ~(CRC_TEXGENCOMP_MASK << (_iTxt * CRC_NUMTEXCOORDCOMP))) | (_CompMask << (_iTxt * CRC_NUMTEXCOORDCOMP)); m_lTexGenMode[_iTxt] = _TexGen; };
	M_FORCEINLINE void Attrib_TexGenAttr(fp32* _pTexGenAttr)						{ m_pTexGenAttr = _pTexGenAttr; };

	M_FORCEINLINE void Attrib_VBFlags(uint32 _Flags)								{ m_VBFlags = _Flags; }

	M_FORCEINLINE int GetTexGenComp(int _iTxt) const								{ return (m_TexGenComp >> (_iTxt * 4)) & CRC_TEXGENCOMP_MASK; };

	static int GetTexGenModeAttribSize(int _TexGenMode, int _TexGenComp);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Vertex registers / format enums
|__________________________________________________________________________________________________
\*************************************************************************************************/

enum
{


	// Scalable
	CRC_VREG_POS			= 0,
	CRC_VREG_TEXCOORD0		= 1,
	CRC_VREG_TEXCOORD1		= 2,
	CRC_VREG_TEXCOORD2		= 3,
	CRC_VREG_TEXCOORD3		= 4,

	CRC_MAXVERTEXREGSCALE,

	// NonScalable
	CRC_VREG_TEXCOORD4		= 5,
	CRC_VREG_TEXCOORD5		= 6,
	CRC_VREG_TEXCOORD6		= 7,
	CRC_VREG_TEXCOORD7		= 8,

	CRC_VREG_NORMAL			= 9,
	CRC_VREG_COLOR			= 10,
	CRC_VREG_SPECULAR		= 11,
	CRC_VREG_MI0			= 12,
	CRC_VREG_MW0			= 13,
	CRC_VREG_MI1			= 14,
	CRC_VREG_MW1			= 15,

	CRC_MAXVERTEXREG,


	CRC_VREGFMT_VOID = 0,
	CRC_VREGFMT_V1_F32,			// -_FP32_MAX.._FP32_MAX, 32-bits, 32
	CRC_VREGFMT_V2_F32,			// -_FP32_MAX.._FP32_MAX, 64-bits, 32,32
	CRC_VREGFMT_V3_F32,			// -_FP32_MAX.._FP32_MAX, 96-bits, 32,32,32
	CRC_VREGFMT_V4_F32,			// -_FP32_MAX.._FP32_MAX, 128-bits, 32,32,32,32
	CRC_VREGFMT_V1_I16,			// -32768..32767, 16-bits
	CRC_VREGFMT_V2_I16,			// -32768..32767, 32-bits (16,16)
	CRC_VREGFMT_V3_I16,			// -32768..32767, 48-bits (16,16,16)
	CRC_VREGFMT_V4_I16,			// -32768..32767, 64-bits (16,16,16,16)
	CRC_VREGFMT_V1_U16,			// 0..65536, 16-bits
	CRC_VREGFMT_V2_U16,			// 0..65536, 32-bits (16,16)
	CRC_VREGFMT_V3_U16,			// 0..65536, 48-bits (16,16,16)
	CRC_VREGFMT_V4_U16,			// 0..65536, 64-bits (16,16,16,16)
	CRC_VREGFMT_NS3_I16,		// Normalized signed, -1..1, 48-bits (16,16,16)
	CRC_VREGFMT_NS3_P32,		// Normalized signed, -1..1, 32-bits (11,11,10)
	CRC_VREGFMT_NU3_P32,		// Normalized unsigned, 0..1, 32-bits (11,11,10)
	CRC_VREGFMT_N4_UI8_P32_NORM,		// Normalized, 0..1, 32-bit, (8,8,8,8)
	CRC_VREGFMT_N4_UI8_P32,		// Normalized, 0..255, 32-bit, (8,8,8,8)
	CRC_VREGFMT_N4_COL,			// Normalized, 0..1, 32-bits, (8,8,8,8), BGRA byte order.

	CRC_VREGFMT_U3_P32,			// Unsigned, 0..2048/1024, 32-bits (11,11,10)

	CRC_VREGFMT_NS1_I16,		// Normalized signed, -1..1, 16-bits (16)
	CRC_VREGFMT_NS2_I16,		// Normalized signed, -1..1, 32-bits (16,16)
	CRC_VREGFMT_NS4_I16,		// Normalized signed, -1..1, 64-bits (16,16,16,16)

	CRC_VREGFMT_NU1_I16,		// Normalized signed, 0..1, 16-bits (16)
	CRC_VREGFMT_NU2_I16,		// Normalized signed, 0..1, 32-bits (16,16)
	CRC_VREGFMT_NU3_I16,		// Normalized signed, 0..1, 48-bits (16,16,16)
	CRC_VREGFMT_NU4_I16,		// Normalized signed, 0..1, 64-bits (16,16,16,16)

//	CRC_VREGFMT_V8_F32,				// Like ..N8.. but marks dual lists
//	CRC_VREGFMT_N8_UI8_P32_NORM,	// Like ..N4.. but marks dual lists

	CRC_VREGFMT_MAX,
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CRC_VertexFormat
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CRC_VRegTransform;
class CRC_VertexFormat
{
public:
	uint8 m_lVRegFormats[CRC_MAXVERTEXREG];
	static uint8 ms_lVRegFormatSizes[CRC_VREGFMT_MAX];
	static uint8 ms_lVRegFormatComp[CRC_VREGFMT_MAX];
	static const char* ms_lVRegFormatName[CRC_VREGFMT_MAX];

	void Clear()
	{
		FillChar(this, sizeof(CRC_VertexFormat), 0);
	}

	M_INLINE int GetFormat(int _iVReg) const { return m_lVRegFormats[_iVReg]; };
	M_INLINE void SetFormat(int _iVReg, int _Format) { m_lVRegFormats[_iVReg] = _Format; };

	int GetStride() const;

	static int GetRegisterSize(uint32 _Format);
	static int GetRegisterComponents(uint32 _Format);
	static const char* GetRegisterName(uint32 _Format);

	static void ConvertRegisterFormat(
		const void* _pSrc, int _SrcFmt, int _SrcStride, const CRC_VRegTransform* _pSrcScale, 
		void* _pDst, int _DstFmt, int _DstStride, const CRC_VRegTransform* _pDstScale, int _nV);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CRC_BuildVertexBuffer
|__________________________________________________________________________________________________
\*************************************************************************************************/
#define CRC_BVB_SETVREG(Name, Type, VReg, VRegFormat)	\
void Geometry_##Name(Type* _p) { m_lpVReg[VReg] = _p; m_Format.SetFormat(VReg, VRegFormat); m_WantFormat.SetFormat(VReg, VRegFormat); }

#define CRC_BVB_SETVREG_TEX(Name, Type, VReg, VRegFormat)	\
void Geometry_##Name(Type* _p, int _iTxtChannel) { m_lpVReg[VReg+_iTxtChannel] = _p; m_Format.SetFormat(VReg+_iTxtChannel, VRegFormat); m_WantFormat.SetFormat(VReg+_iTxtChannel, VRegFormat); }

class CRC_VRegTransform
{
public:
	CVec4Dfp32 m_Scale;
	CVec4Dfp32 m_Offset;
};

class SYSTEMDLLEXPORT CRC_BuildVertexBuffer
{
public:

	CRC_BuildVertexBuffer()
	{
		Clear();
	}

	void* m_lpVReg[CRC_MAXVERTEXREG];
	CRC_VRegTransform m_lTransform[CRC_MAXVERTEXREGSCALE];
	CRC_VRegTransform m_lWantTransform[CRC_MAXVERTEXREGSCALE];
	uint32 m_TransformEnable;
	uint32 m_WantTransformEnable;
	CRC_VertexFormat m_Format;
	CRC_VertexFormat m_WantFormat;

	int m_nV;
	uint16* m_piPrim;
	uint16 m_nPrim;
	int16 m_PrimType;

	void Geometry_SetData(int _Register, int _Format, void *_Data)
	{
		m_WantFormat.SetFormat(_Register, _Format);
		m_lpVReg[_Register] = _Data;
	}

	void Geometry_SetWantFormat(int _Register, int _Format)
	{
		m_WantFormat.SetFormat(_Register, _Format);
	}

	void Geometry_SetTransform(int _Register, CRC_VRegTransform &_Transform)
	{
		M_ASSERT(_Register < CRC_MAXVERTEXREGSCALE, "Overflow");
		m_lTransform[_Register] = _Transform;
		m_TransformEnable |= 1 << _Register;
	}

	void Geometry_SetWantTransform(int _Register, CRC_VRegTransform &_Transform)
	{
		M_ASSERT(_Register < CRC_MAXVERTEXREGSCALE, "Overflow");
		m_lWantTransform[_Register] = _Transform;
		m_WantTransformEnable |= 1 << _Register;
	}

	void Clear()
	{
		FillChar(this, sizeof(CRC_BuildVertexBuffer), 0);
	}

	aint GetStride()
	{
		return m_Format.GetStride();
	}

	aint GetWantStride()
	{
		return m_WantFormat.GetStride();
	}

	CRC_BVB_SETVREG(VertexArray, CVec3Dfp32, CRC_VREG_POS, CRC_VREGFMT_V3_F32);
	CRC_BVB_SETVREG(NormalArray, CVec3Dfp32, CRC_VREG_NORMAL, CRC_VREGFMT_V3_F32);
	CRC_BVB_SETVREG(ColorArray, CPixel32, CRC_VREG_COLOR, CRC_VREGFMT_N4_COL);
	CRC_BVB_SETVREG(ColorArray, uint32, CRC_VREG_COLOR, CRC_VREGFMT_N4_COL);
	CRC_BVB_SETVREG(ColorArray, int32, CRC_VREG_COLOR, CRC_VREGFMT_N4_COL);
	CRC_BVB_SETVREG(SpecularArray, CVec3Dfp32, CRC_VREG_SPECULAR, CRC_VREGFMT_N4_COL);
	CRC_BVB_SETVREG(MatrixIndex0, uint32, CRC_VREG_MI0, CRC_VREGFMT_N4_UI8_P32_NORM);
	CRC_BVB_SETVREG(MatrixIndex0Int, uint32, CRC_VREG_MI0, CRC_VREGFMT_N4_UI8_P32);
	CRC_BVB_SETVREG(MatrixIndex1, uint32, CRC_VREG_MI1, CRC_VREGFMT_N4_UI8_P32_NORM);
	CRC_BVB_SETVREG(MatrixIndex1Int, uint32, CRC_VREG_MI1, CRC_VREGFMT_N4_UI8_P32);
	CRC_BVB_SETVREG(MatrixWeight0, fp32, CRC_VREG_MW0, CRC_VREGFMT_V1_F32);
	CRC_BVB_SETVREG(MatrixWeight0, CVec2Dfp32, CRC_VREG_MW0, CRC_VREGFMT_V2_F32);
	CRC_BVB_SETVREG(MatrixWeight0, CVec3Dfp32, CRC_VREG_MW0, CRC_VREGFMT_V3_F32);
	CRC_BVB_SETVREG(MatrixWeight0, CVec4Dfp32, CRC_VREG_MW0, CRC_VREGFMT_V4_F32);
	CRC_BVB_SETVREG(MatrixWeight1, fp32, CRC_VREG_MW1, CRC_VREGFMT_V1_F32);
	CRC_BVB_SETVREG(MatrixWeight1, CVec2Dfp32, CRC_VREG_MW1, CRC_VREGFMT_V2_F32);
	CRC_BVB_SETVREG(MatrixWeight1, CVec3Dfp32, CRC_VREG_MW1, CRC_VREGFMT_V3_F32);
	CRC_BVB_SETVREG(MatrixWeight1, CVec4Dfp32, CRC_VREG_MW1, CRC_VREGFMT_V4_F32);
	CRC_BVB_SETVREG_TEX(TVertexArray, fp32, CRC_VREG_TEXCOORD0, CRC_VREGFMT_V1_F32);
	CRC_BVB_SETVREG_TEX(TVertexArray, CVec2Dfp32, CRC_VREG_TEXCOORD0, CRC_VREGFMT_V2_F32);
	CRC_BVB_SETVREG_TEX(TVertexArray, CVec3Dfp32, CRC_VREG_TEXCOORD0, CRC_VREGFMT_V3_F32);
	CRC_BVB_SETVREG_TEX(TVertexArray, CVec4Dfp32, CRC_VREG_TEXCOORD0, CRC_VREGFMT_V4_F32);

//	CRC_BVB_SETVREG(MatrixIndex2, uint32, CRC_VREG_TEXCOORD6, CRC_VREGFMT_N4_UI8_P32_NORM);
//	CRC_BVB_SETVREG(MatrixWeight2, CVec4Dfp32, CRC_VREG_TEXCOORD7, CRC_VREGFMT_V4_F32);

	void ConvertToInterleaved(void* _pDst, const CRC_VertexFormat& _DstFmt, CRC_VRegTransform* _pDstScale, uint32 _DestTransformEnable, int _nV);

	mint CRC_VertexBuffer_GetSize() const;
	void CRC_VertexBuffer_ConvertTo(void* _pDst, CRC_VertexBuffer& _DstBuffer) const;
};

/***************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
|  CRC_Statistics
|___________________________________________________________________________________________________
\***************************************************************************************************/

template <typename t_CData, typename t_CDataIn, int t_nHistory>
class CRC_StatCounter
{
public:
	t_CData m_LastValue;
	t_CData m_AccValue;
	t_CData m_Average;
	t_CData m_History[t_nHistory];
	int m_iCurrentHistory;
	CRC_StatCounter()
	{
		m_LastValue = 0;
		m_Average = 0;
		m_AccValue = 0;
		for (int i = 0; i < t_nHistory; ++i)
			m_History[i] = 0;
		m_iCurrentHistory = 0;
	}

	void AddData(t_CDataIn _Value)
	{
		m_LastValue = _Value;

		++m_iCurrentHistory;
		if (m_iCurrentHistory == t_nHistory)
		{
			m_iCurrentHistory = 0;
		}

		m_AccValue -= m_History[m_iCurrentHistory];
		m_AccValue += _Value;
		m_History[m_iCurrentHistory] = _Value;
		m_Average = m_AccValue / t_CData(t_nHistory);
	}

	void FillHistory(fp32 *_pHistory, int _nMax)
	{
		int iCurrent = m_iCurrentHistory;
		for (int i = Min(_nMax, t_nHistory) - 1; i >= 0; --i)
		{
			_pHistory[i] = m_History[iCurrent];

			--iCurrent;
            if (iCurrent < 0)            
				iCurrent = t_nHistory - 1;
		}
	}

	void FillHistoryRelation(const CRC_StatCounter<t_CData, t_CDataIn, t_nHistory> &_Other, fp32 *_pHistory, int _nMax)
	{
		int iCurrent = m_iCurrentHistory;
		int iCurrent1 = _Other.m_iCurrentHistory;
		for (int i = Min(_nMax, t_nHistory) - 1; i >= 0; --i)
		{
			_pHistory[i] = 100.0 - (m_History[iCurrent] / _Other.m_History[iCurrent1]) * 100.0;

			--iCurrent1;
            if (iCurrent1 < 0)            
				iCurrent1 = t_nHistory - 1;

			--iCurrent;
            if (iCurrent < 0)            
				iCurrent = t_nHistory - 1;
		}
	}

	void FillHistorySubRelationInv(const CRC_StatCounter<t_CData, t_CDataIn, t_nHistory> &_Other, const CRC_StatCounter<t_CData, t_CDataIn, t_nHistory> &_OtherSub, fp32 *_pHistory, int _nMax)
	{
		int iCurrent = m_iCurrentHistory;
		int iCurrent1 = _Other.m_iCurrentHistory;
		int iCurrent2 = _OtherSub.m_iCurrentHistory;
		for (int i = Min(_nMax, t_nHistory) - 1; i >= 0; --i)
		{
			_pHistory[i] = 1.0/(m_History[iCurrent] - _OtherSub.m_History[iCurrent2] - m_History[iCurrent] * (100.0 - _Other.m_History[iCurrent1]) * 0.01);

			--iCurrent1;
            if (iCurrent1 < 0)            
				iCurrent1 = t_nHistory - 1;
			--iCurrent2;
            if (iCurrent2 < 0)            
				iCurrent2 = t_nHistory - 1;

			--iCurrent;
            if (iCurrent < 0)            
				iCurrent = t_nHistory - 1;
		}
	}

	void FillHistoryRelationInv(const CRC_StatCounter<t_CData, t_CDataIn, t_nHistory> &_Other, fp32 *_pHistory, int _nMax)
	{
		int iCurrent = m_iCurrentHistory;
		int iCurrent1 = _Other.m_iCurrentHistory;
		for (int i = Min(_nMax, t_nHistory) - 1; i >= 0; --i)
		{
			_pHistory[i] = 1.0/(m_History[iCurrent] - m_History[iCurrent] * (100.0 - _Other.m_History[iCurrent1]) * 0.01);

			--iCurrent1;
            if (iCurrent1 < 0)            
				iCurrent1 = t_nHistory - 1;

			--iCurrent;
            if (iCurrent < 0)            
				iCurrent = t_nHistory - 1;
		}
	}

	void FillHistoryRelationSub(const CRC_StatCounter<t_CData, t_CDataIn, t_nHistory> &_Other, const CRC_StatCounter<t_CData, t_CDataIn, t_nHistory> &_OtherSub, fp32 *_pHistory, int _nMax)
	{
		int iCurrent = m_iCurrentHistory;
		int iCurrent1 = _Other.m_iCurrentHistory;
		int iCurrent2 = _OtherSub.m_iCurrentHistory;
		for (int i = Min(_nMax, t_nHistory) - 1; i >= 0; --i)
		{
			_pHistory[i] = 100.0 - ((((m_History[iCurrent]) / _Other.m_History[iCurrent1]) * 100.0 + (100.0 - _OtherSub.m_History[iCurrent2])));

			--iCurrent1;
            if (iCurrent1 < 0)            
				iCurrent1 = t_nHistory - 1;
			--iCurrent2;
            if (iCurrent2 < 0)            
				iCurrent2 = t_nHistory - 1;

			--iCurrent;
            if (iCurrent < 0)            
				iCurrent = t_nHistory - 1;
		}
	}

	void FillHistoryInv(fp32 *_pHistory, int _nMax)
	{
		int iCurrent = m_iCurrentHistory;
		for (int i = Min(_nMax, t_nHistory) - 1; i >= 0; --i)
		{
			_pHistory[i] = 1.0/m_History[iCurrent];

			--iCurrent;
            if (iCurrent < 0)            
				iCurrent = t_nHistory - 1;
		}
	}

	void FillHistorySubInv(const CRC_StatCounter<t_CData, t_CDataIn, t_nHistory> &_Other, fp32 *_pHistory, int _nMax)
	{
		int iCurrent = m_iCurrentHistory;
		int iCurrent1 = _Other.m_iCurrentHistory;
		for (int i = Min(_nMax, t_nHistory) - 1; i >= 0; --i)
		{
			_pHistory[i] = 1.0/(m_History[iCurrent] - _Other.m_History[iCurrent1]);

			--iCurrent;
            if (iCurrent < 0)            
				iCurrent = t_nHistory - 1;

			--iCurrent1;
            if (iCurrent1 < 0)            
				iCurrent1 = t_nHistory - 1;
		}
	}

	void FillHistoryComplemnt(fp32 *_pHistory, int _nMax)
	{
		int iCurrent = m_iCurrentHistory;
		for (int i = Min(_nMax, t_nHistory) - 1; i >= 0; --i)
		{
			_pHistory[i] = 100.0 - m_History[iCurrent];

			--iCurrent;
            if (iCurrent < 0)            
				iCurrent = t_nHistory - 1;
		}
	}
};

class CRC_Statistics
{
public:
	CRC_StatCounter<fp64, fp32, 60> m_GPUUsage; // The fraction of the frame time the GPU did not spend waiting for the CPU
	CRC_StatCounter<fp64, fp32, 60> m_CPUUsage; // The fraction of the frame time the CPU did not spend waiting for the GPU
	CRC_StatCounter<fp64, fp32, 60> m_Time_FrameTime; // The frametime in seconds
	CRC_StatCounter<fp64, fp32, 60> m_BlockTime; // Should be avoidable (only includes blocking that stops the CPU until all GPU activity is finished)
	CRC_StatCounter<int64, int64, 60> m_nPixlesZPass;
	CRC_StatCounter<int64, int64, 60> m_nPixlesTotal;
};

class CRC_TextureMemoryUsage
{
public:
	CRC_TextureMemoryUsage()
	{
		m_Theoretical = 1;
		m_WorstCase = 1;
		m_BestCase = 1;
		m_pFormat = "";
	}
	const ch8 *m_pFormat;
	mint m_Theoretical;
	mint m_WorstCase;
	mint m_BestCase;
};


#endif // _INC_MRender_Classes
