#ifndef __MWNDCTRLEDIT_H
#define __MWNDCTRLEDIT_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Edit controller

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CMWnd_Edit
\*____________________________________________________________________________________________*/


#include "MWindows.h"

// -------------------------------------------------------------------
//  CMWnd_Edit
// -------------------------------------------------------------------
class CMWnd_Edit : public CMWnd
{
	MRTC_DECLARE;

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
	
	int m_CursorLine;
	int m_CursorPos;

	void Clear();

public:

	int ProcessTextKey(CScanKey _Key);
	CStr m_Text;

public:

	virtual void EvaluateKey(CMWnd_Param* _pParam, CStr _Key, CStr _Value);

	virtual void PreCreate(CMWnd_Param* _pParam);
	
	virtual void Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, CStr _ID = "", CStr _Text = "",
		int _TextStyle = WSTYLE_TEXT_CENTER | WSTYLE_TEXT_CENTERY | WSTYLE_TEXT_CUTOUT | WSTYLE_TEXT_SHADOW | WSTYLE_TEXT_HIGHLIGHT, 
		CStr _Image = "", CPnt _ImagePos = CPnt(0,0), CPnt _ImageSize = CPnt(0,0), int _ImageStyle = 0);

	CMWnd_Edit();
	
	CMWnd_Edit(CMWnd* _pWndParent, const CMWnd_Param& _Param, CStr _ID = "", CStr _Text = "",
		int _TextStyle = WSTYLE_TEXT_CENTER | WSTYLE_TEXT_CENTERY | WSTYLE_TEXT_CUTOUT | WSTYLE_TEXT_SHADOW | WSTYLE_TEXT_HIGHLIGHT, 
		CStr _Image = "", CPnt _ImagePos = CPnt(0,0), CPnt _ImageSize = CPnt(0,0), int _ImageStyle = 0);

	virtual aint OnMessage(const CMWnd_Message* _pMsg);
	
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

#endif //__INC_MWINCTRL3D
