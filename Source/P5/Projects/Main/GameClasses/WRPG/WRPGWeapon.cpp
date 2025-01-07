#include "PCH.h"

#include "WRPGInitParams.h"
#include "WRPGChar.h"
#include "WRPGWeapon.h"
#include "WRPGAmmo.h"
#include "../WObj_Char.h"
#include "../WObj_Game/WObj_GameMod.h"
#include "../WObj_CharMsg.h"

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Weapon, CRPG_Object_Summon);

//-------------------------------------------------------------------
//- CRPG_Object_Weapon ----------------------------------------------
//-------------------------------------------------------------------

void CRPG_Object_Weapon::OnCreate()
{
	MAUTOSTRIP(CRPG_Object_Weapon_OnCreate, MAUTOSTRIP_VOID);
	CRPG_Object_Summon::OnCreate();

	m_Range = 1000;
	m_MuzzleFlameOffTick = -1;
	m_PlayerScatter = 0;
	m_bTriggerPressed = false;
	m_bNoFlashLight = false;
	m_nMagazines = 0;
	m_MaxMagazines = -1;
	m_iSound_Reload = -1;
	m_iSound_OutOfAmmo = -1;
	m_iSound_Merge = -1;
	m_bForceReload = false;
	m_bDefaultRicochet = false;
	m_LastShotTick = 0;
	m_ScatterDecreaseTime = 1;
	m_LastScatter = 0.0f;
	m_ScatterIncreaseAmount = 0.5f;
	m_ScatterFrequencyPenalty = 0.5f;
	m_MuzzleLightColor = 0xff000000;
	m_MuzzleLightRange = 0;
	m_bLightningLight = false;
}

void CRPG_Object_Weapon::OnIncludeClass(const CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CRPG_Object_Weapon_OnIncludeClass, MAUTOSTRIP_VOID);
	CRPG_Object_Summon::OnIncludeClass(_pReg, _pMapData, _pWServer);

	IncludeClassFromKey("DAMAGE_EFFECT", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_RELOAD", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_OUTOFAMMO", _pReg, _pMapData);
	IncludeSoundFromKey("MERGE_SOUND", _pReg, _pMapData);
}

bool CRPG_Object_Weapon::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CRPG_Object_Weapon_OnEvalKey, false);

	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	const fp32 KeyValuef = KeyValue.Val_fp64();
	const int KeyValuei = KeyValue.Val_int();

	switch (_KeyHash)
	{
	case MHASH2('DAMA','GE'): // "DAMAGE"
		{
			m_Damage.Parse(KeyValue, CRPG_Object::ms_DamageTypeStr);
			break;
		}
	case MHASH3('DAMA','GETY','PE'): // "DAMAGETYPE"
		{
			m_Damage.m_DamageType = (uint32)KeyValue.TranslateFlags(ms_DamageTypeStr);
			break;
		}
	case MHASH4('DAMA','GE_E','FFEC','T'): // "DAMAGE_EFFECT"
		{
			m_DamageEffect = KeyValue;
			break;
		}
	case MHASH2('RANG','E'): // "RANGE"
		{
			m_Range = KeyValuef;
			break;
		}
	case MHASH4('PLAY','ERSC','ATTE','R'): // "PLAYERSCATTER"
		{
			m_PlayerScatter = KeyValuef;
			break;
		}
	case MHASH5('SCAT','TERD','ECRE','ASET','IME'): // "SCATTERDECREASETIME"
		{
			m_ScatterDecreaseTime = KeyValuei;
			break;
		}
	case MHASH6('SCAT','TERI','NCRE','ASEA','MOUN','T'): // "SCATTERINCREASEAMOUNT"
		{
			m_ScatterIncreaseAmount = KeyValuef;
			break;
		}
	case MHASH6('SCAT','TERF','REQU','ENCY','PENA','LTY'): // "SCATTERFREQUENCYPENALTY"
		{
			m_ScatterFrequencyPenalty = KeyValuef;
			break;
		}
	case MHASH3('SOUN','D_RE','LOAD'): // "SOUND_RELOAD"
		{
			m_iSound_Reload = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH4('SOUN','D_OU','TOFA','MMO'): // "SOUND_OUTOFAMMO"
		{
			m_iSound_OutOfAmmo = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH3('MERG','E_SO','UND'): // "MERGE_SOUND"
		{
			m_iSound_Merge = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH5('WEAP','ON_M','AXMA','GAZI','NES'): // "WEAPON_MAXMAGAZINES"
		{
			m_MaxMagazines = KeyValuei;
			break;
		}
	case MHASH3('NOFL','ASHL','IGHT'): // "NOFLASHLIGHT"
		{
			m_bNoFlashLight = true;
			break;
		}
	case MHASH5('MUZZ','LE_L','IGHT','_COL','OR'): // "MUZZLE_LIGHT_COLOR"
		{
			CPixel32 MuzzleColor;
			MuzzleColor.Parse(KeyValue);
			m_MuzzleLightColor = MuzzleColor;
			break;
		}
	case MHASH5('MUZZ','LE_L','IGHT','_RAN','GE'): // "MUZZLE_LIGHT_RANGE"
		{
			m_MuzzleLightRange = KeyValuei;
			break;
		}
	case MHASH3('NO_A','MMO_','DRAW'): // "NO_AMMO_DRAW"
		{
			if (KeyValuei)
			{
				m_Flags2 |= RPG_ITEM_FLAGS2_NOAMMODRAW;
			}
			else
			{
				m_Flags2 &= ~RPG_ITEM_FLAGS2_NOAMMODRAW;
			}
			break;
		}
	case MHASH4('LIGH', 'TNIN', 'G_LI', 'GHT'): // "LIGHTNING_LIGHT"
		{
			m_bLightningLight = KeyValuei ? true : false;
			break;
		}
	default:
		{
			return CRPG_Object_Summon::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
	
	return true;
}

void CRPG_Object_Weapon::OnFinishEvalKeys()
{
	CRPG_Object_Summon::OnFinishEvalKeys();

	// Convert extra ammo to magazines
	if(m_MaxAmmo == -1)
		m_AmmoLoad = 1;
	else if(m_AmmoLoad == -1)
		m_AmmoLoad = -(m_MaxAmmo + 1);
	else
	{
		m_nMagazines = m_MaxAmmo > 0 ? TruncToInt(fp32(m_AmmoLoad) / m_MaxAmmo) : 0;
		m_AmmoLoad -= m_nMagazines * m_MaxAmmo;
		
		// Check so that we don't have over the limit in magazines
		if (m_MaxMagazines >= 0 && (m_nMagazines > m_MaxMagazines))
			m_nMagazines = m_MaxMagazines;

		if(m_nMagazines > 0 && m_AmmoLoad == 0)
		{
			m_AmmoLoad = m_MaxAmmo;
			m_nMagazines--;
		}
	}
}

CWO_Damage * CRPG_Object_Weapon::GetDamage()
{
	return &m_Damage;
}

void CRPG_Object_Weapon::GetMuzzleProperties(uint32& _MuzzleLightColor, uint32& _MuzzleLightRange)
{
	_MuzzleLightColor = m_MuzzleLightColor;
	_MuzzleLightRange = m_MuzzleLightRange;
}

bool CRPG_Object_Weapon::HasLightningLight()
{
	return m_bLightningLight;
}

void CRPG_Object_Weapon::FireMuzzle(int _iObject)
{
//	if(m_MuzzleFlameOffTick == -1)
	if (!(m_Flags2 & RPG_ITEM_FLAGS2_NOMUZZLE))
	{
		{
			fp32 Target = 0;
			fp32 Speed = 0;
			CWObject_Message Msg(OBJMSG_CHAR_SETITEMANIMTARGET, *(int *)&Target, *(int *)&Speed);
			Msg.m_Reason = m_iItemType >> 8;
			m_pWServer->Message_SendToObject(Msg, _iObject);
		}
		m_MuzzleFlameOffTick = GetGameTick(_iObject) + 10;
		fp32 Target = 10000000;
		fp32 Speed = 1.0f;
		CWObject_Message Msg(OBJMSG_CHAR_SETITEMANIMTARGET, *(int *)&Target, *(int *)&Speed);
		Msg.m_Reason = m_iItemType >> 8;
		m_pWServer->Message_SendToObject(Msg, _iObject);
	}
//	else
//		m_MuzzleFlameOffTick = GetGameTick(_iObject) + 3;

}

void CRPG_Object_Weapon::RefreshMuzzle(int _iObject)
{
	if(m_MuzzleFlameOffTick != -1)
	{
		int Tick = GetGameTick(_iObject);
		if(Tick >= m_MuzzleFlameOffTick)
		{
			fp32 Target = 0;
			fp32 Speed = 0;
			CWObject_Message Msg(OBJMSG_CHAR_SETITEMANIMTARGET, *(int *)&Target, *(int *)&Speed);
			Msg.m_Reason = m_iItemType >> 8;
			m_pWServer->Message_SendToObject(Msg, _iObject);
			m_MuzzleFlameOffTick = -1;
		}
	}
}

bool CRPG_Object_Weapon::NeedReload(int _iObject, bool _bCanReload)
{
	if (m_nMagazines == 0)
	{
		CWObject* pObj = m_pWServer->Object_Get(_iObject);
		if ((pObj)&&(!(pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER)))
		{	// Bots get infinite ammo
			m_nMagazines++;
		}
	}

	if (_bCanReload)
		return (m_nMagazines > 0 && m_AmmoLoad < m_MaxAmmo) || m_AmmoLoad == -1 || m_bForceReload;
	else
		return (m_nMagazines > 0 && m_AmmoLoad == 0) || m_AmmoLoad == -1 || m_bForceReload;
}

void CRPG_Object_Weapon::Reload(int _iObject)
{
	// Reload parent
	CRPG_Object_Summon::Reload(_iObject);
	if (NeedReload(_iObject,true))
	{
		CWObject *pObj = m_pWServer->Object_Get(_iObject);
		m_bForceReload = false;

		if(m_AmmoLoad == -1)
			m_AmmoLoad = -(m_MaxAmmo + 1);
		else
		{
			if(m_nMagazines > 0)
			{
				m_AmmoLoad = m_MaxAmmo;
				m_nMagazines--;
				if(pObj && pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER)
				{
					SendMsg(_iObject, OBJMSG_CHAR_UPDATEMAGAZINE, m_iIconSurface, ShowNumMagazines());
				}
			}
		}
		
		if(pObj && m_iSound_Reload != -1)
			m_pWServer->Sound_On(_iObject, m_iSound_Reload, 0);
		
		ApplyWait(_iObject, 36, 0, 0);
	}
}

void CRPG_Object_Weapon::ForceReload(int _iObject)
{
	if(m_AmmoLoad != m_MaxAmmo && m_nMagazines > 0)
		m_bForceReload = true;
	else
		SendMsg(_iObject, OBJMSG_CHAR_UPDATEMAGAZINE, m_iIconSurface, ShowNumMagazines());
}

bool CRPG_Object_Weapon::Equip(int _iObject, CRPG_Object *_pRoot, bool _bInstant, bool _bHold)
{
	//if(!_bInstant)
	{
		CWObject_Character* pChar = (CWObject_Character*)m_pWServer->Object_Get(_iObject);
		if(pChar && pChar->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER)
			SendMsg(_iObject, OBJMSG_CHAR_UPDATEMAGAZINE, m_iIconSurface, ShowNumMagazines());
	}

//	SendMsg(_iObject, OBJMSG_CHAR_SETMUZZLELIGHTCOLOR, m_MuzzleLightColor, m_MuzzleLightRange);

	return CRPG_Object_Summon::Equip(_iObject, _pRoot, _bInstant, _bHold);
}

int CRPG_Object_Weapon::GetAmmo(CRPG_Object* _pRoot, int _iType)
{
	if(_iType == 1)
		return m_nMagazines;
	else
		return m_AmmoLoad;
}

int CRPG_Object_Weapon::GetMagazines(void)
{
	return m_nMagazines;
}

void CRPG_Object_Weapon::SetAmmo(int _Num)
{
	m_AmmoLoad = _Num;
}

void CRPG_Object_Weapon::AddMagazine(int _Num)
{
	if(m_nMagazines + _Num <= m_MaxMagazines || m_MaxMagazines == -1)
		m_nMagazines += _Num;
}

bool CRPG_Object_Weapon::CanPickup()
{
	// Check so that we can pickup 
	if ((m_MaxMagazines < 0) || (m_nMagazines < m_MaxMagazines))
		return true;

	return false;
}

bool CRPG_Object_Weapon::MergeItem(int _iObject, CRPG_Object_Item *_pObj)
{
	CRPG_Object_Summon* pOther = TDynamicCast<CRPG_Object_Summon>(_pObj);  // safe_cast crashed because of a melee weapon ending up in here..
	if (!pOther)
		return false;

	if (pOther->m_iItemType != m_iItemType)
		return false;

	CRPG_Object_Weapon *pWeapon = safe_cast<CRPG_Object_Weapon>(_pObj);
	if (!pWeapon)
		return false;

	if(m_AmmoLoad != -1 && m_MaxAmmo != -1)
	{
		int nMagazines = pWeapon->m_nMagazines;
		if(pWeapon->m_AmmoLoad == pWeapon->m_MaxAmmo)
			nMagazines++;
		m_nMagazines += nMagazines;
		
		if ((m_MaxMagazines >= 0) && (m_nMagazines > m_MaxMagazines))
			m_nMagazines = m_MaxMagazines;

		SendMsg(_iObject, OBJMSG_CHAR_UPDATEMAGAZINE, m_iIconSurface, ShowNumMagazines());
	}

/*	if(m_MaxAmmo != 0)
		m_AmmoLoad = MinMT(m_AmmoLoad + pOther->m_AmmoLoad, m_MaxAmmo);
	else
		m_AmmoLoad = m_AmmoLoad + pOther->m_AmmoLoad;*/

	CWObject *pObj = m_pWServer->Object_Get(_iObject);
	if(pObj && m_iSound_Merge != -1)
		m_pWServer->Sound_At(pObj->GetPosition(), m_iSound_Merge, 0);

	return true;
}

#define PULL_MINANGLE 0.97f

int CRPG_Object_Weapon::Summon(const char* _pClass, int _iObject, const CMat4Dfp32& _Mat, CRPG_InitParams* _pInitParams, int _NumDrawAmmo, bool _bApplyWait, bool _bFinalPos)
{
	CWObject_CoreData* pObj = m_pWServer->Object_Get(_iObject);
	CWObject_Character* pChar = pObj ? pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER ? (CWObject_Character*)pObj : NULL : NULL;

 	CMat4Dfp32 Effect, Res;
 	Effect = GetCurAttachOnItemMatrix(_iObject);
	if(_bFinalPos)
		Res = _Mat;
	else if((m_Flags & RPG_ITEM_FLAGS_WEAPONTARGETING) && pChar && pChar->IsPlayer())
	{	// We are aiming along weapon but we should introduce scatter anyway
		Res = Effect;
	}
	else
	{
		Res = _Mat;
		Res.k[3][0] = Effect.k[3][0];
		Res.k[3][1] = Effect.k[3][1];
		Res.k[3][2] = Effect.k[3][2];
	}


	CWO_Character_ClientData* pCD = (pChar ? CWObject_Character::GetClientData(pChar) : NULL);
	if(pChar && pCD && pChar->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER)
	{	
		// We don't scatter the first round in a second
		//int Tick = m_pWServer->GetGameTick();
		int Tick = pCD->m_GameTick;
		{
			// Before each shot, calculate how much the scatter has decreased over time
			fp32 ScatterDecreasePerTick = ((fp32)m_ScatterDecreaseTime) * m_pWServer->GetGameTickTime();
//			fp32 ScatterBefore = m_LastScatter;
			// If we have shot recently, decrease the time amount
			fp32 Scale = Clamp01(m_ScatterFrequencyPenalty + Sqr(((fp32)(Tick - m_LastShotTick)) * m_pWServer->GetGameTickTime() / m_ScatterDecreaseTime));
			fp32 Time = (fp32)(Tick - m_LastShotTick) * Scale;
			m_LastScatter = Clamp01(m_LastScatter - ScatterDecreasePerTick * Time);
			m_LastShotTick = Tick;

			fp32 ScatterX = m_LastScatter * m_PlayerScatter * (Random - 0.5f) * (Random - 0.5f);
			fp32 ScatterY = m_LastScatter * m_PlayerScatter * (Random - 0.5f) * (Random - 0.5f);

			if ((ScatterX > 0.015f || ScatterY > 0.015f) && _pInitParams)
			{
				_pInitParams->m_DamageDeliveryFlags |= DAMAGEFLAG_NO_CRIT;
				/* *** NOTE: Should be handled through effects system instead
				if(m_bDefaultRicochet)
					_pInitParams->m_DamageDeliveryFlags |= DAMAGEFLAG_DEFAULTRICOCHET;
				*/
			}

			// Calculate scatter for current shot
			// ..
			CVec3Dfp32::GetRow(Res, 0) += CVec3Dfp32::GetRow(Res, 1) * ScatterX;
			CVec3Dfp32::GetRow(Res, 0) += CVec3Dfp32::GetRow(Res, 2) * ScatterY;
			Res.RecreateMatrix(0, 1);

			// 
			/*{
				ConOut(CStrF("Tick: %d ScatterBefore: %f LastScatter: %f ScatterRes: %f PlayerScatter: %f Scale: %f", Tick, ScatterBefore, m_LastScatter, m_LastScatter * m_PlayerScatter,m_PlayerScatter, Scale));
				CVec3Dfp32 DebugPos = CVec3Dfp32::GetRow(Res,3);
				m_pWServer->Debug_RenderWire(DebugPos, DebugPos + CVec3Dfp32::GetRow(Res,0) * 100.0f,0xff7f7f7f,10.0f);
			}*/

			// After each shot, increase scatter
			m_LastScatter = Clamp01(m_LastScatter + m_ScatterIncreaseAmount);

			// moved to CPostAnimSystem
			/*
			int iTarget = pChar->OnMessage(CWObject_Message(OBJMSG_CHAR_HASAUTOAIMTARGET));
			if(iTarget)
			{
				CWObject* pTarget = m_pWServer->Object_Get(iTarget);
				if(pTarget)
				{
					CVec3Dfp32 TarPos =  pTarget->GetPosition();
					{
						aint Ret = pTarget->OnMessage(CWObject_Message(OBJMSG_CHAR_GETAUTOAIMOFFSET));
						TarPos += CVec3Dfp32().Unpack32(Ret, 256.0f);
					}
					CVec3Dfp32 OldFwd = Res.GetRow(0);
					CVec3Dfp32 StartPos = Res.GetRow(3);
					CCollisionInfo CInfo;
					CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;

					CWO_OnIntersectLineContext Ctx;
					Ctx.m_pObj = pTarget;
					Ctx.m_pPhysState = m_pWServer;
					Ctx.m_p0 = StartPos;
					Ctx.m_p1 = StartPos + OldFwd*1000.0f;
					Ctx.m_ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
					Ctx.m_ObjectIntersectionFlags = 0;
					Ctx.m_MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;

					if(!pTarget->OnIntersectLine(Ctx, &CInfo)) // Do NOT fuck with the direction if the bullet already hits the target
					{
						CVec3Dfp32 NewDir = TarPos - StartPos;
						NewDir.Normalize();
						fp32 DotProd = OldFwd * NewDir; 
						if(DotProd > PULL_MINANGLE)
						{
							CQuatfp32 QOldDir;
							QOldDir.Create(Res);

							NewDir.SetMatrixRow(Res, 0);
							Res.RecreateMatrix(0,2);
							CQuatfp32 QNewDir;
							QNewDir.Create(Res);

							CQuatfp32::Lerp(QOldDir, QNewDir, QNewDir, Min(1.2f*(DotProd - PULL_MINANGLE)/(1.0f - PULL_MINANGLE),1.0f));
							QNewDir.CreateMatrix(Res);
							StartPos.SetMatrixRow(Res,3);		
						}
					}
				}
			}
			*/
		}

		if(m_nRumble_Activate > 0)
		{
			int iRumble = MRTC_RAND() % m_nRumble_Activate;
			CWObject_Message Msg(OBJMSG_CHAR_RUMBLE);
			Msg.m_pData = (void *)m_lRumble_Activate[iRumble].Str();
			m_pWServer->Message_SendToObject(Msg, _iObject);
		}
	}
	else if (pChar)
	{	// Scatter (Again! We already do thius in get activate pos but the damned minigun overrides this!)
		if ((m_Flags & RPG_ITEM_FLAGS_WEAPONTARGETING) && (pChar->m_spAI) && (pChar->m_spAI->m_pAIResources->ms_Gamestyle != CWObject_GameP4::GAMESTYLE_KILL))
		{
			CVec3Dfp32::GetRow(Res, 0) += CVec3Dfp32::GetRow(Res, 1) * (0.1f * (Random - 0.5f) * (Random - 0.5f));
			CVec3Dfp32::GetRow(Res, 0) += CVec3Dfp32::GetRow(Res, 2) * (0.1f * (Random - 0.5f) * (Random - 0.5f));
			Res.RecreateMatrix(0, 1);
		}

		// How far from actual activate pos 
		CWO_Character_ClientData* pCD = pChar->GetClientData(pChar);
		if (pCD)
		{
			pChar->m_ActivatePosition = CVec3Dfp32::GetRow(Effect,3);
			pChar->m_ActivatePositionTick = pCD->m_GameTick;
		}
	}

	return CRPG_Object_Summon::Summon(_pClass, _iObject, Res, _pInitParams, _NumDrawAmmo, _bApplyWait);
}

bool CRPG_Object_Weapon::DrawAmmo(int _Num, const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, bool _bApplyFireTimeout)
{
	return CRPG_Object_Summon::DrawAmmo(_Num, _Mat, _pRoot, _iObject, _bApplyFireTimeout);
}

bool CRPG_Object_Weapon::Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	if (_Input == 3 || _Input == 5)
	{
		// Fake activation, do muzzle effect and sound
		if ( _Input == 3)
		{
			// don't do sound if input is 5
			int32 iSound = m_iSound_Cast;
			if(iSound != -1)
			{
				CWObject_CoreData* pObj = m_pWServer->Object_GetCD(_iObject);
				bool bPlayer = (pObj && pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER);
				if (bPlayer)
				{
					iSound = m_iSound_Cast_Player != -1 ? m_iSound_Cast_Player : iSound;
					m_pWServer->Sound_Global(iSound, 1.0f);
				}
				else
					m_pWServer->Sound_At(CVec3Dfp32::GetMatrixRow(_Mat, 3), m_iSound_Cast, 0);
			}
		}
		FireMuzzle(_iObject);
		// Send projectile with zero damage...?
		return true;
	}

	//Check if out of ammo (don't check for ammo when butting :))
	if((_Input == 1) || (m_AmmoLoad == 0 && !(m_Flags2 & RPG_ITEM_FLAGS2_DRAINSDARKNESS)) && (_Input != 2) && (_Input != 4))
	{
		if(m_iExtraActivationWait < GetGameTick(_iObject) && (!m_bTriggerPressed || (m_Flags2 & RPG_ITEM_FLAGS2_AUTOFJUFF)))
		{		
			m_bTriggerPressed = true;
			if(_iObject > 0 && m_iSound_OutOfAmmo != -1)
			{
				CWObject *pObj = m_pWServer->Object_Get(_iObject);
				if(pObj)
					m_pWServer->Sound_At(pObj->GetPosition(), m_iSound_OutOfAmmo, 0);
				if(m_FireTimeout > 0)
					ApplyWait(_iObject, m_FireTimeout, m_NoiseLevel, m_Visibility);
			}
		}
		return false;
	}

	if(m_Flags & RPG_ITEM_FLAGS_SINGLESHOT)
	{
		CWObject *pObj = m_pWServer->Object_Get(_iObject);
		if(pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER)
		{
			if(!m_bTriggerPressed)
			{
				//Don't set trigger pressed when butting with weapon. Should perhaps only set trigger pressed when using primary fire?
				if (_Input != 2)
					m_bTriggerPressed = true;
				return CRPG_Object_Summon::Activate(_Mat, _pRoot, _iObject, _Input);
			}
			return false;
		}
		else
			return CRPG_Object_Summon::Activate(_Mat, _pRoot, _iObject, _Input);
	}
	else
		return CRPG_Object_Summon::Activate(_Mat, _pRoot, _iObject, _Input);
}

bool CRPG_Object_Weapon::Deactivate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	if(m_Flags & RPG_ITEM_FLAGS_SINGLESHOT)
	{
		CWObject *pObj = m_pWServer->Object_Get(_iObject);
		if(pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER)
		{
			if(m_bTriggerPressed)
			{
				m_bTriggerPressed = false;
				return CRPG_Object_Summon::Deactivate(_Mat, _pRoot, _iObject, _Input);
			}
			return false;
		}
		else
			return CRPG_Object_Summon::Deactivate(_Mat, _pRoot, _iObject, _Input);
	}
	else
	{
		if(m_bTriggerPressed)
			m_bTriggerPressed = false;
		return CRPG_Object_Summon::Deactivate(_Mat, _pRoot, _iObject, _Input);
	}
}

bool CRPG_Object_Weapon::IsActivatable(CRPG_Object* _pRoot,int _iObject,int _Input)
{
	if(!(m_Flags & RPG_ITEM_FLAGS_SINGLESHOT) || !m_bTriggerPressed)
		return CRPG_Object_Summon::IsActivatable(_pRoot,_iObject,_Input);

	return false;
}

void CRPG_Object_Weapon::OnDeltaLoad(CCFile* _pFile, CRPG_Object* _pCharacter)
{
	CRPG_Object_Summon::OnDeltaLoad(_pFile, _pCharacter);

	int8 nMagazines;
	_pFile->ReadLE(nMagazines);
	m_nMagazines = nMagazines;
}

void CRPG_Object_Weapon::OnDeltaSave(CCFile* _pFile, CRPG_Object* _pCharacter)
{
	CRPG_Object_Summon::OnDeltaSave(_pFile, _pCharacter);

	int8 nMagazines = m_nMagazines;
	_pFile->WriteLE(nMagazines);
}

//Get the number of extra magazines. Return -1 if we don't need/use extra magazines.
int CRPG_Object_Weapon::ShowNumMagazines()
{
 	if (m_AmmoLoad < 0)
		return -1;
	else
		return m_nMagazines;
};


bool CRPG_Object_Weapon::OnProcess(CRPG_Object* _pRoot, int _iObject)
{
	//SendMsg(_iObject, OBJMSG_CHAR_SETMUZZLELIGHTCOLOR, m_MuzzleLightColor, m_MuzzleLightRange);
	return CRPG_Object_Summon::OnProcess(_pRoot, _iObject);
}


void CRPG_Object_DualSupport::LoadDualSupport(CCFile* _pFile, CRPG_Object* _pCharacter)
{
	_pFile->ReadLE(m_PrevCloneIdentifier);
}

void CRPG_Object_DualSupport::SaveDualSupport(CCFile* _pFile, CRPG_Object* _pCharacter)
{
	_pFile->WriteLE(m_PrevCloneIdentifier);
}

void CRPG_Object_DualSupport::EquipDualSupport(int _iObject, CRPG_Object *_pRoot, bool _bInstant, CRPG_Object_Weapon* _pHost)
{
	// Add a copy if this item to armor inventory
	CRPG_Object_Char* pChar = (CRPG_Object_Char*)_pRoot;
	CRPG_Object_Inventory* pInv = pChar ? pChar->GetInventory(RPG_CHAR_INVENTORY_ARMOUR) : NULL;
	CRPG_Object_Inventory* pWeapInv = pChar ? pChar->GetInventory(RPG_CHAR_INVENTORY_WEAPONS) : NULL;
	// Check if dual wield is supported on target
	if (!(_pHost->SendMsg(_iObject,OBJMSG_CHAR_GETFLAGS) & PLAYER_FLAGS_DUALWIELDSUPPORTED))
		return;

	if (pInv)
	{
		TPtr<CRPG_Object_Summon> spItem = NULL;
		bool bAddtoArmor = true;
		// Try to find a similar item in the weapon inventory and use that one
		// doesn't work too good if we have no ammo left and equipping 
		// (have prev identifier?)
		if (m_PrevCloneIdentifier != -1)
		{
			spItem = (CRPG_Object_Summon*)pInv->FindItemByIdentifier(m_PrevCloneIdentifier);
			if (spItem && spItem->m_Identifier != _pHost->m_Identifier)
			{
				bAddtoArmor = false;
			}
			else
			{
				// Check if it's moved back to weapon inventory
				spItem = (CRPG_Object_Summon*)pWeapInv->FindItemByIdentifier(m_PrevCloneIdentifier);
				if (spItem && spItem->m_Identifier != _pHost->m_Identifier)
				{
					pWeapInv->RemoveItemByIdentifier(_iObject, spItem->m_Identifier, true);
					spItem->m_Flags &= ~RPG_ITEM_FLAGS_REMOVED;
				}
			}
			if (spItem && spItem->m_Identifier == _pHost->m_Identifier)
				spItem = NULL;
			/*if (spItem && spItem->m_AmmoLoad != _pHost->m_AmmoLoad)
			{
				pInv->RemoveItemByIdentifier(_iObject, spItem->m_Identifier,true);
				spItem = NULL;
			}
			else */
		}
		if (!spItem)
		{
			// Try to find one in weapons inventory
			bool bRemove = false;
			TPtr<CRPG_Object_Summon> spTemp = (CRPG_Object_Summon*)pInv->FindNextItemNotEquippedByType(_pHost->m_Identifier,0x200|_pHost->m_iItemType,_pHost);
			if (!spTemp)
			{
				spTemp = (CRPG_Object_Summon*)pWeapInv->FindNextItemNotEquippedByType(_pHost->m_Identifier,_pHost->m_iItemType,_pHost);
				bRemove = true;
			}
			if (spTemp && (spTemp->m_Flags2 & RPG_ITEM_FLAGS2_CLONEATTACHITEM) /*&& 
				(spTemp->m_AmmoLoad == _pHost->m_AmmoLoad)*/)
			{
				if (bRemove)
					pWeapInv->RemoveItemByIdentifier(_iObject, spTemp->m_Identifier, true);
				spItem = spTemp;
				spItem->m_Flags &= ~RPG_ITEM_FLAGS_REMOVED;
			}
		}
		// Create a clone if none of the other alternatives worked
		if (!spItem)
		{
			// If no similar item was found, create a clone of current one....
			spCRPG_Object spNewObj = pChar->CreateObject(_pHost->m_Name);
			spItem = (CRPG_Object_Summon *)((CRPG_Object *)spNewObj);
			spItem->m_AmmoLoad = _pHost->m_AmmoLoad;
		}

		if (spItem)
		{
			// Get this from template later perhaps....
			//spItem->m_AmmoLoad = _pHost->m_AmmoLoad;
			spItem->m_Model.m_iAttachRotTrack = 23;
			spItem->m_Model.m_iAttachPoint[0] = 1;
			//spItem->m_FireTimeout = 0;
			spItem->m_iItemType |= 0x200;
			spItem->m_Flags2 |= RPG_ITEM_FLAGS2_IAMACLONE;
			// Set timeout to zero (fire this gun first and then the other? wait lies in root...)
			if (bAddtoArmor)
				pInv->AddItem(_iObject, spItem);

			if(_pHost->m_iItemType == 1)
				pWeapInv->ReloadItemFromOthers(_pHost);

			pInv->ForceSelectItem(_pHost->m_iItemType|0x200, pChar, _bInstant);
			pInv->ForceSetEquipped(_iObject);
			m_PrevCloneIdentifier = spItem->m_Identifier;
		}
	}
}

void CRPG_Object_DualSupport::UnequipDualSupport(int _iObject, CRPG_Object *_pRoot, bool _bInstant, CRPG_Object_Weapon* _pHost)
{
	CRPG_Object_Char* pChar = (CRPG_Object_Char*)_pRoot;
	CRPG_Object_Inventory* pInv = pChar ? pChar->GetInventory(RPG_CHAR_INVENTORY_ARMOUR) : NULL;
	CRPG_Object_Inventory* pWeapInv = pChar ? pChar->GetInventory(RPG_CHAR_INVENTORY_WEAPONS) : NULL;
	if (pInv)
	{
		// Delete any items of the same type in the armor inventory
		//pInv->ForceSetUnequipped(_iObject);
		TPtr<CRPG_Object_Summon> spItem = (CRPG_Object_Summon*)pInv->FindItemByType(_pHost->m_iItemType | 0x200);
		if (spItem && (spItem->m_Flags2 & RPG_ITEM_FLAGS2_CLONEATTACHITEM))
		{
			int32 GameTick = spItem->GetGameTick(_iObject);
			spItem->SendMsg(_iObject,OBJMSG_CHAR_SETWEAPONAG2, 0, spItem->m_iItemType >> 8 > 0 ? 1: 0, GameTick);
			spItem->SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL,  spItem->m_iItemType >> 8, GameTick);
			pInv->RemoveItemByIdentifier(_iObject, spItem->m_Identifier, true);
			spItem->m_Flags &= ~(RPG_ITEM_FLAGS_REMOVED | RPG_ITEM_FLAGS_EQUIPPED);
			spItem->m_Model.m_iAttachRotTrack = _pHost->m_Model.m_iAttachRotTrack;
			spItem->m_Model.m_iAttachPoint[0] = _pHost->m_Model.m_iAttachPoint[0];
			if (spItem->m_Flags2 & RPG_ITEM_FLAGS2_PERMANENT)
			{
				// "Magically" Fill up with ammo
				spItem->SetAmmo(spItem->GetMaxAmmo());
			}
			//spItem->m_FireTimeout = _pHost->m_FireTimeout;
			spItem->m_iItemType &= ~0x200;
			spItem->m_Flags2 &= ~RPG_ITEM_FLAGS2_IAMACLONE;
			// Reset identifier
			spItem->m_Identifier = 0;
			// Add back to weapons inventory
			pWeapInv->AddItem(_iObject, spItem);
			m_PrevCloneIdentifier = spItem->m_Identifier;
		}
	}
}
