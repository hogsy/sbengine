#include "PCH.h"
#include "MWin32.h"

// -------------------------------------------------------------------
TArray<CWin32_WinInfo> CWin32_Window::ms_lWindows;
CWin32_Window* CWin32_Window::ms_pCreateObj = NULL;

void CWin32_Window::RegisterWindowClass()
{
	WNDCLASS windowclass;
	HANDLE hIcon = NULL;
	// Setup and register window class
	WNDCLASS* wc = &windowclass;
	wc->style = CS_HREDRAW | CS_VREDRAW;
	wc->lpfnWndProc = WinProc;
	wc->cbClsExtra = 0;
	wc->cbWndExtra = sizeof( DWORD );
	wc->hInstance = (HINSTANCE__*)m_pWinInfo->m_hInstance;
	wc->hIcon = NULL;
	wc->hCursor = NULL;
	wc->hbrBackground = 0;
	wc->lpszMenuName = NULL;

//	CStr Class = CStrF("%s%s", MRTC_ClassName(), "_Class");
//	CStr Class = typeid(*this).name();
	m_WindowClassName = MRTC_ClassName();
//	CStr Class = MRTC_GetRuntimeClass()->m_ClassName;
//ConOutL("(CWin32_Window::RegisterWindowClass) Classname = " + Class);
//	CStr Class = CStr(MRTC_ClassName()).Del(0, 6);
	wc->lpszClassName = m_WindowClassName;
//	wc->lpszClassName = "FNISS";

	if (!RegisterClass(wc)) Win32Err("Win32_RegisterClass");
};

void CWin32_Window::AddWindow(HWND _hWnd, CWin32_Window* _pWnd)
{
	CWin32_WinInfo Inf;
	Inf.m_hWnd = _hWnd;
	Inf.m_pWnd = _pWnd;
	ms_lWindows.Add(Inf);
}

void CWin32_Window::RemoveWindow(CWin32_Window* _pWnd)
{
	for(int i = 0; i < ms_lWindows.Len(); i++)
		if (ms_lWindows[i].m_pWnd == _pWnd)
		{
			ms_lWindows.Del(i);
			return;
		}
	Error_static("CWin32_Window::RemoveWindow", "Window not found.");
}

CWin32_Window* CWin32_Window::FindWindow(HWND _hWnd)
{
	CWin32_Window* pWnd = NULL;
	for(int i = 0; i < ms_lWindows.Len(); i++)
		if (ms_lWindows[i].m_hWnd == _hWnd)
		{
			pWnd = ms_lWindows[i].m_pWnd;
			break;
		}

	if (ms_pCreateObj)
	{
		AddWindow(_hWnd, ms_pCreateObj);
		pWnd = ms_pCreateObj;
		pWnd->m_hWnd = _hWnd;
		ms_pCreateObj = NULL;
	}

	return pWnd;
}

LRESULT CALLBACK CWin32_Window::WinProc(HWND hwnd, unsigned msg, WPARAM wparam, LPARAM lparam)
{
	CWin32_Window* pWnd = FindWindow(hwnd);
	if (pWnd) return pWnd->OnMessage(msg, wparam, lparam);
	return 0;
}


CWin32_Window::CWin32_Window()
{
	bNoDestroy = false;
	m_hWnd = NULL;
	MACRO_GetRegisterObject(CWin32AppInfo, pWinInfo, "SYSTEM.WIN32APPINFO");
	if (!pWinInfo) Error("-", "No win-app info.");
	m_pWinInfo = pWinInfo;
}

CWin32_Window::~CWin32_Window()
{
	Destroy();
}

void CWin32_Window::Create(CWin32_CreateStruct _Params)
{
	RegisterWindowClass();
	Destroy();
	ms_pCreateObj = this;
	m_hWnd = CreateWindowExA(
		_Params.m_ExWS,
//		"FNISS",
		m_WindowClassName,
//		"HIRR!",
		_Params.m_pCaption,
		_Params.m_WS,
		_Params.x, _Params.y, _Params.w, _Params.h, 
		(HWND__*)_Params.m_hParent,
		(HMENU__*)_Params.m_hMenu,
		(HINSTANCE__*)_Params.m_hInstance,
		_Params.m_pContext);
	if (!m_hWnd)
	{
		Error("Create", "Unable to create window.");
	}
	ShowWindow(m_hWnd, SW_NORMAL);
}

void CWin32_Window::Create(CRct _Pos, int _WS, int _ExWS, HWND _hWndParent)
{
	Destroy();
//	CStr Class = CStrF("%s%s", MRTC_ClassName(), "_Class");
	CWin32_CreateStruct Params = 
	{
		_ExWS, 
		m_WindowClassName,
		NULL,
		_WS,
		_Pos.p0.x, _Pos.p0.y, _Pos.GetWidth(), _Pos.GetHeight(),
		_hWndParent,				/* parent window */
		NULL,						/* menu handle */
		m_pWinInfo->m_hInstance,	/* program handle */
		NULL						/* create parms */
	};
	Create(Params);
}

void CWin32_Window::CreateFromExisting(HWND _hWnd)
{

	bNoDestroy = true;

	m_hWnd = _hWnd;

}

void CWin32_Window::Destroy()
{
	if (m_hWnd)
	{
		OnDestroy();

		if (!bNoDestroy)
			DestroyWindow(m_hWnd);

		RemoveWindow(this);
		m_hWnd = NULL;
	}
}

void CWin32_Window::SetPosition(int _x, int _y)
{
	if (m_hWnd) SetWindowPos(m_hWnd, NULL, _x, _y, 0, 0, SWP_NOSIZE);
}

void CWin32_Window::SetSize(int _w, int _h)
{
	if (m_hWnd) SetWindowPos(m_hWnd, NULL, 0, 0, _w, _h, SWP_NOMOVE);
}

CPnt CWin32_Window::GetPosition()
{
	RECT Rect;
	if (m_hWnd)
	{
		GetWindowRect(m_hWnd, &Rect);
		return CPnt(Rect.left, Rect.top);
	}
	else
		return CPnt(0,0);
}

CPnt CWin32_Window::GetSize()
{
	RECT Rect;
	if (m_hWnd)
	{
		GetWindowRect(m_hWnd, &Rect);
		return CPnt(Rect.right - Rect.left, Rect.bottom  - Rect.top);
	}
	else
		return CPnt(0,0);
}

void CWin32_Window::ProcessMessages()
{
	MSG	msg;
	while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
	{
/*		if (msg.message==WM_QUIT) 
		{
			PostQuitMessage(msg.wParam);
			return;
		}*/
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	};
}

void CWin32_Window::PreCreate(CWin32_CreateStruct& _Params)
{
}

void CWin32_Window::OnInitInstance()
{
}

aint CWin32_Window::OnMessage(unsigned msg, mint wparam, mint lparam)
{
	switch(msg)
	{
/*	case WM_NCCREATE :
*/	case WM_CREATE :
		{
			OnInitInstance();
			return DefWindowProc(m_hWnd, msg, wparam, lparam);
		}

	case WM_CLOSE :
		{
			Destroy();
			return DefWindowProc(m_hWnd, msg, wparam, lparam);
		}
	case WM_DESTROY : 
		{
//			Destroy();
			return DefWindowProc(m_hWnd, msg, wparam, lparam);
//			PostQuitMessage( 0 );
//			return TRUE;
		}

	default :
		return DefWindowProc(m_hWnd, msg, wparam, lparam);
	}
}

void CWin32_Window::OnDestroy()
{
/*	if (m_hWnd)
	{
		OnDestroy();
		DestroyWindow(m_hWnd);
		RemoveWindow(this);
		m_hWnd = NULL;
	}*/
}

// -------------------------------------------------------------------
BOOL CALLBACK CWin32_Dialog::DialogProc(HWND hwnd,  UINT msg, WPARAM wparam, LPARAM lparam)
{
	CWin32_Window* pWnd = TDynamicCast<CWin32_Dialog>(FindWindow(hwnd));

	if (pWnd) return pWnd->OnMessage(msg, wparam, lparam);
	return 0;
}


void CWin32_Dialog::Create(int _DlgID)
{
	Destroy();
	ms_pCreateObj = this;
	m_hWnd = CreateDialog((HINSTANCE__*)m_pWinInfo->m_hInstance, MAKEINTRESOURCE(_DlgID), 0, (DLGPROC) DialogProc);
	if (!m_hWnd)
	{
		RemoveWindow(this);
		Error("Create", "Unable to create dialog.");
	}
	ShowWindow(m_hWnd, SW_NORMAL);
}

aint CWin32_Dialog::OnMessage(unsigned msg, mint wparam, mint lparam)
{
	switch(msg)
	{
		case WM_INITDIALOG :
			{
				OnInitInstance();
//				return DefDlgProc(m_hWnd, msg, wparam, lparam);
				return FALSE;
			}
		case WM_CLOSE: 
			{
				// Destroy the application window.
				EndDialog(m_hWnd, 0);
				Destroy();
				PostQuitMessage(0);
				return FALSE;
			}
		case WM_DESTROY: 
			{
//				Destroy();
				return FALSE;
//				return DefDlgProc(m_hWnd, msg, wparam, lparam);
			}

//		default :
//			return DefDlgProc(m_hWnd, msg, wparam, lparam);
//			return CWin32_Window::OnMessage(msg, wparam, lparam);

	}
	return FALSE;
}

bool CWin32_Dialog::ContinueModal()
{
	return (m_hWnd != NULL);
}

int CWin32_Dialog::DoModal()
{
	while(ContinueModal())
	{
		ProcessMessages();
		Sleep(20);
	}
	Destroy();
	return 0;
}

