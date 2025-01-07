#include "PCH.h"

#include "WFrontEndMod.h"
#include "../GameClasses/WObj_Game/WObj_GameMod.h"
#include "../../Shared/MOS/Classes/Win/MWinCtrlGfx.h"
#include "../../Shared/MOS/XR/XREngineVar.h"
#include "../../Shared/MOS/XR/XREngineImp.h"
#include "../../Shared/MOS/MSystem/Raster/MRCCore.h"
#include "../../Shared/MOS/Classes/GameContext/WGameContext.h"
#include "../../Shared/MOS/Classes/GameWorld/Server/WServer_Core.h"
#include "../../Projects/Main/GameWorld/WServerMod.h"
#include "../Exe/WGameContextMain.h"
#include "MFloat.h"

// for invetory display
#include "../GameClasses/WObj_Char.h"
#include "../GameClasses/WObj_CharClientData.h"
#include "../GameClasses/WObj_Misc/WObj_ActionCutscene.h" // just for a define

#include "../Exe_Xenon/Darkness.spa.h"

//#define DEBUG_ATTRACT
#define SCROLLSTEPTIME 0.2f
#define NUMBEROFROWSINSCROLLIST 7
#define NUMBEROFROWSABOVEINSCROLLIST 3
#define NUMBEROFROWSABELLOWINSCROLLIST 3
#define SPACEBETWEENSCROLLBUTTONANDTEXT 5

// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModMenu, CMWnd_ModTexture);

//
//
//
aint CMWnd_ModMenu::OnMessage(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_ModMenu_OnMessage, 0);
	switch(_pMsg->m_Msg)
	{
	case WMSG_GETMAPDATA : 
		return (aint)(CMapData*) ((m_FrontEndInfo.m_pFrontEnd) ? m_FrontEndInfo.m_pFrontEnd->m_spMapData : (TPtr<CMapData>)NULL);

	case WMSG_KEY:
		if(m_FrontEndInfo.m_pFrontEnd && !m_FrontEndInfo.m_pFrontEnd->m_spWData->Resource_AsyncCacheCategoryDone(0))
			return 0;

	default:
		return CMWnd_ModTexture::OnMessage(_pMsg);
	}
}


void CMWnd_ModMenu::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if(_Key == "SHOW_PRESSSTART")
	{
		m_ShowPressStart = EPos_Bottom;

		if(_Value == "top")
			m_ShowPressStart = EPos_Top;
	}
	else if(_Key == "SHOW_LOGO")
		m_ShowLogo = true;
	else
		CMWnd_ModTexture::EvaluateKey(_pParam, _Key, _Value);
}


void CMWnd_ModMenu::PaintPressStart(CMWnd *_pWnd, int32 pos, CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	//	return;

	CRC_Font *pFont = _pWnd->GetFont("HEADINGS");
	if(!pFont)
		return;

	int TextColorM = 0xffdddddd;
	int TextColorH = 0x60ffffff;
	int TextColorD = 0x60000000;

	CMTime Time = CMTime::GetCPU();
	if(Time.GetTimeModulusScaled(2.0f, 2.0f) > 1.0f)
	{
		int Style = WSTYLE_TEXT_WORDWRAP|WSTYLE_TEXT_CENTER;

		int32 y = 480-80;
		if(pos == EPos_Top)
			y = 40;

		CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pFont, "§LMENU_PRESSSTART", 0, y, Style, TextColorM, TextColorH, TextColorD,
			TruncToInt(_Clip.GetWidth()*0.95f), _Clip.GetHeight());
	}
}

void CMWnd_ModMenu::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	CMWnd_ModTexture::OnPaint(_pRCUtil, _Clip, _Client);
	if(m_ShowPressStart != 0)
		PaintPressStart(this, m_ShowPressStart, _pRCUtil, _Clip, _Client);

	if(m_ShowLogo)
	{
		fp32 scale = 2.5f;
		int32 w = int32(512/scale);
		int32 h = int32(256/scale);

		int32 x = 640-w-50;
		int32 y = 40;

		_pRCUtil->SetTexture(_pRCUtil->GetTC()->GetTextureID("RiddickLogo01"));

		_pRCUtil->SetTextureScale(scale,scale);
		_pRCUtil->SetTextureOrigo(_Clip, CPnt(x, y));
		//_pRCUtil->SetTextureOrigo(_Clip, CPnt(AIconPos.x, AIconPos.y+pFocusWnd->m_iButtonDescriptorVCorrection));
		_pRCUtil->Rect(_Clip, CRct(x, y, x+w, y+h), 0xff808080);

	}
}


// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_Timed, CMWnd_ModMenu);

CMWnd_Timed::CMWnd_Timed()
{
	m_Time = 3;
}

void CMWnd_Timed::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if(_Key == "TIME")
		m_Time = _Value.Val_fp64();
	else if(_Key == "SCRIPT_TIMEOUT")
		m_NextMenu = _Value;
	else
		CMWnd_ModMenu::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_Timed::OnRefresh()
{
	if((CMTime::GetCPU()-m_StartTime).GetTime() > m_Time)
	{
		M_TRACEALWAYS("Executing GUI on timeout: %s\n", m_NextMenu.Str());
		ConExecuteImp(m_NextMenu, __FUNCTION__, 0);
	}
}

void CMWnd_Timed::OnCreate()
{
	m_StartTime = CMTime::GetCPU();
	CMWnd_ModMenu::OnCreate();
}

// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_Random, CMWnd_ModMenu);

void CMWnd_Random::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if(_Key == "SCRIPT_RANDOM")
		m_lScripts.Add(_Value);
	else if(_Key == "RANDOMIZE")
	{
		int32 i = int32(m_lScripts.Len()*(Random));
		if(i >= m_lScripts.Len())
			i = m_lScripts.Len()-1;
		if(i < m_lScripts.Len())
			ConExecuteImp(m_lScripts[i], __FUNCTION__, 0);
	}
	else
		CMWnd_ModMenu::EvaluateKey(_pParam, _Key, _Value);
}


// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CenterImage, CMWnd_ModMenu);

CMWnd_CenterImage::CMWnd_CenterImage()
{
	m_ImageSize = CPnt(0, 0);
	m_bFitSafe = false;
}

//
//
//
void CMWnd_CenterImage::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if(_Key == "SAFEFIT")
		m_bFitSafe = _Value.Val_int() != 0;
	else if(_Key == "IMAGESIZE")
	{
		CStr Val = _Value;
		m_ImageSize.x = Val.GetStrSep(",").Val_int();
		m_ImageSize.y = Val.Val_int();
	}
	else
		CMWnd_ModMenu::EvaluateKey(_pParam, _Key, _Value);
}


//
//
//
void CMWnd_CenterImage::PaintTexture(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	CXR_VBManager* pVBM = _pRCUtil->GetVBM();

	CRct Rect = pVBM->Viewport_Get()->GetViewRect();
	int VPWidth = Rect.GetWidth();
	int VPHeight = Rect.GetHeight();
	_pRCUtil->SetCoordinateScale(CVec2Dfp32(VPWidth / 640.0f , VPHeight / 480.0f));

	_pRCUtil->GetAttrib()->Attrib_ZCompare(CRC_COMPARE_ALWAYS);
	_pRCUtil->GetAttrib()->Attrib_Enable(CRC_FLAGS_ZCOMPARE);
	_pRCUtil->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_NONE);

	CClipRect Clip(0, 0, 640, 480);

	_pRCUtil->SetTexture(NULL);
	_pRCUtil->Rect(Clip, CRct(0, 0, 640, 480), 0x0);

	if(_pRCUtil->SetTexture(m_TextureName))
	{
		CPnt SourceSize(_pRCUtil->GetTextureWidth(), _pRCUtil->GetTextureHeight());
		if(m_ImageSize.x != 0 || m_ImageSize.y != 0)
			SourceSize = m_ImageSize;

		_pRCUtil->AspectRect(Clip, GetPosition(), SourceSize, m_PixelAspect);
		/*		CRct Position = GetPosition();

		int Width = Position.p1.x - Position.p0.x;
		int Height = Position.p1.y - Position.p0.y;
		fp32 OffsetX = 0;
		fp32 OffsetY = 0;

		fp32 Scale = m_PixelAspect / (_pRCUtil->GetRC()->GetDC()->GetScreenAspect() * _pRCUtil->GetRC()->GetDC()->GetPixelAspect());
		if (Scale < 1.0)
		{
		OffsetX = (Width - ((Scale) * Width))/2;
		_pRCUtil->SetTextureScale((SourceSize.x / (fp32) Width) * (1.0f / Scale), (SourceSize.y / (fp32) Height));
		}
		else
		{
		OffsetY = (Height - ((1.0f / Scale) * Height))/2;
		_pRCUtil->SetTextureScale((SourceSize.x / (fp32) Width), (SourceSize.y / (fp32) Height) * (Scale));
		}

		_pRCUtil->SetTextureOrigo(Clip, CPnt(Position.p0.x + m_Offset.k[0] + OffsetX, Position.p0.y + m_Offset.k[1] + OffsetY));

		_pRCUtil->Rect(Clip, CRct(Position.p0.x + m_Offset.k[0] + OffsetX, Position.p0.y + m_Offset.k[1] + OffsetY, Position.p1.x + m_Offset.k[0] - OffsetX, Position.p1.y + m_Offset.k[1] - OffsetY),  0x80808080);*/
	}

	/*	CRenderContext* pRC = _pRCUtil->GetRC();

	// 853x480 wide
	// 640x480 normal

	// 1024x512 texture
	fp32 Scale = 1;


	int32 XOffset = -(m_ImageSizeX-640)/2;
	int32 YOffset = -(m_ImageSizeY-480)/2;

	if(m_bFitSafe)
	{
	Scale = (m_ImageSizeX)/(fp32)(640*0.85f);
	XOffset = (640-(m_ImageSizeX*(1/Scale)))/2;
	YOffset = (480-(m_ImageSizeY*(1/Scale)))/2;
	//m_ImageSizeX = 640;
	}


	CRct Rect = pRC->Viewport_Get()->GetViewRect();
	int VPWidth = Rect.GetWidth();
	int VPHeight = Rect.GetHeight();
	_pRCUtil->SetCoordinateScale(CVec2Dfp32(VPWidth / 640.0f , VPHeight / 480.0f));

	pRC->Attrib_Push();
	pRC->Attrib_ZCompare(CRC_COMPARE_ALWAYS);
	pRC->Attrib_Enable(CRC_FLAGS_ZCOMPARE);

	CClipRect Clip(0, 0, VPWidth, VPHeight);

	if(_pRCUtil->SetTexture(m_TextureName))
	{
	CRct Position = GetPosition();
	int Width = Position.p1.x - Position.p0.x;
	int Height = Position.p1.y - Position.p0.y;

	_pRCUtil->SetTextureScale(Scale, Scale);
	_pRCUtil->SetTextureOrigo(Clip, CPnt(XOffset, YOffset));

	_pRCUtil->Rect(Clip, CRct(0, 0, 640, 480),  0x007f7f7f|(Alpha<<24)); // fix
	}

	pRC->Attrib_Pop();*/
}


// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu, CMWnd_CubeLayout);

CMWnd_CubeMenu::CMWnd_CubeMenu()
{
	SetPosition(0,0,640,480);

	//m_AttractTime = CMTime::GetCPU();
	//m_CanActivateAttract = false;
	m_ShowPressStart = 0;
	m_UGLYDEMOHACK = false;
	m_InstantShow = false;
	m_Time = 0;
	m_StartTime.Reset();

	// get context
	//CWFrontEnd_Mod *pFrontEnd = m_CubeUser.GetFrontEnd();
	//if(m_FrontEndInfo.m_pFrontEnd != NULL && pFrontEnd->m_pGameContext == NULL)
	//	m_pGameContext = safe_cast<CGameContextMod>(pFrontEnd->m_pGameContext);
}

CMWnd_CubeMenu::~CMWnd_CubeMenu()
{
}

int CMWnd_CubeMenu::ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
{
	return CMWnd_CubeLayout::ProcessKey(_Key, _OriginalKey);
}

aint CMWnd_CubeMenu::OnMessage(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_ModMenu_OnMessage, 0);
	switch(_pMsg->m_Msg)
	{
	case WMSG_GETMAPDATA : 
		return (aint)(CMapData*) ((m_FrontEndInfo.m_pFrontEnd) ? m_FrontEndInfo.m_pFrontEnd->m_spMapData : (TPtr<CMapData>)NULL);
	case WMSG_KEY :
		{
			CScanKey Key;
			Key.m_ScanKey32 = _pMsg->m_Param0;
			CScanKey OriginalKey;
			OriginalKey.m_ScanKey32 = _pMsg->m_Param2;
			OriginalKey.m_Char = _pMsg->m_Param3;

			if (Key.IsDown())
			{
				if (Key.GetKey9() == SKEY_GUI_OK || Key.GetKey9() == SKEY_GUI_START)
				{
					CMWnd *pFocus = GetFocusWnd();
					if (OriginalKey.GetKey9() == SKEY_MOUSE1)
					{
						if (GetFrontEndInfo() && GetFrontEndInfo()->m_pFrontEnd)
						{
							CMWnd *pCursor = GetFrontEndInfo()->m_pFrontEnd->GetCursorWindow(false);
							if (!pCursor || ((((pCursor->m_Group.Len()||pFocus->m_Group.Len()) && pCursor->m_Group != pFocus->m_Group)) || (!(pCursor->m_Group.Len()||pFocus->m_Group.Len()) && pCursor != pFocus)))
								return CMWnd_CubeLayout::OnMessage(_pMsg);
						}
					}

					if(pFocus && OnButtonPress(pFocus))
						return true;
				}
				else
				{
					if(OnOtherButtonPress(GetFocusWnd(), Key.GetKey9()))
						return true;
				}
			}

			return CMWnd_CubeLayout::OnMessage(_pMsg);
		} break;
	default:
		return CMWnd_CubeLayout::OnMessage(_pMsg);
	}
}

void CMWnd_CubeMenu::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{

	// set template
	if(_Key == "TENTACLELAYOUT")
	{
		if(_Value == "TENTACLE_TEMPLATE_DEFAULT")
			m_iTentacleTemplate = CCube::TENTACLE_TEMPLATE_DEFAULT;
		else if(_Value == "TENTACLE_TEMPLATE_KNOT")
			m_iTentacleTemplate = CCube::TENTACLE_TEMPLATE_KNOT;
		else if(_Value == "TENTACLE_TEMPLATE_FULLWALL")
			m_iTentacleTemplate = CCube::TENTACLE_TEMPLATE_FULLWALL;
		else
			m_iTentacleTemplate = CCube::TENTACLE_TEMPLATE_DEFAULT;
	}

	if (_Key == "DISPLAYINFOFILE")
	{
		CCFile File;
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		CStr Path = (pSys ? pSys->m_ExePath : CStr("")) + _Value;
		if(CDiskUtil::FileExists(Path))
		{
			File.Open(Path, CFILE_BINARY | CFILE_READ);
			char Buf[1024];
			uint32 Len = MinMT(File.Length(), 1024);
			File.Read(Buf, Len);
			m_Info = "§Z14";
			for(int i = 0; i < Len; i++)
				if(Buf[i] == '\r')
				{}
				else if(Buf[i] == '\n')
					m_Info += "|";
				else if(Buf[i] >= 30)
					m_Info += CStrF("%c", Buf[i]);
				File.Close();
		}
	}
	else if(_Key == "INSTANTSHOW")
		m_InstantShow = true;
	else if(_Key == "UGLYDEMOHACK")
		m_UGLYDEMOHACK = true;
	else if(_Key == "SHOW_PRESSSTART")
	{
		m_ShowPressStart = CMWnd_ModMenu::EPos_Bottom;

		if(_Value == "top")
			m_ShowPressStart = CMWnd_ModMenu::EPos_Top;
	}
	else if(_Key == "TIME")
		m_Time = _Value.Val_fp64();
	else if(_Key == "SCRIPT_TIMEOUT")
		m_Script_Timeout = _Value;
	/*
	else if(_Key == "ATTRACT")
	{
	if(_Value.Len())
	m_CanActivateAttract = (_Value.Val_int() != 0);
	else
	m_CanActivateAttract = true;
	}*/
	else
		CMWnd_CubeLayout::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_CubeMenu::SetDynamicLocalKey(CStr _KeyName, CStr _Value)
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	CRegistry *pSt = pSys->GetRegistry()->FindChild("STRINGTABLES");
	CRegistry *pDynamicST = pSt ? pSt->FindChild("DYNAMIC") : NULL;
	if(pDynamicST)
		pDynamicST->SetValue(_KeyName, _Value);
}

void CMWnd_CubeMenu::OnRefresh()
{
	if(m_StartTime.IsReset())
		m_StartTime = CMTime::GetCPU();
	if(m_Script_Timeout.Len() && (CMTime::GetCPU()-m_StartTime).GetTime() > m_Time)
	{
		ConExecuteImp(m_Script_Timeout, __FUNCTION__, 0);
		m_Script_Timeout.Clear(); // prevent script from being executed again
	}

	CMWnd_CubeLayout::OnRefresh();
}

void CMWnd_CubeMenu::SoundEvent(uint _SoundEvent, const char* _pSoundName)
{
	switch (_SoundEvent)
	{
	case WSND_NAVIGATE:
		{
			m_CubeUser.m_pFrontEndMod->m_Cube.PlaySoundFXEvent(CCube::SOUNDEVENT_TENTACLES_HILITE_SWAP);
			break;
		}

	case WSND_TAB:
		{
			m_CubeUser.m_pFrontEndMod->m_Cube.PlaySoundFXEvent(CCube::SOUNDEVENT_TENTACLES_TAB);
			break;
		}

	default:
		CMWnd_CubeLayout::SoundEvent(_SoundEvent, _pSoundName);
		break;
	}
}

void CMWnd_CubeMenu::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{

	//	CMWnd_CubeLayout::OnPaint(_pRCUtil, _Clip, _Client);
	/*
	bool InGame = m_pGameContext->m_lspWClients.Len() > 0;

	if(InGame && m_spDesktop == NULL)
	return;
	*/

	//if(m_CanActivateAttract && (CMTime::GetCPU() - m_AttractTime).GetTime() > 15)
	//	ConExecute("cg_rootmenu(\"attract\")");

	/*
	if((CMTime::GetCPU() - m_AttractTime).GetTime() > 15)
	{
	bool Ingame = false;
	bool IsLoading = false;

	//if((m_spFrontEnd != NULL && m_spFrontEnd->m_spDesktop != NULL) || !bClientInteractive)

	CWorld_Client* pC = m_CubeUser.m_pGameContextMod->GetCurrentClient();
	//if((pC && pC->GetClientState() != WCLIENT_STATE_INGAME) || (m_spWServer != NULL && m_spWServer->IsLoading()))
	//	IsLoading = true;

	bool bClientInteractive = false;
	if(pC)
	bClientInteractive = pC->GetInteractiveState() != 0;

	if(!(pC && pC->GetClientState() == WCLIENT_STATE_INGAME) && !bClientInteractive)
	ConExecute("cg_rootmenu(\"attract\")");
	}*/

	bool CanShowText = false;

	m_CubeUser.m_pFrontEndMod->m_Cube.SetTentacleTemplate(m_iTentacleTemplate);	
	m_CubeUser.m_pFrontEndMod->Cube_Update(this);

	if(m_CubeUser.m_pFrontEndMod->Cube_ShouldRender())
	{
#if defined(PLATFORM_XBOX1) && !defined(M_DEMO_XBOX)
		{
			CRct Pos;
			CRct Pos2;
			Pos.p0.x = 0;
			Pos.p0.y = 0;
			Pos.p1.x = 19;
			Pos.p1.y = 1;
			Pos2 = Layout_CubeSpace2ViewSpace(Pos);

			Layout_Clear(Pos2, CCellInfo::FLAG_FASTSWITCH);

			if(m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetStatusText().Len())
			{
				CStr Text = Localize_Str(m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetStatusText());
				Layout_WriteText(Pos, 's', Text, FLAG_NO_CONVERT|FLAG_FASTSWITCH);
			}
		}
#endif
		// Do mapping
		if(m_CubeUser.m_pFrontEndMod->m_Cube.m_CanDoLayout)
			m_CubeUser.m_pFrontEndMod->Cube_SetStateFromLayout(this, m_InstantShow);
		m_CubeUser.m_pFrontEndMod->Cube_SetSecondaryRect(m_SecondaryRect);
		m_CubeUser.m_pFrontEndMod->Cube_SetSecondary(m_iSecondaryID, m_BlendMode);

		if(m_InstantShow)
			m_CubeUser.m_pFrontEndMod->m_Cube.m_MoveTime = 1;

		m_CubeUser.m_pFrontEndMod->Cube_Render(_pRCUtil->GetRC(), _pRCUtil->GetVBM(), false);


	}

	if(m_CubeUser.m_pFrontEndMod->m_Cube.m_BgItens > 0.9f)
		CanShowText = true;

	// press start and code/data info
	_pRCUtil->GetVBM()->ScopeBegin("CMWnd_CubeMenu::OnPaint", false);
	CRC_Font *pFont = GetFont("TEXT");
	if(pFont)
	{
		int TextColorM = 0xffdddddd;
		int TextColorH = 0x60ffffff;
		int TextColorD = 0x60000000;
		if(m_Info != "")
		{
			int Style = WSTYLE_TEXT_WORDWRAP;
			CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pFont, m_Info, 30, 400, Style, TextColorM, TextColorH, TextColorD, _Clip.GetWidth(), _Clip.GetHeight());
		}

		/*
		CMTime Time = CMTime::GetCPU();
		if(m_StartText != "" && (Time.GetTimeModulusScaled(2.0f, 2.0f) > 1.0f) && CanShowText)
		{
		int Style = WSTYLE_TEXT_WORDWRAP|WSTYLE_TEXT_CENTER;
		CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pFont, m_StartText, 0, 480-80, Style, TextColorM, TextColorH, TextColorD,
		_Clip.GetWidth()*0.95f, _Clip.GetHeight());
		}*/

		/*
		#if defined(PLATFORM_XBOX1) && !defined(M_DEMO_XBOX)
		if(m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetDownloadStatus() == CLiveHandler::STATUS_PROCESSING)
		{
		CStr Text = CStrF("%d", m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetDownloadProgress());
		int Style = WSTYLE_TEXT_WORDWRAP;
		CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pFont, m_StartText, 320, 240,
		Style, TextColorM, TextColorH, TextColorD,
		_Clip.GetWidth(), _Clip.GetHeight());
		}
		#endif
		*/
	}

	if(m_ShowPressStart != 0)
	{
		CMWnd_ModMenu::PaintPressStart(this, m_ShowPressStart, _pRCUtil, _Clip, _Client);		
	}
	_pRCUtil->GetVBM()->ScopeEnd();

	CMWnd_CubeLayout::OnPaint(_pRCUtil, _Clip, _Client);
}


//
//
//
void CMWnd_CubeMenu::SetMenu(CStr _ID)
{
	MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
	if(!pCon)
		return;
	Error("SetMenu", "Obsolete command");
}

//
//
//
void CMWnd_CubeMenu::SetMenu(CStr _ID, CStr _Transfer)
{
	m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = _Transfer;
	SetMenu(_ID);
}

//#define M_REGION_EUROPE
#define M_REGION_NORTHAMERICA

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Fork, CMWnd_CubeMenu);

void CMWnd_CubeMenu_Fork::EvaluateRegisty(CMWnd_Param* _pParam, CRegistry *_pReg)
{
	CMWnd_CubeMenu::EvaluateRegisty(_pParam, _pReg);

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(!pSys || !m_CubeUser.m_pGameContextMod || !pSys->GetOptions())
		return;

	bool InGame = m_CubeUser.m_pGameContextMod->m_lspWClients.Len() > 0;
	int iConfig = pSys->GetOptions()->GetValuei("CONTROLLER_TYPE", 0, 1);

	CStr Value;


	Value = _pReg->GetValue("GOTSAVES");
	if(Value != "")
	{
		TArray<CStr> lSaveFiles;
		m_CubeUser.m_pGameContextMod->EnumSaveFiles("", lSaveFiles);
		if(lSaveFiles.Len() > 1) // one is the profile
		{
			ConExecuteImp(Value, __FUNCTION__, 0);
			return ;
		}
	}

	Value = _pReg->GetValue("CONTROLLER_A");
	if(Value != "" && iConfig == 0)
	{
		ConExecuteImp(Value, __FUNCTION__, 0);
		return;
	}

	Value = _pReg->GetValue("CONTROLLER_B");
	if(Value != "" && iConfig == 1)
	{
		ConExecuteImp(Value, __FUNCTION__, 0);
		return;
	}

	Value = _pReg->GetValue("PROFILESMAXEDOUT");
	if(Value != "")
	{
		TArray<CStr> lProfiles;
		m_CubeUser.m_pGameContextMod->EnumProfiles(lProfiles);
		if(lProfiles.Len() >= 10)
		{
			ConExecuteImp(Value, __FUNCTION__, 0);
			return;
		}
	}

#ifdef PLATFORM_XENON
	Value = _pReg->GetValue("XENON");
	if(Value != "") // SAVEGAMEMAXSIZE
	{
		ConExecuteImp(Value, __FUNCTION__, 0);
		return;
	}
#endif

#ifdef PLATFORM_PS3
	Value = _pReg->GetValue("PS3");
	if(Value != "") // SAVEGAMEMAXSIZE
	{
		ConExecuteImp(Value, __FUNCTION__, 0);
		return;
	}
#endif

#ifdef PLATFORM_WIN
	Value = _pReg->GetValue("WINDOWS");
	if(Value != "") // SAVEGAMEMAXSIZE
	{
		ConExecuteImp(Value, __FUNCTION__, 0);
		return;
	}
#endif

#ifdef PLATFORM_CONSOLE
	Value = _pReg->GetValue("CONSOLE");
	if(Value != "") // SAVEGAMEMAXSIZE
	{
		ConExecuteImp(Value, __FUNCTION__, 0);
		return;
	}
#endif

#ifdef PLATFORM_XBOX1
	Value = _pReg->GetValue("FREESPACE");
	if(Value != "") // SAVEGAMEMAXSIZE
	{
		ULARGE_INTEGER FreeToCaller, TotalBytes, TotalFreeBytes;
		GetDiskFreeSpaceEx("U:\\", &FreeToCaller, &TotalBytes, &TotalFreeBytes);

		int32 Blocks = FreeToCaller.QuadPart/(16*1024);
		if(Blocks < 100) // SAVEGAMEMAXSIZE
		{
			SetDynamicLocalKey("GAME_BLOCKSNEEDED", CStrF("%d", 100-Blocks)); // SAVEGAMEMAXSIZE
			ConExecuteImp(Value);
			return;
		}
	}

	Value = _pReg->GetValue("PAL");
	if(Value != "" && XGetVideoStandard() == XC_VIDEO_STANDARD_PAL_I)
	{
		ConExecuteImp(Value);
		return;
	}

	Value = _pReg->GetValue("NTSC");
	if(Value != "" && XGetVideoStandard() != XC_VIDEO_STANDARD_PAL_I)
	{
		ConExecuteImp(Value);
		return;
	}

#endif
#ifdef M_DEMO
	Value = _pReg->GetValue("DEMO");
	if(Value != "")
	{
		ConExecuteImp(Value);
		return;
	}
#ifdef M_DEMO_XBOX
	Value = _pReg->GetValue("LAUNCHED_FROM_LAUNCHER");
	if(Value != "" && g_XboxDemoParams.m_bLaunchedFromDemoLauncher)
	{
		ConExecuteImp(Value);
		return;
	}
	Value = _pReg->GetValue("KIOSK");
	if(Value != "")
	{
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if(pSys && pSys->GetEnvironment()->GetValuei("KIOSKMODE", 0))
		{
			ConExecuteImp(Value);
			return;
		}
	}
#endif
#else
	Value = _pReg->GetValue("REAL");
	if(Value != "")
	{
		ConExecuteImp(Value, __FUNCTION__, 0);
		return;
	}
#endif
#ifdef M_REGION_EUROPE
	Value = _pReg->GetValue("REGION_EUROPE");
	if(Value != "")
	{
		ConExecuteImp(Value);
		return;
	}
#endif
#ifdef M_REGION_NORTHAMERICA
	Value = _pReg->GetValue("REGION_NORTHAMERICA");
	if(Value != "")
	{
		ConExecuteImp(Value, __FUNCTION__, 0);
		return;
	}
#endif

	Value = _pReg->GetValue("INGAME");
	if(Value != "" && InGame)
	{
		ConExecuteImp(Value, __FUNCTION__, 0);
		return;
	}

	Value = _pReg->GetValue("OUTGAME");
	if(Value != "" && !InGame)
	{
		ConExecuteImp(Value, __FUNCTION__, 0);
		return;
	}

	Value = _pReg->GetValue("DEFAULT");
	if(Value != "")
	{
		ConExecuteImp(Value, __FUNCTION__, 0);
		return;
	}

}

void CMWnd_CubeMenu_Fork::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
}


//
//
//
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Select_Profile, CMWnd_CubeMenu);

CMWnd_CubeMenu_Select_Profile::CMWnd_CubeMenu_Select_Profile()
{
	m_SelectAction = ACTION_USE;
}

void CMWnd_CubeMenu_Select_Profile::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if(_Key.UpperCase() == "SELECTACTION")
	{
		if(_Value.LowerCase() == "use")
			m_SelectAction = ACTION_USE;
		else if(_Value.LowerCase() == "delete")
			m_SelectAction = ACTION_DELETE;
	}
	else
		CMWnd_CubeMenu::EvaluateKey(_pParam, _Key, _Value);
}


//
//
//
int32 CMWnd_CubeMenu_Select_Profile::GetNumListItems(bool _Drawing)
{
	if(_Drawing)
	{
		// update list
		if(!m_lProfiles.Len())
		{
			CMWnd *pWnd = GetItem("CreateNew");
			if(pWnd)
				pWnd->SetFocus();
		}

		//m_aButtonDescriptions[2] = "";
		//m_iSelected = -1;
	}

	return m_lProfiles.Len();
}

//
//
//
bool CMWnd_CubeMenu_Select_Profile::GetListItem(int32 _Index, CStr &_Name, bool _Focus)
{
	if(_Index < 0 || _Index > m_lProfiles.Len())
		return false;

	if(_Focus)
	{
		m_iSelected = _Index;

		if(_Index != m_lProfiles.Len())
		{
			MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
			pSys->GetOptions()->SetValue("GAME_PROFILE", m_lProfiles[m_iSelected]);
		}

		/*
#ifdef PLATFORM_XBOX
		m_aButtonDescriptions[2] = "y, §LMENU_DELETE";
#endif
		*/
	}

	{
		wchar wText[1024];
		Localize_Str(CStrF("nc, %s", m_lProfiles[_Index].Ansi().Str()), wText, 1023);
		_Name = wText;
	}

	return true;
}

//
//
//
void CMWnd_CubeMenu_Select_Profile::OnActivate()
{
	// update list
	m_lProfiles.Clear();
	m_CubeUser.m_pGameContextMod->EnumProfiles(m_lProfiles);
}


//
//
//
void CMWnd_CubeMenu_Select_Profile::OnCreate()
{
	m_iSelected = -1;

	/*
	for(int32 i = 0; i < lSaves.Len(); i++)
	if(lSaves[i].Left(2) == "p_") // len("p_") = 8
	m_lProfiles.Add(lSaves[i].CopyFrom(2));
	*/

	// init stuff
	CMWnd_CubeMenu::OnCreate();
}

//
//
//
bool CMWnd_CubeMenu_Select_Profile::OnOtherButtonPress(CMWnd *_pFocus, int32 _Button)
{
	if(_Button == SKEY_GUI_BUTTON3 || _Button == SKEY_BACKSPACE || _Button == SKEY_DELETE)
	{
		if(m_iSelected != -1 && m_iSelected < m_lProfiles.Len())
		{
			SetDynamicLocalKey("GAME_CURRENTPROFILE", m_lProfiles[m_iSelected].Ansi());
			ConExecute("cg_submenu(\"profile_confirmdelete\")");
		}
		return true; // processed
	}
	return false; // not processed
}

//
//
//
bool CMWnd_CubeMenu_Select_Profile::OnButtonPress(CMWnd *_pFocus)
{
	if(_pFocus == GetItem("NEWPROFILE")) // create new
	{
		//ConExecute("cg_submenu(\"name_profile\")");
	}
	else if(_pFocus == GetItem("PROFILEDELETION")) // profile deletion
	{

	}
	else if(m_iSelected >= 0 && m_iSelected < m_lProfiles.Len())
	{
		if(m_SelectAction == ACTION_USE)
		{
			SetDynamicLocalKey("GAME_CURRENTPROFILE", m_lProfiles[m_iSelected]);

			if(m_CubeUser.m_pGameContextMod->ValidateProfile(m_lProfiles[m_iSelected]))
			{
				// OK!
				MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
				pSys->GetOptions()->SetNumChildren(0);
				m_CubeUser.m_pGameContextMod->ReadSaveFileToReg(m_lProfiles[m_iSelected], "_profile", pSys->GetOptions(1));
				ConExecute("option_update()");
				ConExecute("cg_rootmenu(\"MainMenu\")");

				pSys->GetOptions()->SetValue("GAME_PROFILE", m_lProfiles[m_iSelected]);
			}
			else
			{
				// Corrupt!
				ConExecute("cg_submenu(\"profile_confirmdelete_corrupt\")");
			}
		}
		else if(m_SelectAction == ACTION_DELETE)
		{
			SetDynamicLocalKey("GAME_CURRENTPROFILE", m_lProfiles[m_iSelected]);
			ConExecute("cg_switchmenu(\"profile_confirmdelete\")");
		}


		return true;
	}

	return false;
}


//
//
//
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_ButtonConfig, CMWnd_CubeMenu);

CMWnd_CubeMenu_ButtonConfig::CMWnd_CubeMenu_ButtonConfig()
{

}

int32 CMWnd_CubeMenu_ButtonConfig::GetNumListItems(bool _Drawing)
{
	return m_lActions.Len();
}

bool CMWnd_CubeMenu_ButtonConfig::GetListItem(int32 _Index, CStr &_Name, bool _Focus)
{
	M_ASSERT(m_lButtons.Len() == m_lActions.Len() && _Index < m_lActions.Len() && _Index >= 0, "");
	wchar aTemp[128];

	//
	Localize_Str(CStr("§LMENU_BINDING_")+m_lActions[_Index], aTemp, 127);
	CStr Text(aTemp);

	//for(int32 i = 0; i < 40-4; i++)
	//	aText[i] = L' ';

	CStr Button;
	if(_Focus && m_Binding)
	{
		Button = "§LMENU_PRESS_ANY_KEY_BIND";
		Button = Button.Unicode();
	}
	else
		Button = m_lButtons[_Index];

	// localize
	Localize_Str(Button, aTemp, 127);
	Button = aTemp;

	int SpaceAdd = (40-4) - (Text.Len()+Button.Len());

	if (SpaceAdd > 0)
	{
		CStr Add;
		char *pBuffer = Add.GetBuffer(SpaceAdd);
		while(SpaceAdd)
		{
			*pBuffer = ' ';
			++pBuffer;
			--SpaceAdd;
		}
		Text += Add;
	}


	_Name = (const wchar *)L"s,";
	_Name += Text;
	_Name += Button;

	return true;
}

bool CMWnd_CubeMenu_ButtonConfig::OnButtonPress(CMWnd *_pFocus)
{
	if(Layout_WndIsButton(_pFocus))
		m_Binding = 2; // delay 20 frames before accepting input to bind

	return CMWnd_CubeMenu::OnButtonPress(_pFocus);
}


bool CMWnd_CubeMenu_ButtonConfig::OnOtherButtonPress(CMWnd *_pFocus, int32 _Button)
{
	if((_Button == SKEY_BACKSPACE || _Button == SKEY_DELETE) && m_Binding == 0)
	{
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		M_ASSERT(pSys, "");
		pSys->GetOptions()->SetValuei(CStrF("ACTION_%s", m_lActions[m_iCurrentListItem].Str()), 0);
		FetchNames();
		ConExecute("option_update");
	}
	return CMWnd_CubeMenu::OnOtherButtonPress(_pFocus, _Button);
}

void CMWnd_CubeMenu_ButtonConfig::FetchNames()
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	M_ASSERT(pSys, "");

	m_lButtons.Clear();

	CRegistry *pOptions = pSys->GetOptions();
	M_ASSERT(pOptions, "");

	for(int32 n = 0; n < m_lActions.Len(); n++)
	{
		CStr Name;
		int32 Scan = pOptions->GetValuei(CStrF("ACTION_%s", m_lActions[n].Str()), 0);
		if (Scan == 0)
			Name = "";
		else
		{
			// Move this to CScanKey::GetLocalizedKeyName
			Name = CScanKey::GetLocalizedKeyName(Scan);
			if(Name.CompareSubStr("JOY_") == 0)
			{
				CStr St, St2;
				while(true)
				{
					St2 = Name.GetStrSep("_");
					if(Name == "")
						break;
					if(St != "")
						St += "_" + St2;
					else
						St += St2;
				}
				// If the zeros in the beginning of the string is not removed,
				// there is a risk that this will be interpretated as an octal value.
				while(St2[0] == '0')
					St2 = St2.Copy(1, 1024);
				Name = "§L" + St + CStrF(" %i", ("0x" + St2).Val_int());
			}
			else if(Name.CompareSubStr("MOUSE") == 0)
				Name = "§LMOUSE_BUTTON " + Name.Copy(5, 1024);

		}
		m_lButtons.Add(Name);
	}

	M_ASSERT(m_lButtons.Len() == m_lActions.Len(), "");

}


int CMWnd_CubeMenu_ButtonConfig::ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
//int CMWnd_CubeMenu_ButtonConfig::OnMessage(const CMWnd_Message* _pMsg)
{
	if(m_Binding < 0) // under 0 means that we have bound a key and should restrict input for a while
		return true;
	if(m_Binding == 1) // above 1 means that we are waiting to bind, so we don't get alot of strange input
	{

		/*
		if(_pMsg->m_Msg == WMSG_KEY)
		{
		CScanKey Key;
		Key.m_ScanKey32 = _pMsg->m_Param0;
		CScanKey OriginalKey;
		OriginalKey.m_ScanKey32 = _pMsg->m_Param2;
		OriginalKey.m_Char = _pMsg->m_Param3;
		char Ascii = _pMsg->m_Param1;
		*/
		if(_OriginalKey.IsDown() && !_OriginalKey.IsRepeat())
		{
			if(_OriginalKey.GetKey9() == SKEY_ESC)
			{
				m_Binding = -2;
				return 1;
			}

			if(_OriginalKey.GetKey9() == SKEY_F1)
				return 1;

			if(_OriginalKey.GetKey9() > SKEY_GUISTART && _OriginalKey.GetKey9() < SKEY_GUIEND)
				return 1;

			if(_OriginalKey.GetKey9() == SKEY_MOUSEMOVE || _OriginalKey.GetKey9() == SKEY_MOUSEMOVEREL)
				return 1;

			if(m_lType[m_iCurrentListItem] == 0 && (_OriginalKey.GetKey9() > SKEY_JOY_START && _OriginalKey.GetKey9() < SKEY_JOY_END))
				return 1;

			if(m_lType[m_iCurrentListItem] == 1 && (_OriginalKey.GetKey9() < SKEY_JOY_START || _OriginalKey.GetKey9() >= SKEY_JOY_END))
				return 1;

			CStr s = CScanKey::GetScanCodeName(_OriginalKey.GetKey9());
			if(s != "")
			{

				M_ASSERT(m_iCurrentListItem >= 0 && m_iCurrentListItem < m_lButtons.Len(), "");

				m_Binding = -2; // restrict input for a while

				m_lButtons[m_iCurrentListItem] = s;

				MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
				M_ASSERT(pSys, "");

				// remove duplicated keys
				CRegistry *pOptions = pSys->GetOptions();

				bool bIsGUIKey = m_lActions[m_iCurrentListItem].CompareSubStr("GUI") == 0;
				for(int32 i = 0; i < pOptions->GetNumChildren(); i++)
				{
					CRegistry *pChild = pOptions->GetChild(i);
					if(pChild->GetThisName().Left(7) == "ACTION_")
					{
						bool bIsGUIKey2 = pChild->GetThisName().CompareSubStr("ACTION_GUI") == 0;
						if(!(bIsGUIKey ^ bIsGUIKey2) && pChild->GetThisValuei() == _OriginalKey.GetKey9())
							pOptions->DeleteKey(i);
					}
				}

				// set bind
				pOptions->SetValuei(CStrF("ACTION_%s", m_lActions[m_iCurrentListItem].Str()), _OriginalKey.GetKey9());
				FetchNames();
				ConExecute("option_update()");
			}
			//_OriginalKey = _OriginalKey;
			// FIXME, WHICH BUTTONS SHOULD IT REACT TO
			/*
			if((Ascii >= 'a' && Ascii <= 'z') || (Ascii >= 'A' && Ascii <= 'Z'))
			{
			if(m_String.Len() < 8)
			m_String += CStr(Ascii);//_pFocus->m_ID.Right(1);
			return 1;
			}*/
		}
		//}

		return 1;
	}
	return CMWnd_CubeMenu::ProcessKey(_Key, _OriginalKey);
	//if(m_Binding)
	//	return false;
	//return CMWnd_CubeMenu::OnMessage(_pMsg);
}

void CMWnd_CubeMenu_ButtonConfig::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if(_Key == "BINDING")
	{
		CStr Value = _Value;
		m_lActions.Add(Value.GetStrSep(","));
		m_lType.Add(Value.Val_int());
		FetchNames();
	}


	CMWnd_CubeMenu::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_CubeMenu_ButtonConfig::OnCreate()
{
	m_Binding = 0;

	CMWnd_CubeMenu::OnCreate();
}

void CMWnd_CubeMenu_ButtonConfig::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	if(m_Binding > 1)
		m_Binding--;
	if(m_Binding < 0)
		m_Binding++;
	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);
}

// ---------------------------------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_VideoSelection, CMWnd_CubeMenu);

void CMWnd_CubeMenu_VideoSelection::FetchList(class CDisplayContext *pDC)
{
	if(m_lRes.Len())
		return;

	int32 NumModes = pDC->ModeList_GetNumModes();
	int32 w = 0;
	int32 h = 0;
	int32 bpp = 0;
	for(int32 i = 0; i < NumModes; i++)
	{
		const CDC_VideoMode &rVideoMode = pDC->ModeList_GetDesc(i);
		if(rVideoMode.m_Format.GetWidth() == w && rVideoMode.m_Format.GetHeight() == h &&
			bpp == rVideoMode.m_Format.GetPixelSize())
			continue;

		w = rVideoMode.m_Format.GetWidth();
		h = rVideoMode.m_Format.GetHeight();
		bpp = rVideoMode.m_Format.GetPixelSize();
		if(w >= 400 && h >= 300 && bpp >= 4)
		{
			m_lRes.Add(CPnt(w,h));

			if(pDC->GetScreenSize().x == w && pDC->GetScreenSize().y == h)
				m_CurrentRes = m_lRes.Len()-1;
		}
	}

	MACRO_GetSystem;

	pSys->GetRegistry()->DeleteDir("GUI\\VIDSEL\\RESOLUTIONLIST");
	CRegistry *pResList = pSys->GetRegistry()->CreateDir("GUI\\VIDSEL\\RESOLUTIONLIST");
	for(int32 i = 0; i < m_lRes.Len(); i++)
	{
		pResList->AddKeyi(CStrF("LIST%d", i), i);
	}
	M_ASSERT(pResList->GetNumChildren() == m_lRes.Len(), "");

	pSys->GetRegistry()->SetValuei("GUI\\VIDSEL\\FULLSCREEN", pDC->IsFullScreen());



	UpdateButtons();
}

CMWnd_CubeMenu_VideoSelection::~CMWnd_CubeMenu_VideoSelection()
{
	MACRO_GetSystem;
	pSys->GetRegistry()->DeleteDir("GUI\\VIDSEL");
}

void CMWnd_CubeMenu_VideoSelection::FetchHertz(class CDisplayContext *pDC, int32 w, int32 h, int32 bpp)
{
	int LastRefreshRate = 85;

	if (m_CurrentHertzInit >= 0)
	{
		LastRefreshRate = m_CurrentHertzInit;
		m_CurrentHertzInit = -1;
	}
	else if (m_lHertz.Len())
		LastRefreshRate = m_lHertz[m_CurrentHertz];

	m_lHertz.Clear();

	int32 NumModes = pDC->ModeList_GetNumModes();

	int iRefresh = -1;

	for(int32 i = 0; i < NumModes; i++)
	{
		const CDC_VideoMode &rVideoMode = pDC->ModeList_GetDesc(i);

		// match mode
		if(rVideoMode.m_Format.GetWidth() != w || rVideoMode.m_Format.GetHeight() != h ||
			bpp != rVideoMode.m_Format.GetPixelSize())
		{
			continue;
		}

		// no duplicates
		bool Found = false;
		for(int32 h = 0; h < m_lHertz.Len(); h++)
			if(m_lHertz[h] == rVideoMode.m_RefreshRate)
			{
				Found = true;
				break;
			}

			if(!Found)
			{
				int iAdd = m_lHertz.Add(rVideoMode.m_RefreshRate);

				if (rVideoMode.m_RefreshRate == LastRefreshRate)
					iRefresh = iAdd;			
			}
	}

	if (iRefresh >= 0)
		m_CurrentHertz = iRefresh;
	if(m_CurrentHertz >= m_lHertz.Len())
		m_CurrentHertz = m_lHertz.Len()-1;
	if (m_CurrentHertz < 0)
		m_CurrentHertz = 0;

	MACRO_GetSystem;

	pSys->GetRegistry()->DeleteDir("GUI\\VIDSEL\\REFRESHRATELIST");
	CRegistry *pResList = pSys->GetRegistry()->CreateDir("GUI\\VIDSEL\\REFRESHRATELIST");
	for(int32 i = 0; i < m_lHertz.Len(); i++)
	{
		pResList->AddKeyi(CStrF("LIST%d", i), i);
	}
	M_ASSERT(pResList->GetNumChildren() == m_lHertz.Len(), "");


	UpdateButtons();
}

void CMWnd_CubeMenu_VideoSelection::UpdateButtons()
{
	MACRO_GetSystem;

	if (m_lRes.Len() && m_CurrentRes >= 0 && m_CurrentRes < m_lRes.Len())
	{
		pSys->GetRegistry()->SetValuei("GUI\\VIDSEL\\RESOLUTION", m_CurrentRes);
		pSys->GetRegistry()->SetValue("GUI\\VIDSEL\\RESOLUTIONSTR", CStrF("%d x %d", m_lRes[m_CurrentRes].x, m_lRes[m_CurrentRes].y));
	}

	if (m_lHertz.Len() && m_CurrentHertz >= 0 && m_CurrentHertz < m_lHertz.Len())
	{
		pSys->GetRegistry()->SetValuei("GUI\\VIDSEL\\REFRESHRATE", m_CurrentHertz);
		pSys->GetRegistry()->SetValue("GUI\\VIDSEL\\REFRESHRATESTR", CStrF("%d", m_lHertz[m_CurrentHertz]));
	}

	int iPixelAspect = pSys->GetRegistry()->GetValuei("GUI\\VIDSEL\\PIXELASPECT", -1);
	if (iPixelAspect >= 0 && iPixelAspect < m_PixelAspects.Len())
	{
		pSys->GetRegistry()->SetValue("GUI\\VIDSEL\\PIXELASPECTSTR", m_PixelAspects[iPixelAspect].m_Desc);
	}
	else
	{
		pSys->GetRegistry()->SetValue("GUI\\VIDSEL\\PIXELASPECTSTR", "§LMENU_VIDEO_ASPECT_CUSTOM");		
	}


	int iAntiAlias = pSys->GetRegistry()->GetValuei("GUI\\VIDSEL\\ANTIALIAS", 1);
	pSys->GetRegistry()->SetValue("GUI\\VIDSEL\\ANTIALIASSTR", CStrF("%dx", iAntiAlias));

	uint32 HdrFormat = pSys->GetRegistry()->GetValuei("GUI\\VIDSEL\\HDR", IMAGE_FORMAT_RGBA8);
	switch(HdrFormat)
	{
	case IMAGE_FORMAT_RGBA8:
		pSys->GetRegistry()->SetValue("GUI\\VIDSEL\\HDRSTR", "8 bit int"); break;
	case IMAGE_FORMAT_RGBA16_F:
		pSys->GetRegistry()->SetValue("GUI\\VIDSEL\\HDRSTR", "16 bit float"); break;
	case IMAGE_FORMAT_RGBA32_F:
		pSys->GetRegistry()->SetValue("GUI\\VIDSEL\\HDRSTR", "32 bit float"); break;
	case IMAGE_FORMAT_RGB10A2_F:
		pSys->GetRegistry()->SetValue("GUI\\VIDSEL\\HDRSTR", "10 bit float"); break;
	case IMAGE_FORMAT_RGBA16:
		pSys->GetRegistry()->SetValue("GUI\\VIDSEL\\HDRSTR", "16 bit int"); break;
	case IMAGE_FORMAT_BGR10A2:
		pSys->GetRegistry()->SetValue("GUI\\VIDSEL\\HDRSTR", "10 bit int"); break;
	default:
		pSys->GetRegistry()->SetValue("GUI\\VIDSEL\\HDRSTR", "Unknown"); break;
	}


	/*
	CMWnd_CubeButton *pResButton = TDynamicCast<CMWnd_CubeButton>(this->FindItem("Res"));
	if(pResButton && m_lRes.Len())
	pResButton->m_Text = CStrF("nc, %d x %d    ", m_lRes[m_CurrentRes].x, m_lRes[m_CurrentRes].y);

	CMWnd_CubeButton *pHertzButton = TDynamicCast<CMWnd_CubeButton>(this->FindItem("Hertz"));
	if(pHertzButton && m_lHertz.Len())
	pHertzButton->m_Text = CStrF("nc, %d §LMENU_HERTZ", m_lHertz[m_CurrentHertz]);

	CMWnd_CubeButton *pFullscreenButton = TDynamicCast<CMWnd_CubeButton>(this->FindItem("Fullscreen"));
	if(pFullscreenButton)
	{
	if(m_Fullscreen)
	pFullscreenButton->m_Text = "nc, §LMENU_VIDEO_FULLSCREEN";
	else
	pFullscreenButton->m_Text = "nc, §LMENU_VIDEO_WINDOWED";
	}*/



}


CMWnd_CubeMenu_VideoSelection::CMWnd_CubeMenu_VideoSelection()
{
	m_CurrentRes = 0;
	m_CurrentHertz = 0;
	m_Apply = false;

	MACRO_GetSystem;

	pSys->GetRegistry()->SetValuei("GUI\\VIDSEL\\FULLSCREEN", pSys->GetRegistry()->GetValuei("OPTG\\VIDEO_DISPLAY_FULLSCREEN", 1));
	pSys->GetRegistry()->SetValuei("GUI\\VIDSEL\\VSYNC", pSys->GetRegistry()->GetValuei("OPTG\\VIDEO_DISPLAY_VSYNC", 1));
	fp32 PixelAspect = pSys->GetRegistry()->GetValuef("OPTG\\VIDEO_DISPLAY_PIXELASPECT", 1.0);
	m_CurrentHertzInit = pSys->GetRegistry()->GetValuei("OPTG\\VIDEO_DISPLAY_REFRESH", 85);

	m_PixelAspects.Add(CPixelAspect("§LMENU_VIDEO_ASPECT_NORMAL", 1));
	m_PixelAspects.Add(CPixelAspect("4:3->5:4",		(5.0f/4.0f)		/(4.0f/3.0f)));	
	m_PixelAspects.Add(CPixelAspect("4:3->16:9",	(16.0f/9.0f)	/(4.0f/3.0f)));	
	m_PixelAspects.Add(CPixelAspect("4:3->2.35",	2.35f			/(4.0f/3.0f)));	
	m_PixelAspects.Add(CPixelAspect("5:4->4:3",		(4.0f/3.0f)		/(5.0f/4.0f)));	
	m_PixelAspects.Add(CPixelAspect("5:4->16:9",	(16.0f/9.0f)	/(5.0f/4.0f)));	
	m_PixelAspects.Add(CPixelAspect("5:4->2.35",	(2.35f)			/(5.0f/4.0f)));	
	m_PixelAspects.Add(CPixelAspect("16:9->4:3",	(4.0f/3.0f)		/(16.0f/9.0f)));	
	m_PixelAspects.Add(CPixelAspect("16:9->5:4",	(5.0f/4.0f)		/(16.0f/9.0f)));	
	m_PixelAspects.Add(CPixelAspect("16:9->2.35",	(2.35f)			/(16.0f/9.0f)));	

	int iPixelAspect = -1;
	pSys->GetRegistry()->DeleteDir("GUI\\VIDSEL\\PIXELASPECTLIST");
	CRegistry *pList = pSys->GetRegistry()->CreateDir("GUI\\VIDSEL\\PIXELASPECTLIST");
	for(int32 i = 0; i < m_PixelAspects.Len(); i++)
	{
		if (M_Fabs(m_PixelAspects[i].m_Aspect - PixelAspect) < 0.0001f)
			iPixelAspect = i;
		pList->AddKeyf(CStrF("LIST%d", i), i);
	}
	pSys->GetRegistry()->SetValuei("GUI\\VIDSEL\\PIXELASPECT", iPixelAspect);

	{
		pSys->GetRegistry()->DeleteDir("GUI\\VIDSEL\\ANTIALIASLIST");
		CRegistry *pList = pSys->GetRegistry()->CreateDir("GUI\\VIDSEL\\ANTIALIASLIST");

#if defined(PLATFORM_PS3)
		// PS3 only supports 4x multisample
		pList->AddKeyf(CStrF("LIST%d", 0), 1);
		pList->AddKeyf(CStrF("LIST%d", 1), 4);
#elif defined(PLATFORM_XENON)
		pList->AddKeyf(CStrF("LIST%d", 0), 1);
		pList->AddKeyf(CStrF("LIST%d", 1), 2);
		pList->AddKeyf(CStrF("LIST%d", 2), 4);
#else
		pList->AddKeyf(CStrF("LIST%d", 0), 1);
#endif

		pSys->GetRegistry()->SetValuei("GUI\\VIDSEL\\ANTIALIAS", pSys->GetRegistry()->GetValuei("OPTG\\VIDEO_DISPLAY_ANTIALIAS", 1));
	}

	{
		pSys->GetRegistry()->DeleteDir("GUI\\VIDSEL\\HDRLIST");
		CRegistry *pList = pSys->GetRegistry()->CreateDir("GUI\\VIDSEL\\HDRLIST");

#if defined(PLATFORM_PS3)
		pList->AddKeyf(CStrF("LIST%d", 0), IMAGE_FORMAT_RGBA8);
		pList->AddKeyf(CStrF("LIST%d", 1), IMAGE_FORMAT_RGBA16_F); // Blending not supported on xenon
		pList->AddKeyf(CStrF("LIST%d", 2), IMAGE_FORMAT_RGBA32_F); // Blending not supported on xenon
#elif defined(PLATFORM_XENON)
		pList->AddKeyf(CStrF("LIST%d", 0), IMAGE_FORMAT_RGBA8);
		pList->AddKeyf(CStrF("LIST%d", 1), IMAGE_FORMAT_BGR10A2);		
		pList->AddKeyf(CStrF("LIST%d", 2), IMAGE_FORMAT_RGBA16);
		pList->AddKeyf(CStrF("LIST%d", 3), IMAGE_FORMAT_RGB10A2_F);
		//		pList->AddKeyf(CStrF("LIST%d", 4), IMAGE_FORMAT_RGBA16_F); // Blending not supported on xenon
		//		pList->AddKeyf(CStrF("LIST%d", 5), IMAGE_FORMAT_RGBA32_F); // Blending not supported on xenon
#else
		pList->AddKeyf(CStrF("LIST%d", 0), IMAGE_FORMAT_RGBA8);
#endif

		pSys->GetRegistry()->SetValuei("GUI\\VIDSEL\\HDR", pSys->GetRegistry()->GetValuei("OPTG\\VIDEO_DISPLAY_HDR", IMAGE_FORMAT_RGBA8));
	}

}

bool CMWnd_CubeMenu_VideoSelection::OnButtonPress(CMWnd *_pFocus)
{
	if(!_pFocus)
		return CMWnd_CubeMenu::OnButtonPress(_pFocus);

	if(_pFocus->m_ID.LowerCase() == "apply") // we must have the DC to find the correct mode.. do this in onpaint
		m_Apply = true;
	return CMWnd_CubeMenu::OnButtonPress(_pFocus);
}

void CMWnd_CubeMenu_VideoSelection::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	CMWnd_CubeMenu::EvaluateKey(_pParam, _Key, _Value);
}
void CMWnd_CubeMenu_VideoSelection::OnCreate()
{
	CMWnd_CubeMenu::OnCreate();
}


void CMWnd_CubeMenu_VideoSelection::OnRefresh()
{
	if(m_Apply && m_lRes.Len() > 0)
	{
		MACRO_GetSystem;
		CRegistry *pReg = pSys->GetRegistry();

		pReg = pReg->CreateDir("OPTG");

		pReg->SetValuei("VIDEO_DISPLAY_WIDTH", m_lRes[m_CurrentRes].x);
		pReg->SetValuei("VIDEO_DISPLAY_HEIGHT", m_lRes[m_CurrentRes].y);
		pReg->SetValuei("VIDEO_DISPLAY_REFRESH", m_lHertz[m_CurrentHertz]);
		pReg->SetValuei("VIDEO_DISPLAY_FULLSCREEN", pSys->GetRegistry()->GetValuei("GUI\\VIDSEL\\FULLSCREEN", 1));
		pReg->SetValuei("VIDEO_DISPLAY_VSYNC", pSys->GetRegistry()->GetValuei("GUI\\VIDSEL\\VSYNC", 1));
		int iPixelAspect = pSys->GetRegistry()->GetValuei("GUI\\VIDSEL\\PIXELASPECT", -1);
		if (iPixelAspect >= 0 && iPixelAspect < m_PixelAspects.Len())
		{
			pReg->SetValuef("VIDEO_DISPLAY_PIXELASPECT", m_PixelAspects[iPixelAspect].m_Aspect);
			pSys->GetRegistry()->SetValue("GUI\\VIDSEL\\PIXELASPECTSTR", m_PixelAspects[iPixelAspect].m_Desc);
		}

		pReg->SetValuei("VIDEO_DISPLAY_ANTIALIAS", pSys->GetRegistry()->GetValuei("GUI\\VIDSEL\\ANTIALIAS", 1));
		pReg->SetValuei("VIDEO_DISPLAY_HDR", pSys->GetRegistry()->GetValuei("GUI\\VIDSEL\\HDR", IMAGE_FORMAT_RGBA8));

		ConExecute(CStr("deferredscript(\"option_update()\")"));

		m_Apply = false;
	}

}

void CMWnd_CubeMenu_VideoSelection::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	MACRO_GetSystem;
	int iRes = pSys->GetRegistry()->GetValuei("GUI\\VIDSEL\\RESOLUTION");
	m_CurrentRes = Max(Min(iRes, m_lRes.Len()-1), 0); 

	int iRefresh = pSys->GetRegistry()->GetValuei("GUI\\VIDSEL\\REFRESHRATE");
	m_CurrentHertz = Max(Min(iRefresh, m_lHertz.Len()-1), 0); 

	bool SetHZ = m_lRes.Len() == 0;
	FetchList(_pRCUtil->GetRC()->GetDC());
	FetchHertz(_pRCUtil->GetRC()->GetDC(), m_lRes[m_CurrentRes].x, m_lRes[m_CurrentRes].y, 4);

	if(SetHZ)
	{
		const CDC_VideoMode &rMode = _pRCUtil->GetRC()->GetDC()->ModeList_GetDesc(_pRCUtil->GetRC()->GetDC()->GetMode());
		for(int32 i = 0; i < m_lHertz.Len(); i++)
			if(m_lHertz[i] == rMode.m_RefreshRate)
			{
				m_CurrentHertz = i;
				break;
			}
	}

	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);
}


// ---------------------------------------------------------------------------------------------
/*
class CMWnd_CubeMenu_TextInput : public CMWnd_CubeMenu
{
MRTC_DECLARE;
public:
CStr m_PreString;
CStr m_String;

CMWnd *m_pTextView;
*/
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_TextInput, CMWnd_CubeMenu);

void CMWnd_CubeMenu_TextInput::OnCreate()
{
	m_pTextView = GetItem("INPUTBOX");
	m_String = "";

	CRct Pos = m_pTextView->GetPosition();
	m_nMaxLen = uint(Pos.GetWidth() * (20/640.0f));
}

bool CMWnd_CubeMenu_TextInput::OnOtherButtonPress(CMWnd *_pFocus, int32 _Button)
{
	if(_Button == SKEY_GUI_BUTTON2 || _Button == SKEY_BACKSPACE)
	{
		if(m_String.Len() > 0)
			m_String = m_String.Left(m_String.Len()-1);
		return true;
	}
	return false;
}

bool CMWnd_CubeMenu_TextInput::OnButtonPress(CMWnd *_pFocus)
{
	if(_pFocus->m_ID.Find("KEY_") != -1)
	{
		if(m_String.Len() < m_nMaxLen)
			m_String += _pFocus->m_ID.Right(1);
		return true;
	}
	else
	{
		if(m_String.Len() != 0)
		{
			return OnTextInputDone();
		}
	}

	return CMWnd_CubeMenu::OnButtonPress(_pFocus);
}

bool CMWnd_CubeMenu_TextInput::OnTextInputDone()
{
	if(m_CubeUser.m_pGameContextMod->CreateProfile(m_String))
	{
		// OK!
		// set default
		ConExecute("option_setdefault()"); 


		// set current profile
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if(pSys->GetOptions())
		{
			pSys->GetOptions()->SetValue("GAME_EXTRACONTENTKEY", "0");
			pSys->GetOptions()->SetValue("GAME_PROFILE", m_String);
		}

		// save profile
		CStr Profile;
		m_CubeUser.m_pGameContextMod->FixProfile(Profile);
		m_CubeUser.m_pGameContextMod->m_bValidProfileLoaded	= true;
		m_CubeUser.m_pGameContextMod->m_SaveProfile = Profile;
		ConExecute("cg_rootmenu(\"MainMenu\")");
		return true;
	}
	else
	{
		// taken / corrupt?
		ConExecute("cg_submenu(\"profile_taken\")");
		return true;
	}
}

void CMWnd_CubeMenu_TextInput::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	return CMWnd_CubeMenu::EvaluateKey(_pParam, _Key, _Value);
}

int CMWnd_CubeMenu_TextInput::ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
{
	MAUTOSTRIP(CMWnd_ModMenu_OnMessage, 0);
	/*
	if(_pMsg->m_Msg == WMSG_KEY)
	{
	CScanKey Key;
	Key.m_ScanKey32 = _pMsg->m_Param0;
	CScanKey OriginalKey;
	OriginalKey.m_ScanKey32 = _pMsg->m_Param2;
	OriginalKey.m_Char = _pMsg->m_Param3;
	char Ascii = _pMsg->m_Param1;
	*/
	if (_Key.IsASCII())
	{
		char Ascii = _Key.GetASCII();

		if( (Ascii >= 'a' && Ascii <= 'z') || (Ascii >= 'A' && Ascii <= 'Z') ||
			(Ascii >= '0' && Ascii <= '9') || Ascii == '_')
		{
			if(m_String.Len() < m_nMaxLen)
				m_String += CStr(Ascii);//_pFocus->m_ID.Right(1);
			CMWnd *pDoneButton = FindItem("DONE");
			if(pDoneButton)
				pDoneButton->SetFocus();

			return 1;
		}
	}
	//}
	return CMWnd_CubeMenu::ProcessKey(_Key, _OriginalKey);
}

void CMWnd_CubeMenu_TextInput::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	Layout_SetMapping(((CMWnd_CubeRgn *)m_pTextView)->GetPositionCube(), 16*16+6);
	CRct Pos = m_pTextView->GetPosition();
	Layout_WriteText(Pos, CStrF("n, %s", m_String.Str()));
	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);
}

//
//
//
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Select_SaveGame, CMWnd_CubeMenu);

int32 CMWnd_CubeMenu_Select_SaveGame::GetNumListItems(bool _Drawing)
{
	return m_lSaveGames.Len();
}

//
//
//
bool CMWnd_CubeMenu_Select_SaveGame::GetListItem(int32 _Index, CStr &_Name, bool _Focus)
{
	if(_Index < m_lSaveGames.Len())
	{
		wchar wText[1024];
		Localize_Str(CStrF("s, %s", m_lSaveGames[_Index].Str()), wText, 1023);
		_Name = wText;

		if(_Focus)
			m_iSelected = _Index;

		_Index++;
		return true;
	}

	return false;
}

//
//
//
void CMWnd_CubeMenu_Select_SaveGame::OnCreate()
{
	m_iSelected = 0;

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	CStr ProfileName = pSys->GetOptions()->GetValue("GAME_PROFILE");

	TArray<CStr> lSaves;
	m_CubeUser.m_pGameContextMod->EnumSaveFiles("", lSaves);

	for(int32 i = 0; i < lSaves.Len(); i++)
	{
		int32 c = 0;
		while(c < m_lSaveGames.Len())
		{
			if(m_lSaveGames[c] < lSaves[i])
				break;
			c++;
		}

		m_lSaveGames.Insert(c, lSaves[i]);

	}

	//m_lSaveGames.Sort(true);


	/*
	for(int32 i = 0; i < m_lSaveGames.Len(); i++)
	{
	if(m_lSaveGames[i] == "profile")
	{
	m_lSaveGames.Del(i);
	break;
	}
	}*/

	// init stuff
	CMWnd_CubeMenu::OnCreate();
}

bool CMWnd_CubeMenu_Select_SaveGame::OnButtonPress(CMWnd *_pFocus)
{
	if(_pFocus->m_ID == "" && m_lButtons[m_iSelected - m_iFirstListItem]->m_Status & WSTATUS_FOCUS)
	{
		if(m_CubeUser.m_pGameContextMod->ValidateSaveFile("", m_lSaveGames[m_iSelected]))
		{
			ConExecute("padlock()"); // lock pad
			m_CubeUser.m_pFrontEndMod->m_CachedCommand = CStrF("loadgame (\"%s\"); cg_clearmenus()", m_lSaveGames[m_iSelected].Str());
			ConExecute("cg_blackloading();cg_rootmenu(\"begin_loadtransform\")");
		}
		else
		{
			// Warn about corrupt file
		}

		// Load game
		return true;
	}

	return false;
}

// ---------------------------------------------------------------------------------------------
/*
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_SaveProfile, CMWnd_CubeMenu);
//
//
//
void CMWnd_CubeMenu_SaveProfile::OnCreate()
{
m_StartTime = CMTime::GetCPU();
m_HasWritten = false;
CMWnd_CubeMenu::OnCreate();
}

//
//
//
void CMWnd_CubeMenu_SaveProfile::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
if((CMTime::GetCPU()-m_StartTime).GetTime() > 1)
{
if(!m_HasWritten)
{
CStr Profile;
m_CubeUser.m_pGameContextMod->FixProfile(Profile);
m_CubeUser.m_pGameContextMod->m_SaveProfile = Profile;
m_HasWritten = true;
}

if(!m_CubeUser.m_pGameContextMod->m_spAsyncSaveContext || m_CubeUser.m_pGameContextMod->m_spAsyncSaveContext->m_spWData->m_AsyncWriter.Done())
{
ConExecute("cg_prevmenu()");
}
}

CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);
};
*/


// --------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Extra_Content, CMWnd_CubeMenu);

// $$$ OPTIMIZE ME!

//
//
//
int32 CMWnd_CubeMenu_Extra_Content::GetNumListItems(bool _Drawing)
{
	return m_CubeUser.m_pGameContextMod->m_lViewableContent.Len();

	/*
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");

	int32 count = 0;
	CExtraContentKey Key;
	Key.Parse(pSys->GetOptions()->GetValue("GAME_EXTRACONTENTKEY"));
	for(int32 i = 0; i < m_CubeUser.m_pGameContextMod->m_ExtraContent.m_lspContent.Len(); i++)
	{
	if(Key.IsActive(m_CubeUser.m_pGameContextMod->m_ExtraContent.m_lspContent[i]->m_Key))
	count++;
	}

	return count; //m_CubeUser.m_pGameContextMod->m_ExtraContent.m_lspContent.Len();

	*/
}

//
//
//
bool CMWnd_CubeMenu_Extra_Content::GetListItem(int32 _Index, CStr &_Name, bool _Focus)
{
	wchar wText[1024];
	Localize_Str(CStrF("s, %s", m_CubeUser.m_pGameContextMod->m_lViewableContent[_Index].m_spContent->m_Name.Str()), wText, 1023);
	_Name = wText;
	if(_Focus)
		m_CubeUser.m_pGameContextMod->m_iCurrentContent =_Index;
	return true;
}

//
//
//
void CMWnd_CubeMenu_Extra_Content::AfterItemRender(int32 _Index)
{
	return;
}

void CMWnd_CubeMenu_Extra_Content::OnCreate()
{
	CMWnd_CubeMenu::OnCreate();
	m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = CStr("");
	CMWnd *pWnd = FindItem("LIVEUPDATE");
	if(pWnd)
		pWnd->SetFocus();
	//m_CubeUser.m_pFrontEndMod->m_CurrentContent = -1;
	m_CubeUser.m_pGameContextMod->m_iCurrentContent = -1;

	m_CubeUser.m_pGameContextMod->UpdateViewableContent();

	/*

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");

	int32 count = 0;
	CExtraContentKey Key;
	Key.Parse(pSys->GetOptions()->GetValue("GAME_EXTRACONTENTKEY"));
	for(int32 i = 0; i < m_CubeUser.m_pGameContextMod->m_ExtraContent.m_lspContent.Len(); i++)
	{
	if(Key.IsActive(m_CubeUser.m_pGameContextMod->m_ExtraContent.m_lspContent[i]->m_Key))
	{
	CItem Item;
	Item.m_Index = i;
	Item.m_spContent = m_CubeUser.m_pGameContextMod->m_ExtraContent.m_lspContent[i];

	int32 c = 0;
	for(; c < m_lItems.Len(); c++)
	{
	if(m_lItems[c].m_spContent->m_Name.Compare(Item.m_spContent->m_Name) > 0)
	break;
	}

	m_lItems.Insert(c, Item);
	}
	}*/
}


bool CMWnd_CubeMenu_Extra_Content::OnButtonPress(CMWnd *_pFocus)
{
	if(!Layout_WndIsButton(_pFocus))
		return false;

	if(_pFocus->m_ID == "LIVEUPDATE")
	{
#if defined(PLATFORM_XBOX1) && !defined(M_DEMO_XBOX)
		if(m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetLoginStatus() != CLiveHandler::STATUS_SUCCESS)
			ConExecute("cg_submenu(\"live_accounts\")");
#endif
	}
	else
	{
		// OPTION: download
		// OPTION: more details
		// OPTION: view content
		if(m_CubeUser.m_pGameContextMod->m_iCurrentContent != -1)
			ConExecute("cg_submenu(\"extra_content_view\")");
	}

	return true;
}

/*
int CMWnd_CubeMenu_Extra_Content::OnMessage(const CMWnd_Message* _pMsg)
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
if (Key.GetKey9() == SKEY_GUI_OK || Key.GetKey9() == SKEY_GUI_START || Key.GetKey9() == SKEY_A)
{
CMWnd *pFocus = GetFocusWnd();
if(!pFocus)
return true;

if(pFocus->m_ID == "LIVEUPDATE")
{
#ifdef PLATFORM_XBOX1
if(m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetLoginStatus() != CLiveHandler::STATUS_SUCCESS)
SetMenu("live_account");
#endif
}
else
{
// OPTION: download
// OPTION: more details
// OPTION: view content
if(m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0].Len())
SetMenu("extra_content_view");
}

return true;
}
}
}
}

return CMWnd_CubeMenu::OnMessage(_pMsg);
}*/



// --------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Extra_Content_View, CMWnd_CubeMenu);

void CMWnd_CubeMenu_Extra_Content_View::OnCreate()
{
	CMWnd_CubeMenu::OnCreate();

	m_Action = ACTION_NONE;
	DoLayout();
	//m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "";
	m_ContentIndex = m_CubeUser.m_pGameContextMod->m_iCurrentContent;


	//CRct Pos = m_lButtons[Count]->GetPosition();
	//Layout_WriteText(m_pExecute, Name, FLAG_ADJUST_SIZE|FLAG_NO_WRAP);

	/*
	m_pDescription = GetItem("Description");
	m_pExecute = GetItem("Execute");
	m_pInstallRemove = GetItem("InstallRemove");

	m_pExecute->SetStatus(WSTATUS_DISABLED, true);

	//CRct Pos = m_lButtons[Count]->GetPosition();
	//Layout_WriteText(m_pExecute, Name, FLAG_ADJUST_SIZE|FLAG_NO_WRAP);
	//m_lButtons[Count]->SetPosition(Pos);
	//
	*/
}

void CMWnd_CubeMenu_Extra_Content_View::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	m_aButtonDescriptions[3] = "";
	m_aButtonDescriptions[2] = "";
	//if(m_ContentIndex > 0)
	//m_aButtonDescriptions[3] = "<,§LMENU_PREVIOUS";

	//if(m_ContentIndex < m_CubeUser.m_pGameContextMod->m_lViewableContent.Len()-1)
	//m_aButtonDescriptions[2] = ">,§LMENU_NEXT";

	DoLayout();
	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);
}

void CMWnd_CubeMenu_Extra_Content_View::DoLayout()
{
	m_ContentIndex = m_CubeUser.m_pGameContextMod->m_iCurrentContent;
	TPtr<CExtraContentHandler::CContent> spContent = m_CubeUser.m_pGameContextMod->m_lViewableContent[m_ContentIndex].m_spContent;

	wchar wText[1024];
	if(GetItem("Description"))
	{
		Localize_Str(spContent->m_Desc, wText, 1023);
		CRct Pos = GetItem("Description")->GetPosition();
		Layout_Clear(Pos, CCellInfo::FLAG_FASTSWITCH);
		Layout_WriteText(Pos, 's', wText, FLAG_FASTSWITCH);
	}

	if(GetItem("Title"))
	{
		Localize_Str(spContent->m_Name, wText, 1023);
		CRct Pos = GetItem("Title")->GetPosition();
		CStr Text = wText;
		Layout_Clear(Pos, CCellInfo::FLAG_FASTSWITCH);
		if(Text.Len() > 20)
			Layout_WriteText(Pos, 's', wText, FLAG_CENTER|FLAG_FASTSWITCH);
		else
			Layout_WriteText(Pos, 'n', wText, FLAG_CENTER|FLAG_FASTSWITCH);
	}

#if defined(PLATFORM_XBOX1) && !defined(M_DEMO_XBOX) && defined(XBOX_SUPPORT_DLCONTENT)
	// check for live content
	CLiveHandler::CLiveContent *pLiveContent = m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.FindLiveContent(spContent->m_ID);
	if(pLiveContent && !pLiveContent->m_Installed)
	{
		if(GetItem("DownloadSize"))
		{
			Localize_Str(CStrF("%d §LMENU_TO_DOWNLOAD", pLiveContent->m_DownloadSize/1024), wText, 1023);
			Layout_WriteText(GetItem("DownloadSize")->GetPosition(), 's', wText, 0);
		}
		if(GetItem("InstallSize"))
		{
			Localize_Str(CStrF("%d §LMENU_BLOCKSNEEDED", pLiveContent->m_InstallSize), wText, 1023);
			Layout_WriteText(GetItem("InstallSize")->GetPosition(), 's', wText, 0);
		}
	}
#endif


	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if(pTC)
	{

		int32 TexID = -1;
		if(spContent->m_Thumbnail.Len())
			TexID = pTC->GetTextureID(spContent->m_Thumbnail);

		if(TexID == -1)
			TexID = pTC->GetTextureID(spContent->m_ID);

		if(TexID == -1)
			TexID = pTC->GetTextureID("GUI_ExtraContentNA");

		Layout_SetSecondary(TexID, CRC_RASTERMODE_ALPHABLEND);
	}

}

//
//
//
int32 CMWnd_CubeMenu_Extra_Content_View::GetNumListItems(bool _Drawing)
{
	int32 Count = 0;
	TPtr<CExtraContentHandler::CContent> spContent = m_CubeUser.m_pGameContextMod->m_lViewableContent[m_ContentIndex].m_spContent;

	bool CanBeActivated = false;
	if(spContent->m_Type != CExtraContentHandler::TYPE_TIMELOCKLEVEL)
		CanBeActivated = true;
#if defined(PLATFORM_XBOX1) && !defined(M_DEMO_XBOX) && defined(XBOX_SUPPORT_DLCONTENT)
	CLiveHandler::CLiveContent *pLiveContent = m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.FindLiveContent(spContent->m_ID);
	if(pLiveContent)
	{
		if(!pLiveContent->m_Installed)
			CanBeActivated = false;

		// download/remove
		Count++;
	}
#endif

	if(CanBeActivated)
		Count++; // activate

	return Count;
}

//
//
//
bool CMWnd_CubeMenu_Extra_Content_View::GetListItem(int32 _Index, CStr &_Name, bool _Focus)
{
	TPtr<CExtraContentHandler::CContent> spContent = m_CubeUser.m_pGameContextMod->m_lViewableContent[m_ContentIndex].m_spContent;
	// check for live content
	bool CanBeActivated = false;
	if(spContent->m_Type != CExtraContentHandler::TYPE_TIMELOCKLEVEL)
		CanBeActivated = true;


#if defined(PLATFORM_XBOX1) && !defined(M_DEMO_XBOX) && defined(XBOX_SUPPORT_DLCONTENT)
	CLiveHandler::CLiveContent *pLiveContent = m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.FindLiveContent(spContent->m_ID);
	if(pLiveContent)
	{
		if(!pLiveContent->m_Installed)
			CanBeActivated = false;
	}
#endif

	if(_Index == 0 && CanBeActivated)
	{
		if(_Focus && spContent->m_Type == CExtraContentHandler::TYPE_EXTRALEVEL || spContent->m_Type == CExtraContentHandler::TYPE_SCRIPT)
			m_Action = ACTION_LAUNCH;
		else if(_Focus && spContent->m_Type == CExtraContentHandler::TYPE_TEXT)
			m_Action = ACTION_READ;
		else
			m_Action = ACTION_VIEW;

		if(	spContent->m_Type == CExtraContentHandler::TYPE_PICTURE ||
			spContent->m_Type == CExtraContentHandler::TYPE_VIDEO)
		{
			_Name = "n, §LMENU_VIEW";
		}
		else if(spContent->m_Type == CExtraContentHandler::TYPE_EXTRALEVEL || spContent->m_Type == CExtraContentHandler::TYPE_SCRIPT)
		{
			_Name = "n, §LMENU_LAUNCH";
		}
		else if(spContent->m_Type == CExtraContentHandler::TYPE_TEXT)
		{
			_Name = "n, §LMENU_READ";
		}

		return true;
	}
	else
	{
#if defined(PLATFORM_XBOX1) && !defined(M_DEMO_XBOX) && defined(XBOX_SUPPORT_DLCONTENT)
		if(pLiveContent)
		{
			if(pLiveContent->m_Installed)
			{
				if(_Focus)
					m_Action = ACTION_REMOVE;
				_Name = "n, §LMENU_REMOVE";
			}
			else
			{
				if(_Focus)
					m_Action = ACTION_DOWNLOAD;
				_Name = "n, §LMENU_DOWNLOAD";
			}
		}

		return true;
#endif
	}

	return false;
}

//
//
//
bool CMWnd_CubeMenu_Extra_Content_View::OnButtonPress(CMWnd *_pFocus)
{
	if(!Layout_WndIsButton(_pFocus))
		return false;

	if(m_Action == ACTION_VIEW)
	{
		if(m_CubeUser.m_pGameContextMod->m_lViewableContent.Len() > m_ContentIndex)
		{
			TPtr<CExtraContentHandler::CContent> spContent = m_CubeUser.m_pGameContextMod->m_lViewableContent[m_ContentIndex].m_spContent;
			if(spContent && spContent->m_Type == CExtraContentHandler::TYPE_VIDEO)
			{
				MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
				if(pTC)
				{
					int iID = pTC->GetTextureID("*video_" + spContent->m_ID);
					if(iID == 0)
					{
						ConExecute("cg_submenu(\"extra_content_error\")");
						return true;
					}
				}
			}
		}

		ConExecute("cg_submenu(\"extra_content_view_texture\")");
	}

	if(m_Action == ACTION_READ)
		ConExecute("cg_submenu(\"extra_content_view_text\")");

	if(m_Action == ACTION_LAUNCH)
		ConExecuteImp(m_CubeUser.m_pGameContextMod->m_lViewableContent[m_ContentIndex].m_spContent->m_ID, __FUNCTION__, 0);

#if defined(PLATFORM_XBOX1) && defined(XBOX_SUPPORT_DLCONTENT)
	CLiveHandler::CLiveContent *pLiveContent = m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.FindLiveContent(m_CubeUser.m_pGameContextMod->m_lViewableContent[m_ContentIndex]->m_ID);
	if(pLiveContent)
	{
		if(m_Action == ACTION_DOWNLOAD)
		{
			m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = CStrF("%d", m_ContentIndex);
			ConExecute("cg_submenu(\"live_download\")");
		}

		if(m_Action == ACTION_REMOVE)
		{
			m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.RemoveContent(pLiveContent->m_OfferingID);
			pLiveContent->m_Installed = false;
			m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "";
			ConExecute("cg_switchmenu(\"live_updatecontentlist\")");
		}
	}
#endif
	return true;
}

// --------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_View_Texture, CMWnd_CenterImage);

void CMWnd_View_Texture::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if(_Key == "TEXTURE_NAME")
	{
		MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
		if(pGame)
		{
			CWFrontEnd_Mod *m_pFrontEndMod = TDynamicCast<CWFrontEnd_Mod>((CWFrontEnd*)pGame->m_spFrontEnd); // UGLY
			class CGameContextMod *m_pGameContextMod = TDynamicCast<CGameContextMod>(pGame);
			if(m_pGameContextMod && m_pFrontEndMod)
			{
				int32 Index = m_pGameContextMod->m_iCurrentContent; //m_aWindowTransferData[0].Val_int();
				CStr TextureName;
				TextureName = m_pGameContextMod->m_lViewableContent[Index].m_spContent->m_ID;
				if(m_pGameContextMod->m_lViewableContent[Index].m_spContent->m_Type == CExtraContentHandler::TYPE_VIDEO)
				{
					TextureName = "*VIDEO_"+TextureName;
					ConExecute("stream_stop(1)");
				}

				return CMWnd_ModTexture::EvaluateKey(_pParam, _Key, TextureName);
			}
		}
	}
	if(_Key == "TEXTURE_MODE")
	{
		MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
		if(pGame)
		{
			CWFrontEnd_Mod *m_pFrontEndMod = TDynamicCast<CWFrontEnd_Mod>((CWFrontEnd*)pGame->m_spFrontEnd); // UGLY
			class CGameContextMod *m_pGameContextMod = TDynamicCast<CGameContextMod>(pGame);
			if(m_pGameContextMod && m_pFrontEndMod)
			{
				int32 Index = m_pGameContextMod->m_iCurrentContent; //m_aWindowTransferData[0].Val_int();
				if(m_pGameContextMod->m_lViewableContent[Index].m_spContent->m_Type == CExtraContentHandler::TYPE_VIDEO)
					CMWnd_ModTexture::EvaluateKey(_pParam, _Key, "LINK cg_prevmenu");

				fp32 Aspect = 1.0f;
				if(m_pGameContextMod->m_lViewableContent[Index].m_spContent->m_Param != 0)
					Aspect = m_pGameContextMod->m_lViewableContent[Index].m_spContent->m_Param;

				CMWnd_ModTexture::EvaluateKey(_pParam, "PIXELASPECT", CStrF("%f", Aspect));

				return;
			}
		}
	}


	return CMWnd_CenterImage::EvaluateKey(_pParam, _Key, _Value);
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Extra_Content_ViewText, CMWnd_CubeMenu);

bool CMWnd_CubeMenu_Extra_Content_ViewText::OnButtonPress(CMWnd *_pFocus)
{
	if(m_Page == m_NumPages-1)
#ifdef PLATFORM_CONSOLE
		ConExecute("cg_prevmenu()");
#else
	{
	}
#endif
	else
	{
		m_Page++;
		Redisplay();
	}
	return false;
}
/*
bool CMWnd_CubeMenu_Extra_Content_ViewText::OnOtherButtonPress(CMWnd *_pFocus, int32 _Button)
{
if((_Button == SKEY_MOUSE2 || _Button == SKEY_GUI_LEFT) && m_Page > 0)
{
m_Page--;
Redisplay();
}
if(_Button == SKEY_GUI_RIGHT && m_Page < m_NumPages - 1)
{
m_Page++;
Redisplay();
}
return false;
}*/

aint CMWnd_CubeMenu_Extra_Content_ViewText::OnMessage(const CMWnd_Message* _pMsg)
{
	if(_pMsg->m_Msg == WMSG_MENUIMPULSE)
	{
		if(_pMsg->m_Param0 == 0 && m_Page > 0)
		{
			m_Page--;
			Redisplay();
		}
		else if(_pMsg->m_Param0 == 1 && m_Page < m_NumPages - 1)
		{
			m_Page++;
			Redisplay();
		}
	}

	return CMWnd_CubeMenu::OnMessage(_pMsg);
}

void CMWnd_CubeMenu_Extra_Content_ViewText::Redisplay()
{
	if(!m_pReg)
		return;

	Layout_Clear(Layout_CubeSpace2ViewSpace(m_TextRect));
	//pTextWnd->

	for(int32 i = 0; i < m_PageSize; i++)
	{
		CRct r = m_TextRect;
		r.p0.y += i;
		r.p1.y = r.p0.y;
		if(m_Page*m_PageSize+i == m_pReg->GetNumChildren())
			break;
		Layout_WriteText(r, "s," + m_pReg->GetValue(m_Page*m_PageSize+i), FLAG_NO_CONVERT);
	}

	CMWnd *pPageWnd = GetItem("PageCount");
	if(pPageWnd)
	{
		Layout_Clear(Layout_CubeSpace2ViewSpace(m_PageCountRect));
		Layout_WriteText(m_PageCountRect, "n," + CStrF("%d/%d", m_Page+1, m_NumPages), FLAG_NO_CONVERT);
	}
}

void CMWnd_CubeMenu_Extra_Content_ViewText::OnCreate()
{
	m_Page = 0;
	m_PageSize = 18;
	m_NumPages = 1;
	m_pReg = NULL;

	CMWnd *pTextWnd = GetItem("Text");
	if(pTextWnd)
	{
		m_TextRect = Layout_Viewspace2CubeSpace(pTextWnd->GetPosition());
		m_PageSize = m_TextRect.p1.y-m_TextRect.p0.y;

	}

	CMWnd *pPageWnd = GetItem("PageCount");
	if(pPageWnd)
	{
		//Layout_Clear(pPageWnd->GetPosition(), FLAG_NO_CONVERT);
		m_PageCountRect = Layout_Viewspace2CubeSpace(pPageWnd->GetPosition());
		//Layout_WriteText(r, "s," + pChild->GetValue(m_Page*PageSize+i), FLAG_NO_CONVERT);
	}

	TPtr<CExtraContentHandler::CContent> spContent =
		m_CubeUser.m_pGameContextMod->m_lViewableContent[m_CubeUser.m_pGameContextMod->m_iCurrentContent].m_spContent;

	int iRc = m_CubeUser.m_pFrontEndMod->m_spMapData->GetResourceIndex_Registry(spContent->m_ID);
	CRegistry *pReg = m_CubeUser.m_pFrontEndMod->m_spMapData->GetResource_Registry(iRc);
	if(pReg)
	{
		m_pReg = pReg->GetChild(0);

		if(m_pReg)
			m_NumPages = ((m_pReg->GetNumChildren() - 1)/m_PageSize) + 1;
	}


	/*
	CRct m_TextRect;
	CRct m_PageCountRect;
	*/

	Redisplay();


	//CWRes_Registry

	CMWnd_CubeMenu::OnCreate();
}

// --------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_ChooserBuilder, CMWnd_CubeMenu);
void CMWnd_CubeMenu_ChooserBuilder::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if(_Key == "BACK")
	{
		m_CubeUser.m_pFrontEndMod->m_BackoutCommand = _Value;
	}
	else if(_Key == "CHOICE")
	{
		CStr Cmd = _Value;
		CStr Name = Cmd.GetStrSep(",");
		m_CubeUser.m_pFrontEndMod->Chooser_AddChoice(Name, Cmd);
	}
	else if(_Key == "INVOKE")
	{
		m_CubeUser.m_pFrontEndMod->Chooser_Invoke(false);
	}
	else if(_Key == "INVOKE_SWITCHED")
	{
		m_CubeUser.m_pFrontEndMod->Chooser_Invoke(true);
	}
}

void CMWnd_CubeMenu_ChooserBuilder::EvaluateRegisty(CMWnd_Param* _pParam, CRegistry *_pReg)
{
	EvaluateByKey(_pParam, _pReg, "QUESTION");
}

void CMWnd_CubeMenu_ChooserBuilder::EvaluateKeyOrdered(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if(_Key == "QUESTION")
	{
		m_CubeUser.m_pFrontEndMod->Chooser_Init(_Value);
	}
}


// --------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Chooser, CMWnd_CubeMenu);

void CMWnd_CubeMenu_Chooser::OnCreate()
{
	CMWnd_CubeMenu::OnCreate();
}

int CMWnd_CubeMenu_Chooser::ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
{
	if(_Key.GetKey9() == SKEY_GUI_BACK || _Key.GetKey9() == SKEY_GUI_CANCEL)
	{
		ConExecuteImp(m_CubeUser.m_pFrontEndMod->m_BackoutCommand, __FUNCTION__, 0);
		return true;
	}
	return CMWnd_CubeMenu::ProcessKey(_Key, _OriginalKey);
}

void CMWnd_CubeMenu_Chooser::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	CMWnd *pQuestionBox = GetItem("Description");

	//_Desc = wText;
	/*
	wchar wText[1024];
	Localize_Str(pCD->m_lMissionDesc[i].Str(), wText, 1023);
	CStr Desc = wText;
	_Name = CStr(L"s, %s", Desc.GetStrSep("|").StrW());

	//_Name = CStrF("s, %s", Desc.GetStrSep("|").Str());
	*/
	if(pQuestionBox)
	{
		//wchar wText[1024];
		//Localize_Str(Desc, wText, 1023);
		Layout_Clear(pQuestionBox->GetPosition(), 0);
		CRct Pos = pQuestionBox->GetPosition();
		Layout_WriteText(Pos, CStrF("n, %s", m_CubeUser.m_pFrontEndMod->m_Question.Str()), 0);
	}

	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);
}

bool CMWnd_CubeMenu_Chooser::OnButtonPress(CMWnd *_pFocus)
{
	if (m_CubeUser.m_pFrontEndMod->m_lChoicesCommands[m_iCurrentListItem] != "")
	{
		ConExecuteImp(m_CubeUser.m_pFrontEndMod->m_lChoicesCommands[m_iCurrentListItem], __FUNCTION__, 0);
		return true;
	}
	return CMWnd_CubeMenu::OnButtonPress(_pFocus);
}

int32 CMWnd_CubeMenu_Chooser::GetNumListItems(bool _Drawing)
{
	return m_CubeUser.m_pFrontEndMod->m_lChoices.Len();
}

//
bool CMWnd_CubeMenu_Chooser::GetListItem(int32 _Index, CStr &_Name, bool _Focus)
{
	_Name = CStrF("nc, %s", m_CubeUser.m_pFrontEndMod->m_lChoices[_Index].Str());
	return true;
}

// -------------------------------------------------------------------
/*--------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_ReturnBack, CMWnd_CubeMenu);
CMWnd_CubeMenu_ReturnBack::~CMWnd_CubeMenu_ReturnBack()
{
SetMenu(m_FrontEndInfo.m_pFrontEnd->m_aWindowTransferData[0], "");
}

void CMWnd_CubeMenu_ReturnBack::OnDestroy()
{
SetMenu(m_FrontEndInfo.m_pFrontEnd->m_aWindowTransferData[0], "");
CMWnd_CubeMenu::OnDestroy();
}*/


// ---------------------------------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_NewGame, CMWnd_CubeMenu);

void CMWnd_CubeMenu_NewGame::OnCreate()
{
	CMWnd_CubeMenu::OnCreate();

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	int bCommentary = pSys->GetRegistry()->GetValuei("OPTG\\GAME_COMMENTARY");
	if(!bCommentary)
	{
		CMWnd *pWnd = FindItem("Commentary");
		if(pWnd)
		{
			CMWnd_ModButton *pButton = TDynamicCast<CMWnd_ModButton>(pWnd);
			if(pButton)
				pButton->m_Script_Pressed = "cg_playsound(\"extra_access_denied\")";
		}
	}
	int Difficulty = pSys->GetRegistry()->GetValuei("OPT\\GAME_DIFFICULTY", 0);
	m_lspWndChildren[1]->KillFocus();
	m_lspWndChildren[Difficulty + 1]->SetFocus();
}

// ---------------------------------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Controller, CMWnd_CenterImage);
void CMWnd_CubeMenu_Controller::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if(_Key == "BUTTON")
	{
		CStr Value = _Value;
		CButton Button;
		Button.x = Value.GetStrSep(",").Val_int();
		Button.y = Value.GetStrSep(",").Val_int();
		Button.m_Text = Value;
		m_lButtons.Add(Button);
	}
	else
		CMWnd_CenterImage::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_CubeMenu_Controller::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	if(m_FrontEndInfo.m_pFrontEnd->m_iCurrentWindow > 0)
		m_FrontEndInfo.m_pFrontEnd->m_aWindows[m_FrontEndInfo.m_pFrontEnd->m_iCurrentWindow-1]->OnPaint(_pRCUtil, _Clip, _Client);

	_pRCUtil->GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE);

	CRct Rect(0,0,640,480);
	_pRCUtil->Rect(_Clip, Rect, 0xC0000000);

	CMWnd_CenterImage::OnPaint(_pRCUtil, _Clip, _Client);

	CRC_Font *pFont = GetFont("TEXT");
	if(!pFont)
		return;


	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	CRegistry *pSt = pSys->GetRegistry()->FindChild("STRINGTABLES");
	CRegistry *pDynamicST = pSt ? pSt->FindChild("DYNAMIC") : NULL;
	if(!pDynamicST)
		return;

	int iConfig = pSys->GetOptions()->GetValuei("CONTROLLER_TYPE", 0, 1);
	wchar wText[1024];
	Localize_Str(CStrF("§LMENU_CONTROLLERTYPE_%d", iConfig+1), wText, 1023);
	fp32 w = pFont->GetWidth(16, wText);
	fp32 h = pFont->GetHeight(16, wText);
	_pRCUtil->Text(_Clip, pFont, TruncToInt(640/2-w/2), TruncToInt((480*0.15f)/2+h), wText, CPixel32(95*2,90*2,70*2, 0xc0), 16);

	bool LeanHack = false;
	if(pDynamicST->Find("CONTROLLER_LEAN2"))
		LeanHack = true;

	for(int32 b = 0; b < m_lButtons.Len(); b++)
	{
		fp32 Scale = (m_ImageSize.x)/(fp32)(640*0.85f);
		fp32 XOffset = (640-(m_ImageSize.x*(1/Scale)))/2;
		fp32 YOffset = (480-(m_ImageSize.y*(1/Scale)))/2;
		fp32 x = m_lButtons[b].x*(1/Scale)+XOffset;
		fp32 y = m_lButtons[b].y*(1/Scale)+YOffset;
		CStr Text = m_lButtons[b].m_Text;
		Text.Trim();

		CStr LocString = "§L"+Text;
		for(int32 i = 0; i < pDynamicST->GetNumChildren(); i++)
		{
			if(pDynamicST->GetValue(i).Compare(LocString) == 0)
			{
				if(LeanHack && pDynamicST->GetName(i) == "CONTROLLER_LEAN")
					continue;
				CStr n = pDynamicST->GetName(i);
				n.GetStrSep("_");
				Text = "§LACTION_"+n;
				break;
			}
		}

		wchar wText[1024];
		Localize_Str(Text, wText, 1023);
		//		fp32 w = pFont->GetWidth(12, wText);
		fp32 h = pFont->GetHeight(12, wText);
		_pRCUtil->Text(_Clip, pFont, TruncToInt(x), TruncToInt(y-h/2), wText, CPixel32(95*2,90*2,70*2, 0xc0), 12);
	}
}


// ---------------------------------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Controller2, CMWnd_CubeMenu);
void CMWnd_CubeMenu_Controller2::OnCreate()
{

}

void CMWnd_CubeMenu_Controller2::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if(_Key == "CONTROLLERTYPE")
	{
		CStr Value = _Value;
		m_iType = Value.Val_int();
	}
	if(_Key == "BUTTON")
	{
		CStr Value = _Value;
		CButton Button;
		Button.x = Value.GetStrSep(",").Val_int();
		Button.y = Value.GetStrSep(",").Val_int();
		Button.m_Text = Value.GetStrSep(",");
		Button.m_ImageName = Value.GetStrSep(",");
		Button.m_bAfterText = Value.GetStrSep(",").Val_int() ? true : false;
		Button.m_Size.x = Value.GetStrSep(",").Val_int();
		if(!Button.m_Size.x)
			Button.m_Size.x = 22;
		Button.m_Size.y = Value.GetStrSep(",").Val_int();
		if(!Button.m_Size.y)
			Button.m_Size.y = 22;
		m_lButtons.Add(Button);
	}
	else if(_Key == "OFFSET")
	{
		CStr Value = _Value;
		m_Offset.x = Value.GetStrSep(",").Val_int();
		m_Offset.y = Value.GetStrSep(",").Val_int();
	}
	else if(_Key == "TEXTURE_NAME")
	{
		CStr Value = _Value;
		m_TextureName = Value;
	}
	else if(_Key == "IMAGESIZE")
	{
		CStr Value = _Value;
		m_ImageSize.x = Value.GetStrSep(",").Val_int();
		m_ImageSize.y = Value.GetStrSep(",").Val_int();
	}
	else
		CMWnd_CubeMenu::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_CubeMenu_Controller2::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	m_CubeUser.m_pFrontEndMod->m_Cube.m_GUIFlags |=	CCube::GUI_FLAGS_DISABLEPROJECTIONANIM;
	m_CubeUser.m_pFrontEndMod->m_Cube.m_GUIFlags |=	CCube::GUI_FLAGS_DONTPROJECT; // HACK
	m_CubeUser.m_pFrontEndMod->m_Cube.m_GUIFlags |= CCube::GUI_FLAGS_ISJOURNAL;

	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);

	CXR_VBManager *pVBM = _pRCUtil->GetVBM();
	_pRCUtil->GetVBM()->ScopeBegin("CMWnd_CubeMenu_Controller2::OnPaint", false);
	pVBM->AddClearRenderTarget(0.0f, CDC_CLEAR_COLOR, 0x0, 0, 0);

	//-----------------------------------------------------------------------------------------------------
	// render the texture first, center of the screen

	CVec2Dfp32 CoordScale = _pRCUtil->GetCoordinateScale();
	CMat4Dfp32* pMat2D = pVBM->Alloc_M4_Proj2DRelBackPlane();
	if (!pMat2D)
		return;

	// for flip or no-flip UV cords
	CVec2Dfp32 UVMin(0.0f, 0.0f);
	CVec2Dfp32 UVMax(1.0f, 1.0f);
	CVec2Dfp32* pTV = CXR_Util::VBM_CreateRectUV(pVBM, UVMin, UVMax);

	int PictureHeightBase	 =  m_ImageSize.x;
	int PictureWidthBase	 =  m_ImageSize.y;

	fp32 PicWidth			 =	PictureWidthBase * CoordScale.k[0];
	fp32 PicHeight			 =	PictureHeightBase * CoordScale.k[1];

	CRC_Viewport*pViewport = pVBM->Viewport_Get();
	CClipRect VPClip = pViewport->GetViewClip();
	fp32 cw = TruncToInt((VPClip.clip.GetWidth() / CoordScale.k[0]) * 0.5f);
	fp32 ch = TruncToInt((VPClip.clip.GetHeight() / CoordScale.k[1]) * 0.5f);
	CPnt PictureStartPos;
	PictureStartPos.x = TruncToInt((cw - PictureWidthBase*0.5f) * CoordScale.k[0]);
	PictureStartPos.y = TruncToInt((ch - PictureHeightBase*0.5f) * CoordScale.k[1]);

	CRect2Duint16 PictureScreenCoord;
	PictureScreenCoord.m_Min = CVec2Duint16(PictureStartPos.x, PictureStartPos.y);
	PictureScreenCoord.m_Max = CVec2Duint16((PictureScreenCoord.m_Min.k[0] + TruncToInt(PicWidth)), (PictureScreenCoord.m_Min.k[1] + TruncToInt(PicHeight)));

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	//int iTextureId = pTC->GetTextureID(m_Texture);
	int iTextureId = pTC->GetTextureID(m_TextureName);
	if(iTextureId)
	{
		CRC_Attributes* pA = pVBM->Alloc_Attrib();
		pA->SetDefault();
		pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
		pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
		pA->Attrib_TextureID(0, iTextureId);

		CXR_VertexBuffer* pVBScreenQuad = CXR_Util::VBM_RenderRect(pVBM, pMat2D, PictureScreenCoord, 0xffffffff, 0.1f, pA);
		pVBScreenQuad->Geometry_TVertexArray(pTV, 0);
		pVBM->AddVB(pVBScreenQuad);
	}

	//-----------------------------------------------------------------------------------------------------

	CRC_Font *pFont = GetFont("HEADINGS");
	if(!pFont)
		return;

	CPixel32 PixColor(255, 255, 255, 255);
	CPixel32 PixColorGray(180, 180, 180,90);
	CPixel32 PixColorBlack(0, 0, 0, 255);
	CPixel32 PixColorBlackOutline(0, 0, 0, 105);
	CPixel32 PixColorFlash(255, 255, 255, 64);
	_pRCUtil->GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
	
	wchar wText[1024];
	Localize_Str(CStrF("§LMENU_CONTROLLERTYPE_%d", m_iType), wText, 1023);
	fp32 w = pFont->GetWidth(14, wText);
	fp32 h = pFont->GetHeight(14, wText);
	_pRCUtil->Text(_Clip, pFont, TruncToInt(640/2-w/2), TruncToInt((480*0.15f)/2+h), wText, PixColor, 14);

	for(int32 b = 0; b < m_lButtons.Len(); b++)
	{
		fp32 Scale = (1024.0f)/(fp32)(640*0.85f);
		fp32 XOffset = (640-(1024.0f*(1/Scale)))/2;
		fp32 YOffset = (480-(576.0f*(1/Scale)))/2;
		fp32 x = m_lButtons[b].x*(1/Scale)+XOffset;
		fp32 y = m_lButtons[b].y*(1/Scale)+YOffset;
		CStr Text = m_lButtons[b].m_Text;
		wchar wText[1024];
		Localize_Str(Text, wText, 1023);
		h = pFont->GetHeight(14, wText);
		_pRCUtil->Text(_Clip, pFont, TruncToInt(x), TruncToInt(y-h/2), wText, PixColor, 14);

		iTextureId = pTC->GetTextureID(m_lButtons[b].m_ImageName);
		if(iTextureId)
		{
			PicWidth			 =	m_lButtons[b].m_Size.x * CoordScale.k[0];
			PicHeight			 =	m_lButtons[b].m_Size.y * CoordScale.k[1];
			CPnt PictureStartPos;

			if(m_lButtons[b].m_bAfterText)
			{
				w = pFont->GetWidth(14, wText);
				PictureStartPos.x = TruncToInt((x + w - 5 + (m_lButtons[b].m_Size.x/2.0f) + _Clip.ofs.x) * CoordScale.k[0] );
			}
			else
				PictureStartPos.x = TruncToInt((x - 5 - (m_lButtons[b].m_Size.x/2.0f) + _Clip.ofs.x) * CoordScale.k[0] );
			
			PictureStartPos.y = TruncToInt((y - (m_lButtons[b].m_Size.y/2.0f)  + _Clip.ofs.y) * CoordScale.k[1]);

			PictureScreenCoord.m_Min = CVec2Duint16(PictureStartPos.x, PictureStartPos.y);
			PictureScreenCoord.m_Max = CVec2Duint16((PictureScreenCoord.m_Min.k[0] + TruncToInt(PicWidth)), (PictureScreenCoord.m_Min.k[1] + TruncToInt(PicHeight)));

			CRC_Attributes* pA = pVBM->Alloc_Attrib();
			pA->SetDefault();
			pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
			pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			pA->Attrib_TextureID(0, iTextureId);

			CXR_VertexBuffer* pVBScreenQuad = CXR_Util::VBM_RenderRect(pVBM, pMat2D, PictureScreenCoord, 0xffffffff, 0.1f, pA);
			pVBScreenQuad->Geometry_TVertexArray(pTV, 0);
			pVBM->AddVB(pVBScreenQuad);
		}
		
	}

	MixTentaclesAndMenu(_pRCUtil);
	pVBM->ScopeEnd();
}

// ---------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModGameMenu, CMWnd);
//
//
//
aint CMWnd_ModGameMenu::OnMessage(const CMWnd_Message* _pMsg)
{
	MAUTOSTRIP(CMWnd_ModGameMenu_OnMessage, 0);
	switch(_pMsg->m_Msg)
	{
	case WMSG_GETMAPDATA : 
		return (aint)(CMapData*) ((m_FrontEndInfo.m_pFrontEnd) ? m_FrontEndInfo.m_pFrontEnd->m_spMapData : (TPtr<CMapData>)NULL);

	case WMSG_KEY :
		{

			CScanKey Key;
			Key.m_ScanKey32 = _pMsg->m_Param0;
			CScanKey OriginalKey;
			OriginalKey.m_ScanKey32 = _pMsg->m_Param2;
			OriginalKey.m_Char = _pMsg->m_Param3;

			if (Key.IsDown())
			{	
				// Unlock X Y X X Y   Y X Y X X Y Y
				// 1 = x, 2 = y
				const int32 aCheatSequence[] = {1,2,1,1,2, 2,1,2,1,1,2,2};
				int32 CheatKey = 0;
				if (Key.GetKey9() == SKEY_GUI_BUTTON2 || Key.GetKey9() == SKEY_X)
					CheatKey = 1;
				else if (Key.GetKey9() == SKEY_GUI_BUTTON3 || Key.GetKey9() == SKEY_Y)
					CheatKey = 2;

				if(aCheatSequence[m_CheatUplockMode] == CheatKey)
				{
					m_CheatUplockMode++;
					if(m_CheatUplockMode == 12 && m_CheatScript1.Len())
					{
						m_CheatUplockMode = 0;
						ConExecuteImp(m_CheatScript1, __FUNCTION__, 0);
					}
				}
				else
					m_CheatUplockMode = 0;

				// Escape
				if ((Key.GetKey9() == SKEY_ESC ||
					Key.GetKey9() == SKEY_GUI_BACK ||
					Key.GetKey9() == SKEY_GUI_CANCEL)
					&& !m_pInfoScreen->m_bActive)
				{
					ConExecute("cg_clearmenus()");
					return true;
				}
			}

			return CMWnd::OnMessage(_pMsg);
		}
	default:

		return CMWnd::OnMessage(_pMsg);
	}
}

//
//

//
void CMWnd_ModGameMenu::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	MAUTOSTRIP(CMWnd_ModGameMenu_EvaluateKey, MAUTOSTRIP_VOID);
	if (_Key == "SCRIPT_SECRET1")
		m_CheatScript1 = _Value;

	CMWnd::EvaluateKey(_pParam, _Key, _Value);
}

//
//
//
void CMWnd_ModGameMenu::OnRefresh()
{
	MAUTOSTRIP(CMWnd_ModGameMenu_OnRefresh, MAUTOSTRIP_VOID);
	if (m_pInfoScreen)
	{
		if (m_pInfoScreen->m_ShouldCreate)
		{
			m_pInfoScreen->m_ShouldCreate = false;

			m_pInfoScreen->Create(NULL, CMWnd_Param(0, 0, 640, 480, 
				WSTYLE_ONTOP | WSTYLE_HIDDENFOCUS), "m_pInfoScreen");

			AddChild(m_pInfoScreen);
		}

		// Exit
		if (m_pInfoScreen->m_Result == 1) 
		{
			MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
			pCon->ExecuteString(m_ConfimScript);
		}
	}
}



class CMWnd_CubeMenu_PuffTris : public CMWnd_CubeMenu
{
	MRTC_DECLARE;
public:
	enum
	{
		TWIDTH = 10,
		THEIGHT = 20,
	};

	int8 m_Field[THEIGHT][TWIDTH];
	int8 m_OldField[THEIGHT][TWIDTH];
	bool m_bPartRedraw;
	int8 m_Blocks[7][4][2];
	int m_iX;
	int m_iY;
	int m_iType;
	int m_iRot;
	int m_iScore;
	CMTime m_NextRefresh;
	CVec3Dfp32 m_Angles;
	fp32 m_Mul;
	fp32 m_MulChange;

	void OnCreate()
	{
		for(int y = 0; y < THEIGHT; y++)
			for(int x = 0; x < TWIDTH; x++)
			{
				m_Field[y][x] = -1;
				m_OldField[y][x] = 0;
			}

			m_NextRefresh = CMTime::GetCPU() + CMTime::CreateFromSeconds(1);
			m_iRot = -2;
			m_iScore = 0;
			m_iX = 0;
			m_iY = 0;
			m_iType = -1;
			m_Mul = -0.18359375f;
			m_MulChange = 0.24609375f;

			m_Blocks[0][0][0] = 0; m_Blocks[0][0][1] = -1;
			m_Blocks[0][1][0] = 0; m_Blocks[0][1][1] = 0;
			m_Blocks[0][2][0] = 0; m_Blocks[0][2][1] = 1;
			m_Blocks[0][3][0] = 0; m_Blocks[0][3][1] = 2;

			m_Blocks[1][0][0] = 0; m_Blocks[1][0][1] = 0;
			m_Blocks[1][1][0] = -1; m_Blocks[1][1][1] = 0;
			m_Blocks[1][2][0] = 0; m_Blocks[1][2][1] = -1;
			m_Blocks[1][3][0] = -1; m_Blocks[1][3][1] = -1;

			m_Blocks[2][0][0] = -1; m_Blocks[2][0][1] = 0;
			m_Blocks[2][1][0] = 0; m_Blocks[2][1][1] = 0;
			m_Blocks[2][2][0] = 1; m_Blocks[2][2][1] = 0;
			m_Blocks[2][3][0] = 0; m_Blocks[2][3][1] = 1;

			m_Blocks[3][0][0] = 0; m_Blocks[3][0][1] = -1;
			m_Blocks[3][1][0] = 0; m_Blocks[3][1][1] = 0;
			m_Blocks[3][2][0] = 0; m_Blocks[3][2][1] = 1;
			m_Blocks[3][3][0] = -1; m_Blocks[3][3][1] = 1;

			m_Blocks[4][0][0] = 0; m_Blocks[4][0][1] = -1;
			m_Blocks[4][1][0] = 0; m_Blocks[4][1][1] = 0;
			m_Blocks[4][2][0] = 0; m_Blocks[4][2][1] = 1;
			m_Blocks[4][3][0] = 1; m_Blocks[4][3][1] = 1;

			m_Blocks[5][0][0] = -1; m_Blocks[5][0][1] = 0;
			m_Blocks[5][1][0] = 0; m_Blocks[5][1][1] = 0;
			m_Blocks[5][2][0] = 0; m_Blocks[5][2][1] = 1;
			m_Blocks[5][3][0] = 1; m_Blocks[5][3][1] = 1;

			m_Blocks[6][0][0] = 1; m_Blocks[6][0][1] = 0;
			m_Blocks[6][1][0] = 0; m_Blocks[6][1][1] = 0;
			m_Blocks[6][2][0] = 0; m_Blocks[6][2][1] = 1;
			m_Blocks[6][3][0] = -1; m_Blocks[6][3][1] = 1;


			for(int i = 0; i < 20; i++)
			{
				Layout_SetMapping(19, i, 16 * 16);
				Layout_SetMapping(20 - TWIDTH - 2, i, 256 - 16);
			}
			Layout_SetMapping(0, 1, 256 + 10);
			Layout_SetMapping(1, 1, 256 + 11);
			Layout_SetMapping(2, 1, 256 + 12);
			Layout_SetMapping(3, 1, 256 + 13);
			Layout_SetMapping(4, 1, 256 + 14);
			Layout_SetMapping(5, 1, 256 + 11);
			Layout_SetMapping(6, 1, 256 + 12);
			Layout_SetMapping(7, 1, 256 + 15);

			Layout_SetMapping(0, 8, 256 + 10);
			Layout_SetMapping(1, 8, 256 + 11);
			Layout_SetMapping(2, 8, 256 + 12);
			Layout_SetMapping(3, 8, 256 + 13);
			Layout_SetMapping(4, 8, 256 + 14);
			Layout_SetMapping(5, 8, 256 + 11);
			Layout_SetMapping(6, 8, 256 + 12);
			Layout_SetMapping(7, 8, 256 + 15);

			CRct Pos = CRct(0, 2, 9, 2);
			Layout_WriteText(Pos, 'h', WTEXT("Puff"), FLAG_NO_CONVERT | FLAG_FASTSWITCH);
			Pos = CRct(0, 4, 9, 4);
			Layout_WriteText(Pos, 'h', WTEXT("Tris"), FLAG_NO_CONVERT | FLAG_FASTSWITCH);
			Pos = CRct(3, 6, 9, 6);
			Layout_WriteText(Pos, 'h', WTEXT("2"), FLAG_NO_CONVERT | FLAG_FASTSWITCH);
	}

	bool Move(int x, int y, int r)
	{
		int i;
		if(r == -2)
			return FALSE;

		bool bClear = true;
		if(r == -1) 
		{
			m_iRot = 0;
			r = 0;
			bClear = false;
		}

		bool RotSwap = m_iRot == 1 || m_iRot == 3;
		int RotXMul = (m_iRot == 2 || m_iRot == 3) ? -1 : 1;
		int RotYMul = (m_iRot == 1 || m_iRot == 2) ? -1 : 1;

		bool RSwap = r == 1 || r == 3;
		int RXMul = (r == 2 || r == 3) ? -1 : 1;
		int RYMul = (r == 1 || r == 2) ? -1 : 1;

		if(bClear)
		{
			for(i = 0; i < 4; i++) 
			{
				//Clear old
				int nx = m_iX + (RotSwap ? m_Blocks[m_iType][i][1] : m_Blocks[m_iType][i][0]) * RotXMul;
				int ny = m_iY + (RotSwap ? m_Blocks[m_iType][i][0] : m_Blocks[m_iType][i][1]) * RotYMul;
				if(nx >= 0 && nx < TWIDTH && ny >= 0 && ny < THEIGHT)
					m_Field[ny][nx] = -1;
			}
		}

		bool b = TRUE;
		for(i = 0; i < 4; i++)
		{
			//Check if fit
			int nx = x + (RSwap ? m_Blocks[m_iType][i][1] : m_Blocks[m_iType][i][0]) * RXMul;
			int ny = y + (RSwap ? m_Blocks[m_iType][i][0] : m_Blocks[m_iType][i][1]) * RYMul;
			if(ny >= 0)
				if(nx < 0 || nx >= TWIDTH || ny < 0 || ny >= THEIGHT || m_Field[ny][nx] != -1)
					b = FALSE;
		}

		if(b)
		{
			for(i = 0; i < 4; i++)
			{
				//Put new
				int nx = x + (RSwap ? m_Blocks[m_iType][i][1] : m_Blocks[m_iType][i][0]) * RXMul;
				int ny = y + (RSwap ? m_Blocks[m_iType][i][0] : m_Blocks[m_iType][i][1]) * RYMul;
				if(nx >= 0 && nx < TWIDTH && ny >= 0 && ny < THEIGHT)
					m_Field[ny][nx] = m_iType;
			}
			m_iX = x;
			m_iY = y;
			m_iRot = r;

			m_bPartRedraw = TRUE;
			return TRUE;
		} 
		else
		{
			for(i = 0; i < 4; i++)
			{
				//Put old back
				int nx = m_iX + (RotSwap ? m_Blocks[m_iType][i][1] : m_Blocks[m_iType][i][0]) * RotXMul;
				int ny = m_iY + (RotSwap ? m_Blocks[m_iType][i][0] : m_Blocks[m_iType][i][1]) * RotYMul;
				if(nx >= 0 && nx < TWIDTH && ny >= 0 && ny < THEIGHT)
					m_Field[ny][nx] = m_iType;
			}
			return FALSE;
		}
	}

	void Spin()
	{
		Move(m_iX, m_iY, (m_iRot + 1) & 3);
	}

	void FixField()
	{
		int nrr = 0;
		for(int y = THEIGHT - 1; y >= 0; y--)
		{
			bool b = TRUE;
			for(int x = 0; x < TWIDTH; x++)
				if(m_Field[y][x] == -1)
					b = FALSE;
			if(b)
			{
				for(int y2 = y; y2 > 0; y2--)
					for(int x = 0; x < TWIDTH; x++)
						m_Field[y2][x] = m_Field[y2 - 1][x];
				nrr++;
				y++;
			}
		}
		if(nrr > 0)
		{
			switch(nrr)
			{
			case 1:
				m_iScore += 1 * 3;
				break;
			case 2:
				m_iScore += 2 * 4;
				break;
			case 3:
				m_iScore += 3 * 5;
				break;
			case 4:
				m_iScore += 4 * 6;
				break;
			}
		}
	}

	int ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
	{
		if(m_iRot != -3 && m_iType >= 0)
		{
			switch(_Key.GetKey9())
			{
			case SKEY_GUI_LEFT:
				Move(m_iX - 1, m_iY, m_iRot);
				break;
			case SKEY_GUI_RIGHT:
				Move(m_iX + 1, m_iY, m_iRot);
				break;
			case SKEY_GUI_DOWN:
				while(Move(m_iX, m_iY + 1, m_iRot));
				m_NextRefresh = CMTime::GetCPU();
				break;
			case SKEY_GUI_UP:
			case SKEY_GUI_OK:
				Spin();
				break;
			}	
		}

		return CMWnd_CubeMenu::ProcessKey(_Key, _OriginalKey);
	}

	virtual void OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
	{
		CMTime Cur = CMTime::GetCPU();
		if(m_NextRefresh.Compare(Cur) < 0 && m_iRot != -3)
		{
			if(!Move(m_iX, m_iY + 1, m_iRot))
			{
				FixField();
				m_iY = 0;		
				m_iType = MRTC_RAND() % 7;
				if(!Move(TWIDTH / 2 - 1, 0, -1))
				{
					m_iType = -3;
				}
			}
			m_NextRefresh = Cur + CMTime::CreateFromSeconds(0.5f);
		}

		for(int y = 0; y < THEIGHT; y++)
			for(int x = 0; x < TWIDTH; x++)
			{
				switch(m_Field[y][x])
				{
				case -1:
					m_aDepth[y][x + 20 - TWIDTH - 1] = 0;
					Layout_SetMapping(x + 20 - TWIDTH - 1, y, 10, CCellInfo::FLAG_FASTSWITCH);
					break;
				default:
					m_aDepth[y][x + 20 - TWIDTH - 1] = 0.03f;
					Layout_SetMapping(x + 20 - TWIDTH - 1, y, 256 + 6, CCellInfo::FLAG_FASTSWITCH);
					break;
				}
			}

			CRct Pos = CRct(2, 17, 9, 17);
			Layout_WriteText(Pos, 'n', WTEXT("Score"), FLAG_NO_CONVERT | FLAG_FASTSWITCH);
			Pos = CRct(4, 18, 9, 18);
			Layout_WriteText(Pos, 'n', CStrF("%i", m_iScore).Unicode(), FLAG_NO_CONVERT | FLAG_FASTSWITCH);

			fp32 Mul = fp32(m_iScore) / 3.0f;
			if(m_iType == -3)
				Mul = 0;
			Moderatef(m_Mul, Mul, m_MulChange, 8);
			CMat4Dfp32 Mat, Base;
			CVec3Dfp32(M_Sin(Cur.GetTime() * 2.135782f) * m_Mul / 256, M_Sin(Cur.GetTime() * 1.731324f) * m_Mul / 256, M_Sin(Cur.GetTime() * 1.531324f) * m_Mul / 256).CreateMatrixFromAngles(0, Mat);
			m_CubeUser.m_pFrontEndMod->m_Cube.m_aSides[m_CubeUser.m_pFrontEndMod->m_Cube.m_CurrentSide].m_Transform.Transpose(Base);
			Mat.Multiply(Base, m_CubeUser.m_pFrontEndMod->m_Cube.m_Wanted);

			CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);
	}
};

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_PuffTris, CMWnd_CubeMenu);


// ---------------------------------------------------------------------------------------------
void CMWnd_CubeMenu_LoadMap::OnCreate()
{
	CMWnd_CubeMenu::OnCreate();

	m_iSelected = -1;
	m_LastKeyPressTime = CMTime::CreateFromSeconds(0.0f);

	CMapData& MapData = *m_CubeUser.m_pFrontEndMod->m_spMapData;
	TArray_SimpleSortable<CStr> lFiles = 
		MapData.ResolveDirectory("Worlds\\*.XW", false);

	for (int i = 0; i < lFiles.Len(); i++)
		m_List.Add( lFiles[i].GetFilenameNoExt() );

	if (m_List.Len() == 0)
	{
		// Look for XDF files
		lFiles = MapData.ResolveDirectory("XDF\\*.XDF", false);
		for (int i = 0; i < lFiles.Len(); i++)
		{
			CStr FileName = lFiles[i].GetFilenameNoExt();
			int iFind = FileName.Find("_Server");
			if (iFind >= 0)
			{
				FileName = FileName.Left(iFind);
				m_List.Add( FileName );
			}
		}
	}
}

void CMWnd_CubeMenu_LoadMap::UpdateFocus()
{
	int NumItems = m_List.Len();
	int NumButtons = m_lButtons.Len();

	if (m_iCurrentListItem == 0)
		m_iFirstListItem = 0;
	else if (m_iCurrentListItem == (NumItems - 1))
		m_iFirstListItem = m_iCurrentListItem - Min(NumItems, NumButtons) + 1;
	else
	{
		m_iFirstListItem = Min(m_iFirstListItem, m_iCurrentListItem - 1);
		m_iFirstListItem = Max(m_iFirstListItem, m_iCurrentListItem - Min(NumItems, NumButtons) + 2);
	}

	if (NumButtons > 0)
	{
		int iButton = m_iCurrentListItem - m_iFirstListItem;
		m_lButtons[iButton]->SetFocus();
	}
}


static int FindSubStrNoCase(const char* _pStr, const char* _pSubStr)
{
	if (!_pStr[0] || !_pSubStr[0])
		return -1;

	int j = 0, k = 0;
	for (int i = 0; ; i++)
	{
		if (!_pSubStr[j])
			return k;

		if (!_pStr[i])
			return -1;

		char c1 = CStrBase::clwr(_pStr[i]);
		char c2 = CStrBase::clwr(_pSubStr[j]);
		if (c1 != c2)
		{ // restart
			j = 0; 
			c2 = CStrBase::clwr(_pSubStr[j]);
			k = i + (c1 == c2 ? 0 : 1);
		}
		if (c1 == c2)
			j++; // advance
	}
}

int CMWnd_CubeMenu_LoadMap::ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
{
	char ch = CStrBase::clwr( _OriginalKey.GetASCII() );
	if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9'))
	{
		// Update quickstr
		CMTime tNow = CMTime::GetCPU();
		if ((tNow - m_LastKeyPressTime).GetTime() > 0.7f)
			m_QuickStr.Clear();
		m_QuickStr += ch;
		m_LastKeyPressTime = tNow;
		int QuickStrLen = m_QuickStr.Len();

		// Jump to first entry matching our quickstr
		TAP<const CStr> pList = m_List;
		int iBestEntry = -1, iBestSubStr = 10000;
		for (int i = 0; i < pList.Len(); i++)
		{
			int iSubStr = FindSubStrNoCase(pList[i], m_QuickStr);
			if ((iSubStr >= 0) && (iSubStr < iBestSubStr) && (QuickStrLen > 2 || iSubStr == 0))
			{
				iBestSubStr = iSubStr;
				iBestEntry = i;
			}
		}
		if (iBestEntry >= 0)
		{
			m_iSelected = iBestEntry;
			m_iCurrentListItem = iBestEntry;
			UpdateFocus();
		}
		return 1;
	}
	return CMWnd_CubeMenu::ProcessKey(_Key, _OriginalKey);
}


bool CMWnd_CubeMenu_LoadMap::OnButtonPress(CMWnd* _pFocus)
{
	if (m_iSelected >= 0)
	{
		CStr cmd = CFStrF("map(\"%s\")", m_List[m_iSelected].Str());
		ConExecute(cmd);
		return true;
	}
	return CMWnd_CubeMenu::OnButtonPress(_pFocus);
}

int32 CMWnd_CubeMenu_LoadMap::GetNumListItems(bool _Drawing)
{
	return m_List.Len();
}

bool CMWnd_CubeMenu_LoadMap::GetListItem(int32 _Index, CStr& _Name, bool _bFocus)
{
	_Name = CStrF("s, %s", m_List[_Index].Str());

	if (_bFocus)
		m_iSelected = _Index;

	return true;
}

void CMWnd_CubeMenu_LoadMap::AfterItemRender(int32 _Index)
{
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_LoadMap, CMWnd_CubeMenu);

// ---------------------------------------------------------------------------------------------
TArray<CStr> CMWnd_CubeMenu_LoadScriptLayer::ms_lScriptLayers;
int64 CMWnd_CubeMenu_LoadScriptLayer::ms_TimeStamp = -1;

void CMWnd_CubeMenu_LoadScriptLayer::UpdateScriptLayers()
{
	// get a list of all script-layers from LevelKeys.xrg
	// find the right map/layers for this script-layer
	TPtr<CRegistryCompiled> spRegCompiled;
	spCRegistry spRegistry;
	//spRegistry = CWorld_ServerCore::ReadServerReg("Sv");

	MACRO_GetSystem;
	CStr Params;
	CStr ContentPath = pSys->GetEnvironment()->GetValue("CONTENTPATH");
	if (ContentPath == "")
		ContentPath = pSys->m_ExePath + "Content\\";

	CStr RegisterFile = ContentPath + "Registry\\";
	RegisterFile += "Sv.xrg";

	// Read register
	CStr FileName = RegisterFile.GetPath() + RegisterFile.GetFilenameNoExt() + ".xcr";
	if (CDiskUtil::FileExists(FileName))
	{
		int64 TimeStamp = CDiskUtil::FileTimeGet(FileName).m_TimeWrite;
		if (TimeStamp == ms_TimeStamp)
			return;

		ms_TimeStamp = TimeStamp;
		spRegCompiled = MNew(CRegistryCompiled);
		if (spRegCompiled)
		{
			spRegCompiled->Read_XCR(FileName);
			spRegistry = spRegCompiled->GetRoot();
		}
	}
	else if (CDiskUtil::FileExists(RegisterFile))
	{
		int64 TimeStamp = CDiskUtil::FileTimeGet(RegisterFile).m_TimeWrite;
		if (TimeStamp == ms_TimeStamp)
			return;

		ms_TimeStamp = TimeStamp;
		spRegistry = REGISTRY_CREATE;
		if(spRegistry)
			spRegistry->XRG_Read(RegisterFile);
	}
	else
	{
		ConOutL(CStrF("§cf00ERROR (CWorld_ServerCore::ReadServerReg): file '%s' not found!", RegisterFile.Str()));
		M_TRACEALWAYS("ERROR (CWorld_ServerCore::ReadServerReg): file '%s' not found!\n", RegisterFile.Str());
	}

	if(!spRegistry)
	{
		ConOut("Error reading Registry.");
		return;
	}

	CRegistry *pLevelKeys = spRegistry->Find("LEVELKEYS");
	if(!pLevelKeys)
	{
		ConOut("Error retrieving LevelKeys. Registry not found");
		return;
	}

	//------------------------------------------------------------------------------------------------
	TArray_SimpleSortable<CStr> lTmpSorted;

	int iNrOfChildren = pLevelKeys->GetNumChildren();
	for(int i = 0; i < iNrOfChildren; i++)
	{
		bool bNoLayer = true;
		CRegistry *pReg = pLevelKeys->GetChild(i);
		if (pReg && pReg->GetNumChildren() > 0)
		{
			int iNumScriptLayers = pReg->GetNumChildren();
			for(int j = 0; j < iNumScriptLayers; j++)
			{
				CStr ScriptLayer =  pReg->GetChild(j)->GetThisName();
				if(ScriptLayer == "LAYER")
				{
					lTmpSorted.Add(pReg->GetChild(j)->GetThisValue());
					bNoLayer = false;
				}
			}
		}

		if (bNoLayer)
		{
			CStr ScriptLayer =  pLevelKeys->GetChild(i)->GetThisName();
			if (!ScriptLayer.IsEmpty() && ScriptLayer.CompareNoCase("BASE") != 0)
				lTmpSorted.Add(ScriptLayer);
		}
	}

	// Sort and add to main list
	lTmpSorted.Sort();
	ms_lScriptLayers = lTmpSorted;
}


void CMWnd_CubeMenu_LoadScriptLayer::OnCreate()
{
	CMWnd_CubeMenu::OnCreate();

	UpdateScriptLayers();
	m_Filter = "";
	CreateList();
}


void CMWnd_CubeMenu_LoadScriptLayer::CreateList()
{
	m_List.Clear();
	TAP<CStr> pAllItems = ms_lScriptLayers;
	for (uint i = 0; i < pAllItems.Len(); i++)
	{
		CStr Item = pAllItems[i];
		if (m_Filter.IsEmpty())
		{
			// No filter, add "NY1", "NY2 etc...
			Item = Item.GetStrSep("_");
			int iFound;
			for (iFound = 0; iFound < m_List.Len(); iFound++)
				if (m_List[iFound] == Item) break;
			if (iFound == m_List.Len())
				m_List.Add(Item);
		}
		else
		{
			if (Item.CompareSubStr(m_Filter) == 0)
				m_List.Add(Item);
		}
	}
	m_iSelected = -1;
	m_iCurrentListItem = 0;

	// Reset disabled (to get auto-scroll working properly)
	TAP<CMWnd*> pButtons = m_lButtons;
	for (int i = 0; i < pButtons.Len(); i++)
		pButtons[i]->SetStatus(WSTATUS_DISABLED, false);
}


bool CMWnd_CubeMenu_LoadScriptLayer::OnButtonPress(CMWnd* _pFocus)
{
	if (m_iSelected >= 0)
	{
		if (m_Filter.IsEmpty())
		{
			m_Filter = m_List[m_iSelected];
			CreateList();
			UpdateFocus();
		}
		else
		{
			CStr cmd = CFStrF("scriptlayer(\"%s\")", m_List[m_iSelected].Str());
			ConExecute(cmd);
		}
		return true;
	}
	return CMWnd_CubeMenu::OnButtonPress(_pFocus);
}

int CMWnd_CubeMenu_LoadScriptLayer::ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
{
	if (_Key.GetKey9() == SKEY_GUI_BACK || _Key.GetKey9() == SKEY_GUI_CANCEL)
	{
		if (!m_Filter.IsEmpty())
		{
			m_Filter.Clear();
			CreateList();
			UpdateFocus();
			return true;
		}
	}
	return CMWnd_CubeMenu_LoadMap::ProcessKey(_Key, _OriginalKey);
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_LoadScriptLayer, CMWnd_CubeMenu);

// ---------------------------------------------------------------------------------------------

void CMWnd_CubeMenu_Achievements::OnCreate()
{
	CMWnd_CubeMenu::OnCreate();

	m_iSelected = -1;

#ifdef PLATFORM_XENON
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
	CWGameLiveHandler *pLive = (CWGameLiveHandler*) pGameMod->m_pMPHandler;
	if(!pLive->StartAchievementEnumeration())
		ConExecute("cg_prevmenu()");
	m_bEnumerating = true;
#else
	for(int i = 0; i < 50; i++)
	{
		SListItem item;
		item.Name = CStrF("s, %i. Achievement", i + 1);
		item.Info = "s, Description goes here.";
		item.Completed = false;
		m_List.Add(item);
	}
	m_bEnumerating = false;
#endif
}

int32 CMWnd_CubeMenu_Achievements::GetNumListItems(bool _Drawing)
{
	if(m_bEnumerating)
		return 0;
	return m_List.Len();
}

bool CMWnd_CubeMenu_Achievements::OnButtonPress(CMWnd* _pFocus)
{
#ifdef PLATFORM_XENON
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
	CWGameLiveHandler *pLive = (CWGameLiveHandler*) pGameMod->m_pMPHandler;
	XShowAchievementsUI(pLive->m_iPad);
#endif
	return CMWnd_CubeMenu::OnButtonPress(_pFocus);
}

bool CMWnd_CubeMenu_Achievements::OnOtherButtonPress(CMWnd *_pFocus, int32 _Button)
{
#ifdef PLATFORM_XENON
	if(_Button == SKEY_GUI_BUTTON3 || _Button == SKEY_GUI_BUTTON2)
	{
		MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
		CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
		CWGameLiveHandler *pLive = (CWGameLiveHandler*) pGameMod->m_pMPHandler;
		XShowAchievementsUI(pLive->m_iPad);
	}
#endif

	return CMWnd_CubeMenu::OnOtherButtonPress(_pFocus, _Button);
}

void CMWnd_CubeMenu_Achievements::OnRefresh()
{
	CMWnd_CubeMenu::OnRefresh();
#ifdef PLATFORM_XENON
	if(m_bEnumerating)
	{
		MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
		CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
		CWGameLiveHandler *pLive = (CWGameLiveHandler*) pGameMod->m_pMPHandler;
		if(pLive->GetAchievement(0))
		{
			for(int i = 0; i < 50; i++)
			{
				XACHIEVEMENT_DETAILS *pDetails = pLive->GetAchievement(i);
				if(pDetails)
				{
					SListItem item;
					item.Name = CStrF(WTEXT("s, %i. %s"), i, pDetails->pwszLabel);
					item.Info = CStrF(WTEXT("s, %s"), pDetails->pwszDescription);
					item.Completed = AchievementEarned(pDetails->dwFlags);
					m_List.Add(item);
				}
			}
			m_bEnumerating = false;
		}
	}
#endif
	bool bHasFocus = false;
	for(int32 i = 0; i < m_lButtons.Len(); i++)
	{
		if(m_lButtons[i] == GetFocusWnd())
			bHasFocus = true;
	}
	if(!bHasFocus && m_iCurrentListItem != m_iFirstListItem + 7)
	{
		m_lButtons[7]->SetFocus();
		m_iCurrentListItem = m_iFirstListItem + 7;
	}
}

bool CMWnd_CubeMenu_Achievements::GetListItem(int32 _Index, CStr& _Name, bool _bFocus)
{
	_Name = m_List[_Index].Name;

	if (_bFocus)	
	{
		if(m_iSelected != _Index)
		{
			for(int32 y = 11; y < 17; y++)
				for(int32 x = 0; x < 20; x++)
				{
					m_aMap[y][x].m_Mode = CCellInfo::MODE_NORMAL;
					m_aMap[y][x].Cell() = CCellInfo::Empty();
					m_aMap[y][x].Flags() = 0;
				}
		}
		m_iSelected = _Index;
	}

	return true;
}

void CMWnd_CubeMenu_Achievements::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);
	CRC_Font *pFont = GetFont("HEADINGS");
	if(m_iSelected != -1)
	{
		CRct Rect = CRct(0, 11, 19, 17);
		Layout_WriteText(Rect, m_List[m_iSelected].Info, FLAG_NO_CONVERT|FLAG_FASTSWITCH); 
	}

	if(m_bEnumerating)
		return;

	for(int i = 0; i < m_lButtons.Len(); i++)
	{
		int iIndex = i + m_iFirstListItem;
		if(m_iFirstListItem > 0 && i == 0)
			iIndex = -1;
		if(i == (m_lButtons.Len() - 1) && (m_List.Len() - m_iFirstListItem) != m_lButtons.Len())
			iIndex = -1;

		if(iIndex > -1 && m_List[iIndex].Completed)
		{
			CRct Point = m_lButtons[i]->GetPosition();
			int TexID = pTC->GetTextureID("GUI_HUD_DARKNESS");	

			CVec2Dfp32 CoordScale = _pRCUtil->GetCoordinateScale();
			CXR_VBManager *pVBM = _pRCUtil->GetVBM();
			int ProjSize = m_CubeUser.m_pFrontEndMod->m_Cube.GetProjectionSize();
			Point.p0.x = TruncToInt(Point.p0.x * (ProjSize / 640.0f));
			Point.p0.y = TruncToInt(Point.p0.y * (ProjSize / 480.0f));

			int PicSizeMod = TruncToInt(30 * (ProjSize / 640.0f));

			CRect2Duint16 PictureScreenCoord;
			PictureScreenCoord.m_Min = CVec2Duint16(Point.p0.x - PicSizeMod, Point.p0.y);
			PictureScreenCoord.m_Max = CVec2Duint16(PictureScreenCoord.m_Min.k[0] + PicSizeMod, PictureScreenCoord.m_Min.k[1] + PicSizeMod);

			m_CubeUser.m_pFrontEndMod->m_Cube.AddProjImage(PictureScreenCoord, TexID);
		}
	}
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Achievements, CMWnd_CubeMenu);

// ---------------------------------------------------------------------------------------------

void CMWnd_CubeMenu_Multiplayer_SelectMap::OnCreate()
{
	CMWnd_CubeMenu::OnCreate();

	m_iSelected = -1;

	CStr GameMode;

	MACRO_GetRegisterObject(CGameContext, pGameContext, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGameContext);
	CStr Game = pGameMod->m_spGameReg->GetValue("DEFAULTGAME");
	m_bIsCTF = false;
	if(!Game.Compare("CTF"))
		m_bIsCTF = true;

#ifdef PLATFORM_CONSOLE
	GameMode = pGameMod->m_pMPHandler->GetGameModeName();
#else
	GameMode = "Shapeshifter - CTF";
#endif


	m_List.Add(CStrF("nc, %s", GameMode.Str()));
	m_List.Add("s, §LMENU_RANDOM_MAP");
	m_iMap = 1;

	m_nMaps = 0;
	CMapData& MapData = *m_CubeUser.m_pFrontEndMod->m_spMapData;
	if(!m_bIsCTF)
	{
		TArray_SimpleSortable<CStr> lFiles = MapData.ResolveDirectory("Worlds\\MP*.XW", false);	
		m_nMaps += lFiles.Len();
		for (uint16 i=0; i<lFiles.Len(); i++)
			m_List.Add(CStrF("s, %s", lFiles[i].GetFilenameNoExt().Str()));
	}

	TArray_SimpleSortable<CStr> lFiles2 = MapData.ResolveDirectory("Worlds\\CTF*.XW", false);	
	m_nMaps += lFiles2.Len();
	for (uint16 i=0; i<lFiles2.Len(); i++)
		m_List.Add(CStrF("s, %s", lFiles2[i].GetFilenameNoExt().Str()));

	int ButtonsToRemove = 8 - m_nMaps;
	while(ButtonsToRemove > 0)
	{
		m_lButtons.Del(2 + m_nMaps);
		ButtonsToRemove--;
	}

	m_List.Add("nc, §LMENU_PREVIOUS");
	m_List.Add("nc, §LMENU_FIND_GAME");
	m_List.Add("nc, §LMENU_CREATE_GAME");

	RandomMap();
}

bool CMWnd_CubeMenu_Multiplayer_SelectMap::OnButtonPress(CMWnd* _pFocus)
{
	MACRO_GetRegisterObject(CGameContext, pGameContext, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGameContext);
	if(m_iSelected == m_nMaps + 2)
		ConExecute("cg_prevmenu()");
	else if(m_iSelected == m_nMaps + 3)
	{
#ifdef PLATFORM_CONSOLE
		ConExecute("mp_customgame(0)");
#else
		CStr Map = m_List[m_iMap];
		Map.GetStrSep(",");
		Map.Trim();
		CStr cmd = CFStrF("changemap(\"%s\",16); deferredscript(\"list()\")", Map.Str());
		ConExecute(cmd);
#endif
		return true;
	}
	else if(m_iSelected == m_nMaps + 4)
	{
#ifdef PLATFORM_CONSOLE
		ConExecute("mp_customgame(1)");
#else
		CStr Map = m_List[m_iMap];
		Map.GetStrSep(",");
		Map.Trim();
		CStr cmd = CFStrF("changemap(\"%s\",16); deferredscript(\"list()\")", Map.Str());
		ConExecute(cmd);
#endif
		return true;
	}
	else if(m_iSelected == 1)
	{
		m_lButtons[m_iSelected]->KillFocus();
		m_lButtons[m_nMaps + 3]->SetFocus();
		m_iMap = m_iSelected;
		RandomMap();
	}
	else if(m_iSelected > 1)
	{
		m_lButtons[m_iSelected]->KillFocus();
		m_lButtons[m_nMaps + 3]->SetFocus();
		m_iMap = m_iSelected;

#ifdef PLATFORM_CONSOLE
		pGameMod->m_pMPHandler->m_Flags &= ~MP_FLAGS_RANDOMMAP;
		CStr Map = m_List[m_iMap];
		Map.GetStrSep(",");
		Map.Trim();
		pGameMod->m_pMPHandler->m_Map = Map;
#endif
	}
	return CMWnd_CubeMenu::OnButtonPress(_pFocus);
}

void CMWnd_CubeMenu_Multiplayer_SelectMap::RandomMap(void)
{
#ifdef PLATFORM_CONSOLE
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

	pGameMod->m_pMPHandler->m_Flags |= MP_FLAGS_RANDOMMAP;
	int NumMaps = m_nMaps;
	int map = MRTC_RAND() % NumMaps;
	map += 2;

	CStr Map = m_List[map];
	Map.GetStrSep(",");
	Map.Trim();
	pGameMod->m_pMPHandler->m_Map = Map;
#endif
}

int32 CMWnd_CubeMenu_Multiplayer_SelectMap::GetNumListItems(bool _Drawing)
{
	return m_List.Len();
}

bool CMWnd_CubeMenu_Multiplayer_SelectMap::GetListItem(int32 _Index, CStr& _Name, bool _bFocus)
{
	_Name = m_List[_Index].Str();

	if (_bFocus)	
		m_iSelected = _Index;

	return true;
}

void CMWnd_CubeMenu_Multiplayer_SelectMap::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);

	CRct Point = m_lButtons[m_iMap]->GetPosition();
	int TexID = pTC->GetTextureID("GUI_HUD_DARKNESS");	

	CVec2Dfp32 CoordScale = _pRCUtil->GetCoordinateScale();
	CXR_VBManager *pVBM = _pRCUtil->GetVBM();
	int ProjSize = m_CubeUser.m_pFrontEndMod->m_Cube.GetProjectionSize();
	Point.p0.x = TruncToInt(Point.p0.x * (ProjSize / 640.0f));
	Point.p0.y = TruncToInt(Point.p0.y * (ProjSize / 480.0f));

	int PicSizeMod = TruncToInt(30 * (ProjSize / 640.0f));

	CRect2Duint16 PictureScreenCoord;
	PictureScreenCoord.m_Min = CVec2Duint16(Point.p0.x - PicSizeMod, Point.p0.y);
	PictureScreenCoord.m_Max = CVec2Duint16(PictureScreenCoord.m_Min.k[0] + PicSizeMod, PictureScreenCoord.m_Min.k[1] + PicSizeMod);

	m_CubeUser.m_pFrontEndMod->m_Cube.AddProjImage(PictureScreenCoord, TexID);

	//Level pic
	int iMap = m_iSelected;
	if(iMap < 1 || iMap >= m_nMaps + 2)
		iMap = m_iMap;
	Point = m_lButtons[1]->GetPosition();	//Random Map button
	CStr MapPic = m_List[iMap];
	MapPic.GetStrSep(",");
	MapPic.Trim();
	MapPic = CStrF("GUI_%s", MapPic.Str());
	TexID = pTC->GetTextureID(MapPic.Str());	

	Point.p0.x = TruncToInt((Point.p0.x + 200) * (ProjSize / 640.0f));
	Point.p0.y = TruncToInt(Point.p0.y * (ProjSize / 480.0f));

	int PicSizeModX = TruncToInt(350 * (ProjSize / 640.0f));
	int PicSizeModY = TruncToInt(196 * (ProjSize / 640.0f));

	PictureScreenCoord.m_Min = CVec2Duint16(Point.p0.x, Point.p0.y);
	PictureScreenCoord.m_Max = CVec2Duint16(PictureScreenCoord.m_Min.k[0] + PicSizeModX, PictureScreenCoord.m_Min.k[1] + PicSizeModY);

	m_CubeUser.m_pFrontEndMod->m_Cube.AddProjImage(PictureScreenCoord, TexID);
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Multiplayer_SelectMap, CMWnd_CubeMenu);

// ---------------------------------------------------------------------------------------------

void CMWnd_CubeMenu_Multiplayer_SetupCharacter::OnCreate()
{
	CMWnd_CubeMenu::OnCreate();

	m_iSelected = -1;
	m_iModel = 2;
	m_bHumans = true;

	CreateList();
	m_TypePos.p0.x = 32;
	m_TypePos.p0.y = 48;
	m_TypePos.p1.x = 224;
	m_TypePos.p1.y = 72;
}

void CMWnd_CubeMenu_Multiplayer_SetupCharacter::CreateList(void)
{
	for(int32 y = 4; y < 16; y++)
		for(int32 x = 0; x < 20; x++)
		{
			m_aMap[y][x].m_Mode = CCellInfo::MODE_NORMAL;
			m_aMap[y][x].Cell() = CCellInfo::Empty();
			m_aMap[y][x].Flags() = 0;
		}

	m_List.Clear();
	SListItem Item;
/*	Item.DisplayName = "n, §LMENU_HUMANS";
	m_List.Add(Item);
	Item.DisplayName = "n, §LMENU_DARKLINGS";
	m_List.Add(Item);*/

	int State = m_CubeUser.m_pFrontEndMod->m_spMapData->GetState();
	m_CubeUser.m_pFrontEndMod->m_spMapData->SetState(State & ~WMAPDATA_STATE_NOCREATE);
	int iRc = m_CubeUser.m_pFrontEndMod->m_spMapData->GetResourceIndex_Registry("Registry\\Sv");
	CRegistry *pReg = m_CubeUser.m_pFrontEndMod->m_spMapData->GetResource_Registry(iRc);
	m_CubeUser.m_pFrontEndMod->m_spMapData->SetState(State);

	if(pReg)
	{
		int iSel = 0;
		CRegistry *pTemplates = pReg->FindChild("TEMPLATES");
		if(pTemplates)
		{
			for(int i = 0; i < pTemplates->GetNumChildren(); i++)
			{
				CRegistry *pChild = pTemplates->GetChild(i);
				CStr St = pChild->GetThisName();
				if(St.CompareSubStr("MULTIPLAYER") == 0)
				{
					if(St.Find("DARKLING") != -1)
					{
						if(!m_bHumans)
						{
							Item.TemplateName = St;
							Item.DisplayName = "s, " + pChild->GetValue("character_name");
							m_List.Add(Item);
						}
					}
					else if(m_bHumans)
					{
						Item.TemplateName = St;
						Item.DisplayName = "s, " + pChild->GetValue("character_name");
						m_List.Add(Item);
					}
				}
			}
		}
	}

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	CRegistry *pGlobalOpt = pSys->GetRegistry()->FindChild("OPT");
	CStr SelectedTemplate;
	if(m_bHumans)
		SelectedTemplate = pGlobalOpt->GetValue("MP_HUMANTEMPLATE", "multiplayer_Mob_01");
	else
		SelectedTemplate = pGlobalOpt->GetValue("MP_DARKLINGTEMPLATE", "multiplayer_Darkling_05");
	for(int i = 0; i < m_List.Len(); i++)
	{
		if(m_List[i].TemplateName.CompareNoCase(SelectedTemplate) == 0)
		{
			m_iModel = i;
		}
	}
}

bool CMWnd_CubeMenu_Multiplayer_SetupCharacter::OnButtonPress(CMWnd* _pFocus)
{
	MACRO_GetRegisterObject(CGameContext, pGameContext, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGameContext);

	if(_pFocus->m_ID.CompareNoCase("Human") == 0)
	{
		m_TypePos = _pFocus->GetPosition();
		m_bHumans = true;
		m_iFirstListItem = 0;
		CreateList();
	}
	else if(_pFocus->m_ID.CompareNoCase("Darkling") == 0)
	{
		m_TypePos = _pFocus->GetPosition();
		m_bHumans = false;
		m_iFirstListItem = 0;
		CreateList();
	}
	else
	{
		m_iModel = m_iSelected;
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		CRegistry *pGlobalOpt = pSys->GetRegistry()->FindChild("OPT");
		if(m_bHumans)
			pGlobalOpt->SetValue("MP_HUMANTEMPLATE", m_List[m_iModel].TemplateName);
		else
			pGlobalOpt->SetValue("MP_DARKLINGTEMPLATE", m_List[m_iModel].TemplateName);
	}
	

	return CMWnd_CubeMenu::OnButtonPress(_pFocus);
}

int32 CMWnd_CubeMenu_Multiplayer_SetupCharacter::GetNumListItems(bool _Drawing)
{
	return m_List.Len();
}

bool CMWnd_CubeMenu_Multiplayer_SetupCharacter::GetListItem(int32 _Index, CStr& _Name, bool _bFocus)
{
	_Name = m_List[_Index].DisplayName.Str();

	if (_bFocus)	
		m_iSelected = _Index;

	return true;
}

void CMWnd_CubeMenu_Multiplayer_SetupCharacter::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);

	CRct Point;
	int TexID = pTC->GetTextureID("GUI_HUD_DARKNESS");	

	CVec2Dfp32 CoordScale = _pRCUtil->GetCoordinateScale();
	CXR_VBManager *pVBM = _pRCUtil->GetVBM();
	int ProjSize = m_CubeUser.m_pFrontEndMod->m_Cube.GetProjectionSize();
	Point.p0.x = TruncToInt(m_TypePos.p0.x * (ProjSize / 640.0f));
	Point.p0.y = TruncToInt(m_TypePos.p0.y * (ProjSize / 480.0f));

	int PicSizeMod = TruncToInt(30 * (ProjSize / 640.0f));

	CRect2Duint16 PictureScreenCoord;
	PictureScreenCoord.m_Min = CVec2Duint16(Point.p0.x - PicSizeMod, Point.p0.y);
	PictureScreenCoord.m_Max = CVec2Duint16(PictureScreenCoord.m_Min.k[0] + PicSizeMod, PictureScreenCoord.m_Min.k[1] + PicSizeMod);

	m_CubeUser.m_pFrontEndMod->m_Cube.AddProjImage(PictureScreenCoord, TexID);

	int iIndex = m_iModel - m_iFirstListItem;
	if(m_iFirstListItem > 0 && iIndex == 0)
		iIndex = -1;
	if(iIndex >= m_lButtons.Len() || (iIndex == (m_lButtons.Len() - 1) && (m_List.Len() - m_iFirstListItem != m_lButtons.Len())))
		iIndex = -1;
	if(iIndex >= 0)
	{
		Point = m_lButtons[iIndex]->GetPosition();
		TexID = pTC->GetTextureID("GUI_HUD_DARKNESS");	

		Point.p0.x = TruncToInt(Point.p0.x * (ProjSize / 640.0f));
		Point.p0.y = TruncToInt(Point.p0.y * (ProjSize / 480.0f));

		PictureScreenCoord.m_Min = CVec2Duint16(Point.p0.x - PicSizeMod, Point.p0.y);
		PictureScreenCoord.m_Max = CVec2Duint16(PictureScreenCoord.m_Min.k[0] + PicSizeMod, PictureScreenCoord.m_Min.k[1] + PicSizeMod);

		m_CubeUser.m_pFrontEndMod->m_Cube.AddProjImage(PictureScreenCoord, TexID);
	}
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Multiplayer_SetupCharacter, CMWnd_CubeMenu);

// ---------------------------------------------------------------------------------------------

CMWnd_CubeMenu_Multiplayer_SelectServerList::CMWnd_CubeMenu_Multiplayer_SelectServerList()
{
	m_LastUpdate = CMTime::GetCPU();
}

void CMWnd_CubeMenu_Multiplayer_SelectServerList::OnCreate()
{
	CMWnd_CubeMenu::OnCreate();

	m_iSelected = -1;
}

void CMWnd_CubeMenu_Multiplayer_SelectServerList::OnActivate()
{
	m_bNoServers = true;
	m_List.Clear();
	m_ActivateTime = CMTime::GetCPU();
}

bool CMWnd_CubeMenu_Multiplayer_SelectServerList::OnButtonPress(CMWnd* _pFocus)
{
	if (m_iSelected >= 0)
	{
		CMTime compare = m_ActivateTime + CMTime::CreateFromSeconds(1.5f);
		CMTime cpu = CMTime::GetCPU();
		if(compare.GetTime() > cpu.GetTime())
			return CMWnd_CubeMenu::OnButtonPress(_pFocus);

#if defined(PLATFORM_CONSOLE)
		MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
		CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
		if(!m_bNoServers)
			pGameMod->m_pMPHandler->JoinSession(m_iSelected);
#endif

		return true;
	}
	return CMWnd_CubeMenu::OnButtonPress(_pFocus);
}

int32 CMWnd_CubeMenu_Multiplayer_SelectServerList::GetNumListItems(bool _Drawing)
{
	return m_List.Len();
}

bool CMWnd_CubeMenu_Multiplayer_SelectServerList::GetListItem(int32 _Index, CStr& _Name, bool _bFocus)
{
#ifdef PLATFORM_XENON
	_Name = CStrF(WTEXT("s, %s"), m_List[_Index].StrW());
#endif
#ifdef PLATFORM_PS3
	_Name = CStrF("s, %s", m_List[_Index].Str());
#endif

	if (_bFocus)	
		m_iSelected = _Index;

	return true;
}

void CMWnd_CubeMenu_Multiplayer_SelectServerList::OnRefresh()
{
#if defined(PLATFORM_CONSOLE)
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
	if(pGameMod)
	{
		CMTime compare = m_LastUpdate + CMTime::CreateFromSeconds(1.0f);
		CMTime cpu_time = CMTime::GetCPU();
		if((compare.GetTime() < cpu_time.GetTime()))
		{
			m_List.Clear();
			for(uint8 i = 0; i < pGameMod->m_pMPHandler->GetNumLanServers(); i++)
			{
				CStr server = pGameMod->m_pMPHandler->GetLanServerInfo(i);
				if(server.Len())
					m_List.Add(server);
			}
			if(!m_List.Len())
			{
#ifdef PLATFORM_XENON
				m_List.Add(WTEXT("No server found"));
#endif
#ifdef PLATFORM_PS3
				m_List.Add("No server found");
#endif
				m_bNoServers = true;
			}
			else
				m_bNoServers = false;
			m_LastUpdate = CMTime::GetCPU();
			pGameMod->m_pMPHandler->Broadcast();
		}
	}
#endif
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Multiplayer_SelectServerList, CMWnd_CubeMenu);

// ---------------------------------------------------------------------------------------------

CMWnd_CubeMenu_Multiplayer_PostGameLobby::CMWnd_CubeMenu_Multiplayer_PostGameLobby()
{

}

bool CMWnd_CubeMenu_Multiplayer_PostGameLobby::OnButtonPress(CMWnd* _pFocus)
{
#if defined(PLATFORM_CONSOLE)
		MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
		CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

		pGameMod->m_pMPHandler->ShowGamerCard(m_List[m_iSelected].PlayerID);

		return true;
#endif
	return CMWnd_CubeMenu::OnButtonPress(_pFocus);
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Multiplayer_PostGameLobby, CMWnd_CubeMenu);

// ---------------------------------------------------------------------------------------------

CMWnd_CubeMenu_Multiplayer_StartGame::CMWnd_CubeMenu_Multiplayer_StartGame()
{
	m_LastNumberOfPlayers = 0;
}

void CMWnd_CubeMenu_Multiplayer_StartGame::OnCreate()
{
	CMWnd_CubeMenu::OnCreate();

	m_iSelected = -1;
}

bool CMWnd_CubeMenu_Multiplayer_StartGame::OnButtonPress(CMWnd* _pFocus)
{
#if defined(PLATFORM_CONSOLE)
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
	pGameMod->m_pMPHandler->ShowGamerCard(m_List[m_iSelected].PlayerID);

	return true;
#endif
	return CMWnd_CubeMenu::OnButtonPress(_pFocus);
}

int32 CMWnd_CubeMenu_Multiplayer_StartGame::GetNumListItems(bool _Drawing)
{
	return m_List.Len();
}

bool CMWnd_CubeMenu_Multiplayer_StartGame::GetListItem(int32 _Index, CStr& _Name, bool _bFocus)
{
#ifdef PLATFORM_XENON
	_Name = CStrF(WTEXT("s, %s"), m_List[_Index].Name.StrW());
#else
	_Name = CStrF("s, %s", m_List[_Index].Name.Str());
#endif

	if (_bFocus)	
		m_iSelected = _Index;

	return true;
}

void CMWnd_CubeMenu_Multiplayer_StartGame::OnRefresh()
{
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
#if defined(PLATFORM_CONSOLE)
	m_List.Clear();

	SListItem item;
	CStr Voice, Ready;

	for(int i = 0; i < pGameMod->m_pMPHandler->GetNumPlayers(); i++)
	{
		int ID = pGameMod->m_pMPHandler->GetPlayerID(i);
		if(ID)
		{
			SListItem item;
			item.PlayerID = ID;
			int PlayerStatus = pGameMod->m_pMPHandler->GetPlayerLobbyStatus(i);

#ifdef PLATFORM_XENON
			item.Name = CStrF(WTEXT(" %s"), pGameMod->m_pMPHandler->GetPlayerName(i).StrW());
#else 
			item.Name = CStrF(" %s", pGameMod->m_pMPHandler->GetPlayerName(i).Str());
#endif
			if(PlayerStatus & LOBBY_PLAYER_READY)
				item.bIsReady = true;
			else
				item.bIsReady = false;
			if(PlayerStatus & LOBBY_PLAYER_TALKING)
				item.bIsTalking = true;
			else
				item.bIsTalking = false;
			m_List.Add(item);
		}
	}

	item.PlayerID = pGameMod->m_pMPHandler->GetPlayerID(-1);
	int PlayerStatus = pGameMod->m_pMPHandler->GetPlayerLobbyStatus(-1);

#ifdef PLATFORM_XENON
	item.Name = CStrF(WTEXT("%s"), pGameMod->m_pMPHandler->GetPlayerName(-1).StrW());
#else 
	item.Name = CStrF("%s", pGameMod->m_pMPHandler->GetPlayerName(-1).Str());
#endif
	if(PlayerStatus & LOBBY_PLAYER_READY)
		item.bIsReady = true;
	else
		item.bIsReady = false;
	if(PlayerStatus & LOBBY_PLAYER_TALKING)
		item.bIsTalking = true;
	else
		item.bIsTalking = false;
	m_List.Add(item);
#endif
}

void CMWnd_CubeMenu_Multiplayer_StartGame::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);
	int TexIDTalking = pTC->GetTextureID("GUI_speakeron");	
	int TexIDNotTalking = pTC->GetTextureID("GUI_speakeroff");	

	CVec2Dfp32 CoordScale = _pRCUtil->GetCoordinateScale();
	CXR_VBManager *pVBM = _pRCUtil->GetVBM();
	int ProjSize = m_CubeUser.m_pFrontEndMod->m_Cube.GetProjectionSize();
	int PicSizeMod = TruncToInt(30 * (ProjSize / 640.0f));
	CRC_Font *pFont = GetFont("HEADINGS");

	for(int i = 0; i < m_List.Len(); i++)
	{
		CRct Point = m_lButtons[i]->GetPosition();
		Point.p0.x = TruncToInt(Point.p0.x * (ProjSize / 640.0f));
		Point.p0.y = TruncToInt(Point.p0.y * (ProjSize / 480.0f));

		CRect2Duint16 PictureScreenCoord;
		PictureScreenCoord.m_Min = CVec2Duint16(Point.p0.x - PicSizeMod, Point.p0.y);
		PictureScreenCoord.m_Max = CVec2Duint16(PictureScreenCoord.m_Min.k[0] + PicSizeMod, PictureScreenCoord.m_Min.k[1] + PicSizeMod);

		int TexID = m_List[i].bIsTalking ? TexIDTalking : TexIDNotTalking;

		CRct PosView = m_lButtons[i]->GetPosition();
		PosView.p0.x += 430;
		PosView.p1.x += 430;
		CRct PosCube = Layout_Viewspace2CubeSpace(PosView);
		
		if(m_List[i].bIsReady)
			Layout_WriteText(PosCube, "s, [Ready]", FLAG_NO_CONVERT|FLAG_NO_WRAP|FLAG_FASTSWITCH); 
		else
			Layout_WriteText(PosCube, "s, [Waiting]", FLAG_NO_CONVERT|FLAG_NO_WRAP|FLAG_FASTSWITCH); 

		m_CubeUser.m_pFrontEndMod->m_Cube.AddProjImage(PictureScreenCoord, TexID);
	}
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Multiplayer_StartGame, CMWnd_CubeMenu);

// ---------------------------------------------------------------------------------------------

void CMWnd_CubeMenu_Multiplayer_SelectGameMode::OnCreate()
{
	CMWnd_CubeMenu::OnCreate();
	ConExecute(CStr("setgameclass(\"DM\"); mp_setgamemode(\"0\")"));

	m_iSelected = -1;

	m_iModeSelected = 2;
	m_iRuleSelected = 6;

	m_List.Add("nc, §LMENU_ADVANCED_SETTINGS");
	m_List.Add("nc, §LMENU_NEXT");
	m_List.Add("s, §LMENU_SHAPESHIFTER");
	m_List.Add("s, §LMENU_DARKLINGS_VS_DARKLINGS");
	m_List.Add("s, §LMENU_SURVIVOR");
	m_List.Add("s, §LMENU_DARKLINGS_VS_HUMANS");
	m_List.Add("s, §LMENU_DEATHMATCH");
	m_List.Add("s, §LMENU_TEAMDEATHMATCH");
	m_List.Add("s, §LMENU_CAPTURETHEFLAG");
	m_List.Add("s, §LMENU_SURVIVOR");
	m_List.Add("s, §LMENU_LASTHUMAN");

	m_lButtonsRemoved.Add(9);
	m_lButtonsRemoved.Add(10);

/*	CMapData& MapData = *m_CubeUser.m_pFrontEndMod->m_spMapData;
	TArray_SimpleSortable<CStr> lFiles = 
		MapData.ResolveDirectory("Worlds\\MP*.XW", false);	

	m_nNonCTFMaps = lFiles.Len();
	m_nMaps = m_nNonCTFMaps;

	for (uint16 i=0; i<lFiles.Len(); i++)
		m_List.Add( lFiles[i].GetFilenameNoExt() );

	TArray_SimpleSortable<CStr> lFiles2 = 
		MapData.ResolveDirectory("Worlds\\CTF*.XW", false);	

	m_nMaps += lFiles2.Len();

	for (uint16 i=0; i<lFiles2.Len(); i++)
		m_List.Add(lFiles2[i].GetFilenameNoExt());

#ifdef PLATFORM_CONSOLE
	RandomMap();
#else
	m_Map = m_List[13];
#endif*/
}

bool CMWnd_CubeMenu_Multiplayer_SelectGameMode::OnButtonPress(CMWnd* _pFocus)
{
	CStr cmd;
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

	switch(m_iSelected) 
	{
	case 0:
#ifdef PLATFORM_CONSOLE
		if(pGameMod)
		{
			switch(pGameMod->m_pMPHandler->m_GameMode) 
			{
			case CONTEXT_GAME_MODE_FREE_FOR_ALL:
				cmd = CStr("cg_submenu(\"Multiplayer_AdvancedSettings\")");
				break;
			case CONTEXT_GAME_MODE_TEAM_GAMES:
				cmd = CStr("cg_submenu(\"Multiplayer_AdvancedSettingsCTF\")");
				break;
			}
		}
#endif
		break;
	case 1:
		cmd = CStr("cg_submenu(\"Multiplayer_SelectMap\")");
		break;
	case 2:
		cmd = CStr("mp_setgamemode(\"0\")");
		m_lButtons[m_iSelected]->KillFocus();
		m_lButtons[6]->SetFocus();
		m_iModeSelected = m_iSelected;
		m_lButtonsRemoved.Clear();
		m_lButtonsRemoved.Add(9);
		m_lButtonsRemoved.Add(10);
		ValidateRule();
		break;
	case 3:
		cmd = CStr("mp_setgamemode(\"1\")");
		m_lButtons[m_iSelected]->KillFocus();
		m_lButtons[6]->SetFocus();
		m_iModeSelected = m_iSelected;
		m_lButtonsRemoved.Clear();
		m_lButtonsRemoved.Add(9);
		m_lButtonsRemoved.Add(10);
		ValidateRule();
		break;
	case 4:
		cmd = CStr("mp_setgamemode(\"0\")");
		m_lButtons[m_iSelected]->KillFocus();
		m_lButtons[6]->SetFocus();
		m_iModeSelected = m_iSelected;
		m_lButtonsRemoved.Clear();
		m_lButtonsRemoved.Add(6);
		m_lButtonsRemoved.Add(7);
		m_lButtonsRemoved.Add(8);
		ValidateRule();
		break;
	case 5:
		cmd = CStr("mp_setgamemode(\"2\")");
		m_lButtons[m_iSelected]->KillFocus();
		m_lButtons[6]->SetFocus();
		m_iModeSelected = m_iSelected;
		m_lButtonsRemoved.Clear();
		m_lButtonsRemoved.Add(6);
		m_lButtonsRemoved.Add(7);
		m_lButtonsRemoved.Add(9);
		m_lButtonsRemoved.Add(10);
		ValidateRule();
		break;
	case 6:
		cmd = CStr("setgameclass(\"DM\");");
		m_lButtons[m_iSelected]->KillFocus();
		m_lButtons[0]->SetFocus();
		m_iRuleSelected = m_iSelected;
		break;
	case 7:
		cmd = CStr("setgameclass(\"TDM\");");
		m_lButtons[m_iSelected]->KillFocus();
		m_lButtons[0]->SetFocus();
		m_iRuleSelected = m_iSelected;
		break;
	case 8:
		cmd = CStr("setgameclass(\"CTF\");");
		m_lButtons[m_iSelected]->KillFocus();
		m_lButtons[0]->SetFocus();
		m_iRuleSelected = m_iSelected;
		break;
	case 9:
		cmd = CStr("setgameclass(\"Survivor\");");
		m_lButtons[m_iSelected]->KillFocus();
		m_lButtons[0]->SetFocus();
		m_iRuleSelected = m_iSelected;
		break;
	case 10:
		cmd = CStr("setgameclass(\"LastHuman\");");
		m_lButtons[m_iSelected]->KillFocus();
		m_lButtons[0]->SetFocus();
		m_iRuleSelected = m_iSelected;
		break;
	}

	ConExecute(cmd);
	return true;
}

void CMWnd_CubeMenu_Multiplayer_SelectGameMode::ValidateRule()
{
	switch(m_iModeSelected)
	{
	case 2:	//Shapeshift
	case 3:	//Darklings vs darklings
		if(m_iRuleSelected > 8)
		{
			ConExecute("setgameclass(\"DM\");");
			m_iRuleSelected = 6;
		}
		break;
	case 4:	//Survivor
		if(m_iRuleSelected < 9)
		{
			ConExecute("setgameclass(\"Survivor\");");
			m_iRuleSelected = 9;
		}
		break;
	case 5:	//Darklings vs Humans
		if(m_iRuleSelected != 8)
		{
			ConExecute("setgameclass(\"CTF\");");
			m_iRuleSelected = 8;
		}
		break;
	}
}

int32 CMWnd_CubeMenu_Multiplayer_SelectGameMode::GetNumListItems(bool _Drawing)
{
	return m_List.Len() - m_lButtonsRemoved.Len();
}

bool CMWnd_CubeMenu_Multiplayer_SelectGameMode::GetListItem(int32 _Index, CStr& _Name, bool _bFocus)
{
	int Index = _Index;

	for(int i = 0; i < m_lButtonsRemoved.Len(); i++)
	{
		if(m_lButtonsRemoved[i] <= Index)
			Index++;
	}

	_Name = m_List[Index].Str();

	if (_bFocus)	
		m_iSelected = Index;

	return true;
}

void CMWnd_CubeMenu_Multiplayer_SelectGameMode::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);

	CRct Point = m_lButtons[m_iModeSelected]->GetPosition();
	int TexID = pTC->GetTextureID("GUI_HUD_DARKNESS");	

	CVec2Dfp32 CoordScale = _pRCUtil->GetCoordinateScale();
	CXR_VBManager *pVBM = _pRCUtil->GetVBM();
	int ProjSize = m_CubeUser.m_pFrontEndMod->m_Cube.GetProjectionSize();
	Point.p0.x = TruncToInt(Point.p0.x * (ProjSize / 640.0f));
	Point.p0.y = TruncToInt(Point.p0.y * (ProjSize / 480.0f));

	int PicSizeMod = TruncToInt(30 * (ProjSize / 640.0f));

	CRect2Duint16 PictureScreenCoord;
	PictureScreenCoord.m_Min = CVec2Duint16(Point.p0.x - PicSizeMod, Point.p0.y);
	PictureScreenCoord.m_Max = CVec2Duint16(PictureScreenCoord.m_Min.k[0] + PicSizeMod, PictureScreenCoord.m_Min.k[1] + PicSizeMod);

	m_CubeUser.m_pFrontEndMod->m_Cube.AddProjImage(PictureScreenCoord, TexID);

	int iRule = m_iRuleSelected;
	for(int i = 0; i < m_lButtonsRemoved.Len(); i++)
	{
		if(m_lButtonsRemoved[i] < m_iRuleSelected)
			iRule--;
	}

	Point = m_lButtons[iRule]->GetPosition();

	Point.p0.x = TruncToInt(Point.p0.x * (ProjSize / 640.0f));
	Point.p0.y = TruncToInt(Point.p0.y * (ProjSize / 480.0f));

	PictureScreenCoord.m_Min = CVec2Duint16(Point.p0.x - PicSizeMod, Point.p0.y);
	PictureScreenCoord.m_Max = CVec2Duint16(PictureScreenCoord.m_Min.k[0] + PicSizeMod, PictureScreenCoord.m_Min.k[1] + PicSizeMod);

	m_CubeUser.m_pFrontEndMod->m_Cube.AddProjImage(PictureScreenCoord, TexID);
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Multiplayer_SelectGameMode, CMWnd_CubeMenu);

// ---------------------------------------------------------------------------------------------

void CMWnd_CubeMenu_Multiplayer_Search::OnCreate()
{
	CMWnd_CubeMenu::OnCreate();

	m_iSelected = -1;

	m_List.Add("Deathmatch");
	m_List.Add("Team Deathmatch");
	m_List.Add("Capture The Flag");

	m_List.Add("Deathmatch");
	m_List.Add("Team Deathmatch");
	m_List.Add("Capture The Flag");

	m_List.Add("Yes");
	m_List.Add("No");

	m_List.Add("Search");
}

bool CMWnd_CubeMenu_Multiplayer_Search::OnButtonPress(CMWnd* _pFocus)
{
#if defined(PLATFORM_CONSOLE)
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

	switch(m_iSelected) 
	{
	case 0:
		pGameMod->m_pMPHandler->m_GameMode = CONTEXT_GAME_MODE_FREE_FOR_ALL;
		pGameMod->m_pMPHandler->m_GameStyle = CONTEXT_GAME_STYLE_SHAPESHIFTER;
		m_lButtons[0]->KillFocus();
		m_lButtons[6]->SetFocus();
		break;
	case 1:
		pGameMod->m_pMPHandler->m_GameStyle = CONTEXT_GAME_STYLE_SHAPESHIFTER;
		pGameMod->m_pMPHandler->m_GameMode = CONTEXT_GAME_MODE_TEAM_GAMES;
		pGameMod->m_pMPHandler->m_GameSubMode = CONTEXT_GAME_SUB_MODE_TEAM_DEATHMATCH;
		m_lButtons[1]->KillFocus();
		m_lButtons[6]->SetFocus();
		break;
	case 2:
		pGameMod->m_pMPHandler->m_GameStyle = CONTEXT_GAME_STYLE_SHAPESHIFTER;
		pGameMod->m_pMPHandler->m_GameMode = CONTEXT_GAME_MODE_TEAM_GAMES;
		pGameMod->m_pMPHandler->m_GameSubMode = CONTEXT_GAME_SUB_MODE_CAPTURE_THE_FLAG;
		m_lButtons[2]->KillFocus();
		m_lButtons[6]->SetFocus();
		break;
	case 3:
		pGameMod->m_pMPHandler->m_GameStyle = CONTEXT_GAME_STYLE_DARKLINGS_VS_DARKLINGS;
		pGameMod->m_pMPHandler->m_GameMode = CONTEXT_GAME_MODE_FREE_FOR_ALL;
		m_lButtons[3]->KillFocus();
		m_lButtons[6]->SetFocus();
		break;
	case 4:
		pGameMod->m_pMPHandler->m_GameStyle = CONTEXT_GAME_STYLE_DARKLINGS_VS_DARKLINGS;
		pGameMod->m_pMPHandler->m_GameMode = CONTEXT_GAME_MODE_TEAM_GAMES;
		pGameMod->m_pMPHandler->m_GameSubMode = CONTEXT_GAME_SUB_MODE_TEAM_DEATHMATCH;
		m_lButtons[4]->KillFocus();
		m_lButtons[6]->SetFocus();
		break;
	case 5:
		pGameMod->m_pMPHandler->m_GameStyle = CONTEXT_GAME_STYLE_DARKLINGS_VS_DARKLINGS;
		pGameMod->m_pMPHandler->m_GameMode = CONTEXT_GAME_MODE_TEAM_GAMES;
		pGameMod->m_pMPHandler->m_GameSubMode = CONTEXT_GAME_SUB_MODE_CAPTURE_THE_FLAG;
		m_lButtons[5]->KillFocus();
		m_lButtons[6]->SetFocus();
		break;
	case 6:
		//pGameMod->m_pMPHandler->m_GameType = X_CONTEXT_GAME_TYPE_RANKED;
		ConExecute("mp_ranked(1)");
		m_lButtons[6]->KillFocus();
		m_lButtons[8]->SetFocus();
		break;
	case 7:
		//pGameMod->m_pMPHandler->m_GameType = X_CONTEXT_GAME_TYPE_STANDARD;
		ConExecute("mp_ranked(0)");
		m_lButtons[7]->KillFocus();
		m_lButtons[8]->SetFocus();
		break;
	case 8:
		ConExecute(CStr("mp_searchforsessions()"));
		break;
	}
#endif
	//	ConExecute("cg_submenu(\"Multiplayer_SelectMap\")");
	return true;
}

int32 CMWnd_CubeMenu_Multiplayer_Search::GetNumListItems(bool _Drawing)
{
	return m_List.Len();
}

bool CMWnd_CubeMenu_Multiplayer_Search::GetListItem(int32 _Index, CStr& _Name, bool _bFocus)
{
	_Name = CStrF("s, %s", m_List[_Index].Str());

	if (_bFocus)	
		m_iSelected = _Index;

	return true;
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Multiplayer_Search, CMWnd_CubeMenu);

// --------------------------------------------------------------------------------------------

bool CMWnd_CubeMenu_Multiplayer_AdvancedSettings::OnButtonPress(CMWnd* _pFocus)
{
	return true;
}


MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Multiplayer_AdvancedSettings, CMWnd_CubeMenu);

// ---------------------------------------------------------------------------------------------

void CMWnd_CubeMenu_Multiplayer_Leaderboard::OnCreate()
{
	CMWnd_CubeMenu::OnCreate();
	m_bFriends = false;

#ifdef PLATFORM_XENON
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

	SListItem Item;
	Item.Name = WTEXT("Retrieving leaderboard");
	Item.PlayerID = 0;

	m_List.Add(Item);

#endif
}

bool CMWnd_CubeMenu_Multiplayer_Leaderboard::OnButtonPress(CMWnd* _pFocus)
{
	if (m_iSelected >= 1)
	{
#if defined(PLATFORM_CONSOLE)
		MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
		CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
		//TODO
		//Fix this when fixing PN
/*		BOOL result;
		XUserCheckPrivilege(pGameMod->m_pMPHandler->m_iPad, XPRIVILEGE_PROFILE_VIEWING, &result);
		if(!result)
		{
			XUserCheckPrivilege(pGameMod->m_pMPHandler->m_iPad, XPRIVILEGE_PROFILE_VIEWING_FRIENDS_ONLY, &result);
			if(result)
			{
				BOOL Friend;
				XUserAreUsersFriends(pGameMod->m_pMPHandler->m_iPad, &m_List[m_iSelected].Xuid, 1, &Friend, NULL);
				if(Friend)
					result = TRUE;
				else
					result = FALSE;
			}
		}
		if(result)*/
			pGameMod->m_pMPHandler->ShowGamerCard(m_List[m_iSelected].PlayerID);

		return true;
#endif
	}
	return CMWnd_CubeMenu::OnButtonPress(_pFocus);
}

bool CMWnd_CubeMenu_Multiplayer_Leaderboard::OnOtherButtonPress(CMWnd *_pFocus, int32 _Button)
{
	if (m_iSelected >= 0)
	{
#if defined(PLATFORM_CONSOLE)
		MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
		CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

		if(_Button == SKEY_GUI_BUTTON3)
		{
			if(!m_bFriends)
			{
				pGameMod->m_pMPHandler->CleanupStats();
				pGameMod->m_pMPHandler->ReadLeaderBoardFriends();
				m_bFriends = true;
			}
			else
			{
				pGameMod->m_pMPHandler->CleanupStats();
				pGameMod->m_pMPHandler->ReadLeaderBoard(-1);
				m_bFriends = false;
			}
			return true;
		}
		else if(_Button == SKEY_GUI_BUTTON2)
		{
			pGameMod->m_pMPHandler->CleanupStats();
			pGameMod->m_pMPHandler->ReadLeaderBoard(-1, pGameMod->m_pMPHandler->m_Local.m_player_id);
			m_bFriends = false; 
			return true;
		}
		return CMWnd_CubeMenu::OnOtherButtonPress(_pFocus, _Button);
#endif
	}
	return CMWnd_CubeMenu::OnOtherButtonPress(_pFocus, _Button);
}

int32 CMWnd_CubeMenu_Multiplayer_Leaderboard::GetNumListItems(bool _Drawing)
{
	return m_List.Len();
}

bool CMWnd_CubeMenu_Multiplayer_Leaderboard::GetListItem(int32 _Index, CStr& _Name, bool _bFocus)
{
	_Name = CStrF(WTEXT("s, %s"), m_List[_Index].Name.StrW());

	if (_bFocus)	
		m_iSelected = _Index;

	return true;
}

void CMWnd_CubeMenu_Multiplayer_Leaderboard::OnRefresh()
{
#ifdef PLATFORM_XENON
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
	CWGameLiveHandler *pLive = (CWGameLiveHandler*) pGameMod->m_pMPHandler;
	if(pLive->m_bUpdateLeaderBoard)
	{
		m_List.Clear();
		if(pLive->m_pStats && pLive->m_pStats->dwNumViews)
		{
			CStr Header;
			switch(pLive->m_pStats->pViews[0].dwViewId) 
			{
			case STATS_VIEW_SKILL_RANKED_TEAM_GAMES:        
			case STATS_VIEW_SKILL_RANKED_FREE_FOR_ALL:
			case STATS_VIEW_SKILL_STANDARD_FREE_FOR_ALL:
			case STATS_VIEW_SKILL_STANDARD_TEAM_GAMES:
			case STATS_VIEW_SURVIVOR:
			case STATS_VIEW_LASTHUMAN:
				Header = WTEXT("R - GP - Name");
				break;
			case STATS_VIEW_DEATHMATCH_DARKLINGS:             
			case STATS_VIEW_TEAMDEATHMATCH_DARKLINGS:         
			case STATS_VIEW_DEATHMATCH_SHAPESHIFTER:
			case STATS_VIEW_TEAMDEATHMATCH_SHAPESHIFTER:      
				Header = WTEXT("R - S - D - Name");
				break;
			case STATS_VIEW_CAPTURETHEFLAG_DARKLINGS: 
			case STATS_VIEW_CAPTURETHEFLAG_SHAPESHIFTER:
			case STATS_VIEW_CAPTURETHEFLAG_DVH:
				Header = WTEXT("R - S - D - C - Name");
				break;
			}

			SListItem Item;
			Item.Name = Header;
			Item.PlayerID = 0;

			m_List.Add(Item);

			if(!m_bFriends)
			{
				for(uint16 i = 0; i < pLive->m_pStats->pViews[0].dwNumRows; i++)
				{
					XUSER_STATS_VIEW *view = &pLive->m_pStats->pViews[0];

					if(view->pRows[i].dwRank == 0)
						continue;
					CStr St;
					WCHAR name[XUSER_NAME_SIZE];
					MultiByteToWideChar(CP_ACP, 0, view->pRows[i].szGamertag, -1, name, XUSER_NAME_SIZE);

					switch(pLive->m_pStats->pViews[0].dwViewId) 
					{
					case STATS_VIEW_SKILL_RANKED_TEAM_GAMES:        
					case STATS_VIEW_SKILL_RANKED_FREE_FOR_ALL:
					case STATS_VIEW_SKILL_STANDARD_FREE_FOR_ALL:
					case STATS_VIEW_SKILL_STANDARD_TEAM_GAMES:
					case STATS_VIEW_SURVIVOR:
					case STATS_VIEW_LASTHUMAN:
						Item.Name = CStrF(WTEXT("%i - %i - %s"), view->pRows[i].dwRank, view->pRows[i].pColumns[1].Value.i64Data, name);
						break;
					case STATS_VIEW_DEATHMATCH_DARKLINGS:             
					case STATS_VIEW_TEAMDEATHMATCH_DARKLINGS:         
					case STATS_VIEW_DEATHMATCH_SHAPESHIFTER:          
					case STATS_VIEW_TEAMDEATHMATCH_SHAPESHIFTER:      
						Item.Name = CStrF(WTEXT("%i - %i - %i - %s"), view->pRows[i].dwRank, view->pRows[i].i64Rating, view->pRows[i].pColumns[0].Value.nData, name);
						break;
					case STATS_VIEW_CAPTURETHEFLAG_DARKLINGS: 
					case STATS_VIEW_CAPTURETHEFLAG_SHAPESHIFTER:
					case STATS_VIEW_CAPTURETHEFLAG_DVH:
						Item.Name = CStrF(WTEXT("%i - %i - %i - %i - %s"), view->pRows[i].dwRank, view->pRows[i].i64Rating, view->pRows[i].pColumns[0].Value.nData, view->pRows[i].pColumns[1].Value.nData, name);
						break;
					}
					int player_id = pLive->SetXuid(view->pRows[i].xuid, 0, true);
					Item.PlayerID = player_id;
					m_List.Add(Item);
				}
			}
			else
			{
				for(int16 i = pLive->m_liFriendStats.Len() - 1; i > -1 ; i--)
				{
					XUSER_STATS_VIEW *view = &pLive->m_pStats->pViews[0];

					CStr St;
					WCHAR name[XUSER_NAME_SIZE];
					MultiByteToWideChar(CP_ACP, 0, view->pRows[pLive->m_liFriendStats[i].m_Index].szGamertag, -1, name, XUSER_NAME_SIZE);

					switch(pLive->m_pStats->pViews[0].dwViewId) 
					{
					case STATS_VIEW_SKILL_RANKED_FREE_FOR_ALL:        
					case STATS_VIEW_SKILL_RANKED_TEAM_GAMES:
					case STATS_VIEW_SKILL_STANDARD_FREE_FOR_ALL:
					case STATS_VIEW_SKILL_STANDARD_TEAM_GAMES:
					case STATS_VIEW_SURVIVOR:
					case STATS_VIEW_LASTHUMAN:
						Item.Name = CStrF(WTEXT("%i - %i - %s"), view->pRows[pLive->m_liFriendStats[i].m_Index].dwRank, view->pRows[pLive->m_liFriendStats[i].m_Index].pColumns[1].Value.i64Data, name);
						break;
					case STATS_VIEW_DEATHMATCH_DARKLINGS:             
					case STATS_VIEW_TEAMDEATHMATCH_DARKLINGS:         
					case STATS_VIEW_DEATHMATCH_SHAPESHIFTER:          
					case STATS_VIEW_TEAMDEATHMATCH_SHAPESHIFTER:      
						Item.Name = CStrF(WTEXT("%i - %i - %i - %s"), view->pRows[pLive->m_liFriendStats[i].m_Index].dwRank, view->pRows[pLive->m_liFriendStats[i].m_Index].i64Rating, view->pRows[pLive->m_liFriendStats[i].m_Index].pColumns[0].Value.nData, name);
						break;
					case STATS_VIEW_CAPTURETHEFLAG_DARKLINGS: 
					case STATS_VIEW_CAPTURETHEFLAG_SHAPESHIFTER:
					case STATS_VIEW_CAPTURETHEFLAG_DVH:
						Item.Name = CStrF(WTEXT("%i - %i - %i - %i - %s"), view->pRows[pLive->m_liFriendStats[i].m_Index].dwRank, view->pRows[pLive->m_liFriendStats[i].m_Index].i64Rating, view->pRows[pLive->m_liFriendStats[i].m_Index].pColumns[0].Value.nData, view->pRows[pLive->m_liFriendStats[i].m_Index].pColumns[1].Value.nData, name);
						break;
					}
					int player_id = pLive->SetXuid(view->pRows[pLive->m_liFriendStats[i].m_Index].xuid, 0, true);
					Item.PlayerID = player_id;
					m_List.Add(Item);
				}
			}
			pLive->m_bUpdateLeaderBoard = false;
			if(m_List.Len() == 1)
			{
				SListItem Item;
				Item.Name = WTEXT("No entries"); 
				Item.PlayerID = 0;
				m_List.Add(Item);
			}
		}
	}
#endif
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Multiplayer_Leaderboard, CMWnd_CubeMenu);

// ---------------------------------------------------------------------------------------------
void CMWnd_CubeMenu_Demos::OnCreate()
{
	CMWnd_CubeMenu::OnCreate();

	m_iSelected = -1;

	CMapData& MapData = *m_CubeUser.m_pFrontEndMod->m_spMapData;
	TArray_SimpleSortable<CStr> lFiles = 
		MapData.ResolveDirectory("Demos\\*.xdm", false);

	for (int i=0; i<lFiles.Len(); i++)
		m_List.Add( lFiles[i].GetFilenameNoExt() );
}

bool CMWnd_CubeMenu_Demos::OnButtonPress(CMWnd* _pFocus)
{
	if (m_iSelected >= 0)
	{
		CStr cmd = CFStrF(m_Execute.Str(), m_List[m_iSelected].Str(), m_List[m_iSelected].Str());
		ConExecute(cmd);
		return true;
	}
	return CMWnd_CubeMenu::OnButtonPress(_pFocus);
}

int32 CMWnd_CubeMenu_Demos::GetNumListItems(bool _Drawing)
{
	return m_List.Len();
}

bool CMWnd_CubeMenu_Demos::GetListItem(int32 _Index, CStr& _Name, bool _bFocus)
{
	_Name = CStrF("s, %s", m_List[_Index].Str());

	if (_bFocus)
		m_iSelected = _Index;

	return true;
}

void CMWnd_CubeMenu_Demos::AfterItemRender(int32 _Index)
{
}

void CMWnd_CubeMenu_Demos::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if(_Key == "DEMOEXECUTE")
		m_Execute = _Value;

	CMWnd_CubeMenu::EvaluateKey(_pParam, _Key, _Value);
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Demos, CMWnd_CubeMenu);



// ---------------------------------------------------------------------------------------------
bool CMWnd_CubeMenu_SelectServer::OnTextInputDone()
{
	ConExecute( CStrF("connect(\"%s\"); deferredscript(\"cg_clearmenus()\")", m_String.Str()) );
	return true;
}

int CMWnd_CubeMenu_SelectServer::ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
{
	//Hack! I don't want the letter keys to get mapped to GUI_xxx... 
	uint16 Orig = _OriginalKey.GetKey9();
	if (_Key.GetKey9() != Orig && Orig < 0x80 && Orig != SKEY_RETURN && Orig != SKEY_ESC)
		return 0;

	return CMWnd_CubeMenu_TextInput::ProcessKey(_Key, _OriginalKey);
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_SelectServer, CMWnd_CubeMenu_TextInput);


//------------------------------------------------------------------------------------
// CMWnd_CubeMenu_Mission_Items funtions

bool CMWnd_CubeMenu_Mission_Items::OnButtonPress(CMWnd *_pFocus)
{
	//if(!Layout_WndIsButton(_pFocus))
	//	return false;

	return true;
}

int32 CMWnd_CubeMenu_Mission_Items::GetNumListItems(bool _Drawing)
{
	return m_List.Len();
}

bool CMWnd_CubeMenu_Mission_Items::GetListItem(int32 _Index, CStr &_Name, bool _bFocus)
{
	//_Name = CStrF("s, %s", m_List[_Index].Str());

	if (_bFocus)
		m_iSelected = _Index;

	return true;
}
void CMWnd_CubeMenu_Mission_Items::OnCreate()
{
	MACRO_GetRegisterObject(CGameContext, pGameContext, "GAMECONTEXT");
	CWObject_Game *pGame = pGameContext->m_spWServer->Game_GetObject();
	CWO_Player *pPlayer = pGame ? pGame->Player_Get(0) : NULL; 
	CWObject *pCharObj = pPlayer ? pGameContext->m_spWServer->Object_Get(pPlayer->m_iObject) : NULL;
	CWObject_Character *pChar  = pCharObj ? safe_cast<CWObject_Character>(pCharObj) : NULL;
	CWO_Character_ClientData* pCD = pChar ? TDynamicCast<CWO_Character_ClientData>((CReferenceCount*)pChar->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
	if(!pCD)
		return;

	CWorld_Server* pWServer = pGameContext->m_spWServer;
	CWServer_Mod *pServerMod = pWServer ? safe_cast<CWServer_Mod>(pWServer) : NULL;
	CStr CurrentLevel = CStr(pServerMod->m_WorldName).GetFilenameNoExt().UpperCase();
	bool bIsNy = ((CurrentLevel.Find("OW1_") == -1) && (CurrentLevel.Find("OW2_") == -1));

	SMissionItem Item;
	int TaggedItem = -1;
	int iNumItems = pCD->m_InventoryInfo.m_lInventory.Len();
	bool AddTheItem;
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	for(int i = 0; i < iNumItems; i++)
	{
		AddTheItem = true;
		Item.m_AmountOfItem = 1;
		
		Item.m_ItemName = Localize_Str(pCD->m_InventoryInfo.m_lInventory[i].m_ItemName);
		Item.m_ItemName = Item.m_ItemName.UpperCase();
		Item.m_ItemDesc = Localize_Str(pCD->m_InventoryInfo.m_lInventory[i].m_ItemDescription);
		Item.m_iPictureID = pCD->m_InventoryInfo.m_lInventory[i].m_iJournalImage;

		// setup picture-ID for the item
		if(pCD->m_InventoryInfo.m_lInventory[i].m_Flags & CWO_Inventory_Info::INVENTORYINFO_SUBTYPE_WEAPON_GUN)
		{
			Item.m_iItemType = ITEMTYPE_WEAPON_GUN;
			Item.m_AmountOfItem = 1;//pCD->m_InventoryInfo.m_lInventory[i].m_iAmount;
			if(bIsNy)
				Item.m_iPictureID = pTC->GetTextureID("journal_Guns");
			else
				Item.m_iPictureID = pTC->GetTextureID("journal_OW_Guns");
			
			//find the first gun in the list, if any, and pile up the guns
			int WeaponListLen = m_ListWeapons.Len();
			for(int j = 0; j < WeaponListLen; j++)
			{
				if(m_ListWeapons[j].m_iItemType == ITEMTYPE_WEAPON_GUN)
				{
					m_ListWeapons[j].m_AmountOfItem += 1;
					AddTheItem = false;
					break;
				}
			}
		}
		else if(pCD->m_InventoryInfo.m_lInventory[i].m_Flags & CWO_Inventory_Info::INVENTORYINFO_SUBTYPE_WEAPON_RIFLE)
		{
			Item.m_iItemType = ITEMTYPE_WEAPON_RIFLE;
			Item.m_AmountOfItem = pCD->m_InventoryInfo.m_lInventory[i].m_iAmount;

			/*
			int WeaponListLen = m_ListWeapons.Len();
			for(int j = 0; j < WeaponListLen; j++) // this can actually never happen since you only have 1shotgun, tactical etc
			{
				if(m_ListWeapons[j].m_ItemName == Item.m_ItemName)
				{
					m_ListWeapons[j].m_AmountOfItem += Item.m_AmountOfItem;
					AddTheItem = false;
					break;
				}
			}
			*/
		}
		else
		{
			Item.m_iItemType = ITEMTYPE_MISSION;

			// first try to get the image set via .xrg file. If none is set, the 
			// name of the image should be the same as the name of the item
			if(!Item.m_iPictureID)
			{
				CStr PictureName = pCD->m_InventoryInfo.m_lInventory[i].m_ItemName.DelTo(1);
				Item.m_iPictureID = pTC->GetTextureID(PictureName);		
				if(!Item.m_iPictureID)
					Item.m_iPictureID = pTC->GetTextureID("journal_na");
			}

			if(pCD->m_InventoryInfo.m_lInventory[i].m_Identifier == pCD->m_InventoryInfo.m_iGUITaggedItem)
			{
				TaggedItem = m_List.Len();
				pCD->m_InventoryInfo.m_iGUITaggedItem = -1;
			}

		}

		int WeaponListLen = m_ListWeapons.Len();
		if(WeaponListLen == 5 && Item.m_iItemType == ITEMTYPE_WEAPON_GUN) // only 5 weapongroups
		{
			ConOutL(CStrF("WARNING: (CMWnd_CubeMenu_Mission_Items::OnCreate): Tried to add more than 5 types of weapons"));
			M_TRACEALWAYS("WARNING: (CMWnd_CubeMenu_Mission_Items::OnCreate): Tried to add more than 5 types of weapons");
			AddTheItem = false;
		}

		if(AddTheItem)
		{
			if(Item.m_iItemType == ITEMTYPE_MISSION)
				m_List.Add(Item);
			else
			{
				// get amount of ammo for this weapon (eg shells etc)
				m_ListWeapons.Add(Item);
			}
		}
	}

	if(TaggedItem == -1)
	{
		if(m_List.Len() > NUMBEROFROWSINSCROLLIST)
			m_iSelected = NUMBEROFROWSABOVEINSCROLLIST;
		else
			m_iSelected = TruncToInt(m_List.Len() *0.5f);
	}
	else
		m_iSelected = TaggedItem;
	

	m_iLastSelected = m_iSelected-1;
	m_iStartAt = m_iSelected  - NUMBEROFROWSABOVEINSCROLLIST;

	m_ScrollUpTimer = 0.0f;
	m_ScrollDownTimer = 0.0f;
	m_LastTime = 0.0f;

	CMWnd_CubeMenu::OnCreate();
}

static fp32 Sinc(fp32 _x)
{
	MAUTOSTRIP(Sinc, 0.0f);
	return M_Sin((_x - 0.5f)*_PI)*0.5f + 0.5f;
}

void CMWnd_CubeMenu::OnPaintCommonStyle(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client,
										TArray<SMissionItem> _lItemList, 
										int *_iSelected, int *_iLastSelected, int *_iStartAt,
										fp32 _ScrollUpTimer, fp32 _ScrollDownTimer,
										int8 _iType)
{
	int ListLen = _lItemList.Len();
	CRC_Font *pFont = GetFont("HEADINGS");
	if(!pFont)
		return;

	CXR_VBManager *pVBM = _pRCUtil->GetVBM();
	CRC_Viewport*pViewport = pVBM->Viewport_Get();
	CVec2Dfp32 CoordScale = _pRCUtil->GetCoordinateScale();
	CMat4Dfp32* pMat2D = pVBM->Alloc_M4_Proj2DRelBackPlane();
	if (!pMat2D)
		return;

	CPixel32 PixColor(255, 255, 255, 255);
	CPixel32 PixColorGray(180, 180, 180,90);
	CPixel32 PixColorBlack(0, 0, 0, 255);
	CPixel32 PixColorBlackOutline(0, 0, 0, 105);
	CPixel32 PixColorFlash(255, 255, 255, 64);
	//-------------------------------------------------------------
	// headings

	// looking best at 1280/720
	CClipRect VPClip = pViewport->GetViewClip();
	fp32 FontSize = 18.0f;
	
	// HACK!! set x = 0 when projecting
	CPnt HeadingOffset = CPnt((TruncToInt((VPClip.clip.GetWidth() / CoordScale.k[0]) * 0.5f)) - 256 , (TruncToInt((VPClip.clip.GetHeight() / CoordScale.k[1]) * 0.5)) - 186); 

	CStr Text;	
	CRct FlashRect;

	// x positions for header
	int ItemsHeadingPosVal		=  15 + HeadingOffset.x;
	int MapHeadingPosVal		= 230 + HeadingOffset.x;
	int DarklingsHeadingPosVal	= 380 + HeadingOffset.x;

	//------------------------------------------------------------
	// a couple of lines TODO: change to graphics

	FlashRect.p0.y = HeadingOffset.y + TruncToInt(FontSize) + 2;
	FlashRect.p0.x = HeadingOffset.x - 60;
	FlashRect.p1.x = 720;
	FlashRect.p1.y = FlashRect.p0.y + 3;
	_pRCUtil->Rect(VPClip, FlashRect, PixColorFlash);

	switch(_iType)
	{
	case STYLE_DARKLINGS:
		{
			Text =  Localize_Str("§LGUI_INVENTORY_TURN");
			Text = Text.UpperCase();
			_pRCUtil->Text(VPClip, pFont, ItemsHeadingPosVal, HeadingOffset.y, Text, PixColor, FontSize);		

			Text = Localize_Str("§LGUI_MAP_TURN");
			Text = Text.UpperCase();
			_pRCUtil->Text(VPClip, pFont, MapHeadingPosVal, HeadingOffset.y, Text, PixColor, FontSize);	

			Text = Localize_Str("§LGUI_DARKNESSHEADER");
			FlashRect.p0 = CPnt(DarklingsHeadingPosVal - 6, (HeadingOffset.y) - 4);
			FlashRect.p1 = CPnt(FlashRect.p0.x + _pRCUtil->TextWidth(pFont, Text, (FontSize / pFont->GetOriginalSize())) + 12, (HeadingOffset.y + TruncToInt(FontSize) + 2));
			_pRCUtil->Rect(VPClip, FlashRect, PixColorFlash);
			_pRCUtil->Text_DrawFormatted(VPClip, pFont, Text, DarklingsHeadingPosVal, HeadingOffset.y, 0, PixColorBlackOutline, PixColorBlackOutline, PixColorBlackOutline, VPClip.GetWidth(), VPClip.GetHeight(), true, 0, (FontSize / pFont->GetOriginalSize()));
			_pRCUtil->Text_DrawFormatted(VPClip, pFont, Text, DarklingsHeadingPosVal, HeadingOffset.y, 0, PixColor, PixColor, PixColorBlack, VPClip.GetWidth(), VPClip.GetHeight(), false, 0, (FontSize / pFont->GetOriginalSize()));		}
		break;

	case STYLE_MAP:
		{
			Text =  Localize_Str("§LGUI_INVENTORY_TURN");
			Text = Text.UpperCase();
			_pRCUtil->Text(VPClip, pFont, ItemsHeadingPosVal, HeadingOffset.y, Text, PixColor, FontSize);		

			Text = Localize_Str("§LGUI_MAP_TURN");
			Text = Text.UpperCase();
			FlashRect.p0 = CPnt(MapHeadingPosVal - 6, (HeadingOffset.y) - 4);
			FlashRect.p1 = CPnt(MapHeadingPosVal + _pRCUtil->TextWidth(pFont, Text, (FontSize / pFont->GetOriginalSize())) + 6, (HeadingOffset.y + TruncToInt(FontSize) + 2));
			_pRCUtil->Rect(VPClip, FlashRect, PixColorFlash);
			_pRCUtil->Text_DrawFormatted(VPClip, pFont, Text, MapHeadingPosVal, HeadingOffset.y, 0, PixColorBlackOutline, PixColorBlackOutline, PixColorBlackOutline, VPClip.GetWidth(), VPClip.GetHeight(), true, 0, (FontSize / pFont->GetOriginalSize()));
			_pRCUtil->Text_DrawFormatted(VPClip, pFont, Text, MapHeadingPosVal, HeadingOffset.y, 0, PixColor, PixColor, PixColorBlack, VPClip.GetWidth(), VPClip.GetHeight(), false, 0, (FontSize / pFont->GetOriginalSize()));

			Text = Localize_Str("§LGUI_DARKNESSHEADER");
			_pRCUtil->Text(VPClip, pFont, DarklingsHeadingPosVal, HeadingOffset.y, Text, PixColor, FontSize);	

			// maps are different
			return;
		}
		break;

	case STYLE_ITEMS:
		{
			Text = Localize_Str("§LGUI_INVENTORY_TURN");
			Text = Text.UpperCase();
			FlashRect.p0 = CPnt(ItemsHeadingPosVal - 6, (HeadingOffset.y) - 4);
			FlashRect.p1 = CPnt(ItemsHeadingPosVal + _pRCUtil->TextWidth(pFont, Text, (FontSize / pFont->GetOriginalSize())) + 6, (HeadingOffset.y + TruncToInt(FontSize) + 2));
			_pRCUtil->Rect(VPClip, FlashRect, PixColorFlash);
			_pRCUtil->Text_DrawFormatted(VPClip, pFont, Text, 15 + HeadingOffset.x, HeadingOffset.y, 0, PixColorBlackOutline, PixColorBlackOutline, PixColorBlackOutline, VPClip.GetWidth(), VPClip.GetHeight(), true, 0, (FontSize / pFont->GetOriginalSize()));
			_pRCUtil->Text_DrawFormatted(VPClip, pFont, Text, 15 + HeadingOffset.x, HeadingOffset.y, 0, PixColor, PixColor, PixColorBlack, VPClip.GetWidth(), VPClip.GetHeight(), false, 0, (FontSize / pFont->GetOriginalSize()));

			Text = Localize_Str("§LGUI_MAP_TURN");
			Text = Text.UpperCase();
			_pRCUtil->Text(VPClip, pFont, MapHeadingPosVal, HeadingOffset.y, Text, PixColor, FontSize);	

			Text = Localize_Str("§LGUI_DARKNESSHEADER");
			_pRCUtil->Text(VPClip, pFont, DarklingsHeadingPosVal, HeadingOffset.y, Text, PixColor, FontSize);
		}
	    break;

	default:
		return;
	}


	//------------------------------------------------------------
	// a couple of lines TODO: change to graphics

	FlashRect.p0.y = HeadingOffset.y + TruncToInt(FontSize) + 2 + 80;
	FlashRect.p0.x = HeadingOffset.x - 60;
	FlashRect.p1.x = 720;
	FlashRect.p1.y = FlashRect.p0.y + 3;
	_pRCUtil->Rect(VPClip, FlashRect, PixColorFlash);
	
	//------------------------------------------------------------
	// scrolltext up/down + flash-stuff and up/down buttons

	FontSize = 15.0f; 
	int32 TrunceFS15 = TruncToInt(FontSize*1.5f);

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if(ListLen == 0)
		return;
	// up/down buttons

	int PictureWidthBase;
	fp32 PicWidth;
	fp32 PicHeight;

	CRect2Duint16 PictureScreenCoord;
	CPnt ScrollButtonPointBase;
	fp32 FirstScrollButtonYpos = 150.0f + HeadingOffset.y;
	ScrollButtonPointBase.y = TruncToInt(FirstScrollButtonYpos);

	int TextureID = 0;
	bool bPressed = false;

	// for flip or no-flip UV cords
	CVec2Dfp32 UVMin(0.0f, 0.0f);
	CVec2Dfp32 UVMax(1.0f, 1.0f);

	CVec2Dfp32* pTV;
	pTV = CXR_Util::VBM_CreateRectUV(pVBM, UVMin, UVMax) ;

	for(int i = 0; i < 2; i++)
	{			
		if((((*_iSelected > *_iLastSelected) || (_ScrollDownTimer != 0)) && i) ||
			(((*_iSelected < *_iLastSelected) || (_ScrollUpTimer != 0)) && !i))
		{
			bPressed = true;
		}

		if(i)
		{
			if(bPressed)
				TextureID = pTC->GetTextureID("GUI_Arrow_Down_pressed");
			else
				TextureID = pTC->GetTextureID("GUI_Arrow_Down");
		}
		else
		{
			if(bPressed)
				TextureID = pTC->GetTextureID("GUI_Arrow_Up_pressed");
			else
				TextureID = pTC->GetTextureID("GUI_Arrow_Up");
		}

		bPressed = false;

		if(TextureID)
		{
			PictureWidthBase	 =  12;
			PicWidth  = PictureWidthBase * CoordScale.k[0];
			PicHeight = PicWidth;
			ScrollButtonPointBase.x = 75 + HeadingOffset.x;

			PictureScreenCoord.m_Min = CVec2Duint16(TruncToInt(ScrollButtonPointBase.x * CoordScale.k[0]), TruncToInt(ScrollButtonPointBase.y * CoordScale.k[1]));
			PictureScreenCoord.m_Max = CVec2Duint16(PictureScreenCoord.m_Min.k[0] + TruncToInt(PicWidth), PictureScreenCoord.m_Min.k[1] + TruncToInt(PicHeight));

			CRC_Attributes* pA = pVBM->Alloc_Attrib();
			pA->SetDefault();
			pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
			pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
			pA->Attrib_TextureID(0, TextureID);

			CXR_VertexBuffer* pVBScreenQuad = CXR_Util::VBM_RenderRect(pVBM, pMat2D, PictureScreenCoord, PixColor, 0.1f, pA);
			pVBScreenQuad->Geometry_TVertexArray(pTV, 0);
			pVBM->AddVB(pVBScreenQuad);
		}

		// second scrollbutton
		ScrollButtonPointBase.y = TruncToInt(FirstScrollButtonYpos + (TrunceFS15 * (NUMBEROFROWSINSCROLLIST-1) + (SPACEBETWEENSCROLLBUTTONANDTEXT*2)));
	}

	//---------------------------------------------------------------------------
	// scrollist text

	if(*_iSelected < *_iStartAt)
	{
		*_iStartAt = *_iSelected;
	}
	else if(*_iSelected > *_iLastSelected)
	{
		if((*_iSelected - *_iStartAt) > (NUMBEROFROWSINSCROLLIST-1))
			*_iStartAt = *_iSelected - NUMBEROFROWSINSCROLLIST-1;
	}
	
	*_iLastSelected = *_iSelected;

	int iStopBefore = NUMBEROFROWSINSCROLLIST + *_iStartAt;
	iStopBefore = Min(iStopBefore, ListLen);

	fp32 ScrollTextYOffset = FirstScrollButtonYpos + SPACEBETWEENSCROLLBUTTONANDTEXT;
	int Ypos = TruncToInt(ScrollTextYOffset-(FontSize*1.5f));

	FlashRect.p0.x = HeadingOffset.x - 3;
	FlashRect.p0.y = (Ypos + (TrunceFS15 * (NUMBEROFROWSABOVEINSCROLLIST+1))) - 2;
	FlashRect.p1.y = FlashRect.p0.y + TruncToInt(FontSize ) + 3;

	CClipRect TextBox;
	TextBox = VPClip;
	TextBox.clip.p0.x = HeadingOffset.x - 3;
	TextBox.clip.p0.y = Ypos+TrunceFS15*2;
	TextBox.clip.p1.x = TextBox.clip.p0.x + 200;
	TextBox.clip.p1.y = TextBox.clip.p0.y + (TrunceFS15 * (NUMBEROFROWSINSCROLLIST-2)) - SPACEBETWEENSCROLLBUTTONANDTEXT - 3;


	if(_ScrollDownTimer != 0.0f)
		Ypos += TruncToInt(Sinc(1.0f - (_ScrollDownTimer / SCROLLSTEPTIME)) * (FontSize*1.5f));
	else if(_ScrollUpTimer != 0.0f)
		Ypos -= TruncToInt(Sinc(1.0f - (_ScrollUpTimer / SCROLLSTEPTIME)) * (FontSize*1.5f));
	
	int StoredSelectedYpos = Ypos;
	int iPref = -1;
	for(int i = *_iStartAt; i < iStopBefore; i++)
	{
		Ypos += TrunceFS15;

		if( i < 0)
			continue;

		Text = _lItemList[i].m_ItemName;

		if(i == *_iSelected)
		{
			FlashRect.p1.x = FlashRect.p0.x + _pRCUtil->TextWidth(pFont, Text, (FontSize / pFont->GetOriginalSize())) + 6;
			StoredSelectedYpos = Ypos;
		}
		
		_pRCUtil->Text(TextBox, pFont, HeadingOffset.x, Ypos, Text, PixColor,FontSize);
	}

	//----------------------------------------------------------------
	// here's the flash
	if(*_iSelected != -1)
	{
		_pRCUtil->Rect(VPClip, FlashRect, PixColorFlash);

		Text = _lItemList[*_iSelected].m_ItemName;
				
		_pRCUtil->Text_DrawFormatted(TextBox, pFont, Text, HeadingOffset.x, StoredSelectedYpos, 0, PixColorBlackOutline, PixColorBlackOutline, PixColorBlackOutline, VPClip.GetWidth(), VPClip.GetHeight(), true, 0, (FontSize / pFont->GetOriginalSize()));
		_pRCUtil->Text_DrawFormatted(TextBox, pFont, Text, HeadingOffset.x, StoredSelectedYpos, 0, PixColor, PixColor, PixColorBlack, VPClip.GetWidth(), VPClip.GetHeight(), false, 0, (FontSize / pFont->GetOriginalSize()));
	}

	//-----------------------------------------------------------------------------------------------------------------------
	// draw mission picture if any

	if(*_iSelected != -1 && _lItemList.Len())
	{
		int PictureWidthBase	 =  96;
		int PictureHeightBase	 =  96;
		int PictureBorders		 =  10;

		PicWidth			 =	PictureWidthBase * CoordScale.k[0];
		PicHeight			 =	PictureHeightBase * CoordScale.k[1];

		CPnt PictureStartPos;
		PictureStartPos.x = TruncToInt((200 + HeadingOffset.x) * CoordScale.k[0]);
		PictureStartPos.y = TruncToInt((160 + HeadingOffset.y) * CoordScale.k[1]);

		PictureScreenCoord.m_Min = CVec2Duint16(PictureStartPos.x, PictureStartPos.y);
		PictureScreenCoord.m_Max = CVec2Duint16((PictureScreenCoord.m_Min.k[0] + TruncToInt(PicWidth)), (PictureScreenCoord.m_Min.k[1] + TruncToInt(PicHeight)));

		CPnt PictureStartPosBase;
		PictureStartPosBase.x = HeadingOffset.x - 80;
		PictureStartPosBase.y = HeadingOffset.y + 37;

		if(_lItemList[*_iSelected].m_iPictureID)
		{
			CRC_Attributes* pA = pVBM->Alloc_Attrib();
			pA->SetDefault();
			pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
			pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
			pA->Attrib_TextureID(0, _lItemList[*_iSelected].m_iPictureID);

			CXR_VertexBuffer* pVBScreenQuad = CXR_Util::VBM_RenderRect(pVBM, pMat2D, PictureScreenCoord, 0xffffffff, 0.1f, pA);
			pVBScreenQuad->Geometry_TVertexArray(pTV, 0);
			pVBM->AddVB(pVBScreenQuad);
		}
		else // error! missing image
		{
			CRct ErrorRect;
			ErrorRect.p0.y = PictureStartPosBase.y + 15;
			ErrorRect.p0.x = PictureStartPosBase.x;

			ErrorRect.p1.x = ErrorRect.p0.x + PictureWidthBase;
			ErrorRect.p1.y = ErrorRect.p0.y + PictureHeightBase;
			_pRCUtil->Rect(VPClip, ErrorRect, 0x55555555);
		}

		//-----------------------------------------------------------------------------------------------------------------------
		// split text into two regions
		// first part
		CPnt InfoTextStartPos;
		InfoTextStartPos.x = TruncToInt(PictureStartPos.x / CoordScale.k[0]) + PictureWidthBase + PictureBorders;
		InfoTextStartPos.y = TruncToInt(PictureStartPos.y / CoordScale.k[1]);

		// first print out the name of the item as a header
		// here text = name + amount
		CPnt InfoTextSize;
		InfoTextSize.x = 200;
		InfoTextSize.y = PictureWidthBase;
		int th = 0;

		// the split the description into two parts
		FontSize = 14.0f;
		int nrFirstSpaceLines = 5;
		CStr Text = _lItemList[*_iSelected].m_ItemDesc;
		for(int contr = 0; contr < 2; contr++)
		{
			wchar TextWchar[1024];
			Localize_Str(Text, TextWchar, 1023);
			int Len = CStrBase::StrLen(TextWchar);

			wchar Lines[32][512];
			const int MaxLines = 31;
			wchar* lpLines[32];

			for(int i = 0; i < 32; i++) 
				lpLines[i] = &Lines[i][0];

			int nLines = 0;
			nLines = _pRCUtil->Text_WordWrap(pFont, InfoTextSize.x, (wchar*) TextWchar, Len, &lpLines[nLines], 31, FontSize / pFont->GetOriginalSize());

			int nrOfCharsInFirstSpace = 0;

			if(contr==1)
				nrFirstSpaceLines = nLines;
			else
			{
				for(int ii = 0; ii < nrFirstSpaceLines; ii++)
					nrOfCharsInFirstSpace += CStrBase::StrLen(lpLines[ii]);
			}

			int th = 0;
			int32 LineCount = Min(nLines,nrFirstSpaceLines);
			for(int ln = 0; ln < LineCount; ln++)
			{
				_pRCUtil->Text_Draw(VPClip, pFont, &Lines[ln][0], InfoTextStartPos.x, InfoTextStartPos.y, 0, PixColor, 0, 0, FontSize / pFont->GetOriginalSize());
				th = _pRCUtil->TextHeight(pFont, &Lines[ln][0], FontSize / pFont->GetOriginalSize());
				InfoTextStartPos.y += th + 6; // textheight + additional height
			}

			if(nLines <= nrFirstSpaceLines)
				break;

			Text = Text.DelTo(nrOfCharsInFirstSpace-1); //  erase and rewind
			InfoTextStartPos.x -= (PictureWidthBase + PictureBorders);
			InfoTextSize.x += PictureWidthBase;
		}
	}

}

void CMWnd_CubeMenu_Mission_Items::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	// render tentacles and the buttons and stuff
	m_CubeUser.m_pFrontEndMod->m_Cube.m_GUIFlags |=	CCube::GUI_FLAGS_DISABLEPROJECTIONANIM;
	m_CubeUser.m_pFrontEndMod->m_Cube.m_GUIFlags |=	CCube::GUI_FLAGS_DONTPROJECT; // HACK
	m_CubeUser.m_pFrontEndMod->m_Cube.m_GUIFlags |= CCube::GUI_FLAGS_ISJOURNAL;

	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);

	CXR_VBManager *pVBM = _pRCUtil->GetVBM();

	_pRCUtil->GetVBM()->ScopeBegin("CMWnd_CubeMenu_Mission_Items::OnPaint", false);

	// clear out the tentacles (image store last in render_p5)
	pVBM->AddClearRenderTarget(0.0f, CDC_CLEAR_COLOR, 0x0, 0, 0);

	CMTime Time = CMTime::GetCPU();
	fp32 CurTime = Time.GetTime();
	fp32 DeltaTime = CurTime - m_LastTime;

	if(m_ScrollUpTimer != 0.0f)
		m_ScrollUpTimer += DeltaTime;

	if(m_ScrollDownTimer != 0.0f)
		m_ScrollDownTimer += DeltaTime;

	m_LastTime = CurTime;

	m_ScrollUpTimer = Min(m_ScrollUpTimer, SCROLLSTEPTIME);
	m_ScrollDownTimer = Min(m_ScrollDownTimer, SCROLLSTEPTIME );

	OnPaintCommonStyle(_pRCUtil, _Clip, _Client, m_List, &m_iSelected, &m_iLastSelected, &m_iStartAt, m_ScrollUpTimer, m_ScrollDownTimer, STYLE_ITEMS);

	m_ScrollUpTimer = m_ScrollUpTimer == SCROLLSTEPTIME ? 0.0f : m_ScrollUpTimer;
	m_ScrollDownTimer = m_ScrollDownTimer == SCROLLSTEPTIME ? 0.0f : m_ScrollDownTimer;

	//----------------------------------------------------------------------------------
	// info about items
	CRC_Font *pFont = GetFont("HEADINGS");
	if(!pFont)
		return;

	CRC_Viewport*pViewport = pVBM->Viewport_Get();
	CVec2Dfp32 CoordScale = _pRCUtil->GetCoordinateScale();

	CPixel32 PixColor(255, 255, 255, 255);

	CClipRect VPClip = pViewport->GetViewClip();

	// weapons are 1-2 and missionimages 1-1.33
	int PictureHeightBase	 =  50;
	int PictureWidthBase	 =  PictureHeightBase * 2;
	int PictureBorders		 =  10;

	//--------------------------------------------------------------------------------------------------
	// TODO: duplicate calculations from commonstyle!!
	fp32 FontSize = 18.0f;
	CPnt HeadingOffset = CPnt((TruncToInt((VPClip.clip.GetWidth() / CoordScale.k[0]) * 0.5f)) - 200 , (TruncToInt((VPClip.clip.GetHeight() / CoordScale.k[1]) * 0.5)) - 186); 

	// positions
	CPnt PictureStartPosBase;
	PictureStartPosBase.x = HeadingOffset.x - 80;
	PictureStartPosBase.y = HeadingOffset.y + 37;

	//--------------------------------------------------------------------------------------------------

	// first the info picture
	fp32 PicWidth  = PictureWidthBase * CoordScale.k[0];
	fp32 PicHeight = PictureHeightBase * CoordScale.k[1];

	CRect2Duint16 PictureScreenCoord;
	PictureScreenCoord.m_Min = CVec2Duint16(TruncToInt(PictureStartPosBase.x * CoordScale.k[0]), TruncToInt(PictureStartPosBase.y * CoordScale.k[1]));
	PictureScreenCoord.m_Max = CVec2Duint16(PictureScreenCoord.m_Min.k[0] + TruncToInt(PicWidth), PictureScreenCoord.m_Min.k[1] + TruncToInt(PicHeight));

	CVec2Dfp32 UVMin(0.0f, 0.0f);
	CVec2Dfp32 UVMax(1.0f, 1.0f);

	CMat4Dfp32* pMat2D = pVBM->Alloc_M4_Proj2DRelBackPlane();
	if(!pMat2D)
		return;

	CVec2Dfp32* pTV = CXR_Util::VBM_CreateRectUV(pVBM, UVMin, UVMax);


	for(int i= 0; i < m_ListWeapons.Len(); i++)
	{
		if(m_ListWeapons[i].m_iPictureID)
		{
			//----------------------------------------------------------------------------
			// for flip or no-flip UV cords
			CRC_Attributes* pA = pVBM->Alloc_Attrib();
			pA->SetDefault();
			pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
			pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
			pA->Attrib_TextureID(0, m_ListWeapons[i].m_iPictureID);

			CXR_VertexBuffer* pVBScreenQuad = CXR_Util::VBM_RenderRect(pVBM, pMat2D, PictureScreenCoord, 0xffffffff, 0.1f, pA);
			pVBScreenQuad->Geometry_TVertexArray(pTV, 0);
			pVBM->AddVB(pVBScreenQuad);
		}
		else // error! missing image
		{
			CRct ErrorRect;
			ErrorRect.p0.y = HeadingOffset.y + 37;
			ErrorRect.p0.x = HeadingOffset.x - 80 + ((PictureWidthBase + PictureBorders)*i);

			ErrorRect.p1.x = ErrorRect.p0.x + PictureWidthBase;
			ErrorRect.p1.y = ErrorRect.p0.y + PictureHeightBase;
			_pRCUtil->Rect(VPClip, ErrorRect, 0x55555555);
		}

		CStr Text;
		if(m_ListWeapons[i].m_AmountOfItem > 1) 	//+ the x 3, if 3 copies of it in inventory
		{
			CStr StrNum;
			sprintf(StrNum.GetBuffer(3), "%3d", m_ListWeapons[i].m_AmountOfItem);
			if(m_ListWeapons[i].m_iItemType == ITEMTYPE_WEAPON_GUN)
				Text = " x" + StrNum;
			else
				Text = " /" + StrNum;
			_pRCUtil->Text(VPClip, pFont, PictureStartPosBase.x + 35 + (PictureWidthBase + PictureBorders)*i, PictureStartPosBase.y + 40, Text, PixColor, 18);
		}

		int Adder = TruncToInt((PictureWidthBase + PictureBorders) * CoordScale.k[0]);
		PictureScreenCoord.m_Min.k[0] += Adder;
		PictureScreenCoord.m_Max.k[0] += Adder;
	}

	//--------------------------------------------------------------------------------------------
	// store current screen, restore tentacle image, and render this screen as a quad

	MixTentaclesAndMenu(_pRCUtil);
	pVBM->ScopeEnd();
	//CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);

}

void CMWnd_CubeMenu::MixTentaclesAndMenu(CRC_Util2D* _pRCUtil)
{
	CXR_VBManager *pVBM = _pRCUtil->GetVBM();
	CRC_Viewport* pVP = pVBM->Viewport_Get();
	int sw = pVP->GetViewRect().GetWidth();
	int sh = pVP->GetViewRect().GetHeight();

	CXR_Engine *pEngine = m_CubeUser.m_pFrontEndMod->GetRender_Engine();
	CTextureContainer_Screen* pTCScreen = safe_cast<CTextureContainer_Screen>(pEngine->GetInterface(XR_ENGINE_TCSCREEN));
	int iTentacleImage = pTCScreen->GetTextureID(0);
	int iJournalImage = pTCScreen->GetTextureID(4);

	// store the current image
	pVBM->AddCopyToTexture(0.0f, CRct(0, 0, sw, sh), CPnt(0,0), iJournalImage, false);

	m_CubeUser.m_pFrontEndMod->m_Cube.RenderFullScreenImage(_pRCUtil->GetRC(), pEngine, pVBM, iTentacleImage, 0.0f, 0xffffffff);

	// 4. Render Quad
	CVec3Dfp32 Origo(0.0f,0.0f,0.0f);
	CMat4Dfp32 UnitMat;
	UnitMat.Unit();
	UnitMat.GetRow(3) = CVec3Dfp32(5.0f, 0.0f, 0.0f);

	fp32 QuadSizeX;;
	fp32 QuadSizeY;
	if(!(m_CubeUser.m_pFrontEndMod->m_Cube.m_GUIFlags & CCube::GUI_FLAGS_ISWIDESCREEN))
	{
		QuadSizeX = 8.96f;
		QuadSizeY = 6.72f;
	}
	else
	{
		QuadSizeX = 12.288f;
		QuadSizeY =  6.912f;
	}

	if(m_CubeUser.m_pFrontEndMod->m_Cube.ReadyToDrawMenu())
	{
		CVec2Dfp32 SendUVMax(sw, sh);
		m_CubeUser.m_pFrontEndMod->m_Cube.Render_Quad(pVBM, pEngine, iJournalImage, Origo, QuadSizeX, QuadSizeY, UnitMat, m_CubeUser.m_pFrontEndMod->m_Cube.GetCameraMatrix(), 
			true, 0.0f, SendUVMax);
	}

	//m_CubeUser.m_pFrontEndMod->m_Cube.RenderSafeBorders(_pRCUtil->GetRC(), pVBM);
}

int CMWnd_CubeMenu_Mission_Items::ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
{
	if(_Key.GetKey9() == SKEY_GUI_UP && _Key.IsDown() &&m_iSelected != -1 && m_List.Len())
	{
		m_iSelected--;
		if(m_iSelected != -1)
		{
			SoundEvent(WSND_TAB);
			//ConExecute("cg_playsound (\"GUI_Select_01\")");
			m_ScrollUpTimer = 0.0001f;
		}

		m_iSelected = Max(m_iSelected, 0);
		m_iStartAt--;
		m_iStartAt = Max(m_iStartAt, -NUMBEROFROWSABOVEINSCROLLIST);	
	}
	else if(_Key.GetKey9() == SKEY_GUI_DOWN  && _Key.IsDown() && m_iSelected != -1 && m_List.Len())
	{
		m_iSelected++;
		if(m_iSelected != m_List.Len())
		{
			SoundEvent(WSND_TAB);
			//ConExecute("cg_playsound (\"GUI_Select_01\")");
			m_ScrollDownTimer = 0.0001f;
		}

		m_iSelected = Min(m_iSelected, m_List.Len()-1);
		m_iStartAt++;
		m_iStartAt = Min(m_iStartAt, m_iSelected-NUMBEROFROWSABOVEINSCROLLIST);
		
	}
	else if(_Key.GetKey9() == SKEY_GUI_BUTTON0 && _Key.IsDown())
	{
		
	}
	else if(_Key.GetKey9() == SKEY_MOUSE1)
	{
		// check for mouse over scrollbutton
	}
	else if(_Key.GetKey9() == SKEY_GUI_RIGHT && _Key.IsDown())
	{
		SoundEvent(WSND_TAB);
		//ConExecute("cg_playsound (\"GUI_Select_01\")");
	}
	else if (_Key.GetKey9() == SKEY_GUI_LEFT && _Key.IsDown())
	{
		SoundEvent(WSND_TAB);
	}
	else if((_Key.GetKey9() == SKEY_GUI_CANCEL || _Key.GetKey9() == SKEY_GUI_START || _Key.GetKey9() == SKEY_GUI_BACK) && _Key.IsDown())
	{
		SoundEvent(WSND_TAB);
		//ConExecute("cg_playsound (\"GUI_Select_01\")");
		ConExecute("cg_cubeseqforce(\"backtogame\")");
	}
	
	return CMWnd_CubeLayout::ProcessKey(_Key, _OriginalKey);
}


MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Mission_Items, CMWnd_CubeMenu);

bool CMWnd_CubeMenu_Map::OnButtonPress(CMWnd *_pFocus)
{
	//if(!Layout_WndIsButton(_pFocus))
	//	return false;

	return true;
}
int32 CMWnd_CubeMenu_Map::GetNumListItems(bool _Drawing)
{
	return m_List.Len();
}
bool CMWnd_CubeMenu_Map::GetListItem(int32 _Index, CStr &_Name, bool _bFocus)
{
	
	if (_bFocus)
		m_iSelected = _Index;

	return true;
}
void CMWnd_CubeMenu_Map::OnCreate()
{
	MACRO_GetRegisterObject(CGameContext, pGameContext, "GAMECONTEXT");
	CWorld_Server* pWServer = pGameContext->m_spWServer;
	CWServer_Mod *pServerMod = pWServer ? safe_cast<CWServer_Mod>(pWServer) : NULL;
	
	if(pServerMod)
	{
		CMapPieces *pMapPieces = &m_CubeUser.m_pFrontEndMod->m_MapPieces;
		CStr Level;
		CStr CurrentLevel = CStr(pServerMod->m_WorldName).GetFilenameNoExt().UpperCase();

		m_bIsNy = ((CurrentLevel.Find("OW1_") == -1) && (CurrentLevel.Find("OW2_") == -1));
		
		int iStartAt = 0;
		if(!m_bIsNy)
			iStartAt = pMapPieces->GetNumberOfPieces(true);
		int16 iLen = pMapPieces->GetNumberOfPieces(m_bIsNy) + iStartAt;

		for(int i = iStartAt; i < iLen; i++)
		{
			Level = pMapPieces->GetActualPieceLevelName(i);
			if(Level.UpperCase() == CurrentLevel)
			{
				pMapPieces->SetPieceVisited(i);
				pMapPieces->SetPieceCurrentPiece(i);
			}
			else if(!pServerMod->m_GameState.IsFirstLevelVisit(pServerMod, Level))
				pMapPieces->SetPieceVisited(i);
		}	
	}

	CMWnd_CubeMenu::OnCreate();
}

int CMWnd_CubeMenu_Map::ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
{
	if(_Key.GetKey9() == SKEY_GUI_UP)
	{
		m_iSelected--;
		m_iSelected = Max(m_iSelected, 0);
	}
	else if(_Key.GetKey9() == SKEY_GUI_DOWN)
	{
		m_iSelected++;
		m_iSelected = Min(m_iSelected, m_List.Len()-1);
	}
	else if((_Key.GetKey9() == SKEY_GUI_CANCEL || _Key.GetKey9() == SKEY_GUI_START || _Key.GetKey9() == SKEY_GUI_BACK || _Key.GetKey9() == SKEY_GUI_LEFT || _Key.GetKey9() == SKEY_GUI_RIGHT) && _Key.IsDown())
	{
		SoundEvent(WSND_TAB);
		//ConExecute("cg_playsound (\"GUI_Select_01\")");
	}

	return CMWnd_CubeLayout::ProcessKey(_Key, _OriginalKey);
}

//---------------------------------------------------------------------------
// helpfunction

static void GetImageSize(int32 _ImageID, int16 &_Width, int16 &_Height)
{
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	CImage RetDesc;
	int Nothing = 0;
	pTC->GetTextureDesc(_ImageID, &RetDesc, Nothing);
	_Width = RetDesc.GetWidth();
	_Height = RetDesc.GetHeight();

	pTC->GetTextureDesc(_ImageID, &RetDesc, Nothing);
	_Width = RetDesc.GetWidth();
	_Height = RetDesc.GetHeight();
}

void CMWnd_CubeMenu_Map::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	// disable textanimation
	m_CubeUser.m_pFrontEndMod->m_Cube.m_GUIFlags |=	CCube::GUI_FLAGS_DISABLEPROJECTIONANIM;
	m_CubeUser.m_pFrontEndMod->m_Cube.m_GUIFlags |=	CCube::GUI_FLAGS_DONTPROJECT; // HACK!!!
	m_CubeUser.m_pFrontEndMod->m_Cube.m_GUIFlags |= CCube::GUI_FLAGS_ISJOURNAL;

	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client); // HACK
	CXR_VBManager *pVBM = _pRCUtil->GetVBM();
	CRC_Viewport*pViewport = pVBM->Viewport_Get();
	CVec2Dfp32 CoordScale = _pRCUtil->GetCoordinateScale();

	pVBM->ScopeBegin("CMWnd_CubeMenu_Map::OnPaint", 0); // HACK
	
	// clear out the tentacles (image store last in render_p5)
	pVBM->AddClearRenderTarget(0.0f, CDC_CLEAR_COLOR, 0x0, 0, 0);

	OnPaintCommonStyle(_pRCUtil, _Clip, _Client, m_List, &m_iSelected, &m_iLastSelected, &m_iStartAt, 0, 0, STYLE_MAP);
	CClipRect VPClip = pViewport->GetViewClip();
	CPnt HeadingOffset = CPnt((TruncToInt((VPClip.clip.GetWidth() / CoordScale.k[0]) * 0.5f)) - 200 , (TruncToInt((VPClip.clip.GetHeight() / CoordScale.k[1]) * 0.5)) - 186); 
	
	// here we go!
	CMapPieces *pMapPieces =  &m_CubeUser.m_pFrontEndMod->m_MapPieces;
	CRect2Duint16 PictureScreenCoord;

	CVec2Dfp32 UVMin(0.0f, 0.0f);
	CVec2Dfp32 UVMax(1.0f, 1.0f);

	CMat4Dfp32* pMat2D = pVBM->Alloc_M4_Proj2DRelBackPlane();
	if(!pMat2D)
		return;
	CVec2Dfp32* pTV = CXR_Util::VBM_CreateRectUV(pVBM, UVMin, UVMax);

	int16 TextureWidth;
	int16 TextureHeight;
	fp32 MapScale = 0.5f;

	int32 iBGImage = pMapPieces->GetBackGroundImage(m_bIsNy);
	if(iBGImage)
	{
		//-------------------------------------------------------------------------
		// Render the background

		GetImageSize(iBGImage, TextureWidth, TextureHeight);

		CPnt PictureStartPosBase;
		PictureStartPosBase.x = HeadingOffset.x - 60;
		PictureStartPosBase.y = HeadingOffset.y + 30;

		fp32 PicWidth = TextureWidth * CoordScale.k[0] * MapScale;
		fp32 PicHeight = TextureHeight * CoordScale.k[1] * MapScale;

		PictureScreenCoord.m_Min = CVec2Duint16(TruncToInt(PictureStartPosBase.x * CoordScale.k[0]), TruncToInt(PictureStartPosBase.y * CoordScale.k[1]));
		PictureScreenCoord.m_Max = CVec2Duint16(PictureScreenCoord.m_Min.k[0] + TruncToInt(PicWidth), PictureScreenCoord.m_Min.k[1] + TruncToInt(PicHeight));

		CRC_Attributes* pA = pVBM->Alloc_Attrib();
		pA->SetDefault();
		pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
		pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
		pA->Attrib_TextureID(0, iBGImage);

		CXR_VertexBuffer* pVBScreenQuad = CXR_Util::VBM_RenderRect(pVBM, pMat2D, PictureScreenCoord, 0xffffffff, 0.1f, pA);
		pVBScreenQuad->Geometry_TVertexArray(pTV, 0);
		pVBM->AddVB(pVBScreenQuad);

		//-------------------------------------------------------------------------
		// Render the map-parts
		int iStartAt = 0;
		if(!m_bIsNy)
			iStartAt = pMapPieces->GetNumberOfPieces(true);

		CStr LevelName;
		int16 iLen = pMapPieces->GetNumberOfPieces(m_bIsNy) + iStartAt;
		int32 iMapPieceImage = 0;
		for(int i = iStartAt; i < iLen; i++)
		{
			if(!pMapPieces->ShouldRender(i))
				continue;

			CPnt MapPiecePosBase = PictureStartPosBase;

			if(pMapPieces->GetGlowingPiece() == i)
			{
				LevelName = pMapPieces->GetActualPieceLevelName(i);
				iMapPieceImage = pMapPieces->GetGlowImageID(i);
				GetImageSize(iMapPieceImage, TextureWidth, TextureHeight);
				MapPiecePosBase.x += TruncToInt(pMapPieces->GetGlowPos(i).x * MapScale);
				MapPiecePosBase.y += TruncToInt(pMapPieces->GetGlowPos(i).y * MapScale);
			}
			else
			{
				iMapPieceImage = pMapPieces->GetImageID(i);
				GetImageSize(iMapPieceImage, TextureWidth, TextureHeight);
				MapPiecePosBase.x += TruncToInt(pMapPieces->GetPos(i).x * MapScale);
				MapPiecePosBase.y += TruncToInt(pMapPieces->GetPos(i).y * MapScale);
			}

			PicWidth = TextureWidth * CoordScale.k[0] * MapScale;
			PicHeight = TextureHeight * CoordScale.k[1] * MapScale;

			PictureScreenCoord.m_Min = CVec2Duint16(TruncToInt(MapPiecePosBase.x * CoordScale.k[0]), TruncToInt(MapPiecePosBase.y * CoordScale.k[1]));
			PictureScreenCoord.m_Max = CVec2Duint16(PictureScreenCoord.m_Min.k[0] + TruncToInt(PicWidth), PictureScreenCoord.m_Min.k[1] + TruncToInt(PicHeight));

			if(iMapPieceImage)
			{
				CRC_Attributes* pA = pVBM->Alloc_Attrib();
				pA->SetDefault();
				pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
				pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
				pA->Attrib_TextureID(0, iMapPieceImage);

				CXR_VertexBuffer* pVBScreenQuad = CXR_Util::VBM_RenderRect(pVBM, pMat2D, PictureScreenCoord, 0xffffffff, 0.1f, pA);
				pVBScreenQuad->Geometry_TVertexArray(pTV, 0);
				pVBM->AddVB(pVBScreenQuad);
			}
			
		}

		// print out the name of the current map
		CRC_Font *pFont = GetFont("HEADINGS");
		if(!pFont)
			return;

		const fp32 FontSize = 16.0f;
		CStr Text = Localize_Str("§L"+LevelName);
		CPixel32 PixColor(255, 255, 255, 255);
		CPixel32 PixColorBlack(0, 0, 0, 255);

		GetImageSize(iBGImage, TextureWidth, TextureHeight);
		PictureStartPosBase.y = HeadingOffset.y + 30;
		PicHeight = TextureHeight * CoordScale.k[1] * MapScale;

		fp32 hw = pFont->GetWidth(FontSize, Text)*0.5f;
		CPnt TextPos(TruncToInt(((VPClip.clip.GetWidth() / CoordScale.k[0]) * 0.5f) - hw), TruncToInt((PictureStartPosBase.y * CoordScale.k[1] + PicHeight) / CoordScale.k[1]));
		_pRCUtil->Text_DrawFormatted(VPClip, pFont, Text, TextPos.x, TextPos.y + 5, 0, PixColor, PixColor, PixColorBlack, VPClip.GetWidth(), VPClip.GetHeight(), false, 0, (FontSize / pFont->GetOriginalSize()));
	}
	
	MixTentaclesAndMenu(_pRCUtil);
	_pRCUtil->GetVBM()->ScopeEnd();
}
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Map, CMWnd_CubeMenu);

bool CMWnd_CubeMenu_Darklings::OnButtonPress(CMWnd *_pFocus)
{
	//if(!Layout_WndIsButton(_pFocus))
	//	return false;

	return true;
}
int32 CMWnd_CubeMenu_Darklings::GetNumListItems(bool _Drawing)
{
	return m_List.Len();
}
bool CMWnd_CubeMenu_Darklings::GetListItem(int32 _Index, CStr &_Name, bool _bFocus)
{
	//_Name = CStrF("s, %s", m_List[_Index].Str());

	if (_bFocus)
		m_iSelected = _Index;

	return true;
}

void CMWnd_CubeMenu_Darklings::AddPowerToList(SMissionItem &_Item, CStr _PowerName, CStr _PowerDesc, CStr _ImageName, CTextureContext *_pTC)
{
	_Item.m_ItemName = Localize_Str(_PowerName);
	_Item.m_ItemDesc = Localize_Str(_PowerDesc);
	_Item.m_iItemType = ITEMTYPE_DARKNESS;
	_Item.m_iPictureID = _pTC->GetTextureID(_ImageName);	
	if(!_Item.m_iPictureID)
		_Item.m_iPictureID = _pTC->GetTextureID("journal_na");		
	m_List.Add(_Item);

}
void CMWnd_CubeMenu_Darklings::OnCreate()
{
	MACRO_GetRegisterObject(CGameContext, pGameContext, "GAMECONTEXT");
	CWObject_Game *pGame = pGameContext->m_spWServer->Game_GetObject();
	CWO_Player *pPlayer = pGame ? pGame->Player_Get(0) : NULL; 
	CWObject *pCharObj = pPlayer ? pGameContext->m_spWServer->Object_Get(pPlayer->m_iObject) : NULL;
	CWObject_Character *pChar  = pCharObj ? safe_cast<CWObject_Character>(pCharObj) : NULL;
	CWO_Character_ClientData* pCD = pChar ? TDynamicCast<CWO_Character_ClientData>((CReferenceCount*)pChar->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
	if(!pCD)
		return;
	
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");

	SMissionItem Item;	
	Item.m_AmountOfItem = 1;
	Item.m_iPictureID = 0;
	Item.m_iItemType = 0;
	bool bBoostedLevels = false;

	int iPowerCounter = 0;
	int iTaggedPower = -1;
	uint16 DarknessPowersAvailable = pCD->m_DarknessPowersAvailable;
	
	if(DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK)
	{
		AddPowerToList(Item, "§LPOWER_CREEPINGDARK", "§LDESCPOWER_CREEPINGDARK", "GUI_creepingdark",pTC); 
		if(pCD->m_GUITaggedNewDarknessPower == (DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK))
			iTaggedPower = iPowerCounter;
		iPowerCounter++;
	}
		
	if(DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_DEMONARM)
	{
		AddPowerToList(Item, "§LPOWER_DEMONARM", "§LDESCPOWER_DEMONARM", "GUI_demonarm", pTC); 
		if(pCD->m_GUITaggedNewDarknessPower == (DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_DEMONARM))
			iTaggedPower = iPowerCounter;
		iPowerCounter++;
	}

	if(DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS)
	{
		AddPowerToList(Item, "§LPOWER_ANCIENTWEAPONS", "§LDESCPOWER_ANCIENTWEAPONS", "GUI_ancientweapons",pTC); 
		if(pCD->m_GUITaggedNewDarknessPower == (DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS))
			iTaggedPower = iPowerCounter;
		iPowerCounter++;
	}

	if(DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_BLACKHOLE)
	{
		AddPowerToList(Item, "§LPOWER_BLACKHOLE", "§LDESCPOWER_BLACKHOLE", "GUI_blackhole", pTC); 
		if(pCD->m_GUITaggedNewDarknessPower == (DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_BLACKHOLE))
			iTaggedPower = iPowerCounter;
		iPowerCounter++;
	}
		
	if(DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_DARKNESSVISION)
	{
		AddPowerToList(Item, "§LPOWER_DARKNESSVISION", "§LDESCPOWER_DARKNESSVISION", "GUI_darknessvision", pTC); 
		if(pCD->m_GUITaggedNewDarknessPower == (DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_DARKNESSVISION))
			iTaggedPower = iPowerCounter;
		iPowerCounter++;
	}

	if(DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD)
	{
		AddPowerToList(Item, "§LPOWER_DARKNESSHIELD", "§LDESCPOWER_DARKNESSHIELD", "GUI_darknesshield", pTC);
		if(pCD->m_GUITaggedNewDarknessPower == (DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD))
			iTaggedPower = iPowerCounter;
		iPowerCounter++;
	}
 
	if(pCD->m_DarknessLevel.GetPowerLevel(PLAYER_DARKNESSMODE_POWER_DEMONARM)) // returns 1 for strong demon arm, indicating NY3
		bBoostedLevels = true;

	m_DarknessLevel = pCD ? pCD->m_DarknessLevel.GetLevel() : 0;
	if(iPowerCounter)
		m_DarknessLevel += 1;

	pCD->m_GUITaggedNewDarknessPower = 0;
	m_DarknessLevelPicIDList.Clear();

	int LitCounter = 0;
	int PictureId;

	for(int i = 0; i < 9; i++)
	{
		PictureId = 0;
		if(i == 0)
		{
			if(LitCounter < m_DarknessLevel)
			{
				LitCounter++;
				PictureId = bBoostedLevels ? pTC->GetTextureID("GUI_lvl2_01") : pTC->GetTextureID("GUI_lvl1_01");
			}
			else
				PictureId = pTC->GetTextureID("GUI_lvl0_01");

		}
		else if(i == 1 || i == 3 || i == 5 || i == 7)
		{
			if(LitCounter < m_DarknessLevel)
			{
				PictureId = bBoostedLevels ? pTC->GetTextureID("GUI_lvl2_04") : pTC->GetTextureID("GUI_lvl1_04");
			}
			else
				PictureId = pTC->GetTextureID("GUI_lvl0_04");
		}
		else if(i == 2 || i == 4 || i == 6)
		{
			if(LitCounter < m_DarknessLevel)
			{
				LitCounter++;
				PictureId = bBoostedLevels ? pTC->GetTextureID("GUI_lvl2_02") : pTC->GetTextureID("GUI_lvl1_02");
			}
			else
				PictureId = pTC->GetTextureID("GUI_lvl0_02");
		}
		else if(i == 8)
		{
			if(LitCounter < m_DarknessLevel)
				PictureId = bBoostedLevels ? pTC->GetTextureID("GUI_lvl2_03") : pTC->GetTextureID("GUI_lvl1_03");
			else
				PictureId = pTC->GetTextureID("GUI_lvl0_03");
		}
		if(!PictureId)
			PictureId = pTC->GetTextureID("journal_na");
		m_DarknessLevelPicIDList.Add(PictureId);
	}

	int iNrOfDarklings = pChar->m_lAvailableDarklings.Len();
	int LastPower = m_List.Len();
	int iTaggedDarkling = -1;
	for(int i= 0; i < iNrOfDarklings; i++)
	{
		if(pChar->m_lAvailableDarklings[i].m_bTaggedForFocus)
		{
			iTaggedDarkling = LastPower+i;
			pChar->m_lAvailableDarklings[i].m_bTaggedForFocus = false;
		}

		// setup picture-ID for the item
		CStr PictureName = pChar->m_lAvailableDarklings[i].m_DarklingType.DelTo(11);
		PictureName = "GUI_" + PictureName;

		Item.m_iPictureID = pTC->GetTextureID(PictureName);	
		if(!Item.m_iPictureID) // temporary placeholder
			Item.m_iPictureID = pTC->GetTextureID("journal_na");

		CStr Adder = "§L" + pChar->m_lAvailableDarklings[i].m_DarklingType;
		Item.m_ItemName = Localize_Str(Adder);
		Item.m_ItemName = Item.m_ItemName.UpperCase();

		Adder = "§LDESC" + Adder.DelTo(1);
		Item.m_ItemDesc = Localize_Str(Adder);

		CStr temp;
		temp = Item.m_ItemDesc.Ansi();
		temp.GetStrSep(";");
		Item.m_ItemDesc = Item.m_ItemDesc.GetStrSep(";");
		m_List.Add(Item);
	}

	m_iSelected = -1; // -1 will be "cant select anything. all grayde out"

	if(iTaggedPower != -1)
		m_iSelected = iTaggedPower;
	else if(iTaggedDarkling != -1)
		m_iSelected = iTaggedDarkling;
	else
	{
		if(m_List.Len() > NUMBEROFROWSINSCROLLIST)
			m_iSelected = NUMBEROFROWSABOVEINSCROLLIST;
		else
			m_iSelected = TruncToInt(m_List.Len() *0.5f);
	}

	m_iLastSelected = m_iSelected-1;
	m_iStartAt = m_iSelected  - NUMBEROFROWSABOVEINSCROLLIST;

	m_ScrollUpTimer = 0.0f;
	m_ScrollDownTimer = 0.0f;
	m_LastTime = 0.0f;
	
	CMWnd_CubeMenu::OnCreate();
}

int CMWnd_CubeMenu_Darklings::ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
{
	if(_Key.GetKey9() == SKEY_GUI_UP && _Key.IsDown() && m_iSelected != -1 && m_List.Len())
	{
		m_iSelected--;
		if(m_iSelected != -1)
		{
			SoundEvent(WSND_TAB);
			//ConExecute("cg_playsound (\"GUI_Select_01\")");
			m_ScrollUpTimer = 0.0001f;
		}

		m_iSelected = Max(m_iSelected, 0);
		m_iStartAt--;
		m_iStartAt = Max(m_iStartAt, -NUMBEROFROWSABOVEINSCROLLIST);	
	}
	else if(_Key.GetKey9() == SKEY_GUI_DOWN  && _Key.IsDown() && m_iSelected != -1 && m_List.Len())
	{
		m_iSelected++;
		if(m_iSelected != m_List.Len())
		{
			SoundEvent(WSND_TAB);
			//ConExecute("cg_playsound (\"GUI_Select_01\")");
			m_ScrollDownTimer = 0.0001f;
		}

		m_iSelected = Min(m_iSelected, m_List.Len()-1);
		m_iStartAt++;
		m_iStartAt = Min(m_iStartAt, m_iSelected-NUMBEROFROWSABOVEINSCROLLIST);

	}
	else if((_Key.GetKey9() == SKEY_GUI_CANCEL || _Key.GetKey9() == SKEY_GUI_START || _Key.GetKey9() == SKEY_GUI_BACK || _Key.GetKey9() == SKEY_GUI_LEFT) && _Key.IsDown())
	{
		SoundEvent(WSND_TAB);
		//ConExecute("cg_playsound (\"GUI_Select_01\")");
	}
	
	return CMWnd_CubeLayout::ProcessKey(_Key, _OriginalKey);
}

void CMWnd_CubeMenu_Darklings::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	// disable textanimation
	m_CubeUser.m_pFrontEndMod->m_Cube.m_GUIFlags |=	CCube::GUI_FLAGS_DISABLEPROJECTIONANIM;
	m_CubeUser.m_pFrontEndMod->m_Cube.m_GUIFlags |=	CCube::GUI_FLAGS_DONTPROJECT; // HACK
	m_CubeUser.m_pFrontEndMod->m_Cube.m_GUIFlags |= CCube::GUI_FLAGS_ISJOURNAL;

	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);
	CXR_VBManager *pVBM = _pRCUtil->GetVBM();

	_pRCUtil->GetVBM()->ScopeBegin("CMWnd_CubeMenu_Darklings::OnPaint", 0);
	

	// clear out the tentacles (image store last in render_p5)
	pVBM->AddClearRenderTarget(0.0f, CDC_CLEAR_COLOR, 0x0, 0, 0);
	
	CMTime Time = CMTime::GetCPU();
	fp32 CurTime = Time.GetTime();
	fp32 DeltaTime = CurTime - m_LastTime;

	if(m_ScrollUpTimer != 0.0f)
		m_ScrollUpTimer += DeltaTime;

	if(m_ScrollDownTimer)
		m_ScrollDownTimer += DeltaTime;

	m_LastTime = CurTime;

	m_ScrollUpTimer = Min(m_ScrollUpTimer, SCROLLSTEPTIME);
	m_ScrollDownTimer = Min(m_ScrollDownTimer, SCROLLSTEPTIME );

	OnPaintCommonStyle(_pRCUtil, _Clip, _Client, m_List, &m_iSelected, &m_iLastSelected, &m_iStartAt, m_ScrollUpTimer, m_ScrollDownTimer, STYLE_DARKLINGS);

	m_ScrollUpTimer = m_ScrollUpTimer == SCROLLSTEPTIME ? 0.0f : m_ScrollUpTimer;
	m_ScrollDownTimer = m_ScrollDownTimer == SCROLLSTEPTIME ? 0.0f : m_ScrollDownTimer;

	// -----------------------------------------------------------------------
	// draw the dakness level

	CRC_Viewport*pViewport = pVBM->Viewport_Get();
	CVec2Dfp32 CoordScale = _pRCUtil->GetCoordinateScale();
	CClipRect VPClip = pViewport->GetViewClip();

	int PictureHeightBase	 =  50;
	int PictureWidthBase	 =  PictureHeightBase;
	//int PictureBorders		 =  0;

	//--------------------------------------------------------------------------------------------------
	// TODO: duplicate calculations from commonstyle!!
	CPnt HeadingOffset = CPnt((TruncToInt((VPClip.clip.GetWidth() / CoordScale.k[0]) * 0.5f)) - 200 , (TruncToInt((VPClip.clip.GetHeight() / CoordScale.k[1]) * 0.5)) - 186); 
		
	// positions
	CPnt PictureStartPosBase;
	PictureStartPosBase.x = HeadingOffset.x - 33;
	PictureStartPosBase.y = HeadingOffset.y + 37;

	//--------------------------------------------------------------------------------------------------

	// first the info picture
	fp32 PicWidth  = PictureWidthBase * CoordScale.k[0];
	fp32 PicHeight = PictureHeightBase * CoordScale.k[1];

	CRect2Duint16 PictureScreenCoord;
	PictureScreenCoord.m_Min = CVec2Duint16(TruncToInt(PictureStartPosBase.x * CoordScale.k[0]), TruncToInt(PictureStartPosBase.y * CoordScale.k[1]));
	PictureScreenCoord.m_Max = CVec2Duint16(PictureScreenCoord.m_Min.k[0] + TruncToInt(PicWidth), PictureScreenCoord.m_Min.k[1] + TruncToInt(PicHeight));

	CVec2Dfp32 UVMin(0.0f, 0.0f);
	CVec2Dfp32 UVMax(1.0f, 1.0f);

	CMat4Dfp32* pMat2D = pVBM->Alloc_M4_Proj2DRelBackPlane();
	if(!pMat2D)
		return;

	CVec2Dfp32* pTV = CXR_Util::VBM_CreateRectUV(pVBM, UVMin, UVMax);
	int PictureId = 0;
	for(int i= 0; i < 9; i++)
	{
		PictureId = m_DarknessLevelPicIDList[i];

		if(PictureId)
		{
			//----------------------------------------------------------------------------
			// for flip or no-flip UV cords
			CRC_Attributes* pA = pVBM->Alloc_Attrib();
			pA->SetDefault();
			pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
			pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
			pA->Attrib_TextureID(0, PictureId);

			CXR_VertexBuffer* pVBScreenQuad = CXR_Util::VBM_RenderRect(pVBM, pMat2D, PictureScreenCoord, 0xffffffff, 0.1f, pA);
			pVBScreenQuad->Geometry_TVertexArray(pTV, 0);
			pVBM->AddVB(pVBScreenQuad);
		}

		int Adder = TruncToInt((PictureWidthBase/* + PictureBorders*/) * CoordScale.k[0]);
		PictureScreenCoord.m_Min.k[0] += Adder;
		PictureScreenCoord.m_Max.k[0] += Adder;
	}

	MixTentaclesAndMenu(_pRCUtil);
	_pRCUtil->GetVBM()->ScopeEnd();

}
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Darklings, CMWnd_CubeMenu);
