#include "PCH.h"

#include "WRPGInitParams.h"
#include "WRPGChar.h"
#include "WRPGWeapon.h"
#include "WRPGAmmo.h"
#include "../WObj_Char.h"
#include "../WObj_Game/WObj_GameMod.h"
#include "../WObj_CharMsg.h"

void RenderSphere(CWorld_PhysState* _pWPhys, const CVec3Dfp32& _Pos, fp32 _Size, fp32 _Duration = 1.0f, int32 _Color = 0xffffffff);

// This is supposed to work the same as a eve smartbomb (kinda), shockwave going out from the center
// affecting objects (incl players etc)
class CRPG_Object_Flashbang : public CRPG_Object_Weapon
{
	MRTC_DECLARE;

protected:
	// All objects within maxdistance will receive some force (depending on falloff)
	// (0 == no falloff, equal force up to maxdistance, 1 == square falloff up to maxdistance
	fp32 m_FallOff;
	fp32 m_DamageFalloff;
	fp32 m_MaxDistance;
	fp32 m_Force;
	int16 m_MinDamage;

public:
	virtual void OnCreate()
	{
		CRPG_Object_Weapon::OnCreate();
		m_FallOff = 1.0f;
		m_DamageFalloff = 0.5f;
		m_MaxDistance = 200.0f;
		m_Force = 10.0f;
		m_MinDamage = 0;
	}

	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
	{
		CStr KeyName = _pKey->GetThisName();
		CStr KeyValue = _pKey->GetThisValue();

		switch (_KeyHash)
		{
		case MHASH2('FALL','OFF'): // "FALLOFF"
			{
				m_FallOff = KeyValue.Val_fp64();
				break;
			}
		case MHASH3('MAXD','ISTA','NCE'): // "MAXDISTANCE"
			{
				m_MaxDistance = KeyValue.Val_fp64();
				break;
			}
		case MHASH2('FORC','E'): // "FORCE"
			{
				m_Force = KeyValue.Val_fp64();
				break;
			}
		case MHASH4('DAMA','GEFA','LLOF','F'): // "DAMAGEFALLOFF"
			{
				m_DamageFalloff = KeyValue.Val_fp64();
				break;
			}
		case MHASH3('MIND','AMAG','E'): // "MINDAMAGE"
			{
				m_MinDamage = KeyValue.Val_int();
				break;
			}
		default:
			{
				return CRPG_Object_Weapon::OnEvalKey(_KeyHash, _pKey);
				break;
			}
		}

		return true;
	}

	bool GetForce(const CVec3Dfp32& _CurrentPos, const CVec3Dfp32& _TargetPos, CVec3Dfp32& _Force, int32& _Damage)
	{
		if (m_Force <= 0.0f)
			return false;

		CVec3Dfp32 Dir = _TargetPos - _CurrentPos;
		//m_pWServer->Debug_RenderWire(_CurrentPos,_TargetPos,0xff00ffff,10.0f,false);
		fp32 Distance = Dir.Length();
		Dir = Dir / Distance;
		fp32 Factor = (1.0f - Sqr(Distance / m_MaxDistance));
		if (Factor <= 0.0f)
			return false;

		_Damage = Max((int32)m_MinDamage,(int32)(_Damage * ((1.0f - m_DamageFalloff) + Factor * m_DamageFalloff)));

		fp32 Force = m_Force * ((1.0f - m_FallOff) + Factor * m_FallOff);
		_Force = Dir * Force;
		return true;
	}

	void SendForce(int16 _iActivator)
	{
		// Find targets within the maxdistance area
		CVec3Dfp32 Position;
		CWObject_CoreData* pObjActivator = m_pWServer->Object_GetCD(_iActivator);
		if (!pObjActivator)
			return;
		// Activate from center of character?
		pObjActivator->GetAbsBoundBox()->GetCenter(Position);

//		int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
		int32 ObjectFlags = (OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER);

		// PLAYER_PHYSFLAGS_NOPROJECTILECOLL fix: Replace with this after Riddick GM
		//int32 ObjectFlags = (_CollisionObjects != 0) ? _CollisionObjects: (OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT);
//		int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
		TSelection<CSelection::LARGE_BUFFER> Selection;
		m_pWServer->Selection_AddBoundSphere(Selection,ObjectFlags,Position,m_MaxDistance);
		//RenderSphere(m_pWServer,Position,m_MaxDistance,10.0f);
		const int16* pSel;
		int32 NumSel = m_pWServer->Selection_Get(Selection,&pSel);
		int32 OrgDamage = m_Damage.m_Damage;
		uint32 OrgDamageType = m_Damage.m_DamageType;
		m_Damage.m_DamageType &= ~DAMAGETYPE_DISORIENT_MASK;
		for (int32 i = 0; i < NumSel; i++)
		{
			// If we find the one who activated it, don't care
			if (pSel[i] == _iActivator)
				continue;

			CWObject_CoreData* pObj = m_pWServer->Object_GetCD(pSel[i]);
			if (pObj)
			{
				CVec3Dfp32 Center,Force;
				pObj->GetAbsBoundBox()->GetCenter(Center);
				CCollisionInfo Info;
				Info.m_Pos = Center;
				int32 Damage = OrgDamage;
				if (GetForce(Position,Center,Force,Damage))
				{
					m_Damage.m_Damage = Damage;
					m_Damage.SendExt(pSel[i],_iActivator,m_pWServer,&Info,&Force);
				}
			}
		}
		m_Damage.m_Damage = OrgDamage;
		m_Damage.m_DamageType = OrgDamageType;

	}

	void SendDisorientation(int16 _iActivator)
	{
		CVec3Dfp32 Position;
		CWObject_CoreData* pObjActivator = m_pWServer->Object_GetCD(_iActivator);
		if (!pObjActivator)
			return;

		// Activate from center of character,
		pObjActivator->GetAbsBoundBox()->GetCenter(Position);

		// Setup selection
		const int16* pSel;
		int32 ObjectFlags = OBJECT_FLAGS_CHARACTER;
		TSelection<CSelection::LARGE_BUFFER> Selection;
		m_pWServer->Selection_AddBoundSphere(Selection, ObjectFlags, Position, m_MaxDistance * 100.0f);
		int32 NumSel = m_pWServer->Selection_Get(Selection, &pSel);

		// Store and alter damage message
		int32 OrgDamage = m_Damage.m_Damage;
		uint32 OrgDamageType = m_Damage.m_DamageType;
		m_Damage.m_Damage = (int)MinMT((uint16)m_MaxDistance * 100, 0xffff);
		m_Damage.m_DamageType &= DAMAGETYPE_DISORIENT_MASK;

		// Send disorientation damage to all characters
		for (int32 i = 0; i < NumSel; i++)
		{
			if (pSel[i] == _iActivator)
				continue;

			CWObject_Character* pObj = CWObject_Character::IsCharacter(m_pWServer->Object_Get(pSel[i]));
			if(pObj)
				m_Damage.Send(pSel[i], _iActivator, m_pWServer, Position);
		}

		// Restore damage message and pop selection
		m_Damage.m_Damage = OrgDamage;
		m_Damage.m_DamageType = OrgDamageType;
	}

	virtual bool Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
	{
		// Send force and possible disorientation to nearby objects...
		SendForce(_iObject);
		
		if(m_Damage.m_DamageType & DAMAGETYPE_DISORIENT_MASK)
			SendDisorientation(_iObject);

		ApplyWait(_iObject,-m_FireTimeout,0.0f,0.0f);
		return CRPG_Object_Weapon::Activate(_Mat,_pRoot,_iObject,_Input);
	}

};

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Flashbang, CRPG_Object_Weapon);
