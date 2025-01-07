#include "PCH.h"
#include "WFrontEnd_Menues_P6.h"
#include "../Exe/WGameContextMain.h"
#include "../GameClasses/WObj_Game/WObj_GameCore.h"
#include "../GameClasses/WObj_Char/WObj_CharPlayer_P6.h"
#include "../GameClasses/WObj_Char/WObj_CharNPC_P6.h"
#include "WFrontEnd_P6.h"
#include "WClient_P6.h"

CImageButton::CImageButton()
{
	x = 0;
	y = 0;

	bDisabled = false;
	bMarked = false;
}

void CImageButton::Init(CStr _Name, int _x, int _y)
{
	name = _Name;
	x = _x;
	y = _y;
}

void CImageButton::Render(CRC_Util2D* _pRCUtil, const CClipRect &_Clip)
{
	CXR_VBManager* pVBM = _pRCUtil->GetVBM();
	CRC_Viewport* pViewport = pVBM->Viewport_Get();

	if(!bDisabled)
	{
		CPnt pos;
		pos.x = x;
		pos.y = y;
		CVec2Dfp32 scale(5.0f, 5.0f);

		CPixel32 pix = 0xffffffff;
		if(bMarked)
			pix = 0xffa0a0a0;
		_pRCUtil->DrawTexture(_Clip, pos, name.Str(), pix, scale);
	}
}

CMWnd_CubeMenu_Main_P6::CMWnd_CubeMenu_Main_P6()
{
	m_BG = "P6_inv_w_BG02";
}

void CMWnd_CubeMenu_Main_P6::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if(_Key == "BACKGROUND")
	{
		m_BG = _Value;
	}
	else
		CMWnd_CubeMenu::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_CubeMenu_Main_P6::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	((CWFrontEnd_P6*)m_CubeUser.m_pFrontEndMod)->m_BG = m_BG.Str();
	CXR_VBManager *pVBM = _pRCUtil->GetVBM();
	CRC_Viewport *pVP = pVBM->Viewport_Get();
	CRct rct = pVP->GetViewRect();
	_pRCUtil->Rect(rct, rct, 0xff000000);
	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Main_P6, CMWnd_CubeMenu);

CMWnd_CubeMenu_Journal_P6::CMWnd_CubeMenu_Journal_P6()
{
	m_BG = "P6_inv_w_BG02";
}

void CMWnd_CubeMenu_Journal_P6::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if(_Key == "BACKGROUND")
	{
		m_BG = _Value;
	}
	else
		CMWnd_CubeMenu::EvaluateKey(_pParam, _Key, _Value);
}

void CMWnd_CubeMenu_Journal_P6::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	((CWFrontEnd_P6*)m_CubeUser.m_pFrontEndMod)->m_BG = m_BG.Str();
	CXR_VBManager *pVBM = _pRCUtil->GetVBM();
	CRC_Viewport *pVP = pVBM->Viewport_Get();
	CRct rct = pVP->GetViewRect();
	_pRCUtil->Rect(rct, rct, 0xff000000);
	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);
	_pRCUtil->DrawTexture(_Clip, CPnt(0, 0), "P6_inv_journal", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Journal_P6, CMWnd_CubeMenu);

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Inventory, CMWnd_CubeMenu);

CMWnd_CubeMenu_Inventory::CMWnd_CubeMenu_Inventory()
{
	m_XPos = 1;
	m_YPos = 0;
	m_LastMarkedX = -1;
	m_LastMarkedY = -1;
}

void CMWnd_CubeMenu_Inventory::OnCreate()
{
	for(int x = 1; x < INVENTORY_WIDTH; x++)
	{
		for(int y = 0; y < INVENTORY_HEIGHT; y++)	
		{
			m_lButtons[x][y].Init("P6_inv_boxdark", 405 + (x - 1) * 77, int(52 + y * 32.3f));
		}
	}

	m_lButtons[0][0].Init("P6_inv_boxlight", 310, 10);
	m_lButtons[0][1].Init("P6_inv_boxlight", 310, 47);
	m_lButtons[0][2].Init("P6_inv_boxlight", 310, 100);
	m_lButtons[0][3].Init("P6_inv_boxlight", 310, 137);
	m_lButtons[0][4].Init("P6_inv_boxlight", 310, 190);
	m_lButtons[0][5].Init("P6_inv_boxlight", 310, 227);
	m_lButtons[0][6].Init("P6_inv_boxlight", 310, 300);
	m_lButtons[0][7].Init("P6_inv_boxlight", 310, 380);

	m_Mode = MENU_INV;

	ConExecute("pause(0)");
}

int CMWnd_CubeMenu_Inventory::ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
{
	MACRO_GetRegisterObject(CGameContext, pGameContext, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGameContext);
	CWO_CharPlayer_ClientData_P6* pCD = NULL;
	if(pGameContext->m_spWServer)
	{
		CWObject_Game *pGame = pGameMod ? pGameContext->m_spWServer->Game_GetObject() : NULL;
		CWO_Player *pPlayer = pGame ? pGame->Player_Get(0) : NULL; 
		CWObject *pCharObj = pPlayer ? pGameContext->m_spWServer->Object_Get(pPlayer->m_iObject) : NULL;
		CWObject_CharPlayer_P6 *pChar  = pCharObj ? safe_cast<CWObject_CharPlayer_P6>(pCharObj) : NULL;
		pCD = pChar ? safe_cast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pChar->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
	}
	else
	{
		CWorld_Client* pClientOrg = pGameContext->GetCurrentClient();
		CWClient_P6 *pClient = safe_cast<CWClient_P6>(pClientOrg);
		CWObject_CoreData *pCoreData = pClient->Object_GetFirstCopy(pClient->Player_GetLocalObject());
		pCD = safe_cast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pCoreData->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	}

	int32 k = _Key.GetKey9();

	int lx = m_XPos;
	int ly = m_YPos;

	if(_Key.IsDown())
	{
		if(k == SKEY_JOY_BUTTON09 || k == SKEY_JOY_BUTTON0B)
		{	//Right
			m_Mode++;
			if(m_Mode == MENU_LAST)
				m_Mode = 0;
		}
		else if(k == SKEY_JOY_BUTTON08 || k == SKEY_JOY_BUTTON0A)
		{	//Left
			m_Mode--;
			if(m_Mode == -1)
				m_Mode = MENU_LAST - 1;
		}
		if(m_Mode == MENU_INV)
		{
			if(k == SKEY_GUI_LEFT)
			{
				m_XPos--;
				m_XPos = Max(m_XPos, 0);
			}
			else if(k == SKEY_GUI_RIGHT)
			{
				m_XPos++;
				m_XPos = Min(m_XPos, INVENTORY_WIDTH - 1);
			}
			else if(k == SKEY_GUI_DOWN)
			{
				m_YPos++;
				m_YPos = Min(m_YPos, INVENTORY_HEIGHT - 1);
			}
			else if(k == SKEY_GUI_UP)
			{
				m_YPos--;
				m_YPos = Max(m_YPos, 0);
			}
			else if(k == SKEY_GUI_BUTTON0)
			{
				bool bMarkIt = true;
				if(m_LastMarkedX >= 0 && m_LastMarkedY >= 0)
				{
					if(m_LastMarkedX == m_XPos && m_LastMarkedY == m_YPos)
						bMarkIt = false;
					else
					{	
						if((m_XPos < 1 || m_LastMarkedX < 1) && (m_XPos >= 1 || m_LastMarkedX >= 1))
						{	//Equip/unequip
							bMarkIt = false;
							int ix = -1;
							int iy = -1;
							int ex = -1;
							int ey = -1;
							if(m_XPos < 1)
							{
								ix = m_LastMarkedX;
								iy = m_LastMarkedY;
								ex = m_XPos;
								ey = m_YPos;
							}
							else
							{
								ix = m_XPos;
								iy = m_YPos;
								ex = m_LastMarkedX;
								ey = m_LastMarkedY;
							}
							pCD->InventoryEquip(ex, ey, ix, iy);
						}
						else if(m_XPos >= 1 || m_LastMarkedX >= 1)
						{	//Switch places in inventory
							bMarkIt = false;
							pCD->InventorySwitch(m_XPos, m_YPos, m_LastMarkedX, m_LastMarkedY);
						}
					}

					m_lButtons[m_LastMarkedX][m_LastMarkedY].bMarked = false;
				}
				if(bMarkIt)
				{
					bool bOther = true;
					if(m_XPos > 0)
					{
						int iSelected = -1;
						for(int i = 0; i < pCD->m_lInventory.Len(); i++)
						{
							if(m_XPos == pCD->m_lInventory[i].x && m_YPos == pCD->m_lInventory[i].y && pCD->m_lInventory[i].item.m_LootType >= 0)
							{
								iSelected = i;
								break;
							}
						}
						if(iSelected != -1)
						{
							int c = pCD->m_lInventory[iSelected].item.m_LootType;
							switch(pCD->m_lInventory[iSelected].item.m_LootType)
							{
							case INV_ARMOR:
								{
									pCD->InventoryEquip(0, 2, m_XPos, m_YPos);
									bOther = false;
								}
								break;
							case INV_WEAPON:
								{
									pCD->InventoryEquip(0, 4, m_XPos, m_YPos);
									bOther = false;
								}
								break;
							}
						}
					}
					if(bOther)
					{
						m_lButtons[m_XPos][m_YPos].bMarked = true;
						m_LastMarkedX = m_XPos;
						m_LastMarkedY = m_YPos;
					}
				}
				else
				{
					m_LastMarkedX = -1;
					m_LastMarkedY = -1;
				}
			}
			else if(k == SKEY_JOY_BUTTON02)
			{
				if(m_XPos > 0)
				{
					int iSelected = -1;
					for(int i = 0; i < pCD->m_lInventory.Len(); i++)
					{
						SInventoryItem c = pCD->m_lInventory[i];
						if(m_XPos == pCD->m_lInventory[i].x && m_YPos == pCD->m_lInventory[i].y && pCD->m_lInventory[i].item.m_LootType >= 0)
						{
							iSelected = i;
							break;
						}
					}
					if(iSelected != -1)
					{
						if(pCD->m_lInventory[iSelected].item.m_LootType == INV_WEAPON)
						{
							pCD->m_iWeaponIndex = iSelected;
							ConExecute("cg_submenu(\"p6_weapon_mod\")");
						}
					}
				}
			}
			else
				return CMWnd_CubeMenu::ProcessKey(_Key, _OriginalKey);
		}
		else
			return CMWnd_CubeMenu::ProcessKey(_Key, _OriginalKey);
	}
	else
		return CMWnd_CubeMenu::ProcessKey(_Key, _OriginalKey);

	if(m_lButtons[m_XPos][m_YPos].bDisabled)
	{
		m_XPos = lx;
		m_YPos = ly;
	}

	return 1;
}

void CMWnd_CubeMenu_Inventory::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	((CWFrontEnd_P6*)m_CubeUser.m_pFrontEndMod)->m_BG = "P6_inv_BG01";
	CXR_VBManager* pVBM = _pRCUtil->GetVBM();
	CRC_Viewport *pVP = pVBM->Viewport_Get();
	CRct rct = pVP->GetViewRect();
	_pRCUtil->Rect(rct, rct, 0xff000000);

	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);

	MACRO_GetRegisterObject(CGameContext, pGameContext, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGameContext);
	CWO_CharPlayer_ClientData_P6* pCD = NULL;
	if(pGameContext->m_spWServer)
	{
		CWObject_Game *pGame = pGameMod ? pGameContext->m_spWServer->Game_GetObject() : NULL;
		CWO_Player *pPlayer = pGame ? pGame->Player_Get(0) : NULL; 
		CWObject *pCharObj = pPlayer ? pGameContext->m_spWServer->Object_Get(pPlayer->m_iObject) : NULL;
		CWObject_CharPlayer_P6 *pChar  = pCharObj ? safe_cast<CWObject_CharPlayer_P6>(pCharObj) : NULL;
		pCD = pChar ? safe_cast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pChar->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
	}
	else
	{
		CWorld_Client* pClientOrg = pGameContext->GetCurrentClient();
		CWClient_P6 *pClient = safe_cast<CWClient_P6>(pClientOrg);
		CWObject_CoreData *pCoreData = pClient->Object_GetFirstCopy(pClient->Player_GetLocalObject());
		pCD = safe_cast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pCoreData->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	}
	
	if(!pCD)
		return;

	pVBM->ScopeBegin("CMWnd_CubeMenu_Inventory::OnPaint", false);

	if(m_Mode == MENU_INV)
	{
		CRC_Font *pFont = GetFont("TEXT");
		CPixel32 col = 0xffffffff;
		_pRCUtil->DrawTexture(_Clip, CPnt(400, 5), "P6_inv_markers", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
		_pRCUtil->Text(_Clip, pFont, 545, 17, CStrF("§Z10Markers  %i", pCD->m_Money.m_Value), col);
		
		_pRCUtil->DrawTexture(_Clip, CPnt(10, 445), "P6_inv_meny", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));

		_pRCUtil->DrawTexture(_Clip, CPnt(220, 5), "P6_inv_ruta03", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
		_pRCUtil->DrawTexture(_Clip, CPnt(220, 95), "P6_inv_ruta03", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
		_pRCUtil->DrawTexture(_Clip, CPnt(220, 185), "P6_inv_ruta03", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));

		_pRCUtil->DrawTexture(_Clip, CPnt(220, 290), "P6_inv_ruta01", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
		_pRCUtil->DrawTexture(_Clip, CPnt(220, 370), "P6_inv_ruta01", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));

		//_pRCUtil->DrawTexture(_Clip, CPnt(200, 100), "P6_inv_ruta02", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));

		//Render empty buttons
		for(int y = 0; y < INVENTORY_HEIGHT; y++)
		{
			for(int x = 0; x < INVENTORY_WIDTH; x++)
			{
				m_lButtons[x][y].Render(_pRCUtil, _Clip);
			}
		}

		//The inventory grid
		_pRCUtil->DrawTexture(_Clip, CPnt(400, 35), "P6_inv_grid01", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
		_pRCUtil->Text(_Clip, pFont, 410, 40, "§Z10Inventory", col);

		bool bModify = false;

		//Info1
		_pRCUtil->DrawTexture(_Clip, CPnt(405, 320), "P6_inv_info01", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
		for(int i = 0; i < pCD->m_lInventory.Len(); i++)
		{
			if(pCD->m_lInventory[i].x == m_XPos && pCD->m_lInventory[i].y == m_YPos)
			{
				if(pCD->m_lInventory[i].item.m_LootType == INV_WEAPON)
					bModify = true;
				_pRCUtil->Text(_Client, pFont, 520, 325, CStrF("§Z10%s", pCD->m_lInventory[i].item.m_LootObject.Str()), col);

				wchar Text[1024];
				Localize_Str(CStrF("§Z10%s", pCD->m_lInventory[i].item.m_Info.Str()), Text, 1023);

				int Len = pCD->m_lInventory[i].item.m_Info.Len() + 4;

				wchar Lines[16][512];
				const int MaxLines = 15;
				wchar* lpLines[16];
				for(int ii = 0; ii < 16; ii++) 
					lpLines[ii] = &Lines[ii][0];

				int nLines = _pRCUtil->Text_WordWrap(pFont, 120, (wchar*) Text, Len, &lpLines[0], 15);

				for(int ln = 0; ln < nLines; ln++)
				{
					int th = _pRCUtil->TextHeight(pFont, &Lines[ln][0]);
					_pRCUtil->Text(_Client, pFont, 520, 335 + ln * th, &Lines[ln][0], col);
				}
				
				break;
			}
		}

		//Info2
		_pRCUtil->DrawTexture(_Clip, CPnt(550, 320), "P6_inv_info02", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));

		_pRCUtil->DrawTexture(_Clip, CPnt(550, 315), "P6_use_YB", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
		_pRCUtil->DrawTexture(_Clip, CPnt(550, 315), "P6_use_A", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
		if(bModify)
			_pRCUtil->DrawTexture(_Clip, CPnt(550, 315), "P6_use_X01", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
		else
			_pRCUtil->DrawTexture(_Clip, CPnt(550, 315), "P6_use_X02", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));

		//now lets render the actual inventory
		for(int i = 0; i < pCD->m_lInventory.Len(); i++)
		{
			CPnt pos;
			pos.x = m_lButtons[pCD->m_lInventory[i].x][pCD->m_lInventory[i].y].x;
			pos.y = m_lButtons[pCD->m_lInventory[i].x][pCD->m_lInventory[i].y].y;
				
			_pRCUtil->DrawTexture(_Clip, pos, pCD->m_lInventory[i].item.m_GUI_Icon, CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
		}

		CPnt pos;
		pos.x = m_lButtons[m_XPos][m_YPos].x;
		pos.y = m_lButtons[m_XPos][m_YPos].y;
		_pRCUtil->DrawTexture(_Clip, pos, "P6_inv_indicat02", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));

		if(m_XPos == 0)
		{
			if(m_YPos == 0 || m_YPos == 1)
				_pRCUtil->DrawTexture(_Clip, CPnt(172, 20), "P6_inv_head", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
			else if(m_YPos == 2 || m_YPos == 3)
				_pRCUtil->DrawTexture(_Clip, CPnt(100, 68), "P6_inv_torso", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
			else if(m_YPos == 6)
				_pRCUtil->DrawTexture(_Clip, CPnt(142, 225), "P6_inv_legs", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
			
		}
	}
	else if(m_Mode == MENU_JOURNAL)
		_pRCUtil->DrawTexture(_Clip, CPnt(0, 0), "P6_inv_journal", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
	else if(m_Mode == MENU_VEHICLE)
	{
		if(1.4f < ((fp32)rct.p1.x / (fp32)rct.p1.y))
		{
			CClipRect Clip;
			Clip.clip = rct;
			_pRCUtil->DrawTexture(Clip, CPnt(0, 0), "p6_gui_vehicle_w", CPixel32(0xffffffff), CVec2Dfp32(2.5f, 2.5f));
		}
		else
			_pRCUtil->DrawTexture(_Clip, CPnt(0, 0), "p6_gui_vehicle", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
	}
	
	pVBM->ScopeEnd();
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_WeaponMod, CMWnd_CubeMenu);

CMWnd_CubeMenu_WeaponMod::CMWnd_CubeMenu_WeaponMod()
{
	m_XPos = 3;
	m_YPos = 0;
	m_LastMarkedX = -1;
	m_LastMarkedY = -1;
}

void CMWnd_CubeMenu_WeaponMod::OnCreate()
{
	for(int x = 3; x < WEAPONMOD_WIDTH; x++)
	{
		for(int y = 0; y < WEAPONMOD_HEIGHT; y++)	
		{
			m_lButtons[x][y].Init("P6_inv_boxdark", 405 + (x - 3) * 77, int(52 + y * 32.3f));
		}
	}

	for(int x = 0; x < 3; x++)
	{
		for(int y = 0; y < WEAPONMOD_HEIGHT; y++)
		{
			m_lButtons[x][y].bDisabled = true;
		}
	}

	m_lButtons[0][0].Init("P6_inv_boxdark", 50, 70);
	m_lButtons[1][0].Init("P6_inv_boxdark", 150, 70);
	m_lButtons[2][0].Init("P6_inv_boxdark", 250, 70);
	m_lButtons[0][1].Init("P6_inv_boxdark", 50, 250);
	m_lButtons[1][1].Init("P6_inv_boxdark", 150, 250);
	m_lButtons[2][1].Init("P6_inv_boxdark", 250, 250);
	m_lButtons[0][0].bDisabled = false;
	m_lButtons[1][0].bDisabled = false;
	m_lButtons[2][0].bDisabled = false;
	m_lButtons[0][1].bDisabled = false;
	m_lButtons[1][1].bDisabled = false;
	m_lButtons[2][1].bDisabled = false;
}

int CMWnd_CubeMenu_WeaponMod::ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
{
	MACRO_GetRegisterObject(CGameContext, pGameContext, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGameContext);
	CWO_CharPlayer_ClientData_P6* pCD = NULL;
	if(pGameContext->m_spWServer)
	{
		CWObject_Game *pGame = pGameMod ? pGameContext->m_spWServer->Game_GetObject() : NULL;
		CWO_Player *pPlayer = pGame ? pGame->Player_Get(0) : NULL; 
		CWObject *pCharObj = pPlayer ? pGameContext->m_spWServer->Object_Get(pPlayer->m_iObject) : NULL;
		CWObject_CharPlayer_P6 *pChar  = pCharObj ? safe_cast<CWObject_CharPlayer_P6>(pCharObj) : NULL;
		pCD = pChar ? safe_cast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pChar->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
	}
	else
	{
		CWorld_Client* pClientOrg = pGameContext->GetCurrentClient();
		CWClient_P6 *pClient = safe_cast<CWClient_P6>(pClientOrg);
		CWObject_CoreData *pCoreData = pClient->Object_GetFirstCopy(pClient->Player_GetLocalObject());
		pCD = safe_cast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pCoreData->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	}

	int32 k = _Key.GetKey9();

	int lx = m_XPos;
	int ly = m_YPos;

	if(_Key.IsDown())
	{
		if(k == SKEY_GUI_LEFT)
		{
			m_XPos--;
			m_XPos = Max(m_XPos, 0);
		}
		else if(k == SKEY_GUI_RIGHT)
		{
			m_XPos++;
			m_XPos = Min(m_XPos, WEAPONMOD_WIDTH - 1);
		}
		else if(k == SKEY_GUI_DOWN)
		{
			m_YPos++;
			m_YPos = Min(m_YPos, WEAPONMOD_HEIGHT - 1);
		}
		else if(k == SKEY_GUI_UP)
		{
			m_YPos--;
			m_YPos = Max(m_YPos, 0);
		}
		else if(k == SKEY_GUI_BUTTON0)
		{
			bool bMarkIt = true;
			if(m_LastMarkedX >= 0 && m_LastMarkedY >= 0)
			{
				if(m_LastMarkedX == m_XPos && m_LastMarkedY == m_YPos)
					bMarkIt = false;
				else
				{	
					if((m_XPos < 3 || m_LastMarkedX < 3) && (m_XPos >= 3 || m_LastMarkedX >= 3))
					{	//Equip/unequip
						bMarkIt = false;
						int ix = -1;
						int iy = -1;
						int ex = -1;
						int ey = -1;
						if(m_XPos < 3)
						{
							ix = m_LastMarkedX;
							iy = m_LastMarkedY;
							ex = m_XPos;
							ey = m_YPos;
						}
						else
						{
							ix = m_XPos;
							iy = m_YPos;
							ex = m_LastMarkedX;
							ey = m_LastMarkedY;
						}
						for(int i = 0; i < pCD->m_lInventory.Len(); i++)
						{
							if((pCD->m_lInventory[i].x + 2) == ix &&  pCD->m_lInventory[i].y == iy)
							{
								if(pCD->m_lInventory[i].item.m_LootType == INV_WEAPON_MOD)
								{	//Mod weapon
									pCD->WeaponMod(ex, ey, ix - 2, iy);
								}
							}
						}
					}
				}

				m_lButtons[m_LastMarkedX][m_LastMarkedY].bMarked = false;
			}
			if(bMarkIt)
			{
				bool bOther = true;
				if(m_XPos > 2)
				{
					for(int i = 0; i < pCD->m_lInventory.Len(); i++)
					{
						if(m_XPos == pCD->m_lInventory[i].x + 2 && m_YPos == pCD->m_lInventory[i].y)
						{
							if(pCD->m_lInventory[i].item.m_LootType == INV_WEAPON_MOD)
							{
								switch(pCD->m_lInventory[i].item.m_SubType)
								{
								case 0:
									pCD->WeaponMod(0, 0, m_XPos - 2, m_YPos);
									break;
								case 1:
									pCD->WeaponMod(1, 0, m_XPos - 2, m_YPos);
									break;
								case 2:
									pCD->WeaponMod(2, 0, m_XPos - 2, m_YPos);
								    break;
								case 3:
									pCD->WeaponMod(0, 1, m_XPos - 2, m_YPos);
								    break;
								case 4:
									pCD->WeaponMod(1, 1, m_XPos - 2, m_YPos);
									break;
								case 5:
									pCD->WeaponMod(2, 1, m_XPos - 2, m_YPos);
									break;
								}
								bOther = false;
							}
							break;
						}
					}
				}
				if(bOther)
				{
					m_lButtons[m_XPos][m_YPos].bMarked = true;
					m_LastMarkedX = m_XPos;
					m_LastMarkedY = m_YPos;
				}
			}
			else
			{
				m_LastMarkedX = -1;
				m_LastMarkedY = -1;
			}
		}
		else
			return CMWnd_CubeMenu::ProcessKey(_Key, _OriginalKey);
	}
	else
		return CMWnd_CubeMenu::ProcessKey(_Key, _OriginalKey);

	if(m_lButtons[m_XPos][m_YPos].bDisabled)
	{
		m_YPos = 1;
//		m_XPos = lx;
//		m_YPos = ly;
	}

	return 1;
}

void CMWnd_CubeMenu_WeaponMod::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	((CWFrontEnd_P6*)m_CubeUser.m_pFrontEndMod)->m_BG = "P6_inv_w_BG";
	CXR_VBManager* pVBM = _pRCUtil->GetVBM();

	CRC_Viewport *pVP = pVBM->Viewport_Get();
	CRct rct = pVP->GetViewRect();
	_pRCUtil->Rect(rct, rct, 0xff000000);

	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);

	MACRO_GetRegisterObject(CGameContext, pGameContext, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGameContext);
	CWO_CharPlayer_ClientData_P6* pCD = NULL;
	if(pGameContext->m_spWServer)
	{
		CWObject_Game *pGame = pGameMod ? pGameContext->m_spWServer->Game_GetObject() : NULL;
		CWO_Player *pPlayer = pGame ? pGame->Player_Get(0) : NULL; 
		CWObject *pCharObj = pPlayer ? pGameContext->m_spWServer->Object_Get(pPlayer->m_iObject) : NULL;
		CWObject_CharPlayer_P6 *pChar  = pCharObj ? safe_cast<CWObject_CharPlayer_P6>(pCharObj) : NULL;
		pCD = pChar ? safe_cast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pChar->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
	}
	else
	{
		CWorld_Client* pClientOrg = pGameContext->GetCurrentClient();
		CWClient_P6 *pClient = safe_cast<CWClient_P6>(pClientOrg);
		CWObject_CoreData *pCoreData = pClient->Object_GetFirstCopy(pClient->Player_GetLocalObject());
		pCD = safe_cast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pCoreData->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	}

	if(!pCD)
		return;

	pVBM->ScopeBegin("CMWnd_CubeMenu_WeaponMod::OnPaint", false);
	CRC_Font *pFont = GetFont("TEXT");
	CPixel32 col = 0xffffffff;
	_pRCUtil->DrawTexture(_Clip, CPnt(400, 5), "P6_inv_markers", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
	_pRCUtil->Text(_Clip, pFont, 545, 17, CStrF("§Z10Markers  %i", pCD->m_Money.m_Value), col);

	_pRCUtil->DrawTexture(_Clip, CPnt(10, 445), "P6_inv_meny", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));

	//Render empty buttons
	for(int y = 0; y < WEAPONMOD_HEIGHT; y++)
	{
		for(int x = 0; x < WEAPONMOD_WIDTH; x++)
		{
			m_lButtons[x][y].Render(_pRCUtil, _Clip);
		}
	}

	//The inventory grid
	_pRCUtil->DrawTexture(_Clip, CPnt(400, 35), "P6_inv_grid01", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
	_pRCUtil->Text(_Clip, pFont, 410, 40, "§Z10Inventory", col);

	//Info1
	_pRCUtil->DrawTexture(_Clip, CPnt(405, 320), "P6_inv_info01", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
	for(int i = 0; i < pCD->m_lInventory.Len(); i++)
	{
		if((pCD->m_lInventory[i].x + 2) == m_XPos && pCD->m_lInventory[i].y == m_YPos)
		{
			_pRCUtil->Text(_Client, pFont, 520, 325, CStrF("§Z10%s", pCD->m_lInventory[i].item.m_LootObject.Str()), col);

			wchar Text[1024];
			Localize_Str(CStrF("§Z10%s", pCD->m_lInventory[i].item.m_Info.Str()), Text, 1023);

			int Len = pCD->m_lInventory[i].item.m_Info.Len() + 4;

			wchar Lines[16][512];
			const int MaxLines = 15;
			wchar* lpLines[16];
			for(int ii = 0; ii < 16; ii++) 
				lpLines[ii] = &Lines[ii][0];

			int nLines = _pRCUtil->Text_WordWrap(pFont, 120, (wchar*) Text, Len, &lpLines[0], 15);

			for(int ln = 0; ln < nLines; ln++)
			{
				int th = _pRCUtil->TextHeight(pFont, &Lines[ln][0]);
				_pRCUtil->Text(_Client, pFont, 520, 335 + ln * th, &Lines[ln][0], col);
			}

			break;
		}
	}

	//Info2
	_pRCUtil->DrawTexture(_Clip, CPnt(550, 320), "P6_inv_info02", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));

	//now lets render the actual inventory
	for(int i = 0; i < pCD->m_lInventory.Len(); i++)
	{
		CPnt pos;
		if(pCD->m_lInventory[i].x)
		{
			pos.x = m_lButtons[pCD->m_lInventory[i].x + 2][pCD->m_lInventory[i].y].x;
			pos.y = m_lButtons[pCD->m_lInventory[i].x + 2][pCD->m_lInventory[i].y].y;

			_pRCUtil->DrawTexture(_Clip, pos, pCD->m_lInventory[i].item.m_GUI_Icon, CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));	
		}
	}

	//Now lets render the weapons inventory
	for(int y = 0; y < 2; y++)
	{
		for(int x = 0; x < 3; x++)
		{
			CPnt pos;
			pos.x = m_lButtons[x][y].x;
			pos.y = m_lButtons[x][y].y;

			_pRCUtil->DrawTexture(_Clip, pos, pCD->m_lInventory[pCD->m_iWeaponIndex].inv[x][y].m_GUI_Icon, CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));	
		}
	}

	if(m_YPos == 0)
	{
		if(m_XPos == 0)	
			_pRCUtil->DrawTexture(_Clip, CPnt(5, 160), "P6_inv_w_slot07", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));	//Kolv
		else if(m_XPos == 1)
			_pRCUtil->DrawTexture(_Clip, CPnt(100, 143), "P6_inv_w_slot02", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));	//Stock
		else if(m_XPos == 2)
			_pRCUtil->DrawTexture(_Clip, CPnt(295, 140), "P6_inv_w_slot04", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));	//Barrel
	}
	else if(m_YPos == 1)
	{
		if(m_XPos == 0)	
			_pRCUtil->DrawTexture(_Clip, CPnt(100, 180), "P6_inv_w_slot06", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));	//Back handle
		else if(m_XPos == 1)
			_pRCUtil->DrawTexture(_Clip, CPnt(168, 175), "P6_inv_w_slot01", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));	//Ammo
		else if(m_XPos == 2)
			_pRCUtil->DrawTexture(_Clip, CPnt(225, 160), "P6_inv_w_slot05", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));	//Front handle
	}
	
//	_pRCUtil->DrawTexture(_Clip, CPnt(320, 160), "P6_inv_w_slot03", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));	//?

	CPnt pos;
	pos.x = m_lButtons[m_XPos][m_YPos].x;
	pos.y = m_lButtons[m_XPos][m_YPos].y;
	_pRCUtil->DrawTexture(_Clip, pos, "P6_inv_indicat02", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));

	pVBM->ScopeEnd();
}
/*
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Trading, CMWnd_CubeMenu);

CMWnd_CubeMenu_Trading::CMWnd_CubeMenu_Trading()
{
	m_XPos = 3;
	m_YPos = 0;
	m_LastMarkedX = -1;
	m_LastMarkedY = -1;

	m_bDoOnce = true;
}

void CMWnd_CubeMenu_Trading::OnCreate()
{
	m_CubeUser.m_pFrontEndMod->m_Cube.m_pCurrentSequence = NULL;	//Hack

	for(int x = 0; x < 3; x++)
	{
		for(int y = 0; y < WEAPONMOD_HEIGHT; y++)	
		{
			m_lButtons[x][y].Init("P6_inv_boxdark", 15 + x * 77, int(52 + y * 32.3f));
		}
	}

	for(int x = 3; x < WEAPONMOD_WIDTH; x++)
	{
		for(int y = 0; y < WEAPONMOD_HEIGHT; y++)	
		{
			m_lButtons[x][y].Init("P6_inv_boxdark", 405 + (x - 3) * 77, int(52 + y * 32.3f));
		}
	}

	ConExecute("deferredscript(\"pause(0)\")");
}

int CMWnd_CubeMenu_Trading::ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
{
	MACRO_GetRegisterObject(CGameContext, pGameContext, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGameContext);
	CWO_CharPlayer_ClientData_P6* pCD = NULL;
	if(pGameContext->m_spWServer)
	{
		CWObject_Game *pGame = pGameMod ? pGameContext->m_spWServer->Game_GetObject() : NULL;
		CWO_Player *pPlayer = pGame ? pGame->Player_Get(0) : NULL; 
		CWObject *pCharObj = pPlayer ? pGameContext->m_spWServer->Object_Get(pPlayer->m_iObject) : NULL;
		CWObject_CharPlayer_P6 *pChar  = pCharObj ? safe_cast<CWObject_CharPlayer_P6>(pCharObj) : NULL;
		pCD = pChar ? safe_cast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pChar->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
	}
	else
	{
		CWorld_Client* pClientOrg = pGameContext->GetCurrentClient();
		CWClient_P6 *pClient = safe_cast<CWClient_P6>(pClientOrg);
		CWObject_CoreData *pCoreData = pClient->Object_GetFirstCopy(pClient->Player_GetLocalObject());
		pCD = safe_cast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pCoreData->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	}

	int32 k = _Key.GetKey9();

	int lx = m_XPos;
	int ly = m_YPos;

	if(_Key.IsDown())
	{
		if(k == SKEY_GUI_LEFT)
		{
			m_XPos--;
			m_XPos = Max(m_XPos, 0);
		}
		else if(k == SKEY_GUI_RIGHT)
		{
			m_XPos++;
			m_XPos = Min(m_XPos, WEAPONMOD_WIDTH - 1);
		}
		else if(k == SKEY_GUI_DOWN)
		{
			m_YPos++;
			m_YPos = Min(m_YPos, WEAPONMOD_HEIGHT - 1);
		}
		else if(k == SKEY_GUI_UP)
		{
			m_YPos--;
			m_YPos = Max(m_YPos, 0);
		}
		else if(k == SKEY_GUI_BUTTON0)
		{
			bool bMarkIt = true;
			if(m_LastMarkedX >= 0 && m_LastMarkedY >= 0)
			{
				if(m_LastMarkedX == m_XPos && m_LastMarkedY == m_YPos)
					bMarkIt = false;
				else
				{	
					if((m_XPos < 3 || m_LastMarkedX < 3) && (m_XPos >= 3 || m_LastMarkedX >= 3))
					{	//Buy/Sell
						bMarkIt = false;
						int ix = -1;
						int iy = -1;
						int ex = -1;
						int ey = -1;
						if(m_XPos < 3)
						{
							ix = m_LastMarkedX;
							iy = m_LastMarkedY;
							ex = m_XPos;
							ey = m_YPos;
						}
						else
						{
							ix = m_XPos;
							iy = m_YPos;
							ex = m_LastMarkedX;
							ey = m_LastMarkedY;
						}
						bool bSell = false;
						bool bBuy = false;
						int iInventoryItem = 0;
						for(int i = 0; i < pCD->m_lInventory.Len(); i++)
						{
							if((pCD->m_lInventory[i].x + 2) == ix &&  pCD->m_lInventory[i].y == iy)
							{
								if(pCD->m_lInventory[i].item.m_GUI_Icon.Len())
								{
									//Sell this item
									iInventoryItem = i;
									bSell = true;
								}
							}
						}
						
						CWO_CharNPC_ClientData_P6* pStoreCD = NULL;
						if(pGameContext->m_spWServer)
						{
							CWObject *pStoreObj = pGameContext->m_spWServer->Object_Get(pCD->m_3PI_FocusObject);
							CWObject_CharNPC_P6 *pStore  = pStoreObj ? safe_cast<CWObject_CharNPC_P6>(pStoreObj) : NULL;
							pStoreCD = pStore ? safe_cast<CWO_CharNPC_ClientData_P6>((CReferenceCount*)pStore->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
						}
						else
						{
							CWObject_CoreData *pCoreData = pGameContext->GetCurrentClient()->Object_GetCD(pCD->m_3PI_FocusObject);
							pStoreCD = safe_cast<CWO_CharNPC_ClientData_P6>((CReferenceCount*)pCoreData->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
						}

						int iStoreItem = ex + ey * 3;	
						TArray<SLootItem> lTmp;
						TArray<int> lTmpIndices;
						for(int ii = 0; ii < pStoreCD->m_lLoot.Len(); ii++)
						{
							if(pStoreCD->m_lLoot[ii].m_GUI_Icon.Len())
							{
								lTmp.Add(pStoreCD->m_lLoot[ii]);
								lTmpIndices.Add(ii);
							}
						}
						if(iStoreItem < lTmp.Len())
						{
							if(lTmp[iStoreItem].m_GUI_Icon.Len())
								bBuy = true;
						}
						if(bSell && !bBuy)
						{	//Sell item
							int iPrice = 0;
							bool bNoSell = false;
							switch(pCD->m_lInventory[iInventoryItem].item.m_LootType) 
							{
							case INV_ARMOR:
								iPrice = 20;
								break;
							case INV_WEAPON:
								iPrice = 30;
								break;
							case INV_WEAPON_MOD:
								iPrice = 50;
								break;
							case INV_HEALTH:
								iPrice = 5;
								break;
							default:
								bNoSell = true;
								iPrice = 0;
								break;
							}
							if(!bNoSell)
							{
								SLootItem SellItem;
								SellItem = pCD->m_lInventory[iInventoryItem].item;
								pStoreCD->m_lLoot.Add(SellItem);
								pCD->m_lInventory[iInventoryItem].item.m_GUI_Icon = "";
								pCD->m_lInventory[iInventoryItem].item.m_Info = "";
								pCD->m_lInventory[iInventoryItem].item.m_LootObject = "";
								pCD->m_lInventory[iInventoryItem].item.m_LootTemplate = "";
								pCD->m_lInventory[iInventoryItem].item.m_LootType = -1;
								pCD->m_lInventory[iInventoryItem].item.m_SubType = -1;
								pCD->m_Money.m_Value += iPrice;
							}
						}
						else if(bBuy && !bSell)
						{	//Buy it
							int iPrice = 0;
							switch(lTmp[iStoreItem].m_LootType) 
							{
							case INV_ARMOR:
								iPrice = 20;
								break;
							case INV_WEAPON:
								iPrice = 30;
								break;
							case INV_WEAPON_MOD:
								iPrice = 50;
								break;
							case INV_HEALTH:
								iPrice = 5;
								break;
							default:
								iPrice = 999;
								break;
							}
							if(pCD->m_Money.m_Value >= iPrice)
							{
								SLootItem BuyItem;
								BuyItem = lTmp[iStoreItem];
								CStr item;
								item = CStrF("%s:%s:%s:%s:%i:%i", BuyItem.m_LootObject.Str(), BuyItem.m_LootTemplate.Str(), BuyItem.m_Info.Str(), BuyItem.m_GUI_Icon.Str(), BuyItem.m_LootType, BuyItem.m_SubType);
								pCD->InventoryAddItem(item);
								pStoreCD->m_lLoot[lTmpIndices[iStoreItem]].m_GUI_Icon = "";
								pStoreCD->m_lLoot[lTmpIndices[iStoreItem]].m_Info = "";
								pStoreCD->m_lLoot[lTmpIndices[iStoreItem]].m_LootObject = "";
								pStoreCD->m_lLoot[lTmpIndices[iStoreItem]].m_LootTemplate = "";
								pStoreCD->m_lLoot[lTmpIndices[iStoreItem]].m_LootType = -1;
								pStoreCD->m_lLoot[lTmpIndices[iStoreItem]].m_SubType = -1;
								pCD->m_Money.m_Value -= iPrice;
							}
						}
					}
				}

				m_lButtons[m_LastMarkedX][m_LastMarkedY].bMarked = false;
			}
			if(bMarkIt)
			{
				m_lButtons[m_XPos][m_YPos].bMarked = true;
				m_LastMarkedX = m_XPos;
				m_LastMarkedY = m_YPos;
			}
			else
			{
				m_LastMarkedX = -1;
				m_LastMarkedY = -1;
			}
		}
		else
			return CMWnd_CubeMenu::ProcessKey(_Key, _OriginalKey);
	}
	else
		return CMWnd_CubeMenu::ProcessKey(_Key, _OriginalKey);

	return 1;
}

void CMWnd_CubeMenu_Trading::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	((CWFrontEnd_P6*)m_CubeUser.m_pFrontEndMod)->m_BG = "P6_inv_BG01";
	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);
	CXR_VBManager* pVBM = _pRCUtil->GetVBM();

	MACRO_GetRegisterObject(CGameContext, pGameContext, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGameContext);
	CWO_CharPlayer_ClientData_P6* pCD = NULL;
	if(pGameContext->m_spWServer)
	{
		CWObject_Game *pGame = pGameMod ? pGameContext->m_spWServer->Game_GetObject() : NULL;
		CWO_Player *pPlayer = pGame ? pGame->Player_Get(0) : NULL; 
		CWObject *pCharObj = pPlayer ? pGameContext->m_spWServer->Object_Get(pPlayer->m_iObject) : NULL;
		CWObject_CharPlayer_P6 *pChar  = pCharObj ? safe_cast<CWObject_CharPlayer_P6>(pCharObj) : NULL;
		pCD = pChar ? safe_cast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pChar->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
	}
	else
	{
		CWorld_Client* pClientOrg = pGameContext->GetCurrentClient();
		CWClient_P6 *pClient = safe_cast<CWClient_P6>(pClientOrg);
		CWObject_CoreData *pCoreData = pClient->Object_GetFirstCopy(pClient->Player_GetLocalObject());
		pCD = safe_cast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pCoreData->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	}

	if(!pCD)
		return;

	pVBM->ScopeBegin(false);
	CRC_Font *pFont = GetFont("TEXT");
	CPixel32 col = 0xffffffff;
	_pRCUtil->DrawTexture(_Clip, CPnt(400, 5), "P6_inv_markers", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
	_pRCUtil->Text(_Clip, pFont, 545, 17, CStrF("§Z10Markers  %i", pCD->m_Money.m_Value), col);

	_pRCUtil->DrawTexture(_Clip, CPnt(10, 445), "P6_inv_meny", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));

	//Render empty buttons
	for(int y = 0; y < WEAPONMOD_HEIGHT; y++)
	{
		for(int x = 0; x < WEAPONMOD_WIDTH; x++)
		{
			m_lButtons[x][y].Render(_pRCUtil, _Clip);
		}
	}

	//The inventory grid
	_pRCUtil->DrawTexture(_Clip, CPnt(400, 35), "P6_inv_grid01", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
	_pRCUtil->Text(_Clip, pFont, 410, 40, "§Z10Inventory", col);

	//Info1
	_pRCUtil->DrawTexture(_Clip, CPnt(405, 320), "P6_inv_info01", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
	for(int i = 0; i < pCD->m_lInventory.Len(); i++)
	{
		if((pCD->m_lInventory[i].x + 2) == m_XPos && pCD->m_lInventory[i].y == m_YPos)
		{
			if(pCD->m_lInventory[i].x)
			{
				_pRCUtil->Text(_Client, pFont, 520, 325, CStrF("§Z10%s",pCD->m_lInventory[i].item.m_LootObject.Str()), col);

				wchar Text[1024];
				Localize_Str(CStrF("§Z10%s", pCD->m_lInventory[i].item.m_Info.Str()), Text, 1023);

				int Len = pCD->m_lInventory[i].item.m_Info.Len() + 4;

				wchar Lines[16][512];
				const int MaxLines = 15;
				wchar* lpLines[16];
				for(int ii = 0; ii < 16; ii++) 
					lpLines[ii] = &Lines[ii][0];

				int nLines = _pRCUtil->Text_WordWrap(pFont, 120, (wchar*) Text, Len, &lpLines[0], 15);

				for(int ln = 0; ln < nLines; ln++)
				{
					int th = _pRCUtil->TextHeight(pFont, &Lines[ln][0]);
					_pRCUtil->Text(_Client, pFont, 520, 335 + ln * th, &Lines[ln][0], col);
				}
			}

			break;
		}
	}

	//Info2
	_pRCUtil->DrawTexture(_Clip, CPnt(550, 320), "P6_inv_info02", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));

	//now lets render the actual inventory
	for(int i = 0; i < pCD->m_lInventory.Len(); i++)
	{
		CPnt pos;
		int x = pCD->m_lInventory[i].x + 2;
		if(x >= 3)
		{
			pos.x = m_lButtons[x][pCD->m_lInventory[i].y].x;
			pos.y = m_lButtons[x][pCD->m_lInventory[i].y].y;

			_pRCUtil->DrawTexture(_Clip, pos, pCD->m_lInventory[i].item.m_GUI_Icon, CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));	

			if(pCD->m_lInventory[i].item.m_LootType == INV_QUEST)
			{
				_pRCUtil->DrawTexture(_Clip, pos, "P6_inv_red", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));	
			}
		}
	}

	//Now render the store
	//The inventory grid
	_pRCUtil->DrawTexture(_Clip, CPnt(7, 3), "P6_inv_trade01", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
	_pRCUtil->Text(_Clip, pFont, 20, 13, "§Z10Store", col);

	//now lets render the actual store
	if(pCD->m_3PI_FocusObject && pCD->m_3PI_FocusObject != -1)
	{
		CWO_CharNPC_ClientData_P6* pStoreCD = NULL;
		if(pGameContext->m_spWServer)
		{
			CWObject *pStoreObj = pGameContext->m_spWServer->Object_Get(pCD->m_3PI_FocusObject);
			CWObject_CharNPC_P6 *pStore  = pStoreObj ? safe_cast<CWObject_CharNPC_P6>(pStoreObj) : NULL;
			pStoreCD = pStore ? safe_cast<CWO_CharNPC_ClientData_P6>((CReferenceCount*)pStore->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
		}
		else
		{
			CWObject_CoreData *pCoreData = pGameContext->GetCurrentClient()->Object_GetCD(pCD->m_3PI_FocusObject);
			pStoreCD = safe_cast<CWO_CharNPC_ClientData_P6>((CReferenceCount*)pCoreData->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
		}
		int sx = 0;
		int sy = 0;
		for(int i = 0; i < pStoreCD->m_lLoot.Len(); i++)
		{
			if(pStoreCD->m_lLoot[i].m_GUI_Icon.Len())
			{
				CPnt pos;
				pos.x = m_lButtons[sx][sy].x;
				pos.y = m_lButtons[sx][sy].y;

				_pRCUtil->DrawTexture(_Clip, pos, pStoreCD->m_lLoot[i].m_GUI_Icon, CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));	

				int iPrice = 999;
				switch(pStoreCD->m_lLoot[i].m_LootType) 
				{
				case INV_ARMOR:
					iPrice = 20;
					break;
				case INV_WEAPON:
					iPrice = 30;
					break;
				case INV_WEAPON_MOD:
					iPrice = 50;
					break;
				case INV_HEALTH:
					iPrice = 5;
					break;
				default:
					iPrice = 999;
					break;
				}
				if(pCD->m_Money.m_Value < iPrice)
				{
					_pRCUtil->DrawTexture(_Clip, pos, "P6_inv_red", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));	
				}

				sx++;
				if(sx == 3)
				{
					sx = 0;
					sy++;
				}
			}
		}
	}

	_pRCUtil->DrawTexture(_Clip, CPnt(10, 320), "P6_inv_trade02", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));

	if(m_XPos < 3)
	{	//Buying
		CWO_CharNPC_ClientData_P6* pStoreCD = NULL;
		if(pGameContext->m_spWServer)
		{
			CWObject *pStoreObj = pGameContext->m_spWServer->Object_Get(pCD->m_3PI_FocusObject);
			CWObject_CharNPC_P6 *pStore  = pStoreObj ? safe_cast<CWObject_CharNPC_P6>(pStoreObj) : NULL;
			pStoreCD = pStore ? safe_cast<CWO_CharNPC_ClientData_P6>((CReferenceCount*)pStore->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
		}
		else
		{
			CWObject_CoreData *pCoreData = pGameContext->GetCurrentClient()->Object_GetCD(pCD->m_3PI_FocusObject);
			pStoreCD = safe_cast<CWO_CharNPC_ClientData_P6>((CReferenceCount*)pCoreData->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
		}
		int iStoreItem = m_XPos + m_YPos * 3;	
		TArray<SLootItem> lTmp;
		for(int ii = 0; ii < pStoreCD->m_lLoot.Len(); ii++)
		{
			if(pStoreCD->m_lLoot[ii].m_GUI_Icon.Len())
			{
				lTmp.Add(pStoreCD->m_lLoot[ii]);
			}
		}
		if(iStoreItem < lTmp.Len())
		{
			if(lTmp[iStoreItem].m_GUI_Icon.Len())
			{
				int iPrice;
				switch(lTmp[iStoreItem].m_LootType) 
				{
				case INV_ARMOR:
					iPrice = 20;
					break;
				case INV_WEAPON:
					iPrice = 30;
					break;
				case INV_WEAPON_MOD:
					iPrice = 50;
					break;
				case INV_HEALTH:
					iPrice = 5;
					break;
				default:
					iPrice = 0;
					break;
				}

				if(iPrice)
					_pRCUtil->Text(_Clip, pFont, 20, 330, CStrF("§Z10This item can be bought for %i markers", iPrice), col);
				else
					_pRCUtil->Text(_Clip, pFont, 20, 330, "§Z10This item cannot be bought", col);

				if(iPrice > pCD->m_Money.m_Value)
					_pRCUtil->Text(_Clip, pFont, 20, 345, "§Z10You cannot afford this item!", col);

				_pRCUtil->Text(_Client, pFont, 520, 325, CStrF("§Z10%s", lTmp[iStoreItem].m_LootObject.Str()), col);

				wchar Text[1024];
				Localize_Str(CStrF("§Z10%s", lTmp[iStoreItem].m_Info.Str()), Text, 1023);

				int Len = lTmp[iStoreItem].m_Info.Len() + 4;

				wchar Lines[16][512];
				const int MaxLines = 15;
				wchar* lpLines[16];
				for(int ii = 0; ii < 16; ii++) 
					lpLines[ii] = &Lines[ii][0];

				int nLines = _pRCUtil->Text_WordWrap(pFont, 120, (wchar*) Text, Len, &lpLines[0], 15);

				for(int ln = 0; ln < nLines; ln++)
				{
					int th = _pRCUtil->TextHeight(pFont, &Lines[ln][0]);
					_pRCUtil->Text(_Client, pFont, 520, 335 + ln * th, &Lines[ln][0], col);
				}
			}
		}
	}
	else
	{	//Selling
		int iInventoryItem = -1;
		for(int i = 0; i < pCD->m_lInventory.Len(); i++)
		{
			if((pCD->m_lInventory[i].x + 2) == m_XPos &&  pCD->m_lInventory[i].y == m_YPos)
			{
				if(pCD->m_lInventory[i].item.m_GUI_Icon.Len())
				{
					//Sell this item
					iInventoryItem = i;
					break;
				}
			}
		}
		if(iInventoryItem > -1)
		{
			int iPrice;
			switch(pCD->m_lInventory[iInventoryItem].item.m_LootType) 
			{
			case INV_ARMOR:
				iPrice = 20;
				break;
			case INV_WEAPON:
				iPrice = 30;
				break;
			case INV_WEAPON_MOD:
				iPrice = 50;
				break;
			case INV_HEALTH:
				iPrice = 5;
				break;
			default:
				iPrice = 0;
				break;
			}

			if(iPrice)
				_pRCUtil->Text(_Clip, pFont, 20, 330, CStrF("§Z10This item can be sold for %i markers", iPrice), col);
			else
				_pRCUtil->Text(_Clip, pFont, 20, 330, "§Z10This item cannot be sold", col);
		}
	}

	CPnt pos;
	pos.x = m_lButtons[m_XPos][m_YPos].x;
	pos.y = m_lButtons[m_XPos][m_YPos].y;
	_pRCUtil->DrawTexture(_Clip, pos, "P6_inv_indicat02", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));

	pVBM->ScopeEnd();
}
*/

CMWnd_CubeMenu_Multiplayer_SelectServerList_P6::CMWnd_CubeMenu_Multiplayer_SelectServerList_P6()
{
	m_LastUpdate = CMTime::GetCPU();
}

void CMWnd_CubeMenu_Multiplayer_SelectServerList_P6::OnCreate()
{
	CMWnd_CubeMenu::OnCreate();

	m_iSelected = -1;

#if defined(PLATFORM_CONSOLE)
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

	pGameMod->m_pMPHandler->m_Map = "p6_sample";
#endif
}

void CMWnd_CubeMenu_Multiplayer_SelectServerList_P6::OnActivate()
{
	m_bNoServers = true;
	m_List.Clear();
	m_ActivateTime = CMTime::GetCPU();
}

bool CMWnd_CubeMenu_Multiplayer_SelectServerList_P6::OnButtonPress(CMWnd* _pFocus)
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

int32 CMWnd_CubeMenu_Multiplayer_SelectServerList_P6::GetNumListItems(bool _Drawing)
{
	return m_List.Len();
}

bool CMWnd_CubeMenu_Multiplayer_SelectServerList_P6::GetListItem(int32 _Index, CStr& _Name, bool _bFocus)
{
	_Name = CStrF(WTEXT("s, %s"), m_List[_Index].StrW());

	if (_bFocus)	
		m_iSelected = _Index;

	return true;
}

void CMWnd_CubeMenu_Multiplayer_SelectServerList_P6::OnRefresh()
{
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
#if defined(PLATFORM_XENON)
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
				m_List.Add(WTEXT("No server found"));
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

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Multiplayer_SelectServerList_P6, CMWnd_CubeMenu);




















