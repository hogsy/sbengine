
// -------------------------------------------------------------------
//  ATTENUATION
// -------------------------------------------------------------------
class CXR_VirtualAttributes_ShaderTexEnvAttenuation : public CXR_VirtualAttributes
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
			pA->Attrib_TextureID(0, _pShader->m_TextureID_AttenuationDst2Alpha);
			pA->Attrib_TextureID(1, _pShader->m_TextureID_AttenuationDst2Alpha);
			pA->Attrib_TexEnvMode(1, CRC_TEXENVMODE_COMBINE_ADD);
	#else
			pA->Attrib_TextureID(0, _pShader->m_TextureID_AttenuationExp);
			pA->Attrib_TextureID(1, _pShader->m_TextureID_AttenuationExp);
	#endif
			pA->Attrib_Enable(CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_CULL);
			pA->Attrib_Disable(CRC_FLAGS_COLORWRITE);
			pA->Attrib_Disable(CRC_FLAGS_COLORWRITE);
	//		pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
			pA->Attrib_ZCompare(CRC_COMPARE_LESSEQUAL);

			pA->Attrib_StencilRef(128, 255);
			pA->Attrib_StencilFrontOp(CRC_COMPARE_LESSEQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
			pA->Attrib_StencilWriteMask(0);
		}
		ms_pBase = pA;
	}

	void Create(const CXR_ShaderParams* _pParams, int _TextureID_Projection, fp32* _pTexGenAttr)
	{
		m_pBaseAttrib = ms_pBase;
		m_Class = 1;
		m_ShaderParamFlags = _pParams->m_Flags & (XR_SHADERFLAGS_USEZLESS | XR_SHADERFLAGS_NOSTENCILTEST);
		m_TextureID_Projection = _TextureID_Projection;
		m_pTexGenAttr = _pTexGenAttr;
	}

	int OnCompare(const CXR_VirtualAttributes* _pOther)
	{
		const CXR_VirtualAttributes_ShaderTexEnvAttenuation* pLast = (CXR_VirtualAttributes_ShaderTexEnvAttenuation*)_pOther;
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
				_pDest->Attrib_TexEnvMode(2, CRC_TEXENVMODE_MULTIPLY | CRC_TEXENVMODE_ALPHA_SOURCE0_COMPLEMENT);
			}
			else
			{
				_pDest->m_TextureID[2] = 0;
				_pDest->Attrib_TexGen(2, CRC_TEXGENMODE_TEXCOORD, 0);
				_pDest->Attrib_TexEnvMode(2, CRC_TEXENVMODE_MULTIPLY);
			}
			_pDest->m_ZCompare = (m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;

			int StencilTest = (m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST) ? 0 : CRC_FLAGS_STENCIL;
			_pDest->Attrib_Enable(StencilTest);
		}
		else
		{
			CXR_VirtualAttributes_ShaderTexEnvAttenuation *pLast = (CXR_VirtualAttributes_ShaderTexEnvAttenuation *)_pLastAttr;

			_pDest->Attrib_TexGenAttr(m_pTexGenAttr);
			AttrChg |= CRC_ATTRCHG_TEXGENATTR;
			AttrChg |= -1;

			int StencilTest = (m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST) ? 0 : CRC_FLAGS_STENCIL;
			_pDest->Attrib_Enable(StencilTest);

			if (m_TextureID_Projection != pLast->m_TextureID_Projection)
			{
				_pDest->m_TextureID[2] = m_TextureID_Projection;
				AttrChg |= CRC_ATTRCHG_TEXTUREID;
			}

			if ((m_TextureID_Projection != 0) != (pLast->m_TextureID_Projection != 0))
			{
				if (m_TextureID_Projection)
				{
					_pDest->Attrib_TexGen(2, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V | CRC_TEXGENCOMP_W);
					_pDest->Attrib_TexEnvMode(2, CRC_TEXENVMODE_MULTIPLY | CRC_TEXENVMODE_ALPHA_SOURCE0_COMPLEMENT);
				}
				else
				{
					_pDest->Attrib_TexGen(2, CRC_TEXGENMODE_TEXCOORD, 0);
					_pDest->Attrib_TexEnvMode(2, CRC_TEXENVMODE_MULTIPLY);
				}
				AttrChg |= CRC_ATTRCHG_TEXGENCOMP | CRC_ATTRCHG_TEXGEN | CRC_ATTRCHG_TEXENVMODE;
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

CRC_Attributes* CXR_VirtualAttributes_ShaderTexEnvAttenuation::ms_pBase = NULL;


// -------------------------------------------------------------------
//  PROJECTION
// -------------------------------------------------------------------
class CXR_VirtualAttributes_ShaderTexEnvProj : public CXR_VirtualAttributes
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
			pA->Attrib_TexGen(0, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V | CRC_TEXGENCOMP_W);
			pA->Attrib_TexEnvMode(0, CRC_TEXENVMODE_MULTIPLY/* | 0x80*/);

			pA->Attrib_Enable(CRC_FLAGS_BLEND | CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_CULL);
			pA->Attrib_Disable(CRC_FLAGS_COLORWRITE);
	//		pA->Attrib_RasterMode(CRC_RASTERMODE_NONE);
			pA->Attrib_SourceBlend(CRC_BLEND_INVDESTALPHA);
			pA->Attrib_DestBlend(CRC_BLEND_ZERO);
	//		pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
			pA->Attrib_ZCompare(CRC_COMPARE_LESSEQUAL);

			pA->Attrib_StencilRef(128, 255);
			pA->Attrib_StencilFrontOp(CRC_COMPARE_LESSEQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
			pA->Attrib_StencilWriteMask(0);
		}
		ms_pBase = pA;
	}

	void Create(const CXR_ShaderParams* _pParams, int _TextureID_Projection, fp32* _pTexGenAttr)
	{
		m_pBaseAttrib = ms_pBase;
		m_Class = 2;
		m_ShaderParamFlags = _pParams->m_Flags & (XR_SHADERFLAGS_USEZLESS | XR_SHADERFLAGS_NOSTENCILTEST);
		m_TextureID_Projection = _TextureID_Projection;
		m_pTexGenAttr = _pTexGenAttr;
	}

	int OnCompare(const CXR_VirtualAttributes* _pOther)
	{
		const CXR_VirtualAttributes_ShaderTexEnvProj* pLast = (CXR_VirtualAttributes_ShaderTexEnvProj*)_pOther;
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
			_pDest->m_TextureID[0] = m_TextureID_Projection;

			_pDest->m_ZCompare = (m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;

			int StencilTest = (m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST) ? 0 : CRC_FLAGS_STENCIL;
			_pDest->Attrib_Enable(StencilTest);
		}
		else
		{
			CXR_VirtualAttributes_ShaderTexEnvProj *pLast = (CXR_VirtualAttributes_ShaderTexEnvProj *)_pLastAttr;

			_pDest->Attrib_TexGenAttr(m_pTexGenAttr);
			AttrChg |= CRC_ATTRCHG_TEXGENATTR;
			AttrChg |= -1;

			int StencilTest = (m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST) ? 0 : CRC_FLAGS_STENCIL;
			_pDest->Attrib_Enable(StencilTest);

			if (m_TextureID_Projection != pLast->m_TextureID_Projection)
			{
				_pDest->m_TextureID[0] = m_TextureID_Projection;
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

CRC_Attributes* CXR_VirtualAttributes_ShaderTexEnvProj::ms_pBase = NULL;

// -------------------------------------------------------------------
//  DOT3
// -------------------------------------------------------------------
class CXR_VirtualAttributes_ShaderTexEnvDot3 : public CXR_VirtualAttributes
{
public:
	static CRC_Attributes* ms_pBase;

	uint16 m_ShaderParamFlags;
	uint16 m_TextureID_Normal;
	uint8 m_iTexCoordSetMapping;
	uint8 m_iTexCoordSetTangentU;
	uint8 m_iTexCoordSetTangentV;
	fp32* m_pTexGenAttr;

	static void PrepareFrame(CXR_Shader* _pShader)
	{
		CRC_Attributes* pA = _pShader->m_pVBM->Alloc_Attrib();
		if (pA)
		{
			pA->SetDefault();
			pA->Attrib_TexGen(1, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V | CRC_TEXGENCOMP_W);

			pA->Attrib_TextureID(1, _pShader->m_TextureID_Normalize);
			pA->Attrib_TexEnvMode(1, CRC_TEXENVMODE_COMBINE_DOT3);

			pA->Attrib_Enable(CRC_FLAGS_BLEND | CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_CULL);
			pA->Attrib_Disable(CRC_FLAGS_COLORWRITE);
	//		pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
			pA->Attrib_ZCompare(CRC_COMPARE_LESSEQUAL);
			pA->Attrib_SourceBlend(CRC_BLEND_DESTALPHA);
			pA->Attrib_DestBlend(CRC_BLEND_ZERO);

			pA->Attrib_StencilRef(128, 255);
			pA->Attrib_StencilFrontOp(CRC_COMPARE_LESSEQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
			pA->Attrib_StencilWriteMask(0);
		}
		ms_pBase = pA;
	}

	void Create(const CXR_ShaderParams* _pParams, int _TextureID_Normal, fp32* _pTexGenAttr, bool _bInvDestAlphaBlend)
	{
		m_pBaseAttrib = ms_pBase;
		m_Class = 3;
		m_ShaderParamFlags = _pParams->m_Flags & (XR_SHADERFLAGS_USEZLESS | XR_SHADERFLAGS_NOSTENCILTEST);
		if (_bInvDestAlphaBlend)
			m_ShaderParamFlags |= 0x8000;
		m_TextureID_Normal = _TextureID_Normal;
		m_iTexCoordSetMapping = _pParams->m_iTexCoordSetMapping;
		m_iTexCoordSetTangentU = _pParams->m_iTexCoordSetTangentU;
		m_iTexCoordSetTangentV = _pParams->m_iTexCoordSetTangentV;
		m_pTexGenAttr = _pTexGenAttr;
	}

	int OnCompare(const CXR_VirtualAttributes* _pOther)
	{
		const CXR_VirtualAttributes_ShaderTexEnvDot3* pLast = (CXR_VirtualAttributes_ShaderTexEnvDot3*)_pOther;
		XR_COMPAREATTRIBUTE_INT(m_ShaderParamFlags, pLast->m_ShaderParamFlags);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Normal, pLast->m_TextureID_Normal);
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
/*			if (m_TextureID_Projection)
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
			}*/
			_pDest->Attrib_TextureID(0, m_TextureID_Normal);

			if (m_ShaderParamFlags & 0x8000)
				_pDest->Attrib_SourceBlend(CRC_BLEND_INVDESTALPHA);

			int StencilTest = (m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST) ? 0 : CRC_FLAGS_STENCIL;
			_pDest->Attrib_Enable(StencilTest);

			_pDest->m_ZCompare = (m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;
		}
		else
		{
			CXR_VirtualAttributes_ShaderTexEnvDot3 *pLast = (CXR_VirtualAttributes_ShaderTexEnvDot3 *)_pLastAttr;

			_pDest->m_iTexCoordSet[0] = m_iTexCoordSetMapping;
			_pDest->m_iTexCoordSet[1] = m_iTexCoordSetMapping;
			_pDest->m_iTexCoordSet[2] = m_iTexCoordSetTangentU;
			_pDest->m_iTexCoordSet[3] = m_iTexCoordSetTangentV;
			AttrChg |= CRC_ATTRCHG_TEXCOORDSET;

			int StencilTest = (m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST) ? 0 : CRC_FLAGS_STENCIL;
			_pDest->Attrib_Enable(StencilTest);
			AttrChg |= CRC_ATTRCHG_FLAGS;

			_pDest->Attrib_TexGenAttr(m_pTexGenAttr);
			AttrChg |= CRC_ATTRCHG_TEXGENATTR;
			AttrChg |= -1;

			if (m_TextureID_Normal != pLast->m_TextureID_Normal)
			{
				_pDest->Attrib_TextureID(0, m_TextureID_Normal);
				AttrChg |= CRC_ATTRCHG_TEXTUREID;
			}

			int SrcBlend = (m_ShaderParamFlags & 0x8000) ? CRC_BLEND_INVDESTALPHA : CRC_BLEND_DESTALPHA;
			if (SrcBlend != _pDest->m_SourceBlend)
			{
				_pDest->Attrib_SourceBlend(SrcBlend);
				AttrChg |= CRC_ATTRCHG_BLEND;
			}

/*			if ((m_TextureID_Projection != 0) != (pLast->m_TextureID_Projection != 0))
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
			}*/

			if ((m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) != (pLast->m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS))
			{
				_pDest->m_ZCompare = (m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;
				AttrChg |= CRC_ATTRCHG_ZCOMPARE;
			}
		}

		return AttrChg;
	}
};

CRC_Attributes* CXR_VirtualAttributes_ShaderTexEnvDot3::ms_pBase = NULL;



void CXR_Shader::RenderShading_TexEnvCombine(const CXR_Light& _Light, CXR_VertexBuffer* _pVB, const CXR_ShaderParams* _pShaderParams)
{
	MAUTOSTRIP(CXR_Shader_RenderShading_TexEnvCombine, MAUTOSTRIP_VOID);

	int TextureID_Diffuse = _pShaderParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE];
	if(!TextureID_Diffuse)
		return;

	int TextureID_Normal = _pShaderParams->m_lTextureIDs[XR_SHADERMAP_NORMAL];
	if(!TextureID_Normal)
		return;

	int TextureIDProj = _Light.m_ProjMapID;
//	TextureIDProj = 0;
	if (!TextureIDProj && _Light.m_Type == CXR_LIGHTTYPE_SPOT)
		TextureIDProj = m_TextureID_DefaultLens;


	bool bDot3InvDestAlphaBlend = true;

	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VB();
		
		if( !pVB ) 
			return;
		
		pVB->CopyVBChain(_pVB);

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

		int TextureIDProjForVA = 0;

		if (TextureIDProj && (m_Caps_nMultiTexture > 2))
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

			TextureIDProjForVA = TextureIDProj;
			TextureIDProj = 0;
			bDot3InvDestAlphaBlend = false;
		}


		void* pVAMem = m_pVBM->Alloc(sizeof(CXR_VirtualAttributes_ShaderTexEnvAttenuation));
		if(!pVAMem)
			return;
		CXR_VirtualAttributes_ShaderTexEnvAttenuation* pVA = new(pVAMem) CXR_VirtualAttributes_ShaderTexEnvAttenuation;
		pVA->Create(_pShaderParams, TextureIDProjForVA, (fp32*)pTexGenAttr);
		pVB->SetVirtualAttr(pVA);

		pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
		pVB->m_iLight = _Light.m_iLight;
		pVB->m_Priority = _Light.m_iLight + 0.3f;
		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

		m_pVBM->AddVB( pVB );
	}

	if (TextureIDProj)
	{
		bDot3InvDestAlphaBlend = false;

		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VB();
		if( !pVB ) 
			return;
		pVB->CopyVBChain(_pVB);

		int nTexGenAttr = 3 * 4;
		CPlane3Dfp32* pTexGenAttr = (CPlane3Dfp32*) m_pVBM->Alloc_fp32(nTexGenAttr);
		if (!pTexGenAttr)
			return;

		const CMat4Dfp32* pProjPos = (_Light.m_Flags & CXR_LIGHT_PROJMAPTRANSFORM) ? 
			&_Light.m_ProjMapTransform : &_Light.m_Pos;

		const CVec3Dfp32& ProjX = CVec3Dfp32::GetRow(*pProjPos, 0);
		CVec3Dfp32 ProjY; CVec3Dfp32::GetRow(*pProjPos, 1).Scale(1.0f / _Light.m_SpotWidth, ProjY);
		CVec3Dfp32 ProjZ; CVec3Dfp32::GetRow(*pProjPos, 2).Scale(1.0f / _Light.m_SpotHeight, ProjZ);
		const CVec3Dfp32& Pos = CVec3Dfp32::GetRow(*pProjPos, 3);

		pTexGenAttr[0].CreateND(ProjX, -(ProjX * Pos));
		pTexGenAttr[1].CreateND(ProjY, -(ProjY * Pos));
		pTexGenAttr[2].CreateND(ProjZ, -(ProjZ * Pos));

		void* pVAMem = m_pVBM->Alloc(sizeof(CXR_VirtualAttributes_ShaderTexEnvProj));
		if(!pVAMem)
			return;
		CXR_VirtualAttributes_ShaderTexEnvProj* pVA = new(pVAMem) CXR_VirtualAttributes_ShaderTexEnvProj;

		pVA->Create(_pShaderParams, TextureIDProj, (fp32*)pTexGenAttr);
		pVB->SetVirtualAttr(pVA);

		pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
		pVB->m_iLight = _Light.m_iLight;
		pVB->m_Priority = _Light.m_iLight + 0.31f;
		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

		m_pVBM->AddVB( pVB );
	}


	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VB();
		
		if( !pVB ) 
			return;
		
		pVB->CopyVBChain(_pVB);


		int TextureIDProj = _Light.m_ProjMapID;
		if (!TextureIDProj && _Light.m_Type == CXR_LIGHTTYPE_SPOT)
			TextureIDProj = m_TextureID_DefaultLens;

		// Texgen for attenuation
//		fp32 Range = _Light.m_Range;
//		fp32 Offset = 0.5f;

//		const CVec3Dfp32& Pos= _Light.GetPosition();

		int nTexGenAttr = 4;
		CPlane3Dfp32* pTexGenAttr = (CPlane3Dfp32*) m_pVBM->Alloc_fp32(nTexGenAttr);
		if (!pTexGenAttr)
			return;

		*((CVec3Dfp32*)pTexGenAttr) = _Light.GetPosition();
		pTexGenAttr->d = 1.0f;


		void* pVAMem = m_pVBM->Alloc(sizeof(CXR_VirtualAttributes_ShaderTexEnvDot3));
		if(!pVAMem)
			return;
		CXR_VirtualAttributes_ShaderTexEnvDot3* pVA = new(pVAMem) CXR_VirtualAttributes_ShaderTexEnvDot3;

		int TextureID_Normal = (_pShaderParams->m_lTextureIDs[XR_SHADERMAP_NORMAL]) ? _pShaderParams->m_lTextureIDs[XR_SHADERMAP_NORMAL] : m_TextureID_DefaultNormal;

		pVA->Create(_pShaderParams, TextureID_Normal, (fp32*)pTexGenAttr, bDot3InvDestAlphaBlend);
		pVB->SetVirtualAttr(pVA);

		pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
		pVB->m_iLight = _Light.m_iLight;
		pVB->m_Priority = _Light.m_iLight + 0.32f;
		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

		m_pVBM->AddVB( pVB );
	}
	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VBAttrib();

		int StencilTest = (_pShaderParams->m_Flags & XR_SHADERFLAGS_NOSTENCILTEST) ? 0 : CRC_FLAGS_STENCIL;

		if( !pVB ) 
			return;
		pVB->CopyVBChain(_pVB);
		
		pVB->m_pAttrib->m_iTexCoordSet[0] = _pShaderParams->m_iTexCoordSetMapping;
		pVB->m_pAttrib->m_iTexCoordSet[1] = _pShaderParams->m_iTexCoordSetMapping;
		pVB->m_pAttrib->m_iTexCoordSet[2] = _pShaderParams->m_iTexCoordSetTangentU;
		pVB->m_pAttrib->m_iTexCoordSet[3] = _pShaderParams->m_iTexCoordSetTangentV;

		CVec4Dfp32 DiffuseColor;
		DiffuseColor << _pShaderParams->m_DiffuseColor;
		DiffuseColor[0] *= _Light.m_Intensity[0] * m_DiffuseScale[0];
		DiffuseColor[1] *= _Light.m_Intensity[1] * m_DiffuseScale[1];
		DiffuseColor[2] *= _Light.m_Intensity[2] * m_DiffuseScale[2];

		CPixel32 DiffuseColor32(RoundToInt(DiffuseColor[0] * 255), RoundToInt(DiffuseColor[1] * 255), RoundToInt(DiffuseColor[2] * 255), 0);
		pVB->m_Color = DiffuseColor32;
//		pVB->m_Color = _Light.m_IntensityInt32;
		
		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;
		pVB->m_pAttrib->Attrib_TextureID(0, TextureID_Diffuse);
		pVB->m_pAttrib->Attrib_TexEnvMode(0, CRC_TEXENVMODE_COMBINE_MULTIPLY2);
		pVB->m_pAttrib->Attrib_SourceBlend(CRC_BLEND_DESTALPHA);
		pVB->m_pAttrib->Attrib_DestBlend(CRC_BLEND_ONE);
		pVB->m_pAttrib->Attrib_Enable(CRC_FLAGS_BLEND | StencilTest);

		pVB->m_pAttrib->Attrib_StencilRef(128, 255);
		pVB->m_pAttrib->Attrib_StencilFrontOp(CRC_COMPARE_LESSEQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
		pVB->m_pAttrib->Attrib_StencilWriteMask(0);

		pVB->m_pAttrib->Attrib_ZCompare(_pShaderParams->m_Flags & XR_SHADERFLAGS_USEZLESS ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL);

		pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
		pVB->m_iLight = _Light.m_iLight;
		pVB->m_Priority = _Light.m_iLight + 0.33f;
		
		m_pVBM->AddVB( pVB );
	}
}

