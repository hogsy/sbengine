#ifndef _INC_WOBJ_CHARSPECTATOR
#define _INC_WOBJ_CHARSPECTATOR
//#ifndef M_RTM
#include "WObj_Char/WObj_CharPlayer.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Spectator
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObject_Spectator : public CWObject_CharPlayer
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	virtual void OnFinishEvalKeys();
	virtual void OnRefresh();
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	static void OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_SpectatorIntermission
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObject_SpectatorIntermission : public CWObject_Spectator
{
	MRTC_DECLARE_SERIAL_WOBJECT;
public:
	virtual void OnFinishEvalKeys();
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
};

//#endif

#endif // _INC_WOBJ_CHARSPECTATOR
