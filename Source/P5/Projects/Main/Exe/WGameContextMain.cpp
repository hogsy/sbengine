
#include "PCH.h"

#include "WGameContextMain.h"
#include "../../../Shared/MOS/Classes/GameWorld/Server/WServer.h"
#include "../../../Shared/MOS/Classes/GameWorld/Server/WServer_Core.h"
#include "../../../Shared/MOS/Classes/GameWorld/WPackets.h"
#include "../../../Shared/MOS/Classes/GameWorld/WDataRes_MiscMedia.h"
//#include "../../../Shared/Shared/MOS/MSystem/Input/XBox_XTL/MInput_XTL.h"
#include "../GameWorld/WFrontEndMod.h"
#include "../../Projects/Main/GameWorld/WServerMod.h"

//#include "../../../Shared/MOS/Classes/GameWorld/WObjCore.h"
#include "../GameClasses/WObj_CharMsg.h"

#include "../Exe_Xenon/Darkness.spa.h"

static const char *g_ContentTypeTranslate[] =
{
	"picture", "video", "timelocklevel", "extralevel", "text", "sound", "script", NULL
};
//
//
//
CExtraContentHandler::CExtraContentHandler()
{
	m_UpdatingContent = false;
}

bool CExtraContentHandler::AddKeyBit(int _KeyID, TArray<CContent *> *_lpContent)
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(!pSys || !pSys->GetOptions())
		return false;

	CExtraContentKey Key;
	Key.Parse(pSys->GetOptions()->GetValue("GAME_EXTRACONTENTKEY"));

	CExtraContentKey Key2 = Key;
	if(!Key2.AddKeyBit(_KeyID))
		return false;

	if(_lpContent)
	{
		for(int i = 0; i < m_lspContent.Len(); i++)
			if(Key2.IsActive(m_lspContent[i]->m_Key) &&
				!Key.IsActive(m_lspContent[i]->m_Key))
				_lpContent->Add(m_lspContent[i]);
	}

	pSys->GetOptions()->AddKey("GAME_EXTRACONTENTKEY", Key2.GetString());
	return true;
}


//
//
//
void CExtraContentHandler::Update()
{
	m_lspContent.SetLen(0);
	for(int32 i = 0; i < m_lProviders.Len(); i++)
		m_lProviders[i]->UpdateContentList();
	m_UpdatingContent = true;
}

//
//
//
bool CExtraContentHandler::IsUpdateDone()
{
	if(!m_UpdatingContent)
		return true;

	int32 DoneCount = 0;
	for(int32 i = 0; i < m_lProviders.Len(); i++)
		DoneCount += m_lProviders[i]->IsContentUpdateDone();

	if(DoneCount == m_lProviders.Len())
	{
		// all providers are done
		m_UpdatingContent = false;
		return true;
	}
	return false;
}


void CExtraContent_Offline::UpdateContentList()
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) Error_static("Create", "No system.");

	CStr ExtraContentPath = pSys->m_ExePath + "ExtraContent\\";
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

bool CExtraContent_Offline::IsContentUpdateDone()
{
	return true;
}

CGameContextMod::CGameContextMod()
{
	m_PendingWindow = 0;
	m_Pad_ShowReconnect = 0;
	m_Pad_Active = -1;
	m_LockedPad = -1;

	m_SilentLogonRequested = false;

	m_BlackLoadingBG = false;
	m_LoadHinted = false;

#if defined(PLATFORM_CONSOLE)
	m_pMPHandler = NULL;
#endif
}

CGameContextMod::~CGameContextMod()
{
	m_spAsyncSaveContext = NULL;
}

void CGameContextMod::Create(CStr _WorldPathes, CStr _GameName, spCXR_Engine _spEngine)
{
	CGameContext::Create(_WorldPathes, _GameName, _spEngine);

#if defined(PLATFORM_XBOX1) && !defined(M_DEMO_XBOX)
	#ifdef XBOX_SUPPORT_DLCONTENT
		m_ExtraContent.m_lProviders.Add(&m_ExtraContent_Live);
		m_ExtraContent_Live.m_pHandler = &m_ExtraContent;
	#endif
	m_ExtraContent_Live.m_pFriendsHandler = &m_FriendsHandler;
	m_ExtraContent_Live.m_pGameContext = this;
#endif
	m_ExtraContent.m_lProviders.Add(&m_ExtraContent_Offline);
	m_ExtraContent_Offline.m_pHandler = &m_ExtraContent;

	m_ExtraContent.Update();

#if defined(PLATFORM_XENON)
	m_pMPHandler = DNew(CWGameLiveHandler) CWGameLiveHandler;
#endif
#if defined(PLATFORM_PS3)
	m_pMPHandler = DNew(CWGamePSNetworkHandler) CWGamePSNetworkHandler;
#endif
}

void CGameContextMod::RenderSplitScreenGUI(CXR_VBManager* _pVBM, CRenderContext* _pRC, bool _bVSplit, bool _bHSplit)
{
	CRC_Util2D Util2D;
	Util2D.Begin(_pRC, _pVBM->Viewport_Get(), _pVBM);
	CRct Rect = _pVBM->Viewport_Get()->GetViewRect();
	Util2D.SetCoordinateScale(CVec2Dfp32(Rect.GetWidth() * (1.0f/640.0f) , Rect.GetHeight() * (1.0f/480.0f)));
	CClipRect Clip(0, 0, 640, 480);

	Util2D.GetAttrib()->Attrib_ZCompare(CRC_COMPARE_ALWAYS);
	Util2D.GetAttrib()->Attrib_Enable(CRC_FLAGS_ZCOMPARE);
	Util2D.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

	if(_bHSplit)
	{
		Util2D.SetTextureScale(1, 1);

		int Height = 32;
		int lWidth[] = { 256, 256, 128 };
		char *lTextures[] = { "GUI_SPLITBAR02_01", "GUI_SPLITBAR02_02", "GUI_SPLITBAR02_03" };
		int x = 0;
		for(int i = 0; i < 3; i++)
		{
			Util2D.SetTexture(lTextures[i]);
			Util2D.SetTextureOrigo(Clip, CPnt(x, 480 / 2 - Height / 2));
			Util2D.Rect(Clip, CRct(x, 480 / 2 - Height / 2, x + lWidth[i], 480 / 2 + Height / 2), 0xff808080);
			x += lWidth[i];
		}
	}
	if(_bVSplit)
	{
		Util2D.SetTextureScale(1, 1);

		int lHeight[] = { 256, 128, 64, 32 };
		char *lTextures[] = { "GUI_SPLITBAR02_04", "GUI_SPLITBAR02_05", "GUI_SPLITBAR02_06", "GUI_SPLITBAR02_07" };

		int Width = 32;
		int y = 0;
		for(int i = 0; i < 4; i++)
		{
			Util2D.SetTexture(lTextures[i]);
			Util2D.SetTextureOrigo(Clip, CPnt(640 / 2 - Width / 2, y));
			Util2D.Rect(Clip, CRct(640 / 2 - Width / 2, y, 640 / 2 + Width / 2, y + lHeight[i]), 0xff808080);
			y += lHeight[i];
		}
	}
	Util2D.End();
}

void CGameContextMod::Simulate_Pause()
{
#ifdef PLATFORM_CONSOLE
	if(m_pMPHandler && m_pMPHandler->m_Flags & MP_FLAGS_INITIALIZED)
		return;
#endif
	CGameContext::Simulate_Pause();
}

void CGameContextMod::DoLoadingStream()
{
	if (m_spWServer && m_spSoundContext)
	{
		CRegistry* pReg = m_spWServer->Registry_GetLevelKeys("Game");
		int32 PrevLevelMask = m_spWServer->m_PrevSpawnMask;
		
		if (pReg)
		{
			// Find load monologue
			CRegistry* pMonoReg = pReg->FindChild("LoadingStream");
			CWFrontEnd_Mod *pFrontEnd = safe_cast<CWFrontEnd_Mod>( (CWFrontEnd *)m_spFrontEnd );
			if (pMonoReg && pFrontEnd)
			{
				TArray<CStr> lMonolog;
				TArray<CStr> lMusicRandom;
				CStr LoadingMusic;
				int32 Len = pMonoReg->GetNumChildren();
				fp32 Volumes[2];
				Volumes[0] = 1.0f;
				Volumes[1] = 1.0f;
				const char* pMusicToPlay[2];
				pMusicToPlay[0] = NULL;
				pMusicToPlay[1] = NULL;

				CStr LoadingBG = "";
				for (int32 i = 0; i < Len; i++)
				{
					CRegistry* pChild = pMonoReg->GetChild(i);
					if (!pChild)
						continue;

					CStr Type = pChild->GetThisName();
					CStr StrMusic = pChild->GetThisValue();
					switch (StringToHash(Type))
					{
					case MHASH2('VOCA', 'P'): // "COVAP"
						{
							lMonolog.Add(StrMusic);
							break;
						}
					case MHASH3('LOAD','INGM','USIC'): // "LOADINGMUSIC"
						{
							LoadingMusic = StrMusic;
							pMusicToPlay[1] = LoadingMusic.Str();
							break;
						}
					case MHASH2('RAND','OM'): // "RANDOM"
						{
							while (StrMusic.Len())
							{
								lMusicRandom.Add(StrMusic.GetStrSep(","));
							}
							break;
						}
					case MHASH3('VOLU','MEMU','SIC'): // "VOLUMEMUSIC"
						{
							pChild->GetThisValueaf(1,&Volumes[1]);
							break;
						}
					case MHASH3('VOLU','MESF','X'): // "VOLUMESFX"
						{
							pChild->GetThisValueaf(1,&Volumes[0]);
							break;
						}
					case MHASH3('LOAD', 'INGB', 'G'): // "LOADINGBGP"
						{
							LoadingBG = StrMusic;
							break;
						}
					default:
						break;
					}
				}

				//bool bOk = false;
				if (lMonolog.Len())
				{
					CWServer_Mod *pServerMod = m_spWServer ? TDynamicCast<CWServer_Mod>((CWorld_Server *)m_spWServer) : NULL;
					if(pServerMod && pServerMod->m_GameState.IsFirstLevelVisit(pServerMod))
					{
						pFrontEnd->m_Cube.SetLoadingSceneAnimation(lMonolog[0], m_spWServer->m_WorldName, LoadingBG);
					}
					else
					{
						int Len = lMonolog.Len();
						int UseThisLoadingMonologue = 0;
						if(Len > 1)
							UseThisLoadingMonologue = (1 + ((int)(Random * 100.0f)) % (Len-1));	
						pFrontEnd->m_Cube.SetLoadingSceneAnimation(lMonolog[Min(Len-1, UseThisLoadingMonologue)], m_spWServer->m_WorldName, LoadingBG);
					}
				}
				else
					pFrontEnd->m_Cube.SetLoadingSceneAnimation("", "", LoadingBG);


				/*
				if (!bOk) 
				{
					// Play a random bit
					int Len = lMusicRandom.Len();
					pMusicToPlay[0] = Len > 0 ? lMusicRandom[((int)(Random * 100.0f)) % Len].Str() : NULL;
				}
				*/

				if (pMusicToPlay[0])
				{
					int32 Num = pMusicToPlay[1] ? 2 : 1;
					m_spSoundContext->MultiStream_Play(pMusicToPlay,Num,true,true,M_Bit(1));
					// Get music/or sfx volume from options reg...?
					if (m_pSystem)
					{
						Volumes[0] *= m_pSystem->GetOptions()->GetValuef("SND_VOLUMESFX", 1.0f);
						Volumes[1] *= m_pSystem->GetOptions()->GetValuef("SND_VOLUMEMUSIC", 1.0f);
					}
					m_spSoundContext->MultiStream_Volumes(Volumes, Num,false,NULL);
				}
			}
			else if(pFrontEnd)
				pFrontEnd->m_Cube.SetLoadingSceneAnimation("", "");

			// Precache resources
			if (m_pSystem->m_spDisplay)
			{
				M_TRACEALWAYS("---- Precaching loading monologue ----\n");
				pFrontEnd->DoPrecache(NULL, m_spEngine);
				M_TRACEALWAYS("---- Done precaching loading monologue ----\n");
			}

			// Precache is done, let CCube start the loading monologue
			pFrontEnd->m_Cube.m_GUIFlags |= CCube::GUI_FLAGS_READYTOSTART_LS;
		}
	}
}

void CGameContextMod::Render_Loading(CXR_Engine * _pEngine, CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP, int _Context)
{
	CWFrontEnd_Mod *pFrontEnd = ((CWFrontEnd_Mod *)((CWFrontEnd *)m_spFrontEnd));

	CFStr28 LoadingScreen;
	static int m_bDelayTicks = 0;
	static int m_CubeRemoval = 0;

	static int m_bIngamePrecache = 0;

	M_LOCK(m_ServerDeleteLock);

	if(m_spWServer)
	{
		CWorld_Client *pClient = GetCurrentClient();
		if((pClient && (pClient->GetClientState() != WCLIENT_STATE_INGAME || pClient->Precache_Status())) || (m_spWServer->IsLoading()))
		{
			if (pClient && pClient->GetClientState() == WCLIENT_STATE_INGAME && pClient->Precache_Status())
			{
				m_bIngamePrecache = true;
			}

			// This is borken
//			CRegistry *pReg = m_spWServer->Registry_GetLevelKeys("");
//			if(pReg)
//				LoadingScreen = pReg->GetValue("LOADINGSCREEN");
			if(LoadingScreen == "")
				LoadingScreen = "GUI_Splash02";
		}
	}
	else 
	{
		CWorld_Client *pClient = GetCurrentClient();
		if((pClient && (pClient->GetClientState() != WCLIENT_STATE_INGAME || pClient->Precache_Status())))
		{
			if (pClient && pClient->GetClientState() == WCLIENT_STATE_INGAME && pClient->Precache_Status())
			{
				m_bIngamePrecache = true;
			}
			if(LoadingScreen == "")
				LoadingScreen = "GUI_Splash02";
		}
	}

	static CFStr28 LastLoadingScreen;
	static bool UsingCube = false;
	static int32 Tip = 0;

	if(!pFrontEnd)
	{
		m_CubeRemoval = 0;
		return;
	}


	CRC_Util2D Util2D;
	Util2D.Begin(_pRC, _pVBM->Viewport_Get(), _pVBM);
	CRct Rect = _pVBM->Viewport_Get()->GetViewRect();
	Util2D.SetCoordinateScale(CVec2Dfp32(Rect.GetWidth() *(1.0f/640.0f) , Rect.GetHeight() * (1.0f/480.0f)));
	Util2D.GetAttrib()->Attrib_ZCompare(CRC_COMPARE_ALWAYS);
	Util2D.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZWRITE);
	Util2D.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_NONE);
	CClipRect Clip(0, 0, 640, 480);

#ifndef M_DEMO_XBOX
	if(LoadingScreen != "")
	{
		if (!m_bIngamePrecache)
			if(m_spFrontEnd != NULL && m_spFrontEnd->m_spDesktop != NULL)
			{
				m_spFrontEnd->ClearMenus();
//				ConExecute("cg_clearmenus()");
			}
	}
	else
	{
		if (m_bIngamePrecache)
		{
			m_bIngamePrecache = false;
			ConExecute("cg_cubeseq(\"normal\")");
			ConExecute("cg_cuberesetwindow()");
			m_CubeRemoval = 0;
		}
		else
		{
			if (pFrontEnd && pFrontEnd->m_spDesktop != NULL)
				m_CubeRemoval = 0;
		}
	}
#endif

	if(LoadingScreen != "" || m_bDelayTicks)
	{
		if(m_spWServer)
		{
			if(m_bDelayTicks == -1)
				m_bDelayTicks = m_spWServer->GetGameTick()+5;

			if(m_bDelayTicks <= m_spWServer->GetGameTick() || m_spFrontEnd->m_spDesktop != NULL)
			{
				m_bDelayTicks = 0;
				if(!(LoadingScreen != ""))
					m_BlackLoadingBG = false;
			}
		}
		else
			m_bDelayTicks = 0;

		//M_TRACEALWAYS("delaying: %d\n", m_spWServer->GetGameTick());
		//if()
		//	m_bDelayTicks--;


		/*
			{
				UVMin = CVec2Dfp32(0, 0);
				UVDelta = CVec2Dfp32(853/1024.0f, sh/512.0f);
			}

			for(int32 i = 0; i < 5; i++)
				aViewVerts[i].k[1] *= (640/853.0f);
				*/
		Util2D.SetTextureScale(1,1);
		Util2D.SetTextureOrigo(Clip, CPnt(0, 0));

	/*
#ifndef M_DEMO_XBOX
		CRct WholeScreen(0,0,640,480);
 		if(m_BlackLoadingBG)
			Util2D.Rect(Clip, WholeScreen, 0xff000000);
		else if(pFrontEnd && pFrontEnd->m_spGameTexture != NULL)
		{
			int TextureID = pFrontEnd->m_spGameTexture->GetTextureID(0);

			if (_pRC->Texture_GetTCIDInfo()->m_pTCIDInfo[TextureID].m_Fresh & 1)
			{
				Util2D.SetTexture(TextureID);
				CPnt ScreenSize = _pRC->GetDC()->GetScreenSize();

				if (_pRC->Caps_Flags() & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP)
				{
					Util2D.SetTextureScale(fp32(ScreenSize.x) / 640.0f, -fp32(ScreenSize.y) / 480.0f);
					Util2D.SetTextureOrigo(Clip, CPnt(0, (int)(Util2D.GetTextureHeight() * 480.0f / fp32(ScreenSize.y))));
				}
				else
				{
					Util2D.SetTextureScale(fp32(ScreenSize.x) / 640.0f, fp32(ScreenSize.y) / 480.0f);
					Util2D.SetTextureOrigo(Clip, CPnt(0, 0));
				}
				Util2D.Rect(Clip, WholeScreen, 0xffFFFFFF);
			}
			else
				Util2D.Rect(Clip, WholeScreen, 0xff000000);
		}
#else
#endif
		*/
	
/*
		if(Util2D.SetTexture(LoadingScreen))
		{
			CClipRect Clip(0, 0, 640, 480);
			Util2D.SetTextureScale(853/640.0f, 853/640.0f);
			Util2D.SetTextureOrigo(Clip, CPnt(0, (480-480*(640/853.0f))/2));
			Util2D.Rect(Clip, CRct(0, (480-480*(640/853.0f))/2, 640, 640-(853.0f-640)), 0xff808080);
		}
		*/

		
	}
	else
		m_bDelayTicks = 0;

	bool NeedCubeRef = false;
	bool NeedCube = false;
	bool Loading = (LoadingScreen != "") != 0;

	if(m_CubeRemoval && pFrontEnd)
	{
		if(m_CubeRemoval == 150)
			pFrontEnd->Cube_InitLoadingCubeRemoval();

		m_CubeRemoval--;
	}

	if(Loading || m_LoadHinted || m_CubeRemoval > 0)
		NeedCube = true;

	if(Loading)
		NeedCubeRef = true;
	else
		Tip = ((int32)(Random*5))%5;

	if(NeedCube)
	{
		//M_TRACEALWAYS("loading: %d\n", m_spWServer->GetGameTick());

		if(Loading)
		{
			_pRC->Render_SetMainMemoryOverride(false);
	//		_pRC->Render_SetOnlyAllowExternalMemory(true);

			m_LoadHinted = false;
			m_bDelayTicks = -1;
			m_CubeRemoval = 150;

			if(m_spFrontEnd != NULL)
			{
				m_spFrontEnd->m_GameIsLoading = true;
			}

			

			// disable vibration
			MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
			if (pSys && pSys->m_spInput != NULL)
			{
				pSys->m_spInput->FlushEnvelopes();
				pSys->m_spInput->SetFeedbackAmount(1);
			}

			static int HasBlocked = false;
			if (!HasBlocked)
			{
				HasBlocked = true;
				m_spWData->Resource_AsyncCacheBlockOnCategory(0);
			}
			
			if(LoadingScreen != LastLoadingScreen)
			{
				LastLoadingScreen = LoadingScreen;
			}


			#ifdef M_DEMO_XBOX
				ConExecute("pausetimer5()");
			#endif
		}

		if(pFrontEnd && pFrontEnd->m_spDesktop != NULL && m_spFrontEnd->m_spDesktop->m_BlocksLoadingScreen)
		{
			return;
		}

#ifndef PLATFORM_PS2
		
		if(NeedCubeRef && !UsingCube)
		{
			pFrontEnd->m_CubeReferneces++;
			UsingCube = true;
		}

		//CCubeUser m_CubeUser;
		//if(Loading)
		{
			//pFrontEnd->Cube_DoLoadingLayout();
			pFrontEnd->m_Cube.Update();
			//pFrontEnd->Cube_Render(_pRC, _pVBM, false);

			//whoomp
			pFrontEnd->Cube_Render(_pRC, _pVBM, false, true); // override, special funktion
			//pFrontEnd->m_Cube.Render_P5(_pRC, _pVBM, pFrontEnd->m_Cube.m_FrameTime, pFrontEnd->m_Cube.GetGUIProcessContainer(), true, _pEngine, pFrontEnd->m_GameIsLoading, true);
		}

		/*
		if(m_CubeRemoval)
		{
			if(pFrontEnd->m_Cube.m_SequenceTime < 0.5f)
			{
				pFrontEnd->m_Cube.Update();
				pFrontEnd->Cube_Render(_pRC, _pVBM, false);
			}
			else
				m_CubeRemoval = 0;
		}
		*/
		
#endif

		//if(0)
		{		
			CClipRect Clip(0, 0, 640, 480);
			Util2D.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
			Util2D.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			
//			int x0 = 0;
//			int y0 = 480/2-60;
//			int x1 = 640;
//			int y1 = (int)(480 * 0.85f - 25);
			
			int StyleCenter = WSTYLE_TEXT_WORDWRAP | WSTYLE_TEXT_CENTER | WSTYLE_TEXT_CENTERY;
			int iFont = m_spFrontEnd->m_spWData->GetResourceIndex("XFC:HEADINGS", NULL);
			if(iFont > 0)
			{
				TPtr<CWRes_XFC> spRcFont = TDynamicCast<CWRes_XFC>((CWResource*)m_spFrontEnd->m_spWData->GetResourceRef(iFont));
				CRC_Font *pFont = 0;
				if(spRcFont) pFont = spRcFont->GetFont();
				if(pFont)
				{
					fp32 FlashTime = CMTime::GetCPU().ModulusScaled(2.0f, 1.0f).GetTime();
					uint8 Intensity = (uint8)(255 * Sqr(M_Sin(FlashTime * _PI)));

					/*if (Loading)
					{
						Util2D.Text_DrawFormatted(
							Clip, pFont, "§LSTD_LOADING", (int)(640*0.15f/2+1), (int)(480-480*0.15f/2-25+1),
							StyleCenter, CPixel32(0,0,0, Intensity), 0, 0, (int)(640 * 0.85f), 20, false);

						Util2D.Text_DrawFormatted(
							Clip, pFont, "§LSTD_LOADING", (int)(640*0.15f/2), (int)(480-480*0.15f/2-25),
							StyleCenter, CPixel32(95,90,70, Intensity), 0, 0, (int)(640*0.85f), 20, false);
					}

					//if(pFrontEnd->Cube_LoadingIsTakingToMuchTime())
					if(Loading && pFrontEnd->Cube_LoadingIsTakingToMuchTime())
					{
						Util2D.Text_DrawFormatted(
							Clip, pFont, CStrF("§LLOADING_TIP%d", Tip+1), (int)(640 * 0.15f/2), 480/2-100,
							StyleCenter, CPixel32(95,90,70, 0xff), 0, 0, (int)(640 * 0.85f), 20, false);
					}*/

					/*
					if(m_LoadHinted)
					{
						Util2D.Text_DrawFormatted(
							Clip, pFont, "Entering Loading", 640*0.15f/2, 480-480*0.15f/2-25,
							StyleCenter, CPixel32(95,90,70, 0xff), 0, 0, 640*0.85f, 20, false);
					}*/
				}

				fp32 Amount = 0.0f;
				int32 size = m_spWData->XDF_GetSize();
				if (size > 0)
				{
					int32 pos = m_spWData->XDF_GetPosition();
					Amount = pos/(fp32)size;
				}
				else
				{
					fp32 Status = m_spWServer ? m_spWServer->Load_GetProgress() : 0.0f;
					fp32 StatusClient = 0.0f;
					if ((m_lspWClients.Len() > 0) && (m_lspWClients[0] != NULL))
						StatusClient = m_lspWClients[0]->Load_GetProgress();
					Amount = (Status + StatusClient) * 0.5f;
				}

				int bx = 150;
				int ym = 420, hy = 6;
				int lw = 2;
				int x0 = bx, x1 = 640 - bx, w = 640 - 2*bx - lw*4;
				int y0 = ym - hy, y1 = ym + hy;

				
				CWorld_Client *pClient = GetCurrentClient();
				if(!pClient || (pClient && !(pClient->CanRenderGame())))
				{
					_pVBM->ScopeBegin("Loading", false);
					Util2D.Rect(Clip, CRct(x0, y0, x1, y1), 0x5f000000);
					Util2D.Rect(Clip, CRct(x0+lw, y0, x1-lw, y0+lw), 0xafffffff); // upper
					Util2D.Rect(Clip, CRct(x0, y0, x0+lw, y1), 0xafffffff); // left
					Util2D.Rect(Clip, CRct(x1-lw, y0, x1, y1), 0xafffffff); // right
					Util2D.Rect(Clip, CRct(x0+lw, y1-lw, x1-lw, y1), 0xafffffff); // lower
					Util2D.Rect(Clip, CRct(x0+lw*2, y0+lw*2, (int)(x0+lw*2 + w*Amount), y1-lw*2), 0xafffffff);
					_pVBM->ScopeEnd();
				}
				
				
			}
		}

		//*/
	}
	else
	{
		if(m_spFrontEnd != NULL)
			m_spFrontEnd->m_GameIsLoading = false;



		if(LastLoadingScreen != "")
		{
			_pRC->Render_SetOnlyAllowExternalMemory(false);
#ifdef M_DEMO
			ConExecute("pausetimer()");
#endif
			LastLoadingScreen = "";
		}
	
	}

	if(!NeedCubeRef && UsingCube && pFrontEnd)
	{
		pFrontEnd->m_CubeReferneces--;
		UsingCube = false;
	}
} 

void CGameContextMod::SetDefaultProfileSettings(CRegistry* _pOptions)
{
#ifdef PLATFORM_CONSOLE
	m_pMPHandler->SetDefaultProfileSettings(_pOptions);
#endif
}

void CGameContextMod::OnDisconnect()
{
	CGameContext::OnDisconnect();
	CWFrontEnd_Mod *pFrontEnd = ((CWFrontEnd_Mod *)((CWFrontEnd *)m_spFrontEnd));

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");

	if(pFrontEnd && pFrontEnd->m_spGameTexture != NULL && pTC)
	{
		int TextureID = pFrontEnd->m_spGameTexture->GetTextureID(0);
		pTC->MakeDirty(TextureID);
	}
}

void CGameContextMod::Render_PadStatus(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP, int _Context)
{
	// not is use anymore
}

void CGameContextMod::Render_Corrupt(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP, int _Context)
{
	if (!m_spFrontEnd)
		return;

	if (!_pVBM->Viewport_Push(&_GUIVP))
		return;
	
	CRC_Util2D Util2D;
	Util2D.Begin(_pRC, &_GUIVP, _pVBM);

	Util2D.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE | CRC_FLAGS_CULL);
	Util2D.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);


	CRct Rect = _GUIVP.GetViewRect();
	Util2D.SetCoordinateScale(CVec2Dfp32(Rect.GetWidth() *(1.0f/640.0f),  Rect.GetHeight() * (1.0f/480.0f)));

	{		
		CClipRect Clip(0, 0, 640, 480);
		Util2D.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
		Util2D.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		
		int x0 = 0;
		int y0 = 480/2-60;
		int x1 = 640;
		int y1 = 480/2+60;
		
		Util2D.SetTexture(0);
		
		Util2D.SetTextureScale(1,1);
		Util2D.SetTextureOrigo(Clip, CPnt(x0,y0));
		
		Util2D.Rect(Clip, CRct(x0++, y0++, x1--, y1--), 0xe0000000);
		
		int StyleCenter = WSTYLE_TEXT_WORDWRAP | WSTYLE_TEXT_CENTER | WSTYLE_TEXT_CENTERY;
		
		int iFont = m_spFrontEnd->m_spWData->GetResourceIndex("XFC:HEADINGS", NULL);
		if(iFont > 0)
		{
			CWRes_XFC* pRcFont = (CWRes_XFC*)m_spFrontEnd->m_spWData->GetResource(iFont);
			CRC_Font *pFont = pRcFont->GetFont();
			if(pFont)
			{
				CStr Msg;
				int Status = CDiskUtil::GetCorrupt();
				if (Status & DISKUTIL_STATUS_CORRUPTFILE)
				{
#ifdef M_DEMO_XBOX
					if (g_XboxDemoParams.m_bLaunchedFromDemoLauncher)
						Msg = CStr("§Z22§LSTD_BADGAMEDISCCONTINUE");
					else
						Msg = CStr("§Z22§LSTD_BADGAMEDISC");
#else
					Msg = CStr("§Z22§LSTD_BADGAMEDISC");
#endif
				}
				else
					Msg = CStr("§Z22§LSTD_BADAPP");

				Util2D.Text_DrawFormatted(
					Clip, pFont, 
					Msg,
					48, 
					36,
					StyleCenter, 
					0x7fffffff, 0, 0, 544, 408, false);
			}
		}
	}

	Util2D.End();
	_pVBM->Viewport_Pop();
}

void CGameContextMod::Render(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP, int _Context)
{
	if (m_spFrontEnd)
	{
		((CWFrontEnd_Mod *)((CWFrontEnd *)m_spFrontEnd))->m_3DViewPort = _3DVP;
		((CWFrontEnd_Mod *)((CWFrontEnd *)m_spFrontEnd))->DoClientRenderCheck();
	}

	CGameContext::Render(_pVBM, _pRC, _3DVP, _GUIVP, _Context);
}

void CGameContextMod::RenderGUI(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP, int _Context)
{
	if (_pVBM->Viewport_Push(&_GUIVP) && m_spFrontEnd)
	{
		M_LOCK(m_spFrontEnd->m_FrontEndLock);
		_pVBM->ScopeBegin("CGameContextMod::RenderGUI", true);
		
		m_spFrontEnd->SetRender_Engine(m_spEngine);
		Render_PadStatus(_pVBM, _pRC, _3DVP, _GUIVP, _Context);
		Render_Loading(m_spEngine, _pVBM, _pRC, _3DVP, _GUIVP, _Context);
		m_spFrontEnd->SetRender_Engine(NULL);
		CGameContext::RenderGUI(_pVBM, _pRC, _3DVP, _GUIVP, _Context);
		_pVBM->Viewport_Pop();
		_pVBM->ScopeEnd();
	}
}

uint32 CGameContextMod::GetViewFlags()
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	CWorld_Client* pClient = GetCurrentClient();

	if (pClient)
	{
		return pClient->GetViewFlags();
	}
	return 0;
}



void CGameContextMod::ExecuteCommand(CGC_Command& _Cmd)
{
	/*
	switch(_Cmd.m_Command)
	{

	default:
		CGameContext::ExecuteCommand(_Cmd);
	}
	*/
	CGameContext::ExecuteCommand(_Cmd);
}

void CGameContextMod::Command_Map(CStr _Name)
{
	CGameContext::Command_Map(_Name);
}

void CGameContextMod::Con_CampaignMap(CStr _Map)
{
	Con_SetGameClass("campaign");
	Con_ChangeMap(_Map, 0);
}

CStr CGameContextMod::GetScriptLayerInfo(CStr _ScriptLayer)
{
	// find the right map/layers for this script-layer
	// Read register
	spCRegistry spRegistry;
	CStr RetVal = "";
	//spRegistry = CWorld_ServerCore::ReadServerReg("Sv");

	MACRO_GetSystem;
	CStr Params;
	CStr ContentPath = pSys->GetEnvironment()->GetValue("CONTENTPATH");
	if (ContentPath == "")
		ContentPath = pSys->m_ExePath + "Content\\";

	CStr RegisterFile = ContentPath + "Registry\\";
	RegisterFile += "Sv.xrg";

	// Read register
	TPtr<CRegistryCompiled> spRegCompiled;
	CStr FileName = RegisterFile.GetPath() + RegisterFile.GetFilenameNoExt() + ".xcr";
	if (CDiskUtil::FileExists(FileName))
	{
		spRegCompiled = MNew(CRegistryCompiled);
		if (spRegCompiled)
		{
			spRegCompiled->Read_XCR(FileName);
			spRegistry = spRegCompiled->GetRoot();
		}
	}
	else if (CDiskUtil::FileExists(RegisterFile))
	{
		spRegistry = REGISTRY_CREATE;
		if(spRegistry)
			spRegistry->XRG_Read(RegisterFile);
	}
	else
	{
		ConOutL(CStrF("§cf00ERROR (CWorld_ServerCore::ReadServerReg): file '%s' not found!", RegisterFile.Str()));
		M_TRACEALWAYS("ERROR (CWorld_ServerCore::ReadServerReg): file '%s' not found!\n", RegisterFile.Str());
	}

	if(!spRegistry)
	{
		ConOut("Error reading Registry.");
		return "";
	}

	CRegistry *pLevelKeys = spRegistry->Find("LEVELKEYS");
	if(!pLevelKeys)
	{
		ConOut("Error retrieving LevelKeys. Registry not found");
		spRegistry = NULL;
		return "";
	}

	//------------------------------------------------------------------------------------------------

	// search all maps for the given scriptlayer
	CRegistry *pLevel = NULL;
	int iNrOfChildren = pLevelKeys->GetNumChildren();
	int iStoreMapID = 0;
	for(int i = 0; i < iNrOfChildren; i++)
	{
		CRegistry *pReg = pLevelKeys->GetChild(i);
		if(pReg && pReg->GetNumChildren() > 0)
		{
			int iNumScriptLayers = pReg->GetNumChildren();
			for(int j = 0; j < iNumScriptLayers; j++)
			{
				CStr ScriptLayer =  pReg->GetChild(j)->GetThisValue();
				if(ScriptLayer == _ScriptLayer)
				{
					pLevel = pReg->GetChild(j);
					iStoreMapID = i;
					break;
				}
			}
		}

		if(pLevel)
			break;
	}

	if(!pLevel)
	{
		pLevel = pLevelKeys->Find(_ScriptLayer);
		if(!pLevel)
			ConOut(CStrF("Error: Scriptlayer %s not found in LevelKeys.xrg", _ScriptLayer.Str()));
		else
			RetVal = pLevel->GetThisName();
		
		spRegistry = NULL;
		return RetVal;
	}

	CRegistry *pSpawnMask = pLevel->FindChild("spawnmask");
	CRegistry *pMap = pLevelKeys->GetChild(iStoreMapID);
	if(!pSpawnMask || !pMap)
	{
		ConOut(CStrF("Error: Missing map or spawnmap for scriptlayer %s in in LevelKeys.xrg", _ScriptLayer.Str()));
		spRegistry = NULL;
		return "";
	}

	// Launch map
	RetVal = pMap->GetThisName() + ";" + pSpawnMask->GetThisValue();	
	spRegistry = NULL;

	return RetVal;
}
//-----------------------------------------------------------------------
void CGameContextMod::Con_ScriptLayer(CStr _ScriptLayer)
{
	CStr SLInfo = GetScriptLayerInfo(_ScriptLayer);
	CStr MapName = SLInfo.GetStrSep(";");
	CStr SpawnMask = SLInfo.GetStrSep(";");

	if(MapName == "")
		ConOut(CStrF("Error: Missing map for scriptlayer %s in in LevelKeys.xrg", _ScriptLayer.Str()));

	// Launch map
	if(SpawnMask != "")
	{
		SpawnMask += "+once";
		Con_SetGameKey("DEFAULTSPAWNFLAGS", SpawnMask);
	}
	Con_CampaignMap(MapName);
}

/*
void CGameContextMod::Con_InitCampaign(int _Mode)
{
	Con_SetGameClass("campaign");
	
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(!pSys)
		return;

	CStr Quick = pSys->GetEnvironment()->GetValue("QUICKCHALLENGE");
	if(Quick != "")
	{
		CStr Map = Quick.GetStrMSep(",; ");
		Con_SetGameKey("current_campaign", "Quick");
		Con_SetGameKey("quick_challenge", Quick);
		ConExecute(CStr("changemap(\"%s\",0)", Map.Str()));
		ConExecute("deferredscript(\"showbootscreen(0)\")");
	}
	else
	{
		Con_SetGameKey("current_campaign", "");
		Command_ChangeMap("campaign", "0");
		m_PendingWindow = _Mode;
	}
}*/

void CGameContextMod::UpdateControllerStatus()
{
#if 0

//#ifdef PLATFORM_CONSOLE
	// -------------------------------------------------------------------
	// Pad AI 3.0
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys)
		return;
    
	CInputContext* pInput = pSys->m_spInput;
	if (!pInput)
		return;

	CWorld_Client *pC = GetCurrentClient();
	CWClient_Mod *pClientMod = NULL;

	bool bClientInteractive = false;
	bool GotPad = false;
	bool Ingame = false;
	bool CanShowReconnect = true;
	bool ShowInstant = false;
	bool IsLoading = false;
	bool ShowingCube = false;

	if(pC)
	{
		pClientMod = TDynamicCast<CWClient_Mod>(pC);

		bClientInteractive = pC->GetInteractiveState() != 0;

		/*
		if(pClientMod)
			if(pClientMod->m_spWnd != NULL)
				ShowingCube = true;
				*/
	}
		
	if(m_spFrontEnd != NULL)
	{
		CWFrontEnd_Mod *pFrontEnd = ((CWFrontEnd_Mod *)((CWFrontEnd *)m_spFrontEnd));
		if(pFrontEnd != NULL)
			if(pFrontEnd->m_CubeReferneces)
				ShowingCube = true;
	}


	if((pC && pC->GetClientState() != WCLIENT_STATE_INGAME) || (m_spWServer != NULL && m_spWServer->IsLoading()))
		IsLoading = true;
	else
		IsLoading = false;


	if(IsLoading)
	{
		if(m_spFrontEnd != NULL && m_spFrontEnd->m_spDesktop != NULL && m_spFrontEnd->m_spDesktop->m_BlocksLoadingScreen)
			ShowInstant = true; // special case
		else
			CanShowReconnect = false;
	}

	if(m_spFrontEnd != NULL && m_spFrontEnd->m_spDesktop != NULL)
	{
		ShowingCube = true;
		if(m_spFrontEnd->m_spDesktop->m_Interactive == 0)
			CanShowReconnect = false;
	}

	if(m_LockedPad == -1)
		CanShowReconnect = false;

	if(pClientMod && pClientMod->IsCutscene() && !ShowingCube && !bClientInteractive)
		CanShowReconnect = false; // REAL cutscene

	if((m_spFrontEnd != NULL && m_spFrontEnd->m_spDesktop != NULL) || !bClientInteractive)
		Ingame = false;
	else
		Ingame = true;

	if(IsLoading)
		CanShowReconnect = false;
	
	if(Ingame && m_LockedPad == -1)
	{
		for(int i = 0; i < INPUT_MAXGAMEPADS; i++)
		{
			if (pInput->IsGamepadActive(i))
			{
				SetLockedPort(i);
				break;
			}
		}
	}

	if(m_LockedPad != -1)
	{
		if(pInput->IsGamepadValid(m_LockedPad))
			GotPad = true;

		//if(!bClientInteractive && pC/* && !ShowingCube*/)
		//	CanShowReconnect = false;

		// Disable input from all pads.
		for(int i = 0; i < INPUT_MAXGAMEPADS; i++)
			pInput->SetGamepadMapping(i, -1);

		pInput->SetGamepadMapping(m_LockedPad, 0);

#ifdef WADDITIONALCAMERACONTROL
		// Control camera from other controller
		for (int32 i = 0; i < INPUT_MAXGAMEPADS; i++)
		{
			if (m_LockedPad != i && pInput->IsGamepadActive(i))
			{
				pInput->SetGamepadMapping(i, 1);
				break;
			}
		}
#endif
	}
	else
	{
		CanShowReconnect = false;

		for(int32 i = 0; i < INPUT_MAXGAMEPADS; i++)
			pInput->SetGamepadMapping(i, 0);
	}

	if( GotPad || !CanShowReconnect )
		m_ReconnectTimer = CMTime::GetCPU()+CMTime::CreateFromSeconds(0.2f);
	else
		GotPad = GotPad;

	//if(bClientInteractive)
	//{
	if(GotPad)
		m_Pad_ShowReconnect = false;
	else
	{
		if(CanShowReconnect && (m_ReconnectTimer-CMTime::GetCPU()).GetTime() < 0)
		{
			CWFrontEnd_Mod *pFrontEndMod = TDynamicCast<CWFrontEnd_Mod>(m_spFrontEnd.p);
			if(pFrontEndMod && pFrontEndMod->CurrentMenu() != "error_reconnect" &&
					pFrontEndMod->CurrentMenu() != "error_reconnect_insta")
			{
				m_ReconnectTimer = CMTime::GetCPU()+CMTime::CreateFromSeconds(0.5f);
				if(ShowingCube)
				{
					if(ShowInstant)
						ConExecute("deferredscript (\"cg_submenu (\\\"error_reconnect_insta\\\")\")");
					else
						ConExecute("deferredscript (\"cg_submenu (\\\"error_reconnect\\\")\")");
				}
				else
				{
					if(ShowInstant)
						ConExecute("deferredscriptgrabscreen (\"cg_submenu (\\\"error_reconnect_insta\\\")\")");
					else
						ConExecute("deferredscriptgrabscreen (\"cg_submenu (\\\"error_reconnect\\\")\")");
				}
			}
			m_Pad_ShowReconnect = true;
		}
	}

	//}
#endif
}

void CGameContextMod::SetLockedPort(int32 _Port)
{
	int32 LastLockedPad = m_LockedPad;
	m_LockedPad = _Port;
#if defined(PLATFORM_XBOX1) && !defined(M_DEMO_XBOX)
	m_ExtraContent_Live.m_Pad = _Port;
#endif

#if defined(PLATFORM_XENON)
	//TODO
	//change to locked controller
	CWGameLiveHandler *pLive = (CWGameLiveHandler*) m_pMPHandler;
	pLive->m_iPad = 0;//_Port;
#endif

	// Only update registry if anything has changed (memoryfragmentation bullshit)
	if(LastLockedPad != _Port)
	{
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		CRegistry *pSt = pSys->GetRegistry()->FindChild("STRINGTABLES");
		CRegistry *pDynamicST = pSt ? pSt->FindChild("DYNAMIC") : NULL;
		if(pDynamicST)
			pDynamicST->SetValue("GAME_LOCKEDPORT", CStrF("%d", _Port+1).GetStr());
	}
}

void CGameContextMod::Refresh(class CXR_VBManager* _pVBM)
{
	MSCOPESHORT( CGameContextMod::Refresh );
	CGameContext::Refresh(_pVBM);

#if defined(PLATFORM_XBOX1) && !defined(M_DEMO_XBOX)
	if(m_SilentLogonRequested && m_spAsyncSaveContext == NULL)
	{
		m_ExtraContent_Live.SilentLogin();
		m_SilentLogonRequested = false;
	}

	m_ExtraContent_Live.Refresh();
	m_FriendsHandler.Update();
#endif

#if defined(PLATFORM_CONSOLE)
	m_pMPHandler->Update();
#endif

	if(m_PendingWindow != 0)
	{
		CWorld_Client* pC = GetCurrentClient();		
		if(pC && pC->GetClientState() == WCLIENT_STATE_INGAME)
		{
			if(m_PendingWindow == 1)
				ConExecute("deferredscript (\"cg_rootmenu (\\\"intro\\\")\")");
			else if(m_PendingWindow == 2)
#ifdef	PLATFORM_PS2
				ConExecute("deferredscript (\"cg_rootmenu (\\\"main_ps2\\\")\")");
#else
				ConExecute("deferredscript (\"cg_rootmenu (\\\"main\\\")\")");
#endif
			m_PendingWindow = 0;
		}
	}
	
	UpdateControllerStatus();
}


void CGameContextMod::Con_noop()
{
}
void CGameContextMod::Con_unlockall()
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	pSys->GetOptions()->SetValue("GAME_EXTRACONTENTKEY", "FFFFFFFFFFFFFFFF");
}

void CGameContextMod::Con_SilentLogon()
{
#if defined(PLATFORM_XBOX1) && !defined(M_DEMO_XBOX)
	m_ExtraContent_Live.SilentLogin();
#endif
	m_SilentLogonRequested = true;
}

/*

void CGameContextMod::Con_DeleteCheckpoints()
{
//	DeleteCheckpoints();
}
*/

void CGameContextMod::Con_DeleteAllSavegames()
{
	CStr Profile = "";
	FixProfile(Profile);

	m_spAsyncSaveContext = NULL;
	m_spAsyncSaveContext = MNew1(CGameContext::CSaveContext, m_spWData);

	TArray<CStr> SaveFiles;
	EnumSaveFiles(Profile, SaveFiles);
	RemoveProfileFiller(Profile);

	for(int32 i = 0; i < SaveFiles.Len(); i++)
	{
		if(SaveFiles[i][0] != '_') // skip "meta" data
			DeleteSaveFile(Profile, SaveFiles[i]);
	}

	FillProfile(Profile);
	//m_lFillProfiles.Add(Profile);
}

void CGameContextMod::Con_DeleteProfile()
{
	m_spAsyncSaveContext = NULL; // Block
	DeleteProfile("");
}

void CGameContextMod::Con_SaveProfile()
{
	CStr Profile;
	FixProfile(Profile);
	m_SaveProfile = Profile;
}

#if defined(PLATFORM_XBOX1) && !defined(M_DEMO_XBOX)
void CGameContextMod::Con_ConfirmLaunchDashboard(CStr _What)
{
	CWFrontEnd_Mod *pFrontEndMod = TDynamicCast<CWFrontEnd_Mod>(m_spFrontEnd.p);
	if(pFrontEndMod)
	{
		CWorld_Client *pC = GetCurrentClient();
		if(pC)
		{
			pFrontEndMod->Chooser_Init("§LMENU_LOSE_UNSAVEDPROGRESS");
			pFrontEndMod->Chooser_AddChoice("§LMENU_NO", "cg_prevmenu");
			pFrontEndMod->Chooser_AddChoice("§LMENU_YES", "launchdashboard "+_What);
			pFrontEndMod->Chooser_Invoke();
		}
		else
			ConExecute("launchdashboard (\""+_What+"\")");
	}
}
#endif


void CGameContextMod::UpdateViewableContent()
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	m_lViewableContent.Clear();
//	int32 count = 0;
	CExtraContentKey Key;
	Key.Parse(pSys->GetOptions()->GetValue("GAME_EXTRACONTENTKEY"));
	for(int32 i = 0; i < m_ExtraContent.m_lspContent.Len(); i++)
	{
		if(Key.IsActive(m_ExtraContent.m_lspContent[i]->m_Key))
		{
			CItem Item;
			Item.m_Index = i;
			Item.m_spContent = m_ExtraContent.m_lspContent[i];
			
			int32 c = 0;
			for(; c < m_lViewableContent.Len(); c++)
			{
				if(m_lViewableContent[c].m_spContent->m_Name.Compare(Item.m_spContent->m_Name) > 0)
					break;
			}

			m_lViewableContent.Insert(c, Item);
		}
	}
}

void CGameContextMod::Con_NextContent()
{
	//CWFrontEnd_Mod *pFrontEnd = ((CWFrontEnd_Mod *)((CWFrontEnd *)m_spFrontEnd));

	if(m_iCurrentContent < m_lViewableContent.Len()-1)
	{
		//ConExecute("cg_playsound (\"GUI_Select_01\")");
		//ConExecute("cg_playsound (\"gui_m_open\")");
		m_iCurrentContent++;
		//ConExecute("cg_switchmenu (\"extra_content_view\")");
	}
}

void CGameContextMod::Con_PrevContent()
{
	//CWFrontEnd_Mod *pFrontEnd = ((CWFrontEnd_Mod *)((CWFrontEnd *)m_spFrontEnd));

	if(m_iCurrentContent > 0)
	{
		//ConExecute("cg_playsound (\"GUI_Select_01\")");
		//ConExecute("cg_playsound (\"gui_m_open\")");
		m_iCurrentContent--;
		//ConExecute("cg_switchmenu (\"extra_content_view\")");
	}
}

#if defined(PLATFORM_XBOX1) && !defined(M_DEMO_XBOX)
void CGameContextMod::Con_Logout()
{
	m_ExtraContent_Live.Logout();
	m_FriendsHandler.Shutdown();
}
#endif

#if defined(PLATFORM_CONSOLE)
void CGameContextMod::Con_mpSearchForSessions()
{
	m_pMPHandler->SearchForSessions();
}

void CGameContextMod::Con_mpCreateSession()
{
	m_pMPHandler->CreateSession();
}

void CGameContextMod::Con_mpStartGame()
{
	m_pMPHandler->StartGame();
}

void CGameContextMod::Con_mpShowFriends(void)
{
	m_pMPHandler->ShowFriends();
}

void CGameContextMod::Con_mpShowLeaderboard(int _board)
{
	switch(_board) 
	{
	case 0:
		m_pMPHandler->ReadLeaderBoard(STATS_VIEW_SKILL_STANDARD_FREE_FOR_ALL);
		break;
	case 1:
		m_pMPHandler->ReadLeaderBoard(STATS_VIEW_SKILL_STANDARD_TEAM_GAMES);
		break;
	case 2:
		m_pMPHandler->ReadLeaderBoard(STATS_VIEW_SKILL_RANKED_FREE_FOR_ALL);
		break;
	case 3:
		m_pMPHandler->ReadLeaderBoard(STATS_VIEW_SKILL_RANKED_TEAM_GAMES);
		break;
	case 4:
		m_pMPHandler->ReadLeaderBoard(STATS_VIEW_DEATHMATCH_SHAPESHIFTER);
		break;
	case 5:
		m_pMPHandler->ReadLeaderBoard(STATS_VIEW_TEAMDEATHMATCH_SHAPESHIFTER);
		break;
	case 6:
		m_pMPHandler->ReadLeaderBoard(STATS_VIEW_CAPTURETHEFLAG_SHAPESHIFTER);
		break;
	case 7:
		m_pMPHandler->ReadLeaderBoard(STATS_VIEW_DEATHMATCH_DARKLINGS);
		break;
	case 8:
		m_pMPHandler->ReadLeaderBoard(STATS_VIEW_TEAMDEATHMATCH_DARKLINGS);
		break;
	case 9:
		m_pMPHandler->ReadLeaderBoard(STATS_VIEW_CAPTURETHEFLAG_DARKLINGS);
		break;
	case 10:
		m_pMPHandler->ReadLeaderBoard(STATS_VIEW_SURVIVOR);
		break;
	case 11:
		m_pMPHandler->ReadLeaderBoard(STATS_VIEW_LASTHUMAN);
		break;
	case 12:
		m_pMPHandler->ReadLeaderBoard(STATS_VIEW_CAPTURETHEFLAG_DVH);
		break;
	}
}

void CGameContextMod::Con_mpLan(int _lan)
{
	if(_lan)
	{
		if(m_pMPHandler->InitializeLAN())
			ConExecute(CStr("cg_submenu(\"Multiplayer_Link\")"));
	}
	else
	{
		if(m_pMPHandler->CheckLoggedInAndMultiplayerRights())
		{
			m_pMPHandler->m_Flags &= ~MP_FLAGS_LANGAME;
		}
	}
}

void CGameContextMod::Con_mpRanked(int _ranked)
{
#ifdef PLATFORM_XENON
	if(_ranked)
		m_pMPHandler->m_GameType = X_CONTEXT_GAME_TYPE_RANKED;
	else
		m_pMPHandler->m_GameType = X_CONTEXT_GAME_TYPE_STANDARD;
#endif
}

void CGameContextMod::Con_mpClearLeaderboard(void)
{
	m_pMPHandler->CleanupStats();
}

void CGameContextMod::Con_mpReady(void)
{
	m_pMPHandler->ToggleReadyStatus();
}

void CGameContextMod::Con_mpQuickgame(void)
{
	m_pMPHandler->Quickgame();
}

void CGameContextMod::Con_mpCustomgame(uint8 _CreateSession)
{
	m_pMPHandler->Customgame(_CreateSession);
}

void CGameContextMod::Con_mpBroadcast(void)
{
	m_pMPHandler->Broadcast();
}

void CGameContextMod::Con_mpShutdownLAN(void)
{
	m_pMPHandler->ShutdownLAN();
}

#endif

void CGameContextMod::Con_mpClearAchievements(void)
{
#ifdef PLATFORM_XENON
	CWGameLiveHandler *pLive = (CWGameLiveHandler*) m_pMPHandler;
	pLive->CleanupSearchAndRegResults();
#endif
}

void CGameContextMod::Con_mpShutdownSession(uint32 _no_report)
{
#if defined(PLATFORM_CONSOLE)
	m_pMPHandler->ShutdownSession(_no_report ? 1 : 0);
#endif
}

void CGameContextMod::Con_mpShutdown(void)
{
#if defined(PLATFORM_CONSOLE)
	m_pMPHandler->Shutdown();
#endif
}

void CGameContextMod::Con_mpInit(void)
{
#if defined(PLATFORM_CONSOLE)
	if(!m_pMPHandler->Initialize())
		m_pMPHandler->Shutdown();
#endif
}

void CGameContextMod::Con_PadLock()
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys && pSys->m_spInput != NULL)
		for(int i = 0; i < INPUT_MAXGAMEPADS; i++)
		{
			if(pSys->m_spInput->IsGamepadActive(i))
			{
				SetLockedPort(i);
				break;
			}
		}
}

void CGameContextMod::Con_RemovePadLock()
{
	SetLockedPort(-1);
}

void CGameContextMod::Con_HintLoad()
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys && pSys->m_spInput != NULL)
	{
		pSys->m_spInput->FlushEnvelopes();
		pSys->m_spInput->SetFeedbackAmount(0);
	}

	m_LoadHinted = true;
}
/*
void CGameContextMod::Con_SetGameClass(CStr _s)
{
	m_spGameReg->SetValue("DEFAULTGAME", _s);
#if defined(PLATFORM_XENON)
	
	if(!_s.Compare("DM"))
		m_pMPHandler->m_GameMode = CONTEXT_GAME_MODE_DARKLINGS_DM;
	else if(!_s.Compare("TDM"))
		m_pMPHandler->m_GameMode = CONTEXT_GAME_MODE_DARKLINGS_TDM;
	else if(!_s.Compare("CTF"))
		m_pMPHandler->m_GameMode = CONTEXT_GAME_MODE_DARKLINGS_CTF;
#endif
}*/

void CGameContextMod::Con_mpSetGameMode(CStr _s)
{
	m_spGameReg->SetValue("DEFAULTGAMEMODE", _s);
#if defined(PLATFORM_XENON)
	if(!_s.Compare("0"))
		m_pMPHandler->m_GameStyle = CONTEXT_GAME_STYLE_SHAPESHIFTER;
	else if(!_s.Compare("1"))
		m_pMPHandler->m_GameStyle = CONTEXT_GAME_STYLE_DARKLINGS_VS_DARKLINGS;
	else if(!_s.Compare("2"))
		m_pMPHandler->m_GameStyle = CONTEXT_GAME_STYLE_DARKLINGS_VS_HUMANS;
#endif
}

void CGameContextMod::Con_SetDifficulty(CStr _What)
{
	int32 Diff = -1;
	if(_What == "easy")
		Diff = 1;
	else if(_What == "normal")
		Diff = 2;
	else if(_What == "hard")
		Diff = 3;
	else if(_What == "commentary")
		Diff = 4;

	if(Diff != -1)
	{
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if(pSys && pSys->GetOptions())
			pSys->GetOptions()->SetValuei("GAME_DIFFICULTY", Diff);
		ConExecute("saveprofile()");
	}
}	

void CGameContextMod::Register(CScriptRegisterContext & _RegContext)
{
	CGameContext::Register(_RegContext);

	_RegContext.RegFunction("noop", this, &CGameContextMod::Con_noop);
	_RegContext.RegFunction("saveprofile", this, &CGameContextMod::Con_SaveProfile);
	_RegContext.RegFunction("padlock", this, &CGameContextMod::Con_PadLock);
	_RegContext.RegFunction("removepadlock", this, &CGameContextMod::Con_RemovePadLock);

	_RegContext.RegFunction("hintload", this, &CGameContextMod::Con_HintLoad);

	_RegContext.RegFunction("nextcontent", this, &CGameContextMod::Con_NextContent);
	_RegContext.RegFunction("prevcontent", this, &CGameContextMod::Con_PrevContent);
	_RegContext.RegFunction("deleteprofile", this, &CGameContextMod::Con_DeleteProfile);
	_RegContext.RegFunction("deleteallsavegames", this, &CGameContextMod::Con_DeleteAllSavegames);
	_RegContext.RegFunction("unlockall", this, &CGameContextMod::Con_unlockall);

	
	_RegContext.RegFunction("setdifficulty", this, &CGameContextMod::Con_SetDifficulty);

	_RegContext.RegFunction("silentlogon", this, &CGameContextMod::Con_SilentLogon);
#if defined(PLATFORM_XBOX1) && !defined(M_DEMO_XBOX)
	_RegContext.RegFunction("logout", this, &CGameContextMod::Con_Logout);
	_RegContext.RegFunction("confirmlaunchdashboard", this, &CGameContextMod::Con_ConfirmLaunchDashboard);
#endif
#if defined(PLATFORM_CONSOLE)
	_RegContext.RegFunction("mp_searchforsessions", this, &CGameContextMod::Con_mpSearchForSessions);
	_RegContext.RegFunction("mp_createsession", this, &CGameContextMod::Con_mpCreateSession);
	_RegContext.RegFunction("mp_startgame", this, &CGameContextMod::Con_mpStartGame);
	_RegContext.RegFunction("mp_showfriends", this, &CGameContextMod::Con_mpShowFriends);
	_RegContext.RegFunction("mp_showleaderboard", this, &CGameContextMod::Con_mpShowLeaderboard);
	_RegContext.RegFunction("mp_clearleaderboard", this, &CGameContextMod::Con_mpClearLeaderboard);
	_RegContext.RegFunction("mp_ready", this, &CGameContextMod::Con_mpReady);
	_RegContext.RegFunction("mp_quickgame", this, &CGameContextMod::Con_mpQuickgame);
	_RegContext.RegFunction("mp_customgame", this, &CGameContextMod::Con_mpCustomgame);
	_RegContext.RegFunction("mp_lan", this, &CGameContextMod::Con_mpLan);
	_RegContext.RegFunction("mp_ranked", this, &CGameContextMod::Con_mpRanked);
	_RegContext.RegFunction("mp_broadcast", this, &CGameContextMod::Con_mpBroadcast);
	_RegContext.RegFunction("mp_shutdownlan", this, &CGameContextMod::Con_mpShutdownLAN);
#endif
	_RegContext.RegFunction("mp_clearachievements", this, &CGameContextMod::Con_mpClearAchievements);
	_RegContext.RegFunction("mp_shutdownsession", this, &CGameContextMod::Con_mpShutdownSession);
	_RegContext.RegFunction("mp_shutdown", this, &CGameContextMod::Con_mpShutdown);
	_RegContext.RegFunction("mp_init", this, &CGameContextMod::Con_mpInit);
	_RegContext.RegFunction("mp_setgamemode", this, &CGameContextMod::Con_mpSetGameMode);

	_RegContext.RegFunction("campaignmap", this, &CGameContextMod::Con_CampaignMap);
	_RegContext.RegFunction("scriptlayer", this, &CGameContextMod::Con_ScriptLayer);
}

