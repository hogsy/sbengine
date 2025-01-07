#include "PCH.h"
#include "WGameContext_P6.h"

static const char *g_ContentTypeTranslate[] =
{
	"picture", "video", "timelocklevel", "extralevel", "text", "sound", "script", NULL
};

void CGameContext_P6::Create(CStr _WorldPathes, CStr _GameName, spCXR_Engine _spEngine)
{
	CGameContext::Create(_WorldPathes, _GameName, _spEngine);

	m_ExtraContent.m_lProviders.Add(&m_ExtraContent_Offline_P6);
	m_ExtraContent_Offline_P6.m_pHandler = &m_ExtraContent;

	m_ExtraContent.Update();
#if defined(PLATFORM_XENON)
	m_pMPHandler = DNew(CWGameLiveHandler) CWGameLiveHandler;
	m_pMPHandler->Initialize();
#endif
}

void CExtraContent_P6::UpdateContentList()
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) Error_static("Create", "No system.");

	CStr ExtraContentPath = pSys->m_ExePath + "P6_ExtraContent\\";
	CStr LocalizeSuffix;
#ifdef PLATFORM_XBOX
	switch(XGetLanguage())
	{
	case XC_LANGUAGE_GERMAN:	LocalizeSuffix = "Ger"; break;
	case XC_LANGUAGE_FRENCH:	LocalizeSuffix = "Fre"; break;
	case XC_LANGUAGE_SPANISH:	LocalizeSuffix = "Spa"; break;
	case XC_LANGUAGE_ITALIAN:	LocalizeSuffix = "Ita"; break;
	}
#else
	if(pSys->GetEnvironment())
		LocalizeSuffix = pSys->GetEnvironment()->GetValue("LANGUAGE");
#endif

	TArray<CStr> lDefines;
	CRegistry* pRegDef = pSys->GetRegistry()->GetDir("GAMECONTEXT\\REGISTRYDEFINES");
	if (pRegDef)
	{
		for(int i = 0; i < pRegDef->GetNumChildren(); i++)
			lDefines.Add(pRegDef->GetName(i));
	}

	CDirectoryNode Node;
	Node.ReadDirectory(ExtraContentPath + "*.*");
	for(int f = 0; f < Node.GetFileCount(); f++)
	{
		if(Node.IsDirectory(f))
		{
			if(Node.GetFileName(f) != "." && Node.GetFileName(f) != "..")
			{
				CStr Path = ExtraContentPath + Node.GetFileName(f) + "\\info.xrg";
				if(CDiskUtil::FileExists(Path))
				{
					if(LocalizeSuffix != "")
					{
						// Manually find path, since the info.xrg files conflict if we would use the
						// resource handler (and also that the resource handler isn't initiated yet)
						CStr Path2 = pSys->m_ExePath + "ExtraContent_" + LocalizeSuffix + "\\" + Node.GetFileName(f) + "\\info.xrg";
						if(CDiskUtil::FileExists(Path2))
							Path = Path2;
					}

					CRegistry_Dynamic Reg;
					Reg.XRG_Read(Path, lDefines);

					for(int i = 0; i < Reg.GetNumChildren(); i++)
					{
						CRegistry *pChild = Reg.GetChild(i);

						TPtr<CExtraContentHandler::CContent> spContent = MNew(CExtraContentHandler::CContent);
						spContent->m_ID = pChild->GetValue("ID");
						spContent->m_Name = pChild->GetValue("NAME");
						spContent->m_Desc = pChild->GetValue("DESC");
						spContent->m_Thumbnail = pChild->GetValue("THUMBNAIL");
						spContent->m_Type = pChild->GetValue("TYPE").CStr::TranslateInt(g_ContentTypeTranslate);
						spContent->m_Flags = pChild->GetValuei("FLAGS");
						spContent->m_Param = pChild->GetValuef("PARAM");
						spContent->m_Installed = true;
						spContent->m_Key.Parse(pChild->GetValue("KEY"));
						m_pHandler->m_lspContent.Add(spContent);
					}
				}
			}
		}
	}
}


