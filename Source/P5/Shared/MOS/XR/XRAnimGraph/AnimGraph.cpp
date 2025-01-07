//--------------------------------------------------------------------------------

#include "PCH.h"
#include "MRTC.h"
#include "MMemMgrHeap.h"
#include "MDA.h"
#include "AnimGraph.h"

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG::Clear()
{
	m_Name = "";

	m_lFullStates.Clear();
	m_lSmallStates.Clear();

	m_lFullAnimLayers.Clear();
	m_lSmallAnimLayers.Clear();
	m_lAnimLayersMap.Clear();

	m_lFullActions.Clear();
	m_lSmallActions.Clear();
	m_lActionsMap.Clear();

	m_lNodes.Clear();
	m_lEffectInstances.Clear();
	m_lStateConstants.Clear();
	m_lCallbackParams.Clear();

	m_lActionHashEntries.Clear();

#ifndef M_RTM
	m_lExportedActionNames.Clear();
	m_lExportedStateNames.Clear();
	m_lExportedPropertyNames.Clear();
	m_lExportedOperatorNames.Clear();
	m_lExportedEffectNames.Clear();
	m_lExportedStateConstantNames.Clear();
#endif
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

const CXRAG_State* CXRAG::GetState(CAGStateIndex _iState) const
{
/*
	if (_iState == AG_STATEINDEX_NULL)
		return NULL;

	if (_iState == AG_STATEINDEX_TERMINATE)
		return NULL;
*/

	if (_iState < 0)
		return NULL;

	if (_iState < m_lFullStates.Len())
		return &(m_lFullStates[_iState]);

	_iState -= m_lFullStates.Len();

	if (_iState < m_lSmallStates.Len())
		return &(m_lSmallStates[_iState]);

	return NULL;
}

//--------------------------------------------------------------------------------

const CXRAG_AnimLayer* CXRAG::GetAnimLayer(CAGAnimLayerIndex _iAnimLayer) const
{
	if (!m_lAnimLayersMap.ValidPos(_iAnimLayer))
		return NULL;

	_iAnimLayer = m_lAnimLayersMap[_iAnimLayer];

	if (_iAnimLayer < m_lFullAnimLayers.Len())
		return &(m_lFullAnimLayers[_iAnimLayer]);
	else
		return &(m_lSmallAnimLayers[_iAnimLayer - m_lFullAnimLayers.Len()]);
}

//--------------------------------------------------------------------------------

const CXRAG_Action* CXRAG::GetAction(CAGActionIndex _iAction) const
{
	if (!m_lActionsMap.ValidPos(_iAction))
		return NULL;

	_iAction = m_lActionsMap[_iAction];

	if (_iAction < m_lFullActions.Len())
		return &(m_lFullActions[_iAction]);
	else
		return &(m_lSmallActions[_iAction - m_lFullActions.Len()]);
}

//--------------------------------------------------------------------------------

const CXRAG_ConditionNode* CXRAG::GetNode(CAGNodeIndex _iNode) const
{
	if (!m_lNodes.ValidPos(_iNode))
		return NULL;

	return &(m_lNodes[_iNode]);
}

//--------------------------------------------------------------------------------

const CXRAG_StateConstant* CXRAG::GetStateConstant(CAGStateConstantIndex _iStateConstant) const
{
	if (!m_lStateConstants.ValidPos(_iStateConstant))
		return NULL;

	return &(m_lStateConstants[_iStateConstant]);
}

//--------------------------------------------------------------------------------

const CXRAG_EffectInstance* CXRAG::GetEffectInstance(CAGEffectInstanceIndex _iEffectInstance) const
{
	if (!m_lEffectInstances.ValidPos(_iEffectInstance))
		return NULL;

	return &(m_lEffectInstances[_iEffectInstance]);
}
		
//--------------------------------------------------------------------------------

CXRAG_ICallbackParams CXRAG::GetICallbackParams(CAGCallbackParamIndex _iParams, uint8 _nParams) const
{
	if (m_lCallbackParams.Len() < (_iParams + _nParams - 1))
		return CXRAG_ICallbackParams(NULL, 0);

	if (_nParams == 0)
		return CXRAG_ICallbackParams(NULL, 0);

	return CXRAG_ICallbackParams(&(m_lCallbackParams[_iParams]), _nParams);
}

//--------------------------------------------------------------------------------

int16 CXRAG::GetActionIndexFromHashKey(uint32 _ActionHashKey) const
{
	// FIXME: Implement binary search or whatever faster search algo (mayby use hash properties =)?).
	for (int iEntry = 0; iEntry < m_lActionHashEntries.Len(); iEntry++)
	{
		if (m_lActionHashEntries[iEntry].m_HashKey == _ActionHashKey)
		{
			return (m_lActionHashEntries[iEntry].m_iAction);
		}
	}

	return AG_ACTIONINDEX_NULL;
}

#ifndef M_RTM

//--------------------------------------------------------------------------------

int16 CXRAG::GetExportedActionIndexFromActionIndex(CAGActionIndex _iAction) const
{
	for (int iActionHashEntry = 0; iActionHashEntry < m_lActionHashEntries.Len(); iActionHashEntry++)
	{
		if (m_lActionHashEntries[iActionHashEntry].m_iAction == _iAction)
			return iActionHashEntry;
	}
	return AG_EXPORTEDACTIONINDEX_NULL;
}

//--------------------------------------------------------------------------------

int16 CXRAG::GetExportedActionAnimIndex(int16 _iExportedAction) const
{
	CStr ExportedActionName = GetExportedActionName(_iExportedAction);
	uint32 ActionHashKey = StringToHash(ExportedActionName);

	int16 iAction = GetActionIndexFromHashKey(ActionHashKey);
	const CXRAG_Action* pAction = GetAction(iAction);
	if (pAction == NULL)
		return AG_ANIMINDEX_NULL;

	int16 iState = pAction->GetTargetStateIndex();
	const CXRAG_State* pState = GetState(iState);
	if (pState == NULL || !pState->GetNumAnimLayers())
		return AG_ANIMINDEX_NULL;

	int16 iAnimLayer = pState->GetBaseAnimLayerIndex();
	const CXRAG_AnimLayer* pAnimLayer = GetAnimLayer(iAnimLayer);
	if (pAnimLayer == NULL)
		return AG_ANIMINDEX_NULL;

	int16 iAnim = pAnimLayer->GetAnimIndex();
	return iAnim;
}

//--------------------------------------------------------------------------------

CStr CXRAG::GetExportedStateName(CAGStateIndex _iState) const
{
	if (m_lExportedStateNames.ValidPos(_iState))
		return m_lExportedStateNames[_iState];
	return "";
}

//--------------------------------------------------------------------------------

CStr CXRAG::GetExportedPropertyNameFromIndex(uint8 _iProperty) const
{
	if (m_lExportedPropertyNames.ValidPos(_iProperty))
		return m_lExportedPropertyNames[_iProperty].m_Name;
	return "";
}

//--------------------------------------------------------------------------------

CStr CXRAG::GetExportedOperatorNameFromIndex(uint8 _iOperator) const
{
	if (m_lExportedOperatorNames.ValidPos(_iOperator))
		return m_lExportedOperatorNames[_iOperator].m_Name;
	return "";
}

//--------------------------------------------------------------------------------

CStr CXRAG::GetExportedEffectNameFromIndex(uint8 _iEffect) const
{
	if (m_lExportedEffectNames.ValidPos(_iEffect))
		return m_lExportedEffectNames[_iEffect].m_Name;
	return "";
}

//--------------------------------------------------------------------------------

CStr CXRAG::GetExportedStateConstantNameFromIndex(uint8 _iStateConstant) const
{
	if (m_lExportedStateConstantNames.ValidPos(_iStateConstant))
		return m_lExportedStateConstantNames[_iStateConstant].m_Name;
	return "";
}

//--------------------------------------------------------------------------------

CStr CXRAG::GetExportedPropertyNameFromID(CAGPropertyID _PropertyID) const
{
	for (int iProperty = 0; iProperty < m_lExportedPropertyNames.Len(); iProperty++)
	{
		if (m_lExportedPropertyNames[iProperty].m_ID == _PropertyID)
			return m_lExportedPropertyNames[iProperty].m_Name;
	}
	return "";
}

//--------------------------------------------------------------------------------

CStr CXRAG::GetExportedOperatorNameFromID(CAGOperatorID _OperatorID) const
{
	for (int iOperator = 0; iOperator < m_lExportedOperatorNames.Len(); iOperator++)
	{
		if (m_lExportedOperatorNames[iOperator].m_ID == _OperatorID)
			return m_lExportedOperatorNames[iOperator].m_Name;
	}
	return "";
}

//--------------------------------------------------------------------------------

CStr CXRAG::GetExportedEffectNameFromID(CAGEffectID _EffectID) const
{
	for (int iEffect = 0; iEffect < m_lExportedEffectNames.Len(); iEffect++)
	{
		if (m_lExportedEffectNames[iEffect].m_ID == _EffectID)
			return m_lExportedEffectNames[iEffect].m_Name;
	}
	return "";
}

//--------------------------------------------------------------------------------

CStr CXRAG::GetExportedStateConstantNameFromID(CAGStateConstantID _StateConstantID) const
{
	for (int iStateConstant = 0; iStateConstant < m_lExportedStateConstantNames.Len(); iStateConstant++)
	{
		if (m_lExportedStateConstantNames[iStateConstant].m_ID == _StateConstantID)
			return m_lExportedStateConstantNames[iStateConstant].m_Name;
	}
	return "";
}
#endif //M_RTM

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
