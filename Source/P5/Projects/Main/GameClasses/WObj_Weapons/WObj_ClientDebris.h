#ifndef __WOBJ_CLIENTDEBRIS_H
#define __WOBJ_CLIENTDEBRIS_H

#include "WObj_Spells.h"

#ifndef M_DISABLE_CURRENTPROJECT
// -------------------------------------------------------------------
//  CWObject_ClientDebris
// -------------------------------------------------------------------
class CWObject_ClientDebris : public CWObject_Ext_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	enum
	{
			CLIENTFLAGS_V2 = (CWO_CLIENTFLAGS_USERBASE << 7),
	};

	static void OnClientCreate(CWObject_ClientExecute* _pObj, CWorld_Client* _pWClient);
	static void OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);
	static void OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg);
	static void OnClientExecute(CWObject_ClientExecute* _pObj, CWorld_Client* _pWClient);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);

public:
	static void CreateClientDebris(CCollisionInfo *_pInfo, const CVec3Dfp32 &_Dir, CWorld_Server *_pWServer);
	static void CreateSparks(const CVec3Dfp32 &_Pos, CWorld_Server *_pWServer);
};

#endif
#endif
