
#include "PCH.h"
#include "MScript.h"

#if DScriptDebug>0
#pragma  optimize("",off)
#pragma  inline_depth(0)
#endif

namespace NScript
{
	void CRegisterContext::AddSymbol(spCSymbol _spSymbol)
	{
		if (!m_pParent->AddSymbol(_spSymbol))
		{
			return;
			//M_BREAKPOINT;
			//Error("AddFunction", "The symbol already exists");
		}

		CRegisteredSymbol RegS;
		RegS.m_pParent = m_pParent;
		RegS.m_spSymbol = _spSymbol;
		m_lRegisteredSymbols.Add(RegS);
	}

	void CRegisterContext::AddOperator(CStr _Name, CSymbol_Function *_pFunction)
	{
		spCSymbol spSymbol = m_pParent->GetSymbol(_Name, false);

		if (!spSymbol)
		{
			spSymbol = MNew2(CSymbol_Operator, m_pParent, _Name); 
		}

		if (!(spSymbol->m_ClassType & ESymbol_Operator))
		{
			M_BREAKPOINT;
			Error("AddOperator", "The symbol isn't an operator");
			return;
		}

		CSymbol_Operator *pOperator = (CSymbol_Operator *)(CSymbol *)spSymbol;

/*		if (pOperator->m_OperatorType != _OperatorType)
		{
			M_BREAKPOINT;
			Error("AddOperator", "An operator of another type is already registered for this identifier");
			return;
		}*/

		CSymbol *pSymbol = pOperator->GetSymbol(_pFunction->m_Name, false);
		if (pSymbol)
		{
			M_BREAKPOINT;
			Error("AddOperator", "An operator with these types have already been registered");
			return;
		}

		pOperator->AddSymbol(_pFunction);

		CRegisteredSymbol RegS;
		RegS.m_pParent = pOperator;
		RegS.m_spSymbol = _pFunction;
		m_lRegisteredSymbols.Add(RegS);
	}

	CRegisterContext::~CRegisterContext()
	{
		for (int i = 0; i < m_lRegisteredSymbols.Len(); ++i)
		{
			m_lRegisteredSymbols[i].m_pParent->RemoveSymbol(m_lRegisteredSymbols[i].m_spSymbol);
		}
	}

	//template <>
	const char *NFunctionConvert::TFunctionCaller0<void>::ms_lArgTypes[] = {NULL};

	void CRegisterContext::CFunctionSymbol::Evaluate(CExecutionContext &_Context)
	{
		uint8 *pStack = (uint8 *)_Context.GetFunctionStackPtr();
		m_pCaller->Call(pStack + sizeof(void *), pStack);
	}

	void CRegisterContext::CConstantSymbol::Evaluate(CExecutionContext &_Context)
	{
		uint8 *pStack = (uint8 *)_Context.GetFunctionStackPtr();
		m_pGetFunction->Call(pStack + sizeof(void *), pStack);
	}

	void CRegisterContext::CVariableSymbol::Evaluate(CExecutionContext &_Context)
	{
		uint8 *pStack = (uint8 *)_Context.GetFunctionStackPtr();
		m_pGetFunction->Call(pStack + sizeof(void *), pStack);
	}

	void CRegisterContext::CVariableSymbol::EvaluateSet(CExecutionContext &_Context)
	{
		uint8 *pStack = (uint8 *)_Context.GetFunctionStackPtr();
		m_pSetFunction->Call(pStack + sizeof(void *), pStack);
	}
}

