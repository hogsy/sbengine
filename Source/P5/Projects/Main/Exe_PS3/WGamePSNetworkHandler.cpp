/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			The PlayStation Network handler

Author:			Roger Mattsson

Copyright:		2006 Starbreeze Studios AB

Contents:		CWGamePSNetworkHandler

Comments:

History:		
060606:		Created file
\*____________________________________________________________________________________________*/

#include "PCH.h"
#include "WGamePSNetworkHandler.h"
#include <cell/sysmodule.h>

#include "../Exe_Xenon/Darkness.spa.h"

CWGamePSNetworkHandler::CWGamePSNetworkHandler()
{
	m_MaxPrivatePlayers		= 1;
	m_MaxPublicPlayers		= 7;
	m_TeamID				= -1;

	m_LastHeartBeatSent		= CMTime::GetCPU();
	m_LastHeartBeatFromServer = CMTime::GetCPU();
	m_WaitOnConnectionTimer.MakeInvalid();

	//	m_GameType				= X_CONTEXT_GAME_TYPE_STANDARD;
	m_GameMode				= CONTEXT_GAME_MODE_FREE_FOR_ALL;
	m_GameStyle				= CONTEXT_GAME_STYLE_SHAPESHIFTER;
	m_GameSubMode			= CONTEXT_GAME_SUB_MODE_TEAM_DEATHMATCH;
	m_ListenSocket			= INVALID_SOCKET;
	m_VDPSocket				= INVALID_SOCKET;
	m_BroadSocket			= INVALID_SOCKET;

	m_Flags	= MP_FLAGS_RANDOMMAP | MP_FLAGS_USEVOICE;

	memset(&m_Local, 0, sizeof(ClientInfo));
	m_Local.m_dropped = -1;
	memset(&m_Host, 0, sizeof(ClientInfo));
	memset(&m_LobbyID, 0, sizeof(SceNpLobbyId));
	memset(&m_RoomID, 0, sizeof(SceNpRoomId));
	memset(&m_TitleID, 0, sizeof(SceNpTitleId));
	memcpy(&m_TitleID.data, "XXXX00026", 9);
	m_Context = 0;

	m_TimeLimit				= 600;
	m_ScoreLimit			= 20;
	m_CaptureLimit			= 3;
	m_JoinStarted			= -1;
	m_FakePlayerID			= 1;
	m_Slots[0]				= 7;
	m_Slots[1]				= 0;
	m_Slots[2]				= 1;
	m_Slots[3]				= 1;
	m_Map					= "";
	m_NextPlayerID			= 1;
	m_ConnectionIDLost		= 0;
	m_bGetGUIData			= false;
	m_bWaitOnConnection		= false;
	m_bConnectionLost		= false;
	m_bAllReady				= false;
	m_pJoinedRoom			= NULL;
	m_pSearchJoinRoom		= NULL;
	m_JoinRequestTimeOut.MakeInvalid();

	m_WantedMenu			= "";
	m_WantedMenuCount		= 0;
}

bool CWGamePSNetworkHandler::Initialize()
{
	int Ret = cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_NP);
	if(Ret != CELL_OK)
	{
		M_TRACEALWAYS("cellSysmoduleLoadModule Failed\n");
		ConExecute("cg_prevmenu()");
		return false;
	}

/*	Ret = cellSysutilInit();
	if (Ret != CELL_OK)
	{
		M_TRACEALWAYS("cellSysutilInit Failed\n");
		ConExecute("cg_prevmenu()");
		return false;
	}*/

	Ret = sceNpInit(NP_POOL_SIZE, m_NPPool);
	if(Ret != CELL_OK)
	{
		M_TRACEALWAYS("sceNpInit Failed\n");
		ConExecute("cg_prevmenu()");
		return false;
	}

	int status;
	Ret = sceNpManagerGetStatus(&status);
	if(Ret != CELL_OK || status != SCE_NP_MANAGER_STATUS_ONLINE)
	{
		M_TRACEALWAYS("PS3 is not online/error\n");
		ConExecute("cg_prevmenu()");
		return false;
	}

	Ret = sceNpBasicInit();
	if(Ret != CELL_OK)
	{
		M_TRACEALWAYS("sceNpBasicInit Failed\n");
		ConExecute("cg_prevmenu()");
		return false;
	}

	//return unknown error code, disabled for now, not critical to have atm
	Ret = sceNpBasicRegisterHandler(&m_TitleID, BasicEventHandlerCB, this);
	if(Ret != CELL_OK)
	{
		M_TRACEALWAYS("sceNpBasicRegisterHandler Failed\n");
		ConExecute("cg_prevmenu()");
		return false;
	}

	Ret = sceNpMatchingInit();
	if(Ret != CELL_OK)
	{
		M_TRACEALWAYS("sceNpMatchingInit Failed\n");
		ConExecute("cg_prevmenu()");
		return false;
	}

	Ret = sceNpManagerGetNpId(&m_Local.m_npid);
	if(Ret != CELL_OK)
	{
		M_TRACEALWAYS("Failed to get NPID\n");
		ConExecute("cg_prevmenu()");
		return false;
	}
	m_Local.m_player_id = SetNPID(m_Local.m_npid);

	Ret = sceNpMatchingCreateCtx(&m_Local.m_npid, &MatchingHandlerCB, this, &m_Context);
	if(Ret != CELL_OK)
	{
		M_TRACEALWAYS("Failed to set Matchinghandler callback\n");
		ConExecute("cg_prevmenu()");
		return false;
	}

	Ret = sceNpSignalingInit();
	if(Ret != CELL_OK)
	{
		M_TRACEALWAYS("sceNpSignalingInit Failed\n");
		ConExecute("cg_prevmenu()");
		return false;
	}

	Ret = sceNpSignalingCreateCtx(&m_Local.m_npid, SignalingHandlerCB, this, &m_SignalingContext);
	if(Ret != CELL_OK)
	{
		M_TRACEALWAYS("sceNpSignalingCreateCtx Failed\n");
		ConExecute("cg_prevmenu()");
		return false;
	}

	SceNpOnlineName Name;
	Ret = sceNpManagerGetOnlineName(&Name);
	if(Ret != CELL_OK)
	{
		M_TRACEALWAYS("Failed to set sceNpManagerGetPsHandle callback\n");
		ConExecute("cg_prevmenu()");
		return false;
	}

	memcpy(m_Local.m_gamertag, Name.data, SCE_NET_NP_ONLINENAME_MAX_LENGTH);

	// Create the socket
	m_VDPSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(m_VDPSocket == INVALID_SOCKET )
	{
		M_TRACEALWAYS("Failed to create vdp socket for Playstation Network\n");
		ConExecute("cg_prevmenu()");
		return false;
	}

	// Bind the socket
	sockaddr_in sa;
	sa.sin_family      = AF_INET;          
	sa.sin_addr.s_addr = INADDR_ANY;        
	sa.sin_port        = htons(SOCKET_VOICE_PORT);

	Ret = bind(m_VDPSocket, (sockaddr*)&sa, sizeof(sa));
	if(Ret != CELL_OK)
	{
		M_TRACEALWAYS("Failed to bind vdp socket for Playstation Network\n");
		ConExecute("cg_prevmenu()");
		return false;
	}

	int block_mode = 1;
	Ret = setsockopt(m_VDPSocket, SOL_SOCKET, SO_NBIO, &block_mode, sizeof(int));
	if(Ret != CELL_OK)
	{
		M_TRACEALWAYS("Failed to make the UDP socket non blocking for Playstion Network\n");
		ConExecute("cg_prevmenu()");
		return false;
	}

	m_Attr.SetLen(10);

	m_Flags |= MP_FLAGS_INITIALIZED;
	M_TRACEALWAYS("PlaystationNetwork is now up and running\n");

/*	SceNpSignalingNetInfo info;
	memset(&info, 0, sizeof(info));
	info.size = sizeof(info);
	Ret = sceNpSignalingGetLocalNetInfo(m_SignalingContext, &info);
	if(Ret != CELL_OK)
		M_TRACEALWAYS("sceNpSignalingGetLocalNetInfo failed\n");
*/
	return true;
}

void CWGamePSNetworkHandler::Shutdown(void)
{
	if(m_Flags & MP_FLAGS_INITIALIZED)
	{
		M_TRACEALWAYS("Shutdown PSN, everything should be cleaned up\n");
		m_Flags &= ~MP_FLAGS_INITIALIZED;
		ShutdownSession();
		ShutdownLAN();

		sceNpMatchingDestroyCtx(m_Context);
		m_Context = 0;
		if(m_VDPSocket && m_VDPSocket != INVALID_SOCKET)
		{
			socketclose(m_VDPSocket);
			m_VDPSocket = NULL;
		}
		sceNpMatchingTerm();
		sceNpBasicUnregisterHandler();
		sceNpBasicTerm();
		sceNpTerm();
		cellSysmoduleUnloadModule(CELL_SYSMODULE_SYSUTIL_NP);
	}
}

bool CWGamePSNetworkHandler::InitializeTCPSocket(SOCKET *s, uint8 _port_adjust)
{
	if(!(*s == NULL || *s == INVALID_SOCKET))
		socketclose(*s);

	// Create the socket
	*s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(*s == INVALID_SOCKET )
	{
		M_TRACEALWAYS("Failed to create local socket for Playstation Network\n");
		return false;
	}

	// Bind the socket
	sockaddr_in sa;
	sa.sin_family      = AF_INET;          
	sa.sin_addr.s_addr = INADDR_ANY;        
	sa.sin_port        = htons(SOCKET_TCP_PORT + _port_adjust);

	if(bind(*s, (sockaddr*)&sa, sizeof(sa)) != 0 )
	{
		int Err = sys_net_errno;
		M_TRACEALWAYS("Failed to bind local socket for Playstation Network\n");
		return false;
	}

	int block_mode = 1;
	if(setsockopt(*s, SOL_SOCKET, SO_NBIO, &block_mode, sizeof(int)))
	{
		M_TRACEALWAYS("Failed to make the local socket non blocking for Playstation Network\n");
		return false;
	}

	return true;
}

bool CWGamePSNetworkHandler::ReceiveFromSocket(SOCKET *s, bool _bConnectedSocket, int8 _client)
{
	sockaddr_in addr;
	addr.sin_family = AF_INET;

	SLiveMsg Msg;

	socklen_t addr_size = sizeof(addr);
	int ret;
	if(_bConnectedSocket)
	{
		//TCP sockets can give us several packets at the same time, so we need to make sure we only get the data we want/can handle at one time
		ret = recv((SOCKET)*s, (char *)&Msg, sizeof(Msg.Header) + sizeof(Msg.m_cbGameData), 0);
		if(ret == (sizeof(Msg.Header) + sizeof(Msg.m_cbGameData)) && Msg.Header.m_size)
		{
			ret = recv((SOCKET)*s, (char *)&Msg.m_Data, Msg.Header.m_size, 0);
			if(ret != Msg.Header.m_size && ret > 0)
			{
				M_TRACEALWAYS("Failed to recv() msg body!\n");
				ret = -1;
			}
		}
	}
	else	
		ret = recvfrom((SOCKET)*s, (char *)&Msg, sizeof(Msg), 0, (sockaddr *)&addr, &addr_size);

	if(ret == 0)
	{
		M_TRACEALWAYS("Connection has been closed by other side\n");
		ShutdownSession();
		if(!(m_Flags & MP_FLAGS_HOST))
			ConExecute("cg_prevmenu()");
		return false;
	}

	if(ret > 0)
	{
		HandleMessage(&Msg, addr, _client);
	}
	else
	{
		int Err = sys_net_errno;
		if(Err != SYS_NET_EWOULDBLOCK)
		{
			M_TRACEALWAYS("Could not receive from Live socket\n");
			if(m_Flags & MP_FLAGS_HOST)
			{
				if(Err == SYS_NET_ECONNRESET)
				{
					M_TRACEALWAYS("Connection to client was lost, not shutting down the session\n");
					return false;
				}
				else
				{
					M_TRACEALWAYS(CStrF("Socket error %i, shutting down the session(server)\n", Err));
					ShutdownSession();
				}
			}
			else
			{
				M_TRACEALWAYS(CStrF("Socket error %i, shutting down the session(client)\n", Err));
				ShutdownSession();
			}
		}
	}
	return true;
}

bool CWGamePSNetworkHandler::SendMessage(SLiveMsg *_msg, int8 _client, bool _broad, bool _voice)
{
	_msg->Header.m_size = _msg->m_Data.GetSize();
	sockaddr_in sa = {0};
	if(_broad)
	{
		sa.sin_addr.s_addr = INADDR_BROADCAST;   
		sa.sin_port = htons(SOCKET_BROAD_PORT);
	}

	sa.sin_family = AF_INET;    

	_msg->m_cbGameData =_msg->Header.m_size + sizeof(_msg->Header);

	int size = _msg->m_cbGameData + sizeof(_msg->m_cbGameData);

	int ret;
	if(_voice)
	{
		//		_msg->m_voice.m_Size = m_LocalDataSize;
		//		size += m_LocalDataSize + sizeof(_msg->m_voice.m_Size);

		if(_msg->Header.m_id == LIVE_MSG_VOICE)	//We use the voice socket to send some none voice msgs (join requests, info we need to send before we are connected)
		{
			_msg->m_cbGameData = sizeof(_msg->Header);
			_msg->Header.m_size = 0;
			size = _msg->m_cbGameData + sizeof(_msg->m_cbGameData) + _msg->m_Data.GetSize();
		}

		if(_client == -1)
			sa.sin_addr = m_Host.m_addr;   
		else
			sa.sin_addr = m_lClients[_client].m_addr;   
		sa.sin_port = htons(SOCKET_VOICE_PORT);

		ret = sendto(m_VDPSocket, (char *)_msg, size, 0, (sockaddr*)&sa, sizeof(sa));
	}
	else if(!_broad)
	{
		if(_client == -1)
			ret = send(m_Host.m_socket, (char *)_msg, size, 0);
		else
			ret = send(m_lClients[_client].m_socket, (char *)_msg, size, 0);
	}
	else
	{
		ret = sendto(m_BroadSocket, (char *)_msg, size, 0, (sockaddr*)&sa, sizeof(sa));
	}

	if(ret != size)	//Something is wrong, kick player
	{
		int Error = sys_net_errno;
		if(_msg->Header.m_id == LIVE_MSG_VOICE)
		{
			M_TRACEALWAYS("Failed to send voice packet");
			return true;
		}
		if(Error == SYS_NET_EBADF && _broad)
		{	//We lost the broad socket somehow, try and create it again
			if(InitializeLAN())
				return true;
		}
		if(m_Flags & MP_FLAGS_HOST)
			ClientDropped(_client);
		else
		{	
			//Connection to host was lost, shut down session and clean up
			M_TRACEALWAYS("Connection to host was lost\n");
			ShutdownSession(false, true);

			//TODO
			//For now, should display an error msg explaining what happened
			m_WantedMenu = "Multiplayer";
		}
		return false;
	}
	return true;
}

bool CWGamePSNetworkHandler::InitializeLAN(void)
{
	if(m_Flags & MP_FLAGS_LANGAME)
		return true;

	m_Flags |= MP_FLAGS_LANGAME;

	m_BroadSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(m_BroadSocket == INVALID_SOCKET )
	{
		M_TRACEALWAYS("Failed to create socket for Playstion Network");
		return false;
	}

	// Bind the socket
	sockaddr_in sa;
	sa.sin_family      = AF_INET;          
	sa.sin_addr.s_addr = INADDR_ANY;        
	sa.sin_port        = htons(SOCKET_BROAD_PORT);

	if(bind(m_BroadSocket, (sockaddr*)&sa, sizeof(sa)) != 0 )
	{
		M_TRACEALWAYS("Failed to bind socket for Playstion Network");
		return false;
	}

	int block_mode = 1;
	if(setsockopt(m_BroadSocket, SOL_SOCKET, SO_NBIO, &block_mode, sizeof(int)))
	{
		M_TRACEALWAYS("Failed to make the socket non blocking for Playstion Network");
		return false;
	}

	int t = 1;
	if(setsockopt(m_BroadSocket, SOL_SOCKET, SO_BROADCAST, &t, sizeof(int)))
	{
		int Error = sys_net_errno;
		M_TRACEALWAYS("Failed to set broadcast on socket\n");
		return false;
	}

	return true;
}

bool CWGamePSNetworkHandler::InitializeVoice(void)
{

	return false;
}

int CWGamePSNetworkHandler::BasicEventHandlerCB(int _event, int _ret_code, uint32_t _req_id, void *_arg)
{
	switch(_event)
	{
	case SCE_NP_BASIC_EVENT_UNKNOWN:
		M_TRACEALWAYS("SCE_NP_BASIC_EVENT_UNKNOWN recieved \n");
		break;
	case SCE_NP_BASIC_EVENT_OFFLINE:
		M_TRACEALWAYS("SCE_NP_BASIC_EVENT_OFFLINE recieved \n");
		break;
	case SCE_NP_BASIC_EVENT_PRESENCE:
		M_TRACEALWAYS("SCE_NP_BASIC_EVENT_PRESENCE recieved \n");
	    break;
	case SCE_NP_BASIC_EVENT_MESSAGE:
		M_TRACEALWAYS("SCE_NP_BASIC_EVENT_MESSAGE recieved \n");
	    break;
	}
	return 1;
}

void CWGamePSNetworkHandler::MatchingHandlerCB(uint32_t _ctx_id, uint32_t _req_id, int _event, int _error_code, void *_arg)
{
	CWGamePSNetworkHandler *pPSN = (CWGamePSNetworkHandler *)_arg;
	switch(_event)
	{
	case SCE_NP_MATCHING_EVENT_ERROR:
		M_TRACEALWAYS("SCE_NP_MATCHING_EVENT_ERROR received\n");
		break;
	case SCE_NP_MATCHING_EVENT_LEAVE_ROOM_DONE:
		M_TRACEALWAYS("SCE_NP_MATCHING_EVENT_LEAVE_ROOM_DONE received\n");
		pPSN->m_Flags &= ~MP_FLAGS_WAITING_ON_CALLBACK;
		break;
	case SCE_NP_MATCHING_EVENT_ROOM_UPDATE_NEW_MEMBER:
		M_TRACEALWAYS("SCE_NP_MATCHING_EVENT_ROOM_UPDATE_NEW_MEMBER received\n");
		break;
	case SCE_NP_MATCHING_EVENT_ROOM_UPDATE_MEMBER_LEAVE:
		M_TRACEALWAYS("SCE_NP_MATCHING_EVENT_ROOM_UPDATE_MEMBER_LEAVE received\n");
		break;
	case SCE_NP_MATCHING_EVENT_ROOM_DISAPPEARED:
		M_TRACEALWAYS("SCE_NP_MATCHING_EVENT_ROOM_DISAPPEARED received\n");
		break;
	case SCE_NP_MATCHING_EVENT_GET_ROOM_INFO_DONE:
		M_TRACEALWAYS("SCE_NP_MATCHING_EVENT_GET_ROOM_INFO_DONE received\n");
		break;
	case SCE_NP_MATCHING_EVENT_SET_ROOM_INFO_DONE:
		M_TRACEALWAYS("SCE_NP_MATCHING_EVENT_SET_ROOM_INFO_DONE received\n");
		break;
	case SCE_NP_MATCHING_EVENT_GRANT_OWNER_DONE:
		M_TRACEALWAYS("SCE_NP_MATCHING_EVENT_GRANT_OWNER_DONE received\n");
		break;
	case SCE_NP_MATCHING_EVENT_ROOM_UPDATE_OWNER_CHANGE:
		M_TRACEALWAYS("SCE_NP_MATCHING_EVENT_ROOM_UPDATE_OWNER_CHANGE received\n");
		break;
	case SCE_NP_MATCHING_EVENT_KICK_MEMBER_DONE:
		M_TRACEALWAYS("SCE_NP_MATCHING_EVENT_KICK_MEMBER_DONE received\n");
		break;
	case SCE_NP_MATCHING_EVENT_ROOM_KICKED:
		M_TRACEALWAYS("SCE_NP_MATCHING_EVENT_ROOM_KICKED received\n");
		break;
	case SCE_NP_MATCHING_EVENT_GET_ROOM_SEARCH_FLAG_DONE:
		M_TRACEALWAYS("SCE_NP_MATCHING_EVENT_GET_ROOM_SEARCH_FLAG_DONE received\n");
		break;
	case SCE_NP_MATCHING_EVENT_SET_ROOM_SEARCH_FLAG_DONE:
		M_TRACEALWAYS("SCE_NP_MATCHING_EVENT_GET_ROOM_SEARCH_FLAG_DONE received\n");
		break;
	}
}

void CWGamePSNetworkHandler::MatchingGUIHandlerCB(uint32_t _ctx_id, int _event, int _error_code, void *_arg)
{
	CWGamePSNetworkHandler *pPSN = (CWGamePSNetworkHandler *)_arg;
	if(_error_code < 0)
	{
		M_TRACEALWAYS("MatchingGUIHandlerCB error \n");
		pPSN->m_Flags &= ~MP_FLAGS_WAITING_ON_CALLBACK;
		pPSN->m_Flags &= ~MP_FLAGS_SEARCHING;
		if(_error_code == SCE_NP_MATCHING_ERROR_SEARCH_JOIN_ROOM_NOT_FOUND)
			pPSN->m_Flags |= MP_FLAGS_SEARCH_FAILED_CREATE_SESSION;
	}
	else
	{
		switch(_event)
		{
		case SCE_NP_MATCHING_GUI_EVENT_CREATE_ROOM:
			{
				M_TRACEALWAYS("SCE_NP_MATCHING_GUI_EVENT_CREATE_ROOM recieved \n");
				pPSN->m_bGetGUIData = true;

				pPSN->m_Flags |= MP_FLAGS_SESSIONVALID;
				pPSN->m_Flags |= MP_FLAGS_HOST;
				pPSN->m_Flags |= MP_FLAGS_GUI_IN_PROGRESS;
				pPSN->m_Flags |= MP_FLAGS_WAITING_ON_CALLBACK;

				pPSN->m_Local.m_player_id = pPSN->SetNPID(pPSN->m_Local.m_npid);	
				pPSN->SetTeam(pPSN->m_Local.m_player_id, ++pPSN->m_TeamID);

				ConExecute(CStr("cg_submenu(\"Multiplayer_StartGame\")"));
			}
			break;
		case SCE_NP_MATCHING_GUI_EVENT_COMMON_LOAD:
			{
				M_TRACEALWAYS("SCE_NP_MATCHING_GUI_EVENT_COMMON_LOAD received\n");
				pPSN->m_Flags |= MP_FLAGS_GUI_IN_PROGRESS;
			}
			break;
		case SCE_NP_MATCHING_GUI_EVENT_COMMON_UNLOAD:
			{
				M_TRACEALWAYS("SCE_NP_MATCHING_GUI_EVENT_COMMON_UNLOAD received\n");
				pPSN->m_Flags &= ~MP_FLAGS_GUI_IN_PROGRESS;
			}
			break;
		case SCE_NP_MATCHING_GUI_EVENT_SEARCH_JOIN:
			{
				M_TRACEALWAYS("SCE_NP_MATCHING_GUI_EVENT_SEARCH_JOIN recieved \n");
				pPSN->m_bGetGUIData = true;
			}
			break;
		}
	}
}

void CWGamePSNetworkHandler::SignalingHandlerCB(uint32_t _ctx_id, uint32_t _subject_id, int _event, int _error_code, void *_arg)
{
	switch(_event)
	{
	case SCE_NP_SIGNALING_EVENT_DEAD:
		{
			M_TRACEALWAYS("SCE_NP_SIGNALING_EVENT_DEAD received\n");
			CWGamePSNetworkHandler *pPSN = (CWGamePSNetworkHandler *)_arg;
			pPSN->m_bConnectionLost = true;
			pPSN->m_ConnectionIDLost = _subject_id;
		}
		break;
	case SCE_NP_SIGNALING_EVENT_ESTABLISHED:
		{
			M_TRACEALWAYS("SCE_NP_SIGNALING_EVENT_ESTABLISHED received\n");
			CWGamePSNetworkHandler *pPSN = (CWGamePSNetworkHandler *)_arg;
			pPSN->m_Flags |= MP_FLAGS_CONNECTION_ESTABLISHED;
		}
		break;
	}
}

void CWGamePSNetworkHandler::ShutdownSession(bool _NoReport, bool _SendFailed)
{
	if(!(m_Flags & MP_FLAGS_SESSIONVALID))
		return;

	m_Flags &= ~MP_FLAGS_SESSIONVALID;
	M_TRACEALWAYS("Session is now invalid(shutdown)\n");

	ClearNPIDs();

	//Tell everyone we are done playing
	if(!_SendFailed)
	{
		if((m_Flags & MP_FLAGS_HOST))
		{
			SLiveMsg Msg;
			Msg.Header.m_id = LIVE_MSG_GAME_OVER;
			SendMessageAll(&Msg);
		}
		else
		{
			SLiveMsg Msg;
			Msg.Header.m_id = LIVE_MSG_QUIT;
			Msg.m_Data.AddInt32(m_Local.m_player_id);

			SendMessage(&Msg);
		}
	}

	CleanupSearchAndRegResults();

	int Ret = sceNpMatchingLeaveRoom(m_Context, &m_RoomID, &m_RequestID);
	if(Ret != CELL_OK)
		M_TRACEALWAYS("sceNpMatchingLeaveRoom failed \n");
	else
		m_Flags |= MP_FLAGS_WAITING_ON_CALLBACK;

	for(int i = 0; i < m_lClients.Len(); i++)
	{
		if((m_Flags & MP_FLAGS_HOST))
		{
			if(m_lClients[i].m_socket)
				socketclose(m_lClients[i].m_socket);
		}
	}
	m_lClients.Destroy();
	m_LastHeartBeatReceived.Destroy();

	m_bAllReady				= false;
	m_Local.m_bPrivateSlot	= false;
	m_Local.m_bInvited		= false;
	m_Local.m_bMuted		= false;
	m_Local.m_bReady		= false;
	m_Local.m_bRegistered	= false;
	m_Local.m_bConnected	= false;
	m_Local.m_bReady		= false;
	m_Local.m_dropped		= -1;
	m_bWaitOnConnection		= false;
	m_Flags &= ~MP_FLAGS_HOST;
	m_Flags &= ~MP_FLAGS_INPROGRESS;
	m_Flags &= ~MP_FLAGS_SESSIONLOCKED;
	m_Flags &= ~MP_FLAGS_GAMESTARTED;
	m_Flags &= ~MP_FLAGS_LOADING;
	m_Flags &= ~MP_FLAGS_GUI_IN_PROGRESS;
	m_Flags &= ~MP_FLAGS_SEARCHING;
	m_Flags &= ~MP_FLAGS_CONNECTION_ESTABLISHED;
	m_Flags &= ~MP_FLAGS_SESSION_STARTED;

	if(m_Host.m_socket && m_Host.m_socket != INVALID_SOCKET)
	{
		socketclose(m_Host.m_socket);
		m_Host.m_socket = NULL;
	}
	if(m_ListenSocket && m_ListenSocket != INVALID_SOCKET)
	{
		socketclose(m_ListenSocket);
		m_ListenSocket = NULL;
	}
}

void CWGamePSNetworkHandler::CleanupSearchAndRegResults(void)
{
	if(m_pJoinedRoom)
	{
		delete[] m_pJoinedRoom;
		m_pJoinedRoom = NULL;
	}
	if(m_pSearchJoinRoom)
	{
		delete[] m_pSearchJoinRoom;
		m_pSearchJoinRoom = NULL;
	}
}

void CWGamePSNetworkHandler::ShutdownLAN(void)
{
	if(m_Flags & MP_FLAGS_LANGAME)
	{
		if(m_BroadSocket)
		{
			socketclose(m_BroadSocket);
			m_BroadSocket = NULL;
		}
		m_Flags &= ~MP_FLAGS_LANGAME;
	}
}

void CWGamePSNetworkHandler::ShutdownVoice(void)
{

}

void CWGamePSNetworkHandler::Update(void)
{
	if(UpdateMenu())
		return;

	if(!(m_Flags & MP_FLAGS_INITIALIZED))
		return;

	if(m_bConnectionLost)
	{
		if(m_Flags & MP_FLAGS_HOST)
		{
			for(int i = 0; i < m_lClients.Len(); i++)	
			{
				if(m_lClients[i].m_player_id && m_lClients[i].m_ConnectionID == m_ConnectionIDLost)
				{
					ClientDropped(i);
					break;
				}
			}
		}
		else
			ShutdownSession();
	}

	if(m_bGetGUIData)
	{
		size_t size = 0;
		int event = 0;

		m_Flags &= ~MP_FLAGS_WAITING_ON_CALLBACK;
		m_Flags &= ~MP_FLAGS_GUI_IN_PROGRESS;

		sceNpMatchingGetResultGUI(NULL, &size, &event);
		switch(event)
		{
		case SCE_NP_MATCHING_GUI_EVENT_CREATE_ROOM:
			if(m_pJoinedRoom)
			{
				delete[] m_pJoinedRoom;
				m_pJoinedRoom = NULL;
			}
			m_pJoinedRoom = (SceNpMatchingJoinedRoomInfo *) DNew(char) char[size];
			sceNpMatchingGetResultGUI(m_pJoinedRoom, &size, &event);
			m_RoomID = m_pJoinedRoom->room_status.id;
			break;
		case SCE_NP_MATCHING_GUI_EVENT_SEARCH_JOIN:
			{
				if(m_pSearchJoinRoom)
				{
					delete[] m_pSearchJoinRoom;
					m_pSearchJoinRoom = NULL;
				}
				m_pSearchJoinRoom = (SceNpMatchingSearchJoinRoomInfo *) DNew(char) char[size];
				sceNpMatchingGetResultGUI(m_pSearchJoinRoom, &size, &event);
				JoinSession(0, false);
				break;
			}
		}
	
		if(!(m_Flags & MP_FLAGS_HOST))
		{
			if(event == SCE_NP_MATCHING_GUI_EVENT_SEARCH_JOIN)
			{
				SceNpMatchingRoomMember *member = m_pSearchJoinRoom->room_status.members;
				for(int i = 0; i < m_pSearchJoinRoom->room_status.num; i++)
				{
					if(member->owner)
					{
						break;
					}
					else
						member = m_pSearchJoinRoom->room_status.members->next;
				}
				
				int Ret = sceNpSignalingActivateConnection(m_SignalingContext, &member->user_info.userId, &m_Host.m_ConnectionID);
				if(Ret != CELL_OK)
				{
					M_TRACEALWAYS("sceNpSignalingActivateConnection to server failed\n");
					ShutdownSession();
				}
				else
				{
					m_bWaitOnConnection = true;
					m_WaitOnConnectionTimer = CMTime::GetCPU();
				}
			}
		}

		m_bGetGUIData = false;
	}

	if(m_bWaitOnConnection)
	{
		CMTime Compare = m_WaitOnConnectionTimer + CMTime::CreateFromSeconds(15.0f);
		if(!m_WaitOnConnectionTimer.IsInvalid() && Compare.GetTime() < CMTime::GetCPU().GetTime())
		{
			ShutdownSession();
		}

		//wait on ESTABLISHED in signalingCB then get adress and send connect request
		if(m_Flags & MP_FLAGS_CONNECTION_ESTABLISHED)
		{
			//First get IP adress
			int ConnectionStatus;
			in_addr ServerAddr;
			in_port_t ServerPort;
			int Ret = sceNpSignalingGetConnectionStatus(m_SignalingContext, m_Host.m_ConnectionID, &ConnectionStatus, &ServerAddr, &ServerPort);
			if(Ret != CELL_OK)
			{
				M_TRACEALWAYS("sceNpSignalingGetConnectionStatus failed!\n");
				ShutdownSession();
				return;
			}

			if(ConnectionStatus == SCE_NP_SIGNALING_CONN_STATUS_ACTIVE)
			{
				m_Host.m_addr = ServerAddr;

				SLiveMsg Msg;
				Msg.Header.m_id = LIVE_MSG_JOIN_REQUEST;
				Msg.m_Data.AddData(&m_Local, sizeof(m_Local));

				if(!SendMessage(&Msg, -1, false, true))
					return;
				m_bWaitOnConnection = false;
			}
		}
	}
	
	cellSysutilCheckCallback();

	CWGameMultiplayerHandler::Update();

	if(m_Flags & MP_FLAGS_SEARCH_FAILED_CREATE_SESSION && !(m_Flags & MP_FLAGS_GUI_IN_PROGRESS))
	{
		m_Flags &= ~MP_FLAGS_SEARCH_FAILED_CREATE_SESSION;
		CreateSession();
	}

	if(!(m_Flags & MP_FLAGS_HOST))
	{
		if(!m_Local.m_bConnected)
		{
			if(m_ListenSocket && m_ListenSocket != INVALID_SOCKET)
			{
				sockaddr_in addr;
				socklen_t size = sizeof(addr);
				m_Host.m_socket = accept(m_ListenSocket, (sockaddr *)&addr, &size);
				if(m_Host.m_socket > 0)
				{
					M_TRACEALWAYS("Connection accepted\n");
					memcpy(&m_HostAddr, &addr.sin_addr, sizeof(m_HostAddr));
					m_Flags |= MP_FLAGS_SESSIONVALID;
					M_TRACEALWAYS("Session is now valid(join request completed)\n");
					m_Local.m_bConnected = true;
					if(m_ListenSocket)
					{
						socketclose(m_ListenSocket);
						m_ListenSocket = NULL;
					}
				}
				else
				{
					int Err = sys_net_errno;
					if(Err != SYS_NET_EWOULDBLOCK)
					{
						M_TRACEALWAYS("Accept() failed\n");
						m_Flags |= MP_FLAGS_SESSIONVALID;	//Make session valid so it will shutdown properly
						M_TRACEALWAYS("Session is now valid(accept failed, session is made valid for proper cleanup)\n");
						ShutdownSession();
					}
				}
			}
		}
	}
	else
	{
		for(int i = 0; i < m_lClients.Len(); i++)
		{
			if(m_lClients[i].m_player_id && m_lClients[i].m_dropped == -1)
			{
				if(m_lClients[i].m_bConnected)
				{
					CMTime CpuTime = CMTime::GetCPU();
					CMTime Compare = m_LastHeartBeatReceived[i] + CMTime::CreateFromSeconds(16.0f);
					if(Compare.GetTime() < CpuTime.GetTime())
					{	
						M_TRACEALWAYS("Client timed out\n");
						ClientDropped(i);
					}
				}
				else
				{
					fd_set write, error;
					timeval time;
					time.tv_usec = 25;
					FD_ZERO(&write);
					FD_ZERO(&error);
					FD_SET(m_lClients[i].m_socket, &write);
					FD_SET(m_lClients[i].m_socket, &error);
					socketselect(NULL, NULL, &write, NULL/*&error*/, &time);

					if(FD_ISSET(m_lClients[i].m_socket, &write))
					{	//We are connected!
						OnClientConnect(i);
					}

			/*		if(FD_ISSET(m_lClients[i].m_socket, &error))
					{
						M_TRACEALWAYS("Error from select\n");
					}*/
				}
			}
		}
	}
}

void CWGamePSNetworkHandler::Broadcast(void)
{

}

bool CWGamePSNetworkHandler::SearchForSessions(void)
{
	if(m_Flags & MP_FLAGS_WAITING_ON_CALLBACK || m_Flags & MP_FLAGS_GUI_IN_PROGRESS)
		return false;

	M_TRACEALWAYS("Searching for session...\n");

	m_SearchCond.SetLen(5);

	//First, make sure the server isn't full
	m_SearchCond[0].target_attr_type = SCE_NP_MATCHING_ATTR_TYPE_BASIC_NUM;
	m_SearchCond[0].target_attr_id = SCE_NP_MATCHING_ROOM_ATTR_ID_TOTAL_SLOT;
	m_SearchCond[0].comp_op = SCE_NP_MATCHING_CONDITION_SEARCH_NE;
	m_SearchCond[0].comp_type = SCE_NP_MATCHING_CONDITION_TYPE_VALUE;
	m_SearchCond[0].compared.type = SCE_NP_MATCHING_ATTR_TYPE_BASIC_NUM;
	m_SearchCond[0].compared.id = SCE_NP_MATCHING_ROOM_ATTR_ID_CUR_TOTAL_NUM;
	m_SearchCond[0].next = &m_SearchCond[1];

	//TODO fix
	//For some reason comparing game attributes in search doesn't work, it results in an undocumented error, leave it for now
	m_SearchCond[0].next = NULL;

	//Can only use 3 extra integer values in search, need to merge m_GameMode, m_GameStyle and m_GameSubMode

	//Now lets check the game mode
	m_SearchCond[1].target_attr_type = SCE_NP_MATCHING_ATTR_TYPE_GAME_NUM;
//	m_SearchCond[1].target_attr_id = CONTEXT_GAME_MODE;
	m_SearchCond[1].comp_op = SCE_NP_MATCHING_CONDITION_SEARCH_EQ;
	m_SearchCond[1].comp_type = SCE_NP_MATCHING_CONDITION_TYPE_VALUE;
	m_SearchCond[1].compared.type = SCE_NP_MATCHING_ATTR_TYPE_GAME_NUM;
	m_SearchCond[1].compared.value.num = m_GameMode;
	m_SearchCond[1].next = &m_SearchCond[2];

	//And then the game style
	m_SearchCond[2].target_attr_type = SCE_NP_MATCHING_ATTR_TYPE_GAME_NUM;
	m_SearchCond[2].target_attr_id = CONTEXT_GAME_STYLE;
	m_SearchCond[2].comp_op = SCE_NP_MATCHING_CONDITION_SEARCH_EQ;
	m_SearchCond[2].comp_type = SCE_NP_MATCHING_CONDITION_TYPE_VALUE;
	m_SearchCond[2].compared.type = SCE_NP_MATCHING_ATTR_TYPE_GAME_NUM;
	m_SearchCond[2].compared.value.num = m_GameStyle;
	m_SearchCond[2].next = &m_SearchCond[3];

	//And then the game sub mode
	m_SearchCond[3].target_attr_type = SCE_NP_MATCHING_ATTR_TYPE_GAME_NUM;
	m_SearchCond[3].target_attr_id = CONTEXT_GAME_SUB_MODE;
	m_SearchCond[3].comp_op = SCE_NP_MATCHING_CONDITION_SEARCH_EQ;
	m_SearchCond[3].comp_type = SCE_NP_MATCHING_CONDITION_TYPE_VALUE;
	m_SearchCond[3].compared.type = SCE_NP_MATCHING_ATTR_TYPE_GAME_NUM;
	m_SearchCond[3].compared.value.num = m_GameSubMode;
	m_SearchCond[3].next = NULL;
	if(m_SearchMethod == SESSION_MATCH_QUERY_EXACT) 
	{
		//And now the map
		m_SearchCond[3].next = &m_SearchCond[4];
		m_SearchCond[4].target_attr_type = SCE_NP_MATCHING_ATTR_TYPE_GAME_NUM;
		m_SearchCond[4].target_attr_id = CONTEXT_MAP;
		m_SearchCond[4].comp_op = SCE_NP_MATCHING_CONDITION_SEARCH_EQ;
		m_SearchCond[4].comp_type = SCE_NP_MATCHING_CONDITION_TYPE_VALUE;
		m_SearchCond[4].compared.type = SCE_NP_MATCHING_ATTR_TYPE_GAME_NUM;
		m_SearchCond[4].compared.value.num = GetMapID();
		m_SearchCond[4].next = NULL;
	}

	m_SearchAttr.SetLen(5);
	memset(m_SearchAttr.GetBasePtr(), 0, m_SearchAttr.ListSize());
	m_SearchAttr[0].type = SCE_NP_MATCHING_ATTR_TYPE_BASIC_NUM;
	m_SearchAttr[0].id = SCE_NP_MATCHING_ROOM_ATTR_ID_TOTAL_SLOT;
	m_SearchAttr[0].next = &m_SearchAttr[1];

	m_SearchAttr[1].type = SCE_NP_MATCHING_ATTR_TYPE_BASIC_NUM;
	m_SearchAttr[1].id = SCE_NP_MATCHING_ROOM_ATTR_ID_PRIVATE_SLOT;
	m_SearchAttr[1].next = &m_SearchAttr[2];

	m_SearchAttr[2].type = SCE_NP_MATCHING_ATTR_TYPE_BASIC_NUM;
	m_SearchAttr[2].id = SCE_NP_MATCHING_ROOM_ATTR_ID_CUR_TOTAL_NUM;
	m_SearchAttr[2].next = &m_SearchAttr[3];

	m_SearchAttr[3].type = SCE_NP_MATCHING_ATTR_TYPE_BASIC_NUM;
	m_SearchAttr[3].id = SCE_NP_MATCHING_ROOM_ATTR_ID_CUR_PUBLIC_NUM;
	m_SearchAttr[3].next = &m_SearchAttr[4];

	m_SearchAttr[4].type = SCE_NP_MATCHING_ATTR_TYPE_BASIC_NUM;
	m_SearchAttr[4].id = SCE_NP_MATCHING_ROOM_ATTR_ID_CUR_PRIVATE_NUM;
	m_SearchAttr[4].next = NULL;

	int Ret = sceNpMatchingSearchJoinRoomGUI(m_Context, &m_TitleID, m_SearchCond.GetBasePtr(), m_SearchAttr.GetBasePtr(), MatchingGUIHandlerCB, this);
	if(Ret != CELL_OK)
	{
		M_TRACEALWAYS("sceNpMatchingSearchJoinRoomGUI failed\n");
		return false;
	}

	m_Flags |= MP_FLAGS_GUI_IN_PROGRESS;
	m_Flags |= MP_FLAGS_WAITING_ON_CALLBACK;
	m_Flags |= MP_FLAGS_SEARCHING;
	
	return true;
}

void CWGamePSNetworkHandler::CreateSession(void)
{
	if(m_Flags & MP_FLAGS_WAITING_ON_CALLBACK || m_Flags & MP_FLAGS_GUI_IN_PROGRESS)
	{
		ConExecute("cg_prevmenu()");
		return;
	}

	M_TRACEALWAYS("Create session\n");

	m_Slots[SLOTS_PUBLIC]			= m_MaxPublicPlayers;	//Max number of public slots
	m_Slots[SLOTS_PUBLIC_FILLED]	= 0;
	m_Slots[SLOTS_PRIVATE_FILLED]	= 1;	//Number of private slots, used for invites(and friends?)
	m_Slots[SLOTS_PRIVATE]			= m_MaxPrivatePlayers;

	m_lClients.SetLen(m_MaxPublicPlayers + m_MaxPrivatePlayers - 1);	//-1 is because the local player will not be in this list
	m_LastHeartBeatReceived.SetLen(m_MaxPublicPlayers + m_MaxPrivatePlayers - 1);
	memset(m_lClients.GetBasePtr(), 0, m_lClients.ListSize());
	memset(m_LastHeartBeatReceived.GetBasePtr(), 0, m_LastHeartBeatReceived.ListSize());

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	CRegistry *pGlobalOpt = pSys->GetRegistry()->FindChild("OPTG");

	m_TimeLimit = pGlobalOpt->GetValuei("MP_TIME") * 60;
	m_ScoreLimit = pGlobalOpt->GetValuei("MP_SCORE");
	m_CaptureLimit = pGlobalOpt->GetValuei("MP_CAPTURE");

	//set all Attr members
	m_Attr[0].id = SCE_NP_MATCHING_ROOM_ATTR_ID_TOTAL_SLOT;
	m_Attr[0].type = SCE_NP_MATCHING_ATTR_TYPE_BASIC_NUM;
	m_Attr[0].value.num = m_MaxPublicPlayers + m_MaxPrivatePlayers;
	m_Attr[0].next = &m_Attr[1];

	m_Attr[1].id = SCE_NP_MATCHING_ROOM_ATTR_ID_PRIVATE_SLOT;
	m_Attr[1].type = SCE_NP_MATCHING_ATTR_TYPE_BASIC_NUM;
	m_Attr[1].value.num = m_MaxPrivatePlayers;
	m_Attr[1].next = &m_Attr[2];

	m_Attr[2].id = SCE_NP_MATCHING_ROOM_ATTR_ID_PRIVILEGE_TYPE;
	m_Attr[2].type = SCE_NP_MATCHING_ATTR_TYPE_BASIC_NUM;
	m_Attr[2].value.num = SCE_NP_MATCHING_ROOM_PRIVILEGE_TYPE_AUTO_GRANT;
	m_Attr[2].next = NULL;//&m_Attr[3];
	//TODO undo
	//For now try without custom data
	m_Attr[3].id = CONTEXT_GAME_MODE;	
	m_Attr[3].type = SCE_NP_MATCHING_ATTR_TYPE_GAME_NUM;
	m_Attr[3].value.num = m_GameMode;
	m_Attr[3].next = &m_Attr[4];

	m_Attr[4].id = CONTEXT_GAME_SUB_MODE;
	m_Attr[4].type = SCE_NP_MATCHING_ATTR_TYPE_GAME_NUM;
	m_Attr[4].value.num = m_GameSubMode;
	m_Attr[4].next = &m_Attr[5];

	m_Attr[5].id = CONTEXT_GAME_STYLE;
	m_Attr[5].type = SCE_NP_MATCHING_ATTR_TYPE_GAME_NUM;
	m_Attr[5].value.num = m_GameStyle;
	m_Attr[5].next = &m_Attr[6];

	m_Attr[6].id = CONTEXT_MAP;
	m_Attr[6].type = SCE_NP_MATCHING_ATTR_TYPE_GAME_NUM;
	m_Attr[6].value.num = GetMapID();
	m_Attr[6].next = NULL;

	int Ret = sceNpMatchingCreateRoomGUI(m_Context, &m_TitleID, m_Attr.GetBasePtr(), MatchingGUIHandlerCB, this);
	if(Ret != CELL_OK)
	{
		M_TRACEALWAYS("sceNpMatchingCreateRoomGUI failed\n");
		ConExecute("cg_prevmenu()");
		return;
	}
}

void CWGamePSNetworkHandler::Quickgame(void)
{
	if(m_Flags & MP_FLAGS_WAITING_ON_CALLBACK)
		return;


}

void CWGamePSNetworkHandler::Customgame(void)
{
	if(m_Flags & MP_FLAGS_WAITING_ON_CALLBACK)
		return;

	if(!(m_Flags & MP_FLAGS_RANDOMMAP))
		m_SearchMethod = SESSION_MATCH_QUERY_EXACT;
	else	
		m_SearchMethod = SESSION_MATCH_QUERY_SIMPLE;

	if(m_Flags & MP_FLAGS_SESSIONVALID)
	{
		ShutdownSession(true);	//Make sure everything is cleaned up
		m_WantedMenu = "";
	}

	if(m_Flags & MP_FLAGS_LANGAME)
		CreateSession();
	else
		SearchForSessions();
}

void CWGamePSNetworkHandler::JoinSession(uint16 _iServer, bool _bFromFriend)
{
	if(m_Flags & MP_FLAGS_WAITING_ON_CALLBACK)
		return;

	M_TRACEALWAYS("Join session\n");

	if(!InitializeTCPSocket(&m_ListenSocket))
	{
		M_TRACEALWAYS("InitializeTCPSocket(&m_ListenSocket) failed\n");
		return;
	}
	listen(m_ListenSocket, 1);

	m_RoomID = m_pSearchJoinRoom->room_status.id;

	int nPlayers = 0;
	SceNpMatchingAttr *attr = m_pSearchJoinRoom->attr;
	while(attr)
	{
		if(attr->id == SCE_NP_MATCHING_ROOM_ATTR_ID_TOTAL_SLOT)
		{
			nPlayers = attr->value.num;
			attr = NULL;
		}
		else
			attr = attr->next;
	}
	
	if(!_bFromFriend && !(m_Flags & MP_FLAGS_LANGAME))
	{
		m_lClients.SetLen(nPlayers);	
		memset(m_lClients.GetBasePtr(), 0, m_lClients.ListSize());
	}
	//No support for invites or langames atm

	m_Flags &= ~MP_FLAGS_HOST;

	//NEWSDK, we are already joined
/*	int Ret = 0;
	if(!_invited && !(m_Flags & MP_FLAGS_LANGAME))
	{
		Ret = sceNpMatchingJoinRoomGUI(m_Context, &m_RoomID, MatchingGUIHandlerCB, this);
	}
	//No support for invites or langames atm

	if(Ret != CELL_OK)
	{
		CleanupSearchAndRegResults();
		M_TRACEALWAYS("CWGamePSNetworkHandler::JoinSession  failed");
		return;
	}*/

//	m_Flags |= MP_FLAGS_GUI_IN_PROGRESS;
//	m_Flags |= MP_FLAGS_WAITING_ON_CALLBACK;
//	m_Flags |= MP_FLAGS_SEARCHING;
	m_Flags &= ~MP_FLAGS_SEARCHING;

	m_LastHeartBeatFromServer = CMTime::GetCPU();
}

void CWGamePSNetworkHandler::ReceiveMessage(void)
{
	if(m_VDPSocket && m_VDPSocket != INVALID_SOCKET)
		ReceiveFromSocket(&m_VDPSocket, false, -1);

	if(m_Flags & MP_FLAGS_HOST)
	{
		for(int i = 0; i < m_lClients.Len(); i++)
		{
			if(m_lClients[i].m_bConnected)
				if(!ReceiveFromSocket(&m_lClients[i].m_socket, true, i))
					ClientDropped(i);
		}
	}
	else if(m_Local.m_bConnected)
		ReceiveFromSocket(&m_Host.m_socket, true, -1);

	if(!(m_Flags & MP_FLAGS_LANGAME) || !m_BroadSocket)
		return; 

	sockaddr_in addr;
	addr.sin_family = AF_INET;

	SLiveMsg Msg;

	socklen_t addr_size = sizeof(addr);
	int ret = recvfrom((SOCKET)m_BroadSocket, (char *)&Msg, sizeof(Msg), 0, (sockaddr *)&addr, &addr_size);

	if(ret > 0)
	{
		HandleMessage(&Msg, addr);
	}
	else
	{
		int Err = sys_net_errno;
		if(Err != SYS_NET_EWOULDBLOCK)
			M_TRACEALWAYS("Could not receive from broadcast socket");
	}
}

void CWGamePSNetworkHandler::HandleMessage(SLiveMsg *_msg, sockaddr_in _addr, int _client)
{
	int p = 0;
	switch(_msg->Header.m_id) 
	{
	case LIVE_MSG_JOIN_REQUEST:
		{
			ClientInfo client_info;
			_msg->m_Data.GetData(&client_info, sizeof(ClientInfo), p);
			memcpy(&client_info.m_addr, &_addr.sin_addr, sizeof(in_addr));
			if(!(m_Flags & MP_FLAGS_SESSIONLOCKED) && AddUserToSession(&client_info)) 
			{	//User was allowed to connect
				bool bConnect = false;
				for(int i = 0; i < m_lClients.Len(); i++)
				{
					ClientInfo c = m_lClients[i];
					if(memcmp(&_addr.sin_addr, &m_lClients[i].m_addr, sizeof(in_addr)) == 0)
					{
						if(!InitializeTCPSocket(&m_lClients[i].m_socket, i))
						{
							M_TRACEALWAYS("InitializeTCPSocket(&m_lClients[i].m_socket, i) for new client failed\n");
							return;
						}
						_addr.sin_port = SOCKET_TCP_PORT;
						connect(m_lClients[i].m_socket, (sockaddr*)&_addr, sizeof(_addr));
						bConnect = true;

						int Ret = sceNpSignalingActivateConnection(m_SignalingContext, &m_lClients[i].m_npid, &m_lClients[i].m_ConnectionID);
						if(Ret != CELL_OK)
						{
							M_TRACEALWAYS("sceNpSignalingActivateConnection to client failed\n");
							ShutdownSession();
						}
						break;
					}
				}

				if(bConnect)
				{
					//Now we need to tell everyone else about this client
					SLiveMsg new_client;
					new_client.Header.m_id = LIVE_MSG_NEWCLIENT;
					new_client.m_Data.AddData(&client_info, sizeof(client_info));
					for(int i = 0; i < m_lClients.Len(); i++)
					{
						bool is_old_client = false;
						if(!(m_Flags & MP_FLAGS_LANGAME))
						{
							if((m_lClients[i].m_player_id != client_info.m_player_id))
								is_old_client = true;
						}
						else
						{
							if(m_lClients[i].m_player_id != m_FakePlayerID)
								is_old_client = true;
						}

						if(is_old_client && m_lClients[i].m_player_id)
						{
							//Tell the old players about the new
							SendMessage(&new_client, i);
						}
					}
				}
			}
		}
		break;
	case LIVE_MSG_BROADCAST_SEARCH:
		{

		}
		break;
	case LIVE_MSG_BROADCAST_ANSWER:
		{

		}
		break;
	case LIVE_MSG_NEWCLIENT:
		{
			if(m_Flags & MP_FLAGS_HOST)
				return;

			ClientInfo client_info;
			_msg->m_Data.GetData(&client_info, sizeof(ClientInfo), p);

/*			if(client_info.m_addr.S_un.S_un_b.s_b1 == 127 && client_info.m_addr.S_un.S_un_b.s_b2 == 0 && client_info.m_addr.S_un.S_un_b.s_b3 == 0 && client_info.m_addr.S_un.S_un_b.s_b4 == 1)
				memcpy(&client_info.m_addr, &m_HostAddr, sizeof(m_HostAddr));
				*/
			AddUserToSession(&client_info);
		}
		break;
	case LIVE_MSG_SERVER_INFO:
		{

		}
		break;
	case LIVE_MSG_CONNECTION_ACCEPTED:
		{
//			XSessionJoinLocal(m_hSession, 1, &m_iPad, &m_Local.m_bPrivateSlot, NULL);
			m_JoinStarted = -1;
			ConExecute("cg_submenu(\"Multiplayer_StartGame\")");
		}
		break;
	case LIVE_MSG_NONCE:
		{

		}
		break;
	case LIVE_MSG_REGISTER:
		{

		}
		break;
	case LIVE_MSG_REGISTER_DONE:
		{

		}
		break;
	case LIVE_MSG_WRITE_STATISTICS:
		{

		}
		break;
	case LIVE_MSG_CREATION_INFO:
		{

		}
		break;
	case LIVE_MSG_VOICE:
		{

		}
		break;
	case LIVE_MSG_GAME_MODE:
		{
			m_GameMode = _msg->m_Data.GetInt32(p);
			m_GameSubMode = _msg->m_Data.GetInt32(p);
			m_GameStyle = _msg->m_Data.GetInt32(p);
		}
		break;
	case LIVE_MSG_PLAYERID:
		{
			if(!(m_Flags & MP_FLAGS_HOST))
			{
				int player_id = _msg->m_Data.GetInt32(p);
				SetNPID(m_Local.m_npid, player_id);
				m_Local.m_player_id = player_id;
			}
			break;
		}
	default:
		{
			CWGameMultiplayerHandler::HandleMessage(_msg, _addr, _client);
		}
		break;
	}
}

bool CWGamePSNetworkHandler::AddUserToSession(ClientInfo* _pClient)
{
	if((m_Flags & MP_FLAGS_INPROGRESS) && (m_Flags & MP_FLAGS_RANKED))
		return false;

	if(m_Flags & MP_FLAGS_HOST)
	{	//Check if the client can join, if not return false
		if(_pClient->m_bInvited)
		{	//Check private slots
			if(m_Slots[SLOTS_PRIVATE] - m_Slots[SLOTS_PRIVATE_FILLED] > 0)
			{	//Slot open
				m_Slots[SLOTS_PRIVATE_FILLED]++;
				_pClient->m_bPrivateSlot = true;
			}
			else if(m_Slots[SLOTS_PUBLIC] - m_Slots[SLOTS_PUBLIC_FILLED] > 0)
			{	//Slot open
				m_Slots[SLOTS_PUBLIC_FILLED]++;
				_pClient->m_bPrivateSlot = false;
			}
		}
		else
		{
			bool result = false;
			//TODO
			//Check if friends
				
			if(result)
			{
				if(m_Slots[SLOTS_PRIVATE] - m_Slots[SLOTS_PRIVATE_FILLED] > 0)
				{	//Slot open
					m_Slots[SLOTS_PRIVATE_FILLED]++;
					_pClient->m_bPrivateSlot = true;
				}
				else if(m_Slots[SLOTS_PUBLIC] - m_Slots[SLOTS_PUBLIC_FILLED] > 0)
				{	//Slot open
					m_Slots[SLOTS_PUBLIC_FILLED]++;
					_pClient->m_bPrivateSlot = false;
				}
			}
			else if(m_Slots[SLOTS_PUBLIC] - m_Slots[SLOTS_PUBLIC_FILLED] > 0)
			{	//Slot open
				m_Slots[SLOTS_PUBLIC_FILLED]++;
				_pClient->m_bPrivateSlot = false;
			}
			else
				return false;
		}
	}

	int i = 0;
	for(;i < m_lClients.Len(); i++)
	{
		if(m_lClients[i].m_player_id == 0)
			break;
	}

	M_TRACEALWAYS(CStrF("Adding client: %i \n", i));
	memcpy(&m_lClients[i], _pClient, sizeof(ClientInfo));

	if((m_Flags & MP_FLAGS_HOST) && (m_Flags & MP_FLAGS_LANGAME))
	{
		m_FakePlayerID++;
		m_lClients[i].m_player_id = m_FakePlayerID;
	}

	if(m_Flags & MP_FLAGS_HOST)
	{
		m_lClients[i].m_player_id = SetNPID(m_lClients[i].m_npid);
		m_lClients[i].m_team_id = ++m_TeamID;
		_pClient->m_team_id = m_lClients[i].m_team_id;
		_pClient->m_player_id = m_lClients[i].m_player_id;
	}
	else
		SetNPID(m_lClients[i].m_npid, m_lClients[i].m_player_id);

	if(m_Flags & MP_FLAGS_HOST)
		m_LastHeartBeatReceived[i] = CMTime::GetCPU();

	return true;
}

void CWGamePSNetworkHandler::ClientDropped(int8 _client, int _player_id)
{
	M_TRACEALWAYS("Dropping client\n");
	int ClientIndex = _client;
	if(_client == -1)
	{
		for(int i = 0; i < m_lClients.Len(); i++)
		{
			if(m_lClients[i].m_player_id == _player_id)
			{
				ClientIndex = i;
				break;
			}
		}
	}

	if(m_lClients[ClientIndex].m_player_id)
	{
		if(m_lClients[ClientIndex].m_socket)
		{
			socketclose(m_lClients[ClientIndex].m_socket);
			m_lClients[ClientIndex].m_socket = NULL;
		}

		if(m_lClients[ClientIndex].m_bPrivateSlot)
			m_Slots[SLOTS_PRIVATE_FILLED]--;
		else
			m_Slots[SLOTS_PUBLIC_FILLED]--;

		if(m_Flags & MP_FLAGS_HOST)
		{
			SLiveMsg Msg;
			Msg.Header.m_id = LIVE_MSG_CLIENT_DROPPED;
			Msg.m_Data.AddInt32(m_lClients[ClientIndex].m_player_id);
			for(int ii = 0; ii < m_lClients.Len(); ii++)
			{
				if(m_lClients[ii].m_player_id)
				{
					if(_client != ii && m_lClients[ii].m_socket)
					{
						SendMessage(&Msg, ii);
					}
				}
			}
		}

		memset(&m_lClients[ClientIndex], 0, sizeof(ClientInfo));

		M_TRACEALWAYS("Client dropped!\n");
		return;
	}
}

void CWGamePSNetworkHandler::StartGame(void)
{
	if(m_Flags & MP_FLAGS_WAITING_ON_CALLBACK)
		return;

	CWGameMultiplayerHandler::StartGame();
}

void CWGamePSNetworkHandler::Connect(void)
{
//	CStr cmd = CStrF("connect(\"%d.%d.%d.%d\")", m_Host.m_addr.S_un.S_un_b.s_b1, m_Host.m_addr.S_un.S_un_b.s_b2, m_Host.m_addr.S_un.S_un_b.s_b3, m_Host.m_addr.S_un.S_un_b.s_b4);
//	ConExecute(cmd);
}

void CWGamePSNetworkHandler::ClearNPIDs(void)
{
	M_TRACEALWAYS("Clearing NPIDs\n");
	SPlayerID save_id;
	for(int i = 0; i < m_lPlayerIDs.Len(); i++)
	{
		if(m_lPlayerIDs[i].player_id != -1)
		{
			save_id = m_lPlayerIDs[i];
		}
	}
	m_lPlayerIDs.Clear();
	m_lPlayerIDs.Add(save_id);
}

SceNpId CWGamePSNetworkHandler::GetNPID(int _PlayerId)
{
	for(int i = 0; i < m_lPlayerIDs.Len(); i++)
	{
		if(m_lPlayerIDs[i].player_id == _PlayerId)
			return m_lPlayerIDs[i].np_id;
	}

	M_TRACEALWAYS(CStrF("Failed to get NPI for player id: %i", _PlayerId));
	SceNpId Tmp;
	memset(&Tmp, 0, sizeof(SceNpId));
	return Tmp;
}

int CWGamePSNetworkHandler::SetNPID(SceNpId _npid, int _player_id)
{
	int id_to_set = 0;
	if(m_Flags & MP_FLAGS_HOST)
	{
		m_NextPlayerID++;
		id_to_set = m_NextPlayerID;
	}
	else
	{
		if(_player_id)
			id_to_set = _player_id;
		else
		{
			m_NextPlayerID++;
			id_to_set = m_NextPlayerID * -1;
		}
	}

	for(int i = 0; i < m_lPlayerIDs.Len(); i++)
	{
		if((sceNpUtilCmpNpId(&m_lPlayerIDs[i].np_id, &_npid) == 0) && m_lPlayerIDs[i].player_id != -1)
		{
			m_lPlayerIDs[i].player_id = id_to_set;
			M_TRACEALWAYS(CStrF("SetNPID, NPID already exists - id: %i\n", m_lPlayerIDs[i].player_id));
			return m_lPlayerIDs[i].player_id;
		}
	}

	SPlayerID new_id;
	new_id.np_id = _npid;
	new_id.player_id = id_to_set;
	M_TRACEALWAYS(CStrF("Adding new NPID - ID %i\n", id_to_set));
	m_lPlayerIDs.Add(new_id);
	return new_id.player_id;
}

