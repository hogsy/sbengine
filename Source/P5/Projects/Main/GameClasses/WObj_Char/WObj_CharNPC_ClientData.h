/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Client data for CWObject_CharNPC.

	Author:			Anton Ragnarsson

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_CharNPC_ClientData

	Comments:

	History:		
		050308:		Created file
\*____________________________________________________________________________________________*/
#ifndef __WObj_CharNPC_ClientData_h__
#define __WObj_CharNPC_ClientData_h__

#include "../WObj_CharClientData.h"

//
// TEMP!
//
// For now, use placeholder ClientData structs. These should be moved into separate files,
// and the player-specific data should be moved out of the base class.
//
class CWO_CharNPC_ClientData : public CWO_Character_ClientData
{
	typedef CWO_Character_ClientData parent;
	MRTC_DECLARE;
	AUTOVAR_SETCLASS(CWO_CharNPC_ClientData, parent);
public:
	CAUTOVAR_OP(CAutoVar_uint32,	m_iGhostGlow,	DIRTYMASK_0_0);

	AUTOVAR_PACK_BEGIN
	AUTOVAR_PACK_VAR(m_iGhostGlow)
	AUTOVAR_PACK_END

public:
	CWO_CharNPC_ClientData();
	void Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	virtual void Copy(const CWO_Player_ClientData& _CD);

	virtual CWO_CharNPC_ClientData* IsNPC() { return this; }
};


#endif // __WObj_CharNPC_ClientData_h__
