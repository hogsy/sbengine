
#include "PCH.h"
#include "../Platform/Platform.h"
#include "MFloat.h"
#include "MSystem_Core.h"

#include "../XR/XRSurfaceContext.h"
#include "../XR/XRVBContext.h"

#include "MContentContext.h"

// -------------------------------------------------------------------
//  CSubSystem
// -------------------------------------------------------------------
CSubSystem::CSubSystem()
{
	MAUTOSTRIP(CSubSystem_ctor, MAUTOSTRIP_VOID);
/*	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) Error_static("CSubSystem::-", "No system");

	pSys->System_Add(this);*/
}

CSubSystem::~CSubSystem()
{
	MAUTOSTRIP(CSubSystem_dtor, MAUTOSTRIP_VOID);
/*	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) Error_static("CSubSystem::~", "No system");

	pSys->System_Remove(this);*/
}

/*int CSubSystem::OnMessage(const CSS_Msg& _Msg)
{
	MAUTOSTRIP(CSubSystem_OnMessage, NULL);
	return 0;
}

void CSubSystem::OnRefresh(int _Context)
{
	MAUTOSTRIP(CSubSystem_OnRefresh, MAUTOSTRIP_VOID);
}

void CSubSystem::OnBusy(int _Context)
{
	MAUTOSTRIP(CSubSystem_OnBusy, MAUTOSTRIP_VOID);
}*/

// -------------------------------------------------------------------
//  CApplication
// -------------------------------------------------------------------
CApplication::CApplication()
{
	MAUTOSTRIP(CApplication_ctor, MAUTOSTRIP_VOID);
	m_pObjMgr = NULL;
	m_pSystem = NULL;
}

CApplication::~CApplication()
{
	MAUTOSTRIP(CApplication_dtor, MAUTOSTRIP_VOID);
	if (m_pSystem) m_pSystem->System_Remove(this);
	
	RemoveFromConsole();
}

void CApplication::Create()
{
	MAUTOSTRIP(CApplication_Create, MAUTOSTRIP_VOID);
	m_pObjMgr = MRTC_GOM();
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) Error("Create", "No system.");
	m_pSystem = pSys;
	m_pSystem->System_Add(this);
}

void CApplication::OnRender(CDisplayContext* _pDisplay, CImage* _pFrame, CClipRect _Clip, int _Context)
{
	MAUTOSTRIP(CApplication_OnRender, MAUTOSTRIP_VOID);
	OnRender(_pDisplay, _pFrame, _Clip);
}

// -------------------------------------------------------------------
//  CSystemCore
// -------------------------------------------------------------------
CDisplayContextContainer::CDisplayContextContainer(spCDisplayContextDesc _spDesc)
{
	MAUTOSTRIP(CDisplayContextContainer_ctor, MAUTOSTRIP_VOID);
	m_spDCDesc = _spDesc;

	if (MRTC_GOM()->GetDllLoading())
	{
		MRTC_GOM()->GetClassRegistry()->LoadClassLibrary(m_spDCDesc->m_FileName);
	}
	

	spCReferenceCount spObj = (CReferenceCount*) MRTC_GOM()->CreateObject(m_spDCDesc->m_DCClass);
	spCDisplayContext spDisplay = safe_cast<CDisplayContext> ((CReferenceCount*)spObj);
	if (!spDisplay)
	{
		if (MRTC_GOM()->GetDllLoading())
		{
			if (m_spDCDesc->m_FileName.Len())
				MRTC_GOM()->GetClassRegistry()->UnloadClassLibrary(m_spDCDesc->m_FileName);
		}
		Error("-", "Failed to create display-context.");
	}
	M_TRY
	{ 
		spDisplay->Create();
	}
	M_CATCH(
	catch(CCException)
	{ 
		if (MRTC_GOM()->GetDllLoading())
		{
			if (m_spDCDesc->m_FileName.Len())
				MRTC_GOM()->GetClassRegistry()->UnloadClassLibrary(m_spDCDesc->m_FileName);
		}
		throw;
	}
	)
	m_spDC = spDisplay;
}

CDisplayContextContainer::~CDisplayContextContainer()
{
	MAUTOSTRIP(CDisplayContextContainer_dtor, MAUTOSTRIP_VOID);
	if (m_spDC!=NULL)
	{
		m_spDC = NULL;

		if (MRTC_GetObjectManager()->GetDllLoading())
		{
			if (m_spDCDesc->m_FileName.Len())
				MRTC_GOM()->GetClassRegistry()->UnloadClassLibrary(m_spDCDesc->m_FileName);
		}
	}
}

// -------------------------------------------------------------------
//MRTC_IMPLEMENT_DYNAMIC(CSystemCore, CSystem);

// -------------------------------------------------------------------
/*
IMPLEMENT_OPERATOR_NEW(CSystemCore);

*/

CSystemCore::CSystemCore()
{
	MAUTOSTRIP(CSystemCore_ctor, MAUTOSTRIP_VOID);
	m_bReg = false;
	m_bMXRLoaded = false;
	m_bEnvLoaded = false;
	m_bBreakRequested = false;
	m_bMemoryCheck = false;
	m_iCurrentDC = -1;
	m_iMainWindow = -1;
	m_RefreshRecursion = 0;
};

CSystemCore::~CSystemCore()
{
	MAUTOSTRIP(CSystemCore_dtor, MAUTOSTRIP_VOID);
//	CSystemCore::Destroy();
};

void CSystemCore::Create(char* _cmdline, const char* _pAppClassName)
{
	MAUTOSTRIP(CSystemCore_Create, MAUTOSTRIP_VOID);
	MSCOPE(CSystemCore::Create, SYSTEMCORE);
	
//	DetectCPU();

	// Get startup info
	m_CommandLine = CStr((char*) _cmdline);
	m_pCmdLine = _cmdline;
	m_AppClassName = _pAppClassName;

	MACRO_AddSubSystem(this);

	CreateSystems(); // 2964 kb
	CreateXR(); // 4324 kb
	CreateTextureContext(GetEnvironment()->GetValuei("XR_MAXTEXTURES", 1024*32), 0);
	DC_InitList();

	ConOutL(CStrF("Processor detected: %s", (char*) GetCPUName(true) ));

	for(int i = 1 ; i < CONTENT_MAX; i++)
	{
		CContentContext* pCC = GetContentContext(i);
		if(pCC) pCC->Create();
	}

	if (_pAppClassName)
	{
LogFile(CStrF("(CSystemCore::Create) Creating application: %s", _pAppClassName));
//		MRTC_GOM()->GetClassRegistry()->GetClass(_pAppClassName);
		spCReferenceCount spObj = (CReferenceCount*) MRTC_GOM()->GetClassRegistry()->CreateObject(_pAppClassName);
		m_spApp = (CApplication *)(CReferenceCount *)spObj;//TDynamicCast<CApplication> ((CReferenceCount*)spObj);
		if (m_spApp==NULL) Error("Create", CStrF("Failed to create application object. (%s)", _pAppClassName));
		m_spApp->Create();
	}
	else
		LogFile("WARNING: No application object created.");
//LogFile("(CSystemCore::Create) Done.");
}

void CSystemCore::Destroy()
{
	MAUTOSTRIP(CSystemCore_Destroy, MAUTOSTRIP_VOID);
	MSCOPE(CSystemCore::Destroy, SYSTEMCORE);
	
	LogFile("(CSystemCore::Destroy)");

/*	if (GetEnvironment() && GetEnvironment()->GetValuei("RESOURCES_LOG") != 0)
	{
		if (m_spTC)
			m_spTC->LogUsed();
		if (m_spWC)
			m_spWC->LogUsed();
	}*/

	// Destroy application.
	m_spApp = NULL;


	for(int i = 1 ; i < CONTENT_MAX; i++)
	{
		CContentContext* pCC = GetContentContext(i);
		if(pCC) pCC->Destroy();
	}

	// Destroy InputContext
	m_spInput = NULL;

	// Unregister VBContext
	if (m_spVBCtx != NULL) MRTC_GOM()->UnregisterObject((CReferenceCount*)m_spVBCtx, "SYSTEM.VBCONTEXT");
	m_spVBCtx = NULL;

	// Unregister SurfaceContext
	if (m_spSurfCtx != NULL) MRTC_GOM()->UnregisterObject((CReferenceCount*)m_spSurfCtx, "SYSTEM.SURFACECONTEXT");
	m_spSurfCtx = NULL;

	// Unregister TextureContext
	if (m_spTC != NULL) MRTC_GOM()->UnregisterObject((CReferenceCount*)m_spTC, "SYSTEM.TEXTURECONTEXT");
	m_spTC = NULL;

	// Unload XR DLL
	if (m_bMXRLoaded)
	{
		M_TRY
		{
			if (MRTC_GetObjectManager()->GetDllLoading())
			{
				MRTC_GOM()->GetClassRegistry()->UnloadClassLibrary(m_SystemPath + GetModuleName("MXR"));
			}
		}
		M_CATCH(
		catch(CCException)
		{
		}
		)
		m_bMXRLoaded = false;
	}

	// Unregister WaveContet
	if (m_spWC != NULL) MRTC_GOM()->UnregisterObject((CReferenceCount*)m_spWC, "SYSTEM.WAVECONTEXT");
	
	m_spWC = NULL;

	// Unregister Console
	if (m_spCon != NULL)
	{
		m_spCon->RemoveSubSystem(this);
		MRTC_GOM()->UnregisterObject((CReferenceCount*)m_spCon, "SYSTEM.CONSOLE");
	}

	// Destroy Console
	m_spCon = NULL;

	// Write Environment
#ifndef PLATFORM_CONSOLE
	if (m_spRegistry != NULL && m_bEnvLoaded)
	{
		try
		{
			//spCRegistry spEnv = m_spRegistry->Find("ENV");
			//m_spRegistry->WriteRegistryDir("OPT", m_OptionsFilename);
//			WriteOptions();
			//spEnv->DeleteDir("OPT"); 
		}
		catch(CCException)
		{
			LogFile("(CSystemCore::Destroy) Error writing options " + m_OptionsFilename);
		}

		try
		{
			LogFile("(CSystemCore::CreateSystems) Writing environment " + m_EnvFileName);
			CCFile File;
			File.Open(m_EnvFileName, CFILE_WRITE);

			m_spRegistry->WriteRegistryDir("ENV", &File);
		}
		catch(CCException)
		{
			LogFile("(CSystemCore::Destroy) Error writing environment " + m_EnvFileName);
		}
	}
#endif

	// Unregister registry
	if (m_spRegistry != NULL)
	{
		//AR-REMOVE MRTC_GOM()->UnregisterObject((CReferenceCount*)m_spRegistry, "REGISTRY");
		//AR-REMOVE MRTC_GOM()->UnregisterObject((CReferenceCount*)GetEnvironment(), "REGISTRY.ENV");
	}

	// Remove System from the sub-system list
	MACRO_RemoveSubSystem(this);

	LogFile("(CSystemCore::Destroy) Destroying registry.");
	m_spRegistry = NULL;
	LogFile("(CSystemCore::Destroy) Done.");
}

void CSystemCore::WriteEnvironment()
{
	if (m_spRegistry != NULL && m_bEnvLoaded)
	{
		M_TRY
		{
			LogFile("(CSystemCore::CreateSystems) Writing environment " + m_EnvFileName);

			CDiskUtil::CreatePath(m_EnvFileName.GetPath());
			CCFile File;
			File.Open(m_EnvFileName, CFILE_WRITE);

			m_spRegistry->WriteRegistryDir("ENV", &File);
		}
		M_CATCH(
		catch(CCException)
		{
			LogFile("(CSystemCore::Destroy) Error writing environment " + m_EnvFileName);
		}
		)
	}
}

#ifdef PLATFORM_XENON
#include "xtl.h"
#endif

void CSystemCore::CreateSystems()
{
	MAUTOSTRIP(CSystemCore_CreateSystems, MAUTOSTRIP_VOID);
	MSCOPE(CSystemCore::CreateSystems, SYSTEMCORE);
	
	MFloat_Init();

	// Construct!
	{
		LogFile("(CSystemCore::CreateRegistry) Creating Registry...");
		if ((m_spRegistry = REGISTRY_CREATE) == NULL) MemError("CreateSystems");
		//AR-REMOVE MRTC_GOM()->RegisterObject((CReferenceCount*)m_spRegistry, "REGISTRY");

		// Create and read environment directory
		{
#ifdef PLATFORM_XBOX
/*	#ifdef M_DEMO_XBOX
				CStr Env = "EnvironmentXBox.cfg";
	#else
				CStr Env;
				switch(XGetLanguage())
				{
				case XC_LANGUAGE_ENGLISH:	Env = "EnvironmentXBox.cfg"; break;
				case XC_LANGUAGE_GERMAN:	Env = "EnvironmentXBox_Ger.cfg"; break;
				case XC_LANGUAGE_FRENCH:	Env = "EnvironmentXBox_Fre.cfg"; break;
	//			case XC_LANGUAGE_SPANISH:	Env = "EnvironmentXBox_Spa.cfg"; break;
	//			case XC_LANGUAGE_ITALIAN:	Env = "EnvironmentXBox_Ita.cfg"; break;
				default: 
					Env = "EnvironmentXBox.cfg";
				}
				
				{
					CStr EnvCheck = Env;
					if (EnvCheck.GetDevice() == "")
						EnvCheck = m_ExePath + EnvCheck;
					
					if (!CDiskUtil::FileExists(EnvCheck))
						Env = "EnvironmentXBox.cfg";					
				}

				
				m_OptionsFilename = CStr("T:\\Options.cfg");
	#endif*/
		
			// Automatically genereate contentpath information further down - JA
#if defined(M_DEMO_XBOX)
			CStr Env;
			if(CDiskUtil::FileExists(m_ExePath + "EnvironmentXboxDemo.cfg"))
				Env = "EnvironmentXboxDemo.cfg";
			else
				Env = "EnvironmentXbox.cfg";

			m_OptionsFilename = CStr("Z:\\Options.cfg");
#else
			CStr Env("EnvironmentXbox.cfg");
			m_OptionsFilename = CStr("T:\\Options.cfg");
#endif
#elif defined(PLATFORM_DOLPHIN)
		CStr Env;
		switch (OSGetLanguage())
		{
		case OS_LANG_ENGLISH: Env = "Environment_GC.cfg"; break;
		case OS_LANG_GERMAN:  Env = "Environment_GC_Ger.cfg"; break;
		case OS_LANG_FRENCH:  Env = "Environment_GC_Fre.cfg"; break;
		case OS_LANG_SPANISH: Env = "Environment_GC_Spa.cfg"; break;
		case OS_LANG_ITALIAN: Env = "Environment_GC_Ita.cfg"; break;
		default:
			Env = "Environment_GC.cfg"; break;
		}
		m_OptionsFilename = m_ExePath + "Options.cfg";

#elif defined(PLATFORM_PS2)
			CStr Env("Environment_PS2.cfg");
			m_OptionsFilename = m_ExePath + CStr("Options.cfg");
#elif defined(PLATFORM_PS3)
			CStr Env("Environment_PS3.cfg");
			m_OptionsFilename = m_ExePath + CStr("Options.cfg");
#else
			CStr Env("Environment.cfg");
			m_OptionsFilename = m_ExePath + CStr("Options.cfg");
#endif

			CStr SavePath = m_ExePath;
			CStr Cmd = m_CommandLine;
			if (Cmd.Find("-env") >= 0)
			{
				Cmd.GetStrSep("-env");
				Cmd.Trim();
				if (Cmd[0] == '"') //"
				{
					int Pos = 0;
					Cmd = Cmd.Del(0,1);
					Env = ParseEscSeq_Char((char*)Cmd, Pos, Cmd.Len(), false);
				}
				else
				{
					Env = Cmd.GetStrSep(" ");
				}

				if (Env.GetDevice() == "")
					Env = m_ExePath + Env;

				if (!CDiskUtil::FileExists(Env))
				{
					ConOutL("(CSystemCore::CreateSystems) Invalid environment file: " + Env);
					Env = m_ExePath + "Environment.cfg";
				}
			}
			else
			{
#if !defined(PLATFORM_XENON) || 1
				M_TRY
				{
					CCFile TestFile;
					TestFile.Open(SavePath + "76CEC8F0-E170-4fd3-BBC3-BF93B4F8B925.test", CFILE_WRITE);
					TestFile.Write("Test", 4);
					TestFile.Close();
					CDiskUtil::DelFile(SavePath + "76CEC8F0-E170-4fd3-BBC3-BF93B4F8B925.test");
				}
				M_CATCH(
				catch (CCException)
				{
					SavePath = GetUserFolder();
				}		
				)
#else
				SavePath = GetUserFolder();
#endif
				Env = SavePath + Env;
			}

			m_EnvFileName = Env;
			bool bIsRemote = Cmd.LowerCase().Find("-remote") >= 0;
			if (!bIsRemote && CDiskUtil::FileExists(m_EnvFileName))
			{
				LogFile("(CSystemCore::CreateSystems) Reading environment " + m_EnvFileName);
				m_spRegistry->ReadRegistryDir("ENV", m_EnvFileName);

				spCRegistry spEnv = m_spRegistry->Find("ENV");
				if (!spEnv) Error("CreateSystems", "Internal error. (1)");
			}
			else
			{
				if(!bIsRemote) ConOutL("(CSystemCore::CreateSystems) " + m_EnvFileName + " doesn't exist.");
				m_spRegistry->CreateDir("ENV");
			}

			m_spRegistry->SetValue("SAVEPATH", SavePath);

#ifdef PLATFORM_XBOX
			spCRegistry spEnv = m_spRegistry->Find("ENV");
			if(spEnv)
			{
				CStr ContentPath = spEnv->GetValue("DEFAULTGAMEPATH", "Content\\");
				CStr Path0 = ContentPath.GetStrSep(";");
				CStr Path1;
				switch(XGetLanguage())
				{
				case XC_LANGUAGE_GERMAN:	
					Path1 = Path0.Copy(0, Path0.Len() - 1) + "_Ger\\"; 
					spEnv->SetValuei("CUTSCENESUBTITLESDEFAULT", 1);
					break;
				case XC_LANGUAGE_FRENCH:	
					Path1 = Path0.Copy(0, Path0.Len() - 1) + "_Fre\\"; 
					spEnv->SetValuei("CUTSCENESUBTITLESDEFAULT", 1);
					break;
				case XC_LANGUAGE_SPANISH:	
					Path1 = Path0.Copy(0, Path0.Len() - 1) + "_Spa\\"; 
					break;
				case XC_LANGUAGE_ITALIAN:	
					Path1 = Path0.Copy(0, Path0.Len() - 1) + "_Ita\\"; 
					break;
				}
				if(Path1 != "" && CDiskUtil::FileExists(m_ExePath + Path1 + spEnv->GetValue("DEFAULTGAME", "P5") + ".xrg"))
				{
					if(ContentPath != "")
						spEnv->SetValue("DEFAULTGAMEPATH", Path0 + ";" + Path1 + ";" + ContentPath);
					else
						spEnv->SetValue("DEFAULTGAMEPATH", Path0 + ";" + Path1);
				}
			}
#endif
			
			// Don't flag environment as loaded if we're remote (so we won't save it)
			if(!bIsRemote) m_bEnvLoaded = true;
			
			// LoadOptions();
		}

		//MRTC_GOM()->RegisterObject((CReferenceCount*)GetEnvironment(), "REGISTRY.ENV");
	}
	
	// Check for commandline in environment
	if (GetEnvironment()->FindChild("COMMANDLINE"))
	{
		m_CommandLine = GetEnvironment()->GetValue("COMMANDLINE");
		ConOutL(CStr("CommandLine is overrided by value in environment file."));
	}

	// Setup commandline arguments
	if( m_CommandLine.Len() > 0 )
	{
		CRegistry* pCmdLine = GetRegistry();
		pCmdLine->AddKey( "CommandLine", "" );

		pCmdLine	= pCmdLine->FindChild( "CommandLine" );

		CStr CmdLine = m_CommandLine;
		int iOption = 0;

		while( CmdLine.Len() > 0 )
		{
			CmdLine.Trim();	// Get rid of whitespaces
			CStr Word;
		
			if( CmdLine[0] == '\"' )
			{
				// If it starts with a quotation marker.. then hunt for the next marker	
				const char *pStr = CmdLine;
				int Mode = 1;
				 // If a word contains 1+ "//" sequences we need the number of extra chars to clip away at the end.
				int nExtraChars = 0;
				while (pStr && *pStr)
				{		
					char Char = *pStr;
					switch(Char)
					{
					case '\"':
						{
							if (Mode == 2)
							{
								char Temp[2];
								Temp[0] = Char;
								Temp[1] = 0;
								Word += Temp;
								Mode = 0;
							}
							else if (Mode == 1)
							{
								Mode = 0;
							}
							else
							{
								pStr = NULL;
								break;
							}
							++pStr;
						}
						break;
					case '\\':
						{
							if (Mode == 2)
							{
								char Temp[2];
								Temp[0] = Char;
								Temp[1] = 0;
								Word += Temp;
								Mode = 0;
							}
							else {
								nExtraChars++;
								Mode = 2;
							}
							++pStr;
						}
						break;
					default:
						{
							char Temp[2];
							Temp[0] = Char;
							Temp[1] = 0;
							Word += Temp;
							++pStr;
						}
						break;
					}

				}

				CmdLine = CmdLine.DelTo( Word.Len() + 2 + nExtraChars );
			}
			else
			{
				Word = CmdLine.Separate( " " );
			}

			pCmdLine->AddKey( CStrF("option%d", iOption).Str(), Word.Str());
			iOption++;
		}
	}


/*LogFile("(CSystemCore::CreateSystems) Creating Environment...");
	if ((m_spEnv = DNew(CEnvironment) CEnvironment(m_ExePath + "\\Environment.cfg")) == NULL) MemError("CreateSystems");*/

	{
		MSCOPESHORT(Console);
		LogFile("(CSystemCore::CreateSystems) Creating Console...");
		if ((m_spCon = MNew(CConsole)) == NULL) MemError("CreateSystems");
		int Lines = GetEnvironment()->GetValuei("CON_LINES", 1024);
		m_spCon->Create(Lines, "SYSTEM.CONSOLE");
		MRTC_GOM()->RegisterObject((CReferenceCount*)m_spCon, "SYSTEM.CONSOLE");
	}

	{
		MSCOPESHORT(WaveContext);
		LogFile("(CSystemCore::CreateSystems) Creating WaveContext...");
		int MaxWaveIDs = GetEnvironment()->GetValuei("SND_MAXWAVES", 32768);
		if ((m_spWC = MNew1(CWaveContext, MaxWaveIDs)) == NULL) MemError("Init");
		MRTC_GOM()->RegisterObject((CReferenceCount*)m_spWC, "SYSTEM.WAVECONTEXT");
	}

	m_spCon->AddSubSystem(this);
//	m_spCon->AddSubSystem(m_spEnv);
}

void CSystemCore::CreateInput()
{
	MAUTOSTRIP(CSystemCore_CreateInput, MAUTOSTRIP_VOID);
	MSCOPE(CSystemCore::CreateInput, SYSTEMCORE);
	
	LogFile("(CSystemCore::CreateSystems) Creating InputContext...");
	if ((m_spInput = MCreateInputContext() ) == NULL)
		Error("CreateInput", "Failed to create input-object.");
}

void CSystemCore::CreateXR()
{
	MAUTOSTRIP(CSystemCore_CreateXR, MAUTOSTRIP_VOID);
	MSCOPE(CSystemCore::CreateXR, SYSTEMCORE_XR);
	
	LogFile("(CSystemCore::CreateXR) Initializing XR...");

	if (CDiskUtil::FileExists(m_SystemPath + GetModuleName("MXR")))
	{
		M_TRY
		{
			if (MRTC_GetObjectManager()->GetDllLoading())
			{
				MRTC_GOM()->GetClassRegistry()->LoadClassLibrary(m_SystemPath + GetModuleName("MXR"));
				m_bMXRLoaded = true;
			}
		}
		M_CATCH(
		catch(CCException _e)
		{
			LogFile("(CSystemCore::CreateXR) Failure loading" + GetModuleName("MXR"));
		}
		)
	}
	else
		LogFile("    ...MXR module unavailable.");
}

void CSystemCore::CreateTextureContext(int _TextureIDHeap, int _TextureCacheHeap)
{
	MAUTOSTRIP(CSystemCore_CreateTextureContext, MAUTOSTRIP_VOID);
	// ----------------------------------------------------------------
	// Create and register texture-context
	{
		MSCOPE(CSystemCore::CreateTextureContext, SYSTEMCORE_TC);
		LogFile("(CSystemCore::CreateTextureContext) Creating texture context...");
		if ((m_spTC = MNew(CTextureContext)) == NULL) MemError("Init");
		m_spTC->Create(_TextureIDHeap, _TextureCacheHeap);
		MRTC_GOM()->RegisterObject((CReferenceCount*)m_spTC, "SYSTEM.TEXTURECONTEXT");
	}

	// ----------------------------------------------------------------
	// Create and register surface-context if class is available
	{
		MSCOPE(CSystemCore::CreateTextureContext, SYSTEMCORE_SC);
		LogFile("(CSystemCore::CreateTextureContext) Creating surface context...");
		MRTC_SAFECREATEOBJECT_NOEX(spSurfCtx, "CXR_SurfaceContext", CXR_SurfaceContext);
		if (spSurfCtx != NULL)
		{
			spSurfCtx->Create();
			m_spSurfCtx = spSurfCtx;
	//		LogFile("    ...successfull.");
			MRTC_GOM()->RegisterObject((CReferenceCount*)m_spSurfCtx, "SYSTEM.SURFACECONTEXT");
		}
		else
			LogFile("    ...not available.");
	}

	// ----------------------------------------------------------------
	// Create and register VB-context if class is available
	{
		MSCOPE(CSystemCore::CreateTextureContext, SYSTEMCORE_VBC);

		LogFile("(CSystemCore::CreateTextureContext) Creating VB context...");
		MRTC_SAFECREATEOBJECT_NOEX(spVBCtx, "CXR_VBContext", CXR_VBContext);
		if (spVBCtx != NULL)
		{
			spVBCtx->Create(GetEnvironment()->GetValuei("XR_MAXVERTEXBUFFERS", 8192));
			m_spVBCtx = spVBCtx;
	//		LogFile("    ...successfull.");
			MRTC_GOM()->RegisterObject((CReferenceCount*)m_spVBCtx, "SYSTEM.VBCONTEXT");
		}
		else
			LogFile("    ...not available.");
	}
}

// -------------------------------------------------------------------
void CSystemCore::System_Add(CSubSystem* _pSubSys)
{
	MAUTOSTRIP(CSystemCore_System_Add, MAUTOSTRIP_VOID);
	MSCOPE(CSystemCore::System_Add, SYSTEMCORE);
	
	{
		for(int i = 0; i < m_lpSubSystems.Len(); i++)
			if (m_lpSubSystems[i] == _pSubSys) return;
	}

	{
		for(int i = 0; i < m_lpSubSystems.Len(); i++)
			if (m_lpSubSystems[i] == NULL)
			{
				m_lpSubSystems[i] = _pSubSys;
				return;
			}
	}

	m_lpSubSystems.Add(_pSubSys);
}

void CSystemCore::System_Remove(CSubSystem* _pSubSys)
{
	MAUTOSTRIP(CSystemCore_System_Remove, MAUTOSTRIP_VOID);
	MSCOPE(CSystemCore::System_Remove, SYSTEMCORE);
	
	for(int i = 0; i < m_lpSubSystems.Len(); i++)
		if (m_lpSubSystems[i] == _pSubSys)
		{
			m_lpSubSystems[i] = NULL;
			return;
		}
}

void CSystemCore::System_BroadcastMessage(const CSS_Msg& _Msg)
{
	MAUTOSTRIP(CSystemCore_System_BroadcastMessage, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_lpSubSystems.Len(); i++)
	{
		if (m_lpSubSystems[i] != NULL)
			m_lpSubSystems[i]->OnMessage(_Msg);
	}
}

int CSystemCore::System_SendMessage(const char* _pSysName, const CSS_Msg& _Msg)
{
	MAUTOSTRIP(CSystemCore_System_SendMessage, 0);
	// The systems do not have names yet, so what do we do?...

	return 0;
}

aint CSystemCore::OnMessage(const CSS_Msg& _Msg)
{
	MAUTOSTRIP(CSystemCore_OnMessage, 0);
	// Be careful when sending message from this routine since everything will be echoed back.
/*	switch(_Msg.m_Msg)
	{
	case ? :
		{

		}
		break;
	}
*/
	return 0;
}

void CSystemCore::DoModal()
{
	MAUTOSTRIP(CSystemCore_DoModal, MAUTOSTRIP_VOID);
	if (m_spApp!=NULL) m_spApp->DoModal();
}

// -------------------------------------------------------------------
void CSystemCore::Parser_Quit()
{
	MAUTOSTRIP(CSystemCore_Parser_Quit, MAUTOSTRIP_VOID);
	m_bBreakRequested = TRUE;
};

void CSystemCore::Parser_CaptureScreenshot()
{
	MAUTOSTRIP(CSystemCore_Parser_CaptureScreenshot, MAUTOSTRIP_VOID);
#if !defined(M_RTM) || defined(M_Profile) || defined(PLATFORM_WIN)
	if(m_spDisplay != NULL)
		m_spDisplay->CaptureScreenshot();
#endif
}

void CSystemCore::Parser_CPU()
{
	MAUTOSTRIP(CSystemCore_Parser_CPU, MAUTOSTRIP_VOID);
	ConOutL(CStrF("Processor: %s", (char*) GetCPUName(true) ));
}

void CSystemCore::Parser_CPUEnable(int _Feature)
{
	MAUTOSTRIP(CSystemCore_Parser_CPUEnable, MAUTOSTRIP_VOID);
	EnableCPUFeatures(_Feature);
	ConOutL(CStrF("Processor: %s", (char*) GetCPUName(true) ));
}

void CSystemCore::Parser_CPUDisable(int _Feature)
{
	MAUTOSTRIP(CSystemCore_Parser_CPUDisable, MAUTOSTRIP_VOID);
	DisableCPUFeatures(_Feature);
	ConOutL(CStrF("Processor: %s", (char*) GetCPUName(true) ));
}

void CSystemCore::Parser_SysShowMemory()
{
	MAUTOSTRIP(CSystemCore_Parser_SysShowMemory, MAUTOSTRIP_VOID);
}

void CSystemCore::Parser_MemoryDebug(int _Flags)
{
	MAUTOSTRIP(CSystemCore_Parser_MemoryDebug, MAUTOSTRIP_VOID);
}

void CSystemCore::Parser_TestMemory()
{
	MAUTOSTRIP(CSystemCore_Parser_TestMemory, MAUTOSTRIP_VOID);
}
void CSystemCore::Parser_MT_Enable(int enable)
{
	MAUTOSTRIP(CSystemCore_Parser_MT_Enable, MAUTOSTRIP_VOID);
#ifdef M_SUPPORTMEMORYDEBUG
	if (enable)
		MRTC_GetMemoryManager()->MemTracking_Start();
	else
		MRTC_GetMemoryManager()->MemTracking_Stop();
#endif
}

void CSystemCore::Parser_MemoryCheckEnable(int On)
{
	MAUTOSTRIP(CSystemCore_Parser_MemoryCheckEnable, MAUTOSTRIP_VOID);
	m_bMemoryCheck = On != 0;
}

void CSystemCore::Parser_MemReport(int Verbose)
{
	MAUTOSTRIP(CSystemCore_Parser_MemReport, MAUTOSTRIP_VOID);
#ifdef M_SUPPORTMEMORYDEBUG
	MRTC_GetMemoryManager()->MemTracking_ReportMemUsage(Verbose != 0);
#endif
}

void CSystemCore::Parser_MemShowAllocs(CStr _Str)
{
	MAUTOSTRIP(CSystemCore_Parser_MemShowAllocs, MAUTOSTRIP_VOID);
#ifdef M_SUPPORTMEMORYDEBUG
	MRTC_GetMemoryManager()->ShowAllocs(_Str);
#endif
}

void CSystemCore::Parser_MemHideAllocs()
{
	MAUTOSTRIP(CSystemCore_Parser_MemHideAllocs, MAUTOSTRIP_VOID);
#ifdef M_SUPPORTMEMORYDEBUG
	MRTC_GetMemoryManager()->HideAllocs();
#endif
}

#ifdef PLATFORM_XBOX

	#include <xtl.h>

#endif
void CSystemCore::ExitProcess()
{
	MAUTOSTRIP(CSystemCore_ExitProcess, MAUTOSTRIP_VOID);
/*#ifdef PLATFORM_XBOX
//	XLaunchNewImage("D:\\default.xbe", NULL);
#elif defined(PLATFORM_WIN)
	m_bBreakRequested = true;
#endif*/
	m_bBreakRequested = true;
}

CStr CSystemCore::Parser_GetPlatformName()
{
	int Platform = D_MPLATFORM;
	switch(Platform)
	{
	case 0:
		return "";
	case 1:
		return "_Xbox";
	case 2:
		return "_GC";
	case 3:
		return "_PS2";
	case 4:
		return "_Xenon";
	case 5:
		return "_PS3";
	}

	return "Unknown";
}

CStr CSystemCore::Parser_GetExePath()
{
	return m_ExePath;
}

void CSystemCore::Register(CScriptRegisterContext & _RegContext)
{
	CSystem::Register(_RegContext);
	_RegContext.RegFunction("dc_set", this, &CSystemCore::DC_Set);
	_RegContext.RegFunction("dc_getn", this, &CSystemCore::DC_GetN);
	_RegContext.RegFunction("dc_getname", this, &CSystemCore::DC_GetName);
	_RegContext.RegFunction("dc_getdesc", this, &CSystemCore::DC_GetDesc);
	_RegContext.RegFunction("quit", this, &CSystemCore::Parser_Quit);
	_RegContext.RegFunction("capturescreen", this, &CSystemCore::Parser_CaptureScreenshot);
	_RegContext.RegFunction("cpu_updatefreq", this, &CSystemCore::InitCPUFrequencyConstant);
	_RegContext.RegFunction("cpu", this, &CSystemCore::Parser_CPU);
	_RegContext.RegFunction("cpu_disable", this, &CSystemCore::Parser_CPUDisable);
	_RegContext.RegFunction("cpu_enable", this, &CSystemCore::Parser_CPUEnable);
	_RegContext.RegFunction("sys_showmemory", this, &CSystemCore::Parser_SysShowMemory);
	_RegContext.RegFunction("sys_memorydebug", this, &CSystemCore::Parser_MemoryDebug);
	_RegContext.RegFunction("sys_testmemory", this, &CSystemCore::Parser_TestMemory);
	_RegContext.RegFunction("sys_memorytracking", this, &CSystemCore::Parser_MT_Enable);
	_RegContext.RegFunction("sys_memorycheckenable", this, &CSystemCore::Parser_MemoryCheckEnable);
	_RegContext.RegFunction("sys_memoryreport", this, &CSystemCore::Parser_MemReport);
	
	_RegContext.RegFunction("sys_exit", this, &CSystemCore::ExitProcess);

	_RegContext.RegFunction("sys_memoryshowallocs", this, &CSystemCore::Parser_MemShowAllocs);
	_RegContext.RegFunction("sys_memoryhideallocs", this, &CSystemCore::Parser_MemHideAllocs);


	_RegContext.RegFunction("GetPlatformName", this, &CSystemCore::Parser_GetPlatformName);
	_RegContext.RegFunction("GetExePath", this, &CSystemCore::Parser_GetExePath);
	
};

/*
void CSystemCore::ConsoleWrite(CStr _Str)
{
	MAUTOSTRIP(CSystemCore_ConsoleWrite, MAUTOSTRIP_VOID);
	if (m_spCon) m_spCon->Write(_Str);
}

void CSystemCore::ConsoleWriteL(CStr _Str)
{(
	MAUTOSTRIP(CSystemCore_ConsoleWriteL, MAUTOSTRIP_VOID);
	if (m_spCon) m_spCon->Write(_Str);
	LogFile(_Str);
}
*/
// -------------------------------------------------------------------
void CSystemCore::Refresh()
{
	MAUTOSTRIP(CSystemCore_Refresh, MAUTOSTRIP_VOID);
	MSCOPESHORT(CSystemCore::Refresh);
	if (m_RefreshRecursion) return;
	m_RefreshRecursion++;
	M_TRY
	{
		for(int i = 0; i < m_lpSubSystems.Len(); i++)
			if (m_lpSubSystems[i]) m_lpSubSystems[i]->OnRefresh(0);
	}
	M_CATCH(
	catch(CCException)
	{
		m_RefreshRecursion--;
		throw;
	}
	)
	m_RefreshRecursion--;

/*	if (!IsRendering())
		if (m_spDisplay) m_spDisplay->Win32_ProcessMessages();

	if (m_spDisplay && (m_iMainWindow >= 0)) 
		m_spWinInfo->m_hMainWnd = m_spDisplay->Win32_GethWnd(m_iMainWindow);
	else
		m_spWinInfo->m_hMainWnd = NULL;

	if (m_spInput) m_spInput->Update();*/
};

// -------------------------------------------------------------------
void CSystemCore::Render(CDisplayContext* _pDC, int _Context)
{
	MAUTOSTRIP(CSystemCore_Render, MAUTOSTRIP_VOID);
};

// -------------------------------------------------------------------
void CSystemCore::InitCPUFrequencyConstant()
{
	MAUTOSTRIP(CSystemCore_InitCPUFrequencyConstant, MAUTOSTRIP_VOID);
	MRTC_SystemInfo::MRTC_GetSystemInfo().CPU_MeasureFrequency();
	if (m_spCon!=NULL) 
	{
		ConOut(CStrF("CPU-Frequency: %.0f Mhz", MGetCPUFrequencyFp() / 1000000.0));
		ConOut(CStrF("OS-Frequency: %.0f Mhz", MGetOSFrequencyFp() / 1000000.0));
	}
};

CApplication* CSystemCore::GetApp()
{
	MAUTOSTRIP(CSystemCore_GetApp, NULL);
	return m_spApp;
}

void CSystemCore::DestroyApp()
{
	MAUTOSTRIP(CSystemCore_DestroyApp, MAUTOSTRIP_VOID);
	m_spApp = NULL;
}

CRegistry* CSystemCore::GetRegistry()
{
	MAUTOSTRIP(CSystemCore_GetRegistry, NULL);
	if (!m_spRegistry) Error("GetRegistry", "Registry not created.");
	return m_spRegistry;
}

CRegistry* CSystemCore::GetEnvironment()
{
	MAUTOSTRIP(CSystemCore_GetEnvironment, NULL);
	if (!m_spRegistry) Error("GetEnvironment", "Registry not created.");
	spCRegistry spEnv = m_spRegistry->Find("ENV");
	if (!spEnv) Error("GetEnvironment", "Environement directory not created.");
	return spEnv;
}

CRegistry* CSystemCore::GetOptions(bool _create)
{
	MAUTOSTRIP(CSystemCore_GetOptions, NULL);
	if (!m_spRegistry) Error("GetEnvironment", "Registry not created.");
	spCRegistry spEnv = m_spRegistry->Find("OPT");
	//if (!spEnv) Error("GetEnvironment", "Environement directory not created.");
	//spCRegistry spOpt = spEnv->Find("OPT");
	if(!spEnv && _create)
	{
		spEnv = m_spRegistry->CreateDir("OPT");
		spEnv = spEnv->MakeThreadSafe();
//		spEnv->EnableAutoHashing(true);
		//spEnv->Clear(); 
	}
	return spEnv;
}

int CSystemCore::ReadOptions()
{
	// 0 == Corrupt, 1 == Ok, 2 == NoFile

	// Option registry added to the environment
	if (CDiskUtil::FileExists(m_OptionsFilename))
	{
		M_TRY
		{
			m_spRegistry->ReadRegistryDir("OPT", m_OptionsFilename);
		}
		M_CATCH(
		catch(CCException)
		{
			return 0;
		}
		)
	}
	else
	{
		ConOutL("(CSystemCore::CreateSystems) " + m_OptionsFilename + " doesn't exist.");
		return 2;
	}

	return 1;
}

int CSystemCore::WriteOptions()
{
	// // 0 == Failed, 1 == Ok, 2 == Corrupt
		
	M_TRY
	{
		GetOptions();	// Make sure there is an "OPT" directory.
		m_spRegistry->WriteRegistryDir("OPT", m_OptionsFilename);
	}
	M_CATCH(
	catch(...)	
	{
		LogFile("(CSystemCore::WriteOptions) Error writing options " + m_OptionsFilename);
		CDiskUtil::DelFile(m_OptionsFilename);
		return 0;	
	}
	)
	return 1;
}


// -------------------------------------------------------------------
spCDisplayContextContainer CSystemCore::CreateDisplayContext(const char* _pName)
{
	MAUTOSTRIP(CSystemCore_CreateDisplayContext, NULL);
	MSCOPE(CSystemCore::CreateDisplayContext, SYSTEMCORE_DISPC);
	
	for(int i = 0; i < m_lspDC.Len(); i++)
		if (m_lspDC[i]->m_Name == _pName)
		{
			spCDisplayContextContainer spDCC = MNew1(CDisplayContextContainer, m_lspDC[i]);
			if (!spDCC) MemError("CreateDisplayContext");
			return spDCC;
		}

	Error("CreateDisplayContext", CStrF("A Display-context by the name %s was not found", _pName));
	return NULL;
}

void CSystemCore::DC_SetName(const char *_pClassName)
{

	// First try to match the class exactly
	for (int i = 0; i < m_lspDC.Len(); ++i)
	{
		if (m_lspDC[i]->m_DCClass.Compare(_pClassName) == 0)
		{
			DC_Set(i);
			return;
		}
	}    

	// Then search for name
	for (int i = 0; i < m_lspDC.Len(); ++i)
	{
		if (m_lspDC[i]->m_Name.Compare(_pClassName) == 0)
		{
			DC_Set(i);
			return;
		}
	}    

	Error("DC_Set", CStrF("A Display-context by the name %s was not found", _pClassName));
}

#if defined(PLATFORM_XENON)
CDisplayContext *gf_CreateDisplayContextStatic();
#elif defined(PLATFORM_PS3)
CDisplayContext* gf_CreateDisplayContextPS3Static();
CDisplayContext* gf_CreateDisplayContextPS3GCMStatic();
#endif

void CSystemCore::DC_SetPtr(spCReferenceCount _spDisplayContext)
{
	MAUTOSTRIP(CSystemCore_DC_Set, MAUTOSTRIP_VOID);
	MSCOPE(CSystemCore::DC_Set, SYSTEMWIN32);
	
	m_bIsRendering = true;
	
//	if (m_spWinInfo != NULL) m_spWinInfo->m_hMainWnd = NULL;

	if (m_iMainWindow >= 0)
	{
		m_spDisplay->DeleteWindow(m_iMainWindow);
		m_iMainWindow = -1;
	}

	m_spDisplay = NULL;
	if (m_iCurrentDC >= 0)
	{
		if (MRTC_GetObjectManager()->GetDllLoading())
		{
			if (m_lspDC[m_iCurrentDC]->m_FileName.Len())
				MRTC_GOM()->GetClassRegistry()->UnloadClassLibrary(m_lspDC[m_iCurrentDC]->m_FileName);
		}
	}

	m_iCurrentDC = -2;

	spCReferenceCount spObj = _spDisplayContext;

	m_spDisplay = safe_cast<CDisplayContext> ((CReferenceCount*)spObj);
	if (!m_spDisplay)
		Error("DC_Set", "Failed to create display-context.");
	
	m_spDisplay->Create();

	DC_InitVidModeFromEnvironment();
	m_bIsRendering = false;

	// Notify all subsystems
	System_BroadcastMessage(CSS_Msg(CSS_MSG_POSTCHANGEDISPLAYMODE));
}

void CSystemCore::DC_Set(int _nr)
{
	MAUTOSTRIP(CSystemCore_DC_Set, MAUTOSTRIP_VOID);
	MSCOPE(CSystemCore::DC_Set, SYSTEMWIN32);
	
	if (_nr == m_iCurrentDC) return;
	m_bIsRendering = true;
	
//	if (m_spWinInfo != NULL) m_spWinInfo->m_hMainWnd = NULL;

	if (m_iMainWindow >= 0)
	{
		m_spDisplay->DeleteWindow(m_iMainWindow);
		m_iMainWindow = -1;
	}

	m_spDisplay = NULL;
	if (m_iCurrentDC >= 0)
	{
		if (MRTC_GetObjectManager()->GetDllLoading())
		{
			if (m_lspDC[m_iCurrentDC]->m_FileName.Len())
				MRTC_GOM()->GetClassRegistry()->UnloadClassLibrary(m_lspDC[m_iCurrentDC]->m_FileName);
		}
	}

	m_iCurrentDC = _nr;

	if (m_iCurrentDC < 0)
	{
		m_bIsRendering = false;
		return;
	}
	if (m_iCurrentDC >= m_lspDC.Len())
		Error("DC_Set", "Invalid DC index.");

	ConOut("Initializing: " + m_lspDC[m_iCurrentDC]->m_Desc);

	if (MRTC_GetObjectManager()->GetDllLoading())
	{
		if (m_lspDC[m_iCurrentDC]->m_FileName.Len())
			MRTC_GOM()->GetClassRegistry()->LoadClassLibrary(m_lspDC[m_iCurrentDC]->m_FileName);
	}

#if defined(PLATFORM_XENON)
	spCReferenceCount spObj = gf_CreateDisplayContextStatic();
#elif defined(PLATFORM_PS3)
	#ifdef PS3_RENDERER_GCM
		spCReferenceCount spObj = gf_CreateDisplayContextPS3GCMStatic();
	#elif defined(PS3_RENDERER_PSGL)
		spCReferenceCount spObj = gf_CreateDisplayContextPS3Static();
	#endif
#else
	spCReferenceCount spObj = (CReferenceCount*) MRTC_GOM()->CreateObject(m_lspDC[m_iCurrentDC]->m_DCClass);
#endif

	m_spDisplay = safe_cast<CDisplayContext> ((CReferenceCount*)spObj);
	if (!m_spDisplay)
	{
		// Try to forceload dll
		if (m_lspDC[m_iCurrentDC]->m_FileName.Len())
		{
			M_TRY
			{
				MRTC_GOM()->GetClassRegistry()->LoadClassLibrary(m_lspDC[m_iCurrentDC]->m_FileName);

				spCReferenceCount spObj = (CReferenceCount*) MRTC_GOM()->CreateObject(m_lspDC[m_iCurrentDC]->m_DCClass);
				m_spDisplay = safe_cast<CDisplayContext> ((CReferenceCount*)spObj);
			}
			M_CATCH(
			catch(CCException)
			{}
			)
		}
	}
	if (!m_spDisplay)
		Error("DC_Set", "Failed to create display-context.");
	
	m_spDisplay->Create();

	DC_InitVidModeFromEnvironment();
//	m_spDisplay = m_lDCDesc[m_iCurrentDC]->pConstr(m_spCon);
//	if (m_spDisplay == NULL) MemError("DC_Set");
	m_bIsRendering = false;

	// Notify all subsystems
	System_BroadcastMessage(CSS_Msg(CSS_MSG_POSTCHANGEDISPLAYMODE));
};

int CSystemCore::DC_GetN()
{
	MAUTOSTRIP(CSystemCore_DC_GetN, 0);
	return m_lspDC.Len();
};

int CSystemCore::DC_Find(CStr _Name)
{
	MAUTOSTRIP(CSystemCore_DC_Find, 0);
	for(int i = 0; i < m_lspDC.Len(); i++)
		if (m_lspDC[i]->m_Name.CompareNoCase(_Name) == 0) return i;
	return -1;
}

CStr CSystemCore::DC_GetName(int _nr)
{
	MAUTOSTRIP(CSystemCore_DC_GetName, *((void*)NULL));
	return m_lspDC[_nr]->m_Name;
};

CStr CSystemCore::DC_GetDesc(int _nr)
{
	MAUTOSTRIP(CSystemCore_DC_GetDesc, *((CStr*)NULL));
	return m_lspDC[_nr]->m_Desc;
};

void CSystemCore::DC_InitVidModeFromEnvironment()
{
	MAUTOSTRIP(CSystemCore_DC_InitVidModeFromEnvironment, MAUTOSTRIP_VOID);
	MSCOPE(CSystemCore::DC_InitVidModeFromEnvironment, SYSTEMWIN32);
	
	CRegistry* pEnv = GetEnvironment();
#if defined(M_RTM) || defined(PLATFORM_CONSOLE)
#if defined(PLATFORM_XENON) || defined(PLATFORM_PS3)
	CStr Mode = pEnv->GetValue("VID_MODE", "1024 576 32 85");
#else
	CStr Mode = pEnv->GetValue("VID_MODE", "640 480 32 85");
#endif
#else
	CStr Mode = pEnv->GetValue("VID_MODE", "desktop");
#endif
	int iMode = 0;

	ConOutL("(CSystemCore) VID_MODE = " + Mode);

	if (Mode.CompareNoCase("desktop") == 0)
		iMode = 0;
	else
	{
		int width = Mode.GetStrMSep(", ").Val_int();
		int height = (Mode != "") ? Mode.GetStrMSep(", ").Val_int() : -1;
		int bpp = (Mode != "") ? Mode.GetStrMSep(", ").Val_int() : -1;
		int RefreshRate = (Mode != "") ? Mode.GetStrMSep(", ").Val_int() : -1;
		int format = (bpp != -1) ? CImage::BPP2Format(bpp) : -1;
		iMode = m_spDisplay->ModeList_Find(width, height, format, RefreshRate);
/*		if (iMode < 0) iMode = m_spDisplay->ModeList_Find(width, height, format, -1);
		if (iMode < 0) iMode = m_spDisplay->ModeList_Find(width, height, -1, -1);
		if (iMode < 0) iMode = m_spDisplay->ModeList_Find(width, -1, -1, -1);*/
		if (iMode < 0)
		{
			ConOut("(CSystemCore) Can't find video mode. Choosing mode 0.");
			iMode = 0;
		}
		else
			ConOutL(CStrF("(CSystemCore) Best match for VID_MODE %d,%d,%d,%d is mode %d", width, height, bpp, RefreshRate, iMode));
	}

	ConOutL(CStrF("(CSystemCore) Initializing video mode %d",iMode));
	m_spDisplay->SetMode(iMode);

}

void CSystemCore::DC_InitFromEnvironment()
{
	MAUTOSTRIP(CSystemCore_DC_InitFromEnvironment, MAUTOSTRIP_VOID);
	MSCOPE(CSystemCore::DC_InitFromEnvironment, SYSTEMWIN32);
	
	m_bIsRendering = true;
	CRegistry* pEnv = GetEnvironment();

#ifdef PLATFORM_XENON
	CStr DCName = "Xenon";
#elif defined (PLATFORM_XBOX)
	CStr DCName = "Xbox";
#elif defined (PLATFORM_PS3)
	CStr DCName = "PS3";
#else
	CStr DCName = pEnv->GetValue("VID_RENDER", "OpenGL");
#endif

	ConOutL("(CSystemCore) VID_RENDER = " + DCName);
	int iDC = DC_Find(DCName);
	if (iDC < 0) iDC = 0;
	pEnv->SetValue("VID_RENDER", (char*)DC_GetName(iDC));

	DC_Set(iDC);
	m_bIsRendering = false;
}

CStr CSystemCore::GetCaption()
{
	MAUTOSTRIP(CSystemCore_GetCaption, CStr());
	return "";
}

void CSystemCore::SetCaption(CStr _Caption)
{
	MAUTOSTRIP(CSystemCore_SetCaption, MAUTOSTRIP_VOID);
}


// -------------------------------------------------------------------
void SYSTEMDLLEXPORT ConsoleWrite(CStr _s)
{
	MAUTOSTRIP(ConsoleWrite, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
	if (pCon)
	{
		pCon->WriteExceptions();
		pCon->Write(_s);
	}
}

void SYSTEMDLLEXPORT ConsoleWriteL(CStr _s)
{
	MAUTOSTRIP(ConsoleWriteL, MAUTOSTRIP_VOID);
	LogFile(_s);
	MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
	if (pCon)
	{
		pCon->WriteExceptions();
		pCon->Write(_s);
	}
}

bool SYSTEMDLLEXPORT ConExecuteMTImp(CStr _Script, const char *_File, int _Line)
{
	MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
	if (!pCon) return false;
	bool bException = false;
	M_TRY
	{
		pCon->ExecuteString(_Script, _File, _Line);
	}
	M_CATCH(
	catch(CCException)
	{
		bException = true;
	}
	)
	return !bException;
}

bool SYSTEMDLLEXPORT ConExecuteImp(CStr _Script, const char *_File, int _Line)
{
	M_ASSERT((MRTC_GOM()->m_hScriptThread == 0) || (MRTC_SystemInfo::OS_GetThreadID() == MRTC_GOM()->m_hScriptThread), "Can only run scripts from scriptthread");
	return ConExecuteMTImp(_Script, _File, _Line);
}
