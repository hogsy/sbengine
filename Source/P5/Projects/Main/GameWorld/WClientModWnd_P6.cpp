#include "PCH.h"
#include "WClientModWnd_P6.h"
#include "MWinCtrlMod.h"
#include "../Exe/WGameContextMain.h"
#include "../GameClasses/WObj_Game/WObj_GameCore.h"
#include "../GameClasses/WObj_Char/WObj_CharPlayer_P6.h"
#include "../GameClasses/WObj_Char/WObj_CharNPC_P6.h"
#include "WClient_P6.h"

CMWnd_LootList::CMWnd_LootList()
{
	m_ItemHeight = 34;
	m_TextHeight = 14;
}

void CMWnd_LootList::SetItemCount(int _nItems)
{
	m_lChars.QuickSetLen(_nItems);
	CMWnd_List2::SetItemCount(_nItems);
}

void CMWnd_LootList::AddItem(const CListItem_Loot& _Item)
{
	m_lChars.Add(_Item);
	CMWnd_List2::SetItemCount(m_lChars.Len());
}

void CMWnd_LootList::OnPaintItem(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client, int _iItem)
{
	CRC_Font* pF = GetFont("TEXT");
	if(!pF)
		return;

	if(GetItemFocus() == _iItem)
		_pRCUtil->DrawTexture(_Clip, CPnt(0, 0), "P6_inv_indicat02", 0xffffffff, CVec2Dfp32(5.0f, 5.0f));
	_pRCUtil->DrawTexture(_Clip, CPnt(0, 0), m_lChars[_iItem].m_GUI_Icon, 0xffffffff, CVec2Dfp32(5.0f, 5.0f));
}

void CMWnd_LootList::OnTouchItem(int _iItem, int _bSelected)
{
	if (GetParent())
	{
		CMWnd_Message Msg(WMSG_COMMAND, m_ID, _iItem, _bSelected);
		GetParent()->OnMessage(&Msg);
	}
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_LootList, CMWnd_List2);

MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModLoot_P6, CMWnd);

CMWnd_ModLoot_P6::CMWnd_ModLoot_P6()
{
	m_LastUpdateTick = 0;
	m_LastLootTick = 0;
	m_bCreated = false;
	m_bParentCreated = false;
}

int CMWnd_ModLoot_P6::GetItemState(const char* _pName)
{
	CMWnd* pWnd = GetItem(_pName);
	if (pWnd)
	{
		CMWnd_Message Msg(WMSG_GETSTATE, NULL, 0);
		return pWnd->OnMessage(&Msg);
	}
	return 0;
}

void CMWnd_ModLoot_P6::SetItemState(const char* _pName, int _Value)
{
	CMWnd* pWnd = GetItem(_pName);
	if (pWnd)
	{
		CMWnd_Message Msg(WMSG_SETSTATE, NULL, _Value);
		pWnd->OnMessage(&Msg);
	}
}

void CMWnd_ModLoot_P6::OnCreate()
{
	if(!m_bParentCreated)
	{
		CMWnd::OnCreate();
		m_bParentCreated = true;
	}

	CMWnd* pWnd = GetItem("LootList");
	CMWnd_LootList* pList = safe_cast<CMWnd_LootList>(pWnd);

	int iObj = -1;
	MACRO_GetRegisterObject(CGameContext, pGameContext, "GAMECONTEXT");
	CWO_CharPlayer_ClientData_P6* pCD = NULL;
	CWO_CharNPC_ClientData_P6* pLootCD = NULL;
	if(pGameContext->m_spWServer)
	{
		CWObject_Game *pGame = pGameContext->m_spWServer->Game_GetObject();
		CWO_Player *pPlayer = pGame ? pGame->Player_Get(0) : NULL; 
		CWObject *pCharObj = pPlayer ? pGameContext->m_spWServer->Object_Get(pPlayer->m_iObject) : NULL;
		CWObject_CharPlayer_P6 *pChar  = pCharObj ? safe_cast<CWObject_CharPlayer_P6>(pCharObj) : NULL;
		pCD = pChar ? TDynamicCast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pChar->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
		iObj = pPlayer->m_iObject;

		CWObject *pLootCharObj = pGameContext->m_spWServer->Object_Get(pCD->m_iLootTarget);
		CWObject_CharNPC_P6 *pLootChar  = pLootCharObj ? safe_cast<CWObject_CharNPC_P6>(pLootCharObj) : NULL;
		pLootCD = pLootChar ? TDynamicCast<CWO_CharNPC_ClientData_P6>((CReferenceCount*)pLootChar->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
	}
	else
	{
		CWorld_Client* pClientOrg = pGameContext->GetCurrentClient();
		CWClient_P6 *pClient = safe_cast<CWClient_P6>(pClientOrg);
		CWObject_CoreData *pCoreData = pClient->Object_GetFirstCopy(pClient->Player_GetLocalObject());
		pCD = safe_cast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pCoreData->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
		iObj = pClient->Player_GetLocalObject();

		M_TRACEALWAYS(CStrF("Loot window is showing NPC %i", (int32)pCD->m_iLootTarget));

		if(pCD->m_iLootTarget != -1)
		{
			CWObject_CoreData *pLootCoreData = pClient->Object_GetCD(pCD->m_iLootTarget);
			pLootCD = pLootCoreData ? safe_cast<CWO_CharNPC_ClientData_P6>((CReferenceCount*)pLootCoreData->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
		}
	}

	if(!pCD || !pLootCD)
	{
		ConExecute(CStrF("loot_exit(%i)", iObj));
		return;
	}

	m_bCreated = true;

	CListItem_Loot Item;
	if(pLootCD->m_Money)
	{
		Item.m_LootName = CStrF("%i Markers", (int32)pLootCD->m_Money);
		Item.m_GUI_Icon = "P6_inv_dogtag";
		Item.m_Money = pLootCD->m_Money;
		pList->AddItem(Item);
	}
	for(int i = 0; i < pLootCD->m_lLoot.Len(); i++)
	{
		if(pLootCD->m_lLoot[i].m_LootObject.Len())
		{
			Item.m_Money = 0;
			Item.m_LootName = pLootCD->m_lLoot[i].m_LootObject;
			Item.m_GUI_Icon = pLootCD->m_lLoot[i].m_GUI_Icon;
			Item.m_iLootIndex = i;
			pList->AddItem(Item);
		}
	}
}

void CMWnd_ModLoot_P6::OnRefresh()
{
	if(!m_bCreated)
		OnCreate();

	if(m_ClientInfo.m_pClient->GetGameTick() < (RoundToInt(1.0f * m_ClientInfo.m_pClient->GetGameTicksPerSecond()) + m_LastUpdateTick))
		return;

	m_LastUpdateTick = m_ClientInfo.m_pClient->GetGameTick();
}

void CMWnd_ModLoot_P6::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	_pRCUtil->DrawTexture(_Clip, CPnt(0, 0), "P6_inv_loot", 0xffffffff, CVec2Dfp32(5.0f, 5.0f));
	_pRCUtil->DrawTexture(_Clip, CPnt(142, 15), "P6_inv_loot_bg", 0xffffffff, CVec2Dfp32(5.0f, 5.0f));

	CMWnd* pWnd = GetItem("LootList");
	CMWnd_LootList* pList = safe_cast<CMWnd_LootList>(pWnd);

	if (pList)
	{
		int iItem = pList->GetItemFocus();
		if (pList->m_lChars.ValidPos(iItem))
		{
			CRC_Font *pFont = GetFont("TEXT");

			MACRO_GetRegisterObject(CGameContext, pGameContext, "GAMECONTEXT");
			CWO_CharPlayer_ClientData_P6* pCD = NULL;
			CWO_CharNPC_ClientData_P6* pLootCD = NULL;
			if(pGameContext->m_spWServer)
			{
				CWObject_Game *pGame = pGameContext->m_spWServer->Game_GetObject();
				CWO_Player *pPlayer = pGame ? pGame->Player_Get(0) : NULL; 
				CWObject *pCharObj = pPlayer ? pGameContext->m_spWServer->Object_Get(pPlayer->m_iObject) : NULL;
				CWObject_CharPlayer_P6 *pChar  = pCharObj ? safe_cast<CWObject_CharPlayer_P6>(pCharObj) : NULL;
				pCD = pChar ? TDynamicCast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pChar->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;

				CWObject *pLootCharObj = pGameContext->m_spWServer->Object_Get(pCD->m_iLootTarget);
				CWObject_CharNPC_P6 *pLootChar  = pLootCharObj ? safe_cast<CWObject_CharNPC_P6>(pLootCharObj) : NULL;
				pLootCD = pLootChar ? TDynamicCast<CWO_CharNPC_ClientData_P6>((CReferenceCount*)pLootChar->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
			}
			else
			{
				CWorld_Client* pClientOrg = pGameContext->GetCurrentClient();
				CWClient_P6 *pClient = safe_cast<CWClient_P6>(pClientOrg);
				CWObject_CoreData *pCoreData = pClient->Object_GetFirstCopy(pClient->Player_GetLocalObject());
				pCD = safe_cast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pCoreData->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);

				CWObject_CoreData *pLootCoreData = pClient->Object_GetCD(pCD->m_iLootTarget);
				pLootCD = safe_cast<CWO_CharNPC_ClientData_P6>((CReferenceCount*)pLootCoreData->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
			}

			CListItem_Loot* pItem = &pList->m_lChars[iItem];

			wchar Text[1024];
			int Len = 0;
			if(!pItem->m_Money)
			{	
				//Localize_Str(CStrF("§Z10%s", pLootCD->m_lLoot[pItem->m_iLootIndex].m_Info.Str()), Text, 1023);
				//Len = pLootCD->m_lLoot[pItem->m_iLootIndex].m_Info.Len() + 4;
				Localize_Str(CStrF("§Z10%s", pItem->m_LootName.Str()), Text, 1023);
				Len = pItem->m_LootName.Len() + 4;
			}
			else
			{
				Localize_Str(CStrF("§Z10%s", pItem->m_LootName.Str()), Text, 1023);
				Len = pItem->m_LootName.Len() + 4;
			}

			wchar Lines[16][512];
			const int MaxLines = 15;
			wchar* lpLines[16];
			for(int ii = 0; ii < 16; ii++) 
				lpLines[ii] = &Lines[ii][0];

			int nLines = _pRCUtil->Text_WordWrap(pFont, 130, (wchar*) Text, Len, &lpLines[0], 15);

			for(int ln = 0; ln < nLines; ln++)
			{
				int th = _pRCUtil->TextHeight(pFont, &Lines[ln][0]);
				_pRCUtil->Text(_Client, pFont, 170, 70 + ln * th, &Lines[ln][0], 0xffffffff);
			}
		}
	}
}

aint CMWnd_ModLoot_P6::OnMessage(const CMWnd_Message* _pMsg)
{
	switch(_pMsg->m_Msg)
	{
	case WMSG_GETMAPDATA : 
		return (aint)(CMapData*) ((m_ClientInfo.m_pClient) ? m_ClientInfo.m_pClient->m_spGUIData : (TPtr<CMapData>)NULL);

	case WMSG_KEY :
		{
			CScanKey Key;
			Key.m_ScanKey32 = _pMsg->m_Param0;

			if (Key.GetKey9() == SKEY_GUI_CANCEL || Key.GetKey9() == SKEY_GUI_BACK)
			{
				MACRO_GetRegisterObject(CGameContext, pGameContext, "GAMECONTEXT");
				CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGameContext);
				int iObj = -1;
				if(pGameContext->m_spWServer)
				{
					CWObject_Game *pGame = pGameMod ? pGameContext->m_spWServer->Game_GetObject() : NULL;
					CWO_Player *pPlayer = pGame ? pGame->Player_Get(0) : NULL; 
					iObj = pPlayer->m_iObject;
				}
				else
				{
					CWorld_Client* pClientOrg = pGameContext->GetCurrentClient();
					CWClient_P6 *pClient = safe_cast<CWClient_P6>(pClientOrg);
					iObj = pClient->Player_GetLocalObject();
				}
				ConExecute(CStrF("loot_exit(%i)", iObj));
			}

			return CMWnd::OnMessage(_pMsg);
		}
	case WMSG_COMMAND :
		if (_pMsg->m_pID)
		{
			if (strcmp(_pMsg->m_pID, "LootList") == 0)
			{
				CMWnd* pWnd = GetItem("LootList");
				CMWnd_LootList* pList = safe_cast<CMWnd_LootList>(pWnd);

				if (pList)
				{
					int iItem = pList->GetSelected();
					if (pList->m_lChars.ValidPos(iItem) && m_LastLootTick != m_ClientInfo.m_pClient->GetGameTick())
					{
						MACRO_GetRegisterObject(CGameContext, pGameContext, "GAMECONTEXT");
						CWO_CharPlayer_ClientData_P6* pCD = NULL;
						CWO_CharNPC_ClientData_P6* pLootCD = NULL;
						int32 iObj = 0;
						if(pGameContext->m_spWServer)
						{
							CWObject_Game *pGame = pGameContext->m_spWServer->Game_GetObject();
							CWO_Player *pPlayer = pGame ? pGame->Player_Get(0) : NULL; 
							CWObject *pCharObj = pPlayer ? pGameContext->m_spWServer->Object_Get(pPlayer->m_iObject) : NULL;
							CWObject_CharPlayer_P6 *pChar  = pCharObj ? safe_cast<CWObject_CharPlayer_P6>(pCharObj) : NULL;
							iObj = pChar->m_iObject;
							pCD = pChar ? TDynamicCast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pChar->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;

							CWObject *pLootCharObj = pGameContext->m_spWServer->Object_Get(pCD->m_iLootTarget);
							CWObject_CharNPC_P6 *pLootChar  = pLootCharObj ? safe_cast<CWObject_CharNPC_P6>(pLootCharObj) : NULL;
							pLootCD = pLootChar ? TDynamicCast<CWO_CharNPC_ClientData_P6>((CReferenceCount*)pLootChar->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
						}
						else
						{
							CWorld_Client* pClientOrg = pGameContext->GetCurrentClient();
							CWClient_P6 *pClient = safe_cast<CWClient_P6>(pClientOrg);
							CWObject_CoreData *pCoreData = pClient->Object_GetFirstCopy(pClient->Player_GetLocalObject());
							pCD = safe_cast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pCoreData->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
							iObj = pCD->m_pObj->m_iObject;

							CWObject_CoreData *pLootCoreData = pClient->Object_GetCD(pCD->m_iLootTarget);
							pLootCD = safe_cast<CWO_CharNPC_ClientData_P6>((CReferenceCount*)pLootCoreData->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
						}

						m_LastLootTick = m_ClientInfo.m_pClient->GetGameTick();

						CListItem_Loot* pItem = &pList->m_lChars[iItem];
						if(pItem->m_Money)
						{
							ConExecute(CStrF("loot_money(%i)", iObj));
							pList->m_lChars.Del(iItem);
							pList->SetItemCount(pList->m_lChars.Len());
						}
						else
						{
							ConExecute(CStrF("loot_object(%i, %i)", iObj, pItem->m_iLootIndex));
							pList->m_lChars.Del(iItem);
							pList->SetItemCount(pList->m_lChars.Len());
						}

						pList->DeselectAll();

						if(pList->m_nItems <= 0)
						{
							int iObj = -1;
							if(pGameContext->m_spWServer)
							{
								CWObject_Game *pGame = pGameContext->m_spWServer->Game_GetObject();
								CWO_Player *pPlayer = pGame ? pGame->Player_Get(0) : NULL; 
								iObj = pPlayer->m_iObject;
							}
							else
							{
								CWorld_Client* pClientOrg = pGameContext->GetCurrentClient();
								CWClient_P6 *pClient = safe_cast<CWClient_P6>(pClientOrg);
								iObj = pClient->Player_GetLocalObject();
							}
							ConExecute(CStrF("loot_exit(%i)", iObj));
						}
					}
				}
			}
		}
	default:
		return CMWnd::OnMessage(_pMsg);
	}
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModTrading_P6, CMWnd);

CMWnd_ModTrading_P6::CMWnd_ModTrading_P6()
{
	m_XPos = 3;
	m_YPos = 0;
	m_LastMarkedX = -1;
	m_LastMarkedY = -1;

	m_bDoOnce = true;
}

void CMWnd_ModTrading_P6::OnCreate()
{
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

	int iObj = -1;
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
		iObj = pPlayer->m_iObject;
	}
	else
	{
		CWorld_Client* pClientOrg = pGameContext->GetCurrentClient();
		CWClient_P6 *pClient = safe_cast<CWClient_P6>(pClientOrg);
		CWObject_CoreData *pCoreData = pClient->Object_GetFirstCopy(pClient->Player_GetLocalObject());
		pCD = safe_cast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pCoreData->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
		iObj = pClient->Player_GetLocalObject();
	}
	
	if(!pCD || pCD->m_3PI_FocusObject == -1)
		ConExecute(CStrF("loot_exit(%i)", iObj));
}


aint CMWnd_ModTrading_P6::OnMessage(const CMWnd_Message* _pMsg)
{
	if(_pMsg->m_Msg == WMSG_KEY)
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

		CScanKey Key;
		Key.m_ScanKey32 = _pMsg->m_Param0;

		int32 k = Key.GetKey9();

		int lx = m_XPos;
		int ly = m_YPos;

		if(Key.IsDown())
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
				static int i = 0;
				i++;
				if(i % 2)
					return CMWnd::OnMessage(_pMsg);

				bool bSell = false;
				bool bBuy = false;
				int iInventoryItem = 0;
				for(int i = 0; i < pCD->m_lInventory.Len(); i++)
				{
					if((pCD->m_lInventory[i].x + 2) == m_XPos &&  pCD->m_lInventory[i].y == m_YPos)
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
				if(!pStoreCD)
				{
					int iObj = -1;
					if(pGameContext->m_spWServer)
					{
						CWObject_Game *pGame = pGameMod ? pGameContext->m_spWServer->Game_GetObject() : NULL;
						CWO_Player *pPlayer = pGame ? pGame->Player_Get(0) : NULL; 
						iObj = pPlayer->m_iObject;
					}
					else
					{
						CWorld_Client* pClientOrg = pGameContext->GetCurrentClient();
						CWClient_P6 *pClient = safe_cast<CWClient_P6>(pClientOrg);
						iObj = pClient->Player_GetLocalObject();
					}
					ConExecute(CStrF("loot_exit(%i)", iObj));
					return 1;
				}

				int iStoreItem = m_XPos + m_YPos * 3;	
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
				if(bSell)
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
						bool bAdded = false;
						for(int ii = 0; ii < pStoreCD->m_lLoot.Len(); ii++)
						{
							if(pStoreCD->m_lLoot[ii].m_LootType == -1)
							{
								pStoreCD->m_lLoot[ii] = SellItem;
								bAdded = true;
								break;
							}
						}
						if(!bAdded && pStoreCD->m_lLoot.Len() < 20)
						{
							pStoreCD->m_lLoot.Add(SellItem);
						}
						pCD->m_lInventory[iInventoryItem].item.m_GUI_Icon = "";
						pCD->m_lInventory[iInventoryItem].item.m_Info = "";
						pCD->m_lInventory[iInventoryItem].item.m_LootObject = "";
						pCD->m_lInventory[iInventoryItem].item.m_LootTemplate = "";
						pCD->m_lInventory[iInventoryItem].item.m_LootType = -1;
						pCD->m_lInventory[iInventoryItem].item.m_SubType = -1;
						pCD->m_Money.m_Value += iPrice;
					}
				}
				else if(bBuy)
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
			else if(k == SKEY_GUI_BACK || k == SKEY_GUI_CANCEL)
			{
				MACRO_GetRegisterObject(CGameContext, pGameContext, "GAMECONTEXT");
				CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGameContext);
				int iObj = -1;
				if(pGameContext->m_spWServer)
				{
					CWObject_Game *pGame = pGameMod ? pGameContext->m_spWServer->Game_GetObject() : NULL;
					CWO_Player *pPlayer = pGame ? pGame->Player_Get(0) : NULL; 
					iObj = pPlayer->m_iObject;
				}
				else
				{
					CWorld_Client* pClientOrg = pGameContext->GetCurrentClient();
					CWClient_P6 *pClient = safe_cast<CWClient_P6>(pClientOrg);
					iObj = pClient->Player_GetLocalObject();
				}
				ConExecute(CStrF("loot_exit(%i)", iObj));	
			}

		}
	}
	return CMWnd::OnMessage(_pMsg);
}

void CMWnd_ModTrading_P6::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	CXR_VBManager* pVBM = _pRCUtil->GetVBM();
	
	CRC_Viewport *pVP = pVBM->Viewport_Get();
	CRct rct = pVP->GetViewRect();
	_pRCUtil->Rect(rct, rct, 0xff000000);

	CMWnd::OnPaint(_pRCUtil, _Clip, _Client);

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
  	pVBM->ScopeBegin("CMWnd_ModTrading_P6::OnPaint", false);

	CRC_Viewport* pViewport = pVBM->Viewport_Get();

	const CRct& ViewRect = pViewport->GetViewRect();
	fp32 Width = ViewRect.GetWidth();
	fp32 Height = ViewRect.GetHeight();

	if(Width / Height * (3.0f / 4) * _pRCUtil->GetRC()->GetDC()->GetPixelAspect() > (1.0f + 1.3333333f) / 2)
		_pRCUtil->SetTexture("P6_inv_BG01_w");
	else
		_pRCUtil->SetTexture("P6_inv_BG01");
	_pRCUtil->AspectRect(_Clip, _Clip.clip, CPnt(_pRCUtil->GetTextureWidth(), _pRCUtil->GetTextureHeight()), 1.0f);
	
	///////////////////////
	_pRCUtil->SetCoordinateScale(CVec2Dfp32(ViewRect.GetHeight() / 480.0f, ViewRect.GetHeight() / 480.0f));

	CClipRect Clip = _Clip;
	fp32 aspect = ((fp32)ViewRect.p1.x / (fp32)ViewRect.p1.y);
	if(aspect > 1.4f)
	{
		CVec2Dfp32 scale(ViewRect.GetHeight() / 480.0f, ViewRect.GetHeight() / 480.0f);
		int p1 = TruncToInt(Clip.clip.p1.x * scale.k[0]);
		Clip.ofs.x = TruncToInt(((ViewRect.p1.x - p1) / 2) / scale.k[0]);
		Clip.clip.p0.x = TruncToInt(((ViewRect.p1.x - p1) / 2) / scale.k[0]);
		Clip.clip.p1.x += TruncToInt(((ViewRect.p1.x - p1) / 2) / scale.k[0]);
//		Clip.ofs.x = int((Width - _Clip.clip.p1.x) / 2.0f);
//		Clip.clip.p0.x = Clip.ofs.x;
//		Clip.clip.p1.x += Clip.ofs.x;
	}

	CRC_Font *pFont = GetFont("TEXT");
	CPixel32 col = 0xffffffff;
	_pRCUtil->DrawTexture(Clip, CPnt(400, 5), "P6_inv_markers", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
	_pRCUtil->Text(Clip, pFont, 545, 17, CStrF("§Z10Markers  %i", pCD->m_Money.m_Value), col);

	_pRCUtil->DrawTexture(Clip, CPnt(10, 445), "P6_inv_meny", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));

	//Render empty buttons
	for(int y = 0; y < WEAPONMOD_HEIGHT; y++)
	{
		for(int x = 0; x < WEAPONMOD_WIDTH; x++)
		{
			m_lButtons[x][y].Render(_pRCUtil, Clip);
		}
	}

	//The inventory grid
	_pRCUtil->DrawTexture(Clip, CPnt(400, 35), "P6_inv_grid01", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
	_pRCUtil->Text(Clip, pFont, 410, 40, "§Z10Inventory", col);

	//Info1
	_pRCUtil->DrawTexture(Clip, CPnt(405, 320), "P6_inv_info01", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
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
	_pRCUtil->DrawTexture(Clip, CPnt(550, 320), "P6_inv_info02", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));

	//now lets render the actual inventory
	for(int i = 0; i < pCD->m_lInventory.Len(); i++)
	{
		CPnt pos;
		int x = pCD->m_lInventory[i].x + 2;
		if(x >= 3)
		{
			pos.x = m_lButtons[x][pCD->m_lInventory[i].y].x;
			pos.y = m_lButtons[x][pCD->m_lInventory[i].y].y;

			_pRCUtil->DrawTexture(Clip, pos, pCD->m_lInventory[i].item.m_GUI_Icon, CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));	

			if(pCD->m_lInventory[i].item.m_LootType == INV_QUEST)
			{
				_pRCUtil->DrawTexture(Clip, pos, "P6_inv_red", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));	
			}
		}
	}

	//Now render the store
	//The inventory grid
	_pRCUtil->DrawTexture(Clip, CPnt(7, 3), "P6_inv_trade01", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));
	_pRCUtil->Text(Clip, pFont, 20, 13, "§Z10Store", col);

	//now lets render the actual store
	if(pCD->m_3PI_FocusObject != -1)
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

				_pRCUtil->DrawTexture(Clip, pos, pStoreCD->m_lLoot[i].m_GUI_Icon, CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));	

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
					_pRCUtil->DrawTexture(Clip, pos, "P6_inv_red", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));	
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

	_pRCUtil->DrawTexture(Clip, CPnt(10, 320), "P6_inv_trade02", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));

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
					_pRCUtil->Text(Clip, pFont, 20, 330, CStrF("§Z10This item can be bought for %i markers", iPrice), col);
				else
					_pRCUtil->Text(Clip, pFont, 20, 330, "§Z10This item cannot be bought", col);

				if(iPrice > pCD->m_Money.m_Value)
					_pRCUtil->Text(Clip, pFont, 20, 345, "§Z10You cannot afford this item!", col);

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
				_pRCUtil->Text(Clip, pFont, 20, 330, CStrF("§Z10This item can be sold for %i markers", iPrice), col);
			else
				_pRCUtil->Text(Clip, pFont, 20, 330, "§Z10This item cannot be sold", col);
		}
	}

	CPnt pos;
	pos.x = m_lButtons[m_XPos][m_YPos].x;
	pos.y = m_lButtons[m_XPos][m_YPos].y;
	_pRCUtil->DrawTexture(Clip, pos, "P6_inv_indicat02", CPixel32(0xffffffff), CVec2Dfp32(5.0f, 5.0f));

	pVBM->ScopeEnd();
}

