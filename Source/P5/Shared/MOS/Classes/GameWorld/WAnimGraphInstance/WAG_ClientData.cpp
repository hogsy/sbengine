#include "PCH.h"

#include "WAG_ClientData.h"

void CWO_ClientData_AnimGraphInterface::AG_RefreshGlobalProperties(const CWAGI_Context* _pContext)
{
	for(int i = 0; i < NUM_DIALOGUE_PROPERTIES; i++)
		if(m_Dialogue_Once[i] != -1)
		{
			m_Dialogue_Cur[i] = m_Dialogue_Once[i];
			m_Dialogue_Once[i] = 0;
		}
}

void CWO_ClientData_AnimGraphInterface::AG_RefreshStateInstanceProperties(const CWAGI_Context* _pContext, const CWAGI_StateInstance* _pStateInstance)
{
	m_EnterStateTime = _pStateInstance->GetEnterTime();
	m_iEnterAction = _pStateInstance->GetEnterActionIndex();

	m_iRandseed = _pStateInstance->GetAGI()->GetRandseed();

	m_AnimLoopDuration = _pStateInstance->GetAnimLoopDuration_Cached();

	m_EnterAnimTimeOffset = _pStateInstance->GetEnterAnimTimeOffset();
}

void CWO_ClientData_AnimGraphInterface::Clear()
{
	m_iRandseed = 0;
	m_EnterStateTime.Reset();
	m_iEnterAction = -1;
	m_AnimLoopDuration = 0;
	m_EnterAnimTimeOffset = 0.0f;
	m_ImportantAGEvent = 0;
	m_WallCollision = 0;
	m_HandledResponse = 0;
	for(int i = 0; i < NUM_DIALOGUE_PROPERTIES; i++)
	{
		m_Dialogue_Cur[i] = 0;
		m_Dialogue_Once[i] = -1;
	}
}

void CWO_ClientData_AnimGraphInterface::Copy(const CWO_ClientData_AnimGraphInterface& _CD)
{
	m_iRandseed = _CD.m_iRandseed;
	m_EnterStateTime = _CD.m_EnterStateTime;
	m_iEnterAction = _CD.m_iEnterAction;
	m_AnimLoopDuration = _CD.m_AnimLoopDuration;
	m_EnterAnimTimeOffset = _CD.m_EnterAnimTimeOffset;
	m_WallCollision = _CD.m_WallCollision;
	m_ImportantAGEvent = _CD.m_ImportantAGEvent;
	m_HandledResponse = _CD.m_HandledResponse;
	for(int i = 0; i < NUM_DIALOGUE_PROPERTIES; i++)
	{
		m_Dialogue_Cur[i] = _CD.m_Dialogue_Cur[i];
		m_Dialogue_Once[i] = _CD.m_Dialogue_Once[i];
	}
}

//--------------------------------------------------------------------------------

bool CWO_ClientData_AnimGraphInterface::Condition_StateTime(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams, int _iOperator, const CAGVal &_Constant, fp32& _TimeFraction)
{
	bool bOperatorMod = (AG_EvaluateOperator(_pContext, _iOperator, _Constant.Scale(0.0f), _Constant) &&
		!AG_EvaluateOperator(_pContext, _iOperator, _Constant.Scale(0.5f), _Constant) &&
		AG_EvaluateOperator(_pContext, _iOperator, _Constant, _Constant) &&
		!AG_EvaluateOperator(_pContext, _iOperator, _Constant.Scale(1.5f), _Constant) &&
		AG_EvaluateOperator(_pContext, _iOperator, _Constant.Scale(2.0f), _Constant));

	if (bOperatorMod)
	{
		CMTime Constant = _Constant.GetTime();
		CMTime StateTime = _pContext->m_GameTime - m_EnterStateTime;
		while (StateTime.Compare(Constant) > 0)
			StateTime -= Constant;

		fp32 TimeOffset = (StateTime - Constant).GetTime();
		return ConditionHelper(_pContext, TimeOffset, _TimeFraction, true, false, false);
	}
	else
	{
		bool bOperatorEqual = AG_EvaluateOperator(_pContext, _iOperator, CAGVal::From(0), CAGVal::From(0));
		bool bOperatorGreater = AG_EvaluateOperator(_pContext, _iOperator, CAGVal::From(1), CAGVal::From(0));
		bool bOperatorLess = AG_EvaluateOperator(_pContext, _iOperator, CAGVal::From(0), CAGVal::From(1));

		CMTime StateTime = _pContext->m_GameTime - m_EnterStateTime;
		fp32 TimeOffset = (_Constant.GetTime() - StateTime).GetTime();
		return ConditionHelper(_pContext, TimeOffset, _TimeFraction, bOperatorEqual, bOperatorGreater, bOperatorLess);
	}
}

//--------------------------------------------------------------------------------

bool CWO_ClientData_AnimGraphInterface::Condition_AnimLoopCount(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams, int _iOperator, const CAGVal &_Constant, fp32& _TimeFraction)
{
	bool bOperatorEqual = AG_EvaluateOperator(_pContext, _iOperator, CAGVal::From(0), CAGVal::From(0));
	bool bOperatorGreater = AG_EvaluateOperator(_pContext, _iOperator, CAGVal::From(1), CAGVal::From(0));
	bool bOperatorLess = AG_EvaluateOperator(_pContext, _iOperator, CAGVal::From(0), CAGVal::From(1));

	CMTime StateTime = _pContext->m_GameTime - m_EnterStateTime;
	CMTime TimeOffset = (_Constant.GetTime().Scale(m_AnimLoopDuration)) - StateTime;

	return ConditionHelper(_pContext, TimeOffset.GetTime(), _TimeFraction, bOperatorEqual, bOperatorGreater, bOperatorLess);
}

bool CWO_ClientData_AnimGraphInterface::Condition_LoopedAnimCount(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams, int _iOperator, const CAGVal &_Constant, fp32& _TimeFraction)
{
	bool bOperatorEqual = AG_EvaluateOperator(_pContext, _iOperator, CAGVal::From(0), CAGVal::From(0));
	bool bOperatorGreater = AG_EvaluateOperator(_pContext, _iOperator, CAGVal::From(1), CAGVal::From(0));
	bool bOperatorLess = AG_EvaluateOperator(_pContext, _iOperator, CAGVal::From(0), CAGVal::From(1));

	fp32 StateTime = (_pContext->m_GameTime - m_EnterStateTime).GetTime();
	fp32 CurrentAnimCount = M_Floor(StateTime/m_AnimLoopDuration);
	fp32 TimeOffset = ((_Constant.GetTime().GetTime() + CurrentAnimCount) * m_AnimLoopDuration) - StateTime;

	return ConditionHelper(_pContext, TimeOffset, _TimeFraction, bOperatorEqual, bOperatorGreater, bOperatorLess);
}

//--------------------------------------------------------------------------------

bool CWO_ClientData_AnimGraphInterface::Condition_AnimLoopCountOffset(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams, int _iOperator, const CAGVal &_Constant, fp32& _TimeFraction)
{
	bool bOperatorEqual = AG_EvaluateOperator(_pContext, _iOperator, CAGVal::From(0), CAGVal::From(0));
	bool bOperatorGreater = AG_EvaluateOperator(_pContext, _iOperator, CAGVal::From(1), CAGVal::From(0));
	bool bOperatorLess = AG_EvaluateOperator(_pContext, _iOperator, CAGVal::From(0), CAGVal::From(1));

	fp32 StateTime = (_pContext->m_GameTime - m_EnterStateTime).GetTime() + m_EnterAnimTimeOffset;
	fp32 TimeOffset = (_Constant.GetTime().GetTime() * m_AnimLoopDuration) - StateTime;
	return ConditionHelper(_pContext, TimeOffset, _TimeFraction, bOperatorEqual, bOperatorGreater, bOperatorLess);
}
//--------------------------------------------------------------------------------

bool CWO_ClientData_AnimGraphInterface::Condition_StateTimeOffset(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams, int _iOperator, const CAGVal &_Constant, fp32& _TimeFraction)
{
	bool bOperatorMod = (AG_EvaluateOperator(_pContext, _iOperator, _Constant.Scale(0.0f), _Constant) &&
		!AG_EvaluateOperator(_pContext, _iOperator, _Constant.Scale(0.5f), _Constant) &&
		AG_EvaluateOperator(_pContext, _iOperator, _Constant, _Constant) &&
		!AG_EvaluateOperator(_pContext, _iOperator, _Constant.Scale(1.5f), _Constant) &&
		AG_EvaluateOperator(_pContext, _iOperator, _Constant.Scale(2.0f), _Constant));

	if (bOperatorMod)
	{
		CMTime Constant = _Constant.GetTime();
		CMTime StateTime = _pContext->m_GameTime - m_EnterStateTime + CMTime::CreateFromSeconds(m_EnterAnimTimeOffset);
		while (StateTime.Compare(Constant) > 0)
			StateTime -= Constant;

		fp32 TimeOffset = (StateTime - Constant).GetTime();
		return ConditionHelper(_pContext, TimeOffset, _TimeFraction, true, false, false);
	}
	else
	{
		bool bOperatorEqual = AG_EvaluateOperator(_pContext, _iOperator, CAGVal::From(0), CAGVal::From(0));
		bool bOperatorGreater = AG_EvaluateOperator(_pContext, _iOperator, CAGVal::From(1), CAGVal::From(0));
		bool bOperatorLess = AG_EvaluateOperator(_pContext, _iOperator, CAGVal::From(0), CAGVal::From(1));

		CMTime StateTime = _pContext->m_GameTime - m_EnterStateTime + CMTime::CreateFromSeconds(m_EnterAnimTimeOffset);
		fp32 TimeOffset = (_Constant.GetTime() - StateTime).GetTime();
		return ConditionHelper(_pContext, TimeOffset, _TimeFraction, bOperatorEqual, bOperatorGreater, bOperatorLess);
	}
}

//--------------------------------------------------------------------------------

/*bool CWO_ClientData_AnimGraphInterface::Condition_AnimExitPoint(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams, int _iOperator, fp32 _Constant, fp32& _TimeFraction)
{
	for (int iKey = 0; iKey < m_lAnimExitPoints.Len(); iKey++)
	{
		if (m_lAnimExitPoints[iKey].m_Value == _Constant)
		{
			if (_pContext->m_TimeSpan != 0)
				_TimeFraction = (m_lAnimExitPoints[iKey].m_GameTime - _pContext->m_GameTime) / _pContext->m_TimeSpan;
			else
				_TimeFraction = 0;
			return true;
		}
	}
	return false;
}*/

//--------------------------------------------------------------------------------

CAGVal CWO_ClientData_AnimGraphInterface::Property_LoopedAnimTime(const CWAGI_Context* _pContext,
																const CXRAG_ICallbackParams* _pParams)
{	
	CMTime StateTime = (_pContext->m_GameTime - m_EnterStateTime).Modulus(m_AnimLoopDuration);
	return CAGVal::From(StateTime);
}

//--------------------------------------------------------------------------------

CAGVal CWO_ClientData_AnimGraphInterface::Property_StateTimeOffset(const CWAGI_Context* _pContext, 
																 const CXRAG_ICallbackParams* _pParams)
{
	// Find statetime with offset
	CMTime StateTime = (_pContext->m_GameTime - m_EnterStateTime) + CMTime::CreateFromSeconds(m_EnterAnimTimeOffset);
	return CAGVal::From(StateTime);
}

//--------------------------------------------------------------------------------

CAGVal CWO_ClientData_AnimGraphInterface::Property_Rand1(const CWAGI_Context* _pContext,
													   const CXRAG_ICallbackParams* _pParams)
{
	// This is neccessary for prediction
	uint32 iRandseed = m_iRandseed + m_iEnterAction;
	fp32 RandValue = MFloat_GetRand(iRandseed); /*((fp32)rand()) / RAND_MAX; //*/

	/*if (_pContext->m_pWPhysState->IsServer())
	ConOutL(CStrF("(Server)iObj: %d Rand1() = %f, - m_iRandseed %d, m_GameTime %3.3f", _pContext->m_pObj->m_iObject,RandValue, _pContext->m_pWPhysState, m_iRandseed, _pContext->m_GameTime));
	else
	ConOutL(CStrF("(Client)iObj: %d Rand1() = %f  - m_iRandseed %d, m_GameTime %3.3f", _pContext->m_pObj->m_iObject,RandValue, _pContext->m_pWPhysState, m_iRandseed, _pContext->m_GameTime));*/

	return CAGVal::From(RandValue);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:			Checks if a response has been handled

Parameters:			
_pContext:		AG context
_pParams:		AG params

Returns:			Whether the given response has been handled or not

Comments:			Needs to be checked for each response type, otherwise
animgraph might get confused....
\*_____________________________________________________________________________*/

CAGVal CWO_ClientData_AnimGraphInterface::Property_HandledResponse(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams)
{
	if (_pParams == NULL || _pParams->GetNumParams() == 0)
		return CAGVal::From(0.0f);

	uint8 Response = (uint8)_pParams->GetParam(0);

	return CAGVal::From((fp32)(Response & m_HandledResponse));
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:			Registers a handled response

Parameters:			
_pContext:		AG context
_pParams:		AG params

Comments:			Whenever a response is handled, it should be registered
here so it won't be handled twice
\*_____________________________________________________________________________*/
void CWO_ClientData_AnimGraphInterface::Effect_RegisterHandledResponse(const CWAGI_Context* _pContext, 
																		const CXRAG_ICallbackParams* _pParams)
{
	if (_pParams == NULL || _pParams->GetNumParams() == 0)
		return;

	m_HandledResponse |= (uint8) _pParams->GetParam(0);
}

//--------------------------------------------------------------------------------

bool CWO_ClientData_AnimGraphInterface::OperatorMOD(const CWAGI_Context* _pContext, const CAGVal &_OperandA, const CAGVal &_OperandB)
{
	if (_OperandA.m_bIsTime || _OperandB.m_bIsTime)
	{
		CMTime OperandA = _OperandA.GetTime();
		OperandA = OperandA.Modulus(_OperandB.GetTime());

		return OperandA.Compare(CMTime()) == 0;
	}
	else
	{
		fp32 Operand = _OperandA.GetFp();
		fp32 Mod = _OperandB.GetFp();

		while (Operand >= Mod)
			Operand -= Mod;
		return (_OperandA.GetFp() == 0);
	}

}

//--------------------------------------------------------------------------------

bool CWO_ClientData_AnimGraphInterface::ConditionHelper(const CWAGI_Context* _pContext, fp32 TimeOffset, fp32& _TimeFraction, bool _bOpEqual, bool _bOpGreater, bool _bOpLess)
{
	_TimeFraction = 0;

	if (TimeOffset == 0)
	{
		if (_bOpEqual && _pContext->m_bGameTimeIncluded)
		{
			_TimeFraction = 0;
			return true;
		}

		if (_bOpGreater)
		{
			_TimeFraction = 0.0001f; // Just to get past 0.
			return true;
		}

		return false;
	}

	if (TimeOffset < 0)
	{
		if (_bOpGreater)
		{
			_TimeFraction = 0;
			return true;
		}

		return false;
	}

	if (TimeOffset > _pContext->m_TimeSpan)
	{
		if (_bOpLess)
		{
			_TimeFraction = 0;
			return true;
		}

		return false;
	}

	if (_pContext->m_TimeSpan != 0)
		_TimeFraction = TimeOffset / _pContext->m_TimeSpan;

	return true;
}

//--------------------------------------------------------------------------------

PFN_ANIMGRAPH_CONDITION CWO_ClientData_AnimGraphInterface::ms_lpfnDefConditions[MAX_ANIMGRAPH_DEFCONDITIONS] =
{
/* 00 */ (PFN_ANIMGRAPH_CONDITION)&CWO_ClientData_AnimGraphInterface::Condition_StateTime,
/* 01 */ (PFN_ANIMGRAPH_CONDITION)NULL,//&CWO_ClientData_AnimGraphInterface::Condition_AnimExitPoint,
/* 02 */ (PFN_ANIMGRAPH_CONDITION)&CWO_ClientData_AnimGraphInterface::Condition_AnimLoopCount,
/* 03 */ (PFN_ANIMGRAPH_CONDITION)NULL,
/* 04 */ (PFN_ANIMGRAPH_CONDITION)&CWO_ClientData_AnimGraphInterface::Condition_AnimLoopCountOffset,
/* 05 */ (PFN_ANIMGRAPH_CONDITION)NULL,
/* 06 */ (PFN_ANIMGRAPH_CONDITION)NULL,
/* 07 */ (PFN_ANIMGRAPH_CONDITION)NULL,
/* 08 */ (PFN_ANIMGRAPH_CONDITION)NULL,
/* 09 */ (PFN_ANIMGRAPH_CONDITION)&CWO_ClientData_AnimGraphInterface::Condition_StateTimeOffset,
};

//--------------------------------------------------------------------------------

PFN_ANIMGRAPH_PROPERTY CWO_ClientData_AnimGraphInterface::ms_lpfnDefProperties[MAX_ANIMGRAPH_DEFPROPERTIES] =
{
/* 00 */ (PFN_ANIMGRAPH_PROPERTY)&CWO_ClientData_AnimGraphInterface::Property_StateTime,
/* 01 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 02 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 03 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 04 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 05 */ (PFN_ANIMGRAPH_PROPERTY)&CWO_ClientData_AnimGraphInterface::Property_Rand1,
/* 06 */ (PFN_ANIMGRAPH_PROPERTY)&CWO_ClientData_AnimGraphInterface::Property_Rand255,
/* 07 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 08 */ (PFN_ANIMGRAPH_PROPERTY)&CWO_ClientData_AnimGraphInterface::Property_HandledResponse,
/* 09 */ (PFN_ANIMGRAPH_PROPERTY)&CWO_ClientData_AnimGraphInterface::Property_StateTimeOffset,

/* 10 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 11 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 12 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 13 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 14 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 15 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 16 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 17 */ (PFN_ANIMGRAPH_PROPERTY)&CWO_ClientData_AnimGraphInterface::Property_LoopedAnimTime,

/* 18 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 19 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 20 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 21 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 22 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 23 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 24 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 25 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 26 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 27 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 28 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 29 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 30 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 31 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 32 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 33 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 34 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 35 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 36 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 37 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 38 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 39 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 40 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 41 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 42 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 43 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 44 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 45 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 46 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 47 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 48 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 49 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 50 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 51 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 52 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 53 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 54 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 55 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 56 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 57 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 58 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 59 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 60 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 61 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 62 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 63 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 64 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 65 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 66 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 67 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 68 */ (PFN_ANIMGRAPH_PROPERTY)NULL,
/* 69 */ (PFN_ANIMGRAPH_PROPERTY)NULL,

/* 70 */ (PFN_ANIMGRAPH_PROPERTY)&CWO_ClientData_AnimGraphInterface::Property_Dialogue0,
/* 71 */ (PFN_ANIMGRAPH_PROPERTY)&CWO_ClientData_AnimGraphInterface::Property_Dialogue1,
/* 72 */ (PFN_ANIMGRAPH_PROPERTY)&CWO_ClientData_AnimGraphInterface::Property_Dialogue2,
/* 73 */ (PFN_ANIMGRAPH_PROPERTY)&CWO_ClientData_AnimGraphInterface::Property_Dialogue3,
/* 74 */ (PFN_ANIMGRAPH_PROPERTY)&CWO_ClientData_AnimGraphInterface::Property_Dialogue4,
/* 75 */ (PFN_ANIMGRAPH_PROPERTY)&CWO_ClientData_AnimGraphInterface::Property_Dialogue5,
};

//--------------------------------------------------------------------------------

PFN_ANIMGRAPH_OPERATOR CWO_ClientData_AnimGraphInterface::ms_lpfnDefOperators[MAX_ANIMGRAPH_DEFOPERATORS] =
{
/* 00 */ (PFN_ANIMGRAPH_OPERATOR)&CWO_ClientData_AnimGraphInterface::OperatorEQ,
/* 01 */ (PFN_ANIMGRAPH_OPERATOR)&CWO_ClientData_AnimGraphInterface::OperatorGT,
/* 02 */ (PFN_ANIMGRAPH_OPERATOR)&CWO_ClientData_AnimGraphInterface::OperatorLT,
/* 03 */ (PFN_ANIMGRAPH_OPERATOR)&CWO_ClientData_AnimGraphInterface::OperatorGE,
/* 04 */ (PFN_ANIMGRAPH_OPERATOR)&CWO_ClientData_AnimGraphInterface::OperatorLE,
/* 05 */ (PFN_ANIMGRAPH_OPERATOR)&CWO_ClientData_AnimGraphInterface::OperatorNE,
/* 06 */ (PFN_ANIMGRAPH_OPERATOR)&CWO_ClientData_AnimGraphInterface::OperatorMOD,
};

//--------------------------------------------------------------------------------

PFN_ANIMGRAPH_EFFECT CWO_ClientData_AnimGraphInterface::ms_lpfnDefEffects[MAX_ANIMGRAPH_DEFEFFECTS] =
{
/* 00 */ NULL,
/* 01 */ NULL,
/* 02 */ NULL,
/* 03 */ NULL,
/* 04 */ NULL,
/* 05 */ NULL,
/* 06 */ NULL,
/* 07 */ NULL,
/* 08 */ NULL,
/* 09 */ NULL,

/* 10 */ NULL,//&CWO_Clientdata_Character_AnimGraph::Effect_FightMove,
/* 11 */ NULL,
/* 12 */ NULL,
/* 13 */ NULL,
/* 14 */ NULL,
/* 15 */ NULL,
/* 16 */ NULL,
/* 17 */ NULL,
/* 18 */ NULL,
/* 19 */ NULL,

/* 20 */ NULL,
/* 21 */ NULL,
/* 22 */ NULL,
/* 23 */ NULL,
/* 24 */ &CWO_ClientData_AnimGraphInterface::Effect_RegisterHandledResponse,
};
