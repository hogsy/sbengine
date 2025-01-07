
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Codecs
|__________________________________________________________________________________________________
\*************************************************************************************************/

#include "MRTC.h"

class CRegisterMSystem
{
public:
	CRegisterMSystem()
	{
		MRTC_REFERENCE(CScriptLanguageCore);
		
		MRTC_REFERENCE(CRegistry);
//		MRTC_REFERENCE(CRegistry_Const);
		MRTC_REFERENCE(CRegistryCompiled);
		MRTC_REFERENCE(CRegistry_Dynamic);


		MRTC_REFERENCE(CPS3_Device_Joystick);
		MRTC_REFERENCE(CPS3_Device_KeyBoard);
		MRTC_REFERENCE(CInputContext_PS3);

		MRTC_REFERENCE(CSystemPS3);
		MRTC_REFERENCE(CDisplayContextNULL);
		MRTC_REFERENCE(CConsole);
		MRTC_REFERENCE(CPerfGraph);
		MRTC_REFERENCE(CTextureContext);
		MRTC_REFERENCE(CTexture);
		MRTC_REFERENCE(CTextureContainer_Plain);
		MRTC_REFERENCE(CTextureContainer_VirtualXTC);
		MRTC_REFERENCE(CTextureContainer_Video);
		MRTC_REFERENCE(CTextureContainer_VirtualXTC2);

		MRTC_REFERENCE(CScriptContext);
	}
};

CRegisterMSystem g_MSystemDyn;
