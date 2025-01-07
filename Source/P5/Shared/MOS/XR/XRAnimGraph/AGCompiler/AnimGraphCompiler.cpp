#include "PCH.h"

//--------------------------------------------------------------------------------

#include "../AnimGraphDefs.h"
#include "AnimGraphCompiler.h"

//--------------------------------------------------------------------------------

const char* ms_lpAGCAnimTimeOffsetTypes[] = { "STATIC", "INHERIT", "ROLLBACKBLEND", NULL };

//--------------------------------------------------------------------------------

#define AGC_SCOPESEPARATOR " . "

//--------------------------------------------------------------------------------

#define AGC_CONDITION_SORTEDBIT		(1<<15)

//--------------------------------------------------------------------------------

static CStr ResolvePath(CStr _Path)
{
	CStr Result;

	TArray<CFStr> Dirs;
	while (_Path != "")
	{
		CFStr Dir = _Path.GetStrSep("\\");
		if (Dir != "..")
			Dirs.Add(Dir);
		else
			Dirs.Del(Dirs.Len() - 1);
	}

	for (int iDir = 0; iDir < Dirs.Len(); iDir++)
	{
		if (Result == "")
			Result = Result + Dirs[iDir];
		else
			Result = Result + "\\" + Dirs[iDir];
	}

/*
	while (_Path != "")
	{
		int iSlashPos;
		if (_Path.Left(2) == "..")
		{
			iSlashPos = Result.FindReverse("\\");
			Result = Result.DelFrom(iSlashPos);
			iSlashPos = _Path.Find("\\");
			_Path = _Path.RightFrom(iSlashPos + 1);
		}
		else
		{
			iSlashPos = _Path.Find("\\");
			if (iSlashPos != -1)
			{
				if (Result == "")
					Result = Result + _Path.LeftTo(iSlashPos - 1);
				else
					Result = Result + "\\" + _Path.LeftTo(iSlashPos - 1);
				_Path = _Path.RightFrom(iSlashPos + 1);
			}
			else
			{
				if (Result == "")
					Result = Result + _Path;
				else
					Result = Result + "\\" + _Path;
				_Path = "";
			}
		}
	}
*/
	return Result;
}

//--------------------------------------------------------------------------------

bool CXRAGC_State::ExpandInheritance()
{
	m_bPendingExpandInheritance = true;

	for (int iSuperState = 0; iSuperState < m_lSuperStates.Len(); iSuperState++)
	{
		CFStr SuperStateName = m_lSuperStates[iSuperState];
		bool bSuperStateFound = false;
		for (int iState = 0; iState < m_pCompiler->m_lpStates.Len(); iState++)
		{
			if (SuperStateName == (m_pCompiler->m_lpStates[iState]->m_Name))
			{
				if (!Inherit(*(m_pCompiler->m_lpStates[iState])))
					return false;
				bSuperStateFound = true;
				break;
			}
		}
		if (!bSuperStateFound)
		{
			m_pCompiler->OnError(CStrF("Undefined state '%s' inherited by state '%s'.", (char*)SuperStateName, (char*)m_Name));
			return false;
		}
	}

	m_bPendingExpandInheritance = false;
	m_bExpandedInheritance = true;

	return true;
}

//--------------------------------------------------------------------------------

#define INHERIT(Flag)	(((m_InheritFlags & Flag) != 0) && ((_SuperState.m_InheritFlags & Flag) == 0))

//--------------------------------------------------------------------------------

// Assumes _SuperState is already fully expanded.
bool CXRAGC_State::Inherit(CXRAGC_State& _SuperState)
{
	if (_SuperState.m_bPendingExpandInheritance)
	{
		m_pCompiler->OnError("Cyclic inheritance between state '" + m_Name + "' and state '" + _SuperState.m_Name + "'.");
		return false;
	}

	if (!_SuperState.m_bExpandedInheritance)
		if (!_SuperState.ExpandInheritance())
			return false;
	
	if (INHERIT(AGC_INHERITFLAGS_FLAGS0))
		m_Flags[0] |= _SuperState.m_Flags[0] & _SuperState.m_FlagsMask[0];

	if (INHERIT(AGC_INHERITFLAGS_FLAGS1))
		m_Flags[1] |= _SuperState.m_Flags[1] & _SuperState.m_FlagsMask[1];

	if (INHERIT(AGC_INHERITFLAGS_PRIORITY))
		m_Priority = _SuperState.m_Priority;

	// If we don't have any anims, just copy animations from inherited state
	if (INHERIT(AGC_INHERITFLAGS_ANIMINDEX))
	{
		int32 Len = _SuperState.m_lStateAnims.Len();
		m_lStateAnims.SetLen(Len);
		for (int32 i = 0; i < Len; i++)
			m_lStateAnims[i] = _SuperState.m_lStateAnims[i];//.m_iAnim;
	}
	/*if (INHERIT(AGC_INHERITFLAGS_ANIMBASEJOINT))
	{
		int32 Len = _SuperState.m_lStateAnims.Len();
		m_lStateAnims.SetLen(Len);
		for (int32 i = 0; i < Len; i++)
			m_lStateAnims[i].m_iBaseJoint = _SuperState.m_lStateAnims[i].m_iBaseJoint;
	}
	if (INHERIT(AGC_INHERITFLAGS_ANIMTIMEOFFSET))
	{
		int32 Len = _SuperState.m_lStateAnims.Len();
		m_lStateAnims.SetLen(Len);
		for (int32 i = 0; i < Len; i++)
			m_lStateAnims[i].m_TimeOffset = _SuperState.m_lStateAnims[i].m_TimeOffset;
	}*/
/*
	if (INHERIT(AGC_INHERITFLAGS_ANIMTIMEOFFSETTYPE))
		m_StateAnim.m_TimeOffset = _SuperState.m_StateAnim.m_TimeOffsetType;
*/
	/*if (INHERIT(AGC_INHERITFLAGS_ANIMTIMESCALE))
	{
		int32 Len = _SuperState.m_lStateAnims.Len();
		m_lStateAnims.SetLen(Len);
		for (int32 i = 0; i < Len; i++)
			m_lStateAnims[i].m_TimeScale = _SuperState.m_lStateAnims[i].m_TimeScale;
	}
	if (INHERIT(AGC_INHERITFLAGS_ANIMOPACITY))
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
		CXRAGC_Condition Condition = _SuperState.m_lConditions[iCondition];
		Condition.m_SortedIndex = 0;
		Condition.m_iFirstAction += nOwnActions;
		m_lConditions.Add(Condition);
	}

	// Copy actions and adjust conditionmap indices and effect base indices.
	for (int jAction = 0; jAction < _SuperState.m_lActions.Len(); jAction++)
	{
		int iAction = nOwnActions + jAction;
		CXRAGC_Action InheritedAction = _SuperState.m_lActions[jAction]; // We have to copy safely here, since Add() merely will memcpy the element's content.
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
		CXRAGC_Condition Condition = m_lConditions[iCondition];
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
			CXRAGC_Condition ConditionA = m_lConditions[iCondition];
			CXRAGC_Condition ConditionB = m_lConditions[jCondition];
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
		CXRAGC_Condition Condition = m_lConditions[iCondition];

	for (iAction = 0; iAction < m_lActions.Len(); iAction++)
		for (int iConditionMapping = 0; iConditionMapping < m_lActions[iAction].m_lConditionMap.Len(); iConditionMapping++)
			int iCondition = m_lActions[iAction].m_lConditionMap[iConditionMapping];
*/
	return true;
}

//--------------------------------------------------------------------------------

bool CXRAGC_State::ParseReg(CXR_AnimGraphCompiler* _pCompiler, const CRegistry* _pStateReg)
{
	if (_pCompiler == NULL)
		return false;

	if (_pStateReg == NULL)
		return false;

	m_pCompiler = _pCompiler;

	m_Name = _pStateReg->GetThisName();

	CStr Location = m_Name;

	m_InheritFlags = AGC_INHERITFLAGS_ALL;
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
			m_InheritFlags &= ~AGC_INHERITFLAGS_FLAGS0;
			if (!m_pCompiler->ParseStateFlags(pChildReg->GetThisValue(), m_Flags[0], m_FlagsMask[0], Location))
				return false;
		}
		else if (pChildReg->GetThisName() == "FLAGSHI")
		{
			m_InheritFlags &= ~AGC_INHERITFLAGS_FLAGS1;
			if (!m_pCompiler->ParseStateFlags(pChildReg->GetThisValue(), m_Flags[1], m_FlagsMask[1], Location))
				return false;
		}
		else if (pChildReg->GetThisName() == "PRIORITY")
		{
			m_InheritFlags &= ~AGC_INHERITFLAGS_PRIORITY;
			fp32 Priority;
			if (!m_pCompiler->EvalConstantExpression(pChildReg->GetThisValue(), Priority, this, Location))
				return false;
			m_Priority = Priority;
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
			m_pCompiler->OnWarning(CStrF("Unrecognized keyword '%s' (%s).", (char*)pChildReg->GetThisName(), (char*)Location));
		}
	}

//	Process();

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAGC_State::ParseInheritReg(const CRegistry* _pInheritReg)
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
		m_pCompiler->OnWarning("Missing inheritance specification.");
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

bool CXRAGC_State::ParseAnimationReg(const CRegistry* _pAnimationReg)
{
	if (_pAnimationReg == NULL)
		return false;

	CStr Location = m_Name;

	CXRAGC_StateAnim TempAnim;

	if (_pAnimationReg->GetThisValue() != "")
	{
		m_InheritFlags &= ~AGC_INHERITFLAGS_ANIMINDEX;
		fp32 iAnim;
		if (!m_pCompiler->EvalConstantExpression(_pAnimationReg->GetThisValue(), iAnim, this, Location))
			return false;
		TempAnim.m_iAnim = iAnim;
	}
	
	for (int iChild = 0; iChild < _pAnimationReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pAnimationReg->GetChild(iChild);

		if (pChildReg->GetThisName() == "BASEJOINT")
		{
			m_InheritFlags &= ~AGC_INHERITFLAGS_ANIMBASEJOINT;
			TempAnim.m_iBaseJoint = pChildReg->GetThisValuei();
		}
/*
		else if (pChildReg->GetThisName() == "TIMEOFFSETTYPE")
		{
			m_InheritFlags &= ~AGC_INHERITFLAGS_ANIMTIMEOFFSETTYPE;
			m_StateAnim.m_TimeOffsetType = pChildReg->GetThisValue().TranslateInt(ms_lpAGCAnimTimeOffsetTypes);
		}
*/
		else if (pChildReg->GetThisName() == "TIMEOFFSET")
		{
			m_InheritFlags &= ~AGC_INHERITFLAGS_ANIMTIMEOFFSET;
			if (!m_pCompiler->EvalConstantExpression(pChildReg->GetThisValue(), TempAnim.m_TimeOffset, this, Location))
				return false;
		}
		else if (pChildReg->GetThisName() == "TIMESCALE")
		{
			m_InheritFlags &= ~AGC_INHERITFLAGS_ANIMTIMESCALE;
			if (!m_pCompiler->EvalConstantExpression(pChildReg->GetThisValue(), TempAnim.m_TimeScale, this, Location))
				return false;
		}
		else if (pChildReg->GetThisName() == "OPACITY")
		{
			m_InheritFlags &= ~AGC_INHERITFLAGS_ANIMOPACITY;
			if (!m_pCompiler->EvalConstantExpression(pChildReg->GetThisValue(), TempAnim.m_Opacity, this, Location))
				return false;
		}
		else
		{
			m_pCompiler->OnWarning(CStrF("Unrecognized keyword '%s' (%s).", (char*)pChildReg->GetThisName(), (char*)Location));
		}
	}

	// Add animation
	m_lStateAnims.Add(TempAnim);

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAGC_State::ParseStateConstantsReg(const CRegistry* _pStateConstantsReg)
{
	if (_pStateConstantsReg == NULL)
		return false;

	CStr Location = "(" + m_Name + AGC_SCOPESEPARATOR + "CONSTANTS)";

	for (int iChild = 0; iChild < _pStateConstantsReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pStateConstantsReg->GetChild(iChild);
		CStr SCName = pChildReg->GetThisName();

		uint32 SCID = m_pCompiler->GetStateConstantID(SCName);
		if (SCID == -1)
		{
			m_pCompiler->OnError(CStrF("Undefined stateconstant '%s' (%s).", (char*)SCName, (char*)Location));
			return false;
		}

		fp32 SCValue;
		if (!m_pCompiler->EvalConstantExpression(pChildReg->GetThisValue(), SCValue, this, Location))
			return false;

		CXRAGC_StateConstant StateConstant(SCName, SCID, SCValue);
		m_lConstants.Add(StateConstant);
	}

	for (int iConstant = 0; iConstant < m_lConstants.Len(); iConstant++)
	{
		int32* pConstantID = &(m_lConstants[iConstant].m_ID);
		fp32* pConstantValue = &(m_lConstants[iConstant].m_Value);
		int Apa = 5;
	}

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAGC_State::ParseActionsReg(const CRegistry* _pActionsReg)
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

bool CXRAGC_State::ParseActionReg(const CRegistry* _pActionReg)
{
	if (_pActionReg == NULL)
		return false;

	if (_pActionReg->GetThisName().Left(6) != "ACTION")
	{
		m_pCompiler->OnError("Unrecognized keyword '" + _pActionReg->GetThisName() + "' (" + m_Name + AGC_SCOPESEPARATOR + "ACTIONS).");
		return false;
	}

	CStr ActionName = _pActionReg->GetThisName();
	CStr Location = m_Name + AGC_SCOPESEPARATOR + ActionName;

	int8 TokenID = AG_TOKENID_NULL;
	CStr TargetState;
	fp32 AnimBlendDuration = 0;
	fp32 AnimBlendDelay = 0;
	fp32 AnimTimeOffset = 0;
	int8 iAnimTimeOffsetType = AG_ANIMLAYER_TIMEOFFSETTYPE_INDEPENDENT;
	int iBaseEffect = 0;
	int nEffects = 0;
	bool bExported = false;

	bool bConditionSetFound = false;

	for (int iChild = 0; iChild < _pActionReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pActionReg->GetChild(iChild);

		if (pChildReg->GetThisName() == "EXPORT")
		{
			bExported = true;
		}
		else if (pChildReg->GetThisName() == "MOVETOKEN")
		{
			if (!ParseMoveTokenReg(pChildReg, Location, TokenID, TargetState, AnimBlendDuration, AnimBlendDelay, AnimTimeOffset, iAnimTimeOffsetType))
				return false;
		}
		else if (pChildReg->GetThisName() == "EFFECTS")
		{
			if (!ParseActionEffectsReg(pChildReg, iBaseEffect, nEffects, Location))
				return false;
		}
		else if (pChildReg->GetThisName().Left(12) == "CONDITIONSET")
		{
			if ((TargetState == "") && (nEffects == 0))
			{
				m_pCompiler->OnError("Missing targetstate or effects (" + Location + ").");
				m_pCompiler->OnNote("MoveActions must be declared above all ConditionSets.");
				return false;
			}

			bConditionSetFound = true;
			if (!ParseConditionSetReg(pChildReg, Location, ActionName, bExported, TokenID, TargetState, iBaseEffect, nEffects, AnimBlendDuration, AnimBlendDelay, AnimTimeOffset, iAnimTimeOffsetType))
				return false;
		}
		else if (pChildReg->GetThisName() == "EDITPROPERTIES")
		{
			// Ignore editor-only keys.
		}
		else
		{
			m_pCompiler->OnWarning(CStrF("Unrecognized keyword '%s' (%s).", (char*)pChildReg->GetThisName(), (char*)Location));
		}
	}

	if (!bConditionSetFound)
	{
		m_pCompiler->OnError("Missing conditions (" + Location + ").");
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

bool CXRAGC_State::ParseMoveTokenReg(const CRegistry* _pMoveTokenReg, CStr _Location,
									 int8& _TokenID, CStr& _TargetState, 
									 fp32& _AnimBlendDuration, fp32& _AnimBlendDelay, fp32& _AnimTimeOffset, int8& _iAnimTimeOffsetType)
{
	fp32 TokenID = AG_TOKENID_NULL;
	if (_pMoveTokenReg->GetThisValue() != "")
	{
		if (!m_pCompiler->EvalConstantExpression(_pMoveTokenReg->GetThisValue(), TokenID, this, _Location))
			return false;
	}
	_TokenID = TokenID;

	for (int iChild = 0; iChild < _pMoveTokenReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pMoveTokenReg->GetChild(iChild);

		if (pChildReg->GetThisName() == "TARGETSTATE")
		{
			_TargetState = pChildReg->GetThisValue().UpperCase();
		}
		else if (pChildReg->GetThisName() == "ANIMBLENDDURATION")
		{
			if (!m_pCompiler->EvalConstantExpression(pChildReg->GetThisValue(), _AnimBlendDuration, this, _Location))
				return false;
		}
		else if (pChildReg->GetThisName() == "ANIMBLENDDELAY")
		{
			if (!m_pCompiler->EvalConstantExpression(pChildReg->GetThisValue(), _AnimBlendDelay, this, _Location))
				return false;
		}
		else if (pChildReg->GetThisName() == "ANIMTIMEOFFSET")
		{
			if (!m_pCompiler->EvalConstantExpression(pChildReg->GetThisValue(), _AnimTimeOffset, this, _Location))
				return false;
		}
		else if (pChildReg->GetThisName() == "ANIMTIMEOFFSETTYPE")
		{
			CStr TimeOffsetType = pChildReg->GetThisValue().UpperCase();
			if (TimeOffsetType == "INDEPENDENT")
				_iAnimTimeOffsetType = AG_ANIMLAYER_TIMEOFFSETTYPE_INDEPENDENT;
			else if (TimeOffsetType == "INHERIT")
				_iAnimTimeOffsetType = AG_ANIMLAYER_TIMEOFFSETTYPE_INHERIT;
			else if (TimeOffsetType == "INHERITEQ")
				_iAnimTimeOffsetType = AG_ANIMLAYER_TIMEOFFSETTYPE_INHERITEQ;
			else
			{
				m_pCompiler->OnError(CStrF("Invalid TimeOffsetType '%s' (%s)", (char*)TimeOffsetType, (char*)_Location));
				return false;
			}
		}
	}

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAGC_State::ParseActionEffectsReg(const CRegistry* _pEffectsReg, int& _iBaseEffect, int& _nEffects, CStr _Location)
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
		
		int32 ID = m_pCompiler->GetEffectID(Name);
		if (ID == -1)
		{
			m_pCompiler->OnError(CStrF("Undefined effect '%s' (%s).", (char*)Name, (char*)_Location));
			m_pCompiler->OnNote(CStr("Syntax is 'Effect(Param0, Param1, ..., ParamN)'."));
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
			m_pCompiler->OnError(CStrF("Missing parantheses enclosing parameters '%s' (%s).", (char*)pEffectReg->GetThisValue(), (char*)_Location));
			m_pCompiler->OnNote(CStr("Syntax is 'Effect(Param0, Param1, ..., ParamN)'."));
			return false;
		}

		CXRAGC_EffectInstance EffectInstance(ID);

		if (EffectStr != "")
		{
			// Resolve constantnames here, since CXRAG_CallbackParams should not have access to the compiler.
			CStr ParamsStrResolved;
			{
				CStr ParamsStr = EffectStr;
				int iParam = 0;
				while (ParamsStr != "")
				{
					CStr ParamStr = ParamsStr.GetStrSep(","); ParamStr.Trim();
					if (ParamStr != "")
					{
						fp32 ParamValue;
						if (!m_pCompiler->EvalConstantExpression(ParamStr, ParamValue, this, _Location))
							return false;

						if (ParamsStrResolved != "")
							ParamsStrResolved += "," + CStrF("%f", ParamValue);
						else
							ParamsStrResolved = CStrF("%f", ParamValue);
					}
				}
			}

			if (!m_pCompiler->ParseParamsStr(ParamsStrResolved, EffectInstance.m_lParams, _Location))
				return false;
		}

		m_lEffectInstances.Add(EffectInstance);
		_nEffects++;
	}			

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAGC_State::ParseConditionSetReg(const CRegistry* _pActionReg, CStr _Location, CStr _ActionName,
										bool _bExported, int8 _TokenID, CStr _TargetState, int _iBaseEffectInstance, int _nEffectInstances, 
										fp32 _AnimBlendDuration, fp32 _AnimBlendDelay, fp32 _AnimTimeOffset, int8 _iAnimTimeOffsetType)
{
	if (_pActionReg == NULL)
		return false;

	//CStr Location = _Location + AGC_SCOPESEPARATOR + _ActionName;
	CStr Location = _Location;

	if (_pActionReg->GetNumChildren() == 0)
	{
		m_pCompiler->OnError("Missing conditions (" + Location + ").");
		return false;
	}

	CXRAGC_Action Action;
	Action.m_TokenID = AG_TOKENID_NULL;
	Action.m_iTargetState = AG_STATEINDEX_NULL;

	Action.m_Name = _ActionName;
	Action.m_bExported = _bExported;
	Action.m_TokenID = _TokenID;
	Action.m_TargetState = _TargetState;
	Action.m_iBaseEffectInstance = _iBaseEffectInstance;
	Action.m_nEffectInstances = _nEffectInstances;
	Action.m_AnimBlendDuration = _AnimBlendDuration;
	Action.m_AnimBlendDelay = _AnimBlendDelay;
	Action.m_AnimTimeOffset = _AnimTimeOffset;
	Action.m_iAnimTimeOffsetType = _iAnimTimeOffsetType;

	int iAction = AddAction(Action);

	for (int iChild = 0; iChild < _pActionReg->GetNumChildren(); iChild++)
	{
		int ConditionMapLen = m_lActions[iAction].m_lConditionMap.Len();
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

int32 CXRAGC_State::AddAction(CXRAGC_Action& _Action)
{
	int32 iAction = m_lActions.Len();
	m_lActions.Add(_Action);
	return iAction;
}

//--------------------------------------------------------------------------------

int CXRAGC_State::ParseConditionReg(const CRegistry* _pConditionReg, int _iAction, CStr _Location)
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
			m_pCompiler->OnError(CStrF("Missing right paranthesis in condition '%s' (%s).", (char*)ConditionStr, (char*)_Location));
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
		m_pCompiler->OnError(CStrF("Invalid syntax in condition '%s' (%s).", (char*)ConditionStr, (char*)_Location));
		m_pCompiler->OnNote(CStr("Condition syntax is '[!]Property' or 'Property Operator Constant'."));
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
	int iProperty = m_pCompiler->GetPropertyID(PropertyNameStr);
	if (iProperty == -1)
	{
		m_pCompiler->OnError(CStrF("Undefined Property '%s' (%s).", (char*)PropertyNameStr, (char*)_Location));
		return -1;
	}

	TArray<fp32> lPropertyParams;
	if (!m_pCompiler->ParseParamsStr(PropertyParamsStr, lPropertyParams, _Location))
		return -1;

	int iOperator = m_pCompiler->GetOperatorID(OperatorStr);
	if (iOperator == -1)
	{
		m_pCompiler->OnError(CStrF("Undefined operator '%s' (%s).", (char*)OperatorStr, (char*)_Location));
		return -1;
	}

	fp32 ConstantValue;
	if (!m_pCompiler->EvalConstantExpression(ConstantStr, ConstantValue, this, _Location))
		return -1;

	CXRAGC_Condition Condition;
	Condition.m_iProperty = iProperty;
	lPropertyParams.Duplicate(&Condition.m_lPropertyParams);
	Condition.m_iOperator = iOperator;
//	Condition.m_Constant = ConstantValue;
	Condition.m_ConstantStr = ConstantStr;
	Condition.m_iFirstAction = _iAction;
	int32 iCondition = AddCondition(Condition);
	return iCondition;
}

//--------------------------------------------------------------------------------

int32 CXRAGC_State::AddCondition(CXRAGC_Condition& _Condition)
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

bool CXRAGC_State::Process()
{
	CStr Location = m_Name;

	if (!ExpandInheritance())
		return false;

	for (int iCondition = 0; iCondition < m_lConditions.Len(); iCondition++)
	{
		CStr Location = m_Name + AGC_SCOPESEPARATOR + CStr("ACTIONS");
		CStr ConstantStr = m_lConditions[iCondition].m_ConstantStr;
		fp32 ConstantValue;
		if (!m_pCompiler->EvalConstantExpression(ConstantStr, ConstantValue, this, Location))
			return false;
		m_lConditions[iCondition].m_ConstantValue = ConstantValue;
	}

	m_DisabledWarnings &= ~m_EnabledWarnings;

	if ((m_InheritFlags & AGC_INHERITFLAGS_ANIMINDEX) && (m_lStateAnims.Len() == 0) && (!(m_DisabledWarnings & AGC_DISABLEDWARNING_MISSINGANIMATIONINDEX)))
		m_pCompiler->OnWarning("Missing animation index (" + Location + ").");

	if ((m_lActions.Len() == 0) && (!(m_DisabledWarnings & AGC_DISABLEDWARNING_MISSINGACTIONS)))
		m_pCompiler->OnWarning("Missing actions (" + Location + ").");
/*
	// Sort conditions by frequency/occurences (hybrid selectionsort).
	for (int iCondition = 0; iCondition < m_lConditions.Len(); iCondition++)
	{
		int iMostOccuringCondition = 0;
		int Occurences = 0;
		for (int jCondition = 0; jCondition < m_lConditions.Len(); jCondition++)
		{
			if (((m_lConditions[jCondition].m_SortedIndex & AGC_CONDITION_SORTEDBIT) == 0) && 
				(m_lConditions[jCondition].m_Occurrences > Occurences))
			{
				iMostOccuringCondition = jCondition;
				Occurences = m_lConditions[jCondition].m_Occurrences;
			}
		}

		m_lConditions[iMostOccuringCondition].m_SortedIndex = iCondition | AGC_CONDITION_SORTEDBIT;
	}
*/	
/*
	for (int iCondition = 0; iCondition < m_lConditions.Len(); iCondition++)
		m_lConditions[iCondition].m_SortedIndex = iCondition | AGC_CONDITION_SORTEDBIT;
*/
	for (int iCondition = 0; iCondition < m_lConditions.Len(); iCondition++)
	{
		int iMostOccuringCondition = 0;
		int MostOccurences = 0;
		int iFirstAction = AG_ACTIONINDEX_NULL;
		for (int jCondition = 0; jCondition < m_lConditions.Len(); jCondition++)
		{
			CXRAGC_Condition* pCondition = &(m_lConditions[jCondition]);
			if (((pCondition->m_SortedIndex & AGC_CONDITION_SORTEDBIT) == 0) && 
				(pCondition->m_Occurrences > MostOccurences) &&
				((iFirstAction == AG_ACTIONINDEX_NULL) ||
				(pCondition->m_iFirstAction == iFirstAction)))
			{
				iMostOccuringCondition = jCondition;
				MostOccurences = pCondition->m_Occurrences;
				iFirstAction = pCondition->m_iFirstAction;
			}
		}

		if (MostOccurences > 0)
			m_lConditions[iMostOccuringCondition].m_SortedIndex = iCondition | AGC_CONDITION_SORTEDBIT;
	}

	for (int iAction = 0; iAction < m_lActions.Len(); iAction++)
	{
		if (m_lActions[iAction].m_bExported)
			m_References++;

		CStr Location = m_Name + AGC_SCOPESEPARATOR + CStrF("ACTION%d",iAction);

		// Resolve TargetState index.
		{
			CXRAGC_Action* pAction = &(m_lActions[iAction]);
			if (pAction->m_TargetState == m_Name)
				m_pCompiler->OnWarning(CStrF("Cyclic target state '%s' (%s)", (char*)m_Name, (char*)Location));

			pAction->m_iTargetState = AG_STATEINDEX_NULL;
			if (pAction->m_TargetState != "TERMINATE")
			{
				for (int jState = 0; jState < m_pCompiler->m_lpStates.Len(); jState++)
				{
					CFStr StateName = m_pCompiler->m_lpStates[jState]->m_Name;
					if (pAction->m_TargetState == StateName)
					{
						pAction->m_iTargetState = jState;
						m_pCompiler->m_lpStates[jState]->m_References++;
					}
				}
				if (((pAction->m_iTargetState == AG_STATEINDEX_NULL) && (pAction->m_nEffectInstances == 0)) ||
					((pAction->m_iTargetState == AG_STATEINDEX_NULL) && (pAction->m_TargetState.Len() > 0)))
				{
					m_pCompiler->OnError(CStrF("Undefined target state '%s' (%s).", (char*)(pAction->m_TargetState), (char*)Location));
					return false;
				}
			}
			else
			{
				pAction->m_iTargetState = AG_STATEINDEX_TERMINATE;
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
			int SortedIndex = (m_lConditions[iCondition].m_SortedIndex & (~AGC_CONDITION_SORTEDBIT));
			m_lActions[iAction].m_ConditionBits |= (1 << ((TotalBits - 1) - SortedIndex));
		}
	}

	BuildNodeTree();

	return true;
}

//--------------------------------------------------------------------------------

void CXRAGC_State::BuildNodeTree()
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

int CXRAGC_State::BuildNodeTree_Recurse(int _Level, int _Offset, int _Width)
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
			int32 iOffsetAction = AG_ACTIONINDEX_NULL;

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
			if (iOffsetAction != AG_ACTIONINDEX_NULL)
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
			uint16 SortedIndex = (m_lConditions[jCondition].m_SortedIndex) & (~AGC_CONDITION_SORTEDBIT);
			if (SortedIndex == iSortedCondition)
			{
				iCondition = jCondition;
				break;
			}
		}

		if (iCondition == -1)
		{
			m_pCompiler->OnError(CStrF("Internal Error. Invalid condition %d'.", iSortedCondition));
			return AGC_NODEACTION_FAILPARSE;
		}

		int iNode = m_lConditionNodes.Add(CXRAGC_ConditionNode());

		int TrueAction = BuildNodeTree_Recurse(_Level + 1, _Offset, _Width / 2);
		int FalseAction = BuildNodeTree_Recurse(_Level + 1, _Offset + _Width / 2, _Width / 2);

		CXRAGC_Condition* pCondition = &(m_lConditions[iCondition]);
		CXRAGC_ConditionNode* pNode = &(m_lConditionNodes[iNode]);
		pNode->m_iProperty = pCondition->m_iProperty;
		pCondition->m_lPropertyParams.Duplicate(&pNode->m_lPropertyParams);
		pNode->m_iOperator = pCondition->m_iOperator;
		pNode->m_Constant = pCondition->m_ConstantValue;
		pNode->m_TrueAction = TrueAction;
		pNode->m_FalseAction = FalseAction;

/*
		if ((iNode & ~AG_NODEACTION_NODEMASK) != 0)
		{
			m_pCompiler->OnError(CStrF("Internal Error. Node index %d is out of range (%s).", iNode, (char*)m_Name));
			return AG_NODEACTION_FAILPARSE;
		}
*/

		return (iNode | AGC_NODEACTION_PARSENODE);
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
			int32 iOffsetAction = AG_ACTIONINDEX_NULL; // This is an invalid action, meaning parsing failed.

			for (int iAction = 0; iAction < m_lActions.Len(); iAction++)
			{
				if ((OffsetBits & m_lActions[iAction].m_ConditionBits) == m_lActions[iAction].m_ConditionBits)
				{
					iOffsetAction = iAction;
					break;
				}
			}

			if (iOffsetAction == AG_ACTIONINDEX_NULL)
				return AGC_NODEACTION_FAILPARSE;

			if ((iOffsetAction & ~AGC_NODEACTION_ACTIONMASK) != 0)
			{
				m_pCompiler->OnError(CStrF("Internal Error. Action index %d is out of range (%s).", iOffsetAction, (char*)m_Name));
				return AGC_NODEACTION_FAILPARSE;
			}

			return (iOffsetAction | AGC_NODEACTION_ENDPARSE);
		}
	}
}

//--------------------------------------------------------------------------------

void CXRAGC_State::OptimizeNodeTree()
{
	bool bDone = false;
	bool bMerged = true;
	while (!bDone)
	{
		bDone = true;
		for (int iNodeA = 0; iNodeA < (m_lConditionNodes.Len() - 1); iNodeA++)
		{
			CXRAGC_ConditionNode* pNodeA = &(m_lConditionNodes[iNodeA]);
			for (int iNodeB = iNodeA+1; iNodeB < m_lConditionNodes.Len(); iNodeB++)
			{
				bMerged = false;
				CXRAGC_ConditionNode* pNodeB = &(m_lConditionNodes[iNodeB]);
				if (m_lConditionNodes[iNodeA] == m_lConditionNodes[iNodeB])
				{
					bMerged = true;
					for (int iNodeC = 0; iNodeC < m_lConditionNodes.Len(); iNodeC++)
					{
						// Redirect C to lead to A instead of to B
						CXRAGC_ConditionNode* pNodeC = &(m_lConditionNodes[iNodeC]);
						int TrueAction = pNodeC->m_TrueAction;
						if ((TrueAction & AGC_NODEACTION_TYPEMASK) == AGC_NODEACTION_PARSENODE)
						{
							int iNode = (TrueAction & AGC_NODEACTION_NODEMASK);
							if (iNode == iNodeB)
								pNodeC->m_TrueAction = (iNodeA | AGC_NODEACTION_PARSENODE);
							if (iNode > iNodeB)
								pNodeC->m_TrueAction = ((iNode - 1) | AGC_NODEACTION_PARSENODE);
						}

						int FalseAction = pNodeC->m_FalseAction;
						if ((FalseAction & AGC_NODEACTION_TYPEMASK) == AGC_NODEACTION_PARSENODE)
						{
							int iNode = (FalseAction & AGC_NODEACTION_NODEMASK);
							if (iNode == iNodeB)
								pNodeC->m_FalseAction = (iNodeA | AGC_NODEACTION_PARSENODE);
							if (iNode > iNodeB)
								pNodeC->m_FalseAction = ((iNode - 1) | AGC_NODEACTION_PARSENODE);
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

bool CXRAGC_State::ConvertNodeActions()
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

bool CXRAGC_State::ConvertNodeAction(uint16& _NodeAction)
{
	uint16 NodeAction = _NodeAction;

	if (NodeAction == AGC_NODEACTION_FAILPARSE)
	{
		_NodeAction = AG_NODEACTION_FAILPARSE;
		return true;
	}
	else if ((NodeAction & AGC_NODEACTION_TYPEMASK) == AGC_NODEACTION_ENDPARSE)
	{
		int iAction = (NodeAction & AGC_NODEACTION_ACTIONMASK);
		if ((iAction & ~AG_NODEACTION_ACTIONMASK) != 0)
		{
			m_pCompiler->OnError(CStrF("Internal Error. Action index %d is out of range (%s).", iAction, (char*)m_Name));
			return false;
		}

		_NodeAction = iAction | AG_NODEACTION_ENDPARSE;
		return true;
	}
	else if ((NodeAction & AGC_NODEACTION_TYPEMASK) == AGC_NODEACTION_PARSENODE)
	{
		int iNode = (NodeAction & AGC_NODEACTION_NODEMASK);
		if ((iNode & ~AG_NODEACTION_NODEMASK) != 0)
		{
			m_pCompiler->OnError(CStrF("Internal Error. Node index %d is out of range (%s).", iNode, (char*)m_Name));
			return false;
		}

		_NodeAction = iNode | AG_NODEACTION_PARSENODE;
		return true;
	}

	return false;
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXR_AnimGraphCompiler::PushFileName(CStr _Filename)
{
	m_lFilenames.Insert(0, _Filename);
}

void CXR_AnimGraphCompiler::PopFileName()
{
	if (m_lFilenames.Len() > 0)
		m_lFilenames.Del(0);
}

CStr CXR_AnimGraphCompiler::GetFileName()
{
	if (m_lFilenames.Len() > 0)
		return m_lFilenames[0];
	else
		return "";
}

CStr CXR_AnimGraphCompiler::GetFilePath()
{
	return GetFileName().GetPath();
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraphCompiler::ParseReg(const CRegistry* _pRootReg, CXR_AnimGraph* _pAnimGraph, CStr _FileName, CStr& _DestFileName)
{
	bool bSuccess = true;
	
	if (_pRootReg == NULL)
		return false;

	if (_pAnimGraph == NULL)
		return false;

	_DestFileName = _FileName.GetPath() + _FileName.GetFilenameNoExt() + ".xag";

	m_pAnimGraph = _pAnimGraph;
	PushFileName(_FileName);
	OnNote(CStrF("Parsing file '%s'...", (char*)_FileName));

	m_Name = _FileName.GetFilenameNoExt();

	for (int iChild = 0; iChild < _pRootReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pRootReg->GetChild(iChild);
		
		if (pChildReg->GetThisName() == "NAME")
		{
			m_Name = pChildReg->GetThisValue();
		}
		else if (pChildReg->GetThisName() == "OUTPUTFILE")
		{
			_DestFileName = ResolvePath(_FileName.GetPath() + pChildReg->GetThisValue());
		}
		else if (pChildReg->GetThisName() == "DECLARATIONS")
		{
			if (!ParseDeclarationsReg(pChildReg))
			{
				bSuccess = false;
				break;
			}
		}
		else if (pChildReg->GetThisName() == "STATES")
		{
			if (!ParseStatesReg(pChildReg, false))
			{
				bSuccess = false;
				break;
			}
		}
		else if (pChildReg->GetThisName() == "EDITPROPERTIES")
		{
			// Ignore editor-only keys.
		}
		else
		{
			OnWarning(CStrF("Unrecognized keyword '%s' (root).", (char*)pChildReg->GetThisName()));
		}
	}

	if (bSuccess)
	{
		OnNote("Processing...");
		if (!Process())
			bSuccess = false;
	}

	if (bSuccess)
	{
		OnNote("Converting...");
		if (!Convert())
			bSuccess = false;
	}

	if (bSuccess)
	{

		CStr LogFileName = _FileName.GetPath() + _FileName.GetFilenameNoExt() + ".log";
		//DumpLog(LogFileName);
#ifndef M_RTM
		m_pAnimGraph->LogDump(LogFileName, AG_DUMPFLAGS_ALL);
#endif

		OnNote("Checking consistency...");
		CStr Result = m_pAnimGraph->CheckConsistency(false);
		if (Result != "")
		{
			OnError("Inconsistent AnimGraph. " + Result);
			bSuccess = false;
		}
	}

	m_lProperties.Clear();
	m_lOperators.Clear();
	m_lEffects.Clear();
	m_lpStates.Clear();

	if (bSuccess)
		OnMsg("'" + _FileName + "' Done!");
	else
		OnMsg("Failed to convert '" + _FileName + "'.");
	
	return bSuccess;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraphCompiler::ParseStatesReg(const CRegistry* _pStatesReg, bool _bIgnoreStartState)
{
	if (!_bIgnoreStartState)
		m_StartState = _pStatesReg->GetThisValue().UpperCase();

	for (int iChild = 0; iChild < _pStatesReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pStatesReg->GetChild(iChild);

		if (pChildReg->GetThisName() == "EXTERNAL")
		{
			if (!IncludeStatesFile(GetFilePath() + pChildReg->GetThisValue()))
				return false;
		}
		else
		{
			CXRAGC_State* pState = DNew(CXRAGC_State) CXRAGC_State;
			if (!pState->ParseReg(this, pChildReg))
				return false;

			m_lpStates.Add(pState);
		}
	}
	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraphCompiler::IncludeStatesFile(CStr _FileName)
{
	_FileName = ResolvePath(_FileName);
	if (_FileName == GetFileName())
	{
		OnError(CStrF("Cyclic inclusion of state file '%s'.", (char*)_FileName));
		return false;
	}

	try
	{
		CRegistry_Dynamic IncludeReg;
		IncludeReg.XRG_Read(_FileName);
		if ((IncludeReg.GetNumChildren() >= 1) &&
			(IncludeReg.GetChild(0)->GetThisName() == "ANIMGRAPH"))
		{
			bool bStatesRegFound = false;
			for (int iChild = 0; iChild < IncludeReg.GetChild(0)->GetNumChildren(); iChild++)
			{
				if (IncludeReg.GetChild(0)->GetChild(iChild)->GetThisName() == "STATES")
				{
					FILENAMESCOPE(_FileName);
					bStatesRegFound = true;
					if (!ParseStatesReg(IncludeReg.GetChild(0)->GetChild(iChild), true))
						return false;
				}
			}

			if (bStatesRegFound)
				OnNote(CStrF("States file '%s' successfully included (from '%s').", (char*)_FileName, (char*)GetFileName()));
			else
				OnWarning(CStrF("States file '%s' is an AnimGraph, but with no states.", (char*)_FileName));
		}
		else
		{
			OnError(CStrF("States file '%s' is not an AnimGraph.", (char*)_FileName));
			return false;
		}
		return true;
	}
	catch (CCException)
	{
		OnError(CStrF("States file '%s' can't be found.", (char*)_FileName));
		return false;
	}
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraphCompiler::ParseDeclarationsReg(const CRegistry* _pDeclReg)
{
	if (_pDeclReg->GetThisValue() != "")
	{
		CFStr IncludeFile = GetFilePath() + _pDeclReg->GetThisValue();
		IncludeDeclarationsFile(IncludeFile);
	}

	try
	{
		for (int iChild = 0; iChild < _pDeclReg->GetNumChildren(); iChild++)
		{
			const CRegistry* pChildReg = _pDeclReg->GetChild(iChild);
			if (pChildReg->GetThisName() == "EXTERNAL")
			{
				if (!IncludeDeclarationsFile(GetFilePath() + pChildReg->GetThisValue()))
					return false;
			}
			else if (pChildReg->GetThisName() == "PROPERTIES")
			{
				if (!ParsePropertiesReg(pChildReg))
					return false;
			}
			else if (pChildReg->GetThisName() == "OPERATORS")
			{
				if (!ParseOperatorsReg(pChildReg))
					return false;
			}
			else if (pChildReg->GetThisName() == "EFFECTS")
			{
				if (!ParseEffectsReg(pChildReg))
					return false;
			}
			else if (pChildReg->GetThisName() == "STATECONSTANTS")
			{
				if (!ParseStateConstantsReg(pChildReg))
					return false;
			}
			else if (pChildReg->GetThisName() == "STATEFLAGS")
			{
				if (!ParseStateFlagsReg(pChildReg))
					return false;
			}
			else if (pChildReg->GetThisName() == "CONSTANTS")
			{
				if (!ParseConstantsReg(pChildReg))
					return false;
			}
			else
			{
				OnWarning(CStrF("Unrecognized keyword '%s' (DECLARATIONS).", (char*)pChildReg->GetThisName()));
			}
		}
	}
	catch (CCException)
	{
		OnError("Declaration parsing failed.");
		return false;
	}

	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraphCompiler::IncludeDeclarationsFile(CStr _FileName)
{
	_FileName = ResolvePath(_FileName);
	try
	{
		CRegistry_Dynamic IncludeReg;
		IncludeReg.XRG_Read(_FileName);
		if ((IncludeReg.GetNumChildren() >= 1) &&
			(IncludeReg.GetChild(0)->GetThisName() == "ANIMGRAPH"))
		{
			bool bDeclarationsRegFound = false;
			for (int iChild = 0; iChild < IncludeReg.GetChild(0)->GetNumChildren(); iChild++)
			{
				if (IncludeReg.GetChild(0)->GetChild(iChild)->GetThisName() == "DECLARATIONS")
				{
					FILENAMESCOPE(_FileName);
					bDeclarationsRegFound = true;
					if (!ParseDeclarationsReg(IncludeReg.GetChild(0)->GetChild(iChild)))
						return false;
				}
			}

			if (bDeclarationsRegFound)
				OnNote(CStrF("Declarations file '%s' successfully included (from '%s').", (char*)_FileName, (char*)GetFileName()));
			else
				OnWarning(CStrF("Declarations file '%s' is an AnimGraph, but with no declarations.", (char*)_FileName));
		}
		else
		{
			OnError(CStrF("Declarations file '%s' is not an AnimGraph.", (char*)_FileName));
			return false;
		}
		return true;
	}
	catch (CCException)
	{
		OnError(CStrF("Declarations file '%s' can't be found.", (char*)_FileName));
		return false;
	}
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraphCompiler::ParsePropertiesReg(const CRegistry* _pPropReg)
{
	for (int iChild = 0; iChild < _pPropReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pPropReg->GetChild(iChild);
		CXRAGC_Property Property(pChildReg->GetThisName(), pChildReg->GetThisValuei());
		int iProperty = GetPropertyIndexByID(Property.m_ID);
		if (iProperty != -1)
		{
			if (Property.m_Name != m_lProperties[iProperty].m_Name)
			{
				OnWarning(CStrF("New property '%s' replaces old property '%s' (slot %d)", (char*)Property.m_Name, (char*)m_lProperties[iProperty].m_Name, Property.m_ID));
				m_lProperties.Del(iProperty);
				m_lProperties.Add(Property);
			}
		}
		else
		{
			m_lProperties.Add(Property);
		}
	}
	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraphCompiler::ParseOperatorsReg(const CRegistry* _pOpReg)
{
	for (int iChild = 0; iChild < _pOpReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pOpReg->GetChild(iChild);
		CXRAGC_Operator Operator(pChildReg->GetThisName(), pChildReg->GetThisValuei());
		int iOperator = GetOperatorIndexByID(Operator.m_ID);
		if (iOperator != -1)
		{
			if (Operator.m_Name != m_lOperators[iOperator].m_Name)
			{
				OnWarning(CStrF("New operator '%s' replaces old operator '%s' (slot %d)", (char*)Operator.m_Name, (char*)m_lOperators[iOperator].m_Name, Operator.m_ID));
				m_lOperators.Del(iOperator);
				m_lOperators.Add(Operator);
			}
		}
		else
		{
			m_lOperators.Add(Operator);
		}
	}
	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraphCompiler::ParseEffectsReg(const CRegistry* _pEffectReg)
{
	for (int iChild = 0; iChild < _pEffectReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pEffectReg->GetChild(iChild);
		CXRAGC_Effect Effect(pChildReg->GetThisName(), pChildReg->GetThisValuei());
		int iEffect = GetEffectIndexByID(Effect.m_ID);
		if (iEffect != -1)
		{
			if (Effect.m_Name != m_lEffects[iEffect].m_Name)
			{
				OnWarning(CStrF("New effect '%s' replaces old effect '%s' (slot %d)", (char*)Effect.m_Name, (char*)m_lEffects[iEffect].m_Name, Effect.m_ID));
				m_lEffects.Del(iEffect);
				m_lEffects.Add(Effect);
			}
		}
		else
		{
			m_lEffects.Add(Effect);
		}
	}
	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraphCompiler::ParseStateConstantsReg(const CRegistry* _pStateConstantsReg)
{
	for (int iChild = 0; iChild < _pStateConstantsReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pStateConstantsReg->GetChild(iChild);
		CXRAGC_StateConstant StateConstant(pChildReg->GetThisName(), pChildReg->GetThisValuei());
		int iStateConstant = GetStateConstantIndexByID(StateConstant.m_ID);
		if (iStateConstant != -1)
		{
			if (StateConstant.m_Name != m_lStateConstants[iStateConstant].m_Name)
			{
				OnWarning(CStrF("New state constant '%s' replaces old state constant '%s' (slot %d)", (char*)StateConstant.m_Name, (char*)m_lStateConstants[iStateConstant].m_Name, StateConstant.m_ID));
				m_lConstants.Del(iStateConstant);
				m_lStateConstants.Add(StateConstant);
			}
		}
		else
		{
			m_lStateConstants.Add(StateConstant);
		}
	}
	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraphCompiler::ParseConstantsReg(const CRegistry* _pConstantsReg)
{
	CStr Location = CStr("DECLARATIONS") + AGC_SCOPESEPARATOR + "CONSTANTS";
	for (int iChild = 0; iChild < _pConstantsReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pConstantsReg->GetChild(iChild);
		fp32 ConstantValue;
		if (!EvalConstantExpression(pChildReg->GetThisValue(), ConstantValue, NULL, Location))
			return false;

		m_lConstants.Add(CXRAGC_Constant(pChildReg->GetThisName(), ConstantValue));
	}
	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraphCompiler::ParseStateFlagsReg(const CRegistry* _pStateFlagsReg)
{
	for (int iChild = 0; iChild < _pStateFlagsReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pStateFlagsReg->GetChild(iChild);
		m_lStateFlags.Add(CXRAGC_StateFlag(pChildReg->GetThisName(), pChildReg->GetThisValuei()));
	}
	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraphCompiler::ParseGeneratedStateReg(const CRegistry* _pStateReg)
{
	CXRAGC_State* pState = DNew(CXRAGC_State) CXRAGC_State;
	m_lpStates.Add(pState);

	OnNote("Generating intermediate state '" + _pStateReg->GetThisName() + "'...");
	if (!pState->ParseReg(this, _pStateReg))
		return false;

	return true;
}

//--------------------------------------------------------------------------------

int32 CXR_AnimGraphCompiler::GetPropertyIndexByID(int32 _ID)
{
	for (int iProperty = 0; iProperty < m_lProperties.Len(); iProperty++)
	{
		if (m_lProperties[iProperty].m_ID == _ID)
			return iProperty;
	}
	return -1;
}

//--------------------------------------------------------------------------------

int32 CXR_AnimGraphCompiler::GetOperatorIndexByID(int32 _ID)
{
	for (int iOperator = 0; iOperator < m_lOperators.Len(); iOperator++)
	{
		if (m_lOperators[iOperator].m_ID == _ID)
			return iOperator;
	}
	return -1;
}

//--------------------------------------------------------------------------------

int32 CXR_AnimGraphCompiler::GetEffectIndexByID(int32 _ID)
{
	for (int iEffect = 0; iEffect < m_lEffects.Len(); iEffect++)
	{
		if (m_lEffects[iEffect].m_ID == _ID)
			return iEffect;
	}
	return -1;
}

//--------------------------------------------------------------------------------

int32 CXR_AnimGraphCompiler::GetStateConstantIndexByID(int32 _ID)
{
	for (int iStateConstant = 0; iStateConstant < m_lStateConstants.Len(); iStateConstant++)
	{
		if (m_lStateConstants[iStateConstant].m_ID == _ID)
			return iStateConstant;
	}
	return -1;
}

//--------------------------------------------------------------------------------

int32 CXR_AnimGraphCompiler::GetPropertyID(const char* _Name)
{
	for (int iProperty = 0; iProperty < m_lProperties.Len(); iProperty++)
	{
		if (m_lProperties[iProperty].m_Name == _Name)
			return m_lProperties[iProperty].m_ID;
	}
	return -1;
}

//--------------------------------------------------------------------------------

int32 CXR_AnimGraphCompiler::GetOperatorID(const char* _Name)
{
	for (int iOperator = 0; iOperator < m_lOperators.Len(); iOperator++)
	{
		if (m_lOperators[iOperator].m_Name == _Name)
			return m_lOperators[iOperator].m_ID;
	}
	return -1;
}

//--------------------------------------------------------------------------------

int32 CXR_AnimGraphCompiler::GetEffectID(const char* _Name)
{
	for (int iEffect = 0; iEffect < m_lEffects.Len(); iEffect++)
	{
		if (m_lEffects[iEffect].m_Name == _Name)
			return m_lEffects[iEffect].m_ID;
	}
	return -1;
}

//--------------------------------------------------------------------------------

int32 CXR_AnimGraphCompiler::GetStateConstantID(const char* _Name)
{
	for (int iStateConstant = 0; iStateConstant < m_lStateConstants.Len(); iStateConstant++)
	{
		if (m_lStateConstants[iStateConstant].m_Name == _Name)
			return m_lStateConstants[iStateConstant].m_ID;
	}
	return -1;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraphCompiler::GetConstantValue(const char* _Name, fp32& _Value)
{
	for (int iConstant = 0; iConstant < m_lConstants.Len(); iConstant++)
	{
		if (m_lConstants[iConstant].m_Name == _Name)
		{
			_Value = m_lConstants[iConstant].m_Value;
			return true;
		}
	}
	return false;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraphCompiler::EvalConstantExpression(CStr _ExprStr, fp32& _ConstantValue, CXRAGC_State* _pState, CStr _Location)
{
	CXRAGC_ConstExprParser Parser(_ExprStr, this, _pState, _Location);
	return Parser.ParseExpr(_ConstantValue);
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraphCompiler::ResolveConstantValue(CStr _ConstantStr, fp32& _ConstantValue, CXRAGC_State* _pState, CStr _Location)
{
	if (_ConstantStr.Left(2) == "0x")
	{
		_ConstantValue = _ConstantStr.Val_int();
		return true;
	}

	fp64 ConstantValueFP64;
	if (CStr::Val_fp64((char*)_ConstantStr, ConstantValueFP64) > 0)
	{
		_ConstantValue = ConstantValueFP64;
		return true;
	}

	if (_pState != NULL)
	{
		int32 StateConstantID = GetStateConstantID(_ConstantStr.UpperCase());
		if (StateConstantID != -1)
		{
			for (int iConstant = 0; iConstant < _pState->m_lConstants.Len(); iConstant++)
			{
				if (_pState->m_lConstants[iConstant].m_ID == StateConstantID)
				{
					_ConstantValue = _pState->m_lConstants[iConstant].m_Value;
					return true;
				}
			}
		}
	}

	if (GetConstantValue(_ConstantStr.UpperCase(), _ConstantValue))
		return true;

	OnError(CStrF("Undefined/Invalid constant '%s' (%s).", (char*)_ConstantStr, (char*)_Location));
	return false;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraphCompiler::GetStateFlag(const char* _Name, uint32& _Value)
{
	for (int iStateFlag = 0; iStateFlag < m_lStateFlags.Len(); iStateFlag++)
	{
		CStr DebugTemp = m_lStateFlags[iStateFlag].m_Name;
		if (DebugTemp == _Name)
		{
			_Value = m_lStateFlags[iStateFlag].m_Value;
			return true;
		}
	}
	return false;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraphCompiler::ParseStateFlags(CStr _FlagsStr, uint32& _Flags, uint32& _FlagsMask, CStr _Location)
{
	uint32 ResultValue = 0;
	do
	{
		CStr FlagStr, Operator;
		int iOperatorPos = _FlagsStr.FindOneOf("+-");
		if (iOperatorPos == -1)
		{
			FlagStr = _FlagsStr.UpperCase();
			Operator = "";
			_FlagsStr = "";
		}
		else
		{
			FlagStr = _FlagsStr.Left(iOperatorPos).UpperCase();
			Operator = _FlagsStr.SubStr(iOperatorPos, 1);
			_FlagsStr = _FlagsStr.DelTo(iOperatorPos);
		}
		FlagStr.Trim();

		uint32 FlagValue;
		if (!GetStateFlag(FlagStr, FlagValue))
		{
			OnError(CStrF("Undefined stateflag '%s' (%s)", (char*)FlagStr, (char*)_Location));
			return false;
		}
		else
		{
			if ((Operator == "+") || (Operator == ""))
			{
				_Flags |= FlagValue;
				_FlagsMask |= FlagValue;
			}
			else if (Operator == "-")
			{
				_FlagsMask |= FlagValue;
			}
			else
				OnError(CStrF("Invalid symbol '%s' in stateflag syntax (%s).", (char*)Operator, (char*)_Location));
		}
	}
	while (_FlagsStr != "");

	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraphCompiler::Process()
{
	if (m_lpStates.Len() == 0)
	{
		if ((m_lProperties.Len() == 0) && 
			(m_lOperators.Len() == 0) && 
			(m_lEffects.Len() == 0) && 
			(m_lConstants.Len() == 0) && 
			(m_lStateFlags.Len() == 0))
			OnError("AnimGraph contains no states nor declarations.");
		else
			OnError("AnimGraph contains no states (It's probably an AnimGraph declaration file).");
		return false;
	}
	
	// Resolve startstate.
	if (m_StartState == "")
	{
		OnError("Starting state not specified.");
		return false;
	}
	else
	{
		m_iStartState = AG_STATEINDEX_NULL;
		for (int iState = 0; iState < m_lpStates.Len(); iState++)
		{
			if (m_StartState == m_lpStates[iState]->m_Name)
			{
				m_iStartState = iState;
				m_lpStates[iState]->m_References++;
			}
		}

		if (m_iStartState == AG_STATEINDEX_NULL)
		{
			OnError("Specified starting state '" + m_StartState + "' is not defined.");
			return false;
		}
	}

	int iState;

	for (iState = 0; iState < m_lpStates.Len(); iState++)
	{
		if (!m_lpStates[iState]->Process())
			return false;
	}

	// Remove states with no references to them (i.e. not a targetstate/startstate).
	{
		int jState = 0;
		// Build remap indices.
		for (iState = 0; iState < m_lpStates.Len(); iState++)
		{
			if (m_lpStates[iState]->m_References == 0)
			{
				m_lpStates[iState]->m_Index = AG_STATEINDEX_NULL;
			}
			else
			{
				m_lpStates[iState]->m_Index = jState;
				jState++;
			}
		}

		// Remap states.
		for (iState = 0; iState < m_lpStates.Len(); iState++)
		{
			for (int iAction = 0; iAction < m_lpStates[iState]->m_lActions.Len(); iAction++)
			{
				int OldIndex = m_lpStates[iState]->m_lActions[iAction].m_iTargetState;

				if ((OldIndex != AG_STATEINDEX_NULL) && (OldIndex != AG_STATEINDEX_TERMINATE))
				{
					int NewIndex = m_lpStates[OldIndex]->m_Index;
					m_lpStates[iState]->m_lActions[iAction].m_iTargetState = NewIndex;
				}
			}
		}

		// Remap startstate.
		m_iStartState = m_lpStates[m_iStartState]->m_Index;

		// Delete unref'ed states.
		for (iState = 0; iState < m_lpStates.Len();)
		{
			if (m_lpStates[iState]->m_References == 0)
			{
				m_lpStates.Del(iState);
			}
			else
			{
				iState++;
			}
		}
	}

	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraphCompiler::Convert()
{
	// Convert into CXR_AnimGraph classes...

	int nTotalStates = 0;
	int nTotalAnimLayers = 0;
	int nTotalActions = 1; // Count start action.
	int nTotalCallbackParams = 0;
	int nTotalNodes = 0;
	int nTotalEffectInstances = 0;
	int nTotalStateConstants = 0;

	int iState;

	for (iState = 0; iState < m_lpStates.Len(); iState++)
	{
		CXRAGC_State* pState = (m_lpStates[iState]);
		nTotalStates++;
		nTotalAnimLayers += m_lpStates[iState]->m_lStateAnims.Len();
		nTotalActions += pState->m_lActions.Len();
		nTotalNodes += pState->m_lConditionNodes.Len();
		nTotalEffectInstances += pState->m_lEffectInstances.Len();
		nTotalStateConstants += pState->m_lConstants.Len();

		for (int iCNode = 0; iCNode < pState->m_lConditionNodes.Len(); iCNode++)
			nTotalCallbackParams += pState->m_lConditionNodes[iCNode].m_lPropertyParams.Len();

		for (int iEffectInstance = 0; iEffectInstance < pState->m_lEffectInstances.Len(); iEffectInstance++)
		{
			CXRAGC_EffectInstance* pEI = &(pState->m_lEffectInstances[iEffectInstance]);
			nTotalCallbackParams += pEI->m_lParams.Len();
		}
	}

	m_pAnimGraph->m_Name = m_Name;

	m_pAnimGraph->m_lFullStates.SetLen(nTotalStates);
	m_pAnimGraph->m_lFullAnimLayers.SetLen(nTotalAnimLayers);
	m_pAnimGraph->m_lAnimLayersMap.SetLen(nTotalAnimLayers);
	m_pAnimGraph->m_lFullActions.SetLen(nTotalActions);
	m_pAnimGraph->m_lActionsMap.SetLen(nTotalActions);
	m_pAnimGraph->m_lNodes.SetLen(nTotalNodes);
	m_pAnimGraph->m_lEffectInstances.SetLen(nTotalEffectInstances);
	m_pAnimGraph->m_lStateConstants.SetLen(nTotalStateConstants);
	m_pAnimGraph->m_lActionHashEntries.SetLen(nTotalActions);
	m_pAnimGraph->m_lCallbackParams.SetLen(nTotalCallbackParams);


	m_lActionHashNames.SetLen(nTotalActions);

#ifndef M_RTM
	m_pAnimGraph->m_lExportedActionNames.SetLen(nTotalActions);
	m_pAnimGraph->m_lExportedStateNames.SetLen(nTotalStates);
	// Exported Names
	{
		m_pAnimGraph->m_lExportedPropertyNames.SetLen(m_lProperties.Len());
		for (int iProperty = 0; iProperty < m_lProperties.Len(); iProperty++)
			m_pAnimGraph->m_lExportedPropertyNames[iProperty] = CXRAG_NameAndID(m_lProperties[iProperty].m_Name, m_lProperties[iProperty].m_ID);

		m_pAnimGraph->m_lExportedOperatorNames.SetLen(m_lOperators.Len());
		for (int iOperator = 0; iOperator < m_lOperators.Len(); iOperator++)
			m_pAnimGraph->m_lExportedOperatorNames[iOperator] = CXRAG_NameAndID(m_lOperators[iOperator].m_Name, m_lOperators[iOperator].m_ID);

		m_pAnimGraph->m_lExportedEffectNames.SetLen(m_lEffects.Len());
		for (int iEffect = 0; iEffect < m_lEffects.Len(); iEffect++)
			m_pAnimGraph->m_lExportedEffectNames[iEffect] = CXRAG_NameAndID(m_lEffects[iEffect].m_Name, m_lEffects[iEffect].m_ID);

		m_pAnimGraph->m_lExportedStateConstantNames.SetLen(m_lStateConstants.Len());
		for (int iStateConstant = 0; iStateConstant < m_lStateConstants.Len(); iStateConstant++)
			m_pAnimGraph->m_lExportedStateConstantNames[iStateConstant] = CXRAG_NameAndID(m_lStateConstants[iStateConstant].m_Name, m_lStateConstants[iStateConstant].m_ID);
	}
#endif

	int iAction = 0;
	int iAnimLayer = 0;
	int iNode = 0;
	int iEffectInstance = 0;
	int iCallbackParam = 0;
	int iStateConstant = 0;
	int iExportedActionName = 0;
	int iExportedStateName = 0;

	// Add start action.
	m_pAnimGraph->m_lFullActions[iAction].m_iTargetState = m_iStartState;
	m_pAnimGraph->m_lActionsMap[iAction] = iAction;

	CStr ActionName = "STARTACTION";
	uint32 ActionHashKey = StringToHash(ActionName);
	CXRAG_ActionHashEntry ActionHashEntry(ActionHashKey, iAction);
	m_lActionHashNames[iAction] = ActionName;
	m_pAnimGraph->m_lActionHashEntries[iAction] = ActionHashEntry;
#ifndef M_RTM
	m_pAnimGraph->m_lExportedActionNames[iExportedActionName++] = ActionName;
#endif

	iAction++;

	int iStateEffectInstanceBase = 0;
	for (iState = 0; iState < m_lpStates.Len(); iState++)
	{
#ifndef M_RTM
		m_pAnimGraph->m_lExportedStateNames[iExportedStateName++] = m_lpStates[iState]->m_Name;
#endif

		// Misc
		CXRAG_State_Full* pState = &(m_pAnimGraph->m_lFullStates[iState]);
		pState->m_lFlags[0] = m_lpStates[iState]->m_Flags[0];
		pState->m_lFlags[1] = m_lpStates[iState]->m_Flags[1];
		pState->m_Priority = m_lpStates[iState]->m_Priority;

		// Animation
		int32 Len = m_lpStates[iState]->m_lStateAnims.Len();
		m_pAnimGraph->m_lFullStates[iState].m_iBaseAnimLayer = iAnimLayer;
		for (int32 i = 0; i < Len; i++)
		{
			CXRAG_AnimLayer_Full* pAnimLayer = &(m_pAnimGraph->m_lFullAnimLayers[iAnimLayer]);
			pAnimLayer->m_iAnim = m_lpStates[iState]->m_lStateAnims[i].m_iAnim;
			pAnimLayer->m_iBaseJoint = m_lpStates[iState]->m_lStateAnims[i].m_iBaseJoint;
			pAnimLayer->m_TimeOffset = m_lpStates[iState]->m_lStateAnims[i].m_TimeOffset;
			pAnimLayer->m_TimeScale = m_lpStates[iState]->m_lStateAnims[i].m_TimeScale;
			pAnimLayer->m_Opacity = m_lpStates[iState]->m_lStateAnims[i].m_Opacity;
			pAnimLayer->m_iTimeControlProperty = 0;

			m_pAnimGraph->m_lAnimLayersMap[iAnimLayer] = iAnimLayer;
			iAnimLayer++;
		}
		// Set num anim layers
		m_pAnimGraph->m_lFullStates[iState].m_nAnimLayers = iAnimLayer - m_pAnimGraph->m_lFullStates[iState].m_iBaseAnimLayer;

		// Actions
		m_pAnimGraph->m_lFullStates[iState].m_iBaseAction = iAction;
		for (int jAction = 0; jAction < m_lpStates[iState]->m_lActions.Len(); jAction++, iAction++)
		{
			CXRAGC_Action* pAction = &(m_lpStates[iState]->m_lActions[jAction]);

			CStr ActionName = m_lpStates[iState]->m_Name + "." + pAction->m_Name;
			uint32 ActionHashKey = StringToHash(ActionName);
			CXRAG_ActionHashEntry ActionHashEntry(ActionHashKey, iAction);
			m_lActionHashNames[iAction] = ActionName;
			m_pAnimGraph->m_lActionHashEntries[iAction] = ActionHashEntry;
#ifndef M_RTM
			m_pAnimGraph->m_lExportedActionNames[iExportedActionName++] = ActionName;
#endif

			m_pAnimGraph->m_lFullActions[iAction].m_TokenID = pAction->m_TokenID;
			m_pAnimGraph->m_lFullActions[iAction].m_iTargetState = pAction->m_iTargetState;
			m_pAnimGraph->m_lFullActions[iAction].m_AnimBlendDuration = pAction->m_AnimBlendDuration;
			m_pAnimGraph->m_lFullActions[iAction].m_AnimBlendDelay = pAction->m_AnimBlendDelay;
			m_pAnimGraph->m_lFullActions[iAction].m_AnimTimeOffset = pAction->m_AnimTimeOffset;
			m_pAnimGraph->m_lFullActions[iAction].m_iAnimTimeOffsetType = pAction->m_iAnimTimeOffsetType;
			m_pAnimGraph->m_lActionsMap[iAction] = iAction;

			// EffectInstances
			int iBaseEffectInstance = m_lpStates[iState]->m_lActions[jAction].m_iBaseEffectInstance;
			int nEffectInstances = m_lpStates[iState]->m_lActions[jAction].m_nEffectInstances;
			m_pAnimGraph->m_lFullActions[iAction].m_iBaseEffectInstance = iStateEffectInstanceBase + iBaseEffectInstance;
			m_pAnimGraph->m_lFullActions[iAction].m_nEffectInstances = m_lpStates[iState]->m_lActions[jAction].m_nEffectInstances;
			for (int jEffectInstance = 0; jEffectInstance < nEffectInstances; jEffectInstance++)
			{
				int iAGEffectInstance = iStateEffectInstanceBase + iBaseEffectInstance + jEffectInstance;
				CXRAGC_EffectInstance* pIMEffectInstance = &(m_lpStates[iState]->m_lEffectInstances[iBaseEffectInstance + jEffectInstance]);
				CXRAG_EffectInstance* pAGEffectInstance = &(m_pAnimGraph->m_lEffectInstances[iAGEffectInstance]);
				if (pAGEffectInstance->m_ID == AG_EFFECTID_NULL)
				{
					pAGEffectInstance->m_ID = pIMEffectInstance->m_ID;
					pAGEffectInstance->m_iParams = iCallbackParam;
					int nParams = pIMEffectInstance->m_lParams.Len(); pAGEffectInstance->m_nParams = nParams;
					for (int iParam = 0; iParam < nParams; iParam++)
					{
						fp32 Param = pIMEffectInstance->m_lParams[iParam];
						m_pAnimGraph->m_lCallbackParams[iCallbackParam + iParam] = Param;
					}
					iCallbackParam += nParams;
					iEffectInstance++;
				}
			}
		}

		iStateEffectInstanceBase += m_lpStates[iState]->m_lEffectInstances.Len();

		// Nodes
		if (m_lpStates[iState]->m_lConditionNodes.Len() > 0)
		{
			uint8 iStatePropertyParams = 0;
			m_pAnimGraph->m_lFullStates[iState].m_iBasePropertyParam = iCallbackParam;
			m_pAnimGraph->m_lFullStates[iState].m_iBaseNode = iNode;
			for (int jNode = 0; jNode < m_lpStates[iState]->m_lConditionNodes.Len(); jNode++, iNode++)
			{
				CXRAG_ConditionNode* pAGNode = &(m_pAnimGraph->m_lNodes[iNode]);
				CXRAGC_ConditionNode* pIMNode = &(m_lpStates[iState]->m_lConditionNodes[jNode]);

				int nParams = pIMNode->m_lPropertyParams.Len();
				pAGNode->SetPropertyParamsIndex(iStatePropertyParams);
				pAGNode->SetNumPropertyParams(nParams);
				for (int iParam = 0; iParam < nParams; iParam++)
				{
					fp32 Param = pIMNode->m_lPropertyParams[iParam];
					m_pAnimGraph->m_lCallbackParams[iCallbackParam + iParam] = Param;
				}
				iCallbackParam += pIMNode->m_lPropertyParams.Len();
				iStatePropertyParams += pIMNode->m_lPropertyParams.Len();

				pAGNode->SetProperty(pIMNode->m_iProperty);
				pAGNode->SetOperator(pIMNode->m_iOperator);
				pAGNode->SetConstant(pIMNode->m_Constant);
				pAGNode->SetTrueAction(pIMNode->m_TrueAction);
				pAGNode->SetFalseAction(pIMNode->m_FalseAction);
			}
		}
		else
		{
			m_pAnimGraph->m_lFullStates[iState].m_iBasePropertyParam = AG_CALLBACKPARAMINDEX_NULL;
			m_pAnimGraph->m_lFullStates[iState].m_iBaseNode = AG_NODEINDEX_NULL;
		}

		// StateConstants
		m_pAnimGraph->m_lFullStates[iState].m_iBaseConstant = iStateConstant;
		m_pAnimGraph->m_lFullStates[iState].m_nConstants = m_lpStates[iState]->m_lConstants.Len();
		for (int jConstant = 0; jConstant < m_lpStates[iState]->m_lConstants.Len(); jConstant++, iStateConstant++)
		{
			m_pAnimGraph->m_lStateConstants[iStateConstant].m_ID = m_lpStates[iState]->m_lConstants[jConstant].m_ID;
			m_pAnimGraph->m_lStateConstants[iStateConstant].m_Value = m_lpStates[iState]->m_lConstants[jConstant].m_Value;
		}
	}

	return true;
}

//--------------------------------------------------------------------------------

bool CXR_AnimGraphCompiler::ParseParamsStr(CStr _ParamsStr, TArray<fp32>& _lParams, CStr _Location)
{
	while (_ParamsStr != "")
	{
		CStr ParamStr = _ParamsStr.GetStrSep(",");
		fp32 ParamValue = 0;
		if (!EvalConstantExpression(ParamStr, ParamValue, NULL, _Location))
			return false;

		_lParams.Add(ParamValue);
	}
	return true;
}

//--------------------------------------------------------------------------------

void CXR_AnimGraphCompiler::OnError(const char* _pMsg)
{
	ConOutL(CStrF("(AnimGraphCompiler) ERROR: %s", _pMsg));
	//Error("AnimGraphCompiler", _pMsg);
}

void CXR_AnimGraphCompiler::OnWarning(const char* _pMsg)
{
	ConOutL(CStrF("(AnimGraphCompiler) Warning: %s", _pMsg));
}

void CXR_AnimGraphCompiler::OnNote(const char* _pMsg)
{
	ConOutL(CStrF("(AnimGraphCompiler) Note: %s", _pMsg));
}

void CXR_AnimGraphCompiler::OnMsg(const char* _pMsg)
{
	ConOutL(CStrF("(AnimGraphCompiler) %s", _pMsg));
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXR_AnimGraphCompiler::DumpLog(CStr _LogFileName)
{
	try
	{
		CCFile File;
		File.Open(_LogFileName, CFILE_WRITE);

		File.Writeln(CStrF("AnimGraph: %s", (char*)m_pAnimGraph->m_Name));
		File.Writeln(CStrF("nStates: %d", m_pAnimGraph->m_lFullStates.Len()));
		File.Writeln(CStrF("nNodes: %d", m_pAnimGraph->m_lNodes.Len()));
		File.Writeln(CStrF("nActions: %d", m_pAnimGraph->m_lFullActions.Len()));
		File.Writeln(CStrF("StartState: %d", m_iStartState));
		File.Writeln("");

		OnNote(CStrF("Dumping AnimGraph to logfile '%s'", (char*)_LogFileName));

		for (int iState = 0; iState < m_pAnimGraph->m_lFullStates.Len(); iState++)
			DumpLog_State(&File, iState);

/*
		if (m_pAnimGraph->m_lAnimLayersMap.Len() > 0)
		{
			File.Writeln("");
			File.Writeln("AnimLayersMap:");

			for (int jAnimLayer = 0; jAnimLayer < m_pAnimGraph->m_lAnimLayersMap.Len(); jAnimLayer++)
			{
			int iAnimLayer = m_pAnimGraph->m_lAnimLayersMap[jAnimLayer];
			File.Writeln(CStrF("  %d -> %d", jAnimLayer, iAnimLayer));
			}
		}
*/

		if (m_pAnimGraph->m_lFullAnimLayers.Len() > 0)
		{
			File.Writeln("");
			File.Writeln("AnimLayers:");

			int nAnimLayers = m_pAnimGraph->GetNumAnimLayers();
			for (int iAnimLayer = 0; iAnimLayer < nAnimLayers; iAnimLayer++)
			{
				const CXRAG_AnimLayer* pAnimLayer = m_pAnimGraph->GetAnimLayer(iAnimLayer);
				File.Writeln(CStrF("  %d: iAnim %d", iAnimLayer, pAnimLayer->GetAnimIndex()));
			}
		}

		if (m_pAnimGraph->m_lActionHashEntries.Len() > 0)
		{
			File.Writeln("");
			File.Writeln("ActionHash:");

			for (int iAction = 0; iAction < m_pAnimGraph->m_lActionHashEntries.Len(); iAction++)
			{
				File.Writeln(CStrF("Action %d: Name '%s', HashKey %X", 
								  m_pAnimGraph->m_lActionHashEntries[iAction].m_iAction,
//								  (char*)m_lActionHashNames[iAction], 
#ifndef M_RTM
								  (char*)m_pAnimGraph->m_lExportedActionNames[iAction], 
#endif
								  m_pAnimGraph->m_lActionHashEntries[iAction].m_HashKey));
			}
		}

		File.Writeln("");
		File.Writeln("<EndOfLog>");
		File.Close();
	}
	catch (...)
	{
		OnWarning(CStrF("Failed to dump AnimGraph to logfile '%s'.", (char*)_LogFileName));
	}
}

//--------------------------------------------------------------------------------

void CXR_AnimGraphCompiler::DumpLog_State(CCFile* _pFile, int _iState)
{
	CXRAG_State* pState = &(m_pAnimGraph->m_lFullStates[_iState]);
	_pFile->Writeln(CStrF("State%d (%s)", _iState, (char*)m_lpStates[_iState]->m_Name));
	_pFile->Writeln(CStr("{"));
	_pFile->Writeln(CStrF("  Priority: %d", pState->GetPriority()));

	const CXRAG_AnimLayer* pAnimLayer = m_pAnimGraph->GetAnimLayer(pState->GetBaseAnimLayerIndex());
	if ((pAnimLayer != NULL) && (pAnimLayer->GetAnimIndex() != -1))
		_pFile->Writeln(CStrF("  Animation: %d", pAnimLayer->GetAnimIndex()));

	_pFile->Writeln(CStrF("  iBaseAction: %d", pState->GetBaseActionIndex()));
	_pFile->Writeln(CStrF("  nActions: %d", m_lpStates[_iState]->m_lActions.Len()));

	if (pState->GetNumConstants() > 0)
	{
		_pFile->Writeln(CStr(""));
		_pFile->Writeln(CStrF("  StateConstants: (iBaseConstant=%d)", pState->GetBaseConstantIndex()));
		for (int jStateConstant = 0; jStateConstant < pState->GetNumConstants(); jStateConstant++)
		{
			int iStateConstant = pState->GetBaseConstantIndex() + jStateConstant;
			_pFile->Writeln(CStrF("    %s = %3.3f", (char*)m_lpStates[_iState]->m_lConstants[jStateConstant].m_Name, 
												   m_pAnimGraph->m_lStateConstants[iStateConstant].m_Value));
		}
	}

	if (m_lpStates[_iState]->m_lConditions.Len() > 0)
	{
		_pFile->Writeln(CStr(""));
		_pFile->Writeln(CStr("  ConditionPrio:"));
		for (int jCondition = 0; jCondition < m_lpStates[_iState]->m_lConditions.Len(); jCondition++)
		{
			for (int iCondition = 0; iCondition < m_lpStates[_iState]->m_lConditions.Len(); iCondition++)
			{
				if ((m_lpStates[_iState]->m_lConditions[iCondition].m_SortedIndex & ~AGC_CONDITION_SORTEDBIT) == jCondition)
				{
					CXRAG_ICallbackParams IParams(m_lpStates[_iState]->m_lConditions[iCondition].m_lPropertyParams.GetBasePtr(), m_lpStates[_iState]->m_lConditions[iCondition].m_lPropertyParams.Len());
					_pFile->Writeln(CStrF("    %s", 
										 (char*)DumpLog_GetConditionStr(m_lpStates[_iState]->m_lConditions[iCondition].m_iProperty, &IParams,
																		m_lpStates[_iState]->m_lConditions[iCondition].m_iOperator, 
																		m_lpStates[_iState]->m_lConditions[iCondition].m_ConstantValue)));
					break;
				}
			}
		}
	}

	if (pState->GetBaseNodeIndex() != AG_NODEINDEX_NULL)
	{
		_pFile->Writeln(CStr(""));
		_pFile->Writeln(CStrF("  NodeTree: (iBaseNode=%d, nNodes=%d)", pState->GetBaseNodeIndex(), m_lpStates[_iState]->m_lConditionNodes.Len()));
		DumpLog_Node(_pFile, pState->GetBaseActionIndex(), pState->GetBaseNodeIndex(), pState->GetBasePropertyParamIndex(), 0, "    ", "");
	}

	_pFile->Writeln(CStr("}"));
	_pFile->Writeln(CStr(""));
}

//--------------------------------------------------------------------------------

void CXR_AnimGraphCompiler::DumpLog_Node(CCFile* _pFile, int _iBaseAction, int _iBaseNode, int _iStatePropertyParamsBase, int _iNode, CStr _Indent, CStr _BranchSymbol)
{
	const char* Indent = ((char*)_Indent != NULL) ? (char*)_Indent : "";
	const char* BranchSymbol = ((char*)_BranchSymbol != NULL) ? (char*)_BranchSymbol : "";
	CXRAG_ConditionNode* pNode = &(m_pAnimGraph->m_lNodes[_iBaseNode + _iNode]);
	_pFile->Writeln(CStrF("%s%s%s", (char*)Indent, (char*)BranchSymbol, (char*)DumpLog_GetNodeStr(_iBaseAction, _iBaseNode, _iStatePropertyParamsBase, _iNode)));

	if (_BranchSymbol != "")
	{
		if (_BranchSymbol == " |-")
			_Indent += " | ";
		else
			_Indent += "   ";
	}

	DumpLog_NodeAction(_pFile, _iBaseAction, pNode->GetTrueAction(), _iBaseNode, _iStatePropertyParamsBase, _iNode, _Indent , " |-");
	DumpLog_NodeAction(_pFile, _iBaseAction, pNode->GetFalseAction(), _iBaseNode, _iStatePropertyParamsBase, _iNode, _Indent, " '-");
}

//--------------------------------------------------------------------------------

void CXR_AnimGraphCompiler::DumpLog_NodeAction(CCFile* _pFile, int _iBaseAction, int _NodeAction, int _iBaseNode, int _iStatePropertyParamsBase, int _iNode, CStr _Indent, CStr _BranchSymbol)
{
	if (_NodeAction == AG_NODEACTION_FAILPARSE)
	{
		_pFile->Writeln(CStrF("%s%s[FAIL]", (char*)_Indent, (char*)_BranchSymbol));
		return;
	}

	if ((_NodeAction & AG_NODEACTION_TYPEMASK) == AG_NODEACTION_ENDPARSE)
	{
		int16 iAction = _iBaseAction + (_NodeAction & AG_NODEACTION_ACTIONMASK);
		int8 TokenID = m_pAnimGraph->m_lFullActions[iAction].GetTokenID();
		int16 iTargetState = m_pAnimGraph->m_lFullActions[iAction].GetTargetStateIndex();
		if (iTargetState != AG_STATEINDEX_NULL)
		{
			CStr TokenName;
			if (TokenID == AG_TOKENID_NULL)
				TokenName = "CurToken";
			else
				TokenName = CStrF("Token%d", TokenID);

			if (iTargetState != AG_STATEINDEX_TERMINATE)
				_pFile->Writeln(CStrF("%s%s[A%03d]  %s -> State%d (%s)", (char*)_Indent, (char*)_BranchSymbol, iAction, (char*)TokenName, iTargetState, (char*)m_lpStates[iTargetState]->m_Name));
			else
				_pFile->Writeln(CStrF("%s%s[A%03d]  %s -> %s", (char*)_Indent, (char*)_BranchSymbol, iAction, (char*)TokenName, "TERMINATE"));
		}
		else
			_pFile->Writeln(CStrF("%s%s[A%03d]", (char*)_Indent, (char*)_BranchSymbol, iAction));
		
		if (m_pAnimGraph->m_lFullActions[iAction].GetNumEffectInstances() > 0)
		{
			CStr EffectIndent = _Indent;
			if (_BranchSymbol != "")
			{
				if (_BranchSymbol == " |-")
					EffectIndent += " | ";
				else
					EffectIndent += "   ";
			}

			int nEffectInstances = m_pAnimGraph->m_lFullActions[iAction].GetNumEffectInstances();
			for (int jEffectInstance = 0; jEffectInstance < nEffectInstances; jEffectInstance++)
			{
				int iEffectInstance = m_pAnimGraph->m_lFullActions[iAction].GetBaseEffectInstanceIndex() + jEffectInstance;
				int EffectID = m_pAnimGraph->m_lEffectInstances[iEffectInstance].m_ID;
				CStr EffectName = m_lEffects[GetEffectIndexByID(EffectID)].m_Name;
				CXRAG_ICallbackParams Params = m_pAnimGraph->GetICallbackParams(m_pAnimGraph->m_lEffectInstances[iEffectInstance].m_iParams, m_pAnimGraph->m_lEffectInstances[iEffectInstance].m_nParams);
				CStr EffectParamsStr = Params.GetStr();
				if (jEffectInstance < (nEffectInstances - 1))
					_pFile->Writeln(CStrF("%s |-[E%03d]  %s(%s)", (char*)EffectIndent, iEffectInstance, (char*)EffectName, (char*)EffectParamsStr));
				else
					_pFile->Writeln(CStrF("%s '-[E%03d]  %s(%s)", (char*)EffectIndent, iEffectInstance, (char*)EffectName, (char*)EffectParamsStr));
			}
		}

		return;
	}

	if ((_NodeAction & AG_NODEACTION_TYPEMASK) == AG_NODEACTION_PARSENODE)
		DumpLog_Node(_pFile, _iBaseAction, _iBaseNode, _iStatePropertyParamsBase, (_NodeAction & AG_NODEACTION_NODEMASK), _Indent, _BranchSymbol);
}

//--------------------------------------------------------------------------------

CStr CXR_AnimGraphCompiler::DumpLog_GetNodeStr(int _iBaseAction, int _iBaseNode, int _iStatePropertyParamsBase, int _iNode)
{
	CXRAG_ConditionNode* pNode = &(m_pAnimGraph->m_lNodes[_iBaseNode + _iNode]);
	CXRAG_ICallbackParams IParams = m_pAnimGraph->GetICallbackParams(_iStatePropertyParamsBase + pNode->GetPropertyParamsIndex(), pNode->GetNumPropertyParams());
	return CStrF("[N%03d]  %s  [%s | %s]", _iBaseNode + _iNode, 
				(char*)DumpLog_GetConditionStr(pNode->GetProperty(), &IParams, pNode->GetOperator(), pNode->GetConstant()),
				(char*)(DumpLog_GetNodeActionStr(_iBaseAction, _iBaseNode, pNode->GetTrueAction())), 
				(char*)(DumpLog_GetNodeActionStr(_iBaseAction, _iBaseNode, pNode->GetFalseAction())));
}

//--------------------------------------------------------------------------------

CStr CXR_AnimGraphCompiler::DumpLog_GetConditionStr(int _PropertyID, CXRAG_ICallbackParams* _pIParams, int _OperatorID, fp32 _Constant)
{
	CStr PropertyParamStr;
	if (_pIParams->GetNumParams() > 0)
	{
		for (int iParam = 0; iParam < _pIParams->GetNumParams(); iParam++)
		{
			PropertyParamStr += CStrF("%.2f", _pIParams->GetParam(iParam));
			if (iParam < (_pIParams->GetNumParams() - 1))
				PropertyParamStr += ", ";
		}
	}

	if (PropertyParamStr != "")
	{
		return CStrF("(%s(%s) %s %.2f)", 
			(char*)(m_lProperties[GetPropertyIndexByID(_PropertyID)].m_Name), (char*)PropertyParamStr,
			(char*)(m_lOperators[GetOperatorIndexByID(_OperatorID)].m_Name),
			_Constant);
	}
	else
	{
		return CStrF("(%s %s %.2f)", 
			(char*)(m_lProperties[GetPropertyIndexByID(_PropertyID)].m_Name),
			(char*)(m_lOperators[GetOperatorIndexByID(_OperatorID)].m_Name),
			_Constant);
	}

}

//--------------------------------------------------------------------------------

CStr CXR_AnimGraphCompiler::DumpLog_GetNodeActionStr(int _iBaseAction, int _iBaseNode, int _NodeAction)
{
	if (_NodeAction == AG_NODEACTION_FAILPARSE)
		return "FAIL";

	if ((_NodeAction & AG_NODEACTION_TYPEMASK) == AG_NODEACTION_ENDPARSE)
		return CStrF("A%03d", _iBaseAction + (_NodeAction & AG_NODEACTION_ACTIONMASK));

	if ((_NodeAction & AG_NODEACTION_TYPEMASK) == AG_NODEACTION_PARSENODE)
		return CStrF("N%03d", _iBaseNode + (_NodeAction & AG_NODEACTION_NODEMASK));

	return "    ";
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

char CXRAGC_ConstExprParser::NextChar()
{
	return (m_Source.Str()[0]);
}

char CXRAGC_ConstExprParser::ReadChar()
{
	char c = NextChar();
	m_Source = m_Source.Right(m_Source.Len() - 1);
	return c;
}

void CXRAGC_ConstExprParser::SkipWhitespace()
{
	while (NextChar() == ' ')
		ReadChar();
}

bool CXRAGC_ConstExprParser::IsDigit(char c)
{
	return ((c >= '0') && (c <= '9'));
}

bool CXRAGC_ConstExprParser::IsAlpha(char c)
{
	return (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || (c == '_'));
}

//--------------------------------------------------------------------------------

bool CXRAGC_ConstExprParser::ParseConstant(fp32& _Result)
{
	CStr ConstantStr;
	while (IsDigit(NextChar()) || (NextChar() == '.') || (NextChar() == '-') || (NextChar() == 'x'))
		ConstantStr += CStr(ReadChar());

	return m_pCompiler->ResolveConstantValue(ConstantStr, _Result, m_pState, m_Location);
}

//--------------------------------------------------------------------------------

bool CXRAGC_ConstExprParser::ParseIdentifier(fp32& _Result)
{
	CStr IdentifierStr;
	while (IsAlpha(NextChar()) || IsDigit(NextChar()))
		IdentifierStr += CStr(ReadChar());

	return m_pCompiler->ResolveConstantValue(IdentifierStr, _Result, m_pState, m_Location);
}

//--------------------------------------------------------------------------------

bool CXRAGC_ConstExprParser::ParseParantheses(fp32& _Result)
{
	ReadChar();
	SkipWhitespace();
	if (!ParseExpr(_Result))
		return false;
	SkipWhitespace();
	if (NextChar() == ')')
		ReadChar();
	else
	{
		OnError(CStr("Missing closing parantheses."));
		return false;
	}

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAGC_ConstExprParser::ParseTerm(fp32& _Result)
{
	if (NextChar() == '-')
	{
		ReadChar();
		SkipWhitespace();
		if (!ParseTerm(_Result))
			return false;

		_Result = -_Result;
		return true;
	}
	else if (NextChar() == '(')
		return ParseParantheses(_Result);
	else if (IsDigit(NextChar()))
		return ParseConstant(_Result);
	else if (IsAlpha(NextChar()))
		return ParseIdentifier(_Result);

	OnError(CStrF("Invalid character '%c'.", NextChar()));
	return false;
}

//--------------------------------------------------------------------------------

bool CXRAGC_ConstExprParser::ParseMulDivExpr(fp32& _Result, bool _bShiftResult)
{
	if (!_bShiftResult)
		if (!ParseTerm(_Result))
			return false;

	SkipWhitespace();
	if (NextChar() == '*')
	{
		ReadChar();
		SkipWhitespace();

		fp32 RightOperand;
		if (!ParseTerm(RightOperand))
			return false;

		_Result *= RightOperand;
		return ParseMulDivExpr(_Result, true);
	}
	else if (NextChar() == '/')
	{
		ReadChar();
		SkipWhitespace();
		fp32 RightOperand;
		if (!ParseTerm(RightOperand))
			return false;

		_Result /= RightOperand;
		return ParseMulDivExpr(_Result, true);
	}

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAGC_ConstExprParser::ParseAddSubExpr(fp32& _Result, bool _bShiftResult)
{
	if (!_bShiftResult)
		if (!ParseMulDivExpr(_Result, false))
			return false;

	SkipWhitespace();
	if (NextChar() == '+')
	{
		ReadChar();
		SkipWhitespace();
		fp32 RightOperand;
		if (!ParseMulDivExpr(RightOperand, false))
			return false;

		_Result += RightOperand;
		return ParseAddSubExpr(_Result, true);
	}
	else if (NextChar() == '-')
	{
		ReadChar();
		SkipWhitespace();
		fp32 RightOperand;
		if (!ParseMulDivExpr(RightOperand, false))
			return false;

		_Result -= RightOperand;
		return ParseAddSubExpr(_Result, true);
	}

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAGC_ConstExprParser::ParseExpr(fp32& _Result)
{
	if (m_Source == "")
	{
		_Result = 0;
		return true;
	}

	return ParseAddSubExpr(_Result, false);
}

//--------------------------------------------------------------------------------

void CXRAGC_ConstExprParser::OnError(CStr _Msg)
{
	ConOutL(CStrF("AGC ConstExprParser Error: %s (%s)", (char*)_Msg, (char*)m_Location));
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
