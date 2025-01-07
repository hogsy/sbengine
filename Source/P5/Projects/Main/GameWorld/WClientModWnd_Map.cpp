#include "PCH.h"
#include "MWinCtrlMod.h"
#include "WClientModWnd_Map.h"



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Phobos: Added Mission Map implementation
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModMissionMap, CMWnd_ModMissionMapParent);


CMWnd_ModMissionMap::CMWnd_ModMissionMap()
{
	// Not much here yet...
	m_ScaleX = 0.01f;
	m_ScaleY = 0.01f;
	m_CenterX = 256.0f;
	m_CenterY = 256.0f;
	m_PointerRange = CPnt(4,4);
	m_AngleOffset = 0;

	return;
}

void CMWnd_ModMissionMap::EvaluateKey(CMWnd_Param* _pParam, CStr _Key, CStr _Value)
{
	if (_Key.CompareSubStr("CENTER") == 0)
	{
		m_CenterX = _Value.GetStrSep(",").Val_fp64();
		m_CenterY = _Value.GetStrSep(",").Val_fp64();

		return;
	}
	else if (_Key.CompareSubStr("SCALE") == 0)
	{
		m_ScaleX = _Value.GetStrSep(",").Val_fp64();
		m_ScaleY = _Value.GetStrSep(",").Val_fp64();

		return;
	}
	else if (_Key.CompareSubStr("POINTER_RGN") == 0)
	{
		m_PointerRange.x = _Value.GetStrSep(",").Val_int();
		m_PointerRange.y = _Value.GetStrSep(",").Val_int();
		return;
	}
	else if (_Key.CompareSubStr("POINTER_ANGLEOFFSET") == 0)
	{
		m_AngleOffset = _Value.Val_fp64();
		return;
	}
	else if (_Key.CompareSubStr("POINTER") == 0)
	{
		m_PointerName = _Value;
		return;
	}
	


	CMWnd_ModMissionMapParent::EvaluateKey(_pParam, _Key, _Value);
}

aint CMWnd_ModMissionMap::OnMessage(const CMWnd_Message* _pMsg)
{
	switch(_pMsg->m_Msg)
	{
	case WMSG_KEY :
		{
			CScanKey Key;
			Key.m_ScanKey32 = _pMsg->m_Param0;
			CScanKey OriginalKey;
			OriginalKey.m_ScanKey32 = _pMsg->m_Param2;
			OriginalKey.m_Char = _pMsg->m_Param3;
			
			if (Key.IsDown())
			{
				// FIXME, WHICH BUTTONS SHOULD IT REACT TO
				if (Key.GetKey9() == SKEY_GUI_OK || Key.GetKey9() == SKEY_GUI_START ||
					Key.GetKey9() == SKEY_A)
				{
					// Should close down this window
					
					MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
					if(!pCon)
						return 0;

					pCon->ExecuteString("cmd(closeclientwindow)");
					//ConOut("Destroyed Map window");
					
					return true;
				}
			}
		}
	}
	
	return CMWnd_ModMissionMapParent::OnMessage(_pMsg);
}

void CMWnd_ModMissionMap::PaintTexture(CRC_Util2D* _pRCUtil, CClipRect _Clip, CRct _Text)
{
	CRenderContext* pRC = _pRCUtil->GetRC();
	CXR_VBManager* pVBM = _pRCUtil->GetVBM();
	
	CRct Rect = pVBM->Viewport_Get()->GetViewRect();
	int VPWidth = Rect.GetWidth();
	int VPHeight = Rect.GetHeight();
	_pRCUtil->SetCoordinateScale(CVec2Dfp32(VPWidth / 640.0f , VPHeight / 480.0f));
	
	_pRCUtil->GetAttrib()->Attrib_ZCompare(CRC_COMPARE_ALWAYS);
	_pRCUtil->GetAttrib()->Attrib_Enable(CRC_FLAGS_ZCOMPARE);
	_pRCUtil->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

	CClipRect Clip(0, 0, VPWidth, VPHeight);

	// Paint dark background
//	_pRCUtil->Rect(Clip, CRct(0,0,640,480),CPixel32(0,0,0,100));
	
	fp32 TexScaleX = 1.0f;
	fp32 TexScaleY = 1.0f;
	if(_pRCUtil->SetTexture(m_TextureName))
	{
		CRct Position = GetPosition();
		int Width = Position.p1.x - Position.p0.x;
		int Height = Position.p1.y - Position.p0.y;

		TexScaleX = _pRCUtil->GetTextureWidth() / (fp32) Width;
		TexScaleY = _pRCUtil->GetTextureHeight() / (fp32) Height;

		_pRCUtil->SetTextureScale(TexScaleX, TexScaleY);
		_pRCUtil->SetTextureOrigo(Clip, CPnt::From_fp32(Position.p0.x + m_Offset.k[0], Position.p0.y + m_Offset.k[1]));
		
		_pRCUtil->Rect(Clip, CRct::From_fp32(Position.p0.x + m_Offset.k[0], Position.p0.y + m_Offset.k[1], Position.p1.x + m_Offset.k[0], Position.p1.y + m_Offset.k[1]),  0xff7f7f7f);
	}

	if (_pRCUtil->SetTexture(m_PointerName))
	{
		// Paint the players position as a small rectangle "blob"
		int iObject = m_ClientInfo.m_pClient->Player_GetLocalObject();
		CVec3Dfp32 Position = m_ClientInfo.m_pClient->Object_GetPosition(iObject);
		CMat4Dfp32 PosMat = m_ClientInfo.m_pClient->Object_GetPositionMatrix(iObject);
		CVec3Dfp32 Direction = CVec3Dfp32::GetMatrixRow(PosMat,0);
		fp32 Angle = m_AngleOffset - CVec3Dfp32::AngleFromVector(Direction[0], Direction[1]);

		int PosX = (int)((1.0f/TexScaleX) * (m_CenterX + Position.k[0] * m_ScaleX) + m_Offset.k[0]);
		int PosY = (int)((1.0f/TexScaleY) * (m_CenterY + Position.k[1] * m_ScaleY) + m_Offset.k[1]);

		pRC->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
		pRC->Attrib_RasterMode(CRC_RASTERMODE_ONE_ALPHA);
		pRC->Attrib_ZCompare(CRC_COMPARE_NEVER);
		
		int Width = m_PointerRange.x;
		int Height = m_PointerRange.y;
		int HalfWidth = Width / 2;
		int HalfHeight = Height / 2;

		_pRCUtil->SetTextureScale(_pRCUtil->GetTextureWidth() / (fp32)Width, _pRCUtil->GetTextureHeight() / (fp32)Height);
		_pRCUtil->SetTextureOrigo(Clip, CPnt(PosX - HalfWidth, PosY - HalfHeight));
		// Draw rotated sprite...
		_pRCUtil->RotatedSprite(Clip, CPnt(PosX,PosY), m_PointerRange, Angle);
	}
}
