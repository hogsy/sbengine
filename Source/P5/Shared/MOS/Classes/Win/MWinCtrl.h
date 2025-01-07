#ifndef __MWINCTRL_H
#define __MWINCTRL_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Misc window classes

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CMWnd_Explorer
					CMWnd_FileRequester
\*____________________________________________________________________________________________*/

#include "MWindows.h"

//----------------------------------------------------------------
// CMWnd_Explorer
//----------------------------------------------------------------
class CMWnd_Explorer : public CMWnd_List
{
	MRTC_DECLARE;

	CDirectoryNode m_Dir;
	CStr m_Path;
	TArray<CStr> m_lWildcards;

public:
	CMWnd_Explorer();
	virtual void OnCreate();

	virtual void Explorer_Update();
	virtual void Explorer_Go(CStr _Path);
	virtual void Explorer_GoParent();
	virtual void Explorer_GoSubDir(CStr _Name);
	virtual void OnAction(int _iItem);
};

typedef TPtr<CMWnd_Explorer> spCMWnd_Explorer;

//----------------------------------------------------------------
// CMWnd_FileRequester
//----------------------------------------------------------------
class CMWnd_FileRequester : public CMWnd_Window
{
	MRTC_DECLARE;

	CMWnd_List* m_pWndDrives;
	CMWnd_Explorer* m_pWndExplorer;
	CMWnd_SplitWnd* m_pWndSplit;

protected:
	virtual void DoCreate(CMWnd* _pWndParent, const CMWnd_Param& _Param, CStr _ID);
	virtual void EvaluateKey(CMWnd_Param* _pParam, CStr _Key, CStr _Value);
	void Clear();

public:
	CMWnd_FileRequester();
	virtual void OnCreate();
	virtual void OnMove();
	virtual aint OnMessage(const CMWnd_Message* _pMsg);
};

#endif //_INC_MWINCTRL
