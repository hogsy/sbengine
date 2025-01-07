#include "PCH.h"
#include "WObj_Game.h"
#include "../../Shared/MOS/Classes/GameWorld/Server/WServer.h"

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Game, CWObject, 0x0100);


CWObject_Game::CWObject_Game()
{
	MAUTOSTRIP(CWObject_Game_ctor, MAUTOSTRIP_VOID);
}

void CWObject_Game::OnCreate()
{
	MAUTOSTRIP(CWObject_Game_OnCreate, MAUTOSTRIP_VOID);

	ClientFlags() |= CWO_CLIENTFLAGS_LINKINFINITE;
}

void CWObject_Game::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Game_OnEvalKey, MAUTOSTRIP_VOID);

	switch (_KeyHash)
	{
	case MHASH2('GAME','MENU'): // "GAMEMENU"
		{
			m_GameMenu = _pKey->GetThisValue();
			break;
		}

	default:
		{
			CWObject::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_Game::OnFinishEvalKeys()
{
	// Also parse all things that are defined in the server registry
	CRegistry *pReg = m_pWServer->Registry_GetServer();
	for(int i = 0; i < pReg->GetNumChildren(); i++)
	{
		const CRegistry* pKey = pReg->GetChild(i);
		OnEvalKey(pKey->GetThisNameHash(), pKey);
	}
}

void CWObject_Game::OnRefresh()
{
	MAUTOSTRIP(CWObject_Game_OnRefresh, MAUTOSTRIP_VOID);

	CWObject::OnRefresh();
}

aint CWObject_Game::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_Game_OnMessage, 0);

	switch(_Msg.m_Msg)
	{
	case OBJMSG_GAME_GETGAMESTATE:
		return GetGameState(this);
		
	case OBJSYSMSG_GAME_CLIENTSAY :
		return OnClientSay(_Msg.m_Param0, _Msg.m_Param1, (const char *)_Msg.m_pData);
		
	case OBJMSG_GAME_SETCLIENTWINDOW:
		return OnSetClientWindow(_Msg.m_Param1, (const char *)_Msg.m_Param0);
		
	case OBJMSG_GAME_CHANGEWORLD:
		return OnChangeWorld((const char *)_Msg.m_pData, _Msg.m_Param0, _Msg.m_iSender);
		
	case OBJSYSMSG_GAME_INITWORLD:
		return OnInitWorld();
		
	case OBJSYSMSG_GAME_CLOSEWORLD:
		return OnCloseWorld();
		
	case OBJSYSMSG_GAME_ADDCLIENT:
		return OnAddClient(_Msg.m_Param0);
		
	case OBJSYSMSG_GAME_CLIENTCONNECTED:
		return OnClientConnected(_Msg.m_Param0);
		
	case OBJSYSMSG_GAME_REMOVECLIENT:
		return OnRemoveClient(_Msg.m_Param0, _Msg.m_Param1);
		
	case OBJMSG_GAME_CONTROL:
		{
			if (!_Msg.m_pData) return 0;
			if (_Msg.m_DataSize != sizeof(CCmd)) return 0;

			return OnClientCommand(_Msg.m_Param0, (CCmd *)_Msg.m_pData);
		}
	}
	return CWObject::OnMessage(_Msg);
}

spCWO_Player CWObject_Game::CreatePlayerObject()
{
	spCWO_Player spP = MNew(CWO_Player);
	if (!spP) MemError("CreateChar");

	return spP;
}

int CWObject_Game::Player_Add(spCWO_Player _spPlayer, int _iClient)
{
	for(int i = 0; i < m_lspPlayers.Len(); i++)
		if (m_lspPlayers[i] == NULL)
		{
			_spPlayer->m_iPlayer = i;
			_spPlayer->m_iClient = _iClient;
			m_lspPlayers[i] = _spPlayer;
			return i;
		}

	_spPlayer->m_iPlayer = m_lspPlayers.Add(_spPlayer);
	_spPlayer->m_iClient = _iClient;
	return _spPlayer->m_iPlayer;
}

void CWObject_Game::Player_Destroy(int _iPlayer)
{
	if (m_lspPlayers[_iPlayer]->m_iObject >= 0)
		m_pWServer->Object_Destroy(m_lspPlayers[_iPlayer]->m_iObject);
	
	m_lspPlayers[_iPlayer]->m_iObject = -1;
	m_lspPlayers[_iPlayer]->m_Flags = 0;
	m_lspPlayers[_iPlayer] = NULL;
}

void CWObject_Game::Player_DestroyAllObjects()
{
	for(int i = 0; i < m_lspPlayers.Len(); i++)
		Player_DestroyObject(i);
}

void CWObject_Game::Player_DestroyObject(int _iPlayer)
{
	CWO_Player *pPlayer = m_lspPlayers[_iPlayer];
	if(pPlayer)
	{
		if (m_lspPlayers[_iPlayer]->m_iObject >= 0)
			m_pWServer->Object_Destroy(m_lspPlayers[_iPlayer]->m_iObject);
		Player_SetObject(_iPlayer, -1);
	}
}

int CWObject_Game::Player_Respawn(int _iPlayer, const CMat4Dfp32& _Pos, const char* _pClassName, int _Param1, int _iObj)
{
	uint32 GUID = 0;
	CWO_Player *pPlayer = m_lspPlayers[_iPlayer];
	if (pPlayer)
		GUID = m_pWServer->Object_GetGUID(pPlayer->m_iObject);

	Player_DestroyObject(_iPlayer);
	aint Params[2];
	Params[0] = _iPlayer;
	Params[1] = _Param1;

	int iObj = m_pWServer->Object_Create(_pClassName, _Pos, -1, Params, 2, _iObj, GUID);
	if (iObj < 0) return -1;

	Player_SetObject(_iPlayer, iObj);
	Player_OnSpawn(_iPlayer);

	return iObj;
}

const CControlFrame* CWObject_Game::Player_GetControlFrame(int _iPlayer)
{
//	if (_iPlayer < 0 || _iPlayer >= m_lspPlayers.Len())
//		return NULL;

	return &m_lspPlayers[_iPlayer]->m_CurrentControl;
}

bool CWObject_Game::Player_ConOut(int _iPlayer, const char *_pSt)
{
	int iC = Player_GetClient(_iPlayer);
	
	return (iC >= 0) ? m_pWServer->Net_ConOut(_pSt, iC) != 0 : false;
}

void CWObject_Game::Player_SetObject(int _iPlayer, int _iObj)
{
	// Notify old player-obj that it no longer is an object of this player.
	{
		CWObject* pObj = m_pWServer->Object_Get(m_lspPlayers[_iPlayer]->m_iObject);
		CRegistry_Dynamic Reg; Reg.SetThisKey("PLAYERNR", "-1");
		if (pObj) pObj->OnEvalKey(Reg.GetThisNameHash(), &Reg);
	}
	
	// Tell the new object that this is it's player.
	{
		m_lspPlayers[_iPlayer]->m_iObject = _iObj;
		CWObject* pObj = m_pWServer->Object_Get(m_lspPlayers[_iPlayer]->m_iObject);
		CRegistry_Dynamic Reg; Reg.SetThisKey("PLAYERNR", CStrF("%d", _iPlayer));
		if (pObj)
		{
			pObj->OnEvalKey(Reg.GetThisNameHash(), &Reg);
			m_lspPlayers[_iPlayer]->m_Flags |= PLAYER_FLAGS_PLAYER;
		}
		else
			m_lspPlayers[_iPlayer]->m_Flags &= ~PLAYER_FLAGS_PLAYER;
	}
	
	// Set object nr in this player's client-registry
	int iC = Player_GetClient(_iPlayer);
	if (iC >= 0)
	{
		CRegistry* pReg = m_pWServer->Registry_GetClient(iC, WCLIENT_REG_GAMESTATE);
		if (pReg) pReg->SetValuei("PLAYEROBJ", _iObj);
	}
	
	// Set game object nr i this player's client-registry
	if(_iObj != -1)
	{
		int iGame = m_pWServer->Game_GetObjectIndex();
		if(iGame >= 0)
		{
			CRegistry* pReg = m_pWServer->Registry_GetClient(iC, WCLIENT_REG_GAMESTATE);
			if (pReg) pReg->SetValuei("GAMEOBJ", iGame);
		}
	}
}

int CWObject_Game::Player_GetNum()
{
	return m_lspPlayers.Len();
}

CWO_Player *CWObject_Game::Player_Get(int _iPlayer)
{
//	if (m_lspPlayers.ValidPos(_iPlayer))
		return m_lspPlayers[_iPlayer];
//	else
//		return NULL;
}

CWO_Player *CWObject_Game::Player_GetWithClient(int _iClient)
{
	for(int i = 0; i < m_lspPlayers.Len(); i++)
	{
		CWO_Player* pP = m_lspPlayers.GetBasePtr()[i];
		if (pP && pP->m_iClient == _iClient)
			return pP;
	}
	
	return NULL;
}

CWO_Player *CWObject_Game::Player_GetWithObject(int _iObject)
{
	for(int i = 0; i < m_lspPlayers.Len(); i++)
	{
		CWO_Player* pP = m_lspPlayers[i];
		if (pP && pP->m_iObject == _iObject)
			return pP;
	}

	return NULL;
}

CWO_Player *CWObject_Game::Player_GetWithServerPlayer(int _iPlayer)
{
	for(int i = 0; i < m_lspPlayers.Len(); i++)
	{
		CWO_Player* pC = m_lspPlayers[i];
		if (pC && pC->m_iPlayer == _iPlayer)
			return pC;
	}
	
	return NULL;
}

int CWObject_Game::Player_GetClient(int _iPlayer)
{
	return m_lspPlayers[_iPlayer]->m_iClient;
}

CWObject *CWObject_Game::Player_GetObject(int _iPlayer)
{
	return m_pWServer->Object_Get(Player_GetObjectIndex(_iPlayer));
}

int CWObject_Game::Player_GetObjectIndex(int _iPlayer)
{
	CWO_Player *pPlayer = m_lspPlayers[_iPlayer];
	if(!pPlayer)
		return 0;
	return pPlayer->m_iObject;
}

int CWObject_Game::GetGameState(const CWObject_CoreData *_pObj)
{
	return _pObj->m_iAnim0 & 255;
}

void CWObject_Game::SetGameState(int _GameState)
{
	iAnim0() = (m_iAnim0 & 0xff00) | (_GameState & 255);
	
	SetAnimTick(m_pWServer, 0, 0);
	
	RefreshGameState();
	for(int i = 0; i < m_pWServer->Client_GetCount(); i++)
		RefreshClientGameState(i);
}

int CWObject_Game::OnClientSay(int _iClient, int _Mode, const char *_pSt)
{
	MAUTOSTRIP(CWObject_Game_OnClientSay, 0);

	return m_pWServer->Net_ConOut(_pSt, -1);
}

int CWObject_Game::OnAddClient(int _iClient)
{
	spCWO_Player spPlayer = CreatePlayerObject();
	int iPlayer = Player_Add(spPlayer, _iClient);
	m_pWServer->Client_SetPlayer(_iClient, iPlayer);
	return 1;
}

int CWObject_Game::OnRemoveClient(int _iClient, int _Reason)
{
	// TODO: Remove player somehow
	return 1;
}

int CWObject_Game::OnClientConnected(int _iClient)
{
	if (m_GameMenu != "") 
	{
		CRegistry* pReg = m_pWServer->Registry_GetClient(_iClient, WCLIENT_REG_GAMESTATE);
		if (pReg)
			pReg->SetValue("GAMEMENU", m_GameMenu);
	}

	RefreshClientGameState(_iClient);

	return 1;
}

// -------------------------------------------------------------------
//  GAMESETTINGS
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Game_Settings, CWObject, 0x0100);

void CWObject_Game_Settings::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Game_Settings_OnEvalKey, MAUTOSTRIP_VOID);

	CWObject* pGame = m_pWServer->Object_Get(m_pWServer->Selection_GetSingleTarget(WSERVER_GAMEOBJNAME));
	if (pGame)
	{
		if (_KeyHash != MHASH3('TARG','ETNA','ME'))
			pGame->OnEvalKey(_KeyHash, _pKey);
	}
	else
		ConOutL("§cf80WARNING: (CWObject_Game_Settings::OnEvalKey) No game object.");
}

void CWObject_Game_Settings::OnFinishEvalKeys()
{
	MAUTOSTRIP(CWObject_Game_Settings_OnFinishEvalKeys, MAUTOSTRIP_VOID);

	CWObject* pGame = m_pWServer->Object_Get(m_pWServer->Selection_GetSingleTarget(WSERVER_GAMEOBJNAME));
	if (pGame) 
		pGame->OnFinishEvalKeys();
	else
		ConOutL("§cf80WARNING: (CWObject_Game_Settings::OnFinishEvalKeys) No game object.");
}

