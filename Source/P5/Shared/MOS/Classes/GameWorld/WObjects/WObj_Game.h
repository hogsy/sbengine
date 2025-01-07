#ifndef __WOBJ_GAME_H
#define __WOBJ_GAME_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Base GameObject Class

	Author:			Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWObject_Game
					CWObject_GameSettings
\*____________________________________________________________________________________________*/


#include "../WObjCore.h"
#include "../Client/WClientClass.h"

enum
{
	OBJMSG_GAME_CONTROL = 0x0080,
	OBJMSG_GAME_RENDERSTATUSBAR,
	OBJMSG_GAME_NEVERRENDERWORLD,
	OBJMSG_GAME_SETRENDERWORLD,
	OBJMSG_GAME_GETGAMESTATE,
	OBJMSG_GAME_SETCLIENTWINDOW,
	OBJMSG_GAME_PIPEMESSAGETOWORLD,
	OBJMSG_GAME_LOADPRECACHEINFO,
	OBJMSG_GAME_LOADANIMATIONS,

	OBJMSG_PLAYER_GETSPECIALCLASS,
	OBJMSG_PLAYER_GETPHYSTYPE,

	OBJMSG_GAME_SPAWN				= 0x100b,	// Moved from WObj_Character. ID is reserver
	OBJMSG_GAME_CHANGEWORLD			= 0x10c,
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CWO_Player
					
	Comments:		Base class for Player objects
					Player objects are used primary to be the link
					between a Client index and the player object
					(CWObject_Character for example). This link
					is used to poll input from the server among
					other things.

					CWO_Player is supposed to be inherited, and to
					contain game-specif
\*____________________________________________________________________*/

class CWO_Player : public CWC_Player
{
public:
	int m_iObject;			// Current controlled object
	int m_iClient;			// Connected client	(remove ?)
	int m_iPlayer;			// Index of this structure in CWObject_Game::m_lspPlayers (remove?)
	int m_iServerPlayer;	// Index of players structure on Server (remove)
	int m_bCtrlValid;
	int m_Mode;				// Free to be used as see fit
	
	CWO_Player()
	{
		m_iObject = -1;
		m_iPlayer = -1;
		m_iClient = -1;
		m_iServerPlayer = -1;
		m_bCtrlValid = true;
		m_Mode = 0;
	}

	bool IsClient() { return m_iClient >= 0; }
};

typedef TPtr<CWO_Player> spCWO_Player;


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CWObject_Game
					
	Comments:		Base class for Game object. Handles game specific
					initialization, world changes etc.
					Since this is the Shared GameObject, not much
					functionality is includes since most such code
					is very project specific
\*____________________________________________________________________*/

class CWObject_Game : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	CStr m_GameMenu;
	TArray<TPtr<CWO_Player> > m_lspPlayers;

	CWObject_Game();
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual void OnRefresh();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	
	// Player functions
	virtual spCWO_Player CreatePlayerObject(); // Override this to use custom Player-classes
	
	virtual int Player_Add(spCWO_Player _spPlayer, int _iClient);
	virtual void Player_Destroy(int _iPlayer);
	void Player_DestroyObject(int _iPlayer);
	virtual void Player_DestroyAllObjects();
	virtual void Player_SetObject(int _iPlayer, int _iObj);
	virtual int Player_Respawn(int _iPlayer, const CMat4Dfp32& _Pos, const char* _pClassName, int _Param1, int _iObj = -1);
	const CControlFrame* Player_GetControlFrame(int _iPlayer);
	bool Player_ConOut(int _iPlayer, const char *_pSt);
	virtual void Player_OnSpawn(int _iPlayer) {}
		
	virtual int Player_GetNum();
	virtual CWO_Player *Player_Get(int _iPlayer);
	CWO_Player *Player_GetWithClient(int _iClient);
	CWO_Player *Player_GetWithObject(int _iObject);
	CWO_Player *Player_GetWithServerPlayer(int _iPlayer);
	int Player_GetClient(int _iPlayer);
	virtual CWObject *Player_GetObject(int _iPlayer);
	int Player_GetObjectIndex(int _iPlayer);
	
	// Game state functions
	int GetGameState();
	static int GetGameState(const CWObject_CoreData *_pObj);
	void SetGameState(int _GameState);
	virtual void RefreshGameState() {}
	virtual void RefreshClientGameState(int _iClient) {}
	
	// Message wrappers
	virtual int OnClientSay(int _iClient, int _Mode, const char *_pSt);
	virtual int OnChangeWorld(const char *_pWorld, int _Flags, int _iSender) { return 0; }
	virtual int OnSetClientWindow(int _iClient, const char *_pWnd) { return 0; }
	virtual int OnInitWorld() { return 1; }
	virtual int OnCloseWorld() { return 1; }
	virtual int OnClientConnected(int _iClient);
	virtual int OnAddClient(int _iClient);
	virtual int OnRemoveClient(int _iClient, int _Reason);
	virtual int OnClientCommand(int _iPlayer, const CCmd* _pCmd) { return 0; }
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CWObject_Game_Settings
					
	Comments:		Simple class to transfer level specific settings
					to the game object. All keys that are set to every
					Game_Settings object in a level is automatically
					transferred to the current GameObject through
					OnEvalKey.
\*____________________________________________________________________*/

class CWObject_Game_Settings : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
};


#endif // _INC_WOBJ_GAME
