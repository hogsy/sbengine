#ifndef _INC_MRndrGL
#define _INC_MRndrGL

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Render context implementation for OpenGL 1.1

	Author:			Magnus Högdahl

	Copyright:		Copyright 1996-2002 Starbreeze Studios AB
					
	History:		
		9610xx:		Created File
\*____________________________________________________________________________________________*/


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Includes
|__________________________________________________________________________________________________
\*************************************************************************************************/
#include "MGLGlobalFunctions.h"
#include "MRenderPS3_Image.h"
#include "MRenderPS3_Def.h"
#include "../../MSystem/Raster/MRCCore.h"

#include "../../Classes/Render/MRenderVPGen.h"


#ifdef _DEBUG
#define CRCGL_VADEBUG_ENABLE
#endif
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Classes
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum
{
	CRCGL_TEXTUREBLOCKSIZE = 256*4,
	CRCGL_TEXTUREBLOCKSHIFT = 8+2,
	CRCGL_TEXTUREBLOCKAND = (CRCGL_TEXTUREBLOCKSIZE - 1),

	CRCGL_DEFAULTVAMINVERTEXCOUNT = 25,				// That's the number of vertices on a sorcery landscape tile.

	CRCGLES_MAX_TEXTURESIZE	= 2048,

	CRCGLES_MAX_VPPROGRAMCONSTANTS = 256,
};

// -------------------------------------------------------------------
//  Flags for gl_enable/gl_disable
// -------------------------------------------------------------------

enum
{
	CRCGL_EXT_MULTITEXTURE					= 0x00000001,
	CRCGL_EXT_NV_COPYDEPTHTOCOLOR			= 0x00000002,
	CRCGL_EXT_WINDOWPOS						= 0x00000004,
	CRCGL_ARB_TEXTURE_NON_POWER_OF_TWO		= 0x00000008,
	CRCGL_EXT_BGRA							= 0x00000010,
	CRCGL_EXT_SECONDARYCOLOR				= 0x00000020,
	CRCGL_EXT_TEXTURE_COMPRESSION_3DC		= 0x00000040,
	CRCGL_EXT_TEXTURECOMPRESSION			= 0x00000080,
	CRCGL_SHARPENMIPMAPS					= 0x00000100,
	CRCGL_ARB_PBUFFER						= 0x00000200,
	CRCGL_EXT_NV_REGCOMBINERS				= 0x00000400,
	CRCGL_EXT_CUBEMAP						= 0x00000800,
	CRCGL_EXT_S3TC							= 0x00001000,
	CRCGL_EXT_ANISOTROPIC					= 0x00002000,
	CRCGL_EXT_NV_VERTEXPROGRAM				= 0x00004000,
	CRCGL_EXT_NV_TEXTURESHADER				= 0x00008000,
	CRCGL_EXT_NV_OCCLUSIONQUERY				= 0x00010000,
	CRCGL_RESIDENTVB						= 0x00020000,
	CRCGL_EXT_TEXTURE_LOD_BIAS				= 0x00040000,
	CRCGL_EXT_VERTEXPROGRAM					= 0x00080000,
//	CRCGL_EXT_ATI_VERTEXARRAYOBJECT			= 0x00100000,
//	CRCGL_EXT_ATI_VERTEXATTRIBARRAYOBJECT	= 0x00200000,
	CRCGL_ARB_TEXTURE_ENV_DOT3				= 0x00400000,
	CRCGL_EXT_NV_REGCOMBINERS2				= 0x00800000,
	CRCGL_EXT_FRAGMENTPROGRAM				= 0x01000000,
	CRCGL_EXT_ATI_SEPARATESTENCIL			= 0x02000000,
	CRCGL_EXT_STENCILTWOSIDE				= 0x04000000,
	CRCGL_EXT_ATI_FRAGMENTSHADER			= 0x08000000,
	CRCGL_EXT_VERTEXBUFFEROBJECT			= 0x10000000,
	CRCGL_EXT_NV_FRAGMENT_PROGRAM			= 0x20000000,
	CRCGL_EXT_NV_FRAGMENT_PROGRAM2			= 0x40000000,
	CRCGL_EXT_FRAMEBUFFER_OBJECT			= 0x80000000,

	CRCGL_EXT_ANYVERTEXPROGRAM				= CRCGL_EXT_VERTEXPROGRAM + CRCGL_EXT_NV_VERTEXPROGRAM,
};

// -------------------------------------------------------------------
//  Internal vertex-array identifiers
// -------------------------------------------------------------------
enum
{
	CRCGL_VA_VERTEX			= DBit(0),
	CRCGL_VA_MATRIXWEIGHT	= DBit(1),
	CRCGL_VA_NORMAL			= DBit(2),
	CRCGL_VA_COLOR			= DBit(3),
	CRCGL_VA_SECONDARYCOLOR	= DBit(4),
	CRCGL_VA_FOGCOORD		= DBit(5),
	CRCGL_VA_PSIZE			= DBit(6),
	CRCGL_VA_MATRIXINDEX	= DBit(7),
	CRCGL_VA_TEXCOORD0		= DBit(8),
	CRCGL_VA_TEXCOORD1		= DBit(9),
	CRCGL_VA_TEXCOORD2		= DBit(10),
	CRCGL_VA_TEXCOORD3		= DBit(11),
	CRCGL_VA_TEXCOORD4		= DBit(12),
	CRCGL_VA_TEXCOORD5		= DBit(13),
	CRCGL_VA_TEXCOORD6		= DBit(14),
	CRCGL_VA_TEXCOORD7		= DBit(15),
	CRCGL_VA_CVA_LOCK		= DBit(16),
	CRCGL_VA_VBO_ENABLE		= DBit(18),

	CRCGL_VA_MATRIXWEIGHT2	= CRCGL_VA_FOGCOORD,
	CRCGL_VA_MATRIXINDEX2	= CRCGL_VA_SECONDARYCOLOR,
};



// -------------------------------------------------------------------
#define MACRO_INDXGEOMARRAY_DECL(Name)		\
	void Internal_IndxGeomArray_##Name(const uint16* _piV, int _nV);	\
	friend void Internal_IndxVert_##Name(CRenderContextPS3* _pRC, const uint16* _piV, int _nV);


#define Internal_VAIndxVert(_piV, _nV, _PrimType)	\
	glnDrawElements(_PrimType, _nV, GL_UNSIGNED_SHORT, _piV)

#define Internal_VAIndxVert_End() void(0)

#define VAR_RenderVB_Begin(_pVBI)	\
	Internal_VA_SetArrays(_pVBI->m_VtxFmt, _pVBI->m_Stride, (uint8*)_pVBI->m_VB.m_pV);

#define VAR_RenderVB_End(_pVBI) void(0)

#ifndef M_RTM
#define GLErrDebug(_functionname) GLErr(_functionname)
#else
#define GLErrDebug(_functionname) {}
#endif

#ifndef M_RTM
#define GLErrDebugStatic(_functionname) GLErr(_functionname)
#else
#define GLErrDebugStatic(_functionname) {}
#endif

class CCGFPProgram
{
public:
	class CTreeCompare
	{
	public:
		M_INLINE static int Compare(const CCGFPProgram* _pFirst, const CCGFPProgram* _pSecond, void* _pContext)
		{
			if(_pFirst->m_Hash != _pSecond->m_Hash)
			{
				return (_pFirst->m_Hash<_pSecond->m_Hash)?-1:1;
			}
			return 0;
		}
		M_INLINE static int Compare(const CCGFPProgram* _pFirst, const uint32& _Key, void* _pContext)
		{
			if(_pFirst->m_Hash != _Key)
			{
				return (_pFirst->m_Hash<_Key)?-1:1;
			}
			return 0;
		}
	};
	CCGFPProgram() : m_Hash(0), m_Program(0), m_ConstParam(0), m_bWrongNumberOfParameters(false) {}

	DAVLAligned_Link(CCGFPProgram, m_Link, const uint32, CTreeCompare);

	uint32 m_Hash;
	CGprogram m_Program;
	CGparameter m_ConstParam;
	CStr m_Name;
	bool	m_bWrongNumberOfParameters;
};

class CCGVPProgram
{
public:
	class CTreeCompare
	{
	public:
		M_INLINE static int Compare(const CCGVPProgram* _pFirst, const CCGVPProgram* _pSecond, void* _pContext)
		{
			return memcmp(&_pFirst->m_Format, &_pSecond->m_Format, sizeof(CRC_VPFormat::CProgramFormat));
		}
		M_INLINE static int Compare(const CCGVPProgram* _pFirst, const CRC_VPFormat::CProgramFormat& _Key, void* _pContext)
		{
			return memcmp(&_pFirst->m_Format, &_Key, sizeof(CRC_VPFormat::CProgramFormat));
		}
	};

	CCGVPProgram() : m_Program(0), m_Parameter(0) {}

	DAVLAligned_Link(CCGVPProgram, m_Link, const CRC_VPFormat::CProgramFormat, CTreeCompare);

	CRC_VPFormat::CProgramFormat m_Format;
	CGprogram	m_Program;
	CGparameter	m_Parameter;	// constant parameter
};

class CRCGL_VBInfo : public CReferenceCount, public TLinkSP<CRCGL_VBInfo>
{
public:
	int m_VBID;
	int m_nTri;
	int m_VtxFmt;
	int m_Stride;
	uint16 m_Min, m_Max;
	void* m_pVB;
	CRC_VertexBuffer m_VB;
	TArray<uint16> m_liPrim;

	virtual CRCGL_VBInfo* GetThis()
	{
		return this;
	}

	void Clear()
	{
		m_VBID = -1;
		m_nTri = 0;
		m_Stride = 0;
		m_pVB = NULL;
		m_VB.Clear();
		m_liPrim.Clear();
		m_Min = 0xfffe;
		m_Max = 0;
	}

	void Create(int _VBID)
	{
		m_VBID = _VBID;
	}

	CRCGL_VBInfo()
	{
		Clear();
	}

	bool IsEmpty()
	{
		return (m_pVB == NULL);
	}
};

typedef TPtr<CRCGL_VBInfo> spCRCGL_VBInfo;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CRenderContextPS3
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CDisplayContextPS3;

class CRenderContextPS3;

typedef void(*PFN_INTERNAL_INDXVERT)(CRenderContextPS3* _pRC, const CRC_VertexBuffer& _VB, const uint16* _piV, int _nV);

class CRenderContextPS3 : public CRC_Core
{
	friend class CDisplayContextPS3;
	MRTC_DECLARE;
public:
	static CRenderContextPS3 ms_This;
	PSGLcontext*	m_pContext;
	CGcontext	m_CGContext;

	int m_bLog:1;
	int m_bLogUsage:1;
	int m_bLogVP:1;

protected:

/*	class CRCGL_VBInfo
	{
	public:
		int m_nTri;
		int m_nV;

		void Clear()
		{
			m_nTri = 0;
			m_nV = 0;
		}

		CRCGL_VBInfo()
		{
			Clear();
		}
	};*/

	TArray<TPtr<CRCGL_VBInfo> > m_lspVB;

	virtual CDisplayContext *GetDC();
	
	// -------------------------------------------------------------------
	//  Statistics
	int m_nBindTexture;
	int m_nAttribSet;
	int m_nTriTotal;
	int m_nTriCulled;
	int m_nVertices;

	int m_nStateEnableColorWrite;
	int m_nStateEnableZWrite;
	int m_nStateEnableZTest;
	int m_nStateEnableBlend;
	int m_nStateEnableSmooth;
	int m_nStateEnableCull;
	int m_nStateEnableCullCCW;
	int m_nStateEnableFog;
	int m_nStateEnableAlphaTest;

	int m_nStateBlendFunc;
	int m_nStateZFunc;
	int m_nStateAlphaFunc;
	int m_nStateStencil;
	int m_nStateFogColor;
	int m_nStateFogStart;
	int m_nStateFogEnd;
	int m_nStateFogDensity;
	int m_nStatePolygonOffset;

	int m_nStateVAAddress;
	int m_nStateVADrawElements;
	int m_nStateVAEnable;

	int m_nStateVP;
	int m_nStateFP;
	int m_nStateVPPipeline;

	int m_nStateParamTransform;
	int m_nStateParamVP;
	int m_nStateParamFP;

	// Status
//	int m_nTextures;

	// -------------------------------------------------------------------
	// GL Info

	uint8 m_lPicMips[CRC_MAXPICMIPS];			// Default: 0,0,0,0

	uint32 m_bPendingTextureParamUpdate : 1;
	uint32 m_bPendingProgramReload : 1;
	uint32 m_bResourceLog : 1;
	uint32 m_bDirtyVPCache : 1;
	uint32 m_bDirtyFPCache : 1;


#ifdef CRCGL_POLYGONOFFSETPROJECTIONMATRIX
	fp4 m_CurrentProjectionPolygonOffset;
#endif
	uint32 m_CurrentAttribGeomColor;
	uint32 m_CurrentVPHeight;
	uint32 m_CurrentVPWidth;
	CPnt m_CurrentVPPos;
//	uint32 m_CurrentRenderTargetHeight;
//	uint32 m_CurrentRenderTargetWidth;
	uint32 m_CurrentWindowHeight;
	uint32 m_CurrentWindowWidth;

	uint8 m_bInternalTexMatrixEnable[CRC_MAXTEXCOORDS];
	CMat4Dfp4 m_lInternalTexMatrix[CRC_MAXTEXCOORDS];

	CMat4Dfp4 m_Matrix_ProjectionGL;
	CMat4Dfp4 m_Matrix_ProjectionTransposeGL;
	CMat4Dfp4 m_Matrix_Unit;

	int m_VACurrentVBID;
	int m_VACurrentFmt;
	int m_VACurrentStride;
	uint8* m_pVACurrentFmtBase;
	int m_VAEnable;
	int m_VAMinVertexCount;		// Minimum number of vertices in batch for VA to be used.
#ifdef CRCGL_VADEBUG_ENABLE
	CRC_VertexBuffer m_VADebugState;
#endif
	
	// -------------------------------------------------------------------
	// Collect function data
	static FCollect m_lGLCollectFunctionsAll[161];
	static FCollect m_lGLCollectFunctionsIndexed[161];

	FCollect GL_Collect_GetFunctionAll(int _iFunction);
	FCollect GL_Collect_GetFunctionIndexed(int _iFunction);

	// -------------------------------------------------------------------
	// VertexBufferObjects
	void VB_DeleteAll();
	void VB_Delete(int _VBID);
	void VB_Create(int _VBID);

	void VBO_CreateVB(int _VBID, CRC_BuildVertexBuffer& _VB);
	void VBO_DestroyVB(int _VBID);

	void VBO_Begin(int _VBID);
	void VBO_Disable();

	void VBO_RenderVB(int _VBID);
	void VBO_RenderVB_IndexedTriangles(int _VBID, uint16* _piPrim, int _nTri);
	void VBO_RenderVB_IndexedPrimitives(int _VBID, uint16* _piPrim, int _nPrim);


	// -------------------------------------------------------------------
	// Stencil
	void Attrib_SetStencil(CRC_Attributes* _pAttrib);

	// -------------------------------------------------------------------
	// VSync
	int			m_VSync;

	// -------------------------------------------------------------------
	// Vertex program
	uint8 m_VP_liTexMatrixReg[CRC_MAXTEXCOORDS];
	uint8 m_VP_iConstantColor;
	
	CRC_VPGenerator m_VP_ProgramGenerator;

	DAVLAligned_Tree(CCGVPProgram, m_Link, const CRC_VPFormat::CProgramFormat, CCGVPProgram::CTreeCompare) m_ProgramTree;
	DAVLAligned_Tree(CCGFPProgram, m_Link, const uint32, CCGFPProgram::CTreeCompare) m_FPProgramTree;
	CCGVPProgram* m_pCurrentVPProgram;
	CCGFPProgram* m_pCurrentFPProgram;
	CVec4Dfp4 m_VPConstRegisters[256];
	uint16 m_nUpdateVPConst:15;
	uint16 m_bUpdateVPConst:1;

	void VP_SaveCache();
	void VP_LoadCache();
	void VP_LoadBinary(const uint8* _pObject, const CRC_VPFormat::CProgramFormat& _Format);
	void VP_Update();
	void VP_Load(const char* _pProgramName, const CRC_VPFormat::CProgramFormat& _Format);
	void VP_Bind(const CRC_VPFormat& _Format);
	void VP_Disable();
	void VP_Init();
	void VP_DeleteAllObjects();

	// -------------------------------------------------------------------
	// Fragment program
	void FP_SaveCache();
	void FP_LoadCache();
	void FP_LoadBinary(const uint8* _pProgram, uint32 _Hash);
	void FP_Load(const char* _pProgram, uint32 _Hash);
	int FP_LoadFile(CStr _Name, int _ID);
	void FP_Bind(const char* _ProgramName);
	void FP_Bind(CRC_ExtAttributes_FragmentProgram20* _pExtAttr);
	void FP_Disable();
	void FP_Init();

	// -------------------------------------------------------------------
	// EXT_TEXTURE_FILTER_ANISOTROPIC
	fp4 m_Anisotropy;
	fp4 m_MaxAnisotropy;

	// -------------------------------------------------------------------
	// Vertex-array rendering
	void Internal_VA_Disable();

	void Internal_VA_NormalPtr(const CVec3Dfp4* _pN, int _Stride = 0);
	void Internal_VA_ColorPtr(const CPixel32* _pCol, int _Stride = 0);
//	void Internal_VA_SpecPtr(const CPixel32* _pSpec, int _Stride = 0);
//	void Internal_VA_FogPtr(const fp4* _pFog, int _Stride = 0);
	void Internal_VA_TexCoordPtr(const fp4* _pTV, int _nComp, int _iTxt, int _Stride = 0);
	void Internal_VA_MatrixIndexPtr(const uint32* _piMatrices, int _nComp, int _Stride = 0);
	void Internal_VA_MatrixWeightPtr(const fp4* _pMatrixWeights, int _nComp, int _Stride = 0);
	void Internal_VA_VertexPtr(const CVec3Dfp4* _pV, int _Stride = 0);

	void Internal_VA_SetArrays(int _VBID, int _VtxFmt, int _Stride, uint8* _pBase);
	void Internal_VA_SetArrays(const CRC_VertexBuffer& _VB);

//	void Internal_VAIndxVert_Begin();
//	void Internal_VAIndxVert_End();

	static int Internal_VAIndx(int _glPrimType, const uint16* _piVerts, int _nVerts, int _Offset);
	void Internal_VAIndx(int _glPrimType, const uint16* _piVerts, int _nVerts, uint16 _Min, uint16 _Max);

	void Internal_VAIndxPrimitives(uint16* _pPrimStream, int _StreamLen, int _Offset);
	void Internal_VAIndxTriangles(uint16* _pPrim, int _nTri);

	void Internal_VBIndxTriangles(int _VBID, uint16* _piPrim, int _nTri);
	void Internal_VBIndxPrimitives(int _VBID, uint16* _piPrim, int _nPrim);

	// -------------------------------------------------------------------
protected:
	void Internal_IndxTriangles(uint16* _pIndices, int _nTriangles, const CRC_VertexBuffer& _VB, int _bAllUsed);
	void Internal_IndxWires(uint16* _pIndices, int _Len, const CRC_VertexBuffer& _VB, int _bAllUsed);
	void Internal_IndxPrimitives(uint16* _pPrimStream, int _StreamLen, const CRC_VertexBuffer& _VB, int _bAllUsed);

	// -------------------------------------------------------------------
	// Internal texture-management functions

	void GL_InitTextureBuildBuffers(int _MaxTextureSize, int _DebugResponsibleTextureID = 0);
	void GL_InitTextures();
	void GL_DeleteTextures();

	bool LoadTexture(int _GLImageTarget, int _TextureID, CTextureContainer* _pTC, int _iLocal, uint8 _TextureVersion, const CTC_TextureProperties& _Properties, int _iFirstMip, int _Width, int _Height, int _nMip);
	bool LoadTextureBuildInto(int _GLImageTarget, int _TextureID, CTextureContainer* _pTC, int _iLocal, const CImage& _TxtDesc, const CTC_TextureProperties& _Properties, int _iFirstMip, int _Width, int _Height, int _nMip);
	bool LoadTextureRender(int _GLImageTarget, int _TextureID, CTextureContainer* _pTC, int _iLocal, const CImage& _TxtDesc, const CTC_TextureProperties& _Properties, int _iFirstMip, int _Width, int _Height, int _nMip);

	void GL_BuildTexture2D(int _RCID, int _TextureID, int _GLObjectTarget, int _GLImageTarget, const CTC_TextureProperties& _Properties);
	void GL_SetTextureParameters(int _RCID, int _TextureID, int _GLObjectTarget, const CTC_TextureProperties& _Properties);
	void GL_SetTextureParameters(int _TextureID);
	void GL_UpdateAllTextureParameters();
	void GL_BuildTexture(int _RCID, int _TextureID, const CTC_TextureProperties& _Properties);
	void GL_UnloadTexture(int _RCID);
	void GL_SetTexture(int _TextureID, int _iTexChannel, bool _bPrecache = false);

	// -------------------------------------------------------------------
	// The universal render-polygon routine, used for clipped geometry and fall-back for any unimplemented overrides.
	virtual void Internal_RenderPolygon(int _nV, const CVec3Dfp4* _pV, const CVec3Dfp4* _pN, const CVec4Dfp4* _pCol = NULL, 
		const CVec4Dfp4* _pSpec = NULL, //const fp4* _pFog = NULL,
		const CVec4Dfp4* _pTV0 = NULL, const CVec4Dfp4* _pTV1 = NULL, const CVec4Dfp4* _pTV2 = NULL, const CVec4Dfp4* _pTV3 = NULL, int _Color = 0xffffffff);


	void GL_DestroyAllPrograms();
	bool m_bAttribGlobalUpdate;
public:
	CRenderContextPS3();
	~CRenderContextPS3();
	virtual void Create(CObj* _pContext, const char* _pParams);

	void GL_InitSettings();
private:
	void VSync_Update();

public:
	virtual void BeginScene(CRC_Viewport* _pVP);
	virtual void EndScene();
	virtual const char * GetRenderingStatus();

	virtual void Viewport_Update();

	virtual void Texture_Precache(int _TextureID);
	virtual void Texture_Copy(int _SourceTexID, int _DestTexID, CRct _SrcRgn, CPnt _DstPos);
	virtual CRC_TextureMemoryUsage Texture_GetMem(int _TextureID);
	virtual int Texture_GetPicmipFromGroup(int _iPicmip);
	virtual void Texture_PrecacheFlush();
	virtual void Texture_PrecacheBegin( int _Count );
	virtual void Texture_PrecacheEnd();
		
	virtual void Flip_SetInterval(int _nFrames){};

	// Attribute overrides
protected:
	virtual int Attrib_GlobalGetVar(int _Var);

	bint Attrib_SetRasterMode(CRC_Attributes* _pA, int _RasterMode);
	void Attrib_SetVPPipeline(CRC_Attributes* _pAttrib);
	void Attrib_SetPixelProgram(CRC_Attributes* _pAttrib);

	virtual void Attrib_Set(CRC_Attributes* _pAttrib);
	virtual void Attrib_SetAbsolute(CRC_Attributes* _pAttrib);

	virtual void Attrib_GlobalUpdate();
	virtual void Attrib_DeferredGlobalUpdate();
#ifdef CRCGL_POLYGONOFFSETPROJECTIONMATRIX
	void Attrib_SetPolygonOffset(fp4 _Offset);
#endif

	// Transform overrides
	virtual void Matrix_SetRender(int _iMode, const CMat4Dfp4* _pMatrix);

public:
	// Geometry overrides
	virtual void Geometry_Color(CPixel32 _Col);

	virtual void Geometry_PrecacheFlush( );
	virtual void Geometry_PrecacheBegin( int _Count );
	virtual void Geometry_Precache(int _VBID);

	// Rendering overrides
	virtual void Render_IndexedTriangles(uint16* _pTriVertIndices, int _nTriangles);
	virtual void Render_IndexedWires(uint16* _pIndices, int _Len);
 	virtual void Render_IndexedPrimitives(uint16* _pPrimStream, int _StreamLen);

	virtual void Render_VertexBuffer(int _VBID);


	virtual void Render_Wire(const CVec3Dfp4& _v0, const CVec3Dfp4& _v1, CPixel32 _Color);
	virtual void Render_WireStrip(const CVec3Dfp4* _pV, const uint16* _piV, int _nVert, CPixel32 _Color);
	virtual void Render_WireLoop(const CVec3Dfp4* _pV, const uint16* _piV, int _nVert, CPixel32 _Color);

	// -------------------------------------------------------------------
	// Occlusion query
public:
	enum
	{
		OCID_HASHFRAMES = 3,

		OCID_HASHBITS = 7,
		OCID_HASHNUM = 1 << OCID_HASHBITS,
		OCID_HASHMASK = (OCID_HASHNUM - 1),
	};

protected:
	class COCID
	{
	public:
		int m_QueryID;
	};

	class COSIDHash : public THash<int16, COCID>
	{
	public:
		void Insert(int _OCID, int _QueryID)
		{
			THash<int16, COCID>::Insert(_OCID, _QueryID & OCID_HASHMASK);
			CHashIDInfo* pID = &m_pIDInfo[_OCID];
			pID->m_QueryID = _QueryID;
		}

		int GetOCID(int _QueryID)
		{
			int ID = m_pHash[_QueryID & OCID_HASHMASK];
			while(ID != -1)
			{
				if (m_pIDInfo[ID].m_QueryID == _QueryID)
					return ID;
				ID = m_pIDInfo[ID].m_iNext;
			}
			return -1;
		}
	};

	int m_OC_iHashInsert;
	int m_OC_iHashQuery;
	COSIDHash m_OC_lQueryIDHash[OCID_HASHFRAMES];
	int m_OC_iHashOCIDNext[OCID_HASHFRAMES];

	void OcclusionQuery_Init();
	void OcclusionQuery_PrepareFrame();
	int OcclusionQuery_AddQueryID(int _QueryID);	// Returns GL OCid. if zero, no more OCid:s are available.
	int OcclusionQuery_FindOCID(int _QueryID);		// Returns GL OCid, if -1, the id doesn't exist
	
public:
	virtual void OcclusionQuery_Begin(int _QueryID);
	virtual void OcclusionQuery_End();
	virtual int OcclusionQuery_GetVisiblePixelCount(int _QueryID);

	// -------------------------------------------------------------------
	virtual bool ReadDepthPixels(int _x, int _y, int _w, int _h, fp4* _pBuffer);

	// Console commands:

	void Con_ChangePicMip(int _Level, int _iPicMip);
	void Con_r_picmip(int _iPicMip, int _Level);

	void Con_gl_reloadprograms();

	void Con_gl_anisotropy(fp4 _Value);
	void Con_gl_vsync(int _Value);

	void Con_gl_logprograms(int _Value);

	void Register(CScriptRegisterContext &_RegContext);

	virtual void RenderTarget_Clear(CRct _ClearRect, int _WhatToClear, CPixel32 _Color, fp4 _ZBufferValue, int _StecilValue);
	virtual void RenderTarget_Copy(CRct _SrcRect, CPnt _Dest, int _CopyType);
	virtual void RenderTarget_CopyToTexture(int _TextureID, CRct _SrcRect, CPnt _Dest, bint _bContinueTiling, uint16 _Slice);

	virtual int Texture_GetBackBufferTextureID() {return 0;}
	virtual int Texture_GetFrontBufferTextureID() {return 0;}
	virtual int Texture_GetZBufferTextureID() {return 0;}

	virtual int Geometry_GetVBSize(int _VBID) {return 0;}

	CMTime m_StartFrame;
	CMTime m_LastFrame;
	CRC_Statistics m_Stats;
	CRC_Statistics Statistics_Get() 
	{
		return m_Stats;
	}


	void DebugNop(void* _pMoomin = NULL);
};

void GetMinMax(const uint16* _pPrim, int _nPrim, uint16& _Min, uint16& _Max);

#endif //_INC_MRndrGL
