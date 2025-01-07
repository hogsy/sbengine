
#include "PCH.h"

#include "WServer_Core.h"
#include "../WPackets.h"
#include "../WObjects/WObj_Game.h"

// -------------------------------------------------------------------
//  Network messaging.
// -------------------------------------------------------------------
bool CWorld_ServerCore::Net_PutMsg(int _iClient, const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWorld_ServerCore_Net_PutMsg, false);
	MSCOPESHORT(CWorld_ServerCore::Net_PutMsg);
	if (_iClient < 0) return true;
	CWServer_ClientInfo* pC = m_lspClientInfo[_iClient];

#ifdef WSERVER_NOLOCALMIRROR
	if (pC->m_spLocalClient != NULL)
	{
		pC->m_spLocalClient->Net_OnMessage(_Msg);
	}
	else
#endif
	{
		if (!pC->m_OutMsg.AddMsg(_Msg))
		{
			if (!Net_FlushClientMsg(_iClient)) return false;
			if (!pC->m_OutMsg.AddMsg(_Msg)) Error("Net_PutMsg", "Internal error.");
		}
	}
//ConOutL(CStrF("SV %.8x, Msg %d, %d, %d", this, _Msg.m_MsgType, _Msg.m_MsgSize, pC->m_OutMsg.m_Size));
	return true;
}

bool CWorld_ServerCore::Net_PutMsgEx(int _iClient, const CNetMsg& _Msg, int _PVSNode, int _State, int _MaxLagFrames)
{
	MAUTOSTRIP(CWorld_ServerCore_Net_PutMsgEx, false);
	MSCOPESHORT(CWorld_ServerCore::Net_PutMsgEx);
	if (!m_lspClients.ValidPos(_iClient)) return true;
	CWorld_Client* pC = m_lspClients[_iClient];
	CWServer_ClientInfo* pCI = m_lspClientInfo[_iClient];

	if (pC && pCI)
	{
		if (_State >= 0 && pCI->m_State != _State) return true;
		if (_MaxLagFrames >= 0 && pCI->m_nClientSimTicks > _MaxLagFrames) return true;
		if (_PVSNode >= 0 && pCI->m_pPVS && !(pCI->m_pPVS[_PVSNode >> 3] & (1 << (_PVSNode & 7)))) return true;

		return Net_PutMsg(_iClient, _Msg);
	}
	else
		return true;
}

void CWorld_ServerCore::Net_PutMsgAll(const CNetMsg& _Msg, int _PVSNode, int _State, int _MaxLagFrames)
{
	MAUTOSTRIP(CWorld_ServerCore_Net_PutMsgAll, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_lspClients.Len(); i++)
	{
		CWorld_Client* pC = m_lspClients[i];
		if (pC) Net_PutMsgEx(i, _Msg, _PVSNode, _State, _MaxLagFrames);
	}
}

const CNetMsg* CWorld_ServerCore::Net_GetMsg(int _iClient)
{
	MAUTOSTRIP(CWorld_ServerCore_Net_GetMsg, NULL);
	MSCOPESHORT(CWorld_ServerCore::Net_GetMsg);
	CWServer_ClientInfo* pC = m_lspClientInfo[_iClient];
	int hCon = pC->m_hConnection;

//	if (hCon == -1) Error("Net_GetMsg", "Not a network-client.");

	const CNetMsg* pMsg = pC->m_InMsg.GetMsg();
	if (!pMsg)
	{
		pC->m_InMsg.Clear();
		if (hCon == -1)
		{
			CWorld_Client* pClient = pC->m_spLocalClient;
//LogFile(CStrF("(CWorld_ServerCore::Net_GetMsg) iC %d, pC %.8x, vtbl %.8x", _iClient, pClient, *((uint32*)pClient)));
			if (pClient->Local_OutQueueEmpty())
			{
//LogFile(CStr("(CWorld_ServerCore::Net_GetMsg) NULL"));
				return NULL;
			}
//LogFile(CStr("(CWorld_ServerCore::Net_GetMsg) ..."));
			pC->m_InMsg = *pClient->Local_GetOutMsg();
//LogFile(CStr("(CWorld_ServerCore::Net_GetMsg) Done."));
		}
		else
		{
			while (1)
			{
				CNetwork_Packet *pPacket = m_spNetwork->Connection_PacketReceive(hCon);

				if (pPacket)
				{
					if (pPacket->m_Size > NET_MAXPACKETSIZE)
					{
						m_spNetwork->Connection_PacketRelease(hCon, pPacket);
						ConOut("(CWorld_ServerCore::Net_GetMsg) Threw away packet that was bigger than NET_MAXPACKETSIZE.");
						continue;
					}
					pC->m_InMsg.m_Size = pPacket->m_Size;
					memcpy(pC->m_InMsg.m_MsgBuf, pPacket->m_pData, pC->m_InMsg.m_Size);
					m_spNetwork->Connection_PacketRelease(hCon, pPacket);
					break;
				}
				else
					break;
			}
		}
		pMsg = pC->m_InMsg.GetMsg();
	}

	return pMsg;
}

bool CWorld_ServerCore::Net_FlushClientConnection(int _iClient)
{
	MAUTOSTRIP(CWorld_ServerCore_Net_FlushClientMsg, false);
	MSCOPESHORT(CWorld_ServerCore::Net_FlushClientMsg);
	CWServer_ClientInfo* pC = m_lspClientInfo[_iClient];
	if (!pC) return false;

	int hCon = pC->m_hConnection;

	if (hCon != -1)
		m_spNetwork->Connection_Flush(hCon);

	return true;
}

bool CWorld_ServerCore::Net_FlushClientMsg(int _iClient)
{
	MAUTOSTRIP(CWorld_ServerCore_Net_FlushClientMsg, false);
	MSCOPESHORT(CWorld_ServerCore::Net_FlushClientMsg);
	CWServer_ClientInfo* pC = m_lspClientInfo[_iClient];
	if (!pC) return true;

	int hCon = pC->m_hConnection;
//	if (hCon == -1) return true;

	if (pC->m_OutMsg.m_Size)
	{
		if (hCon == -1)
			{ if (!pC->m_spLocalClient->Local_PutInMsg(pC->m_OutMsg)) return false; }
		else
			{ 
				CNetwork_Packet Packet;
				Packet.m_pData = pC->m_OutMsg.m_MsgBuf;
				Packet.m_Size = pC->m_OutMsg.m_Size;
				if (!m_spNetwork->Connection_PacketSend(hCon, &Packet)) 
					return false;
			}

		pC->m_OutMsg.Clear();
	}
	return true;
}

void CWorld_ServerCore::Net_FlushMessages()
{
	MAUTOSTRIP(CWorld_ServerCore_Net_FlushMessages, MAUTOSTRIP_VOID);
	int ncl = m_lspClientInfo.Len();
	for(int i = 0; i < ncl; i++)
		Net_FlushClientMsg(i);

	for(int i = 0; i < ncl; i++)
		Net_FlushClientConnection(i);

	if (m_spNetwork != NULL && m_hServerSocket >= 0)
	{
		m_spNetwork->Connection_Flush(m_hServerSocket);		
	}
}

// -------------------------------------------------------------------
void CWorld_ServerCore::Net_CheckPendingConnections()
{
	MAUTOSTRIP(CWorld_ServerCore_Net_CheckPendingConnections, MAUTOSTRIP_VOID);
	if (m_ServerMode & SERVER_MODE_NOCONNECT) return;

	// Any new connections?
	if (m_spNetwork != NULL && m_hServerSocket >= 0)
	{
		while(m_spNetwork->Server_Connection_Avail(m_hServerSocket))
		{
LogFile("New connection...");
LogFile("	-Accepting connection");
			int hCon = m_spNetwork->Server_Connection_Accept(m_hServerSocket, NULL);
			Client_Connect(hCon);
LogFile("	-Done");
		}

	}
}

void CWorld_ServerCore::Net_DestroyClient(int _iClient, int _Reason)
{
	MAUTOSTRIP(CWorld_ServerCore_Net_DestroyClient, MAUTOSTRIP_VOID);
	/* Reason	0 : Server shutdown
				1 : Client quit
				2 : Client timeout
	*/
	CWorld_Client* pC = m_lspClients[_iClient];
	CWServer_ClientInfo* pCI = m_lspClientInfo[_iClient];
	if (!pC || !pCI) return;

	CStr QuitMsg;
	if (pCI->m_iPlayer >= 0)
	{
/*		CPlayer* pP = Player_Get(pCI->m_iPlayer);
		if (pP && _Reason) 
			QuitMsg = (_Reason == 1) ?
				CStrF("%s disconnected with %d frags.", (char*)pP->m_Name, pP->m_Frags) :
				CStrF("%s dropped with %d frags.", (char*)pP->m_Name, pP->m_Frags);
*/
		Message_SendToTarget(CWObject_Message(OBJSYSMSG_GAME_REMOVECLIENT, _iClient, _Reason), WSERVER_GAMEOBJNAME);
		CWObject_Game *pGame = Game_GetObject();
		if(pGame)
			pGame->Player_Destroy(pCI->m_iPlayer);
	}

	if (m_lspClientInfo[_iClient]->m_hConnection >= 0)
	{
		m_spServerReg->DeleteDir(CStrF("CLIENTMIRROR%d", _iClient));
		m_spNetwork->Connection_Close(m_lspClientInfo[_iClient]->m_hConnection);
		m_lspClientInfo[_iClient]->m_hConnection = -1;
	}
	m_lspClientInfo[_iClient] = NULL;
	m_lspClients[_iClient] = NULL;

	// Update cheats variable
	// Removed because the server registry is now read-only.
/*	if (!m_spServerReg->GetValuei("ALWAYSCHEATS", 0, 0))
	{
		int nClients = 0;
		for(int i = 0; i < m_lspClientInfo.Len(); i++)
			if (m_lspClientInfo[i] != NULL) nClients++;

		m_spServerReg->SetValuei("CHEATS", (nClients > 1) ? 0 : 1);
	}
*/
//	if (QuitMsg.Len()) Net_ConOut(QuitMsg);
}

// -------------------------------------------------------------------
static const char* Msg2Str(int _Msg)
{
	MAUTOSTRIP(Msg2Str, NULL);
	switch(_Msg)
	{
	case WPACKET_VOID : return "Void";
	case WPACKET_CONTROLS : return "Controls";
	case WPACKET_PLAYERINFO : return "PlayerInfo";
	case WPACKET_DELTAFRAME : return "DeltaFrame";
	case WPACKET_STATUSBARINFO : return "StatusBarInfo";
	case WPACKET_SOUND : return "Sound";

	case WPACKET_LARGESIZE : return "LargeSize";

	case WPACKET_WORLD : return "World";
	case WPACKET_FILEREQUEST : return "FileRequest";
	case WPACKET_FILEFRAGMENT : return "FileFragment";
	case WPACKET_COMMAND : return "Command";
	case WPACKET_RESOURCEID : return "ResourceID";
	case WPACKET_ENTERGAME : return "EnterGame";
	case WPACKET_DELTAREGISTRY : return "DeltaRegistry";
	default : return "Unknown";
	}
}

void CWorld_ServerCore::Net_RConWriteCallback(const CStr& _Str, void* _pContext)
{
	MAUTOSTRIP(CWorld_ServerCore_Net_RConWriteCallback, MAUTOSTRIP_VOID);
	CWorld_ServerCore* pWServer = (CWorld_ServerCore*)_pContext;
	CStr s(_Str);
	if (!s.Len())
		pWServer->Net_ConOut(s, pWServer->m_iRConClient);
	else
		pWServer->Net_ConOut(CStrF("§ccfc%s", (char*)s), pWServer->m_iRConClient);
}

void CWorld_ServerCore::Net_OnMessage(int _iClient, const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWorld_ServerCore_Net_OnMessage, MAUTOSTRIP_VOID);
	MSCOPESHORT(Net_OnMessage);
//	CWorld_Client* pC = m_lspClients[_iClient];
	CWServer_ClientInfo* pCI = m_lspClientInfo[_iClient];

	if (m_bNetShowMsg && _Msg.m_MsgType != WPACKET_CONTROLS)
	{
		CWServer_ClientInfo* pCI = m_lspClientInfo[_iClient];
		CStr ClName;
		if (pCI->m_hConnection < 0) 
			ClName = "Local";
		else
			ClName = CStrF("hCon %d, Status %d", pCI->m_hConnection, m_spNetwork->Connection_Status(pCI->m_hConnection));

		ConOutL(CStrF("Client %d (%s), Msg %s, MsgSize %d", _iClient, (char*)ClName, Msg2Str(_Msg.m_MsgType), _Msg.m_MsgSize));
	}

	switch(_Msg.m_MsgType)
	{
	// ---------------------------------
	case WPACKET_PLAYERINFO :
		{
//LogFile("(CWorld_ServerCore::Net_OnMessage) Recieved playerinfo.");
			CPlayer Player;
			int Pos = 0;

			while(Pos < _Msg.m_MsgSize)
			{
				CStr Key = _Msg.GetStr(Pos);
				CStr Value;
				if (Pos < _Msg.m_MsgSize)
					Value = _Msg.GetStr(Pos);

				Net_SetClientVar(_iClient, Key, Value);
			}

			Message_SendToTarget(CWObject_Message(OBJSYSMSG_GAME_CLIENTVARCHANGED, _iClient), WSERVER_GAMEOBJNAME);

//			Player.GetFromPacket(_Msg, Pos);
/*			CPlayer* pP = Player_Get(m_lspClientInfo[_iClient]->m_iPlayer);
			if (pP)
			{
				CStr NewName = pC->m_spClientReg->GetValue("NAME", "Player", 0);
				if (!CStr::StrLen(pP->m_Name))
					Net_ConOut(CStrF("%s connected.", (char*) NewName));
				pP->SetName(NewName);

				Message_SendToTarget(CWObject_Message(OBJSYSMSG_GAME_CLIENTVARCHANGED, iC), WSERVER_GAMEOBJNAME);
			}
			else
				ConOutL("§cf80WARNING: No player created!!!");*/

//LogFile(CStrF("    -Name %s", (char*)Player.m_Name));
//LogFile("(CWorld_ServerCore::Net_OnMessage) Done.");
			break;
		}
	// ---------------------------------
	case WPACKET_CONTROLS :
		{
			CWObject_Game *pGame = Game_GetObject();
			CWO_Player* pP = pGame ? pGame->Player_Get(m_lspClientInfo[_iClient]->m_iPlayer) : NULL;
			if (pP)
			{
				CControlFrame CtrlFrame;
				CtrlFrame = _Msg;
				if (!pP->m_spCmdQueue->AddCmds(CtrlFrame))
				{
					ConOut("(CWorld_ServerCore::Net_OnMessage) Client cmd buffer full. Zonking it...");
					while(!pP->m_spCmdQueue->Empty())
						pP->m_spCmdQueue->Get();
				}
//				pP->m_spControlQueue->Put(CtrlFrame);
			}
			

/*	ConOut(CStrF("(%d, %d, %d) (%d, %d, %d)", 
		pC->m_LocalPlayer.m_Controls.m_Controls[0], 
		pC->m_LocalPlayer.m_Controls.m_Controls[1], 
		pC->m_LocalPlayer.m_Controls.m_Controls[2], 
		pC->m_LocalPlayer.m_Controls.m_Controls[3], 
		pC->m_LocalPlayer.m_Controls.m_Controls[4], 
		pC->m_LocalPlayer.m_Controls.m_Controls[5]));*/
//							memcpy(&pC->m_LocalPlayer.m_Controls, &RecvPacket.m_Data[0], sizeof(pC->m_LocalPlayer.m_Controls));

//							m_lPlayers[pCI->m_iPlayer].m_Controls.GetPackedControls(&RecvPacket.m_Data, RecvLen-1);
			break;
		}
	// ---------------------------------
	case WPACKET_FILEREQUEST :
		{
			ConOut("Got a filerequest!");
			break;
		}
	case WPACKET_COMMAND : 
		{
			CStr RConPw = Registry_GetServer()->GetValue("RCON_PASSWORD");
			if (RConPw == "")
			{
				Net_ConOut(CStr("Remote control not enabled."), _iClient);
				break;
			}

			int Pos = 0;
			CStr Pw = _Msg.GetStr(Pos);
			if (RConPw.CompareNoCase(Pw) != 0)
			{
				Net_ConOut(CStr("Invalid password."), _iClient);
// URGENTFIXME: Must break at invalid password.
				break;
			}

			CStr Cmd = _Msg.GetStr(Pos);

			m_iRConClient = _iClient;
			M_TRY
			{
				MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
				if (pCon)
				{
					pCon->AddWriteCallback(Net_RConWriteCallback, this);
					M_TRY
					{
						pCon->ExecuteString(Cmd);
					}
					M_CATCH(
					catch(CCException)
					{
						pCon->RemoveWriteCallback(Net_RConWriteCallback, this);
						throw;
					}
					)
					pCon->RemoveWriteCallback(Net_RConWriteCallback, this);
				}
			}
#ifndef M_RTM
			M_CATCH(
			catch(CCException E)
			{
				Net_ConOut("§ccfc" + E.GetExceptionInfo().m_Message + " (Command: " + Cmd + ")", _iClient);
			}
			)
#else
			M_CATCH(
			catch(CCException E)
			{
				Net_ConOut("§ccfc" + E.GetExceptionInfo().GetString() + " (Command: " + Cmd + ")", _iClient);
			}
			)
#endif
m_iRConClient = -1;
			break;
		}

	case WPACKET_SETVAR :
		{
			int Pos = 0;
			_Msg.GetInt16(Pos);
			CStr Key = _Msg.GetStr(Pos);
			CStr Value = _Msg.GetStr(Pos);

			Net_SetClientVar(_iClient, Key, Value);
			Message_SendToTarget(CWObject_Message(OBJSYSMSG_GAME_CLIENTVARCHANGED, _iClient), WSERVER_GAMEOBJNAME);

			// Hack.. flytta någonstans..
			if (Key.CompareNoCase("RATE") == 0)
			{
				if (pCI->m_hConnection < 0)
				{
					Net_ConOut(CStr("Cannot set network rate on local client."), _iClient);
				}
				else
				{
					int Rate = int(Value.Val_fp64());
					m_spNetwork->Connection_SetRate(pCI->m_hConnection, Rate);
					Net_ConOut(CStrF("Network rate changed to %d", Rate), _iClient);
				}
			}

			return;
		}

	case WPACKET_SAY :
		{
			int Pos = 0;
			uint16 Mode = _Msg.GetInt16(Pos);
			CStr Msg = _Msg.GetStr(Pos);

			Message_SendToTarget(CWObject_Message(OBJSYSMSG_GAME_CLIENTSAY, _iClient, Mode, -1, 0, 0, 0, (char*) Msg, Msg.Len()), WSERVER_GAMEOBJNAME);
			return;
		}

	case WPACKET_DONEPRECACHE :
		{
			if (pCI->m_ChangeLevelState == SERVER_CLS_WAITPRECACHE)
			{
				if(m_bPendingPrecacheDoUnpause)
				{
					MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
					if (pCon)
					{
						pCon->ExecuteString("pause(0)");
						m_bPendingPrecacheDoUnpause = false;
					}
				}

				pCI->m_ChangeLevelState++;
			}
		}
		break;

	case WPACKET_SOUNDSYNC:
		{
			// A client has finished loading streams for a specific group id. Tell the group object about it.
			int Pos = 0;
			uint32 GroupID = _Msg.GetInt32(Pos);
		//	M_TRACE("[SERVER] got sound sync for sync group 0x%08X from client %d\n", GroupID, _iClient);
			int iGroupObj = Selection_GetSingleTarget(GroupID);
			Message_SendToObject(CWObject_Message(OBJSYSMSG_SOUNDSYNC, 1, M_BitD(_iClient)), iGroupObj);
		}
		break;

	default :
		ConOut(CStrF("(Net_OnMessage) Unknown msg. (%d, %d)", _Msg.m_MsgType, _Msg.m_MsgSize));
	}
}


// -------------------------------------------------------------------
void CWorld_ServerCore::Net_Refresh()
{
	MAUTOSTRIP(CWorld_ServerCore_Net_Refresh, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ServerCore::Net_Refresh, WORLD_SERVER);
	M_NAMEDEVENT("CWorld_ServerCore::Net_Refresh", 0xff000000);

//	if (m_spNetwork != NULL) m_spNetwork->Refresh();

	Net_CheckPendingConnections();

	// Check all clients
	for(int iC = 0; iC < m_lspClients.Len(); iC++)
		if (m_lspClients[iC] != NULL)
		{
			CWorld_Client* pC = m_lspClients[iC];
			CWServer_ClientInfo* pCI = m_lspClientInfo[iC];
			bool bIsLocal = false;

#ifdef WSERVER_NOLOCALMIRROR
			if (pCI->m_spLocalClient != NULL)
			{
				pC = pCI->m_spLocalClient;

				bIsLocal = true;
			}
#endif


			int hCon = pCI->m_hConnection;

//			bool bNetLog = (m_bNetShowMsg && hCon >= 0);
			bool bNetLog = m_bNetShowMsg;

			if (hCon >= 0)
			{
#ifdef _DEBUG
				int bTimeOut = false;
#else
				int bTimeOut = m_spNetwork->Connection_Status(hCon) & ENetworkPacketStatus_Timeout;
#endif
				if (bTimeOut || (m_spNetwork->Connection_Status(hCon) & ENetworkPacketStatus_Closed))
				{
					Net_DestroyClient(iC, (bTimeOut) ? 2 : 1);
					continue;
				}
			}

			// -------------------------------------------------------------------
			// Process messages.
			const CNetMsg* pMsg = Net_GetMsg(iC);
			while(pMsg)
			{
				Net_OnMessage(iC, *pMsg);
				pMsg = Net_GetMsg(iC);
			}

			// -------------------------------------------------------------------
			// Update ping-time
			if (hCon >= 0)
			{
				pCI->m_LastPing = int(1000.0f*m_spNetwork->Connection_GetPing(hCon));
//			ConOut(CStrF("Netrefresh %d", pCI->m_LastPing));
			}
			else
				pCI->m_LastPing = 0;

			// -------------------------------------------------------------------
			// Replicate pause state
			bool bClPause = (pC->GetClientMode() & WCLIENT_MODE_PAUSE) != 0;
			bool bSvPause = (m_ServerMode & SERVER_MODE_PAUSE) != 0;

			if (bClPause != bSvPause)
			{
				CNetMsg OutPacket(WPACKET_PAUSE);
				OutPacket.AddInt8((m_ServerMode & SERVER_MODE_PAUSE) ? 1 : int(0));

				if (Net_PutMsg(iC, OutPacket))
					if (!bIsLocal)
						pC->Net_OnMessage(OutPacket);
			}

			// -------------------------------------------------------------------
			if (pCI->m_ServerUpdateMask & SERVER_CLIENTUPDATE_WORLD)
			{
if (bNetLog) LogFile(CStrF("(CWorld_ServerCore::Net_Refresh) World, Client %d.", iC));
				pCI->m_ServerUpdateMask &= ~SERVER_CLIENTUPDATE_ALL;
				pCI->m_ServerUpdateMask |= SERVER_CLIENTUPDATE_WORLD;
				pCI->m_bFrameOut = false;
				pCI->m_FrameOut.Clear();

//LogFile(CStrF("(CWorld_ServerCore::Net_Refresh) Client upd, world. %d", iC));
				CNetMsg OutPacket(WPACKET_WORLD);

				// Store object-heap size
				OutPacket.AddInt16(m_lspObjects.Len());

				OutPacket.AddInt16(m_spMapData->GetNumResources());

				// Store Spawnmask
				OutPacket.AddInt32(m_CurSpawnMask);

				// Store world name
				CStr WName = m_WorldName.GetFilenameNoExt();
				OutPacket.AddData((char*) WName, WName.Len());
				OutPacket.AddInt8(0);

				

				// Ship it!
//				if (m_spNetwork->SendPacket(hCon, &OutPacket, OutPacket.GetSize()))
				if (Net_PutMsg(iC, OutPacket))
				{
					pCI->m_ServerUpdateMask &= ~SERVER_CLIENTUPDATE_WORLD;
					pCI->m_State = WCLIENT_STATE_CHANGELEVEL;
					pCI->m_ResourceNr = 0;
					pCI->m_ChangeLevelState = 0;
if (bNetLog) LogFile(CStr("(CWorld_ServerCore::Net_Refresh) World, calling mirror."));
					if (!bIsLocal)
						pC->Net_OnMessage(OutPacket);
if (bNetLog) LogFile(CStrF("(CWorld_ServerCore::Net_Refresh) World, Client %d, switch to state_changelevel.", iC));
//					pC->m_spMapData = m_spMapData;
				}
				else
					continue;

//LogFile(CStrF("(CWorld_ServerCore::Net_Refresh) Client upd, world. %d (Done)", iC));
			}

			if (pCI->m_State == WCLIENT_STATE_CHANGELEVEL)
			{
				switch(pCI->m_ChangeLevelState)
				{
				case SERVER_CLS_RESOURCE :
					// Transfer resource IDs.
					{
if (bNetLog) LogFile(CStrF("(CWorld_ServerCore::Net_Refresh) Client %d, connect state 0", iC));
						int nRc = m_spMapData->GetNumResources();
						while(pCI->m_ResourceNr < nRc)
						{
							CWResource* pRc = m_spMapData->GetResource(pCI->m_ResourceNr);
							if (pRc)
							{
								CNetMsg OutPacket(WPACKET_RESOURCEID);
								OutPacket.AddInt16(pCI->m_ResourceNr);
								OutPacket.AddInt8(pRc->GetClass());
								OutPacket.AddStr(pRc->GetName());
								if (!Net_PutMsg(iC, OutPacket))
									break;

								if (!bIsLocal)
									pC->Net_OnMessage(OutPacket);
							}
							pCI->m_ResourceNr++;
						}

						if (pCI->m_ResourceNr >= nRc)
						{
if (bNetLog) LogFile(CStrF("(CWorld_ServerCore::Net_Refresh) Client %d, connect state 0 -> 1", iC));
							pCI->m_ChangeLevelState++;
							pCI->m_ServerUpdateMask |= 
//								SERVER_CLIENTUPDATE_PLAYER | 
								SERVER_CLIENTUPDATE_STATUSBAR | 
								SERVER_CLIENTUPDATE_DELTAFRAME | 
								SERVER_CLIENTUPDATE_DELTAREGISTRY | 
								SERVER_CLIENTUPDATE_COMPLETEFRAME;
						}
						else
							break;
					}
				case SERVER_CLS_GAMESTATE :
if (bNetLog) LogFile(CStrF("(CWorld_ServerCore::Net_Refresh) Client %d, connect state 1, UpdateMask %.8x", iC, pCI->m_ServerUpdateMask));
					if ((pCI->m_ServerUpdateMask & (SERVER_CLIENTUPDATE_DELTAREGISTRY | SERVER_CLIENTUPDATE_PLAYER | SERVER_CLIENTUPDATE_STATUSBAR | SERVER_CLIENTUPDATE_DELTAFRAME | SERVER_CLIENTUPDATE_COMPLETEFRAME)) == 0)
					{
if (bNetLog) LogFile(CStrF("(CWorld_ServerCore::Net_Refresh) Client %d, connect state 1->2", iC));
						pCI->m_ChangeLevelState++;
					}
					else
						break;
				case SERVER_CLS_DOPRECACHE :
					{
						CNetMsg OutPacket(WPACKET_DOPRECACHE);
						if (Net_PutMsg(iC, OutPacket))
						{
							pCI->m_ChangeLevelState++;
						}
					}
					break;

				case SERVER_CLS_WAITPRECACHE :
					{
						// Awaiting precache done msg.
					};
					break;

				case SERVER_CLS_ENTERGAME :
					{
if (bNetLog) LogFile(CStrF("(CWorld_ServerCore::Net_Refresh) Client %d, connect state 2", iC));
						CNetMsg OutPacket(WPACKET_ENTERGAME);
						if (Net_PutMsg(iC, OutPacket))
						{
							if (!bIsLocal)
								pC->Net_OnMessage(OutPacket);
							pCI->m_State = WCLIENT_STATE_INGAME;

							Message_SendToTarget(CWObject_Message(OBJSYSMSG_GAME_CLIENTCONNECTED, iC), WSERVER_GAMEOBJNAME);

if (bNetLog) LogFile(CStrF("(CWorld_ServerCore::Net_Refresh) Client %d, connect complete.", iC));

/*							CPlayer* pP = Player_Get(pCI->m_iPlayer);
							if (!(pP->m_Flags & PLAYER_FLAGS_PLAYER))
								Player_Respawn(pCI->m_iPlayer);
							if (pP) Net_ConOut(CStrF("%s entered this realm.", (char*)pP->m_Name));*/
						}
					}
				}
			}

			// -------------------------------------------------------------------
			if (pCI->m_ServerUpdateMask & SERVER_CLIENTUPDATE_STATUSBAR)
			{
/*				int iPlayer = pCI->m_iPlayer;

				CNetMsg OutPacket(WPACKET_STATUSBARINFO);
				m_lPlayers[iPlayer].m_StatusBar.DeltaAddToPacket(OutPacket, &pC->m_LocalPlayer.m_StatusBar);

//				if (m_spNetwork->SendPacket(hCon, &OutPacket, 1+size))
				if (Net_PutMsg(iC, OutPacket))
				{
					if (!bIsLocal)
						pC->Net_OnMessage(OutPacket);
					pCI->m_ServerUpdateMask &= ~SERVER_CLIENTUPDATE_STATUSBAR;
				}*/

				pCI->m_ServerUpdateMask &= ~SERVER_CLIENTUPDATE_STATUSBAR;
			}

			// -------------------------------------------------------------------
			if (pCI->m_ServerUpdateMask & SERVER_CLIENTUPDATE_DELTAFRAME)
			{
				if (bIsLocal)
					pC->Net_FlushGameFrames();

				if (!pCI->m_bFrameOut)
				{
//CHECKMEMORY("Net_Refresh");
					pCI->m_FrameOut.Clear();

					pCI->m_bFrameComplete = CreateClientUpdate(iC, &pCI->m_FrameOut, pCI->m_ServerUpdateMask & SERVER_CLIENTUPDATE_COMPLETEFRAME);

					CWObject_Game *pGame = Game_GetObject();
					CWO_Player* pP = pGame ? pGame->Player_Get(pCI->m_iPlayer) : NULL;
					if (pP)
					{
						pCI->m_FrameOut.m_ControlSerial = pP->m_ServerControlSerial;
						pCI->m_FrameOut.m_ControlAccumTime = pP->m_ServerControlAccumTime;
					}

//CHECKMEMORY("Net_Refresh");

					if (m_bNetShowMsg) 
						LogFile(CStrF("Client upd %d, iC %d, Complete %d", pCI->m_FrameOut.GetSize(), iC, pCI->m_bFrameComplete));

					TProfileDef(TUnpack);
					{
						TMeasureProfile(TUnpack);
						if (!bIsLocal)
							pC->Net_UnpackClientUpdate(&pCI->m_FrameOut, 0);
					}
//		ConOut(CStrF("UnpackClientUpdate %f ms", 1000.0*TUnpack / GetCPUFrequency()));

					pCI->m_bFrameOut = true;
				}

				while(pCI->m_bFrameOut)
				{
					CNetMsg OutPacket(0);
					pCI->m_FrameOut.GetPacket(OutPacket, WPACKET_DELTAFRAME);

//LogFile(CStrF("Client upd, dFrame. iC %d, size %d/%d, n %d", iC, pCI->m_FrameOut.m_IOPos, pCI->m_FrameOut.m_Size, pCI->m_FrameOut.m_nChunks));
/*					if (!pCI->m_FrameOut.m_IOPos)
					{
						int Size = pCI->m_FrameOut.m_Data[0] + (pCI->m_FrameOut.m_Data[1] << 8);
						int nChunks = pCI->m_FrameOut.m_Data[2] + (pCI->m_FrameOut.m_Data[3] << 8);
						LogFile(CStrF("FrameSize %d, %d", Size, nChunks));
					}*/

					if (Net_PutMsg(iC, OutPacket))
					{
						if (pCI->m_FrameOut.GetNext())
						{
							// ConOutL(CStrF("%10.3f, Completed update frame %d", GetCPUClock() / GetCPUFrequency(), pCI->m_FrameOut.m_SimulationTick));
							pCI->m_bFrameOut = false;

							if (pCI->m_spLocalClient != NULL)
							{
/*								CNetMsg OutPacket(WPACKET_LOCALFRAMEFRACTION);
							ConOut(CStrF("Fraction %f", m_Simulate_LocalFrameFraction));	
								OutPacket.Addfp32(m_Simulate_LocalFrameFraction);
								if (!Net_PutMsg(iC, OutPacket))
									ConOut("§cf80WARNING: Unable to send frame fraction to local client.");*/
							//	pCI->m_spLocalClient->Net_OnMessage(OutPacket);		
							}
						}
					}
					else 
						break;
				}

//CHECKMEMORY("Net_Refresh");
				if (pCI->m_bFrameOut)
				{
					ConOut(CStrF("§cf80WARNING: Complete game state update could not be sent. (Size %d)", pCI->m_FrameOut.GetSize() ));
				}

				if (!pCI->m_bFrameOut)
				{
if (m_bNetShowMsg) 
	LogFile(CStrF("Client update complete, iC %d, UpdateMask %.8x (1)", iC, pCI->m_ServerUpdateMask));

					if (pCI->m_ServerUpdateMask & SERVER_CLIENTUPDATE_COMPLETEFRAME)
					{
						if (pCI->m_bFrameComplete)
						{
							pCI->m_ServerUpdateMask &= ~(SERVER_CLIENTUPDATE_DELTAFRAME | SERVER_CLIENTUPDATE_COMPLETEFRAME);
if (m_bNetShowMsg) 
	LogFile(CStrF("Client update complete, iC %d, UpdateMask %.8x", iC, pCI->m_ServerUpdateMask));
						}
						else
if (m_bNetShowMsg) 
	LogFile(CStrF("Client update not complete, iC %d, UpdateMask %.8x", iC, pCI->m_ServerUpdateMask));
					}
					else
						pCI->m_ServerUpdateMask &= ~(SERVER_CLIENTUPDATE_DELTAFRAME | SERVER_CLIENTUPDATE_COMPLETEFRAME);

/*					if (pCI->m_bFrameComplete || !(pCI->m_ServerUpdateMask & SERVER_CLIENTUPDATE_COMPLETEFRAME))
						pCI->m_ServerUpdateMask &= ~(SERVER_CLIENTUPDATE_DELTAFRAME | SERVER_CLIENTUPDATE_COMPLETEFRAME);
					else if (pCI->m_bFrameComplete)
						pCI->m_ServerUpdateMask &= ~(SERVER_CLIENTUPDATE_DELTAFRAME | SERVER_CLIENTUPDATE_COMPLETEFRAME);*/
				}
				else
					continue;
//LogFile(CStrF("Client upd, dFrame. %d (Done)", iC));
			}

			if (pCI->m_ServerUpdateMask & SERVER_CLIENTUPDATE_DELTAREGISTRY)
			{
				TProfileDef(TUnpack); 
				{
					TMeasureProfile(TUnpack);
					if (!pCI->m_bRegFrameOut)
					{
	//CHECKMEMORY("Net_Refresh");
						pCI->m_RegFrameOut.Clear();
						pCI->m_bRegFrameComplete = CreateClientRegUpdate(iC, &pCI->m_RegFrameOut);
						if (pCI->m_bRegFrameComplete)
						{
							pCI->m_ServerUpdateMask &= ~(SERVER_CLIENTUPDATE_DELTAREGISTRY);
	//						ConOut(CStrF("RegUpdate %f ms", 1000.0*TUnpack / GetCPUFrequency()));
							goto RegDone; 
						}
	//CHECKMEMORY("Net_Refresh");

	//LogFile(CStrF("Client reg-upd %d, iC %d", pCI->m_RegFrameOut.m_Size, iC));
	//					if (pCI->m_hConnection != -1)
						pC->Net_UnpackClientRegUpdate(&pCI->m_RegFrameOut);
						if (bIsLocal)
							pC->OnRegistryChange();
						pCI->m_bRegFrameOut = true;
					}
				}
//		ConOut(CStrF("RegUpdate %f ms", 1000.0*TUnpack / GetCPUFrequency()));

				if (bIsLocal)
				{
					pCI->m_bRegFrameOut = false;
				}
				else
				{
					while(pCI->m_bRegFrameOut)
					{
						CNetMsg OutPacket(0);
						pCI->m_RegFrameOut.GetPacket(OutPacket, WPACKET_DELTAREGISTRY);

	//ConOut(CStrF("Client upd, dRegFrame. iC %d, size %d/%d, n %d", iC, pCI->m_RegFrameOut.m_IOPos, pCI->m_RegFrameOut.m_Size, pCI->m_RegFrameOut.m_nChunks));
	/*					if (!pCI->m_RegFrameOut.m_IOPos)
						{
							int Size = pCI->m_RegFrameOut.m_Data[0] + (pCI->m_RegFrameOut.m_Data[1] << 8);
							int nChunks = pCI->m_RegFrameOut.m_Data[2] + (pCI->m_RegFrameOut.m_Data[3] << 8);
	//						LogFile(CStrF("RegFrameSize %d, %d", Size, nChunks));
						}*/

						if (Net_PutMsg(iC, OutPacket))
						{
							if (pCI->m_RegFrameOut.GetNext())
							{
								pCI->m_bRegFrameOut = false;
							}
						}
						else 
							break;
					}
				}
//CHECKMEMORY("Net_Refresh");
//LogFile(CStrF("Client upd, dRegFrame. %d (Done)", iC));

				if (!pCI->m_bRegFrameOut)
				{
					if (pCI->m_bRegFrameComplete)
						pCI->m_ServerUpdateMask &= ~(SERVER_CLIENTUPDATE_DELTAREGISTRY);
				}
				else
					continue;
			}
RegDone:				// I'm so, so, sorry for this goto..   but it made everything so simple.. 

			// -------------------------------------------------------------------
			if (pCI->m_ServerUpdateMask & SERVER_CLIENTUPDATE_DOWNLOAD)
			{
LogFile(CStrF("(CWorld_ServerCore::Net_Refresh) Client upd, Download. %d", iC));
				Error("Refresh", "Shouldn't be here. Download not operational.");

/*				fp64 m_Time = m_spNetwork->GetTime();
				while((m_spNetwork->GetTime() - m_Time) < 0.002f)
				{
					CNetMsg OutPacket(WPACKET_FILEFRAGMENT);
					pC->m_Download_FilePos;
					pC->m_Download_FileLength;
					int Size = Min((int)sizeof(OutPacket.m_Data)-1, pC->m_Download_FileLength - pC->m_Download_FilePos);
					pC->m_Download_spFile->Seek(pC->m_Download_FilePos);
					OutPacket.m_Data[0] = Size;
					try { pC->m_Download_spFile->Read(&OutPacket.m_Data[1], Size); }
					catch(CCException)
					{ 
						OutPacket.m_Data[0] = 0xff;		// Abort! 
						Size = 0;
					}
					if (Net_PutMsg(iC, OutPacket))
//					if (m_spNetwork->SendPacket(hCon, &OutPacket, 2 + Size))
					{
						if (Size)
							pC->m_Download_FilePos += Size;
						else
							pC->m_Download_FilePos = pC->m_Download_FileLength;

						if (pC->m_Download_FilePos == pC->m_Download_FileLength)
						{
							pCI->m_ServerUpdateMask &= ~SERVER_CLIENTUPDATE_DOWNLOAD;
							pC->m_Download_spFile = NULL;
							pC->m_Download_FilePos = 0;
							pC->m_Download_FileLength = 0;
						}
					}
LogFile(CStrF("(CWorld_ServerCore::Net_Refresh) Client upd, Download. %d (Done)", iC));
				}*/

			}
		}

//CHECKMEMORY("Net_Refresh");
	Net_FlushMessages();

//LogFile("Checking for empty client slots...");
	// Remove empty client slots
//CHECKMEMORY("Net_Refresh");
/*	{
		for(int iC = m_lspClients.Len()-1; iC >= 0; iC--)
			if (m_lspClients[iC] == NULL)
			{
				m_lspClients.Del(iC);
				m_lspClientInfo.Del(iC);
			}
	}*/
//LogFile("Done...");
}

bool CWorld_ServerCore::Net_ClientCommand(int _iClient, const CStr& _Cmd)
{
	MAUTOSTRIP(CWorld_ServerCore_Net_ClientCommand, false);
	if (!_Cmd.Len()) return true;
	if (_iClient < 0 || _iClient >= m_lspClientInfo.Len()) return true;
	if (!m_lspClientInfo[_iClient]) return true;
	CNetMsg Msg(WPACKET_COMMAND);
	Msg.AddData((const char*)_Cmd, _Cmd.Len());
	return Net_PutMsg(_iClient, Msg);
}

bool CWorld_ServerCore::Net_ClientCommandAll(const CStr& _Cmd)
{
	MAUTOSTRIP(CWorld_ServerCore_Net_ClientCommandAll, false);
	if (!_Cmd.Len()) return true;
	CNetMsg Msg(WPACKET_COMMAND);
	Msg.AddData((const char*)_Cmd, _Cmd.Len());

	bool bSuccess = true;
	for(int iC = 0; iC < m_lspClientInfo.Len(); iC++)
		if (m_lspClients[iC] != NULL)
			bSuccess &= Net_PutMsg(iC, Msg);
	return bSuccess;
}

int CWorld_ServerCore::Net_ConOut(const CStr& _s, int _iClient)
{
	MAUTOSTRIP(CWorld_ServerCore_Net_ConOut, 0);
	if (!_s.Len()) return true;
	CStr Cmd = CFStrF("echo(\"%s\")", (const char*) _s);
	if (_iClient == -1)
		return Net_ClientCommandAll(Cmd);
	else
		return Net_ClientCommand(_iClient, Cmd);
}

void CWorld_ServerCore::Net_SetClientVar(int _iClient, const CStr& _Key, const CStr& _Value)
{
	MAUTOSTRIP(CWorld_ServerCore_Net_SetClientVar, MAUTOSTRIP_VOID);
	CRegistry *pReg = Registry_GetClientVar(_iClient);
	if(pReg)
		pReg->SetValue(_Key, _Value);
}

// -------------------------------------------------------------------
void CWorld_ServerCore::NetMsg_SendToObject(const CNetMsg& _Msg, int _iObj, int _MaxLag)
{
	MAUTOSTRIP(CWorld_ServerCore_NetMsg_SendToObject, MAUTOSTRIP_VOID);

	CWObject* pObj = Object_Get(_iObj);
	if (!pObj)
		return;
	CNetMsg Msg(WPACKET_OBJNETMSG);
	Msg.AddInt8(_Msg.m_MsgType);
	Msg.AddInt16(_iObj);
	Msg.AddInt16(pObj->m_iClass);
	if (Msg.AddData((void*)_Msg.m_Data, _Msg.m_MsgSize))
	{
		Net_PutMsgAll(Msg, -1, WCLIENT_STATE_INGAME, _MaxLag);
	}
	else
		ConOut("(CWorld_ServerCore::NetMsg_SendToObject) Too large object net-message. (1)");
}

void CWorld_ServerCore::NetMsg_SendToClass(const CNetMsg& _Msg, int _iClass, int _MaxLag)
{
	MAUTOSTRIP(CWorld_ServerCore_NetMsg_SendToClass, MAUTOSTRIP_VOID);
	CNetMsg Msg(WPACKET_NULLOBJNETMSG);
	Msg.AddInt8(_Msg.m_MsgType);
	Msg.AddInt16(_iClass);
	if (Msg.AddData((void*)_Msg.m_Data, _Msg.m_MsgSize))
	{
		Net_PutMsgAll(Msg, -1, WCLIENT_STATE_INGAME, _MaxLag);
	}
	else
		ConOut("(CWorld_ServerCore::NetMsg_SendToClass) Too large object net-message. (2)");
}

void CWorld_ServerCore::NetMsg_SendToObject_Culled(const CVec3Dfp32& _PVSPos, const CNetMsg& _Msg, int _iObj, int _MaxLag)
{
	MAUTOSTRIP(CWorld_ServerCore_NetMsg_SendToObject_Culled, MAUTOSTRIP_VOID);
	NetMsg_SendToObject(_Msg, _iObj, _MaxLag);
}

void CWorld_ServerCore::NetMsg_SendToObject_Culled(const CBox3Dfp32& _PVSPos, const CNetMsg& _Msg, int _iObj, int _MaxLag)
{
	MAUTOSTRIP(CWorld_ServerCore_NetMsg_SendToObject_Culled_2, MAUTOSTRIP_VOID);
	NetMsg_SendToObject(_Msg, _iObj, _MaxLag);
}

void CWorld_ServerCore::NetMsg_SendToClient(const CNetMsg& _Msg, int _iClient, int _MaxLag)
{
	MAUTOSTRIP(CWorld_ServerCore_NetMsg_SendToClient, MAUTOSTRIP_VOID);
	if (_iClient == -1)
		Net_PutMsgAll(_Msg);
	else
		Net_PutMsg(_iClient, _Msg);
}

void CWorld_ServerCore::NetMsg_SendToClient_Culled(const CVec3Dfp32& _PVSPos, const CNetMsg& _Msg, int _iClient, int _MaxLag)
{
	MAUTOSTRIP(CWorld_ServerCore_NetMsg_SendToClient_Culled, MAUTOSTRIP_VOID);
	NetMsg_SendToClient(_Msg, _iClient, _MaxLag);
}

void CWorld_ServerCore::NetMsg_SendToClient_Culled(const CBox3Dfp32& _PVSPos, const CNetMsg& _Msg, int _iClient, int _MaxLag)
{
	MAUTOSTRIP(CWorld_ServerCore_NetMsg_SendToClient_Culled_2, MAUTOSTRIP_VOID);
	NetMsg_SendToClient(_Msg, _iClient, _MaxLag);
}


