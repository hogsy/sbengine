#include "PCH.h"

#include "WRPGMiniGun.h"
#include "../WObj_Char.h"
#include "WRPGChar.h"

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_MiniGun, CRPG_Object_Weapon);


void CRPG_Object_MiniGun::OnCreate()
{
	CRPG_Object_Weapon::OnCreate();
	m_bSecondaryFire = false;
	m_Secondary_WaitTicks = (int)(1.5f * m_pWServer->GetGameTicksPerSecond());
	m_Secondary_AttachPoint[0] = -1;
	m_Secondary_AttachPoint[1] = -1;
	m_Secondary_AttachPoint[2] = -1;
	m_Secondary_RotTrack = 1;
	m_iCurAttachPoint = 0;
	m_BuildUpTick = -1;
	m_BuildUpDelayTicks = (int16)(0.93f * m_pWServer->GetGameTicksPerSecond());
	m_bSkipMuzzle = false;
	m_bDefaultRicochet = true;
	m_CurrentInput = -1;
	m_AGState = MINIGUN_AGSTATE_IDLE;
}

void CRPG_Object_MiniGun::OnIncludeClass(const CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
{
	CRPG_Object_Weapon::OnIncludeClass(_pReg, _pMapData, _pWServer);

	IncludeClassFromKey("SECONDARY_SPAWN", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_START", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_LOOP", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_STOP", _pReg, _pMapData);

	IncludeAnimFromKey("ANIM_START", _pReg, _pMapData);
	IncludeAnimFromKey("ANIM_LOOP", _pReg, _pMapData);
	IncludeAnimFromKey("ANIM_STOP", _pReg, _pMapData);
}

bool CRPG_Object_MiniGun::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr Val = _pKey->GetThisValue();
	switch (_KeyHash)
	{
	case MHASH4('SECO','NDAR','Y_SP','AWN'): // "SECONDARY_SPAWN"
		{
			m_bSecondaryFire = true;
			m_Secondary_Spawn = Val;
			break;
		}

	case MHASH4('SECO','NDAR','Y_WA','IT'): // "SECONDARY_WAIT"
		{
			m_Secondary_WaitTicks = RoundToInt(_pKey->GetThisValuef() * m_pWServer->GetGameTicksPerSecond());
			break;
		}

	case MHASH6('SECO','NDAR','Y_AT','TACH','POIN','TS'): // "SECONDARY_ATTACHPOINTS"
		{
			for(int i = 0; i < 3; i++)
				m_Secondary_AttachPoint[i] = Val.GetStrSep(",").Val_int();
			break;
		}
	case MHASH5('SECO','NDAR','Y_RO','TTRA','CK'): // "SECONDARY_ROTTRACK"
		{
			m_Secondary_RotTrack = Val.Val_int();
			break;
		}

	case MHASH6('SECO','NDAR','Y_BU','ILDU','PDEL','AY'): // "SECONDARY_BUILDUPDELAY"
		{
			m_BuildUpDelayTicks = RoundToInt(Val.Val_fp64() * m_pWServer->GetGameTicksPerSecond());
			break;
		}

	case MHASH3('SOUN','D_ST','ART'): // "SOUND_START"
		{
			m_iSound_Start = m_pWServer->GetMapData()->GetResourceIndex_Sound(Val);
			break;
		}

	case MHASH3('SOUN','D_LO','OP'): // "SOUND_LOOP"
		{
			m_iSound_Loop = m_pWServer->GetMapData()->GetResourceIndex_Sound(Val);
			break;
		}

	case MHASH3('SOUN','D_ST','OP'): // "SOUND_STOP"
		{
			m_iSound_Stop = m_pWServer->GetMapData()->GetResourceIndex_Sound(Val);
			break;
		}

	case MHASH3('ANIM','_STA','RT'): // "ANIM_START"
		{
			m_iAnim_Start = ResolveAnimHandle(Val);
			break;
		}

	case MHASH3('ANIM','_LOO','P'): // "ANIM_LOOP"
		{
			m_iAnim_Loop = ResolveAnimHandle(Val);
			break;
		}

	case MHASH3('ANIM','_STO','P'): // "ANIM_STOP"
		{
			m_iAnim_Stop = ResolveAnimHandle(Val);
			break;
		}

	case MHASH3('BUIL','DUPD','ELAY'): // "BUILDUPDELAY"
		{
			m_BuildUpDelayTicks = RoundToInt(Val.Val_fp64() * m_pWServer->GetGameTicksPerSecond());
			break;
		}

	case MHASH3('FORC','EDPI','CKUP'): // "FORCEDPICKUP"
		{
			m_ForcedPickup = Val;
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

TPtr<class CRPG_Object_Item> CRPG_Object_MiniGun::GetForcedPickup()
{
	// Create rpgobject from the forced pickup
	return (TPtr<class CRPG_Object_Item>)(CRPG_Object_Item*)(CRPG_Object*)CRPG_Object::CreateObject(m_ForcedPickup, m_pWServer);
}

bool CRPG_Object_MiniGun::DrawAmmo(int _Num, const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, bool _bApplyFireTimeout)
{
	if(m_bSkipMuzzle)
		m_bSkipMuzzle = false;
	else
		FireMuzzle(_iObject);

	bool bResult = CRPG_Object_Weapon::DrawAmmo(_Num, _Mat, _pRoot, _iObject, _bApplyFireTimeout);
	// Turn off sound if ammo is zero
	if(m_AmmoLoad <= 0)
	{
		CWObject *pObj = m_pWServer->Object_Get(_iObject);
		if(pObj)
			pObj->iSound(0) = 0;
	}

	return bResult;
}

bool CRPG_Object_MiniGun::OnProcess(CRPG_Object *_pRoot, int _iObject)
{
	RefreshMuzzle(_iObject);
	return CRPG_Object_Weapon::OnProcess(_pRoot, _iObject);
}

bool CRPG_Object_MiniGun::Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	/*CRPG_Object_Char *pRPGChar = (CRPG_Object_Char *)_pRoot;
	CRPG_Object_Item *pItem = pRPGChar->GetInventory(1)->GetEquippedItem();
	if(m_iItemType == 0x10 && pItem && pItem->m_iItemType == 0x111)
	{
		// Riot-guard dual weapon hack
		pRPGChar->QuickActivateItem(pItem, _Mat, _iObject, _Input);
	}*/

	if(_Input == 2)
	{
		if (m_CurrentInput == -1)
			m_CurrentInput = 2;

		//Use secondary fire if available, otherwise use weapon secondary (melee attack)
		if (m_bSecondaryFire)
		{
			CRPG_InitParams InitParams;
			InitParams.Clear();
			InitParams.m_iCreator = _iObject;
			InitParams.m_pRPGCreator = _pRoot;
			InitParams.m_pRPGCreatorItem = this;

			CMat4Dfp32 Effect, Res;
			Effect = GetAttachMatrix(_iObject, m_Secondary_RotTrack, m_Secondary_AttachPoint[m_iCurAttachPoint++]);
			if(m_iCurAttachPoint >= 3 || m_Secondary_AttachPoint[m_iCurAttachPoint] == -1)
				m_iCurAttachPoint = 0;

			Res = _Mat;
			Res.k[3][0] = Effect.k[3][0];
			Res.k[3][1] = Effect.k[3][1];
			Res.k[3][2] = Effect.k[3][2];

			m_SpawnedObjectGUID = 0;
			m_bSkipMuzzle = true;
			fp32 Ofs = m_StartOffset;
			m_StartOffset = 0;
			CRPG_Object_Weapon::Summon(m_Secondary_Spawn, _iObject, Res, &InitParams, 1, true, true);
			m_StartOffset = Ofs;
			ApplyWait(_iObject, m_Secondary_WaitTicks, m_NoiseLevel, m_Visibility);
			return true;
		}
		else
			return CRPG_Object_Weapon::Activate(_Mat, _pRoot, _iObject, _Input);
	}
	else
	{
		m_CurrentInput = _Input;
		int GameTick = GetGameTick(_iObject);
		CWObject *pObj = m_pWServer->Object_Get(_iObject);
		if(m_BuildUpTick == -1)
		{
			m_BuildUpTick = GameTick + m_BuildUpDelayTicks;
/* 			if(pObj && pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER)
				m_pWServer->Sound_Global(m_iSound_Start);
			else*/
				m_pWServer->Sound_At(CVec3Dfp32::GetRow(_Mat, 3), m_iSound_Start, 0);
		}
		// Send anim impulse
		if (m_AGState == MINIGUN_AGSTATE_WINDDOWN || m_AGState == MINIGUN_AGSTATE_IDLE)
		{
			CFStr Data("5,100");
			CWObject_Message Msg(OBJMSG_CHAR_WEAPONANIMIMPULSE,6);
			Msg.m_pData = (void*)Data.Str();
			m_pWServer->Message_SendToObject(Msg,_iObject);
			m_AGState = MINIGUN_AGSTATE_WINDUP;
		}
		if (m_BuildUpTick <= GameTick)
		{
			if(m_BuildUpTick != 0)
			{
				m_BuildUpTick = 0;
				if(pObj)
				{
					//m_pWServer->Sound_At(_iObject, m_iSound_Loop, 0);
					pObj->iSound(0) = m_iSound_Loop;
				}
			}

			return CRPG_Object_Weapon::Activate(_Mat, _pRoot, _iObject, _Input);
		}
		else
			return false;
	}
}

bool CRPG_Object_MiniGun::Deactivate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
 	m_BuildUpTick = -1;
	CWObject *pObj = m_pWServer->Object_Get(_iObject);
	if(pObj)
		pObj->iSound(0) = 0;

/*	if(pObj && pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER)
		m_pWServer->Sound_Global(m_iSound_Stop);
	else*/
		m_pWServer->Sound_At(CVec3Dfp32::GetRow(_Mat, 3), m_iSound_Stop, 0);

	m_CurrentInput = -1;
	if (m_AGState == MINIGUN_AGSTATE_WINDUP || m_AGState == MINIGUN_AGSTATE_LOOPING)
	{
		CFStr Data("5,200");
		CWObject_Message Msg(OBJMSG_CHAR_WEAPONANIMIMPULSE,6);
		Msg.m_pData = (void*)Data.Str();
		m_pWServer->Message_SendToObject(Msg,_iObject);
		m_AGState = MINIGUN_AGSTATE_WINDDOWN;
	}

	return CRPG_Object_Weapon::Deactivate(_Mat, _pRoot, _iObject, _Input);
}


bool CRPG_Object_MiniGun::MergeItem(int _iObject, CRPG_Object_Item *_pObj)
{
	CRPG_Object_Summon* pOther = safe_cast<CRPG_Object_Summon>(_pObj);
	if (!pOther)
		return false;

	if(m_AmmoLoad != -1 && m_MaxAmmo != -1)
	{
		m_AmmoLoad += pOther->m_AmmoLoad;

		//Hardcode this bugger. maxammo is 0 for some reason and I don't want to touch that right now...
 		if (m_AmmoLoad > 4000)
			m_AmmoLoad = 4000;
	}

	//Weapon mergeitem is dependent on magazines, which minigun doesn't have. 
	//Run this to play sound/show icons etc though
	return CRPG_Object_Weapon::MergeItem(_iObject, _pObj);
}



//CRPG_Object_Drill////////////////////////////////////////////////////////////////////////////////
MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Drill, CRPG_Object_MiniGun);

void CRPG_Object_Drill::OnCreate()
{
	CRPG_Object_MiniGun::OnCreate();
	for(int i = 0; i < 2; i++)
		m_lMelee_AttachPoints[i] = -1;
}


bool CRPG_Object_Drill::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr Val = _pKey->GetThisValue();

	switch (_KeyHash)
	{
	case MHASH5('MELE','E_AT','TACH','POIN','TS'): // "MELEE_ATTACHPOINTS"
		{
			for(int i = 0; i < 2; i++)
				m_lMelee_AttachPoints[i] = Val.GetStrSep(",").Val_int();
			break;
		}
	default:
		{
			return CRPG_Object_MiniGun::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}

	return true;
}


bool CRPG_Object_Drill::IsActivatable(CRPG_Object* _pRoot,int _iObject,int _Input)
{
	//Never care about ammo
	int Wait = ((CRPG_Object_Char*)_pRoot)->Wait();
	if (Wait != 0)
		return false;

	return true;
}

bool CRPG_Object_Drill::Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
//	CRPG_Object_Char *pRPGChar = (CRPG_Object_Char *)_pRoot;
//	CRPG_Object_Item *pItem = pRPGChar->GetInventory(1)->GetEquippedItem();

	if(_Input == 2)
	{
		//Secondary fire, handle as with minigun
		return CRPG_Object_MiniGun::Activate(_Mat, _pRoot, _iObject, _Input);
	}
	else
	{
		//Primary attack 
  		int GameTick = GetGameTick(_iObject);
		CWObject *pObj = m_pWServer->Object_Get(_iObject);

		if(m_BuildUpTick == -1)
		{
			//Firing up drill
			m_BuildUpTick = GetGameTick(_iObject) + m_BuildUpDelayTicks;
/*			if(pObj && pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER)
				m_pWServer->Sound_Global(m_iSound_Start);
			else*/
				m_pWServer->Sound_At(CVec3Dfp32::GetRow(_Mat, 3), m_iSound_Start, 0);
		}
		if(m_BuildUpTick <= GameTick)
		{
			//Drill is in full motion
			if(m_BuildUpTick != 0)
			{
				m_BuildUpTick = 0;
			}

			if(pObj)
				pObj->iSound(0) = m_iSound_Loop;

		}

		//Perform melee attack every firetimeout ticks until deactivated
		if ((GameTick % m_FireTimeout) == 0)
		{
			PerformMeleeAttack(_Mat, _iObject);
			return true;
		}
		else
			return false;
	}
}


void CRPG_Object_Drill::PerformMeleeAttack(const CMat4Dfp32 &_Mat, int _iObject)
{
	//Should perhaps use tracelines instead of selection...whatever
	//Check if we need to set AttachInfo stuff
	CAttachInfo Info;
	if ((m_lMelee_AttachPoints[0] != -1) || (m_lMelee_AttachPoints[1] != -1))
	{
		//Set attach info stuff
		CWObject *pObj = m_pWServer->Object_Get(_iObject);
		if(!pObj)
			return;

		//Get owners animstate (including skeleton instance)
		CXR_AnimState AnimState;
		CWObject_Message Msg(OBJSYSMSG_GETANIMSTATE);
		Msg.m_pData = &AnimState;
		Msg.m_DataSize = sizeof(AnimState);
		if(!m_pWServer->Message_SendToObject(Msg, _iObject))
			return;

		//Get owners skeleton
		CXR_Model *pChar = m_pWServer->GetMapData()->GetResource_Model(pObj->m_iModel[0]);
		CXR_Skeleton *pSkelChar = pChar ? (CXR_Skeleton*)pChar->GetParam(MODEL_PARAM_SKELETON) : NULL;
		if(!AnimState.m_pSkeletonInst || !pSkelChar)
			return;

		//Set owner related attach info stuff 
		Info.m_iCharAttachPoint = m_Model.m_iAttachPoint[0];	//Items attach point on owner
		Info.m_iCharRotTrack = m_Model.m_iAttachRotTrack;		//Items rottrack on owner
		Info.m_pCharSkeleton = pSkelChar;
		Info.m_pSkelInstance = AnimState.m_pSkeletonInst;

		//Set item related attach info stuff
		CXR_Model *pItem = m_pWServer->GetMapData()->GetResource_Model(m_Model.m_iModel[0]);
		Info.m_pItemSkeleton = pItem ? (CXR_Skeleton*)pItem->GetParam(MODEL_PARAM_SKELETON) : NULL;
	}

	//Get selection attach point position
	CVec3Dfp32 Pos;
	if (m_lMelee_AttachPoints[0] == -1)
	{
		//Default position
		Pos = CVec3Dfp32::GetRow(_Mat, 3) + CVec3Dfp32::GetRow(_Mat, 0) * 20;
	}
	else
	{
		//Get attach point in world coordinates
		Info.m_iItemAttachPoint = m_lMelee_AttachPoints[0];
		CMat4Dfp32 Mat = GetAttachOnItemMatrix(Info);
		Pos = CVec3Dfp32::GetRow(Mat, 3);
	}

	CVec3Dfp32 Size;
	if (m_lMelee_AttachPoints[1] == -1)
	{
		//Default size
		Size = CVec3Dfp32(20.0f, 20.0f, 20.0f);
	}
	else
	{
		//Get attach point in world coordinates
		Info.m_iItemAttachPoint = m_lMelee_AttachPoints[1];
		CMat4Dfp32 Mat = GetAttachOnItemMatrix(Info);
		Size = CVec3Dfp32::GetRow(Mat, 3) - Pos;

		//Beef out narrow sizes
		for (int i = 0; i < 3; i++)
		{
			if (Abs(Size[i]) < 5.0f)
			{
				Size[i] = ((Size[i] > 0.0f) ? 5.0f : -5.0f);
			}
		}
	}

	//Make selection
	const int16 *pSel;
	int nSel = FindMeleeAttackSelection(Pos, Size, _iObject, &pSel);

	CVec3Dfp32 Force = 0;
	if(m_MeleeForce > 0)
		Force = CVec3Dfp32::GetRow(_Mat, 0) * m_MeleeForce;

	//Deal damage to any objects in selection (potential shish-kebab if nSel > 1 :) )
	for(int i = 0; i < nSel; i++)
	{
		int iObj = pSel[i];

		//Set damage based on wheter we hit target in front or rear
		int Damage;
		fp32 Angle = M_Fabs(GetMeleeAttackAngle(_Mat, m_pWServer->Object_GetPositionMatrix(iObj)));
		if (Angle < 0.15f)
			Damage = m_MeleeDamage_FromBehind;
		else
			Damage = m_MeleeDamage_Frontal;

		//Ouchie!
		SendDamage(iObj, Pos, _iObject, Damage, m_Damage.m_DamageType, -1, &Force);
		SendMsg(iObj, OBJMSG_RPG_SPEAK_OLD, 1);

		//Play hit sound
		bool bPlaySound = false;
		if (m_MeleeSoundTick == 0)
		{
			bPlaySound = true;
		} else {
			int TickDiff = m_pWServer->GetGameTick() - m_MeleeSoundTick;
			if (TickDiff > 10)		// Half a second between melee hit sounds.
				bPlaySound = true;
		}
		if (bPlaySound)
		{
			m_MeleeSoundTick = m_pWServer->GetGameTick();
			m_pWServer->Sound_At(Pos, m_iMeleeHitSound, 0);
		}
	}
}


int CRPG_Object_Drill::FindMeleeAttackSelection(const CVec3Dfp32 &_Pos, const CVec3Dfp32& _Size, int _iObject, const int16** _ppRetList)
{
	//Size must be positive in all dimensions; offset position accordingly
	CVec3Dfp32 Pos = _Pos;
	CVec3Dfp32 Size = _Size;
	for (int i = 0; i < 3; i++)
	{
		if (Size[i] < 0.0f)
		{
			Pos[i] += Size[i];
			Size[i] = -Size[i];
		}
	}

	TSelection<CSelection::LARGE_BUFFER> Selection;
	CWO_PhysicsState PhysState;
	PhysState.AddPhysicsPrim(0, CWO_PhysicsPrim(OBJECT_PRIMTYPE_BOX, 0, Size, 0));
	PhysState.m_ObjectFlags = OBJECT_FLAGS_PROJECTILE;
	PhysState.m_ObjectIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
	PhysState.m_iExclude = _iObject;

	m_pWServer->Selection_AddIntersection(Selection, Pos, PhysState);
	int nSel = m_pWServer->Selection_Get(Selection, _ppRetList);

#ifndef M_RTM
	//DEBUG
	static const int s_lSides[12][2][3] = 
	{
		{{0,0,0},{0,0,1}},
		{{0,0,0},{0,1,0}},
		{{0,0,0},{1,0,0}},
		{{0,0,1},{0,1,1}},
		{{0,0,1},{1,0,1}},
		{{0,1,0},{0,1,1}},
		{{0,1,0},{1,1,0}},
		{{0,1,1},{1,1,1}},
		{{1,0,0},{1,0,1}},
		{{1,0,0},{1,1,0}},
		{{1,0,1},{1,1,1}},
		{{1,1,0},{1,1,1}},
	};
	CPixel32 Colour = (nSel > 0) ? 0xffff0000 : 0xff00ff00; 
	for (int i = 0; i < 12; i++)
	{
		CVec3Dfp32 From = Pos + CVec3Dfp32(s_lSides[i][0][0] * Size[0], s_lSides[i][0][1] * Size[1], s_lSides[i][0][2] * Size[2]);
		CVec3Dfp32 To = Pos + CVec3Dfp32(s_lSides[i][1][0] * Size[0], s_lSides[i][1][1] * Size[1], s_lSides[i][1][2] * Size[2]); 
		m_pWServer->Debug_RenderWire(From, To, Colour, 1.0f);
	}
	//DEBUG
#endif

	return nSel;
}

