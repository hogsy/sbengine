#ifndef _INC_MWIN32UTIL
#define _INC_MWIN32UTIL

#include "MWin32.h"

// -------------------------------------------------------------------
//  CWin32_ListBoxLog - Logfile output for win32 listbox controls
// -------------------------------------------------------------------
class CLogStrLink : public CReferenceCount
{
public:
	CStr m_Str;
	CLogStrLink* m_pPrev;
	TPtr<CLogStrLink> m_spNext;

	CLogStrLink()
	{
		m_pPrev = NULL;
	}
};

typedef TPtr<CLogStrLink> spCLogStrLink;

class CWin32_ListBoxLog : public ILogFile
{
protected:
	spCReferenceCount m_spOldLog;
	ILogFile* m_pOldLog;
	HWND m_hItem;
	CStr m_OldLog;
	void *m_MainThread;

	MRTC_CriticalSection m_Lock;
	spCLogStrLink m_spDeferredLog;

	virtual void InternalLog(const CStr& _Str);

public:
	CWin32_ListBoxLog(HWND _hItem, const char *_pOldLog);
	~CWin32_ListBoxLog();

	virtual void Create(CStr _FileName, bool _bAppend = false);
	virtual void CommitDeferredLog();
	virtual void Log(const CStr& _s);
	virtual void Log(const char* _pStr);
	virtual void SetFileName(const CStr& _s, bool _bAppend = false);
};

// -------------------------------------------------------------------
//  CWin32_LogWindow - A listbox.
// -------------------------------------------------------------------
class CWin32_LogWindow : public CWin32_Dialog
{
	HBRUSH m_hListBoxBrush;
	HWND m_hItem;
	int m_BkColor;
	int m_TextColor;
	int m_ListBoxID;

public:
	CWin32_LogWindow();
	void Create(int _DlgID, int _ListBoxID);
	virtual void PreCreate(CWin32_CreateStruct& _Params);

	virtual void OnInitInstance();
	virtual aint OnMessage(unsigned msg, mint wparam, mint lparam);
	virtual void OnDestroy();

	virtual void Log(const CStr _s);
};

typedef TPtr<CWin32_LogWindow> spCWin32_LogWindow;

// -------------------------------------------------------------------
//  CLogFileWindow - Window with listbox which implements the logfile interface.
// -------------------------------------------------------------------
class CLogFileWindow : public ILogFile
{
	spCReferenceCount m_spOldLog;
	ILogFile* m_pOldLog;
	CWin32_LogWindow m_LogWin;

public:
	CLogFileWindow(int _DlgID, int _ListBoxID);
	~CLogFileWindow();

	virtual void Create(CStr _FileName, bool _bAppend = false);
	virtual void Log(const CStr& _s);
	virtual void SetFileName(const CStr& _s, bool _bAppend = false);
};

typedef TPtr<CLogFileWindow> spCLogFileWindow;

// -------------------------------------------------------------------
//  CWin32_ProgressBar - Progress interface for win32 progress-bar controls.
//
//  !! Remember to clear m_pNextProgress when you inherit this class. !!
//
// -------------------------------------------------------------------

class CWin32_ProgressBar : public IProgress
{
	HWND m_hItem;
	bool m_bPumpMsgs;
	void* m_hThreadOwner;

	int m_ProgressLimit;
	int m_ProgressCount;

	void MessagePump();

public:
	CWin32_ProgressBar(HWND _hItem, bool _bPumpMsgs = false);
	virtual void Push(const char* _pLevelName = NULL);
	virtual void Pop();
	virtual void SetProgress(fp32 _p, const char* _pTaskName = NULL);
	virtual void SetProgressText(const char* _pTaskName = NULL);
	virtual void InitProgressCount(int _Count, const char* _pTaskName = NULL);
	virtual void IncProgress(const char* _pTaskName = NULL);
	
	MACRO_OPERATOR_TPTR(CReferenceCount);
};

#include "MWin32Registry.h"

#endif
