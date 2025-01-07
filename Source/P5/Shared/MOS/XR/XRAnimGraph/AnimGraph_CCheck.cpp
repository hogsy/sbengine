//--------------------------------------------------------------------------------

#include "PCH.h"
#include "MDA.h"
#include "AnimGraph.h"

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

CStr CXRAG::CheckConsistency(bool _bThrowException)
{
	CStr Result;
	if (!CheckGraphConsistency(Result))
	{
		if (_bThrowException)
		{
			Error("AnimGraph::CheckConsistency()", "Inconsistent AnimGraph data. " + Result);
		}
		else
			return Result;
	}

	return "";
}

bool CXRAG::ReportInconsistency(CStr _Msg, CStr& _Result)
{
	_Result = _Msg;
	return false;
}

bool CXRAG::CheckGraphConsistency(CStr& _Result)
{
	const CXRAG_Action* pAction = GetAction(0);
	if (pAction == NULL)
		return ReportInconsistency(CStr("Missing start action."), _Result);

	const CXRAG_State* pState = GetState(pAction->GetTargetStateIndex());
	if (pState == NULL)
		return ReportInconsistency(CStrF("Invalid state %d referenced from start action (range is 0-%d).", pAction->GetTargetStateIndex(), GetNumStates() - 1), _Result);

	for (int iState = 0; iState < GetNumStates(); iState++)
	{
		const CXRAG_State* pState = GetState(iState);
		if (!CheckStateConsistency(pState, _Result))
			return false;
	}

	for (int iEntry = 0; iEntry < m_lActionHashEntries.Len(); iEntry++)
	{
		const CXRAG_Action* pAction = GetAction(m_lActionHashEntries[iEntry].m_iAction);
		if (pAction == NULL)
			return ReportInconsistency(CStrF("Invalid action %d for hashkey %X (range is 0-%d).", m_lActionHashEntries[iEntry].m_iAction, m_lActionHashEntries[iEntry].m_HashKey, GetNumActions()), _Result);
	}

	return true;
}

bool CXRAG::CheckStateConsistency(const CXRAG_State* _pState, CStr& _Result)
{
	int iNode = _pState->GetBaseNodeIndex();

	if (iNode != -1)
	{
		const CXRAG_ConditionNode* pNode = GetNode(iNode);
		if (pNode == NULL)
			return ReportInconsistency(CStrF("Invalid node reference %d (range is 0-%d).", iNode, m_lNodes.Len() - 1), _Result);

		if (!CheckNodeConsistency(_pState, pNode, _Result))
			return ReportInconsistency(_Result, _Result);
	}

	if (!CheckStateConstantsConsistency(_pState->GetBaseConstantIndex(), _pState->GetNumConstants(), _Result))
		return ReportInconsistency(_Result, _Result);

	return true;
}

bool CXRAG::CheckNodeConsistency(const CXRAG_State* _pState, const CXRAG_ConditionNode* _pNode, CStr& _Result)
{
	uint8 iOperator = _pNode->GetOperator();
	uint8 iProperty = _pNode->GetProperty();
	uint8 iPropertyParams = _pState->GetBasePropertyParamIndex() + _pNode->GetPropertyParamsIndex();
	uint8 nPropertyParams = _pNode->GetNumPropertyParams();
	fp32 Constant = _pNode->GetConstant();
	int16 TrueAction = _pNode->GetTrueAction();
	int16 FalseAction = _pNode->GetFalseAction();

	if ((iPropertyParams < 0) || ((iPropertyParams + nPropertyParams - 1) >= m_lCallbackParams.Len()))
		return ReportInconsistency(CStrF("Invalid property param references %d-%d (range is 0-%d).", iPropertyParams, (iPropertyParams + nPropertyParams - 1), m_lCallbackParams.Len() - 1), _Result);

	if (!CheckNodeActionConsistency(_pState, TrueAction, _Result))
		return ReportInconsistency(_Result, _Result);

	if (!CheckNodeActionConsistency(_pState, FalseAction, _Result))
		return ReportInconsistency(_Result, _Result);

	return true;
}

bool CXRAG::CheckNodeActionConsistency(const CXRAG_State* _pState, int16 _NodeAction, CStr& _Result)
{
	if (_NodeAction == AG_NODEACTION_FAILPARSE)
		return true;

	if ((_NodeAction & AG_NODEACTION_TYPEMASK) == AG_NODEACTION_ENDPARSE)
	{
		int16 iAction = _pState->GetBaseActionIndex() + (_NodeAction & AG_NODEACTION_ACTIONMASK);
		const CXRAG_Action* pAction = GetAction(iAction);
		if (pAction == NULL)
			return ReportInconsistency(CStrF("Invalid action reference %d (range is 0-%d).", iAction, m_lActionsMap.Len() - 1), _Result);

		int16 iTargetState = pAction->GetTargetStateIndex();

		if (!CheckActionEffectsConsistency(pAction->GetBaseEffectInstanceIndex(), pAction->GetNumEffectInstances(), _Result))
			return ReportInconsistency(_Result, _Result);

		if (iTargetState != AG_STATEINDEX_NULL)
		{
			iTargetState = iTargetState & AG_TARGETSTATE_INDEXMASK;
			const CXRAG_State* pState = GetState(iTargetState);
			if (pState == NULL)
				return ReportInconsistency(CStrF("Invalid state reference %d (range is 0-%d).", iTargetState, (GetNumStates() - 1)), _Result);
		}

	}
	else
	{
		int iNode = _pState->GetBaseNodeIndex() + (_NodeAction & AG_NODEACTION_NODEMASK);
		CXRAG_ConditionNode* pNode = &(m_lNodes[iNode]);
		if (!CheckNodeConsistency(_pState, pNode, _Result))
			return ReportInconsistency(_Result, _Result);
	}

	return true;
}

bool CXRAG::CheckActionEffectsConsistency(int _iBaseEffect, int _nEffects, CStr& _Result)
{
	if (_nEffects == 0)
		return true;

	if ((!m_lEffectInstances.ValidPos(_iBaseEffect)) ||
		(!m_lEffectInstances.ValidPos(_iBaseEffect + _nEffects - 1)))
		return ReportInconsistency(CStrF("Invalid effect references %d-%d (range is 0-%d).", 
										_iBaseEffect, _iBaseEffect + _nEffects - 1, m_lEffectInstances.Len() - 1),
								   _Result);

	for (int jEffect = _iBaseEffect; jEffect < (_iBaseEffect + _nEffects); jEffect++)
	{
		int iParams = m_lEffectInstances[jEffect].m_iParams;
		int nParams = m_lEffectInstances[jEffect].m_nParams;
		int jParams = (iParams + nParams - 1);
		if ((iParams < 0) || (jParams >= m_lCallbackParams.Len()))
			return ReportInconsistency(CStrF("Invalid effect param references %d-%d (range is 0-%d).", iParams, jParams, m_lCallbackParams.Len() - 1), _Result);
	}

	return true;
}

bool CXRAG::CheckStateConstantsConsistency(int _iBaseConstant, int _nConstants, CStr& _Result)
{
	if (_nConstants == 0)
		return true;

	if ((!m_lStateConstants.ValidPos(_iBaseConstant)) ||
		(!m_lStateConstants.ValidPos(_iBaseConstant + _nConstants - 1)))
		return ReportInconsistency(CStrF("Invalid StateConstant references %d-%d (range is 0-%d).", 
										_iBaseConstant, _iBaseConstant + _nConstants - 1, m_lStateConstants.Len() - 1),
								   _Result);

	return true;
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
