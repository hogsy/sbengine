
#include "MRender.h"
#ifndef _INC_MRndrGL_RegCombiners
#define _INC_MRndrGL_RegCombiners


#ifdef SUPPORT_REGISTERCOMBINERS

#ifndef PLATFORM_CONSOLE
// -------------------------------------------------------------------
//  NV_REGISTER_COMBINERS
// -------------------------------------------------------------------
enum
{
//	CRC_ATTRIBTYPE_NV10			= 1,
//	CRC_ATTRIBTYPE_NV20			= 2,

	NV_COMP_RGB					= 0x00000000,
	NV_COMP_ALPHA				= 0x80000000,
	NV_COMP_BLUE				= 0x00000000,

	NVGL_COMP_RGB				= 0x1907,
	NVGL_COMP_ALPHA				= 0x1906,
	NVGL_COMP_BLUE				= 0x1905,

	NV_COMBINER0				= 0x8550,
	NV_COMBINER1,
	NV_COMBINER2,
	NV_COMBINER3,
	NV_COMBINER4,
	NV_COMBINER5,
	NV_COMBINER6,
	NV_COMBINER7,

	NV_VARIABLE_A				= 0x8523,
	NV_VARIABLE_B,
	NV_VARIABLE_C,
	NV_VARIABLE_D,
	NV_VARIABLE_E,
	NV_VARIABLE_F,
	NV_VARIABLE_G,

	NV_INPUT_ZERO				= 0,
	NV_INPUT_COLOR0				= 0x852a,
	NV_INPUT_COLOR1				= 0x852b,
	NV_INPUT_FOG				= 0x0B60,
	NV_INPUT_PRIMARY			= 0x852c,
	NV_INPUT_SECONDARY			= 0x852d,
	NV_INPUT_SPARE0				= 0x852e,
	NV_INPUT_SPARE1				= 0x852f,
	NV_INPUT_TEXTURE0			= 0x84c0,
	NV_INPUT_TEXTURE1			= 0x84c1,
	NV_INPUT_TEXTURE2			= 0x84c2,
	NV_INPUT_TEXTURE3			= 0x84c3,
	NV_INPUT_EF					= 0x8531,
	NV_INPUT_COLORSUM			= 0x8532,

	NV_MAPPING_IDENTITY			= 0,	// Max(0, e)			// Nothing
	NVGL_MAPPING_IDENTITY		= 0x536,	// Max(0, e)			// Nothing
	NV_MAPPING_INVERT			= 0x5370000,	// 1 - Clamp01(e)		// 1 -
	NV_MAPPING_EXPNORMAL		= 0x5380000,	// 2 * Max(0, e) - 1	// _bx2
	NV_MAPPING_EXPNEGATE		= 0x5390000,	// -2 * Max(0, e) + 1	// - _bx2
	NV_MAPPING_HALFBIASNORMAL	= 0x53a0000,	// Max(0, e) - 0.5		// _bias
	NV_MAPPING_HALFBIASNEGATE	= 0x53b0000,	// -Max(0, e) + 0.5		// - _bias
	NV_MAPPING_SIGNEDIDENTITY	= 0x53c0000,	// e					// Nothing
	NV_MAPPING_SIGNEDNEGATE		= 0x53d0000,	// -e					// -

	NV_SCALE_ONE				= 0x00,
	NV_SCALE_ONE_BIAS			= 0x80,
	NV_SCALE_TWO				= 0x01,
	NV_SCALE_TWO_BIAS			= 0x81,	
	NV_SCALE_FOUR				= 0x02,
	NV_SCALE_HALF				= 0x03,

	NVGL_SCALE_ONE				= 0,
	NVGL_SCALE_TWO				= 0x853e,
	NVGL_SCALE_FOUR				= 0x853f,
	NVGL_SCALE_HALF				= 0x8540,

	NV_OUTPUT_DISCARD			= 0x8530,
	NV_OUTPUT_PRIMARY			= NV_INPUT_PRIMARY,
	NV_OUTPUT_SECONDARY			= NV_INPUT_SECONDARY,
	NV_OUTPUT_SPARE0			= NV_INPUT_SPARE0,
	NV_OUTPUT_SPARE1			= NV_INPUT_SPARE1,
	NV_OUTPUT_TEXTURE0			= NV_INPUT_TEXTURE0,
	NV_OUTPUT_TEXTURE1			= NV_INPUT_TEXTURE1,

///	NV_AB_DOT					= 0x8545,
//	NV_CD_DOT,
//	NV_MUX_SUM,
//	NV_SCALE,
//	NV_BIAS,
//	NV_AB_OUTPUT,
//	NV_CD_OUTPUT,
//	NV_SUM_OUTPUT,
};

#define DNVInuputMappingToGL(_Input) (((_Input >> 16) & 0xfff))
M_INLINE static int NVInuputMappingToGL(int _Input)
{
	int Mapping = DNVInuputMappingToGL(_Input);
	if (!Mapping)
		Mapping = NVGL_MAPPING_IDENTITY;
	return Mapping | 0x8000;
}


#define DNVInuputRegToGL(_Input) (_Input & 0xffff)
M_INLINE static int NVInputCompToGL(int _Input)
{
	if (_Input & NV_COMP_ALPHA)
		return NVGL_COMP_ALPHA;
	else
		return NVGL_COMP_RGB;
}

M_INLINE static int NVInputCompToGLAlpha(int _Input)
{
	if (_Input & NV_COMP_ALPHA)
		return NVGL_COMP_ALPHA;
	else
		return NVGL_COMP_BLUE;
}

M_INLINE static int NVScaleGL(int _Input)
{
	switch (_Input & 0xf)
	{
	case 0:
		return NVGL_SCALE_ONE;
	case 1:
		return NVGL_SCALE_TWO;
	case 2:
		return NVGL_SCALE_FOUR;
	case 3:
		return NVGL_SCALE_HALF;
	}
	return 0;
}

M_INLINE static int NVBiasGL(int _Input)
{
	if (_Input & 0x80)
		return 1;
	else
		return 0;
}

// -------------------------------------------------------------------
class CNV_RegCombiners_CombinerInput
{
public:
	uint16 m_Register;
	uint16 m_Mapping;
	uint16 m_ComponentUsage;			// For RGB-input: RGB/ALPHA, For alpha-input: ALPHA/BLUE

	void Clear()
	{
#define NVDefaultInput (NV_MAPPING_IDENTITY | NV_INPUT_ZERO | NV_COMP_RGB)
		memset(this, 0, sizeof(*this));
		m_Register = DNVInuputRegToGL(NVDefaultInput);
		m_Mapping = NVInuputMappingToGL(NVDefaultInput);
		m_ComponentUsage = NVInputCompToGL(NVDefaultInput);
	}

	void ClearAlpha()
	{
#define NVDefaultInputAlpha (NV_MAPPING_IDENTITY | NV_INPUT_ZERO | NV_COMP_ALPHA)
		memset(this, 0, sizeof(*this));
		m_Register = DNVInuputRegToGL(NVDefaultInputAlpha);
		m_Mapping = NVInuputMappingToGL(NVDefaultInputAlpha);
		m_ComponentUsage = NVInputCompToGLAlpha(NVDefaultInputAlpha);
	}

	void Set(int _Register, int _Mapping, int _Comp)
	{
		m_Register = _Register;
		m_Mapping = _Mapping;
		m_ComponentUsage = _Comp;
	}
};

// -------------------------------------------------------------------
class CNV_RegCombiners_CombinerOutput
{
public:
	uint16 m_DestAB;
	uint16 m_DestCD;
	uint16 m_DestSum;
	uint16 m_Scale;
	uint16 m_Bias;
	union
	{
		struct
		{
			uint16 m_bABDot : 1;
			uint16 m_bCDDot : 1;
			uint16 m_bMuxSum : 1;
		} m_Bits;
		uint16 m_Fniss;
	};

	void Clear()
	{
		memset(this, 0, sizeof(*this));
		m_DestAB = NV_OUTPUT_DISCARD;
		m_DestCD = NV_OUTPUT_DISCARD;
		m_DestSum = NV_OUTPUT_DISCARD;
		m_Scale = NV_SCALE_ONE;
		m_Bias = 0;
		m_Fniss = 0;
	}

	void Set(int _DestAB, int _DestCD, int _DestSum, int _Scale, int _Bias, int _bABDot, int _bCDDot, int _bMuxSum)
	{
		m_DestAB = _DestAB;
		m_DestCD = _DestCD;
		m_DestSum = _DestSum;
		m_Scale = _Scale;
		m_Bias = _Bias;
		m_Fniss = 0;
		m_Bits.m_bABDot = (int)_bABDot!=0;
		m_Bits.m_bCDDot = (int)_bCDDot!=0;
		m_Bits.m_bMuxSum = (int)_bMuxSum!=0;
	}
};

// -------------------------------------------------------------------
class CNV_RegCombiners_Combiner
{
public:
	CNV_RegCombiners_CombinerInput m_InputRGB[4];
	CNV_RegCombiners_CombinerInput m_InputAlpha[4];
	CNV_RegCombiners_CombinerOutput m_OutputRGB;
	CNV_RegCombiners_CombinerOutput m_OutputAlpha;

	void Clear()
	{
		for(int i = 0; i < 4; i++)
		{
			m_InputRGB[i].Clear();
			m_InputAlpha[i].ClearAlpha();
		}

		m_OutputRGB.Clear();
		m_OutputAlpha.Clear();
	}
};

// -------------------------------------------------------------------
class CNV_RegCombiners_FinalCombiner
{
public:
	CNV_RegCombiners_CombinerInput m_InputRGB[6];
	CNV_RegCombiners_CombinerInput m_InputAlpha;

	void Clear()
	{
		for(int i = 0; i < 6; i++)
			m_InputRGB[i].Clear();
		m_InputAlpha.ClearAlpha();
	}

	void SetFastDualTexture()
	{
		// No general combiners are used.

		// Color = Texture0 * Texture1 * Color + Secondary
		// Alpha = Texture0.Alpha

		m_InputRGB[0].m_Register = NV_INPUT_PRIMARY;
		m_InputRGB[3].m_Register = NV_INPUT_SECONDARY;
		m_InputRGB[1].m_Register = NV_INPUT_EF;
		m_InputRGB[4].m_Register = NV_INPUT_TEXTURE0;	// E
		m_InputRGB[5].m_Register = NV_INPUT_TEXTURE1;	// F

		m_InputAlpha.m_Register = NV_INPUT_TEXTURE0;
	}
};

// -------------------------------------------------------------------
//  CRC_ExtAttributes_NV10
// -------------------------------------------------------------------
class CRC_ExtAttributes_NV10 : public CRC_ExtAttributes
{
	friend class CRenderContextGL;
protected:
	// Register combiners
	CNV_RegCombiners_Combiner m_Combiners[8];
	CNV_RegCombiners_FinalCombiner m_FinalCombiner;
	CPixel32 m_lColor0[8];
	CPixel32 m_lColor1[8];
	union
	{
		struct
		{
			int m_NumCombiners : 8;
			int m_bColorSumClamp : 1;
			int m_bRegCombinersEnable : 1;
			int m_bPerCombinerColors : 1;			// RegisterCombiners2 (NV20+) extension. Each combiners have it's own unique constant colors.
		};
		int m_WholeBitField;
	};

public:
	void Clear()
	{
		m_AttribType = CRC_ATTRIBTYPE_NV10;

		m_lColor0[0] = 0xffffffff;
		m_lColor1[0] = 0xffffffff;
		m_WholeBitField = 0;
		m_NumCombiners = 0;
		m_bColorSumClamp = false;
		m_bRegCombinersEnable = true;
		m_bPerCombinerColors = false;

		m_FinalCombiner.Clear();
	}

	M_INLINE void SetNumCombiners(int _nComb, bool _bPerCombinerColors = false)
	{
		m_bPerCombinerColors = _bPerCombinerColors;
		m_NumCombiners = _nComb;
	}

	M_INLINE void SetConst0(int _iConst, uint32 _Value)
	{
		m_lColor0[_iConst] = _Value;
	}

	M_INLINE void SetConst1(int _iConst, uint32 _Value)
	{
		m_lColor1[_iConst] = _Value;
	}

	M_INLINE void SetConst0(uint32 _Value)
	{
		m_lColor0[0] = _Value;
	}

	M_INLINE void SetConst1(uint32 _Value)
	{
		m_lColor1[0] = _Value;
	}

	M_INLINE void SetNoColorUsage()
	{
	}

	M_INLINE void SetInputRGB(int _Combiner, uint32 _Reg1, uint32 _Reg2 = NVDefaultInput, uint32 _Reg3 = NVDefaultInput, uint32 _Reg4 = NVDefaultInput)
	{
		m_Combiners[_Combiner].m_InputRGB[0].Set(DNVInuputRegToGL(_Reg1), NVInuputMappingToGL(_Reg1), NVInputCompToGL(_Reg1));
		m_Combiners[_Combiner].m_InputRGB[1].Set(DNVInuputRegToGL(_Reg2), NVInuputMappingToGL(_Reg2), NVInputCompToGL(_Reg2));
		m_Combiners[_Combiner].m_InputRGB[2].Set(DNVInuputRegToGL(_Reg3), NVInuputMappingToGL(_Reg3), NVInputCompToGL(_Reg3));
		m_Combiners[_Combiner].m_InputRGB[3].Set(DNVInuputRegToGL(_Reg4), NVInuputMappingToGL(_Reg4), NVInputCompToGL(_Reg4));
	}

	M_INLINE void SetInputAlpha(int _Combiner, uint32 _Reg1, uint32 _Reg2 = NVDefaultInputAlpha, uint32 _Reg3 = NVDefaultInputAlpha, uint32 _Reg4 = NVDefaultInputAlpha)
	{
		m_Combiners[_Combiner].m_InputAlpha[0].Set(DNVInuputRegToGL(_Reg1), NVInuputMappingToGL(_Reg1), NVInputCompToGLAlpha(_Reg1));
		m_Combiners[_Combiner].m_InputAlpha[1].Set(DNVInuputRegToGL(_Reg2), NVInuputMappingToGL(_Reg2), NVInputCompToGLAlpha(_Reg2));
		m_Combiners[_Combiner].m_InputAlpha[2].Set(DNVInuputRegToGL(_Reg3), NVInuputMappingToGL(_Reg3), NVInputCompToGLAlpha(_Reg3));
		m_Combiners[_Combiner].m_InputAlpha[3].Set(DNVInuputRegToGL(_Reg4), NVInuputMappingToGL(_Reg4), NVInputCompToGLAlpha(_Reg4));
	}

	M_INLINE void SetOutputRGB(int _Combiner, int _DestAB, int _DestCD, int _DestSum, int _Scale, int _bABDot, int _bCDDot, int _bMuxSum)
	{
		m_Combiners[_Combiner].m_OutputRGB.Set(_DestAB, _DestCD, _DestSum, NVScaleGL(_Scale), NVBiasGL(_Scale), _bABDot, _bCDDot, _bMuxSum);
	}

	M_INLINE void SetOutputAlpha(int _Combiner, int _DestAB, int _DestCD, int _DestSum, int _Scale, int _bABDot, int _bCDDot, int _bMuxSum)
	{
		m_Combiners[_Combiner].m_OutputAlpha.Set(_DestAB, _DestCD, _DestSum, NVScaleGL(_Scale), NVBiasGL(_Scale), _bABDot, _bCDDot, _bMuxSum);
	}

	M_INLINE void SetFinal(uint32 _Reg0, uint32 _Reg1, uint32 _Reg2, uint32 _Reg3, uint32 _Reg4, uint32 _Reg5, uint32 _RegA, int _bColorSumClamp = false)
	{
		m_FinalCombiner.m_InputRGB[0].Set(DNVInuputRegToGL(_Reg0), NVInuputMappingToGL(_Reg0), NVInputCompToGL(_Reg0));
		m_FinalCombiner.m_InputRGB[1].Set(DNVInuputRegToGL(_Reg1), NVInuputMappingToGL(_Reg1), NVInputCompToGL(_Reg1));
		m_FinalCombiner.m_InputRGB[2].Set(DNVInuputRegToGL(_Reg2), NVInuputMappingToGL(_Reg2), NVInputCompToGL(_Reg2));
		m_FinalCombiner.m_InputRGB[3].Set(DNVInuputRegToGL(_Reg3), NVInuputMappingToGL(_Reg3), NVInputCompToGL(_Reg3));
		m_FinalCombiner.m_InputRGB[4].Set(DNVInuputRegToGL(_Reg4), NVInuputMappingToGL(_Reg4), NVInputCompToGL(_Reg4));
		m_FinalCombiner.m_InputRGB[5].Set(DNVInuputRegToGL(_Reg5), NVInuputMappingToGL(_Reg5), NVInputCompToGL(_Reg5));
		m_FinalCombiner.m_InputAlpha.Set(DNVInuputRegToGL(_RegA), NVInuputMappingToGL(_RegA), NVInputCompToGLAlpha(_RegA));

		m_bColorSumClamp = _bColorSumClamp;
	}

	M_INLINE void Clear(int _Combiner)
	{
		m_Combiners[_Combiner].Clear();
	}

	M_INLINE void ClearFinal()
	{
		m_FinalCombiner.Clear();
	}

	M_INLINE void Fixup()
	{
	}
};


// -------------------------------------------------------------------
//  NV_TEXTURE_SHADER
// -------------------------------------------------------------------

enum
{
	NV_TS_TEXTURE0 = 0x84c0,
	NV_TS_TEXTURE1,
	NV_TS_TEXTURE2,
	NV_TS_TEXTURE3,
	NV_TS_TEXTURE4,
	NV_TS_TEXTURE5,
	NV_TS_TEXTURE6,
	NV_TS_TEXTURE7,

	NV_TS_TEXTURE_1D							= 0x0DE0, // tex
	NV_TS_TEXTURE_2D							= 0x0DE1, // tex
	NV_TS_CONST_EYE_NV							= 0x86E5, // tex
	NV_TS_PASS_THROUGH_NV						= 0x86E6, // texcoord
	NV_TS_CULL_FRAGMENT_NV						= 0x86E7, // texkill 
	NV_TS_OFFSET_TEXTURE_2D_NV					= 0x86E8, // texbem
	NV_TS_DEPENDENT_AR_TEXTURE_2D_NV			= 0x86E9, // texreg2ar 
	NV_TS_DEPENDENT_GB_TEXTURE_2D_NV			= 0x86EA, // texreg2gb 
	NV_TS_DOT_PRODUCT_NV						= 0x86EC, // texm3x2pad, texm3x3pad
	NV_TS_DOT_PRODUCT_DEPTH_REPLACE_NV			= 0x86ED, // texm3x2depth
	NV_TS_DOT_PRODUCT_TEXTURE_2D_NV				= 0x86EE, // texm3x2tex
	NV_TS_DOT_PRODUCT_TEXTURE_3D_NV				= 0x86EF, // texm3x3tex
	NV_TS_DOT_PRODUCT_TEXTURE_CUBE_MAP_NV		= 0x86F0, // texm3x3vspec 
	NV_TS_DOT_PRODUCT_DIFFUSE_CUBE_MAP_NV		= 0x86F1, // not supported
	NV_TS_DOT_PRODUCT_REFLECT_CUBE_MAP_NV		= 0x86F2, // texm3x3vspec
	NV_TS_DOT_PRODUCT_CONST_EYE_REFLECT_CUBE_MAP_NV = 0x86F3, // texm3x3spec //

};


class CNV_TextureShader_Stage
{
public:
	uint16 m_Operation;
	uint16 m_Previous;
	fp4* m_pTextureMatrix;

	void Set(int _Op, int _Prev, fp4* _pTextureMatrix = NULL)
	{
		m_Operation = _Op;
		m_Previous = _Prev;
		m_pTextureMatrix = _pTextureMatrix;
	}

	CNV_TextureShader_Stage()
	{
		m_Operation = 0;
		m_Previous = NV_TS_TEXTURE0;
		m_pTextureMatrix = NULL;
	}
};

// -------------------------------------------------------------------
//  CRC_ExtAttributes_NV20
// -------------------------------------------------------------------
class CRC_ExtAttributes_NV20 : public CRC_ExtAttributes_NV10
{
	friend class CRenderContextGL;
protected:
	// NV Texture shader
	CNV_TextureShader_Stage m_TexShaderStages[4];
	bool m_bTexShaderEnable;

	char* m_pProgramName;			// Vertex program name, such as "NV20_GenEnv" (no path or extension)
	CVec4Dfp4* m_pProgramParams;	// Pointer to parameter array
	int m_iProgramParams;			// Start program parameter index to copy m_pProgramParams to.
	int m_nProgramParams;			// Number of parameters in m_pProgramParams

public:
	void Clear()
	{
		CRC_ExtAttributes_NV10::Clear();

		m_AttribType = CRC_ATTRIBTYPE_NV20;
		m_bTexShaderEnable = false;

		m_pProgramName = NULL;
		m_pProgramParams = NULL;
		m_iProgramParams = 0;
		m_nProgramParams = 0;
	}

	CRC_ExtAttributes_NV20()
	{
		Clear();
	}

	M_INLINE void Clear(int _Combiner)
	{	
		CRC_ExtAttributes_NV10::Clear(_Combiner);
	}

	M_INLINE void SetTexureOps(int _Op0 = 0, int _Op1 = 0, int _Op2 = 0, int _Op3 = 0, int _Prev1 = NV_TS_TEXTURE0, int _Prev2 = NV_TS_TEXTURE0, int _Prev3 = NV_TS_TEXTURE0)
	{
		m_TexShaderStages[0].Set(_Op0, NV_TS_TEXTURE0);
		m_TexShaderStages[1].Set(_Op1, _Prev1);
		m_TexShaderStages[2].Set(_Op2, _Prev2);
		m_TexShaderStages[3].Set(_Op3, _Prev3);
		m_bTexShaderEnable = true;
	}

	M_INLINE void SetTextureShaderMatrix(int _iStage, fp4* _pMat)
	{
		m_TexShaderStages[_iStage].m_pTextureMatrix = _pMat;
	}
};


#else

namespace NXboxTypes
{
	enum
	{
		PS_INPUTMAPPING_UNSIGNED_IDENTITY= 0x00L, // max(0,x)         OK for final combiner
		PS_INPUTMAPPING_UNSIGNED_INVERT=   0x20L, // 1 - max(0,x)     OK for final combiner
		PS_INPUTMAPPING_EXPAND_NORMAL=     0x40L, // 2*max(0,x) - 1   invalid for final combiner
		PS_INPUTMAPPING_EXPAND_NEGATE=     0x60L, // 1 - 2*max(0,x)   invalid for final combiner
		PS_INPUTMAPPING_HALFBIAS_NORMAL=   0x80L, // max(0,x) - 1/2   invalid for final combiner
		PS_INPUTMAPPING_HALFBIAS_NEGATE=   0xa0L, // 1/2 - max(0,x)   invalid for final combiner
		PS_INPUTMAPPING_SIGNED_IDENTITY=   0xc0L, // x                invalid for final combiner
		PS_INPUTMAPPING_SIGNED_NEGATE=     0xe0L, // -x               invalid for final combiner
	};

	enum
	{
		PS_REGISTER_ZERO=              0x00L, // r
		PS_REGISTER_DISCARD=           0x00L, // w
		PS_REGISTER_C0=                0x01L, // r
		PS_REGISTER_C1=                0x02L, // r
		PS_REGISTER_FOG=               0x03L, // r
		PS_REGISTER_V0=                0x04L, // r/w
		PS_REGISTER_V1=                0x05L, // r/w
		PS_REGISTER_T0=                0x08L, // r/w
		PS_REGISTER_T1=                0x09L, // r/w
		PS_REGISTER_T2=                0x0aL, // r/w
		PS_REGISTER_T3=                0x0bL, // r/w
		PS_REGISTER_R0=                0x0cL, // r/w
		PS_REGISTER_R1=                0x0dL, // r/w
		PS_REGISTER_V1R0_SUM=          0x0eL, // r
		PS_REGISTER_EF_PROD=           0x0fL, // r

		PS_REGISTER_ONE=               PS_REGISTER_ZERO | PS_INPUTMAPPING_UNSIGNED_INVERT, // OK for final combiner
		PS_REGISTER_NEGATIVE_ONE=      PS_REGISTER_ZERO | PS_INPUTMAPPING_EXPAND_NORMAL,   // invalid for final combiner
		PS_REGISTER_ONE_HALF=          PS_REGISTER_ZERO | PS_INPUTMAPPING_HALFBIAS_NEGATE, // invalid for final combiner
		PS_REGISTER_NEGATIVE_ONE_HALF= PS_REGISTER_ZERO | PS_INPUTMAPPING_HALFBIAS_NORMAL, // invalid for final combiner
	};

	// FOG ALPHA is only available in final combiner
	// V1R0_SUM and EF_PROD are only available in final combiner (A,B,C,D inputs only)
	// V1R0_SUM_ALPHA and EF_PROD_ALPHA are not available
	// R0_ALPHA is initialized to T0_ALPHA in stage0

	enum
	{
		PS_CHANNEL_RGB=   0x00, // used as RGB source
		PS_CHANNEL_BLUE=  0x00, // used as ALPHA source
		PS_CHANNEL_ALPHA= 0x10, // used as RGB or ALPHA source
	};

	enum
	{                                 // valid in stage 0 1 2 3
		PS_TEXTUREMODES_NONE=                 0x00L, // * * * *
		PS_TEXTUREMODES_PROJECT2D=            0x01L, // * * * *
		PS_TEXTUREMODES_PROJECT3D=            0x02L, // * * * *
		PS_TEXTUREMODES_CUBEMAP=              0x03L, // * * * *
		PS_TEXTUREMODES_PASSTHRU=             0x04L, // * * * *
		PS_TEXTUREMODES_CLIPPLANE=            0x05L, // * * * *
		PS_TEXTUREMODES_BUMPENVMAP=           0x06L, // - * * *
		PS_TEXTUREMODES_BUMPENVMAP_LUM=       0x07L, // - * * *
		PS_TEXTUREMODES_BRDF=                 0x08L, // - - * *
		PS_TEXTUREMODES_DOT_ST=               0x09L, // - - * *
		PS_TEXTUREMODES_DOT_ZW=               0x0aL, // - - * *
		PS_TEXTUREMODES_DOT_RFLCT_DIFF=       0x0bL, // - - * -
		PS_TEXTUREMODES_DOT_RFLCT_SPEC=       0x0cL, // - - - *
		PS_TEXTUREMODES_DOT_STR_3D=           0x0dL, // - - - *
		PS_TEXTUREMODES_DOT_STR_CUBE=         0x0eL, // - - - *
		PS_TEXTUREMODES_DPNDNT_AR=            0x0fL, // - * * *
		PS_TEXTUREMODES_DPNDNT_GB=            0x10L, // - * * *
		PS_TEXTUREMODES_DOTPRODUCT=           0x11L, // - * * -
		PS_TEXTUREMODES_DOT_RFLCT_SPEC_CONST= 0x12L, // - - - *
		// 0x13-0x1f reserved
	};
	enum
	{                                 // valid in stage 0 1 2 3
		PS_COMBINEROUTPUT_IDENTITY=            0x00L, // y = x
		PS_COMBINEROUTPUT_BIAS=                0x08L, // y = x - 0.5
		PS_COMBINEROUTPUT_SHIFTLEFT_1=         0x10L, // y = x*2
		PS_COMBINEROUTPUT_SHIFTLEFT_1_BIAS=    0x18L, // y = (x - 0.5)*2
		PS_COMBINEROUTPUT_SHIFTLEFT_2=         0x20L, // y = x*4
		PS_COMBINEROUTPUT_SHIFTRIGHT_1=        0x30L, // y = x/2
	};

	struct D3DPIXELSHADERDEF
	{
	uint32    PSAlphaInputs[8];          // Alpha inputs for each stage
	uint32    PSFinalCombinerInputsABCD; // Final combiner inputs
	uint32    PSFinalCombinerInputsEFG;  // Final combiner inputs (continued)
	uint32    PSConstant0[8];            // C0 for each stage
	uint32    PSConstant1[8];            // C1 for each stage
	uint32    PSAlphaOutputs[8];         // Alpha output for each stage
	uint32    PSRGBInputs[8];            // RGB inputs for each stage
//	uint32    PSFinalCombinerConstant0;  // C0 in final combiner
//	uint32    PSFinalCombinerConstant1;  // C1 in final combiner
	uint32    PSRGBOutputs[8];           // Stage 0 RGB outputs
	uint32    PSCombinerCount;           // Active combiner count (Stages 0-7)
	uint32    PSTextureModes;            // Texture addressing modes
	uint32    PSDotMapping;              // Input mapping for dot product modes
	uint32    PSInputTexture;            // Texture source for some texture modes
	};
#ifndef PS_DOTMAPPING
#	define PS_DOTMAPPING(t0,t1,t2,t3) (((t3)<<8)|((t2)<<4)|(t1))
#endif

	// Mappings:
	// ZERO_TO_ONE         :rgb->(r,g,b): 0x0=>0.0, 0xff=>1.0
	// MINUS1_TO_1_D3D     :rgb->(r,g,b): 0x0=>-128/127, 0x01=>-1.0, 0x80=>0.0, 0xff=>1.0
	// MINUS1_TO_1_GL      :rgb->(r,g,b): 0x80=>-1.0, 0x7f=>1.0
	// MINUS1_TO_1         :rgb->(r,g,b): 0x80=>-128/127, 0x81=>-1.0, 0x0=>0.0, 0x7f=>1.0
	// HILO_1              :HL->(H,L,1.0): 0x0000=>0.0, 0xffff=>1.0
	// HILO_HEMISPHERE     :HL->(H,L,sqrt(1-H*H-L*L)): 0x8001=>-1.0, 0x0=>0.0, 0x7fff=>1.0, 0x8000=>-32768/32767

	enum PS_DOTMAPPING
	{                              // valid in stage 0 1 2 3
		PS_DOTMAPPING_ZERO_TO_ONE=         0x00L, // - * * *
		PS_DOTMAPPING_MINUS1_TO_1_D3D=     0x01L, // - * * *
		PS_DOTMAPPING_MINUS1_TO_1_GL=      0x02L, // - * * *
		PS_DOTMAPPING_MINUS1_TO_1=         0x03L, // - * * *
		PS_DOTMAPPING_HILO_1=              0x04L, // - * * *
		PS_DOTMAPPING_HILO_HEMISPHERE=     0x07L, // - * * *
	};

#ifndef PS_CONSTANTMAPPING
	#define PS_CONSTANTMAPPING(s0,s1,s2,s3,s4,s5,s6,s7) \
		(((uint32)(s0)&0xf)<< 0) | (((uint32)(s1)&0xf)<< 4) | \
		(((uint32)(s2)&0xf)<< 8) | (((uint32)(s3)&0xf)<<12) | \
		(((uint32)(s4)&0xf)<<16) | (((uint32)(s5)&0xf)<<20) | \
		(((uint32)(s6)&0xf)<<24) | (((uint32)(s7)&0xf)<<28)
#endif

#ifndef PS_FINALCOMBINERCONSTANTS
	#define PS_FINALCOMBINERCONSTANTS(c0,c1,flags) (((uint32)(flags) << 8) | ((uint32)(c0)&0xf)<< 0) | (((uint32)(c1)&0xf)<< 4)
#endif
	// c0 and c1 contain the offset of the D3D constant that corresponds to the
	// constants in the final combiner.  These mappings are only used in
	// SetPixelShaderConstant().  Flags contains values from PS_GLOBALFLAGS

	enum PS_GLOBALFLAGS
	{
		// if this flag is set, the texture mode for each texture stage is adjusted as follows:
		//     if set texture is a cubemap,
		//         change PS_TEXTUREMODES_PROJECT2D to PS_TEXTUREMODES_CUBEMAP
		//         change PS_TEXTUREMODES_PROJECT3D to PS_TEXTUREMODES_CUBEMAP
		//         change PS_TEXTUREMODES_DOT_STR_3D to PS_TEXTUREMODES_DOT_STR_CUBE
		//     if set texture is a volume texture,
		//         change PS_TEXTUREMODES_PROJECT2D to PS_TEXTUREMODES_PROJECT3D
		//         change PS_TEXTUREMODES_CUBEMAP to PS_TEXTUREMODES_PROJECT3D
		//         change PS_TEXTUREMODES_DOT_STR_CUBE to PS_TEXTUREMODES_DOT_STR_3D
		//     if set texture is neither cubemap or volume texture,
		//         change PS_TEXTUREMODES_PROJECT3D to PS_TEXTUREMODES_PROJECT2D
		//         change PS_TEXTUREMODES_CUBEMAP to PS_TEXTUREMODES_PROJECT2D

		PS_GLOBALFLAGS_NO_TEXMODE_ADJUST=     0x0000L, // don't adjust texture modes
		PS_GLOBALFLAGS_TEXMODE_ADJUST=        0x0001L, // adjust texture modes according to set texture
	};

#ifndef PS_COMBINERINPUTS
	#define PS_COMBINERINPUTS(a,b,c,d) (((a)<<24)|((b)<<16)|((c)<<8)|(d))
#endif

	enum PS_FINALCOMBINERSETTING
	{
		PS_FINALCOMBINERSETTING_CLAMP_SUM=     0x80, // V1+R0 sum clamped to [0,1]

		PS_FINALCOMBINERSETTING_COMPLEMENT_V1= 0x40, // unsigned invert mapping

		PS_FINALCOMBINERSETTING_COMPLEMENT_R0= 0x20, // unsigned invert mapping
	};
#ifndef COMBINEROUTPUTS
	#define COMBINEROUTPUTS(ab,cd,mux_sum,flags) (((flags)<<12)|((mux_sum)<<8)|((ab)<<4)|(cd))
#endif
#ifndef PS_TEXTUREMODES
	#define PS_TEXTUREMODES(t0,t1,t2,t3) (((t3)<<15)|((t2)<<10)|((t1)<<5)|(t0))
#endif

#ifndef PS_COMBINERCOUNT
	#define PS_COMBINERCOUNT(count, flags) (((flags)<<8)|(count))
#endif
	// count is 1-8, flags contains one or more values from PS_COMBINERCOUNTFLAGS

	enum PS_COMBINERCOUNTFLAGS
	{
		PS_COMBINERCOUNT_MUX_LSB=     0x0000L, // mux on r0.a lsb
		PS_COMBINERCOUNT_MUX_MSB=     0x0001L, // mux on r0.a msb

		PS_COMBINERCOUNT_SAME_C0=     0x0000L, // c0 same in each stage
		PS_COMBINERCOUNT_UNIQUE_C0=   0x0010L, // c0 unique in each stage

		PS_COMBINERCOUNT_SAME_C1=     0x0000L, // c1 same in each stage
		PS_COMBINERCOUNT_UNIQUE_C1=   0x0100L  // c1 unique in each stage
	};


}

enum
{
	NV_COMP_RGB					= NXboxTypes::PS_CHANNEL_RGB,
	NV_COMP_ALPHA				= NXboxTypes::PS_CHANNEL_ALPHA,
	NV_COMP_BLUE				= NXboxTypes::PS_CHANNEL_BLUE,

	NV_INPUT_ZERO				= NXboxTypes::PS_REGISTER_ZERO,
	NV_INPUT_COLOR0				= NXboxTypes::PS_REGISTER_C0,
	NV_INPUT_COLOR1				= NXboxTypes::PS_REGISTER_C1,
	NV_INPUT_FOG				= NXboxTypes::PS_REGISTER_FOG,
	NV_INPUT_PRIMARY			= NXboxTypes::PS_REGISTER_V0,
	NV_INPUT_SECONDARY			= NXboxTypes::PS_REGISTER_V1,
	NV_INPUT_SPARE0				= NXboxTypes::PS_REGISTER_R0,
	NV_INPUT_SPARE1				= NXboxTypes::PS_REGISTER_R1,
	NV_INPUT_TEXTURE0			= NXboxTypes::PS_REGISTER_T0,
	NV_INPUT_TEXTURE1			= NXboxTypes::PS_REGISTER_T1,
	NV_INPUT_TEXTURE2			= NXboxTypes::PS_REGISTER_T2,
	NV_INPUT_TEXTURE3			= NXboxTypes::PS_REGISTER_T3,
	NV_INPUT_EF					= NXboxTypes::PS_REGISTER_EF_PROD,
	NV_INPUT_COLORSUM			= NXboxTypes::PS_REGISTER_V1R0_SUM,

	NV_MAPPING_IDENTITY			= NXboxTypes::PS_INPUTMAPPING_UNSIGNED_IDENTITY,
	NV_MAPPING_INVERT			= NXboxTypes::PS_INPUTMAPPING_UNSIGNED_INVERT,
	NV_MAPPING_EXPNORMAL		= NXboxTypes::PS_INPUTMAPPING_EXPAND_NORMAL,
	NV_MAPPING_EXPNEGATE		= NXboxTypes::PS_INPUTMAPPING_EXPAND_NEGATE,
	NV_MAPPING_HALFBIASNORMAL	= NXboxTypes::PS_INPUTMAPPING_HALFBIAS_NORMAL,
	NV_MAPPING_HALFBIASNEGATE	= NXboxTypes::PS_INPUTMAPPING_HALFBIAS_NEGATE,
	NV_MAPPING_SIGNEDIDENTITY	= NXboxTypes::PS_INPUTMAPPING_SIGNED_IDENTITY,
	NV_MAPPING_SIGNEDNEGATE		= NXboxTypes::PS_INPUTMAPPING_SIGNED_NEGATE,

	NV_SCALE_ONE				= NXboxTypes::PS_COMBINEROUTPUT_IDENTITY,
	NV_SCALE_ONE_BIAS			= NXboxTypes::PS_COMBINEROUTPUT_BIAS,
	NV_SCALE_TWO				= NXboxTypes::PS_COMBINEROUTPUT_SHIFTLEFT_1,
	NV_SCALE_TWO_BIAS			= NXboxTypes::PS_COMBINEROUTPUT_SHIFTLEFT_1_BIAS,	
	NV_SCALE_FOUR				= NXboxTypes::PS_COMBINEROUTPUT_SHIFTLEFT_2,
	NV_SCALE_HALF				= NXboxTypes::PS_COMBINEROUTPUT_SHIFTRIGHT_1,

	NV_OUTPUT_DISCARD			= NXboxTypes::PS_REGISTER_DISCARD,
	NV_OUTPUT_PRIMARY			= NXboxTypes::PS_REGISTER_V0,
	NV_OUTPUT_SECONDARY			= NXboxTypes::PS_REGISTER_V1,
	NV_OUTPUT_SPARE0			= NXboxTypes::PS_REGISTER_R0,
	NV_OUTPUT_SPARE1			= NXboxTypes::PS_REGISTER_R1,
	NV_OUTPUT_TEXTURE0			= NXboxTypes::PS_REGISTER_T0,
	NV_OUTPUT_TEXTURE1			= NXboxTypes::PS_REGISTER_T1
};

// -------------------------------------------------------------------

// -------------------------------------------------------------------
//  CRC_ExtAttributes_NV10
// -------------------------------------------------------------------
class CRC_ExtAttributes_NV10 : public CRC_ExtAttributes
{
public:
	// Register combiners
	NXboxTypes::D3DPIXELSHADERDEF m_PixelSharedDef;
	M_INLINE void ClearInternal()
	{
//		m_PixelSharedDef = s_DefaultPixelSharedDef;
//		memcpy(&m_PixelSharedDef, &s_DefaultPixelSharedDef, sizeof(s_DefaultPixelSharedDef));
//		memset(&m_PixelSharedDef, 0, 53*4);
		m_PixelSharedDef.PSConstant0[0] = 0xffffffff;
		m_PixelSharedDef.PSConstant1[0] = 0xffffffff;
//		m_PixelSharedDef.PSCompareMode = 0; 
		m_PixelSharedDef.PSCombinerCount = PS_COMBINERCOUNT(1, NXboxTypes::PS_COMBINERCOUNT_SAME_C0 | NXboxTypes::PS_COMBINERCOUNT_SAME_C1);
		m_PixelSharedDef.PSTextureModes = PS_TEXTUREMODES(NXboxTypes::PS_TEXTUREMODES_CUBEMAP, NXboxTypes::PS_TEXTUREMODES_CUBEMAP, NXboxTypes::PS_TEXTUREMODES_CUBEMAP, NXboxTypes::PS_TEXTUREMODES_CUBEMAP);
		m_PixelSharedDef.PSDotMapping = PS_DOTMAPPING(0,NXboxTypes::PS_DOTMAPPING_MINUS1_TO_1_D3D,NXboxTypes::PS_DOTMAPPING_MINUS1_TO_1_D3D,NXboxTypes::PS_DOTMAPPING_MINUS1_TO_1_D3D);
//		m_PixelSharedDef.PSDotMapping = PS_DOTMAPPING(0,NXboxTypes::PS_DOTMAPPING_MINUS1_TO_1_D3D,NXboxTypes::PS_DOTMAPPING_MINUS1_TO_1_D3D,NXboxTypes::PS_DOTMAPPING_MINUS1_TO_1_D3D);
		m_PixelSharedDef.PSInputTexture = 0;
		
//		m_PixelSharedDef.PSC0Mapping = PS_CONSTANTMAPPING(0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
//		m_PixelSharedDef.PSC1Mapping = PS_CONSTANTMAPPING(0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
//		m_PixelSharedDef.PSFinalCombinerConstants = PS_FINALCOMBINERCONSTANTS(0xff, 0xff, NXboxTypes::PS_GLOBALFLAGS_TEXMODE_ADJUST);
	}

	M_INLINE bool SetCheckChangedNumCombiners(int _nComb, bool _bPerCombinerColors = false)
	{
		uint32 ToSet;
		if (_bPerCombinerColors)
		{
			if (_nComb)
				ToSet = PS_COMBINERCOUNT(_nComb, NXboxTypes::PS_COMBINERCOUNT_UNIQUE_C0 | NXboxTypes::PS_COMBINERCOUNT_UNIQUE_C1);
			else
				ToSet = PS_COMBINERCOUNT(1, NXboxTypes::PS_COMBINERCOUNT_UNIQUE_C0 | NXboxTypes::PS_COMBINERCOUNT_UNIQUE_C1);
		}
		else
		{
			if (_nComb)
				ToSet = PS_COMBINERCOUNT(_nComb, NXboxTypes::PS_COMBINERCOUNT_SAME_C0 | NXboxTypes::PS_COMBINERCOUNT_SAME_C1);
			else
				ToSet = PS_COMBINERCOUNT(1, NXboxTypes::PS_COMBINERCOUNT_SAME_C0 | NXboxTypes::PS_COMBINERCOUNT_SAME_C1);
		}

		if (m_PixelSharedDef.PSCombinerCount != ToSet)
		{
			m_PixelSharedDef.PSCombinerCount = ToSet;
			return true;
		}

		return false;
	}

	M_INLINE void SetDotMapping(int _Stage1, int _Stage2 = NXboxTypes::PS_DOTMAPPING_MINUS1_TO_1_D3D, int _Stage3 = NXboxTypes::PS_DOTMAPPING_MINUS1_TO_1_D3D)
	{
		m_PixelSharedDef.PSDotMapping = PS_DOTMAPPING(0,_Stage1,_Stage2,_Stage3);
	}

	M_INLINE void SetNoColorUsage()
	{
		m_PixelSharedDef.PSCombinerCount |= 1 << 31;
	}

	M_INLINE bool SetCheckChangedConst0(int _iConst, uint32 _Value)
	{
		if (m_PixelSharedDef.PSConstant0[_iConst] != _Value)
		{
			m_PixelSharedDef.PSConstant0[_iConst] = _Value;
			return true;
		}
		else
			return false;
	}

	M_INLINE bool SetCheckChangedConst1(int _iConst, uint32 _Value)
	{
		if (m_PixelSharedDef.PSConstant1[_iConst] != _Value)
		{
			m_PixelSharedDef.PSConstant1[_iConst] = _Value;
			return true;
		}
		else
			return false;
	}

	M_INLINE bool SetCheckChangedConst0(uint32 _Value)
	{
		if (m_PixelSharedDef.PSConstant0[0] != _Value)
		{
			m_PixelSharedDef.PSConstant0[0] = _Value;
			return true;
		}
		else
			return false;
	}

	M_INLINE bool SetCheckChangedConst1(uint32 _Value)
	{
		if (m_PixelSharedDef.PSConstant1[0] != _Value)
		{
			m_PixelSharedDef.PSConstant1[0] = _Value;
			return true;
		}
		else
			return false;
	}

	M_INLINE bool SetCheckChangedInputRGB(int _Combiner, uint32 _Reg1, uint32 _Reg2 = 0, uint32 _Reg3 = 0, uint32 _Reg4 = 0)
	{
		uint32 Changed = PS_COMBINERINPUTS(_Reg1,_Reg2,_Reg3,_Reg4);
		if (m_PixelSharedDef.PSRGBInputs[_Combiner] != Changed)
		{
			m_PixelSharedDef.PSRGBInputs[_Combiner] = Changed;
			return true;
		}
		else
			return false;
	}

	M_INLINE bool SetCheckChangedOutputRGB(int _Combiner, int _DestAB, int _DestCD, int _DestSum, int _Scale, int _bABDot, int _bCDDot, int _bMuxSum)
	{
		uint32 Changed = COMBINEROUTPUTS(_DestAB, _DestCD, _DestSum, _Scale | _bMuxSum << 2 | _bABDot << 1 | _bCDDot);
		if (m_PixelSharedDef.PSRGBOutputs[_Combiner] != Changed)
		{
			m_PixelSharedDef.PSRGBOutputs[_Combiner] = Changed;
			return true;
		}
		else
			return false;
	}

	M_INLINE bool SetCheckChangedOutputAlpha(int _Combiner, int _DestAB, int _DestCD, int _DestSum, int _Scale, int _bABDot, int _bCDDot, int _bMuxSum)
	{
		uint32 Changed = COMBINEROUTPUTS(_DestAB, _DestCD, _DestSum, _Scale | _bMuxSum << 2 | _bABDot << 1 | _bCDDot);
		if (m_PixelSharedDef.PSAlphaOutputs[_Combiner] != Changed)
		{
			m_PixelSharedDef.PSAlphaOutputs[_Combiner] = Changed;
			return true;
		}
		else
			return false;
	}

	M_INLINE bool SetCheckChangedInputAlpha(int _Combiner, uint32 _Reg1, uint32 _Reg2 = NV_COMP_ALPHA, uint32 _Reg3 = NV_COMP_ALPHA, uint32 _Reg4 = NV_COMP_ALPHA)
	{
		uint32 Changed = PS_COMBINERINPUTS(_Reg1,_Reg2,_Reg3,_Reg4);
		if (m_PixelSharedDef.PSAlphaInputs[_Combiner] != Changed)
		{
			m_PixelSharedDef.PSAlphaInputs[_Combiner] = Changed;
			return true;
		}
		else
			return false;
	}

	M_INLINE bool SetCheckChangedFinal(uint32 _Reg0, uint32 _Reg1, uint32 _Reg2, uint32 _Reg3, uint32 _Reg4, uint32 _Reg5, uint32 _RegA, int _bColorSumClamp = false)
	{
		uint32 Changed = PS_COMBINERINPUTS(_Reg0,_Reg1,_Reg2,_Reg3);
		if (m_PixelSharedDef.PSFinalCombinerInputsABCD != Changed)
		{
			m_PixelSharedDef.PSFinalCombinerInputsABCD = Changed;
			m_PixelSharedDef.PSFinalCombinerInputsEFG = PS_COMBINERINPUTS(_Reg4, _Reg5, _RegA,_bColorSumClamp<<7);
			return true;
		}

		Changed = PS_COMBINERINPUTS(_Reg4, _Reg5, _RegA,_bColorSumClamp<<7);
		if(m_PixelSharedDef.PSFinalCombinerInputsEFG != Changed)
		{
			m_PixelSharedDef.PSFinalCombinerInputsEFG = Changed;
			return true;
		}

		return false;
	}

	M_INLINE bool CheckChangedClear(int _Combiner)
	{
		if(m_PixelSharedDef.PSRGBInputs[_Combiner] != 0)
		{
			m_PixelSharedDef.PSRGBInputs[_Combiner] = 0;
			m_PixelSharedDef.PSAlphaInputs[_Combiner] = 0;
			m_PixelSharedDef.PSRGBOutputs[_Combiner] = 0;
			m_PixelSharedDef.PSAlphaOutputs[_Combiner] = 0;
			return true;
		}

		if (m_PixelSharedDef.PSAlphaInputs[_Combiner] != 0)
		{
			m_PixelSharedDef.PSAlphaInputs[_Combiner] = 0;
			m_PixelSharedDef.PSRGBOutputs[_Combiner] = 0;
			m_PixelSharedDef.PSAlphaOutputs[_Combiner] = 0;
			return true;
		}

		if (m_PixelSharedDef.PSRGBOutputs[_Combiner] != 0)
		{
			m_PixelSharedDef.PSRGBOutputs[_Combiner] = 0;
			m_PixelSharedDef.PSAlphaOutputs[_Combiner] = 0;
			return true;
		}

		if (m_PixelSharedDef.PSAlphaOutputs[_Combiner] != 0)
		{
			m_PixelSharedDef.PSAlphaOutputs[_Combiner] = 0;
			return true;
		}

		return false;
	}

	M_INLINE bool CheckChangedClearFinal()
	{
		if (m_PixelSharedDef.PSFinalCombinerInputsABCD != 0)
		{
			m_PixelSharedDef.PSFinalCombinerInputsABCD = 0;
			m_PixelSharedDef.PSFinalCombinerInputsEFG = 0;
			return true;
		}

		if (m_PixelSharedDef.PSFinalCombinerInputsEFG != 0)
		{
			m_PixelSharedDef.PSFinalCombinerInputsEFG = 0;
			return true;
		}

		return false;
	}

	M_INLINE void SetNumCombiners(int _nComb, bool _bPerCombinerColors = false)
	{
		if (_bPerCombinerColors)
		{
			if (_nComb)
				m_PixelSharedDef.PSCombinerCount = PS_COMBINERCOUNT(_nComb, NXboxTypes::PS_COMBINERCOUNT_UNIQUE_C0 | NXboxTypes::PS_COMBINERCOUNT_UNIQUE_C1);
			else
				m_PixelSharedDef.PSCombinerCount = PS_COMBINERCOUNT(1, NXboxTypes::PS_COMBINERCOUNT_UNIQUE_C0 | NXboxTypes::PS_COMBINERCOUNT_UNIQUE_C1);
		}
		else
		{
			if (_nComb)
				m_PixelSharedDef.PSCombinerCount = PS_COMBINERCOUNT(_nComb, NXboxTypes::PS_COMBINERCOUNT_SAME_C0 | NXboxTypes::PS_COMBINERCOUNT_SAME_C1);
			else
				m_PixelSharedDef.PSCombinerCount = PS_COMBINERCOUNT(1, NXboxTypes::PS_COMBINERCOUNT_SAME_C0 | NXboxTypes::PS_COMBINERCOUNT_SAME_C1);
		}
	}

	M_INLINE void SetConst0(int _iConst, uint32 _Value)
	{
		m_PixelSharedDef.PSConstant0[_iConst] = _Value;
	}

	M_INLINE void SetConst1(int _iConst, uint32 _Value)
	{
		m_PixelSharedDef.PSConstant1[_iConst] = _Value;
	}

	M_INLINE void SetConst0(uint32 _Value)
	{
		m_PixelSharedDef.PSConstant0[0] = _Value;
	}

	M_INLINE void SetConst1(uint32 _Value)
	{
		m_PixelSharedDef.PSConstant1[0] = _Value;
	}

	M_INLINE void Clear()
	{
		m_AttribType = CRC_ATTRIBTYPE_NV10;

		ClearInternal();
	}

	M_INLINE void SetInputRGB(int _Combiner, uint32 _Reg1, uint32 _Reg2 = 0, uint32 _Reg3 = 0, uint32 _Reg4 = 0)
	{
		m_PixelSharedDef.PSRGBInputs[_Combiner] = PS_COMBINERINPUTS(_Reg1,_Reg2,_Reg3,_Reg4);
	}

	M_INLINE void SetOutputRGB(int _Combiner, int _DestAB, int _DestCD, int _DestSum, int _Scale, int _bABDot, int _bCDDot, int _bMuxSum)
	{
		m_PixelSharedDef.PSRGBOutputs[_Combiner] = COMBINEROUTPUTS(_DestAB, _DestCD, _DestSum, _Scale | _bMuxSum << 2 | _bABDot << 1 | _bCDDot);
	}

	M_INLINE void SetOutputAlpha(int _Combiner, int _DestAB, int _DestCD, int _DestSum, int _Scale, int _bABDot, int _bCDDot, int _bMuxSum)
	{
		m_PixelSharedDef.PSAlphaOutputs[_Combiner] = COMBINEROUTPUTS(_DestAB, _DestCD, _DestSum, _Scale | _bMuxSum << 2 | _bABDot << 1 | _bCDDot);
	}

	M_INLINE void SetInputAlpha(int _Combiner, uint32 _Reg1, uint32 _Reg2 = NV_COMP_ALPHA, uint32 _Reg3 = NV_COMP_ALPHA, uint32 _Reg4 = NV_COMP_ALPHA)
	{
		m_PixelSharedDef.PSAlphaInputs[_Combiner] = PS_COMBINERINPUTS(_Reg1,_Reg2,_Reg3,_Reg4);
	}

	M_INLINE void SetFinal(uint32 _Reg0, uint32 _Reg1, uint32 _Reg2, uint32 _Reg3, uint32 _Reg4, uint32 _Reg5, uint32 _RegA, int _bColorSumClamp = false)
	{
		m_PixelSharedDef.PSFinalCombinerInputsABCD = PS_COMBINERINPUTS(_Reg0,_Reg1,_Reg2,_Reg3);
		m_PixelSharedDef.PSFinalCombinerInputsEFG = PS_COMBINERINPUTS(_Reg4, _Reg5, _RegA,_bColorSumClamp<<7);
	}

	M_INLINE void Clear(int _Combiner)
	{
		m_PixelSharedDef.PSRGBInputs[_Combiner] = 0;
		m_PixelSharedDef.PSAlphaInputs[_Combiner] = 0;
		m_PixelSharedDef.PSRGBOutputs[_Combiner] = 0;
		m_PixelSharedDef.PSAlphaOutputs[_Combiner] = 0;
	}

	M_INLINE void ClearFinal()
	{
		m_PixelSharedDef.PSFinalCombinerInputsABCD = 0;
		m_PixelSharedDef.PSFinalCombinerInputsEFG = 0;
	}



	M_INLINE void Fixup()
	{
/*		int nComb = m_PixelSharedDef.PSCombinerCount & 0xff;
		for (int i = nComb; i < MaxCombiners; ++i)
			m_PixelSharedDef.PSAlphaInputs[i] = 0;
		for (int i = nComb; i < MaxCombiners; ++i)
			m_PixelSharedDef.PSAlphaOutputs[i] = 0;
		for (int i = nComb; i < MaxCombiners; ++i)
			m_PixelSharedDef.PSRGBInputs[i] = 0;
		for (int i = nComb; i < MaxCombiners; ++i)
			m_PixelSharedDef.PSRGBOutputs[i] = 0;

		FixupCol();*/
	}
	
};

// -------------------------------------------------------------------
//  NV_TEXTURE_SHADER
// -------------------------------------------------------------------

enum
{

	NV_TS_TEXTURE0 = 0,
	NV_TS_TEXTURE1,
	NV_TS_TEXTURE2,
	NV_TS_TEXTURE3,
	NV_TS_TEXTURE4,
	NV_TS_TEXTURE5,
	NV_TS_TEXTURE6,
	NV_TS_TEXTURE7,

	NV_TS_TEXTURE_1D									= NXboxTypes::PS_TEXTUREMODES_PROJECT2D,
	NV_TS_TEXTURE_2D									= NXboxTypes::PS_TEXTUREMODES_PROJECT3D,
	NV_TS_CONST_EYE_NV									= NXboxTypes::PS_TEXTUREMODES_CUBEMAP,
	NV_TS_PASS_THROUGH_NV								= NXboxTypes::PS_TEXTUREMODES_PASSTHRU,
	NV_TS_CULL_FRAGMENT_NV								= NXboxTypes::PS_TEXTUREMODES_CLIPPLANE,
	NV_TS_OFFSET_TEXTURE_2D_NV							= NXboxTypes::PS_TEXTUREMODES_BUMPENVMAP,
	NV_TS_DEPENDENT_AR_TEXTURE_2D_NV					= NXboxTypes::PS_TEXTUREMODES_DPNDNT_AR,
	NV_TS_DEPENDENT_GB_TEXTURE_2D_NV					= NXboxTypes::PS_TEXTUREMODES_DPNDNT_GB,
	NV_TS_DOT_PRODUCT_NV								= NXboxTypes::PS_TEXTUREMODES_DOTPRODUCT,
	NV_TS_DOT_PRODUCT_DEPTH_REPLACE_NV					= NXboxTypes::PS_TEXTUREMODES_DOT_ZW,
	NV_TS_DOT_PRODUCT_TEXTURE_2D_NV						= NXboxTypes::PS_TEXTUREMODES_DOT_ST,
	NV_TS_DOT_PRODUCT_TEXTURE_3D_NV						= NXboxTypes::PS_TEXTUREMODES_DOT_STR_3D,
	NV_TS_DOT_PRODUCT_TEXTURE_CUBE_MAP_NV				= NXboxTypes::PS_TEXTUREMODES_DOT_STR_CUBE,
	NV_TS_DOT_PRODUCT_DIFFUSE_CUBE_MAP_NV				= NXboxTypes::PS_TEXTUREMODES_DOT_RFLCT_DIFF,
	NV_TS_DOT_PRODUCT_REFLECT_CUBE_MAP_NV				= NXboxTypes::PS_TEXTUREMODES_DOT_RFLCT_SPEC,
	NV_TS_DOT_PRODUCT_CONST_EYE_REFLECT_CUBE_MAP_NV		= NXboxTypes::PS_TEXTUREMODES_DOT_RFLCT_SPEC_CONST,
};

#ifndef PS_INPUTTEXTURE
#define PS_INPUTTEXTURE(t0,t1,t2,t3) (((t3)<<20)|((t2)<<16))
#endif

// -------------------------------------------------------------------
//  CRC_ExtAttributes_NV20
// -------------------------------------------------------------------
class CRC_ExtAttributes_NV20 : public CRC_ExtAttributes_NV10
{
public:
	// NV Texture shader
	M_INLINE void Clear(int _Combiner)
	{
		CRC_ExtAttributes_NV10::Clear(_Combiner);
	}
	M_INLINE void Clear()
	{
		m_AttribType = CRC_ATTRIBTYPE_NV20;

		ClearInternal();

//		m_PixelSharedDef.PSTextureModes = PS_TEXTUREMODES(NXboxTypes::PS_TEXTUREMODES_CUBEMAP, NXboxTypes::PS_TEXTUREMODES_CUBEMAP, NXboxTypes::PS_TEXTUREMODES_CUBEMAP, NXboxTypes::PS_TEXTUREMODES_CUBEMAP);
//		m_PixelSharedDef.PSInputTexture = 0;
	}

	M_INLINE void SetTexureOps(int _Op0 = 0, int _Op1 = 0, int _Op2 = 0, int _Op3 = 0, int _Prev1 = NV_TS_TEXTURE0, int _Prev2 = NV_TS_TEXTURE0, int _Prev3 = NV_TS_TEXTURE0)
	{
		m_PixelSharedDef.PSTextureModes = PS_TEXTUREMODES(_Op0,_Op1,_Op2,_Op3);
		m_PixelSharedDef.PSInputTexture = PS_INPUTTEXTURE(0, _Prev1,_Prev2,_Prev3);
	}

	M_INLINE CRC_ExtAttributes_NV20()
	{
		Clear();
	}
};

#endif // Xbox
#endif // SUPPORT_REGISTERCOMBINERS

#endif // _INC_MRndrGL_RegCombiners

