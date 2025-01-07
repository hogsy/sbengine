#ifndef __WFRONTEND_H
#define __WFRONTEND_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Frontend base-class

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWFrontEnd
\*____________________________________________________________________________________________*/

#include "../WMapData.h"
#include "../../Win/MWindows.h"

class CWF_CreateInfo
{
public:
	class CGameContext* m_pGameContext;
	spCNetwork m_spNetwork;
	spCSoundContext m_spSoundContext;
	CStr m_Params;
};


class CWFrontEnd : public CConsoleClient
{
	MRTC_DECLARE;

public:
	spCMWnd m_spDesktop;
	CSystem* m_pSystem;

	class CGameContext* m_pGameContext;
	spCWorldData m_spWData;
	spCMapData m_spMapData;		// Resource-mapping for client-game. 
								// Make sure not to mix up resource-IDs between client and client-game as they are NOT the same.
	spCRegistry m_spGameReg;
	spCNetwork m_spNetwork;

	// sound
	spCSoundContext m_spSoundContext;
	int m_iChannel;


	enum
	{
		EMODE_NONE=-1,
		EMODE_CLEARALL,
		EMODE_SWITCH,
		EMODE_SUBMENU,
		EMODE_ROOT,
		EMODE_PREV,

		EWINDOWSTACKSIZE=8
	};


//	CStr m_NewWindow;
	CFStr m_NewWindow;
	int32 m_SwitchMode;
	int32 m_Steps;
	MRTC_CriticalSection m_WindowSwitchLock;
	void DoWindowSwitch();

	bool m_Switching;
	bool m_PausedGame;
	bool m_GameIsLoading;


 
	spCMWnd m_aWindows[EWINDOWSTACKSIZE]; // (not used for now)
	CStr m_aContextStack[EWINDOWSTACKSIZE]; // max 10 sub menus
	int32 m_iCurrentWindow; // if -1, then no menu present

	MRTC_CriticalSection m_FrontEndLock;

	// commands for handling the context stack
	virtual void ClearMenus();
	void SwitchMenu(CStr _Name);
	void SubMenu(CStr _Name);
	void RootMenu(CStr _Name);
	void PrevMenu(int32 _Steps=1);
	CStr CurrentMenu();

	bool CanDoAttract(); // check so that it's ok to enter attract mode

	CXR_Engine* m_pEngine;


	CWFrontEnd();
	~CWFrontEnd();
	virtual void Create(CWF_CreateInfo& _CreateInfo);
	virtual CMWnd* GetDesktop();
	virtual int GetRenderMode();	// 0 == Overlay, 1 == Full screen

	virtual void SetWindow(CStr _ID);
	virtual void OnNewGame(CStr _Name) {}

	virtual	bool ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey);

	virtual void GrabScreen() pure;
	virtual void DoPrecache(class CRenderContext* _pRC, class CXR_Engine *_pEngine) pure;

	virtual CMWnd *GetCursorWindow(bool _bOnlyTabstops) pure;

	virtual void OnRefresh();
	virtual void LoadAllResources() {};
	virtual void OnPrecache(class CXR_Engine* _pEngine){};
	virtual void OnRender(class CRenderContext* _pRC, class CXR_VBManager* _pVBM);
	virtual void OnPostRenderInterface(class CRC_Util2D* _pRCUtil, CClipRect _Clip, CVec2Dfp32 *_pScale);

	virtual void Con_CG_switchmenu(CStr _Menu);
	virtual void Con_CG_submenu(CStr _Menu);
	virtual void Con_CG_clearmenus();
	virtual void Con_CG_prevmenu();
	virtual void Con_CG_prevmenu2();
	virtual void Con_CG_rootmenu(CStr _Menu);
	virtual void Con_CG_rootmenu_ingame(CStr _Menu);
	virtual void Con_CG_menuimpulse(int _iImpulse);

	virtual void Con_CG_menu(CStr _Menu);
	virtual void Con_CG_dumpwnd();
	virtual void Con_CG_NewGame(CStr _Name);

	virtual void Con_CG_LoadLastValidProfile(CStr _SuccessScript, CStr _FailScript);

	// Sound

	virtual void SetSoundContext(spCSoundContext _spSoundContext);
	virtual void SetVolume(fp32 _Volume);
	virtual void Con_CG_PlaySound(CStr _Sound);
	virtual void Con_CG_PlaySound_SfxVol(CStr _Sound);

	void Register(CScriptRegisterContext &_RegContext);

	void SetRender_Engine(CXR_Engine* _pEngine) { m_pEngine = _pEngine; }
	CXR_Engine* GetRender_Engine() { return m_pEngine; }
};

typedef TPtr<CWFrontEnd> spCWFrontEnd;


#endif // _INC_WCLIENTGAME
