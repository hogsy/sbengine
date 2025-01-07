#include "PCH.h"

#include "MWinCtrl.h"

//----------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd_Explorer, CMWnd_List);


CMWnd_Explorer::CMWnd_Explorer()
{
	MAUTOSTRIP(CMWnd_Explorer_ctor, MAUTOSTRIP_VOID);
#ifdef PLATFORM_DREAMCAST
	m_Path = "GDROM:\\";
#else
	m_Path = "c:\\";
#endif
}

void CMWnd_Explorer::OnCreate()
{
	MAUTOSTRIP(CMWnd_Explorer_OnCreate, MAUTOSTRIP_VOID);
	CMWnd_List::OnCreate();
	Explorer_Update();
}

void CMWnd_Explorer::Explorer_Update()
{
	MAUTOSTRIP(CMWnd_Explorer_Explorer_Update, MAUTOSTRIP_VOID);
	M_TRY
	{
		m_Dir.ReadDirectory(m_Path + "*");
	}
	M_CATCH(
	catch(CCException)
	{
	}
	)

	RemoveAllList();
	int nFiles = m_Dir.GetFileCount();
	for(int i = 0; i < nFiles; i++)
		AddListItem(m_Dir.GetDecoratedName(i));
}

void CMWnd_Explorer::Explorer_Go(CStr _Path)
{
	MAUTOSTRIP(CMWnd_Explorer_Explorer_Go, MAUTOSTRIP_VOID);
	int i = _Path.Len();
	if (!i) 
		m_Path = "\\";
	else
	{
		if (_Path[i-1] != '\\')
			m_Path = _Path + "\\";
		else
			m_Path = _Path;
	}
	Explorer_Update();
}

void CMWnd_Explorer::Explorer_GoParent()
{
	MAUTOSTRIP(CMWnd_Explorer_Explorer_GoParent, MAUTOSTRIP_VOID);
	int i = m_Path.FindReverse("\\");
	if (i == m_Path.Len()-1)
		i = m_Path.Copy(0, i).FindReverse("\\");
	if (i >= 0)
		m_Path = m_Path.Copy(0, i+1);
	Explorer_Update();
}

void CMWnd_Explorer::Explorer_GoSubDir(CStr _Name)
{
	MAUTOSTRIP(CMWnd_Explorer_Explorer_GoSubDir, MAUTOSTRIP_VOID);
	m_Path += _Name + "\\";
	Explorer_Update();
}

void CMWnd_Explorer::OnAction(int _iItem)
{
	MAUTOSTRIP(CMWnd_Explorer_OnAction, MAUTOSTRIP_VOID);
	if (m_Dir.IsDirectory(_iItem))
	{
		CStr Name = m_Dir.GetFileName(_iItem);
		if (Name == ".") return;
		if (Name == "..")
			Explorer_GoParent();
		else
			Explorer_GoSubDir(Name);
	}
	else
	{
		// Notify someone...
	}
}

//----------------------------------------------------------------
// CMWnd_FileRequester
//----------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd_FileRequester, CMWnd_Window);


void CMWnd_FileRequester::DoCreate(CMWnd* _pWndParent, const CMWnd_Param& _Param, CStr _ID)
{
	MAUTOSTRIP(CMWnd_FileRequester_DoCreate, MAUTOSTRIP_VOID);
	CMWnd_Window::DoCreate(_pWndParent, _Param, _ID);

	spCMWnd spWnd = MNew4(CMWnd_Text, this, 
		
		(CMWnd_Param(10, 120, 100, 15, WSTYLE_TEXT_CUTOUT | WSTYLE_TEXT_HIGHLIGHT | WSTYLE_TEXT_SHADOW, WSTATUS_ALIGNY0_BOTTOM | WSTATUS_ALIGNY1_BOTTOM, CPnt(0,-40), CPnt(0,-25)))
		
		, "", "File name:");
	spWnd->m_Col_Background = 0xff000000;

	spWnd = MNew4(CMWnd_Text, this, 
		(CMWnd_Param(10, 140, 100, 15, WSTYLE_TEXT_CUTOUT | WSTYLE_TEXT_HIGHLIGHT | WSTYLE_TEXT_SHADOW, WSTATUS_ALIGNY0_BOTTOM | WSTATUS_ALIGNY1_BOTTOM, CPnt(0,-20), CPnt(0,-5)))
		, "", "File of types:");
	spWnd->m_Col_Background = 0xff000000;

	spCMWnd_List spWnd1 = MNew(CMWnd_List);
	spCMWnd_Explorer spWnd2 = MNew(CMWnd_Explorer);
	spWnd1->Create(NULL, CMWnd_Param(0, 0, 100, 100, WSTYLE_CLIENT_SCROLLX | WSTYLE_CLIENT_BORDER | WSTYLE_LIST_AUTOWIDTH), "DRIVES");
	spWnd2->Create(NULL, CMWnd_Param(0, 0, 100, 100, WSTYLE_CLIENT_SCROLLX | WSTYLE_CLIENT_BORDER | WSTYLE_LIST_AUTOWIDTH), "EXPLORER");

	m_pWndSplit = MNew(CMWnd_SplitWnd);
	m_pWndSplit->Create(NULL, CMWnd_Param(0, 0, GetWidth(), GetHeight(), WSTYLE_CLIENTEDGE, 
		WSTATUS_MAXIMIZED | WSTATUS_ALIGNX0_LEFT | WSTATUS_ALIGNX1_RIGHT | WSTATUS_ALIGNY0_TOP | WSTATUS_ALIGNY1_BOTTOM, CPnt(5,5), CPnt(-5,-50)), "", *spWnd1, *spWnd2, 50);
	AddChild(m_pWndSplit);

	m_pWndDrives = spWnd1;
	m_pWndExplorer = spWnd2;
	OnMove();
}

void CMWnd_FileRequester::EvaluateKey(CMWnd_Param* _pParam, CStr _Key, CStr _Value)
{
	MAUTOSTRIP(CMWnd_FileRequester_EvaluateKey, MAUTOSTRIP_VOID);
	CMWnd_Window::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_FileRequester::Clear()
{
	MAUTOSTRIP(CMWnd_FileRequester_Clear, MAUTOSTRIP_VOID);
	m_pWndDrives = NULL;
	m_pWndExplorer = NULL;
	m_pWndSplit = NULL;
}

CMWnd_FileRequester::CMWnd_FileRequester()
{
	MAUTOSTRIP(CMWnd_FileRequester_ctor, MAUTOSTRIP_VOID);
	Clear();
}

void CMWnd_FileRequester::OnCreate()
{
	MAUTOSTRIP(CMWnd_FileRequester_OnCreate, MAUTOSTRIP_VOID);
	CMWnd_Window::OnCreate();

	if (m_pWndDrives)
	{
		lCStr lDrives = CDiskUtil::GetDrives();
		for(int i = 0; i < lDrives.Len(); i++)
			m_pWndDrives->AddListItem(lDrives[i] + ":");
	}
}

void CMWnd_FileRequester::OnMove()
{
	MAUTOSTRIP(CMWnd_FileRequester_OnMove, MAUTOSTRIP_VOID);
	CMWnd_Window::OnMove();
}

aint CMWnd_FileRequester::OnMessage(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_FileRequester_OnMessage, 0);
	switch(_pMsg->m_Msg)
	{
	case WMSG_COMMAND :
		if (_pMsg->m_pID)
		{
			if (!strcmp(_pMsg->m_pID, "DRIVES"))
			{
				if (m_pWndExplorer)
					m_pWndExplorer->Explorer_Go(m_pWndDrives->GetListItem(_pMsg->m_Param0));
				return true;
			}
			else if (!strcmp(_pMsg->m_pID, "EXPLORER"))
			{
				return true;
			}
		}
	}
	return CMWnd_Window::OnMessage(_pMsg);
}

