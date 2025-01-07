#include "PCH.h"

//--------------------------------------------------------------------------------

#include "../AnimGraph2Defs.h"
#include "AnimGraph2Compiler.h"

//--------------------------------------------------------------------------------

#define AG2C_SCOPESEPARATOR " . "

//--------------------------------------------------------------------------------

#define AG2C_CONDITION_SORTEDBIT		(1<<15)

extern const char* ms_lpAG2CAnimTimeOffsetTypes[];


// REACTIONS
//--------------------------------------------------------------------------------

bool CXRAG2C_Reactions::ExpandInheritance()
{
	m_bPendingExpandInheritance = true;

	// Inherit other graphblocks for now (their reaction states

	for (int iSuperReaction = 0; iSuperReaction < m_lSuperReactions.Len(); iSuperReaction++)
	{
		CFStr SuperReactionName = m_lSuperReactions[iSuperReaction];
		bool bSuperReactionFound = false;
		for (int iGraphBlock = 0; iGraphBlock < m_pAGCompiler->m_lspGraphBlocks.Len(); iGraphBlock++)
		{
			if (SuperReactionName == (m_pAGCompiler->m_lspGraphBlocks[iGraphBlock]->m_Name))
			{
				if (!Inherit(m_pAGCompiler->m_lspGraphBlocks[iGraphBlock]->m_Reactions))
					return false;
				bSuperReactionFound = true;
				break;
			}
		}
		if (!bSuperReactionFound)
		{
			m_pAGCompiler->OnError(CStrF("Undefined Reaction '%s' inherited by state '%s'.", (char*)SuperReactionName, (char*)m_Name));
			return false;
		}
	}

	m_bPendingExpandInheritance = false;
	m_bExpandedInheritance = true;

	return true;
}


//--------------------------------------------------------------------------------

// Assumes _SuperState is already fully expanded.
bool CXRAG2C_Reactions::Inherit(CXRAG2C_Reactions& _SuperReaction)
{
	if (_SuperReaction.m_bPendingExpandInheritance)
	{
		m_pAGCompiler->OnError("Cyclic inheritance between reaction '" + m_Name + "' and reaction '" + _SuperReaction.m_Name + "'.");
		return false;
	}

	if (!_SuperReaction.m_bExpandedInheritance)
		if (!_SuperReaction.ExpandInheritance())
			return false;

	int nOwnEffectInstances = m_lEffectInstances.Len();
	int nOwnReactions = m_lReactions.Len();
	//	int nSuperConditions = _SuperState.m_lConditions.Len();
	//	int nSuperActions = _SuperState.m_lActions.Len();

	// Copy EffectInstances.
	for (int jEffectInstance = 0; jEffectInstance < _SuperReaction.m_lEffectInstances.Len(); jEffectInstance++)
		m_lEffectInstances.Add(_SuperReaction.m_lEffectInstances[jEffectInstance]);

	// Copy actions and adjust conditionmap indices and effect base indices.
	int32 iReaction = nOwnReactions;
	for (int jReaction = 0; jReaction < _SuperReaction.m_lReactions.Len(); jReaction++)
	{
		CXRAG2C_Reaction InheritedReaction = _SuperReaction.m_lReactions[jReaction]; // We have to copy safely here, since Add() merely will memcpy the element's content.
		bool bOk = true;
		for (int32 i = 0; i < nOwnReactions; i++)
		{
			if (m_lReactions[i].m_Condition.m_ImpulseType == InheritedReaction.m_Condition.m_ImpulseType)
			{
				bOk = false;
				break;
			}
		}
		if (!bOk)
			continue;

		m_lReactions.Add(InheritedReaction);
		m_lReactions[iReaction].m_iBaseEffectInstance += nOwnEffectInstances;
		iReaction++;
	}

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_Reactions::ParseReg(CXRAG2C_GraphBlock* _pGraphBlock, const CRegistry* _pReactionsReg)
{
	if (_pGraphBlock == NULL)
		return false;

	if (_pReactionsReg == NULL)
		return false;

	m_pGraphBlock = _pGraphBlock;
	m_pAGCompiler = _pGraphBlock->m_pCompiler;

	m_Name = _pReactionsReg->GetThisName().UpperCase();

	CStr Location = m_Name;

	m_InheritFlags = AG2C_INHERITFLAGS_ALL;
	m_bPendingExpandInheritance = false;
	m_bExpandedInheritance = false;


	for (int iChild = 0; iChild < _pReactionsReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pReactionsReg->GetChild(iChild);

		if (pChildReg->GetThisName() == "INHERIT")
		{
			if (!ParseInheritReg(pChildReg))
				return false;
		}
		else if (pChildReg->GetThisName().Left(8) == "REACTION")
		{
			if (!ParseReactionReg(pChildReg))
				return false;
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

bool CXRAG2C_Reactions::ParseInheritReg(const CRegistry* _pInheritReg)
{
	if (_pInheritReg == NULL)
		return false;

	if (_pInheritReg->GetThisValue() != "")
	{
		CStr SuperReactions = _pInheritReg->GetThisValue().UpperCase();
		CStr SuperReaction;
		do
		{
			SuperReaction = SuperReactions.GetStrSep("+");
			SuperReaction.Trim();
			m_lSuperReactions.Add(SuperReaction);
		}
		while (SuperReactions != "");
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
			m_lSuperReactions.Add(pChildReg->GetThisName().UpperCase());
		}
	}

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_Reactions::ParseReactionReg(const CRegistry* _pReactionReg)
{
	if (_pReactionReg == NULL)
		return false;

	if (_pReactionReg->GetThisName().Left(8) != "REACTION")
	{
		m_pAGCompiler->OnError("Unrecognized keyword '" + _pReactionReg->GetThisName() + "' (" + m_Name + AG2C_SCOPESEPARATOR + "ACTIONS).");
		return false;
	}

	CStr ReactionName = _pReactionReg->GetThisName();
	CStr Location = m_Name + AG2C_SCOPESEPARATOR + ReactionName;

//	int8 TokenID = AG2_TOKENID_NULL;

	CXRAG2C_Reaction Reaction;
	Reaction.m_bExported = false;
	Reaction.m_iBaseEffectInstance = 0;
	Reaction.m_nEffectInstances = 0;
	Reaction.m_Name = ReactionName;

	bool bConditionSetFound = false;

	for (int iChild = 0; iChild < _pReactionReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pReactionReg->GetChild(iChild);

		if (pChildReg->GetThisName() == "EXPORT")
		{
			Reaction.m_bExported = true;
		}
		else if (pChildReg->GetThisName() == "MOVETOKEN")
		{
			CXRAG2C_MoveToken MoveToken;
			if (!ParseMoveTokenReg(pChildReg, Location, MoveToken))//MoveToken.m_TokenID, MoveToken.m_TargetState, Reaction.m_TargetGraphBlock, MoveToken.m_AnimBlendDuration, MoveToken.m_AnimBlendDelay, MoveToken.m_AnimTimeOffset, MoveToken.m_iAnimTimeOffsetType))
				return false;

			Reaction.m_lMoveTokens.Add(MoveToken);
		}
		else if (pChildReg->GetThisName() == "MOVEANIMGRAPH")
		{
			CXRAG2C_MoveAnimGraph MoveAG;
			if (!ParseMoveAnimGraphReg(pChildReg, Location, MoveAG))
				return false;

			Reaction.m_lMoveAnimGraphs.Add(MoveAG);
		}
		else if (pChildReg->GetThisName() == "EFFECTS")
		{
			if (!ParseActionEffectsReg(pChildReg, Reaction.m_iBaseEffectInstance, Reaction.m_nEffectInstances, Location))
				return false;
		}
		else if (pChildReg->GetThisName() == "IMPULSETYPE")
		{
			// Find impulsetype
			if (!m_pAGCompiler->ResolveImpulseType(pChildReg->GetThisValue(),Reaction.m_Condition.m_ImpulseType,Location))
				return false;
			bConditionSetFound = true;
		}
		else if (pChildReg->GetThisName() == "IMPULSEVALUE")
		{
			// Find impulsevalue
			if (!m_pAGCompiler->ResolveImpulseValue(pChildReg->GetThisValue(),Reaction.m_Condition.m_ImpulseValue,Location))
				return false;
		}
		/*else if (pChildReg->GetThisName().Left(12) == "CONDITIONSET")
		{
		if ((TargetState == "") && (TargetGraphBlock == "") && (Reaction.m_nEffectInstances == 0))
		{
		m_pAGCompiler->OnError("Missing targetstate/targetblock or effects (" + Location + ").");
		m_pAGCompiler->OnNote("MoveActions must be declared above all ConditionSets.");
		return false;
		}

		bConditionSetFound = true;
		if (!ParseConditionSetReg(pChildReg, Location, ReactionName, bExported, TokenID, TargetState, TargetGraphBlock, iBaseEffect, nEffects, AnimBlendDuration, AnimBlendDelay, AnimTimeOffset, iAnimTimeOffsetType))
		return false;
		}*/
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

	// Add reaction 
	AddReaction(Reaction);

	return true;
}

//--------------------------------------------------------------------------------

// Movetoken now with graphblock as well
bool CXRAG2C_Reactions::ParseMoveTokenReg(const CRegistry* _pMoveTokenReg, CStr _Location, CXRAG2C_MoveToken& _MoveToken)
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

bool CXRAG2C_Reactions::ParseMoveAnimGraphReg(const CRegistry* _pMoveAGReg, CStr _Location, CXRAG2C_MoveAnimGraph& _MoveAG)
{
	int TokenID = AG2_TOKENID_NULL;
	if (_pMoveAGReg->GetThisValue() != "")
	{
		if (!m_pAGCompiler->EvalConstantExpressionInt(_pMoveAGReg->GetThisValue(), TokenID, NULL, _Location))
			return false;
	}
	_MoveAG.m_TokenID = TokenID;

	for (int iChild = 0; iChild < _pMoveAGReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pMoveAGReg->GetChild(iChild);

		if (pChildReg->GetThisName() == "ANIMBLENDDURATION")
		{
			if (!m_pAGCompiler->EvalConstantExpressionFloat(pChildReg->GetThisValue(), _MoveAG.m_AnimBlendDuration, NULL, _Location))
				return false;
		}
		else if (pChildReg->GetThisName() == "ANIMBLENDDELAY")
		{
			if (!m_pAGCompiler->EvalConstantExpressionFloat(pChildReg->GetThisValue(), _MoveAG.m_AnimBlendDelay, NULL, _Location))
				return false;
		}
		else if (pChildReg->GetThisName() == "TARGETANIMGRAPH")
		{
			_MoveAG.m_TargetAnimGraph = pChildReg->GetThisValue().UpperCase();
			_MoveAG.m_TargetAnimGraphHash = StringToHash(_MoveAG.m_TargetAnimGraph);
		}
	}

	return _MoveAG.m_TargetAnimGraph.Len() > 0;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_Reactions::ParseActionEffectsReg(const CRegistry* _pEffectsReg, int& _iBaseEffect, int& _nEffects, CStr _Location)
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


//--------------------------------------------------------------------------------

int32 CXRAG2C_Reactions::AddReaction(CXRAG2C_Reaction& _Reaction)
{
	int32 iReaction = m_lReactions.Len();
	m_lReactions.Add(_Reaction);
	return iReaction;
}

//--------------------------------------------------------------------------------

/*int CXRAG2C_Reactions::ParseConditionReg(const CRegistry* _pConditionReg, int _iReaction, CStr _Location)
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
int iProperty = m_pAGCompiler->GetPropertyID(PropertyNameStr);
if (iProperty == -1)
{
m_pAGCompiler->OnError(CStrF("Undefined Property '%s' (%s).", (char*)PropertyNameStr, (char*)_Location));
return -1;
}

TArray<fp32> lPropertyParams;
if (!m_pAGCompiler->ParseParamsStr(PropertyParamsStr, lPropertyParams, _Location))
return -1;

int iOperator = m_pAGCompiler->GetOperatorID(OperatorStr);
if (iOperator == -1)
{
m_pAGCompiler->OnError(CStrF("Undefined operator '%s' (%s).", (char*)OperatorStr, (char*)_Location));
return -1;
}

fp32 ConstantValue;
if (!m_pAGCompiler->EvalConstantExpression(ConstantStr, ConstantValue, NULL, _Location))
return -1;

CXRAG2C_Condition Condition;
Condition.m_iProperty = iProperty;
lPropertyParams.Duplicate(&Condition.m_lPropertyParams);
Condition.m_iOperator = iOperator;
//	Condition.m_Constant = ConstantValue;
Condition.m_ConstantStr = ConstantStr;
Condition.m_iFirstAction = _iReaction;
int32 iCondition = AddCondition(Condition);
return iCondition;
}*/

//--------------------------------------------------------------------------------

/*int32 CXRAG2C_Reactions::AddCondition(CXRAG2C_Condition& _Condition)
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

_Condition.m_Occurrences++;
m_lConditions.Add(_Condition);

return iCondition;
}*/

//--------------------------------------------------------------------------------

bool CXRAG2C_Reactions::Process()
{
	CStr Location = m_Name;

	if (!ExpandInheritance())
		return false;

	if (m_lReactions.Len() == 0)
		m_pAGCompiler->OnWarning("Missing reactions (" + Location + ").");

	// Sort reactions by type and value
	TArray<CXRAG2C_Reaction> lReactions;
	//lReactions.SetLen(m_lReactions.Len());

	for (int iReaction = 0; iReaction < m_lReactions.Len(); iReaction++)
	{
		CStr Location = m_Name + AG2C_SCOPESEPARATOR + CStrF("REACTION%d",iReaction);

		// First find targetgraphblock, then target state
		// Resolve TargetState index.
		{
			CXRAG2C_Reaction* pReaction = &(m_lReactions[iReaction]);

			for (int32 i = 0; i < pReaction->m_lMoveTokens.Len(); i++)
			{
				CXRAG2C_GraphBlock* pGraphBlock = NULL;
				// If token is undefined set as zero
				/*if (pReaction->m_lMoveTokens[i].m_TokenID == AG2_TOKENID_NULL)
				{
				pReaction->m_lMoveTokens[i].m_TokenID = 0;
				m_pAGCompiler->OnNote(CStrF("Reaction: %s No token defined, using: %d",pReaction->m_Name.GetStr(),pReaction->m_lMoveTokens[i].m_TokenID));
				}*/
				/*if (pReaction->m_TargetGraphBlock == m_Name)
				m_pAGCompiler->OnWarning(CStrF("Cyclic target reaction '%s' (%s)", (char*)m_Name, (char*)Location));*/

				pReaction->m_lMoveTokens[i].m_iTargetState = AG2_STATEINDEX_NULL;
				pReaction->m_lMoveTokens[i].m_iTargetGraphBlock = AG2_GRAPHBLOCKINDEX_NULL;

				if (!pReaction->m_lMoveTokens[i].m_TargetGraphBlock.Len() && !pReaction->m_lMoveTokens[i].m_TargetState.Len())
				{
					// Only want to specify token...
					m_pAGCompiler->OnWarning(CStrF("%s CXRAG2C_Reactions::Process: ONLY TOKEN SPECIFIED: %s",Location.Str(),pReaction->m_lMoveTokens[i].m_TargetGraphBlock.GetStr()));
					continue;
				}

				if (pReaction->m_lMoveTokens[i].m_TargetGraphBlock.Len() <= 0)
				{
					pGraphBlock = m_pGraphBlock;
					// Use this graphblock if none defined
					pReaction->m_lMoveTokens[i].m_iTargetGraphBlock = m_pAGCompiler->GetGraphBlockIndex(m_Name);
				}
				else
				{
					pReaction->m_lMoveTokens[i].m_iTargetGraphBlock = m_pAGCompiler->GetGraphBlockIndex(pReaction->m_lMoveTokens[i].m_TargetGraphBlock);
					if (pReaction->m_lMoveTokens[i].m_iTargetGraphBlock == -1)
						m_pAGCompiler->OnError(CStrF("%s CXRAG2C_Reactions::Process: Unable to find graphblock: %s",Location.Str(),pReaction->m_lMoveTokens[i].m_TargetGraphBlock.GetStr()));
					pGraphBlock = m_pAGCompiler->m_lspGraphBlocks[pReaction->m_lMoveTokens[i].m_iTargetGraphBlock];
				}

				if (pReaction->m_lMoveTokens[i].m_TargetState == "TERMINATE")
				{
					pReaction->m_lMoveTokens[i].m_iTargetState = AG2_STATEINDEX_TERMINATE;
				}
				else if (pReaction->m_lMoveTokens[i].m_TargetState == "STARTAG")
				{
					pReaction->m_lMoveTokens[i].m_iTargetState = AG2_STATEINDEX_STARTAG;
				}
				else
				{
					if (pReaction->m_lMoveTokens[i].m_TargetState.Len() <= 0)
					{
						// No target state defined, use the startstate of target graphblock
						if (pReaction->m_lMoveTokens[i].m_iTargetGraphBlock != AG2_GRAPHBLOCKINDEX_NULL)
						{
							pReaction->m_lMoveTokens[i].m_iTargetState = pGraphBlock->m_StartMoveToken.m_iTargetState;//m_iStartState;
							pReaction->m_lMoveTokens[i].m_TargetStateType = pGraphBlock->m_StartMoveToken.m_TargetStateType;//m_iStartState;
						}
					}
					else
					{
						pGraphBlock->AcquireTargetState(pReaction->m_lMoveTokens[i]);
					}
					if (((pReaction->m_lMoveTokens[i].m_iTargetState == AG2_STATEINDEX_NULL) && (pReaction->m_nEffectInstances == 0)) ||
						((pReaction->m_lMoveTokens[i].m_iTargetState == AG2_STATEINDEX_NULL) && (pReaction->m_lMoveTokens[i].m_TargetState.Len() > 0)))
					{
						m_pAGCompiler->OnError(CStrF("Undefined target state '%s' (%s).", (char*)(pReaction->m_lMoveTokens[i].m_TargetState), (char*)Location));
						return false;
					}
				}
			}

			// No need to process MoveAnimGraphs atm

			// Sort reaction, add it last if no index found
			int32 iIndex = lReactions.Len();
			for (int32 i = 0; i < lReactions.Len(); i++)
			{
				// Check for mixed value/non value, not allowed (atm), screws up searching
				if ((pReaction->m_Condition.m_ImpulseType == lReactions[i].m_Condition.m_ImpulseType) &&
					((pReaction->m_Condition.m_ImpulseValue == -1) || (lReactions[i].m_Condition.m_ImpulseValue == -1)))
				{
					m_pAGCompiler->OnError(CStrF("Can't mix reactions with and without values '%s (%s)'.", (char*)Location,pReaction->m_Name.GetStr()));
					return false;
				}
				// Make sure we have no dual reactions
				if ((pReaction->m_Condition.m_ImpulseType == lReactions[i].m_Condition.m_ImpulseType) &&
					(pReaction->m_Condition.m_ImpulseValue == lReactions[i].m_Condition.m_ImpulseValue))

				{
					m_pAGCompiler->OnError(CStrF("Can't have reactions with same values '%s (%s - %s)'.", (char*)Location,pReaction->m_Name.GetStr(),lReactions[i].m_Name.GetStr()));
					return false;
				}
				if ((pReaction->m_Condition.m_ImpulseType < lReactions[i].m_Condition.m_ImpulseType) ||
					((pReaction->m_Condition.m_ImpulseType == lReactions[i].m_Condition.m_ImpulseType) &&
					(pReaction->m_Condition.m_ImpulseValue < lReactions[i].m_Condition.m_ImpulseValue)))
				{
					iIndex = i;
					break;
				}
			}
			lReactions.Insert(iIndex,*pReaction);
		}
	}
	m_lReactions = lReactions;
/*	for (int32 i = 0; i < m_lReactions.Len(); i++)
	{
		CXRAG2C_Reaction* pReaction = &(m_lReactions[i]);
		int i2 = 0;
	}
*/
	// Check impulsetype/value?..

	return true;
}

//--------------------------------------------------------------------------------

bool CXRAG2C_Impulse::ParseReg(const CRegistry* _pImpulseReg)
{
	for (int iChild = 0; iChild < _pImpulseReg->GetNumChildren(); iChild++)
	{
		const CRegistry* pChildReg = _pImpulseReg->GetChild(iChild);

		if (pChildReg->GetThisName() == "TYPE")
		{
			m_ImpulseType = pChildReg->GetThisValue().Val_int();
		}
		else if (pChildReg->GetThisName() == "VALUE")
		{
			m_ImpulseValue = pChildReg->GetThisValue().Val_int();
		}
	}

	return true;
}
