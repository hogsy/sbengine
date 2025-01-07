#ifndef WRPGInitParams_h
#define WRPGInitParams_h

#include "MRTC.h"

//-------------------------------------------------------------------
//- CRPG_InitParams -------------------------------------------------
//-------------------------------------------------------------------

class CRPG_InitParams
{

public:

	enum VelocityType { VelocityUndefined, VelocityAbsolute, VelocityFraction };

	int m_iCreator;
	class CRPG_Object* m_pRPGCreator;
	class CRPG_Object_Item* m_pRPGCreatorItem;

	class CCollisionInfo* m_pCInfo;

	fp32 m_Velocity;
	int m_VelocityType;

	class CWO_Shockwave* m_pShockwave;			// Sent when created
	class CWO_Damage* m_pDamage;				// Sent on impact or while attached.
	uint32 m_DamageDeliveryFlags;					// DAMAGEFLAG_NONCRIT etc
	const char* m_pDamageEffect;
	fp32 m_ImpactForce;							// Send on impact

	int m_iTargetObj;
	int16 m_liExcludeObj[2];

public:

	CRPG_InitParams(int _iCreator, class CRPG_Object* _pRPGCreator, class CRPG_Object_Item* _pRPGCreatorItem);
	CRPG_InitParams() { Clear(); }
	void Clear();

};

//-------------------------------------------------------------------

#endif /* WRPGInitParams_h */
