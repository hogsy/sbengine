#include "PCH.h"

#include "MDisplayPS3.h"
#include "MRenderPS3_Context.h"
#include "MRenderPS3_Def.h"

void CRenderContextPS3::Con_ChangePicMip(int _Level, int _iPicMip)
{
	if (m_lPicMips[_iPicMip] == _Level)
	{
//		LogExtensionState();
		return;
	}

	m_lPicMips[_iPicMip] = _Level;
	{
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if (pSys && pSys->GetEnvironment())
		{
			pSys->GetEnvironment()->SetValuei(CFStrF("R_PICMIP%d", _iPicMip) , m_lPicMips[_iPicMip]);
		}

		Texture_MakeAllDirty(_iPicMip);

		// Notify all subsystems
		if(pSys) pSys->System_BroadcastMessage(CSS_Msg(CSS_MSG_PRECACHEHINT_TEXTURES));
	}

}

void CRenderContextPS3::Con_r_picmip(int _iPicMip, int _Level)
{
	Con_ChangePicMip(_Level, _iPicMip);
}

void CRenderContextPS3::Con_gl_reloadprograms()
{
	m_bPendingProgramReload = 1;
}

/*void CRenderContextPS3::Con_gl_vpalways(int _Value)
{
	m_VP_bUseAlways = _Value != 0;
}*/

void CRenderContextPS3::Con_gl_anisotropy(fp4 _Value)
{
	if (m_Anisotropy == _Value)
		return;

	m_Anisotropy = _Value;
	m_bPendingTextureParamUpdate = 1;

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys) pSys->GetEnvironment()->SetValuef("R_ANISOTROPY", m_Anisotropy);
}

void CRenderContextPS3::Con_gl_vsync(int _Value)
{
	if (m_VSync == _Value)
		return;

	m_VSync = _Value;
	// Notify all subsystems
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys) pSys->GetEnvironment()->SetValuei("VID_VSYNC", m_VSync);
}

void CRenderContextPS3::Con_gl_logprograms(int _Value)
{
	m_bLogVP = _Value != 0;
}

void CRenderContextPS3::Register(CScriptRegisterContext & _RegContext)
{
	CRC_Core::Register(_RegContext);

	_RegContext.RegFunction("r_picmip", this, &CRenderContextPS3::Con_r_picmip);
	_RegContext.RegFunction("gl_reloadprograms", this, &CRenderContextPS3::Con_gl_reloadprograms);
	_RegContext.RegFunction("r_anisotropy", this, &CRenderContextPS3::Con_gl_anisotropy);
	_RegContext.RegFunction("r_vsync", this, &CRenderContextPS3::Con_gl_vsync);
	_RegContext.RegFunction("gl_logprograms", this, &CRenderContextPS3::Con_gl_logprograms);

};

