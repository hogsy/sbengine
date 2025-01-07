#include "PCH.h"
#include "WGameContext.h"

#include "../GameWorld/Server/WServer.h"
#include "../GameWorld/WMapData.h"
#include "../GameWorld/Client/WClient_Core.h"
#include "../GameWorld/WPackets.h"
#include "../../XRModels/Model_BSP/WBSPDef.h"					// Hmm...
#include "../GameWorld/WDataRes_MiscMedia.h"
#include "../GameWorld/FrontEnd/WFrontEnd.h"
#include "../Video/MVideo.h"

#ifndef	PLATFORM_CONSOLE
#include "../../SDK/Include/cdapfn.h" // SafeDisc
#endif

#if defined(PLATFORM_CONSOLE)
#include "../../../../Projects/Main/Exe/WGameContextMain.h"
#endif

#if defined(PLATFORM_XENON)
#include "../../../../Projects/Main/Exe_Xenon/Darkness.spa.h"
#endif

#if defined(MRTC_MEMORYDEBUG) && defined(PLATFORM_WIN)
#include "crtdbg.h"
#define CHECKMEMORY(s) { if (!_CrtCheckMemory()) Error(s, "Memory check failure."); }
#else
#define CHECKMEMORY(s)
#endif


#define INIT_PAD(n) uint8 Pad[n]; FillChar(&Pad, n, 0xaa); const int nPad = n;

#define CHECK_PAD(Name)	\
	LogFile( CStrF("CHECK_PAD: %s", (char*) #Name) );	\
	{ for(int i = 0; i < nPad; i++) if (Pad[i] != 0xaa) LogFile( CStrF("Pad failure at pos %.8x, address %.8x, Value %.2x", i, &Pad[i], Pad[i]) );	} \
	LogFile( "CHECK_PAD: done." );


//#pragma optimize("", off)
//#pragma inline_depth(0)





// change CCrcCheck::GetChecksumList if CHECKSUM_BYTELENGTH is changed
#ifdef PLATFORM_XBOX1
#define CHECKSUM_BYTELENGTH XCALCSIG_SIGNATURE_SIZE


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Class:   		Calculates checksums using CRC-16
\*____________________________________________________________________*/
class CCrcCheck
{
public:
	HANDLE m_hChecksum;

public:
	CCrcCheck();

	void UpdateCrc(const uint8 *_pData, int32 _Size);
	void UpdateCrc(int c);
	void GetChecksumList(uint8 _list[CHECKSUM_BYTELENGTH]);
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:   	Creates and initializes a CCrcCheck object
\*____________________________________________________________________*/
CCrcCheck::CCrcCheck()
{
	m_hChecksum = XCalculateSignatureBegin(XCALCSIG_FLAG_SAVE_GAME);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:   	Updates the checksum in reverse. Uses CRC-16.
\*____________________________________________________________________*/
void CCrcCheck::UpdateCrc(int c)
{
	XCalculateSignatureUpdate(m_hChecksum, (BYTE *)&c, sizeof(c));
}

void CCrcCheck::UpdateCrc(const uint8 *_pData, int32 _Size)
{
	XCalculateSignatureUpdate(m_hChecksum, _pData, _Size);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:   	Divides the checksum into a list of bytes 
\*____________________________________________________________________*/
void CCrcCheck::GetChecksumList(uint8 _List[CHECKSUM_BYTELENGTH])
{
	XCalculateSignatureEnd(m_hChecksum, _List);
}

#else
#define CHECKSUM_BYTELENGTH 4
#define MASK_CRC16			0xA001

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Class:   		Calculates checksums using CRC-16
\*____________________________________________________________________*/
class CCrcCheck
{
public:
	unsigned int m_nChecksum;

public:
	CCrcCheck();

	void UpdateCrc(const uint8 *_pData, int32 _Size);
	void UpdateCrc(int c);
	void GetChecksumList(uint8 _list[CHECKSUM_BYTELENGTH]);
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:   	Creates and initializes a CCrcCheck object
\*____________________________________________________________________*/
CCrcCheck::CCrcCheck()
{
#ifdef M_DEMO
	m_nChecksum = 0x4e01;
#else
	m_nChecksum = 0x0001;
#endif
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:   	Updates the checksum in reverse. Uses CRC-16.
\*____________________________________________________________________*/
void CCrcCheck::UpdateCrc(int c)
{
	for(int i = 0; i < 8; i++)
	{
		if((m_nChecksum ^ c) & 1) 
			m_nChecksum = (m_nChecksum>>1) ^ MASK_CRC16;
		else 
			m_nChecksum>>=1;

		c>>=1;
	}
}

void CCrcCheck::UpdateCrc(const uint8 *_pData, int32 _Size)
{
	for(int32 i = 0; i < _Size; i++)
		UpdateCrc(_pData[i]);

}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:   	Divides the checksum into a list of bytes 
\*____________________________________________________________________*/
void CCrcCheck::GetChecksumList(uint8 _List[CHECKSUM_BYTELENGTH])
{
	_List[0] = m_nChecksum;
	_List[1] = m_nChecksum >> 8;
	_List[2] = m_nChecksum >> 16;
	_List[3] = m_nChecksum >> 24;
}
#endif




/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CGameContext
|__________________________________________________________________________________________________
\*************************************************************************************************/

CGameContext::CGameContext()
{
	MAUTOSTRIP(CGameContext_ctor, MAUTOSTRIP_VOID);

	m_pSystem = NULL;
	m_bLoadedGameDll = false;
	m_nSplitScreens = 1;
	m_bSplitAxis = 0;
	m_hListenConnection = -1;
	m_hServerSocket = -1;
	m_Mode = 0;
	m_PauseCount = 0;
	m_bSingleStep = false;
	//	m_PauseTime = 0;
	m_bBooting = true;
	m_bTimeLeap = 0;
	m_bValidProfileLoaded = false;
	m_bSoakMode = false;
	m_nSoakCount = 0;


	m_TimeDemoRate = 1.0f / 30.0f;

	m_bStartCallGraphPending = false;
	m_bStopCallGraphPending = false;

	m_nTimeDemoFramesCaptured = 0;
	m_bBenchmark = false;

	m_CutsceneStatus = 0;

	/*	m_bShowFPS   = false;
	m_bSimpleFPS = false;
	m_bShowStats = false;*/

#ifdef WCLIENT_FIXEDRATE
	m_bSimulateFractional = 0;
#else
	m_bSimulateFractional = 1;
#endif

	m_VolumeAmbient = 1;
	m_VolumeSfx = 1;
	m_VolumeVoice = 1;
	m_VolumeFrontEnd = 1;

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys && pSys->GetEnvironment())
		m_Viewport3DScaleX = pSys->GetEnvironment()->GetValuef("VID_VIEWSCALEX", 1.0, 0);
	else
		m_Viewport3DScaleX = 1.0;

	if(pSys && pSys->GetEnvironment())
		m_Viewport3DScaleY = pSys->GetEnvironment()->GetValuef("VID_VIEWSCALEY", 1.0, 0);
	else
		m_Viewport3DScaleY = 1.0;


}


#if 0
void CWC_SoundMixSettings::ReadSettings(CRegistry *_pRegistry)
{
/*	for (int i = 0; i < _pRegistry->GetNumChildren(); ++i)
	{
		M_TRACEALWAYS("%s, %f\n", _pRegistry->GetChild(i)->GetThisName().Str(), _pRegistry->GetChild(i)->GetThisValuef());
	}*/

	m_2DRPGVolume = _pRegistry->GetValuef("SND_MIX_2DRPG", 0.8);
	m_2DGuiCubeVolume = _pRegistry->GetValuef("SND_MIX_2DGUICUBE", 0.9);
	m_2DCutsceneVolume = _pRegistry->GetValuef("SND_MIX_2DCUTSCENE", 1.0);
	m_2DGameVolume = _pRegistry->GetValuef("SND_MIX_2DGAME", 0.8);
	m_2DCGPlaySoundVolume = _pRegistry->GetValuef("SND_MIX_2DCGPLAYSOUND", 1.0);
	m_2DCGPlaySoundSfxVolVolume = _pRegistry->GetValuef("SND_MIX_2DCGPLAYSOUNDSFXVOL", 1.0);
	m_2DVideoVolume = _pRegistry->GetValuef("SND_MIX_2DVIDEO", 1.0);

	m_3DVideoVolume = _pRegistry->GetValuef("SND_MIX_3DVIDEO", 1.0);
	m_3DRPGVolume = _pRegistry->GetValuef("SND_MIX_3DRPG", 0.8);
	m_3DGameVolume = _pRegistry->GetValuef("SND_MIX_3DGAME", 0.7);
	m_3DGameLoopingVolume = _pRegistry->GetValuef("SND_MIX_3DGAMELOOPING", 1.0);

	for (int i = 0; i < 256; ++i)
	{
		m_MixCategories[i] = _pRegistry->GetValuef(CStrF("SND_CATEGORYVOL_%d",i).Str(), 1.0);
	}

	for (int i = 0; i < 256; ++i)
	{
		m_2DSoundNess[i] = _pRegistry->GetValuei(CStrF("SND_CATEGORYDIM_%d",i).Str(), 0);
	}
}
#endif

void CGameContext::Create(CStr _WorldPathes, CStr _GameName, spCXR_Engine _spEngine)
{
	m_spEngine = _spEngine;

	MACRO_GetSystem;
	{
		CStr WorldPath = _WorldPathes;
		WorldPath = WorldPath.GetStrSep(";");

		if (!CDiskUtil::DirectoryExists(WorldPath))
			WorldPath = pSys->m_ExePath + WorldPath;

		CStr XDFPath = WorldPath + "XDF\\GameContext_Create.XDF";


		if (D_MXDFCREATE == 2)
		{
			CStr XDFBase = pSys->GetEnvironment()->GetValue("XDF_PATH");
			if (XDFBase.Len() > 0)
				XDFPath = XDFBase + "\\GameContext_Create.XDL";
			else
				XDFPath = WorldPath + "XDF\\GameContext_Create.XDL";

			if (!CDiskUtil::FileExists(XDFPath))
			{
				CDiskUtil::XDF_Record(XDFPath, WorldPath);
			}
			else
			{
				ConOutL(CStrF("Skippng creation of XDF file, because it already exists (%s)", XDFPath.Str()));
			}
		}
		else if (CDiskUtil::FileExists(XDFPath))
		{
			CDiskUtil::XDF_Use(XDFPath, WorldPath);
		}
	}

	MAUTOSTRIP(CGameContext_Create, MAUTOSTRIP_VOID);
	MSCOPE(CGameContext::Create, GAMECONTEXT);

	//	m_WorldPath = _WorldPath;
	//	m_DefaultGameName = _GameName;
	AddToConsole();

	m_pSystem = pSys;
	//	m_spWData = DNew(CWorldData) CWorldData;
	//	m_spWData->Create(_WorldPath);

	CStr NetDevice = m_pSystem->GetEnvironment()->GetValue("NET_DEVICE", "UDP");
	m_NetDeviceSettings = m_pSystem->GetEnvironment()->GetValue("NET_ServerListen", "*:28000");


	if (NetDevice != "")
	{
		m_spNetDevice = (CNetwork_Device *)MRTC_GetObjectManager()->GetClassRegistry()->CreateObject(CStr("CNetwork_Device_") + NetDevice);
		if (m_spNetDevice)
		{
			MSCOPESHORT(Network);
			m_spNetwork = (CNetwork *)MRTC_GetObjectManager()->GetClassRegistry()->CreateObject(m_pSystem->GetEnvironment()->GetValue("NET_Context", "CNetworkCore"));

			if (m_spNetwork)
				m_spNetwork->Create(m_spNetDevice);
		}
	}

	if (!SetGame(_WorldPathes, _GameName))
		Error("Create", CStrF("Failed to initialize game '%s' at %s", (char*) _GameName, (char*) _WorldPathes));

	{
		MSCOPESHORT(FrontEnd);
		CreateClientGame();
	}
	//	m_pSystem->GetRegistry()->ReadRegistryDirectory(_GameName + "\\USER", m_WorldPath + "User.cfg");
	/*	m_bShowFPS			= m_pSystem->GetEnvironment()->GetValuei("CON_SHOWFPS", 0) != 0;
	m_bSimpleFPS		= m_pSystem->GetEnvironment()->GetValuei("CON_SIMPLEFPS", 0) != 0;
	m_bShowStats		= m_pSystem->GetEnvironment()->GetValuei("CON_SHOWSTAT", 0) != 0;*/
	CDiskUtil::XDF_Stop();
}

CGameContext::~CGameContext()
{
	MAUTOSTRIP(CGameContext_dtor, MAUTOSTRIP_VOID);
	MSCOPE(CGameContext::~CGameContext, GAMECONTEXT);

//	M_TRY
//	{
		m_spEngine = NULL;

		LogFile("(CGameContext::~CGameContext) Begin");
		//M_TRY{ 
			Command_Disconnect(); 
		//} M_CATCH(catch(CCException){};)

			LogFile("(CGameContext::~CGameContext) Destroying frontend...");
		//M_TRY 
		{ m_spFrontEnd = NULL; } 
		//M_CATCH(catch(CCException) {};)

			m_spWServer = NULL;
		m_lspWClients.Destroy();
		m_spGameReg = NULL;

		M_LOCK(m_PendingCommandLock);
		m_lPendingCommands.Destroy();


		LogFile("(CGameContext::~CGameContext) Destroying worlddata...");
		//M_TRY 
		{ m_spWData = NULL; } 
		//M_CATCH(catch(CCException) {};)

			LogFile("(CGameContext::~CGameContext) Destroying game...");
		//M_TRY 
		{ SetGame("","");} 
		//M_CATCH(catch(CCException) {};)	

			LogFile("(CGameContext::~CGameContext) Removing console...");
		//M_TRY 
		{ RemoveFromConsole(); } 
		//M_CATCH(catch(CCException) {};)

			LogFile("(CGameContext::~CGameContext) Destroying string tables...");
		//M_TRY 
		{ CreateStringTables(CStr()); } 
		//M_CATCH(catch(CCException) {};)

			LogFile("(CGameContext::~CGameContext) Done");

		//	g_pOS->m_spCon->RemoveSubSystem(this);
/*	}
	M_CATCH(
		catch(CCException)
	{
	}
	)*/
}

CStr CGameContext::ResolveFileName(CStr _FileName)
{
	MAUTOSTRIP(CGameContext_ResolveFileName, CStr());
	if (_FileName.GetDevice().Len())
		return _FileName;
	CStr TestPath;

	int nPathes = m_lWorldPathes.Len();
	for(int i = nPathes-1; i >= 0; i--)
	{
		TestPath = m_lWorldPathes[i] + _FileName;
		if(CDiskUtil::FileExists(TestPath))
			return TestPath;
	}

	return TestPath;	// Didn't find a file. Return the lowest priority path.
}

CStr CGameContext::ResolvePath(CStr _FileName)
{
	MAUTOSTRIP(CGameContext_ResolvePath, CStr());
	return ResolveFileName(_FileName).GetPath();
}

void CGameContext::ExecuteCommand(CGC_Command& _Cmd)
{
	MAUTOSTRIP(CGameContext_ExecuteCommand, MAUTOSTRIP_VOID);
	MSCOPESHORT(CGameContext::ExecuteCommand); //AR-SCOPE

	M_TRY
	{
		//		LogFile(CStrF("(CGameContext::ExecuteCommand) COMMAND %d, %s, %s, %s, %s", 
		//			_Cmd.m_Command, _Cmd.m_lParams[0].Str(), _Cmd.m_lParams[1].Str(), _Cmd.m_lParams[2].Str(), _Cmd.m_lParams[3].Str()));

		switch(_Cmd.m_Command)
		{
		case CGC_COMMAND_TIMELEAP :
			{
				M_TRACE("(CGameContextMod::ExecuteCommand) TimeLeap begin.\n");
				if (!m_bTimeLeap)
					break;
				bool bDestroySounds = (_Cmd.m_lParams[0] != "2") != 0;
				CWorld_Client* pC = GetCurrentClient();
				if (pC && bDestroySounds)
				{
					if(m_spSoundContext) m_spSoundContext->MultiStream_Pause();
					pC->Sound_SetSoundContext(NULL);
					CNetMsg OutPacket(WPACKET_PAUSE);
					OutPacket.AddInt8(1);
					pC->Net_OnMessage(OutPacket);
				}
				Simulate_Pause();


				if(m_spWServer)
					m_spWServer->m_bSkipSound = true;
				int nTimeLeap = 0;
				while(m_bTimeLeap && m_spWServer)
				{
					m_spWServer->Simulate(NULL);

					// Security measure to not be locked up in TimeLeap. 15 minutes is the limit
					nTimeLeap++;
					if(nTimeLeap > (15 * 60 * m_spWServer->GetGameTicksPerSecond()))
					{
						m_bTimeLeap = false;
						ConOutL("WARNING: Aborted abnormally long time-leap!");
					}
				}
				if(m_spWServer)
					m_spWServer->m_bSkipSound = false;
				Simulate_Resume();
				if (pC && bDestroySounds)
				{
					if(m_spSoundContext) m_spSoundContext->MultiStream_Resume();
					pC->Sound_SetSoundContext(m_spSoundContext);
					CNetMsg OutPacket(WPACKET_PAUSE);
					OutPacket.AddInt8(0);
					pC->Net_OnMessage(OutPacket);
				}
				M_TRACE("(CGameContextMod::ExecuteCommand) TimeLeap end.\n");
			};
			break;
		case CGC_COMMAND_DISCONNECT :
			{
				Command_Disconnect();
			}
			break;

		case CGC_COMMAND_MAP :
			{
				Command_Map(_Cmd.m_lParams[0]);
			}
			break;

		case CGC_COMMAND_CONNECT :
			{
				Command_Connect(_Cmd.m_lParams[0]);
			}
			break;

		case CGC_COMMAND_SETGAME :
			{
				Command_SetGame(_Cmd.m_lParams[0], _Cmd.m_lParams[1]);
			}
			break;

		case CGC_COMMAND_LOADGAME :
			{
				Command_LoadGame(_Cmd.m_lParams[0]);
			}
			break;

		case CGC_COMMAND_FREERESOURCES :
			{
				Command_FreeResources();
			}
			break;

		case CGC_COMMAND_SCRIPTGRABSCREEN :
			{
				m_spFrontEnd->GrabScreen();
				AddPendingCommand(CGC_Command(CGC_COMMAND_SCRIPT, _Cmd.m_lParams[0]));
			}
			break;

		case CGC_COMMAND_SCRIPT :
			{
				M_TRACE(CStrF("Executing '%s'\r\n", _Cmd.m_lParams[0].GetStr()));
				Command_Script(_Cmd.m_lParams[0]);
			}
			break;

		case CGC_COMMAND_CHANGEMAP :
			{
				_Cmd.m_Param.Clear();
				Command_ChangeMap(_Cmd.m_lFParam[0], _Cmd.m_lFParam[1]);
			}
			break;

		case CGC_COMMAND_PLAYCUTSCENESOUND:
			{
				Command_PlayCutsceneSound(_Cmd.m_lFParam[0], _Cmd.m_lFParam[1]);
			}
			break;

		default :
			Error("ExecuteCommand", CStrF("Invalid command %d, %s, %s, %s, %s", _Cmd.m_Command,
				_Cmd.m_lParams[0].Str(), _Cmd.m_lParams[1].Str(), _Cmd.m_lParams[2].Str(), _Cmd.m_lParams[3].Str()));
		}
	}
	M_CATCH(
		catch(CCException)
	{
		LogFile(CStrF("§cf80WARNING: Exception during execution of command %d, %s, %s, %s, %s", _Cmd.m_Command,
			_Cmd.m_lParams[0].Str(), _Cmd.m_lParams[1].Str(), _Cmd.m_lParams[2].Str(), _Cmd.m_lParams[3].Str()));
	}
	)
}

void CGameContext::AddPendingCommand(const CGC_Command& _Cmd)
{
	MAUTOSTRIP(CGameContext_AddPendingCommand, MAUTOSTRIP_VOID);
	LogFile(CStrF("Cmd %d, Arg '%s'", _Cmd.m_Command, _Cmd.m_lParams[0].GetStr()));
	M_LOCK(m_PendingCommandLock);
	m_lPendingCommands.Add(_Cmd);
}

void CGameContext::FlushPendingCommands()
{
	MAUTOSTRIP(CGameContext_FlushPendingCommands, MAUTOSTRIP_VOID);
	M_LOCK(m_PendingCommandLock);
	int WantedLen = 0;
	while(m_lPendingCommands.Len() > WantedLen)
	{
		CGC_Command Cmd = m_lPendingCommands[0];
		if (Cmd.m_Command == CGC_COMMAND_SCRIPTGRABSCREEN)
			++WantedLen;
		m_lPendingCommands.Del(0);

		{
			M_UNLOCK(m_PendingCommandLock);
			ExecuteCommand(Cmd);
		}
	}
}


bool CGameContext::SetGame(CStr _WorldPathes, CStr _GameName)
{
	MAUTOSTRIP(CGameContext_SetGame, false);
	MSCOPE(CGameContext::SetGame, GAMECONTEXT);

	if (_GameName.CompareNoCase(m_GameName) == 0 &&
		_WorldPathes.CompareNoCase(m_WorldPathesRel) == 0) return true;

#ifndef WCLIENT_STATICLIBRARY
	// Unload previously loaded game
	if (m_GameName != "")
	{
		m_spFrontEnd = NULL;
		m_spWData = NULL;
		if (m_Mode & WMODE_GAMELOADED)
		{
			ConOutL(CStrF("(CGameContext::SetGame) Closing game '%s'", (char*)m_GameName));
			// Save user-settings if they exist
			/*			Profile/Options replaces the need for a user.cfg
			try
			{
			if (m_pSystem->GetRegistry()->Find(m_GameName + "\\USER"))
			m_pSystem->GetRegistry()->WriteRegistryDir(m_GameName + "\\USER", ResolveFileName("User.cfg"));
			}
			catch (CCException)
			{

			}*/

			// Destroys the game's registry FIRST, extremely important since part of registry's 
			// destructors points into the game-DLL, which is beeing unloaded the next line.
			m_spGameReg = NULL;
			m_pSystem->GetRegistry()->DeleteDir(m_GameName);
			ConOutL(CStrF("(CGameContext::SetGame) Unloading game DLL '%s'", (char*) m_DLLName));

			if (m_bLoadedGameDll)
			{
				m_bLoadedGameDll = false;
				if (MRTC_GetObjectManager()->GetDllLoading())
				{
					MRTC_GOM()->GetClassRegistry()->UnloadClassLibrary(m_DLLName);
				}
			}
		}
		m_Mode &= ~WMODE_GAMELOADED;
		m_GameName = "";
	}
#endif

	// Clear base directory
	m_lWorldPathes.Clear();
	m_WorldPathesRel = "";
	m_WorldPathesAbs = "";

	// Null?
	if (_GameName == "") return true;

	// Set new base directory
	CStr Pathes = _WorldPathes;
	while(Pathes != "")
	{
		CStr Path = Pathes.GetStrSep(";");

		if (Path.Right(1) != "\\")
			Path += "\\";

		if (m_lWorldPathes.Len())
			m_WorldPathesRel += ";";
		m_WorldPathesRel += Path;


		if (Path.GetDevice() == "")
			Path = m_pSystem->m_ExePath + Path;

		if (m_lWorldPathes.Len())
			m_WorldPathesAbs += ";";
		m_WorldPathesAbs += Path;

		m_lWorldPathes.Add(Path);
	}

	if (!m_lWorldPathes.Len())
		Error("Create", "WORLDPATH did not contain a path.");

	CStr LocalizeSuffix;
	{
		// Add auto-detected content-directories

		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if (!pSys) Error("Create", "No system.");

		CStr aSubContentPaths[32];
		int32 NumPaths = 0;

#if defined(PLATFORM_XBOX1) && !defined(M_DEMO_XBOX)
		aSubContentPaths[NumPaths++] = "T:\\$c\\";
#endif

		aSubContentPaths[NumPaths++] = "ExtraContent\\";
		if(_GameName == "P6")
			aSubContentPaths[NumPaths - 1] = "P6_ExtraContent\\";

#ifdef PLATFORM_XBOX1
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

		if(LocalizeSuffix != "")
		{
			aSubContentPaths[NumPaths++] = "ExtraContent_" + LocalizeSuffix + "\\";

			int nWorldPathes = m_lWorldPathes.Len();
			for(int i = 0; i < nWorldPathes; i++)
			{
				// Scan for localized versions of original content directories

				CStr St = m_lWorldPathes[i];
				if(St.CompareSubStr(pSys->m_ExePath) == 0)
					St = St.Copy(pSys->m_ExePath.Len(), 1024);
				int nSt = St.Len();
				if(nSt > 0)
				{
					if(St[nSt - 1] == '\\')
						St = St.Copy(0, nSt - 1);
				}

				CStr Path = St + "_" + LocalizeSuffix + "\\";
				if(CDiskUtil::DirectoryExists(pSys->m_ExePath + Path))
				{
					if (m_lWorldPathes.Len())
						m_WorldPathesRel += ";";
					m_WorldPathesRel += Path;
					if (m_lWorldPathes.Len())
						m_WorldPathesAbs += ";";
					Path = pSys->m_ExePath + Path;
					m_WorldPathesAbs += Path;
					m_lWorldPathes.Add(Path);
				}
			}
		}

		for(int32 i = 0; i < NumPaths; i++)
		{
			CStr ExtraContentPath = aSubContentPaths[i];
			if(ExtraContentPath != "" && CDiskUtil::DirectoryExists(pSys->m_ExePath + ExtraContentPath))
			{
				CDirectoryNode Node;
				Node.ReadDirectory(pSys->m_ExePath + ExtraContentPath + "*");
				for(int f = 0; f < Node.GetFileCount(); f++)
					if(Node.IsDirectory(f))
					{
						if(Node.GetFileName(f) != "." && Node.GetFileName(f) != "..")
						{
							CStr Path = ExtraContentPath + Node.GetFileName(f) + "\\";
							if (m_lWorldPathes.Len())
								m_WorldPathesRel += ";";
							m_WorldPathesRel += Path;
							if (m_lWorldPathes.Len())
								m_WorldPathesAbs += ";";
							Path = pSys->m_ExePath + Path;
							m_WorldPathesAbs += Path;
							m_lWorldPathes.Add(Path);
						}
					}
			}
		}
	}

	//	m_WorldPath = m_lWorldPathes[0];

	// -------------------------------------------------------------------
	// Initialize registry defines
	M_TRY
	{ 
		m_pSystem->GetRegistry()->DeleteDir("GAMECONTEXT\\REGISTRYDEFINES");
		CRegistry* pRegDef = m_pSystem->GetRegistry()->CreateDir("GAMECONTEXT\\REGISTRYDEFINES");
		if (pRegDef)
		{
			if (LocalizeSuffix != "")
				pRegDef->AddKey(CStrF("LANGUAGE_%s", LocalizeSuffix.UpperCase().Str()), (char*)NULL);
			else
				pRegDef->AddKey("LANGUAGE_ENG", (char*)NULL);

#ifdef M_RTM
			pRegDef->AddKey("M_RTM", (char*)NULL);
#endif
#ifdef _DEBUG
			pRegDef->AddKey("_DEBUG", (char*)NULL);
#endif
#ifdef PLATFORM_CONSOLE
			pRegDef->AddKey("PLATFORM_CONSOLE", (char*)NULL);
#endif
#ifdef PLATFORM_WIN
			pRegDef->AddKey("PLATFORM_WIN", (char*)NULL);
#endif
#ifdef PLATFORM_WIN_PC
			pRegDef->AddKey("PLATFORM_WIN_PC", (char*)NULL);
#endif
#ifdef PLATFORM_WIN32
			pRegDef->AddKey("PLATFORM_WIN32", (char*)NULL);
#endif
#ifdef PLATFORM_WIN32_PC
			pRegDef->AddKey("PLATFORM_WIN32_PC", (char*)NULL);
#endif
#ifdef PLATFORM_WIN64
			pRegDef->AddKey("PLATFORM_WIN64", (char*)NULL);
#endif
#ifdef PLATFORM_WIN64_PC
			pRegDef->AddKey("PLATFORM_WIN64_PC", (char*)NULL);
#endif
#ifdef PLATFORM_XBOX
			pRegDef->AddKey("PLATFORM_XBOX", (char*)NULL);
#endif
#ifdef PLATFORM_XBOX1
			pRegDef->AddKey("PLATFORM_XBOX1", (char*)NULL);
#endif
#ifdef PLATFORM_XENON
			pRegDef->AddKey("PLATFORM_XENON", (char*)NULL);
#endif
#ifdef PLATFORM_PS2
			pRegDef->AddKey("PLATFORM_PS2", (char*)NULL);
#endif
#ifdef PLATFORM_DOLPHIN
			pRegDef->AddKey("PLATFORM_DOLPHIN", (char*)NULL);
#endif
		}
	}
	M_CATCH(
		catch(CCException)
	{
		try
		{
			m_pSystem->GetRegistry()->DeleteDir("GAMECONTEXT\\REGISTRYDEFINES");
		}
		catch(CCException)
		{
		}
		m_spGameReg = NULL;
		return false;
	}
	)


		// -------------------------------------------------------------------
		// Load game-registry
		ConOutL(CStrF("(CGameContext::SetGame) Loading game '%s'", (char*) _GameName));

	M_TRY
	{ 
		CreateGameRegistry(_GameName);

		ConOutL("GAMENAME " + _GameName);
		ConOutL("WORLDPATH " + m_WorldPathesAbs);
		ConOutL("WORLDPATHREL " + m_WorldPathesRel);

		CreateStringTables(m_spGameReg->GetValue("STRINGTABLES"));
	}
	M_CATCH(
		catch(CCException)
	{
		m_spGameReg = NULL;
		m_pSystem->GetRegistry()->DeleteDir(_GameName);	// Clean up just in case.
		return false;
	}
	)

#ifndef WCLIENT_STATICLIBRARY
		// Load main game-DLL
		MACRO_GetSystem;
	m_DLLName = ResolveFileName(pSys->GetModuleName(m_spGameReg->GetValue("DLL", _GameName), true));

	M_TRY
	{ 
		if (!MRTC_GOM()->GetClassRegistry()->GetRuntimeClass("CWorld_Server"))
		{
			m_bLoadedGameDll = true;
			if (MRTC_GetObjectManager()->GetDllLoading())
			{
				ConOutL(CStrF("(CGameContext::SetGame) Loading game DLL '%s'", (char*) m_DLLName));
				MRTC_GOM()->GetClassRegistry()->LoadClassLibrary(m_DLLName);
			}
		}
	}
	M_CATCH(
		catch(CCException)
	{
		m_spGameReg = NULL;
		m_pSystem->GetRegistry()->DeleteDir(_GameName);	// Clean up just in case.
		return false;
	}
	)
#endif
		m_Mode |= WMODE_GAMELOADED;

	// Create resource manager
	CreateResourceManager();
	if (!m_spWData)
	{
		if (m_bLoadedGameDll)
		{
			m_bLoadedGameDll = false;
			if (MRTC_GetObjectManager()->GetDllLoading())
			{
				MRTC_GOM()->GetClassRegistry()->UnloadClassLibrary(m_DLLName);
			}
		}
		m_Mode &= ~WMODE_GAMELOADED;
		m_spGameReg = NULL;
		m_pSystem->GetRegistry()->DeleteDir(_GameName);	// Clean up just in case.
		return false;
	}

	// Get some settings from the game-registry
	m_GameName = _GameName;
	m_ServerClass = m_spGameReg->GetValue("SERVERCLASS", "CWorld_Server");
	m_ClientClass = m_spGameReg->GetValue("CLIENTCLASS", "CWorld_Client");
	m_ClientGameClass = m_spGameReg->GetValue("CLIENTGAMECLASS", "CWClientGame");
	return true;
}

void CGameContext::CreateResourceManager()
{
	MAUTOSTRIP(CGameContext_CreateResourceManager, MAUTOSTRIP_VOID);
	MSCOPE(CGameContext::CreateResourceManager, GAMECONTEXT);

	CStr RcClass = m_spGameReg->GetValue("RESOURCECLASS", "CWorldData");

	ConOutL(CStrF("(CGameContext::SetGame) Creating resource-manager '%s'", (char*) RcClass));

	TPtr<CReferenceCount> spObj = (CReferenceCount*) MRTC_GOM()->GetClassRegistry()->CreateObject(RcClass);
	m_spWData = safe_cast<CWorldData>((CReferenceCount*)spObj);
	if(m_spWData)
		m_spWData->Create(m_spGameReg);
}

void CGameContext::CreateGameRegistry(CStr _GameName)
{
	MAUTOSTRIP(CGameContext_CreateGameRegistry, MAUTOSTRIP_VOID);
	MSCOPE(CGameContext::CreateGameRegistry, GAMECONTEXT);

	M_TRY
	{
		TArray<CStr> lDefines;
		MACRO_GetSystemRegistry(pReg);
		CRegistry* pRegDef = (pReg) ? pReg->GetDir("GAMECONTEXT\\REGISTRYDEFINES") : NULL;
		if (pRegDef)
		{
			for(int i = 0; i < pRegDef->GetNumChildren(); i++)
				lDefines.Add(pRegDef->GetName(i));
		}

		if (CDiskUtil::FileExists(ResolveFileName(_GameName + ".xrg")))
		{
			m_spGameReg = m_pSystem->GetRegistry()->CreateDir(_GameName);
			m_spGameReg->XRG_Read(ResolveFileName(_GameName + ".xrg"), lDefines);
			m_spGameReg = m_spGameReg->MakeThreadSafe();
		}
		else
		{
			m_pSystem->GetRegistry()->ReadRegistryDir(_GameName, ResolveFileName(_GameName + ".cfg"));
			m_spGameReg = m_pSystem->GetRegistry()->Find(_GameName);
			if (m_spGameReg)
				m_spGameReg = m_spGameReg->MakeThreadSafe();
		}
		if (!m_spGameReg) Error("SetGame", "No game-registry.");

		// Load user settings
		//m_spGameReg->ReadRegistryDir("USER", ResolveFileName("User.cfg"));//This causes "internal com,piler error
		CStr File = ResolveFileName("User.cfg");
		m_spGameReg->ReadRegistryDir("USER", File);
	}
	M_CATCH(
		catch(CCException)
	{
		m_pSystem->GetRegistry()->DeleteDir(_GameName);	// Clean up just in case.
		throw;
	}
	)

		m_spGameReg->SetValue("GAMENAME", (char*)_GameName);
	m_spGameReg->SetValue("WORLDPATH", (char*)m_WorldPathesAbs);
	m_spGameReg->SetValue("WORLDPATHREL", (char*)m_WorldPathesRel);
	// Get cachepath from environment so it's easier to override.
	//m_spGameReg->SetValue("CACHEPATH", m_pSystem->GetEnvironment()->GetValue("CACHEPATH"));
}

void CGameContext::CreateStringTables(CStr _StringTables)
{
	MAUTOSTRIP(CGameContext_CreateStringTables, MAUTOSTRIP_VOID);
	MSCOPE(CGameContext::CreateStringTables, GAMECONTEXT);

	// _StringTables == List of string table registries
	// i.e  "Registry\StringTable_eng.txt;Registry\StringTable_swe.txt"

	if(!m_pSystem)
		return;

	CRegistry* pSysST = m_pSystem->GetRegistry()->CreateDir("STRINGTABLES");
	pSysST->SetNumChildren(0);
	pSysST->SetNumChildren(1);
	pSysST->RenameKey(0, "DYNAMIC");
	pSysST->SimulateRegistryCompiled(true);

#ifdef M_ENABLE_REGISTRYCOMPILED
	m_lspRegCompiledStringTables.Destroy();
#endif

	TArray<CStr> lDefines;
	MACRO_GetSystemRegistry(pReg);
	CRegistry* pRegDef = (pReg) ? pReg->GetDir("GAMECONTEXT\\REGISTRYDEFINES") : NULL;
	if (pRegDef)
	{
		for(int i = 0; i < pRegDef->GetNumChildren(); i++)
			lDefines.Add(pRegDef->GetName(i));
	}

	while(_StringTables != "")
	{
		CStr RegName = _StringTables.GetStrSep(";");
		CStr XCRName = RegName.GetPath() + RegName.GetFilenameNoExt() + ".XCR";
		CStr XCRFileName = ResolveFileName(XCRName);

		spCRegistry spST;
#ifdef M_ENABLE_REGISTRYCOMPILED
		if (CDiskUtil::FileExists(XCRFileName))
		{
			TPtr<CRegistryCompiled> spXCR = MNew(CRegistryCompiled);
			if (!spXCR)
				MemError("CreateStringTables");
			{
				//				D_NOXDF;
				spXCR->Read_XCR(XCRFileName);
			}
			spST = spXCR->GetRoot();
#if defined( PLATFORM_PS2 ) || defined( PLATFORM_DOLPHIN )
			spST->Sort();
#endif
			m_lspRegCompiledStringTables.Add(spXCR);
		}
		else
#endif
		{
			CStr File = ResolveFileName(RegName);

			spST = REGISTRY_CREATE;
			if (!spST)
				MemError("CreateStringTables");

			spST->XRG_Read(File, lDefines);
			spST->SimulateRegistryCompiled(true);
		}

		if (spST != NULL)
		{
			for(int i = 0; i < spST->GetNumChildren(); i++)
			{
				spCRegistry spReg = spCRegistry(spST->GetChild(i));
				//				spReg->EnableAutoHashing(false);
				pSysST->AddReg(spReg);
			}
		}
	}
}

bool CGameContext::CreateClientGame()
{
	MAUTOSTRIP(CGameContext_CreateClientGame, false);
	MSCOPE(CGameContext::CreateClientGame, GAMECONTEXT);

	if (m_spFrontEnd != NULL)
		return true;

	spCReferenceCount spObj = (CReferenceCount*) MRTC_GOM()->CreateObject(m_ClientGameClass);
	m_spFrontEnd = safe_cast<CWFrontEnd>((CReferenceCount*)spObj);

	if (!m_spFrontEnd)
	{
		ConOut(CStrF("Unable to create client-game '%s', GUI unavailable.", (char*)m_ClientGameClass));
		return false;
	}
	M_TRY
	{
		CWF_CreateInfo CreateInfo;
		CreateInfo.m_pGameContext = this;
		CreateInfo.m_spNetwork = m_spNetwork;
		CreateInfo.m_spSoundContext = m_spSoundContext;
		m_spFrontEnd->Create(CreateInfo);
	}
	M_CATCH(
		catch(CCException)
	{
		ConOut(CStrF("Failure initializing client-game '%s', GUI unavailable.", (char*)m_ClientGameClass));
		m_spFrontEnd = NULL;
		return false;
	}
	)

		m_bBooting = false;

	return true;
}

void CGameContext::ChangeWorld(CStr _FileName, int _Flags)
{
	MAUTOSTRIP(CGameContext_ChangeWorld, MAUTOSTRIP_VOID);
	MSCOPE(CGameContext::ChangeWorld, GAMECONTEXT);

	if (_FileName.GetDevice() == "")
		_FileName = ResolveFileName("WORLDS\\" + _FileName);
	if (_FileName.GetFilenameExtenstion() == "")
		_FileName = _FileName + ".XW";

	LogFile("ChangeWorld: " + _FileName);
	ConOut("ChangeWorld: " + _FileName);

	if (m_spWServer == NULL) Error("ChangeWorld", "No server.");

	MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
#ifndef PLATFORM_WIN_PC
	if (pCon) pCon->ExecuteString("stream_stop(1)");
#endif
	m_spWServer->World_Change(_FileName, _Flags);

	if (pCon) pCon->ExecuteString("cg_clearmenus()");
}

void CGameContext::InitServer(bint _bInitNet, CStr _Settings)
{
	MAUTOSTRIP(CGameContext_InitServer, MAUTOSTRIP_VOID);
	MSCOPE(CGameContext::InitServer, GAMECONTEXT);

	Command_Disconnect();
	CHECKMEMORY("InitServer");

	ConOutL("InitServer (" + _Settings + ")");

	m_Mode = WMODE_SERVER | WMODE_LOCALCLIENT | WMODE_GAMELOADED;

	ConOutL("Creating game server: " + m_ServerClass);
	spCReferenceCount spObj = (CReferenceCount*) MRTC_GOM()->CreateObject(m_ServerClass);
	m_spWServer = safe_cast<CWorld_Server> ((CReferenceCount*)spObj);
	if (!m_spWServer) Error("InitServer", "Unable to create server-object.");
	m_spWServer->Create(0, m_spGameReg, m_spWData);

	if (_bInitNet)
	{
		ConOutL("Creating network device");

		if (!m_spNetwork && m_spNetDevice)  
		{
			MSCOPESHORT(Network);
			m_spNetwork = (CNetwork *)MRTC_GetObjectManager()->GetClassRegistry()->CreateObject(m_pSystem->GetEnvironment()->GetValue("NET_Context", "CNetworkCore"));

			if (!m_spNetwork) 
			{
				ConOutL("(CGameContext::InitServer) No network");
				return;
			}

			m_spNetwork->Create(m_spNetDevice);
		}
		else if (!m_spNetwork)
		{
			ConOutL("(CGameContext::InitServer) No network device");
			return;
		}

		m_spWServer->m_spNetwork = m_spNetwork;
		m_spWServer->m_hServerSocket = -1;

		M_TRY
		{
			CNetwork_Address *pAddr = m_spWServer->m_spNetwork->Global_ResolveAddress(_Settings);
			if (!pAddr)
			{
				ConOut("Failed resolve server listen address.");
				return;
			}

			m_hServerSocket = m_spWServer->m_spNetwork->Server_Listen(pAddr, NULL);
			pAddr->Delete();
			if (m_hServerSocket < 0)
			{
				ConOut("Failed to create server listen socket.");				
				return;
			}


		}
		M_CATCH(
			catch(CCException)
		{
			ConOut("Failed initializing TCP/IP server.");
		}
		)
			m_spWServer->m_hServerSocket = m_hServerSocket;

		CRegistry* pUser = m_spGameReg->CreateDir("USER");

		if (!pUser->FindChild("SERVERNAME"))
			pUser->SetValue("SERVERNAME", (char*) (pUser->GetValue("NAME", "Noname") + "'s server"));

		CStr ServerName = pUser->GetValue("SERVERNAME", "Noname's server");

		m_spWServer->m_ServerName = ServerName;
		CStr SvInfo = m_GameName + "|" + m_WorldPathesRel + "|Game:No game.";
		CNetwork_ServerInfo Info(ServerName, SvInfo.Str(), SvInfo.Len());

		m_spWServer->m_spNetwork->Server_SetInfo(m_hServerSocket, Info);

		m_spWServer->m_PlayMode = SERVER_PLAYMODE_DEMO;
	}

	CHECKMEMORY("InitServer (Done)");
}

void CGameContext::DestroyClients()
{
	MAUTOSTRIP(CGameContext_DestroyClients, MAUTOSTRIP_VOID);
	MSCOPE(CGameContext::DestroyClients, GAMECONTEXT);

	for(int i = m_lspWClients.Len()-1; i >= 0; i--)
		m_lspWClients[i] = NULL;
	m_lspWClients.Clear();
}

void CGameContext::InitLocalGame()
{
	MAUTOSTRIP(CGameContext_InitLocalGame, MAUTOSTRIP_VOID);
	MSCOPE(CGameContext::InitLocalGame, GAMECONTEXT);

	CHECKMEMORY("InitLocalGame");

	ConOutL("InitLocalGame...");

	if (!m_spWServer)
	{
		InitServer();
		if (!m_spWServer)
			return;
	}

	ConOutL("CreateLocalClient...");

	DestroyClients();

	// Create clients
	for(int i = 0; i < m_nSplitScreens; i++)
		Con_AddLocalClient(m_nSplitScreens > 1);

	CHECKMEMORY("InitLocalGame (Done)");
	StartSimulation();
}

void CGameContext::Disconnect(bool _bDestroyNet)
{
	MAUTOSTRIP(CGameContext_Disconnect, MAUTOSTRIP_VOID);
	MSCOPE(CGameContext::Disconnect, GAMECONTEXT);

	ConOutL("Disconnecting...");

	MACRO_GetRegisterObject(CXR_VBMContainer, pVBMContainer, "SYSTEM.VBMCONTAINER");
	if(pVBMContainer)
	{
		pVBMContainer->BlockUntilAllVBMFree();
	}

	StopSimulation();

	OnDisconnect();

#ifndef PLATFORM_CONSOLE
	ConExecute("videofileend()");
#endif
	m_TimeDemoCapturePath = "";

	{
		for(int i = m_lspWClients.Len()-1; i >= 0; i--)
		{
			LogFile("(CGameContext::Con_Disconnect) Destroying client...");
			m_lspWClients[i] = NULL;
		}
		m_lspWClients.Clear();
	}

	if(m_spWServer != NULL)
	{
		m_spWServer->PreDisconnect();
		LogFile("(CGameContext::Con_Disconnect) Destroying server...");
	}
	M_LOCK(m_ServerDeleteLock);
	m_spWServer = NULL;


	if (m_spNetwork != NULL && m_hListenConnection >= 0)
	{
		m_spNetwork->Connection_Close(m_hListenConnection);
		m_hListenConnection = -1;
	}

	if (m_spNetwork != NULL && m_hServerSocket >= 0)
	{
		m_spNetwork->Connection_Close(m_hServerSocket);
		m_hServerSocket = -1;
	}	

	if(_bDestroyNet && m_spNetwork != NULL)
	{
		//		LogFile("(CGameContext::Con_Disconnect) Destroying network...");
		m_spNetwork = NULL;
	}

	/*
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (pTC)
	{
	int nTxt = pTC->GetIDCapacity();
	for(int i = 0; i < nTxt; i++)
	{
	int Flags = pTC->GetTextureFlags(i);
	if ((Flags & CTC_TXTIDFLAGS_PRECACHE) && !(Flags & CTC_TXTIDFLAGS_RESIDENT))
	pTC->SetTextureParam(i, CTC_TEXTUREPARAM_CLEARFLAGS, CTC_TXTIDFLAGS_PRECACHE);
	}
	}

	MACRO_GetRegisterObject(CWaveContext, pWC, "SYSTEM.WAVECONTEXT");
	if (pWC)
	{
	pWC->ClearWaveIDFlags(CWC_WAVEIDFLAGS_PRECACHE);
	}*/
	//	Command_FreeResources();
}

void CGameContext::Connect(CStr _Address)
{
	MAUTOSTRIP(CGameContext_Connect, MAUTOSTRIP_VOID);

	MSCOPE(CGameContext::Connect, GAMECONTEXT);

	if (_Address.Find(":") < 0) _Address += ":28000";

	LogFile("(CGameContext::Connect)...");
	if (m_lspWClients.Len())
	{
		LogFile("(CGameContext::Connect) Already connected.");
		return;
	}

	DestroyClients();
	m_spWServer = NULL;
	if (m_spNetwork != NULL && m_hServerSocket >= 0)  
	{		
		m_spNetwork->Connection_Close(m_hServerSocket);
		m_hServerSocket = -1;
	}
	m_lspWClients.Clear();

	LogFile("(CGameContext::Connect) Creating network...");
	if (!m_spNetwork && m_spNetDevice) 
	{
		m_spNetwork = (CNetwork *)MRTC_GetObjectManager()->GetClassRegistry()->CreateObject(m_pSystem->GetEnvironment()->GetValue("NET_Context", "CNetworkCore"));

		if (!m_spNetwork) 
		{
			ConOutL("(CGameContext::Connect) No network");
			return;
		}

		m_spNetwork->Create(m_spNetDevice);
	}
	else if (!m_spNetwork)
	{
		ConOutL("(CGameContext::Connect) No network device");
		return;
	}

	LogFile("(CGameContext::Connect) Creating network client...");
	CNetwork_Address *pAddrListen = m_spNetwork->Global_ResolveAddress(m_pSystem->GetEnvironment()->GetValue("NET_ClientListen", "*:0"));

	if (!pAddrListen)
	{
		ConOutL("(CGameContext::Connect) Could not resolve client listen address");
		return;
	}

	int hConnection = m_spNetwork->Client_Create(pAddrListen, NULL);
	pAddrListen->Delete();
	if (hConnection == -1)
	{
		ConOut("Failed to open connection.");
		return;
	}
	LogFile("(CGameContext::Connect) Open connection...");

	CNetwork_Address *pConnectAddr = m_spNetwork->Global_ResolveAddress(_Address);

	if (!pConnectAddr)
	{
		m_spNetwork->Connection_Close(hConnection);
		ConOut("Failed to resolve address: " + _Address);
		return;
	}

	CNetwork_ServerInfo* pInfo = m_spNetwork->Client_OpenConnection(hConnection, pConnectAddr, 2.5f);
	pConnectAddr->Delete();

	if (pInfo == NULL)
	{
		m_spNetwork->Connection_Close(hConnection);
		LogFile("(CGameContext::Connect) Failed...");
		ConOut("Failed to connect to " + _Address);
		return;
	}

	ConExecute("cg_clearmenus();cg_dowindowswitch()");


	CStr Info;
	Info.Capture((const char *)pInfo->m_ServerInfo, pInfo->m_InfoSize);
	CStr GameName = Info.GetStrSep("|");
	CStr WorldPathes = Info.GetStrSep("|");

	ConOutL("Connected...");
	ConOutL("        GameName: " + GameName);
	ConOutL("        WorldPathes: " + WorldPathes);

	if (!SafeSetGame(WorldPathes, GameName))
	{
		m_spNetwork->Connection_Close(hConnection);
		ConOut("Game-type not available: " + GameName);
		return;
	}

	CreateClientGame();

	LogFile("(CGameContext::Connect) Creating client...");
	spCReferenceCount spObj = (CReferenceCount*) MRTC_GOM()->CreateObject(m_ClientClass);
	spCWorld_Client spWC = safe_cast<CWorld_Client> ((CReferenceCount*)spObj);
	if (spWC==NULL) Error("Connect", "Unable to create client-object.");

	CWC_CreateInfo CreateInfo;
	CreateInfo.m_Mode = WCLIENT_MODE_PRIMARY;
	CreateInfo.m_spGameReg = m_spGameReg;
	CreateInfo.m_spWData = m_spWData;
	CreateInfo.m_spEngine = m_spEngine;

	spWC->Create(CreateInfo);
	spWC->Net_SetConnection(m_spNetwork, hConnection);

	// Set playername
	/*	CStr Name = "Noname";
	if((m_spGameReg != NULL) && m_spGameReg->Find("USER\\NAME"))
	Name = m_spGameReg->GetValue("USER\\NAME", "Noname");
	spWC->Player_GetLocal()->m_Name = Name;*/


	LogFile("(CGameContext::Connect) Adding client to sub-client list.");
	// Set SoundContext
	spWC->Sound_SetSoundContext(m_spSoundContext);

	m_lspWClients.Add(spWC);
	LogFile("(CGameContext::Connect) Done.");
	StartSimulation();
}

bool CGameContext::SafeSetGame(CStr &_WorldPathes, CStr &_GameName)
{
	MAUTOSTRIP(CGameContext_SafeSetGame, false);
	MSCOPE(CGameContext::SafeSetGame, GAMECONTEXT);

	Disconnect(false);

	CStr OldWorldPathesRel = m_WorldPathesRel;
	CStr OldGameName = m_GameName;

	if (!SetGame(_WorldPathes, _GameName))
	{
		ConOutL("(CGameContext::SafeSetGame) Failed to set game " + _GameName + " at " + _WorldPathes);

		if (!SetGame(OldWorldPathesRel, OldGameName))
		{
			Error("SafeSetGame", "Failed to restore old game.");
		}

		return false;
	}

	CreateClientGame();
	return true;
}


CWorld_Client* CGameContext::GetCurrentClient()
{
	MAUTOSTRIP(CGameContext_GetCurrentClient, NULL);
	if (!m_lspWClients.Len()) 
		return NULL;

	return m_lspWClients[0];
}

void CGameContext::StartSimulation()
{
	MAUTOSTRIP(CGameContext_StartSimulation, MAUTOSTRIP_VOID);
	//	m_LastSimulateTime = CMTime::GetCPU();
	m_LastSimulateTime.Snapshot();
	m_SimulateTime = m_LastSimulateTime;
	m_SimulateCPUTimeAverage = 0;
	m_SimulateCPUTimeAveragePrim = 0;
	m_SimulateCPUTimeCurrent = 0;
}

void CGameContext::StopSimulation()
{
	MAUTOSTRIP(CGameContext_StopSimulation, MAUTOSTRIP_VOID);
}

void CGameContext::Simulate_Pause()
{
	MAUTOSTRIP(CGameContext_Simulate_Pause, MAUTOSTRIP_VOID);
	M_TRACE("Simulate_Pause, pausecount %d -> %d\n", m_PauseCount, m_PauseCount + 1);

	if (!m_PauseCount)
		m_PauseTime.Snapshot();
	m_PauseCount++;

	if (m_spWServer != NULL)
	{
		//M_TRACEALWAYS("%i. Pause\n", m_spWServer->GetGameTick());
		m_spWServer->World_Pause(1);
	}
}

void CGameContext::Simulate_Resume()
{
	MAUTOSTRIP(CGameContext_Simulate_Resume, MAUTOSTRIP_VOID);
	M_TRACE("Simulate_Resume, pausecount %d -> %d\n", m_PauseCount, m_PauseCount - 1);

	if (!m_PauseCount) 
	{
		ConOut("§cf80WARNING: (Simulate_Resume) Not paused.");

		// Just to be absolutely sure.
		if (m_spWServer != NULL && m_spWServer->World_Pause(0))
		{
			m_LastSimulateTime.Snapshot();
			m_SimulateCPUTimeAverage = 0;
			m_SimulateCPUTimeAveragePrim = 0;
			m_SimulateCPUTimeCurrent = 0;
		}

		return;
		//Error("Simulate_Resume", "Not paused.");
	}

	if (!(--m_PauseCount))
	{
		CMTime tNow;
		tNow.Snapshot();
		if (m_Mode & WMODE_TIMEDEMO)
			m_SimulateTime += (tNow - m_LastSimulateTime); //AR-NOTE: I added this adjustment, because when capturing demos the delta time would go insane without it...
		m_LastSimulateTime = tNow;

		m_SimulateCPUTimeAverage = 0;
		m_SimulateCPUTimeAveragePrim = 0;
		m_SimulateCPUTimeCurrent = 0;
		//	LogFile(CStrF("Adding time %f", GetRawTime() - m_PauseTime));
		//		m_LastSimulateTime += GetRawTime() - m_PauseTime;

		if (m_spWServer != NULL)
		{
			//M_TRACEALWAYS("%i. Resume\n", m_spWServer->GetGameTick());
			m_spWServer->World_Pause(0);
		}
	}
}

void CGameContext::Simulate()
{
	MAUTOSTRIP(CGameContext_Simulate, MAUTOSTRIP_VOID);
	MSCOPESHORT(CGameContext::Simulate);

	if (CDiskUtil::GetCorrupt() & DISKUTIL_STATUS_CORRUPTANY)
		return;

	if (m_spWServer == NULL) 
		return;

	if (m_PauseCount && !m_bSingleStep)
	{	
		m_spWServer->Net_Refresh();
		return;
	}
	CWorld_Client* pC = GetCurrentClient();
#if defined(M_Profile) && !defined(DEF_DISABLE_PERFGRAPH)
	CPerfGraph* pPerfGraph = (pC) ? pC->GetGraph(WCLIENT_GRAPH_SERVERTIME) : NULL;
#else
	CPerfGraph* pPerfGraph = NULL;
#endif

#ifdef WCLIENT_FIXEDRATE
	// -------------------------------------------------------------------
	// FIXED FRAME RATE
	m_spWServer->Net_Refresh();

	m_spWServer->Simulate(pPerfGraph);

	if (pC)
	{
		CNetMsg OutPacket(WPACKET_LOCALFRAMEFRACTION);
		OutPacket.Addfp32(1.0f);
		pC->Net_OnMessage(OutPacket);
	}
	m_LastSimulateTime.Snapshot();
	m_spWServer->Net_Refresh();

#else
	// -------------------------------------------------------------------
	// DYNAMIC FRAME RATE

	// Network update
#ifndef	DEF_DISABLE_PERFGRAPH
	CMTime TNet; TStart(TNet);
#endif
	m_spWServer->Net_Refresh();
#if defined(M_Profile) && !defined(DEF_DISABLE_PERFGRAPH)
	TStop(TNet); if (pC) pC->AddGraphPlot(WCLIENT_GRAPH_NETTIME, TNet.GetTime(), 0xff0000ff);
#endif

	fp32 TickTime = m_spWServer->GetGameTickRealTime();

	//	CMTime Time = CMTime::GetCPU();
	CMTime Time;
	Time.Snapshot();
	fp32 dTime = (Time - m_LastSimulateTime).GetTime();
	if (dTime < 0) dTime = 0;
	if (dTime > 1.000f || m_bSingleStep)
	{
		m_LastSimulateTime = Time;
		dTime = TickTime;
	}

	if (dTime < TickTime && m_bSimulateFractional)
	{
		CMTime TSim; 
		{
			TMeasure(TSim);
			m_spWServer->SimulateAhead(pPerfGraph, m_SimulateCPUTimeAverage * dTime / TickTime);
		}
		if (m_bSimulateFractional & 2)
			ConOut(CStrF("SimulateAhead %f / %f, Average %f", TSim.GetTime(), m_SimulateCPUTimeAverage * dTime / TickTime, m_SimulateCPUTimeAverage));
		m_SimulateCPUTimeCurrent += TSim.GetTime();
	}
	else 
	{
		while(dTime >= TickTime)
		{
			dTime -= TickTime;
			m_LastSimulateTime += CMTime::CreateFromSeconds(TickTime);

			//			fp64 Time; T_Start(Time);
			//			if (pC) pC->UpdateControls();
			if (pC) pC->Refresh();

			CMTime TSim; 
			{
				TMeasure(TSim);
				if (m_spWServer->Simulate(pPerfGraph))
				{
					//					m_LastSimulateTime = CMTime::GetCPU();
					m_LastSimulateTime.Snapshot();
					break;
				}

				m_spWServer->Simulate_SetCurrentLocalFrameFraction(dTime / TickTime);

			}
			m_SimulateCPUTimeCurrent += TSim.GetTime();

			//			if (pPerfGraph) pPerfGraph->Plot(m_SimulateCPUTimeCurrent, 0xffffff01);

			m_SimulateCPUTimeCurrent *= 100.0f;
			m_SimulateCPUTimeAverage *= 100.0f;
			m_SimulateCPUTimeAveragePrim *= 100.0f;
			Moderatef(m_SimulateCPUTimeAverage, m_SimulateCPUTimeCurrent,m_SimulateCPUTimeAveragePrim, 256);
			m_SimulateCPUTimeAverage /= 100.0f;
			m_SimulateCPUTimeAveragePrim /= 100.0f;
			m_SimulateCPUTimeCurrent /= 100.0f;
			//ConOut(CStrF("SimulateAverage %f, Current %f", m_SimulateCPUTimeAverage, m_SimulateCPUTimeCurrent));
			m_SimulateCPUTimeCurrent = 0;

#ifndef	DEF_DISABLE_PERFGRAPH
			TStart(TNet);
#endif
			m_spWServer->Net_Refresh();
#if defined(M_Profile) && !defined(DEF_DISABLE_PERFGRAPH)
			TStop(TNet); if (pC) pC->AddGraphPlot(WCLIENT_GRAPH_NETTIME, TNet.GetTime(), 0xff0000ff);
#endif

			//			T_Stop(Time);
			//			if (pC) pC->AddGraphPlot(WCLIENT_GRAPH_SERVERTIME, Time / GetCPUFrequency(), 0xffffff00);
		}
	}

	if (pC)
	{
		CNetMsg OutPacket(WPACKET_LOCALFRAMEFRACTION);
		OutPacket.Addfp32(dTime / TickTime);
		pC->Net_OnMessage(OutPacket);
	}
#endif

	if (m_bSingleStep)
		m_spWServer->NetMsg_SendToClient(CNetMsg(WPACKET_SINGLESTEP));

	m_bSingleStep = false;
}


bool CGameContext::WriteProfileInfo(CStr _Profile)
{
	m_spAsyncSaveContext = NULL;
	m_spAsyncSaveContext = MNew1(CGameContext::CSaveContext, m_spWData);

	//m_SaveInfoString = "§LMENU_SAVINGPROFILE";
	m_SaveInfoString = "";

	FixProfile(_Profile);
#ifdef PLATFORM_CONSOLE
	spCCFile spFileMem = CDiskUtil::CreateCCFile(m_lProfileInfo, CFILE_WRITE | CFILE_BINARY);
	//CDataFile DataFile;
	//DataFile.Create(spFileMem);
	//CCFile *pF = DataFile.GetFile();

	//DataFile.BeginEntry("OPTIONS");
	//MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	pSys->GetOptions()->Write(spFileMem);
	spFileMem->Close();
	m_lProfileInfo.OptimizeMemory();
	int32 Len = m_lProfileInfo.Len();
	int32 AlignedLen = (Len+3)/4*4;
	m_lProfileInfo.SetLen(AlignedLen);
	m_lProfileInfo.QuickSetLen(Len);
#else
	m_lProfileInfo.SetLen(0);
	spCCFile spFileMem = CDiskUtil::CreateCCFile(m_lProfileInfo, CFILE_WRITE | CFILE_BINARY);

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	pSys->GetOptions()->XRG_WriteChildren(spFileMem);
	spFileMem->Close();
	m_lProfileInfo.OptimizeMemory();
	int32 Len = m_lProfileInfo.Len();
	int32 AlignedLen = (Len+3)/4*4;
	m_lProfileInfo.SetLen(AlignedLen);
	for (int i = Len; i < AlignedLen; ++i)
		m_lProfileInfo[i] = 32;

	m_lProfileInfo.QuickSetLen(Len);
#endif

	if(!BeginWriteSaveFile(_Profile, "_profile"))
		return false;

	WriteSaveFileBlock(m_lProfileInfo.GetBasePtr(), AlignedLen);
	EndWriteSaveFile();

	return true;
}

static bool g_bPlayingDemoMusic = false;

void CGameContext::Refresh(CXR_VBManager* _pVBM)
{
	MAUTOSTRIP(CGameContext_Refresh, MAUTOSTRIP_VOID);
	MSCOPE(CGameContext::Refresh, GAMECONTEXT);

	if (!m_spWData)
		return;

	if (CDiskUtil::GetCorrupt() & DISKUTIL_STATUS_CORRUPTANY)
		return;

	if (CDiskUtil::GetCorrupt() & DISKUTIL_STATUS_CORRUPTWRITE)
	{
		CDiskUtil::RemoveCorrupt(DISKUTIL_STATUS_CORRUPTWRITE);
		ConExecute("disconnect()");
		ConExecute("cg_rootmenu(\"select_profile\")");
		return;
	}


	FlushPendingCommands();

	//

	if(m_spAsyncSaveContext && m_spAsyncSaveContext->m_spWData->m_AsyncWriter.Done())
	{
		m_llChunks.Clear();
		for(int32 i = 0; i < m_lCRCData.Len(); i++)
			delete [] m_lCRCData[i];
		m_lCRCData.Clear();

		m_spAsyncSaveContext = NULL;
	}

	if(m_spAsyncSaveContext == NULL && m_SaveProfile.Len())
	{
		WriteProfileInfo(m_SaveProfile);
		m_SaveProfile = "";
	}

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	fp32 ValueSfx = pSys->GetOptions()->GetValuef("SND_VOLUMESFX", 1.0f);

	if (pSys)
	{
		pSys->GetRegistry()->SetValuef("SND_VIDEO_VOLUME", ValueSfx);
	}



	/*	if (m_PendingSetGameWorldPathes != "")
	{
	Disconnect();
	if (!SafeSetGame(m_PendingSetGameWorldPathes, m_PendingSetGameGameName))
	ConOutL("(CGameContext::Refresh) Deferred setgame failed.");
	m_PendingSetGameWorldPathes = "";
	m_PendingSetGameGameName = "";
	return;
	}*/

	if (m_spNetwork!=NULL) 
		m_spNetwork->Refresh();

	if (m_hListenConnection >= 0 && (m_spNetwork!=NULL))
	{
		while(m_spNetwork->Client_ServerInfoAvail(m_hListenConnection))
		{			
			CNetwork_ServerInfo *pServerInfo = m_spNetwork->Client_GetServerInfo(m_hListenConnection);
			if (pServerInfo)
			{
				CStr Info;
				Info.Capture((const char *)pServerInfo->m_ServerInfo, pServerInfo->m_InfoSize);
				Info.SetChar(pServerInfo->m_InfoSize, 0);
				ConOut(CStr() + "Server: " + pServerInfo->m_Name + " at " + pServerInfo->m_Address + "  Info:" + Info);

				m_spNetwork->Client_GetServerInfoRelease(m_hListenConnection);
			}
			else
				break;
		}
	}

	if (m_spFrontEnd != NULL)
		m_spFrontEnd->OnRefresh();

	if (m_spWServer != NULL)
	{
		for(int i = 0; i < m_lspWClients.Len(); i++)
		{
			if (!m_lspWClients[i]) Error("PreRefresh", CStrF("NULL client %d", i));
			m_lspWClients[i]->PreRefresh();
		}
		Simulate();
		for(int i = 0; i < m_lspWClients.Len(); i++)
		{
			if (!m_lspWClients[i]) Error("Refresh", CStrF("NULL client %d", i));
			m_lspWClients[i]->Refresh();
		}
	}
	else
	{
		if (m_lspWClients.Len())
		{
			if (m_Mode & WMODE_DEMO)
			{
				// Only progress demo when precaching is complete.
				CWorld_Client* pC = m_lspWClients[0];
				if (pC->Precache_Status() == 0)
				{
					if (m_TimeDemoStart.IsReset())
					{
						if (m_bTimeDemoGraph)
						{
							m_bStartCallGraphPending = true;
						}
						if (m_bBenchmark)
						{
							m_TimeDemoFrameTimes.SetGrow(4096);

							MACRO_GetSystem;
							CRegistry* pReg = pSys->GetRegistry()->CreateDir("XR");
							int ShaderMode = pSys->GetEnvironment()->GetValuei("XR_SHADERMODE", 0);
							int ShaderModeCurrent = pReg->GetValuei("XR_SHADERMODECURRENT", 0);

							if (ShaderMode != 0)
							{
								if (ShaderModeCurrent != ShaderMode)
								{
									M_TRY
									{
										{
											CStr FileName = pSys->m_ExePath + "BenchResults.txt";
											CCFile File;
											File.Open(FileName, CFILE_WRITE | CFILE_BINARY);
											CStr ToWrite = CStrF("Your graphics card does not support shader mode %d the hightest available was: %d\r\n", ShaderMode, ShaderModeCurrent);
											File.Write(ToWrite.Str(), ToWrite.Len());
										}
									}
									M_CATCH(
										catch (CCException)
									{
									}		
									)

										pSys->m_bBreakRequested = true;
								}
							}
						}


						M_TRACEALWAYS("----------------- Starting Timedemo ----------------\n");
						m_TimeDemoFrameTimer = CMTime::GetCPU();
						m_nTimeDemoFramesCaptured = 0;
						m_TimeDemoStart.Start();

					}
					else
					{
						if (m_bBenchmark)
						{
							CMTime CpuTime = CMTime::GetCPU();
							m_TimeDemoFrameTimes.Add((CpuTime - m_TimeDemoFrameTimer).GetTime());
							m_TimeDemoFrameTimer = CpuTime;
						}
					}

					if (m_nTimeDemoFramesPlayed <= 10)
					{
						m_TimeDemoStart.Reset();
						m_TimeDemoStart.Start();
					}

					//					if(!g_bPlayingDemoMusic)
					//					{
					//						MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
					//						if (pCon) pCon->ExecuteString("stream_play \"sbz1\\\\music\\\\Light_01.wma\"");
					//						g_bPlayingDemoMusic = true;
					//					}
					// Standard demo

					fp32 TickTime = pC->GetGameTickRealTime();

					fp32 dTime = 0;
					if (m_Mode & WMODE_TIMEDEMO)
					{
						m_SimulateTime += CMTime::CreateFromSeconds(m_TimeDemoRate);
						dTime = (m_SimulateTime - m_LastSimulateTime).GetTime();

						//					ConOutL(CStrF("DemoTick %d, Time %f, LastTime %f, m_TimeDemoRate %f", m_nTimeDemoFramesPlayed, m_SimulateTime, m_LastSimulateTime, m_TimeDemoRate));
					}
					else
					{
						//						CMTime Time = CMTime::GetCPU();
						CMTime Time;
						Time.Snapshot();
						dTime = (Time - m_LastSimulateTime).GetTime();
						if (dTime < 0) dTime = 0;
						if (dTime > 0.250f / pC->GetTimeScale())
						{
							m_LastSimulateTime = Time - CMTime::CreateFromSeconds(TickTime);
							dTime = TickTime;
						}
					}

					while(dTime >= TickTime)
					{
						if (!pC->Demo_PlayTick() || m_nTimeDemoFramesPlayed >= m_nTimeDemoMaxFrames)
						{
							if (m_Mode & WMODE_TIMEDEMO)
							{
								m_nTimeDemoFramesPlayed -= 10;
								m_TimeDemoStart.Stop();

								fp32 t = m_TimeDemoStart.GetTime();
								ConOutL("-----------------------------------------------------");
								ConOutL(CStrF("Frames played %d, Time elapsed %.2fs", m_nTimeDemoFramesPlayed, t));
								ConOutL(CStrF("Average FPS %.3f", fp32(m_nTimeDemoFramesPlayed) / t));
								ConOutL("-----------------------------------------------------");

								MACRO_GetSystemRegistry(pReg);
								if (pReg)
								{
									pReg->SetValue("Timedemo\\Fps", CStrF("%.3f", fp32(m_nTimeDemoFramesPlayed) / t));
									pReg->SetValue("Timedemo\\Time", CStrF("%.3f", t));
									pReg->SetValue("Timedemo\\Frames", CStrF("%d", m_nTimeDemoFramesPlayed));
								}
								M_TRACEALWAYS("-----------------------------------------------------\n");
								M_TRACEALWAYS("Frames played %d, Time elapsed %.3fs\n", m_nTimeDemoFramesPlayed, t);
								M_TRACEALWAYS("Average FPS %.3f\n", fp32(m_nTimeDemoFramesPlayed) / t);
								M_TRACEALWAYS("-----------------------------------------------------\n");

								if (m_bTimeDemoGraph)
								{
									m_bStopCallGraphPending = true;
								}
							}

							if (m_iDemo >= m_lDemos.Len())
							{
								m_Mode &= ~WMODE_DEMO;
#if !defined( PLATFORM_PS2 ) && !defined( PLATFORM_DOLPHIN )
								ConExecute("videofileend()");
#endif
							}
							else
								PlayDemo(m_lDemos[m_iDemo++], (m_Mode & WMODE_TIMEDEMO) != 0);
						}

						pC->PreRefresh();
						pC->Refresh();
						dTime -= TickTime;
						m_LastSimulateTime += CMTime::CreateFromSeconds(TickTime);
					}

					if (m_Mode & WMODE_TIMEDEMO)
						m_nTimeDemoFramesPlayed++;
				}

				if (!(m_Mode & WMODE_DEMO))
				{
					MACRO_GetRegisterObject(CXR_VBMContainer, pVBMContainer, "SYSTEM.VBMCONTAINER");
					if(pVBMContainer)
					{
						pVBMContainer->BlockUntilAllVBMFree();
					}

					m_lspWClients.Clear();
					MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
					m_spWData->Resource_DeleteUnreferenced(_pVBM);
					if (m_bBenchmark)
					{
						M_TRY
						{
							{
								CStr FileName = pSys->m_ExePath + "BenchResults.txt";
								CCFile File;
								File.Open(FileName, CFILE_WRITE | CFILE_BINARY);
								CStr ToWrite = CStrF("Played demo: %s\r\n", m_lDemos[0].Str());
								File.Write(ToWrite.Str(), ToWrite.Len());

								fp32 MinTime = 1000000000000.0;
								fp32 MaxTime = 0.0;
								for (int i = 0; i < m_TimeDemoFrameTimes.Len(); ++i)
								{
									if (m_TimeDemoFrameTimes[i] < MinTime)
										MinTime = m_TimeDemoFrameTimes[i];
									if (m_TimeDemoFrameTimes[i] > MaxTime)
										MaxTime = m_TimeDemoFrameTimes[i];
								}

								fp32 t = m_TimeDemoStart.GetTime();

								ToWrite = CStrF("Time: %.2f seconds\r\n", t);
								File.Write(ToWrite.Str(), ToWrite.Len());
								ToWrite = CStrF("Frames played: %d\r\n", m_nTimeDemoFramesPlayed);
								File.Write(ToWrite.Str(), ToWrite.Len());
								ToWrite = CStrF("Average Frame rate: %.2f\r\n", fp32(m_nTimeDemoFramesPlayed) / t);
								File.Write(ToWrite.Str(), ToWrite.Len());
								ToWrite = CStrF("Min Frame rate: %.2f\r\n", 1.0 / MaxTime);
								File.Write(ToWrite.Str(), ToWrite.Len());
								ToWrite = CStrF("Max Frame rate: %.2f\r\n", 1.0 / MinTime);
								File.Write(ToWrite.Str(), ToWrite.Len());
								ToWrite = CStr("\r\nIndividual frames (fps):\r\n");
								File.Write(ToWrite.Str(), ToWrite.Len());
								for (int i = 0; i < m_TimeDemoFrameTimes.Len(); ++i)
								{
									ToWrite = CStrF("%.2f\r\n", 1.0 / m_TimeDemoFrameTimes[i]);
									File.Write(ToWrite.Str(), ToWrite.Len());
								}
							}
						}
						M_CATCH(
							catch (CCException)
						{
						}
						)

							pSys->m_bBreakRequested = true; // Quit
					}
					else
					{
#ifdef PLATFORM_XBOX

						//						if (pCon) pCon->ExecuteString("cg_rootmenu(\"MainMenu_Real\");deferredscript(\"deferredscript (\\\"cg_submenu (\\\\\"DemoMenu\\\\\")\\\")\")");
						if (pCon) pCon->ExecuteString("cg_rootmenu(\"MainMenu_Real\");cg_dowindowswitch();cg_submenu(\"ZoneMenu\");cg_dowindowswitch();cg_submenu(\"DemoMenu\")");
#else
						if (pCon) pCon->ExecuteString("cg_rootmenu(\"mainMenu\")");
#endif
					}
				}
			}
			else
			{
				m_lspWClients[0]->PreRefresh();
				m_lspWClients[0]->Refresh();
				if ((m_spNetwork != NULL) && (m_lspWClients[0]->Net_GetConnection() >= 0))
				{
					uint32 Status = m_lspWClients[0]->Net_GetConnectionStatus();
					//#ifdef _DEBUG
					//					int bTimeOut = false;
					//#else
					int bTimeOut = Status & ENetworkPacketStatus_Timeout;
					//#endif
					//					bTimeOut = false;
					if ((bTimeOut || (Status & ENetworkPacketStatus_Closed)) && !m_spFrontEnd->m_GameIsLoading)
					{
						ConExecute("removepadlock(); disconnect(); cg_rootmenu(\"MainMenu\")");//cg_cubeseq(\"startup\"); 
						ConExecute("deferredscript(\"cg_submenu(\\\"Multiplayer\\\")\")");//; cg_cubeside(2)
						m_lspWClients.Clear();
						if (bTimeOut)
							ConOut("Server timeout.");
						else
							ConOut("Server quit.");

#if defined(PLATFORM_CONSOLE)
						MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
						CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
						if(pGameMod)
						{
							pGameMod->m_pMPHandler->ShutdownSession(false, true);
							pGameMod->m_pMPHandler->ShutdownLAN();
						}
#endif
					}
				}
			}
		}
	}
	//	CHECKMEMORY("Refresh (Done)");

	if (m_spSoundContext != NULL)
	{
		MSCOPE(Sound, GAMECONTEXT_SOUND);
		/*		CWorld_Client* pC = GetCurrentClient();
		if (pC)
		{
		CMat4Dfp32 Pos;
		pC->Render_GetLastRenderCamera(Pos);
		m_spSoundContext->Listen3D_SetCoordinateScale(1.0f/32.0f);
		m_spSoundContext->Listen3D_SetOrigin(Pos);
		}*/
		m_spSoundContext->Refresh();
	}

	if (m_spWData != NULL)
		m_spWData->OnRefresh();

	if (m_spNetwork && m_hListenConnection >= 0)
	{
		m_spNetwork->Connection_Flush(m_hListenConnection);
	}

	if(m_bSoakMode)
	{
		// Make sure we're not currenly loading somehow
		CWorld_Client* pClient = GetCurrentClient();
		if(!pClient || pClient->GetClientState() == WCLIENT_STATE_INGAME)
		{
			CMTime Now = CMTime::GetCPU();
			static CMTime NextSpawnTime;

			if(NextSpawnTime.IsInvalid())
				NextSpawnTime = Now + CMTime::CreateFromSeconds((MRTC_RAND() % 40) + 20);

			if(!pClient || Now.Compare(NextSpawnTime) > 0)
			{
				CStr LvlName = "ny1_tunnel";
				do 
				{
					if(!m_spWServer)
						break;
					CRegistry* pServerReg = m_spWServer->Registry_GetServer();
					if(!pServerReg)
						break;
					CRegistry* pLvlKeys = pServerReg->FindChild("LEVELKEYS");
					if(!pLvlKeys)
						break;

					int nLvl = pLvlKeys->GetNumChildren();
					CRegistry* pLvl = NULL;
					do 
					{
						int iLvl = MRTC_RAND() % nLvl;

						pLvl = pLvlKeys->GetChild(iLvl);
						if(!pLvl)
							break;

						if(!CDiskUtil::FileExists(ResolveFileName(CStrF("Worlds\\%s.xw", pLvl->GetThisName().GetStr()))))
						{
							M_TRACEALWAYS("Could not find lvl '%s', choosing another for soaktesting\r\n", pLvl->GetThisName().GetStr());
							continue;
						}

						break;
					} while(1);

					if(!pLvl)
						break;
					int nLayers = 0;
					for(int i = 0; i < pLvl->GetNumChildren(); i++)
					{
						if(pLvl->GetName(i).CompareNoCase("layer") == 0)
						{
							nLayers++;
						}
					}

					if(nLayers > 0)
					{
						int iLayer = MRTC_RAND() % nLayers;
						for(int i = 0; i < pLvl->GetNumChildren(); i++)
						{
							if(pLvl->GetName(i).CompareNoCase("layer") == 0)
							{
								if(iLayer == 0)
								{
									LvlName = pLvl->GetValue(i);
									break;
								}
								iLayer--;
							}
						}
					}
					else
						LvlName = pLvl->GetThisName();
				} while(0);
				m_nSoakCount++;
				M_TRACEALWAYS("Trying to soak layer '%s', soak instance %u\r\n", LvlName.GetStr(), m_nSoakCount);
				ConExecute(CStrF("scriptlayer(\"%s\");", LvlName.GetStr()));

				NextSpawnTime.MakeInvalid();
			}
		}
	}
}

bool CGameContext::GameRunning()
{
	MAUTOSTRIP(CGameContext_GameRunning, false);
	return (GetCurrentClient() != NULL);
}

void CGameContext::SetSoundContext(spCSoundContext _spSoundContext)
{
	MAUTOSTRIP(CGameContext_SetSoundContext, MAUTOSTRIP_VOID);
	// Set null-sndctx. on all clients to kill all sounds.
	{ 
		for(int i = 0; i < m_lspWClients.Len(); i++)
			if (m_lspWClients[i]!=NULL) m_lspWClients[i]->Sound_SetSoundContext(NULL);

		if (m_spFrontEnd)
			m_spFrontEnd->SetSoundContext(NULL);
	}

	// Set the new sndctx on all clients. All Clients in the world-context are local.
	m_spSoundContext = _spSoundContext;
	if(m_spSoundContext)
	{ 
		for(int i = 0; i < Min(1, m_lspWClients.Len()); i++)
			if (m_lspWClients[i]!=NULL) m_lspWClients[i]->Sound_SetSoundContext(m_spSoundContext);

		if (m_spFrontEnd)
			m_spFrontEnd->SetSoundContext(m_spSoundContext);
	}
}

void CGameContext::SetVolume(fp32 _VolumeAmbient, fp32 _VolumeSfx, fp32 _VolumeVoice, fp32 _VolumeFrontEnd)
{
	MAUTOSTRIP(CGameContext_SetVolume, MAUTOSTRIP_VOID);
	m_VolumeAmbient = _VolumeAmbient;
	m_VolumeSfx = _VolumeSfx;
	m_VolumeVoice = _VolumeVoice;
	m_VolumeFrontEnd = _VolumeFrontEnd;

	if(m_spSoundContext)
	{ 
		for(int i = 0; i < Min(1, m_lspWClients.Len()); i++)
			if (m_lspWClients[i]!=NULL) m_lspWClients[i]->Sound_SetVolume(m_VolumeAmbient, m_VolumeSfx, m_VolumeVoice);

		if (m_spFrontEnd)
			m_spFrontEnd->SetVolume(m_VolumeFrontEnd);
	}
}

bool CGameContext::CanRenderGame()
{
	MAUTOSTRIP(CGameContext_CanRenderGame, false);
	MSCOPESHORT(CGameContext::CanRenderGame);

	CWorld_Client* pC = GetCurrentClient();
	if (pC)
		return pC->CanRenderGame();
	else
		return false;
}

static CScanKey ConstructScanKey(int _ScanCode, const CScanKey& _Old)
{
	CScanKey SK(_Old);
	SK.m_ScanKey32 = _ScanCode | (_Old.m_ScanKey32 & 0xfffffe00);
	SK.m_Char = _Old.m_Char;
	SK.m_Time = _Old.m_Time;
	return SK;
}


// PC PORT: TODO: Global vars? NO! Yach! Not this way anyway...
#ifdef GUI_PC
// Timer for smooth GUI mousemovement
CMTime PrevTime = CMTime::GetCPU();
#endif 

static int TranslateScankey(const CScanKey& _Key, CScanKey* _pTranslated, int _MaxTranslate, int _PlayerNr)
{
	MAUTOSTRIP(TranslateScankey, 0);
	int nTranslate = 0;

	int Scan = _Key.GetKey9();

	if (_Key.IsRepeat() && _Key.GetKey9() != SKEY_MOUSEMOVEREL)
	{
		_pTranslated[nTranslate++] = ConstructScanKey(0, _Key);
		return nTranslate;
	}

	// Only player 0 and "nobody" (_PlayerNr -1) recieves all scans.
	if (_PlayerNr == -1 || _PlayerNr == 0)
		_pTranslated[nTranslate++] = _Key;

	if (!Scan)
		return nTranslate;

	// Translate joystick scans to GUI scans.
	// If player number is specified, only scans for the associated joystick are translated.
#if defined(PLATFORM_XENON) || defined(PLATFORM_PS3)
	{
		int iJoy = _Key.m_iDevice;
		if (Scan >= SKEY_JOY_BUTTON00 &&
			Scan <= SKEY_JOY_BUTTON0F)
		{
			if (_PlayerNr == -1 || iJoy == _PlayerNr)
			{
				if (Scan == SKEY_JOY_BUTTON00) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON0, _Key);
				else if (Scan == SKEY_JOY_BUTTON01) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON1, _Key);
				else if (Scan == SKEY_JOY_BUTTON02) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON2, _Key);
				else if (Scan == SKEY_JOY_BUTTON03) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON3, _Key);
				else if (Scan == SKEY_JOY_BUTTON04) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON4, _Key);
				else if (Scan == SKEY_JOY_BUTTON05) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON5, _Key);
			}
		}
	}
#elif defined(PLATFORM_CONSOLE)
	int JoyScanOfs = SKEY_JOY1_AXIS00 - SKEY_JOY0_AXIS00;
	for(int iJoy = 0; iJoy < 4; iJoy++)
	{
		int Scan = _Key.GetKey9();
		if (Scan >= SKEY_JOY0_AXIS00 + JoyScanOfs*iJoy &&
			Scan <= SKEY_JOY0_AXIS0F + JoyScanOfs*iJoy)
		{
			if (_PlayerNr == -1 || iJoy == _PlayerNr)
			{
				Scan -= JoyScanOfs*iJoy;
#if defined(PLATFORM_XBOX) || defined(PLATFORM_DOLPHIN)
				if (Scan == SKEY_JOY0_AXIS00) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON0, _Key);
				else if (Scan == SKEY_JOY0_AXIS01) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON1, _Key);
				else if (Scan == SKEY_JOY0_AXIS02) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON2, _Key);
				else if (Scan == SKEY_JOY0_AXIS03) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON3, _Key);
				else if (Scan == SKEY_JOY0_AXIS08) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON4, _Key);
				else if (Scan == SKEY_JOY0_AXIS09) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON5, _Key);
#elif defined PLATFORM_PS2
				if (Scan == SKEY_JOY0_AXIS02) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON0, _Key);	// Cross
				//				else if (Scan == SKEY_JOY0_AXIS04) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON1, _Key);
				else if (Scan == SKEY_JOY0_AXIS01) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON2, _Key);	// Circle
				else if (Scan == SKEY_JOY0_AXIS0A) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON3, _Key);	// R3
				else if (Scan == SKEY_JOY0_AXIS0B) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON4, _Key);	// START
				else if (Scan == SKEY_JOY0_AXIS00) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON5, _Key);	// Triangle
#else
				if (Scan == SKEY_JOY0_AXIS03) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON0, _Key);
				else if (Scan == SKEY_JOY0_AXIS04) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON1, _Key);
				else if (Scan == SKEY_JOY0_AXIS00) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON2, _Key);
				else if (Scan == SKEY_JOY0_AXIS01) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON3, _Key);
				else if (Scan == SKEY_JOY0_AXIS0A) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON4, _Key);
				else if (Scan == SKEY_JOY0_AXIS0B) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_BUTTON5, _Key);
#endif

				// This is hard-coded for Gravis Eliminator Shock. Note the y-flip.
				else if (Scan == SKEY_JOY0_AXIS0E)
				{
					int x = _Key.m_Data[0];
					int y = -_Key.m_Data[1];
					if (Abs(x) > 128 || Abs(y) > 128)
					{
						if (Abs(x) > Abs(y))
						{	_pTranslated[nTranslate++] = (x > 0) ? ConstructScanKey(SKEY_GUI_RIGHT, _Key) : ConstructScanKey(SKEY_GUI_LEFT, _Key); }
						else
						{	_pTranslated[nTranslate++] = (y > 0) ? ConstructScanKey(SKEY_GUI_UP, _Key) : ConstructScanKey(SKEY_GUI_DOWN, _Key); }
					}
				}

				// Convert left analog XBox stick to GUI presses. We don't want to bind this .. bla bla tjaffs..
				else if (Scan == SKEY_JOY0_AXIS0C)
				{
					int x = _Key.m_Data[0];
					int y = _Key.m_Data[1];
					if (Abs(x) > 128)
						_pTranslated[nTranslate++] = (x > 0) ? ConstructScanKey(SKEY_GUI_RIGHT, _Key) : ConstructScanKey(SKEY_GUI_LEFT, _Key);

					if (Abs(y) > 128)
						_pTranslated[nTranslate++] = (y > 0) ? ConstructScanKey(SKEY_GUI_UP, _Key) :  ConstructScanKey(SKEY_GUI_DOWN, _Key);
				}
			}
		}
	}
#endif

	// Translate keyboard scans to GUI scans. Keyboard belongs to player 0, or nobody.
	if ((_PlayerNr == -1 || _PlayerNr == 0) && _Key.IsDown())
	{
		if (Scan == SKEY_CURSOR_LEFT) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_LEFT, _Key);
		else if (Scan == SKEY_CURSOR_RIGHT) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_RIGHT, _Key);
		else if (Scan == SKEY_CURSOR_UP) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_UP, _Key);
		else if (Scan == SKEY_CURSOR_DOWN) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_DOWN, _Key);
		else if (Scan == SKEY_MOUSEWHEELUP) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_UP, _Key);
		else if (Scan == SKEY_MOUSEWHEELDOWN) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_DOWN, _Key);
		else if (Scan == SKEY_MOUSE1) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_OK, _Key);
		else if (Scan == SKEY_MOUSE3) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_OK, _Key);
		else if (Scan == SKEY_MOUSE4) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_CANCEL, _Key);
		else if (Scan == SKEY_MOUSE5) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_OK, _Key);
		else if (Scan == SKEY_RETURN) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_OK, _Key);
		else if (Scan == SKEY_ESC) _pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_CANCEL, _Key);

		MACRO_GetSystem;
		CRegistry *pOpt = pSys->GetOptions();
		if (pOpt)
		{
			int iScanKeyGui = pOpt->GetValuei("ACTION_GUI_UP");
			int iScanKey = pOpt->GetValuei("ACTION_FORWARD");
			if (Scan == iScanKey || Scan == iScanKeyGui)
				_pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_UP, _Key);

			iScanKeyGui = pOpt->GetValuei("ACTION_GUI_DOWN");
			iScanKey = pOpt->GetValuei("ACTION_BACKWARD");
			if (Scan == iScanKey || Scan == iScanKeyGui)
				_pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_DOWN, _Key);

			iScanKeyGui = pOpt->GetValuei("ACTION_GUI_LEFT");
			iScanKey = pOpt->GetValuei("ACTION_STRAFELEFT");
			if (Scan == iScanKey || Scan == iScanKeyGui)
				_pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_LEFT, _Key);

			iScanKeyGui = pOpt->GetValuei("ACTION_GUI_RIGHT");
			iScanKey = pOpt->GetValuei("ACTION_STRAFERIGHT");
			if (Scan == iScanKey || Scan == iScanKeyGui)
				_pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_RIGHT, _Key);

			iScanKeyGui = pOpt->GetValuei("ACTION_GUI_OK");
			iScanKey = pOpt->GetValuei("ACTION_USE");
			if ((Scan == iScanKey && _PlayerNr != -1) || Scan == iScanKeyGui)
			{
				bool bAlreadyAdded = false;
				for(int i = 0; i < nTranslate; i++)
				{
					if(_pTranslated[i].m_ScanKey32 == SKEY_GUI_OK)
					{
						bAlreadyAdded = true;
						break;
					}
				}
				if(!bAlreadyAdded)
					_pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_OK, _Key);
			}

			iScanKeyGui = pOpt->GetValuei("ACTION_GUI_CANCEL");
			if (Scan == iScanKeyGui)
				_pTranslated[nTranslate++] = ConstructScanKey(SKEY_GUI_CANCEL, _Key);			

		}

	}
	// PC PORT: Intercept all mouse button scans to fully let them behave as GUI OK/CANCEL
	if (_PlayerNr == -1 || _PlayerNr == 0)
	{
		if((Scan == SKEY_MOUSE1 || Scan == SKEY_MOUSE2))
			_pTranslated[nTranslate++] = _Key;
	}

	return nTranslate;
}

bool CGameContext::ProcessKey(const CScanKey& _Key)
{
	MAUTOSTRIP(CGameContext_ProcessKey, false);
	if (m_spFrontEnd != NULL)
	{
		CScanKey lTranslated[8];
		int nTranslated = TranslateScankey(_Key, lTranslated, 8, -1);

		bool bProcessed = false;
		for(int i = 0; i < nTranslated; i++)
		{
			if (m_spFrontEnd->ProcessKey(lTranslated[i], _Key))
			{
				bProcessed = true;
				break;
			}
		}

		if (bProcessed)
			return true;

		if(m_spFrontEnd->m_spDesktop != NULL)
			return true;
	}

	for(int iC = 0; iC < m_lspWClients.Len(); iC++)
	{
		CWorld_Client* pC = m_lspWClients[iC];
		if (!pC) continue;

		CScanKey lTranslated[8];
		int nTranslated = TranslateScankey(_Key, lTranslated, 8, iC);

		bool bProcessed = false;
		for(int i = 0; i < nTranslated; i++)
			if (pC->ProcessKey(lTranslated[i], _Key)) bProcessed = true;

		if (bProcessed) return true;
	}

	return false;
	/*	CWorld_Client* pC = GetCurrentClient();
	if (pC)
	return pC->ProcessKey(_Key);
	else
	return false;*/
}



void CGameContext::RenderClient(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _GUIVP, CWorld_Client* _pC, int _Context)
{
	MAUTOSTRIP(CGameContext_RenderClient, MAUTOSTRIP_VOID);
	MSCOPE(CGameContext::RenderClient, GAMECONTEXT);

	//	CMTime Time = CMTime::GetCPU();
	CMTime Time;
	Time.Snapshot();
	fp32 dTime = 0;
	if (m_Mode & WMODE_TIMEDEMO)
	{
		dTime = (m_SimulateTime - m_LastSimulateTime).GetTime();
	}
	else
		dTime = Max(0.0f, Min(0.250f/_pC->GetTimeScale(), (Time - m_LastSimulateTime).GetTime()));

	//ConOutL(CStrF("T %f, LT %f, dT %f, Frac %f", Time, m_LastSimulateTime, dTime, dTime / _pC->GetGameTickRealTime()) );
//	M_TRY
//	{
		_pC->Render(_pVBM, _pRC, _GUIVP, dTime / _pC->GetGameTickRealTime(), _Context);

		if (m_spWServer != NULL)
		{
			CRC_Viewport VP; 
			_pC->Render_GetLastRenderViewport(VP);
			CClipRect clip = VP.GetViewClip();
			if(clip.GetHeight() == 0 && clip.GetWidth() == 0)
				VP = _GUIVP;

			CMat4Dfp32 Mat; 
			_pC->Render_GetLastRenderCamera(Mat);

			if (_pVBM->Viewport_Push(&VP))
			{
				m_spWServer->Render(_pVBM, Mat, _Context);
				_pVBM->Viewport_Pop();
			}
		}
/*	}
	M_CATCH(
		catch(CCExceptionFile)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
	}
	)
#ifdef M_SUPPORTSTATUSCORRUPT
		M_CATCH(
		catch(CCException)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
	}
	)
#endif*/

		//
		//if(!m_AsyncWriter.Done())
		/*
		{
		int iRcFont = m_spWData->GetResourceIndex("XFC:HEADINGS", NULL);
		CWRes_XFC* pRcFont = safe_cast<CWRes_XFC>(m_spWData->GetResource(iRcFont));
		CRC_Font* pFont = NULL;
		if (pRcFont && (pFont = pRcFont->GetFont()))
		{
		CRC_Util2D Util2D;
		Util2D.Begin(_pRC, _pRC->Viewport_Get());
		CRct Rect = _pRC->Viewport_Get()->GetViewRect();
		Util2D.SetCoordinateScale(CVec2Dfp32(Rect.GetWidth() / 640.0f , Rect.GetHeight() / 480.0f));
		CClipRect Clip(0, 0, 640, 480);

		_pRC->Attrib_Push();
		CRC_Attributes Attrib;
		Attrib.SetDefault();
		_pRC->Attrib_Set(Attrib);
		_pRC->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		_pRC->Geometry_Clear();

		int Style = WSTYLE_TEXT_WORDWRAP;
		int TextColorM = CPixel32(95,90,70, 0xff);//0xffdddddd; 
		int TextColorH = 0x60ffffff;
		int TextColorD = 0x60000000;						
		fp32 w = pFont->GetWidth(pFont->GetOriginalSize(), "Saving");
		Util2D.Text_Draw(Clip, pFont, CStr("Saving").Unicode(), 640*0.85f-w, 480*0.15f, 0, TextColorM, TextColorH, TextColorD);
		//Util2D.Text_Draw()

		//			_pRC->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_COLORWRITE | CRC_FLAGS_ZWRITE);
		//			_pRC->Attrib_Enable(CRC_FLAGS_STENCIL);
		//			_pRC->Attrib_StencilOp(CRC_STENCILOP_REPLACE, CRC_STENCILOP_REPLACE, CRC_STENCILOP_REPLACE);
		//			_pRC->Attrib_StencilFunction(CRC_COMPARE_ALWAYS, 1, -1);
		//			_pRC->Attrib_StencilWriteMask(-1);

		//			// Draw mask
		//			CMTime t = CMTime::GetCPU();
		//			for(int i = 0; i < 24; i++)
		//			{
		//				int x = (M_Sin(t.GetTimeModulus(_PI2))+1)*270;
		//				int y = (M_Sin(t.GetTimeModulusScaled(0.67f, _PI2))+1)*190;
		//				Util2D.Rect(Clip, CRct(x, y, x+(M_Sin(t.GetTimeModulusScaled(0.75f, _PI2))+1)*50, y+(M_Sin(t.GetTimeModulusScaled(0.97f, _PI2))+1)*50), 0xffffffff);
		//				t += 0.37635f;
		//			}



		//			_pRC->Attrib_Pop();
		//			Util2D.End();
		}
		}*/
}

void CGameContext::RenderSplitScreenGUI(CXR_VBManager* _pVBM, CRenderContext* _pRC, bool _bVSplit, bool _bHSplit)
{
	MAUTOSTRIP(CGameContext_RenderSplitScreenGUI, MAUTOSTRIP_VOID);
}

void CGameContext::PrecachePerform(CRenderContext* _pRC)
{
	int nClients = m_lspWClients.Len();
	if (m_lspWClients.Len())
	{
		for(int i = 0; i < nClients; i++)
		{
			m_lspWClients[i]->Precache_Perform(_pRC);
		}
	}
}

void CGameContext::RenderAllClients(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP, int _Context)
{
	MAUTOSTRIP(CGameContext_RenderAllClients, MAUTOSTRIP_VOID);
	MSCOPE(CGameContext::RenderAllClients, GAMECONTEXT);

	if (m_lspWClients.Len())
	{
		CRC_Viewport VP = *_pVBM->Viewport_Get();

		/*if(m_pSystem->GetEnvironment()->GetValuei("VID_WIDESCREEN"))
		{
		VP.SetAspectRatio(16.f / 9.f);
		}*/

		//VP.SetAspectRatio(m_pSystem->m_spDisplay->GetWidescreenAspect());

		//CRct Rect = VP.GetViewRect();

		CRct _GUIRect = _GUIVP.GetViewRect();
		CRct _3DRect = _3DVP.GetViewRect();

		// -------------------------------------------------------------------

		// Get client count and figure out if and how the screen should be split.
		bool bShare = false;
		int nClients = m_lspWClients.Len();
		if (m_lspWClients[0]->GetSplitScreenMode() == WCLIENT_SPLITSCREEN_SHARE) bShare = 1;
		if(bShare)
			nClients = 1;
		bool bSplitY = nClients > 1;
		bool bSplitX = nClients > 2;
		if (m_bSplitAxis) Swap(bSplitX, bSplitY);

		int _GUI_w = (bSplitX) ? _GUIRect.GetWidth() >> 1 : _GUIRect.GetWidth();
		int _GUI_h = (bSplitY) ? _GUIRect.GetHeight() >> 1 : _GUIRect.GetHeight();

		int _3D_w = (bSplitX) ? _3DRect.GetWidth() >> 1 : _3DRect.GetWidth();
		int _3D_h = (bSplitY) ? _3DRect.GetHeight() >> 1 : _3DRect.GetHeight();


		//int w = (bSplitX) ? Rect.GetWidth() >> 1 : Rect.GetWidth();
		//int h = (bSplitY) ? Rect.GetHeight() >> 1 : Rect.GetHeight();

		// Render the clients
		for(int i = 0; i < nClients; i++)
		{
			int xw,yw;
			if (bSplitX)
			{
				xw = i & 1;
				yw = (i & 2) >> 1;
			}
			else
			{
				xw = 0;
				yw = (bSplitY) ? (i & 1) : 0;
			}




			CRct _3DView(xw*_3D_w, yw*_3D_h, xw*_3D_w+_3D_w, yw*_3D_h+_3D_h);
			_3DView += _3DVP.GetViewClip().ofs;
			_3DVP.SetView(_3DVP.GetViewClip(), _3DView);

			CRct _GUIView(xw*_GUI_w, yw*_GUI_h, xw*_GUI_w+_GUI_w, yw*_GUI_h+_GUI_h);
			_GUIView += _GUIVP.GetViewClip().ofs;
			_GUIVP.SetView(_GUIVP.GetViewClip(), _GUIView);

			if (_pVBM->Viewport_Push(&_3DVP))
			{
				_pVBM->ScopeBegin("CGameContext::RenderAllClients", true);

				/*CRC_Viewport VPNew = VP;
				CRct View(xw*w, yw*h, xw*w+w, yw*h+h);
				VPNew.SetView(VP.GetViewClip(), View);
				_pRC->Viewport_Set(&VPNew);*/

				RenderClient(_pVBM, _pRC, _GUIVP, m_lspWClients[i], _Context);
				_pVBM->ScopeEnd();
				_pVBM->Viewport_Pop();
			}
		}

		if(!bShare)
		{
			_pVBM->ScopeBegin("RenderSplitScreenGUI", true);
			RenderSplitScreenGUI(_pVBM, _pRC, bSplitX, bSplitY);
			_pVBM->ScopeEnd();
		}
	}
}

void CGameContext::GetViewport(CRC_Viewport* _vp, int _type)
{
	MAUTOSTRIP(CGameContext_GetViewport, MAUTOSTRIP_VOID);
	CVec2Dfp32 ViewScale;
	ViewScale[0] = (_type == 0) ? m_Viewport3DScaleX : 1.0f;
	ViewScale[1] = (_type == 0) ? m_Viewport3DScaleY : 1.0f;

	CWorld_Client* pC = GetCurrentClient();
	if (pC)
		ViewScale.CompMul(pC->Render_GetViewScaleMultiplier(), ViewScale);

	GetViewport(m_pSystem, _vp, _type, ViewScale, m_bVirtualWideScreen != 0, m_WideScreen);
}

fp32 CGameContext::DetermineWidescreen()
{	
	fp32 WideScreenFormat = 1.3333333333333333333333333333f * 1.3333333333333333333333333333f; // Double widescreen

	//	if (m_pSystem->m_spDisplay->IsWidescreen())
	//	{

	fp32 Format = WideScreenFormat / (m_pSystem->m_spDisplay->GetScreenAspect() * 1.333333333333333333333f);
	Format /= (m_pSystem->m_spDisplay->GetPixelAspect());
	if (GetCurrentClient() && (GetViewFlags() & XR_VIEWFLAGS_WIDESCREEN) != 0 && Format > 1.001f)
	{
		m_bVirtualWideScreen = true;
		GetCurrentClient()->SetViewFlags(XR_VIEWFLAGS_WIDESCREEN, Format);
	}
	else if (GetCurrentClient())
	{
		m_bVirtualWideScreen = false;
		GetCurrentClient()->SetViewFlags(0, Format);
	}
	m_WideScreen = Format;

	return Format;
	/*	}
	else
	{
	fp32 Format = WideScreenFormat / ((m_pSystem->m_spDisplay->GetScreenAspect() * 1.333333333333333333333f));
	if (GetCurrentClient() && m_bVirtualWideScreen && Format > 1.001f)
	{
	GetCurrentClient()->SetViewFlags(CWorld_Client::EViewFlags_VirtualWideScreen, Format);
	}
	m_WideScreen = Format;
	return Format;
	}*/
}


void CGameContext::GetViewport(CSystem* _pSystem, CRC_Viewport* _pVP, int _type, CVec2Dfp32 _Scale, bool _bVirtualWideScreen, fp32 _WideScreen)
{
	MAUTOSTRIP(CGameContext_GetViewport_2, MAUTOSTRIP_VOID);
	// NOTE: This function should be named AdjustViewport or something simiular. /mh

	CVec2Dfp32 _Border;
	switch(_type)
	{
	case 1:		// GUI
		{
			//			_Border[1] = _pSystem->GetEnvironment()->GetValuef("VID_BORDERY");
			/*			if(_pSystem->m_spDisplay)
			{
			fp32 Border = _pSystem->GetEnvironment()->GetValuef("VID_BORDERY");
			fp32 Aspect = _pSystem->m_spDisplay->GetScreenAspect();
			if (Aspect < 1.0)
			{
			fp32 ScreenSizeY = (1.0 - Border) * Aspect;
			fp32 ScreenSizeX = (1.0 - Border);
			_Border[0] = 1.0 - ScreenSizeX;
			_Border[1] = 1.0 - ScreenSizeY;
			}
			else
			{
			fp32 ScreenSizeY = 1.0 - Border;
			fp32 ScreenSizeX = (1.0 - Border) / Aspect;
			_Border[0] = 1.0 - ScreenSizeX;
			_Border[1] = 1.0 - ScreenSizeY;
			}
			}
			_pVP->SetBorder(_Border);*/

			fp32 Aspect = 1.0;
			CRct Rect = _pVP->GetViewRect();
			if(_pSystem->m_spDisplay)
			{
				CPnt ViewRect = _pSystem->m_spDisplay->GetScreenSize();
				Rect = CRct(0,0,ViewRect.x, ViewRect.y);
				Aspect = _pSystem->m_spDisplay->GetPixelAspect();
			}

			fp32 w = int(Rect.GetWidth()*_Scale.k[0] / 4) * 4;
			fp32 h = Rect.GetHeight()*_Scale.k[1];

			h = int(h / 4) * 4;

			CRct RectNew(0, 0, (int)w, (int)h);

			_Scale.k[0] = w / Rect.GetWidth();

			_Scale.k[1] = h / Rect.GetHeight();

			_pVP->SetAspectRatio(Aspect);
			fp32 AspectX;
			fp32 AspectY;
			AspectY = 1.0;
			AspectX = _Scale.k[0] / _Scale.k[1];

			_pVP->SetPixelAspect(AspectX, AspectY);

			_pVP->SetView(_pVP->GetViewClip(), RectNew);

		}
		break;

	default:	// Normal/3D
		{ 
			//_Border[0] = _pSystem->GetEnvironment()->GetValuef("VID_3D_BORDERX");
			//_Border[1] = _pSystem->GetEnvironment()->GetValuef("VID_3D_BORDERY");

			fp32 Aspect = 1.0;
			CRct Rect = _pVP->GetViewRect();
			if(_pSystem->m_spDisplay)
			{
				CPnt ViewRect = _pSystem->m_spDisplay->GetScreenSize();
				Rect = CRct(0,0,ViewRect.x, ViewRect.y);
				Aspect = _pSystem->m_spDisplay->GetPixelAspect();
			}

			fp32 w = int(Rect.GetWidth()*_Scale.k[0] / 4) * 4;
			//			fp32 w = Rect.GetWidth()*_Scale.k[0];
			fp32 h = Rect.GetHeight()*_Scale.k[1];
			//			fp32 midx = (Rect.p0.x + Rect.p1.x) >> 1;
			//			fp32 midy = (Rect.p0.y + Rect.p1.y) >> 1;

			if (_bVirtualWideScreen)
			{
				// Virtual widescreen with black borders
				h /= _WideScreen;
				//				Aspect *= 1.333333f;
			}

			h = int(h / 4) * 4;

			CRct RectNew(0, 0, (int)w, (int)h);

			_Scale.k[0] = w / Rect.GetWidth();
			if (_bVirtualWideScreen)
			{
				fp32 WideScreen = (1.0f - 1.0f / _WideScreen) * 0.5f;
				int Borders = (int)M_Ceil(Rect.GetHeight() * WideScreen);
				_Scale.k[1] = h / (Rect.GetHeight() - Borders*2);
			}
			else
				_Scale.k[1] = h / Rect.GetHeight();

			_pVP->SetAspectRatio(Aspect);
			fp32 AspectX;
			fp32 AspectY;
			AspectY = 1.0;
			AspectX = _Scale.k[0] / _Scale.k[1];
			//			if (_Scale.k[0] > _Scale.k[1])
			//			{
			///			}
			///			else
			///			{
			//				AspectX = 1.0f;
			//				AspectY = _Scale.k[1] / _Scale.k[0];
			//			}
			_pVP->SetPixelAspect(AspectX, AspectY);

			_pVP->SetView(_pVP->GetViewClip(), RectNew);

			//			_pVP->SetBorder(CVec2Dfp32(Width, Height) * fp32(0.5*(1.0f - _Scale)));
		}
	}
}

void CGameContext::Render_Corrupt(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP, int _Context)
{
	MAUTOSTRIP(CGameContext_Render_Corrupt, MAUTOSTRIP_VOID);

}

void CGameContext::Render(CXR_VBManager* _pVBM, CRenderContext* _pRC, int _Context)
{	
	MAUTOSTRIP(CGameContext_Render, MAUTOSTRIP_VOID);
	// Create Viewports
	CRC_Viewport _3DVP = *_pVBM->Viewport_Get();
	CRC_Viewport _GUIVP = *_pVBM->Viewport_Get();

	GetViewport(&_3DVP, 0);
	GetViewport(&_GUIVP, 1);

	Render(_pVBM, _pRC, _3DVP, _GUIVP, _Context);
}

void CGameContext::RenderGUI(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP, int _Context)
{
	MAUTOSTRIP(CGameContext_RenderGUI, MAUTOSTRIP_VOID);
	if (!m_spWData)
		return;

	if (CDiskUtil::GetCorrupt() & DISKUTIL_STATUS_CORRUPTANY)
	{
		Render_Corrupt(_pVBM, _pRC, _3DVP, _GUIVP, _Context);
		return;
	}

	// Render client-game
	if (m_spFrontEnd != NULL && (!_Context || (m_spFrontEnd->m_spDesktop && m_spFrontEnd->m_spDesktop->m_BlocksLoadingScreen)))
	{
		if (_pVBM->Viewport_Push(&_GUIVP))
		{
			M_LOCK(m_spFrontEnd->m_FrontEndLock);
			_pVBM->ScopeBegin("CGameContext::RenderGUI", true);
			CWorld_Client* pWClient = GetCurrentClient();
			m_spFrontEnd->SetRender_Engine(m_spEngine); //(pWClient) ? pWClient->Render_GetEngine() : NULL);
			m_spFrontEnd->OnRender(_pRC, _pVBM);
			m_spFrontEnd->SetRender_Engine(NULL);
			_pVBM->Viewport_Pop();
			_pVBM->ScopeEnd();
		}
	}


	if(m_spAsyncSaveContext && !m_spAsyncSaveContext->m_spWData->m_AsyncWriter.Done())
		m_LastWriteTime = CMTime::GetCPU();

	bool bGui = true;
/*	if (m_lspWClients.Len())	This is not thread safe. Need locking mechanism or lspWClients can change at any moment here.
	{
		bGui = !m_lspWClients[0]->HideHud();
	}*/

	if(((CMTime::GetCPU()-m_LastWriteTime).GetTime() < 0.15f) && bGui)
	{
		int iRcFont = m_spWData->GetResourceIndex("XFC:TEXT", NULL);
		CWRes_XFC* pRcFont = safe_cast<CWRes_XFC>(m_spWData->GetResource(iRcFont));
		CStr Text = "§Z16" + m_SaveInfoString;

		CRC_Font *pFont = NULL; //m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("HEADINGS"));
		if(pRcFont && (pFont = pRcFont->GetFont()))
		{
			_pVBM->ScopeBegin("SaveInfoString", false);
			CRC_Util2D Util2D;
			//			CRC_Viewport* pVP = _pRC->Viewport_Get();
			CRC_Viewport* pVP = _pVBM->Viewport_Get();
			Util2D.Begin(_pRC, pVP, _pVBM);
			CRct Rect = pVP->GetViewRect();
			Util2D.SetCoordinateScale(CVec2Dfp32(Rect.GetWidth() / 640.0f , Rect.GetHeight() / 480.0f));
			CClipRect Clip(0, 0, 640, 480);

			Util2D.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			Util2D.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE);

			int Style = WSTYLE_TEXT_WORDWRAP | WSTYLE_TEXT_SHADOW;
			wchar wText[1024];
			Localize_Str(Text, wText, 1023);
			fp32 w = Min(pFont->GetWidth(16, wText), 120.0f);
			//			fp32 h = pFont->GetHeight(16, wText);
			Util2D.Text_DrawFormatted(Clip, pFont, wText, (int)(640-640*0.15f/2-w), (int)(480*0.15f/2+1), Style, 0x80ffffff, 0, 0xff000000, 120, 100, false);

			Util2D.End();
			_pVBM->ScopeEnd();
		}
	}

	if( m_CutsceneStatus > 0 )
	{
		int iRcFont = m_spWData->GetResourceIndex("XFC:TEXT", NULL);
		CWRes_XFC* pRcFont = safe_cast<CWRes_XFC>(m_spWData->GetResource(iRcFont));
		//_pVBM->Begin(_pRC);

		CRC_Font *pFont = NULL; //m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("HEADINGS"));
		if(pRcFont && (pFont = pRcFont->GetFont()))
		{
			_pVBM->ScopeBegin("CGameContext::RenderGUI", false);
			CRC_Util2D Util2D;
			//			CRC_Viewport* pVP = _pRC->Viewport_Get();
			CRC_Viewport* pVP = _pVBM->Viewport_Get();
			Util2D.Begin(_pRC, pVP, _pVBM);
			CRct Rect = pVP->GetViewRect();
			Util2D.SetCoordinateScale(CVec2Dfp32(Rect.GetWidth() / 640.0f , Rect.GetHeight() / 480.0f));
			CClipRect Clip(0, 0, 640, 480);

			Util2D.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			Util2D.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
			m_CutsceneStatus--;
			Util2D.Text(Clip, pFont, 320, (int)(480*0.15f/2), "STARTING CUTSCENE", 0xffffffff, 16);
			Util2D.End();
			_pVBM->ScopeEnd();
		}
	}
}

void CGameContext::RenderClients(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP, int _Context)
{
	MAUTOSTRIP(CGameContext_RenderClients, MAUTOSTRIP_VOID);
	if (!m_spWData)
		return;

	if (CDiskUtil::GetCorrupt() & DISKUTIL_STATUS_CORRUPTANY)
	{
		return;
	}
	RenderAllClients(_pVBM, _pRC, _3DVP, _GUIVP, _Context);
}

class CCaptureVideoFrameCallback : public CXR_VBCallback
{
public:
	spCImage m_spTimeDemoCapture;
	spCImage m_spTimeDemoConv;
	spCVideoFile m_spTimeDemoFile;

	virtual void Callback(CRenderContext* _pRC, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, CXR_VBMScope* _pScope, int _Flags)
	{
		CImage* pImg = _pRC->GetDC()->GetFrameBuffer();
		if (pImg != NULL)
		{

			// Copy and convert from framebuffer
			CImage::Convert(pImg, m_spTimeDemoCapture, IMAGE_CONVERT_RGB);
			CImage::Convert(m_spTimeDemoCapture, m_spTimeDemoConv, IMAGE_CONVERT_RGB);

			// Store frame
			m_spTimeDemoFile->AddFrame(m_spTimeDemoConv);
		};

		this->~CCaptureVideoFrameCallback();
	}

};

void CGameContext::Render(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP, int _Context)
{
	MAUTOSTRIP(CGameContext_Render_2, MAUTOSTRIP_VOID);
	if (!m_spWData)
		return;

	MSCOPE(CGameContext::Render, GAMECONTEXT_RENDER);

	//	CWorld_Client* pC = GetCurrentClient();

	//	if (m_lspWClients.Len() && m_lspWClients[0] != NULL)
	//		RenderClient(_pRC, m_lspWClients[0]);

	// Create Viewports
	/*	CRC_Viewport _3DVP = *_pRC->Viewport_Get();
	CRC_Viewport _GUIVP = *_pRC->Viewport_Get();

	GetViewport(&_3DVP, 0);
	GetViewport(&_GUIVP, 1);*/

	// Render client(s)
	RenderClients(_pVBM, _pRC, _3DVP, _GUIVP, _Context);

	RenderGUI(_pVBM, _pRC, _3DVP, _GUIVP, _Context);

	// Is capturing enabled?
#if 1
#if !defined( PLATFORM_PS2 ) && !defined( PLATFORM_DOLPHIN )
	if ((m_Mode & WMODE_TIMEDEMO) && m_spTimeDemoFile && (m_TimeDemoCapturePath != ""))
	{
		if (m_nTimeDemoFramesPlayed > 5)
		{
			// Create temporary images if they dont exist.
			CImage* pImg = _pRC->GetDC()->GetFrameBuffer();
			int w = _pRC->GetDC()->GetScreenSize().x;
			int h = _pRC->GetDC()->GetScreenSize().y;
			int Fmt = IMAGE_FORMAT_BGR8;
			if (!m_spTimeDemoCapture)
			{
				m_spTimeDemoCapture = MNew(CImage);
				m_spTimeDemoCapture->Create(w, h, pImg->GetFormat(), IMAGE_MEM_IMAGE);
			}
			if (!m_spTimeDemoConv)
			{
				m_spTimeDemoConv = MNew(CImage);
				m_spTimeDemoConv->Create(w, h, Fmt, IMAGE_MEM_IMAGE);
			}
			_pVBM->ScopeBegin("CGameContext::Render(VideoFrameCallback)", true);
			CCaptureVideoFrameCallback *pCallback = _pVBM->AddCallbackClass<CCaptureVideoFrameCallback>(1000000.0);
			_pVBM->ScopeEnd();
			if (pCallback)
			{
				pCallback->m_spTimeDemoCapture = m_spTimeDemoCapture;
				pCallback->m_spTimeDemoConv = m_spTimeDemoConv;
				pCallback->m_spTimeDemoFile = m_spTimeDemoFile;
			}
		}
		else if (m_nTimeDemoFramesPlayed > 1)
		{
			_pRC->Texture_BlockUntilStreamingTexturesDone();
		}

	}
#endif
#endif
}

/*
void CGameContext::RenderFPSCounter(CRenderContext* _pRC, CRC_Util2D* _Util2D, const CClipRect& _ClipVP, fp32 _FPS, fp32 _AvgFPS, const char* _Win32MemoryStatusStr)
{
MAUTOSTRIP(CGameContext_RenderFPSCounter, MAUTOSTRIP_VOID);
if (!m_bShowFPS || !m_spWData)
return;

int iRcFont = m_spWData->GetResourceIndex("XFC:MINIFONT2", NULL);
CWRes_XFC* pRcFont = safe_cast<CWRes_XFC>(m_spWData->GetResource(iRcFont));
if (!pRcFont)
return;

CRC_Font* pFont = pRcFont->GetFont();
if (!pFont)
return;

fp32 nTriTotal				= _pRC->Attrib_GlobalGetVar(20);
CDA_MemoryManager* pMemMgr	= MRTC_GetMemoryManager();

CStr FPSStr, AvgFPSStr, TCntStr, TPSStr;
if(m_bSimpleFPS)
{
// ==========================
// Default simple fps-counter
FPSStr	  = CStrF("FPS				: %.2f",	_FPS);
AvgFPSStr = CStrF("Average FPS		: %.2f",	_AvgFPS);
TCntStr   = CStrF("Triangle Count	: %.2f",	nTriTotal);
TPSStr    = CStrF("Triangle Per Sec	: %d",		int(nTriTotal*_AvgFPS));
}
else
{

#ifdef PLATFORM_XBOX 
// =======================
// XBox normal fps-counter
FPSStr = CStrF("%.2f(%.2f, %d) %s GloH: %0.1f(%0.1f)/%0.1f U %0.1f ResH: %0.1f(%0.1f)/%0.1f U %0.1f", 
_FPS, _AvgFPS, int(nTriTotal*_AvgFPS), _Win32MemoryStatusStr, 

(float)pMemMgr->GetFreeMem() / (1024.0* 1024.0), 
(float)pMemMgr->GetLargestFreeMem() / (1024.0* 1024.0), 
(float)pMemMgr->m_AllocatedMem / (1024.0* 1024.0), 
(float)pMemMgr->GetUsedMem() / (1024.0* 1024.0)

, (float)MRTC_GetGraphicsHeap()->GetFreeMem() / (1024.0* 1024.0)
, (float)MRTC_GetGraphicsHeap()->GetLargestFreeMem() / (1024.0* 1024.0)
, (float)MRTC_GetGraphicsHeap()->m_AllocatedMem / (1024.0* 1024.0), 
(float)MRTC_GetGraphicsHeap()->GetUsedMem() / (1024.0* 1024.0)
);


#else 
#ifdef PLATFORM_WIN
// ==========================
// Windows normal fps-counter
FPSStr = CStrF("%.2f(%.2f, %d) %s GloH: %0.1f(%0.1f)/%0.1f U %0.1f", 
_FPS, _AvgFPS, int(nTriTotal*_AvgFPS), _Win32MemoryStatusStr, 
(float)pMemMgr->GetFreeMem() / (1024.0* 1024.0), 
(float)pMemMgr->GetLargestFreeMem() / (1024.0* 1024.0), 
(float)pMemMgr->m_AllocatedMem / (1024.0* 1024.0), 
(float)pMemMgr->GetUsedMem() / (1024.0* 1024.0)
);
#else	
// ==========================
// Default normal fps-counter
FPSStr = CStrF("%.2f(%.2f, %d)", _FPS, _AvgFPS, int(nTriTotal*_AvgFPS));
#endif
#endif	
}

_pRC->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
_pRC->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
#ifdef PLATFORM_XBOX 
_Util2D->SetCoordinateScale(CVec2Dfp32(1.5,1.5));
int y = 20;
#else
int y = 0;
#endif
if(!m_bSimpleFPS)
{
_Util2D->Text(_ClipVP, pFont, 0, y, (char*) FPSStr, 0xffffffff);
}
else
{
y += 20;
_Util2D->Text(_ClipVP, pFont, _ClipVP.GetWidth()-150, y,    (char*) FPSStr, 0xffffffff);
_Util2D->Text(_ClipVP, pFont, _ClipVP.GetWidth()-150, y+10, (char*) AvgFPSStr, 0xffffffff);
_Util2D->Text(_ClipVP, pFont, _ClipVP.GetWidth()-150, y+20, (char*) TCntStr, 0xffffffff);
_Util2D->Text(_ClipVP, pFont, _ClipVP.GetWidth()-150, y+30, (char*) TPSStr, 0xffffffff);
}	
}


bool CGameContext::ShowStats()
{
MAUTOSTRIP(CGameContext_ShowStats, false);
return m_bShowStats;
}
*/

void CGameContext::PlayDemo(CStr _Name, bool _bTimeDemo)
{
	MAUTOSTRIP(CGameContext_PlayDemo, MAUTOSTRIP_VOID);
	MSCOPE(CGameContext::PlayDemo, GAMECONTEXT);

	//	INIT_PAD(1000);

	//	if (!(m_Mode & WMODE_DEMO))
	MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
	if (pCon) pCon->ExecuteString("stream_stop(1)");

	Command_Disconnect();

	CreateClientGame();

	spCReferenceCount spObj = (CReferenceCount*) MRTC_GOM()->CreateObject(m_ClientClass);
	spCWorld_Client spC = safe_cast<CWorld_Client> ((CReferenceCount*)spObj);
	if (!spC) Error("PlayDemo", "Unable to create client-object.");

	CWC_CreateInfo CreateInfo;
	CreateInfo.m_Mode = WCLIENT_MODE_PRIMARY;
	CreateInfo.m_spGameReg = m_spGameReg;
	CreateInfo.m_spWData = m_spWData;
	CreateInfo.m_spEngine = m_spEngine;
	spC->Create(CreateInfo);

	m_lspWClients.Add(spC);

	spC->Demo_Get(_Name);
	spC->Sound_SetSoundContext(m_spSoundContext);
	spC->Precache_Init();

	m_Mode = WMODE_LOCALCLIENT | WMODE_DEMO | WMODE_GAMELOADED;
	if (_bTimeDemo) m_Mode |= WMODE_TIMEDEMO;
	StartSimulation();

	m_TimeDemoStart.Reset();
	m_nTimeDemoFramesPlayed = 0;
	m_nTimeDemoMaxFrames = 0x7fffffff;
	m_bTimeDemoGraph = 0;

	if (pCon) pCon->ExecuteString("cg_clearmenus()");

	g_bPlayingDemoMusic = false;
	//	CHECK_PAD(PlayDemo 7);
}

void CGameContext::Capture(CStr _Path, int _FPS)
{
	MAUTOSTRIP(CGameContext_Capture, MAUTOSTRIP_VOID);
	MSCOPE(CGameContext::Capture, GAMECONTEXT);

	if (!(m_Mode & WMODE_DEMO))
	{
		ConOut("Can only capture in demo playback mode.");
		return;
	}
	//	if (_Path.Copy(_Path.Len()-1, 1) != "\\")
	//		_Path += "\\";
	m_Mode |= WMODE_TIMEDEMO;
	m_TimeDemoRate = 1.0 / fp64(_FPS);

	ConExecute(CStrF("videofilebegin (\"%s\", %d)", _Path.Str(), _FPS ));

#if 1
	CStr File;
#ifdef PLATFORM_XBOX1
	CDiskUtil::CreatePath("T:\\DemoImages");
	File = CStr("T:\\DemoImages\\") + _Path;
#elif defined(PLATFORM_XENON)
	MACRO_GetSystem;
	CDiskUtil::CreatePath(pSys->m_ExePath + "DemoImages");
	File = pSys->m_ExePath + CStr("DemoImages\\") + _Path;
	CDiskUtil::CreatePath(pSys->m_ExePath + "DemoImages");
#else
	File = (_Path.GetDevice().Len()) ? _Path : ResolvePath(_Path);
#endif
	m_TimeDemoCapturePath = File;

	ConOut("Capturing to " + File);

	CImage* pImg = m_pSystem->m_spDisplay->GetFrameBuffer();
	if (!pImg) return;

#if !defined( PLATFORM_PS2 ) && !defined( PLATFORM_DOLPHIN )
	m_spTimeDemoFile = MCreateVideoFile();
	if (!m_spTimeDemoFile) return;
	if (!m_spTimeDemoFile->Create(File, pImg->GetWidth(), pImg->GetHeight(), _FPS, 10))
	{
		ConOutL("(CGameContext::Capture) Video aborted.");
		m_spTimeDemoFile = NULL;
	}

	//	Create(CStr _FileName, int _Width, int _Height, fp32 _FrameRate, int _KeyFrame = 1) = 0;


	//	CStr Path = (_Path.GetDevice().Len()) ? _Path : m_WorldPath + m_TimeDemoCapturePath;
	//	m_spTimeDemoFile = Path
	//	CDiskUtil::CreatePath(Path);
#endif
#endif
}

// -------------------------------------------------------------------
//  Deferred commands
// -------------------------------------------------------------------
void CGameContext::Command_Map(CStr _Name)
{
	MAUTOSTRIP(CGameContext_Command_Map, MAUTOSTRIP_VOID);
	Command_ChangeMap(_Name, "");
}


//
// Bogus protected function
// 
// Always returns 0
//

int32 CheckMap(void *_pSys)
{
	return 0;
}


void CGameContext::Command_ChangeMap(CStr _Name, CStr _Flags)
{
#ifdef M_DEMO
	if (_Name.CompareNoCase("pa1_mainframe") != 0)
		return;
#endif
	MAUTOSTRIP(CGameContext_Command_ChangeMap, MAUTOSTRIP_VOID);
	MSCOPE(CGameContext::Command_ChangeMap, GAMECONTEXT);

	MACRO_GetRegisterObject(CXR_VBMContainer, pVBMContainer, "SYSTEM.VBMCONTAINER");
	if(pVBMContainer)
	{
		pVBMContainer->BlockUntilAllVBMFree();
	}

	StopSimulation();

	if(_Name.Find(".xw") == -1)
		_Name += ".xw";

	CStr FileName = m_spWData->ResolveFileName(CStrF("worlds\\%s", _Name.Str()));
	CStr FileNameXDF = m_spWData->ResolveFileName(CStrF("XDF\\%s_Server.xdf", _Name.GetFilenameNoExt().Str()));
	if (!CDiskUtil::FileExists(FileName) && !CDiskUtil::FileExists(FileNameXDF))
	{
#ifdef PLATFORM_CONSOLE
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
#endif
		ConOutL("World doesn't exist: " + FileName);
		return;
	}
	FileName = "";

	if (!m_spWServer)
	{
		Command_Disconnect();
		if(_Flags.Val_int() & SERVER_CHANGEWORLD_INITNETWORK)
			InitServer(true, m_NetDeviceSettings);
		else
			InitServer();

		if (!m_spWServer) return;
	}
	else
	{
		CStr GameType = m_spGameReg->GetValue("DEFAULTGAME", "Debug");
		CStr ServerGameType = m_spWServer->m_GameType;
		if(ServerGameType.CompareNoCase(GameType) != 0)
		{
			Command_Disconnect();
			if(_Flags.Val_int() & SERVER_CHANGEWORLD_INITNETWORK)
				InitServer(true, m_NetDeviceSettings);
			else
				InitServer();
			if (!m_spWServer) return;
		}		
	}

	for(int i = 0; i < m_lspWClients.Len(); i++)
		if (m_lspWClients[i] != NULL)
		{
			CWorld_ClientCore* pC = safe_cast<CWorld_ClientCore>((CWorld_Client*)m_lspWClients[i]);
			if (pC)
				pC->World_Close();
		}

		// Reset progressbar
		m_spWServer->Load_ProgressReset();
		int32 Len = m_lspWClients.Len();
		for (int32 i = 0; i< Len; i++)
		{
			if (m_lspWClients[i])
				m_lspWClients[i]->Load_ProgressReset();
		}

		// Init default spawnflags
		if (!(_Flags.Val_int() & SERVER_CHANGEWORLD_KEEPGAME))
		{
			uint32 SpawnFlags = m_spWServer->World_TranslateSpawnFlags( m_spGameReg->GetValue("DEFAULTSPAWNFLAGS") );

			if (!SpawnFlags) // default: "ny1+layer1+once"
				SpawnFlags |= M_Bit(SERVER_SPAWNFLAGS_GLOBAL_BASE) | M_Bit(SERVER_SPAWNFLAGS_LAYERS_BASE) | SERVER_SPAWNFLAGS_ONCE; 

			if (!(SpawnFlags & SERVER_SPAWNFLAGS_DIFFICULTY_MASK))
				SpawnFlags |= M_Bit(SERVER_SPAWNFLAGS_DIFFICULTY_BASE);	// default: "easy"

			m_spWServer->m_CurSpawnMask = SpawnFlags;
		}

#ifndef M_RTM
		{
			CRegistry* pServerReg = m_spWServer->Registry_GetServer();
			if (pServerReg)
			{
				// Remove old debug objects
				pServerReg->DeleteKey("agdbgobj");
			}
		}
#endif

		//if(!(m_spFrontEnd != NULL && m_spFrontEnd->m_spDesktop != NULL && m_spFrontEnd->m_spDesktop->m_BlocksLoadingScreen))
		//	ConExecute("cg_clearmenus()");

#ifndef PLATFORM_WIN_PC
		ConExecute("stream_stop(1)");
#endif

		M_TRY
		{
			m_spWServer->World_Change(_Name, _Flags.Val_int());
		}
		M_CATCH(
			catch(CCExceptionFile)
		{
			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
			Disconnect();
			return;
		}
		)
#ifdef M_SUPPORTSTATUSCORRUPT
			M_CATCH(
			catch(CCException)
		{
			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
			Disconnect();
			return;
		}
		)
#endif

			if (!GetCurrentClient())
				InitLocalGame();

		StartSimulation();
}


void CGameContext::Command_Disconnect()
{
	MAUTOSTRIP(CGameContext_Command_Disconnect, MAUTOSTRIP_VOID);
	MSCOPE(CGameContext::Command_Disconnect, GAMECONTEXT);
	Disconnect();
}

void CGameContext::Command_Connect(CStr _Addr)
{
	MAUTOSTRIP(CGameContext_Command_Connect, MAUTOSTRIP_VOID);
	StopSimulation();
	M_TRY
	{ 
		Connect(_Addr); 
	}
	M_CATCH(
		catch(CCException)
	{
		Command_Disconnect();
	}
	)

		if (!m_lspWClients.Len())
		{
			Command_Disconnect();
		}
}

void CGameContext::Command_SetGame(CStr _WorldPathes, CStr _GameName)
{
	MAUTOSTRIP(CGameContext_Command_SetGame, MAUTOSTRIP_VOID);
	SafeSetGame(_WorldPathes, _GameName);
}

void CGameContext::Command_LoadGame(CStr _GameName)
{
	MAUTOSTRIP(CGameContext_Command_LoadGame, MAUTOSTRIP_VOID);
	MSCOPE(CGameContext::Command_LoadGame, GAMECONTEXT);

	MACRO_GetRegisterObject(CXR_VBMContainer, pVBMContainer, "SYSTEM.VBMCONTAINER");
	if(pVBMContainer)
	{
		pVBMContainer->BlockUntilAllVBMFree();
	}


	if(m_spWServer)
	{
		if(!m_spWServer->World_SaveExist(_GameName))
		{
			ConOutL("Savegame does not exist: " + _GameName);
			return;
		}

		if(m_spWServer->GetServerMode() == SERVER_MODE_SPAWNWORLD)
		{
			ConOutL("Can't load during server spawn");
			return;
		}

		CWorld_Client *pClient = GetCurrentClient();
		if(pClient)
		{
			int State = pClient->GetClientState();
			if(State == WCLIENT_STATE_CHANGELEVEL || State ==  WCLIENT_STATE_PRECACHE)
			{
				ConOutL("Can't load during client changelevel/precache");
				return;
			}
		}

		m_spWServer->Load_ProgressReset();
	}

	for(int i = 0; i < m_lspWClients.Len(); i++)
		if (m_lspWClients[i] != NULL)
			m_lspWClients[i]->Load_ProgressReset();

	{
		CWorld_Client* pC = GetCurrentClient();

		if (m_spWServer != NULL && pC)
		{
			for(int k = 0; k < 8; k++)
			{
				m_spWServer->Net_Refresh();
				pC->Refresh();
			}
		}
	}

	for(int i = 0; i < m_lspWClients.Len(); i++)
		if (m_lspWClients[i] != NULL)
		{
			CWorld_ClientCore* pC = safe_cast<CWorld_ClientCore>((CWorld_Client*)m_lspWClients[i]);
			if (pC)
				pC->World_Close();
		}

		if (!m_spWServer)
		{
			InitLocalGame();

			if(!m_spWServer->World_SaveExist(_GameName))
			{
				ConOutL("Savegame does not exist: " + _GameName);
				return;
			}
		}

		M_TRY
		{
			m_spWServer->World_Load(_GameName);

			MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
			if(pCon)
				pCon->ExecuteString("cg_clearmenus()");
		}
		M_CATCH(
			catch(CCExceptionFile)
		{
			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
			ConOutL("Failed to load game " + _GameName);
			Command_Disconnect();
		}
		)
			M_CATCH(
			catch(CCException)
		{
			ConOutL("Failed to load game " + _GameName);
			Command_Disconnect();
		}
		)
}

void CGameContext::Command_FreeResources()
{
	MAUTOSTRIP(CGameContext_Command_FreeResources, MAUTOSTRIP_VOID);
	MSCOPE(CGameContext::Command_FreeResources, GAMECONTEXT);

M_TRACEALWAYS("[GameContext] ignoring Command_FreeResources... (it's evil!)\n");
return;

	// This is sort of temporary just to fix the worst memory problems on the Xbox.

	if (!m_spWData) return;

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys)
	{
		int nRc = m_spWData->Resource_GetMaxIndex() + 1;
		for(int i = 0; i < nRc; i++)
		{
			if (m_spWData->Resource_IsValid(i))
			{
				const char* pPrefix = m_spWData->Resource_GetClassPrefix(i);
				if (pPrefix)
				{
					if (strcmp(pPrefix, "BSP") == 0 ||
						strcmp(pPrefix, "XW2") == 0 ||
						strcmp(pPrefix, "XWD") == 0 ||
						strcmp(pPrefix, "XWI") == 0 ||
						strcmp(pPrefix, "XWG") == 0 ||
						strcmp(pPrefix, "XMD") == 0 ||
						strcmp(pPrefix, "CTM") == 0 ||
						strcmp(pPrefix, "CMF") == 0 ||
						strcmp(pPrefix, "ANM") == 0 ||
						strcmp(pPrefix, "SND") == 0 ||
						strcmp(pPrefix, "TPL") == 0 ||
						strcmp(pPrefix, "DLG") == 0 ||
						strcmp(pPrefix, "ANL") == 0)
					{
						LogFile((char*)CStrF("(CGameContext::FreeResources) Deleting %s", m_spWData->Resource_GetName(i)));
						m_spWData->Resource_Delete(i);
					}
				}
			}
		}
	}

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (pTC)
	{
		int nTxt = pTC->GetIDCapacity();
		for(int i = 0; i < nTxt; i++)
		{
			int Flags = pTC->GetTextureFlags(i);
			if ((Flags & CTC_TXTIDFLAGS_PRECACHE) && !(Flags & CTC_TXTIDFLAGS_RESIDENT))
				pTC->SetTextureParam(i, CTC_TEXTUREPARAM_CLEARFLAGS, CTC_TXTIDFLAGS_PRECACHE);
		}
	}

	MACRO_GetRegisterObject(CWaveContext, pWC, "SYSTEM.WAVECONTEXT");
	if (pWC)
	{
		pWC->ClearWaveIDFlags(CWC_WAVEIDFLAGS_PRECACHE);
	}
}

void CGameContext::Command_Script(CStr _Script)
{
	MAUTOSTRIP(CGameContext_Command_Script, MAUTOSTRIP_VOID);
	M_TRY
	{
		m_pSystem->m_spCon->ExecuteString(_Script);
	}
	M_CATCH(
		catch(CCException)
	{
	}
	)
}

void CGameContext::Command_PlayCutsceneSound(CStr _Name, CStr _BeepSync)
{
	if( m_lspWClients.Len() == 0 )
		return;

	CWorld_ClientCore* pCore = safe_cast<CWorld_ClientCore>( (CWorld_Client*)m_lspWClients[0] );
	if(pCore && pCore->m_spSound)
	{
		Con_Pause(1);
		do
		{
			int iSyncedSound = m_spWServer->GetMapData()->GetResourceIndex_Sound(_Name);

			CSC_SFXDesc *pDesc = pCore->GetMapData()->GetResource_SoundDesc(iSyncedSound);
			if(!pDesc) break;

			// Get wave id
			int32 iW = pDesc->GetPlayWaveId();
			if(iW < 0) break;

			// start the new sound and set proper parameters
			int32 WaveID = pDesc->GetWaveId(iW);
			if(WaveID <= 0) break;

			CSC_VoiceCreateParams CreateParams;
			pCore->m_spSound->Voice_InitCreateParams(CreateParams, pDesc->GetCategory());
			CreateParams.m_Volume *= pDesc->GetVolume() + Random*pDesc->GetVolumeRandAmp();
			CreateParams.m_hChannel = pCore->m_hChannels[WCLIENT_CHANNEL_CUTSCENE];
			uint32 Prio = pDesc->GetPriority();
			if (Prio)
				CreateParams.m_Priority = Prio;
			CreateParams.m_WaveID = WaveID;
			CreateParams.m_Pitch *= pDesc->GetPitch() + Random*pDesc->GetPitchRandAmp();
			CreateParams.m_bStartPaused = true;

			int hVoice = pCore->m_spSound->Voice_Create(CreateParams);

			while( _BeepSync.Len() > 0 )
			{
				int iBeepSound = m_spWServer->GetMapData()->GetResourceIndex_Sound(_BeepSync);

				CSC_SFXDesc *pBeepDesc = pCore->GetMapData()->GetResource_SoundDesc(iBeepSound);
				if(!pBeepDesc) break;

				// Get wave id
				int32 iBeepW = pBeepDesc->GetPlayWaveId();
				if(iBeepW < 0) break;

				// start the new sound and set proper parameters
				int32 BeepWaveID = pBeepDesc->GetWaveId(iBeepW);
				if(BeepWaveID <= 0) break;

				CSC_VoiceCreateParams CreateParams;
				pCore->m_spSound->Voice_InitCreateParams(CreateParams, pBeepDesc->GetCategory());
				CreateParams.m_Volume *= pBeepDesc->GetVolume() + Random*pBeepDesc->GetVolumeRandAmp();
				CreateParams.m_hChannel = pCore->m_hChannels[WCLIENT_CHANNEL_CUTSCENE];
				uint32 Prio = pBeepDesc->GetPriority();
				if (Prio)
					CreateParams.m_Priority = Prio;
				CreateParams.m_WaveID = WaveID;
				CreateParams.m_Pitch *= pBeepDesc->GetPitch() + Random*pBeepDesc->GetPitchRandAmp();
				CreateParams.m_bStartPaused = true;
				int hBeepVoice = pCore->m_spSound->Voice_Create(CreateParams);

				pCore->m_spSound->Voice_Unpause(hBeepVoice);

				m_CutsceneStatus = 30;
				break;
			}

			pCore->m_spSound->Voice_Unpause(hVoice);
		} while( 0 );
		Con_Pause(0);
	}
}

void CGameContext::FixProfile(CStr &_rProfile)
{
	if(_rProfile.Len())
		return;

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	_rProfile = pSys->GetOptions()->GetValue("GAME_PROFILE");
	if(_rProfile == "")
		_rProfile = "default";
}


int32 CGameContext::EnumProfiles(TArray<CStr> &_rProfiles)
{
#if defined(M_DEMO_XBOX) // NOT supported
	return 0;
#elif defined(PLATFORM_XBOX1)
	XGAME_FIND_DATA FindData;
	const char *pSaveDrv = "U:\\";
	HANDLE hSearch = XFindFirstSaveGame(pSaveDrv, &FindData);
	BOOL Continue = hSearch != INVALID_HANDLE_VALUE;
	while(Continue)
	{
		CStr Name = FindData.szSaveGameName;
		_rProfiles.Add(Name);
		Continue = XFindNextSaveGame(hSearch, &FindData);
	}
	if( hSearch != INVALID_HANDLE_VALUE )
		XFindClose(hSearch);

#elif defined( PLATFORM_PS2 )
#warning "Implement CGameContextMod::EnumSaveGames"
	return;
#elif defined( PLATFORM_DOLPHIN )
#warning "Implement CGameContextMod::EnumSaveGames"
	return;
#else
	if(m_spWData == NULL)
		return 0;

	MACRO_GetSystem;

	CStr SavePath = pSys->GetRegistry()->GetValue("SAVEPATH");

	if (SavePath != pSys->m_ExePath)
	{
		CStr Path = pSys->TranslateSavePath(m_spWData->ResolvePath("Save\\")) + "*";
		CStr Pather = Path.GetPath();

		CDirectoryNode FindDirs;

		FindDirs.ReadDirectory(Path);			

		for(int32 i = 0; i < FindDirs.GetFileCount(); i++)
		{
			CStr File = FindDirs.GetFileName(i);
			if(File != "." && File != ".." && FindDirs.IsDirectory(i))
			{
				_rProfiles.Add(File);
			}
		}
	}
	else
	{
		TArray_SimpleSortable<CStr> Dirs = m_spWData->ResolveDirectory("Save\\*", true);

		for(int32 i = 0; i < Dirs.Len(); i++)
		{
			if(Dirs[i] != "." && Dirs[i] != "..")
			{
				_rProfiles.Add(Dirs[i]);
			}
		}
	}
#endif

	return _rProfiles.Len();
}

bool CGameContext::ValidateProfile(CStr _Profile)
{
	FixProfile(_Profile);

	TArray<CStr> lFiles;
	EnumSaveFiles(_Profile, lFiles, true);
	if(lFiles.Len() == 0 )
		return false;
	for(int32 i = 0; i < lFiles.Len(); i++)
		if(!ValidateSaveFile(_Profile, lFiles[i]))
			return false;

	// [INSERT CODE]
	return true;
}

CStr CGameContext::GetProfilePath(CStr _Profile)
{
	if (m_CurrentProfile != _Profile)
	{
		CStr Path;
#if defined (PLATFORM_XBOX1)
		CFStr TempPath;

#if defined(M_DEMO_XBOX)
		Path = "Z:\\Save\\" + _Profile+"\\";
		CDiskUtil::CreatePath(Path);
		//	const char *pSaveDrv = "Z:\\Saves\\";
#else
		const char *pSaveDrv = "U:\\";
		if(XCreateSaveGame(pSaveDrv, _Profile.Unicode().StrW(), OPEN_EXISTING, 0, TempPath, TempPath.GetMax()) != ERROR_SUCCESS)
			return "";
		Path = TempPath.Str();
#endif

#else
		MACRO_GetSystem;
		Path = pSys->TranslateSavePath(m_spWData->ResolvePath("Save\\") + _Profile + "\\");
#endif

		m_CurrentProfile = _Profile;
		m_CurrentProfilePath = Path;
	}
	return m_CurrentProfilePath;
}


int32 CGameContext::EnumSaveFiles(CStr _Profile, TArray<CStr> &_rSaveFiles, bool _bAll)
{
	FixProfile(_Profile);

	m_lSaveFiles.Clear();
	CStr Path = GetProfilePath(_Profile); // holds the savegame path

	CDirectoryNode Dir;
	Dir.ReadDirectory(Path + "*");
	int nf = Dir.GetFileCount();
	for(int i = 0; i < nf; i++)
	{
		CDir_FileRec* pF = Dir.GetFileRec(i);
		if(!Dir.IsDirectory(i))
		{
			if(pF->m_Name.GetFilenameExtenstion() != "xbx" && (_bAll || pF->m_Name[0] != '_'))
			{
				_rSaveFiles.Add(pF->m_Name);
			}

			m_lSaveFiles.Add(pF->m_Name);
		}
	}

	return _rSaveFiles.Len();
	// Nuke previous files before saving game.
	//if(CDiskUtil::FileExists(Path + _Name))
	//	CDiskUtil::DelFile(Path + _Name);
}

// dllvirtual void ReadSaveFile(spCWorldData _pWData, CStr _Profile, CStr _Name, TArray<uint8> _lData);
// dllvirtual void WriteSaveFile(spCWorldData _pWData, CStr _Profile, CStr _Name, TArray<uint8> _lData);

bool CGameContext::FillProfile(CStr _Profile)
{
	if (!m_spAsyncSaveContext)
		return 0;

	FixProfile(_Profile);
	m_spAsyncSaveContext->m_Directory = GetProfilePath(_Profile);
	m_spAsyncSaveContext->m_CalcSizeSearchString = GetProfilePath(_Profile) + "*";
	m_spAsyncSaveContext->m_spWData->m_AsyncWriter.FillDirecotry(&m_spAsyncSaveContext->m_AsyncOptions);

	/*	const int32 WantedSize = 80*(16*1024); // 1.25MiB (-3 because of padding)
	FixProfile(_Profile);
	CStr Path = GetProfilePath(_Profile);
	int Size = WantedSize;
	int32 ClusterSize = 1;

	#ifdef PLATFORM_XBOX
	ClusterSize = XGetDiskClusterSize(Path);
	//ClusterSize -= ClusterSize/2;
	Size -= XGetDisplayBlocks(Path)*(16*1024)+8*1024; // consume only half a block.. safty measure
	#else
	// get current size
	CDirectoryNode Dir;
	Dir.ReadDirectory(Path + "*");
	int nf = Dir.GetFileCount();
	for(int i = 0; i < nf; i++)
	{
	CDir_FileRec *pF = Dir.GetFileRec(i);
	if(!Dir.IsDirectory(i))
	Size -= (pF->m_Size+ClusterSize-1)*ClusterSize/ClusterSize;
	}
	#endif
	//#else
	// write file
	BeginWriteSaveFile(_Profile, "_filler");

	const int32 BlockSize = 1024*16;
	static uint8 aData[BlockSize] = {0};

	while(Size > 0)
	{

	int32 ChunkSize = BlockSize;
	if(ChunkSize > Size)
	ChunkSize = Size;
	ChunkSize = (ChunkSize+3)/4*4;
	Size -= ChunkSize;

	WriteSaveFileBlock(aData, ChunkSize, true);

	//WriteSaveFileBlock(aData, ChunkSize);
	//Filer.Write(aData, ChunkSize);
	}
	EndWriteSaveFile(false);
	*/
	return true;
}

bool CGameContext::RemoveProfileFiller(CStr _Profile)
{
	if (!m_spAsyncSaveContext)
		return false;
	FixProfile(_Profile);
	CStr Path = GetProfilePath(_Profile);
	//	m_AsyncWriter.BlockUntilDone();
	m_spAsyncSaveContext->m_spWData->m_AsyncWriter.DelFile(Path + "_filler???");
	//	if(CDiskUtil::FileExists(Path + "_filler"))
	//		CDiskUtil::DelFile(Path + "_filler");
	return true;
}


CCrcCheck g_Crc;
bool CGameContext::BeginWriteSaveFile(CStr _Profile, CStr _Name)
{
	if (!m_spAsyncSaveContext)
		return false;
	FixProfile(_Profile);

	{
		bool Exist = false;
		for(int32 i = 0; i < m_lSaveFiles.Len(); i++)
		{
			if(m_lSaveFiles[i] == _Name)
			{
				Exist = true;
				break;
			}
		}
		if(!Exist)
			m_lSaveFiles.Add(_Name);
	}

	g_Crc = CCrcCheck();
	M_TRY
	{
		//m_AsyncWriter.BlockUntilDone();

		CStr Path = GetProfilePath(_Profile); // holds the savegame path
		RemoveProfileFiller(_Profile);

		// Nuke previous files before saving game.
		m_spAsyncSaveContext->m_spWData->m_AsyncWriter.DelFile(Path + _Name);

		// Write save game to disk
		m_CurrentSaveFilename = Path + _Name;
		//m_CurrentSaveFile.OpenExt(Path + _Name, CFILE_WRITE | CFILE_BINARY | CFILE_NODEFERCLOSE);
		m_CurrentSaveProfile = _Profile;
	}
	M_CATCH(
		catch(CCException _Ex)
	{
		// Corrupt Save
		//  We need to inform the player abt it!!
		ConOutL("§cf80WARNING: (CWorld_DeltaGameState::World_Save) Exception during save."); 	
		throw;
		//Error("CWorld_DeltaGameState::World_Save", _Ex.GetExceptionInfo().GetString());
	}
	)

		return true;
}

bool CGameContext::WriteSaveFileBlock(const uint8 *_pData, int32 _Size, bool _UpdateCrc)
{

	CFileAsyncWriteChunk Chunk;// = DNew(CFileAsyncWriteChunk) CFileAsyncWriteChunk;
	int32 Aligned = (_Size+3)/4*4;
	Chunk.m_pMemory = _pData;
	Chunk.m_MemorySize = Aligned;

	m_lPendingChunks.Add(Chunk);

	if(_UpdateCrc)
		g_Crc.UpdateCrc(_pData, Aligned);


	//m_CurrentSaveFile.Write(_pData, _Size);
	return true;
}


bool CGameContext::EndWriteSaveFile(bool _WriteFiller)
{
	if (!m_spAsyncSaveContext)
		return false;
	uint8 *pCrcCalc = DNew(uint8) uint8[CHECKSUM_BYTELENGTH];
	g_Crc.GetChecksumList(pCrcCalc);
	m_lCRCData.Add(pCrcCalc);

	WriteSaveFileBlock(pCrcCalc, CHECKSUM_BYTELENGTH, false);
	TArray<CFileAsyncWriteChunk> lChunks;
	for(int32 i = 0; i < m_lPendingChunks.Len(); i++)
		lChunks.Add(m_lPendingChunks[i]);
	int32 index = m_llChunks.Add(lChunks);
	m_spAsyncSaveContext->m_spWData->m_AsyncWriter.SaveFile(m_CurrentSaveFilename, m_llChunks[index].GetBasePtr(), m_llChunks[index].Len());

	m_lPendingChunks.Clear();

	if (_WriteFiller)
		FillProfile(m_CurrentSaveProfile);

	//	if(_WriteFiller)
	//		m_lFillProfiles.Add(m_CurrentSaveProfile);

	return true;
}

/*
bool CGameContext::WriteSaveFile(CStr _Profile, CStr _Name, , uint8 *_pData, int32 _Size)
{
MAUTOSTRIP(CGameContext_WriteSaveFile, MAUTOSTRIP_VOID);
MSCOPE(CGameContext::WriteSaveFile, WSERVERMOD);

FixProfile(_Profile);

// Cryptate data and add checksum
CCrcCheck Crc;
//uint8 *pData = _lData.GetBasePtr();
for(int i = 0; i < _Size; i++)
{
Crc.UpdateCrc(_pData[i]);
_pData[i] ^= 42;
}

// Add the checksum last in the file
uint8 CrcCalc[CHECKSUM_BYTELENGTH];
Crc.GetChecksumList(CrcCalc);
//for(int j = 0; j < CHECKSUM_BYTELENGTH; j++)
//	_lData.Add(CrcCalc[j]);

try
{
CStr Path; // holds the savegame path

#if defined (PLATFORM_XBOX)
CFStr TempPath;

#if defined(M_DEMO_XBOX)
Path = "Z:\\Save\\" + _Profile+"\\";
CDiskUtil::CreatePath(Path);
//	const char *pSaveDrv = "Z:\\Saves\\";
#else
const char *pSaveDrv = "U:\\";
if(XCreateSaveGame(pSaveDrv, _Profile.Unicode().StrW(), OPEN_EXISTING, 0, TempPath, TempPath.GetMax()) != ERROR_SUCCESS)
return false;
Path = TempPath;
#endif

#else
Path = _pWData->ResolvePath("Save\\") + _Profile + "\\";
CDiskUtil::CreatePath(Path);
#endif

// Nuke previous files before saving game.
if(CDiskUtil::FileExists(Path + _Name))
CDiskUtil::DelFile(Path + _Name);

// Write save game to disk
{
CCFile File;
File.OpenExt(Path + _Name, CFILE_WRITE | CFILE_BINARY | CFILE_NODEFERCLOSE);
File.Write(_pData, _Size);
File.Close();
}

}
catch(CCException _Ex)
{
// Corrupt Save
//  We need to inform the player abt it!!
ConOutL("§cf80WARNING: (CWorld_DeltaGameState::World_Save) Exception during save."); 	
throw;
//Error("CWorld_DeltaGameState::World_Save", _Ex.GetExceptionInfo().GetString());
}

return true;
}*/


bool CGameContext::ValidateSaveFile(CStr _Profile, CStr _Name)
{
	const int32 BlockSize = 1024*4;
	g_Crc = CCrcCheck();

	FixProfile(_Profile);

	bool bReturn = true;
	do
	{
		// open file
		if(!BeginReadSaveFile(_Profile, _Name, false))
		{
			bReturn = false;
			break;
		}	

		// process all data
		uint8 aBuffer[BlockSize];
		if (_Name.Left(7).CompareNoCase("_filler") == 0)
		{

			int32 BytesLeft;
			int32 FileSize;
			BytesLeft = FileSize = m_CurrentSaveFile.Length();
			if(BytesLeft <= 0)
			{
				bReturn = false;
				break;
			}
			int32 SizeCheck = 0;

			uint32 *pBuffer = (uint32 *)aBuffer;
			while(BytesLeft)
			{
				int32 CurrentBlockSize = Min(BytesLeft, BlockSize);
				BytesLeft -= CurrentBlockSize;
				m_CurrentSaveFile.Read(aBuffer, CurrentBlockSize);

				int i = 0;
				if (!SizeCheck)
				{
					if (*pBuffer != FileSize)
					{
						bReturn = false;
						break;
					}
					SizeCheck = 16*1024;
					i = 1;
				}

				SizeCheck -= CurrentBlockSize;

				CurrentBlockSize /= 4;

				for (; i < CurrentBlockSize; ++i)
				{
					if (pBuffer[i] != 0x80808080)
					{
						bReturn = false;
						break;
					}
				}

				if (!bReturn)
					break;
			}
		}
		else
		{
			int32 BytesLeft = m_CurrentSaveFile.Length()-CHECKSUM_BYTELENGTH;

			if(BytesLeft < 0)
			{
				bReturn = false;
				break;
			}	

			while(BytesLeft)
			{
				int32 CurrentBlockSize = BytesLeft < BlockSize ? BytesLeft : BlockSize;
				BytesLeft -= CurrentBlockSize;
				m_CurrentSaveFile.Read(aBuffer, CurrentBlockSize);

				g_Crc.UpdateCrc(aBuffer, CurrentBlockSize);
			}

			// read the CRC and close
			m_CurrentSaveFile.Read(aBuffer, CHECKSUM_BYTELENGTH);

			uint8 aCRCNum[CHECKSUM_BYTELENGTH];
			g_Crc.GetChecksumList(aCRCNum);

			for(int32 i = 0; i < CHECKSUM_BYTELENGTH; i++)
			{
				if(aCRCNum[i] != aBuffer[i])
				{
					bReturn = false;
					break;
				}
			}
		}
	}
	while (0);

	EndReadSaveFile();

	if (bReturn)
		M_TRACEALWAYS("SAVE VALIDATION: '%s/%s' passed validation\n", _Profile.Str(), _Name.Str());
	else
		M_TRACEALWAYS("SAVE VALIDATION: '%s/%s' failed validation\n", _Profile.Str(), _Name.Str());

	return bReturn;
}

bool CGameContext::BeginReadSaveFile(CStr _Profile, CStr _Name, bool _Validate)
{
	m_spAsyncSaveContext = NULL;

	if(_Validate)
	{
		if(!ValidateSaveFile(_Profile, _Name))
			return false;
	}

	FixProfile(_Profile);

	M_TRY
	{
		CStr Path; // holds the savegame path

#if defined (PLATFORM_XBOX1)

#if defined(M_DEMO_XBOX)
		Path = "Z:\\Save\\" + _Profile+"\\";
		CDiskUtil::CreatePath(Path);
		//	const char *pSaveDrv = "Z:\\Saves\\";
#else
		CFStr TempPath;
		const char *pSaveDrv = "U:\\";
		if(XCreateSaveGame(pSaveDrv, _Profile.Unicode().StrW(), OPEN_EXISTING, 0, TempPath, TempPath.GetMax()) != ERROR_SUCCESS)
			return false;
		Path = TempPath;
#endif

#else
		MACRO_GetSystem;
		Path = pSys->TranslateSavePath(m_spWData->ResolvePath("Save\\") + _Profile + "\\");
		CDiskUtil::CreatePath(Path);
#endif

		// Write save game to disk
		if(!CDiskUtil::FileExists(Path + _Name))
			return false;

		m_CurrentSaveFile.OpenExt(Path + _Name, CFILE_READ | CFILE_BINARY | CFILE_NODEFERCLOSE);
	}
	M_CATCH(
		catch(CCException _Ex)
	{
		// Corrupt Save
		//  We need to inform the player abt it!!
		ConOutL("§cf80WARNING: (CWorld_DeltaGameState::World_Save) Exception during load.");
		throw;
		//Error("CWorld_DeltaGameState::World_Save", _Ex.GetExceptionInfo().GetString());
	}
	)

		return true;
}

bool CGameContext::EndReadSaveFile()
{
	m_CurrentSaveFile.Close();
	return true;
}

bool CGameContext::ReadWholeSaveFile(CStr _Profile, CStr _Name, TArray<uint8> &_rData)
{
	if(!BeginReadSaveFile(_Profile, _Name))
		return false;

	int32 Len = m_CurrentSaveFile.Length() - CHECKSUM_BYTELENGTH;
	_rData.SetLen(Len);
	m_CurrentSaveFile.Read(_rData.GetBasePtr(), Len);

	EndReadSaveFile();
	return true;
}

/*
bool CGameContext::ReadSaveFile(CStr _Profile, CStr _Name, uint8 *_pData, int32 _Size) //ReadSaveFile(spCWorldData _pWData, CStr _SaveName, CStr _FileName, TArray<uint8> &_lData, bool _bCryptate)
{
int32 nLoadDataLength;
CFStr Path;

FixProfile(_Profile);

#if defined(PLATFORM_XBOX)
// Create (or open an existing) save game with the specified name.
#if defined(M_DEMO_XBOX)
Path = "Z:\\Save\\" + _Profile + "\\";
#else
const char *pSaveDrv = "U:\\";
if(XCreateSaveGame(pSaveDrv, _Profile.Unicode().StrW(), OPEN_EXISTING, 0, Path, Path.GetMax()) != ERROR_SUCCESS)
return false;


//if(Path == "")
//	return false;

Path += "\\";
#endif
#else
Path = _pWData->ResolvePath("Save\\") + _Profile + "\\";
#endif

CStr FilePath = Path.Str() + _Name;
if(!CDiskUtil::FileExists(FilePath))
return false;

CCFile File;
File.Open(FilePath, CFILE_READ | CFILE_BINARY);
nLoadDataLength = File.Length();

if(nLoadDataLength < CHECKSUM_BYTELENGTH) // 4 byte for crc.. less than this and it's foobar
{
File.Close();
return false;
}

_lData.SetLen(nLoadDataLength);
File.ReadLE(_lData.GetBasePtr(), nLoadDataLength);
File.Close();

// Decryptate data and check checksum
//if(_bCryptate)
{

// Extract checksum
uint8 FileCrcCalc[CHECKSUM_BYTELENGTH];
for(int j = 0; j < CHECKSUM_BYTELENGTH; j++)
FileCrcCalc[j] = _lData[j + _lData.Len() - CHECKSUM_BYTELENGTH];
_lData.SetLen(_lData.Len() - CHECKSUM_BYTELENGTH);

CCrcCheck Crc;
int32 Len = _lData.Len();
uint8 *pData = _lData.GetBasePtr();
for(int i = 0; i < Len; i++)
{
pData[i] ^= 42;
Crc.UpdateCrc(pData[i]);
}

// Compare checksum
uint8 CrcCalc[CHECKSUM_BYTELENGTH];
Crc.GetChecksumList(CrcCalc);
for(int k = 0; k < CHECKSUM_BYTELENGTH; k++)
{
if(CrcCalc[k] != FileCrcCalc[k])
return false;
}
}
return false;
return true;
}
*/

bool CGameContext::CreateProfile(CStr _Profile)
{
	CStr Path;
#if defined(PLATFORM_XBOX1)
	// Create (or open an existing) save game with the specified name.
#if defined(M_DEMO_XBOX)
	Path = "Z:\\Save\\" + _Profile + "\\";
#else
	const char *pSaveDrv = "U:\\";
	if(XCreateSaveGame(pSaveDrv, _Profile.Unicode().StrW(), CREATE_NEW, 0, Path, Path.GetMax()) != ERROR_SUCCESS)
		return false;
	CopyFile("d:\\xbox\\profimage.xbx", Path+"\\saveimage.xbx", FALSE);
#endif
#else
	if(m_spWData == NULL)
		return false;

	MACRO_GetSystem;
	Path = pSys->TranslateSavePath(m_spWData->ResolvePath("Save\\") + _Profile + "\\");
	if(CDiskUtil::FileExists(Path+"_profile"))
		return false;

	if(!CDiskUtil::CreatePath(Path))
		return false;

#endif
	return true;
}

bool CGameContext::DeleteProfile(CStr _Profile)
{
	m_CurrentProfile.Clear();
	FixProfile(_Profile);
#ifdef PLATFORM_XBOX1
	if(XDeleteSaveGame("U:\\", _Profile.Unicode().StrW()) != ERROR_SUCCESS)
		return false;
#else
	CStr FullPath = GetProfilePath(_Profile);
	if(!CDiskUtil::DirectoryExists(FullPath))
		return false;
	if(FullPath.Find("Content\\Save\\") != -1)
		CDiskUtil::DelTree(FullPath);
#endif

	return true;
}

bool CGameContext::DeleteSaveFile(CStr _Profile, CStr _Name)
{
	FixProfile(_Profile);
	M_TRY
	{
		CStr FullPath = GetProfilePath(_Profile) + _Name;
		if(!CDiskUtil::FileExists(FullPath))
			return false;
		CDiskUtil::DelFile(FullPath);
		for(int32 i = 0; i < m_lSaveFiles.Len(); i++)
			if(m_lSaveFiles[i] == _Name)
			{
				m_lSaveFiles.Del(i);
				i--;
			}


	}
	M_CATCH(
		catch(...)
	{
		return false;
	}
	)

		return true;
}




bool CGameContext::ReadSaveFileToReg(CStr _Profile, CStr _Name, CRegistry *_pReg)
{
	FixProfile(_Profile);

	TArray<uint8> lData;
	if(!ReadWholeSaveFile(_Profile, _Name, lData))
		return false;

	spCCFile spFileMem = CDiskUtil::CreateCCFile(lData, CFILE_READ | CFILE_BINARY);
	_pReg->SetNumChildren(0);
#ifdef PLATFORM_CONSOLE
	_pReg->Read(spFileMem, REGISTRY_BINARY_VERSION);
#else
	_pReg->XRG_Read(spFileMem, "", TArray<CStr>::TArray());
#endif
	/*
	// read
	TArray<uint8> lData;
	if(!ReadSaveFile(_Profile, _Name, lData))
	return false;

	// write to reg
	spCCFile spFileMem = CDiskUtil::CreateCCFile(lData, CFILE_READ | CFILE_BINARY);
	_pReg->Read(spFileMem);
	*/

	m_bValidProfileLoaded = true;
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	pSys->GetEnvironment()->SetValue("GAME_LASTVALIDPROFILE", _Profile);
	pSys->WriteEnvironment();


	return true;
}

bool CGameContext::LoadLastValidProfile()
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	CStr ProfileName = pSys->GetEnvironment()->GetValue("GAME_LASTVALIDPROFILE");
	if(ProfileName.Len() == 0)
		return false;

	if(!ValidateProfile(ProfileName))
		return false;

	if(!ReadSaveFileToReg(ProfileName, "_profile", pSys->GetOptions(1)))
		return false;
	ConExecute("option_update()");
	pSys->GetOptions()->SetValue("GAME_PROFILE", ProfileName);

	return true;
}

// -------------------------------------------------------------------
//  Script commands
// -------------------------------------------------------------------
void CGameContext::Con_BSPModelEnable(int _Mask)
{
	MAUTOSTRIP(CGameContext_Con_BSPModelEnable, MAUTOSTRIP_VOID);
	CWorld_Client* pWC = GetCurrentClient();
	if (!pWC || !pWC->m_spMapData) return;

	/*
	CXR_Model* pModel = pWC->GetMapData()->GetResource_Model(pWC->GetMapData()->GetResourceIndex_BSPModel("$WORLD:0"));
	if (!pModel)
	pModel = pWC->GetMapData()->GetResource_Model(pWC->GetMapData()->GetResourceIndex_BSPModel2("$WORLD:0"));
	if (!pModel)
	pModel = pWC->GetMapData()->GetResource_Model(pWC->GetMapData()->GetResourceIndex_BSPModel3("$WORLD:0"));
	*/
	CXR_Model* pModel = pWC->GetMapData()->GetResource_Model(pWC->GetMapData()->GetResourceIndex_BSPModel("$WORLD:0"));
	if (pModel) pModel->SetParam(MODEL_BSP_PARAM_GLOBALFLAGS, _Mask);
}

void CGameContext::Con_BSPModelSwitch(int _Mask)
{
	MAUTOSTRIP(CGameContext_Con_BSPModelSwitch, MAUTOSTRIP_VOID);
	CWorld_Client* pWC = GetCurrentClient();
	if (!pWC || !pWC->m_spMapData) return;

	CXR_Model* pModel = pWC->GetMapData()->GetResource_Model(pWC->GetMapData()->GetResourceIndex_BSPModel("$WORLD:0"));
	/*
	if (!pModel)
	pModel = pWC->GetMapData()->GetResource_Model(pWC->GetMapData()->GetResourceIndex_BSPModel2("$WORLD:0"));
	if (!pModel)
	pModel = pWC->GetMapData()->GetResource_Model(pWC->GetMapData()->GetResourceIndex_BSPModel3("$WORLD:0"));
	*/
	if (pModel)
	{
		pModel->SetParam(MODEL_BSP_PARAM_GLOBALFLAGS, pModel->GetParam(MODEL_BSP_PARAM_GLOBALFLAGS) ^ _Mask);
		ConOut(CStrF("BSP-Model flags 0x%.8x", pModel->GetParam(MODEL_BSP_PARAM_GLOBALFLAGS) ));
	}
}

void CGameContext::Con_SplitScreen(int _nSplitScreens)
{
	MAUTOSTRIP(CGameContext_Con_SplitScreen, MAUTOSTRIP_VOID);
	if (_nSplitScreens == m_nSplitScreens) return;

	if (_nSplitScreens < 1 || _nSplitScreens > 4)
	{
		ConOut("Invalid number of split screen players.");
		return;
	}

	m_nSplitScreens = _nSplitScreens;
}

void CGameContext::Con_SplitAxis(int _Axis)
{
	MAUTOSTRIP(CGameContext_Con_SplitAxis, MAUTOSTRIP_VOID);
	m_bSplitAxis = _Axis != 0;
}

void CGameContext::Con_AddLocalClient(int _bSplit)
{
	MAUTOSTRIP(CGameContext_Con_AddLocalClient, MAUTOSTRIP_VOID);
	spCReferenceCount spObj = (CReferenceCount*) MRTC_GOM()->CreateObject(m_ClientClass);
	spCWorld_Client spLocalClient = safe_cast<CWorld_Client> ((CReferenceCount*)spObj);
	if (spLocalClient == NULL) Error("CreateLocalClient", "Unable to create client-object.");

	int Mode = WCLIENT_MODE_PRIMARY | WCLIENT_MODE_LOCAL;
	if (_bSplit) Mode |= WCLIENT_MODE_SPLITSCREEN;

	//	int Mode = WCLIENT_MODE_PRIMARY | WCLIENT_MODE_SPLITSCREEN;

	CWC_CreateInfo CreateInfo;
	CreateInfo.m_Mode = Mode;
	CreateInfo.m_spGameReg = m_spGameReg;
	CreateInfo.m_spWData = m_spWData;
	CreateInfo.m_PlayerNr = m_lspWClients.Len();
	CreateInfo.m_spEngine = m_spEngine;

	if(m_lspWClients.Len() > 0)
	{
		CreateInfo.m_Mode |= WCLIENT_MODE_REFERENCE;
		CreateInfo.m_pClientRef = m_lspWClients[0];
		spLocalClient->Create(CreateInfo);
	}
	else
		spLocalClient->Create(CreateInfo);

	m_spWServer->Client_ConnectLocal(spLocalClient);
	int iC = m_lspWClients.Add(spLocalClient);

	if (iC == 0)
		spLocalClient->Sound_SetSoundContext(m_spSoundContext);
}

void CGameContext::Con_Pause(int _Enable)
{
	MAUTOSTRIP(CGameContext_Con_Pause, MAUTOSTRIP_VOID);
	if (_Enable)
		Simulate_Pause();
	else
		Simulate_Resume();
}

void CGameContext::Con_SingleStep()
{
	// This will make the server perform one Simulate() run..
	if (m_PauseCount)
		m_bSingleStep = true;
}

void CGameContext::Con_Dump(int _DumpFlags)
{
	MAUTOSTRIP(CGameContext_Con_Dump, MAUTOSTRIP_VOID);
	if(m_spWServer != NULL)
		m_spWServer->Dump(_DumpFlags);

	if (_DumpFlags & WDUMP_CLIENTS)
	{
		for(int i = 0; i < m_lspWClients.Len(); i++)
		{
			LogFile("-------------------------------------------------------------------");
			LogFile("                          Client-Dump");
			LogFile("-------------------------------------------------------------------");
			LogFile("");
			m_lspWClients[i]->Dump(_DumpFlags);
		}
	}
}

void CGameContext::Con_Map(CStr _Name)
{
	MAUTOSTRIP(CGameContext_Con_Map, MAUTOSTRIP_VOID);
	if(m_bValidProfileLoaded)
		AddPendingCommand(CGC_Command(CGC_COMMAND_MAP, _Name));
	else
		ConOut("Have to select a valid profile before loading a map");
}

void CGameContext::Con_SMap(CStr _Name)
{
	MAUTOSTRIP(CGameContext_Con_Map, MAUTOSTRIP_VOID);
	if(m_bValidProfileLoaded)
		AddPendingCommand(CGC_Command(CGC_COMMAND_MAP, _Name, CStrF("%i", SERVER_CHANGEWORLD_INITNETWORK)));
	else
		ConOut("Have to select a valid profile before loading a map");
}

void CGameContext::Con_ChangeMap(CStr _Name, int _Flags)
{
	MAUTOSTRIP(CGameContext_Con_ChangeMap, MAUTOSTRIP_VOID);
	MSCOPE(CGameContext::Con_ChangeMap, GAMECONTEXT);
	if(m_bValidProfileLoaded)
	{
		CGC_Command Cmd(CGC_COMMAND_CHANGEMAP);
		Cmd.m_lFParam[0] = _Name;
		Cmd.m_lFParam[1] = CFStrF("%i", _Flags);
		AddPendingCommand(Cmd);
	}
	else
		ConOut("Have to select a valid profile before loading a map");
}

void CGameContext::Con_Record(CStr _DemoName)
{
	MAUTOSTRIP(CGameContext_Con_Record, MAUTOSTRIP_VOID);
	StopSimulation();

	{
		// Sync on
		MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
		if (pCon) pCon->ExecuteString("cl_set(\"sync\",\"1\")");
	}

	if(m_spWServer)
		m_spWServer->m_PlayMode = SERVER_PLAYMODE_DEMO;

#ifdef PLATFORM_XBOX1
	_DemoName = "T:\\Demos\\" + _DemoName;
#else
	if (_DemoName.GetDevice() == "")
	{
		CStr Path = ResolvePath("Demos\\");
		_DemoName = Path + _DemoName;
	}
#endif
	if (_DemoName.GetFilenameExtenstion() == "")
		_DemoName = _DemoName + ".XDM";

	CWorld_Client* pC = GetCurrentClient();
	if (!pC) return;

	pC->Demo_Start(_DemoName);

	StartSimulation();
}

void CGameContext::Con_Stop()
{
	MAUTOSTRIP(CGameContext_Con_Stop, MAUTOSTRIP_VOID);
	CWorld_Client* pC = GetCurrentClient();
	if (!pC) return;

	{
		// Sync off
		MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
		if (pCon) pCon->ExecuteString("cl_set(\"sync\",\"0\")");
	}

	if(m_spWServer)
		m_spWServer->m_PlayMode = SERVER_PLAYMODE_NORMAL;

#if !defined( PLATFORM_PS2 ) && !defined( PLATFORM_DOLPHIN )
	ConExecute("videofileend()");
#endif
	StopSimulation();
	pC->Demo_Stop();
	StartSimulation();
}

void CGameContext::Con_Demo(CStr _Name)
{
	MAUTOSTRIP(CGameContext_Con_Demo, MAUTOSTRIP_VOID);
	INIT_PAD(1000);

	m_lDemos.Clear();
	CStr Demos = _Name;
	while(Demos != "")
	{
		CStr s = Demos.GetStrSep(";");
		if (s.GetFilenameExtenstion() == "")
			s = s + ".XDM";		

#ifdef PLATFORM_XBOX1
		CStr File = "T:\\Demos\\" + s;
		if(!CDiskUtil::FileExists(File))
			File = "D:\\Content\\Demos\\" + s;
		m_lDemos.Add(File);
#else
		if (s.GetDevice() == "")
			s = "Demos\\" + s;
		m_lDemos.Add(ResolveFileName(s));
#endif
	}
	m_iDemo = 0;
	if (!m_lDemos.Len()) return;

	PlayDemo(m_lDemos[m_iDemo++], false);

	CHECK_PAD(Con_Demo);
}

void CGameContext::Con_CaptureDemo(CStr _Name, CStr _Dir, int _Rate)
{
	MAUTOSTRIP(CGameContext_Con_CaptureDemo, MAUTOSTRIP_VOID);
	Con_TimeDemo(_Name);
	Capture(_Dir, _Rate);
}

void CGameContext::Con_TimeDemo(CStr _Name)
{
	MAUTOSTRIP(CGameContext_Con_TimeDemo, MAUTOSTRIP_VOID);
	m_lDemos.Clear();
	CStr Demos = _Name;
	while(Demos != "")
	{
		CStr s = Demos.GetStrSep(";");

#ifdef PLATFORM_XBOX1
		s = "T:\\Demos\\" + s;
#else
		if (s.GetDevice() == "")
			s = "Demos\\" + s;
#endif
		if (s.GetFilenameExtenstion() == "")
			s = s + ".XDM";

		m_lDemos.Add(ResolveFileName(s));
	}
	m_iDemo = 0;
	if (!m_lDemos.Len()) return;

	PlayDemo(m_lDemos[m_iDemo++], true);
}

void CGameContext::Con_Benchmark(CStr _Name)
{
	m_bBenchmark = true;

	Con_TimeDemo(_Name);
}

void CGameContext::Con_TimeDemoGraph(CStr _Name)
{
	Con_TimeDemo(_Name);
	m_bTimeDemoGraph = true;
}

void CGameContext::Con_Connect(CStr _Addr)
{
	MAUTOSTRIP(CGameContext_Con_Connect, MAUTOSTRIP_VOID);
	AddPendingCommand(CGC_Command(CGC_COMMAND_CONNECT, _Addr));
}

void CGameContext::Con_Disconnect()
{
	MAUTOSTRIP(CGameContext_Con_Disconnect, MAUTOSTRIP_VOID);
	AddPendingCommand(CGC_Command(CGC_COMMAND_DISCONNECT));
}

void CGameContext::Con_Listen()
{
	MAUTOSTRIP(CGameContext_Con_Listen, MAUTOSTRIP_VOID);
	if (m_hListenConnection < 0)
	{
		if (!m_spNetwork && m_spNetDevice) 
		{
			m_spNetwork = (CNetwork *)MRTC_GetObjectManager()->GetClassRegistry()->CreateObject(m_pSystem->GetEnvironment()->GetValue("NET_Context", "CNetworkCore"));

			if (!m_spNetwork) 
			{
				ConOutL("(CGameContext::Con_Listen) No network");
				return;
			}

			m_spNetwork->Create(m_spNetDevice);
		}
		else if (!m_spNetwork)
		{
			ConOutL("(CGameContext::Con_Listen) No network device");
			return;
		}

		CNetwork_Address *pAddrListen = m_spNetwork->Global_ResolveAddress(m_pSystem->GetEnvironment()->GetValue("NET_ClientListenBroadcast", "*:0;Broadcast"));
		if (!pAddrListen)
		{
			ConOutL("(CGameContext::Con_Listen) Could not resolve client listen address");
			return;
		}

		m_hListenConnection = m_spNetwork->Client_Create(pAddrListen, NULL);
		pAddrListen->Delete();
		if (m_hListenConnection < 0)
		{
			ConOut("Could not open connection.");
			return;
		}
	}

	CNetwork_Address *pAddrBroadcast = m_spNetwork->Global_ResolveAddress(m_pSystem->GetEnvironment()->GetValue("NET_QUERYCONNECTION", "@:28000"));
	//	CNetwork_Address *pAddrBroadcast = m_spNetwork->Global_ResolveAddress("127.0.0.1:28000");
	if (!pAddrBroadcast)
	{
		ConOutL("(CGameContext::Con_Listen) Could not resolve query connection address");
		return;
	}	

	if (!m_spNetwork->Client_QueryConnection(m_hListenConnection, pAddrBroadcast))
		ConOut("Query connection failed.");
	pAddrBroadcast->Delete();
}

void CGameContext::Con_Dir(CStr _Path)
{
	MAUTOSTRIP(CGameContext_Con_Dir, MAUTOSTRIP_VOID);
	if (!m_spWData) return;
	M_TRY
	{
		/*		if (_Path.GetDevice() == "") _Path = m_WorldPath + _Path;
		if (_Path[_Path.Len()-1] != '\\')
		_Path += "\\";
		_Path += "*.*";

		CDirectoryNode Dir;
		Dir.ReadDirectory(_Path);
		*/
		TArray_SimpleSortable<CStr> lFiles = m_spWData->ResolveDirectory(_Path + "\\*", true);

		ConOut("Directory of " + _Path);
		int nf = lFiles.Len();
		for(int i = 0; i < nf; i++)
			ConOut(lFiles[i]);
	}
	M_CATCH(
		catch(CCException)
	{
	}
	)
}

void CGameContext::Con_SaveGame(CStr _GameName)
{
	MAUTOSTRIP(CGameContext_Con_SaveGame, MAUTOSTRIP_VOID);
	if (!m_spWServer)
	{
		ConOut("Not running server.");
		return;
	}

	m_spWServer->World_Save(_GameName,true);
}

void CGameContext::Con_LoadGame(CStr _GameName)
{
	MAUTOSTRIP(CGameContext_Con_LoadGame, MAUTOSTRIP_VOID);
	AddPendingCommand(CGC_Command(CGC_COMMAND_LOADGAME, _GameName));
}

void CGameContext::Con_SetGameKey(CStr _Key, CStr _Value)
{
	MAUTOSTRIP(CGameContext_Con_SetGameKey, MAUTOSTRIP_VOID);
	m_spGameReg->SetValue(_Key, _Value);
}

void CGameContext::Con_FreeResources()
{
	MAUTOSTRIP(CGameContext_Con_FreeResources, MAUTOSTRIP_VOID);
	AddPendingCommand(CGC_Command(CGC_COMMAND_FREERESOURCES));
}

void CGameContext::Con_SetGame(CStr _WorldPathes, CStr _GameName)
{
	MAUTOSTRIP(CGameContext_Con_SetGame, MAUTOSTRIP_VOID);
	AddPendingCommand(CGC_Command(CGC_COMMAND_SETGAME, _WorldPathes, _GameName));
}

void CGameContext::Con_DeferredScript(CStr _Script)
{
	MAUTOSTRIP(CGameContext_Con_DeferredScript, MAUTOSTRIP_VOID);
	AddPendingCommand(CGC_Command(CGC_COMMAND_SCRIPT, _Script));
}

void CGameContext::Con_DeferredScriptGrabScreen(CStr _Script)
{
	MAUTOSTRIP(CGameContext_Con_DeferredScript, MAUTOSTRIP_VOID);
	AddPendingCommand(CGC_Command(CGC_COMMAND_SCRIPTGRABSCREEN, _Script));
}


/*
void CGameContext::CopyDirectory_r(CStr _Path, CStr _Dest)
{
MAUTOSTRIP(CGameContext_CopyDirectory_r, MAUTOSTRIP_VOID);
CDirectoryNode Dir;
ConOutL("Scanning " + _Path + "*");
Dir.ReadDirectory(_Path + "*");
{
CStr Path = _Dest.GetPath();
if(Path != "" && Path[Path.Len() - 1] == '\\')
{
CStr Path2 = Path.Copy(0, Path.Len() - 1);
Path = Path2;
}
CStr Name = _Dest.GetFilename();
CDiskUtil::MakeDir(Path, Name);
}
int nFiles = Dir.GetFileCount();
for(int i = 0; i < nFiles; i++)
{
CDir_FileRec* pRec  = Dir.GetFileRec(i);
if (pRec->IsDirectory())
{
if (pRec->m_Name.Copy(0,1) != ".")
CopyDirectory_r(_Path + pRec->m_Name + "\\", _Dest + "\\" + pRec->m_Name);
}
else
{
CDiskUtil::CpyFile(_Path + pRec->m_Name, _Dest + "\\" + pRec->m_Name, 65536 * 16);
}
}
}
*/

/*
void CGameContext::Con_DeleteCache()
{
MAUTOSTRIP(CGameContext_Con_DeleteCache, MAUTOSTRIP_VOID);
CStr CachePath = m_spGameReg->GetValue("CACHEPATH");
if(CachePath != "")
{
CDirectoryNode Dir;
Dir.ReadDirectory(CachePath + "*");
int nFiles = Dir.GetFileCount();
for(int i = 0; i < nFiles; i++)
{
CDir_FileRec* pRec  = Dir.GetFileRec(i);
if (pRec->IsDirectory())
{
if (pRec->m_Name.Copy(0,1) != ".")
CDiskUtil::DelTree(CachePath + pRec->m_Name);
}
else
CDiskUtil::DelFile(CachePath + pRec->m_Name);
}

}
}
*/

void CGameContext::Con_SetWidescreen(int _ws)
{
	MAUTOSTRIP(CGameContext_Con_SetWidescreen, MAUTOSTRIP_VOID);
	/*	if(!m_pSystem || !m_pSystem->m_spDisplay)
	return;

	switch(_ws)
	{
	case 0:
	m_pSystem->m_spDisplay->SetWidescreen(SCREENASPECT_NORMAL);
	break;
	case 1:
	m_pSystem->m_spDisplay->SetWidescreen(SCREENASPECT_WIDE);
	break;
	}
	*/
}

void CGameContext::Con_SetGameClass(CStr _s)
{
	m_spGameReg->SetValue("DEFAULTGAME", _s);

#if defined(PLATFORM_CONSOLE)
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

	if(!_s.Compare("DM"))
	{
		pGameMod->m_pMPHandler->m_GameMode = CONTEXT_GAME_MODE_FREE_FOR_ALL;
		pGameMod->m_pMPHandler->m_GameSubMode = CONTEXT_GAME_SUB_MODE_TEAM_DEATHMATCH;	//This is the default value
	}
	else if(!_s.Compare("TDM"))
	{
		pGameMod->m_pMPHandler->m_GameMode = CONTEXT_GAME_MODE_TEAM_GAMES;
		pGameMod->m_pMPHandler->m_GameSubMode = CONTEXT_GAME_SUB_MODE_TEAM_DEATHMATCH;
	}
	else if(!_s.Compare("CTF"))
	{
		pGameMod->m_pMPHandler->m_GameMode = CONTEXT_GAME_MODE_TEAM_GAMES;
		pGameMod->m_pMPHandler->m_GameSubMode = CONTEXT_GAME_SUB_MODE_CAPTURE_THE_FLAG;
	}
	else if(!_s.Compare("Survivor"))
	{
		pGameMod->m_pMPHandler->m_GameMode = CONTEXT_GAME_MODE_FREE_FOR_ALL;
		pGameMod->m_pMPHandler->m_GameStyle = CONTEXT_GAME_STYLE_SURVIVOR;
		pGameMod->m_pMPHandler->m_GameSubMode = CONTEXT_GAME_SUB_MODE_TEAM_DEATHMATCH;	//This is the default value
	}
	else if(!_s.Compare("LastHuman"))
	{
		pGameMod->m_pMPHandler->m_GameMode = CONTEXT_GAME_MODE_FREE_FOR_ALL;
		pGameMod->m_pMPHandler->m_GameStyle = CONTEXT_GAME_STYLE_LASTHUMAN;
		pGameMod->m_pMPHandler->m_GameSubMode = CONTEXT_GAME_SUB_MODE_TEAM_DEATHMATCH;	//This is the default value
	}
#endif
}

void CGameContext::Con_ListOptions()
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) return;

	CRegistry* pOptionsRegistry = pSys->GetOptions();
	int nOptions = pOptionsRegistry->GetNumChildren();
	int iOption;
	for (iOption=0; iOption<nOptions; iOption++)
	{
		CRegistry* pKey = pOptionsRegistry->GetChild(iOption);
		ConOut(CStrF("%s=%s", pKey->GetThisName().GetStr(), pKey->GetThisValue().GetStr()));
	};
};

void CGameContext::Con_ForceOption(CStr _Option, CStr _NewSetting)
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) return;

	CStr Opt = _Option.UpperCase();
	CStr Set = _NewSetting.UpperCase();
	CRegistry* pOptionsRegistry = pSys->GetOptions();
	pOptionsRegistry->SetValue(Opt, Set);
};

void CGameContext::Con_SetOption(CStr _Option, CStr _NewSetting)
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) return;

	CStr Opt = _Option.UpperCase();
	CStr Set = _NewSetting.UpperCase();
	CRegistry* pOptionsRegistry = pSys->GetOptions();

	CRegistry* pKey = pOptionsRegistry->Find(Opt);
	if (!pKey)
	{
		ConOut(CStrF("ERROR: Option '%s' did not exist.", Opt.GetStr()));
		ConOut("Use 'addoption \"option\", \"setting\"' to add an option");
		ConOut("Use 'deloption \"option\"' to remove an option");
		ConOut("Use 'listoptions' to see complete list of options and settings");
		return;
	};

	pKey->SetThisKey(Opt, Set);
};

void CGameContext::Con_AddOption(CStr _Option, CStr _Setting)
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) return;

	CStr Opt = _Option.UpperCase();
	CStr Set = _Setting.UpperCase();
	CRegistry* pOptionsRegistry = pSys->GetOptions();

	CRegistry* pKey = pOptionsRegistry->Find(Opt);
	if (pKey)
	{
		ConOut(CStrF("ERROR: Option '%s' did already exist.", Opt.GetStr()));
		ConOut("Use 'setoption \"option\", \"new setting\"' to modify an option");
		ConOut("Use 'deloption \"option\"' to remove an option");
		ConOut("Use 'listoptions' to see complete list of options and settings");
		return;
	};

	pOptionsRegistry->AddKey(Opt, Set);
};

void CGameContext::Con_DelOption(CStr _Option)
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) return;

	CStr Opt = _Option.UpperCase();
	CRegistry* pOptionsRegistry = pSys->GetOptions();

	CRegistry* pKey = pOptionsRegistry->Find(Opt);
	if (!pKey)
	{
		ConOut(CStrF("ERROR: Option '%s' did not exist.", Opt.GetStr()));
		ConOut("Use 'addoption \"option\", \"setting\"' to add an option");
		ConOut("Use 'setoption \"option\", \"new setting\"' to modify an option");
		ConOut("Use 'listoptions' to see complete list of options and settings");
		return;
	};

	pOptionsRegistry->DeleteKey(Opt);
};

void CGameContext::Con_SimulateFractional(int _v)
{
	MAUTOSTRIP(CGameContext_Con_SimulateFractional, MAUTOSTRIP_VOID);
	m_bSimulateFractional = _v;	
}

void CGameContext::Con_ViewScale(fp32 _vX, fp32 _vY)
{
	m_Viewport3DScaleX = Min(1.0f, Max(0.01f, _vX));
	m_Viewport3DScaleY = Min(1.0f, Max(0.01f, _vY));
}

void CGameContext::Con_AddViewScale(fp32 _vX, fp32 _vY)
{
	Con_ViewScale(m_Viewport3DScaleX + _vX, m_Viewport3DScaleY + _vY);
}

void CGameContext::Con_TimeLeap(int _v)
{
	if (!m_spWServer)
		return;

	if (_v)
	{
		if (m_bTimeLeap)
			return;

		m_bTimeLeap = 1;
		AddPendingCommand(CGC_Command(CGC_COMMAND_TIMELEAP, CFStrF("%i", _v)));
	}
	else
		m_bTimeLeap = 0;
}


void CGameContext::Con_SoakMode()
{
	// Enable soak mode
	// 10 select random scriptlayer
	// 20 load
	// 30 wait until loading finished
	// 40 goto 10

	MRTC_GETRAND()->InitRand((uint32)CMTime::GetCPU().GetCycles());

	m_bSoakMode = true;
}

void CGameContext::Register(CScriptRegisterContext & _RegContext)
{
	MAUTOSTRIP(CGameContext_Register, MAUTOSTRIP_VOID);
	_RegContext.RegFunction("wenable", this, &CGameContext::Con_BSPModelEnable);
	_RegContext.RegFunction("wswitch", this, &CGameContext::Con_BSPModelSwitch);

	_RegContext.RegFunction("splitscreen", this, &CGameContext::Con_SplitScreen);
	_RegContext.RegFunction("splitaxis", this, &CGameContext::Con_SplitAxis);
	_RegContext.RegFunction("addlocalclient", this, &CGameContext::Con_AddLocalClient);
	_RegContext.RegFunction("pause", this, &CGameContext::Con_Pause);
	_RegContext.RegFunction("singlestep", this, &CGameContext::Con_SingleStep);
	_RegContext.RegFunction("wdump", this, &CGameContext::Con_Dump);

	_RegContext.RegFunction("map", this, &CGameContext::Con_Map);
	_RegContext.RegFunction("smap", this, &CGameContext::Con_SMap);
	_RegContext.RegFunction("changemap", this, &CGameContext::Con_ChangeMap);

	_RegContext.RegFunction("record", this, &CGameContext::Con_Record);
	_RegContext.RegFunction("stop", this, &CGameContext::Con_Stop);
	_RegContext.RegFunction("demo", this, &CGameContext::Con_Demo);
	_RegContext.RegFunction("play", this, &CGameContext::Con_Demo);
	_RegContext.RegFunction("capture", this, &CGameContext::Capture);
	_RegContext.RegFunction("capturedemo", this, &CGameContext::Con_CaptureDemo);
	_RegContext.RegFunction("timedemo", this, &CGameContext::Con_TimeDemo);
	_RegContext.RegFunction("benchmark", this, &CGameContext::Con_Benchmark);
	_RegContext.RegFunction("timedemograph", this, &CGameContext::Con_TimeDemoGraph);

	_RegContext.RegFunction("deloption", this, &CGameContext::Con_DelOption);
	_RegContext.RegFunction("listoptions", this, &CGameContext::Con_ListOptions);
	_RegContext.RegFunction("forceoption", this, &CGameContext::Con_ForceOption);
	_RegContext.RegFunction("setoption", this, &CGameContext::Con_SetOption);
	_RegContext.RegFunction("addoption", this, &CGameContext::Con_AddOption);

	_RegContext.RegFunction("connect", this, &CGameContext::Con_Connect);
	_RegContext.RegFunction("disconnect", this, &CGameContext::Con_Disconnect);
	_RegContext.RegFunction("list", this, &CGameContext::Con_Listen);
	_RegContext.RegFunction("dir", this, &CGameContext::Con_Dir);
	_RegContext.RegFunction("savegame", this, &CGameContext::Con_SaveGame);
	_RegContext.RegFunction("loadgame", this, &CGameContext::Con_LoadGame);
	_RegContext.RegFunction("setgamekey", this, &CGameContext::Con_SetGameKey);
	_RegContext.RegFunction("setgame", this, &CGameContext::Con_SetGame);
	_RegContext.RegFunction("deferredscript", this, &CGameContext::Con_DeferredScript);
	_RegContext.RegFunction("deferredscriptgrabscreen", this, &CGameContext::Con_DeferredScriptGrabScreen);
	_RegContext.RegFunction("freeresources", this, &CGameContext::Con_FreeResources);

	_RegContext.RegFunction("setgameclass", this, &CGameContext::Con_SetGameClass);
	_RegContext.RegFunction("setwidescreen", this, &CGameContext::Con_SetWidescreen);
	_RegContext.RegFunction("simulatefractional", this, &CGameContext::Con_SimulateFractional);
	_RegContext.RegFunction("viewscale", this, &CGameContext::Con_ViewScale);
	_RegContext.RegFunction("addviewscale", this, &CGameContext::Con_AddViewScale);
	_RegContext.RegFunction("timeleap", this, &CGameContext::Con_TimeLeap);

//	_RegContext.RegFunction("snd_mix_reload", this, &CGameContext::Con_Snd_Mix_Reload);
//	_RegContext.RegFunction("snd_mix_autoreload", this, &CGameContext::Con_Snd_Mix_Autoload);

	_RegContext.RegFunction("soakmode", this, &CGameContext::Con_SoakMode);

};

// Default Video class
#ifndef PLATFORM_WIN_PC
class CTGASequence : public CVideoFile
{
public:
	CTGASequence()
	{
	}

	virtual ~CTGASequence()
	{
		Close();
	}

	CStr m_FileName;
	int m_iFrame;

	bool Create(CStr _FileName, int _Width, int _Height, fp32 _FrameRate, int _KeyFrame = 1)
	{
		m_FileName = _FileName;
		m_iFrame = 0;
		return true;
	}

	bool AddFrame(CImage* _pImg)
	{
		bool bSuccess = false;

		while (!bSuccess)
		{
			M_TRY
			{
				int Thousand = m_iFrame / 1000;
				CStr Path = CStrF("%s\\%d", m_FileName.Str(), Thousand * 1000);
				CDiskUtil::CreatePath(Path);

				_pImg->Write(CStrF("%s\\%04i.png", Path.Str(), m_iFrame));
				bSuccess = true;
			}
			M_CATCH(
				catch (CCExceptionFile)
			{
			}
			)
		}

		++m_iFrame;

		return true;
	}

	void Close()
	{
	}
};

spCVideoFile MCreateVideoFile()
{
	TPtr<CTGASequence> spAVI = MNew(CTGASequence);
	return (CTGASequence*)spAVI;
}
#endif
