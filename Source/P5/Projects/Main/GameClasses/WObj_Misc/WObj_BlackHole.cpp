/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_BlackHole.cpp

	Author:			Anton Ragnarsson, Patrik Willbo

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_BlackHole implementation

	Comments:		

	History:		
		050929:		(anton)  Created File
		051009:		(willbo) Added visual effect, moved m_iState to m_Data[0].
		060130:		(anton)  Added line-check, tweaked forces, added ragdoll support.
		060614:		(willbo) Removed OnRefresh/OnClientRender when inactive. Added script support.
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_BlackHole.h"
#include "../WObj_CharMsg.h"
#include "../CConstraintSystem.h"

#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_PhysCluster.h"

const fp32 BLACKHOLE_STATETIME_INIT_SOUNDS		= 0.15f;
const fp32 BLACKHOLE_STATETIME_INIT    			= 1.0f;
const fp32 BLACKHOLE_STATETIME_MAIN    			= 9.5f + BLACKHOLE_STATETIME_INIT;
const fp32 BLACKHOLE_STATETIME_COLLAPSE			= 0.6f + BLACKHOLE_STATETIME_MAIN;
const fp32 BLACKHOLE_STATETIME_EXPLODE 			= BLACKHOLE_STATETIME_COLLAPSE;
const fp32 BLACKHOLE_STATETIME_POST				= 0.2f + BLACKHOLE_STATETIME_EXPLODE;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_BlackHole
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_BlackHole, parent, 0x0100);

#define BLACKHOLE_DEFAULT_FORCE 25.0f		// Per each level	( 25,  50,  75, 100, 125)
#define BLACKHOLE_DEFAULT_MAXFORCE 175.0f	// Per each level	(175, 350, 525, 700, 875)

void CWObject_BlackHole::OnCreate()
{
	parent::OnCreate();
	m_bNoSave = true;

	Data(BLACKHOLE_DATA_FLAGS) = BLACKHOLE_FLAG_TRACETEST | BLACKHOLE_FLAG_BILLBOARD;
	Data(BLACKHOLE_DATA_STATE) = BLACKHOLE_STATE_INACTIVE;
	Data(BLACKHOLE_DATA_OWNER) = m_iOwner;
	m_Force = BLACKHOLE_DEFAULT_FORCE;
	m_MaxForce = BLACKHOLE_DEFAULT_MAXFORCE;
	m_ActiveForce = 4.0f;
	m_nObjects = 0;
	m_bQueuedDeactivation = false;
	m_ObjectFlags = OBJECT_FLAGS_OBJECT | OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_PICKUP;
}


void CWObject_BlackHole::OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer)
{
	// Include models
	IncludeModelFromKey("MODEL",  _pReg, _pMapData);
	IncludeModelFromKey("MODEL0", _pReg, _pMapData);
	IncludeModelFromKey("MODEL1", _pReg, _pMapData);
	IncludeModelFromKey("MODEL2", _pReg, _pMapData);
	IncludeModelFromKey("LVL_MODEL1", _pReg, _pMapData);
	IncludeModelFromKey("LVL_MODEL2", _pReg, _pMapData);
	IncludeModelFromKey("LVL_MODEL3", _pReg, _pMapData);
	IncludeModelFromKey("LVL_MODEL4", _pReg, _pMapData);
	IncludeModelFromKey("LVL_MODEL5", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_3D_START", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_3D_END", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_3D_LOOP", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_2D_START", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_2D_END", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_2D_LOOP", _pReg, _pMapData);
}

void CWObject_BlackHole::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	// Moved to register, yay! =)
    //_pWData->GetResourceIndex_Sound("Gam_drk_bh-start");	// Start sound
	//_pWData->GetResourceIndex_Sound("Gam_drk_bh-end");		// Stop sound
	//_pWData->GetResourceIndex_Sound("Gam_drk_bh-loop");		// Loop sound
}


void CWObject_BlackHole::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr Value = _pKey->GetThisValue();

	switch (_KeyHash)
	{
	case MHASH2('MODE','L'): // "MODEL"
	case MHASH2('MODE','L0'): // "MODEL0"
		{
			Model_Set(0, Value);
			break;
		}
	
	case MHASH2('MODE','L1'): // "MODEL1"
		{
			Model_Set(1, Value);
			break;
		}
	
	case MHASH2('MODE','L2'): // "MODEL2"
		{
			Model_Set(2, Value);
			break;
		}
	
	case MHASH3('LVL_','MODE','L1'):
		{
			ModelEx_Set(0, Value);
			break;
		}

	case MHASH3('LVL_','MODE','L2'):
		{
			ModelEx_Set(1, Value);
			break;
		}

	case MHASH3('LVL_','MODE','L3'):
		{
			ModelEx_Set(2, Value);
			break;
		}

	case MHASH3('LVL_','MODE','L4'):
		{
			ModelEx_Set(3, Value);
			break;
		}

	case MHASH3('LVL_','MODE','L5'):
		{
			ModelEx_Set(4, Value);
			break;
		}

	case MHASH4('SOUN','D_3D','_STA','RT'): // "SOUND_3D_START"
		{
			m_liSounds[BLACKHOLE_SOUND_3D_START] = m_pWServer->GetMapData()->GetResourceIndex_Sound(Value);
			break;
		}

	case MHASH3('SOUN','D_3D','_END'): // "SOUND_3D_END"
		{
			m_liSounds[BLACKHOLE_SOUND_3D_END] = m_pWServer->GetMapData()->GetResourceIndex_Sound(Value);
			break;
		}

	case MHASH4('SOUN','D_3D','_LOO','P'): // "SOUND_3D_LOOP"
		{
			m_liSounds[BLACKHOLE_SOUND_3D_LOOP] = m_pWServer->GetMapData()->GetResourceIndex_Sound(Value);
			break;
		}

	case MHASH4('SOUN','D_2D','_STA','RT'): // "SOUND_2D_START"
		{
			m_liSounds[BLACKHOLE_SOUND_2D_START] = m_pWServer->GetMapData()->GetResourceIndex_Sound(Value);
			break;
		}

	case MHASH3('SOUN','D_2D','_END'): // "SOUND_2D_END"
		{
			m_liSounds[BLACKHOLE_SOUND_2D_END] = m_pWServer->GetMapData()->GetResourceIndex_Sound(Value);
			break;
		}

	case MHASH4('SOUN','D_2D','_LOO','P'): // "SOUND_2D_LOOP"
		{
			m_liSounds[BLACKHOLE_SOUND_2D_LOOP] = m_pWServer->GetMapData()->GetResourceIndex_Sound(Value);
			break;
		}

	case MHASH3('ACTI','VEFO','RCE'): // "ACTIVEFORCE"
		{
			m_ActiveForce = (fp32)Value.Val_fp64();
			break;
		}

	case MHASH4('AFFE','CT_O','BJEC','TS'):
		{
			m_ObjectFlags = (int32)Value.TranslateFlags(ms_ObjectFlagsTranslate);
			break;
		}

	case MHASH2('FLAG','S'):
		{
			static const char* s_BlackHoleFlagsTranslate[] = { "TRACETEST", "BILLBOARD", "XROTATION", "YROTATION", NULL };
			Data(BLACKHOLE_DATA_FLAGS) = Value.TranslateFlags(s_BlackHoleFlagsTranslate);
			break;
		}

	default:
		{
			parent::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}


void CWObject_BlackHole::OnFinishEvalKeys()
{
	parent::OnFinishEvalKeys();
}


void CWObject_BlackHole::OnSpawnWorld()
{
	parent::OnSpawnWorld();
}


void CWObject_BlackHole::Model_Set(const uint8 _iModel, const CStr& _Model)
{
	// Set model index (... and maybe some additional setup?)
	iModel(_iModel) = m_pWServer->GetMapData()->GetResourceIndex_Model(_Model);
}


void CWObject_BlackHole::ModelEx_Set(const uint8 _iSlot, const CStr& _Model)
{
	// Set model index (... and maybe some additional setup?)
	iExModel(_iSlot) = m_pWServer->GetMapData()->GetResourceIndex_Model(_Model);
}


void CWObject_BlackHole::Activate(const CVec3Dfp32& _Position, uint _iLevel)
{
	// If blackhole isn't active and not waiting for deactivation we can activate it
	if(!IsActive() && !m_bQueuedDeactivation)
	{
		m_nObjects = 0;
		m_liObjects.Create(100, 8);

		// Set hole to initialization state and correct animation time and position
		Data(BLACKHOLE_DATA_STATE) = BLACKHOLE_STATE_INIT;
		Data(BLACKHOLE_DATA_ACTIVATETICK) = m_pWServer->GetGameTick();
		Data(BLACKHOLE_DATA_LASTDAMAGETICK) = 0;
		Data(BLACKHOLE_DATA_LVLMODEL) = iExModel(MinMT(_iLevel, BLACKHOLE_EXMODELS_NUM-1));
		SetAnimTick(m_pWServer, 0, 0);
		
		// Reset force modifier
		fp32 LevelFp32 = fp32(_iLevel + 1);
		m_Force = BLACKHOLE_DEFAULT_FORCE * LevelFp32;
		m_MaxForce = BLACKHOLE_DEFAULT_MAXFORCE * LevelFp32;

		// Set black hole position
		CMat4Dfp32 PositionMat;
		PositionMat.Unit3x3();
		PositionMat.GetRow(3) = _Position;
		m_pWServer->Object_SetPositionNoIntersection(m_iObject, PositionMat);

		// Play black hole activation sound
		int i3DSound = m_liSounds[BLACKHOLE_SOUND_3D_START];
		int i2DSound = m_liSounds[BLACKHOLE_SOUND_2D_START];
		if (i3DSound > 0)
			m_pWServer->Sound_At(_Position, i3DSound, 0);
		if (i2DSound > 0)
			m_pWServer->Sound_On(m_iObject, i2DSound, 0);

		//M_TRACE("Black hole start!  pos: %s\n", _Position.GetString().Str());

		ClientFlags() &= ~(CWO_CLIENTFLAGS_NOREFRESH | CWO_CLIENTFLAGS_INVISIBLE);
	}
}


void CWObject_BlackHole::Deactivate()
{
	if(CanDeactivate())
	{
		//M_TRACE("BlackHole::Deactivate\n");

		// Set correct anim time for deactivation
		int Ticks = int(BLACKHOLE_STATETIME_MAIN * m_pWServer->GetGameTicksPerSecond());
		fp32 Fraction = BLACKHOLE_STATETIME_MAIN - (Ticks * m_pWServer->GetGameTickTime());
		SetAnimTick(m_pWServer, Ticks, Fraction);

		// Set black hole state (This is already valid after can deactive check ?)
		//Data(BLACKHOLE_DATA_STATE) = BLACKHOLE_STATE_MAIN;

		m_bQueuedDeactivation = false;
	}
	else
		m_bQueuedDeactivation = true;
}


void CWObject_BlackHole::SetExplode(const bool _bExplode)
{
	if (_bExplode)
		Data(BLACKHOLE_DATA_FLAGS) = Data(BLACKHOLE_DATA_FLAGS) | BLACKHOLE_FLAG_EXPLODE;
	else
		Data(BLACKHOLE_DATA_FLAGS) = Data(BLACKHOLE_DATA_FLAGS) & ~BLACKHOLE_FLAG_EXPLODE;
}


void CWObject_BlackHole::AffectObjects(fp32 _Radius, fp32 _Force, uint _nDamage, bool _bExplode)
{
	FindObjects(_Radius);

	CWO_DamageMsg Dmg(_nDamage, DAMAGETYPE_BLACKHOLE);
	const CVec3Dfp32& BlackHolePos = GetPosition();

	if(_bExplode && m_iOwner)
	{
		CWObject* pObj = m_pWServer->Object_Get(m_iOwner);
		CWObject_Character* pChar = TDynamicCast<CWObject_Character>(pObj);
		CWO_Character_ClientData* pCharCD = (pChar) ? CWObject_Character::GetClientData(pChar) : NULL;
		if(pCharCD && pCharCD->m_iPlayer != -1)
		{
			//CCollisionInfo CInfo;
			//int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
			//int32 TraceLineFlags = (OBJECT_FLAGS_WORLD | OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_PROJECTILE);
			//int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID;
			
			// Yeah yeah, ugly as hell. Might need to do this a weeebit nicer...
			//bool bHit = m_pWServer->Phys_IntersectLine(BlackHolePos,pChar->GetPosition() + CVec3Dfp32(0,0,32.0f),OwnFlags,TraceLineFlags,MediumFlags,&CInfo,m_iObject);
			//if(bHit && CInfo.m_bIsValid && CInfo.m_iObject == pChar->m_iObject)
			{
				CNetMsg Msg(PLAYER_NETMSG_DISORIENTATION_BLACKHOLE);
				m_pWServer->NetMsg_SendToObject(Msg, pChar->m_iObject);
			}
		}
	}

	uint nActive = m_nObjects;
	for (uint i = 0; i < nActive; i++)
	{
		uint16 iObj = 0;
		m_liObjects.GetValue(i, iObj);

		CWObject* pObj = m_pWServer->Object_Get(iObj);
		CWObject_Character* pChar = TDynamicCast<CWObject_Character>(pObj);

		if (pObj && (pObj->m_pRigidBody2 || pChar))
		{
			CVec3Dfp32 ObjPos;
			pObj->GetAbsBoundBox()->GetCenter(ObjPos);
			const fp32 DistanceSqr = ObjPos.DistanceSqr(BlackHolePos);

			CVec3Dfp32 x;
			if (_bExplode)
			{
				uint32 Seed = (iObj * 4561);
				x.k[0] = MFloat_GetRand(Seed + 0) * 2.0f - 1.0f;
				x.k[1] = MFloat_GetRand(Seed + 1) * 2.0f - 1.0f;
				x.k[2] = MFloat_GetRand(Seed + 2) * 2.0f;
			}
			else
			{
				x = ObjPos;
				x -= BlackHolePos;
			}
			x.Normalize();
			x *= _Force * m_Force;

			if (pChar)
			{
				// Make sure blackhole doesn't affect darklings or player
				CWO_Character_ClientData* pCharCD = CWObject_Character::GetClientData(pChar);
				if (pCharCD->IsDarkling() || pChar->m_iObject == m_iOwner)
					continue;

#ifdef INCLUDE_OLD_RAGDOLL
				if (pChar->m_spRagdoll)
				{
					x *= 0.006f;
					fp32 l = x.Length();
					if (l > 3.8f)
						x.SetLength(3.8f);

					pChar->m_spRagdoll->AddImpulse(CConstraintSystem::BODY_PART, x);
				}
				else 
#endif // INCLUDE_OLD_RAGDOLL

				if(pChar->m_pPhysCluster)
				{
					x *= 0.006f;
					fp32 l = x.Length();
					if (l > 3.8f)
						x.SetLength(3.8f);

					x *= RAGDOLL_FORCE_MULTIPLIER;

					m_pWServer->Phys_AddForce(pChar->m_pPhysCluster->m_lObjects[0].m_pRB,
						//pChar->m_pPhysCluster->m_lObjects[0].m_Transform.GetRow(3),
						x);
				}
				else
				{
					uint8 State = (!_bExplode) ? BLACKHOLE_STATE_PULL : BLACKHOLE_STATE_IDLE;
					
					// Insta-kill characters within 1 meter
					if (DistanceSqr <= Sqr(32.0f))
						State = BLACKHOLE_STATE_KILL;

					// Calculate amount of damage between 0 -> 1 range
					fp32 HalfRadiusSqr = Sqr(_Radius * 0.5f);
					fp32 Damage01 = Clamp01(1.0f - (MaxMT(DistanceSqr - HalfRadiusSqr, 0.0f) / HalfRadiusSqr));

					// Send message to character
					CWObject_Message Msg(OBJMSG_CHAR_EXPOSEDTOBLACKHOLE, (aint)&Damage01, State, -1, 0, BlackHolePos);
					m_pWServer->Message_SendToObject(Msg, iObj);

					// Send damage to character only when hole explodes
					if (_nDamage > 0 && _bExplode)
						Dmg.Send(iObj, m_iObject, m_pWServer);
				}
			}
			else
			{
				fp32 Mass = pObj->GetMass();
				fp32 MassDamp = M_Exp(0.015f * Mass);
				x *= 0.015f * Mass / MassDamp;

				m_pWServer->Phys_AddForce(iObj, x);

				// Give damage to objects
				if (_nDamage > 0)
					Dmg.Send(iObj, m_iObject, m_pWServer);

				// Make sure player doesn't receive physics damage from objects affected by black hole
				pObj->m_iOwner = m_iOwner;
			}
		}
	}


	// Damage triggers only affected by blackhole damage
	if (_nDamage > 0)
		DamageTriggers(Dmg, _Radius);
}


static M_INLINE void GetRandomPointInBox(const CBox3Dfp32& _Box, CVec3Dfp32& _Result)
{
	CVec3Dfp32 t(Random, Random, Random);
	_Result.k[0] = _Box.m_Min.k[0] + t.k[0] * (_Box.m_Max.k[0] - _Box.m_Min.k[0]);
	_Result.k[1] = _Box.m_Min.k[1] + t.k[1] * (_Box.m_Max.k[1] - _Box.m_Min.k[1]);
	_Result.k[2] = _Box.m_Min.k[2] + t.k[2] * (_Box.m_Max.k[2] - _Box.m_Min.k[2]);
}


void CWObject_BlackHole::DamageTriggers(CWO_DamageMsg& _Dmg, fp32 _Radius)
{
	// Find triggers and damage them,
	const CVec3Dfp32& BlackHolePos = GetPosition();
	TSelection<CSelection::MEDIUM_BUFFER> Selection;
	m_pWServer->Selection_AddBoundSphere(Selection, OBJECT_FLAGS_TRIGGER, BlackHolePos, _Radius);

	const int16* pSel;
	uint nSel = m_pWServer->Selection_Get(Selection, &pSel);
	for (uint i = 0; i < nSel; i++)
		_Dmg.Send(pSel[i], m_iObject, m_pWServer);
}


void CWObject_BlackHole::FindObjects(fp32 _Radius)
{
	const CVec3Dfp32& BlackHolePos = GetPosition();
	TSelection<CSelection::LARGE_BUFFER> Selection;
	m_pWServer->Selection_AddBoundSphere(Selection, m_ObjectFlags, BlackHolePos, _Radius);

	const int16* pSel;
	uint nSel = m_pWServer->Selection_Get(Selection, &pSel);
	const bool bTraceTest = (m_Data[BLACKHOLE_DATA_FLAGS] & BLACKHOLE_FLAG_TRACETEST) != 0;
	for (uint i = 0; i < nSel; i++)
	{
		uint16 iObj = pSel[i];
		if (m_liObjects.GetIndex(iObj) >= 0)
			continue; // already added to list

		if (m_nObjects >= m_liObjects.GetMaxIDs())
			break; // no room for more objects

		CWObject* pObj = m_pWServer->Object_Get(iObj);
		CWObject_Character* pChar = TDynamicCast<CWObject_Character>(pObj);

		if (pObj && (pObj->m_pRigidBody2 || pChar))
		{
			if (bTraceTest)
			{
				CVec3Dfp32 ObjPos;
				GetRandomPointInBox(*pObj->GetAbsBoundBox(), ObjPos);
				const fp32 DistanceSqr = ObjPos.DistanceSqr(BlackHolePos);

				// Check radius
				if ((DistanceSqr > Sqr(_Radius)))
					continue;

				// Do trace-line test
				CCollisionInfo CInfo;
				CVec3Dfp32 p0 = BlackHolePos;
				CVec3Dfp32 p1 = ObjPos;
				p0 += (CVec3Dfp32(Random, Random, Random) * 32.0f) - CVec3Dfp32(16.0f);

				m_pWServer->Phys_IntersectLine(p0, p1, 0, OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PLAYERPHYSMODEL, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &CInfo, iObj); 
				m_pWServer->Debug_RenderWire(p0, p1, CInfo.m_bIsValid ? 0xff0000ff : 0xff00ff00);
				if (CInfo.m_bIsValid)
					continue;
			}

			// Ok, add to list
			m_liObjects.Insert(m_nObjects++, iObj);
		}
	}
}


void CWObject_BlackHole::OnRefresh()
{
	parent::OnRefresh();

	// No update when hole is inactive
	if (Data(BLACKHOLE_DATA_STATE) == BLACKHOLE_STATE_INACTIVE)
	{
		ClientFlags() |= (CWO_CLIENTFLAGS_NOREFRESH | CWO_CLIENTFLAGS_INVISIBLE);
		return;
	}

	int32 AnimTick = GetAnimTick(m_pWServer);
	fp32 AnimTime = AnimTick * m_pWServer->GetGameTickTime();

	switch (Data(BLACKHOLE_DATA_STATE))
	{
	case BLACKHOLE_STATE_INIT:
		{
			// Start playing loop sound
			if (AnimTime >= BLACKHOLE_STATETIME_INIT_SOUNDS && (!iSound(0) || !iSound(1)))
			{
				iSound(0) = m_liSounds[BLACKHOLE_SOUND_3D_LOOP];
				iSound(1) = m_liSounds[BLACKHOLE_SOUND_2D_LOOP];
			}

			// Time to change state?
			if (AnimTime >= BLACKHOLE_STATETIME_INIT)
				Data(BLACKHOLE_DATA_STATE) = BLACKHOLE_STATE_MAIN;

			fp32 Phase = (AnimTime * (1.0f / BLACKHOLE_STATETIME_INIT));
			fp32 Radius = 32.0f + 224.0f * Phase;
			fp32 Force = -(1.0f + Phase);
			AffectObjects(Radius, Force, 0, false);
			m_pWServer->Phys_SetCollisionPrecision(1);
		}
		break;

	case BLACKHOLE_STATE_MAIN:
		{
			// We are waiting to be able to deactivate, so de it now
			if(m_bQueuedDeactivation)
				Deactivate();

			// Time to change state?
			if (AnimTime >= BLACKHOLE_STATETIME_MAIN)
			{
				if (Data(BLACKHOLE_DATA_FLAGS) & BLACKHOLE_FLAG_EXPLODE)
				{
					Data(BLACKHOLE_DATA_STATE) = BLACKHOLE_STATE_EXPLODE;
					Data(BLACKHOLE_DATA_FLAGS) = Data(BLACKHOLE_DATA_FLAGS) & ~BLACKHOLE_FLAG_EXPLODE;
				}
				else
					Data(BLACKHOLE_DATA_STATE) = BLACKHOLE_STATE_COLLAPSE;

				// Play end sound
				int i3DSound = m_liSounds[BLACKHOLE_SOUND_3D_END];
				int i2DSound = m_liSounds[BLACKHOLE_SOUND_2D_END];
				if (i3DSound > 0)
					m_pWServer->Sound_At(GetPosition(), i3DSound, 0);
				if (i2DSound > 0)
					m_pWServer->Sound_On(m_iObject, i2DSound, 0);
			}

			// Apply some damage every second...
			uint nDamage = 0;
			uint nMainTicks = uint((AnimTime - BLACKHOLE_STATETIME_INIT) * m_pWServer->GetGameTicksPerSecond());
			if(nMainTicks - Data(BLACKHOLE_DATA_LASTDAMAGETICK) > (m_pWServer->GetGameTicksPerSecond() * 0.25f))
			{
				Data(BLACKHOLE_DATA_LASTDAMAGETICK) += uint(m_pWServer->GetGameTicksPerSecond());
				nDamage = uint(Data(BLACKHOLE_DATA_LASTDAMAGETICK) / m_pWServer->GetGameTicksPerSecond());
			}

			AffectObjects(256.0f, -2.0f, nDamage, false);
		}
		break;

	case BLACKHOLE_STATE_EXPLODE:
		{
			// Time to change state?
			if (AnimTime >= BLACKHOLE_STATETIME_EXPLODE)
				SetPostState();

			// Damage time is the time the blackhole has been running after it was initialized
			const fp32 ActiveTime = MinMT((Data(BLACKHOLE_DATA_ACTIVATETICK) * m_pWServer->GetGameTickTime()) - BLACKHOLE_STATETIME_INIT, 5.0f);
			
			// Calculate amount of damage, force and the range of affection
			//uint nDamage = uint(ActiveTime * 20.0f);
			const fp32 Force = ActiveTime * 2.0f;
			const fp32 Range = Clamp01(ActiveTime) * 96.0f;

			// Run the affect with current setup
			AffectObjects(Range, Force, 0, true);
		}
		break;

	case BLACKHOLE_STATE_COLLAPSE:
		{
			if (AnimTime >= BLACKHOLE_STATETIME_COLLAPSE)
				SetPostState();
		}
		break;

	case BLACKHOLE_STATE_POST:
		{
			// Time to change state?
			if(AnimTime >= BLACKHOLE_STATETIME_POST)
			{
				iSound(0) = 0;
				iSound(1) = 0;
				Data(BLACKHOLE_DATA_STATE) = BLACKHOLE_STATE_INACTIVE;
				m_bQueuedDeactivation = false;

				// reset m_iOwner for all objects
				uint nActive = m_nObjects;
				for (uint i = 0; i < nActive; i++)
				{
					uint16 iObj = 0;
					m_liObjects.GetValue(i, iObj);
					CWObject* pObj = m_pWServer->Object_Get(iObj);
					if (pObj && (pObj->m_iOwner == m_iOwner))
						pObj->m_iOwner = 0;
				}
			}
		}
		break;
	}
}


void CWObject_BlackHole::SetOwnerDarkness(const uint16 _DarknessType)
{
	if (m_iOwner)
	{
		CWObject_Character* pOwner = CWObject_Character::IsCharacter(m_iOwner, m_pWServer);
		if (pOwner && pOwner->m_spAI)
			pOwner->m_spAI->SetPlayerDarkness(_DarknessType);
	}
}


void CWObject_BlackHole::SetPostState()
{
	Data(BLACKHOLE_DATA_STATE) = BLACKHOLE_STATE_POST;
	m_nObjects = 0;

	SetOwnerDarkness(0);
	m_pWServer->Phys_SetCollisionPrecision(0);
}


bool CWObject_BlackHole::IsActive()
{
	return (Data(BLACKHOLE_DATA_STATE) != BLACKHOLE_STATE_INACTIVE);
}


bool CWObject_BlackHole::CanDeactivate()
{
	return (Data(BLACKHOLE_DATA_STATE) == BLACKHOLE_STATE_MAIN);
}


fp32 CWObject_BlackHole::AddForce()
{
	return AddForce(m_ActiveForce);
}


fp32 CWObject_BlackHole::AddForce(fp32 _Force)
{
	if (_Force == 0.0f)
		return 0.0f;

	fp32 Before = m_Force;
	m_Force = Min(m_MaxForce,m_Force + _Force);

	// Return amount of force added
	return (m_Force - Before);
}


int CWObject_BlackHole::GetDarkness(int _MaxDarkness, int _GameTick)
{
	fp32 Length(m_pWServer->GetGameTicksPerSecond() * 5.0f);
	fp32 Amount = fp32(_GameTick - m_Data[BLACKHOLE_DATA_ACTIVATETICK]) / Length;
	return (_MaxDarkness - (TruncToInt(fp32(_MaxDarkness) * Amount)));
}


fp32 CWObject_BlackHole::GetActiveForce()
{
	return m_ActiveForce;
}


aint CWObject_BlackHole::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_IMPULSE:
		{
			switch(_Msg.m_Param0)
			{
			case BLACKHOLE_IMPULSE_ACTIVATE:
				{
					Activate(GetPosition(), 0);
				}
				break;

			case BLACKHOLE_IMPULSE_DEACTIVATE_EXPLODE:
				{
					SetExplode(true);
					Deactivate();
				}
				break;

			case BLACKHOLE_IMPULSE_DEACTIVATE_COLLAPSE:
				{
					SetExplode(false);
					Deactivate();
				}
				break;
			}
		}
		return 1;
	}

	return parent::OnMessage(_Msg);
}


void CWObject_BlackHole::OnDeltaSave(CCFile* _pFile)
{
	parent::OnDeltaSave(_pFile);
}


void CWObject_BlackHole::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	parent::OnDeltaLoad(_pFile, _Flags);
}


fp32 CWObject_BlackHole::GetActivateRenderTime(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	const fp32 TickTime = _pWClient->GetGameTickTime();
	return (((_pWClient->GetGameTick() - _pObj->m_Data[BLACKHOLE_DATA_ACTIVATETICK]) * TickTime) + (_pWClient->GetRenderTickFrac() * TickTime));
}


void CWObject_BlackHole::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _RenderMat)
{
	// Render only if the black hole isn't inactive
	if (_pObj->m_Data[BLACKHOLE_DATA_STATE] != BLACKHOLE_STATE_INACTIVE)
	{
		// Get anim state and position matrix
		CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
		CMat4Dfp32 PosMat;

		if (_pObj->m_Data[BLACKHOLE_DATA_FLAGS] & BLACKHOLE_FLAG_BILLBOARD)
			_pWClient->Render_GetLastRenderCamera(PosMat);
		else
			PosMat = _pObj->GetPositionMatrix();

		if (_pObj->m_Data[BLACKHOLE_DATA_FLAGS] & BLACKHOLE_FLAG_XROTATION)
			PosMat.RotX_x_M(GetActivateRenderTime(_pObj, _pWClient) * 0.1f);
		else if (_pObj->m_Data[BLACKHOLE_DATA_FLAGS] & BLACKHOLE_FLAG_YROTATION)
			PosMat.RotY_x_M(GetActivateRenderTime(_pObj, _pWClient) * 0.1f);
		else
			PosMat.RotZ_x_M(GetActivateRenderTime(_pObj, _pWClient) * 0.1f);
		PosMat.GetRow(3) = _pObj->GetPosition();

		// Render level model, this one has a diffrent matrix setup than the other normal models
		{
			CXR_Model* pModel = (_pObj->m_Data[BLACKHOLE_DATA_LVLMODEL]) ? _pWClient->GetMapData()->GetResource_Model(_pObj->m_Data[BLACKHOLE_DATA_LVLMODEL]) : NULL;
			if (pModel)
				_pEngine->Render_AddModel(pModel, PosMat, AnimState);
		}

		// Add the other two models for rendering
		PosMat = _pObj->GetPositionMatrix();
		for (uint8 i = 0; i < 3; i++)
		{
			CXR_Model* pModel = (_pObj->m_iModel[i]) ? _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]) : NULL;
			if (pModel)
				_pEngine->Render_AddModel(pModel, PosMat, AnimState);
		}
	}
}

