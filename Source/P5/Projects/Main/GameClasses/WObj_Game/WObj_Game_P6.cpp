#include "PCH.h"
#include "WObj_Game_P6.h"
#include "../WObj_Char/WObj_CharPlayer_P6.h"
#include "../WObj_Char/WObj_CharNPC_P6.h"
#include "../WRPG/WRPGChar.h"
#include "../../GameWorld/WClient_P6.h"
#include "../../Exe/WGameContextMain.h"

void AddInvItem(CNetMsg *_Msg, SInventoryItem *_item)
{
	_Msg->AddInt32(_item->x);
	_Msg->AddInt32(_item->y);
	_Msg->AddStr(_item->item.m_LootObject);
	_Msg->AddStr(_item->item.m_LootTemplate);
	_Msg->AddStr(_item->item.m_Info);
	_Msg->AddStr(_item->item.m_GUI_Icon);
	_Msg->AddInt32(_item->item.m_LootType);
	_Msg->AddInt32(_item->item.m_SubType);
}

void AddInvWeaponMod(CNetMsg *_Msg, SLootItem *_item)
{
	_Msg->AddStr(_item->m_LootObject);
	_Msg->AddStr(_item->m_LootTemplate);
	_Msg->AddStr(_item->m_Info);
	_Msg->AddStr(_item->m_GUI_Icon);
	_Msg->AddInt32(_item->m_LootType);
	_Msg->AddInt32(_item->m_SubType);
}

void GetInvItem(const CNetMsg& _Msg, int _p, SInventoryItem *_item)
{
	_item->x = _Msg.GetInt32(_p);
	_item->y = _Msg.GetInt32(_p);
	_item->item.m_LootObject = _Msg.GetStr(_p);
	_item->item.m_LootTemplate = _Msg.GetStr(_p);
	_item->item.m_Info = _Msg.GetStr(_p);
	_item->item.m_GUI_Icon = _Msg.GetStr(_p);
	_item->item.m_LootType = _Msg.GetInt32(_p);
	_item->item.m_SubType = _Msg.GetInt32(_p);
}

void GetInvWeaponMod(const CNetMsg& _Msg, int _p, SLootItem *_item)
{
	_item->m_LootObject = _Msg.GetStr(_p);
	_item->m_LootTemplate = _Msg.GetStr(_p);
	_item->m_Info = _Msg.GetStr(_p);
	_item->m_GUI_Icon = _Msg.GetStr(_p);
	_item->m_LootType = _Msg.GetInt32(_p);
	_item->m_SubType = _Msg.GetInt32(_p);
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_GameP6, CWObject_GameCampaign, 0x0100);

CWObject_GameP6::CWObject_GameP6()
{
	m_bDoOnce = true;
}

void CWObject_GameP6::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	CWObject_GameCampaign::OnIncludeClass(_pWData, _pWServer);
	_pWData->GetResourceIndex_Class("Ai_character_Badguy1");
	_pWData->GetResourceIndex_Class("Ai_character_Badguy2");
	_pWData->GetResourceIndex_Class("Ai_character_Badguy3");
}

int CWObject_GameP6::OnCharacterKilled(int _iObject, int _iSender)
{
	CWObject_Character *pObj = (CWObject_Character *)m_pWServer->Object_Get(_iObject);
	CWObject_CharPlayer_P6 *pChar = TDynamicCast<CWObject_CharPlayer_P6>(pObj);
	if(pChar)
	{
		pChar->m_DropItemTick = -1;
		pChar->GetClientData().m_DeathTick = -1;

		pChar->ClientFlags() &= ~(PLAYER_CLIENTFLAGS_NOLOOK | PLAYER_CLIENTFLAGS_NOMOVE);
		pChar->Char_SetPhysics(pObj, m_pWServer, m_pWServer, PLAYER_PHYS_STAND);
		pChar->Char()->ReceiveHealth(pChar->Char()->MaxHealth());

		CWO_CharPlayer_ClientData_P6 &pCDChar = pChar->GetClientData(pChar);
		CWAG2I_Context Context(pObj,m_pWServer,pCDChar.m_GameTime);
		pCDChar.m_AnimGraph2.GetAG2I()->RestartAG(&Context, 0);

		TSelection<CSelection::SMALL_BUFFER> Selection;
		const int16* pSel;
		int nSel = 0;
		m_pWServer->Selection_AddClass(Selection, "info_player_start");
		nSel = m_pWServer->Selection_Get(Selection, &pSel);

		if(nSel > 0)
		{
			CWObject *pObjSpawn = m_pWServer->Object_Get(pSel[0]);
			m_pWServer->Object_SetPosition(pChar->m_iObject, pObjSpawn->GetPositionMatrix());
		}
	}
	else
	{
		CWObject_GameCampaign::OnCharacterKilled(_iObject, _iSender);
	}

	return 1;
}

int CWObject_GameP6::OnClientCommand(int _iPlayer, const CCmd* _pCmd)
{
	int Size = _pCmd->m_Size;
	switch(_pCmd->m_Cmd)
	{
	case CONTROL_EQUIP_WEAPON:
		{
			char cmd[256];
			memcpy(cmd, _pCmd->m_Data, Size);
			cmd[Size] = 0;
			CStr command = cmd;
			int iInv = command.GetStrSep(":").Val_int();
			int iObj = command.Val_int();

			m_pWServer->Message_SendToObject(CWObject_Message(P6_OBJMSG_CHAR_EQUIP_WEAPON, iInv), iObj);
			return true;
		}
	case CONTROL_UNEQUIP_WEAPON:
		{
			char cmd[256];
			memcpy(cmd, _pCmd->m_Data, Size);
			cmd[Size] = 0;
			CStr command = cmd;
			int iObj = command.Val_int();

			m_pWServer->Message_SendToObject(CWObject_Message(P6_OBJMSG_CHAR_UNEQUIP_WEAPON), iObj);
			return true;
		}
	case CONTROL_INV_SWITCH2:
		{
			char cmd[256];
			memcpy(cmd, _pCmd->m_Data, Size);
			cmd[Size] = 0;
			CStr command = cmd;
			int32 iObj = command.GetStrSep(":").Val_int();
			int32 iItem1 = command.GetStrSep(":").Val_int();
			int32 iItem2 = command.Val_int();

			CWObject *pObj = m_pWServer->Object_Get(iObj);
			CWObject_CharPlayer_P6 *pChar = safe_cast<CWObject_CharPlayer_P6>(pObj);
			CWO_CharPlayer_ClientData_P6 &CD = pChar->GetClientData(pChar);

			int ox = CD.m_lInventory[iItem1].x;
			int oy = CD.m_lInventory[iItem1].y;
			CD.m_lInventory[iItem1].x = CD.m_lInventory[iItem2].x;
			CD.m_lInventory[iItem1].y = CD.m_lInventory[iItem2].y;
			CD.m_lInventory[iItem2].x = ox;
			CD.m_lInventory[iItem2].y = oy;

			CNetMsg Msg;
			Msg.m_MsgType = NETMSG_GAME_INV_MOD;
			Msg.AddInt32(iObj);	//Player
			Msg.AddInt32(iItem1);	//Inventory index
			//The item
			SInventoryItem item = CD.m_lInventory[iItem1];
			AddInvItem(&Msg, &item);
			m_pWServer->NetMsg_SendToObject(Msg, m_iObject);
			if(item.item.m_LootType == INV_WEAPON)
			{
				for(int y = 0; y < 2; y++)
				{
					for(int x = 0; x < 3; x++)
					{
						CNetMsg MsgWeaponMod;
						MsgWeaponMod.m_MsgType = NETMSG_GAME_INV_WEAPON_MOD;
						MsgWeaponMod.AddInt32(iObj);	//Player
						MsgWeaponMod.AddInt32(iItem1);	//Inventory index
						MsgWeaponMod.AddInt32(x);
						MsgWeaponMod.AddInt32(y);
						//The item
						SLootItem loot_item = CD.m_lInventory[iItem1].inv[x][y];
						AddInvWeaponMod(&MsgWeaponMod, &loot_item);
						m_pWServer->NetMsg_SendToObject(MsgWeaponMod, m_iObject);
					}
				}
			}

			CNetMsg Msg2;
			Msg2.m_MsgType = NETMSG_GAME_INV_MOD;
			Msg2.AddInt32(iObj);	//Player
			Msg2.AddInt32(iItem2);	//Inventory index
			//The item
			item = CD.m_lInventory[iItem2];
			AddInvItem(&Msg2, &item);
			m_pWServer->NetMsg_SendToObject(Msg2, m_iObject);
			if(item.item.m_LootType == INV_WEAPON)
			{
				for(int y = 0; y < 2; y++)
				{
					for(int x = 0; x < 3; x++)
					{
						CNetMsg MsgWeaponMod;
						MsgWeaponMod.m_MsgType = NETMSG_GAME_INV_WEAPON_MOD;
						MsgWeaponMod.AddInt32(iObj);	//Player
						MsgWeaponMod.AddInt32(iItem2);	//Inventory index
						MsgWeaponMod.AddInt32(x);
						MsgWeaponMod.AddInt32(y);
						//The item
						SLootItem loot_item = CD.m_lInventory[iItem2].inv[x][y];
						AddInvWeaponMod(&MsgWeaponMod, &loot_item);
						m_pWServer->NetMsg_SendToObject(MsgWeaponMod, m_iObject);
					}
				}
			}

			return true;
		}

	case CONTROL_INV_SWITCH:
		{
			char cmd[256];
			memcpy(cmd, _pCmd->m_Data, Size);
			cmd[Size] = 0;
			CStr command = cmd;
			int32 iObj = command.GetStrSep(":").Val_int();
			int32 iItem = command.GetStrSep(":").Val_int();
			int32 x = command.GetStrSep(":").Val_int();
			int32 y = command.Val_int();

			CWObject *pObj = m_pWServer->Object_Get(iObj);
			CWObject_CharPlayer_P6 *pChar = safe_cast<CWObject_CharPlayer_P6>(pObj);
			CWO_CharPlayer_ClientData_P6 &CD = pChar->GetClientData(pChar);

			CD.m_lInventory[iItem].x = x;
			CD.m_lInventory[iItem].y = y;

			CNetMsg Msg;
			Msg.m_MsgType = NETMSG_GAME_INV_MOD;
			Msg.AddInt32(iObj);	//Player
			Msg.AddInt32(iItem);	//Inventory index
			//The item
			SInventoryItem item = CD.m_lInventory[iItem];
			AddInvItem(&Msg, &item);
			m_pWServer->NetMsg_SendToObject(Msg, m_iObject);
			if(item.item.m_LootType == INV_WEAPON)
			{
				for(int y = 0; y < 2; y++)
				{
					for(int x = 0; x < 3; x++)
					{
						CNetMsg MsgWeaponMod;
						MsgWeaponMod.m_MsgType = NETMSG_GAME_INV_WEAPON_MOD;
						MsgWeaponMod.AddInt32(iObj);	//Player
						MsgWeaponMod.AddInt32(iItem);	//Inventory index
						MsgWeaponMod.AddInt32(x);
						MsgWeaponMod.AddInt32(y);
						//The item
						SLootItem loot_item = CD.m_lInventory[iItem].inv[x][y];
						AddInvWeaponMod(&MsgWeaponMod, &loot_item);
						m_pWServer->NetMsg_SendToObject(MsgWeaponMod, m_iObject);
					}
				}
			}

			return true;
		}

	case CONTROL_INV_MOD:
		{
			char cmd[256];
			memcpy(cmd, _pCmd->m_Data, Size);
			cmd[Size] = 0;
			CStr command = cmd;
			int32 iObj = command.GetStrSep(":").Val_int();
			int32 iWeapon = command.GetStrSep(":").Val_int();
			int32 iMod = command.GetStrSep(":").Val_int();
			int32 x = command.GetStrSep(":").Val_int();
			int32 y = command.Val_int();

			CWObject *pObj = m_pWServer->Object_Get(iObj);
			CWObject_CharPlayer_P6 *pChar = safe_cast<CWObject_CharPlayer_P6>(pObj);
			CWO_CharPlayer_ClientData_P6 &CD = pChar->GetClientData(pChar);

			CD.m_lInventory[iWeapon].inv[x][y] = CD.m_lInventory[iMod].item;
			CD.m_lInventory[iMod].item.m_GUI_Icon = "";
			CD.m_lInventory[iMod].item.m_Info = "";
			CD.m_lInventory[iMod].item.m_LootObject = "";
			CD.m_lInventory[iMod].item.m_LootTemplate = "";
			CD.m_lInventory[iMod].item.m_LootType = -1;
			CD.m_lInventory[iMod].item.m_SubType = -1;
			CD.m_lInventory[iMod].x = -1;
			CD.m_lInventory[iMod].y = -1;

			CNetMsg Msg;
			Msg.m_MsgType = NETMSG_GAME_INV_MOD;
			Msg.AddInt32(iObj);	//Player
			Msg.AddInt32(iWeapon);	//Inventory index
			//The item
			SInventoryItem item = CD.m_lInventory[iWeapon];
			AddInvItem(&Msg, &item);
			m_pWServer->NetMsg_SendToObject(Msg, m_iObject);

			if(item.item.m_LootType == INV_WEAPON)
			{
				for(int y = 0; y < 2; y++)
				{
					for(int x = 0; x < 3; x++)
					{
						CNetMsg MsgWeaponMod;
						MsgWeaponMod.m_MsgType = NETMSG_GAME_INV_WEAPON_MOD;
						MsgWeaponMod.AddInt32(iObj);	//Player
						MsgWeaponMod.AddInt32(iWeapon);	//Inventory index
						MsgWeaponMod.AddInt32(x);
						MsgWeaponMod.AddInt32(y);
						//The item
						SLootItem loot_item = CD.m_lInventory[iWeapon].inv[x][y];
						AddInvWeaponMod(&MsgWeaponMod, &loot_item);
						m_pWServer->NetMsg_SendToObject(MsgWeaponMod, m_iObject);
					}
				}
			}

			CNetMsg Msg2;
			Msg2.m_MsgType = NETMSG_GAME_INV_MOD;
			Msg2.AddInt32(iObj);	//Player
			Msg2.AddInt32(iMod);	//Inventory index
			//The item
			item = CD.m_lInventory[iMod];
			AddInvItem(&Msg2, &item);
			m_pWServer->NetMsg_SendToObject(Msg2, m_iObject);

			return true;
		}

	case CONTROL_INV_UNMOD:
		{
			char cmd[256];
			memcpy(cmd, _pCmd->m_Data, Size);
			cmd[Size] = 0;
			CStr command = cmd;
			int32 iObj = command.GetStrSep(":").Val_int();
			int32 iWeapon = command.GetStrSep(":").Val_int();
			int32 x = command.GetStrSep(":").Val_int();
			int32 y = command.Val_int();

			CWObject *pObj = m_pWServer->Object_Get(iObj);
			CWObject_CharPlayer_P6 *pChar = safe_cast<CWObject_CharPlayer_P6>(pObj);
			CWO_CharPlayer_ClientData_P6 &CD = pChar->GetClientData(pChar);

			CD.m_lInventory[iWeapon].inv[x][y].m_GUI_Icon = "";
			CD.m_lInventory[iWeapon].inv[x][y].m_Info = "";
			CD.m_lInventory[iWeapon].inv[x][y].m_LootObject = "";
			CD.m_lInventory[iWeapon].inv[x][y].m_LootTemplate = "";
			CD.m_lInventory[iWeapon].inv[x][y].m_LootType = -1;
			CD.m_lInventory[iWeapon].inv[x][y].m_SubType = -1;

/*			CNetMsg Msg;
			Msg.m_MsgType = NETMSG_GAME_INV_MOD;
			Msg.AddInt32(iObj);	//Player
			Msg.AddInt32(iWeapon);	//Inventory index
			//The item
			SInventoryItem item = CD.m_lInventory[iWeapon];
			AddInvItem(&Msg, &item);
			m_pWServer->NetMsg_SendToObject(Msg, m_iObject);*/

			CNetMsg MsgWeaponMod;
			MsgWeaponMod.m_MsgType = NETMSG_GAME_INV_WEAPON_MOD;
			MsgWeaponMod.AddInt32(iObj);	//Player
			MsgWeaponMod.AddInt32(iWeapon);	//Inventory index
			MsgWeaponMod.AddInt32(x);
			MsgWeaponMod.AddInt32(y);
			//The item
			SLootItem loot_item = CD.m_lInventory[iWeapon].inv[x][y];
			AddInvWeaponMod(&MsgWeaponMod, &loot_item);
			m_pWServer->NetMsg_SendToObject(MsgWeaponMod, m_iObject);

			return true;
		}

	case CONTROL_INV_FORCE_UPDATE:
		{
			char cmd[256];
			memcpy(cmd, _pCmd->m_Data, Size);
			cmd[Size] = 0;
			CStr command = cmd;
			int32 iObj = command.Val_int();

			CWObject *pObj = m_pWServer->Object_Get(iObj);
			CWObject_CharPlayer_P6 *pChar = safe_cast<CWObject_CharPlayer_P6>(pObj);
			CWO_CharPlayer_ClientData_P6 &CD = pChar->GetClientData(pChar);

			for(int i = 0; i < CD.m_lInventory.Len(); i++)
			{
				CStr Obj = CStrF("%s:%s:%s:%s:%i:%i", 
					CD.m_lInventory[i].item.m_LootObject.Str(), CD.m_lInventory[i].item.m_LootTemplate.Str(), 
					CD.m_lInventory[i].item.m_Info.Str(), CD.m_lInventory[i].item.m_GUI_Icon.Str(), 
					CD.m_lInventory[i].item.m_LootType, CD.m_lInventory[i].item.m_SubType);

				CNetMsg Msg;
				Msg.m_MsgType = NETMSG_GAME_INV_ADD;
				Msg.AddInt32(iObj);	//Player
				Msg.AddStr(Obj);	//Inventory index
				m_pWServer->NetMsg_SendToObject(Msg, m_iObject);
			}

			TSelection<CSelection::SMALL_BUFFER> Selection;
			const int16* pSel;
			int nSel = 0;
			m_pWServer->Selection_AddClass(Selection, "CharNPC_P6");
			nSel = m_pWServer->Selection_Get(Selection, &pSel);

			for(int i = 0; i < nSel; i++)
			{
				CWObject *pObj = m_pWServer->Object_Get(pSel[i]);
				CWObject_CharNPC_P6 *pChar = safe_cast<CWObject_CharNPC_P6>(pObj);
				CWO_CharNPC_ClientData_P6 &CD = pChar->GetClientData(pChar);
				CD.m_lLoot.MakeDirty();
				CD.m_Money.MakeDirty();
			}

			return true;
		}

	case CONTROL_LOOT_MONEY:
		{
			char cmd[256];
			memcpy(cmd, _pCmd->m_Data, Size);
			cmd[Size] = 0;
			CStr command = cmd;
			int32 iObj = command.GetStrSep(":").Val_int();

			CWObject *pObj = m_pWServer->Object_Get(iObj);
			CWObject_CharPlayer_P6 *pChar = safe_cast<CWObject_CharPlayer_P6>(pObj);
			CWO_CharPlayer_ClientData_P6 &CD = pChar->GetClientData(pChar);

			CWObject *pLootObj = m_pWServer->Object_Get(CD.m_iLootTarget);
			CWObject_CharNPC_P6 *pLootChar = safe_cast<CWObject_CharNPC_P6>(pLootObj);
			CWO_CharNPC_ClientData_P6 &LootCD = pLootChar->GetClientData(pLootChar);

			CD.m_Money.m_Value += LootCD.m_Money;
			LootCD.m_Money = 0;
			LootCD.m_Money.MakeDirty();
			CD.m_Money.MakeDirty();

			return true;
		}

	case CONTROL_LOOT_OBJECT:
		{
			char cmd[256];
			memcpy(cmd, _pCmd->m_Data, Size);
			cmd[Size] = 0;
			CStr command = cmd;
			int32 iObj = command.GetStrSep(":").Val_int();
			int32 iLootIndex = command.Val_int();

			CWObject *pObj = m_pWServer->Object_Get(iObj);
			CWObject_CharPlayer_P6 *pChar = safe_cast<CWObject_CharPlayer_P6>(pObj);
			CWO_CharPlayer_ClientData_P6 &CD = pChar->GetClientData(pChar);

			CWObject *pLootObj = m_pWServer->Object_Get(CD.m_iLootTarget);
			CWObject_CharNPC_P6 *pLootChar = safe_cast<CWObject_CharNPC_P6>(pLootObj);
			CWO_CharNPC_ClientData_P6 &LootCD = pLootChar->GetClientData(pLootChar);

			CStr Obj = CStrF("%s:%s:%s:%s:%i:%i", 
				LootCD.m_lLoot[iLootIndex].m_LootObject.Str(), LootCD.m_lLoot[iLootIndex].m_LootTemplate.Str(), 
				LootCD.m_lLoot[iLootIndex].m_Info.Str(), LootCD.m_lLoot[iLootIndex].m_GUI_Icon.Str(), 
				LootCD.m_lLoot[iLootIndex].m_LootType, LootCD.m_lLoot[iLootIndex].m_SubType);

			CD.InventoryAddItem(Obj);

			LootCD.m_lLoot[iLootIndex].m_LootObject = "";
			LootCD.m_lLoot[iLootIndex].m_LootTemplate = "";
			LootCD.m_lLoot[iLootIndex].m_Info = "";
			LootCD.m_lLoot[iLootIndex].m_GUI_Icon = "";
			LootCD.m_lLoot[iLootIndex].m_LootType = -1;
			
			LootCD.m_lLoot.MakeDirty();
			//CD.m_lInventory.MakeDirty();
			CNetMsg Msg;
			Msg.m_MsgType = NETMSG_GAME_INV_ADD;
			Msg.AddInt32(iObj);	//Player
			Msg.AddStr(Obj);	//Inventory index
			m_pWServer->NetMsg_SendToObject(Msg, m_iObject);

			return true;
		}

	case CONTROL_LOOT_EXIT:
		{
			char cmd[256];
			memcpy(cmd, _pCmd->m_Data, Size);
			cmd[Size] = 0;
			CStr command = cmd;
			int32 iObj = command.Val_int();
			int32 iObjClient = 1;

			MACRO_GetRegisterObject(CGameContext, pGameContext, "GAMECONTEXT");
			for(int i = 0; i < pGameContext->m_lspWClients.Len(); i++)
			{
				if(pGameContext->m_lspWClients[i]->Player_GetLocalObject() == iObj)
				{
					iObjClient = i;
					break;
				}
			}

			CRegistry* pReg = m_pWServer->Registry_GetClient(iObjClient, WCLIENT_REG_GAMESTATE);
			if(pReg)
				pReg->SetValue("WINDOW", "");

			return true;
		}

	default:
		return CWObject_GameCampaign::OnClientCommand(_iPlayer, _pCmd);
	}
	return 1;
}

void CWObject_GameP6::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	switch(_Msg.m_MsgType)
	{
	case NETMSG_GAME_INV_ADD:
		{
			int p = 0;
			int32 iPlayer = _Msg.GetInt32(p);
			if(_pWClient->Player_GetLocalObject() == iPlayer)
			{
				SInventoryItem item;
				CStr Obj = _Msg.GetStr(p);
				
				CWObject_CoreData *pCoreData = _pWClient->Object_GetFirstCopy(_pWClient->Player_GetLocalObject());
				CWO_CharPlayer_ClientData_P6 *pCD = safe_cast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pCoreData->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
				pCD->InventoryAddItem(Obj);
			}
		}
		return;
	case NETMSG_GAME_INV_WEAPON_MOD:
		{
			int p = 0;
			int32 iPlayer = _Msg.GetInt32(p);
			int32 iInv = _Msg.GetInt32(p);
			if(_pWClient->Player_GetLocalObject() == iPlayer)
			{
				int32 x = _Msg.GetInt32(p);
				int32 y = _Msg.GetInt32(p);
				SLootItem item;
				GetInvWeaponMod(_Msg, p, &item);

				CWObject_CoreData *pCoreData = _pWClient->Object_GetFirstCopy(_pWClient->Player_GetLocalObject());
				CWO_CharPlayer_ClientData_P6 *pCD = safe_cast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pCoreData->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
				pCD->m_lInventory[iInv].inv[x][y] = item;
			}
		}
		return;
	case NETMSG_GAME_INV_MOD:
		{
			int p = 0;
			int32 iPlayer = _Msg.GetInt32(p);
			int32 iInv = _Msg.GetInt32(p);
			if(_pWClient->Player_GetLocalObject() == iPlayer)
			{
				SInventoryItem item;
				GetInvItem(_Msg, p, &item);

				CWObject_CoreData *pCoreData = _pWClient->Object_GetFirstCopy(_pWClient->Player_GetLocalObject());
				CWO_CharPlayer_ClientData_P6 *pCD = safe_cast<CWO_CharPlayer_ClientData_P6>((CReferenceCount*)pCoreData->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
				pCD->m_lInventory[iInv] = item;
			}
		}
		return;
	}
	return CWObject_GameCampaign::OnClientNetMsg(_pObj, _pWClient, _Msg);
}

void CWObject_GameP6::OnRefresh()
{
	CWObject_GameCampaign::OnRefresh();
	if(m_bDoOnce)
	{
#if defined(PLATFORM_CONSOLE)
		MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
		CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

		pGameMod->m_pMPHandler->LoadingDone();
#endif

		m_bDoOnce = false;		
	}
}
aint CWObject_GameP6::OnMessage(const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJMSG_GAME_GETIS_P6:
		return 1;
	}

	return CWObject_GameCampaign::OnMessage(_Msg);
}

aint CWObject_GameP6::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_GAME_RENDERSTATUSBAR:
		OnClientRenderStatusBar(_pObj, _pWClient, (CXR_Engine *)_Msg.m_Param0, (CRC_Util2D *)_Msg.m_Param1, (CVec2Dfp32 *)_Msg.m_pData);
		return 1;
	}

	return CWObject_GameCampaign::OnClientMessage(_pObj, _pWClient, _Msg);
}

void CWObject_GameP6::DrawInfoRect(CRC_Util2D *_pUtil2D, CWorld_Client *_pWClient, const CRct &_Rct, const wchar *_pTitle, CRC_Font *_pTitleFont, const wchar *_pText, CRC_Font *_pFont, int _iSurfRes, int _SurfaceWidth, int _SurfaceHeight, int _TitleExtraHeight, int _TextCol, int _Color1, int _Color2)
{
	int XPos = _Rct.p0.x;
	int YPos = _Rct.p0.y;
	int Width = _Rct.GetWidth();
	int Height = _Rct.GetHeight();
	int TitleHeight = 2;
	if(_pTitleFont && _pTitle)
		TitleHeight = int(_pTitleFont->GetHeight(_pTitleFont->GetOriginalSize(), _pTitle));
	TitleHeight += _TitleExtraHeight;

	int32 Color = (_Color1 ? CPixel32(_Color1) : CPixel32::From_fp32((80/2),(100/2),(115/2),0.65f * 255.0f));
	int32 Color2 = (_Color2 ? CPixel32(_Color2) : CPixel32::From_fp32((80/2),(100/2),(115/2),0.86f * 255.0f));

	_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	_pUtil2D->SetTexture(0);
	CClipRect Clip(0, 0, _Rct.p1.x, _Rct.p1.y);
/*	if (Height > 0)
	{
		_pUtil2D->Rect(Clip, CRct(XPos + 2, YPos, XPos + Width - 2, YPos + Height - 2), Color);
		_pUtil2D->Rect(Clip, CRct(XPos, YPos, XPos + 2, YPos + Height - 2), Color2);
		_pUtil2D->Rect(Clip, CRct(XPos + Width - 2, YPos, XPos + Width , YPos + Height - 2), Color2);
		_pUtil2D->Rect(Clip, CRct(XPos, YPos + Height - 2, XPos + Width, YPos + Height), Color2);
	}
	_pUtil2D->Rect(Clip, CRct(XPos, YPos - TitleHeight - 8, XPos + Width - 35, YPos), Color2);
	_pUtil2D->Rect(Clip, CRct(XPos + Width - 35, YPos - TitleHeight, XPos + Width, YPos), Color2);*/
	CXR_VBManager* pVBM = _pUtil2D->GetVBM();
	CRC_Viewport *pVP = pVBM->Viewport_Get();
	CRct rct = pVP->GetViewRect();
	CClipRect NoClip(0, 0, rct.p1.x, rct.p1.y);
	_pUtil2D->DrawTexture(NoClip, CPnt(XPos - 25, YPos - 40), "P6_inforuta", 0xffffffff, CVec2Dfp32(1.7f, 2.0f));

	_pUtil2D->SetTexture("GUI_TRIANGLE");
	CPnt Pnt(XPos + Width - 35, YPos - TitleHeight - 8);
	_pUtil2D->SetTextureOrigo(Clip, Pnt);
//	_pUtil2D->Rect(Clip, CRct(Pnt.x, Pnt.y, XPos + Width - 27, YPos - TitleHeight), Color2);

	int32 TextCol = (_TextCol ? CPixel32(_TextCol) : CPixel32::From_fp32((226/1.7),(224/1.7),(220/1.7),255));
	//int32 ShadowCol = CPixel32((226/8),(224/8),(220/8),255);
	if(_pFont && _pText)
		_pUtil2D->Text_DrawFormatted(Clip, _pFont, _pText, XPos + 4 + _SurfaceWidth, YPos + 4, WSTYLE_TEXT_WORDWRAP, TextCol, 0, 0, Width - 8 - _SurfaceWidth, Height - 8, false);
	if(_pTitleFont && _pTitle)
		_pUtil2D->Text_DrawFormatted(Clip, _pTitleFont, _pTitle, XPos + 4, YPos - (TitleHeight + 2 - _TitleExtraHeight), WSTYLE_TEXT_WORDWRAP, TextCol, 0, 0, Width - 8, TitleHeight + 6, false);

	//_pUtil2D->Flush();	
	if(_iSurfRes != 0)
	{
		int iSurf = _pWClient->GetMapData()->GetResource_SurfaceID(_iSurfRes);
		_pUtil2D->SetSurface(iSurf, CMTime::CreateFromSeconds(0));
		_pUtil2D->SetTextureOrigo(Clip, CPnt(XPos, YPos));
		_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / _SurfaceWidth, fp32(_pUtil2D->GetTextureHeight()) / _SurfaceHeight);
//		_pUtil2D->Rect(Clip, CRct(XPos, YPos, XPos + _SurfaceWidth, YPos + _SurfaceHeight), 0xff808080);
		Width += _SurfaceWidth;
	}
}

extern int CompareSubtitleType(int _Type1, int _Type2);

void CWObject_GameP6::OnClientRenderStatusBar(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D, CVec2Dfp32 *_rect)
{
	MSCOPESHORT(CWObject_GameP6::OnClientRenderStatusBar);

	CClientData *pCD = GetClientData(_pObj);
	int Tick = _pWClient->GetGameTick();

	CRC_Viewport VP;
	_pWClient->Render_GetLastRenderViewport(VP);
	CClipRect Clip(VP.GetViewClip());
	if(pCD->m_GameMessage_Text.m_Value != "")
	{
		// Game message
		if(pCD->m_GameMessage_Text.m_Value[0] == '$')
		{
			int Alpha = 0xff000000 - (GetClientData(_pObj)->GetCurScreenFade(_pWClient->GetGameTick(), _pWClient->GetRenderTickFrac()) & 0xff000000);

			CStr Reg = pCD->m_GameMessage_Text.m_Value.Copy(1, 1024);
			int iReg = _pWClient->GetMapData()->GetResourceIndex_Registry(Reg);
			CRegistry *pReg = _pWClient->GetMapData()->GetResource_Registry(iReg);
			if(pReg && pReg->GetNumChildren() == 1)
			{
				pReg = pReg->GetChild(0);

				CFStr St = pReg->GetThisValue();
				fp32 Speed = St.GetStrSep(",").Val_fp64() * _pWClient->GetGameTickTime();
				int LineSize = St.GetStrSep(",").Val_int();
				fp32 TimeBeforeFade = St.GetStrSep(",").Val_fp64() * _pWClient->GetGameTicksPerSecond();
				fp32 FadeDuration = St.GetStrSep(",").Val_fp64() * _pWClient->GetGameTicksPerSecond();
				fp32 Duration = Tick + _pWClient->GetRenderTickFrac() - pCD->m_GameMessage_StartTick;

				_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
				if(Duration > TimeBeforeFade)
				{
					int Alpha = 255;
					if(Duration - TimeBeforeFade < FadeDuration)
						Alpha = int(255 * (Duration - TimeBeforeFade) / FadeDuration);
					_pUtil2D->SetTexture(0);
					_pUtil2D->Rect(Clip, Clip.clip, Alpha << 24);
				}

				fp32 ScrolledPixels = Duration * Speed;
				fp32 LowestLine = ScrolledPixels / LineSize;
				int iLine = TruncToInt(LowestLine);
				fp32 Frac = LowestLine - iLine;

				int Pos = int(480 - Frac * LineSize);
				while(Pos > -LineSize && iLine >= 0)
				{
					if(iLine < pReg->GetNumChildren())
					{
						int32 TextCol = 0x00808080 | Alpha;
						int32 ShadowCol = 0x00000000 | Alpha;
						CFStr Name = pReg->GetName(iLine);
						if(Name != "$SPACE")
						{
							CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font(Name.GetStrSep(",")));
							CStr St = CStrF("§Z%i%s", Name.GetStrSep(",").Val_int(), pReg->GetValue(iLine).Str());
							if(pFont)
								_pUtil2D->Text_DrawFormatted(Clip, pFont, St, Name.GetStrSep(",").Val_int(), Pos, WSTYLE_TEXT_SHADOW, TextCol, ShadowCol, 0, 640, LineSize, false);
						}
					}

					Pos -= LineSize;
					iLine--;
				}
			}
		}
		else
		{
			const fp32 EnterTicks = 0.3f * _pWClient->GetGameTicksPerSecond();
			int MsgDuration = pCD->m_GameMessage_Duration;
			if(MsgDuration == 0)
				MsgDuration = int(EnterTicks * 2 + 5);
			int StopTick = pCD->m_GameMessage_StartTick + MsgDuration;
			if(StopTick != 0 && StopTick > Tick)
			{
				CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("P6_text"));
				CRC_Font *pTitleFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("P6_text"));
				if(pFont && pTitleFont)
				{
					fp32 Duration = Tick + _pWClient->GetRenderTickFrac() - pCD->m_GameMessage_StartTick;

					CFStr Text = pCD->m_GameMessage_Text;
					CFStr Title;
					if(Text.Find("|") != -1)
						Title = Text.GetStrSep("|");
					else
						Title = "§LGAMEMSG_INFO";

					int iSurfRes = 0;
					int SurfW = 0;
					int SurfH = 0;
					if(Text.CompareSubStr("$") == 0)
					{
						iSurfRes = Text.GetStrSep(",").Copy(1, 1024).Val_int();
						SurfW = Text.GetStrSep(",").Val_int();
						SurfH = Text.GetStrSep(",").Val_int();
					}
					Text = "§Z18" + Text;

					wchar Buffer[1024];
					Localize_Str(Text.Str(), Buffer, 1024);

					if(Buffer[4] == '#')
					{
						CStr St = Buffer + 5;
						while(St != "")
						{
							int X = St.GetStrSep(",").Val_int();
							int Y = St.GetStrSep(",").Val_int();
							//pFont->GetOriginalSize()

							int Alpha;
							if(Duration < EnterTicks)
								Alpha = int(255 * Duration / EnterTicks);
							else if(Duration < MsgDuration - EnterTicks)
								Alpha = 255;
							else
								Alpha = int(255 * (MsgDuration - Duration) / EnterTicks);
							if(St[0] == '$')
							{
								St = St.Copy(1, 1024);
								iSurfRes = _pWClient->GetMapData()->GetResourceIndex_Surface(St.GetStrSep(",").Ansi());
								int iSurf = _pWClient->GetMapData()->GetResource_SurfaceID(iSurfRes);
								_pUtil2D->SetSurface(iSurf, _pWClient->GetRenderTime());
								int W = St.GetStrSep(",").Val_int();
								int H = St.GetStrSep(",").Val_int();
								if(W > 10000)
								{
									_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / 640, fp32(_pUtil2D->GetTextureHeight()) / 480);
									W -= 10000;
								}
								else
								{
									_pUtil2D->SetTextureOrigo(Clip, CPnt(X, Y));
									_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / W, fp32(_pUtil2D->GetTextureHeight()) / H);
								}
								_pUtil2D->Rect(Clip, CRct(X, Y, X + W, Y + H), 0x00ffffff | (Alpha << 24));
							}
							else
							{
								CRC_Font *pFont2 = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("PALATINO"));
								if(pFont2)
									pFont = pFont2;
								CStr St2 = "§Z24" + St;
								int32 TextCol = 0x00808080 | (Alpha << 24);
								int32 ShadowCol = 0x00000000 | (Alpha << 24);
								_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
								_pUtil2D->Text_DrawFormatted(Clip, pFont, St2, X, Y, WSTYLE_TEXT_WORDWRAP | WSTYLE_TEXT_SHADOW, TextCol, ShadowCol, 0, 640 - X, 480 - Y, false);
							}
						}
					}
					else
					{
						SurfW = int((SurfW / _pUtil2D->GetRC()->GetDC()->GetPixelAspect()));

						wchar BufTitle[1024];
						Localize_Str(Title.Str(), BufTitle, 1024);
						CRct Rct = CalcInfoRectWidth(_pUtil2D, BufTitle, pTitleFont, Buffer, pFont, iSurfRes, SurfW, SurfH);
						int XPos;
						int XBasePos = 640 / 2 - ((Rct.GetWidth() / 2));
						if(1.4f < ((fp32)Clip.clip.p1.x / (fp32)Clip.clip.p1.y))
						{
							XBasePos += (Clip.clip.p1.x - 640) / 2;
							_pUtil2D->SetCoordinateScale(CVec2Dfp32(1.0f, 1.0f));
						}
						if(Duration < EnterTicks)
							XPos = 640 - RoundToInt((640 - XBasePos) * (1.0f - Sqr(1.0f - Duration / EnterTicks)));
						else if(Duration < MsgDuration - EnterTicks)
							XPos = XBasePos;
						else
							XPos = XBasePos - RoundToInt((Rct.GetWidth() + XBasePos) * Sqr(1.0f - ((MsgDuration - Duration) / EnterTicks)));
						int YPos = 150;

						Rct.p0.x += XPos;
						Rct.p1.x += XPos;
						Rct.p0.y += YPos;
						Rct.p1.y += YPos;

						DrawInfoRect(_pUtil2D, _pWClient, Rct, BufTitle, pTitleFont, Buffer, pFont, iSurfRes, SurfW, SurfH, 0, 0, 0xb0000000, 0xff000000);
					}
				}
			}	
		}
	}

	int iPlayer = _pWClient->Player_GetLocalObject();
	CWObject_Client *pPlayerObj = _pWClient->Object_Get(iPlayer);
	if(_pObj->m_ClientFlags & GAME_CLIENTFLAGS_PENDINGGAMEMSG && pPlayerObj && CWObject_Character::Char_IsPlayerView(pPlayerObj))
	{
		CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("P6_text"));
		if(pFont)
		{
			wchar Buf[1024];
			Localize_Str("§z09§LGAMEMSG_HELP", Buf, 1024);
			int Width = int(pFont->GetWidth(pFont->GetOriginalSize(), Buf));
			int32 TextCol = CPixel32::From_fp32((226/1.7f),(224/1.7f),(220/1.7f),255);
			_pUtil2D->Text_DrawFormatted(Clip, pFont, Buf, 555 - Width, 87, 0, TextCol, 0, 0, Width, 40, false);
		}

		if((_pWClient->GetGameTick() & 0xf) > 7)
		{
			_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
#ifdef PLATFORM_XBOX
			_pUtil2D->DrawTexture(Clip, CPnt(560, 80), "GUI_Button_B_32");
#elif !defined(PLATFORM_CONSOLE)
			_pUtil2D->DrawTexture(Clip, CPnt(560, 80), "GUI_Button_F1_64", 0xff808080, CVec2Dfp32(2.0f, 2.0f));
#endif
		}
	}

	if(pCD->m_GameSurface_Surface != 0)
	{
		fp32 HintDur = fp32(_pWClient->GetGameTick() - pCD->m_GameSurface_StartTick) + _pWClient->GetRenderTickFrac();
		bool bOn = _pWClient->GetGameTick() % 25 > 12;
		if(bOn && (pCD->m_GameSurface_Duration == 0 || HintDur < pCD->m_GameSurface_Duration * _pWClient->GetGameTicksPerSecond()))
		{
			CXW_Surface *pSurface = _pWClient->GetMapData()->GetResource_Surface(pCD->m_GameSurface_Surface);
			if (pSurface)
			{
				fp32 Time = 0;
#ifdef XW_SURF_THIN
				int iBindingSt = pSurface->m_Keys.GetKeyIndex("BINDINGID");
				if(iBindingSt != -1)
				{
					CStr Action = pSurface->m_Keys.GetKeyValue(iBindingSt);
#else
				int iBindingSt = pSurface->m_spKeys->GetKeyIndex("BINDINGID");
				if(iBindingSt != -1)
				{
					CStr Action = pSurface->m_spKeys->GetKeyValue(iBindingSt);
#endif
					MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
					CRegistry *pOptions = pSys ? pSys->GetOptions() : 0;
					if(pOptions)
					{
						Time = pOptions->GetValue(Action).Val_int();
						Time += 0.001f;
					}
				}
				_pUtil2D->SetSurface(pSurface, CMTime::CreateFromSeconds(Time));
				fp32 PixelAspect = _pUtil2D->GetRC()->GetDC()->GetPixelAspect();
				CPnt Size = CPnt(RoundToInt(_pUtil2D->GetTextureWidth() * PixelAspect), _pUtil2D->GetTextureHeight());
				CPnt TopLeft(RoundToInt(Clip.clip.p1.x * 0.80f), RoundToInt(Clip.clip.p1.y * 0.15f));
				_pUtil2D->SetTextureScale(1.0f * PixelAspect, 1.0f);
				_pUtil2D->SetTextureOrigo(Clip, TopLeft);
				_pUtil2D->Rect(Clip, CRct(TopLeft.x, TopLeft.y, TopLeft.x + Size.x, TopLeft.y + Size.y), 0xffffffff);
			}
		}
	}

	CMat4Dfp32 Mat;
	_pWClient->Render_GetLastRenderCamera(Mat);

	CDialogueInstance *pNewestItem = NULL;
	int iNewestObject = 0;
	int NewestParam = 0;
	int NewestType = -1;
	const fp32 FadeoutDistance = 64;
	fp32 NewestDistanceSqr = 0;
	fp32 NewestMaxDistanceSqr = 0;

	const CMTime& Time = _pWClient->GetGameTime();

	CWObject_Client *pPlayer = _pWClient->Object_Get(_pWClient->Player_GetLocalObject());
	bool bCutscene = (pPlayer ? pPlayer->m_ClientFlags & PLAYER_CLIENTFLAGS_CUTSCENE : 0) != 0;
	bool bDialogue = (pPlayer ? pPlayer->m_ClientFlags & PLAYER_CLIENTFLAGS_DIALOGUE : 0) != 0;

	//	int nSub = 0;
	for(int i = pCD->m_lCurSubtitles.Len() - 1; i >= 0; i--)
	{
		CClientData::CSubtitleInstance *pSI = &pCD->m_lCurSubtitles[i];
		int Param = 0;
		CDialogueInstance *pItem = (CDialogueInstance *)_pWClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_REQUESTSUBTITLE, aint(&Param)), pSI->m_iObject);
		if(pItem != NULL && pItem->m_Choice != "")
		{
			pNewestItem = pItem;
			iNewestObject = pSI->m_iObject;
			NewestDistanceSqr = 0;
			NewestMaxDistanceSqr = 10000;
			NewestParam = Param;
			NewestType = pItem->m_SubtitleFlags & SUBTITLE_TYPE_MASK;
			break;
		}
		if (pItem && pItem->m_Subtitle != "" && (Time - pItem->m_StartTime).GetTime() <= pItem->m_SampleLength)
		{
			int Type = pItem->m_SubtitleFlags & SUBTITLE_TYPE_MASK;
			if((!bCutscene || Type == SUBTITLE_TYPE_CUTSCENE) &&
				(!bDialogue || Type == SUBTITLE_TYPE_INTERACTIVE))
			{
				CWObject_Client *pObj = _pWClient->Object_Get(pSI->m_iObject);
				if(pObj)
				{
					fp32 DistanceSqr = (pObj->GetPosition() - CVec3Dfp32::GetRow(Mat, 3)).LengthSqr();
					fp32 MaxDistanceSqr = pSI->m_RangeSqr - (FadeoutDistance * FadeoutDistance);
					if(DistanceSqr < MaxDistanceSqr)
					{
						int TypeComp = CompareSubtitleType(NewestType, pItem->m_SubtitleFlags & SUBTITLE_TYPE_MASK);
						if(TypeComp > 0 || (TypeComp == 0 && DistanceSqr < NewestDistanceSqr))
						{
							pNewestItem = pItem;
							iNewestObject = pSI->m_iObject;
							NewestDistanceSqr = DistanceSqr;
							NewestMaxDistanceSqr = MaxDistanceSqr;
							NewestParam = Param;
							NewestType = pItem->m_SubtitleFlags & SUBTITLE_TYPE_MASK;
						}
					}
				}
			}
		}
		else
			pCD->m_lCurSubtitles.Del(i);
	}

	if(pNewestItem)
	{
		fp32 NewestFade = 0;
		fp32 Distance = M_Sqrt(NewestDistanceSqr);
		fp32 MaxDistance = M_Sqrt(NewestMaxDistanceSqr);
		if(MaxDistance - Distance > FadeoutDistance)
			NewestFade = 1.0f;
		else
			NewestFade = (MaxDistance - Distance) / FadeoutDistance;
		NewestFade *= 1.0f - (fp32(GetClientData(_pObj)->GetCurScreenFade(_pWClient->GetGameTick(), _pWClient->GetRenderTickFrac()) >> 24) / 255);
		CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("P6_text"));
		if(pFont)
		{
			CClipRect Clip(0, 0, 640, 480);
			CRct Rect = _pUtil2D->GetVBM()->Viewport_Get()->GetViewRect();

			fp32 aspect = ((fp32)Rect.p1.x / (fp32)Rect.p1.y);
			int ExtraHeight = 5;
			int YStart = 400;
			if(aspect > 1.4f)
			{
				CVec2Dfp32 scale(Rect.GetWidth() / 640.0f, Rect.GetHeight() / 480.0f);
				_pUtil2D->SetFontScale(1.0f + scale.k[0] - scale.k[1], 1.0f + scale.k[0] - scale.k[1]);
				ExtraHeight = 10;
				YStart = 390;
			}

			int Alpha = RoundToInt(NewestFade* 255) << 24;
			int Col = pNewestItem->m_SubtitleColor | Alpha;
			int ShadowCol = Alpha;
			int Border = 150;
			int XStart = Border / 2;
			Clip.clip.p0.x += Border / 2;
			Clip.clip.p1.x -= Border / 2;

			_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

			if(pNewestItem->m_Subtitle != "")
			{
				CStr Subtitle = pNewestItem->m_Subtitle;
				CStr Text = CStrF("§Z%i", pNewestItem->m_SubtitleSize) + Subtitle;
				char moo = '§';	// GCC workaround
				if (pNewestItem->m_Subtitle.Ansi().Str()[0] == moo)
					Text += CStrF("§p0%i§pq", NewestParam);

				_pUtil2D->Text_DrawFormatted(Clip, pFont, Text, XStart, YStart, WSTYLE_TEXT_WORDWRAP, ShadowCol, ShadowCol, ShadowCol, Clip.GetWidth(), Clip.GetHeight(), true, ExtraHeight);
				_pUtil2D->Text_DrawFormatted(Clip, pFont, Text, XStart, YStart, WSTYLE_TEXT_WORDWRAP, Col, Col, ShadowCol, Clip.GetWidth(), Clip.GetHeight(), false, ExtraHeight);
				_pUtil2D->SetFontScale(1.0f, 1.0f);
			}
		}
	}

	if(_rect)
		_pUtil2D->SetCoordinateScale(*_rect);

	// Allow player to render statusbar
	if(iPlayer >= 0)
		_pWClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_RENDERSTATUSBAR, aint(_pEngine), aint(_pUtil2D)), iPlayer);
}

