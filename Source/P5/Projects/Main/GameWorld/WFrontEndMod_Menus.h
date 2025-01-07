#ifndef _INC_WFRONTENDMOD_MENUS
#define _INC_WFRONTENDMOD_MENUS

//#include "../Exe/WGameContextMain.h"

// -------------------------------------------------------------------
class CMWnd_ModMenu : public CMWnd_ModTexture
{
	MRTC_DECLARE;
public:
	CMWnd_FrontEndInfo m_FrontEndInfo;
	CMWnd_FrontEndInfo *GetFrontEndInfo()
	{
		return &m_FrontEndInfo;
	}

	enum
	{
		EPos_Top=1,
		EPos_Bottom=2
	};

	int32 m_ShowPressStart;
	bool m_ShowLogo;
	CMWnd_ModMenu() { m_ShowPressStart = 0; m_ShowLogo = false; }
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);

	static void PaintPressStart(CMWnd *_pWnd, int32 pos, CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

// -------------------------------------------------------------------
class CMWnd_CenterImage : public CMWnd_ModMenu
{
	MRTC_DECLARE;
public:
	bool m_bFitSafe;
	CPnt m_ImageSize;
	CMWnd_CenterImage();
	virtual void PaintTexture(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
};


// -------------------------------------------------------------------
class CMWnd_Timed : public CMWnd_ModMenu
{
	MRTC_DECLARE;
public:
	CMTime m_StartTime;
	CStr m_NextMenu;
	fp32 m_Time;
	CMWnd_Timed();
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void OnCreate();
	virtual void OnRefresh();
};


// -------------------------------------------------------------------
class CMWnd_Random : public CMWnd_ModMenu
{
	MRTC_DECLARE;
public:
	TArray<CStr> m_lScripts;
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
};


// -------------------------------------------------------------------
class CMWnd_View_Texture : public CMWnd_CenterImage
{
	MRTC_DECLARE;
public:
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
};

// --------------------------------------------------------------------
enum
{
	ITEMTYPE_WEAPON_GUN,
	ITEMTYPE_WEAPON_RIFLE,
	ITEMTYPE_MISSION,
	ITEMTYPE_DARKNESS = ITEMTYPE_MISSION,
};

struct SMissionItem {
	CStr m_ItemName;
	CStr m_ItemDesc;
	int m_AmountOfItem;
	int m_iPictureID;
	int m_iItemType;
};

class CMWnd_CubeMenu : public CMWnd_CubeLayout
{
	MRTC_DECLARE;
public:
	CMWnd_FrontEndInfo m_FrontEndInfo;
	CMWnd_FrontEndInfo *GetFrontEndInfo()
	{
		return &m_FrontEndInfo;
	}

	CStr m_Info;
	fp32 m_Time;
	CStr m_Script_Timeout;
	CMTime m_StartTime;


	bool m_InstantShow;

	int32 m_ShowPressStart;

	CCubeUser m_CubeUser;

	CMWnd_CubeMenu();
	virtual ~CMWnd_CubeMenu();

	virtual void SetDynamicLocalKey(CStr _KeyName, CStr _Value);

	virtual int ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey);

	void SetMenu(CStr _ID);
	void SetMenu(CStr _ID, CStr _Transfer);

	virtual bool OnButtonPress(CMWnd *_pFocus) { return false; }; // should return true if it was handeled
	virtual bool OnOtherButtonPress(CMWnd *_pFocus, int32 _Button) { return false; }; // should return true if it was handeled
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
	virtual void OnRefresh();

	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);

	virtual void SoundEvent(uint _SoundEvent, const char* _pSoundName = NULL);

	enum{
	STYLE_DARKLINGS,
	STYLE_MAP,
	STYLE_ITEMS,
	};
	virtual void OnPaintCommonStyle(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client, TArray<SMissionItem> _lItemList, int *_iSelected, int *_iLastSelected, int *_iStartAt, fp32 _ScrollUpTimer, fp32 _ScrollDownTimer, int8 _iStyle);
	virtual void MixTentaclesAndMenu(CRC_Util2D* _pRCUtil);
};

// ---------------------------------------------------------------------------------------------
class CMWnd_CubeMenu_Controller: public CMWnd_CenterImage
{
	MRTC_DECLARE;
public:
	CStr m_Texture;

	class CButton
	{
	public:
		int32 x, y;
		CStr m_Text;
	};

	TArray<CButton> m_lButtons;
	


	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

class CMWnd_CubeMenu_Controller2: public CMWnd_CubeMenu
{
	MRTC_DECLARE;
public:
	CStr m_TextureName;
	CPnt m_ImageSize;
	CPnt m_Offset;
	uint8 m_iType;

	class CButton
	{
	public:
		int32 x, y;
		CStr m_Text;
		CStr m_ImageName;
		CPnt m_Size;
		bool m_bAfterText;
	};

	TArray<CButton> m_lButtons;

	virtual void OnCreate();
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

// ---------------------------------------------------------------------------------------------
class CMWnd_CubeMenu_Select_Profile : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
public:
	CMWnd_CubeMenu_Select_Profile();
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);

	TArray<CStr> m_lProfiles;
	int32 m_iSelected;
	int32 m_SelectAction;

	enum
	{
		ACTION_USE,
		ACTION_DELETE
	};

	virtual bool OnOtherButtonPress(CMWnd *_pFocus, int32 _Button);
	virtual bool OnButtonPress(CMWnd *_pFocus);
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr &_Name, bool _Focus);
	virtual void OnCreate();
	virtual void OnActivate();
};

// ---------------------------------------------------------------------------------------------

/*
class CMWnd_CubeMenu_ReturnBack : public CMWnd_CubeMenu
{
MRTC_DECLARE;
public:
~CMWnd_CubeMenu_ReturnBack();
virtual void OnDestroy();
};*/


// ---------------------------------------------------------------------------------------------
class CMWnd_CubeMenu_ButtonConfig : public CMWnd_CubeMenu
{
	MRTC_DECLARE;

	int32 m_Binding;
	TArray<CStr> m_lActions;
	TArray<CStr> m_lButtons;
	TArray<int> m_lType;

	void FetchNames(); // fetches binds from the options-registry and translates into names

public:

	CMWnd_CubeMenu_ButtonConfig();

	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr &_Name, bool _Focus);
	virtual bool OnButtonPress(CMWnd *_pFocus);

	virtual int ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey);
	//virtual int OnProcessScanKey(const CScanKey& _Key);
	virtual bool OnOtherButtonPress(CMWnd *_pFocus, int32 _Button);
	//virtual aint OnMessage(const CMWnd_Message* _pMsg);
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void OnCreate();
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};


// ---------------------------------------------------------------------------------------------
class CMWnd_CubeMenu_VideoSelection : public CMWnd_CubeMenu
{
	MRTC_DECLARE;

	bool m_Apply;
	int32 m_CurrentRes;
	int32 m_CurrentHertz;
	int32 m_CurrentHertzInit;
	TArray<CPnt> m_lRes;
	TArray<int32> m_lHertz;

	class CPixelAspect
	{
	public:
		CPixelAspect(CStr _Desc, fp32 _Value)
		{
			m_Desc = _Desc;
			m_Aspect = _Value;
		}
		CPixelAspect()
		{
		}
		CStr m_Desc;
		fp32 m_Aspect;
	};
	TArray<CPixelAspect> m_PixelAspects;

	void FetchList(class CDisplayContext *pDC); // fetches binds from the options-registry and translates into names
	void FetchHertz(class CDisplayContext *pDC, int32 w, int32 h, int32 bpp);
	void UpdateButtons();
public:

	CMWnd_CubeMenu_VideoSelection();
	~CMWnd_CubeMenu_VideoSelection();

	virtual bool OnButtonPress(CMWnd *_pFocus);

	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void OnCreate();
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
	virtual void OnRefresh();
};

// ---------------------------------------------------------------------------------------------
class CMWnd_CubeMenu_TextInput : public CMWnd_CubeMenu // shoudl be renamed
{
	MRTC_DECLARE;
public:
	CStr m_PreString;
	CStr m_String;
	uint m_nMaxLen;
	CMWnd *m_pTextView;

	virtual int ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey);
	virtual bool OnOtherButtonPress(CMWnd *_pFocus, int32 _Button);
	virtual bool OnButtonPress(CMWnd *_pFocus);
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void OnCreate();
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);

	virtual bool OnTextInputDone();
};

// ---------------------------------------------------------------------------------------------
class CMWnd_CubeMenu_Select_SaveGame : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
public:
	TArray<CStr> m_lSaveGames;
	int32 m_iSelected;

	virtual bool OnButtonPress(CMWnd *_pFocus);
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr &_Name, bool _Focus);
	virtual void OnCreate();
};

// ---------------------------------------------------------------------------------------------
class CMWnd_CubeMenu_Extra_Content : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
public:
	virtual bool OnButtonPress(CMWnd *_pFocus);
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr &_Name, bool _Focus);
	virtual void AfterItemRender(int32 _Index);
	virtual void OnCreate();
};

class CMWnd_CubeMenu_Extra_Content_View : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
public:
	enum
	{
		ACTION_NONE,
		ACTION_VIEW,		// Video and picture
		ACTION_READ,		// Text
		ACTION_LAUNCH,		// Extra level
		ACTION_REMOVE,		// DL content remove
		ACTION_DOWNLOAD		// DL content download
	};

	int32 m_ContentIndex;
	int32 m_Action;

	virtual void DoLayout();
	virtual bool OnButtonPress(CMWnd *_pFocus);
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr &_Name, bool _Focus);
	virtual void OnCreate();
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

class CMWnd_CubeMenu_Extra_Content_ViewText : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
public:
	int32 m_iReg;
	int32 m_Page;
	int32 m_PageSize;
	int32 m_NumPages;

	CRct m_TextRect;
	CRct m_PageCountRect;

	const CRegistry *m_pReg;

	void Redisplay();
	virtual bool OnButtonPress(CMWnd *_pFocus);
	//virtual bool OnOtherButtonPress(CMWnd *_pFocus, int32 _Button);
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
	virtual void OnCreate();
};

// ---------------------------------------------------------------------------------------------
class CMWnd_CubeMenu_Fork: public CMWnd_CubeMenu
{
	MRTC_DECLARE;
public:
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void EvaluateRegisty(CMWnd_Param* _pParam, CRegistry *_pReg);
};

// ---------------------------------------------------------------------------------------------
/*
class CMWnd_CubeMenu_SaveProfile : public CMWnd_CubeMenu
{
MRTC_DECLARE;
public:
bool m_HasWritten;
CMTime m_StartTime;
virtual void OnCreate();
virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};*/



// ---------------------------------------------------------------------------------------------
class CMWnd_CubeMenu_Chooser : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
public:
	virtual void OnCreate();

	virtual bool OnButtonPress(CMWnd *_pFocus);
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr &_Name, bool _Focus);

	virtual	int ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

// ---------------------------------------------------------------------------------------------
class CMWnd_CubeMenu_ChooserBuilder : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
public:
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void EvaluateRegisty(CMWnd_Param* _pParam, CRegistry *_pReg);
	virtual void EvaluateKeyOrdered(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
};

// -------------------------------------------------------------------
class CMWnd_CubeMenu_NewGame : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
public:
	virtual void OnCreate();
};

// -------------------------------------------------------------------
class CMWnd_ModGameMenu : public CMWnd
{
	MRTC_DECLARE;
public:
	CMWnd_FrontEndInfo m_FrontEndInfo;
	CMWnd_FrontEndInfo *GetFrontEndInfo()
	{
		return &m_FrontEndInfo;
	}

	CMWnd_ModGameMenu()
	{
		m_CheatUplockMode = 0;
		m_pInfoScreen = MNew(CMWnd_ModInfoScreen);
	}
	int m_CheatUplockMode;
	CStr m_CheatScript1;
	CStr m_ConfimScript;
	CMWnd_ModInfoScreen * m_pInfoScreen;	
	CM_Mod_ColorController m_CC;
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
	virtual void OnRefresh();
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
};

class CMWnd_CubeMenu_Achievements : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
	struct SListItem 
	{
		CStr Name;
		CStr Info;
		bool Completed;
	};
	int m_iSelected;
	bool m_bEnumerating;

	TArray<SListItem>	m_List;

public:
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr& _Name, bool _Focus);
	virtual bool OnButtonPress(CMWnd* _pFocus);
	virtual bool OnOtherButtonPress(CMWnd *_pFocus, int32 _Button); 
	virtual void OnCreate();
	virtual void OnRefresh();
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

// ---------------------------------------------------------------------------------------------
class CMWnd_CubeMenu_LoadMap : public CMWnd_CubeMenu
{
	MRTC_DECLARE;

protected:
	TArray<CStr> m_List;
	int m_iSelected;
	CStr m_QuickStr;
	CMTime m_LastKeyPressTime;

	void UpdateFocus();

public:
	virtual int ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey);
	virtual bool OnButtonPress(CMWnd* _pFocus);
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr& _Name, bool _Focus);
	virtual void AfterItemRender(int32 _Index);
	virtual void OnCreate();
};


class CMWnd_CubeMenu_LoadScriptLayer  : public CMWnd_CubeMenu_LoadMap
{
	MRTC_DECLARE;

	static TArray<CStr> ms_lScriptLayers;	// only reloaded when filetime changes
	static int64 ms_TimeStamp;

	CStr m_Filter;
	void UpdateScriptLayers();
	void CreateList();

public:
	virtual void OnCreate();
	virtual bool OnButtonPress(CMWnd* _pFocus);
	virtual int ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey);
};


class CMWnd_CubeMenu_Multiplayer_SelectMap : public CMWnd_CubeMenu
{
	MRTC_DECLARE;

	TArray<CStr> m_List;
	int m_iSelected;
	int m_iMap;
	bool m_bIsCTF;
	int m_nMaps;
	void RandomMap(void);

public:
	virtual bool OnButtonPress(CMWnd* _pFocus);
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr& _Name, bool _Focus);
	virtual void OnCreate();
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

class CMWnd_CubeMenu_Multiplayer_SetupCharacter : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
	struct SListItem 
	{
		CStr DisplayName;
		CStr TemplateName;
	};

	TArray<SListItem>	m_List;
	CRct m_TypePos;
	int m_iSelected;
	int m_iModel;
	bool m_bHumans;

	void CreateList(void);

public:
	virtual bool OnButtonPress(CMWnd* _pFocus);
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr& _Name, bool _Focus);
	virtual void OnCreate();
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

class CMWnd_CubeMenu_Multiplayer_SelectServerList : public CMWnd_CubeMenu
{
	MRTC_DECLARE;

	TArray<CStr> m_List;
	int m_iSelected;

	bool	m_bNoServers;

public:
	CMWnd_CubeMenu_Multiplayer_SelectServerList();
	virtual void OnActivate();
	virtual bool OnButtonPress(CMWnd* _pFocus);
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr& _Name, bool _Focus);
	virtual void OnCreate();

	virtual void OnRefresh();

private:
	CMTime		m_LastUpdate;
	CMTime		m_ActivateTime;
};

class CMWnd_CubeMenu_Multiplayer_StartGame : public CMWnd_CubeMenu
{
	MRTC_DECLARE;

protected:
	struct SListItem 
	{
		CStr Name;
		int PlayerID;
		bool bIsTalking:1;
		bool bIsReady:1;
	};

	TArray<SListItem>	m_List;
	int m_iSelected;
	int		m_LastNumberOfPlayers;

public:
	CMWnd_CubeMenu_Multiplayer_StartGame();
	virtual bool OnButtonPress(CMWnd* _pFocus);
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr& _Name, bool _Focus);
	virtual void OnCreate();

	virtual void OnRefresh();
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

class CMWnd_CubeMenu_Multiplayer_PostGameLobby : public CMWnd_CubeMenu_Multiplayer_StartGame
{
	MRTC_DECLARE;

public:
	CMWnd_CubeMenu_Multiplayer_PostGameLobby();
	virtual bool OnButtonPress(CMWnd* _pFocus);
};

class CMWnd_CubeMenu_Multiplayer_SelectGameMode : public CMWnd_CubeMenu
{
	MRTC_DECLARE;

	TArray<CStr> m_List;
	int m_iSelected;

	CStr m_Map;
	int m_iModeSelected;
	int m_iRuleSelected;
	TArray<int> m_lButtonsRemoved;
	void ValidateRule(void);

public:
	virtual bool OnButtonPress(CMWnd* _pFocus);
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr& _Name, bool _Focus);
	virtual void OnCreate();
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

class CMWnd_CubeMenu_Multiplayer_Search : public CMWnd_CubeMenu
{
	MRTC_DECLARE;

	TArray<CStr> m_List;
	int m_iSelected;

public:
	virtual bool OnButtonPress(CMWnd* _pFocus);
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr& _Name, bool _Focus);
	virtual void OnCreate();
};

class CMWnd_CubeMenu_Multiplayer_Leaderboard : public CMWnd_CubeMenu
{
	MRTC_DECLARE;

	struct SListItem 
	{
		CStr Name;
		int PlayerID;	
	};

	TArray<SListItem> m_List;
	int m_iSelected;

	bool m_bFriends;

public:
	virtual bool OnButtonPress(CMWnd* _pFocus);
	virtual bool OnOtherButtonPress(CMWnd *_pFocus, int32 _Button); 
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr& _Name, bool _Focus);
	virtual void OnCreate();
	virtual void OnRefresh();
};

class CMWnd_CubeMenu_Multiplayer_AdvancedSettings : public CMWnd_CubeMenu
{
	MRTC_DECLARE;

public:
	virtual bool OnButtonPress(CMWnd* _pFocus);
};

// ---------------------------------------------------------------------------------------------
class CMWnd_CubeMenu_Demos : public CMWnd_CubeMenu
{
	MRTC_DECLARE;

	TArray<CStr> m_List;
	int m_iSelected;
	CStr m_Execute;

public:

	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual bool OnButtonPress(CMWnd* _pFocus);
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr& _Name, bool _Focus);
	virtual void AfterItemRender(int32 _Index);
	virtual void OnCreate();
};

// ---------------------------------------------------------------------------------------------
class CMWnd_CubeMenu_SelectServer : public CMWnd_CubeMenu_TextInput
{
	MRTC_DECLARE;
public:
	virtual bool OnTextInputDone();
	virtual int  ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey);
};

//----------------------------------------------------------------------------------------------

class CMWnd_CubeMenu_Mission_Items :  public CMWnd_CubeMenu
{
	MRTC_DECLARE;

	TArray<SMissionItem> m_List;
	TArray<SMissionItem> m_ListWeapons;
	int m_iSelected;
	int m_iLastSelected;
	int m_iStartAt;

	fp32 m_ScrollUpTimer;
	fp32 m_ScrollDownTimer;
	fp32 m_LastTime;

public:
	virtual bool OnButtonPress(CMWnd *_pFocus);
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr &_Name, bool _Focus);
	virtual void OnCreate();

	virtual int ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey);	
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

class CMWnd_CubeMenu_Map :  public CMWnd_CubeMenu
{
	MRTC_DECLARE;

	TArray<SMissionItem> m_List;
	TArray<CStr> m_ListInfo;
	int m_iSelected;
	int m_iLastSelected;
	int m_iStartAt;
	bool m_bIsNy;

public:
	virtual bool OnButtonPress(CMWnd *_pFocus);
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr &_Name, bool _Focus);
	virtual void OnCreate();

	virtual int ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};


class CMWnd_CubeMenu_Darklings :  public CMWnd_CubeMenu
{
	MRTC_DECLARE;

	TArray<SMissionItem> m_List;
	int m_iSelected;
	int m_iLastSelected;
	int m_iStartAt;

	fp32 m_ScrollUpTimer;
	fp32 m_ScrollDownTimer;
	fp32 m_LastTime;
	
	uint16 m_DarknessLevel;
	TArray<int> m_DarknessLevelPicIDList;

public:
	virtual bool OnButtonPress(CMWnd *_pFocus);
	virtual int32 GetNumListItems(bool _Drawing);
	virtual bool GetListItem(int32 _Index, CStr &_Name, bool _Focus);
	virtual void OnCreate();

	virtual int ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);

	virtual void AddPowerToList(SMissionItem &_Item, CStr _PowerName, CStr _PowerDesc, CStr _ImageName, CTextureContext *_pTC);
};

#endif // _INC_WFRONTENDMOD_MENUS
