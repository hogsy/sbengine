/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			The MultiplayerHandler

Author:			Roger Mattsson

Copyright:		2006 Starbreeze Studios AB

Contents:		CWGameMultiplayerHandler

Comments:		Base class for Live and Playstation Network classes

History:		
060604:		Created file
\*____________________________________________________________________________________________*/
#ifndef __WGame_MultiplayerHandler_h__
#define __WGame_MultiplayerHandler_h__

#define MAX_CLIENTS 8

#include "../Exe_Xenon/Darkness.spa.h"	//File with defines used for for game modes/maps/search methodes etc.

enum
{
	MP_FLAGS_INITIALIZED = M_Bit(0),
	MP_FLAGS_SESSIONVALID = M_Bit(1),
	MP_FLAGS_HOST = M_Bit(2),	
	MP_FLAGS_RANKED = M_Bit(3), 
	MP_FLAGS_INPROGRESS = M_Bit(4), 
	MP_FLAGS_SESSIONLOCKED = M_Bit(5), //The host has locked the session
	MP_FLAGS_LANGAME = M_Bit(6),
	MP_FLAGS_GAMESTARTED = M_Bit(7),
	MP_FLAGS_LOADING = M_Bit(8),
	MP_FLAGS_USEVOICE = M_Bit(9),
	MP_FLAGS_VOICEFRIENDSONLY = M_Bit(10),
	MP_FLAGS_VOICEAWAY = M_Bit(11),
	MP_FLAGS_RANDOMMAP = M_Bit(12),
	MP_FLAGS_SESSION_STARTED = M_Bit(13),
	//PS3 flags. perhaps move to PlaystationNetwork handler, but then it's easier to make a mistake if adding a flag
	MP_FLAGS_GUI_IN_PROGRESS = M_Bit(14), //PS3 only
	MP_FLAGS_WAITING_ON_CALLBACK = M_Bit(15), //PS3 only
	MP_FLAGS_SEARCHING = M_Bit(16), //PS3 only
	MP_FLAGS_CONNECTION_ESTABLISHED = M_Bit(17), //PS3 only
	MP_FLAGS_SEARCH_FAILED_CREATE_SESSION = M_Bit(18), //PS3 Only

	LOBBY_PLAYER_READY = M_Bit(0),
	LOBBY_PLAYER_VOICE = M_Bit(1),
	LOBBY_PLAYER_TALKING = M_Bit(2),
};

class CWGameMultiplayerHandler
{
public:
#ifdef PLATFORM_PS3	
#define SOCKET int
#define INVALID_SOCKET  (SOCKET)(~0)
#endif

	enum
	{
		//Generic
		LIVE_MSG_CLIENT_DROPPED,		//player_id
		LIVE_MSG_CONNECTION_REFUSED,	//no data
		LIVE_MSG_FAKE_PLAYER_ID,		//player_id
		LIVE_MSG_GAME_IN_PROGRESS,		//no data
		LIVE_MSG_GAME_OVER,				//no data
		LIVE_MSG_HEARTBEAT,				//no data
		LIVE_MSG_HEARTBEAT_DELAY,		//no data
		LIVE_MSG_JOIN_REQUEST,			//ClientInfo 
		LIVE_MSG_MATCH_OVER,			//no data
		LIVE_MSG_NEWCLIENT,				//ClientInfo
		LIVE_MSG_NONCE,					//ULONGLONG
		LIVE_MSG_READY,					//no data, toggles
		LIVE_MSG_READY_ALL,				//no data
		LIVE_MSG_START_GAME,			//no data
		LIVE_MSG_TEAM_ID,				//TeamID

		//Platform specific
		LIVE_MSG_BROADCAST_ANSWER,		//BroadcastAnswer
		LIVE_MSG_BROADCAST_SEARCH,		//ULONGLONG
		LIVE_MSG_CONNECTION_ACCEPTED,	//no data
		LIVE_MSG_CREATION_INFO,			//CreationInfo
		LIVE_MSG_GAME_MODE,				//DWORD
		LIVE_MSG_PLAYERID,				//player_id and XUID/SceNpId
		LIVE_MSG_QUIT,					//player_id
		LIVE_MSG_REGISTER,				//no data
		LIVE_MSG_REGISTER_DONE,			//no data
		LIVE_MSG_SERVER_INFO,			//ServerInfo
		LIVE_MSG_VOICE,					//Voice
		LIVE_MSG_WRITE_STATISTICS,		//no data

		SLOTS_PUBLIC = 0,
		SLOTS_PUBLIC_FILLED,
		SLOTS_PRIVATE,
		SLOTS_PRIVATE_FILLED,

		SLOTS_SIZE,
	};

	struct ClientInfo
	{
		in_addr		m_addr;								// IP address
		bool		m_bInvited:1;
		bool		m_bRegistered:1;							// has the peer registered for arbitration
		bool		m_bVoice:1;
		bool		m_bMuted:1;
		bool		m_bReady:1;
		bool		m_bConnected:1;
		int8		m_dropped;							//should be -1, if no the player has dropped, set dropped as new score (last)
		int			m_team_id;
		int			m_player_id;
#ifdef PLATFORM_XENON
		ULONGLONG	m_id;									// Machine ID
		BOOL		m_bPrivateSlot;
		XUID		m_xuid;
		wchar		m_gamertag[16];	// gamertag
#endif
#ifdef PLATFORM_PS3
		char		m_gamertag[SCE_NET_NP_ONLINENAME_MAX_LENGTH];	// gamertag
		bool		m_bPrivateSlot;
		SceNpId		m_npid;
		uint32_t	m_ConnectionID;	//Valid for all clients on server, only valid for m_Host on clients
#endif
		SOCKET		m_socket;		
	};

	//Msg must look like this for it to work with Live, but not PN? could strip this more here and just have
	//a CNetMsg and then add WORD m_cbGameData in Live::SendMsg?
#pragma pack(push)
#pragma pack(1)
	struct SLiveMsg
	{
		int16 m_cbGameData;

		struct 
		{
			uint8	m_id;
			uint16	m_size;	//size of union data, does not include header data
		} Header;

		CNetMsg m_Data;
	};
#pragma pack(pop)

	CWGameMultiplayerHandler();

	virtual bool Initialize(void);
	virtual bool InitializeTCPSocket(SOCKET *s, uint8 _port_adjust = 0);
	virtual bool InitializeLAN(void);

	virtual bool AddUserToSession(ClientInfo* _pClient);
	virtual void Broadcast(void);
	virtual bool SearchForSessions(void);
	virtual void CreateSession(void);
	virtual void Quickgame(void);
	virtual void Customgame(uint8 _CreateSession);
	virtual void JoinSession(uint16 _iServer, bool _bFromFriend = false) {};	//_bFromFriend - if it true the session is being joined through a friend invite or by selecting a friend and joining him, it's not from a search and thus is lacking data needed(slots, context, properties and so on)

	virtual void Shutdown(void);
	virtual void ShutdownSession(bool _NoReport = false, bool _SendFailed = false);	//We can skip reporting if we leave a ranked game before it begins
	virtual void ShutdownLAN(void);
	virtual void ClientDropped(int8 _client, int _player_id = 0);

	virtual bool SendMessage(SLiveMsg *_msg, int8 client = -1, bool _broad = false, bool _voice = false);
	virtual void SendMessageAll(SLiveMsg *_msg);

	virtual void ReceiveMessage(void);
	virtual bool ReceiveFromSocket(SOCKET *s, bool _bConnectedSocket, int8 _client);
	virtual void HandleMessage(SLiveMsg *_msg, sockaddr_in _addr, int _client = -1);
	virtual void Update(void); 

	virtual void StartGame(void);
	virtual void SetTeam(int _player_id, int _team);
	virtual void MatchOver(void);

	virtual void ToggleReadyStatus(void);	
	virtual void Connect(void);	//Connects to m_Host adress
	virtual void OnClientConnect(int _iClient);
	virtual void LoadingDone(void);	//Call this when this client has entered the game fully
	virtual void PrepareMatchOver(void);

	virtual bool CheckLoggedInAndMultiplayerRights(void);
	virtual int GetNumLanServers(void);
	virtual CStr GetLanServerInfo(int _iNum);
	virtual void ShowFriends(void);
	virtual void SetDefaultProfileSettings(CRegistry* _pOptions) {};

	virtual void ShowGamerCard(int _player_id);
	virtual void CleanupStats(void);	//Will clear up all stats from a leaderboard view
	virtual void ReadLeaderBoard(int32 _board, int _player_id = 0);
	virtual void ReadLeaderBoardFriends(void);
	virtual void StatisticsSetDeaths(int _value, int _player_id);
	virtual void StatisticsSetScore(int _value, int _player_id);
	virtual void StatisticsSetCaptures(int _value, int _player_id);
	virtual void AchievementComplete(int _AchievementID) {};	//IDs are in Darkness.spa.h, ACHIEVEMENT_*
	virtual void AwardGamerPicture(int _PictureID) {}; //IDs are in Darkness.spa.h, GAMER_PICTURE_*

	virtual int GetMapID(void);
	virtual CStr GetMapName(int _MapID);
	virtual CStr GetRandomMap(void);
	virtual bool UpdateMenu(void);

	virtual int GetPlayerID(int _player_index);
	virtual int GetPlayerLobbyStatus(int _player_index);
	virtual CStr GetPlayerName(int _player_index);
	virtual int GetNumPlayers();

	virtual bool IsTalking(int _player_index);
	CStr GetGameModeName(void);
	void SetNextGameMode(void);
	void SetNextMap(void);

	int						m_Flags;
	CStr					m_Map;

	CStr					m_WantedMenu;
	int						m_WantedMenuCount;

	//Information for creating a session
	uint8					m_Slots[SLOTS_SIZE];
	int						m_MaxPublicPlayers;
	int						m_MaxPrivatePlayers;
	int						m_TeamID;	//Used to give all players an unique ID for non team matches, otherwise GameMod will handle the team IDs
	
	//limits
	int32					m_TimeLimit;	//in seconds
	int32					m_ScoreLimit;
	int32					m_CaptureLimit;

	//Game parameters
	int						m_GameType;
	int						m_GameMode;	//FFA, TEAM, this is for different skill
	int						m_GameSubMode;	//TDM, CTF
	int						m_GameStyle;	//shapeshifter, darklings vs darklings
	int						m_SearchMethod;

	ClientInfo				m_Local;                     // information about the local client

protected:
	int						m_JoinStarted;
	SOCKET					m_ListenSocket;
	SOCKET					m_VDPSocket;	//VDP on Xenon, UDP on PS3
	SOCKET					m_BroadSocket;
	in_addr					m_HostAddr;

	CMTime					m_ConnectToServer;
	CMTime					m_LastHeartBeatFromServer;
	CMTime					m_LastHeartBeatSent;
	CMTime					m_JoinRequestTimeOut;
	TThinArray<CMTime>		m_LastHeartBeatReceived;

	bool					m_bAllReady:1;
	ClientInfo				m_Host;
	TThinArray<ClientInfo>	m_lClients;

	int						m_NextPlayerID;
	int						m_FakePlayerID;
};

#endif

