#include "PCH.h"

#include "WRPGSpell.h"
#include "WRPGChar.h"
#include "../WObj_RPG.h"
#include "../WObj_Weapons/WObj_Spells.h"
#include "../WObj_Weapons/WObj_ClientDebris.h"
#include "MFloat.h"
#include "../WObj_Char.h"
#include "WRPGAmmo.h"

// -------------------------------------------------------------------
//  CRPG_Object_DragonBreath
// -------------------------------------------------------------------

// -------------------------------------------------------------------

class CRPG_Object_DragonBreath : public CRPG_Object_Summon
{
	MRTC_DECLARE;

private:

	fp32 m_Spread;
	int m_iDuration; //Number of frames that the firebreath is used (only affects AI right now)
	int m_SpawnDelay;

public:

	void OnCreate()
	{
		MAUTOSTRIP(CRPG_Object_DragonBreath_OnCreate, MAUTOSTRIP_VOID);
		CRPG_Object_Summon::OnCreate();

		m_Spread = 0.0f;
		m_SpawnDelay = 0;
		m_iDuration = 1;
	}

	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
	{
		MAUTOSTRIP(CRPG_Object_DragonBreath_OnEvalKey, false);
		switch (_KeyHash)
		{
		case MHASH2('SPRE','AD'): // "SPREAD"
			{
				m_Spread = _pKey->GetThisValuef();
				break;
			}
		case MHASH2('DURA','TION'): // "DURATION"
			{
				m_iDuration = _pKey->GetThisValuef() * SERVER_TICKSPERSECOND;
				break;
			}
		default:
			{
				return CRPG_Object_Summon::OnEvalKey(_pKey);
				break;
			}
		}

		return true;
	}

	virtual aint OnMessage(CRPG_Object* _pParent, const CWObject_Message& _Msg, int _iOwner)
	{
		MAUTOSTRIP(CRPG_Object_DragonBreath_OnMessage, 0);
		switch (_Msg.m_Msg)
		{
		case OBJMSG_DRAGONBREATH_GETDURATION:
			{
				return m_iDuration;
			}
		default: 
			return CRPG_Object_Summon::OnMessage(_pParent, _Msg, _iOwner);
		};
	}

	void SummonWithoutDrawAmmo(const char *_pClass, fp32 _Param0, const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, CCollisionInfo *_pInfo)
	{
		MAUTOSTRIP(CRPG_Object_DragonBreath_SummonWithoutDrawAmmo, MAUTOSTRIP_VOID);
		int iObj = m_pWServer->Object_Create(_pClass, _Mat, _iObject);
	}

	void Breath(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject)
	{
		MAUTOSTRIP(CRPG_Object_DragonBreath_Breath, MAUTOSTRIP_VOID);
		const CVec3Dfp32 &Pos = CVec3Dfp32::GetMatrixRow(_Mat, 3);
		const CVec3Dfp32 &Fwd = CVec3Dfp32::GetMatrixRow(_Mat, 0);
		const CVec3Dfp32 &Right = CVec3Dfp32::GetMatrixRow(_Mat, 1);
		const CVec3Dfp32 &Up = CVec3Dfp32::GetMatrixRow(_Mat, 2);
		
		int iRand = GetGameTick(_iObject);


		CVec3Dfp32 Dir = Fwd + ((Right * (MFloat_GetRand(iRand++) - 0.5f)) + (Up * (MFloat_GetRand(iRand++) - 0.5f))) * m_Spread;
		Dir.Normalize();

		CMat43fp32 Mat;
		Dir.SetMatrixRow(Mat, 0);
		Right.SetMatrixRow(Mat, 1);
		Mat.RecreateMatrix(0, 1);
		Pos.SetMatrixRow(Mat, 3);
			
		SummonWithoutDrawAmmo(m_Spawn, 0, Mat, _pRoot, _iObject, NULL);
	}

	bool Activate(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
	{
		MAUTOSTRIP(CRPG_Object_DragonBreath_Activate, false);
		if (m_SpawnDelay > 0)
		{
			m_SpawnDelay--;
		}
		else
		{
			Breath(_Mat, _pRoot, _iObject);
			m_SpawnDelay = m_FireTimeout;
		}

		return true;
	}

};

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_DragonBreath, CRPG_Object_Summon);

// -------------------------------------------------------------------
