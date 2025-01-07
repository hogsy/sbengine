//--------------------------------------------------------------------------------

#include "PCH.h"
#include "MRTC.h"
#include "MMemMgrHeap.h"
#include "MDA.h"
#include "AnimGraph2.h"
#include "../../Classes/GameWorld/WDataRes_Anim.h"
#include "../../Classes/GameWorld/WMapData.h"
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG2::Clear()
{
	m_Name = "";

	m_lFullStates.Clear();

	m_lFullAnimLayers.Clear();

	m_lFullActions.Clear();
	m_lFullReactions.Clear();

	m_lNodesV2.Clear();
	m_lEffectInstances.Clear();
	m_lStateConstants.Clear();
	m_lCallbackParams.Clear();

	m_lActionHashEntries.Clear();

#ifndef M_RTM
	m_lExportedActionNames.Clear();
	m_lExportedStateNames.Clear();
	m_lExportedReactionNames.Clear();
	m_lExportedPropertyFloatNames.Clear();
	m_lExportedPropertyIntNames.Clear();
	m_lExportedPropertyBoolNames.Clear();
	m_lExportedOperatorNames.Clear();
	m_lExportedEffectNames.Clear();
	m_lExportedStateConstantNames.Clear();
#endif
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

const CXRAG2_State* CXRAG2::GetState(CAG2StateIndex _iState) const
{
	if (!m_lFullStates.ValidPos(_iState))
		return NULL;

	return &(m_lFullStates[_iState]);
}

const CXRAG2_SwitchState* CXRAG2::GetSwitchState(CAG2StateIndex _iState) const
{
	if (!m_lSwitchStates.ValidPos(_iState))
		return NULL;

	return &(m_lSwitchStates[_iState]);
}

const CXRAG2_GraphBlock* CXRAG2::GetGraphBlock(CAG2StateIndex _iBlock) const
{
	if (!m_lGraphBlocks.ValidPos(_iBlock))
		return NULL;

	return &(m_lGraphBlocks[_iBlock]);
}

const CXRAG2_AnimNames* CXRAG2::GetAnimName(CAG2AnimLayerIndex _iAnimLayer) const
{
	if (!m_lAnimNames.ValidPos(_iAnimLayer))
		return NULL;

	return &(m_lAnimNames[_iAnimLayer]);
}

CStr CXRAG2::GetAnimContainerName(int32 _iContainer) const
{
	return m_AnimContainerNames.GetContainerName(_iContainer);
}

//--------------------------------------------------------------------------------

const CXRAG2_AnimLayer* CXRAG2::GetAnimLayer(CAG2AnimLayerIndex _iAnimLayer) const
{
	if (!m_lFullAnimLayers.ValidPos(_iAnimLayer))
		return NULL;

	return &(m_lFullAnimLayers[_iAnimLayer]);
}

//--------------------------------------------------------------------------------

const CXRAG2_Action* CXRAG2::GetAction(CAG2ActionIndex _iAction) const
{
	if (!m_lFullActions.ValidPos(_iAction))
		return NULL;

	return &(m_lFullActions[_iAction]);
}

const CXRAG2_SwitchStateActionVal* CXRAG2::GetSwitchStateActionVal(CAG2ActionIndex _iAction) const
{
	if (!m_lSwitchStateActionVals.ValidPos(_iAction))
		return NULL;

	return &(m_lSwitchStateActionVals[_iAction]);
}

const CXRAG2_MoveToken* CXRAG2::GetMoveToken(CAG2MoveTokenIndex _iMoveToken) const
{
	if (!m_lMoveTokens.ValidPos(_iMoveToken))
		return NULL;

	return &(m_lMoveTokens[_iMoveToken]);
}

const CXRAG2_MoveAnimGraph* CXRAG2::GetMoveAnimGraph(CAG2MoveAnimgraphIndex _iMoveAnimGraph) const
{
	if (!m_lMoveAnimGraphs.ValidPos(_iMoveAnimGraph))
		return NULL;

	return &(m_lMoveAnimGraphs[_iMoveAnimGraph]);
}

const CXRAG2_Reaction* CXRAG2::GetReaction(CAG2ReactionIndex _iReaction) const
{
	if (!m_lFullReactions.ValidPos(_iReaction))
		return NULL;

	return &(m_lFullReactions[_iReaction]);
}

//--------------------------------------------------------------------------------

const CXRAG2_ConditionNodeV2* CXRAG2::GetNode(CAG2NodeIndex _iNode) const
{
	if (!m_lNodesV2.ValidPos(_iNode))
		return NULL;

	return &(m_lNodesV2[_iNode]);
}

//--------------------------------------------------------------------------------

const CXRAG2_StateConstant* CXRAG2::GetStateConstant(CAG2StateConstantIndex _iStateConstant) const
{
	if (!m_lStateConstants.ValidPos(_iStateConstant))
		return NULL;

	return &(m_lStateConstants[_iStateConstant]);
}

//--------------------------------------------------------------------------------

const CXRAG2_EffectInstance* CXRAG2::GetEffectInstance(CAG2EffectInstanceIndex _iEffectInstance) const
{
	if (!m_lEffectInstances.ValidPos(_iEffectInstance))
		return NULL;

	return &(m_lEffectInstances[_iEffectInstance]);
}

//--------------------------------------------------------------------------------

CXRAG2_ICallbackParams CXRAG2::GetICallbackParams(CAG2CallbackParamIndex _iParams, uint8 _nParams) const
{
	if (m_lCallbackParams.Len() < (_iParams + _nParams - 1))
		return CXRAG2_ICallbackParams(NULL, 0);

	if (_nParams == 0)
		return CXRAG2_ICallbackParams(NULL, 0);

	return CXRAG2_ICallbackParams(&(m_lCallbackParams[_iParams]), _nParams);
}

//--------------------------------------------------------------------------------

int16 CXRAG2::GetActionIndexFromHashKey(uint32 _ActionHashKey) const
{
	// FIXME: Implement binary search or whatever faster search algo (mayby use hash properties =)?).
	for (int iEntry = 0; iEntry < m_lActionHashEntries.Len(); iEntry++)
	{
		if (m_lActionHashEntries[iEntry].m_HashKey == _ActionHashKey)
		{
			return (m_lActionHashEntries[iEntry].m_iAction);
		}
	}

	return AG2_ACTIONINDEX_NULL;
}

void CXRAG2::SetAnimContainerResource(const CStr& _ContainerName, int32 _iContainerRes)
{
	// Find which index match the name
	int32 Len = m_AnimContainerNames.m_lNames.Len();
	int32 iName = -1;
	for (int32 i = 0; i < Len; i++)
	{
		if (_ContainerName == m_AnimContainerNames.m_lNames[i])
		{
			iName = i;
			break;
		}
	}
	if (iName == -1)
		return;

	Len = m_lAnimNames.Len();
	for (int32 i = 0; i < Len; i++)
	{
		if (m_lAnimNames[i].m_iContainerName == iName)
			m_lAnimNames[i].m_iAnimContainerResource = _iContainerRes;
	}
}

const CXR_Anim_SequenceData* CXRAG2::GetAnimSequenceData(CWorldData* _pWData, int32 _iAnim) const
{
	if (!m_lAnimNames.ValidPos(_iAnim))
		return NULL;

	const CXRAG2_AnimNames& Animation = m_lAnimNames[_iAnim];

	if (Animation.m_iAnimContainerResource <= 0)
	{
		int32 iRes = _pWData->ResourceExistsPartial("ANM:" + m_AnimContainerNames.GetContainerName(Animation.m_iContainerName));
		Animation.m_iAnimContainerResource = iRes;
	}
	
	if (Animation.m_pSeq)
		return Animation.m_pSeq;

	spCWResource spAnimContainerResource = _pWData->GetResourceRef(Animation.m_iAnimContainerResource);
	if (!spAnimContainerResource)
		return NULL;

	CWRes_Anim* pAnimRes = (CWRes_Anim*)(const CWResource*)spAnimContainerResource;
	pAnimRes->m_TouchTime = _pWData->m_TouchTime; // Added by Anton

	CXR_Anim_Base* pAnimBase = pAnimRes->GetAnim();
	if (pAnimBase == NULL)
		return NULL;

	Animation.m_pSeq = (CXR_Anim_SequenceData*)pAnimBase->GetSequence(Animation.m_iAnimSeq);

	return Animation.m_pSeq;
}

void CXRAG2::LoadAllContainers(CMapData *_pMapData)
{
	for (int32 i = 0; i < m_AnimContainerNames.m_lNames.Len(); i++)
	{
		CStr LoadStr = "ANM:" + m_AnimContainerNames.m_lNames[i];
		//int iRes = -1;
		M_TRY
		{
			_pMapData->m_spWData->GetResourceIndex(LoadStr, WRESOURCE_CLASS_XSA, _pMapData);
		}
		M_CATCH(
		catch (CCException &)
		{
			ConOutL(CStrF("Failed to load animation container %s", m_AnimContainerNames.m_lNames[i].Str()));
		}
		)
	}
}

void CXRAG2::ClearAnimSequenceCache()
{
	int32 Len = m_lAnimNames.Len();
	for (int32 i = 0; i < Len; i++)
		m_lAnimNames[i].m_pSeq = NULL;
}

CAG2ReactionIndex CXRAG2::GetMatchingReaction(int16 _iBlock, CXRAG2_Impulse _Impulse) const
{
	// Search through given blocks reactions to see if we can find a match
	const CXRAG2_GraphBlock* pBlock = GetGraphBlock(_iBlock);
	if (!pBlock || pBlock->GetNumReactions() == 0)
		return -1;

	int32 iReactionStart = pBlock->GetBaseReactionIndex();
	int32 NumReactions = pBlock->GetNumReactions();
	int32 iSearchStart = iReactionStart;
	int32 iSearchEnd = iSearchStart + NumReactions - 1;
	bool bFoundMatch = false;

	while (iSearchEnd >= iSearchStart)
	{
		// Go left or right?
		int32 Middle = (iSearchEnd - iSearchStart);
		if (Middle > 1)
		{
			//Middle = M_Floor(Middle / 2);
			Middle = Middle / 2;
			int16 Type = m_lFullReactions[iSearchStart + Middle].m_Impulse.m_ImpulseType - _Impulse.m_ImpulseType;
			if (Type == 0)
			{
				// Ok, got type match, check if the values match
				int16 Value = m_lFullReactions[iSearchStart + Middle].m_Impulse.m_ImpulseValue;
				// Special case, this reaction might react to anything
				if (Value == -1)
				{
					iSearchStart += Middle;
					bFoundMatch = true;
					break;
				}
				Value -= _Impulse.m_ImpulseValue;
				if (Value == 0)
				{
					iSearchStart += Middle;
					bFoundMatch = true;
					break;
				}
				else if (Value < 0)
				{
					// Go right
					iSearchStart += Middle + 1;
				}
				else
				{
					// Go Left
					iSearchEnd  = iSearchStart + Middle - 1;
				}
			}
			else if (Type < 0)
			{
				// Go Right
				iSearchStart += Middle + 1;
			}
			else
			{
				// Go left
				iSearchEnd  = iSearchStart + Middle - 1;
			}
		}
		else
		{
			// Ok, found nothing, check the last two reactions
			if (m_lFullReactions[iSearchStart].m_Impulse.m_ImpulseType == _Impulse.m_ImpulseType)
			{
				int16 Value = m_lFullReactions[iSearchStart].m_Impulse.m_ImpulseValue;
				if ((Value == -1) || ((Value - _Impulse.m_ImpulseValue) == 0))
				{
					bFoundMatch = true;
					break;
				}
			}
			if (iSearchStart != iSearchEnd && (m_lFullReactions[iSearchEnd].m_Impulse.m_ImpulseType == _Impulse.m_ImpulseType))
			{
				int16 Value = m_lFullReactions[iSearchEnd].m_Impulse.m_ImpulseValue;
				if ((Value == -1) || ((Value - _Impulse.m_ImpulseValue) == 0))
				{
					bFoundMatch = true;
					iSearchStart = iSearchEnd;
					break;
				}
			}
			break;
		}
	}

	return (bFoundMatch ? iSearchStart : -1);
}

CAG2MoveTokenIndex CXRAG2::GetMatchingMoveTokenInt(int16 _iSwitchState, int _RefVal) const
{
	// Action val's already sorted in compile time, just find the best one to use or 
	// take the default one
	const CXRAG2_SwitchState* pSwitchState = GetSwitchState(_iSwitchState);
	if (!pSwitchState || pSwitchState->GetNumActionVal() == 0)
		return -1;

	int32 iActionValStart = pSwitchState->GetBaseActionValIndex();
	// Last actionval is the default one
	int32 NumActionVals = pSwitchState->GetNumActionVal()-1;
	int32 iSearchStart = iActionValStart;
	int32 iSearchEnd = iSearchStart + NumActionVals - 1;
	bool bFoundMatch = false;

	while (iSearchEnd >= iSearchStart)
	{
		// Go left or right?
		int32 Middle = (iSearchEnd - iSearchStart);
		if (Middle > 1)
		{
			//Middle = M_Floor(Middle / 2);
			Middle = Middle / 2;
			int16 Value = m_lSwitchStateActionVals[iSearchStart + Middle].m_ConstantInt - _RefVal;
			if (Value == 0)
			{
				// Ok, got value match
				iSearchStart += Middle;
				bFoundMatch = true;
				break;
			}
			else if (Value < 0)
			{
				// Go Right
				iSearchStart += Middle + 1;
			}
			else
			{
				// Go left
				iSearchEnd  = iSearchStart + Middle - 1;
			}
		}
		else
		{
			// Ok, found nothing, check the last two reactions
			if (m_lSwitchStateActionVals[iSearchStart].m_ConstantInt == _RefVal)
			{
				bFoundMatch = true;
				break;
			}
			if (iSearchStart != iSearchEnd && (m_lSwitchStateActionVals[iSearchEnd].m_ConstantInt == _RefVal))
			{
				bFoundMatch = true;
				iSearchStart = iSearchEnd;
				break;
			}
			break;
		}
	}
	// If we haven't found a match return the default one
	if (!bFoundMatch)
		iSearchStart = pSwitchState->GetBaseActionValIndex() + NumActionVals;

	const CXRAG2_SwitchStateActionVal* pActionVal = GetSwitchStateActionVal(iSearchStart);
	return pActionVal->GetMoveTokenIndex();
}

// Same as above I guess but with graphblocks instead
CAG2GraphBlockIndex CXRAG2::GetMatchingGraphBlock(CXRAG2_Impulse _Impulse) const
{
	// Search through blocks to see if we can find a match
	int32 iBlockStart = 0;
	int32 NumBlocks = GetNumGraphBlocks();
	int32 iSearchStart = iBlockStart;
	int32 iSearchEnd = iSearchStart + NumBlocks - 1;
	bool bFoundMatch = false;

	while (true)
	{
		// Go left or right?
		int32 Middle = (iSearchEnd - iSearchStart);
		if (Middle > 1)
		{
			//Middle = M_Floor(Middle / 2);
			Middle = Middle / 2;
			int16 Type = m_lGraphBlocks[iSearchStart + Middle].m_Condition.m_ImpulseType - _Impulse.m_ImpulseType;
			if (Type == 0)
			{
				// Ok, got type match, check if the values match
				int16 Value = m_lGraphBlocks[iSearchStart + Middle].m_Condition.m_ImpulseValue;
				// Special case, this reaction might react to anything
				if (Value == -1)
				{
					iSearchStart += Middle;
					bFoundMatch = true;
					break;
				}
				Value -= _Impulse.m_ImpulseValue;
				if (Value == 0)
				{
					iSearchStart += Middle;
					bFoundMatch = true;
					break;
				}
				else if (Value < 0)
				{
					// Go right
					iSearchStart += Middle + 1;
				}
				else
				{
					// Go Left
					iSearchEnd  = iSearchStart + Middle - 1;
				}
			}
			else if (Type < 0)
			{
				// Go Right
				iSearchStart += Middle + 1;
			}
			else
			{
				// Go left
				iSearchEnd  = iSearchStart + Middle - 1;
			}
		}
		else
		{
			// Ok, found nothing, check the last two reactions
			if (m_lGraphBlocks[iSearchStart].m_Condition.m_ImpulseType == _Impulse.m_ImpulseType)
			{
				int16 Value = m_lGraphBlocks[iSearchStart].m_Condition.m_ImpulseValue;
				if ((Value == -1) || ((Value - _Impulse.m_ImpulseValue) == 0))
				{
					bFoundMatch = true;
					break;
				}
			}
			if (iSearchStart != iSearchEnd && (m_lGraphBlocks[iSearchEnd].m_Condition.m_ImpulseType == _Impulse.m_ImpulseType))
			{
				int16 Value = m_lGraphBlocks[iSearchEnd].m_Condition.m_ImpulseValue;
				if ((Value == -1) || ((Value - _Impulse.m_ImpulseValue) == 0))
				{
					bFoundMatch = true;
					iSearchStart = iSearchEnd;
					break;
				}
			}
			break;
		}
	}

	return (bFoundMatch ? iSearchStart : -1);
}

void CXRAG2::GetGraphBlockConstants(CXRAG2_Impulse _Impulse, TArray<CXRAG2_StateConstant>& _lConstants) const
{
	return GetGraphBlockConstants(GetGraphBlock(GetMatchingGraphBlock(_Impulse)),_lConstants);
}

void CXRAG2::GetGraphBlockConstants(const CXRAG2_GraphBlock* _pBlock, TArray<CXRAG2_StateConstant>& _lConstants) const
{
	if (!_pBlock)
		return;

	// From this graphblock add the constants
	int32 Start = _pBlock->GetBaseStateConstantIndex();
	int32 Len = _pBlock->GetNumStateConstants();
	TAP_RCD<const CXRAG2_StateConstant> lAGConstants = m_lStateConstants;
	TAP_RCD<CXRAG2_StateConstant> lConstants = _lConstants;
	for (int32 i = 0; i < Len; i++)
	{
		// Check if we need to set the
		const CXRAG2_StateConstant& Constant = lAGConstants[i + Start];
		for (int32 j = 0; j < lConstants.Len(); j++)
		{
			if (lConstants[j].m_ID == Constant.m_ID)
			{
				lConstants[j].m_Value = Constant.m_Value;
				break;
			}
		}
	}
}

#ifndef M_RTM

//--------------------------------------------------------------------------------

int16 CXRAG2::GetExportedActionIndexFromActionIndex(CAG2ActionIndex _iAction) const
{
	for (int iActionHashEntry = 0; iActionHashEntry < m_lActionHashEntries.Len(); iActionHashEntry++)
	{
		if (m_lActionHashEntries[iActionHashEntry].m_iAction == _iAction)
			return iActionHashEntry;
	}
	return AG2_EXPORTEDACTIONINDEX_NULL;
}

//--------------------------------------------------------------------------------

int16 CXRAG2::GetExportedActionAnimIndex(int16 _iExportedAction) const
{
	CStr ExportedActionName = GetExportedActionName(_iExportedAction);
	uint32 ActionHashKey = StringToHash(ExportedActionName);

	int16 iAction = GetActionIndexFromHashKey(ActionHashKey);
	const CXRAG2_Action* pAction = GetAction(iAction);
	if (pAction == NULL)
		return AG2_ANIMINDEX_NULL;

	// First movetoken in action
	if (pAction->GetNumMoveTokens() > 0)
	{
		const CXRAG2_MoveToken* pMoveToken = GetMoveToken(pAction->GetBaseMoveTokenIndex());
		int16 iState = pMoveToken->GetTargetStateIndex();
		const CXRAG2_State* pState = GetState(iState);
		if (pState == NULL || !pState->GetNumAnimLayers())
			return AG2_ANIMINDEX_NULL;

		int16 iAnimLayer = pState->GetBaseAnimLayerIndex();
		const CXRAG2_AnimLayer* pAnimLayer = GetAnimLayer(iAnimLayer);
		if (pAnimLayer == NULL)
			return AG2_ANIMINDEX_NULL;

		int16 iAnim = pAnimLayer->GetAnimIndex();
		return iAnim;
	}
	else
	{
		return AG2_ANIMINDEX_NULL;
	}
}

//--------------------------------------------------------------------------------

CStr CXRAG2::GetExportedStateName(CAG2StateIndex _iState) const
{
	if (m_lExportedStateNames.ValidPos(_iState))
		return m_lExportedStateNames[_iState];
	return "";
}

CStr CXRAG2::GetExportedSwitchStateName(CAG2StateIndex _iState) const
{
	if (m_lExportedSwitchStateNames.ValidPos(_iState))
		return m_lExportedSwitchStateNames[_iState];
	return "";
}

CStr CXRAG2::GetExportedGraphBlockName(CAG2GraphBlockIndex _iGraphBlock) const
{
	if (m_lExportedGraphBlockNames.ValidPos(_iGraphBlock))
		return m_lExportedGraphBlockNames[_iGraphBlock];
	return "";
}

//--------------------------------------------------------------------------------

CStr CXRAG2::GetExportedPropertyConditionNameFromIndex(uint8 _iProperty) const
{
	if (m_lExportedPropertyConditionNames.ValidPos(_iProperty))
		return m_lExportedPropertyConditionNames[_iProperty].m_Name;
	return "";
}

CStr CXRAG2::GetExportedPropertyFloatNameFromIndex(uint8 _iProperty) const
{
	if (m_lExportedPropertyFloatNames.ValidPos(_iProperty))
		return m_lExportedPropertyFloatNames[_iProperty].m_Name;
	return "";
}

CStr CXRAG2::GetExportedPropertyFunctionNameFromIndex(uint8 _iProperty) const
{
	if (m_lExportedPropertyFunctionNames.ValidPos(_iProperty))
		return m_lExportedPropertyFunctionNames[_iProperty].m_Name;
	return "";
}

CStr CXRAG2::GetExportedPropertyIntNameFromIndex(uint8 _iProperty) const
{
	if (m_lExportedPropertyIntNames.ValidPos(_iProperty))
		return m_lExportedPropertyIntNames[_iProperty].m_Name;
	return "";
}

CStr CXRAG2::GetExportedPropertyBoolNameFromIndex(uint8 _iProperty) const
{
	if (m_lExportedPropertyBoolNames.ValidPos(_iProperty))
		return m_lExportedPropertyBoolNames[_iProperty].m_Name;
	return "";
}

//--------------------------------------------------------------------------------

CStr CXRAG2::GetExportedOperatorNameFromIndex(uint8 _iOperator) const
{
	if (m_lExportedOperatorNames.ValidPos(_iOperator))
		return m_lExportedOperatorNames[_iOperator].m_Name;
	return "";
}

//--------------------------------------------------------------------------------

CStr CXRAG2::GetExportedEffectNameFromIndex(uint8 _iEffect) const
{
	if (m_lExportedEffectNames.ValidPos(_iEffect))
		return m_lExportedEffectNames[_iEffect].m_Name;
	return "";
}

//--------------------------------------------------------------------------------

CStr CXRAG2::GetExportedStateConstantNameFromIndex(uint8 _iStateConstant) const
{
	if (m_lExportedStateConstantNames.ValidPos(_iStateConstant))
		return m_lExportedStateConstantNames[_iStateConstant].m_Name;
	return "";
}

CStr CXRAG2::GetExportedImpulseTypeNameFromIndex(uint8 _iImpulseType) const
{
	if (m_lExportedImpulseTypeNames.ValidPos(_iImpulseType))
		return m_lExportedImpulseTypeNames[_iImpulseType].m_Name;
	return "";
}

CStr CXRAG2::GetExportedImpulseValueNameFromIndex(uint8 _iImpulseValue) const
{
	if (m_lExportedImpulseValueNames.ValidPos(_iImpulseValue))
		return m_lExportedImpulseValueNames[_iImpulseValue].m_Name;
	return "";
}

int CXRAG2::GetExportedImpulseValueFromIndex(uint8 _iImpulseValue) const
{
	if (m_lExportedImpulseValueNames.ValidPos(_iImpulseValue))
		return m_lExportedImpulseValueNames[_iImpulseValue].m_Value;
	return 0;
}

int CXRAG2::GetExportedImpulseTypeValueFromIndex(uint8 _iImpulseType) const
{
	if (m_lExportedImpulseTypeNames.ValidPos(_iImpulseType))
		return m_lExportedImpulseTypeNames[_iImpulseType].m_Value;
	return 0;
}

//--------------------------------------------------------------------------------
CStr CXRAG2::GetExportedPropertyConditionNameFromID(CAG2PropertyID _PropertyID) const
{
	for (int iProperty = 0; iProperty < m_lExportedPropertyConditionNames.Len(); iProperty++)
	{
		if (m_lExportedPropertyConditionNames[iProperty].m_ID == _PropertyID)
			return m_lExportedPropertyConditionNames[iProperty].m_Name;
	}
	return "";
}

CStr CXRAG2::GetExportedPropertyFunctionNameFromID(CAG2PropertyID _PropertyID) const
{
	for (int iProperty = 0; iProperty < m_lExportedPropertyFunctionNames.Len(); iProperty++)
	{
		if (m_lExportedPropertyFunctionNames[iProperty].m_ID == _PropertyID)
			return m_lExportedPropertyFunctionNames[iProperty].m_Name;
	}
	return "";
}

CStr CXRAG2::GetExportedPropertyFloatNameFromID(CAG2PropertyID _PropertyID) const
{
	for (int iProperty = 0; iProperty < m_lExportedPropertyFloatNames.Len(); iProperty++)
	{
		if (m_lExportedPropertyFloatNames[iProperty].m_ID == _PropertyID)
			return m_lExportedPropertyFloatNames[iProperty].m_Name;
	}
	return "";
}

CStr CXRAG2::GetExportedPropertyIntNameFromID(CAG2PropertyID _PropertyID) const
{
	for (int iProperty = 0; iProperty < m_lExportedPropertyIntNames.Len(); iProperty++)
	{
		if (m_lExportedPropertyIntNames[iProperty].m_ID == _PropertyID)
			return m_lExportedPropertyIntNames[iProperty].m_Name;
	}
	return "";
}

CStr CXRAG2::GetExportedPropertyBoolNameFromID(CAG2PropertyID _PropertyID) const
{
	for (int iProperty = 0; iProperty < m_lExportedPropertyBoolNames.Len(); iProperty++)
	{
		if (m_lExportedPropertyBoolNames[iProperty].m_ID == _PropertyID)
			return m_lExportedPropertyBoolNames[iProperty].m_Name;
	}
	return "";
}

//--------------------------------------------------------------------------------

CStr CXRAG2::GetExportedOperatorNameFromID(CAG2OperatorID _OperatorID) const
{
	for (int iOperator = 0; iOperator < m_lExportedOperatorNames.Len(); iOperator++)
	{
		if (m_lExportedOperatorNames[iOperator].m_ID == _OperatorID)
			return m_lExportedOperatorNames[iOperator].m_Name;
	}
	return "";
}

//--------------------------------------------------------------------------------

CStr CXRAG2::GetExportedEffectNameFromID(CAG2EffectID _EffectID) const
{
	for (int iEffect = 0; iEffect < m_lExportedEffectNames.Len(); iEffect++)
	{
		if (m_lExportedEffectNames[iEffect].m_ID == _EffectID)
			return m_lExportedEffectNames[iEffect].m_Name;
	}
	return "";
}

//--------------------------------------------------------------------------------

CStr CXRAG2::GetExportedStateConstantNameFromID(CAG2StateConstantID _StateConstantID) const
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
