
#include "PCH.h"

#include "XRShader.h"
#include "../MSystem/Raster/MRender_NVidia.h"		// Hmm.. not too fond of this dependecy, but at least MRndrGL_RegCombiners.h have no dependencies at all.
#include "XREngineVar.h"

MRTC_IMPLEMENT_DYNAMIC(CXR_Shader, CReferenceCount);


#define DST2_ATTENUATION

#define XR_COMPAREANDSETATTR(Val, Attr, ChgFlag) { if ((Val) != (Attr)) { (Attr) = (Val); AttrChg |= (ChgFlag); } }


#undef M_ASSERT
#define M_ASSERT(x, a) if(!(x)) M_BREAKPOINT;

#define XRSHADER_INSERTTEXGENATTR_TSVEC(Vec, Scale)	\
	{	\
		pTexGenAttr[nTexGenPos++] = Vec[0];	\
		pTexGenAttr[nTexGenPos++] = Vec[1];	\
		pTexGenAttr[nTexGenPos++] = Vec[2];	\
		pTexGenAttr[nTexGenPos++] = Scale;	\
	}

#define XRSHADER_CALC_TSEYEVEC(_pMat, _Eye)	\
	{	\
		const CMat4Dfp32* pMat = _pMat;	\
		for (int j = 0; j < 3; j++)	\
			_Eye[j] = - (pMat->k[3][0]*pMat->k[j][0] + pMat->k[3][1]*pMat->k[j][1] + pMat->k[3][2]*pMat->k[j][2]);	\
	}

#define XRSHADER_INSERTTEXGENATTR_LINEAR_SCREENPROJECTION	\
	{	\
		CMat4Dfp32& DestMat(*(CMat4Dfp32*)&pTexGenAttr[nTexGenPos]);	\
		if (m_pEngine)	\
		{	\
			CMat4Dfp32 Inv;	\
			_pVB->m_pTransform->InverseOrthogonal(Inv);	\
			_pVB->m_pTransform->Multiply(m_pEngine->m_Screen_TextureProjection, DestMat);	\
			DestMat.Transpose();	\
		}\
		else\
			DestMat.Unit();\
		nTexGenPos += 16;\
	}

#define XRSHADER_INSERTTEXGENATTR_PIXELINFO(_pMat)	\
	{	\
		const CMat4Dfp32& M = *_pMat;	\
		pTexGenAttr[nTexGenPos++] = M.k[0][0];	\
		pTexGenAttr[nTexGenPos++] = M.k[1][0];	\
		pTexGenAttr[nTexGenPos++] = M.k[2][0];	\
		pTexGenAttr[nTexGenPos++] = M.k[3][0];	\
		pTexGenAttr[nTexGenPos++] = M.k[0][1];	\
		pTexGenAttr[nTexGenPos++] = M.k[1][1];	\
		pTexGenAttr[nTexGenPos++] = M.k[2][1];	\
		pTexGenAttr[nTexGenPos++] = M.k[3][1];	\
		pTexGenAttr[nTexGenPos++] = M.k[0][2];	\
		pTexGenAttr[nTexGenPos++] = M.k[1][2];	\
		pTexGenAttr[nTexGenPos++] = M.k[2][2];	\
		pTexGenAttr[nTexGenPos++] = M.k[3][2];	\
	}

#define XRSHADER_ATTRIB_SETLIGHTRANGE(_Vec, _Light)	\
	_Vec.k[0] = _Light.m_RangeInv;	\
	_Vec.k[1] = _Light.m_Range;	\
	_Vec.k[2] = Sqr(_Light.m_RangeInv);	\
	_Vec.k[3] = m_FallOffExp_Current;

#define XRSHADER_ATTRIB_SETLIGHTDIFF(_Vec, _Light)	\
	_Vec.v = M_VSelMsk(M_VConstMsk(0,0,0,1), M_VLdScalar(pSurfParams->m_SpecularAnisotrophy), M_VMul(M_VMul(_Light.GetIntensityv().v, m_DiffuseScale.v), M_VLd_V4f16_f32(&pSurfParams->m_DiffuseColor)));

/*	_Vec.k[0]	= _Light.m_Intensity[0] * m_DiffuseScale[0] * _pShaderParams->m_DiffuseColor.k[0];	\
	_Vec.k[1]	= _Light.m_Intensity[1] * m_DiffuseScale[1] * _pShaderParams->m_DiffuseColor.k[1];	\
	_Vec.k[2]	= _Light.m_Intensity[2] * m_DiffuseScale[2] * _pShaderParams->m_DiffuseColor.k[2];	\
	_Vec.k[3]	= _pShaderParams->m_SpecularAnisotrophy;
*/
#define XRSHADER_ATTRIB_SETLIGHTSPEC(_Vec, _Light)	\
	_Vec.v = M_VMul(M_VMul(_Light.GetIntensityv().v, m_CurrentSpecularScale.v), M_VLd_V4f16_f32(&pSurfParams->m_SpecularColor));	\
	if (m_SpecularForcePower)	\
		_Vec.k[3] = m_SpecularForcePower;


/*	_Vec.k[0] = _Light.m_Intensity[0] * m_CurrentSpecularScale[0] * _pShaderParams->m_SpecularColor.k[0];	\
	_Vec.k[1] = _Light.m_Intensity[1] * m_CurrentSpecularScale[1] * _pShaderParams->m_SpecularColor.k[1];	\
	_Vec.k[2] = _Light.m_Intensity[2] * m_CurrentSpecularScale[2] * _pShaderParams->m_SpecularColor.k[2];	\
	_Vec.k[3] = m_CurrentSpecularScale[3] * _pShaderParams->m_SpecularColor.k[3];	\
	if (m_SpecularForcePower)	\
		_Vec.k[3] = m_SpecularForcePower;
*/
#define XRSHADER_ATTRIB_SETLIGHTSPEC_OLD(_Vec, _Light)	\
{	\
	vec128 spec = M_VLd_V4f16_f32(&pSurfParams->m_SpecularColor);	\
	vec128 power = M_VAdd(M_VOne(), M_VMul(M_VSub(M_VSplatW(spec), M_VOne()), M_VHalf()));	\
	_Vec.v = M_VSelMsk(M_VConstMsk(0,0,0,1), power, M_VMul(M_VMul(_Light.GetIntensityv().v, m_CurrentSpecularScale.v), spec)); \
}

/*	_Vec.k[0] = _Light.m_Intensity[0] * m_CurrentSpecularScale[0] * _pShaderParams->m_SpecularColor.k[0];	\
	_Vec.k[1] = _Light.m_Intensity[1] * m_CurrentSpecularScale[1] * _pShaderParams->m_SpecularColor.k[1];	\
	_Vec.k[2] = _Light.m_Intensity[2] * m_CurrentSpecularScale[2] * _pShaderParams->m_SpecularColor.k[2];	\
	{ int Power = (m_SpecularForcePower) ? m_SpecularForcePower : _pShaderParams->m_SpecularPower;	\
		_Vec.k[3] = 1 + ((Power-1) >> 1) ; }
*/

#define XRSHADER_ATTRIB_SETLFMDIFF(_Vec)	\
	_Vec.v = M_VMul(m_DiffuseScale.v, M_VLd_V4f16_f32(&pSurfParams->m_DiffuseColor));

/*	_Vec.k[0]	= m_DiffuseScale[0] * _pShaderParams->m_DiffuseColor.k[0];	\
	_Vec.k[1]	= m_DiffuseScale[1] * _pShaderParams->m_DiffuseColor.k[1];	\
	_Vec.k[2]	= m_DiffuseScale[2] * _pShaderParams->m_DiffuseColor.k[2];	\
	_Vec.k[3]	= 0.0f;*/

#define XRSHADER_ATTRIB_SETLFMSPEC(_Vec)	\
	_Vec.v = M_VMul(m_CurrentSpecularScale.v, M_VLd_V4f16_f32(&pSurfParams->m_SpecularColor));	\
	if (m_SpecularForcePower) _Vec.k[3] = m_SpecularForcePower;

/*	_Vec.k[0] = m_CurrentSpecularScale[0] * _pShaderParams->m_SpecularColor.k[0];	\
	_Vec.k[1] = m_CurrentSpecularScale[1] * _pShaderParams->m_SpecularColor.k[1];	\
	_Vec.k[2] = m_CurrentSpecularScale[2] * _pShaderParams->m_SpecularColor.k[2];	\
	_Vec.k[3] = m_CurrentSpecularScale[3] * _pShaderParams->m_SpecularColor.k[3];	\
	if (m_SpecularForcePower) _Vec.k[3] = m_SpecularForcePower;*/

#if 0
static void CopyGeometry(const CXR_VertexBuffer* _pVBSrc, CXR_VertexBuffer* _pVBDst)
{
	_pVBDst->CopyVBChain(_pVBSrc);
	/*
	while(_pVBSrc && _pVBDst)
	{
		_pVBDst->m_pV = _pVBSrc->m_pV;
		_pVBDst->m_nV = _pVBSrc->m_nV;
		_pVBDst->m_pN = _pVBSrc->m_pN;
		_pVBDst->m_pTV[0] = _pVBSrc->m_pTV[0];
		_pVBDst->m_pTV[1] = _pVBSrc->m_pTV[1];
		_pVBDst->m_pTV[2] = _pVBSrc->m_pTV[2];
		_pVBDst->m_pTV[3] = _pVBSrc->m_pTV[3];
		_pVBDst->m_nTVComp[0] = _pVBSrc->m_nTVComp[0];
		_pVBDst->m_nTVComp[1] = _pVBSrc->m_nTVComp[1];
		_pVBDst->m_nTVComp[2] = _pVBSrc->m_nTVComp[2];
		_pVBDst->m_nTVComp[3] = _pVBSrc->m_nTVComp[3];
		_pVBDst->m_piPrim = _pVBSrc->m_piPrim;
		_pVBDst->m_nPrim = _pVBSrc->m_nPrim;
		_pVBDst->m_PrimType = _pVBSrc->m_PrimType;

		_pVBSrc = _pVBSrc->m_pNextVB;
		_pVBDst = _pVBDst->m_pNextVB;
	}*/
}

static void CopyGeometry_VB(const CXR_VertexBuffer* _pVBSrc, CXR_VertexBuffer* _pVBDst)
{
	_pVBDst->CopyVBChain(_pVBSrc);
/*	while(_pVBSrc && _pVBDst)
	{
		_pVBDst->Render_VertexBuffer(_pVBSrc->m_VBID);
		_pVBDst->m_piPrim = _pVBSrc->m_piPrim;
		_pVBDst->m_nPrim = _pVBSrc->m_nPrim;
		_pVBDst->m_PrimType = _pVBSrc->m_PrimType;

		_pVBSrc = _pVBSrc->m_pNextVB;
		_pVBDst = _pVBDst->m_pNextVB;
	}*/
}

static void VBChain_SetColor(CXR_VertexBuffer* _pVBDst, uint32 _Color)
{
	_pVBDst->m_Color = _Color;
	/*
	for(; _pVBDst; _pVBDst = _pVBDst->m_pNextVB)
		_pVBDst->m_Color = _Color; */
}
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_SurfaceShaderParams
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CXR_SurfaceShaderParams::Create(const CXW_Surface* _pSurf, const CXW_SurfaceKeyFrame* _pSurfKey)
{
	int nLayers = _pSurfKey->m_lTextures.Len();
	const CXW_SurfaceLayer* pLayer = _pSurfKey->m_lTextures.GetBasePtr();

	m_lTextureIDs[0] = 0;
	m_lTextureIDs[1] = 0;
	m_lTextureIDs[2] = 0;
	m_lTextureIDs[3] = 0;
	m_lTextureIDs[4] = 0;
	m_lTextureIDs[5] = 0;
	m_lTextureIDs[6] = 0;
	m_lTextureIDs[7] = 0;
	m_lTextureIDs[8] = 0;

	m_DiffuseColor.SetOne();
	m_SpecularColor.SetOne();
	m_AttribScale.SetOne();
	m_EnvColor.SetOne();
	m_TransmissionColor.SetOne();
	m_SpecularAnisotrophy = 0;
	m_SpecularFresnelBias = 0;
	m_SpecularFresnelScale = 0;
	m_Flags = 0;
	m_AlphaFunc = CRC_COMPARE_ALWAYS;
	m_AlphaRef = 0;
	m_iBRDFModel = 0;
	m_iController = _pSurf->m_iController;
	m_iGroup = _pSurf->m_iGroup;
	m_lDeferredRasterMode[0] = CRC_RASTERMODE_NONE;
	m_lDeferredRasterMode[1] = CRC_RASTERMODE_NONE;
	m_lDeferredRasterMode[2] = CRC_RASTERMODE_NONE;
	m_PolyOffset = _pSurfKey->m_PolygonOffsetUnits;
	m_PolyOffsetScale = _pSurfKey->m_PolygonOffsetScale;

	uint TextureIDOred = 0;

	for(int i = 0; i < nLayers; i++)
	{
		if (pLayer->m_Type)
		{
			uint32 iTxt = pLayer->m_Type-1;
			if (iTxt <= XW_LAYERTYPE_ANISOTROPICDIR)
			{
				if (iTxt == XR_SHADERMAP_DIFFUSE)
				{
					TextureIDOred |= pLayer->m_TextureID;
					m_lTextureIDs[iTxt] = pLayer->m_TextureID;
					m_lDeferredRasterMode[1] = pLayer->m_RasterMode;
					m_DiffuseColor = pLayer->m_HDRColor;
					if (pLayer->m_Flags & XW_LAYERFLAGS_ALPHACOMPARE)
					{
						m_Flags |= XR_SHADERFLAGS_USEZEQUAL;
						m_AlphaFunc = pLayer->m_AlphaFunc;
						m_AlphaRef = pLayer->m_AlphaRef;
					}
				}
				else if (iTxt == XR_SHADERMAP_SPECULAR)
				{
					TextureIDOred |= pLayer->m_TextureID;
					m_lTextureIDs[iTxt] = pLayer->m_TextureID;
					m_lDeferredRasterMode[2] = pLayer->m_RasterMode;
					m_SpecularColor = pLayer->m_HDRColor;
					m_SpecularFresnelBias = pLayer->m_SpecularFresnelBias;
					m_SpecularFresnelScale = pLayer->m_SpecularFresnelScale;
					//					m_SpecularColor.k[3] = pLayer->m_AlphaRef;
				}
				else if (iTxt == XR_SHADERMAP_SPECULAR2)
				{
					TextureIDOred |= pLayer->m_TextureID;
					m_lTextureIDs[XR_SHADERMAP_SPECULAR] = pLayer->m_TextureID;
					m_lDeferredRasterMode[2] = pLayer->m_RasterMode;
					m_SpecularColor = pLayer->m_HDRColor;
					m_SpecularFresnelBias = pLayer->m_SpecularFresnelBias;
					m_SpecularFresnelScale = pLayer->m_SpecularFresnelScale;
					m_iBRDFModel = 1;
				}
				else if (iTxt == XR_SHADERMAP_ENVIRONMENT)
				{
					TextureIDOred |= pLayer->m_TextureID;
					m_lTextureIDs[XR_SHADERMAP_ENVIRONMENT] = pLayer->m_TextureID;
					m_EnvColor = pLayer->m_HDRColor;
				}
				else if (iTxt == XR_SHADERMAP_TRANSMISSION)
				{
					TextureIDOred |= pLayer->m_TextureID;
					m_lTextureIDs[XR_SHADERMAP_TRANSMISSION] = pLayer->m_TextureID;
					m_TransmissionColor = pLayer->m_HDRColor;
					//					m_TransmissionColor.k[3] = fp32(m_TransmissionColor.k[3]) * 255.0f;
				}
				else if (iTxt == XR_SHADERMAP_ANISOTROPICDIR)
				{
					TextureIDOred |= pLayer->m_TextureID;
					m_lTextureIDs[XR_SHADERMAP_ANISOTROPICDIR] = pLayer->m_TextureID;
					vec128 col = M_VLd_V4f16_f32(&pLayer->m_HDRColor);
					M_VStAny32(M_VSplatW(col), &m_SpecularAnisotrophy);
					//					m_SpecularAnisotrophy = pLayer->m_HDRColor.k[3];
				}
				else if (iTxt == XR_SHADERMAP_NORMAL)
				{
					TextureIDOred |= pLayer->m_TextureID;
					m_lTextureIDs[iTxt] = pLayer->m_TextureID;
					m_lDeferredRasterMode[0] = pLayer->m_RasterMode;
					static CTextureContext *pTextureContext = NULL;
					if (!pTextureContext)
					{
						MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
						pTextureContext = pTC;
					}

					//CTC_TextureProperties Properties;
					//pTextureContext->GetTextureProperties(m_lTextureIDs[iTxt], Properties);

					//					if ((Properties.m_Flags & (CTC_TEXTUREFLAGS_PALETTE | CTC_TEXTUREFLAGS_NOCOMPRESS | CTC_TEXTUREFLAGS_HIGHQUALITY)) == (CTC_TEXTUREFLAGS_PALETTE | CTC_TEXTUREFLAGS_NOCOMPRESS | CTC_TEXTUREFLAGS_HIGHQUALITY))
					//						m_Flags |= XR_SHADERFLAGS_SPECULARINDIFFUSE;
				}
				else
				{
					TextureIDOred |= pLayer->m_TextureID;
					m_lTextureIDs[iTxt] = pLayer->m_TextureID;
				}
				/*				else if (iTxt == XR_SHADERMAP_HEIGHT)
				{
				}*/
			}
		}

		pLayer++;
	}

	if (TextureIDOred >= XW_SURFTEX_SPECIALBASE)
		m_Flags |= XR_SHADERFLAGS_SPECIALBASETEXTURE;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_ShaderParams
|__________________________________________________________________________________________________
\*************************************************************************************************/
#if 0
void CXR_ShaderParams::Create(const CXW_SurfaceKeyFrame& _Surf, const CMat4Dfp32* _pCurrentWMat, const CMat4Dfp32* _pCurrentVMat, CXR_Shader* _pShader)
{
	int nLayers = _Surf.m_lTextures.Len();
	const CXW_SurfaceLayer* pLayer = _Surf.m_lTextures.GetBasePtr();

	m_nVB = 1;

	m_lTextureIDs[0] = 0;
	m_lTextureIDs[1] = 0;
	m_lTextureIDs[2] = 0;
	m_lTextureIDs[3] = 0;
	m_lTextureIDs[4] = 0;
	m_lTextureIDs[5] = 0;
	m_lTextureIDs[6] = 0;
	m_lTextureIDs[7] = 0;
	m_lTextureIDs[8] = 0;

	m_DiffuseColor.SetOne();
	m_SpecularColor.SetOne();
	m_AttribScale.SetOne();
	m_EnvColor.SetOne();
	m_TransmissionColor.SetOne();
	m_SpecularAnisotrophy = 0;
	m_SpecularFresnelBias = 0;
	m_SpecularFresnelScale = 0;
	m_Flags = 0;
	m_iBRDFModel = 0;
	m_iTexCoordSetMapping = 0;
	m_iTexCoordSetTangentU = 1;
	m_iTexCoordSetTangentV = 2;

	m_pCurrentWMat = _pCurrentWMat;
	m_pCurrentVMat = _pCurrentVMat;

	uint TextureIDOred = 0;

	for(int i = 0; i < nLayers; i++)
	{
		if (pLayer->m_Type)
		{
			uint32 iTxt = pLayer->m_Type-1;
			if (iTxt <= XW_LAYERTYPE_ANISOTROPICDIR)
			{
				if (iTxt == XR_SHADERMAP_DIFFUSE)
				{
					TextureIDOred |= pLayer->m_TextureID;
					m_lTextureIDs[iTxt] = pLayer->m_TextureID;
					m_DiffuseColor = pLayer->m_HDRColor;
					if (pLayer->m_Flags & XW_LAYERFLAGS_ALPHACOMPARE)
						m_Flags |= XR_SHADERFLAGS_USEZEQUAL;
				}
				else if (iTxt == XR_SHADERMAP_SPECULAR)
				{
					TextureIDOred |= pLayer->m_TextureID;
					m_lTextureIDs[iTxt] = pLayer->m_TextureID;
					m_SpecularColor = pLayer->m_HDRColor;
					m_SpecularFresnelBias = pLayer->m_SpecularFresnelBias;
					m_SpecularFresnelScale = pLayer->m_SpecularFresnelScale;
//					m_SpecularColor.k[3] = pLayer->m_AlphaRef;
				}
				else if (iTxt == XR_SHADERMAP_SPECULAR2)
				{
					TextureIDOred |= pLayer->m_TextureID;
					m_lTextureIDs[XR_SHADERMAP_SPECULAR] = pLayer->m_TextureID;
					m_SpecularColor = pLayer->m_HDRColor;
					m_SpecularFresnelBias = pLayer->m_SpecularFresnelBias;
					m_SpecularFresnelScale = pLayer->m_SpecularFresnelScale;
					m_iBRDFModel = 1;
				}
				else if (iTxt == XR_SHADERMAP_ENVIRONMENT)
				{
					TextureIDOred |= pLayer->m_TextureID;
					m_lTextureIDs[XR_SHADERMAP_ENVIRONMENT] = pLayer->m_TextureID;
					m_EnvColor = pLayer->m_HDRColor;
				}
				else if (iTxt == XR_SHADERMAP_TRANSMISSION)
				{
					TextureIDOred |= pLayer->m_TextureID;
					m_lTextureIDs[XR_SHADERMAP_TRANSMISSION] = pLayer->m_TextureID;
					m_TransmissionColor = pLayer->m_HDRColor;
//					m_TransmissionColor.k[3] = fp32(m_TransmissionColor.k[3]) * 255.0f;
				}
				else if (iTxt == XR_SHADERMAP_ANISOTROPICDIR)
				{
					TextureIDOred |= pLayer->m_TextureID;
					m_lTextureIDs[XR_SHADERMAP_ANISOTROPICDIR] = pLayer->m_TextureID;
					vec128 col = M_VLd_V4f16_f32(&pLayer->m_HDRColor);
					M_VStAny32(M_VSplatW(col), &m_SpecularAnisotrophy);
//					m_SpecularAnisotrophy = pLayer->m_HDRColor.k[3];
				}
				else if (iTxt == XR_SHADERMAP_NORMAL)
				{
					TextureIDOred |= pLayer->m_TextureID;
					m_lTextureIDs[iTxt] = pLayer->m_TextureID;
					static CTextureContext *pTextureContext = NULL;
					if (!pTextureContext)
					{
						MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
						pTextureContext = pTC;
					}

					//CTC_TextureProperties Properties;
					//pTextureContext->GetTextureProperties(m_lTextureIDs[iTxt], Properties);

//					if ((Properties.m_Flags & (CTC_TEXTUREFLAGS_PALETTE | CTC_TEXTUREFLAGS_NOCOMPRESS | CTC_TEXTUREFLAGS_HIGHQUALITY)) == (CTC_TEXTUREFLAGS_PALETTE | CTC_TEXTUREFLAGS_NOCOMPRESS | CTC_TEXTUREFLAGS_HIGHQUALITY))
//						m_Flags |= XR_SHADERFLAGS_SPECULARINDIFFUSE;
				}
				else
				{
					TextureIDOred |= pLayer->m_TextureID;
					m_lTextureIDs[iTxt] = pLayer->m_TextureID;
				}
/*				else if (iTxt == XR_SHADERMAP_HEIGHT)
				{
				}*/
			}
		}

		pLayer++;
	}

	if (TextureIDOred >= XW_SURFTEX_SPECIALBASE)
	{
		for(int iTxt = 0; iTxt < XR_SHADERMAP_MAXMAPS; iTxt++)
		{
			uint TextureID =m_lTextureIDs[iTxt];

			switch(TextureID)
			{
/*			case XW_SURFTEX_REFLECTIONMAP :
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
				break;*/

			case XW_SURFTEX_ENVIRONMENTMAP :
				m_lTextureIDs[iTxt] = _pShader->m_TextureID_DefaultEnvMap;
				break;

			default :;
			}
		}
	}
}
#endif
/*
void CXR_ShaderParamsDef::Create(const CXW_SurfaceKeyFrame& _Surf)
{
	FillChar(this,sizeof(CXR_ShaderParamsDef),0);

	int nLayers = _Surf.m_lTextures.Len();
	const CXW_SurfaceLayer* pLayer = _Surf.m_lTextures.GetBasePtr();

	m_PolyOffset = _Surf.m_PolygonOffsetUnits;
	m_PolyOffsetScale = _Surf.m_PolygonOffsetScale;

	for(int i = 0; i < nLayers; i++)
	{
		switch( pLayer->m_Type - 1 )
		{

		case XR_SHADERMAP_NORMAL:
			m_lTextureIDs[0] = pLayer->m_TextureID;
			m_lRasterMode[0] = pLayer->m_RasterMode;
			break;

		case XR_SHADERMAP_DIFFUSE:
			m_lTextureIDs[1] = pLayer->m_TextureID;
			m_lRasterMode[1] = pLayer->m_RasterMode;
			break;

		case XR_SHADERMAP_SPECULAR:
		case XR_SHADERMAP_SPECULAR2:
			m_lTextureIDs[2] = pLayer->m_TextureID;
			m_lRasterMode[2] = pLayer->m_RasterMode;
			break;
		}

		pLayer++;
	}
}
*/

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VirtualAttributes_Deferred
|__________________________________________________________________________________________________
\*************************************************************************************************/


struct RasterMode
{
	uint8 m_SrcBlend, m_DstBlend;
};
static RasterMode RasterModes[] =
{
	{CRC_BLEND_ONE, CRC_BLEND_ZERO},				//CRC_RASTERMODE_NONE				= 0,		//  D = S,
	{CRC_BLEND_SRCALPHA, CRC_BLEND_INVSRCALPHA},	//CRC_RASTERMODE_ALPHABLEND		= 1,		//  D = D*S.InvAlpha + S*S.Alpha,
	{CRC_BLEND_ZERO, CRC_BLEND_INVSRCCOLOR},		//CRC_RASTERMODE_LIGHTMAPBLEND	= 2,		//  D = D*S, or  D*S.InvAlpha + S*SrcAlpha,
	{CRC_BLEND_DESTCOLOR, CRC_BLEND_ZERO},			//CRC_RASTERMODE_MULTIPLY			= 3,		//  D = D*S,
	{CRC_BLEND_ONE, CRC_BLEND_ONE},					//CRC_RASTERMODE_ADD				= 4,		//  D = D + S,
	{CRC_BLEND_SRCALPHA, CRC_BLEND_ONE},			//CRC_RASTERMODE_ALPHAADD			= 5,		//  D = D + S*S.Alpha,
	{CRC_BLEND_DESTCOLOR, CRC_BLEND_INVSRCALPHA},	//CRC_RASTERMODE_ALPHAMULTIPLY	= 6,		//  D = D*S.InvAlpha + S*D,
	{CRC_BLEND_DESTCOLOR, CRC_BLEND_SRCCOLOR},		//CRC_RASTERMODE_MULTIPLY2		= 7,		//  D = D*S + S*D,
	{CRC_BLEND_DESTCOLOR, CRC_BLEND_ONE},			//CRC_RASTERMODE_MULADD			= 8,		//  D = D + S*D,
	{CRC_BLEND_ONE, CRC_BLEND_SRCALPHA},			//CRC_RASTERMODE_ONE_ALPHA		= 9,		//  D = S + D*S.Alpha,
	{CRC_BLEND_ONE, CRC_BLEND_INVSRCALPHA},			//CRC_RASTERMODE_ONE_INVALPHA		= 10,		//  D = S + D*S.InvAlpha,
	{CRC_BLEND_DESTALPHA, CRC_BLEND_INVDESTALPHA},	//CRC_RASTERMODE_DESTALPHABLEND	= 11,		//  D = D*D.InvAlpha + S*D.Alpha,
	{CRC_BLEND_DESTCOLOR, CRC_BLEND_SRCALPHA},		//CRC_RASTERMODE_DESTADD			= 12,		//  D = D + D*S.Alpha
	{CRC_BLEND_DESTALPHA, CRC_BLEND_ONE},			//CRC_RASTERMODE_MULADD_DESTALPHA = 13,		//  D = D + S*D.Alpha
};
class CXR_VirtualAttributes_Deferred : public CXR_VirtualAttributes
{

public:

	uint16 m_TextureID;
	int m_Enable,m_Disable;
	uint8 m_ZComp;
	uint8 m_RasterMode;

	fp32 *m_pTexGenAttr;
	union
	{
		uint8 m_TexGen[4];
		uint32 m_TexGenAll;
	};

	fp32 m_PolygonOffset,m_PolygonOffsetScale;

	CRC_ExtAttributes * m_pExtAttrib;

	void Create()
	{
		m_Class = 0x30; 

		m_pExtAttrib = NULL;

		m_pTexGenAttr = 0;
		m_TexGenAll = 0;

		m_PolygonOffset = 0.0f;
	}

	int OnCompare(const CXR_VirtualAttributes * _pOther)
	{
		const CXR_VirtualAttributes_Deferred * pLast = (const CXR_VirtualAttributes_Deferred*)_pOther;
		XR_COMPAREATTRIBUTE_INT(m_TextureID,pLast->m_TextureID);
		XR_COMPAREATTRIBUTE_INT(m_pTexGenAttr,pLast->m_pTexGenAttr);
		XR_COMPAREATTRIBUTE_INT(m_Enable,pLast->m_Enable);
		XR_COMPAREATTRIBUTE_INT(m_Disable,pLast->m_Disable);
		XR_COMPAREATTRIBUTE_INT(m_ZComp,pLast->m_ZComp);
		XR_COMPAREATTRIBUTE_INT(m_RasterMode,pLast->m_RasterMode);
		XR_COMPAREATTRIBUTE_INT(m_TexGenAll,pLast->m_TexGenAll);
		XR_COMPAREATTRIBUTE_INT(m_pExtAttrib,pLast->m_pExtAttrib);
		if( !AlmostEqual(m_PolygonOffset,pLast->m_PolygonOffset,0.001f) )
		{
			if( m_PolygonOffset < pLast->m_PolygonOffset ) return -1;
			return 1;
		}
		return 0;
	}

	uint OnSetAttributes(CRC_Attributes* _pDest, const CXR_VirtualAttributes* _pLastAttr)
	{
		int AttrChg = 0;

		if (!_pLastAttr || _pLastAttr->m_Class != m_Class)
		{
			AttrChg = -1;
			_pDest->SetDefault();
			_pDest->Attrib_TextureID(0, m_TextureID);
			_pDest->Attrib_Disable(m_Disable);
			_pDest->Attrib_Enable(m_Enable);
			_pDest->Attrib_ZCompare(m_ZComp);
			_pDest->m_pExtAttrib = m_pExtAttrib;
			_pDest->Attrib_RasterMode(m_RasterMode);
			_pDest->Attrib_TexGenAttr(m_pTexGenAttr);
			
			_pDest->Attrib_PolygonOffset(0.0f,m_PolygonOffset);

			_pDest->Attrib_TexGen(0, m_TexGen[0],CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);
			for( int i = 1; i < 4; i++ )
			{
				_pDest->Attrib_TexGen(i,m_TexGen[i],CRC_TEXGENCOMP_ALL);
			}
			if( m_TexGen[2] == CRC_TEXGENMODE_DECALTSTRANSFORM )
			{
				_pDest->Attrib_TexCoordSet(2,1);
				_pDest->Attrib_TexCoordSet(3,2);
			}
			
		}
		else
		{
			const CXR_VirtualAttributes_Deferred * pLast = (const CXR_VirtualAttributes_Deferred*)_pLastAttr;

			if( m_TextureID != pLast->m_TextureID )
			{
				_pDest->Attrib_TextureID(0, m_TextureID);
				AttrChg |= CRC_ATTRCHG_TEXTUREID;
			}

			if( m_pExtAttrib != pLast->m_pExtAttrib )
			{
				_pDest->m_pExtAttrib = m_pExtAttrib;
				AttrChg |= CRC_ATTRCHG_EXATTR;
			}

			if( m_ZComp != pLast->m_ZComp )
			{
				_pDest->Attrib_ZCompare(m_ZComp);
				AttrChg |= CRC_ATTRCHG_ZCOMPARE;
			}

			if( (m_Enable != pLast->m_Enable) || (m_Disable != pLast->m_Disable) )
			{
				_pDest->Attrib_Default();
				_pDest->Attrib_Enable(m_Enable);
				_pDest->Attrib_Disable(m_Disable);
				AttrChg |= CRC_ATTRCHG_FLAGS;
			}

			if( (m_RasterMode != pLast->m_RasterMode) 
				/*|| (!(pLast->m_Enable & CRC_FLAGS_BLEND) && (m_Enable & CRC_FLAGS_BLEND))*/ )
			{
				_pDest->Attrib_RasterMode(m_RasterMode);
				AttrChg |= CRC_ATTRCHG_BLEND;
			}

			if( m_pTexGenAttr != pLast->m_pTexGenAttr )
			{
				_pDest->Attrib_TexGenAttr(m_pTexGenAttr);
				AttrChg |= CRC_ATTRCHG_TEXGENATTR;
			}

			if( m_TexGenAll != pLast->m_TexGenAll )
			{
				_pDest->Attrib_TexGen(0, m_TexGen[0],CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);
				for( int i = 1; i < 4; i++ )
				{
					_pDest->Attrib_TexGen(i,m_TexGen[i],CRC_TEXGENCOMP_ALL);
				}

				//No need to handle alternatives since they don't use the channels
				if( (m_TexGen[2] == CRC_TEXGENMODE_DECALTSTRANSFORM) &&
					(pLast->m_TexGen[2] != CRC_TEXGENMODE_DECALTSTRANSFORM) )
				{
					_pDest->Attrib_TexCoordSet(2,1);
					_pDest->Attrib_TexCoordSet(3,2);
					AttrChg |= CRC_ATTRCHG_TEXCOORDSET;
				}
				else if( (m_TexGen[2] != CRC_TEXGENMODE_DECALTSTRANSFORM) &&
					(pLast->m_TexGen[2] == CRC_TEXGENMODE_DECALTSTRANSFORM) )
				{
					_pDest->Attrib_TexCoordSet(1,1);
					_pDest->Attrib_TexCoordSet(2,2);
					AttrChg |= CRC_ATTRCHG_TEXCOORDSET;
				}

				AttrChg |= CRC_ATTRCHG_TEXGEN;
			}

			if( !AlmostEqual(m_PolygonOffset,pLast->m_PolygonOffset,0.001f) )
			{
				if( m_Enable & CRC_FLAGS_POLYGONOFFSET )
				{
					_pDest->Attrib_PolygonOffset(m_PolygonOffsetScale,m_PolygonOffset);
				}
				else
				{
					_pDest->Attrib_PolygonOffset(0.0f,0.0f);
				}
				AttrChg |= CRC_ATTRCHG_POLYGONOFFSET;
			}
		}

		return AttrChg;
	}
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VirtualAttributes_MotionMap
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_VirtualAttributes_MotionMap : public CXR_VirtualAttributes
{

public:
	int m_Enable,m_Disable;
	uint8 m_ZComp;
	uint8 m_RasterMode;

	fp32 *m_pTexGenAttr;
	uint8 m_TexGen[4];

	fp32 m_PolygonOffset,m_PolygonOffsetScale;

	CRC_ExtAttributes * m_pExtAttrib;

	void Create()
	{
		m_Class = 0x32; 

		m_pExtAttrib = NULL;

		m_pTexGenAttr = 0;
		*(uint32*)m_TexGen = 0;

		m_PolygonOffset = 0.0f;

		m_TexGen[0] = CRC_TEXGENMODE_LINEAR;
		m_TexGen[1] = CRC_TEXGENMODE_LINEAR;
		m_TexGen[2] = CRC_TEXGENMODE_VOID;
		m_TexGen[3] = CRC_TEXGENMODE_VOID;
	}

	int OnCompare(const CXR_VirtualAttributes * _pOther)
	{
		const CXR_VirtualAttributes_MotionMap * pLast = (const CXR_VirtualAttributes_MotionMap*)_pOther;
		XR_COMPAREATTRIBUTE_INT(m_pTexGenAttr,pLast->m_pTexGenAttr);
		XR_COMPAREATTRIBUTE_INT(m_Enable,pLast->m_Enable);
		XR_COMPAREATTRIBUTE_INT(m_Disable,pLast->m_Disable);
		XR_COMPAREATTRIBUTE_INT(m_ZComp,pLast->m_ZComp);
		XR_COMPAREATTRIBUTE_INT(m_RasterMode,pLast->m_RasterMode);
		XR_COMPAREATTRIBUTE_INT(*((uint32*)m_TexGen),*((uint32*)pLast->m_TexGen));
		XR_COMPAREATTRIBUTE_INT(m_pExtAttrib,pLast->m_pExtAttrib);
		if( !AlmostEqual(m_PolygonOffset,pLast->m_PolygonOffset,0.001f) )
		{
			if( m_PolygonOffset < pLast->m_PolygonOffset ) return -1;
			return 1;
		}
		return 0;
	}

	uint OnSetAttributes(CRC_Attributes* _pDest, const CXR_VirtualAttributes* _pLastAttr)
	{
		int AttrChg = 0;

		if (!_pLastAttr || _pLastAttr->m_Class != m_Class)
		{
			AttrChg = -1;
			_pDest->SetDefault();
			_pDest->Attrib_Disable(m_Disable);
			_pDest->Attrib_Enable(m_Enable);
			_pDest->Attrib_ZCompare(m_ZComp);
			_pDest->m_pExtAttrib = m_pExtAttrib;
			_pDest->Attrib_RasterMode(m_RasterMode);
			_pDest->Attrib_TexGenAttr(m_pTexGenAttr);
			_pDest->Attrib_PolygonOffset(0.0f,m_PolygonOffset);

			for( int i = 0; i < 4; i++ )
			{
				_pDest->Attrib_TexGen(i, m_TexGen[i], CRC_TEXGENCOMP_ALL);
			}

		}
		else
		{
			const CXR_VirtualAttributes_MotionMap * pLast = (const CXR_VirtualAttributes_MotionMap*)_pLastAttr;

			if( m_pExtAttrib != pLast->m_pExtAttrib )
			{
				_pDest->m_pExtAttrib = m_pExtAttrib;
				AttrChg |= CRC_ATTRCHG_EXATTR;
			}

			if( m_ZComp != pLast->m_ZComp )
			{
				_pDest->Attrib_ZCompare(m_ZComp);
				AttrChg |= CRC_ATTRCHG_ZCOMPARE;
			}

			if( (m_Enable != pLast->m_Enable) || (m_Disable != pLast->m_Disable) )
			{
				_pDest->Attrib_Default();
				_pDest->Attrib_Enable(m_Enable);
				_pDest->Attrib_Disable(m_Disable);
				AttrChg |= CRC_ATTRCHG_FLAGS;
			}

			if( (m_RasterMode != pLast->m_RasterMode) 
				/*|| (!(pLast->m_Enable & CRC_FLAGS_BLEND) && (m_Enable & CRC_FLAGS_BLEND))*/ )
			{
				_pDest->Attrib_RasterMode(m_RasterMode);
				AttrChg |= CRC_ATTRCHG_BLEND;
			}

			if( m_pTexGenAttr != pLast->m_pTexGenAttr )
			{
				_pDest->Attrib_TexGenAttr(m_pTexGenAttr);
				AttrChg |= CRC_ATTRCHG_TEXGENATTR;
			}

			if( *((uint32*)m_TexGen) != *((uint32*)pLast->m_TexGen) )
			{
				for( int i = 0; i < 4; i++ )
				{
					_pDest->Attrib_TexGen(i,m_TexGen[i],CRC_TEXGENCOMP_ALL);
				}

				AttrChg |= CRC_ATTRCHG_TEXGEN;
			}

			if( !AlmostEqual(m_PolygonOffset,pLast->m_PolygonOffset,0.001f) )
			{
				if( m_Enable & CRC_FLAGS_POLYGONOFFSET )
				{
					_pDest->Attrib_PolygonOffset(m_PolygonOffsetScale,m_PolygonOffset);
				}
				else
				{
					_pDest->Attrib_PolygonOffset(0.0f,0.0f);
				}
				AttrChg |= CRC_ATTRCHG_POLYGONOFFSET;
			}
		}

		return AttrChg;
	}
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VirtualAttributes_DeferredMRT
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CXR_VirtualAttributes_DeferredMRT : public CXR_VirtualAttributes
{
public:
	uint16 m_TextureID_Normal;
	uint16 m_TextureID_Diffuse;
	uint16 m_TextureID_Specular;
	int m_Enable,m_Disable;
	uint8 m_ZComp;
	uint8 m_RasterMode;

	fp32 *m_pTexGenAttr;
	union
	{
		uint8 m_TexGen[4];
		uint32 m_TexGenAll;
	};

	fp32 m_PolygonOffset,m_PolygonOffsetScale;

	CRC_ExtAttributes * m_pExtAttrib;

	void Create()
	{
		m_Class = 0x31; 

		m_pExtAttrib = NULL;

		m_pTexGenAttr = 0;
		m_TexGenAll = 0;

		m_PolygonOffset = 0.0f;
	}

	int OnCompare(const CXR_VirtualAttributes * _pOther)
	{
		const CXR_VirtualAttributes_DeferredMRT * pLast = (const CXR_VirtualAttributes_DeferredMRT*)_pOther;
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Normal,pLast->m_TextureID_Normal);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Diffuse,pLast->m_TextureID_Diffuse);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Specular,pLast->m_TextureID_Specular);
		XR_COMPAREATTRIBUTE_INT(m_pTexGenAttr,pLast->m_pTexGenAttr);
		XR_COMPAREATTRIBUTE_INT(m_Enable,pLast->m_Enable);
		XR_COMPAREATTRIBUTE_INT(m_Disable,pLast->m_Disable);
		XR_COMPAREATTRIBUTE_INT(m_ZComp,pLast->m_ZComp);
		XR_COMPAREATTRIBUTE_INT(m_RasterMode,pLast->m_RasterMode);
		XR_COMPAREATTRIBUTE_INT(m_TexGenAll,pLast->m_TexGenAll);
		XR_COMPAREATTRIBUTE_INT(m_pExtAttrib,pLast->m_pExtAttrib);
		if( !AlmostEqual(m_PolygonOffset,pLast->m_PolygonOffset,0.001f) )
		{
			if( m_PolygonOffset < pLast->m_PolygonOffset ) return -1;
			return 1;
		}
		return 0;
	}

	uint OnSetAttributes(CRC_Attributes* _pDest, const CXR_VirtualAttributes* _pLastAttr)
	{
		int AttrChg = 0;

		if (!_pLastAttr || _pLastAttr->m_Class != m_Class)
		{
			AttrChg = -1;
			_pDest->SetDefault();
			_pDest->Attrib_TextureID(0, m_TextureID_Normal);
			_pDest->Attrib_TextureID(1, m_TextureID_Diffuse);
			_pDest->Attrib_TextureID(2, m_TextureID_Specular);
			_pDest->Attrib_Disable(m_Disable);
			_pDest->Attrib_Enable(m_Enable);
			_pDest->Attrib_ZCompare(m_ZComp);
			_pDest->m_pExtAttrib = m_pExtAttrib;
			_pDest->Attrib_RasterMode(m_RasterMode);
			_pDest->Attrib_TexGenAttr(m_pTexGenAttr);

			_pDest->Attrib_PolygonOffset(0.0f,m_PolygonOffset);

			_pDest->Attrib_TexGen(0, m_TexGen[0],CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);
			for( int i = 1; i < 4; i++ )
			{
				_pDest->Attrib_TexGen(i,m_TexGen[i],CRC_TEXGENCOMP_ALL);
			}
			if( m_TexGen[2] == CRC_TEXGENMODE_DECALTSTRANSFORM )
			{
				_pDest->Attrib_TexCoordSet(2,1);
				_pDest->Attrib_TexCoordSet(3,2);
			}

		}
		else
		{
			const CXR_VirtualAttributes_DeferredMRT * pLast = (const CXR_VirtualAttributes_DeferredMRT*)_pLastAttr;

			if( (m_TextureID_Normal - pLast->m_TextureID_Normal) |
				(m_TextureID_Diffuse - pLast->m_TextureID_Diffuse) |
				(m_TextureID_Specular - pLast->m_TextureID_Specular))
			{
				_pDest->Attrib_TextureID(0, m_TextureID_Normal);
				_pDest->Attrib_TextureID(1, m_TextureID_Diffuse);
				_pDest->Attrib_TextureID(2, m_TextureID_Specular);
				AttrChg |= CRC_ATTRCHG_TEXTUREID;
			}

			if( m_pExtAttrib != pLast->m_pExtAttrib )
			{
				_pDest->m_pExtAttrib = m_pExtAttrib;
				AttrChg |= CRC_ATTRCHG_EXATTR;
			}

			if( m_ZComp != pLast->m_ZComp )
			{
				_pDest->Attrib_ZCompare(m_ZComp);
				AttrChg |= CRC_ATTRCHG_ZCOMPARE;
			}

			if( (m_Enable != pLast->m_Enable) || (m_Disable != pLast->m_Disable) )
			{
				_pDest->Attrib_Default();
				_pDest->Attrib_Enable(m_Enable);
				_pDest->Attrib_Disable(m_Disable);
				AttrChg |= CRC_ATTRCHG_FLAGS;
			}

			if( m_RasterMode != pLast->m_RasterMode )
			{
				_pDest->Attrib_RasterMode(m_RasterMode);
				AttrChg |= CRC_ATTRCHG_BLEND;
			}

			if( m_pTexGenAttr != pLast->m_pTexGenAttr )
			{
				_pDest->Attrib_TexGenAttr(m_pTexGenAttr);
				AttrChg |= CRC_ATTRCHG_TEXGENATTR;
			}

			if( m_TexGenAll != pLast->m_TexGenAll )
			{
				_pDest->Attrib_TexGen(0, m_TexGen[0],CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);
				for( int i = 1; i < 4; i++ )
				{
					_pDest->Attrib_TexGen(i,m_TexGen[i],CRC_TEXGENCOMP_ALL);
				}

				//No need to handle alternatives since they don't use the channels
				if( (m_TexGen[2] == CRC_TEXGENMODE_DECALTSTRANSFORM) &&
					(pLast->m_TexGen[2] != CRC_TEXGENMODE_DECALTSTRANSFORM) )
				{
					_pDest->Attrib_TexCoordSet(2,1);
					_pDest->Attrib_TexCoordSet(3,2);
					AttrChg |= CRC_ATTRCHG_TEXCOORDSET;
				}
				else if( (m_TexGen[2] != CRC_TEXGENMODE_DECALTSTRANSFORM) &&
					(pLast->m_TexGen[2] == CRC_TEXGENMODE_DECALTSTRANSFORM) )
				{
					_pDest->Attrib_TexCoordSet(1,1);
					_pDest->Attrib_TexCoordSet(2,2);
					AttrChg |= CRC_ATTRCHG_TEXCOORDSET;
				}

				AttrChg |= CRC_ATTRCHG_TEXGEN;
			}

			if( !AlmostEqual(m_PolygonOffset,pLast->m_PolygonOffset,0.001f) )
			{
				if( m_Enable & CRC_FLAGS_POLYGONOFFSET )
				{
					_pDest->Attrib_PolygonOffset(m_PolygonOffsetScale,m_PolygonOffset);
				}
				else
				{
					_pDest->Attrib_PolygonOffset(0.0f,0.0f);
				}
				AttrChg |= CRC_ATTRCHG_POLYGONOFFSET;
			}
		}

		return AttrChg;
	}
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Shader
|__________________________________________________________________________________________________
\*************************************************************************************************/

//#include "XRShader_TexEnvCombine.cpp"
//#include "XRShader_NV20.cpp"

#ifdef SUPPORT_FRAGMENTPROGRAM

//#include "XRShader_FP14.cpp"
#include "XRShader_FP20.cpp"
#include "XRShader_LightField.cpp"
#include "XRShader_FP20Def.cpp"
#include "XRShader_FP20Def_LightField.cpp"
#include "XRShader_Def.cpp"

#endif


CXR_Shader::CXR_Shader()
{
	MAUTOSTRIP(CXR_Shader_ctor, MAUTOSTRIP_VOID);
	m_pEngine = NULL;
	m_pVBM = NULL;
	m_pSys = NULL;

#	ifdef SUPPORT_REGISTERCOMBINERS
//		m_lpShaders = NULL;
#	endif
//	m_nLightShaders = 0;
//	m_MaxShaders = 0;
	m_TextureID_Normalize = 0;
	m_TextureID_FlipNormalize = 0;
	m_TextureID_AttenuationExp = 0;
	m_TextureID_AttenuationDst2 = 0;
	m_TextureID_AttenuationDst2Alpha = 0;
	m_TextureID_DefaultLens = 0;
	m_TextureID_SbzProjTemp = 0;
	FillChar(&m_lTextureID_Specular, sizeof(m_lTextureID_Specular), 0);
	m_TextureID_SpecularPower = 0;
	m_TextureID_Special_Cube_ffffffff = 0;
	m_TextureID_Special_Cube_ff000000 = 0;
	m_TextureID_Special_ffffffff = 0;
	m_TextureID_Special_ff000000 = 0;
	m_TextureID_Rand_02 = 0;
	m_TextureID_Rand_03 = 0;
	m_TextureID_Rand_04 = 0;
	m_TextureID_DefaultEnvMap = 0;

	m_ShaderMode = 0;
	m_PreferredShaderMode = 0;
	m_LastPreferredShaderMode = 0;
	m_LastShaderMode = ~0;
	m_LastModesAvail = ~0;
	m_bIsRegistered = 0;
	m_ShaderFlags = 0;
	m_SpecularMode = 0;
	m_SpecularForcePower = 0;
	m_bCanGetBackbuffer = 0;
	m_bDisplayNextModeSelection = 0;
	m_bUseVirtualAttr = 1;
	m_ShaderModeTraits = 0;
	m_iDeferredShaderOffset = 0;

	m_DiffuseScale = 4.0f;		// Why? Long story...    -mh
	m_SpecularScale = 2.0f;
	m_CurrentSpecularScale = 2.0f;
	m_FallOffExp_Linear = 4.0f;
	m_FallOffExp_Gamma = 2.0f;

	m_Caps_Flags = 0;
	m_Caps_nMultiTexture = 0;
	m_Caps_nMultiTextureCoords = 0;

#ifdef SUPPORT_FRAGMENTPROGRAM
	{
		int iPrg = 0;
		while(ms_lProgramsShading20[iPrg].m_pName)
		{
			ms_lProgramsShading20[iPrg].m_Hash = StringToHash(ms_lProgramsShading20[iPrg].m_pName);
			iPrg++;
		}
	}
#endif

}

CXR_Shader::~CXR_Shader()
{
	if (m_bIsRegistered)
	{
		m_bIsRegistered = false;
		RemoveFromConsole();
	}
}

void CXR_Shader::Create(CTextureContext *_pTC, CXR_VBManager *_pVBM)
{
	m_pEngine = NULL;
	m_pVBM = _pVBM;
	AssignTextures(_pTC);

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	m_pSys = pSys;
}

void CXR_Shader::Create(CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CXR_Shader_Create, MAUTOSTRIP_VOID);

	CTextureContext* pTC = _pEngine->GetTC();
	if (!pTC)
		Error("Create", "No TC.");

	m_pEngine = _pEngine;
	m_pVBM = NULL;

	AssignTextures(pTC);

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	m_pSys = pSys;
}

void CXR_Shader::AssignTextures(CTextureContext *_pTC)
{
	m_TextureID_Normalize = _pTC->GetTextureID("SPECIAL_CUBENORMALIZE0");
	m_TextureID_FlipNormalize = _pTC->GetTextureID("SPECIAL_NORMALIZE0");
	m_TextureID_AttenuationExp = _pTC->GetTextureID("SPECIAL_ATTENUATION3");
	m_TextureID_AttenuationDst2 = _pTC->GetTextureID("SPECIAL_ATTENUATION4");
	m_TextureID_AttenuationDst2Alpha =  _pTC->GetTextureID("SPECIAL_ATTENUATION4A");
	m_TextureID_DefaultLens = _pTC->GetTextureID("SPECIAL_DEFAULTLENS");
	m_TextureID_DefaultNormal = _pTC->GetTextureID("SPECIAL_FF8080");
	m_TextureID_SpecularPower = _pTC->GetTextureID("SPECIAL_SPECPOWER");

	m_TextureID_Special_Cube_ffffffff = _pTC->GetTextureID("SPECIAL_CUBE_FFFFFFFF");
	m_TextureID_Special_Cube_ff000000 = _pTC->GetTextureID("SPECIAL_CUBE_FF000000");
	m_TextureID_Special_ffffffff = _pTC->GetTextureID("SPECIAL_FFFFFF");
	m_TextureID_Special_ff000000 = _pTC->GetTextureID("SPECIAL_000000");
	m_TextureID_Rand_02 = _pTC->GetTextureID("SPECIAL_RAND_02");
	m_TextureID_Rand_03 = _pTC->GetTextureID("SPECIAL_RAND_03");
	m_TextureID_Rand_04 = _pTC->GetTextureID("SPECIAL_RAND_04");

//	m_TextureID_SbzProjTemp = pTC->GetTextureID("Special_SbzProj256");

	m_lTextureID_Specular[0] = _pTC->GetTextureID("SPECIAL_SPECULAR001_0");
	m_lTextureID_Specular[1] = _pTC->GetTextureID("SPECIAL_SPECULAR002_0");
	m_lTextureID_Specular[2] = _pTC->GetTextureID("SPECIAL_SPECULAR004_0");
	m_lTextureID_Specular[3] = _pTC->GetTextureID("SPECIAL_SPECULAR008_0");
	m_lTextureID_Specular[4] = _pTC->GetTextureID("SPECIAL_SPECULAR016_0");
	m_lTextureID_Specular[5] = _pTC->GetTextureID("SPECIAL_SPECULAR032_0");
	m_lTextureID_Specular[6] = _pTC->GetTextureID("SPECIAL_SPECULAR064_0");
	m_lTextureID_Specular[7] = _pTC->GetTextureID("SPECIAL_SPECULAR128_0");
}

int CXR_Shader::ms_TextureID_BackBuffer = 0;

void CXR_Shader::Options_UpdateAvailShaderModes(int _ModesAvail)
{
	if (!m_pSys)
		return;

	m_LastModesAvail = _ModesAvail;

	CRegistry* pReg = m_pSys->GetRegistry()->CreateDir("XR");
	pReg->DeleteDir("XR_SHADERMODELIST");
	pReg = pReg->CreateDir("XR_SHADERMODELIST");

	pReg->AddKeyi("§LSYS_SHADERMODE0", XR_SHADERMODE_AUTO);
	for(int i = 1; i < 8; i++)
		if (_ModesAvail & (1 << i))
			pReg->AddKeyi(CStrF("§LSYS_SHADERMODE%d", i), i);
}

void CXR_Shader::Options_UpdateCurrentShaderModes(int _ShaderMode)
{
	if (!m_pSys)
		return;

	m_LastShaderMode = _ShaderMode;

	CRegistry* pReg = m_pSys->GetRegistry()->CreateDir("XR");

	pReg->SetValuei("XR_SHADERMODECURRENT", _ShaderMode);

	if (m_PreferredShaderMode == XR_SHADERMODE_AUTO)
		pReg->SetValue("XR_SHADERMODE", CStrF("§LSYS_SHADERMODE0 (§LSYS_SHADERMODE%d)", m_ShaderMode));
	else
		pReg->SetValue("XR_SHADERMODE", CStrF("§LSYS_SHADERMODE%d", m_ShaderMode));

	if (m_PreferredShaderMode == XR_SHADERMODE_AUTO)
		pReg->SetValue("XR_SHADERMODEDESC", CStrF("§LSYS_SHADERMODEDESCAUTO", m_ShaderMode));
	else
		pReg->SetValue("XR_SHADERMODEDESC", CStrF("§LSYS_SHADERMODEDESC%d", m_ShaderMode));
}

bool CXR_Shader::PrepareFrame(CRenderContext *_pRC, CXR_VBManager* _pVBM)
{
	m_pVBM = _pVBM;

#ifdef SUPPORT_REGISTERCOMBINERS
//	m_lpShaders = NULL;
#endif
//	m_nLightShaders = 0;
//	m_MaxShaders = 0;
//	m_nShaderSaved = 0;

	ms_TextureID_BackBuffer = _pRC->Texture_GetBackBufferTextureID();
	m_TextureID_DefaultEnvMap = (m_pEngine) ? m_pEngine->m_TextureID_DefaultEnvMap : 0;
	m_Caps_Flags = _pRC->Caps_Flags();
	m_Caps_nMultiTexture = _pRC->Attrib_GlobalGetVar(CRC_GLOBALVAR_NUMTEXTURES);
	m_Caps_nMultiTextureCoords = _pRC->Attrib_GlobalGetVar(CRC_GLOBALVAR_NUMTEXTURECOORDS);

	m_bCanGetBackbuffer = (m_Caps_Flags & CRC_CAPS_FLAGS_BACKBUFFERASTEXTURE) != 0;

	int ModesAvail = 0;
//	int ModesAvail = 1 << XR_SHADERMODE_TEXENVCOMBINE;		// Assume we checked minimum features for this before app started.

//	if (m_Caps_Flags & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20)
//		ModesAvail |= 1 << XR_SHADERMODE_REGCOMBINERS_NV20;

#ifdef SUPPORT_FRAGMENTPROGRAM
//	if (m_Caps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM30)
//		ModesAvail |= 1 << XR_SHADERMODE_FRAGMENTPROGRAM30;	

	if (m_Caps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM20 && (m_Caps_nMultiTexture >= 8) && (m_Caps_nMultiTextureCoords >= 8))
	{
		ModesAvail |= (1 << XR_SHADERMODE_FRAGMENTPROGRAM20) | (1 << XR_SHADERMODE_FRAGMENTPROGRAM20DEF) | (1 << XR_SHADERMODE_FRAGMENTPROGRAM20DEFLIN);
		if (m_Caps_Flags & CRC_CAPS_FLAGS_MRT)
			ModesAvail |= (1 << XR_SHADERMODE_FRAGMENTPROGRAM20DEFMRT);
	}

	if ((m_Caps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM30) && 
		(m_Caps_Flags & CRC_CAPS_FLAGS_COPYDEPTH) && 
		(ModesAvail & (1 << XR_SHADERMODE_FRAGMENTPROGRAM20)))
		ModesAvail |= 1 << XR_SHADERMODE_FRAGMENTPROGRAM20SS;

	if (m_Caps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM14 && (m_Caps_nMultiTexture >= 4) && (m_Caps_nMultiTextureCoords >= 4))
		ModesAvail |= 1 << XR_SHADERMODE_FRAGMENTPROGRAM14;

	if (ModesAvail & M_Bit(XR_SHADERMODE_FRAGMENTPROGRAM20DEF))
		ModesAvail |= M_Bit(XR_SHADERMODE_FRAGMENTPROGRAM20DEFMM);

//	if (m_Caps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM11)
//		ModesAvail |= 1 << XR_SHADERMODE_FRAGMENTPROGRAM11;
#endif // SUPPORT_FRAGMENTPROGRAM

	m_PreferredShaderMode = (m_pSys) ? m_pSys->GetEnvironment()->GetValuei("XR_SHADERMODE", XR_SHADERMODE_AUTO) : XR_SHADERMODE_AUTO;

	m_ShaderMode = XR_SHADERMODE_TEXENVCOMBINE;
	switch(m_PreferredShaderMode)
	{
	case XR_SHADERMODE_AUTO :
		{
#ifdef PLATFORM_XENON
			m_ShaderMode = BitScanBwd32(ModesAvail & ~(M_Bit(XR_SHADERMODE_FRAGMENTPROGRAM20SS) | M_Bit(XR_SHADERMODE_FRAGMENTPROGRAM20DEFLIN)));
#else
			m_ShaderMode = BitScanBwd32(ModesAvail & ~(M_Bit(XR_SHADERMODE_FRAGMENTPROGRAM20SS) | M_Bit(XR_SHADERMODE_FRAGMENTPROGRAM20DEF) | M_Bit(XR_SHADERMODE_FRAGMENTPROGRAM20DEFLIN)));
#endif
		}
		break;

	default :
		{
			if (ModesAvail & (1 << m_PreferredShaderMode))
				m_ShaderMode = m_PreferredShaderMode;
			else
				m_ShaderMode = BitScanBwd32(ModesAvail);
		}
	}

	if (m_ShaderMode + 1 == 1 << 7)
		m_ShaderMode = XR_SHADERMODE_TEXENVCOMBINE;

	if (m_LastModesAvail != ModesAvail)
		Options_UpdateAvailShaderModes(ModesAvail);
	if (m_LastShaderMode != m_ShaderMode || m_PreferredShaderMode != m_LastPreferredShaderMode)
	{
		m_LastPreferredShaderMode = m_PreferredShaderMode;
		Options_UpdateCurrentShaderModes(m_ShaderMode);
	}

	// -------------------------------------------------------------------
	// Init shader mode traits and settings
	m_ShaderModeTraits = 0;
	m_iDeferredShaderOffset = 0;
	m_CurrentSpecularScale = m_SpecularScale;

	switch(m_ShaderMode)
	{
	case XR_SHADERMODE_FRAGMENTPROGRAM20DEF :
		m_ShaderModeTraits |= XR_SHADERMODETRAIT_DEFERRED;
		break;
	case XR_SHADERMODE_FRAGMENTPROGRAM20DEFMRT :
		m_ShaderModeTraits |= XR_SHADERMODETRAIT_DEFERRED | XR_SHADERMODETRAIT_MRT;
		break;
	case XR_SHADERMODE_FRAGMENTPROGRAM20DEFMM :
		m_ShaderModeTraits |= XR_SHADERMODETRAIT_DEFERRED | XR_SHADERMODETRAIT_MOTIONMAP;
		break;
	case XR_SHADERMODE_FRAGMENTPROGRAM20DEFLIN :
		m_ShaderModeTraits |= XR_SHADERMODETRAIT_DEFERRED | XR_SHADERMODETRAIT_LINEARCOLORSPACE;
		m_iDeferredShaderOffset = XRSHADER_FP20DEFLIN_LFM - XRSHADER_FP20DEF_LFM;
		break;
	default :;
	}

	if (m_ShaderModeTraits & XR_SHADERMODETRAIT_LINEARCOLORSPACE)
		m_CurrentSpecularScale[3] *= 2.0f;

	m_FallOffExp_Current = (m_ShaderModeTraits & XR_SHADERMODETRAIT_LINEARCOLORSPACE) ? m_FallOffExp_Linear : m_FallOffExp_Gamma;

	if (m_bDisplayNextModeSelection)
	{
		ConOutL(CStrF("ShaderMode %d (Preferred %d)", m_ShaderMode, m_PreferredShaderMode));
		ConOutL(CStrF("    Deferred: %s", (m_ShaderModeTraits & XR_SHADERMODETRAIT_DEFERRED) ? "Yes" : "No"));
		ConOutL(CStrF("    MRT: %s", (m_ShaderModeTraits & XR_SHADERMODETRAIT_MRT) ? "Yes" : "No"));
		ConOutL(CStrF("    ColorSpace: %s", (m_ShaderModeTraits & XR_SHADERMODETRAIT_LINEARCOLORSPACE) ? "Linear" : "Gamma"));
		ConOutL(CStrF("    MotionMap: %s", (m_ShaderModeTraits & XR_SHADERMODETRAIT_MOTIONMAP) ? "Yes" : "No"));
		m_bDisplayNextModeSelection = 0;
	}


	// -------------------------------------------------------------------

#ifdef SUPPORT_FRAGMENTPROGRAM
	CXR_VirtualAttributes_ShaderFP20_COREFBB::PrepareFrame(this);
	CXR_VirtualAttributes_ShaderFP20::PrepareFrame(this);
	CXR_VirtualAttributes_ShaderFP20_Glass::PrepareFrame(this);
//	CXR_VirtualAttributes_ShaderFP20SS::PrepareFrame(this);

	CXR_VirtualAttributes_ShaderFP20Def_COREFBB::PrepareFrame(this);
	CXR_VirtualAttributes_ShaderFP20Def::PrepareFrame(this);

	CXR_VirtualAttributes_ShaderFP20_LF::PrepareFrame(this);
	CXR_VirtualAttributes_ShaderFP20_LFM::PrepareFrame(this);
	CXR_VirtualAttributes_ShaderFP20Def_LF::PrepareFrame(this);
	CXR_VirtualAttributes_ShaderFP20Def_LFM::PrepareFrame(this);
#endif

	return true;
}

bool CXR_Shader::OnPrepareFrame(CXR_VBManager *_pVBM)
{
	if (!m_pEngine)
		Error("OnPrepareFrame", "Shader not created.");

	if(!_pVBM)
		_pVBM = m_pEngine->GetVBM();

	return PrepareFrame(m_pEngine->m_pRender, _pVBM);
}

bool CXR_Shader::OnEndFrame()
{
//	if (m_MaxShaders)
	{
//		OutputDebugString(CStrF("Shaders saved %d\n", m_nShaderSaved));
	}

	return true;
}

void CXR_Shader::OnPrecache()
{
	MAUTOSTRIP(CXR_Shader_OnPrecache, MAUTOSTRIP_VOID);

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("Read", "No texture-context available.");

	// Set precache on textures
	pTC->SetTextureParam(m_TextureID_AttenuationExp, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	pTC->SetTextureParam(m_TextureID_AttenuationDst2, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	pTC->SetTextureParam(m_TextureID_AttenuationDst2Alpha, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	pTC->SetTextureParam(m_TextureID_Normalize, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	pTC->SetTextureParam(m_TextureID_FlipNormalize, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	pTC->SetTextureParam(m_TextureID_DefaultLens, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	pTC->SetTextureParam(m_TextureID_DefaultNormal, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	pTC->SetTextureParam(m_TextureID_SpecularPower, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	pTC->SetTextureParam(m_TextureID_Special_Cube_ffffffff, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	pTC->SetTextureParam(m_TextureID_Special_Cube_ff000000, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	pTC->SetTextureParam(m_TextureID_Special_ffffffff, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	pTC->SetTextureParam(m_TextureID_Special_ff000000, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	pTC->SetTextureParam(m_TextureID_Rand_02, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	pTC->SetTextureParam(m_TextureID_Rand_03, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	pTC->SetTextureParam(m_TextureID_Rand_04, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);

	pTC->SetTextureParam(pTC->GetTextureID("SPECIAL_FIT2"), CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);

//	pTC->SetTextureParam(m_TextureID_SbzProjTemp, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);

	for(int i = 0; i < 8; i++)
		pTC->SetTextureParam(m_lTextureID_Specular[i], CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
}


int CXR_Shader::GetShaderMode()
{
	MAUTOSTRIP(CXR_Shader_GetShaderMode, 0);
	return m_ShaderMode;
}

int CXR_Shader::GetShaderModeTraits()
{
	MAUTOSTRIP(CXR_Shader_GetShaderModeTraits, 0);
	return m_ShaderModeTraits;
}

int CXR_Shader::GetShaderFlags()
{
	MAUTOSTRIP(CXR_Shader_GetShaderFlags, 0);
	return 0;
}

void CXR_Shader::CalcTangentSpaceLightVector(const CVec3Dfp32& _LightPos, int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN, const CVec3Dfp32* _pTangU, const CVec3Dfp32* _pTangV, CVec3Dfp32* _pLV)
{
	MAUTOSTRIP(CXR_Shader_CalcTangentSpaceLightVector, MAUTOSTRIP_VOID);
	for(int v = 0; v < _nV; v++)
	{
		CVec3Dfp32 lv;
		_LightPos.Sub(_pV[v], lv);

		fp32 x = lv[0]*_pTangU[v][0] + lv[1]*_pTangU[v][1] + lv[2]*_pTangU[v][2];
		fp32 y = lv[0]*_pTangV[v][0] + lv[1]*_pTangV[v][1] + lv[2]*_pTangV[v][2];
		fp32 z = lv[0]*_pN[v][0] + lv[1]*_pN[v][1] + lv[2]*_pN[v][2];

		_pLV[v] = CVec3Dfp32(x,y,z);	
	}
}

bool CXR_Shader::CalcTangentSpaceLightVector(const CVec3Dfp32& _LightPos, CXR_VertexBuffer* _pVB)
{
	MAUTOSTRIP(CXR_Shader_CalcTangentSpaceLightVector_2, false);
	CXR_VBChain *pChain = _pVB->GetVBChain();
	CVec3Dfp32* pLV = m_pVBM->Alloc_V3(pChain->m_nV);
	if (!pLV) return false;

	CalcTangentSpaceLightVector(_LightPos, pChain->m_nV, pChain->m_pV, pChain->m_pN, (CVec3Dfp32*)pChain->m_pTV[2], (CVec3Dfp32*)pChain->m_pTV[3], pLV);
	_pVB->Geometry_TVertexArray(pLV, 0);
	return true;
}

void CXR_Shader::Debug_RenderTexture(CXR_VertexBufferGeometry* _pVB, int _TextureID)
{
	CXR_VertexBuffer* pVB = m_pVBM->Alloc_VBAttrib();
	if (!pVB) return;

/*	if (_pVB->m_pAttrib)
	{
		pVB->m_pAttrib->m_iTexCoordSet[0] = _pVB->m_pAttrib->m_iTexCoordSet[0];
		pVB->m_pAttrib->m_iTexCoordSet[1] = _pVB->m_pAttrib->m_iTexCoordSet[1];
		pVB->m_pAttrib->m_iTexCoordSet[2] = _pVB->m_pAttrib->m_iTexCoordSet[2];
		pVB->m_pAttrib->m_iTexCoordSet[3] = _pVB->m_pAttrib->m_iTexCoordSet[3];
	}*/

	// Copy geometry
	pVB->CopyVBChain(_pVB);

	pVB->m_Priority = CXR_VBPRIORITY_MODEL_OPAQUE;
	pVB->Matrix_Set(_pVB->m_pTransform);
	pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

	pVB->m_pAttrib->Attrib_TextureID(0, _TextureID);
	pVB->m_Color = 0xff808080;

	m_pVBM->AddVB(pVB);
}

void CXR_Shader::RenderShading(const CXR_Light& _Light, CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams* _pShaderParams, const CXR_SurfaceShaderParams** _lpSurfParams)
{
	MAUTOSTRIP(CXR_Shader_RenderShading, MAUTOSTRIP_VOID);
	if (!_pVB || !_pShaderParams) return;

	if (_Light.m_Type != CXR_LIGHTTYPE_POINT && _Light.m_Type != CXR_LIGHTTYPE_SPOT)
		return;

	if (m_pEngine && m_pEngine->m_bCurrentVCMirrored)
		const_cast<CXR_ShaderParams*>(_pShaderParams)->m_Flags |= XR_SHADERFLAGS_CULLCW;

//	const CXR_SurfaceShaderParams* pSurfParams = _pShaderParams->m_pSurfParams;

	uint nVB = _pShaderParams->m_nVB;
	if (m_pEngine && m_pEngine->m_DebugFlags & M_Bit(11) && !(_pShaderParams->m_Flags & XR_SHADERFLAGS_GLASS) )
	{
		for(uint iVB = 0; iVB < nVB; iVB++)
			Debug_RenderTexture(_pVB+iVB, _lpSurfParams[iVB]->m_lTextureIDs[XR_SHADERMAP_NORMAL]);
	}
	else
	{
		switch(m_ShaderMode)
		{
	/*
		case XR_SHADERMODE_REGCOMBINERS_NV10 :
			RenderShading_NV10(_Light, _pVB, _pSurf);
			return;

		case XR_SHADERMODE_REGCOMBINERS_NV10HQ :
			RenderShading_NV10HQ(_Light, _pVB, _pSurf);
			return;
	*/

//		case XR_SHADERMODE_FRAGMENTPROGRAM11 :
//			RenderShading_NV20(_Light, _pVB, _pShaderParams);
//			return;

#ifdef SUPPORT_FRAGMENTPROGRAM
/*		case XR_SHADERMODE_FRAGMENTPROGRAM14 :
			RenderShading_FP14(_Light, _pVB, _pShaderParams);
			return;
*/
		case XR_SHADERMODE_FRAGMENTPROGRAM20DEF :
		case XR_SHADERMODE_FRAGMENTPROGRAM20DEFMRT :
		case XR_SHADERMODE_FRAGMENTPROGRAM20DEFLIN :
		case XR_SHADERMODE_FRAGMENTPROGRAM20DEFMM :
			if (m_pEngine && !(_pShaderParams->m_Flags & XR_SHADERFLAGS_TRANSPARENT))
			{
				{
					uint iVB = 0;
					while(iVB < nVB)
					{
						uint iBRDF = _lpSurfParams[iVB]->m_iBRDFModel;
						uint iVBStart = iVB;
						iVB++;
						while((iVB < nVB) && (iBRDF == _lpSurfParams[iVB]->m_iBRDFModel))
							iVB++;
						
						const_cast<CXR_ShaderParams*>(_pShaderParams)->m_nVB = iVB - iVBStart;

						if (iBRDF > 0)
						{
							for(uint i = iVBStart; i < iVB; i++)
								RenderShading_FP20Def(_Light, _pVB + i, _pShaderParams, _lpSurfParams[i]);
						}
						else
							RenderShading_FP20Def_COREFBB(_Light, _pVB + iVBStart, _pShaderParams, _lpSurfParams + iVBStart);
					}
					const_cast<CXR_ShaderParams*>(_pShaderParams)->m_nVB = nVB;
				}
				return;
			}	// Fall through to XR_SHADERMODE_FRAGMENTPROGRAM20

		case XR_SHADERMODE_FRAGMENTPROGRAM20 :
			if (_pShaderParams->m_Flags & XR_SHADERFLAGS_GLASS)
			{
//				for(uint iVB = 0; iVB < nVB; iVB++)
				RenderShading_FP20_Glass(_Light, _pVB, _pShaderParams, _lpSurfParams[0]);
			}
			else
			{
				for(uint iVB = 0; iVB < nVB; iVB++)
					if (_lpSurfParams[iVB]->m_iBRDFModel > 0)
						RenderShading_FP20(_Light, _pVB+iVB, _pShaderParams, _lpSurfParams[iVB]);
					else
						RenderShading_FP20_COREFBB(_Light, _pVB+iVB, _pShaderParams, _lpSurfParams[iVB]);
			}
			return;

		case XR_SHADERMODE_FRAGMENTPROGRAM20SS :
			{
				for(uint iVB = 0; iVB < nVB; iVB++)
					if (_lpSurfParams[iVB]->m_iBRDFModel > 0)
						RenderShading_FP20(_Light, _pVB+iVB, _pShaderParams, _lpSurfParams[iVB]);
					else
						RenderShading_FP20_COREFBB(_Light, _pVB+iVB, _pShaderParams, _lpSurfParams[iVB]);
			}
			return;

#endif // SUPPORT_FRAGMENTPROGRAM
		}
	}
}

void CXR_Shader::RenderShading_LightField(CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams_LightField* _pShaderParams, const CXR_SurfaceShaderParams** _lpSurfParams)
{
	MAUTOSTRIP(CXR_Shader_RenderShading, MAUTOSTRIP_VOID);
	if (!_pVB || !_pShaderParams)
		return;

	if (m_pEngine && m_pEngine->m_bCurrentVCMirrored)
		const_cast<CXR_ShaderParams_LightField*>(_pShaderParams)->m_Flags |= XR_SHADERFLAGS_CULLCW;

	if (m_pEngine && m_pEngine->m_DebugFlags & M_Bit(11))
	{
		uint nVB = _pShaderParams->m_nVB;
		for(uint iVB = 0; iVB < nVB; iVB++)
			Debug_RenderTexture(_pVB, _lpSurfParams[iVB]->m_lTextureIDs[XR_SHADERMAP_NORMAL]);
	}
	else
	{
		switch(m_ShaderMode)
		{
		case XR_SHADERMODE_FRAGMENTPROGRAM20DEF :
		case XR_SHADERMODE_FRAGMENTPROGRAM20DEFMRT :
		case XR_SHADERMODE_FRAGMENTPROGRAM20DEFLIN :
		case XR_SHADERMODE_FRAGMENTPROGRAM20DEFMM :

			if (m_pEngine)
			{
				uint nVB = _pShaderParams->m_nVB;
				for(uint iVB = 0; iVB < nVB; iVB++)
					RenderShading_FP20Def_LF(_pVB, _pShaderParams, _lpSurfParams[iVB]);
				return;
			}	// Note: Intentional fall through
		case XR_SHADERMODE_FRAGMENTPROGRAM20 :
		case XR_SHADERMODE_FRAGMENTPROGRAM20SS :
			{
				uint nVB = _pShaderParams->m_nVB;
				for(uint iVB = 0; iVB < nVB; iVB++)
					RenderShading_FP20_LF(_pVB, _pShaderParams, _lpSurfParams[iVB]);
				return;
			}
		}
	}
}

void CXR_Shader::RenderShading_LightFieldMapping(CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams_LightFieldMapping* _pShaderParams, const CXR_SurfaceShaderParams** _lpSurfParams)
{
	MAUTOSTRIP(CXR_Shader_RenderShading, MAUTOSTRIP_VOID);
	if (!_pVB || !_pShaderParams)
		return;

	if (m_pEngine && m_pEngine->m_bCurrentVCMirrored)
		const_cast<CXR_ShaderParams_LightFieldMapping*>(_pShaderParams)->m_Flags |= XR_SHADERFLAGS_CULLCW;

	uint nVB = _pShaderParams->m_nVB;
	M_ASSERT(nVB <= XR_SHADER_MAXVB, "Too many VBs.");

	if (m_pEngine && m_pEngine->m_DebugFlags & M_Bit(11))
	{
		for(uint iVB = 0; iVB < nVB; iVB++)
		{
			const CXR_SurfaceShaderParams* pSurfParams = _lpSurfParams[iVB];
			Debug_RenderTexture(_pVB+iVB, pSurfParams->m_lTextureIDs[XR_SHADERMAP_NORMAL]);
		}
	}
	else
	{
		switch(m_ShaderMode)
		{
		case XR_SHADERMODE_FRAGMENTPROGRAM20DEF :
		case XR_SHADERMODE_FRAGMENTPROGRAM20DEFMRT :
		case XR_SHADERMODE_FRAGMENTPROGRAM20DEFLIN :
		case XR_SHADERMODE_FRAGMENTPROGRAM20DEFMM :

			if (m_pEngine)
			{
				RenderShading_FP20Def_LFM(_pVB, _pShaderParams, _lpSurfParams);
				return;
			}	// Note: Intentional fall through
		case XR_SHADERMODE_FRAGMENTPROGRAM20 :
		case XR_SHADERMODE_FRAGMENTPROGRAM20SS :
			for(uint iVB = 0; iVB < nVB; iVB++)
				RenderShading_FP20_LFM(_pVB+iVB, _pShaderParams, _lpSurfParams[iVB]);
			return;

		}
	}
}

void CXR_Shader::RenderDeferredArray(uint _nVB, CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams* _pShaderParams, const CXR_SurfaceShaderParams** _lpShaderParams,
								int _Enable,int _Disable,fp32 _BasePrio,fp32 _OffsPrio,uint16 _Src,
								CVec4Dfp32 * _pParams)
{
	uint bUseMRT = m_ShaderModeTraits & XR_SHADERMODETRAIT_MRT;
	if (!_nVB)
		return;

	if (m_ShaderModeTraits & XR_SHADERMODETRAIT_MOTIONMAP)
	{
		RenderDeferredMotionVectorArray(_nVB, _pVB, _pShaderParams, _lpShaderParams, _Enable, _Disable, _BasePrio, _OffsPrio, _Src, _pParams);
		return;
	}

	if ((_lpShaderParams[0]->m_lDeferredRasterMode[1] - _lpShaderParams[0]->m_lDeferredRasterMode[0]) | (_lpShaderParams[0]->m_lDeferredRasterMode[2] - _lpShaderParams[0]->m_lDeferredRasterMode[0]))
		bUseMRT = 0;

	if (_Src != 0)
		bUseMRT = 0;

	if (bUseMRT)
	{
		uint nBytes = _nVB*(sizeof(CXR_VertexBuffer) + sizeof(CXR_VirtualAttributes_DeferredMRT) + 4) + sizeof(CRC_ExtAttributes_FragmentProgram20);
		CXR_VertexBuffer *M_RESTRICT lVB = (CXR_VertexBuffer*)m_pVBM->Alloc(nBytes, true);
		if(!lVB) return;

		CXR_VirtualAttributes_DeferredMRT *M_RESTRICT lVA = (CXR_VirtualAttributes_DeferredMRT*)(lVB + _nVB);
		CRC_ExtAttributes_FragmentProgram20 *M_RESTRICT pFP = (CRC_ExtAttributes_FragmentProgram20*)(lVA + _nVB);

		pFP->Clear();
		pFP->SetProgram("XRShader_DeferredMRT", MHASH5('XRSh','ader','_Def','erre','dMRT'));

		CXR_VertexBuffer** lpVB = (CXR_VertexBuffer**) (pFP + 1);

		uint iVB;
		for(iVB = 0; iVB < _nVB; iVB++)
		{
			const CXR_SurfaceShaderParams* pSSP = _lpShaderParams[iVB];
			CXR_VirtualAttributes_DeferredMRT*M_RESTRICT pVA = lVA+iVB;
			new(pVA) CXR_VirtualAttributes_DeferredMRT;
			pVA->Create();
			pVA->m_pExtAttrib = pFP;

			pVA->m_TextureID_Normal = pSSP->m_lTextureIDs[XR_SHADERMAP_NORMAL];
			pVA->m_TextureID_Diffuse = pSSP->m_lTextureIDs[XR_SHADERMAP_DIFFUSE];
			pVA->m_TextureID_Specular= pSSP->m_lTextureIDs[XR_SHADERMAP_SPECULAR];
			pVA->m_RasterMode = pSSP->m_lDeferredRasterMode[0];
			pVA->m_Disable = _Disable;
			pVA->m_Enable = _Enable | 
				CRC_FLAGS_COLORWRITE0 | CRC_FLAGS_COLORWRITE1 | CRC_FLAGS_COLORWRITE2 | 
				CRC_FLAGS_ALPHAWRITE0 | CRC_FLAGS_ALPHAWRITE1 | CRC_FLAGS_ALPHAWRITE2;
			pVA->m_ZComp = CRC_COMPARE_EQUAL;
		}

		{
			for(uint i = 0; i < _nVB; i++)
				lpVB[i] = lVB+i;
		}

		for(iVB = 0; iVB < _nVB; iVB++)
		{
			CXR_VertexBuffer*M_RESTRICT pVB = lVB+iVB;
			pVB->Construct_Geometry_VA_Priority(_pVB+iVB, lVA+iVB, CXR_VBPRIORITY_UNIFIED_ZBUFFER + 7.0f);
//			lpVB[iVB] = lVB+iVB;
		}
/*		for(iVB = 0; iVB < _nVB; iVB++)
		{
			CXR_VertexBuffer*M_RESTRICT pVB = lVB+iVB;
			pVB->CopyVBChainAndTransform(_pVB+iVB);
		}
		for(iVB = 0; iVB < _nVB; iVB++)
		{
			CXR_VertexBuffer*M_RESTRICT pVB = lVB+iVB;
			CXR_VirtualAttributes_DeferredMRT*M_RESTRICT pVA = lVA+iVB;
			pVB->SetVirtualAttr(pVA);
			pVB->m_Priority = CXR_VBPRIORITY_UNIFIED_ZBUFFER + 7.0f;
		}*/

		m_pVBM->AddVBArray(lpVB, _nVB);
	}
	else
	{
		for(uint iVB = 0; iVB < _nVB; iVB++)
		{
			const CXR_SurfaceShaderParams* pSSP = _lpShaderParams[iVB];
			uint nBytes = Align(sizeof(CXR_VertexBuffer) * 3,4) + Align(sizeof(CRC_ExtAttributes_FragmentProgram20), 4) + Align(sizeof(CXR_VirtualAttributes_Deferred) * 3,4);
			
			CXR_VertexBuffer *M_RESTRICT pVBData = (CXR_VertexBuffer*)m_pVBM->Alloc(nBytes);

			if( !pVBData ) return;

			CXR_VirtualAttributes_Deferred *M_RESTRICT pNewAttrib = (CXR_VirtualAttributes_Deferred*)(pVBData + 3);
			CXR_VirtualAttributes_Deferred *M_RESTRICT lpAttrib[3];
			CXR_VertexBuffer *M_RESTRICT lpVB[3];

			int i;

			lpAttrib[0] = new(pNewAttrib+0) CXR_VirtualAttributes_Deferred;
			lpAttrib[1] = new(pNewAttrib+1) CXR_VirtualAttributes_Deferred;
			lpAttrib[2] = new(pNewAttrib+2) CXR_VirtualAttributes_Deferred;

			lpAttrib[0]->Create();
			lpAttrib[1]->Create();
			lpAttrib[2]->Create();
			lpVB[0] = pVBData+0;
			lpVB[1] = pVBData+1;
			lpVB[2] = pVBData+2;
			lpVB[0]->Construct();
			lpVB[1]->Construct();
			lpVB[2]->Construct();


			uint8 iComp = CRC_COMPARE_EQUAL;

			CRC_ExtAttributes_FragmentProgram20*M_RESTRICT pExtAttrib = (CRC_ExtAttributes_FragmentProgram20*)(pNewAttrib + 3);
			switch( _Src )
			{

			//Normal
			case 0:
				{
					pExtAttrib->Clear();
					pExtAttrib->SetProgram("XRShader_DeferredNormal", MHASH6('XRSh','ader','_Def','erre','dNor','mal'));
					lpAttrib[0]->m_pExtAttrib = pExtAttrib;
				}
				break;

			//BSP Decal
			case 1:
				{
					pExtAttrib->Clear();
					pExtAttrib->SetProgram("XRShader_DecalNormalTransform", MHASH8('XRSh','ader','_Dec','alNo','rmal','Tran','sfor','m'));
					lpAttrib[0]->m_pExtAttrib = pExtAttrib;

					for( i = 0;i < 3; i++ )
					{
						lpAttrib[i]->m_PolygonOffset = pSSP->m_PolyOffset;
						lpAttrib[i]->m_PolygonOffsetScale = pSSP->m_PolyOffsetScale;
					}
					_Enable |= CRC_FLAGS_POLYGONOFFSET | CRC_FLAGS_BLEND;  //The blend-flag should be tested first, really

					iComp = CRC_COMPARE_LESSEQUAL;
				}
				break;

			//Trimesh Decal
			case 2:
				{
					CRC_ExtAttributes_FragmentProgram20 *M_RESTRICT pNormal = 
						(CRC_ExtAttributes_FragmentProgram20*M_RESTRICT)m_pVBM->Alloc(
						sizeof(CRC_ExtAttributes_FragmentProgram20) * 2);
					CRC_ExtAttributes_FragmentProgram20 *M_RESTRICT pDiffSpec = pNormal + 1;
					pNormal->Clear();
					pDiffSpec->Clear();
					pNormal->SetProgram("XRShader_DecalNormalTransformTM", MHASH8('XRSh','ader','_Dec','alNo','rmal','Tran','sfor','mTM'));
					pDiffSpec->SetProgram("XRShader_DecalTM",MHASH4('XRSh','ader','_Dec','alTM'));
					pNormal->SetParameters(_pParams+2,1);
					pDiffSpec->SetParameters(_pParams+2,1);
					lpAttrib[0]->m_pExtAttrib = pNormal;
					lpAttrib[1]->m_pExtAttrib = pDiffSpec;
					lpAttrib[2]->m_pExtAttrib = pDiffSpec;

					for( i = 0; i < 3; i++ )
					{
						lpAttrib[i]->m_TexGen[0] = CRC_TEXGENMODE_LINEAR;
						lpAttrib[i]->m_TexGen[1] = CRC_TEXGENMODE_MSPOS;
						lpAttrib[i]->m_pTexGenAttr = (fp32*)_pParams;
						lpAttrib[i]->m_PolygonOffset = pSSP->m_PolyOffset;
						lpAttrib[i]->m_PolygonOffsetScale = pSSP->m_PolyOffsetScale;
					}
					_Enable |= CRC_FLAGS_POLYGONOFFSET | CRC_FLAGS_BLEND;  //The blend-flag should be tested first, really

					//Normal creation
					lpAttrib[0]->m_TexGen[2] = CRC_TEXGENMODE_DECALTSTRANSFORM;
					lpAttrib[0]->m_TexGen[3] = CRC_TEXGENMODE_VOID;

					iComp = CRC_COMPARE_LESSEQUAL;
				}
				break;

			}

			lpAttrib[0]->m_TextureID = pSSP->m_lTextureIDs[XR_SHADERMAP_NORMAL];
			lpAttrib[1]->m_TextureID = pSSP->m_lTextureIDs[XR_SHADERMAP_DIFFUSE];
			lpAttrib[2]->m_TextureID = pSSP->m_lTextureIDs[XR_SHADERMAP_SPECULAR];

			for( i = 0;i < 3;i ++)
			{
				lpAttrib[i]->m_RasterMode = pSSP->m_lDeferredRasterMode[i];
				lpAttrib[i]->m_Disable = _Disable;
				lpAttrib[i]->m_Enable = _Enable;
				lpAttrib[i]->m_ZComp = iComp;

				lpVB[i]->CopyVBChainAndTransform(_pVB + iVB);
				lpVB[i]->SetVirtualAttr(lpAttrib[i]);
			}

			{
				fp32 Prio = _BasePrio;
				for(uint8 i = 0;i < 3;i++)
				{
					if(lpAttrib[i]->m_TextureID)
						lpVB[i]->m_Priority = Prio;
					Prio += _OffsPrio;
				}
			}
			m_pVBM->AddVBArray(lpVB, 3);
		}

//		RenderDeferred_VB(_pVB,(CXR_VertexBuffer*)pVBData,_BasePrio,_OffsPrio);
	}
}

void CXR_Shader::RenderDeferredMotionVectorArray(
	uint _nVB, CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams* _pShaderParams, const CXR_SurfaceShaderParams** _lpShaderParams,
	int _Enable,int _Disable,fp32 _BasePrio,fp32 _OffsPrio,uint16 _Src,
	CVec4Dfp32 * _pParams)
{
	uint bUseMRT = m_ShaderModeTraits & XR_SHADERMODETRAIT_MRT;
	if (!_nVB)
		return;

	if ((_lpShaderParams[0]->m_lDeferredRasterMode[1] - _lpShaderParams[0]->m_lDeferredRasterMode[0]) | (_lpShaderParams[0]->m_lDeferredRasterMode[2] - _lpShaderParams[0]->m_lDeferredRasterMode[0]))
		bUseMRT = 0;

	if (_Src != 0)
		bUseMRT = 0;

/*	if (bUseMRT)
	{
		uint nBytes = _nVB*(sizeof(CXR_VertexBuffer) + sizeof(CXR_VirtualAttributes_DeferredMRT) + 4) + sizeof(CRC_ExtAttributes_FragmentProgram20);
		CXR_VertexBuffer *M_RESTRICT lVB = (CXR_VertexBuffer*)m_pVBM->Alloc(nBytes, true);
		if(!lVB) return;

		CXR_VirtualAttributes_DeferredMRT *M_RESTRICT lVA = (CXR_VirtualAttributes_DeferredMRT*)(lVB + _nVB);
		CRC_ExtAttributes_FragmentProgram20 *M_RESTRICT pFP = (CRC_ExtAttributes_FragmentProgram20*)(lVA + _nVB);

		pFP->Clear();
		pFP->SetProgram("XRShader_DeferredMM_MRT", MHASH6('XRSh','ader','_Def','erre','dMM_','MRT'));

		CXR_VertexBuffer** lpVB = (CXR_VertexBuffer**) (pFP + 1);

		uint iVB;
		for(iVB = 0; iVB < _nVB; iVB++)
		{
			const CXR_SurfaceShaderParams* pSSP = _lpShaderParams[iVB];
			CXR_VirtualAttributes_DeferredMRT*M_RESTRICT pVA = lVA+iVB;
			new(pVA) CXR_VirtualAttributes_DeferredMRT;
			pVA->Create();
			pVA->m_pExtAttrib = pFP;

			pVA->m_TextureID_Normal = pSSP->m_lTextureIDs[XR_SHADERMAP_NORMAL];
			pVA->m_TextureID_Diffuse = pSSP->m_lTextureIDs[XR_SHADERMAP_DIFFUSE];
			pVA->m_TextureID_Specular= pSSP->m_lTextureIDs[XR_SHADERMAP_SPECULAR];
			pVA->m_RasterMode = pSSP->m_lDeferredRasterMode[0];
			pVA->m_Disable = _Disable;
			pVA->m_Enable = _Enable | 
				CRC_FLAGS_COLORWRITE0 | CRC_FLAGS_COLORWRITE1 | CRC_FLAGS_COLORWRITE2 | 
				CRC_FLAGS_ALPHAWRITE0 | CRC_FLAGS_ALPHAWRITE1 | CRC_FLAGS_ALPHAWRITE2;
			pVA->m_ZComp = CRC_COMPARE_EQUAL;
		}

		{
			for(uint i = 0; i < _nVB; i++)
				lpVB[i] = lVB+i;
		}

		for(iVB = 0; iVB < _nVB; iVB++)
		{
			CXR_VertexBuffer*M_RESTRICT pVB = lVB+iVB;
			pVB->Construct_Geometry_VA_Priority(_pVB+iVB, lVA+iVB, CXR_VBPRIORITY_UNIFIED_ZBUFFER + 7.0f);
			//			lpVB[iVB] = lVB+iVB;
		}

		m_pVBM->AddVBArray(lpVB, _nVB);
	}
	else */
	{
		if (!m_pEngine)
			return;

		CVec4Dfp32*M_RESTRICT pParamMM = m_pVBM->Alloc_V4(9);
		if (!pParamMM)
			return;

		((CMat4Dfp32*)pParamMM)->Unit();
		CVec4Dfp32*M_RESTRICT pParamVP = pParamMM+8;

		CXR_ViewContext* pVC = m_pEngine->GetVC();

		const CMat4Dfp32* pM2VMat = _pVB[0].m_pTransform;

		CMat4Dfp32 M;
		if (_pShaderParams->m_pCurrentWVelMat)
		{
			if (!_pShaderParams->m_pCurrentVMat)
			{
				M_TRACEALWAYS("(CXR_Shader::RenderDeferredMotionMapArray) Missing VMat\n");
				M.Unit();
			}
			else
			{
				CMat4Dfp32 WMat, M2;
				_pShaderParams->m_pCurrentWMat->Multiply(*_pShaderParams->m_pCurrentWVelMat, WMat);
				WMat.Multiply(*_pShaderParams->m_pCurrentVMat, M2);
				M2.Multiply(pVC->m_dW2VMat, M);
			}
		}
		else
			pM2VMat->Multiply(pVC->m_dW2VMat, M);

		pM2VMat->Transpose(*((CMat4Dfp32*)pParamMM));
		M.Transpose(*((CMat4Dfp32*)(pParamMM+4)));

		for(uint iVB = 0; iVB < _nVB; iVB++)
		{
			const CXR_SurfaceShaderParams* pSSP = _lpShaderParams[iVB];
			uint nBytes = Align(sizeof(CXR_VertexBuffer) * 4, 4) + Align(sizeof(CRC_ExtAttributes_FragmentProgram20)*2, 4) + Align(sizeof(CXR_VirtualAttributes_Deferred) * 3 + sizeof(CXR_VirtualAttributes_MotionMap),4);

			CXR_VertexBuffer *M_RESTRICT pVBData = (CXR_VertexBuffer*)m_pVBM->Alloc(nBytes);

			if( !pVBData ) return;

			CXR_VirtualAttributes_Deferred *M_RESTRICT pNewAttrib = (CXR_VirtualAttributes_Deferred*)(pVBData + 4);
			CXR_VirtualAttributes_Deferred *M_RESTRICT lpAttrib[3];
			CXR_VertexBuffer *M_RESTRICT lpVB[4];

			int i;

			lpAttrib[0] = new(pNewAttrib+0) CXR_VirtualAttributes_Deferred;
			lpAttrib[1] = new(pNewAttrib+1) CXR_VirtualAttributes_Deferred;
			lpAttrib[2] = new(pNewAttrib+2) CXR_VirtualAttributes_Deferred;

			lpAttrib[0]->Create();
			lpAttrib[1]->Create();
			lpAttrib[2]->Create();
			lpVB[0] = pVBData + 0;
			lpVB[1] = pVBData + 1;
			lpVB[2] = pVBData + 2;
			lpVB[3] = pVBData + 3;
			lpVB[0]->Construct();
			lpVB[1]->Construct();
			lpVB[2]->Construct();
			lpVB[3]->Construct();

			CXR_VirtualAttributes_MotionMap *M_RESTRICT pAttribMM = new(pNewAttrib+3) CXR_VirtualAttributes_MotionMap;
			pAttribMM->Create();

			uint8 iComp = CRC_COMPARE_EQUAL;

			CRC_ExtAttributes_FragmentProgram20*M_RESTRICT pExtAttrib = (CRC_ExtAttributes_FragmentProgram20*)(pAttribMM + 1);
			CRC_ExtAttributes_FragmentProgram20*M_RESTRICT pExtAttribMM = (CRC_ExtAttributes_FragmentProgram20*)(pExtAttrib + 1);

			pExtAttribMM->Clear();
			pExtAttribMM->SetProgram("XRShader_MotionMap", MHASH5('XRSh','ader','_Mot','ion','Map'));
			pExtAttribMM->SetParameters(pParamVP, 1);
			pAttribMM->m_pExtAttrib = pExtAttribMM;

			switch( _Src )
			{

				//Normal
			case 0:
				{
					pExtAttrib->Clear();
					pExtAttrib->SetProgram("XRShader_DeferredNormal", MHASH6('XRSh','ader','_Def','erre','dNor','mal'));
					lpAttrib[0]->m_pExtAttrib = pExtAttrib;
				}
				break;

				//BSP Decal
			case 1:
				{
					pExtAttrib->Clear();
					pExtAttrib->SetProgram("XRShader_DecalNormalTransform", MHASH8('XRSh','ader','_Dec','alNo','rmal','Tran','sfor','m'));
					lpAttrib[0]->m_pExtAttrib = pExtAttrib;

					for( i = 0;i < 3; i++ )
					{
						lpAttrib[i]->m_PolygonOffset = pSSP->m_PolyOffset;
						lpAttrib[i]->m_PolygonOffsetScale = pSSP->m_PolyOffsetScale;
					}
					_Enable |= CRC_FLAGS_POLYGONOFFSET | CRC_FLAGS_BLEND;  //The blend-flag should be tested first, really

					iComp = CRC_COMPARE_LESSEQUAL;
				}
				break;

				//Trimesh Decal
			case 2:
				{
					CRC_ExtAttributes_FragmentProgram20 *M_RESTRICT pNormal = 
						(CRC_ExtAttributes_FragmentProgram20*M_RESTRICT)m_pVBM->Alloc(
						sizeof(CRC_ExtAttributes_FragmentProgram20) * 2);
					CRC_ExtAttributes_FragmentProgram20 *M_RESTRICT pDiffSpec = pNormal + 1;
					pNormal->Clear();
					pDiffSpec->Clear();
					pNormal->SetProgram("XRShader_DecalNormalTransformTM", MHASH8('XRSh','ader','_Dec','alNo','rmal','Tran','sfor','mTM'));
					pDiffSpec->SetProgram("XRShader_DecalTM",MHASH4('XRSh','ader','_Dec','alTM'));
					pNormal->SetParameters(_pParams+2,1);
					pDiffSpec->SetParameters(_pParams+2,1);
					lpAttrib[0]->m_pExtAttrib = pNormal;
					lpAttrib[1]->m_pExtAttrib = pDiffSpec;
					lpAttrib[2]->m_pExtAttrib = pDiffSpec;

					for( i = 0; i < 3; i++ )
					{
						lpAttrib[i]->m_TexGen[0] = CRC_TEXGENMODE_LINEAR;
						lpAttrib[i]->m_TexGen[1] = CRC_TEXGENMODE_MSPOS;
						lpAttrib[i]->m_pTexGenAttr = (fp32*)_pParams;
						lpAttrib[i]->m_PolygonOffset = pSSP->m_PolyOffset;
						lpAttrib[i]->m_PolygonOffsetScale = pSSP->m_PolyOffsetScale;
					}
					_Enable |= CRC_FLAGS_POLYGONOFFSET | CRC_FLAGS_BLEND;  //The blend-flag should be tested first, really

					//Normal creation
					lpAttrib[0]->m_TexGen[2] = CRC_TEXGENMODE_DECALTSTRANSFORM;
					lpAttrib[0]->m_TexGen[3] = CRC_TEXGENMODE_VOID;

					iComp = CRC_COMPARE_LESSEQUAL;
				}
				break;

			}

			lpAttrib[0]->m_TextureID = pSSP->m_lTextureIDs[XR_SHADERMAP_NORMAL];
			lpAttrib[1]->m_TextureID = pSSP->m_lTextureIDs[XR_SHADERMAP_DIFFUSE];
			lpAttrib[2]->m_TextureID = pSSP->m_lTextureIDs[XR_SHADERMAP_SPECULAR];

			for( i = 0;i < 3;i ++)
			{
				lpAttrib[i]->m_RasterMode = pSSP->m_lDeferredRasterMode[i];
				lpAttrib[i]->m_Disable = _Disable;
				lpAttrib[i]->m_Enable = _Enable;
				lpAttrib[i]->m_ZComp = iComp;

				lpVB[i]->CopyVBChainAndTransform(_pVB + iVB);
				lpVB[i]->SetVirtualAttr(lpAttrib[i]);
			}

			{
				pAttribMM->m_RasterMode = pSSP->m_lDeferredRasterMode[0];
				pAttribMM->m_Disable = _Disable;
				pAttribMM->m_Enable = _Enable;
				pAttribMM->m_ZComp = iComp;
				pAttribMM->m_pTexGenAttr = (fp32*) pParamMM;
				lpVB[3]->CopyVBChainAndTransform(_pVB + iVB);
				lpVB[3]->SetVirtualAttr(pAttribMM);
			}

			{
				fp32 Prio = _BasePrio;
				for(uint8 i = 0;i < 3;i++)
				{
					if(lpAttrib[i]->m_TextureID)
						lpVB[i]->m_Priority = Prio;
					Prio += _OffsPrio;
				}
				lpVB[3]->m_Priority = Prio;
			}
			m_pVBM->AddVBArray(lpVB, 4);
		}

		//		RenderDeferred_VB(_pVB,(CXR_VertexBuffer*)pVBData,_BasePrio,_OffsPrio);
	}
}

void CXR_Shader::RenderMotionVectors(
	CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams* _pShaderParams,
	uint _Enable, uint _Disable, uint _RasterMode, uint _ZCompare)
{
	uint bUseMRT = m_ShaderModeTraits & XR_SHADERMODETRAIT_MRT;

	uint nVB = _pShaderParams->m_nVB;

	if (!nVB)
		return;

	{
		if (!m_pEngine)
			return;

		uint VBMem = sizeof(CVec4Dfp32) * 8 + sizeof(CRC_ExtAttributes_FragmentProgram20) + sizeof(CXR_VirtualAttributes_MotionMap) + nVB * Align(sizeof(CXR_VertexBuffer), 16);
		uint8* pVBMem = (uint8*) m_pVBM->Alloc(VBMem);
		if (!pVBMem)
			return;

		CVec4Dfp32*M_RESTRICT pParamMM = (CVec4Dfp32*M_RESTRICT)pVBMem; pVBMem += sizeof(CVec4Dfp32) * 8;
		CRC_ExtAttributes_FragmentProgram20*M_RESTRICT pExtAttribMM = (CRC_ExtAttributes_FragmentProgram20*)pVBMem; pVBMem += sizeof(CRC_ExtAttributes_FragmentProgram20);
		pExtAttribMM->Clear();
		pExtAttribMM->SetProgram("XRShader_MotionMap", MHASH5('XRSh','ader','_Mot','ion','Map'));
		CXR_VirtualAttributes_MotionMap *M_RESTRICT pAttribMM = new(pVBMem) CXR_VirtualAttributes_MotionMap; pVBMem += sizeof(CXR_VirtualAttributes_MotionMap);
		pAttribMM->Create();
		pAttribMM->m_pExtAttrib = pExtAttribMM;

		((CMat4Dfp32*)pParamMM)->Unit();

		CXR_ViewContext* pVC = m_pEngine->GetVC();

		const CMat4Dfp32* pM2VMat = _pVB[0].m_pTransform;

		CMat4Dfp32 M;
		if (_pShaderParams->m_pCurrentWVelMat)
		{
			if (!_pShaderParams->m_pCurrentVMat)
			{
				M_TRACEALWAYS("(CXR_Shader::RenderDeferredMotionMapArray) Missing VMat\n");
				M.Unit();
			}
			else
			{
				CMat4Dfp32 WMat, M2;
				_pShaderParams->m_pCurrentWMat->Multiply(*_pShaderParams->m_pCurrentWVelMat, WMat);
				WMat.Multiply(*_pShaderParams->m_pCurrentVMat, M2);
				M2.Multiply(pVC->m_dW2VMat, M);
			}
		}
		else
			pM2VMat->Multiply(pVC->m_dW2VMat, M);

		pM2VMat->Transpose(*((CMat4Dfp32*)pParamMM));
		M.Transpose(*((CMat4Dfp32*)(pParamMM+4)));

		CXR_VertexBuffer* lpVB[XR_SHADER_MAXVB];

		for(uint iVB = 0; iVB < nVB; iVB++)
		{
			CXR_VertexBuffer* pVB = (CXR_VertexBuffer*) pVBMem; pVBMem += sizeof(CXR_VertexBuffer);
			pVB->Construct();

			uint8 iComp = CRC_COMPARE_EQUAL;

			{
				pAttribMM->m_RasterMode = _RasterMode;
				pAttribMM->m_Disable = _Disable;
				pAttribMM->m_Enable = _Enable;
				pAttribMM->m_ZComp = _ZCompare;
				pAttribMM->m_pTexGenAttr = (fp32*) pParamMM;
				pVB->CopyVBChainAndTransform(_pVB + iVB);
				pVB->SetVirtualAttr(pAttribMM);
			}

			pVB->m_Priority = 11.0f;
			lpVB[iVB] = pVB;
		}

		m_pVBM->AddVBArray(lpVB, nVB);

		//		RenderDeferred_VB(_pVB,(CXR_VertexBuffer*)pVBData,_BasePrio,_OffsPrio);
	}
}

void CXR_Shader::Con_ShaderMode(int _v)
{
	m_PreferredShaderMode = _v;
	m_bDisplayNextModeSelection = 1;
	m_pSys->GetEnvironment()->SetValuei("XR_SHADERMODE", _v);
}

void CXR_Shader::Con_SpecularMode(int _v)
{
	m_SpecularMode = _v;
}

void CXR_Shader::Con_SpecularForcePower(int _v)
{
	m_SpecularForcePower = _v;
}

void CXR_Shader::Con_ShaderVirtualAttr(int _v)
{
	m_bUseVirtualAttr = _v;
}

void CXR_Shader::Con_ShaderFallOffExpLinear(fp32 _v)
{
	m_FallOffExp_Linear = _v;
}
void CXR_Shader::Con_ShaderFallOffExpGamma(fp32 _v)
{
	m_FallOffExp_Gamma = _v;
}


void CXR_Shader::Register()
{
	if (!m_bIsRegistered)
	{
		AddToConsole();
		m_bIsRegistered = true;
	}
}

void CXR_Shader::Register(CScriptRegisterContext & _RegContext)
{
	CConsoleClient::Register(_RegContext);
	_RegContext.RegFunction("xr_shadermode", this, &CXR_Shader::Con_ShaderMode);
	_RegContext.RegFunction("xr_specularmode", this, &CXR_Shader::Con_SpecularMode);
	_RegContext.RegFunction("xr_specularforcepower", this, &CXR_Shader::Con_SpecularForcePower);
	_RegContext.RegFunction("xr_shadervirtualattr", this, &CXR_Shader::Con_ShaderVirtualAttr);
	_RegContext.RegFunction("xr_shaderfalloffexp_linear", this, &CXR_Shader::Con_ShaderFallOffExpLinear);
	_RegContext.RegFunction("xr_shaderfalloffexp_gamma", this, &CXR_Shader::Con_ShaderFallOffExpGamma);
}

