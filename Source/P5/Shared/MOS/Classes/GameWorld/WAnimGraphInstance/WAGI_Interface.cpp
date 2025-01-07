//--------------------------------------------------------------------------------

#include "PCH.h"

#include "WAGI.h"
#include "WAG_ClientData.h"

//--------------------------------------------------------------------------------

bool CWAGI::EvaluateCondition(const CWAGI_Context* _pContext, int16 _iStatePropertyParamBase, const CXRAG_ConditionNode* _pNode, fp32& _TimeFraction)
{
	//MSCOPESHORT(CWAGI::EvaluateCondition);
	CXRAG_ICallbackParams IParams = m_spAnimGraph->GetICallbackParams(_iStatePropertyParamBase + _pNode->GetPropertyParamsIndex(), _pNode->GetNumPropertyParams());
	M_ASSERT(m_pEvaluator, "!");
	return m_pEvaluator->AG_EvaluateCondition(_pContext, _pNode->GetProperty(), &IParams, _pNode->GetOperator(), CAGVal::From(_pNode->GetConstant()), _TimeFraction);
}

//--------------------------------------------------------------------------------

void CWAGI::InvokeEffects(const CWAGI_Context* _pContext, int16 _iBaseEffectInstance, uint8 _nEffectInstances)
{
	MSCOPESHORT(CWAGI::InvokeEffects);
	M_ASSERT(m_pEvaluator, "!");

	for (int jEffectInstance = 0; jEffectInstance < _nEffectInstances; jEffectInstance++)
	{
		uint32 iEffectInstance = _iBaseEffectInstance + jEffectInstance;
		const CXRAG_EffectInstance* pEffectInstance = m_spAnimGraph->GetEffectInstance(iEffectInstance);
		uint32 EffectID = pEffectInstance->m_ID;
		const CXRAG_ICallbackParams& IParams = m_spAnimGraph->GetICallbackParams(pEffectInstance->m_iParams, pEffectInstance->m_nParams);
		m_pEvaluator->AG_InvokeEffect(_pContext, EffectID, &IParams);
	}
}

//--------------------------------------------------------------------------------
