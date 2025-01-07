
#include "PCH.h"

#include "MWindows.h"
#include "MFloat.h"
#include "../../MSystem/Misc/MLocalizer.h"

#ifdef COMPILER_MSVC
#pragma warning(disable : 4065)
#endif


/*

class CCTime
{
protected:
	uint64 m_Cycles;		// At 1ns per clock this counter will wrap 585 years after the computer was turned on.

public:
	CCTime();

	const CCTime& operator= (const CCTime&);
	const CCTime& operator= (uint64);			// Cycles
	const CCTime& operator= (fp64);				// Seconds

	CCTime operator- (const CCTime&);
	CCTime Delta() const;
	operator fp32 () const;						// Seconds
	operator fp64 () const;						// Seconds
};

*/
class CStringPair
{
public:
	CStr m_Name;
	CStr m_Value;
};

class CStringPairList : public TArray<CStringPair>
{
	// used for the parser
	bool IsSpace(char c) { return c == ' ' || c == '\t'; }
	bool IsAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
	bool IsNumeric(char c) { return c >= '0' && c <= '9'; }
	bool IsAlphaNumeric(char c) { return IsAlpha(c) || IsNumeric(c) || c == '_'; }
public:

	// constructors
	CStringPairList() {}
	CStringPairList(const CStr &rStr) { CreateFromString(rStr); }

	//
	// Format: name="value" name="value" name="value"
	//
	void CreateFromString(const CStr &rStr)
	{
		// parse value
		const char *pPos = rStr.GetStr();

		const char *pNameStart = NULL, *pValueStart = NULL;
		int32 ValueLen = 0, NameLen = 0;

		enum { ESkip, EName, ESearchForEqual, ESearchForString, EValue, EProcess };
		int32 State = ESkip;

		while(1)
		{
			if(!*pPos)
			{
				if(State == ESkip)
					break;
				State = EProcess;
			}

			switch(State)
			{
			case ESkip:
				if(IsSpace(*pPos))
				{
					pPos++;
					break; // continue;
				}
				pNameStart = pPos;
				State = EName;
			case EName:
				if(IsAlphaNumeric(*pPos))
				{
					NameLen++;
					pPos++;
					break; // continue;
				}
				if(!NameLen)
					return; // problems
				State = ESearchForEqual;
			case ESearchForEqual:
				if(IsSpace(*pPos))
					pPos++;
				else if(*pPos == '=')
				{
					pPos++;
					State = ESearchForString;
				}
				else
					State = EProcess;
				break;
			case ESearchForString:
				if(IsSpace(*pPos))
					pPos++;
				else if(*pPos == '\'')
				{
					pPos++;
					pValueStart = pPos;
					State = EValue;
				}
				else
					State = EProcess;
				break;
			case EValue:
				if(*pPos == '\'')
					State = EProcess;
				else
					ValueLen++;
				pPos++;
				break;
			case EProcess:
				if(NameLen)
				{
					CStringPair Pair;

					Pair.m_Name.Capture(pNameStart, NameLen);
					if(pValueStart && ValueLen)
						Pair.m_Value.Capture(pValueStart, ValueLen);
					else
						Pair.m_Value = "";

					Add(Pair);

					//pView->Set(Name, Value);
				}
				pNameStart = NULL;
				pValueStart = NULL;
				NameLen = 0;
				ValueLen = 0;
				State = ESkip;
				break;
			}
		}
	}

private: // don't allow copy
	CStringPairList(const CStringPairList &)
	{}
};


// -------------------------------------------------------------------
spCMWnd g_spDesktop;

// -------------------------------------------------------------------
void CMWnd_Message::Clear()
{
	MAUTOSTRIP(CMWnd_Message_Clear, MAUTOSTRIP_VOID);
	m_Msg = 0;
	m_pID = NULL;
	m_Param0 = 0;
	m_Param1 = 0;
	m_Param2 = 0;
	m_Param3 = 0;
	m_fParam0 = 0;
	m_fParam1 = 0;
	m_vParam0 = 0;
	m_vParam1 = 0;
}

CMWnd_Message::CMWnd_Message(int _Msg, char* _pID, int _p0, int _p1, int _p2, int _p3)
{
	MAUTOSTRIP(CMWnd_Message_ctor, MAUTOSTRIP_VOID);
	Clear();
	m_Msg = _Msg;
	m_pID = _pID;
	m_Param0 = _p0;
	m_Param1 = _p1;
	m_Param2 = _p2;
	m_Param3 = _p3;
}

// -------------------------------------------------------------------
//MRTC_IMPLEMENT_DYNAMIC(CMWnd_View, CReferenceCount);

// -------------------------------------------------------------------

fp32 CMWnd::ms_RepeatDelay = 0.5;
CMTime CMWnd::ms_LastRepeatTime;

// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd, CMWnd_View);


void CMWnd::SetRoot(CMWnd* _pWnd)
{
	MAUTOSTRIP(CMWnd_SetRoot, MAUTOSTRIP_VOID);
	m_pWndRoot = _pWnd;

	int nw = m_lspWndChildren.Len();
	for(int i = 0; i < nw; i++)
		m_lspWndChildren[i]->SetRoot(_pWnd);
}

void CMWnd::ParentDestroyed()
{
	MAUTOSTRIP(CMWnd_ParentDestroyed, MAUTOSTRIP_VOID);
	m_pWndParent = NULL;
	SetRoot(this);
	m_AbsPos = m_Pos;
	OnParentDestroyed();
}

void CMWnd::DoBusy()
{
	MAUTOSTRIP(CMWnd_DoBusy, MAUTOSTRIP_VOID);
	if (m_pfnBusyCallback) m_pfnBusyCallback();
	OnBusy();
	for(int i = 0; i < m_lspWndChildren.Len(); i++)
		m_lspWndChildren[i]->DoBusy();
}

void CMWnd::DoRefresh()
{
	MAUTOSTRIP(CMWnd_DoRefresh, MAUTOSTRIP_VOID);
	OnRefresh();
	for(int i = 0; i < m_lspWndChildren.Len(); i++)
		m_lspWndChildren[i]->DoRefresh();
}

// -------------------------------------------------------------------
CMWnd* CMWnd::GetFocusWnd()
{
	MAUTOSTRIP(CMWnd_GetFocusWnd, NULL);
	if (m_Status & WSTATUS_DISABLED) return NULL;
	if (!(m_Status & WSTATUS_FOCUS)) return NULL;

	CMWnd* pWndFocus = NULL;
	for(int i = 0; i < m_lspWndChildren.Len(); i++)
		if ((pWndFocus = m_lspWndChildren[i]->GetFocusWnd())) return pWndFocus;

	// No child was focus, return this
	return this;
}

CMWnd* CMWnd::GetChildFocusWnd()
{
	MAUTOSTRIP(CMWnd_GetChildFocusWnd, NULL);
	for(int i = 0; i < m_lspWndChildren.Len(); i++)
		if (m_lspWndChildren[i]->m_Status & WSTATUS_FOCUS) return m_lspWndChildren[i];
	return NULL;
}

// Search in noclipwindows

CMWnd* CMWnd::GetCursorOverNoClippingWnd(const CPnt& _Pos)
{
	MAUTOSTRIP(CMWnd_GetCursorOverNoClippingWnd, NULL);
	CMWnd* pWndOver = NULL;

	// Children first

	for(int i = 0; i < m_lspWndChildren.Len(); i++)
		if ((pWndOver = m_lspWndChildren[i]->GetCursorOverNoClippingWnd(_Pos))) return pWndOver;

	if (m_AbsPos.Inside(_Pos))
	{
		return this;
	}

	return NULL;
}

CMWnd* CMWnd::SearchCursorOverNoClippingWnd(const CPnt& _Pos)
{
	MAUTOSTRIP(CMWnd_SearchCursorOverNoClippingWnd, NULL);
	if (m_Status & WSTATUS_DISABLED) return NULL;

	CMWnd* pWndOver = NULL;

	if (m_Style & WSTYLE_NOCLIPPING)
	{
		// Search this leaf

		if ((pWndOver = GetCursorOverNoClippingWnd(_Pos)))
			return pWndOver;
	}

	for(int i = 0; i < m_lspWndChildren.Len(); i++)
		if ((pWndOver = m_lspWndChildren[i]->SearchCursorOverNoClippingWnd(_Pos))) return pWndOver;

	return NULL;
}

CMWnd* CMWnd::GetCursorOverWnd(const CPnt& _Pos, bool _OnlyTabStops)
{
	MAUTOSTRIP(CMWnd_GetCursorOverWnd, NULL);
	if (m_Status & WSTATUS_DISABLED) return NULL;

	if (m_AbsPos.Inside(_Pos))
	{
		CMWnd* pWndOver = NULL;
		for(int i = 0; i < m_lspWndChildren.Len(); i++)
		{
			if ((pWndOver = m_lspWndChildren[i]->GetCursorOverWnd(_Pos, _OnlyTabStops))) 
			{
				if (!_OnlyTabStops)
					return pWndOver;
				else if (pWndOver->m_Style & WSTYLE_TABSTOP)
					return pWndOver;
			}
		}

		return this;
	}

	return NULL;
}

CMWnd* CMWnd::GetRootWnd()
{
	MAUTOSTRIP(CMWnd_GetRootWnd, NULL);
	return m_pWndRoot;
}

CMWnd* CMWnd::GetFirstChild()
{
	MAUTOSTRIP(CMWnd_GetFirstChild, NULL);
	if (m_lspWndChildren.Len())
		return m_lspWndChildren[0];

	return NULL;
}

// -------------------------------------------------------------------
void CMWnd::FetchInput()
{
	MAUTOSTRIP(CMWnd_FetchInput, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) return;

	DoRefresh();
	if (pSys->m_spInput!=NULL)
	{
		CScanKey lKeys[16];
		uint nKeys = pSys->m_spInput->GetScanKeys(lKeys, 16);
		while(nKeys > 0 &&  !m_bQuit)
		{
			for(uint iK = 0; iK < nKeys; iK++)
			{
				CScanKey Scan = lKeys[iK];
				if (Scan.GetKey9() == SKEY_ESC) pSys->m_bBreakRequested = true;
				ProcessKey(Scan, Scan);
			}
			nKeys = pSys->m_spInput->GetScanKeys(lKeys, 16);
		}
	}
	else
	{
		m_bQuit = true;
		m_ExitCode = -2;
	}


//	Error("FetchInput", "This function is obsolete.");

/*	DoRefresh();

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) return;

	if (pSys->m_spInput!=NULL) pSys->m_spInput->Update();

	if (pSys->m_spDisplay!=NULL)
	{
		CPnt Pos = m_AbsPos.p0 + GetCursorPos();
		int Buttons = GetCursorBtn();

		CMWnd* pWOver = GetCursorOverWnd(Pos);
		CMWnd* pWnd = pWOver;

		CMWnd* pWFocus = GetFocusWnd();
		if (pWnd)
		{
			if ((pWFocus != pWnd) && (Buttons)) 
				pWnd->SetFocus();
		}
		else
			pWnd = pWFocus;

		if (pWnd && pWnd->CursorInWnd())
		{
			CMWnd_Message Message(WMSG_CURSOR, "", Pos.x - pWnd->m_AbsPos.p0.x, Pos.y - pWnd->m_AbsPos.p0.y, Buttons);
			pWnd->OnMessage(&Message);
		}
	}

	if (pSys->m_spInput!=NULL)
		while(pSys->m_spInput->KeyPressed())
		{
			CMWnd* pWFocus = GetFocusWnd();
			CScanKey Key = pSys->m_spInput->ScanKey();
			if (Key.GetKey9() == SKEY_ESC) pSys->m_bBreakRequested = true;
//LogFile(CStrF("Key %.4x to %.8x", Key.m_ScanKey32, pWFocus));
			CMWnd_Message Msg(WMSG_KEY, "", Key.m_ScanKey32, Key.m_Char);
			if (pWFocus) pWFocus->OnMessage(&Msg);
		}*/
}

// -------------------------------------------------------------------
int CMWnd::ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
{
	MAUTOSTRIP(CMWnd_ProcessKey, 0);
	if (m_SuperFocusWindow)
	{
		if (_Key.GetKey9() >= SKEY_MOUSE1 && (_Key.GetKey9() <= SKEY_MOUSE4))
		{
			ConOut("MOUSE SuperFocusWindow->ProcessKey(_Key)");
		}
		else
		{
			return m_SuperFocusWindow->ProcessKey(_Key, _OriginalKey);
		}
	}

	CMWnd* pWndLock = m_pWndLockInput;
//	CMWnd* pWFocus = GetFocusWnd();

	CMWnd_Message Msg;
	/*
	if (_Key.GetKey9() >= SKEY_MOUSE1 && (_Key.GetKey9() <= SKEY_MOUSE4))
	{
		if (!pWndLock && _Key.IsDown())
		{
			CPnt Pos(_Key.GetPosX(), _Key.GetPosY());

			CMWnd* pWOver = NULL;

			if (!(pWOver = SearchCursorOverNoClippingWnd(Pos)))
			{
				pWOver = GetCursorOverWnd(Pos);
			}

			if (pWOver)
			{
//				ConOut(CStrF("winpos x %d y %d",pWOver->m_AbsPos.p0.x,pWOver->m_AbsPos.p0.y));
				if (pWFocus != pWOver) 
				{
					pWOver->SetFocus();
					pWFocus = GetFocusWnd();
				}
			}
		}

//		ConOutL(CStrF("Mouse button %.8x", _Key.m_ScanKey32));
		int Button = _Key.GetKey9() - SKEY_MOUSE1;
		Msg = CMWnd_Message((pWndLock) ? WMSG_LOCKEDCURSOR : WMSG_CURSOR, "", _Key.GetPosX(), _Key.GetPosY(), (1 << Button) + (_Key.IsDown() ? 0 : 0x8000));
//		ConOutL(CStrF("Mouse button %.8x, %d, %d", _Key.m_ScanKey32, _Key.GetPosX(), _Key.GetPosY()));
	}
	else if (_Key.GetKey9() == SKEY_MOUSEMOVE)
	{
		Msg = CMWnd_Message((pWndLock) ? WMSG_LOCKEDCURSORMOVE : WMSG_CURSORMOVE, "", _Key.GetPosX(), _Key.GetPosY(), _Key.GetUserShift() );
//		ConOutL(CStrF("Mouse move %d, %d, %d", _Key.m_ScanKey32, _Key.GetPosX(), _Key.GetPosY()));
	}
	else*/
	{
//		ConOutL(CStrF("Key %d", _Key.m_ScanKey32));
		Msg = CMWnd_Message((pWndLock) ? WMSG_LOCKEDKEY : WMSG_KEY, "", _Key.m_ScanKey32, _Key.m_Char, _OriginalKey.m_ScanKey32, _OriginalKey.m_Char);
		Msg.m_fParam0 = _Key.m_Data[0];
		Msg.m_fParam1 = _Key.m_Data[1];
	}

	CMWnd* pWTarget = (pWndLock) ? pWndLock : this;

	return pWTarget->OnProcessScanKey(&Msg);
}

// -------------------------------------------------------------------
int CMWnd::ProcessCursor(const CPnt& _Pos, const CPnt& _dPos, int _Buttons)
{
	MAUTOSTRIP(CMWnd_ProcessCursor, 0);
	Error("ProcessCursor", "This function is obsolete.");

	CPnt Pos = m_AbsPos.p0 + _Pos;

	CMWnd* pWOver = GetCursorOverWnd(Pos, false);
	CMWnd* pWnd = pWOver;

	CMWnd* pWFocus = GetFocusWnd();

	// Debug print...
//	ConOut(CStrF("MOverPtr: %x,  FocusPtr: %x, Buttons: %d, Pos: (%d, %d)", pWOver, pWFocus, _Buttons, Pos.x, Pos.y));

	if (pWnd)
	{
		if ((pWFocus != pWnd) )//&& (_Buttons)) 
			pWnd->SetFocus();
	}
	else
		pWnd = pWFocus;

	if (pWnd && pWnd->GetCursorOverWnd(Pos, false))
	{
		CMWnd_Message Message(WMSG_CURSOR, "", Pos.x - pWnd->m_AbsPos.p0.x, Pos.y - pWnd->m_AbsPos.p0.y, _Buttons);
		return pWnd->OnMessage(&Message);
	}

	return false;
}

bool CMWnd::DoLockInput(CMWnd* _pWnd)
{
	MAUTOSTRIP(CMWnd_DoLockInput, false);
	if (m_pWndRoot && (m_pWndRoot != this)) 
		return m_pWndRoot->DoLockInput(_pWnd);
	else
	{
		// This is the root
		if (m_pWndLockInput)
			return false;
		else
		{
			m_pWndLockInput = _pWnd;
			return true;
		}
	}
}

void CMWnd::DoUnlockInput(CMWnd* _pWnd)
{
	MAUTOSTRIP(CMWnd_DoUnlockInput, MAUTOSTRIP_VOID);
	if (m_pWndRoot && (m_pWndRoot != this)) 
		m_pWndRoot->DoUnlockInput(_pWnd);
	else
	{
		// This is the root
		if (m_pWndLockInput == _pWnd)
			m_pWndLockInput = NULL;
		else
			Error("DoUnlockInput", "Input was locked by another window.");
	}
}

bool CMWnd::LockInput()
{
	MAUTOSTRIP(CMWnd_LockInput, false);
	return DoLockInput(this);
}

void CMWnd::UnlockInput()
{
	MAUTOSTRIP(CMWnd_UnlockInput, MAUTOSTRIP_VOID);
	DoUnlockInput(this);
}

// -------------------------------------------------------------------
void CMWnd::Wait()
{
	MAUTOSTRIP(CMWnd_Wait, MAUTOSTRIP_VOID);
	Error("Wait", "Temporarily disabled.");

	m_pWndRoot->DoBusy();
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	pSys->Refresh();
//	if (pSys && pSys->m_spInput)
//		pSys->m_spInput->Update();
}

void CMWnd::ResetRepeatDelay()
{
	MAUTOSTRIP(CMWnd_ResetRepeatDelay, MAUTOSTRIP_VOID);
	ms_RepeatDelay = 0.4;
//	ms_LastRepeatTime = CMTime::GetCPU();
	ms_LastRepeatTime.Snapshot();
}

void CMWnd::MouseRepeatDelay(int _Type)
{
	MAUTOSTRIP(CMWnd_MouseRepeatDelay, MAUTOSTRIP_VOID);
	return;
/*
	fp64 WaitTime = ms_RepeatDelay;
	if (GetCursorBtn() & 2) WaitTime = 0.0;
	while(GetTime() - ms_LastRepeatTime < WaitTime)
	{
		Wait();
		if (!GetCursorBtn()) return;
		if (GetCursorBtn() & 2) return;
	}

	fp64 MinTime = 0.1;
	switch(_Type)
	{
	case 0 : MinTime = 0.05; break;
	case 1 : MinTime = 0.1; break;
	case 2 : MinTime = 0.2; break;
	}
	ms_RepeatDelay -= 0.15;
	if (ms_RepeatDelay < MinTime) ms_RepeatDelay = MinTime;
	ms_LastRepeatTime = GetTime();*/
}

// -------------------------------------------------------------------
void CMWnd::AddChild(spCMWnd _spWnd)
{
	MAUTOSTRIP(CMWnd_AddChild, MAUTOSTRIP_VOID);
	{
		int nw = m_lspWndChildren.Len();
		for(int i = 0; i < nw; i++)
			if (m_lspWndChildren[i] == _spWnd)
				Error("AddChild", "Window is already a child of this window.");
	}

	if (_spWnd->m_Style & WSTYLE_BEHIND)
		m_lspWndChildren.Add(_spWnd);
	else if (_spWnd->m_Style & WSTYLE_ONTOP)
		m_lspWndChildren.Insert(0, _spWnd);
	else
	{
		int nw = m_lspWndChildren.Len();
		int i = 0;
		while((i < nw) && !(m_lspWndChildren[i]->m_Style & WSTYLE_BEHIND)) i++;
		m_lspWndChildren.Insert(i, _spWnd);
	}

	_spWnd->m_pWndParent = this;
	_spWnd->SetRoot(m_pWndRoot);

	_spWnd->SetPosition(_spWnd->GetPosition());
}

void CMWnd::RemoveChild(CMWnd* _pWnd)
{
	MAUTOSTRIP(CMWnd_RemoveChild, MAUTOSTRIP_VOID);
	int nw = m_lspWndChildren.Len();
	for(int i = 0; i < nw; i++)
		if ((CMWnd*)m_lspWndChildren[i] == _pWnd)
		{
			if(FindItem(m_pWndLockInput))
				m_pWndLockInput = NULL;

			spCMWnd spMWnd = m_lspWndChildren[i];
			m_lspWndChildren.Del(i);
			spMWnd = NULL;
			return;
		}
}

void CMWnd::RemoveAllChilds()
{
	MAUTOSTRIP(CMWnd_RemoveAllChilds, MAUTOSTRIP_VOID);
	while(m_lspWndChildren.Len())
	{
		RemoveChild((CMWnd*)m_lspWndChildren[0]);
	}
}


CMWnd* CMWnd::GetParent()
{
	MAUTOSTRIP(CMWnd_GetParent, NULL);
	return m_pWndParent;
}

void CMWnd::BringToTop(CMWnd* _pWnd)
{
	MAUTOSTRIP(CMWnd_BringToTop, MAUTOSTRIP_VOID);
	int i;
	for(i = 0; i < m_lspWndChildren.Len();i++)
		if (_pWnd == (CMWnd*)m_lspWndChildren[i]) break;

	// Find window position
	if (i >= m_lspWndChildren.Len()) return;
	spCMWnd spW = m_lspWndChildren[i];

	// Remove it
	m_lspWndChildren.Del(i);

	// Find insert position.
	int ins = 0;
	if (_pWnd->m_Style & WSTYLE_BEHIND)
	{
		ins = m_lspWndChildren.Len() -1;
		while((ins > 0) && (m_lspWndChildren[ins]->m_Style & WSTYLE_BEHIND)) ins--;
		ins++;
	}
	else if (_pWnd->m_Style & WSTYLE_ONTOP)
	{
		ins = 0;
	}
	else
	{
		int nw = m_lspWndChildren.Len();
		ins = 0;
		while((ins < nw) && (m_lspWndChildren[ins]->m_Style & WSTYLE_ONTOP)) ins++;
	}

	// Insert
	m_lspWndChildren.Insert(ins, spW);

	if (m_pWndParent) m_pWndParent->BringToTop(this);
}

void CMWnd::KillFocus()
{
	MAUTOSTRIP(CMWnd_KillFocus, MAUTOSTRIP_VOID);
	int nIter = 0;
	CMWnd* pWFocus = (m_pWndRoot) ? m_pWndRoot->GetFocusWnd() : GetFocusWnd();
	do
	{
		while(pWFocus)
		{
			pWFocus->m_Status &= ~WSTATUS_FOCUS;
			pWFocus = pWFocus->GetParent();
		}

		pWFocus = (m_pWndRoot) ? m_pWndRoot->GetFocusWnd() : GetFocusWnd();
		if (nIter++ > 1000) Error("KillFocus", "Internal error.");
	}
	while(pWFocus);
}

void CMWnd::OnSetFocus()
{
	MAUTOSTRIP(CMWnd_OnSetFocus, MAUTOSTRIP_VOID);
}

void CMWnd::SetFocus()
{
	MAUTOSTRIP(CMWnd_SetFocus, MAUTOSTRIP_VOID);
	KillFocus();

	CMWnd* pFocus = this;
	while(pFocus->GetParent())
	{
		pFocus->m_Status |= WSTATUS_FOCUS;
		pFocus->GetParent()->BringToTop(this);
		pFocus = pFocus->GetParent();
	}

	if (pFocus)
	{
		pFocus->m_Status |= WSTATUS_FOCUS;
		OnSetFocus();
	}
}

int32 CMWnd::GetNumItems()
{
	return m_lspWndChildren.Len();
}

CMWnd* CMWnd::GetItem(const int32 _Index)
{
	return m_lspWndChildren[_Index];
}

CMWnd* CMWnd::GetItem(const CStr& _ID)
{
	MAUTOSTRIP(CMWnd_GetItem, NULL);
	int nChildren = m_lspWndChildren.Len();
	for(int i = 0; i < nChildren; i++)
	{
//		LogFile("FNISS: " + m_lspWndChildren[i]->m_ID);
		if (m_lspWndChildren[i]->m_ID == _ID) return m_lspWndChildren[i];
	}
	return NULL;
}

CMWnd* CMWnd::FindItem(const CStr& _ID)
{
	MAUTOSTRIP(CMWnd_FindItem, NULL);
	if (m_ID == _ID)
		return this;
	else
	{
		int nChildren = m_lspWndChildren.Len();
		for(int i = 0; i < nChildren; i++)
		{
			CMWnd* pWnd = m_lspWndChildren[i]->FindItem(_ID);
			if (pWnd) return pWnd;
		}
	}
	return NULL;
}

CMWnd* CMWnd::FindItem(CMWnd *_pWnd)
{
	MAUTOSTRIP(CMWnd_FindItem_2, NULL);
	if (this == _pWnd)
		return this;
	else
	{
		int nChildren = m_lspWndChildren.Len();
		for(int i = 0; i < nChildren; i++)
		{
			CMWnd* pWnd = m_lspWndChildren[i]->FindItem(_pWnd);
			if (pWnd) return pWnd;
		}
	}
	return NULL;
}

// -------------------------------------------------------------------
void CMWnd::Clear()
{
	MAUTOSTRIP(CMWnd_Clear, MAUTOSTRIP_VOID);
	m_bQuit = false;
	m_ExitCode = 0;
	m_pfnBusyCallback = NULL;
	m_Col_Highlight =	0xffffff;
	m_Col_Background =	0xc0c0c0;
	m_Col_Shadow =		0x808080;
	m_Col_DarkShadow =	0x404040;
	m_Col_Alpha =		0xff;
	m_Col_Custom0 =		0xa0a0b0;
	m_Col_Custom0_Focus = 0xa0e0f0;
//	int Alpha = 0xc0000000;
//	int ColH = 0xffffff + Alpha;
//	int ColM = 0xc0c0c0 + Alpha;
//	int ColD = 0x808080 + Alpha;
//	int ColDD = 0x404040 + Alpha;

	m_Pos = CRct(0,0,0,0);
	m_pWndParent = 0;
	m_pWndRoot = this;
	m_pWndLockInput = NULL;
	m_Style = 0;
	m_Status = 0;
	m_UGLYDEMOHACK = false;
	m_Align0 = CPnt(0,0);
	m_Align1 = CPnt(0,0);

	m_bMouseOverWindow = false;

	m_BlocksLoadingScreen = false;
	m_bShowMouse = false;
	m_Interactive = true; // int
	m_AllowsAttractMode = false;
	//m_iButtonDescriptorVCorrection = 0;
	m_SuperFocusWindow = 0;
	m_bAllowChildFocusWrap = false;
}

void CMWnd::DoCreate(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID)
{
	MAUTOSTRIP(CMWnd_DoCreate, MAUTOSTRIP_VOID);
	m_Style = _Param.m_Style;
	m_Status = _Param.m_Status | WSTATUS_CREATED;
	m_Align0 = _Param.m_Align0;
	m_Align1 = _Param.m_Align1;
	m_pWndParent = _pWndParent;
	m_ID = _ID;
	if (m_pWndParent) m_pWndParent->AddChild(this);

	SetPosition(_Param.m_Pos);
}

void CMWnd::DoDestroy()
{
	MAUTOSTRIP(CMWnd_DoDestroy, MAUTOSTRIP_VOID);
	m_Status &= ~WSTATUS_CREATED;

	int nw = m_lspWndChildren.Len();
	for(int i = 0; i < nw; i++)
		m_lspWndChildren[i]->ParentDestroyed();
	m_lspWndChildren.Clear();

	if (m_pWndParent) m_pWndParent->RemoveChild(this);
	m_pWndParent = NULL;
}

void CMWnd::EvaluateByKey(CMWnd_Param* _pParam, CRegistry *_pReg, const char *_pKey)
{
	CStr Key = _pKey;
	if (_pReg->FindChild(_pKey))
		EvaluateKeyOrdered(_pParam, Key, _pReg->GetValue(Key));
}

void CMWnd::EvaluateRegisty(CMWnd_Param* _pParam, CRegistry *_pReg)
{
	EvaluateByKey(_pParam, _pReg, "POS");
	EvaluateByKey(_pParam, _pReg, "SIZE");
	EvaluateByKey(_pParam, _pReg, "RGN");
	EvaluateByKey(_pParam, _pReg, "RGNABS");
	EvaluateByKey(_pParam, _pReg, "STYLE");
	EvaluateByKey(_pParam, _pReg, "STATUS");
	EvaluateByKey(_pParam, _pReg, "ALIGN");
	EvaluateByKey(_pParam, _pReg, "VIEW");
	EvaluateByKey(_pParam, _pReg, "WINDOW");
}

void CMWnd::EvaluateKeyOrdered(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	CStr Value = _Value;
	if (_Key == "POS")
	{
		_pParam->m_Pos.p1 -= _pParam->m_Pos.p0;
		_pParam->m_Pos.p0.x = Value.GetStrSep(",").Val_int();
		_pParam->m_Pos.p0.y = Value.GetStrSep(",").Val_int();
		_pParam->m_Pos.p1 += _pParam->m_Pos.p0;
	}
	else if (_Key == "SIZE")
	{
//		LogFile(_Key + ", " + _Value);
		_pParam->m_Pos.p1.x = Value.GetStrSep(",").Val_int();
		_pParam->m_Pos.p1.y = Value.GetStrSep(",").Val_int();
		_pParam->m_Pos.p1 += _pParam->m_Pos.p0;

//LogFile(CStrF("%d, %d", _pParam->m_Pos.p1.x, _pParam->m_Pos.p1.y));
	}
	else if (_Key == "RGN")
	{
		_pParam->m_Pos.p0.x = Value.GetStrSep(",").Val_int();
		_pParam->m_Pos.p0.y = Value.GetStrSep(",").Val_int();
		_pParam->m_Pos.p1.x = Value.GetStrSep(",").Val_int();
		_pParam->m_Pos.p1.y = Value.GetStrSep(",").Val_int();
		_pParam->m_Pos.p1 += _pParam->m_Pos.p0;
	}
	else if (_Key == "RGNABS")
	{
		_pParam->m_Pos.p0.x = Value.GetStrSep(",").Val_int();
		_pParam->m_Pos.p0.y = Value.GetStrSep(",").Val_int();
		_pParam->m_Pos.p1.x = Value.GetStrSep(",").Val_int();
		_pParam->m_Pos.p1.y = Value.GetStrSep(",").Val_int();
	}
	else if (_Key == "STYLE")
	{
		while(Value.Len())
		{
			CStr s = Value.GetStrSep(",").LTrim().RTrim();
			if (s == "MOVABLE")
				_pParam->m_Style |= WSTYLE_MOVABLE;
			else if (s == "RESIZABLE")
				_pParam->m_Style |= WSTYLE_RESIZABLE;
			else if (s == "TABSTOP")
				_pParam->m_Style |= WSTYLE_TABSTOP;
			else if (s == "ONTOP")
				_pParam->m_Style |= WSTYLE_ONTOP;
			else if (s == "BEHIND")
				_pParam->m_Style |= WSTYLE_BEHIND;
			else if (s == "CLIENTEDGE")
				_pParam->m_Style |= WSTYLE_CLIENTEDGE;
			else if (s == "NOKEYPROCESS")
				_pParam->m_Style |= WSTYLE_NOKEYPROCESS;			
			else if (s == "HIDDENFOCUS")
				_pParam->m_Style |= WSTYLE_HIDDENFOCUS;	
			else if (s == "NOCLIPPING")
				_pParam->m_Style |= WSTYLE_NOCLIPPING;	
			else
				LogFile("(CMWnd::EvaluateKey) Unknown style: " + s);
		}
	}
	else if (_Key == "STATUS")
	{
		while(Value.Len())
		{
			CStr s = Value.GetStrSep(",").LTrim().RTrim();
			if (s == "MAXIMIZED")
				_pParam->m_Status |= WSTATUS_MAXIMIZED;
			if (s == "MINIMIZED")
				_pParam->m_Status |= WSTATUS_MINIMIZED;
			if (s == "DISABLED")
				_pParam->m_Status |= WSTATUS_DISABLED;
			if (s == "INACTIVE")
				_pParam->m_Status |= WSTATUS_INACTIVE;
			if (s == "PREASURE")
				_pParam->m_Status |= WSTATUS_PRESSURE;
			if (s == "MSGTOPARENT")
				_pParam->m_Status |= WSTATUS_MSGTOPARENT;

			if (s == "ALIGNY0_TOP")
				_pParam->m_Status |= WSTATUS_ALIGNY0_TOP;
			if (s == "ALIGNY0_BOTTOM")
				_pParam->m_Status |= WSTATUS_ALIGNY0_BOTTOM;
			if (s == "ALIGNX0_LEFT")
				_pParam->m_Status |= WSTATUS_ALIGNX0_LEFT;
			if (s == "ALIGNX0_RIGHT")
				_pParam->m_Status |= WSTATUS_ALIGNX0_RIGHT;
			if (s == "ALIGNY1_TOP")
				_pParam->m_Status |= WSTATUS_ALIGNY1_TOP;
			if (s == "ALIGNY1_BOTTOM")
				_pParam->m_Status |= WSTATUS_ALIGNY1_BOTTOM;
			if (s == "ALIGNX1_LEFT")
				_pParam->m_Status |= WSTATUS_ALIGNX1_LEFT;
			if (s == "ALIGNX1_RIGHT")
				_pParam->m_Status |= WSTATUS_ALIGNX1_RIGHT;

			else
				LogFile("(CMWnd::EvaluateKey) Unknown status: " + s);
		}
	}
	else if (_Key == "ALIGN")
	{
		_pParam->m_Align0.x = Value.GetStrSep(",").Val_int();
		_pParam->m_Align0.y = Value.GetStrSep(",").Val_int();
		_pParam->m_Align1.x = Value.GetStrSep(",").Val_int();
		_pParam->m_Align1.y = Value.GetStrSep(",").Val_int();
	}
	else if (_Key == "VIEW")
	{
		spCMWnd_View spView = (CMWnd_View*) MRTC_GetObjectManager()->CreateObject("CMWnd_" + _Value);
		if (!spView) LogFile("(CMWnd::EvaluateKey) Failed to create view: " + _Value);
		SetView(spView);
	}
	else if (_Key == "WINDOW")
	{
		// Ignore.
	}
}
// -------------------------------------------------------------------
void CMWnd::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	MAUTOSTRIP(CMWnd_EvaluateKey, MAUTOSTRIP_VOID);
	
	CStr Value = _Value;
	
	if (_Key == "SCRIPT_CREATE")
	{
		m_Script_Create = _Value;
	}
	else if (_Key == "SCRIPT_DESTROY")
	{
		m_Script_Destroy = _Value;
	}
	else if (_Key == "SCRIPT_UPDATE")
	{
		m_Script_Update = _Value;
	}
	else if (_Key.CompareSubStr("ACCELLERATOR") == 0)
	{
		spCMWnd_Accellerator spAcc = MNew(CMWnd_Accellerator);
		if (!spAcc) MemError("EvaluateKey");

		CStr Key = Value.GetStrSep(",");
		CStr Focus = Value.GetStrSep(",");
		CStr Script = Value.GetStrSep(",");
		spAcc->m_ScanCode = CScanKey::GetScanKey32Comb(Key);
		if (spAcc->m_ScanCode)
		{
			spAcc->m_FocusTarget = Focus;
			spAcc->m_Script = Script;
			m_lspAccellerators.Add(spAcc);
		}
		else
			ConOutL(CStrF("(CMWnd::EvaluateKey) Invalid scancode definition %s", (char*) Key));
	}
	else if(_Key == "OUTSIDEBUTTON")
	{
		COutsideButton Button;

		Button.m_Pos = 0;
		Button.m_Scale = 1;
		Button.m_Type = 0;
		Button.m_Extend = 0;

		CStringPairList List(_Value);
		for(int32 i = 0; i < List.Len(); i++)
		{
			CStr rName = List[i].m_Name.LowerCase();
			CStr rValue= List[i].m_Value;
			if(rName == "pos")
			{
				if(rValue.LowerCase() == "bottomleft")
					Button.m_Pos = 0;
				else if(rValue.LowerCase() == "bottomright")
					Button.m_Pos = 1;
				else if(rValue.LowerCase() == "middleleft")
					Button.m_Pos = 2;
				else if(rValue.LowerCase() == "bottomcenter")
					Button.m_Pos = 3;
				else
					Button.m_Pos = rValue.Val_int();
			}
			else if(rName == "action")
				Button.m_Action = rValue;
			else if(rName == "type")
			{
				if(rValue.LowerCase() == "left")
					Button.m_Type = 1;
				else if(rValue.LowerCase() == "right")
					Button.m_Type = 2;
				else if(rValue.LowerCase() == "accept")
					Button.m_Type = 3;
				else
					Button.m_Type = 0;
			}
			else if(rName == "caption")
				Button.m_Caption = rValue;
		}

		m_lOutsideButtons.Add(Button);
	}
	else if (_Key == "CLASSNAME")
	{
		//  /Ignore.
	}
	else if (_Key == "ID")
	{
		m_ID = _Value;
	}
	else if (_Key == "GROUP")
	{
		m_Group = _Value;
	}
	else if (_Key == "COLOR_BG")
	{
		m_Col_Background = Value.Val_int();
	}
	else if (_Key == "COLOR_HL")
	{
		m_Col_Highlight = Value.Val_int();
	}
	else if (_Key == "COLOR_SHDW")
	{
		m_Col_Shadow = Value.Val_int();
	}
	else if (_Key == "COLOR_DSHDW")
	{
		m_Col_DarkShadow = Value.Val_int();
	}
	else if (_Key == "COLOR_ALPHA")
	{
		m_Col_Alpha = Value.Val_int();
	}
	else if (_Key == "COLOR_CUSTOM0")
	{
		m_Col_Custom0  = Value.GetStrSep(",").Val_int();
		m_Col_Custom0_Focus  = Value.GetStrSep(",").Val_int();
	}
	else if (_Key == "COLOR_CUSTOM1")
	{
		m_Col_Custom1  = Value.GetStrSep(",").Val_int();
		m_Col_Custom1_Focus  = Value.GetStrSep(",").Val_int();
	}
	else if (_Key == "INTERACTIVE")
		m_Interactive = _Value.Val_int() != 0;
	else if (_Key == "ATTRACT")
		m_AllowsAttractMode = _Value.Val_int() != 0;
	else if (_Key == "BLOCKSLOADINGSCREEN")
		m_BlocksLoadingScreen = _Value.Val_int() != 0;
	else if (_Key == "SHOWMOUSE")
		m_bShowMouse = _Value.Val_int() != 0;
	else if (_Key == "BUTTONDESC0")
		m_aButtonDescriptions[0] = _Value;
	else if (_Key == "BUTTONDESC1")
		m_aButtonDescriptions[1] = _Value;
	else if (_Key == "BUTTONDESC2")
		m_aButtonDescriptions[2] = _Value;
	else if (_Key == "BUTTONDESC3")
		m_aButtonDescriptions[3] = _Value;
	else if (_Key == "CHILDFOCUSWRAP")
	{
		m_bAllowChildFocusWrap = (Value.Val_int() != 0);
	}

//	LogFile("(CMWnd::EvaluateKey) Key not processed: (" + _Key + " = " + _Value + ")");
}

void CMWnd::CreateChildren(CKeyContainerNode* _pKeys)
{
	MAUTOSTRIP(CMWnd_CreateChildren, MAUTOSTRIP_VOID);
//	CKeyContainer* pKC = _pKeys->GetKeys();

	int nChildren = _pKeys->GetNumChildren();
	for(int i = 0; i < nChildren; i++)
	{
		CKeyContainerNode* pN = _pKeys->GetChild(i);
		CKeyContainer* pKC = pN->GetKeys();

		int iKey = pKC->GetKeyIndex("CLASSNAME");
		if (iKey < 0) Error("Create", "No classname.");
		CStr Class = "CMWnd_" + pKC->GetKeyValue(iKey);
		spCMWnd spWnd = (CMWnd*) MRTC_GetObjectManager()->CreateObject(Class);
		if (!spWnd) Error("CreateChildren", "Invalid classname: " + Class);
		spWnd->Create(pN, this);
	}
}

void CMWnd::CreateChildren(CRegistry* _pKeys)
{
	MAUTOSTRIP(CMWnd_CreateChildren_2, MAUTOSTRIP_VOID);
	int nChildren = _pKeys->GetNumChildren();
	for(int i = 0; i < nChildren; i++)
	{
		CRegistry* pN = _pKeys->GetChild(i);
		if ((pN->GetThisName() == "WINDOW") && pN->GetNumChildren())
		{
			CRegistry* pClass = pN->Find("CLASSNAME");
			if (!pClass) Error("CreateChildren", "No class name.");
			CStr Class = "CMWnd_" + pClass->GetThisValue();
			spCMWnd spWnd = (CMWnd*) MRTC_GetObjectManager()->CreateObject(Class);
			if (!spWnd) Error("CreateChildren", "Invalid classname: " + Class);
			spWnd->Create(pN, this);
		}
	}
}

void CMWnd::Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID)
{
	MAUTOSTRIP(CMWnd_Create, MAUTOSTRIP_VOID);
	if (m_Status & WSTATUS_CREATED) Error("Create", "Window already created.");

	CMWnd_Param Param = _Param;
	PreEvalKeys(&Param);
	PreCreate(&Param);
	DoCreate(_pWndParent, Param, _ID);

	OnCreate();
}

void CMWnd::Destroy()
{
	MAUTOSTRIP(CMWnd_Destroy, MAUTOSTRIP_VOID);
	OnDestroy();
	DoDestroy();
}

void CMWnd::Create(CKeyContainerNode* _pKeys, CMWnd* _pWndParent)
{
	MAUTOSTRIP(CMWnd_Create_2, MAUTOSTRIP_VOID);
	if (m_Status & WSTATUS_CREATED) Error("Create", "Window already created.");

	CMWnd_Param Param;

	PreEvalKeys(&Param);

	CKeyContainer* pKC = _pKeys->GetKeys();
	int nKeys = pKC->GetnKeys();
	for(int i = 0; i < nKeys; i++)
		EvaluateKey(&Param, pKC->GetKeyName(i), pKC->GetKeyValue(i));

	PreCreate(&Param);
	DoCreate(_pWndParent, Param, m_ID);

	CreateChildren(_pKeys);

	OnCreate();
}

void CMWnd::Create(CRegistry* _pKeys, CMWnd* _pWndParent)
{
	MAUTOSTRIP(CMWnd_Create_3, MAUTOSTRIP_VOID);
	if (m_Status & WSTATUS_CREATED) Error("Create", "Window already created.");

	CMWnd_Param Param;

	PreCreate(&Param);


	m_pWndParent = _pWndParent; // we need to know the parent when we are evaluating keys /kma


	int nKeys = _pKeys->GetNumChildren();
	EvaluateRegisty(&Param, _pKeys);
	for(int i = 0; i < nKeys; i++)
		EvaluateKey(&Param, _pKeys->GetName(i), _pKeys->GetValue(i));

	PreCreate(&Param);
	DoCreate(_pWndParent, Param, m_ID);

	CreateChildren(_pKeys);

	OnCreate();
}

CMWnd::CMWnd()
{
	MAUTOSTRIP(CMWnd_ctor, MAUTOSTRIP_VOID);
	Clear();
}

CMWnd::CMWnd(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID)
{
	MAUTOSTRIP(CMWnd_ctor_2, MAUTOSTRIP_VOID);
	Clear();
	Create(_pWndParent, _Param, _ID);
}

CMWnd::~CMWnd()
{
	MAUTOSTRIP(CMWnd_dtor, MAUTOSTRIP_VOID);
	if (m_Status & WSTATUS_CREATED) Destroy();
}

void CMWnd::RenderTree(CRC_Util2D* _pRCUtil, const CClipRect& _Clip)
{
	MAUTOSTRIP(CMWnd_RenderTree, MAUTOSTRIP_VOID);
	if (m_Status & WSTATUS_DISABLED) return;
	if (m_Status & WSTATUS_TREEINVISIBLE) return;
	if (!_Clip.Visible(m_Pos) && !(m_Style & WSTYLE_NOCLIPPING)) return;
 
	CClipRect ClipLocal;
	ClipLocal.ofs = CPnt(0,0);
	ClipLocal.clip = CRct(0, 0, GetWidth(), GetHeight());

 	CClipRect Clip = _Clip;
	Clip.ofs += m_Pos.p0;
	ClipLocal += Clip;

	if (!(m_Status & WSTATUS_INVISIBLE))
	{
//		_pRCUtil->GetRC()->Attrib_Push();
//		_pRCUtil->GetRC()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
//		_pRCUtil->GetRC()->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
		_pRCUtil->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		_pRCUtil->GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
		
		if (!(m_Style & WSTYLE_NOCLIPPING))
		{
			CRct noclipClipRect = CRct(0, 0, GetWidth(), GetHeight());

			if (GetRootWnd())
			{
				noclipClipRect = GetRootWnd()->m_AbsPos;
			}

			if (!m_spView)
			{
				OnPaint(_pRCUtil, ClipLocal, noclipClipRect);
			}
			else
				m_spView->OnPaint(_pRCUtil, ClipLocal, noclipClipRect);
		}
		else
		{
			// Use m_pWndRoot as cliprect

			if (m_pWndRoot)
			{
				ClipLocal.clip = CRct(0, 0, m_pWndRoot->GetWidth(), m_pWndRoot->GetHeight());

				OnPaint(_pRCUtil, ClipLocal, CRct(0, 0, m_pWndRoot->GetWidth(), m_pWndRoot->GetHeight()));
			}
		}
		
//		_pRCUtil->GetRC()->Attrib_Pop();

		if ((m_Status & WSTATUS_FOCUS) && !(m_Style & WSTYLE_HIDDENFOCUS))
			OnPaintFocus(_pRCUtil, ClipLocal, CRct(0, 0, GetWidth(), GetHeight()));
	}

	for(int i = m_lspWndChildren.Len()-1; i >= 0; i--)
		m_lspWndChildren[i]->RenderTree(_pRCUtil, ClipLocal);
}

void CMWnd::AddFont(spCRC_Font _spFont, const CStr& _Name)
{
	MAUTOSTRIP(CMWnd_AddFont, MAUTOSTRIP_VOID);
	if (!_spFont) return;
	m_lspFonts.Add(_spFont);
	m_lFontNames.Add(_Name);
}

spCRC_Font CMWnd::GetFont(char* _pName)
{
	MAUTOSTRIP(CMWnd_GetFont, NULL);
	int nFonts = m_lspFonts.Len();
	for(int i = 0; i < nFonts; i++)
		if (!m_lFontNames[i].Compare(_pName)) return m_lspFonts[i];

	if (m_pWndParent) return m_pWndParent->GetFont(_pName);
	return NULL;
}

CPnt CMWnd::GetCursorPos(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_GetCursorPos, CPnt());
	return _pMsg->GetCursorPos() - m_AbsPos.p0;
/*	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys && (pSys->m_spInput!=NULL))
		return pSys->m_spInput->GetMousePosition() - m_AbsPos.p0;
	else
		return CPnt(0,0);*/
}

/*int CMWnd::GetCursorBtn(CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_GetCursorBtn, NULL);
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys || !pSys->m_spInput) return 0;

	int Btn = pSys->m_spInput->GetMouseButtons();
	if (!Btn) ResetRepeatDelay();
	return Btn;
}
*/

bool CMWnd::CursorInRect(const CMWnd_Message* _pMsg, const CRct& _Rect)
{
	MAUTOSTRIP(CMWnd_CursorInRect, false);
	return _Rect.Inside(GetCursorPos(_pMsg)) != 0;
}

bool CMWnd::CursorInWnd(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_CursorInWnd, false);
	return CursorInRect(_pMsg, CRct(0, 0, m_Pos.GetWidth(), m_Pos.GetHeight()));
}


bool CMWnd::PntInWnd(const CPnt& _Pos)
{
	MAUTOSTRIP(CMWnd_PntInWnd, false);
	return CRct(0, 0, m_Pos.GetWidth(), m_Pos.GetHeight()).Inside(_Pos) != 0;
}

CPnt CMWnd::GetAbsOrigo()
{
	MAUTOSTRIP(CMWnd_GetAbsOrigo, CPnt());
	if (m_pWndParent)
		return m_pWndParent->m_AbsPos.p0;
	else
		return CPnt(0,0);
}

void CMWnd::SetPosition(const CRct& _Pos)
{
	MAUTOSTRIP(CMWnd_SetPosition, MAUTOSTRIP_VOID);
	CRct OldPos = m_Pos;

	if ((m_Status & WSTATUS_MAXIMIZED) && m_pWndParent)
		m_Pos = CRct(0, 0, m_pWndParent->GetWidth(), m_pWndParent->GetHeight());
	else
	{
		m_Pos = _Pos;
		if (OldPos.p1.x != _Pos.p1.x)
			m_Pos.p1.x = m_Pos.p0.x + GetWidth();
		else
			m_Pos.p0.x = m_Pos.p1.x - GetWidth();

		if (OldPos.p1.y != _Pos.p1.y)
			m_Pos.p1.y = m_Pos.p0.y + GetHeight();
		else
			m_Pos.p0.y = m_Pos.p1.y - GetHeight();
	}

	if (m_pWndParent)
	{
		if (m_Status & WSTATUS_ALIGNY0_TOP) m_Pos.p0.y = m_Align0.y;
		if (m_Status & WSTATUS_ALIGNY0_BOTTOM) m_Pos.p0.y = m_Align0.y + m_pWndParent->GetHeight();
		if (m_Status & WSTATUS_ALIGNX0_LEFT) m_Pos.p0.x = m_Align0.x;
		if (m_Status & WSTATUS_ALIGNX0_RIGHT) m_Pos.p0.x = m_Align0.x + m_pWndParent->GetWidth();

		if (m_Status & WSTATUS_ALIGNY1_TOP) m_Pos.p1.y = m_Align1.y;
		if (m_Status & WSTATUS_ALIGNY1_BOTTOM) 
			m_Pos.p1.y = m_Align1.y + m_pWndParent->GetHeight();
		if (m_Status & WSTATUS_ALIGNX1_LEFT) m_Pos.p1.x = m_Align1.x;
		if (m_Status & WSTATUS_ALIGNX1_RIGHT) m_Pos.p1.x = m_Align1.x + m_pWndParent->GetWidth();
	}

/*	if ((m_Pos.p0.x == OldPos.p0.x) && 
		(m_Pos.p0.y == OldPos.p0.y) && 
		(m_Pos.p1.x == OldPos.p1.x) && 
		(m_Pos.p1.y == OldPos.p1.y)) return;*/

//	M_TRACEALWAYS("Window(0x%08x) pos set: %d %d %d %d\n", this, m_Pos.p0.x, m_Pos.p0.y, m_Pos.p1.x, m_Pos.p1.y);
	

	m_AbsPos = m_Pos;
	m_AbsPos += GetAbsOrigo();

	OnMove();

	for(int i = 0; i < m_lspWndChildren.Len(); i++) 
		m_lspWndChildren[i]->OnParentMove();
//	for(int i = 0; i < m_lspWndChildren.Len(); i++)
//		m_lspWndChildren[i]->SetPosition(m_lspWndChildren[i]->m_Pos);
}


void CMWnd::SetPosition(const CPnt& _Pos)
{
	MAUTOSTRIP(CMWnd_SetPosition_2, MAUTOSTRIP_VOID);
	SetPosition(CRct(_Pos, CPnt(GetWidth(), GetHeight()) + _Pos ));
}

void CMWnd::SetPosition(int _x0, int _y0, int _x1, int _y1)
{
	MAUTOSTRIP(CMWnd_SetPosition_3, MAUTOSTRIP_VOID);
	SetPosition(CRct(_x0, _y0, _x1, _y1));
}

CPnt CMWnd::GetCenterPosition()
{
	return CPnt((m_Pos.p0.x + m_Pos.p1.x) >> 1, (m_Pos.p0.y + m_Pos.p1.y) >> 1);
}

CRct CMWnd::GetPosition()
{
	return m_Pos;
}

int CMWnd::GetWidth()
{
	MAUTOSTRIP(CMWnd_GetWidth, 0);
	return m_Pos.GetWidth();
}

int CMWnd::GetHeight()
{
	MAUTOSTRIP(CMWnd_GetHeight, 0);
	return m_Pos.GetHeight();
}

void CMWnd::SetWidth(int _w)
{
	MAUTOSTRIP(CMWnd_SetWidth, MAUTOSTRIP_VOID);
	SetPosition(CRct(m_Pos.p0, CPnt(m_Pos.p0.x + _w, m_Pos.p1.y)));
}

void CMWnd::SetHeight(int _h)
{
	MAUTOSTRIP(CMWnd_SetHeight, MAUTOSTRIP_VOID);
	SetPosition(CRct(m_Pos.p0, CPnt(m_Pos.p1.x, m_Pos.p0.y + _h)));
}

void CMWnd::Maximize()
{
	MAUTOSTRIP(CMWnd_Maximize, MAUTOSTRIP_VOID);
	if (!m_pWndParent) return;

	if (m_Status & WSTATUS_MAXIMIZED)
	{
		m_Status &= ~WSTATUS_MAXIMIZED;
		SetPosition(m_RestorePos);
	}
	else
	{
		m_Status |= WSTATUS_MAXIMIZED;
		m_RestorePos = m_Pos;
		SetPosition(m_Pos);
	}
}

void CMWnd::Minimize()
{
	MAUTOSTRIP(CMWnd_Minimize, MAUTOSTRIP_VOID);
	if (!m_pWndParent) return;
}

void CMWnd::SetStatus(int _Flag, bool _State)
{
	MAUTOSTRIP(CMWnd_SetStatus, MAUTOSTRIP_VOID);
	if (_State)
		m_Status |= _Flag;
	else
		m_Status &= ~_Flag;
}

void CMWnd::SetView(spCMWnd_View _spView)
{
	MAUTOSTRIP(CMWnd_SetView, MAUTOSTRIP_VOID);
	m_spView = _spView;
}

void CMWnd::SetBusyCallBack(void(*_pfnBusyCallBack)())
{
	MAUTOSTRIP(CMWnd_SetBusyCallBack, MAUTOSTRIP_VOID);
	m_pfnBusyCallback = _pfnBusyCallBack;
}

void CMWnd::PreEvalKeys(CMWnd_Param* _pParam)
{
	MAUTOSTRIP(CMWnd_PreEvalKeys, MAUTOSTRIP_VOID);
}

void CMWnd::PreCreate(CMWnd_Param* _pParam)
{
	MAUTOSTRIP(CMWnd_PreCreate, MAUTOSTRIP_VOID);
}

void CMWnd::OnSetAutoFocus()
{
	MAUTOSTRIP(CMWnd_OnSetAutoFocus, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_lspWndChildren.Len(); i++)
		if (m_lspWndChildren[i]->m_Style & WSTYLE_TABSTOP)
		{
			m_lspWndChildren[i]->OnSetAutoFocus();
			return;
		}
		
	if (m_lspWndChildren.Len())
	{
		m_lspWndChildren[0]->OnSetAutoFocus();
		return;
	}

	SetFocus();
}

static CPnt GetClostestPointOnRect(CRct _Rect, CPnt _Point)
{
	CPnt Ret;
	Ret.x = Max(_Rect.p0.x, Min(_Rect.p1.x, _Point.x)); 
	Ret.y = Max(_Rect.p0.y, Min(_Rect.p1.y, _Point.y)); 
	return Ret;
}

int CMWnd::OnChangeChildFocus(int _dX, int _dY)
{
	MAUTOSTRIP(CMWnd_OnChangeChildFocus, 0);
	// This method is usually run from a child that has recieved a 'tab'-key.

	CMWnd* pWndCurrent = GetChildFocusWnd();
	if (!pWndCurrent)
		return 0;

//	TArray<CMWnd*> lPotentialWindows;		// What the fuck?  -mh

	CPnt CurrentPos = pWndCurrent->GetCenterPosition();
	CRct CurrentRct = pWndCurrent->GetPosition();
/*	CRct CollideRct = CurrentRct;

	if(_dX > 0) CollideRct.p0.x = CollideRct.p1.x + 1000;
	else if(_dX < 0) CollideRct.p1.x = CollideRct.p0.x - 1000;
	
	if(_dY > 0) CollideRct.p0.y = CollideRct.p1.y + 1000;
	else if(_dY < 0) CollideRct.p1.y = CollideRct.p0.y - 1000;

	if(CollideRct.p0.x > CollideRct.p1.x) Swap(CollideRct.p0.x, CollideRct.p1.x);
	if(CollideRct.p0.y > CollideRct.p1.y) Swap(CollideRct.p0.y, CollideRct.p1.y);

	for(int i = 0; i < m_lspWndChildren.Len(); i++)
	{
		CMWnd *pWnd = m_lspWndChildren[i];
		if (pWnd == pWndCurrent || pWnd->m_Status & WSTATUS_DISABLED || !(pWnd->m_Style & WSTYLE_TABSTOP))
			continue;

		CRct Rct = pWnd->GetPosition();
		if(	CollideRct.Inside(pWnd->GetCenterPosition()) ||
			CollideRct.Inside(Rct.p0) ||
			CollideRct.Inside(Rct.p1) ||
			CollideRct.Inside(CPnt(Rct.p0.x, Rct.p1.y)) ||
			CollideRct.Inside(CPnt(Rct.p1.x, Rct.p0.y)))
		{
			lPotentialWindows.Add(pWnd);
			continue;
		}
	}
*/

	fp32 BestScore = _FP32_MAX;
	CMWnd* pWndBest = NULL;
	for(int i = 0; i < m_lspWndChildren.Len(); i++)
	{
		CMWnd *pWnd = m_lspWndChildren[i];
		if (pWnd == pWndCurrent || pWnd->m_Status & WSTATUS_DISABLED || !(pWnd->m_Style & WSTYLE_TABSTOP))
			continue;

		CPnt Pos = GetClostestPointOnRect(pWnd->GetPosition(), CurrentPos);
		CPnt Vec = Pos - CurrentPos;
		CVec2Dfp32 V(Vec.x, Vec.y);

	//	fp32 Dist = V[0] * fp32(_dX) + V[1] * fp32(_dY);
		fp32 Dist = V.Length();
		V.Normalize();

		fp32 Dot = V[0] * fp32(_dX) + V[1] * fp32(_dY);
		if (Dot < 0.1f)
			continue;

		Dot = M_Pow(Dot, 1.0f);

		Dist /= Dot;
		if(Dist < BestScore)
		{
			BestScore = Dist;
			pWndBest = pWnd;
		}
	}

	if(!pWndBest) // failed to find a window, reverting to dot.
	{
		fp32 BestScore = _FP32_MAX;
		for (int i = 0; i < m_lspWndChildren.Len(); i++)
		{
			CMWnd *pWnd = m_lspWndChildren[i];
			if (pWnd->m_Status & WSTATUS_DISABLED)
				continue;

			if (pWnd->m_Style & WSTYLE_TABSTOP)
			{
				CPnt Pos = pWnd->GetCenterPosition();
				CPnt Vec = Pos - CurrentPos;
				CVec2Dfp32 V(Vec.x, Vec.y);

				//fp32 Dist = V.Length();
				//V.Normalize();

				fp32 Dot = V[0] * fp32(_dX) + V[1] * fp32(_dY);
				if (Dot < 0.01f)
					continue;

				//Dot = M_Pow(Dot, 0.01f);

				//Dist /= Dot;
				if(Dot < BestScore)
				{
					BestScore = Dot;
					pWndBest = pWnd;
				}
			}
		}
	}

	//AR-ADD: Loop two times (if wrapping is enabled)
	//        The second run we check for positions "across the border"
	/*
	for (int j=0; j<=1; j++)
	{
		if (j == 1 && !m_bAllowChildFocusWrap)
			break;

		if (j == 1)
		{
			CurrentPos.x -= _dX * m_Pos.GetWidth();
			CurrentPos.y -= _dY * m_Pos.GetHeight();
		}

		for (int i=0; i<m_lspWndChildren.Len(); i++)
		{
			CMWnd* pWnd = m_lspWndChildren[i];
			if ((j==0 && pWnd == pWndCurrent) || pWnd->m_Status & WSTATUS_DISABLED) continue;

			if (pWnd->m_Style & WSTYLE_TABSTOP)
			{
				CPnt Pos = pWnd->GetCenterPosition();
				CPnt Vec = Pos - CurrentPos;
				CVec2Dfp32 V(Vec.x, Vec.y);

				//fp32 Dist = V.Length();
				//V.Normalize();

				fp32 Dot = V[0] * fp32(_dX) + V[1] * fp32(_dY);
				if (Dot < 0.01f)
					continue;

				//Dot = M_Pow(Dot, 0.01f);

				//Dist /= Dot;
				if(Dot < BestScore)
				{
					BestScore = Dot;
					pWndBest = pWnd;
				}
			}
		}
	}*/

	if (pWndBest) 
		pWndBest->OnSetAutoFocus();
//	else
//		ConOutL(CStrF("No valid tabstop found, pWnd %.8x, nChildren %d", this, m_lspWndChildren.Len() ));

	return 1;
}

void CMWnd::OnCreate()
{
	MAUTOSTRIP(CMWnd_OnCreate, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
	if (pCon && m_Script_Create.Len()) pCon->ExecuteString(m_Script_Create);
}

void CMWnd::OnActivate()
{
}


void CMWnd::OnDestroy()
{
	MAUTOSTRIP(CMWnd_OnDestroy, MAUTOSTRIP_VOID);
	// This can be run from the CMWnd destructor, which is very dangerous.
	// A better solution would be nice

	MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
	if (pCon && m_Script_Destroy.Len()) pCon->ExecuteString(m_Script_Destroy);
}

void CMWnd::OnParentDestroyed()
{
	MAUTOSTRIP(CMWnd_OnParentDestroyed, MAUTOSTRIP_VOID);
}

void CMWnd::OnMove()
{
	MAUTOSTRIP(CMWnd_OnMove, MAUTOSTRIP_VOID);
}

void CMWnd::UpdateAbsPos()
{
	MAUTOSTRIP(CMWnd_UpdateAbsPos, MAUTOSTRIP_VOID);
	m_AbsPos = m_Pos;
	m_AbsPos += GetAbsOrigo();
	for(int i = 0; i < m_lspWndChildren.Len(); i++)
		m_lspWndChildren[i]->UpdateAbsPos();
}

void CMWnd::OnParentMove()
{
	MAUTOSTRIP(CMWnd_OnParentMove, MAUTOSTRIP_VOID);
	if (m_Status & (WSTATUS_MAXIMIZED | WSTATUS_ALIGNMASK)) SetPosition(m_Pos);
	UpdateAbsPos();
}

void CMWnd::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	MAUTOSTRIP(CMWnd_OnPaint, MAUTOSTRIP_VOID);
}

void CMWnd::OnPaintFocus(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	MAUTOSTRIP(CMWnd_OnPaintFocus, MAUTOSTRIP_VOID);
	int Alpha = m_Col_Alpha << 24;
	int ColH = m_Col_Highlight + Alpha;
	_pRCUtil->Frame(_Clip, 0, 0, GetWidth()-1, GetHeight()-1, ColH, ColH, true);
}

int CMWnd::OnPressed(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_OnPressed, 0);
	CMWnd_Message Msg(WMSG_COMMAND, m_ID, 1, _pMsg->GetCursorBtn());
	return m_pWndParent->OnMessage(&Msg);
}

void CMWnd::OnPress(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_OnPress, MAUTOSTRIP_VOID);
	if (m_Style & WSTYLE_MOVABLE)
	{
		CPnt dMove = GetCursorPos(_pMsg) - m_GrabPos;
		if (m_pWndParent)
		{
			CMWnd_Message Msg(WMSG_WNDDRAG, m_ID, dMove.x, dMove.y);
			m_pWndParent->OnMessage(&Msg);
		}
	}
	else
	{
		if (CursorInWnd(_pMsg))
			m_Status |= WSTATUS_PRESSURE;
		else
			m_Status &= ~WSTATUS_PRESSURE;
	}

	if (m_pWndParent && CursorInWnd(_pMsg))
	{
		CMWnd_Message Msg(WMSG_COMMAND, m_ID, 2, _pMsg->GetCursorBtn());
		m_pWndParent->OnMessage(&Msg);
	}
}

int CMWnd::OnProcessScanKey(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_OnProcessScanKey, 0);
	// Scankey has not been processed by a child focus window or this 
	// window, so lets check if there's an accellerator for the scankey.
	if (_pMsg->m_Msg == WMSG_KEY)
	{
		for(int i = 0; i < m_lspAccellerators.Len(); i++)
		{
			CMWnd_Accellerator* pA = m_lspAccellerators[i];
			if (pA->m_ScanCode == _pMsg->m_Param0)
			{
				if (pA->m_FocusTarget != "")
				{
					CMWnd* pWnd = FindItem(pA->m_FocusTarget);
					if (pWnd)
						pWnd->SetFocus();
					else
						ConOutL(CStrF("(CMWnd::OnProcessScanKey) Could not find accellerator focus target: %s", (char*) pA->m_FocusTarget));
				}

				if (pA->m_Script != "")
					OnExecuteString(pA->m_Script);

				return 1;
			}
		}
	}

	CMWnd* pFocus = GetChildFocusWnd();

	// No focus?, grab the topmost window.
//	if (!pFocus && m_lspWndChildren.Len())
//		pFocus = m_lspWndChildren[0];

	// If no child focus exists or the child-focus did not process the scankey, this CMWnd get the opportunity.
	if (pFocus)
	{
		int Ret = pFocus->OnProcessScanKey(_pMsg);
		if (Ret) return Ret;
	}

	// Let this window process the scankey
	int Ret = OnMessage(_pMsg);
	if (Ret)
		return Ret;

	return 0;
}

aint CMWnd::OnMessage(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_OnMessage, 0);
	switch(_pMsg->m_Msg)
	{
	case WMSG_KEY :
		{
			if (m_Style & WSTYLE_NOKEYPROCESS) return 0;	// Let parent process key

			CScanKey Key;
			Key.m_ScanKey32 = _pMsg->m_Param0;
			CScanKey OriginalKey;
			OriginalKey.m_ScanKey32 = _pMsg->m_Param2;
			OriginalKey.m_Char = _pMsg->m_Param3;

			if (m_Status & WSTATUS_FOCUS && GetParent() && Key.IsDown())
			{
				if (Key.GetKey9() == SKEY_GUI_UP)
					return GetParent()->OnChangeChildFocus(0, -1);
				else if (Key.GetKey9() == SKEY_GUI_DOWN)
					return GetParent()->OnChangeChildFocus(0, 1);
				else if (Key.GetKey9() == SKEY_GUI_LEFT)
					return GetParent()->OnChangeChildFocus(-1, 0);
				else if (Key.GetKey9() == SKEY_GUI_RIGHT)
					return GetParent()->OnChangeChildFocus(1, 0);

				else if (Key.GetKey9() == SKEY_GUI_OK || Key.GetKey9() == SKEY_GUI_START)
					return OnPressed(_pMsg);
/*				else if (Key.GetKey9() == SKEY_GUI_BUTTON1)
					return OnPressed(_pMsg);
				else if (Key.GetKey9() == SKEY_GUI_BUTTON2)
					return OnPressed(_pMsg);
				else if (Key.GetKey9() == SKEY_GUI_BUTTON3)
					return OnPressed(_pMsg);*/
			}
		}
		break;

	case WMSG_LOCKEDCURSORMOVE :
		{
			CMWnd* pParent = GetParent();
			if (!pParent) 
				UnlockInput();
			else
				OnPress(_pMsg);
			return true;
		}

	case WMSG_LOCKEDCURSOR :
		{
			CMWnd* pParent = GetParent();
			if (!pParent) 
				UnlockInput();
			else
			{
				if (_pMsg->IsReleased() && _pMsg->IsMouse1())
				{

					OnPress(_pMsg);
					UnlockInput();
					m_Status &= ~WSTATUS_PRESSURE;
					if (CursorInWnd(_pMsg) && m_pWndParent)
					{
						// Pressed!
						return OnPressed(_pMsg);
					}
				}
				else
				{
					OnPress(_pMsg);
				}
			}
			return true;
		}

	case WMSG_CURSOR :
		{
			if (_pMsg->IsPressed() && _pMsg->IsMouse1())
			{
				m_GrabPos = GetCursorPos(_pMsg);
				if (LockInput())
					OnPress(_pMsg);
				return true;
			}
			return false;
		}

/*		if (GetCursorBtn())
		{
			CPnt GrabPos = GetCursorPos();
			m_Status |= WSTATUS_PRESSURE;

			int LastBtn = 0;
			while(GetCursorBtn())
			{
				LastBtn = GetCursorBtn();
				if (m_Style & WSTYLE_MOVABLE)
				{
					CPnt dMove = GetCursorPos() - GrabPos;
					if (m_pWndParent)
						m_pWndParent->OnMessage(&CMWnd_Message(WMSG_WNDDRAG, m_ID, dMove.x, dMove.y));
				}
				else
				{
					if (CursorInWnd())
						m_Status |= WSTATUS_PRESSURE;
					else
						m_Status &= ~WSTATUS_PRESSURE;
				}

				if (m_pWndParent && CursorInWnd())
					m_pWndParent->OnMessage(&CMWnd_Message(WMSG_COMMAND, m_ID, 2, GetCursorBtn()));
				Wait();
			}
			m_Status &= ~WSTATUS_PRESSURE;
			if (CursorInWnd() && m_pWndParent)
			{
				// Pressed!
				return m_pWndParent->OnMessage(&CMWnd_Message(WMSG_COMMAND, m_ID, 1, LastBtn));
			}
			return false;
		}*/

	case WMSG_CLOSE :
		{
			m_bQuit = true;
			return true;
		}
	}

	// Send to parent?
	if ((m_Status & WSTATUS_MSGTOPARENT) && (m_pWndParent))
		return m_pWndParent->OnMessage(_pMsg);
	else
		return false;
}

void CMWnd::OnRefresh()
{
	MAUTOSTRIP(CMWnd_OnRefresh, MAUTOSTRIP_VOID);
}

void CMWnd::OnBusy()
{
	MAUTOSTRIP(CMWnd_OnBusy, MAUTOSTRIP_VOID);
}

bool CMWnd::OnExecuteString(const CStr& _Script)
{
	MAUTOSTRIP(CMWnd_OnExecuteString, false);
	MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
	if (!pCon) return false;
	bool bException = false;
	M_TRY
	{
//		ConOut("Execute: " + _Script);
		pCon->ExecuteString(_Script);
	}
	M_CATCH(
	catch(CCException)
	{
		bException = true;
	}
	)
	return !bException;
}

void CMWnd::SoundEvent(uint _SoundEvent, const char* _pSoundName)
{
	if (_SoundEvent == WSND_USENAME && _pSoundName)
	{
		MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
		if (pCon)
			pCon->ExecuteString(CFStrF("cg_playsound(\"%s\")", _pSoundName));
	}
}

int CMWnd::DoModal()
{
	MAUTOSTRIP(CMWnd_DoModal, 0);
	m_pWndRoot->BringToTop(this);
	m_ExitCode = 0;
	m_bQuit = false;
	while(!m_bQuit)
	{
		FetchInput();
		Wait();
	}
	Destroy();
	return m_ExitCode;
}


CStr CMWnd::OnGetDumpInformation()
{
	MAUTOSTRIP(CMWnd_OnGetDumpInformation, CStr());
	return CStrF("%s, (%d,%d,%d,%d), Style %4x, Status %8x, ID '%s', ", (char*) MRTC_ClassName(), m_Pos.p0.x, m_Pos.p0.y, m_Pos.p1.x, m_Pos.p1.y, m_Style, m_Status, (m_ID != "") ? (char*)m_ID : "");
}

void CMWnd::OnDumpTree(int _iLevel)
{
	MAUTOSTRIP(CMWnd_OnDumpTree, MAUTOSTRIP_VOID);
	CStr s(char(8), _iLevel);
	LogFile(s + OnGetDumpInformation());
	if (m_lspWndChildren.Len())
	{
		LogFile(s + "{");
		for(int i = 0; i < m_lspWndChildren.Len(); i++)
			m_lspWndChildren[i]->OnDumpTree(_iLevel + 1);
		LogFile(s + "}");
	}
}

// -------------------------------------------------------------------
//  CMWnd_Caption
// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd_Caption, CMWnd);


void CMWnd_Caption::Clear()
{
	MAUTOSTRIP(CMWnd_Caption_Clear, MAUTOSTRIP_VOID);
	m_pWndClose = NULL;
	m_pWndMax = NULL;
	m_pWndMin = NULL;
}

void CMWnd_Caption::DoCreate(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID)
{
	MAUTOSTRIP(CMWnd_Caption_DoCreate, MAUTOSTRIP_VOID);
	CMWnd::DoCreate(_pWndParent, _Param, _ID);

	if (m_Style & WSTYLE_CAPTN_CLOSEBTN)
		m_pWndClose = MNew9(CMWnd_Button, this, 
		CMWnd_Param(0,0,0,0, 0), 
		"WND_CLOSE", "", 0, "WND_CLOSE", CPnt(2, 2), CPnt(0, 0), WSTYLE_BTNIMG_STRETCHFITX | WSTYLE_BTNIMG_STRETCHFITY |WSTYLE_BTNIMG_CENTERX | WSTYLE_BTNIMG_CENTERY);

	if (m_Style & WSTYLE_CAPTN_MAXBTN)
		m_pWndMax = MNew9(CMWnd_Button, this, CMWnd_Param(0,0,0,0, 0), "WND_MAXIMIZE", "", 0, 
			"WND_MAXIMIZE", CPnt(2, 2), CPnt(0, 0), WSTYLE_BTNIMG_STRETCHFITX | WSTYLE_BTNIMG_STRETCHFITY |
			WSTYLE_BTNIMG_CENTERX | WSTYLE_BTNIMG_CENTERY);

	if (m_Style & WSTYLE_CAPTN_MINBTN)
		m_pWndMin = MNew9(CMWnd_Button, this, CMWnd_Param(0,0,0,0, 0), "WND_MINIMIZE", "", 0, 
			"WND_MINIMIZE", CPnt(2, 2), CPnt(0, 0), WSTYLE_BTNIMG_STRETCHFITX | WSTYLE_BTNIMG_STRETCHFITY |
			WSTYLE_BTNIMG_CENTERX | WSTYLE_BTNIMG_CENTERY);

	SetPosition(m_Pos);
}

void CMWnd_Caption::DoDestroy()
{
	MAUTOSTRIP(CMWnd_Caption_DoDestroy, MAUTOSTRIP_VOID);
	// Destroy stuff here:

	CMWnd::DoDestroy();
}

void CMWnd_Caption::Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, const CStr& _Caption)
{
	MAUTOSTRIP(CMWnd_Caption_Create, MAUTOSTRIP_VOID);
	m_Caption = _Caption;
	CMWnd::Create(_pWndParent, _Param, _ID);
}

CMWnd_Caption::CMWnd_Caption(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, const CStr& _Caption)
{
	MAUTOSTRIP(CMWnd_Caption_ctor, MAUTOSTRIP_VOID);
	Clear();
	Create(_pWndParent, _Param, _ID, _Caption);
}

CMWnd_Caption::CMWnd_Caption()
{
	MAUTOSTRIP(CMWnd_Caption_ctor_2, MAUTOSTRIP_VOID);
	Clear();
}

int CMWnd_Caption::GetHeight()
{
	MAUTOSTRIP(CMWnd_Caption_GetHeight, 0);
	return (m_Style & WSTYLE_CAPTN_TOOLWINDOW) ? 12 : 15;
}

void CMWnd_Caption::OnMove()
{
	MAUTOSTRIP(CMWnd_Caption_OnMove, MAUTOSTRIP_VOID);
	int y0 = 1;
	int h = GetHeight() - 2;
	int x1 = GetWidth() - 2 - h;

	if (m_pWndClose)
	{
		m_pWndClose->SetPosition(CRct(x1, y0, x1+h, y0+h));
		x1 -= h;
	}

	if (m_pWndMax)
	{
		m_pWndMax->SetPosition(CRct(x1, y0, x1+h, y0+h));
		x1 -= h;
	}

	if (m_pWndMin)
	{
		m_pWndMin->SetPosition(CRct(x1, y0, x1+h, y0+h));
		x1 -= h;
	}
}

void CMWnd_Caption::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	MAUTOSTRIP(CMWnd_Caption_OnPaint, MAUTOSTRIP_VOID);
	int Alpha = m_Col_Alpha << 24;
	int ColH = m_Col_Highlight + Alpha;
//	int ColM = m_Col_Background + Alpha;
	int ColD = m_Col_Shadow + Alpha;
//	int ColDD = m_Col_DarkShadow + Alpha;

	int CaptionH = GetHeight();
	int x0 = 0;
	int y0 = 0;
	int x1 = GetWidth();

	int bDown = (m_Status & WSTATUS_PRESSURE) ? 1 : 0;

	int CapCol = (m_Status & WSTATUS_FOCUS) ? m_Col_Custom0_Focus : m_Col_Custom0;
	_pRCUtil->SetTexture("WINBG1");
	_pRCUtil->SetTextureOrigo(_Clip, CPnt(0, 0));
	if (bDown)
		_pRCUtil->Frame(_Clip, x0, y0, x1, y0+CaptionH, ColD, ColH, true);
	else
		_pRCUtil->Frame(_Clip, x0, y0, x1, y0+CaptionH, ColH, ColD);

	_pRCUtil->Rect(_Clip, CRct(x0+1, y0+1, x1-1, y0+CaptionH-1), CapCol + Alpha);
	_pRCUtil->SetTexture(0);

	CRC_Font* pF = GetFont("SYSTEM");
	if (pF) 
	{
		CStr s = m_Caption;
		int y = (m_Style & WSTYLE_CAPTN_TOOLWINDOW) ? (y0 + 1) : (y0 + 3);
//		y += bDown;
		int x = x0 /*+ bDown*/;
		_pRCUtil->Text(_Clip, pF, x+5, y+2, s, 0x60ffffff);
		_pRCUtil->Text(_Clip, pF, x+3, y, s, 0x60000000);
		_pRCUtil->Text(_Clip, pF, x+4, y+1, s, Alpha);
	}
}

aint CMWnd_Caption::OnMessage(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_Caption_OnMessage, 0);
	switch(_pMsg->m_Msg)
	{
	case WMSG_CURSOR :
		{
			if (m_pWndParent)
			{
				if ((m_Style & WSTYLE_CAPTN_MAXBTN) && _pMsg->IsPressed() && _pMsg->IsMouse2())
				{
/*					m_Status |= WSTATUS_PRESSURE;
					while(GetCursorBtn()) Wait();
					m_Status &= ~WSTATUS_PRESSURE;*/
					CMWnd_Message Msg(WMSG_COMMAND, "WND_MAXIMIZE", 1);
					m_pWndParent->OnMessage(&Msg);
				}
				else
					CMWnd::OnMessage(_pMsg);


/*				if (m_Style & WSTYLE_MOVABLE)
				{
					CPnt GrabPos = GetCursorPos();
					m_Status |= WSTATUS_PRESSURE;

					while(GetCursorBtn())
					{
						CPnt dMove = GetCursorPos() - GrabPos;
						CPnt NewPos = m_pWndParent->GetPosition().p0 + dMove;
						if (m_pWndParent)
							m_pWndParent->OnMessage(&CMWnd_Message(WMSG_WNDDRAG, m_ID, dMove.x, dMove.y));
//						m_pWndParent->SetPosition(NewPos);
						m_pWndParent->OnMessage(&CMWnd_Message(WMSG_COMMAND, m_ID, 2, GetCursorBtn()));
						Wait();
					}

					m_Status &= ~WSTATUS_PRESSURE;
				}*/
			}
			return true;
		}
		break;

	case WMSG_COMMAND :
		if (m_pWndParent) return m_pWndParent->OnMessage(_pMsg);
		break;

	default :
		return CMWnd::OnMessage(_pMsg);
	}
	return false;
}

// -------------------------------------------------------------------
//  CMWnd_Text
// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd_Text, CMWnd);


void CMWnd_Text::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	MAUTOSTRIP(CMWnd_Text_EvaluateKey, MAUTOSTRIP_VOID);
	
	CStr Value = _Value;
	
	if (_Key == "TEXT")
	{
		m_Text = _Value;
	}
	else if (_Key == "FONT")
		m_Font = _Value;
	else if (_Key == "STYLE")
	{
		while(Value.Len())
		{
			CStr s = Value.GetStrSep(",").LTrim().RTrim();
			if (s == "TEXT_CUTOUT")
				_pParam->m_Style |= WSTYLE_TEXT_CUTOUT;
			else if (s == "TEXT_SHADOW")
				_pParam->m_Style |= WSTYLE_TEXT_SHADOW;
			else if (s == "TEXT_HIGHLIGHT")
				_pParam->m_Style |= WSTYLE_TEXT_HIGHLIGHT;
			else if (s == "TEXT_CENTER")
				_pParam->m_Style |= WSTYLE_TEXT_CENTER;
			else if (s == "TEXT_CENTERY")
				_pParam->m_Style |= WSTYLE_TEXT_CENTERY;
			else if (s == "TEXT_WORDWRAP")
				_pParam->m_Style |= WSTYLE_TEXT_WORDWRAP;
			else if (s == "TEXT_OUTLINE")
				m_Outline = true;
			else
				CMWnd::EvaluateKey(_pParam, _Key, s);
				// Send any unknown styles to baseclass for eval.
		}
	}
	else
		CMWnd::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_Text::Clear()
{
	MAUTOSTRIP(CMWnd_Text_Clear, MAUTOSTRIP_VOID);
	m_Col_Highlight = 0x60ffffff;
	m_Col_Shadow = 0xff000000;
	m_Col_Background = 0xffffffff;
	m_Col_DarkShadow = 0xff000000;
	m_Outline = false;
}

CMWnd_Text::CMWnd_Text()
{
	MAUTOSTRIP(CMWnd_Text_ctor, MAUTOSTRIP_VOID);
	m_Font = "SYSTEM";
	Clear();
}

CMWnd_Text::CMWnd_Text(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, const CStr& _Text)
{
	MAUTOSTRIP(CMWnd_Text_ctor_2, MAUTOSTRIP_VOID);
	Clear();
	m_Text = _Text;
	CMWnd::Create(_pWndParent, _Param, _ID);
}

aint CMWnd_Text::OnMessage(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_Text_OnMessage, 0);
	switch(_pMsg->m_Msg)
	{
		case WMSG_SETTEXT:
		{
			m_Text = (char *)_pMsg->m_Param0;
			return true;
		}
	}

	return CMWnd::OnMessage(_pMsg);
}

void CMWnd_DrawText(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, CRC_Font* _pF, wchar* _pStr, int _x, int _y,
	int _Style, int ColM, int ColH, int ColD)
{
	MAUTOSTRIP(CMWnd_DrawText, MAUTOSTRIP_VOID);
	_pRCUtil->Text_Draw(_Clip, _pF, _pStr, _x, _y, _Style, ColM, ColH, ColD);
}

int CMWnd_Text_WordWrap(CRC_Util2D* _pRCUtil, CRC_Font* _pF, int _Width, wchar* _pStr, int _Len, wchar** _ppLines, int _MaxLines)
{
	MAUTOSTRIP(CMWnd_Text_WordWrap, 0);
	return _pRCUtil->Text_WordWrap(_pF, _Width, _pStr, _Len, _ppLines, _MaxLines);
}

int CMWnd_Text_DrawFormated(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, CRC_Font* _pF, const CStr& _Text, int _x, int _y,
	int _Style, int _ColM, int _ColH, int _ColD, int _Width, int _Height)
{
	MAUTOSTRIP(CMWnd_Text_DrawFormated, 0);
		return _pRCUtil->Text_DrawFormatted(_Clip, _pF, _Text, _x, _y, _Style, _ColM, _ColH, _ColD, _Width, _Height, false);
}

int CMWnd_Text_DrawFormated(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, CRC_Font* _pF, const CStr& _Text, int& _x, int& _y,
	int _Style, int _ColM, int _ColH, int _ColD, int _Width, int _Height, fp32 _PercentVis)
{
	MAUTOSTRIP(CMWnd_Text_DrawFormated_2, 0);
	return _pRCUtil->Text_DrawFormatted(_Clip, _pF, _Text, _x, _y, _Style, _ColM, _ColH, _ColD, _Width, _Height, _PercentVis);
}


void CMWnd_Text::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Text)
{
	MAUTOSTRIP(CMWnd_Text_OnPaint, MAUTOSTRIP_VOID);
	int ColH = m_Col_Highlight;
	int ColM = /*m_Col_Background;*/ CPixel32(95*2,90*2,70*2, 0xff);
	int ColD = m_Col_Shadow;
//	int ColDD = m_Col_DarkShadow;

	CXR_VBManager* pVBM = _pRCUtil->GetVBM();

	CRct Rect = pVBM->Viewport_Get()->GetViewRect();
	int VPWidth = Rect.GetWidth();
	int VPHeight = Rect.GetHeight();

	_pRCUtil->SetCoordinateScale(CVec2Dfp32(VPHeight / 480.0f, VPHeight / 480.0f));

	CRC_Font* pF = GetFont(m_Font);
	if (pF) 
	{
		if(m_Outline)
		{
			CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, m_Text, 0, -1, m_Style, ColD, ColD, ColD, m_Pos.GetWidth(), m_Pos.GetHeight());
			CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, m_Text, 0,  1, m_Style, ColD, ColD, ColD, m_Pos.GetWidth(), m_Pos.GetHeight());
			CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, m_Text,-1,  0, m_Style, ColD, ColD, ColD, m_Pos.GetWidth(), m_Pos.GetHeight());
			CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, m_Text, 1,  0, m_Style, ColD, ColD, ColD, m_Pos.GetWidth(), m_Pos.GetHeight());
		}
		CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, m_Text, 0, 0,
			m_Style, ColM, ColH, ColD, m_Pos.GetWidth(), m_Pos.GetHeight());
	}

	_pRCUtil->SetCoordinateScale(CVec2Dfp32(VPWidth / 640.0f , VPHeight / 480.0f));
}

// -------------------------------------------------------------------
//  CMWnd_Client
// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd_Client, CMWnd);


void CMWnd_Client::Clear()
{
	MAUTOSTRIP(CMWnd_Client_Clear, MAUTOSTRIP_VOID);
	m_MinClientSize = CPnt(0,0);
	m_pWndScrollX = NULL;
	m_pWndScrollY = NULL;
	m_pWndClient = NULL;
	m_pWndClipClient = NULL;
	m_pWndResizePad = NULL;
}

void CMWnd_Client::DoCreate(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID)
{
	MAUTOSTRIP(CMWnd_Client_DoCreate, MAUTOSTRIP_VOID);
	CMWnd::DoCreate(_pWndParent, _Param, _ID);

	if (m_Style & (WSTYLE_CLIENT_SCROLLX | WSTYLE_CLIENT_SCROLLY))
	{
		m_pWndClipClient = MNew(CMWnd_Client);
		m_pWndClipClient->Create(NULL, CMWnd_Param(0,0,0,0, WSTYLE_BEHIND | WSTYLE_TABSTOP | WSTYLE_NOKEYPROCESS, WSTATUS_MSGTOPARENT | WSTATUS_INVISIBLE), "");
		CMWnd::AddChild(m_pWndClipClient);

		{
//			m_pWndClient = DNew(CMWnd_Client) CMWnd_Client;
			spCMWnd spClient = CreateClient();
			spClient->Create(m_pWndClipClient, CMWnd_Param(0,0,0,0, WSTYLE_BEHIND | WSTYLE_NOKEYPROCESS, WSTATUS_MSGTOPARENT), "");
			m_pWndClient = spClient;
			// spClient out of scope, should be ok.
		}

		if (m_Style & WSTYLE_CLIENT_SCROLLX)
		{
			m_pWndScrollX = MNew(CMWnd_DragbarX);
			m_pWndScrollX->Create(NULL, CMWnd_Param(0,0,0,0, WSTYLE_ONTOP | WSTYLE_NOKEYPROCESS), "CLIENT_DRAGX");
			CMWnd::AddChild(m_pWndScrollX);
		}

		if (m_Style & WSTYLE_CLIENT_SCROLLY)
		{
			m_pWndScrollY = MNew(CMWnd_DragbarY);
			m_pWndScrollY->Create(NULL, CMWnd_Param(0,0,0,0, WSTYLE_ONTOP | WSTYLE_NOKEYPROCESS), "CLIENT_DRAGY");
			CMWnd::AddChild(m_pWndScrollY);
		}

		if (m_Style & WSTYLE_CLIENT_RESIZEPAD)
		{
			m_pWndResizePad = MNew(CMWnd_ResizePad);
			m_pWndResizePad->Create(NULL, CMWnd_Param(0,0,0,0, WSTYLE_ONTOP | WSTYLE_NOKEYPROCESS), "CLIENT_RESIZE");
			CMWnd::AddChild(m_pWndResizePad);
		}

		SetPosition(m_Pos);
	}
}

spCMWnd CMWnd_Client::CreateClient()
{
	MAUTOSTRIP(CMWnd_Client_CreateClient, NULL);
	return MNew(CMWnd);
}

void CMWnd_Client::MakeVisible(const CRct& _Area)
{
	MAUTOSTRIP(CMWnd_Client_MakeVisible, MAUTOSTRIP_VOID);
	int ViewW = m_pWndClipClient->GetWidth();
	int ViewH = m_pWndClipClient->GetHeight();
	if (_Area.p1.x > m_ClientPos.x+ViewW)
		m_ClientPos.x = _Area.p1.x-ViewW;
	if (_Area.p1.y > m_ClientPos.y+ViewH)
		m_ClientPos.y = _Area.p1.y-ViewH;
	if (_Area.p0.x < m_ClientPos.x)
		m_ClientPos.x = _Area.p0.x;
	if (_Area.p0.y < m_ClientPos.y)
		m_ClientPos.y = _Area.p0.y;

	if (m_pWndScrollX) m_pWndScrollX->SetDragBarPos(m_ClientPos.x);
	if (m_pWndScrollY) m_pWndScrollY->SetDragBarPos(m_ClientPos.y);

	OnMove();
}

void CMWnd_Client::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	MAUTOSTRIP(CMWnd_Client_EvaluateKey, MAUTOSTRIP_VOID);
	
	CStr Value = _Value;
	
	if (_Key == "CLIENTSIZE")
	{
		m_MinClientSize.x = Value.GetStrSep(",").Val_int();
		m_MinClientSize.y = Value.GetStrSep(",").Val_int();
	}
	else if (_Key == "STYLE")
	{
		while(Value.Len())
		{
			CStr s = Value.GetStrSep(",").LTrim().RTrim();
			if (s == "CLIENT_SCROLLX")
				_pParam->m_Style |= WSTYLE_CLIENT_SCROLLX;
			else if (s == "CLIENT_SCROLLY")
				_pParam->m_Style |= WSTYLE_CLIENT_SCROLLY;
			else if (s == "CLIENT_BORDER")
				_pParam->m_Style |= WSTYLE_CLIENT_BORDER;
			else if (s == "CLIENT_RESIZEPAD")
				_pParam->m_Style |= WSTYLE_CLIENT_RESIZEPAD;
			else
				CMWnd::EvaluateKey(_pParam, _Key, s);
				// Send any unknown styles to baseclass for eval.
		}
	}
	else
		CMWnd::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_Client::Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, const CPnt& _Size)
{
	MAUTOSTRIP(CMWnd_Client_Create, MAUTOSTRIP_VOID);
	m_MinClientSize = _Size;
	CMWnd::Create(_pWndParent, _Param, _ID);
}

CMWnd_Client::CMWnd_Client()
{
	MAUTOSTRIP(CMWnd_Client_ctor, MAUTOSTRIP_VOID);
	Clear();
}

CMWnd_Client::CMWnd_Client(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, const CPnt& _Size)
{
	MAUTOSTRIP(CMWnd_Client_ctor_2, MAUTOSTRIP_VOID);
	Clear();
	Create(_pWndParent, _Param, _ID, _Size);
}

void CMWnd_Client::AddChild(spCMWnd _spWnd)
{
	MAUTOSTRIP(CMWnd_Client_AddChild, MAUTOSTRIP_VOID);
	if (m_pWndClient)
		m_pWndClient->AddChild(_spWnd);
	else
		CMWnd::AddChild(_spWnd);
}

CMWnd* CMWnd_Client::GetItem(const CStr& _ID)
{
	MAUTOSTRIP(CMWnd_Client_GetItem, NULL);
	if (m_pWndClient)
		return m_pWndClient->GetItem(_ID);
	else
		return CMWnd::GetItem(_ID);
}

void CMWnd_Client::SetView(spCMWnd_View _spView)
{
	MAUTOSTRIP(CMWnd_Client_SetView, MAUTOSTRIP_VOID);
//	m_spClientView = _spView;
	if (m_pWndClient)
		m_pWndClient->SetView(_spView);
	else
		m_spView = _spView;
}

void CMWnd_Client::OnMove()
{
	MAUTOSTRIP(CMWnd_Client_OnMove, MAUTOSTRIP_VOID);
	if (m_Style & (WSTYLE_CLIENT_SCROLLX | WSTYLE_CLIENT_SCROLLY))
	{
		int x0 = 0;
		int y0 = 0;
		int x1 = m_Pos.GetWidth();
		int y1 = m_Pos.GetHeight();

		if (m_Style & WSTYLE_CLIENTEDGE)
			{ x0++; y0++; x1--; y1--;}
		if (m_Style & WSTYLE_CLIENT_BORDER)
			{ x0++; y0++; x1--; y1--;}

		int bScrX = (m_MinClientSize.x > (x1-x0));
		int bScrY = (m_MinClientSize.y > (y1-y0));
		if (bScrY) bScrX = (m_MinClientSize.x > (x1-x0-15));
		if (bScrX) bScrY = (m_MinClientSize.y > (y1-y0-15));

		if (m_pWndResizePad)
		{
			int bRP = (bScrX && bScrY);
			m_pWndResizePad->SetStatus(WSTATUS_DISABLED, !bRP);
			if (bRP)
			{
				m_pWndResizePad->SetPosition(x1-15, y1-15, x1, y1);
			}
		}

		if (m_pWndScrollX)
		{
			m_pWndScrollX->SetStatus(WSTATUS_DISABLED, !bScrX);
			if (bScrX)
			{
				m_pWndScrollX->SetPosition(x0, y1-15, x1 - ((bScrY) ? 15 : 0), y1);
				y1 -= 15;
			}
		}

		if (m_pWndScrollY)
		{
			m_pWndScrollY->SetStatus(WSTATUS_DISABLED, !bScrY);
			if (bScrY)
			{
				m_pWndScrollY->SetPosition(x1-15, y0, x1, y1);
				x1 -= 15;
			}
		}

		if (m_pWndScrollX && bScrX)
		{
			m_pWndScrollX->Drag_SetWindow(0, m_MinClientSize.x, x1-x0);
			m_ClientPos.x = int(m_pWndScrollX->Drag_GetPos());
		}
		else
			m_ClientPos.x = 0;

		if (m_pWndScrollY && bScrY)
		{
			m_pWndScrollY->Drag_SetWindow(0, m_MinClientSize.y, y1-y0);
			m_ClientPos.y = int(m_pWndScrollY->Drag_GetPos());
		}
		else
			m_ClientPos.y = 0;

		if (m_pWndClipClient && m_pWndClient)
		{
			m_pWndClipClient->SetPosition(x0, y0, x1, y1);

			CPnt p(-m_ClientPos.x, -m_ClientPos.y);
			CPnt p2(Max(m_pWndClipClient->GetWidth(), m_MinClientSize.x),
				Max(m_pWndClipClient->GetHeight(), m_MinClientSize.y));

			m_pWndClient->SetPosition(CRct(p, p+p2));
		}
	}
}

void CMWnd_Client::OnParentMove()
{
	MAUTOSTRIP(CMWnd_Client_OnParentMove, MAUTOSTRIP_VOID);
	CMWnd::OnParentMove();
	if (m_pWndClipClient) 
		m_pWndClipClient->OnParentMove();
	else
		if (m_pWndClient) m_pWndClient->OnParentMove();
}

void CMWnd_Client::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	MAUTOSTRIP(CMWnd_Client_OnPaint, MAUTOSTRIP_VOID);
	int Alpha = m_Col_Alpha << 24;
	int ColH = m_Col_Highlight + Alpha;
	int ColM = m_Col_Background + Alpha;
	int ColD = m_Col_Shadow + Alpha;
	int ColDD = m_Col_DarkShadow + Alpha;

	int Col = ColM;
	if (m_Status & WSTATUS_PRESSURE) Col = ColD;
		
	int x0 = 0;
	int y0 = 0;
	int x1 = _Client.GetWidth();
	int y1 = _Client.GetHeight();

	if (m_Style & WSTYLE_CLIENTEDGE)
		_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, ColD, ColH, true);

	if (m_Style & WSTYLE_CLIENT_BORDER)
		_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, ColDD, ColDD);

	if ((!m_pWndClient) && (!m_pWndClipClient))
		_pRCUtil->Rect(_Clip, CRct(x0, y0, x1, y1), Col);
}

aint CMWnd_Client::OnMessage(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_Client_OnMessage, 0);
	switch(_pMsg->m_Msg)
	{
	case WMSG_COMMAND : 
		if (_pMsg->m_pID)
		{
			if (!strcmp(_pMsg->m_pID, "CLIENT_DRAGX") && (_pMsg->m_Param0 == 0))
			{
				m_ClientPos.x = int(_pMsg->m_fParam0);
				SetPosition(m_Pos);
			}
			else if (!strcmp(_pMsg->m_pID, "CLIENT_DRAGY") && (_pMsg->m_Param0 == 0))
			{
				m_ClientPos.y = int(_pMsg->m_fParam0);
				SetPosition(m_Pos);
			}
			else

			{
				if (m_Status & WSTATUS_MSGTOPARENT)
				{	if (m_pWndParent) return m_pWndParent->OnMessage(_pMsg);}
				else
					return CMWnd::OnMessage(_pMsg);
			}
			return false;
		}
		break;
	}

	if (m_Status & WSTATUS_MSGTOPARENT)
	{
		if (m_pWndParent) return m_pWndParent->OnMessage(_pMsg);
	}
	else
	{
		return CMWnd::OnMessage(_pMsg);
	}
	return false;
}

void CMWnd_Client::SetClientSize(int _w, int _h)
{
	MAUTOSTRIP(CMWnd_Client_SetClientSize, MAUTOSTRIP_VOID);
	if ((m_MinClientSize.x == _w) && (m_MinClientSize.x == _h)) return;
	m_MinClientSize.x = _w;
	m_MinClientSize.y = _h;
	CMWnd_Client::OnMove();
}

// -------------------------------------------------------------------
//  CMWnd_Splitbar
// -------------------------------------------------------------------

void CMWnd_Splitbar::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	MAUTOSTRIP(CMWnd_Splitbar_OnPaint, MAUTOSTRIP_VOID);
	int Alpha = m_Col_Alpha << 24;
	int ColH = m_Col_Highlight + Alpha;
	int ColM = m_Col_Background + Alpha;
	int ColD = m_Col_Shadow + Alpha;
//	int ColDD = m_Col_DarkShadow + Alpha;

	int x0 = 0;
	int y0 = 0;
	int x1 = m_Pos.GetWidth()-0;
	int y1 = m_Pos.GetHeight();

/*	int bDown = (m_Status & WSTATUS_PRESSURE) ? 1 : 0;

	int CapCol = (m_Status & WSTATUS_FOCUS) ? m_Col_Custom0_Focus : m_Col_Custom0;
	_pRCUtil->SetTexture("WINBG1");
	_pRCUtil->SetTextureOrigo(_Clip, CPnt(0, 0));
	if (bDown)
		_pRCUtil->Frame(_Clip, x0, y0, x1, y1, ColD, ColH, true);
	else
		_pRCUtil->Frame(_Clip, x0, y0, x1, y1, ColH, ColD);

	_pRCUtil->Rect(_Clip, CRct(x0+1, y0+1, x1-1, y1-1), CapCol + Alpha);
	_pRCUtil->SetTexture(0);*/

	int bDown = (m_Status & WSTATUS_PRESSURE) ? 1 : 0;
	if (bDown)
	{
//		_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, ColDD, ColDD);
//		_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, ColD, ColH, true);
		_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, ColD, ColM, true);
		_pRCUtil->Rect(_Clip, CRct(x0, y0, x1, y1), ColM);
	}
	else
	{
//		_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, ColDD, ColDD);
//		_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, ColD, ColH, true);
		_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, ColH, ColD);
		_pRCUtil->Rect(_Clip, CRct(x0, y0, x1, y1), ColM);
	}
//	_pRCUtil->Rect(_Clip, _Client, (m_Status & WSTATUS_PRESSURE) ? 0xff8080c0 : 0xff404040);

}

// -------------------------------------------------------------------
//  CMWnd_SplitWnd
// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd_SplitWnd, CMWnd);


void CMWnd_SplitWnd::DoCreate(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID)
{
	MAUTOSTRIP(CMWnd_SplitWnd_DoCreate, MAUTOSTRIP_VOID);
	CMWnd::DoCreate(_pWndParent, _Param, _ID);

	if (m_SplitPos < 0) m_SplitPos = (m_Style & WSTYLE_SPLITWND_Y) ? (GetHeight() >> 1) : (GetWidth() >> 1);
	m_pWndDrag = MNew(CMWnd_Splitbar);
	if (m_Style & WSTYLE_SPLITWND_Y)
		m_pWndDrag->Create(NULL, CMWnd_Param(0, m_SplitPos, 0, 4, WSTYLE_BEHIND | WSTYLE_MOVABLE), "SPLITBAR");
	else
		m_pWndDrag->Create(NULL, CMWnd_Param(m_SplitPos, 0, 4, 0, WSTYLE_BEHIND | WSTYLE_MOVABLE), "SPLITBAR");
	AddChild(m_pWndDrag);
}

void CMWnd_SplitWnd::Clear()
{
	MAUTOSTRIP(CMWnd_SplitWnd_Clear, MAUTOSTRIP_VOID);
	m_pSplitWnd1 = NULL;
	m_pSplitWnd2 = NULL;
	m_pWndDrag = NULL;
	m_SplitPos = -1;
}

void CMWnd_SplitWnd::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	MAUTOSTRIP(CMWnd_SplitWnd_EvaluateKey, MAUTOSTRIP_VOID);
	
	CStr Value = _Value;
	
	if (_Key == "SPLITPOS")
	{
		m_SplitPos = _Value.Val_int();
	}
	else if (_Key == "SPLITWND1")
	{
		m_SplitWnd1ID = _Value;
	}
	else if (_Key == "SPLITWND2")
	{
		m_SplitWnd2ID = _Value;
	}
	else if (_Key == "STYLE")
	{
		while(Value.Len())
		{
			CStr s = Value.GetStrSep(",").LTrim().RTrim();
			if (s == "SPLITWND_Y")
				_pParam->m_Style |= WSTYLE_SPLITWND_Y;
			else
				CMWnd::EvaluateKey(_pParam, _Key, s);
				// Send any unknown styles to baseclass for eval.
		}
	}
	else
		CMWnd::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_SplitWnd::PreCreate(CMWnd_Param* _pParam)
{
	MAUTOSTRIP(CMWnd_SplitWnd_PreCreate, MAUTOSTRIP_VOID);
	CMWnd::PreCreate(_pParam);
	_pParam->m_Status |= WSTATUS_MSGTOPARENT;
}

void CMWnd_SplitWnd::Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, 
	spCMWnd _spWnd1, spCMWnd _spWnd2, int _SplitPos)
{
	MAUTOSTRIP(CMWnd_SplitWnd_Create, MAUTOSTRIP_VOID);
	m_pSplitWnd1 = _spWnd1;
	m_pSplitWnd2 = _spWnd2;
	m_SplitPos = _SplitPos;
	CMWnd::Create(_pWndParent, _Param, _ID);
}

CMWnd_SplitWnd::CMWnd_SplitWnd()
{
	MAUTOSTRIP(CMWnd_SplitWnd_ctor, MAUTOSTRIP_VOID);
	Clear();
}

CMWnd_SplitWnd::CMWnd_SplitWnd(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID,
	spCMWnd _spWnd1, spCMWnd _spWnd2, int _SplitPos)
{
	MAUTOSTRIP(CMWnd_SplitWnd_CMWnd_SplitWnd, MAUTOSTRIP_VOID);
	Clear();
	Create(_pWndParent, _Param, _ID, _spWnd1, _spWnd2, _SplitPos);
}

void CMWnd_SplitWnd::OnCreate()
{
	MAUTOSTRIP(CMWnd_SplitWnd_OnCreate, MAUTOSTRIP_VOID);
	if (!m_pSplitWnd1) 
		m_pSplitWnd1 = GetItem(m_SplitWnd1ID);
	else
		AddChild(m_pSplitWnd1);

	if (!m_pSplitWnd2) 
		m_pSplitWnd2 = GetItem(m_SplitWnd2ID);
	else
		AddChild(m_pSplitWnd2);

	OnMove();
}

void CMWnd_SplitWnd::OnMove()
{
	MAUTOSTRIP(CMWnd_SplitWnd_OnMove, MAUTOSTRIP_VOID);
	if (!m_pWndDrag) return;

	if (m_Style & WSTYLE_SPLITWND_Y)
	{
		int bary = m_pWndDrag->GetPosition().p0.y;
		int barh = m_pWndDrag->GetHeight();
		if (m_SplitPos < 0) m_SplitPos = 0;
		if (barh > GetHeight())
			m_SplitPos = 0;
		else
			if (m_SplitPos + barh > GetHeight())
				m_SplitPos = GetHeight() - barh;

		int x0 = 0;
		int y0 = 0;
		int x1 = GetWidth();
		int y1 = GetHeight();

		if (m_Style & WSTYLE_CLIENTEDGE)
			{ x0 += 1; y0 += 1; x1 -= 1; y1 -= 1; }

		bary = m_SplitPos;
		if (m_pSplitWnd1) m_pSplitWnd1->SetPosition(x0, y0, x1, bary);
		int h1 = (m_pSplitWnd1) ? m_pSplitWnd1->GetHeight() : bary-y0;
		if (m_pSplitWnd2) m_pSplitWnd2->SetPosition(x0, bary+barh, x1, y1);
		int h2 = (m_pSplitWnd2) ? m_pSplitWnd2->GetHeight() : y1-bary-barh;

		if (m_pSplitWnd2) m_pSplitWnd2->SetPosition(x0, y1-h2, x1, y1);
		h1 = y1-h2-barh;
		if (m_pSplitWnd1) m_pSplitWnd1->SetPosition(x0, y0, x1, h1);

		m_pWndDrag->SetStatus(WSTATUS_DISABLED, h1+h2 > y1-y0);
		m_pWndDrag->SetPosition(x0, h1, x1, h1+barh);
	}
	else
	{
		int barx = m_pWndDrag->GetPosition().p0.x;
		int barw = m_pWndDrag->GetWidth();
		if (m_SplitPos < 0) m_SplitPos = 0;
		if (barw > GetWidth())
			m_SplitPos = 0;
		else
			if (m_SplitPos + barw > GetWidth())
				m_SplitPos = GetWidth() - barw;

		int x0 = 0;
		int y0 = 0;
		int x1 = GetWidth();
		int y1 = GetHeight();

		if (m_Style & WSTYLE_CLIENTEDGE)
			{ x0 += 1; y0 += 1; x1 -= 1; y1 -= 1; }

		barx = m_SplitPos;
		if (m_pSplitWnd1) m_pSplitWnd1->SetPosition(x0, y0, barx, y1);
		int w1 = (m_pSplitWnd1) ? m_pSplitWnd1->GetWidth() : barx-x0;
		if (m_pSplitWnd2) m_pSplitWnd2->SetPosition(barx+barw, y0, x1, y1);
		int w2 = (m_pSplitWnd2) ? m_pSplitWnd2->GetWidth() : x1-barx-barw;

		if (m_pSplitWnd2) m_pSplitWnd2->SetPosition(x1-w2, y0, x1, y1);
		w1 = x1-w2-barw;
		if (m_pSplitWnd1) m_pSplitWnd1->SetPosition(x0, y0, w1, y1);

		m_pWndDrag->SetStatus(WSTATUS_DISABLED, w1+w2 > GetWidth());
		m_pWndDrag->SetPosition(w1, y0, w1+barw, y1);
	}

//LogFile(CStrF("%.8x, %.8x, ", m_pSplitWnd1, m_pSplitWnd2) + m_SplitWnd1ID + ", " + m_SplitWnd2ID);
}

void CMWnd_SplitWnd::OnParentMove()
{
	MAUTOSTRIP(CMWnd_SplitWnd_OnParentMove, MAUTOSTRIP_VOID);
	CMWnd::OnParentMove();
	SetPosition(m_Pos);
//	OnMove();
}

aint CMWnd_SplitWnd::OnMessage(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_SplitWnd_OnMessage, 0);
	switch(_pMsg->m_Msg)
	{
	case WMSG_WNDDRAG:
		if (_pMsg->m_pID)
		{
			if (!strcmp(_pMsg->m_pID, "SPLITBAR"))
			{
				if (m_Style & WSTYLE_SPLITWND_Y)
					m_SplitPos = m_SplitPos + _pMsg->m_Param1;
				else
					m_SplitPos = m_SplitPos + _pMsg->m_Param0;
				OnMove();
			}
		}
		return true;

	default :
		return CMWnd::OnMessage(_pMsg);
	}
}

void CMWnd_SplitWnd::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	MAUTOSTRIP(CMWnd_SplitWnd_OnPaint, MAUTOSTRIP_VOID);
	int Alpha = m_Col_Alpha << 24;
	int ColH = m_Col_Highlight + Alpha;
//	int ColM = m_Col_Background + Alpha;
	int ColD = m_Col_Shadow + Alpha;
//	int ColDD = m_Col_DarkShadow + Alpha;

	int x0 = 0;
	int y0 = 0;
	int x1 = m_Pos.GetWidth();
	int y1 = m_Pos.GetHeight();

	if (m_Style & WSTYLE_CLIENTEDGE)
	{
		_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, ColD, ColH, true);
//		_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, ColDD, ColDD);
	}
}

// -------------------------------------------------------------------
//  CMWnd_Border
// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd_Border, CMWnd);


CMWnd_Border::CMWnd_Border()
{
	MAUTOSTRIP(CMWnd_Border_ctor, MAUTOSTRIP_VOID);
	m_Preasure = 0;
}

CMWnd_Border::CMWnd_Border(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID)
{
	MAUTOSTRIP(CMWnd_Border_ctor_2, MAUTOSTRIP_VOID);
	m_Preasure = 0;
	CMWnd::Create(_pWndParent, _Param, _ID);
}

void CMWnd_Border::PreCreate(CMWnd_Param* _pParam)
{
	MAUTOSTRIP(CMWnd_Border_PreCreate, MAUTOSTRIP_VOID);
	CMWnd::PreCreate(_pParam);
	_pParam->m_Style |= WSTYLE_BEHIND;
}

void CMWnd_Border::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	MAUTOSTRIP(CMWnd_Border_OnPaint, MAUTOSTRIP_VOID);
	int Alpha = m_Col_Alpha << 24;
	int ColH = m_Col_Highlight + Alpha;
	int ColM = m_Col_Background + Alpha;
	int ColD = m_Col_Shadow + Alpha;
	int ColDD = m_Col_DarkShadow + Alpha;

	int x0 = 0;
	int y0 = 0;
	int x1 = _Client.GetWidth();
	int y1 = _Client.GetHeight();

	int bDown = m_Status & WSTATUS_PRESSURE;

	if (m_Style & WSTYLE_BORDER_THIN)
	{
		if (bDown)
		{
			_pRCUtil->Frame(_Clip, x0, y0, x1, y1, ColH, ColH);
			_pRCUtil->Frame(_Clip, x0+1, y0+1, x1-1, y1-1, ColDD, ColDD);
		}
		else
		{
			_pRCUtil->Frame(_Clip, x0, y0, x1, y1, ColD, ColDD);
			_pRCUtil->Frame(_Clip, x0+1, y0+1, x1-1, y1-1, ColH, ColD);
		}
		x0 += 2;
		y0 += 2;
		x1 -= 2;
		y1 -= 2;
	}
	else
	{
		if (bDown)
		{
			_pRCUtil->Frame(_Clip, x0, y0, x1, y1, ColDD, ColDD);
			_pRCUtil->Frame(_Clip, x0+1, y0+1, x1-1, y1-1, ColH, ColM);
			_pRCUtil->Frame(_Clip, x0+2, y0+2, x1-2, y1-2, ColM, ColH, true);
			_pRCUtil->Frame(_Clip, x0+3, y0+3, x1-3, y1-3, ColDD, ColDD, true);
		}
		else
		{
			_pRCUtil->Frame(_Clip, x0, y0, x1, y1, ColD, ColDD);
			_pRCUtil->Frame(_Clip, x0+1, y0+1, x1-1, y1-1, ColH, ColD);
			_pRCUtil->Frame(_Clip, x0+2, y0+2, x1-2, y1-2, ColM, ColM);
			_pRCUtil->Frame(_Clip, x0+3, y0+3, x1-3, y1-3, ColD, ColH, true);
		}
		x0 += 4;
		y0 += 4;
		x1 -= 4;
		y1 -= 4;
	}
}

int CMWnd_Border::OnBorderPress(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_Border_OnBorderPress, 0);
	CMWnd* pParent = GetParent();
	if (!pParent) return 0;

	CRct OldPos = pParent->GetPosition();
	CRct Pos = OldPos;
	CPnt NewPos1 = CPnt(Pos.p0 + pParent->GetCursorPos(_pMsg));// - m_GrabPos;
	NewPos1 -= m_GrabPos;
	CPnt NewPos2 = CPnt(pParent->GetCursorPos(_pMsg) + pParent->GetPosition().p0);// - m_GrabPos2;
	NewPos2 -= m_GrabPos2;

	if (m_GrabMode & 1) Pos.p0.x = NewPos1.x;
	if (m_GrabMode & 4) Pos.p0.y = NewPos1.y;
	if (m_GrabMode & 2) Pos.p1.x = NewPos2.x;
	if (m_GrabMode & 8) Pos.p1.y = NewPos2.y;
	
	CMWnd_Message Msg(WMSG_WNDRESIZE, m_ID, 
					Pos.p0.x - OldPos.p0.x, Pos.p0.y - OldPos.p0.y, 
					Pos.p1.x - OldPos.p1.x, Pos.p1.y - OldPos.p1.y);
	pParent->OnMessage(&Msg);
	return 0;
}

aint CMWnd_Border::OnMessage(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_Border_OnMessage, 0);
	switch(_pMsg->m_Msg)
	{
	case WMSG_LOCKEDCURSOR :
		{
			CMWnd* pParent = GetParent();
			if (!pParent) 
				UnlockInput();
			else
			{
				if (_pMsg->IsReleased() && _pMsg->IsMouse1())
				{
					OnBorderPress(_pMsg);
					UnlockInput();
					m_Status &= ~WSTATUS_PRESSURE;
					if (CursorInWnd(_pMsg) && m_pWndParent)
					{
						// Pressed!
//						return m_pWndParent->OnMessage(&CMWnd_Message(WMSG_COMMAND, m_ID, 1, _pMsg->GetCursorBtn()));
					}
				}
				else
					OnBorderPress(_pMsg);
				}

			return true;
		}

	case WMSG_LOCKEDCURSORMOVE :
		{
			CMWnd* pParent = GetParent();
			if (!pParent) 
				UnlockInput();
			else
				OnBorderPress(_pMsg);
			return true;
		}

	case WMSG_CURSOR :
		{
			if (m_pWndParent && _pMsg->IsPressed() && _pMsg->IsMouse1())
			{
				if (m_Style & WSTYLE_RESIZABLE)
				{
					CMWnd* pParent = GetParent();

					m_GrabPos = pParent->GetCursorPos(_pMsg);
					m_GrabPos2 = CPnt(pParent->GetCursorPos(_pMsg) + pParent->GetPosition().p0);// - (pParent->GetPosition().p1);
					m_GrabPos2 -= (pParent->GetPosition().p1);

					int Mode = 0;
					CPnt gp = m_GrabPos;
					int w = m_Pos.GetWidth();
					int h = m_Pos.GetHeight();
					if (gp.x < 6) Mode |= 1;
					if (gp.x > w-6) Mode |= 2;
					if (gp.y < 6) Mode |= 4;
					if (gp.y > h-6) Mode |= 8;
					m_GrabMode = Mode;
					m_Preasure = Mode;
					m_Status |= WSTATUS_PRESSURE;

					LockInput();
				}
				return true;
			}
			break;
		}


/*	case WMSG_CURSOR :
		{
			if (m_pWndParent)
			{
				if (m_Style & WSTYLE_RESIZABLE)
				{
					CMWnd* pParent = GetParent();

					CPnt GrabPos = pParent->GetCursorPos();
					CPnt GrabPos2 = CPnt(pParent->GetCursorPos() + pParent->GetPosition().p0) - (pParent->GetPosition().p1);

					int Mode = 0;
					CPnt gp = GrabPos;
					int w = m_Pos.GetWidth();
					int h = m_Pos.GetHeight();
					if (gp.x < 6) Mode |= 1;
					if (gp.x > w-6) Mode |= 2;
					if (gp.y < 6) Mode |= 4;
					if (gp.y > h-6) Mode |= 8;

					m_Preasure = Mode;
					m_Status |= WSTATUS_PRESSURE;
					while(GetCursorBtn())
					{
						CRct OldPos = pParent->GetPosition();
						CRct Pos = OldPos;
						CPnt NewPos1 = CPnt(Pos.p0 + pParent->GetCursorPos()) - GrabPos;
						CPnt NewPos2 = CPnt(pParent->GetCursorPos() + pParent->GetPosition().p0) - GrabPos2;

						if (Mode & 1) Pos.p0.x = NewPos1.x;
						if (Mode & 4) Pos.p0.y = NewPos1.y;
						if (Mode & 2) Pos.p1.x = NewPos2.x;
						if (Mode & 8) Pos.p1.y = NewPos2.y;
						pParent->OnMessage(&CMWnd_Message(WMSG_WNDRESIZE, m_ID, 
							Pos.p0.x - OldPos.p0.x, Pos.p0.y - OldPos.p0.y, 
							Pos.p1.x - OldPos.p1.x, Pos.p1.y - OldPos.p1.y));
//						pParent->SetPosition(Pos);

						Wait();
					}
					m_Preasure = 0;
					m_Status &= ~WSTATUS_PRESSURE;
					if (CursorInWnd())
					{
						// Pressed!
					}

					return true;
				}
			}
		}
		break;
*/
	default :
		return CMWnd::OnMessage(_pMsg);
	}

	return false;
}

// -------------------------------------------------------------------
//  CMWnd_ResizePad
// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd_ResizePad, CMWnd);


CMWnd_ResizePad::CMWnd_ResizePad()
{
	MAUTOSTRIP(CMWnd_ResizePad_ctor, MAUTOSTRIP_VOID);
}

CMWnd_ResizePad::CMWnd_ResizePad(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID)
{
	MAUTOSTRIP(CMWnd_ResizePad_ctor_2, MAUTOSTRIP_VOID);
	CMWnd::Create(_pWndParent, _Param, _ID);
}

void CMWnd_ResizePad::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	MAUTOSTRIP(CMWnd_ResizePad_OnPaint, MAUTOSTRIP_VOID);
	int Alpha = m_Col_Alpha << 24;
	int ColH = m_Col_Highlight + Alpha;
	int ColM = m_Col_Background + Alpha;
	int ColD = m_Col_Shadow + Alpha;
//	int ColDD = m_Col_DarkShadow + Alpha;

	int x0 = 0;
	int y0 = 0;
	int x1 = _Client.GetWidth();
	int y1 = _Client.GetHeight();

	int bDown = m_Status & WSTATUS_PRESSURE;

	_pRCUtil->Rect(_Clip, CRct(x0, y0, x1, y1), ColM);
	if (bDown)
	{
		_pRCUtil->Rect(_Clip, CRct(x0, y0, x1, y0+1), ColD);
		_pRCUtil->Rect(_Clip, CRct(x0, y0, x0+1, y1), ColD);
		x0++; y0++; x1++; y1++;
	}

	_pRCUtil->Line(_Clip, CPnt(x0+3, y1), CPnt(x1, y0+3), ColH);
	_pRCUtil->Line(_Clip, CPnt(x0+5, y1), CPnt(x1, y0+5), ColD);
	_pRCUtil->Line(_Clip, CPnt(x0+7, y1), CPnt(x1, y0+7), ColH);
	_pRCUtil->Line(_Clip, CPnt(x0+9, y1), CPnt(x1, y0+9), ColD);
	_pRCUtil->Line(_Clip, CPnt(x0+11, y1), CPnt(x1, y0+11), ColH);
	_pRCUtil->Line(_Clip, CPnt(x0+13, y1), CPnt(x1, y0+13), ColD);
}

void CMWnd_ResizePad::OnResizePadPress(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_ResizePad_OnResizePadPress, MAUTOSTRIP_VOID);
	CMWnd* pParent = GetParent();
	if (!pParent) return;

	CRct OldPos = pParent->GetPosition();
	CRct Pos = OldPos;
	CPnt NewPos2 = CPnt(pParent->GetCursorPos(_pMsg) + pParent->GetPosition().p0);// - m_GrabPos;
	NewPos2 -= m_GrabPos;
	Pos.p1.x = NewPos2.x;
	Pos.p1.y = NewPos2.y;

	CMWnd_Message Msg(WMSG_WNDRESIZE, m_ID, 
		Pos.p0.x - OldPos.p0.x, Pos.p0.y - OldPos.p0.y, 
		Pos.p1.x - OldPos.p1.x, Pos.p1.y - OldPos.p1.y);

	pParent->OnMessage(&Msg);

	pParent->SetPosition(Pos);
}

aint CMWnd_ResizePad::OnMessage(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_ResizePad_OnMessage, 0);
	switch(_pMsg->m_Msg)
	{
	case WMSG_LOCKEDCURSORMOVE :
		{
			CMWnd* pParent = GetParent();
			if (!pParent) 
				UnlockInput();
			else
				OnResizePadPress(_pMsg);
			return true;
		}

	case WMSG_LOCKEDCURSOR :
		{
			CMWnd* pParent = GetParent();
			if (!pParent) 
				UnlockInput();
			else
			{
				if (_pMsg->IsReleased() && _pMsg->IsMouse1())
				{

					OnResizePadPress(_pMsg);
					UnlockInput();
					m_Status &= ~WSTATUS_PRESSURE;
					if (CursorInWnd(_pMsg) && m_pWndParent)
					{
						// Pressed!
//						return m_pWndParent->OnMessage(&CMWnd_Message(WMSG_COMMAND, m_ID, 1, _pMsg->GetCursorBtn()));
					}
				}
				else
				{
					OnPress(_pMsg);
				}
			}
			return true;
		}

	case WMSG_CURSOR :
		{
			if (GetParent() && _pMsg->IsPressed() && _pMsg->IsMouse1())
			{
				CMWnd* pParent = GetParent();
				m_GrabPos = CPnt(pParent->GetCursorPos(_pMsg) + pParent->GetPosition().p0);// - (pParent->GetPosition().p1);
				m_GrabPos -= (pParent->GetPosition().p1);
				if (LockInput())
				{
					m_Status |= WSTATUS_PRESSURE;
					OnResizePadPress(_pMsg);
				}
				return true;
			}
			return false;

/*			if (m_pWndParent)
			{
				CMWnd* pParent = GetParent();
				CPnt GrabPos2 = CPnt(pParent->GetCursorPos(_pMsg) + pParent->GetPosition().p0) - (pParent->GetPosition().p1);

				int w = m_Pos.GetWidth();
				int h = m_Pos.GetHeight();

				m_Status |= WSTATUS_PRESSURE;
				while(GetCursorBtn())
				{
					CRct OldPos = pParent->GetPosition();
					CRct Pos = OldPos;
					CPnt NewPos2 = CPnt(pParent->GetCursorPos() + pParent->GetPosition().p0) - GrabPos2;
					Pos.p1.x = NewPos2.x;
					Pos.p1.y = NewPos2.y;

					pParent->OnMessage(&CMWnd_Message(WMSG_WNDRESIZE, m_ID, 
						Pos.p0.x - OldPos.p0.x, Pos.p0.y - OldPos.p0.y, 
						Pos.p1.x - OldPos.p1.x, Pos.p1.y - OldPos.p1.y));

					pParent->SetPosition(Pos);

					Wait();
				}
				m_Status &= ~WSTATUS_PRESSURE;
				return true;
			}*/
		}
		break;

	default :
		return CMWnd::OnMessage(_pMsg);
	}
	return false;
}
// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd_Window, CMWnd);


void CMWnd_Window::Clear()
{
	MAUTOSTRIP(CMWnd_Window_Clear, MAUTOSTRIP_VOID);
	m_pWndCaption = NULL;
	m_pWndClient = NULL;
	m_pWndBorder = NULL;
	m_WndCaptionClass = "Caption";
	m_WndClientClass = "Client";
	m_WndBorderClass = "Border";

	m_ClientStyle = 0;
	m_ClientSize = CPnt(0,0);
}

void CMWnd_Window::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	MAUTOSTRIP(CMWnd_Window_EvaluateKey, MAUTOSTRIP_VOID);
	
	CStr Value = _Value;
	
	if (_Key == "CAPTION")
	{
		m_Caption = _Value;
	}
	else if (_Key == "CAPTIONCLASS")
	{
		m_WndCaptionClass = _Value;
	}
	else if (_Key == "CLIENTCLASS")
	{
		m_WndClientClass = _Value;
	}
	else if (_Key == "BORDERCLASS")
	{
		m_WndBorderClass = _Value;
	}
	else if (_Key == "STYLE")
	{
		while(Value.Len())
		{
			CStr s = Value.GetStrSep(",").LTrim().RTrim();
			if (s == "WND_CAPTION")
				_pParam->m_Style |= WSTYLE_WND_CAPTION;
			else if (s == "WND_NOFRAME")
				_pParam->m_Style |= WSTYLE_WND_NOFRAME;
			else if (s == "WND_THINFRAME")
				_pParam->m_Style |= WSTYLE_WND_THINFRAME;
			else if (s == "WND_TOOLWINDOW")
				_pParam->m_Style |= WSTYLE_WND_TOOLWINDOW;
			else if (s == "WND_CLOSEBTN")
				_pParam->m_Style |= WSTYLE_WND_CLOSEBTN;
			else if (s == "WND_MAXBTN")
				_pParam->m_Style |= WSTYLE_WND_MAXBTN;
			else if (s == "WND_MINBTN")
				_pParam->m_Style |= WSTYLE_WND_MINBTN;
			else
				CMWnd::EvaluateKey(_pParam, _Key, s);

				// Send any unknown styles to baseclass for eval.
		}
	}
	else if (_Key == "CLIENTSIZE")
	{
		m_ClientSize.x = Value.GetStrSep(",").Val_int();
		m_ClientSize.y = Value.GetStrSep(",").Val_int();
	}
	else if (_Key == "CLIENTSTYLE")
	{
		while(Value.Len())
		{
			CStr s = Value.GetStrSep(",").LTrim().RTrim();
			if (s == "CLIENT_SCROLLX")
				m_ClientStyle |= WSTYLE_CLIENT_SCROLLX;
			else if (s == "CLIENT_SCROLLY")
				m_ClientStyle |= WSTYLE_CLIENT_SCROLLY;
			else if (s == "CLIENT_RESIZEPAD")
				m_ClientStyle |= WSTYLE_CLIENT_RESIZEPAD;
		}
	}
	else
		CMWnd::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_Window::DoCreate(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID)
{
	MAUTOSTRIP(CMWnd_Window_DoCreate, MAUTOSTRIP_VOID);
	CMWnd::DoCreate(_pWndParent, _Param, _ID);

	if (m_WndCaptionClass != "")
	{
		MRTC_SAFECREATEOBJECT(spWnd, "CMWnd_" + m_WndCaptionClass, CMWnd_Caption);
		m_pWndCaption = spWnd;
		m_pWndCaption->Create(NULL, CMWnd_Param(0, 0, 0, 0, (m_Style & (WSTYLE_RESIZABLE | WSTYLE_MOVABLE)) | (m_Style & WSTYLE_WND_CAPTIONSHAREMASK)), "WND_CAPTION", m_Caption);
		CMWnd::AddChild((CMWnd*)spWnd);
	}

	if (m_WndClientClass != "")
	{
		MRTC_SAFECREATEOBJECT(spWnd, "CMWnd_" + m_WndClientClass, CMWnd_Client);
		m_pWndClient = spWnd;
		m_pWndClient->Create(NULL, CMWnd_Param(0, 0, 0, 0, m_ClientStyle, WSTATUS_MSGTOPARENT), "WND_CLIENT", m_ClientSize);
		CMWnd::AddChild((CMWnd*)spWnd);
		m_pWndClient->SetView(m_spClientView);
	}


/*	m_pWndCaption = DNew(CMWnd_Caption) CMWnd_Caption;
	m_pWndCaption->Create(NULL, CMWnd_Param(0, 0, 0, 0, (m_Style & (WSTYLE_RESIZABLE | WSTYLE_MOVABLE)) | (m_Style & WSTYLE_WND_CAPTIONSHAREMASK)), "WND_CAPTION", m_Caption);
	CMWnd::AddChild(m_pWndCaption);

	m_pWndClient = DNew(CMWnd_Client) CMWnd_Client;
	m_pWndClient->Create(NULL, CMWnd_Param(0, 0, 0, 0, m_ClientStyle, WSTATUS_MSGTOPARENT), "WND_CLIENT", m_ClientSize);
	CMWnd::AddChild(m_pWndClient);
*/

	if (!(m_Style & WSTYLE_WND_NOFRAME) && m_WndBorderClass != "")
	{
		MRTC_SAFECREATEOBJECT(spWnd, "CMWnd_" + m_WndBorderClass, CMWnd_Border);
		m_pWndBorder = spWnd;

//		m_pWndBorder = DNew(CMWnd_Border) CMWnd_Border;
		int Style = (m_Style & WSTYLE_WND_THINFRAME) ? WSTYLE_BORDER_THIN : 0;
		Style |= m_Style & (WSTYLE_RESIZABLE | WSTYLE_MOVABLE);
		m_pWndBorder->Create(NULL, CMWnd_Param(0, 0, 0, 0, Style), "WND_BORDER");
		CMWnd::AddChild(m_pWndBorder);
	}
	SetPosition(m_Pos);
}

void CMWnd_Window::DoDestroy()
{
	MAUTOSTRIP(CMWnd_Window_DoDestroy, MAUTOSTRIP_VOID);
	// Destroy stuff here:

	CMWnd::DoDestroy();
}

void CMWnd_Window::Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, const CStr& _Caption)
{
	MAUTOSTRIP(CMWnd_Window_Create, MAUTOSTRIP_VOID);
	m_Caption = _Caption;
	CMWnd::Create(_pWndParent, _Param, _ID);
}

CMWnd_Window::CMWnd_Window()
{
	MAUTOSTRIP(CMWnd_Window_ctor, MAUTOSTRIP_VOID);
	Clear();
}

CMWnd_Window::CMWnd_Window(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, const CStr& _Caption)
{
	MAUTOSTRIP(CMWnd_Window_ctor_2, MAUTOSTRIP_VOID);
	Clear();
	Create(_pWndParent, _Param, _ID, _Caption);
}

void CMWnd_Window::AddChild(spCMWnd _spWnd)
{
	MAUTOSTRIP(CMWnd_Window_AddChild, MAUTOSTRIP_VOID);
	if (m_pWndClient)
		m_pWndClient->AddChild(_spWnd);
	else
		CMWnd::AddChild(_spWnd);
}

CMWnd* CMWnd_Window::GetItem(const CStr& _ID)
{
	MAUTOSTRIP(CMWnd_Window_GetItem, NULL);
	if (m_pWndClient)
		return m_pWndClient->GetItem(_ID);
	else
		return CMWnd::GetItem(_ID);
}

int CMWnd_Window::GetWidth()
{
	MAUTOSTRIP(CMWnd_Window_GetWidth, 0);
	return Max(80, m_Pos.GetWidth());
}

int CMWnd_Window::GetHeight()
{
	MAUTOSTRIP(CMWnd_Window_GetHeight, 0);
	if (m_pWndCaption)
	{
		int Add = (m_Style & WSTYLE_WND_NOFRAME) ? 0 : ((m_Style & WSTYLE_WND_THINFRAME) ? 2 : 4);
		return Max(m_Pos.GetHeight(), m_pWndCaption->GetHeight() + 2*Add);
	}
	else
		return Max(m_Pos.GetHeight(), 20);
}

void CMWnd_Window::SetView(spCMWnd_View _spView)
{
	MAUTOSTRIP(CMWnd_Window_SetView, MAUTOSTRIP_VOID);
	m_spClientView = _spView;
	if (m_pWndClient)
		m_pWndClient->SetView(_spView);
}

void CMWnd_Window::OnMove()
{
	MAUTOSTRIP(CMWnd_Window_OnMove, MAUTOSTRIP_VOID);
	int w = GetWidth();
	int h = GetHeight();
	int x0 = 0;
	int y0 = 0;
	int x1 = w;
	int y1 = h;

	if (m_pWndBorder)
	{
		m_pWndBorder->SetStatus(WSTATUS_DISABLED, (m_Status & WSTATUS_MAXIMIZED) != 0);
		if (!(m_Status & WSTATUS_MAXIMIZED))
		{
			m_pWndBorder->SetPosition(CRct(x0, y0, x1, y1));
			int Add = (m_Style & WSTYLE_WND_THINFRAME) ? 2 : 4;
			x0 += Add;
			y0 += Add;
			x1 -= Add;
			y1 -= Add;
		}
	}

	if (m_pWndCaption)
	{
		m_pWndCaption->SetPosition(CRct(x0, y0, x1, y0+15));
		y0 += m_pWndCaption->GetHeight();
	}

	if (m_pWndClient)
	{
		m_pWndClient->SetPosition(CRct(x0, y0, x1, y1));
	}
	CMWnd::OnMove();
}

void CMWnd_Window::OnParentMove()
{
	MAUTOSTRIP(CMWnd_Window_OnParentMove, MAUTOSTRIP_VOID);
	CMWnd::OnParentMove();
	if (m_pWndClient) m_pWndClient->OnParentMove();
}

aint CMWnd_Window::OnMessage(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_Window_OnMessage, 0);
	switch(_pMsg->m_Msg)
	{
	case WMSG_WNDDRAG:
		if (_pMsg->m_pID)
		{
//			if (!strcmp(_pMsg->m_pID, "WND_CAPTION"))
			SetPosition(GetPosition().p0 + CPnt(_pMsg->m_Param0, _pMsg->m_Param1));
		}
		return true;

	case WMSG_WNDRESIZE:
		{
			CRct Pos = GetPosition();
			SetPosition(Pos.p0.x + _pMsg->m_Param0, Pos.p0.y + _pMsg->m_Param1, 
				Pos.p1.x + _pMsg->m_Param2, Pos.p1.y + _pMsg->m_Param3);
		}
		return true;

	case WMSG_COMMAND:
		if (_pMsg->m_pID)
		{
			if (!strcmp(_pMsg->m_pID, "WND_MAXIMIZE") && (_pMsg->m_Param0 == 1))
			{
				Maximize();
				return true;
			}
			else if (!strcmp(_pMsg->m_pID, "WND_MINIMIZE") && (_pMsg->m_Param0 == 1))
			{
				Minimize();
				return true;
			}
			else if (!strcmp(_pMsg->m_pID, "WND_CLOSE") && (_pMsg->m_Param0 == 1))
			{
				CMWnd_Message Msg(WMSG_CLOSE, "", 0);
				return OnMessage(&Msg);
			}
		}
		break;

	default :
		return CMWnd::OnMessage(_pMsg);
	}
	return false;
}

// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd_Desktop, CMWnd);


CMWnd_Desktop::CMWnd_Desktop()
{
	MAUTOSTRIP(CMWnd_Desktop_ctor, MAUTOSTRIP_VOID);
}

CMWnd_Desktop::CMWnd_Desktop(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID)
{
	MAUTOSTRIP(CMWnd_Desktop_ctor_2, MAUTOSTRIP_VOID);
	CMWnd::Create(_pWndParent, _Param, _ID);
}

void CMWnd_Desktop::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	MAUTOSTRIP(CMWnd_Desktop_OnPaint, MAUTOSTRIP_VOID);
	return;
}
// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd_Button, CMWnd);


void CMWnd_Button::Clear()
{
	MAUTOSTRIP(CMWnd_Button_Clear, MAUTOSTRIP_VOID);
	m_TextStyle = WSTYLE_TEXT_CENTER | WSTYLE_TEXT_CENTERY | WSTYLE_TEXT_CUTOUT | WSTYLE_TEXT_SHADOW | WSTYLE_TEXT_HIGHLIGHT;
	m_TextColorM = 0xff000000;
	m_TextColorH = 0x60ffffff;
	m_TextColorD = 0x60000000;
	m_TextPos = CPnt(0, 0);

	m_ImageStyle = 0;
	m_ImagePos = CPnt(0, 0);
	m_ImageSize = CPnt(0, 0);

	m_Transparent = false;
}

void CMWnd_Button::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	MAUTOSTRIP(CMWnd_Button_EvaluateKey, MAUTOSTRIP_VOID);
	
	CStr Value = _Value;
	
	if (_Key == "TEXT")
	{
		m_Text = _Value;
	}
	else if (_Key == "TEXTCOLOR_BG")
	{
		m_TextColorM = _Value.Val_int();
	}
	else if (_Key == "TEXTCOLOR_HL")
	{
		m_TextColorH = _Value.Val_int();
	}
	else if (_Key == "TEXTCOLOR_SHDW")
	{
		m_TextColorD = _Value.Val_int();
	}
	else if (_Key == "TEXTSTYLE")
	{
//		m_TextStyle = 0;
		while(Value.Len())
		{
			CStr s = Value.GetStrSep(",").LTrim().RTrim();
			if (s == "TEXT_CUTOUT")
				m_TextStyle |= WSTYLE_TEXT_CUTOUT;
			else if (s == "TEXT_SHADOW")
				m_TextStyle |= WSTYLE_TEXT_SHADOW;
			else if (s == "TEXT_HIGHLIGHT")
				m_TextStyle |= WSTYLE_TEXT_HIGHLIGHT;
			else if (s == "TEXT_CENTER")
				m_TextStyle |= WSTYLE_TEXT_CENTER;
			else if (s == "TEXT_CENTERY")
				m_TextStyle |= WSTYLE_TEXT_CENTERY;
			else if (s == "TEXT_WORDWRAP")
				m_TextStyle |= WSTYLE_TEXT_WORDWRAP;
			else if (s == "-TEXT_CUTOUT")
				m_TextStyle &= ~WSTYLE_TEXT_CUTOUT;
			else if (s == "-TEXT_SHADOW")
				m_TextStyle &= ~WSTYLE_TEXT_SHADOW;
			else if (s == "-TEXT_HIGHLIGHT")
				m_TextStyle &= ~WSTYLE_TEXT_HIGHLIGHT;
			else if (s == "-TEXT_CENTER")
				m_TextStyle &= ~WSTYLE_TEXT_CENTER;
			else if (s == "-TEXT_CENTERY")
				m_TextStyle &= ~WSTYLE_TEXT_CENTERY;
			else if (s == "-TEXT_WORDWRAP")
				m_TextStyle &= ~WSTYLE_TEXT_WORDWRAP;
			else
				LogFile("(CMWnd_Button::EvaluateKey) Uknown text-style: " + s);
		}
	}
	else if (_Key == "TEXTPOS")
	{
		m_TextPos.x = Value.GetStrSep(",").Val_int();
		m_TextPos.y = Value.GetStrSep(",").Val_int();
	}
	else if (_Key == "IMAGE")
	{
		m_Image = _Value;
	}
	else if (_Key == "IMAGESTYLE")
	{
		m_ImageStyle = 0;
		while(Value.Len())
		{
			CStr s = Value.GetStrSep(",").LTrim().RTrim();
			if (s == "IMG_CENTERX")
				m_ImageStyle |= WSTYLE_BTNIMG_CENTERX;
			else if (s == "IMG_CENTERY")
				m_ImageStyle |= WSTYLE_BTNIMG_CENTERY;
			else if (s == "IMG_STRETCHX")
				m_ImageStyle |= WSTYLE_BTNIMG_STRETCHX;
			else if (s == "IMG_STRETCHY")
				m_ImageStyle |= WSTYLE_BTNIMG_STRETCHY;
			else if (s == "IMG_STRETCHFITX")
				m_ImageStyle |= WSTYLE_BTNIMG_STRETCHFITX;
			else if (s == "IMG_STRETCHFITY")
				m_ImageStyle |= WSTYLE_BTNIMG_STRETCHFITY;
			else
				LogFile("(CMWnd_Button::EvaluateKey) Uknown text-style: " + s);
		}
	}
	else if (_Key == "IMAGEPOS")
	{
		m_ImagePos.x = Value.GetStrSep(",").Val_int();
		m_ImagePos.y = Value.GetStrSep(",").Val_int();
	}
	else if (_Key == "IMAGESIZE")
	{
		m_ImageSize.x = Value.GetStrSep(",").Val_int();
		m_ImageSize.y = Value.GetStrSep(",").Val_int();
	}
	else
		CMWnd::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_Button::PreEvalKeys(CMWnd_Param* _pParam)
{
	MAUTOSTRIP(CMWnd_Button_PreEvalKeys, MAUTOSTRIP_VOID);
	CMWnd::PreEvalKeys(_pParam);
}

void CMWnd_Button::PreCreate(CMWnd_Param* _pParam)
{
	MAUTOSTRIP(CMWnd_Button_PreCreate, MAUTOSTRIP_VOID);
	CMWnd::PreCreate(_pParam);
	_pParam->m_Style |= WSTYLE_TABSTOP;
}

void CMWnd_Button::Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, const CStr& _Text,
	int _TextStyle, const CStr& _Image, const CPnt& _ImagePos, const CPnt& _ImageSize, int _ImageStyle)
{
	MAUTOSTRIP(CMWnd_Button_Create, MAUTOSTRIP_VOID);
	m_Text = _Text;
	m_TextStyle = _TextStyle;
	m_Image = _Image;
	m_ImagePos = _ImagePos;
	m_ImageSize = _ImageSize;
	m_ImageStyle = _ImageStyle;
	CMWnd::Create(_pWndParent, _Param, _ID);
}

CMWnd_Button::CMWnd_Button()
{
	MAUTOSTRIP(CMWnd_Button_ctor, MAUTOSTRIP_VOID);
	Clear();
}

CMWnd_Button::CMWnd_Button(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, const CStr& _Text,
	int _TextStyle, const CStr& _Image, const CPnt& _ImagePos, const CPnt& _ImageSize, int _ImageStyle)
{
	MAUTOSTRIP(CMWnd_Button_ctor2, MAUTOSTRIP_VOID);
	Clear();
	Create(_pWndParent, _Param, _ID, _Text, _TextStyle, _Image, _ImagePos, _ImageSize, _ImageStyle);
}

void CMWnd_Button::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	MAUTOSTRIP(CMWnd_Button_OnPaint, MAUTOSTRIP_VOID);
	int Alpha = m_Col_Alpha << 24;
	int ColH = m_Col_Highlight + Alpha;
	int ColM = m_Col_Background + Alpha;
	int ColD = m_Col_Shadow + Alpha;
	int ColDD = m_Col_DarkShadow + Alpha;

	int x0 = 0;
	int y0 = 0;
	int x1 = m_Pos.GetWidth();
	int y1 = m_Pos.GetHeight();

	if (m_Style & WSTYLE_CLIENTEDGE)
		_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, ColD, ColH, true);

	int bDown = (m_Status & WSTATUS_PRESSURE) ? 1 : 0;
	if (bDown)
	{
		_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, ColDD, ColDD);
		_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, ColD, ColM, true);
		_pRCUtil->Rect(_Clip, CRct(x0, y0, x1, y1), ColM);
	}
	else
	{
		_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, ColDD, ColDD);
		_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, ColH, ColD);
		_pRCUtil->Rect(_Clip, CRct(x0, y0, x1, y1), ColM);
	}

	if (m_Text != "")
	{
		CRC_Font* pF = GetFont("SYSTEM");
		if (pF) 
		{
			CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, m_Text, bDown, bDown,
				m_TextStyle, m_TextColorM, m_TextColorH, m_TextColorD, m_Pos.GetWidth(), m_Pos.GetHeight());
		}
	}

	if (m_Image.Len())
	{
		_pRCUtil->SetTexture(m_Image);
//		_pRCUtil->Sprite(_Clip, CPnt(2 + bDown, 2 + bDown));
		CPnt Pos, Size;
		Size.x = (m_ImageStyle & WSTYLE_BTNIMG_STRETCHX) ? m_ImageSize.x : _pRCUtil->GetTextureWidth();
		Size.y = (m_ImageStyle & WSTYLE_BTNIMG_STRETCHX) ? m_ImageSize.y : _pRCUtil->GetTextureHeight();
		if (m_ImageStyle & WSTYLE_BTNIMG_STRETCHFITX) Size.x = m_Pos.GetWidth() - 2*m_ImagePos.x;
		if (m_ImageStyle & WSTYLE_BTNIMG_STRETCHFITY) Size.y = m_Pos.GetHeight() - 2*m_ImagePos.y;
		Pos.x = (m_ImageStyle & WSTYLE_BTNIMG_CENTERX) ? ((m_Pos.GetWidth() - Size.x) >> 1) : m_ImagePos.x;
		Pos.y = (m_ImageStyle & WSTYLE_BTNIMG_CENTERY) ? ((m_Pos.GetHeight() - Size.y) >> 1) : m_ImagePos.y;
		Pos.x += bDown;
		Pos.y += bDown;

		_pRCUtil->ScaleSprite(_Clip, Pos, Size);
//		_pRCUtil->ScaleSprite(_Clip, CPnt(x0, y0), CPnt(x1-x0, y1-y0));
		_pRCUtil->SetTexture(0);
	}
}

aint CMWnd_Button::OnMessage(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_Button_OnMessage, 0);
	return CMWnd::OnMessage(_pMsg);
}

// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd_DragbarX, CMWnd);


void CMWnd_DragbarX::GetDragBtnInfo(int _w, int& wbtn, int& xbtn)
{
	MAUTOSTRIP(CMWnd_DragbarX_GetDragBtnInfo, MAUTOSTRIP_VOID);
	fp32 dRng = m_RangeMax - m_RangeMin;
	fp32 vsize = Min(m_ViewSize, dRng);

	if (m_ViewSize < 0)
	{
		wbtn = int((fp32(_w) / 6.0f) + 1.0f);
		xbtn = int((fp32(_w) - wbtn) * (m_ViewPos - m_RangeMin) / dRng);
	}
	else
	{
		wbtn = int(vsize / dRng * fp32(_w)  + 1.0f);
		xbtn = int((m_ViewPos - m_RangeMin) / (dRng) * fp32(_w));
	}
	if (xbtn > _w) xbtn = _w;
	if (xbtn + wbtn > _w) xbtn = _w - wbtn;
	if (xbtn < 0) xbtn = 0;
	if (xbtn + wbtn > _w) wbtn = _w - xbtn;
}

void CMWnd_DragbarX::SetDragBarPos(fp32 _Pos)
{
	MAUTOSTRIP(CMWnd_DragbarX_SetDragBarPos, MAUTOSTRIP_VOID);
	m_ViewPos = _Pos;
	if (m_ViewPos+m_ViewSize >= m_RangeMax) m_ViewPos = m_RangeMax - m_ViewSize;
	if (m_ViewPos < m_RangeMin) m_ViewPos = m_RangeMin;
	OnMove();
}

void CMWnd_DragbarX::PostViewPosMsg()
{
	MAUTOSTRIP(CMWnd_DragbarX_PostViewPosMsg, MAUTOSTRIP_VOID);
	CMWnd_Message Msg(WMSG_COMMAND, m_ID, 0);
	Msg.m_fParam0 = m_ViewPos;
	if (m_pWndParent) m_pWndParent->OnMessage(&Msg);
}

void CMWnd_DragbarX::Clear()
{
	MAUTOSTRIP(CMWnd_DragbarX_Clear, MAUTOSTRIP_VOID);
	m_pWndDragBtn = NULL;
	m_pWndIncBtn = NULL;
	m_pWndDecBtn = NULL;
	m_pWndIncArea = NULL;
	m_pWndDecArea = NULL;

	m_BarLen = 0;
	m_RangeMin = 0;
	m_RangeMax = 1;
	m_ViewPos = 0;
	m_ViewSize = 1;

	m_IncBtnArw = "ARROW2_RIGHT";
	m_DecBtnArw = "ARROW2_LEFT";
}

void CMWnd_DragbarX::PreCreate(CMWnd_Param* _pParam)
{
	MAUTOSTRIP(CMWnd_DragbarX_PreCreate, MAUTOSTRIP_VOID);
	CMWnd::PreCreate(_pParam);
	_pParam->m_Style  |=  WSTYLE_NOKEYPROCESS;
}

void CMWnd_DragbarX::DoCreate(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID)
{
	MAUTOSTRIP(CMWnd_DragbarX_DoCreate, MAUTOSTRIP_VOID);
	CMWnd::DoCreate(_pWndParent, _Param, _ID);

	m_pWndDragBtn = MNew4(CMWnd_Button, (CMWnd *)NULL, (CMWnd_Param(0,0,0,0, WSTYLE_MOVABLE | WSTYLE_NOKEYPROCESS)), "DRAG_DRAGBAR", "");
	AddChild(m_pWndDragBtn);

	m_pWndIncBtn = MNew9(CMWnd_Button, (CMWnd *)NULL, (CMWnd_Param(0,0,0,0, m_Style | WSTYLE_NOKEYPROCESS)), "DRAG_INCBTN", "", 0, 
		m_IncBtnArw, CPnt(3, 3), CPnt(0, 0), WSTYLE_BTNIMG_STRETCHFITX | WSTYLE_BTNIMG_STRETCHFITY |
		WSTYLE_BTNIMG_CENTERX | WSTYLE_BTNIMG_CENTERY);
	AddChild(m_pWndIncBtn);

	m_pWndDecBtn = MNew9(CMWnd_Button, (CMWnd *)NULL, CMWnd_Param(0,0,0,0, m_Style | WSTYLE_NOKEYPROCESS), "DRAG_DECBTN", "", 0, 
		m_DecBtnArw, CPnt(3, 3), CPnt(0, 0), WSTYLE_BTNIMG_STRETCHFITX | WSTYLE_BTNIMG_STRETCHFITY |
		WSTYLE_BTNIMG_CENTERX | WSTYLE_BTNIMG_CENTERY);
	AddChild(m_pWndDecBtn);

	m_pWndIncArea = MNew3(CMWnd_Client, (CMWnd *)NULL, CMWnd_Param(0,0,0,0, WSTYLE_CLIENT_BORDER | WSTYLE_NOKEYPROCESS), "DRAG_INCAREA");
	AddChild(m_pWndIncArea);
	m_pWndIncArea->m_Col_Background = 0xff303030;
	m_pWndIncArea->m_Col_Shadow = 0xff202020;
//	m_pWndIncArea->m_Col_Background = 0xffe0e0e0;
//	m_pWndIncArea->m_Col_Shadow = 0xffa0a0a0;

	m_pWndDecArea = MNew3(CMWnd_Client, (CMWnd *)NULL, CMWnd_Param(0,0,0,0, WSTYLE_CLIENT_BORDER | WSTYLE_NOKEYPROCESS), "DRAG_DECAREA");
	AddChild(m_pWndDecArea);
	m_pWndDecArea->m_Col_Background = 0xff303030;
	m_pWndDecArea->m_Col_Shadow = 0xff202020;
//	m_pWndDecArea->m_Col_Background = 0xffe0e0e0;
//	m_pWndDecArea->m_Col_Shadow = 0xffa0a0a0;

	SetPosition(m_Pos);
}

void CMWnd_DragbarX::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	MAUTOSTRIP(CMWnd_DragbarX_EvaluateKey, MAUTOSTRIP_VOID);
	
	const fp32 Valuef = _Value.Val_fp64();
	
	if (_Key == "DRAG_MIN")
	{
		m_RangeMin = Valuef;
	}
	else if (_Key == "DRAG_MAX")
	{
		m_RangeMax = Valuef;
	} 
	else if (_Key == "DRAG_VIEWSIZE")
	{
		m_ViewSize = Valuef;
	}
	else if (_Key == "DRAG_VIEWPOS")
	{
		m_ViewPos = Valuef;
	}
	else
		CMWnd::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_DragbarX::Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, 
	 fp32 _RangeMin, fp32 _RangeMax, fp32 _ViewPos, fp32 _ViewSize)
{
	MAUTOSTRIP(CMWnd_DragbarX_Create, MAUTOSTRIP_VOID);
	m_BarLen = 0;
	m_RangeMin = _RangeMin;
	m_RangeMax = _RangeMax;
	m_ViewPos = _ViewPos;
	m_ViewSize = _ViewSize;
	CMWnd::Create(_pWndParent, _Param, _ID);
}

CMWnd_DragbarX::CMWnd_DragbarX()
{
	MAUTOSTRIP(CMWnd_DragbarX_ctor, MAUTOSTRIP_VOID);
	Clear();
}

CMWnd_DragbarX::CMWnd_DragbarX(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, 
	 fp32 _RangeMin, fp32 _RangeMax, fp32 _ViewPos, fp32 _ViewSize)
{
	MAUTOSTRIP(CMWnd_DragbarX_ctor2, MAUTOSTRIP_VOID);
	Clear();
	Create(_pWndParent, _Param, _ID, _RangeMin, _RangeMax, _ViewPos, _ViewSize);
}

void CMWnd_DragbarX::OnMove()
{
	MAUTOSTRIP(CMWnd_DragbarX_OnMove, MAUTOSTRIP_VOID);
	int x0 = 0;
	int y0 = 0;
	int x1 = m_Pos.GetWidth();
	int y1 = m_Pos.GetHeight();

	bool bBarOff = false;
	int bh = y1-y0;
	if (bh*2 >= (x1-x0))
	{
		bh = (x1-x0) >> 1;
		bBarOff = true;
		m_BarLen = 0;
	}
	if (m_pWndDragBtn) m_pWndDragBtn->SetStatus(WSTATUS_DISABLED, bBarOff);
	if (m_pWndIncArea) m_pWndIncArea->SetStatus(WSTATUS_DISABLED, bBarOff);
	if (m_pWndDecArea) m_pWndDecArea->SetStatus(WSTATUS_DISABLED, bBarOff);

	if (m_pWndDecBtn) m_pWndDecBtn->SetPosition(x0, y0, x0+bh, y1);
	if (m_pWndIncBtn) m_pWndIncBtn->SetPosition(x1-bh, y0, x1, y1);

	x0 += bh;
	x1 -= bh;
	if (!bBarOff)
	{
		int wbtn, xbtn;
		GetDragBtnInfo(x1-x0, wbtn, xbtn);
		if (m_pWndDragBtn) m_pWndDragBtn->SetPosition(x0 + xbtn, y0, x0 + xbtn + wbtn, y1);
		if (m_pWndDecArea) m_pWndDecArea->SetPosition(x0, y0, x0 + xbtn, y1);
		if (m_pWndIncArea) m_pWndIncArea->SetPosition(x0 + xbtn + wbtn, y0, x1, y1);

		m_BarLen = x1-x0;
//LogFile(CStrF("x0 %d, x1 %d, y0 %d, y1 %d, bh %d, xbtn %d, wbtn %d", x0, x1, y0, y1, bh, xbtn, wbtn));
	}
}

void CMWnd_DragbarX::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	MAUTOSTRIP(CMWnd_DragbarX_OnPaint, MAUTOSTRIP_VOID);
}

aint CMWnd_DragbarX::OnMessage(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_DragbarX_OnMessage, 0);
	switch(_pMsg->m_Msg)
	{
	case WMSG_WNDDRAG :
		if (_pMsg->m_pID)
		{
			if (!strcmp(_pMsg->m_pID, "DRAG_DRAGBAR") && m_BarLen)
			{
				int dx = _pMsg->m_Param0;
				Drag_SetPos(m_ViewPos + fp32(dx)/m_BarLen * (m_RangeMax - m_RangeMin));
				return true;
			}
		}
		break;

	case WMSG_COMMAND :
		if (_pMsg->m_pID)
		{
			if (!strcmp(_pMsg->m_pID, "DRAG_DECBTN") && _pMsg->m_Param0 == 1)
			{
				Drag_SetPos(m_ViewPos - 1.0f);
				MouseRepeatDelay(0);
			}
			else if (!strcmp(_pMsg->m_pID, "DRAG_INCBTN") && _pMsg->m_Param0 == 1)
			{
				Drag_SetPos(m_ViewPos + 1.0f);
				MouseRepeatDelay(0);
			}
			else if (!strcmp(_pMsg->m_pID, "DRAG_DECAREA") && _pMsg->m_Param0 == 1)
			{
				Drag_SetPos(m_ViewPos - Max(1.0f, m_ViewSize));
				MouseRepeatDelay(2);
			}
			else if (!strcmp(_pMsg->m_pID, "DRAG_INCAREA") && _pMsg->m_Param0 == 1)
			{
				Drag_SetPos(m_ViewPos + Max(1.0f, m_ViewSize));
				MouseRepeatDelay(2);
			}
			else
				return false;

			return true;
		}
		break;

	case WMSG_DRAG_SETWINDOW :
		{
			// fP0 = Min
			// fP1 = Max
			// fP2 = ViewSize
			Drag_SetWindow(_pMsg->m_fParam0, _pMsg->m_fParam1, _pMsg->m_fParam2);
			return true;
		}

	case 0x0200 :	// WMSG_SETSTATE
		{
			Drag_SetPos(_pMsg->m_Param0);
			return true;
		}

	case WMSG_DRAG_SETPOS :
		{
			// fP0 = Pos
			Drag_SetPos(_pMsg->m_fParam0);
			return true;
		}

	case 0x0201 :	// WMSG_GETSTATE
		{
			return RoundToInt(m_ViewPos);
		}

	default :
		return CMWnd::OnMessage(_pMsg);
	}
	return false;
}

void CMWnd_DragbarX::Drag_SetWindow(fp32 _Min, fp32 _Max, fp32 _ViewSize)
{
	MAUTOSTRIP(CMWnd_DragbarX_Drag_SetWindow, MAUTOSTRIP_VOID);
	m_RangeMin = _Min;
	m_RangeMax = _Max;
	m_ViewSize = _ViewSize;
//	if ((m_ViewPos < m_RangeMin) || (m_ViewPos + m_ViewSize > m_RangeMax))
	SetDragBarPos(m_ViewPos);
}

void CMWnd_DragbarX::Drag_SetPos(fp32 _Pos)
{
	MAUTOSTRIP(CMWnd_DragbarX_Drag_SetPos, MAUTOSTRIP_VOID);
	fp32 OldPos(m_ViewPos);
	SetDragBarPos(_Pos);
	if (m_ViewPos != OldPos) PostViewPosMsg();
}

fp32 CMWnd_DragbarX::Drag_GetPos()
{
	MAUTOSTRIP(CMWnd_DragbarX_Drag_GetPos, 0.0f);
	return m_ViewPos;
}

// -------------------------------------------------------------------
//  CMWnd_DragbarY
// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd_DragbarY, CMWnd_DragbarX);


void CMWnd_DragbarY::Clear()
{
	MAUTOSTRIP(CMWnd_DragbarY_Clear, MAUTOSTRIP_VOID);
	m_IncBtnArw = "ARROW2_DOWN";
	m_DecBtnArw = "ARROW2_UP";
}

CMWnd_DragbarY::CMWnd_DragbarY()
{
	MAUTOSTRIP(CMWnd_DragbarY_ctor, MAUTOSTRIP_VOID);
	Clear();
}

CMWnd_DragbarY::CMWnd_DragbarY(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, 
	 fp32 _RangeMin, fp32 _RangeMax, fp32 _ViewPos, fp32 _ViewSize)
{
	MAUTOSTRIP(CMWnd_DragbarY_ctor2, MAUTOSTRIP_VOID);
	Clear();
	Create(_pWndParent, _Param, _ID, _RangeMin, _RangeMax, _ViewPos, _ViewSize);
}

void CMWnd_DragbarY::OnMove()
{
	MAUTOSTRIP(CMWnd_DragbarY_OnMove, MAUTOSTRIP_VOID);
	int x0 = 0;
	int y0 = 0;
	int x1 = m_Pos.GetWidth();
	int y1 = m_Pos.GetHeight();

	bool bBarOff = false;
	int bh = x1-x0;
	if (bh*2 >= (y1-y0))
	{
		bh = (y1-y0) >> 1;
		bBarOff = true;
		m_BarLen = 0;
	}
	if (m_pWndDragBtn) m_pWndDragBtn->SetStatus(WSTATUS_DISABLED, bBarOff);
	if (m_pWndIncArea) m_pWndIncArea->SetStatus(WSTATUS_DISABLED, bBarOff);
	if (m_pWndDecArea) m_pWndDecArea->SetStatus(WSTATUS_DISABLED, bBarOff);

	if (m_pWndDecBtn) m_pWndDecBtn->SetPosition(x0, y0, x1, y0+bh);
	if (m_pWndIncBtn) m_pWndIncBtn->SetPosition(x0, y1-bh, x1, y1);

	y0 += bh;
	y1 -= bh;
	if (!bBarOff)
	{
		int hbtn, ybtn;
		GetDragBtnInfo(y1-y0, hbtn, ybtn);
		if (m_pWndDragBtn) m_pWndDragBtn->SetPosition(x0, y0 + ybtn, x1, y0  + ybtn + hbtn);
		if (m_pWndDecArea) m_pWndDecArea->SetPosition(x0, y0, x1, y0 + ybtn);
		if (m_pWndIncArea) m_pWndIncArea->SetPosition(x0, y0 + ybtn + hbtn, x1, y1);

		m_BarLen = y1-y0;
//LogFile(CStrF("x0 %d, x1 %d, y0 %d, y1 %d, bh %d, xbtn %d, wbtn %d", x0, x1, y0, y1, bh, xbtn, wbtn));
	}
}

aint CMWnd_DragbarY::OnMessage(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_DragbarY_OnMessage, 0);
	switch(_pMsg->m_Msg)
	{
	case WMSG_WNDDRAG :
		if (_pMsg->m_pID)
		{
			if (!strcmp(_pMsg->m_pID, "DRAG_DRAGBAR") && m_BarLen)
			{
				int dy = _pMsg->m_Param1;
				Drag_SetPos(m_ViewPos + fp32(dy)/m_BarLen * (m_RangeMax - m_RangeMin));
				return true;
			}
		}
		break;
	default :
		return CMWnd_DragbarX::OnMessage(_pMsg);
	}
	return false;
}

// -------------------------------------------------------------------
//  CMWnd_List
// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd_List, CMWnd_Client);


void CMWnd_List::DoCreate(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID)
{
	MAUTOSTRIP(CMWnd_List_DoCreate, MAUTOSTRIP_VOID);
	CMWnd_Client::DoCreate(_pWndParent, _Param, _ID);
	Update();
}

spCMWnd CMWnd_List::CreateClient()
{
	MAUTOSTRIP(CMWnd_List_CreateClient, NULL);
	return MNew1(CMWnd_ListClient, this);
}

void CMWnd_List::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	MAUTOSTRIP(CMWnd_List_EvaluateKey, MAUTOSTRIP_VOID);
	
	CStr Value = _Value;
	
	if (_Key == "STYLE")
	{
		while(Value.Len())
		{
			CStr s = Value.GetStrSep(",").LTrim().RTrim();
			if (s == "LIST_MULTISELECT")
				_pParam->m_Style |= WSTYLE_LIST_MULTISELECT;
			if (s == "LIST_AUTOWIDTH")
				_pParam->m_Style |= WSTYLE_LIST_AUTOWIDTH;
			else
				CMWnd_Client::EvaluateKey(_pParam, _Key, s);

				// Send any unknown styles to baseclass for eval.
		}
	}
	else if (_Key == "LIST_WIDTH")
	{
		m_ListWidth = _Value.Val_int();

	}
	else
		CMWnd_Client::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_List::Clear()
{
	MAUTOSTRIP(CMWnd_List_Clear, MAUTOSTRIP_VOID);
	m_Col_Shadow = 0xffe0e0e0;
	m_Col_Background = 0xffffffff;
	m_Col_Custom0 = 0xff000000;
	m_Col_Custom0_Focus = 0xffffffff;
	m_Col_Custom1_Focus = 0xff404060;
	m_Status = WSTATUS_INVISIBLE;
	m_nItemPerCol = 1;
	m_nColumns = 1;
	m_ListWidth = 100;
	m_MinListWidth = 1;
}

CMWnd_List::CMWnd_List()
{
	MAUTOSTRIP(CMWnd_List_ctor, MAUTOSTRIP_VOID);
	Clear();
}

CMWnd_List::CMWnd_List(CMWnd* _pWndParent, const CMWnd_Param& _Param, const CStr& _ID, const CPnt& _Size)
{
	MAUTOSTRIP(CMWnd_List_ctor_2, MAUTOSTRIP_VOID);
	Clear();
	Create(_pWndParent, _Param, _ID, _Size);
}

void CMWnd_List::PreCreate(CMWnd_Param* _pParam)
{
	MAUTOSTRIP(CMWnd_List_PreCreate, MAUTOSTRIP_VOID);
	CMWnd_Client::PreCreate(_pParam);
	_pParam->m_Status |= WSTATUS_MSGTOPARENT;
}

void CMWnd_List::OnMove()
{
	MAUTOSTRIP(CMWnd_List_OnMove, MAUTOSTRIP_VOID);
	CMWnd_Client::OnMove();
	Update();
}

void CMWnd_List::OnRefresh()
{
	MAUTOSTRIP(CMWnd_List_OnRefresh, MAUTOSTRIP_VOID);
	if (m_bUpdate) Update();
}

void CMWnd_List::Update()
{
	MAUTOSTRIP(CMWnd_List_Update, MAUTOSTRIP_VOID);
//LogFile(CStrF("ClipClient %d, %d", m_pWndClipClient->GetWidth(), m_pWndClipClient->GetHeight()));

	m_bUpdate = false;
	int nItems = m_lspItems.Len();
	if (m_Style & WSTYLE_CLIENT_SCROLLY)
		SetClientSize(GetWidth(), nItems*CMWND_LIST_ITEMHEIGHT);
	else
	{
		if (m_Style & WSTYLE_LIST_AUTOWIDTH)
		{
			int Width = 6;
			CRC_Font* pF = GetFont("SYSTEM");
			if (pF)
			{
				for(int i = 0; i < nItems; i++)
				{
					int w = 6 + CRC_Util2D::TextWidth(pF, m_lspItems[i]->m_Text);
					if (w > Width) Width = w;
				}
				m_ListWidth = Width;
			}
		}

		int h = (m_pWndClipClient) ? m_pWndClipClient->GetHeight() : 0;
		m_nItemPerCol = (h) ? Max(1, (h / CMWND_LIST_ITEMHEIGHT)) : 1;
//		LogFile(CStrF("%d, %d, %d", h, m_nItemPerCol, 0));
		m_nColumns = (nItems + m_nItemPerCol -1) / m_nItemPerCol;
		SetClientSize(m_nColumns * m_ListWidth, h);
	}
}

int CMWnd_List::AddImage(const CStr& _Name)
{
	MAUTOSTRIP(CMWnd_List_AddImage, 0);
	return -1;
}

void CMWnd_List::RemoveImage(int _iImage)
{
	MAUTOSTRIP(CMWnd_List_RemoveImage, MAUTOSTRIP_VOID);
}

int CMWnd_List::GetListItemCount()
{
	MAUTOSTRIP(CMWnd_List_GetItemCount, 0);
	return m_lspItems.Len();
}

int CMWnd_List::AddListItem(const CStr& _Text, int _Data, int _iImage)
{
	MAUTOSTRIP(CMWnd_List_AddItem, 0);
	int iItem = m_lspItems.Add(MNew3(CMWnd_ListItem, _Text, _Data, _iImage));
	m_bUpdate = true;
	return iItem;
}

void CMWnd_List::RemoveListItem(int _iItem)
{
	MAUTOSTRIP(CMWnd_List_RemoveItem, MAUTOSTRIP_VOID);
	if ((_iItem < 0) || (_iItem >= m_lspItems.Len())) return;
	m_lspItems.Del(_iItem);
	m_bUpdate = true;
}

void CMWnd_List::RemoveAllList()
{
	MAUTOSTRIP(CMWnd_List_RemoveAll, MAUTOSTRIP_VOID);
	m_lspItems.Clear();
	m_bUpdate = true;
}

void CMWnd_List::SetListItem(int _iItem, const CStr& _Text, int _Data, int _iImage)
{
	MAUTOSTRIP(CMWnd_List_SetItem, MAUTOSTRIP_VOID);
	if ((_iItem < 0) || (_iItem >= m_lspItems.Len())) return;
	CMWnd_ListItem* pI = m_lspItems[_iItem];
	pI->m_Text = _Text;
	pI->m_Data = _Data;
	pI->m_iImage = _iImage;
}

void CMWnd_List::SetListItem(int _iItem, const CStr& _Text, int _Data)
{
	MAUTOSTRIP(CMWnd_List_SetItem_2, MAUTOSTRIP_VOID);
	if ((_iItem < 0) || (_iItem >= m_lspItems.Len())) return;
	CMWnd_ListItem* pI = m_lspItems[_iItem];
	pI->m_Text = _Text;
	pI->m_Data = _Data;
}

void CMWnd_List::SetListItem(int _iItem, const CStr& _Text)
{
	MAUTOSTRIP(CMWnd_List_SetItem_3, MAUTOSTRIP_VOID);
	if ((_iItem < 0) || (_iItem >= m_lspItems.Len())) return;
	m_lspItems[_iItem]->m_Text = _Text;
}

void CMWnd_List::SetListItemData(int _iItem, int _Data)
{
	MAUTOSTRIP(CMWnd_List_SetItemData, MAUTOSTRIP_VOID);
	if ((_iItem < 0) || (_iItem >= m_lspItems.Len())) return;
	m_lspItems[_iItem]->m_Data = _Data;
}

void CMWnd_List::SetListItemImage(int _iItem, int _iImage)
{
	MAUTOSTRIP(CMWnd_List_SetItemImage, MAUTOSTRIP_VOID);
	if ((_iItem < 0) || (_iItem >= m_lspItems.Len())) return;
	m_lspItems[_iItem]->m_iImage = _iImage;
}

CStr CMWnd_List::GetListItem(int _iItem)
{
	MAUTOSTRIP(CMWnd_List_GetItem, CStr());
	if ((_iItem < 0) || (_iItem >= m_lspItems.Len())) return "";
	return m_lspItems[_iItem]->m_Text;
}

int CMWnd_List::GetListItemData(int _iItem)
{
	MAUTOSTRIP(CMWnd_List_GetItemData, 0);
	return m_lspItems[_iItem]->m_Data;
}

int CMWnd_List::GetSelected(int _iLast)
{
	MAUTOSTRIP(CMWnd_List_GetSelected, 0);
	int iSearch = _iLast + 1;
	int nItems = m_lspItems.Len();
	while((iSearch < nItems) && !(m_lspItems[iSearch]->m_Flags & CMWND_LISTITEM_SELECTED)) iSearch++;
	if (iSearch == nItems) return -1;
	return iSearch;
}

void CMWnd_List::DeselectAll()
{
	MAUTOSTRIP(CMWnd_List_DeselectAll, MAUTOSTRIP_VOID);
	int nItems = m_lspItems.Len();
	for(int i = 0; i < nItems; i++) m_lspItems[i]->m_Flags &= ~CMWND_LISTITEM_SELECTED;
}

void CMWnd_List::OnTouchItem(int _iItem, int _bSelected)
{
	MAUTOSTRIP(CMWnd_List_OnTouchItem, MAUTOSTRIP_VOID);
}

void CMWnd_List::OnAction(int _iItem)
{
	MAUTOSTRIP(CMWnd_List_OnAction, MAUTOSTRIP_VOID);
}

// -------------------------------------------------------------------
CMWnd_ListClient::CMWnd_ListClient(CMWnd_List* _pList)
{
	MAUTOSTRIP(CMWnd_ListClient_ctor, MAUTOSTRIP_VOID);
	m_pList = _pList;
}

int CMWnd_ListClient::Pos2Item(const CPnt& _Pos)
{
	MAUTOSTRIP(CMWnd_ListClient_Pos2Item, 0);
	if (!PntInWnd(_Pos)) return -1;
	
	CPnt Pos = _Pos;

	if (m_pList->m_Style & WSTYLE_CLIENT_SCROLLY)
	{
		Pos.y -= CMWND_LIST_YPADDING;
		int iItem = Pos.y / CMWND_LIST_ITEMHEIGHT;
		int nItems = m_pList->GetListItemCount();
		if ((iItem >= 0) && (iItem < nItems)) return iItem;
		return -1;
	}
	else
	{
		Pos.x -= CMWND_LIST_XPADDING;
		Pos.y -= CMWND_LIST_YPADDING;
		int iItem = Pos.y / CMWND_LIST_ITEMHEIGHT;
		if (iItem < 0) iItem = 0;
		if (iItem >= m_pList->m_nItemPerCol) iItem = m_pList->m_nItemPerCol-1;
		int iCol = Pos.x / Max(1, m_pList->m_ListWidth);
		iItem += iCol*m_pList->m_nItemPerCol;

		int nItems = m_pList->GetListItemCount();
		if ((iItem >= 0) && (iItem < nItems)) return iItem;
		return -1;
	}
}

aint CMWnd_ListClient::OnMessage(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_ListClient_OnMessage, 0);
	switch(_pMsg->m_Msg)
	{
	case WMSG_CURSOR :
		Error("OnMessage", "This item was not converted for non-blocking cursor interaction.");

/*		if (GetCursorBtn())
		{
			int LastBtn = 0;
			int Action = -1;
			int iItem = -1;
			while(GetCursorBtn())
			{
				LastBtn = GetCursorBtn();
				if (CursorInWnd())
				{
					CPnt Pos = GetCursorPos();
					iItem = Pos2Item(Pos);
					if (iItem >= 0)
					{
						CMWnd_ListItem* pI = m_pList->m_lspItems[iItem];
						if (Action < 0)
							Action = (pI->m_Flags & CMWND_LISTITEM_SELECTED) ? 0 : 1;
						else
						{
							if (Action)
							{
								if (!(pI->m_Flags & CMWND_LISTITEM_SELECTED))
								{
									if (!(m_pList->m_Style & WSTYLE_LIST_MULTISELECT)) m_pList->DeselectAll();
									pI->m_Flags |= CMWND_LISTITEM_SELECTED;
									m_pList->OnTouchItem(iItem, 1);
								}
							}
							else
							{
								if ((pI->m_Flags & CMWND_LISTITEM_SELECTED))
								{
									pI->m_Flags &= ~CMWND_LISTITEM_SELECTED;
									m_pList->OnTouchItem(iItem, 0);
								}
							}
						}
					}
				}
				Wait();
			}

			if ((iItem >= 0) && m_pWndParent && (LastBtn & 2))
			{
				m_pList->OnAction(iItem);
				// Pressed!
				return m_pWndParent->OnMessage(&CMWnd_Message(WMSG_COMMAND, m_pList->m_ID, iItem, LastBtn));
			}
		}*/
		break;

	}
	return CMWnd_Client::OnMessage(_pMsg);
}

void CMWnd_ListClient::RenderColumn(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client, const CPnt& _Pos, int _iItem, int _nItems, int _Width)
{
	MAUTOSTRIP(CMWnd_ListClient_RenderColumn, MAUTOSTRIP_VOID);
	CRC_Font* pF = m_pList->GetFont("SYSTEM");

	int Alpha = m_pList->m_Col_Alpha << 24;
//	int ColH = m_pList->m_Col_Highlight + Alpha;
//	int ColD = m_pList->m_Col_Shadow + Alpha;
	int ColM = m_pList->m_Col_Background + Alpha;
	int ColBG_Sel = m_pList->m_Col_Custom1_Focus + Alpha;
	int ColT = m_pList->m_Col_Custom0 + Alpha;
	int ColT_Sel = m_pList->m_Col_Custom0_Focus + Alpha;


	int nItems = m_pList->GetListItemCount();
	int iEndItem = _iItem+_nItems;
	if (iEndItem > nItems) iEndItem = nItems;

	int iItem = _iItem;

	{
		CRct Total(_Pos, CPnt(_Width, _nItems*CMWND_LIST_ITEMHEIGHT) + _Pos);
		if (!_Clip.Visible(Total)) return;
	}

	for(int i = 0; i < _nItems; i++, iItem++)
	{
		int x = _Pos.x;
		int y = _Pos.y + i * CMWND_LIST_ITEMHEIGHT;
		CClipRect ItemClip;
		CRct ItemRct(x, y, x + _Width, y+CMWND_LIST_ITEMHEIGHT);
		ItemClip.clip = ItemRct;
		if (!_Clip.Visible(ItemClip.clip)) continue;
		ItemClip.ofs = CPnt(0,0);
		ItemClip += _Clip;
		
//	LogFile(CStrF("I %d", iItem));

		if (iItem < nItems)
		{
			CMWnd_ListItem* pI = m_pList->m_lspItems[iItem];
			int bSelected = (pI->m_Flags & CMWND_LISTITEM_SELECTED);
			int TCol = ColT;
			int BGCol = ColM;
			if (bSelected) 
			{
				TCol = ColT_Sel;
				BGCol = ColBG_Sel;
			}
			_pRCUtil->Rect(ItemClip, ItemRct, BGCol);
			ItemClip.clip.p1.x -= 2;
			_pRCUtil->Text(ItemClip, pF, x+3, y+1, pI->m_Text, TCol);
		}
		else
			_pRCUtil->Rect(ItemClip, ItemRct, ColM);
	}
}

void CMWnd_ListClient::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	MAUTOSTRIP(CMWnd_ListClient_OnPaint, MAUTOSTRIP_VOID);
	int nItems = m_pList->GetListItemCount();
	const int itemh = CMWND_LIST_ITEMHEIGHT;

	int Alpha = m_pList->m_Col_Alpha << 24;
//	int ColH = m_pList->m_Col_Highlight + Alpha;
	int ColD = m_pList->m_Col_Shadow + Alpha;
	int ColM = m_pList->m_Col_Background + Alpha;

//	int ColBG_Sel = m_pList->m_Col_Custom1_Focus + Alpha;
//	int ColT = m_pList->m_Col_Custom0 + Alpha;
//	int ColT_Sel = m_pList->m_Col_Custom0_Focus + Alpha;

	int x0 = 0;
//	int y0 = 0;
	int x1 = _Client.GetWidth();
//	int y1 = _Client.GetHeight();
//	int Style = m_pList->m_Style;
/*	if (Style & WSTYLE_CLIENTEDGE)
		_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, ColD, ColH, true);

	_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, 0xff000000, 0xff000000, true);
*/
//	CClipRect Clip(x0, y0, x1, y1);
//	Clip += _Clip;

	{
		int y1 = 0;
		int y2 = 1;
		_pRCUtil->Rect(_Clip, CRct(x0, y1, x1, y2), ColM);
	}

	if (m_pList->m_Style & WSTYLE_CLIENT_SCROLLY)
	{
		RenderColumn(_pRCUtil, _Clip, _Client, CPnt(0, 1), 0, nItems, _Client.GetWidth());
		{
//			int w = m_pList->m_ListWidth;
			int y1 = nItems*itemh;
			int y2 = _Client.GetHeight();
			_pRCUtil->Rect(_Clip, CRct(0, y1, _Client.GetWidth(), y2), ColM);
//			_pRCUtil->Rect(_Clip, CRct(w, 0, _Client.GetWidth(), y1), ColM);
		}
	}
	else
	{
		int w = m_pList->m_ListWidth;
		int nPerCol = m_pList->m_nItemPerCol;
		int nCol = m_pList->m_nColumns;
		for(int iCol = 0; iCol < nCol; iCol++)
		{
			RenderColumn(_pRCUtil, _Clip, _Client, CPnt(iCol * w, 1), nPerCol * iCol, nPerCol, m_pList->m_ListWidth);
			if (iCol < nCol-1)
				_pRCUtil->Rect(_Clip, CRct((iCol+1)*w-1, 3, (iCol+1)*w, nPerCol*CMWND_LIST_ITEMHEIGHT-3), ColD);
		}

		{
			int y1 = nPerCol*itemh;
			int y2 = _Client.GetHeight();
			if (y2 > y1)
				_pRCUtil->Rect(_Clip, CRct(0, y1, _Client.GetWidth(), y2), ColM);

			_pRCUtil->Rect(_Clip, CRct(nCol*w, 0, _Client.GetWidth(), y1), ColM);
		}
	}
}

// -------------------------------------------------------------------
//  CMWnd_ModList3 vSetProportions
// -------------------------------------------------------------------
void CMWnd_ModList3::vSetProportions(int nVisibleItems)
{
	MAUTOSTRIP(CMWnd_ModList3_vSetProportions, MAUTOSTRIP_VOID);
	m_iVisibleItems = nVisibleItems;
	m_iItemHeight = m_Pos.GetHeight() / nVisibleItems;
	m_iItemWidth = m_Pos.GetWidth();
}

// -------------------------------------------------------------------
//  CMWnd_ModList3 vSetItemText
// -------------------------------------------------------------------
void CMWnd_ModList3::vSetItemText()
{
	MAUTOSTRIP(CMWnd_ModList3_vSetItemText, MAUTOSTRIP_VOID);
	int nw = m_lButton.Len();

	int iCounter = 0;

	for(int i = 0; i < nw; i++)
	{
		CMWnd_ModListItem3 * pItem = m_lButton[i];

		if (iCounter < m_lItems.Len())
		{
			pItem->m_ItemText = m_lItems[iCounter];
			pItem->m_Text = pItem->m_ItemText;
			pItem->m_NumItem = i;
			
			++iCounter;
		}
		else
		{
			// Disable

			pItem->SetStatus(WSTATUS_DISABLED,true);
		}
	}
}

// -------------------------------------------------------------------
//  CMWnd_ModList3 OnPaint
// -------------------------------------------------------------------
void CMWnd_ModList3::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	MAUTOSTRIP(CMWnd_ModList3_OnPaint, MAUTOSTRIP_VOID);
	if (m_bDrawControlFrame)
	{
		int x0 = 0;
		int y0 = 0;
		int x1 = m_Pos.GetWidth();
		int y1 = m_Pos.GetHeight();
		
		_pRCUtil->SetTexture("GROUND_TILE04_07");
		
		_pRCUtil->SetTextureScale(1,1);
		_pRCUtil->SetTextureOrigo(_Clip, CPnt(x0,y0));
		
		_pRCUtil->Frame(_Clip, x0++, y0++, x1--, y1--, 0xb0c0c0c0, 0xb0cccccc);
		_pRCUtil->Rect3D(_Clip, CRct(x0++, y0++, x1--, y1--), 0xb0202020, 0xb0202020, 0xb0202020);
	}
}

// -------------------------------------------------------------------
//  CMWnd_ModList3 vSetItemData
// -------------------------------------------------------------------
void CMWnd_ModList3::vSetItemData(int nItem, uint32 dwData)
{
	MAUTOSTRIP(CMWnd_ModList3_vSetItemData, MAUTOSTRIP_VOID);
	int nw = m_lButton.Len();

//	int iCounter = 0;

	for(int i = 0; i < nw; i++)
	{
		CMWnd_ModListItem3 * pItem = m_lButton[i];

		if (pItem->m_NumItem == nItem)
		{
			pItem->m_dwData = dwData;
			break;
		}
	}
}

// -------------------------------------------------------------------
//  CMWnd_ModList3 vSetItemDataText
// -------------------------------------------------------------------
void CMWnd_ModList3::vSetItemDataText(int nItem, const CStr& strData)
{
	MAUTOSTRIP(CMWnd_ModList3_vSetItemDataText, MAUTOSTRIP_VOID);
	int nw = m_lButton.Len();

//	int iCounter = 0;

	for(int i = 0; i < nw; i++)
	{
		CMWnd_ModListItem3 * pItem = m_lButton[i];

		if (pItem->m_NumItem == nItem)
		{
			pItem->m_strData = strData;
			break;
		}
	}
}

// -------------------------------------------------------------------
//  CMWnd_ModList3 vSetList
// -------------------------------------------------------------------
void CMWnd_ModList3::vSetList(TArray<CStr>& _lItems)
{
	MAUTOSTRIP(CMWnd_ModList3_vSetList, MAUTOSTRIP_VOID);
	m_lItems = _lItems;
	vSetItemText();
}

// -------------------------------------------------------------------
//  CMWnd_ModList3 vOnLeftClick
// -------------------------------------------------------------------
void CMWnd_ModList3::vOnLeftClick(CMWnd_ModListItem3 * pItem)
{
	MAUTOSTRIP(CMWnd_ModList3_vOnLeftClick, MAUTOSTRIP_VOID);
}

// -------------------------------------------------------------------
//  CMWnd_ModList3 vOnSelChange
// -------------------------------------------------------------------
void CMWnd_ModList3::vOnSelChange(CMWnd_ModListItem3 * pItem)
{
	MAUTOSTRIP(CMWnd_ModList3_vOnSelChange, MAUTOSTRIP_VOID);
}

// -------------------------------------------------------------------
//  CMWnd_ModList3 vRecreateList
// -------------------------------------------------------------------
void CMWnd_ModList3::vRecreateList(int numItems)
{
	MAUTOSTRIP(CMWnd_ModList3_vRecreateList, MAUTOSTRIP_VOID);
	float yy = 0;

	for(int i = 0; i < numItems; ++i)
	{
		CMWnd_ModListItem3 * pItem = MNew(CMWnd_ModListItem3);
		
		pItem->Create(
			this, 
			CMWnd_Param(0, int(yy), m_Pos.GetWidth(), m_iItemHeight, 0), 
			"", 
			"", 
			m_TextStyle);

		pItem->m_ItemText = "";
//		pItem->m_NoOnPaintBackground = true;
		pItem->m_List = this;
		pItem->EvaluateKey(NULL, "TEXTCOLOR_BG", "0xffffffff");
// PC PORT: We handle keypresses ourself here...
#ifdef GUI_PC
		pItem->m_Style |= WSTYLE_NOKEYPROCESS;
#endif

		m_lButton.Add(pItem);
		
		yy+=m_iItemHeight;
	}
}

// -------------------------------------------------------------------
//  CMWnd_ModList3 vSetInitialFocus
// -------------------------------------------------------------------
void CMWnd_ModList3::vSetInitialFocus()
{
	MAUTOSTRIP(CMWnd_ModList3_vSetInitialFocus, MAUTOSTRIP_VOID);
	if (m_lButton.Len())
	{
		CMWnd_ModListItem3 * pItem = (CMWnd_ModListItem3*)m_lButton[0];
		pItem->SetFocus();
	}
}

void CMWnd_ModList3::vSetFocus(int i)
{
	MAUTOSTRIP(CMWnd_ModList3_vSetFocus, MAUTOSTRIP_VOID);
	if (m_lButton.Len() && i >= 0 && i < m_lButton.Len())
	{
		CMWnd_ModListItem3 * pItem = (CMWnd_ModListItem3*)m_lButton[i];
		pItem->SetFocus();
	}
}


// -------------------------------------------------------------------
//  CMWnd_ModListItem3 OnPressed
// -------------------------------------------------------------------
int CMWnd_ModListItem3::OnPressed(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_ModListItem3_OnPressed, 0);
	m_List->vOnLeftClick(this);

	return 1;
}

// -------------------------------------------------------------------
//  CMWnd_ModListItem3 OnSetFocus
// -------------------------------------------------------------------
void CMWnd_ModListItem3::OnSetFocus()
{
	MAUTOSTRIP(CMWnd_ModListItem3_OnSetFocus, MAUTOSTRIP_VOID);
	m_List->vOnSelChange(this);

	CMWnd_Button::OnSetFocus();
}

// -------------------------------------------------------------------
//  CMWnd_ModListItem3 OnPaintFocus
// -------------------------------------------------------------------
void CMWnd_ModListItem3::OnPaintFocus(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	MAUTOSTRIP(CMWnd_ModListItem3_OnPaintFocus, MAUTOSTRIP_VOID);
	CRC_Font* pF = GetFont("TEXT");
	
	if (pF) 
	{
//		int bDown = (m_Status & WSTATUS_PRESSURE) ? 1 : 0;
		
//		_pRCUtil->GetRC()->Attrib_Push();
//		_pRCUtil->GetRC()->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
		_pRCUtil->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
		
//		CMTime Time = CMTime::GetCPU();
		CMTime Time;
		Time.Snapshot();
		
		fp32 coll = M_Sin(Time.GetTimeModulusScaled(4.0f, _PI2))*128.0f+128.0f;
		int col2 = CPixel32::From_fp32(255,255,255,coll*(1.0f/20.0f)+15);
		
		_pRCUtil->SetTexture(0);
		_pRCUtil->Rect3D(_Clip, CRct(0, 0, GetWidth(), GetHeight()), col2, col2, col2);
		
		if (GetRootWnd())
			_pRCUtil->SetTextureOrigo(GetRootWnd()->m_AbsPos,CPnt(0,0));
		
//		_pRCUtil->GetRC()->Attrib_Pop();
	}
}

// -------------------------------------------------------------------
//  CMWnd_ModListItem3 OnPaint
// -------------------------------------------------------------------
void CMWnd_ModListItem3::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	MAUTOSTRIP(CMWnd_ModListItem3_OnPaint, MAUTOSTRIP_VOID);
	int bDown = (m_Status & WSTATUS_PRESSURE) ? 1 : 0;

//	int x0 = 0;
//	int y0 = 0;
//	int x1 = m_Pos.GetWidth();
//	int y1 = m_Pos.GetHeight();

	_pRCUtil->SetTexture(0);

	if (m_Text != "")
	{
		CRC_Font* pF = GetFont("TEXT");
		if (pF) 
		{
			int Style = m_TextStyle & (WSTYLE_TEXT_CENTER | WSTYLE_TEXT_CENTERY	| WSTYLE_TEXT_WORDWRAP);

			CStr sDrawText;
			sDrawText = m_Text;

			int iSlot = 1;

			if (sDrawText.Len())
			{
				iSlot = sDrawText[0];
			}

			// Time

//			CMTime Time = CMTime::GetCPU();
			CMTime Time;
			Time.Snapshot();
			fp32 col1 = (M_Sin(Time.GetTimeModulusScaled(2.5f, _PI2) + (fp32)iSlot*15.0f)*128.0f+128.0f);
			int col2 = CPixel32::From_fp32(200, 215, 240, col1*(1.0f/7.0f) + 105);

			CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, sDrawText, bDown, bDown,
				Style, col2, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
		}
	}
}

// -------------------------------------------------------------------
//  CMWnd_Starview
// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd_Starview, CMWnd_View);


void CMWnd_Starview::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	MAUTOSTRIP(CMWnd_Starview_OnPaint, MAUTOSTRIP_VOID);
	{
		int w = _Client.GetWidth();
		int h = _Client.GetHeight();
		int Alpha = 0xff000000;
		_pRCUtil->Rect(_Clip, CRct(0, 0, w, h), 0x080810 + Alpha);
	}

	{
		int w = _Client.GetWidth();
		int h = _Client.GetHeight();

		int ns = 300;
//		CMTime t = CMTime::GetCPU();
		CMTime t;
		t.Snapshot();

//		srand(0);
		MRTC_GetRand()->InitRand(0);
		int xmid = (w >> 1);
		int ymid = (h >> 1);
		for(int i = 0; i < ns; i++)
		{
			fp32 x = Random - 0.5f;
			fp32 y = Random - 0.5f;
			CMTime zT = (CMTime::CreateFromSeconds(Random) - t.Scale(0.1f));
			fp32 z = zT.GetTimeModulus(1.0f);
			int xs = xmid + int(x*w*2.0f / (z*10.0f+1.0f));
			int ys = ymid + int(y*w*2.0f / (z*10.0f+1.0f));

			_pRCUtil->Pixel(_Clip, CPnt(xs, ys), CPixel32(0xffffffff) * (1.0f-z));
		}
	}
}

// -------------------------------------------------------------------
//  Global utility functions:
// -------------------------------------------------------------------
spCMWnd CMWnd_Create(CKeyContainerNode* _pRsrc, const CStr& _Name, spCMWnd _spWndParent)
{
	MAUTOSTRIP(CMWnd_Create_4, NULL);
	CKeyContainerNode* pN = _pRsrc->FindContainer("NAME", _Name);
	if (!pN) return NULL;

	CKeyContainer* pK = pN->GetKeys();
	int iKeyCls = pK->GetKeyIndex("CLASSNAME");
	if (iKeyCls < 0) return NULL;

	spCMWnd spWnd = (CMWnd*) MRTC_GetObjectManager()->CreateObject("CMWnd_" + pK->GetKeyValue(iKeyCls));
	if (!spWnd) return NULL;

	if (!_spWndParent)
		spWnd->Create(pN, g_spDesktop);
	else
		spWnd->Create(pN, _spWndParent);

	return spWnd;
}

spCMWnd CMWnd_Create(CRegistry* _pRsrc, const CStr& _Name, spCMWnd _spWndParent)
{
	MAUTOSTRIP(CMWnd_Create_5, NULL);
	CRegistry* pN = _pRsrc->FindChild("WINDOW", _Name, false);
	if (!pN) 
		return NULL;

	CRegistry* pClass = pN->Find("CLASSNAME");
	if (!pClass) 
		return NULL;

	CStr ObjName = "CMWnd_" + pClass->GetThisValue();
	spCMWnd spWnd = (CMWnd*) MRTC_GetObjectManager()->CreateObject(ObjName);
	if (!spWnd) 
		return NULL;

	if (!_spWndParent) _spWndParent = g_spDesktop;

	spWnd->Create(pN, _spWndParent);
	if (!_spWndParent) spWnd->OnSetAutoFocus();

	return spWnd;
}

// -------------------------------------------------------------------
int CMWnd_MessageBox(const CStr& _Caption, const CStr& _Message, int _Style)
{
	MAUTOSTRIP(CMWnd_MessageBox, 0);
	CRct DeskSize = g_spDesktop->GetPosition();

	spCMWnd spMB = MNew4(CMWnd_Window, g_spDesktop, CMWnd_Param(10, 10, 240, 100, 
		WSTYLE_WND_CAPTION | WSTYLE_WND_THINFRAME| WSTYLE_WND_CLOSEBTN | 
		WSTYLE_MOVABLE  | WSTYLE_ONTOP), "", _Caption);

	MNew4(CMWnd_Button,spMB, CMWnd_Param(40, 40, 60, 20, 0), "MB_OK", "Ok");
	MNew4(CMWnd_Button,spMB, CMWnd_Param(140, 40, 60, 20, 0), "MB_CANCEL", "Cancel");

	return spMB->DoModal();
}

// -------------------------------------------------------------------
