#ifndef __GAMECONTEXTMOD_H
#define __GAMECONTEXTMOD_H

#include "../../Shared/MOS/Classes/GameContext/WGameContext.h"

#if defined(PLATFORM_XENON)
#include "../Exe_Xenon/WGameLiveHandler.h"
#endif
#if defined(PLATFORM_PS3)
#include "../Exe_PS3/WGamePSNetworkHandler.h"
#endif
//#include "../GameWorld/WFrontEndMod.h"


class CExtraContentKey
{
public:
	uint32 m_High;
	uint32 m_Low;

	CExtraContentKey() { m_High = m_Low = 0; }
	void Parse(CStr _Key)
	{
		m_High = ("0x" + _Key.Copy(0, 8)).Val_int();
		m_Low = ("0x" + _Key.Copy(8, 8)).Val_int();
	}

	CStr GetString()
	{
		return CStrF("%.8x%.8x", m_High, m_Low);
	}
	
	bool AddKeyBit(int _KeyID)
	{
		if(_KeyID >= 32)
		{
			if(m_High & (1 << _KeyID))
				return false;
			m_High |= 1 << _KeyID;
			return true;
		}
		else
		{
			if(m_Low & (1 << _KeyID))
				return false;
			m_Low |= 1 << _KeyID;
			return true;
		}
	}

	bool IsActive(const CExtraContentKey &_ChildKey)
	{
		if((m_High & _ChildKey.m_High) == _ChildKey.m_High &&
			(m_Low & _ChildKey.m_Low) == _ChildKey.m_Low)
			return true;
		else
			return false;
	}
};

class CExtraContentHandler
{
public:
	enum
	{
		TYPE_PICTURE,
		TYPE_VIDEO,
		TYPE_TIMELOCKLEVEL,
		TYPE_EXTRALEVEL,
		TYPE_TEXT,
		TYPE_SOUND,
		TYPE_SCRIPT,
	};

	class CContent : public CReferenceCount
	{
	public:
		CStr m_Name;
		CStr m_Desc;
		CStr m_Thumbnail;
		CStr m_ID;
		int32 m_Type;
		int32 m_Flags;
		fp32 m_Param;
		bool m_Installed;

		CExtraContentKey m_Key;
	};

	//
	// Rules:
	//	- Content provider must not remove content form the m_lContent
	// 
	//
	class CContentProvider
	{
	public:
		CExtraContentHandler *m_pHandler;

		virtual void UpdateContentList() pure;		// initates a content update
		virtual bool IsContentUpdateDone() pure;	// returns true when the list is compiled
	};

	CExtraContentHandler();
	bool m_UpdatingContent;
	void Update();			// call this
	bool IsUpdateDone();	// then wait for this to return true

	dllvirtual bool AddKeyBit(int _KeyID, TArray<CContent *> *_lpContent = NULL); // Used to register new key-part, and get new content that was unlocked

	TArray<CContentProvider*> m_lProviders;
	TArray<TPtr<CContent> > m_lspContent;
};

class CExtraContent_Offline : public CExtraContentHandler::CContentProvider
{
	virtual void UpdateContentList();	// initates a content update
	virtual bool IsContentUpdateDone();	// returns true when the list is compiled
};

class CGameContextMod : public CGameContext
{
public:
	enum
	{
		CGCMOD_COMMAND_LAST = CGC_COMMAND_LAST,
	};
#if defined(PLATFORM_XBOX1) && !defined(M_DEMO_XBOX)
	CLiveHandler m_ExtraContent_Live;
#endif

#if defined(PLATFORM_CONSOLE)
	CWGameMultiplayerHandler *m_pMPHandler;
#endif

	virtual void SetDefaultProfileSettings(CRegistry* _pOptions);

	CExtraContent_Offline m_ExtraContent_Offline;
	CExtraContentHandler m_ExtraContent;

	unsigned int m_PendingWindow : 2;
	int m_Pad_ShowReconnect : 1;
	int m_Pad_Active : 7;

	//
	bool m_LoadHinted;
	bool m_BlackLoadingBG;

	bool m_SilentLogonRequested;

	//
	CMTime m_ReconnectTimer;
	int32 m_LockedPad; // -1 = none

	CGameContextMod();
	~CGameContextMod();

	virtual void Create(CStr _WorldPathes, CStr _GameName, spCXR_Engine _spEngine);

	class CItem
	{
	public:
		int32 m_Index;
		TPtr<CExtraContentHandler::CContent> m_spContent;
	};

	TArray<CItem> m_lViewableContent;
	int32 m_iCurrentContent;

	virtual void Simulate_Pause();
	virtual void DoLoadingStream();
	virtual void UpdateViewableContent();

	//CCubeUser m_CubeUser;
	virtual void UpdateControllerStatus();

	virtual void RenderSplitScreenGUI(CXR_VBManager* _pVBM, CRenderContext* _pRC, bool _bVSplit, bool _bHSplit);
	virtual void Render_PadStatus(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP, int _Context);
	virtual void Render_Corrupt(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP, int _Context);
	virtual void Render(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP, int _Context);
	virtual void RenderGUI(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP, int _Context);
	void Render_Loading(CXR_Engine * _pEngine, CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP, int _Context);

	virtual void ExecuteCommand(CGC_Command& _Cmd);

	virtual void OnDisconnect();

	virtual void Command_Map(CStr _Name);
	virtual uint32 GetViewFlags();

	void SetLockedPort(int32 _Port);

	void Con_SilentLogon();
#if defined(PLATFORM_XBOX1) && !defined(M_DEMO_XBOX)
	void Con_Logout();
	void Con_ConfirmLaunchDashboard(CStr _What);
#endif
#if defined(PLATFORM_CONSOLE)
	void Con_mpSearchForSessions(void);
	void Con_mpCreateSession(void);
	void Con_mpStartGame(void);
	void Con_mpShowFriends(void);
	void Con_mpShowLeaderboard(int _board);
	void Con_mpClearLeaderboard(void);
	void Con_mpReady(void);
	void Con_mpQuickgame(void);
	void Con_mpCustomgame(uint8 _CreateSession);
	void Con_mpLan(int _mp);
	void Con_mpRanked(int _ranked);
	void Con_mpBroadcast(void);
	void Con_mpShutdownLAN(void);
#endif
	void Con_mpClearAchievements(void);
	void Con_mpShutdownSession(uint32 _no_report = 0);
	void Con_mpShutdown(void);
	void Con_mpInit(void);

	void Con_SetDifficulty(CStr _What);

	void Con_noop();
	void Con_DeleteAllSavegames();
	void Con_DeleteProfile();
	void Con_SaveProfile();

	void Con_NextContent();
	void Con_PrevContent();

	void Con_HintLoad();

	void Con_PadLock();
	void Con_RemovePadLock();
//	void Con_SetGameClass(CStr _s);
	void Con_mpSetGameMode(CStr _s);

	void Con_unlockall();
	void Con_CampaignMap(CStr _Challenge);
	void Con_ScriptLayer(CStr _ScriptLayer);
	CStr GetScriptLayerInfo(CStr _ScriptLayer);

	virtual void Refresh(class CXR_VBManager* _pVBM);
	
	void Register(CScriptRegisterContext &_RegContext);
};

#endif
