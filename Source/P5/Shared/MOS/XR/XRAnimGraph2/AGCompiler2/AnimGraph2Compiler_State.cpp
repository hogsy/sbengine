#include "PCH.h"

//--------------------------------------------------------------------------------

#include "../AnimGraph2Defs.h"
#include "AnimGraph2Compiler.h"

//--------------------------------------------------------------------------------

#define AG2C_SCOPESEPARATOR " . "

//--------------------------------------------------------------------------------

#define AG2C_CONDITION_SORTEDBIT		(1<<15)

extern const char* ms_lpAG2CAnimTimeOffsetTypes[];

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
bool CXRAG2C_State_Base::ParseMoveTokenReg(const CRegistry* _pMoveTokenReg, CStr _Location, CXRAG2C_MoveToken& _MoveToken)
{
	int TokenID = AG2_TOKENID_NULL;
	if (_pMoveTokenReg->GetThisValue() != "")
	{
		if (!m_pAGCompiler->EvalConstantExpressionInt(_pMoveTokenReg->GetThisValue(), TokenID, NULL, _Location))
			return false;
	}
	_MoveToken.m_TokenID = TokenID;

	for (int iChild = 0; iChild < _pMoveTokenReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pMoveTokenReg->GetChild(iChild);

		if (pChildReg->GetThisName() == "TARGETSTATE")
		{
			_MoveToken.m_TargetState = pChildReg->GetThisValue().UpperCase();
		}
		else if (pChildReg->GetThisName() == "TARGETGRAPHBLOCK")
		{
			_MoveToken.m_TargetGraphBlock = pChildReg->GetThisValue().UpperCase();
		}
		else if (pChildReg->GetThisName() == "ANIMBLENDDURATION")
		{
			if (!m_pAGCompiler->EvalConstantExpressionFloat(pChildReg->GetThisValue(), _MoveToken.m_AnimBlendDuration, NULL, _Location))
				return false;
		}
		else if (pChildReg->GetThisName() == "ANIMBLENDDELAY")
		{
			if (!m_pAGCompiler->EvalConstantExpressionFloat(pChildReg->GetThisValue(), _MoveToken.m_AnimBlendDelay, NULL, _Location))
				return false;
		}
		else if (pChildReg->GetThisName() == "ANIMTIMEOFFSET")
		{
			if (!m_pAGCompiler->EvalConstantExpressionFloat(pChildReg->GetThisValue(), _MoveToken.m_AnimTimeOffset, NULL, _Location))
				return false;
		}
		else 
			m_pAGCompiler->OnError(CStrF("Unknown keyword '%s' (%s)", pChildReg->GetThisName().GetStr(), (char*)_Location));
	}

	return true;
}

bool CXRAG2C_State_Base::ParseActionEffectsReg(const CRegistry* _pEffectsReg, int& _iBaseEffect, int& _nEffects, CStr _Location)
{
	if (_pEffectsReg == NULL)
		return false;

	_iBaseEffect = m_lEffectInstances.Len();

	for (int iChild = 0; iChild < _pEffectsReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pEffectReg = _pEffectsReg->GetChild(iChild);
		CStr EffectStr = pEffectReg->GetThisName() + " " + pEffectReg->GetThisValue();
		int iLeftPar = EffectStr.Find("(");

		CStr Name;
		if (iLeftPar != -1)
		{
			Name = EffectStr.Left(iLeftPar);
			EffectStr = EffectStr.Del(0, Name.Len());
		}
		else
		{
			Name = EffectStr;
			EffectStr = "";
		}

		Name.Trim();
		EffectStr.Trim();

		int32 ID = m_pAGCompiler->GetEffectID(Name);
		if (ID == -1)
		{
			m_pAGCompiler->OnError(CStrF("Undefined effect '%s' (%s).", (char*)Name, (char*)_Location));
			m_pAGCompiler->OnNote(CStr("Syntax is 'Effect(Param0, Param1, ..., ParamN)'."));
			return false;
		}

		/*
		{
		m_pCompiler->OnWarning(CStrF("Probably invalid effect syntax '%s %s' (%s).", (char*)Name, (char*)pEffectReg->GetThisValue(), (char*)_Location));
		m_pCompiler->OnNote(CStr("Syntax is 'Effect(Param0, Param1, ..., ParamN)'."));
		}
		*/

		if ((EffectStr.Left(1) == "(") && (EffectStr.Right(1) == ")"))
		{
			EffectStr = EffectStr.Del(0, 1);
			EffectStr = EffectStr.Del(EffectStr.Len() - 1, 1);
		}
		else
		{
			m_pAGCompiler->OnError(CStrF("Missing parantheses enclosing parameters '%s' (%s).", (char*)pEffectReg->GetThisValue(), (char*)_Location));
			m_pAGCompiler->OnNote(CStr("Syntax is 'Effect(Param0, Param1, ..., ParamN)'."));
			return false;
		}

		CXRAG2C_EffectInstance EffectInstance(ID);

		if (EffectStr != "")
		{
			// Resolve constantnames here, since CXRAG2_CallbackParams should not have access to the compiler.
			CStr ParamsStrResolved;
			{
				CStr ParamsStr = EffectStr;
//				int iParam = 0;
				while (ParamsStr != "")
				{
					CStr ParamStr = ParamsStr.GetStrSep(","); ParamStr.Trim();
					if (ParamStr != "")
					{
						int ParamValue;
						if (!m_pAGCompiler->EvalConstantExpressionInt(ParamStr, ParamValue, NULL, _Location))
							return false;

						if (ParamsStrResolved != "")
							ParamsStrResolved += "," + CStrF("%d", ParamValue);
						else
							ParamsStrResolved = CStrF("%d", ParamValue);
					}
				}
			}

			if (!m_pAGCompiler->ParseParamsStr(ParamsStrResolved, EffectInstance.m_lParams, _Location))
				return false;
		}

		m_lEffectInstances.Add(EffectInstance);
		_nEffects++;
	}			

	return true;
}

bool CXRAG2C_State::ExpandInheritance()
{
	m_bPendingExpandInheritance = true;

	for (int iSuperState = 0; iSuperState < m_lSuperStates.Len(); iSuperState++)
	{
		CFStr SuperStateName = m_lSuperStates[iSuperState];
		bool bSuperStateFound = false;
		for (int iState = 0; iState < m_pGraphBlock->m_lspStates.Len(); iState++)
		{
			if (SuperStateName == (m_pGraphBlock->m_lspStates[iState]->m_Name))
			{
				if (!Inherit(*(m_pGraphBlock->m_lspStates[iState])))
					return false;
				bSuperStateFound = true;
				break;
			}
		}
		if (!bSuperStateFound)
		{
			m_pAGCompiler->OnError(CStrF("Undefined state '%s' inherited by state '%s'.", (char*)SuperStateName, (char*)m_Name));
			return false;
		}
	}

	m_bPendingExpandInheritance = false;
	m_bExpandedInheritance = true;

	return true;
}

//--------------------------------------------------------------------------------

#define INHERIT(Flag)	(((m_InheritFlags & Flag) != 0))
//&& ((_SuperState.m_InheritFlags & Flag) == 0))

//--------------------------------------------------------------------------------

// Assumes _SuperState is already fully expanded.
bool CXRAG2C_State::Inherit(CXRAG2C_State& _SuperState)
{
	if (_SuperState.m_bPendingExpandInheritance)
	{
		m_pAGCompiler->OnError("Cyclic inheritance between state '" + m_Name + "' and state '" + _SuperState.m_Name + "'.");
		return false;
	}

	if (!_SuperState.m_bExpandedInheritance)
		if (!_SuperState.ExpandInheritance())
			return false;

	if (INHERIT(AG2C_INHERITFLAGS_FLAGS0))
	{
		m_Flags[0] |= _SuperState.m_Flags[0] & _SuperState.m_FlagsMask[0];
		m_FlagsMask[0] |= _SuperState.m_FlagsMask[0];
	}

	if (INHERIT(AG2C_INHERITFLAGS_FLAGS1))
	{
		m_Flags[1] |= _SuperState.m_Flags[1] & _SuperState.m_FlagsMask[1];
		m_FlagsMask[1] |= _SuperState.m_FlagsMask[1];
	}

	if (INHERIT(AG2C_INHERITFLAGS_PRIORITY))
		m_Priority = _SuperState.m_Priority;

	// Add inherit anims
	//if (INHERIT(AG2C_INHERITFLAGS_ANIMINDEX))
	{
		int32 Len = _SuperState.m_lStateAnims.Len();
		int32 StateLen = m_lStateAnims.Len();
		//m_lStateAnims.SetLen(Len);
		for (int32 i = 0; i < Len; i++)
		{
			// Check if we have this anim already
			bool bOK = true;
			if (_SuperState.m_lStateAnims[i].m_iAnim == -1)
				continue;
			for (int32 j = 0; j < StateLen; j++)
			{
				if (m_lStateAnims[j].m_iAnim == _SuperState.m_lStateAnims[i].m_iAnim)
				{
					bOK = false;
					break;
				}
			}
			if (bOK)
			{
				m_pAGCompiler->OnWarning(CStrF("Warning: %s Inheriting animation: %d (%s)",m_Name.Str(), _SuperState.m_lStateAnims[i].m_iAnim,_SuperState.m_lStateAnims[i].m_Name.Str()).Str());
				m_lStateAnims.Add(_SuperState.m_lStateAnims[i]);//.m_iAnim;
				// Make sure the anim is referenced
				m_pAGCompiler->m_lAnimations[_SuperState.m_lStateAnims[i].m_iAnim].m_nReferences++;
			}
		}
	}
	/*if (INHERIT(AG2C_INHERITFLAGS_ANIMBASEJOINT))
	{
	int32 Len = _SuperState.m_lStateAnims.Len();
	m_lStateAnims.SetLen(Len);
	for (int32 i = 0; i < Len; i++)
	m_lStateAnims[i].m_iBaseJoint = _SuperState.m_lStateAnims[i].m_iBaseJoint;
	}
	if (INHERIT(AG2C_INHERITFLAGS_ANIMTIMEOFFSET))
	{
	int32 Len = _SuperState.m_lStateAnims.Len();
	m_lStateAnims.SetLen(Len);
	for (int32 i = 0; i < Len; i++)
	m_lStateAnims[i].m_TimeOffset = _SuperState.m_lStateAnims[i].m_TimeOffset;
	}*/
	/*
	if (INHERIT(AG2C_INHERITFLAGS_ANIMTIMEOFFSETTYPE))
	m_StateAnim.m_TimeOffset = _SuperState.m_StateAnim.m_TimeOffsetType;
	*/
	/*if (INHERIT(AG2C_INHERITFLAGS_ANIMTIMESCALE))
	{
	int32 Len = _SuperState.m_lStateAnims.Len();
	m_lStateAnims.SetLen(Len);
	for (int32 i = 0; i < Len; i++)
	m_lStateAnims[i].m_TimeScale = _SuperState.m_lStateAnims[i].m_TimeScale;
	}
	if (INHERIT(AG2C_INHERITFLAGS_ANIMOPACITY))
	{
	int32 Len = _SuperState.m_lStateAnims.Len();
	m_lStateAnims.SetLen(Len);
	for (int32 i = 0; i < Len; i++)
	m_lStateAnims[i].m_Opacity = _SuperState.m_lStateAnims[i].m_Opacity;
	}*/

	m_InheritDEWarnings |= _SuperState.m_InheritDEWarnings;
	m_DisabledWarnings |= _SuperState.m_DisabledWarnings & m_InheritDEWarnings;
	m_EnabledWarnings |= _SuperState.m_EnabledWarnings & m_InheritDEWarnings;

	// Inherit StateConstants.
	for (int jConstant = 0; jConstant < _SuperState.m_lConstants.Len(); jConstant++)
	{
		bool bOverloaded = false;
		for (int iConstant = 0; iConstant < m_lConstants.Len(); iConstant++)
		{
			int32 SuperConstantID = _SuperState.m_lConstants[jConstant].m_ID;
			int ConstantID = m_lConstants[iConstant].m_ID;
			if (SuperConstantID == ConstantID)
				bOverloaded = true;
		}
		if (!bOverloaded)
			m_lConstants.Add(_SuperState.m_lConstants[jConstant]);
	}

	int iCondition;
	int nOwnEffectInstances = m_lEffectInstances.Len();
	int nOwnConditions = m_lConditions.Len();
	int nOwnActions = m_lActions.Len();
	//	int nSuperConditions = _SuperState.m_lConditions.Len();
	//	int nSuperActions = _SuperState.m_lActions.Len();

	// Copy EffectInstances.
	for (int jEffectInstance = 0; jEffectInstance < _SuperState.m_lEffectInstances.Len(); jEffectInstance++)
		m_lEffectInstances.Add(_SuperState.m_lEffectInstances[jEffectInstance]);

	// Copy conditions
	for (iCondition = 0; iCondition < _SuperState.m_lConditions.Len(); iCondition++)
	{
		CXRAG2C_Condition Condition = _SuperState.m_lConditions[iCondition];
		Condition.m_SortedIndex = 0;
		Condition.m_iFirstAction += nOwnActions;
		m_lConditions.Add(Condition);
	}

	// Copy actions and adjust conditionmap indices and effect base indices.
	for (int jAction = 0; jAction < _SuperState.m_lActions.Len(); jAction++)
	{
		int iAction = nOwnActions + jAction;
		CXRAG2C_Action InheritedAction = _SuperState.m_lActions[jAction]; // We have to copy safely here, since Add() merely will memcpy the element's content.
		m_lActions.Add(InheritedAction);

		int ConditionMapOffset = nOwnConditions;
		int ConditionMapLen = m_lActions[iAction].m_lConditionMap.Len();
		for (int iConditionMapping = 0; iConditionMapping < ConditionMapLen; iConditionMapping++)
			m_lActions[iAction].m_lConditionMap[iConditionMapping] += ConditionMapOffset;

		m_lActions[iAction].m_iBaseEffectInstance += nOwnEffectInstances;
	}
	/*
	for (iCondition = 0; iCondition < (m_lConditions.Len() - 1); iCondition++)
	{
	CXRAG2C_Condition Condition = m_lConditions[iCondition];
	}

	for (iAction = 0; iAction < m_lActions.Len(); iAction++)
	{
	for (int iConditionMapping = 0; iConditionMapping < m_lActions[iAction].m_lConditionMap.Len(); iConditionMapping++)
	{
	int iCondition = m_lActions[iAction].m_lConditionMap[iConditionMapping];
	}
	}
	*/
	// Merge identical conditions and redirect action conditionmap indices.
	for (iCondition = 0; iCondition < (m_lConditions.Len() - 1); iCondition++)
	{
		for (int jCondition = (iCondition + 1); jCondition < m_lConditions.Len(); jCondition++)
		{
			/*
			CXRAG2C_Condition ConditionA = m_lConditions[iCondition];
			CXRAG2C_Condition ConditionB = m_lConditions[jCondition];
			*/
			if (m_lConditions[iCondition] == m_lConditions[jCondition])
			{
				// Merge and remove condition jCondition.
				m_lConditions[iCondition].m_Occurrences += m_lConditions[jCondition].m_Occurrences;

				for (int iAction = 0; iAction < m_lActions.Len(); iAction++)
				{
					for (int iConditionMapping = 0; iConditionMapping < m_lActions[iAction].m_lConditionMap.Len(); iConditionMapping++)
					{
						if (m_lActions[iAction].m_lConditionMap[iConditionMapping] == jCondition)
							m_lActions[iAction].m_lConditionMap[iConditionMapping] = iCondition;

						if (m_lActions[iAction].m_lConditionMap[iConditionMapping] > jCondition)
							m_lActions[iAction].m_lConditionMap[iConditionMapping]--;
					}
				}

				m_lConditions.Del(jCondition);
				jCondition--;
			}
		}
	}
	/*
	for (iCondition = 0; iCondition < (m_lConditions.Len() - 1); iCondition++)
	CXRAG2C_Condition Condition = m_lConditions[iCondition];

	for (iAction = 0; iAction < m_lActions.Len(); iAction++)
	for (int iConditionMapping = 0; iConditionMapping < m_lActions[iAction].m_lConditionMap.Len(); iConditionMapping++)
	int iCondition = m_lActions[iAction].m_lConditionMap[iConditionMapping];
	*/
	return true;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_State::ParseReg(CXRAG2C_GraphBlock* _pGraphBlock, const CRegistry* _pStateReg)
{
	if (_pGraphBlock == NULL)
		return false;

	if (_pStateReg == NULL)
		return false;

	m_pGraphBlock = _pGraphBlock;
	m_pAGCompiler = _pGraphBlock->m_pCompiler;

	m_Name = _pStateReg->GetThisName();

	CStr Location = m_Name;

	m_InheritFlags = AG2C_INHERITFLAGS_ALL;
	m_bPendingExpandInheritance = false;
	m_bExpandedInheritance = false;

	m_DisabledWarnings = 0;
	m_EnabledWarnings = 0;
	m_InheritDEWarnings = 0;

	m_Priority = 0;
	m_Flags[0] = 0;
	m_FlagsMask[0] = 0;
	m_Flags[1] = 0;
	m_FlagsMask[1] = 0;

	m_References = 0;


	for (int iChild = 0; iChild < _pStateReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pStateReg->GetChild(iChild);

		if (pChildReg->GetThisName() == "FLAGSLO")
		{
			m_InheritFlags &= ~AG2C_INHERITFLAGS_FLAGS0;
			if (!m_pAGCompiler->ParseStateFlags(pChildReg->GetThisValue(), m_Flags[0], m_FlagsMask[0], Location))
				return false;
		}
		else if (pChildReg->GetThisName() == "FLAGSHI")
		{
			m_InheritFlags &= ~AG2C_INHERITFLAGS_FLAGS1;
			if (!m_pAGCompiler->ParseStateFlags(pChildReg->GetThisValue(), m_Flags[1], m_FlagsMask[1], Location))
				return false;
		}
		else if (pChildReg->GetThisName() == "PRIORITY")
		{
			m_InheritFlags &= ~AG2C_INHERITFLAGS_PRIORITY;
			fp32 Priority;
			if (!m_pAGCompiler->EvalConstantExpressionFloat(pChildReg->GetThisValue(), Priority, this, Location))
				return false;
			m_Priority = (uint8)Priority;
		}
		else if (pChildReg->GetThisName() == "INHERIT")
		{
			if (!ParseInheritReg(pChildReg))
				return false;
		}
		else if (pChildReg->GetThisName() == "ANIMATION")
		{
			if (!ParseAnimationReg(pChildReg))
				return false;
		}
		else if (pChildReg->GetThisName() == "CONSTANTS")
		{
			if (!ParseStateConstantsReg(pChildReg))
				return false;
		}
		else if (pChildReg->GetThisName() == "ACTIONS")
		{
			if (!ParseActionsReg(pChildReg))
				return false;
		}
		else if (pChildReg->GetThisName() == "DISABLEWARNINGS")
		{
			const char* lpWarningStr[] = { "MISSINGANIMATION", "MISSINGACTIONS", NULL };
			m_DisabledWarnings = pChildReg->GetThisValue().TranslateFlags(lpWarningStr);
		}
		else if (pChildReg->GetThisName() == "ENABLEWARNINGS")
		{
			const char* lpWarningStr[] = { "MISSINGANIMATION", "MISSINGACTIONS", NULL };
			m_EnabledWarnings = pChildReg->GetThisValue().TranslateFlags(lpWarningStr);
		}
		else if (pChildReg->GetThisName() == "INHERITWARNINGS")
		{
			const char* lpWarningStr[] = { "MISSINGANIMATION", "MISSINGACTIONS", NULL };
			m_InheritDEWarnings = pChildReg->GetThisValue().TranslateFlags(lpWarningStr);
		}
		else if (pChildReg->GetThisName() == "EDITPROPERTIES")
		{
			// Ignore editor-only keys.
		}
		else
		{
			m_pAGCompiler->OnWarning(CStrF("Unrecognized keyword '%s' (%s).", (char*)pChildReg->GetThisName(), (char*)Location));
		}
	}

	//	Process();

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_State::ParseInheritReg(const CRegistry* _pInheritReg)
{
	if (_pInheritReg == NULL)
		return false;

	if (_pInheritReg->GetThisValue() != "")
	{
		CStr SuperStates = _pInheritReg->GetThisValue().UpperCase();
		CStr SuperState;
		do
		{
			SuperState = SuperStates.GetStrSep("+");
			SuperState.Trim();
			m_lSuperStates.Add(SuperState);
		}
		while (SuperStates != "");
	}
	else if (_pInheritReg->GetNumChildren() == 0)
	{
		m_pAGCompiler->OnWarning("Missing inheritance specification.");
	}
	else
	{
		for (int iChild = 0; iChild < _pInheritReg->GetNumChildren(); iChild++)
		{
			const CRegistry* pChildReg = _pInheritReg->GetChild(iChild);
			m_lSuperStates.Add(pChildReg->GetThisName().UpperCase());
		}
	}

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_State::ParseAnimationReg(const CRegistry* _pAnimationReg)
{
	if (_pAnimationReg == NULL)
		return false;

	CStr Location = m_Name;

	CXRAG2C_StateAnim TempAnim;
	TempAnim.m_Name = _pAnimationReg->GetThisValue();

	if (_pAnimationReg->GetThisValue() != "")
	{
		m_InheritFlags &= ~AG2C_INHERITFLAGS_ANIMINDEX;
		// Get animation index
		if (!m_pAGCompiler->GetAnimIndex(_pAnimationReg->GetThisValue().UpperCase(), TempAnim.m_iAnim, this, Location))
		{
			m_pAGCompiler->OnError(CStrF("%s Animation: %s not found",Location.Str(),_pAnimationReg->GetThisValue().Str()));
			return false;
		}
	}

	for (int iChild = 0; iChild < _pAnimationReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pAnimationReg->GetChild(iChild);

		if (pChildReg->GetThisName() == "BASEJOINT")
		{
			m_InheritFlags &= ~AG2C_INHERITFLAGS_ANIMBASEJOINT;
			TempAnim.m_iBaseJoint = pChildReg->GetThisValuei();
		}
		/*
		else if (pChildReg->GetThisName() == "TIMEOFFSETTYPE")
		{
		m_InheritFlags &= ~AG2C_INHERITFLAGS_ANIMTIMEOFFSETTYPE;
		m_StateAnim.m_TimeOffsetType = pChildReg->GetThisValue().TranslateInt(ms_lpAG2CAnimTimeOffsetTypes);
		}
		*/
		else if (pChildReg->GetThisName() == "TIMEOFFSET")
		{
			m_InheritFlags &= ~AG2C_INHERITFLAGS_ANIMTIMEOFFSET;
			if (!m_pAGCompiler->EvalConstantExpressionFloat(pChildReg->GetThisValue(), TempAnim.m_TimeOffset, this, Location))
				return false;
		}
		else if (pChildReg->GetThisName() == "TIMESCALE")
		{
			m_InheritFlags &= ~AG2C_INHERITFLAGS_ANIMTIMESCALE;
			if (!m_pAGCompiler->EvalConstantExpressionFloat(pChildReg->GetThisValue(), TempAnim.m_TimeScale, this, Location))
				return false;
			if (TempAnim.m_TimeScale == 0.0f)
				TempAnim.m_TimeScale = _FP32_MIN;
		}
		else if (pChildReg->GetThisName() == "OPACITY")
		{
			m_InheritFlags &= ~AG2C_INHERITFLAGS_ANIMOPACITY;
			if (!m_pAGCompiler->EvalConstantExpressionFloat(pChildReg->GetThisValue(), TempAnim.m_Opacity, this, Location))
				return false;
		}
		else if (pChildReg->GetThisName() == "ANIMFLAGS")
		{
			m_InheritFlags &= ~AG2C_INHERITFLAGS_ANIMFLAGS;
			uint32 FlagMask = 0;
			if (!m_pAGCompiler->ParseAnimFlags(pChildReg->GetThisValue(), TempAnim.m_AnimFlags, FlagMask, Location))
				return false;
		}
		else if (pChildReg->GetThisName() == "MERGEOPERATOR")
		{
			m_InheritFlags &= ~AG2C_INHERITFLAGS_ANIMFLAGS;
			int32 iProperty = m_pAGCompiler->GetPropertyIntID(pChildReg->GetThisValue().UpperCase());
			if (iProperty == -1)
			{
				m_pAGCompiler->OnError(CStrF("%s MergeOperatorerror: %s %d",Location.Str(),pChildReg->GetThisValue().Str(),iProperty));
				return false;
			}
			TempAnim.m_iMergeOperator = (uint8)iProperty;
		}
		else if (pChildReg->GetThisName() == "TIMECONTROL")
		{
			m_InheritFlags &= ~AG2C_INHERITFLAGS_ANIMFLAGS;
			int iProperty = m_pAGCompiler->GetPropertyFloatID(pChildReg->GetThisValue().UpperCase());
			if (iProperty == -1)
			{
				// Check int constants as well..
				if (!m_pAGCompiler->EvalConstantExpressionInt(pChildReg->GetThisValue(), iProperty, this, Location))
				{
					m_pAGCompiler->OnError(CStrF("%s TimeControlError %s %d",Location.Str(),pChildReg->GetThisValue().Str(),iProperty));
					return false;
				}
				//return false;
			}
			TempAnim.m_iTimeControlProperty = (uint8)iProperty;
		}
		else
		{
			m_pAGCompiler->OnWarning(CStrF("Unrecognized keyword '%s' (%s).", (char*)pChildReg->GetThisName(), (char*)Location));
		}
	}

	// Add animation
	m_lStateAnims.Add(TempAnim);

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_State::ParseStateConstantsReg(const CRegistry* _pStateConstantsReg)
{
	if (_pStateConstantsReg == NULL)
		return false;

	CStr Location = "(" + m_Name + AG2C_SCOPESEPARATOR + "CONSTANTS)";

	for (int iChild = 0; iChild < _pStateConstantsReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pStateConstantsReg->GetChild(iChild);
		CStr SCName = pChildReg->GetThisName();

		uint32 SCID = m_pAGCompiler->GetStateConstantID(SCName);
		if (SCID == -1)
		{
			m_pAGCompiler->OnError(CStrF("Undefined stateconstant '%s' (%s).", (char*)SCName, (char*)Location));
			return false;
		}

		fp32 SCValue;
		if (!m_pAGCompiler->EvalConstantExpressionFloat(pChildReg->GetThisValue(), SCValue, this, Location))
			return false;

		CXRAG2C_StateConstant StateConstant(SCName, SCID, SCValue);
		m_lConstants.Add(StateConstant);
	}
/*
	for (int iConstant = 0; iConstant < m_lConstants.Len(); iConstant++)
	{
		int32* pConstantID = &(m_lConstants[iConstant].m_ID);
		fp32* pConstantValue = &(m_lConstants[iConstant].m_Value);
		int Apa = 5;
	}
*/
	return true;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_State::ParseActionsReg(const CRegistry* _pActionsReg)
{
	if (_pActionsReg == NULL)
		return false;

	// FIXME: Check for unique and continously ordered ACTION# keywords.
	for (int iChild = 0; iChild < _pActionsReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pActionsReg->GetChild(iChild);
		if (!ParseActionReg(pChildReg))
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_State::ParseActionReg(const CRegistry* _pActionReg)
{
	if (_pActionReg == NULL)
		return false;

	if (_pActionReg->GetThisName().Left(6) != "ACTION")
	{
		m_pAGCompiler->OnError("Unrecognized keyword '" + _pActionReg->GetThisName() + "' (" + m_Name + AG2C_SCOPESEPARATOR + "ACTIONS).");
		return false;
	}

	CStr ActionName = _pActionReg->GetThisName();
	CStr Location = m_Name + AG2C_SCOPESEPARATOR + ActionName;

	CXRAG2C_Action NewAction;

	bool bConditionSetFound = false;

	for (int iChild = 0; iChild < _pActionReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pActionReg->GetChild(iChild);

		if (pChildReg->GetThisName() == "EXPORT")
		{
			NewAction.m_bExported = true;
		}
		else if (pChildReg->GetThisName() == "MOVETOKEN")
		{
			CXRAG2C_MoveToken MoveToken;
			if (!ParseMoveTokenReg(pChildReg, Location, MoveToken))//MoveToken.m_TokenID, MoveToken.m_TargetState, MoveToken.m_AnimBlendDuration, MoveToken.m_AnimBlendDelay, MoveToken.m_AnimTimeOffset, MoveToken.m_iAnimTimeOffsetType))
				return false;
			NewAction.m_lMoveTokens.Add(MoveToken);
		}
		else if (pChildReg->GetThisName() == "EFFECTS")
		{
			if (!ParseActionEffectsReg(pChildReg, NewAction.m_iBaseEffectInstance, NewAction.m_nEffectInstances, Location))
				return false;
		}
		else if (pChildReg->GetThisName().Left(12) == "CONDITIONSET")
		{
			if ((NewAction.m_lMoveTokens.Len() == 0) && (NewAction.m_nEffectInstances == 0))
			{
				m_pAGCompiler->OnError("Missing targetstate or effects (" + Location + ").");
				m_pAGCompiler->OnNote("MoveActions must be declared above all ConditionSets.");
				return false;
			}

			bConditionSetFound = true;
			if (!ParseConditionSetReg(pChildReg, Location, ActionName, NewAction))//bExported, TokenID, TargetState, iBaseEffect, nEffects, AnimBlendDuration, AnimBlendDelay, AnimTimeOffset, iAnimTimeOffsetType))
				return false;
		}
		else if (pChildReg->GetThisName() == "EDITPROPERTIES")
		{
			// Ignore editor-only keys.
		}
		else
		{
			m_pAGCompiler->OnWarning(CStrF("Unrecognized keyword '%s' (%s).", (char*)pChildReg->GetThisName(), (char*)Location));
		}
	}

	if (!bConditionSetFound)
	{
		m_pAGCompiler->OnError("Missing conditions (" + Location + ").");
		return false;
	}
	/*
	if ((TargetState == "") && (nEffects == 0))
	{
	m_pCompiler->OnError("Missing targetstate or effects (" + Location + ").");
	return false;
	}
	*/
	return true;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_State::ParseConditionSetReg(const CRegistry* _pActionReg, CStr _Location, CStr _ActionName, CXRAG2C_Action& _Action)
{
	if (_pActionReg == NULL)
		return false;

	//CStr Location = _Location + AG2C_SCOPESEPARATOR + _ActionName;
	CStr Location = _Location;

	if (_pActionReg->GetNumChildren() == 0)
	{
		m_pAGCompiler->OnError("Missing conditions (" + Location + ").");
		return false;
	}

	_Action.m_Name = _ActionName;
	/*CXRAG2C_Action Action;
	Action.m_TokenID = AG2_TOKENID_NULL;
	Action.m_iTargetState = AG2_STATEINDEX_NULL;
	Action.m_bExported = _bExported;
	Action.m_TokenID = _TokenID;
	Action.m_TargetState = _TargetState;
	Action.m_iBaseEffectInstance = _iBaseEffectInstance;
	Action.m_nEffectInstances = _nEffectInstances;
	Action.m_AnimBlendDuration = _AnimBlendDuration;
	Action.m_AnimBlendDelay = _AnimBlendDelay;
	Action.m_AnimTimeOffset = _AnimTimeOffset;
	Action.m_iAnimTimeOffsetType = _iAnimTimeOffsetType;*/

	int iAction = AddAction(_Action);

	for (int iChild = 0; iChild < _pActionReg->GetNumChildren(); iChild++)
	{
//		int ConditionMapLen = m_lActions[iAction].m_lConditionMap.Len();
		const CRegistry* pChildReg = _pActionReg->GetChild(iChild);
		int iCondition = ParseConditionReg(pChildReg, iAction, Location);
		if (iCondition != -1)
			m_lActions[iAction].m_lConditionMap.Add(iCondition);
		else
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------------

int32 CXRAG2C_State::AddAction(CXRAG2C_Action& _Action)
{
	int32 iAction = m_lActions.Len();
	m_lActions.Add(_Action);
	return iAction;
}

//--------------------------------------------------------------------------------

int CXRAG2C_State::ParseConditionReg(const CRegistry* _pConditionReg, int _iAction, CStr _Location)
{
	if (_pConditionReg == NULL)
		return -1;

	CStr ConditionStr = _pConditionReg->GetThisName();
	ConditionStr.Trim();

	CStr PropertyStr;
	int iLeftPar = ConditionStr.Find("(");
	int iSpace = ConditionStr.Find(" ");
	if ((iLeftPar >= 0) && (iSpace > iLeftPar))
	{
		int iRightPar = ConditionStr.Find(")");
		if (iRightPar < 0)
		{
			m_pAGCompiler->OnError(CStrF("Missing right paranthesis in condition '%s' (%s).", (char*)ConditionStr, (char*)_Location));
			return -1;
		}

		PropertyStr = ConditionStr.GetStrSep(")");
		PropertyStr += ")";
	}
	else
	{
		PropertyStr = ConditionStr.GetStrSep(" ");
	}

	ConditionStr.Trim();
	CStr OperatorStr = ConditionStr.GetStrSep(" ");
	ConditionStr.Trim();
	CStr ConstantStr = ConditionStr;
	ConditionStr.Trim();

	if ((PropertyStr == "") || ((OperatorStr != "") && (ConstantStr == "")))
	{
		CStr ConditionStr = _pConditionReg->GetThisName();
		m_pAGCompiler->OnError(CStrF("Invalid syntax in condition '%s' (%s).", (char*)ConditionStr, (char*)_Location));
		m_pAGCompiler->OnNote(CStr("Condition syntax is '[!]Property' or 'Property Operator Constant'."));
		return -1;
	}

	// Interpret condition as "implicit true/not-true".
	if ((OperatorStr == "") && (ConstantStr == ""))
	{
		if (PropertyStr[0] == '!')
		{
			OperatorStr = "=";
			ConstantStr = "0";
			PropertyStr = PropertyStr.RightFrom(1);
		}
		else
		{
			OperatorStr = "!=";
			ConstantStr = "0";
		}
	}

	CStr PropertyNameStr = PropertyStr.GetStrSep("(");
	CStr PropertyParamsStr = PropertyStr.GetStrSep(")");
	int32 PropertyType = -1;
	if (PropertyParamsStr.Len() > 0)
		m_pAGCompiler->OnError(CStrF("Parameters to a property not allowed '%s' (%s)", PropertyNameStr.Str(), _Location.Str()));
	// Find type of property	
	int32 iProperty = -1;
	if (!m_pAGCompiler->GetPropertyID(PropertyNameStr,iProperty,PropertyType))
	{
		m_pAGCompiler->OnError(CStrF("Undefined Property '%s' (%s).", (char*)PropertyNameStr, (char*)_Location));
		return -1;
	}

	int iOperator = m_pAGCompiler->GetOperatorID(OperatorStr);
	if (iOperator == -1)
	{
		m_pAGCompiler->OnError(CStrF("Undefined operator '%s' (%s).", (char*)OperatorStr, (char*)_Location));
		return -1;
	}

	/*fp32 ConstantValue;
	if (!m_pAGCompiler->EvalConstantExpressionFloat(ConstantStr, ConstantValue, this, _Location))
	return -1;*/

	CXRAG2C_Condition Condition;
	Condition.m_iProperty = iProperty;
	Condition.m_PropertyType = PropertyType;
	Condition.m_iOperator = iOperator;
	//	Condition.m_Constant = ConstantValue;
	Condition.m_ConstantStr = ConstantStr;
	Condition.m_iFirstAction = _iAction;
	int32 iCondition = AddCondition(Condition);
	return iCondition;
}

//--------------------------------------------------------------------------------

int32 CXRAG2C_State::AddCondition(CXRAG2C_Condition& _Condition)
{
	int32 iCondition;

	for (iCondition = 0; iCondition < m_lConditions.Len(); iCondition++)
	{
		if (m_lConditions[iCondition] == _Condition)
		{
			m_lConditions[iCondition].m_Occurrences++;
			return iCondition;
		}
	}

	iCondition = m_lConditions.Len();

	/*
	int32 MaxCondition = 32;
	if (iCondition >= MaxCondition)
	{
	m_pCompiler->OnError(CStrF("Too many unique conditions in state '%s' (Max is %d).", (char*)m_Name, MaxCondition));
	return -1;
	}
	*/

	_Condition.m_Occurrences++;
	m_lConditions.Add(_Condition);

	return iCondition;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_State::ProcessPass1()
{
	CStr Location = m_Name;

	if (!ExpandInheritance())
		return false;

	for (int iCondition = 0; iCondition < m_lConditions.Len(); iCondition++)
	{
		CStr Location = m_Name + AG2C_SCOPESEPARATOR + CStr("ACTIONS");
		CStr ConstantStr = m_lConditions[iCondition].m_ConstantStr;
		fp32 ConstantValueFloat = 0.0f;
		int ConstantValueInt = 0;
		bool ConstantValueBool = 0;
		if (!m_pAGCompiler->EvalConstantExpressionByType(ConstantStr, m_lConditions[iCondition].m_PropertyType, ConstantValueFloat, ConstantValueInt,ConstantValueBool, this, Location))
		{
			m_pAGCompiler->OnError(CStrF("CXRAG2C_State::Process Expression not found: '%s' (%s)",ConstantStr.Str(),Location.Str()));
			return false;
		}
		m_lConditions[iCondition].m_ConstantValueFloat = ConstantValueFloat;
		m_lConditions[iCondition].m_ConstantValueInt = ConstantValueInt;
		m_lConditions[iCondition].m_ConstantValueBool = ConstantValueBool;
	}

	m_DisabledWarnings &= ~m_EnabledWarnings;

	/*if ((m_InheritFlags & AG2C_INHERITFLAGS_ANIMINDEX) && (m_lStateAnims.Len() == 0) && (!(m_DisabledWarnings & AG2C_DISABLEDWARNING_MISSINGANIMATIONINDEX)))
		m_pAGCompiler->OnWarning("Missing animation index (" + Location + ").");

	if ((m_lActions.Len() == 0) && (!(m_DisabledWarnings & AG2C_DISABLEDWARNING_MISSINGACTIONS)))
		m_pAGCompiler->OnWarning("Missing actions (" + Location + ").");*/
	/*
	// Sort conditions by frequency/occurences (hybrid selectionsort).
	for (int iCondition = 0; iCondition < m_lConditions.Len(); iCondition++)
	{
	int iMostOccuringCondition = 0;
	int Occurences = 0;
	for (int jCondition = 0; jCondition < m_lConditions.Len(); jCondition++)
	{
	if (((m_lConditions[jCondition].m_SortedIndex & AG2C_CONDITION_SORTEDBIT) == 0) && 
	(m_lConditions[jCondition].m_Occurrences > Occurences))
	{
	iMostOccuringCondition = jCondition;
	Occurences = m_lConditions[jCondition].m_Occurrences;
	}
	}

	m_lConditions[iMostOccuringCondition].m_SortedIndex = iCondition | AG2C_CONDITION_SORTEDBIT;
	}
	*/	
	/*
	for (int iCondition = 0; iCondition < m_lConditions.Len(); iCondition++)
	m_lConditions[iCondition].m_SortedIndex = iCondition | AG2C_CONDITION_SORTEDBIT;
	*/
	for (int iCondition = 0; iCondition < m_lConditions.Len(); iCondition++)
	{
		int iMostOccuringCondition = 0;
		int MostOccurences = 0;
		int iFirstAction = AG2_ACTIONINDEX_NULL;
		for (int jCondition = 0; jCondition < m_lConditions.Len(); jCondition++)
		{
			CXRAG2C_Condition* pCondition = &(m_lConditions[jCondition]);
			if (((pCondition->m_SortedIndex & AG2C_CONDITION_SORTEDBIT) == 0) && 
				(pCondition->m_Occurrences > MostOccurences) &&
				((iFirstAction == AG2_ACTIONINDEX_NULL) ||
				(pCondition->m_iFirstAction == iFirstAction)))
			{
				iMostOccuringCondition = jCondition;
				MostOccurences = pCondition->m_Occurrences;
				iFirstAction = pCondition->m_iFirstAction;
			}
		}

		if (MostOccurences > 0)
			m_lConditions[iMostOccuringCondition].m_SortedIndex = iCondition | AG2C_CONDITION_SORTEDBIT;
	}

	for (int iAction = 0; iAction < m_lActions.Len(); iAction++)
	{
		if (m_lActions[iAction].m_bExported)
			m_References++;

		CStr Location = m_Name + AG2C_SCOPESEPARATOR + CStrF("ACTION%d",iAction);

		// Resolve TargetState index.
		{
			CXRAG2C_Action* pAction = &(m_lActions[iAction]);
			// Make sure we don't move the same token more than once
			TArray<int8> MovedTokens;

			for (int32 i = 0; i < pAction->m_lMoveTokens.Len(); i++)
			{
				// Check so that state movetokens don't have graphblock (for now)
				/*if (pAction->m_lMoveTokens[i].m_TargetGraphBlock.Len() > 0)
				{
				m_pAGCompiler->OnError(CStrF("Can't move graphblock in a state: '%s' (%s).", (char*)(pAction->m_lMoveTokens[i].m_TargetGraphBlock), (char*)Location));
				return false;
				}*/
				for (int32 nMove = 0; nMove < MovedTokens.Len(); nMove++)
				{
					if (MovedTokens[nMove] == pAction->m_lMoveTokens[i].m_TokenID)
					{
						m_pAGCompiler->OnError(CStrF("Moving the same token more than once in the same action: '%s' (%s).", (char*)(pAction->m_lMoveTokens[i].m_TargetState), (char*)Location));
						return false;
					}
				}
				MovedTokens.Add(pAction->m_lMoveTokens[i].m_TokenID);

				CFStr TargetState = pAction->m_lMoveTokens[i].m_TargetState;
				if (TargetState == m_Name)
					m_pAGCompiler->OnWarning(CStrF("Cyclic target state '%s' (%s)", (char*)m_Name, (char*)Location));

				CXRAG2C_GraphBlock* pGraphBlock = m_pGraphBlock;

				if (pAction->m_lMoveTokens[i].m_TargetGraphBlock.Len() > 0)
				{
					pAction->m_lMoveTokens[i].m_iTargetGraphBlock = m_pAGCompiler->GetGraphBlockIndex(pAction->m_lMoveTokens[i].m_TargetGraphBlock);
					if (pAction->m_lMoveTokens[i].m_iTargetGraphBlock == -1)
					{
						m_pAGCompiler->OnError(CStrF("%s CXRAG2C_State::Process: Unable to find graphblock: %s",Location.Str(),pAction->m_lMoveTokens[i].m_TargetGraphBlock.GetStr()));
						return false;
					}

					pGraphBlock = m_pAGCompiler->m_lspGraphBlocks[pAction->m_lMoveTokens[i].m_iTargetGraphBlock];
				}

				pAction->m_lMoveTokens[i].m_iTargetState = AG2_STATEINDEX_NULL;
				if (TargetState == "TERMINATE")
				{
					pAction->m_lMoveTokens[i].m_iTargetState = AG2_STATEINDEX_TERMINATE;
				}
				else if (TargetState == "STARTAG")
				{
					pAction->m_lMoveTokens[i].m_iTargetState = AG2_STATEINDEX_STARTAG;
				}
				else
				{
					if (pAction->m_lMoveTokens[i].m_TargetState.Len() <= 0)
					{
						// No target state defined, use the startstate of target graphblock
						if (pAction->m_lMoveTokens[i].m_iTargetGraphBlock != AG2_GRAPHBLOCKINDEX_NULL)
						{
//							int32 iTargetState = pGraphBlock->m_StartMoveToken.m_iTargetState;
							pAction->m_lMoveTokens[i].m_iTargetState = pGraphBlock->m_StartMoveToken.m_iTargetState;//m_iStartState;
							pAction->m_lMoveTokens[i].m_TargetStateType = pGraphBlock->m_StartMoveToken.m_TargetStateType;
						}
					}
					else
					{
						pGraphBlock->AcquireTargetState(pAction->m_lMoveTokens[i]);
					}

					if (((pAction->m_lMoveTokens[i].m_iTargetState == AG2_STATEINDEX_NULL) && (pAction->m_nEffectInstances == 0)) ||
						((pAction->m_lMoveTokens[i].m_iTargetState == AG2_STATEINDEX_NULL) && (pAction->m_lMoveTokens[i].m_TargetState.Len() > 0)))
					{
						m_pAGCompiler->OnError(CStrF("Undefined target state '%s' (%s).", (char*)(pAction->m_lMoveTokens[i].m_TargetState), (char*)Location));
						return false;
					}
				}
			}
		}

		// Build ConditionBits.
		int TotalBits = 0;
		{
			int TotalWidth = (1 << m_lConditions.Len());
			int Width = TotalWidth;
			while (Width > 1)
			{
				TotalBits++;
				Width >>= 1;
			}
		}

		m_lActions[iAction].m_ConditionBits = 0;
		int ConditionMapLen = m_lActions[iAction].m_lConditionMap.Len();
		for (int iConditionMapping = 0; iConditionMapping < ConditionMapLen; iConditionMapping++)
		{
			int32& iCondition = m_lActions[iAction].m_lConditionMap[iConditionMapping];
			int SortedIndex = (m_lConditions[iCondition].m_SortedIndex & (~AG2C_CONDITION_SORTEDBIT));
			m_lActions[iAction].m_ConditionBits |= (1 << ((TotalBits - 1) - SortedIndex));
		}
	}

	BuildNodeTree();

	return true;
}


bool CXRAG2C_State::ProcessPass2()
{
	// Remake animation index
	for (int32 i = 0; i < m_lStateAnims.Len(); i++)
	{
		if (m_lStateAnims[i].m_iAnim != -1)
			m_lStateAnims[i].m_iAnim -= m_pAGCompiler->m_lAnimations[m_lStateAnims[i].m_iAnim].m_iOffset;
	}

	return true;
}

//--------------------------------------------------------------------------------

void CXRAG2C_State::BuildNodeTree()
{
	if (m_lConditions.Len() > 0)
	{
		int Width = (1 << m_lConditions.Len());
		BuildNodeTree_Recurse(0, 0, Width);
		OptimizeNodeTree();
		ConvertNodeActions();
	}
}

//--------------------------------------------------------------------------------

int CXRAG2C_State::BuildNodeTree_Recurse(int _Level, int _Offset, int _Width)
{
	bool bIsUsed = false;

	int TotalWidth = (1 << m_lConditions.Len());
	int MaxLevel = 0;

	{
		int Width = TotalWidth;
		while (Width > 1)
		{
			MaxLevel++;
			Width >>= 1;
		}
	}

	uint32 LevelBit = (_Level < MaxLevel) ? (1 << (MaxLevel - _Level - 1)) : 0;
	if (LevelBit > 0)
	{
		for (int CurOffset = _Offset; CurOffset < (_Offset + _Width); CurOffset++)
		{
			// Compute offset bits
			uint32 OffsetBits = TotalWidth - CurOffset - 1;
			int32 iOffsetAction = AG2_ACTIONINDEX_NULL;

			// Find first actions that matches offset bits.
			for (int iAction = 0; iAction < m_lActions.Len(); iAction++)
			{
				uint32 ActionConditionBits = m_lActions[iAction].m_ConditionBits;
				if ((OffsetBits & ActionConditionBits) == ActionConditionBits)
				{
					iOffsetAction = iAction;
					break;
				}
			}

			// See if action is active on this level.
			uint32 OffsetActionBits = 0;
			if (iOffsetAction != AG2_ACTIONINDEX_NULL)
				OffsetActionBits = m_lActions[iOffsetAction].m_ConditionBits;

			if ((OffsetActionBits & LevelBit) != 0)
			{
				bIsUsed = true;
				break;
			}
		}
	}

	if (bIsUsed)
	{
		int iSortedCondition = _Level;
		int iCondition = -1;

		for (int jCondition = 0; jCondition < m_lConditions.Len(); jCondition++)
		{
			uint16 SortedIndex = (m_lConditions[jCondition].m_SortedIndex) & (~AG2C_CONDITION_SORTEDBIT);
			if (SortedIndex == iSortedCondition)
			{
				iCondition = jCondition;
				break;
			}
		}

		if (iCondition == -1)
		{
			m_pAGCompiler->OnError(CStrF("Internal Error. Invalid condition %d'.", iSortedCondition));
			return AG2C_NODEACTION_FAILPARSE;
		}

		int iNode = m_lConditionNodes.Add(CXRAG2C_ConditionNode());

		int TrueAction = BuildNodeTree_Recurse(_Level + 1, _Offset, _Width / 2);
		int FalseAction = BuildNodeTree_Recurse(_Level + 1, _Offset + _Width / 2, _Width / 2);

		CXRAG2C_Condition* pCondition = &(m_lConditions[iCondition]);
		CXRAG2C_ConditionNode* pNode = &(m_lConditionNodes[iNode]);
		pNode->m_iProperty = pCondition->m_iProperty;
		pNode->m_PropertyType = pCondition->m_PropertyType;
		pNode->m_iOperator = pCondition->m_iOperator;
		pNode->m_ConstantFloat = pCondition->m_ConstantValueFloat;
		pNode->m_ConstantInt = pCondition->m_ConstantValueInt;
		pNode->m_ConstantBool = pCondition->m_ConstantValueBool;
		pNode->m_TrueAction = TrueAction;
		pNode->m_FalseAction = FalseAction;

		/*
		if ((iNode & ~AG2_NODEACTION_NODEMASK) != 0)
		{
		m_pCompiler->OnError(CStrF("Internal Error. Node index %d is out of range (%s).", iNode, (char*)m_Name));
		return AG2_NODEACTION_FAILPARSE;
		}
		*/

		return (iNode | AG2C_NODEACTION_PARSENODE);
	}
	else
	{
		if (_Width > 1)
		{
			return BuildNodeTree_Recurse(_Level + 1, _Offset, _Width / 2);
		}
		else
		{
			uint32 OffsetBits = TotalWidth - _Offset - 1;
			int32 iOffsetAction = AG2_ACTIONINDEX_NULL; // This is an invalid action, meaning parsing failed.

			for (int iAction = 0; iAction < m_lActions.Len(); iAction++)
			{
				if ((OffsetBits & m_lActions[iAction].m_ConditionBits) == m_lActions[iAction].m_ConditionBits)
				{
					iOffsetAction = iAction;
					break;
				}
			}

			if (iOffsetAction == AG2_ACTIONINDEX_NULL)
				return AG2C_NODEACTION_FAILPARSE;

			if ((iOffsetAction & ~AG2C_NODEACTION_ACTIONMASK) != 0)
			{
				m_pAGCompiler->OnError(CStrF("Internal Error. Action index %d is out of range (%s).", iOffsetAction, (char*)m_Name));
				return AG2C_NODEACTION_FAILPARSE;
			}

			return (iOffsetAction | AG2C_NODEACTION_ENDPARSE);
		}
	}
}

//--------------------------------------------------------------------------------

void CXRAG2C_State::OptimizeNodeTree()
{
	bool bDone = false;
	bool bMerged = true;
	while (!bDone)
	{
		bDone = true;
		for (int iNodeA = 0; iNodeA < (m_lConditionNodes.Len() - 1); iNodeA++)
		{
//			CXRAG2C_ConditionNode* pNodeA = &(m_lConditionNodes[iNodeA]);
			for (int iNodeB = iNodeA+1; iNodeB < m_lConditionNodes.Len(); iNodeB++)
			{
				bMerged = false;
//				CXRAG2C_ConditionNode* pNodeB = &(m_lConditionNodes[iNodeB]);
				if (m_lConditionNodes[iNodeA] == m_lConditionNodes[iNodeB])
				{
					bMerged = true;
					for (int iNodeC = 0; iNodeC < m_lConditionNodes.Len(); iNodeC++)
					{
						// Redirect C to lead to A instead of to B
						CXRAG2C_ConditionNode* pNodeC = &(m_lConditionNodes[iNodeC]);
						int TrueAction = pNodeC->m_TrueAction;
						if ((TrueAction & AG2C_NODEACTION_TYPEMASK) == AG2C_NODEACTION_PARSENODE)
						{
							int iNode = (TrueAction & AG2C_NODEACTION_NODEMASK);
							if (iNode == iNodeB)
								pNodeC->m_TrueAction = (iNodeA | AG2C_NODEACTION_PARSENODE);
							if (iNode > iNodeB)
								pNodeC->m_TrueAction = ((iNode - 1) | AG2C_NODEACTION_PARSENODE);
						}

						int FalseAction = pNodeC->m_FalseAction;
						if ((FalseAction & AG2C_NODEACTION_TYPEMASK) == AG2C_NODEACTION_PARSENODE)
						{
							int iNode = (FalseAction & AG2C_NODEACTION_NODEMASK);
							if (iNode == iNodeB)
								pNodeC->m_FalseAction = (iNodeA | AG2C_NODEACTION_PARSENODE);
							if (iNode > iNodeB)
								pNodeC->m_FalseAction = ((iNode - 1) | AG2C_NODEACTION_PARSENODE);
						}
					}
				}

				if (bMerged)
				{
					bDone = false;
					m_lConditionNodes.Del(iNodeB);
					iNodeB--;
				}
			}
		}
	}
}

//--------------------------------------------------------------------------------

bool CXRAG2C_State::ConvertNodeActions()
{
	for (int iNode = 0; iNode < m_lConditionNodes.Len(); iNode++)
	{
		if (!ConvertNodeAction(m_lConditionNodes[iNode].m_TrueAction))
			return false;
		if (!ConvertNodeAction(m_lConditionNodes[iNode].m_FalseAction))
			return false;
	}
	return true;
}

bool CXRAG2C_State::ConvertNodeAction(uint16& _NodeAction)
{
	uint16 NodeAction = _NodeAction;

	if (NodeAction == AG2C_NODEACTION_FAILPARSE)
	{
		_NodeAction = AG2_NODEACTION_FAILPARSE;
		return true;
	}
	else if ((NodeAction & AG2C_NODEACTION_TYPEMASK) == AG2C_NODEACTION_ENDPARSE)
	{
		int iAction = (NodeAction & AG2C_NODEACTION_ACTIONMASK);
		if ((iAction & ~AG2_NODEACTION_ACTIONMASK) != 0)
		{
			m_pAGCompiler->OnError(CStrF("Internal Error. Action index %d is out of range (%s).", iAction, (char*)m_Name));
			return false;
		}

		_NodeAction = iAction | AG2_NODEACTION_ENDPARSE;
		return true;
	}
	else if ((NodeAction & AG2C_NODEACTION_TYPEMASK) == AG2C_NODEACTION_PARSENODE)
	{
		int iNode = (NodeAction & AG2C_NODEACTION_NODEMASK);
		if ((iNode & ~AG2_NODEACTION_NODEMASK) != 0)
		{
			m_pAGCompiler->OnError(CStrF("Internal Error. Node index %d is out of range (%s).", iNode, (char*)m_Name));
			return false;
		}

		_NodeAction = iNode | AG2_NODEACTION_PARSENODE;
		return true;
	}

	return false;
}






/////////////////////////////////////////////////////
// SWITCHSTATES
/////////////////////////////////////////////////////

bool CXRAG2C_SwitchState::ParseSwitchStateActionValReg(const CRegistry* _pActionReg, 
													   CXRAG2C_SwitchStateActionval& _ActionVal)
{
	if (!_pActionReg)
		return false;
	
	// Get value on process
	_ActionVal.m_ConstantStr = _pActionReg->GetThisValue();
	
	return true;
}

bool CXRAG2C_SwitchState::ParseReg(CXRAG2C_GraphBlock* _pGraphBlock, const CRegistry* _pStateReg)
{
	if (_pGraphBlock == NULL)
		return false;

	if (_pStateReg == NULL)
		return false;

	m_pGraphBlock = _pGraphBlock;
	m_pAGCompiler = _pGraphBlock->m_pCompiler;

	m_Name = _pStateReg->GetThisName();

	CStr Location = m_Name;


	m_Priority = 0;
	m_References = 0;

	bool bDefaultActionFound = false;
	for (int iChild = 0; iChild < _pStateReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pStateReg->GetChild(iChild);

		if (pChildReg->GetThisName() == "PRIORITY")
		{
			fp32 Priority;
			if (!m_pAGCompiler->EvalConstantExpressionFloat(pChildReg->GetThisValue(), Priority, NULL, Location))
				return false;
			m_Priority = (uint8)Priority;
		}
		else if (pChildReg->GetThisName() == "ACTIONS")
		{
			if (!ParseActionsReg(pChildReg))
				return false;
		}
		else if (pChildReg->GetThisName() == "DEFAULTACTION")
		{
			if (!ParseActionReg(pChildReg, m_DefaultAction))
				return false;
			bDefaultActionFound = true;
		}
		else if (pChildReg->GetThisName() == "PROPERTY")
		{
			if (!ParsePropertyReg(pChildReg, Location))
				return false;
		}
		else
		{
			m_pAGCompiler->OnWarning(CStrF("Unrecognized keyword '%s' (%s).", (char*)pChildReg->GetThisName(), (char*)Location));
		}
	}

	if (!bDefaultActionFound)
		m_pAGCompiler->OnError(CStrF("No defaultaction (%s).", (char*)Location));

	return bDefaultActionFound;
}

bool CXRAG2C_SwitchState::ParseActionsReg(const CRegistry* _pActionsReg)
{
	if (_pActionsReg == NULL)
		return false;

	// FIXME: Check for unique and continously ordered ACTION# keywords.
	for (int iChild = 0; iChild < _pActionsReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pActionsReg->GetChild(iChild);
		CXRAG2C_Action NewAction;
		if (!ParseActionReg(pChildReg,NewAction))
			return false;
		else
			m_lActions.Add(NewAction);
	}

	return true;
}

bool CXRAG2C_SwitchState::ParseActionReg(const CRegistry* _pActionReg, CXRAG2C_Action& _Action)
{
	if (_pActionReg == NULL)
		return false;

	if (!(_pActionReg->GetThisName().Left(6) == "ACTION"  || _pActionReg->GetThisName().Left(13) == "DEFAULTACTION"))
	{
		m_pAGCompiler->OnError("Unrecognized keyword '" + _pActionReg->GetThisName() + "' (" + m_Name + AG2C_SCOPESEPARATOR + "ACTIONS).");
		return false;
	}

	CStr ActionName = _pActionReg->GetThisName();
	CStr Location = m_Name + AG2C_SCOPESEPARATOR + ActionName;
	_Action.m_Name = ActionName;

	bool bActionValFound = false;

	for (int iChild = 0; iChild < _pActionReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pActionReg->GetChild(iChild);

		if (pChildReg->GetThisName() == "MOVETOKEN")
		{
			CXRAG2C_MoveToken MoveToken;
			if (!ParseMoveTokenReg(pChildReg, Location, MoveToken))//MoveToken.m_TokenID, MoveToken.m_TargetState, MoveToken.m_AnimBlendDuration, MoveToken.m_AnimBlendDelay, MoveToken.m_AnimTimeOffset, MoveToken.m_iAnimTimeOffsetType))
				return false;
			_Action.m_lMoveTokens.Add(MoveToken);
		}
		else if (pChildReg->GetThisName() == "EFFECTS")
		{
			if (!ParseActionEffectsReg(pChildReg, _Action.m_iBaseEffectInstance, _Action.m_nEffectInstances, Location))
				return false;
		}
		else if (pChildReg->GetThisName().Left(9) == "ACTIONVAL")
		{
			if (!ParseSwitchStateActionValReg(pChildReg,_Action.m_ActionVal))
			{
				m_pAGCompiler->OnError("ActionVal Error (" + Location + ").");
				return false;
			}
			bActionValFound = true;
		}
		// Don't care about conditionsets
		else
		{
			m_pAGCompiler->OnWarning(CStrF("Unrecognized keyword '%s' (%s).", (char*)pChildReg->GetThisName(), (char*)Location));
		}
	}

	/*
	if ((TargetState == "") && (nEffects == 0))
	{
	m_pCompiler->OnError("Missing targetstate or effects (" + Location + ").");
	return false;
	}
	*/
	if (!bActionValFound)
		m_pAGCompiler->OnError(CStrF("No Actionval found (%s).", (char*)Location));
	return bActionValFound;
}

bool CXRAG2C_SwitchState::ParsePropertyReg(const CRegistry* _pConditionReg, CStr _Location)
{
	if (_pConditionReg == NULL)
		return false;

	// On the format "Property<(..)>"
	CStr PropertyStr = _pConditionReg->GetThisValue().UpperCase();
	PropertyStr.Trim();

	CStr PropertyNameStr = PropertyStr.GetStrSep("(");
	CStr PropertyParamsStr = PropertyStr.GetStrSep(")");
//	int32 PropertyType = -1;
	if (PropertyParamsStr.Len() > 0)
		m_pAGCompiler->OnError(CStrF("Parameters to a property not allowed '%s' (%s)", PropertyNameStr.Str(), _Location.Str()));
	// Find type of property	
//	int32 iProperty = -1;
	if (!m_pAGCompiler->GetPropertyID(PropertyNameStr,m_iProperty,m_PropertyType))
	{
		m_pAGCompiler->OnError(CStrF("Undefined Property '%s' (%s).", (char*)PropertyNameStr, (char*)_Location));
		return false;
	}

	if (!((m_PropertyType == AG2_PROPERTYTYPE_INT) || (m_PropertyType == AG2_PROPERTYTYPE_FUNCTION)))
	{
		m_pAGCompiler->OnError(CStrF("PropertyType Invalid '%s' (%s).", (char*)PropertyNameStr, (char*)_Location));
		return false;
	}

	return true;
}

bool CXRAG2C_SwitchState::ProcessPass1()
{
	CStr Location = m_Name;

	// Check property and go through all action values
	int32 NumActions = m_lActions.Len();
	for (int iActionVal = 0; iActionVal < NumActions+1; iActionVal++)
	{
		CXRAG2C_Action* pAction = iActionVal < NumActions ? &(m_lActions[iActionVal]) : &m_DefaultAction;
		CStr Location = m_Name + AG2C_SCOPESEPARATOR + CStr("ACTIONS");
		CStr ConstantStr = pAction->m_ActionVal.m_ConstantStr;
		fp32 ConstantValueFloat = 0.0f;
		int ConstantValueInt = 0;
		bool ConstantValueBool = 0;
		if (!m_pAGCompiler->EvalConstantExpressionByType(ConstantStr, m_PropertyType, ConstantValueFloat, ConstantValueInt,ConstantValueBool, NULL, Location))
		{
			m_pAGCompiler->OnError(CStrF("CXRAG2C_State::Process Expression not found: '%s' (%s)",ConstantStr.Str(),Location.Str()));
			return false;
		}
		pAction->m_ActionVal.m_ConstantValueFloat = ConstantValueFloat;
		pAction->m_ActionVal.m_ConstantValueInt = ConstantValueInt;
		pAction->m_ActionVal.m_ConstantValueBool = ConstantValueBool;
		pAction->m_ActionVal.m_PropertyType = m_PropertyType;
	}


	if ((m_lActions.Len() == 0) && !m_DefaultAction.m_nMoveTokens)
		m_pAGCompiler->OnError("Missing actions (" + Location + ").");

	// Sort actions
	TArray<CXRAG2C_Action> lActions;
	for (int32 j = 0; j < m_lActions.Len(); j++)
	{
		CXRAG2C_Action& Action = (m_lActions[j]);
		// Add last if no other found
		int32 iIndex = lActions.Len();
		for (int32 i = 0; i < lActions.Len(); i++)
		{
			if (lActions[i].m_ActionVal.Compare(Action.m_ActionVal) == 0)
			{
				m_pAGCompiler->OnError(CStrF("ActionVal with same values %s - %s ( %s ).",lActions[i].m_ActionVal.m_ConstantStr.Str(),Action.m_ActionVal.m_ConstantStr.Str(),Location.Str()));
				return false;
			}
			if (lActions[i].m_ActionVal.Compare(Action.m_ActionVal) < 0)
			{
				iIndex = i;
				break;
			}
		}
		lActions.Insert(iIndex,Action);
	}
	m_lActions = lActions;

	for (int iAction = 0; iAction < NumActions + 1; iAction++)
	{
		CXRAG2C_Action* pAction = iAction < NumActions ? &(m_lActions[iAction]) : &m_DefaultAction;

		if (pAction->m_bExported)
			m_References++;

		CStr Location = m_Name + AG2C_SCOPESEPARATOR + CStrF("ACTION%d",iAction);

		// Resolve TargetState index.
		{
			// Make sure we don't move the same token more than once
			TArray<int8> MovedTokens;

			if (pAction->m_lMoveTokens.Len() > 1)
				m_pAGCompiler->OnWarning(CStrF("Warning, moving more than one token in the same action in a switchstate (%s)", (char*)Location));
			for (int32 i = 0; i < pAction->m_lMoveTokens.Len(); i++)
			{
				for (int32 nMove = 0; nMove < MovedTokens.Len(); nMove++)
				{
					if (MovedTokens[nMove] == pAction->m_lMoveTokens[i].m_TokenID)
					{
						m_pAGCompiler->OnError(CStrF("Moving the same token more than once in the same action: '%s' (%s).", (char*)(pAction->m_lMoveTokens[i].m_TargetState), (char*)Location));
						return false;
					}
				}
				MovedTokens.Add(pAction->m_lMoveTokens[i].m_TokenID);

				CFStr TargetState = pAction->m_lMoveTokens[i].m_TargetState;
				if (TargetState == m_Name)
					m_pAGCompiler->OnWarning(CStrF("Cyclic target state '%s' (%s)", (char*)m_Name, (char*)Location));

				CXRAG2C_GraphBlock* pGraphBlock = m_pGraphBlock;

				if (pAction->m_lMoveTokens[i].m_TargetGraphBlock.Len() > 0)
				{
					pAction->m_lMoveTokens[i].m_iTargetGraphBlock = m_pAGCompiler->GetGraphBlockIndex(pAction->m_lMoveTokens[i].m_TargetGraphBlock);
					if (pAction->m_lMoveTokens[i].m_iTargetGraphBlock == -1)
					{
						m_pAGCompiler->OnError(CStrF("%s CXRAG2C_SwitchState::Process: Unable to find graphblock: %s",Location.Str(),pAction->m_lMoveTokens[i].m_TargetGraphBlock.GetStr()));
						return false;
					}

					pGraphBlock = m_pAGCompiler->m_lspGraphBlocks[pAction->m_lMoveTokens[i].m_iTargetGraphBlock];
				}

				pAction->m_lMoveTokens[i].m_iTargetState = AG2_STATEINDEX_NULL;
				if (TargetState == "TERMINATE")
				{
					pAction->m_lMoveTokens[i].m_iTargetState = AG2_STATEINDEX_TERMINATE;
				}
				else if (TargetState == "STARTAG")
				{
					pAction->m_lMoveTokens[i].m_iTargetState = AG2_STATEINDEX_STARTAG;
				}
				else
				{
					if (pAction->m_lMoveTokens[i].m_TargetState.Len() <= 0)
					{
						// No target state defined, use the startstate of target graphblock
						if (pAction->m_lMoveTokens[i].m_iTargetGraphBlock != AG2_GRAPHBLOCKINDEX_NULL)
						{
//							int32 iTargetState = pGraphBlock->m_StartMoveToken.m_iTargetState;
							pAction->m_lMoveTokens[i].m_iTargetState = pGraphBlock->m_StartMoveToken.m_iTargetState;//m_iStartState;
							pAction->m_lMoveTokens[i].m_TargetStateType = pGraphBlock->m_StartMoveToken.m_TargetStateType;
						}
					}
					else
					{
						pGraphBlock->AcquireTargetState(pAction->m_lMoveTokens[i]);
					}

					if (((pAction->m_lMoveTokens[i].m_iTargetState == AG2_STATEINDEX_NULL) && (pAction->m_nEffectInstances == 0)) ||
						((pAction->m_lMoveTokens[i].m_iTargetState == AG2_STATEINDEX_NULL) && (pAction->m_lMoveTokens[i].m_TargetState.Len() > 0)))
					{
						m_pAGCompiler->OnError(CStrF("Undefined target state '%s' (%s).", (char*)(pAction->m_lMoveTokens[i].m_TargetState), (char*)Location));
						return false;
					}
				}
			}
		}
	}


	return true;
}

bool CXRAG2C_SwitchState::ProcessPass2()
{
	return true;
}
