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
//  CRPG_Object_RaySummon
// -------------------------------------------------------------------
void CRPG_Object_RaySummon::OnCreate()
{
	MAUTOSTRIP(CRPG_Object_RaySummon_OnCreate, MAUTOSTRIP_VOID);
	CRPG_Object_Summon::OnCreate();
	
	m_Range = 512;
	m_MoveBack = 0;
	m_SpawnType = 0;
}

bool CRPG_Object_RaySummon::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CRPG_Object_RaySummon_OnEvalKey, false);
	switch (_KeyHash)
	{
	case MHASH2('RANG','E'): // "RANGE"
		{
			m_Range = _pKey->GetThisValuei();
			break;
		}
	
	case MHASH2('MOVE','BACK'): // "MOVEBACK"
		{
			m_MoveBack = _pKey->GetThisValuef();
			break;
		}
	
	case MHASH3('SPAW','NTYP','E'): // "SPAWNTYPE"
		{
			m_SpawnType = _pKey->GetThisValuei();
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

bool CRPG_Object_RaySummon::Activate(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	MAUTOSTRIP(CRPG_Object_RaySummon_Activate, false);
	if(!CheckAmmo(_pRoot))
	{
		Reload(&_Mat, _pRoot, _iObject);
		return false;
	}
	
	const CVec3Dfp32 &Pos = CVec3Dfp32::GetMatrixRow(_Mat, 3);
	const CVec3Dfp32 &Fwd = CVec3Dfp32::GetMatrixRow(_Mat, 0);
	
	CMat43fp32 Mat;
	
	CCollisionInfo Info;
	if(!TraceRay(_Mat, m_Range, &Info, 0, 0, _iObject))
	{
		Mat = _Mat;
		(Pos + Fwd * m_Range).SetMatrixRow(Mat, 3);
	}
	else
	{
		if(m_SpawnType == 0)
		{
			Mat.Unit();
			CVec3Dfp32 Res;
			Fwd.Reflect(Info.m_Plane.n, Res);
			Res.SetMatrixRow(Mat, 0);
			CVec3Dfp32(0, 0, 1).SetMatrixRow(Mat, 2);
			Mat.RecreateMatrix(0, 2);
			(Info.m_Pos - Fwd * m_MoveBack + Info.m_Plane.n * 1.0f).SetMatrixRow(Mat, 3);
		}
		else
		{
			Info.m_Plane.n.SetMatrixRow(Mat, 0);
			if(Abs(Info.m_Plane.n * CVec3Dfp32(0, 0, 1)) < 0.99f)
				CVec3Dfp32(0, 0, 1).SetMatrixRow(Mat, 2);
			else
				CVec3Dfp32(0, 1, 0).SetMatrixRow(Mat, 2);
			Mat.RecreateMatrix(0, 2);
			(Info.m_Pos - Fwd * m_MoveBack + Info.m_Plane.n * 1.0f).SetMatrixRow(Mat, 3);
			
			//	ConOut("RaySummon: " + Mat.GetString());
			//			m_pWServer->Object_Create("CoordinateSystem", Mat);
		}
	}
	
//	Summon(Mat, _pRoot, _iObject, &Info);
	return true;
}

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_RaySummon, CRPG_Object_Item);

// -------------------------------------------------------------------
//  CRPG_Object_MultiRaySummon
// -------------------------------------------------------------------
void CRPG_Object_MultiRaySummon::OnCreate()
{
	MAUTOSTRIP(CRPG_Object_MultiRaySummon_OnCreate, MAUTOSTRIP_VOID);
	CRPG_Object_Summon::OnCreate();
	
	m_Range = 512;
	m_MoveBack = 0;
	m_SpawnType = 0;
	m_nRays = 1;
	m_nChunks = 1;
	m_iChunk = 0;
	m_ChunkDuration = 0;
	m_ChunkTime = 0;
	m_ChunkOwner = 0;
	m_Scatter = 0;
	m_Damage = 0;
	m_ImpactForce = 0;
	m_EffectRate = 1.0f;
	m_iSound_Chunk = -1;
	m_iSound_LRP = -1;
	m_iSound_HitChar = -1;
	m_iSound_HitWorld = -1;
}

void CRPG_Object_MultiRaySummon::OnIncludeClass(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CRPG_Object_MultiRaySummon_OnIncludeClass, MAUTOSTRIP_VOID);
	CRPG_Object_Summon::OnIncludeClass(_pReg, _pMapData, _pWServer);
	
	IncludeSoundFromKey("SOUND_HITCHAR", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_HITWORLD", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_LRP", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_CHUNK", _pReg, _pMapData);
}

bool CRPG_Object_MultiRaySummon::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CRPG_Object_MultiRaySummon_OnEvalKey, false);
	switch (_KeyHash)
	{
	case MHASH2('RANG','E'): // "RANGE"
		{
			m_Range = _pKey->GetThisValuei();
			break;
		}
	
	case MHASH2('MOVE','BACK'): // "MOVEBACK"
		{
			m_MoveBack = _pKey->GetThisValuef();
			break;
		}
	
	case MHASH3('SPAW','NTYP','E'): // "SPAWNTYPE"
		{
			m_SpawnType = _pKey->GetThisValuei();
			break;
		}
	
	case MHASH1('RAYS'): // "RAYS"
		{
			m_nRays = _pKey->GetThisValuei();
			break;
		}
	
	case MHASH2('CHUN','KS'): // "CHUNKS"
		{
			m_nChunks = _pKey->GetThisValuei();
			break;
		}
	
	case MHASH4('CHUN','KDUR','ATIO','N'): // "CHUNKDURATION"
		{
			m_ChunkDuration = _pKey->GetThisValuei();
			break;
		}
	
	case MHASH3('SOUN','D_CH','UNK'): // "SOUND_CHUNK"
		{
			m_iSound_Chunk = m_pWServer->GetMapData()->GetResourceIndex_Sound(_pKey->GetThisValue());
			break;
		}
	
	case MHASH3('SOUN','D_LR','P'): // "SOUND_LRP"
		{
			m_iSound_LRP = m_pWServer->GetMapData()->GetResourceIndex_Sound(_pKey->GetThisValue());
			break;
		}

	case MHASH4('SOUN','D_HI','TCHA','R'): // "SOUND_HITCHAR"
		{
			m_iSound_HitChar = m_pWServer->GetMapData()->GetResourceIndex_Sound(_pKey->GetThisValue());
			break;
		}

	case MHASH4('SOUN','D_HI','TWOR','LD'): // "SOUND_HITWORLD"
		{
			m_iSound_HitWorld = m_pWServer->GetMapData()->GetResourceIndex_Sound(_pKey->GetThisValue());
			break;
		}
//		m_iSound_HitWorld = m_pWServer->GetMapData()->GetResourceIndex_Sound("stl02");

	case MHASH2('SCAT','TER'): // "SCATTER"
		{
			m_Scatter = _pKey->GetThisValuef();
			break;
		}
	
	case MHASH3('IMPA','CTFO','RCE'): // "IMPACTFORCE"
		{
			m_ImpactForce = _pKey->GetThisValuef();
			break;
		}
	
	case MHASH3('EFFE','CTRA','TE'): // "EFFECTRATE"
		{
			m_EffectRate = _pKey->GetThisValuef();
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

void CRPG_Object_MultiRaySummon::FireChunk(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject)
{
	MAUTOSTRIP(CRPG_Object_MultiRaySummon_FireChunk, MAUTOSTRIP_VOID);
	const CVec3Dfp32 &Pos = CVec3Dfp32::GetMatrixRow(_Mat, 3);
	const CVec3Dfp32 &Fwd = CVec3Dfp32::GetMatrixRow(_Mat, 0);
	const CVec3Dfp32 &Right = CVec3Dfp32::GetMatrixRow(_Mat, 1);
	const CVec3Dfp32 &Up = CVec3Dfp32::GetMatrixRow(_Mat, 2);
	
	int iRand = GetGameTick(_iObject);
	
	CMat43fp32 MatFirst;
	CCollisionInfo CInfoFirst;
	CInfoFirst.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
	

	fp32 Accuracy = 1.0f;

	// Recoil
	CWObject_Character* pObj = safe_cast<CWObject_Character>(m_pWServer->Object_Get(_iObject));
	if (pObj)
	{
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pObj);
		if (pCD)
		{
			CVec2Dfp32 Vel((Random+1.0f)*0.010f, (Random-0.5f)*0.025f);
			CVec2Dfp32 Acc = Vel;

/*			pCD->Target_AddAimAngles(Vel);
			pCD->Target_AddAimAnglesVel(Acc);*/

			Accuracy = fp32(pCD->Target_GetAccuracy()) / 255.0f;
//			Accuracy = pCD->Target_GetAccuracy();
		}
	}

//	Accuracy = 0;
	
	for(int iRay = 0; iRay < m_nRays; iRay++)
	{
		CMat43fp32 Mat;
		CVec3Dfp32 Dir(Fwd);
/*		fp32 ScatterBoost = 1.0f;
		{
			CWObject *pObj = m_pWServer->Object_Get(_iObject);
			CVec3Dfp32 Vel = pObj->GetMoveVelocity();
			fp32 Speed = Vel.Length();
			ScatterBoost *= (1.0f + Speed / 6);
		}*/
		fp32 XScatter = 4.0f*(1.0f - Accuracy) * m_Scatter*(2.0f*(MFloat_GetRand(iRand++) - 0.5f));
		fp32 YScatter = 4.0f*(1.0f - Accuracy) * m_Scatter*(2.0f*(MFloat_GetRand(iRand++) - 0.5f));
//XScatter = 0.0f;
//YScatter = 0.0f;
		Dir.Combine(Right, XScatter, Dir);
		Dir.Combine(Up, YScatter, Dir);
		Dir.Normalize();
		CVec3Dfp32 Pos2;
		Pos.Combine(Dir, m_Range, Pos2);
		
		bool bSky = false;;
		CCollisionInfo Info;
		Info.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
//		Line(Pos, Pos2);
		if(!m_pWServer->Phys_IntersectLine(Pos, Pos2, OBJECT_FLAGS_PROJECTILE, OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_CHARACTER, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &Info, _iObject) ||
//		if(!TraceRay(Pos, Pos2, Info, OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_PICKUP, _iObject) ||
			Info.m_bIsValid && Info.m_pSurface && (Info.m_pSurface->GetBaseFrame()->m_Medium.m_MediumFlags & XW_MEDIUM_SKY))
		{
			if(Info.m_bIsValid)
				bSky = true;
			
			if(m_SpawnType == 4)
			{
				Mat = _Mat;
//				if(_pChar)
//					_pChar->EyeToHands(Mat);
				CVec3Dfp32 Dir = (Pos2 - CVec3Dfp32::GetMatrixRow(Mat, 3)).Normalize();
				Dir.SetMatrixRow(Mat, 0);
				if(Abs(Dir * CVec3Dfp32(0, 0, 1)) < 0.99f)
					CVec3Dfp32(0, 0, 1).SetMatrixRow(Mat, 1);
				else
					CVec3Dfp32(0, 1, 0).SetMatrixRow(Mat, 1);
				Mat.RecreateMatrix(0, 1);
				Pos2.SetMatrixRow(Mat, 3);
			}
			else
			{
				Mat = _Mat;
				Pos2.SetMatrixRow(Mat, 3);
			}

			if (m_iSound_LRP != -1)
				m_pWServer->Sound_At(CVec3Dfp32::GetMatrixRow(_Mat, 3), m_iSound_LRP, 2, 0, 1.0f, -1, Pos2 - Pos);
		}
		else
		{
			CVec3Dfp32 HitPos = Info.m_Pos - Dir * m_MoveBack /* + Info.m_Plane.n * 1.0f*/; 
			if(m_SpawnType == 0)
			{
				// Reflect
				Mat.Unit();
				CVec3Dfp32 Res;
				Dir.Reflect(Info.m_Plane.n, Res);
				Res.SetMatrixRow(Mat, 0);
				CVec3Dfp32(0, 0, 1).SetMatrixRow(Mat, 2);
				Mat.RecreateMatrix(0, 2);
				HitPos.SetMatrixRow(Mat, 3);
			}
			else if(m_SpawnType == 2)
			{
				// Direction
				Dir.SetMatrixRow(Mat, 0);
				if(Abs(Dir * CVec3Dfp32(0, 0, 1)) < 0.99f)
					CVec3Dfp32(0, 0, 1).SetMatrixRow(Mat, 1);
				else
					CVec3Dfp32(0, 1, 0).SetMatrixRow(Mat, 1);
				Mat.RecreateMatrix(0, 1);
				HitPos.SetMatrixRow(Mat, 3);
			}
			else if(m_SpawnType == 4)
			{
				// Direction to hands
				Mat = _Mat;
//				if(_pChar)
//					_pChar->EyeToHands(Mat);
				CVec3Dfp32 Dir = (HitPos - CVec3Dfp32::GetMatrixRow(Mat, 3)).Normalize();
				Dir.SetMatrixRow(Mat, 0);
				if(Abs(Dir * CVec3Dfp32(0, 0, 1)) < 0.99f)
					CVec3Dfp32(0, 0, 1).SetMatrixRow(Mat, 1);
				else
					CVec3Dfp32(0, 1, 0).SetMatrixRow(Mat, 1);
				Mat.RecreateMatrix(0, 1);
				HitPos.SetMatrixRow(Mat, 3);
			}
			else
			{
				// Normal
				Info.m_Plane.n.SetMatrixRow(Mat, 2);
				if(Abs(Info.m_Plane.n * CVec3Dfp32(0, 0, 1)) < 0.99f)
					CVec3Dfp32(0, 0, 1).SetMatrixRow(Mat, 1);
				else
					CVec3Dfp32(0, 1, 0).SetMatrixRow(Mat, 1);
				Mat.RecreateMatrix(2, 1);
				HitPos.SetMatrixRow(Mat, 3);
			}
			
			if (!iRay)
			{
				MatFirst = Mat;
				CInfoFirst = Info;
			}
			
			CVec3Dfp32 DirTimesImpactForce = (Dir*m_ImpactForce);
			SendDamage(Info.m_iObject, CVec3Dfp32::GetMatrixRow(Mat, 3), _iObject, m_Damage.m_Damage, m_Damage.m_DamageType, Info.m_SurfaceType, &DirTimesImpactForce);
			if (m_iSound_LRP != -1)
				m_pWServer->Sound_At(CVec3Dfp32::GetMatrixRow(_Mat, 3), m_iSound_LRP, 2, 0, 1.0f, -1, HitPos - CVec3Dfp32::GetMatrixRow(_Mat, 3));

			CWObject *pHitObj = m_pWServer->Object_Get(Info.m_iObject);
			if(m_iSound_HitChar != -1 && pHitObj && pHitObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER)
				m_pWServer->Sound_At(HitPos, m_iSound_HitChar, 0);
			else if(m_iSound_HitWorld != -1)
				m_pWServer->Sound_At(HitPos, m_iSound_HitWorld, 0);
		}
/*		
		int iEffect = -1;
		if(m_Effect != "" && Random < m_EffectRate)
		{
			CMat4Dfp32 MEffect;
			if(GetCurEffectMatrix(MEffect))
			{
				CMat4Dfp32 Mat;
				Mat.UnitNot3x3();
				CVec3Dfp32 Ray = CVec3Dfp32::GetMatrixRow(MatFirst, 3) - CVec3Dfp32::GetMatrixRow(MEffect, 3);
				
				Ray.SetMatrixRow(Mat, 0);
				CVec3Dfp32(0, 0, 1).SetMatrixRow(Mat, 1);
				CVec3Dfp32::GetMatrixRow(MEffect, 3).SetMatrixRow(Mat, 3);
				Mat.RecreateMatrix(0, 1);
				
				iEffect = m_pWServer->Object_Create(m_Effect, Mat, _iObject);
				if(iEffect)
				{
					CWObject *pObj = m_pWServer->Object_Get(iEffect);
					if(pObj)
						pObj->m_iAnim0 = Ray.Length();
				}
			}
		}
*/		
		if(m_Spawn != "")
		{
			int iObj = m_pWServer->Object_Create(m_Spawn, Mat, _iObject);
			if(bSky)
			{
				CWObject *pObj = m_pWServer->Object_Get(iObj);
				if(pObj)
				{
					pObj->ClientFlags() |= CWO_CLIENTFLAGS_LINKINFINITE;
					m_pWServer->Object_SetPosition(iObj, pObj->GetPosition());
				}
			}
		}
	}
		
	if (m_iSound_Chunk != -1)
		m_pWServer->Sound_At(CVec3Dfp32::GetMatrixRow(_Mat, 3), m_iSound_Chunk, 0);
}

bool CRPG_Object_MultiRaySummon::Activate(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	MAUTOSTRIP(CRPG_Object_MultiRaySummon_Activate, false);
	if(!CheckAmmo(_pRoot))
	{
		Reload(&_Mat, _pRoot, _iObject);
		return false;
	}
	
	FireChunk(_Mat, _pRoot, _iObject);
	m_iChunk = 1;
	m_ChunkTime = 0;
	m_ChunkOwner = _iObject;

	if(m_iSound_Cast != -1)
		m_pWServer->Sound_At(CVec3Dfp32::GetMatrixRow(_Mat, 3), m_iSound_Cast, 0);
	
	DrawAmmo(1, _Mat, _pRoot, _iObject);
	
	return true;
}

bool CRPG_Object_MultiRaySummon::OnProcess(CRPG_Object *_pRoot, const CMat43fp32 &_Mat, int _iObject)
{
	MAUTOSTRIP(CRPG_Object_MultiRaySummon_OnProcess, false);
	if (CRPG_Object_Summon::OnProcess(_pRoot, _Mat, _iObject)) return true;

	//	ConOut("OnProcess " + GetDesc(0) + CStrF(", Chunk %d/%d, Time %d/%d", m_iChunk, m_nChunks, m_ChunkTime, m_ChunkDuration));
	if (m_iChunk < m_nChunks)
	{
		if (m_ChunkTime >= m_ChunkDuration)
		{
			CWObject* pObj = m_pWServer->Object_Get(m_ChunkOwner);
			if (!pObj) return false;
			
			CMat43fp32 Mat;
			CWObject_Message Msg(OBJMSG_SPELLS_GETACTIVATEPOSITION);
			Msg.m_pData = &Mat;
			Msg.m_DataSize = sizeof(Mat);
			if (m_pWServer->Message_SendToObject(Msg, m_ChunkOwner))
			{
				FireChunk(Mat, NULL, m_ChunkOwner);
			}
			m_iChunk++;
			m_ChunkTime = 0;
		}
		m_ChunkTime++;
	}
	return false;
}

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_MultiRaySummon, CRPG_Object_Item);

// -------------------------------------------------------------------
//  CRPG_Object_SphereDamage
// -------------------------------------------------------------------
class CRPG_Object_SphereDamage : public CRPG_Object_Weapon
{
	MRTC_DECLARE;
		
	fp32 m_HitRadius;
	fp32 m_Force;
	
public:
	CRPG_Object_SphereDamage()
	{
		MAUTOSTRIP(CRPG_Object_SphereDamage_ctor, MAUTOSTRIP_VOID);
		m_HitRadius = 8;
		m_Damage = 10;
		m_Force = 5;
	}
	
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
	{
		MAUTOSTRIP(CRPG_Object_SphereDamage_OnEvalKey, false);

		switch (_KeyHash)
		{
		case MHASH3('HITR','ADUI','S'): // "HITRADUIS"
			{
				m_HitRadius = _pKey->GetThisValuef();
				break;
			}
		default:
			{
				break;
			}
		}
		
		switch (_KeyHash)
		{
		case MHASH2('FORC','E'): // "FORCE"
			{
				m_Force = _pKey->GetThisValuef();
				break;
			}

		default:
			{
				return CRPG_Object_Item::OnEvalKey(_pKey);
				break;
			}
		}
		
		return true;
	}

	bool OnProcess(CRPG_Object *_pRoot, const CMat43fp32 &_Mat, int _iObject)
	{
		MAUTOSTRIP(CRPG_Object_SphereDamage_OnProcess, false);

		if(_pRoot->GetType() == CRPG_Object::TYPE_CHAR && ((CRPG_Object_Char *)_pRoot)->Wait() == 0)
		{
			CCollisionInfo Info;
			Info.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
			
			CWO_PhysicsState PhysState;
			PhysState.AddPhysicsPrim(0, CWO_PhysicsPrim(OBJECT_PRIMTYPE_BOX, 0, CVec3Dfp32(m_HitRadius, m_HitRadius, m_HitRadius), 0, 1.0f));
			PhysState.m_ObjectFlags = OBJECT_FLAGS_PROJECTILE;
			PhysState.m_ObjectIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
//			m_pWServer->Debug_RenderOBB(_Mat, CVec3Dfp32(m_HitRadius, m_HitRadius, m_HitRadius), 0xffffffff, 0.05f, false);

			if(m_pWServer->Phys_IntersectWorld(-1, PhysState, _Mat, _Mat, _iObject, &Info) && Info.m_bIsValid)
			{
				const CVec3Dfp32 &Dir = CVec3Dfp32::GetRow(_Mat, 0);
				const CVec3Dfp32 DirTimesForce = (Dir * m_Force);
				
				if (SendDamage(Info.m_iObject, CVec3Dfp32::GetRow(_Mat, 3), _iObject, m_Damage.m_Damage, m_Damage.m_DamageType, Info.m_SurfaceType, &DirTimesForce, &Dir) > 0)
				{
					ApplyWait(_iObject, m_FireTimeout, m_NoiseLevel, m_Visibility);
				}
			}
		}
		return true;
	}	
};

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_SphereDamage, CRPG_Object_Item);

// -------------------------------------------------------------------
