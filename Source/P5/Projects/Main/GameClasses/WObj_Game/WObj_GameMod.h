#ifndef __WOBJ_GAMEMOD_H
#define __WOBJ_GAMEMOD_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_GameMod
					
	Author:			Jens Andersson
					
	Copyright:		
					
	Contents:		
					
	History:		
		020808:		Created File
		051???:		(Roger) Added multiplayer modes(DM/TDM/CTF)
		060817		(Roger) Added Survivor and Darklings vs Humans mode
		060830		(Roger) Added LastHuman mode
\*____________________________________________________________________________________________*/

#include "WObj_GameCore.h"
#include "WObj_GameMessages.h"
#include "../WObj_AI/AI_ResourceHandler.h"

#ifdef PLATFORM_XENON
#include <xtl.h>
#endif

enum
{
	OBJMSG_GAMEP4_SETMISSION = 0x130,
	OBJMSG_GAMEP4_COMPLETEMISSION = 0x131,
	OBJMSG_GAMEP4_FAILMISSION = 0x132,
	OBJMSG_GAMEP4_CHECKPOINT = 0x133,
	OBJMSG_GAMEP4_SETNVRANGE = 0x134,
	
	OBJMSG_GAMEP4_GETNIGHTVISIONLIGHTRANGE = OBJMSG_GAMECORE_LAST,
	OBJMSG_GAMEP4_DESTROYVOICES,
	OBJMSG_GAMEP4_SAVEPRECACHEINFO,

	OBJMSG_GAMEMOD_RENDERMULTIPLAYERSTATUS,

	NETMSG_GAME_DESTROYVOICES = 0x40,
	NETMSG_GAME_PLAYER_DISCONNECT,
	NETMSG_GAME_PLAYERKILL,
	NETMSG_GAME_PLAYERSUICIDE,
	NETMSG_GAME_TEAMKILL,
	NETMSG_GAME_FLAGSTOLEN,
	NETMSG_GAME_FLAGCAPTURED,
	NETMSG_GAME_FLAGRETURNED,
	NETMSG_GAME_ENEMY_FLAGSTOLEN,
	NETMSG_GAME_ENEMY_FLAGCAPTURED,
	NETMSG_GAME_ENEMY_FLAGRETURNED,
	NETMSG_GAME_SETNAME,
	NETMSG_GAME_JOIN,
	NETMSG_GAME_JOIN_TEAM,
	NETMSG_GAME_ACHIEVEMENT,	//Used to give a client an achievement (MP achievements)

	PRECACHEINFO_RPG = 0,
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CWObject_GameP4
					
	Comments:		Pitch Black specific GameObject base-class
\*____________________________________________________________________*/
class CWObject_GameP4 : public CWObject_GameCore
{
public:
	enum
	{
		GAMESTYLE_RESET = 0,	// Not really a gamestyle, just a value telling us to restore
		GAMESTYLE_SNEAK = 1,
		GAMESTYLE_FIGHT = 2,
		GAMESTYLE_ADVENTURE = 3,
		GAMESTYLE_KILL = 4,
	};
protected:
	enum
	{
		PLAYERMODE_NORMAL = 0,
		PLAYERMODE_DEAD,
		PLAYERMODE_WAITINGFORRESPAWN,
	};

	int m_iRailObject;
	int m_MinimumClearanceLevel;
	int m_Gamestyle;
	int m_DefaultGamestyle;

	fp32 m_DamageTweak_Melee[NUM_DIFFICULTYLEVELS];
	fp32 m_DamageTweak_Ranged[NUM_DIFFICULTYLEVELS];
	fp32 m_GlobalDamageTweak_Melee[NUM_DIFFICULTYLEVELS];
	fp32 m_GlobalDamageTweak_Ranged[NUM_DIFFICULTYLEVELS];

	int m_DifficultyLevel;
	TArray<CStr> m_lDefaultTeams;

	uint32 m_NightVisionLightRange;
	int8 m_bLightsOut : 1;
	int8 m_bNoHintIcon : 1;
	int8 m_bRailForgetEnemies : 1;

	CStr m_LastSave;
	uint32 m_LastLoadTick;
	uint32 m_LastSaveTick;

	CStr m_StartLevel;
	CStr m_AutoSave;

	CStr m_DeathSeqPast;
	CStr m_DeathSeqPresent;
	CStr m_DeathSeqFuture;
	CStr m_DeathSeqSound;

	TArray<uint8> m_DelayedAchievments;

	//Handler for global AI resources
	CAI_ResourceHandler m_AIResources;
	int m_lAIParams[GAME_NUM_AIPARAM];

protected:
	CWObject_GameP4();

	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnRefresh();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	static void OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg);
	virtual void Player_OnSpawn(int _iPlayer);
	virtual int OnClientConnected(int _iClient);
	virtual int OnInitWorld();
	virtual int OnCloseWorld();
	virtual void OnSpawnWorld();
	virtual int OnClientCommand(int _iPlayer, const CCmd* _pCmd);
	static void OnClientRenderStatusBar(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D, CVec2Dfp32 *_rect = NULL);

	virtual CStr GetDefualtSpawnClass();

	virtual void OnDeltaSave(CCFile *_pFile);
	virtual void OnDeltaLoad(CCFile *_pFile, int _Flags);
	virtual int OnChangeWorld(const char *_pWorld, int _Flags, int _iSender);

	int SetMission(int _MissionID, const char *_pSt);
	int RemoveMission(int _MissionID, bool _bCompleted);

	void AwardDelayedAchievements(void);
	static void AwardAchievement(int _ID, bool _bSaveProfile);

	void OnSpawnRailHandler();		// Called from OnSpawnWorld
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CWObject_GameCampaign
					
	Comments:		Pitch Black specific GameObject base-class
\*____________________________________________________________________*/
class CWObject_GameCampaign : public CWObject_GameP4
{
	MRTC_DECLARE_SERIAL_WOBJECT;
protected:
	enum
	{
		PLAYERMODE_NORMAL = 0,
		PLAYERMODE_DEAD = 1,
	};
			
	virtual int OnInitWorld();
	virtual int OnClientConnected(int _iClient);
	virtual int OnCharacterKilled(int _iObject, int _iSender);
	virtual void OnRefresh();
};


#ifdef M_Profile
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CWObject_GameDebug
					
	Comments:		Debug game mode
\*____________________________________________________________________*/
class CWObject_GameDebug : public CWObject_GameP4
{
public:
	CWObject_GameDebug();
	MRTC_DECLARE_SERIAL_WOBJECT;

protected:
	class CWO_PlayerDebug : public CWO_Player
	{
	public:
		CStr m_Class;
	};

	CWO_PlayerDebug *Player_GetDebug(int _iPlayer);
	
	virtual int OnInitWorld();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual int OnClientConnected(int _iClient);
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual int OnClientCommand(int _iPlayer, const CCmd* _pCmd);
	virtual void OnRefresh();
	virtual int OnCharacterKilled(int _iObject, int _iSender);
	
	virtual spCWO_Player CreatePlayerObject();
};

#endif


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CWObject_GameDM
					
	Comments:		Multiplayer death match game mode
\*____________________________________________________________________*/
class CWObject_GameDM : public CWObject_GameP4
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	typedef CWObject_GameP4 parent;

#define WARMUPCOUNTDOWN 5.0f

public:
	CWObject_GameDM();

	class CWO_PlayerDM : public CWO_Player
	{
	public:
		CWO_PlayerDM()
		{
			m_Score = 0;
			m_Deaths = 0;
			m_bIsHuman = true;
			m_bJoined = false;
			m_bReady = false;
			m_Mode = 0;
			m_DeathTick = 0;
			m_KillsInARow = 0;
		}

		CStr	m_ClassHuman;
		CStr	m_ModelDarkling;
		bool	m_bIsHuman:1;
		bool	m_bJoined:1;
		bool	m_bReady:1;
		int16	m_Score;
		int16	m_KillsInARow;
		uint16	m_Deaths;
		uint32	m_DeathTick;
		uint32	m_PickupTrigger;
		int		m_player_id;	//This is so we now which client in LiveHandler this player is, needed for stats
	};

	struct SPlayerInfo
	{
		uint16 m_SortKey;	//score
		int16 m_Deaths;
		CStr m_Name;
	};

	CWO_PlayerDM		*Player_GetDM(int _iPlayer);

	virtual int			OnInitWorld();
	virtual void		OnSpawnWorld();
	virtual void		OnSpawnWorld2();
	virtual int			OnClientCommand(int _iPlayer, const CCmd* _pCmd);
	virtual int			OnClientConnected(int _iClient);
	static aint			OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	static void			OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg);
	static void			OnClientRenderMultiplayerStatus(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D);
	static void			OnClientRenderStatusBar(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D, CVec2Dfp32 *_rect = NULL);
	static CStr			GetStatusBarText(int _iText, CClientData* _pCD);
	static void			GetStatusBarTextImportant(int _iText, CClientData* _pCD, CStr &_Title, CStr &_Text);
	virtual int			OnCharacterKilled(int _iObject, int _iSender);
	virtual int 		GetHighestScore(void);
	virtual void		GivePoint(int _iPlayer, int _nPoints);
	static void			OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);
	virtual aint		OnMessage(const CWObject_Message& _Msg);
	virtual void		OnRefresh();
	virtual int			OnRemoveClient(int _iClient, int _Reason);
	virtual void		OnPickUp(int _iPlayer, int _iObject, int _TicksLeft);
	virtual CStr		GetDefualtSpawnClass();
	virtual void		CheckForWinner(void);
	virtual void		RespawnAll(bool _bAlreadySpawned = false);
	virtual void		ToggleDarklingHuman(int16 _iSender, bool _bForce = false);
	virtual int			Player_Respawn(int _iPlayer, const CMat4Dfp32& _Pos, const char* _pClassName, int _Param1, int _iObj = -1);

	virtual spCWO_Player CreatePlayerObject(void);

	CMat4Dfp32			GetSpawnPosition(int _Team = -1);
	void				AddKill(int _iPlayer);

	virtual void		EndGame(void); //Called when time is out or someone won

protected:
	bool				m_bDoOnce;
	int					m_nNumPlayersNeeded;

	enum
	{
		MP_MODE_SHAPESHIFTER,
		MP_MODE_DARKLINGS,
		MP_MODE_DARKLINGS_VS_HUMANS,

		MP_WARMUP_MODE_NONE = 0,
		MP_WARMUP_MODE_PREGAME,
		MP_WARMUP_MODE_BETWEEN_ROUNDS,
	};

	struct CWObject_PickupInfo
	{
		CStr			m_TemplateName;
		CVec3Dfp32		m_Pos;
		uint32			m_SpawnTick;
		int16			m_TicksLeft;
	};

	TList_Linked<CWObject_PickupInfo>	m_lPickups;

	TArray<int>			m_liDummyDarklings;
	int					m_GameMode;
	int					m_GameOverTick;
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Class:			CWObject_GameTDM

Comments:		Multiplayer team death match game mode
\*____________________________________________________________________*/
class CWObject_GameTDM : public CWObject_GameDM
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	typedef CWObject_GameDM parent;

public:
	CWObject_GameTDM();

	class CWO_PlayerTDM : public CWO_PlayerDM 
	{
	public:
		CWO_PlayerTDM()
		{
			m_Team = 0;
		}

		uint8	m_Team;
	};

	struct SPlayerInfo
	{
		uint16 m_SortKey;	//score
		int16 m_Deaths;
		CStr m_Name;
		uint8 m_Team;
	};

	CWO_PlayerTDM		*Player_GetTDM(int _iPlayer);

	virtual int			OnClientConnected(int _iClient);
	virtual int			OnClientCommand(int _iPlayer, const CCmd* _pCmd);
	static aint			OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	static void			OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg);
	static void			OnClientRenderMultiplayerStatus(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D);
	static void			OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);
	virtual int			OnRemoveClient(int _iClient, int _Reason);
	virtual int			OnCharacterKilled(int _iObject, int _iSender);
	virtual aint		OnMessage(const CWObject_Message& _Msg);
	virtual void		OnRefresh();
	virtual void		GivePoint(int _iPlayer, int _nPoints);
	virtual void		ValidateHumanModelForTeam(int _iPlayer);
	virtual void		ValidateDarklingModelforTeam(int _iPlayer);

	virtual void		RespawnAll(bool _bAlreadySpawned = false);
	virtual void		EndGame(void);
	virtual spCWO_Player CreatePlayerObject();
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Class:			CWObject_GameCTF

Comments:		Multiplayer capture the flag game mode
\*____________________________________________________________________*/
class CWObject_GameCTF : public CWObject_GameTDM
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	typedef CWObject_GameTDM parent;

public:
	CWObject_GameCTF();

	class CWO_PlayerCTF : public CWO_PlayerTDM 
	{
	public:
		CWO_PlayerCTF()
		{
			m_Team = 0;
			m_bHasFlag = false;
		}

		bool	m_bHasFlag;
	};

	virtual spCWO_Player CreatePlayerObject();
	
	void				GetFlagSpawnPosition(void);

	virtual int			OnClientConnected(int _iClient);
	static aint			OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	static void			OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg);
	static void			OnClientRenderMultiplayerStatus(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D);
	virtual int			OnCharacterKilled(int _iObject, int _iSender);
	void				OnFlagPickedUp(int _iPlayer, int _iObject);
	static void			OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);
	virtual int			OnInitWorld();
	virtual void		GivePoint(int _iPlayer, int _nPoints);
	virtual aint		OnMessage(const CWObject_Message& _Msg);
	virtual int			OnRemoveClient(int _iClient, int _Reason);
	virtual void		OnPickUp(int _iPlayer, int _iObject, int _TicksLeft);
	virtual void		OnSpawnWorld();
	virtual void		OnRefresh();

	CWO_PlayerCTF		*Player_GetCTF(int _iPlayer);

	void				ToggleDarklingHuman(int16 _iSender, bool _bForce = false);

	virtual void		EndGame(void);

private:
	uint8				m_NumCaptures[2];
	CMat4Dfp32			m_FlagSpawnPos[2];

	int8				m_MaxCaptures;
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Class:			CWObject_GameSurvivor

Comments:		Multiplayer survivor game mode
				Last human to live wins. When killed players turn 
				into darklings
\*____________________________________________________________________*/
class CWObject_GameSurvivor : public CWObject_GameDM
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	typedef CWObject_GameDM parent;

public:
	CWObject_GameSurvivor();

	class CWO_PlayerSurvivor : public CWO_PlayerDM 
	{
	public:
		CWO_PlayerSurvivor()
		{
			m_SpawnAsDarkling = 0;
		}
		uint8 m_SpawnAsDarkling;
	};

	struct SPlayerInfo
	{
		uint16 m_SortKey;	//score
		int16 m_Deaths;
		CStr m_Name;
	};

	CWO_PlayerSurvivor	*Player_GetSurvivor(int _iPlayer);

	virtual aint		OnMessage(const CWObject_Message& _Msg);
	virtual int			OnClientConnected(int _iClient);
	virtual int			OnClientCommand(int _iPlayer, const CCmd* _pCmd);
	static aint			OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	virtual int			OnCharacterKilled(int _iObject, int _iSender);
	virtual int			OnRemoveClient(int _iClient, int _Reason);
	static void			OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);
	virtual void		ToggleDarklingHuman(int16 _iSender, bool _bForce = false);
	virtual void		CheckForWinner(void);
	virtual void		RespawnAll(bool _bAlreadySpawned = false);

	virtual spCWO_Player CreatePlayerObject();

	int					m_iNextToBeDarkling;	//First player to die, if -1 take a random player
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Class:			CWObject_GameLastHuman (Rambo mode)

Comments:		Multiplayer LastHuman game mode
				Human gets infinite ammo and 2 smgs, is alone against all
				darklings, when killed human is turned to a darkling and
				the killer gets to be human.
				Only human can get score.
\*____________________________________________________________________*/
class CWObject_GameLastHuman : public CWObject_GameSurvivor
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	typedef CWObject_GameSurvivor parent;

public:
	CWObject_GameLastHuman();

	class CWO_PlayerLastHuman : public CWO_PlayerSurvivor 
	{
	};

	struct SPlayerInfo
	{
		uint16 m_SortKey;	//score
		int16 m_Deaths;
		CStr m_Name;
	};

	CWO_PlayerLastHuman	*Player_GetLastHuman(int _iPlayer);

	virtual void		OnSpawnWorld();
	virtual aint		OnMessage(const CWObject_Message& _Msg);
	virtual int			OnClientConnected(int _iClient);
	virtual int			OnClientCommand(int _iPlayer, const CCmd* _pCmd);
	virtual int			OnCharacterKilled(int _iObject, int _iSender);
	static void			OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);
	virtual void		CheckForWinner(void);
	virtual void		RespawnAll(bool _bAlreadySpawned = false);
	virtual void		OnRefresh(void);
	virtual void		GiveHumanWeapons(int _iPlayer);
	virtual void		OnPickUp(int _iPlayer, int _iObject, int _TicksLeft);

	virtual spCWO_Player CreatePlayerObject();

	int					m_iNextToBeHuman;	//First player to die, if -1 take a random player

private:
	TPtr<class CRPG_Object_Item>	m_spDummyObject;
	CStr							m_WeaponTemplate;
};

#endif
