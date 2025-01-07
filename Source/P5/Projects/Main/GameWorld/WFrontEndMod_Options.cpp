#include "PCH.h"

#include "WFrontEndMod.h"
#include "WFrontEndMod_Menus.h"
#include "WFrontEndMod_Options.h"

#if defined(PLATFORM_XBOX1) && !defined(M_DEMO_XBOX)
// --------------------------------------------------------------------
// --------------------------------------------------------------------
// --------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Live_Message, CMWnd_CubeMenu);

void CMWnd_CubeMenu_Live_Message::OnCreate()
{
	if(m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0].Len())
	{
		m_NextScreen = m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0].GetStrSep(",");
		m_Message = m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0];
	}
	m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "";
	CMWnd_CubeMenu::OnCreate();
}

void CMWnd_CubeMenu_Live_Message::EvaluateKey(CMWnd_Param* _pParam, const CStr& _Key, const CStr& _Value)
{
	if(_Key == "NEXTSCREEN")
		m_NextScreen = _Value;
	else
		CMWnd_CubeMenu::EvaluateKey(_pParam, _Key, _Value);
}

aint CMWnd_CubeMenu_Live_Message::OnMessage(const CMWnd_Message* _pMsg)
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
					ConExecute("cg_prevmenu()");
					return true;
				}
			}
		}
	}

	return CMWnd_CubeMenu::OnMessage(_pMsg);
}

void CMWnd_CubeMenu_Live_Message::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	//m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "";
	if(m_Message.Len())
	{
		CRct Rct;
		Rct.p0.x = 0;
		Rct.p0.y = 9;
		Rct.p1.x = 19;
		Rct.p1.y = 11;
		Layout_WriteText(Rct, CStrF("nc, %s", m_Message.Str()),
							CMWnd_CubeLayout::FLAG_NO_CONVERT | CMWnd_CubeLayout::FLAG_CENTER);
	}
	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);
}

// --------------------------------------------------------------------
// --------------------------------------------------------------------
// --------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Live_Account, CMWnd_CubeMenu);

bool CMWnd_CubeMenu_Live_Account::OnButtonPress(CMWnd *_pFocus)
{
    if(m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0].Len())
	{
		XONLINE_USER *pUser = m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetUser(m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0].Val_int());
		if(pUser)
		{
			if(pUser->dwUserOptions&XONLINE_USER_OPTION_REQUIRE_PASSCODE)
				ConExecute("cg_switchmenu(\"live_passcode\")");
			else
				ConExecute("cg_switchmenu(\"live_login\")");
			return true;
		}
	}
	return false;
}

int32 CMWnd_CubeMenu_Live_Account::GetNumListItems(bool _Drawing)
{
	if(_Drawing)
		m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "";
	return m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetNumUsers();
}

bool CMWnd_CubeMenu_Live_Account::GetListItem(int32 _Index, CStr &_Name, bool _Focus)
{

	//XONLINE_USER aUsers[32];
	//int32 NumUsers = pContext->m_ExtraContent_Live.GetUsers(aUsers, 32);

	if(_Index < m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetNumUsers()) 
	{
		XONLINE_USER *pUser = m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetUser(_Index);
		if(pUser)
		{
			wchar wText[1024];
			Localize_Str(CStrF("nc, %s", pUser->szGamertag), wText, 1023);
			_Name = wText;
			
			if(_Focus)
				m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = CStrF("%d", _Index);

			_Index++;
			
			return true;
		}
	} 
	/*
	else if(_Index == 0)
	{
		wchar wText[1024];
		Localize_Str(CStr("s, No accounts found"), wText, 1023);
		_Name = wText;
			
		m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "";

		_Index++;
		return true;
	}
	*/



	/*
	if(m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetLoginStatus() == CLiveHandler::STATUS_SUCCESS)
		return false;
	
	_Index -= m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetNumUsers();

	if(_Index == 0)
	{
		_Name = "";
		_Index++;
		return true;
	}
	else if(_Index == 1)
	{
		wchar wText[1024];
		Localize_Str("Logout", wText, 1023);
		m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "LOGOUT";
		_Name = wText;
		_Index++;
		return true;
	}*/
	

	return false;
}

void CMWnd_CubeMenu_Live_Account::OnCreate()
{
	m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "";
	CMWnd_CubeMenu::OnCreate();
}

// --------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Live_Login, CMWnd_CubeMenu);
void CMWnd_CubeMenu_Live_Login::OnCreate()
{
	m_State = 0;
	m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.ClearLoginStatus();
	XONLINE_USER *pUser = m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetUser(m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0].Val_int());
	if(pUser)
		m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.Login(pUser);
	m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "";

	CMWnd_CubeMenu::OnCreate();
}

void CMWnd_CubeMenu_Live_Login::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	static CMTime BlinkTime = CMTime::GetCPU();
	static int32 Counter = 0;

	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);

	if((CMTime::GetCPU()-BlinkTime).GetTime() > 0.1f)
	{
		Counter++;
		Counter %= 10;
		BlinkTime = CMTime::GetCPU();
	}

	int32 n = Counter;
	for(int32 i = 0; i < 10; i++, n++)
	{
		n %= 10;
		if(n < 5)
			Layout_SetMapping(10+4-i, 9, 16*15, CCellInfo::FLAG_FASTSWITCH);
		else
			Layout_SetMapping(10+4-i, 9, 0, CCellInfo::FLAG_FASTSWITCH);
	}

	if(m_State == 0)
	{
		if(m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetLoginStatus() == CLiveHandler::STATUS_SUCCESS)
		{
			m_State++;
			m_CubeUser.m_pGameContextMod->m_ExtraContent.Update();
		}
		else if(m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetLoginStatus() == CLiveHandler::STATUS_PROCESSING)
		{
			// processing
		}
		else
		{
			ConExecute(CStr("cg_switchmenu(\"%s\")", m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetLoginError()));
			//m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "live_account,Login Error";
			//ConExecute("cg_switchmenu(\"live_login\")");
		}
	}
	else if(m_State == 1)
	{
		if(m_CubeUser.m_pGameContextMod->m_ExtraContent.IsUpdateDone())
			m_State++;
	}
	else
	{
		if(m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[1].Len())
			ConExecute(CStr("cg_switchmenu(\"%s\")", m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[1].Str()));
		else
			ConExecute("cg_prevmenu()");


		// there maybe an extra message that we need to show
		//if(m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetLoginError())
		//	ConExecute(CStr("cg_submenu(\"%s\")", m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetLoginError()));
		//SetMenu(m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[1], ""); // FIXME: we need to back up here
		//SetMenu("live_updatecontentlist", "");

	}
}


// --------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Live_PassCode, CMWnd_CubeMenu);

void CMWnd_CubeMenu_Live_PassCode::OnCreate()
{
	Reset();
}

void CMWnd_CubeMenu_Live_PassCode::Reset()
{
	m_aPasscode[0] = m_aPasscode[1] = m_aPasscode[2] = m_aPasscode[3] = -1;
	m_Current = 0;
}

void CMWnd_CubeMenu_Live_PassCode::PerformCheck()
{
	XONLINE_USER *pUser = m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetUser(m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0].Val_int());
	if(pUser)
	{
		bool Diff = false;
		for(int32 i = 0; i < 4; i++)
			if(pUser->passcode[i] != m_aPasscode[i])
			{
				Diff = true;
				break;
			}

		if(Diff)
			ConExecute("cg_submenu(\"live_wrong_passcode\")");
		else
			ConExecute("cg_switchmenu(\"live_login\")");
	}
	Reset();

	//m_CubeUser.m_pGameContextMod->m_ExtraContent_Live
}

void CMWnd_CubeMenu_Live_PassCode::Add(int8 _p)
{
	m_aPasscode[m_Current++] = _p;
	if(m_Current == 4)
		PerformCheck();
}



int CMWnd_CubeMenu_Live_PassCode::ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
{
	if(_Key.IsDown())
	{
		switch(_Key.GetKey9())
		{
			case SKEY_GUI_UP: Add(1); break;
			case SKEY_GUI_DOWN: Add(2); break;
			case SKEY_GUI_LEFT: Add(3); break;
			case SKEY_GUI_RIGHT: Add(4); break;
			case SKEY_GUI_BUTTON2: Add(5); break; // X
			case SKEY_GUI_BUTTON3: Add(6); break; // Y
			case SKEY_JOY0_AXIS06: Add(9); break; // L
			case SKEY_JOY0_AXIS07: Add(10); break; // R
			default : return CMWnd_CubeMenu::ProcessKey(_Key, _OriginalKey);
		}
		return true;
	}

	switch(_Key.GetKey9())
	{
		case SKEY_GUI_UP: return true;
		case SKEY_GUI_DOWN: return true;
		case SKEY_GUI_LEFT: return true;
		case SKEY_GUI_RIGHT: return true;
		case SKEY_GUI_BUTTON2: return true;
		case SKEY_GUI_BUTTON3: return true;
		case SKEY_JOY0_AXIS06: return true;
		case SKEY_JOY0_AXIS07: return true;
	}

	return CMWnd_CubeMenu::ProcessKey(_Key, _OriginalKey);
}

/*
int CMWnd_CubeMenu_Live_PassCode::OnMessage(const CMWnd_Message* _pMsg)
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
				if (Key.GetKey9() == SKEY_GUI_BACK || Key.GetKey9() == SKEY_GUI_CANCEL)
					break;
				
				
				//if(Key.GetKey9() == SKEY_JOY0_AXIS0E && Key.m_Data > 64) Add(1);
				if(Key.GetKey9() == SKEY_GUI_DOWN) Add(2);
				if(Key.GetKey9() == SKEY_GUI_LEFT) Add(3);
				if(Key.GetKey9() == SKEY_GUI_RIGHT) Add(4);
				if(Key.GetKey9() == SKEY_GUI_BUTTON2) Add(5); // x?
				if(Key.GetKey9() == SKEY_GUI_BUTTON3) Add(6); // y?

				return true;
			}
		}
	}

	return CMWnd_CubeMenu::OnMessage(_pMsg);
}*/

void CMWnd_CubeMenu_Live_PassCode::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	for(int32 i = 0; i < 4; i++) 
	{
		if(m_Current > i)
		{
			Layout_SetMapping(6+i*2, 9, 16*16+6);
			m_aDepth[9][6+i*2] = 0;
		}
		else
		{
			Layout_SetMapping(6+i*2, 9, 0);
			m_aDepth[9][6+i*2] = 0.05f;
		}
	}

	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);
}

// --------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Live_UpdateContentList, CMWnd_CubeMenu);
void CMWnd_CubeMenu_Live_UpdateContentList::OnCreate()
{
	m_CubeUser.m_pGameContextMod->m_ExtraContent.Update();
	m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "";
	CMWnd_CubeMenu::OnCreate();
}

void CMWnd_CubeMenu_Live_UpdateContentList::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);

	if(m_CubeUser.m_pGameContextMod->m_ExtraContent.IsUpdateDone())
		ConExecute("cg_switchmenu(\"extra_content\")");
}


// --------------------------------------------------------------------
/*
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Live_Content, CMWnd_CubeMenu);
	m_pDescription = GetItem("Description");

bool CMWnd_CubeMenu_Live_Content::GetListItem(int32 &_Index, CStr &_Name, bool _Focus)
{
	if(_Index < m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.m_lspContent.Len())
	{
		wchar wText[1024];
		Localize_Str(m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.m_lspContent[_Index].m_Name, wText, 1023);
		_Name = wText;

		if(_Focus)
		{
			m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = CStrF("%d", _Index);
			Layout_Clear(m_pDescription->GetPosition());

			wchar wText[1024];
			Localize_Str(m_lspContent[_Index].m_Desc.Str(), wText, 1023);
			Layout_WriteText(m_pDescription->GetPosition(), 1, 's', wText, 0);
		}

		_Index++;
		return true;
	}

	return false;
}

void CMWnd_CubeMenu_Live_Content::OnCreate()
{
	CMWnd_CubeMenu::OnCreate();
	m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = CStr("0");
}
*/
// --------------------------------------------------------------------
#ifdef XBOX_SUPPORT_DLCONTENT
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Live_DownloadContent, CMWnd_CubeMenu);
void CMWnd_CubeMenu_Live_DownloadContent::OnCreate()
{
	
	int32 i = m_CubeUser.m_pFrontEndMod->m_CurrentContent; //m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0].Val_int();
	m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.ClearDownloadStatus();
	CLiveHandler::CLiveContent *pLiveContent = m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.FindLiveContent(m_CubeUser.m_pGameContextMod->m_ExtraContent.m_lspContent[i]->m_ID);
	if(!pLiveContent)
		SetMenu("live_message", "extra_content,Download error"); // 
	else
		m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.DownloadContent(pLiveContent->m_OfferingID);
	m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "";
	CMWnd_CubeMenu::OnCreate();
}

void CMWnd_CubeMenu_Live_DownloadContent::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{

	if(m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetDownloadStatus() == CLiveHandler::STATUS_ERROR)
	{
		m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "extra_content,Download error";
		ConExecute("cg_submenu(\"live_message\")");
	}
	else if(m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetDownloadStatus() == CLiveHandler::STATUS_SUCCESS)
	{
		m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "";
		MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
		if(pCon)
			ConExecute("cg_switchmenu(\"live_download_ok\")");
	}
	else
	{
		CRct Rct;
		Rct.p0.x = 0;
		Rct.p0.y = 9;
		Rct.p1.x = 19;
		Rct.p1.y = 11;
		Layout_WriteText(Rct, CStrF("hc, %d%%", m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetDownloadProgress()),
							CMWnd_CubeLayout::FLAG_NO_CONVERT | CMWnd_CubeLayout::FLAG_CENTER);
	}
	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);
}

#endif


// --------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Friends, CMWnd_CubeMenu);

bool CMWnd_CubeMenu_Friends::OnButtonPress(CMWnd *_pFocus)
{
	CFriendsHandler &rHandler = m_CubeUser.m_pGameContextMod->m_FriendsHandler;
	if(m_CurrentSelected < rHandler.m_lFriends.Len())
	{
		XONLINE_FRIEND &rFriend = rHandler.m_lFriends[m_CurrentSelected];
		rHandler.m_CurrentFriend = rFriend;

		if(rFriend.dwFriendState&XONLINE_FRIENDSTATE_FLAG_RECEIVEDREQUEST || rFriend.dwFriendState&XONLINE_FRIENDSTATE_FLAG_SENTREQUEST)
		{
			// 2.x: the friend is a pending friend
			if(rFriend.dwFriendState&XONLINE_FRIENDSTATE_FLAG_RECEIVEDREQUEST)
			{
				// 2.1: We got an request
				// live_friends_menu_gotrequest
				m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "gotrequest";
				ConExecute("cg_submenu(\"live_friends_takeaction\")");
			}
			else if(rFriend.dwFriendState&XONLINE_FRIENDSTATE_FLAG_SENTREQUEST)
			{
				// 2.2: We have sent an request
				// live_friends_menu_sentrequest
				m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "sentrequest";
				ConExecute("cg_submenu(\"live_friends_takeaction\")");
			}
			else
			{
				// unknown, wtf?
			}
		}
		else
		{
			// 1.x
			if(rFriend.dwFriendState&XONLINE_FRIENDSTATE_FLAG_RECEIVEDINVITE)
			{
				// 1.1: Got game invite
				// live_friends_menu_gotinvite
				m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "gotinvite";
				ConExecute("cg_submenu(\"live_friends_takeaction\")");
			}
			else if(rFriend.dwFriendState&XONLINE_FRIENDSTATE_FLAG_JOINABLE)
			{
				// 1.3: Online joinable friend
				// live_friends_menu_joinable
				m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "joinable";
				ConExecute("cg_submenu(\"live_friends_takeaction\")");
			}
			else
			{
				// 1.2: Offline or Online non-joinable friend
				//live_friends_menu_offline
				m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "offline";
				ConExecute("cg_submenu(\"live_friends_takeaction\")");
			}
		}

		/*
		// take action
		if(rFriend.dwFriendState&XONLINE_FRIENDSTATE_FLAG_RECEIVEDREQUEST)
		{
			m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "r";
			ConExecute("cg_submenu(\"live_friends_reply_request\")");//, "r");
		}
		else if(rFriend.dwFriendState&XONLINE_FRIENDSTATE_FLAG_RECEIVEDINVITE)
		{
			m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "i";
			ConExecute("cg_submenu(\"live_friends_reply_invite\")");//, "i");
		}*/
	}

	return CMWnd_CubeMenu::OnButtonPress(_pFocus);
}


int32 CMWnd_CubeMenu_Friends::GetNumListItems(bool _Drawing)
{
	CFriendsHandler &rHandler = m_CubeUser.m_pGameContextMod->m_FriendsHandler;
	return rHandler.m_lFriends.Len();
}

bool CMWnd_CubeMenu_Friends::GetListItem(int32 _Index, CStr &_Name, bool _Focus)
{
	CFriendsHandler &rHandler = m_CubeUser.m_pGameContextMod->m_FriendsHandler;
	if(_Index >= rHandler.m_lFriends.Len())
		return false;

	_Name = CStrF("n, __%s", rHandler.m_lFriends[_Index].szGamertag);
	
	if(_Focus)
		m_CurrentSelected = _Index;

	return true;
}

void CMWnd_CubeMenu_Friends::AfterItemRender(int32 _Index)
{
	CFriendsHandler &rHandler = m_CubeUser.m_pGameContextMod->m_FriendsHandler;
	if(_Index >= 0 && _Index < rHandler.m_lFriends.Len())
	{
		CRct Rect = GetCurrentListItemRct();

		// general icon
		int32 Icon = 0;
		if(rHandler.m_lFriends[_Index].dwFriendState&XONLINE_FRIENDSTATE_FLAG_RECEIVEDREQUEST)
			Icon = 15*16+9;
		else if(rHandler.m_lFriends[_Index].dwFriendState&XONLINE_FRIENDSTATE_FLAG_SENTREQUEST)
			Icon = 15*16+10;
		else if(rHandler.m_lFriends[_Index].dwFriendState&XONLINE_FRIENDSTATE_FLAG_RECEIVEDINVITE)
			Icon = 15*16+7;
		else if(rHandler.m_lFriends[_Index].dwFriendState&XONLINE_FRIENDSTATE_FLAG_SENTINVITE)
			Icon = 15*16+8;
		else if(rHandler.m_lFriends[_Index].dwFriendState&XONLINE_FRIENDSTATE_FLAG_ONLINE)
			Icon = 15*16+11;

		Layout_SetMapping(Rect.p0.x, Rect.p0.y, Icon, CCellInfo::FLAG_FASTSWITCH);

		// voice icon
		Icon = 0;
		if(rHandler.m_lFriends[_Index].dwFriendState&XONLINE_FRIENDSTATE_FLAG_VOICE)
			Icon = 15*16+6;
		Layout_SetMapping(Rect.p0.x+1, Rect.p0.y, Icon, CCellInfo::FLAG_FASTSWITCH);
	}
}


void CMWnd_CubeMenu_Friends::OnCreate()
{
	m_CurrentSelected = 0;
	if(m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetLoginStatus() == CLiveHandler::STATUS_SUCCESS)
	{
		m_CubeUser.m_pGameContextMod->m_FriendsHandler.Init();
		//m_CubeUser.m_pGameContextMod->m_FriendsHandler.Enumerate();
	}
	else
	{
		m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[1] = "live_friends";
		ConExecute("cg_switchmenu(\"live_accounts\")");
	}


	if(m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.m_MessageFlag)
	{
		m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.m_MessageFlag = false;
		ConExecute(CStr("cg_submenu(\"live_question_gotmessage\")"));
	}

	CMWnd_CubeMenu::OnCreate();
}

void CMWnd_CubeMenu_Friends::OnDestroy()
{
	CMWnd_CubeMenu::OnDestroy();
}

bool CMWnd_CubeMenu_Friends::OnOtherButtonPress(CMWnd *_pFocus, int32 _Button)
{
	if(_Button == SKEY_GUI_BUTTON2)
	{
		m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.SetPresence(
				!m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetPresence(),
				m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GotMic()
			);
	}
	else if(_Button == SKEY_GUI_BUTTON3)
	{
		//m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[1] = "live_friends";
		ConExecute("cg_submenu(\"confirm_logout\")");
		return true;
	}
	return false;
}


void CMWnd_CubeMenu_Friends::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);
	CFriendsHandler &rHandler = m_CubeUser.m_pGameContextMod->m_FriendsHandler;

	// check for errors
	if(m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetLoginStatus() != CLiveHandler::STATUS_SUCCESS)
		ConExecute(CStr("cg_switchmenu(\"%s\")", m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetLoginError()));

	// update presence
	if(m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetPresence())
		m_aButtonDescriptions[2] = CStr("x,§LLIVE_APPEARONLINE");
	else
		m_aButtonDescriptions[2] = CStr("x,§LLIVE_APPEAROFFLINE");

	CMWnd *pDescWnd = GetItem("Description");
	if(pDescWnd && m_CurrentSelected < rHandler.m_lFriends.Len())
	{
		Layout_Clear(pDescWnd->GetPosition(), CCellInfo::FLAG_FASTSWITCH);

		XONLINE_FRIEND &rFriend = rHandler.m_lFriends[m_CurrentSelected];
		CStr Desc;

		// user is online
		if(rFriend.dwFriendState&XONLINE_FRIENDSTATE_FLAG_ONLINE)
		{
			wchar wGame[MAX_TITLENAME_LEN];
			HRESULT Res = XOnlineFriendsGetTitleName(rFriend.dwTitleID, XGetLanguage(), MAX_TITLENAME_LEN, wGame);
			CStr Gamename = wGame;

			bool Truncated = false;
			if(Gamename.Len() > 15+3)
			{
				Truncated = true;
				Gamename = Gamename.Left(15)+CStr("... ").Unicode();
			}
			SetDynamicLocalKey("GAME_NEWGAMENAME", Gamename);
				
			if(rFriend.dwFriendState&XONLINE_FRIENDSTATE_FLAG_PLAYING) // user is playing XXX
			{
				Desc += L"§LMENU_FRIENDS_IS_PLAYING ";
				Desc += Gamename.Unicode();
				if(!Truncated)
					Desc += L". ";
			}
			else
			{
				Desc += L"§LMENU_FRIENDS_ONLINE ";
				Desc += Gamename.Unicode();
				if(!Truncated)
					Desc += L". ";
			}
		}
		else
			Desc += L"§LMENU_FRIENDS_OFFLINE\n";
			
		if(rFriend.dwFriendState&XONLINE_FRIENDSTATE_FLAG_RECEIVEDREQUEST) // someone wanna be a friend
			Desc += L"§LMENU_FRIENDS_REQUEST\n";

		if(rFriend.dwFriendState&XONLINE_FRIENDSTATE_FLAG_RECEIVEDINVITE) // someone want you to play XXX
			Desc += L"§LMENU_FRIENDS_INVITE\n";
		if(rFriend.dwFriendState&XONLINE_FRIENDSTATE_FLAG_SENTREQUEST) // request to be friend
			Desc += L"§LMENU_FRIENDS_SENTREQUEST\n";

			//Desc += "Is playing a game\n";

		/*
		if(rFriend.dwFriendState&XONLINE_FRIENDSTATE_FLAG_PLAYING) // user is playing XXX
		{
			wchar wGame[65];
			XOnlineFriendsGetTitleName(rFriend.dwTitleID, XGetLanguage(), 64, wGame);
			Desc += L"§LMENU_FRIENDS_IS_PLAYING";
			Desc += wGame;
			Desc += L"\n";
		}*/

		// semi intresting
			// if(rFriend.dwFriendState&XONLINE_FRIENDSTATE_FLAG_JOINABLE)
			// if(rFriend.dwFriendState&XONLINE_FRIENDSTATE_FLAG_VOICE)

		// not intresting (we can't send invites)
			// if(rFriend.dwFriendState&XONLINE_FRIENDSTATE_FLAG_INVITEACCEPTED)
			// if(rFriend.dwFriendState&XONLINE_FRIENDSTATE_FLAG_INVITEREJECTED)
			// if(rFriend.dwFriendState&XONLINE_FRIENDSTATE_FLAG_SENTINVITE)

		// 
		wchar wText[1024];
		Localize_Str(Desc, wText, 1023);
		Layout_WriteText(pDescWnd->GetPosition(), 's', wText, CCellInfo::FLAG_FASTSWITCH);
	}
}


// --------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CMWnd_CubeMenu_Friends_TakeAction, CMWnd_CubeMenu);

void CMWnd_CubeMenu_Friends_TakeAction::OnCreate()
{
	if(!m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0].Len())
	{
		ConExecute("cg_prevmenu()");
		return;
	}

	if(m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] == "gotinvite")
	{
		m_Type = EType_GotInvite;
		m_NumButtons = 3; // join, decline, remove
	}
	else if(m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] == "offline")
	{
		m_Type = EType_Offline;
		m_NumButtons = 1; // remove
	}
	else if(m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] == "joinable")
	{
		m_Type = EType_Joinable;
		m_NumButtons = 2; // join, remove
	}
	else if(m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] == "gotrequest")
	{
		m_Type = EType_GotRequest;
		m_NumButtons = 3; // join, decline, block
	}
	else if(m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] == "sentrequest")
	{
		m_Type = EType_SentRequest;
		m_NumButtons = 1; // cancel
	}
	else if(m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] == "confirmblock")
	{
		m_Type = EType_ConfirmBlock;
		m_NumButtons = 2; // yes, cancel
	}
	else if(m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] == "confirmremove")
	{
		m_Type = EType_ConfirmRemove;
		m_NumButtons = 2; // yes, cancel
	}

	CMWnd_CubeMenu::OnCreate();
}

bool CMWnd_CubeMenu_Friends_TakeAction::OnButtonPress(CMWnd *_pFocus)
{
	CFriendsHandler &rHandler = m_CubeUser.m_pGameContextMod->m_FriendsHandler;
	switch(m_Action)
	{
		case EAction_Bail:
			ConExecute("cg_prevmenu()");
			break;
		case EAction_Block:
			XOnlineFriendsAnswerRequest(0, &rHandler.m_CurrentFriend, XONLINE_REQUEST_BLOCK);
			ConExecute("cg_prevmenu()");
			break;
		case EAction_Remove:
			XOnlineFriendsRemove(0, &rHandler.m_CurrentFriend);
			ConExecute("cg_prevmenu()");
			break;
		case EAction_ConfirmBlock: // confirm
			m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "confirmblock";
			ConExecute("cg_switchmenu(\"live_friends_takeaction\")");
			break;
		case EAction_ConfirmRemove: // confirm
			m_CubeUser.m_pFrontEndMod->m_aWindowTransferData[0] = "confirmremove";
			ConExecute("cg_switchmenu(\"live_friends_takeaction\")");
			break;
		case EAction_AcceptInvite:
			XOnlineFriendsAnswerGameInvite(0, &rHandler.m_CurrentFriend, XONLINE_GAMEINVITE_YES);
		case EAction_Join:
			XOnlineFriendsJoinGame(0, &rHandler.m_CurrentFriend);
			ConExecute("cg_switchmenu(\"live_crossgame_join_ingamecheck\")");
			//ConExecute("cg_prevmenu()");
			break;
		case EAction_DeclineInvite:
			XOnlineFriendsAnswerGameInvite(0, &rHandler.m_CurrentFriend, XONLINE_GAMEINVITE_NO);
			ConExecute("cg_prevmenu()");
			break;
		case EAction_AcceptRequest:
			XOnlineFriendsAnswerRequest(0, &rHandler.m_CurrentFriend, XONLINE_REQUEST_YES);
			ConExecute("cg_prevmenu()");
			break;
		case EAction_DeclineRequest:
			XOnlineFriendsAnswerRequest(0, &rHandler.m_CurrentFriend, XONLINE_REQUEST_NO);
			ConExecute("cg_prevmenu()");
			break;
		case EAction_CancelRequest:
			XOnlineFriendsRemove(0, &rHandler.m_CurrentFriend);
			ConExecute("cg_prevmenu()"); // confirm
			break;
	 	default: return false;
	}
	return true; // do stuff
}

int32 CMWnd_CubeMenu_Friends_TakeAction::GetNumListItems(bool _Drawing)
{
	return m_NumButtons;
}

bool CMWnd_CubeMenu_Friends_TakeAction::GetListItem(int32 _Index, CStr &_Name, bool _Focus)
{
	switch(m_Type)
	{
	case EType_GotInvite:
		if(_Index == 0)
		{
			_Name = "nc, §LMENU_FRIENDS_ACCEPTINVITE";
			if(_Focus) m_Action = EAction_AcceptInvite; // join
		}
		else if(_Index == 1)
		{
			_Name = "nc, §LMENU_FRIENDS_DECLINEINVITE";
			if(_Focus) m_Action = EAction_DeclineInvite;
		}
		else if(_Index == 2)
		{
			_Name = "nc, §LMENU_FRIENDS_REMOVEFRIEND";
			if(_Focus) m_Action = EAction_ConfirmRemove;
		}
		break;
	case EType_Offline:
		if(_Index == 0)
		{
			_Name = "nc, §LMENU_FRIENDS_REMOVEFRIEND";
			if(_Focus) m_Action = EAction_ConfirmRemove;
		}
		break;
	case EType_Joinable:
		if(_Index == 0)
		{
			_Name = "nc, §LMENU_FRIENDS_JOIN";
			if(_Focus) m_Action = EAction_Join;
		}
		else if(_Index == 1)
		{
			_Name = "nc, §LMENU_FRIENDS_REMOVEFRIEND";
			if(_Focus) m_Action = EAction_ConfirmRemove;
		}
		break;
	case EType_GotRequest:
		if(_Index == 0)
		{
			_Name = "nc, §LMENU_FRIENDS_ACCEPTREQUEST";
			if(_Focus) m_Action = EAction_AcceptRequest;
		}
		else if(_Index == 1)
		{
			_Name = "nc, §LMENU_FRIENDS_DECLINEREQUEST";
			if(_Focus) m_Action = EAction_DeclineRequest;
		}
		else if(_Index == 2)
		{
			_Name = "nc, §LMENU_FRIENDS_BLOCK";
			if(_Focus) m_Action = EAction_ConfirmBlock;
		}
		break;
	case EType_SentRequest:
		if(_Index == 0)
		{
			_Name = "nc, §LMENU_FRIENDS_CANCELREQUEST";
			if(_Focus) m_Action = EAction_CancelRequest;
		}
		break;
	case EType_ConfirmRemove:
		if(_Index == 0)
		{
			_Name = "nc, §LMENU_NO";
			if(_Focus) m_Action = EAction_Bail;
		}
		else if(_Index == 1)
		{
			_Name = "nc, §LMENU_YES";
			if(_Focus) m_Action = EAction_Remove;
		}
		break;
	case EType_ConfirmBlock:
		if(_Index == 0)
		{
			_Name = "nc, §LMENU_NO";
			if(_Focus) m_Action = EAction_Bail;
		}
		else if(_Index == 1)
		{
			_Name = "nc, §LMENU_YES";
			if(_Focus) m_Action = EAction_Block;
		}
		break;
	}
	return true;
}

void CMWnd_CubeMenu_Friends_TakeAction::OnPaint(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client)
{
	// check for errors
	if(m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetLoginStatus() != CLiveHandler::STATUS_SUCCESS)
		ConExecute(CStr("cg_switchmenu(\"%s\")", m_CubeUser.m_pGameContextMod->m_ExtraContent_Live.GetLoginError()));

	//
	CMWnd *pDesc = GetItem("DESC");
	if(pDesc)
	{
		CRct Pos = pDesc->GetPosition();

		if(m_Type == EType_ConfirmRemove)
		{
			Layout_WriteText(Pos, CStr("n, §LMENU_ASKREMOVEFRIEND"));
		}
		else if(m_Type == EType_ConfirmBlock)
		{
			Layout_WriteText(Pos, CStr("n, §LMENU_ASKBLOCKFRIEND"));
		}
	}

	CMWnd_CubeMenu::OnPaint(_pRCUtil, _Clip, _Client);
}

// --------------------------------------------------------------------
#endif
