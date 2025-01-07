#ifndef __MWINDOWS_H
#define __MWINDOWS_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Window-system top-level classes

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CMWnd_Param
					CMWnd_Accellerator
					CMWnd
					CMWnd_Client
					CMWnd_Splitbar
					CMWnd_SplitWnd
					CMWnd_Border
					CMWnd_ResizePad
					CMWnd_Window
					CMWnd_Desktop
					CMWnd_Button
					CMWnd_DragbarX
					CMWnd_DragbarY
					CMWnd_ListItem
					CMWnd_List
					CMWnd_ListClient
					CMWnd_ModListItem3
					CMWnd_ModList3
					CMWnd_Starview
\*____________________________________________________________________________________________*/

// ENZO: For GUI coding 
//#define TESTING_GC_GUI

#if defined(PLATFORM_WIN_PC)
#   if defined(TESTING_GC_GUI)
#      define GUI_DOLPHIN 
#   else 
#      define GUI_PC 
#   endif
#elif defined(PLATFORM_DOLPHIN)
#   define GUI_DOLPHIN 
#endif



#include "MWinGrph.h"

#define XWND_DOUBLECLICKTIME	0.3f

MRTCDLLEXPORT void CMWnd_DrawText(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, CRC_Font* _pF, wchar* _pStr, int _x, int _y,
	int _Style, int ColM, int ColH, int ColD);

MRTCDLLEXPORT int CMWnd_Text_WordWrap(CRC_Util2D* _pRCUtil, CRC_Font* _pF, int _Width, wchar* _pStr, int _Len, wchar** _ppLines, int _MaxLines);

MRTCDLLEXPORT int CMWnd_Text_DrawFormated(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, CRC_Font* _pF, const CStr& _Text, int _x, int _y,
	int _Style, int _ColM, int _ColH, int _ColD, int _Width, int _Height);

MRTCDLLEXPORT int CMWnd_Text_DrawFormated(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, CRC_Font* _pF, const CStr& _Text, int& _x, int& _y,
	int _Style, int _ColM, int _ColH, int _ColD, int _Width, int _Height, fp32 _PercentVis);

// -------------------------------------------------------------------
class CMWnd;
typedef TPtr<CMWnd> spCMWnd;

extern spCMWnd g_spDesktop;

// -------------------------------------------------------------------
class CMWnd_Param
{
public:
	CRct m_Pos;
	int m_Style;
	int m_Status;
	CPnt m_Align0;
	CPnt m_Align1;

	CMWnd_Param() :
		m_Pos(0,0,0,0)
	{
		m_Style = 0;
		m_Status = 0;
	}

	CMWnd_Param(int _x, int _y, int _w, int _h, int _Style, int _Status = 0, const CPnt& _Align0 = CPnt(0,0), const CPnt& _Align1 = CPnt(0,0))
	{
		m_Pos = CRct(_x, _y, _x+_w, _y+_h);
		m_Style = _Style;
		m_Status = _Status;
		m_Align0 = _Align0;
		m_Align1 = _Align1;
	}
};

// -------------------------------------------------------------------
class CMWnd_Message
{
public:
	int m_Msg;
	char* m_pID;
	aint m_Param0;
	aint m_Param1;
	aint m_Param2;
	aint m_Param3;
	fp32 m_fParam0;
	fp32 m_fParam1;
	fp32 m_fParam2;
	fp32 m_fParam3;
	CVec3Dfp32 m_vParam0;
	CVec3Dfp32 m_vParam1;

	void Clear();
	CMWnd_Message() { Clear(); };
	CMWnd_Message(int _Msg, char* _pID, int _p0, int _p1 = 0, int _p2 = 0, int _p3 = 0);

	// Helpers
	int GetCursorBtn() const { return m_Param2 & 0xff; };
	CPnt GetCursorPos() const { return CPnt(m_Param0, m_Param1); };
	bool IsPressed() const { return !(m_Param2 & 0x8000); };
	bool IsReleased() const { return (m_Param2 & 0x8000) != 0; };
	bool IsMouse1() const { return (GetCursorBtn() & 1) != 0; };
	bool IsMouse2() const { return (GetCursorBtn() & 2) != 0; };
	bool IsMouse3() const { return (GetCursorBtn() & 4) != 0; };
};

// -------------------------------------------------------------------
#define WMSG_CURSOR		1		// Clicks
#define WMSG_CURSORMOVE	2		// Cursor move
#define WMSG_KEY		3

#define WMSG_LOCKEDCURSOR		4
#define WMSG_LOCKEDCURSORMOVE	5
#define WMSG_LOCKEDKEY			6

#define WMSG_COMMAND	7
#define WMSG_CLOSE		8
#define WMSG_OK			9
#define WMSG_CANCEL		10

#define WMSG_MENUIMPULSE 11

#define WMSG_WNDDRAG	12
#define WMSG_WNDRESIZE	13
#define WMSG_CONSTRUCT	14

#define	WMSG_MULTIDISC_CALLBACK 15
#define WMSG_GLOBAL_PAUSE       16
#define WMSG_GLOBAL_RESUME      17



// -------------------------------------------------------------------
class CMWnd_View : public CReferenceCount
{
//	MRTC_DECLARE;

public:

	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client) pure;

	MACRO_OPERATOR_TPTR(CMWnd_View);
};

typedef TPtr<CMWnd_View> spCMWnd_View;

// -------------------------------------------------------------------
//  Basic styles, custom window-styles start at 0x100
#define WSTYLE_MOVABLE		1
#define WSTYLE_RESIZABLE	2
#define WSTYLE_TABSTOP		4
#define WSTYLE_ONTOP		8
#define WSTYLE_BEHIND		16
#define WSTYLE_CLIENTEDGE	32
#define WSTYLE_NOKEYPROCESS	64
#define WSTYLE_HIDDENFOCUS	128		// Focus is not shown visually.
#define WSTYLE_NOCLIPPING	256

// -------------------------------------------------------------------
#define WSTATUS_FOCUS			1
#define WSTATUS_MAXIMIZED		2
#define WSTATUS_MINIMIZED		4
#define WSTATUS_DISABLED		8
#define WSTATUS_INACTIVE		16
#define WSTATUS_PRESSURE		32
#define WSTATUS_MSGTOPARENT		64
#define WSTATUS_CREATED			128
#define WSTATUS_INVISIBLE		256
#define WSTATUS_TREEINVISIBLE	512
#define WSTATUS_MOUSEOVER		1024

#define WSTATUS_ALIGNY0_TOP		0x00010000
#define WSTATUS_ALIGNY0_BOTTOM	0x00020000
#define WSTATUS_ALIGNX0_LEFT	0x00040000
#define WSTATUS_ALIGNX0_RIGHT	0x00080000
#define WSTATUS_ALIGNY1_TOP		0x00100000
#define WSTATUS_ALIGNY1_BOTTOM	0x00200000
#define WSTATUS_ALIGNX1_LEFT	0x00400000
#define WSTATUS_ALIGNX1_RIGHT	0x00800000

#define WSTATUS_ALIGNMASK		0x00ff0000

enum
{
	WSND_USENAME = 0,
	WSND_ENTER,
	WSND_EXIT,
	WSND_OPEN,
	WSND_CLOSE,
	WSND_TAB,
	WSND_SELECT,
	WSND_NAVIGATE,
	WSND_ACTIVATE,
	WSND_LEFT,
	WSND_RIGHT,
	WSND_UP,
	WSND_DOWN,
	WSND_SCROLL,
	WSND_SCROLL_LEFT,
	WSND_SCROLL_RIGHT,
	WSND_SCROLL_UP,
	WSND_SCROLL_DOWN,
	WSND_SWITCH,
	WSND_SWITCH_LEFT,
	WSND_SWITCH_RIGHT,
	WSND_SWITCH_UP,
	WSND_SWITCH_DOWN,
	WSND_USERBASE = 0x00010000,
};

// -------------------------------------------------------------------
//  CMWnd_Accellerator
//
//  "Accellerator" is win32 terminology for hotkey, shortcut, etc..
//  Accellerators cannot be triggered unless the window
//  containing them is in the focus-chain. Therefore, accellerators are 
//  typically added to the root window object instead of the window
//  items.
// -------------------------------------------------------------------
class CMWnd_Accellerator : public CReferenceCount
{
public:
	uint32 m_ScanCode;		// Scancode that triggers this accellerator
	CStr m_FocusTarget;		// Child window to change focus to when triggered.
	CStr m_Script;			// Script to execute when triggered.
};

typedef TPtr<CMWnd_Accellerator> spCMWnd_Accellerator;

class COutsideButton
{
public:
	int32 m_Pos;
	fp32 m_Scale;
	CStr m_Action;
	CStr m_Caption;
	int32 m_Type;
	fp32 m_Extend; // used so that we can press on the text aswell
};

// -------------------------------------------------------------------
//  CMWnd
// -------------------------------------------------------------------
typedef TPtr<CMWnd> spCMWnd;

class CMWnd : public CMWnd_View
{
	MRTC_DECLARE;

private:
	// Static data

	static CMTime ms_LastRepeatTime;
	static fp32 ms_RepeatDelay;

protected:
	TArray<spCMWnd> m_lspWndChildren;
	CMWnd* m_pWndParent;
	CMWnd* m_pWndRoot;

	CMWnd* m_pWndLockInput;

	CStr m_Script_Create;
	CStr m_Script_Destroy;
	TArray<spCMWnd_Accellerator> m_lspAccellerators;

	TArray<spCRC_Font> m_lspFonts;
	TArray<CStr> m_lFontNames;
	bool m_bAllowChildFocusWrap;

public:

	CStr m_Script_Update;

	int m_Style;
	int m_Status;
	bool m_bQuit;
	int m_ExitCode;

	CRct m_Pos;
	CPnt m_Align0;
	CPnt m_Align1;
	CRct m_AbsPos;
	CRct m_RestorePos;
	CPnt m_GrabPos;

	// Item identifier
	CStr m_ID;
	CStr m_Group;

	// Focus A&B button graphics
	CStr m_aButtonDescriptions[4];
	bool m_Interactive;
	bool m_BlocksLoadingScreen;
	bool m_bShowMouse;
	bool m_AllowsAttractMode;
	bool m_UGLYDEMOHACK; // $$$$$$
	bool m_bMouseOverWindow;

	TArray<COutsideButton> m_lOutsideButtons; // buttons outside das cube

	/*
	CStr m_AButtonDescriptor;
	CStr m_BButtonDescriptor;
	CStr m_XButtonDescriptor;
	CStr m_YButtonDescriptor;
	int m_iButtonDescriptorVCorrection;
	*/

	spCMWnd_View m_spView;
	void(*m_pfnBusyCallback)();

	// Privates
	void SetRoot(CMWnd* _pWnd);
	void ParentDestroyed();
	void DoBusy();
	void DoRefresh();
	bool DoLockInput(CMWnd* _pWnd);
	void DoUnlockInput(CMWnd* _pWnd);

	CMWnd* GetFocusWnd();
	CMWnd* GetChildFocusWnd();
	CMWnd* GetCursorOverNoClippingWnd(const CPnt& _Pos);
	CMWnd* SearchCursorOverNoClippingWnd(const CPnt& _Pos);
	CMWnd* GetCursorOverWnd(const CPnt& _Pos, bool _OnlyTabStops);
	CMWnd* GetRootWnd();
	CMWnd* GetFirstChild();

	// Superfocus

	CMWnd* m_SuperFocusWindow;

public:
	// Colors
	int m_Col_Alpha;
	int m_Col_Highlight;
	int m_Col_Background;
	int m_Col_Shadow;
	int m_Col_DarkShadow;
	int m_Col_Custom0;
	int m_Col_Custom0_Focus;
	int m_Col_Custom1;
	int m_Col_Custom1_Focus;

	// Run tree.
	virtual void FetchInput();
	virtual int ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey);
	virtual int ProcessCursor(const CPnt& _Pos, const CPnt& _dPos, int _Buttons);

	virtual bool LockInput();
	virtual void UnlockInput();

	// Asyncronous waiting
	void Wait();
	void ResetRepeatDelay();
	void MouseRepeatDelay(int _Type);

	// Hierarchy
	void UpdateAbsPos();
	virtual void AddChild(spCMWnd _spWnd);
	virtual void RemoveChild(CMWnd* _pWnd);
	virtual void RemoveAllChilds();
	virtual CMWnd* GetParent();
	void BringToTop(CMWnd* _pWnd);
	void KillFocus();
	virtual void SetFocus();
	virtual int32 GetNumItems();
	virtual CMWnd* GetItem(const int32 _Index);
	virtual CMWnd* GetItem(const CStr& _ID);
	virtual CMWnd* FindItem(const CStr& _ID);
	virtual CMWnd* FindItem(CMWnd *_pWnd);

	// Create/Destroy
private:
	void Clear();
protected:
	virtual void DoCreate(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID);
	virtual void DoDestroy();

public:
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void EvaluateRegisty(CMWnd_Param* _pParam, CRegistry *_pReg);
	virtual void EvaluateByKey(CMWnd_Param* _pParam, CRegistry *_pReg, const char *_pKey);
	virtual void EvaluateKeyOrdered(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void CreateChildren(CKeyContainerNode* _pKeys);
	virtual void CreateChildren(CRegistry* _pReg);
	virtual void Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID);

	void Create(CKeyContainerNode* _pKeys, CMWnd* _pWndParent = NULL);
	void Create(CRegistry* _pReg, CMWnd* _pWndParent = NULL);
	void Destroy();

	CMWnd();
	CMWnd(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID);
	virtual ~CMWnd();

	// Font
	virtual void AddFont(spCRC_Font _spFont, const CStr& _Name);
	virtual spCRC_Font GetFont(char* _pName);
	
	virtual class CMWnd_FrontEndInfo *GetFrontEndInfo()
	{
		return NULL;
	}

	virtual class CMWnd_ClientInfo *GetClientInfo()
	{
		return NULL;
	}

	// Cursor
	virtual CPnt GetCursorPos(const CMWnd_Message* _pMsg);
//	virtual int GetCursorBtn(const CMWnd_Message* _pMsg);
	virtual bool CursorInRect(const CMWnd_Message* _pMsg, const CRct& _Rect);
	virtual bool CursorInWnd(const CMWnd_Message* _pMsg);
	virtual bool PntInWnd(const CPnt& _Pos);

	// Positioning
	virtual CPnt GetAbsOrigo();
	virtual void SetPosition(const CRct& _Pos);
	virtual void SetPosition(const CPnt& _Pos);
	virtual void SetPosition(int _x0, int _y0, int _x1, int _y1);
	virtual CRct GetPosition();
	virtual CPnt GetCenterPosition();
	virtual int GetWidth();
	virtual int GetHeight();
	virtual void SetWidth(int _w);
	virtual void SetHeight(int _h);
	virtual void Maximize();
	virtual void Minimize();

	// Set
	virtual void SetStatus(int _Flag, bool _State);
	virtual void SetView(spCMWnd_View _spView);
	virtual void SetBusyCallBack(void(*_pfnBusyCallBack)());

	// Notification/Interception
	virtual void PreEvalKeys(CMWnd_Param* _pParam);									// Run before eval keys, also run before PreCreate if window is created without keys.
	virtual void PreCreate(CMWnd_Param* _pParam);									// Run after eval keys and before DoCreate, also run before DoCreate if window is created without keys.
	virtual void OnSetAutoFocus();
	virtual void OnSetFocus();
	virtual int OnChangeChildFocus(int _dX, int _dY);
	virtual void OnCreate();
	virtual void OnActivate();
	virtual void OnDestroy();
	virtual void OnParentDestroyed();
	virtual void OnMove();
	virtual void OnParentMove();

	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual void OnPaintFocus(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual void OnPaintPreWorld(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client) {}
	virtual int OnPressed(const CMWnd_Message* _pMsg);								// Called upon cursor interaction by CMWnd::OnMessage in the default handler.
	virtual void OnPress(const CMWnd_Message* _pMsg);								// Called upon cursor interaction by CMWnd::OnMessage in the default handler.
	virtual int OnProcessScanKey(const CMWnd_Message* _pMsg);
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
	virtual void OnRefresh();
	virtual void OnBusy();

	virtual bool OnExecuteString(const CStr& _Script);										// Returns false if script compile or execution throws.

	// Sound
	virtual void SoundEvent(uint _SoundEvent, const char* _pSoundName = NULL);

	// Exclusive execution of this window
	virtual int DoModal();

	// Render
	void RenderTree(CRC_Util2D* _pRCUtil, const CClipRect& _Clip);

	// Debug, write window tree to logfile
	CStr OnGetDumpInformation();
	void OnDumpTree(int _iLevel = 0);

	MACRO_OPERATOR_TPTR(CMWnd);
};

// -------------------------------------------------------------------
//  CMWnd_Caption
// -------------------------------------------------------------------
// Styles shared with CMWnd_Window:
#define WSTYLE_CAPTN_TOOLWINDOW		0x0100
#define WSTYLE_CAPTN_CLOSEBTN		0x0200
#define WSTYLE_CAPTN_MAXBTN			0x0400
#define WSTYLE_CAPTN_MINBTN			0x0800

class CMWnd_Caption : public CMWnd
{
	MRTC_DECLARE;

protected:
	CStr m_Caption;
	CMWnd* m_pWndClose;
	CMWnd* m_pWndMax;
	CMWnd* m_pWndMin;

	void Clear();
protected:
	virtual void DoCreate(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID);
	virtual void DoDestroy();

public:
	virtual void Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, const CStr& _Caption = "");

	CMWnd_Caption();
	CMWnd_Caption(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, const CStr& _Caption = "");

	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual int GetHeight();
	virtual void OnMove();
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
};

// -------------------------------------------------------------------
//  CMWnd_Text
// -------------------------------------------------------------------
#define WSTYLE_TEXT_CUTOUT		0x0100
#define WSTYLE_TEXT_SHADOW		0x0200
#define WSTYLE_TEXT_HIGHLIGHT	0x0400
#define WSTYLE_TEXT_CENTER		0x0800
#define WSTYLE_TEXT_CENTERY		0x1000
#define WSTYLE_TEXT_WORDWRAP	0x2000

#define WMSG_SETTEXT 0x3000

class CMWnd_Text : public CMWnd
{
	MRTC_DECLARE;
	
protected:
	CStr m_Text;
	CStr m_Font;
	bool m_Outline;

	void Clear();

public:
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);

	CMWnd_Text();
	CMWnd_Text(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, const CStr& _Text);

	virtual aint OnMessage(const CMWnd_Message* _pMsg);

	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
};

// -------------------------------------------------------------------
//  CMWnd_Client
// -------------------------------------------------------------------
#define WSTYLE_CLIENT_SCROLLX	0x0100
#define WSTYLE_CLIENT_SCROLLY	0x0200
#define WSTYLE_CLIENT_BORDER	0x0400
#define WSTYLE_CLIENT_RESIZEPAD	0x0800	// Make a resizing control of the unused corner
										// inbetween the scrollbars. (if both are visible.)

class CMWnd_DragbarX;
class CMWnd_DragbarY;

class CMWnd_Client : public CMWnd
{
	MRTC_DECLARE;

protected:
	CPnt m_MinClientSize;
	CPnt m_ClientPos;

	CMWnd_DragbarX* m_pWndScrollX;
	CMWnd_DragbarY* m_pWndScrollY;
	CMWnd* m_pWndClient;
	CMWnd* m_pWndClipClient;
	CMWnd* m_pWndResizePad;

	void Clear();
protected:
	virtual void DoCreate(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID);
	virtual spCMWnd CreateClient();

public:
	virtual CMWnd* GetClient() { return m_pWndClient; };			// m_pWndClient is a child to m_pWndClipClient
	virtual CMWnd* GetClipClient() { return m_pWndClipClient; };	// m_pWndClipClient is a child to *this

	virtual void MakeVisible(const CRct& _Area);						// Try to make _Area portion of the _pWndClient visible;

	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, const CPnt& _Size = CPnt(0,0));

	CMWnd_Client();
	CMWnd_Client(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, const CPnt& _Size = CPnt(0,0));

	virtual void AddChild(spCMWnd _spWnd);
	virtual CMWnd* GetItem(const CStr& _ID);

	virtual void SetView(spCMWnd_View _spView);
	virtual void OnMove();
	virtual void OnParentMove();
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual aint OnMessage(const CMWnd_Message* _pMsg);

	virtual void SetClientSize(int _w, int _h);
};

// -------------------------------------------------------------------
//  CMWnd_Splitbar
// -------------------------------------------------------------------
class CMWnd_Splitbar : public CMWnd
{
public:
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
};

// -------------------------------------------------------------------
//  CMWnd_SplitWnd
// -------------------------------------------------------------------
#define WSTYLE_SPLITWND_Y	0x0100

class CMWnd_SplitWnd : public CMWnd
{
	MRTC_DECLARE;

protected:
	CStr m_SplitWnd1ID;
	CStr m_SplitWnd2ID;
	CMWnd* m_pSplitWnd1;
	CMWnd* m_pSplitWnd2;

	CMWnd_Splitbar* m_pWndDrag;
	int m_SplitPos;

	virtual void DoCreate(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID);
	void Clear();

public:
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, 
		spCMWnd _spWnd1, spCMWnd _spWnd2, int _SplitPos = -1);

	CMWnd_SplitWnd();
	CMWnd_SplitWnd(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID,
		spCMWnd _spWnd1, spCMWnd _spWnd2, int _SplitPos = -1);

	virtual void PreCreate(CMWnd_Param* _pParam);
	virtual void OnCreate();
	virtual void OnMove();
	virtual void OnParentMove();

	virtual aint OnMessage(const CMWnd_Message* _pMsg);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
};

// -------------------------------------------------------------------
//  CMWnd_Border
// -------------------------------------------------------------------
#define WSTYLE_BORDER_THIN	0x0100

class CMWnd_Border : public CMWnd
{
	MRTC_DECLARE;

protected:
	int m_Preasure;
	CPnt m_GrabPos2;
	int m_GrabMode;

	virtual int OnBorderPress(const CMWnd_Message* _pMsg);

public:
	CMWnd_Border();
	CMWnd_Border(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID);

	virtual void PreCreate(CMWnd_Param* _pParam);

	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
};

// -------------------------------------------------------------------
//  CMWnd_ResizePad
// -------------------------------------------------------------------
class CMWnd_ResizePad : public CMWnd
{
	MRTC_DECLARE;

public:
	CMWnd_ResizePad();
	CMWnd_ResizePad(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID);

	virtual void OnResizePadPress(const CMWnd_Message* _pMsg);

	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
};

// -------------------------------------------------------------------
//  CMWnd_Window
// -------------------------------------------------------------------

// Styles shared with CMWnd_Caption:
#define WSTYLE_WND_CAPTIONSHAREMASK (WSTYLE_CAPTN_TOOLWINDOW | WSTYLE_CAPTN_CLOSEBTN | WSTYLE_CAPTN_MAXBTN | WSTYLE_CAPTN_MINBTN)

#define WSTYLE_WND_TOOLWINDOW	WSTYLE_CAPTN_TOOLWINDOW
#define WSTYLE_WND_CLOSEBTN		WSTYLE_CAPTN_CLOSEBTN
#define WSTYLE_WND_MAXBTN		WSTYLE_CAPTN_MAXBTN
#define WSTYLE_WND_MINBTN		WSTYLE_CAPTN_MINBTN

// Private styles:
#define WSTYLE_WND_CAPTION		0x1000
#define WSTYLE_WND_NOFRAME		0x2000
#define WSTYLE_WND_THINFRAME	0x4000

// -------------------------------------------------------------------
class CMWnd_Window : public CMWnd
{
	MRTC_DECLARE;

protected:
	CStr m_WndCaptionClass;
	CStr m_WndClientClass;
	CStr m_WndBorderClass;

	CMWnd_Caption* m_pWndCaption;
	CMWnd_Client* m_pWndClient;
	CMWnd_Border* m_pWndBorder;

	spCMWnd_View m_spClientView;

	CStr m_Caption;

	// Client creation parameters.
	int m_ClientStyle;
	CPnt m_ClientSize;

	void Clear();
protected:
	virtual void DoCreate(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID);
	virtual void DoDestroy();

public:
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, const CStr& _Caption = "");
	
	CMWnd_Window();
	CMWnd_Window(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, const CStr& _Caption = "");

	virtual void AddChild(spCMWnd _spWnd);
	virtual CMWnd* GetItem(const CStr& _ID);

	virtual int GetWidth();
	virtual int GetHeight();
	virtual void SetView(spCMWnd_View _spView);
	virtual void OnMove();
	virtual void OnParentMove();
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
};

typedef TPtr<CMWnd_Window> spCMWnd_Window;

// -------------------------------------------------------------------
//  CMWnd_Desktop
// -------------------------------------------------------------------
class CMWnd_Desktop : public CMWnd
{
	MRTC_DECLARE;

public:
	CMWnd_Desktop();
	CMWnd_Desktop(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID);

	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
};

typedef TPtr<CMWnd_Desktop> spCMWnd_Desktop;

// -------------------------------------------------------------------
//  CMWnd_Button
// -------------------------------------------------------------------
#define WSTYLE_BTNIMG_CENTERX	1
#define WSTYLE_BTNIMG_CENTERY	2
#define WSTYLE_BTNIMG_STRETCHX	4
#define WSTYLE_BTNIMG_STRETCHY	8
#define WSTYLE_BTNIMG_STRETCHFITX	16
#define WSTYLE_BTNIMG_STRETCHFITY	32

class CMWnd_Button : public CMWnd
{
	MRTC_DECLARE;

public:
	CStr m_Text;
protected:
	CPnt m_TextPos;
	int m_TextStyle;
	int m_TextColorM;
	int m_TextColorH;
	int m_TextColorD;

	int m_ImageStyle;
	CStr m_Image;
	CPnt m_ImagePos;
	CPnt m_ImageSize;
	
	bool m_Transparent;

	void Clear();
public:
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void PreEvalKeys(CMWnd_Param* _pParam);
	virtual void PreCreate(CMWnd_Param* _pParam);
	virtual void Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID = "", const CStr& _Text = "",
		int _TextStyle = WSTYLE_TEXT_CENTER | WSTYLE_TEXT_CENTERY | WSTYLE_TEXT_CUTOUT | WSTYLE_TEXT_SHADOW | WSTYLE_TEXT_HIGHLIGHT, 
		const CStr& _Image = "", const CPnt& _ImagePos = CPnt(0,0), const CPnt& _ImageSize = CPnt(0,0), int _ImageStyle = 0);

	CMWnd_Button();
	CMWnd_Button(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID = "", const CStr& _Text = "",
		int _TextStyle = WSTYLE_TEXT_CENTER | WSTYLE_TEXT_CENTERY | WSTYLE_TEXT_CUTOUT | WSTYLE_TEXT_SHADOW | WSTYLE_TEXT_HIGHLIGHT, 
		const CStr& _Image = "", const CPnt& _ImagePos = CPnt(0,0), const CPnt& _ImageSize = CPnt(0,0), int _ImageStyle = 0);

	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
};

typedef TPtr<CMWnd_Button> spCMWnd_Button;

// -------------------------------------------------------------------
//  CMWnd_DragbarX
// -------------------------------------------------------------------
#define WMSG_DRAG_SETWINDOW		0x0100
#define WMSG_DRAG_SETPOS		0x0101

class CMWnd_DragbarX : public CMWnd
{
	MRTC_DECLARE;

protected:
	CMWnd* m_pWndDragBtn;
	CMWnd* m_pWndIncBtn;
	CMWnd* m_pWndDecBtn;
	CMWnd* m_pWndIncArea;
	CMWnd* m_pWndDecArea;

public:
	void GetDragBtnInfo(int _w, int& wbtn, int& xbtn);
	void SetDragBarPos(fp32 _Pos);
protected:
	void PostViewPosMsg();

	fp32 m_BarLen;

	fp32 m_RangeMin;
	fp32 m_RangeMax;
	fp32 m_ViewPos;
	fp32 m_ViewSize;		// For proportional bar, -1 if dragbox

	CStr m_IncBtnArw;	// Button images
	CStr m_DecBtnArw;

	void Clear();
	virtual void PreCreate(CMWnd_Param* _pParam);
	virtual void DoCreate(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID);

public:
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);
	virtual void Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID = "", 
		fp32 _RangeMin = 0.0f, fp32 _RangeMax = 1.0f, fp32 _ViewPos = 0.0f, fp32 _ViewSize = -1.0f);

	CMWnd_DragbarX();
	CMWnd_DragbarX(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID = "", 
		fp32 _RangeMin = 0.0f, fp32 _RangeMax = 1.0f, fp32 _ViewPos = 0.0f, fp32 _ViewSize = -1.0f);

	virtual void OnMove();
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual aint OnMessage(const CMWnd_Message* _pMsg);

	virtual void Drag_SetWindow(fp32 _Min, fp32 _Max, fp32 _ViewSize);
	virtual void Drag_SetPos(fp32 _Pos);
	virtual fp32 Drag_GetPos();
};

typedef TPtr<CMWnd_DragbarX> spCMWnd_DragbarX;

// -------------------------------------------------------------------
//  CMWnd_DragbarY
// -------------------------------------------------------------------
class CMWnd_DragbarY : public CMWnd_DragbarX
{
	MRTC_DECLARE;

	void Clear();
public:
	CMWnd_DragbarY();
	CMWnd_DragbarY(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID = "", 
		fp32 _RangeMin = 0.0f, fp32 _RangeMax = 1.0f, fp32 _ViewPos = 0.0f, fp32 _ViewSize = -1.0f);

	virtual void OnMove();
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
};

// -------------------------------------------------------------------
//  CMWnd_List
// -------------------------------------------------------------------
#define CMWND_LISTITEM_SELECTED 1

class CMWnd_ListItem : public CReferenceCount
{
public:
	CStr m_Text;
	int m_Flags;
	int m_Data;
	int m_iImage;

	CMWnd_ListItem()
	{
		m_Flags = 0;
		m_Data = 0;
		m_iImage = -1;
	}

	CMWnd_ListItem(const CStr& _Text, int _Data, int _iImage)
	{
		m_Flags = 0;
		m_Text = _Text;
		m_Data = _Data;
		m_iImage = _iImage;
	}
};

typedef TPtr<CMWnd_ListItem> spCMWnd_ListItem;

// -------------------------------------------------------------------
class CMWnd_ListClient;

#define WSTYLE_LIST_MULTISELECT	0x10000
#define WSTYLE_LIST_AUTOWIDTH	0x20000

class CMWnd_List : public CMWnd_Client
{
	MRTC_DECLARE;

	TArray<spCMWnd_ListItem> m_lspItems;
	TArray<CStr> m_ImageNames;

	int m_nItemPerCol;
	int m_nColumns;
	int m_MinListWidth;
	int m_ListWidth;
	bool m_bUpdate;

protected:
	virtual void DoCreate(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID);
	virtual spCMWnd CreateClient();
	virtual void EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value);

	void Update();
	void Clear();

public:
	CMWnd_List();
	CMWnd_List(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, const CPnt& _Size);

	virtual void PreCreate(CMWnd_Param* _pParam);
	virtual void OnMove();
	virtual void OnRefresh();

	int AddImage(const CStr& _Name);
	void RemoveImage(int _iImage);

	int GetListItemCount();
	int AddListItem(const CStr& _Text, int _Data = 0, int _iImage = 0);
	int InsertListItem(int _iPos, const CStr& _Text, int _Data = 0, int _iImage = -1);
	void RemoveListItem(int _iItem);
	void RemoveAllList();

	void SetListItem(int _iItem, const CStr& _Text, int _Data, int _iImage);
	void SetListItem(int _iItem, const CStr& _Text, int _Data);
	void SetListItem(int _iItem, const CStr& _Text);
	void SetListItemData(int _iItem, int _Data);
	void SetListItemImage(int _iItem, int _iImage);
	CStr GetListItem(int _iItem);
	int GetListItemData(int _iItem);

	int GetSelected(int _iLast = -1);
	void DeselectAll();

	virtual void OnTouchItem(int _iItem, int _bSelected);
	virtual void OnAction(int _iItem);

	friend class CMWnd_ListClient;
};

typedef TPtr<CMWnd_List> spCMWnd_List;

// -------------------------------------------------------------------
#define CMWND_LIST_ITEMHEIGHT 12
#define CMWND_LIST_XPADDING 0
#define CMWND_LIST_YPADDING 1

class CMWnd_ListClient : public CMWnd_Client
{
	CMWnd_List* m_pList;

	void RenderColumn(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client, const CPnt& _Pos, int _iItem, int _nItems, int _Width);
	int Pos2Item(const CPnt& _Pos);

public:
	CMWnd_ListClient(CMWnd_List* _pList);

	virtual aint OnMessage(const CMWnd_Message* _pMsg);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
};

class CMWnd_ModList3;

// -------------------------------------------------------------------
//  CMWnd_ModListItem3
// -------------------------------------------------------------------
class CMWnd_ModListItem3 : public CMWnd_Button
{
public:

	CMWnd_ModListItem3()
	{
		m_List = NULL;
		m_NumItem = -1;
		m_dwData = 0;
		m_InitialPeriodValue = (fp32)(MRTC_RAND()%1000);
		m_Transparent = true;
	}

	uint32 m_dwData;
	CStr m_strData;

	// List

	CMWnd_ModList3 * m_List;
	int m_NumItem;
	CStr m_ItemText;
	
	// Pulsing effect

	fp32 m_InitialPeriodValue;

	// Window functions

	virtual void OnPaintFocus(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual int OnPressed(const CMWnd_Message* _pMsg);
	virtual void OnSetFocus();
};

// -------------------------------------------------------------------
//  CMWnd_ModList3
// -------------------------------------------------------------------
class CMWnd_ModList3 : public CMWnd
{
public:
		
	CMWnd_ModList3()
	{
		m_bDrawControlFrame = true;
		m_iVisibleItems = 0;
		m_iItemHeight = 0;
		m_iItemWidth = 0;
		m_TextStyle = WSTYLE_TEXT_CENTER | WSTYLE_TEXT_CENTERY;
	}

	// Graphics

	bool m_bDrawControlFrame;
	int m_iVisibleItems;
	int m_iItemHeight;
	int m_iItemWidth;
	int m_TextStyle;

	// Items

	TArray<CStr> m_lItems;
	TArray<CMWnd_ModListItem3 *> m_lButton;

	// Creation & proportions

	void vSetProportions(int nVisibleItems);
	void vRecreateList(int numItems);

	// Set

	void vSetList(TArray<CStr>& _lItems);
	void vSetItemData(int nItem, uint32 dwData);
	void vSetItemDataText(int nItem, const CStr& strData);
	void vSetItemText();
	void vSetInitialFocus();
	void vSetFocus(int i);

	// Messsages

	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
	virtual void vOnLeftClick(CMWnd_ModListItem3 * pItem);
	virtual void vOnSelChange(CMWnd_ModListItem3 * pItem);
};

// -------------------------------------------------------------------
//  CMWnd_Starview
// -------------------------------------------------------------------
class CMWnd_Starview : public CMWnd_View
{
	MRTC_DECLARE;

public:
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client);
};

typedef TPtr<CMWnd_Starview> spCMWnd_Starview;

// -------------------------------------------------------------------
spCMWnd CMWnd_Create(CKeyContainerNode* _pRsrc, const CStr& _Name, spCMWnd _spWndParent = NULL);
spCMWnd CMWnd_Create(CRegistry* _pRsrc, const CStr& _Name, spCMWnd _spWndParent = NULL);

// -------------------------------------------------------------------
#define XWND_MB_OK			0
#define XWND_MB_OKCANCEL	1
#define XWND_MB_YESNO		2

#define XWND_MB_ICONSTOP		0x0100
#define XWND_MB_ICONINFO		0x0200
#define XWND_MB_ICONQUESTION	0x0400

int CMWnd_MessageBox(const CStr& _Caption, const CStr& _Message, int _Style = XWND_MB_OK);

// -------------------------------------------------------------------


#endif // __INC_MWINOWS
