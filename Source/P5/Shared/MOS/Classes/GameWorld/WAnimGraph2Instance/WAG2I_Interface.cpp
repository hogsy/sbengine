//--------------------------------------------------------------------------------

#include "PCH.h"

#include "WAG2I.h"
#include "WAG2_ClientData.h"

//--------------------------------------------------------------------------------

bool CWAG2I::EvaluateCondition(const CWAG2I_Context* _pContext, const CXRAG2_ConditionNodeV2* _pNode, fp32& _TimeFraction)
{
	//MSCOPESHORT(CWAG2I::EvaluateCondition);
	M_ASSERT(m_pEvaluator, "!");
	// FIXME DIVIDE INTO FLOAT/INT/BOOL!!!!!!
	CXRAG2_ICallbackParams Params;
	CAG2Val Constant;
	int32 PropertyType = _pNode->GetPropertyType();
	switch(PropertyType)
	{
	case AG2_PROPERTYTYPE_CONDITION:
	case AG2_PROPERTYTYPE_FLOAT:
		Constant = CAG2Val::From(_pNode->GetConstantFloat());
		break;
	case AG2_PROPERTYTYPE_FUNCTION:
	case AG2_PROPERTYTYPE_INT:
		Constant = CAG2Val::From(_pNode->GetConstantInt());
		break;
	case AG2_PROPERTYTYPE_BOOL:
		Constant = CAG2Val::From(_pNode->GetConstantBool() != 0);
		break;
	}
	if (PropertyType == AG2_PROPERTYTYPE_CONDITION)
	{
		return m_pEvaluator->AG2_EvaluateCondition(_pContext, _pNode->GetProperty(),_pNode->GetPropertyType(), _pNode->GetOperator(), Constant, _TimeFraction);
	}
	else
	{
		return m_pEvaluator->AG2_DoProperty(_pContext, _pNode->GetProperty(),_pNode->GetPropertyType(), _pNode->GetOperator(), Constant, _TimeFraction);
	}
}

//--------------------------------------------------------------------------------

void CWAG2I::InvokeEffects(const CWAG2I_Context* _pContext, CAG2AnimGraphID _iAnimGraph, int16 _iBaseEffectInstance, uint8 _nEffectInstances)
{
	MSCOPESHORT(CWAG2I::InvokeEffects);
	const CXRAG2* pAnimGraph = GetAnimGraph(_iAnimGraph);
	M_ASSERT(m_pEvaluator && pAnimGraph, "!");

	for (int jEffectInstance = 0; jEffectInstance < _nEffectInstances; jEffectInstance++)
	{
		uint32 iEffectInstance = _iBaseEffectInstance + jEffectInstance;
		const CXRAG2_EffectInstance* pEffectInstance = pAnimGraph->GetEffectInstance(iEffectInstance);
		uint32 EffectID = pEffectInstance->m_ID;
		const CXRAG2_ICallbackParams& IParams = pAnimGraph->GetICallbackParams(pEffectInstance->m_iParams, pEffectInstance->m_nParams);
		m_pEvaluator->AG2_InvokeEffect(_pContext, EffectID, &IParams);
	}
}

//--------------------------------------------------------------------------------
