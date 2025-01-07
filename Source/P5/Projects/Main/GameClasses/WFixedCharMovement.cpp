
#include "PCH.h"
#include "WFixedCharMovement.h"
#include "WObj_Game/WObj_GameMessages.h"
#include "WObj_Char.h"
#include "../../../Shared/MOS/Classes/GameContext/WGameContext.h"

/*CMoveTarget::CMoveTarget()
{
	MakeInvalid();
}

void CMoveTarget::MakeInvalid()
{
	// Clear the member vars
	m_Target = 0;
	m_Velocity = 0;
	m_TargetTime.Reset();
	m_MoveMode = MOVETARGET_MODE_INVALID;
}

void CMoveTarget::Copy(const CMoveTarget& _MoveTarget)
{
	m_Target = _MoveTarget.m_Target;
	m_Velocity = _MoveTarget.m_Velocity;
	m_TargetTime = _MoveTarget.m_TargetTime;
	m_MoveMode = _MoveTarget.m_MoveMode;
}

void CMoveTarget::Pack(uint8 *&_pD)
{
	// Pack the movetarget a bit, not very optimized yet :/
	int32 Temp = *(int32*)&m_Target.k[0];
	PTR_PUTINT32(_pD, Temp);
	Temp = *(int32*)&m_Target.k[1];
	PTR_PUTINT32(_pD, Temp);
	Temp = *(int32*)&m_Target.k[2];
	PTR_PUTINT32(_pD, Temp);

	Temp = *(int32*)&m_Velocity.k[0];
	PTR_PUTINT32(_pD, Temp);
	Temp = *(int32*)&m_Velocity.k[1];
	PTR_PUTINT32(_pD, Temp);
	Temp = *(int32*)&m_Velocity.k[2];
	PTR_PUTINT32(_pD, Temp);

	PTR_PUTCMTIME(_pD, m_TargetTime);

	PTR_PUTUINT8(_pD, m_MoveMode);
}

void CMoveTarget::Unpack(const uint8 *&_pD)
{
	// Unpack the movetarget a bit
	int32 Temp;
	PTR_GETINT32(_pD, Temp);
	m_Target.k[0] = *(fp32*)&Temp;
	PTR_GETINT32(_pD, Temp);
	m_Target.k[1] = *(fp32*)&Temp;
	PTR_GETINT32(_pD, Temp);
	m_Target.k[2] = *(fp32*)&Temp;

	PTR_GETINT32(_pD, Temp);
	m_Velocity.k[0] = *(fp32*)&Temp;
	PTR_GETINT32(_pD, Temp);
	m_Velocity.k[1] = *(fp32*)&Temp;
	PTR_GETINT32(_pD, Temp);
	m_Velocity.k[2] = *(fp32*)&Temp;

	PTR_GETCMTIME(_pD, m_TargetTime);

	PTR_GETUINT8(_pD, m_MoveMode);
}

CMoveTarget::CMoveTarget(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CVec3Dfp32 _Target, 
						 fp32 _Duration, CMTime _StartTime)
{
	SetTarget(_pWPhys, _pObj, _Target, _Duration, _StartTime);
}

void CMoveTarget::SetTarget(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CVec3Dfp32 _Target,
							fp32 _Duration, CMTime _StartTime)
{
	CMTime StartTime;
	CVec3Dfp32 Start;

	if (!_StartTime.IsInvalid())
		StartTime = _StartTime;
	else
		StartTime = _pWPhys->GetGameTime();

	m_TargetTime = StartTime + CMTime::CreateFromSeconds(_Duration);
	if (m_MoveMode & MOVETARGET_MODE_LEDGEPOS)
	{
		m_Target = _Target;
		m_Velocity.k[0] = (m_Target.k[0] - m_Target.k[1]) / (m_TargetTime - StartTime).GetTime();
	}
	else
	{
		Start = _pObj->GetPosition();
		m_Target = _Target;
		m_Velocity = (m_Target - Start) / (20 * (m_TargetTime - StartTime).GetTime());
	}

	m_MoveMode |= MOVETARGET_MODE_MOVING;
}

// Get animation velocity
void CMoveTarget::GetUserAccelleration(const CSelection& _Selection, CWObject_CoreData* _pObj, 
		CWorld_PhysState* _pPhysState, CVec3Dfp32& _VRet, CWO_Character_ClientData* _pCD, 
		int16& _Flags, uint8 _MoveFlags)
{
	// If acceleration should not be applied, make sure the characters stands still
	_VRet = 0;

	if (m_MoveMode & MOVETARGET_MODE_MOVING)
	{
		// Find suitable velocity
		//CVec3Dfp32 OptimalVelocity = (m_Target - m_Start) / (20 * (m_TargetTime - m_StartTime));
		//ConOut(CStrF("%s, LEN: %f", OptimalVelocity.GetString().GetStr(), OptimalVelocity.Length()));

		// Just set velocity for now...., should have adjustments later....
		//CVec3Dfp32 OptimalPos = 

		if (m_MoveMode & MOVETARGET_MODE_LEDGEPOS)
		{
			// Current position
			fp32 CurrentPos = _pCD->m_ControlMode_Param1;

			// Check so we haven't passed our point
			if ((m_Target.k[0] - CurrentPos) * m_Velocity.k[0] > 0)
			{
				_VRet = m_Velocity;
				return;
			}
		}
		else
		{
			CVec3Dfp32 CharPos = _pObj->GetPosition();
			fp32 Dot = (m_Target - CharPos) * m_Velocity;
			// If we have passed the target point, set the position, otherwise set the velocity
			if (Dot > 0.0f)
			{
				if (_MoveFlags & FIXEDCHARMOVE_FLAG_APPLYVELOCITY)
				{
					_pPhysState->Object_SetVelocity(_pObj->m_iObject, m_Velocity);
				}
				_VRet = m_Velocity;
				return;
			}
		}
	}

	//ConOut("Argh overshot -> NoMove");
	if (_MoveFlags & FIXEDCHARMOVE_FLAG_APPLYVELOCITY)
		_pPhysState->Object_SetVelocity(_pObj->m_iObject, 0);
	_VRet = 0;

	if (!(m_MoveMode & MOVETARGET_MODE_MOVERELATIVE))
		_pPhysState->Object_SetPosition(_pObj->m_iObject, m_Target);

	//m_MoveActionMode = MOVEACTION_MODE_NOMOVE;
	m_MoveMode |= MOVETARGET_MODE_MOVEENDED;
}

bool CMoveTarget::DoFightMove(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, int _Move, 
							  int _Stance, fp32 _Duration, CMTime _StartTime)
{
	if (!_pObj)
		return false;

	MakeInvalid();
	// Test if we can move to the new location
	CVec3Dfp32 CharPos = _pObj->GetPosition();
	// Check active stance and see if we can switch to the correct stance

	int iOpponent = _pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ISINFIGHTMODE), 
		_pObj->m_iObject);
	// The enemy might be moving so we take its target as its "real position"
	CVec3Dfp32 EnemyPos = 0;
	// Set relative move mode..
	m_MoveMode |= MOVETARGET_MODE_MOVERELATIVE;
	// Send message to opponent that checks its target position
	if (_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETFIGHTPOSITION, 0, 0, -1, 0, 
		0, 0,&EnemyPos, sizeof(CVec3Dfp32)), iOpponent) || (_Move == FIGHT_MOVE_NEWTEST_NOOPP))
	{
		CVec3Dfp32 NewPosition;
		int MoveType = 0;
		if (!GetNewFightPosition(_pObj,_Move, _Stance, CharPos, EnemyPos, NewPosition, MoveType))
			return false;

		// Check if we can move to the new location
		if (CanMove(CharPos, EnemyPos, NewPosition))
		{
			// Move to the new location somehow. Set a velocity towards the new point?
			// And hope the animation takes care of the rest (ie to look good)
			// Ok then, set the velocity for the character towards the new point
			// Set the move animation with the move action...
			SetTarget(_pWPhys, _pObj, NewPosition, _Duration, _StartTime);
			
			return true;
		}
	}
	else
	{
		// If we didn't find any opponent, just go forward
		EnemyPos = CharPos + CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(),0)*10;
		CVec3Dfp32 NewPosition;
		int MoveType = 0;
		if (!GetNewFightPosition(_pObj,_Move, _Stance, CharPos, EnemyPos, NewPosition, MoveType))
			return false;

		// Check if we can move to the new location
		if (CanMove(CharPos, EnemyPos, NewPosition))
		{
			// Move to the new location somehow. Set a velocity towards the new point?
			// And hope the animation takes care of the rest (ie to look good)
			// Ok then, set the velocity for the character towards the new point
			// Set the move animation with the move action...
			SetTarget(_pWPhys, _pObj, NewPosition, _Duration, _StartTime);
			
			return true;
		}
	}

	return false;
}*/


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Tests whether or not we can move when fighting
						
	Parameters:			
		_CharPos:			Character position
		_EnemyPos:			Enemy position
		_NewPosition:		The new position
						
	Returns:			Whether or not we can move to the new position
						right now hardcoded to true though
\*____________________________________________________________________*/
/*bool CMoveTarget::CanMove(CVec3Dfp32 _CharPos, CVec3Dfp32 _EnemyPos, CVec3Dfp32 _NewPosition)
{
	// Should be better testing here later
	return true;
}*/

#define STEPDISTANCE (25.0f)
#define STEPSIDEY (19.13417161825448858642299920152f)
#define STEPSIDEX (3.8060233744356621935908405301606f)
#define STEPSIDETURNY (35.355339059327376220042218105242f)
#define STEPSIDETURNX (14.644660940672623779957781894758f)
#define STEPSIDEFORWARDY (17.677669529663688110021109052621f)
#define STEPSIDEFORWARDX (32.322330470336311889978890947379f)
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Finds a new position from a fight move
						
	Parameters:			
		_Move:				Fight move that is being performed
		_CharPos:			Current character position
		_OpponentPos:		Current opponent position
						
	Returns:			The new position
\*____________________________________________________________________*/
/*bool CMoveTarget::GetNewFightPosition(CWObject_CoreData* _pObj, int _Move, int _Stance, 
									  const CVec3Dfp32& _CharPos, const CVec3Dfp32& _OpponentPos, 
									  CVec3Dfp32& _ReturnPos, int& _MoveType)
{
	CVec3Dfp32 Direction = (_CharPos - _OpponentPos);
	Direction.k[2] = 0.0f;
	Direction.Normalize();
	CVec3Dfp32 Right;
	Direction.CrossProd(CVec3Dfp32(0,0,1.0f),Right);
	fp32 DistanceOffset = 0.0f;
	if (_Stance & FIGHT_STANCE_CLOSEMASK)
		DistanceOffset = 25.0f;
	else if (_Stance & FIGHT_STANCE_NEUTRALMASK)
		DistanceOffset = 50.0f;
	else if (_Stance & FIGHT_STANCE_SAFEMASK)
		DistanceOffset = 75.0f;

	// The movement is quite constant so there's no need to calculate distances each time, only
	// take direction into account
	switch (_Move)
	{
	case FIGHT_MOVE_NEWTEST:
		{
			// Move towards opponent
			fp32 StepLength = 70.0f;
			_MoveType = AG_FIGHTMODE_STANCEMOVETYPENONE;
			_ReturnPos = _CharPos - Direction * StepLength;
			break;
		}
	case FIGHT_MOVE_NEWTEST_NOOPP:
		{
			CVec3Dfp32 CharDir = CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(),0);
			CharDir.k[2] = 0.0f;
			CharDir.Normalize();
			fp32 StepLength = 35.0f;
			_MoveType = AG_FIGHTMODE_STANCEMOVETYPENONE;
			_ReturnPos = _CharPos + CharDir * StepLength;
			break;
		}
	default:
		{
			ConOut("CAnimAction::GetNewPosition: Error - Wrong move");
			return false;
		}
	}

	return true;
}

CFixedCharMovement::CFixedCharMovement()
{
	// Nothing here right now
	m_MoveMode = FIXEDCHARMOVE_NOCOPY;
}

void CFixedCharMovement::Invalidate()
{
	// Make the movetargets invalid
	for (int i = 0; i < CFIXEDCHARMOVEMENT_MAXMOVES; i++)
	{
		m_MoveTargets[i].MakeInvalid();
	}
}

void CFixedCharMovement::Copy(const CFixedCharMovement& _CharMovement)
{
	m_MoveMode = _CharMovement.m_MoveMode;
	m_MoveTargets[0].Copy(_CharMovement.m_MoveTargets[0]);
}

bool CFixedCharMovement::AddMoveTarget(const CMoveTarget& _MoveTarget)
{
	//m_MoveTargets.Add(_MoveTarget);
	// Only one at the moment, so....
	m_MoveTargets[0] = _MoveTarget;
	m_MoveMode = FIXEDCHARMOVE_COPYONE;

	return true;
}

CVec3Dfp32 CFixedCharMovement::GetNextTarget(CWObject_CoreData* _pObj)
{
	if (m_MoveTargets[0].GetMoveMode() != MOVETARGET_MODE_INVALID)
	{
		return m_MoveTargets[0].GetTarget();
	}

	return _pObj->GetPosition();
}

// SHOULD ADJUST TO LATEST POSITION IF NO ITEMS IN QUEUE
uint8 CFixedCharMovement::GetUserAccelleration(const CSelection& _Selection, CWObject_CoreData* _pObj, 
		CWorld_PhysState* _pPhysState, CVec3Dfp32& _VRet, CWO_Character_ClientData* _pCD, 
		int16& _Flags, uint8 _MoveFlags, CWObject_CoreData* _pRelObj)
{
	// Set crouching move if needed
	if (m_MoveTargets[0].GetMoveMode() & MOVETARGET_MODE_MOVEENDED)
	{
		if (_Flags & FIXEDCHARMOVE_FLAG_APPLYVELOCITY)
			_pPhysState->Object_SetVelocity(_pObj->m_iObject, 0);
		if (m_MoveTargets[0].GetMoveMode() & MOVETARGET_MODE_MOVERELATIVE)
		{
			// Set relative end position
		}
		else
		{
			// Adjust position to latest target, (for now just set position)
			//_pPhysState->Object_SetVelocity(_pObj->m_iObject, 0);
			_pPhysState->Object_SetPosition(_pObj->m_iObject, m_MoveTargets[0].GetTarget());
		}
		m_MoveTargets[0].MakeInvalid();
	}
	else if (m_MoveTargets[0].GetMoveMode() & MOVETARGET_MODE_MOVING)
	{
		m_MoveTargets[0].GetUserAccelleration(_iSel, _pObj, _pPhysState, _VRet, _pCD, _Flags, 
			_MoveFlags);

		if (m_MoveTargets[0].GetTargetTime().Compare(_pPhysState->GetGameTime()) <= 0)
		{
			m_MoveTargets[0].GetMoveMode() |= MOVETARGET_MODE_MOVEENDED;
		}
	}
	else
	{
		// Set zero velocity
		if (_MoveFlags & FIXEDCHARMOVE_FLAG_APPLYVELOCITY)
			_pPhysState->Object_SetVelocity(_pObj->m_iObject, 0);
		_VRet = 0;
	}

	return m_MoveTargets[0].GetMoveMode();
}

void CFixedCharMovement::SCopyFrom(void* _pThis, const void* _pFrom)
{
	CFixedCharMovement& This = *(CFixedCharMovement*)_pThis;
	const CFixedCharMovement& From = *(const CFixedCharMovement*)_pFrom;
	This.m_MoveMode = From.m_MoveMode;
	
	for (int i = 0; i < CFIXEDCHARMOVEMENT_MAXMOVES; i++)
	{
		This.m_MoveTargets[i] = From.m_MoveTargets[i];
	}
}

void CFixedCharMovement::SPack(const void* _pThis, uint8 *&_pD, CMapData* _pMapData)
{
	CFixedCharMovement& This = *(CFixedCharMovement*)_pThis;
	PTR_PUTINT8(_pD, This.m_MoveMode);
	//ConOut("Pack");
	if (This.m_MoveMode & FIXEDCHARMOVE_COPYONE)
	{
		//ConOut("Pack Movetarget");
		// PACK FIRST MOVE (and only at this time)
		This.m_MoveTargets[0].Pack(_pD);
		This.m_MoveMode &= ~FIXEDCHARMOVE_COPYONE;
	}
}

void CFixedCharMovement::SUnpack(void* _pThis, const uint8 *&_pD, CMapData* _pMapData)
{
	CFixedCharMovement& This = *(CFixedCharMovement*)_pThis;
	PTR_GETINT8(_pD, This.m_MoveMode);
	//ConOut("UnPack");
	if (This.m_MoveMode & FIXEDCHARMOVE_COPYONE)
	{
		//ConOut("Unpack Movetarget");
		// UNPACK FIRST MOVE (and only at this time)
		This.m_MoveTargets[0].Unpack(_pD);
		This.m_MoveMode &= ~FIXEDCHARMOVE_COPYONE;
	}
}*/


// GENERATOR FOR FIGHT ENTRIES IN ANIMGRAPH, SHOULD PERHAPS EXPAND THIS TO OTHER AREAS AS WELL...?
#ifdef PHOBOSDEBUG
CAGFightEntry::CAGFightEntry()
{
	Clear();
}

CAGFightEntry::CAGFightEntry(CStr _Name, int _TargetStance, int _Move, int _Stance, 
							 int _FightStatus, int _Damage, int _Stamina, int _OpponentStamina, 
							 fp32 _Pre, fp32 _Attack, fp32 _Post)
{
	m_ActionName = _Name;
	m_TargetStance = _TargetStance;
	m_Move = _Move;
	m_Stance = _Stance;
	m_FightStatus = _FightStatus;
	m_Damage = _Damage;
	m_Stamina = _Stamina;
	m_OpponentStamina = _OpponentStamina;
	m_PreAttackTime = _Pre;
	m_AttackTime = _Attack;
	m_PostAttackTime = _Post;
}

void CAGFightEntry::Clear()
{
	m_ActionName.Clear();
	m_Move = 0;
	// Which stance
	m_Stance = 0;
	m_TargetStance = 0;
	// Options (what blocks/interrupts/and so on is active
	m_FightStatus = AG_FIGHTMODE_ATTACKACTIVE;

	m_Damage = 0;
	m_Stamina = 0;
	m_OpponentStamina = 0;

	m_Index = -1;

	m_PreAttackTime = 0;
	m_AttackTime = 0;
	m_PostAttackTime = 0;
	m_AttackOffsetTime = 0;

	m_Input = 0;

	m_TimeScale = 1.0f;
	m_bRegisterInput = false;
	m_AttackType = 0;
}

CStr CAGFightEntry::GetConstants(CStr _TabStr)
{
	CStr Entry;
	CStr Stance = StringFromStance(m_Stance);

	Entry = _TabStr + "*FightMode_" + Stance + "_" + m_ActionName + "_TimeScale\t" + CStrF("%f\n", m_TimeScale);
	Entry += _TabStr + "*FightMode_" + Stance + "_" + m_ActionName + "_PrePhaseTime\t" + CStrF("%f\n", m_PreAttackTime);
	Entry += _TabStr + "*FightMode_" + Stance + "_" + m_ActionName + "_AttackPhaseTime\t" + CStrF("%f\n", m_AttackTime);
	Entry += _TabStr + "*FightMode_" + Stance + "_" + m_ActionName + "_PostPhaseTime\t" + CStrF("%f\n", m_PostAttackTime);
	Entry += _TabStr + "*FightMode_" + Stance + "_" + m_ActionName + "_AttackOffsetTime\t" + CStrF("%f\n", m_AttackOffsetTime);

	return Entry;
}

bool CAGFightEntry::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	// Options (what blocks/interrupts/and so on is active
	
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();
	
	if (KeyName.Find("NAME") != -1)
	{
		m_ActionName = KeyValue;
	}
	else if (KeyName.Find("MOVE") != -1)
	{
		m_Move = ResolveMove(KeyValue);
	}
	else if (KeyName.Find("TARGETSTANCE") != -1)
	{
		m_TargetStance = ResolveStanceFromString(KeyValue);
	}
	else if (KeyName.Find("STANCE") != -1)
	{
		m_Stance = ResolveStanceFromString(KeyValue);
	}
	else if (KeyName.Find("FIGHTSTATUS") != -1)
	{
		m_FightStatus |= ResolveFightStatus(KeyValue);
	}
	else if (KeyName.Find("DAMAGE") != -1)
	{
		m_Damage = ResolveDamage(KeyValue);
	}
	else if (KeyName.Find("OPPONENTSTAMINA") != -1)
	{
		m_OpponentStamina = ResolveStamina(KeyValue);
	}
	else if (KeyName.Find("ATTACKTYPE") != -1)
	{
		m_AttackType = ResolveAttackType(KeyValue);
	}
	else if (KeyName.Find("ATTACKOFFSET") != -1)
	{
		m_AttackOffsetTime = KeyValuef;
	}
	else if (KeyName.Find("STAMINA") != -1)
	{
		m_Stamina = ResolveStamina(KeyValue);
	}
	else if (KeyName.Find("PREATTACK") != -1)
	{
		m_PreAttackTime = KeyValuef;
	}
	else if (KeyName.Find("POSTATTACK") != -1)
	{
		m_PostAttackTime = KeyValuef;
	}
	else if (KeyName.Find("ATTACK") != -1)
	{
		m_AttackTime = KeyValuef;
	}
	else if (KeyName.Find("INPUT") != -1)
	{
		m_Input |= InputFromString(KeyValue);
	}
	else if (KeyName.Find("TIMESCALE") != -1)
	{
		m_TimeScale = KeyValuef;
	}
	else if (KeyName.Find("NOTUSED") != -1)
	{
		m_bRegisterInput = (KeyValue.Val_int() != 0);
	}

	return true;
}

// Generates a state from this entry
CStr CAGFightEntry::GenerateState(CStr _TabStr, CAGFightEntryManager* _pManager)
{
	CStr Entry;
	CStr Stance = StringFromStance(m_Stance);
	CStr TargetStance = StringFromStance(m_TargetStance);
	CStr TabLvl2 = _TabStr + "\t";
	CStr PostPhaseTime = "FightMode_" + Stance + "_" + m_ActionName + "_PostPhaseTime";

	Entry += _TabStr + "*FightMode_" + Stance + "_" + m_ActionName + "\n" + _TabStr + "{\n";
	Entry += TabLvl2 + "*Inherit FightMode_OnceTo" + TargetStance + "\n";
	Entry += TabLvl2 + "*Inherit FightMode_CanInterrupt" + "\n";
	Entry += TabLvl2 + "*Animation Anim_FightMode_" + Stance + "_" + m_ActionName + "\n";
	Entry += TabLvl2 + "{\n\t" + TabLvl2 + "*TimeScale FightMode_" + Stance + "_" + m_ActionName + "_TimeScale" + "\n" + TabLvl2 + "}\n";
	Entry += "\n" + TabLvl2 + "// " + m_ActionName + ": " + StringFromFightStatus(m_FightStatus) + "\n";
	// Actions
	Entry += TabLvl2 + "*Actions" + "\n\t" + _TabStr + "{\n";
	Entry += GeneratePrePhase(TabLvl2 + "\t");
	Entry += GenerateAttackPhase(TabLvl2 + "\t");
	Entry += GeneratePostPhase(TabLvl2 + "\t");

	// Add all states that we can use from the target
	Entry += _pManager->GenerateActionList(m_TargetStance, (TabLvl2 + "\t"), PostPhaseTime);

	Entry += TabLvl2 + "}\n" + _TabStr + "}\n";

	return Entry;
}

CStr CAGFightEntry::GeneratePrePhase(CStr _TabStr)
{
	CStr Entry;
	CStr TabLvl2 = _TabStr + "\t";
	CStr TabLvl3 = TabLvl2 + "\t";
	CStr Stance = StringFromStance(m_Stance);

	Entry += _TabStr + "// PREPHASE" +"\n" + _TabStr + "*Action0\n" + _TabStr + "{\n";
	Entry += TabLvl2 + "*Effects\n" + TabLvl2 + "{\n" + TabLvl3 + "// FIGHTSTATUS TAG\n";
	Entry += TabLvl3 + "*SetFightStatus(" + GenerateFightStatus(m_FightStatus & AG_FIGHTMODE_PREPHASEMASK) + ")\n";
	Entry += TabLvl3 + "// Stamina Drain\n";
	Entry += TabLvl3 + "*DrainStamina(" + GenerateStamina(m_Stamina, AG_FIGHTMODE_STAMINATARGET_SELF) + ")\n";
	if (m_Move != 0)
	{
		Entry += TabLvl3 + "// FightMove\n";
		Entry += TabLvl3 + "*FightMove(" + GenerateFightMove(m_Move) + "," + GetMoveStanceStr(m_Stance) + ")\n";
	}
	Entry += TabLvl2 + "}\n\n" + TabLvl2 + "*ConditionSet0\n" + TabLvl2 + "{\n";
	Entry += TabLvl3 + "*\"StateTime = 0\"\n" + TabLvl2 + "}\n";
	Entry += _TabStr + "}\n";

	return Entry;
}

CStr CAGFightEntry::GenerateAttackPhase(CStr _TabStr)
{
	CStr Entry;
	CStr TabLvl2 = _TabStr + "\t";
	CStr TabLvl3 = TabLvl2 + "\t";
	CStr Stance = StringFromStance(m_Stance);

	// First attackphase action
	Entry += _TabStr + "// ATTACKPHASE" +"\n" + _TabStr + "*Action1\n" + _TabStr + "{\n";
	Entry += TabLvl2 + "*Effects\n" + TabLvl2 + "{\n" + TabLvl3 + "// FIGHTSTATUS TAG\n";
	Entry += TabLvl3 + "*SetFightStatus(" + GenerateFightStatus(m_FightStatus & AG_FIGHTMODE_ATTACKPHASEMASK) + ")\n";
	Entry += TabLvl2 + "}\n\n" + TabLvl2 + "*ConditionSet0\n" + TabLvl2 + "{\n";
	Entry += TabLvl3 + "*\"StateTimeOffset = FightMode_" + Stance + "_" + m_ActionName + "_PrePhaseTime\"\n" + TabLvl2 + "}\n";
	Entry += _TabStr + "}\n\n";
	
	// Second attackphase action
	if (m_Damage != 0)
	{
		Entry += _TabStr + "*Action2\n" + _TabStr + "{\n";;
		Entry += TabLvl2 + "*Effects\n" + TabLvl2 + "{\n" + TabLvl3 + "// Send damage and drain stamina (if any)\n";
		Entry += TabLvl3 + "*SendFightDamage(" + GenerateFightDamage(m_Damage) + "," + GenerateAttackType(m_AttackType) + ")\n";
		if (m_OpponentStamina != 0)
			Entry += TabLvl3 + "*DrainStamina(" + GenerateStamina(m_OpponentStamina, AG_FIGHTMODE_STAMINATARGET_OPPONENT) + ")\n";
		Entry += TabLvl2 + "}\n\n" + TabLvl2 + "*ConditionSet0\n" + TabLvl2 + "{\n";
		Entry += TabLvl3 + "*\"StateTimeOffset = FightMode_" + Stance + "_" + m_ActionName + "_AttackPhaseTime\"\n" + TabLvl2 + "}\n";
		Entry += _TabStr + "}\n\n";
	}

	return Entry;
}

CStr CAGFightEntry::GeneratePostPhase(CStr _TabStr)
{
	CStr Entry;
	CStr TabLvl2 = _TabStr + "\t";
	CStr TabLvl3 = TabLvl2 + "\t";
	CStr Stance = StringFromStance(m_Stance);

	Entry += _TabStr + "// POSTPHASE" +"\n" + _TabStr + "*Action3\n" + _TabStr + "{\n";
	Entry += TabLvl2 + "*Effects\n" + TabLvl2 + "{\n" + TabLvl3 + "// FIGHTSTATUS TAG\n";
	Entry += TabLvl3 + "*SetFightStatus(" + GenerateFightStatus(m_FightStatus & AG_FIGHTMODE_POSTPHASEMASK) + ")\n";
	Entry += TabLvl2 + "}\n\n" + TabLvl2 + "*ConditionSet0\n" + TabLvl2 + "{\n";
	Entry += TabLvl3 + "*\"StateTimeOffset = FightMode_" + Stance + "_" + m_ActionName + "_PostPhaseTime\"\n" + TabLvl2 + "}\n";
	Entry += _TabStr + "}\n";

	return Entry;
}

CStr CAGFightEntry::GenerateFightStatus(int _FightStatus)
{
	CStr Entry;

	if (_FightStatus & AG_FIGHTMODE_ATTACKACTIVE)
		Entry += "FIGHTMODE_ATTACKACTIVE,";
	if (_FightStatus & AG_FIGHTMODE_HURTACTIVE)
		Entry += "FIGHTMODE_HURTACTIVE,";
	if (_FightStatus & AG_FIGHTMODE_CANINTERRUPT)
		Entry += "FIGHTMODE_CANINTERRUPT,";
	if (_FightStatus & AG_FIGHTMODE_CANBLOCK)
		Entry += "FIGHTMODE_CANBLOCK,";
	if (_FightStatus & AG_FIGHTMODE_CANGUARD)
		Entry += "FIGHTMODE_CANGUARD,";
	if (_FightStatus & AG_FIGHTMODE_CANDODGEBACK)
		Entry += "FIGHTMODE_CANDODGEBACK,";
	if (_FightStatus & AG_FIGHTMODE_CANDODGELEFT)
		Entry += "FIGHTMODE_CANDODGELEFT,";
	if (_FightStatus & AG_FIGHTMODE_CANDODGERIGHT)
		Entry += "FIGHTMODE_CANDODGERIGHT,";

	if (_FightStatus == 0)
		Entry = "FIGHTMODE_INACTIVE,";
	CStr Return;
	if (Entry.Len() > 0)
		Return = Entry.SubStr(0, Entry.Len() - 1);
	
	return Return;
}

/*AG_FIGHTMODE_DAMAGELIGHT			= 1,
	AG_FIGHTMODE_DAMAGEMEDIUM			= 2,
	AG_FIGHTMODE_DAMAGEHEAVY			= 3,*/
CStr CAGFightEntry::GenerateFightDamage(int _Damage)
{
	switch (_Damage)
	{
	case AG_FIGHTMODE_DAMAGELIGHT:
		return "FIGHTMODE_DAMAGELIGHT";
	case AG_FIGHTMODE_DAMAGEMEDIUM:
		return "FIGHTMODE_DAMAGEMEDIUM";
	case AG_FIGHTMODE_DAMAGEHEAVY:
		return "FIGHTMODE_DAMAGEHEAVY";
	default:
		return "DAMAGE_ERROR";
	}
}
CStr CAGFightEntry::GenerateStamina(int _Stamina, int _Target)
{
	CStr Stamina;

	if (_Stamina == AG_FIGHTMODE_STAMINADRAIN_LOW)
		Stamina = "FIGHTMODE_STAMINADRAINLOW";
	else if (_Stamina == AG_FIGHTMODE_STAMINADRAIN_MEDIUM)
		Stamina = "FIGHTMODE_STAMINADRAINMEDIUM";
	else if (_Stamina == AG_FIGHTMODE_STAMINADRAIN_HIGH)
		Stamina = "FIGHTMODE_STAMINADRAINHIGH";
	else 
		Stamina = "STAMINA_ERROR";
	
	if (_Target == AG_FIGHTMODE_STAMINATARGET_SELF)
		Stamina += ",FIGHTMODE_STAMINATARGET_SELF";
	else
		Stamina += ",FIGHTMODE_STAMINATARGET_OPPONENT";

	return Stamina;
}

CStr CAGFightEntry::GenerateFightMove(int _Move)
{
	switch (_Move)
	{
	case FIGHT_MOVE_LEFTFORWARDSMALL:
		return "FIGHT_MOVE_LEFTFORWARDSMALL";
	case FIGHT_MOVE_LEFTBACKWARDSMALL:
		return "FIGHT_MOVE_LEFTBACKWARDSMALL";
	case FIGHT_MOVE_LEFTNEUTRALSTRAFELEFT:
		return "FIGHT_MOVE_LEFTNEUTRALSTRAFELEFT";
	case FIGHT_MOVE_LEFTNEUTRALSTRAFERIGHT:
		return "FIGHT_MOVE_LEFTNEUTRALSTRAFERIGHT";
	case FIGHT_MOVE_LEFTNEUTRALFORWARDLARGE:
		return "FIGHT_MOVE_LEFTNEUTRALFORWARDLARGE";
	case FIGHT_MOVE_LEFTSAFEFORWARDLARGE:
		return "FIGHT_MOVE_LEFTSAFEFORWARDLARGE";
	case FIGHT_MOVE_LEFTNEUTRALFORWARDLEFT:
		return "FIGHT_MOVE_LEFTNEUTRALFORWARDLEFT";
	case FIGHT_MOVE_LEFTNEUTRALFORWARDRIGHT:
		return "FIGHT_MOVE_LEFTNEUTRALFORWARDRIGHT";
	case FIGHT_MOVE_LEFTNEUTRALBACKWARDLARGE:
		return "FIGHT_MOVE_LEFTNEUTRALBACKWARDLARGE";
	case FIGHT_MOVE_LEFTCLOSEBACKWARDLARGE:
		return "FIGHT_MOVE_LEFTCLOSEBACKWARDLARGE";
	case FIGHT_MOVE_LEFTCLOSEDODGELEFT:
		return "FIGHT_MOVE_LEFTCLOSEDODGELEFT";
	case FIGHT_MOVE_LEFTCLOSEDODGERIGHT:
		return "FIGHT_MOVE_LEFTCLOSEDODGERIGHT";
	// Right
	case FIGHT_MOVE_RIGHTFORWARDSMALL:
		return "FIGHT_MOVE_RIGHTFORWARDSMALL";
	case FIGHT_MOVE_RIGHTBACKWARDSMALL:
		return "FIGHT_MOVE_RIGHTBACKWARDSMALL";
	case FIGHT_MOVE_RIGHTNEUTRALSTRAFERIGHT:
		return "FIGHT_MOVE_RIGHTNEUTRALSTRAFERIGHT";
	case FIGHT_MOVE_RIGHTNEUTRALSTRAFELEFT:
		return "FIGHT_MOVE_RIGHTNEUTRALSTRAFELEFT";
	case FIGHT_MOVE_RIGHTNEUTRALFORWARDLARGE:
		return "FIGHT_MOVE_RIGHTNEUTRALFORWARDLARGE";
	case FIGHT_MOVE_RIGHTSAFEFORWARDLARGE:
		return "FIGHT_MOVE_RIGHTSAFEFORWARDLARGE";
	case FIGHT_MOVE_RIGHTNEUTRALFORWARDRIGHT:
		return "FIGHT_MOVE_RIGHTNEUTRALFORWARDRIGHT";
	case FIGHT_MOVE_RIGHTNEUTRALFORWARDLEFT:
		return "FIGHT_MOVE_RIGHTNEUTRALFORWARDLEFT";
	case FIGHT_MOVE_RIGHTNEUTRALBACKWARDLARGE:
		return "FIGHT_MOVE_RIGHTNEUTRALBACKWARDLARGE";
	case FIGHT_MOVE_RIGHTCLOSEBACKWARDLARGE:
		return "FIGHT_MOVE_RIGHTCLOSEBACKWARDLARGE";
	case FIGHT_MOVE_RIGHTCLOSEDODGELEFT:
		return "FIGHT_MOVE_RIGHTCLOSEDODGELEFT";
	case FIGHT_MOVE_RIGHTCLOSEDODGERIGHT:
		return "FIGHT_MOVE_RIGHTCLOSEDODGERIGHT";
	default:
		return "FIGHT_MOVE_NONE";
	}
}

int CAGFightEntry::ResolveMove(CStr _Str)
{
	CStr _Upper = _Str.UpperCase();

	// Left
	if (_Upper.Find("LEFTFORWARDSMALL") != -1)
		return FIGHT_MOVE_LEFTFORWARDSMALL;
	else if (_Upper.Find("LEFTBACKWARDSMALL") != -1)
		return FIGHT_MOVE_LEFTBACKWARDSMALL;
	else if (_Upper.Find("LEFTNEUTRALSTRAFELEFT") != -1)
		return FIGHT_MOVE_LEFTNEUTRALSTRAFELEFT;
	else if (_Upper.Find("LEFTNEUTRALSTRAFERIGHT") != -1)
		return FIGHT_MOVE_LEFTNEUTRALSTRAFERIGHT;
	else if (_Upper.Find("LEFTNEUTRALFORWARDLARGE") != -1)
		return FIGHT_MOVE_LEFTNEUTRALFORWARDLARGE;
	else if (_Upper.Find("LEFTSAFEFORWARDLARGE") != -1)
		return FIGHT_MOVE_LEFTSAFEFORWARDLARGE;
	else if (_Upper.Find("LEFTNEUTRALFORWARDLEFT") != -1)
		return FIGHT_MOVE_LEFTNEUTRALFORWARDLEFT;
	else if (_Upper.Find("LEFTNEUTRALFORWARDRIGHT") != -1)
		return FIGHT_MOVE_LEFTNEUTRALFORWARDRIGHT;
	else if (_Upper.Find("LEFTNEUTRALBACKWARDLARGE") != -1)
		return FIGHT_MOVE_LEFTNEUTRALBACKWARDLARGE;
	else if (_Upper.Find("LEFTCLOSEBACKWARDLARGE") != -1)
		return FIGHT_MOVE_LEFTCLOSEBACKWARDLARGE;
	else if (_Upper.Find("LEFTCLOSEDODGELEFT") != -1)
		return FIGHT_MOVE_LEFTCLOSEDODGELEFT;
	else if (_Upper.Find("LEFTCLOSEDODGERIGHT") != -1)
		return FIGHT_MOVE_LEFTCLOSEDODGERIGHT;
	// Right
	else if (_Upper.Find("RIGHTFORWARDSMALL") != -1)
		return FIGHT_MOVE_RIGHTFORWARDSMALL;
	else if (_Upper.Find("RIGHTBACKWARDSMALL") != -1)
		return FIGHT_MOVE_RIGHTBACKWARDSMALL;
	else if (_Upper.Find("RIGHTNEUTRALSTRAFERIGHT") != -1)
		return FIGHT_MOVE_RIGHTNEUTRALSTRAFERIGHT;
	else if (_Upper.Find("RIGHTNEUTRALSTRAFELEFT") != -1)
		return FIGHT_MOVE_RIGHTNEUTRALSTRAFELEFT;
	else if (_Upper.Find("RIGHTNEUTRALFORWARDLARGE") != -1)
		return FIGHT_MOVE_RIGHTNEUTRALFORWARDLARGE;
	else if (_Upper.Find("RIGHTSAFEFORWARDLARGE") != -1)
		return FIGHT_MOVE_RIGHTSAFEFORWARDLARGE;
	else if (_Upper.Find("RIGHTNEUTRALFORWARDRIGHT") != -1)
		return FIGHT_MOVE_RIGHTNEUTRALFORWARDRIGHT;
	else if (_Upper.Find("RIGHTNEUTRALFORWARDLEFT") != -1)
		return FIGHT_MOVE_RIGHTNEUTRALFORWARDLEFT;
	else if (_Upper.Find("RIGHTNEUTRALBACKWARDLARGE") != -1)
		return FIGHT_MOVE_RIGHTNEUTRALBACKWARDLARGE;
	else if (_Upper.Find("RIGHTCLOSEBACKWARDLARGE") != -1)
		return FIGHT_MOVE_RIGHTCLOSEBACKWARDLARGE;
	else if (_Upper.Find("RIGHTCLOSEDODGELEFT") != -1)
		return FIGHT_MOVE_RIGHTCLOSEDODGELEFT;
	else if (_Upper.Find("RIGHTCLOSEDODGERIGHT") != -1)
		return FIGHT_MOVE_RIGHTCLOSEDODGERIGHT;
	else
		return FIGHT_MOVE_NONE;
}

int CAGFightEntry::ResolveFightStatus(CStr _Str)
{
	CStr Upper = _Str.UpperCase();
	
	if (Upper.Find("ATTACKACTIVE") != -1)
		return AG_FIGHTMODE_ATTACKACTIVE;
	else if (Upper.Find("HURTACTIVE") != -1)
		return AG_FIGHTMODE_HURTACTIVE;
	else if (Upper.Find("CANINTERRUPT") != -1)
		return AG_FIGHTMODE_CANINTERRUPT;
	else if (Upper.Find("CANBLOCK") != -1)
		return AG_FIGHTMODE_CANBLOCK;
	else if (Upper.Find("CANGUARD") != -1)
		return AG_FIGHTMODE_CANGUARD;
	else if (Upper.Find("CANDONORMALMOVE") != -1)
		return AG_FIGHTMODE_CANDONORMALMOVE;
	else if (Upper.Find("CANATTACK") != -1)
		return AG_FIGHTMODE_CANATTACK;
	else if (Upper.Find("CANDODGEBACK") != -1)
		return AG_FIGHTMODE_CANDODGEBACK;
	else if (Upper.Find("CANDODGELEFT") != -1)
		return AG_FIGHTMODE_CANDODGELEFT;
	else if (Upper.Find("CANDODGERIGHT") != -1)
		return AG_FIGHTMODE_CANDODGERIGHT;

	return 0;
}

int CAGFightEntry::ResolveDamage(CStr _Str)
{
	CStr Upper = _Str.UpperCase();

	if (Upper.Find("LIGHT") != -1)
		return AG_FIGHTMODE_DAMAGELIGHT;
	else if (Upper.Find("MEDIUM") != -1)
		return AG_FIGHTMODE_DAMAGEMEDIUM;
	if (Upper.Find("HEAVY") != -1)
		return AG_FIGHTMODE_DAMAGEHEAVY;

	return 0;
}

int CAGFightEntry::ResolveStamina(CStr _Str)
{
	CStr Upper = _Str.UpperCase();

	if (Upper.Find("LOW") != -1)
		return AG_FIGHTMODE_STAMINADRAIN_LOW;
	else if (Upper.Find("MEDIUM") != -1)
		return AG_FIGHTMODE_STAMINADRAIN_MEDIUM;
	else if (Upper.Find("HIGH") != -1)
		return AG_FIGHTMODE_STAMINADRAIN_HIGH;

	return 0;
}

int CAGFightEntry::ResolveAttackType(CStr _AttackType)
{
	CStr Upper = _AttackType.UpperCase();

	if (Upper.Find("HAND") != -1)
		return AG_FIGHTMODE_SOUNDTYPE_HAND;
	else if (Upper.Find("KNEE") != -1)
		return AG_FIGHTMODE_SOUNDTYPE_KNEE;
	else if (Upper.Find("KICK") != -1)
		return AG_FIGHTMODE_SOUNDTYPE_KICK;

	return 0;
}

CStr CAGFightEntry::GenerateAttackType(int _AttackType)
{
	switch (_AttackType)
	{
	case AG_FIGHTMODE_SOUNDTYPE_HAND:
		return "AG_FIGHTMODE_SOUNDTYPE_HAND";
	case AG_FIGHTMODE_SOUNDTYPE_KNEE:
		return "AG_FIGHTMODE_SOUNDTYPE_KNEE";
	case AG_FIGHTMODE_SOUNDTYPE_KICK:
		return "AG_FIGHTMODE_SOUNDTYPE_KICK";
	default:
		return "ATTACKTYPE_ERROR";
	}
}

int CAGFightEntry::ResolveStanceFromString(CStr _Stance)
{
	CStr Upper = _Stance.UpperCase();

	if ((Upper.Find("LEFTSAFE") != -1) || (Upper.Find("SAFELEFT") != -1))
	{
		return FIGHT_STANCE_LEFTSAFE;
	}
	else if ((Upper.Find("LEFTNEUTRAL") != -1) || (Upper.Find("NEUTRALLEFT") != -1))
	{
		return FIGHT_STANCE_LEFTNEUTRAL;
	}
	else if ((Upper.Find("LEFTCLOSE") != -1) || (Upper.Find("CLOSELEFT") != -1))
	{
		return FIGHT_STANCE_LEFTCLOSE;
	}
	else if ((Upper.Find("RIGHTSAFE") != -1) || (Upper.Find("SAFERIGHT") != -1))
	{
		return FIGHT_STANCE_RIGHTSAFE;
	}
	else if ((Upper.Find("RIGHTNEUTRAL") != -1) || (Upper.Find("NEUTRALRIGHT") != -1))
	{
		return FIGHT_STANCE_RIGHTNEUTRAL;
	}
	else if ((Upper.Find("RIGHTCLOSE") != -1) || (Upper.Find("CLOSERIGHT") != -1))
	{
		return FIGHT_STANCE_RIGHTCLOSE;
	}

	return 0;
}

CStr CAGFightEntry::StringFromStance(int _Stance)
{
	switch (_Stance)
	{
	case FIGHT_STANCE_LEFTSAFE:
		return "SafeLeft";
	case FIGHT_STANCE_RIGHTSAFE:
		return "SafeLeft";
	case FIGHT_STANCE_LEFTNEUTRAL:
		return "NeutralLeft";
	case FIGHT_STANCE_RIGHTNEUTRAL:
		return "NeutralRight";
	case FIGHT_STANCE_LEFTCLOSE:
		return "CloseLeft";
	case FIGHT_STANCE_RIGHTCLOSE:
		return "CloseRight";
	default:
		return "StanceError";
	};
}

CStr CAGFightEntry::GetMoveStanceStr(int _Stance)
{
	switch (_Stance)
	{
	case FIGHT_STANCE_LEFTSAFE:
		return "FIGHT_STANCE_LEFTSAFE";
	case FIGHT_STANCE_RIGHTSAFE:
		return "FIGHT_STANCE_RIGHTSAFE";
	case FIGHT_STANCE_LEFTNEUTRAL:
		return "FIGHT_STANCE_LEFTNEUTRAL";
	case FIGHT_STANCE_RIGHTNEUTRAL:
		return "FIGHT_STANCE_RIGHTNEUTRAL";
	case FIGHT_STANCE_LEFTCLOSE:
		return "FIGHT_STANCE_LEFTCLOSE";
	case FIGHT_STANCE_RIGHTCLOSE:
		return "FIGHT_STANCE_RIGHTCLOSE";
	default:
		return "STANCE_ERROR";
	};
}

CStr CAGFightEntry::StringFromFightStatus(int _FightStatus)
{
	CStr Result;
	if (_FightStatus & AG_FIGHTMODE_ATTACKACTIVE)
		Result += "AttackActive/";
	if (_FightStatus & AG_FIGHTMODE_HURTACTIVE)
		Result += "HurtActive/";
	if (_FightStatus & AG_FIGHTMODE_CANINTERRUPT)
		Result += "CanInterrupt/";
	if (_FightStatus & AG_FIGHTMODE_CANBLOCK)
		Result += "CanBlock/";
	if (_FightStatus & AG_FIGHTMODE_CANGUARD)
		Result += "CanGuard/";
	if (_FightStatus & AG_FIGHTMODE_CANDODGEBACK)
		Result += "CanDodgeBack/";
	if (_FightStatus & AG_FIGHTMODE_CANDODGELEFT)
		Result += "CanDodgeLeft/";
	if (_FightStatus & AG_FIGHTMODE_CANDODGERIGHT)
		Result += "CanDodgeRight";

	return Result;
}

/*
 			*Action0
				{
					*MoveToken
					{
						*TargetState FightMode_CloseLeft_StomachKneeLeft
						*AnimBlendDuration FightMode_AttackBlendDuration
						*AnimTimeOffset FightMode_KneeTimeOffset
					}
					
					*ConditionSet0
					{
						*"FightMoveTest = FixedMoveAngle_Left90"
						*"LeftTrigger > FightMode_TriggerThreshold"
						*"RightTrigger > FightMode_TriggerThreshold"
						*"IsDoneBlending"
					}
				}
 */

CStr CAGFightEntry::GenerateActionString(CStr _TabStr, CStr _PostPhaseTime)
{
	CStr Entry;
	CStr TabLvl2 = _TabStr + "\t";
	CStr TabLvl3 = TabLvl2 + "\t";
	CStr Stance = StringFromStance(m_Stance);
	// Blend duration is the preattack time, here we won't have any offset...
	CStr BlendDuration = CStrF("%f", m_PreAttackTime);

	Entry += _TabStr + "// Some Attack" +"\n" + _TabStr + "*Action999\n" + _TabStr + "{\n";
	Entry += TabLvl2 + "*MoveToken\n" + TabLvl2 + "{\n" + TabLvl3 + "// FIGHTSTATUS TAG\n";
	Entry += TabLvl3 + "*TargetState " + "FightMode_" + Stance + "_" + m_ActionName + "\n";
	Entry += TabLvl3 + "*AnimBlendDuration " + "FightMode_" + Stance + "_" + m_ActionName + "_PrePhaseTime\n" + TabLvl2 + "}\n\n";

	// Register input
	if (m_bRegisterInput)
	{
		Entry += TabLvl2 + "*Effects\n" + TabLvl2 + "{\n";
		Entry += GenerateRegisterInput(m_Input, TabLvl3);
		Entry += TabLvl2 + "}\n\n";
	}
	
	// ConditionSet
	Entry += TabLvl2 + "*ConditionSet0\n" + TabLvl2 + "{\n";
	Entry += GenerateInput(m_Input, TabLvl3);
	Entry += TabLvl3 + "*\"StateTimeOffset > " + _PostPhaseTime + "\"\n";
	Entry += TabLvl3 + "*\"CheckFightStatus(FIGHTMODE_CANATTACK)\"\n" + TabLvl2 + "}\n";
	Entry += _TabStr + "}\n";

	return Entry;
}

CStr CAGFightEntry::GenerateInput(int _Input, CStr _TabStr)
{
	// Generate input text block from the input
	CStr Entry;

	for (int i = 0; i < AG_FIGHTMODE_NUMINPUTTYPES; i++)
	{
		bool bNotUsed = (i < AG_FIGHTMODE_MOVEFWD ? true : false );
		int Input = 1 << i;
		if (_Input & Input)
			Entry += _TabStr + GenerateInputTestString(Input, bNotUsed);
	}

	return Entry;
}
#define AG_FIGHTMODE_REGINPUTMASK (CONTROLBITS_PRIMARY | CONTROLBITS_SECONDARY |CONTROLBITS_JUMP | CONTROLBITS_BUTTON0 | CONTROLBITS_BUTTON1 |CONTROLBITS_BUTTON2 | CONTROLBITS_BUTTON3)
CStr CAGFightEntry::GenerateRegisterInput(int _Input, CStr _TabStr)
{
	// Generate input text block from the input
	CStr Entry;

	for (int i = 0; i < AG_FIGHTMODE_NUMINPUTTYPES; i++)
	{
		bool bNotUsed = (i < AG_FIGHTMODE_MOVEFWD ? true : false );
		int Input = 1 << i;
		if ((_Input & Input) & AG_FIGHTMODE_REGINPUTMASK)
			Entry += _TabStr + GenerateRegisterInputString(Input, bNotUsed);
	}

	return Entry;
}

int CAGFightEntry::InputFromString(CStr _Str)
{
	CStr Upper = _Str.UpperCase();

	if (Upper.Find("PRIMARY") != -1)
		return CONTROLBITS_PRIMARY;
	else if (Upper.Find("SECONDARY") != -1)
		return CONTROLBITS_SECONDARY;
	else if (Upper.Find("JUMP") != -1)
		return CONTROLBITS_JUMP;
	else if (Upper.Find("CROUCH") != -1)
		return CONTROLBITS_CROUCH;
	else if ((Upper.Find("BUTTON0") != -1) || (Upper.Find("BUTTON_A") != -1))
		return CONTROLBITS_BUTTON0;
	else if ((Upper.Find("BUTTON1") != -1) || (Upper.Find("BUTTON_B") != -1))
		return CONTROLBITS_BUTTON1;
	else if ((Upper.Find("BUTTON2") != -1)  || (Upper.Find("BUTTON_X") != -1))
		return CONTROLBITS_BUTTON2;
	else if ((Upper.Find("BUTTON3") != -1) || (Upper.Find("BUTTON_Y") != -1))
		return CONTROLBITS_BUTTON3;
	// Move 
	else if ((Upper.Find("LTRIGGER") != -1) || (Upper.Find("LEFTTRIGGER") != -1))
		return AG_FIGHTMODE_LTRIGGER;
	else if ((Upper.Find("RTRIGGER") != -1) || (Upper.Find("RIGHTTRIGGER") != -1))
		return AG_FIGHTMODE_RTRIGGER;
	else if (Upper.Find("MOVEFWD") != -1)
		return AG_FIGHTMODE_MOVEFWD;
	else if (Upper.Find("MOVEBWD") != -1)
		return AG_FIGHTMODE_MOVEBWD;
	else if (Upper.Find("MOVELEFT45") != -1)
		return AG_FIGHTMODE_MOVELEFT45;
	else if (Upper.Find("MOVELEFT90") != -1)
		return AG_FIGHTMODE_MOVELEFT90;
	else if (Upper.Find("MOVERIGHT45") != -1)
		return AG_FIGHTMODE_MOVERIGHT45;
	else if (Upper.Find("MOVERIGHT90") != -1)
		return AG_FIGHTMODE_MOVERIGHT90;

	return 0;	
}

// Should only have one input at a time (matches the first one it finds)
CStr CAGFightEntry::GenerateInputTestString(int _Input, bool bNotUsed)
{
	CStr Result;

	if (_Input & CONTROLBITS_PRIMARY)
		Result = "*ButtonTest(BUTTON_PRIMARY";
	else if (_Input & CONTROLBITS_SECONDARY)
		Result =  "*ButtonTest(BUTTON_SECONDARY";
	else if (_Input & CONTROLBITS_JUMP)
		Result = "*ButtonTest(BUTTON_JUMP";
	else if (_Input & CONTROLBITS_CROUCH)
		Result = "*ButtonTest(BUTTON_CROUCH";
	else if (_Input & CONTROLBITS_BUTTON0)
		Result = "*ButtonTest(BUTTON_A";
	else if (_Input & CONTROLBITS_BUTTON1)
		Result = "*ButtonTest(BUTTON_B";
	else if (_Input & CONTROLBITS_BUTTON2)
		Result = "*ButtonTest(BUTTON_X";
	else if (_Input & CONTROLBITS_BUTTON3)
		Result = "*ButtonTest(BUTTON_Y";

	if (Result.Len() > 0)
	{
		if (bNotUsed)
			Result += ",BUTTONTEST_NOTUSED)\n";
		else
			Result += ")";

		return Result;
	}
	
	// Triggers
	if (_Input & AG_FIGHTMODE_LTRIGGER)
		Result = "*\"LeftTrigger > FightMode_TriggerThreshold\"\n";
	else if (_Input & AG_FIGHTMODE_RTRIGGER)
		Result = "*\"RightTrigger > FightMode_TriggerThreshold\"\n";
	// Move 
	else if (_Input & AG_FIGHTMODE_MOVEFWD)
		Result = "*\"FightMoveTest = FixedMoveAngle_Fwd\"\n";
	else if (_Input & AG_FIGHTMODE_MOVEBWD)
		Result = "*\"FightMoveTest = FixedMoveAngle_Bwd\"\n";
	else if (_Input & AG_FIGHTMODE_MOVELEFT45)
		Result = "*\"FightMoveTest = FixedMoveAngle_Left45\"\n";
	else if (_Input & AG_FIGHTMODE_MOVELEFT90)
		Result = "*\"FightMoveTest = FixedMoveAngle_Left90\"\n";
	else if (_Input & AG_FIGHTMODE_MOVERIGHT45)
		Result = "*\"FightMoveTest = FixedMoveAngle_Right45\"\n";
	else if (_Input & AG_FIGHTMODE_MOVERIGHT90)
		Result = "*\"FightMoveTest = FixedMoveAngle_Right90\"\n";

	return Result;
}

// Should only have one input at a time (matches the first one it finds)
CStr CAGFightEntry::GenerateRegisterInputString(int _Input, bool bNotUsed)
{
	CStr Result;

	if (_Input & CONTROLBITS_PRIMARY)
		Result = "*RegisterUsedInput(BUTTON_PRIMARY)\n";
	else if (_Input & CONTROLBITS_SECONDARY)
		Result =  "*RegisterUsedInput(BUTTON_SECONDARY)\n";
	else if (_Input & CONTROLBITS_JUMP)
		Result = "*RegisterUsedInput(BUTTON_JUMP)\n";
	else if (_Input & CONTROLBITS_CROUCH)
		Result = "*RegisterUsedInput(BUTTON_CROUCH)\n";
	else if (_Input & CONTROLBITS_BUTTON0)
		Result = "*RegisterUsedInput(BUTTON_A)\n";
	else if (_Input & CONTROLBITS_BUTTON1)
		Result = "*RegisterUsedInput(BUTTON_B)\n";
	else if (_Input & CONTROLBITS_BUTTON2)
		Result = "*RegisterUsedInput(BUTTON_X)\n";
	else if (_Input & CONTROLBITS_BUTTON3)
		Result = "*RegisterUsedInput(BUTTON_Y)\n";

	return Result;
}


CStr CAGFightEntryManager::GenerateConstantList(CStr _TabStr)
{
	// Go through every entry and get the constants
	CStr Entry;
	int Len = m_FightEntries.Len();

	Entry += _TabStr + "*Constants\n" + _TabStr + "{\n";
	
	CStr TabLvl2 = _TabStr + "\t";
	for (int i = 0; i < Len; i++)
	{
		Entry += m_FightEntries[i].GetConstants(TabLvl2);
	}
	
	Entry += _TabStr + "}\n";

	return Entry;
}

CStr CAGFightEntryManager::GenerateActionList(int _Stance, CStr _TabStr, CStr _PostPhaseTime)
{
	// Go through the list and add every state that matches the stance
	CStr Entry;
	int Len = m_FightEntries.Len();
	for (int i = 0; i < Len; i++)
	{
		if (m_FightEntries[i].GetStance() == _Stance)
			Entry += m_FightEntries[i].GenerateActionString(_TabStr, _PostPhaseTime);
	}

	return Entry;
}

bool CAGFightEntryManager::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	if (_pKey->GetThisName().Find("ENTRY") != -1)
	{
		int NumChildren = _pKey->GetNumChildren();

		CAGFightEntry Entry;

		for (int i = 0; i < NumChildren; i++)
		{
			Entry.OnEvalKey(_pKey->GetChild(i));
		}
		
		m_FightEntries.Add(Entry);
	}

	return true;
}

void CAGFightEntryManager::WriteStatesToFile(CStr _FileName)
{
	CCFile Debug;
	Debug.Open("S:\\FightModeConstants.xrg", CFILE_WRITE);
	
	/*CAGFightEntry Hej("LeftPunch", FIGHT_STANCE_LEFTSAFE, 0, FIGHT_STANCE_LEFTCLOSE, FightStatus, 
		AG_FIGHTMODE_DAMAGELIGHT, AG_FIGHTMODE_STAMINADRAIN_LOW,0, 0,0,0);
	CStr Mogg = Hej.GenerateState("\t", NULL);*/
	CStr Temp;
	int Len = m_FightEntries.Len();
	
	Temp = GenerateConstantList("\t\t");
	Debug.WriteLE((uint8*)Temp.GetStr(), Temp.Len());
	// Close the open file
	Debug.Close();

	// Write in stance order
	for (int j = 0; j < FIGHT_STANCE_NUMBEROFSTANCES; j++)
	{
		int Stance = 1 << j;
		// Open file
		CStr FileName = CStrF("S:\\%s.xrg",CAGFightEntry::StringFromStance(Stance).GetStr());
		Debug.Open(FileName, CFILE_WRITE);

		CStr StanceStr = CStrF("\n//!! WRITING STANCE: %s !! ////////////////////////////\n", 
			CAGFightEntry::StringFromStance(Stance).GetStr());
		Debug.WriteLE((uint8*)StanceStr.GetStr(), StanceStr.Len());
		for (int i = 0; i < Len; i++)
		{
			if (m_FightEntries[i].GetStance() == Stance)
			{
				Temp = m_FightEntries[i].GenerateState("\t\t", this);
				Debug.WriteLE((uint8*)Temp.GetStr(), Temp.Len());
			}
		}
		Debug.Close();
	}
}
#endif
