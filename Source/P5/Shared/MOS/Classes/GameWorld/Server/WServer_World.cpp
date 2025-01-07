#include "PCH.h"
#include "WServer_Core.h"
#include "../WBlockNavGrid.h"
#include "../WDynamics.h"
#include "../WPhysState_Hash.h"
#include "../Client/WClient_Core.h"
#include "../FrontEnd/WFrontEnd.h"
#include "../WNavGraph/WNavGraph_Pathfinder.h"
#include "../WObjects/WObj_Game.h"
#include "../../GameContext/WGameContext.h"
#include "../../../MSystem/Raster/MTextureContainerXTC2.h"
#include "../../../XR/XRBlockNav.h"
#include "../../../XR/XRBlockNavInst.h"
#include "../../../XR/XRNavGraphBuilder.h"
#include "../../../XR/XRVBContext.h"
#include "../../../XRModels/Model_BSP2/WBSP2Model.h"


#ifndef	PLATFORM_CONSOLE
#include "../../SDK/Include/cdapfn.h" // SafeDisc
#endif

#ifdef MRTC_MEMORYDEBUG
	#define CHECKMEMORY(s) { if (!_CrtCheckMemory()) Error(s, "Memory check failure."); }
#else
	#define CHECKMEMORY(s)
#endif

#define SERVER_VERBOSE 0
#define VERBOSE_OUT (!(SERVER_VERBOSE)) ? (void)0 : M_TRACE


void Thread_NukeSurfaceContextTempSurface(int _iObj, void* _pArg)
{
	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	CXW_SurfaceKeyFrame EmptyFrame;
	pSC->GetTempSurfaceKeyFrame() = EmptyFrame;
}

#ifdef PLATFORM_XENON
#include "xtl.h"
#include "tracerecording.h"
#include "XbDm.h"
extern BOOL D3D__NullHardware;
#endif

void CWorld_ServerCore::World_Change(const CFStr &WorldName, int _Flags)
{
	m_bWorld_PendingChange = false;
	ConExecute("cg_clearmenus();cg_dowindowswitch()");

	CMTime Timer;
	{
		TMeasure(Timer);


		++m_Loading;
		MAUTOSTRIP(CWorld_ServerCore_World_Change, MAUTOSTRIP_VOID);
	//	M_CALLGRAPHW;
		MSCOPE(CWorld_ServerCore::World_Change, WORLD_SERVER);


		MACRO_GetRegisterObject(CXR_VBMContainer, pVBMContainer, "SYSTEM.VBMCONTAINER");
		if(pVBMContainer)
		{
			pVBMContainer->BlockUntilAllVBMFree();
		}

	#ifdef PLATFORM_XENON
		static bint bTraceRecord = false;
		if (bTraceRecord)
		{
			D3D__NullHardware = true;
			XTraceStartRecording( "cache:\\World_Change.bin" );
		}
	#endif

		CStr _WorldName;
		{
			MSCOPESHORT(_WorldName);
			_WorldName = WorldName;
		}

		// Check if we're loading the "current" world, if not then remove the keepresources flag
	#ifdef PLATFORM_CONSOLE
		if( m_WorldName.CompareNoCase( _WorldName.GetFilenameNoExt() ) )
			_Flags &= ~SERVER_CHANGEWORLD_KEEPRESOURCES;
	#endif

		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if (!pSys) Error("World_Change", "No system.");

		// Reset load progress
		Load_ProgressReset();
		for(int i = 0; i < m_lspClients.Len(); i++)
			if (m_lspClients[i] != NULL)
				m_lspClients[i]->Load_ProgressReset();

		m_spWData->Resource_WorldOpenServer(_WorldName.GetFilenameNoExt());
		m_spWData->Resource_AsyncCacheDisable(2);

		M_TRY
		{
			MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
			if (pCon && !m_bPendingPrecacheDoUnpause)
			{
				MSCOPESHORT(Pause);
				pCon->ExecuteString("pause(1)");
				m_bPendingPrecacheDoUnpause = true;
			}

			ConOutL("(CWorld_ServerCore::World_Change) WorldName: " + _WorldName);
			m_ServerMode |= SERVER_MODE_NOCONNECT;
			m_spMapData->SetState(0);

			_WorldName = _WorldName.GetFilenameNoExt();

			World_SetGame(m_NextGameType);

	/*		if (_Flags & SERVER_CHANGEWORLD_MIGRATION)
			{
				if (m_WorldName == "")
					Error("World_Change", "No world is currenly running.");
				World_Save("Current");
				World_Load("Current", _WorldName);
			}
			else*/
			{
				TPtr<CWObject> spGameObj;
				CStr GameObjClass;

				{
					int iGame = Selection_GetSingleTarget(WSERVER_GAMEOBJNAME);
					if (iGame > 0)
					{
						// Tell the game object that the world is closing
						{
							MSCOPE(CloseGameObject, WORLD_SERVER);
							Message_SendToTarget(CWObject_Message(OBJSYSMSG_GAME_CLOSEWORLD), WSERVER_GAMEOBJNAME);
						}

						if(_Flags & SERVER_CHANGEWORLD_KEEPGAME)
						{
							spGameObj = m_lspObjects[iGame];
							Object_SetName(iGame, NULL);
							GameObjClass = m_spMapData->GetResourceName(spGameObj->m_iClass);
						}
					}
				}

				uint32 OldNextGUID = m_NextGUID;
				World_Close();

				// To keep GUID consistant while zoning, m_NextGUID must always be resetted.
				// Also, we must take in consideration that the gameobject could already be created.
				if(spGameObj != NULL)
					m_NextGUID = 2;
				else
					m_NextGUID = 1;

				World_CleanResources1(_Flags);

				if(m_spNetwork && m_hServerSocket >= 0)
					m_spNetwork->Server_ClearInfo(m_hServerSocket);

				CFStr WName = _WorldName;
				m_WorldName = "";
				_WorldName = "";
				
				if (spGameObj != NULL)
					spGameObj->OnMessage(CWObject_Message(OBJSYSMSG_GAME_POSTWORLDCLOSE));

				World_CleanResources2(_Flags);
				// Remove precache flag from all textures

				//////////////////////////////////////////////////

	#ifdef MRTC_ENABLE_REMOTEDEBUGGER
				MRTC_GetRD()->Flush();
	#endif

				//
				// This is the point were we are supposed to have the lowest memory consumption
	/*			{
					MRTC_GetMemoryManager()->MemTracking_Start();
					MRTC_GetMemoryManager()->MemTracking_Report(true);
					MRTC_GetMemoryManager()->MemTracking_Stop();
				}
	*/
				//

				// Read common resource list if we have one


				//
				World_Init(WName);
				m_ServerMode |= SERVER_MODE_SPAWNWORLD;		

				if (spGameObj != NULL)
				{
					int iGame = Object_Insert(spGameObj);
					CWObject* pGame = Object_Get(iGame);
					if (!pGame) Error("World_Change", "Unable to reinsert game object in new world.");

					pGame->m_iClass = m_spMapData->GetResourceIndex(GameObjClass);
					Object_SetName(iGame, WSERVER_GAMEOBJNAME);
				}
				else
					World_InitGameObject();


				{
					// Tell the game object that the world will be spawned
					MSCOPE(InitGameObject, WORLD_SERVER);
					Message_SendToTarget(CWObject_Message(OBJSYSMSG_GAME_INITWORLD), WSERVER_GAMEOBJNAME);
				}
				
				{
					CMTime Timer;
					{
						TMeasure(Timer);
						World_SpawnObjectsFromWorld(m_WorldName);

					}

					M_TRACEALWAYS("%s\n", TString("World_SpawnObjectsFromWorld", Timer).Str());
				}


				{
					CMTime Timer;
					{
						TMeasure(Timer);
					#ifdef PLATFORM_XENON
						static bint bTraceRecordWorld_DoOnSpawnWorld = false;
						if (bTraceRecordWorld_DoOnSpawnWorld)
						{
							D3D__NullHardware = true;
							XTraceStartRecording( "cache:\\World_DoOnSpawnWorld.bin" );
						}
					#endif
						World_DoOnSpawnWorld();
					#ifdef PLATFORM_XENON
						if (bTraceRecordWorld_DoOnSpawnWorld)
						{
							bTraceRecordWorld_DoOnSpawnWorld = false;
							XTraceStopRecording();
						}
					#endif
					}

					M_TRACEALWAYS("%s\n", TString("World_DoOnSpawnWorld", Timer).Str());
				}

				// Only initialize clients if a new game object was created.
				if (!spGameObj)
				{
					MSCOPE(AddClients, WORLD_SERVER);
					
					for(int i = 0; i < m_lspClients.Len(); i++)
						if (m_lspClients[i] != NULL)
						{
							Message_SendToTarget(CWObject_Message(OBJSYSMSG_GAME_ADDCLIENT, i), WSERVER_GAMEOBJNAME);
							Message_SendToTarget(CWObject_Message(OBJSYSMSG_GAME_CLIENTVARCHANGED, i), WSERVER_GAMEOBJNAME);
						}
				}

				if(_Flags & SERVER_CHANGEWORLD_KEEPRESOURCES)
					// Clean off any still unreferenced resources
					m_spWData->Resource_DeleteUnreferenced(NULL);

				// After spawning all objects from the level, set m_NextGUID to use dynamic GUID heap (this will be overwritten by save-game)
				m_NextGUID = Max(Max(uint32(0x80000001), OldNextGUID), uint32(m_NextGUID));
				VERBOSE_OUT("[ServerCore] NextGUID reset to 0x%08X\n", m_NextGUID);

	#ifndef WSERVER_DYNAMICSENGINE2
				// Init dynamics engine
				CWO_DynamicsCollider::GetInstance()->Init(m_spMapData);
	#endif
			}

			if (m_spNetwork != NULL && m_hServerSocket >= 0)
			{
				MSCOPE(SetConnectionInfo, WORLD_SERVER);

				CStr Info;
				Info += m_spGameReg->GetValue("GAMENAME", "Noname") + "|" + m_spGameReg->GetValue("WORLDPATHREL", "Nopath");
				Info += CStr("|Game:") + m_GameType.Ansi();
				Info += CStr("|Map:") + m_WorldName.Ansi();
				Info += CStr("|Cl:") + CStrF("%d", Client_GetCount());
				Info += CStr("|MaxCl:") + CStrF("%d", 16);
				Info += CStr("|FL:") + CStrF("%d", 0);
				Info += CStr("|TL:") + CStrF("%d", 0);

				ConOutL("(CWorld_ServerCore::World_Change) GameName: "+ m_spGameReg->GetValue("GAMENAME"));
				ConOutL("(CWorld_ServerCore::World_Change) WorldPathes: "+ m_spGameReg->GetValue("WORLDPATHREL"));
				
				CNetwork_ServerInfo ServerInfo(m_ServerName, Info.Str(), Info.Len());

				m_spNetwork->Server_SetInfo(m_hServerSocket, ServerInfo);
			}

		//	CStr ServInfo = m_ServerName + ", " + m_WorldName;
		//	if (m_spNetwork != NULL)
		//		m_spNetwork->Server_SetConnectionInfo(m_spGameReg->GetValue("GAMENAME", "Noname"), (char*) ServInfo, ServInfo.Len());
			m_spMapData->SetState(WMAPDATA_STATE_NOCREATE);

			SetUpdate_All(SERVER_CLIENTUPDATE_WORLD);
			m_ServerMode &= ~SERVER_MODE_SPAWNWORLD;	
			m_ServerMode &= ~SERVER_MODE_NOCONNECT;
		}
		M_CATCH(
		catch (CCException)
		{
			CDiskUtil::XDF_Stop();
			m_spWData->Resource_AsyncCacheEnable(2);

			throw;
		}
		)

		CDiskUtil::XDF_Stop();
		m_spWData->Resource_AsyncCacheEnable(2);

		for(int i = 0; i < m_lspObjects.Len(); i++)
			if(m_lspObjects[i])
				m_lspObjects[i]->m_bOriginallySpawned = true;


		// Dumping memusage to file
#ifdef M_Profile
 #if defined(PLATFORM_XENON)
		//CStr Path = "Cache:\\MemUsage\\";
		CStr Path = m_spMapData->ResolvePath("MemUsage\\");
 #elif defined(PLATFORM_PS3)
		CStr Path = "MemUsage/";
 #else
		CStr Path = m_spMapData->ResolvePath("MemUsage\\");
 #endif
		CDiskUtil::CreatePath(Path);
		DumpMemUsage( CStrF("%sMem_%s_%08x.txt", Path.Str(), m_WorldName.Str(), m_CurSpawnMask) );
#endif // M_Profile


#ifdef PLATFORM_XENON
		if (bTraceRecord)
		{
			XTraceStopRecording();
			bTraceRecord = false;
		}
#endif
		--m_Loading;
	}

	M_TRACEALWAYS("%s\n", TString("World Change", Timer).Str());
}


void CWorld_ServerCore::World_CleanResources1(int _Flags)
{
#ifdef PLATFORM_CONSOLE
	CXR_Model_BSP2::CleanStatic();
#endif

	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	// Always delete krick resources
	m_spWData->Resource_DeleteUnreferenced(NULL, (1 << WRESOURCE_CLASS_XAH));

	if (!(_Flags & SERVER_CHANGEWORLD_KEEPRESOURCES))
		m_spWData->Resource_DeleteUnreferenced(NULL);
	else if(_Flags & SERVER_CHANGEWORLD_DONTKEEPXWRES)
		m_spWData->Resource_DeleteUnreferenced(NULL, 1 << WRESOURCE_CLASS_MODEL_XW);

	Registry_ClearWorld();

	// Run CPU * 4 times to make sure if gets run on every possible core..
	MRTC_ThreadPoolManager::ProcessEachInstance(MRTC_SystemInfo::MRTC_GetSystemInfo().m_nCPU * 4, NULL, Thread_NukeSurfaceContextTempSurface);
}

void CWorld_ServerCore::World_CleanResources2(int _Flags)
{
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

	// Remove precache flag from all waves
	MACRO_GetRegisterObject(CWaveContext, pWC, "SYSTEM.WAVECONTEXT");
	if (pWC)
	{
		pWC->ClearWaveIDFlags(CWC_WAVEIDFLAGS_PRECACHE);
	}

	// Remove precache flag from all vb's
	MACRO_GetRegisterObject(CXR_VBContext, pVBC, "SYSTEM.VBCONTEXT");
	if(pVBC)
	{
		int nVB = pVBC->GetIDCapacity();
		for(int i = 0; i < nVB; i++)
		{
			pVBC->VB_SetFlags(i, pVBC->VB_GetFlags(i) & ~CXR_VBFLAGS_PRECACHE);
		}
	}

	m_spWData->Hash_Update();

#if defined(PLATFORM_CONSOLE)
	if(!(_Flags & SERVER_CHANGEWORLD_KEEPRESOURCES))
	{
		// On PC these functions are run in the Client Precache (when we know what to keep)
		if(pVBC)
			pVBC->Precache_Flush();
		if(pWC)
			pWC->Precache_Flush();
	}
#endif
}

void CWorld_ServerCore::DumpMemUsage(CStr _FileName)
{
	MAUTOSTRIP(CWorld_ServerCore_DumpMemUsage, MAUTOSTRIP_VOID);
	if(m_WorldName.CompareNoCase("campaign") == 0)
		return;
	
	CWorldData* pWD = m_spMapData->m_spWData;
	int nRc = pWD->Resource_GetMaxIndex();

	const int MaxClasses = 64;
	int lClassMem[MaxClasses];
	FillChar(&lClassMem, sizeof(lClassMem), 0);

	int AllMem = 0;
	{
		for(int i = 0; i < nRc; i++)
		{
			CWResource* pR = pWD->GetResource(i);
			if (pR)
			{
				lClassMem[pR->GetClass()] -= pR->m_MemUsed;
				AllMem -= pR->m_MemUsed;
			}
		}
	}

	M_TRY
	{
		const char* pSpawnMaskStr = World_GetCurrSpawnFlagsStr(SERVER_SPAWNFLAGS_DIFFICULTY_MASK | SERVER_SPAWNFLAGS_ONCE);

		CCFile File;
		File.Open(_FileName, CFILE_WRITE);
		File.Writeln("Memory Report:");
		File.Writeln("");
		File.Writeln(CStrF("  WorldName:       %s", m_WorldName.Str()));
		File.Writeln(CStrF("  GameType:        %s", m_GameType.Str()));
		File.Writeln(CStrF("  SpawnMask:       %08X (%s)", m_CurSpawnMask, pSpawnMaskStr));
		File.Writeln(CStrF("  Build date:      %s", __DATE__));
		File.Writeln(CStrF("  Build time:      %s", __TIME__));
#ifdef _DEBUG
		File.Writeln(CStr("  Configuration:   Debug"));
#elif defined M_RTM
		File.Writeln(CStr("  Configuration:   RTM"));
#else
		File.Writeln(CStr("  Configuration:   Release"));
#endif

		File.Writeln("");
		File.Writeln(CStrF("  Allocated:                      %10i KB", MRTC_GetMemoryManager()->m_AllocatedMem  / 1024));
		File.Writeln(CStrF("  Free:                           %10i KB", MRTC_GetMemoryManager()->m_FreeMem  / 1024));
		File.Writeln(CStrF("  Resources:                      %10i KB", AllMem  / 1024));
		File.Writeln("");
		File.Writeln("");
		File.Writeln("");
		File.Writeln("Resource Summary:");
		File.Writeln("");

		{
		for(int c = 0; c < MaxClasses; c++)
			{
				if (lClassMem[c])
					File.Writeln(CStrF("  %-35s %6i KB", m_spMapData->m_spWData->GetResourceClassName(c), lClassMem[c] / 1024));
			}
		}
		File.Writeln("  ---------------------------------------------");
		File.Writeln(CStrF("  All resources                       %6i KB", AllMem / 1024));

		File.Writeln("");
		File.Writeln("");
		File.Writeln("");
		File.Writeln("Resources:");
		File.Writeln("");
		{
			const int MAXCLASSES = 64;
			bool bClass[MAXCLASSES];
			MemSet(bClass, 0, sizeof(bClass));
			int nClasses = 0;
	//		int nRes = m_spMapData->GetNumResources();
			int nRes = m_spMapData->m_spWData->Resource_GetMaxIndex();
			int nRealRes = 0;
			for(int i = 0; i < nRes; i++)
			{						
				CWResource *pRes = m_spMapData->m_spWData->GetResource(i);
				if(pRes && pRes->m_iRcClass > 0 && pRes->m_iRcClass < MAXCLASSES && !bClass[pRes->m_iRcClass])
				{
					bClass[pRes->m_iRcClass] = true;
					nClasses++;
				}
				if(pRes)
					nRealRes++;
			}
			
			int AllMem = 0;
			int Index = 0;
			for(int c = 0; c < MAXCLASSES; c++)
			{
				if(bClass[c])
				{
					int TotMem = 0, TextWidth = 35;
					for(int i = 0; i < nRes; i++)
					{
						CWResource* pRes = m_spMapData->m_spWData->GetResource(i);
						if (pRes && pRes->m_iRcClass == c)
						{
							int Len = pRes->GetName().Len() - 4;
							TextWidth = Max(TextWidth, Min(Len+3, 70));
						}
					}
					for(int i = 0; i < nRes; i++)
					{
						CWResource *pRes = m_spMapData->m_spWData->GetResource(i);
						if((!pRes && c == 0) || (pRes && pRes->m_iRcClass == c))
						{
							const char *pName = pRes?&pRes->GetName().Str()[4]:"";
							File.Writeln(CStrF("  %-*s %6i KB", TextWidth, pName, -pRes->m_MemUsed / 1024));
							TotMem -= pRes ? pRes->m_MemUsed : 0;
							Index++;
						}
					}
					CFStr Sep("  ---------------------------------------------");
					for (uint i = 0; i < TextWidth - 35; i++)
						Sep += "-";
					File.Writeln(Sep.Str());
					File.Writeln(CStrF("  %-*s %6i KB", TextWidth, m_spMapData->m_spWData->GetResourceClassName(c), TotMem / 1024));
					File.Writeln("");
					File.Writeln("");
					AllMem += TotMem;
				}


			}
			File.Writeln("  ---------------------------------------------");
			File.Writeln(CStrF("  All resources                       %6i KB", AllMem / 1024));
		}

		File.Writeln("");
		File.Writeln("");
		File.Close();
	}
	M_CATCH(
	catch(CCException Exception)
	{
	}
	)

	M_TRY
	{
		CCFile File;
		File.Open(_FileName.GetPath() + "_Resources_Log.txt", CFILE_WRITE | CFILE_APPEND);
		CStr St = CStrF("%.2f MB", fp32(AllMem) / 1024 / 1024);
		File.Writeln(CStrF("%-30s %10s", m_WorldName.Str(), St.Str()));
		File.Close();
	}
	M_CATCH(
	catch(CCException Exception)
	{
	}
	)
}

bool CWorld_ServerCore::World_ChangeFromGame(CStr _World, int _Flags)
{
	MAUTOSTRIP(CWorld_ServerCore_World_ChangeFromGame, false);
	MSCOPE(CWorld_ServerCore::World_ChangeFromGame, WORLD_SERVER);
	
	CStr XWName = m_spWData->ResolveFileName("WORLDS\\" + _World.GetPath() + _World.GetFilenameNoExt() + ".xw", false);
	if (!CDiskUtil::FileExists(XWName))
	{
		CStr XDFName = m_spWData->ResolveFileName("XDF\\" + _World.GetPath() + _World.GetFilenameNoExt() + "_Server_Common.xdf", false);
		if (!CDiskUtil::FileExists(XDFName))
		{
			ConOut(_World + " doesn't exist.");
			return false;
		}
	}

	if (!m_bWorld_PendingChange)
	{
		m_bWorld_PendingChange = true;
		MSCOPESHORT(ChangeMap);
		MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
		if (!pCon) Error("Simulate", "No console.");
		pCon->ExecuteString(CStrF("deferredscriptgrabscreen(\"changemap(\\\"%s\\\", %d)\")", _World.Str(), _Flags));
	}

//	m_World_PendingName = _World;
//	m_World_PendingFlags = _Flags;
	return true;
}

void CWorld_ServerCore::World_SetGame(CStr _Game)
{
	MAUTOSTRIP(CWorld_ServerCore_World_SetGame, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ServerCore::World_SetGame, WORLD_SERVER);
	
	if (_Game == m_GameType) return;

	m_spServerReg = NULL;
	m_spGameReg->DeleteDir("SERVER");

	// Read base server registry
	CStr SvReg = m_spGameReg->GetValue("SERVERREG", "", 0);

	// Check for XCR
#ifdef M_ENABLE_REGISTRYCOMPILED
	CStr Name = m_spWData->ResolveFileName(SvReg + _Game + ".XCR", false);
	if (CDiskUtil::FileExists(Name))
	{
		MSCOPESHORT(ServerReg);
		// Compiled registry found, note that XCR:s are game-type specific,
		// so one XCR-version for every possible _Game must exist.


		{
//			D_NOXDF;
			m_ServerRegCompiled.Read_XCR(Name);
		}
		m_spServerReg = m_ServerRegCompiled.GetRoot();
//		m_spServerReg->XRG_Write("d:\\Server.xrg");
		m_spServerReg->SetThisName("SERVER");
		m_spGameReg->AddReg(m_spServerReg);
	}
	else
#endif
	{
		if (SvReg != "")
		{
			MSCOPESHORT(ServerReg);

			TArray<CStr> lDefines;
			MACRO_GetSystemRegistry(pReg);
			CRegistry* pRegDef = (pReg) ? pReg->GetDir("GAMECONTEXT\\REGISTRYDEFINES") : NULL;
			if (pRegDef)
			{
				for(int i = 0; i < pRegDef->GetNumChildren(); i++)
					lDefines.Add(pRegDef->GetName(i));
			}

			m_spServerReg = m_spGameReg->CreateDir("SERVER");
			CStr Reg = m_spWData->ResolveFileName(SvReg + ".XRG", false);
			{
//				D_NOXDF;
				m_spServerReg->XRG_Read(Reg, lDefines);
			}

			// Look for game-specific registry
			{
				CStr Name = m_spWData->ResolveFileName(SvReg + _Game + ".XRG", false);
				if (CDiskUtil::FileExists(Name))
				{
					MSCOPESHORT(GameReg);

					CRegistry_Dynamic Reg;
					Reg.SetThisKey(m_spServerReg->GetThisName(), m_spServerReg->GetThisValue());
					{
//						D_NOXDF;
						Reg.XRG_Read(Name, lDefines);
					}

					// Delete templates since they can't be overriden.
					for(int i = Reg.GetNumChildren()-1; i >= 0; i--)
						if (Reg.GetChild(i)->GetNumChildren()) Reg.DeleteKey(i);

					m_spServerReg->CopyDir(&Reg);
				}
				else
					ConOutL(CStrF("§cf80WARNING: (CWorld_ServerCore::World_SetGame) No server registry for game '%s'", (char*) _Game));
			}
			m_spServerReg->SimulateRegistryCompiled(true);
		}
	}

	m_spTemplateReg = m_spServerReg->Find("TEMPLATES");

	m_GameType = _Game;
	ConOutL( CStrF("(CWorld_ServerCore::World_SetGame) Gametype: '%s'", (char*) _Game) );
	OnRegistryChanged();
}

bool CWorld_ServerCore::World_Pause(bool _bPause)
{
	MAUTOSTRIP(CWorld_ServerCore_World_Pause, MAUTOSTRIP_VOID);

	bool bRes = false;
	if (!_bPause && (m_ServerMode & SERVER_MODE_PAUSE))
	{
		// Pause switched from on to off.

		for(int i = 0; i < m_lspClients.Len(); i++)
			if (m_lspClientInfo[i] != NULL)
				m_lspClientInfo[i]->m_bEatAllInput = 1;
		bRes = true;
	}
	else if(_bPause && !(m_ServerMode & SERVER_MODE_PAUSE))
		bRes = true;

	m_ServerMode &= ~SERVER_MODE_PAUSE;
	if (_bPause)
		m_ServerMode |= SERVER_MODE_PAUSE;

	return bRes;
}

void CWorld_ServerCore::World_InitPlayers()
{
	MAUTOSTRIP(CWorld_ServerCore_World_InitPlayers, MAUTOSTRIP_VOID);
/*	for(int i = 0; i < m_lspClients.Len(); i++)
	{
		if (m_lspClientInfo[i] != NULL)
			Player_Respawn(m_lspClientInfo[i]->m_iPlayer);
	}*/
}

/*void CWorld_ServerCore::World_ClearSave(CStr _Name)
{
	MAUTOSTRIP(CWorld_ServerCore_World_ClearSave, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ServerCore::World_ClearSave, WORLD_SERVER);
#ifdef PLATFORM_XBOX
	LogFile("(CWorld_ServerCore::World_ClearSave) (IGNORING) Clearing save-game " + _Name);
#else
	LogFile("(CWorld_ServerCore::World_ClearSave) Clearing save-game " + _Name);
	// 'Waste' all files in save-directory.

	CDiskUtil::CreatePath(m_SavePath + _Name);

	CDirectoryNode Dir;
	CStr Path = m_SavePath + _Name + "\\";
	Dir.ReadDirectory(Path + "*");
	for(int i = 0; i < Dir.GetFileCount(); i++)
	{
		CDir_FileRec* pF = Dir.GetFileRec(i);
		if (pF->IsArchive())
		{
			LogFile("    Deleting " + Path + pF->m_Name);
			CDiskUtil::DelFile(Path + pF->m_Name);
		}
	}
#endif
}*/

/*void CWorld_ServerCore::World_CopySave(CStr _Src, CStr _Dst)
{
	MAUTOSTRIP(CWorld_ServerCore_World_CopySave, MAUTOSTRIP_VOID);
#ifdef PLATFORM_XBOX
	LogFile("(CWorld_ServerCore::World_ClearSave) (IGNORING) Copying save-game " + _Src + " to " + _Dst);

#else
	if (_Src.CompareNoCase(_Dst) == 0) return;

	World_ClearSave(_Dst);
	LogFile("(CWorld_ServerCore::World_ClearSave) Copying save-game " + _Src + " to " + _Dst);

	CDiskUtil::CreatePath(m_SavePath + _Dst);

	CDirectoryNode Dir;
	CStr SrcPath = m_SavePath + _Src + "\\";
	CStr DstPath = m_SavePath + _Dst + "\\";
	Dir.ReadDirectory(SrcPath + "*");
	for(int i = 0; i < Dir.GetFileCount(); i++)
	{
		CDir_FileRec* pF = Dir.GetFileRec(i);
		if (pF->IsArchive())
			CopyFile(SrcPath + pF->m_Name, DstPath + pF->m_Name);
	}
#endif
}*/


bool CWorld_ServerCore::XDFStart(const char *_pName)
{
	MACRO_GetSystem;
	MSCOPESHORT(WSERVER_XDF_SUPPORT);

	CStr Name = _pName;
	CStr XDFPath = m_spMapData->ResolveFileName("XDF\\" + Name + ".XDF");
	
	if (D_MXDFCREATE == 3)
	{
		// we are creating XDFs, not using them
		CStr XDFBase = pSys->GetEnvironment()->GetValue("XDF_PATH");
		if (XDFBase.Len() > 0)
			XDFPath = XDFBase + "\\" + Name + ".XDL";
		else
			XDFPath = m_spMapData->ResolveFileName("XDF\\" + Name + ".XDL");

		if (!CDiskUtil::FileExists(XDFPath))
		{
			CDiskUtil::XDF_Record(XDFPath, m_spMapData->ResolvePath(""));
		}
		else
		{
			ConOutL(CStrF("Skippng creating of XDF file, because it already exists (%s)", XDFPath.Str()));
		}
	}
	else if (CDiskUtil::FileExists(XDFPath))
	{
		// XDF found, enable it
		CDiskUtil::XDF_Use(XDFPath, m_spMapData->ResolvePath(""));
		return true;
	}

	return false;
}

void CWorld_ServerCore::XDFStop()
{
	CDiskUtil::XDF_Stop();
}

void CWorld_ServerCore::World_Init(CStr _WorldName)
{
	MAUTOSTRIP(CWorld_ServerCore_World_Init, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ServerCore::World_Init, WORLD_SERVER);

	ClearStatistics();

	m_WorldName = _WorldName.GetFilenameNoExt();
ConOutL("(CWorld_ServerCore::World_Init) WorldName: " + m_WorldName);

	{
		MSCOPESHORT(WorldData);
		m_spMapData = MNew(CMapData);
		if(!m_spMapData)
			MemError("World_Init");
		m_spMapData->m_pWServer = this;
		m_spMapData->Create(m_spWData);
		m_spMapData->SetWorld(m_WorldName);
		m_spMapData->SetIndexMap(m_lResourceNameHashes);
		m_lResourceNameHashes.Clear();
		m_spMapData->GetResourceIndex_Font("MONOPRO");
	}

	// Reset time
/*	m_SimulationTick = 0;
	m_SimulationTime = 0;
	m_Simulate_iNextObject = 0;*/

	// Setup message queue.
	typedef TQueue<CMsgQueueElement> TQueue_CMsgQueueElement;
	m_spMsgQueue = MNew1(TQueue_CMsgQueueElement, SERVER_MESSAGEQUEUESIZE);
	if (m_spMsgQueue == NULL) MemError("World_Init");

	// Create object-heap
//	int nObj = (m_spMapData->m_lspObjectAttr.Len() + 512+32) & 0xffffffe0;
	const int nObj = 2560;
	M_STATIC_ASSERT(nObj <= CSelection::LARGE_BUFFER);
	m_lspObjects.SetLen(nObj);

#ifndef DEF_DISABLE_PERFGRAPH
	// Create object perfgraphs
/*	{
		MSCOPE(PerfGraph, IGNORE);
		m_PerfGraph_lObjects.Clear();
		m_PerfGraph_lObjects.SetLen(nObj);
		for(int i = 0; i < nObj; i++)
			m_PerfGraph_lObjects[i].Create();
		m_PerfGraph_Tick_OnRefresh = 0;
	}*/
#endif

	m_liObjectDeferredDestroy.Clear();
	m_liObjectDeferredDestroy.SetGrow(nObj);

	{
		for(int i = 0; i < m_lspClientInfo.Len(); i++)
			if (m_lspClientInfo[i] != NULL)
			{
				Client_SetNumRegDirectories(i, WCLIENT_REG_NUM);
				m_lspClientInfo[i]->m_lObjInfo.Clear();
				m_lspClientInfo[i]->m_lObjInfo.SetLen(nObj);
			}
	}

	// Setup object-allocation heap manager
	m_spObjectHeap = MNew1(CIDHeap, m_lspObjects.Len());
	if (m_spObjectHeap == NULL) MemError("World_Init");
	m_spObjectHeap->AllocID();		// Eat up index 0


	// Path
	m_iWorldModel = -1;
	m_spSceneGraphInstance = NULL;
	m_pSceneGraph = 0;
	m_pBlockNavGrid = NULL;
	m_pNavGraph = NULL;

	// Start using common XDF
	bool CommonXDFUsed = false;

	// When spawning the world with fullspawnmask it indecates that we just want to make
	// the clientside complete XDF files.
	CStr XDFBaseName = _WorldName;
	CStr XDFSpawnMask;
	XDFSpawnMask.CaptureFormated("%08x", m_CurSpawnMask&SERVER_SPAWNFLAGS_XDF_MASK);

	if(1)
	{
		XDFStart(XDFBaseName + "_" + XDFSpawnMask + "_Load");

		// do loading stuff
		MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
		if (pGame)
		{
			// Loadmusic/voices
			pGame->DoLoadingStream();
		}

		XDFStop();
	}

	// Read and load all common resources
	if(1)
	{
		CommonXDFUsed = XDFStart(XDFBaseName + "_Common");

		CStr Filename = m_spMapData->ResolveFileName("XDF\\" + _WorldName + ".rul");
		if(CDiskUtil::FileExists(Filename))
		{
			CCFile Input;
			Input.Open(Filename, CFILE_READ|CFILE_BINARY);
			TThinArray<CStr> lCommon;
			if(Input.IsOpen())
			{
				int32 nResources;
				Input.ReadLE(nResources);
				lCommon.SetLen(nResources);

				for(int i = 0; i < nResources; ++i)
					lCommon[i].Read(&Input);

				Input.Close();

				for(int i = 0; i < nResources; ++i)
				{
					CStr &RcName = lCommon[i];
					CStr tmp = RcName;
					CStr type = tmp.GetStrSep(":");
					int iRC = m_spMapData->GetResourceIndex(RcName);
					if(iRC >= 0)
						m_spMapData->GetResource(iRC);
				}
			}
		}

		XDFStop();
	}

	// Start spawnmask specific XDF
	XDFStart(XDFBaseName + "_" + XDFSpawnMask);

	// Setup enum-space.
	m_lEnumSpace.SetLen(m_lspObjects.Len());

	// Setup space-enumerator.
	{
		MSCOPESHORT(SpaceEnum);
		m_spSpaceEnum = MNew(CWO_SpaceEnum);
		if (m_spSpaceEnum == NULL) MemError("World_Init");
		
		m_spSpaceEnum->Create(SERVER_HASH1_SIZE, SERVER_HASH1_BUCKET_SHIFT_SIZE, SERVER_HASH2_SIZE, SERVER_HASH2_BUCKET_SHIFT_SIZE, m_lspObjects.Len(), true);
	}

	// Setup GUID hashing
	{
		MSCOPESHORT(GUIDHash);
		m_spGUIDHash = MNew(CWS_GUIDHash);
		if (m_spGUIDHash == NULL) MemError("World_Init");
		m_spGUIDHash->Create(m_lspObjects.Len());
	}

	m_ObjPoolDynamics.Create(m_lspObjects.Len());
	m_Listeners.Create(m_lspObjects.Len(), 512);

	MPUSH(Navigation);

	// Create block-navigator
	CXR_BlockNav_Grid_GameWorld* pGrid = m_spMapData->GetResource_XWNavGrid(m_spMapData->GetResourceIndex_XWNavGrid("$WORLD"));
	if (pGrid)
	{
		pGrid->Create(64, this);
		m_pBlockNavGrid = pGrid;

		MRTC_SAFECREATEOBJECT_NOEX(spWBN, "CXR_BlockNav", CXR_BlockNav);
		if (!spWBN) Error("World_Init", "Unable to create CXR_BlockNav object.");
		spWBN->Create(pGrid);
		m_spBlockNav = (CXR_BlockNav*)spWBN;

		// create searcher
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
#ifdef PLATFORM_CONSOLE
		const int DefaultPathfinder = 1;
#else
		const int DefaultPathfinder = 0;
#endif
		if (pSys->GetEnvironment()->GetValuei("GAME_VPUPATHFINDER", DefaultPathfinder))
		{
			ConOutL("(CWorld_ServerCore::World_Init) Using VPU-based pathfinder");
			MRTC_SAFECREATEOBJECT_NOEX(spWBNSearcher, "CXR_BlockNavSearcher_VPUDispatcher", CXR_BlockNavSearcher_VPUDispatcher);
			if (!spWBNSearcher) Error("World_Init", "Unable to create CXR_BlockNavSearcher object.");
			spWBNSearcher->m_pPhysState = this;
			m_spBlockNavSearcher = (CXR_BlockNavSearcher*)spWBNSearcher;
			m_spBlockNavSearcher->Create(m_spBlockNav, 1);
		}
		else
		{
			ConOutL("(CWorld_ServerCore::World_Init) Using plain pathfinder");

			MRTC_SAFECREATEOBJECT_NOEX(spWBNSearcher, "CXR_BlockNavSearcher_Plain", CXR_BlockNavSearcher_Plain);
			if (!spWBNSearcher) Error("World_Init", "Unable to create CXR_BlockNavSearcher object.");
			m_spBlockNavSearcher = (CXR_BlockNavSearcher*)spWBNSearcher;
			m_spBlockNavSearcher->Create(m_spBlockNav, 2);
		}
		
	}
	else
		m_spBlockNav = NULL;

	//Create navgraph
	CXR_NavGraph* pNavGraph = m_spMapData->GetResource_XWNavGraph(m_spMapData->GetResourceIndex_XWNavGraph("$WORLD"));
	if (pNavGraph)
	{
		pNavGraph->BuildHash(CVec3Dint16(pNavGraph->GetMinDistance(), pNavGraph->GetMinDistance(), pNavGraph->GetMinDistance() * 5), 
							 CVec3Dint16(m_pBlockNavGrid->m_CellGridDim[0], m_pBlockNavGrid->m_CellGridDim[1], m_pBlockNavGrid->m_CellGridDim[2]));
		m_pNavGraph = pNavGraph;

		MRTC_SAFECREATEOBJECT_NOEX(spNPF, "CWorld_Navgraph_Pathfinder_AStar", CWorld_Navgraph_Pathfinder_AStar);
		if (!spNPF) Error("World_Init", "Unable to create CWorld_Navgraph_Pathfinder_AStar object.");
		spNPF->Create(m_pNavGraph, 2, 1024);
		m_spGraphPathfinder = (CWorld_Navgraph_Pathfinder *)spNPF;
	}
	else
		m_spGraphPathfinder = NULL;

	MPOP;

	// Assign the server's resource-manager to all client-mirrors.
	{
		for(int i = 0; i < m_lspClients.Len(); i++)
			if (m_lspClients[i] != NULL) m_lspClients[i]->m_spMapData = m_spMapData;
	}


}

void CWorld_ServerCore::World_InitGameObject()
{
	MAUTOSTRIP(CWorld_ServerCore_World_InitGameObject, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ServerCore::World_InitGameObject, WORLD_SERVER);
	
	CStr GameClass = Registry_GetServer()->GetValue("GameClass", "GameCore", 0);
	{ MRTC_Context Context(GameClass);
		int iObj = Object_Create(GameClass);
		if (iObj < 0) Error("World_Init", "Unable to create game object: " + GameClass);

		Object_SetName(iObj, WSERVER_GAMEOBJNAME);
	}
}


int32 CheckNav(void *_pSys)
{
	return 0;
}

void CWorld_ServerCore::World_Close()
{
	MAUTOSTRIP(CWorld_ServerCore_World_Close, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ServerCore::World_Close, WORLD_SERVER);
	
	m_WorldModelCollider.OnWorldClose();

	// Bogus Function
	CheckNav(m_pNavGraph);


	DumpStatistics();

	m_spSceneGraphInstance = NULL;
	m_pSceneGraph = NULL;

	// Set player-objects to void
	CWObject_Game *pGame = Game_GetObject();
	if(pGame)
		pGame->Player_DestroyAllObjects();

//	if (m_ServerMode & SERVER_MODE_RECORD) StoreRecording();
	for(int i = 0; i < m_lspClientInfo.Len(); i++)
		if (m_lspClientInfo[i] != NULL)
		{
			m_lspClientInfo[i]->m_lObjInfo.Clear();

			m_lspClientInfo[i]->m_spNetReg = NULL;
			m_lspClientInfo[i]->m_spClientReg = NULL;
			m_lspClientInfo[i]->m_FrameOut.Clear();
			m_lspClientInfo[i]->m_RegFrameOut.Clear();

			CWorld_ClientCore* pC = safe_cast<CWorld_ClientCore>((CWorld_Client*)m_lspClients[i]);
			if (pC)
				pC->World_Close();

/*			if (m_lspClientInfo[i]->m_bFrameOut) 
				LogFile(CStrF("Destroying update %d", m_lspClientInfo[i]->m_FrameOut.m_Size));

			m_lspClientInfo[i]->m_bFrameOut = false;
			m_lspClientInfo[i]->m_FrameOut.Clear();
			m_lspClientInfo[i]->m_ServerUpdateMask &= ~SERVER_CLIENTUPDATE_DELTAFRAME;*/
		}

	m_pBlockNavGrid = NULL;
	m_pNavGraph = NULL;

	if (m_spSpaceEnum)
	{
		for (int i = 0; i < m_lspObjects.Len(); ++i)
		{
			CWObject* pObj = m_lspObjects[i];
			if (pObj)
			{
				m_spSpaceEnum->Remove(i);
				if (pObj->m_pNameNode)
					Object_SetName(i, NULL);
			}
		}
	}
	m_ObjPoolDynamics.Clear();
	m_lspObjects.Clear();
	m_spSpaceEnum = NULL;
	m_spGUIDHash = NULL;
	m_lEnumSpace.Clear();
	m_NameSearchTree.Clear();


	m_spObjectHeap = NULL;
	m_spMsgQueue = NULL;
	m_spCurLevelKeys = NULL;

	m_spMapData = NULL;

	m_NextGUID = 1;

	m_spBlockNavSearcher = NULL;
	m_spGraphPathfinder = NULL;
	m_spBlockNav = NULL;

	m_liObjectDeferredDestroy.Clear();

#ifndef DEF_DISABLE_PERFGRAPH
	m_PerfGraph_lObjects.Clear();
#endif


	/*
		Dynamics
	 */
#ifndef WSERVER_DYNAMICSENGINE2
	m_DynamicsWorld.Clear();
#endif
	m_lConstraints.Clear();
//	m_NextConstraintID = 0;
}


void CWorld_ServerCore::World_DoOnSpawnWorld()
{
	MAUTOSTRIP(CWorld_ServerCore_World_DoOnSpawnWorld, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ServerCore::World_DoOnSpawnWorld, WORLD_SERVER);
	
	// Create a selection so that we don't risk running OnSpawnWorld on any objects beeing spawned during this process.
//	int nObj = m_lspObjects.Len();
	TSelection<CSelection::LARGE_BUFFER> Selection;
	{
		Selection_AddAll(Selection);
		const int16* piObj = NULL;
		int nObj = Selection_Get(Selection, &piObj);
		int i;
		for(i = 0; i < nObj; i++)
		{
			CWObject* pObj = Object_Get(piObj[i]);
			if (pObj) pObj->OnSpawnWorld();
		}
		m_WorldModelCollider.OnSpawnWorld(m_spMapData);
		for(i = 0; i < nObj; i++)
		{
			CWObject* pObj = Object_Get(piObj[i]);
			if (pObj) pObj->OnSpawnWorld2();
		}
	}
	// After spawning all objects, reset the "once" spawn flag
	m_CurSpawnMask &= ~SERVER_SPAWNFLAGS_ONCE;

	// Commit deferred deletes
	Object_CommitDeferredDestruction();
}


void CWorld_ServerCore::World_PushSpawnTargetNamePrefix(CStr _Prefix)
{
	MAUTOSTRIP(CWorld_ServerCore_World_PushSpawnTargetNamePrefix, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ServerCore::World_PushSpawnTargetNamePrefix, WORLD_SERVER);
	if (m_lSpawnTargetNamePrefixStack.Len())
		m_lSpawnTargetNamePrefixStack.Add(CStrF("%s%s", m_lSpawnTargetNamePrefixStack[m_lSpawnTargetNamePrefixStack.Len()-1].Str(), _Prefix.Str()));
	else
		m_lSpawnTargetNamePrefixStack.Add(_Prefix);
}

void CWorld_ServerCore::World_PopSpawnTargetNamePrefix()
{
	MAUTOSTRIP(CWorld_ServerCore_World_PopSpawnTargetNamePrefix, MAUTOSTRIP_VOID);
	m_lSpawnTargetNamePrefixStack.Del(m_lSpawnTargetNamePrefixStack.Len()-1);
}

CStr CWorld_ServerCore::World_GetSpawnTargetNamePrefix()
{
	MAUTOSTRIP(CWorld_ServerCore_World_GetSpawnTargetNamePrefix, CStr());
	if (!m_lSpawnTargetNamePrefixStack.Len())
		return CStr();

	return m_lSpawnTargetNamePrefixStack[m_lSpawnTargetNamePrefixStack.Len()-1];
}

CStr CWorld_ServerCore::World_MangleTargetName(CStr _TargetName)
{
	MAUTOSTRIP(CWorld_ServerCore_World_MangleTargetName, CStr());
	if (m_lSpawnTargetNamePrefixStack.Len())
		return CStrF("%s%s", World_GetSpawnTargetNamePrefix().Str(), _TargetName.UpperCase().Str());
	else
		return _TargetName.UpperCase();
}


uint32 CWorld_ServerCore::World_TranslateSpawnFlags(const char* _pFlagsStr) const
{
	static const char* gs_ServerFlagsTranslate[] = { "easy", "medium", "hard", "commentary", NULL };
	return CStrBase::TranslateFlags(_pFlagsStr, gs_ServerFlagsTranslate);
}


bool CWorld_ServerCore::World_TestSpawnFlags(uint32 _Flags, bool _bMaskPrevFlags) const
{
	uint32 Test = (m_CurSpawnMask & _Flags);
	if (_bMaskPrevFlags)
		Test &= ~m_PrevSpawnMask;

	bool bTest1 = ((_Flags & SERVER_SPAWNFLAGS_DIFFICULTY_MASK) == 0) || ((Test & SERVER_SPAWNFLAGS_DIFFICULTY_MASK) != 0);
	bool bTest2 = ((_Flags & SERVER_SPAWNFLAGS_GLOBAL_MASK) == 0)     || ((Test & SERVER_SPAWNFLAGS_GLOBAL_MASK) != 0);
	bool bTest3 = ((_Flags & SERVER_SPAWNFLAGS_LAYERS_MASK) == 0)     || ((Test & SERVER_SPAWNFLAGS_LAYERS_MASK) != 0);
	bool bTest4 = ((_Flags & SERVER_SPAWNFLAGS_CUSTOM_MASK) == 0)     || ((Test & SERVER_SPAWNFLAGS_CUSTOM_MASK) != 0);

	return (bTest1 && bTest2 && bTest3 && bTest4);
}


bool CWorld_ServerCore::World_TestSpawnFlags(const char* _pFlagsStr, bool _bMaskPrevFlags) const
{
	uint32 Flags = World_TranslateSpawnFlags(_pFlagsStr);
	return World_TestSpawnFlags(Flags, _bMaskPrevFlags);
}


void CWorld_ServerCore::World_SpawnObjectFromKeys(const CRegistry& _Obj, const CMat4Dfp32* _pTransform)
{
	MAUTOSTRIP(CWorld_ServerCore_World_SpawnObjectFromKeys, MAUTOSTRIP_VOID);

	const CRegistry *pReg = &_Obj;
	if (pReg->GetValuei("EVALED", 0))
	{
		pReg = pReg->FindChild("TEMPLATES");
		if (pReg && pReg->GetNumChildren())
			pReg = pReg->GetChild(0);
	}

	CStr ServerFlags;
	if (pReg)
		ServerFlags = pReg->GetValue("SERVERFLAGS");


	if (!ServerFlags.Len() || World_TestSpawnFlags(ServerFlags))
	{
		Object_Create(_Obj, _pTransform);
	}
	else
	{
		VERBOSE_OUT("[SERVER], rejected object (class: '%s', name: '%s', spawnflags: '%s'), Server SpawnMask: '%s')\n", 
					pReg->GetValue("CLASSNAME").Str(), pReg->GetValue("TARGETNAME").Str(), ServerFlags.Str(), World_GetCurrSpawnFlagsStr());
		m_NextGUID++; // to keep GUID's intact when spawning
	}
}


void CWorld_ServerCore::World_SpawnObjectsFromKeys(CRegistryDirectory *_pRegDir, const CMat4Dfp32* _pTransform, bool _bSkipWorldSpawn)
{
	MAUTOSTRIP(CWorld_ServerCore_World_SpawnObjectsFromKeys, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ServerCore::World_SpawnObjectsFromKeys, WORLD_SERVER);

	CMTime PrevTime;

//	ConOutL("(CWorld_ServerCore::World_SpawnObjectsFromMap) Spawning objects.");
	int Len = _pRegDir->NumReg();
	for(int iObj = (_bSkipWorldSpawn) ? 1 : 0; iObj < Len; iObj++)
	{
		m_SpawnCurrentObject++;
	#ifdef PLATFORM_XENON
		static bint bTraceRecordXTraceStartRecording = false;
		if (bTraceRecordXTraceStartRecording)
		{
//			D3D__NullHardware = true;
//			XTraceStartRecording( "cache:\\World_SpawnObjectsFromWorld.bin" );
			DWORD flags =  DM_PROFILE_FLAG_HW_THREAD2;
			DmSetProfilingOptions( flags );
			static HRESULT Ret;
			Ret = DmStartProfiling(CStrF("e:\\World_SpawnObjectsFromWorld_%04d.cap", iObj), 96*1024*1024);
		}
	#endif
	//	CMTime Timer;
		{
	//		TMeasure(Timer);
			World_SpawnObjectFromKeys(*_pRegDir->GetReg(iObj), _pTransform);
		}
	//	M_TRACEALWAYS("%s\n", TString(CStrF("Spawn Object Time %d", iObj).Str(), Timer).Str());
	#ifdef PLATFORM_XENON
		if (bTraceRecordXTraceStartRecording)
		{
//			XTraceStopRecording();
			DmStopProfiling();
		}
	#endif
	}
}

void CWorld_ServerCore::World_SpawnObjectsFromWorld(CStr _Name, const CMat4Dfp32* _Transform, bool _bSkipWorldSpawn)
{
	MAUTOSTRIP(CWorld_ServerCore_World_SpawnObjectsFromWorld, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ServerCore::World_SpawnObjectsFromWorld, WORLD_SERVER);

	CStr OldWorld = m_spMapData->GetWorld();
	m_spMapData->SetWorld(_Name);

	CRegistryDirectory *pRegDir = World_LoadObjects(_Name);
	m_SpawnLen = pRegDir->NumReg();
	m_SpawnCurrentObject = 0;

	VERBOSE_OUT("[SERVER], World_SpawnObjectsFromWorld, SpawnMask: 0x%08X\n", m_CurSpawnMask);
	World_SpawnObjectsFromKeys(pRegDir, _Transform, _bSkipWorldSpawn);

	m_spMapData->SetWorld(OldWorld);
}

fp32 CWorld_ServerCore::Load_GetProgress()
{
	// Check how many objects are spawned 
	if (m_SpawnLen <= 0)
		return 0.0f;

	return (fp32)m_SpawnCurrentObject / (fp32)m_SpawnLen;
}

void CWorld_ServerCore::Load_ProgressReset()
{
	m_SpawnLen = 0;
	m_SpawnCurrentObject = 0;
}

/*
void CWorld_ServerCore::World_SpawnPlayers()
{
	MAUTOSTRIP(CWorld_ServerCore_World_SpawnPlayers, MAUTOSTRIP_VOID);
}

bool CWorld_ServerCore::World_Migrate(CStr _World, CSelection& _Selection)
{
	MAUTOSTRIP(CWorld_ServerCore_World_Migrate, false);
	MSCOPE(CWorld_ServerCore::World_Migrate, WORLD_SERVER);
	
	// FIXME: Player handling for migration is not functional
	Error("World_Migrate", "Out of order.");

	CStr Name = CStrF("WORLDS\\%s", (char*)_World);
	Name = m_spWData->ResolveFileName(Name);

	if (!CDiskUtil::FileExists(Name))
	{
		ConOut(Name + " doesn't exist.");
		return false;
	}

	CHECKMEMORY("World_Migrate (1)");
	ConOutL("(CWorld_ServerCore::World_Migrate) " + _World);
	// Assign names to all player-objects.
	{
		for(int i = 0; i < m_lspPlayers.Len(); i++)
		{
			if ((m_lspPlayers[i]->m_Flags & (PLAYER_FLAGS_INUSE | PLAYER_FLAGS_PLAYER)) == (PLAYER_FLAGS_INUSE | PLAYER_FLAGS_PLAYER))
			{
				if (Object_Get(m_lspPlayers[i]->m_iObject))
					Object_SetName(m_lspPlayers[i]->m_iObject, CStrF("PLAYER%d", i));
				if (!Selection_ContainsObject(_iSel, m_lspPlayers[i]->m_iObject))
					ConOutL(CStrF("§cf80WARNING: (CWorld_ServerCore::World_Migrate) Object %d for player %d is not in selection.", m_lspPlayers[i]->m_iObject, i));
			}
		}
	}

	CHECKMEMORY("World_Migrate (2)");

	World_ChangeFromGame(_World, SERVER_CHANGEWORLD_MIGRATION);

	ConOutL("(CWorld_ServerCore::World_Migrate) Done.");
	CHECKMEMORY("World_Migrate (3)");
	return true;
}
*/
