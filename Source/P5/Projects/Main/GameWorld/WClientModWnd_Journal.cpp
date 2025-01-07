#include "PCH.h"

#include "WFrontEndMod.h"
#include "MWinCtrlMod.h"
#include "WClientModWnd_Journal.h"
#include "../GameClasses/WObj_CharMsg.h"
#include "../GameClasses/WObj_Game/WObj_GameMod.h"
#include "../GameClasses/WObj_CharClientData.h"


MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeIngameScreen, CMWnd_CubeIngameScreen_Parent);


//
//
//
void CMWnd_CubeIngameScreen::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	CWFrontEnd_Mod *pFrontEnd = m_CubeUser.GetFrontEnd();
	if(!pFrontEnd)
		return;

	if(pFrontEnd->m_spDesktop != NULL)
		return;

	if(pFrontEnd->CurrentMenu().Find("reconnect") != -1)
		return;

	pFrontEnd->Cube_Update(this);
	if(pFrontEnd->Cube_ShouldRender())
	{
		// Do mapping
		pFrontEnd->m_Cube.SetSequence(&pFrontEnd->m_Cube.m_Seq_normal);
		if(pFrontEnd->m_Cube.m_CanDoLayout)
			DoMapping(pFrontEnd);
		pFrontEnd->Cube_SetSecondaryRect(m_SecondaryRect);
		pFrontEnd->Cube_SetSecondary(m_iSecondaryID, m_BlendMode);

		pFrontEnd->Cube_Render(_pRCUtil->GetRC(), _pRCUtil->GetVBM());
	}

	CMWnd_CubeIngameScreen_Parent::OnPaint(_pRCUtil, _Clip, _Client);
}



int CMWnd_CubeIngameScreen::ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
{
	CWFrontEnd_Mod *pFrontEnd = m_CubeUser.GetFrontEnd();
	if(!pFrontEnd)
		return CMWnd_CubeIngameScreen_Parent::ProcessKey(_Key, _OriginalKey);

	if(pFrontEnd->m_spDesktop != NULL)
		return CMWnd_CubeIngameScreen_Parent::ProcessKey(_Key, _OriginalKey);

	if(_OriginalKey.GetKey9() == SKEY_MOUSEMOVEREL)
	{
		pFrontEnd->Cube_UpdateMouse(this);
	}

	CRegistry *pOptions = pFrontEnd->m_pSystem->GetOptions();
	if(pOptions)
	{
		int iScanKey = pOptions->GetValuei("ACTION_JOURNAL");
		if(_Key.GetKey9() == iScanKey)
			ConExecute("pause(0); cmd_forced(closeclientwindow)");
	}

	return CMWnd_CubeIngameScreen_Parent::ProcessKey(_Key, _OriginalKey);
}

//
//
//
void CMWnd_CubeIngameScreen::DoMapping(CWFrontEnd_Mod *_pFrontEnd)
{
	_pFrontEnd->Cube_SetStateFromLayout(this);
}

//
//
//

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeCommonLayout, CMWnd_CubeIngameScreen);


void CMWnd_CubeCommonLayout::OnCreate()
{
	// fetch description area
	m_pDescription = GetItem("Description");

	// fetch buttons
	/*
	CMWnd *pWnd = GetItem("Button");
	int32 Count = 0;

	while(pWnd)
	{
		pWnd->m_ID = "";
		m_lButtons.Add(pWnd);
		pWnd = GetItem("Button");
		Count++;
	};*/

	CMWnd_CubeIngameScreen::OnCreate();
}

//
//
//
void CMWnd_CubeCommonLayout::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
//	CWObject_GameCore::CClientData *pCD = (CWObject_GameCore::CClientData *)m_ClientInfo.m_pClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_GETCLIENTDATA), m_ClientInfo.m_pClient->Game_GetObjectIndex());

	// clear the description area
	//if(m_pDescription)
	//	Layout_Clear(m_pDescription->GetPosition());

	CMWnd_CubeIngameScreen::OnPaint(_pRCUtil, _Clip, _Client);
}

//
//
//

MRTC_IMPLEMENT_DYNAMIC(CMWnd_Mission_Objectives, CMWnd_CubeCommonLayout);


int32 CMWnd_Mission_Objectives::GetNumListItems(bool _Drawing)
{
	CWObject_GameCore::CClientData *pCD = (CWObject_GameCore::CClientData *)m_ClientInfo.m_pClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_GETCLIENTDATA), m_ClientInfo.m_pClient->Game_GetObjectIndex());
	int32 Count = 0;
	if(pCD)
	{
		for(int32 i = 0; i < pCD->m_lMissionFlags.Len(); i++)
		{
			if(pCD->m_lMissionFlags[i]&1) // completed mission
				continue;
			Count++;
		}
	}

	return Count;
}


bool CMWnd_Mission_Objectives::GetListItem(int32 _Index, CStr &_Name, bool _Focus)
{
	// REDO
	CWObject_GameCore::CClientData *pCD = (CWObject_GameCore::CClientData *)m_ClientInfo.m_pClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_GETCLIENTDATA), m_ClientInfo.m_pClient->Game_GetObjectIndex());
	
	if(pCD)
	{
		int32 Count = 0;
		for(int32 i = 0; i < pCD->m_lMissionFlags.Len(); i++)
		{
			if(pCD->m_lMissionFlags[i]&1) // completed mission
				continue;

			if(Count == _Index)
			{

				//_Desc = wText;
				wchar wText[1024];
				Localize_Str(pCD->m_lMissionDesc[i].Str(), wText, 1023);
				CStr Desc = wText;
				_Name = CStrF(WTEXT("s, %s"), Desc.GetStrSep("|").StrW());

				//_Name = CStrF("s, %s", Desc.GetStrSep("|").Str());
				if(_Focus && m_pDescription)
				{
					//wchar wText[1024];
					//Localize_Str(Desc, wText, 1023);
					Layout_Clear(m_pDescription->GetPosition(), 0);
					CRct Pos = m_pDescription->GetPosition();
					Layout_WriteText(Pos, 's', Desc, 0);
					
				}
				return true;
			}
			Count++;
		}
	}
	return false;
}

//
//
//

MRTC_IMPLEMENT_DYNAMIC(CMWnd_Mission_Inventory, CMWnd_CubeCommonLayout);


int32 CMWnd_Mission_Inventory::GetNumListItems(bool _Drawing)
{
	// redo
	//CWObject_GameCore::CClientData *pCD = (CWObject_GameCore::CClientData *)m_ClientInfo.m_pClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_GETCLIENTDATA), m_ClientInfo.m_pClient->Game_GetObjectIndex());
	//pCD->m_
	int iObject = m_ClientInfo.m_pClient->Player_GetLocalObject();
//	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(m_ClientInfo.m_pClient->Object_GetCD(iObject));
	CWObject_Client *pChar = m_ClientInfo.m_pClient->Object_Get(iObject);
	CWO_Character_ClientData* pCD = pChar ? TDynamicCast<CWO_Character_ClientData>((CReferenceCount*)pChar->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
	if(!pCD)
		return 0;

	return pCD->m_InventoryInfo.GetNumMatches(CWO_Inventory_Info::INVENTORYINFO_TYPE_INVENTORY);
	/*
	//static CWO_Character_ClientData* GetClientData();
	//	m_Inventory

	int32 Count = 0;
	for(int32 i = 0; i < 6; i++)
	{
		CFStr Temp;
		CWObject_Message Msg(OBJMSG_CHAR_GETQUESTITEMNAME);
		Msg.m_pData = &Temp;
		Msg.m_Param0 = i;
		if(m_ClientInfo.m_pClient->ClientMessage_SendToObject(Msg, iObject) && Temp.Len())
		{
			Count++;
		}
	}
	return Count;
	*/
}

bool CMWnd_Mission_Inventory::GetListItem(int32 _Index, CStr &_Name, bool _Focus)
{
	// redo
	int iObject = m_ClientInfo.m_pClient->Player_GetLocalObject();
	CWObject_Client *pChar = m_ClientInfo.m_pClient->Object_Get(iObject);
	CWO_Character_ClientData* pCD = pChar ? TDynamicCast<CWO_Character_ClientData>((CReferenceCount*)pChar->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
	if(!pCD)
		return false;

	const CWO_Inventory_Info::CInventoryItem* pItem = pCD->m_InventoryInfo.GetNth(CWO_Inventory_Info::INVENTORYINFO_TYPE_INVENTORY,_Index);
	if (!pItem)
		return false;
	
	_Name = CStrF("s, %s", pItem->m_ItemName.Str());

	// get description aswell
	if(_Focus && m_pDescription)
	{
		Layout_Clear(m_pDescription->GetPosition());

		//Msg.m_Msg = OBJMSG_CHAR_GETQUESTITEMDESC;
		if(pItem->m_ItemDescription.Len())
		{
			//Localize_Str(Temp.Str(), wText, 1023);
			CRct Pos = m_pDescription->GetPosition();
/*			if(pCD->m_Inventory.m_lItems[_Index].m_nItems != -1)
				Layout_WriteText(Pos, CStrF("s, §LMENU_INV_COUNT %d\n%s",
					pCD->m_Inventory.m_lItems[_Index].m_nItems,
					pCD->m_Inventory.m_lItems[_Index].m_Desc.Str()), 0);
			else
				Layout_WriteText(Pos, CStrF("s, %s", pCD->m_Inventory.m_lItems[_Index].m_Desc.Str()), 0);*/
			Layout_WriteText(Pos, CStrF("s, %s§p0%i§pq", pItem->m_ItemDescription.Str(), pItem->m_NumItems), 0);
			//_Desc = wText;
		}

		//Temp = "";
		
		int32 iSurf = pItem->m_iMissionSurface;
		if(iSurf)
		{
			CXW_Surface *pSurf = m_ClientInfo.m_pClient->GetMapData()->GetResource_Surface(iSurf);
			if(pSurf)
			{
				const CXW_SurfaceKeyFrame *pFrame = pSurf->GetBaseFrame();
				if(pFrame)
				{
					if(pFrame->m_lTextures.Len())
						Layout_SetSecondary(pFrame->m_lTextures[0].m_TextureID, CRC_RASTERMODE_ALPHABLEND);
				}
			}
		}
	}

	return true;
	//return pCD->m_Inventory.m_lItems.Len();
	/*
	CWObject_GameCore::CClientData *pCD = (CWObject_GameCore::CClientData *)m_ClientInfo.m_pClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_GETCLIENTDATA), m_ClientInfo.m_pClient->Game_GetObjectIndex());
	int iObject = m_ClientInfo.m_pClient->Player_GetLocalObject();
	
	int32 Count = 0;
	for(int32 i = 0; i < 6; i++)
	{
		CFStr Temp;
		CWObject_Message Msg(OBJMSG_CHAR_GETQUESTITEMNAME);
		Msg.m_pData = &Temp;
		Msg.m_Param0 = i;
		if(m_ClientInfo.m_pClient->ClientMessage_SendToObject(Msg, iObject) && Temp.Len())
		{
			if(Count ==_Index)
			{
				//Localize_Str(Temp.Str(), wText, 1023);
				_Name = CStrF("2s, %s", Temp.Str());

				// get description aswell
				if(_Focus && m_pDescription)
				{
					Layout_Clear(m_pDescription->GetPosition());

					Temp = "";
					Msg.m_Msg = OBJMSG_CHAR_GETQUESTITEMDESC;
					if(m_ClientInfo.m_pClient->ClientMessage_SendToObject(Msg, iObject) && Temp.Len())
					{
						//Localize_Str(Temp.Str(), wText, 1023);
						CRct Pos = m_pDescription->GetPosition();
						Layout_WriteText(Pos, CStrF("2s, %s", Temp.Str()), 0);
						//_Desc = wText;
					}

					//Temp = "";
					
					Msg.m_Msg = OBJMSG_CHAR_GETQUESTICON;
					int32 iSurf = m_ClientInfo.m_pClient->ClientMessage_SendToObject(Msg, iObject);
					if(iSurf)
					{
						
						
						CXW_Surface *pSurf = m_ClientInfo.m_pClient->GetMapData()->GetResource_Surface(iSurf);
						if(pSurf)
						{
							const CXW_SurfaceKeyFrame *pFrame = pSurf->GetBaseFrame();
							if(pFrame)
							{
								if(pFrame->m_lTextures.Len())
									Layout_SetSecondary(pFrame->m_lTextures[0].m_TextureID, CRC_RASTERMODE_ALPHABLEND);
							}
						}

						//	Layout_SetSecondary(pSC->GetSurfaceID(SurfaceName), true);
						
						//MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
						//if(pTC)
						//	m_iSecondaryTexture = pTC->GetTextureID("");
					//		Cube_SetSecondaryTexture(pTC->GetTextureID(_TexName));
						
					}
				}
				return true;
			}
			Count++;
		}
	}
	*/
	//return false;
}

//
//
//

MRTC_IMPLEMENT_DYNAMIC(CMWnd_Mission_PhoneBook, CMWnd_CubeCommonLayout);


int32 CMWnd_Mission_PhoneBook::GetNumListItems(bool _Drawing)
{
	// redo
	//CWObject_GameCore::CClientData *pCD = (CWObject_GameCore::CClientData *)m_ClientInfo.m_pClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_GETCLIENTDATA), m_ClientInfo.m_pClient->Game_GetObjectIndex());
	//pCD->m_
	int iObject = m_ClientInfo.m_pClient->Player_GetLocalObject();
	//	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(m_ClientInfo.m_pClient->Object_GetCD(iObject));
	CWObject_Client *pChar = m_ClientInfo.m_pClient->Object_Get(iObject);
	CWO_Character_ClientData* pCD = pChar ? TDynamicCast<CWO_Character_ClientData>((CReferenceCount*)pChar->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
	if(!pCD)
		return 0;

	// Check how many items are phonebook items
	return pCD->m_InventoryInfo.GetNumMatches(CWO_Inventory_Info::INVENTORYINFO_TYPE_PHONEBOOK);
}

bool CMWnd_Mission_PhoneBook::GetListItem(int32 _Index, CStr &_Name, bool _Focus)
{
	// redo
	int iObject = m_ClientInfo.m_pClient->Player_GetLocalObject();
	CWObject_Client *pChar = m_ClientInfo.m_pClient->Object_Get(iObject);
	CWO_Character_ClientData* pCD = pChar ? TDynamicCast<CWO_Character_ClientData>((CReferenceCount*)pChar->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
	if(!pCD)
		return false;

	const CWO_Inventory_Info::CInventoryItem* pItem = pCD->m_InventoryInfo.GetNth(CWO_Inventory_Info::INVENTORYINFO_TYPE_PHONEBOOK,_Index);
	if (!pItem)
		return false;

	_Name = CStrF("s, %s", pItem->m_ItemName.Str());

	// get description aswell
	if(_Focus && m_pDescription)
	{
		Layout_Clear(m_pDescription->GetPosition());

		//Msg.m_Msg = OBJMSG_CHAR_GETQUESTITEMDESC;
		if(pItem->m_ItemDescription.Len())
		{
			//Localize_Str(Temp.Str(), wText, 1023);
			CRct Pos = m_pDescription->GetPosition();
			Layout_WriteText(Pos, CStrF("s, %s§p0%i§pq", pItem->m_ItemDescription.Str(), pItem->m_NumItems), 0);
			//_Desc = wText;
		}

		//Temp = "";

		int32 iSurf = pItem->m_iMissionSurface;
		if(iSurf)
		{
			CXW_Surface *pSurf = m_ClientInfo.m_pClient->GetMapData()->GetResource_Surface(iSurf);
			if(pSurf)
			{
				const CXW_SurfaceKeyFrame *pFrame = pSurf->GetBaseFrame();
				if(pFrame)
				{
					if(pFrame->m_lTextures.Len())
						Layout_SetSecondary(pFrame->m_lTextures[0].m_TextureID, CRC_RASTERMODE_ALPHABLEND);
				}
			}
		}
	}

	return true;
}

//
//

MRTC_IMPLEMENT_DYNAMIC(CMWnd_Mission_Collectibles, CMWnd_CubeCommonLayout);

/*
class CMWnd_Mission_Collectibles : public CMWnd_CubeCommonLayout
{
	MRTC_DECLARE;

public:
	virtual void OnCreate();
	virtual bool GetListItem(int32 &_Index, CStr &_Name, bool _Focus);
};*/

void CMWnd_Mission_Collectibles::OnCreate()
{
	//ConExecute("unlockall()");
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	//pSys->GetOptions()->SetValue("GAME_EXTRACONTENTKEY", "0000001F00000000");
	m_Key.Parse(pSys->GetOptions()->GetValue("GAME_EXTRACONTENTKEY"));
	CMWnd_CubeCommonLayout::OnCreate();
}

int32 CMWnd_Mission_Collectibles::GetNumListItems(bool _Drawing)
{
	int32 count = 0;
	int32 i = 0;
	while(i < 64)
	{
		CExtraContentKey Key;
		Key.AddKeyBit(i);
		if(m_Key.IsActive(Key))
			count++;
		i++;
	}

	return count;
}

bool CMWnd_Mission_Collectibles::GetListItem(int32 _Index, CStr &_Name, bool _Focus)
{

	int32 count = 0;
	int32 i = 0;
	while(i < 64)
	{
		CExtraContentKey Key;
		Key.AddKeyBit(i);
		if(m_Key.IsActive(Key))
		{
			if(count == _Index)
			{
				_Name = CStrF("s, §LMENU_CIGPACK %d", i+1);


				if(_Focus)
				{
					int iRc = m_ClientInfo.m_pFrontEnd->m_spMapData->GetResourceIndex_Registry("Registry\\RpgCollectibles");
					CRegistry* pReg = m_ClientInfo.m_pFrontEnd->m_spMapData->GetResource_Registry(iRc);
					Layout_SetSecondary(-1, CRC_RASTERMODE_ALPHABLEND);
					if(pReg && pReg->GetNumChildren() > 0)
					{
						for(int32 c = 0; c < pReg->GetNumChildren(); c++)
						{
							CRegistry *pChild = pReg->GetChild(c);
							int32 Collectid = pChild->GetValuei("collectibleid", -1);
							if(pChild && Collectid == i)
							{
								CStr SurfaceName = pChild->GetValue("iconsurface");
								MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
								if(pTC)
									Layout_SetSecondary(pTC->GetTextureID(SurfaceName), CRC_RASTERMODE_ALPHABLEND);
								break;
							}
						}
					}
				}
				/*
				int iRc = m_spMapData->GetResourceIndex_Registry("GUI\\CubeWnd");
				//int iRc = m_spMapData->GetResourceIndex_Registry("GUI\\ModWnd");


				CRegistry *pRc = NULL;

				if(0) // set this to 1 to ease development of the GUI files
				{
					CRegistry_Dynamic Reg;
					Reg.XRG_Read("s:\\PB\\Content\\GUI\\CubeWnd.xrg");
					pRc = &Reg;
				}
				else
				{
					pRc = m_spMapData->GetResource_Registry(iRc);
				}*/


				return true;
			}

			count++;
		}
		i++;
	}

	return false;

	// REDO
	/*
	while(_Index < 128)
	{
		CExtraContentKey Key;
		Key.AddKeyBit(_Index);
		if(m_Key.IsActive(Key))
		{
			_Name = CStrF("2s, Packet %d", _Index+1);
			_Index++;
			return true;
		}
		_Index++;
	}
	
*/
	/*
	CWObject_GameCore::CClientData *pCD = (CWObject_GameCore::CClientData *)m_ClientInfo.m_pClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_GETCLIENTDATA), m_ClientInfo.m_pClient->Game_GetObjectIndex());
	int iObject = m_ClientInfo.m_pClient->Player_GetLocalObject();
	
	for(; _Index < 6; _Index++)
	{
		CFStr Temp;
		CWObject_Message Msg(OBJMSG_CHAR_GETQUESTITEMNAME);
		Msg.m_pData = &Temp;
		Msg.m_Param0 = _Index;
		if(m_ClientInfo.m_pClient->ClientMessage_SendToObject(Msg, iObject) && Temp.Len())
		{
			//Localize_Str(Temp.Str(), wText, 1023);
			_Name = CStrF("2s, %s", Temp.Str());

			// get description aswell
			if(_Focus && m_pDescription)
			{
				Layout_Clear(m_pDescription->GetPosition());

				Msg.m_Msg = OBJMSG_CHAR_GETQUESTITEMDESC;
				if(m_ClientInfo.m_pClient->ClientMessage_SendToObject(Msg, iObject) && Temp.Len())
				{
					//Localize_Str(Temp.Str(), wText, 1023);
					CRct Pos = m_pDescription->GetPosition();
					Layout_WriteText(Pos, CStrF("2s, %s", Temp.Str()), 0);
					//_Desc = wText;
				}
			}

			// increase index and return
			_Index++;
			return true;
		}
	}
	*/
	return false;
}



//
//
//

MRTC_IMPLEMENT_DYNAMIC(CMWnd_Mission_Map, CMWnd_CubeIngameScreen);


void CMWnd_Mission_Map::OnCreate()
{
}

void CMWnd_Mission_Map::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect& _Clip, const CRct& _Client)
{
	CWO_MapInfo MapInfo;

	// get map info
	int iObject = m_ClientInfo.m_pClient->Player_GetLocalObject();
	CWObject_Message Msg(OBJMSG_CHAR_GETMAPINFO);
	Msg.m_pData = &MapInfo;
	if(!m_ClientInfo.m_pClient->ClientMessage_SendToObject(Msg, iObject))
		return; // no map info, aka, big problems

	CRct Pos;
	Pos.p0.x = int(1*(640/20.0f));
	Pos.p0.y = int(3*(480/20.0f));
	Pos.p1.x = int(15*(640/20.0f));
	Pos.p1.y = int(15*(480/20.0f));

	//wchar wText[1024];
	/*
	CStr Str = CStrF("%s %d %x", MapInfo.m_OverviewMap.Str(), MapInfo.m_OverviewMap_Current, MapInfo.m_OverviewMap_Visited);
	
	Localize_Str(Str.Str(), wText, 1023);
	Layout_WriteText(Pos, 1, 's', wText);
	*/

	int32 places = 0;
	int32 count = 0;
	int32 rows = 0;
	int32 parts = 0;
	int32 foundparts = 0;

	if(MapInfo.m_OverviewMap != "")
	{
		int iRc = m_ClientInfo.m_pClient->GetMapData()->GetResourceIndex_Registry("Registry\\Map");
		CRegistry* pReg = m_ClientInfo.m_pClient->GetMapData()->GetResource_Registry(iRc);
		if(pReg && pReg->GetNumChildren() > 0)
		{
			CRegistry *pMap = pReg->GetChild(0)->FindChild(MapInfo.m_OverviewMap);
			if(pMap)
			{

				CStr SurfaceName = pMap->GetValue("SURFACE");
				MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
				if(pTC)
					Layout_SetSecondary(pTC->GetTextureID(SurfaceName), CRC_RASTERMODE_ADD);
				//Layout_SetSecondary(pTC->GetTextureID("Map_P_UDMoney"), false);
				//	m_iSecondaryTexture = pTC->GetTextureID(ImageName);
				//MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
				//if(pSC)
				//	Layout_SetSecondary(pSC->GetSurfaceID(SurfaceName), true);

			

				//CStr Map = pChild->GetValue("BASETEXTURE");
				//if(Map != "")
				//{
//					CVec2Dfp32 Scale(640.0f / 512.0f, 480.0f / 512.0f);
				//CVec2Dfp32 Scale(512.0f / 640.0f, 512.0f / 480.0f);
				//_pRCUtil->DrawTexture(Clip, CPnt(0, 0), Map, 0xff808080, Scale);
				
				int Visited = MapInfo.m_OverviewMap_Visited;
				//Visited = 0xFFFF; // everywhere //65535 + 65536;
				int iZone = 0;
				for(;Visited != 0; Visited >>= 1, iZone++)
				{
					if(!(Visited&1))
						continue;

					CRegistry *pPiece = pMap->FindChild(CFStrF("%i", iZone));
					if(pPiece)// && pPiece->GetThisValue() != "")
					{
						places++;

						for(int32 i = 0; i < pPiece->GetNumChildren(); i++)
						{
							CRegistry *pChild = pPiece->GetChild(i);
							if(!pChild)
								break;
							
							if(pChild->GetThisName() == "PART")
							{
								parts++;
								CRegistry *pPart = pMap->FindChild("PART", pChild->GetThisValue());
								if(!pPart)
									continue;
								foundparts++;

								CStr s = pPart->GetValue("OFFSET");
								CPnt Start;
								CPnt Current;
								Start.x = s.GetStrSep(",").Val_int()+2;
								Start.y = s.Val_int()+2;
								Current = Start;

								for(int32 r = 0; r < pPart->GetNumChildren(); r++)
								{
									CRegistry *pChild = pPart->GetChild(r);
									if(!pChild)
										break;
									
									if(pChild->GetThisName() == "ROW")
									{
										rows++;
										s = pChild->GetThisValue();
										for(int32 c = 0; c < s.Len(); c++, Current.x++)
											if(s.GetStr()[c] == '#')
											{
												m_aMap[Current.y][Current.x].m_Mode = CCellInfo::MODE_SECONDARY;
												m_aMap[Current.y][Current.x].Cell() = CCellInfo::Empty();
												m_aMap[Current.y][Current.x].Flags() = 0;

												if(MapInfo.m_OverviewMap_Current == iZone && i == 0)
													m_aDepth[Current.y][Current.x] = 0.05f;
												count++;
											}
										
										Current.x = Start.x;
										Current.y++;
									}
								}
							}
						}

						
						/*
						int32 index = 1;
						while(1)
						{
							CStr Value = pPiece->GetValue(CFStrF("PART%i", index));
							if(!Value.Len())
								break;

							
							index++;	
						}*/

/*
						int32 index = 1;
						while(1)
						{
							CStr Value = pPiece->GetValue(CFStrF("RGN%i", index));
							if(!Value.Len())
								break;
							
							CRct Pos;
							Pos.p0.x = Value.GetStrSep(",").Val_int();
							Pos.p0.y = Value.GetStrSep(",").Val_int();
							Pos.p1.x = Value.GetStrSep(",").Val_int();
							Pos.p1.y = Value.GetStrSep(",").Val_int();
							CStr Text = Value.GetStrSep(",");//.Val_int();
							Pos.p1 += Pos.p0;
							
							if(MapInfo.m_OverviewMap_Current == iZone)
								Layout_SetDepth(Pos, 0.065f);
							else
								Layout_SetDepth(Pos, 0.05f);
							
							if(Text.Len())
							{
								Localize_Str(Text.Str(), wText, 1023);
								if(MapInfo.m_OverviewMap_Current == iZone)
									Layout_WriteText(Pos, 1, 's', wText, FLAG_NO_CONVERT);
								else
									Layout_WriteText(Pos, 1, 's', wText, FLAG_NO_CONVERT);
							}
							index++;
						}
						index = 1;
						while(1)
						{
							CStr Value = pPiece->GetValue(CFStrF("CON%i", index));
							if(!Value.Len())
								break;
							
							CRct Pos;
							Pos.p0.x = Value.GetStrSep(",").Val_int();
							Pos.p0.y = Value.GetStrSep(",").Val_int();
							if(Value.Len())
							{
								Pos.p1.x = Value.GetStrSep(",").Val_int();
								Pos.p1.y = Value.GetStrSep(",").Val_int();
							}
							else
							{
								Pos.p1.x = 1;
								Pos.p1.y = 1;
							}
							Pos.p1 += Pos.p0;
							Layout_SetMapping(Pos, 16*12-1);
							Layout_SetDepth(Pos, 0.045f);
							index++;
						}
						*/
					}
				}
				//}
			}
		}
	}

	count = count;
	
	CMWnd_CubeIngameScreen::OnPaint(_pRCUtil, _Clip, _Client);
}
