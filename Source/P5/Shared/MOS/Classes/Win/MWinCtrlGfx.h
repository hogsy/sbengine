#ifndef __MGFXWINDOWS_H
#define __MGFXWINDOWS_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Misc graphical window classes

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CMWnd_GfxWindow
					CMWnd_GfxRect
					CMWnd_GfxButton
					CMWnd_GfxText
\*____________________________________________________________________________________________*/

#include "MWindows.h"

// -------------------------------------------------------------------
//  CMWnd_GfxWindow
// -------------------------------------------------------------------
class CMWnd_GfxWindow : public CMWnd_Window
{
	MRTC_DECLARE;

public:
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

// -------------------------------------------------------------------
//  CMWnd_GfxRect
// -------------------------------------------------------------------
class CMWnd_GfxRect : public CMWnd_Window
{
	MRTC_DECLARE;

	CStr m_Texture;
	int m_iTexture;

public:
	CMWnd_GfxRect();

	virtual void Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, CStr _ID, CStr _Texture);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

// -------------------------------------------------------------------
//  CMWnd_GfxButton
// -------------------------------------------------------------------
class CMWnd_GfxButton : public CMWnd_Button
{
	MRTC_DECLARE;

public:
	CStr m_Script_Pressed;

	virtual void Execute(CStr& _Script);
	virtual int OnPressed(const CMWnd_Message* _pMsg);
	virtual void EvaluateKey(CMWnd_Param* _pParam, CStr _Key, CStr _Value);
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};

// -------------------------------------------------------------------
//  CMWnd_GfxText
// -------------------------------------------------------------------
class CMWnd_GfxText : public CMWnd_Text
{
	MRTC_DECLARE;

protected:
	CStr m_Font;

public:
	CMWnd_GfxText();
	virtual void Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, CStr _ID = "", CStr _Text = "", int _TextStyle = 0, CStr _Font = "");
	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client);
};


#endif // _INC_MGFXWINDOWS
