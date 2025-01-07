// -------------------------------------------------------------------

#include "PCH.h"

#include "WRPGCore.h"
#include "WRPGChar.h"
#include "WRPGItem.h"
#include "WRPGBow.h"
#include "WRPGAmmo.h"
/*
#include "WRPGSpell.h"
#include "WRPGChar.h"
#include "../WObj_RPG.h"
#include "../WObj_Weapons/WObj_Spells.h"
#include "../WObj_Weapons/WObj_ClientDebris.h"
#include "MFloat.h"
#include "../WObj_Char.h"
*/

// -------------------------------------------------------------------
//  CRPG_Object_BurstBow
// -------------------------------------------------------------------

BOWMODE CRPG_Object_Bow::GetMode(int _iObject, CRPG_Object *_pRoot, bool _bForceUsableItem)
{
	BOWMODE LastMode = m_Mode;
	m_Mode = BOWMODE_NONE;
	
	{
		if ((_pRoot != NULL) && (_pRoot->GetType() == TYPE_CHAR))
		{
			CRPG_Object_Char *pChar = (CRPG_Object_Char *)_pRoot;
			CRPG_Object_Item *pAmmo = pChar->GetAssociatedAmmo();
			if (pAmmo != NULL)
			{
				//JK-FIX: Don't do this using dynamic_cast!
				CRPG_Object_BurstAmmo* pBurstAmmo = TDynamicCast<CRPG_Object_BurstAmmo>(pAmmo);
				CRPG_Object_SniperAmmo* pSniperAmmo = TDynamicCast<CRPG_Object_SniperAmmo>(pAmmo);

				if (pBurstAmmo != NULL)
					m_Mode = BOWMODE_BURST;
				else if (pSniperAmmo != NULL)
					m_Mode = BOWMODE_SNIPER;
			}
			else
			{
				m_Mode = BOWMODE_BURST; // We need this, as a fallback/default, for some conflict variables must be set to something valid.
			}
		}
	}

	if (m_Mode != LastMode)
	{
		switch (m_Mode)
		{
			case BOWMODE_BURST: m_BurstBow.UpdateState(this); break;
			case BOWMODE_SNIPER: m_SniperBow.UpdateState(this); break;
		}

		switch (LastMode)
		{
			case BOWMODE_BURST: m_BurstBow.Unequip(_iObject, _pRoot); break;
			case BOWMODE_SNIPER: m_SniperBow.Unequip(_iObject, _pRoot); break;
		}
	}

	return m_Mode;
}

// -------------------------------------------------------------------

void CRPG_Object_Bow::OnCreate()
{
	m_EvalKeyMode = EVALKEYMODE_BOW;
	m_Mode = BOWMODE_NONE;

	m_BurstBow.m_Base = this;
	m_SniperBow.m_Base = this;

	m_BurstBow.OnCreate();
	m_SniperBow.OnCreate();
	CRPG_Object_Summon::OnCreate();
}

// -------------------------------------------------------------------

void CRPG_Object_Bow::OnIncludeClass(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
{
	m_BurstBow.m_Base = this;
	m_SniperBow.m_Base = this;

	IncludeClassFromKey("BURST_SPAWN", _pReg, _pMapData);
	IncludeClassFromKey("SNIPER_SPAWN", _pReg, _pMapData);

	m_BurstBow.OnIncludeClass(_pReg, _pMapData, _pWServer);
	m_SniperBow.OnIncludeClass(_pReg, _pMapData, _pWServer);
	CRPG_Object_Summon::OnIncludeClass(_pReg, _pMapData, _pWServer);
}

// -------------------------------------------------------------------

bool CRPG_Object_Bow::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	switch (_KeyHash)
	{
	case MHASH3('BURS','TBOW','KEYS'): // "BURSTBOWKEYS"
		{
			m_EvalKeyMode = EVALKEYMODE_BURSTBOW;
			break;
		}
	case MHASH4('SNIP','ERBO','WKEY','S'): // "SNIPERBOWKEYS"
		{
			m_EvalKeyMode = EVALKEYMODE_SNIPERBOW;
			break;
		}
	case MHASH4('SHAR','EDBO','WKEY','S'): // "SHAREDBOWKEYS"
		{
			m_EvalKeyMode = EVALKEYMODE_BOW;
			break;
		}
	default:
		{
			bool bHandled = false;

			switch (m_EvalKeyMode)
			{
				case EVALKEYMODE_BOW: return CRPG_Object_Summon::OnEvalKey(_pKey); break;
				case EVALKEYMODE_BURSTBOW: bHandled = m_BurstBow.OnEvalKey(_pKey); break;
				case EVALKEYMODE_SNIPERBOW: bHandled = m_SniperBow.OnEvalKey(_pKey); break;
			}

			if (bHandled)
				return true;
			break;
		}
	}
	return CRPG_Object_Summon::OnEvalKey(_pKey);
}

void CRPG_Object_Bow::OnFinishEvalKeys()
{
	m_BurstBow.UpdateState(this);

	CRPG_Object_Summon::OnFinishEvalKeys();
}

// -------------------------------------------------------------------

int CRPG_Object_Bow::OnMessage(CRPG_Object* _pParent, const CWObject_Message& _Msg, int _iOwner)
{
	switch (GetMode(_iOwner, _pParent, false))
	{
		case BOWMODE_BURST: 
			return m_BurstBow.OnMessage(_pParent, _Msg, _iOwner); 
		case BOWMODE_SNIPER: 
			return m_SniperBow.OnMessage(_pParent, _Msg, _iOwner); 
		default:
			return CRPG_Object_Summon::OnMessage(_pParent, _Msg, _iOwner);
	}
}

// -------------------------------------------------------------------

bool CRPG_Object_Bow::Equip(int _iObject, CRPG_Object *_pRoot, int _iPrecedingUnequipAnim, bool _bInstant, bool _bHold)
{
	switch (GetMode(_iObject, _pRoot, true))
	{
		case BOWMODE_BURST: m_BurstBow.Equip(_iObject, _pRoot); break;
		case BOWMODE_SNIPER: m_SniperBow.Equip(_iObject, _pRoot); break;
	}
	return CRPG_Object_Summon::Equip(_iObject, _pRoot, _iPrecedingUnequipAnim, _bInstant, _bHold);
}

// -------------------------------------------------------------------

bool CRPG_Object_Bow::Unequip(int _iObject, CRPG_Object *_pRoot, int _iSucceedingEquipAnim, bool _bInstant, bool _bHold)
{
	switch (GetMode(_iObject, _pRoot, true))
	{
		case BOWMODE_BURST: m_BurstBow.Unequip(_iObject, _pRoot); break;
		case BOWMODE_SNIPER: m_SniperBow.Unequip(_iObject, _pRoot); break;
	}
	return CRPG_Object_Summon::Unequip(_iObject, _pRoot, _iSucceedingEquipAnim, _bInstant, _bHold);
}

// -------------------------------------------------------------------

bool CRPG_Object_Bow::EquipSecondary(int _iObject, CRPG_Object *_pRoot)
{
	switch (GetMode(_iObject, _pRoot, true))
	{
		case BOWMODE_BURST: m_BurstBow.EquipSecondary(_iObject, _pRoot); break;
		case BOWMODE_SNIPER: m_SniperBow.EquipSecondary(_iObject, _pRoot); break;
	}
	return true;
}

// -------------------------------------------------------------------

bool CRPG_Object_Bow::UnequipSecondary(int _iObject, CRPG_Object *_pRoot)
{
	switch (GetMode(_iObject, _pRoot, true))
	{
		case BOWMODE_BURST: m_BurstBow.UnequipSecondary(_iObject, _pRoot); break;
		case BOWMODE_SNIPER: m_SniperBow.UnequipSecondary(_iObject, _pRoot); break;
	}
	return true;
}

// -------------------------------------------------------------------

bool CRPG_Object_Bow::Activate(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	switch (GetMode(_iObject, _pRoot, true))
	{
		case BOWMODE_BURST: return m_BurstBow.Activate(_Mat, _pRoot, _iObject, _Input); break;
		case BOWMODE_SNIPER: return m_SniperBow.Activate(_Mat, _pRoot, _iObject, _Input); break;
	}
	return CRPG_Object_Summon::Activate(_Mat, _pRoot, _iObject, _Input);
}

// -------------------------------------------------------------------

bool CRPG_Object_Bow::Deactivate(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	switch (GetMode(_iObject, _pRoot, false))
	{
		case BOWMODE_BURST: return m_BurstBow.Deactivate(_Mat, _pRoot, _iObject, _Input); break;
	}
	return CRPG_Object_Summon::Deactivate(_Mat, _pRoot, _iObject, _Input);
}

// -------------------------------------------------------------------

bool CRPG_Object_Bow::ActivateSecondary(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject)
{
	switch (GetMode(_iObject, _pRoot, true))
	{
		case BOWMODE_BURST: return m_BurstBow.ActivateSecondary(_Mat, _pRoot, _iObject); break;
		case BOWMODE_SNIPER: return m_SniperBow.ActivateSecondary(_Mat, _pRoot, _iObject); break;
	}
	return true;
}

// -------------------------------------------------------------------

bool CRPG_Object_Bow::DeactivateSecondary(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject)
{
	switch (GetMode(_iObject, _pRoot, true))
	{
		case BOWMODE_BURST: return m_BurstBow.DeactivateSecondary(_Mat, _pRoot, _iObject); break;
		case BOWMODE_SNIPER: return m_SniperBow.DeactivateSecondary(_Mat, _pRoot, _iObject); break;
	}
	return true;
}

// -------------------------------------------------------------------

bool CRPG_Object_Bow::OnProcess(CRPG_Object *_pRoot, const CMat43fp32 &_Mat, int _iObject)
{
	if (!(m_Flags & RPG_ITEM_FLAGS_USEABLE))
		return false;

	switch (GetMode(_iObject, _pRoot, false))
	{
		case BOWMODE_BURST: m_BurstBow.OnProcess(_pRoot, _Mat, _iObject); break;
		case BOWMODE_SNIPER: m_SniperBow.OnProcess(_pRoot, _Mat, _iObject); break;
	}
	return CRPG_Object_Summon::OnProcess(_pRoot, _Mat, _iObject);
}

// -------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Bow, CRPG_Object_Summon);

// -------------------------------------------------------------------
