
#ifndef _INC_MWIN32
#define _INC_MWIN32

#include "../../MOS.h"
#include <windows.h>

// -------------------------------------------------------------------
class CWin32_CreateStruct
{
public:
	int m_ExWS;
	char* m_pClass;
	char* m_pCaption;
	int m_WS;
	int x;
	int y;
	int w;
	int h;
	void* m_hParent;
	void* m_hMenu;
	void* m_hInstance;
	void* m_pContext;
};

class CWin32_Window;

class CWin32_WinInfo
{
public:
	void* m_hWnd;
	CWin32_Window* m_pWnd;
};

// -------------------------------------------------------------------
class CWin32_Window : public CReferenceCount
{
protected:
	static TArray<CWin32_WinInfo> ms_lWindows;
	static CWin32_Window* ms_pCreateObj;

	CWin32AppInfo* m_pWinInfo;

	CStr m_WindowClassName;
	HWND m_hWnd;
	bool bNoDestroy;
	static void AddWindow(HWND _hWnd, CWin32_Window* _pWnd);
	static void RemoveWindow(CWin32_Window* _pWnd);
	static CWin32_Window* FindWindow(HWND _hWnd);
	static LRESULT CALLBACK WinProc(HWND hwnd, unsigned msg, WPARAM wparam, LPARAM lparam);

public:
	CWin32_Window();
	~CWin32_Window();

	virtual HWND GethWnd() { return m_hWnd; };

	virtual void CreateFromExisting(HWND _hWnd);
	virtual void RegisterWindowClass();

	virtual void Create(CWin32_CreateStruct _Params);
	virtual void Create(CRct _Pos, int _WS = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE, int _ExWS = WS_EX_TOPMOST | WS_EX_APPWINDOW, HWND _hWndParent = NULL);
	virtual void Destroy();

	virtual void SetPosition(int _x, int _y);
	virtual void SetSize(int _w, int _h);
	virtual CPnt GetPosition();
	virtual CPnt GetSize();

	static void ProcessMessages();

	virtual void PreCreate(CWin32_CreateStruct& _Params);

	virtual void OnInitInstance();
	virtual aint OnMessage(unsigned msg, mint wparam, mint lparam);
	virtual void OnDestroy();
};

// -------------------------------------------------------------------
class CWin32_Dialog : public CWin32_Window
{
protected:
	static BOOL CALLBACK DialogProc(HWND hwnd,  UINT Message, WPARAM wParam, LPARAM lParam);

public:
	void Create(int _DlgID);
	virtual aint OnMessage(unsigned msg, mint wparam, mint lparam);
	virtual bool ContinueModal();
	virtual int DoModal();
};

#endif // _INC_MWIN32
