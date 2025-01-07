/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Chain weapon for the Angelus character

	Author:			Olle Rosenquist

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CRPG_Object_AngelusChain

	Comments:

	History:		
		050623		Created file
		051118		Cleaned up some old mess
\*____________________________________________________________________________________________*/

#include "PCH.h"
#include "WRPGWeapon.h"
#include "../WObj_Misc/WObj_ChainEffect.h"


// Defines
#define CRPG_Object_AngelusChain_Parent CRPG_Object_Weapon


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CRPG_Object_AngelusChain
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CRPG_Object_AngelusChain : public CRPG_Object_AngelusChain_Parent
{
protected:
	MRTC_DECLARE;

	CMTime	m_ActivateTime;
	CMTime	m_DeactivateTime;
	fp32		m_ActiveTime;
	fp32		m_ChainRange;
	int32	m_AttackDamage;
public:

	virtual void OnIncludeClass(const CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
	{
		CRPG_Object_AngelusChain_Parent::OnIncludeClass(_pReg,_pMapData,_pWServer);
		IncludeClassFromKey("CHAINSPAWNTYPE", _pReg, _pMapData);
	}

	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
	{
		CStr KeyName = _pKey->GetThisName();
		CStr KeyValue = _pKey->GetThisValue();

		switch (_KeyHash)
		{
		case MHASH4('CHAI','NACT','IVET','IME'): // "CHAINACTIVETIME"
			{
				m_ActiveTime = KeyValue.Val_fp64();
				return true;
			}
		case MHASH3('ATTA','CKDA','MAGE'): // "ATTACKDAMAGE"
			{
				m_AttackDamage = KeyValue.Val_int();
				return true;
			}
		case MHASH3('CHAI','NRAN','GE'): // "CHAINRANGE"
			{
				m_ChainRange = KeyValue.Val_fp64();
				return true;
			}
		default:
			{
				return CRPG_Object_AngelusChain_Parent::OnEvalKey(_KeyHash, _pKey);
			}
		}
	}

	virtual void OnCreate()
	{
		CRPG_Object_AngelusChain_Parent::OnCreate();
		m_ActivateTime.Reset();
		m_DeactivateTime.Reset();
		m_ChainRange = 100.0f;
		m_AttackDamage = 0;
	}

	virtual bool OnProcess(CRPG_Object *_pRoot, int _iObject)
	{
		// Deactivate the chain (check if active and how long it's been active)
		DeactivateChain();
		return CRPG_Object_AngelusChain_Parent::OnProcess(_pRoot, _iObject);
	}

	virtual bool IsActivatable(CRPG_Object* _pRoot,int _iObject,int _Input = 1)
	{
		if (!m_ActivateTime.IsReset() || ((m_pWServer->GetGameTime() - m_DeactivateTime).GetTime() < 0.0f))
			return false;

		return CRPG_Object_AngelusChain_Parent::IsActivatable(_pRoot, _iObject, _Input);
	}

	virtual bool Equip(int _iObject, CRPG_Object *_pRoot, bool _bInstant = false, bool _bHold = false)
	{
		// Create chain object attach it to character (hand bone..?)
		CWObject_CoreData* pChar = m_pWServer->Object_GetCD(_iObject);

		// Set some parameters and stuff...
		m_ActivateTime.Reset();

		return CRPG_Object_AngelusChain_Parent::Equip(_iObject,_pRoot,_bInstant,_bHold);
	}

	virtual bool Unequip(int _iObject, CRPG_Object *_pRoot, bool _bInstant = false, bool _bHold = false)
	{
		// Delete chain object
		DeactivateChain(true);
		return CRPG_Object_AngelusChain_Parent::Unequip(_iObject,_pRoot,_bInstant,_bHold);
	}

	void ActivateChain(const CMat4Dfp32& _Mat, int _iChar)
	{
		m_DeactivateTime.Reset();
		m_ActivateTime = m_pWServer->GetGameTime();
	}

	void DeactivateChain(bool _bForce = false)
	{
		// Shrink chain again
		if (!m_ActivateTime.IsReset() && (_bForce || (m_pWServer->GetGameTime() - m_ActivateTime).GetTime() >= m_ActiveTime))
		{
			m_DeactivateTime = m_pWServer->GetGameTime();// + CMTime::CreateFromSeconds(m_ShrinkTime);
			m_ActivateTime.Reset();
		}
	}

	virtual bool OnActivate(const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, int _Input)
	{
		ActivateChain(_Mat, _iObject);
		return CRPG_Object_AngelusChain_Parent::OnActivate(_Mat, _pRoot, _iObject, _Input);
	}

	virtual void OnDeltaLoad(CCFile* _pFile, CRPG_Object* _pCharacter)
	{
		m_ActivateTime.Read(_pFile);
		m_DeactivateTime.Read(_pFile);
		return CRPG_Object_AngelusChain_Parent::OnDeltaLoad(_pFile,_pCharacter);
	}

	virtual void OnDeltaSave(CCFile* _pFile, CRPG_Object* _pCharacter)
	{
		// Save properties that may have changed during run
		m_ActivateTime.Write(_pFile);
		m_DeactivateTime.Write(_pFile);
		return CRPG_Object_AngelusChain_Parent::OnDeltaSave(_pFile,_pCharacter);
	}
};


MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_AngelusChain, CRPG_Object_AngelusChain_Parent);

