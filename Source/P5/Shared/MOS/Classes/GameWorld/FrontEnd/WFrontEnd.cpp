
#include "PCH.h"
#include "WFrontEnd.h"
#include "../../GameContext/WGameContext.h"

MRTC_IMPLEMENT(CWFrontEnd, CConsoleClient);


CWFrontEnd::CWFrontEnd()
{
	m_iChannel = -1;
	m_PausedGame = false;
	m_pSystem = NULL;
	m_pGameContext = NULL;
}

CWFrontEnd::~CWFrontEnd()
{
	//M_TRY
	//{
		RemoveFromConsole();
	/*}
	M_CATCH(
	catch(CCException)
	{
	}
	)*/
}

void CWFrontEnd::Create(CWF_CreateInfo& _CreateInfo)
{
	m_pGameContext = _CreateInfo.m_pGameContext;
	m_spNetwork = _CreateInfo.m_spNetwork;
	m_NewWindow = "$$$";
	m_iCurrentWindow = -1;
	m_Switching = false;
	m_GameIsLoading = false;
	m_spWData = m_pGameContext->m_spWData;
	SetSoundContext(_CreateInfo.m_spSoundContext);
	m_spGameReg = m_spWData->GetGameReg();
	if (!m_spGameReg) Error("Create", "No game-reg.");

	m_spMapData = MNew(CMapData);
	if (!m_spMapData) MemError("Create");

	m_spMapData->Create(m_spWData);

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) Error("Create", "Could not get system-object.");
	m_pSystem = pSys;

#ifdef PLATFORM_XBOX
	if(m_pSystem && m_pSystem->m_spInput)
		m_pSystem->m_spInput->SetShowCursor(false);
#endif

	AddToConsole();
}


void CWFrontEnd::ClearMenus() 
{ 
//	M_TRACEALWAYS("CWFrontEnd::ClearMenus()\n");
	m_SwitchMode = EMODE_CLEARALL; 
}
void CWFrontEnd::SwitchMenu(CStr _Name) 
{ 
//	M_TRACEALWAYS("CWFrontEnd::SwitchMenu(%s)\n", _Name.Str());
	m_NewWindow = _Name; m_SwitchMode = EMODE_SWITCH; 
}
void CWFrontEnd::SubMenu(CStr _Name) 
{ 
//	M_TRACEALWAYS("CWFrontEnd::SubMenu(%s)\n", _Name.Str());
	m_NewWindow = _Name; m_SwitchMode = EMODE_SUBMENU; 
}

void CWFrontEnd::PrevMenu(int32 _Steps) 
{ 
//	M_TRACEALWAYS("CWFrontEnd::PrevMenu(%d)\n", _Steps);
	m_SwitchMode = EMODE_PREV; m_Steps = _Steps; 
}
void CWFrontEnd::RootMenu(CStr _Name) 
{ 
//	M_TRACEALWAYS("CWFrontEnd::RootMenu(%s)\n", _Name.Str());
	m_NewWindow = _Name; m_SwitchMode = EMODE_ROOT; 
}


CStr CWFrontEnd::CurrentMenu()
{
	if(m_iCurrentWindow < 0)
		return "";
	return m_aContextStack[m_iCurrentWindow];
}



CMWnd* CWFrontEnd::GetDesktop()
{
	return m_spDesktop;
}

int CWFrontEnd::GetRenderMode()
{
	return 0;
}

void CWFrontEnd::SetWindow(CStr _ID)
{
}


bool CWFrontEnd::CanDoAttract()
{
	if(m_iCurrentWindow == -1)
		return false;

	for(int32 i = 0; i <= m_iCurrentWindow; i++)
		if(!m_aWindows[m_iCurrentWindow]->m_AllowsAttractMode)
			return false;
	
	return true;
}


void CWFrontEnd::DoWindowSwitch()
{
	M_LOCK(m_WindowSwitchLock);
	bool Switched = m_SwitchMode != EMODE_NONE;

	while(m_SwitchMode != EMODE_NONE)
	{
		bool ShouldSetWindow = true;
		m_Switching = true;
		int32 Mode = m_SwitchMode;
		CStr Window = m_NewWindow;
		CStr WindowToSet = "";
		m_NewWindow = "$$$";
		m_SwitchMode = EMODE_NONE;
		
		// 
		if(Mode == EMODE_CLEARALL) WindowToSet = "";
		else if(Mode == EMODE_SWITCH) 
		{
			WindowToSet = Window;
		}
		else if(Mode == EMODE_SUBMENU) 
		{
			WindowToSet = Window;
			ConExecute("cg_playsound (\"GUI_Select_01\")");
		}
		else if(Mode == EMODE_ROOT) 
		{
			WindowToSet = Window;
		}
		else if(Mode == EMODE_PREV)
		{	
			ConExecute("cg_playsound (\"GUI_Select_01\")");
			if(m_iCurrentWindow-m_Steps < 0)
				WindowToSet = "";
			else
				WindowToSet = m_aContextStack[m_iCurrentWindow-m_Steps];
			ShouldSetWindow = false;
		}

		// switch window
		if(ShouldSetWindow)
			SetWindow(WindowToSet);

		//
		if(WindowToSet.Len() && m_spDesktop == NULL)
		{
			// failure.. switch back ffs!
			if(m_iCurrentWindow >= 0)
				SetWindow(m_aContextStack[m_iCurrentWindow]);
			M_TRACEALWAYS("DoWindowSwitch():failure to set window\n");
		}
		else
		{
			int iOldCurrentWindow = m_iCurrentWindow;
			// success, adjust the stack
			if(Mode == EMODE_CLEARALL)
				m_iCurrentWindow = -1;
			else if(Mode == EMODE_SWITCH)
			{
			}
			else if(Mode == EMODE_SUBMENU)
			{
				m_iCurrentWindow++;
				if(m_iCurrentWindow >= EWINDOWSTACKSIZE)
					M_TRACEALWAYS("DoWindowSwitch():Stack overflow\n");
			}
			else if(Mode == EMODE_ROOT)
				m_iCurrentWindow = 0;
			else if(Mode == EMODE_PREV)
			{
				m_iCurrentWindow-=m_Steps;
				if(m_iCurrentWindow < 0)
					m_iCurrentWindow = -1;
			}

			// Clear the frontend's contextstack
			CStr EmptyStr;
			for( int i = m_iCurrentWindow + 1; i <= iOldCurrentWindow; i++ )
			{
				if( i < 0 ) continue;
				m_aContextStack[i] = EmptyStr;
			}

			// apply new value
			if(m_iCurrentWindow >= 0)
			{
				m_aContextStack[m_iCurrentWindow] = WindowToSet;
				if(ShouldSetWindow)
					m_aWindows[m_iCurrentWindow] = m_spDesktop;
				else
					m_spDesktop = m_aWindows[m_iCurrentWindow];
			}

		}

		if(m_iCurrentWindow < -1)
			m_iCurrentWindow = -1;
		for(int32 i = m_iCurrentWindow+1; i < EWINDOWSTACKSIZE; i++)
			m_aWindows[i] = NULL;

		if(m_iCurrentWindow < 0)
			m_spDesktop = NULL;
		else
			m_spDesktop = m_aWindows[m_iCurrentWindow];

		if(m_spDesktop != NULL)
			m_spDesktop->OnActivate();

		m_Switching = false;
	}

	if(Switched)
	{
//		CWorld_Client *pClient = NULL;
		if(m_pGameContext && m_pGameContext->m_lspWClients.Len())
			ConExecute("releaseall()");

		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if (pSys && pSys->GetEnvironment()->GetValuei("SPAMUISTACK", 0))
		{
			M_TRACEALWAYS("--Current Stack:\n");
			for(int32 i = 0; i <= m_iCurrentWindow; i++)
				M_TRACEALWAYS("\t%s\t%d\n", m_aContextStack[i].Str(), m_aWindows[i]->m_AllowsAttractMode);
		}
	}

	// pause/unpause here as well as in cube.cpp, since this also handles the debug menu
	if(m_spDesktop != NULL)
	{
		// disable vibration
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if (pSys && pSys->m_spInput != NULL)
			pSys->m_spInput->FlushEnvelopes();

		if(!m_PausedGame)
		{
			M_TRACEALWAYS("++ Paused Game\n");
			ConExecute("pause(1)");
			m_PausedGame = true;
		}
	}
	else
	{
		
		if(m_PausedGame)
		{
			M_TRACEALWAYS("-- Unpaused Game\n");
			ConExecute("pause(0)");
			m_PausedGame = false;
		}
	}

	/*
	if (m_NewWindow != "$$$")
	{
		// SetWindow can change the m_NewWindow so we need to keep
		// setting the window until it settles
		CStr Window = "$$$";
		while(Window != m_NewWindow)
		{
			Window = m_NewWindow;
			int32 iMode = 
			SetWindow(m_NewWindow);
		}
		m_CurrentWindow = m_NewWindow;
		m_NewWindow = "$$$";
	}*/
}

bool CWFrontEnd::ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
{
//	DoWindowSwitch();

	spCMWnd spDesktop = m_spDesktop;
	if (spDesktop != NULL)
	{
		bool ret = spDesktop->ProcessKey(_Key, _OriginalKey) != 0;
//		DoWindowSwitch();
		return ret;
	}

//	DoWindowSwitch();

	return false;
}

void CWFrontEnd::OnRefresh()
{
	MSCOPE(CWFrontEnd::OnRefresh, FRONTEND);
	M_LOCK(m_FrontEndLock);
	DoWindowSwitch();

	spCMWnd spDesktop = m_spDesktop;
	if (spDesktop != NULL)
		spDesktop->OnRefresh();
}

void CWFrontEnd::OnRender(CRenderContext* _pRC, CXR_VBManager* _pVBM)
{
	MSCOPESHORT(CWFrontEnd::OnRender);
//	DoWindowSwitch();

	spCMWnd spDesktop = m_spDesktop; // keep ref
	if (!spDesktop)
		return;

	M_LOCK(m_FrontEndLock);

	CRC_Viewport* pVP = _pVBM->Viewport_Get();

//	if (m_pSystem->m_spInput!=NULL)
//		m_pSystem->m_spInput->SetMouseArea(pVP->GetViewRect());

	{
		CRC_Util2D Util2D;
		Util2D.Begin(_pRC, pVP, _pVBM);
		Util2D.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE);

		// Constantly clear the VBM, so we don't have to have to have so much memory allocated
//		Util2D.SetAutoFlush(true);

		CRct Rect = pVP->GetViewRect();
		CClipRect Clip(0,0,640,480);

		fp32 aspect = ((fp32)Rect.p1.x / (fp32)Rect.p1.y);
		CVec2Dfp32 scale(Rect.GetHeight() / 480.0f, Rect.GetHeight() / 480.0f);
		if(aspect > 1.4f)
		{
			//int p1 = (int)(Clip.clip.p1.x * (1.0f + (Rect.GetWidth() / 640.0f - Rect.GetHeight() / 480.0f)));
			int p1 = TruncToInt(Clip.clip.p1.x * scale.k[0]);
			Clip.ofs.x = TruncToInt(((Rect.p1.x - p1) / 2) / scale.k[0]);
			Clip.clip.p0.x = TruncToInt(((Rect.p1.x - p1) / 2) / scale.k[0]);
			Clip.clip.p1.x += TruncToInt(((Rect.p1.x - p1) / 2) / scale.k[0]);
		}

		Util2D.SetCoordinateScale(scale);

//		CClipRect Clip(pVP->GetViewClip());
// ConOut(CStrF("Rect %d, %d", Rect.GetWidth(), Rect.GetHeight()));
// ConOut(CStrF("Clip %d, %d, %d, %d, %d, %d", Clip.clip.p0.x, Clip.clip.p0.y, Clip.clip.p1.x, Clip.clip.p1.y, Clip.ofs.x, Clip.ofs.y ));

		CPnt MousePos(0,0);
		if (m_pSystem->m_spInput) MousePos = m_pSystem->m_spInput->GetMousePosition();
		if (spDesktop != NULL)
		{
			TPtr<CMWnd> spWndMouseOver = spDesktop->GetCursorOverWnd(MousePos, false);
			int OldStatus = 0;
			if (spWndMouseOver != NULL)
			{
				OldStatus = spWndMouseOver->m_Status;
				spWndMouseOver->m_Status |= WSTATUS_MOUSEOVER;
			}

//LogFile("(CWFrontEnd_OnRender) Render tree...");
			spDesktop->RenderTree(&Util2D, Clip);
//LogFile("(CWFrontEnd_OnRender) Render tree returned...");

			if (spWndMouseOver != NULL) 
//				spWndMouseOver->m_Status = OldStatus;
//				Changed to this instead to prvent loss of status-change during RenderTree.
//				Please tell me if something got f**ked up /MG
			{

				spWndMouseOver->m_Status &= ~WSTATUS_MOUSEOVER;	
			}

			scale.k[0] = Rect.GetWidth() / 640.0f;

			_pVBM->ScopeBegin("OnPostRenderInterface", false);
			OnPostRenderInterface(&Util2D, Rect, &scale);
			_pVBM->ScopeEnd();

		}
	
#ifndef GUI_PC // PC PORT: added as part of new PC GUI functionality
#ifndef PLATFORM_CONSOLE
/*		if(m_pSystem->m_spInput != NULL && m_pSystem->m_spInput->GetShowCursor())
		{	
			Util2D.GetRC()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			Util2D.SetTexture(_pRC->Texture_GetTC()->GetTextureID("CURSOR"));
			Util2D.SetTextureScale(1.0f, 1.0f);
//			Util2D.SetTexture("CURSOR");
			Util2D.Sprite(Clip, MousePos);
		}
*/
#endif
#endif 
		Util2D.SetCoordinateScale(CVec2Dfp32(Rect.GetHeight() / 640.0f, Rect.GetHeight() / 480.0f));
		Util2D.End();
	}
}

void CWFrontEnd::OnPostRenderInterface(CRC_Util2D* _pRCUtil, CClipRect _Clip, CVec2Dfp32 *_pScale)
{
}

void CWFrontEnd::Con_CG_menu(CStr _Menu)
{
	ConOut("Obsolete command.");
	//m_NewWindow = _Menu;
}

void CWFrontEnd::Con_CG_switchmenu(CStr _Menu) { SwitchMenu(_Menu); }
void CWFrontEnd::Con_CG_submenu(CStr _Menu) { SubMenu(_Menu); }
void CWFrontEnd::Con_CG_clearmenus() { ClearMenus(); }
void CWFrontEnd::Con_CG_prevmenu() { PrevMenu(); }
void CWFrontEnd::Con_CG_prevmenu2() { PrevMenu(2); }
void CWFrontEnd::Con_CG_rootmenu(CStr _Menu) { RootMenu(_Menu); }
void CWFrontEnd::Con_CG_rootmenu_ingame(CStr _Menu)
{
	if(!m_GameIsLoading)
		RootMenu(_Menu);
}

void CWFrontEnd::Con_CG_dumpwnd()
{
	spCMWnd spDesktop = m_spDesktop; // keep ref
	if (spDesktop != NULL)
		spDesktop->OnDumpTree();
	else
		ConOut("No current window system.");
}

void CWFrontEnd::Con_CG_NewGame(CStr _Name)
{
	OnNewGame(_Name);
}
void CWFrontEnd::Con_CG_menuimpulse(int _iImpulse)
{
	spCMWnd spDesktop = m_spDesktop; // keep ref
	if (spDesktop)
	{
		CMWnd_Message Msg(WMSG_MENUIMPULSE, "", _iImpulse);
		spDesktop->OnMessage(&Msg);
	}
}

void CWFrontEnd::SetSoundContext(spCSoundContext _spSoundContext)
{
	if (_spSoundContext)
	{
		m_spSoundContext = _spSoundContext;
		m_iChannel = m_spSoundContext->Chn_Alloc(8);
	}
	else
	{
		if (m_iChannel >= 0)
		{
			m_spSoundContext->Chn_Free(m_iChannel);
			m_iChannel = -1;
		}
	}
}

void CWFrontEnd::SetVolume(fp32 _Volume)
{
	if (m_spSoundContext != NULL)
	{
		m_spSoundContext->Chn_SetVolume(m_iChannel, _Volume);
	}
}


void CWFrontEnd::Con_CG_PlaySound(CStr _Sound)
{
	if (m_iChannel>=0)
	{
		int _iSound = m_spMapData->GetResourceIndex_Sound(_Sound);
		
		CSC_SFXDesc* pDesc = m_spMapData->GetResource_SoundDesc(_iSound);
		
		if (!pDesc) return;
		int hVoice = 0;
		
		// Global sounds
		
		CSC_VoiceCreateParams CreateParams;
		m_spSoundContext->Voice_InitCreateParams(CreateParams, pDesc->GetCategory());
		CreateParams.m_WaveID = pDesc->GetWaveId(0);
		CreateParams.m_hChannel = m_iChannel;
		CreateParams.m_Volume *= pDesc->GetVolume() + Random*pDesc->GetVolumeRandAmp();
		CreateParams.m_Pitch *= pDesc->GetPitch() + Random*pDesc->GetPitchRandAmp();
		uint32 Prio = pDesc->GetPriority();
		if (Prio)
			CreateParams.m_Priority = Prio;

		hVoice = m_spSoundContext->Voice_Create(CreateParams);
	}
}

void CWFrontEnd::Con_CG_PlaySound_SfxVol(CStr _Sound)
{
	if (m_iChannel>=0)
	{
		int _iSound = m_spMapData->GetResourceIndex_Sound(_Sound);
		
		CSC_SFXDesc* pDesc = m_spMapData->GetResource_SoundDesc(_iSound);
		
		if (!pDesc) return;
		int hVoice = 0;

		fp32 Vol = m_pSystem->GetOptions()->GetValuef("SND_VOLUMESFX", 1.0f);
		// Global sounds
		CSC_VoiceCreateParams CreateParams;
		m_spSoundContext->Voice_InitCreateParams(CreateParams, pDesc->GetCategory());
		CreateParams.m_WaveID = pDesc->GetWaveId(0);
		CreateParams.m_hChannel = m_iChannel;
		CreateParams.m_Volume *= (pDesc->GetVolume() + Random*pDesc->GetVolumeRandAmp()) * Vol;
		CreateParams.m_Pitch *= pDesc->GetPitch() + Random*pDesc->GetPitchRandAmp();
		uint32 Prio = pDesc->GetPriority();
		if (Prio)
			CreateParams.m_Priority = Prio;

		hVoice = m_spSoundContext->Voice_Create(CreateParams);
	}
}

void CWFrontEnd::Con_CG_LoadLastValidProfile(CStr _SuccessScript, CStr _FailScript)
{
	if( m_pGameContext->LoadLastValidProfile())
	{
		ConExecute(_SuccessScript);
	}
	else
	{
		ConExecute(_FailScript);
	}
}

void CWFrontEnd::Register(CScriptRegisterContext & _RegContext)
{
	_RegContext.RegFunction("cg_switchmenu", this, &CWFrontEnd::Con_CG_switchmenu);
	_RegContext.RegFunction("cg_submenu", this, &CWFrontEnd::Con_CG_submenu);
	_RegContext.RegFunction("cg_clearmenus", this, &CWFrontEnd::Con_CG_clearmenus);
	_RegContext.RegFunction("cg_prevmenu", this, &CWFrontEnd::Con_CG_prevmenu);
	_RegContext.RegFunction("cg_prevmenu2", this, &CWFrontEnd::Con_CG_prevmenu2);
	_RegContext.RegFunction("cg_rootmenu", this, &CWFrontEnd::Con_CG_rootmenu);
	_RegContext.RegFunction("cg_rootmenu_ingame", this, &CWFrontEnd::Con_CG_rootmenu_ingame);
	_RegContext.RegFunction("cg_menuimpulse", this, &CWFrontEnd::Con_CG_menuimpulse);
	_RegContext.RegFunction("cg_dowindowswitch", this, &CWFrontEnd::DoWindowSwitch);

	_RegContext.RegFunction("cg_newgame", this, &CWFrontEnd::Con_CG_NewGame);
	_RegContext.RegFunction("cg_menu", this, &CWFrontEnd::Con_CG_menu);
	_RegContext.RegFunction("cg_dumpwnd", this, &CWFrontEnd::Con_CG_dumpwnd);

	_RegContext.RegFunction("cg_playsound", this, &CWFrontEnd::Con_CG_PlaySound);
	_RegContext.RegFunction("cg_playsound_sfxvol", this, &CWFrontEnd::Con_CG_PlaySound_SfxVol);

	_RegContext.RegFunction("cg_loadlastvalidprofile", this, &CWFrontEnd::Con_CG_LoadLastValidProfile);
}

