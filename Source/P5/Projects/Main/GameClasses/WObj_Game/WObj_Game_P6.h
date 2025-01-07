#ifndef __WOBJ_GAMEP6_H
#define __WOBJ_GAMEP6_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			WObj_Game_P6

Author:			Roger Mattsson

Copyright:		

Contents:		

History:		
0600413:		Created File
\*____________________________________________________________________________________________*/

#include "WObj_GameMod.h"

enum
{
	NETMSG_GAME_INV_ADD = NETMSG_GAME_JOIN_TEAM + 1,
	NETMSG_GAME_INV_MOD,
	NETMSG_GAME_INV_WEAPON_MOD,
};

class CWObject_GameP6 : public CWObject_GameCampaign
{
	MRTC_DECLARE_SERIAL_WOBJECT;
public:
	CWObject_GameP6();
	int OnCharacterKilled(int _iObject, int _iSender);
	virtual void OnRefresh();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual int	OnClientCommand(int _iPlayer, const CCmd* _pCmd);
	static void OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);
	static void OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg);
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	static void OnClientRenderStatusBar(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D, CVec2Dfp32 *_rect = NULL);
	static void DrawInfoRect(CRC_Util2D *_pUtil2D, CWorld_Client *_pWClient, const CRct &_Rct, const wchar *_pTitle = NULL, CRC_Font *_pTitleFont = NULL, const wchar *_pText = NULL, CRC_Font *_pFont = NULL, int _iSurfRes = 0, int _SurfaceWidth = 0, int _SurfaceHeight = 0, int _TitleExtraHeight = 0, int _TextCol = 0, int _Color1 = 0, int _Color2 = 0);

	bool m_bDoOnce;
};

#endif

