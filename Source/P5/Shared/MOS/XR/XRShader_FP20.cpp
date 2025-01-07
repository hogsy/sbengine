
CXR_Shader_ProgramName CXR_Shader::ms_lProgramsShading20[] =
{
	{ "XRShader_FP20_LFM", 0 },
	{ "XRShader_FP20_LF", 0 },
	{ "XRShader_FP20_NDS", 0 },
	{ "XRShader_FP20_NDSP", 0 },
	{ "XRShader_FP20_NDSEATP", 0 },
	{ "XRShader_FP20Def_LFM", 0 },
	{ "XRShader_FP20Def_LF", 0 },
	{ "XRShader_FP20Def_NDS", 0 },
	{ "XRShader_FP20Def_NDSP", 0 },
	{ "XRShader_FP20Def_NDSE", 0 },
	{ "XRShader_FP20Def_NDSEP", 0 },
	{ "XRShader_FP20Def_NDSEAP", 0 },
	{ "XRShader_FP20DefLin_LFM", 0 },
	{ "XRShader_FP20DefLin_LF", 0 },
	{ "XRShader_FP20DefLin_NDS", 0 },
	{ "XRShader_FP20DefLin_NDSP", 0 },
	{ "XRShader_FP20DefLin_NDSE", 0 },
	{ "XRShader_FP20DefLin_NDSEP", 0 },
	{ "XRShader_FP20DefLin_NDSEAP", 0 },
	{ NULL, 0 }
};

enum
{
	XRSHADER_FP20_LFM = 0,
	XRSHADER_FP20_LF,
	XRSHADER_FP20_NDS,
	XRSHADER_FP20_NDSP,
	XRSHADER_FP20_NDSEATP,

	XRSHADER_FP20DEF_LFM,
	XRSHADER_FP20DEF_LF,
	XRSHADER_FP20DEF_NDS,
	XRSHADER_FP20DEF_NDSP,
	XRSHADER_FP20DEF_NDSE,
	XRSHADER_FP20DEF_NDSEP,
	XRSHADER_FP20DEF_NDSEAP,

	XRSHADER_FP20DEFLIN_LFM,
	XRSHADER_FP20DEFLIN_LF,
	XRSHADER_FP20DEFLIN_NDS,
	XRSHADER_FP20DEFLIN_NDSP,
	XRSHADER_FP20DEFLIN_NDSE,
	XRSHADER_FP20DEFLIN_NDSEP,
	XRSHADER_FP20DEFLIN_NDSEAP,
};

#define MACRO_ISALIGNED16(p) (!(mint((uint8*)p) & 0x0f))
int CXR_Shader::CreateProjMapTexGenAttr(fp32* _pTexGenAttr, const CXR_LightPosition& _LightPos, const CXR_Light& _Light)
{
#ifndef M_RTM
	if(!MACRO_ISALIGNED16(_pTexGenAttr))
		Error_static("CreateProjMapTexGenAttr", "Destination pointer must be 16-byte aligned");
#endif
	vec128* pTexGenProj = (vec128*)_pTexGenAttr;
	const CMat4Dfp32* pProjPos = (_LightPos.m_Flags & CXR_LIGHT_PROJMAPTRANSFORM) ? &_LightPos.m_ProjMapTransform : &_LightPos.m_Pos;
	vec128 ProjX = pProjPos->r[0];
	vec128 ProjY = M_VMul(pProjPos->r[1], M_VRcp(M_VLdScalar(_Light.m_SpotWidth)));
	vec128 ProjZ = M_VMul(pProjPos->r[2], M_VRcp(M_VLdScalar(_Light.m_SpotHeight)));
	vec128 Pos = pProjPos->r[3];

	vec128 SelMsk = M_VConst_u32(~0, ~0, ~0, 0);

	M_VSt(M_VSelMsk(SelMsk, ProjX, M_VSub(M_VZero(), M_VDp3(ProjX, Pos))), &pTexGenProj[0]);
	M_VSt(M_VSelMsk(SelMsk, ProjY, M_VSub(M_VZero(), M_VDp3(ProjY, Pos))), &pTexGenProj[1]);
	M_VSt(M_VSelMsk(SelMsk, ProjZ, M_VSub(M_VZero(), M_VDp3(ProjZ, Pos))), &pTexGenProj[2]);

	return 4*3;
}

// -------------------------------------------------------------------
//  
// -------------------------------------------------------------------
class CXR_VirtualAttributes_ShaderFP20_COREFBB : public CXR_VirtualAttributes
{
public:
	static CRC_Attributes* ms_pBase;

	uint16 m_TextureID_Projection;
	uint16 m_TextureID_Normal;
	uint16 m_TextureID_Diffuse;
	uint16 m_TextureID_Specular;
	uint16 m_TextureID_Height;
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
			pA->Attrib_TexGen(1, CRC_TEXGENMODE_MSPOS, 0);
			pA->Attrib_TexGen(2, CRC_TEXGENMODE_VOID, 0);
			pA->Attrib_TexGen(3, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_ALL);
			pA->Attrib_TexGen(4, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_ALL);
	//		pA->Attrib_TexGen(5, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_ALL);

	//		pA->Attrib_TextureID(3, _pShader->m_TextureID_Normalize);
			pA->Attrib_TextureID(8, _pShader->m_TextureID_Rand_03);
			pA->Attrib_TextureID(9, _pShader->m_TextureID_Rand_02);

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

	void Create(CXR_Shader* _pShader, const CXR_ShaderParams* _pShaderParams, const CXR_SurfaceShaderParams* _pSurfParams, int _TextureID_Projection, fp32* _pTexGenAttr, CRC_ExtAttributes_FragmentProgram20* _pFP)
	{
		const CXR_SurfaceShaderParams* pSurfParams = _pSurfParams;
		m_pBaseAttrib = ms_pBase;
		m_Class = 7;
		m_TextureID_Projection = _TextureID_Projection;
		m_TextureID_Normal = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_NORMAL]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_NORMAL] : _pShader->m_TextureID_DefaultNormal;
		m_TextureID_Diffuse = pSurfParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE];
		m_TextureID_Specular = pSurfParams->m_lTextureIDs[XR_SHADERMAP_SPECULAR];
		m_TextureID_Height = pSurfParams->m_lTextureIDs[XR_SHADERMAP_HEIGHT];
		m_ShaderParamFlags = pSurfParams->m_Flags & (XR_SHADERFLAGS_USEZLESS | XR_SHADERFLAGS_NOSTENCILTEST | XR_SHADERFLAGS_CULLCW);
		m_iTexCoordSetMapping = _pShaderParams->m_iTexCoordSetMapping;
		m_iTexCoordSetTangentU = _pShaderParams->m_iTexCoordSetTangentU;
		m_iTexCoordSetTangentV = _pShaderParams->m_iTexCoordSetTangentV;
		m_pFP = _pFP;
		m_pTexGenAttr = _pTexGenAttr;
	}

	int OnCompare(const CXR_VirtualAttributes* _pOther)
	{
		const CXR_VirtualAttributes_ShaderFP20_COREFBB* pLast = (CXR_VirtualAttributes_ShaderFP20_COREFBB*)_pOther;
		XR_COMPAREATTRIBUTE_INT(m_pFP->m_ProgramNameHash, pLast->m_pFP->m_ProgramNameHash);
		XR_COMPAREATTRIBUTE_INT(m_ShaderParamFlags, pLast->m_ShaderParamFlags);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Normal, pLast->m_TextureID_Normal);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Diffuse, pLast->m_TextureID_Diffuse);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Projection, pLast->m_TextureID_Projection);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Height, pLast->m_TextureID_Height);
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
				_pDest->Attrib_TexGen(7, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V | CRC_TEXGENCOMP_W);
				_pDest->Attrib_TextureID(4, m_TextureID_Projection);
			}
			else
			{
			}

			_pDest->Attrib_TextureID(0, m_TextureID_Diffuse);
			_pDest->Attrib_TextureID(1, m_TextureID_Specular);
			_pDest->Attrib_TextureID(2, m_TextureID_Normal);
//			if (m_TextureID_Height)
//				_pDest->Attrib_TextureID(1, m_TextureID_Height);

			int StencilTest = (m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST) ? 0 : CRC_FLAGS_STENCIL;
			int CullXW = (m_ShaderParamFlags & XR_SHADERFLAGS_CULLCW) ? CRC_FLAGS_CULLCW : 0;
			_pDest->Attrib_Enable(CullXW | StencilTest);

			_pDest->m_pExtAttrib = m_pFP;
			_pDest->m_ZCompare = (m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;
		}
		else
		{
			CXR_VirtualAttributes_ShaderFP20_COREFBB *pLast = (CXR_VirtualAttributes_ShaderFP20_COREFBB *)_pLastAttr;

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
			if (m_TextureID_Specular != pLast->m_TextureID_Specular)
			{
				_pDest->m_TextureID[1] = m_TextureID_Specular;
				AttrChg |= CRC_ATTRCHG_TEXTUREID;
			}
			if (m_TextureID_Normal != pLast->m_TextureID_Normal)
			{
				_pDest->m_TextureID[2] = m_TextureID_Normal;
				AttrChg |= CRC_ATTRCHG_TEXTUREID;
			}

/*			if (m_TextureID_Height != pLast->m_TextureID_Height)
			{
				if (m_TextureID_Height)
					_pDest->Attrib_TextureID(1, m_TextureID_Height);
				AttrChg |= CRC_ATTRCHG_TEXTUREID;
			}*/


			if ((m_TextureID_Projection != 0) != (pLast->m_TextureID_Projection != 0))
			{
				if (m_TextureID_Projection)
				{
					_pDest->Attrib_TexGen(7, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V | CRC_TEXGENCOMP_W);
				}
				else
				{
					_pDest->Attrib_TexGen(7, CRC_TEXGENMODE_VOID, 0);
				}
				AttrChg |= CRC_ATTRCHG_TEXGENCOMP | CRC_ATTRCHG_TEXGEN | CRC_ATTRCHG_EXATTR;
			}

			if (m_TextureID_Projection != pLast->m_TextureID_Projection)
			{
				_pDest->m_TextureID[4] = m_TextureID_Projection;
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

CRC_Attributes* CXR_VirtualAttributes_ShaderFP20_COREFBB::ms_pBase = NULL;

static int Align(int _nSize, int _Align)
{
	return (_nSize + _Align - 1) & ~(_Align - 1);
}

void CXR_Shader::RenderShading_FP20_COREFBB(const CXR_Light& _Light, CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams* _pShaderParams, const CXR_SurfaceShaderParams* _pSurfParams)
{
	MAUTOSTRIP(CXR_Shader_RenderShading_FP20_COREFBB, MAUTOSTRIP_VOID);
	const CXR_SurfaceShaderParams* pSurfParams = _pSurfParams;

//	int StencilTest = (_pShaderParams->m_Flags & XR_SHADERFLAGS_NOSTENCILTEST) ? 0 : CRC_FLAGS_STENCIL;
//	int ZCompare = (_pShaderParams->m_Flags & XR_SHADERFLAGS_USEZEQUAL) ? CRC_COMPARE_EQUAL : CRC_COMPARE_LESSEQUAL;
//	int ZCompare = _pShaderParams->m_Flags&XR_SHADERFLAGS_USEZLESS ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;

//	int TextureID_Normal = (_pShaderParams->m_lTextureIDs[XR_SHADERMAP_NORMAL]) ? _pShaderParams->m_lTextureIDs[XR_SHADERMAP_NORMAL] : m_TextureID_DefaultNormal;
	int TextureID_Diffuse = pSurfParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE];
	if(!TextureID_Diffuse)
		return;

	int TextureIDProj = _Light.m_ProjMapID;
	if (!TextureIDProj && _Light.m_Type == CXR_LIGHTTYPE_SPOT)
		TextureIDProj = m_TextureID_DefaultLens;

	int nTexGenSize = 0;
	nTexGenSize += 4*2;		// TSLV * 2
//	nTexGenSize += 16;		// Linear TexGen

	if( TextureIDProj )
		nTexGenSize	+= 3*4;	// Projectionap params


	int VBRamRequired = Align(sizeof(CVec4Dfp32) * 6, 16)
					  + Align(sizeof(CXR_VertexBuffer), 4)
					  + Align(sizeof(fp32) * (nTexGenSize), 16)
					  + Align(sizeof(CRC_ExtAttributes_FragmentProgram20), 4)
					  + Align(sizeof(CXR_VirtualAttributes_ShaderFP20_COREFBB), 4);
	uint8* pVBMData = (uint8*)m_pVBM->Alloc(VBRamRequired);
	uint8* pVBMStart = pVBMData;
	if(!pVBMData)
		return;

	/* -------------------------------------------------------------------
		-Pass 1:
			Texture0 = ExpAttenuation 2D texture
			Texture1 = ExpAttenuation 2D texture
			Texture2 = [ProjectionMap]
			Texture3 = [LightMask]

			FB.Alpha = Attenuation [* ProjectionMap] [* LightMask]
	 ------------------------------------------------------------------- */
	{
		CVec4Dfp32* pParams = (CVec4Dfp32*)pVBMData; pVBMData += Align(sizeof(CVec4Dfp32) * 6, 16);
		fp32* pTexGenAttr = (fp32*)pVBMData; pVBMData += Align(sizeof(fp32) * nTexGenSize, 16);
		CXR_VertexBuffer* pVB = (CXR_VertexBuffer*)pVBMData; pVBMData += Align(sizeof(CXR_VertexBuffer), 4);
		pVB->Construct();

		pVB->CopyVBChain(_pVB);
		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

		// Texgen for attenuation
//		fp32 Range = _Light.m_Range;
//		fp32 Offset = 0.5f;

		const CVec3Dfp32& Pos= _Light.GetPosition();
		const CVec3Dfp32& Dir= _Light.GetDirection();

		int nTexGenPos = 0;

		// -------------------------------------------------------------------
		XRSHADER_INSERTTEXGENATTR_TSVEC(Pos, 1/32.0f);
		CVec3Dfp32 Eye;
		XRSHADER_CALC_TSEYEVEC(_pVB->m_pTransform, Eye);
		XRSHADER_INSERTTEXGENATTR_TSVEC(Eye, 1/32.0f);

		if( TextureIDProj )
			nTexGenPos += CreateProjMapTexGenAttr(&pTexGenAttr[nTexGenPos], _Light, _Light);

		// -------------------------------------------------------------------
		CRC_ExtAttributes_FragmentProgram20 *pFP = (CRC_ExtAttributes_FragmentProgram20 *)pVBMData; pVBMData += Align(sizeof(CRC_ExtAttributes_FragmentProgram20), 4);
		pFP->Clear();


		pFP->SetParameters(pParams, 6);

		CVec4Dfp32& LightPos = pParams[0];
		CVec4Dfp32& LightRange = pParams[1];
		CVec4Dfp32& LightColor = pParams[2];
		CVec4Dfp32& SpecColor = pParams[3];
		CVec4Dfp32& EyePos = pParams[4];
		CVec4Dfp32& NoiseOffset = pParams[5];

		LightPos.k[0] = Pos.k[0];
		LightPos.k[1] = Pos.k[1];
		LightPos.k[2] = Pos.k[2];
		LightPos.k[3] = 1;

		LightRange.k[0] = _Light.m_RangeInv;
		LightRange.k[1] = _Light.m_Range;
		LightRange.k[2] = Sqr(_Light.m_RangeInv);
		LightRange.k[3] = Sqr(_Light.m_Range);

		XRSHADER_ATTRIB_SETLIGHTDIFF(LightColor, _Light);
		XRSHADER_ATTRIB_SETLIGHTSPEC_OLD(SpecColor, _Light);


		EyePos = Eye;
		NoiseOffset = 0;

		int iShader = (TextureIDProj) ? XRSHADER_FP20_NDSP : XRSHADER_FP20_NDS;
		if (_Light.m_LightGUID == 0x2346)
		{
//			iShader = 10;

		pTexGenAttr[2] += Dir.k[1];	//-= 300;
		LightPos.k[2] += Dir.k[0];	// -= 100;

			CMTime Time;
			if (m_pEngine) Time = m_pEngine->GetEngineTime(); else Time.Reset();

			NoiseOffset[0] = Time.GetTimeModulusScaled(0.01f, 1.0f);
			NoiseOffset[1] = Time.GetTimeModulusScaled(0.0087f, 1.0f);
			NoiseOffset[2] = Time.GetTimeModulusScaled(0.007f, 1.0f);
			NoiseOffset[3] = Time.GetTimeModulusScaled(0.13f, 1.0f);
		}

		pFP->SetProgram(ms_lProgramsShading20[iShader].m_pName, ms_lProgramsShading20[iShader].m_Hash);

		// Alloc VA
		void* pVAMem = pVBMData; pVBMData += Align(sizeof(CXR_VirtualAttributes_ShaderFP20_COREFBB), 4);
		CXR_VirtualAttributes_ShaderFP20_COREFBB* pVA = new(pVAMem) CXR_VirtualAttributes_ShaderFP20_COREFBB;
		pVA->Create(this, _pShaderParams, pSurfParams, TextureIDProj, (fp32*)pTexGenAttr, pFP);
		pVB->SetVirtualAttr(pVA);

		M_ASSERT(pVBMData == (pVBMStart + VBRamRequired), "!");

		pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
		pVB->m_iLight = _Light.m_iLight;
		pVB->m_Priority = (_pShaderParams->m_Flags & XR_SHADERFLAGS_TRANSPARENT) ? _pShaderParams->m_Priority :  (_Light.m_iLight + 0.3f);

		pVB->SetVBEColor(0xff404080);
		m_pVBM->AddVB(pVB);
	}
}

// -------------------------------------------------------------------
//  
// -------------------------------------------------------------------
class CXR_VirtualAttributes_ShaderFP20 : public CXR_VirtualAttributes
{
public:
	static CRC_Attributes* ms_pBase;

	uint16 m_TextureID_Diffuse;
	uint16 m_TextureID_Specular;
	uint16 m_TextureID_Normal;
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

	//		pA->Attrib_TexGen(1, CRC_TEXGENMODE_NORMALMAP, CRC_TEXGENCOMP_ALL);
	//		pA->Attrib_TexGen(2, CRC_TEXGENMODE_TANG_V, CRC_TEXGENCOMP_ALL);
	//		pA->Attrib_TexGen(3, CRC_TEXGENMODE_TANG_U, CRC_TEXGENCOMP_ALL);
			pA->Attrib_TexGen(1, CRC_TEXGENMODE_MSPOS, 0);
			pA->Attrib_TexGen(2, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_ALL);
			pA->Attrib_TexGen(3, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_ALL);
			pA->Attrib_TexGen(4, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V | CRC_TEXGENCOMP_W);
			pA->Attrib_TexGen(5, CRC_TEXGENMODE_BUMPCUBEENV, CRC_TEXGENCOMP_ALL);	// NOTE: The proper version of CRC_TEXGENMODE_BUMPCUBEENV only works on texcoord 5!
			pA->Attrib_TexGen(6, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);
			pA->Attrib_TexGen(7, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);

			pA->Attrib_TextureID(14, _pShader->m_TextureID_Rand_03);
			pA->Attrib_TextureID(15, _pShader->m_TextureID_Rand_02);

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

	void Create(CXR_Shader* _pShader, const CXR_ShaderParams* _pShaderParams, const CXR_SurfaceShaderParams* _pSurfParams, int _TextureID_Projection1, int _TextureID_Projection2, fp32* _pTexGenAttr, CRC_ExtAttributes_FragmentProgram20* _pFP)
	{
		const CXR_SurfaceShaderParams* pSurfParams = _pSurfParams;
		m_pBaseAttrib = ms_pBase;
		m_Class = 8;
		m_TextureID_Diffuse = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE] : _pShader->m_TextureID_Special_ffffffff;
		m_TextureID_Specular = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_SPECULAR]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_SPECULAR] : _pShader->m_TextureID_Special_ffffffff;
		m_TextureID_Normal = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_NORMAL]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_NORMAL] : _pShader->m_TextureID_DefaultNormal;
		m_TextureID_Attribute = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_ATTRIBUTE]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_ATTRIBUTE] : _pShader->m_TextureID_Special_ff000000;
		m_TextureID_Transmission = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_TRANSMISSION]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_TRANSMISSION] : _pShader->m_TextureID_Special_ff000000;
		m_TextureID_AnisotropicDir = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_ANISOTROPICDIR]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_ANISOTROPICDIR] : _pShader->m_TextureID_Special_ff000000;
		m_TextureID_Projection1 = _TextureID_Projection1;
		m_TextureID_Projection2 = _TextureID_Projection2;
		m_TextureID_Environment = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_ENVIRONMENT]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_ENVIRONMENT] : _pShader->m_TextureID_Special_Cube_ff000000;
		m_ShaderParamFlags = _pShaderParams->m_Flags & (XR_SHADERFLAGS_USEZLESS | XR_SHADERFLAGS_NOSTENCILTEST | XR_SHADERFLAGS_CULLCW);
		m_iTexCoordSetMapping = _pShaderParams->m_iTexCoordSetMapping;
		m_iTexCoordSetTangentU = _pShaderParams->m_iTexCoordSetTangentU;
		m_iTexCoordSetTangentV = _pShaderParams->m_iTexCoordSetTangentV;
		m_pFP = _pFP;
		m_pTexGenAttr = _pTexGenAttr;
	}

	int OnCompare(const CXR_VirtualAttributes* _pOther)
	{
		const CXR_VirtualAttributes_ShaderFP20* pLast = (CXR_VirtualAttributes_ShaderFP20*)_pOther;
		XR_COMPAREATTRIBUTE_INT(m_pFP->m_ProgramNameHash, pLast->m_pFP->m_ProgramNameHash);
		XR_COMPAREATTRIBUTE_INT(m_ShaderParamFlags, pLast->m_ShaderParamFlags);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Diffuse, pLast->m_TextureID_Diffuse);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Specular, pLast->m_TextureID_Specular);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Normal, pLast->m_TextureID_Normal);
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
			_pDest->Attrib_TextureID(0, m_TextureID_Diffuse);
			_pDest->Attrib_TextureID(1, m_TextureID_Specular);
			_pDest->Attrib_TextureID(2, m_TextureID_Normal);
			_pDest->Attrib_TextureID(3, m_TextureID_Attribute);
			_pDest->Attrib_TextureID(4, m_TextureID_Transmission);
			_pDest->Attrib_TextureID(5, m_TextureID_Projection1);
			_pDest->Attrib_TextureID(6, m_TextureID_Projection2);
			_pDest->Attrib_TextureID(7, m_TextureID_Environment);
			_pDest->Attrib_TextureID(8, m_TextureID_AnisotropicDir);

			int StencilTest = (m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST) ? 0 : CRC_FLAGS_STENCIL;
			int CullXW = (m_ShaderParamFlags & XR_SHADERFLAGS_CULLCW) ? CRC_FLAGS_CULLCW : 0;
			_pDest->Attrib_Enable(CullXW | StencilTest);

			_pDest->m_pExtAttrib = m_pFP;
			_pDest->m_ZCompare = (m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;
		}
		else
		{
			CXR_VirtualAttributes_ShaderFP20 *pLast = (CXR_VirtualAttributes_ShaderFP20 *)_pLastAttr;

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

			_pDest->Attrib_TextureID(0, m_TextureID_Diffuse);
			_pDest->Attrib_TextureID(1, m_TextureID_Specular);
			_pDest->Attrib_TextureID(2, m_TextureID_Normal);
			_pDest->Attrib_TextureID(3, m_TextureID_Attribute);
			_pDest->Attrib_TextureID(4, m_TextureID_Transmission);
			_pDest->Attrib_TextureID(5, m_TextureID_Projection1);
			_pDest->Attrib_TextureID(6, m_TextureID_Projection2);
			_pDest->Attrib_TextureID(7, m_TextureID_Environment);
			_pDest->Attrib_TextureID(8, m_TextureID_AnisotropicDir);
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

CRC_Attributes* CXR_VirtualAttributes_ShaderFP20::ms_pBase = NULL;

void CXR_Shader::RenderShading_FP20(const CXR_Light& _Light, CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams* _pShaderParams, const CXR_SurfaceShaderParams* _pSurfParams)
{
	MAUTOSTRIP(CXR_Shader_RenderShading_FP20, MAUTOSTRIP_VOID);
	const CXR_SurfaceShaderParams* pSurfParams = _pSurfParams;

//	int StencilTest = (_pShaderParams->m_Flags & XR_SHADERFLAGS_NOSTENCILTEST) ? 0 : CRC_FLAGS_STENCIL;
//	int ZCompare = (_pShaderParams->m_Flags & XR_SHADERFLAGS_USEZEQUAL) ? CRC_COMPARE_EQUAL : CRC_COMPARE_LESSEQUAL;
//	int ZCompare = _pShaderParams->m_Flags&XR_SHADERFLAGS_USEZLESS ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;

//	int TextureID_Normal = (_pShaderParams->m_lTextureIDs[XR_SHADERMAP_NORMAL]) ? _pShaderParams->m_lTextureIDs[XR_SHADERMAP_NORMAL] : m_TextureID_DefaultNormal;
	int TextureID_Diffuse = pSurfParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE];
	if(!TextureID_Diffuse)
		return;

//	bool bSpecularDiffuse = (_pShaderParams->m_Flags & XR_SHADERFLAGS_SPECULARINDIFFUSE) != 0;

	int TextureIDProj = _Light.m_ProjMapID;
	if (!TextureIDProj && _Light.m_Type == CXR_LIGHTTYPE_SPOT)
		TextureIDProj = m_TextureID_DefaultLens;
	if (!TextureIDProj)
		TextureIDProj = m_TextureID_Special_Cube_ffffffff;

	int nTexGenSize = 0;
	nTexGenSize += 4*2;		// TSLV * 2
	if( TextureIDProj )
		nTexGenSize	+= 3*4;	// Projectionap params
	nTexGenSize += 4*4*2;	// BumpCubeEnv texgen

	int VBRamRequired = Align(sizeof(CXR_VertexBuffer), 4)
					  + Align(sizeof(fp32) * nTexGenSize, 16)
					  + Align(sizeof(CRC_ExtAttributes_FragmentProgram20), 4)
					  + Align(sizeof(CVec4Dfp32) * 8, 16)
					  + Align(sizeof(CXR_VirtualAttributes_ShaderFP20), 4);

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

		CVec4Dfp32* pParams = (CVec4Dfp32*)pVBMData; pVBMData += Align(sizeof(CVec4Dfp32) * 8, 16);
		fp32* pTexGenAttr = (fp32*)pVBMData; pVBMData += Align(sizeof(fp32) * nTexGenSize, 16);

		CXR_VertexBuffer* pVB = (CXR_VertexBuffer*)pVBMData; pVBMData += Align(sizeof(CXR_VertexBuffer), 4);
		pVB->Construct();

		pVB->CopyVBChain(_pVB);
		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

		// Texgen for attenuation
//		fp32 Range = _Light.m_Range;
//		fp32 Offset = 0.5f;

		const CVec3Dfp32& Pos= _Light.GetPosition();
		const CVec3Dfp32& Dir= _Light.GetDirection();

		int nTexGenPos = 0;

		// -------------------------------------------------------------------
		// TSLV texgen parameters
		pTexGenAttr[nTexGenPos++] = Pos[0];
		pTexGenAttr[nTexGenPos++] = Pos[1];
		pTexGenAttr[nTexGenPos++] = Pos[2];
		pTexGenAttr[nTexGenPos++] = 1.0f/32.0f;

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
		// Projection map texgen parameters
		if( 1 || TextureIDProj )
		{
			CPlane3Dfp32* pTexGenProj = 0;
			pTexGenProj = (CPlane3Dfp32*)&(pTexGenAttr[nTexGenPos]);
			nTexGenPos	+= 4*3;

			const CMat4Dfp32* pProjPos = (_Light.m_Flags & CXR_LIGHT_PROJMAPTRANSFORM) ? 
				&_Light.m_ProjMapTransform : &_Light.m_Pos;

			const CVec3Dfp32& ProjX = CVec3Dfp32::GetRow(*pProjPos, 0);
			CVec3Dfp32 ProjY; CVec3Dfp32::GetRow(*pProjPos, 1).Scale(1.0f / _Light.m_SpotWidth, ProjY);
			CVec3Dfp32 ProjZ; CVec3Dfp32::GetRow(*pProjPos, 2).Scale(1.0f / _Light.m_SpotHeight, ProjZ);
			const CVec3Dfp32& Pos = CVec3Dfp32::GetRow(*pProjPos, 3);

			pTexGenProj[0].CreateND(ProjX, -(ProjX * Pos));
			pTexGenProj[1].CreateND(ProjY, -(ProjY * Pos));
			pTexGenProj[2].CreateND(ProjZ, -(ProjZ * Pos));
		}

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

		pFP->SetParameters(pParams, 8);

		CVec4Dfp32& LightPos = pParams[0];
		CVec4Dfp32& LightRange = pParams[1];
		CVec4Dfp32& LightColor = pParams[2];
		CVec4Dfp32& SpecColor = pParams[3];
		CVec4Dfp32& AttribScale = pParams[4];
		CVec4Dfp32& EnvColor = pParams[5];
		CVec4Dfp32& TransmissionColor = pParams[6];
		CVec4Dfp32& NoiseOffset = pParams[7];

		LightPos.k[0] = Pos.k[0];
		LightPos.k[1] = Pos.k[1];
		LightPos.k[2] = Pos.k[2];
		LightPos.k[3] = 1;

		LightRange.k[0] = _Light.m_RangeInv;
		LightRange.k[1] = _Light.m_Range;
		LightRange.k[2] = Sqr(_Light.m_RangeInv);
		LightRange.k[3] = Sqr(_Light.m_Range);

		vec128 Diff = M_VLd_V4f16_f32(&pSurfParams->m_DiffuseColor);
		LightColor.v = M_VMul(M_VMul(_Light.GetIntensityv().v, m_DiffuseScale.v), Diff);
		LightColor.k[3]	= pSurfParams->m_SpecularAnisotrophy;

		vec128 Spec = M_VLd_V4f16_f32(&pSurfParams->m_SpecularColor);
		SpecColor.v = M_VMul(M_VMul(_Light.GetIntensityv().v, m_SpecularScale.v), Spec);

/*		LightColor.k[0]	= _Light.m_Intensity[0] * m_DiffuseScale[0] * _pShaderParams->m_DiffuseColor.k[0];
		LightColor.k[1]	= _Light.m_Intensity[1] * m_DiffuseScale[1] * _pShaderParams->m_DiffuseColor.k[1];
		LightColor.k[2]	= _Light.m_Intensity[2] * m_DiffuseScale[2] * _pShaderParams->m_DiffuseColor.k[2];
		LightColor.k[3]	= _pShaderParams->m_SpecularAnisotrophy;

		SpecColor.k[0] = _Light.m_Intensity[0] * m_SpecularScale[0] * _pShaderParams->m_SpecularColor.k[0];
		SpecColor.k[1] = _Light.m_Intensity[1] * m_SpecularScale[1] * _pShaderParams->m_SpecularColor.k[1];
		SpecColor.k[2] = _Light.m_Intensity[2] * m_SpecularScale[2] * _pShaderParams->m_SpecularColor.k[2];
		SpecColor.k[3] = m_SpecularScale[3] * _pShaderParams->m_SpecularColor.k[3];*/

		if (m_SpecularForcePower)
			SpecColor.k[3] = m_SpecularForcePower;

		AttribScale << pSurfParams->m_AttribScale;
		EnvColor << pSurfParams->m_EnvColor;
		TransmissionColor << pSurfParams->m_TransmissionColor;

		CMTime Time;
		if (m_pEngine) Time = m_pEngine->GetEngineTime(); else Time.Reset();


		NoiseOffset[0] = Time.GetTimeModulusScaled(0.01f, 1.0f);
		NoiseOffset[1] = Time.GetTimeModulusScaled(0.0087f, 1.0f);
		NoiseOffset[2] = Time.GetTimeModulusScaled(0.007f, 1.0f);
		NoiseOffset[3] = Time.GetTimeModulusScaled(0.13f, 1.0f);

		int iShader = XRSHADER_FP20_NDSEATP;
		if (_Light.m_LightGUID == 0x2346)
		{
			pTexGenAttr[2] += Dir.k[1];	//-= 300;
			LightPos.k[2] += Dir.k[0];	// -= 100;
		}
		pFP->SetProgram(ms_lProgramsShading20[iShader].m_pName, ms_lProgramsShading20[iShader].m_Hash);

		// Alloc VA
		void* pVAMem = pVBMData; pVBMData += Align(sizeof(CXR_VirtualAttributes_ShaderFP20), 4);
		CXR_VirtualAttributes_ShaderFP20* pVA = new(pVAMem) CXR_VirtualAttributes_ShaderFP20;
		pVA->Create(this, _pShaderParams, pSurfParams, TextureIDProj, TextureIDProj, (fp32*)pTexGenAttr, pFP);
		pVB->SetVirtualAttr(pVA);

		M_ASSERT(pVBMData == (pVBMStart + VBRamRequired), "!");

		pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
		pVB->m_iLight = _Light.m_iLight;
		pVB->m_Priority = (_pShaderParams->m_Flags & XR_SHADERFLAGS_TRANSPARENT) ? _pShaderParams->m_Priority :  (_Light.m_iLight + 0.3f);

		pVB->SetVBEColor(0xff404080);
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
			void* pVAMem = m_pVBM->Alloc(sizeof(CXR_VirtualAttributes_ShaderFP20));
			if(!pVAMem)
				return;
			CXR_VirtualAttributes_ShaderFP20* pVA = new(pVAMem) CXR_VirtualAttributes_ShaderFP20;
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
class CXR_VirtualAttributes_ShaderFP20_Glass : public CXR_VirtualAttributes
{
public:
	static CRC_Attributes* ms_pBase;

	uint16 m_TextureID_Diffuse;
	uint16 m_TextureID_Specular;
	uint16 m_TextureID_Normal;
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

			pA->Attrib_TexGen(1, CRC_TEXGENMODE_MSPOS, 0);
			pA->Attrib_TexGen(2, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_ALL);
			pA->Attrib_TexGen(3, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_ALL);
			pA->Attrib_TexGen(4, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V | CRC_TEXGENCOMP_W);
			pA->Attrib_TexGen(5, CRC_TEXGENMODE_BUMPCUBEENV, CRC_TEXGENCOMP_ALL);
			pA->Attrib_TexGen(6, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);
			pA->Attrib_TexGen(7, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);

			pA->Attrib_TextureID(14, _pShader->m_TextureID_Rand_03);
			pA->Attrib_TextureID(15, _pShader->m_TextureID_Rand_02);

			pA->Attrib_SourceBlend(CRC_BLEND_ONE);
			pA->Attrib_DestBlend(CRC_BLEND_ONE);
			pA->Attrib_Enable(CRC_FLAGS_BLEND | CRC_FLAGS_CULL | CRC_FLAGS_COLORWRITE);
			pA->Attrib_Disable(CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_ZWRITE);

			pA->Attrib_StencilRef(128, 255);
			pA->Attrib_StencilFrontOp(CRC_COMPARE_LESSEQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
			pA->Attrib_StencilWriteMask(0);
		}
		ms_pBase = pA;
	}

	void Create(CXR_Shader* _pShader, const CXR_ShaderParams* _pShaderParams, const CXR_SurfaceShaderParams* _pSurfParams, int _TextureID_Projection1, int _TextureID_Projection2, fp32* _pTexGenAttr, CRC_ExtAttributes_FragmentProgram20* _pFP)
	{
		const CXR_SurfaceShaderParams* pSurfParams = _pSurfParams;
		m_pBaseAttrib = ms_pBase;
		m_Class = 0x40;
		m_TextureID_Diffuse = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE] : _pShader->m_TextureID_Special_ffffffff;
		m_TextureID_Specular = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_SPECULAR]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_SPECULAR] : _pShader->m_TextureID_Special_ffffffff;
		m_TextureID_Normal = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_NORMAL]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_NORMAL] : _pShader->m_TextureID_DefaultNormal;
		m_TextureID_Attribute = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_ATTRIBUTE]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_ATTRIBUTE] : _pShader->m_TextureID_Special_ff000000;
		m_TextureID_Transmission = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_TRANSMISSION]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_TRANSMISSION] : _pShader->m_TextureID_Special_ff000000;
		m_TextureID_AnisotropicDir = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_ANISOTROPICDIR]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_ANISOTROPICDIR] : _pShader->m_TextureID_Special_ff000000;
		m_TextureID_Projection1 = _TextureID_Projection1;
		m_TextureID_Projection2 = _TextureID_Projection2;
		m_TextureID_Environment = (pSurfParams->m_lTextureIDs[XR_SHADERMAP_ENVIRONMENT]) ? pSurfParams->m_lTextureIDs[XR_SHADERMAP_ENVIRONMENT] : _pShader->m_TextureID_Special_Cube_ff000000;
		m_ShaderParamFlags = _pShaderParams->m_Flags & (XR_SHADERFLAGS_USEZLESS | XR_SHADERFLAGS_NOSTENCILTEST | XR_SHADERFLAGS_CULLCW);
		m_iTexCoordSetMapping = _pShaderParams->m_iTexCoordSetMapping;
		m_iTexCoordSetTangentU = _pShaderParams->m_iTexCoordSetTangentU;
		m_iTexCoordSetTangentV = _pShaderParams->m_iTexCoordSetTangentV;
		m_pFP = _pFP;
		m_pTexGenAttr = _pTexGenAttr;
	}

	int OnCompare(const CXR_VirtualAttributes* _pOther)
	{
		const CXR_VirtualAttributes_ShaderFP20_Glass* pLast = (CXR_VirtualAttributes_ShaderFP20_Glass*)_pOther;
		XR_COMPAREATTRIBUTE_INT(m_pFP->m_ProgramNameHash, pLast->m_pFP->m_ProgramNameHash);
		XR_COMPAREATTRIBUTE_INT(m_ShaderParamFlags, pLast->m_ShaderParamFlags);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Diffuse, pLast->m_TextureID_Diffuse);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Specular, pLast->m_TextureID_Specular);
		XR_COMPAREATTRIBUTE_INT(m_TextureID_Normal, pLast->m_TextureID_Normal);
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
			_pDest->Attrib_TextureID(0, m_TextureID_Diffuse);
			_pDest->Attrib_TextureID(1, m_TextureID_Specular);
			_pDest->Attrib_TextureID(2, m_TextureID_Normal);
			_pDest->Attrib_TextureID(3, m_TextureID_Attribute);
			_pDest->Attrib_TextureID(4, m_TextureID_Transmission);
			_pDest->Attrib_TextureID(5, m_TextureID_Projection1);
			_pDest->Attrib_TextureID(6, m_TextureID_Projection2);
			_pDest->Attrib_TextureID(7, m_TextureID_Environment);
			_pDest->Attrib_TextureID(8, m_TextureID_AnisotropicDir);

			int StencilTest = (m_ShaderParamFlags & XR_SHADERFLAGS_NOSTENCILTEST) ? 0 : CRC_FLAGS_STENCIL;
			int CullXW = (m_ShaderParamFlags & XR_SHADERFLAGS_CULLCW) ? CRC_FLAGS_CULLCW : 0;
			_pDest->Attrib_Enable(CullXW | StencilTest);
			
			_pDest->m_pExtAttrib = m_pFP;
			_pDest->m_ZCompare = (m_ShaderParamFlags & XR_SHADERFLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;
		}
		else
		{
			CXR_VirtualAttributes_ShaderFP20_Glass *pLast = (CXR_VirtualAttributes_ShaderFP20_Glass *)_pLastAttr;

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

			_pDest->Attrib_TextureID(0, m_TextureID_Diffuse);
			_pDest->Attrib_TextureID(1, m_TextureID_Specular);
			_pDest->Attrib_TextureID(2, m_TextureID_Normal);
			_pDest->Attrib_TextureID(3, m_TextureID_Attribute);
			_pDest->Attrib_TextureID(4, m_TextureID_Transmission);
			_pDest->Attrib_TextureID(5, m_TextureID_Projection1);
			_pDest->Attrib_TextureID(6, m_TextureID_Projection2);
			_pDest->Attrib_TextureID(7, m_TextureID_Environment);
			_pDest->Attrib_TextureID(8, m_TextureID_AnisotropicDir);
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

CRC_Attributes* CXR_VirtualAttributes_ShaderFP20_Glass::ms_pBase = NULL;

void CXR_Shader::RenderShading_FP20_Glass(const CXR_Light& _Light, CXR_VertexBufferGeometry* _pVB, const CXR_ShaderParams* _pShaderParams, const CXR_SurfaceShaderParams* _pSurfParams)
{
	MAUTOSTRIP(CXR_Shader_RenderShading_FP20_Glass, MAUTOSTRIP_VOID);
	const CXR_SurfaceShaderParams* pSurfParams = _pSurfParams;

	// Assume vertex buffers isn't placed on stack!!

	const uint16 nVB = _pShaderParams->m_nVB;
    const int TextureID_Diffuse = pSurfParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE];
	if(!TextureID_Diffuse)
		return;

	int TextureIDProj = _Light.m_ProjMapID;
	if(!TextureIDProj && _Light.m_Type == CXR_LIGHTTYPE_SPOT)
		TextureIDProj = m_TextureID_DefaultLens;
	if(!TextureIDProj)
		TextureIDProj = m_TextureID_Special_Cube_ffffffff;

	// TSLV * 2
	int nTexGenSize = 4*2;

	// Projectionmap params
	if( TextureIDProj )
		nTexGenSize += 3*4;

	// BumpCubeEnv texgen
	nTexGenSize += 4*4*2;

	// Allocate needed data to run all vertex buffers
	int VBRamRequired = Align(sizeof(fp32) * nTexGenSize, 4)
					  + Align(sizeof(CRC_ExtAttributes_FragmentProgram20), 4)
					  + Align(sizeof(CVec4Dfp32) * 8, 16)
					  + Align(sizeof(CXR_VirtualAttributes_ShaderFP20_Glass), 4)
					  + Align(sizeof(CXR_VertexBuffer) * nVB, 4);

	uint8* pVBMData = (uint8*)m_pVBM->Alloc(VBRamRequired);
	uint8* pVBMStart = pVBMData;	// TODO: Needed??!
	if(!pVBMData)
		return;

	// -------------------------------------------------------------------
	//	-Pass 1:
	//		# Texture0 = Diffuse Map		// Default = 1,1,1,1
	//		# Texture1 = Specular Map		// Default = 1,1,1,1
	//		# Texture2 = Normal map			// Default = 1, 0.5, 0.5
	//		# Texture3 = Attribute Map		// r = Fresnel, g = Silkness, b = ?, a = ?, Default = 0,0,0,0
	//		# Texture4 = Transmission Diffuse Map	// Default = 0,0,0,0
	//		# Texture5 = Projection Map 1		// Default = 1,1,1
	//		# Texture6 = Projection Map 2		// Default = 1,1,1
	//		# Texture7 = Environment Map		// Default = 0,0,0
	//
	//		#-----------------------------------------
	//		# TexCoord0 = Mapping tex coord
	//		# TexCoord1 = Animated model space pixel position
	//		# TexCoord2 = Interpolated tangent space light vector (IPTSLV)
	//		# TexCoord3 = Interpolated tangent space eye vector (IPTSEV)
	//		# TexCoord4 = ProjMap tex coord
	//		# TexCoord5 = Tangentspace-2-world transform, row 0
	//		# TexCoord6 = Tangentspace-2-world transform, row 1
	//		# TexCoord7 = Tangentspace-2-world transform, row 2
	//
	// ------------------------------------------------------------------- 

	CVec4Dfp32* pParams = (CVec4Dfp32*)pVBMData; pVBMData += Align(sizeof(CVec4Dfp32) * 8, 16);

	// Texgen for attenuation
	fp32 Range = _Light.m_Range;
	fp32 Offset = 0.5f;

	const CVec3Dfp32& Pos = _Light.GetPosition();
	const CVec3Dfp32& Dir = _Light.GetDirection();

	fp32* pTexGenAttr = (fp32*)pVBMData; pVBMData += Align(sizeof(fp32) * nTexGenSize, 4);
	int nTexGenPos = 0;

	// -------------------------------------------------------------------
	// TSLV texgen parameters
	pTexGenAttr[nTexGenPos++] = Pos.k[0];
	pTexGenAttr[nTexGenPos++] = Pos.k[1];
	pTexGenAttr[nTexGenPos++] = Pos.k[2];
	pTexGenAttr[nTexGenPos++] = 0.03125f;	// 1.0f / 32.0f
	// -------------------------------------------------------------------

	const CMat4Dfp32* pMat = _pVB->m_pTransform;
	CVec3Dfp32 Eye;
	Eye.k[0] = - (pMat->k[3][0]*pMat->k[0][0] + pMat->k[3][1]*pMat->k[0][1] + pMat->k[3][2]*pMat->k[0][2]);
	Eye.k[1] = - (pMat->k[3][0]*pMat->k[1][0] + pMat->k[3][1]*pMat->k[1][1] + pMat->k[3][2]*pMat->k[1][2]);
	Eye.k[2] = - (pMat->k[3][0]*pMat->k[2][0] + pMat->k[3][1]*pMat->k[2][1] + pMat->k[3][2]*pMat->k[2][2]);

	pTexGenAttr[nTexGenPos++] = Eye.k[0];
	pTexGenAttr[nTexGenPos++] = Eye.k[1];
	pTexGenAttr[nTexGenPos++] = Eye.k[2];
	pTexGenAttr[nTexGenPos++] = 0.03125f;	// 1.0f / 32.0f

	// -------------------------------------------------------------------
	// Projection map texgen parameters
	if( 1 || TextureIDProj )
	{
		CPlane3Dfp32* pTexGenProj = (CPlane3Dfp32*)&(pTexGenAttr[nTexGenPos]);
		nTexGenPos += 4*3;

		const CMat4Dfp32* pProjPos = (_Light.m_Flags & CXR_LIGHT_PROJMAPTRANSFORM) ?
			&_Light.m_ProjMapTransform : &_Light.m_Pos;

		const CVec3Dfp32& ProjX = pProjPos->GetRow(0);
		const CVec3Dfp32  ProjY = pProjPos->GetRow(1) * (1.0f / _Light.m_SpotWidth);
		const CVec3Dfp32  ProjZ = pProjPos->GetRow(2) * (1.0f / _Light.m_SpotHeight);
		const CVec3Dfp32& ProjP = pProjPos->GetRow(3);

		pTexGenProj[0].CreateND(ProjX, -(ProjX * Pos));
		pTexGenProj[1].CreateND(ProjY, -(ProjY * Pos));
		pTexGenProj[2].CreateND(ProjZ, -(ProjZ * Pos));
	}

	// -------------------------------------------------------------------
	// BumpCubeEnv texgen parameters
	if (_pShaderParams->m_pCurrentWMat)
		_pShaderParams->m_pCurrentWMat->InverseOrthogonal(* (CMat4Dfp32*)&pTexGenAttr[nTexGenPos]);
	else
		((CMat4Dfp32*)&pTexGenAttr[nTexGenPos])->Unit();
	nTexGenPos += 16;

	if (_pShaderParams->m_pCurrentVMat)
		_pShaderParams->m_pCurrentVMat->InverseOrthogonal(*(CMat4Dfp32*)&pTexGenAttr[nTexGenPos]);
	else
		((CMat4Dfp32*)&pTexGenAttr[nTexGenPos])->Unit();
	nTexGenPos += 16;
	// -------------------------------------------------------------------

	CRC_ExtAttributes_FragmentProgram20* pFP = (CRC_ExtAttributes_FragmentProgram20*)pVBMData;
	pVBMData += Align(sizeof(CRC_ExtAttributes_FragmentProgram20), 4);
	pFP->Clear();

	pFP->SetParameters(pParams, 8);
	CVec4Dfp32& LightPos = pParams[0];
	CVec4Dfp32& LightRange = pParams[1];
	CVec4Dfp32& LightColor = pParams[2];
	CVec4Dfp32& SpecColor = pParams[3];
	CVec4Dfp32& AttribScale = pParams[4];
	CVec4Dfp32& EnvColor = pParams[5];
	CVec4Dfp32& TransmissionColor = pParams[6];
	CVec4Dfp32& NoiseOffset = pParams[7];

	// Light positions
	LightPos.k[0] = Pos.k[0];
	LightPos.k[1] = Pos.k[1];
	LightPos.k[2] = Pos.k[2];
	LightPos.k[3] = 1;

	// Light range
	LightRange.k[0] = _Light.m_RangeInv;
	LightRange.k[1] = _Light.m_Range;
	LightRange.k[2] = Sqr(_Light.m_RangeInv);
	LightRange.k[3] = Sqr(_Light.m_Range);

	// Light color
	vec128 Diff = M_VLd_V4f16_f32(&pSurfParams->m_DiffuseColor);
	LightColor.v = M_VMul(M_VMul(_Light.GetIntensityv().v, m_DiffuseScale.v), Diff);
	LightColor.k[3]	= pSurfParams->m_SpecularAnisotrophy;

	// Spec color
	vec128 Spec = M_VLd_V4f16_f32(&pSurfParams->m_SpecularColor);
	SpecColor.v = M_VMul(M_VMul(_Light.GetIntensityv().v, m_SpecularScale.v), Spec);

	if (m_SpecularForcePower)
		SpecColor.k[3] = m_SpecularForcePower;

	AttribScale << pSurfParams->m_AttribScale;
	EnvColor << pSurfParams->m_EnvColor;
	TransmissionColor << pSurfParams->m_TransmissionColor;

	CMTime Time;
	if (m_pEngine)
		Time = m_pEngine->GetEngineTime();
	else
		Time.Reset();

	NoiseOffset[0] = Time.GetTimeModulusScaled(0.01f, 1.0f);
	NoiseOffset[1] = Time.GetTimeModulusScaled(0.0087f, 1.0f);
	NoiseOffset[2] = Time.GetTimeModulusScaled(0.007f, 1.0f);
	NoiseOffset[3] = Time.GetTimeModulusScaled(0.13f, 1.0f);

	int iShader = XRSHADER_FP20_NDSEATP;
	if (_Light.m_LightGUID == 0x2346)
	{
		// Darkness vision special light, (This may be removed when not needed anymore)
		pTexGenAttr[2] += Dir.k[1];
		LightPos.k[2] += Dir.k[0];
	}
	pFP->SetProgram(ms_lProgramsShading20[iShader].m_pName, ms_lProgramsShading20[iShader].m_Hash);

	// Alloc VA
	void* pVAMem = pVBMData;
	pVBMData += Align(sizeof(CXR_VirtualAttributes_ShaderFP20_Glass), 4);
	CXR_VirtualAttributes_ShaderFP20_Glass* pVA = new(pVAMem) CXR_VirtualAttributes_ShaderFP20_Glass;
	pVA->Create(this, _pShaderParams, pSurfParams, TextureIDProj, TextureIDProj, (fp32*)pTexGenAttr, pFP);

	const fp32 Priority = (_pShaderParams->m_Flags & XR_SHADERFLAGS_TRANSPARENT) ? _pShaderParams->m_Priority : (_Light.m_iLight + 0.3f);

//	CXR_VertexBuffer* pVB

//	uint8* pVMBVBData = pVBMData;
	for(uint16 i = 0; i < nVB; i++)
	{
//		CXR_VertexBuffer* pVB = &_pVB[i];
		CXR_VertexBuffer* pVB = (CXR_VertexBuffer*)pVBMData;
		pVB->Construct();
		pVBMData += sizeof(CXR_VertexBuffer);
		pVB->CopyVBChainAndTransform(&_pVB[i]);

		pVB->SetVirtualAttr(pVA);
		pVB->m_iLight = _Light.m_iLight;
		pVB->m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
		pVB->m_Priority = Priority;

		pVB->SetVBEColor(0xff40ff80);
		m_pVBM->AddVB(pVB);
	}

	M_ASSERT(pVBMData == (pVBMStart + VBRamRequired), "CXR_Shader_RenderShadingFP20_Glass: VBRam not initialized properly!");
	//pVBMData += Align(sizeof(CXR_VertexBuffer) * nVB, 4);
}

