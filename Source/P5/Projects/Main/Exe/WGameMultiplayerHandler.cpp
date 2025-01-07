/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			The MultiplayerHandler

Author:			Roger Mattsson

Copyright:		2006 Starbreeze Studios AB

Contents:		CWGameMultiplayerHandler

Comments:		

History:		
060604:		Created file
\*____________________________________________________________________________________________*/

#include "PCH.h"
#include "WGameMultiplayerHandler.h"
#include "../GameWorld/WFrontEndMod.h"

//#pragma optimize("", off)
//#pragma inline_depth(0)

CWGameMultiplayerHandler::CWGameMultiplayerHandler()
{
	
}

bool CWGameMultiplayerHandler::Initialize(void)
{

	return false;
}

bool CWGameMultiplayerHandler::InitializeTCPSocket(SOCKET *s, uint8 _port_adjust)
{
	return false;
}

void CWGameMultiplayerHandler::ReceiveMessage(void)
{

}

bool CWGameMultiplayerHandler::ReceiveFromSocket(SOCKET *s, bool _bConnectedSocket, int8 _client)
{

	return false;
}

void CWGameMultiplayerHandler::HandleMessage(SLiveMsg *_msg, sockaddr_in _addr, int _client)
{
	int p = 0;
	switch(_msg->Header.m_id) 
	{
	case LIVE_MSG_CONNECTION_REFUSED:
		{
			//XSessionDelete(m_hSession, NULL);
			ShutdownSession(true);
			m_JoinStarted = -1;

			//TODO
			//Display error msg
			//ConExecute("cg_submenu(\"Multiplayer\")");
		}
		break;
	case LIVE_MSG_CLIENT_DROPPED:
		{
			int player_id;
			player_id = _msg->m_Data.GetInt32(p);

			ClientDropped(-1, player_id);
		}
		break;
	case LIVE_MSG_START_GAME:
		{
			StartGame();
		}
		break;
	case LIVE_MSG_FAKE_PLAYER_ID:
		{
			int player_id = _msg->m_Data.GetInt32(p);
			if(m_Flags & MP_FLAGS_LANGAME)
				m_Local.m_player_id = player_id;
		}
		break;
	case LIVE_MSG_TEAM_ID:
		{
			int player_id = _msg->m_Data.GetInt32(p);
			int team_id = _msg->m_Data.GetInt32(p);
			SetTeam(player_id, team_id);
		}
		break;
	case LIVE_MSG_GAME_OVER:
		{
			ShutdownSession();
			ConExecute("removepadlock(); cg_switchmenu(\"mainmenu_back_fork\")");
		}
		break;
	case LIVE_MSG_MATCH_OVER:
		{
			MatchOver();
		}
		break;
	case LIVE_MSG_HEARTBEAT:
		{
			if(m_Flags & MP_FLAGS_HOST)
			{
				if(!(m_Flags & MP_FLAGS_LOADING))
				{
					CMTime CpuTime = CMTime::GetCPU();
					if(m_LastHeartBeatReceived[_client].GetTime() < CpuTime.GetTime())
						m_LastHeartBeatReceived[_client] = CpuTime;
					SLiveMsg HeartBeat;
					HeartBeat.Header.m_id = LIVE_MSG_HEARTBEAT;
					SendMessage(&HeartBeat, _client);
				}
			}
			else
			{
				CMTime CpuTime = CMTime::GetCPU();
				if(m_LastHeartBeatFromServer.GetTime() < CpuTime.GetTime())
					m_LastHeartBeatFromServer = CpuTime;
			}
		}
		break;
	case LIVE_MSG_READY:
		{
			int player_id = _msg->m_Data.GetInt32(p);
			if(m_Flags & MP_FLAGS_HOST)
			{
				bool bAllReady = true;
				for(int i = 0; i < m_lClients.Len(); i++)
				{
					if(m_lClients[i].m_player_id == player_id)
					{
						if(m_lClients[i].m_bReady)
							m_lClients[i].m_bReady = false;
						else
							m_lClients[i].m_bReady = true;

						SLiveMsg ReadyMsg;
						ReadyMsg.Header.m_id = LIVE_MSG_READY;
						ReadyMsg.m_Data.AddInt32(player_id);
						SendMessageAll(&ReadyMsg);
					}
					if(m_lClients[i].m_player_id && !m_lClients[i].m_bReady)
						bAllReady = false;
				}
				if(bAllReady && m_Local.m_bReady)
				{
					m_bAllReady = true;

					SLiveMsg AllReady;
					AllReady.Header.m_id = LIVE_MSG_READY_ALL;
					SendMessageAll(&AllReady);

					StartGame();
				}
			}
			else
			{
				for(int i = 0; i < m_lClients.Len(); i++)
				{
					if(m_lClients[i].m_player_id == player_id)
					{
						if(m_lClients[i].m_bReady)
							m_lClients[i].m_bReady = false;
						else
							m_lClients[i].m_bReady = true;
					}
				}
				if(m_Local.m_player_id == player_id)
				{
					if(m_Local.m_bReady)
						m_Local.m_bReady = false;
					else
						m_Local.m_bReady = true;
				}
			}
		}
		break;
	case LIVE_MSG_READY_ALL:
		{
			m_bAllReady = true;
		}
		break;
	case LIVE_MSG_GAME_IN_PROGRESS:
		{
			m_Flags |= MP_FLAGS_INPROGRESS;
		}
		break;
	case LIVE_MSG_HEARTBEAT_DELAY:
		{
			if(m_Flags & MP_FLAGS_HOST)
				m_LastHeartBeatReceived[_client] = CMTime::GetCPU() + CMTime::CreateFromSeconds(101.0f);
			else
				m_LastHeartBeatFromServer = CMTime::GetCPU() + CMTime::CreateFromSeconds(101.0f);
		}
		break;
	case LIVE_MSG_QUIT:
		{
			int player_id = _msg->m_Data.GetInt32(p);
			ClientDropped(-1, player_id);
		}
		break;
	default:
		{
			M_TRACEALWAYS("Unknown packet!\n");
		}
		break;
	}
}

bool CWGameMultiplayerHandler::InitializeLAN(void)
{
	return false;
}

void CWGameMultiplayerHandler::ShutdownLAN(void)
{

}

bool CWGameMultiplayerHandler::SearchForSessions(void)
{
	return false;
}

void CWGameMultiplayerHandler::CreateSession(void)
{

}

void CWGameMultiplayerHandler::Quickgame(void)
{

}

void CWGameMultiplayerHandler::Customgame(uint8 _CreateSession)
{

}

void CWGameMultiplayerHandler::Shutdown(void)
{

}

void CWGameMultiplayerHandler::ShutdownSession(bool _NoReport, bool _SendFailed)
{

}

void CWGameMultiplayerHandler::MatchOver(void)
{

}

void CWGameMultiplayerHandler::Update(void)
{
	if(!m_ConnectToServer.IsInvalid())
	{
		CMTime CpuTime = CMTime::GetCPU();
		CMTime Compare = m_ConnectToServer + CMTime::CreateFromSeconds(20.0f);	
		if(Compare.GetTime() < CpuTime.GetTime())
		{
			Connect();
			m_ConnectToServer.MakeInvalid();
		}
	}

	if(!m_JoinRequestTimeOut.IsInvalid())
	{
		CMTime CpuTime = CMTime::GetCPU();
		CMTime Compare = m_JoinRequestTimeOut + CMTime::CreateFromSeconds(15.0f);
		if(Compare.GetTime() < CpuTime.GetTime())
		{
			M_TRACEALWAYS("Join request timed out, shutting down session and starting a custom game\n");
			ShutdownSession(true, true);
			Customgame(1);
			m_JoinRequestTimeOut.MakeInvalid();
		}
	}

	if(m_JoinStarted != -1)
	{
		MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
		if((m_JoinStarted + RoundToInt(15.0f * pGame->m_spWServer->GetGameTicksPerSecond())) < pGame->m_spWServer->GetGameTick())
		{	//join request timed out
			ShutdownSession(true);
			m_JoinStarted = -1;
			//TODO
			//go back to search menu?
		}
	}

	if(!(m_Flags & MP_FLAGS_HOST))
	{
		if((m_Flags & MP_FLAGS_SESSIONVALID) && m_Local.m_bConnected)
		{
			CMTime CpuTime = CMTime::GetCPU();
			CMTime Compare = m_LastHeartBeatSent + CMTime::CreateFromSeconds(1.0f);
			if(Compare.GetTime() < CpuTime.GetTime())
			{
				m_LastHeartBeatSent = CpuTime.GetCPU();
				SLiveMsg Msg;
				Msg.Header.m_id = LIVE_MSG_HEARTBEAT;
				SendMessage(&Msg);
			}

			Compare = m_LastHeartBeatFromServer + CMTime::CreateFromSeconds(16.0f);
			if(!(m_Flags & MP_FLAGS_LOADING) && !(m_Flags & MP_FLAGS_INPROGRESS) && Compare.GetTime() < CpuTime.GetTime())
			{	//We lost the server
				ShutdownSession();
				if(!(m_Flags & MP_FLAGS_LOADING) && !(m_Flags & MP_FLAGS_INPROGRESS))
					ConExecute("cg_prevmenu()");
				M_TRACEALWAYS("Server timed out");
			}
		}
	}

	ReceiveMessage();
}

void CWGameMultiplayerHandler::ShowGamerCard(int _player_id)
{

}

void CWGameMultiplayerHandler::ReadLeaderBoard(int32 _board, int _player_id)
{

}

void CWGameMultiplayerHandler::ReadLeaderBoardFriends(void)
{

}

void CWGameMultiplayerHandler::CleanupStats(void)
{

}

void CWGameMultiplayerHandler::OnClientConnect(int _iClient)
{
	m_lClients[_iClient].m_bConnected = true;

	SetTeam(m_lClients[_iClient].m_player_id, m_lClients[_iClient].m_team_id);

	{
		SLiveMsg Msg;
		Msg.Header.m_id = LIVE_MSG_GAME_MODE;
		Msg.m_Data.AddInt32(m_GameMode);
		Msg.m_Data.AddInt32(m_GameSubMode);
		Msg.m_Data.AddInt32(m_GameStyle);
		Msg.m_Data.AddInt32(GetMapID());
		SendMessage(&Msg, _iClient);
	}

	{
		//Tell the client it's team, this will be overwritten later if it's TDM or CTF
		SLiveMsg Msg;
		Msg.Header.m_id = LIVE_MSG_TEAM_ID;
		Msg.m_Data.AddInt32(m_lClients[_iClient].m_player_id);
		Msg.m_Data.AddInt32(m_lClients[_iClient].m_team_id);
		SendMessage(&Msg, _iClient);
	}

	{
		SLiveMsg Msg;
		Msg.Header.m_id = LIVE_MSG_PLAYERID;
		Msg.m_Data.AddInt32(m_lClients[_iClient].m_player_id);
		SendMessage(&Msg, _iClient);
	}

	if(m_Flags & MP_FLAGS_INPROGRESS)
	{
		SLiveMsg Msg;
		Msg.Header.m_id = LIVE_MSG_GAME_IN_PROGRESS;
		SendMessage(&Msg, _iClient);
	}

	{
		SLiveMsg Msg;
		Msg.Header.m_id = LIVE_MSG_CONNECTION_ACCEPTED;
		SendMessage(&Msg, _iClient);
	}

	for(int ii = 0; ii < m_lClients.Len(); ii++)
	{
		SLiveMsg old_client;
		old_client.Header.m_id = LIVE_MSG_NEWCLIENT;

		bool is_old_client = false;
		if((m_lClients[ii].m_player_id != m_lClients[_iClient].m_player_id))
			is_old_client = true;

		if(is_old_client && m_lClients[ii].m_player_id)
		{
			old_client.m_Data.AddData(&m_lClients[ii], sizeof(ClientInfo));
			//Tell the new about the old ones
			SendMessage(&old_client, _iClient);
		}
	}

	//And the server client
	{
		SLiveMsg old_client;
		old_client.Header.m_id = LIVE_MSG_NEWCLIENT;
		old_client.m_Data.AddData(&m_Local, sizeof(ClientInfo));
		SendMessage(&old_client, _iClient);	
	}

	if(m_Flags & MP_FLAGS_LANGAME)
	{
		SLiveMsg Answer;
		Answer.Header.m_id = LIVE_MSG_FAKE_PLAYER_ID;
		Answer.m_Data.AddInt32(m_lClients[_iClient].m_player_id);
		SendMessage(&Answer, _iClient);
	}
}

void CWGameMultiplayerHandler::Connect()
{

}

bool CWGameMultiplayerHandler::AddUserToSession(ClientInfo* _pClient)
{

	return false;
}

int CWGameMultiplayerHandler::GetMapID(void)
{
	if(!m_Map.Compare("Mp01"))
		return CONTEXT_MAP_MP01;
	else if(!m_Map.Compare("Mp02"))
		return CONTEXT_MAP_MP02;
	else if(!m_Map.Compare("Mp03"))
		return CONTEXT_MAP_MP03;
	else if(!m_Map.Compare("ctf01"))
		return CONTEXT_MAP_CTF01;
	else if(!m_Map.Compare("ctf02")) 
		return CONTEXT_MAP_CTF02;
	M_TRACEALWAYS(CStrF("Map not defined: %s!\n", m_Map.Str()));
	return -1;
}

CStr CWGameMultiplayerHandler::GetMapName(int _MapID)
{
	CStr Map;
	switch(_MapID)
	{
	case CONTEXT_MAP_MP01:
		Map = "Mp01";
		break;
	case CONTEXT_MAP_MP02:
		Map = "Mp02";
		break;
	case CONTEXT_MAP_MP03:
		Map = "Mp03";
		break;
	case CONTEXT_MAP_CTF01:
		Map = "ctf01";
	    break;
	case CONTEXT_MAP_CTF02:
		Map = "ctf02";
	    break;
	}
	return Map;
}

CStr CWGameMultiplayerHandler::GetRandomMap(void)
{
	bool bOnlyCTF = false;
	if(m_GameMode == CONTEXT_GAME_MODE_TEAM_GAMES && m_GameSubMode == CONTEXT_GAME_SUB_MODE_CAPTURE_THE_FLAG)
		bOnlyCTF = true;

	int m = 0;
	if(!bOnlyCTF)
		m = MRTC_RAND() % 3;
	else
		m = MRTC_RAND() % 2 + 3;
	if(m == 0)
		return CStr("Mp01");
	else if(m == 1)
		return CStr("Mp02");
	else if(m == 2)
		return CStr("Mp03");
	else if(m == 3)
		return CStr("ctf01");
	else if(m == 4) 
		return CStr("ctf02");
	return CStr("ctf02");
}

void CWGameMultiplayerHandler::SetTeam(int _player_id, int _team)
{
	if(m_Flags & MP_FLAGS_HOST)
	{
		SLiveMsg LiveMsg;
		LiveMsg.Header.m_id = LIVE_MSG_TEAM_ID;
		LiveMsg.m_Data.AddInt32(_player_id);
		LiveMsg.m_Data.AddInt32(_team);

		SendMessageAll(&LiveMsg);
	}
}

bool CWGameMultiplayerHandler::CheckLoggedInAndMultiplayerRights(void)
{

	return true;
}

int CWGameMultiplayerHandler::GetNumLanServers(void)
{

	return 0;
}

CStr CWGameMultiplayerHandler::GetLanServerInfo(int _iNum)
{
	CStr empty;
	return empty;
}

void CWGameMultiplayerHandler::ShowFriends(void)
{

}

void CWGameMultiplayerHandler::StatisticsSetDeaths(int _value, int _player_id)
{

}

void CWGameMultiplayerHandler::StatisticsSetScore(int _value, int _player_id)
{

}

void CWGameMultiplayerHandler::StatisticsSetCaptures(int _value, int _player_id)
{

}

void CWGameMultiplayerHandler::StartGame(void)
{
	if(!(m_Flags & MP_FLAGS_SESSIONVALID))
		return;

	if(m_Flags & MP_FLAGS_HOST)
	{
		m_Flags |= MP_FLAGS_SESSIONLOCKED;
		m_Flags |= MP_FLAGS_INPROGRESS;
		m_Flags |= MP_FLAGS_LOADING;

		for(int i = 0; i < m_lClients.Len(); i++)
		{
			if(m_lClients[i].m_player_id && m_lClients[i].m_dropped == -1)
			{
				if(m_lClients[i].m_bConnected)
				{
					CMTime CpuTime = CMTime::GetCPU();
					m_LastHeartBeatReceived[i] = CpuTime + CMTime::CreateFromSeconds(101.0f);	//give client plenty of time
				}
			}
		}

		SLiveMsg Msg;
		Msg.Header.m_id = LIVE_MSG_START_GAME;
		SendMessageAll(&Msg);

		ConExecute("cg_clearmenus()");

		CStr cmd = CStrF("changemap(\"%s\", 16); deferredscript(\"list()\")", m_Map.Str());
		ConExecute(cmd);
	}
	else
	{
		CMTime CpuTime = CMTime::GetCPU();
		m_LastHeartBeatFromServer = CpuTime + CMTime::CreateFromSeconds(101.0f);

		if(!(m_Flags & MP_FLAGS_INPROGRESS))
			m_ConnectToServer = CpuTime;
		else
			m_ConnectToServer = CpuTime - CMTime::CreateFromSeconds(10.0f);

		m_Flags |= MP_FLAGS_INPROGRESS;
		m_Flags |= MP_FLAGS_LOADING;
	}
}


void CWGameMultiplayerHandler::LoadingDone(void)
{
	if(!(m_Flags & MP_FLAGS_RANKED))
		m_Flags &= ~MP_FLAGS_SESSIONLOCKED;

	m_Flags |= MP_FLAGS_INPROGRESS;
	m_Flags |= MP_FLAGS_GAMESTARTED;
	m_Flags &= ~MP_FLAGS_LOADING;
}

void CWGameMultiplayerHandler::PrepareMatchOver(void)
{
	if(m_Flags & MP_FLAGS_HOST)
	{
		SLiveMsg Msg;
		Msg.Header.m_id = LIVE_MSG_MATCH_OVER;

		SendMessageAll(&Msg);
		MatchOver();
	}
}

void CWGameMultiplayerHandler::ToggleReadyStatus(void)
{
	if(m_bAllReady)
		return;

	if(m_Flags & MP_FLAGS_HOST)
	{
		m_Local.m_bReady = !m_Local.m_bReady;

		bool bAllReady = true;
		for(int i = 0; i < m_lClients.Len(); i++)
		{
			if(m_lClients[i].m_player_id && !m_lClients[i].m_bReady)
				bAllReady = false;
		}
		if(bAllReady && m_Local.m_bReady)
		{
			m_bAllReady = true;

			SLiveMsg AllReady;
			AllReady.Header.m_id = LIVE_MSG_READY_ALL;
			SendMessageAll(&AllReady);

			StartGame();
		}
	}

	if((m_Flags & MP_FLAGS_INPROGRESS) && !(m_Flags & MP_FLAGS_HOST))
	{
		SLiveMsg Msg;
		Msg.Header.m_id = LIVE_MSG_HEARTBEAT_DELAY;
		SendMessage(&Msg);

		StartGame();
		return;
	}

	SLiveMsg Msg;
	Msg.Header.m_id = LIVE_MSG_READY;
	Msg.m_Data.AddInt32(m_Local.m_player_id);

	if(m_Flags & MP_FLAGS_HOST)
		SendMessageAll(&Msg);
	else
		SendMessage(&Msg);
}

int CWGameMultiplayerHandler::GetPlayerID(int _player_index)
{
	if(_player_index != -1)
		return m_lClients[_player_index].m_player_id;
	return m_Local.m_player_id;
}

int CWGameMultiplayerHandler::GetNumPlayers()
{
	return m_lClients.Len();
}

bool CWGameMultiplayerHandler::IsTalking(int _player_index)
{

	return false;
}

CStr CWGameMultiplayerHandler::GetGameModeName(void)
{
	CStr Name;
	bool bDoSubStyle = false;
	switch(m_GameStyle)
	{
	case CONTEXT_GAME_STYLE_SHAPESHIFTER:
		Name = "§LMENU_SHAPESHIFTER";
		bDoSubStyle = true;
		break;
	case CONTEXT_GAME_STYLE_DARKLINGS_VS_DARKLINGS:
		Name = "§LMENU_DARKLINGS_VS_DARKLINGS";
		bDoSubStyle = true;
		break;
	case CONTEXT_GAME_STYLE_DARKLINGS_VS_HUMANS:
		Name = "§LMENU_DVH";
		break;
	case CONTEXT_GAME_STYLE_SURVIVOR:
		Name = "§LMENU_SURVIVOR";
	    break;
	case CONTEXT_GAME_STYLE_LASTHUMAN:
		Name = "§LMENU_LASTHUMAN";
		break;
	}

	if(bDoSubStyle)
	{
		if(m_GameSubMode == CONTEXT_GAME_SUB_MODE_CAPTURE_THE_FLAG)
			Name = CStrF("%s - §LMENU_CTF", Name.Str());
		else
		{
			if(m_GameMode == CONTEXT_GAME_MODE_FREE_FOR_ALL)
				Name = CStrF("%s - §LMENU_DM", Name.Str());
			else
				Name = CStrF("%s - §LMENU_TDM", Name.Str());
		}
	}

	return Name;
}

void CWGameMultiplayerHandler::SetNextGameMode(void)
{
	switch(m_GameStyle)
	{
	case CONTEXT_GAME_STYLE_SHAPESHIFTER:
	case CONTEXT_GAME_STYLE_DARKLINGS_VS_DARKLINGS:
		if(!m_GameMode)
			m_GameMode++;
		else
		{
			if(!m_GameSubMode)
				m_GameSubMode++;
			else
			{
				m_GameMode = CONTEXT_GAME_MODE_FREE_FOR_ALL;
				m_GameSubMode = CONTEXT_GAME_SUB_MODE_TEAM_DEATHMATCH;
				m_GameStyle++;
			}
		}
		break;
	case CONTEXT_GAME_STYLE_SURVIVOR:
		m_GameStyle++;		
		break;
	case CONTEXT_GAME_STYLE_DARKLINGS_VS_HUMANS:
		m_GameStyle++;
		break;
	case CONTEXT_GAME_STYLE_LASTHUMAN:
		m_GameStyle = 0;		
		break;
	}

	//Set correct game class and make sure we get the correct GameMode and SubMode
	switch(m_GameStyle)
	{
	case CONTEXT_GAME_STYLE_SHAPESHIFTER:
	case CONTEXT_GAME_STYLE_DARKLINGS_VS_DARKLINGS:
		if(m_GameMode == CONTEXT_GAME_MODE_FREE_FOR_ALL)
			ConExecute("setgameclass(\"DM\")");
		else
		{
			if(m_GameSubMode == CONTEXT_GAME_SUB_MODE_TEAM_DEATHMATCH)
				ConExecute("setgameclass(\"TDM\")");
			else
				ConExecute("setgameclass(\"CTF\")");
		}
		
		break;
	case CONTEXT_GAME_STYLE_SURVIVOR:
		ConExecute("setgameclass(\"Survivor\")");
		break;
	case CONTEXT_GAME_STYLE_DARKLINGS_VS_HUMANS:
		ConExecute("setgameclass(\"CTF\")");
		break;
	case CONTEXT_GAME_STYLE_LASTHUMAN:
		ConExecute("setgameclass(\"LastHuman\")");
		break;
	}

	SLiveMsg Msg;
	Msg.Header.m_id = LIVE_MSG_GAME_MODE;
	Msg.m_Data.AddInt32(m_GameMode);
	Msg.m_Data.AddInt32(m_GameSubMode);
	Msg.m_Data.AddInt32(m_GameStyle);
	Msg.m_Data.AddInt32(GetMapID());
	SendMessageAll(&Msg);
}

void CWGameMultiplayerHandler::SetNextMap(void)
{
	m_Map = GetMapName((GetMapID() + 1) % 4);

	SLiveMsg Msg;
	Msg.Header.m_id = LIVE_MSG_GAME_MODE;
	Msg.m_Data.AddInt32(m_GameMode);
	Msg.m_Data.AddInt32(m_GameSubMode);
	Msg.m_Data.AddInt32(m_GameStyle);
	Msg.m_Data.AddInt32(GetMapID());
	SendMessageAll(&Msg);
}

int CWGameMultiplayerHandler::GetPlayerLobbyStatus(int _player_index)
{
	int status = 0;
	if(_player_index != -1)
	{
		if(m_lClients[_player_index].m_bReady)
			status |= LOBBY_PLAYER_READY;
		if(m_lClients[_player_index].m_bVoice)
		{
			if(IsTalking(_player_index))
				status |= LOBBY_PLAYER_TALKING;
			else
				status |= LOBBY_PLAYER_VOICE;
		}
		return status;
	}
	if(m_Local.m_bReady)
		status |= LOBBY_PLAYER_READY;
	if(m_Local.m_bVoice)
	{
		if(IsTalking(_player_index))
			status |= LOBBY_PLAYER_TALKING;
		else
			status |= LOBBY_PLAYER_VOICE;
	}

	return status;
}

CStr CWGameMultiplayerHandler::GetPlayerName(int _player_index)
{
	if(_player_index != -1)
		return m_lClients[_player_index].m_gamertag;
	return m_Local.m_gamertag;
}

void CWGameMultiplayerHandler::Broadcast(void)
{

}

bool CWGameMultiplayerHandler::SendMessage(SLiveMsg *_msg, int8 _client, bool _broad, bool _voice)
{

	return false;
}

void CWGameMultiplayerHandler::ClientDropped(int8 _client, int _player_id)
{
	
}

void CWGameMultiplayerHandler::SendMessageAll(SLiveMsg *_msg)
{
	for(int i = 0; i < m_lClients.Len(); i++)
	{
		if(m_lClients[i].m_dropped == -1)
		{
			if(_msg->Header.m_id == LIVE_MSG_VOICE)
				SendMessage(_msg, i, false, true);
			else
				SendMessage(_msg, i);
		}
	}
}

bool CWGameMultiplayerHandler::UpdateMenu(void)
{
	if(m_WantedMenu.Len())
	{
		MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
		CWFrontEnd_Mod *pFrontEndMod = TDynamicCast<CWFrontEnd_Mod>(pGame->m_spFrontEnd.p);
		CStr CurrentMenu = pFrontEndMod->CurrentMenu();
		if(CurrentMenu.IsEmpty())
		{
			Shutdown();
			ConExecute("removepadlock(); disconnect(); cg_rootmenu(\"MainMenu\")");
			m_WantedMenu = "";
			return true;
		}
		if(m_WantedMenuCount < 10 && pFrontEndMod && m_WantedMenu.CompareNoCase(pFrontEndMod->CurrentMenu()))
		{
			M_TRACEALWAYS(CStrF("Trying to go to %s current menu: %s\n", m_WantedMenu.Str(), pFrontEndMod->CurrentMenu().Str()));
			ConExecute("cg_prevmenu();");
			m_WantedMenuCount++;
			return true;
		}
		else
		{
			m_WantedMenuCount = 0;
			m_WantedMenu = "";
		}
	}
	return false;
}

