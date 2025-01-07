
class CXR_VirtualAttributes_ShaderFP20_LF : public CXR_VirtualAttributes
{
public:
	static CRC_Attributes* ms_pBase;

	uint16 m_TextureID_Diffuse;
	uint16 m_TextureID_Specular;
	uint16 m_TextureID_Normal;
	uint16 m_TextureID_Attribute;
	uint16 m_TextureID_Transmission;
	uint16 m_TextureID_AnisotropicDir;
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

	//		pA->Attrib_TexGen(1, CRC_TEXGENMODE_NORMALMAP, CRC_TEXGENCOMP_ALL);
	//		pA->Attrib_TexGen(2, CRC_TEXGENMODE_TANG_V, CRC_TEXGENCOMP_ALL);
	//		pA->Attrib_TexGen(3, CRC_TEXGENMODE_TANG_U, CRC_TEXGENCOMP_ALL);
	//		pA->Attrib_TexGen(2, CRC_TEXGENMODE_MSPOS, 0);
			pA->Attrib_TexGen(2, CRC_TEXGENMODE_VOID, 0);
			pA->Attrib_TexGen(3, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_ALL);
	//		pA->Attrib_TexGen(3, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_ALL);
	//		pA->Attrib_TexGen(4, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V | CRC_TEXGENCOMP_W);
			pA->Attrib_TexGen(5, CRC_TEXGENMODE_BUMPCUBEENV, CRC_TEXGENCOMP_ALL);	// NOTE: The proper version of CRC_TEXGENMODE_BUMPCUBEENV only works on texcoord 5!
			pA->Attrib_TexGen(6, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);
			pA->Attrib_TexGen(7, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);

			pA->Attrib_SourceBlend(CRC_BLEND_ONE);
			pA->Attrib_DestBlend(CRC_BLEND_ONE);
			pA->Attrib_Enable(CRC_FLAGS_BLEND | CRC_FLAGS_CULL | CRC_FLAGS_COLORWRITE);
			pA->Attrib_Disable(CRC_FLAGS_ALPHAWRITE);

	//		pA->Attrib_StencilRef(128, 255);
	//		pA->Attrib_StencilFrontOp(CRC_COMPARE_LESSEQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
	//		pA->Attrib_StencilWriteMask(0);
		}
		ms_pBase = pA;
	}

	void Create(CXR_Shader* _pShader, const CXR_ShaderParams_LightField* _pParams, const CXR_SurfaceShaderParams* _pSurfParams, int _TextureID_Projection1, int _TextureID_Projection2, fp32* _pTexGenAttr, CRC_ExtAttributes_FragmentProgram20* _pFP)
	{
		const CXR_SurfaceShaderParams* pSurfParams = _pSurfParams;
		m_pBaseAttrib	= ms_pBase;
		m_Class = 10;
		m_TextureID_Diffuse = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE] : _pShader->m_TextureID_Special_ffffffff;
		m_TextureID_Specular = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_SPECULAR]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_SPECULAR] : _pShader->m_TextureID_Special_ffffffff;
		m_TextureID_Normal = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_NORMAL]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_NORMAL] : _pShader->m_TextureID_DefaultNormal;
		m_TextureID_Attribute = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_ATTRIBUTE]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_ATTRIBUTE] : _pShader->m_TextureID_Special_ff000000;
		m_TextureID_Transmission = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_TRANSMISSION]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_TRANSMISSION] : _pShader->m_TextureID_Special_ff000000;
		m_TextureID_AnisotropicDir = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_ANISOTROPICDIR]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_ANISOTROPICDIR] : _pShader->m_TextureID_Special_ff000000;
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
		const CXR_VirtualAttributes_ShaderFP20_LF* pLast = (CXR_VirtualAttributes_ShaderFP20_LF*)_pOther;
		XR_COMPAREATTRIBUTE_INT(m_pFP->m_ProgramNameHash, pLast->m_pFP->m_ProgramNameHash);
		XR_COMPAREATTRIBUTE_INT(m_ShaderParamFlags, pLast->m_ShaderParamFlags);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Diffuse, pLast->m_TextureID_Diffuse);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Specular, pLast->m_TextureID_Specular);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Normal, pLast->m_TextureID_Normal);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Attribute, pLast->m_TextureID_Attribute);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Transmission, pLast->m_TextureID_Transmission);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_AnisotropicDir, pLast->m_TextureID_AnisotropicDir);
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
			_pDest->m_iTexCoordSet[2] = m_iTexCoordSetTangentU;
			_pDest->m_iTexCoordSet[3] = m_iTexCoordSetTangentV;

			_pDest->Attrib_TexGenAttr(m_pTexGenAttr);
			_pDest->Attrib_TextureID(0, m_TextureID_Diffuse);
			_pDest->Attrib_TextureID(1, m_TextureID_Specular);
			_pDest->Attrib_TextureID(2, m_TextureID_Normal);
			_pDest->Attrib_TextureID(3, m_TextureID_Attribute);
			_pDest->Attrib_TextureID(4, m_TextureID_Transmission);
			_pDest->Attrib_TextureID(7, m_TextureID_Environment);
			_pDest->Attrib_TextureID(8, m_TextureID_AnisotropicDir);

			int CullXW = (m_ShaderParamFlags & XR_SHADERFLAGS_CULLCW) ? CRC_FLAGS_CULLCW : 0;
			_pDest->Attrib_Enable(CullXW);

			_pDest->m_pExtAttrib = m_pFP;
			_pDest->m_ZCompare = (m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;
		}
		else
		{
			CXR_VirtualAttributes_ShaderFP20_LF *pLast = (CXR_VirtualAttributes_ShaderFP20_LF *)_pLastAttr;

			if (m_iTexCoordSetMapping != pLast->m_iTexCoordSetMapping)
			{
				_pDest->m_iTexCoordSet[0] = m_iTexCoordSetMapping;
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

			_pDest->Attrib_TextureID(0, m_TextureID_Diffuse);
			_pDest->Attrib_TextureID(1, m_TextureID_Specular);
			_pDest->Attrib_TextureID(2, m_TextureID_Normal);
			_pDest->Attrib_TextureID(3, m_TextureID_Attribute);
			_pDest->Attrib_TextureID(4, m_TextureID_Transmission);
			_pDest->Attrib_TextureID(7, m_TextureID_Environment);
			_pDest->Attrib_TextureID(8, m_TextureID_AnisotropicDir);
			AttrChg |= CRC_ATTRCHG_TEXTUREID;

			if ((m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) != (pLast->m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS))
			{
				_pDest->m_ZCompare = (m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;
				AttrChg |= CRC_ATTRCHG_ZCOMPARE;
			}

	/*		if ((m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST) != (pLast->m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST))
			{
				if (m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST)
					_pDest->Attrib_Enable(CRC_FLAGS_STENCIL);
				else
					_pDest->Attrib_Disable(CRC_FLAGS_STENCIL);
				AttrChg |= CRC_ATTRCHG_FLAGS;
			}*/

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

CRC_Attributes* CXR_VirtualAttributes_ShaderFP20_LF::ms_pBase = NULL;

#ifndef DOTPROD3
#define DOTPROD3(a, b) (a[0]*b[0] + a[1]*b[1] + a[2]*b[2])
#endif

void CXR_Shader::RenderShading_FP20_LF(CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams_LightField* _pShaderParams, const CXR_SurfaceShaderParams* _pSurfParams)
{
	MAUTOSTRIP(CXR_Shader_RenderShading_FP20_LF, MAUTOSTRIP_VOID);
	const CXR_SurfaceShaderParams* pSurfParams = _pSurfParams;

//	int StencilTest = (_pShaderParams->m_Flags & XR_SHADERFLAGS_NOSTENCILTEST) ? 0 : CRC_FLAGS_STENCIL;
//	int ZCompare = (_pShaderParams->m_Flags & XR_SHADERFLAGS_USEZEQUAL) ? CRC_COMPARE_EQUAL : CRC_COMPARE_LESSEQUAL;
//	int ZCompare = _pShaderParams->m_Flags&XR_SHADERFLAGS_USEZLESS ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;

//	int TextureID_Normal = (_pShaderParams->m_lTextureIDs[XR_SHADERMAP_NORMAL]) ? _pShaderParams->m_lTextureIDs[XR_SHADERMAP_NORMAL] : m_TextureID_DefaultNormal;
	int TextureID_Diffuse = pSurfParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE];
	if(!TextureID_Diffuse)
		return;

//	bool bSpecularDiffuse = (_pShaderParams->m_Flags & XR_SHADERFLAGS_SPECULARINDIFFUSE) != 0;

	int nTexGenSize = 0;
	nTexGenSize += 4*1;		// TSLV * 1
	nTexGenSize += 4*4*2;	// BumpCubeEnv texgen
	int nFPConst = 7+6;
	int VBRamRequired = Align(sizeof(CXR_VertexBuffer), 4)
					  + Align(sizeof(fp32) * nTexGenSize, 16)
					  + Align(sizeof(CRC_ExtAttributes_FragmentProgram20), 4)
					  + Align(sizeof(CVec4Dfp32) * nFPConst, 16)
					  + Align(sizeof(CXR_VirtualAttributes_ShaderFP20_LF), 4);
	uint8* pVBMData = (uint8*)m_pVBM->Alloc(VBRamRequired);
	uint8* pVBMStart = pVBMData;
	if(!pVBMData)
		return;

	/* -------------------------------------------------------------------
		-Pass 1:
			# Texture0 = Diffuse Map		// Default = 1,1,1,1
			# Texture1 = Specular Map		// Default = 1,1,1,1
			# Texture2 = Normal map			// Default = 1, 0.5, 0.5
			# Texture3 = Attribute Map		// r = Fresnel, g = Silkness, b = ?, a = ?, Default = 0,0,0,0
			# Texture4 = Transmission Diffuse Map	// Default = 0,0,0,0
			# Texture5 = Projection Map 1		// Default = 1,1,1
			# Texture6 = Projection Map 2		// Default = 1,1,1
			# Texture7 = Environment Map		// Default = 0,0,0

			#-----------------------------------------
			# TexCoord0 = Mapping tex coord
			# TexCoord1 = Animated model space pixel position
			# TexCoord2 = Interpolated tangent space light vector (IPTSLV)
			# TexCoord3 = Interpolated tangent space eye vector (IPTSEV)
			# TexCoord4 = ProjMap tex coord
			# TexCoord5 = Tangentspace-2-world transform, row 0
			# TexCoord6 = Tangentspace-2-world transform, row 1
			# TexCoord7 = Tangentspace-2-world transform, row 2

	 ------------------------------------------------------------------- */
	{
		CVec4Dfp32* pParams = (CVec4Dfp32*)pVBMData; pVBMData += Align(sizeof(CVec4Dfp32) * nFPConst, 16);
		fp32* pTexGenAttr = (fp32*)pVBMData; pVBMData += Align(sizeof(fp32) * nTexGenSize, 16);
		CXR_VertexBuffer* pVB = (CXR_VertexBuffer*)pVBMData; pVBMData += Align(sizeof(CXR_VertexBuffer), 4);
		pVB->Construct();
		pVB->CopyVBChain(_pVB);
		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

		// Texgen for attenuation
		int nTexGenPos = 0;

		// -------------------------------------------------------------------
		// TSLV texgen parameters
/*		pTexGenAttr[nTexGenPos++] = Pos[0];
		pTexGenAttr[nTexGenPos++] = Pos[1];
		pTexGenAttr[nTexGenPos++] = Pos[2];
		pTexGenAttr[nTexGenPos++] = 1.0f/32.0f;
*/
		// -------------------------------------------------------------------
		const CMat4Dfp32* pMat = _pVB->m_pTransform;
//		CMat4Dfp32 Inv;
//		pMat->InverseOrthogonal(Inv);
//		CVec3Dfp32 Eye(0);
//		Eye *= Inv;

		CVec3Dfp32 Eye;
		for (int j = 0; j < 3; j++)
			Eye[j] = - (pMat->k[3][0]*pMat->k[j][0] + pMat->k[3][1]*pMat->k[j][1] + pMat->k[3][2]*pMat->k[j][2]);

		pTexGenAttr[nTexGenPos++] = Eye[0];
		pTexGenAttr[nTexGenPos++] = Eye[1];
		pTexGenAttr[nTexGenPos++] = Eye[2];
		pTexGenAttr[nTexGenPos++] = 1.0f/32.0f;

		// -------------------------------------------------------------------
		// BumpCubeEnv texgen parameters
		if (_pShaderParams->m_pCurrentWMat)
		{
			_pShaderParams->m_pCurrentWMat->InverseOrthogonal(*(CMat4Dfp32*)&pTexGenAttr[nTexGenPos]);
//			memcpy(&pTexGenAttr[nTexGenPos], m_pCurrentWMat, sizeof(CMat4Dfp32));
		}
		else
			((CMat4Dfp32*)&pTexGenAttr[nTexGenPos])->Unit();
		nTexGenPos += 16;

		if (_pShaderParams->m_pCurrentVMat)
			_pShaderParams->m_pCurrentVMat->InverseOrthogonal(*(CMat4Dfp32*)&pTexGenAttr[nTexGenPos]);
		else
			((CMat4Dfp32*)&pTexGenAttr[nTexGenPos])->Unit();
		nTexGenPos += 16;

		// -------------------------------------------------------------------
		CRC_ExtAttributes_FragmentProgram20 *pFP = (CRC_ExtAttributes_FragmentProgram20*)pVBMData; pVBMData += Align(sizeof(CRC_ExtAttributes_FragmentProgram20), 4);
		pFP->Clear();

		pFP->SetParameters(pParams, nFPConst);

		CVec4Dfp32& LightPos = pParams[0];
		CVec4Dfp32& LightRange = pParams[1];
		CVec4Dfp32& LightColor = pParams[2];
		CVec4Dfp32& SpecColor = pParams[3];
		CVec4Dfp32& AttribScale = pParams[4];
		CVec4Dfp32& EnvColor = pParams[5];
		CVec4Dfp32& TransmissionColor = pParams[6];

		CVec3Dfp32 MajorLightDir;
		MajorLightDir[0] = 
			(_pShaderParams->m_lLFAxes[0][0] - _pShaderParams->m_lLFAxes[1][0]) + 
			(_pShaderParams->m_lLFAxes[0][1] - _pShaderParams->m_lLFAxes[1][1])*2.0f + 
			(_pShaderParams->m_lLFAxes[0][2] - _pShaderParams->m_lLFAxes[1][2]);
		MajorLightDir[1] = 
			(_pShaderParams->m_lLFAxes[2][0] - _pShaderParams->m_lLFAxes[3][0]) + 
			(_pShaderParams->m_lLFAxes[2][1] - _pShaderParams->m_lLFAxes[3][1])*2.0f + 
			(_pShaderParams->m_lLFAxes[2][2] - _pShaderParams->m_lLFAxes[3][2]);
		MajorLightDir[2] = 
			(_pShaderParams->m_lLFAxes[4][0] - _pShaderParams->m_lLFAxes[5][0]) + 
			(_pShaderParams->m_lLFAxes[4][1] - _pShaderParams->m_lLFAxes[5][1])*2.0f + 
			(_pShaderParams->m_lLFAxes[4][2] - _pShaderParams->m_lLFAxes[5][2]);
		MajorLightDir.Normalize();

		LightPos.k[0] = MajorLightDir[0];
		LightPos.k[1] = MajorLightDir[1];
		LightPos.k[2] = MajorLightDir[2];
		LightPos.k[3] = 1;

		LightRange.k[0] = 1.0;
		LightRange.k[1] = 1.0;
		LightRange.k[2] = 1.0;
		LightRange.k[3] = 1.0;

		vec128 Diffuse = M_VMul(M_VLd_V4f16_f32(&pSurfParams->m_DiffuseColor), m_DiffuseScale.v);
		vec128 Specular = M_VMul(M_VLd_V4f16_f32(&pSurfParams->m_SpecularColor), m_SpecularScale.v);
		LightColor.v = Diffuse;
		SpecColor.v = Specular;

/*		LightColor.k[0]	= m_DiffuseScale[0] * _pShaderParams->m_DiffuseColor.k[0];
		LightColor.k[1]	= m_DiffuseScale[1] * _pShaderParams->m_DiffuseColor.k[1];
		LightColor.k[2]	= m_DiffuseScale[2] * _pShaderParams->m_DiffuseColor.k[2];
		LightColor.k[3]	= 0.0f;

		SpecColor.k[0] = m_SpecularScale[0] * _pShaderParams->m_SpecularColor.k[0];
		SpecColor.k[1] = m_SpecularScale[1] * _pShaderParams->m_SpecularColor.k[1];
		SpecColor.k[2] = m_SpecularScale[2] * _pShaderParams->m_SpecularColor.k[2];
		SpecColor.k[3] = m_SpecularScale[3] * _pShaderParams->m_SpecularColor.k[3];
*/
		if (m_SpecularForcePower)
			SpecColor.k[3] = m_SpecularForcePower;

		AttribScale << pSurfParams->m_AttribScale;
		EnvColor << pSurfParams->m_EnvColor;
		TransmissionColor << pSurfParams->m_TransmissionColor;

		memcpy(pParams+7, _pShaderParams->m_lLFAxes, sizeof(CVec4Dfp32)*6);

		int iShader = XRSHADER_FP20_LF;
		pFP->SetProgram(ms_lProgramsShading20[iShader].m_pName, ms_lProgramsShading20[iShader].m_Hash);

		// Alloc VA
		void* pVAMem = pVBMData; pVBMData += Align(sizeof(CXR_VirtualAttributes_ShaderFP20_LF), 4);
		CXR_VirtualAttributes_ShaderFP20_LF* pVA = new(pVAMem) CXR_VirtualAttributes_ShaderFP20_LF;
		pVA->Create(this, _pShaderParams, pSurfParams, 0, 0, (fp32*)pTexGenAttr, pFP);
		pVB->SetVirtualAttr(pVA);

		M_ASSERT(pVBMData == (pVBMStart + VBRamRequired), "!");

		pVB->m_Priority = 0;

		m_pVBM->AddVB(pVB);


		// -------------------------------------------------------------------
		// Non self-shadowing SSS/transmission test.

/*		{
			CXR_VertexBuffer* pVB = m_pVBM->Alloc_VB();
			if (!pVB) 
				return;

			pVB->CopyVBChain(_pVB);
			pVB->Matrix_Set(_pVB->m_pTransform);
			pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

			CRC_ExtAttributes_FragmentProgram20 *pFP = (CRC_ExtAttributes_FragmentProgram20 *) m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
			if (!pFP) return;
			pFP->Clear();

			pFP->SetParameters(pParams, 7);
			pFP->SetProgram(ms_lProgramsShading20[11].m_pName, ms_lProgramsShading20[11].m_Hash);

			// Alloc VA
			void* pVAMem = m_pVBM->Alloc(sizeof(CXR_VirtualAttributes_ShaderFP20_LF));
			if(!pVAMem)
				return;
			CXR_VirtualAttributes_ShaderFP20_LF* pVA = new(pVAMem) CXR_VirtualAttributes_ShaderFP20_LF;
			pVA->Create(this, _pShaderParams, TextureIDProj, TextureIDProj, (fp32*)pTexGenAttr, pFP);
			pVB->SetVirtualAttr(pVA);

			pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
			pVB->m_iLight = _Light.m_iLight;
			pVB->m_Priority = _Light.m_iLight + 0.000f;
			m_pVBM->AddVB(pVB);
		}*/
	}
}

// -------------------------------------------------------------------
//  
// -------------------------------------------------------------------
class CXR_VirtualAttributes_ShaderFP20_LFM : public CXR_VirtualAttributes
{
public:
	static CRC_Attributes* ms_pBase;

	uint16 m_TextureID_Diffuse;
	uint16 m_TextureID_Specular;
	uint16 m_TextureID_Normal;
	uint16 m_TextureID_Attribute;
	uint16 m_TextureID_Transmission;
	uint16 m_TextureID_AnisotropicDir;
	uint16 m_TextureID_Environment;
	uint16 m_TextureID_LFM[4];
	uint8 m_ShaderParamFlags;
	uint8 m_iTexCoordSetMapping;
	uint8 m_iTexCoordSetTangentU;
	uint8 m_iTexCoordSetTangentV;
	uint8 m_iTexCoordSetLFM;
	uint8 m_iTexCoordSetLMScale;
	CRC_ExtAttributes_FragmentProgram20* m_pFP;
	fp32* m_pTexGenAttr;

	static void PrepareFrame(CXR_Shader* _pShader)
	{
		CRC_Attributes* pA = _pShader->m_pVBM->Alloc_Attrib();
		if (pA)
		{
			pA->SetDefault();

	//		pA->Attrib_TexGen(1, CRC_TEXGENMODE_NORMALMAP, CRC_TEXGENCOMP_ALL);
	//		pA->Attrib_TexGen(2, CRC_TEXGENMODE_TANG_V, CRC_TEXGENCOMP_ALL);
	//		pA->Attrib_TexGen(3, CRC_TEXGENMODE_TANG_U, CRC_TEXGENCOMP_ALL);
	//		pA->Attrib_TexGen(2, CRC_TEXGENMODE_MSPOS, 0);
			pA->Attrib_TexGen(2, CRC_TEXGENMODE_VOID, 0);
	//		pA->Attrib_TexGen(2, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_ALL);
			pA->Attrib_TexGen(3, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_ALL);
			pA->Attrib_TexGen(4, CRC_TEXGENMODE_TEXCOORD, CRC_TEXGENCOMP_U);
	//		pA->Attrib_TexGen(3, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_ALL);
	//		pA->Attrib_TexGen(4, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V | CRC_TEXGENCOMP_W);
	//		pA->Attrib_TexGen(5, CRC_TEXGENMODE_BUMPCUBEENV, CRC_TEXGENCOMP_ALL);	// NOTE: The proper version of CRC_TEXGENMODE_BUMPCUBEENV only works on texcoord 5!
	//		pA->Attrib_TexGen(6, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);
	//		pA->Attrib_TexGen(7, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);

			pA->Attrib_SourceBlend(CRC_BLEND_ONE);
			pA->Attrib_DestBlend(CRC_BLEND_ONE);
			pA->Attrib_Enable(CRC_FLAGS_BLEND | CRC_FLAGS_CULL | CRC_FLAGS_COLORWRITE);
			pA->Attrib_Disable(CRC_FLAGS_ALPHAWRITE);

	//		pA->Attrib_StencilRef(128, 255);
	//		pA->Attrib_StencilFrontOp(CRC_COMPARE_LESSEQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
	//		pA->Attrib_StencilWriteMask(0);
		}
		ms_pBase = pA;
	}

	void Create(CXR_Shader* _pShader, const CXR_ShaderParams_LightFieldMapping* _pParams, const CXR_SurfaceShaderParams* _pSurfParams, int _TextureID_Projection1, int _TextureID_Projection2, fp32* _pTexGenAttr, CRC_ExtAttributes_FragmentProgram20* _pFP)
	{
		const CXR_SurfaceShaderParams* pSurfParams = _pSurfParams;
		m_pBaseAttrib	= ms_pBase;
		m_Class = 11;
		m_TextureID_Diffuse = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE] : _pShader->m_TextureID_Special_ffffffff;
		m_TextureID_Specular = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_SPECULAR]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_SPECULAR] : _pShader->m_TextureID_Special_ffffffff;
		m_TextureID_Normal = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_NORMAL]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_NORMAL] : _pShader->m_TextureID_DefaultNormal;
		m_TextureID_Attribute = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_ATTRIBUTE]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_ATTRIBUTE] : _pShader->m_TextureID_Special_ff000000;
		m_TextureID_Transmission = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_TRANSMISSION]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_TRANSMISSION] : _pShader->m_TextureID_Special_ff000000;
		m_TextureID_AnisotropicDir = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_ANISOTROPICDIR]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_ANISOTROPICDIR] : _pShader->m_TextureID_Special_ff000000;
		m_TextureID_Environment = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_ENVIRONMENT]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_ENVIRONMENT] : _pShader->m_TextureID_Special_Cube_ff000000;
		m_TextureID_LFM[0] = _pParams->m_lLFMTextureID[0];
		m_TextureID_LFM[1] = _pParams->m_lLFMTextureID[1];
		m_TextureID_LFM[2] = _pParams->m_lLFMTextureID[2];
		m_TextureID_LFM[3] = _pParams->m_lLFMTextureID[3];
		m_ShaderParamFlags = _pParams->m_Flags & (XR_SHADERFLAGS_USEZLESS | XR_SHADERFLAGS_NOSTENCILTEST | XR_SHADERFLAGS_CULLCW);
		m_iTexCoordSetMapping = _pParams->m_iTexCoordSetMapping;
		m_iTexCoordSetTangentU = _pParams->m_iTexCoordSetTangentU;
		m_iTexCoordSetTangentV = _pParams->m_iTexCoordSetTangentV;
		m_iTexCoordSetLFM = _pParams->m_iTexCoordSetLFM;
		m_iTexCoordSetLMScale	= _pParams->m_iTexCoordSetLMScale;
		m_pFP = _pFP;
		m_pTexGenAttr = _pTexGenAttr;
	}

	int OnCompare(const CXR_VirtualAttributes* _pOther)
	{
		const CXR_VirtualAttributes_ShaderFP20_LFM* pLast = (CXR_VirtualAttributes_ShaderFP20_LFM*)_pOther;
		XR_COMPAREATTRIBUTE_INT(m_pFP->m_ProgramNameHash, pLast->m_pFP->m_ProgramNameHash);
		XR_COMPAREATTRIBUTE_INT(m_ShaderParamFlags, pLast->m_ShaderParamFlags);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Diffuse, pLast->m_TextureID_Diffuse);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Specular, pLast->m_TextureID_Specular);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Normal, pLast->m_TextureID_Normal);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Attribute, pLast->m_TextureID_Attribute);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Transmission, pLast->m_TextureID_Transmission);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_AnisotropicDir, pLast->m_TextureID_AnisotropicDir);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Environment, pLast->m_TextureID_Environment);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_LFM[0], pLast->m_TextureID_LFM[0]);
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
			_pDest->m_iTexCoordSet[1] = m_iTexCoordSetLFM;
			_pDest->m_iTexCoordSet[2] = m_iTexCoordSetTangentU;
			_pDest->m_iTexCoordSet[3] = m_iTexCoordSetTangentV;
			_pDest->m_iTexCoordSet[4] = m_iTexCoordSetLMScale;

			_pDest->Attrib_TexGenAttr(m_pTexGenAttr);
			_pDest->Attrib_TextureID(0, m_TextureID_Diffuse);
			_pDest->Attrib_TextureID(1, m_TextureID_Specular);
			_pDest->Attrib_TextureID(2, m_TextureID_Normal);
			_pDest->Attrib_TextureID(3, m_TextureID_Attribute);
			_pDest->Attrib_TextureID(4, m_TextureID_Transmission);
			_pDest->Attrib_TextureID(7, m_TextureID_Environment);
			_pDest->Attrib_TextureID(8, m_TextureID_AnisotropicDir);
			_pDest->Attrib_TextureID(10, m_TextureID_LFM[0]);
			_pDest->Attrib_TextureID(11, m_TextureID_LFM[1]);
			_pDest->Attrib_TextureID(12, m_TextureID_LFM[2]);
			_pDest->Attrib_TextureID(13, m_TextureID_LFM[3]);

			int CullXW = (m_ShaderParamFlags & XR_SHADERFLAGS_CULLCW) ? CRC_FLAGS_CULLCW : 0;
			_pDest->Attrib_Enable(CullXW);

			_pDest->m_pExtAttrib = m_pFP;
			_pDest->m_ZCompare = (m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;
		}
		else
		{
			CXR_VirtualAttributes_ShaderFP20_LFM *pLast = (CXR_VirtualAttributes_ShaderFP20_LFM *)_pLastAttr;

			if (m_iTexCoordSetMapping != pLast->m_iTexCoordSetMapping)
			{
				_pDest->m_iTexCoordSet[0] = m_iTexCoordSetMapping;
				AttrChg |= CRC_ATTRCHG_TEXCOORDSET;
			}
			if (m_iTexCoordSetLFM != pLast->m_iTexCoordSetLFM)
			{
				_pDest->m_iTexCoordSet[1] = m_iTexCoordSetLFM;
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
			if(m_iTexCoordSetLMScale != pLast->m_iTexCoordSetLMScale)
			{
				_pDest->m_iTexCoordSet[4] = m_iTexCoordSetLMScale;
				AttrChg |= CRC_ATTRCHG_TEXCOORDSET;
			}

			_pDest->Attrib_TexGenAttr(m_pTexGenAttr);
			AttrChg |= CRC_ATTRCHG_TEXGENATTR;

			_pDest->Attrib_TextureID(0, m_TextureID_Diffuse);
			_pDest->Attrib_TextureID(1, m_TextureID_Specular);
			_pDest->Attrib_TextureID(2, m_TextureID_Normal);
			_pDest->Attrib_TextureID(3, m_TextureID_Attribute);
			_pDest->Attrib_TextureID(4, m_TextureID_Transmission);
			_pDest->Attrib_TextureID(7, m_TextureID_Environment);
			_pDest->Attrib_TextureID(8, m_TextureID_AnisotropicDir);
			_pDest->Attrib_TextureID(10, m_TextureID_LFM[0]);
			_pDest->Attrib_TextureID(11, m_TextureID_LFM[1]);
			_pDest->Attrib_TextureID(12, m_TextureID_LFM[2]);
			_pDest->Attrib_TextureID(13, m_TextureID_LFM[3]);
			AttrChg |= CRC_ATTRCHG_TEXTUREID;

			if ((m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) != (pLast->m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS))
			{
				_pDest->m_ZCompare = (m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;
				AttrChg |= CRC_ATTRCHG_ZCOMPARE;
			}

	/*		if ((m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST) != (pLast->m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST))
			{
				if (m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST)
					_pDest->Attrib_Enable(CRC_FLAGS_STENCIL);
				else
					_pDest->Attrib_Disable(CRC_FLAGS_STENCIL);
				AttrChg |= CRC_ATTRCHG_FLAGS;
			}*/

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

CRC_Attributes* CXR_VirtualAttributes_ShaderFP20_LFM::ms_pBase = NULL;

void CXR_Shader::RenderShading_FP20_LFM(CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams_LightFieldMapping* _pShaderParams, const CXR_SurfaceShaderParams* _pSurfParams)
{
	MAUTOSTRIP(CXR_Shader_RenderShading_FP20_LFM, MAUTOSTRIP_VOID);
	const CXR_SurfaceShaderParams* pSurfParams = _pSurfParams;

	/* -------------------------------------------------------------------
		-Pass 1:
			# Texture0 = Diffuse Map		// Default = 1,1,1,1
			# Texture1 = Specular Map		// Default = 1,1,1,1
			# Texture2 = Normal map			// Default = 1, 0.5, 0.5
			# Texture3 = Attribute Map		// r = Fresnel, g = Silkness, b = ?, a = ?, Default = 0,0,0,0
			# Texture4 = Transmission Diffuse Map	// Default = 0,0,0,0
			# Texture5 = Projection Map 1		// Default = 1,1,1
			# Texture6 = Projection Map 2		// Default = 1,1,1
			# Texture7 = Environment Map		// Default = 0,0,0

			#-----------------------------------------
			# TexCoord0 = Mapping tex coord
			# TexCoord1 = Animated model space pixel position
			# TexCoord2 = Interpolated tangent space light vector (IPTSLV)
			# TexCoord3 = Interpolated tangent space eye vector (IPTSEV)
			# TexCoord4 = ProjMap tex coord
			# TexCoord5 = Tangentspace-2-world transform, row 0
			# TexCoord6 = Tangentspace-2-world transform, row 1
			# TexCoord7 = Tangentspace-2-world transform, row 2

	 ------------------------------------------------------------------- */
	const int nFPConst = 7;
	int nTexGenSize = 0;
	nTexGenSize += 4*1;		// TSLV * 1
	nTexGenSize += 4*4*2;	// BumpCubeEnv texgen

	int VBRamRequired = Align(sizeof(CXR_VertexBuffer), 4)
					  + Align(sizeof(fp32) * nTexGenSize, 16)
					  + Align(sizeof(CRC_ExtAttributes_FragmentProgram20), 4)
					  + Align(sizeof(CVec4Dfp32) * nFPConst, 16)
					  + Align(sizeof(CXR_VirtualAttributes_ShaderFP20_LFM), 4);
	uint8* pVBMData = (uint8*)m_pVBM->Alloc(VBRamRequired);
	uint8* pVBMStart = pVBMData;
	if(!pVBMData)
		return;
	{
		fp32* pTexGenAttr = (fp32*)pVBMData; pVBMData += Align(sizeof(fp32) * nTexGenSize, 16);
		CVec4Dfp32* pParams = (CVec4Dfp32*)pVBMData; pVBMData += Align(sizeof(CVec4Dfp32) * nFPConst, 16);
		CXR_VertexBuffer* pVB = (CXR_VertexBuffer*)pVBMData; pVBMData += Align(sizeof(CXR_VertexBuffer), 4);
		pVB->Construct();

		pVB->CopyVBChain(_pVB);
		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

		int nTexGenPos = 0;

		// -------------------------------------------------------------------
		// TSLV texgen parameters
/*		pTexGenAttr[nTexGenPos++] = Pos[0];
		pTexGenAttr[nTexGenPos++] = Pos[1];
		pTexGenAttr[nTexGenPos++] = Pos[2];
		pTexGenAttr[nTexGenPos++] = 1.0f/32.0f;
*/
		// -------------------------------------------------------------------
		// Linear texgen for screenspace texcoords
/*		CMat4Dfp32& DestMat(*(CMat4Dfp32*)&pTexGenAttr[nTexGenPos]);
		if (m_pEngine)
		{
			CMat4Dfp32 Inv;
			_pVB->m_pTransform->InverseOrthogonal(Inv);
			_pVB->m_pTransform->Multiply(m_pEngine->m_Screen_TextureProjection, DestMat);
			DestMat.Transpose();
		}
		else
			DestMat.Unit();
		
		nTexGenPos += 16;
*/
		// -------------------------------------------------------------------
		const CMat4Dfp32* pMat = _pVB->m_pTransform;
//		CMat4Dfp32 Inv;
//		pMat->InverseOrthogonal(Inv);
//		CVec3Dfp32 Eye(0);
//		Eye *= Inv;

		CVec3Dfp32 Eye;
		for (int j = 0; j < 3; j++)
			Eye[j] = - (pMat->k[3][0]*pMat->k[j][0] + pMat->k[3][1]*pMat->k[j][1] + pMat->k[3][2]*pMat->k[j][2]);

		pTexGenAttr[nTexGenPos++] = Eye[0];
		pTexGenAttr[nTexGenPos++] = Eye[1];
		pTexGenAttr[nTexGenPos++] = Eye[2];
		pTexGenAttr[nTexGenPos++] = 1.0f/32.0f;

		// -------------------------------------------------------------------
		// BumpCubeEnv texgen parameters
		if (_pShaderParams->m_pCurrentWMat)
		{
			_pShaderParams->m_pCurrentWMat->InverseOrthogonal(*(CMat4Dfp32*)&pTexGenAttr[nTexGenPos]);
//			memcpy(&pTexGenAttr[nTexGenPos], m_pCurrentWMat, sizeof(CMat4Dfp32));
		}
		else
			((CMat4Dfp32*)&pTexGenAttr[nTexGenPos])->Unit();
		nTexGenPos += 16;

		if (_pShaderParams->m_pCurrentVMat)
			_pShaderParams->m_pCurrentVMat->InverseOrthogonal(*(CMat4Dfp32*)&pTexGenAttr[nTexGenPos]);
		else
			((CMat4Dfp32*)&pTexGenAttr[nTexGenPos])->Unit();
		nTexGenPos += 16;

		// -------------------------------------------------------------------
		CRC_ExtAttributes_FragmentProgram20* pFP = (CRC_ExtAttributes_FragmentProgram20*)pVBMData; pVBMData += Align(sizeof(CRC_ExtAttributes_FragmentProgram20), 4);
		pFP->Clear();
		pFP->SetParameters(pParams, 7);

		CVec4Dfp32& LightPos = pParams[0];
		CVec4Dfp32& LightRange = pParams[1];
		CVec4Dfp32& LightColor = pParams[2];
		CVec4Dfp32& SpecColor = pParams[3];
		CVec4Dfp32& AttribScale = pParams[4];
		CVec4Dfp32& EnvColor = pParams[5];
		CVec4Dfp32& TransmissionColor = pParams[6];

		LightPos.k[0] = 0;
		LightPos.k[1] = 0;
		LightPos.k[2] = 0;
		LightPos.k[3] = 1;

		LightRange.k[0] = 1.0;
		LightRange.k[1] = 1.0;
		LightRange.k[2] = 1.0;
		LightRange.k[3] = 1.0;

		vec128 Diffuse = M_VMul(M_VLd_V4f16_f32(&pSurfParams->m_DiffuseColor), m_DiffuseScale.v);
		vec128 Specular = M_VMul(M_VLd_V4f16_f32(&pSurfParams->m_SpecularColor), m_SpecularScale.v);
		LightColor.v = Diffuse;
		SpecColor.v = Specular;

/*		LightColor.k[0]	= m_DiffuseScale[0] * _pShaderParams->m_DiffuseColor.k[0];
		LightColor.k[1]	= m_DiffuseScale[1] * _pShaderParams->m_DiffuseColor.k[1];
		LightColor.k[2]	= m_DiffuseScale[2] * _pShaderParams->m_DiffuseColor.k[2];
		LightColor.k[3]	= 0.0f;

		SpecColor.k[0] = m_SpecularScale[0] * _pShaderParams->m_SpecularColor.k[0];
		SpecColor.k[1] = m_SpecularScale[1] * _pShaderParams->m_SpecularColor.k[1];
		SpecColor.k[2] = m_SpecularScale[2] * _pShaderParams->m_SpecularColor.k[2];
		SpecColor.k[3] = m_SpecularScale[3] * _pShaderParams->m_SpecularColor.k[3];*/

		if (m_SpecularForcePower)
			SpecColor.k[3] = m_SpecularForcePower;

		AttribScale << pSurfParams->m_AttribScale;
		EnvColor << pSurfParams->m_EnvColor;
		TransmissionColor << pSurfParams->m_TransmissionColor;

		int iShader = XRSHADER_FP20_LFM;
		pFP->SetProgram(ms_lProgramsShading20[iShader].m_pName, ms_lProgramsShading20[iShader].m_Hash);

		// Alloc VA
		void* pVAMem = pVBMData; pVBMData += Align(sizeof(CXR_VirtualAttributes_ShaderFP20_LFM), 4);
		CXR_VirtualAttributes_ShaderFP20_LFM* pVA = new(pVAMem) CXR_VirtualAttributes_ShaderFP20_LFM;
		pVA->Create(this, _pShaderParams, _pSurfParams, 0, 0, (fp32*)pTexGenAttr, pFP);
		pVB->SetVirtualAttr(pVA);

		M_ASSERT(pVBMData == (pVBMStart + VBRamRequired), "!");

		pVB->m_Priority = 0;

		m_pVBM->AddVB(pVB);


		// -------------------------------------------------------------------
		// Non self-shadowing SSS/transmission test.

/*		{
			CXR_VertexBuffer* pVB = m_pVBM->Alloc_VB();
			if (!pVB) 
				return;

			pVB->CopyVBChain(_pVB);
			pVB->Matrix_Set(_pVB->m_pTransform);
			pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

			CRC_ExtAttributes_FragmentProgram20 *pFP = (CRC_ExtAttributes_FragmentProgram20 *) m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
			if (!pFP) return;
			pFP->Clear();

			pFP->SetParameters(pParams, 7);
			pFP->SetProgram(ms_lProgramsShading20[11].m_pName, ms_lProgramsShading20[11].m_Hash);

			// Alloc VA
			void* pVAMem = m_pVBM->Alloc(sizeof(CXR_VirtualAttributes_ShaderFP20_LFM));
			if(!pVAMem)
				return;
			CXR_VirtualAttributes_ShaderFP20_LFM* pVA = new(pVAMem) CXR_VirtualAttributes_ShaderFP20_LFM;
			pVA->Create(this, _pShaderParams, TextureIDProj, TextureIDProj, (fp32*)pTexGenAttr, pFP);
			pVB->SetVirtualAttr(pVA);

			pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
			pVB->m_iLight = _Light.m_iLight;
			pVB->m_Priority = _Light.m_iLight + 0.000f;
			m_pVBM->AddVB(pVB);
		}*/
	}
}

