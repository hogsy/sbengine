#include "PCH.h"
#include "WFrontEnd_P6.h"
#include "../Exe/WGameContextMain.h"

MRTC_IMPLEMENT_DYNAMIC(CWFrontEnd_P6, CWFrontEnd_Mod);

CWFrontEnd_P6::CWFrontEnd_P6()
{
	m_CubeWndName = "P6_CubeWnd";
	m_BG = "P6_inv_w_BG02";
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

void CWFrontEnd_P6::OnPrecache(CXR_Engine* _pEngine)
{
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	MACRO_GetRegisterObject(CWaveContext, pWC, "SYSTEM.WAVECONTEXT");
	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");

	PrecacheFont(m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("TEXT")), pTC);
	PrecacheFont(m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("HEADINGS")), pTC);
	PrecacheFont(m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("MONOPRO")), pTC);
	PrecacheFont(m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("P6_text")), pTC);

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

	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_ak47"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_armour"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_barrel"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_BG01"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_w_BG02"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_box01"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_boxdark"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_boxlight"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_dogtag"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_grid01"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_head"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_indicat01"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_indicat02"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_info01"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_info02"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_legs"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_markers"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_meny"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_ruta03"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_ruta01"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_loot"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_journal"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_w_guns02"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_trade01"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_trade02"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_red"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_kolv"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_stock"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_barrel02"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_hand01"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_hand02"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_torso"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_mag"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_w_BG"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_w_BG02_w"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_w_slot01"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_w_slot02"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_w_slot04"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_w_slot05"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_w_slot06"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_w_slot07"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_vest01"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_vest02"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_vest03"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_BG01_w"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_BG02_w"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_BG_w"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_m4"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_inv_m4mod"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_use_YB"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_use_A"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_use_X01"));
	PrecacheTexture(pTC, pTC->GetTextureID("P6_use_X02"));

	m_Cube.OnPrecache(_pEngine, m_pGameContext->m_spWServer);
}

void CWFrontEnd_P6::Cube_Render(CRenderContext* _pRC, CXR_VBManager* _pVBM, bool _IsJournal)
{
	// transfer surface
	int32 iSurface = m_spMapData->GetResourceIndex_Surface(m_Cube.m_SurfaceName);
	m_Cube.m_pSurface = m_spMapData->GetResource_Surface(iSurface);		// Resource-mapping for client-game. 

	//
	bool InGame = m_pGameContext->m_lspWClients.Len() > 0;

	_pVBM->ScopeBegin("CWFrontEnd_P6::Cube_Render", false);
	//m_Cube.Render_P6(_pRC, _pVBM, m_Cube.m_FrameTime*0.75f, m_Cube.GetGUIProcessContainer(), (m_pGameContext->m_lspWClients.Len() > 0), GetRender_Engine(),m_GameIsLoading, false, m_BG, _IsJournal);
	
	CRC_Util2D util;
	CRC_Viewport* pViewport = _pVBM->Viewport_Get();
	util.Begin(_pRC, pViewport, _pVBM);
	util.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
	const CRct& ViewRect = pViewport->GetViewRect();
	CClipRect Clip(0, 0, ViewRect.GetWidth(), ViewRect.GetHeight());

	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

	fp32 Amount = 0.0f;
	int32 size = pGameMod->m_spWData->XDF_GetSize();
	if (size > 0)
	{
		int32 pos = pGameMod->m_spWData->XDF_GetPosition();
		Amount = pos/(fp32)size;
	}
	else
	{
		fp32 Status = pGameMod->m_spWServer ? pGameMod->m_spWServer->Load_GetProgress() : 0.0f;
		fp32 StatusClient = 0.0f;
		if ((pGameMod->m_lspWClients.Len() > 0) && (pGameMod->m_lspWClients[0] != NULL))
			StatusClient = pGameMod->m_lspWClients[0]->Load_GetProgress();
		Amount = (Status + StatusClient) * 0.5f;
	}

	bool bLoading = false;
	CWorld_Client *pClient = pGameMod->GetCurrentClient();
	if((pClient && (pClient->GetClientState() != WCLIENT_STATE_INGAME || pClient->Precache_Status())))
		bLoading = true;

	if(bLoading)
	{
		int bx = 150;
		int ym = 360, hy = 10;
		int lw = 2;
		int x0 = bx, x1 = 640 - bx, w = 640 - 2*bx - lw*4;
		int y0 = ym - hy, y1 = ym + hy;

		util.SetCoordinateScale(CVec2Dfp32(ViewRect.GetWidth() *(1.0f/640.0f) , ViewRect.GetHeight() * (1.0f/480.0f)));

		util.Rect(Clip, CRct(x0, y0, x1, y1), 0x5f000000);
		util.Rect(Clip, CRct(x0+lw, y0, x1-lw, y0+lw), 0xafffffff); // upper
		util.Rect(Clip, CRct(x0, y0, x0+lw, y1), 0xafffffff); // left
		util.Rect(Clip, CRct(x1-lw, y0, x1, y1), 0xafffffff); // right
		util.Rect(Clip, CRct(x0+lw, y1-lw, x1-lw, y1), 0xafffffff); // lower
		util.Rect(Clip, CRct(x0+lw*2, y0+lw*2, (int)(x0+lw*2 + w*Amount), y1-lw*2), 0xafffffff);
	}

	util.End();
	_pVBM->ScopeEnd();
}

void CWFrontEnd_P6::OnRender(CRenderContext* _pRC, CXR_VBManager* _pVBM)
{
	MSCOPESHORT(CWFrontEnd_P6::OnRender);

	m_Cube.m_GUIFlags = m_Cube.m_GUIFlags | CCube::GUI_FLAGS_EXITCOMPLETE;

	if(!m_bDonePrecache)
		DoPrecache(_pRC, m_pEngine);

	CWFrontEnd::OnRender(_pRC, _pVBM);

	// render reference points
	if(((!m_OutOfGUI || m_ClientMouse) && !m_GameIsLoading) || (m_spDesktop && m_spDesktop->m_bShowMouse))

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

		Util.End();
	}

	if (m_bShowConfidential)
	{
		CRC_Font *pFont = m_spMapData->GetResource_Font(m_spMapData->GetResourceIndex_Font("P6_text"));
		if (pFont)
		{
			_pVBM->ScopeBegin("Confidential Text", false);
			CRC_Util2D Util;
			CRC_Viewport* pViewport = _pVBM->Viewport_Get();

			Util.Begin(_pRC, pViewport, _pVBM);
			Util.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			Util.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE);

			CClipRect ClipRect(pViewport->GetViewRect());

			const char* pMessage = "CONFIDENTIAL";
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

