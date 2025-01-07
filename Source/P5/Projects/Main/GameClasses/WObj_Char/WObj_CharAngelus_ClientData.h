/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Client data for CWObject_CharAngelus.

	Author:			Patrik Willbo

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_CharAngelus_ClientData

	Comments:

	History:		
		051114:		Created file
\*____________________________________________________________________________________________*/
#ifndef __WObj_CharAngelus_ClientData_h__
#define __WObj_CharAngelus_ClientData_h__

#include "WObj_CharNPC_ClientData.h"


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_CharAngelus_ClientData
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_CharAngelus_ClientData : public CWO_CharNPC_ClientData
{
	typedef CWO_CharNPC_ClientData parent;
	MRTC_DECLARE;
	AUTOVAR_SETCLASS(CWO_CharAngelus_ClientData, parent);

public:
	// Not replicated
	CMTime	m_AnimClothTime;
	uint8	m_AnimClothParam;
	fp32		m_AnimClothBlendTime;
	
	// Replicated
	CAUTOVAR_OP(CAutoVar_int32,		m_iAuraEffect,		DIRTYMASK_1_4);
	CAUTOVAR_OP(CAutoVar_fp32,		m_AuraRampingTime,	DIRTYMASK_1_4);
	CAUTOVAR_OP(CAutoVar_fp32,		m_AuraRestingTime,	DIRTYMASK_1_4);
	CAUTOVAR_OP(CAutoVar_uint8,		m_AuraState,		DIRTYMASK_1_5);
	CAUTOVAR_OP(CAutoVar_int32,		m_AuraStateTick,	DIRTYMASK_1_5);

	AUTOVAR_PACK_BEGIN
	AUTOVAR_PACK_VAR(m_iAuraEffect)
	AUTOVAR_PACK_VAR(m_AuraRampingTime)
	AUTOVAR_PACK_VAR(m_AuraRestingTime)
	AUTOVAR_PACK_VAR(m_AuraState)
	AUTOVAR_PACK_VAR(m_AuraStateTick)
	AUTOVAR_PACK_END

public:
	CWO_CharAngelus_ClientData();
	void Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);

	virtual void Copy(const CWO_Player_ClientData& _CD);
};


#endif // __WObj_CharAngelus_ClientData_h__

