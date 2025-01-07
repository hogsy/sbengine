#include "PCH.h"

#include "WAG2_ClientData.h"

void CWO_ClientData_AnimGraph2Interface::AG2_RefreshGlobalProperties(const CWAG2I_Context* _pContext)
{
}

void CWO_ClientData_AnimGraph2Interface::SetNumProperties(int32 _NumPropertiesFloat, int32 _NumPropertiesInt, int32 _NumPropertiesBool)
{
	SetNumPropertiesFloat(_NumPropertiesFloat);
	SetNumPropertiesInt(_NumPropertiesInt);
	SetNumPropertiesBool(_NumPropertiesBool);
}

void CWO_ClientData_AnimGraph2Interface::SetNumPropertiesFloat(int32 _NumPropertiesFloat)
{
	m_lPropertiesFloat.SetLen(_NumPropertiesFloat);
	// Set default values (0)
	for (int32 i = 0; i < _NumPropertiesFloat; i++)
		m_lPropertiesFloat[i] = 0.0f;
}

void CWO_ClientData_AnimGraph2Interface::SetNumPropertiesInt(int32 _NumPropertiesInt)
{
	m_lPropertiesInt.SetLen(_NumPropertiesInt);
	// Set default values (0)
	for (int32 i = 0; i < _NumPropertiesInt; i++)
		m_lPropertiesInt[i] = 0;
}

void CWO_ClientData_AnimGraph2Interface::SetNumPropertiesBool(int32 _NumPropertiesBool)
{
	m_lPropertiesBool.SetLen(_NumPropertiesBool);
	// Set default values (false)
	for (int32 i = 0; i < _NumPropertiesBool; i++)
		m_lPropertiesBool[i] = false;
}

void CWO_ClientData_AnimGraph2Interface::AG2_RefreshStateInstanceProperties(const CWAG2I_Context* _pContext, const CWAG2I_StateInstance* _pStateInstance)
{
	m_EnterStateTime = _pStateInstance->GetEnterTime();
	//m_iEnterAction = _pStateInstance->GetEnterActionIndex();
	m_iEnterMoveToken = _pStateInstance->GetEnterMoveTokenIndex();

	m_iRandseed = _pStateInstance->GetAG2I()->GetRandseed();

	m_AnimLoopDuration = _pStateInstance->GetAnimLoopDuration_Cached();

	m_EnterAnimTimeOffset = _pStateInstance->GetEnterAnimTimeOffset();
}

void CWO_ClientData_AnimGraph2Interface::Clear()
{
	m_iRandseed = 0;
	m_EnterStateTime.Reset();
	//m_iEnterAction = -1;
	m_iEnterMoveToken = -1;
	m_AdjustmentOffset = 0.0f;
	m_AdjustmentCutOff = 0.0f;
	m_AnimMoveScale = 1.0f;
	m_AnimLoopDuration = 0.0f;
	m_EnterAnimTimeOffset = 0.0f;
	m_SyncAnimScale = 0.0f;
	m_AdaptiveTimeScale = 0.0f;
	m_ImportantAGEvent = 0;
	m_WallCollision = 0;
	m_HandledResponse = 0;
	m_MDestination.Unit();
	m_MStartPosition.Unit();
	m_MDestination.GetRow(0) = _FP32_MAX;
	m_MDestination.GetRow(3) = _FP32_MAX;
	m_MDestinationSpeed = 0.0f;
	m_ModeFlags = 0;

	m_UpVector = CVec3Dfp32(0,0,1);
}

void CWO_ClientData_AnimGraph2Interface::Copy(const CWO_ClientData_AnimGraph2Interface& _CD)
{
	m_MDestination = _CD.m_MDestination;
	m_MStartPosition = _CD.m_MStartPosition;
	m_MDestinationSpeed = _CD.m_MDestinationSpeed;
	m_iRandseed = _CD.m_iRandseed;
	m_EnterStateTime = _CD.m_EnterStateTime;
	//m_iEnterAction = _CD.m_iEnterAction;
	m_iEnterMoveToken = _CD.m_iEnterMoveToken;
	m_AdjustmentOffset = _CD.m_AdjustmentOffset;
	m_AdjustmentCutOff = _CD.m_AdjustmentCutOff;
	m_AnimMoveScale = _CD.m_AnimMoveScale;
	m_AnimLoopDuration = _CD.m_AnimLoopDuration;
	m_SyncAnimScale = _CD.m_SyncAnimScale;
	m_AdaptiveTimeScale = _CD.m_AdaptiveTimeScale;
	int32 Len = _CD.m_lPropertiesFloat.Len();
	m_lPropertiesFloat.SetLen(Len);
	for (int32 i = 0; i < Len; i++)
		m_lPropertiesFloat[i] = _CD.m_lPropertiesFloat[i];
	
	Len = _CD.m_lPropertiesInt.Len();
	m_lPropertiesInt.SetLen(Len);
	for (int32 i = 0; i < Len; i++)
		m_lPropertiesInt[i] = _CD.m_lPropertiesInt[i];

	Len = _CD.m_lPropertiesBool.Len();
	m_lPropertiesBool.SetLen(Len);
	for (int32 i = 0; i < Len; i++)
		m_lPropertiesBool[i] = _CD.m_lPropertiesBool[i];

	m_EnterAnimTimeOffset = _CD.m_EnterAnimTimeOffset;
	m_WallCollision = _CD.m_WallCollision;
	m_ImportantAGEvent = _CD.m_ImportantAGEvent;
	m_HandledResponse = _CD.m_HandledResponse;
	m_ModeFlags = _CD.m_ModeFlags;
	m_UpVector = _CD.m_UpVector;
}

//--------------------------------------------------------------------------------

bool CWO_ClientData_AnimGraph2Interface::Condition_StateTime(const CWAG2I_Context* _pContext, int _iOperator, const CAG2Val &_Constant, fp32& _TimeFraction)
{
	bool bOperatorMod = (AG2_EvaluateOperator(_pContext, _iOperator, _Constant.Scale(0.0f), _Constant) &&
		!AG2_EvaluateOperator(_pContext, _iOperator, _Constant.Scale(0.5f), _Constant) &&
		AG2_EvaluateOperator(_pContext, _iOperator, _Constant, _Constant) &&
		!AG2_EvaluateOperator(_pContext, _iOperator, _Constant.Scale(1.5f), _Constant) &&
		AG2_EvaluateOperator(_pContext, _iOperator, _Constant.Scale(2.0f), _Constant));

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
		bool bOperatorEqual = AG2_EvaluateOperator(_pContext, _iOperator, CAG2Val::From(0), CAG2Val::From(0));
		bool bOperatorGreater = AG2_EvaluateOperator(_pContext, _iOperator, CAG2Val::From(1), CAG2Val::From(0));
		bool bOperatorLess = AG2_EvaluateOperator(_pContext, _iOperator, CAG2Val::From(0), CAG2Val::From(1));

		CMTime StateTime = _pContext->m_GameTime - m_EnterStateTime;
		fp32 TimeOffset = (_Constant.GetTime() - StateTime).GetTime();
		return ConditionHelper(_pContext, TimeOffset, _TimeFraction, bOperatorEqual, bOperatorGreater, bOperatorLess);
	}
}

//--------------------------------------------------------------------------------

bool CWO_ClientData_AnimGraph2Interface::Condition_AnimLoopCount(const CWAG2I_Context* _pContext, int _iOperator, const CAG2Val &_Constant, fp32& _TimeFraction)
{
	bool bOperatorEqual = AG2_EvaluateOperator(_pContext, _iOperator, CAG2Val::From(0), CAG2Val::From(0));
	bool bOperatorGreater = AG2_EvaluateOperator(_pContext, _iOperator, CAG2Val::From(1), CAG2Val::From(0));
	bool bOperatorLess = AG2_EvaluateOperator(_pContext, _iOperator, CAG2Val::From(0), CAG2Val::From(1));

	CMTime StateTime = _pContext->m_GameTime - m_EnterStateTime;
	CMTime TimeOffset = (_Constant.GetTime().Scale(m_AnimLoopDuration)) - StateTime;

	return ConditionHelper(_pContext, TimeOffset.GetTime(), _TimeFraction, bOperatorEqual, bOperatorGreater, bOperatorLess);
}

bool CWO_ClientData_AnimGraph2Interface::Condition_LoopedAnimCount(const CWAG2I_Context* _pContext, int _iOperator, const CAG2Val &_Constant, fp32& _TimeFraction)
{
	bool bOperatorEqual = AG2_EvaluateOperator(_pContext, _iOperator, CAG2Val::From(0), CAG2Val::From(0));
	bool bOperatorGreater = AG2_EvaluateOperator(_pContext, _iOperator, CAG2Val::From(1), CAG2Val::From(0));
	bool bOperatorLess = AG2_EvaluateOperator(_pContext, _iOperator, CAG2Val::From(0), CAG2Val::From(1));

	fp32 StateTime = (_pContext->m_GameTime - m_EnterStateTime).GetTime();
	fp32 CurrentAnimCount = M_Floor(StateTime/m_AnimLoopDuration);
	fp32 TimeOffset = ((_Constant.GetTime().GetTime() + CurrentAnimCount) * m_AnimLoopDuration) - StateTime;

	return ConditionHelper(_pContext, TimeOffset, _TimeFraction, bOperatorEqual, bOperatorGreater, bOperatorLess);
}

//--------------------------------------------------------------------------------

bool CWO_ClientData_AnimGraph2Interface::Condition_AnimLoopCountOffset(const CWAG2I_Context* _pContext, int _iOperator, const CAG2Val &_Constant, fp32& _TimeFraction)
{
	bool bOperatorEqual = AG2_EvaluateOperator(_pContext, _iOperator, CAG2Val::From(0), CAG2Val::From(0));
	bool bOperatorGreater = AG2_EvaluateOperator(_pContext, _iOperator, CAG2Val::From(1), CAG2Val::From(0));
	bool bOperatorLess = AG2_EvaluateOperator(_pContext, _iOperator, CAG2Val::From(0), CAG2Val::From(1));

	fp32 StateTime = (_pContext->m_GameTime - m_EnterStateTime).GetTime() + m_EnterAnimTimeOffset;
	fp32 TimeOffset = (_Constant.GetTime().GetTime() * m_AnimLoopDuration) - StateTime;
	return ConditionHelper(_pContext, TimeOffset, _TimeFraction, bOperatorEqual, bOperatorGreater, bOperatorLess);
}
//--------------------------------------------------------------------------------

bool CWO_ClientData_AnimGraph2Interface::Condition_StateTimeOffset(const CWAG2I_Context* _pContext, int _iOperator, const CAG2Val &_Constant, fp32& _TimeFraction)
{
	bool bOperatorMod = (AG2_EvaluateOperator(_pContext, _iOperator, _Constant.Scale(0.0f), _Constant) &&
		!AG2_EvaluateOperator(_pContext, _iOperator, _Constant.Scale(0.5f), _Constant) &&
		AG2_EvaluateOperator(_pContext, _iOperator, _Constant, _Constant) &&
		!AG2_EvaluateOperator(_pContext, _iOperator, _Constant.Scale(1.5f), _Constant) &&
		AG2_EvaluateOperator(_pContext, _iOperator, _Constant.Scale(2.0f), _Constant));

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
		bool bOperatorEqual = AG2_EvaluateOperator(_pContext, _iOperator, CAG2Val::From(0), CAG2Val::From(0));
		bool bOperatorGreater = AG2_EvaluateOperator(_pContext, _iOperator, CAG2Val::From(1), CAG2Val::From(0));
		bool bOperatorLess = AG2_EvaluateOperator(_pContext, _iOperator, CAG2Val::From(0), CAG2Val::From(1));

		CMTime StateTime = _pContext->m_GameTime - m_EnterStateTime + CMTime::CreateFromSeconds(m_EnterAnimTimeOffset);
		fp32 TimeOffset = (_Constant.GetTime() - StateTime).GetTime();
		return ConditionHelper(_pContext, TimeOffset, _TimeFraction, bOperatorEqual, bOperatorGreater, bOperatorLess);
	}
}

bool CWO_ClientData_AnimGraph2Interface::SetPackedAnimProperty(const CWAG2I_Context *_pContext, uint32 _iProperty, fp32 _Value)
{
	if (!m_pAG2I->AcquireAllResources(_pContext))
		return false;

	int iType = _iProperty >> AG2_PACKEDANIMPROPERTY_TYPESHIFT;
	switch(iType)
	{
		case AG2_PACKEDANIMPROPERTY_TYPE_SETPROPERTY:
			{
				int iType = _iProperty >> AG2_PACKEDANIMPROPERTY_SETPROPERTY_TYPESHIFT;
				_iProperty &= AG2_PACKEDANIMPROPERTY_SETPROPERTY_PROPERTYMASK;
				/*if(iType == 0 && (

				//_iProperty == PROPERTY_FLOAT_MOVERADIUSCONTROL ||
				_iProperty == PROPERTY_FLOAT_MOVEANGLECONTROL ||
				_iProperty == PROPERTY_FLOAT_MOVEANGLESCONTROL ||
				_iProperty == PROPERTY_FLOAT_MOVEANGLEUNITCONTROL ||
				_iProperty == PROPERTY_FLOAT_MOVEANGLEUNITSCONTROL ||
				_iProperty == PROPERTY_FLOAT_MOVEVELOCITY ||
				_iProperty == PROPERTY_FLOAT_MOVEVELOCITYVERTICAL ||
				_iProperty == PROPERTY_FLOAT_MOVEANGLE ||
				_iProperty == PROPERTY_FLOAT_MOVEANGLES ||
				_iProperty == PROPERTY_FLOAT_MOVEANGLEUNIT ||
				_iProperty == PROPERTY_FLOAT_MOVEANGLEUNITS ||
				_iProperty == PROPERTY_FLOAT_MOVEHCONTROL ||
				_iProperty == PROPERTY_FLOAT_MOVEVCONTROL ||
				_iProperty == PROPERTY_FLOAT_ANGLEDIFFS))

				return;*/

				switch(iType)
				{
				case 0:
					if(m_lPropertiesFloat.Len() > _iProperty)
						m_lPropertiesFloat[_iProperty] = _Value;
					break;
				case 1:
					if(m_lPropertiesInt.Len() > _iProperty)
						m_lPropertiesInt[_iProperty] = RoundToInt(_Value);
					break;
				case 2:
					if(m_lPropertiesBool.Len() > _iProperty)
						m_lPropertiesBool[_iProperty] = _Value != 0;
					break;
				case 3:
					return false;
				}
			}
			break;
		case AG2_PACKEDANIMPROPERTY_TYPE_SENDIMPULSE:
			{
				int iToken = (_iProperty >> AG2_PACKEDANIMPROPERTY_TOKENSHIFT) & AG2_PACKEDANIMPROPERTY_TOKENMASK;
				CXRAG2_Impulse Impulse;
				Impulse.m_ImpulseType = _iProperty & AG2_PACKEDANIMPROPERTY_SENDIMPULSE_IMPULSETYPEMASK;
				Impulse.m_ImpulseValue = RoundToInt(_Value);
				return m_pAG2I->SendImpulse(_pContext, Impulse, iToken);
			}
			break;
		case AG2_PACKEDANIMPROPERTY_TYPE_MOVETOKEN:
			{
				m_pAG2I->ClearTokens();
				int iToken = (_iProperty >> AG2_PACKEDANIMPROPERTY_TOKENSHIFT) & AG2_PACKEDANIMPROPERTY_TOKENMASK;
				int iEnterMoveToken = ((unsigned int)_iProperty >> AG2_PACKEDANIMPROPERTY_MOVETOKEN_GRAPHBLOCKSHIFT) & AG2_PACKEDANIMPROPERTY_MOVETOKEN_GRAPHBLOCKMASK;
				int iGraphBlockMoveToken = (unsigned int)_iProperty & AG2_PACKEDANIMPROPERTY_MOVETOKEN_ACTIONMASK;

				m_pAG2I->MoveGraphBlock(_pContext, 0, iToken, iGraphBlockMoveToken);
				m_pAG2I->MoveAction(_pContext, 0, iToken, iEnterMoveToken);
			}
			break;
		case AG2_PACKEDANIMPROPERTY_TYPE_DIALOGUE:
			{
				int iToken = (_iProperty >> AG2_PACKEDANIMPROPERTY_TOKENSHIFT) & AG2_PACKEDANIMPROPERTY_TOKENMASK;
				CXRAG2_Impulse Impulse;
				Impulse.m_ImpulseType = _iProperty & AG2_PACKEDANIMPROPERTY_SENDIMPULSE_IMPULSETYPEMASK;
				Impulse.m_ImpulseValue = RoundToInt(_Value);

				int16 iMatchingValue;
				switch(iToken)
				{
				case AG2_TOKEN_DIALOG_BASE: 
					SetPropertyInt(PROPERTY_INT_DIALOG_BASE, Impulse.m_ImpulseValue);
					iMatchingValue = AG2_IMPULSEVALUE_DIALOG_BASE;
					break;
				case AG2_TOKEN_DIALOG_GESTURE:
					SetPropertyInt(PROPERTY_INT_DIALOG_GESTURE, Impulse.m_ImpulseValue);
					iMatchingValue = AG2_IMPULSEVALUE_DIALOG_GESTURE;
					break;
				case AG2_TOKEN_DIALOG_FACIAL:
					SetPropertyInt(PROPERTY_INT_DIALOG_FACIAL, Impulse.m_ImpulseValue);
					iMatchingValue = AG2_IMPULSEVALUE_DIALOG_FACIAL;
					break;
				case AG2_TOKEN_DIALOG_FACIALGESTURE:
				case AG2_TOKEN_DIALOG_FACIALGESTURE + 1:
					iMatchingValue = AG2_IMPULSEVALUE_DIALOG_FACIALGESTURE;
					break;
				default: return false;
				}

				// Ok, then a little more specialized, check if we have the token already
				const CWAG2I_Token* pToken = m_pAG2I->GetTokenFromID(iToken);
				if (pToken)
				{
					// Just send the impulse
					return m_pAG2I->SendImpulse(_pContext,Impulse, iToken, true);
				}
				else
				{
					// Create a token
					const CXRAG2* pAG = m_pAG2I->GetAnimGraph(0);
					if (pAG)
					{
						CAG2GraphBlockIndex iBlock = pAG->GetMatchingGraphBlock(CXRAG2_Impulse(AG2_CLD_IMPULSETYPE_DIALOG, iMatchingValue));
						if (iBlock != -1)
						{
							CAG2ReactionIndex iReaction = pAG->GetMatchingReaction(iBlock, Impulse);
							const CXRAG2_Reaction* pReaction = pAG->GetReaction(iReaction);
							if (pReaction)
								m_pAG2I->MoveGraphBlock(_pContext,0,iToken,pReaction->GetBaseMoveTokenIndex());
						}
					}
				}
			}
			break;
	}

	return true;
}

void CWO_ClientData_AnimGraph2Interface::PrecacheDialog(const CWAG2I_Context* _pContext, int16 _DialogType, const TArray<CXRAG2_Impulse>& _lReactions)
{
	if (!m_pAG2I)
		return;

	CXRAG2_Impulse Block(AG2_CLD_IMPULSETYPE_DIALOG, _DialogType);
	m_pAG2I->TagAnimSetFromBlockReaction(_pContext,_pContext->m_pWPhysState->GetMapData(),_pContext->m_pWPhysState->GetWorldData(),Block,_lReactions,true);
}

void CWO_ClientData_AnimGraph2Interface::EnumerateDialogTypes(const CWAG2I_Context* _pContext, int16 _DialogType, TArray<int16>& _lTypes, TArray<CStr>* _lpDesc)
{
	// Create a token
	const CXRAG2* pAG = m_pAG2I->GetAnimGraph(0);
	if (pAG)
	{
		CAG2GraphBlockIndex iBlock = pAG->GetMatchingGraphBlock(CXRAG2_Impulse(AG2_CLD_IMPULSETYPE_DIALOG, _DialogType));
		if (iBlock != -1)
		{
			const CXRAG2_GraphBlock* pBlock = pAG->GetGraphBlock(iBlock);
			if (!pBlock)
				return;

			CAG2ReactionIndex iReactionBase = pBlock->GetBaseReactionIndex();
			int32 NumReactions = iReactionBase + pBlock->GetNumReactions();
			for (int32 i = iReactionBase; i < NumReactions; i++)
			{
				const CXRAG2_Reaction* pReaction = pAG->GetReaction(i);
				if (pReaction && pReaction->m_Impulse.m_ImpulseType == AG2_CLD_IMPULSETYPE_DIALOGTYPE)
				{
					_lTypes.Add(pReaction->m_Impulse.m_ImpulseValue);
#ifndef M_RTM
					if (_lpDesc)
					{
						CStr Name = pAG->GetExportedReactionName(i);
						Name.GetStrSep("REACTION_");
						_lpDesc->Add(Name);
					}
#endif
				}
			}
		}
	}
}
//--------------------------------------------------------------------------------

/*bool CWO_ClientData_AnimGraph2Interface::Condition_AnimExitPoint(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams, int _iOperator, fp32 _Constant, fp32& _TimeFraction)
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

/*CAG2Val CWO_ClientData_AnimGraph2Interface::Property_LoopedAnimTime(const CWAG2I_Context* _pContext,
																const CXRAG2_ICallbackParams* _pParams)
{	
	CMTime StateTime = (_pContext->m_GameTime - m_EnterStateTime).Modulus(m_AnimLoopDuration);
	return CAG2Val::From(StateTime);
}

//--------------------------------------------------------------------------------

CAG2Val CWO_ClientData_AnimGraph2Interface::Property_StateTimeOffset(const CWAG2I_Context* _pContext, 
																 const CXRAG2_ICallbackParams* _pParams)
{
	// Find statetime with offset
	CMTime StateTime = (_pContext->m_GameTime - m_EnterStateTime) + CMTime::CreateFromSeconds(m_EnterAnimTimeOffset);
	return CAG2Val::From(StateTime);
}

//--------------------------------------------------------------------------------

CAG2Val CWO_ClientData_AnimGraph2Interface::Property_Rand1(const CWAG2I_Context* _pContext,
													   const CXRAG2_ICallbackParams* _pParams)
{
	// This is neccessary for prediction
	uint32 iRandseed = m_iRandseed + m_iEnterMoveToken;// + m_iEnterAction;
	fp32 RandValue = MFloat_GetRand(iRandseed);

	return CAG2Val::From(RandValue);
}

CAG2Val CWO_ClientData_AnimGraph2Interface::Property_HandledResponse(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams)
{
	if (_pParams == NULL || _pParams->GetNumParams() == 0)
		return CAG2Val::From(0.0f);

	uint8 Response = (uint8)_pParams->GetParam(0);

	return CAG2Val::From((fp32)(Response & m_HandledResponse));
}*/

void CWO_ClientData_AnimGraph2Interface::Effect_RegisterHandledResponse(const CWAG2I_Context* _pContext, 
																		const CXRAG2_ICallbackParams* _pParams)
{
	if (_pParams == NULL || _pParams->GetNumParams() == 0)
		return;

	m_HandledResponse |= (uint8) _pParams->GetParam(0);
}

//--------------------------------------------------------------------------------

bool CWO_ClientData_AnimGraph2Interface::OperatorMOD(const CWAG2I_Context* _pContext, const CAG2Val &_OperandA, const CAG2Val &_OperandB)
{
	if (_OperandA.IsTime() || _OperandB.IsTime())
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

bool CWO_ClientData_AnimGraph2Interface::ConditionHelper(const CWAG2I_Context* _pContext, fp32 TimeOffset, fp32& _TimeFraction, bool _bOpEqual, bool _bOpGreater, bool _bOpLess)
{
	_TimeFraction = 0;

	if (TimeOffset == 0)
	{
		if (_bOpEqual)
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

CAG2Val CWO_ClientData_AnimGraph2Interface::Property_NULL(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams)
{
	return CAG2Val::From(0);
}

//--------------------------------------------------------------------------------

PFN_ANIMGRAPH2_CONDITION CWO_ClientData_AnimGraph2Interface::ms_lpfnDefConditions[MAX_ANIMGRAPH2_DEFCONDITIONS] =
{
/* 00 */ (PFN_ANIMGRAPH2_CONDITION)&CWO_ClientData_AnimGraph2Interface::Condition_StateTime,
/* 01 */ (PFN_ANIMGRAPH2_CONDITION)NULL,//&CWO_ClientData_AnimGraph2Interface::Condition_AnimExitPoint,
/* 02 */ (PFN_ANIMGRAPH2_CONDITION)&CWO_ClientData_AnimGraph2Interface::Condition_AnimLoopCount,
/* 03 */ (PFN_ANIMGRAPH2_CONDITION)&CWO_ClientData_AnimGraph2Interface::Condition_AnimLoopCountOffset,
/* 04 */ (PFN_ANIMGRAPH2_CONDITION)NULL,
/* 05 */ (PFN_ANIMGRAPH2_CONDITION)NULL,
/* 06 */ (PFN_ANIMGRAPH2_CONDITION)NULL,
/* 07 */ (PFN_ANIMGRAPH2_CONDITION)NULL,
/* 08 */ (PFN_ANIMGRAPH2_CONDITION)NULL,
/* 09 */ (PFN_ANIMGRAPH2_CONDITION)&CWO_ClientData_AnimGraph2Interface::Condition_StateTimeOffset,
};

PFN_ANIMGRAPH2_PROPERTY CWO_ClientData_AnimGraph2Interface::ms_lpfnDefProperties[MAX_ANIMGRAPH2_DEFPROPERTIES] =
{
	/* 00 */ (PFN_ANIMGRAPH2_PROPERTY)NULL,
	/* 01 */ (PFN_ANIMGRAPH2_PROPERTY)NULL,//&CWO_ClientData_AnimGraph2Interface::Condition_AnimExitPoint,
	/* 02 */ (PFN_ANIMGRAPH2_PROPERTY)NULL,
	/* 03 */ (PFN_ANIMGRAPH2_PROPERTY)NULL,
	/* 04 */ (PFN_ANIMGRAPH2_PROPERTY)NULL,
	/* 05 */ (PFN_ANIMGRAPH2_PROPERTY)NULL,
	/* 06 */ (PFN_ANIMGRAPH2_PROPERTY)NULL,
	/* 07 */ (PFN_ANIMGRAPH2_PROPERTY)NULL,
	/* 08 */ (PFN_ANIMGRAPH2_PROPERTY)NULL,
	/* 09 */ (PFN_ANIMGRAPH2_PROPERTY)NULL,
	/* 10 */ (PFN_ANIMGRAPH2_PROPERTY)NULL,
	/* 11 */ (PFN_ANIMGRAPH2_PROPERTY)NULL,
	/* 12 */ (PFN_ANIMGRAPH2_PROPERTY)NULL,
	/* 13 */ (PFN_ANIMGRAPH2_PROPERTY)NULL,
	/* 14 */ (PFN_ANIMGRAPH2_PROPERTY)NULL,
	/* 15 */ (PFN_ANIMGRAPH2_PROPERTY)NULL,
	/* 16 */ (PFN_ANIMGRAPH2_PROPERTY)NULL,
	/* 17 */ (PFN_ANIMGRAPH2_PROPERTY)&CWO_ClientData_AnimGraph2Interface::Property_NULL,
};


//--------------------------------------------------------------------------------

PFN_ANIMGRAPH2_OPERATOR CWO_ClientData_AnimGraph2Interface::ms_lpfnDefOperators[MAX_ANIMGRAPH2_DEFOPERATORS] =
{
/* 00 */ (PFN_ANIMGRAPH2_OPERATOR)&CWO_ClientData_AnimGraph2Interface::OperatorEQ,
/* 01 */ (PFN_ANIMGRAPH2_OPERATOR)&CWO_ClientData_AnimGraph2Interface::OperatorGT,
/* 02 */ (PFN_ANIMGRAPH2_OPERATOR)&CWO_ClientData_AnimGraph2Interface::OperatorLT,
/* 03 */ (PFN_ANIMGRAPH2_OPERATOR)&CWO_ClientData_AnimGraph2Interface::OperatorGE,
/* 04 */ (PFN_ANIMGRAPH2_OPERATOR)&CWO_ClientData_AnimGraph2Interface::OperatorLE,
/* 05 */ (PFN_ANIMGRAPH2_OPERATOR)&CWO_ClientData_AnimGraph2Interface::OperatorNE,
/* 06 */ (PFN_ANIMGRAPH2_OPERATOR)&CWO_ClientData_AnimGraph2Interface::OperatorMOD,
/* 07 */ (PFN_ANIMGRAPH2_OPERATOR)&CWO_ClientData_AnimGraph2Interface::OperatorAND,
/* 08 */ (PFN_ANIMGRAPH2_OPERATOR)&CWO_ClientData_AnimGraph2Interface::OperatorNOTAND,
};

//--------------------------------------------------------------------------------

PFN_ANIMGRAPH2_EFFECT CWO_ClientData_AnimGraph2Interface::ms_lpfnDefEffects[MAX_ANIMGRAPH2_DEFEFFECTS] =
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
/* 24 */ &CWO_ClientData_AnimGraph2Interface::Effect_RegisterHandledResponse,
};

#ifdef AG2_RECORDPROPERTYCHANGES
CXRAG2_PropertyRecorder::CXRAG2_PropertyRecorder()
{
	m_pWAG2I = NULL;
	m_State = AG2_PROPERTYRECORDER_NONE;
}

void CXRAG2_PropertyRecorder::StartRecording(const CWAG2I_Context* _pContext, CWAG2I* _pWAG2I, fp32 _LowerBodyOffset)
{
	m_pWAG2I = _pWAG2I;
	if (!m_pWAG2I || m_State != AG2_PROPERTYRECORDER_NONE)
		return;
	m_State = AG2_PROPERTYRECORDER_RECORDING;
	
	m_lTokens.Clear();
	m_lIntChanges.Clear();
	m_lFloatChanges.Clear();
	m_lBoolChanges.Clear();
	m_lFunctionChanges.Clear();
	m_LowerBodyOffsetChanges.m_lChanges.Clear();
	m_ImpulseChanges.m_lChanges.Clear();
	m_PositionChanges.m_lChanges.Clear();
	CMat4Dfp32 CurPos = _pContext->m_pObj->GetPositionMatrix();
	m_PositionChanges.AddChange(0.0f,CurPos);
	CurPos.Inverse(m_PositionChanges.m_InvStartPosition);

	m_StartTime = _pContext->m_GameTime;
	// Get initial tokens and their primary state /statetimes
	m_lTokens.SetLen(m_pWAG2I->GetNumTokens());
	for (int32 i = 0; i < m_lTokens.Len(); i++)
	{
		m_lTokens[i].m_iToken = _pWAG2I->m_lTokens[i].m_ID;
		const CXRAG2_GraphBlock* pGraphBlock = _pWAG2I->GetGraphBlock(_pWAG2I->m_lTokens[i].m_iGraphBlock,0);
		const CWAG2I_StateInstance* pStateInst = _pWAG2I->m_lTokens[i].GetTokenStateInstance();
		if (pStateInst && pGraphBlock)
		{
			m_lTokens[i].m_iGraphBlockMoveToken = pGraphBlock->GetStartMoveTokenIndex();
			m_lTokens[i].m_iEnterMoveToken = pStateInst->m_iEnterMoveToken;
			m_lTokens[i].m_StateTime = (_pContext->m_GameTime - pStateInst->m_EnterTime).GetTime();
		}
		else
		{
			m_lTokens[i].m_iGraphBlockMoveToken = 0;
			m_lTokens[i].m_iEnterMoveToken = 0;
			m_lTokens[i].m_StateTime = 0.0f;
		}
	}

	// Get initial properties
	m_lIntChanges.SetLen(m_pWAG2I->m_pEvaluator->m_lPropertiesInt.Len());
	m_lFloatChanges.SetLen(m_pWAG2I->m_pEvaluator->m_lPropertiesFloat.Len());
	m_lBoolChanges.SetLen(m_pWAG2I->m_pEvaluator->m_lPropertiesBool.Len());
	m_lFunctionChanges.SetLen(m_pWAG2I->m_pEvaluator->m_nProperties);
	TAP_RCD<int> lPropertiesInt = m_pWAG2I->m_pEvaluator->m_lPropertiesInt;
	for (int32 i = 0; i < lPropertiesInt.Len(); i++)
	{
		m_lIntChanges[i].AddChange(0.0f,lPropertiesInt[i]);
	}
	TAP_RCD<fp32> lPropertiesFloat = m_pWAG2I->m_pEvaluator->m_lPropertiesFloat;
	for (int32 i = 0; i < lPropertiesFloat.Len(); i++)
	{
		m_lFloatChanges[i].AddChange(0.0f,lPropertiesFloat[i]);
	}
	TAP_RCD<bool> lPropertiesBool = m_pWAG2I->m_pEvaluator->m_lPropertiesBool;
	for (int32 i = 0; i < lPropertiesBool.Len(); i++)
	{
		m_lBoolChanges[i].AddChange(0.0f,lPropertiesBool[i]);
	}
	for (int32 i = 0; i < m_lFunctionChanges.Len(); i++)
	{
		if (m_pWAG2I->m_pEvaluator->m_lpfnProperties[i])
		{
			CAG2Val Val = (m_pWAG2I->m_pEvaluator->*(m_pWAG2I->m_pEvaluator->m_lpfnProperties[i]))(_pContext);
			m_lFunctionChanges[i].AddChange(0.0f,Val.GetInt());
		}
		else
		{
			m_lFunctionChanges[i].AddChange(0.0f,0);
		}
	}
	m_LowerBodyOffsetChanges.AddChange(0.0f,_LowerBodyOffset);
}

void CXRAG2_PropertyRecorder::Update(const CWAG2I_Context* _pContext, fp32 _LowerBodyOffset)
{
	if (!m_pWAG2I || m_State != AG2_PROPERTYRECORDER_RECORDING)
		return;

	fp32 CurTime = (_pContext->m_GameTime - m_StartTime).GetTime();
	TAP_RCD<int> lPropertiesInt = m_pWAG2I->m_pEvaluator->m_lPropertiesInt;
	TAP_RCD<CPropertyChanges> lChanges = m_lIntChanges;
	if (lChanges.Len() == lPropertiesInt.Len())
	{
		for (int32 i = 0; i < lPropertiesInt.Len(); i++)
		{
			if (lChanges[i].GetLatestInt() != lPropertiesInt[i])
				lChanges[i].AddChange(CurTime,lPropertiesInt[i]);
		}
	}

	TAP_RCD<fp32> lPropertiesFloat = m_pWAG2I->m_pEvaluator->m_lPropertiesFloat;
	lChanges = m_lFloatChanges;
	if (lChanges.Len() == lPropertiesFloat.Len())
	{
		for (int32 i = 0; i < lPropertiesFloat.Len(); i++)
		{
			if (lChanges[i].GetLatestFloat() != lPropertiesFloat[i])
				lChanges[i].AddChange(CurTime,lPropertiesFloat[i]);
		}
	}

	TAP_RCD<bool> lPropertiesBool = m_pWAG2I->m_pEvaluator->m_lPropertiesBool;
	lChanges = m_lBoolChanges;
	if (lChanges.Len() == lPropertiesBool.Len())
	{
		for (int32 i = 0; i < lPropertiesBool.Len(); i++)
		{
			if (lChanges[i].GetLatestBool() != lPropertiesBool[i])
				lChanges[i].AddChange(CurTime,lPropertiesBool[i]);
		}
	}

	// Check function changes
	lChanges = m_lFunctionChanges;
	for (int32 i = 0; i < lChanges.Len(); i++)
	{
		if (m_pWAG2I->m_pEvaluator->m_lpfnProperties[i])
		{
			CAG2Val Val = (m_pWAG2I->m_pEvaluator->*(m_pWAG2I->m_pEvaluator->m_lpfnProperties[i]))(_pContext);
			if (lChanges[i].GetLatestInt() != Val.GetInt())
				lChanges[i].AddChange(CurTime,Val.GetInt());
		}
	}
	// Lower body offset changes
	if (m_LowerBodyOffsetChanges.GetLatestFloat() != _LowerBodyOffset)
		m_LowerBodyOffsetChanges.AddChange(CurTime,_LowerBodyOffset);

	// Check position changes
	CMat4Dfp32 NewVal = _pContext->m_pObj->GetPositionMatrix();
	if (!m_PositionChanges.GetLatest().AlmostEqual(NewVal,0.05f))
		m_PositionChanges.AddChange(CurTime,NewVal);
}

void CXRAG2_PropertyRecorder::AddImpulse(const CWAG2I_Context* _pContext, CXRAG2_Impulse _Impulse, int8 _Token)
{
	if (!m_pWAG2I || m_State != AG2_PROPERTYRECORDER_RECORDING)
		return;
	fp32 CurTime = (_pContext->m_GameTime - m_StartTime).GetTime();
	m_ImpulseChanges.AddChange(CurTime,_Impulse, _Token);
}

void CXRAG2_PropertyRecorder::StopOperation()
{
	m_State = AG2_PROPERTYRECORDER_NONE;
}

void CXRAG2_PropertyRecorder::StartUpdating(CWAG2I* _pWAG2I)
{
	m_pWAG2I = _pWAG2I;
	m_State = AG2_PROPERTYRECORDER_PLAYBACK;
}

void CXRAG2_PropertyRecorder::UpdateToAnimGraph(CWAG2I_Context* _pContext, fp32 _CurTime, fp32 _LastTime)
{
	if (!m_pWAG2I || m_State != AG2_PROPERTYRECORDER_PLAYBACK)
		return;
	// Updates all properties all the time
	// If time is zero, move animgraph to the start graphblock/state, otherwise just 
	// update the properties
	if (_pContext->m_GameTime.GetTime() == 0.0f)
	{
		m_pWAG2I->ClearTokens();
		TAP_RCD<CTokenInfo> lTokens = m_lTokens;
		for (int32 i = 0; i < lTokens.Len(); i++)
		{
			int8 TokenID = lTokens[i].m_iToken;
			m_pWAG2I->MoveGraphBlock(_pContext,0,TokenID,lTokens[i].m_iGraphBlockMoveToken);
			m_pWAG2I->MoveAction(_pContext,0,lTokens[i].m_iToken,lTokens[i].m_iEnterMoveToken);
			// State animoffset
			CWAG2I_Token* pToken = m_pWAG2I->GetTokenFromID(TokenID);
			if (pToken)
			{
				CWAG2I_StateInstance* pState = pToken->GetTokenStateInstance();
				if (pState)
					pState->m_Enter_AnimTimeOffset = lTokens[i].m_StateTime;
			}
		}
	}
	fp32 CurTime = _CurTime;
	// Set properties
	TAP_RCD<int> lPropertiesInt = m_pWAG2I->m_pEvaluator->m_lPropertiesInt;
	TAP_RCD<CPropertyChanges> lChanges = m_lIntChanges;
	int32 Len = Min(lPropertiesInt.Len(),lChanges.Len());
	for (int32 i = 0; i < Len; i++)
	{
		lChanges[i].GetValue(CurTime,lPropertiesInt[i]);
	}

	TAP_RCD<fp32> lPropertiesFloat = m_pWAG2I->m_pEvaluator->m_lPropertiesFloat;
	lChanges = m_lFloatChanges;
	Len = Min(lPropertiesFloat.Len(),lChanges.Len());
	for (int32 i = 0; i < Len; i++)
	{
		lChanges[i].GetValue(CurTime,lPropertiesFloat[i]);
	}

	TAP_RCD<bool> lPropertiesBool = m_pWAG2I->m_pEvaluator->m_lPropertiesBool;
	lChanges = m_lBoolChanges;
	Len = Min(lPropertiesBool.Len(),lChanges.Len());
	for (int32 i = 0; i < Len; i++)
	{
		lChanges[i].GetValue(CurTime,lPropertiesBool[i]);
	}

	// Any impulsechanges?
	CXRAG2_Impulse Impulse;
	int8 iToken = 0;
	int32 iImpulse = 0;
	while (m_ImpulseChanges.IsChange(Impulse, iToken, _CurTime, _LastTime, iImpulse))
		m_pWAG2I->SendImpulse(_pContext,Impulse,iToken);
}

void CXRAG2_PropertyRecorder::Write(CCFile& _File)
{
	// Hrm, write as binary file for now
	TAP_RCD<CTokenInfo> lTokenInfo = m_lTokens;
	int8 NumTokens = lTokenInfo.Len();
	_File.WriteLE(NumTokens);
	for (int32 i = 0; i < lTokenInfo.Len(); i++)
	{
		_File.WriteLE(lTokenInfo[i].m_StateTime);
		//_File.WriteLE(lTokenInfo[i].m_iStartGraphBlock);
		//_File.WriteLE(lTokenInfo[i].m_iStartState);
		_File.WriteLE(lTokenInfo[i].m_iGraphBlockMoveToken);
		_File.WriteLE(lTokenInfo[i].m_iEnterMoveToken);
		_File.WriteLE(lTokenInfo[i].m_iToken);
	}
	m_ImpulseChanges.Write(_File);
	m_PositionChanges.Write(_File);
	m_LowerBodyOffsetChanges.WriteFloat(_File);

	TAP_RCD<CPropertyChanges> lChanges = m_lIntChanges;
	int16 NumChanges = lChanges.Len();
	_File.WriteLE(NumChanges);
	for (int32 i = 0; i < lChanges.Len(); i++)
		lChanges[i].WriteInt(_File);

	lChanges = m_lFloatChanges;
	NumChanges = lChanges.Len();
	_File.WriteLE(NumChanges);
	for (int32 i = 0; i < lChanges.Len(); i++)
		lChanges[i].WriteFloat(_File);

	lChanges = m_lBoolChanges;
	NumChanges = lChanges.Len();
	_File.WriteLE(NumChanges);
	for (int32 i = 0; i < lChanges.Len(); i++)
		lChanges[i].WriteBool(_File);

	lChanges = m_lFunctionChanges;
	NumChanges = lChanges.Len();
	_File.WriteLE(NumChanges);
	for (int32 i = 0; i < lChanges.Len(); i++)
		lChanges[i].WriteInt(_File);
}

void CXRAG2_PropertyRecorder::Read(CCFile& _File)
{
	int8 NumTokens;
	_File.ReadLE(NumTokens);
	m_lTokens.SetLen(NumTokens);
	TAP_RCD<CTokenInfo> lTokenInfo = m_lTokens;
	for (int32 i = 0; i < lTokenInfo.Len(); i++)
	{
		_File.ReadLE(lTokenInfo[i].m_StateTime);
		//_File.ReadLE(lTokenInfo[i].m_iStartGraphBlock);
		//_File.ReadLE(lTokenInfo[i].m_iStartState);
		_File.ReadLE(lTokenInfo[i].m_iGraphBlockMoveToken);
		_File.ReadLE(lTokenInfo[i].m_iEnterMoveToken);
		_File.ReadLE(lTokenInfo[i].m_iToken);
	}
	m_ImpulseChanges.Read(_File);
	m_PositionChanges.Read(_File);
	m_LowerBodyOffsetChanges.ReadFloat(_File);

	int16 NumChanges;
	_File.ReadLE(NumChanges);
	m_lIntChanges.SetLen(NumChanges);
	TAP_RCD<CPropertyChanges> lChanges = m_lIntChanges;
	for (int32 i = 0; i < lChanges.Len(); i++)
		lChanges[i].ReadInt(_File);

	_File.ReadLE(NumChanges);
	m_lFloatChanges.SetLen(NumChanges);
	lChanges = m_lFloatChanges;
	for (int32 i = 0; i < lChanges.Len(); i++)
		lChanges[i].ReadFloat(_File);

	_File.ReadLE(NumChanges);
	m_lBoolChanges.SetLen(NumChanges);
	lChanges = m_lBoolChanges;
	for (int32 i = 0; i < lChanges.Len(); i++)
		lChanges[i].ReadBool(_File);
	
	_File.ReadLE(NumChanges);
	m_lFunctionChanges.SetLen(NumChanges);
	lChanges = m_lFunctionChanges;
	for (int32 i = 0; i < lChanges.Len(); i++)
		lChanges[i].ReadInt(_File);
}

void CXRAG2_PropertyRecorder::CPropertyChanges::GetValue(fp32 _Time, int& _Value)
{
	TAP_RCD<CPropertyChange> lChanges = m_lChanges;
	int32 Len = lChanges.Len() - 1;
	if (Len < 0)
	{
		_Value = 0;
		return;
	}
	for (int32 i = 0; i < Len; i++)
	{
		if (lChanges[i].m_Time <= _Time && lChanges[i+1].m_Time > _Time)
		{
			_Value = lChanges[i].m_IntVal;
			return;
		}
	}
	_Value = lChanges[Len].m_IntVal;
}

void CXRAG2_PropertyRecorder::CPropertyChanges::GetValueLerp(fp32 _Time, fp32& _Value)
{
	TAP_RCD<CPropertyChange> lChanges = m_lChanges;
	int32 Len = lChanges.Len() - 1;
	if (Len < 0)
	{
		_Value = 0;
		return;
	}
	for (int32 i = 0; i < Len; i++)
	{
		if (lChanges[i].m_Time <= _Time && lChanges[i+1].m_Time > _Time)
		{
			fp32 Scale;
			Scale = Min(1.0f,Max(0.0f,(_Time - lChanges[i].m_Time) / (lChanges[i+1].m_Time - lChanges[i].m_Time)));
			_Value = LERP(lChanges[i].m_FloatVal, lChanges[i + 1].m_FloatVal, Scale);
			return;
		}
	}
	_Value = lChanges[Len].m_FloatVal;
}

void CXRAG2_PropertyRecorder::CPropertyChanges::GetValue(fp32 _Time, fp32& _Value)
{
	TAP_RCD<CPropertyChange> lChanges = m_lChanges;
	int32 Len = lChanges.Len() - 1;
	if (Len < 0)
	{
		_Value = 0.0f;
		return;
	}
	for (int32 i = 0; i < Len; i++)
	{
		if (lChanges[i].m_Time <= _Time && lChanges[i+1].m_Time > _Time)
		{
			_Value = lChanges[i].m_FloatVal;
			return;
		}
	}
	_Value = lChanges[Len].m_FloatVal;
}

void CXRAG2_PropertyRecorder::CPropertyChanges::GetValue(fp32 _Time, bool& _Value)
{
	TAP_RCD<CPropertyChange> lChanges = m_lChanges;
	int32 Len = lChanges.Len() - 1;
	if (Len < 0)
	{
		_Value = false;
		return;
	}
	for (int32 i = 0; i < Len; i++)
	{
		if (lChanges[i].m_Time <= _Time && lChanges[i+1].m_Time > _Time)
		{
			_Value = lChanges[i].m_BoolVal;
			return;
		}
	}
	_Value = lChanges[Len].m_BoolVal;
}

void CXRAG2_PropertyRecorder::CPropertyChanges::WriteInt(CCFile& _File)
{
	TAP_RCD<CPropertyChange> lChanges = m_lChanges;
	int16 NumChanges = lChanges.Len();
	_File.WriteLE(NumChanges);
	for (int32 i = 0; i < lChanges.Len(); i++)
	{
		_File.WriteLE(lChanges[i].m_Time);
		_File.WriteLE((int32)lChanges[i].m_IntVal);
	}
}

void CXRAG2_PropertyRecorder::CPropertyChanges::WriteFloat(CCFile& _File)
{
	TAP_RCD<CPropertyChange> lChanges = m_lChanges;
	int16 NumChanges = lChanges.Len();
	_File.WriteLE(NumChanges);
	for (int32 i = 0; i < lChanges.Len(); i++)
	{
		_File.WriteLE(lChanges[i].m_Time);
		_File.WriteLE(lChanges[i].m_FloatVal);
	}
}

void CXRAG2_PropertyRecorder::CPropertyChanges::WriteBool(CCFile& _File)
{
	TAP_RCD<CPropertyChange> lChanges = m_lChanges;
	int16 NumChanges = lChanges.Len();
	_File.WriteLE(NumChanges);
	for (int32 i = 0; i < lChanges.Len(); i++)
	{
		_File.WriteLE(lChanges[i].m_Time);
		_File.WriteLE((int8)lChanges[i].m_BoolVal);
	}
}

void CXRAG2_PropertyRecorder::CPropertyChanges::ReadInt(CCFile& _File)
{
	int16 NumChanges;
	_File.ReadLE(NumChanges);
	m_lChanges.SetLen(NumChanges);
	TAP_RCD<CPropertyChange> lChanges = m_lChanges;
	for (int32 i = 0; i < lChanges.Len(); i++)
	{
		_File.ReadLE(lChanges[i].m_Time);
		int32 Val;
		_File.ReadLE(Val);
		lChanges[i].m_IntVal = (int)Val;
	}
}

void CXRAG2_PropertyRecorder::CPropertyChanges::ReadFloat(CCFile& _File)
{
	int16 NumChanges;
	_File.ReadLE(NumChanges);
	m_lChanges.SetLen(NumChanges);
	TAP_RCD<CPropertyChange> lChanges = m_lChanges;
	for (int32 i = 0; i < lChanges.Len(); i++)
	{
		_File.ReadLE(lChanges[i].m_Time);
		_File.ReadLE(lChanges[i].m_FloatVal);
	}
}

void CXRAG2_PropertyRecorder::CPropertyChanges::ReadBool(CCFile& _File)
{
	int16 NumChanges;
	_File.ReadLE(NumChanges);
	m_lChanges.SetLen(NumChanges);
	TAP_RCD<CPropertyChange> lChanges = m_lChanges;
	for (int32 i = 0; i < lChanges.Len(); i++)
	{
		_File.ReadLE(lChanges[i].m_Time);
		int8 Val;
		_File.ReadLE(Val);
		lChanges[i].m_BoolVal = Val != 0;
	}
}

void CXRAG2_PropertyRecorder::CImpulseChanges::Write(CCFile& _File)
{
	TAP_RCD<CImpulseChange> lChanges = m_lChanges;
	int16 NumChanges = lChanges.Len();
	_File.WriteLE(NumChanges);
	for (int32 i = 0; i < lChanges.Len(); i++)
	{
		_File.WriteLE(lChanges[i].m_Time);
		_File.WriteLE(lChanges[i].m_ImpulseType);
		_File.WriteLE(lChanges[i].m_ImpulseValue);
		_File.WriteLE(lChanges[i].m_iToken);
	}
}

void CXRAG2_PropertyRecorder::CImpulseChanges::Read(CCFile& _File)
{
	int16 NumChanges;
	_File.ReadLE(NumChanges);
	m_lChanges.SetLen(NumChanges);
	TAP_RCD<CImpulseChange> lChanges = m_lChanges;
	for (int32 i = 0; i < lChanges.Len(); i++)
	{
		_File.ReadLE(lChanges[i].m_Time);
		_File.ReadLE(lChanges[i].m_ImpulseType);
		_File.ReadLE(lChanges[i].m_ImpulseValue);
		_File.ReadLE(lChanges[i].m_iToken);
	}
}

void CXRAG2_PropertyRecorder::CImpulseChanges::AddChange(fp32 _Time, CXRAG2_Impulse _Impulse, int8 _iToken)
{
	m_lChanges.Add(CImpulseChange(_Time, _Impulse, _iToken));
}

bool CXRAG2_PropertyRecorder::CImpulseChanges::IsChange(CXRAG2_Impulse& _Impulse, int8& _iToken, fp32 _CurTime, fp32 _LastTime, int32& _iImpulse)
{
	TAP_RCD<const CImpulseChange> lChanges = m_lChanges;

	for (; _iImpulse < lChanges.Len(); _iImpulse++)
	{
		if (lChanges[_iImpulse].m_Time > _LastTime && lChanges[_iImpulse].m_Time <= _CurTime)
		{
			_Impulse.m_ImpulseType = lChanges[_iImpulse].m_ImpulseType;
			_Impulse.m_ImpulseValue = lChanges[_iImpulse].m_ImpulseValue;
			_iToken = lChanges[_iImpulse].m_iToken;
			_iImpulse++;
			return true;
		}
		else if (lChanges[_iImpulse].m_Time > _CurTime)
			return false;
	}
	return false;
}

void CXRAG2_PropertyRecorder::CPositionChanges::AddChange(fp32 _Time, const CMat4Dfp32& _Position)
{
	m_lChanges.Add(CPositionChange(_Time, _Position));
}

void CXRAG2_PropertyRecorder::CPositionChanges::Write(CCFile& _File)
{
	m_InvStartPosition.Write(&_File);
	TAP_RCD<CPositionChange> lChanges = m_lChanges;
	int16 NumChanges = lChanges.Len();
	_File.WriteLE(NumChanges);
	for (int32 i = 0; i < lChanges.Len(); i++)
	{
		_File.WriteLE(lChanges[i].m_Time);
		lChanges[i].m_Position.Write(&_File);
	}
}

void CXRAG2_PropertyRecorder::CPositionChanges::Read(CCFile& _File)
{
	m_InvStartPosition.Read(&_File);
	int16 NumChanges;
	_File.ReadLE(NumChanges);
	m_lChanges.SetLen(NumChanges);
	TAP_RCD<CPositionChange> lChanges = m_lChanges;
	for (int32 i = 0; i < lChanges.Len(); i++)
	{
		_File.ReadLE(lChanges[i].m_Time);
		lChanges[i].m_Position.Read(&_File);
	}
}

CMat4Dfp32 CXRAG2_PropertyRecorder::CPositionChanges::GetPosition(fp32 _Time, bool _bFromOrg)
{
	// Get 2 positions and lerp them
	CMat4Dfp32 MatResult;
	TAP_RCD<CPositionChange> lChanges = m_lChanges;
	int32 Len = lChanges.Len() - 1;
	if (Len < 0)
	{
		MatResult.Unit();
	}
	else
	{
		// Find 2 positions and lerp them
		bool bOk = true;
		for (int32 i = 0; i < Len; i++)
		{
			if (lChanges[i].m_Time <= _Time && lChanges[i+1].m_Time > _Time)
			{
				CMat4Dfp32 Mat1;
				CMat4Dfp32 Mat2;
				fp32 Scale;
				Mat1 = lChanges[i].m_Position;
				Mat2 = lChanges[i+1].m_Position;
				Scale = Min(1.0f,Max(0.0f,(_Time - lChanges[i].m_Time) / (lChanges[i+1].m_Time - lChanges[i].m_Time)));
				bOk = true;
				Interpolate2(Mat1, Mat2, MatResult, Scale);
				break;
			}
		}
		if (!bOk)
		{
			// Use the last position
			MatResult = GetLatest();
		}
	}
	if (!_bFromOrg)
	{
		CMat4Dfp32 Temp = MatResult;
		Temp.Multiply(m_InvStartPosition,MatResult);
	}
	return MatResult;
}
#endif
