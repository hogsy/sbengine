#include "PCH.h"

#include "MDisplayPS3.h"
#include "MRenderPS3_Context.h"
#include "MRenderPS3_Def.h"

#ifdef PLATFORM_WIN32_PC
#define GL_CLIP_PLANE0 0x3000
#endif

const char* gs_TexEnvPrograms[9] = {"TexEnvMode00", "TexEnvMode01", "TexEnvMode02", "TexEnvMode03", "TexEnvMode04", "TexEnvMode05", "TexEnvMode06", "TexEnvMode07", "TexEnvMode08"};
uint32 gs_TexEnvProgramHash[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

//----------------------------------------------------------------
// Attribute overrides
//----------------------------------------------------------------
static int ConvertToGLBlend(int _Blend)
{
	switch(_Blend)
	{
	case CRC_BLEND_ZERO : return GL_ZERO;
	case CRC_BLEND_ONE : return GL_ONE;
	case CRC_BLEND_SRCCOLOR : return GL_SRC_COLOR;
	case CRC_BLEND_INVSRCCOLOR : return GL_ONE_MINUS_SRC_COLOR;
	case CRC_BLEND_SRCALPHA : return GL_SRC_ALPHA;
	case CRC_BLEND_INVSRCALPHA : return GL_ONE_MINUS_SRC_ALPHA;
	case CRC_BLEND_DESTALPHA : return GL_DST_ALPHA;
	case CRC_BLEND_INVDESTALPHA : return GL_ONE_MINUS_DST_ALPHA;
	case CRC_BLEND_DESTCOLOR : return GL_DST_COLOR;
	case CRC_BLEND_INVDESTCOLOR : return GL_ONE_MINUS_DST_COLOR;
	case CRC_BLEND_SRCALPHASAT : return GL_SRC_ALPHA_SATURATE;
	default : return GL_ONE;
	}
}

static int ConvertToGLCompare(int _Compare)
{
	switch(_Compare)
	{
	case CRC_COMPARE_NEVER : return GL_NEVER;
	case CRC_COMPARE_LESS : return GL_LESS;
	case CRC_COMPARE_EQUAL : return GL_EQUAL;
	case CRC_COMPARE_LESSEQUAL : return GL_LEQUAL;
	case CRC_COMPARE_GREATER : return GL_GREATER;
	case CRC_COMPARE_NOTEQUAL : return GL_NOTEQUAL;
	case CRC_COMPARE_GREATEREQUAL : return GL_GEQUAL;
	case CRC_COMPARE_ALWAYS	: return GL_ALWAYS;
	default : return GL_LEQUAL;
	}
}

static int ConvertToGLStencilOp(int _Op)
{
	switch(_Op)
	{
	case CRC_STENCILOP_NONE : return GL_KEEP;
	case CRC_STENCILOP_ZERO : return GL_ZERO;
	case CRC_STENCILOP_REPLACE : return GL_REPLACE;
	case CRC_STENCILOP_INC : return GL_INCR;
	case CRC_STENCILOP_DEC : return GL_DECR;
	case CRC_STENCILOP_INVERT : return GL_INVERT;
	case CRC_STENCILOP_INCWRAP : return GL_INCR_WRAP;
	case CRC_STENCILOP_DECWRAP : return GL_DECR_WRAP;
	default : return GL_KEEP;
	}
}

bint CRenderContextPS3::Attrib_SetRasterMode(CRC_Attributes* _pA, int _RasterMode)
{
	if (_RasterMode != CRC_RASTERMODE_NONE)
		_pA->ClearRasterModeSettings();

	switch(_RasterMode)
	{
		case CRC_RASTERMODE_ALPHABLEND :
			{
				_pA->m_Flags |= CRC_FLAGS_BLEND;
				_pA->m_SourceBlend = CRC_BLEND_SRCALPHA;
				_pA->m_DestBlend = CRC_BLEND_INVSRCALPHA;
//				_pA->m_SourceBlend = CRC_BLEND_ONE;
//				_pA->m_DestBlend = CRC_BLEND_INVSRCCOLOR;
				return true;
			}
		case CRC_RASTERMODE_LIGHTMAPBLEND :
			{
				// Jag hoggade den här rastermoden till lite fifflande  /mh

				_pA->m_Flags |= CRC_FLAGS_BLEND;
				_pA->m_SourceBlend = CRC_BLEND_ZERO;
				_pA->m_DestBlend = CRC_BLEND_INVSRCCOLOR;


/*				_pA->m_Flags |= CRC_FLAGS_BLEND;
#ifdef CRCGL_ALPHALIGHTMAPS
				if (m_bAlphaLightMaps)
				{
					_pA->m_SourceBlend = CRC_BLEND_SRCALPHA;
					_pA->m_DestBlend = CRC_BLEND_INVSRCALPHA;
				}
				else
#endif
				{
					_pA->m_SourceBlend = CRC_BLEND_DESTCOLOR;
					_pA->m_DestBlend = CRC_BLEND_ZERO;
//					_pA->m_DestBlend = CRC_BLEND_SRCCOLOR;
				}*/
				return true;
			}
		case CRC_RASTERMODE_MULTIPLY :
			{
				_pA->m_Flags |= CRC_FLAGS_BLEND;
				_pA->m_SourceBlend = CRC_BLEND_DESTCOLOR;
				_pA->m_DestBlend = CRC_BLEND_ZERO;
				return true;
			}
		case CRC_RASTERMODE_ADD :
			{
				_pA->m_Flags |= CRC_FLAGS_BLEND;
				_pA->m_SourceBlend = CRC_BLEND_ONE;
				_pA->m_DestBlend = CRC_BLEND_ONE;
				return true;
			}
		case CRC_RASTERMODE_ALPHAADD :
			{
				_pA->m_Flags |= CRC_FLAGS_BLEND;
				_pA->m_SourceBlend = CRC_BLEND_SRCALPHA;
				_pA->m_DestBlend = CRC_BLEND_ONE;
				return true;
			}
		case CRC_RASTERMODE_ALPHAMULTIPLY :
			{
				_pA->m_Flags |= CRC_FLAGS_BLEND;
				_pA->m_SourceBlend = CRC_BLEND_DESTCOLOR;
				_pA->m_DestBlend = CRC_BLEND_INVSRCALPHA;
				return true;
			}

		case CRC_RASTERMODE_MULTIPLY2 :
			{
				_pA->m_Flags |= CRC_FLAGS_BLEND;
				_pA->m_SourceBlend = CRC_BLEND_DESTCOLOR;
				_pA->m_DestBlend = CRC_BLEND_SRCCOLOR;
				return true;
			}
		case CRC_RASTERMODE_MULADD :
			{
				_pA->m_Flags |= CRC_FLAGS_BLEND;
				_pA->m_SourceBlend = CRC_BLEND_DESTCOLOR;
				_pA->m_DestBlend = CRC_BLEND_ONE;
				return true;
			}
		case CRC_RASTERMODE_ONE_ALPHA :
			{
				_pA->m_Flags |= CRC_FLAGS_BLEND;
				_pA->m_SourceBlend = CRC_BLEND_ONE;
				_pA->m_DestBlend = CRC_BLEND_SRCALPHA;
				return true;
			}
		case CRC_RASTERMODE_ONE_INVALPHA :
			{
				_pA->m_Flags |= CRC_FLAGS_BLEND;
				_pA->m_SourceBlend = CRC_BLEND_ONE;
				_pA->m_DestBlend = CRC_BLEND_INVSRCALPHA;
				return true;
			}
		case CRC_RASTERMODE_DESTALPHABLEND :
			{
				_pA->m_Flags |= CRC_FLAGS_BLEND;
				_pA->m_SourceBlend = CRC_BLEND_DESTALPHA;
				_pA->m_DestBlend = CRC_BLEND_INVDESTALPHA;
				return true;
			}

		case CRC_RASTERMODE_DESTADD :
			{
				_pA->m_Flags |= CRC_FLAGS_BLEND;
				_pA->m_SourceBlend = CRC_BLEND_DESTCOLOR;	// source.rgb must be 255,255,255
				_pA->m_DestBlend = CRC_BLEND_SRCALPHA;		// source.a can range from 0 (no effect) to 255 (full effect)
				return true;
			}

		case CRC_RASTERMODE_MULADD_DESTALPHA:
			{
				_pA->m_Flags |= CRC_FLAGS_BLEND;
				_pA->m_SourceBlend = CRC_BLEND_DESTALPHA;
				_pA->m_DestBlend = CRC_BLEND_ONE;
				return true;
			}
	}
	return true;
}


static int IsTexGenEqual(const CRC_Attributes& _A0, const CRC_Attributes& _A1)
{
	if (_A0.m_TexGenComp != _A1.m_TexGenComp)
		return false ;
	if (_A0.m_pTexGenAttr != _A1.m_pTexGenAttr)
		return false ;

	for(int iTxt = 0; iTxt < CRC_MAXTEXCOORDS; iTxt++)
	{
		if (_A0.m_lTexGenMode[iTxt] != _A1.m_lTexGenMode[iTxt])
			return false ;
	}

	return true;
}


class CGetNewRegsEmpty
{
public:
	static int NewRegisterBatch(CVec4Dfp4* &_pRegisters, int &_nRegsNeeded)
	{
		return 256;
	}

};

void CRenderContextPS3::Attrib_SetVPPipeline(CRC_Attributes* _pAttrib)
{
	GLErr("Attrib_SetVPPipeline");

	CRC_MatrixState& MS = Matrix_GetState();

	// Get texture matrices
	const CMat4Dfp4* lpTexMat[CRC_MAXTEXCOORDS];
	{
//		for(int t = CRC_MATRIX_TEXTURE0; t <= CRC_MATRIX_TEXTURE3; t++)
//			lpTexMat[t - CRC_MATRIX_TEXTURE0 ] = (MS.m_MatrixUnit & (1 << t)) ? NULL : &MS.m_lMatrices[t];
		for(int t = 0; t < CRC_MAXTEXCOORDS; t++)
		{
			if( t < CRC_MAXTEXCOORDS )
				lpTexMat[t] = (MS.m_MatrixUnit & (1 << (t+CRC_MATRIX_TEXTURE0))) ? NULL : &MS.m_lMatrices[t+CRC_MATRIX_TEXTURE0];
			else
				lpTexMat[t] = 0;
		}
	}

	CRC_VPFormat VPFormat(false, 8);
	VPFormat.m_iConstant_Base = 0;	//TEMP

	VPFormat.SetAttrib(_pAttrib, lpTexMat);
	if (_pAttrib->m_Flags & CRC_FLAGS_LIGHTING)
	{
		VPFormat.SetLights(_pAttrib->m_pLights, _pAttrib->m_nLights);
	}

	if (Clip_IsEnabled())
	{
		VPFormat.SetClipPlanes(true);
	}

	if (MS.m_pMatrixPaletteArgs)
	{
		VPFormat.SetMPFlags( MS.m_pMatrixPaletteArgs->m_Flags & ERC_MatrixPaletteFlags_SpecialCubeFlags );
	}

	if (MS.m_pMatrixPaletteArgs)
	{
		VPFormat.SetMWComp(4);
	}

	{
		for(int t = 0; t < CRC_MAXTEXCOORDS; t++)
			if (lpTexMat[t]) 
				VPFormat.SetTexMatrix(t);
	}
	int iRegFirst = CRC_VPFormat::GetFirstFreeRegister();
	int iReg = iRegFirst;
	{
		iReg += VPFormat.SetRegisters_Init(&(m_VPConstRegisters[iReg]), 0, iReg);
		if (Clip_IsEnabled())
		{
			CRC_ClipStackEntry& Clip = m_lClipStack[m_iClipStack];
			CMat4Dfp4 VMatInv;
			MS.m_lMatrices[CRC_MATRIX_MODEL].InverseOrthogonal(VMatInv);

			CPlane3Dfp4 Planes[6];
			int nP = Clip.m_nPlanes;
			memcpy(&Planes, Clip.m_Planes, sizeof(CPlane3Dfp4) * nP);
			for(int i = 0; i < nP; i++)
			{
				Planes[i].Transform(VMatInv);
			}

	/*		CVec4Dfp4 Planes[6];
			int nP = Clip.m_nPlanes;
			memcpy(&Planes, Clip.m_Planes, sizeof(CPlane3Dfp4) * nP);
			for(int i = 0; i < nP; i++)
				Planes[i] *= m_Matrix_ProjectionGL;
	*/
			iReg += VPFormat.SetRegisters_ClipPlanes<CGetNewRegsEmpty>(&(m_VPConstRegisters[iReg]), iReg, (CPlane3Dfp4*)Planes, nP);
		}

		iReg += VPFormat.SetRegisters_ConstantColor(&(m_VPConstRegisters[iReg]), iReg, m_GeomColor);
//		iReg += VPFormat.SetRegisters_ConstantColor(&(m_VPConstRegisters[iReg]), iReg, 0xffffffff);
		iReg += VPFormat.SetRegisters_Attrib(&(m_VPConstRegisters[iReg]), iReg, _pAttrib, this);
		CVec4Dfp4* pRegisters = &(m_VPConstRegisters[iReg]);
		iReg += VPFormat.SetRegisters_TexGenMatrix<CGetNewRegsEmpty>(pRegisters, iReg, lpTexMat, _pAttrib);
		m_VP_iConstantColor = VPFormat.m_iConstant_ConstantColor;
		for(int iTexMat = 0; iTexMat < CRC_MAXTEXCOORDS; iTexMat++)
		{
			m_VP_liTexMatrixReg[iTexMat] = VPFormat.m_iConstant_TexMatrix[iTexMat];
		}

		if (_pAttrib->m_Flags & CRC_FLAGS_LIGHTING)
		{
			CVec4Dfp4* pRegisters = &(m_VPConstRegisters[iReg]);
			iReg += VPFormat.SetRegisters_Lights<CGetNewRegsEmpty>(pRegisters, iReg, _pAttrib->m_pLights, _pAttrib->m_nLights);
		}
		pRegisters = &(m_VPConstRegisters[iReg]);
		
		if(MS.m_pMatrixPaletteArgs != NULL)
		{
			if(MS.m_pMatrixPaletteArgs->m_Flags & ERC_MatrixPaletteFlags_SpecialCubeFlags)
				iReg += VPFormat.SetRegisters_MatrixPaletteSpecialCube<CGetNewRegsEmpty>(pRegisters, iReg, CRCGLES_MAX_VPPROGRAMCONSTANTS - iReg, MS.m_pMatrixPaletteArgs);
			else
				iReg += VPFormat.SetRegisters_MatrixPalette<CGetNewRegsEmpty>(pRegisters, iReg, CRCGLES_MAX_VPPROGRAMCONSTANTS - iReg, MS.m_pMatrixPaletteArgs);
		}

		M_ASSERT(iReg < 256, "Too many constants for vertex program");
	}

	VP_Bind(VPFormat);
	GLErr("VP_Bind(1)");
	m_nUpdateVPConst = iReg;
	m_bUpdateVPConst = true;
}

void CRenderContextPS3::Attrib_SetPixelProgram(CRC_Attributes* _pAttrib)
{
	CRC_ExtAttributes* pExtAttrib = _pAttrib->m_pExtAttrib;
	if(!pExtAttrib)
	{
		static CRC_ExtAttributes_FragmentProgram30 ExtAttrib;
		static CVec4Dfp4 TexEnvParams[CRC_MAXTEXTUREENV];

		int i = 0;
		for(; i < CRC_MAXTEXTUREENV; i++)
		{
			if(!_pAttrib->m_TextureID[i])
				break;

			switch(_pAttrib->m_TexEnvMode[i])
			{
			case CRC_TEXENVMODE_MULTIPLY:
				{
					TexEnvParams[i] = 0.0f;
					break;
				}
			case CRC_TEXENVMODE_REPLACE:
				{
					TexEnvParams[i] = 1.0f;
					break;
				}
			default:
				{
#ifndef M_RTM
					M_TRACE("Unsupported texenvmode: %d\n", _pAttrib->m_TexEnvMode);
#endif
					break;
				}
			}
		}

		
		ExtAttrib.Clear();
		ExtAttrib.SetProgram(gs_TexEnvPrograms[i], gs_TexEnvProgramHash[i]);
		ExtAttrib.SetParameters(TexEnvParams, i);
		pExtAttrib = &ExtAttrib;
	}

	static bool bEnableDebugging = false;
	if(bEnableDebugging)
	{
		static CRC_ExtAttributes_FragmentProgram20 ExtAttrib;
		static CVec4Dfp4 TexEnvParams[4];
		ExtAttrib.Clear();
		ExtAttrib.SetProgram("XRShader_FP20_LF", StringToHash("XRShader_FP20_LF"));
		ExtAttrib.SetParameters(TexEnvParams, 0);
		pExtAttrib = &ExtAttrib;
	}

	switch(pExtAttrib->m_AttribType)
	{
	case CRC_ATTRIBTYPE_FP30:
	case CRC_ATTRIBTYPE_FP20:
		{
			FP_Bind((CRC_ExtAttributes_FragmentProgram20*)pExtAttrib);
			break;
		}

	default:
		{
			Error_static("CRenderContextPS3::Attrib_SetPixelProgram", "Unsupported attribtype");
		}
	}

}

void CRenderContextPS3::Attrib_Set(CRC_Attributes* _pAttrib)
{
	MSCOPE(CRenderContextPS3::Attrib_Set, RENDER_GL);
	GLErr("Attrib_Set (Entry)");

	for (int i = 0; i < CRC_MAXTEXTURES; ++i)
	{
		int TexID = _pAttrib->m_TextureID[i];
		if (TexID)
		{
			CRC_IDInfo &IDInfo = m_spTCIDInfo->m_pTCIDInfo[TexID];
			if (!(IDInfo.m_Fresh & 1))
			{
				CTC_TextureProperties Properties;
				m_pTC->GetTextureProperties(TexID, Properties);

				if (m_bAllowTextureLoad || (Properties.m_Flags & (CTC_TEXTUREFLAGS_PROCEDURAL | CTC_TEXTUREFLAGS_RENDER)))
				{
					if(!((Properties.m_Flags & (CTC_TEXTUREFLAGS_PROCEDURAL | CTC_TEXTUREFLAGS_RENDER))))
					{
						M_TRACEALWAYS("Texture %d '%s' is not loaded, precache = %s\r\n", TexID, m_pTC->GetName(TexID).Str(), (m_pTC->GetTextureFlags(TexID) & CTC_TXTIDFLAGS_PRECACHE)?"true":"false");
					}
//					GL_SetTexture(0, 0);
					GL_BuildTexture(TexID, TexID, Properties);
					IDInfo.m_Fresh |= 1;

					GL_SetTexture(0, 0);
					m_CurrentAttrib.m_TextureID[0] = 0;
					m_AttribChanged |= CRC_ATTRCHG_TEXTUREID;
				}
			}
		}
	}

	int AttrChg = m_AttribChanged;
	m_nAttribSet++;
	m_AttribChanged = 0;

	bool bVPNeedUpdate = false;

// URGENTFIXME: There's a render state update bug that is eliminated by forcing update of everything, thus:
// AttrChg = -1;


	if (m_Mode.m_Flags & CRC_GLOBALFLAGS_WIRE)
	{
		_pAttrib->m_ZCompare = CRC_COMPARE_ALWAYS;
		_pAttrib->m_Flags |= CRC_FLAGS_COLORWRITE;
		_pAttrib->m_Flags &= ~(CRC_FLAGS_STENCIL | CRC_FLAGS_SEPARATESTENCIL);

	}

	// -------------------------------------------------------------------
	if (AttrChg & CRC_ATTRCHG_RASTERMODE)
	{
		Attrib_SetRasterMode(_pAttrib, _pAttrib->m_RasterMode);
		m_CurrentAttrib.m_RasterMode = _pAttrib->m_RasterMode;
	}

	int Changed = (m_CurrentAttrib.m_Flags ^ _pAttrib->m_Flags);

	if (AttrChg & ~CRC_ATTRCHG_TEXTUREID)
	{
		// Different flags?
		if (AttrChg & CRC_ATTRCHG_FLAGS)
		if (m_CurrentAttrib.m_Flags != _pAttrib->m_Flags)
		{
			int f = _pAttrib->m_Flags;

			if (Changed & (CRC_FLAGS_COLORWRITE | CRC_FLAGS_ALPHAWRITE))
			{
				int ColorMask = (f & CRC_FLAGS_COLORWRITE) ? 1 : 0;
				int AlphaMask = (f & CRC_FLAGS_ALPHAWRITE) ? 1 : 0;
				glnColorMask(ColorMask, ColorMask, ColorMask, AlphaMask);
				MACRO_GLCOUNTSTATE(m_nStateEnableColorWrite);
				GLErr("Attrib_Set (ColorMask)");
			}

			if (Changed & CRC_FLAGS_ZWRITE)
			{
				glnDepthMask((f & CRC_FLAGS_ZWRITE) ? 1 : 0);
				//	glnStencilMask((f & CRC_FLAGS_ZWRITE) ? 0xffffffff : 0);	// Test to check fillrate improvement if stencil write is consistent with z-write.

				MACRO_GLCOUNTSTATE(m_nStateEnableZWrite);
				GLErr("Attrib_Set (ZWrite)");
			}

			if (Changed & CRC_FLAGS_ZCOMPARE)
			{
				if (f & CRC_FLAGS_ZCOMPARE)
					glnEnable(GL_DEPTH_TEST);
				else
					glnDisable(GL_DEPTH_TEST);
				MACRO_GLCOUNTSTATE(m_nStateEnableZTest);
				GLErr("Attrib_Set (ZCompare)");
			}

			if (Changed & CRC_FLAGS_BLEND)
			{
				if (f & CRC_FLAGS_BLEND)
					glnEnable(GL_BLEND);
				else
					glnDisable(GL_BLEND);
				MACRO_GLCOUNTSTATE(m_nStateEnableBlend);
				GLErr("Attrib_Set (BlendEnable)");
			}

			if (Changed & CRC_FLAGS_SMOOTH)
			{
				if (f & CRC_FLAGS_SMOOTH)
					glnShadeModel(GL_SMOOTH);
				else
					glnShadeModel(GL_FLAT);
				MACRO_GLCOUNTSTATE(m_nStateEnableSmooth);
				GLErr("Attrib_Set (Smooth)");
			}

			if (Changed & CRC_FLAGS_CULL)
			{
				if (f & CRC_FLAGS_CULL)
					glnEnable(GL_CULL_FACE);
				else
					glnDisable(GL_CULL_FACE);
				MACRO_GLCOUNTSTATE(m_nStateEnableCull);
				GLErr("Attrib_Set (CullFace)");
			}

			if (Changed & CRC_FLAGS_CULLCW)
			{
				if (f & CRC_FLAGS_CULLCW)
					glnCullFace(GL_BACK);
				else
					glnCullFace(GL_FRONT);
				MACRO_GLCOUNTSTATE(m_nStateEnableCullCCW);
				GLErr("Attrib_Set (CullCCW)");
			}

#ifdef CRCGL_DEPTHFOGENABLE
			// Fog
			if (Changed & CRC_FLAGS_FOG)
			{
				bVPNeedUpdate = true;

				if (f & CRC_FLAGS_FOG)
					glnEnable(GL_FOG);
				else
					glnDisable(GL_FOG);
				MACRO_GLCOUNTSTATE(m_nStateEnableFog);
				GLErr("Attrib_Set (FogEnable)");
			}
#endif

			if (Changed & CRC_FLAGS_SEPARATESTENCIL)
			{
				AttrChg |= CRC_ATTRCHG_STENCIL;
			}
			
			if (Changed & CRC_FLAGS_STENCIL)
			{
				if( f & CRC_FLAGS_STENCIL )
					glnEnable( GL_STENCIL_TEST );
				else
					glnDisable( GL_STENCIL_TEST );
				AttrChg |= CRC_ATTRCHG_STENCIL;
			}

			if (Changed & CRC_FLAGS_POLYGONOFFSET)
			{
				if (f & CRC_FLAGS_POLYGONOFFSET)
				{
					glnEnable(GL_POLYGON_OFFSET_FILL);
				}
				else
				{
					glnDisable(GL_POLYGON_OFFSET_FILL);
				}
			}

			if (Changed & CRC_FLAGS_LIGHTING)
			{
				DebugBreak();
			}

			if (Changed & CRC_FLAGS_SCISSOR)
			{
				if (f & CRC_FLAGS_SCISSOR)
					glnEnable(GL_SCISSOR_TEST);
				else
					glnDisable(GL_SCISSOR_TEST);
			}

			m_CurrentAttrib.m_Flags = _pAttrib->m_Flags;
		}

		GLErr("Attrib_Set (Post flags)");

		// Different source/dest blend?
		if (AttrChg & CRC_ATTRCHG_BLEND)
			if ((m_CurrentAttrib.m_SourceBlend != _pAttrib->m_SourceBlend) ||
				(m_CurrentAttrib.m_DestBlend != _pAttrib->m_DestBlend))
			{
				GLErr("Attrib_Set (Pre blend)");

				m_CurrentAttrib.m_SourceBlend = _pAttrib->m_SourceBlend;
				m_CurrentAttrib.m_DestBlend = _pAttrib->m_DestBlend;

				glnBlendFunc(ConvertToGLBlend(_pAttrib->m_SourceBlend), ConvertToGLBlend(_pAttrib->m_DestBlend));
				GLErr("Attrib_Set (glnBlendFunc)");
				MACRO_GLCOUNTSTATE(m_nStateBlendFunc);
				GLErr("Attrib_Set (BlendFactors)");
			}

		// Different z-compare?
		if (AttrChg & CRC_ATTRCHG_ZCOMPARE)
		{
			if (m_CurrentAttrib.m_ZCompare != _pAttrib->m_ZCompare)
			{
				m_CurrentAttrib.m_ZCompare = _pAttrib->m_ZCompare;
				glnDepthFunc(ConvertToGLCompare(_pAttrib->m_ZCompare));
				GLErr("Attrib_Set (ZFunc)");
				MACRO_GLCOUNTSTATE(m_nStateZFunc);
			}
		}

		// Alpha compare
		if (AttrChg & CRC_ATTRCHG_ALPHACOMPARE)
		{
			if (_pAttrib->m_AlphaCompare != m_CurrentAttrib.m_AlphaCompare ||
				_pAttrib->m_AlphaRef != m_CurrentAttrib.m_AlphaRef)
			{
				GLErr("Attrib_Set (Pre alphacompare)");

				glnAlphaFunc(ConvertToGLCompare(_pAttrib->m_AlphaCompare), fp4(_pAttrib->m_AlphaRef)/255.0f);
				if (_pAttrib->m_AlphaCompare == CRC_COMPARE_ALWAYS)
					glnDisable(GL_ALPHA_TEST);
				else
					glnEnable(GL_ALPHA_TEST);

				m_CurrentAttrib.m_AlphaCompare = _pAttrib->m_AlphaCompare;
				m_CurrentAttrib.m_AlphaRef = _pAttrib->m_AlphaRef;

				GLErr("Attrib_Set (AlphaCompare)");
				MACRO_GLCOUNTSTATE(m_nStateAlphaFunc);
				MACRO_GLCOUNTSTATE(m_nStateEnableAlphaTest);
			}
		}

		// Any fog-settings changed?
		if (AttrChg & CRC_ATTRCHG_FOG)
		{
			if (m_CurrentAttrib.m_FogColor != _pAttrib->m_FogColor)
			{
				m_CurrentAttrib.m_FogColor = _pAttrib->m_FogColor;
				int Colorsi[4];
				Colorsi[0] = (int)(fp4((_pAttrib->m_FogColor >> 16) & 255) / 255.0f * 0x7fffffff);
				Colorsi[1] = (int)(fp4((_pAttrib->m_FogColor >> 8) & 255) / 255.0f * 0x7fffffff);
				Colorsi[2] = (int)(fp4((_pAttrib->m_FogColor >> 0) & 255) / 255.0f * 0x7fffffff);
				Colorsi[3] = 0;
				glnFogiv(GL_FOG_COLOR, Colorsi);
				MACRO_GLCOUNTSTATE(m_nStateFogColor);
				GLErr("Attrib_Set (FogColor)");
			}

			if (m_CurrentAttrib.m_FogStart != _pAttrib->m_FogStart)
			{
				m_CurrentAttrib.m_FogStart = _pAttrib->m_FogStart;
				glnFogf(GL_FOG_START, _pAttrib->m_FogStart);
				MACRO_GLCOUNTSTATE(m_nStateFogStart);
				GLErr("Attrib_Set (FogStart)");
			}

			if (m_CurrentAttrib.m_FogEnd != _pAttrib->m_FogEnd)
			{
				m_CurrentAttrib.m_FogEnd = _pAttrib->m_FogEnd;
				glnFogf(GL_FOG_END, _pAttrib->m_FogEnd);
				MACRO_GLCOUNTSTATE(m_nStateFogEnd);
				GLErr("Attrib_Set (FogEnd)");
			}

			if (m_CurrentAttrib.m_FogDensity != _pAttrib->m_FogDensity)
			{
				m_CurrentAttrib.m_FogDensity = _pAttrib->m_FogDensity;
				glnFogf(GL_FOG_DENSITY, _pAttrib->m_FogDensity);
				MACRO_GLCOUNTSTATE(m_nStateFogDensity);
				GLErr("Attrib_Set (FogDensity)");
			}
		}

		if (AttrChg & CRC_ATTRCHG_POLYGONOFFSET)
		{
			if(m_CurrentAttrib.m_PolygonOffsetScale != _pAttrib->m_PolygonOffsetScale && m_CurrentAttrib.m_PolygonOffsetUnits != _pAttrib->m_PolygonOffsetUnits)
			{
				m_CurrentAttrib.m_PolygonOffsetScale = _pAttrib->m_PolygonOffsetScale;
				m_CurrentAttrib.m_PolygonOffsetUnits = _pAttrib->m_PolygonOffsetUnits;
				glnPolygonOffset(_pAttrib->m_PolygonOffsetScale, _pAttrib->m_PolygonOffsetUnits);
				GLErr("Attrib_Set (PolygonOffset)");
				MACRO_GLCOUNTSTATE(m_nStatePolygonOffset);
			}
		}

		// Stencil changed?
		if (AttrChg & CRC_ATTRCHG_STENCIL)
		{
			GLErr("Attrib_Set (Pre Stencil)");

			Attrib_SetStencil( _pAttrib );
		}

		if (_pAttrib->m_Flags & CRC_FLAGS_SCISSOR)
		{
			m_CurrentAttrib.m_Scissor = _pAttrib->m_Scissor;
			int w = _pAttrib->m_Scissor.m_Max[0] - _pAttrib->m_Scissor.m_Min[0];
			int h = _pAttrib->m_Scissor.m_Max[1] - _pAttrib->m_Scissor.m_Min[1];
			if (w < 0)
				w = 0;
			if (h < 0)
				h = 0;
			glnScissor(_pAttrib->m_Scissor.m_Min[0], CDisplayContextPS3::ms_This.m_CurrentBackbufferContext.m_Setup.m_Height - _pAttrib->m_Scissor.m_Max[1], w, h);
			GLErr("Attrib_Set (Scissor)");
		}
	}

	// Set texture
	if (AttrChg & CRC_ATTRCHG_TEXTUREID)
	{
		GLErr("Attrib_Set (Pre TextureID)");
		for(int i = 0; i < CRC_MAXTEXTURES; i++)
		{
			if (m_CurrentAttrib.m_TextureID[i] != _pAttrib->m_TextureID[i])
			{
				GL_SetTexture(_pAttrib->m_TextureID[i], i);
				m_CurrentAttrib.m_TextureID[i] = _pAttrib->m_TextureID[i];
				GLErr(CStrF("Attrib_Set (TextureID %d)", i));
			}
		}
	}

	{
		for(int i = 0; i < CRC_MAXTEXCOORDS; i++)
		{
			if (m_CurrentAttrib.m_iTexCoordSet[i] != _pAttrib->m_iTexCoordSet[i])
			{
				m_CurrentAttrib.m_iTexCoordSet[i] = _pAttrib->m_iTexCoordSet[i];
				m_VACurrentFmt = -1;
			}
		}
	}

	// -------------------------------------------------------------------
	// TODO: Select vertex program pipeline for obscure texgen modes.

	CRC_MatrixState& MS = Matrix_GetState();

	{
		// -------------------------------------------------------------------
		// Vertex program pipeline setup
		
		// Replaces:
		//	- Texture coordinate generation
		//	- Texture coordinate transform
		//	- Lighting

/*		if ((AttrChg & CRC_ATTRCHG_TEXGEN) ||
			(AttrChg & CRC_ATTRCHG_OTHER))
		{
		}*/

#if	(DEF_CRC_MAXTEXTURES==4)
		int MatrixMask = 
			(1 << CRC_MATRIX_PALETTE) | 
			(1 << CRC_MATRIX_TEXTURE0) | 
			(1 << CRC_MATRIX_TEXTURE1) |
			(1 << CRC_MATRIX_TEXTURE2) |
			(1 << CRC_MATRIX_TEXTURE3);
#else
		int MatrixMask = 
			(1 << CRC_MATRIX_PALETTE) | 
			(1 << CRC_MATRIX_TEXTURE0) | 
			(1 << CRC_MATRIX_TEXTURE1) |
			(1 << CRC_MATRIX_TEXTURE2) |
			(1 << CRC_MATRIX_TEXTURE3) |
			(1 << CRC_MATRIX_TEXTURE4) | 
			(1 << CRC_MATRIX_TEXTURE5) |
			(1 << CRC_MATRIX_TEXTURE6) |
			(1 << CRC_MATRIX_TEXTURE7);
#endif

bVPNeedUpdate |= 1;

		if (bVPNeedUpdate ||
			(m_GeomColor != m_CurrentAttribGeomColor) ||
			!IsTexGenEqual(m_CurrentAttrib, *_pAttrib) ||
			(Changed & CRC_FLAGS_LIGHTING) ||
			(m_CurrentAttrib.m_pLights != _pAttrib->m_pLights) ||
			(m_CurrentAttrib.m_nLights != _pAttrib->m_nLights) ||
			(m_MatrixChanged & MatrixMask))
		{
			Attrib_SetVPPipeline(_pAttrib);
			m_CurrentAttribGeomColor = m_GeomColor;
		}
		GLErr("Attrib_Set");
	}

	Attrib_SetPixelProgram(_pAttrib);

	{
		int nClip = m_lClipStack[m_iClipStack].m_nPlanes;
		int i = 0;
		for(; i < nClip; i++)
			glEnable(GL_CLIP_PLANE0 + i);
		for(; i < 6; i++)
			glDisable(GL_CLIP_PLANE0 + i);
	}

	// -------------------------------------------------------------------
	// NV10 Attributes (REGISTERCOMBINERS)
/*	if (m_Caps_Flags & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV10)
	{
		CRC_ExtAttributes_NV10* pExtAttr = (CRC_ExtAttributes_NV10*) _pAttrib->m_pExtAttrib;
		if (pExtAttr)
		{
			if ((m_Caps_Flags & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20) &&
				(pExtAttr->m_AttribType != CRC_ATTRIBTYPE_NV10) &&
				(pExtAttr->m_AttribType != CRC_ATTRIBTYPE_NV20))
				pExtAttr = NULL;
			else if (!(m_Caps_Flags & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20) &&
				(pExtAttr->m_AttribType != CRC_ATTRIBTYPE_NV10))
				pExtAttr = NULL;
		}

		if (pExtAttr != m_pCurrentExtAttrNV10)
		{
			if (pExtAttr)
			{
				Attrib_SetRegCombiners(pExtAttr);
				m_pCurrentExtAttrNV10 = pExtAttr;
			}
			else
			{
//			ConOut("RegCombiners disabled.");
				glnDisable(GL_REGISTER_COMBINERS_NV);
				m_pCurrentExtAttrNV10 = NULL;
			}
		}
		GLErr("Attrib_Set");
	}

	// -------------------------------------------------------------------
	// NV20 Attributes (TEXTURESHADER, VERTEXPROGRAM)
	if (m_Caps_Flags & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20)
	{
		CRC_ExtAttributes_NV20* pExtAttr = (CRC_ExtAttributes_NV20*) _pAttrib->m_pExtAttrib;
		if (pExtAttr && (pExtAttr->m_AttribType != CRC_ATTRIBTYPE_NV20)) pExtAttr = NULL;

		// -------------------------------------------------------------------
		// TEXTURESHADER & REGISTERCOMBINERS
		if (pExtAttr)
		{
			if (pExtAttr != m_pCurrentExtAttrNV20)
			{
				Attrib_SetTextureShader(pExtAttr);
				GLErr("Attrib_Set");
			}
		}
		else
		{
			if (pExtAttr != m_pCurrentExtAttrNV20)
			{
				glnDisable(GL_TEXTURE_SHADER_NV);
				GLErr("Attrib_Set");
			}
		}


		m_pCurrentExtAttrNV20 = pExtAttr;
		GLErr("Attrib_Set");
	}

	{
		if( _pAttrib->m_pExtAttrib )
		{
			if(m_Caps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM11)
			{
				CRC_ExtAttributes_FragmentProgram11* pExtAttr = (CRC_ExtAttributes_FragmentProgram11*)_pAttrib->m_pExtAttrib;
				if (pExtAttr && (pExtAttr->m_AttribType != CRC_ATTRIBTYPE_PixelShader)) pExtAttr = NULL;

				if (pExtAttr)
				{
					if( pExtAttr != m_pCurrentExtAttrPixelShader )
					{
						if( !m_pCurrentExtAttrPixelShader )
							glnEnable( GL_FRAGMENT_PROGRAM_ARB );
						Attrib_SetPixelShader(pExtAttr);
						GLErr("Attrib_Set");
					}
					m_pCurrentExtAttrPixelShader = pExtAttr;
				}
			}

			if(m_Caps_Flags & CRC_CAPS_FLAGS_FRAGMENTPROGRAM20)
			{
				CRC_ExtAttributes_FragmentProgram20* pExtAttr = (CRC_ExtAttributes_FragmentProgram20*)_pAttrib->m_pExtAttrib;
				if (pExtAttr && (pExtAttr->m_AttribType != CRC_ATTRIBTYPE_FP20)) pExtAttr = NULL;

				if (pExtAttr)
				{
					if( pExtAttr != m_pCurrentExtAttrPixelShader )
					{
						if( !m_pCurrentExtAttrPixelShader )
							glnEnable( GL_FRAGMENT_PROGRAM_ARB );
						Attrib_SetPixelShader20(pExtAttr);
						GLErr("Attrib_Set");
					}
					m_pCurrentExtAttrPixelShader = pExtAttr;
				}
			}
		}
		else
		{
			if( m_pCurrentExtAttrPixelShader )
			{
				glnDisable( GL_FRAGMENT_PROGRAM_ARB );
				m_pCurrentExtAttrPixelShader = NULL;
			}
		}
		GLErr("Attrib_Set");
	}
*/
//	m_CurrentAttrib = *_pAttrib;
	GLErr("Attrib_Set");
}

void CRenderContextPS3::Attrib_SetAbsolute(CRC_Attributes* _pAttrib)
{
	MSCOPE(CRenderContextPS3::Attrib_SetAbsolute, RENDER_GL);

	m_nAttribSet++;

	Attrib_SetRasterMode(_pAttrib, _pAttrib->m_RasterMode);

	// Flags
	int f = _pAttrib->m_Flags;

	// Color-write
	{
		int ColorMask = (f & CRC_FLAGS_COLORWRITE) ? 1 : 0;
		int AlphaMask = (f & CRC_FLAGS_ALPHAWRITE) ? 1 : 0;
		glnColorMask(ColorMask, ColorMask, ColorMask, AlphaMask);
	}

	// Z-Write
	glnDepthMask((f & CRC_FLAGS_ZWRITE) ? 1 : 0);
	//	glnStencilMask((f & CRC_FLAGS_ZWRITE) ? 0xffffffff : 0);	// Fillrate test, see Attrib_Set()

	// Z-compare
	if (f & CRC_FLAGS_ZCOMPARE)
		glnEnable(GL_DEPTH_TEST);
	else
		glnDisable(GL_DEPTH_TEST);
	MACRO_GLCOUNTSTATE(m_nStateEnableZWrite);

	// Alpha-compare
	glnAlphaFunc(ConvertToGLCompare(_pAttrib->m_AlphaCompare), fp4(_pAttrib->m_AlphaRef)/255.0f);
	if (_pAttrib->m_AlphaCompare == CRC_COMPARE_ALWAYS)
		glnDisable(GL_ALPHA_TEST);
	else
		glnEnable(GL_ALPHA_TEST);
	MACRO_GLCOUNTSTATE(m_nStateAlphaFunc);
	MACRO_GLCOUNTSTATE(m_nStateEnableAlphaTest);

	// Blending
	if (f & CRC_FLAGS_BLEND)
		glnEnable(GL_BLEND);
	else
		glnDisable(GL_BLEND);
	MACRO_GLCOUNTSTATE(m_nStateEnableBlend);

	// Smoothing
	if (f & CRC_FLAGS_SMOOTH)
		glnShadeModel(GL_SMOOTH);
	else
		glnShadeModel(GL_FLAT);
	MACRO_GLCOUNTSTATE(m_nStateEnableSmooth);

	// Culling
	if (f & CRC_FLAGS_CULL)
		glnEnable(GL_CULL_FACE);
	else
		glnDisable(GL_CULL_FACE);
	MACRO_GLCOUNTSTATE(m_nStateEnableCull);

	// Frontface
	if (f & CRC_FLAGS_CULLCW)
		glnCullFace(GL_BACK);
	else
		glnCullFace(GL_FRONT);
	MACRO_GLCOUNTSTATE(m_nStateEnableCullCCW);

	// Fog enable
#ifdef CRCGL_DEPTHFOGENABLE
	if (f & CRC_FLAGS_FOG)
		glnEnable(GL_FOG);
	else
		glnDisable(GL_FOG);
#endif
	GLErr("Attrib_SetAbsolute (1)");

	// Stencil enable
	if (f & CRC_FLAGS_STENCIL)
		glnEnable(GL_STENCIL_TEST);
	else
		glnDisable(GL_STENCIL_TEST);
	GLErr("Attrib_SetAbsolute (2)");


	// PolygonOffset enable
	if (f & CRC_FLAGS_POLYGONOFFSET)
	{
		glnEnable(GL_POLYGON_OFFSET_FILL);
	}
	else
	{
		glnDisable(GL_POLYGON_OFFSET_FILL);
	}

	// Scissor
	if (f & CRC_FLAGS_SCISSOR)
	{
		int w = _pAttrib->m_Scissor.m_Max[0] - _pAttrib->m_Scissor.m_Min[0];
		int h = _pAttrib->m_Scissor.m_Max[1] - _pAttrib->m_Scissor.m_Min[1];
		if (w < 0)
			w = 0;
		if (h < 0)
			h = 0;

		glnEnable(GL_SCISSOR_TEST);
		glnScissor(_pAttrib->m_Scissor.m_Min[0], CDisplayContextPS3::ms_This.m_CurrentBackbufferContext.m_Setup.m_Height - _pAttrib->m_Scissor.m_Max[1], w, h);
	}
	else
		glnDisable(GL_SCISSOR_TEST);

	// Stencil
	if (!(m_Mode.m_Flags & CRC_GLOBALFLAGS_WIRE))
	{
		Attrib_SetStencil( _pAttrib );
	}

	// Fog
	{
		int Colors[4];
		Colors[1] = ((_pAttrib->m_FogColor >> 16) & 255) << 23;
		Colors[2] = ((_pAttrib->m_FogColor >> 8) & 255) << 23;
		Colors[3] = ((_pAttrib->m_FogColor >> 0) & 255) << 23;
		Colors[0] = ((_pAttrib->m_FogColor >> 24) & 255) << 23;
		glnFogiv(GL_FOG_COLOR, Colors);

		glnFogf(GL_FOG_START, _pAttrib->m_FogStart);
		glnFogf(GL_FOG_END, _pAttrib->m_FogEnd);
		glnFogf(GL_FOG_DENSITY, _pAttrib->m_FogDensity);

		MACRO_GLCOUNTSTATE(m_nStateFogColor);
		MACRO_GLCOUNTSTATE(m_nStateFogStart);
		MACRO_GLCOUNTSTATE(m_nStateFogEnd);
		MACRO_GLCOUNTSTATE(m_nStateFogDensity);
	}

	// PolygonOffset
#ifndef CRCGL_POLYGONOFFSETPROJECTIONMATRIX
	glnPolygonOffset(_pAttrib->m_PolygonOffsetScale, _pAttrib->m_PolygonOffsetUnits);
	GLErr("Attrib_Set (PolygonOffset)");
	MACRO_GLCOUNTSTATE(m_nStatePolygonOffset);
#endif

	// Different source/dest blend?
	glnBlendFunc(ConvertToGLBlend(_pAttrib->m_SourceBlend), ConvertToGLBlend(_pAttrib->m_DestBlend));
	MACRO_GLCOUNTSTATE(m_nStateBlendFunc);

	// Different z-compare?
	glnDepthFunc(ConvertToGLCompare(_pAttrib->m_ZCompare));
	MACRO_GLCOUNTSTATE(m_nStateZFunc);

	// Set Texture
	for(int i = 0; i < CRC_MAXTEXTURES; i++)
	{
		GL_SetTexture(_pAttrib->m_TextureID[i], i);
		GLErr(CStrF("Attrib_Set (TextureID %d)", i));

		m_CurrentAttrib.m_TextureID[i] = _pAttrib->m_TextureID[i];
		m_CurrentAttrib.m_iTexCoordSet[i] = _pAttrib->m_iTexCoordSet[i];
	}

	{
		// -------------------------------------------------------------------
		// Vertex program pipeline setup
		Attrib_SetVPPipeline(_pAttrib);
	}

	Attrib_SetPixelProgram(_pAttrib);

	{
		int nClip = m_lClipStack[m_iClipStack].m_nPlanes;
		int i = 0;
		for(; i < nClip; i++)
			glEnable(GL_CLIP_PLANE0 + i);
		for(; i < 6; i++)
			glDisable(GL_CLIP_PLANE0 + i);
	}

	m_CurrentAttrib = *_pAttrib;
	GLErr("Attrib_SetAbsolute");
}


void CRenderContextPS3::Attrib_GlobalUpdate()
{
	MSCOPE(CRenderContextPS3::Attrib_GlobalUpdate, RENDER_GL);
	if(MRTC_SystemInfo::OS_GetThreadID() == GetDC()->GetOwner())
		Attrib_DeferredGlobalUpdate();
	else
		m_bAttribGlobalUpdate = true;
}

void CRenderContextPS3::Attrib_DeferredGlobalUpdate()
{
	MSCOPE(CRenderContextPS3::Attrib_DeferredGlobalUpdate, RENDER_GL);
	m_bAttribGlobalUpdate = false;

	GLErr("Attrib_GlobalUpdate");
	if (m_Mode.m_Flags & CRC_GLOBALFLAGS_WIRE)
	{
		glnPolygonMode(GL_FRONT, GL_LINE);
		glnPolygonMode(GL_BACK, GL_LINE);
	}
	else
	{
		glnPolygonMode(GL_FRONT, GL_FILL);
		glnPolygonMode(GL_BACK, GL_FILL);
	}
	GLErr("Attrib_GlobalUpdate");
}

int CRenderContextPS3::Attrib_GlobalGetVar(int _Var)
{
	switch(_Var)
	{
	case /*CRC_GLOBALVAR_TOTALTRIANGLES*/ 20 : return m_nTriTotal;
	default : return CRC_Core::Attrib_GlobalGetVar(_Var);
	}
}

#ifdef CRCGL_POLYGONOFFSETPROJECTIONMATRIX
void CRenderContextPS3::Attrib_SetPolygonOffset(fp4 _Offset)
{
	if (m_CurrentProjectionPolygonOffset == _Offset)
		return;

	m_CurrentProjectionPolygonOffset = _Offset;
	Viewport_Update();
}
#endif

//----------------------------------------------------------------
// Transform overrides
//----------------------------------------------------------------
void CRenderContextPS3::Matrix_SetRender(int _iMode, const CMat4Dfp4* _pMatrix)
{
	MSCOPE(CRenderContextPS3::Matrix_SetRender, RENDER_GL);

	//TODO: Implement CRenderContextPS3::Matrix_SetRender properly

	if (_iMode == CRC_MATRIX_MODEL)
	{
		{
			if (_pMatrix)
			{
				CMat4Dfp4 Mat, MatProj;
				Mat = *_pMatrix;
				Mat.Transpose();
				m_Matrix_ProjectionTransposeGL.Multiply(Mat, MatProj);
				memcpy(m_VPConstRegisters[0].k, MatProj.k[0], sizeof(CMat4Dfp4));
				memcpy(m_VPConstRegisters[4].k, Mat.k[0], sizeof(CMat4Dfp4));
			}
			else
			{
				memcpy(m_VPConstRegisters[0].k, m_Matrix_ProjectionTransposeGL.k[0], sizeof(CMat4Dfp4));
				memcpy(m_VPConstRegisters[4].k, m_Matrix_Unit.k[0], sizeof(CMat4Dfp4));
			}
			m_nUpdateVPConst = Max(m_nUpdateVPConst, (uint16)8);
			m_bUpdateVPConst = true;
		}
	}
	else if (_iMode == CRC_MATRIX_PROJECTION)
	{
		ConOut("(CRenderContextPS3::Matrix_SetRender) Projection matrix stack is not supported.");
	}
	else if ((_iMode >= CRC_MATRIX_TEXTURE) && (_iMode < CRC_MATRIX_TEXTURE + CRC_MAXTEXCOORDS))
	{
		// Handled with Attrib_SetVPPipeline
	}
		
	GLErr("Matrix_SetRender");
}

void CRenderContextPS3::Attrib_SetStencil( CRC_Attributes* _pAttrib )
{
	if( !( _pAttrib->m_Flags & (CRC_FLAGS_STENCIL | CRC_FLAGS_SEPARATESTENCIL) ) )
	{
		return;
	}

	bool bState = false;

	{
		uint32 StencilWriteMask = _pAttrib->m_StencilWriteMask;
		uint32 StencilFuncRef = _pAttrib->m_StencilRef;
		uint32 StencilFuncAnd = _pAttrib->m_StencilFuncAnd;

		if(_pAttrib->m_Flags & CRC_FLAGS_SEPARATESTENCIL)
		{
			glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
			GLErr("Attrib_Set (Stencil, 1)");

			uint32 StencilBackFunc = _pAttrib->m_StencilBackFunc;
			uint32 StencilBackOpFail = _pAttrib->m_StencilBackOpFail;
			uint32 StencilBackOpZFail = _pAttrib->m_StencilBackOpZFail;
			uint32 StencilBackOpZPass = _pAttrib->m_StencilBackOpZPass;

			glActiveStencilFaceEXT(GL_BACK);
			glStencilMask(StencilWriteMask);
			GLErr("Attrib_Set (Stencil, 2)");
			glStencilFunc(ConvertToGLCompare(StencilBackFunc), StencilFuncRef, StencilFuncAnd);
			GLErr("Attrib_Set (Stencil, 3)");
			glStencilOp(ConvertToGLStencilOp(StencilBackOpFail), ConvertToGLStencilOp(StencilBackOpZFail), ConvertToGLStencilOp(StencilBackOpZPass));
			GLErr("Attrib_Set (Stencil, 4)");

		}
		else
			glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);


		{
			uint32 StencilFrontFunc = _pAttrib->m_StencilFrontFunc;
			uint32 StencilFrontOpFail = _pAttrib->m_StencilFrontOpFail;
			uint32 StencilFrontOpZFail = _pAttrib->m_StencilFrontOpZFail;
			uint32 StencilFrontOpZPass = _pAttrib->m_StencilFrontOpZPass;

			glActiveStencilFaceEXT(GL_FRONT);
			glStencilMask(StencilWriteMask);
			GLErr("Attrib_Set (Stencil, 5)");
			glStencilFunc(ConvertToGLCompare(StencilFrontFunc), StencilFuncRef, StencilFuncAnd);
			GLErr("Attrib_Set (Stencil, 6)");
			glStencilOp(ConvertToGLStencilOp(StencilFrontOpFail), ConvertToGLStencilOp(StencilFrontOpZFail), ConvertToGLStencilOp(StencilFrontOpZPass));
			GLErr("Attrib_Set (Stencil, 7)");
		}

		bState = true;

		m_CurrentAttrib.m_StencilWriteMask = StencilWriteMask;
		m_CurrentAttrib.m_StencilRef = StencilFuncRef;
		m_CurrentAttrib.m_StencilFuncAnd = StencilFuncAnd;

		m_CurrentAttrib.m_StencilDWord1	= _pAttrib->m_StencilDWord1;
	}

	if( bState ) MACRO_GLCOUNTSTATE(m_nStateStencil);
	GLErr("Attrib_Set (Stencil)");
}
