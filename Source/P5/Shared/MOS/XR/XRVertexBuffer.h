#ifndef __XRVERTEXBUFFER_H
#define __XRVERTEXBUFFER_H

#include "../MOS.h"

#define CRC_RIP_STREAM		-1
#define CRC_RIP_WIRES		-2
#define CRC_RIP_VBID		-3

// -------------------------------------------------------------------
//  CXR_VertexBuffer
// -------------------------------------------------------------------

// Content flags for use with Alloc_VB()

enum
{
	CXR_VB_ATTRIB = 1,
	CXR_VB_VERTICES = 2,
	CXR_VB_TVERTICES0 = 4,
	CXR_VB_TVERTICES1 = 8,
	CXR_VB_TVERTICES2 = 16,
	CXR_VB_TVERTICES3 = 32,
	CXR_VB_COLORS = 64,
	CXR_VB_DIFFUSE = 128,
	CXR_VB_SPECULAR = 256,
//	CXR_VB_FOG = 512,
	

	CXR_VB_TVERTICESALL = 4+8+16+32
};

enum
{
	CXR_VBFLAGS_TRACKVERTEXUSAGE = DBit(0),
	CXR_VBFLAGS_COLORBUFFER = DBit(1),		// PS2 Specific flag!
	CXR_VBFLAGS_LIGHTSCISSOR = DBit(2),
	CXR_VBFLAGS_VBIDCHAIN = DBit(3),
	CXR_VBFLAGS_VBCHAIN = DBit(4),
	CXR_VBFLAGS_PRERENDER = DBit(5),
	CXR_VBFLAGS_VIRTUALATTRIBUTES = DBit(6),
	CXR_VBFLAGS_FORCENOCLIP = DBit(7),
	CXR_VBFLAGS_OGR_SELECTED = 16384,
};

class CXR_VBManager;
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
	uint16 m_VBID;
	uint16 m_nPrim;
	int16 m_PrimType;

	void Clear()
	{
		m_pNextVB = NULL;
		m_piPrim = NULL;
		m_VBID = 0;
		m_nPrim = 0;
		m_PrimType = 0;
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

	void Render_VertexBuffer(int _VBID) // , int _bAllUsed
	{
		m_VBID = _VBID;
		m_PrimType = CRC_RIP_VBID;
	}

};

class CXR_VBChain : public CRC_VertexBuffer 
{
public:
	CXR_VBChain* m_pNextVB;
	uint16* m_piVertUse;
	int32 m_nVertUse;
	void Clear()
	{
		CRC_VertexBuffer::Clear();
		m_pNextVB = NULL;
		m_piVertUse = NULL;
		m_nVertUse = 0;
	}

	bool BuildVertexUsage(CXR_VBManager* _pVBM);

	bool IsValid()
	{
		if (!m_piPrim || !m_nPrim) 
			return false;
		if (!m_pV) 
			return false;

		return true;
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
};

class CXR_VertexBuffer_PreRender
{
public:
	PFN_VERTEXBUFFER_PRERENDER m_pfnPreRender;
	void* m_pPreRenderContext;
};

class CXR_VertexBuffer
{
public:
	void* m_pVBChain;
	fp4 m_Priority;

	union
	{
		CRC_Attributes* m_pAttrib;
		CXR_VirtualAttributes* m_pVAttrib;
	};
	const CRC_MatrixPalette *m_pMatrixPaletteArgs;
	const CXR_VertexBuffer_PreRender *m_pPreRender;

	uint16 m_Flags;
	uint16 m_iLight;		// Used when CXR_VBFLAGS_LIGHTSCISSOR is enabled.
	uint8 m_iVP;
	uint8 m_iClip;

	// 7 DWORDs

//	const CMat4Dfp4* m_pTransform[CRC_MATRIXSTACKS]; // 6 of them from hell
	CPixel32 m_Color; 

	const CMat4Dfp4* m_pTransform; 
	const CMat4Dfp4** m_pTextureTransform;
	// 2 DWORDs

	M_INLINE CXR_VBIDChain* GetVBIDChain() const
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
	
	M_INLINE CXR_VBChain* GetVBChain() const
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

	void CopyVBChain(const CXR_VertexBuffer *_pSrc)
	{
		m_pVBChain = _pSrc->m_pVBChain;
		m_Flags = (m_Flags & (~(CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN | CXR_VBFLAGS_TRACKVERTEXUSAGE))) | (_pSrc->m_Flags & (CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN | CXR_VBFLAGS_TRACKVERTEXUSAGE));
	}

	void SetVBChain(CXR_VBChain *_pVBChain)
	{
		M_ASSERT((m_Flags & (CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN)) != CXR_VBFLAGS_VBIDCHAIN, "!"); 
		m_pVBChain = _pVBChain;
		m_Flags |= CXR_VBFLAGS_VBCHAIN;
	}

	void SetVBIDChain(CXR_VBIDChain *_pVBChain)
	{
		M_ASSERT((m_Flags & (CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN)) != CXR_VBFLAGS_VBCHAIN, "!"); 
		m_pVBChain = _pVBChain;
		m_Flags |= CXR_VBFLAGS_VBIDCHAIN;
	}

	bool SetVBChain(CXR_VBManager *_pVBM, bool _bVBIDs);

	void SetFlags(uint32 _Clear, uint32 _Add)
	{
		m_Flags = (m_Flags & (~_Clear)) | _Add;
	}

	void SetVirtualAttr(CXR_VirtualAttributes* _pVA)
	{
		m_pVAttrib = _pVA;
		m_Flags |= CXR_VBFLAGS_VIRTUALATTRIBUTES;
	}

	bool AllocTextureMatrix(CXR_VBManager *_pVBM);

	bool IsVBIDChain() const
	{
		return (m_Flags & CXR_VBFLAGS_VBIDCHAIN) != 0;
	}

	void Clear();

	CXR_VertexBuffer()
	{
		m_Color = 0xffffffff;
		Clear();
	}

	void Matrix_Set(const CMat4Dfp4* _pMat)
	{
		m_pTransform = _pMat;
	}

	void TextureMatrix_Set(int _iTex, const CMat4Dfp4* _pMat)
	{
		m_pTextureTransform[_iTex] = _pMat;
	}

	void Matrix_Unit()
	{
		m_pTransform = NULL;
	}

	void TextureMatrix_Unit(int _iTex, const CMat4Dfp4* _pMat)
	{
		m_pTextureTransform[_iTex] = NULL;
	}

	void Geometry_VertexArray(CVec3Dfp4* _pV, int _nVertices, int _bAllUsed)
	{
		CXR_VBChain* pChain = GetVBChain();
		pChain->m_pV = _pV;
		pChain->m_nV = _nVertices;
		if (!_bAllUsed)
			m_Flags |= CXR_VBFLAGS_TRACKVERTEXUSAGE;
	}

	void Geometry_Normal(CVec3Dfp4* _pN)
	{
		CXR_VBChain* pChain = GetVBChain();
		pChain->m_pN = _pN;
	}

	void Geometry_TVertexArray(fp4* _pTV, int _TxtChannel, int _nComp)
	{
		CXR_VBChain* pChain = GetVBChain();
		pChain->m_pTV[_TxtChannel] = _pTV;
		pChain->m_nTVComp[_TxtChannel] = _nComp;
	}

	void Geometry_TVertexArray(CVec2Dfp4* _pTV, int _TxtChannel)
	{
		CXR_VBChain* pChain = GetVBChain();
		pChain->m_pTV[_TxtChannel] = (fp4*)_pTV;
		pChain->m_nTVComp[_TxtChannel] = 2;
	}

	void Geometry_TVertexArray(CVec3Dfp4* _pTV, int _TxtChannel)
	{
		CXR_VBChain* pChain = GetVBChain();
		pChain->m_pTV[_TxtChannel] = (fp4*)_pTV;
		pChain->m_nTVComp[_TxtChannel] = 3;
	}

	void Geometry_ColorArray(CPixel32* _pCol)
	{
		CXR_VBChain* pChain = GetVBChain();
		pChain->m_pCol = _pCol;
	}

	void Geometry_SpecularArray(CPixel32* _pSpec)
	{
		CXR_VBChain* pChain = GetVBChain();
		pChain->m_pSpec = _pSpec;
	}

/*	void Geometry_FogArray(fp4* _pFog)
	{
		m_pFog= _pFog;
	}*/

	void Geometry_Color(CPixel32 _Col)
	{
		m_Color = _Col;
	}

	void Geometry_VertexBuffer(int _VBID, int _bAllUsed)
	{
		CXR_VBIDChain* pChain = GetVBIDChain();
//		pChain->m_PrimType	= CRC_RIP_VBID;
		pChain->m_VBID = _VBID;
		if (!_bAllUsed)
			m_Flags |= CXR_VBFLAGS_TRACKVERTEXUSAGE;
	}

	void Render_IndexedTriangles(uint16* _pTriVertIndices, int _nTriangles)
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

 	void Render_IndexedWires(uint16* _pIndices, int _Len)
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

 	void Render_IndexedPrimitives(uint16* _pPrimStream, int _StreamLen)
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

	void Render_VertexBuffer(int _VBID) // , int _bAllUsed
	{
		GetVBIDChain()->Render_VertexBuffer(_VBID);
	}

	bool AllUsed()
	{
		return !(m_Flags & CXR_VBFLAGS_TRACKVERTEXUSAGE);
	}

/*	void SetFlags(int _Flags)
	{
		m_Flags = _Flags;
	}*/

	bool IsValid();

	void SetMatrix(CRenderContext* _pRC);
	void SetAttrib(CRenderContext* _pRC);
	int RenderGeometry(CRenderContext* _pRC);
	int Render(CRenderContext* _pRC);

//	int GetChainLen() const;

//	bool BuildVertexUsage(CXR_VBManager* _pVBM);
};


#endif
