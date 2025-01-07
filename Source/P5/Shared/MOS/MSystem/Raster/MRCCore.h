
#ifndef _INC_MRCCore
#define _INC_MRCCore

#include "MRender.h"

//#define M_RENDER_XENON_PUSHBUFFER
#define M_RENDER_XENON_DIRECT3D

//----------------------------------------------------------------
class CRC_GlobalAttrib// damn these vptrs that wont stay the same across dlls : public CObj
{
public:
	int m_Flags;
	int m_Filter;
	CVec4Dfp4 m_GammaScale;
	CVec4Dfp4 m_GammaAdd;
	fp4 m_GammaCorrection;

public:
	CRC_GlobalAttrib()
	{
		m_Flags = CRC_GLOBALFLAGS_TEXTURE;
		m_Filter = CRC_GLOBALFILTER_TRILINEAR;
		m_GammaScale = 1.0f;
		m_GammaAdd = 0.0f;
		m_GammaCorrection = 1.0f;
	};
};

//----------------------------------------------------------------
enum
{
	CRC_VF_NORMAL				= DBit(0),
	CRC_VF_COLOR				= DBit(1),
	CRC_VF_SPECULAR				= DBit(2),
	CRC_VF_TEXCOORD0			= DBit(3),
	CRC_VF_TEXCOORD1			= DBit(6),
	CRC_VF_TEXCOORD2			= DBit(9),
	CRC_VF_TEXCOORD3			= DBit(12),
	CRC_VF_TEXCOORD4			= DBit(15),
	CRC_VF_TEXCOORD5			= DBit(18),
	CRC_VF_TEXCOORD6			= DBit(21),
	CRC_VF_TEXCOORD7			= DBit(24),
	CRC_VF_MATRIXI				= DBit(27),
	CRC_VF_MATRIXW				= DBit(28),

	// -------------------------------
	CRC_VF_TEXCOORD_SHIFT		= 3,

	CRC_VF_TEXCOORD_MASK0		= (7 << CRC_VF_TEXCOORD_SHIFT),
	CRC_VF_TEXCOORD_MASK1		= (CRC_VF_TEXCOORD_MASK0 << 3),
	CRC_VF_TEXCOORD_MASK2		= (CRC_VF_TEXCOORD_MASK1 << 3),
	CRC_VF_TEXCOORD_MASK3		= (CRC_VF_TEXCOORD_MASK2 << 3),
	CRC_VF_TEXCOORD_MASK4		= (CRC_VF_TEXCOORD_MASK3 << 3),
	CRC_VF_TEXCOORD_MASK5		= (CRC_VF_TEXCOORD_MASK4 << 3),
	CRC_VF_TEXCOORD_MASK6		= (CRC_VF_TEXCOORD_MASK5 << 3),
	CRC_VF_TEXCOORD_MASK7		= (CRC_VF_TEXCOORD_MASK6 << 3),

	CRC_VF_MATRIXW_SHIFT		= 28,
	CRC_VF_MATRIXW_MASK			= (15 << CRC_VF_MATRIXW_SHIFT),
};

#define CRC_FV_TEXCOORD(n,ncomp)		(ncomp << (CRC_VF_TEXCOORD_SHIFT + n*3))
#define CRC_FV_TEXCOORD_MASK(n)			(CRC_VF_TEXCOORD_MASK0 << (n*3))
#define CRC_FV_TEXCOORD_NUMCOMP(vf, n)	((vf >> (CRC_VF_TEXCOORD_SHIFT + n*3)) & 7)

#define CRC_FV_MATRIXW(ncomp)			(ncomp << (CRC_VF_MATRIXW_SHIFT))
#define CRC_FV_MATRIXW_NUMCOMP(vf)		((vf >> (CRC_VF_MATRIXW_SHIFT)) & 15)

//----------------------------------------------------------------
enum
{
	CRC_CLIPARRAY_VERTEX		= 1,
	CRC_CLIPARRAY_COLOR			= 2,
	CRC_CLIPARRAY_SPECULAR		= 4,
	CRC_CLIPARRAY_TVERTEX0		= 8,
	CRC_CLIPARRAY_TVERTEX1		= 16,
	CRC_CLIPARRAY_TVERTEX2		= 32,
	CRC_CLIPARRAY_TVERTEX3		= 64,
//	CRC_CLIPARRAY_FOG			= 128,
	CRC_CLIPARRAY_NORMAL		= 256,
};

enum
{
	CRC_CLIPSTACK				= 8,
	CRC_CLIPMAXPLANES			= 16,
};

class CRC_ClipStackEntry
{
public:
	CPlane3Dfp4 m_Planes[CRC_CLIPMAXPLANES];
	int m_nPlanes;

	CRC_ClipStackEntry();
	void Copy(const CRC_ClipStackEntry& _Src, const CMat4Dfp4* _pTransform = NULL);
};

#ifdef M_STATIC_RENDERER
#define DRenderVirtual
#else
#define DRenderVirtual virtual
#endif

//----------------------------------------------------------------
struct CRC_IDInfo
{
#ifndef PLATFORM_CONSOLE
	int16 m_RCID;					// RC ID used by this texture
#endif
	int16 m_Fresh;					// Need to be rebuilt?

	CRC_IDInfo()
	{
#ifndef PLATFORM_CONSOLE
		m_RCID = -1;
#endif
		m_Fresh = 0;
	}
};

class CRC_TCIDInfo : public CReferenceCount
{
public:
	CRC_IDInfo* m_pTCIDInfo;		// Parallell array to m_pTxtInfo

	CRC_TCIDInfo(int _IDCapacity)
	{
		m_pTCIDInfo = DNew(CRC_IDInfo) CRC_IDInfo[_IDCapacity];
		if (!m_pTCIDInfo) MemError("-");
	}
	
	~CRC_TCIDInfo()
	{
		if (m_pTCIDInfo != NULL) { delete[] m_pTCIDInfo; m_pTCIDInfo= NULL; }
	}
};

// -------------------------------------------------------------------
class CRC_VBIDInfo
{
public:
#ifndef PLATFORM_CONSOLE
	int16 m_RCID;					// RC ID (vb or displaylist #) used by this VB
#endif
	int8 m_Fresh;					// Need to be rebuilt?
	
	CRC_VBIDInfo()
	{
#ifndef PLATFORM_CONSOLE
		m_RCID = -1;
#endif
		m_Fresh = 0;
	}
};

// -------------------------------
// Collect function data

class CRC_CollectFunctionParams : public CRC_VertexBuffer
{
public:
	uint8 m_lTVCompDest[CRC_MAXTEXCOORDS];
	uint32 m_ConstantColor;

	fp4 *m_pVertexList;
};

typedef void* (*FCollect ) (CRC_CollectFunctionParams *Params);

//----------------------------------------------------------------
class CRCPrimStreamIteratorEntry
{
public:
	const uint16* m_pPrimStreamPos;
	const uint16* m_pPrimStreamPosEnd;
};

#define MAX_CRCPrimStreamIterator_StackDepth 3

class CRCPrimStreamIterator
{
public:
	const uint16 *m_pCurrentPtr;
	CRCPrimStreamIteratorEntry *m_pCurrentEntry;
	CRCPrimStreamIteratorEntry m_Stack[MAX_CRCPrimStreamIterator_StackDepth];
	
	CRCPrimStreamIterator(const uint16* _pPrimStream, int _StreamLen)
	{
		m_pCurrentEntry = m_Stack;
		m_Stack[0].m_pPrimStreamPos = _pPrimStream;
		m_Stack[0].m_pPrimStreamPosEnd = _pPrimStream + _StreamLen;
		FindNextRealEntry();

		while (!IsValid())
		{
			m_pCurrentEntry->m_pPrimStreamPos += (*(m_pCurrentEntry->m_pPrimStreamPos)) >> 8;
		
			if ((m_pCurrentEntry->m_pPrimStreamPos >= m_pCurrentEntry->m_pPrimStreamPosEnd) ||
				(((*(m_pCurrentEntry->m_pPrimStreamPos)) & CRC_RIP_TYPEMASK) == CRC_RIP_END))
			{
				if (m_pCurrentEntry == m_Stack)
					return;
			
				--m_pCurrentEntry;
			}
		
			FindNextRealEntry();
		}

		m_pCurrentPtr = m_pCurrentEntry->m_pPrimStreamPos;

	}
	
	void FindNextRealEntry()
	{
		while (((*(m_pCurrentEntry->m_pPrimStreamPos)) & CRC_RIP_TYPEMASK) == CRC_RIP_PRIMLIST)
		{
			
			if (m_pCurrentEntry == &m_Stack[MAX_CRCPrimStreamIterator_StackDepth - 1])
			{
				m_pCurrentEntry->m_pPrimStreamPos += (*(m_pCurrentEntry->m_pPrimStreamPos)) >> 8;

				Error_static("CRCPrimStreamIterator::FindNextRealEntry", "Nesting too deep.");

				if (!IsValid())
					return;
			}
			else
			{
#ifdef CPU_ALIGNED_MEMORY_ACCESS
				uint16 *pDst = (uint16*)&((m_pCurrentEntry + 1)->m_pPrimStreamPos);
				uint16 *pSrc = (uint16*)&m_pCurrentEntry->m_pPrimStreamPos[1];
				
				pDst[0]	= pSrc[0];
				pDst[1]	= pSrc[1];
				if(sizeof(void*) == 8)
				{
					pDst[2]	= pSrc[2];
					pDst[3]	= pSrc[3];
				}
#else
				(m_pCurrentEntry + 1)->m_pPrimStreamPos = (*((const uint16 **)(m_pCurrentEntry->m_pPrimStreamPos + 1)));
#endif
				(m_pCurrentEntry + 1)->m_pPrimStreamPosEnd = (m_pCurrentEntry + 1)->m_pPrimStreamPos + (*((m_pCurrentEntry->m_pPrimStreamPos + 1 + sizeof(void *) / 2)));
				
				m_pCurrentEntry->m_pPrimStreamPos += (*(m_pCurrentEntry->m_pPrimStreamPos)) >> 8;
				++m_pCurrentEntry;
			}
		}
	}
	
	M_INLINE bool IsValid()
	{
		if (m_pCurrentEntry->m_pPrimStreamPos >= m_pCurrentEntry->m_pPrimStreamPosEnd)
			return false;

		int CurType = ((*(m_pCurrentEntry->m_pPrimStreamPos)) & CRC_RIP_TYPEMASK);
		return (CurType != CRC_RIP_END && CurType != CRC_RIP_PRIMLIST);
	}
	
	M_INLINE bool Next()
	{
		do
		{
			int RIPSize = (*(m_pCurrentEntry->m_pPrimStreamPos)) >> 8;
			if (!RIPSize)
				return false;
			
			m_pCurrentEntry->m_pPrimStreamPos += RIPSize;
		
			if ((m_pCurrentEntry->m_pPrimStreamPos >= m_pCurrentEntry->m_pPrimStreamPosEnd) ||
				(((*(m_pCurrentEntry->m_pPrimStreamPos)) & CRC_RIP_TYPEMASK) == CRC_RIP_END))
			{
				if (m_pCurrentEntry == m_Stack)
					return false;
			
				--m_pCurrentEntry;
			}
		
			FindNextRealEntry();
		}
		while (!IsValid());

		m_pCurrentPtr = m_pCurrentEntry->m_pPrimStreamPos;
		
		return true;
	}
	
	M_INLINE int GetCurrentType()
	{
		return ((*m_pCurrentPtr) & CRC_RIP_TYPEMASK);
	}
	
	M_INLINE const uint16 *GetCurrentPointer()
	{
		return m_pCurrentPtr + 1;
	}
};

//----------------------------------------------------------------
class SYSTEMDLLEXPORT CRC_Core 
	: public CRenderContext
{
public:
	bool m_bAddedToConsole;
	int m_bShowGamma : 1;				// Show gamma correction helpers.
	int m_bAllowTextureLoad : 1;

	int m_Caps_TextureFormats;			// Initialized in contructors
	int m_Caps_DisplayFormats;
	int m_Caps_ZFormats;
	int m_Caps_StencilDepth;
	int m_Caps_AlphaDepth;
	int m_Caps_Flags;

	int m_Caps_nMultiTexture;
	int m_Caps_nMultiTextureCoords;
	int m_Caps_nMultiTextureEnv;

	CRC_GlobalAttrib m_Mode;			// Global render mode

	// -------------------------------
	// Viewport stack
	TList_Vector<CRC_Viewport> m_lVPStack;
	int m_iVPStack;

	// -------------------------------
	// Texture management data
	CTextureContext* m_pTC;				// Same as "SYSTEM.TEXTURECONTEXT"
	TPtr<CRC_TCIDInfo> m_spTCIDInfo;	// TextureID info
	int m_iTC;							// This RC's Index in the global TextureContext

	// -------------------------------
	// VB management data
	class CXR_VBContext* m_pVBCtx;		// Same as "SYSTEM.VBCONTEXT"
	TArray<CRC_VBIDInfo> m_lVBIDInfo;	// TextureID info
	int m_iVBCtxRC;						// This RC's Index in m_pVBCtx

	// -------------------------------
	// Attribute stack
	CRC_Attributes m_CurrentAttrib;		// Current settings passed to the underlying API/renderer.
	int m_iAttribStack;					// Attrib stack pointer, points at the actual current setting.
	int m_AttribChanged;				// Anything that might need an update?
	int m_AttribStackDepth;
#ifdef RC_DYNAMIC_ATTRIB_STACK
	TList_Vector<CRC_Attributes> m_lAttribStack;
	TList_Vector<int> m_lAttribStackChanged;
#else
	CRC_Attributes m_lAttribStack[CRC_ATTRIBSTACKDEPTH];
	int m_lAttribStackChanged[CRC_ATTRIBSTACKDEPTH];
#endif

	// -------------------------------
	// Matrix stack
	class CRC_MatrixState
	{
	public:
		CMat4Dfp4 m_lMatrices[CRC_MATRIXSTACKS];
		const CRC_MatrixPalette *m_pMatrixPaletteArgs;
//		const CMat4Dfp4* m_pMatrixPalette;
//		unsigned int m_nMatrixPalette : 16;
		unsigned int m_MatrixUnit : (CRC_MATRIXSTACKS+1);			// We currently need (CRC_MATRIXSTACKS+1) bits (7)
		unsigned int m_MatrixChanged : (CRC_MATRIXSTACKS+1);		// We currently need (CRC_MATRIXSTACKS+1) bits (7)
												// MatrixChanged is a bit per matrix that is true if matrix has changed relative to the previous stack entry. (iThisStackEntry-1)

		void Clear()
		{
			for(int i = 0; i < CRC_MATRIXSTACKS; i++)
				m_lMatrices[i].Unit();
			m_pMatrixPaletteArgs = NULL;
//			m_pMatrixPalette = NULL;
//			m_nMatrixPalette = 0;
			m_MatrixUnit = ~0;
			m_MatrixChanged = 0;
		}

		CRC_MatrixState()
		{
			Clear();
		}
	};

	TThinArray<CRC_MatrixState> m_lMatrixStack;

	int m_MatrixChanged;						// Current matrix changed state. Matrix_Pop will OR it's matrixchanged state on top of this.
	int m_iMatrixStack;
	int m_iMatrixMode;

/*	int m_iMatrixStack;
	int m_iMatrixMode;
	int m_MatrixChanged;				// Need to update API/Renderer?
	int m_MatrixUnit;					// Matrix is unit
*/
	bool m_bClipCurrentMatrixIsMirrored;
	CMat4Dfp4 m_ClipCurrentMatrixInv;
	CVec3Dfp4 m_ClipCurrentLocalVP;		// POV in current model coordinate-space, updated in Matrix_Update()

	// -------------------------------
	// Clipping stack
	TList_Vector<CRC_ClipStackEntry> m_lClipStack;
	int m_iClipStack;

	bool m_bClipChanged;
	CRC_ClipStackEntry m_ClipCurrent;	// Transformed clipping.

	// -------------------------------
	// Collect function data
//	static FCollect m_lCollectFunctionsAll[160];
//	static FCollect m_lCollectFunctionsIndexed[160];

	
	// -------------------------------
	// Clipping working data
	TList_Vector<uint16> m_lClipVertexMask;
	int m_ClipMask;

	CPixel32 m_ClipColor;
	CVec3Dfp4 m_ClipV[CRC_MAXPOLYGONVERTICES];
	CVec3Dfp4 m_ClipN[CRC_MAXPOLYGONVERTICES];
	CVec4Dfp4 m_ClipCol[CRC_MAXPOLYGONVERTICES];
	CVec4Dfp4 m_ClipSpec[CRC_MAXPOLYGONVERTICES];
//	fp4 m_ClipFog[CRC_MAXPOLYGONVERTICES];
	CVec4Dfp4 m_ClipTV0[CRC_MAXPOLYGONVERTICES];
	CVec4Dfp4 m_ClipTV1[CRC_MAXPOLYGONVERTICES];
	CVec4Dfp4 m_ClipTV2[CRC_MAXPOLYGONVERTICES];
	CVec4Dfp4 m_ClipTV3[CRC_MAXPOLYGONVERTICES];

	// -------------------------------
	// Current geometry					// (NOT PART OF THE ATTRIBUTES!, THESE ARE NOT PUSH-/POP-ed!
	CRC_VertexBuffer m_Geom;
	CPixel32 m_GeomColor;

/*	const CVec3Dfp4* m_GeomArrayV;
	const CVec3Dfp4* m_GeomArrayN;
	const fp4* m_GeomArrayTV[CRC_MAXTEXCOORDS];
	int m_nGeomArrayTVComp[CRC_MAXTEXCOORDS];
	const CPixel32* m_GeomArrayCol;
	const CPixel32* m_GeomArraySpecular;
	const fp4* m_GeomArrayFog;
	const uint32* m_GeomArrayMI;
	const fp4* m_GeomArrayMW;
	int m_nGeomArrayMWComp;
	int m_nGeomArrayV;
	CPixel32 m_GeomColor;*/

	int m_bGeomArrayAllUsed;

	int m_GeomVBID;

public:
	DRenderVirtual void Internal_RenderPolygon(int _nV, const CVec3Dfp4* _pV, const CVec3Dfp4* _pN, const CVec4Dfp4* _pCol = NULL, 
		const CVec4Dfp4* _pSpec = NULL, 
//const fp4* _pFog = NULL,
		const CVec4Dfp4* _pTV0 = NULL, const CVec4Dfp4* _pTV1 = NULL, const CVec4Dfp4* _pTV2 = NULL, const CVec4Dfp4* _pTV3 = NULL, int _Color = 0xffffffff) DRenderPost ;

	// -------------------------------
	// Current view settings
	fp4 m_BackPlane;
	fp4 m_FrontPlane;
	fp4 m_BackPlaneInv;
	fp4 m_FrontPlaneInv;

	// -------------------------------
	// Stats
	int m_nParticles;
	int m_nWires;
	int m_nTriangles;
	int m_nPolygons;
	int m_nClipVertices;
	int m_nClipTriangles;
	int m_nClipTrianglesDrawn;

public:
	CRC_Core();
	virtual ~CRC_Core();

	DRenderVirtual void PreEndScene();
	
	DRenderVirtual void Texture_MakeAllDirty(int _iPicMip = -1);

	// Internals, don't use.
	DRenderVirtual void Attrib_Update();
	DRenderVirtual void Attrib_GlobalUpdate();
	DRenderVirtual bint Attrib_SetRasterMode(CRC_Attributes* _pA, int _RasterMode);
	DRenderVirtual void Attrib_Set(CRC_Attributes* _pAttrib) DRenderPost;
	DRenderVirtual void Attrib_SetAbsolute(CRC_Attributes* _pAttrib); 

public:

	// Transform
public:
	CRC_MatrixState& Matrix_GetState();
	const CMat4Dfp4& Matrix_Get();
	DRenderVirtual void Matrix_SetRender(int _iMode, const CMat4Dfp4* _pMatrix);
	DRenderVirtual void Matrix_Update();

public:

	// Clipping
public:
	M_INLINE bool Clip_IsVisible(const CVec3Dfp4* _pV0, const CVec3Dfp4* _pV1, const CVec3Dfp4* _pV2)
	{
		M_ASSERT(Clip_IsEnabled(), "Can only be used when clip is enabled.");

		fp4 v1x = (_pV1->k[0] - _pV0->k[0]);
		fp4 v1y = (_pV1->k[1] - _pV0->k[1]);
		fp4 v1z = (_pV1->k[2] - _pV0->k[2]);
		fp4 v2x = (_pV2->k[0] - _pV0->k[0]);
		fp4 v2y = (_pV2->k[1] - _pV0->k[1]);
		fp4 v2z = (_pV2->k[2] - _pV0->k[2]);

		fp4 nx = v1y*v2z - v1z*v2y;
		fp4 ny = - v1x*v2z + v1z*v2x;
		fp4 nz = v1x*v2y - v1y*v2x;

		fp4 dotp = 
			nx*(_pV0->k[0] - m_ClipCurrentLocalVP.k[0]) + 
			ny*(_pV0->k[1] - m_ClipCurrentLocalVP.k[1]) + 
			nz*(_pV0->k[2] - m_ClipCurrentLocalVP.k[2]);

		if (m_CurrentAttrib.m_Flags & CRC_FLAGS_CULLCW) dotp = -dotp;

		if (m_bClipCurrentMatrixIsMirrored)
		{	if (dotp > 0.0f) return false; }
		else
		{	if (dotp < 0.0f) return false; }

		return true;
	}

	int Clip_IsEnabled() { return m_lClipStack[m_iClipStack].m_nPlanes; };
	int Clip_CutFace(int _nv, const CPlane3Dfp4* _pPlanes, int _np, int _Components, int _bInvertPlanes = false);
	void Clip_RenderPolygon(int _nV, int _Components, int _PlaneMask = -1);
	int Clip_InitVertexMasks(int _nV, const CVec3Dfp4* _pV, const uint16* _piV = NULL);
	void Clip_Update();

public:

	// Geometry helpers
	TList_Vector<uint16> m_liVUse;
	TList_Vector<uint8> m_lVUseMap;
	TList_Vector<uint16> m_lVUMClear;

	TList_Vector<uint8> m_lBVUMap;
	TList_Vector<uint16> m_lBVUMapIndex;

//	FCollect Collect_GetFunctionAll(int _iFunction);
//	FCollect Collect_GetFunctionIndexed(int _iFunction);

	void Geometry_GetVertexFormat(const CRC_VertexBuffer& _VB, int &_RetVF, int &_RetVSize, int &_RetCollectFunction);
	int Geometry_GetCollectFunction(int _VF);

	bool Geometry_BuildTriangleListFromPrimitives(CRCPrimStreamIterator &_StreamIterate, uint16* _pDest, int& _nMaxDestRetSize);
	int Geometry_BuildTriangleListFromPrimitivesCount(CRCPrimStreamIterator &_StreamIterate);
	int Geometry_BuildVertexUsageCount(int _bStream, const uint16* _piPrim, int _nPrim);
	int Geometry_BuildVertexUsage(int _bStream, const uint16* _pPrim, int _nPrim, uint16*& _piVRet);
	int Geometry_BuildVertexUsageMap(int _bStream, const uint16* _pPrim, int _nPrim, uint16*& _piVRet, uint16 *&_pIndexRemap, int _MaxV);
	int Geometry_BuildVertexUsage2(int _bStream, const uint16* _pPrim, int _nPrim, uint16*& _piVRet, int _MaxV);
	bool Geometry_BuildVertexUsageAndTriangleList_Stream(CRCPrimStreamIterator &_StreamIterate, uint16* _pUsage, int &_UsageSize, uint16* _pRetPrimList, int &_RetListSize, int StartVertexIndex);
	bool Geometry_BuildVertexUsageAndTriangleList_List(const uint16* &_pPrimList, int &_nIndices, uint16* _pUsage, int &_UsageSize, uint16* _pRetPrimList, int &_RetListSize, int StartVertexIndex, int PrimitiveSize);

	int Geometry_GetVertexCount(const uint16* _pIndices, int _nIndices);
	int Geometry_GetVertexCount_Primitives(const uint16* _pPrimStream, int _StreamLen);
	static int Geometry_GetComponentMask(const CRC_VertexBuffer& _VB);
	void Geometry_MoveToClipBuffer(int _iV, int _iClip);

	DRenderVirtual void Internal_IndexedTriangles2Wires(uint16* _pPrim, int _nTri);
	DRenderVirtual void Internal_IndexedTriStrip2Wires(uint16* _pPrim, int _Len);
	DRenderVirtual void Internal_IndexedTriFan2Wires(uint16* _pPrim, int _Len);
 	DRenderVirtual void Internal_IndexedPrimitives2Wires(uint16* _pPrimStream, int _StreamLen);

	// Console add-ins
	void Con_EnableFlags(int _Flags);
	void Con_DisableFlags(int _Flags);
	void Con_Filter(int _Filter);
	void Con_EchoZFormats();
	void Con_EchoDisplayFormats();
	void Con_EchoTextureFormats();
	void Con_ShowGamma(int _v);
	void Con_DumpTextures(int _v);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Implemented from interface
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void Create(CObj* _pContext, const char* _pParams);

	CMTime m_LastTime;
	CRC_Statistics m_Stats;
	DRenderVirtual CRC_Statistics Statistics_Get() 
	{
		CMTime Now = CMTime::GetCPU();
		m_Stats.m_Time_FrameTime.AddData((Now - m_LastTime).GetTime());
		m_LastTime = Now;
		return m_Stats;
	}

	const char * GetRenderingStatus();
	const char * GetRenderingStatus(int _iStatus)
	{
		if (_iStatus == 0)
			return GetRenderingStatus();
		return "";
	};

	void BeginScene(CRC_Viewport* _pVP);
	void EndScene();

	// Viewport
	void Viewport_Update();
	CRC_Viewport* Viewport_Get();
	void Viewport_Set(CRC_Viewport* _pVP);
	void Viewport_Push();
	void Viewport_Pop();

	// Render target
	void RenderTarget_Clear(CRct _ClearRect, int _WhatToClear, CPixel32 _Color, fp4 _ZBufferValue, int _StecilValue)
	{
		Error_static("RenderTarget_Clear","Implement this");
	}
	void RenderTarget_SetNextClearParams(CRct _ClearRect, int _WhatToClear, CPixel32 _Color, fp4 _ZBufferValue, int _StecilValue)
	{
	}

	DRenderVirtual void RenderTarget_Copy(CRct _SrcRect, CPnt _Dest, int _CopyType);
	DRenderVirtual void RenderTarget_CopyToTexture(int _TextureID, CRct _SrcRect, CPnt _Dest, bint _bContinueTiling, uint16 _Slice);

	void Render_EnableHardwareMemoryRegion(void *_pMemStart, mint _Size)
	{
	}

	CXR_VBManager *m_pRegionManager;
	CXR_VBMContainer *m_pRegionContainer;
	void Render_EnableHardwareMemoryRegion(CXR_VBManager *_pManager, CXR_VBMContainer *_pContainer);

	void Render_DisableHardwareMemoryRegion();

	// Caps
	int Caps_Flags() { return m_Caps_Flags; };
	int Caps_TextureFormats() { return m_Caps_TextureFormats; };		// No need, automatic
	int Caps_DisplayFormats() { return m_Caps_DisplayFormats; };		// No need, automatic
	int Caps_StencilDepth() { return m_Caps_StencilDepth; };

	void Render_PrecacheFlush( ) {}
	void Render_PrecacheEnd() {}
	void Render_SetRenderOptions(uint32 _Options, uint32 _Format) {}


	// Texture stuff
	CTextureContext* Texture_GetTC();
	class CRC_TCIDInfo* Texture_GetTCIDInfo();
	void Texture_Precache(int _TextureID);
	void Texture_Copy(int _SourceTexID, int _DestTexID, CRct _SrcRgn, CPnt _DstPos);	// Not implemented.
	CRC_TextureMemoryUsage Texture_GetMem(int _TextureID);
	int Texture_GetPicmipFromGroup(int _iPicmip);

	void Texture_BlockUntilStreamingTexturesDone(){};

	// VertexBuffer stuff
	CXR_VBContext* VB_GetVBContext();
	class CRC_VBIDInfo* VB_GetVBIDInfo();

	// Attribute
	void Attrib_Set(const CRC_Attributes& _Attrib);
	void Attrib_Get(CRC_Attributes& _Attrib) const;

	CRC_Attributes* Attrib_Begin();
	void Attrib_End(uint _ChgFlags);

	void Attrib_Lock(int _Flags);
	void Attrib_LockFlags(int _Flags);

	void Attrib_Push();
	void Attrib_Pop();

	void Attrib_Enable(int _Flags);
	void Attrib_Disable(int _Flags);
	void Attrib_Switch(int _Flags);
	void Attrib_ZCompare(int _Compare);
	void Attrib_AlphaCompare(int _Compare, int _AlphaRef);
	void Attrib_StencilRef(int _Ref, int _FuncAnd);
	void Attrib_StencilWriteMask(int _Mask);
	void Attrib_StencilFrontOp(int _Func, int _OpFail, int _OpZFail, int _OpZPass);
	void Attrib_StencilBackOp(int _Func, int _OpFail, int _OpZFail, int _OpZPass);
	void Attrib_RasterMode(int _Mode);			// RasterMode preceeds Blend enable, SrcBlend & DestBlend

	void Attrib_SourceBlend(int _Blend);
	void Attrib_DestBlend(int _Blend);
	void Attrib_FogColor(CPixel32 _FogColor);
	void Attrib_FogStart(fp4 _FogStart);
	void Attrib_FogEnd(fp4 _FogEnd);
	void Attrib_FogDensity(fp4 _FogDensity);
	void Attrib_PolygonOffset(fp4 _Scale, fp4 _Offset);
	void Attrib_Scissor(const CRect2Duint16& _Scissor);

	// Light state
	void Attrib_Lights(const CRC_Light* _pLights, int _nLights);	// _pLights must be valid as long as the light-state is in use.

	// Texture state
	void Attrib_TextureID(int _iTxt, int _TextureID);
	void Attrib_TexEnvMode(int _iTxt, int _TexEnvMode);
//	void Attrib_TexEnvColor(int _iTxt, CPixel32 _TexEnvColor);

	void Attrib_TexGen(int _iTxt, int _TexGen, int _Comp);
	void Attrib_TexGenAttr(fp4* _pAttr);

	void Attrib_VBFlags(uint32 _Flags);

	// Global attributes
	void Attrib_GlobalEnable(int _Flags);
	void Attrib_GlobalDisable(int _Flags);
	void Attrib_GlobalSwitch(int _Flags);
	void Attrib_GlobalSetVar(int _Var, int _Value);
	void Attrib_GlobalSetVarfv(int _Var, const fp4* _pValues);
	int Attrib_GlobalGetVar(int _Var);
	int Attrib_GlobalGetVarfv(int _Var, fp4* _pValues);

	// Transform
	void Matrix_SetMode(int _iMode);
	void Matrix_Push();
	void Matrix_Pop();
	void Matrix_SetUnit();
	void Matrix_Set(const CMat4Dfp4& _Matrix);
	void Matrix_Get(CMat4Dfp4& _Matrix);
	void Matrix_Multiply(const CMat4Dfp4& _Matrix);
	void Matrix_MultiplyInverse(const CMat4Dfp4& _Matrix);
	void Matrix_PushMultiply(const CMat4Dfp4& _Matrix);
	void Matrix_PushMultiplyInverse(const CMat4Dfp4& _Matrix);
	void Matrix_SetPalette(const CRC_MatrixPalette* _pMatrixPaletteArgs);

#ifndef DEFINE_MAT43_IS_MAT4D
	void Matrix_Set(const CMat43fp4& _Matrix);
	void Matrix_Multiply(const CMat43fp4& _Matrix);
	void Matrix_MultiplyInverse(const CMat43fp4& _Matrix);
	void Matrix_PushMultiply(const CMat43fp4& _Matrix);
	void Matrix_PushMultiplyInverse(const CMat43fp4& _Matrix);
#endif

	// Clipping
	void Clip_Push();
	void Clip_Pop();
	void Clip_Clear();
	void Clip_Set(const CPlane3Dfp4* _pPlanes, int _nPlanes);
	void Clip_AddPlane(const CPlane3Dfp4& _Plane, const CMat4Dfp4* _pTransform = NULL, bool _bClipBack = true);

	// Index-array rendering.
	void Geometry_VertexArray(const CVec3Dfp4* _pV, int _nVertices = 0, int _bAllUsed = true);
	void Geometry_NormalArray(const CVec3Dfp4* _pN);
	void Geometry_TVertexArray(const fp4* _pTV, int _TxtChannel = 0, int _nComp = 2);
	void Geometry_TVertexArray(const CVec2Dfp4* _pTV, int _TxtChannel = 0);
	void Geometry_TVertexArray(const CVec3Dfp4* _pTV, int _TxtChannel = 0);
	void Geometry_TVertexArray(const CVec4Dfp4* _pTV, int _TxtChannel = 0);
	void Geometry_ColorArray(const CPixel32* _pCol);
	void Geometry_SpecularArray(const CPixel32* _pCol);
//	void Geometry_FogArray(const fp4* _pFog);					// No longer supported. /mh
	void Geometry_MatrixIndexArray(const uint32* _pMI);
	void Geometry_MatrixWeightArray(const fp4* _pMW, int _nComp);
	void Geometry_Color(CPixel32 _Col);

	void Geometry_VertexBuffer(const CRC_VertexBuffer& _VB, int _bAllUsed);
	void Geometry_VertexBuffer(int _VBID, int _bAllUsed);

	void Geometry_Clear();


	void Geometry_PrecacheFlush( );
	void Geometry_PrecacheBegin( int _Count );
	void Geometry_Precache(int _VBID);
	void Geometry_PrecacheEnd();

	void Render_IndexedTriangles(uint16* _pTriVertIndices, int _nTriangles);
 	void Render_IndexedTriangleStrip(uint16* _pIndices, int _Len);
 	void Render_IndexedWires(uint16* _pIndices, int _Len);
 	void Render_IndexedPolygon(uint16* _pIndices, int _Len);

 	void Render_IndexedPrimitives(uint16* _pPrimStream, int _StreamLen);

	void Render_VertexBuffer(int _VBID);

	// Wire
	void Render_Wire(const CVec3Dfp4& _v0, const CVec3Dfp4& _v1, CPixel32 _Color);
	void Render_WireStrip(const CVec3Dfp4* _pV, const uint16* _piV, int _nVertices, CPixel32 _Color);
	void Render_WireLoop(const CVec3Dfp4* _pV, const uint16* _piV, int _nVertices, CPixel32 _Color);

	// Occlusion query
	void OcclusionQuery_Begin(int _QueryID);
	void OcclusionQuery_End();
	int OcclusionQuery_GetVisiblePixelCount(int _QueryID);			// This may be a high latency operation and it is expected that this operation is performed one frame after OcclusionQuery_End has been performed.

	// Occlusion query helper
	int OcclusionQuery_Rect(int _QueryID, CRct _Rct, fp4 _Depth);

	// Depth-buffer read
	bool ReadDepthPixels(int _x, int _y, int _w, int _h, fp4* _pBuffer);

	void Register(CScriptRegisterContext &_RegContext);

};

#ifdef M_STATIC_RENDERER
#define DRenderCoreTopClass CRC_CoreVirtualImp::
class CRC_CoreVirtualImp
{
public:
	static void Internal_RenderPolygon(int _nV, const CVec3Dfp4* _pV, const CVec3Dfp4* _pN, const CVec4Dfp4* _pCol = NULL, const CVec4Dfp4* _pSpec = NULL, 
		const CVec4Dfp4* _pTV0 = NULL, const CVec4Dfp4* _pTV1 = NULL, const CVec4Dfp4* _pTV2 = NULL, const CVec4Dfp4* _pTV3 = NULL, int _Color = 0xffffffff);

	static void PreEndScene();
	static void Texture_MakeAllDirty(int _iPicMip = -1);

	// Internals, don't use.
	static void Attrib_Update();
	static void Attrib_GlobalUpdate();
	static void Attrib_Set(CRC_Attributes* _pAttrib) DRenderPost;
	static void Attrib_SetAbsolute(CRC_Attributes* _pAttrib); 

	static void Matrix_SetRender(int _iMode, const CMat4Dfp4* _pMatrix);
	static void Matrix_Update();

	static void Internal_IndexedTriangles2Wires(uint16* _pPrim, int _nTri);
	static void Internal_IndexedTriStrip2Wires(uint16* _pPrim, int _Len);
	static void Internal_IndexedTriFan2Wires(uint16* _pPrim, int _Len);
 	static void Internal_IndexedPrimitives2Wires(uint16* _pPrimStream, int _StreamLen);
};
#else
#define DRenderCoreTopClass
#endif
#endif // _INC_MRCCore

