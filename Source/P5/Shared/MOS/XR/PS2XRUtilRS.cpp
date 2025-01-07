
#include "PCH.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Surface renderer and associated functions
					
	Author:			Magnus Högdahl
					
	Copyright:		Starbreeze Studios AB, 1998-2001
					
	Comments:		







	IF YOU WRITE IN THIS FILE WITHOUT MY PRIOR APPROVAL I WILL DEFINITELY AND TOTALLY KILL YOU!

		SERIOUSLY, YOU WILL HAVE A GOOD CHANCE AT WRECKING TONS OF STUFF WITHOUT KNOWING IT.






					
\*____________________________________________________________________________________________*/

#include "XRUtil.h"
#include "XRVertexBuffer.h"
#include "XRVBManager.h"
#include "XRSurfaceContext.h"
#include "XRVBContext.h"

#include "XRFog.h"
#include "XRVBPrior.h"

#include "MSIMD.h"
#include "MFloat.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Stuff in MImage.cpp. These should be moved to MSIMD.cpp
|__________________________________________________________________________________________________
\*************************************************************************************************/

void PPA_Mul_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);
void PPA_MulAddSaturate_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);

void PPA_Mul_RGBA32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);

// -------------------------------------------------------------------
void CalcEnvironmentMappingCheap(const CMat4Dfp32* _pMat, int _nV, const uint16* _piV, const CVec3Dfp32* _pN, CVec2Dfp32* _pTV);
void CalcEnvironmentMappingCheap(const CMat4Dfp32* _pMat, int _nV, const uint16* _piV, const CVec3Dfp32* _pN, CVec2Dfp32* _pTV)
{
	MAUTOSTRIP(CalcEnvironmentMappingCheap, MAUTOSTRIP_VOID);
	// FIXME: Optimize for SSE & 3DNow!

	if (_pMat)
	{
		const CMat4Dfp32* pM = _pMat;
		fp32 M00 = pM->k[0][0]*0.5f;
		fp32 M10 = pM->k[1][0]*0.5f;
		fp32 M20 = pM->k[2][0]*0.5f;
		fp32 M01 = pM->k[0][1]*0.5f;
		fp32 M11 = pM->k[1][1]*0.5f;
		fp32 M21 = pM->k[2][1]*0.5f;
		
		if (_piV)
			for(int v = 0; v < _nV; v++)
			{
				int iv = _piV[v];
				_pTV[iv].k[0] = M00*_pN[iv].k[0] + M10*_pN[iv].k[1] + M20*_pN[iv].k[2] + 0.5f;
				_pTV[iv].k[1] = -(M01*_pN[iv].k[0] + M11*_pN[iv].k[1] + M21*_pN[iv].k[2]) + 0.5f;
			}
		else
			for(int v = 0; v < _nV; v++)
			{
				int iv = v;
				_pTV[iv].k[0] = M00*_pN[iv].k[0] + M10*_pN[iv].k[1] + M20*_pN[iv].k[2] + 0.5f;
				_pTV[iv].k[1] = -(M01*_pN[iv].k[0] + M11*_pN[iv].k[1] + M21*_pN[iv].k[2]) + 0.5f;
			}
	}
	else
	{
		if (_piV)
			for(int v = 0; v < _nV; v++)
			{
				int iv = _piV[v];
				_pTV[iv].k[0] = _pN[iv].k[0]*0.5f + 0.5f;
				_pTV[iv].k[1] = -(_pN[iv].k[1]*0.5f) + 0.5f;
			}
		else
			for(int v = 0; v < _nV; v++)
			{
				_pTV[v].k[0] = _pN[v].k[0]*0.5f + 0.5f;
				_pTV[v].k[1] = -(_pN[v].k[1]*0.5f) + 0.5f;
			}
	}
}

static bool SetColorizedLights(CRC_Attributes* _pA, CXR_VBManager* _pVBM, CPixel32 _Color, const CRC_Light* _pLights, int _nLights)
{
	MAUTOSTRIP(SetColorizedLights, false);
	CRC_Light* pL = (CRC_Light*) _pVBM->Alloc(sizeof(CRC_Light) * _nLights);

	M_ASSERT( pL, "SetColorizedLights failed Alloc" );

	for(int i = 0; i < _nLights; i++)
	{
		pL[i] = _pLights[i];
		pL[i].m_Color *= _Color;
	}

	_pA->Attrib_Lights(pL, _nLights);

	return true;
}



#define MACRO_COPYSTATE(_pSrc, _pDst)		\
{											\
	_pDst->m_pAttrib = _pSrc->m_pAttrib;	\
	for(int i = 0; i < CRC_MATRIXSTACKS; i++)					\
		_pDst->m_pTransform[i] = _pSrc->m_pTransform[i];		\
	_pDst->m_pfnPreRender = _pSrc->m_pfnPreRender;				\
	_pDst->m_pPreRenderContext = _pSrc->m_pPreRenderContext;	\
	_pDst->m_Priority = _pSrc->m_Priority;						\
}

#ifdef PRODUCT_ENCLAVE
#define MACRO_COPYGEOMETRY(_pSrc, _pDst)	\
{											\
	_pDst->m_VBID = _pSrc->m_VBID;			\
	_pDst->m_pV = _pSrc->m_pV;				\
	_pDst->m_pN = _pSrc->m_pN;				\
	_pDst->m_nV = _pSrc->m_nV;				\
	_pDst->m_pTV[0] = _pSrc->m_pTV[0];		\
	_pDst->m_pTV[1] = _pSrc->m_pTV[1];		\
	_pDst->m_pTV[2] = _pSrc->m_pTV[2];		\
	_pDst->m_pTV[3] = _pSrc->m_pTV[3];		\
	_pDst->m_nTVComp[0] = _pSrc->m_nTVComp[0];		\
	_pDst->m_nTVComp[1] = _pSrc->m_nTVComp[1];		\
	_pDst->m_nTVComp[2] = _pSrc->m_nTVComp[2];		\
	_pDst->m_nTVComp[3] = _pSrc->m_nTVComp[3];		\
	_pDst->m_piPrim = _pSrc->m_piPrim;		\
	_pDst->m_nPrim = _pSrc->m_nPrim;		\
	_pDst->m_PrimType = _pSrc->m_PrimType;	\
	_pDst->m_pMatrixPalette = _pSrc->m_pMatrixPalette;\
	_pDst->m_nMatrixPalette = _pSrc->m_nMatrixPalette;\
}
#else
#define MACRO_COPYGEOMETRY(_pSrc, _pDst)	\
{											\
	_pDst->m_VBID = _pSrc->m_VBID;			\
	_pDst->m_pV = _pSrc->m_pV;				\
	_pDst->m_pN = _pSrc->m_pN;				\
	_pDst->m_pFog = _pSrc->m_pFog;			\
	_pDst->m_nV = _pSrc->m_nV;				\
	_pDst->m_pTV[0] = _pSrc->m_pTV[0];		\
	_pDst->m_pTV[1] = _pSrc->m_pTV[1];		\
	_pDst->m_pTV[2] = _pSrc->m_pTV[2];		\
	_pDst->m_pTV[3] = _pSrc->m_pTV[3];		\
	_pDst->m_nTVComp[0] = _pSrc->m_nTVComp[0];		\
	_pDst->m_nTVComp[1] = _pSrc->m_nTVComp[1];		\
	_pDst->m_nTVComp[2] = _pSrc->m_nTVComp[2];		\
	_pDst->m_nTVComp[3] = _pSrc->m_nTVComp[3];		\
	_pDst->m_piPrim = _pSrc->m_piPrim;		\
	_pDst->m_nPrim = _pSrc->m_nPrim;		\
	_pDst->m_PrimType = _pSrc->m_PrimType;	\
	_pDst->m_pMatrixPalette = _pSrc->m_pMatrixPalette;\
	_pDst->m_nMatrixPalette = _pSrc->m_nMatrixPalette;\
}
#endif

//	_pDst->m_nTex = _pSrc->m_nTex;			


#define MACRO_COPYGEOMETRY_VERTEXONLY(_pSrc, _pDst)	\
{											\
	_pDst->m_VBID = _pSrc->m_VBID;			\
	_pDst->m_pV = _pSrc->m_pV;				\
	_pDst->m_nV = _pSrc->m_nV;				\
	_pDst->m_piPrim = _pSrc->m_piPrim;		\
	_pDst->m_nPrim = _pSrc->m_nPrim;		\
	_pDst->m_PrimType = _pSrc->m_PrimType;	\
	_pDst->m_pMatrixPalette = _pSrc->m_pMatrixPalette;\
	_pDst->m_nMatrixPalette = _pSrc->m_nMatrixPalette;\
}

#ifdef PRODUCT_ENCLAVE
#define MACRO_COPYCHAINGEOMETRY(_pSrc, _pDst)			\
{														\
	CXR_VertexBuffer* pCopyDst = _pDst;					\
	const CXR_VertexBuffer* pCopySrc = _pSrc;			\
	while(pCopyDst && pCopySrc)							\
	{													\
		pCopyDst->m_Flags = pCopySrc->m_Flags;			\
		pCopyDst->m_VBID = pCopySrc->m_VBID;			\
		pCopyDst->m_pV = pCopySrc->m_pV;				\
		pCopyDst->m_pN = pCopySrc->m_pN;				\
		pCopyDst->m_nV = pCopySrc->m_nV;				\
		pCopyDst->m_pTV[0] = pCopySrc->m_pTV[0];		\
		pCopyDst->m_pTV[1] = pCopySrc->m_pTV[1];		\
		pCopyDst->m_pTV[2] = pCopySrc->m_pTV[2];		\
		pCopyDst->m_pTV[3] = pCopySrc->m_pTV[3];		\
		pCopyDst->m_nTVComp[0] = pCopySrc->m_nTVComp[0];\
		pCopyDst->m_nTVComp[1] = pCopySrc->m_nTVComp[1];\
		pCopyDst->m_nTVComp[2] = pCopySrc->m_nTVComp[2];\
		pCopyDst->m_nTVComp[3] = pCopySrc->m_nTVComp[3];\
		pCopyDst->m_piPrim = pCopySrc->m_piPrim;		\
		pCopyDst->m_nPrim = pCopySrc->m_nPrim;			\
		pCopyDst->m_PrimType = pCopySrc->m_PrimType;	\
		pCopyDst->m_pMatrixPalette = pCopySrc->m_pMatrixPalette;\
		pCopyDst->m_nMatrixPalette = pCopySrc->m_nMatrixPalette;\
		pCopySrc = pCopySrc->m_pNextVB;					\
		pCopyDst = pCopyDst->m_pNextVB;					\
	}													\
}
#else
#define MACRO_COPYCHAINGEOMETRY(_pSrc, _pDst)			\
{														\
	CXR_VertexBuffer* pCopyDst = _pDst;					\
	const CXR_VertexBuffer* pCopySrc = _pSrc;			\
	while(pCopyDst && pCopySrc)							\
	{													\
		pCopyDst->m_Flags = pCopySrc->m_Flags;			\
		pCopyDst->m_VBID = pCopySrc->m_VBID;			\
		pCopyDst->m_pV = pCopySrc->m_pV;				\
		pCopyDst->m_pN = pCopySrc->m_pN;				\
		pCopyDst->m_pFog = pCopySrc->m_pFog;			\
		pCopyDst->m_nV = pCopySrc->m_nV;				\
		pCopyDst->m_pTV[0] = pCopySrc->m_pTV[0];		\
		pCopyDst->m_pTV[1] = pCopySrc->m_pTV[1];		\
		pCopyDst->m_pTV[2] = pCopySrc->m_pTV[2];		\
		pCopyDst->m_pTV[3] = pCopySrc->m_pTV[3];		\
		pCopyDst->m_nTVComp[0] = pCopySrc->m_nTVComp[0];\
		pCopyDst->m_nTVComp[1] = pCopySrc->m_nTVComp[1];\
		pCopyDst->m_nTVComp[2] = pCopySrc->m_nTVComp[2];\
		pCopyDst->m_nTVComp[3] = pCopySrc->m_nTVComp[3];\
		pCopyDst->m_piPrim = pCopySrc->m_piPrim;		\
		pCopyDst->m_nPrim = pCopySrc->m_nPrim;			\
		pCopyDst->m_PrimType = pCopySrc->m_PrimType;	\
		pCopyDst->m_pMatrixPalette = pCopySrc->m_pMatrixPalette;\
		pCopyDst->m_nMatrixPalette = pCopySrc->m_nMatrixPalette;\
		pCopySrc = pCopySrc->m_pNextVB;					\
		pCopyDst = pCopyDst->m_pNextVB;					\
	}													\
}
#endif

//		pCopyDst->m_nTex = pCopySrc->m_nTex;			


#define MACRO_COPYCHAINGEOMETRY_VERTEXONLY(_pSrc, _pDst)	\
{											\
	CXR_VertexBuffer* pCopyDst = _pDst;		\
	const CXR_VertexBuffer* pCopySrc = _pSrc;		\
	while(pCopyDst && pCopySrc)			\
	{										\
		pCopyDst->m_Flags = pCopySrc->m_Flags;			\
		pCopyDst->m_VBID = pCopySrc->m_VBID;			\
		pCopyDst->m_pV = pCopySrc->m_pV;				\
		pCopyDst->m_nV = pCopySrc->m_nV;				\
		pCopyDst->m_piPrim = pCopySrc->m_piPrim;		\
		pCopyDst->m_nPrim = pCopySrc->m_nPrim;		\
		pCopyDst->m_PrimType = pCopySrc->m_PrimType;	\
		pCopyDst->m_pMatrixPalette = pCopySrc->m_pMatrixPalette;\
		pCopyDst->m_nMatrixPalette = pCopySrc->m_nMatrixPalette;\
		pCopySrc = pCopySrc->m_pNextVB;				\
		pCopyDst = pCopyDst->m_pNextVB;				\
	}										\
}

#ifdef PRODUCT_ENCLAVE
#define MACRO_COPYCHAINGEOMETRY_NOTEXCOORD(_pSrc, _pDst)	\
{											\
	CXR_VertexBuffer* pCopyDst = _pDst;		\
	const CXR_VertexBuffer* pCopySrc = _pSrc;		\
	while(pCopyDst && pCopySrc)			\
	{										\
		pCopyDst->m_Flags = pCopySrc->m_Flags;			\
		pCopyDst->m_VBID = pCopySrc->m_VBID;			\
		pCopyDst->m_pV = pCopySrc->m_pV;				\
		pCopyDst->m_pN = pCopySrc->m_pN;				\
		pCopyDst->m_nV = pCopySrc->m_nV;				\
		pCopyDst->m_piPrim = pCopySrc->m_piPrim;		\
		pCopyDst->m_nPrim = pCopySrc->m_nPrim;		\
		pCopyDst->m_PrimType = pCopySrc->m_PrimType;	\
		pCopyDst->m_pMatrixPalette = pCopySrc->m_pMatrixPalette;\
		pCopyDst->m_nMatrixPalette = pCopySrc->m_nMatrixPalette;\
		pCopySrc = pCopySrc->m_pNextVB;				\
		pCopyDst = pCopyDst->m_pNextVB;				\
	}										\
}
#else
#define MACRO_COPYCHAINGEOMETRY_NOTEXCOORD(_pSrc, _pDst)	\
{											\
	CXR_VertexBuffer* pCopyDst = _pDst;		\
	const CXR_VertexBuffer* pCopySrc = _pSrc;		\
	while(pCopyDst && pCopySrc)			\
	{										\
		pCopyDst->m_Flags = pCopySrc->m_Flags;			\
		pCopyDst->m_VBID = pCopySrc->m_VBID;			\
		pCopyDst->m_pV = pCopySrc->m_pV;				\
		pCopyDst->m_pN = pCopySrc->m_pN;				\
		pCopyDst->m_pFog = pCopySrc->m_pFog;				\
		pCopyDst->m_nV = pCopySrc->m_nV;				\
		pCopyDst->m_piPrim = pCopySrc->m_piPrim;		\
		pCopyDst->m_nPrim = pCopySrc->m_nPrim;		\
		pCopyDst->m_PrimType = pCopySrc->m_PrimType;	\
		pCopyDst->m_pMatrixPalette = pCopySrc->m_pMatrixPalette;\
		pCopyDst->m_nMatrixPalette = pCopySrc->m_nMatrixPalette;\
		pCopySrc = pCopySrc->m_pNextVB;				\
		pCopyDst = pCopyDst->m_pNextVB;				\
	}										\
}
#endif

#define MACRO_COPYCHAINGEOMETRY_TEXCOORD(_pSrc, _pDst, iSrc, iDst)	\
{													\
	CXR_VertexBuffer* pCopyDst = _pDst;				\
	const CXR_VertexBuffer* pCopySrc = _pSrc;		\
	while(pCopyDst && pCopySrc)						\
	{												\
		pCopyDst->m_pTV[iDst] = pCopySrc->m_pTV[iSrc];	\
		pCopyDst->m_nTVComp[iDst] = pCopySrc->m_nTVComp[iSrc];	\
		pCopySrc = pCopySrc->m_pNextVB;				\
		pCopyDst = pCopyDst->m_pNextVB;				\
	}												\
}

//	int nTex = Max((pCopyDst) ? pCopyDst->m_nTex : 0, iDst+1);	\
//		pCopyDst->m_nTex = nTex;					


#define MACRO_COPYCHAINGEOMETRY_COLOR(_pSrc, _pDst, SrcCol, DstCol, Col)	\
{													\
	CXR_VertexBuffer* pCopyDst = _pDst;				\
	const CXR_VertexBuffer* pCopySrc = _pSrc;		\
	while(pCopyDst && pCopySrc)						\
	{												\
		CPixel32* pCol = pCopySrc->m_p##SrcCol;		\
		if (Col != 0xffffffff && pCol)				\
		{											\
			CPixel32* pC = _pVBM->Alloc_CPixel32(pCopyDst->m_nV);	\
			M_ASSERT( pC, "" );						\
			PPA_Mul_RGBA32(Col, pCol, pC, pCopyDst->m_nV);	\
			pCopyDst->m_p##DstCol = pC;				\
		}											\
		else										\
		{											\
			pCopyDst->m_p##DstCol = pCol;			\
			pCopyDst->m_Color = pCopySrc->m_Color;	\
			if (Col != 0xffffffff) pCopyDst->m_Color *= CPixel32(Col);\
		}											\
													\
		pCopySrc = pCopySrc->m_pNextVB;				\
		pCopyDst = pCopyDst->m_pNextVB;				\
	}												\
}

#define MACRO_SETCHAINGEOMETRY_COLOR(_pDst, Col)	\
{													\
	CXR_VertexBuffer* pCopyDst = _pDst;				\
	while(pCopyDst)						\
	{												\
		pCopyDst->m_Color = Col;					\
		pCopyDst = pCopyDst->m_pNextVB;				\
	}												\
}

#define MACRO_SETDEFAULTATTRIBUTES(pA, pSurf, pSurfKey, pLayer)	\
{														\
	if (pSurf->m_Flags & XW_SURFFLAGS_POLYGONOFFSET)	\
	{													\
		pA->Attrib_PolygonOffset(pSurfKey->m_PolygonOffsetScale, pSurfKey->m_PolygonOffsetUnits);	\
		pA->Attrib_Enable(CRC_FLAGS_POLYGONOFFSET);	\
	}													\
	if (pSurf->m_Flags & XW_SURFFLAGS_NOCULL)			\
		pA->Attrib_Disable(CRC_FLAGS_CULL);				\
	else												\
		pA->Attrib_Enable(CRC_FLAGS_CULL);				\
}

// #define XRUTIL_LAYER_POLY_OFFSET

// CXR_VertexBuffer* SubdivisionSurface_TesselateVB(CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat);


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Renders a vertex buffer with a surface.
						
	Parameters:			
		_Flags:					RENDERSURFACE_xxxxxxxx enum.
		_pSurface:				Cannot be NULL
		_pSurfKey:				Evaluated surface-keyframe, Cannot be NULL
		_pEngine:				May be NULL, but surface will not be rendered correctly.
		_pVBM:					Cannot be NULL
		_pModel2World:			WMat, may be NULL, specify RENDERSURFACE_MATRIXSTATIC_M2W in flags if it can be used in VB.
		_pWorld2View:			VMat, may be NULL, specify RENDERSURFACE_MATRIXSTATIC_W2V in flags if it can be used in VB.
		_pModel2View:			WMat*VMat, may be NULL, specify RENDERSURFACE_MATRIXSTATIC_M2V in flags if it can be used in VB.
		_pVB:					Geometry source. pV, pN, nV, piPrim, nPrim, PrimType, pTV[0] and pTV[1] are used.
		_BasePriority:			Base priority
		_PriorityOffset:					Priority offset between rendering passes.
		_pParams->m_TextureIDLightMap:		Lightmap texture, zero if lightmapping is not used. _pVB->m_pTV[1] should be pointing at the lightmap UV coordinates.
		_pParams->m_TextureIDReflectionMap:	Refraction map texture
						
	Returns:			true if success
						
\*____________________________________________________________________*/

#define RENDERSURFACE_SHOWVBTYPES 4096

#ifndef DEFINE_MAT43_IS_MAT4D
bool CXR_Util::Render_Surface(
	int _Flags, CXW_Surface* _pSurface, CXW_SurfaceKeyFrame* _pSurfKey, CXR_Engine* _pEngine, CXR_VBManager *_pVBM, 
	const CMat43fp32* _pModel2World, const CMat43fp32* _pWorld2View, const CMat4Dfp32* _pModel2View, 
	CXR_VertexBuffer* _pVB, fp32 _BasePriority, fp32 _PriorityOffset, CXR_RenderSurfExtParam* _pParams)
{
	CMat43fp32 M2V;
	if (_pModel2View)
	{
		M2V.CreateFrom4x4(*_pModel2View);
		_Flags &= ~RENDERSURFACE_MATRIXSTATIC_M2V;
	}
	return Render_Surface(_Flags, _pSurface, _pSurfKey, _pEngine, _pVBM, 
	                      _pModel2World, _pWorld2View, (_pModel2View ? &M2V : NULL), 
	                      _pVB, _BasePriority, _PriorityOffset, _pParams);
}
#endif


bool CXR_Util::Render_Surface(
	int _Flags, CXW_Surface* _pSurface, CXW_SurfaceKeyFrame* _pSurfKey, CXR_Engine* _pEngine, CXR_VBManager *_pVBM, 
	const CMat43fp32* _pModel2World, const CMat43fp32* _pWorld2View, const CMat43fp32* _pModel2View, 
	CXR_VertexBuffer* _pVB, fp32 _BasePriority, fp32 _PriorityOffset, CXR_RenderSurfExtParam* _pParams)
{
	MAUTOSTRIP(CXR_Util_Render_Surface, false);
	MSCOPE(Render_Surface, XR);

#ifndef M_RTM
	_pVBM->m_Stats_nRenderSurface++;
#endif

	// -------------------------------------------------------------------
	//  Resolve transformation
	// -------------------------------------------------------------------
	const CMat4Dfp32* pVBMatrix = NULL;

	if (!_pModel2View && !(_Flags & RENDERSURFACE_MATRIXSTATIC_M2V))
	{
		if (_pModel2World && _pWorld2View)
		{
			CMat43fp32* pMat = _pVBM->Alloc_M43();
			if (!pMat) return false;
			_pModel2World->Multiply(*_pWorld2View, *pMat);
			_pModel2View = pMat;
			_Flags |= RENDERSURFACE_MATRIXSTATIC_M2V;
		}
		else
		{
			if (_pModel2World)
			{
				if (_Flags & RENDERSURFACE_MATRIXSTATIC_M2W)
					_pModel2View = _pModel2World;
				else
				{
					_pModel2View = _pVBM->Alloc_M43(*_pModel2World);
					if (!_pModel2View) return false;
				}
				_Flags |= RENDERSURFACE_MATRIXSTATIC_M2V;
			}
			else if (_pWorld2View)
			{
				if (_Flags & RENDERSURFACE_MATRIXSTATIC_W2V)
					_pModel2View = _pWorld2View;
				else
				{
					_pModel2View = _pVBM->Alloc_M43(*_pWorld2View);
					if (!_pModel2View) return false;
				}
				_Flags |= RENDERSURFACE_MATRIXSTATIC_M2V;
			}
			else
				_pModel2View = NULL;
		}
	}

#ifdef DEFINE_MAT43_IS_MAT4D
	if (_pModel2View && (_Flags & RENDERSURFACE_MATRIXSTATIC_M2V))
		pVBMatrix = _pModel2View;
	else
#endif
	{
		if (_pModel2View)
		{
			pVBMatrix = _pVBM->Alloc_M4(*_pModel2View);
			if (!pVBMatrix) return false;
		}
	}


	// -------------------------------------------------------------------
	//  Get some info
	// -------------------------------------------------------------------

	int nLayers = _pSurfKey->m_lTextures.Len();
	CXW_SurfaceLayer* pLayers = _pSurfKey->m_lTextures.GetBasePtr();

	int ChainLen = _pVB->GetChainLen();

	fp32 BasePriority = _BasePriority;

	int bDepthFog = !(_pSurface->m_Flags & XW_SURFFLAGS_NOFOG) && _pEngine && _pEngine->GetFogState()->DepthFogEnable(); /*&& (_Flags & RENDERSURFACE_DEPTHFOG)*/;
	fp32 DepthFogScale = (_pParams) ? _pParams->m_DepthFogScale : 1.0f;


	int nMultiTexture = (_pEngine) ? _pEngine->m_pRender->Attrib_GlobalGetVar(CRC_GLOBALVAR_NUMTEXTURES) : 1;
#ifndef M_RTM
	int ShowVBTypes = (_pEngine) ? _pEngine->m_ShowVBTypes : 0;
#endif

	int LightMapTextureID = (_pParams) ? _pParams->m_TextureIDLightMap : 0;


	// -------------------------------------------------------------------
	//  Render layers
	// -------------------------------------------------------------------
	int nFinalGroups = 0;
	CXR_VertexBuffer* lpFinalGroupVBChains[64];		// Surely, this must be sufficient.

	fp32 Priority = 0;

	CXR_VertexBuffer* pVB = NULL;
	int nChannels = 1;

/*
	fp32 PolygonOffsetScale = 0.0f;
	fp32 PolygonOffset = -100.0f;
	fp32 PolygonOffsetDelta = -100.0f;
	int bPolygonOffset = _pSurface->m_Flags & XW_SURFFLAGS_POLYGONOFFSET;
*/

	for(int iTxt = 0; iTxt < nLayers; iTxt++)
	{
		CXW_SurfaceLayer* pLayer = &pLayers[iTxt];

		// Invisible?
		if (pLayer->m_Flags & XW_LAYERFLAGS_INVISIBLE)
		{
			if (pLayer->m_Flags & XW_LAYERFLAGS_GROUP)
			{
				// If grouped, skip all layers in the group.
				iTxt++;
				while((iTxt < nLayers) && 
					(pLayers[iTxt].m_Flags & XW_LAYERFLAGS_GROUP))
					iTxt++;
				continue;
			}
			else
				continue;
		}

		// Detail to be discarded?
		if ((_Flags & RENDERSURFACE_NODETAIL) && 
			(pLayer->m_Flags & XW_LAYERFLAGS_DETAIL)) continue;

		if (!pVB)
		{
			pVB = _pVBM->Alloc_VBChain(CXR_VB_ATTRIB, ChainLen);
			M_ASSERT( pVB, "" );

			if (_pEngine) _pEngine->SetDefaultAttrib(pVB->m_pAttrib);

			pVB->Matrix_Set( pVBMatrix );
//			MACRO_COPYCHAINGEOMETRY_NOTEXCOORD(_pVB, pVB);
			MACRO_COPYCHAINGEOMETRY(_pVB, pVB);
		}

		CRC_Attributes* pA = pVB->m_pAttrib;

		int bGrouped = pLayer->m_Flags & XW_LAYERFLAGS_GROUP;
		int iChannel = pLayer->m_TexChannel;
		int iTexCoordSrc = pLayer->m_TexCoordNr;
		nChannels = Max(nChannels, iChannel+1);

//		MACRO_COPYCHAINGEOMETRY_TEXCOORD(_pVB, pVB, iTexCoordSrc, iChannel);
		pA->Attrib_TexCoordSet(iChannel, iTexCoordSrc);



		// -------------------------------------------------------------------
		//  Set attributes, 
		//  unless this layer is grouped with another layer
		// -------------------------------------------------------------------
		if (!bGrouped)
		{
			pVB->m_Priority = BasePriority + Priority;
			Priority += _PriorityOffset;
/*			if (bPolygonOffset)
			{
				pA->Attrib_PolygonOffset(PolygonOffsetScale, PolygonOffset);
				PolygonOffset += PolygonOffsetDelta;
			}
*/
			MACRO_SETDEFAULTATTRIBUTES(pA, _pSurface, _pSurfKey, pLayer);

			// Color-Write
			if (pLayer->m_Flags & XW_LAYERFLAGS_NOCOLORWRITE)
				pA->Attrib_Disable(CRC_FLAGS_COLORWRITE);
			else
				pA->Attrib_Enable(CRC_FLAGS_COLORWRITE);

			// Alpha-Write
			if (pLayer->m_Flags & XW_LAYERFLAGS_NOALPHAWRITE)
				pA->Attrib_Disable(CRC_FLAGS_ALPHAWRITE);
			else
				pA->Attrib_Enable(CRC_FLAGS_ALPHAWRITE);

			// Z-Write/Compare
			if (pLayer->m_Flags & XW_LAYERFLAGS_NOZWRITE)
				pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
			else
				pA->Attrib_Enable(CRC_FLAGS_ZWRITE);
			pA->Attrib_ZCompare(pLayer->m_ZFunc);

			// Alpha-compare?
			if (pLayer->m_Flags & XW_LAYERFLAGS_ALPHACOMPARE)
				pA->Attrib_AlphaCompare(pLayer->m_AlphaFunc, pLayer->m_AlphaRef);
			else
				pA->Attrib_AlphaCompare(CRC_COMPARE_ALWAYS, 0);

			pA->Attrib_RasterMode(pLayer->m_RasterMode);
			
			
			// DepthFog
			if (bDepthFog && !(pLayer->m_Flags & XW_LAYERFLAGS_NOFOG))
			{
//				_pEngine->GetFogState()->SetDepthFog(pA, -1, DepthFogScale);	// Pass -1 means force depthfog to fogcolor			

				if (pLayer->m_Flags & XW_LAYERFLAGS_FOGBLACK)
				{
					_pEngine->GetFogState()->SetDepthFogBlack(pA, DepthFogScale);
				}
				else if (pLayer->m_Flags & XW_LAYERFLAGS_FOGCOLOR)
				{
					_pEngine->GetFogState()->SetDepthFog(pA, -1, DepthFogScale);	// Pass -1 means force depthfog to fogcolor
				}
				else
					_pEngine->GetFogState()->SetDepthFog(pA, iTxt, DepthFogScale);
			}
		}

		// -------------------------------------------------------------------
		//  Texture-channel specific states
		// -------------------------------------------------------------------

		// Get TextureID, translate any special texture-identifiers to supplied real texture-IDs.
		int TextureID = pLayer->m_TextureID;
		if (TextureID >= XW_SURFTEX_SPECIALBASE)
		{
			switch(TextureID)
			{
			case XW_SURFTEX_REFLECTIONMAP :
				TextureID = (_pParams) ? _pParams->m_TextureIDReflectionMap : 0;
				break;

			case XW_SURFTEX_REFRACTIONMAP :
				TextureID = (_pParams) ? _pParams->m_TextureIDRefractionMap : 0;
				break;

			case XW_SURFTEX_LIGHTMAP :
				TextureID = LightMapTextureID;
				break;

			case XW_SURFTEX_TEXTUREMAP0 :
			case XW_SURFTEX_TEXTUREMAP1 :
			case XW_SURFTEX_TEXTUREMAP2 :
			case XW_SURFTEX_TEXTUREMAP3 :
				TextureID = (_pParams) ? _pParams->m_TextureID[TextureID - XW_SURFTEX_TEXTUREMAP0] : 0;
				break;

			default :
				TextureID = 0;
			}
		}

#ifdef _DEBUG
		if (_pEngine && !_pEngine->GetTC()->IsValidID(TextureID))
			M_ASSERT(0, "?");
#endif

		pA->Attrib_TextureID(iChannel, TextureID);
		pA->Attrib_TexEnvMode(iChannel, pLayer->m_TexEnvMode);

		// -------------------------------------------------------------------
		//  Execute layer operations
		// -------------------------------------------------------------------
		if (pLayer->m_lOper.Len())
		{
			// -------------------------------------------------------------------
			//  Initialize VB-operator context
			CXR_VBOperatorContext Context;

			Context.m_pEngine = _pEngine;
			Context.m_pVBM = _pVBM;
			Context.m_AnimTime = _pSurfKey->m_AnimTime;
			Context.m_AnimTimeWrapped = _pSurfKey->m_AnimTimeWrapped;
			Context.m_VBChainIndex;
			Context.m_iTexChannel = iChannel;
			Context.m_iTexChannelNext = iChannel;
			Context.m_VBArrayReadOnlyMask = -1;
			Context.m_pModel2View = _pModel2View;
			Context.m_pModel2World = _pModel2World;
			Context.m_pWorld2View = _pWorld2View;
			Context.m_pVBHead = pVB;
			Context.m_pVBHeadSrc = _pVB;

			int iFree = 0;
			Context.m_iFreeTexCoordSet = 0;
			while(iFree < CRC_MAXTEXTURES && _pVB->m_pTV[iFree] != NULL) iFree++;
			if (iFree == CRC_MAXTEXTURES) iFree = -1;
			Context.m_iFreeTexCoordSet = iFree;

			// -------------------------------------------------------------------
			CXR_SurfaceContext* pSC;
			if(!_pEngine)
			{
				MACRO_GetRegisterObject(CXR_SurfaceContext, pTempSC, "SYSTEM.SURFACECONTEXT");
				pSC = pTempSC;
				if(!pSC)
					Error_static("-", "No surface-context available.");
			}
			else
				pSC = _pEngine->GetSC();

			// -------------------------------------------------------------------
			//ConOut(CStrF("nOperations %d", pLayer->m_lOper.Len()));
			for(int iOper = 0; iOper < pLayer->m_lOper.Len(); iOper++)
			{
				const CXW_LayerOperation& Oper = pLayer->m_lOper[iOper];
				//ConOut(CStrF("    OpCode %d, Components %.8x, iClass %d", Oper.m_OpCode, Oper.m_Components, Oper.m_iOperatorClass));
				switch(Oper.m_OpCode)
				{
				case XW_LAYEROPCODE_OPERATOR :
					{
						CXR_VBOperator* pVBOperator = pSC->VBOperator_Get(Oper.m_iOperatorClass);
						//ConOut(CStrF("    pVBOperator %.8x", pVBOperator));
						if (pVBOperator)
						{
							CXR_VertexBuffer* pTargetVB = pVB;
							CXR_VertexBuffer* pSrcVB = _pVB;
							Context.m_VBChainIndex = 0;

							while(pSrcVB && pTargetVB)
							{
								if (!pVBOperator->OnOperate(Context, Oper, pSrcVB, pTargetVB))
									return false;

								pSrcVB = pSrcVB->m_pNextVB;
								pTargetVB = pTargetVB->m_pNextVB;
								Context.m_VBChainIndex++;
							}

							if (!pVBOperator->OnOperateFinish(Context, Oper, _pVB, pVB))
								return false;
						}
					}
					break;

				default:;
				}
			}

			iChannel = Context.m_iTexChannelNext;
			nChannels = Max(nChannels, iChannel+1);
		}

		if (bGrouped) continue;

//		if (nChannels > 1) ConOut(CStrF("Grouped surface: %d, %d, %.8x, %.8x", pA->m_TextureID[0], pA->m_TextureID[1], pVB->m_pTV[0], pVB->m_pTV[1]));


		// -------------------------------------------------------------------
		// Alpha blend surface
		if (_Flags & RENDERSURFACE_ALPHABLEND)
		{
			CPixel32 FadeColor = (_pParams) ? _pParams->m_Color : CPixel32(0xffffffff);
			CPixel32 Color = pLayer->m_Color;
//			pA->Attrib_Disable(CRC_FLAGS_ZWRITE);

			// Translate rasterization mode
			switch(pA->m_RasterMode)
			{
			case CRC_RASTERMODE_NONE : 
				{
					Color *= FadeColor;
					pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
				}
				break;

			case CRC_RASTERMODE_ALPHABLEND : 
				{
					Color *= FadeColor;
				}
				break;

			case CRC_RASTERMODE_ALPHAADD : 
				{
					Color *= FadeColor;
				}
				break;

			case CRC_RASTERMODE_MULTIPLY : 
				{
					CPixel32 Fade(FadeColor.GetA(), FadeColor.GetA(), FadeColor.GetA(), FadeColor.GetA());
//					CPixel32 Fade(255,255,255, FadeColor.GetA());
					Color *= Fade;
					pA->Attrib_RasterMode(CRC_RASTERMODE_NONE);
					pA->Attrib_Enable(CRC_FLAGS_BLEND);
					pA->Attrib_SourceBlend(CRC_BLEND_DESTCOLOR);
					pA->Attrib_DestBlend(CRC_BLEND_INVSRCALPHA);
				}
				break;

			case CRC_RASTERMODE_ONE_INVALPHA :
				{
					CPixel32 Fade(FadeColor.GetA(), FadeColor.GetA(), FadeColor.GetA(), FadeColor.GetA());
					Color *= Fade;
				};
				break;

			default :
				ConOutD(CStrF("Unsupported fade %d", pA->m_RasterMode));
				break;
			}

			if (pA->m_AlphaCompare != CRC_COMPARE_ALWAYS)
			{
				int Old = pA->m_AlphaRef;
				pA->m_AlphaRef = int(pA->m_AlphaRef) * FadeColor.GetA() >> 8;
//			ConOut(CStrF("AlphaCompare %d -> %d, %d", Old, pA->m_AlphaRef, pA->m_AlphaCompare));
			}

//			ConOut(CStrF("Color %.8x, RM %d", Color, pA->m_RasterMode));

			// Vertex lighting

			if (pLayer->m_Flags & XW_LAYERFLAGS_LIGHTING)
			{
				if (_pParams && _pParams->m_pLights)
				{
					pA->m_pLights = _pParams->m_pLights;
					pA->m_nLights = _pParams->m_nLights;
					pA->Attrib_Enable(CRC_FLAGS_LIGHTING);
					pVB->m_Color = Color;
				}
				else
				{
					if (!pVB->m_VBID)
					{
#ifndef PRODUCT_ENCLAVE
						if (pLayer->m_Flags & XW_LAYERFLAGS_SPECULAR)
							MACRO_COPYCHAINGEOMETRY_COLOR(_pVB, pVB, Spec, Col, Color)
						else
#endif
							MACRO_COPYCHAINGEOMETRY_COLOR(_pVB, pVB, Col, Col, Color)
					}
					else
					{
						ConOut("§cf80WARNING: (CXR_Util::Render_Surface) Vertex lighting on static VB, but no lights.");
					}
				}
			}
			else
				pVB->m_Color = Color;

			pVB->m_pAttrib->m_nChannels = nChannels;
			_pVBM->AddVB(pVB);
			lpFinalGroupVBChains[nFinalGroups++] = pVB;
		}

		// -------------------------------------------------------------------
		// Apply lighting
		else if (!(_Flags & RENDERSURFACE_FULLBRIGHT) &&
			 (pLayer->m_Flags & XW_LAYERFLAGS_LIGHTING))
		{
			// -------------------------------------------------------------------
			//  Standard light
			// -------------------------------------------------------------------
			if (LightMapTextureID || 
				((nChannels > 1) && !(pLayer->m_Flags & XW_LAYERFLAGS_CUSTOMTEXENV)))
			{
				// Lightmap or vertex lighting on multitexture surface

				if (LightMapTextureID && 
					nMultiTexture > nChannels &&
					!(pLayer->m_Flags & XW_LAYERFLAGS_CUSTOMTEXENV))
				{
					// nChannels+1 multitexture is supported  (Lightmap only)

					// draw surfaces with lightmaps slightly after vertexlight surfaces!
					pVB->m_Priority += 0.0001f;

					pA->Attrib_TextureID(nChannels, LightMapTextureID);
					pA->Attrib_TexCoordSet(nChannels, 1);
//					MACRO_COPYCHAINGEOMETRY_TEXCOORD(_pVB, pVB, 1, nChannels);

#ifndef	PRODUCT_ENCLAVE
					if (pLayer->m_Flags & XW_LAYERFLAGS_SPECULAR)
						MACRO_COPYCHAINGEOMETRY_COLOR(_pVB, pVB, Spec, Col, pLayer->m_Color)
					else
#endif
						MACRO_COPYCHAINGEOMETRY_COLOR(_pVB, pVB, Col, Col, pLayer->m_Color)

					pVB->m_pAttrib->m_nChannels = nChannels + 1;
					_pVBM->AddVB(pVB);
					lpFinalGroupVBChains[nFinalGroups++] = pVB;
				}
				else
				{
					// Multipass
					pVB->m_pAttrib->m_nChannels = nChannels;
					_pVBM->AddVB(pVB);
					lpFinalGroupVBChains[nFinalGroups++] = pVB;

					CXR_VertexBuffer* pVB2 = _pVBM->Alloc_VBChain(CXR_VB_ATTRIB, ChainLen);
					M_ASSERT( pVB2, "" );

					pVB2->Matrix_Set(pVBMatrix);
					MACRO_COPYCHAINGEOMETRY(_pVB, pVB2);

					CRC_Attributes* pA = pVB2->m_pAttrib;
					if (_pEngine) _pEngine->SetDefaultAttrib(pA);
					
					pVB2->m_Priority = pVB->m_Priority + CXR_VBPRIORITY_LIGHTMAP;
					
					MACRO_SETDEFAULTATTRIBUTES(pA, _pSurface, _pSurfKey, pLayer);


/*					if (bPolygonOffset && pA->m_AlphaCompare == CRC_COMPARE_ALWAYS)
					{
						pA->Attrib_PolygonOffset(PolygonOffsetScale, PolygonOffset);
						PolygonOffset += PolygonOffsetDelta;
						pA->Attrib_ZCompare(CRC_COMPARE_LESSEQUAL);
					}
					else
						pA->Attrib_ZCompare(CRC_COMPARE_LESSEQUAL);
*/

					if (LightMapTextureID)
					{
						pA->Attrib_TextureID(0, LightMapTextureID);
						pA->Attrib_TexCoordSet(0, 1);
					}
					else if (_pParams && _pParams->m_pLights)
					{
						if (pLayer->m_Color != 0xffffffff)
						{
							if (!SetColorizedLights(pA, _pVBM, pLayer->m_Color, _pParams->m_pLights, _pParams->m_nLights))
								return false;
						}
						else
						{
							pA->m_pLights = _pParams->m_pLights;
							pA->m_nLights = _pParams->m_nLights;
						}
						pA->Attrib_Enable(CRC_FLAGS_LIGHTING);
					}
					else
					{
						MACRO_COPYCHAINGEOMETRY_COLOR(_pVB, pVB2, Col, Col, pLayer->m_Color);
					}

					pA->m_nChannels = 1;
					pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
					pA->Attrib_ZCompare(CRC_COMPARE_LESSEQUAL);
					pA->Attrib_RasterMode(CRC_RASTERMODE_NONE);
					pA->Attrib_TexEnvMode( 0, CRC_PS2_TEXENVMODE_REPLACE | CRC_PS2_TEXENVMODE_MODULATE | CRC_PS2_TEXENVMODE_COLORBUFFER | CRC_PS2_TEXENVMODE_RGBMASK | CRC_PS2_TEXENVMODE );

					_pVBM->AddVB(pVB2);
				}
			}
			else
			{
				// Vertex lighting
				if (_pParams && _pParams->m_pLights)
				{
					if (pLayer->m_Color != 0xffffffff)
					{
						if (!SetColorizedLights(pA, _pVBM, pLayer->m_Color, _pParams->m_pLights, _pParams->m_nLights))
							return false;
					}
					else
					{
						pA->m_pLights = _pParams->m_pLights;
						pA->m_nLights = _pParams->m_nLights;
					}
					pA->Attrib_Enable(CRC_FLAGS_LIGHTING);
				}
				else
				{
					if (!pVB->m_VBID)
					{
#ifndef PRODUCT_ENCLAVE
						if (pLayer->m_Flags & XW_LAYERFLAGS_SPECULAR)
							MACRO_COPYCHAINGEOMETRY_COLOR(_pVB, pVB, Spec, Col, pLayer->m_Color)
						else
#endif
							MACRO_COPYCHAINGEOMETRY_COLOR(_pVB, pVB, Col, Col, pLayer->m_Color)
					}
					else
					{
						ConOut("§cf80WARNING: (CXR_Util::Render_Surface) Vertex lighting on static VB, but no lights.");
					}
				}

				pVB->m_pAttrib->m_nChannels = nChannels;
				_pVBM->AddVB(pVB);
				lpFinalGroupVBChains[nFinalGroups++] = pVB;
			}
		}
		else
		{
			MACRO_SETCHAINGEOMETRY_COLOR(pVB, pLayer->m_Color);

			pVB->m_pAttrib->m_nChannels = nChannels;
			_pVBM->AddVB(pVB);
			lpFinalGroupVBChains[nFinalGroups++] = pVB;
		}

		pVB = NULL;
		nChannels = 1;
	}








#ifndef M_RTM
	if (ShowVBTypes)
	{
		bool bIsHardwareVB = true;
		CXR_VertexBuffer* pVB = _pVB;
		while(pVB)
		{
			if (!pVB->m_VBID)
			{
				bIsHardwareVB = false;
				break;
			}
			pVB = pVB->m_pNextVB;
		}

		if ( ((ShowVBTypes == 2) && !bIsHardwareVB))
		{
			CXR_VertexBuffer* pShowVB = _pVBM->Alloc_VBChain(0, ChainLen);
 			if (!pShowVB) return false;
			MACRO_COPYCHAINGEOMETRY_VERTEXONLY(_pVB, pShowVB);

			pShowVB->m_pAttrib = _pVBM->Alloc_Attrib();
			if (!pShowVB->m_pAttrib) return false;
			pShowVB->m_pAttrib->SetDefault();
			pShowVB->m_pAttrib->Attrib_TextureID(0, _pEngine->GetTC()->GetTextureID("SPECIAL_00ff00"));
			pShowVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ADD);
			pShowVB->m_pAttrib->m_nChannels = 1;
			pShowVB->m_Priority = CXR_VBPRIORITY_FLARE-1;
			pShowVB->Matrix_Set(pVBMatrix);
			_pVBM->AddVB(pShowVB);
		}

		if (ShowVBTypes & (4+8))
		{
			const fp32 NormalLen = 8.0f;

			_pEngine->m_pRender->Matrix_Push();
			if (_pModel2View)
				_pEngine->m_pRender->Matrix_Set(*_pModel2View);
			else
				_pEngine->m_pRender->Matrix_SetUnit();

			CXR_VertexBuffer* pVB = _pVB;
			while(pVB)
			{
				if (pVB->m_VBID)
					break;

				if (!pVB->BuildVertexUsage(_pVBM))
					return false;

				const uint16* piV = pVB->m_piVertUse;
				int nVA = pVB->m_nV;
				int nV = (piV) ? pVB->m_nVertUse : pVB->m_nV;

				const CVec3Dfp32* pV = pVB->m_pV;
				const CVec3Dfp32* pN = pVB->m_pN;
				const CVec3Dfp32* pTangU = (pVB->m_nTVComp[2] == 3) ? (CVec3Dfp32*)pVB->m_pTV[2] : NULL;
				const CVec3Dfp32* pTangV = (pVB->m_nTVComp[3] == 3) ? (CVec3Dfp32*)pVB->m_pTV[3] : NULL;
				for(int v = 0; v < nV; v++)
				{
					int iv = (piV) ? piV[v] : v;

					if (pN)
					{
						CVec3Dfp32 V(pV[iv]);
						CVec3Dfp32 N(pN[iv]);
						_pEngine->m_pRender->Render_Wire(V, V + N*NormalLen, 0xff0000ff);
					}

					if (pTangU && pTangV)
					{
						CVec3Dfp32 V(pV[iv]);
						CVec3Dfp32 TangU(pTangU[iv]);
						CVec3Dfp32 TangV(pTangV[iv]);
						_pEngine->m_pRender->Render_Wire(V, V + TangU*NormalLen, 0xffff0000);
						_pEngine->m_pRender->Render_Wire(V, V + TangV*NormalLen, 0xff00ff00);
					}
				}

				pVB = pVB->m_pNextVB;
			}

			_pEngine->m_pRender->Matrix_Pop();
		}
	}
#endif

	return true;
}

