/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Client data for CWObject_CharPlayer.

	Author:			Anton Ragnarsson

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_CharPlayer_ClientData

	Comments:

	History:		
		050308:		Created file
\*____________________________________________________________________________________________*/
#ifndef __WObj_CharPlayer_ClientData_h__
#define __WObj_CharPlayer_ClientData_h__

#include "../WObj_CharClientData.h"

//
// TODO: player-specific data should be moved out of the base class.
//

enum
{
	PLAYER_FLAGS_AUTOPULLUP = M_Bit(0),
};


class CWO_CharPlayer_ClientData : public CWO_Character_ClientData
{
	typedef CWO_Character_ClientData parent;
	MRTC_DECLARE;
	AUTOVAR_SETCLASS(CWO_CharPlayer_ClientData, parent);

public:
	void Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	void OnRefresh();
};



#endif // __WObj_CharPlayer_ClientData_h__
