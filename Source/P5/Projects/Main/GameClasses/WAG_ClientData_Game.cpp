//--------------------------------------------------------------------------------

#include "PCH.h"

#include "WObj_Char.h"
#include "WObj_CharMsg.h"
#include "WAG_ClientData_Game.h"
#include "../GameWorld/WClientMod_Defines.h"
#include "WObj_Misc/WObj_Ledge.h"
#include "WObj_AI/AI_Def.h"
#include "WObj_AI/AICore.h"
#include "WRPG/WRPGFist.h"
#include "WRPG/WRPGChar.h"
//--------------------------------------------------------------------------------

CFightDamageQueue::CFightDamageQueue()
{
	m_Tick = -1;
	m_DamageType = 0;
}
void CFightDamageQueue::Create(int32 _Tick, int32 _DamageType)
{
	m_Tick = _Tick;
	m_DamageType = _DamageType;
}
void CFightDamageQueue::Refresh(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, int32 _CurrentTick)
{
	if (m_Tick < 0)
		return;

	// CHECK IF STILL IN STATE
	if (m_Tick <= _CurrentTick)
	{
		CWObject_Character::Char_FightDamage(_pWPhys,_pObj,m_DamageType);
		Invalidate();
	}
}

void CWO_Clientdata_Character_AnimGraph::AG_RefreshStateInstanceProperties(const CWAGI_Context* _pContext, const CWAGI_StateInstance* _pStateInstance)
{
	CWO_ClientData_AnimGraphInterface::AG_RefreshStateInstanceProperties(_pContext, _pStateInstance);
}

#define AG_DEFAULTIDLETURNTHRESHOLD (90)
void CWO_Clientdata_Character_AnimGraph::AG_OnEnterState(const CWAGI_Context* _pContext, CAGTokenID _TokenID, CAGStateIndex _iState, CAGActionIndex _iEnterAction)
{
	// Get state flags and state constants that are useful to us
	m_ImportantAGEvent = 0;
	if (_TokenID == AG_TOKEN_MAIN)
	{
		//ConOut(CStrF("%s OnEnterstate, iState: %d iAction: %d",(_pContext->m_pWPhysState->IsServer() ? "Server" : "Client"),_iState, _iEnterAction));
		CWAGI* pAGI = GetAGI();

		if (!pAGI || !pAGI->AcquireAllResources(_pContext))
			return;

		const CXRAG_State* pState = pAGI->GetState(_iState);
		if (pState)
		{
			m_StateFlagsLo = pState->GetFlags(0);
			m_StateFlagsHi = pState->GetFlags(1);
			m_AnimphysMoveType = (uint8)pAGI->GetStateConstantValueDef(pState,AG_CONSTANT_ANIMPHYSMOVETYPE,0.0f);
			m_ForcedAimingType = (int8)pAGI->GetStateConstantValueDef(pState,AG_CONSTANT_AIMINGTYPE,-1.0f);
			m_IdleTurnThreshold = (uint8)pAGI->GetStateConstantValueDef(pState,AG_CONSTANT_TURNTHRESHOLD,AG_DEFAULTIDLETURNTHRESHOLD);
			m_CanCounterType = (uint8)pAGI->GetStateConstantValueDef(pState,AG_CONSTANT_CANCOUNTER,0);
			m_CurrentAttack = (uint8)pAGI->GetStateConstantValueDef(pState,AG_CONSTANT_CURRENTATTACK,0);
			m_ConstantMaxLookZ = (uint8)pAGI->GetStateConstantValueDef(pState,AG_CONSTANT_MAXLOOKANGLEZ,0);
			m_ConstantMaxLookY = (uint8)pAGI->GetStateConstantValueDef(pState,AG_CONSTANT_MAXLOOKANGLEY,0);
			m_MaxBodyOffset = (uint8)pAGI->GetStateConstantValueDef(pState,AG_CONSTANT_MAXBODYOFFSET,160.0f);
		}
		pAGI->UnacquireAllResources();
	}
	else if (_TokenID == AG_TOKEN_EFFECT)
	{
		CWAGI* pAGI = GetAGI();

		if (!pAGI || !pAGI->AcquireAllResources(_pContext))
			return;

		const CXRAG_State* pState = pAGI->GetState(_iState);
		if (pState)
		{
			m_StateFlagsLoToken2 = pState->GetFlags(0);
			m_StateFlagsLoToken2 |= CHAR_STATEFLAG_EFFECTOKENACTIVE;
			// Should be safe and not overwritten by first token if we are lucky
			if (m_CanCounterType == 0)
				m_CanCounterType = (uint8)pAGI->GetStateConstantValueDef(pState,AG_CONSTANT_CANCOUNTER,0);
		}
		else
		{
			m_StateFlagsLoToken2 = 0;
		}

		pAGI->UnacquireAllResources();
	}
	else if (_TokenID == AG_TOKEN_UNEQUIP)
	{
		CWAGI* pAGI = GetAGI();

		if (!pAGI || !pAGI->AcquireAllResources(_pContext))
			return;

		const CXRAG_State* pState = pAGI->GetState(_iState);
		if (pState)
		{
			m_StateFlagsLoToken3 = pState->GetFlags(0);
			m_StateFlagsLoToken3 |= CHAR_STATEFLAG_EFFECTOKENACTIVE;
		}
		else
		{
			m_StateFlagsLoToken3 = 0;
		}

		pAGI->UnacquireAllResources();
	}
}

bool CWO_Clientdata_Character_AnimGraph::GetStateConstantsRequestPtrs(CWAGI*& _pAGI, const CXRAG_State*& _pState)
{
	_pAGI = GetAGI();

	if (!_pAGI)
		return false;

	const CWAGI_Token* pToken = _pAGI->GetTokenFromID(AG_TOKEN_MAIN);
	if (pToken == NULL)
		return false;

	_pState = _pAGI->GetState(pToken->GetStateIndex());
	if (_pState == NULL)
		return false;

	if (_pState->GetNumConstants() == 0)
		return false;

	return true;
}
//--------------------------------------------------------------------------------

void CWO_Clientdata_Character_AnimGraph::OnHit(const CWO_DamageMsg& _DX, CWObject_CoreData* _pObj)
{
	MSCOPESHORT(CWO_Clientdata_Character_AnimGraph::OnHit);

	if (_DX.m_bForceValid)
	{
		CMat43fp32 ObjMat = _pObj->GetPositionMatrix();
		CVec3Dfp32 ObjFwd = CVec3Dfp32::GetMatrixRow(ObjMat, 0);
		CVec3Dfp32 ObjLeft = CVec3Dfp32::GetMatrixRow(ObjMat, 1);
		// Q: Why the minus sign?
		// A: Force is in direction TOWARDS the target but the hit direction is FROM the target
		fp32 ForceLeft = -ObjLeft * _DX.m_Force;
		fp32 ForceFwd = -ObjFwd * _DX.m_Force;

		if (M_Fabs(ForceFwd) > M_Fabs(ForceLeft))
		{
			m_OnHit_Direction = (ForceFwd > 0) ? ONHIT_DIRECTION_FWD : ONHIT_DIRECTION_BWD;
			m_OnHit_Force = M_Fabs(ForceFwd);
		}
		else
		{
			m_OnHit_Direction = (ForceLeft > 0) ? ONHIT_DIRECTION_LEFT : ONHIT_DIRECTION_RIGHT;
			m_OnHit_Force = M_Fabs(ForceLeft);
		}
		// Bitfield with flags for the different types of damage
		// Definitions in WRPGDefs.h
		m_OnHit_DamageType = _DX.m_DamageType;

		// Damage is relative to m_MaxHealth
		m_OnHit_Damage = _DX.m_Damage / m_AGMaxHealth;
	}

/*	switch (_DX.m_SurfaceType)
	{
		case PLAYER_HITLOCATION_HEAD: m_OnHit_BodyPart = ONHIT_BODYPART_HEAD;
		case PLAYER_HITLOCATION_TORSO: m_OnHit_BodyPart = ONHIT_BODYPART_ABDOMEN;
	}*/

/*
	PLAYER_HITLOCATION_TORSO = 0,
:	PLAYER_HITLOCATION_ARMS,
	PLAYER_HITLOCATION_LEGS,
	PLAYER_HITLOCATION_HEAD,
	PLAYER_HITLOCATION_OTHER,
	PLAYER_NUM_HITLOCATIONS,
	
	PLAYER_HITLOCATION_BASE = 10,

	PLAYER_PROTECTION_SKIN = 0,
	PLAYER_PROTECTION_CLOTH,
	PLAYER_PROTECTION_LEATHER,
	PLAYER_PROTECTION_CHAIN,
	PLAYER_PROTECTION_PLATE,
	PLAYER_PROTECTION_OTHER,
	PLAYER_NUM_PROTECTIONS,
*/
}

//--------------------------------------------------------------------------------

void CWO_Clientdata_Character_AnimGraph::OnHit_Clear()
{
	m_OnHit_BodyPart = ONHIT_BODYPART_NONE;
	m_OnHit_Direction = ONHIT_DIRECTION_NONE;
	m_OnHit_Force = ONHIT_FORCE_NONE;
	m_OnHit_DamageType = DAMAGETYPE_UNDEFINED;
	m_OnHit_Damage = 0;
}

//--------------------------------------------------------------------------------

CAGVal CWO_Clientdata_Character_AnimGraph::Property_IsDead(const CWAGI_Context* _pContext,
														const CXRAG_ICallbackParams* _pParams)
{
	/*if (_pContext->m_pObj->m_iObject == 15)
	{
		if (_pContext->m_pWPhysState->IsServer() && true)
			ConOutL(CStrF("(Server) iObj=%d, GameTime=%3.3f, IsAlive = %d, m_Health = %f", _pContext->m_pObj->m_iObject, _pContext->m_GameTime, IsAlive(_pContext->m_pObj), m_Health));
		if (_pContext->m_pWPhysState->IsClient() && true)
			ConOutL(CStrF("(Client) iObj=%d, GameTime=%3.3f, IsAlive = %d, m_Health = %f", _pContext->m_pObj->m_iObject, _pContext->m_GameTime, IsAlive(_pContext->m_pObj), m_Health));
	}*/

//	return (m_Health <= 0);
	const int PhysType = CWObject_Character::Char_GetPhysType(_pContext->m_pObj);
	if (!IsAlive(PhysType))
		return CAGVal::From(true);

	return CAGVal::From(!IsAlive(PhysType));
//	return (_pContext->m_pObj->ClientFlags() & PLAYER_CLIENTFLAGS_DEAD);
}

//--------------------------------------------------------------------------------

CAGVal CWO_Clientdata_Character_AnimGraph::Property_IsSleeping(const CWAGI_Context* _pContext,
															const CXRAG_ICallbackParams* _pParams)
{
	return CAGVal::From((m_bIsSleeping==1)? 1.0f : 0.0f);
}

//--------------------------------------------------------------------------------

CAGVal CWO_Clientdata_Character_AnimGraph::Property_LedgeType(const CWAGI_Context* _pContext, 
														   const CXRAG_ICallbackParams* _pParams)
{
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (pCD)
	{
		// Find ledgemode
		int LedgeMode = pCD->m_ControlMode_Param4;
		if (_pParams->GetParam(0) == 2.0f)
		{
			// Special test
			return CAGVal::From((fp32)(LedgeMode & CWOBJECT_LEDGE_USEALTCLIMBUP));
		}
		if (LedgeMode & CWOBJECT_LEDGE_ENTERTYPEABOVELEDGE)
			return CAGVal::From(AG_LEDGEMODE_TYPEENTERFROMABOVE);
		if ((LedgeMode & (CWOBJECT_LEDGE_TYPEHIGH|CWOBJECT_LEDGE_ENTERTYPELEVELED)) == 
			(CWOBJECT_LEDGE_TYPEHIGH|CWOBJECT_LEDGE_ENTERTYPELEVELED))
			return CAGVal::From(AG_LEDGETYPE_BADLEDGEENTERDIRECT);
		else if (LedgeMode & CWOBJECT_LEDGE_TYPELOW)
			return CAGVal::From(AG_LEDGEMODE_TYPELOW);
		else if (LedgeMode & CWOBJECT_LEDGE_TYPEHIGH)
			return CAGVal::From(AG_LEDGEMODE_TYPEHIGH);
		else if (LedgeMode & CWOBJECT_LEDGE_TYPEMEDIUM)
			return CAGVal::From(AG_LEDGEMODE_TYPEMEDIUM);
	}

	return CAGVal::From(FP32FALSE);
}

//--------------------------------------------------------------------------------

#define Explore_TurnAngleFwd			(0.0f)
#define Explore_TurnAngleLeft45			(0.875f)
#define Explore_TurnAngleLeft90			(0.75f)
#define Explore_TurnAngleLeft135		(0.625f)
#define Explore_TurnAngleLeft180		(0.5f)
#define Explore_TurnAngleMaxRangeLeft	(0.125f)
#define Explore_HalfTurnAngleMaxRangeLeft (0.0625f)
#define Explore_HalfTurnAngleMaxRangeLeft4 (0.125f)


#define Explore_TurnAngleRight45		(0.125f)
#define Explore_TurnAngleRight90		(0.25f)
#define Explore_TurnAngleRight135		(0.375f)
#define Explore_TurnAngleRight180		(0.5f)
#define Explore_TurnAngleMaxRangeRight	(0.125f)
#define Explore_HalfTurnAngleMaxRangeRight (0.0625f)
#define Explore_HalfTurnAngleMaxRangeRight4 (0.125f)

MACRO_PHOBOSDEBUG(CStr GetStringFromCake(int Cake)
{
	switch (Cake)
	{
	case EXPLORE_ACTIVE_UNDEFINED:
		return "EXPLORE_ACTIVE_UNDEFINED";
	case EXPLORE_ACTIVE_FWD:
		return "EXPLORE_ACTIVE_FWD";
	case EXPLORE_ACTIVE_LEFT45:
		return "EXPLORE_ACTIVE_LEFT45";
	case EXPLORE_ACTIVE_LEFT90:
		return "EXPLORE_ACTIVE_LEFT90";
	case EXPLORE_ACTIVE_LEFT135:
		return "EXPLORE_ACTIVE_LEFT135";
	case EXPLORE_ACTIVE_LEFT180:
		return "EXPLORE_ACTIVE_LEFT180";
	case EXPLORE_ACTIVE_RIGHT45:
		return "EXPLORE_ACTIVE_RIGHT45";
	case EXPLORE_ACTIVE_RIGHT90:
		return "EXPLORE_ACTIVE_RIGHT90";
	case EXPLORE_ACTIVE_RIGHT135:
		return "EXPLORE_ACTIVE_RIGHT135";
	default:
		return "EXPLORE_ACTIVE_UNDEFINED_NOTZERO";
	}
})

fp32 AngleAdjust(fp32 _AngleA, fp32 _AngleB)
{
	fp32 Diff = (_AngleB - _AngleA);
	if (Diff > 0.5f)
		return -1.0f;
	else if (Diff < -0.5f)
		return 1.0f;
	else
		return 0.0f;
}

fp32 HardcodedFindAngle8(fp32 _MoveAngleUnit)
{
	if ((_MoveAngleUnit >= (Explore_TurnAngleLeft45 - Explore_HalfTurnAngleMaxRangeLeft)) &&
		(_MoveAngleUnit <= (Explore_TurnAngleLeft45 + Explore_HalfTurnAngleMaxRangeLeft)))
	{
		return EXPLORE_ACTIVE_LEFT45;	
	}
	else if ((_MoveAngleUnit >= (Explore_TurnAngleLeft90 - Explore_HalfTurnAngleMaxRangeLeft)) && 
		(_MoveAngleUnit <= (Explore_TurnAngleLeft90 + Explore_HalfTurnAngleMaxRangeLeft)))
	{
		return EXPLORE_ACTIVE_LEFT90;
	}
	else if ((_MoveAngleUnit >= (Explore_TurnAngleLeft135 - Explore_HalfTurnAngleMaxRangeLeft)) &&
			(_MoveAngleUnit <= (Explore_TurnAngleLeft135 + Explore_HalfTurnAngleMaxRangeLeft)))
	{
		return EXPLORE_ACTIVE_LEFT135;
	}
	else if ((_MoveAngleUnit >= (Explore_TurnAngleLeft180)) && 
			(_MoveAngleUnit <= (Explore_TurnAngleLeft180 + Explore_HalfTurnAngleMaxRangeLeft)))
	{
		return EXPLORE_ACTIVE_LEFT180;
	}
	else if ((_MoveAngleUnit >= (Explore_TurnAngleRight180 - Explore_HalfTurnAngleMaxRangeRight)) && 
			(_MoveAngleUnit <= (Explore_TurnAngleRight180)))
	{
		return EXPLORE_ACTIVE_RIGHT180;
	}
	else if ((_MoveAngleUnit >= (Explore_TurnAngleRight45 - Explore_HalfTurnAngleMaxRangeLeft)) && 
		(_MoveAngleUnit <= (Explore_TurnAngleRight45 + Explore_HalfTurnAngleMaxRangeLeft)))
	{
		return EXPLORE_ACTIVE_RIGHT45;
	}
	else if ((_MoveAngleUnit >= (Explore_TurnAngleRight90 - Explore_HalfTurnAngleMaxRangeRight)) && 
		(_MoveAngleUnit <= (Explore_TurnAngleRight90 + Explore_HalfTurnAngleMaxRangeRight)))
	{
		return EXPLORE_ACTIVE_RIGHT90;
	}
	else if ((_MoveAngleUnit >= (Explore_TurnAngleRight135 - Explore_HalfTurnAngleMaxRangeRight)) &&
		(_MoveAngleUnit <= (Explore_TurnAngleRight135 + Explore_HalfTurnAngleMaxRangeRight)))
	{
		return EXPLORE_ACTIVE_RIGHT135;
	}
	else
	{
		fp32 AngleTempFwd1 = Explore_TurnAngleFwd - Explore_HalfTurnAngleMaxRangeLeft;
		AngleTempFwd1 = MACRO_ADJUSTEDANGLE(AngleTempFwd1);
		fp32 AngleTempFwd2 = Explore_TurnAngleFwd + Explore_HalfTurnAngleMaxRangeLeft;
		AngleTempFwd2 = MACRO_ADJUSTEDANGLE(AngleTempFwd2);
		
		fp32 Adjust = AngleAdjust(_MoveAngleUnit, AngleTempFwd1);
		AngleTempFwd1 += Adjust;
		Adjust = AngleAdjust(_MoveAngleUnit, AngleTempFwd2);
		AngleTempFwd2 += Adjust;

		if ((_MoveAngleUnit >= AngleTempFwd1) &&
			(_MoveAngleUnit <= AngleTempFwd2))
		{
			return EXPLORE_ACTIVE_FWD;	
		}
	}

	// Shouldn't happen...
	ConOut("HardcodedFindAngle8: FAILED TO FIND ANGLE");
	return EXPLORE_ACTIVE_UNDEFINED;
}

// Left90/left180/right180/right90 fwd
fp32 HardcodedFindAngleDiff5(fp32 _MoveAngleUnit)
{
	if ((_MoveAngleUnit >= (Explore_TurnAngleLeft90 - Explore_HalfTurnAngleMaxRangeLeft4)) && 
		(_MoveAngleUnit <= (Explore_TurnAngleLeft90 + Explore_HalfTurnAngleMaxRangeLeft4)))
	{
		//ConOut("LEFT");
		return EXPLORE_ACTIVE_LEFT90;
	}
	else if ((_MoveAngleUnit >= (Explore_TurnAngleLeft180)) && 
			(_MoveAngleUnit <= (Explore_TurnAngleLeft180 + Explore_HalfTurnAngleMaxRangeLeft4)))
	{
		//ConOut("BWD");
		return EXPLORE_ACTIVE_LEFT180;
	}
	else if ((_MoveAngleUnit >= (Explore_TurnAngleRight90 - Explore_HalfTurnAngleMaxRangeRight4)) && 
			(_MoveAngleUnit <= (Explore_TurnAngleRight90 + Explore_HalfTurnAngleMaxRangeRight4)))
	{
		//ConOut("RIGHT");
		return EXPLORE_ACTIVE_RIGHT90;
	}
	else if ((_MoveAngleUnit >= (Explore_TurnAngleRight180 - Explore_HalfTurnAngleMaxRangeRight4)) && 
			(_MoveAngleUnit <= (Explore_TurnAngleRight180)))
	{
		return EXPLORE_ACTIVE_RIGHT180;
	}
	else
	{
		fp32 AngleTempFwd1 = Explore_TurnAngleFwd - Explore_HalfTurnAngleMaxRangeLeft4;
		AngleTempFwd1 = MACRO_ADJUSTEDANGLE(AngleTempFwd1);
		fp32 AngleTempFwd2 = Explore_TurnAngleFwd + Explore_HalfTurnAngleMaxRangeLeft4;
		AngleTempFwd2 = MACRO_ADJUSTEDANGLE(AngleTempFwd2);
		
		fp32 Adjust = AngleAdjust(_MoveAngleUnit, AngleTempFwd1);
		AngleTempFwd1 += Adjust;
		Adjust = AngleAdjust(_MoveAngleUnit, AngleTempFwd2);
		AngleTempFwd2 += Adjust;

		if ((_MoveAngleUnit >= AngleTempFwd1) &&
			(_MoveAngleUnit <= AngleTempFwd2))
		{
			//ConOut("FWD");
			return EXPLORE_ACTIVE_FWD;	
		}
	}

	//ConOut("UNDEFINED");

	// Shouldn't happen...
	ConOut("HardcodedFindAngle4: FAILED TO FIND ANGLE");
	return EXPLORE_ACTIVE_UNDEFINED;
}

fp32 HardcodedFindAngle4(fp32 _MoveAngleUnit)
{
	if ((_MoveAngleUnit >= (Explore_TurnAngleLeft90 - Explore_HalfTurnAngleMaxRangeLeft4)) && 
		(_MoveAngleUnit <= (Explore_TurnAngleLeft90 + Explore_HalfTurnAngleMaxRangeLeft4)))
	{
		//ConOut("LEFT");
		return EXPLORE_ACTIVE_LEFT90;
	}
	else if ((_MoveAngleUnit >= (Explore_TurnAngleLeft180 - Explore_HalfTurnAngleMaxRangeLeft4)) && 
			(_MoveAngleUnit <= (Explore_TurnAngleLeft180 + Explore_HalfTurnAngleMaxRangeLeft4)))
	{
		//ConOut("BWD");
		return EXPLORE_ACTIVE_LEFT180;
	}
	else if ((_MoveAngleUnit >= (Explore_TurnAngleRight90 - Explore_HalfTurnAngleMaxRangeRight4)) && 
			(_MoveAngleUnit <= (Explore_TurnAngleRight90 + Explore_HalfTurnAngleMaxRangeRight4)))
	{
		//ConOut("RIGHT");
		return EXPLORE_ACTIVE_RIGHT90;
	}
	else
	{
		fp32 AngleTempFwd1 = Explore_TurnAngleFwd - Explore_HalfTurnAngleMaxRangeLeft4;
		AngleTempFwd1 = MACRO_ADJUSTEDANGLE(AngleTempFwd1);
		fp32 AngleTempFwd2 = Explore_TurnAngleFwd + Explore_HalfTurnAngleMaxRangeLeft4;
		AngleTempFwd2 = MACRO_ADJUSTEDANGLE(AngleTempFwd2);
		
		fp32 Adjust = AngleAdjust(_MoveAngleUnit, AngleTempFwd1);
		AngleTempFwd1 += Adjust;
		Adjust = AngleAdjust(_MoveAngleUnit, AngleTempFwd2);
		AngleTempFwd2 += Adjust;

		if ((_MoveAngleUnit >= AngleTempFwd1) &&
			(_MoveAngleUnit <= AngleTempFwd2))
		{
			//ConOut("FWD");
			return EXPLORE_ACTIVE_FWD;	
		}
	}

	//ConOut("UNDEFINED");

	// Shouldn't happen...
	ConOut("HardcodedFindAngle4: FAILED TO FIND ANGLE");
	return EXPLORE_ACTIVE_UNDEFINED;
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_FindFixedMoveAngle8(const CWAGI_Context* _pContext,
																	 const CXRAG_ICallbackParams* _pParams)
{
	if (m_MoveVeloHoriz > MOVEANGLETHRESHOLD)
		return CAGVal::From(HardcodedFindAngle8(Property_MoveAngleUnit(_pContext,_pParams).GetFp()));

	return CAGVal::From(EXPLORE_ACTIVE_UNDEFINED);
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_FindFixedMoveAngle4(const CWAGI_Context* _pContext,
																	 const CXRAG_ICallbackParams* _pParams)
{
	if (m_MoveVeloHoriz > MOVEANGLETHRESHOLD)
		return CAGVal::From(HardcodedFindAngle4(Property_MoveAngleUnit(_pContext,_pParams).GetFp()));
	return CAGVal::From(EXPLORE_ACTIVE_UNDEFINED);
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_FindFixedMoveAngleControl8(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams)
{
	if (m_MoveRadiusControl > MOVEANGLETHRESHOLD)
		return CAGVal::From(HardcodedFindAngle8(Property_MoveAngleUnitControl(_pContext,_pParams).GetFp()));
	return CAGVal::From(EXPLORE_ACTIVE_UNDEFINED);
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_FindFixedMoveAngleControl4(const CWAGI_Context* _pContext,
																			const CXRAG_ICallbackParams* _pParams)
{
	if (m_MoveRadiusControl > MOVEANGLETHRESHOLD)
		return CAGVal::From(HardcodedFindAngle4(Property_MoveAngleUnitControl(_pContext,_pParams).GetFp()));
	return CAGVal::From(EXPLORE_ACTIVE_UNDEFINED);
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_FindFixedMoveAngleDiff8(const CWAGI_Context* _pContext,
																			const CXRAG_ICallbackParams* _pParams)
{
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	fp32 AngleDiff = -(pCD ? pCD->m_TurnCorrectionTargetAngle : 0.0f);
	AngleDiff = MACRO_ADJUSTEDANGLE(AngleDiff);
	fp32 Value =  HardcodedFindAngle8(AngleDiff);
	//ConOut(CStrF("Value: %f",Value));
	return CAGVal::From(Value);
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_FindFixedMoveAngleDiff5(const CWAGI_Context* _pContext,
																			const CXRAG_ICallbackParams* _pParams)
{
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	fp32 AngleDiff = -(pCD ? pCD->m_TurnCorrectionTargetAngle : 0.0f);
	AngleDiff = MACRO_ADJUSTEDANGLE(AngleDiff);
	fp32 Value =  HardcodedFindAngleDiff5(AngleDiff);
	//ConOut(CStrF("Value: %f",Value));
	return CAGVal::From(Value);
}

#define LEFT45END 0.916666667f
#define LEFT45START 0.75f
#define LEFT135END 0.625f
#define LEFT135START 0.6f
#define BACKWARDEND 0.6
#define BACKWARDSTART 0.4f
#define RIGHT135END 0.4f
#define RIGHT135START 0.25f
#define RIGHT45END 0.25f
#define RIGHT45START 0.083333333f


CAGVal CWO_Clientdata_Character_AnimGraph::Property_MoveAngleHeavyGuard(const CWAGI_Context* _pContext, 
																	 const CXRAG_ICallbackParams* _pParams)
{
	if ((m_MoveAngleUnitControl >= LEFT45START) && 
		(m_MoveAngleUnitControl <= LEFT45END))
	{
		//ConOut("LEFT");
		return CAGVal::From(EXPLORE_ACTIVE_LEFT45);
	}
	else if ((m_MoveAngleUnitControl >= LEFT135START) && 
			(m_MoveAngleUnitControl <= LEFT135END))
	{
		//ConOut("BWD");
		return CAGVal::From(EXPLORE_ACTIVE_LEFT135);
	}
	else if ((m_MoveAngleUnitControl >= BACKWARDSTART) && 
			(m_MoveAngleUnitControl <= BACKWARDEND))
	{
		//ConOut("BWD");
		return CAGVal::From(EXPLORE_ACTIVE_LEFT180);
	}
	else if ((m_MoveAngleUnitControl >= RIGHT45START) && 
			(m_MoveAngleUnitControl <= RIGHT45END))
	{
		//ConOut("RIGHT");
		return CAGVal::From(EXPLORE_ACTIVE_RIGHT45);
	}
	else if ((m_MoveAngleUnitControl >= RIGHT135START) && 
			(m_MoveAngleUnitControl <= RIGHT135END))
	{
		//ConOut("RIGHT");
		return CAGVal::From(EXPLORE_ACTIVE_RIGHT135);
	}
	else
	{
		return CAGVal::From(EXPLORE_ACTIVE_FWD);
	}

	//ConOut("UNDEFINED");

	// Shouldn't happen...
	ConOut("HardcodedFindAngle4: FAILED TO FIND ANGLE");
	return CAGVal::From(EXPLORE_ACTIVE_UNDEFINED);
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_WallCollision(const CWAGI_Context* _pContext, 
															  const CXRAG_ICallbackParams* _pParams)
{
	// If we're already in the wallcollision state don't mind the value (don't want repeated 
	// moves to the state)
	if (!(m_StateFlagsLoToken2 & CHAR_STATEFLAG_WALLCOLLISION) || 
		(_pParams->GetNumParams() > 0))
		return CAGVal::From(m_WallCollision);

	return CAGVal::From(FP32FALSE);
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_AngleDiff(const CWAGI_Context* _pContext, 
														   const CXRAG_ICallbackParams* _pParams)
{
	/*CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	return (pCD ? pCD->m_AngleDiff : 0.0f);*/
	return CAGVal::From(0);
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_AngleDiffS(const CWAGI_Context* _pContext, 
															const CXRAG_ICallbackParams* _pParams)
{
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	/*fp32 AngleDiff = (pCD ? pCD->m_AngleDiff : 0.0f);

	return ((AngleDiff < 0.5f) ? AngleDiff : (AngleDiff - 1.0f));*/
	//ConOut(CStrF("%s: %f",(_pContext->m_pWPhysState->IsServer() ? "Server: " : "Client: "),(pCD ? pCD->m_TurnCorrectionTargetAngle : 0.0f)));
	return CAGVal::From((pCD ? pCD->m_TurnCorrectionTargetAngle : 0.0f));
}

#define Explore_WalkAngleFwd			(0.0f)
#define Explore_WalkAngleBackward		(0.5f)
#define Explore_WalkAngleStrafeLeft		(0.685f)
#define Explore_WalkAngleStrafeRight	(0.315f)
#define Explore_WalkRangeFwd			(0.23f)
#define Explore_WalkRangeBwd			(0.1f)
#define Explore_WalkStrafeLeftRange (0.085f)
#define Explore_WalkStrafeRightRange (0.085f)

fp32 HardCodedFindWalkAngle(const fp32& _MoveAngleUnit)
{
	if ((_MoveAngleUnit >= (Explore_WalkAngleStrafeLeft - Explore_WalkStrafeLeftRange)) && 
		(_MoveAngleUnit <= (Explore_WalkAngleStrafeLeft + Explore_WalkStrafeLeftRange)))
	{
		//ConOut("LEFT");
		return EXPLORE_ACTIVE_LEFT90;
	}
	else if ((_MoveAngleUnit >= Explore_WalkAngleBackward) && 
			(_MoveAngleUnit <= (Explore_WalkAngleBackward + Explore_WalkRangeBwd)))
	{
		//ConOut("BWD");
		return EXPLORE_ACTIVE_LEFT180;
	}
	else if ((_MoveAngleUnit >= (Explore_WalkAngleBackward - Explore_WalkRangeBwd)) && 
			(_MoveAngleUnit <= Explore_WalkAngleBackward))
	{
		return EXPLORE_ACTIVE_RIGHT180;
	}
	else if ((_MoveAngleUnit >= (Explore_WalkAngleStrafeRight - Explore_WalkStrafeRightRange)) && 
			(_MoveAngleUnit <= (Explore_WalkAngleStrafeRight + Explore_WalkStrafeRightRange)))
	{
		//ConOut("RIGHT");
		return EXPLORE_ACTIVE_RIGHT90;
	}
	else
	{
		fp32 AngleTempFwd1 = Explore_WalkAngleFwd - Explore_WalkRangeFwd;
		AngleTempFwd1 = MACRO_ADJUSTEDANGLE(AngleTempFwd1);
		fp32 AngleTempFwd2 = Explore_WalkAngleFwd + Explore_WalkRangeFwd;
		AngleTempFwd2 = MACRO_ADJUSTEDANGLE(AngleTempFwd2);
		
		fp32 Adjust = AngleAdjust(_MoveAngleUnit, AngleTempFwd1);
		AngleTempFwd1 += Adjust;
		Adjust = AngleAdjust(_MoveAngleUnit, AngleTempFwd2);
		AngleTempFwd2 += Adjust;

		if ((_MoveAngleUnit >= AngleTempFwd1) &&
			(_MoveAngleUnit <= AngleTempFwd2))
		{
			//ConOut("FWD");
			return EXPLORE_ACTIVE_FWD;	
		}
	}

	//ConOut("UNDEFINED");

	// Shouldn't happen...
	ConOut("HardCodedFindWalkAngle: FAILED TO FIND ANGLE");
	return EXPLORE_ACTIVE_UNDEFINED;
}

#define Explore_FightAngleFwd			(0.0f)
#define Explore_FightAngleBackward		(0.5f)
#define Explore_FightAngleStrafeLeft	(0.75f)
#define Explore_FightAngleStrafeRight	(0.25f)
#define Explore_FightRangeBwd			(0.125f)
#define Explore_FightLeftRange			(0.125f)
#define Explore_FightRightRange			(0.125f)

fp32 HardCodedFindFightAngle(const fp32& _MoveAngleUnit)
{
	if ((_MoveAngleUnit >= (Explore_FightAngleStrafeLeft - Explore_FightLeftRange)) && 
		(_MoveAngleUnit <= (Explore_FightAngleStrafeLeft + Explore_FightLeftRange)))
	{
		//ConOut("LEFT");
		return EXPLORE_ACTIVE_LEFT90;
	}
	else if ((_MoveAngleUnit >= Explore_FightAngleBackward) && 
		(_MoveAngleUnit <= (Explore_FightAngleBackward + Explore_FightRangeBwd)))
	{
		//ConOut("BWD");
		return EXPLORE_ACTIVE_LEFT180;
	}
	else if ((_MoveAngleUnit >= (Explore_FightAngleBackward - Explore_FightRangeBwd)) && 
		(_MoveAngleUnit <= Explore_FightAngleBackward))
	{
		return EXPLORE_ACTIVE_RIGHT180;
	}
	else if ((_MoveAngleUnit >= (Explore_FightAngleStrafeRight - Explore_FightRightRange)) && 
		(_MoveAngleUnit <= (Explore_FightAngleStrafeRight + Explore_FightRightRange)))
	{
		//ConOut("RIGHT");
		return EXPLORE_ACTIVE_RIGHT90;
	}
	else
	{	
		return EXPLORE_ACTIVE_FWD;	
	}
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_FixedWalkAngle4(const CWAGI_Context* _pContext,
															const CXRAG_ICallbackParams* _pParams)
{
	if (m_MoveRadiusControl > MOVEANGLETHRESHOLD)
		return CAGVal::From(HardCodedFindWalkAngle(m_MoveAngleUnitControl));

	return CAGVal::From(0.0f);
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_FixedFightAngle4(const CWAGI_Context* _pContext,
																	const CXRAG_ICallbackParams* _pParams)
{
	if (m_MoveRadiusControl > MOVEANGLETHRESHOLD)
		return CAGVal::From(HardCodedFindFightAngle(m_MoveAngleUnitControl));

	return CAGVal::From(0.0f);
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_BreakOffGrab(const CWAGI_Context* _pContext,
																 const CXRAG_ICallbackParams* _pParams)
{
	// If actionpresscount >= 0 all is well
	return CAGVal::From((fp32)!(m_ActionPressCount >= 0));
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_CanSwitchWeapon(const CWAGI_Context* _pContext,
																	const CXRAG_ICallbackParams* _pParams)
{
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (!pCD)
		return CAGVal::From(0.0f);

	int32 ControlMode = CWObject_Character::Char_GetControlMode(_pContext->m_pObj);
	bool bCanSwitch = !((m_StateFlagsLo |m_StateFlagsLoToken2 | m_StateFlagsLoToken3) & CHAR_STATEFLAG_EQUIPPING) &&
		(((m_ButtonPress & ~m_UsedButtonPress) & BUTTONPRESS_B) || m_bQueueWeaponSelect);
	return CAGVal::From(bCanSwitch && !pCD->m_RelAnimPos.HasQueued(pCD->m_GameTick) && (ControlMode == PLAYER_CONTROLMODE_FREE || ControlMode == PLAYER_CONTROLMODE_CARRYBODY || m_StateFlagsLo & CHAR_STATEFLAG_BLOCKACTIVE) ? FP32TRUE : FP32FALSE);
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_CanFightStandUp(const CWAGI_Context* _pContext,
																	const CXRAG_ICallbackParams* _pParams)
{
	if (!(m_ButtonPress & ~m_UsedButtonPress) & BUTTONPRESS_PRIMARY)
		return CAGVal::From(0.0f);
	fp32 Tension;
	CWObject_Message Msg(OBJMSG_CHAR_GETTENSION);
	Msg.m_pData = &Tension;
	Msg.m_DataSize = sizeof(Tension);
	_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, _pContext->m_pObj->m_iObject);
	if (Tension > 0)
		return CAGVal::From(1.0f);

	return CAGVal::From(0.0f);
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_MeleeeAttack(const CWAGI_Context* _pContext,
																 const CXRAG_ICallbackParams* _pParams)
{
	int32 Mask = AG_AIMELEE_MASK_NORMALATTACK;
	if (_pParams && _pParams->GetParam(0))
		Mask = AG_AIMELEE_MASK_COMBO;
	
	int32 Val = (m_GPBitField & Mask);
	int32 Attack = 0;
	switch (Val)
	{
	case AG_AIMELEE_ATTACK_RIGHT:
	case AG_AIMELEE_ATTACK_COMBO_RIGHT:
		{
			Attack = 1;
			break;
		}
	case AG_AIMELEE_ATTACK_LEFT:
	case AG_AIMELEE_ATTACK_COMBO_LEFT:
		{
			Attack = 2;
			break;
		}
	case AG_AIMELEE_ATTACK_MIDDLE:
	case AG_AIMELEE_ATTACK_COMBO_MIDDLE:
		{
			Attack = 3;
			break;
		}
	case AG_AIMELEE_ATTACK_SPECIAL:
	case AG_AIMELEE_ATTACK_COMBO_SPECIAL:
		{
			Attack = 4;
			break;
		}
	default:
		break;
	}
	
	return CAGVal::From((fp32)Attack);
}

//--------------------------------------------------------------------------------

CAGVal CWO_Clientdata_Character_AnimGraph::Property_IsFighting(const CWAGI_Context* _pContext,
															const CXRAG_ICallbackParams* _pParams)
{
	return CAGVal::From(_pContext->m_pWPhysState->Phys_Message_SendToObject(
		CWObject_Message(OBJMSG_CHAR_ISINFIGHTMODE),_pContext->m_pObj->m_iObject) != -1);
}

//--------------------------------------------------------------------------------

CAGVal CWO_Clientdata_Character_AnimGraph::Property_IsCrouching(const CWAGI_Context* _pContext,
															 const CXRAG_ICallbackParams* _pParams)
{
	return CAGVal::From((CWObject_Character::Char_GetPhysType(_pContext->m_pObj) == PLAYER_PHYS_CROUCH));
}

//--------------------------------------------------------------------------------
int32 GetJoyPadAngle(int32 _JoyPad)
{
	if (_JoyPad & DIGIPAD_UP)
	{
		if (_JoyPad & DIGIPAD_LEFT)
			return EXPLORE_ACTIVE_LEFT45;
		if (_JoyPad & DIGIPAD_RIGHT)
			return EXPLORE_ACTIVE_RIGHT45;
		else
			return EXPLORE_ACTIVE_FWD;
	}
	else if (_JoyPad & DIGIPAD_DOWN)
	{
		if (_JoyPad & DIGIPAD_LEFT)
			return EXPLORE_ACTIVE_LEFT135;
		if (_JoyPad & DIGIPAD_RIGHT)
			return EXPLORE_ACTIVE_RIGHT135;
		else
			return EXPLORE_ACTIVE_LEFT180;
	}
	else if (_JoyPad & DIGIPAD_LEFT)
		return EXPLORE_ACTIVE_LEFT90;
	else if (_JoyPad & DIGIPAD_RIGHT)
		return EXPLORE_ACTIVE_RIGHT90;

	return EXPLORE_ACTIVE_UNDEFINED;
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_LeanActive(const CWAGI_Context* _pContext,
															const CXRAG_ICallbackParams* _pParams)
{
	return CAGVal::From(FP32FALSE);
	/*
	// Check if any bad lean modes are active (ONLY TEMPORARY)
	// Check if primary is active, then no lean
	if (m_ButtonPress & BUTTONPRESS_PRIMARY)
		return CAGVal::From(FP32FALSE);

	bool bOnlyLeftRight = _pParams->GetParam(0) != 0;

	int32 JoyPadAngle = GetJoyPadAngle(m_JoyPad);
	if (JoyPadAngle == EXPLORE_ACTIVE_LEFT90)
		return CAGVal::From(JoyPadAngle);
	else if (JoyPadAngle == EXPLORE_ACTIVE_RIGHT90)
		return CAGVal::From(JoyPadAngle);
	else if (bOnlyLeftRight)
		return CAGVal::From(FP32FALSE);
	else if (JoyPadAngle == EXPLORE_ACTIVE_FWD)
		return CAGVal::From(JoyPadAngle);
	
	return CAGVal::From(FP32FALSE);*/
}

//--------------------------------------------------------------------------------

CAGVal CWO_Clientdata_Character_AnimGraph::Property_JoyPad(const CWAGI_Context* _pContext,
														const CXRAG_ICallbackParams* _pParams)
{
	// Use the same directions as find fixed move angle
	return CAGVal::From(GetJoyPadAngle(m_JoyPad));
}

//--------------------------------------------------------------------------------

CAGVal CWO_Clientdata_Character_AnimGraph::Property_GetControlmode(const CWAGI_Context* _pContext,
																const CXRAG_ICallbackParams* _pParams)
{
	return CAGVal::From(CWObject_Character::Char_GetControlMode(_pContext->m_pObj));
}

//--------------------------------------------------------------------------------

CAGVal CWO_Clientdata_Character_AnimGraph::Property_ActionCutsceneType(const CWAGI_Context* _pContext,
																	const CXRAG_ICallbackParams* _pParams)
{
	// Get actioncutscene type
	if (CWObject_Character::Char_GetControlMode(_pContext->m_pObj) == 
		PLAYER_CONTROLMODE_ACTIONCUTSCENE)
	{
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
		if (!pCD)
			return CAGVal::From(ACTIONCUTSCENE_TYPE_UNDEFINED);
		
		int ACSType = pCD->m_ControlMode_Param2;
		return CAGVal::From((fp32)ACSType);
	}

	return CAGVal::From(ACTIONCUTSCENE_TYPE_UNDEFINED);
}

//--------------------------------------------------------------------------------

CAGVal CWO_Clientdata_Character_AnimGraph::Property_LadderEndPoint(const CWAGI_Context* _pContext,
																const CXRAG_ICallbackParams* _pParams)
{
	// Find endpoint in ladder
	int ControlMode = CWObject_Character::Char_GetControlMode(_pContext->m_pObj);
	if ((ControlMode == PLAYER_CONTROLMODE_LADDER) ||
		(ControlMode == PLAYER_CONTROLMODE_LEDGE2) ||
		(ControlMode == PLAYER_CONTROLMODE_HANGRAIL))
	{
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
		int iLadder = (int)pCD->m_ControlMode_Param0;
		int EndPoint = 0;

		// Check which type of endpoint check we want (first one is more expensive)
		if (_pParams && (_pParams->GetNumParams() > 0) && 
			(_pParams->GetParam(0) == AG_LADDERENDPOINT_MODE1))
		{
			fp32 Width = ((fp32)pCD->m_Phys_Width) * 1.2f;
			int LinkMode = pCD->m_ControlMode_Param4;

			EndPoint = _pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_FINDENDPOINT,
						*(int32*)&Width,LinkMode,_pContext->m_pObj->m_iObject, 0,_pContext->m_pObj->GetPosition()), iLadder);
		}
		else
		{
			EndPoint = _pContext->m_pWPhysState->Phys_Message_SendToObject(
				CWObject_Message(OBJMSG_LADDER_GETCURRENTENDPOINT), iLadder);
		}

		if (_pParams->GetParam(1) == AG_LADDERENDPOINT_LEFT)
		{
			switch (EndPoint)
			{
			case LEDGE_ENDPOINT_INLEFT:
			case LEDGE_ENDPOINT_OUTLEFT:
			case LEDGE_ENDPOINT_LEFT:
				break;
			default:
				EndPoint = 0;
			}
		}
		else if (_pParams->GetParam(1) == AG_LADDERENDPOINT_RIGHT)
		{
			switch (EndPoint)
			{
			case LEDGE_ENDPOINT_INRIGHT:
			case LEDGE_ENDPOINT_OUTRIGHT:
			case LEDGE_ENDPOINT_RIGHT:
				break;
			default:
				EndPoint = 0;
			}
		}
		
		//MACRO_PHOBOSDEBUG(ConOut(CStrF("Endpoint: %d", EndPoint));)
		return CAGVal::From(EndPoint);
	}

	return CAGVal::From(LADDER_ENDPOINT_NOENDPOINT);
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_HangrailCanGoForward(const CWAGI_Context* _pContext,
																		 const CXRAG_ICallbackParams* _pParams)
{
	int ControlMode = CWObject_Character::Char_GetControlMode(_pContext->m_pObj);
	if (ControlMode == PLAYER_CONTROLMODE_HANGRAIL)
	{
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
		int iLadder = (int)pCD->m_ControlMode_Param0;
		int32 EndPoint = _pContext->m_pWPhysState->Phys_Message_SendToObject(
			CWObject_Message(OBJMSG_LADDER_GETCURRENTENDPOINT), iLadder);
		if (!EndPoint)
			return CAGVal::From(FP32TRUE);

		bool bResult = true;
		CVec3Dfp32 Pos1,Pos2;
		// Check if we're facing along hangrail, if we're at an endpoint we can still go forward
		bResult = (_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION1,
				0,0,_pContext->m_pObj->m_iObject, 0,0,0,&Pos1, sizeof(CVec3Dfp32)), iLadder) ? bResult : false);
		bResult = (_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION2,
				0,0,_pContext->m_pObj->m_iObject, 0,0,0,&Pos2, sizeof(CVec3Dfp32)), iLadder) ? bResult : false);
		if (!bResult)
			return CAGVal::From(FP32FALSE);

		CVec3Dfp32 HangrailDir = Pos2 - Pos1;
		CVec3Dfp32 CharDir = CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),0);
		fp32 DirDot = CharDir * HangrailDir;
		if (((DirDot > 0.0f) && EndPoint == 1) || ((DirDot < 0.0f) && EndPoint == 2))
		{
			return CAGVal::From(FP32TRUE);
		}
		
		return CAGVal::From(FP32FALSE);
	}
	return CAGVal::From(FP32FALSE);
}

//--------------------------------------------------------------------------------

CAGVal CWO_Clientdata_Character_AnimGraph::Property_IsInAir(const CWAGI_Context* _pContext,
														 const CXRAG_ICallbackParams* _pParams)
{
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);

	if (pCD)
		return CAGVal::From(pCD->m_Phys_bInAir != 0);

	return CAGVal::From(0.0f);
}

//--------------------------------------------------------------------------------

CAGVal CWO_Clientdata_Character_AnimGraph::Property_EquippedItemClass(const CWAGI_Context* _pContext,
																   const CXRAG_ICallbackParams* _pParams)
{
	// Find equipped item (if any) and check if it can be activated
	// Only on server?
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (pCD)
		return CAGVal::From((fp32)pCD->m_EquippedItemClass);

	return CAGVal::From(0.0f);
	/*if (_pContext->m_pWPhysState->IsClient())
		return CAGVal::From(0.0f);

	CWObject_Character* pChar = safe_cast<CWObject_Character>(_pContext->m_pObj);
	if (pChar)
	{
		CRPG_Object_Char* pRPG = pChar->Char();
		CRPG_Object_Item* pEquippedItem = (pRPG ? pRPG->GetEquippedItem(AG_ITEMSLOT_WEAPONS) : NULL);
		if (pEquippedItem)
			return CAGVal::From(pEquippedItem->m_AnimProperty);
	}

	return CAGVal::From(0.0f);*/
}

//--------------------------------------------------------------------------------

CAGVal CWO_Clientdata_Character_AnimGraph::Property_CanActivateItem(const CWAGI_Context* _pContext,
																 const CXRAG_ICallbackParams* _pParams)
{
	// Find equipped item (if any) and check if it can be activated
	// Only on server?	
	if (!_pContext->m_pWPhysState->IsServer())
		return CAGVal::From(0.0f);

	CWObject_Character* pChar = safe_cast<CWObject_Character>(_pContext->m_pObj);
	if (!((m_StateFlagsLo|m_StateFlagsLoToken2) & (CHAR_STATEFLAG_HURTACTIVE|CHAR_STATEFLAG_EQUIPPING)) && pChar)
	{
		CRPG_Object_Char* pRPG = pChar->Char();
		CRPG_Object_Item* pEquippedItem = (pRPG ? pRPG->GetEquippedItem(AG_ITEMSLOT_WEAPONS) : NULL);
		if (pEquippedItem)
			return CAGVal::From(pEquippedItem->IsActivatable(pRPG,_pContext->m_pObj->m_iObject));
	}

	return CAGVal::From(0.0f);
	
	// Can't activate if hurt flag is active
	/*return CAGVal::From((!(m_StateFlagsLoToken2 & CHAR_STATEFLAG_HURTACTIVE) && 
		_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_CANACTIVATEITEM, AG_ITEMSLOT_WEAPONS), _pContext->m_pObj->m_iObject)));*/
}

//--------------------------------------------------------------------------------

CAGVal CWO_Clientdata_Character_AnimGraph::Property_NeedReload(const CWAGI_Context* _pContext,
															const CXRAG_ICallbackParams* _pParams)
{
	if (!_pContext->m_pWPhysState->IsServer())
		return CAGVal::From(0.0f);
	CWObject_Character* pChar = safe_cast<CWObject_Character>(_pContext->m_pObj);
	CRPG_Object_Item *pItem = pChar->Char()->GetEquippedItem(0);
	if(pItem && pItem->NeedReload(pChar->m_iObject))
		return CAGVal::From(1.0f);

	return CAGVal::From(0.0f);
}

//--------------------------------------------------------------------------------

CAGVal CWO_Clientdata_Character_AnimGraph::Property_ButtonTest(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams)
{
	if ((_pParams == NULL) || (_pParams->GetNumParams() < 1))
		return CAGVal::From(FP32FALSE);

	// Disable all button presses if disable attack is set
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (pCD && (pCD->m_Disable & PLAYER_DISABLE_ATTACK))
		return CAGVal::From(FP32FALSE);

	int Button = (int)_pParams->GetParam(0);
	Button = MACRO_GETBUTTONFROMINDEX(Button);
	int NotUsed = (int)(_pParams->GetNumParams() >= 2 ? _pParams->GetParam(1) : FP32FALSE);
	//ConOut(CStrF("Button: %.2x  NotUsed: %d", (uint8)Button, NotUsed));
	if (NotUsed)
		return CAGVal::From(((m_ButtonPress & ~m_UsedButtonPress) & Button));

	return CAGVal::From((m_ButtonPress & Button));
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_CanBlock(const CWAGI_Context* _pContext, 
															 const CXRAG_ICallbackParams* _pParams)
{
	if ((_pParams == NULL) || (_pParams->GetNumParams() < 1))
		return CAGVal::From(FP32FALSE);

	int32 bBlockActive = (m_StateFlagsLo | m_StateFlagsLoToken2) & CHAR_STATEFLAG_BLOCKACTIVE;
	if (bBlockActive)
		return CAGVal::From(FP32FALSE);

	int Button = (int)_pParams->GetParam(0);
	Button = MACRO_GETBUTTONFROMINDEX(Button);
	int NotUsed = (int)(_pParams->GetNumParams() >= 2 ? _pParams->GetParam(1) : FP32FALSE);
	//ConOut(CStrF("Button: %.2x  NotUsed: %d", (uint8)Button, NotUsed));
	if (NotUsed)
	{
		return CAGVal::From(((m_ButtonPress & ~m_UsedButtonPress) & Button));
	}

	return CAGVal::From((m_ButtonPress & Button));
}

//--------------------------------------------------------------------------------

CAGVal CWO_Clientdata_Character_AnimGraph::Property_CanEndACS(const CWAGI_Context* _pContext, 
														   const CXRAG_ICallbackParams* _pParams)
{
	if (_pContext->m_pWPhysState->IsClient())
		return CAGVal::From(FP32FALSE);

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (pCD)
	{
		int iACS = (int)pCD->m_ControlMode_Param0;

		// Check for if the object really exists
		CWObject_CoreData* pObj = _pContext->m_pWPhysState->Object_GetCD(iACS);
		if (pObj)
		{
			CWObject_Message Msg(OBJMSG_ACTIONCUTSCENE_CANENDACS);
			return CAGVal::From(_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, iACS));
		}

		return CAGVal::From(true);
	}

	return CAGVal::From(FP32FALSE);
}

//--------------------------------------------------------------------------------

CAGVal CWO_Clientdata_Character_AnimGraph::Property_CanClimbUpLedge(const CWAGI_Context* _pContext, 
							 const CXRAG_ICallbackParams* _pParams)
{
	if (_pContext->m_pWPhysState->IsClient())
		return CAGVal::From(FP32FALSE);

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (pCD)
	{
		int iLedge = (int)pCD->m_ControlMode_Param0;
		fp32 Width = (fp32)pCD->m_Phys_Width * 2.0f;
		fp32 Height = (fp32)pCD->m_Phys_Height;

		CWObject_Message Msg(OBJMSG_LEDGE_CANCLIMBUP,Width,Height,0,0,
			_pContext->m_pObj->GetPosition());
		return CAGVal::From(_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, iLedge));
	}

	return CAGVal::From(FP32FALSE);
}

//--------------------------------------------------------------------------------

CAGVal CWO_Clientdata_Character_AnimGraph::Property_NewFightTestMode(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams)
{
	// Find which fight mode we're in (close/medium/noopponent)
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);

	if (pCD && (pCD->m_iFightingCharacter != -1))
	{
		int FightModeFlag = pCD->m_FightModeFlag;

		if ((FightModeFlag & PLAYER_FIGHTMODEFLAG_CLOSE) && 
			(FightModeFlag & PLAYER_FIGHTMODEFLAG_SNEAK))
			return CAGVal::From(AG_FIGHTMODE_MODE_SNEAK);
		if (FightModeFlag & PLAYER_FIGHTMODEFLAG_CLOSE)
			return CAGVal::From(AG_FIGHTMODE_MODE_CLOSE);
		else if (FightModeFlag & PLAYER_FIGHTMODEFLAG_MEDIUM)
			return CAGVal::From(AG_FIGHTMODE_MODE_MEDIUM);
	}

	return CAGVal::From(AG_FIGHTMODE_MODE_NOOPPONENT);
}

//--------------------------------------------------------------------------------

CAGVal CWO_Clientdata_Character_AnimGraph::Property_WantedBehavior(const CWAGI_Context* _pContext, 
																const CXRAG_ICallbackParams* _pParams)
{
	// The AI should set some parameter to do the wanted behavior
	return CAGVal::From((fp32)(~CHAR_WANTEDBEHAVIORFLAG_MASKTYPE & m_WantedBehavior));
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_WantedBehaviorType(const CWAGI_Context* _pContext, 
																   const CXRAG_ICallbackParams* _pParams)
{
	// The AI should set some parameter to do the wanted behavior
	return CAGVal::From((fp32)(CHAR_WANTEDBEHAVIORFLAG_MASKTYPE & m_WantedBehavior));
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_LeverState(const CWAGI_Context* _pContext, 
															const CXRAG_ICallbackParams* _pParams)
{
	if (_pContext->m_pWPhysState->IsClient())
		return CAGVal::From(FP32FALSE);

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (pCD)
	{
		int iACS = (int)pCD->m_ControlMode_Param0;

		// Check for if the object really exists
		CWObject_CoreData* pObj = _pContext->m_pWPhysState->Object_GetCD(iACS);
		if (pObj)
		{
			CWObject_Message Msg(OBJMSG_ACTIONCUTSCENE_LEVER_GETSTATE);
			return CAGVal::From(_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, iACS));
		}

		return CAGVal::From(true);
	}

	return CAGVal::From(CWObject_LeverActionCutscene::LEVERSTATE_UP);
}

//--------------------------------------------------------------------------------

CAGVal CWO_Clientdata_Character_AnimGraph::Property_ValveState(const CWAGI_Context* _pContext, 
															const CXRAG_ICallbackParams* _pParams)
{
	if (_pContext->m_pWPhysState->IsClient() || (_pParams->GetNumParams() <= 0))
		return CAGVal::From(FP32FALSE);

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (pCD)
	{
		int iACS = (int)pCD->m_ControlMode_Param0;

		// Check for if the object really exists
		CWObject_CoreData* pObj = _pContext->m_pWPhysState->Object_GetCD(iACS);
		if (pObj)
		{
			CWObject_Message Msg(OBJMSG_ACTIONCUTSCENE_VALVE_CANTURN, (int)_pParams->GetParam(0));
			return CAGVal::From(_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, iACS));
		}
	}

	return CAGVal::From(FP32FALSE);
}

//--------------------------------------------------------------------------------

CAGVal CWO_Clientdata_Character_AnimGraph::Property_CanPlayEquip(const CWAGI_Context* _pContext, 
															  const CXRAG_ICallbackParams* _pParams)
{
	return CAGVal::From(!((m_StateFlagsLoToken3|m_StateFlagsLoToken2|m_StateFlagsLo) & CHAR_STATEFLAG_EQUIPPING));
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_HasFightTarget(const CWAGI_Context* _pContext, 
																const CXRAG_ICallbackParams* _pParams)
{
	if (_pContext->m_pWPhysState->IsClient())
		return CAGVal::From(_pParams->GetParam(0));

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (pCD && (pCD->m_iCloseObject != 0))
	{
		int32 iTarget = pCD->m_iCloseObject;
		CWObject_CoreData* pTarget = _pContext->m_pWPhysState->Object_GetCD(iTarget);
		CWO_Character_ClientData* pCDTarget = (pTarget ? CWObject_Character::GetClientData(pTarget) : NULL);
		if (!pTarget || !pCDTarget)
			return CAGVal::From(0.0f);
		// Check direction of target object against our own
		const int TargetPhysType = CWObject_Character::Char_GetPhysType(pTarget);
		int32 SpecialGrab = _pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETSPECIALGRAB),pTarget->m_iObject);
		// Check so that the object can be neckbreaked
		if ((!pCDTarget->m_AnimGraph.GetIsSleeping() || SpecialGrab == 3) && 
			IsAlive(TargetPhysType) && (_pContext->m_pWPhysState->Phys_Message_SendToObject(
			CWObject_Message(OBJMSG_CHAR_GETDAMAGEFACTOR,-1,DAMAGETYPE_DARKNESS), iTarget) != 0))
		{
			/*CAI_Core* pAIVictim = NULL;
			_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_AIQUERY_GETAI,0,0,0,0,0,0,&pAIVictim), iTarget);
			// If target doesn't have an AI, he cannot know about me :)
			// Since this won't work on the client return default value?
			CAI_AgentInfo* pAgent = (pAIVictim ? pAIVictim->m_KB.GetAgentInfo(_pContext->m_pObj->m_iObject) : NULL);
			if (pAgent)
			{
				if ((pAgent->GetCurAwareness() >= CAI_AgentInfo::DETECTED) &&
				(pAgent->GetCurRelation() >= CAI_AgentInfo::ENEMY))
					return CAGVal::From(FP32FALSE);
			}
			else
			{
				return CAGVal::From((_pParams ? _pParams->GetParam(0) : FP32FALSE));
			}*/

			#define FIGHT_CLOSELIMIT 1600
			CVec3Dfp32 OpponentPosition, Direction;
			uint8 OppFlag;

			OpponentPosition = pTarget->GetPosition();
			CVec3Dfp32 DirToTarget = OpponentPosition - _pContext->m_pObj->GetPosition();
			OppFlag = (DirToTarget.LengthSqr() > FIGHT_CLOSELIMIT ? 
				PLAYER_FIGHTMODEFLAG_MEDIUM : PLAYER_FIGHTMODEFLAG_CLOSE);
			DirToTarget.Normalize();

			CVec3Dfp32 SelfDir = CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),0);
			CVec3Dfp32 OpponentDir = CVec3Dfp32::GetMatrixRow(_pContext->m_pWPhysState->Object_GetPositionMatrix(iTarget),0);
			fp32 SneakDot = OpponentDir * SelfDir;
			fp32 DotTarget = DirToTarget * OpponentDir;
			fp32 DotToTarget = SelfDir * DirToTarget;

			// Break neck if we are behind the opponent and sufficiently close
			return (SpecialGrab == 3 ? CAGVal::From(DotToTarget > 0.5f) : CAGVal::From(((SneakDot > 0.5f) && (DotTarget > 0.5f) && (DotToTarget > 0.5f) && (OppFlag & PLAYER_FIGHTMODEFLAG_CLOSE))));
		}
	}
	return CAGVal::From(FP32FALSE);
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_HasTranqTarget(const CWAGI_Context* _pContext, 
																   const CXRAG_ICallbackParams* _pParams)
{
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	int32 iBest = (pCD ? pCD->m_iCloseObject : 0);
	if (iBest == 0)
		return CAGVal::From(FP32FALSE);

	CWObject_CoreData* pTarget = (iBest != -1 ? _pContext->m_pWPhysState->Object_GetCD(iBest) : NULL);
	CWO_Character_ClientData* pCDTarget = (pTarget ? CWObject_Character::GetClientData(pTarget) : NULL);
	int32 Disable = _pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETDISABLE),_pContext->m_pObj->m_iObject);
	if(!pTarget || !pCDTarget || (Disable & PLAYER_DISABLE_GRAB))
		return CAGVal::From(FP32FALSE);
	const int TargetPhysType = CWObject_Character::Char_GetPhysType(pTarget);
	int32 TypeIndex = _pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETSPECIALGRAB),pTarget->m_iObject);
	if (IsAlive(TargetPhysType) && (TypeIndex == 3))
	{
		// Target is tranqed
		return CAGVal::From(FP32TRUE);
	}

	return CAGVal::From(FP32FALSE);
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_CanGotoFight(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams)
{
	// Check for 
	//*"ButtonTest(BUTTON_PRIMARY,BUTTONTEST_NOTUSED)"
	//*"ButtonTest(BUTTON_SECONDARY)"
	//*(m_SelectItemTimeOutTick > m_GameTick)

	// Disable all button presses if disable attack is set
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (pCD && (pCD->m_Disable & PLAYER_DISABLE_ATTACK))
		return CAGVal::From(FP32FALSE);

	int Button = BUTTONPRESS_PRIMARY | BUTTONPRESS_SECONDARY;
	return CAGVal::From((pCD->m_SelectItemTimeOutTick > pCD->m_GameTick) || ((m_ButtonPress & ~(m_UsedButtonPress & BUTTONPRESS_PRIMARY)) & Button));
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_HangRailCheckHeight(const CWAGI_Context* _pContext, 
																	 const CXRAG_ICallbackParams* _pParams)
{
#define HANGRAIL_MAXDROPOFFHEIGHT (40.0f)
	// Check height direcly below character
	// Make a trace to check that min height is good for hangrail dropoff
	CCollisionInfo CInfo;
	CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
	int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
	int32 ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;
	int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
	int32 iExclude = _pContext->m_pObj->m_iObject;

	CVec3Dfp32 CharPos, Below;
	Below = CharPos = _pContext->m_pObj->GetPosition();
	CharPos.k[2] += 1.0f;
	Below.k[2] -= HANGRAIL_MAXDROPOFFHEIGHT;	

	//_pContext->m_pWPhysState->Debug_RenderWire(CharPos,Below,0xffffffff,10.0f);
	return CAGVal::From((_pContext->m_pWPhysState->Phys_IntersectLine(CharPos, Below, OwnFlags, ObjectFlags, MediumFlags, &CInfo, iExclude) ? FP32FALSE : FP32TRUE));
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_LadderStepoffType(const CWAGI_Context* _pContext, 
																   const CXRAG_ICallbackParams* _pParams)
{
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (pCD)
		return CAGVal::From((fp32)pCD->m_ControlMode_Param4);

	return CAGVal::From(0.0f);
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_EffectPlaying(const CWAGI_Context* _pContext, 
															   const CXRAG_ICallbackParams* _pParams)
{
	return CAGVal::From((fp32)(m_StateFlagsLoToken2 & CHAR_STATEFLAG_EFFECTOKENACTIVE));
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_CanCounter(const CWAGI_Context* _pContext, 
															const CXRAG_ICallbackParams* _pParams)
{
	/*if (_pContext->m_pWPhysState->IsClient())
		return CAGVal::From(FP32FALSE);*/

	// Okidoki then, if the target is playing a counter animation, return true
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (pCD && (pCD->m_iCloseObject != 0))
	{
		int32 iTarget = pCD->m_iCloseObject;
		CWObject_CoreData* pTarget = (iTarget ? _pContext->m_pWPhysState->Object_GetCD(iTarget) : NULL);
		CWO_Character_ClientData* pCDTarget = (pTarget ? CWObject_Character::GetClientData(pTarget) : NULL);
		// Check so that the distance between the characters isn't too long
		const int TargetPhysType = CWObject_Character::Char_GetPhysType(pTarget);
		if (pTarget && pCDTarget && IsAlive(TargetPhysType))
		{
			#define FIGHT_SQRDISTSTRICTNESS 2500
			// Check relative health on target
			fp32 RelativeHealth = pCDTarget->m_AnimGraph.GetRelativeHealth();

			int32 OppCounterType = pCDTarget->m_AnimGraph.GetCanCounterType();
			int32 Weapon = (int32)_pParams->GetParam(0);
			if ((Weapon > 0) && (OppCounterType != 0) &&
				(pTarget->GetPosition() - _pContext->m_pObj->GetPosition()).LengthSqr() < FIGHT_SQRDISTSTRICTNESS)
			{
				int32 Move, Response;
				fp32 MaxCounterTime = 0.0f;
				if (CRelativeAnimPos::GetCounterMove(Weapon, OppCounterType, Move, Response,MaxCounterTime,RelativeHealth))
				{
					fp32 Offset = (pCDTarget->m_GameTime - pCDTarget->m_AnimGraph.GetEnterStateTime()).GetTime();
					//ConOut(CStrF("%s: Counter: %d", (_pContext->m_pWPhysState->IsServer() ? "S" : "C"),pCDTarget->GetCanCounterType()));
					//ConOut("CHECKING COUNTER SUCCEEDED!!!");
					return CAGVal::From(Offset < MaxCounterTime);
				}
			}
		}
	}
	//ConOut("CHECKING COUNTER FAILED!!!!");
	return CAGVal::From(FP32FALSE);
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_RelAnimQueued(const CWAGI_Context* _pContext, 
															   const CXRAG_ICallbackParams* _pParams)
{
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	
	if (pCD)
		return CAGVal::From(pCD->m_RelAnimPos.HasQueued(pCD->m_GameTick));

	return CAGVal::From(FP32FALSE);
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_CanPlayFightFakeIdle(const CWAGI_Context* _pContext, 
																		 const CXRAG_ICallbackParams* _pParams)
{
	//return CAGVal::From(0.0f);
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (!pCD)
		return CAGVal::From(0.0f);
	return CAGVal::From((fp32)((m_CurrentAttack == 0) && (pCD->m_iPlayer == -1) &&
		!(m_StateFlagsLoToken2 & (CHAR_STATEFLAG_EFFECTOKENACTIVE|CHAR_STATEFLAG_HURTACTIVE)) && 
		!(m_StateFlagsLo & (CHAR_STATEFLAG_HURTACTIVE | CHAR_STATEFLAG_BLOCKACTIVE | CHAR_STATEFLAG_BEHAVIORACTIVE)) &&
		(m_PendingFightInterrupt == 0) && (m_MoveVeloHoriz < 5.0f) && 
		(CWObject_Character::Char_GetPhysType(_pContext->m_pObj) == PLAYER_PHYS_STAND)));
}

CAGVal CWO_Clientdata_Character_AnimGraph::Property_GPBitFieldBit(const CWAGI_Context* _pContext,
																  const CXRAG_ICallbackParams* _pParams)
{
	if (!_pParams || _pParams->GetNumParams() <= 0)
		return CAGVal::From(0.0f);

	// Checks if any of the given flags are in the bitfield
	int32 Flags = 0;
	int32 Len = _pParams->GetNumParams();
	for (int32 i = 0; i < Len; i++)
		Flags |= 1 << ((int32)_pParams->GetParam(i));

	return CAGVal::From((fp32)((m_GPBitField & Flags) != 0));
}
//--------------------------------------------------------------------------------

void CWO_Clientdata_Character_AnimGraph::Effect_SelectItem(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams)
{
	if (_pParams == NULL || _pParams->GetNumParams() == 0)
		return;
	CWObject_Message Msg(OBJMSG_CHAR_SELECTITEMBYTYPE, _pParams->GetParam(0));
	_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, _pContext->m_pObj->m_iObject);
}

//--------------------------------------------------------------------------------

void CWO_Clientdata_Character_AnimGraph::Effect_ActivateItem(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams)
{
	if ((_pParams == NULL) || (_pParams->GetNumParams() == 0) || 
		!_pContext->m_pWPhysState->IsServer())
		return;

	int ActivationMode = AG_ITEMACTIVATION_NORMAL;
	if (_pParams->GetNumParams() > 1)
		ActivationMode = _pParams->GetParam(1);

	if (ActivationMode == AG_ITEMACTIVATION_RELOAD)
	{
		CWObject_Message Msg(OBJMSG_CHAR_RELOADITEM, _pParams->GetParam(0));
		_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, _pContext->m_pObj->m_iObject);
	}
	else if (ActivationMode == AG_ITEMACTIVATION_FAIL)
	{
		//Don't activate if swtching weapon
	  	if (m_WeaponIdentifier == 0)
		{
			int Input = (_pParams->GetNumParams() > 2 ? _pParams->GetParam(2) : 0);
			// ConOut(CStrF("Activating item GameTime: %f", _pContext->m_pWPhysState->GetGameTime()));
			CWObject_Message Msg(OBJMSG_CHAR_ACTIVATEITEM, _pParams->GetParam(0), Input,-1,true);
			_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, _pContext->m_pObj->m_iObject);
		}
	}
	else
	//ConOut("Activate Item");
	//if (ActivationMode == AG_ITEMACTIVATION_NORMAL)
	{
		int Input = (_pParams->GetNumParams() > 2 ? _pParams->GetParam(2) : 0);
		// ConOut(CStrF("Activating item GameTime: %f", _pContext->m_pWPhysState->GetGameTime()));
		CWObject_Message Msg(OBJMSG_CHAR_ACTIVATEITEM, _pParams->GetParam(0), Input);
		_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, _pContext->m_pObj->m_iObject);
	}
	/*else
	{
		// FAILED ACTIVATION, PLEASE COME AGAIN
		 //ConOut("Failed ACtivation. (Yes this is a good thing function just not done yet)");
	}*/
}

//--------------------------------------------------------------------------------

void CWO_Clientdata_Character_AnimGraph::Effect_DeactivateItem(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams)
{
	if (_pParams == NULL || _pParams->GetNumParams() == 0)
		return;

	if (_pContext->m_pWPhysState->IsServer())
	{
		// ConOut(CStrF("Deactivating item GameTime: %f", _pContext->m_pWPhysState->GetGameTime()));
		CWObject_Message Msg(OBJMSG_CHAR_DEACTIVATEITEM, _pParams->GetParam(0), _pParams->GetParam(1));
		_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, _pContext->m_pObj->m_iObject);
	}
}

//--------------------------------------------------------------------------------

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Tries to find objects close to the character 
						that can be fought with, be picked up, 
						actioncutscenes or ledges

						A quite heavy function so it only runs on the 
						serverside (for now...)
						
	Parameters:			
		_pContext:		description
		_pParams:		description
						
\*____________________________________________________________________*/
#define ACTION_SELECTIONRADIUS (100.0f)
#define MAXPICKUPDISTANCE (50.0f)
#define FIGHTMODE_MAXHEIGHTDIFF (20.0f)
void CWO_Clientdata_Character_AnimGraph::Effect_PickupStuff(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams)
{
	// Ok, trying to pickup item... (only on server)
	if (_pContext->m_pWPhysState->IsClient() || (_pParams == NULL) || 
		(_pParams->GetNumParams() == 0) || !CWObject_Character::Char_IsPlayerViewControlled(_pContext->m_pObj))
		return;

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);

	// Don't allow the player to use stuff while immobile (e.g. while devouring)
	if (pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMOBILE)
		return;

	// Ok, this function should now be divided into check fight / pickups / use
	int PickupType = _pParams->GetParam(0);

	// Find and activate stuff... (only if no effect anim is playing)
	int32 iSel = -1;
	int32 iCloseSel = -1;
	bool bCurrentNoPrio = false;
	if (PickupType != AG_PICKUP_TYPELEDGE)
	{
		int8 SelType;
		CWObject_Character::Char_FindStuff(_pContext->m_pWPhysState,_pContext->m_pObj,pCD,iSel,
			iCloseSel, SelType,bCurrentNoPrio);

		if (iSel != -1)
		{
			if (((SelType & SELECTION_MASK_TYPEINVALID) == SELECTION_PICKUP) || !(m_StateFlagsLoToken2 & CHAR_STATEFLAG_EFFECTOKENACTIVE))
			{
				CWObject_Character::Char_ActivateStuff(_pContext->m_pWPhysState,_pContext->m_pObj,pCD,
					iSel,SelType);
				return;
			}
		}
	}

	// If no other objects were found, try to find a ledge instead
	if ((bCurrentNoPrio || (iSel == -1)) && !(m_StateFlagsLoToken2 & CHAR_STATEFLAG_EFFECTOKENACTIVE))
	{
		//ConOut("Grabbing ledge");
		CWObject_Character::Char_GrabLedge(_pContext->m_pWPhysState,_pContext->m_pObj,pCD);
	}
}

//--------------------------------------------------------------------------------

void CWO_Clientdata_Character_AnimGraph::Effect_RegisterUsedInput(const CWAGI_Context* _pContext, 
																const CXRAG_ICallbackParams* _pParams)
{
	if ((_pParams == NULL) || (_pParams->GetNumParams() == 0))
		return;

	// Register the given inputs to the usedbutton press stuff
	int NumParams = _pParams->GetNumParams();
	for (int i = 0; i < NumParams; i++)
	{
		int Button = _pParams->GetParam(i);
		Button = MACRO_GETBUTTONFROMINDEX(Button);
		if (Button != 0)
		{
			//ConOut(CStrF("RegButton: %.2x", (uint8)Button));
			m_UsedButtonPress |= Button;
		}
	}
}

void CWO_Clientdata_Character_AnimGraph::Effect_UnRegisterUsedInput(const CWAGI_Context* _pContext, 
																  const CXRAG_ICallbackParams* _pParams)
{
	if ((_pParams == NULL) || (_pParams->GetNumParams() == 0))
		return;

	// Register the given inputs to the usedbutton press stuff
	int NumParams = _pParams->GetNumParams();
	for (int i = 0; i < NumParams; i++)
	{
		int Button = _pParams->GetParam(i);
		Button = MACRO_GETBUTTONFROMINDEX(Button);
		if (Button != 0)
		{
			//ConOut(CStrF("RegButton: %.2x", (uint8)Button));
			m_UsedButtonPress &= ~Button;
		}
	}
}

//--------------------------------------------------------------------------------

void CWO_Clientdata_Character_AnimGraph::Effect_SetControlMode(const CWAGI_Context* _pContext,
															   const CXRAG_ICallbackParams* _pParams)
{
	// Set new controlmode
	if ((_pParams == NULL) || (_pParams->GetNumParams() == 0))
		return;

	/*if ((int) _pParams->GetParam(0) == PLAYER_CONTROLMODE_LEDGE)
		ConOut(CStrF("!!!! SETTING LEDGE CONTROLMODE!!!!!!! GT: %f Server: %d ",
		_pContext->m_pWPhysState->GetGameTime(),_pContext->m_pWPhysState->IsServer()));*/

	// If the controlmode is "lean", set camera crap....
	int32 ControlMode = (int) _pParams->GetParam(0);
	if (ControlMode == PLAYER_CONTROLMODE_LEAN)
	{
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
		if (pCD)
		{
			/*pCD->m_ActionCutSceneCameraOffsetX = pCD->m_Control_Look_Wanted[2];
			pCD->m_ACSLastAngleOffsetX = pCD->m_Control_Look_Wanted[2];
			pCD->m_ACSTargetAngleOffsetX = pCD->m_ACSLastAngleOffsetX;
			pCD->m_ACSAngleOffsetXChange = 0.0f;*/
			CCollisionInfo CInfo;
			CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
			int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
			int32 ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;
			int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
			int32 iExclude = _pContext->m_pObj->m_iObject;

			// Trace from character and a couple of units forward, if anything is hit we try to 
			// turn up against that plane
			CVec3Dfp32 Start,End,CharDir;
			CharDir = CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),0);
			CharDir.k[2] = 0.0f;
			CharDir.Normalize();
			_pContext->m_pObj->GetAbsBoundBox()->GetCenter(Start);
			End = Start + CharDir * 30;

			if (_pContext->m_pWPhysState->Phys_IntersectLine(Start, End, OwnFlags, ObjectFlags, MediumFlags, &CInfo, iExclude) && 
				CInfo.m_bIsValid && ((CInfo.m_Plane.n * CharDir) < -0.5f))
			{
				CVec3Dfp32 Direction = -CInfo.m_Plane.n;
				fp32 WantedAngleX = 1.0f - CVec3Dfp32::AngleFromVector(Direction[0], Direction[1]);
				// Ah yes we must moderate towards the closest angle as well...
				WantedAngleX += AngleAdjust(pCD->m_Control_Look_Wanted[2], WantedAngleX);
				pCD->m_ACSLastTargetDistance = WantedAngleX;

				//ConOut(CStrF("WantedAngleX: %f",WantedAngleX));
				pCD->m_FightLastAngleX = pCD->m_FightTargetAngleX = pCD->m_Control_Look_Wanted[2];
				pCD->m_FightAngleXChange = 0.0f;
			}
			else
			{
				// Original X angle
				pCD->m_ACSLastTargetDistance = pCD->m_Control_Look_Wanted[2];
			}

			pCD->m_ActionCutSceneCameraOffsetY = pCD->m_Control_Look_Wanted[1];
			pCD->m_ACSLastAngleOffsetY = pCD->m_Control_Look_Wanted[1];
			pCD->m_ACSTargetAngleOffsetY = pCD->m_ACSLastAngleOffsetY;
			pCD->m_ACSAngleOffsetYChange = 0.0f;
		}
	}

	CWObject_Character::Char_SetControlMode(_pContext->m_pObj, ControlMode);
}

//--------------------------------------------------------------------------------

/*void CWO_Clientdata_Character_AnimGraph::Effect_FightMove(const CWAGI_Context* _pContext,
														  const CXRAG_ICallbackParams* _pParams)
{
	// Find new fight position
	if ((_pParams == NULL) || (_pParams->GetNumParams() == 0))
		return;

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (pCD)
	{
		int Move = (int)_pParams->GetParam(0);
		int Stance = (int)_pParams->GetParam(1);
		CMoveTarget MoveTarget;
		if (MoveTarget.DoFightMove(_pContext->m_pWPhysState, _pContext->m_pObj, Move, Stance, 
			m_AnimLoopDuration))
		{
			pCD->m_FCharMovement.AddMoveTarget(MoveTarget);
			pCD->m_FCharMovement.MakeDirty();
		}
	}
}*/

//--------------------------------------------------------------------------------

// Clears data regarding a hit, used to signal animgraph that we have responded to a hit
void CWO_Clientdata_Character_AnimGraph::Effect_ClearHit(const CWAGI_Context* _pContext, 
														 const CXRAG_ICallbackParams* _pParams)
{
	OnHit_Clear();
}

//--------------------------------------------------------------------------------
enum
{
	EF_ERRORMSG_NOWALK = 1,
	EF_ERRORMSG_NORUN,
	EF_ERRORMSG_LEFT,
	EF_ERRORMSG_RIGHT,
	EF_ERRORMSG_FWD,
	EF_ERRORMSG_BWD,
	EF_ERRORMSG_NOJUMP,
	EF_ERRORMSG_NOACTIONCUTSCENETYPE,
	EF_ERRORMSG_NOLEDGEMOVE,
	EF_ERRORMSG_NOLEDGESWITCH,
};
void CWO_Clientdata_Character_AnimGraph::Effect_ErrorMsg(const CWAGI_Context* _pContext, 
														 const CXRAG_ICallbackParams* _pParams)
{
	if ((_pParams == NULL) || (_pParams->GetNumParams() == 0))
		return;

	int iMsg = _pParams->GetParam(0);
	switch (iMsg)
	{
	case EF_ERRORMSG_NOWALK:
		{
			ConOut("EF_ERRORMSG_NOWALK");
			break;
		}
	case EF_ERRORMSG_NORUN:
		{
			ConOut("EF_ERRORMSG_NORUN");
			break;
		}
	case EF_ERRORMSG_LEFT:
		{
			ConOut("EF_ERRORMSG_LEFT");
			break;
		}
	case EF_ERRORMSG_RIGHT:
		{
			ConOut("EF_ERRORMSG_RIGHT");
			break;
		}
	case EF_ERRORMSG_FWD:
		{
			ConOut("EF_ERRORMSG_FWD");
			break;
		}
	case EF_ERRORMSG_BWD:
		{
			ConOut("EF_ERRORMSG_BWD");
			break;
		}
	case EF_ERRORMSG_NOJUMP:
		{
			ConOut("EF_ERRORMSG_NOJUMP");
			break;
		}
	case EF_ERRORMSG_NOACTIONCUTSCENETYPE:
		{
			ConOut("EF_ERRORMSG_NOACTIONCUTSCENETYPE");
			break;
		}
	case EF_ERRORMSG_NOLEDGEMOVE:
		{
			ConOut("EF_ERRORMSG_NOLEDGEMOVE");
			break;
		}
	case EF_ERRORMSG_NOLEDGESWITCH:
		{
			ConOut("EF_ERRORMSG_NOLEDGESWITCH");
			break;
		}
	default:
		{
			ConOut("EF_ERRORMSG_UNDEFINED MESSAGE");
			break;
		}
	};
}

//--------------------------------------------------------------------------------

void CWO_Clientdata_Character_AnimGraph::Effect_ActionCutsceneSwitch(const CWAGI_Context* _pContext, 
																	 const CXRAG_ICallbackParams* _pParams)
{
	// Only on server (for now...)
	if (/*_pContext->m_pWPhysState->IsClient() || */(_pParams == NULL) || (_pParams->GetNumParams() == 0))
		return;

	// Do actioncutscene stuff, type is given in param0
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);

	if (!pCD)
		return;

	int iACS, ACSActionType;
	iACS = (int)pCD->m_ControlMode_Param0;

	// Get type of action to perform from the animgraph
	ACSActionType = _pParams->GetParam(0);

	if (_pContext->m_pWPhysState->IsClient())
	{
		if (ACSActionType == AG_ACSACTIONTYPE_ENDANIMATION)
		{
			//pCD->m_CameraUtil.Clear();
			//pCD->m_CameraUtil.MakeDirty();
			CActionCutsceneCamera::SetInvalidToPcd(pCD);
			pCD->m_ExtraFlags = pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_THIRDPERSON;
		}
		return;
	}

	switch (ACSActionType)
	{
	case AG_ACSACTIONTYPE_STARTANIMATION:
		{
			//ConOut("ACS: START OF ANIM");
			// At the start of an animation, set thirdperson, set startposition and disable physics
			int ACSMode = pCD->m_ControlMode_Param4;
			
			if (ACSMode & ACTIONCUTSCENE_OPERATION_USETHIRDPERSON)
			{
				// Send a message that finds a new cutscenecamera
				/*CActionCutsceneCamera* pCamera = NULL;
				if (_pContext->m_pWPhysState->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_ACTIONCUTSCENE_GETNEWCUTSCENECAMERA, 0, 0, 
					_pContext->m_pObj->m_iObject,0,0,0,&pCamera), iACS) && pCamera != NULL)
				{
					// Activate the thirdperson camera

					// Use the given cutscene camera somehow (override first person cam)
					// (don't forget to enable it, and disable in when returning to first person)
					if (pCamera)
					{
						pCamera->SetActive();
						pCamera->OnRefresh();
						pCamera->SetValid(pCD);
					}

					// Set thirdperson flag so the head will be shown
					// Ugly hack for now (we now it's on the server atleast)
					CWObject* pObj = (CWObject*)_pContext->m_pObj;
					pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_THIRDPERSON;
					pObj->m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
				}*/
				CCameraUtil* pCamUtil = &(pCD->m_CameraUtil);
				if (_pContext->m_pWPhysState->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_ACTIONCUTSCENE_GETNEWCUTSCENECAMERA, 0, 0, 
					_pContext->m_pObj->m_iObject,0,0,0,pCamUtil), iACS))
				{
					pCD->m_CameraUtil.MakeDirty();
					// Set thirdperson flag so the head will be shown
					// Ugly hack for now (we know it's on the server atleast)
					CWObject* pObj = (CWObject*)_pContext->m_pObj;
					pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_THIRDPERSON;
					pObj->m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
				}
			}

			if (ACSMode & ACTIONCUTSCENE_OPERATION_DISABLEPHYSICS)
			{
				// Add these flags the the client flags
				int DisablePhysicsFlags = PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK | 
					PLAYER_CLIENTFLAGS_NOGRAVITY;

				_pContext->m_pWPhysState->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_CHAR_SETCLIENTFLAGS, DisablePhysicsFlags, 0, 
					_pContext->m_pObj->m_iObject), _pContext->m_pObj->m_iObject);
			}

			if (ACSMode & ACTIONCUTSCENE_OPERATION_SETSTARTPOSMASK)
			{
				CMat4Dfp32 PosMat;
				if (CWObject_ActionCutscene::GetStartPosition(PosMat, pCD, _pContext->m_pWPhysState))
				{
					CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(PosMat,3);
					// Make sure we don't collide with floor
					Pos.k[2] += 0.001f;
					Pos.SetMatrixRow(PosMat, 3);
					_pContext->m_pWPhysState->Object_SetPosition(_pContext->m_pObj->m_iObject, 
						PosMat);
					_pContext->m_pWPhysState->Object_SetVelocity(_pContext->m_pObj->m_iObject,
						CVec3Dfp32(0.0f));
				}
			}

			break;
		}
	case AG_ACSACTIONTYPE_ENDANIMATION:
		{
			// At the end of an animation, set firstperson, set endposition enable physics, and 
			// set crouch
			//ConOut("ACS: END OF ANIM");

			// At the start of an animation, set thirdperson, set startposition and disable physics
			int ACSMode = pCD->m_ControlMode_Param4;

			if (true)//(ACSMode & ACTIONCUTSCENE_OPERATION_USETHIRDPERSON)
			{
				// Remove thirdperson flag so the head won't be shown
				// Don't forget to disable cutscene camera....!!!
				//pCD->m_CameraUtil.Clear();
				//pCD->m_CameraUtil.MakeDirty();
				CActionCutsceneCamera::SetInvalidToPcd(pCD);
				pCD->m_ExtraFlags = pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_THIRDPERSON;
				// Ugly hack for now (we now it's on the server atleast)
				/*CWObject* pObj = (CWObject*)_pContext->m_pObj;
				pObj->m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;*/
			}

			if (ACSMode & ACTIONCUTSCENE_OPERATION_DISABLEPHYSICS)
			{
				// Remove these flags the the client flags
				int DisablePhysicsFlags = PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK | 
					PLAYER_CLIENTFLAGS_NOGRAVITY | PLAYER_CLIENTFLAGS_FORCECROUCH;
				_pContext->m_pWPhysState->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_CHAR_SETCLIENTFLAGS, 0, DisablePhysicsFlags, 
					_pContext->m_pObj->m_iObject), _pContext->m_pObj->m_iObject);
			}

			if (ACSMode & ACTIONCUTSCENE_OPERATION_CROUCHATEND)
			{
				// Crouch the character
				// Make the target character crouch somehow.... server might not be needed (check it)
				CWObject_Character::Char_SetPhysics(_pContext->m_pObj, _pContext->m_pWPhysState, 
					NULL, PLAYER_PHYS_CROUCH, false, false);
			}

			if (ACSMode & ACTIONCUTSCENE_OPERATION_SETENDPOSMASK)
			{
				// Set the position of the character from object defined the actioncutscene
				int iEndPosition = _pContext->m_pWPhysState->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_ACTIONCUTSCENE_GETENDPOSITION), iACS);

				if ((iEndPosition != -1) && (iEndPosition != 0))
				{
					CMat43fp32 PosMat;
					if (!_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_HOOK_GETCURRENTMATRIX,aint(&PosMat)),iEndPosition))
						PosMat = _pContext->m_pWPhysState->Object_GetPositionMatrix(iEndPosition);
					CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(PosMat,3);
					// Make sure we don't collide with floor
					Pos.k[2] += 0.001f;
					Pos.SetMatrixRow(PosMat, 3);
					_pContext->m_pWPhysState->Object_SetPosition(_pContext->m_pObj->m_iObject, 
						PosMat);
					_pContext->m_pWPhysState->Object_SetVelocity(_pContext->m_pObj->m_iObject,
						CVec3Dfp32(0.0f));
				}
			}
			_pContext->m_pWPhysState->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_ACTIONCUTSCENE_ONENDACS,0,0,_pContext->m_pObj->m_iObject), iACS);
			break;
		}
	// SEPARATE IMPLEMENTATIONS, to be used by other than actioncutscene
	case AG_ACSACTIONTYPE_DISABLEPHYSICS:
		{
			//ConOut("DISABLE PHYS");
			// Add these flags the the client flags
			int DisablePhysicsFlags = PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK | 
				PLAYER_CLIENTFLAGS_NOGRAVITY;

			_pContext->m_pWPhysState->Phys_Message_SendToObject(
				CWObject_Message(OBJMSG_CHAR_SETCLIENTFLAGS, DisablePhysicsFlags, 0, 
				_pContext->m_pObj->m_iObject), _pContext->m_pObj->m_iObject);
			if (GetStateFlagsLo() & CHAR_STATEFLAG_NOPHYSFLAG)
			{
				// Set physics
				CWObject_Character::Char_SetPhysics(_pContext->m_pObj, _pContext->m_pWPhysState, 
					NULL, CWObject_Character::Char_GetPhysType(_pContext->m_pObj), false, true);
			}
			break;
		}
	case AG_ACSACTIONTYPE_ENABLEPHYSICS:
		{
			//ConOut("ENABLE PHYS");
			// Add these flags the the client flags
			int DisablePhysicsFlags = PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK | 
				PLAYER_CLIENTFLAGS_NOGRAVITY | PLAYER_CLIENTFLAGS_FORCECROUCH;
			_pContext->m_pWPhysState->Phys_Message_SendToObject(
				CWObject_Message(OBJMSG_CHAR_SETCLIENTFLAGS, 0, DisablePhysicsFlags, 
				_pContext->m_pObj->m_iObject), _pContext->m_pObj->m_iObject);
			break;
		}
	case AG_ACSACTIONTYPE_SETFIRSTPERSON:
		{
			// Remove thirdperson flag so the head won't be shown
			// Don't forget to disable cutscene camera....!!!
			//pCD->m_CameraUtil.Clear();
			//pCD->m_CameraUtil.MakeDirty();
			CActionCutsceneCamera::SetInvalidToPcd(pCD);
			pCD->m_ExtraFlags = pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_THIRDPERSON;
			// Send OnEndAcs message (this should only be used by ladders/ledges/acs anyway)	
			_pContext->m_pWPhysState->Phys_Message_SendToObject(
				CWObject_Message(OBJMSG_ACTIONCUTSCENE_ONENDACS,0,0,_pContext->m_pObj->m_iObject), iACS);
			break;
		}
	case AG_ACSACTIONTYPE_CROUCH:
		{
			// Crouch the character
			// Make the target character crouch somehow.... server might not be needed (check it)
			CWObject_Character::Char_SetPhysics(_pContext->m_pObj, _pContext->m_pWPhysState, 
				NULL, PLAYER_PHYS_CROUCH, false, false);
			break;
		}
	case AG_ACSACTIONTYPE_SETTHIRDPERSON:
		{
			// Set thirdperson flag so the head will be shown
			pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_THIRDPERSON;
			break;
		}
	case AG_ACSACTIONTYPE_PICKUPITEM:
		{
			// Pickup the item
			_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_RPG_AVAILABLEPICKUP, 
						_pContext->m_pObj->m_iObject, 0, iACS), iACS);

			/*int Type = (int)pCD->m_ControlMode_Param2;
			if ((Type == ACTIONCUTSCENE_TYPE_PICKUPITEMGROUND) || 
				(Type == ACTIONCUTSCENE_TYPE_PICKUPITEMTABLE))
			{
				// Pickup the item
				_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_RPG_AVAILABLEPICKUP, 
							_pContext->m_pObj->m_iObject, 0, iACS), iACS);
			}*/
			break;
		}
	// Activates the triggers (temporary for success for now)
	case AG_ACSACTIONTYPE_DOTRIGGER:
		{
			//ConOut(CStrF("Time: %f Doing trigger!!",_pContext->m_GameTime));
			// Send the trigger messages in the actioncutscene
			_pContext->m_pWPhysState->Phys_Message_SendToObject(
				CWObject_Message(OBJMSG_ACTIONCUTSCENE_DOTRIGGER,0,0,_pContext->m_pObj->m_iObject), iACS);
			break;
		}
	case AG_ACSACTIONTYPE_DOTRIGGERMIDDLE:
		{
			_pContext->m_pWPhysState->Phys_Message_SendToObject(
				CWObject_Message(OBJMSG_ACTIONCUTSCENE_DOTRIGGERMIDDLE,0,0,_pContext->m_pObj->m_iObject), iACS);
			break;
		}
	case AG_ACSACTIONTYPE_DOTRIGGERREFILL:
		{
			_pContext->m_pWPhysState->Phys_Message_SendToObject(
				CWObject_Message(OBJMSG_ACTIONCUTSCENE_DOTRIGGERREFILL,0,0,_pContext->m_pObj->m_iObject), iACS);
			break;
		}
	case AG_ACSACTIONTYPE_SETLEVERSTATE:
		{
			if (_pParams->GetNumParams() > 1)
			{
				int32 State = _pParams->GetParam(1);
				_pContext->m_pWPhysState->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_ACTIONCUTSCENE_LEVER_SETSTATE,State,0,_pContext->m_pObj->m_iObject), iACS);
			}
			break;
		}
	case AG_ACSACTIONTYPE_ONCHANGEVALVESTATE:
		{
			if (_pParams->GetNumParams() > 1)
			{
				int32 State = _pParams->GetParam(1);
				_pContext->m_pWPhysState->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_ACTIONCUTSCENE_VALVE_ONCHANGESTATE,State,0,_pContext->m_pObj->m_iObject), iACS);
			}
			break;
		}
	case AG_ACSACTIONTYPE_USEACSCAMERA:
		{
			//ConOut("ACS: START OF ANIM");
			// At the start of an animation, set thirdperson, set startposition and disable physics
			int ACSMode = pCD->m_ControlMode_Param4;
			
			if (ACSMode & ACTIONCUTSCENE_OPERATION_USETHIRDPERSON)
			{
				// Send a message that finds a new cutscenecamera
				/*CActionCutsceneCamera* pCamera = NULL;
				if (_pContext->m_pWPhysState->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_ACTIONCUTSCENE_GETNEWCUTSCENECAMERA, 0, 0, 
					_pContext->m_pObj->m_iObject,0,0,0,&pCamera), iACS) && pCamera != NULL)
				{
					// Activate the thirdperson camera

					// Use the given cutscene camera somehow (override first person cam)
					// (don't forget to enable it, and disable in when returning to first person)
					if (pCamera)
					{
						pCamera->SetActive();
						pCamera->OnRefresh();
						pCamera->SetValid(pCD);
					}

					// Set thirdperson flag so the head will be shown
					// Ugly hack for now (we now it's on the server atleast)
					CWObject* pObj = (CWObject*)_pContext->m_pObj;
					pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_THIRDPERSON;
					pObj->m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
				}*/
				CCameraUtil* pCamUtil = &(pCD->m_CameraUtil);
				if (_pContext->m_pWPhysState->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_ACTIONCUTSCENE_GETNEWCUTSCENECAMERA, 0, 0, 
					_pContext->m_pObj->m_iObject,0,0,0,pCamUtil), iACS))
				{
					pCD->m_CameraUtil.MakeDirty();
					// Set thirdperson flag so the head will be shown
					// Ugly hack for now (we know it's on the server atleast)
					CWObject* pObj = (CWObject*)_pContext->m_pObj;
					pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_THIRDPERSON;
					pObj->m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
				}
			}
			break;
		}
	default:
		{
			break;
		}
	}
}

//--------------------------------------------------------------------------------

void CWO_Clientdata_Character_AnimGraph::Effect_SendImpulse(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams)
{
	if ((_pParams == NULL) || (_pParams->GetNumParams() == 0))
		return;

	CWObject_Message Msg(OBJMSG_IMPULSE,_pParams->GetParam(0));
	_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, _pContext->m_pObj->m_iObject);
}

//--------------------------------------------------------------------------------

void CWO_Clientdata_Character_AnimGraph::Effect_PauseTokens(const CWAGI_Context* _pContext,
														   const CXRAG_ICallbackParams* _pParams)
{
    // Move the hurt/response tokens to idle since we can't move multiple tokens in the animgraph yet
	// Movetoken on both the character and ourselves
	CStr MoveToken("Main_ResponseIdle.Action_GotoMe");
	int NumParams = _pParams->GetNumParams();	
	for (int i = 0; i < NumParams; i++)
	{
		int iToken = (int)_pParams->GetParam(i);
		CWObject_Message IdleToken(OBJMSG_CHAR_MOVETOKEN,iToken,0,0,0,0,0,MoveToken.GetStr());
		_pContext->m_pWPhysState->Phys_Message_SendToObject(IdleToken,_pContext->m_pObj->m_iObject);
	}
}

//--------------------------------------------------------------------------------

void CWO_Clientdata_Character_AnimGraph::Effect_DrainStamina(const CWAGI_Context* _pContext, 
															 const CXRAG_ICallbackParams* _pParams)
{
	// Used wrong if less than 2 param
	if (_pParams == NULL || _pParams->GetNumParams() < 2)
		return;

	int StaminaDrain = (int)_pParams->GetParam(0);
	int StaminaTarget = (int)_pParams->GetParam(1);
	if (StaminaTarget == AG_FIGHTMODE_STAMINATARGET_OPPONENT)
	{	
		// Check opponent and see if any damage can be sent
		// Find fightmode flags from opponent
		int iOpponent = _pContext->m_pWPhysState->Phys_Message_SendToObject(
			CWObject_Message(OBJMSG_CHAR_ISINFIGHTMODE), _pContext->m_pObj->m_iObject);

		// No opponent, then we can do whatever moves we want
		if (iOpponent == -1)
			return;
	}
	else
	{
		// Target is ourselves
	}
	// Drain stamina
	switch (StaminaDrain)
	{
	case AG_FIGHTMODE_STAMINADRAIN_LOW:
		{
			break;
		}
	case AG_FIGHTMODE_STAMINADRAIN_MEDIUM:
		{
			break;
		}
	case AG_FIGHTMODE_STAMINADRAIN_HIGH:
		{
			break;
		}
	default:
		{
			break;
		}
	}
}

//--------------------------------------------------------------------------------

void CWO_Clientdata_Character_AnimGraph::Effect_MessageSender(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams)
{
	if ((_pParams == NULL) || (_pParams->GetNumParams() == 0))
		return;

	int MessageType = _pParams->GetParam(0);
	switch(MessageType)
	{
	case AG_MESSAGESENDER_SETGHOSTMODE:
		{
			CWObject_Message Msg(OBJMSG_CHAR_GHOST, _pParams->GetParam(1));
			_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, _pContext->m_pObj->m_iObject);
			break;
		}
	case AG_MESSAGESENDER_KILLYOURSELF:
		{
			_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(
				OBJMSG_CHAR_KILLPLAYER), _pContext->m_pObj->m_iObject);
			break;
		}
	case AG_MESSAGESENDER_SWITCHLEDGE:
		{
			// Find ledge to switch to
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
			if (!pCD)
				break;

			int iLedge = (int)pCD->m_ControlMode_Param0;
			
			// Which ledge to switch to in param 1
			CWObject_Message Msg(OBJMSG_LEDGE_SWITCHLEDGE, (int)_pParams->GetParam(1),0,
				_pContext->m_pObj->m_iObject, 0,0,0,(CWObject_CoreData*)_pContext->m_pObj);
			_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, iLedge);
			break;
		}
	case AG_MESSAGESENDER_FISTSETSHOULDBEDEAD:
		{
			/*CRPG_Object_Fist* pFist = (CRPG_Object_Fist*)_pContext->m_pWPhysState->Phys_Message_SendToObject(
			CWObject_Message(OBJMSG_CHAR_GETITEM, CRPG_Object_Fist_ItemType, 0, _pContext->m_pObj->m_iObject), 
				_pContext->m_pObj->m_iObject);
			if (pFist)
				pFist->SetShouldBeDead();*/
			CAI_Core* pAI = NULL;
			_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_AIQUERY_GETAI,0,0,0,0,0,0,&pAI), _pContext->m_pObj->m_iObject);
			if (pAI)
				pAI->m_KB.SetAlertness(CAI_KnowledgeBase::ALERTNESS_OBLIVIOUS);

			break;
		}
	case AG_MESSAGESENDER_SETFORCECROUCH:
		{
			bool bSet = _pParams->GetParam(1) != 0.0f;

			int32 EnableFlags = (bSet ? PLAYER_CLIENTFLAGS_FORCECROUCH : 0);
			int32 DisableFlags = (bSet ? 0 : PLAYER_CLIENTFLAGS_FORCECROUCH);
			CWObject_Message ForceCrouch(OBJMSG_CHAR_SETCLIENTFLAGS, EnableFlags, DisableFlags, 
				_pContext->m_pObj->m_iObject);
			_pContext->m_pWPhysState->Phys_Message_SendToObject(ForceCrouch,_pContext->m_pObj->m_iObject);
			// Set the new physics
			int32 PhysType = (bSet ? PLAYER_PHYS_CROUCH : PLAYER_PHYS_STAND);
			CWObject_Character::Char_SetPhysics(_pContext->m_pObj, _pContext->m_pWPhysState, 
				NULL, PhysType, false, false);

			break;
		}
	case AG_MESSAGESENDER_INVALIDATEMOVE:
		{
			// Invalidate fightmove
			// Find ledge to switch to
			/*CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
			if (pCD)
			{	
				pCD->m_FCharMovement.Invalidate();
				pCD->m_FCharMovement.MakeDirty();
			}*/
			break;
		}
	case AG_MESSAGESENDER_KILLME:
		{
			_pContext->m_pWPhysState->Phys_Message_SendToObject(
				CWObject_Message(OBJMSG_CHAR_KILLPLAYER), _pContext->m_pObj->m_iObject);
			
			break;
		}
	case AG_MESSAGESENDER_DROPBODY:
		{
			_pContext->m_pWPhysState->Phys_Message_SendToObject(
				CWObject_Message(OBJMSG_CHAR_SETGRABBEDBODY,0), _pContext->m_pObj->m_iObject);
			/*CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
			if (pCD)
				pCD->m_Item0_Flags = pCD->m_Item0_Flags & ~RPG_ITEM_FLAGS_NORENDERMODEL;*/
			// Stop drag sound
			if (_pContext->m_pWPhysState->IsServer())
			{
				CWorld_Server* pWServer = safe_cast<CWorld_Server>(_pContext->m_pWPhysState);
				if (pWServer)
				{
					if(_pContext->m_pObj->m_iSound[1] != 0)
					{
						_pContext->m_pObj->m_iSound[1] = 0;
						_pContext->m_pObj->m_iSound[1] |= CWO_DIRTYMASK_SOUND;
					}
				}
			}
			break;
		}
	case AG_MESSAGESENDER_SETCROUCH:
		{
			// Crouch the character
			int32 Phys = (_pParams->GetParam(1) ? PLAYER_PHYS_CROUCH : PLAYER_PHYS_STAND);
			// Make the target character crouch somehow.... server might not be needed (check it)
			CWObject_Character::Char_SetPhysics(_pContext->m_pObj, _pContext->m_pWPhysState, 
				NULL, Phys, false, false);
			if (Phys != PLAYER_PHYS_CROUCH)
			{
				CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
				if (pCD)
					pCD->m_Control_Press &= ~CONTROLBITS_CROUCH;
			}
		}
	case AG_MESSAGESENDER_DRAGBODYSOUND:
		{
			// Start sound
			if (_pContext->m_pWPhysState->IsServer())
			{
				CWorld_Server* pWServer = safe_cast<CWorld_Server>(_pContext->m_pWPhysState);
				if (pWServer)
				{
					// Override current sound?
					if (_pParams->GetParam(1) != 0)
					{
						//pWServer->Sound_At(_pContext->m_pObj->GetPosition(), pWServer->GetMapData()->GetResourceIndex_Sound("D_Body_Dragdoll001"), 0);
						_pContext->m_pObj->m_iSound[1] = pWServer->GetMapData()->GetResourceIndex_Sound("D_Body_Dragdoll001");
						pWServer->Sound_On(_pContext->m_pObj->m_iObject, _pContext->m_pObj->m_iSound[1], 2);
					}
					else
					{
						if(_pContext->m_pObj->m_iSound[1] != 0)
						{
							_pContext->m_pObj->m_iSound[1] = 0;
							_pContext->m_pObj->m_iSound[1] |= CWO_DIRTYMASK_SOUND;
						}
					}
				}
			}
			break;
		}
	case AG_MESSAGESENDER_TERMINATETOKENS:
		{
			CStr MoveToken("TerminateMe.Action_Die");
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
			if (pCD)
			{
				CWObject_Character::MoveToken(_pContext->m_pObj,_pContext->m_pWPhysState,pCD,AG_TOKEN_RESPOSE,MoveToken,CMTime(),0.0f,3);
				CWObject_Character::MoveToken(_pContext->m_pObj,_pContext->m_pWPhysState,pCD,AG_TOKEN_EFFECT,MoveToken,CMTime(),0.0f,3);
				CWObject_Character::MoveToken(_pContext->m_pObj,_pContext->m_pWPhysState,pCD,AG_TOKEN_WALLCOLLISION,MoveToken,CMTime(),0.0f,3);
			}
			break;
		}
	case AG_MESSAGESENDER_DROPWEAPONS:
		{
			_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_FORCEDROPWEAPON),_pContext->m_pObj->m_iObject);
			break;
		}
	case AG_MESSAGESENDER_SWITCHWEAPONS:
		{
			// Yes, the AG can call this twice or even more on the same tick or some tick aftter
			// when checking animloopcount, hack to counter that
			bool bDropCurrent = _pParams->GetParam(1) != 0;
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
			if (!pCD || pCD->m_GameTick <= m_LastFightDamage)
				break;
			
			m_LastFightDamage = pCD->m_GameTick + 1;

			// Deactivate weapon
			// ConOut(CStrF("Deactivating item GameTime: %f", _pContext->m_pWPhysState->GetGameTime()));
			CWObject_Message Msg(OBJMSG_CHAR_DEACTIVATEITEM,AG_ITEMSLOT_WEAPONS,1);
			_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg, _pContext->m_pObj->m_iObject);

			if (bDropCurrent)
				_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_FORCEDROPWEAPON),_pContext->m_pObj->m_iObject);
			
			// Bleh, I kinda hate it that the AG calls this more than once
			if (_pContext->m_pWPhysState->IsServer())
			{
				//Old system, just switch to next: if (_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBGMSG_CHAR_NEXTWEAPON),_pContext->m_pObj->m_iObject))

				//New system; switch to currently selected
				if (_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBGMSG_CHAR_SWITCHITEMBYIDENTIFIER, AG_ITEMSLOT_WEAPONS, m_WeaponIdentifier),_pContext->m_pObj->m_iObject))
				{
					m_bQueueWeaponSelect = false;
					//ConOut("DOING NEXT ITEM!!!");
					// Move main token to weaponswitch
					CWObject_Character::MoveToken(_pContext->m_pObj,_pContext->m_pWPhysState,pCD,AG_TOKEN_MAIN,
						CStr("WeaponSwitch_Equip.Action_GotoMe"));
					SetWeaponIdentifier();
				}
			}
			break;
		}
	case AG_MESSAGESENDER_CROUCH:
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
			if (!pCD || pCD->m_GameTick <= m_LastCrouchAttempt)
				break;

			m_LastCrouchAttempt = pCD->m_GameTick + 4;
			if (_pContext->m_pWPhysState->IsServer())
				ConExecute("togglecrouch()");
			break;
		}
	case AG_MESSAGESENDER_RESETUNEQUIPCOUNTER:
		{
			_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_RESETWEAPONUNEQUIPTIMEOUT),_pContext->m_pObj->m_iObject);
			break;
		}
	default:
		break;
	}
}

void CWO_Clientdata_Character_AnimGraph::Effect_ClearFightInterrupt(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams)
{
	if (_pParams && _pParams->GetParam(0) == AG_CLEARINTERRUPT_TYPE_SENT)
		m_LastSentFightInterrupt = 0;
	else
		m_PendingFightInterrupt = 0;
}

//--------------------------------------------------------------------------------

void CWO_Clientdata_Character_AnimGraph::Effect_ResetControlmodeParams(const CWAGI_Context* _pContext, 
																	   const CXRAG_ICallbackParams* _pParams)
{
	if (_pContext->m_pWPhysState->IsClient())
		return;

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);

	if (pCD)
	{
		pCD->m_ControlMode_Param0 = 0;
		pCD->m_ControlMode_Param1 = 0;
		pCD->m_ControlMode_Param2 = 0;
		pCD->m_ControlMode_Param3 = 0;
	}
}

void CWO_Clientdata_Character_AnimGraph::Effect_UnregisterHandledResponse(const CWAGI_Context* _pContext, 
																		const CXRAG_ICallbackParams* _pParams)
{
	if (_pParams == NULL || _pParams->GetNumParams() == 0)
		return;

	m_HandledResponse &= ~((uint8)_pParams->GetParam(0));
}

void CWO_Clientdata_Character_AnimGraph::Effect_AIBehaviorSignal(const CWAGI_Context* _pContext, 
															   const CXRAG_ICallbackParams* _pParams)
{
	if (_pParams == NULL || _pParams->GetNumParams() == 0)
		return;

	// Ok signal the AI somehow....
	int32 Signal = (int32)_pParams->GetParam(0);

	switch (Signal)
	{
	case AG_BEHAVIOR_SIGNAL_ENDOFBEHAVIOR:
		{
			m_PendingFightInterrupt = 0;
			OnHit_Clear();
			// Signal the AI associated with m_iObject that the behaviour animation has finished
			// Param0 is the behaviour and iSender is (naturally) the object id
			int32 Behavior = (int32)_pParams->GetParam(1);
			CWObject_Message Msg(OBJMSG_AIEFFECT_ENDOFBEHAVIOR,Behavior,0,_pContext->m_pObj->m_iObject);
			_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg,_pContext->m_pObj->m_iObject);
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
			if (pCD)
			{
#ifdef _DEBUG
//				ConOutL(CStrF("iObj: %d 1: STOPPING Current: %d",_pContext->m_pObj->m_iObject,pCD->m_AnimGraph.m_WantedBehavior));
//				ConOutL(CStrF("iObj: %d 2: STOPPING Current: %d",_pContext->m_pObj->m_iObject,pCD->m_AnimGraph.m_WantedBehavior));
//				ConOutL(CStrF("iObj: %d 3: STOPPING Current: %d",_pContext->m_pObj->m_iObject,pCD->m_AnimGraph.m_WantedBehavior));
#endif
				pCD->m_AnimGraph.m_WantedBehavior = 0;
			}

			break;
		}
	case AG_BEHAVIOR_SIGNAL_STARTOFBEHAVIOR:
		{
			//Signal gamesystem that the main animation of the current behaviour has started
			// Param0 is the behaviour and iSender is (naturally) the object id
			int32 Behavior = (int32)_pParams->GetParam(1);
			CWObject_Message Msg(OBJMSG_AIEFFECT_STARTOFBEHAVIOR,Behavior,0,_pContext->m_pObj->m_iObject);
			_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg,_pContext->m_pObj->m_iObject);

			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
			if (pCD)
			{
#ifdef _DEBUG
//				ConOutL(CStrF("iObj: %d 1: STARTING Current: %d",_pContext->m_pObj->m_iObject,pCD->m_AnimGraph.m_WantedBehavior));
//				ConOutL(CStrF("iObj: %d 2: STARTING Current: %d",_pContext->m_pObj->m_iObject,pCD->m_AnimGraph.m_WantedBehavior));
//				ConOutL(CStrF("iObj: %d 3: STARTING Current: %d",_pContext->m_pObj->m_iObject,pCD->m_AnimGraph.m_WantedBehavior));
#endif
			}
			break;
		}
	default:
		break;
	};
}

// Register what behaviors there are in the current animgraph
void CWO_Clientdata_Character_AnimGraph::Effect_AIRegisterBehavior(const CWAGI_Context* _pContext, 
																   const CXRAG_ICallbackParams* _pParams)
{
	if (_pContext->m_pWPhysState->IsClient() || _pParams == NULL || _pParams->GetNumParams() == 0)
		return;

	int Reg = (int32)_pParams->GetParam(0);
	switch(Reg)
	{
	case REGISTERBEHAVIOR_RESET:
		{
			_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_AIEFFECT_SUPPORTED_BEHAVIOUR,0),
				_pContext->m_pObj->m_iObject);
			break;
		}
	case REGISTERBEHAVIOR_ADD:
		{
			int32 Behavior = _pParams->GetParam(1);
			_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_AIEFFECT_SUPPORTED_BEHAVIOUR,Behavior),
				_pContext->m_pObj->m_iObject);
			break;
		}
	/*case REGISTERBEHAVIOR_REMOVE:
		{
			int32 Behavior = _pParams->GetParam(1);
			_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(-1,Behavior),
				_pContext->m_pObj->m_iObject);
			break;
		}*/
	default:
		break;
	}
}

//--------------------------------------------------------------------------------

void CWO_Clientdata_Character_AnimGraph::Effect_RelAnimMoveSignal(const CWAGI_Context* _pContext, 
																  const CXRAG_ICallbackParams* _pParams)
{
	if (!_pParams || _pParams->GetNumParams() <= 0)
		return;

	// Ok, signal start/stop to relanimmove (only server?)
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (pCD)
	{
		if (pCD->m_RelAnimPos.Signal((int32)_pParams->GetParam(0),_pContext, pCD->m_AnimGraph.GetAGI(), (_pParams->GetParam(1) != 0.0f)))
			pCD->m_RelAnimPos.MakeDirty();
	}
}

void CWO_Clientdata_Character_AnimGraph::Effect_LedgeMove(const CWAGI_Context* _pContext, 
														  const CXRAG_ICallbackParams* _pParams)
{
	if (!_pParams || _pParams->GetNumParams() <= 0)
		return;

	int32 Move = (int32)_pParams->GetParam(0);
	CWObject_Ledge::MakeLedgeMove(_pContext,Move);
}

void CWO_Clientdata_Character_AnimGraph::Effect_LadderMove(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams)
{
	if (!_pParams || _pParams->GetNumParams() <= 0)
		return;

	int32 Move = (int32)_pParams->GetParam(0);
	CWObject_Phys_Ladder::MakeLadderMove(_pContext->m_pWPhysState,_pContext->m_pObj,Move);
}

void CWO_Clientdata_Character_AnimGraph::Effect_FightSignal(const CWAGI_Context* _pContext,
															const CXRAG_ICallbackParams* _pParams)
{
	// Only server, only have rpg there!
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (!pCD || (pCD->m_GameTick <= m_LastFightDamage) || _pContext->m_pWPhysState->IsClient() || 
		!_pParams || (_pParams->GetNumParams() < 2))
		return;

	int32 Type = (int32) _pParams->GetParam(0);
	int32 Value = (int32) _pParams->GetParam(1);

	// Still calls this function twice sometimes OR NOT AT ALL!!!!!!!!!!&!&!&, which sucks

	//ConOut(CStrF("Sending signal: %d",Type));

	switch (Type)
	{
	case AG_FIGHTSIGNAL_DAMAGE:
		{
				m_LastFightDamage = pCD->m_GameTick;
			//int32 Tick = (int32) (_pParams->GetParam(2) * SERVER_TICKSPERSECOND);
			// Ok, got a fightsignal, what we need to do here is find some sort of opponent and
			// damage him a little, a perfect job for a function in charmechanics
			CWObject_Character::Char_FightDamage(_pContext->m_pWPhysState,_pContext->m_pObj,Value);
			//m_FightDamageQueue.Create(pCD->m_GameTick + Tick,Value);
			
			break;
		}
	case AG_FIGHTSIGNAL_COUNTER:
		{
			// Ok, create counter move
			m_LastFightDamage = pCD->m_GameTick;
			CWObject_Character::Char_CreateCounterMove(_pContext->m_pWPhysState,_pContext->m_pObj,Value);
			break;
		}
	case AG_FIGHTSIGNAL_SNEAKGRAB:
		{
			// Ok, create sneak grab move
			m_LastFightDamage = pCD->m_GameTick;
			int32 TypeIndex = (int32) _pParams->GetParam(2);

			CWObject_Character::Char_CreateSneakMove(_pContext->m_pWPhysState,_pContext->m_pObj,Value,TypeIndex);
			break;
		}
	case AG_FIGHTSIGNAL_WEAPONMELEE:
		{
			// If the opponent has its back turned against me, and doesn't know about me,
			// kill it dead
			m_LastFightDamage = pCD->m_GameTick;

			int32 iBest = pCD->m_iCloseObject;
			if (iBest == 0)
			{
				iBest = -1;
				// Select best character in a 50 unit radius
				CWObject_Character::CharGetFightCharacter(_pContext->m_pObj, pCD, _pContext->m_pWPhysState, iBest, 50.0f);
			}

			CWObject_CoreData* pTarget = (iBest != -1 ? _pContext->m_pWPhysState->Object_GetCD(iBest) : NULL);
			CWO_Character_ClientData* pCDTarget = (pTarget ? CWObject_Character::GetClientData(pTarget) : NULL);
			const int TargetPhysType = CWObject_Character::Char_GetPhysType(pTarget);
			if (pTarget && pCDTarget && IsAlive(TargetPhysType) && (_pContext->m_pWPhysState->Phys_Message_SendToObject(
				CWObject_Message(OBJMSG_CHAR_GETDAMAGEFACTOR,-1,DAMAGETYPE_BLOW), iBest) != 0))
			{
				// Check so that target has got his back turned to me, and that he isn't aware of me
				if ((CVec3Dfp32::GetMatrixRow(pTarget->GetPositionMatrix(),0) * 
					CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),0)) < 0.5f)
					return;

				CAI_Core* pAIVictim = NULL;
				_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_AIQUERY_GETAI,0,0,0,0,0,0,&pAIVictim), iBest);
				// If target doesn't have an AI, he cannot know about me :)
				CAI_AgentInfo* pAgent = (pAIVictim ? pAIVictim->m_KB.GetAgentInfo(_pContext->m_pObj->m_iObject) : NULL);
				if (pAgent && (pAgent->GetCurAwareness() >= CAI_AgentInfo::DETECTED)) 
				{	// Victim knows about the player and cannot be killed
					return;
				}

				CWObject_Message EnterFightMsg = CWObject_Message(OBJMSG_CHAR_ENTERFIGHTMODE,
					_pContext->m_pObj->m_iObject,DAMAGETYPE_DARKNESS,0,ENTERFIGHTMODE_EXITBREAKNECK_KILL);
				_pContext->m_pWPhysState->Phys_Message_SendToObject(EnterFightMsg,iBest);

				// Kill the opponent, should have a nice hit velocity here also so the ragdoll really
				// flies off a bit
				_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_KILLPLAYER), iBest);
			}
			break;
		}
	case AG_FIGHTSIGNAL_WEAPONEXCHANGE:
		{
			// Find opponent
			m_LastFightDamage = pCD->m_GameTick;
			int32 iBest = pCD->m_iCloseObject;

			// Steal weapon from other character when countering
			CRPG_Object_Item* pMelee = ((CRPG_Object_Item*)_pContext->m_pWPhysState->Phys_Message_SendToObject(
				CWObject_Message(OBJMSG_CHAR_GETEQUIPPEDITEM, AG_ITEMSLOT_WEAPONS), iBest));
			if (!pMelee)
				return;
			
			// Okidoki then, force remove the item and give it to player character (and equip it)
			CWObject_Message Msg(OBJMSG_RPG_AVAILABLEPICKUP, pMelee->m_iItemType, aint((CRPG_Object_Item *)(CReferenceCount *)pMelee));
			Msg.m_pData = pMelee->m_Name.GetStr();
			_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg,_pContext->m_pObj->m_iObject);
			Msg = CWObject_Message(OBJMSG_CHAR_SELECTITEMBYTYPE, pMelee->m_iItemType);
			_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg,_pContext->m_pObj->m_iObject);
			//Char()->RemoveItemByType(_pContext->m_pObj->m_iObject, pMelee->m_iItemType);
			Msg = CWObject_Message(OBJMSG_CHAR_DESTROYITEM,0,1,0,0,0,0,pMelee->m_Name.GetStr());
			_pContext->m_pWPhysState->Phys_Message_SendToObject(Msg,iBest);

			break;
		}
	case AG_FIGHTSIGNAL_RESETCOMBOCOUNT:
		{
			m_ComboCount = 0;
			break;
		}
	case AG_FIGHTSIGNAL_INCREASECOMBOCOUNT:
		{
			m_LastFightDamage = pCD->m_GameTick;
			// Should probably check so that we actually have a target here
			m_ComboCount++;
			// From here check if we got counter
			/*if (m_ComboCount >= AG_FIGHT_COMBOSNEEDEDFORCOUNTER)
				CWObject_Character::Char_CreateCounterMove(_pContext->m_pWPhysState,_pContext->m_pObj,Value);*/

			break;
		}
	case AG_FIGHTSIGNAL_BREAKOFFGRAB:
		{
			m_LastFightDamage = pCD->m_GameTick;
			CWObject_CoreData* pObj = _pContext->m_pWPhysState->Object_GetCD(pCD->m_RelAnimPos.GetOther());
			CWO_Character_ClientData* pCDOther = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
			if (pCDOther)
			{
				// Send break grab interrupt to opponent
				pCDOther->m_AnimGraph.SetPendingFightInterrupt(AG_FIGHTMODE_INTERRUPT_BREAKGRAB);
			}
			break;
		}
	case AG_FIGHTSIGNAL_CLEARAIMELEE:
		{
			uint8 Val = 1 << ((int32) _pParams->GetParam(1) - 1);
			m_GPBitField &= ~Val;
			break;
		}
	case AG_FIGHTSIGNAL_CLEARAIMELEECOMBO:
		{
			uint8 Val = 1 << ((int32) _pParams->GetParam(1) + AG_AIMELEE_NUM_ATTACKS - 1);
			m_GPBitField &= ~Val;
			break;
		}
	case AG_FIGHTSIGNAL_PUSHDRAGDOLL:
		{
			m_LastFightDamage = pCD->m_GameTick;
			int32 iBest = pCD->m_iCloseObject;
			CWObject_CoreData* pObject = _pContext->m_pWPhysState->Object_GetCD(iBest);
			if (!pObject)
				break;
			CVec3Dfp32 Center, CenterOpponent;
			_pContext->m_pObj->GetAbsBoundBox()->GetCenter(Center);
			pObject->GetAbsBoundBox()->GetCenter(CenterOpponent);
			CCollisionInfo CInfo;
			CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
			CInfo.SetReturnValues(0);
			int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
			int32 ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;
			int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
			bool bHit = _pContext->m_pWPhysState->Phys_IntersectLine(Center, CenterOpponent, OwnFlags, ObjectFlags, MediumFlags, &CInfo, _pContext->m_pObj->m_iObject);
			if (bHit && (CInfo.m_iObject != iBest))
				break;

			CWObject_Message Push(OBJMSG_CHAR_PUSHRAGDOLL);
			Push.m_VecParam0 = _pContext->m_pObj->GetPosition();
			Push.m_VecParam1 = CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),0);
			Push.m_VecParam1.k[2] = 1.0f;
			Push.m_VecParam1.Normalize();
			Push.m_VecParam1 *= 13.0f;
			_pContext->m_pWPhysState->Phys_Message_SendToObject(Push,iBest);
			break;
		}
	case AG_FIGHTSIGNAL_RUMBLE:
		{
			m_LastFightDamage = pCD->m_GameTick;
			// Do some fightrumble
			CWObject_Character::Char_FightRumble(_pContext->m_pWPhysState,Value,_pContext->m_pObj->m_iObject);
			break;
		}
	case AG_FIGHTSIGNAL_GRENADEMELEE:
		{
			if (_pContext->m_pWPhysState->IsClient())
				break;

			int32 iBest = pCD->m_iCloseObject;
			if (iBest == 0)
			{
				iBest = -1;
				// Select best character in a 50 unit radius
				CWObject_Character::CharGetFightCharacter(_pContext->m_pObj, pCD, _pContext->m_pWPhysState, iBest, 50.0f);
			}
			CRPG_Object_Melee* pMelee = NULL;
			{
				CRPG_Object_Item* pItem = (CRPG_Object_Item*)_pContext->m_pWPhysState->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_CHAR_GETITEM, 0), _pContext->m_pObj->m_iObject);
				if (!pItem)
					break;
				switch (pItem->m_AnimProperty)
				{
				case AG_EQUIPPEDITEMCLASS_BAREHANDS:
				case AG_EQUIPPEDITEMCLASS_KNUCKLEDUSTER:
				case AG_EQUIPPEDITEMCLASS_SHANK:
				case AG_EQUIPPEDITEMCLASS_CLUB:
					{
						pMelee = (CRPG_Object_Melee*)pItem;
						break;
					}
				default:
					break;
				}
			}

			CWObject_CoreData* pTarget = (iBest != -1 ? _pContext->m_pWPhysState->Object_GetCD(iBest) : NULL);
			CWO_Character_ClientData* pCDTarget = (pTarget ? CWObject_Character::GetClientData(pTarget) : NULL);
			const int TargetPhysType = CWObject_Character::Char_GetPhysType(pTarget);
			_pContext->m_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_AIEFFECT_MELEEATTACK,iBest),_pContext->m_pObj->m_iObject);
			if (pTarget && pCDTarget && IsAlive(TargetPhysType) && !pCDTarget->m_AnimGraph.GetIsSleeping())
			{
				CVec3Dfp32 OpponentPosition = pTarget->GetPosition();
				CVec3Dfp32 DirToOpponent = (OpponentPosition - _pContext->m_pObj->GetPosition());
				CVec3Dfp32 CharDir = CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),0);
				CharDir.k[2] = 0.0f;
				CharDir.Normalize();
				// Beh, need to normalize anyways
				fp32 Length = DirToOpponent.Length();
				DirToOpponent = DirToOpponent / Length;
				if ((Length < 55.0f) && 
					(CharDir * DirToOpponent) > 0.5f)
				{
					CVec3Dfp32 Force = 0.0f;
					CVec3Dfp32 Left;
					CVec3Dfp32 Up(0.0f,0.0f,0.5f);
					CharDir.CrossProd(CVec3Dfp32(0.0f,0.0f,1.0f),Left);
					int32 HitType = AG_FIGHTMODE_SOUNDTYPE_LEFT;
					int32 AttackType = MELEE_ATTACK_LEFT;
					Force = CharDir + Left + Up;
					Force *= 10.0f;
					bool bBlocking = (pCDTarget->m_AnimGraph.GetStateFlagsLo() & CHAR_STATEFLAG_BLOCKACTIVE) != 0;
					if (bBlocking)
						break;
					if (pCD->m_iPlayer != -1)
						CWObject_Character::Char_FightRumble(_pContext->m_pWPhysState, MELEE_ATTACK_LEFT, _pContext->m_pObj->m_iObject);
					else if (pCDTarget->m_iPlayer != -1)
						CWObject_Character::Char_FightRumble(_pContext->m_pWPhysState, AG_FIGHTDAMAGE_TYPE_HURTLEFT, pTarget->m_iObject);
					
						/*CRPG_Object_Melee* pMeleeTarget = (CRPG_Object_Melee*)_pWPhys->Phys_Message_SendToObject(
						CWObject_Message(OBJMSG_CHAR_GETEQUIPPEDITEM, AG_ITEMSLOT_WEAPONS), pTarget->m_iObject);*/
					int iSound = pMelee->GetSoundIndex(AG_FIGHTMODE_SOUNDTYPE_LEFT);
					int iSoundSurf = pMelee->GetSoundSurface();

					if ((iSound != -1) && (iSoundSurf != -1))
					{
						// Hmm what position should we have, for now get character position and offset
						// a bit in direction and height
						CMat43fp32 PosMat = _pContext->m_pObj->GetPositionMatrix();

						CVec3Dfp32 SoundPosition = CVec3Dfp32::GetMatrixRow(PosMat,3) + 
							CVec3Dfp32(0,0,(pCD->m_Phys_Height / 1.5f)) + CVec3Dfp32::GetMatrixRow(PosMat,0) * 15.0f;
						CWorld_Server* pWServer = TDynamicCast<CWorld_Server>(_pContext->m_pWPhysState);
						if (pWServer)
						{
							pWServer->Sound_At(SoundPosition, iSound, WCLIENT_ATTENUATION_3D, 0);
							pWServer->Sound_At(SoundPosition, iSoundSurf, WCLIENT_ATTENUATION_3D, 50);
						}
					}

					pCDTarget->m_AnimGraph.SetPendingFightInterrupt((uint8)AG_FIGHTDAMAGE_TYPE_HURTLEFT);
					CVec3Dfp32 DamagePos = pTarget->GetPosition();
					DamagePos.k[2] += 50.0f;
					CWO_DamageMsg Msg(Value, DAMAGETYPE_BLOW, &DamagePos, &Force);
					Msg.Send(iBest, _pContext->m_pObj->m_iObject,_pContext->m_pWPhysState);
				}
			}
			break;
		}
	case AG_FIGHTSIGNAL_BLOOD:
		{
			m_LastFightDamage = pCD->m_GameTick;
			fp32 ScaleRight = 1.0f - _pParams->GetParam(2);
			fp32 ScaleUp = _pParams->GetParam(3);
			if (_pContext->m_pWPhysState->IsServer())
			{
				CVec3Dfp32 Direction = -CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),0);
				Direction -= CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),1) * ScaleRight;
				Direction += CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),2) * ScaleUp;
				((CWObject_Character*)_pContext->m_pObj)->Char_DoBloodAtAttach(PLAYER_ROTTRACK_RHANDATTACH,-1,Direction);
			}
			break;
		}
	case AG_FIGHTSIGNAL_BLOOD2:
		{
			m_LastFightDamage = pCD->m_GameTick;
			fp32 ScaleUp = _pParams->GetParam(2);
			int32 Rottrack = (int32)_pParams->GetParam(3);
			Rottrack = (Rottrack ? Rottrack : PLAYER_ROTTRACK_HEAD);
			if (_pContext->m_pWPhysState->IsServer())
			{
				CVec3Dfp32 Direction = -CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),0);
				Direction += CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),2) * ScaleUp;
				((CWObject_Character*)_pContext->m_pObj)->Char_DoBloodAtAttach(Rottrack,-1,Direction);
			}
			break;
		}
	case AG_FIGHTSIGNAL_BLOOD3:
		{
			m_LastFightDamage = pCD->m_GameTick;
			fp32 ScaleRight = _pParams->GetParam(2);
			fp32 ScaleUp = _pParams->GetParam(3);
			if (_pContext->m_pWPhysState->IsServer())
			{
				CVec3Dfp32 Direction = -CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),0);
				Direction += CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),1) * ScaleRight;
				Direction += CVec3Dfp32::GetMatrixRow(_pContext->m_pObj->GetPositionMatrix(),2) * ScaleUp;
				((CWObject_Character*)_pContext->m_pObj)->Char_DoBloodAtAttach(PLAYER_ROTTRACK_RHANDATTACH,-1,Direction,1,"hitEffect_blood01");
			}
			break;
		}
	default:
		break;
	}
}

void CWO_Clientdata_Character_AnimGraph::Effect_KillTokens(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams)
{
	int32 NumParams = _pParams->GetNumParams();
	if (!NumParams)
		return;
	CStr MoveToken("TerminateMe.Action_Die");
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pContext->m_pObj);
	if (pCD)
	{	
		for (int32 i = 0; i < NumParams; i++)
		{
			int32 iToken = (int32)_pParams->GetParam(i);
			CWObject_Character::MoveToken(_pContext->m_pObj,_pContext->m_pWPhysState,pCD,iToken,MoveToken);
		}
	}
}

//--------------------------------------------------------------------------------

CWO_Clientdata_Character_AnimGraph::PFN_ANIMGRAPH_CONDITION CWO_Clientdata_Character_AnimGraph::ms_lpfnConditions_Server[MAX_ANIMGRAPH_CONDITIONS] =
{
/* 00 */ &CWO_Clientdata_Character_AnimGraph::Condition_StateTime,
/* 01 */ NULL,//&CWO_Clientdata_Character_AnimGraph::Condition_AnimExitPoint,
/* 02 */ &CWO_Clientdata_Character_AnimGraph::Condition_AnimLoopCount,
/* 03 */ NULL,
/* 04 */ &CWO_Clientdata_Character_AnimGraph::Condition_AnimLoopCountOffset,
/* 05 */ NULL,
/* 06 */ NULL,
/* 07 */ NULL,
/* 08 */ NULL,
/* 09 */ &CWO_Clientdata_Character_AnimGraph::Condition_StateTimeOffset,

/* 10 */ NULL,
/* 11 */ NULL,
/* 12 */ NULL,
/* 13 */ NULL,
/* 14 */ NULL,
/* 15 */ NULL,
/* 16 */ NULL,
/* 17 */ &CWO_Clientdata_Character_AnimGraph::Condition_LoopedAnimCount,
/* 18 */ NULL,
/* 19 */ NULL,

/* 20 */ NULL,
/* 21 */ NULL,
/* 22 */ NULL,
/* 23 */ NULL,
/* 24 */ NULL,
/* 25 */ NULL,
/* 26 */ NULL,
/* 27 */ NULL,
/* 28 */ NULL,
/* 29 */ NULL,

/* 30 */ NULL,
/* 31 */ NULL,
/* 32 */ NULL,
/* 33 */ NULL,
/* 34 */ NULL,
/* 35 */ NULL,
/* 36 */ NULL,
/* 37 */ NULL,
/* 38 */ NULL,
/* 39 */ NULL,

/* 40 */ NULL,
/* 41 */ NULL,
/* 42 */ NULL,
/* 43 */ NULL,
/* 44 */ NULL,
/* 45 */ NULL,
/* 46 */ NULL,
/* 47 */ NULL,
/* 48 */ NULL,
/* 49 */ NULL,

/* 50 */ NULL,
/* 51 */ NULL,
/* 52 */ NULL,
/* 53 */ NULL,
/* 54 */ NULL,
/* 55 */ NULL,
/* 56 */ NULL,
/* 57 */ NULL,
/* 58 */ NULL,
/* 59 */ NULL,

/* 60 */ NULL,
/* 61 */ NULL,
/* 62 */ NULL,
/* 63 */ NULL,

/* 64 */ NULL,
/* 65 */ NULL,
/* 66 */ NULL,
/* 67 */ NULL,
/* 68 */ NULL,
/* 69 */ NULL,

/* 70 */ NULL,
/* 71 */ NULL,
/* 72 */ NULL,
/* 73 */ NULL,
/* 74 */ NULL,
/* 75 */ NULL,
/* 76 */ NULL,
/* 77 */ NULL,
/* 78 */ NULL,
/* 79 */ NULL,

/* 80 */ NULL,
/* 81 */ NULL,
/* 82 */ NULL,
/* 83 */ NULL,
/* 84 */ NULL,
/* 85 */ NULL,
/* 86 */ NULL,
/* 87 */ NULL,
/* 88 */ NULL,
/* 89 */ NULL,

/* 90 */ NULL,
/* 91 */ NULL,
/* 92 */ NULL,
/* 93 */ NULL,
/* 94 */ NULL,
/* 95 */ NULL,
/* 96 */ NULL,
/* 97 */ NULL,
/* 98 */ NULL,
/* 99 */ NULL,

/* 100 */ NULL,
/* 101 */ NULL,
/* 102 */ NULL,
/* 103 */ NULL,
/* 104 */ NULL,
/* 105 */ NULL,
/* 106 */ NULL,
/* 107 */ NULL,
/* 108 */ NULL,
/* 109 */ NULL,
};

//--------------------------------------------------------------------------------

CWO_Clientdata_Character_AnimGraph::PFN_ANIMGRAPH_PROPERTY CWO_Clientdata_Character_AnimGraph::ms_lpfnProperties_Server[MAX_ANIMGRAPH_PROPERTIES] =
{
/* 00 */ &CWO_Clientdata_Character_AnimGraph::Property_StateTime,
/* 01 */ &CWO_Clientdata_Character_AnimGraph::Property_Dialogue1,
/* 02 */ &CWO_Clientdata_Character_AnimGraph::Property_Dialogue2,
/* 03 */ &CWO_Clientdata_Character_AnimGraph::Property_Dialogue3,
/* 04 */ &CWO_Clientdata_Character_AnimGraph::Property_Dialogue4,
/* 05 */ &CWO_Clientdata_Character_AnimGraph::Property_Rand1,
/* 06 */ &CWO_Clientdata_Character_AnimGraph::Property_Rand255,
/* 07 */ &CWO_Clientdata_Character_AnimGraph::Property_NeedReload,
/* 08 */ &CWO_Clientdata_Character_AnimGraph::Property_HandledResponse,
/* 09 */ &CWO_Clientdata_Character_AnimGraph::Property_StateTimeOffset,

/* 10 */ &CWO_Clientdata_Character_AnimGraph::Property_LeanActive,
/* 11 */ &CWO_Clientdata_Character_AnimGraph::Property_IsSleeping,
/* 12 */ &CWO_Clientdata_Character_AnimGraph::Property_CanActivateItem,
/* 13 */ &CWO_Clientdata_Character_AnimGraph::Property_OnHit_BodyPart,
/* 14 */ &CWO_Clientdata_Character_AnimGraph::Property_OnHit_Direction,
/* 15 */ &CWO_Clientdata_Character_AnimGraph::Property_OnHit_Force,
/* 16 */ &CWO_Clientdata_Character_AnimGraph::Property_OnHit_DamageType,
/* 17 */ &CWO_Clientdata_Character_AnimGraph::Property_LoopedAnimTime,
/* 18 */ &CWO_Clientdata_Character_AnimGraph::Property_EquippedItemClass,
/* 19 */ &CWO_Clientdata_Character_AnimGraph::Property_MoveRadiusControl,

/* 20 */ &CWO_Clientdata_Character_AnimGraph::Property_MoveAngleControl,
/* 21 */ &CWO_Clientdata_Character_AnimGraph::Property_MoveAngleSControl,
/* 22 */ &CWO_Clientdata_Character_AnimGraph::Property_MoveAngleUnitControl,
/* 23 */ &CWO_Clientdata_Character_AnimGraph::Property_MoveAngleUnitSControl,
/* 24 */ &CWO_Clientdata_Character_AnimGraph::Property_MoveVeloHoriz,
/* 25 */ &CWO_Clientdata_Character_AnimGraph::Property_MoveVeloVert,
/* 26 */ &CWO_Clientdata_Character_AnimGraph::Property_MoveAngle,
/* 27 */ &CWO_Clientdata_Character_AnimGraph::Property_MoveAngleS,
/* 28 */ &CWO_Clientdata_Character_AnimGraph::Property_MoveAngleUnit,
/* 29 */ &CWO_Clientdata_Character_AnimGraph::Property_MoveAngleUnitS,

/* 30 */ &CWO_Clientdata_Character_AnimGraph::Property_ButtonTest,
/* 31 */ &CWO_Clientdata_Character_AnimGraph::Property_MoveHControl,
/* 32 */ &CWO_Clientdata_Character_AnimGraph::Property_MoveVControl,
/* 33 */ &CWO_Clientdata_Character_AnimGraph::Property_Vigilance,
/* 34 */ &CWO_Clientdata_Character_AnimGraph::Property_LedgeType,
/* 35 */ &CWO_Clientdata_Character_AnimGraph::Property_OnHit_Damage,
/* 36 */ &CWO_Clientdata_Character_AnimGraph::Property_IsServer,
/* 37 */ &CWO_Clientdata_Character_AnimGraph::Property_PendingFightInterrupt,
/* 38 */ &CWO_Clientdata_Character_AnimGraph::Property_CanBlock,
/* 39 */ &CWO_Clientdata_Character_AnimGraph::Property_AIMoveMode,

/* 40 */ &CWO_Clientdata_Character_AnimGraph::Property_CanEndACS,
/* 41 */ &CWO_Clientdata_Character_AnimGraph::Property_CanClimbUpLedge,
/* 42 */ &CWO_Clientdata_Character_AnimGraph::Property_NewFightTestMode,
/* 43 */ &CWO_Clientdata_Character_AnimGraph::Property_HasFightTarget,
/* 44 */ &CWO_Clientdata_Character_AnimGraph::Property_JoyPad,
/* 45 */ &CWO_Clientdata_Character_AnimGraph::Property_LeftTrigger,
/* 46 */ &CWO_Clientdata_Character_AnimGraph::Property_RightTrigger,
/* 47 */ &CWO_Clientdata_Character_AnimGraph::Property_IsCrouching,
/* 48 */ &CWO_Clientdata_Character_AnimGraph::Property_IsFighting,
/* 49 */ &CWO_Clientdata_Character_AnimGraph::Property_GetControlmode,

/* 50 */ &CWO_Clientdata_Character_AnimGraph::Property_HasTarget,
/* 51 */ &CWO_Clientdata_Character_AnimGraph::Property_DistanceToTarget,
/* 52 */ &CWO_Clientdata_Character_AnimGraph::Property_TurnAngleToTarget,
/* 53 */ &CWO_Clientdata_Character_AnimGraph::Property_FindFixedMoveAngle8,
/* 54 */ &CWO_Clientdata_Character_AnimGraph::Property_FindFixedMoveAngle4,
/* 55 */ &CWO_Clientdata_Character_AnimGraph::Property_Health,
/* 56 */ &CWO_Clientdata_Character_AnimGraph::Property_IsDead,
/* 57 */ &CWO_Clientdata_Character_AnimGraph::Property_ActionCutsceneType,
/* 58 */ &CWO_Clientdata_Character_AnimGraph::Property_LadderEndPoint,
/* 59 */ &CWO_Clientdata_Character_AnimGraph::Property_FindFixedMoveAngleControl8,

/* 60 */ &CWO_Clientdata_Character_AnimGraph::Property_FindFixedMoveAngleControl4,
/* 61 */ &CWO_Clientdata_Character_AnimGraph::Property_EventTrigger,
/* 62 */ &CWO_Clientdata_Character_AnimGraph::Property_IsInAir,
/* 63 */ &CWO_Clientdata_Character_AnimGraph::Property_FindFixedMoveAngleDiff5,
/* 64 */ &CWO_Clientdata_Character_AnimGraph::Property_WantedBehavior,
/* 65 */ &CWO_Clientdata_Character_AnimGraph::Property_AngleDiff,
/* 66 */ &CWO_Clientdata_Character_AnimGraph::Property_AngleDiffS,
/* 67 */ &CWO_Clientdata_Character_AnimGraph::Property_FindFixedMoveAngleDiff8,

/* 68 */ &CWO_Clientdata_Character_AnimGraph::Property_FixedWalkAngle4,
/* 69 */ &CWO_Clientdata_Character_AnimGraph::Property_LeverState,

/* 70 */ &CWO_Clientdata_Character_AnimGraph::Property_Dialogue0,
/* 71 */ &CWO_Clientdata_Character_AnimGraph::Property_Dialogue1,
/* 72 */ &CWO_Clientdata_Character_AnimGraph::Property_Dialogue2,
/* 73 */ &CWO_Clientdata_Character_AnimGraph::Property_Dialogue3,
/* 74 */ &CWO_Clientdata_Character_AnimGraph::Property_Dialogue4,
/* 75 */ &CWO_Clientdata_Character_AnimGraph::Property_Dialogue5,
/* 76 */ NULL,
/* 77 */ NULL,
/* 78 */ NULL,
/* 79 */ NULL,

/* 80 */ &CWO_Clientdata_Character_AnimGraph::Property_ValveState,
/* 81 */ &CWO_Clientdata_Character_AnimGraph::Property_CanPlayEquip,
/* 82 */ &CWO_Clientdata_Character_AnimGraph::Property_MoveAngleHeavyGuard,
/* 83 */ &CWO_Clientdata_Character_AnimGraph::Property_WallCollision,
/* 84 */ &CWO_Clientdata_Character_AnimGraph::Property_HangRailCheckHeight,
/* 85 */ &CWO_Clientdata_Character_AnimGraph::Property_LadderStepoffType,
/* 86 */ &CWO_Clientdata_Character_AnimGraph::Property_EffectPlaying,
/* 87 */ &CWO_Clientdata_Character_AnimGraph::Property_LastSentFightInterrupt,
/* 88 */ &CWO_Clientdata_Character_AnimGraph::Property_CanCounter,
/* 89 */ &CWO_Clientdata_Character_AnimGraph::Property_RelAnimQueued,

/* 90 */ &CWO_Clientdata_Character_AnimGraph::Property_WantedBehaviorType,
/* 91 */ &CWO_Clientdata_Character_AnimGraph::Property_CanPlayFightFakeIdle,
/* 92 */ &CWO_Clientdata_Character_AnimGraph::Property_IdleTurnTimer,
/* 93 */ &CWO_Clientdata_Character_AnimGraph::Property_GPBitFieldBit,
/* 94 */ &CWO_Clientdata_Character_AnimGraph::Property_FixedFightAngle4,
/* 95 */ &CWO_Clientdata_Character_AnimGraph::Property_BreakOffGrab,
/* 96 */ &CWO_Clientdata_Character_AnimGraph::Property_CanSwitchWeapon,
/* 97 */ &CWO_Clientdata_Character_AnimGraph::Property_LastMoveSpeedSqr,
/* 98 */ &CWO_Clientdata_Character_AnimGraph::Property_MeleeeAttack,
/* 99 */ &CWO_Clientdata_Character_AnimGraph::Property_RelativeHealth,

/* 100 */ &CWO_Clientdata_Character_AnimGraph::Property_UseSmallHurt,
/* 101 */ &CWO_Clientdata_Character_AnimGraph::Property_HangrailCanGoForward,
/* 102 */ &CWO_Clientdata_Character_AnimGraph::Property_HasTranqTarget,
/* 103 */ &CWO_Clientdata_Character_AnimGraph::Property_CanGotoFight,
/* 104 */ &CWO_Clientdata_Character_AnimGraph::Property_CanFightStandUp,
/* 105 */ NULL,
/* 106 */ NULL,
/* 107 */ NULL,
/* 108 */ NULL,
/* 109 */ NULL,
};

//--------------------------------------------------------------------------------

CWO_Clientdata_Character_AnimGraph::PFN_ANIMGRAPH_OPERATOR CWO_Clientdata_Character_AnimGraph::ms_lpfnOperators_Server[MAX_ANIMGRAPH_OPERATORS] =
{
/* 00 */ &CWO_Clientdata_Character_AnimGraph::OperatorEQ,
/* 01 */ &CWO_Clientdata_Character_AnimGraph::OperatorGT,
/* 02 */ &CWO_Clientdata_Character_AnimGraph::OperatorLT,
/* 03 */ &CWO_Clientdata_Character_AnimGraph::OperatorGE,
/* 04 */ &CWO_Clientdata_Character_AnimGraph::OperatorLE,
/* 05 */ &CWO_Clientdata_Character_AnimGraph::OperatorNE,
/* 06 */ &CWO_Clientdata_Character_AnimGraph::OperatorMOD,
/* 07 */ NULL,
};

//--------------------------------------------------------------------------------

CWO_Clientdata_Character_AnimGraph::PFN_ANIMGRAPH_EFFECT CWO_Clientdata_Character_AnimGraph::ms_lpfnEffects_Server[MAX_ANIMGRAPH_EFFECTS] =
{
/* 00 */ &CWO_Clientdata_Character_AnimGraph::Effect_ActionCutsceneSwitch,
/* 01 */ &CWO_Clientdata_Character_AnimGraph::Effect_DrainStamina,
/* 02 */ NULL,
/* 03 */ NULL,
/* 04 */ &CWO_Clientdata_Character_AnimGraph::Effect_SendImpulse,
/* 05 */ &CWO_Clientdata_Character_AnimGraph::Effect_SelectItem,
/* 06 */ &CWO_Clientdata_Character_AnimGraph::Effect_ActivateItem,
/* 07 */ &CWO_Clientdata_Character_AnimGraph::Effect_DeactivateItem,
/* 08 */ &CWO_Clientdata_Character_AnimGraph::Effect_SetControlMode,
/* 09 */ &CWO_Clientdata_Character_AnimGraph::Effect_MessageSender,

/* 10 */ &CWO_Clientdata_Character_AnimGraph::Effect_LedgeMove,
/* 11 */ &CWO_Clientdata_Character_AnimGraph::Effect_LadderMove,
/* 12 */ &CWO_Clientdata_Character_AnimGraph::Effect_RegisterUsedInput,
/* 13 */ NULL,
/* 14 */ &CWO_Clientdata_Character_AnimGraph::Effect_PickupStuff,
/* 15 */ &CWO_Clientdata_Character_AnimGraph::Effect_ResetActionPressCount,
/* 16 */ NULL,
/* 17 */ &CWO_Clientdata_Character_AnimGraph::Effect_ClearFightInterrupt,
/* 18 */ &CWO_Clientdata_Character_AnimGraph::Effect_ClearHit,
/* 19 */ NULL,

/* 20 */ &CWO_Clientdata_Character_AnimGraph::Effect_ResetControlmodeParams,
/* 21 */ NULL,
/* 22 */ NULL,
/* 23 */ &CWO_Clientdata_Character_AnimGraph::Effect_PauseTokens,
/* 24 */ &CWO_Clientdata_Character_AnimGraph::Effect_RegisterHandledResponse,
/* 25 */ &CWO_Clientdata_Character_AnimGraph::Effect_AIBehaviorSignal,
/* 26 */ &CWO_Clientdata_Character_AnimGraph::Effect_AIRegisterBehavior,
/* 27 */ &CWO_Clientdata_Character_AnimGraph::Effect_UnRegisterUsedInput,
/* 28 */ &CWO_Clientdata_Character_AnimGraph::Effect_UnregisterHandledResponse,
/* 29 */ &CWO_Clientdata_Character_AnimGraph::Effect_RelAnimMoveSignal,
/* 30 */ &CWO_Clientdata_Character_AnimGraph::Effect_FightSignal,
/* 31 */ &CWO_Clientdata_Character_AnimGraph::Effect_KillTokens,
};

//--------------------------------------------------------------------------------
