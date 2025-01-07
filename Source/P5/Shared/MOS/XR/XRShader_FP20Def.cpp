
// -------------------------------------------------------------------
//  
// -------------------------------------------------------------------
class CXR_VirtualAttributes_ShaderFP20Def_COREFBB : public CXR_VirtualAttributes
{
public:
	static CRC_Attributes* ms_pBase;

	uint16 m_TextureID_Projection;
	uint8 m_ShaderParamFlags;
	uint8 m_iTexCoordSetMapping;
	uint8 m_iTexCoordSetTangentU;
	uint8 m_iTexCoordSetTangentV;
	CRC_ExtAttributes_FragmentProgram20* m_pFP;
	fp32* m_pTexGenAttr;

	static void PrepareFrame(CXR_Shader* _pShader)
	{
		CRC_Attributes* pA = _pShader->m_pVBM->Alloc_Attrib();
		if (pA)
		{
			pA->SetDefault();

			pA->Attrib_TexGen(1, CRC_TEXGENMODE_PIXELINFO, CRC_TEXGENCOMP_ALL);
			pA->Attrib_TexGen(2, CRC_TEXGENMODE_VOID, 0);
			pA->Attrib_TexGen(3, CRC_TEXGENMODE_VOID, 0);
			pA->Attrib_TexGen(4, CRC_TEXGENMODE_VOID, 0);
			pA->Attrib_TexGen(5, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_ALL);

			if (_pShader->m_pEngine)
			{
				pA->Attrib_TextureID(0, _pShader->m_pEngine->m_TextureID_DeferredDiffuse);
				pA->Attrib_TextureID(1, _pShader->m_pEngine->m_TextureID_DeferredSpecular);
				pA->Attrib_TextureID(2, _pShader->m_pEngine->m_TextureID_DeferredNormal);
			}
			else
			{
				pA->Attrib_TextureID(0, _pShader->m_TextureID_Special_ffffffff);
				pA->Attrib_TextureID(1, _pShader->m_TextureID_Special_ffffffff);
				pA->Attrib_TextureID(2, _pShader->m_TextureID_Special_ff000000);
			}

			pA->Attrib_SourceBlend(CRC_BLEND_ONE);
			pA->Attrib_DestBlend(CRC_BLEND_ONE);
			pA->Attrib_Enable(CRC_FLAGS_BLEND | CRC_FLAGS_CULL | CRC_FLAGS_COLORWRITE);
			pA->Attrib_Disable(CRC_FLAGS_ALPHAWRITE);

			pA->Attrib_StencilRef(128, 255);
			pA->Attrib_StencilFrontOp(CRC_COMPARE_LESSEQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
			pA->Attrib_StencilWriteMask(0);
		}
		ms_pBase = pA;
	}

	void Create(CXR_Shader* _pShader, const CXR_ShaderParams* _pParams, int _TextureID_Projection, fp32* _pTexGenAttr, CRC_ExtAttributes_FragmentProgram20* _pFP)
	{
		m_pBaseAttrib = ms_pBase;
		m_Class = 7;
		m_TextureID_Projection = _TextureID_Projection;
		m_ShaderParamFlags = _pParams->m_Flags & (XR_SHADERFLAGS_USEZLESS | XR_SHADERFLAGS_NOSTENCILTEST | XR_SHADERFLAGS_CULLCW);
		m_iTexCoordSetMapping = _pParams->m_iTexCoordSetMapping;
		m_iTexCoordSetTangentU = _pParams->m_iTexCoordSetTangentU;
		m_iTexCoordSetTangentV = _pParams->m_iTexCoordSetTangentV;
		m_pFP = _pFP;
		m_pTexGenAttr = _pTexGenAttr;
	}

	int OnCompare(const CXR_VirtualAttributes* _pOther)
	{
		const CXR_VirtualAttributes_ShaderFP20Def_COREFBB* pLast = (CXR_VirtualAttributes_ShaderFP20Def_COREFBB*)_pOther;
		XR_COMPAREATTRIBUTE_INT(m_pFP->m_ProgramNameHash, pLast->m_pFP->m_ProgramNameHash);
		XR_COMPAREATTRIBUTE_INT(m_ShaderParamFlags, pLast->m_ShaderParamFlags);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Projection, pLast->m_TextureID_Projection);
		XR_COMPAREATTRIBUTE_INT(m_pFP, pLast->m_pFP);
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

#ifdef XR_DEFAULTPOLYOFFSETSCALE
			_pDest->Attrib_PolygonOffset(XR_DEFAULTPOLYOFFSETSCALE, XR_DEFAULTPOLYOFFSET);
#endif
			_pDest->m_iTexCoordSet[0] = m_iTexCoordSetMapping;
			_pDest->m_iTexCoordSet[1] = m_iTexCoordSetMapping;
			_pDest->m_iTexCoordSet[2] = m_iTexCoordSetTangentU;
			_pDest->m_iTexCoordSet[3] = m_iTexCoordSetTangentV;

			_pDest->Attrib_TexGenAttr(m_pTexGenAttr);
			if (m_TextureID_Projection)
			{
				_pDest->Attrib_TexGen(6, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V | CRC_TEXGENCOMP_W);
				_pDest->Attrib_TextureID(3, m_TextureID_Projection);
			}
			else
			{
			}

			int StencilTest = (m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST) ? 0 : CRC_FLAGS_STENCIL;
			int CullXW = (m_ShaderParamFlags & XR_SHADERFLAGS_CULLCW) ? CRC_FLAGS_CULLCW : 0;
			_pDest->Attrib_Enable(CullXW | StencilTest);

			_pDest->m_pExtAttrib = m_pFP;
			_pDest->m_ZCompare = (m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;
		}
		else
		{
			CXR_VirtualAttributes_ShaderFP20Def_COREFBB *pLast = (CXR_VirtualAttributes_ShaderFP20Def_COREFBB *)_pLastAttr;

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


			if ((m_TextureID_Projection != 0) != (pLast->m_TextureID_Projection != 0))
			{
				if (m_TextureID_Projection)
				{
					_pDest->Attrib_TexGen(6, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V | CRC_TEXGENCOMP_W);
				}
				else
				{
					_pDest->Attrib_TexGen(6, CRC_TEXGENMODE_VOID, 0);
				}
				AttrChg |= CRC_ATTRCHG_TEXGENCOMP | CRC_ATTRCHG_TEXGEN | CRC_ATTRCHG_EXATTR;
			}

			if (m_TextureID_Projection != pLast->m_TextureID_Projection)
			{
				_pDest->m_TextureID[3] = m_TextureID_Projection;
				AttrChg |= CRC_ATTRCHG_TEXTUREID;
			}

			if ((m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) != (pLast->m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS))
			{
				_pDest->m_ZCompare = (m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;
				AttrChg |= CRC_ATTRCHG_ZCOMPARE;
			}

			if ((m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST) != (pLast->m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST))
			{
				if (m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST)
					_pDest->Attrib_Enable(CRC_FLAGS_STENCIL);
				else
					_pDest->Attrib_Disable(CRC_FLAGS_STENCIL);
				AttrChg |= CRC_ATTRCHG_FLAG_STENCIL;
			}

			_pDest->m_pExtAttrib = m_pFP;
			AttrChg |= CRC_ATTRCHG_EXATTR;
		}

		return AttrChg;
	}
};

CRC_Attributes* CXR_VirtualAttributes_ShaderFP20Def_COREFBB::ms_pBase = NULL;

void CXR_Shader::RenderShading_FP20Def_COREFBB(const CXR_Light& _Light, CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams* _pShaderParams, const CXR_SurfaceShaderParams** _lpSurfParams)
{
	MAUTOSTRIP(CXR_Shader_RenderShading_FP20Def_COREFBB, MAUTOSTRIP_VOID);

//	int TextureID_Diffuse = pSurfParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE];
//	if(!TextureID_Diffuse)
//		return;

	uint nVB = _pShaderParams->m_nVB;

	uint TextureIDProj = _Light.m_ProjMapID;
	if (!TextureIDProj && _Light.m_Type == CXR_LIGHTTYPE_SPOT)
		TextureIDProj = m_TextureID_DefaultLens;

	uint nTexGenSize = 0;
	nTexGenSize += 3*4;		// PixelInfo
	nTexGenSize += 16;		// Linear TexGen
	if( TextureIDProj )
		nTexGenSize	+= 3*4;	// Projectionap params

	const uint nFPParams = 5;
	uint VBRamRequired = Align(sizeof(fp32) * (nTexGenSize), 16) +
		nVB * (Align(sizeof(CVec4Dfp32) * nFPParams, 16)
			+ Align(sizeof(CXR_VertexBuffer), 16)
			+ Align(sizeof(CRC_ExtAttributes_FragmentProgram20), 4)
			+ Align(sizeof(CXR_VirtualAttributes_ShaderFP20Def_COREFBB), 4));

	uint8*M_RESTRICT pVBMData = (uint8*)m_pVBM->Alloc(VBRamRequired, true);
	uint8*M_RESTRICT pVBMStart = pVBMData;
	if(!pVBMData)
		return;

	const CXR_LightPosition* pLightPos;
	CXR_LightPosition LightPos;
	if (_pShaderParams->m_pCurrentWMat)
	{
		_Light.CXR_LightPosition::Transform(*_pShaderParams->m_pCurrentWMat, LightPos);
		pLightPos = &LightPos;
	}
	else
		pLightPos = &_Light;

	/* ------------------------------------------------------------------- */
	fp32*M_RESTRICT pTexGenAttr = (fp32*)pVBMData; pVBMData += Align(sizeof(fp32) * nTexGenSize, 16);
	CVec3Dfp32 Eye;
	XRSHADER_CALC_TSEYEVEC(_pVB->m_pTransform, Eye);
	Eye *= *_pShaderParams->m_pCurrentWMat;

	int nTexGenPos = 0;

	// -------------------------------------------------------------------
	XRSHADER_INSERTTEXGENATTR_PIXELINFO(_pShaderParams->m_pCurrentWMat);		// 3*4
	XRSHADER_INSERTTEXGENATTR_LINEAR_SCREENPROJECTION;							// 16

	if( TextureIDProj )
		nTexGenPos += CreateProjMapTexGenAttr(&pTexGenAttr[nTexGenPos], _Light, _Light);// 3*4

	CXR_VertexBuffer* lpVB[XR_SHADER_MAXVB];

	const CVec3Dfp32& Pos = pLightPos->GetPosition();
	const CVec3Dfp32& Dir = pLightPos->GetDirection();

	/* ------------------------------------------------------------------- */
	{

		CVec4Dfp32* lParams = (CVec4Dfp32*)pVBMData; pVBMData += nVB * Align(sizeof(CVec4Dfp32) * nFPParams, 16);
		CXR_VertexBuffer* lVB = (CXR_VertexBuffer*)pVBMData; pVBMData += nVB * Align(sizeof(CXR_VertexBuffer), 16);
		CRC_ExtAttributes_FragmentProgram20 *lFP = (CRC_ExtAttributes_FragmentProgram20 *)pVBMData; pVBMData += nVB * Align(sizeof(CRC_ExtAttributes_FragmentProgram20), 4);
		CXR_VirtualAttributes_ShaderFP20Def_COREFBB* pVAMem = (CXR_VirtualAttributes_ShaderFP20Def_COREFBB*) pVBMData; pVBMData += nVB * Align(sizeof(CXR_VirtualAttributes_ShaderFP20Def_COREFBB), 4);

		uint iVB;

		for(iVB = 0; iVB < nVB; iVB++)
			lpVB[iVB] = lVB+iVB;

		for(iVB = 0; iVB < nVB; iVB++)
		{
			CXR_VertexBuffer*M_RESTRICT pVB = lVB+iVB;
			CRC_ExtAttributes_FragmentProgram20* pFP = lFP+iVB;
			CVec4Dfp32*M_RESTRICT pParams = lParams + (iVB * nFPParams);
			CXR_VirtualAttributes_ShaderFP20Def_COREFBB* pVA = new(pVAMem+iVB) CXR_VirtualAttributes_ShaderFP20Def_COREFBB;

			// -------------------------------------------------------------------
			pFP->Clear();
			pFP->SetParameters(pParams, nFPParams);

			const CXR_SurfaceShaderParams* pSurfParams = _lpSurfParams[iVB];
			pParams[0] = Eye;
			pParams[1] = Pos;
			XRSHADER_ATTRIB_SETLIGHTRANGE(pParams[2], _Light);
			XRSHADER_ATTRIB_SETLIGHTDIFF(pParams[3], _Light);
			XRSHADER_ATTRIB_SETLIGHTSPEC_OLD(pParams[4], _Light);

			int iShader = (TextureIDProj) ? XRSHADER_FP20DEF_NDSP : XRSHADER_FP20DEF_NDS;
			iShader += m_iDeferredShaderOffset;
			pFP->SetProgram(ms_lProgramsShading20[iShader].m_pName, ms_lProgramsShading20[iShader].m_Hash);

			// -------------------------------------------------------------------
			// Alloc VA
			pVA->Create(this, _pShaderParams, TextureIDProj, (fp32*)pTexGenAttr, pFP);

			pVB->Construct_Geometry_VA_Priority(_pVB+iVB, pVA, _Light.m_iLightf + 0.3f);
		}
		for(iVB = 0; iVB < nVB; iVB++)
		{
			CXR_VertexBuffer*M_RESTRICT pVB = lVB+iVB;
			pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
			pVB->m_iLight = _Light.m_iLight;
			pVB->SetVBEColor(0xff406080);
		}
		m_pVBM->AddVBArray(lpVB, nVB);
	}

	M_ASSERT(pVBMData <= (pVBMStart + VBRamRequired), "!");
}

// -------------------------------------------------------------------
//  
// -------------------------------------------------------------------
class CXR_VirtualAttributes_ShaderFP20Def : public CXR_VirtualAttributes
{
public:
	static CRC_Attributes* ms_pBase;

	uint16 m_TextureID_Attribute;
	uint16 m_TextureID_Transmission;
	uint16 m_TextureID_AnisotropicDir;
	uint16 m_TextureID_Projection1;
	uint16 m_TextureID_Projection2;
	uint16 m_TextureID_Environment;
	uint8 m_ShaderParamFlags;
	uint8 m_iTexCoordSetMapping;
	uint8 m_iTexCoordSetTangentU;
	uint8 m_iTexCoordSetTangentV;
	CRC_ExtAttributes_FragmentProgram20* m_pFP;
	fp32* m_pTexGenAttr;

	static void PrepareFrame(CXR_Shader* _pShader)
	{
		CRC_Attributes* pA = _pShader->m_pVBM->Alloc_Attrib();
		if (pA)
		{
			pA->SetDefault();

			if (_pShader->m_pEngine)
			{
				pA->Attrib_TextureID(0, _pShader->m_pEngine->m_TextureID_DeferredDiffuse);
				pA->Attrib_TextureID(1, _pShader->m_pEngine->m_TextureID_DeferredSpecular);
				pA->Attrib_TextureID(2, _pShader->m_pEngine->m_TextureID_DeferredNormal);
			}
			else
			{
				pA->Attrib_TextureID(0, _pShader->m_TextureID_Special_ffffffff);
				pA->Attrib_TextureID(1, _pShader->m_TextureID_Special_ffffffff);
				pA->Attrib_TextureID(2, _pShader->m_TextureID_Special_ff000000);
			}

			pA->Attrib_TexGen(1, CRC_TEXGENMODE_PIXELINFO, CRC_TEXGENCOMP_ALL);
			pA->Attrib_TexGen(2, CRC_TEXGENMODE_VOID, 0);
			pA->Attrib_TexGen(3, CRC_TEXGENMODE_VOID, 0);
			pA->Attrib_TexGen(4, CRC_TEXGENMODE_VOID, 0);
			pA->Attrib_TexGen(5, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_ALL);
			pA->Attrib_TexGen(6, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V | CRC_TEXGENCOMP_W);

			pA->Attrib_SourceBlend(CRC_BLEND_ONE);
			pA->Attrib_DestBlend(CRC_BLEND_ONE);
			pA->Attrib_Enable(CRC_FLAGS_BLEND | CRC_FLAGS_CULL | CRC_FLAGS_COLORWRITE);
			pA->Attrib_Disable(CRC_FLAGS_ALPHAWRITE);

			pA->Attrib_StencilRef(128, 255);
			pA->Attrib_StencilFrontOp(CRC_COMPARE_LESSEQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
			pA->Attrib_StencilWriteMask(0);
		}
		ms_pBase = pA;
	}

	void Create(CXR_Shader* _pShader, const CXR_ShaderParams* _pParams, const CXR_SurfaceShaderParams* _pSurfParams, int _TextureID_Projection1, int _TextureID_Projection2, fp32* _pTexGenAttr, CRC_ExtAttributes_FragmentProgram20* _pFP)
	{
		const CXR_SurfaceShaderParams* pSurfParams = _pSurfParams;
		m_pBaseAttrib = ms_pBase;
		m_Class = 8;
		m_TextureID_Attribute = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_ATTRIBUTE]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_ATTRIBUTE] : _pShader->m_TextureID_Special_ff000000;
		m_TextureID_Transmission = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_TRANSMISSION]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_TRANSMISSION] : _pShader->m_TextureID_Special_ff000000;
		m_TextureID_AnisotropicDir = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_ANISOTROPICDIR]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_ANISOTROPICDIR] : _pShader->m_TextureID_Special_ff000000;
		m_TextureID_Projection1 = _TextureID_Projection1;
		m_TextureID_Projection2 = _TextureID_Projection2;
		m_TextureID_Environment = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_ENVIRONMENT]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_ENVIRONMENT] : _pShader->m_TextureID_Special_Cube_ff000000;
		m_ShaderParamFlags = _pParams->m_Flags & (XR_SHADERFLAGS_USEZLESS | XR_SHADERFLAGS_NOSTENCILTEST | XR_SHADERFLAGS_CULLCW);
		m_iTexCoordSetMapping = _pParams->m_iTexCoordSetMapping;
		m_iTexCoordSetTangentU = _pParams->m_iTexCoordSetTangentU;
		m_iTexCoordSetTangentV = _pParams->m_iTexCoordSetTangentV;
		m_pFP = _pFP;
		m_pTexGenAttr = _pTexGenAttr;
	}

	int OnCompare(const CXR_VirtualAttributes* _pOther)
	{
		const CXR_VirtualAttributes_ShaderFP20Def* pLast = (CXR_VirtualAttributes_ShaderFP20Def*)_pOther;
		XR_COMPAREATTRIBUTE_INT(m_pFP->m_ProgramNameHash, pLast->m_pFP->m_ProgramNameHash);
		XR_COMPAREATTRIBUTE_INT(m_ShaderParamFlags, pLast->m_ShaderParamFlags);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Attribute, pLast->m_TextureID_Attribute);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Transmission, pLast->m_TextureID_Transmission);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_AnisotropicDir, pLast->m_TextureID_AnisotropicDir);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Projection1, pLast->m_TextureID_Projection1);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Projection2, pLast->m_TextureID_Projection2);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Environment, pLast->m_TextureID_Environment);
		XR_COMPAREATTRIBUTE_INT(m_pFP, pLast->m_pFP);
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

#ifdef XR_DEFAULTPOLYOFFSETSCALE
			_pDest->Attrib_PolygonOffset(XR_DEFAULTPOLYOFFSETSCALE, XR_DEFAULTPOLYOFFSET);
#endif
			_pDest->m_iTexCoordSet[0] = m_iTexCoordSetMapping;
			_pDest->m_iTexCoordSet[1] = m_iTexCoordSetMapping;
			_pDest->m_iTexCoordSet[2] = m_iTexCoordSetTangentU;
			_pDest->m_iTexCoordSet[3] = m_iTexCoordSetTangentV;

			_pDest->Attrib_TexGenAttr(m_pTexGenAttr);
			_pDest->Attrib_TextureID(3, m_TextureID_Projection1);
			_pDest->Attrib_TextureID(4, m_TextureID_Projection2);
			_pDest->Attrib_TextureID(5, m_TextureID_Attribute);
			_pDest->Attrib_TextureID(6, m_TextureID_Transmission);
			_pDest->Attrib_TextureID(7, m_TextureID_AnisotropicDir);
			_pDest->Attrib_TextureID(8, m_TextureID_Environment);

			int StencilTest = (m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST) ? 0 : CRC_FLAGS_STENCIL;
			int CullXW = (m_ShaderParamFlags & XR_SHADERFLAGS_CULLCW) ? CRC_FLAGS_CULLCW : 0;
			_pDest->Attrib_Enable(CullXW | StencilTest);

			_pDest->m_pExtAttrib = m_pFP;
			_pDest->m_ZCompare = (m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;
		}
		else
		{
			CXR_VirtualAttributes_ShaderFP20Def *pLast = (CXR_VirtualAttributes_ShaderFP20Def *)_pLastAttr;

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

			_pDest->Attrib_TextureID(3, m_TextureID_Projection1);
			_pDest->Attrib_TextureID(4, m_TextureID_Projection2);
			_pDest->Attrib_TextureID(5, m_TextureID_Attribute);
			_pDest->Attrib_TextureID(6, m_TextureID_Transmission);
			_pDest->Attrib_TextureID(7, m_TextureID_AnisotropicDir);
			_pDest->Attrib_TextureID(8, m_TextureID_Environment);
			AttrChg |= CRC_ATTRCHG_TEXTUREID;

			if ((m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) != (pLast->m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS))
			{
				_pDest->m_ZCompare = (m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;
				AttrChg |= CRC_ATTRCHG_ZCOMPARE;
			}

			if ((m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST) != (pLast->m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST))
			{
				if (m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST)
					_pDest->Attrib_Enable(CRC_FLAGS_STENCIL);
				else
					_pDest->Attrib_Disable(CRC_FLAGS_STENCIL);
				AttrChg |= CRC_ATTRCHG_FLAGS;
			}

			if ((m_ShaderParamFlags & XR_SHADERFLAGS_CULLCW) != (pLast->m_ShaderParamFlags & XR_SHADERFLAGS_CULLCW))
			{
				if (m_ShaderParamFlags & XR_SHADERFLAGS_CULLCW)
					_pDest->Attrib_Enable(CRC_FLAGS_CULLCW);
				else
					_pDest->Attrib_Disable(CRC_FLAGS_CULLCW);
				AttrChg |= CRC_ATTRCHG_FLAGS;
			}

			_pDest->m_pExtAttrib = m_pFP;
			AttrChg |= CRC_ATTRCHG_EXATTR;
		}

		return AttrChg;
	}
};

CRC_Attributes* CXR_VirtualAttributes_ShaderFP20Def::ms_pBase = NULL;

void CXR_Shader::RenderShading_FP20Def(const CXR_Light& _Light, CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams* _pShaderParams, const CXR_SurfaceShaderParams* _pSurfParams)
{
	MAUTOSTRIP(CXR_Shader_RenderShading_FP20, MAUTOSTRIP_VOID);

	const CXR_SurfaceShaderParams* pSurfParams = _pSurfParams;

//	int StencilTest = (_pShaderParams->m_Flags & XR_SHADERFLAGS_NOSTENCILTEST) ? 0 : CRC_FLAGS_STENCIL;
//	int ZCompare = _pShaderParams->m_Flags&XR_SHADERFLAGS_USEZLESS ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;

//	int TextureID_Normal = (_pShaderParams->m_lTextureIDs[XR_SHADERMAP_NORMAL]) ? _pShaderParams->m_lTextureIDs[XR_SHADERMAP_NORMAL] : m_TextureID_DefaultNormal;
	int TextureID_Diffuse = pSurfParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE];
	if(!TextureID_Diffuse)
		return;

//	bool bSpecularDiffuse = (_pShaderParams->m_Flags & XR_SHADERFLAGS_SPECULARINDIFFUSE) != 0;

	int TextureIDProj = _Light.m_ProjMapID;
	if (!TextureIDProj && _Light.m_Type == CXR_LIGHTTYPE_SPOT)
		TextureIDProj = m_TextureID_DefaultLens;

	bool bEnabledProjMap = TextureIDProj != 0;
	bool bEnabledEnvMap = pSurfParams->m_lTextureIDs[XR_SHADERMAP_ENVIRONMENT] != 0;
	bool bEnabledAnisoSpec = pSurfParams->m_lTextureIDs[XR_SHADERMAP_ANISOTROPICDIR] != 0;

	if (!TextureIDProj)
		TextureIDProj = m_TextureID_Special_Cube_ffffffff;

	int nTexGenSize = 0;
	nTexGenSize	+= 3*4;	// PixelInfo
	nTexGenSize += 16;	// Projection gen
	nTexGenSize	+= 3*4;	// Projectionap params


	int VBRamRequired = Align(sizeof(CVec4Dfp32) * 8, 16)
					  + Align(sizeof(CXR_VertexBuffer), 4)
					  + Align(sizeof(fp32) * nTexGenSize, 16)
					  + Align(sizeof(CRC_ExtAttributes_FragmentProgram20), 4)
					  + Align(sizeof(CXR_VirtualAttributes_ShaderFP20Def), 4);

	uint8* pVBMData = (uint8*)m_pVBM->Alloc(VBRamRequired);
	uint8* pVBMStart = pVBMData;
	if(!pVBMData)
		return;

	/* -------------------------------------------------------------------
		-Pass 1:
			# Texture0 = Diffuse Map		// Default = 1,1,1,1
			# Texture1 = Specular Map		// Default = 1,1,1,1
			# Texture2 = Normal map			// Default = 1, 0.5, 0.5
			# Texture3 = Projection Map 1	// Default = 1,1,1
			# Texture4 = Projection Map 2	// Default = 1,1,1
			# Texture5 = Attribute Map		// r = Fresnel, g = Silkness, b = ?, a = ?, Default = 0,0,0,0
			# Texture6 = Transmission Diffuse Map	// Default = 0,0,0,0
			# Texture7 = Anisotropic direction
			# Texture8 = Environment Map	// Default = 0,0,0

			#-----------------------------------------
			# TexCoord0 = Mapping tex coord
			# TexCoord1 = PixelInfo 0
			# TexCoord2 = PixelInfo 1
			# TexCoord3 = PixelInfo 2
			# TexCoord4 = PixelInfo 3
			# TexCoord5 = Deferred components tex coord
			# TexCoord6 = Projection map tex coord
			# TexCoord7 = 

	 ------------------------------------------------------------------- */
	const CXR_LightPosition* pLightPos;
	CXR_LightPosition LightPos;
	if (_pShaderParams->m_pCurrentWMat)
	{
		_Light.CXR_LightPosition::Transform(*_pShaderParams->m_pCurrentWMat, LightPos);
		pLightPos = &LightPos;
	}
	else
		pLightPos = &_Light;


	{
		CVec4Dfp32* pParams = (CVec4Dfp32*)pVBMData; pVBMData += Align(sizeof(CVec4Dfp32) * 8, 16);
		fp32* pTexGenAttr = (fp32*)pVBMData; pVBMData += Align(sizeof(fp32) * nTexGenSize, 16);
		CXR_VertexBuffer* pVB = (CXR_VertexBuffer*)pVBMData; pVBMData += Align(sizeof(CXR_VertexBuffer), 4);
		pVB->Construct();

		pVB->CopyVBChain(_pVB);
		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

		const CVec3Dfp32& Pos = pLightPos->GetPosition();
		const CVec3Dfp32& Dir = pLightPos->GetDirection();

		CVec3Dfp32 Eye;
		XRSHADER_CALC_TSEYEVEC(_pVB->m_pTransform, Eye);
		Eye *= *_pShaderParams->m_pCurrentWMat;

		int nTexGenPos = 0;

		// -------------------------------------------------------------------
		XRSHADER_INSERTTEXGENATTR_PIXELINFO(_pShaderParams->m_pCurrentWMat);		// 3*4
		XRSHADER_INSERTTEXGENATTR_LINEAR_SCREENPROJECTION;							// 16

		if( 1 || TextureIDProj )
			nTexGenPos += CreateProjMapTexGenAttr(&pTexGenAttr[nTexGenPos], _Light, _Light);	// 3*4

		// -------------------------------------------------------------------
		CRC_ExtAttributes_FragmentProgram20 *pFP = (CRC_ExtAttributes_FragmentProgram20*)pVBMData; pVBMData += Align(sizeof(CRC_ExtAttributes_FragmentProgram20), 4);
		pFP->Clear();

		pFP->SetParameters(pParams, 8);

		pParams[0] = Eye;
		pParams[1] = Pos;
		XRSHADER_ATTRIB_SETLIGHTRANGE(pParams[2], _Light);
		XRSHADER_ATTRIB_SETLIGHTDIFF(pParams[3], _Light);
		XRSHADER_ATTRIB_SETLIGHTSPEC(pParams[4], _Light);
		pParams[5].v = M_VLd_V4f16_f32(&pSurfParams->m_AttribScale);
		pParams[6].v = M_VLd_V4f16_f32(&pSurfParams->m_EnvColor);
		pParams[7].v = M_VLd_V4f16_f32(&pSurfParams->m_TransmissionColor);

//		pParams[4][3] *= 4.0f;

		if (_Light.m_LightGUID == 0x2346)
		{
//			pTexGenAttr[2] += Dir.k[1];		// -= 300;
			pParams[1].k[2] += Dir.k[1];	// -= 100;
		}

		int iShader = (bEnabledAnisoSpec) ? 
			XRSHADER_FP20DEF_NDSEAP :
			(XRSHADER_FP20DEF_NDS + bEnabledEnvMap*2 + bEnabledProjMap);
		iShader += m_iDeferredShaderOffset;

		pFP->SetProgram(ms_lProgramsShading20[iShader].m_pName, ms_lProgramsShading20[iShader].m_Hash);

		// -------------------------------------------------------------------
		// Alloc VA
		void* pVAMem = pVBMData; pVBMData += Align(sizeof(CXR_VirtualAttributes_ShaderFP20Def), 4);
		CXR_VirtualAttributes_ShaderFP20Def* pVA = new(pVAMem) CXR_VirtualAttributes_ShaderFP20Def;
		pVA->Create(this, _pShaderParams, pSurfParams, TextureIDProj, TextureIDProj, (fp32*)pTexGenAttr, pFP);
		pVB->SetVirtualAttr(pVA);

		M_ASSERT(pVBMData == (pVBMStart + VBRamRequired), "!");

		pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
		pVB->m_iLight = _Light.m_iLight;
		pVB->m_Priority = _Light.m_iLight + 0.3f;

		pVB->SetVBEColor(0xff406080);
		m_pVBM->AddVB(pVB);
	}
}


