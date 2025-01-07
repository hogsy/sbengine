#include "PCH.h"
#include "MWin32Util.h"
#include "commctrl.h"

// -------------------------------------------------------------------
//  CWin32_ListBoxLog
// -------------------------------------------------------------------
CWin32_ListBoxLog::CWin32_ListBoxLog(HWND _hItem, const char *_pOldLog)
{
	m_MainThread = MRTC_SystemInfo::OS_GetThreadID();
	m_OldLog = _pOldLog;
	m_hItem = _hItem;
	m_pOldLog = NULL;
	MACRO_GetRegisterObject(CReferenceCount, pLog, m_OldLog);
	m_spOldLog = pLog;
	if (m_spOldLog)
	{
		MRTC_GOM()->UnregisterObject(NULL, m_OldLog);
		m_pOldLog = TDynamicCast<ILogFile>(pLog);
	}
}

CWin32_ListBoxLog::~CWin32_ListBoxLog()
{
	if (m_spOldLog)
	{
		MRTC_GOM()->RegisterObject(m_spOldLog, m_OldLog);
	}
}

void CWin32_ListBoxLog::Create(CStr _FileName, bool _bAppend)
{
	if (m_pOldLog) m_pOldLog->Create(_FileName, _bAppend);
}

void CWin32_ListBoxLog::CommitDeferredLog()
{
	const int MaxPerIter = 16;
	CStr lLog[MaxPerIter];

	while(1)
	{
		int nLog = 0;
		{
			M_LOCK(m_Lock);
			CLogStrLink* pLogStr = m_spDeferredLog;
			if (pLogStr)
				while(pLogStr->m_spNext != NULL)
					pLogStr = pLogStr->m_spNext;

			while(pLogStr)
			{
				lLog[nLog++] = pLogStr->m_Str;

				pLogStr = pLogStr->m_pPrev;
				if (pLogStr)
					pLogStr->m_spNext = NULL;

				if (nLog == MaxPerIter)
					break;
			}

			if (!pLogStr)
				m_spDeferredLog = NULL;
		}

		for(int i = 0; i < nLog; i++)
			InternalLog(lLog[i]);

		if (!nLog)
			break;
	}
}

//MAde virtual, this comment is to force a fresh compile since it'll link incorrectly otherwise
void CWin32_ListBoxLog::InternalLog(const CStr& _Str)
{
	if (m_pOldLog) 
		m_pOldLog->Log(_Str);

	if (m_hItem)
	{
		int Indx = SendMessage(m_hItem, LB_ADDSTRING, 0, (LPARAM)_Str.Str());
		if ((Indx != LB_ERR) && (Indx != LB_ERRSPACE)) 
			SendMessage(m_hItem, LB_SETTOPINDEX, Indx, 0);
	}
}

void CWin32_ListBoxLog::Log(const CStr& _s)
{
	if (MRTC_SystemInfo::OS_GetThreadID() != m_MainThread)
	{
		M_LOCK(m_Lock);
		spCLogStrLink spNew = MNew(CLogStrLink);
		if (!spNew)
			return;

		spNew->m_Str = _s;
		spNew->m_spNext = m_spDeferredLog;
		if (m_spDeferredLog != NULL)
			m_spDeferredLog->m_pPrev = spNew;
		m_spDeferredLog = spNew;
	}
	else
	{
		InternalLog(_s);
	}

//	UpdateWindow(m_hItem);

//	CWin32_Window::ProcessMessages();
}

void CWin32_ListBoxLog::Log(const char* _pStr)
{
	CWin32_ListBoxLog::Log(CStr(_pStr));
}

void CWin32_ListBoxLog::SetFileName(const CStr& _s, bool _bAppend)
{
	if (m_pOldLog) m_pOldLog->SetFileName(_s, _bAppend);
}

// -------------------------------------------------------------------
//  CWin32_LogWindow
// -------------------------------------------------------------------
CWin32_LogWindow::CWin32_LogWindow()
{
	m_hListBoxBrush = NULL;
	m_hItem = NULL;
//	m_BkColor = 0x00605858;		// 00bbggrr
//	m_TextColor = 0x00ffe0e0;
	m_BkColor = 0x00808080;		// 00bbggrr
	m_TextColor = 0x00000000;
}

void CWin32_LogWindow::Create(int _DlgID, int _ListBoxID)
{
	m_ListBoxID = _ListBoxID;
	CWin32_Dialog::Create(_DlgID);
	m_hItem = GetDlgItem(m_hWnd, _ListBoxID);
}

void CWin32_LogWindow::PreCreate(CWin32_CreateStruct& _Params)
{
	_Params.m_ExWS |= WS_EX_APPWINDOW;
//	_Params.m_ExWS &= ~WS_EX_TOOLWINDOW;
//	_Params.m_WS &= ~WS_APPWINDOW;
}

void CWin32_LogWindow::OnInitInstance()
{
	CWin32_Dialog::OnInitInstance();

	m_hListBoxBrush = CreateSolidBrush(m_BkColor);
	SendMessage(m_hItem, LB_SETHORIZONTALEXTENT , 400, 0);
}

aint CWin32_LogWindow::OnMessage(unsigned msg, mint wparam, mint lparam)
{
	switch(msg)
	{
	case WM_SIZE :
		{
			RECT Rect;
			GetClientRect(m_hWnd, &Rect);

//			HWND hItem = GetDlgItem(m_hWnd, IDC_LIST1);
			HWND hItem = GetDlgItem(m_hWnd, m_ListBoxID);
		//	SetWindowPos(hItem, NULL, 3, 102, Rect.right-6, Rect.bottom-102-15, SWP_NOZORDER);
			int x0 = 0;
			SetWindowPos(hItem, NULL, x0, 0, Rect.right-0-x0, Rect.bottom-0, SWP_NOZORDER);
			return CWin32_Dialog::OnMessage(msg, wparam, lparam);
		}
	case WM_CTLCOLORLISTBOX :
		{
			
			HDC hdcLB = (HDC) wparam;   // handle of list box display context 
			HWND hwndLB = (HWND) lparam; // handle of list box 

//			HWND hItem = GetDlgItem(m_hWnd, IDC_LIST1);
			if ((m_hItem == hwndLB) && hdcLB)
			{
				SetBkColor(hdcLB, m_BkColor);
				SetTextColor(hdcLB, m_TextColor);
				return (aint)m_hListBoxBrush;
			}
			else
				return 0;
		}

	default :
		return CWin32_Dialog::OnMessage(msg, wparam, lparam);
	}
}

void CWin32_LogWindow::OnDestroy()
{
	DeleteObject(m_hListBoxBrush);
	m_hListBoxBrush = NULL;
	CWin32_Dialog::OnDestroy();
}

void CWin32_LogWindow::Log(const CStr _s)
{
	CStr s;
	s.Capture(_s.Ansi().Str());

	if (m_hWnd)
	{
		int Indx = SendMessage(m_hItem, LB_ADDSTRING, 0, (mint)(const char*)(_s));
		if ((Indx != LB_ERR) && (Indx != LB_ERRSPACE)) SendMessage(m_hItem, LB_SETTOPINDEX, Indx, 0);
	}

//	UpdateWindow(m_hItem);
}

typedef TPtr<CWin32_LogWindow> spCWin32_LogWindow;

// -------------------------------------------------------------------
//  CLogFileWindow
// -------------------------------------------------------------------
CLogFileWindow::CLogFileWindow(int _DlgID, int _ListBoxID)
{
	m_pOldLog = NULL;
	MACRO_GetRegisterObject(CReferenceCount, pLog, "SYSTEM.LOG");
	m_spOldLog = pLog;
	if (m_spOldLog)
	{
		MRTC_GOM()->UnregisterObject(m_spOldLog, "SYSTEM.LOG");
		m_pOldLog = TDynamicCast<ILogFile>(pLog);
	}

	m_LogWin.Create(_DlgID, _ListBoxID);
	m_LogWin.SetPosition(60,60);
}

CLogFileWindow::~CLogFileWindow()
{
	m_LogWin.Destroy();
	if (m_spOldLog)
	{
		MRTC_GOM()->RegisterObject(m_spOldLog, "SYSTEM.LOG");
	}
}

void CLogFileWindow::Create(CStr _FileName, bool _bAppend)
{
	if (m_pOldLog) m_pOldLog->Create(_FileName, _bAppend);
//	m_LogWin.ProcessMessages();
}

void CLogFileWindow::Log(const CStr& _s)
{
	if (m_pOldLog) m_pOldLog->Log(_s);
	m_LogWin.Log(_s);
//	m_LogWin.ProcessMessages();
}

void CLogFileWindow::SetFileName(const CStr& _s, bool _bAppend)
{
	if (m_pOldLog) m_pOldLog->SetFileName(_s, _bAppend);
//	m_LogWin.ProcessMessages();
}


// -------------------------------------------------------------------
//  CWin32_ProgressBar - Progress interface for win32 progress-bar controls.
// -------------------------------------------------------------------
void CWin32_ProgressBar::MessagePump()
{
	if (m_hThreadOwner != MRTC_SystemInfo::OS_GetThreadID())
		return;

	if (!m_bPumpMsgs) return;
	MSG	msg;
	while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	};
}

CWin32_ProgressBar::CWin32_ProgressBar(HWND _hItem, bool _bPumpMsgs)
{
	m_pNextProgress = NULL;
	m_bPumpMsgs = _bPumpMsgs;
	m_hItem = _hItem;
	m_ProgressLimit = 1;
	m_ProgressCount = 0;

	m_hThreadOwner = MRTC_SystemInfo::OS_GetThreadID();
}

void CWin32_ProgressBar::Push(const char* _pLevelName)
{
	if (m_pNextProgress) m_pNextProgress->Push(_pLevelName);
}

void CWin32_ProgressBar::Pop()
{
	if (m_pNextProgress) m_pNextProgress->Pop();
}

void CWin32_ProgressBar::SetProgress(fp32 _p, const char* _pTaskName)
{
	if (m_hItem) PostMessage(m_hItem, PBM_SETPOS, _p*100.0, 0);
//	UpdateWindow(m_hItem);
	MessagePump();
	if (m_pNextProgress) m_pNextProgress->SetProgress(_p, _pTaskName);
}

void CWin32_ProgressBar::SetProgressText(const char* _pTaskName)
{
	if (m_pNextProgress) m_pNextProgress->SetProgressText(_pTaskName);
}

void CWin32_ProgressBar::InitProgressCount(int _Count, const char* _pTaskName)
{
	m_ProgressLimit = _Count;
	m_ProgressCount = 0;
	SetProgress(0);
//	UpdateWindow(m_hItem);
	MessagePump();
	if (m_pNextProgress) m_pNextProgress->InitProgressCount(_Count, _pTaskName);
}

void CWin32_ProgressBar::IncProgress(const char* _pTaskName)
{
	if (m_ProgressCount < m_ProgressLimit) m_ProgressCount++;
	if (m_ProgressLimit) SetProgress(fp32(m_ProgressCount)/fp32(m_ProgressLimit));
	if (m_pNextProgress) m_pNextProgress->IncProgress(_pTaskName);
}

#include "MWin32Registry.cpp"
