#include "PCH.h"

#include "WRPGInitParams.h"
#include "WRPGChar.h"
#include "WRPGSummon.h"
#include "WRPGAmmo.h"
#include "../WObj_Char.h"
#include "../WObj_Misc/WObj_Shell.h"
#include "..\..\Shared\mos\XRModels\Model_BSP4Glass\WBSP4Glass.h"

/*
#include "../WObj_RPG.h"
#include "../WObj_Weapons/WObj_Spells.h"
#include "../WObj_Weapons/WObj_ClientDebris.h"
#include "MFloat.h"
*/

enum
{
	class_WorldGlassSpawn =			MHASH7('CWOb','ject','_','Worl','dGla','ssSp','awn'),
	class_Glass_Dynamic =			MHASH7('CWOb','ject','_','Glas','s_Dy','nami','c'),
};

//-------------------------------------------------------------------
//- CRPG_Object_Summon ----------------------------------------------
//-------------------------------------------------------------------

void CRPG_Object_Summon::OnCreate()
{
	MAUTOSTRIP(CRPG_Object_Summon_OnCreate, MAUTOSTRIP_VOID);
	CRPG_Object_Item::OnCreate();

	//Add ai type flag
	m_SummonDelayTicks = 0;
	m_CurSummonDelayTicks = 0;

	m_StartOffset = 0;

	m_SpawnedObjectGUID = 0;

	m_MeleeDelay = 0;
	m_PendingMeleeAttack = -1;
	m_MeleeForce = 4.0f;

	m_LinkedFireItem = 0;
	m_bLockedLinkedFire = false;

	m_iMeleeHitSound = 0;
	m_MeleeSoundPeriod = SUMMON_MELEE_HITSOUND_PERIOD;
	m_MeleeSoundTick = 0;
	m_MeleeDamage_Frontal = 2;
	m_MeleeDamage_FromBehind = 8;
	m_MeleeFwdOffset = 20;
	m_MeleeHalfWidth = 0;
	m_MeleeHalfDepth = 0;
	m_MeleeMinHeight = 0;
}

//-------------------------------------------------------------------

void CRPG_Object_Summon::OnIncludeClass(const CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CRPG_Object_Summon_OnIncludeClass, MAUTOSTRIP_VOID);
	CRPG_Object_Item::OnIncludeClass(_pReg, _pMapData, _pWServer);
	
	IncludeClassFromKey("SPAWN", _pReg, _pMapData);
	IncludeClassFromKey("PROJECTILE", _pReg, _pMapData);
	IncludeClassFromKey("SPAWNEFFECT", _pReg, _pMapData);
	IncludeClassFromKey("SUMMON_PROJECTILECLASS", _pReg, _pMapData);
	IncludeRPGClassFromKey("GIVEONPICKUP", _pReg, _pMapData, _pWServer);
	IncludeSoundFromKey("MELEE_HITSOUND", _pReg, _pMapData);
}

//-------------------------------------------------------------------

bool CRPG_Object_Summon::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
 	MAUTOSTRIP(CRPG_Object_Summon_OnEvalKey, false);

	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	int KeyValuei = KeyValue.Val_int();
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();

	switch (_KeyHash)
	{
	case MHASH3('SPAW','NEFF','ECT'): // "SPAWNEFFECT"
	case MHASH2('EFFE','CT'): // "EFFECT"
		{
			m_SpawnEffect = KeyValue;
			m_pWServer->GetMapData()->GetResourceIndex_Class(m_SpawnEffect);
			break;
		}

	case MHASH6('SUMM','ON_P','ROJE','CTIL','ECLA','SS'): // "SUMMON_PROJECTILECLASS"
		{
			m_Spawn = KeyValue;
			m_pWServer->GetMapData()->GetResourceIndex_Class(m_Spawn);
			break;
		}

	case MHASH4('SUMM','ON_A','MMOL','OAD'): // "SUMMON_AMMOLOAD"
		{
			m_AmmoLoad = KeyValuei;
			break;
		}

	case MHASH4('SUMM','ON_M','AXAM','MO'): // "SUMMON_MAXAMMO"
		{
			m_MaxAmmo = KeyValuei;
			break;
		}

	case MHASH3('GIVE','ONPI','CKUP'): // "GIVEONPICKUP"
		{
			m_GiveOnPickup = KeyValue;
			break;
		}

	case MHASH3('SUMM','ONDE','LAY'): // "SUMMONDELAY"
		{
			m_SummonDelayTicks = (int)(KeyValuef * m_pWServer->GetGameTicksPerSecond());
			break;
		}

	case MHASH3('STAR','TOFF','SET'): // "STARTOFFSET"
		{
			m_StartOffset = KeyValuef;
			break;
		}

	case MHASH3('MELE','E_FO','RCE'): // "MELEE_FORCE"
		{
			m_MeleeForce = KeyValuef;
			break;
		}

	case MHASH3('MELE','E_DE','LAY'): // "MELEE_DELAY"
		{
			m_MeleeDelay = KeyValuei;
			break;
		}

	case MHASH4('MELE','E_HI','TSOU','ND'): // "MELEE_HITSOUND"
		{
			m_iMeleeHitSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}

	case MHASH5('MELE','E_SO','UNDP','ERIO','D'): // "MELEE_SOUNDPERIOD"
		{
			m_MeleeSoundPeriod = Max(10,KeyValuei);
			break;
		}

	case MHASH5('MELE','E_DA','MAGE','_FRO','NTAL'): // "MELEE_DAMAGE_FRONTAL"
		{
			m_MeleeDamage_Frontal = KeyValuei;
			break;
		}

	case MHASH6('MELE','E_DA','MAGE','_FRO','MBEH','IND'): // "MELEE_DAMAGE_FROMBEHIND"
		{
			m_MeleeDamage_FromBehind = KeyValuei;
			break;
		}

	case MHASH5('MELE','E_FO','RWAR','DOFF','SET'): // "MELEE_FORWARDOFFSET"
		{
			m_MeleeFwdOffset = Max(0, Min(255, KeyValuei));
			break;
		}

	case MHASH3('MELE','E_WI','DTH'): // "MELEE_WIDTH"
		{
			m_MeleeHalfWidth = Max(0, Min(255, (int)(KeyValuef / 2.0f)));
			break;
		}

	case MHASH3('MELE','E_DE','PTH'): // "MELEE_DEPTH"
		{
			m_MeleeHalfDepth = Max(0, Min(255, (int)(KeyValuef / 2.0f)));
			break;
		}

	case MHASH3('MELE','E_HE','IGHT'): // "MELEE_HEIGHT"
		{
			m_MeleeMinHeight = Max(0, Min(255, KeyValuei));
			break;
		}

	case MHASH3('LINK','EDFI','RE'): // "LINKEDFIRE"
		{
			m_LinkedFireItem = KeyValuei;
			break;
		}

	default:
		{
			return CRPG_Object_Item::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}

	return true;
}

//-------------------------------------------------------------------

int CRPG_Object_Summon::Summon(const char* _pClass, int _iObject, const CMat4Dfp32& _Mat, CRPG_InitParams* _pInitParams, int _NumDrawAmmo, bool _bApplyWait, bool _bFinalPos)
{
	MAUTOSTRIP(CRPG_Object_Summon_Summon, 0);
  	if(_pClass == NULL)
		return 0;

	int iCurrentObject;

	//Offset summon pos by start offset units in summon direction
  	CMat4Dfp32 Matrix = _Mat;
	CVec3Dfp32 Offset = (CVec3Dfp32::GetMatrixRow(Matrix, 0) * m_StartOffset);
	//m_pWServer->Debug_RenderVector(CVec3Dfp32::GetMatrixRow(Matrix, 3), Offset, 0xFFFF0000, 20.0f);
	Offset.AddMatrixRow(Matrix, 3);
	//m_pWServer->Debug_RenderWire(CVec3Dfp32::GetMatrixRow(Matrix, 3) + CVec3Dfp32(0,0,2), CVec3Dfp32::GetMatrixRow(Matrix, 3) + CVec3Dfp32(0,0,2) + CVec3Dfp32::GetMatrixRow(Matrix, 0) * 50.0f, 0xFFFFFF00, 20.0f);//DEBUG

	if(m_SpawnedObjectGUID != 0 && (iCurrentObject = m_pWServer->Object_GetIndex(m_SpawnedObjectGUID)) != 0)
	{
		// Use already spawned object?
		CWObject_Message Msg(OBJMSG_SUMMON_SPAWNPROJECTILES);
		Msg.m_Param0 = _pInitParams->m_DamageDeliveryFlags;
		Msg.m_pData = (void*)&Matrix;
		if(m_pWServer->Message_SendToObject(Msg, iCurrentObject))
		{
			DrawAmmo(_NumDrawAmmo, Matrix, _pInitParams->m_pRPGCreator, _pInitParams->m_iCreator, _bApplyWait);
			return iCurrentObject;
		}
	}

	const int nParams = 1;
	aint pParams[nParams] = { (aint)_pInitParams };

	int iObj = m_pWServer->Object_Create(_pClass, Matrix, _pInitParams->m_iCreator, pParams, nParams);
	if(iObj > 0)
	{
		DrawAmmo(_NumDrawAmmo, Matrix, _pInitParams->m_pRPGCreator, _pInitParams->m_iCreator, _bApplyWait);
		//int iEffect = m_pWServer->Object_Create(m_SpawnEffect, Res, _pInitParams->m_iCreator, pParams, nParams);
		SAVEMASK_SETBIT(LOADSAVEMASK_SUMMON_GUID);
		m_SpawnedObjectGUID = m_pWServer->Object_GetGUID(iObj);
		//iCurrentObject = m_pWServer->Object_GetIndex(m_SpawnedObjectGUID);
		return iObj;
	}

	return 0;
}

//-------------------------------------------------------------------

int CRPG_Object_Summon::Summon(const CMat4Dfp32& _Mat, CRPG_Object *_pRoot, int _iObject, int _NumDrawAmmo, bool _bApplyWait, int _DelayTicks)
{
	MAUTOSTRIP(CRPG_Object_Summon_Summon_2, 0);
	if (m_CurSummonDelayTicks > 0)
		return false;

	if (_DelayTicks > 0)
	{
		//int AnimDurationTicks = PlayAnimSequence(_iObject, GetActivateAnim(_iObject), PLAYER_ANIM_ONLYTORSO);
		m_CurSummonDelayTicks = _DelayTicks;

		if (_bApplyWait)
			ApplyWait(_iObject, _DelayTicks, m_NoiseLevel, m_Visibility);

		return true;
	}
	
	CRPG_InitParams InitParams;
	InitParams.Clear();
	InitParams.m_iCreator = _iObject;
	InitParams.m_pRPGCreator = _pRoot;
	InitParams.m_pRPGCreatorItem = this;
	InitParams.m_iTargetObj = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETTARGET), _iObject);
	
	return Summon(m_Spawn, _iObject, _Mat, &InitParams, _NumDrawAmmo, _bApplyWait);
}

//-------------------------------------------------------------------

aint CRPG_Object_Summon::OnMessage(CRPG_Object* _pParent, const CWObject_Message& _Msg, int _iOwner)
{
	MAUTOSTRIP(CRPG_Object_Summon_OnMessage, 0);
	switch (_Msg.m_Msg)
	{
		case OBJMSG_RPG_GETINITPARAMS:
		{
			CRPG_InitParams* pInitParams = (CRPG_InitParams*)_Msg.m_pData;
			if (pInitParams == NULL)
				return 0;

			pInitParams->Clear();

			pInitParams->m_iCreator = _iOwner;
			pInitParams->m_pRPGCreator = _pParent;
			pInitParams->m_pRPGCreatorItem = this;

/*			pInitParams->m_pDamage = &m_Damage;
			if(m_DamageEffect != "")
				pInitParams->m_pDamageEffect = m_DamageEffect;*/

			pInitParams->m_iTargetObj = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETTARGET), _iOwner);

			return 1;
		}
		case OBJMSG_SUMMON_LOCKLINKEDFIRE:
		{
			//We don't allow linked fire this tick
            m_bLockedLinkedFire = true;
			return 1;
		}
	}

	return CRPG_Object_Item::OnMessage(_pParent, _Msg, _iOwner);
}

//-------------------------------------------------------------------

bool CRPG_Object_Summon::IsActivatable(CRPG_Object* _pRoot,int _iObject,int _Input)
{
	if(_Input != 2 && m_AmmoLoad <= 0)
		return false;

	int Wait = ((CRPG_Object_Char*)_pRoot)->Wait() + m_iExtraActivationWait - GetGameTick(_iObject);
	if (Wait > 0)
		return false;

	return true;
}

//-------------------------------------------------------------------

bool CRPG_Object_Summon::OnProcess(CRPG_Object *_pRoot, int _iObject)
{
	MAUTOSTRIP(CRPG_Object_Summon_OnProcess, false);
	if (m_CurSummonDelayTicks > 0)
	{
		m_CurSummonDelayTicks--;

		if (m_CurSummonDelayTicks == 0)
		{
//			Error("OnProcess", "Oh this code is used...  better fixed it then");

			CMat4Dfp32 Mat;
			CWObject_Message Msg(OBJMSG_AIQUERY_GETACTIVATEPOSITION);
			Msg.m_pData = &Mat;
			Msg.m_DataSize = sizeof(CMat4Dfp32);
			if (!m_pWServer->Message_SendToObject(Msg, _iObject))
				Mat.Unit();

			Summon(Mat, _pRoot, _iObject, 1, false);

			if (m_FireTimeout > 0)
				ApplyWait(_iObject, m_FireTimeout, m_NoiseLevel, m_Visibility);
		}
	}

	if(m_PendingMeleeAttack != -1)
	{
		if (m_PendingMeleeAttack == 0)
		{
//	Error("OnProcess", "Oh this code is used...  better fixed it then");
			CMat4Dfp32 Mat;
			CWObject_Message Msg(OBJMSG_AIQUERY_GETACTIVATEPOSITION);
			Msg.m_pData = &Mat;
			Msg.m_DataSize = sizeof(CMat4Dfp32);
			if (!m_pWServer->Message_SendToObject(Msg, _iObject))
				Mat.Unit();

			PerformMeleeAttack(Mat, _iObject);
		}

		m_PendingMeleeAttack--;
	}
	
	//Reset linked fire lock
	m_bLockedLinkedFire = false;

	return CRPG_Object_Item::OnProcess(_pRoot, _iObject);
}

bool CRPG_Object_Summon::DrawAmmo(int _Num, const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, bool _bApplyFireTimeout)
{
//	CWObject *pObj = m_pWServer->Object_Get(_iObject);
	// Add shots fired
	m_ShotsFired++;

	if(m_MaxAmmo != -1 && !(m_Flags2 & RPG_ITEM_FLAGS2_NOAMMODRAW))
	{
		if(m_AmmoLoad < 0)
			m_AmmoLoad = Min(-1, m_AmmoLoad + _Num);
		else
			m_AmmoLoad = Max(0, m_AmmoLoad - _Num);
	}

	if(_bApplyFireTimeout)
	{
		if(m_FireTimeout > 0)
			ApplyWait(_iObject, m_FireTimeout, m_NoiseLevel, m_Visibility);

		int Ticks = 0;
		//int Ticks = PlayAnimSequence(_iObject, GetActivateAnim(_iObject), PLAYER_ANIM_ONLYTORSO);
		if(m_FireTimeout == -1 && Ticks > 0)
			ApplyWait(_iObject, Ticks, m_NoiseLevel, m_Visibility);
	}

	if(m_Flags2 & RPG_ITEM_FLAGS2_FORCESHELLEMIT && (m_iShellType >= 0))
	{
		// Force a shell to emit when weapon goes off
		CWO_ShellManager* pShellMgr = (CWO_ShellManager*)m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETSHELLMANAGER), m_pWServer->Game_GetObjectIndex());
		if(pShellMgr)
		{
			CWO_Shell_SpawnData SpawnData;
			GetShellSpawnData(_iObject, &SpawnData);
			pShellMgr->SpawnShell(_iObject, m_iShellType, SpawnData);
		}
	}

	return true;
}

//-------------------------------------------------------------------

bool CRPG_Object_Summon::Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	MAUTOSTRIP(CRPG_Object_Summon_Activate, false);

	//Linked fire activation, if any
	if ((m_LinkedFireItem != 0) && !m_bLockedLinkedFire)
	{
		//Activate linked fire item, if equipped
		int iSlot = m_LinkedFireItem >> 8;
		CRPG_Object_Char *pRPGChar = (CRPG_Object_Char *)_pRoot;
		CRPG_Object_Item *pItem = pRPGChar->GetInventory(iSlot)->GetEquippedItem();
		if(pItem && pItem->m_iItemType == m_LinkedFireItem)
		{
			//Don't allow a weapon activated by linked fire to activate further linked fire weapons
			pItem->OnMessage(_pRoot, CWObject_Message(OBJMSG_SUMMON_LOCKLINKEDFIRE), _iObject);
			pRPGChar->QuickActivateItem(pItem, _Mat, _iObject, _Input);
		}
	}

	if (_Input == 2)
	{
		if (m_MeleeDelay == 0)
		{
			CMat4Dfp32 Mat = GetCurAttachOnItemMatrix(_iObject);
			PerformMeleeAttack(Mat, _iObject);
		}
		else
			if (m_PendingMeleeAttack == -1)
				m_PendingMeleeAttack = m_MeleeDelay;

		if (m_FireTimeout > 0)
		{
			ApplyWait(_iObject, m_FireTimeout, m_NoiseLevel, m_Visibility);
		}

		return true;
	}

/*	if (!CheckAmmo(_pRoot))
	{
		Reload(&_Mat, _pRoot, _iObject);
		return false;
	}
*/
	if (!m_AmmoLoad && !(_Input == 4) && !(m_Flags2 & RPG_ITEM_FLAGS2_DRAINSDARKNESS))
		return false;

	int32 AmmoDraw = (_Input == 4 ? 0 : m_AmmoDraw);

	Summon(_Mat, _pRoot, _iObject, AmmoDraw, true, m_SummonDelayTicks);

	OnActivate(_Mat, _pRoot, _iObject, _Input);

	return true;
}

bool CRPG_Object_Summon::OnActivate(const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, int _Input)
{
	int iSound = m_iSound_Cast;

	CWObject* pObj = m_pWServer->Object_Get(_iObject);
	bool bPlayer = (pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER) != 0;
	int iNotClient = -1;
	if (bPlayer)
	{
		CWObject_Game *pGame = m_pWServer->Game_GetObject();
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pObj);
		CWO_Player* pP = pGame->Player_Get(pCD->m_iPlayer);
		iNotClient = pP->m_iClient;
	}
	// Fire sounds are predicted on the client nowadays
	/*if (bPlayer && m_iSound_Cast_Player != -1)
	{
		// Check for left item
		
		if ((m_Flags2 & RPG_ITEM_FLAGS2_IAMACLONE) && m_iSound_Cast_LeftItem_Player != -1)
			iSound = m_iSound_Cast_LeftItem_Player;
		else
			iSound = m_iSound_Cast_Player;
	}*/

	if (iSound != -1)
	{
		// Local players predicted nowadays
		/*if (bPlayer)
		{
			int RepeatTime = m_FireTimeout + 1;
			if ((m_Flags & RPG_ITEM_FLAGS_SINGLESHOT) || (RepeatTime > 3))
			{
				m_pWServer->Sound_Global(iNotClient, iSound, 1.0f);
			}
			else
			{
				// Send to all client but this one
				CNetMsg Msg(PLAYER_NETMSG_PLAYSOUND_REPEAT);
				Msg.AddInt16(iSound);
				Msg.AddInt16(RepeatTime);
				m_pWServer->NetMsg_SendToObject(Msg, _iObject);
			}
		}
		else*/
		m_pWServer->Sound_AtNotClient(iNotClient,CVec3Dfp32::GetMatrixRow(_Mat, 3), iSound, 0);
	}

	return true;
}

bool CRPG_Object_Summon::Deactivate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	SAVEMASK_CLRBIT(LOADSAVEMASK_SUMMON_GUID);
	//m_SpawnedObjectGUID = 0;

	// Remove pre-created sounds
	CWObject* pObj = m_pWServer->Object_Get(_iObject);
	bool bPlayer = (pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER) != 0;
	if (bPlayer && (m_FireTimeout <= 2) && !(m_Flags & RPG_ITEM_FLAGS_SINGLESHOT))
		m_pWServer->NetMsg_SendToObject(CNetMsg(PLAYER_NETMSG_PLAYSOUND_REPEAT), _iObject);

	return true;
}

//-------------------------------------------------------------------

void CRPG_Object_Summon::SetPendingMeleeAttackTicks(int _Ticks)
{
	m_PendingMeleeAttack = _Ticks;
}

static fp32 GetHeading(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1)
{
	//Get angle in radians
	CVec3Dfp32 RelPos = _p0 - _p1;
	RelPos[2] = 0; 
	RelPos.Normalize();
	fp32 Angle = M_ACos(RelPos[0]);
	if (RelPos[1] > 0) 
		Angle = -Angle;

	//Convert angle to fractions
	Angle /= 2*_PI;

	return Angle;
}

fp32 CRPG_Object_Summon::GetMeleeAttackAngle(const CMat4Dfp32& _AttackPos, const CMat4Dfp32& _VictimPos)
{
	CVec3Dfp32 AttackPos0 = CVec3Dfp32::GetMatrixRow(_AttackPos, 3);
	CVec3Dfp32 AttackPos1 = CVec3Dfp32::GetMatrixRow(_VictimPos, 3);
	fp32 AttackAngle = GetHeading(AttackPos0, AttackPos1);

	CVec3Dfp32 VictimPos0 = CVec3Dfp32::GetMatrixRow(_VictimPos, 3);
	CVec3Dfp32 VictimPos1 = VictimPos0 + (CVec3Dfp32::GetMatrixRow(_VictimPos, 0) * 10.0f);
	fp32 VictimAngle = GetHeading(VictimPos0, VictimPos1);

	return VictimAngle - AttackAngle;
}

void CRPG_Object_Summon::PerformMeleeAttack(const CMat4Dfp32 &_Mat, int _iObject)
{
	const int16 *pSel;
	CVec3Dfp32 Pos = CVec3Dfp32::GetRow(_Mat, 3) + CVec3Dfp32::GetRow(_Mat, 0) * m_MeleeFwdOffset;
	int nSel;
	if ((m_MeleeHalfWidth == 0) && (m_MeleeHalfDepth == 0))
	{
		//Use old hardcoded attack selection
		nSel = FindMeleeAttackSelection(Pos, _iObject, &pSel);
	}
	else
	{
		//Use template defined attack selection
		CMat4Dfp32 Mat = _Mat;
		Pos.SetRow(Mat, 3);
		nSel = FindMeleeAttackSelection(Mat, _iObject, &pSel);
	}

	CVec3Dfp32 Force = 0;
	if(m_MeleeForce > 0)
		Force = CVec3Dfp32::GetRow(_Mat, 0) * m_MeleeForce;

	for(int i = 0; i < nSel; i++)
	{
		int iObj = pSel[i];

		CWObject_CoreData* pObj = m_pWServer->Object_GetCD(iObj);
		if (pObj)
		{
			// Handle glass objects
			if (pObj->IsClass(class_WorldGlassSpawn) || pObj->IsClass(class_Glass_Dynamic))
			{
				CMat4Dfp32 TempMat;
				pObj->GetLocalPositionMatrix().InverseOrthogonal(TempMat);

				CWO_Glass_MsgData MsgData;
				MsgData.m_BoxSize = CVec3Dfp32(m_MeleeHalfWidth, m_MeleeHalfWidth, m_MeleeHalfDepth);
				MsgData.m_CrushType = GLASS_CRUSH_CUBE;
				MsgData.m_ForceDir = Force.Normalize();
				MsgData.m_ForceScale = Force.Length();
				MsgData.m_ForceRange = 1.0f;
				MsgData.m_LocalPosition = Pos * TempMat;
				MsgData.m_iInstance = 0xFFFF;					// Let glass object decide which instance we hit

				CWObject_Message MsgCrush(OBJMSG_GLASS_CRUSH);
				CWO_Glass_MsgData::SetData(MsgCrush, MsgData);
				m_pWServer->Message_SendToObject(MsgCrush, iObj);
			}
			else
			{
				fp32 Angle = M_Fabs(GetMeleeAttackAngle(_Mat, pObj->GetPositionMatrix()));

				int Damage = m_MeleeDamage_Frontal;
				if (Angle < 0.15f)
					Damage = m_MeleeDamage_FromBehind;

				// Send 1 pt blow damage to generic hitloc
				SendDamage(iObj, Pos, _iObject, Damage, DAMAGETYPE_BLOW, -1, &Force);
				SendMsg(iObj, OBJMSG_RPG_SPEAK_OLD, 1);
			}

			bool bPlaySound = false;
			if (m_MeleeSoundTick == 0)
			{
				bPlaySound = true;
			} else {
				int TickDiff = m_pWServer->GetGameTick() - m_MeleeSoundTick;
				if (TickDiff > m_MeleeSoundPeriod)		// Half a second between melee hit sounds.
					bPlaySound = true;
			}

			if (bPlaySound)
			{
				m_MeleeSoundTick = m_pWServer->GetGameTick();
				m_pWServer->Sound_At(Pos, m_iMeleeHitSound, 0);
			}
		}
	}
}

int CRPG_Object_Summon::FindMeleeAttackSelection(const CVec3Dfp32 &_Pos, int _iObject, const int16** _ppRetList)
{
	TSelection<CSelection::LARGE_BUFFER> Selection;
	CWO_PhysicsState PhysState;
	PhysState.AddPhysicsPrim(0, CWO_PhysicsPrim(OBJECT_PRIMTYPE_BOX, 0, CVec3Dfp32(20, 20, 20), 0));
	PhysState.m_ObjectFlags = OBJECT_FLAGS_PROJECTILE;
	PhysState.m_ObjectIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
	PhysState.m_iExclude = _iObject;
	
	m_pWServer->Selection_AddIntersection(Selection, _Pos, PhysState);
	int nSel = m_pWServer->Selection_Get(Selection, _ppRetList);
	return nSel;
}


int CRPG_Object_Summon::FindMeleeAttackSelection(const CMat4Dfp32& _Mat, int _iObject, const int16** _ppRetList)
{
	//Calculate size. Size must encompass the four points defined by pos + fwd*depth/2, pos - fwd*depth/2,
	//pos + side * width/2 and pos - side * width/2, with a lowest possible height.
	CVec3Dfp32 Size;
	{
		CVec3Dfp32 Fwd = CVec3Dfp32::GetRow(_Mat, 0);
		CVec3Dfp32 Side = CVec3Dfp32::GetRow(_Mat, 1);
		CVec3Dfp32 Max = CVec3Dfp32(Fwd * m_MeleeHalfDepth);
		CVec3Dfp32 Min = CVec3Dfp32(Fwd * m_MeleeHalfDepth);

		//Check the remaining positions
		CVec3Dfp32 Temp;
		for (int i = 0; i < 3; i++)
		{
			switch (i)
			{
			case 0:
				Temp = -Fwd * m_MeleeHalfDepth;
				break;
			case 1:
				Temp = Side * m_MeleeHalfWidth;
				break;
			case 2:
				Temp = -Side * m_MeleeHalfWidth;
				break;
			}
			for (int j = 0; j < 3; j++)
			{
				if (Temp[j] > Max[j])
					Max[j] = Temp[j];
				else if (Temp[j] < Min[j])
					Min[j] = Temp[j];
			}
		}
		Size = Max - Min;

		//Pad size if appropriate
		{
			for (int i = 0; i < 2; i++)
				if (Size[i] < 10.0f)
					Size[i] = 10.0f;
			if (Size[2] < m_MeleeMinHeight)
				Size[2] = m_MeleeMinHeight;
		}
	}

	//Selection should be centered on given position, so we must offset used position accordingly
	CVec3Dfp32 Pos = CVec3Dfp32::GetRow(_Mat, 3);
	Pos = Pos - (Size * 0.5);

	//Make selection
	TSelection<CSelection::LARGE_BUFFER> Selection;
	CWO_PhysicsState PhysState;
	PhysState.AddPhysicsPrim(0, CWO_PhysicsPrim(OBJECT_PRIMTYPE_BOX, 0, Size, 0));
	PhysState.m_ObjectFlags = OBJECT_FLAGS_PROJECTILE;
	PhysState.m_ObjectIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
	PhysState.m_iExclude = _iObject;
	m_pWServer->Selection_AddIntersection(Selection, Pos, PhysState);
	int nSel = m_pWServer->Selection_Get(Selection, _ppRetList);

#ifndef M_RTM
	//DEBUG render selection box
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
		m_pWServer->Debug_RenderWire(From, To, Colour, 5.0f);
	}
	//DEBUG
#endif

	return nSel;
}



//-------------------------------------------------------------------

void CRPG_Object_Summon::OnDeltaLoad(CCFile* _pFile, CRPG_Object* _pCharacter)
{
	CRPG_Object_Item::OnDeltaLoad(_pFile, _pCharacter);
	_pFile->ReadLE(m_AmmoLoad);
}

void CRPG_Object_Summon::OnDeltaSave(CCFile* _pFile, CRPG_Object* _pCharacter)
{
	CRPG_Object_Item::OnDeltaSave(_pFile, _pCharacter);
	_pFile->WriteLE(m_AmmoLoad);
}

bool CRPG_Object_Summon::Equip(int _iObject, CRPG_Object *_pRoot, bool _bInstant, bool _bHold)
{
	CRPG_Object_Item::Equip(_iObject, _pRoot, _bInstant, _bHold);
	return true;
}

bool CRPG_Object_Summon::Unequip(int _iObject, CRPG_Object *_pRoot, bool _bInstant, bool _bHold)
{
	// Delete projectiles
	int iProjectiles = m_pWServer->Object_GetIndex(m_SpawnedObjectGUID);
	m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_SUMMON_REMOVEPROJECTILES),iProjectiles);
	return CRPG_Object_Item::Unequip(_iObject, _pRoot, _bInstant, _bHold);
}

bool CRPG_Object_Summon::MergeItem(int _iObject, CRPG_Object_Item *_pObj)
{
	CRPG_Object_Summon* pOther = safe_cast<CRPG_Object_Summon>(_pObj);
	if (!pOther)
		return false;

	if (pOther->m_iItemType != m_iItemType)
		return false;

	if(m_MaxAmmo != 0)
		m_AmmoLoad = MinMT(m_AmmoLoad + pOther->m_AmmoLoad, m_MaxAmmo);
	else
		m_AmmoLoad = m_AmmoLoad + pOther->m_AmmoLoad;

	return true;
}

int CRPG_Object_Summon::OnPickup(int _iObject, CRPG_Object *_pRoot, bool _bNoSound, int _iSender, bool _bNoPickupIcon)
{
	// Give the character ammo for this summon if needed
	// This should really be connected to m_lLinkedItem thingie
	CRPG_Object_Char* pChar = (CRPG_Object_Char*)_pRoot;
	pChar->PickupItem(m_GiveOnPickup, false);

	return CRPG_Object_Item::OnPickup(_iObject, _pRoot, _bNoSound, _iSender,_bNoPickupIcon);
}

int CRPG_Object_Summon::GetAmmo(CRPG_Object* _pRoot, int _iType)
{
	return m_AmmoLoad;
}

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Summon, CRPG_Object_Item);

/*
//-------------------------------------------------------------------
//- CRPG_Object_ReleaseSummon ---------------------------------------
//-------------------------------------------------------------------

const char* CRPG_Object_ThresholdSummon::ms_lpAnimStr[] = { "CHARGE", "THROW", "SHORTTHROW" ,"RELOAD" };

//-------------------------------------------------------------------

void CRPG_Object_ThresholdSummon::PlayAnim(TSANIM _Anim, int _iObject)
{
	MAUTOSTRIP(CRPG_Object_ThresholdSummon_PlayAnim, MAUTOSTRIP_VOID);
	if ((m_CurAnim != TSANIM_NULL) && (m_AnimTime < m_AnimDuration))
	{
		ConOut(CStrF("Interrupting TSAnim%s (%f / %f)", ms_lpAnimStr[m_CurAnim], m_AnimTime, m_AnimDuration));
	}

	m_CurAnim = _Anim;

	int iCurAnim;

	if (m_CurAnim != TSANIM_NULL)
		iCurAnim = m_iAnim[m_CurAnim];
	else
		iCurAnim = 0;
	
	int AnimDurationTicks = PlayAnimSequence(_iObject, iCurAnim, PLAYER_ANIM_ONLYTORSO);

	if (iCurAnim == 0)
		AnimDurationTicks = 0;

	m_AnimTime = 0.0f;
	m_AnimDuration = AnimDurationTicks * SERVER_TIMEPERFRAME;

	if (true)
	{
		if (m_CurAnim != TSANIM_NULL)
		{
			ConOut(CStrF("Playing TSAnim %s (iAnim = %d, Duration = %f)", ms_lpAnimStr[m_CurAnim], iCurAnim, m_AnimDuration));
		}
		else
		{
			ConOut(CStr("Playing TSAnim NULL"));
		}
	}
}

//-------------------------------------------------------------------

void CRPG_Object_ThresholdSummon::OnCreate()
{
	MAUTOSTRIP(CRPG_Object_ThresholdSummon_OnCreate, MAUTOSTRIP_VOID);
	CRPG_Object_ThresholdSummon_Parent::OnCreate();
	
	m_QuickSummonDelayTicks = 0;
	m_LongThrowThreshold = 0.5f;

	m_iState = TSSTATE_NULL;

	m_CurAnim = TSANIM_NULL;
	m_AnimDuration = 0;

	for (int iAnim = 0; iAnim < NUM_TSANIMS; iAnim++)
		m_iAnim[iAnim] = -1;
}

//-------------------------------------------------------------------

void CRPG_Object_ThresholdSummon::OnIncludeClass(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CRPG_Object_ThresholdSummon_OnIncludeClass, MAUTOSTRIP_VOID);
	CRPG_Object_ThresholdSummon_Parent::OnIncludeClass(_pReg, _pMapData, _pWServer);

	IncludeSoundFromKey("SOUND_CHARGING", _pReg, _pMapData);

	for (int iAnim = 0; iAnim < NUM_TSANIMS; iAnim++)
		IncludeAnimFromKey("ANIM_" + CFStr(ms_lpAnimStr[iAnim]), _pReg, _pMapData);
}

//-------------------------------------------------------------------

bool CRPG_Object_ThresholdSummon::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CRPG_Object_ThresholdSummon_OnEvalKey, false);
	for (int iAnim = 0; iAnim < NUM_TSANIMS; iAnim++)
	{
	switch (_KeyHash)
	{
	case MHASH2('ANIM','_') + CFStr(ms_lpAnimStr[iAnim]): // "ANIM_) + CFStr(ms_lpAnimStr[iAnim]"
			{
				m_iAnim[iAnim] = ResolveAnimHandle(_pKey->GetThisValue());
				return true;
			break;
			}
	}

	switch (_KeyHash)
	{
	case MHASH4('QUIC','KSUM','MOND','ELAY'): // "QUICKSUMMONDELAY"
		{
			m_QuickSummonDelayTicks = _pKey->GetThisValuef() * SERVER_TICKSPERSECOND;
			break;
		}

	switch (_KeyHash)
	{
	case MHASH5('LONG','THRO','WTHR','ESHO','LD'): // "LONGTHROWTHRESHOLD"
		{
			m_LongThrowThreshold = _pKey->GetThisValuef();
			break;
		}

	switch (_KeyHash)
	{
	case MHASH3('SOUN','D_CH','ARGE'): // "SOUND_CHARGE"
		{
			m_iSound_Charge = m_pWServer->GetMapData()->GetResourceIndex_Sound(_pKey->GetThisValue());
			break;
		}
	default:
		{
			return CRPG_Object_ThresholdSummon_Parent::OnEvalKey(_pKey);
			break;
		}
	}
	
	return true;
}

//-------------------------------------------------------------------

int CRPG_Object_ThresholdSummon::OnMessage(CRPG_Object* _pParent, const CWObject_Message& _Msg, int _iOwner)
{
	MAUTOSTRIP(CRPG_Object_ThresholdSummon_OnMessage, 0);
	switch (_Msg.m_Msg)
	{
		case OBJMSG_RPG_GETINITPARAMS:
		{
			if (!CRPG_Object_Summon::OnMessage(_pParent, _Msg, _iOwner))
				return 0;

			CRPG_InitParams* pInitParams = (CRPG_InitParams*)_Msg.m_pData;
			if (pInitParams == NULL)
				return 0;

			pInitParams->m_Velocity = Clamp01(m_VelocityFraction);
			pInitParams->m_VelocityType = CRPG_InitParams::VelocityFraction;

			return 1;
		}
	}

	return CRPG_Object_ThresholdSummon_Parent::OnMessage(_pParent, _Msg, _iOwner);
}

//-------------------------------------------------------------------

bool CRPG_Object_ThresholdSummon::Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	MAUTOSTRIP(CRPG_Object_ThresholdSummon_Activate, false);
	if ((m_iState == TSSTATE_NULL) && (m_Flags & RPG_ITEM_FLAGS_EQUIPPED))
	{
		SendMsg(_iObject, OBJMSG_CHAR_STRAIGHTENUP, 1);
		m_iState = TSSTATE_WAITBRANCH;
		m_AnimTime = 0;
		return true;
	}
	return false;
}

//-------------------------------------------------------------------

bool CRPG_Object_ThresholdSummon::Deactivate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	MAUTOSTRIP(CRPG_Object_ThresholdSummon_Deactivate, false);
	if (m_iState == TSSTATE_WAITBRANCH)
	{
		PlayAnim(TSANIM_SHORTTHROW, _iObject);

		m_iState = TSSTATE_SHORTTHROW;
		m_AnimTime = 0;

		m_VelocityFraction = 0;
		Summon(_Mat, _pRoot, _iObject, 1, true, m_QuickSummonDelayTicks);
	}

	return true;
}

//-------------------------------------------------------------------

bool CRPG_Object_ThresholdSummon::OnProcess(CRPG_Object *_pRoot, const CMat4Dfp32 &_Mat, int _iObject)
{
	MAUTOSTRIP(CRPG_Object_ThresholdSummon_OnProcess, false);
	if (m_iState == TSSTATE_WAITBRANCH)
	{
		bool bForceShortThrow = false;
		{
			CWObject *pObj = m_pWServer->Object_Get(_iObject);

			if(pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_CROUCH)
			{
				bForceShortThrow = true;
			}
			else
			{
				CVec3Dfp32 MoveVel = pObj->GetMoveVelocity();
				
				if (MoveVel.LengthSqr() > 0.5f)
				{
					CMat4Dfp32 Mat = pObj->GetPositionMatrix();
					fp32 DotFwd = CVec3Dfp32::GetRow(Mat, 0) * MoveVel;
					fp32 DotSide = CVec3Dfp32::GetRow(Mat, 1) * MoveVel;

					if (M_Fabs(DotSide) > M_Fabs(DotFwd))
						bForceShortThrow = true;
				}
			}
		}

		if (bForceShortThrow)
		{
			PlayAnim(TSANIM_SHORTTHROW, _iObject);

			m_iState = TSSTATE_SHORTTHROW;
			m_AnimTime = 0;

			m_VelocityFraction = 0;
			Summon(_Mat, _pRoot, _iObject, 1, true, m_QuickSummonDelayTicks);
		}
		else if (m_AnimTime > m_LongThrowThreshold)
		{
			PlayAnim(TSANIM_LONGTHROW, _iObject);

			m_iState = TSSTATE_LONGTHROW;
			m_AnimTime = 0;

			m_VelocityFraction = 1.0f;
			Summon(_Mat, _pRoot, _iObject, 1, true, m_SummonDelayTicks);
		}
	}

	if ((m_iState == TSSTATE_NULL) && (!(m_Flags & RPG_ITEM_FLAGS_EQUIPPED)) && IsEquippable(_pRoot))
	{
		if (Reload(&_Mat, _pRoot, _iObject, false))
		{
			PlayAnim(TSANIM_RELOAD, _iObject);

			m_iState = TSSTATE_RELOAD;
			m_AnimTime = 0;
		}
	}

	if ((m_AnimTime > m_AnimDuration) && (m_iState != TSSTATE_WAITBRANCH) && (m_iState != TSSTATE_NULL))
	{
		PlayAnim(TSANIM_NULL, _iObject);

		if (m_iState == TSSTATE_RELOAD)
			m_Flags |= RPG_ITEM_FLAGS_EQUIPPED;

		m_iState = TSSTATE_NULL;
		m_AnimTime = 0;
	}

	m_AnimTime += SERVER_TIMEPERFRAME;

	return CRPG_Object_ThresholdSummon_Parent::OnProcess(_pRoot, _Mat, _iObject);
}

//-------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_ThresholdSummon, CRPG_Object_ThresholdSummon_Parent);

//-------------------------------------------------------------------
//- CRPG_Object_ReleaseSummon ---------------------------------------
//-------------------------------------------------------------------

const char* CRPG_Object_ReleaseSummon::ms_lpAnimStr[] = { "CHARGE", "THROW", "SHORTTHROW" ,"RELOAD" };

//-------------------------------------------------------------------

void CRPG_Object_ReleaseSummon::PlayAnim(RSANIM _Anim, int _iObject)
{
	MAUTOSTRIP(CRPG_Object_ReleaseSummon_PlayAnim, MAUTOSTRIP_VOID);
	if ((m_CurAnim != RSANIM_NULL) && (m_AnimTime < m_AnimDuration))
	{
		ConOut(CStrF("Interrupting RSAnim%s (%f / %f)", ms_lpAnimStr[m_CurAnim], m_AnimTime, m_AnimDuration));
	}

	m_CurAnim = _Anim;

	int iCurAnim;

	if (m_CurAnim != RSANIM_NULL)
//		iCurAnim = m_iAnim[m_CurAnim][GetAnimMode()];
		iCurAnim = m_iAnim[m_CurAnim];
	else
		iCurAnim = 0;
	
	int AnimDurationTicks = PlayAnimSequence(_iObject, iCurAnim, PLAYER_ANIM_ONLYTORSO);

	if (iCurAnim == 0)
		AnimDurationTicks = 0;

	m_AnimTime = 0.0f;
	m_AnimDuration = AnimDurationTicks * SERVER_TIMEPERFRAME;

	if (true)
	{
		if (m_CurAnim != RSANIM_NULL)
		{
			ConOut(CStrF("Playing RSAnim %s (iAnim = %d, Duration = %f)", ms_lpAnimStr[m_CurAnim], iCurAnim, m_AnimDuration));
		}
		else
		{
			ConOut(CStr("Playing RSAnim NULL"));
		}
	}
}

//-------------------------------------------------------------------

void CRPG_Object_ReleaseSummon::OnCreate()
{
	MAUTOSTRIP(CRPG_Object_ReleaseSummon_OnCreate, MAUTOSTRIP_VOID);
	CRPG_Object_ReleaseSummon_Parent::OnCreate();
	
	m_Charge_StartTime = 0;
	m_Charge_HoldTime = 0;
	m_Charge_EndTime = 0;

	m_QuickSummonDelayTicks = 0;
	m_LongThrowThreshold = 0.5f;

	m_iState = RSSTATE_NULL;

	m_CurAnim = RSANIM_NULL;

	for (int iAnim = 0; iAnim < NUM_RSANIMS; iAnim++)
		m_iAnim[iAnim] = -1;
}

//-------------------------------------------------------------------

void CRPG_Object_ReleaseSummon::OnIncludeClass(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CRPG_Object_ReleaseSummon_OnIncludeClass, MAUTOSTRIP_VOID);
	CRPG_Object_ReleaseSummon_Parent::OnIncludeClass(_pReg, _pMapData, _pWServer);

	IncludeSoundFromKey("SOUND_CHARGING", _pReg, _pMapData);

	for (int iAnim = 0; iAnim < NUM_RSANIMS; iAnim++)
		IncludeAnimFromKey("ANIM_" + CFStr(ms_lpAnimStr[iAnim]), _pReg, _pMapData);
}

//-------------------------------------------------------------------

bool CRPG_Object_ReleaseSummon::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CRPG_Object_ReleaseSummon_OnEvalKey, false);
	switch (_KeyHash)
	{
	case MHASH3('CHAR','GE_S','TART'): // "CHARGE_START"
		{
			m_Charge_StartTime = _pKey->GetThisValuef();
			break;
		}
	case MHASH3('CHAR','GE_H','OLD'): // "CHARGE_HOLD"
		{
			m_Charge_HoldTime = _pKey->GetThisValuef();
			break;
		}
	case MHASH3('CHAR','GE_E','ND'): // "CHARGE_END"
		{
			m_Charge_EndTime = _pKey->GetThisValuef();
			break;
		}

	for (int iAnim = 0; iAnim < NUM_RSANIMS; iAnim++)
	{
	switch (_KeyHash)
	{
	case MHASH2('ANIM','_') + CFStr(ms_lpAnimStr[iAnim]): // "ANIM_) + CFStr(ms_lpAnimStr[iAnim]"
			{
				m_iAnim[iAnim] = ResolveAnimHandle(_pKey->GetThisValue());
				return true;
			break;
			}
	}

	switch (_KeyHash)
	{
	case MHASH4('QUIC','KSUM','MOND','ELAY'): // "QUICKSUMMONDELAY"
		{
			m_QuickSummonDelayTicks = _pKey->GetThisValuef() * SERVER_TICKSPERSECOND;
			break;
		}

	switch (_KeyHash)
	{
	case MHASH5('LONG','THRO','WTHR','ESHO','LD'): // "LONGTHROWTHRESHOLD"
		{
			m_LongThrowThreshold = _pKey->GetThisValuef();
			break;
		}

	switch (_KeyHash)
	{
	case MHASH3('SOUN','D_CH','ARGE'): // "SOUND_CHARGE"
		{
			m_iSound_Charge = m_pWServer->GetMapData()->GetResourceIndex_Sound(_pKey->GetThisValue());
			break;
		}
	default:
		{
			return CRPG_Object_ReleaseSummon_Parent::OnEvalKey(_pKey);
			break;
		}
	}
	
	return true;
}

//-------------------------------------------------------------------

int CRPG_Object_ReleaseSummon::OnMessage(CRPG_Object* _pParent, const CWObject_Message& _Msg, int _iOwner)
{
	MAUTOSTRIP(CRPG_Object_ReleaseSummon_OnMessage, 0);
	switch (_Msg.m_Msg)
	{
		case OBJMSG_RPG_GETINITPARAMS:
		{
			if (!CRPG_Object_Summon::OnMessage(_pParent, _Msg, _iOwner))
				return 0;

			CRPG_InitParams* pInitParams = (CRPG_InitParams*)_Msg.m_pData;
			if (pInitParams == NULL)
				return 0;

			pInitParams->m_Velocity = Clamp01((m_Charge_CurTime - m_Charge_StartTime) / (m_Charge_HoldTime - m_Charge_StartTime));
			pInitParams->m_VelocityType = CRPG_InitParams::VelocityFraction;

			return 1;
		}
	}

	return CRPG_Object_ReleaseSummon_Parent::OnMessage(_pParent, _Msg, _iOwner);
}

//-------------------------------------------------------------------

bool CRPG_Object_ReleaseSummon::Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	MAUTOSTRIP(CRPG_Object_ReleaseSummon_Activate, false);
	if ((m_iState == RSSTATE_NULL) && (m_Flags & RPG_ITEM_FLAGS_EQUIPPED))
	{
		SendMsg(_iObject, OBJMSG_CHAR_STRAIGHTENUP);
		m_iState = RSSTATE_WAITBRANCH;
		m_AnimTime = 0;
		return true;
	}
	return false;
}

//-------------------------------------------------------------------

bool CRPG_Object_ReleaseSummon::Deactivate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	MAUTOSTRIP(CRPG_Object_ReleaseSummon_Deactivate, false);
	if (m_iState == RSSTATE_WAITBRANCH)
	{
		m_Charge_CurTime = LERP(m_Charge_StartTime, m_Charge_HoldTime, 0.2f);
		Summon(_Mat, _pRoot, _iObject, 1, true, m_QuickSummonDelayTicks);
		PlayAnim(RSANIM_SHORTTHROW, _iObject);
		m_iState = RSSTATE_SHORTTHROW;
		m_AnimTime = 0;
	}
	else if (m_iState == RSSTATE_CHARGE)
	{
//		if ((m_AnimTime >= m_Charge_StartTime) && (m_AnimTime <= m_Charge_EndTime))
//		{
			//m_Charge_CurTime = m_AnimTime;
			m_Charge_CurTime = LERP(m_Charge_StartTime, m_Charge_HoldTime, 1.0f);
			Summon(_Mat, _pRoot, _iObject, 1, true, m_SummonDelayTicks);
			PlayAnim(RSANIM_LONGTHROW, _iObject);
			m_iState = RSSTATE_LONGTHROW;
			m_AnimTime = 0;
//		}
	}
	else
	{
		m_iState = RSSTATE_NULL;
		m_AnimTime = 0;
	}
	return true;
}

//-------------------------------------------------------------------

bool CRPG_Object_ReleaseSummon::OnProcess(CRPG_Object *_pRoot, const CMat4Dfp32 &_Mat, int _iObject)
{
	MAUTOSTRIP(CRPG_Object_ReleaseSummon_OnProcess, false);
	m_AnimTime += SERVER_TIMEPERFRAME;

	if (m_iState == RSSTATE_WAITBRANCH)
	{
		if (m_AnimTime > m_LongThrowThreshold)
		{
			m_iState = RSSTATE_CHARGE;
			m_AnimTime = 0;
		}
	}

	if ((m_iState == RSSTATE_NULL) && (!(m_Flags & RPG_ITEM_FLAGS_EQUIPPED)) && IsEquippable(_pRoot))
	{
		m_iState = RSSTATE_RELOAD;
		m_AnimTime = 0;

		Reload(&_Mat, _pRoot, _iObject, false);
		PlayAnim(RSANIM_RELOAD, _iObject);
	}

	if ((m_iState == RSSTATE_RELOAD) && (m_AnimTime >= m_AnimDuration))
	{
		m_iState = RSSTATE_NULL;
		m_AnimTime = 0;
		m_Flags |= RPG_ITEM_FLAGS_EQUIPPED;
	}

	return CRPG_Object_ReleaseSummon_Parent::OnProcess(_pRoot, _Mat, _iObject);
}

//-------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_ReleaseSummon, CRPG_Object_ReleaseSummon_Parent);

//-------------------------------------------------------------------
//- CRPG_Object_TargetSummon ----------------------------------------
//-------------------------------------------------------------------

void CRPG_Object_TargetSummon::OnCreate()
{
	MAUTOSTRIP(CRPG_Object_TargetSummon_OnCreate, MAUTOSTRIP_VOID);
	CRPG_Object_Summon::OnCreate();
	
	m_Range = 64;
}

//-------------------------------------------------------------------

bool CRPG_Object_TargetSummon::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CRPG_Object_TargetSummon_OnEvalKey, false);
	switch (_KeyHash)
	{
	case MHASH2('RANG','E'): // "RANGE"
		{
			m_Range = _pKey->GetThisValuei();
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

//-------------------------------------------------------------------

bool CRPG_Object_TargetSummon::Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	MAUTOSTRIP(CRPG_Object_TargetSummon_Activate, false);
	if(!CheckAmmo(_pRoot))
	{
		Reload(&_Mat, _pRoot, _iObject);
		return false;
	}
	
	const CVec3Dfp32 &Pos = CVec3Dfp32::GetMatrixRow(_Mat, 3);
	const CVec3Dfp32 &Fwd = CVec3Dfp32::GetMatrixRow(_Mat, 0);
	
	CCollisionInfo Info;
	if(!TraceRay(_Mat, m_Range, &Info, OBJECT_FLAGS_PHYSMODEL, 0, _iObject))
		return true;
	
	//		ConOut(CStrF("%s, %f", (char *)CVec3Dfp32::GetMatrixRow(_Mat, 0).GetString(), Info.m_Plane.n * CVec3Dfp32(0, 0, 1)));
	if(Info.m_Plane.n * CVec3Dfp32(0, 0, 1) < 0.7f)
		return true;
	
	CMat4Dfp32 Mat;
	(-CVec3Dfp32::GetMatrixRow(_Mat, 0)).SetMatrixRow(Mat, 0);
	CVec3Dfp32(0, 0, 1).SetMatrixRow(Mat, 2);
	Mat.RecreateMatrix(2, 0);
	Info.m_Pos.SetMatrixRow(Mat, 3);
		
//	Summon(Mat, _pRoot, _iObject, &Info);
	
	return true;
}

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_TargetSummon, CRPG_Object_Item);

//-------------------------------------------------------------------
//- CRPG_Object_FloorSummon -----------------------------------------
//-------------------------------------------------------------------

void CRPG_Object_FloorSummon::OnCreate()
{
	MAUTOSTRIP(CRPG_Object_FloorSummon_OnCreate, MAUTOSTRIP_VOID);
	CRPG_Object_TargetSummon::OnCreate();
}

//-------------------------------------------------------------------

bool CRPG_Object_FloorSummon::Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	MAUTOSTRIP(CRPG_Object_FloorSummon_Activate, false);
	if(!CheckAmmo(_pRoot))
	{
		Reload(&_Mat, _pRoot, _iObject);
		return false;
	}
	
	const CVec3Dfp32 &Pos = CVec3Dfp32::GetMatrixRow(_Mat, 3);
	const CVec3Dfp32 &Fwd = CVec3Dfp32::GetMatrixRow(_Mat, 0);
	
	CMat4Dfp32 Mat;
	Mat.UnitNot3x3();
	(-Fwd).SetMatrixRow(Mat, 0);
	CVec3Dfp32(0, 0, 1).SetMatrixRow(Mat, 2);
	Mat.RecreateMatrix(2, 0);
	
	const CVec3Dfp32 &Horiz = CVec3Dfp32::GetMatrixRow(Mat, 0);
	CVec3Dfp32 Dest0 = Pos - Horiz * m_Range;
	
	CCollisionInfo Info;
	if(!TraceRay(Pos, Dest0, &Info, 0, 0, _iObject))
	{
		CVec3Dfp32 Dest1 = Dest0 + CVec3Dfp32(0, 0, -64);
		if(m_pWServer->Phys_IntersectLine(Dest0, Dest1, 0, OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_CHARACTER, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &Info) && Info.m_bIsValid)
			(Info.m_Pos + CVec3Dfp32(0, 0, 0.1f)).SetMatrixRow(Mat, 3);
		else
			Dest1.SetMatrixRow(Mat, 3);
	}
	else
		return true;
	
//	Summon(Mat, _pRoot, _iObject, &Info);
	
	return true;
}

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_FloorSummon, CRPG_Object_Item);

//-------------------------------------------------------------------


//-------------------------------------------------------------------
//- CRPG_Object_MultiSpreadSummon -----------------------------------
//-------------------------------------------------------------------

void CRPG_Object_MultiSpreadSummon::OnCreate()
{
	MAUTOSTRIP(CRPG_Object_MultiSpreadSummon_OnCreate, MAUTOSTRIP_VOID);
	CRPG_Object_Summon::OnCreate();

	//Add ai type flag
	m_HeadingSpread = 0.25f;
	m_PitchSpread = 0.25f;
	m_MinProjectiles = 1;
	m_MaxProjectiles = 1;
}


//-------------------------------------------------------------------

bool CRPG_Object_MultiSpreadSummon::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CRPG_Object_MultiSpreadSummon_OnEvalKey, false);
	switch (_KeyHash)
	{
	case MHASH4('HEAD','INGS','PREA','D'): // "HEADINGSPREAD"
		{
			m_HeadingSpread = _pKey->GetThisValuef()/360.0f;
			break;
		}

	case MHASH3('PITC','HSPR','EAD'): // "PITCHSPREAD"
		{
			m_PitchSpread = _pKey->GetThisValuef()/360.0f;
			break;
		}
	
	case MHASH4('MAXP','ROJE','CTIL','ES'): // "MAXPROJECTILES"
		{
			m_MaxProjectiles = _pKey->GetThisValuei();
			break;
		}

	case MHASH4('MINP','ROJE','CTIL','ES'): // "MINPROJECTILES"
		{
			m_MinProjectiles = _pKey->GetThisValuei();
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

//-------------------------------------------------------------------

int CRPG_Object_MultiSpreadSummon::Summon(const char* _pClass, const CMat4Dfp32& _Mat, CRPG_InitParams* _pInitParams, int _NumDrawAmmo, bool _bApplyWait)
{
	MAUTOSTRIP(CRPG_Object_MultiSpreadSummon_Summon, 0);
	int SpawnNum = m_MinProjectiles + RoundToInt(Random * (fp32)(m_MaxProjectiles - m_MinProjectiles));
	const int nParams = 1;
	int32 pParams[nParams] = { (int32)_pInitParams };
	CVec3Dfp32 Forward = CVec3Dfp32::GetMatrixRow(_Mat, 0);
	CVec3Dfp32 Right = CVec3Dfp32::GetMatrixRow(_Mat, 1);
	CVec3Dfp32 Up = CVec3Dfp32::GetMatrixRow(_Mat, 2);
	CMat4Dfp32 SpawnMat = _Mat;
	CVec3Dfp32 Dir;
	for(int i= 0; i < SpawnNum; i++)
	{
		fp32 RandAngle = (Random * m_HeadingSpread) - m_HeadingSpread/2.0f;
		if(RandAngle < 0)
		{
			Dir = Forward * M_Cos(-RandAngle*_PI*2) + (-Right) * M_Sin(-RandAngle*_PI*2);  
		}
		else
		{
			Dir = Forward * M_Cos(RandAngle*_PI*2) + Right * M_Sin(RandAngle*_PI*2);  
		}

		RandAngle = (Random * m_PitchSpread/2.0f);
		Dir = Dir * M_Cos(RandAngle*_PI*2) + Up * M_Sin(RandAngle*_PI*2);  

		Dir.SetMatrixRow(SpawnMat,0);
		SpawnMat.RecreateMatrix(0,1);
		int iObj = m_pWServer->Object_Create(_pClass, SpawnMat, _pInitParams->m_iCreator, pParams, nParams);
		if(iObj > 0)
		{
			//DrawAmmo(_NumDrawAmmo, _Mat, _pInitParams->m_pRPGCreator, _pInitParams->m_iCreator, _bApplyWait);
			m_AmmoLoad = Max(0, m_AmmoLoad - _NumDrawAmmo);
		}
	}

	return 0;
}


MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_MultiSpreadSummon, CRPG_Object_Summon);
*/

void CRPG_Object_Throw::OnCreate()
{
	m_HideModelDelay = TIMER_INVALID;
	m_ShowModelDelay = TIMER_INVALID;
	m_ActiveTimer = TIMER_INVALID;
	CRPG_Object_Summon::OnCreate();
}

bool CRPG_Object_Throw::Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	CMat4Dfp32 Effect, Res;
	Effect = GetCurAttachOnItemMatrix(_iObject);

	m_ActiveTimer = 1;

	//Should we hide item model immediately?
	if (m_HideModelDelay == 0)
	{
		m_Flags |= RPG_ITEM_FLAGS_NORENDERMODEL;
		SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL, 0, -1);
	}
	if(!CRPG_Object_Summon::Activate(Effect, _pRoot, _iObject, 0))
		return false;
	int iObj = m_pWServer->Object_GetIndex(m_SpawnedObjectGUID);
 	if(iObj > 0)
	{	// _Mat we assume come from the camera
		CVec3Dfp32 Pos = m_pWServer->Object_GetPosition(iObj);
		CVec3Dfp32 Vel = CVec3Dfp32::GetRow(_Mat,0);
		Vel[2] += 0.2f;	// Tilt it up a bit
		Vel.Normalize();
		Vel *= 30.0f;
		// Thrower,thrown obj,pos,force
		SendMsg(iObj,OBJMSG_THROW_RIGID,_iObject,iObj,-1,0,Pos,Vel,NULL,0);

		//Show updated ammo counter
		SendMsg(_iObject, OBJMSG_CHAR_UPDATEMAGAZINE, m_iIconSurface, ShowNumMagazines());
	}
	return true;
}

bool CRPG_Object_Throw::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CRPG_Object_Throw_OnEvalKey, false);

	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	switch (_KeyHash)
	{
	case MHASH4('HIDE','MODE','LDEL','AY'): // "HIDEMODELDELAY"
		{
			int Val = KeyValue.Val_int();
			if (Val >= 0)
				m_HideModelDelay = Min((int)TIMER_MAX, Val);
			else
				m_HideModelDelay = TIMER_INVALID; //Never hide model
			break;
		}

	case MHASH4('SHOW','MODE','LDEL','AY'): // "SHOWMODELDELAY"
		{
			int Val = KeyValue.Val_int();
			if (Val >= 0)
				m_ShowModelDelay = Min((int)TIMER_MAX, Val);
			else
			{
				m_ShowModelDelay = TIMER_INVALID; //Never show model
				m_Flags |= RPG_ITEM_FLAGS_NORENDERMODEL;
			}
			break;
		}
	default:
		{
			return CRPG_Object_Summon::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}

	return true;
};

bool CRPG_Object_Throw::OnProcess(CRPG_Object *_pRoot, int _iObject)
{
	if (m_ActiveTimer < TIMER_MAX)
	{
		//Count up activatetimer until it reaches max or we unequip
		m_ActiveTimer++;

		if (!(m_Flags & RPG_ITEM_FLAGS_NORENDERMODEL) &&
			(m_ActiveTimer > m_HideModelDelay))
		{
			//Time to hide model!
			m_Flags |= RPG_ITEM_FLAGS_NORENDERMODEL;
			SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL, 0, -1);
		}
	}

	//Should we show model?
	//Never show model when out of ammo
	if ((m_Flags & RPG_ITEM_FLAGS_NORENDERMODEL) &&
		(m_ActiveTimer > m_ShowModelDelay) &&
		(m_AmmoLoad != 0))
	{
		//Time to show model!
		m_Flags &= ~RPG_ITEM_FLAGS_NORENDERMODEL;
		SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL, 0, -1,m_AnimGripType);
	}

	return CRPG_Object_Summon::OnProcess(_pRoot, _iObject);
};

bool CRPG_Object_Throw::Unequip(int _iObject, CRPG_Object *_pRoot, bool _bInstant, bool _bHold)
{
	//Inactivate
	m_ActiveTimer = TIMER_INVALID;

	return CRPG_Object_Summon::Unequip(_iObject, _pRoot, _bInstant, _bHold);
};


//Get the number of extra magazines. Return -1 if we don't need/use extra magazines.
int CRPG_Object_Throw::ShowNumMagazines()
{
	return m_AmmoLoad;
};


MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Throw, CRPG_Object_Summon);
