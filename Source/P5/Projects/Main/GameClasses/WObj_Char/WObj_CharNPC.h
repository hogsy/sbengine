/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Character class for Non-Player Characters.

	Author:			Anton Ragnarsson

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_CharNPC

	Comments:

	History:		
		050308:		Created file
\*____________________________________________________________________________________________*/
#ifndef __WObj_CharNPC_h__
#define __WObj_CharNPC_h__

#include "../WObj_Char.h"
#include "WObj_CharNPC_ClientData.h"


class CWObject_CharNPC : public CWObject_Character
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	// Client data - server only interface
	const CWO_CharNPC_ClientData& GetClientData() const { return GetClientData(this); }
	      CWO_CharNPC_ClientData& GetClientData()       { return GetClientData(this); }
	// Client data - client/server interface
	static const CWO_CharNPC_ClientData& GetClientData(const CWObject_CoreData* _pObj);
	static       CWO_CharNPC_ClientData& GetClientData(CWObject_CoreData* _pObj);

	virtual void OnDestroy();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	static void OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer);
	virtual void AI_SetEyeLookDir(const CVec3Dfp32& _EyeLook = CVec3Dfp32(_FP32_MAX,0,0), const int16 _iObj = 0);
};



#endif // __WObj_CharNPC_h__
