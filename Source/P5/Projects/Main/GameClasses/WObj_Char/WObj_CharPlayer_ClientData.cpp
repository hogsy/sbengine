/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Client data for CWObject_CharPlayer.

	Author:			Anton Ragnarsson

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWO_CharPlayer_ClientData implementation

	Comments:

	History:		
		050308:		Created file
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_CharPlayer_ClientData.h"


MRTC_IMPLEMENT(CWO_CharPlayer_ClientData, CWO_CharPlayer_ClientData::parent);


void CWO_CharPlayer_ClientData::Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	parent::Clear(_pObj, _pWPhysState);
}


void CWO_CharPlayer_ClientData::OnRefresh()
{
}

