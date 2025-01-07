/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Client data for CWObject_CharNPC.

	Author:			Anton Ragnarsson

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWO_CharNPC_ClientData implementation

	Comments:

	History:		
		050308:		Created file
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_CharNPC_ClientData.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_CharNPC_ClientData
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT(CWO_CharNPC_ClientData, CWO_CharNPC_ClientData::parent);

CWO_CharNPC_ClientData::CWO_CharNPC_ClientData()
{
};


void CWO_CharNPC_ClientData::Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	m_iGhostGlow = 0;
	parent::Clear(_pObj, _pWPhysState);
};


void CWO_CharNPC_ClientData::Copy(const CWO_Player_ClientData& _CD)
{
//	const CWO_CharNPC_ClientData& CD = *safe_cast<const CWO_CharNPC_ClientData>(&_CD);

	parent::Copy(_CD);
};
