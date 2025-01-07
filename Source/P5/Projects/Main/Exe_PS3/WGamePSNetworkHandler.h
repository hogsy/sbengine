/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			The PlayStation Network handler

Author:			Roger Mattsson

Copyright:		2006 Starbreeze Studios AB

Contents:		CWGame_PSNetworkHandler

Comments:

History:		
060606:		Created file
\*____________________________________________________________________________________________*/
#ifndef __WGame_PSNetworkHandler_h__
#define __WGame_PSNetworkHandler_h__

#include "np.h"
#include "sys/time.h"
#include "sys/select.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "netex/errno.h"
#include "netex/net.h"
#include "sysutil/sysutil_common.h"

#include "../Exe/WGameMultiplayerHandler.h"

#define NP_POOL_SIZE (128*1024)

#define CONTEXT_GAME_MODE 1		//This define is so we are more compatible with the LIVE code, less to rewrite

class CWGamePSNetworkHandler : public CWGameMultiplayerHandler
{
#define SOCKET_VOICE_PORT 1030
#define SOCKET_BROAD_PORT 1031
#define SOCKET_TCP_PORT 1032
public:
	CWGamePSNetworkHandler();
	virtual bool Initialize();
	virtual bool InitializeLAN(void);
	bool InitializeVoice(void);
	virtual bool InitializeTCPSocket(SOCKET *s, uint8 _port_adjust = 0);
	virtual void Shutdown(void);
	virtual void ShutdownSession(bool _NoReport = false, bool _SendFailed = false);	//We can skip reporting if we leave a ranked game before it begins
	virtual void ShutdownLAN(void);
	void ShutdownVoice(void);

	virtual void Update(void); 
	virtual void Broadcast(void);
	virtual bool SearchForSessions(void);
	virtual void CreateSession(void);
	virtual void Quickgame(void);
	virtual void Customgame(void);
	virtual void JoinSession(uint16 _iServer, bool _bFromFriend = false);
	virtual bool ReceiveFromSocket(SOCKET *s, bool _bConnectedSocket, int8 _client);

	void ReceiveMessage(void);
	virtual void HandleMessage(SLiveMsg *_msg, sockaddr_in _addr, int _client = -1);
	virtual bool AddUserToSession(ClientInfo* _pClient);
	virtual void ClientDropped(int8 _client, int _player_id = 0);
	virtual void StartGame(void);
	virtual void Connect(void);	
	virtual bool SendMessage(SLiveMsg *_msg, int8 client = -1, bool _broad = false, bool _voice = false);

	void CleanupSearchAndRegResults(void);
	void ClearNPIDs(void);
	SceNpId GetNPID(int _PlayerId);
	int SetNPID(SceNpId _npid, int _player_id = 0);

private:
	//Callback
	static int BasicEventHandlerCB(int _event, int _ret_code, uint32_t _req_id, void *_arg);
	static void MatchingHandlerCB(uint32_t _ctx_id, uint32_t _req_id, int _event, int _error_code, void *arg);
	static void MatchingGUIHandlerCB(uint32_t _ctx_id, int _event, int _error_code, void *_arg);
	static void SignalingHandlerCB(uint32_t _ctx_id, uint32_t _subject_id, int _event, int _error_code, void *_arg);

	SceNpTitleId		m_TitleID;
	SceNpLobbyId		m_LobbyID;	
	SceNpRoomId			m_RoomID;
	uint32_t			m_Context;
	uint32_t			m_SignalingContext;
	uint32_t			m_RequestID;
	bool				m_bGetGUIData:1;
	bool				m_bWaitOnConnection:1;
	bool				m_bConnectionLost:1;
	uint32_t			m_ConnectionIDLost;
	uint8_t				m_NPPool[NP_POOL_SIZE];
	CMTime				m_WaitOnConnectionTimer;

	TThinArray<SceNpMatchingAttr>				m_Attr;	
	TThinArray<SceNpMatchingSearchCondition>	m_SearchCond;
	TThinArray<SceNpMatchingAttr>				m_SearchAttr;
	SceNpMatchingJoinedRoomInfo					*m_pJoinedRoom;	//Where should this be deleted,when is this needed? atm it's deleted in CleanupSearchAndReg
	SceNpMatchingSearchJoinRoomInfo				*m_pSearchJoinRoom;

	struct SPlayerID
	{
		SceNpId np_id;
		int player_id;
	};
	TArray<SPlayerID>			m_lPlayerIDs;
};

#endif

