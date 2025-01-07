#include "PCH.h"

#include "WRPGInitParams.h"
#include "WRPGCore.h"
#include "WRPGItem.h"

//-------------------------------------------------------------------
//- CRPG_InitParams -------------------------------------------------
//-------------------------------------------------------------------

CRPG_InitParams::CRPG_InitParams(int _iCreator, CRPG_Object* _pRPGCreator, CRPG_Object_Item* _pRPGCreatorItem)
{
	MAUTOSTRIP(CRPG_InitParams_ctor, MAUTOSTRIP_VOID);
	Clear();
	m_iCreator = _iCreator;
	m_pRPGCreator = _pRPGCreator;
	m_pRPGCreatorItem = _pRPGCreatorItem;
}

//-------------------------------------------------------------------

void CRPG_InitParams::Clear()
{
	MAUTOSTRIP(CRPG_InitParams_Clear, MAUTOSTRIP_VOID);
	m_iCreator = 0;
	m_pRPGCreator = NULL;
	m_pRPGCreatorItem = NULL;

	m_pCInfo = NULL;

	m_Velocity = 0;
	m_VelocityType = VelocityUndefined;

	m_pDamage = NULL;
	m_pDamageEffect = NULL;
	m_DamageDeliveryFlags = 0;
	m_pShockwave = NULL;
	m_ImpactForce = 0;

	m_iTargetObj = 0;
	m_liExcludeObj[0] = 0;
	m_liExcludeObj[1] = 0;
	
/*
	m_iParam[0] = 0;
	m_iParam[1] = 0;
	m_iParam[2] = 0;
	m_iParam[3] = 0;

	m_fParam[0] = 0;
	m_fParam[1] = 0;
	m_fParam[2] = 0;
	m_fParam[3] = 0;
*/
}

//-------------------------------------------------------------------
