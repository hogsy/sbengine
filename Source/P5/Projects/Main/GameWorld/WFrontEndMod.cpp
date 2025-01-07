#include "PCH.h"

#include "WFrontEndMod.h"
#include "../GameClasses/WObj_Game/WObj_GameMod.h"
#include "../../Shared/MOS/Classes/Win/MWinCtrlGfx.h"
#include "../../Shared/MOS/XR/XREngineImp.h"
#include "../../Shared/MOS/MSystem/Raster/MRCCore.h"
#include "../../Shared/MOS/Classes/GameContext/WGameContext.h"
#include "../Exe/WGameContextMain.h"
#include "MFloat.h"
#include "../../Shared/MOS/MSystem/MContentContext.h"

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////


// outside button configuration
const fp32 g_ButtonScale = 0.6f;
const fp32 g_ButtonYAdjust = 2.0f * g_ButtonScale;//6 * g_ButtonScale;
const int32 g_ButtonPosAlign[4] = {0, 1, 0, 2}; // 0 = left, 1 = right

const CPnt g_aButtonSizes[4] = {	CPnt::From_fp32(64*g_ButtonScale,64*g_ButtonScale),
CPnt::From_fp32(64*g_ButtonScale,64*g_ButtonScale),
CPnt::From_fp32(64*g_ButtonScale,64*g_ButtonScale),
CPnt::From_fp32(64*g_ButtonScale,64*g_ButtonScale)};

//const CStr g_aButtonTextures[6] = {"GUI_ButtonPC", "GUI_ArrowbackPC", "GUI_ArrowforwardPC","GUI_ButtonPCDown", "GUI_ArrowbackPCDown", "GUI_ArrowforwardPCDown",};
const CStr g_aButtonTextures[8] = {"GUI_Button_B", "GUI_Arrow_Left", "GUI_Arrow_Right","GUI_Button_A", "GUI_Button_B", "GUI_Arrow_Left_Pressed", "GUI_Arrow_Right_Pressed","GUI_Button_A",};

static void PrecacheSurface(CXW_Surface* _pSurf, CXR_Engine* _pEngine, int _TxtIdFlags = 0)
{
	MAUTOSTRIP(PrecacheSurface, MAUTOSTRIP_VOID);
	CXW_Surface* pSurf;
	if (_pEngine)
		pSurf = _pSurf->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
	else
		pSurf = _pSurf->GetSurface(XW_SURFOPTION_HQ0, XW_SURFREQ_MULTITEX4|XW_SURFREQ_NV20|XW_SURFREQ_NV10|XW_SURFOPTION_SHADER);

	pSurf->InitTextures(false);
	pSurf->SetTextureParam(CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE | _TxtIdFlags);
}

static void PrecacheSurface(const char* _pSurf, CXR_Engine* _pEngine, CXR_SurfaceContext *_pSC, int _TxtIdFlags = 0)
{
	MAUTOSTRIP(PrecacheSurface_2, MAUTOSTRIP_VOID);
	CXW_Surface* pSurf = _pSC->GetSurface(_pSurf);
	if (pSurf)
		PrecacheSurface(pSurf, _pEngine, _TxtIdFlags);
}

//
CRct GetButtonRect(COutsideButton &rButton, fp32 _YScale)
{
	CRct Rect;
#ifdef PLATFORM_CONSOLE
	if(rButton.m_Pos == 3)
		Rect.p0.x = int(320);
	else if(rButton.m_Pos & 1)
		Rect.p0.x = int(640 - 640 * 0.15f / 4);
	else
		Rect.p0.x = int(0 + 640 * 0.15f / 4);
#else
	if(rButton.m_Pos == 3)
		Rect.p0.x = int(320);
	else if(rButton.m_Pos & 1)
		Rect.p0.x = 640;
	else
		Rect.p0.x = 0;
#endif

	if(rButton.m_Pos == 1 || rButton.m_Pos == 3)
		Rect.p0.x -= int((64 + g_aButtonSizes[1].x) * g_ButtonScale);
	else if(rButton.m_Type == 0 || rButton.m_Pos == 0)
		Rect.p0.x += int(64 * g_ButtonScale);

	Rect.p1.x = Rect.p0.x+ g_aButtonSizes[rButton.m_Type].x;

	Rect.p1.y = int(480 - 16 * _YScale);
	if(rButton.m_Pos > 1 && rButton.m_Pos != 3)
		Rect.p1.y -= int((64 * g_ButtonScale + 8) * _YScale);
#ifdef PLATFORM_CONSOLE
	Rect.p1.y -= int(480 * 0.15f / 4);
#endif
	Rect.p0.y = int(Rect.p1.y - g_aButtonSizes[rButton.m_Type].y * _YScale);

	return Rect;
}

CRct GetButtonClickableRect(COutsideButton &rButton, fp32 _YScale) // takes m_extend into calculations
{
	CRct Rect = GetButtonRect(rButton, _YScale);
	if(rButton.m_Pos == 1)
		Rect.p0.x -= (int32)rButton.m_Extend;
	else
		Rect.p1.x += (int32)rButton.m_Extend;
	return Rect;
}
//
//
//
CMapPieces::CMapPieces()
{
	m_GlowingCurrentPiece = -1;
	m_lPieces.Clear();
	m_BackgroundImageID = 0;
	m_BackgroundImageIDOW = 0;
}

void CMapPieces::SetupAndPrecacheImage(CTextureContext *pTC, CStr _ImageName, CPnt _ImagePos, CPnt _ImageGlowPos, int16 _iPieceID, CStr _ActualLevelName)
{
	//if(!pTC || _ImageName.IsEmpty()) won't happen
	//	return;

	m_lPieces[_iPieceID].m_ImageID	  = pTC->GetTextureID(_ImageName.Str());
	_ImageName += "_g";
	m_lPieces[_iPieceID].m_GlowImageID = pTC->GetTextureID(_ImageName.Str());

	if(m_lPieces[_iPieceID].m_ImageID > 0 && m_lPieces[_iPieceID].m_GlowImageID > 0)
	{
		pTC->SetTextureParam(m_lPieces[_iPieceID].m_ImageID, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
		m_lPieces[_iPieceID].m_Pos = _ImagePos;

		pTC->SetTextureParam(m_lPieces[_iPieceID].m_GlowImageID, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
		m_lPieces[_iPieceID].m_GlowPos = _ImageGlowPos;
		m_lPieces[_iPieceID].m_bEnabled = false; // for test!
		m_lPieces[_iPieceID].m_ActualLevelName = _ActualLevelName;
	}
	else
	{
		m_lPieces[_iPieceID].m_ImageID = 0;
		m_lPieces[_iPieceID].m_GlowImageID = 0;
		m_lPieces[_iPieceID].m_Pos = CPnt(0,0);
		m_lPieces[_iPieceID].m_GlowPos = CPnt(0,0);
		m_lPieces[_iPieceID].m_bEnabled = false;
		m_lPieces[_iPieceID].m_ActualLevelName = "";
	}

}
void CMapPieces::SetupAndPrecacheImages()
{
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");

	if(pTC)
	{
		m_BackgroundImageID = pTC->GetTextureID("GUI_background"); 
		if(m_BackgroundImageID)
			pTC->SetTextureParam(m_BackgroundImageID, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);

		m_BackgroundImageIDOW = pTC->GetTextureID("GUI_backgroundOW"); 
		if(m_BackgroundImageIDOW)
			pTC->SetTextureParam(m_BackgroundImageIDOW, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
		
		m_lPieces.SetLen(NUMBEROFMAPPIECES + NUMBEROFMAPPIECESOW);
		m_GlowingCurrentPiece = 0;

		SetupAndPrecacheImage(pTC, "GUI_canal", CPnt(387,164), CPnt(384,159), GUI_CANAL, "SUB_Canal");
		SetupAndPrecacheImage(pTC, "GUI_cemetary", CPnt(418,105), CPnt(415,102), GUI_CEMETARY, "ny1_Cemetery");
		SetupAndPrecacheImage(pTC, "GUI_chinatown", CPnt(455,240), CPnt(452,230), GUI_CHINATOWN, "NY1_Chinatown");
		SetupAndPrecacheImage(pTC, "GUI_church", CPnt(556,32), CPnt(552,29), GUI_CHURCH, "NY2_Church");
		SetupAndPrecacheImage(pTC, "GUI_cityhall", CPnt(317,366), CPnt(315,350), GUI_CITYHALL, "NY2_CityHall");
		SetupAndPrecacheImage(pTC, "GUI_consite", CPnt(377,37), CPnt(358,34), GUI_CONSITE, "NY1_Consite");
		SetupAndPrecacheImage(pTC, "GUI_fulton", CPnt(322,237), CPnt(320,234), GUI_FULTON, "SUB_Fulton");
		SetupAndPrecacheImage(pTC, "GUI_grinder", CPnt(335,377), CPnt(332,367), GUI_GRINDER, "NY1_Grinder");
		SetupAndPrecacheImage(pTC, "GUI_gunhill", CPnt(253,302), CPnt(251,300), GUI_GUNHILL, "NY2_GunHill");
		SetupAndPrecacheImage(pTC, "GUI_hunter", CPnt(509,185), CPnt(504,182), GUI_HUNTER, "NY1_Hunter");
		SetupAndPrecacheImage(pTC, "GUI_low_east", CPnt(543,214), CPnt(533,212), GUI_LOW_EAST, "NY1_Lowereast");
		SetupAndPrecacheImage(pTC, "GUI_mansion", CPnt(858,105), CPnt(794,57), GUI_MANSION, "NY3_Mansion");
		SetupAndPrecacheImage(pTC, "GUI_orphange", CPnt(448,276), CPnt(444,272), GUI_ORPHANGE, "NY1_Orphanage");
		SetupAndPrecacheImage(pTC, "GUI_pier", CPnt(726,121), CPnt(723,117), GUI_PIER, "NY3_Pier");
		SetupAndPrecacheImage(pTC, "GUI_tunnel", CPnt(259,103), CPnt(256,86), GUI_TUNNEL, "NY1_Tunnel");
		SetupAndPrecacheImage(pTC, "GUI_turkish", CPnt(245,384), CPnt(228,381), GUI_TURKISH, "NY2_Turkish");

		SetupAndPrecacheImage(pTC, "GUI_trenches", CPnt(164, 285), CPnt(161, 283), GUI_TRENCH, "OW1_Trench");
		SetupAndPrecacheImage(pTC, "GUI_village", CPnt(253, 134), CPnt(250, 133), GUI_VILLAGE, "OW1_Village");
		SetupAndPrecacheImage(pTC, "GUI_sewers", CPnt(589, 240), CPnt(583, 236), GUI_SEWERS, "OW1_Sewers");
		SetupAndPrecacheImage(pTC, "GUI_cannon", CPnt(753, 230), CPnt(746, 227), GUI_CANNON, "OW1_Cannon");
		SetupAndPrecacheImage(pTC, "GUI_castle", CPnt(432, 6), CPnt(431, 4), GUI_CASTLE, "OW2_Castle");
		SetupAndPrecacheImage(pTC, "GUI_hills", CPnt(52, 70), CPnt(52, 67), GUI_HILLS, "OW1_Hills");
		SetupAndPrecacheImage(pTC, "GUI_tankride", CPnt(66, 301), CPnt(64, 299), GUI_TANKRIDE, "OW2_Tankride");
	}
}

//
//
//
CWFrontEnd_Mod::CWFrontEnd_Mod()
{
	m_bDonePrecache = false;
	m_MousePosition = CPnt(0,0);
	m_MouseMoved = false;
	m_CurAspectYScale = 1.0f;
	m_ClientMouse = false;
	m_OutOfGUI = false;
	m_CubeWndName = "CubeWnd";

#if defined(PLATFORM_XENON) && defined(M_RTM)
	m_bShowConfidential = true;
#else
	m_bShowConfidential = false;
#endif
}

void CWFrontEnd_Mod::Create(CWF_CreateInfo& _CreateInfo)
{
	MAUTOSTRIP(CWFrontEnd_Mod_Create, MAUTOSTRIP_VOID);
	m_bShowNoBlocksMessage = true;

	//
	//m_ReadyToChange = false;

	m_QueuedSide = -1;
	m_CubeReferneces = 0;
	m_pCurrentWnd = NULL;
	m_PausedGame = false;
	m_MouseMoved = false;

	m_AttractTime = CMTime::GetCPU();


	//
	//m_CurrentContent = -1;

	// Call super
	CWFrontEnd::Create(_CreateInfo);

	// init cube
	m_Cube.InitSound(m_spMapData, m_spSoundContext, m_iChannel);
	m_Cube.Init();

	//
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	//if(pSys)
	//pSys->ReadOptions();

	//	ConExecute("initcampaign(1)");
	//ConExecute("deferredscript(\"option_update\")");

	CRegistry *pReg = pSys->GetEnvironment();
	if(pReg)
	{
		CStr Map = pReg->GetValue("QUICKMAP");
		if(Map != "")
		{
			ConExecute("campaignmap(\"" + Map + "\")");
			return;
		}

		CStr Menu = pReg->GetValue("QUICKMENU");
		if(Menu != "")
		{
			ConExecute("cg_rootmenu(\"" + Menu + "\")");
			return;
		}

		m_bShowConfidential = pReg->GetValuei("SHOW_CONFIDENTIAL", m_bShowConfidential) != 0;
	}
#ifdef PLATFORM_XBOX1
#ifndef M_Profile
	// Sector size on DVD = 2048, on HD = 512
	if (XGetDiskSectorSize("D:\\") != 2048)
	{
		ConExecute("cg_rootmenu(\"nono\")");
		return;
	}
#endif
#endif

#if defined(M_RTM) || defined(PLATFORM_CONSOLE)
	ConExecute("cg_rootmenu(\"intro\")");
#else
	if (pReg->GetValuei("SKIPINTRO", 1))
		ConExecute("cg_rootmenu(\"Menu_ToFork\")");
	else
		ConExecute("cg_rootmenu(\"intro\")");
#endif

	LoadAllResources();
}


// THE ALL MIGHT KONAMI-CODE!
const int32 g_aCubeFancyCode[] = {	SKEY_GUI_UP, SKEY_GUI_UP, SKEY_GUI_DOWN, SKEY_GUI_DOWN,
SKEY_GUI_LEFT, SKEY_GUI_RIGHT, SKEY_GUI_LEFT, SKEY_GUI_RIGHT,
SKEY_GUI_CANCEL, SKEY_GUI_OK, SKEY_GUI_START};
int32 g_CodeProcess = 0;
bool g_CubeFancy = false;
fp32 g_CubeGlobalTimeScale = 1;

CVec2Dfp32 ClosestPointOnLine(CVec2Dfp32 _LinePoint1, CVec2Dfp32 _LinePoint2, CVec2Dfp32 _Point)
{
	CVec2Dfp32 c = _Point - _LinePoint1;
	CVec2Dfp32 V = (_LinePoint2 - _LinePoint1);
	V.Normalize();
	fp32 d = (_LinePoint1-_LinePoint2).Length();
	fp32 t = V * c;

	if (t < 0) return _LinePoint1;
	if (t > d) return _LinePoint2;

	V = V * t;
	return _LinePoint1 + V;
}

CPnt CWFrontEnd_Mod::GetTransformedMousePosition()
{
	if (m_CubeReferneces)
	{
		fp32 scale = 20.0f * Min(m_ScreenSize.x / 640.0f, m_ScreenSize.y / 480.0f);
		//fp32 ox = m_ScreenSize.x / 2 - scale*(20*1.15f) / 2 + 35;
		//fp32 oy = m_ScreenSize.y / 2 - scale*(20*1.10f) / 2 + 40;
		fp32 ox = 0.0f;
		fp32 oy = 0.0f;

		/*		fp32 scale = 20.0f * (m_ScreenSize.x / 640.0f);
		fp32 ox = 180.0f * (m_ScreenSize.x / 640.0f); 
		fp32 oy = 40.0f * (m_ScreenSize.y / 480.0f);*/

		return CPnt::From_fp32((m_MousePosition.x - ox) * (m_ScreenSize.x/20.0f) / scale, (m_MousePosition.y - oy) * (m_ScreenSize.y/20.0f) / scale);

		/* 
		CPnt TransformedPoint;

		CVec2Dfp32 Mouse(m_MousePosition.x, m_MousePosition.y);
		//CVec2Dfp32 BasePoint = m_Cube.m_aReferencePoints[0];
		fp32 LenX = (m_Cube.m_aReferencePoints[1]-m_Cube.m_aReferencePoints[0]).Length();
		fp32 LenY = (m_Cube.m_aReferencePoints[2]-m_Cube.m_aReferencePoints[0]).Length();
		CVec2Dfp32 BasePoint = m_Cube.m_aReferencePoints[0];
		CVec2Dfp32 PointTop = ClosestPointOnLine(m_Cube.m_aReferencePoints[0], m_Cube.m_aReferencePoints[1], Mouse);
		CVec2Dfp32 PointLeft = ClosestPointOnLine(m_Cube.m_aReferencePoints[0], m_Cube.m_aReferencePoints[2], Mouse);
		CVec2Dfp32 PointRight = ClosestPointOnLine(m_Cube.m_aReferencePoints[1], m_Cube.m_aReferencePoints[3], Mouse);
		CVec2Dfp32 PointBottom = ClosestPointOnLine(m_Cube.m_aReferencePoints[2], m_Cube.m_aReferencePoints[3], Mouse);

		fp32 Lean = (m_Cube.m_aReferencePoints[3] - m_Cube.m_aReferencePoints[1]).Length() / (m_Cube.m_aReferencePoints[2] - m_Cube.m_aReferencePoints[0]).Length();

		fp32 Temp = ((Mouse-PointLeft).Length()/(PointRight-PointLeft).Length());
		if ((Mouse.k[0]-PointLeft.k[0]) < 0)
		Temp = -Temp;

		if (Temp > 0)
		TransformedPoint.x = pow(Temp, 1.0f/Lean)*m_ScreenSize.x;
		else
		TransformedPoint.x = Temp*m_ScreenSize.x;
		Temp = ((Mouse-PointTop).Length()/(PointBottom-PointTop).Length());
		if ((Mouse.k[0]-PointTop.k[0]) < 0)
		Temp = -Temp;
		TransformedPoint.y = Temp*m_ScreenSize.y;
		return TransformedPoint;
		*/
	}
	else
		return m_MousePosition;
}

bool CWFrontEnd_Mod::ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
{
	MAUTOSTRIP(CWFrontEnd_Mod_ProcessKey, false);

	if(m_Cube.m_GUIFlags & CCube::GUI_FLAGS_LOADINGSCENERUNNING)
	{
		if(m_Cube.m_GUIFlags & CCube::GUI_FLAGS_READYTOSKIP_LS)
		{
			if(_Key.GetKey9() >= SKEY_GUI_BUTTON0 && _Key.GetKey9() < SKEY_GUI_BUTTON5)
			{
				m_Cube.m_GUIFlags |= CCube::GUI_FADE_OUT_IN_PROGRESS;
				return true;
			}
		}
		return false;
	}

	if(m_Cube.m_KeyLock)
	{
		if(_Key.GetKey9() >= SKEY_GUISTART && _Key.GetKey9() < SKEY_GUIEND)
		{
			//if(g_aCubeFancyCode[g_CodeProcess] == _Key.GetKey9())
			//	g_CodeProcess++;
		}
		return true;
	}
	else
		g_CodeProcess = 0;

	if(g_CubeFancy)
	{
		int32 Key = _Key.GetKey16();
		if((Key^SKEY_REPEAT) == 0)
		{
			g_CubeGlobalTimeScale = 1-((_Key.m_Data[0]-0x99)/(fp32)(0xff-0x99));
			//g_CubeGlobalTimeScale = 1;
		}
	}
	else
		g_CubeGlobalTimeScale = 1;

	if(_Key.GetKey9() >= SKEY_GUISTART && _Key.GetKey9() < SKEY_GUIEND)
	{
		m_AttractTime = CMTime::GetCPU();

		// Jakob: or maybe not..
		//ConExecute("cg_playsound(\"GUI_Select_01\")");
		//if(_Key.IsDown())
		//	m_Cube.PlaySoundFXEvent(CCube::SOUNDEVENT_TENTACLES_PICK, NULL, false);

	}

	// do outside buttons
	if(_Key.GetKey9() == SKEY_MOUSE1 && _Key.IsDown())
	{
		if(Cube_MouseClick(m_spDesktop))
		{
			return true;
		}
	}

	if(!CWFrontEnd::ProcessKey(_Key, _OriginalKey))
	{
		if(_Key.GetKey9() == SKEY_MOUSEMOVEREL && ((!m_OutOfGUI || m_ClientMouse) || (m_spDesktop && m_spDesktop->m_bShowMouse)))
		{
			m_MousePosition.x += _Key.GetPosX();
			m_MousePosition.y += _Key.GetPosY();

			if(m_MousePosition.x < 0) m_MousePosition.x = 0;
			if(m_MousePosition.y < 0) m_MousePosition.y = 0;
			if(m_MousePosition.x > m_ScreenSize.x) m_MousePosition.x = m_ScreenSize.x;
			if(m_MousePosition.y > m_ScreenSize.y) m_MousePosition.y = m_ScreenSize.y;

			m_MouseMoved = false;

			if(m_spDesktop != NULL)
			{
				Cube_UpdateMouse(m_spDesktop);
				return true;
			}
			else
				return false;
		}
	}
	else
	{
		return true;
	}

	return false;
}

//
bool CWFrontEnd_Mod::Cube_IsMouseOver(CMWnd *_pWnd, COutsideButton &rButton)
{
	// transform mouse
	CPnt Mouse;
	Mouse.x = int((int32)m_MousePosition.x*(640.0f/m_ScreenSize.x));
	Mouse.y = int((int32)m_MousePosition.y*(480.0f/m_ScreenSize.y));

	return GetButtonClickableRect(rButton, m_CurAspectYScale).Inside(Mouse);
}

//
bool CWFrontEnd_Mod::Cube_MouseClick(CMWnd *_pWnd)
{
	if(_pWnd && _pWnd->m_lOutsideButtons.Len())
	{
		for(int32 i = 0; i < _pWnd->m_lOutsideButtons.Len(); i++)
			if(Cube_IsMouseOver(_pWnd, _pWnd->m_lOutsideButtons[i]))
			{
				// hit that shit!
				//ConExecute("cg_playsound(\"gui_m_open\")");
				ConExecuteImp(_pWnd->m_lOutsideButtons[i].m_Action, __FUNCTION__, 0);
				return true;
			}
	}
	return false;
}

CMWnd *CWFrontEnd_Mod::GetCursorWindow(bool _bOnlyTabstops)
{
	if (!m_spDesktop)
		return NULL;
	CPnt Mouse = GetTransformedMousePosition();
	Mouse.x = int(Mouse.x * 640/(fp32)m_ScreenSize.x); // GUI is always in 640x480
	Mouse.y = int(Mouse.y * 480/(fp32)m_ScreenSize.y);

	return m_spDesktop->GetCursorOverWnd(Mouse, _bOnlyTabstops);
}

//
void CWFrontEnd_Mod::Cube_UpdateMouse(CMWnd *pRootWnd)
{
	CPnt Mouse = GetTransformedMousePosition();
	Mouse.x = int(Mouse.x * 640/(fp32)m_ScreenSize.x); // GUI is always in 640x480
	Mouse.y = int(Mouse.y * 480/(fp32)m_ScreenSize.y);

	CMWnd *pWnd = pRootWnd->GetCursorOverWnd(Mouse, true);
	if(pWnd && pRootWnd != pWnd && (pWnd->m_Style & WSTYLE_TABSTOP))
	{
		pWnd->SetFocus();
		pWnd->m_bMouseOverWindow = true;
	}
	else
	{
		CMWnd *pFocus = pRootWnd->GetFocusWnd();
		if (pFocus)
			pFocus->m_bMouseOverWindow = false;
	}
	//	else
	//		pRootWnd->SetFocus(); // clear focus
}


spCMWnd CWFrontEnd_Mod::CreateWindowFromName(const char *_pName)
{
	// Create desktop
	int iRc = m_spMapData->GetResourceIndex_Registry(CStrF("GUI\\%s", m_CubeWndName.GetStr()));
	//int iRc = m_spMapData->GetResourceIndex_Registry("GUI\\ModWnd");

	CRegistry *pRc = m_spMapData->GetResource_Registry(iRc);

	if (pRc)
	{
		spCMWnd spWnd = CMWnd_Create(pRc, _pName);
		if (!spWnd)
			ConOutL(CStrF("(CWFrontEnd_Mod::CreateWindow) Could not create window %s", _pName));
		else
		{
			//m_spDesktop = spWnd;
			//				spWnd->SetFocus();
			spWnd->AddFont(m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("TEXT")), "TEXT");
			spWnd->AddFont(m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("HEADINGS")), "HEADINGS");
			spWnd->AddFont(m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("MONOPRO")), "SYSTEM");
			/*
			m_spDesktop->AddFont(m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("FORGOT")), "FORGOT");
			m_spDesktop->AddFont(m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("CLAIRVAUX")), "CLAIRVAUX");
			m_spDesktop->AddFont(m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("MINIFONT2")), "MINIFONT2");
			m_spDesktop->AddFont(m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("MINIFONT2")), "SYSTEM");
			m_spDesktop->AddFont(m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("CLAIRVAUX")), "MENU");
			m_spDesktop->AddFont(m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("CLAIRVAUX")), "NUMBERS");
			m_spDesktop->AddFont(m_spGUIData->GetResource_Font(m_spGUIData->GetResourceIndex_Font("TEXT")), "TEXT");
			//				m_spDesktop->AddFont(m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("MENU")), "MENU");
			//				m_spDesktop->AddFont(m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("NUMBERS")), "NUMBERS");
			*/
		}
		return spWnd;

	}
	return NULL;
}

static void PrecacheTexture(CTextureContext *_pTC, int _TextureID)
{
	_pTC->SetTextureParam(_TextureID, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
}

static void PrecacheFont(CRC_Font *_pFont, CTextureContext *_pTC)
{
	if (!_pFont)
		return;

	int nTex = _pFont->m_spTC->GetNumLocal();

	for (int i = 0; i < nTex; ++i)
	{
		PrecacheTexture(_pTC, _pFont->m_spTC->GetTextureID(i));
	}
}


void CWFrontEnd_Mod::DoPrecache(CRenderContext* _pRC, CXR_Engine *_pEngine)
{
	MACRO_GetSystem;

	m_bDonePrecache = true;
	m_spWData->Resource_AsyncCacheDisable(1<<3);

	bool bDoXDF = !CByteStream::XDF_GetRecord() && !CByteStream::XDF_GetUse();
	if (bDoXDF)
	{
		CStr XDFPath = m_spWData->ResolveFileName(CStr("XDF\\") + "GUIPrecache.XDF");

		if (D_MXDFCREATE == 2)
		{
			CStr XDFBase = pSys->GetEnvironment()->GetValue("XDF_PATH");
			if (XDFBase.Len() > 0)
				XDFPath = XDFBase + "\\" + "GUIPrecache.XDL";
			else
				XDFPath = m_spWData->ResolveFileName(CStr("XDF\\") + "GUIPrecache.XDL");

			if (!CDiskUtil::FileExists(XDFPath))
			{
				CDiskUtil::XDF_Record(XDFPath, m_spWData->ResolvePath(""));
			}
			else
			{
				ConOutL(CStrF("Skippng creation of XDF file, because it already exists (%s)", XDFPath.Str()));
			}
		}
		else if (CDiskUtil::FileExists(XDFPath))
		{
			CDiskUtil::XDF_Use(XDFPath, m_spWData->ResolvePath(""));
		}
	}

	OnPrecache(_pEngine);

	TArray<uint16> lPrecacheOrder;
	if (_pRC)
	{
		// Textures
		_pRC->Render_PrecacheFlush();
		_pRC->Texture_PrecacheFlush();
		_pRC->Geometry_PrecacheFlush();

		int SaveDynamicMipMaps = pSys->GetEnvironment()->GetValuei("R_DYNAMICLOADMIPS", 1);
		if (SaveDynamicMipMaps != 0)
		{
			ConExecute("r_dynamicloadmips(0);");
		}

		MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
		{
			int nTex = pTC->GetIDCapacity();
			lPrecacheOrder.SetLen(nTex);
			int nCount = 0;
			for (int i = 0; i < nTex; ++i)
			{
				if ((pTC->GetTextureFlags(i) & (CTC_TXTIDFLAGS_ALLOCATED | CTC_TXTIDFLAGS_PRECACHE)) == (CTC_TXTIDFLAGS_ALLOCATED | CTC_TXTIDFLAGS_PRECACHE))
				{
					lPrecacheOrder[nCount] = i;
					++nCount;
				}
			}
			lPrecacheOrder.QSort<CPrecacheTextureCompare>(pTC, nCount);
			_pRC->Texture_PrecacheBegin(nCount);
			for (int i = 0; i < nCount; ++i)
			{
				_pRC->Texture_Precache(lPrecacheOrder[i]);
			}
			_pRC->Texture_PrecacheEnd();
		}
		_pRC->Render_PrecacheEnd();

		if (SaveDynamicMipMaps != 0)
		{
			ConExecute(CStrF("r_dynamicloadmips(%d);", SaveDynamicMipMaps));
		}

		// Geometry
		MACRO_GetRegisterObject(CXR_VBContext, pVBC, "SYSTEM.VBCONTEXT");
		{
			int nVBs = pVBC->GetIDCapacity();
			lPrecacheOrder.SetLen(nVBs);
			int nCount = 0;
			for (int i = 0; i < nVBs; ++i)
			{
				if ((pVBC->VB_GetFlags(i) & (CXR_VBFLAGS_PRECACHE | CXR_VBFLAGS_ALLOCATED)) == (CXR_VBFLAGS_PRECACHE | CXR_VBFLAGS_ALLOCATED))
				{
					lPrecacheOrder[nCount] = i;
					++nCount;
				}
			}
			lPrecacheOrder.QSort<CPrecacheVBCompare>(pVBC, nCount);
			_pRC->Geometry_PrecacheBegin(nCount);
			for (int i = 0; i < nCount; ++i)
			{
				_pRC->Geometry_Precache(lPrecacheOrder[i]);
			}
			_pRC->Geometry_PrecacheEnd();
		}
	}

	// Sounds
	if (m_spSoundContext)
	{
		m_spSoundContext->Wave_PrecacheFlush();
		MACRO_GetRegisterObject(CWaveContext, pWC, "SYSTEM.WAVECONTEXT");
		int nCount = 0;
		int nWaves = pWC->GetIDCapacity();
		lPrecacheOrder.SetLen(nWaves);
		for (int i = 0; i < nWaves; ++i)
		{
			if (pWC->GetWaveIDFlag(i) & CWC_WAVEIDFLAGS_PRECACHE)
			{
				lPrecacheOrder[nCount] = i;
				++nCount;
			}
		}
		lPrecacheOrder.QSort<CPrecacheWaveCompare>(pWC, nCount);
		m_spSoundContext->Wave_PrecacheBegin(nCount);
		for (int i = 0; i < nCount; ++i)
		{
			m_spSoundContext->Wave_Precache(lPrecacheOrder[i]);
		}
		m_spSoundContext->Wave_PrecacheEnd(NULL, 0);
	}

	if (bDoXDF)
		CDiskUtil::XDF_Stop();

	m_spWData->Resource_AsyncCacheEnable(1<<3);
}


void PrecacheSound(CSC_SFXDesc *_pSound, CWaveContext *_pWC)
{
	if(_pSound && _pWC)
	{
		for (int i = 0; i < _pSound->GetNumWaves(); ++i)
		{
			int16 iWave = _pSound->GetWaveId(i);
			if(iWave >= 0)
			{
				_pWC->AddWaveIDFlag(iWave, CWC_WAVEIDFLAGS_PRECACHE);
			}
		}
	}

}

void CWFrontEnd_Mod::LoadAllResources()
{
	m_spMapData->GetResourceIndex_Font("TEXT");
	m_spMapData->GetResourceIndex_Font("HEADINGS");
	m_spMapData->GetResourceIndex_Font("MONOPRO");

	m_spMapData->GetResourceIndex_Sound("menu_ok");
}

void CWFrontEnd_Mod::OnPrecache(CXR_Engine* _pEngine)
{
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	MACRO_GetRegisterObject(CWaveContext, pWC, "SYSTEM.WAVECONTEXT");
	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");

	PrecacheFont(m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("TEXT")), pTC);
	PrecacheFont(m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("HEADINGS")), pTC);
	PrecacheFont(m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("MONOPRO")), pTC);

	if (!_pEngine)
	{
		CStr Splash;
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		CRegistry *pReg = pSys ? pSys->GetEnvironment() : NULL;
		if(pReg)
			Splash = pReg->GetValue("GUI_SPLASH");
		if(Splash == "")
			Splash = "GUI_Splash02";
		CXW_Surface *pSurf = pSC->GetSurface(Splash);
		pSurf->InitTextures(pTC, false);
		pSurf->SetTextureParam(CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);

		PrecacheTexture(pTC, pTC->GetTextureID("GUI_Splash02"));
		PrecacheTexture(pTC, pTC->GetTextureID("GUI_Splash02_01"));
		PrecacheTexture(pTC, pTC->GetTextureID("GUI_Splash02_02"));

		PrecacheTexture(pTC, pTC->GetTextureID("Special_Attenuation4"));
		PrecacheTexture(pTC, pTC->GetTextureID("Special_Normalize0"));
		PrecacheTexture(pTC, pTC->GetTextureID("Special_Specular016_0"));
	}

	PrecacheTexture(pTC, pTC->GetTextureID("special_defaultlens2d")); // needed in cube text-shadow projection

	PrecacheTexture(pTC, pTC->GetTextureID("GUI_Button_A"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_Button_B"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_Button_X"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_Button_Y"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_Cursor"));

	PrecacheTexture(pTC, pTC->GetTextureID("GUI_Arrow_Left"));	
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_Arrow_Right"));	
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_Arrow_Left_Pressed"));	
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_Arrow_Right_Pressed"));	

	PrecacheTexture(pTC, pTC->GetTextureID("GUI_Arrow_Up"));	
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_Arrow_Down"));	
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_Arrow_Up_pressed"));	
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_Arrow_Down_Pressed"));	

	PrecacheTexture(pTC, pTC->GetTextureID("GUI_SBZStamp"));	
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_Proto_Texttmp"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_Img_skull"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_Img_darkling"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_Img_item"));

	PrecacheTexture(pTC, pTC->GetTextureID("Special_Cube_FFFFFFFF"));
	PrecacheTexture(pTC, pTC->GetTextureID("Special_Cube_FF000000"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_Controller"));

	// Precache map-images
	PrecacheTexture(pTC, pTC->GetTextureID("journal_na")); // when journal mission image n/a TODO: REMOVE BEFORE RELEASE!
			
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_lvl0_01"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_lvl0_02"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_lvl0_03"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_lvl0_04"));

	PrecacheTexture(pTC, pTC->GetTextureID("GUI_lvl1_01"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_lvl1_02"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_lvl1_03"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_lvl1_04"));

	PrecacheTexture(pTC, pTC->GetTextureID("journal_OW_Guns"));
	PrecacheTexture(pTC, pTC->GetTextureID("journal_Guns"));

	// Todo: unhardcode power-images and fix in templase
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_creepingdark"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_demonarm"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_ancientweapons"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_blackhole"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_darknessvision"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_darknesshield"));

	// Todo: unhardcode darkling-images and fix in templase
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_berserker"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_gunner"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_lightkiller"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_kamikaze"));

	PrecacheTexture(pTC, pTC->GetTextureID("GUI_lvl2_01"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_lvl2_02"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_lvl2_03"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_lvl2_04"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_speakeron"));	
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_speakeroff"));	
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_HUD_Darkness"));	
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_mp01"));	
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_mp02"));	
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_mp03"));	
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_ctf01"));
	PrecacheTexture(pTC, pTC->GetTextureID("GUI_ctf02"));

	m_MapPieces.SetupAndPrecacheImages();

	PrecacheSound(m_spMapData->GetResource_SoundDesc(m_spMapData->GetResourceIndex_Sound("menu_ok")), pWC);
	//	PrecacheFont(m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("HEADINGS")), pTC);

	m_Cube.OnPrecache(_pEngine, m_pGameContext->m_spWServer);
	//m_Cube.SetGUIProcessProperties(m_Cube.GetGUIProcessContainer());
}


void CWFrontEnd_Mod::SetSoundContext(spCSoundContext _spSoundContext)
{
	CWFrontEnd::SetSoundContext(_spSoundContext);

	if (m_spSoundContext)
		m_Cube.InitSound(m_Cube.m_spMapData, m_spSoundContext, m_iChannel);
}


void CWFrontEnd_Mod::SetFrontEndInfo_r(CMWnd *_pWnd)
{
	CMWnd_FrontEndInfo *pCGI = _pWnd->GetFrontEndInfo();
	if (pCGI)
	{
		pCGI->SetClientGame(this);
		pCGI->OnPostSetClientGame();
	}

	for (int i = 0; i < _pWnd->GetNumItems(); ++i)
	{
		SetFrontEndInfo_r(_pWnd->GetItem(i));
	}
}

void CWFrontEnd_Mod::SetWindow(CStr _ID)
{
	MAUTOSTRIP(CWFrontEnd_Mod_SetWindow, MAUTOSTRIP_VOID);
//	M_TRACEALWAYS("[SetWindow] \"%s\"\n", _ID.Str());

	m_bSwapSide = true;
	m_iPrevWindow = m_iCurrentWindow;

	if(!m_Switching)
		m_Switching = m_Switching;

	if (_ID == "")
		m_spDesktop = NULL;
	else
		m_spDesktop = CreateWindowFromName(_ID);

	if (m_spDesktop != NULL)
	{
		SetFrontEndInfo_r(m_spDesktop);

		Cube_UpdateMouse(m_spDesktop);

		CMWnd_Message Msg(WMSG_COMMAND, "Init", 0);
		m_spDesktop->OnMessage(&Msg);
	}

	return;
}


//
//
//
void CWFrontEnd_Mod::Chooser_Init(CStr _Question)
{
	m_BackoutCommand = "cg_prevmenu()";
	m_Question = _Question;
	m_lChoices.Clear();
	m_lChoicesCommands.Clear();
}

//
//
//
void CWFrontEnd_Mod::Chooser_AddChoice(CStr _Name, CStr _Cmd)
{
	m_lChoices.Add(_Name);
	m_lChoicesCommands.Add(_Cmd);
}

//
//
//
void CWFrontEnd_Mod::Chooser_Invoke(bool _Sub)
{
	if(_Sub)
		ConExecute("cg_submenu(\"chooser\")");
	else
		ConExecute("cg_switchmenu(\"chooser\")");
}

//
//
//
CPnt CWFrontEnd_Mod::GetPointFromReg(const CRegistry *pReg, const char *_pName) const
{
	const CRegistry *pTempReg = pReg->Find(_pName);
	CPnt Pos;
	if (pTempReg)
	{
		//		CStr PosStr = pTempReg->GetThisValue();
		CFStr PosStr = pTempReg->GetThisValue();
		Pos.x = PosStr.GetStrSep(",").Val_int();
		Pos.y = PosStr.GetStrSep(",").Val_int();
	}

	return Pos;
}

void CWFrontEnd_Mod::OnBuildInto(CRenderContext* _pRC)
{
	Error("OnBuildInto", "Do not use, borken!");
	// clear buffer
	{
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if(!pSys || !pSys->m_spDisplay)
			return;

		pSys->m_spDisplay->ClearFrameBuffer(CDC_CLEAR_COLOR | CDC_CLEAR_ZBUFFER | CDC_CLEAR_STENCIL, CPixel32(64,64,64,255));
	}

	//	CWFrontEnd::OnRender(_pRC);
}

void RenderCallback(int _iLocal, class CRenderContext* _pRC, void *_pData)
{
	Error_static("RenderCallBack", "Do not use, borken!");
	CWFrontEnd_Mod *pFrontEnd = (CWFrontEnd_Mod*)_pData;
	pFrontEnd->OnBuildInto(_pRC);
}

//
//
//
#if 0
static void CreateRectTVerts(CVec2Dfp32* _pTV, const CVec2Dfp32& _UVMin, const CVec2Dfp32& _UVMax)
{
	_pTV[0][0] = _UVMax[0];		_pTV[0][1] = _UVMax[1];
	_pTV[1][0] = _UVMin[0];		_pTV[1][1] = _UVMax[1];
	_pTV[2][0] = _UVMin[0];		_pTV[2][1] = _UVMin[1];
	_pTV[3][0] = _UVMax[0];		_pTV[3][1] = _UVMin[1];
}

static void CreateRectTVertsInv(CVec2Dfp32* _pTV, const CVec2Dfp32& _UVMin, const CVec2Dfp32& _UVMax)
{
	_pTV[0][0] = _UVMax[0];		_pTV[0][1] = 1.0f - _UVMax[1];
	_pTV[1][0] = _UVMin[0];		_pTV[1][1] = 1.0f - _UVMax[1];
	_pTV[2][0] = _UVMin[0];		_pTV[2][1] = 1.0f - _UVMin[1];
	_pTV[3][0] = _UVMax[0];		_pTV[3][1] = 1.0f - _UVMin[1];
}
#endif


//
//
//
void CWFrontEnd_Mod::Cube_Render(CRenderContext* _pRC, CXR_VBManager* _pVBM, bool _IsJournal, bool _SpecialOverride)
{
	if(_SpecialOverride)
	{
		m_Cube.Render_P5(_pRC, _pVBM, m_Cube.m_FrameTime, true, GetRender_Engine(), m_GameIsLoading, true);
		return;
	}

	// transfer surface
	int32 iSurface = m_spMapData->GetResourceIndex_Surface(m_Cube.m_SurfaceName);
	m_Cube.m_pSurface = m_spMapData->GetResource_Surface(iSurface);		// Resource-mapping for client-game. 
		
	bool InGame = m_pGameContext->m_lspWClients.Len() > 0;
	//CXR_Engine* pEngine = GetRender_Engine();

	// TODO: move this to the cube class (funktion?)
	{

		m_Cube.m_GUIFlags &= ~CCube::GUI_FLAGS_LOADINGSCENERUNNING;
		m_Cube.m_GUIFlags &= ~CCube::GUI_FADE_IN_IN_PROGRESS;
		m_Cube.m_GUIFlags &= ~CCube::GUI_FADE_OUT_IN_PROGRESS;
	}

	m_Cube.Render_P5(_pRC, _pVBM, m_Cube.m_FrameTime, (m_pGameContext->m_lspWClients.Len() > 0), GetRender_Engine(),m_GameIsLoading, false);

	//m_Cube.Render(_pRC, _pVBM, m_Cube.m_FrameTime*0.75f);
}


void CWFrontEnd_Mod::Cube_SetSecondaryRect(CRct _Rect)
{
	m_Cube.m_aSides[m_Cube.m_CurrentSide].SetSecondaryArea(_Rect.p0.x, _Rect.p0.y, _Rect.p1.x-_Rect.p0.x, _Rect.p1.y-_Rect.p0.y);
}

/*
void CWFrontEnd_Mod::Cube_SetSecondaryTexture(CStr _TexName)
{
MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
if(pTC)
Cube_SetSecondaryTexture(pTC->GetTextureID(_TexName));
//
}*/


void CWFrontEnd_Mod::Cube_SetSecondary(int32 _iSomething, int32 _iBlend)
{
	m_Cube.m_aSides[m_Cube.m_CurrentSide].SetSecondary(_iSomething, _iBlend);
}


//
//
//
void CWFrontEnd_Mod::Cube_ShouldWorldRender(bool _Yes)
{
	if (!m_pGameContext)
		return;

	// disable game rendering
	CWObject_Message Msg(OBJMSG_GAME_SETRENDERWORLD, _Yes);
	for(int i = 0; i < m_pGameContext->m_lspWClients.Len(); i++)
	{
		CWorld_Client* pC = m_pGameContext->m_lspWClients[i];
		if (pC)
			pC->ClientMessage_SendToObject(Msg, pC->Game_GetObjectIndex());
	}
}

void CWFrontEnd_Mod::Cube_Raise(CMWnd *pWnd)
{
	if(!pWnd)
		return;
	if(!pWnd->GetParent())
		return;
	if(pWnd->m_Style&WSTYLE_HIDDENFOCUS)
		return;

	for(int32 y = pWnd->m_AbsPos.p0.y/(480/CCube::CUBE_RES); y < pWnd->m_AbsPos.p1.y/(480/CCube::CUBE_RES); y++)
	{
		if(y >= 20)
			break;
		if(y < 0)
			continue;

		int32 StartX = pWnd->m_AbsPos.p0.x/(640/CCube::CUBE_RES);
		int32 EndX = pWnd->m_AbsPos.p1.x/(640/CCube::CUBE_RES);

		for(int32 x = StartX; x < EndX; x++)
		{
			if(x >= 20)
				break;
			if(x < 0)
				continue;

			//			fp32 a = (x-StartX)/(fp32)(EndX-StartX);
			int32 iActive = m_Cube.m_aSides[m_Cube.m_CurrentSide].ActivateElement(x, y);
			if(iActive != -1)
			{
				m_Cube.m_aActiveData[iActive].m_Scale = 0.25f + Random*0.75f;
				m_Cube.m_aActiveData[iActive].m_WantedDepth =	0.018f + 0.015f;
				//0.015f*((M_Sin(fTime*TIMESCALE_ACTIVEPULSE+a*4)+1)/2.0f);
			}
		}
	}
}


void CWFrontEnd_Mod::Cube_SetStateFromLayout(CMWnd *_pWnd, bool _Instant)
{
	// update layout
	CMWnd_CubeLayout *pLayout = TDynamicCast<CMWnd_CubeLayout>(_pWnd);
	if(!pLayout)
		return;

	m_Cube.m_SpeedModifyer = 1;
	m_Cube.m_aSides[m_Cube.m_CurrentSide].SetMap((CCellInfo*)pLayout->m_aMap, false, _Instant);
	m_Cube.m_aSides[m_Cube.m_CurrentSide].SetDepthMap((fp32*)pLayout->m_aDepth);

	m_Cube.m_SpeedModifyer = 5;
	for(int32 s = 0; s < 6; s++)
		if(s != m_Cube.m_CurrentSide)
		{
			m_Cube.m_aSides[s].SetDepthMap(NULL);
			m_Cube.m_aSides[s].SetMap(NULL, false, _Instant);
		}
		m_Cube.m_SpeedModifyer = 1;

		// selection
		CMWnd *pWnd = _pWnd->GetFocusWnd();
		if(pWnd)
		{
			if(pWnd->m_Group.Len() && pWnd->GetParent())
			{
				CStr Group = pWnd->m_Group;
				CMWnd *pParent = pWnd->GetParent();

				for(int32 i = 0; i < pParent->GetNumItems(); i++)
				{
					pWnd = pParent->GetItem(i);
					if(pWnd && pWnd->m_Group == Group)
						Cube_Raise(pWnd);
				}
			}
			else
				Cube_Raise(pWnd);
		}
}

//
//
//
/*
void CWFrontEnd_Mod::Cube_DoLoadingLayout()
{
static int32 Counter = 0;
static CMTime BlinkTime;

CMWnd_CubeLayout Layout;
CRct TextPos;
TextPos.p0.x = 0;
TextPos.p1.x = 20;
TextPos.p0.y = 2;
TextPos.p1.y = 4;


if(CMTime::GetCPU().Compare(BlinkTime) == 1)
{
Counter++;
BlinkTime = CMTime::GetCPU()+CMTime::CreateFromSeconds(0.5f);
}

//fp32 Time = CMTime::GetCPU()-LastTime).Modulus(1).GetTime();

m_Cube.SetSequence(&m_Cube.m_Seq_normal);
m_Cube.ViewSide(0, true);
for(int32 i = 0; i < 6; i++)
m_Cube.m_aSides[i].m_iSecondaryID = -1;

for(int32 i = 0; i < 20; i++) // force fast switch
{
Layout.m_aMap[15][i].Flags() |= CCellInfo::FLAG_FASTSWITCH;
m_Cube.m_aSides[0].m_aElements[i][15].m_Flags |= CCube::FLAG_FASTSWITCH;
}

Layout.Layout_WriteText(TextPos, "1hc, Loading", CMWnd_CubeLayout::FLAG_NO_CONVERT);

if(Counter%2)
{
Layout.Layout_SetMapping(10-2, 13, 16*27-1, CCellInfo::FLAG_FASTSWITCH);
Layout.Layout_SetMapping(10-4, 13, 16*27-1, CCellInfo::FLAG_FASTSWITCH);
Layout.Layout_SetMapping(10+1, 13, 16*27-1, CCellInfo::FLAG_FASTSWITCH);
Layout.Layout_SetMapping(10+3, 13, 16*27-1, CCellInfo::FLAG_FASTSWITCH);
}


if(m_pGameContext != NULL && m_pGameContext->m_spWData != NULL)
{
fp32 Amount = m_pGameContext->m_spWData->XDF_GetPosition()/(fp32)m_pGameContext->m_spWData->XDF_GetSize();
int32 Dots = (int32)(Amount*18);

for(int32 i = 0; i < Dots; i++)
Layout.Layout_SetMapping(1+i, 15, 16*27-1, CCellInfo::FLAG_FASTSWITCH);
}

//Counter++;
//Counter %= 2;
m_Cube.m_aSides[0].SetMap((CCellInfo*)Layout.m_aMap, false, true);
}*/

void CWFrontEnd_Mod::Cube_InitLoadingCubeRemoval()
{
	m_Cube.SetSequence(&m_Cube.m_Seq_removeloading);
}


bool CWFrontEnd_Mod::Cube_LoadingIsTakingToMuchTime()
{
	if((CMTime::GetCPU()-m_LoadingTimeout).GetTime() > 0.25f)
		return true;
	return false;
}

//
//
void CWFrontEnd_Mod::Cube_DoLoadingLayout()
{
	int32 Counter = 1000;
	static CMTime BlinkTime;

	CMWnd_CubeLayout Layout;
	CRct TextPos;
	TextPos.p0.x = 0;
	TextPos.p1.x = 20;
	TextPos.p0.y = 2;
	TextPos.p1.y = 4;

	{
		// disable vibration
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if (pSys && pSys->m_spInput != NULL)
			pSys->m_spInput->FlushEnvelopes();
	}
	if(m_pGameContext != NULL && m_pGameContext->m_spWData != NULL)
	{
#ifdef PLATFORM_XBOX1
		int32 pos = m_pGameContext->m_spWData->XDF_GetPosition();
		int32 size = m_pGameContext->m_spWData->XDF_GetSize();
		fp32 Amount = pos/(fp32)size;
		Counter *= Amount;
		if(size-pos > 0)
			m_LoadingTimeout = CMTime::GetCPU();
#else

		int32 size = m_pGameContext->m_spWData->XDF_GetSize();
		if (size)
		{
			int32 pos = m_pGameContext->m_spWData->XDF_GetPosition();
			fp32 Amount = pos/(fp32)size;
			Counter = int32(Counter * Amount);
			if(size-pos > 0)
				m_LoadingTimeout = CMTime::GetCPU();
		}
		else
		{

			fp32 Status = m_pGameContext->m_spWServer ? m_pGameContext->m_spWServer->Load_GetProgress() : 0.0f;
			fp32 StatusClient = 0.0f;
			if (m_pGameContext->m_lspWClients.Len() > 0 && m_pGameContext->m_lspWClients[0])
				StatusClient = m_pGameContext->m_lspWClients[0]->Load_GetProgress();

			Status = (Status + StatusClient) * 0.5f;
			//ConOutL(CStrF("Time: %f LoadStatus: %f Client: %f", CMTime::GetCPU().GetTime(),Status, StatusClient));
			Counter = int32(Counter * Status);

			if (Status > 0.0f)
				m_LoadingTimeout = CMTime::GetCPU();
		}
#endif
	}

	/*
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys)
	pSys->m_pWSer
	pSys->*/

	//	if(CMTime::GetCPU().Compare(BlinkTime) == 1)
	//	{
	//Counter+=3;
	//		BlinkTime = CMTime::GetCPU()+CMTime::CreateFromSeconds(0.01f);
	//	}
	//Counter++;

	//Counter %= 1000;

	//fp32 Time = CMTime::GetCPU()-LastTime).Modulus(1).GetTime();

	//m_Cube.m_MoveTime = 1;
	m_Cube.SetSequence(&m_Cube.m_Seq_normal);
	m_Cube.ViewSide(0, true); 
	for(int32 i = 0; i < 6; i++)
	{
		if(i != m_Cube.m_CurrentSide)
			m_Cube.m_aSides[i].Reset();
		m_Cube.m_aSides[i].m_iSecondaryID = -1;
	}

	for(int32 i = 0; i < 20; i++) // force fast switch
	{
		Layout.m_aMap[15][i].Flags() |= CCellInfo::FLAG_FASTSWITCH;
		m_Cube.m_aSides[0].m_aElements[i][15].m_Flags |= CCube::FLAG_FASTSWITCH;
	}

	//Layout.Layout_WriteText(TextPos, "1hc, Loading", CMWnd_CubeLayout::FLAG_NO_CONVERT);
	//	const int32 aPortions[5] = {360, 280, 200, 120, 40};
	//	int32 c = Counter;
	/*for(int32 r = 0; r < 5; r++)
	{
	//fp32 TotalAmount = Clamp01((Counter-r*100)/200.0f);
	fp32 TotalAmount = (c)/(fp32)aPortions[r];
	c -= aPortions[r];
	r *= 2;
	//TotalAmount = 1;
	fp32 Amount = 4-TotalAmount*4;
	fp32 a;
	int32 ir = 10-r;

	a = Clamp01(Amount);
	if(a > 0.01f)
	for(int32 x = 2; x < a*(ir*2); x+=2)
	Layout.Layout_SetMappingBig(r+x, r, 16*15, CCellInfo::FLAG_FASTSWITCH);

	Amount -= 1;
	a = Clamp01(Amount);
	if(a > 0.01f)
	for(int32 x = 2; x < a*(ir*2); x+=2)
	Layout.Layout_SetMappingBig(18-r, r+x, 16*15, CCellInfo::FLAG_FASTSWITCH);

	Amount -= 1;
	a = Clamp01(Amount);
	if(a > 0.01f)
	for(int32 x = 2; x < a*(ir*2); x+=2)
	Layout.Layout_SetMappingBig(r+ir*2-x-2, 18-r, 16*15, CCellInfo::FLAG_FASTSWITCH);

	Amount -= 1;
	a = Clamp01(Amount);
	if(a > 0.01f)
	for(int32 x = 2; x < a*(ir*2); x+=2)
	Layout.Layout_SetMappingBig(r, r+ir*2-x-2, 16*15, CCellInfo::FLAG_FASTSWITCH);
	r /= 2;
	}*/




	//m_Cube.m_Current.Unit();
	m_Cube.SetSequence(&m_Cube.m_Seq_normal);
	m_Cube.m_SequenceOverride = 2;
	m_Cube.m_MoveTime = -10;
	m_Cube.m_SequenceTime = 0;

	{
		m_Cube.SetDefaultSequenceParams();

		// overrides
		m_Cube.m_LightIntens = 1;
		m_Cube.m_LightRange = 8;

	}

	CMat4Dfp32 Trans, Rotation, Rotation2, Temp;
	Trans = m_Cube.CreateTranslationMatrix(0.0f, 1.5f, -2.5f);
	m_Cube.CreateRotationMatrix(0.1f, 0, -_PI/4, &Rotation);
	m_Cube.CreateRotationMatrix(0.2f, 0, 0, &Rotation2);
	Rotation.Multiply(Rotation2, Temp);
	Temp.Multiply(Trans, m_Cube.m_Current);	


	//Counter++;
	//Counter %= 2;
	m_Cube.m_aSides[0].SetMap((CCellInfo*)Layout.m_aMap, false, true);
}

//
//
//
void CWFrontEnd_Mod::Cube_Update(CMWnd *_pWnd)
{
	// Jakob: varför i hela friden skulle man behöva göra såhär?!
	m_Cube.InitSound(m_spMapData, m_spSoundContext, m_iChannel); // so stupid..

	if(m_pCurrentWnd != _pWnd)
		m_bSwapSide = true;

	if(m_pCurrentWnd == NULL && m_OutOfGUI)
		;
	else
	{
		//m_ShouldRender = true;

		m_Cube.Update();

		/*
		if(m_OutOfGUI)
		{
		// returning to the cube
		m_Cube.Reset();

		m_Cube.m_Current.Unit();
		m_Cube.ViewSide(2, true);

		//m_OutOfGUI = false;
		m_bSwapSide = true;

		}*/

		// side swapping
		if(m_bSwapSide && m_Cube.m_CanMove)
		{
			if (m_iPrevWindow != -1)
			{
				if (m_iPrevWindow < m_iCurrentWindow)
				{
					m_Cube.PlaySoundFXEvent(CCube::SOUNDEVENT_TENTACLES_CHANGEPAGE);
				}
				else if (m_iPrevWindow >= m_iCurrentWindow)
				{
					m_Cube.PlaySoundFXEvent(CCube::SOUNDEVENT_TENTACLES_BACK);
					m_Cube.PlaySoundFXEvent(CCube::SOUNDEVENT_TENTACLES_CHANGEPAGE);
				}
			}

			// clean the active side
			m_Cube.m_aSides[m_Cube.m_CurrentSide].SetDepthMap(NULL);
			m_Cube.m_aSides[m_Cube.m_CurrentSide].SetMap(NULL, false);

			// switch to new side
			if(m_QueuedSide >= 0)
			{
				m_Cube.ViewSide(m_QueuedSide, false);
				m_QueuedSide = -1;
			}
			else
				m_Cube.Move((int)(Random*4));

			m_bSwapSide = false;

			// make noise
			m_Cube.PlayEvent(CCube::SOUNDEVENT_ROTATESLOW);
		}

		/*
		if(m_pCurrentWnd == NULL)
		{
		// cube is no longer needed
		m_OutOfGUI = true;
		}*/
	}

	m_pCurrentWnd = _pWnd;
}

bool CWFrontEnd_Mod::Cube_ShouldRender()
{
	return m_ShouldRender;
	//if(m_spCurrentWnd == NULL && m_OutOfGUI)
	//	return false;
	//return true;
}

void CWFrontEnd_Mod::DoClientRenderCheck()
{
	// not out of gui, exit is not finished and we still need to grab a screen
	if(((m_CubeReferneces && !m_OutOfGUI) || !(m_Cube.m_GUIFlags & CCube::GUI_FLAGS_EXITCOMPLETE)) && (m_Cube.m_GUIFlags & CCube::GUI_FLAGS_SCREENGRAB_OK))
		Cube_ShouldWorldRender(false);
	else
		Cube_ShouldWorldRender(true);
}

void CWFrontEnd_Mod::OnRefresh()
{
	CWFrontEnd::OnRefresh();

	if(CurrentMenu().Find("attract") != -1)
		m_AttractTime = CMTime::GetCPU();


#ifdef PLATFORM_CONSOLE

	if(CanDoAttract())
	{
		if ((CMTime::GetCPU() - m_AttractTime).GetTime() > 45.0f)
		{
			//			bool Ingame = false;
			//			bool IsLoading = false;

			//if((m_spFrontEnd != NULL && m_spFrontEnd->m_spDesktop != NULL) || !bClientInteractive)

			CWorld_Client* pC = m_pGameContext->GetCurrentClient();
			//if((pC && pC->GetClientState() != WCLIENT_STATE_INGAME) || (m_spWServer != NULL && m_spWServer->IsLoading()))
			//	IsLoading = true;

			bool bClientInteractive = false;
			if(pC)
				bClientInteractive = pC->GetInteractiveState() != 0;

			if(!(pC && pC->GetClientState() == WCLIENT_STATE_INGAME) && !bClientInteractive)
				ConExecute("cg_rootmenu(\"attract\")");
		}
	}
	else
		m_AttractTime = CMTime::GetCPU();
#endif


	//	if(m_CubeReferneces)

	//m_CubeUser
	if(m_spDesktop != NULL)
	{
		if(m_spDesktop->m_Script_Update.Len())	
			ConExecuteImp(m_spDesktop->m_Script_Update, __FUNCTION__, 0);
	}

	//
	//	bool InGame = m_pGameContext->m_lspWClients.Len() > 0;
	int32 DoRender = 0;
	//	bool ShowingStuff = m_spDesktop != NULL;

	if (m_GameIsLoading)
		m_ShouldRender = false;

	if(m_CubeReferneces && !m_GameIsLoading)
	{
		DoRender = 1;
		m_ShouldRender = true;
		//Cube_ShouldWorldRender(false);
	}
	else
	{
		//if(!m_OutOfGUI)
		DoRender = 0;
		//Cube_ShouldWorldRender(true);
	}

	//
	if(DoRender)
	{
		if(m_OutOfGUI)
		{
			//_pRC->Texture_Precache(_pRC->Texture_GetTC()->GetTextureID("GUI_Splash01"));
			//_pRC->Texture_PrecacheEnd();


			// precache font
			/*
			CRC_Font *pFont = GetFont("CLAIRVAUX");
			if(pFont)
			_pRC->Texture_Precache(pFont->m_TextureID);
			*/

			//int32 iSurface = m_spMapData->GetResourceIndex_Surface(m_Cube.m_SurfaceName);
			//m_Cube.m_pSurface = m_spMapData->GetResource_Surface(iSurface);		// Resource-mapping for client-game. 


			//
			m_Cube.Reset();

			m_Cube.m_Current.Unit();
			m_Cube.ViewSide(2, true);

			//m_OutOfGUI = false;
			m_bSwapSide = true;

			//			if(InGame)
			//				GrabScreen(_pRC);
		}
	}

	if(m_CubeReferneces)
		m_OutOfGUI = false;
	else
	{
		if(m_OutOfGUI) // if we really are out of game
			m_ShouldRender = false;
		m_OutOfGUI = true;
	}

}

//
// this function doubles as an update rutine for the cube
//
void CWFrontEnd_Mod::OnRender(CRenderContext* _pRC, CXR_VBManager* _pVBM)
{
	MSCOPESHORT(CWFrontEnd_Mod::OnRender);
	//m_ShouldRender && ;
	if(!m_bDonePrecache)
		DoPrecache(_pRC, m_pEngine);

	CWFrontEnd::OnRender(_pRC, _pVBM);

	// render reference points
	if(((!m_OutOfGUI || m_ClientMouse) && !m_GameIsLoading) || (m_spDesktop && m_spDesktop->m_bShowMouse))
		//if(m_spDesktop != NULL)
	{
		CRC_Util2D Util;
		CRC_Viewport *pViewport = _pVBM->Viewport_Get();
		Util.Begin(_pRC, pViewport, _pVBM);
		Util.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
		Util.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		CClipRect ClipRect(pViewport->GetViewRect());

		CDisplayContext *pDC = _pRC->GetDC();
		m_ScreenSize	= pDC->GetScreenSize();
#ifndef PLATFORM_CONSOLE
		Util.DrawTexture(ClipRect, CPnt(m_MousePosition.x, m_MousePosition.y-2), "GUI_Cursor", 0xffb0b0b0);
#endif

		/*
		CPnt Pos[4];
		for(int32 i = 0; i < 4; i++)
		Pos[i] = CPnt(m_Cube.m_aReferencePoints[i].k[0], m_Cube.m_aReferencePoints[i].k[1]);
		Util.Line(ClipRect, Pos[0], Pos[1], 0xFFFFFFFF);
		Util.Line(ClipRect, Pos[0], Pos[2], 0xFFFFFFFF);
		Util.Line(ClipRect, Pos[3], Pos[1], 0xFFFFFFFF);
		Util.Line(ClipRect, Pos[3], Pos[2], 0xFFFFFFFF);


		//Util.Line(ClipRect, TransMouse, TransMouse+CPnt(10,0), 0xFF0000FF);
		//Util.Line(ClipRect, TransMouse, TransMouse+CPnt(0,10), 0xFF0000FF);
		//Util.Line(ClipRect, TransMouse, TransMouse+CPnt(20,20), 0xFF0000FF);



		for(int32 i = 0; i < 3; i++)
		{
		CPnt Pos(320+m_Cube.m_aReferencePoints[i].k[0] -8, 240-m_Cube.m_aReferencePoints[i].k[1] -8);
		CRct Rect;
		Rect.p0 = Pos;
		Rect.p1 = Pos;
		Rect.p1.x += 16;
		Rect.p1.y += 16;
		Util.DrawTexture(ClipRect, Pos, "GUI_Notification");
		}

		//if(pGameMod && pGameMod->m_FriendsHandler.GotRequest() && m_spDesktop != NULL && m_CubeReferneces)
		{
		CClipRect ClipRect(0,0,640,480);
		CPnt Pos(640-640*0.15f/2-32, 160);
		CRct Rect;
		Rect.p0 = Pos;
		Rect.p1 = Pos;
		Rect.p1.x += 32;
		Rect.p1.y += 32;

		Util.DrawTexture(ClipRect, Pos, "GUI_FriendReq");
		}*/


		//if(m_spDesktop != NULL && m_spDesktop->m_Interactive && pGameMod && pGameMod->m_ExtraContent_Live.GetStatusText().Len())
		/*if(0)
		{
		CStr Text = Localize_Str(pGameMod->m_ExtraContent_Live.GetStatusText());
		int TextColorM = CPixel32(95,90,70, 0xff);//0xffdddddd; 
		int TextColorH = 0x60ffffff;
		int TextColorD = 0x60000000;						

		CRC_Font *pFont = m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("HEADINGS"));
		//if(pGameMod->m_ExtraContent_Live.GetLoginStatus() == CLiveHandler::STATUS_SUCCESS)
		//	CStr Text(TEXT("Signed In: %s"), pGameMod->m_ExtraContent_Live.GetActiveUser()->szGamertag);

		if(pFont)
		{
		fp32 w = pFont->GetWidth(16, Text);
		fp32 h = pFont->GetHeight(16, Text);
		Util.Text(ClipRect, pFont, 640*0.15f/2, 480*0.15f/2+1, Text, 0xFF000000, 16);
		Util.Text(ClipRect, pFont, 640*0.15f/2, 480*0.15f/2-1, Text, 0xFF000000, 16);
		Util.Text(ClipRect, pFont, 640*0.15f/2+1, 480*0.15f/2, Text, 0xFF000000, 16);
		Util.Text(ClipRect, pFont, 640*0.15f/2-1, 480*0.15f/2, Text, 0xFF000000, 16);
		Util.Text(ClipRect, pFont, 640*0.15f/2, 480*0.15f/2, Text, 0x80ffffff, 16);
		}
		}*/

		Util.End();

	}
	else
	{	
		// Used for rendering tentacles when they're moving out
		if((m_pGameContext->m_lspWClients.Len() > 0) && !m_GameIsLoading) // meaning "is in-game". and not loading 
		{
			m_Cube.Update();
			m_Cube.Render_P5(_pRC, _pVBM, m_Cube.m_FrameTime, true, GetRender_Engine(), false, true);
		}
	}


	if(m_pGameContext)
	{
		//pFrontEndMod = TDynamicCast<CWFrontEnd_Mod>((CWFrontEnd*)pGame->m_spFrontEnd); // UGLY

#if defined(PLATFORM_XBOX1) && !defined(M_DEMO_XBOX)
		CGameContextMod *pGameMod = TDynamicCast<CGameContextMod>(m_pGameContext); // UGLY
		CRC_Util2D Util;
		CRC_Viewport *pViewport = _pVBM->Viewport_Get();
		Util.Begin(_pRC, pViewport, _pVBM);
		Util.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		Util.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE);

		CClipRect ClipRect(0,0,640,480);

		if(pGameMod && (pGameMod->m_FriendsHandler.ShowInviteIcon() ||
			(pGameMod->m_FriendsHandler.GotInvite() && m_spDesktop != NULL && m_CubeReferneces)))
		{
			CPnt Pos(640-640*0.15f/2-32, 130);
			CRct Rect;
			Rect.p0 = Pos;
			Rect.p1 = Pos;
			Rect.p1.x += 32;
			Rect.p1.y += 32;

			Util.DrawTexture(ClipRect, Pos, "GUI_Notification");
		}

		if(pGameMod && pGameMod->m_FriendsHandler.GotRequest() && m_spDesktop != NULL && m_CubeReferneces)
		{
			CClipRect ClipRect(0,0,640,480);
			CPnt Pos(640-640*0.15f/2-32, 160);
			CRct Rect;
			Rect.p0 = Pos;
			Rect.p1 = Pos;
			Rect.p1.x += 32;
			Rect.p1.y += 32;

			Util.DrawTexture(ClipRect, Pos, "GUI_FriendReq");
		}


		//if(m_spDesktop != NULL && m_spDesktop->m_Interactive && pGameMod && pGameMod->m_ExtraContent_Live.GetStatusText().Len())
		if(0)
		{
			CStr Text = Localize_Str(pGameMod->m_ExtraContent_Live.GetStatusText());
			int TextColorM = CPixel32(95,90,70, 0xff);//0xffdddddd; 
			int TextColorH = 0x60ffffff;
			int TextColorD = 0x60000000;						

			CRC_Font *pFont = m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("HEADINGS"));
			//if(pGameMod->m_ExtraContent_Live.GetLoginStatus() == CLiveHandler::STATUS_SUCCESS)
			//	CStr Text(TEXT("Signed In: %s"), pGameMod->m_ExtraContent_Live.GetActiveUser()->szGamertag);

			if(pFont)
			{
				fp32 w = pFont->GetWidth(16, Text);
				fp32 h = pFont->GetHeight(16, Text);
				Util.Text(ClipRect, pFont, 640*0.15f/2, 480*0.15f/2+1, Text, 0xFF000000, 16);
				Util.Text(ClipRect, pFont, 640*0.15f/2, 480*0.15f/2-1, Text, 0xFF000000, 16);
				Util.Text(ClipRect, pFont, 640*0.15f/2+1, 480*0.15f/2, Text, 0xFF000000, 16);
				Util.Text(ClipRect, pFont, 640*0.15f/2-1, 480*0.15f/2, Text, 0xFF000000, 16);
				Util.Text(ClipRect, pFont, 640*0.15f/2, 480*0.15f/2, Text, 0x80ffffff, 16);
			}
		}

		Util.End();
#endif
	}

	if (m_bShowConfidential)
	{
		CRC_Font *pFont = m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("HEADINGS"));
		if (pFont)
		{
			_pVBM->ScopeBegin("Confidential Text", false);
			CRC_Util2D Util;
			CRC_Viewport* pViewport = _pVBM->Viewport_Get();
		
			Util.Begin(_pRC, pViewport, _pVBM);
			Util.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			Util.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
			
			CClipRect ClipRect(pViewport->GetViewRect());

			const char* pMessage = "CONFIDENTIAL - NOT FOR PUBLIC DISPLAY";
			const fp32 Size = 16.0f;
			fp32 sw = ClipRect.GetWidth();
			fp32 sh = ClipRect.GetHeight();
			fp32 tw = pFont->GetWidth(Size, pMessage);
			fp32 th = pFont->GetHeight(Size, pMessage);
			CPnt Pos((int)((sw-tw)*0.5f), (int)(sh*0.95f - th));

			Util.Text(ClipRect, pFont, Pos.x, Pos.y, pMessage, 0xFFFFFFFF, Size);
			Util.End();
			_pVBM->ScopeEnd();			
		}
	}
}

void CWFrontEnd_Mod::GrabScreen()
{
	m_bGrabScreen = true;
}

void CWFrontEnd_Mod::GrabScreen(CRenderContext* _pRC)
{
	if(m_spGameTexture == NULL)
	{
		MRTC_SAFECREATEOBJECT_NOEX(spTC, "CTextureContainer_Screen", CTextureContainer_Screen);
		m_spGameTexture = spTC;
		m_spGameTexture->Create(1, NULL);
	}
	CPnt ScreenSize = _pRC->GetDC()->GetScreenSize();
	uint32 Flags = _pRC->Caps_Flags();

	if (!(Flags & CRC_CAPS_FLAGS_ARBITRARY_TEXTURE_SIZE))
	{
		ScreenSize = CPnt(GetGEPow2(ScreenSize.x), GetGEPow2(ScreenSize.y));
	}

	m_spGameTexture->PrepareFrame(ScreenSize);

	m_spGameTexture->GetSnapshot(0, _pRC);
}

bool CWFrontEnd_Mod::GrabScreenDone()
{
	if(m_PrevScreenGrabRef > 0)
		return true;
	return false;
}

//
//
//
void CWFrontEnd_Mod::Con_CG_CubeSeq(CStr _Seq)
{
	M_LOCK(m_FrontEndLock);
	if(_Seq == "inside")
		m_Cube.SetSequence(&m_Cube.m_Seq_inside);
	else if(_Seq == "startup")
		m_Cube.SetSequence(&m_Cube.m_Seq_startup);
	else if(_Seq == "normal")
		m_Cube.SetSequence(&m_Cube.m_Seq_normal);
	else if(_Seq == "moveout")
		m_Cube.SetSequence(&m_Cube.m_Seq_moveout);
	else if(_Seq == "fromgame")
		m_Cube.SetSequence(&m_Cube.m_Seq_fromgame);
	else if(_Seq == "backtogame")
		m_Cube.SetSequence(&m_Cube.m_Seq_backtogame);
	else if(_Seq == "beginload")
		m_Cube.SetSequence(&m_Cube.m_Seq_beginload);

	m_Cube.UpdateSequence(0);
}

void CWFrontEnd_Mod::Con_CG_SelectSaveUnit()
{
	MACRO_GetSystem;
	CContentContext* pSaveGames = pSys->GetContentContext(CONTENT_SAVEGAMES);
	if(pSaveGames) pSaveGames->StorageSelect(0, 8*1024*1024);	// Use proper amount here
}

void CWFrontEnd_Mod::Con_PlayRandomMusic(CStr _Music)
{
	MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
	
	if(pCon)
	{
		CStr RandMusic = _Music;

		int iCounter = 0;
		while(RandMusic != "")
		{
			iCounter++;
			RandMusic.GetStrSep(",");
		}

		if(iCounter > 0)
		{
			int iUseThis = int(Random*iCounter);

			for(int i = 1; i < iUseThis; i++)
				_Music.GetStrSep(",");

			int iFindPos = _Music.Find(",");
			if(iFindPos > -1)
				_Music = _Music.DelFrom(iFindPos);

			CStr cmd = CStrF("stream_play(\"%s\", \"\", 1)", _Music.Str());
				pCon->ExecuteString(cmd);
		}
		else
			ConOut("Missing string with random music\n");
	}	
}

void CWFrontEnd_Mod::Con_CG_CubeSeqForce(CStr _Seq)
{
	M_LOCK(m_FrontEndLock);
	if(_Seq == "inside" && m_Cube.m_pCurrentSequence != &m_Cube.m_Seq_inside)
		Con_CG_CubeSeq(_Seq);
	else if(_Seq == "startup" && m_Cube.m_pCurrentSequence != &m_Cube.m_Seq_startup)
		Con_CG_CubeSeq(_Seq);
	else if(_Seq == "normal" && m_Cube.m_pCurrentSequence != &m_Cube.m_Seq_normal)
		Con_CG_CubeSeq(_Seq);
	else if(_Seq == "moveout" && m_Cube.m_pCurrentSequence != &m_Cube.m_Seq_moveout)
		Con_CG_CubeSeq(_Seq);
	else if(_Seq == "fromgame" && m_Cube.m_pCurrentSequence != &m_Cube.m_Seq_fromgame)
		Con_CG_CubeSeq(_Seq);
	else if(_Seq == "backtogame" && m_Cube.m_pCurrentSequence != &m_Cube.m_Seq_backtogame)
		Con_CG_CubeSeq(_Seq);
}

void CWFrontEnd_Mod::Con_CG_CubeSide(int i)
{
	M_LOCK(m_FrontEndLock);
	m_QueuedSide = i;
}

void CWFrontEnd_Mod::Con_GrabScreen()
{
	M_LOCK(m_FrontEndLock);
	GrabScreen();
}

void CWFrontEnd_Mod::Con_Cheat(CStr _Cheat)
{
	M_LOCK(m_FrontEndLock);
	//#ifdef M_Profile
	ConExecuteImp(_Cheat, __FUNCTION__, 0);
	//#endif 
}

void CWFrontEnd_Mod::Con_CG_CubeResetWindow()
{
	M_LOCK(m_FrontEndLock);
	CStr CurrentWindow = m_aContextStack[m_iCurrentWindow];

	--m_iCurrentWindow;
	//	Con_CG_prevmenu();

	SubMenu(CurrentWindow);
	//	ConExecute(CStr("deferredscript(\"cg_submenu(\\\"%s\\\")\")", CurrentWindow.Str()));
}


void CWFrontEnd_Mod::Con_CacheCommand(CStr _Cmd)
{
	M_LOCK(m_FrontEndLock);
	m_CachedCommand = _Cmd;
}

void CWFrontEnd_Mod::Con_DoCacheCommand()
{
	M_LOCK(m_FrontEndLock);
	ConExecuteImp(m_CachedCommand, __FUNCTION__, 0);
}

void CWFrontEnd_Mod::Con_CG_Backout()
{
	M_LOCK(m_FrontEndLock);
	if(m_spDesktop == NULL)
		return;

	ConExecuteImp(m_BackoutCommand, __FUNCTION__, 0);
}

void CWFrontEnd_Mod::Con_CG_BlackLoading()
{
	M_LOCK(m_FrontEndLock);
	CGameContextMod *pGameMod = TDynamicCast<CGameContextMod>(m_pGameContext);

	if(pGameMod)
		pGameMod->m_BlackLoadingBG = true;
}

void CWFrontEnd_Mod::Con_CG_ReloadUIRegistry()
{
	if (!m_spMapData)
		return;

	int iRc = m_spMapData->GetResourceIndex_Registry("GUI\\CubeWnd");
	CWResource* pRC = m_spMapData->GetResource(iRc);
	if (pRC)
	{
		pRC->OnUnload();
		pRC->OnLoad();
	}
}

void CWFrontEnd_Mod::Register(CScriptRegisterContext & _RegContext)
{
	CWFrontEnd::Register(_RegContext);

	_RegContext.RegFunction("cg_backout", this, &CWFrontEnd_Mod::Con_CG_Backout);
	_RegContext.RegFunction("cg_blackloading", this, &CWFrontEnd_Mod::Con_CG_BlackLoading);
	_RegContext.RegFunction("docachedcommand", this, &CWFrontEnd_Mod::Con_DoCacheCommand);
	_RegContext.RegFunction("cachecommand", this, &CWFrontEnd_Mod::Con_CacheCommand);
	_RegContext.RegFunction("cg_grabscreen", this, &CWFrontEnd_Mod::Con_GrabScreen);

	_RegContext.RegFunction("cg_cuberesetwindow", this, &CWFrontEnd_Mod::Con_CG_CubeResetWindow);

	_RegContext.RegFunction("cg_cubeside", this, &CWFrontEnd_Mod::Con_CG_CubeSide);
	_RegContext.RegFunction("cg_cubeseq", this, &CWFrontEnd_Mod::Con_CG_CubeSeq);
	_RegContext.RegFunction("cg_cubeseqforce", this, &CWFrontEnd_Mod::Con_CG_CubeSeqForce);
	_RegContext.RegFunction("cheat", this, &CWFrontEnd_Mod::Con_Cheat);
	_RegContext.RegFunction("cg_reloaduiregistry", this, &CWFrontEnd_Mod::Con_CG_ReloadUIRegistry);

	_RegContext.RegFunction("cg_playrandommusic", this, &CWFrontEnd_Mod::Con_PlayRandomMusic);
	_RegContext.RegFunction("cg_selectsaveunit", this, &CWFrontEnd_Mod::Con_CG_SelectSaveUnit);

}


void CWFrontEnd_Mod::RenderHelpButtons(CMWnd *_pWnd, CRC_Util2D* _pRCUtil, CClipRect _Clip, CVec2Dfp32 *_pScale)
{
	// GUI_ArrowbackPC
	// GUI_ArrowforwardPC
	// GUI_ButtonPC

	CRC_Viewport* pVP = _pRCUtil->GetVBM()->Viewport_Get();
	CRct Rect = pVP->GetViewRect();
	CVec2Dfp32 BackupScale = CVec2Dfp32(Rect.GetWidth() / 640.0f, Rect.GetHeight() / 480.0f);

	if(!_pScale)
		_pScale = &BackupScale;
	
	CRC_Font *pFont = m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("HEADINGS"));
	if(!pFont)
		return;
	
	//int Style = WSTYLE_TEXT_CENTERY | WSTYLE_TEXT_WORDWRAP;
	int Style = WSTYLE_TEXT_WORDWRAP|WSTYLE_TEXT_SHADOW;

	int TextColorMOver = CPixel32(255,255,255, 255);
	int TextColorM = CPixel32(255,255,255, 255);
	int TextColorH = CPixel32(255,255,255, 255);;
	int TextColorD = 0xff000000;						

	//	_pRCUtil->GetRC()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	_pRCUtil->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

	if(_pWnd && _pWnd->m_lOutsideButtons.Len())
	{
		//CPnt ScreenSize = pRC->GetDC()->GetScreenSize();
		//CClipRect ClipRect(0,0,ScreenSize.x,ScreenSize.y);

		//if(pGameMod && (pGameMod->m_FriendsHandler.ShowInviteIcon() ||
		//(pGameMod->m_FriendsHandler.GotInvite() && m_spDesktop != NULL && m_CubeReferneces)))

		// render all buttons
		for(int32 i = 0; i < _pWnd->m_lOutsideButtons.Len(); i++)
		{
			// fetch button info
			COutsideButton &rButton = _pWnd->m_lOutsideButtons[i];

			m_CurAspectYScale = _pRCUtil->GetRC()->GetDC()->GetScreenAspect() * _pRCUtil->GetRC()->GetDC()->GetPixelAspect();
			CRct ButtonRect = GetButtonRect(_pWnd->m_lOutsideButtons[i], m_CurAspectYScale);

			// draw button
			bool bFocus = Cube_IsMouseOver(_pWnd, rButton);

			//CRct Rect = pVP->GetViewRect();
			CVec2Dfp32 old_scale = _pRCUtil->GetCoordinateScale();
			if(_pScale)
				_pRCUtil->SetCoordinateScale(*_pScale);
			
			if(bFocus)
				_pRCUtil->DrawTexture(_Clip,ButtonRect.p0, g_aButtonTextures[rButton.m_Type + 4], 0xFFffffFF,
				CVec2Dfp32(1.0f/g_ButtonScale,1.0f/(g_ButtonScale*m_CurAspectYScale)));
			else
				_pRCUtil->DrawTexture(_Clip,ButtonRect.p0, g_aButtonTextures[rButton.m_Type], 0xFFffffFF,
				CVec2Dfp32(1.0f/g_ButtonScale,1.0f/(g_ButtonScale*m_CurAspectYScale)));

			_pRCUtil->SetCoordinateScale(old_scale);

			// check for caption
			if(_pWnd->m_lOutsideButtons[i].m_Caption != "")
			{
				// render caption
				CStr Text = Localize_Str(_pWnd->m_lOutsideButtons[i].m_Caption);				

				// get text size
				const fp32 FontSize = 16.0f;
				fp32 w = pFont->GetWidth(FontSize, Text);
				//				fp32 h = pFont->GetHeight(FontSize, Text);
				Text = CStrF("§Z%.2i", int(FontSize)) + Text;
				rButton.m_Extend = w; // extend the possible clicking area

				CPnt Pos = ButtonRect.p0; // calculate position of the text
				Pos.y += int(g_ButtonYAdjust*m_CurAspectYScale);
				//Pos.y += ((ButtonRect.p1.y-ButtonRect.p0.y)-FontSize)/2;				
				if(rButton.m_Pos == 1)
				{
					if(_pScale)
					{
						//Pos.x = int(Pos.x * (_pScale->k[0] / old_scale.k[0]));
						//Pos.x -= int(w) * (_pScale->k[0] / old_scale.k[0]));
						Pos.x = int((ButtonRect.p0.x -6.0f) * (_pScale->k[0] / old_scale.k[0]));
						Pos.x -= int(w*m_CurAspectYScale);// * (_pScale->k[0] / old_scale.k[0]));
					}
					else
						Pos.x -= int(w);
				}
				else
				{
					if(_pScale)
						//Pos.x = int((ButtonRect.p1.x - (ButtonRect.p0.x*g_ButtonScale) + 10) * (_pScale->k[0] / old_scale.k[0]));					
						Pos.x = int((ButtonRect.p0.x + (ButtonRect.p1.x-ButtonRect.p0.x)*g_ButtonScale) * (_pScale->k[0] / old_scale.k[0]));					
					else
						Pos.x = ButtonRect.p1.x;
				}

				_pRCUtil->SetFontScale(m_CurAspectYScale, m_CurAspectYScale);
				if(bFocus)
				{
					CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pFont,Text, Pos.x, Pos.y,
						Style, TextColorMOver, TextColorH, TextColorD, 280, 50);
				}
				else
				{
					CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pFont,Text, Pos.x, Pos.y,
						Style, TextColorM, TextColorH, TextColorD, 280, 50);
				}
			}

		}

		//_pRCUtil->Line(ClipRect, m_MousePosition, m_MousePosition+CPnt(10,0), 0xFFFFFFFF);
		//_pRCUtil->Line(ClipRect, m_MousePosition, m_MousePosition+CPnt(0,10), 0xFFFFFFFF);
		//_pRCUtil->Line(ClipRect, m_MousePosition, m_MousePosition+CPnt(20,20), 0xFFFFFFFF);
	}


	int iRc = m_spMapData->GetResourceIndex_Registry("GUI\\CubeWnd");
	CRegistry* pRc = m_spMapData->GetResource_Registry(iRc);

	const CRegistry *pMainReg = NULL;
	//	const CRegistry *pTempReg = NULL;

	if(pRc)
		pMainReg = pRc->Find("BUTTON_DESCRIPTOR");

	if(!pMainReg)
		return;

	CStr IconSurface_Y = pMainReg->GetValue("ICONSURFACE_Y");
	CStr IconSurface_X = pMainReg->GetValue("ICONSURFACE_X");
	CStr IconSurface_A = pMainReg->GetValue("ICONSURFACE_A");
	CStr IconSurface_B = pMainReg->GetValue("ICONSURFACE_B");
	CStr IconSurface_Left = pMainReg->GetValue("ICONSURFACE_LEFT");
	CStr IconSurface_Right = pMainReg->GetValue("ICONSURFACE_RIGHT");

	CPnt aTextPos[4];
	CPnt aIconPos[4];
	aTextPos[0] = GetPointFromReg(pMainReg, "TEXTPOSITION_1");
	aTextPos[1] = GetPointFromReg(pMainReg, "TEXTPOSITION_2");
	aTextPos[2] = GetPointFromReg(pMainReg, "TEXTPOSITION_3");
	aTextPos[3] = GetPointFromReg(pMainReg, "TEXTPOSITION_4");

	aIconPos[0] = GetPointFromReg(pMainReg, "ICONPOSITION_1");
	aIconPos[1] = GetPointFromReg(pMainReg, "ICONPOSITION_2");
	aIconPos[2] = GetPointFromReg(pMainReg, "ICONPOSITION_3");
	aIconPos[3] = GetPointFromReg(pMainReg, "ICONPOSITION_4");




	/*
	_pRCUtil->SetTexture(_pRCUtil->GetRC()->Texture_GetTC()->GetTextureID(IconSurface_A));
	_pRCUtil->SetTextureScale(1.0f, 1.0f);
	_pRCUtil->SetTextureOrigo(_Clip, CPnt(16,16));
	_pRCUtil->Rect(_Clip, CRct(16,16,32+16,32+16), 0xff7f7f7f);
	*/


	//_pRCUtil->GetRC()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

	//m_AButtonDescriptor
	for(int32 i = 0; i < 4; i++)
	{
		if(_pWnd->m_aButtonDescriptions[i].Len())
		{
			// process
			CStr Text = _pWnd->m_aButtonDescriptions[i];
			int32 iPos = i; //Text.GetStrSep(",").Val_int();
			CStr Button = Text.GetStrSep(",");
			wchar aText[1024];
			Localize_Str(Text, aText, 1023);
			Text = aText;

			if(iPos >= 0 && iPos < 4)
			{
				fp32 w = pFont->GetWidth(pFont->GetOriginalSize(), Text);
				fp32 h = pFont->GetHeight(pFont->GetOriginalSize(), Text);
				if(iPos%2 == 1)
					w = 0;

				if(Button[0] == 'x')
					_pRCUtil->SetTexture(_pRCUtil->GetTC()->GetTextureID(IconSurface_X));
				else if(Button[0] == 'y')
					_pRCUtil->SetTexture(_pRCUtil->GetTC()->GetTextureID(IconSurface_Y));
				else if(Button[0] == 'a')
					_pRCUtil->SetTexture(_pRCUtil->GetTC()->GetTextureID(IconSurface_A));
				else if(Button[0] == 'b')
					_pRCUtil->SetTexture(_pRCUtil->GetTC()->GetTextureID(IconSurface_B));
				else if(Button[0] == '<')
					_pRCUtil->SetTexture(_pRCUtil->GetTC()->GetTextureID(IconSurface_Left));
				else if(Button[0] == '>')
					_pRCUtil->SetTexture(_pRCUtil->GetTC()->GetTextureID(IconSurface_Right));

				_pRCUtil->SetTextureScale(1.0f, 1.0f);
				_pRCUtil->SetTextureOrigo(_Clip, CPnt(aIconPos[iPos].x,aIconPos[iPos].y));
				//_pRCUtil->SetTextureOrigo(_Clip, CPnt(AIconPos.x, AIconPos.y+pFocusWnd->m_iButtonDescriptorVCorrection));
				_pRCUtil->Rect(_Clip, CRct(aIconPos[iPos].x,aIconPos[iPos].y,aIconPos[iPos].x+32,aIconPos[iPos].y+32), 0xff7f7f7f);

				/*
				wchar LocText[1024];
				Localize_Str(Text, LocText, 1023);*/
				//CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pFont, Text, 30, 400, Style, TextColorM, TextColorH, TextColorD, _Clip.GetWidth(), _Clip.GetHeight());
				CMWnd_Text_DrawFormated(_pRCUtil, _Clip, pFont,Text, 
					int(aTextPos[iPos].x-w), int(aTextPos[iPos].y-h-3),
					Style, TextColorM, TextColorH, TextColorD, 280, 50);



			}
		}
	}
}

//
//
//
void CWFrontEnd_Mod::OnPostRenderInterface(CRC_Util2D* _pRCUtil, CClipRect _Clip, CVec2Dfp32 *_pScale)
{
	MAUTOSTRIP(CWFrontEnd_Mod_OnPostRenderInterface, MAUTOSTRIP_VOID);
	if(m_spDesktop != NULL)
		RenderHelpButtons(m_spDesktop, _pRCUtil, _Clip, _pScale);
}
