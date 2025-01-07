
#include "PCH.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Surface renderer and associated functions
					
	Author:			Magnus Högdahl
					
	Copyright:		Starbreeze Studios AB, 1998-2001
					
	Comments:		







	IF YOU WRITE IN THIS FILE WITHOUT MY PRIOR APPROVAL I WILL DEFINITELY AND TOTALLY KILL YOU!

		SERIOUSLY, YOU WILL HAVE A GOOD CHANCE AT WRECKING TONS OF STUFF WITHOUT KNOWING IT.






					
\*____________________________________________________________________________________________*/

#ifndef	PLATFORM_PS2
// Enable this define to get singlepass lightmaps or multipass vertexlights code
#define REQUIRE_SUPPORT_SINGLEPASSLIGHTMAP_OR_MULTIPASS_VERTEXLIGHT
#endif	// PLATFORM_PS2

#include "XRUtil.h"
#include "XRVertexBuffer.h"
#include "XRVBManager.h"
#include "XRSurfaceContext.h"
#include "XRVBContext.h"
#include "XRShader.h"

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

#if 0
static bool SetColorizedLights(CRC_Attributes* _pA, CXR_VBManager* _pVBM, CPixel32 _Color, const CRC_Light* _pLights, int _nLights)
{
	MAUTOSTRIP(SetColorizedLights, false);
	CRC_Light* pL = (CRC_Light*) _pVBM->Alloc(sizeof(CRC_Light) * _nLights);
	if (!pL) return false;

	for(int i = 0; i < _nLights; i++)
	{
		pL[i] = _pLights[i];
		pL[i].m_Color *= _Color;
	}

	_pA->Attrib_Lights(pL, _nLights);

	return true;
}
#endif

bool CXR_Util::ApplyLightingRenderState(CXR_Engine* _pEngine, CXR_VBManager *_pVBM, CRC_Attributes* _pA, 
										const CXR_Light* _pLights, int _nLights, const CVec4Dfp32& _IntensityScale,
										bool _bNoNormal,const CVec4Dfp32 * _pLightFieldAxes,const CMat4Dfp32 * _pModel2World)
{
	if (!_pEngine)
		return false;

	int nTexGenL = 3;
	int nL = Min(_nLights, nTexGenL);
	int nProj = 0;

	int TexGen = (_bNoNormal) ? CRC_TEXGENMODE_LIGHTING_NONORMAL : CRC_TEXGENMODE_LIGHTING;

	// Count proj map
	{
		for(int i = 0; i < nL; i++)
		{
			if (_pLights[i].m_ProjMapID || _pLights[i].m_Type == CXR_LIGHTTYPE_SPOT)
				nProj++;
		}
	}

	CVec4Dfp32* pTexGenAttr = _pVBM->Alloc_V4(nL*2 + nProj*3 + (3-nL) + 
		((_pLightFieldAxes) ? ((_bNoNormal) ? 1 : 6) : 0) );
	if (!pTexGenAttr)
		return false;

	CVec4Dfp32* pTGProjMap = pTexGenAttr;
	CVec4Dfp32* pTGLighting = pTexGenAttr + (nProj*3);

	CXR_Shader* pShader = _pEngine->GetShader();
	if (!pShader)
		return false;

	int iL = 0;

	// Add projection map lights
	for(int i = 0; i < nL; i++)
	{
		const CXR_Light& Light = _pLights[i];
		if (!Light.m_ProjMapID && _pLights[i].m_Type != CXR_LIGHTTYPE_SPOT)
			continue;

		// Projmap texgen
		CXR_Shader::CreateProjMapTexGenAttr((fp32*)&pTGProjMap[iL*3+0], Light, Light);
		if (Light.m_ProjMapID)
			_pA->Attrib_TextureID(2+iL, Light.m_ProjMapID);
		else
			_pA->Attrib_TextureID(2+iL, pShader->m_TextureID_DefaultLens);

		// Pos+Color
		CVec3Dfp32 Pos = Light.GetPosition();
		pTGLighting[0] = CVec4Dfp32(Pos[0], Pos[1], Pos[2], Light.m_RangeInv);
		CVec4Dfp32 Color;
		CVec4Dfp32 LightColor = Light.GetIntensityv();
		Color[0] = LightColor[0];
		Color[1] = LightColor[1];
		Color[2] = LightColor[2];
		Color[3] = 1.0f;
		Color.CompMul(_IntensityScale, Color);
		pTGLighting[1] = Color;

		_pA->Attrib_TexGen(2+iL, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V | CRC_TEXGENCOMP_W);
		_pA->Attrib_TexGen(5+iL, TexGen, CRC_TEXGENCOMP_ALL);

		pTGLighting += 2;
		iL++;
	}

	// Add !projection map lights
	for(int j = 0; j < nL; j++)
	{
		const CXR_Light& Light = _pLights[j];
		if (Light.m_ProjMapID || _pLights[j].m_Type == CXR_LIGHTTYPE_SPOT)
			continue;

		// Pos+Color
		CVec3Dfp32 Pos = Light.GetPosition();
		pTGLighting[0] = CVec4Dfp32(Pos[0], Pos[1], Pos[2], Light.m_RangeInv);
		CVec4Dfp32 Color;
		CVec4Dfp32 LightColor = Light.GetIntensityv();
		Color[0] = LightColor[0];
		Color[1] = LightColor[1];
		Color[2] = LightColor[2];
		Color[3] = 1.0f;
		Color.CompMul(_IntensityScale, Color);
		pTGLighting[1] = Color;

		_pA->Attrib_TexGen(5+iL, TexGen, CRC_TEXGENCOMP_ALL);

		pTGLighting += 2;
		iL++;
	}

	// Make dummy texgens for unused lights
	for(; iL < nTexGenL - ((_pLightFieldAxes) ? 1 : 0); iL++)
	{
		// Fill unused dummy lights
		_pA->Attrib_TexGen(5+iL, CRC_TEXGENMODE_CONSTANT, CRC_TEXGENCOMP_ALL);
		pTGLighting[0] = 0;
		pTGLighting++;
	}

	// Lightfield
	if( _pLightFieldAxes && iL < nTexGenL )
	{
		if( _bNoNormal )
		{
			// Just use the mean color
			_pA->Attrib_TexGen(5+iL, CRC_TEXGENMODE_CONSTANT, CRC_TEXGENCOMP_ALL);

			CVec4Dfp32 & LFLight = *pTGLighting;
			LFLight = 0;
			for(int i = 0;i < 6;i++)
			{
				LFLight += _pLightFieldAxes[i];
			}
			LFLight.k[3] = 1.0f;
			pTGLighting++;
		}
		else
		{
			// Do lightfield sampling in Vertex Program
			_pA->Attrib_TexGen(5+iL, CRC_TEXGENMODE_LIGHTFIELD, CRC_TEXGENCOMP_ALL);

			if( _pModel2World )
			{
				// Get the color matrix in local space
				for(int i = 0;i < 3;i++)
				{
					const CVec3Dfp32 &Vec = _pModel2World->GetRow(i);
					pTGLighting[i<<1] = _pLightFieldAxes[0] * Max<fp32>(0,Vec.k[0]) + 
						_pLightFieldAxes[1] * Max<fp32>(0,-Vec.k[0]) + 
						_pLightFieldAxes[2] * Max<fp32>(0,Vec.k[1]) + 
						_pLightFieldAxes[3] * Max<fp32>(0,-Vec.k[1]) +
						_pLightFieldAxes[4] * Max<fp32>(0,Vec.k[2]) +
						_pLightFieldAxes[5] * Max<fp32>(0,-Vec.k[2]);

					pTGLighting[(i<<1)+1] = _pLightFieldAxes[0] * Max<fp32>(0,-Vec.k[0]) + 
						_pLightFieldAxes[1] * Max<fp32>(0,Vec.k[0]) + 
						_pLightFieldAxes[2] * Max<fp32>(0,-Vec.k[1]) + 
						_pLightFieldAxes[3] * Max<fp32>(0,Vec.k[1]) +
						_pLightFieldAxes[4] * Max<fp32>(0,-Vec.k[2]) +
						_pLightFieldAxes[5] * Max<fp32>(0,Vec.k[2]);
				}
			}
			else
			{
				// Just use the existing
				memcpy(pTGLighting,_pLightFieldAxes,sizeof(CVec4Dfp32) * 6);
			}

			pTGLighting += 6;
		}
		iL++;
	}

	_pA->Attrib_TexGenAttr((fp32*)pTexGenAttr);

	_pA->m_pExtAttrib = &ms_lFPLighting[nProj];
	return true;
}

#if 0

#define MACRO_COPYSTATE(_pSrc, _pDst)		\
{											\
	_pDst->m_pAttrib = _pSrc->m_pAttrib;	\
	for(int i = 0; i < CRC_MATRIXSTACKS; i++)					\
		_pDst->m_pTransform[i] = _pSrc->m_pTransform[i];		\
	_pDst->m_pfnPreRender = _pSrc->m_pfnPreRender;				\
	_pDst->m_pPreRenderContext = _pSrc->m_pPreRenderContext;	\
	_pDst->m_Priority = _pSrc->m_Priority;						\
}

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
	_pDst->m_pMatrixPaletteArgs = _pSrc->m_pMatrixPaletteArgs;\
}
//	_pDst->m_pFog = _pSrc->m_pFog;			
//	_pDst->m_nTex = _pSrc->m_nTex;			


#define MACRO_COPYGEOMETRY_VERTEXONLY(_pSrc, _pDst)	\
{											\
	_pDst->m_VBID = _pSrc->m_VBID;			\
	_pDst->m_pV = _pSrc->m_pV;				\
	_pDst->m_nV = _pSrc->m_nV;				\
	_pDst->m_piPrim = _pSrc->m_piPrim;		\
	_pDst->m_nPrim = _pSrc->m_nPrim;		\
	_pDst->m_PrimType = _pSrc->m_PrimType;	\
	_pDst->m_pMatrixPaletteArgs = _pSrc->m_pMatrixPaletteArgs;\
}

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
		pCopyDst->m_pMatrixPaletteArgs = pCopySrc->m_pMatrixPaletteArgs;\
		pCopySrc = pCopySrc->m_pNextVB;					\
		pCopyDst = pCopyDst->m_pNextVB;					\
	}													\
}
//		pCopyDst->m_pFog = pCopySrc->m_pFog;

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
		pCopyDst->m_pMatrixPaletteArgs = pCopySrc->m_pMatrixPaletteArgs;\
		pCopySrc = pCopySrc->m_pNextVB;				\
		pCopyDst = pCopyDst->m_pNextVB;				\
	}										\
}


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
		pCopyDst->m_pMatrixPaletteArgs = pCopySrc->m_pMatrixPaletteArgs;\
		pCopySrc = pCopySrc->m_pNextVB;				\
		pCopyDst = pCopyDst->m_pNextVB;				\
	}										\
}
//		pCopyDst->m_pFog = pCopySrc->m_pFog;			

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

//	int nTex = Max((pCopyDst) ? pCopyDst->m_nTex : 0, iDst+1);	
//		pCopyDst->m_nTex = nTex;					

/* Fix the color multiply before using this
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
			if (!pC) return false;					\
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
*/

#define MACRO_SETCHAINGEOMETRY_COLOR(_pDst, Col)	\
{													\
	CXR_VertexBuffer* pCopyDst = _pDst;				\
	while(pCopyDst)						\
	{												\
		pCopyDst->m_Color = Col;					\
		pCopyDst = pCopyDst->m_pNextVB;				\
	}												\
}
#endif // #if 0


#define MACRO_COPYCHAINGEOMETRY_VERTEXONLY(_pSrc, _pDst)	\
{											\
	CXR_VBChain* pCopyDst = _pDst;		\
	const CXR_VBChain* pCopySrc = _pSrc;		\
	while(pCopyDst && pCopySrc)			\
	{										\
		pCopyDst->Clear(); \
		pCopyDst->m_pV = pCopySrc->m_pV;				\
		pCopyDst->m_nV = pCopySrc->m_nV;				\
		pCopyDst->m_piPrim = pCopySrc->m_piPrim;		\
		pCopyDst->m_nPrim = pCopySrc->m_nPrim;		\
		pCopyDst->m_PrimType = pCopySrc->m_PrimType;	\
		pCopySrc = pCopySrc->m_pNextVB;				\
		pCopyDst = pCopyDst->m_pNextVB;				\
	}										\
}


#ifdef XR_DEFAULTPOLYOFFSETSCALE
#define MACRO_SETDEFAULTATTRIBUTES(pA, pSurf, pSurfKey, pLayer)	\
{														\
	if (pSurf->m_Flags & XW_SURFFLAGS_POLYGONOFFSET)	\
	{													\
		pA->Attrib_PolygonOffset(XR_DEFAULTPOLYOFFSETSCALE + pSurfKey->m_PolygonOffsetScale, XR_DEFAULTPOLYOFFSET + pSurfKey->m_PolygonOffsetUnits);	\
		pA->Attrib_Enable(CRC_FLAGS_POLYGONOFFSET);	\
	}													\
	if (pSurf->m_Flags & XW_SURFFLAGS_NOCULL)			\
		pA->Attrib_Disable(CRC_FLAGS_CULL);				\
	else												\
		pA->Attrib_Enable(CRC_FLAGS_CULL);				\
}
#else
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
#endif



bool CXR_Util::Render_Surface(
	int _Flags, CMTime _Time, CXW_Surface* _pSurface, CXW_SurfaceKeyFrame* _pSurfKey, CXR_Engine* _pEngine, CXR_VBManager *_pVBM, 
	const CMat43fp32* _pModel2World, const CMat43fp32* _pWorld2View, const CMat4Dfp32* _pModel2View, 
	CXR_VertexBufferGeometry* _pVB, fp32 _BasePriority, fp32 _PriorityOffset, CXR_RenderSurfExtParam* _pParams)
{
	CMat4Dfp32 M2V;
	if (_pModel2View)
	{
		M2V = *_pModel2View;
		_Flags &= ~RENDERSURFACE_MATRIXSTATIC_M2V;
	}
	const CMat4Dfp32 World2View=_pWorld2View->Get4x4();
	const CMat4Dfp32 Model2World=_pModel2World->Get4x4();
	return Render_Surface(_Flags, _Time, _pSurface, _pSurfKey, _pEngine, _pVBM, 
	                      &Model2World, &World2View, (_pModel2View ? &M2V : (CMat4Dfp32*)NULL), 
	                      _pVB, _BasePriority, _PriorityOffset, _pParams);
}


// #define XRUTIL_LAYER_POLY_OFFSET

// CXR_VertexBuffer* SubdivisionSurface_TesselateVB(CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat);

static CPixel32 MultiplyCPixel32(CPixel32 _c0, CPixel32 _c1)
{
	CPixel32 Ret;
	Ret.B() = ((_c0.GetB() + (_c0.GetB() >> 7)) * _c1.GetB()) >> 8;
	Ret.G() = ((_c0.GetG() + (_c0.GetG() >> 7)) * _c1.GetG()) >> 8;
	Ret.R() = ((_c0.GetR() + (_c0.GetR() >> 7)) * _c1.GetR()) >> 8;
	Ret.A() = ((_c0.GetA() + (_c0.GetA() >> 7)) * _c1.GetA()) >> 8;
	return Ret;
}

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


bool CXR_Util::Render_Surface(
	int _Flags, CMTime _Time, CXW_Surface* _pSurface, CXW_SurfaceKeyFrame* _pSurfKey, CXR_Engine* _pEngine, CXR_VBManager *_pVBM, 
	const CMat4Dfp32* _pModel2World, const CMat4Dfp32* _pWorld2View, const CMat4Dfp32* _pModel2View, 
	CXR_VertexBufferGeometry* _pVB, fp32 _BasePriority, fp32 _PriorityOffset, CXR_RenderSurfExtParam* _pParams)
{
#ifdef M_Profile
	_pVBM->m_Stats_nRenderSurface++;
#endif
	int nLayers = _pSurfKey->m_lTextures.Len();
	const CXW_SurfaceLayer* pLayers = _pSurfKey->m_lTextures.GetBasePtr();

	M_PRECACHEMEM(pLayers, nLayers * sizeof(CXW_SurfaceLayer));

	// -------------------------------------------------------------------
	//  Resolve transformation
	// -------------------------------------------------------------------
	const CMat4Dfp32* pVBMatrix = NULL;

	if (!_pModel2View && !(_Flags & RENDERSURFACE_MATRIXSTATIC_M2V))
	{
		if (_pModel2World && _pWorld2View)
		{
			CMat4Dfp32* pMat = _pVBM->Alloc_M4();
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
					_pModel2View = _pVBM->Alloc_M4(*_pModel2World);
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
					_pModel2View = _pVBM->Alloc_M4(*_pWorld2View);
					if (!_pModel2View) return false;
				}
				_Flags |= RENDERSURFACE_MATRIXSTATIC_M2V;
			}
			else
				_pModel2View = NULL;
		}
	}

	if (_pModel2View && (_Flags & RENDERSURFACE_MATRIXSTATIC_M2V))
		pVBMatrix = _pModel2View;
	else
	{
		if (_pModel2View)
		{
			pVBMatrix = _pVBM->Alloc_M4(*_pModel2View);
			if (!pVBMatrix) return false;
		}
	}

	// -------------------------------------------------------------------
	//  Subdivision surface?
	// -------------------------------------------------------------------
/*	if (_pSurface->m_Flags & XW_SURFFLAGS_SUBDIVISION)
	{
		CXR_VertexBuffer* pVBNewChain = SubdivisionSurface_TesselateVB(_pVBM, _pVB, *_pModel2View, *_pModel2View);
		if (!pVBNewChain) return false;
		_pVB = pVBNewChain;
	}*/

	// -------------------------------------------------------------------
	//  Get some info
	// -------------------------------------------------------------------

//	int ChainLen = _pVB->GetChainLen();

	fp32 BasePriority = _BasePriority;
	fp32 PriorityOffset = _PriorityOffset;

	int bDepthFog = !(_pSurface->m_Flags & XW_SURFFLAGS_NOFOG) && _pEngine && _pEngine->m_pCurrentFogState->DepthFogEnable() && (_Flags & RENDERSURFACE_DEPTHFOG);
//	int bVertexFog = !(_pSurface->m_Flags & XW_SURFFLAGS_NOFOG) && _pEngine && _pEngine->m_pCurrentFogState->VertexFogEnable() && (_Flags & RENDERSURFACE_VERTEXFOG);
	fp32 DepthFogScale = (_pParams) ? _pParams->m_DepthFogScale : 1.0f;

	int nMultiTexture = (_pEngine) ? _pEngine->m_RenderCaps_nMultiTextureEnv : 2;
#ifndef M_RTM
	int ShowVBTypes = (_pEngine) ? _pEngine->m_ShowVBTypes : 0;
#endif

//	int RenderCaps = (_pEngine) ? _pEngine->m_RenderCaps_Flags : 0;
	int LightMapTextureID = (_pParams) ? _pParams->m_TextureIDLightMap : 0;

	bool bSinglePassFog = false;

	// -------------------------------------------------------------------
	//  Examine surface composition if any fog is to be applied.
	// -------------------------------------------------------------------

	// Never single-pass if RENDERSURFACE_ADDLIGHT is specified.
//	 || bVertexFog
	if ((bDepthFog) && !(_Flags & RENDERSURFACE_ADDLIGHT))
	{
		const int bNeverSinglePassFog = (_pSurface->m_Requirements & XW_SURFREQ_NV20);

#ifndef M_RTM
		if (bNeverSinglePassFog && ShowVBTypes == 3)
			ConOut(CStrF("bNeverSinglePassFog on buffer %.8x, Surface %s, Req %.4x, Flags %.8x", _pVB, _pSurface->m_Name.Str(), _pSurface->m_Requirements, _pSurface->m_Flags));
#endif

		bSinglePassFog = 
			// Always if we only have one layer and (multitexture or not lightmap)
			(nLayers == 1) && 
			((nMultiTexture > 1) || !LightMapTextureID) &&
			!bNeverSinglePassFog;

		if (!bSinglePassFog && !bNeverSinglePassFog)
		{
			// No?, check thoroughly...
			bSinglePassFog = true;

			// Examine each group in the surface independently.
			int iLayer = 0;
			while(iLayer < nLayers)
			{
				// Find base-layer for the group and count the layers while we're at it.
				int nGroupTextures = 1;
				while((iLayer < nLayers) && (pLayers[iLayer].m_Flags & XW_LAYERFLAGS_GROUP))
				{
					if (!(pLayers[iLayer].m_Flags & XW_LAYERFLAGS_INVISIBLE)) nGroupTextures++;
					iLayer++;
				}
				if (iLayer >= nLayers)
				{
					ConOut(CStrF("§cf80WARNING: (CXR_Util::Render_Surface) Last layer in surface '%s' was grouped with nothing.", (char*) _pSurface->m_Name) );
					return false;
				}

				if (!(pLayers[iLayer].m_Flags & XW_LAYERFLAGS_INVISIBLE))
				{
					// Check blendmode
					if ((pLayers[iLayer].m_RasterMode != CRC_RASTERMODE_NONE) &&
						(pLayers[iLayer].m_RasterMode != CRC_RASTERMODE_ALPHABLEND) &&
						(pLayers[iLayer].m_RasterMode != CRC_RASTERMODE_ADD) &&
						(pLayers[iLayer].m_RasterMode != CRC_RASTERMODE_ALPHAADD))
					{
						bSinglePassFog = false;
						break;
					}

					if (!(pLayers[iLayer].m_Flags & XW_LAYERFLAGS_CUSTOMTEXENV))
					{
						// Lighting?
						if (pLayers[iLayer].m_Flags & XW_LAYERFLAGS_LIGHTING)
						{
							// If we need to apply a lightmap, one more texture channel is needed.
							if (LightMapTextureID)
								nGroupTextures++;

							// If vertex-lighting is used on grouped layers we must multipass render lighting, and thus also multipass render fog
							else if (nGroupTextures > 1)
								bSinglePassFog = false;
						}

						// If we can't render this group in a single pass, fail.
						if (nGroupTextures > nMultiTexture)
						{
							bSinglePassFog = false;
							break;
						}
					}
				}

				iLayer++;
			}
		}
	}

	bool bSinglePassDepthFog = bDepthFog && bSinglePassFog;
//	bool bSinglePassVertexFog = bVertexFog && bSinglePassFog;

	// AFAIK, black fog works on all surfaces. If not, add logic here for
	// disabling it, and an extra pass will be rendered instead.
	bool bSinglePassBlackDepthFog = false; // bDepthFog && !bSinglePassDepthFog;

	// -------------------------------------------------------------------
	//  Vertex fog
	// -------------------------------------------------------------------
/*	if (bVertexFog)
	{
		if (bSinglePassVertexFog)
		{
			_pEngine->GetFogState()->SetTransform(_pModel2World);
			CXR_VertexBuffer* pPrevVB = NULL;
			CXR_VertexBuffer* pTmpVB = _pVB;
			CXR_VertexBuffer* pFirstVB = NULL;
			while(pTmpVB)
			{
				fp32* pFogCoords = _pEngine->GetFogState()->VertexFog_EvalCoord(_pVBM, pTmpVB);
				if (pFogCoords)
				{
					pTmpVB->Geometry_FogArray(pFogCoords);

					if (!pFirstVB)
						pFirstVB = pTmpVB;
					else
						pPrevVB->m_pNextVB = pTmpVB;
					pPrevVB = pTmpVB;
				}

				pTmpVB = pTmpVB->m_pNextVB;
			}

			if (!pFirstVB) return true;
			MACRO_COPYSTATE(_pVB, pFirstVB);
			_pVB = pFirstVB;
		}
		else
		{
			CXR_VertexBuffer* pFogVB = _pVBM->Alloc_VBChainOnly(ChainLen);
 			if (!pFogVB) return false;

			MACRO_COPYCHAINGEOMETRY(_pVB, pFogVB);

			pFogVB->m_Priority = BasePriority + CXR_VBPRIORITY_DEPTHFOG * PriorityOffset;
			pFogVB->Matrix_Set(pVBMatrix);

			CXW_SurfaceLayer* pLayer = &_pSurfKey->m_lTextures[0];
			bool bNoZWrite = (pLayer->m_Flags & XW_LAYERFLAGS_NOZWRITE) != 0;

			_pEngine->GetFogState()->SetTransform(_pModel2World);

			CXR_VertexBuffer* pFirstVB = NULL;
			CXR_VertexBuffer* pFirstFogVB = NULL;
			CXR_VertexBuffer* pPrevVB = NULL;
			CXR_VertexBuffer* pPrevFogVB = NULL;
			CXR_VertexBuffer* pTmpVB = _pVB;
			CXR_VertexBuffer* pTmpFogVB = pFogVB;

			while(pTmpVB && pTmpFogVB)
			{
				if (_pEngine->GetFogState()->VertexFog_Eval(_pVBM, pTmpFogVB, bNoZWrite))
				{
					if (!pFirstVB)
					{
						pFirstVB = pTmpVB;
						pFirstFogVB = pTmpFogVB;
					}
					else
					{
						pPrevVB->m_pNextVB = pTmpVB;
						pPrevFogVB->m_pNextVB = pTmpFogVB;
					}
					pPrevVB = pTmpVB;
					pPrevFogVB = pTmpFogVB;
				}

				pTmpVB = pTmpVB->m_pNextVB;
				pTmpFogVB = pTmpFogVB->m_pNextVB;
			}

			if (!pFirstVB || !pFirstFogVB) return true;
			MACRO_COPYSTATE(_pVB, pFirstVB);
			MACRO_COPYSTATE(pFogVB, pFirstFogVB);

			if (pFirstFogVB) _pVBM->AddVB(pFogVB);
		}
	}*/

	// -------------------------------------------------------------------
	//  Examine layers to determine if the surface can be completely hardware accelerated

	bool bHWAccelerated = true;

#ifndef PLATFORM_CONSOLE
	{
		CXR_SurfaceContext* pSC;
		if(!_pEngine)
		{
			MACRO_GetRegisterObject(CXR_SurfaceContext, pTempSC, "SYSTEM.SURFACECONTEXT");
			pSC = pTempSC;
			if(!pSC)
				Error_static("-", "No surface-context available.");
		}
		else
			pSC = _pEngine->m_pSC;

		CXR_VBOperatorContext Context;
		Context.m_pEngine = _pEngine;

		for(int iTxt = 0; iTxt < nLayers; iTxt++)
		{
			const CXW_SurfaceLayer* pLayer = &pLayers[iTxt];
			
/*			if (((pLayer->m_Color & 0xff000000) != 0xff000000) ||
				((pLayer->m_Color != 0xffffffff) && !(pLayer->m_Flags & XW_LAYERFLAGS_LIGHTING)))
			{
				bHWAccelerated = false;
			}*/

			for(int iOper = 0; iOper < pLayer->m_lOper.Len(); iOper++)
			{
				const CXW_LayerOperation& Oper = pLayer->m_lOper[iOper];
//ConOut(CStrF("    OpCode %d, Components %.8x, iClass %d", Oper.m_OpCode, Oper.m_Components, Oper.m_iOperatorClass));
				switch(Oper.m_OpCode)
				{
				case XW_LAYEROPCODE_OPERATOR :
					{
//						CXR_VBOperator* pVBOperator = pSC->VBOperator_Get(Oper.m_iOperatorClass);
						CXR_VBOperator* pVBOperator = Oper.m_pOperator;
						if (pVBOperator && !pVBOperator->OnTestHWAccelerated(Context, NULL, Oper))
						{
#ifdef	PLATFORM_PS2
							scePrintf( "Operator '%s' is not hw accelerated\n", pVBOperator->GetDesc().Str() );
#endif	// PLATFORM_PS2
							bHWAccelerated = false;
						}
					}
					break;
				default :;
				}

				if (!bHWAccelerated) break;
			}
			if (!bHWAccelerated) break;
		}
	}
#endif

	// -------------------------------------------------------------------
	//  If surface cannot be hardware accelerated we must convert input
	//  to software vertex-buffers.
	// -------------------------------------------------------------------
#ifndef	PLATFORM_PS2
	if (!bHWAccelerated)
	{
		// First, check if any convertion is needed.

		if (_pVB->IsVBIDChain())
		{
			M_TRACEALWAYS("(CXR_Util::Render_Surface) Software vertex processing fallback has been disabled. Rendering errors may occur. Surface: '%s'\n", _pSurface->m_Name.Str());
#if 0
			CXR_VBContext* pVBCtx;
			if(!_pEngine)
			{
				MACRO_GetRegisterObject(CXR_VBContext, pTempVBCtx, "SYSTEM.VBCONTEXT");
				pVBCtx = pTempVBCtx;
				if(!pVBCtx)
					Error_static("-", "No surface-context available.");
			}
			else
				pVBCtx = _pEngine->m_pRender->VB_GetVBContext();


			CXR_VBIDChain* pSrc = _pVB->GetVBIDChain();
			CXR_VBChain* pNewFirst = NULL;
			CXR_VBChain* pNewLast = NULL;

			CXR_VertexBuffer *pBuf = _pVBM->Alloc_VB();
			*pBuf = *_pVB;
			_pVB = pBuf;

			while(pSrc)
			{
				CXR_VBChain* pNew = _pVBM->Alloc_VBChain();
				if (!pNew) 
					return false;

				// Fix links
				if (!pNewFirst)
					pNewFirst = pNew;
				else
					pNewLast->m_pNextVB = pNew;
				pNewLast = pNew;

				CRC_BuildVertexBuffer BuildBuffer;
				pVBCtx->VB_Get(pSrc->m_VBID, BuildBuffer, VB_GETFLAGS_FALLBACK);
				if (!_pVBM->ConvertVertexBuildBuffer(*pNew, BuildBuffer))
					return false;

				if (pSrc->m_piPrim)
				{
					pNew->m_piPrim = pSrc->m_piPrim;
					pNew->m_nPrim = pSrc->m_nPrim;
					pNew->m_PrimType = pSrc->m_PrimType;
				}

				pSrc = pSrc->m_pNextVB;
			}

			_pVB->m_pVBChain = pNewFirst;
			// Set chain type
			_pVB->m_Flags = (_pVB->m_Flags & (~(CXR_VBFLAGS_VBCHAIN | CXR_VBFLAGS_VBIDCHAIN))) | CXR_VBFLAGS_VBCHAIN;
#endif
		}
	}
#endif	// PLATFORM_PS2

	// -------------------------------------------------------------------
	// Precache operators
	// -------------------------------------------------------------------
	for(int iTxt = 0; iTxt < nLayers; iTxt++)
	{
		const CXW_SurfaceLayer* pLayer = &pLayers[iTxt];

		// m_lOper is a custom thin array that stores the current length in *this, so this data
		// is part of the layer array memory block. Otherwise pLayer->m_lOper.Len() would be a cache miss.
		if (pLayer->m_lOper.Len())
		{
			M_PRECACHEMEM(pLayer->m_lOper.GetBasePtr(), sizeof(CXW_LayerOperation) * pLayer->m_lOper.Len());
		}
	}

	// -------------------------------------------------------------------
	//  Render layers
	// -------------------------------------------------------------------
	const uint  MaxFinalVB = 64;
	int nFinalGroups = 0;
	CXR_VertexBuffer* lpFinalGroupVBChains[MaxFinalVB];		// Surely, this must be sufficient.

	fp32 Priority = 0;

	CXR_VertexBuffer*M_RESTRICT pVB = NULL;
	int nChannels = 1;

#ifdef XR_DEFAULTPOLYOFFSETSCALE
	fp32 PolygonOffsetScale = XR_DEFAULTPOLYOFFSETSCALE;
	fp32 PolygonOffset = XR_DEFAULTPOLYOFFSET + -0.5f;
	fp32 PolygonOffsetDelta = -0.5f;
#else
	fp32 PolygonOffsetScale = 0.0f;
	fp32 PolygonOffset = -0.5f;
	fp32 PolygonOffsetDelta = -0.5f;
#endif

	int bPolygonOffset = _pSurface->m_Flags & XW_SURFFLAGS_POLYGONOFFSET;

	for(int iTxt = 0; iTxt < nLayers; iTxt++)
	{
		const CXW_SurfaceLayer* pLayer = &pLayers[iTxt];

		// Invisible or a disabled shader layer?
		if (pLayer->m_Flags & XW_LAYERFLAGS_INVISIBLE ||
			((_Flags & RENDERSURFACE_NOSHADERLAYERS) && pLayer->m_Type))
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

		if (!pVB)
		{
			pVB = _pVBM->Alloc_VBAttrib();
			if (!pVB) 
				return false;

//			if (_pEngine) 
//				_pEngine->SetDefaultAttrib(pVB->m_pAttrib);

#ifdef XR_DEFAULTPOLYOFFSETSCALE
			pVB->m_pAttrib->Attrib_PolygonOffset(XR_DEFAULTPOLYOFFSETSCALE, XR_DEFAULTPOLYOFFSET);
#endif
			pVB->Matrix_Set(pVBMatrix);
			pVB->CopyVBChain(_pVB);
			pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;
//			MACRO_COPYCHAINGEOMETRY_NOTEXCOORD(_pVB, pVB);
//			MACRO_COPYCHAINGEOMETRY(_pVB, pVB);
		}

		CRC_Attributes*M_RESTRICT pA = pVB->m_pAttrib;

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
			Priority += PriorityOffset;

			uint AttrFlags = pA->m_Flags;

			if (bPolygonOffset)
			{
				pA->m_PolygonOffsetScale = PolygonOffsetScale;
				pA->m_PolygonOffsetUnits = PolygonOffset; 
				AttrFlags |= CRC_FLAGS_POLYGONOFFSET;
//				pA->Attrib_PolygonOffset(PolygonOffsetScale, PolygonOffset);
				PolygonOffset += PolygonOffsetDelta;
			}

			MACRO_SETDEFAULTATTRIBUTES(pA, _pSurface, _pSurfKey, pLayer);

			// _pEngine->SetDefaultAttrib()
			if (_pEngine)
			{
				if (_pEngine->m_bCurrentVCMirrored)
					AttrFlags |= CRC_FLAGS_CULLCW;
				else
					AttrFlags &= ~CRC_FLAGS_CULLCW;
			}

			// Color-Write
			if (pLayer->m_Flags & XW_LAYERFLAGS_NOCOLORWRITE)
				AttrFlags &= ~CRC_FLAGS_COLORWRITE;
			else
				AttrFlags |= CRC_FLAGS_COLORWRITE;

			// Alpha-Write
			if (pLayer->m_Flags & XW_LAYERFLAGS_NOALPHAWRITE)
				AttrFlags &= ~CRC_FLAGS_ALPHAWRITE;
			else
				AttrFlags |= CRC_FLAGS_ALPHAWRITE;

			// Z-Write/Compare
			if (pLayer->m_Flags & XW_LAYERFLAGS_NOZWRITE)
				AttrFlags &= ~CRC_FLAGS_ZWRITE;
			else
				AttrFlags |= CRC_FLAGS_ZWRITE;
			pA->Attrib_ZCompare(pLayer->m_ZFunc);

			// Alpha-compare?
			if (pLayer->m_Flags & XW_LAYERFLAGS_ALPHACOMPARE)
				pA->Attrib_AlphaCompare(pLayer->m_AlphaFunc, pLayer->m_AlphaRef);
			else
				pA->Attrib_AlphaCompare(CRC_COMPARE_ALWAYS, 0);

			// Rastermode
			if(pLayer->m_RasterMode)
			{
				pA->m_SourceDestBlend = CRC_Attributes::GetSourceDestBlend(pLayer->m_RasterMode);
				AttrFlags |= CRC_FLAGS_BLEND;
			}

			pA->m_Flags = AttrFlags;

			// DepthFog
			if (bDepthFog && !(pLayer->m_Flags & XW_LAYERFLAGS_NOFOG))
			{
				if (bSinglePassDepthFog)
				{
					if (pLayer->m_Flags & XW_LAYERFLAGS_FOGBLACK)
					{
						_pEngine->m_pCurrentFogState->SetDepthFogBlack(pA, DepthFogScale);
					}
					else if (pLayer->m_Flags & XW_LAYERFLAGS_FOGCOLOR)
					{
						_pEngine->m_pCurrentFogState->SetDepthFog(pA, -1, DepthFogScale);	// Pass -1 means force depthfog to fogcolor
					}
					else
						_pEngine->m_pCurrentFogState->SetDepthFog(pA, iTxt, DepthFogScale);
				}
				else
				{
					if (bSinglePassBlackDepthFog)
					{
						if((pA->m_SourceDestBlend != MAKE_SOURCEDEST_BLEND(CRC_BLEND_DESTCOLOR, CRC_BLEND_ZERO)) &&
						   (pA->m_SourceDestBlend != MAKE_SOURCEDEST_BLEND(CRC_BLEND_DESTCOLOR, CRC_BLEND_ONE)) &&
						   (pA->m_SourceDestBlend != MAKE_SOURCEDEST_BLEND(CRC_BLEND_DESTCOLOR, CRC_BLEND_SRCCOLOR)))
						_pEngine->m_pCurrentFogState->SetDepthFogBlack(pA, DepthFogScale);
					}
				}
			}

			if (pLayer->m_Flags & 0x100)
			{
				pA->Attrib_Enable(CRC_FLAGS_STENCIL);
				pA->Attrib_StencilRef(0, 255);
				pA->Attrib_StencilFrontOp(CRC_COMPARE_EQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
				pA->Attrib_StencilWriteMask(0);
			}

			// Vertex-fog (fogcoords?)
/*			if (pVB->m_pFog)
			{
				_pEngine->GetFogState()->VertexFog_SetFogCoord(pA);	// Must be set AFTER rastermode
			}*/
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
//		if (TextureID && _pEngine && !_pEngine->GetTC()->IsValidID(TextureID))
//			M_ASSERT(0, "?");
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
			Context.m_AnimTime = _Time;
			Context.m_AnimTimeWrapped = _pSurfKey->m_AnimTimeWrapped;
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

			if (pVB->IsVBChain())
			{
				CXR_VBChain* pVBChain = pVB->GetVBChain();;
				if (pVBChain)
				{
					while(iFree < CRC_MAXTEXCOORDS&& pVBChain->m_pTV[iFree] != NULL) iFree++;
					if (iFree == CRC_MAXTEXCOORDS) iFree = -1;
					Context.m_iFreeTexCoordSet = iFree;
				}
			}

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
//						CXR_VBOperator* pVBOperator = pSC->VBOperator_Get(Oper.m_iOperatorClass);
						CXR_VBOperator* pVBOperator = Oper.m_pOperator;
						//ConOut(CStrF("    pVBOperator %.8x", pVBOperator));
						if (pVBOperator)
						{
//							CXR_VertexBuffer* pTargetVB = pVB;
//							CXR_VertexBuffer* pSrcVB = _pVB;

							if (!pVBOperator->OnOperate(Context, Oper, pVB))
								return false;

							if (!pVBOperator->OnOperateFinish(Context, Oper, pVB))
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
		if ((_Flags & RENDERSURFACE_ALPHABLEND) ||
			(pLayer->m_Flags & XW_LAYERFLAGS_USERCOLOR))
		{
			CVec4Dfp32 FadeColor;
			if(pLayer->m_Flags & XW_LAYERFLAGS_USERCOLOR && pLayer->m_iUserColor > 15 && _pEngine)
				CPixel32(_pEngine->m_lEngineUserColors[pLayer->m_iUserColor & 15]).Assign(FadeColor);
			else
				((_pParams) ? _pParams->m_lUserColors[pLayer->m_iUserColor & 1] : CPixel32(0xffffffff)).Assign(FadeColor);
			CVec4Dfp32 Color;
			Color << pLayer->m_HDRColor;
//			pA->Attrib_Disable(CRC_FLAGS_ZWRITE);

			// Translate rasterization mode
			if (_Flags & RENDERSURFACE_ALPHABLEND && (FadeColor[3] < 255))
			{
				switch(pA->m_SourceDestBlend)
				{
				case MAKE_SOURCEDEST_BLEND(CRC_BLEND_ONE, CRC_BLEND_ZERO):	//CRC_RASTERMODE_NONE
					{
						Color.CompMul(FadeColor, Color);
						pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
					}
					break;

				case MAKE_SOURCEDEST_BLEND(CRC_BLEND_SRCALPHA, CRC_BLEND_INVSRCALPHA):	//CRC_RASTERMODE_ALPHABLEND
				case MAKE_SOURCEDEST_BLEND(CRC_BLEND_SRCALPHA, CRC_BLEND_ONE):	//CRC_RASTERMODE_ALPHAADD
					{
						Color.CompMul(FadeColor, Color);
					}
					break;

				case MAKE_SOURCEDEST_BLEND(CRC_BLEND_DESTCOLOR, CRC_BLEND_ZERO):	//CRC_RASTERMODE_MULTIPLY
					{
						Color *= FadeColor[3];
						pA->Attrib_Enable(CRC_FLAGS_BLEND);
						pA->Attrib_SourceBlend(CRC_BLEND_DESTCOLOR);
						pA->Attrib_DestBlend(CRC_BLEND_INVSRCALPHA);
					}
					break;

				case MAKE_SOURCEDEST_BLEND(CRC_BLEND_ONE, CRC_BLEND_INVSRCALPHA):	//CRC_RASTERMODE_ONE_INVALPHA
					{
						Color *= FadeColor[3];
					};
					break;

				default :
					ConOutD(CStrF("Unsupported fade %d,%d", pA->m_SourceBlend, pA->m_DestBlend));
					break;
				}
			}
			else
			{
				Color.CompMul(FadeColor, Color);
			}

			if (pA->m_AlphaCompare != CRC_COMPARE_ALWAYS)
			{
//				int Old = pA->m_AlphaRef;
				pA->m_AlphaRef = int(pA->m_AlphaRef * FadeColor[3]) >> 8;
//			ConOut(CStrF("AlphaCompare %d -> %d, %d", Old, pA->m_AlphaRef, pA->m_AlphaCompare));
			}

//			ConOut(CStrF("Color %.8x, RM %d", Color, pA->m_RasterMode));

			// Vertex lighting
			if (pLayer->m_Flags & XW_LAYERFLAGS_LIGHTING)
			{

				if (_pParams && (_pParams->m_pLights ||_pParams->m_pLightFieldAxes))
				{
					Color.CompMul(_pEngine->m_RenderSurfaceLightScale, Color);
					ApplyLightingRenderState(_pEngine, _pVBM, pA, _pParams->m_pLights, 
						_pParams->m_nLights, Color * (1.0f / 255.0f),(_Flags & RENDERSURFACE_LIGHTNONORMAL) != 0,
						_pParams->m_pLightFieldAxes,_pModel2World);
					/*
					// FIXME:
					// Fixed, not tested thoroughly tho' - ae
					pA->m_nLights = _pParams->m_nLights;
					CRC_Light * pL = _pVBM->Alloc_Lights(pA->m_nLights);
					
					for(int iRenderLight = 0; iRenderLight < _pParams->m_nLights; iRenderLight++)
					{
						const CXR_Light& Lg = _pParams->m_pLights[iRenderLight];
						CRC_Light & L = pL[iRenderLight];

						Lg.GetPosition().MultiplyMatrix(LTransform, L.m_Pos);
						//L.m_Pos = Lg.GetPosition();
						L.m_Color = Lg.m_IntensityInt32 * Color;
						L.m_Type = CRC_LIGHTTYPE_POINT;
						L.m_Ambient = 0;
						L.m_Direction = 0;
						L.m_Attenuation[0] = 0;
						L.m_Attenuation[1] = Lg.m_RangeInv;
						L.m_Attenuation[2] = 0;
					}
					pA->m_pLights = pL;
					pA->Attrib_Enable(CRC_FLAGS_LIGHTING);
					*/
					M_VSt_V4f32_Pixel32(Color.v, &pVB->m_Color);
				}
				else
				{
					M_VSt_V4f32_Pixel32(M_VMul(Color.v, M_VLd_Pixel32_f32(&pVB->m_Color)), &pVB->m_Color);
				}
			}
			else
				M_VSt_V4f32_Pixel32(Color.v, &pVB->m_Color);

//			_pVBM->AddVB(pVB);
			lpFinalGroupVBChains[nFinalGroups++] = pVB;
		}

		// -------------------------------------------------------------------
		// Apply lighting
		else if (!(_Flags & RENDERSURFACE_FULLBRIGHT) &&
			 (pLayer->m_Flags & XW_LAYERFLAGS_LIGHTING))
		{
/*			if (pLayer->m_Flags & XW_LAYERFLAGS_CUSTOMTEXENV)
			{
				if (LightMapTextureID)
				{
					_pVBM->AddVB(pVB);
					lpFinalGroupVBChains[nFinalGroups++] = pVB;

					ConOut(CStrF("§cf80WARNING: (CXR_Util::Render_Surface) Lightmaps cannot be used with custom texture-environment layers, Surface: %s", _pSurface->m_Name.Str()));
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
							if (pLayer->m_Flags & XW_LAYERFLAGS_SPECULAR)
								MACRO_COPYCHAINGEOMETRY_COLOR(_pVB, pVB, Spec, Col, pLayer->m_Color)
							else
								MACRO_COPYCHAINGEOMETRY_COLOR(_pVB, pVB, Col, Col, pLayer->m_Color)
						}
						else
						{
							ConOut("§cf80WARNING: (CXR_Util::Render_Surface) Vertex lighting on static VB, but no lights.");
						}
					}

					_pVBM->AddVB(pVB);
					lpFinalGroupVBChains[nFinalGroups++] = pVB;
				}
			}
			else*/
			{
				if (_Flags & RENDERSURFACE_MODULATELIGHT)
				{
					// -------------------------------------------------------------------
					//  Modulate light
					//
					//  Lighting is already present in framebuffer, so this surface
					//  should be modulated with it.
					// -------------------------------------------------------------------

					pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_MULTIPLY);
//					_pVBM->AddVB(pVB);
					lpFinalGroupVBChains[nFinalGroups++] = pVB;

				}
				else if (_Flags & RENDERSURFACE_ADDLIGHT)
				{
					// -------------------------------------------------------------------
					//  Additive light (This means that lightmap and surface should be separated
					//  so that dynamic lights can be rendered inbetween the passes.
					// -------------------------------------------------------------------
					pVB->m_Priority += _PriorityOffset;
					pA->Attrib_RasterMode(CRC_RASTERMODE_MULTIPLY);
//					_pVBM->AddVB(pVB);
					lpFinalGroupVBChains[nFinalGroups++] = pVB;

					CXR_VertexBuffer* pVB2 = _pVBM->Alloc_VBAttrib();
					if (!pVB2) 
						return false;
					CRC_Attributes* pA2 = pVB2->m_pAttrib;

					if (_pEngine) 
						_pEngine->SetDefaultAttrib(pA2);
#ifdef XR_DEFAULTPOLYOFFSETSCALE
					pA2->Attrib_PolygonOffset(XR_DEFAULTPOLYOFFSETSCALE, XR_DEFAULTPOLYOFFSET);
#endif
					pVB2->m_Priority = BasePriority + CXR_VBPRIORITY_LIGHTMAP;
//					pVB2->m_Priority = pVB->m_Priority-_PriorityOffset;
					Priority += _PriorityOffset;

					pVB2->Matrix_Set(pVBMatrix);
					pVB2->CopyVBChain(_pVB);
					pVB2->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

					if (LightMapTextureID)
					{
						pA2->Attrib_TextureID(0, LightMapTextureID);
				//		MACRO_SETCHAINGEOMETRY_COLOR(pVB2, pLayer->m_Color);
						pA2->Attrib_TexCoordSet(0, 1);
				//		MACRO_COPYCHAINGEOMETRY_TEXCOORD(_pVB, pVB2, 1, 0);
					}
					else
					{
/*						if (pLayer->m_Flags & XW_LAYERFLAGS_SPECULAR)
							MACRO_COPYCHAINGEOMETRY_COLOR(_pVB, pVB2, Spec, Col, pLayer->m_Color)
						else*/
//							MACRO_COPYCHAINGEOMETRY_COLOR(_pVB, pVB2, Col, Col, )

//						pVB2->m_Color *= CPixel32(pLayer->m_HDRColor.GetPixel32());
						vec128 col = M_VMul(M_VLd_V4f16_f32(&pLayer->m_HDRColor), M_VLd_Pixel32_f32((uint32*)&pVB2->m_Color));
						M_VSt_V4f32_Pixel32(col, (uint32*)&pVB2->m_Color);
					}

					MACRO_SETDEFAULTATTRIBUTES(pA2, _pSurface, _pSurfKey, pLayer);

/*					if (bPolygonOffset && pA->m_AlphaCompare == CRC_COMPARE_ALWAYS)
					{
						pA2->Attrib_PolygonOffset(PolygonOffsetScale, PolygonOffset);
						PolygonOffset += PolygonOffsetDelta;
						pA2->Attrib_ZCompare(CRC_COMPARE_LESSEQUAL);
					}
					else
						pA2->Attrib_ZCompare(CRC_COMPARE_EQUAL);*/

					pA2->Attrib_ZCompare(CRC_COMPARE_LESSEQUAL);
					pA2->Attrib_RasterMode(CRC_RASTERMODE_NONE);
					_pVBM->AddVB(pVB2);
				}
				else
				{
					// -------------------------------------------------------------------
					//  Standard light
					// -------------------------------------------------------------------
#ifdef	REQUIRE_SUPPORT_SINGLEPASSLIGHTMAP_OR_MULTIPASS_VERTEXLIGHT
					if (LightMapTextureID || 
						((nChannels > 1) && !(pLayer->m_Flags & XW_LAYERFLAGS_CUSTOMTEXENV)))
					{
						// Lightmap or vertex lighting on multitexture surface

						if (LightMapTextureID && 
							nMultiTexture > nChannels &&
							!(pLayer->m_Flags & XW_LAYERFLAGS_CUSTOMTEXENV))
						{
							// nChannels+1 multitexture is supported  (Lightmap only)
							pA->Attrib_TextureID(nChannels, LightMapTextureID);
							pA->Attrib_TexCoordSet(nChannels, 1);
//							MACRO_COPYCHAINGEOMETRY_TEXCOORD(_pVB, pVB, 1, nChannels);

/*							if (pLayer->m_Flags & XW_LAYERFLAGS_SPECULAR)
								MACRO_COPYCHAINGEOMETRY_COLOR(_pVB, pVB, Spec, Col, pLayer->m_Color)
							else*/
//								MACRO_COPYCHAINGEOMETRY_COLOR(_pVB, pVB, Col, Col, pLayer->m_Color)

//							pVB->m_Color *= CPixel32(pLayer->m_HDRColor.GetPixel32());
							vec128 col = M_VMul(M_VLd_V4f16_f32(&pLayer->m_HDRColor), M_VLd_Pixel32_f32((uint32*)&pVB->m_Color));
							M_VSt_V4f32_Pixel32(col, (uint32*)&pVB->m_Color);

//							_pVBM->AddVB(pVB);
							lpFinalGroupVBChains[nFinalGroups++] = pVB;
						}
						else
						{
							// Multipass
//							_pVBM->AddVB(pVB);
							lpFinalGroupVBChains[nFinalGroups++] = pVB;

							CXR_VertexBuffer* pVB2 = _pVBM->Alloc_VBAttrib();
							if (!pVB2) 
								return false;

							pVB2->Matrix_Set(pVBMatrix);
							pVB2->CopyVBChain(_pVB);
							pVB2->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;
//							MACRO_COPYCHAINGEOMETRY/*_NOTEXCOORD*/(_pVB, pVB2);
//							MACRO_COPYCHAINGEOMETRY_TEXCOORD(_pVB, pVB2, 1, 0);

							CRC_Attributes* pA = pVB2->m_pAttrib;
							if (_pEngine) 
								_pEngine->SetDefaultAttrib(pA);
#ifdef XR_DEFAULTPOLYOFFSETSCALE
							pA->Attrib_PolygonOffset(XR_DEFAULTPOLYOFFSETSCALE, XR_DEFAULTPOLYOFFSET);
#endif
							pVB2->m_Priority = BasePriority + Priority;
							Priority += _PriorityOffset;

							MACRO_SETDEFAULTATTRIBUTES(pA, _pSurface, _pSurfKey, pLayer);

							if (bPolygonOffset && pA->m_AlphaCompare == CRC_COMPARE_ALWAYS)
							{
								pA->Attrib_PolygonOffset(PolygonOffsetScale, PolygonOffset);
								PolygonOffset += PolygonOffsetDelta;
								pA->Attrib_ZCompare(CRC_COMPARE_LESSEQUAL);
							}
							else
								pA->Attrib_ZCompare(CRC_COMPARE_EQUAL);

							if (LightMapTextureID)
							{
								pA->Attrib_TextureID(0, LightMapTextureID);
								pA->Attrib_TexCoordSet(0, 1);
							}
							else if (_pParams && _pParams->m_pLights)
							{
/*								if (pLayer->m_HDRColor.IsNotOne())
								{
									if (!SetColorizedLights(pA, _pVBM, pLayer->m_HDRColor.GetPixel32(), _pParams->m_pLights, _pParams->m_nLights))
										return false;
								}
								else
								{
									pA->m_pLights = _pParams->m_pLights;
									pA->m_nLights = _pParams->m_nLights;
								}
								pA->Attrib_Enable(CRC_FLAGS_LIGHTING);
*/
							}
							else
							{
//								MACRO_COPYCHAINGEOMETRY_COLOR(_pVB, pVB2, Col, Col, pLayer->m_Color);
//								pVB2->m_Color *= CPixel32(pLayer->m_HDRColor.GetPixel32());
								vec128 col = M_VMul(M_VLd_V4f16_f32(&pLayer->m_HDRColor), M_VLd_Pixel32_f32((uint32*)&pVB2->m_Color));
								M_VSt_V4f32_Pixel32(col, (uint32*)&pVB2->m_Color);
							}

							pA->Attrib_RasterMode(CRC_RASTERMODE_MULTIPLY);
							_pVBM->AddVB(pVB2);
						}
					}
					else
#endif	// REQUIRE_SUPPORT_SINGLEPASSLIGHTMAP_OR_MULTIPASS_VERTEXLIGHT
					{
						// Vertex lighting
						if ( _pParams && (_pParams->m_pLights || _pParams->m_pLightFieldAxes))
						{
/*							if (pLayer->m_HDRColor.IsNotOne())
							{
								if (!SetColorizedLights(pA, _pVBM, pLayer->m_HDRColor.GetPixel32(), _pParams->m_pLights, _pParams->m_nLights))
									return false;
							}
							else
							{
								pA->m_pLights = _pParams->m_pLights;
								pA->m_nLights = _pParams->m_nLights;
							}
							pA->Attrib_Enable(CRC_FLAGS_LIGHTING);*/

							if (!pA->m_pTexGenAttr)
							{
								CVec4Dfp32 LayerColor;
								LayerColor << pLayer->m_HDRColor;
								//if( pLayer->m_Flags & XW_LAYERFLAGS_USERCOLOR )
								/*
								{
									CPixel32 Px = _pParams->m_lUserColors[0];
									CVec4Dfp32 UserColor = Px.operator CVec4Dfp32() * (1.0f/255.0f);
									LayerColor.CompMul(UserColor,LayerColor);
								}
								*/
								ApplyLightingRenderState(_pEngine, _pVBM, pA, _pParams->m_pLights, _pParams->m_nLights, 
									LayerColor,(_Flags & RENDERSURFACE_LIGHTNONORMAL) != 0,
									_pParams->m_pLightFieldAxes,_pModel2World);
							}
							else
								ConOutD("§cf80WARNING: (CXR_Util::RenderSurface) Can't apply lighting to layer using custom texgen/texenv.");

						}
						else
						{
							vec128 LayerColor = M_VLd_V4f16_f32(&pLayer->m_HDRColor);
							if (_pEngine)
								LayerColor = M_VMul(_pEngine->m_RenderSurfaceLightScale.v, LayerColor);
							LayerColor = M_VMul(M_VLd_Pixel32_f32(&_pVB->m_Color), LayerColor);
							M_VSt_V4f32_Pixel32(LayerColor, &pVB->m_Color);


/*							CVec4Dfp32 LayerColor;
							LayerColor << pLayer->m_HDRColor;
							if (_pEngine)
								LayerColor.CompMul(_pEngine->m_RenderSurfaceLightScale, LayerColor);
							LayerColor.CompMul(_pVB->m_Color.operator CVec4Dfp32(), LayerColor);
							pVB->m_Color = LayerColor;*/
						}

//						_pVBM->AddVB(pVB);
						lpFinalGroupVBChains[nFinalGroups++] = pVB;
					}
				}
			}
		}
		else
		{
//			MACRO_SETCHAINGEOMETRY_COLOR(pVB, pLayer->m_Color);
//			pVB->m_Color = MultiplyCPixel32(pVB->m_Color, pLayer->m_HDRColor.GetPixel32());

			vec128 col = M_VMul(M_VLd_V4f16_f32(&pLayer->m_HDRColor), M_VLd_Pixel32_f32((uint32*)&pVB->m_Color));
			M_VSt_V4f32_Pixel32(col, (uint32*)&pVB->m_Color);

//			_pVBM->AddVB(pVB);
			lpFinalGroupVBChains[nFinalGroups++] = pVB;
		}

		pVB = NULL;
		nChannels = 1;
	}

	// -------------------------------------------------------------------
	//  Depth fog
	// -------------------------------------------------------------------
	uint nFinalVBAdd = nFinalGroups;

	if (bDepthFog && !bSinglePassDepthFog && nFinalGroups)
	{
		const CXR_VertexBuffer* pFirstGroupVB = lpFinalGroupVBChains[0];

		// Render an multiply layer with black depth-fog
		if (!bSinglePassBlackDepthFog)
		{
			CXR_VertexBuffer*M_RESTRICT pBlackVB = _pVBM->Alloc_VBAttrib();

 			if (!pBlackVB) 
				return false;

			if (_pVB->IsVBIDChain())
			{
				pBlackVB->CopyVBChain(_pVB);
				pBlackVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;
			}
			else
			{
				if (!_pVBM->Alloc_VBChainCopy(pBlackVB, _pVB))
					return false;
	
				MACRO_COPYCHAINGEOMETRY_VERTEXONLY(_pVB->GetVBChain(), pBlackVB->GetVBChain());
			}

			CRC_Attributes*M_RESTRICT pA = pBlackVB->m_pAttrib;

			_pEngine->SetDefaultAttrib(pA);
#ifdef XR_DEFAULTPOLYOFFSETSCALE
			pA->Attrib_PolygonOffset(XR_DEFAULTPOLYOFFSETSCALE, XR_DEFAULTPOLYOFFSET);
#endif
			_pEngine->m_pCurrentFogState->SetDepthFogBlack(pA, DepthFogScale);
			pA->Attrib_RasterMode(CRC_RASTERMODE_MULTIPLY);

			// Get cull setting flag from first group
			if (pFirstGroupVB->m_pAttrib->m_Flags & CRC_FLAGS_CULL)
			{
				pA->Attrib_Enable(CRC_FLAGS_CULL);
			}
			else
			{
				pA->Attrib_Disable(CRC_FLAGS_CULL);
			}

			pA->Attrib_Disable(CRC_FLAGS_ZWRITE);

			bool bTexCoord = false;
			if (pFirstGroupVB->m_pAttrib->m_Flags & CRC_FLAGS_ZWRITE)
			{
				// Now, lets hope rasterization z is consistent or this will just make a mess.
				pA->Attrib_ZCompare(CRC_COMPARE_EQUAL);
			}
			else
			{
				// Surface appear to not use z-write, so we'll have to check if
				// we're using alpha-compare and emulate it as well as we can
				if (pFirstGroupVB->m_pAttrib->m_AlphaCompare != CRC_COMPARE_ALWAYS)
				{
					// Get texture if alphacompare is used
					bTexCoord = true;

					pA->m_AlphaCompare = pFirstGroupVB->m_pAttrib->m_AlphaCompare;
					pA->m_AlphaRef = pFirstGroupVB->m_pAttrib->m_AlphaRef;
					pA->Attrib_TextureID(0, pFirstGroupVB->m_pAttrib->m_TextureID[0]);
				}
			}

			if (pFirstGroupVB->m_pAttrib->m_Flags & CRC_FLAGS_STENCIL)
			{
				pA->m_StencilDWord1 = pFirstGroupVB->m_pAttrib->m_StencilDWord1;
				pA->m_StencilRef = pFirstGroupVB->m_pAttrib->m_StencilRef;
				pA->m_StencilFuncAnd = pFirstGroupVB->m_pAttrib->m_StencilFuncAnd;
				pA->m_StencilWriteMask = pFirstGroupVB->m_pAttrib->m_StencilWriteMask;
				pA->Attrib_Enable(CRC_FLAGS_STENCIL);
			}

			pBlackVB->m_Priority = BasePriority + CXR_VBPRIORITY_DEPTHFOG * PriorityOffset;
			pBlackVB->Matrix_Set(pVBMatrix);

			if (bTexCoord)
			{
				CXR_VBChain* pTmpBlackVB = pBlackVB->GetVBChain();
				const CXR_VBChain* pGroupVB = pFirstGroupVB->GetVBChain();
				while(pTmpBlackVB)
				{
					pTmpBlackVB->m_pTV[0] = pGroupVB->m_pTV[0];
					pTmpBlackVB->m_nTVComp[0] = pGroupVB->m_nTVComp[0];
					pGroupVB = pGroupVB->m_pNextVB;
					pTmpBlackVB = pTmpBlackVB->m_pNextVB;
				}
			}

			lpFinalGroupVBChains[nFinalVBAdd++] = pBlackVB;
//			_pVBM->AddVB(pBlackVB);
		}

		// Render an 'add' layer with the fog if the fog-color is not black.
		if (_pEngine->m_pCurrentFogState->m_DepthFogColor != 0)
		{
			CXR_VertexBuffer*M_RESTRICT pFogVB = _pVBM->Alloc_VBAttrib();
 			if (!pFogVB) return false;

			pFogVB->CopyVBChain(_pVB);
			pFogVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

			CRC_Attributes*M_RESTRICT pA = pFogVB->m_pAttrib;

			_pEngine->SetDefaultAttrib(pA);
#ifdef XR_DEFAULTPOLYOFFSETSCALE
			pA->Attrib_PolygonOffset(XR_DEFAULTPOLYOFFSETSCALE, XR_DEFAULTPOLYOFFSET);
#endif
			_pEngine->m_pCurrentFogState->SetDepthFog(pA, 0, DepthFogScale);

			pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);

			// Get cull setting flag from first group
			if (pFirstGroupVB->m_pAttrib->m_Flags & CRC_FLAGS_CULL)
				pA->Attrib_Enable(CRC_FLAGS_CULL);
			else
				pA->Attrib_Disable(CRC_FLAGS_CULL);

			pA->Attrib_Disable(CRC_FLAGS_ZWRITE);

			pA->Attrib_TextureID(0, _pEngine->m_pCurrentFogState->m_Special000000TextureID);

//			bool bTexCoord = false;
			if (pFirstGroupVB->m_pAttrib->m_Flags & CRC_FLAGS_ZWRITE)
			{
				// Now, lets hope rasterization z is consistent or this will just make a mess.
				pA->Attrib_ZCompare(CRC_COMPARE_EQUAL);
			}
			else
			{
				// Surface appear to not use z-write, so we'll have to check if
				// we're using alpha-compare and emulate it as well as we can
				if (pFirstGroupVB->m_pAttrib->m_AlphaCompare != CRC_COMPARE_ALWAYS)
				{
					pA->m_AlphaCompare = pFirstGroupVB->m_pAttrib->m_AlphaCompare;
					pA->m_AlphaRef = pFirstGroupVB->m_pAttrib->m_AlphaRef;
					pA->Attrib_TextureID(0, pFirstGroupVB->m_pAttrib->m_TextureID[0]);

					pFogVB->m_Color = 0xff000000;
				}
			}
			if (pFirstGroupVB->m_pAttrib->m_Flags & CRC_FLAGS_STENCIL)
			{
				pA->m_StencilDWord1 = pFirstGroupVB->m_pAttrib->m_StencilDWord1;
				pA->m_StencilRef = pFirstGroupVB->m_pAttrib->m_StencilRef;
				pA->m_StencilFuncAnd = pFirstGroupVB->m_pAttrib->m_StencilFuncAnd;
				pA->m_StencilWriteMask = pFirstGroupVB->m_pAttrib->m_StencilWriteMask;
				pA->Attrib_Enable(CRC_FLAGS_STENCIL);
			}

			pFogVB->m_Priority = BasePriority + (CXR_VBPRIORITY_DEPTHFOG+0.5f) * PriorityOffset;
			pFogVB->Matrix_Set(pVBMatrix);

			lpFinalGroupVBChains[nFinalVBAdd++] = pFogVB;
//			_pVBM->AddVB(pFogVB);
		}
	}

#ifndef M_RTM
	if (ShowVBTypes)
	{
		if (((ShowVBTypes == 1) && !bHWAccelerated) ||
			((ShowVBTypes == 2) && !_pVB->IsVBIDChain()))
		{
			CXR_VertexBuffer* pShowVB = _pVBM->Alloc_VB();
 			if (!pShowVB)
				return false;
			if (!_pVBM->Alloc_VBChainCopy(pShowVB, _pVB))
				return false;
			MACRO_COPYCHAINGEOMETRY_VERTEXONLY(_pVB->GetVBChain(), pShowVB->GetVBChain());

			pShowVB->m_pAttrib = _pVBM->Alloc_Attrib();
			if (!pShowVB->m_pAttrib) 
				return false;
			pShowVB->m_pAttrib->SetDefault();
			pShowVB->m_pAttrib->Attrib_TextureID(0, _pEngine->GetTC()->GetTextureID("SPECIAL_00ff00"));
			pShowVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ADD);

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

			if (!_pVB->IsVBIDChain())
			{
				CXR_VBChain* pVB = _pVB->GetVBChain();
				while(pVB)
				{
					if (!pVB->BuildVertexUsage(_pVBM))
						return false;

					const uint16* piV = pVB->m_piVertUse;
//					int nVA = pVB->m_nV;
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
			}

			_pEngine->m_pRender->Matrix_Pop();
		}
	}
#endif

	M_ASSERT(nFinalVBAdd <= MaxFinalVB, "Overwrote some stack.");

	_pVBM->AddVBArray(lpFinalGroupVBChains, nFinalVBAdd);
	return true;
}

bool CXR_Util::Render_SurfaceArray(int _Flags, uint _nVB, CXR_VertexBufferGeometry* _lVB, CXW_Surface** _lpSurface, CMTime _Time, CXR_Engine* _pEngine, CXR_VBManager *_pVBM, 
						   const CMat4Dfp32* _pModel2World, const CMat4Dfp32* _pWorld2View, const CMat4Dfp32* _pModel2View, 
						   fp32* _lBasePriority, fp32 _PriorityOffset, CXR_RenderSurfExtParam* _pParams)
{
	const CMat4Dfp32* pVBMatrix = NULL;

	const uint MaxVBArray = 64;

	uint iVB = 0;
	while(iVB < _nVB)
	{
		uint nFast = 0;
		while((iVB + nFast < _nVB) && _lpSurface[iVB+nFast]->m_pFastPathVA)
			nFast++;

		if (nFast > MaxVBArray)
			nFast = MaxVBArray;

		if (nFast)
		{
//			if (_pSurface->m_pFastPathVA)
			{
				if (!pVBMatrix)
				{
					if (_pModel2View && (_Flags & RENDERSURFACE_MATRIXSTATIC_M2V))
						pVBMatrix = _pModel2View;
					else
					{
						if (_pModel2View)
						{
							pVBMatrix = _pVBM->Alloc_M4(*_pModel2View);
							if (!pVBMatrix) return false;
						}
					}
				}

				CXR_VertexBuffer* lpVB[MaxVBArray];

				uint VBMemSize = sizeof(CXR_VertexBuffer) * nFast;
				CXR_VertexBuffer* pVBArray = (CXR_VertexBuffer*)_pVBM->Alloc(VBMemSize, true);
				if (!pVBArray)
					return false;

				for(int i = 0; i < nFast; i++)
				{
//					pVBArray[i].Construct();
					lpVB[i] = pVBArray+i;
				}

				for(int i = 0; i < nFast; i++)
				{
					CXR_VertexBuffer*M_RESTRICT pVB = pVBArray+i;
					const CXR_VertexBufferGeometry* pVBSrc = &_lVB[iVB+i];
					CXR_VirtualAttributes_SurfaceBase* pVA = _lpSurface[iVB+i]->m_pFastPathVA;

					pVB->Construct_Geometry_VA_Priority(pVBSrc, pVA, _lBasePriority[iVB+i]);

//					pVB->CopyVBChain(pVBSrc);
					pVB->Matrix_Set(pVBMatrix);
//					pVB->m_pMatrixPaletteArgs = pVBSrc->m_pMatrixPaletteArgs;
//					pVB->SetVirtualAttr(pVA);
//					pVB->m_Priority = _lBasePriority[iVB+i];
					vec128 col = M_VMul(M_VLd_V4f16_f32(&pVA->m_HDRColor), M_VLd_Pixel32_f32((uint32*)&pVBSrc->m_Color));
					M_VSt_V4f32_Pixel32(col, (uint32*)&pVB->m_Color);
				}
				_pVBM->AddVBArray(lpVB, nFast);
			}
			iVB += nFast;
		}
		else
		{
			CXW_SurfaceKeyFrame* pSurfKey = _lpSurface[iVB]->GetFrame(0, _Time);
			Render_Surface(
				_Flags, _Time, _lpSurface[iVB], pSurfKey, _pEngine, _pVBM, 
				_pModel2World, _pWorld2View, _pModel2View, 
				&_lVB[iVB], _lBasePriority[iVB], _PriorityOffset, _pParams);
			iVB++;
		}
	}
	return true;
}

