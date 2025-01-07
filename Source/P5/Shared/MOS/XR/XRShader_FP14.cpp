
void CXR_Shader::RenderShading_FP14(const CXR_Light& _Light, CXR_VertexBuffer* _pVB, const CXR_ShaderParams* _pShaderParams)
{
	MAUTOSTRIP(CXR_Shader_RenderShading_FP14, MAUTOSTRIP_VOID);

	{
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VBAttrib();
		if (!pVB) 
			return;

		CRC_Attributes* pA = pVB->m_pAttrib;

		pA->m_iTexCoordSet[0] = _pShaderParams->m_iTexCoordSetMapping;
		pA->m_iTexCoordSet[1] = _pShaderParams->m_iTexCoordSetMapping;
		pA->m_iTexCoordSet[2] = _pShaderParams->m_iTexCoordSetTangentU;
		pA->m_iTexCoordSet[3] = _pShaderParams->m_iTexCoordSetTangentV;

		pVB->CopyVBChain(_pVB);
		pVB->Matrix_Set(_pVB->m_pTransform);
		pVB->m_pMatrixPaletteArgs = _pVB->m_pMatrixPaletteArgs;

		int iSpecMap = 0;
		int Power = (m_SpecularForcePower) ? m_SpecularForcePower : _pShaderParams->m_SpecularPower;
		while((1 << iSpecMap) < Power)
		{
			iSpecMap++;
		}

		if (iSpecMap > 7)
			iSpecMap = 7;

		int TextureIDSpecFunc = m_lTextureID_Specular[iSpecMap];

		int TextureIDProj = _Light.m_ProjMapID;
		if (!TextureIDProj && _Light.m_Type == CXR_LIGHTTYPE_SPOT)
			TextureIDProj = m_TextureID_DefaultLens;

		int nTexGenAttr = 4 + 2*4;
		if (TextureIDProj)
			nTexGenAttr += 3 * 4;

		fp32* pTexGenAttr = (fp32*) m_pVBM->Alloc_fp32(nTexGenAttr);
		if (!pTexGenAttr)
			return;

		pVB->m_pAttrib->Attrib_TexGenAttr(pTexGenAttr);

		*((CVec3Dfp32*)pTexGenAttr) = _Light.GetPosition();
		pTexGenAttr += 3;
		*pTexGenAttr = _Light.m_RangeInv;
		pTexGenAttr += 1;

		pA->Attrib_TextureID(0, _pShaderParams->m_lTextureIDs[XR_SHADERMAP_NORMAL]);
		pA->Attrib_TextureID(1, _pShaderParams->m_lTextureIDs[XR_SHADERMAP_DIFFUSE]);
		pA->Attrib_TextureID(2, m_TextureID_Normalize);
		pA->Attrib_TextureID(3, TextureIDSpecFunc);

		pVB->m_pAttrib->Attrib_TexGen(1, CRC_TEXGENMODE_TSLV, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V | CRC_TEXGENCOMP_W );
		pVB->m_pAttrib->Attrib_TexGen(3, CRC_TEXGENMODE_TSREFLECTION, CRC_TEXGENCOMP_ALL);
		pVB->m_pAttrib->Attrib_TexGen(4, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);
		pVB->m_pAttrib->Attrib_TexGen(5, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);


		if (TextureIDProj)
		{
			pVB->m_pAttrib->Attrib_TextureID(4, TextureIDProj);

			const CMat4Dfp32* pProjPos = (_Light.m_Flags & CXR_LIGHT_PROJMAPTRANSFORM) ? 
				&_Light.m_ProjMapTransform : &_Light.m_Pos;

			const CVec3Dfp32& ProjX = CVec3Dfp32::GetRow(*pProjPos, 0);
			CVec3Dfp32 ProjY; CVec3Dfp32::GetRow(*pProjPos, 1).Scale(1.0f / _Light.m_SpotWidth, ProjY);
			CVec3Dfp32 ProjZ; CVec3Dfp32::GetRow(*pProjPos, 2).Scale(1.0f / _Light.m_SpotHeight, ProjZ);
			const CVec3Dfp32& Pos = CVec3Dfp32::GetRow(*pProjPos, 3);

			CPlane3Dfp32* pProjTexAttr = (CPlane3Dfp32*)pTexGenAttr;

			pProjTexAttr[0].CreateND(ProjX, -(ProjX * Pos));
			pProjTexAttr[1].CreateND(ProjY, -(ProjY * Pos));
			pProjTexAttr[2].CreateND(ProjZ, -(ProjZ * Pos));
			pVB->m_pAttrib->Attrib_TexGen(2, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V | CRC_TEXGENCOMP_W);
			pTexGenAttr += 3*4;
		}

		{
			CVec4Dfp32* pLightVec = (CVec4Dfp32*)pTexGenAttr;
			if (!pLightVec)
				return;
			CVec3Dfp32 Pos = _Light.GetPosition();
			pLightVec[0][0] = Pos[0];
			pLightVec[0][1] = Pos[1];
			pLightVec[0][2] = Pos[2];
			pLightVec[0][3] = 1;
			const CMat4Dfp32* pMat = _pVB->m_pTransform;
			for (int j = 0; j < 3; j++)
				pLightVec[1][j] = - (pMat->k[3][0]*pMat->k[j][0] + pMat->k[3][1]*pMat->k[j][1] + pMat->k[3][2]*pMat->k[j][2]);
			pLightVec[1][3] = 1;
			pTexGenAttr += 2*4;
		}


		CRC_ExtAttributes_FragmentProgram14 *pShader = (CRC_ExtAttributes_FragmentProgram14*) m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram14));
		if (!pShader) return;
		pShader->Clear();

		const char* lpShaders[] =
		{
			"XRShader_SpecNormal",
			"XRShader_SpecDiffuse",
			"XRShader_Proj_SpecNormal",
			"XRShader_Proj_SpecDiffuse",
		};
		int bSpecularDiffuse = (_pShaderParams->m_Flags & XR_SHADERFLAGS_SPECULARINDIFFUSE);
		int iShader = (bSpecularDiffuse ? 1 : 0) | (TextureIDProj ? 2 : 0);

		pShader->m_pProgramName = lpShaders[iShader];

		CVec4Dfp32* pShaderConst = m_pVBM->Alloc_V4(2);

		pShader->m_pParams = pShaderConst;
		pShader->m_nParams = 2;

		CVec4Dfp32& LightColor = pShaderConst[0];
		CVec4Dfp32& SpecColor = pShaderConst[1];
		LightColor.k[0]	= _Light.m_Intensity[0] * m_DiffuseScale[0] * _pShaderParams->m_DiffuseColor.k[0];
		LightColor.k[1]	= _Light.m_Intensity[1] * m_DiffuseScale[1] * _pShaderParams->m_DiffuseColor.k[1];
		LightColor.k[2]	= _Light.m_Intensity[2] * m_DiffuseScale[2] * _pShaderParams->m_DiffuseColor.k[2];
		LightColor.k[3]	= 0.0f;

		SpecColor.k[0] = _Light.m_Intensity[0] * m_SpecularScale[0] * _pShaderParams->m_SpecularColor.k[0];
		SpecColor.k[1] = _Light.m_Intensity[1] * m_SpecularScale[1] * _pShaderParams->m_SpecularColor.k[1];
		SpecColor.k[2] = _Light.m_Intensity[2] * m_SpecularScale[2] * _pShaderParams->m_SpecularColor.k[2];
		SpecColor.k[3]	= (fp32(Power) - 0.5f) / 127.0f;


		int StencilTest = (_pShaderParams->m_Flags & XR_SHADERFLAGS_NOSTENCILTEST) ? 0 : CRC_FLAGS_STENCIL;
		if (StencilTest)
		{
			pA->Attrib_StencilRef(128, 255);
			pA->Attrib_StencilFrontOp(CRC_COMPARE_LESSEQUAL, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE);
			pA->Attrib_StencilWriteMask(0);
		}

		{
			pA->Attrib_SourceBlend(CRC_BLEND_ONE);
			pA->Attrib_DestBlend(CRC_BLEND_ONE);
			pA->Attrib_Enable(CRC_FLAGS_BLEND | CRC_FLAGS_CULL | CRC_FLAGS_COLORWRITE | StencilTest);
			pA->Attrib_Disable(CRC_FLAGS_ALPHAWRITE);

			int ZCompare = (_pShaderParams->m_Flags & XR_SHADERFLAGS_USEZLESS) ? CRC_COMPARE_LESSEQUAL : CRC_COMPARE_EQUAL;
			pA->Attrib_ZCompare(ZCompare);
		}
		pVB->m_Priority = _Light.m_iLight + 0.3f;

		pA->m_pExtAttrib = pShader;
		m_pVBM->AddVB(pVB);
	}
}

