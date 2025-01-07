/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Character class for actual player characters.

	Author:			Anton Ragnarsson

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_CharPlayer

	Comments:

	History:		
		050308:		Created file
\*____________________________________________________________________________________________*/
#ifndef __WObj_CharPlayer_h__
#define __WObj_CharPlayer_h__

#include "../WObj_Char.h"
#include "WObj_CharPlayer_ClientData.h"


class CWObject_CharPlayer : public CWObject_Character
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	typedef CWObject_Character parent;

public:
	// Client data - server only interface
	const CWO_CharPlayer_ClientData& GetClientData() const { return GetClientData(this); }
	      CWO_CharPlayer_ClientData& GetClientData()       { return GetClientData(this); }
	// Client data - client/server interface
	static const CWO_CharPlayer_ClientData& GetClientData(const CWObject_CoreData* _pObj);
	static       CWO_CharPlayer_ClientData& GetClientData(CWObject_CoreData* _pObj);

	// CWObject overrides
	virtual void OnInitInstance(const aint* _pParam, int _nParam);
	virtual void OnCreate();
	virtual void OnRefresh();

	static void OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
};



#endif // __WObj_CharPlayer_h__
