
#include "PCH.h"

#include "MWndCtrlEdit.h"

#include "MFloat.h"

#ifdef COMPILER_MSVC
#pragma warning(disable : 4065)
#endif

// -------------------------------------------------------------------
// MEdit Control
// -------------------------------------------------------------------


MRTC_IMPLEMENT_DYNAMIC(CMWnd_Edit, CMWnd);


void CMWnd_Edit::Clear()
{
	MAUTOSTRIP(CMWnd_Edit_Clear, MAUTOSTRIP_VOID);
	m_TextStyle = WSTYLE_TEXT_CENTER | WSTYLE_TEXT_CENTERY | WSTYLE_TEXT_CUTOUT | WSTYLE_TEXT_SHADOW | WSTYLE_TEXT_HIGHLIGHT;
	m_TextColorM = 0xff000000;
	m_TextColorH = 0x60ffffff;
	m_TextColorD = 0x60000000;
	m_TextPos = CPnt(0, 0);

	m_ImageStyle = 0;
	m_ImagePos = CPnt(0, 0);
	m_ImageSize = CPnt(0, 0);

	m_CursorPos = 0;
}

void CMWnd_Edit::EvaluateKey(CMWnd_Param* _pParam, CStr _Key, CStr _Value)
{
	MAUTOSTRIP(CMWnd_Edit_EvaluateKey, MAUTOSTRIP_VOID);
	if (_Key == "TEXT")
	{
		m_Text = _Value;
		m_CursorPos = m_Text.Len();
		return;
	}
	if (_Key == "TEXTCOLOR_BG")
	{
		m_TextColorM = _Value.Val_int();
		return;
	}
	if (_Key == "TEXTCOLOR_HL")
	{
		m_TextColorH = _Value.Val_int();
		return;
	}
	if (_Key == "TEXTCOLOR_SHDW")
	{
		m_TextColorD = _Value.Val_int();
		return;
	}
	if (_Key == "TEXTSTYLE")
	{
		m_TextStyle = 0;
		while(_Value.Len())
		{
			CStr s = _Value.GetStrSep(",").LTrim().RTrim();
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
			else
				LogFile("(CMWnd_Edit::EvaluateKey) Uknown text-style: " + s);
		}
		return;
	}
	if (_Key == "TEXTPOS")
	{
		m_TextPos.x = _Value.GetStrSep(",").Val_int();
		m_TextPos.y = _Value.GetStrSep(",").Val_int();
		return;
	}
	if (_Key == "IMAGE")
	{
		m_Image = _Value;
		return;
	}
	if (_Key == "IMAGESTYLE")
	{
		m_ImageStyle = 0;
		while(_Value.Len())
		{
			CStr s = _Value.GetStrSep(",").LTrim().RTrim();
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
				LogFile("(CMWnd_Edit::EvaluateKey) Uknown text-style: " + s);
		}
		return;
	}
	if (_Key == "IMAGEPOS")
	{
		m_ImagePos.x = _Value.GetStrSep(",").Val_int();
		m_ImagePos.y = _Value.GetStrSep(",").Val_int();
		return;
	}
	if (_Key == "IMAGESIZE")
	{
		m_ImageSize.x = _Value.GetStrSep(",").Val_int();
		m_ImageSize.y = _Value.GetStrSep(",").Val_int();
		return;
	}
	CMWnd::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_Edit::PreCreate(CMWnd_Param* _pParam)
{
	MAUTOSTRIP(CMWnd_Edit_PreCreate, MAUTOSTRIP_VOID);
	CMWnd::PreCreate(_pParam);
	_pParam->m_Style |= WSTYLE_TABSTOP;
}

void CMWnd_Edit::Create(CMWnd* _pWndParent, const CMWnd_Param& _Param, CStr _ID, CStr _Text,
	int _TextStyle, CStr _Image, CPnt _ImagePos, CPnt _ImageSize, int _ImageStyle)
{
	MAUTOSTRIP(CMWnd_Edit_Create, MAUTOSTRIP_VOID);
	m_Text = _Text;
	m_TextStyle = _TextStyle;
	m_Image = _Image;
	m_ImagePos = _ImagePos;
	m_ImageSize = _ImageSize;
	m_ImageStyle = _ImageStyle;
	CMWnd::Create(_pWndParent, _Param, _ID);
}

CMWnd_Edit::CMWnd_Edit()
{
	MAUTOSTRIP(CMWnd_Edit_ctor, MAUTOSTRIP_VOID);
	Clear();
}

CMWnd_Edit::CMWnd_Edit(CMWnd* _pWndParent, const CMWnd_Param& _Param, CStr _ID, CStr _Text,
	int _TextStyle, CStr _Image, CPnt _ImagePos, CPnt _ImageSize, int _ImageStyle)
{
	MAUTOSTRIP(CMWnd_Edit_ctor, MAUTOSTRIP_VOID);
	Clear();
	Create(_pWndParent, _Param, _ID, _Text, _TextStyle, _Image, _ImagePos, _ImageSize, _ImageStyle);
}

void CMWnd_Edit::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	MAUTOSTRIP(CMWnd_Edit_OnPaint, MAUTOSTRIP_VOID);
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
	if ((char*)m_Text)
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
		Pos.x = (m_ImageStyle & WSTYLE_BTNIMG_CENTERX) ? ((m_Pos.GetWidth() - Size.x) / 2) : m_ImagePos.x;
		Pos.y = (m_ImageStyle & WSTYLE_BTNIMG_CENTERY) ? ((m_Pos.GetHeight() - Size.y) / 2) : m_ImagePos.y;
		Pos.x += bDown;
		Pos.y += bDown;

		_pRCUtil->ScaleSprite(_Clip, Pos, Size);
//		_pRCUtil->ScaleSprite(_Clip, CPnt(x0, y0), CPnt(x1-x0, y1-y0));
		_pRCUtil->SetTexture(0);
	}
}

aint CMWnd_Edit::OnMessage(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_Edit_OnMessage, 0);
	return CMWnd::OnMessage(_pMsg);
}

int IsControlCode(const char *_pStr, int _iPos);
int IsControlCode(const char *_pStr, int _iPos)
{
	MAUTOSTRIP(IsControlCode, 0);
	int CodeLen = 0;

	char moo = '§';	// GCC workaround
	if(_pStr[_iPos] == moo)
	{
		CodeLen = 1;
		switch(_pStr[_iPos + 1])
		{
			case 'd':
			case 'D':
				CodeLen = 2;

			case 'a':
			case 'A':
				CodeLen = 3;
				break;

			case 'z':
			case 'Z':
				CodeLen = 4;
				break;

			case 'c':
			case 'C':
			case 'x':
			case 'X':
			case 'y':
			case 'Y':
				CodeLen = 5;
		}
	}

	return CodeLen;
}

int CMWnd_Edit::ProcessTextKey(CScanKey _Key)
{
	MAUTOSTRIP(CMWnd_Edit_ProcessTextKey, 0);
	if (_Key.IsASCII())
	{
		if (m_CursorPos >= IsControlCode(m_Text, 0))
		{
			char c = _Key.GetASCII();
			m_Text = m_Text.Insert(m_CursorPos, CStr(c));
			m_CursorPos++;
		}
	}
	else
	{
		switch (_Key.GetKey16()) 
		{
		case SKEY_BACKSPACE : 
			{
				int min = IsControlCode(m_Text, 0);
				
				if (m_CursorPos > min) m_Text = m_Text.Del(m_CursorPos-1, 1);
					m_CursorPos = Max((int)0, (int)(m_CursorPos-1));

				break;
			};
		case SKEY_DELETE : 
			{
				if (m_CursorPos < m_Text.Len()) m_Text = m_Text.Del(m_CursorPos, 1);

				break;
			};
		case SKEY_CURSOR_LEFT : 
			{
				m_CursorPos = Max((int)IsControlCode(m_Text, 0), (int)(m_CursorPos-1));

				break;
			};
		case SKEY_CURSOR_RIGHT : 
			{
				m_CursorPos = Min((int)m_Text.Len(), (int)(m_CursorPos+1));

				break;
			};
		case SKEY_HOME : 
			{
				m_CursorPos = IsControlCode(m_Text, 0);

				break;
			};
		case SKEY_END : 
			{
				m_CursorPos = m_Text.Len();
				break;
			};
		default : return FALSE;
		};
	};
	return TRUE;
};
