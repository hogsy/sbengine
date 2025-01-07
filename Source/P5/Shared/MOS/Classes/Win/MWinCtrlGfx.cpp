
#include "PCH.h"

#include "MWinCtrlGfx.h"


MRTC_IMPLEMENT_DYNAMIC(CMWnd_GfxWindow, CMWnd_Window);


void CMWnd_GfxWindow::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	MAUTOSTRIP(CMWnd_GfxWindow_OnPaint, MAUTOSTRIP_VOID);
}



MRTC_IMPLEMENT_DYNAMIC(CMWnd_GfxButton, CMWnd_Button);


void CMWnd_GfxButton::Execute(CStr& _Script)
{
	MAUTOSTRIP(CMWnd_GfxButton_Execute, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
//ConOutL("(CMWnd_GfxButton::Execute) " + _Script);
	pCon->ExecuteString(_Script);
}


int CMWnd_GfxButton::OnPressed(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_GfxButton_OnPressed, 0);
	if (m_Script_Pressed != "")
		Execute(m_Script_Pressed);

	return CMWnd_Button::OnPressed(_pMsg);
}

void CMWnd_GfxButton::EvaluateKey(CMWnd_Param* _pParam, CStr _Key, CStr _Value)
{
	MAUTOSTRIP(CMWnd_GfxButton_EvaluateKey, MAUTOSTRIP_VOID);
	if (_Key == "SCRIPT_PRESSED")
	{
		m_Script_Pressed = _Value;
		return;
	}
	CMWnd_Button::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_GfxButton::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	MAUTOSTRIP(CMWnd_GfxButton_OnPaint, MAUTOSTRIP_VOID);
	_pRCUtil->SetTexture(0);

	int bDown = (m_Status & WSTATUS_PRESSURE) ? 1 : 0;

	_pRCUtil->Frame(_Clip, 0,0,m_Pos.GetWidth(),m_Pos.GetHeight(), 0xc0000000, 0x20000000, true);
	_pRCUtil->Frame(_Clip, 1,1,m_Pos.GetWidth()-1,m_Pos.GetHeight()-1, 0xff000000, 0xff000000, true);

	_pRCUtil->SetTexture("GROUND_TILE04_07");
	_pRCUtil->Frame(_Clip, 2,2,m_Pos.GetWidth()-2,m_Pos.GetHeight()-2, 0xff606060, 0xff202020);
	_pRCUtil->Rect3D(_Clip, CRct(3,3,m_Pos.GetWidth()-3,m_Pos.GetHeight()-3), 0xffa0a0a0, 0xff606060, 0xff303030);
	_pRCUtil->SetTexture(0);

	if ((char*)m_Text)
	{
		CRC_Font* pF = GetFont("TEXT");
		if (pF) 
		{
			int Style = m_TextStyle & (WSTYLE_TEXT_CENTER | WSTYLE_TEXT_CENTERY	| WSTYLE_TEXT_WORDWRAP);

			CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, m_Text, bDown, bDown,
				Style, m_TextColorM, m_TextColorH, m_TextColorD, m_Pos.GetWidth(), m_Pos.GetHeight());

			if (m_Status & WSTATUS_MOUSEOVER)
			{
				int Style = m_TextStyle & (WSTYLE_TEXT_CENTER | WSTYLE_TEXT_CENTERY	| WSTYLE_TEXT_WORDWRAP);
				_pRCUtil->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
				CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, m_Text, bDown+2, bDown,
					Style, 0x2fc0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
				CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, m_Text, bDown-2, bDown,
					Style, 0x2fc0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
				CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, m_Text, bDown+4, bDown,
					Style, 0x1fc0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
				CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, m_Text, bDown-4, bDown,
					Style, 0x1fc0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
				CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, m_Text, bDown+6, bDown,
					Style, 0x18c0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
				CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, m_Text, bDown-6, bDown,
					Style, 0x18c0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
				CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, m_Text, bDown+8, bDown,
					Style, 0x0cc0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
				CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, m_Text, bDown-8, bDown,
					Style, 0x0cc0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
				CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, m_Text, bDown+10, bDown,
					Style, 0x08c0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
				CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, m_Text, bDown-10, bDown,
					Style, 0x08c0c0ff, 0, 0, m_Pos.GetWidth(), m_Pos.GetHeight());
			}
		}
	}
}



MRTC_IMPLEMENT_DYNAMIC(CMWnd_GfxText, CMWnd_Text);


CMWnd_GfxText::CMWnd_GfxText()
{
	MAUTOSTRIP(CMWnd_GfxText_ctor, MAUTOSTRIP_VOID);
	m_Col_Background = 0xff7f7f7f;
}

void CMWnd_GfxText::Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, CStr _ID, CStr _Text, int _TextStyle, CStr _Font)
{
	MAUTOSTRIP(CMWnd_GfxText_Create, MAUTOSTRIP_VOID);
	m_Text = _Text;
	m_Font = _Font;
	CMWnd_Text::Create(_pWndParent, _Param, _ID);
	m_Style |= _TextStyle;
}

void CMWnd_GfxText::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	MAUTOSTRIP(CMWnd_GfxText_OnPaint, MAUTOSTRIP_VOID);
	int ColH = m_Col_Highlight;
	int ColM = m_Col_Background;
	int ColD = m_Col_Shadow;
//	int ColDD = m_Col_DarkShadow;

	_pRCUtil->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

	CRC_Font* pF = (m_Font.Len()) ? GetFont(m_Font) : GetFont("TEXT");
	if (pF) 
	{
//		int Style = m_Style;
		CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pF, m_Text, 0, 0,
			m_Style, ColM, ColH, ColD, m_Pos.GetWidth(), m_Pos.GetHeight());
	}
}

// -------------------------------------------------------------------
//  CMWnd_GfxRect
// -------------------------------------------------------------------


MRTC_IMPLEMENT_DYNAMIC(CMWnd_GfxRect, CMWnd_Window);


CMWnd_GfxRect::CMWnd_GfxRect()
{
	MAUTOSTRIP(CMWnd_GfxRect_ctor, MAUTOSTRIP_VOID);
	m_iTexture = 0;
}

void CMWnd_GfxRect::Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, CStr _ID, CStr _Texture)
{
	MAUTOSTRIP(CMWnd_GfxRect_Create, MAUTOSTRIP_VOID);
	m_WndCaptionClass = "";
	m_WndBorderClass = "";
	m_WndClientClass = "";
	CMWnd_Window::Create(_pWndParent, _Param, _ID);

	m_Texture = _Texture;
	m_iTexture = -1;
}

void CMWnd_GfxRect::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	MAUTOSTRIP(CMWnd_GfxRect_OnPaint, MAUTOSTRIP_VOID);
	if(m_iTexture == -1)
	{
		m_iTexture = _pRCUtil->GetTC()->GetTextureID(m_Texture);
	}

	if(m_iTexture > 0)
	{
		_pRCUtil->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		_pRCUtil->GetAttrib()->Attrib_Disable(CRC_FLAGS_CULL);
		_pRCUtil->GetAttrib()->Attrib_Disable(CRC_FLAGS_ZWRITE);
		_pRCUtil->SetTexture(m_iTexture);
		_pRCUtil->SetTextureScale(fp32(_pRCUtil->GetTextureWidth()) / _Client.GetWidth(), fp32(_pRCUtil->GetTextureHeight()) / _Client.GetHeight());
		_pRCUtil->SetTextureOrigo(_Clip, CPnt(0, 0));
		_pRCUtil->Rect(_Clip, _Client, 0xffffffff);
		_pRCUtil->SetTexture(0);
	}
}
