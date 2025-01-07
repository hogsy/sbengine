// -------------------------------------------------------------------
//  Shader NV20+
// -------------------------------------------------------------------
#ifdef SUPPORT_REGISTERCOMBINERS


CRC_ExtAttributes_NV10* CXR_Shader::RegCombiners_NV20_DotProd_Specular(int _SpecularPower)
{
	// Alloc reg-combiners attribute
	CRC_ExtAttributes_NV20* pComb = (CRC_ExtAttributes_NV20*) m_pVBM->Alloc(sizeof(CRC_ExtAttributes_NV20));
	if (!pComb) return false;

	int SpecLog2 = 0;
	while((1 << SpecLog2) < _SpecularPower)
	{
		SpecLog2++;
	}

	pComb->Clear();

	int nComb = 0;
	pComb->Clear(nComb);
	pComb->SetInputRGB(nComb, NV_INPUT_TEXTURE2|NV_MAPPING_EXPNORMAL, NV_INPUT_TEXTURE1|NV_MAPPING_EXPNORMAL);
	pComb->SetOutputRGB(nComb, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, true, false, false);
	// dp3 r0.rgb, t2_bx2, t1_bx2
	nComb++;

	int nSqr = Min(4, SpecLog2);
	for(int i = 0; i < nSqr; i++)
	{
		pComb->Clear(nComb);
		pComb->SetInputRGB(nComb, NV_OUTPUT_SPARE0, NV_OUTPUT_SPARE0);
		pComb->SetOutputRGB(nComb, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);
		// mul r0.rgb, r0, r0
		nComb++;
	}

	if (SpecLog2 > 4)
	{
		pComb->Clear(nComb);
		pComb->SetInputRGB(nComb, NV_INPUT_SPARE0, NV_INPUT_ZERO|NV_MAPPING_INVERT, NV_INPUT_COLOR1|NV_MAPPING_SIGNEDNEGATE, NV_INPUT_ZERO|NV_MAPPING_INVERT);
		if (SpecLog2 > 5)
		{
			pComb->SetConst1(0xffc0c0c0);
			pComb->SetOutputRGB(nComb, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_OUTPUT_SPARE0, NV_SCALE_FOUR, false, false, false);
			// add_x4 r0.rgb, r0, -c1
		}
		else
		{
			pComb->SetConst1(0xff808080);
			pComb->SetOutputRGB(nComb, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_OUTPUT_SPARE0, NV_SCALE_TWO, false, false, false);
			// add_x2 r0.rgb, r0, -c1
		}
		nComb++;
	}

	pComb->Clear(nComb);
	pComb->SetInputRGB(nComb, NV_INPUT_COLOR0, NV_INPUT_SPARE0);
	pComb->SetOutputRGB(nComb, NV_OUTPUT_SPARE0, NV_OUTPUT_DISCARD, NV_OUTPUT_DISCARD, NV_SCALE_ONE, false, false, false);
	// mul r0.rgb, r0, c0
	nComb++;
	pComb->SetNumCombiners(nComb);
	pComb->SetFinal(NV_INPUT_TEXTURE0 | NV_COMP_ALPHA, NV_INPUT_SPARE0, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_ZERO, NV_INPUT_SPARE0 | NV_COMP_ALPHA);
	// mul r0.rgb, t0.a, r0

	// rgbout = a*b + (1-sa)*c + d
	// alphaout = alpha channel of g
	// prod = e*f


	return pComb;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Virtual attributes for NV20 register combiners
|__________________________________________________________________________________________________
\*************************************************************************************************/

// -------------------------------------------------------------------
//  ATTENUATION
// -------------------------------------------------------------------
class CXR_VirtualAttributes_ShaderNV20Attenuation : public CXR_VirtualAttributes
{
public:
	static CRC_Attributes* ms_pBase;

	uint16 m_ShaderParamFlags;
	uint16 m_TextureID_Projection;
	fp32* m_pTexGenAttr;

	static void PrepareFrame(CXR_Shader* _pShader)
	{
		CRC_Attributes* pA = _pShader->m_pVBM->Alloc_Attrib();
		if (pA)
		{
			pA->SetDefault();
			pA->Attrib_TexGen(0, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);
			pA->Attrib_TexGen(1, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);

	#ifdef DST2_ATTENUATION
			pA->Attrib_TextureID(0, _pShader->m_TextureID_AttenuationDst2);
			pA->Attrib_TextureID(1, _pShader->m_TextureID_AttenuationDst2);
	#else
			pA->Attrib_TextureID(0, _pShader->m_TextureID_AttenuationExp);
			pA->Attrib_TextureID(1, _pShader->m_TextureID_AttenuationExp);
	#endif
			pA->Attrib_Enable(CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_CULL);
			pA->Attrib_Disable(CRC_FLAGS_COLORWRITE);
			pA->Attrib_ZCompare(CRC_COMPARE_LESSEQUAL);
		}
		ms_pBase = pA;
	}

	void Create(const CXR_ShaderParams* _pParams, int _TextureID_Projection, fp32* _pTexGenAttr)
	{
		m_pBaseAttrib = ms_pBase;
		m_Class = 4;
		m_ShaderParamFlags = _pParams->m_Flags & XR_SHADERFLAGS_USEZLESS;
		m_TextureID_Projection = _TextureID_Projection;
		m_pTexGenAttr = _pTexGenAttr;
	}

	int OnCompare(const CXR_VirtualAttributes* _pOther)
	{
		const CXR_VirtualAttributes_ShaderNV20Attenuation* pLast = (CXR_VirtualAttributes_ShaderNV20Attenuation*)_pOther;
		XR_COMPAREATTRIBUTE_INT(m_ShaderParamFlags, pLast->m_ShaderParamFlags);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Projection, pLast->m_TextureID_Projection);
		XR_COMPAREATTRIBUTE_INT(m_pTexGenAttr, pLast->m_pTexGenAttr);
		return 0;
	}

	uint OnSetAttributes(CRC_Attributes* _pDest, const CXR_VirtualAttributes* _pLastAttr)
	{
		// If we ran out of VBM during rendering then we can't trust this data
		if(!m_pBaseAttrib)
			return 0;

		int AttrChg = 0;
		if (!_pLastAttr || _pLastAttr->m_Class != m_Class)
		{
			*_pDest = *m_pBaseAttrib;
			AttrChg = -1;

			_pDest->Attrib_TexGenAttr(m_pTexGenAttr);
			if (m_TextureID_Projection)
			{
				_pDest->m_TextureID[2] = m_TextureID_Projection;
				_pDest->Attrib_TexGen(2, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V | CRC_TEXGENCOMP_W);
				_pDest->m_pExtAttrib = &m_StaticExattr_Shading_Proj;
			}
			else
			{
				_pDest->m_TextureID[2] = 0;
				_pDest->Attrib_TexGen(2, CRC_TEXGENMODE_TEXCOORD, 0);
				_pDest->m_pExtAttrib = &m_StaticExattr_Shading;
			}
			_pDest->m_ZCompare = (m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;
		}
		else
		{
			CXR_VirtualAttributes_ShaderNV20Attenuation *pLast = (CXR_VirtualAttributes_ShaderNV20Attenuation *)_pLastAttr;

			_pDest->Attrib_TexGenAttr(m_pTexGenAttr);
			AttrChg |= CRC_ATTRCHG_TEXGENATTR;

			if ((m_TextureID_Projection != 0) != (pLast->m_TextureID_Projection != 0))
			{
				if (m_TextureID_Projection)
				{
					_pDest->Attrib_TexGen(2, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V | CRC_TEXGENCOMP_W);
					_pDest->m_pExtAttrib = &m_StaticExattr_Shading_Proj;
				}
				else
				{
					_pDest->Attrib_TexGen(2, CRC_TEXGENMODE_VOID, 0);
					_pDest->m_pExtAttrib = &m_StaticExattr_Shading;
				}
				AttrChg |= CRC_ATTRCHG_TEXGENCOMP | CRC_ATTRCHG_TEXGEN | CRC_ATTRCHG_EXATTR;
			}

			if (m_TextureID_Projection != pLast->m_TextureID_Projection)
			{
				_pDest->m_TextureID[2] = m_TextureID_Projection;
				AttrChg |= CRC_ATTRCHG_TEXTUREID;
			}

			if ((m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) != (pLast->m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS))
			{
				_pDest->m_ZCompare = (m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;
				AttrChg |= CRC_ATTRCHG_ZCOMPARE;
			}
		}

		return AttrChg;
	}
};

CRC_Attributes* CXR_VirtualAttributes_ShaderNV20Attenuation::ms_pBase = NULL;

// -------------------------------------------------------------------
//  DIFFUSE
// -------------------------------------------------------------------
class CXR_VirtualAttributes_ShaderNV20Diffuse : public CXR_VirtualAttributes
{
public:
	static CRC_Attributes* ms_pBase;
	static uint16 ms_TextureID_BackBuffer;

	uint16 m_TextureID_Normal;
	uint16 m_TextureID_Diffuse;
	uint8 m_iTexCoordSetMapping;
	uint8 m_iTexCoordSetTangentU;
	uint8 m_iTexCoordSetTangentV;
	uint8 m_Flags;
	fp32* m_pTexGenAttr;

	enum
	{
		FLAGS_STENCILTEST = 1,
		FLAGS_SPECDIFFONEPASS = 2,
		FLAGS_USEZLESS = 4,
	};

	static void PrepareFrame(CXR_Shader* _pShader)
	{
		CRC_Attributes* pA = _pShader->m_pVBM->Alloc_Attrib();
		if (pA)
		{
			pA->SetDefault();
	#ifdef XR_DEFAULTPOLYOFFSETSCALE
			pA->Attrib_PolygonOffset(XR_DEFAULTPOLYOFFSETSCALE, XR_DEFAULTPOLYOFFSET);
	#endif
			pA->Attrib_TexGen(2, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_ALL);
			pA->Attrib_TextureID(2, _pShader->m_TextureID_Normalize);
			pA->Attrib_Enable(CRC_FLAGS_CULL | CRC_FLAGS_COLORWRITE);
			pA->Attrib_RasterMode(CRC_RASTERMODE_NONE);
			pA->Attrib_StencilRef(128, 255);
			pA->Attrib_StencilFrontOp(CRC_COMPARE_LESSEQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
			pA->Attrib_StencilWriteMask(0);
		}
		ms_pBase = pA;
		ms_TextureID_BackBuffer = _pShader->ms_TextureID_BackBuffer;
	}

	void Create(const CXR_ShaderParams* _pParams)
	{
		m_pBaseAttrib = ms_pBase;
		m_Class = 5;
		m_iTexCoordSetMapping = _pParams->m_iTexCoordSetMapping;
		m_iTexCoordSetTangentU = _pParams->m_iTexCoordSetTangentU;
		m_iTexCoordSetTangentV = _pParams->m_iTexCoordSetTangentV;
	}

	int OnCompare(const CXR_VirtualAttributes* _pOther)
	{
		const CXR_VirtualAttributes_ShaderNV20Diffuse* pLast = (CXR_VirtualAttributes_ShaderNV20Diffuse*)_pOther;
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Normal, pLast->m_TextureID_Normal);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Diffuse, pLast->m_TextureID_Diffuse);
		XR_COMPAREATTRIBUTE_INT(m_Flags, pLast->m_Flags);
		XR_COMPAREATTRIBUTE_INT(m_iTexCoordSetMapping, pLast->m_iTexCoordSetMapping);
		XR_COMPAREATTRIBUTE_INT(m_iTexCoordSetTangentU, pLast->m_iTexCoordSetTangentU);
		XR_COMPAREATTRIBUTE_INT(m_iTexCoordSetTangentV, pLast->m_iTexCoordSetTangentV);
		XR_COMPAREATTRIBUTE_INT(m_pTexGenAttr, pLast->m_pTexGenAttr);
		return 0;
	}

	uint OnSetAttributes(CRC_Attributes* _pDest, const CXR_VirtualAttributes* _pLastAttr)
	{
		// If we ran out of VBM during rendering then we can't trust this data
		if(!m_pBaseAttrib)
			return 0;

		int AttrChg = 0;
		if (!_pLastAttr || _pLastAttr->m_Class != m_Class)
		{
			*_pDest = *m_pBaseAttrib;
			AttrChg = -1;

			_pDest->m_iTexCoordSet[0] = m_iTexCoordSetMapping;
			_pDest->m_iTexCoordSet[1] = m_iTexCoordSetMapping;
			_pDest->m_iTexCoordSet[2] = m_iTexCoordSetTangentU;
			_pDest->m_iTexCoordSet[3] = m_iTexCoordSetTangentV;

			_pDest->Attrib_TexGenAttr(m_pTexGenAttr);

			_pDest->m_TextureID[0] = m_TextureID_Diffuse;
			_pDest->m_TextureID[1] = m_TextureID_Normal;

			int ZCompare = (m_Flags & FLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;
			_pDest->m_ZCompare = ZCompare;

			if (m_Flags & FLAGS_STENCILTEST)
				_pDest->Attrib_Enable(CRC_FLAGS_STENCIL);
			else
				_pDest->Attrib_Disable(CRC_FLAGS_STENCIL);

			if (m_Flags & FLAGS_SPECDIFFONEPASS)
			{
				_pDest->Attrib_TextureID(3, ms_TextureID_BackBuffer);
				_pDest->Attrib_TexGen(3, CRC_TEXGENMODE_SCREEN, CRC_TEXGENCOMP_ALL);
				_pDest->m_pExtAttrib = &m_StaticExattr_DotProd_DiffuseSpecularAlpha;

				_pDest->Attrib_RasterMode(CRC_RASTERMODE_NONE);
				_pDest->Attrib_Enable(CRC_FLAGS_ALPHAWRITE);
				_pDest->Attrib_Disable(CRC_FLAGS_BLEND);
			}
			else
			{
				_pDest->Attrib_TextureID(3, 0);
				_pDest->Attrib_TexGen(3, CRC_TEXGENMODE_VOID, 0);
				_pDest->m_pExtAttrib = &m_StaticExattr_DotProd_Diffuse;

				_pDest->Attrib_SourceBlend(CRC_BLEND_DESTALPHA);
				_pDest->Attrib_DestBlend(CRC_BLEND_ONE);
				_pDest->Attrib_Enable(CRC_FLAGS_BLEND);
				_pDest->Attrib_Disable(CRC_FLAGS_ALPHAWRITE);
			}
		}
		else
		{
			CXR_VirtualAttributes_ShaderNV20Diffuse *pLast = (CXR_VirtualAttributes_ShaderNV20Diffuse *)_pLastAttr;

			if (m_iTexCoordSetMapping != pLast->m_iTexCoordSetMapping)
			{
				_pDest->m_iTexCoordSet[0] = m_iTexCoordSetMapping;
				_pDest->m_iTexCoordSet[1] = m_iTexCoordSetMapping;
				AttrChg |= CRC_ATTRCHG_TEXCOORDSET;
			}
			if (m_iTexCoordSetTangentU != pLast->m_iTexCoordSetTangentU)
			{
				_pDest->m_iTexCoordSet[2] = m_iTexCoordSetTangentU;
				AttrChg |= CRC_ATTRCHG_TEXCOORDSET;
			}
			if (m_iTexCoordSetTangentV != pLast->m_iTexCoordSetTangentV)
			{
				_pDest->m_iTexCoordSet[3] = m_iTexCoordSetTangentV;
				AttrChg |= CRC_ATTRCHG_TEXCOORDSET;
			}

			_pDest->Attrib_TexGenAttr(m_pTexGenAttr);
			AttrChg |= CRC_ATTRCHG_TEXGENATTR;

			if (m_TextureID_Diffuse != pLast->m_TextureID_Diffuse)
			{
				_pDest->m_TextureID[0] = m_TextureID_Diffuse;
				AttrChg |= CRC_ATTRCHG_TEXTUREID;
			}
			if (m_TextureID_Normal != pLast->m_TextureID_Normal)
			{
				_pDest->m_TextureID[1] = m_TextureID_Normal;
				AttrChg |= CRC_ATTRCHG_TEXTUREID;
			}

			if (pLast->m_Flags != m_Flags)
			{
				if ((pLast->m_Flags & FLAGS_USEZLESS) != (m_Flags & FLAGS_USEZLESS))
				{
					_pDest->m_ZCompare = (m_Flags & FLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;
					AttrChg |= CRC_ATTRCHG_ZCOMPARE;
				}

				if ((pLast->m_Flags & FLAGS_STENCILTEST) != (m_Flags & FLAGS_STENCILTEST))
				{
					if (m_Flags & FLAGS_STENCILTEST)
						_pDest->Attrib_Enable(CRC_FLAGS_STENCIL);
					else
						_pDest->Attrib_Disable(CRC_FLAGS_STENCIL);
					AttrChg |= CRC_ATTRCHG_FLAG_STENCIL;
				}

				if ((pLast->m_Flags & FLAGS_SPECDIFFONEPASS) != (m_Flags & FLAGS_SPECDIFFONEPASS))
				{
					if (m_Flags & FLAGS_SPECDIFFONEPASS)
					{
						_pDest->Attrib_TextureID(3, ms_TextureID_BackBuffer);
						_pDest->Attrib_TexGen(3, CRC_TEXGENMODE_SCREEN, CRC_TEXGENCOMP_ALL);
						_pDest->m_pExtAttrib = &m_StaticExattr_DotProd_DiffuseSpecularAlpha;

						_pDest->Attrib_Enable(CRC_FLAGS_ALPHAWRITE);
						_pDest->Attrib_Disable(CRC_FLAGS_BLEND);
					}
					else
					{
						_pDest->Attrib_TextureID(3, 0);
						_pDest->Attrib_TexGen(3, CRC_TEXGENMODE_VOID, 0);
						_pDest->m_pExtAttrib = &m_StaticExattr_DotProd_Diffuse;

						_pDest->Attrib_SourceBlend(CRC_BLEND_DESTALPHA);
						_pDest->Attrib_DestBlend(CRC_BLEND_ONE);
						_pDest->Attrib_Enable(CRC_FLAGS_BLEND);
						_pDest->Attrib_Disable(CRC_FLAGS_ALPHAWRITE);
					}
					AttrChg |= CRC_ATTRCHG_TEXTUREID | CRC_ATTRCHG_BLEND | CRC_ATTRCHG_FLAG_ALHPAWRITE | CRC_ATTRCHG_FLAG_BLEND | CRC_ATTRCHG_TEXGEN | CRC_ATTRCHG_EXATTR;
				}
			}
		}

		return AttrChg;
	}
};

CRC_Attributes* CXR_VirtualAttributes_ShaderNV20Diffuse::ms_pBase = NULL;
uint16 CXR_VirtualAttributes_ShaderNV20Diffuse::ms_TextureID_BackBuffer = 0;

// -------------------------------------------------------------------
//  SPECULAR MODE 0
// -------------------------------------------------------------------
class CXR_VirtualAttributes_ShaderNV20SpecularMode0: public CXR_VirtualAttributes
{
public:
	static CRC_Attributes* ms_pBase;

	uint16 m_TextureID_Normal;
	uint16 m_TextureID_SpecFunc;
	uint8 m_iTexCoordSetMapping;
	uint8 m_iTexCoordSetTangentU;
	uint8 m_iTexCoordSetTangentV;
	uint8 m_Flags;
	fp32* m_pTexGenAttr;

	enum
	{
		FLAGS_STENCILTEST = 1,
//		FLAGS_SPECDIFFONEPASS = 2,
		FLAGS_USEZLESS = 4,
		FLAGS_SPECDIFF = 8,
	};

	static void PrepareFrame(CXR_Shader* _pShader)
	{
		CRC_Attributes* pA = _pShader->m_pVBM->Alloc_Attrib();
		if (pA)
		{
			pA->SetDefault();
			pA->Attrib_SourceBlend(CRC_BLEND_DESTALPHA);
			pA->Attrib_DestBlend(CRC_BLEND_ONE);
			pA->Attrib_Enable(CRC_FLAGS_BLEND | CRC_FLAGS_CULL | CRC_FLAGS_COLORWRITE);
			pA->Attrib_Disable(CRC_FLAGS_ALPHAWRITE);
			pA->Attrib_StencilRef(128, 255);
			pA->Attrib_StencilFrontOp(CRC_COMPARE_LESSEQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
			pA->Attrib_StencilWriteMask(0);

	#ifdef XR_DEFAULTPOLYOFFSETSCALE
			pA->Attrib_PolygonOffset(XR_DEFAULTPOLYOFFSETSCALE, XR_DEFAULTPOLYOFFSET);
	#endif
			pA->Attrib_TexGen(1, CRC_TEXGENMODE_TSREFLECTION, CRC_TEXGENCOMP_ALL);
			pA->Attrib_TexGen(2, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);
			pA->Attrib_TexGen(3, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);
		}
		ms_pBase = pA;
	}

	void Create(const CXR_ShaderParams* _pParams)
	{
		m_pBaseAttrib	= ms_pBase;
		m_Class = 6;
		m_iTexCoordSetMapping = _pParams->m_iTexCoordSetMapping;
		m_iTexCoordSetTangentU = _pParams->m_iTexCoordSetTangentU;
		m_iTexCoordSetTangentV = _pParams->m_iTexCoordSetTangentV;
	}

	int OnCompare(const CXR_VirtualAttributes* _pOther)
	{
		const CXR_VirtualAttributes_ShaderNV20SpecularMode0* pLast = (CXR_VirtualAttributes_ShaderNV20SpecularMode0*)_pOther;
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Normal, pLast->m_TextureID_Normal);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_SpecFunc, pLast->m_TextureID_SpecFunc);
		XR_COMPAREATTRIBUTE_INT(m_Flags, pLast->m_Flags);
		XR_COMPAREATTRIBUTE_INT(m_iTexCoordSetMapping, pLast->m_iTexCoordSetMapping);
		XR_COMPAREATTRIBUTE_INT(m_iTexCoordSetTangentU, pLast->m_iTexCoordSetTangentU);
		XR_COMPAREATTRIBUTE_INT(m_iTexCoordSetTangentV, pLast->m_iTexCoordSetTangentV);
		XR_COMPAREATTRIBUTE_INT(m_pTexGenAttr, pLast->m_pTexGenAttr);
		return 0;
	}

	uint OnSetAttributes(CRC_Attributes* _pDest, const CXR_VirtualAttributes* _pLastAttr)
	{
		// If we ran out of VBM during rendering then we can't trust this data
		if(!m_pBaseAttrib)
			return 0;

		int AttrChg = 0;
		if (!_pLastAttr || _pLastAttr->m_Class != m_Class)
		{
			*_pDest = *m_pBaseAttrib;
			AttrChg = -1;

			_pDest->m_iTexCoordSet[0] = m_iTexCoordSetMapping;
			_pDest->m_iTexCoordSet[1] = m_iTexCoordSetMapping;
			_pDest->m_iTexCoordSet[2] = m_iTexCoordSetTangentU;
			_pDest->m_iTexCoordSet[3] = m_iTexCoordSetTangentV;

			_pDest->Attrib_TexGenAttr(m_pTexGenAttr);

			_pDest->m_TextureID[0] = m_TextureID_Normal;
			_pDest->m_TextureID[1] = m_TextureID_Normal;
			_pDest->m_TextureID[2] = m_TextureID_Normal;
			_pDest->m_TextureID[3] = m_TextureID_SpecFunc;

			int ZCompare = (m_Flags & FLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;
			_pDest->m_ZCompare = ZCompare;

			if (m_Flags & FLAGS_STENCILTEST)
				_pDest->Attrib_Enable(CRC_FLAGS_STENCIL);
			else
				_pDest->Attrib_Disable(CRC_FLAGS_STENCIL);

			if(m_Flags & FLAGS_SPECDIFF)
				_pDest->m_pExtAttrib = &m_StaticExattr_NMapHSpaceCubeMapNoSpec;
			else
				_pDest->m_pExtAttrib = &m_StaticExattr_NMapHSpaceCubeMap;
		}
		else
		{
			CXR_VirtualAttributes_ShaderNV20SpecularMode0 *pLast = (CXR_VirtualAttributes_ShaderNV20SpecularMode0 *)_pLastAttr;

			if (m_iTexCoordSetMapping != pLast->m_iTexCoordSetMapping)
			{
				_pDest->m_iTexCoordSet[0] = m_iTexCoordSetMapping;
				_pDest->m_iTexCoordSet[1] = m_iTexCoordSetMapping;
				AttrChg |= CRC_ATTRCHG_TEXCOORDSET;
			}
			if (m_iTexCoordSetTangentU != pLast->m_iTexCoordSetTangentU)
			{
				_pDest->m_iTexCoordSet[2] = m_iTexCoordSetTangentU;
				AttrChg |= CRC_ATTRCHG_TEXCOORDSET;
			}
			if (m_iTexCoordSetTangentV != pLast->m_iTexCoordSetTangentV)
			{
				_pDest->m_iTexCoordSet[3] = m_iTexCoordSetTangentV;
				AttrChg |= CRC_ATTRCHG_TEXCOORDSET;
			}

			_pDest->Attrib_TexGenAttr(m_pTexGenAttr);
			AttrChg |= CRC_ATTRCHG_TEXGENATTR;

			if (m_TextureID_Normal != pLast->m_TextureID_Normal)
			{
				_pDest->m_TextureID[0] = m_TextureID_Normal;
				_pDest->m_TextureID[1] = m_TextureID_Normal;
				_pDest->m_TextureID[2] = m_TextureID_Normal;
				AttrChg |= CRC_ATTRCHG_TEXTUREID;
			}
			if (m_TextureID_SpecFunc != pLast->m_TextureID_SpecFunc)
			{
				_pDest->m_TextureID[3] = m_TextureID_SpecFunc;
				AttrChg |= CRC_ATTRCHG_TEXTUREID;
			}

			if (pLast->m_Flags != m_Flags)
			{
				if ((pLast->m_Flags & FLAGS_USEZLESS) != (m_Flags & FLAGS_USEZLESS))
				{
					_pDest->m_ZCompare = (m_Flags & FLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;
					AttrChg |= CRC_ATTRCHG_ZCOMPARE;
				}

				if ((pLast->m_Flags & FLAGS_STENCILTEST) != (m_Flags & FLAGS_STENCILTEST))
				{
					if (m_Flags & FLAGS_STENCILTEST)
						_pDest->Attrib_Enable(CRC_FLAGS_STENCIL);
					else
						_pDest->Attrib_Disable(CRC_FLAGS_STENCIL);
					AttrChg |= CRC_ATTRCHG_FLAG_STENCIL;
				}

				if ((pLast->m_Flags & FLAGS_SPECDIFF) != (m_Flags & FLAGS_SPECDIFF))
				{
					if(m_Flags & FLAGS_SPECDIFF)
						_pDest->m_pExtAttrib = &m_StaticExattr_NMapHSpaceCubeMapNoSpec;
					else
						_pDest->m_pExtAttrib = &m_StaticExattr_NMapHSpaceCubeMap;

					AttrChg |= CRC_ATTRCHG_EXATTR;
				}
			}
		}


		return AttrChg | CRC_ATTRCHG_OTHER | CRC_ATTRCHG_FLAGS;
	}
};

CRC_Attributes* CXR_VirtualAttributes_ShaderNV20SpecularMode0::ms_pBase = NULL;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| NV20 Register Combiners shader using virtual attributes
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CXR_Shader::RenderShading_NV20_VA(const CXR_Light& _Light, CXR_VertexBuffer* _pVB, const CXR_ShaderParams* _pShaderParams)
{
	MAUTOSTRIP(CXR_Shader_RenderShading_NV20_VA, MAUTOSTRIP_VOID);

	int StencilTest = (_pShaderParams->m_Flags & XR_SHADERFLAGS_NOSTENCILTEST) ? 0 : CRC_FLAGS_STENCIL;
//	int ZCompare = (_pShaderParams->m_Flags & XR_SHADERFLAGS_USEZEQUAL) ? CRC_COMPARE_EQUAL : CRC_COMPARE_LESSEQUAL;
	int ZCompare = _pShaderParams->m_Flags & XR_SHADERFLAGS_USEZLESS ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;

	int TextureID_Normal = (_pShaderParams->m_lTextureIDs[XR_SHADERMAP_NORMAL]) ? _pShaderParams->m_lTextureIDs[XR_SHADERMAP_NORMAL] : m_TextureID_DefaultNormal;

	bool bSpecularDiffuse = (_pShaderParams->m_Flags & XR_SHADERFLAGS_SPECULARINDIFFUSE) != 0;
	bool bSpecalarEnabled = /*m_SpecularScale &&*/ !(_Light.m_Flags  & CXR_LIGHT_NOSPECULAR);
	bool bSpecularDiffuseOnePass = bSpecularDiffuse && m_CanGetBackbuffer && bSpecalarEnabled;

	/* -------------------------------------------------------------------
		-Pass 1:
			Texture0 = ExpAttenuation 2D texture
			Texture1 = ExpAttenuation 2D texture
			Texture2 = [ProjectionMap]
			Texture3 = [LightMask]

			FB.Alpha = Attenuation [* ProjectionMap] [* LightMask]
	 ------------------------------------------------------------------- */
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VB();
		if (!pVB) return;

		// Copy geometry
		pVB->CopyVBChain(_pVB);

		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

		int TextureIDProj = _Light.m_ProjMapID;
		if (!TextureIDProj && _Light.m_Type == CXR_LIGHTTYPE_SPOT)
			TextureIDProj = m_TextureID_DefaultLens;

		// Texgen for attenuation
		fp32 Range = _Light.m_Range;
		fp32 Offset = 0.5f;

		const CVec3Dfp32& Pos= _Light.GetPosition();

		int nTexGenAttr = 4 * 4;
		if (TextureIDProj)
			nTexGenAttr += 3 * 4;

		CPlane3Dfp32* pTexGenAttr = (CPlane3Dfp32*) m_pVBM->Alloc_fp32(nTexGenAttr);
		if (!pTexGenAttr)
			return;
		pTexGenAttr[0].CreateND(CVec3Dfp32(0.5f / Range,0,0), Offset - 0.5f* Pos[0] / Range);
		pTexGenAttr[1].CreateND(CVec3Dfp32(0, 0.5f / Range,0), Offset - 0.5f* Pos[1] / Range);
		pTexGenAttr[2].CreateND(CVec3Dfp32(0, 0, 0.5f / Range), Offset - 0.5f* Pos[2] / Range);
		pTexGenAttr[3].CreateND(CVec3Dfp32(0, 0, 0), Offset);

		if (TextureIDProj)
		{
			const CMat4Dfp32* pProjPos = (_Light.m_Flags & CXR_LIGHT_PROJMAPTRANSFORM) ? 
				&_Light.m_ProjMapTransform : &_Light.m_Pos;

			const CVec3Dfp32& ProjX = CVec3Dfp32::GetRow(*pProjPos, 0);
			CVec3Dfp32 ProjY; CVec3Dfp32::GetRow(*pProjPos, 1).Scale(1.0f / _Light.m_SpotWidth, ProjY);
			CVec3Dfp32 ProjZ; CVec3Dfp32::GetRow(*pProjPos, 2).Scale(1.0f / _Light.m_SpotHeight, ProjZ);
			const CVec3Dfp32& Pos = CVec3Dfp32::GetRow(*pProjPos, 3);

			pTexGenAttr[4].CreateND(ProjX, -(ProjX * Pos));
			pTexGenAttr[5].CreateND(ProjY, -(ProjY * Pos));
			pTexGenAttr[6].CreateND(ProjZ, -(ProjZ * Pos));
		}


		pVB->m_Priority = _Light.m_iLight + 0.3f;

		void* pVAMem = m_pVBM->Alloc(sizeof(CXR_VirtualAttributes_ShaderNV20Attenuation));
		if(!pVAMem)
			return;

		CXR_VirtualAttributes_ShaderNV20Attenuation* pVA = new(pVAMem) CXR_VirtualAttributes_ShaderNV20Attenuation;

		pVA->Create(_pShaderParams, TextureIDProj, (fp32*)pTexGenAttr);
		pVB->SetVirtualAttr(pVA);

		pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
		pVB->m_iLight = _Light.m_iLight;
		m_pVBM->AddVB(pVB);
	}

	/* -------------------------------------------------------------------
		-Pass 2:
			Texture0 = Diffuse texture
			Texture1 = Normal map
			Texture2 = Lightvector normalization map
			Texture3 = Diffuse texture2 (not impl)

			FB.Color += FB.Alpha * [DiffuseTexture *] [DiffuseTexture2 *] (NormalMap dot LightVector)
	------------------------------------------------------------------- */

//	if (!bSpecularDiffuse)
	if (/*m_DiffuseScale &&*/ !(_Light.m_Flags & CXR_LIGHT_NODIFFUSE))
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VB();
		if (!pVB) 
			return;

		// Copy geometry
		pVB->CopyVBChain(_pVB);

		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

		CVec4Dfp32* pLightPos = m_pVBM->Alloc_V4(1);
		if (!pLightPos)
			return;
		*((CVec3Dfp32*)pLightPos) = _Light.GetPosition();
		pLightPos->k[3] = 1.0f;

		CVec4Dfp32 DiffuseColor;
		DiffuseColor << _pShaderParams->m_DiffuseColor;
		DiffuseColor[0] *= _Light.m_Intensity[0] * m_DiffuseScale[0];
		DiffuseColor[1] *= _Light.m_Intensity[1] * m_DiffuseScale[1];
		DiffuseColor[2] *= _Light.m_Intensity[2] * m_DiffuseScale[2];

		CPixel32 DiffuseColor32(RoundToInt(DiffuseColor[0] * 255), RoundToInt(DiffuseColor[1] * 255), RoundToInt(DiffuseColor[2] * 255), 0);
		pVB->m_Color = DiffuseColor32;


		void* pVAMem = m_pVBM->Alloc(sizeof(CXR_VirtualAttributes_ShaderNV20Diffuse));
		if(!pVAMem)
			return;

		CXR_VirtualAttributes_ShaderNV20Diffuse* pVA = new(pVAMem) CXR_VirtualAttributes_ShaderNV20Diffuse;

		pVA->Create(_pShaderParams);
		pVA->m_TextureID_Diffuse = _pShaderParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE];
		pVA->m_TextureID_Normal = TextureID_Normal;
		pVA->m_pTexGenAttr = (fp32*)pLightPos;
		int Flags = 0;
		if (StencilTest)
			Flags |= CXR_VirtualAttributes_ShaderNV20Diffuse::FLAGS_STENCILTEST;
		if (bSpecularDiffuseOnePass)
			Flags |= CXR_VirtualAttributes_ShaderNV20Diffuse::FLAGS_SPECDIFFONEPASS;
		if (_pShaderParams->m_Flags & XR_SHADERFLAGS_USEZLESS)
			Flags |= CXR_VirtualAttributes_ShaderNV20Diffuse::FLAGS_USEZLESS;
		pVA->m_Flags = Flags;
		pVB->SetVirtualAttr(pVA);


		pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
		pVB->m_iLight = _Light.m_iLight;
		pVB->m_Priority = _Light.m_iLight + 0.31f;
		m_pVBM->AddVB(pVB);
	}

	/* -------------------------------------------------------------------
		-Pass 2b:
			Texture0 = Diffuse texture

			FB.Alpha += FB.Alpha * DiffuseTexture.Alpha
	------------------------------------------------------------------- */

/*	if (bSpecularDiffuse)
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VBAttrib();
		if (!pVB) return;

		pVB->m_pAttrib->m_iTexCoordSet[0] = _pShaderParams->m_iTexCoordSetMapping;
		pVB->m_pAttrib->m_iTexCoordSet[1] = _pShaderParams->m_iTexCoordSetMapping;
		pVB->m_pAttrib->m_iTexCoordSet[2] = _pShaderParams->m_iTexCoordSetTangentU;
		pVB->m_pAttrib->m_iTexCoordSet[3] = _pShaderParams->m_iTexCoordSetTangentV;

		// Copy geometry
		pVB->CopyVBChain(_pVB);

		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

		pVB->m_Priority = _Light.m_iLight + 0.315f + 100;
		pVB->m_pAttrib->Attrib_TextureID(0, ms_TextureID_BackBuffer);
		pVB->m_pAttrib->Attrib_TexGen(0, CRC_TEXGENMODE_SCREEN, CRC_TEXGENCOMP_ALL);

		uint32 iShader = _Light.m_iLight * m_nLightShaders + 3;

		pVB->m_pAttrib->Attrib_Enable(CRC_FLAGS_CULL | CRC_FLAGS_COLORWRITE | CRC_FLAGS_ALPHAWRITE);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_BLEND );
		pVB->m_pAttrib->Attrib_ZCompare(CRC_COMPARE_ALWAYS);

		pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
		pVB->m_iLight = _Light.m_iLight;
		m_pVBM->AddVB(pVB);
	}*/

//	if (!bSpecularDiffuse)
	if (bSpecularDiffuse && !bSpecularDiffuseOnePass && bSpecalarEnabled)
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VBAttrib();
		if (!pVB) return;

		pVB->m_pAttrib->m_iTexCoordSet[0] = _pShaderParams->m_iTexCoordSetMapping;
		pVB->m_pAttrib->m_iTexCoordSet[1] = _pShaderParams->m_iTexCoordSetMapping;
		pVB->m_pAttrib->m_iTexCoordSet[2] = _pShaderParams->m_iTexCoordSetTangentU;
		pVB->m_pAttrib->m_iTexCoordSet[3] = _pShaderParams->m_iTexCoordSetTangentV;

		// Copy geometry
		pVB->CopyVBChain(_pVB);

		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

		pVB->m_Priority = _Light.m_iLight + 0.315f;
		pVB->m_pAttrib->Attrib_TextureID(0, _pShaderParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE]);

//		uint32 iShader = _Light.m_iLight * m_nLightShaders + 3;

		pVB->m_pAttrib->Attrib_SourceBlend(CRC_BLEND_DESTALPHA);
		pVB->m_pAttrib->Attrib_DestBlend(CRC_BLEND_ZERO);
		pVB->m_pAttrib->Attrib_Enable(CRC_FLAGS_BLEND | CRC_FLAGS_CULL | CRC_FLAGS_ALPHAWRITE | StencilTest);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_COLORWRITE);
		pVB->m_pAttrib->Attrib_ZCompare(ZCompare);

		if (StencilTest)
		{
			pVB->m_pAttrib->Attrib_StencilRef(128, 255);
			pVB->m_pAttrib->Attrib_StencilFrontOp(CRC_COMPARE_LESSEQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
			pVB->m_pAttrib->Attrib_StencilWriteMask(0);
		}

		pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
		pVB->m_iLight = _Light.m_iLight;
		m_pVBM->AddVB(pVB);
	}
	
	/* -------------------------------------------------------------------
		-Pass 3a:
  			Texture0 = SpecularMap
			Texture1 = Normal map
			Texture2 = HalfAngle normalization map (For specular)
  			Texture3 = EyeVector normalization map (For fresnel)

			FB.Color += FB.Alpha * [SpecularTexture *] (NormalMap dot HalfAngle)^k [* Fresnel]
	------------------------------------------------------------------- */
//	if (!bSpecularDiffuse)
	if (m_SpecularMode == 1 && /*m_SpecularScale &&*/ !(_Light.m_Flags  & CXR_LIGHT_NOSPECULAR))
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VBAttrib();
		if (!pVB) return;

		pVB->m_pAttrib->m_iTexCoordSet[0] = _pShaderParams->m_iTexCoordSetMapping;
		pVB->m_pAttrib->m_iTexCoordSet[1] = _pShaderParams->m_iTexCoordSetMapping;
		pVB->m_pAttrib->m_iTexCoordSet[2] = _pShaderParams->m_iTexCoordSetTangentU;
		pVB->m_pAttrib->m_iTexCoordSet[3] = _pShaderParams->m_iTexCoordSetTangentV;

		// Copy geometry
		pVB->CopyVBChain(_pVB);

		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

		const CMat4Dfp32* pMat = _pVB->m_pTransform;
		CVec4Dfp32* pLightVec = m_pVBM->Alloc_V4(2);
		if (!pLightVec)
			return;
		CVec3Dfp32 Pos = _Light.GetPosition();
		pLightVec[0][0] = Pos[0];
		pLightVec[0][1] = Pos[1];
		pLightVec[0][2] = Pos[2];
		pLightVec[0][3] = 1;
		for (int j = 0; j < 3; j++)
			pLightVec[1][j] = - (pMat->k[3][0]*pMat->k[j][0] + pMat->k[3][1]*pMat->k[j][1] + pMat->k[3][2]*pMat->k[j][2]);
		pLightVec[1][3] = 1;
//		pVB->m_pAttrib->Attrib_TexGenU(2, CRC_TEXGENMODE_TSREFLECTION, (fp32*)pLightVec);

		pVB->m_pAttrib->Attrib_TexGen(2, CRC_TEXGENMODE_TSREFLECTION, CRC_TEXGENCOMP_ALL);
		pVB->m_pAttrib->Attrib_TexGenAttr((fp32*)pLightVec);
#ifdef XR_DEFAULTPOLYOFFSETSCALE
		pVB->m_pAttrib->Attrib_PolygonOffset(XR_DEFAULTPOLYOFFSETSCALE, XR_DEFAULTPOLYOFFSET);
#endif

		pVB->m_Priority = _Light.m_iLight + 0.32f;
		pVB->m_pAttrib->Attrib_TextureID(0, TextureID_Normal);
		pVB->m_pAttrib->Attrib_TextureID(1, TextureID_Normal);
		pVB->m_pAttrib->Attrib_TextureID(2, m_TextureID_FlipNormalize);
//		pVB->m_pAttrib->Attrib_TextureID(3, _pShaderParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE2]);
		pVB->m_pAttrib->Attrib_TexCoordSet(1, 0);

		CVec4Dfp32 SpecularColor;
		SpecularColor << _pShaderParams->m_SpecularColor;
		SpecularColor[0] *= _Light.m_Intensity[0] * m_SpecularScale[0];
		SpecularColor[1] *= _Light.m_Intensity[1] * m_SpecularScale[1];
		SpecularColor[2] *= _Light.m_Intensity[2] * m_SpecularScale[2];

		CPixel32 SpecularColor32(RoundToInt(SpecularColor[0] * 255), RoundToInt(SpecularColor[1] * 255), RoundToInt(SpecularColor[2] * 255), 0);

		int Power = (m_SpecularForcePower) ? m_SpecularForcePower : _pShaderParams->m_SpecularPower;
#		ifdef SUPPORT_REGISTERCOMBINERS
#			ifdef SUPPORT_FRAGMENTPROGRAM
				if (m_ShaderMode  == XR_SHADERMODE_REGCOMBINERS_NV20)
#			endif
			{
				CRC_ExtAttributes_NV10* pComb = RegCombiners_NV20_DotProd_Specular(Power);
				if (!pComb) return;
				pComb->SetConst0(SpecularColor32);
		//		pComb->SetConst0(((_Light.m_IntensityInt32 * _pShaderParams->m_SpecularColor) * m_SpecularScale) * 512);
				pComb->Fixup();
				pVB->m_pAttrib->m_pExtAttrib = pComb;
			}
#		ifdef SUPPORT_FRAGMENTPROGRAM
			else 
#		endif
#		endif
#		ifdef SUPPORT_FRAGMENTPROGRAM
			if (m_ShaderMode == XR_SHADERMODE_FRAGMENTPROGRAM11)
			{
				CRC_ExtAttributes_FragmentProgram11 *pComb = (CRC_ExtAttributes_FragmentProgram11 *) m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram11));
				if (!pComb) return;
				int SpecLog2 = 0;
				while((1 << SpecLog2) < Power)
				{
					SpecLog2++;
				}
				pComb->Clear();
				pComb->m_Colors[0] = SpecularColor32;
				pComb->m_Colors[1] = 0;
				switch (SpecLog2)
				{
				case 0: pComb->m_pProgramName = "XRShader_DotProd_Specular0"; break; case 1: pComb->m_pProgramName = "XRShader_DotProd_Specular1"; break;
				case 2: pComb->m_pProgramName = "XRShader_DotProd_Specular2"; break; case 3: pComb->m_pProgramName = "XRShader_DotProd_Specular3"; break;
				case 4: pComb->m_pProgramName = "XRShader_DotProd_Specular4"; break; 
				case 5: pComb->m_pProgramName = "XRShader_DotProd_Specular5"; pComb->m_Colors[1] = 0xff808080; break;
				default: pComb->m_pProgramName = "XRShader_DotProd_Specular6"; pComb->m_Colors[1] = 0xffc0c0c0; break;
				}
				pVB->m_pAttrib->m_pExtAttrib = pComb;
			}
#		endif


		pVB->m_pAttrib->Attrib_SourceBlend(CRC_BLEND_DESTALPHA);
		pVB->m_pAttrib->Attrib_DestBlend(CRC_BLEND_ONE);
		pVB->m_pAttrib->Attrib_Enable(CRC_FLAGS_BLEND | CRC_FLAGS_CULL | CRC_FLAGS_COLORWRITE | StencilTest);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ALPHAWRITE);
		pVB->m_pAttrib->Attrib_ZCompare(ZCompare);

		if (StencilTest)
		{
			pVB->m_pAttrib->Attrib_StencilRef(128, 255);
			pVB->m_pAttrib->Attrib_StencilFrontOp(CRC_COMPARE_LESSEQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
			pVB->m_pAttrib->Attrib_StencilWriteMask(0);
		}

		pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
		pVB->m_iLight = _Light.m_iLight;
		m_pVBM->AddVB(pVB);
	}


	/* -------------------------------------------------------------------
		-Pass 3b:
  			Texture0 = Normal map
			Texture1 = Normal map
			Texture2 = Normal map
  			Texture3 = Half angle space specular cubemap

			FB.Color += FB.Alpha * [NormalMap.alpha *] (NormalMap dot HalfAngle)^k
	------------------------------------------------------------------- */
	else if (m_SpecularMode == 0 && /*m_SpecularScale &&*/ !(_Light.m_Flags  & CXR_LIGHT_NOSPECULAR))
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VB();
		if (!pVB) return;

		// Copy geometry
		pVB->CopyVBChain(_pVB);
		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

/*		CVec3Dfp32* pLightVec = m_pVBM->Alloc_V3(1);
		if (!pLightVec)
			return;
		*pLightVec = _Light.m_TransformedPos;
		pVB->m_pAttrib->Attrib_TexGenU(2, CRC_TEXGENMODE_TSLV, (fp32*)pLightVec);
*/
		const CMat4Dfp32* pMat = _pVB->m_pTransform;
		CVec4Dfp32* pLightVec = m_pVBM->Alloc_V4(2);
		if (!pLightVec)
			return;
		CVec3Dfp32 Pos = _Light.GetPosition();
		pLightVec[0][0] = Pos[0];
		pLightVec[0][1] = Pos[1];
		pLightVec[0][2] = Pos[2];
		pLightVec[0][3] = 1;
		for (int j = 0; j < 3; j++)
			pLightVec[1][j] = - (pMat->k[3][0]*pMat->k[j][0] + pMat->k[3][1]*pMat->k[j][1] + pMat->k[3][2]*pMat->k[j][2]);
		pLightVec[1][3] = 1;

		int iSpecMap = 0;
		int Power = (m_SpecularForcePower) ? m_SpecularForcePower : _pShaderParams->m_SpecularPower;
		while((1 << iSpecMap) < Power)
		{
			iSpecMap++;
		}

		if (iSpecMap > 7)
			iSpecMap = 7;

		int TextureIDSpecFunc = m_lTextureID_Specular[iSpecMap];

		CVec4Dfp32 SpecularColor;
		SpecularColor << _pShaderParams->m_SpecularColor;
		SpecularColor[0] *= _Light.m_Intensity[0] * m_SpecularScale[0];
		SpecularColor[1] *= _Light.m_Intensity[1] * m_SpecularScale[1];
		SpecularColor[2] *= _Light.m_Intensity[2] * m_SpecularScale[2];
		CPixel32 SpecularColor32(RoundToInt(SpecularColor[0] * 255), RoundToInt(SpecularColor[1] * 255), RoundToInt(SpecularColor[2] * 255), 0);

		pVB->m_Color = SpecularColor32;

		void* pVAMem = m_pVBM->Alloc(sizeof(CXR_VirtualAttributes_ShaderNV20SpecularMode0));
		if(!pVAMem)
			return;

		CXR_VirtualAttributes_ShaderNV20SpecularMode0* pVA = new(pVAMem) CXR_VirtualAttributes_ShaderNV20SpecularMode0;

		pVA->Create(_pShaderParams);
		pVA->m_TextureID_SpecFunc = TextureIDSpecFunc;
		pVA->m_TextureID_Normal = TextureID_Normal;
		pVA->m_pTexGenAttr = (fp32*)pLightVec;
		int Flags = 0;
		if (StencilTest)
			Flags |= CXR_VirtualAttributes_ShaderNV20SpecularMode0::FLAGS_STENCILTEST;
//		if (bSpecularDiffuseOnePass)
//			Flags |= CXR_VirtualAttributes_ShaderNV20SpecularMode0::FLAGS_SPECDIFFONEPASS;
		if (_pShaderParams->m_Flags & XR_SHADERFLAGS_USEZLESS)
			Flags |= CXR_VirtualAttributes_ShaderNV20SpecularMode0::FLAGS_USEZLESS;
		if (bSpecularDiffuse)
			Flags |= CXR_VirtualAttributes_ShaderNV20SpecularMode0::FLAGS_SPECDIFF;
		pVA->m_Flags = Flags;
		pVB->SetVirtualAttr(pVA);


		pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
		pVB->m_iLight = _Light.m_iLight;
		pVB->m_Priority = _Light.m_iLight + 0.32f;
		m_pVBM->AddVB(pVB);
	}
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| NV20 Register Combiners and pixel shaders
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CXR_Shader::RenderShading_NV20(const CXR_Light& _Light, CXR_VertexBuffer* _pVB, const CXR_ShaderParams* _pShaderParams)
{
	MAUTOSTRIP(CXR_Shader_RenderShading_NV20, MAUTOSTRIP_VOID);

	int StencilTest = (_pShaderParams->m_Flags & XR_SHADERFLAGS_NOSTENCILTEST) ? 0 : CRC_FLAGS_STENCIL;
//	int ZCompare = (_pShaderParams->m_Flags & XR_SHADERFLAGS_USEZEQUAL) ? CRC_COMPARE_EQUAL : CRC_COMPARE_LESSEQUAL;
	int ZCompare = _pShaderParams->m_Flags&XR_SHADERFLAGS_USEZLESS ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;

	int TextureID_Normal = (_pShaderParams->m_lTextureIDs[XR_SHADERMAP_NORMAL]) ? _pShaderParams->m_lTextureIDs[XR_SHADERMAP_NORMAL] : m_TextureID_DefaultNormal;

	bool bSpecularDiffuse = (_pShaderParams->m_Flags & XR_SHADERFLAGS_SPECULARINDIFFUSE) != 0;
	bool bSpecalarEnabled = /*m_SpecularScale &&*/ !(_Light.m_Flags  & CXR_LIGHT_NOSPECULAR);
	bool bSpecularDiffuseOnePass = bSpecularDiffuse && m_CanGetBackbuffer && bSpecalarEnabled;

	/* -------------------------------------------------------------------
		-Pass 1:
			Texture0 = ExpAttenuation 2D texture
			Texture1 = ExpAttenuation 2D texture
			Texture2 = [ProjectionMap]
			Texture3 = [LightMask]

			FB.Alpha = Attenuation [* ProjectionMap] [* LightMask]
	 ------------------------------------------------------------------- */
//	if (!bSpecularDiffuse)
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VBAttrib();
		if (!pVB) return;

		pVB->m_pAttrib->m_iTexCoordSet[0] = _pShaderParams->m_iTexCoordSetMapping;
		pVB->m_pAttrib->m_iTexCoordSet[1] = _pShaderParams->m_iTexCoordSetMapping;
		pVB->m_pAttrib->m_iTexCoordSet[2] = _pShaderParams->m_iTexCoordSetTangentU;
		pVB->m_pAttrib->m_iTexCoordSet[3] = _pShaderParams->m_iTexCoordSetTangentV;

		// Copy geometry
		pVB->CopyVBChain(_pVB);

		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

		int TextureIDProj = _Light.m_ProjMapID;
		if (!TextureIDProj && _Light.m_Type == CXR_LIGHTTYPE_SPOT)
			TextureIDProj = m_TextureID_DefaultLens;

		// Texgen for attenuation
		fp32 Range = _Light.m_Range;
		fp32 Offset = 0.5f;

		const CVec3Dfp32& Pos= _Light.GetPosition();

		int nTexGenAttr = 4 * 4;
		if (TextureIDProj)
			nTexGenAttr += 3 * 4;

		CPlane3Dfp32* pTexGenAttr = (CPlane3Dfp32*) m_pVBM->Alloc_fp32(nTexGenAttr);
		if (!pTexGenAttr)
			return;
		pTexGenAttr[0].CreateND(CVec3Dfp32(0.5f / Range,0,0), Offset - 0.5f* Pos[0] / Range);
		pTexGenAttr[1].CreateND(CVec3Dfp32(0, 0.5f / Range,0), Offset - 0.5f* Pos[1] / Range);
		pTexGenAttr[2].CreateND(CVec3Dfp32(0, 0, 0.5f / Range), Offset - 0.5f* Pos[2] / Range);
		pTexGenAttr[3].CreateND(CVec3Dfp32(0, 0, 0), Offset);
		pVB->m_pAttrib->Attrib_TexGen(0, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);
		pVB->m_pAttrib->Attrib_TexGen(1, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);
		pVB->m_pAttrib->Attrib_TexGenAttr((fp32*)pTexGenAttr);
#ifdef XR_DEFAULTPOLYOFFSETSCALE
		pVB->m_pAttrib->Attrib_PolygonOffset(XR_DEFAULTPOLYOFFSETSCALE, XR_DEFAULTPOLYOFFSET);
#endif



		if (TextureIDProj)
		{
			pVB->m_pAttrib->Attrib_TextureID(2, TextureIDProj);

			const CMat4Dfp32* pProjPos = (_Light.m_Flags & CXR_LIGHT_PROJMAPTRANSFORM) ? 
				&_Light.m_ProjMapTransform : &_Light.m_Pos;

			const CVec3Dfp32& ProjX = CVec3Dfp32::GetRow(*pProjPos, 0);
			CVec3Dfp32 ProjY; CVec3Dfp32::GetRow(*pProjPos, 1).Scale(1.0f / _Light.m_SpotWidth, ProjY);
			CVec3Dfp32 ProjZ; CVec3Dfp32::GetRow(*pProjPos, 2).Scale(1.0f / _Light.m_SpotHeight, ProjZ);
			const CVec3Dfp32& Pos = CVec3Dfp32::GetRow(*pProjPos, 3);


			pTexGenAttr[4].CreateND(ProjX, -(ProjX * Pos));
			pTexGenAttr[5].CreateND(ProjY, -(ProjY * Pos));
			pTexGenAttr[6].CreateND(ProjZ, -(ProjZ * Pos));
			pVB->m_pAttrib->Attrib_TexGen(2, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V | CRC_TEXGENCOMP_W);
		}

		pVB->m_Priority = _Light.m_iLight + 0.3f;
#ifdef DST2_ATTENUATION
		pVB->m_pAttrib->Attrib_TextureID(0, m_TextureID_AttenuationDst2);
		pVB->m_pAttrib->Attrib_TextureID(1, m_TextureID_AttenuationDst2);
#else
		pVB->m_pAttrib->Attrib_TextureID(0, m_TextureID_AttenuationExp);
		pVB->m_pAttrib->Attrib_TextureID(1, m_TextureID_AttenuationExp);
#endif


#		ifdef SUPPORT_REGISTERCOMBINERS
#			ifdef SUPPORT_FRAGMENTPROGRAM
				if (m_ShaderMode == XR_SHADERMODE_REGCOMBINERS_NV20)
#			endif
			{
				if (TextureIDProj)
					pVB->m_pAttrib->m_pExtAttrib = &m_StaticExattr_Shading_Proj;
				else
					pVB->m_pAttrib->m_pExtAttrib = &m_StaticExattr_Shading;
			}
#		ifdef SUPPORT_FRAGMENTPROGRAM
			else 
#		endif
#		endif
#		ifdef SUPPORT_FRAGMENTPROGRAM
			if (m_ShaderMode == XR_SHADERMODE_FRAGMENTPROGRAM11)
			{
				CRC_ExtAttributes_FragmentProgram11 *pComb = (CRC_ExtAttributes_FragmentProgram11 *) m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram11));
				if (!pComb) return;
				pComb->Clear();

#				ifdef DST2_ATTENUATION
					if (TextureIDProj)
						pComb->m_pProgramName = "XRShader_Shading_Dst2_Proj";
					else
						pComb->m_pProgramName = "XRShader_Shading_Dst2";
#				else
					if (TextureIDProj)
						pComb->m_pProgramName = "XRShader_Shading_Proj";
					else
						pComb->m_pProgramName = "XRShader_Shading";
#				endif

				pVB->m_pAttrib->m_pExtAttrib = pComb;
			}
#		endif


		pVB->m_pAttrib->Attrib_Enable(CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_CULL);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_COLORWRITE);
		pVB->m_pAttrib->Attrib_ZCompare(ZCompare);

		pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
		pVB->m_iLight = _Light.m_iLight;
		m_pVBM->AddVB(pVB);
	}

	/* -------------------------------------------------------------------
		-Pass 2:
			Texture0 = Diffuse texture
			Texture1 = Normal map
			Texture2 = Lightvector normalization map
			Texture3 = Diffuse texture2 (not impl)

			FB.Color += FB.Alpha * [DiffuseTexture *] [DiffuseTexture2 *] (NormalMap dot LightVector)
	------------------------------------------------------------------- */

//	if (!bSpecularDiffuse)
	if (/*m_DiffuseScale &&*/ !(_Light.m_Flags & CXR_LIGHT_NODIFFUSE))
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VBAttrib();
		if (!pVB) 
			return;

		pVB->m_pAttrib->m_iTexCoordSet[0] = _pShaderParams->m_iTexCoordSetMapping;
		pVB->m_pAttrib->m_iTexCoordSet[1] = _pShaderParams->m_iTexCoordSetMapping;
		pVB->m_pAttrib->m_iTexCoordSet[2] = _pShaderParams->m_iTexCoordSetTangentU;
		pVB->m_pAttrib->m_iTexCoordSet[3] = _pShaderParams->m_iTexCoordSetTangentV;

		// Copy geometry
		pVB->CopyVBChain(_pVB);

		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

		CVec4Dfp32* pLightPos = m_pVBM->Alloc_V4(1);
		if (!pLightPos)
			return;
		*((CVec3Dfp32*)pLightPos) = _Light.GetPosition();
		pLightPos->k[3] = 1.0f;

#ifdef XR_DEFAULTPOLYOFFSETSCALE
		pVB->m_pAttrib->Attrib_PolygonOffset(XR_DEFAULTPOLYOFFSETSCALE, XR_DEFAULTPOLYOFFSET);
#endif
		pVB->m_pAttrib->Attrib_TexGen(2, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_ALL);
		pVB->m_pAttrib->Attrib_TexGenAttr((fp32*)pLightPos);

//		pVB->m_pAttrib->Attrib_TexGenU(2, CRC_TEXGENMODE_TSLV, (fp32*)pLightPos);

		pVB->m_Priority = _Light.m_iLight + 0.31f;
		pVB->m_pAttrib->Attrib_TextureID(0, _pShaderParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE]);
		pVB->m_pAttrib->Attrib_TextureID(1, TextureID_Normal);
		pVB->m_pAttrib->Attrib_TextureID(2, m_TextureID_Normalize);
		
		if (bSpecularDiffuseOnePass)
		{
			pVB->m_pAttrib->Attrib_TextureID(3, ms_TextureID_BackBuffer);
			pVB->m_pAttrib->Attrib_TexGen(3, CRC_TEXGENMODE_SCREEN, CRC_TEXGENCOMP_ALL);
		}
		
		pVB->m_pAttrib->Attrib_TexCoordSet(1, 0);


#		ifdef SUPPORT_REGISTERCOMBINERS
#			ifdef SUPPORT_FRAGMENTPROGRAM
				if (m_ShaderMode == XR_SHADERMODE_REGCOMBINERS_NV20)
#			endif
			{
				if (bSpecularDiffuseOnePass)
					pVB->m_pAttrib->m_pExtAttrib = &m_StaticExattr_DotProd_DiffuseSpecularAlpha;
				else
					pVB->m_pAttrib->m_pExtAttrib = &m_StaticExattr_DotProd_Diffuse;
			}
#		ifdef SUPPORT_FRAGMENTPROGRAM
			else 
#		endif
#		endif
#		ifdef SUPPORT_FRAGMENTPROGRAM
			if (m_ShaderMode == XR_SHADERMODE_FRAGMENTPROGRAM11)
			{
				CRC_ExtAttributes_FragmentProgram11 *pComb = (CRC_ExtAttributes_FragmentProgram11 *) m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram11));
				if (!pComb) return;
				pComb->Clear();
				pComb->m_Colors[0] = 0;

				if (bSpecularDiffuseOnePass)
					pComb->m_pProgramName = "XRShader_DotProd_DiffuseSpecularAlpha";
				else 
					pComb->m_pProgramName = "XRShader_DotProd_Diffuse";

				pVB->m_pAttrib->m_pExtAttrib = pComb;
			}
#		endif

		CVec4Dfp32 DiffuseColor;
		DiffuseColor << _pShaderParams->m_DiffuseColor;
		DiffuseColor[0] *= _Light.m_Intensity[0] * m_DiffuseScale[0];
		DiffuseColor[1] *= _Light.m_Intensity[1] * m_DiffuseScale[1];
		DiffuseColor[2] *= _Light.m_Intensity[2] * m_DiffuseScale[2];

		CPixel32 DiffuseColor32(RoundToInt(DiffuseColor[0] * 255), RoundToInt(DiffuseColor[1] * 255), RoundToInt(DiffuseColor[2] * 255), 0);
		pVB->m_Color = DiffuseColor32;

		if (bSpecularDiffuseOnePass)
		{
			pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_NONE);
			pVB->m_pAttrib->Attrib_Enable(CRC_FLAGS_CULL | CRC_FLAGS_COLORWRITE | CRC_FLAGS_ALPHAWRITE | StencilTest);
			pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_BLEND );
			pVB->m_pAttrib->Attrib_ZCompare(ZCompare);
		}
		else
		{
			pVB->m_pAttrib->Attrib_SourceBlend(CRC_BLEND_DESTALPHA);
			pVB->m_pAttrib->Attrib_DestBlend(CRC_BLEND_ONE);
			pVB->m_pAttrib->Attrib_Enable(CRC_FLAGS_BLEND | CRC_FLAGS_CULL | CRC_FLAGS_COLORWRITE | StencilTest);
			pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ALPHAWRITE);
			pVB->m_pAttrib->Attrib_ZCompare(ZCompare);
		}

		if (StencilTest)
		{
			pVB->m_pAttrib->Attrib_StencilRef(128, 255);
			pVB->m_pAttrib->Attrib_StencilFrontOp(CRC_COMPARE_LESSEQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
			pVB->m_pAttrib->Attrib_StencilWriteMask(0);
		}

		pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
		pVB->m_iLight = _Light.m_iLight;
		m_pVBM->AddVB(pVB);
	}

	/* -------------------------------------------------------------------
		-Pass 2b:
			Texture0 = Diffuse texture

			FB.Alpha += FB.Alpha * DiffuseTexture.Alpha
	------------------------------------------------------------------- */

/*	if (bSpecularDiffuse)
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VBAttrib();
		if (!pVB) return;

		pVB->m_pAttrib->m_iTexCoordSet[0] = _pShaderParams->m_iTexCoordSetMapping;
		pVB->m_pAttrib->m_iTexCoordSet[1] = _pShaderParams->m_iTexCoordSetMapping;
		pVB->m_pAttrib->m_iTexCoordSet[2] = _pShaderParams->m_iTexCoordSetTangentU;
		pVB->m_pAttrib->m_iTexCoordSet[3] = _pShaderParams->m_iTexCoordSetTangentV;

		// Copy geometry
		pVB->CopyVBChain(_pVB);

		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

		pVB->m_Priority = _Light.m_iLight + 0.315f + 100;
		pVB->m_pAttrib->Attrib_TextureID(0, ms_TextureID_BackBuffer);
		pVB->m_pAttrib->Attrib_TexGen(0, CRC_TEXGENMODE_SCREEN, CRC_TEXGENCOMP_ALL);

		uint32 iShader = _Light.m_iLight * m_nLightShaders + 3;

		pVB->m_pAttrib->Attrib_Enable(CRC_FLAGS_CULL | CRC_FLAGS_COLORWRITE | CRC_FLAGS_ALPHAWRITE);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_BLEND );
		pVB->m_pAttrib->Attrib_ZCompare(CRC_COMPARE_ALWAYS);

		pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
		pVB->m_iLight = _Light.m_iLight;
		m_pVBM->AddVB(pVB);
	}*/

//	if (!bSpecularDiffuse)
	if (bSpecularDiffuse && !bSpecularDiffuseOnePass && bSpecalarEnabled)
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VBAttrib();
		if (!pVB) return;

		pVB->m_pAttrib->m_iTexCoordSet[0] = _pShaderParams->m_iTexCoordSetMapping;
		pVB->m_pAttrib->m_iTexCoordSet[1] = _pShaderParams->m_iTexCoordSetMapping;
		pVB->m_pAttrib->m_iTexCoordSet[2] = _pShaderParams->m_iTexCoordSetTangentU;
		pVB->m_pAttrib->m_iTexCoordSet[3] = _pShaderParams->m_iTexCoordSetTangentV;

		// Copy geometry
		pVB->CopyVBChain(_pVB);

		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

		pVB->m_Priority = _Light.m_iLight + 0.315f;
		pVB->m_pAttrib->Attrib_TextureID(0, _pShaderParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE]);

//		uint32 iShader = _Light.m_iLight * m_nLightShaders + 3;

		pVB->m_pAttrib->Attrib_SourceBlend(CRC_BLEND_DESTALPHA);
		pVB->m_pAttrib->Attrib_DestBlend(CRC_BLEND_ZERO);
		pVB->m_pAttrib->Attrib_Enable(CRC_FLAGS_BLEND | CRC_FLAGS_CULL | CRC_FLAGS_ALPHAWRITE | StencilTest);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_COLORWRITE);
		pVB->m_pAttrib->Attrib_ZCompare(ZCompare);

		if (StencilTest)
		{
			pVB->m_pAttrib->Attrib_StencilRef(128, 255);
			pVB->m_pAttrib->Attrib_StencilFrontOp(CRC_COMPARE_LESSEQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
			pVB->m_pAttrib->Attrib_StencilWriteMask(0);
		}

		pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
		pVB->m_iLight = _Light.m_iLight;
		m_pVBM->AddVB(pVB);
	}
	
	/* -------------------------------------------------------------------
		-Pass 3a:
  			Texture0 = SpecularMap
			Texture1 = Normal map
			Texture2 = HalfAngle normalization map (For specular)
  			Texture3 = EyeVector normalization map (For fresnel)

			FB.Color += FB.Alpha * [SpecularTexture *] (NormalMap dot HalfAngle)^k [* Fresnel]
	------------------------------------------------------------------- */
//	if (!bSpecularDiffuse)
	if (m_SpecularMode == 1 && /*m_SpecularScale &&*/ !(_Light.m_Flags  & CXR_LIGHT_NOSPECULAR))
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VBAttrib();
		if (!pVB) return;

		pVB->m_pAttrib->m_iTexCoordSet[0] = _pShaderParams->m_iTexCoordSetMapping;
		pVB->m_pAttrib->m_iTexCoordSet[1] = _pShaderParams->m_iTexCoordSetMapping;
		pVB->m_pAttrib->m_iTexCoordSet[2] = _pShaderParams->m_iTexCoordSetTangentU;
		pVB->m_pAttrib->m_iTexCoordSet[3] = _pShaderParams->m_iTexCoordSetTangentV;

		// Copy geometry
		pVB->CopyVBChain(_pVB);

		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

		const CMat4Dfp32* pMat = _pVB->m_pTransform;
		CVec4Dfp32* pLightVec = m_pVBM->Alloc_V4(2);
		if (!pLightVec)
			return;
		CVec3Dfp32 Pos = _Light.GetPosition();
		pLightVec[0][0] = Pos[0];
		pLightVec[0][1] = Pos[1];
		pLightVec[0][2] = Pos[2];
		pLightVec[0][3] = 1;
		for (int j = 0; j < 3; j++)
			pLightVec[1][j] = - (pMat->k[3][0]*pMat->k[j][0] + pMat->k[3][1]*pMat->k[j][1] + pMat->k[3][2]*pMat->k[j][2]);
		pLightVec[1][3] = 1;
//		pVB->m_pAttrib->Attrib_TexGenU(2, CRC_TEXGENMODE_TSREFLECTION, (fp32*)pLightVec);

		pVB->m_pAttrib->Attrib_TexGen(2, CRC_TEXGENMODE_TSREFLECTION, CRC_TEXGENCOMP_ALL);
		pVB->m_pAttrib->Attrib_TexGenAttr((fp32*)pLightVec);
#ifdef XR_DEFAULTPOLYOFFSETSCALE
		pVB->m_pAttrib->Attrib_PolygonOffset(XR_DEFAULTPOLYOFFSETSCALE, XR_DEFAULTPOLYOFFSET);
#endif

		pVB->m_Priority = _Light.m_iLight + 0.32f;
		pVB->m_pAttrib->Attrib_TextureID(0, TextureID_Normal);
		pVB->m_pAttrib->Attrib_TextureID(1, TextureID_Normal);
		pVB->m_pAttrib->Attrib_TextureID(2, m_TextureID_FlipNormalize);
//		pVB->m_pAttrib->Attrib_TextureID(3, _pShaderParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE2]);
		pVB->m_pAttrib->Attrib_TexCoordSet(1, 0);

		CVec4Dfp32 SpecularColor;
		SpecularColor << _pShaderParams->m_SpecularColor;
		SpecularColor[0] *= _Light.m_Intensity[0] * m_SpecularScale[0];
		SpecularColor[1] *= _Light.m_Intensity[1] * m_SpecularScale[1];
		SpecularColor[2] *= _Light.m_Intensity[2] * m_SpecularScale[2];
		CPixel32 SpecularColor32(RoundToInt(SpecularColor[0] * 255), RoundToInt(SpecularColor[1] * 255), RoundToInt(SpecularColor[2] * 255), 0);

		int Power = (m_SpecularForcePower) ? m_SpecularForcePower : _pShaderParams->m_SpecularPower;
#		ifdef SUPPORT_REGISTERCOMBINERS
#			ifdef SUPPORT_FRAGMENTPROGRAM
				if (m_ShaderMode  == XR_SHADERMODE_REGCOMBINERS_NV20)
#			endif
			{
				CRC_ExtAttributes_NV10* pComb = RegCombiners_NV20_DotProd_Specular(Power);
				if (!pComb) return;
				pComb->SetConst0(SpecularColor32);
		//		pComb->SetConst0(((_Light.m_IntensityInt32 * _pShaderParams->m_SpecularColor) * m_SpecularScale) * 512);
				pComb->Fixup();
				pVB->m_pAttrib->m_pExtAttrib = pComb;
			}
#		ifdef SUPPORT_FRAGMENTPROGRAM
			else 
#		endif
#		endif
#		ifdef SUPPORT_FRAGMENTPROGRAM
			if (m_ShaderMode == XR_SHADERMODE_FRAGMENTPROGRAM11)
			{
				CRC_ExtAttributes_FragmentProgram11 *pComb = (CRC_ExtAttributes_FragmentProgram11 *) m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram11));
				if (!pComb) return;
				int SpecLog2 = 0;
				while((1 << SpecLog2) < Power)
				{
					SpecLog2++;
				}
				pComb->Clear();
				pComb->m_Colors[0] = SpecularColor32;
				pComb->m_Colors[1] = 0;
				switch (SpecLog2)
				{
				case 0: pComb->m_pProgramName = "XRShader_DotProd_Specular0"; break; case 1: pComb->m_pProgramName = "XRShader_DotProd_Specular1"; break;
				case 2: pComb->m_pProgramName = "XRShader_DotProd_Specular2"; break; case 3: pComb->m_pProgramName = "XRShader_DotProd_Specular3"; break;
				case 4: pComb->m_pProgramName = "XRShader_DotProd_Specular4"; break; 
				case 5: pComb->m_pProgramName = "XRShader_DotProd_Specular5"; pComb->m_Colors[1] = 0xff808080; break;
				default: pComb->m_pProgramName = "XRShader_DotProd_Specular6"; pComb->m_Colors[1] = 0xffc0c0c0; break;
				}
				pVB->m_pAttrib->m_pExtAttrib = pComb;
			}
#		endif


		pVB->m_pAttrib->Attrib_SourceBlend(CRC_BLEND_DESTALPHA);
		pVB->m_pAttrib->Attrib_DestBlend(CRC_BLEND_ONE);
		pVB->m_pAttrib->Attrib_Enable(CRC_FLAGS_BLEND | CRC_FLAGS_CULL | CRC_FLAGS_COLORWRITE | StencilTest);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ALPHAWRITE);
		pVB->m_pAttrib->Attrib_ZCompare(ZCompare);

		if (StencilTest)
		{
			pVB->m_pAttrib->Attrib_StencilRef(128, 255);
			pVB->m_pAttrib->Attrib_StencilFrontOp(CRC_COMPARE_LESSEQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
			pVB->m_pAttrib->Attrib_StencilWriteMask(0);
		}

		pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
		pVB->m_iLight = _Light.m_iLight;
		m_pVBM->AddVB(pVB);
	}


	/* -------------------------------------------------------------------
		-Pass 3b:
  			Texture0 = Normal map
			Texture1 = Normal map
			Texture2 = Normal map
  			Texture3 = Half angle space specular cubemap

			FB.Color += FB.Alpha * [NormalMap.alpha *] (NormalMap dot HalfAngle)^k
	------------------------------------------------------------------- */
	else if (m_SpecularMode == 0 && /*m_SpecularScale &&*/ !(_Light.m_Flags  & CXR_LIGHT_NOSPECULAR))
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VBAttrib();
		if (!pVB) return;
		pVB->m_pAttrib->m_iTexCoordSet[0] = _pShaderParams->m_iTexCoordSetMapping;
		pVB->m_pAttrib->m_iTexCoordSet[1] = _pShaderParams->m_iTexCoordSetMapping;
		pVB->m_pAttrib->m_iTexCoordSet[2] = _pShaderParams->m_iTexCoordSetTangentU;
		pVB->m_pAttrib->m_iTexCoordSet[3] = _pShaderParams->m_iTexCoordSetTangentV;

		// Copy geometry
		pVB->CopyVBChain(_pVB);
		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

		const CMat4Dfp32* pMat = _pVB->m_pTransform;
		CVec4Dfp32* pLightVec = m_pVBM->Alloc_V4(2);
		if (!pLightVec)
			return;
		CVec3Dfp32 Pos = _Light.GetPosition();
		pLightVec[0][0] = Pos[0];
		pLightVec[0][1] = Pos[1];
		pLightVec[0][2] = Pos[2];
		pLightVec[0][3] = 1;
		for (int j = 0; j < 3; j++)
			pLightVec[1][j] = - (pMat->k[3][0]*pMat->k[j][0] + pMat->k[3][1]*pMat->k[j][1] + pMat->k[3][2]*pMat->k[j][2]);
		pLightVec[1][3] = 1;

		pVB->m_pAttrib->Attrib_TexGenAttr((fp32*)pLightVec);
#ifdef XR_DEFAULTPOLYOFFSETSCALE
		pVB->m_pAttrib->Attrib_PolygonOffset(XR_DEFAULTPOLYOFFSETSCALE, XR_DEFAULTPOLYOFFSET);
#endif

		pVB->m_pAttrib->Attrib_TexGen(1, CRC_TEXGENMODE_TSREFLECTION, CRC_TEXGENCOMP_ALL);
		pVB->m_pAttrib->Attrib_TexGen(2, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);
		pVB->m_pAttrib->Attrib_TexGen(3, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);

		pVB->m_Priority = _Light.m_iLight + 0.32f;

		int iSpecMap = 0;
		int Power = (m_SpecularForcePower) ? m_SpecularForcePower : _pShaderParams->m_SpecularPower;
		while((1 << iSpecMap) < Power)
		{
			iSpecMap++;
		}

		if (iSpecMap > 7)
			iSpecMap = 7;

		int TextureIDSpecFunc = m_lTextureID_Specular[iSpecMap];
		pVB->m_pAttrib->Attrib_TextureID(0, TextureID_Normal);
		pVB->m_pAttrib->Attrib_TextureID(1, TextureID_Normal);
		pVB->m_pAttrib->Attrib_TextureID(2, TextureID_Normal);
		pVB->m_pAttrib->Attrib_TextureID(3, TextureIDSpecFunc);


		CPixel32 *pPixel = NULL;
#		ifdef SUPPORT_REGISTERCOMBINERS
#			ifdef SUPPORT_FRAGMENTPROGRAM
				if (m_ShaderMode == XR_SHADERMODE_REGCOMBINERS_NV20)
#			endif
			{
				if(bSpecularDiffuse)
					pVB->m_pAttrib->m_pExtAttrib = &m_StaticExattr_NMapHSpaceCubeMapNoSpec;
				else
					pVB->m_pAttrib->m_pExtAttrib = &m_StaticExattr_NMapHSpaceCubeMap;


			}
#		ifdef SUPPORT_FRAGMENTPROGRAM
			else 
#		endif
#		endif
#		ifdef SUPPORT_FRAGMENTPROGRAM
			if (m_ShaderMode == XR_SHADERMODE_FRAGMENTPROGRAM11)
			{
				CRC_ExtAttributes_FragmentProgram11 *pComb = (CRC_ExtAttributes_FragmentProgram11 *) m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram11));
				if (!pComb) return;
				pComb->Clear();
				pComb->m_Colors[0] = 0;
				pPixel = &pComb->m_Colors[1];
				if(bSpecularDiffuse)
					pComb->m_pProgramName = "XRShader_NMapHSpaceCubeMapNoSpec";
				else
					pComb->m_pProgramName = "XRShader_NMapHSpaceCubeMap";
				pVB->m_pAttrib->m_pExtAttrib = pComb;
			}
#		endif


		CVec4Dfp32 SpecularColor;
		SpecularColor << _pShaderParams->m_SpecularColor;
		SpecularColor[0] *= _Light.m_Intensity[0] * m_SpecularScale[0];
		SpecularColor[1] *= _Light.m_Intensity[1] * m_SpecularScale[1];
		SpecularColor[2] *= _Light.m_Intensity[2] * m_SpecularScale[2];
		CPixel32 SpecularColor32(RoundToInt(SpecularColor[0] * 255), RoundToInt(SpecularColor[1] * 255), RoundToInt(SpecularColor[2] * 255), 0);

		if (pPixel)
			*pPixel = SpecularColor32;

		pVB->m_Color = SpecularColor32;

		pVB->m_pAttrib->Attrib_SourceBlend(CRC_BLEND_DESTALPHA);
		pVB->m_pAttrib->Attrib_DestBlend(CRC_BLEND_ONE);
		pVB->m_pAttrib->Attrib_Enable(CRC_FLAGS_BLEND | CRC_FLAGS_CULL | CRC_FLAGS_COLORWRITE | StencilTest);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ALPHAWRITE);
		pVB->m_pAttrib->Attrib_ZCompare(ZCompare);

		if (StencilTest)
		{
			pVB->m_pAttrib->Attrib_StencilRef(128, 255);
			pVB->m_pAttrib->Attrib_StencilFrontOp(CRC_COMPARE_LESSEQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
			pVB->m_pAttrib->Attrib_StencilWriteMask(0);
		}

		pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
		pVB->m_iLight = _Light.m_iLight;
		m_pVBM->AddVB(pVB);
	}
}

#endif


