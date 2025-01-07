#include "PCH.h"

#include "MDisplayPS3.h"
#include "MRenderPS3_Context.h"
#include "MRenderPS3_Def.h"

namespace NRenderPS3GCM
{

void CRCPS3GCM::Cmd_ReloadPrograms()
{
	m_ContextVP.DeleteAllObjects();
	m_ContextFP.DeleteAllObjects();
}

void CRCPS3GCM::Cmd_InvalidateProgramCache()
{
}

void CRCPS3GCM::Cmd_TexMinLod(int _TexID, int _LOD)
{
	CRenderPS3Resource_Texture* pTex = m_ContextTexture.m_lpTexResources[_TexID];
	if(pTex)
	{
		int Bias = Max(0, Min(_LOD, (int)(pTex->m_MaxLOD - 1)));

		pTex->m_MinLOD;
		ConOut(CStrF("Texture Min LOD for texture %d set to %d", _TexID, Bias));
	}
}

void CRCPS3GCM::Cmd_TexMaxLod(int _TexID, int _LOD)
{
	CRenderPS3Resource_Texture* pTex = m_ContextTexture.m_lpTexResources[_TexID];
	if(pTex)
	{
		int Bias = Max((int)(pTex->m_MinLOD + 1), Min(_LOD, (int)pTex->m_RSXTexture.mipmap - 1));

		pTex->m_MaxLOD = Bias;
		ConOut(CStrF("Texture Max LOD for texture %d set to %d", _TexID, Bias));
	}
}

void CRCPS3GCM::Cmd_TexSingleLod(int _TexID, int _LOD)
{
	CRenderPS3Resource_Texture* pTex = m_ContextTexture.m_lpTexResources[_TexID];
	if(pTex)
	{
		int Bias = Max(0, Min(_LOD, (int)pTex->m_RSXTexture.mipmap - 1));

		pTex->m_MinLOD = Bias;
		pTex->m_MaxLOD = Bias + 1;
		ConOut(CStrF("Texture Single LOD for texture %d set to %d", _TexID, Bias));
	}
}

void CRCPS3GCM::Cmd_Picmip(int _iPicMip, int _Picmip)
{
	if(m_ContextTexture.m_lPicMips[_iPicMip] != _Picmip)
	{
		m_ContextTexture.m_lPicMips[_iPicMip] = _Picmip;
		{
			MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
			if (pSys && pSys->GetEnvironment())
				pSys->GetEnvironment()->SetValuei(CFStrF("R_PICMIP%d", _iPicMip) , _Picmip);

			Texture_MakeAllDirty(_iPicMip);

			// Notify all subsystems
			if(pSys) pSys->System_BroadcastMessage(CSS_Msg(CSS_MSG_PRECACHEHINT_TEXTURES));
		}
	}
}

void CRCPS3GCM::Cmd_DynamicLoadMips(int _Enable)
{
	ms_This.m_ContextTexture.m_bMip0Streaming = (_Enable != 0);
}

void CRCPS3GCM::Cmd_Nop(fp32 _Arg)
{
	(void)_Arg;
	ConOut("Command is a NOP on this platform");
}

void CRCPS3GCM::Register(CScriptRegisterContext & _RegContext)
{
	MSCOPE(CRCPS3GCM::Register, RENDER_PS3);
	CRC_Core::Register(_RegContext);

	_RegContext.RegFunction("r_antialias", this, &CRCPS3GCM::Cmd_Nop);
	_RegContext.RegFunction("r_backbufferformat", this, &CRCPS3GCM::Cmd_Nop);
	_RegContext.RegFunction("r_anisotropy", this, &CRCPS3GCM::Cmd_Nop);
	_RegContext.RegFunction("r_vsync", this, &CRCPS3GCM::Cmd_Nop);

	_RegContext.RegFunction("r_picmip", this, &CRCPS3GCM::Cmd_Picmip);
	_RegContext.RegFunction("r_updateprograms", this, &CRCPS3GCM::Cmd_ReloadPrograms);
	_RegContext.RegFunction("r_dynamicloadmips", this, &CRCPS3GCM::Cmd_DynamicLoadMips);
	_RegContext.RegFunction("ps3_invalidatecache", this, &CRCPS3GCM::Cmd_InvalidateProgramCache);
	_RegContext.RegFunction("ps3_texminlod", this, &CRCPS3GCM::Cmd_TexMinLod);
	_RegContext.RegFunction("ps3_texmaxlod", this, &CRCPS3GCM::Cmd_TexMaxLod);
	_RegContext.RegFunction("ps3_texsinglelod", this, &CRCPS3GCM::Cmd_TexSingleLod);
};

};	// namespace NRenderPS3GCM
