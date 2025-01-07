/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Client data for CWObject_CharAngelus.

	Author:			Patrik Willbo

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_CharAngelus_ClientData implementation

	Comments:

	History:		
		051114:		Created file
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_CharAngelus_ClientData.h"
#include "../../GameWorld/WClientMod_Defines.h"


MRTC_IMPLEMENT(CWO_CharAngelus_ClientData, CWO_CharAngelus_ClientData::parent);


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_CharAngelus_ClientData
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWO_CharAngelus_ClientData::CWO_CharAngelus_ClientData()
{
}


void CWO_CharAngelus_ClientData::Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	parent::Clear(_pObj, _pWPhysState);

	m_AuraState = ANGELUS_AURASTATE_IDLE;

	m_iAuraEffect = 0;
	m_AuraRampingTime = 4.0f;
	m_AuraRestingTime = 4.0f;
	m_AuraStateTick = _pWPhysState->GetGameTick();

	m_AnimClothParam = 2;
	m_AnimClothBlendTime = 0.0f;
}


void CWO_CharAngelus_ClientData::Copy(const CWO_Player_ClientData& _CD)
{
	parent::Copy(_CD);
}

