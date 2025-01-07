
#include "PCH.h"

#include "MDisplayPS3.h"
#include "MRenderPS3_Context.h"
#include "MRenderPS3_Def.h"

#include "../../XR/XRVBContainer.h"

namespace NRenderPS3GCM
{

uint FloatToGfp16(fp32 _Val)
{
	union
	{
		fp32 m_FVal;
		uint32 m_UVal;
	} ConvVal;
	ConvVal.m_FVal = _Val;
	uint32 UVal = ConvVal.m_UVal;

	uint E = (UVal >> 23) & 0xff;
	if(E < 112)
		return 0;
	E -= 112;

	return (E << 10) | ((UVal >> 16) & 0x8000) | ((UVal >> 13) & 0x3ff);
}

const char* gs_TexEnvPrograms[10] = {"TexEnvMode00", "TexEnvMode01", "TexEnvMode02", "TexEnvMode03", "TexEnvMode04", "TexEnvMode05", "TexEnvMode06", "TexEnvMode07", "TexEnvMode08", "TexEnvMode09"};
uint32 gs_TexEnvProgramHash[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

struct ClipSetting
{
	ClipSetting(uint _Clip0, uint _Clip1, uint _Clip2, uint _Clip3, uint _Clip4, uint _Clip5)
	{
		m_Clip0 = _Clip0;
		m_Clip1 = _Clip1;
		m_Clip2 = _Clip2;
		m_Clip3 = _Clip3;
		m_Clip4 = _Clip4;
		m_Clip5 = _Clip5;
	}
	uint16 m_Clip0:2;
	uint16 m_Clip1:2;
	uint16 m_Clip2:2;
	uint16 m_Clip3:2;
	uint16 m_Clip4:2;
	uint16 m_Clip5:2;
};
static ClipSetting ClipSettings[7] = {ClipSetting(CELL_GCM_USER_CLIP_PLANE_DISABLE, CELL_GCM_USER_CLIP_PLANE_DISABLE, CELL_GCM_USER_CLIP_PLANE_DISABLE, CELL_GCM_USER_CLIP_PLANE_DISABLE, CELL_GCM_USER_CLIP_PLANE_DISABLE, CELL_GCM_USER_CLIP_PLANE_DISABLE),
						 ClipSetting(CELL_GCM_USER_CLIP_PLANE_ENABLE_GE, CELL_GCM_USER_CLIP_PLANE_DISABLE, CELL_GCM_USER_CLIP_PLANE_DISABLE, CELL_GCM_USER_CLIP_PLANE_DISABLE, CELL_GCM_USER_CLIP_PLANE_DISABLE, CELL_GCM_USER_CLIP_PLANE_DISABLE),
						 ClipSetting(CELL_GCM_USER_CLIP_PLANE_ENABLE_GE, CELL_GCM_USER_CLIP_PLANE_ENABLE_GE, CELL_GCM_USER_CLIP_PLANE_DISABLE, CELL_GCM_USER_CLIP_PLANE_DISABLE, CELL_GCM_USER_CLIP_PLANE_DISABLE, CELL_GCM_USER_CLIP_PLANE_DISABLE),
						 ClipSetting(CELL_GCM_USER_CLIP_PLANE_ENABLE_GE, CELL_GCM_USER_CLIP_PLANE_ENABLE_GE, CELL_GCM_USER_CLIP_PLANE_ENABLE_GE, CELL_GCM_USER_CLIP_PLANE_DISABLE, CELL_GCM_USER_CLIP_PLANE_DISABLE, CELL_GCM_USER_CLIP_PLANE_DISABLE),
						 ClipSetting(CELL_GCM_USER_CLIP_PLANE_ENABLE_GE, CELL_GCM_USER_CLIP_PLANE_ENABLE_GE, CELL_GCM_USER_CLIP_PLANE_ENABLE_GE, CELL_GCM_USER_CLIP_PLANE_ENABLE_GE, CELL_GCM_USER_CLIP_PLANE_DISABLE, CELL_GCM_USER_CLIP_PLANE_DISABLE),
						 ClipSetting(CELL_GCM_USER_CLIP_PLANE_ENABLE_GE, CELL_GCM_USER_CLIP_PLANE_ENABLE_GE, CELL_GCM_USER_CLIP_PLANE_ENABLE_GE, CELL_GCM_USER_CLIP_PLANE_ENABLE_GE, CELL_GCM_USER_CLIP_PLANE_ENABLE_GE, CELL_GCM_USER_CLIP_PLANE_DISABLE),
						 ClipSetting(CELL_GCM_USER_CLIP_PLANE_ENABLE_GE, CELL_GCM_USER_CLIP_PLANE_ENABLE_GE, CELL_GCM_USER_CLIP_PLANE_ENABLE_GE, CELL_GCM_USER_CLIP_PLANE_ENABLE_GE, CELL_GCM_USER_CLIP_PLANE_ENABLE_GE, CELL_GCM_USER_CLIP_PLANE_ENABLE_GE)};
/*
M_FORCEINLINE static bool ConvertRasterMode(CRC_Attributes* _pAttrib)
{
	if(_pAttrib->m_RasterMode == CRC_RASTERMODE_NONE) return false;
	_pAttrib->m_Flags |= CRC_FLAGS_BLEND;
	switch(_pAttrib->m_RasterMode)
	{
	case CRC_RASTERMODE_ALPHABLEND: _pAttrib->m_SourceBlend = CRC_BLEND_SRCALPHA; _pAttrib->m_DestBlend = CRC_BLEND_INVSRCALPHA; return true;
	case CRC_RASTERMODE_LIGHTMAPBLEND: _pAttrib->m_SourceBlend = CRC_BLEND_DESTCOLOR; _pAttrib->m_DestBlend = CRC_BLEND_ZERO; return true;
	case CRC_RASTERMODE_MULTIPLY: _pAttrib->m_SourceBlend = CRC_BLEND_DESTCOLOR; _pAttrib->m_DestBlend = CRC_BLEND_ZERO; return true;
	case CRC_RASTERMODE_ADD: _pAttrib->m_SourceBlend = CRC_BLEND_ONE; _pAttrib->m_DestBlend = CRC_BLEND_ONE; return true;
	case CRC_RASTERMODE_ALPHAADD: _pAttrib->m_SourceBlend = CRC_BLEND_SRCALPHA; _pAttrib->m_DestBlend = CRC_BLEND_ONE; return true;
	case CRC_RASTERMODE_ALPHAMULTIPLY: _pAttrib->m_SourceBlend = CRC_BLEND_DESTCOLOR; _pAttrib->m_DestBlend = CRC_BLEND_INVSRCALPHA; return true;
	case CRC_RASTERMODE_MULTIPLY2: _pAttrib->m_SourceBlend = CRC_BLEND_DESTCOLOR; _pAttrib->m_DestBlend = CRC_BLEND_SRCCOLOR; return true;
	case CRC_RASTERMODE_MULADD: _pAttrib->m_SourceBlend = CRC_BLEND_DESTCOLOR; _pAttrib->m_DestBlend = CRC_BLEND_ONE; return true;
	case CRC_RASTERMODE_ONE_ALPHA: _pAttrib->m_SourceBlend = CRC_BLEND_ONE; _pAttrib->m_DestBlend = CRC_BLEND_SRCALPHA; return true;
	case CRC_RASTERMODE_ONE_INVALPHA: _pAttrib->m_SourceBlend = CRC_BLEND_ONE; _pAttrib->m_DestBlend = CRC_BLEND_INVSRCALPHA; return true;
	case CRC_RASTERMODE_DESTALPHABLEND: _pAttrib->m_SourceBlend = CRC_BLEND_DESTALPHA; _pAttrib->m_DestBlend = CRC_BLEND_INVDESTALPHA; return true;
	case CRC_RASTERMODE_DESTADD: _pAttrib->m_SourceBlend = CRC_BLEND_DESTCOLOR; _pAttrib->m_DestBlend = CRC_BLEND_SRCALPHA; return true;
	case CRC_RASTERMODE_MULADD_DESTALPHA: _pAttrib->m_SourceBlend = CRC_BLEND_DESTALPHA; _pAttrib->m_DestBlend = CRC_BLEND_ONE; return true;
	}

	M_BREAKPOINT;
	return false;
}
*/
static const uint16 ConvertOpArray[] =
{
	CELL_GCM_KEEP,		// CRC_STENCILOP_NONE			= 0,
	CELL_GCM_ZERO,		// CRC_STENCILOP_ZERO			= 1,
	CELL_GCM_REPLACE,	// CRC_STENCILOP_REPLACE		= 2,
	CELL_GCM_INCR,		// CRC_STENCILOP_INC			= 3,
	CELL_GCM_DECR,		// CRC_STENCILOP_DEC			= 4,
	CELL_GCM_INVERT,	// CRC_STENCILOP_INVERT		= 5,
	CELL_GCM_INCR_WRAP,	// CRC_STENCILOP_INCWRAP		= 6,
	CELL_GCM_DECR_WRAP,	// CRC_STENCILOP_DECWRAP		= 7,
};

M_FORCEINLINE static uint32 ConvertToGCMOp(uint32 _Op)
{
	M_ASSERT(_Op >= 0 && _Op <= CRC_STENCILOP_DECWRAP, "Not a valid blend mode");

	return ConvertOpArray[_Op];
}

static const uint16 ConvertBlendArray[] =
{
	0,								// Not valid
	CELL_GCM_ZERO,					// CRC_BLEND_ZERO				= 1,
	CELL_GCM_ONE,					// CRC_BLEND_ONE				= 2,
	CELL_GCM_SRC_COLOR,				// CRC_BLEND_SRCCOLOR			= 3,
	CELL_GCM_ONE_MINUS_SRC_COLOR,	// CRC_BLEND_INVSRCCOLOR		= 4,
	CELL_GCM_SRC_ALPHA,				// CRC_BLEND_SRCALPHA			= 5,
	CELL_GCM_ONE_MINUS_SRC_ALPHA,	// CRC_BLEND_INVSRCALPHA		= 6,
	CELL_GCM_DST_ALPHA,				// CRC_BLEND_DESTALPHA			= 7,
	CELL_GCM_ONE_MINUS_DST_ALPHA,	// CRC_BLEND_INVDESTALPHA		= 8,
	CELL_GCM_DST_COLOR,				// CRC_BLEND_DESTCOLOR			= 9,
	CELL_GCM_ONE_MINUS_DST_COLOR,	// CRC_BLEND_INVDESTCOLOR		= 10,
	CELL_GCM_SRC_ALPHA_SATURATE,	// CRC_BLEND_SRCALPHASAT		= 11,
};

M_FORCEINLINE static uint32 ConvertToGCMBlend(uint32 _Blend)
{
	M_ASSERT(_Blend > 0 && _Blend <= CRC_BLEND_SRCALPHASAT, "Not a valid blend mode");
	return ConvertBlendArray[_Blend];
}

static const uint16 ConvertCompareArray[] =
{
	0,					// Not valid
	CELL_GCM_NEVER,		// CRC_COMPARE_NEVER
	CELL_GCM_LESS,		// CRC_COMPARE_LESS
	CELL_GCM_EQUAL,		// CRC_COMPARE_EQUAL
	CELL_GCM_LEQUAL,	// CRC_COMPARE_LESSEQUAL
	CELL_GCM_GREATER,	// CRC_COMPARE_GREATER
	CELL_GCM_NOTEQUAL,	// CRC_COMPARE_NOTEQUAL
	CELL_GCM_GEQUAL,	// CRC_COMPARE_GREATEREQUAL
	CELL_GCM_ALWAYS,	// CRC_COMPARE_ALWAYS
};

M_FORCEINLINE static uint32 ConvertToGCMCompare(uint32 _Compare)
{
	M_ASSERT(_Compare > 0 && _Compare <= CRC_COMPARE_ALWAYS, "Not a valid compare mode");
	return ConvertCompareArray[_Compare];
}

static const uint8 ZCullConvertCompareArray[] =
{
	0,							// Not valid
	CELL_GCM_ZCULL_LESS,		// CRC_COMPARE_NEVER
	CELL_GCM_ZCULL_LESS,		// CRC_COMPARE_LESS
	CELL_GCM_ZCULL_LESS,		// CRC_COMPARE_EQUAL
	CELL_GCM_ZCULL_LESS,		// CRC_COMPARE_LESSEQUAL
	CELL_GCM_ZCULL_GREATER,		// CRC_COMPARE_GREATER
	CELL_GCM_ZCULL_GREATER,		// CRC_COMPARE_NOTEQUAL
	CELL_GCM_ZCULL_GREATER,		// CRC_COMPARE_GREATEREQUAL
	CELL_GCM_ZCULL_GREATER,		// CRC_COMPARE_ALWAYS
};

M_FORCEINLINE static uint32 ConvertToZCullCompare(uint32 _Compare)
{
	M_ASSERT(_Compare > 0 && _Compare <= CRC_COMPARE_ALWAYS, "Not a valid compare mode");
	return ZCullConvertCompareArray[_Compare];
}

//----------------------------------------------------------------
// Attribute overrides
//----------------------------------------------------------------
class CGetNewRegsEmpty
{
public:
	static uint32 NewRegisterBatch(CVec4Dfp32* &_pRegisters, uint32 &_nRegsNeeded)
	{
		return 468;
	}

};

void CRCPS3GCM::Attrib_Set(CRC_Attributes* _pAttrib)
{
	CRC_IDInfo* pIDInfo = m_spTCIDInfo->m_pTCIDInfo;
	for (int i = 0; i < CRC_MAXTEXTURES; ++i)
	{
		int TexID = _pAttrib->m_TextureID[i];
		if (TexID)
		{
			CRC_IDInfo &IDInfo = pIDInfo[TexID];
			if (!(IDInfo.m_Fresh & 1))
			{
				m_ContextTexture.Build(TexID);
			}
		}
	}

	int AttrChg = m_AttribChanged;
	m_ContextStatistics.m_nAttribSet++;
	m_AttribChanged = 0;
/*
	if(AttrChg & CRC_ATTRCHG_RASTERMODE)
	{
		if(ConvertRasterMode(_pAttrib))
		{
			AttrChg |= CRC_ATTRCHG_BLEND | CRC_ATTRCHG_FLAG_BLEND;
		}
	}
*/
	if(AttrChg & ~CRC_ATTRCHG_TEXTUREID)
	{
		uint32 ChangedFlags = (m_CurrentAttrib.m_Flags ^ _pAttrib->m_Flags);
		if(AttrChg & CRC_ATTRCHG_FLAGS)
//		if(ChangedFlags)
		{
			if(ChangedFlags & (CRC_FLAGS_COLORWRITE | CRC_FLAGS_ALPHAWRITE))
			{
				uint32 Mask = 0;
				if(_pAttrib->m_Flags & CRC_FLAGS_COLORWRITE)
					Mask |= CELL_GCM_COLOR_MASK_R | CELL_GCM_COLOR_MASK_G | CELL_GCM_COLOR_MASK_B;
				if(_pAttrib->m_Flags & CRC_FLAGS_ALPHAWRITE)
					Mask |= CELL_GCM_COLOR_MASK_A;
				gcmSetColorMask(Mask);
				if(CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_nRT > 1)
				{
					uint MRTMask = 0;
					uint AndMask = 0;
					if(_pAttrib->m_Flags & CRC_FLAGS_COLORWRITE)
						AndMask |= CELL_GCM_COLOR_MASK_MRT1_R | CELL_GCM_COLOR_MASK_MRT1_G | CELL_GCM_COLOR_MASK_MRT1_B | CELL_GCM_COLOR_MASK_MRT2_R | CELL_GCM_COLOR_MASK_MRT2_G | CELL_GCM_COLOR_MASK_MRT2_B | CELL_GCM_COLOR_MASK_MRT3_R | CELL_GCM_COLOR_MASK_MRT3_G | CELL_GCM_COLOR_MASK_MRT3_B;
					if(_pAttrib->m_Flags & CRC_FLAGS_ALPHAWRITE)
						AndMask |= CELL_GCM_COLOR_MASK_MRT1_A | CELL_GCM_COLOR_MASK_MRT2_A | CELL_GCM_COLOR_MASK_MRT3_A;
					if(CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_nRT > 1)
						MRTMask |= (CELL_GCM_COLOR_MASK_MRT1_R | CELL_GCM_COLOR_MASK_MRT1_G | CELL_GCM_COLOR_MASK_MRT1_B | CELL_GCM_COLOR_MASK_MRT1_A);
					if(CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_nRT > 2)
						MRTMask |= (CELL_GCM_COLOR_MASK_MRT2_R | CELL_GCM_COLOR_MASK_MRT2_G | CELL_GCM_COLOR_MASK_MRT2_B | CELL_GCM_COLOR_MASK_MRT2_A);
					if(CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_nRT > 3)
						MRTMask |= (CELL_GCM_COLOR_MASK_MRT3_R | CELL_GCM_COLOR_MASK_MRT3_G | CELL_GCM_COLOR_MASK_MRT3_B | CELL_GCM_COLOR_MASK_MRT3_A);
//					gcmSetColorMaskMrt(MRTMask & AndMask);	// JK-TODO: Fix mask for MRT properly
					gcmSetColorMaskMrt((~0) & AndMask);	// JK-TODO: Fix mask for MRT properly
				}
			}

			if(ChangedFlags & (CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE))
			{
				uint32 Mode = CELL_GCM_FALSE;
				if(_pAttrib->m_Flags & CRC_FLAGS_ZWRITE)
				{
					M_ASSERT(CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_DepthBuffer != CDCPS3GCM::CBackbufferContext::CSetup::BUFFER_UNUSED, "!");
					Mode = CELL_GCM_TRUE;

					if(!(_pAttrib->m_Flags & CRC_FLAGS_ZCOMPARE))
					{
						// No Z-Compare but Z-Write enabled is converted into an Always Z-Compare
						AttrChg |= CRC_ATTRCHG_ZCOMPARE;
						_pAttrib->m_Flags |= CRC_FLAGS_ZCOMPARE;
						_pAttrib->m_ZCompare = CRC_COMPARE_ALWAYS;
					}
				}
				if(CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_DepthBuffer != CDCPS3GCM::CBackbufferContext::CSetup::BUFFER_UNUSED)
				{
					// Wonder if this is safe or if it will causes renderstate hickups?
					gcmSetDepthMask(Mode);
					AttrChg |= CRC_ATTRCHG_ZCOMPARE;
				}
			}

			if(ChangedFlags & CRC_FLAGS_BLEND)
			{
				uint32 Mode = CELL_GCM_FALSE;
				if(_pAttrib->m_Flags & CRC_FLAGS_BLEND)
					Mode = CELL_GCM_TRUE;
				gcmSetBlendEnable(Mode);
				if(CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_nRT > 1)
					gcmSetBlendEnableMrt(Mode, Mode, Mode);	// JK-TODO: Fix blending for MRT properly
			}

			if(ChangedFlags & CRC_FLAGS_SMOOTH)
			{
				uint32 mode = CELL_GCM_FLAT;
				if(_pAttrib->m_Flags & CRC_FLAGS_SMOOTH)
					mode = CELL_GCM_SMOOTH;
				gcmSetShadeMode(mode);
			}

			if(ChangedFlags & CRC_FLAGS_CULL)
			{
				uint32 enable = CELL_GCM_FALSE;
				if(_pAttrib->m_Flags & CRC_FLAGS_CULL)
					enable = CELL_GCM_TRUE;
				gcmSetCullFaceEnable(enable);
			}

			if(ChangedFlags & CRC_FLAGS_CULLCW)
			{
				uint32 cf = CELL_GCM_FRONT;
				if(_pAttrib->m_Flags & CRC_FLAGS_CULLCW)
					cf = CELL_GCM_BACK;
				gcmSetCullFace(cf);
			}

			if(ChangedFlags & CRC_FLAGS_STENCIL)
			{
				uint32 enable = CELL_GCM_FALSE;
				if(_pAttrib->m_Flags & CRC_FLAGS_STENCIL)
					enable = CELL_GCM_TRUE;
				gcmSetStencilTestEnable(enable);
			}

			if(ChangedFlags & CRC_FLAGS_SEPARATESTENCIL)
			{
				uint32 enable = CELL_GCM_FALSE;
				if(_pAttrib->m_Flags & CRC_FLAGS_SEPARATESTENCIL)
					enable = CELL_GCM_TRUE;
				gcmSetTwoSidedStencilTestEnable(enable);
			}

			if(ChangedFlags & CRC_FLAGS_POLYGONOFFSET)
			{
				uint32 enable = CELL_GCM_FALSE;
				if(_pAttrib->m_Flags & CRC_FLAGS_POLYGONOFFSET)
					enable = CELL_GCM_TRUE;
				gcmSetPolygonOffsetFillEnable(enable);
			}

#ifdef CRC_SUPPORTVERTEXLIGHTING
			if(ChangedFlags & CRC_FLAGS_LIGHTING)
			{
				//JK-TODO: Implement this
				LogFile("Implement vertex lighting");
			}
#endif

			if(ChangedFlags & CRC_FLAGS_SCISSOR)
			{
				AttrChg |= CRC_ATTRCHG_SCISSOR;

				// If scissoring is disabled then set a fullscreen scissor
				if(!(_pAttrib->m_Flags & CRC_FLAGS_SCISSOR))
					_pAttrib->m_Scissor.SetRect(0, 0, 4095, 4095);
			}
		}


		if(AttrChg & CRC_ATTRCHG_POLYGONOFFSET)
		{
//			if(memcmp(&m_CurrentAttrib.m_PolygonOffsetScale, &_pAttrib->m_PolygonOffsetScale, sizeof(m_CurrentAttrib.m_PolygonOffsetScale) + sizeof(m_CurrentAttrib.m_PolygonOffsetUnits)))
			{
				gcmSetPolygonOffset(_pAttrib->m_PolygonOffsetScale, _pAttrib->m_PolygonOffsetUnits);
			}
		}

#ifndef GCM_COMMANDBUFFERWRITE
		if(AttrChg & CRC_ATTRCHG_BLEND)
		{
			//			if((m_CurrentAttrib.m_SourceBlend != _pAttrib->m_SourceBlend) || (m_CurrentAttrib.m_DestBlend != _pAttrib->m_DestBlend))
			{
				uint32 SourceBlend = ConvertToGCMBlend(_pAttrib->m_SourceBlend);
				uint32 DestBlend = ConvertToGCMBlend(_pAttrib->m_DestBlend);
				gcmSetBlendFunc(SourceBlend, DestBlend, SourceBlend, DestBlend);
			}
		}
		if(AttrChg & CRC_ATTRCHG_ZCOMPARE)
		{
			// If Z-Compare is disabled but Z-Write is enabled then we cannot disable depthtesting so we have to use ALWAYS instead
//			gcmSetZcullControl(ConvertToZCullCompare(_pAttrib->m_ZCompare), CELL_GCM_ZCULL_LONES);
			if(_pAttrib->m_Flags & CRC_FLAGS_ZCOMPARE)
			{
				gcmSetDepthFunc(ConvertToGCMCompare(_pAttrib->m_ZCompare));
				gcmSetDepthTestEnable(CELL_GCM_TRUE);
			}
			else
			{
				if(_pAttrib->m_Flags & CRC_FLAGS_ZWRITE)
				{
					gcmSetDepthFunc(CELL_GCM_ALWAYS);
					gcmSetDepthTestEnable(CELL_GCM_TRUE);
					_pAttrib->m_ZCompare = CRC_COMPARE_ALWAYS;
					m_CurrentAttrib.m_ZCompare = CRC_COMPARE_ALWAYS;
				}
				else
				{
					gcmSetDepthTestEnable(CELL_GCM_FALSE);
				}
			}
		}
		if(AttrChg & CRC_ATTRCHG_ALPHACOMPARE)
		{
			{
				uint32 Compare = ConvertToGCMCompare(_pAttrib->m_AlphaCompare);
				if(CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_BackbufferFormat == CELL_GCM_SURFACE_F_W16Z16Y16X16)
				{
//					gcmSetAlphaFunc(Compare, _pAttrib->m_AlphaRef << 8);
					gcmSetAlphaFunc(Compare, FloatToGfp16(_pAttrib->m_AlphaRef * (1.0f / 255.0f)));
				}
				else
					gcmSetAlphaFunc(Compare, _pAttrib->m_AlphaRef);
				uint32 enable = CELL_GCM_TRUE;
				if(Compare == CELL_GCM_ALWAYS)
					enable = CELL_GCM_FALSE;
				gcmSetAlphaTestEnable(enable);
			}
		}
		if(AttrChg & CRC_ATTRCHG_STENCIL)
		{
			gcmSetStencilFunc(ConvertToGCMCompare(_pAttrib->m_StencilFrontFunc), _pAttrib->m_StencilRef, _pAttrib->m_StencilFuncAnd);
			gcmSetStencilMask(_pAttrib->m_StencilWriteMask);
			gcmSetStencilOp(ConvertToGCMOp(_pAttrib->m_StencilFrontOpFail), ConvertToGCMOp(_pAttrib->m_StencilFrontOpZFail), ConvertToGCMOp(_pAttrib->m_StencilFrontOpZPass));
			gcmSetBackStencilFunc(ConvertToGCMCompare(_pAttrib->m_StencilBackFunc), _pAttrib->m_StencilRef, _pAttrib->m_StencilFuncAnd);
			gcmSetBackStencilMask(_pAttrib->m_StencilWriteMask);
			gcmSetBackStencilOp(ConvertToGCMOp(_pAttrib->m_StencilBackOpFail), ConvertToGCMOp(_pAttrib->m_StencilBackOpZFail), ConvertToGCMOp(_pAttrib->m_StencilBackOpZPass));
		}

		if(AttrChg & CRC_ATTRCHG_SCISSOR)
		{
			uint32 MinX = 0, MinY = 0, MaxX = 4095, MaxY = 4095;
			if(_pAttrib->m_Scissor.IsValid())
			{
				_pAttrib->m_Scissor.GetRect(MinX, MinY, MaxX, MaxY);
				MaxX &= 0xfff;
				MaxY &= 0xfff;
			}
			uint w = MaxX - MinX;
			uint h = MaxY - MinY;
			gcmSetScissor(MinX, MinY, w, h);
		}
#endif
	}
#ifndef GCM_COMMANDBUFFERWRITE
	if(AttrChg & CRC_ATTRCHG_TEXTUREID)
	{
		for(uint i = 0; i < CRC_MAXTEXTURES; i++)
		{
			uint32 TexID = _pAttrib->m_TextureID[i];
			//uint32 TexID = _pAttrib->m_TextureID[0];
			if(m_CurrentAttrib.m_TextureID[i] != TexID)
			{
				m_CurrentAttrib.m_TextureID[i] = TexID;
				m_ContextStatistics.m_nTextureSet++;
				m_ContextTexture.Bind(i, TexID);
			}
		}
	}
#endif

#ifdef GCM_COMMANDBUFFERWRITE
	uint32 M_RESTRICT* pCmdBegin = RSX_BeginCommand(3 + 4 + 5 + 2*(4+4+2) + 3 + 16*(8*2));
	uint32 M_RESTRICT* pCmd = pCmdBegin;

	if(AttrChg & ~CRC_ATTRCHG_TEXTUREID)
	{
		if(AttrChg & CRC_ATTRCHG_BLEND)
		{
			// nCmd = 3
			uint32 SourceBlend = ConvertToGCMBlend(_pAttrib->m_SourceBlend);
			uint32 DestBlend = ConvertToGCMBlend(_pAttrib->m_DestBlend);
			RSX_BlendFunc(pCmd, SourceBlend, DestBlend, SourceBlend, DestBlend);
		}

		if(AttrChg & CRC_ATTRCHG_ZCOMPARE)
		{
			// nCmd = 4
			// If Z-Compare is disabled but Z-Write is enabled then we cannot disable depthtesting so we have to use ALWAYS instead
			if(_pAttrib->m_Flags & CRC_FLAGS_ZCOMPARE)
			{
				RSX_DepthFunc(pCmd, ConvertToGCMCompare(_pAttrib->m_ZCompare));
				RSX_DepthTestEnable(pCmd, CELL_GCM_TRUE);
			}
			else
			{
				if(_pAttrib->m_Flags & CRC_FLAGS_ZWRITE)
				{
					RSX_DepthFunc(pCmd, CELL_GCM_ALWAYS);
					RSX_DepthTestEnable(pCmd, CELL_GCM_TRUE);
					_pAttrib->m_ZCompare = CRC_COMPARE_ALWAYS;
					m_CurrentAttrib.m_ZCompare = CRC_COMPARE_ALWAYS;
				}
				else
				{
					RSX_DepthTestEnable(pCmd, CELL_GCM_FALSE);
				}
			}
		}
		if(AttrChg & CRC_ATTRCHG_ALPHACOMPARE)
		{
			// nCmd = 5
			uint32 Compare = ConvertToGCMCompare(_pAttrib->m_AlphaCompare);
			RSX_AlphaFunc(pCmd, Compare, _pAttrib->m_AlphaRef);
			uint32 bEnable = CELL_GCM_TRUE;
			if(Compare == CELL_GCM_ALWAYS)
				bEnable = CELL_GCM_FALSE;
			RSX_AlphaTestEnable(pCmd, bEnable);
		}
		if(AttrChg & CRC_ATTRCHG_STENCIL)
		{
			// nCmd = 2*(4+4+2)
			RSX_StencilFunc(pCmd, ConvertToGCMCompare(_pAttrib->m_StencilFrontFunc), _pAttrib->m_StencilRef, _pAttrib->m_StencilFuncAnd);
			RSX_StencilMask(pCmd, _pAttrib->m_StencilWriteMask);
			RSX_StencilOp(pCmd, ConvertToGCMOp(_pAttrib->m_StencilFrontOpFail), ConvertToGCMOp(_pAttrib->m_StencilFrontOpZFail), ConvertToGCMOp(_pAttrib->m_StencilFrontOpZPass));
			RSX_BackStencilFunc(pCmd, ConvertToGCMCompare(_pAttrib->m_StencilBackFunc), _pAttrib->m_StencilRef, _pAttrib->m_StencilFuncAnd);
			RSX_BackStencilMask(pCmd, _pAttrib->m_StencilWriteMask);
			RSX_BackStencilOp(pCmd, ConvertToGCMOp(_pAttrib->m_StencilBackOpFail), ConvertToGCMOp(_pAttrib->m_StencilBackOpZFail), ConvertToGCMOp(_pAttrib->m_StencilBackOpZPass));
		}
		if(AttrChg & CRC_ATTRCHG_SCISSOR)
		{
			// nCmd = 3
			if(_pAttrib->m_Flags & CRC_FLAGS_SCISSOR)
				RSX_Scissor(pCmd, _pAttrib->m_Scissor);
			else
				RSX_ScissorDisable(pCmd);
		}
	}

	if(AttrChg & CRC_ATTRCHG_TEXTUREID)
	{
		uint nBind = 0;
		for(uint i = 0; i < CRC_MAXTEXTURES; i++)
		{
			uint32 TexID = _pAttrib->m_TextureID[i];
			//uint32 TexID = _pAttrib->m_TextureID[0];
			if(m_CurrentAttrib.m_TextureID[i] != TexID)
			{
				m_CurrentAttrib.m_TextureID[i] = TexID;
				m_ContextStatistics.m_nTextureSet++;
				pCmd = m_ContextTexture.RSX_Bind(pCmd, i, TexID);
				nBind++;
			}
		}

		CRCPS3GCM::ms_This.m_ContextStatistics.m_nBindTexture += nBind;
	}
	RSX_EndCommand(pCmd - pCmdBegin);
#endif


	Attrib_SetVPPipeline(_pAttrib, false);
	Attrib_SetFPPipeline(_pAttrib);

	uint nPlanes = m_lClipStack[m_iClipStack].m_nPlanes;
	if(nPlanes != m_nCurrentClipPlanes)
	{
		const ClipSetting* M_RESTRICT pClip = &ClipSettings[nPlanes];
		uint Clip0 = pClip->m_Clip0;
		uint Clip1 = pClip->m_Clip1;
		uint Clip2 = pClip->m_Clip2;
		uint Clip3 = pClip->m_Clip3;
		uint Clip4 = pClip->m_Clip4;
		uint Clip5 = pClip->m_Clip5;
		gcmSetUserClipPlaneControl(Clip0, Clip1, Clip2, Clip3, Clip4, Clip5);
		m_nCurrentClipPlanes = nPlanes;
	}

	memcpy(&m_CurrentAttrib, _pAttrib, sizeof(CRC_Attributes));
}

void CRCPS3GCM::Attrib_SetAbsolute(CRC_Attributes* _pAttrib)
{
	for (int i = 0; i < CRC_MAXTEXTURES; ++i)
	{
		int TexID = _pAttrib->m_TextureID[i];
		if (TexID)
		{
			m_ContextTexture.Build(TexID);
		}
	}

	m_ContextStatistics.m_nAttribSet++;

//	ConvertRasterMode(_pAttrib);
	{
		{
			{
				uint32 Mask = 0;
				if(_pAttrib->m_Flags & CRC_FLAGS_COLORWRITE)
					Mask |= CELL_GCM_COLOR_MASK_R | CELL_GCM_COLOR_MASK_G | CELL_GCM_COLOR_MASK_B;
				if(_pAttrib->m_Flags & CRC_FLAGS_ALPHAWRITE)
					Mask |= CELL_GCM_COLOR_MASK_A;
				gcmSetColorMask(Mask);
				if(CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_nRT > 1)
				{
					uint MRTMask = 0;
					uint AndMask = 0;
					if(_pAttrib->m_Flags & CRC_FLAGS_COLORWRITE)
						AndMask |= CELL_GCM_COLOR_MASK_MRT1_R | CELL_GCM_COLOR_MASK_MRT1_G | CELL_GCM_COLOR_MASK_MRT1_B | CELL_GCM_COLOR_MASK_MRT2_R | CELL_GCM_COLOR_MASK_MRT2_G | CELL_GCM_COLOR_MASK_MRT2_B | CELL_GCM_COLOR_MASK_MRT3_R | CELL_GCM_COLOR_MASK_MRT3_G | CELL_GCM_COLOR_MASK_MRT3_B;
					if(_pAttrib->m_Flags & CRC_FLAGS_ALPHAWRITE)
						AndMask |= CELL_GCM_COLOR_MASK_MRT1_A | CELL_GCM_COLOR_MASK_MRT2_A | CELL_GCM_COLOR_MASK_MRT3_A;
					if(CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_nRT > 1)
						MRTMask |= (CELL_GCM_COLOR_MASK_MRT1_R | CELL_GCM_COLOR_MASK_MRT1_G | CELL_GCM_COLOR_MASK_MRT1_B | CELL_GCM_COLOR_MASK_MRT1_A);
					if(CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_nRT > 2)
						MRTMask |= (CELL_GCM_COLOR_MASK_MRT2_R | CELL_GCM_COLOR_MASK_MRT2_G | CELL_GCM_COLOR_MASK_MRT2_B | CELL_GCM_COLOR_MASK_MRT2_A);
					if(CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_nRT > 3)
						MRTMask |= (CELL_GCM_COLOR_MASK_MRT3_R | CELL_GCM_COLOR_MASK_MRT3_G | CELL_GCM_COLOR_MASK_MRT3_B | CELL_GCM_COLOR_MASK_MRT3_A);
//					gcmSetColorMaskMrt(MRTMask & AndMask);	// JK-TODO: Fix mask for MRT properly
					gcmSetColorMaskMrt((~0) & AndMask);	// JK-TODO: Fix mask for MRT properly
				}
			}

			{
				uint32 Mode = CELL_GCM_FALSE;
				if(_pAttrib->m_Flags & CRC_FLAGS_ZWRITE)
				{
					M_ASSERT(CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_DepthBuffer != CDCPS3GCM::CBackbufferContext::CSetup::BUFFER_UNUSED, "!");
					Mode = CELL_GCM_TRUE;

					if(!(_pAttrib->m_Flags & CRC_FLAGS_ZCOMPARE))
					{
						// No Z-Compare but Z-Write enabled is converted into an Always Z-Compare
						_pAttrib->m_Flags |= CRC_FLAGS_ZCOMPARE;
						_pAttrib->m_ZCompare = CRC_COMPARE_ALWAYS;
					}
				}
				if(CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_DepthBuffer != CDCPS3GCM::CBackbufferContext::CSetup::BUFFER_UNUSED)
				{
					// Wonder if this is safe or if it will causes renderstate hickups?
					gcmSetDepthMask(Mode);
				}

				// If Z-Compare is disabled but Z-Write is enabled then we cannot disable depthtesting so we have to use ALWAYS instead
				if(_pAttrib->m_Flags & CRC_FLAGS_ZCOMPARE)
				{
					gcmSetDepthTestEnable(CELL_GCM_TRUE);
				}
				else
				{
					gcmSetDepthTestEnable(CELL_GCM_FALSE);
				}
			}

			{
				uint32 Mode = CELL_GCM_FALSE;
				if(_pAttrib->m_Flags & CRC_FLAGS_BLEND)
					Mode = CELL_GCM_TRUE;
				gcmSetBlendEnable(Mode);
				if(CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_nRT > 1)
					gcmSetBlendEnableMrt(Mode, Mode, Mode);	// JK-TODO: Fix blending for MRT properly
			}

			{
				uint32 mode = CELL_GCM_FLAT;
				if(_pAttrib->m_Flags & CRC_FLAGS_SMOOTH)
					mode = CELL_GCM_SMOOTH;
				gcmSetShadeMode(mode);
			}

			{
				uint32 enable = CELL_GCM_FALSE;
				if(_pAttrib->m_Flags & CRC_FLAGS_CULL)
					enable = CELL_GCM_TRUE;
				gcmSetCullFaceEnable(enable);
			}

			{
				uint32 cf = CELL_GCM_FRONT;
				if(_pAttrib->m_Flags & CRC_FLAGS_CULLCW)
					cf = CELL_GCM_BACK;
				gcmSetCullFace(cf);
			}

			{
				uint32 enable = CELL_GCM_FALSE;
				if(_pAttrib->m_Flags & CRC_FLAGS_STENCIL)
					enable = CELL_GCM_TRUE;
				gcmSetStencilTestEnable(enable);
			}

			{
				uint32 enable = CELL_GCM_FALSE;
				if(_pAttrib->m_Flags & CRC_FLAGS_SEPARATESTENCIL)
					enable = CELL_GCM_TRUE;
				gcmSetTwoSidedStencilTestEnable(enable);
			}

			{
				//JK-TODO: Implement this
				uint32 enable = CELL_GCM_FALSE;
				if(_pAttrib->m_Flags & CRC_FLAGS_POLYGONOFFSET)
					enable = CELL_GCM_TRUE;
				gcmSetPolygonOffsetFillEnable(enable);
			}

			{
				if(!(_pAttrib->m_Flags & CRC_FLAGS_SCISSOR))
					_pAttrib->m_Scissor.SetRect(0, 0, 4095, 4095);
			}
		}

		{
			{
				uint32 SourceBlend = ConvertToGCMBlend(_pAttrib->m_SourceBlend);
				uint32 DestBlend = ConvertToGCMBlend(_pAttrib->m_DestBlend);
				gcmSetBlendFunc(SourceBlend, DestBlend, SourceBlend, DestBlend);
			}
		}

		{
//			gcmSetZcullControl(ConvertToZCullCompare(_pAttrib->m_ZCompare), CELL_GCM_ZCULL_LONES);
			gcmSetDepthFunc(ConvertToGCMCompare(_pAttrib->m_ZCompare));
		}

		{
			{
				uint32 Compare = ConvertToGCMCompare(_pAttrib->m_AlphaCompare);
				if(CDCPS3GCM::ms_This.m_CurrentBackbufferContext.m_Setup.m_BackbufferFormat == CELL_GCM_SURFACE_F_W16Z16Y16X16)
				{
//					gcmSetAlphaFunc(Compare, _pAttrib->m_AlphaRef << 8);
					gcmSetAlphaFunc(Compare, FloatToGfp16(_pAttrib->m_AlphaRef * (1.0f / 255.0f)));
				}
				else
					gcmSetAlphaFunc(Compare, _pAttrib->m_AlphaRef);
				uint32 enable = CELL_GCM_TRUE;
				if(Compare == CELL_GCM_ALWAYS)
					enable = CELL_GCM_FALSE;
				gcmSetAlphaTestEnable(enable);
			}
		}

		{
			{
				gcmSetPolygonOffset(_pAttrib->m_PolygonOffsetScale, _pAttrib->m_PolygonOffsetUnits);
			}
		}

		{
			gcmSetStencilFunc(ConvertToGCMCompare(_pAttrib->m_StencilFrontFunc), _pAttrib->m_StencilRef, _pAttrib->m_StencilFuncAnd);
			gcmSetStencilMask(_pAttrib->m_StencilWriteMask);
			gcmSetStencilOp(ConvertToGCMOp(_pAttrib->m_StencilFrontOpFail), ConvertToGCMOp(_pAttrib->m_StencilFrontOpZFail), ConvertToGCMOp(_pAttrib->m_StencilFrontOpZPass));
			gcmSetBackStencilFunc(ConvertToGCMCompare(_pAttrib->m_StencilBackFunc), _pAttrib->m_StencilRef, _pAttrib->m_StencilFuncAnd);
			gcmSetBackStencilMask(_pAttrib->m_StencilWriteMask);
			gcmSetBackStencilOp(ConvertToGCMOp(_pAttrib->m_StencilBackOpFail), ConvertToGCMOp(_pAttrib->m_StencilBackOpZFail), ConvertToGCMOp(_pAttrib->m_StencilBackOpZPass));
		}

		{
			uint32 MinX = 0, MinY = 0, MaxX = 4095, MaxY = 4095;
			if(_pAttrib->m_Scissor.IsValid())
			{
				_pAttrib->m_Scissor.GetRect(MinX, MinY, MaxX, MaxY);
			}
			MaxX = Min(MaxX, (uint32)4095);
			MaxY = Min(MaxY, (uint32)4095);
			uint w = MaxX - MinX;
			uint h = MaxY - MinY;
			gcmSetScissor(MinX, MinY, w, h);
		}
	}

	{
		for(uint i = 0; i < CRC_MAXTEXTURES; i++)
		{
			uint32 TexID = _pAttrib->m_TextureID[i];
			{
//				MTRACE_SCOPECOMMANDS_ALWAYS("BindAbs");
				m_ContextStatistics.m_nTextureSet++;
				m_ContextTexture.Bind(i, TexID);
			}
		}
	}



	Attrib_SetVPPipeline(_pAttrib, true);
	Attrib_SetFPPipeline(_pAttrib);

	uint nPlanes = m_lClipStack[m_iClipStack].m_nPlanes;
	{
		const ClipSetting* M_RESTRICT pClip = &ClipSettings[nPlanes];
		uint Clip0 = pClip->m_Clip0;
		uint Clip1 = pClip->m_Clip1;
		uint Clip2 = pClip->m_Clip2;
		uint Clip3 = pClip->m_Clip3;
		uint Clip4 = pClip->m_Clip4;
		uint Clip5 = pClip->m_Clip5;
		gcmSetUserClipPlaneControl(Clip0, Clip1, Clip2, Clip3, Clip4, Clip5);
		m_nCurrentClipPlanes = nPlanes;
	}

	memcpy(&m_CurrentAttrib, _pAttrib, sizeof(CRC_Attributes));
}

void CRCPS3GCM::Attrib_SetFPPipeline(CRC_Attributes* _pAttrib)
{
	CRC_ExtAttributes* pExtAttrib = _pAttrib->m_pExtAttrib;
	if(!pExtAttrib)
	{
		static CRC_ExtAttributes_FragmentProgram30 ExtAttrib;
//		static CVec4Dfp32 TexEnvParams[CRC_MAXTEXTUREENV];
		static CVec4Dfp32 TexEnvParams[6];

		int i = 0;
		if(_pAttrib->m_TexEnvMode[0] & CRC_TEXENVMODE_RGB_SOURCE1_ALPHA)
		{
			// don't like this one
			i = 9;
		}
		else
		{
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
						M_TRACE("Unsupported texenvmode: %d\n", _pAttrib->m_TexEnvMode[i]);
	#endif
						break;
					}
				}
			}
		}

		
		ExtAttrib.Clear();
		ExtAttrib.SetProgram(gs_TexEnvPrograms[i], gs_TexEnvProgramHash[i]);
//		ExtAttrib.SetParameters(TexEnvParams, i);
		ExtAttrib.SetParameters(TexEnvParams, 6);
		pExtAttrib = &ExtAttrib;
	}

	switch(pExtAttrib->m_AttribType)
	{
	case CRC_ATTRIBTYPE_FP30:
	case CRC_ATTRIBTYPE_FP20:
		{
			CRC_ExtAttributes_FragmentProgram20* pFP20 = (CRC_ExtAttributes_FragmentProgram20*)pExtAttrib;
			m_ContextFP.Bind(pFP20);

#ifdef GCM_USEFPPT
			if(pFP20->m_nParams > 0)
			{
				m_iFPPTLine++;
				if (m_iFPPTLine >= GCM_FPPT_HEIGHT)
				{
					m_iFPPTTex++;
					if (m_iFPPTTex >= GCM_FPPT_NUM)
						m_iFPPTTex = 0;
					m_iFPPTLine = 0;
					m_lFPPT[m_iFPPTTex].Bind(GCM_FPPT_SAMPLER);
				}
				vec128 M_RESTRICT* pMem = (vec128*) m_lFPPT[m_iFPPTTex].m_ResourceMain.GetMemory();
				pMem += (m_iFPPTLine * GCM_FPPT_WIDTH);

				uint nP = pFP20->m_nParams;
				CVec4Dfp32* pP = pFP20->m_pParams;
				for(uint i = 0; i < nP; i++)
					pMem[i] = pP[i].v;

				fp32 ParamUV[4]; ParamUV[0] = 0.0f; ParamUV[1] = fp32(m_iFPPTLine) * (1.0f / fp32(GCM_FPPT_HEIGHT)); ParamUV[2] = 0;ParamUV[3] = 0;
				gcmSetVertexProgramConstants(255, 1 * 4, ParamUV);

//				for(uint j = 0; j < 1; j++)
//					pMem[j] = M_VLd(Random+0.5f, Random+0.25f, Random, Random);
//				cellGcmSetInvalidateTextureCache(CELL_GCM_INVALIDATE_TEXTURE);
			}
#endif
			break;
		}

	default:
		{
			Error_static("CRenderContextPS3::Attrib_SetPixelProgram", "Unsupported attribtype");
		}
	}
}

void CRCPS3GCM::Attrib_SetVPPipeline(CRC_Attributes* _pAttrib, bool _bUpdateAllConstants)
{
	CRC_MatrixState& MS = Matrix_GetState();

	// Get texture matrices
	const CMat4Dfp32* lpTexMat[CRC_MAXTEXCOORDS];
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

	CVec4Dfp32 aRegisters[468];
	CRC_VPFormat VPFormat;
	VPFormat.Create(0);
	VPFormat.SetAttrib(_pAttrib, lpTexMat);
#ifdef CRC_SUPPORTVERTEXLIGHTING
	if (_pAttrib->m_Flags & CRC_FLAGS_LIGHTING)
	{
		VPFormat.SetLights(_pAttrib->m_pLights, _pAttrib->m_nLights);
	}
#endif

	if (Clip_IsEnabled())
	{
		VPFormat.SetClipPlanes(true);
	}

#ifdef CRC_SUPPORTCUBEVP
	if (MS.m_pMatrixPaletteArgs)
	{
		VPFormat.SetMPFlags( MS.m_pMatrixPaletteArgs->m_Flags & ERC_MatrixPaletteFlags_SpecialCubeFlags );
	}
#endif

	if (MS.m_pMatrixPaletteArgs)
	{
//		VPFormat.SetMWComp((_pAttrib->m_VBFlags & VBF_DUAL_MATRIX_LISTS) ? 8 : 4);
		VPFormat.SetMWComp(m_ContextGeometry.GetMWComp());
	}

	{
		for(int t = 0; t < CRC_MAXTEXCOORDS; t++)
		{
			if (lpTexMat[t]) 
				VPFormat.SetTexMatrix(t);
		}
	}
	// Texcoord renaming
	uint32 Rename = _pAttrib->m_iTexCoordSet[0]
				  | (_pAttrib->m_iTexCoordSet[1] << 3)
				  | (_pAttrib->m_iTexCoordSet[2] << 6)
				  | (_pAttrib->m_iTexCoordSet[3] << 9)
				  | (_pAttrib->m_iTexCoordSet[4] << 12)
				  | (_pAttrib->m_iTexCoordSet[5] << 15)
				  | (_pAttrib->m_iTexCoordSet[6] << 18)
				  | (_pAttrib->m_iTexCoordSet[7] << 21);
	VPFormat.SetTexRenameFull(Rename);
	VPFormat.SetTransformFull();
	int iRegFirst = CRC_VPFormat::GetFirstFreeRegister();
	int iReg = iRegFirst;
	{
		if(_bUpdateAllConstants)
		{
			iReg += VPFormat.SetRegisters_Init(&(aRegisters[iReg]), iReg);
			iReg += VPFormat.SetRegisters_ConstantColor(&(aRegisters[iReg]), iReg, m_GeomColor);		// Must be fixed at base 10
			iReg += VPFormat.SetRegisters_TextureTransformFull(&(aRegisters[iReg]), iReg, Rename, m_ContextGeometry.m_lScalers);							// Must be fixed at base 11
		}
		else
		{
			VPFormat.m_iConstant_ConstantColor = iReg + 2;
			VPFormat.m_iConstant_PosTransform = iReg + 3;
			VPFormat.m_iConstant_TexTransform[0] = iReg + 5 + (((Rename >> 0) & 7) << 1);
			VPFormat.m_iConstant_TexTransform[1] = iReg + 5 + (((Rename >> 3) & 7) << 1);
			VPFormat.m_iConstant_TexTransform[2] = iReg + 5 + (((Rename >> 6) & 7) << 1);
			VPFormat.m_iConstant_TexTransform[3] = iReg + 5 + (((Rename >> 9) & 7) << 1);
			VPFormat.m_iConstant_TexTransform[4] = iReg + 5 + (((Rename >> 12) & 7) << 1);
			VPFormat.m_iConstant_TexTransform[5] = iReg + 5 + (((Rename >> 15) & 7) << 1);
			VPFormat.m_iConstant_TexTransform[6] = iReg + 5 + (((Rename >> 18) & 7) << 1);
			VPFormat.m_iConstant_TexTransform[7] = iReg + 5 + (((Rename >> 21) & 7) << 1);
			iReg = 29;
			iRegFirst = 29;
		}

		if (Clip_IsEnabled())
		{
			CRC_ClipStackEntry& Clip = m_lClipStack[m_iClipStack];
			CMat4Dfp32 VMatInv;
			MS.m_lMatrices[CRC_MATRIX_MODEL].InverseOrthogonal(VMatInv);

			CPlane3Dfp32 Planes[6];
			int nP = Clip.m_nPlanes;
			memcpy(&Planes, Clip.m_Planes, sizeof(CPlane3Dfp32) * nP);
			for(int i = 0; i < nP; i++)
			{
				Planes[i].Transform(VMatInv);
			}

			iReg += VPFormat.SetRegisters_ClipPlanes<CGetNewRegsEmpty>(&(aRegisters[iReg]), iReg, (CPlane3Dfp32*)Planes, nP);
		}

//		iReg += VPFormat.SetRegisters_ConstantColor(&(aRegisters[iReg]), iReg, 0xffffffff);
		iReg += VPFormat.SetRegisters_Attrib(&(aRegisters[iReg]), iReg, _pAttrib, this);
		CVec4Dfp32* pRegisters = &(aRegisters[iReg]);
		iReg += VPFormat.SetRegisters_TexGenMatrix<CGetNewRegsEmpty>(pRegisters, iReg, lpTexMat, _pAttrib);
//		m_VP_iConstantColor = VPFormat.m_iConstant_ConstantColor;
//		for(int iTexMat = 0; iTexMat < CRC_MAXTEXCOORDS; iTexMat++)
//			m_VP_liTexMatrixReg[iTexMat] = VPFormat.m_iConstant_TexMatrix[iTexMat];

#ifdef CRC_SUPPORTVERTEXLIGHTING
		if (_pAttrib->m_Flags & CRC_FLAGS_LIGHTING)
		{
			CVec4Dfp32* pRegisters = &(aRegisters[iReg]);
			iReg += VPFormat.SetRegisters_Lights<CGetNewRegsEmpty>(pRegisters, iReg, _pAttrib->m_pLights, _pAttrib->m_nLights);
		}
#endif
		pRegisters = &(aRegisters[iReg]);
		
		if(MS.m_pMatrixPaletteArgs != NULL)
		{
#ifdef CRC_SUPPORTCUBEVP
			if(MS.m_pMatrixPaletteArgs->m_Flags & ERC_MatrixPaletteFlags_SpecialCubeFlags)
				iReg += VPFormat.SetRegisters_MatrixPaletteSpecialCube<CGetNewRegsEmpty>(pRegisters, iReg, CContext_VertexProgram::EMaxConstants - iReg, MS.m_pMatrixPaletteArgs);
			else
#endif
				iReg += VPFormat.SetRegisters_MatrixPalette<CGetNewRegsEmpty>(pRegisters, iReg, CContext_VertexProgram::EMaxConstants - iReg, MS.m_pMatrixPaletteArgs);
		}

		M_ASSERT(iReg < 468, "Too many constants for vertex program");
	}

	m_ContextVP.Bind(VPFormat);
	if(iReg > iRegFirst)
		m_ContextVP.SetConstants(iRegFirst, iReg - iRegFirst, aRegisters + iRegFirst);

//	VP_Bind(VPFormat);
//	GLErr("VP_Bind(1)");
//	m_nUpdateVPConst = iReg;
//	m_bUpdateVPConst = true;
}

void CRCPS3GCM::Attrib_GlobalUpdate()
{
	MSCOPE(CRCPS3GCM::Attrib_GlobalUpdate, RENDER_PS3);
}

void CRCPS3GCM::Attrib_DeferredGlobalUpdate()
{
	MSCOPE(CRCPS3GCM::Attrib_DeferredGlobalUpdate, RENDER_PS3);
}

int CRCPS3GCM::Attrib_GlobalGetVar(int _Var)
{
	switch(_Var)
	{
	case /*CRC_GLOBALVAR_TOTALTRIANGLES*/ 20 : return m_nTriangles;
	default : return CRC_Core::Attrib_GlobalGetVar(_Var);
	}
}

//----------------------------------------------------------------
// Transform overrides
//----------------------------------------------------------------
void CRCPS3GCM::Matrix_SetRender(int _iMode, const CMat4Dfp32* _pMatrix)
{
	MSCOPE(CRCPS3GCM::Matrix_SetRender, RENDER_PS3);

	if (_iMode == CRC_MATRIX_MODEL)
	{
		{
			if (_pMatrix)
			{
				vec128 r0,r1,r2,r3;
				r0 = _pMatrix->r[0];
				r1 = _pMatrix->r[1];
				r2 = _pMatrix->r[2];
				r3 = _pMatrix->r[3];
				vec128 Translate = M_VDp3x3(r0, r3, r1, r3, r2, r3);
				M_VTranspose4x4(r0, r1, r2, r3);
				vec128 dp = M_VDp3x3(r0, r0, r1, r1, r2, r2);

				// All rows normalized?
				vec128 isone = M_VCmpLTMsk(M_VAbs(M_VSub(dp, M_VOne())), M_VScalar(0.000001f));
				isone = M_VAnd(M_VAnd(M_VSplatX(isone), M_VSplatY(isone)), M_VSplatZ(isone));

				vec128 dprsq = M_VRcp(M_VSelMsk(isone, dp, M_VOne()));

				Translate = M_VAnd(M_VConstMsk(1,1,1,0), M_VMul(dprsq, Translate));
				Translate = M_VSelMsk(isone, Translate, M_VZero());

				r3 = _pMatrix->r[3];
				r3 = M_VSelMsk(isone, M_VConst(0,0,0,1), r3);

				CMat4Dfp32 Matrices[2];
				CMat4Dfp32& MatProj = Matrices[0];
				CMat4Dfp32& Mat = Matrices[1];
//				CMat4Dfp32 Mat, MatProj;
				Mat.r[0] = _pMatrix->r[0];
				Mat.r[1] = _pMatrix->r[1];
				Mat.r[2] = _pMatrix->r[2];
				Mat.r[3] = r3;
				Mat.Transpose();
				M_VMatMul(m_Matrix_ProjectionTransposeGL, Mat, MatProj);

				Mat.r[3] = Translate;

#ifdef GCM_COMMANDBUFFERWRITE
				{
					MTRACE_SCOPECOMMANDS("RSX_Matrix");

					uint32 CmdReserve = RSXCMD_VPPARAM_SIZE + 16*2;
					uint32* pCmdBegin = RSX_AllocateCommandAlign16(CmdReserve);
					uint32 M_RESTRICT* pCmdBuffer = pCmdBegin;
					RSX_VPParam(pCmdBuffer, 0, 8);
					vec128 M_RESTRICT* pVCmd = (vec128 M_RESTRICT*) pCmdBuffer;
					pVCmd[0] = MatProj.r[0];
					pVCmd[1] = MatProj.r[1];
					pVCmd[2] = MatProj.r[2];
					pVCmd[3] = MatProj.r[3];
					pVCmd[4] = Mat.r[0];
					pVCmd[5] = Mat.r[1];
					pVCmd[6] = Mat.r[2];
					pVCmd[7] = Mat.r[3];
				}
#else
//				gcmSetVertexProgramConstants(0, 4 * 4, MatProj.k[0]);
//				gcmSetVertexProgramConstants(4, 4 * 4, Mat.k[0]);
				gcmSetVertexProgramConstants(0, 8 * 4, Matrices[0].k[0]);
#endif
			}
			else
			{
//				gcmSetVertexProgramParameterBlock(0, 4, m_Matrix_ProjectionTransposeGL.k[0]);
//				gcmSetVertexProgramParameterBlock(4, 4, m_Matrix_Unit.k[0]);
				gcmSetVertexProgramConstants(0, 4 * 4, m_Matrix_ProjectionTransposeGL.k[0]);
				gcmSetVertexProgramConstants(4, 4 * 4, m_Matrix_Unit.k[0]);
			}
		}
	}
	else if (_iMode == CRC_MATRIX_PROJECTION)
	{
		ConOut("(CRCPS3GCM::Matrix_SetRender) Projection matrix stack is not supported.");
	}
	else if ((_iMode >= CRC_MATRIX_TEXTURE) && (_iMode < CRC_MATRIX_TEXTURE + CRC_MAXTEXCOORDS))
	{
		// Handled with Attrib_SetVPPipeline
	}
}

};	// namespace NRenderPS3GCM
