#ifndef __XRVERTEXBUFFER_H
#define __XRVERTEXBUFFER_H

#include "../MOS.h"
#include "XRVertexBuffer_VPUShared.h"


// -------------------------------------------------------------------
//  CXR_VertexBuffer
// -------------------------------------------------------------------

// Content flags for use with Alloc_VB()

enum
{
	CXR_VB_ATTRIB		= 1,
	CXR_VB_VERTICES		= 2,
	CXR_VB_COLORS		= 4,
	CXR_VB_DIFFUSE		= 8,
	CXR_VB_SPECULAR		= 16,
//	CXR_VB_FOG			= 512,

	// These needs to be last!
	CXR_VB_TVERTICES0	= 32,
	CXR_VB_TVERTICES1	= 64,
	CXR_VB_TVERTICES2	= 128,
	CXR_VB_TVERTICES3	= 256,
	
	CXR_VB_TVERTICESALL	= 32+64+128+256
};

enum
{
	CXR_VBFLAGS_TRACKVERTEXUSAGE = M_Bit(0),
	CXR_VBFLAGS_COLORBUFFER = M_Bit(1),		// PS2 Specific flag!
	CXR_VBFLAGS_LIGHTSCISSOR = M_Bit(2),
	CXR_VBFLAGS_VBIDCHAIN = M_Bit(3),
	CXR_VBFLAGS_VBCHAIN = M_Bit(4),
	CXR_VBFLAGS_PRERENDER = M_Bit(5),
	CXR_VBFLAGS_VIRTUALATTRIBUTES = M_Bit(6),
	CXR_VBFLAGS_FORCENOCLIP = M_Bit(7),
	CXR_VBFLAGS_OGR_SELECTED = 16384,
};

class CXR_VertexBuffer;
class CXR_VBMScope;


// -------------------------------------------------------------------
class CXR_VirtualAttributes
{
public:
	uint32 m_Class;
	CRC_Attributes* m_pBaseAttrib;
	virtual int OnCompare(const CXR_VirtualAttributes* _pOther) pure;
	virtual uint OnSetAttributes(CRC_Attributes* _pDest, const CXR_VirtualAttributes* _pLastAttr) pure;
};

#define XR_COMPAREATTRIBUTE_INT(a,b) { if ((a) < (b)) return -1; else if ((a) > (b)) return 1; }

// -------------------------------------------------------------------

typedef void(*PFN_VERTEXBUFFER_PRERENDER)(CRenderContext* _pRC, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext, CXR_VBMScope* _pScope, int _Flags);

class CXR_VBIDChain
{
public:
	CXR_VBIDChain* m_pNextVB;
	uint16* m_piPrim;

	union
	{
		struct
		{
			uint16 m_VBID;
			uint16 m_nPrim;
			int16 m_PrimType;
			uint16 m_TaskId;
		};
		uint64 m_CopyDymmy;
	};
	uint16 m_IBID;
	uint16 m_PrimOffset;

	void CopyData(const CXR_VBIDChain& _Chain)
	{
		m_piPrim = _Chain.m_piPrim;
		m_CopyDymmy = _Chain.m_CopyDymmy;
		m_IBID = _Chain.m_IBID;
		m_PrimOffset = _Chain.m_PrimOffset;
	}

	void Clear()
	{
		m_pNextVB = NULL;
		m_piPrim = NULL;
		m_VBID = 0;
		m_nPrim = 0;
		m_PrimType = 0;
		m_TaskId = ~0;
		m_IBID = 0;
		m_PrimOffset = 0;
	}

	bool IsValid()
	{
		if (m_VBID) 
			return true;
		else
			return false;
	}

	void Render_IndexedTriangles(uint16* _pTriVertIndices, int _nTriangles)
	{
		m_piPrim = _pTriVertIndices;
		m_nPrim = _nTriangles;
		m_PrimType = CRC_RIP_TRIANGLES;
	}

 	void Render_IndexedWires(uint16* _pIndices, int _Len)
	{
		m_piPrim = _pIndices;
		m_nPrim = _Len;
		m_PrimType = CRC_RIP_WIRES;
	};

 	void Render_IndexedPrimitives(uint16* _pPrimStream, int _StreamLen)
	{
		m_piPrim = _pPrimStream;
		m_nPrim = _StreamLen;
		m_PrimType = CRC_RIP_STREAM;
	}

	void Render_VertexBuffer(uint _VBID)
	{
		m_VBID = _VBID;
		m_PrimType = CRC_RIP_VBID;
	}

	void Render_VertexBuffer_IndexBufferTriangles(uint _VBID, uint _IBID, uint _nTriangles, uint _Offset)
	{
		m_VBID = _VBID;
		m_nPrim = _nTriangles;
		m_PrimType = CRC_RIP_VBID_IBTRIANGLES;
		m_IBID = _IBID;
		m_PrimOffset = _Offset;
	}
};


class CXR_VertexBuffer_PreRender
{
public:
	PFN_VERTEXBUFFER_PRERENDER m_pfnPreRender;
	PFN_VERTEXBUFFER_PRERENDER m_pfnPostRender;
	void* m_pPreRenderContext;

	M_FORCEINLINE void Create(void* _pContext, PFN_VERTEXBUFFER_PRERENDER _pfnPreRender, PFN_VERTEXBUFFER_PRERENDER _pfnPostRender = NULL)
	{
		m_pPreRenderContext = _pContext;
		m_pfnPreRender = _pfnPreRender;
		m_pfnPostRender = _pfnPostRender;
	}
};

#ifdef _DEBUG
//	#define M_VBDEBUG
#endif

// -------------------------------------------------------------------
// CXR_VertexBufferGeometry is the bare minimum data needed for 
// Render_Surface and CXR_Shader that used to be sent in a full 
// CXR_VertexBuffer structure.
// -------------------------------------------------------------------
class CXR_VertexBufferGeometry	// 20 bytes, keep this small ffs!
{
public:
	uint16 m_Flags;				// Need flags in this structure because it tells us what type m_pVBChain is
	uint16 m_iLight;			// Used when CXR_VBFLAGS_LIGHTSCISSOR is enabled. In this structure only because of packing reasons.
	void* m_pVBChain;
	const CRC_MatrixPalette *m_pMatrixPaletteArgs;
	const CMat4Dfp32* m_pTransform; 
	uint32 m_Color;				// Because CPixel32 sucks

	M_FORCEINLINE CXR_VertexBufferGeometry()
	{
		// Nothing! Keep it that way
	}

	M_FORCEINLINE void Clear()
	{
		m_Flags = 0;
		m_iLight = 0;
		m_pVBChain = NULL;
		m_pMatrixPaletteArgs = NULL;
		m_pTransform = NULL;
		m_Color = 0xffffffff;
	}

	// NOTE: Do not Clear() before Create(), Create will overwrite everything.
	M_FORCEINLINE void Create(CXR_VBChain* _pChain, CMat4Dfp32* _pTransform)
	{
		m_Flags = CXR_VBFLAGS_VBCHAIN;
		m_iLight = 0;
		m_pVBChain = _pChain;
		m_pMatrixPaletteArgs = NULL;
		m_pTransform = _pTransform;
		m_Color = 0xffffffff;
	}

	// NOTE: Do not Clear() before Create(), Create will overwrite everything.
	M_FORCEINLINE void Create(CXR_VBIDChain* _pChain, CMat4Dfp32* _pTransform)
	{
		m_Flags = CXR_VBFLAGS_VBIDCHAIN;
		m_iLight = 0;
		m_pVBChain = _pChain;
		m_pMatrixPaletteArgs = NULL;
		m_pTransform = _pTransform;
		m_Color = 0xffffffff;
	}

	M_FORCEINLINE void SetFlags(uint32 _Clear, uint32 _Add)
	{
		m_Flags = (m_Flags & (~_Clear)) | _Add;
	}

	M_FORCEINLINE  CXR_VBIDChain* GetVBIDChain() const
	{ 
#ifdef	PLATFORM_PS2
		if( m_Flags & CXR_VBFLAGS_COLORBUFFER )
			return NULL;
#endif
#ifdef M_Profile
		if ((m_Flags & (CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN)) != CXR_VBFLAGS_VBIDCHAIN)
			M_BREAKPOINT;
#endif
		M_ASSERT((m_Flags & (CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN)) == CXR_VBFLAGS_VBIDCHAIN, "!"); 
		return (CXR_VBIDChain*) m_pVBChain; 
	};

	M_FORCEINLINE  CXR_VBChain* GetVBChain() const
	{
#ifdef	PLATFORM_PS2
		if( m_Flags & CXR_VBFLAGS_COLORBUFFER )
			return NULL;
#endif
#ifdef M_Profile
		if ((m_Flags & (CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN)) != CXR_VBFLAGS_VBCHAIN)
			M_BREAKPOINT;
#endif
		M_ASSERT((m_Flags & (CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN)) == CXR_VBFLAGS_VBCHAIN, "!"); 
		return (CXR_VBChain*) m_pVBChain; 
	};

	M_FORCEINLINE void CopyVBChain(const CXR_VertexBufferGeometry*_pSrc)
	{
		m_pVBChain = _pSrc->m_pVBChain;
		m_Flags = (m_Flags & (~(CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN | CXR_VBFLAGS_TRACKVERTEXUSAGE))) | (_pSrc->m_Flags & (CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN | CXR_VBFLAGS_TRACKVERTEXUSAGE));
	}

	M_FORCEINLINE void CopyVBChainAndTransform(const CXR_VertexBufferGeometry*_pSrc)
	{
		m_pVBChain = _pSrc->m_pVBChain;
		m_Flags = (m_Flags & (~(CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN | CXR_VBFLAGS_TRACKVERTEXUSAGE))) | (_pSrc->m_Flags & (CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN | CXR_VBFLAGS_TRACKVERTEXUSAGE));
		m_pMatrixPaletteArgs = _pSrc->m_pMatrixPaletteArgs;
		m_pTransform = _pSrc->m_pTransform;
	}

	M_FORCEINLINE void SetVBChain(CXR_VBChain *_pVBChain)
	{
		M_ASSERT((m_Flags & (CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN)) != CXR_VBFLAGS_VBIDCHAIN, "!"); 
		m_pVBChain = _pVBChain;
		m_Flags |= CXR_VBFLAGS_VBCHAIN;
	}

	M_FORCEINLINE void SetVBIDChain(CXR_VBIDChain *_pVBChain)
	{
		M_ASSERT((m_Flags & (CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN)) != CXR_VBFLAGS_VBCHAIN, "!"); 
		m_pVBChain = _pVBChain;
		m_Flags |= CXR_VBFLAGS_VBIDCHAIN;
	}

	M_FORCEINLINE bool IsVBIDChain() const
	{
		return (m_Flags & CXR_VBFLAGS_VBIDCHAIN) != 0;
	}

	M_FORCEINLINE bool IsVBChain() const
	{
		return (m_Flags & CXR_VBFLAGS_VBCHAIN) != 0;
	}

	bool AllocVBChain(CXR_VBManager *_pVBM, bool _bVBIDs);
};

// -------------------------------------------------------------------
class CXR_VertexBuffer : public CXR_VertexBufferGeometry	// 72 bytes in M_Profile, 52 in RTM
{
public:
	DLinkDA_Link(CXR_VertexBuffer, m_Link);

	fp32 m_Priority;

	union
	{
		CRC_Attributes* m_pAttrib;
		CXR_VirtualAttributes* m_pVAttrib;
	};
	const CXR_VertexBuffer_PreRender *m_pPreRender;

	union
	{
		struct
		{
			uint8 m_iVP;
			uint8 m_iClip;
		};
		uint16 m_iVPClip;
	};

#ifdef M_Profile
	uint32 m_VBEColor;		// VB explorer color
	CMTime m_Time;
	CMTime m_GPUTime;

	M_FORCEINLINE void SetVBEColor(uint32 _VBEColor) { m_VBEColor = _VBEColor; };
#else
	M_FORCEINLINE void SetVBEColor(uint32 _VBEColor) { };
#endif

	const CMat4Dfp32** m_pTextureTransform;

#ifdef M_VBDEBUG
	const char *m_pFile;
	int m_Line;
#endif

	void SetVirtualAttr(CXR_VirtualAttributes* _pVA)
	{
		m_pVAttrib = _pVA;
		m_Flags |= CXR_VBFLAGS_VIRTUALATTRIBUTES;
	}

	bool AllocTextureMatrix(CXR_VBManager *_pVBM);

	void Clear();
	void Construct()
	{
#ifdef M_Profile
		m_Link.Construct();
#endif
		m_Color = 0xffffffff;
		Clear();
	}

	CXR_VertexBuffer()
	{
		Construct();
	}

	// Construct must initialize everything
	M_FORCEINLINE void Construct_Geometry_VA_Priority(const CXR_VertexBufferGeometry* _pSrc, CXR_VirtualAttributes* _pVA, fp32 _Priority)
	{
#ifdef M_Profile
		m_Link.Construct();
#endif
		m_pVBChain = _pSrc->m_pVBChain;
		m_Flags = _pSrc->m_Flags | CXR_VBFLAGS_VIRTUALATTRIBUTES;
		m_pMatrixPaletteArgs = _pSrc->m_pMatrixPaletteArgs;
		m_pTransform = _pSrc->m_pTransform;
		m_Color = _pSrc->m_Color;
		m_Priority = _Priority;
		m_pVAttrib = _pVA;
		m_pPreRender = 0;
		m_iVPClip = 0;
		m_pTextureTransform = 0;
#ifdef M_Profile
		m_VBEColor = 0xff808080;
		m_Time.Reset();
#endif
	}

	M_FORCEINLINE void Matrix_Set(const CMat4Dfp32* _pMat)
	{
		m_pTransform = _pMat;
	}

	M_FORCEINLINE void Matrix_Unit()
	{
		m_pTransform = NULL;
	}

	void TextureMatrix_Set(int _iTex, const CMat4Dfp32* _pMat)
	{
		m_pTextureTransform[_iTex] = _pMat;
	}

	void TextureMatrix_Unit(int _iTex, const CMat4Dfp32* _pMat)
	{
		m_pTextureTransform[_iTex] = NULL;
	}

	M_FORCEINLINE void Geometry_VertexArray(CVec3Dfp32* _pV, int _nVertices, int _bAllUsed)
	{
		CXR_VBChain* pChain = GetVBChain();
		pChain->m_pV = _pV;
		pChain->m_nV = _nVertices;
		if (!_bAllUsed)
			m_Flags |= CXR_VBFLAGS_TRACKVERTEXUSAGE;
	}

	M_FORCEINLINE void Geometry_Normal(CVec3Dfp32* _pN)
	{
		CXR_VBChain* pChain = GetVBChain();
		pChain->m_pN = _pN;
	}

	M_FORCEINLINE void Geometry_MatrixIndices(uint32* _pMI)
	{
		CXR_VBChain* pChain = GetVBChain();
		pChain->m_pMI = _pMI;
	}

	M_FORCEINLINE void Geometry_MatrixWeights(fp32* _pMW)
	{
		CXR_VBChain* pChain = GetVBChain();
		pChain->m_pMW = _pMW;
	}

	M_FORCEINLINE void Geometry_TVertexArray(fp32* _pTV, int _TxtChannel, int _nComp)
	{
		CXR_VBChain* pChain = GetVBChain();
		pChain->m_pTV[_TxtChannel] = _pTV;
		pChain->m_nTVComp[_TxtChannel] = _nComp;
	}

	M_FORCEINLINE void Geometry_TVertexArray(CVec2Dfp32* _pTV, int _TxtChannel)
	{
		CXR_VBChain* pChain = GetVBChain();
		pChain->m_pTV[_TxtChannel] = (fp32*)_pTV;
		pChain->m_nTVComp[_TxtChannel] = 2;
	}

	M_FORCEINLINE void Geometry_TVertexArray(CVec3Dfp32* _pTV, int _TxtChannel)
	{
		CXR_VBChain* pChain = GetVBChain();
		pChain->m_pTV[_TxtChannel] = (fp32*)_pTV;
		pChain->m_nTVComp[_TxtChannel] = 3;
	}

	M_FORCEINLINE void Geometry_ColorArray(CPixel32* _pCol)
	{
		CXR_VBChain* pChain = GetVBChain();
		pChain->m_pCol = _pCol;
	}

	M_FORCEINLINE void Geometry_SpecularArray(CPixel32* _pSpec)
	{
		CXR_VBChain* pChain = GetVBChain();
		pChain->m_pSpec = _pSpec;
	}

/*	void Geometry_FogArray(fp32* _pFog)
	{
		m_pFog= _pFog;
	}*/

	M_FORCEINLINE void Geometry_Color(CPixel32 _Col)
	{
		m_Color = _Col;
	}

	M_FORCEINLINE void Geometry_VertexBuffer(int _VBID, int _bAllUsed)
	{
		CXR_VBIDChain* pChain = GetVBIDChain();
//		pChain->m_PrimType	= CRC_RIP_VBID;
		pChain->m_VBID = _VBID;
		if (!_bAllUsed)
			m_Flags |= CXR_VBFLAGS_TRACKVERTEXUSAGE;
	}

	M_FORCEINLINE void Render_IndexedTriangles(uint16* _pTriVertIndices, int _nTriangles)
	{
		if (m_Flags & CXR_VBFLAGS_VBIDCHAIN)
		{
			GetVBIDChain()->Render_IndexedTriangles(_pTriVertIndices, _nTriangles);
		}
		else if (m_Flags & CXR_VBFLAGS_VBCHAIN)
		{
			GetVBChain()->Render_IndexedTriangles(_pTriVertIndices, _nTriangles);
		}
		else
		{
#ifdef M_Profile
			M_BREAKPOINT;
#endif
		}
	}

	M_FORCEINLINE void Render_SetVPUTask(uint32 _Task)
	{
		if (m_Flags & CXR_VBFLAGS_VBIDCHAIN)
		{
			GetVBIDChain()->m_TaskId = _Task;
		}
		else if (m_Flags & CXR_VBFLAGS_VBCHAIN)
		{
			GetVBChain()->m_TaskId = _Task;
		}
		else
		{
#ifdef M_Profile
			M_BREAKPOINT;
#endif
		}
	}

 	M_FORCEINLINE void Render_IndexedWires(uint16* _pIndices, int _Len)
	{
		if (m_Flags & CXR_VBFLAGS_VBIDCHAIN)
		{
			GetVBIDChain()->Render_IndexedWires(_pIndices, _Len);
		}
		else if (m_Flags & CXR_VBFLAGS_VBCHAIN)
		{
			GetVBChain()->Render_IndexedWires(_pIndices, _Len);
		}
		else
		{
#ifdef M_Profile
			M_BREAKPOINT;
#endif
		}
	};

 	M_FORCEINLINE void Render_IndexedPrimitives(uint16* _pPrimStream, int _StreamLen)
	{
		if (m_Flags & CXR_VBFLAGS_VBIDCHAIN)
		{
			GetVBIDChain()->Render_IndexedPrimitives(_pPrimStream, _StreamLen);
		}
		else if (m_Flags & CXR_VBFLAGS_VBCHAIN)
		{
			GetVBChain()->Render_IndexedPrimitives(_pPrimStream, _StreamLen);
		}
		else
		{
#ifdef M_Profile
			M_BREAKPOINT;
#endif
		}
	}

	M_FORCEINLINE void Render_VertexBuffer(int _VBID) // , int _bAllUsed
	{
		GetVBIDChain()->Render_VertexBuffer(_VBID);
	}

	M_FORCEINLINE void Render_VertexBuffer_IndexBufferTriangles(uint _VBID, uint _IBID, uint _nTriangles, uint _Offset)
	{
		GetVBIDChain()->Render_VertexBuffer_IndexBufferTriangles(_VBID, _IBID, _nTriangles, _Offset);
	}

	M_FORCEINLINE bool AllUsed()
	{
		return !(m_Flags & CXR_VBFLAGS_TRACKVERTEXUSAGE);
	}

/*	void SetFlags(int _Flags)
	{
		m_Flags = _Flags;
	}*/

	M_FORCEINLINE bool IsValid()
	{
#ifdef PLATFORM_PS2
		if( m_Flags & CXR_VBFLAGS_COLORBUFFER ) return true;
#endif

		if( m_Flags & CXR_VBFLAGS_PRERENDER ) 
			return true;

		if (!m_pAttrib)
			return false;

		if (m_Flags & CXR_VBFLAGS_VBIDCHAIN)
		{
			return GetVBIDChain()->IsValid();
		}
		else if (m_Flags & CXR_VBFLAGS_VBCHAIN)
		{
			return GetVBChain()->IsValid();
		}
		else
			return false;

		return true;
	}

	void SetMatrix(CRenderContext* _pRC);
	void SetAttrib(CRenderContext* _pRC);
	int RenderGeometry(CRenderContext* _pRC);
	int Render(CRenderContext* _pRC);

//	int GetChainLen() const;

//	bool BuildVertexUsage(CXR_VBManager* _pVBM);
};


#endif
