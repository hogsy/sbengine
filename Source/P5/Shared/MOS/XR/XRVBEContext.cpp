#include "PCH.h"
#include "XRVBEContext.h"

CXR_VBEContext::CXR_VBEContext()
{
	m_nScopes = 0;
	m_bInputEnable = 1;
	m_bShowTextures = 1;
	m_bShowSelection = 1;
	m_bShowAsWire = 0;
	m_bShowWithClip = 0;
	m_bShowWithZCompare = 0;
	m_bShowGPUTime = 0;
	m_bShowVBChain = 0;
	m_bRenderToSelection = 0;
	m_bQuickStep = 0;
	m_iScope = 0;
	m_iVB = 0;
	m_pFont = NULL;
	m_pUtil2D = NULL;
	m_ScreenSize = CPnt(0, 0);
#ifdef PLATFORM_CONSOLE
	//JK-NOTE: This weird rect is chosen due to crappy TV-scaler clipping
	m_GUIRect = CRct(CPnt(40, 26), CPnt(978, 576));
#else
	m_GUIRect = CRct(CPnt(4, 50), CPnt(1280, 720));
#endif
	ClearVB();
}

void CXR_VBEContext::SetScreenSize(class CDisplayContext* _pDC)
{
	CPnt Size = _pDC->GetScreenSize();
	if(Size.x != m_ScreenSize.x || Size.y != m_ScreenSize.y)
	{
		m_ScreenSize = Size;
#ifdef PLATFORM_CONSOLE
		//JK-NOTE: This weird rect is chosen due to crappy TV-scaler clipping
		m_GUIRect = CRct(CPnt(40, 26), CPnt(Size.x - 40, Size.y - 26));
#else
		m_GUIRect = CRct(CPnt(4, 50), CPnt(Size.x, Size.y));
#endif
	}
}

void CXR_VBEContext::Step(int _nStep)
{
	int nVB = m_lScopeInfo[m_iScope].m_nVB;
	m_iVB = Max(0, Min(m_iVB+_nStep, nVB - 1));
}

bool CXR_VBEContext::ProcessKey(CScanKey _Key)
{
	if (_Key.m_ScanKey32 == SKEY_I + SKEY_MODIFIER_CONTROL)
	{
		m_bInputEnable ^= 1;
		return true;
	}

	if (m_nScopes == 0)
		return false;

	m_iScope = Max(0, Min(m_iScope, m_nScopes - 1));
	int nVB = m_lScopeInfo[m_iScope].m_nVB;
	m_iVB = Max(0, Min(m_iVB, nVB - 1));

	if (m_bInputEnable)
	{
		M_TRACEALWAYS("ScanKey: %.8x\n", _Key.m_ScanKey32);

		switch(_Key.m_ScanKey32)
		{
		case SKEY_JOY_POV03 :
		case SKEY_JOY_POV03 | SKEY_REPEAT :
		case SKEY_CURSOR_LEFT :
			{
				if (m_bQuickStep)
					Step(-10);
				else
					Step(-1);
				break;
			};
		case SKEY_JOY_POV01 :
		case SKEY_JOY_POV01 | SKEY_REPEAT :
		case SKEY_CURSOR_RIGHT :
			{
				if (m_bQuickStep)
					Step(10);
				else
					Step(1);
				break;
			};
//			case SKEY_JOY0_AXIS02
		case SKEY_CURSOR_LEFT+SKEY_MODIFIER_CONTROL :
			{
				m_iVB = Max(0, Min(m_iVB-10, nVB - 1));
				break;
			};
		case SKEY_CURSOR_RIGHT+SKEY_MODIFIER_CONTROL :
			{
				m_iVB = Max(0, Min(m_iVB+10, nVB - 1));
				break;
			};

		case SKEY_CURSOR_LEFT+SKEY_MODIFIER_SHIFT :
			{
				int Diff = Max(0, m_GUIRect.p0.x - 1) - m_GUIRect.p0.x;
				m_GUIRect.p0.x += Diff;
				m_GUIRect.p1.x += Diff;
				break;
			};
		case SKEY_CURSOR_RIGHT+SKEY_MODIFIER_SHIFT :
			{
				int Diff = (m_GUIRect.p0.x + 1) - m_GUIRect.p0.x;
				m_GUIRect.p0.x += Diff;
				m_GUIRect.p1.x += Diff;
				break;
			};
		case SKEY_CURSOR_UP+SKEY_MODIFIER_SHIFT :
			{
				int Diff = Max(0, m_GUIRect.p0.y - 1) - m_GUIRect.p0.y;
				m_GUIRect.p0.y += Diff;
				m_GUIRect.p1.y += Diff;
				break;
			};
		case SKEY_CURSOR_DOWN+SKEY_MODIFIER_SHIFT :
			{
				int Diff = (m_GUIRect.p0.y + 1) - m_GUIRect.p0.y;
				m_GUIRect.p0.y += Diff;
				m_GUIRect.p1.y += Diff;
				break;
			};
		case SKEY_CURSOR_LEFT+SKEY_MODIFIER_SHIFT+SKEY_MODIFIER_CONTROL :
			{
				m_GUIRect.p1.x = Max(m_GUIRect.p0.x + 1, m_GUIRect.p1.x - 1);
				break;
			};
		case SKEY_CURSOR_RIGHT+SKEY_MODIFIER_SHIFT+SKEY_MODIFIER_CONTROL :
			{
				m_GUIRect.p1.x++;
				break;
			};
		case SKEY_CURSOR_UP+SKEY_MODIFIER_SHIFT+SKEY_MODIFIER_CONTROL :
			{
				m_GUIRect.p1.y = Max(m_GUIRect.p0.y + 1, m_GUIRect.p1.y - 1);
				break;
			};
		case SKEY_CURSOR_DOWN+SKEY_MODIFIER_SHIFT+SKEY_MODIFIER_CONTROL :
			{
				m_GUIRect.p1.y++;
				break;
			};

		case SKEY_JOY_POV00 :
		case SKEY_CURSOR_UP :
			{
				m_iVB -= 512;
				if (m_iVB < 0)
				{
					m_iScope = Max(0, Min(m_iScope-1, m_nScopes - 1));
					m_iVB += 512+(m_lScopeInfo[m_iScope].m_nVB & ~511);
				}
				m_iVB = Max(0, Min(m_iVB-1, m_lScopeInfo[m_iScope].m_nVB - 1));
				break;
			};
		case SKEY_JOY_POV02 :
		case SKEY_CURSOR_DOWN :
			{
				m_iVB += 512;
				if (m_iVB >= m_lScopeInfo[m_iScope].m_nVB)
				{
					m_iScope = Max(0, Min(m_iScope+1, m_nScopes - 1));
					m_iVB &= 511;
				}
				m_iVB = Max(0, Min(m_iVB-1, m_lScopeInfo[m_iScope].m_nVB - 1));
				break;
			};
		case SKEY_T + SKEY_MODIFIER_CONTROL :
			{
				m_bShowTextures ^= 1;
				return true;
			}
		case SKEY_S + SKEY_MODIFIER_CONTROL :
			{
				m_bShowSelection ^= 1;
				return true;
			}
		case SKEY_W + SKEY_MODIFIER_CONTROL :
			{
				m_bShowAsWire ^= 1;
				return true;
			}
		case SKEY_C + SKEY_MODIFIER_CONTROL :
			{
				m_bShowWithClip ^= 1;
				return true;
			}
		case SKEY_JOY_BUTTON0B :	// Right lower shoulder button
		case SKEY_Z + SKEY_MODIFIER_CONTROL :
			{
				m_bShowWithZCompare = (m_bShowWithZCompare+1) % 3;
				return true;
			}
		case SKEY_G + SKEY_MODIFIER_CONTROL :
			{
				m_bShowGPUTime ^= 1;
				return true;
			}
		case SKEY_V + SKEY_MODIFIER_CONTROL :
			{
				m_bShowVBChain ^= 1;
				return true;
			}
		case SKEY_JOY_BUTTON08 :	// Left upper shoulder button
		case SKEY_R + SKEY_MODIFIER_CONTROL :
			{
				m_bRenderToSelection ^= 1;
				return true;
			}
		case SKEY_JOY_BUTTON09 :	// Right upper shoulder button
			{
				m_bQuickStep = 1;
				return true;
			}
		case SKEY_JOY_BUTTON09 | SKEY_UP :	// Release right upper shoulder button
			{
				m_bQuickStep = 0;
				return true;
			}

		default : 
			return false;
		};
		return true;
	}
	else
		return false;
};



