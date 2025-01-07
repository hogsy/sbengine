
#include "PCH.h"

#include "MWinCtrlMod.h"
#include "WClientModWnd.h"
#include "../GameClasses/WObj_Game/WObj_GameMod.h"
#include "../GameClasses/WObj_CharClientData.h"

// -------------------------------------------------------------------
//  CMWnd_ModSelectClass
// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModSelectClass, CMWnd)
MRTC_IMPLEMENT_DYNAMIC(CMWnd_ModSelectTeam, CMWnd)

// -------------------------------------------------------------------

CMWnd_CharacterList::CMWnd_CharacterList()
{
	m_ItemHeight = 16;
	m_TextHeight = 14;
}

void CMWnd_CharacterList::SetItemCount(int _nItems)
{
	m_lChars.QuickSetLen(_nItems);
	CMWnd_List2::SetItemCount(_nItems);
}

void CMWnd_CharacterList::AddItem(const CListItem_Char& _Item)
{
	m_lChars.Add(_Item);
	CMWnd_List2::SetItemCount(m_lChars.Len());
}

void CMWnd_CharacterList::OnPaintItem(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client, int _iItem)
{
	CRC_Font* pF = GetFont("TEXT");
	if(!pF)
		return;
	
	int Alpha = m_Col_Alpha << 24;
//	int ColH = m_Col_Highlight + Alpha;
//	int ColD = m_Col_Shadow + Alpha;
	int ColM = m_Col_Background + Alpha;
	int ColBG_Sel = m_Col_Custom1_Focus + Alpha;
	int ColT = m_Col_Custom0 + Alpha;
	int ColT_Sel = m_Col_Custom0_Focus + Alpha;
	
	int bSelected = (m_lSelect[_iItem] & 1);
	int TCol = ColT;
	int BGCol = ColM;
	if (bSelected) 
	{
		TCol = ColT_Sel;
		BGCol = ColBG_Sel;
	}
	
	if (GetItemFocus() == _iItem)
		BGCol |= 0x001f1f3f;
	
	const CListItem_Char& Item = m_lChars[_iItem];
	
	_pRCUtil->Rect(_Clip, _Client, BGCol);
	
	_pRCUtil->Text(_Clip, pF, 2, 1, (const char*) Item.m_Name, TCol, m_TextHeight);
	
	//		if (GetItemFocus() == _iItem)
	//			_pRCUtil->Frame(_Clip, 0, 0, _Client.GetWidth()-1, _Client.GetHeight()-1, 0xff808080, 0xff808080);
}

void CMWnd_CharacterList::OnTouchItem(int _iItem, int _bSelected)
{
	if (GetParent())
	{
		CMWnd_Message Msg(WMSG_COMMAND, m_ID, _iItem, _bSelected);
		GetParent()->OnMessage(&Msg);
	}
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CharacterList, CMWnd_List2);

// -------------------------------------------------------------------

CMWnd_TeamList::CMWnd_TeamList()
{
	m_ItemHeight = 16;
	m_TextHeight = 14;
}

void CMWnd_TeamList::SetItemCount(int _nItems)
{
	m_lChars.QuickSetLen(_nItems);
	CMWnd_List2::SetItemCount(_nItems);
}

void CMWnd_TeamList::AddItem(const CListItem_Team& _Item)
{
	m_lChars.Add(_Item);
	CMWnd_List2::SetItemCount(m_lChars.Len());
}

void CMWnd_TeamList::OnPaintItem(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client, int _iItem)
{
	CRC_Font* pF = GetFont("TEXT");
	if(!pF)
		return;

	int Alpha = m_Col_Alpha << 24;
	//	int ColH = m_Col_Highlight + Alpha;
	//	int ColD = m_Col_Shadow + Alpha;
	int ColM = m_Col_Background + Alpha;
	int ColBG_Sel = m_Col_Custom1_Focus + Alpha;
	int ColT = m_Col_Custom0 + Alpha;
	int ColT_Sel = m_Col_Custom0_Focus + Alpha;

	int bSelected = (m_lSelect[_iItem] & 1);
	int TCol = ColT;
	int BGCol = ColM;
	if (bSelected) 
	{
		TCol = ColT_Sel;
		BGCol = ColBG_Sel;
	}

	if (GetItemFocus() == _iItem)
		BGCol |= 0x001f1f3f;

	const CListItem_Team& Item = m_lChars[_iItem];

	_pRCUtil->Rect(_Clip, _Client, BGCol);

	_pRCUtil->Text(_Clip, pF, 2, 1, (const char*) Item.m_TeamName, TCol, m_TextHeight);
	_pRCUtil->Text(_Clip, pF, 200, 1, CStrF("Players: %i", Item.m_Players), TCol, m_TextHeight);
}

void CMWnd_TeamList::OnTouchItem(int _iItem, int _bSelected)
{
	if (GetParent())
	{
		CMWnd_Message Msg(WMSG_COMMAND, m_ID, _iItem, _bSelected);
		GetParent()->OnMessage(&Msg);
	}
}

MRTC_IMPLEMENT_DYNAMIC(CMWnd_TeamList, CMWnd_List2);

// -------------------------------------------------------------------

CMWnd_ModSelectClass::CMWnd_ModSelectClass()
{
	m_SelectionFilter = "PLAYER_";
}

int CMWnd_ModSelectClass::GetItemState(const char* _pName)
{
	CMWnd* pWnd = GetItem(_pName);
	if (pWnd)
	{
		CMWnd_Message Msg(WMSG_GETSTATE, NULL, 0);
		return pWnd->OnMessage(&Msg);
	}
	return 0;
}

void CMWnd_ModSelectClass::SetItemState(const char* _pName, int _Value)
{
	CMWnd* pWnd = GetItem(_pName);
	if (pWnd)
	{
		CMWnd_Message Msg(WMSG_SETSTATE, NULL, _Value);
		pWnd->OnMessage(&Msg);
	}
}

void CMWnd_ModSelectClass::OnCreate()
{
	CMWnd::OnCreate();
	
	UpdateCharacterList();
}

void CMWnd_ModSelectClass::UpdateCharacterList()
{
	CMWnd* pWnd = GetItem("CharacterList");
	CMWnd_CharacterList* pList = safe_cast<CMWnd_CharacterList>(pWnd);

	if (pList && m_ClientInfo.m_pClient)
	{
		pList->SetItemCount(0);

		int State = m_ClientInfo.m_pClient->GetMapData()->GetState();
		// Allow load of template-registry (it's only for debug purposes :)
		m_ClientInfo.m_pClient->GetMapData()->SetState(State & ~WMAPDATA_STATE_NOCREATE);
		int iRc = m_ClientInfo.m_pClient->GetMapData()->GetResourceIndex_Registry("Registry\\Sv");
		CRegistry *pReg = m_ClientInfo.m_pClient->GetMapData()->GetResource_Registry(iRc);
		m_ClientInfo.m_pClient->GetMapData()->SetState(State);

		if(pReg)
		{
			int iSel = 0;
			CRegistry *pTemplates = pReg->FindChild("TEMPLATES");
			if(pTemplates)
			{
				CStr FirstPlayer;
				for(int i = 0; i < pTemplates->GetNumChildren(); i++)
				{
					CRegistry *pChar = pTemplates->GetChild(i);
					CStr St = pChar->GetThisName();
					CStr Name;
					if(St.CompareSubStr(m_SelectionFilter) == 0)
					{
						if(FirstPlayer == "")
							FirstPlayer = St;
						else if(St.CompareSubStr(FirstPlayer) == 0)
						{
							if(St == FirstPlayer + "_" + m_ClientInfo.m_pClient->m_WorldName)
								iSel = pList->GetItemCount();
							else if(pChar->GetValuei("DEBUGCHAR") != 1)
								continue;
						}
						//Name = St.Copy(m_SelectionFilter.Len(), 1024);
						Name = pChar->GetValue("character_name");
					}
					if(St.CompareSubStr("AI_") == 0)
					{
						if(pChar->GetValuei("DEBUGCHAR") == 1)
							Name = St.Copy(3, 1024);
					}
					if(Name != "")
					{
						Name.MakeLowerCase();
						int team = pChar->GetValuei("team");
						CStr Darkling = pChar->GetValue("Darkling");
						if(m_ClientInfo.m_pClient->m_MPTeam != -1)
						{
							if(team == m_ClientInfo.m_pClient->m_MPTeam)
								pList->AddItem(CListItem_Char(Name, St, Darkling));
						}
						else
							pList->AddItem(CListItem_Char(Name, St, Darkling));
					}
				}
			}
			pList->m_iItemFocus = iSel;
		}
	}
}

void CMWnd_ModSelectClass::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	MAUTOSTRIP(CMWnd_Client_EvaluateKey, MAUTOSTRIP_VOID);

	if(_Key == "SELECTIONFILTER")
	{
		m_SelectionFilter = _Value;
	}
	else
		CMWnd::EvaluateKey(_pParam, _Key, _Value);
}

aint CMWnd_ModSelectClass::OnMessage(const CMWnd_Message* _pMsg)
{
	switch(_pMsg->m_Msg)
	{
	case WMSG_GETMAPDATA : 
		return (aint)(CMapData*) ((m_ClientInfo.m_pClient) ? m_ClientInfo.m_pClient->m_spGUIData : (TPtr<CMapData>)NULL);

	case WMSG_KEY :
		{
			return CMWnd::OnMessage(_pMsg);
		}
	case WMSG_REGISTRYCHANGED:
		{
			UpdateCharacterList();
			return 1;
		}
	case WMSG_COMMAND :
		if (_pMsg->m_pID)
		{
			if (strcmp(_pMsg->m_pID, "CharacterList") == 0)
			{
				CMWnd* pWnd = GetItem("CharacterList");
				CMWnd_CharacterList* pList = safe_cast<CMWnd_CharacterList>(pWnd);

				if (pList)
				{
					int iItem = pList->GetSelected();
					if (pList->m_lChars.ValidPos(iItem))
					{
						CListItem_Char* pItem = &pList->m_lChars[iItem];
						ConExecute(CStrF("mp_setdarklingmodelname(\"%s\")", pItem->m_Darkling.Str()));
						ConExecute(CStrF("mp_setmodelname(\"%s\")", pItem->m_Class.Str()));
					}
				}
			}
		}
	default:
		return CMWnd::OnMessage(_pMsg);
	}
}

// ---------------------------------------------------------------------------------------

CMWnd_ModSelectTeam::CMWnd_ModSelectTeam()
{
	m_LastUpdateTick = 0;
}

int CMWnd_ModSelectTeam::GetItemState(const char* _pName)
{
	CMWnd* pWnd = GetItem(_pName);
	if (pWnd)
	{
		CMWnd_Message Msg(WMSG_GETSTATE, NULL, 0);
		return pWnd->OnMessage(&Msg);
	}
	return 0;
}

void CMWnd_ModSelectTeam::SetItemState(const char* _pName, int _Value)
{
	CMWnd* pWnd = GetItem(_pName);
	if (pWnd)
	{
		CMWnd_Message Msg(WMSG_SETSTATE, NULL, _Value);
		pWnd->OnMessage(&Msg);
	}
}

void CMWnd_ModSelectTeam::OnCreate()
{
	CMWnd::OnCreate();

	UpdateCharacterList();
}

void CMWnd_ModSelectTeam::UpdateCharacterList()
{
	CMWnd* pWnd = GetItem("TeamList");
	CMWnd_TeamList* pList = safe_cast<CMWnd_TeamList>(pWnd);

	if (pList && m_ClientInfo.m_pClient)
	{
		pList->SetItemCount(0);

		int State = m_ClientInfo.m_pClient->GetMapData()->GetState();
		// Allow load of template-registry (it's only for debug purposes :)
		m_ClientInfo.m_pClient->GetMapData()->SetState(State & ~WMAPDATA_STATE_NOCREATE);
		int iRc = m_ClientInfo.m_pClient->GetMapData()->GetResourceIndex_Registry("Registry\\Sv");
		CRegistry *pReg = m_ClientInfo.m_pClient->GetMapData()->GetResource_Registry(iRc);
		m_ClientInfo.m_pClient->GetMapData()->SetState(State);

		CWObject_GameCore::CClientData *pCD = (CWObject_GameCore::CClientData *)m_ClientInfo.m_pClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_GETCLIENTDATA), m_ClientInfo.m_pClient->Game_GetObjectIndex());

		int team1 = 0;
		int team2 = 0;
		for(int i = 0; i < pCD->m_lPlayerTeams.Len(); i++)
		{
			if(pCD->m_lPlayerTeams[i] == 1)
				team2++;
			else if(pCD->m_lPlayerTeams[i] == 0)
				team1++;
		}

		if(pReg)
		{
			int iSel = 0;

			pList->AddItem(CListItem_Team("Team 1", 0, team1));
			pList->AddItem(CListItem_Team("Team 2", 1, team2));

			pList->m_iItemFocus = iSel;
		}
	}
}

void CMWnd_ModSelectTeam::OnRefresh()
{
	if(m_ClientInfo.m_pClient->GetGameTick() < (RoundToInt(1.0f * m_ClientInfo.m_pClient->GetGameTicksPerSecond()) + m_LastUpdateTick))
		return;

	m_LastUpdateTick = m_ClientInfo.m_pClient->GetGameTick();

	CMWnd* pWnd = GetItem("TeamList");
	CMWnd_TeamList* pList = safe_cast<CMWnd_TeamList>(pWnd);

	CWObject_GameCore::CClientData *pCD = (CWObject_GameCore::CClientData *)m_ClientInfo.m_pClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_GETCLIENTDATA), m_ClientInfo.m_pClient->Game_GetObjectIndex());

	int team1 = 0;
	int team2 = 0;
	for(int i = 0; i < pCD->m_lPlayerTeams.Len(); i++)
	{
		if(pCD->m_lPlayerTeams[i] == 1)
			team2++;
		else if(pCD->m_lPlayerTeams[i] == 0)
			team1++;
	}

	pList->m_lChars[0].m_Players = team1;
	pList->m_lChars[1].m_Players = team2;
}

aint CMWnd_ModSelectTeam::OnMessage(const CMWnd_Message* _pMsg)
{
	switch(_pMsg->m_Msg)
	{
	case WMSG_GETMAPDATA : 
		return (aint)(CMapData*) ((m_ClientInfo.m_pClient) ? m_ClientInfo.m_pClient->m_spGUIData : (TPtr<CMapData>)NULL);

	case WMSG_KEY :
		{
			return CMWnd::OnMessage(_pMsg);
		}
	case WMSG_REGISTRYCHANGED:
		{
			UpdateCharacterList();
			return 1;
		}
	case WMSG_COMMAND :
		if (_pMsg->m_pID)
		{
			if (strcmp(_pMsg->m_pID, "TeamList") == 0)
			{
				CMWnd* pWnd = GetItem("TeamList");
				CMWnd_TeamList* pList = safe_cast<CMWnd_TeamList>(pWnd);

				if (pList)
				{
					int iItem = pList->GetSelected();
					if (pList->m_lChars.ValidPos(iItem))
					{
						CListItem_Team* pItem = &pList->m_lChars[iItem];
						ConExecute(CStrF("mp_setteam(\"%i\")", pItem->m_Value));
					}
				}
			}
		}
	default:
		return CMWnd::OnMessage(_pMsg);
	}
}
