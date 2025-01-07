/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			CRPG_Object_Fist implementation
					
	Author:			Olle Rosenquist
					
	Copyright:		Copyright O3 Games AB 2002
					
	Contents:		
					
	Comments:		IT'S NOT FINISHED YET!!!
					
	History:		
		020729:		Olle Rosenquist, Created File
\*____________________________________________________________________________________________*/

#include "PCH.h"

#include "WRPGFist.h"
#include "WRPGChar.h"
#include "../WObj_Char.h"
#include "../WObj_AI/AICore.h"
#include "../WObj_Game/WObj_GameMessages.h"
#include "../WObj_Weapons/WObj_ClientDebris.h"
#include "../../../../Shared/MOS/Classes/GameContext/WGameContext.h"
#include "../WObj_Misc/WObj_ActionCutscene.h"
#include "../WObj_Misc/WObj_Ledge.h"

/*#define RPGFIST_PREDICTIONCOMPENSATIONTICKS 3

CRPG_Object_Fist::CRPG_Object_Fist()
{
	return;
}

void CRPG_Object_Fist::OnCreate()
{
	CRPG_Object_Fist_Parent::OnCreate();

	m_LastActivatedTick = 0;
	m_LastProcessedTick = 0;
	m_Input = 0;
	m_HitRadius = 1.0f;
	m_PunchTick = -1;
	m_PunchMatrix.Unit();
	m_iPickupBody = -1;
	m_HitObjectDelayCountdown = 0;
	m_HitCharacterDelayCountdown = 0;
	m_HitWorldDelayCountdown = 0;
	m_ActionDelay = 0;
	m_iSound_HitHand = -1;
	m_iSound_HitKnee = -1;
	m_iSound_HitKick = -1;
	m_iSound_HitSurf = -1;
	m_bShouldBeDead = false;
	m_bIsActivatable = true;
}

void CRPG_Object_Fist::OnIncludeClass(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
{
	CRPG_Object_Fist_Parent::OnIncludeClass(_pReg, _pMapData, _pWServer);

	IncludeSoundFromKey("SOUND_HITHAND", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_HITKNEE", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_HITKICK", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_HITSURF", _pReg, _pMapData);
	
	IncludeModelFromKey("DAMAGE_EFFECT", _pReg, _pMapData);
}

bool CRPG_Object_Fist::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	if(KeyName.Find("HITRADIUS") != -1)
	{
		m_HitRadius = KeyValue.Val_fp64();
	}
	else if(KeyName.Find("SOUND_HITHAND") != -1)
	{
		m_iSound_HitHand = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
	}
	else if(KeyName.Find("SOUND_HITKNEE") != -1)
	{
		m_iSound_HitKnee = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
	}
	else if(KeyName.Find("SOUND_HITKICK") != -1)
	{
		m_iSound_HitKick = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
	}
	else if(KeyName.Find("SOUND_HITSURF") != -1)
	{
		m_iSound_HitSurf = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
	}
	else if(KeyName.Find("DAMAGE_EFFECT") != -1)
	{
		m_DamageEffect = KeyValue;
	}
	else
		return CRPG_Object_Fist_Parent::OnEvalKey(_pKey);

	return true;
}

void CRPG_Object_Fist::OnFinishEvalKeys()
{
	CRPG_Object_Fist_Parent::OnFinishEvalKeys();
}

bool CRPG_Object_Fist::Equip(int _iObject, CRPG_Object *_pRoot, int _iPrecedingUnequipAnim, 
							 bool _bInstant, bool _bHold)
{
	return CRPG_Object_Fist_Parent::Equip(_iObject, _pRoot, _iPrecedingUnequipAnim, _bInstant, 
		_bHold);
}

bool CRPG_Object_Fist::Unequip(int _iObject, CRPG_Object *_pRoot, int _iSucceedingEquipAnim, 
							   bool _bInstant, bool _bHold)
{
//	ConOut("Unequipping Fist");

	return CRPG_Object_Fist_Parent::Unequip(_iObject, _pRoot, _iSucceedingEquipAnim, _bInstant,
		_bHold);
}

bool CRPG_Object_Fist::OnActivate(const CMat4Dfp32 &, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	CWObject* pObj = m_pWServer->Object_Get(_iObject);
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pObj);

	return false;
}

bool CRPG_Object_Fist::ActivateFist(int _iObject, int _iOpponent)
{
	UpdateInput(CONTROLBITS_PRIMARY | CONTROLBITS_SECONDARY);

	return true;
}

bool CRPG_Object_Fist::ActivateFistSneak(int _iObject)
{
	// Function RESET

	return true;
}

bool CRPG_Object_Fist::OnProcess(CRPG_Object *_pRoot, const CMat4Dfp32 &_Mat, int _iObject)
{
	fp32 ExtraPolate = SERVER_TIMEPERFRAME * 2;
	fp32 ImpactForce = 10.0f;
	bool bComboSpawn = false;

	if(m_HitCharacterDelayCountdown > 0)
		m_HitCharacterDelayCountdown--;
	if(m_HitWorldDelayCountdown > 0)
		m_HitWorldDelayCountdown--;
	if(m_HitObjectDelayCountdown > 0)
		m_HitObjectDelayCountdown--;

	CWObject* pObj = m_pWServer->Object_Get(_iObject);
	if (!pObj)
		return false;
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pObj);
	if (!pCD)
		return false;

	// If there's a knucklebuster present, select that instead
	CRPG_Object_KnuckleDuster* pKnuckle = (CRPG_Object_KnuckleDuster*)m_pWServer->Message_SendToObject(
		CWObject_Message(OBJMSG_CHAR_GETITEM, CRPG_Object_KnuckleDuster_ItemType, 0, _iObject), _iObject);

	if (pKnuckle)
	{
		m_bIsActivatable = false;
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SELECTITEMBYTYPE, CRPG_Object_KnuckleDuster_ItemType, 0, 
		_iObject), _iObject);
	}

//	pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_THIRDPERSON;

	// Update "movedir" input, filter out "push" buttons
	//m_Input &= ~(m_Input & ~(pCD->m_FightLookDir | GetInputState(_iObject)));

	int GameTick = GetGameTick(_iObject);

	if (m_ActionDelay > 0)
		m_ActionDelay--;

	if (m_LastProcessedTick < GameTick)
		m_LastProcessedTick = GameTick;

	// Ugly hack for when other character is not in fight mode
	// Check for if the other character is not in fight mode and has moved outside
	// fighting radius....

	return CRPG_Object_Fist_Parent::OnProcess(_pRoot, _Mat, _iObject);
}

void CRPG_Object_Fist::DoCollisionTest(CWObject *_pObj, CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkelChar, const CVec3Dfp32 &_Force, bool _bSpawnSpecial)
{
	CMat4Dfp32 Mat;
	Mat.CreateFrom(_pSkelInstance->GetBoneTransform(22));
	
	CVec3Dfp32 v(0);
	const CXR_SkeletonAttachPoint* pHand = _pSkelChar->GetAttachPoint(6);
	if (pHand) v = pHand->m_LocalPos;
	v *= Mat;
	v.SetMatrixRow(Mat, 3);
	
	//m_pWServer->Debug_RenderWire(v,v + CVec3Dfp32::GetRow(Mat,0)*30, 0xffffffff, 10.0f);

	//ConOut(v.GetString());
	int iTick = GetGameTick(_pObj->m_iObject);
	if(iTick == m_PunchTick + 1)
	{
		CCollisionInfo Info;
		Info.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
		
		CWO_PhysicsState PhysState;
		PhysState.AddPhysicsPrim(0, CWO_PhysicsPrim(OBJECT_PRIMTYPE_BOX, 0, CVec3Dfp32(m_HitRadius, m_HitRadius, m_HitRadius), 0, 1.0f));
		PhysState.m_ObjectFlags = OBJECT_FLAGS_PROJECTILE;
		PhysState.m_ObjectIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
		
		if(m_pWServer->Phys_IntersectWorld(-1, PhysState, Mat, m_PunchMatrix, _pObj->m_iObject, &Info) && Info.m_bIsValid)
		{
			//ConOut("HIT SOMETHING");
			OnHit(Info, _pObj->m_iObject, (CVec3Dfp32::GetRow(Mat, 3) - CVec3Dfp32::GetRow(m_PunchMatrix, 3)).Normalize(), _Force);
		}
	}
	m_PunchMatrix = Mat;
	m_PunchTick = iTick;
}

void CRPG_Object_Fist::OnHit(CCollisionInfo &_Info, int _iObject, const CVec3Dfp32 &_SplatterDir, const CVec3Dfp32 &_Force)
{
	CWObject *pHitObj = m_pWServer->Object_Get(_Info.m_iObject);
	if(pHitObj)
	{
		if(pHitObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER)
		{
			// Hmm, hit another character, what should the result of that be??
			// Maybe enter fight mode...?
			if(m_HitCharacterDelayCountdown == 0)
			{
				ConOut("CRPG_Object_Fist::OnHit: Punch hit Character, please advise!");
			}
		}
		else if(pHitObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_TRIGGER)
		{
			if(m_HitObjectDelayCountdown == 0)
			{
				//HMMMM
				//m_pWServer->Sound_At(_Info.m_Pos, m_iSound_HitWorld, 0);
				// Send damage?
				//SendDamage(_Info.m_iObject, _Info.m_Pos, _iObject, m_Damage.m_Damage, m_Damage.m_DamageType, _Info.m_SurfaceType, &_Force);
				
				CWObject_ClientDebris::CreateClientDebris(&_Info, _SplatterDir, m_pWServer);
			}
		}
		else
		{
			if(m_HitWorldDelayCountdown == 0)
			{
				//HMMM
				//m_pWServer->Sound_At(_Info.m_Pos, m_iSound_HitWorld, 0);
				// Send damage?
				//SendDamage(_Info.m_iObject, _Info.m_Pos, _iObject, m_Damage.m_Damage, m_Damage.m_DamageType, _Info.m_SurfaceType, &_Force);
				
				CWObject_ClientDebris::CreateClientDebris(&_Info, _SplatterDir, m_pWServer);
			}
		}
	}
}

//-------------------------------------------------------------------
bool CRPG_Object_Fist::IsMeleeWeapon()
{
	return true;
}

bool CRPG_Object_Fist::CanActivate(int _Pos, int _iObject, int _iOpponent, int _GameTick)
{
	// HMM WHAT TO DO HERE IN THE NEW VERSION !!!!! FIXME

	return false;
}

int CRPG_Object_Fist::GetManageableActions(int _GameTick, int _iObject, int _iOpponent)
{
	// Mmmkay, not really sure how to do this, so... go through list
	// of blocks and ask them if they can be activated
	int Result = FIST_ACTION_NONE;

	return Result;
}

//-------------------------------------------------------------------

void CRPG_Object_Fist::UpdateInput(int _Input)
{
	// Add new input
	m_Input |= _Input;
}

fp32 CRPG_Object_Fist::GetMoveLeftRight(int _iObject)
{
	CWObject* pObj = m_pWServer->Object_Get(_iObject);
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pObj);

	if (pCD == NULL)
		return 0;

	return pCD->m_Control_Move[1];
}

fp32 CRPG_Object_Fist::GetMoveUpDown(int _iObject)
{
	CWObject* pObj = m_pWServer->Object_Get(_iObject);
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pObj);

	if (pCD == NULL)
		return 0;

	return pCD->m_Control_Move[0];
}

int CRPG_Object_Fist::GetActiveStance(int _iObject)
{
	return FIST_ACTION_NONE;
}

bool CRPG_Object_Fist::CharIsPlayer(int _iObject)
{
	CWObject* pObj = m_pWServer->Object_Get(_iObject);
	if (!pObj)
		return false;

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pObj);

	if (!pCD)
		return false;

	return (pCD->m_iPlayer != -1);
}

bool CRPG_Object_Fist::CharIsPlayer(CWO_Character_ClientData* _pCD)
{
	if (_pCD)
		return (_pCD->m_iPlayer != -1);

	return false;
}

bool CRPG_Object_Fist::CanEnterFightMode()
{
	int GameTick = m_pWServer->GetGameTick();
	// Make sure the schedule is open on all channels except stance
	// HMM WHAT TO CHECK HERE IN NEW VERSION !!!!! FIXME

	return false;
}

bool CRPG_Object_Fist::ShouldBeDead()
{
	int GameTick = m_pWServer->GetGameTick();

	// HMM WHAT TO DO HERE IN NEW VERSION !!!!! FIXME

	return m_bShouldBeDead;
}

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Fist, CRPG_Object_Fist_Parent);*/


// TEST

#ifndef M_DISABLE_TODELETE
void CRPG_Object_KnuckleDuster::OnCreate()
{
	CRPG_Object_KnuckleDusterParent::OnCreate();
	m_DamageModifier = 1;
}

bool CRPG_Object_KnuckleDuster::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	switch (_KeyHash)
	{
	case MHASH4('DAMA','GEMO','DIFI','ER'): // "DAMAGEMODIFIER"
		{
			m_DamageModifier = _pKey->GetThisValue().Val_int();
			break;
		}
	default:
		{
			return CRPG_Object_KnuckleDusterParent::OnEvalKey(_pKey);
			break;
		}
	}

	return true;
}

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_KnuckleDuster, CRPG_Object_KnuckleDusterParent);
#endif

// New melee base
void CRPG_Object_Melee::OnCreate()
{
	CRPG_Object_MeleeParent::OnCreate();
	//m_DamageModifier = 1;
	m_iSound_HitRight = -1;
	m_iSound_HitLeft = -1;
	m_iSound_HitMiddle = -1;
	m_iSound_HitSpecial = -1;
	m_iSound_HitSurface = -1;
	m_iSound_HitBlock = -1;
	for (int32 i = 0; i < MELEE_ATTACK_NUMATTACKS; i++)
		m_AttackDamage[i] = 0;
}

void CRPG_Object_Melee::OnIncludeClass(const CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
{
	CRPG_Object_MeleeParent::OnIncludeClass(_pReg, _pMapData, _pWServer);

	IncludeSoundFromKey("SOUND_HITRIGHT", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_HITLEFT", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_HITMIDDLE", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_HITSPECIAL", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_HITSURFACE", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_HITBLOCK", _pReg, _pMapData);
}

bool CRPG_Object_Melee::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	/*
	case MHASH4('DAMA','GEMO','DIFI','ER'): // "DAMAGEMODIFIER"
	{
	m_DamageModifier = KeyValue.Val_int();
	break;
	}
	*/

	if(KeyName.Find("SOUND_HITRIGHT") != -1)
	{
		m_iSound_HitRight = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
	}
	else if(KeyName.Find("SOUND_HITLEFT") != -1)
	{
		m_iSound_HitLeft = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
	}
	else if(KeyName.Find("SOUND_HITMIDDLE") != -1)
	{
		m_iSound_HitMiddle = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
	}
	else if(KeyName.Find("SOUND_HITSPECIAL") != -1)
	{
		m_iSound_HitSpecial = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
	}
	else if(KeyName.Find("SOUND_HITSURFACE") != -1)
	{
		m_iSound_HitSurface = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
	}
	else if(KeyName.Find("SOUND_HITBLOCK") != -1)
	{
		m_iSound_HitBlock = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
	}
	else if (KeyName.Find("ATTACKDAMAGE") != -1)
	{
		ResolveAttackDamage(KeyName,KeyValue);
	}
	else
		return CRPG_Object_MeleeParent::OnEvalKey(_KeyHash, _pKey);

	return true;
}

void CRPG_Object_Melee::ResolveAttackDamage(const CStr& _Key, const CStr& _Value)
{
	if (_Key.Find("RIGHT") != -1)
		m_AttackDamage[MELEE_ATTACK_RIGHT] = (uint8)_Value.Val_int();
	else if (_Key.Find("LEFT") != -1)
		m_AttackDamage[MELEE_ATTACK_LEFT] = (uint8)_Value.Val_int();
	else if (_Key.Find("MIDDLE") != -1)
		m_AttackDamage[MELEE_ATTACK_MIDDLE] = (uint8)_Value.Val_int();
	else if (_Key.Find("SPECIAL") != -1)
		m_AttackDamage[MELEE_ATTACK_SPECIAL] = (uint8)_Value.Val_int();
}

int32 CRPG_Object_Melee::GetDamage(int32 _AttackIndex)
{
	if ((_AttackIndex < 0) || (_AttackIndex >= MELEE_ATTACK_NUMATTACKS)) 
		return 0;

	return m_AttackDamage[_AttackIndex];
}

int32 CRPG_Object_Melee::GetSoundIndex(int32 _Type)
{
	/*switch (_Type)
	{
	case AG_FIGHTMODE_SOUNDTYPE_RIGHT:
		return m_iSound_HitRight;
	case AG_FIGHTMODE_SOUNDTYPE_LEFT:
		return m_iSound_HitLeft;
	case AG_FIGHTMODE_SOUNDTYPE_MIDDLE:
		return m_iSound_HitMiddle;
	case AG_FIGHTMODE_SOUNDTYPE_SPECIAL:
		return m_iSound_HitSpecial;
	case AG_FIGHTMODE_SOUNDTYPE_SURFACE:
		return m_iSound_HitSurface;
	case AG_FIGHTMODE_SOUNDTYPE_BLOCK:
		return m_iSound_HitBlock;
	default:
		return -1;
	}*/
	return -1;
}

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Melee, CRPG_Object_MeleeParent);
