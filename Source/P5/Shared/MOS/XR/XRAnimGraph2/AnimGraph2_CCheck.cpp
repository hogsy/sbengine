//--------------------------------------------------------------------------------

#include "PCH.h"
#include "MDA.h"
#include "AnimGraph2.h"

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

CStr CXRAG2::CheckConsistency(bool _bThrowException)
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

bool CXRAG2::ReportInconsistency(CStr _Msg, CStr& _Result)
{
	_Result = _Msg;
	return false;
}

bool CXRAG2::CheckGraphConsistency(CStr& _Result)
{
//	const CXRAG2_Action* pAction = GetAction(0);
//	if (pAction == NULL)
//		return ReportInconsistency(CStr("Missing start action."), _Result);

	/*const CXRAG2_State* pState = GetState(pAction->GetTargetStateIndex());
	if (pState == NULL)
		return ReportInconsistency(CStrF("Invalid state %d referenced from start action (range is 0-%d).", pAction->GetTargetStateIndex(), GetNumStates() - 1), _Result);*/

	for (int iState = 0; iState < GetNumStates(); iState++)
	{
		const CXRAG2_State* pState = GetState(iState);
		if (!CheckStateConsistency(pState, _Result))
			return false;
	}

	for (int iEntry = 0; iEntry < m_lActionHashEntries.Len(); iEntry++)
	{
		const CXRAG2_Action* pAction = GetAction(m_lActionHashEntries[iEntry].m_iAction);
		if (pAction == NULL)
			return ReportInconsistency(CStrF("Invalid action %d for hashkey %X (range is 0-%d).", m_lActionHashEntries[iEntry].m_iAction, m_lActionHashEntries[iEntry].m_HashKey, GetNumActions()), _Result);
	}

	return true;
}

bool CXRAG2::CheckStateConsistency(const CXRAG2_State* _pState, CStr& _Result)
{
	int iNode = _pState->GetBaseNodeIndex();

	if (iNode != -1)
	{
		const CXRAG2_ConditionNodeV2* pNode = GetNode(iNode);
		if (pNode == NULL)
			return ReportInconsistency(CStrF("Invalid node reference %d (range is 0-%d).", iNode, m_lNodesV2.Len() - 1), _Result);

		if (!CheckNodeConsistency(_pState, pNode, _Result))
			return ReportInconsistency(_Result, _Result);
	}

	if (!CheckStateConstantsConsistency(_pState->GetBaseConstantIndex(), _pState->GetNumConstants(), _Result))
		return ReportInconsistency(_Result, _Result);

	return true;
}

bool CXRAG2::CheckNodeConsistency(const CXRAG2_State* _pState, const CXRAG2_ConditionNodeV2* _pNode, CStr& _Result)
{
//	uint8 iOperator = _pNode->GetOperator();
//	uint8 iProperty = _pNode->GetProperty();
	uint8 PropertyType = _pNode->GetPropertyType();
	int16 TrueAction = _pNode->GetTrueAction();
	int16 FalseAction = _pNode->GetFalseAction();

	if (PropertyType > AG2_PROPERTYTYPE_END)
		return ReportInconsistency(CStrF("Invalid property type %d (range is 0-%d).", PropertyType, AG2_PROPERTYTYPE_END), _Result);

	if (!CheckNodeActionConsistency(_pState, TrueAction, _Result))
		return ReportInconsistency(_Result, _Result);

	if (!CheckNodeActionConsistency(_pState, FalseAction, _Result))
		return ReportInconsistency(_Result, _Result);

	return true;
}

bool CXRAG2::CheckNodeActionConsistency(const CXRAG2_State* _pState, int16 _NodeAction, CStr& _Result)
{
	if (_NodeAction == AG2_NODEACTION_FAILPARSE)
		return true;

	if ((_NodeAction & AG2_NODEACTION_TYPEMASK) == AG2_NODEACTION_ENDPARSE)
	{
		int16 iAction = _pState->GetBaseActionIndex() + (_NodeAction & AG2_NODEACTION_ACTIONMASK);
		const CXRAG2_Action* pAction = GetAction(iAction);
		if (pAction == NULL)
			return ReportInconsistency(CStrF("Invalid action reference %d (range is 0-%d).", iAction, m_lFullActions.Len() - 1), _Result);

		/*int16 iTargetState = pAction->GetTargetStateIndex();

		if (!CheckActionEffectsConsistency(pAction->GetBaseEffectInstanceIndex(), pAction->GetNumEffectInstances(), _Result))
			return ReportInconsistency(_Result, _Result);

		if (iTargetState != AG2_STATEINDEX_NULL)
		{
			iTargetState = iTargetState & AG2_TARGETSTATE_INDEXMASK;
			const CXRAG2_State* pState = GetState(iTargetState);
			if (pState == NULL)
				return ReportInconsistency(CStrF("Invalid state reference %d (range is 0-%d).", iTargetState, (GetNumStates() - 1)), _Result);
		}*/

	}
	else
	{
		int iNode = _pState->GetBaseNodeIndex() + (_NodeAction & AG2_NODEACTION_NODEMASK);
		CXRAG2_ConditionNodeV2* pNode = &(m_lNodesV2[iNode]);
		if (!CheckNodeConsistency(_pState, pNode, _Result))
			return ReportInconsistency(_Result, _Result);
	}

	return true;
}

bool CXRAG2::CheckActionEffectsConsistency(int _iBaseEffect, int _nEffects, CStr& _Result)
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

bool CXRAG2::CheckStateConstantsConsistency(int _iBaseConstant, int _nConstants, CStr& _Result)
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
