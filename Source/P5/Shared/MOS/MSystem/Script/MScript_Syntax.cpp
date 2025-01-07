

#include "PCH.h"

#include "MScript.h"

#if DScriptDebug>0
#pragma  optimize("",off)
#pragma  inline_depth(0)
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace NScript
{



	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



	class CParseHandler_Forward : public CParseHandler
	{
		MRTC_DECLARE;
	public:
		CParseHandler_Forward(CContext *_pParser, const char *_pName) : CParseHandler(_pParser, _pName)
		{
		}

		virtual int TryParse(CParseContext &_ParseContext)
		{
			for (int i = 0; i < m_splChildren.Len(); ++i)
			{
				int Ret = m_splChildren[i]->TryParse(_ParseContext);
				if (Ret)
					return Ret;
			}

			return 0;
		}
	};
	MRTC_IMPLEMENT(CParseHandler_Forward, CParseHandler);


	class CParseHandler_Expression : public CParseHandler_Forward
	{
		MRTC_DECLARE;
	public:
		
		CParseHandler_Expression(CContext *_pParser, const char *_pName) : CParseHandler_Forward(_pParser, _pName)
		{
		}

		virtual int TryParse(CParseContext &_ParseContext)
		{
			return CParseHandler_Forward::TryParse(_ParseContext);
		}

	};
	MRTC_IMPLEMENT(CParseHandler_Expression, CParseHandler);


	class CStatement_Function : public CStatement
	{
	public:
	 
		spCSymbol_Function m_spFunction;


		static void CleanupParam(CExecutionContext &_ExecutionContext)
		{
			CSymbol_Class *pClass;
			_ExecutionContext.Pop(pClass);
			void *pStack;
			_ExecutionContext.Pop(pStack);
			pClass->Destruct(pStack);
		}

		static void CleanupFunction(CExecutionContext &_ExecutionContext)
		{
			int ToFree;
			_ExecutionContext.Pop(ToFree);
			_ExecutionContext.PopFunctionContext();
			_ExecutionContext.Free(ToFree);
		}

		static void RunParam(CExecutionContext &_ExecutionContext)
		{
			CStatement_Function *pThis;
			_ExecutionContext.Pop(pThis);
			int iParam;
			_ExecutionContext.Pop(iParam);

			int FunctionStack = _ExecutionContext.GetFunctionPosition();
			void **pParameterStack = ((void **)_ExecutionContext.GetFunctionStackPtr()) + 1;

			pThis->m_spFunction->m_splArgumentTypes[iParam]->Construct(pParameterStack[iParam]);

			_ExecutionContext.PushFunctionContext();
			_ExecutionContext.PushFunctionContextPoper();
			_ExecutionContext.SetFunctionPosition(FunctionStack + sizeof(void *) * (iParam + 1));
			_ExecutionContext.PushEvaluator(pThis->m_splStatements[iParam]);
		}
		
		virtual void Evaluate(CExecutionContext &_ExecutionContext)
		{
			// Get input parameters
			M_ASSERT(m_spFunction->m_nStackVariables >= m_spFunction->m_splArgumentTypes.Len() + 1, "Error");
			int ToFree = 0;
			int FunctionStack = _ExecutionContext.GetStackPosition();
			void **pReturnStack = (void **)_ExecutionContext.Alloc(m_spFunction->m_nStackVariables * sizeof(void *));
			void **pParameterStack = pReturnStack + 1;

			ToFree += m_spFunction->m_nStackVariables * sizeof(void *);

			for (int i = 0; i < m_spFunction->m_splArgumentTypes.Len(); ++i)
			{
				ToFree += m_spFunction->m_splArgumentTypes[i]->m_DataSize;
				pParameterStack[i] = _ExecutionContext.Alloc(m_spFunction->m_splArgumentTypes[i]->m_DataSize);
			}

			// Get return pointer from parent
			*pReturnStack = *((void **)_ExecutionContext.GetFunctionStackPtr());

			// First push cleanup
			_ExecutionContext.PushFunctionContext();
			_ExecutionContext.Push(ToFree);
			_ExecutionContext.PushTask(CleanupFunction, "CleanupFunction");

			_ExecutionContext.SetFunctionPosition(FunctionStack);

			for (int i = m_spFunction->m_splArgumentTypes.Len() - 1; i >= 0; --i)
			{
				CSymbol_Class *pClass = m_spFunction->m_splArgumentTypes[i];
				_ExecutionContext.Push(pParameterStack[i]);
				_ExecutionContext.Push(pClass);
				_ExecutionContext.PushTask(CleanupParam, "CleanupParam");
			}

			_ExecutionContext.PushEvaluator(m_spFunction);
            
			// Evaluate expression back to front
			for (int i = 0; i < m_spFunction->m_splArgumentTypes.Len(); ++i)
			{
				_ExecutionContext.Push(i);
				_ExecutionContext.Push(this);
				_ExecutionContext.PushTask(RunParam, "RunParam");
			}

		}

		virtual int PostParse(int _iParent, CStr &_ErrorReturn)
		{
			M_ASSERT(m_pParentStatement->m_splStatements.Len() >= (3 + _iParent), "Must be so you know");		

			spCStatement spThis = this;
			spCStatement spReceiver = m_pParentStatement->m_splStatements[_iParent + 1];
			spReceiver->m_splStatements.Add(this);
			m_pParentStatement->m_splStatements[_iParent + 2]->m_pParentStatement = this;
			spCStatement spParams = m_pParentStatement->m_splStatements[_iParent + 2];
//			m_splStatements.Add(m_pParentStatement->m_splStatements[_iParent + 2]);
			for (int i = 0; i < spParams->m_splStatements.Len(); ++i)
			{
				spParams->m_splStatements[i]->m_pParentStatement = this;
				m_splStatements.Add(spParams->m_splStatements[i]);
			}
//			m_pParentStatement->m_splStatements.Clear();
//			m_pParentStatement->m_splStatements.Add(spReceiver);
			m_pParentStatement->m_splStatements.Del(_iParent+2); // Parameters
			m_pParentStatement->m_splStatements.Del(_iParent); // This

			m_pParentStatement = spReceiver;
			if (!CStatement::PostParse(_iParent, _ErrorReturn))
				return false;

			if (!m_splStatements.Len())
			{
				if (m_spFunction->m_splArgumentTypes.Len())
				{
					_ErrorReturn = CStrF("%s takes %d parameters, 0 provided", m_spFunction->m_Name.Str(), m_spFunction->m_splArgumentTypes.Len());
					return false;
				}
				return 2;
			}

			if (m_splStatements.Len() != m_spFunction->m_splArgumentTypes.Len())
			{
				_ErrorReturn = CStrF("%s takes %d parameters, %d provided", m_spFunction->m_Name.Str(), m_spFunction->m_splArgumentTypes.Len(), m_splStatements.Len());
				return false;
			}

			for (int i = 0; i < m_splStatements.Len(); ++i)
			{
				spCSymbol_Class spReturn = m_splStatements[i]->GetReturnType();

				if (!spReturn)
				{
					_ErrorReturn = "Symbol of wrong type sent to function";
					return false;
				}


				if (spReturn->m_Name != "void")
				{
					spCStatement spStatement = m_splStatements[i];
					// Remove Receiver
					spCStatement spTemp = m_splStatements[i]->m_splStatements[0];
					m_splStatements[i] = spTemp;
					m_splStatements[i]->m_pParentStatement = this;
				}
				else
				{
					_ErrorReturn = "void is not a valid type for a parameter to a function";
					return false;
				}

				if (spReturn != m_spFunction->m_splArgumentTypes[i])
				{
					bool bConvertFail = true;
					if (m_pFunction)
					{
						CStr ConvertSymbol = CStrF("@Convert@%s@%s", spReturn->m_Name.Str(), m_spFunction->m_splArgumentTypes[i]->m_Name.Str());
						spCSymbol spConvert = m_pFunction->GetSymbol(ConvertSymbol, true);
						if (spConvert)
						{
#ifdef _DEBUG
							if (!(spConvert->m_ClassType & ESymbol_Function))
							{
								_ErrorReturn = CStrF("Convert Symbol(%s) is of wrong symbol type", spConvert->m_Name.Str());
								return false;
							}
							if (((CSymbol_Function*)(CSymbol*)spConvert)->m_spReturnType->m_Name != m_spFunction->m_splArgumentTypes[i]->m_Name)
							{
								_ErrorReturn = CStrF("Convert Symbol(%s) returns wrong type", spConvert->m_Name.Str());
								return false;
							}
							if (((CSymbol_Function*)(CSymbol*)spConvert)->m_splArgumentTypes.Len() != 1)
							{
								_ErrorReturn = CStrF("Convert Symbol(%s) takes wrong number of arguments", spConvert->m_Name.Str());
								return false;
							}
							if (((CSymbol_Function*)(CSymbol*)spConvert)->m_splArgumentTypes[0]->m_Name != spReturn->m_Name)
							{
								_ErrorReturn = CStrF("Convert Symbol(%s) takes wrong argument type for parameter 0", spConvert->m_Name.Str());
								return false;
							}
#endif
							TPtr<CStatement_Function> spStatement = MNew(CStatement_Function);
							spStatement->m_pFunction = m_pFunction;
							spStatement->m_pParentStatement = this;
							spStatement->m_spFunction = (CSymbol_Function *)(CSymbol *)spConvert;	
							spStatement->m_splStatements.Add(m_splStatements[i]);
							m_splStatements[i] = spStatement;

							bConvertFail = false;
						}
					}

					if (bConvertFail)
					{
						_ErrorReturn = CStrF("%s takes %s for parameter %d, %s was provided and no applicable conversion was found", m_spFunction->m_Name.Str(), m_spFunction->m_splArgumentTypes[i]->m_Name.Str(), i, spReturn->m_Name.Str());
						return false;
					}
				}
			}

			return 2;
		}

		virtual CSymbol_Class *GetReturnType()
		{
			return m_spFunction->m_spReturnType;
		}

		virtual CSymbol *GetSymbol()
		{
			return m_spFunction;
		}

		virtual CStr GetDesc()
		{
			return "CParseHandler_Function (" + m_spFunction->m_Name + ")";
		}

	};


	class CParseHandler_ExpressionIdentifier : public CParseHandler
	{
		MRTC_DECLARE;
	public:
		
		CParseHandler_ExpressionIdentifier(CContext *_pParser, const char *_pName) : CParseHandler(_pParser, _pName)
		{
		}

		int TryParse(CParseContext &_ParseContext);

	};
	MRTC_IMPLEMENT(CParseHandler_ExpressionIdentifier, CParseHandler);

	class CParseHandler_ExpressionOperator : public CParseHandler
	{
		MRTC_DECLARE;
	public:
		
		CParseHandler_ExpressionOperator(CContext *_pParser, const char *_pName) : CParseHandler(_pParser, _pName)
		{
		}

		int TryParse(CParseContext &_ParseContext);

	};
	MRTC_IMPLEMENT(CParseHandler_ExpressionOperator, CParseHandler);

	class CParseHandler_ExpressionTerminate : public CParseHandler
	{
		MRTC_DECLARE;
	public:
		
		CParseHandler_ExpressionTerminate(CContext *_pParser, const char *_pName) : CParseHandler(_pParser, _pName)
		{
		}

		virtual int TryParse(CParseContext &_ParseContext)
		{
			const char *pStr = _ParseContext.m_pStr;
			ParseWhiteSpace(pStr);

			if (*pStr == ',')
			{
				++pStr;

				CStatement_Holder *pHolder = MNew(CStatement_Holder);
				pHolder->m_pFunction = _ParseContext.m_pFunctionScope->m_pFunction;
				pHolder->m_pParentStatement = _ParseContext.m_pFunctionStatementHolder->m_pParentStatement;
				_ParseContext.m_pFunctionStatementHolder->m_pParentStatement->m_splStatements.Add(pHolder);
				_ParseContext.m_pFunctionStatementHolder = pHolder;

				// Todo: Add check so you cannoct do may expressions with space
				_ParseContext.m_pStr = pStr;

				return 1;
			}

			return 0;
		}
	};
	MRTC_IMPLEMENT(CParseHandler_ExpressionTerminate, CParseHandler);

	class CParseHandler_ExpressionStatementTerminate : public CParseHandler
	{
		MRTC_DECLARE;
	public:
		
		CParseHandler_ExpressionStatementTerminate(CContext *_pParser, const char *_pName) : CParseHandler(_pParser, _pName)
		{
		}

		virtual int TryParse(CParseContext &_ParseContext)
		{
			const char *pStr = _ParseContext.m_pStr;
			ParseWhiteSpace(pStr);

			if (*pStr == ';')
			{
				if (_ParseContext.m_iParanthesis != 0)
				{
					_ParseContext.SetError("Unexpected ; found in expression", pStr);
					return -1;
				}

				CStatement_Holder *pHolder = MNew(CStatement_Holder);
				pHolder->m_pFunction = _ParseContext.m_pFunctionScope->m_pFunction;
				pHolder->m_pParentStatement = _ParseContext.m_pFunctionScope;
				_ParseContext.m_pFunctionScope->m_splStatements.Add(pHolder);
				_ParseContext.m_pFunctionStatementHolder = pHolder;

				_ParseContext.m_pStr = pStr + 1;

				return 1;
			}

			return 0;
		}
	};
	MRTC_IMPLEMENT(CParseHandler_ExpressionStatementTerminate, CParseHandler);



	class CParseHandler_ExpressionPananthesisEnter : public CParseHandler
	{
		MRTC_DECLARE;
	public:
		
		CParseHandler_ExpressionPananthesisEnter(CContext *_pParser, const char *_pName) : CParseHandler(_pParser, _pName)
		{
		}

		virtual int TryParse(CParseContext &_ParseContext)
		{
			const char *pStr = _ParseContext.m_pStr;
			ParseWhiteSpace(pStr);

			if (*pStr == '(')
			{
				{
					CStatement_Holder *pHolder = MNew(CStatement_Holder);
					pHolder->m_pFunction = _ParseContext.m_pFunctionScope->m_pFunction;
					pHolder->m_pParentStatement = _ParseContext.m_pFunctionStatementHolder;
					_ParseContext.m_pFunctionStatementHolder->m_splStatements.Add(pHolder);
					_ParseContext.m_pFunctionStatementHolder = pHolder;
				}

				{
					CStatement_Holder *pHolder = MNew(CStatement_Holder);
					pHolder->m_pFunction = _ParseContext.m_pFunctionScope->m_pFunction;
					pHolder->m_pParentStatement = _ParseContext.m_pFunctionStatementHolder;
					_ParseContext.m_pFunctionStatementHolder->m_splStatements.Add(pHolder);
					_ParseContext.m_pFunctionStatementHolder = pHolder;
				}

				++_ParseContext.m_iParanthesis;
				_ParseContext.m_pStr = pStr + 1;

				return 1;
			}

			return 0;
		}
	};
	MRTC_IMPLEMENT(CParseHandler_ExpressionPananthesisEnter, CParseHandler);

	class CParseHandler_ExpressionPananthesisExit : public CParseHandler
	{
		MRTC_DECLARE;
	public:
		
		CParseHandler_ExpressionPananthesisExit(CContext *_pParser, const char *_pName) : CParseHandler(_pParser, _pName)
		{
		}

		virtual int TryParse(CParseContext &_ParseContext)
		{
			const char *pStr = _ParseContext.m_pStr;
			ParseWhiteSpace(pStr);

			if (*pStr == ')')
			{
				_ParseContext.m_pFunctionStatementHolder = (CStatement_Holder *)_ParseContext.m_pFunctionStatementHolder->m_pParentStatement;
				_ParseContext.m_pFunctionStatementHolder = (CStatement_Holder *)_ParseContext.m_pFunctionStatementHolder->m_pParentStatement;
				--_ParseContext.m_iParanthesis;
				_ParseContext.m_pStr = pStr + 1;

				return 1;
			}
			return 0;
		}
	};
	MRTC_IMPLEMENT(CParseHandler_ExpressionPananthesisExit, CParseHandler);

	class CStatement_Variable : public CStatement
	{
	public:

		spCSymbol_Variable m_spSymbol;
		
		virtual void Evaluate(CExecutionContext &_ExecutionContext)
		{
			_ExecutionContext.PushEvaluator(m_spSymbol);
		}

		virtual CSymbol_Class *GetReturnType()
		{
			return m_spSymbol->m_spClass;
		}

		virtual CSymbol *GetSymbol()
		{
			return m_spSymbol;
		}

		virtual int PostParse(int _iParent, CStr &_ErrorReturn)
		{
			spCStatement spThis = this;
			spCStatement spReceiver = m_pParentStatement->m_splStatements[_iParent + 1];
			spReceiver->m_splStatements.Add(this);
			m_pParentStatement->m_splStatements.Del(_iParent);
			m_pParentStatement = spReceiver;

			if (!CStatement::PostParse(_iParent, _ErrorReturn))
				return false;

			return 1;
		}

		virtual CStr GetDesc()
		{
			return "CStatement_Variable (" + m_spSymbol->m_spClass->m_Name + " " + m_spSymbol->m_Name + ")";
		}

	};

	class CSymbol_VariableLocal : public CSymbol_Variable
	{
	public:
		int m_iParam;

		CSymbol_VariableLocal(CSymbol *_pParent, const char *_pName) : CSymbol_Variable(_pParent, _pName)
		{
			m_ClassType |= ESymbol_Variable_Script;
		}

		virtual void Evaluate(CExecutionContext &_Context)
		{
			const void **pDataSource = (const void **)((uint8 *)_Context.GetUserFunctionStackPtr() + ((m_iParam) * sizeof(void *)));
			void **pDataDest = (void **)_Context.GetFunctionStackPtr();
			m_spClass->Copy(*pDataDest, *pDataSource);			
		}

		virtual void EvaluateSet(CExecutionContext &_Context)
		{
			void **pDataDest = (void **)((uint8 *)_Context.GetUserFunctionStackPtr() + ((m_iParam) * sizeof(void *)));
			const void **pDataSource = (const void **)_Context.GetFunctionStackPtr();
			m_spClass->Copy(*pDataDest, *pDataSource);			
		}

	};

	class CStatement_LocalVariable : public CStatement
	{
	public:

		TPtr<CSymbol_VariableLocal> m_spSymbol;
		
		virtual void Evaluate(CExecutionContext &_ExecutionContext)
		{
			void **pData = (void **)((uint8 *)_ExecutionContext.GetUserFunctionStackPtr() + ((m_spSymbol->m_iParam) * sizeof(void *)));
			*pData = _ExecutionContext.Alloc(m_spSymbol->m_spClass->m_DataSize);
			m_spSymbol->m_spClass->Construct(*pData);
		}

		virtual CStr GetDesc()
		{
			return "CStatement_LocalVariableConstruct (" + m_spSymbol->m_spClass->m_Name + " " + m_spSymbol->m_Name + ")";
		}

	};

	class CStatement_VariableSetter : public CStatement
	{
	public:

		spCSymbol_Class m_spType;
		spCSymbol_Variable m_spVar;

		static void Epilogue(CExecutionContext &_ExecutionContext)
		{
 			void **pReturnStack = (void **)_ExecutionContext.GetFunctionStackPtr();
			CSymbol_Class *pType;
			_ExecutionContext.Pop(pType);			
			pType->Destruct(*pReturnStack);
			_ExecutionContext.PopFunctionContext();
			_ExecutionContext.Free(pType->m_DataSize + sizeof(void *));
		}

		static void SetVariable(CExecutionContext &_ExecutionContext)
		{
			CStatement_VariableSetter *pThis;
			_ExecutionContext.Pop(pThis);
			pThis->m_spVar->EvaluateSet(_ExecutionContext);
		}
		
		virtual void Evaluate(CExecutionContext &_ExecutionContext)
		{
			// Get input parameters
			int FunctionStack = _ExecutionContext.GetStackPosition();
			void **pReturnStack = (void **)_ExecutionContext.Alloc(sizeof(void *));

			// Get return pointer from parent
			*pReturnStack = _ExecutionContext.Alloc(m_spType->m_DataSize);
			m_spType->Construct(*pReturnStack);

			_ExecutionContext.PushFunctionContext();
			_ExecutionContext.SetFunctionPosition(FunctionStack);

			CSymbol_Class *pType = m_spType;
			_ExecutionContext.Push(pType);			
			_ExecutionContext.PushTask(Epilogue, "CStatement_VariableSetter::Epilogue");

			_ExecutionContext.Push(this);
			_ExecutionContext.PushTask(SetVariable, "CStatement_VariableSetter::SetVariable");			

			_ExecutionContext.PushEvaluator(m_splStatements[0]);
		}

		virtual int PostParse(int _iParent, CStr &_ErrorReturn)
		{	
			M_ASSERT(m_pParentStatement->m_splStatements.Len() == 1, "Must be so you know");		
			if (m_spType->m_Name == "void")
			{
				spCStatement spThis = this;
				m_splStatements[0]->m_pParentStatement = m_pParentStatement;
				m_pParentStatement->m_splStatements[_iParent] = m_splStatements[0];
			}
			return 1;
		}

		virtual CSymbol_Class *GetReturnType()
		{
			return m_spType;
		}

		virtual CSymbol *GetSymbol()
		{
			return m_spType;
		}

		virtual CStr GetDesc()
		{
			return "CStatement_VariableSetter (" + m_spType->m_Name + " " + m_spVar->m_Name + ")";
		}
	};


	class CStatement_OperatorTemp : public CStatement
	{
	public:

		TPtr<CSymbol_Operator> m_spSymbol;
		
		virtual void Evaluate(CExecutionContext &_ExecutionContext)
		{
			M_ASSERT(0, "Should be exchanged");
		}

		CSymbol_Function *GetSingleOperatorFunc(CStatement *_pStatement, int _Type)
		{
			CSymbol_Class *pRet = _pStatement->GetReturnType();
			if (!pRet)
				return NULL;

			CStr SymbolName = CStrF("%d@", _Type) + pRet->m_Name;

			CSymbol_Function *pFunc = (CSymbol_Function *)m_spSymbol->GetSymbol(SymbolName, false);

			if (!pFunc || !(pFunc->m_ClassType & ESymbol_Function))
				return NULL;

			return pFunc;
		}

		CSymbol_Function *GetDualOperatorFunc(CStatement *_pStatement0, CStatement *_pStatement1, int _Type)
		{
			CSymbol_Class *pRet0 = _pStatement0->GetReturnType();
			CSymbol_Class *pRet1 = _pStatement1->GetReturnType();
			if (!pRet0 || !pRet1)
				return NULL;

			CStr SymbolName = CStrF("%d@", _Type) + pRet0->m_Name + "@" + pRet1->m_Name;

			CSymbol_Function *pFunc = (CSymbol_Function *)m_spSymbol->GetSymbol(SymbolName, false);

			if (!pFunc || !(pFunc->m_ClassType & ESymbol_Function))
				return NULL;

			return pFunc;
		}

		int PostParse_UnaryPrefix(int _iParent, CStr &_ErrorReturn)
		{
			CSymbol_Function * pFunc = GetSingleOperatorFunc(m_pParentStatement->m_splStatements[_iParent + 1], EOperatorType_UnaryPrefix);
			if (pFunc)
			{
				TPtr<CStatement_Receiver> spReceiver;
				{
					TPtr<CStatement_Receiver> spStatement = MNew(CStatement_Receiver);
					spReceiver = spStatement;
					spStatement->m_pParentStatement = m_pParentStatement;
					spStatement->m_pFunction = m_pFunction;
					spStatement->m_spType = pFunc->m_spReturnType;	
				}

				TPtr<CStatement_Function> spStatement = MNew(CStatement_Function);
				spStatement->m_pParentStatement = spReceiver;
				spReceiver->m_splStatements.Add((CStatement *)spStatement);
				spReceiver->m_pFunction = m_pFunction;
				spStatement->m_spFunction = pFunc;	
				m_pParentStatement->m_splStatements[_iParent + 1]->m_pParentStatement = spStatement;
				spStatement->m_splStatements.Add(m_pParentStatement->m_splStatements[_iParent + 1]->m_splStatements[0]);
				m_pParentStatement->m_splStatements[_iParent] = spReceiver;
				m_pParentStatement->m_splStatements.Del(_iParent + 1);

				return 1;
			}

			return -1;
		}

		int PostParse_VariablePrefix(int _iParent, CStr &_ErrorReturn)
		{
			CSymbol_Function * pFunc = GetSingleOperatorFunc(m_pParentStatement->m_splStatements[_iParent + 1], EOperatorType_VariablePrefix);
			if (pFunc)
			{
				CSymbol *pSymb = m_pParentStatement->m_splStatements[_iParent + 1]->m_splStatements[0]->GetSymbol();
				if (!pSymb || !(pSymb->m_ClassType & ESymbol_Variable))
				{
					_ErrorReturn = "This operator can only be applied to a variable (l-value)";
					return 0;
				}

				int iStatmentPlace = -1;
				CStatement_Scope *pScope = m_pParentStatement->GetScope(iStatmentPlace);
				m_pParentStatement->m_splStatements.Del(_iParent);

				TPtr<CStatement_Holder> spHolder = MNew(CStatement_Holder);
				spHolder->m_pFunction = m_pFunction;
				spHolder->m_pParentStatement = pScope;
				pScope->m_splStatements.Insert(iStatmentPlace+1, (CStatement *)spHolder);

				TPtr<CStatement_VariableSetter> spReceiver;
				{
					TPtr<CStatement_VariableSetter> spStatement = MNew(CStatement_VariableSetter);
					spReceiver = spStatement;
					spStatement->m_pFunction = m_pFunction;
					spStatement->m_spType = pFunc->m_spReturnType;
					spStatement->m_spVar = (CSymbol_Variable *)pSymb;
					spStatement->m_pParentStatement = spHolder;
				}
				
				TPtr<CStatement_Function> spStatement = MNew(CStatement_Function);
				spStatement->m_pFunction = m_pFunction;
				spStatement->m_pParentStatement = spHolder;
				spStatement->m_spFunction = pFunc;	
				spHolder->m_splStatements.Add((CStatement *)spStatement);
				spHolder->m_splStatements.Add((CStatement *)spReceiver);

				{
					TPtr<CStatement_Holder> spStatement = MNew(CStatement_Holder);
					spStatement->m_pFunction = m_pFunction;
					spStatement->m_pParentStatement = spHolder;
					spHolder->m_splStatements.Add((CStatement *)spStatement);

					TPtr<CStatement_Variable> spVar = MNew(CStatement_Variable);
					spVar->m_pFunction = m_pFunction;
					spVar->m_spSymbol = (CSymbol_Variable *)pSymb;
					spVar->m_pParentStatement = spStatement;
					spStatement->m_splStatements.Add((CStatement *)spVar);

					TPtr<CStatement_Receiver> spReceiver = MNew(CStatement_Receiver);
					spReceiver->m_pFunction = m_pFunction;
					spReceiver->m_spType = ((CSymbol_Variable *)pSymb)->m_spClass;
					spReceiver->m_pParentStatement = spStatement;
					spStatement->m_splStatements.Add((CStatement *)spReceiver);
				}


				return 2;
			}
			return -1;
		}

		int PostParse_VariablePostfix(int _iParent, CStr &_ErrorReturn)
		{
			CSymbol_Function * pFunc = GetSingleOperatorFunc(m_pParentStatement->m_splStatements[_iParent - 1], EOperatorType_VariablePostfix);
			if (pFunc)
			{
				CSymbol *pSymb = m_pParentStatement->m_splStatements[_iParent - 1]->m_splStatements[0]->GetSymbol();
				if (!pSymb || !(pSymb->m_ClassType & ESymbol_Variable))
				{
					_ErrorReturn = "This operator can only be applied to a variable (l-value)";
					return 0;
				}

				int iStatmentPlace = -1;
				CStatement_Scope *pScope = m_pParentStatement->GetScope(iStatmentPlace);
				m_pParentStatement->m_splStatements.Del(_iParent);

				TPtr<CStatement_Holder> spHolder = MNew(CStatement_Holder);
				spHolder->m_pFunction = m_pFunction;
				spHolder->m_pParentStatement = pScope;
				pScope->m_splStatements.Insert(iStatmentPlace+1, (CStatement *)spHolder);

				TPtr<CStatement_VariableSetter> spReceiver;
				{
					TPtr<CStatement_VariableSetter> spStatement = MNew(CStatement_VariableSetter);
					spStatement->m_pFunction = m_pFunction;
					spReceiver = spStatement;
					spStatement->m_spType = pFunc->m_spReturnType;
					spStatement->m_spVar = (CSymbol_Variable *)pSymb;
					spStatement->m_pParentStatement = spHolder;
				}
				
				TPtr<CStatement_Function> spStatement = MNew(CStatement_Function);
				spStatement->m_pFunction = m_pFunction;
				spStatement->m_pParentStatement = spHolder;
				spStatement->m_spFunction = pFunc;	
				spHolder->m_splStatements.Add((CStatement *)spStatement);
				spHolder->m_splStatements.Add((CStatement *)spReceiver);

				{
					TPtr<CStatement_Holder> spStatement = MNew(CStatement_Holder);
					spStatement->m_pFunction = m_pFunction;
					spStatement->m_pParentStatement = spHolder;
					spHolder->m_splStatements.Add((CStatement *)spStatement);

					TPtr<CStatement_Variable> spVar = MNew(CStatement_Variable);
					spVar->m_pFunction = m_pFunction;
					spVar->m_spSymbol = (CSymbol_Variable *)pSymb;
					spVar->m_pParentStatement = spStatement;
					spStatement->m_splStatements.Add((CStatement *)spVar);

					TPtr<CStatement_Receiver> spReceiver = MNew(CStatement_Receiver);
					spReceiver->m_pFunction = m_pFunction;
					spReceiver->m_spType = ((CSymbol_Variable *)pSymb)->m_spClass;
					spReceiver->m_pParentStatement = spStatement;
					spStatement->m_splStatements.Add((CStatement *)spReceiver);
				}


				return 2;
			}

			return -1;
		}

		int PostParse_Binary(int _iParent, CStr &_ErrorReturn)
		{
			CSymbol_Function * pFunc = GetDualOperatorFunc(m_pParentStatement->m_splStatements[_iParent - 1], m_pParentStatement->m_splStatements[_iParent + 1], EOperatorType_Binary);
			if (pFunc)
			{
				spCStatement spLeft = m_pParentStatement->m_splStatements[_iParent - 1];
				spCStatement spRight = m_pParentStatement->m_splStatements[_iParent + 1];
				spCStatement spThis = this;
				m_pParentStatement->m_splStatements.Del(_iParent + 1); // Right
				m_pParentStatement->m_splStatements.Del(_iParent - 1); // Left
				m_pParentStatement->m_splStatements.Del(_iParent - 1); // This

				TPtr<CStatement_Receiver> spReceiver;
				{
					TPtr<CStatement_Receiver> spStatement = MNew(CStatement_Receiver);
					spReceiver = spStatement;
					spStatement->m_pParentStatement = m_pParentStatement;
					spStatement->m_pFunction = m_pFunction;
					spStatement->m_spType = pFunc->m_spReturnType;	
				}

				TPtr<CStatement_Function> spStatement = MNew(CStatement_Function);
				spStatement->m_pParentStatement = spReceiver;
				spReceiver->m_splStatements.Add((CStatement *)spStatement);
				spReceiver->m_pFunction = m_pFunction;
				spStatement->m_spFunction = pFunc;	
				spLeft->m_pParentStatement = spStatement;
				spRight->m_pParentStatement = spStatement;
				spStatement->m_splStatements.Add(spLeft->m_splStatements[0]); // Remove reciever
				spStatement->m_splStatements.Add(spRight->m_splStatements[0]); // Remove reciever

				m_pParentStatement->m_splStatements.Insert(_iParent - 1, (CStatement *)spReceiver);

				return 2;
			}

			return -1;
		}

		int PostParse_Assignment(int _iParent, CStr &_ErrorReturn)
		{
			return -1;
		}

		virtual int PostParse(int _iParent, CStr &_ErrorReturn)
		{
			spCStatement spThis = this;

			if (_iParent == 0)
			{
				// Only prefix operators allowed
				if (m_pParentStatement->m_splStatements.Len() < 2)
				{
					_ErrorReturn = "This operator needs a statement to work on";
					return 0;
				}

				if (!m_pParentStatement->m_splStatements[_iParent + 1]->PostParse(_iParent + 1, _ErrorReturn))
					return 0;

				int Ret = PostParse_VariablePrefix(_iParent, _ErrorReturn);
				if (Ret >= 0)
					return Ret;

				Ret = PostParse_UnaryPrefix(_iParent, _ErrorReturn);
				if (Ret >= 0)
					return Ret;

				_ErrorReturn = "No operator matching the input types was found";
				return 0;
			}
			else
			{
				if (m_pParentStatement->m_splStatements.Len() < 2)
				{
					_ErrorReturn = "This operator needs a statement to work on";
					return 0;
				}

				if (m_pParentStatement->m_splStatements.Len() == 2)
				{
					// Only Postfix
					int Ret = PostParse_VariablePostfix(_iParent, _ErrorReturn);
					if (Ret >= 0)
						return Ret;
				}
				else
				{
					if (!m_pParentStatement->m_splStatements[_iParent + 1]->PostParse(_iParent + 1, _ErrorReturn))
						return 0;

					int Ret = PostParse_UnaryPrefix(_iParent, _ErrorReturn);
					if (Ret >= 0)
						return Ret;

					Ret = PostParse_Binary(_iParent, _ErrorReturn);
					if (Ret >= 0)
						return Ret;

					Ret = PostParse_Assignment(_iParent, _ErrorReturn);
					if (Ret >= 0)
						return Ret;

				}

				_ErrorReturn = "No operator matching the input types was found";
				return 0;		
			}

/*			spCStatement spThis = this;
			spCStatement spReceiver = m_pParentStatement->m_splStatements[1];
			spReceiver->m_splStatements.Add(this);

			m_pParentStatement->m_splStatements.Clear();
			m_pParentStatement->m_splStatements.Add(spReceiver);

			m_pParentStatement = spReceiver;
			if (!CStatement::PostParse(_iParent, _ErrorReturn))
				return false;

			return 2;*/
			return 1;
		}

		virtual CStr GetDesc()
		{
			return "CStatement_OperatorTemp (" + m_spSymbol->m_Name + ")";
		}

	};

	int CParseHandler_ExpressionIdentifier::TryParse(CParseContext &_ParseContext)
	{


		const char *pStr = _ParseContext.m_pStr;

		ParseWhiteSpace(pStr);

		const char *pIdentifier = pStr;
		ParseIdentifier(pStr);
		const char *pIdentifierEnd = pStr;

		if (pIdentifier == pIdentifierEnd)
			return 0;

		CStr Ident;
		Ident.Capture(pIdentifier, pIdentifierEnd - pIdentifier);

		CSymbol *pSymbol = _ParseContext.m_pParentSymbol->GetSymbol(Ident, true);

		if (pSymbol)
		{
			if (pSymbol->m_ClassType & ESymbol_Function)
			{
				ParseWhiteSpace(pStr);
				if (*pStr != '(')
				{
					_ParseContext.SetError("Expected ( for calling a function", pStr);
					return -1;
				}

/*				if (_ParseContext.m_pFunctionStatementHolder->m_splStatements.Len())
				{
					_ParseContext.SetError("Function call does not make sense here");
					return -1;
				}*/

				TPtr<CStatement_Function> spFunctionStatement = MNew(CStatement_Function);
				spFunctionStatement->m_pFunction = _ParseContext.m_pFunctionScope->m_pFunction;
				spFunctionStatement->m_pParentStatement = _ParseContext.m_pFunctionStatementHolder;
				_ParseContext.m_pFunctionStatementHolder->m_splStatements.Add((CStatement *)spFunctionStatement);
				spFunctionStatement->m_spFunction = (CSymbol_Function *)pSymbol;	

				TPtr<CStatement_Receiver> spReceiver = MNew(CStatement_Receiver);
				spReceiver->m_pFunction = _ParseContext.m_pFunctionScope->m_pFunction;
				spReceiver->m_pParentStatement = _ParseContext.m_pFunctionStatementHolder;
				_ParseContext.m_pFunctionStatementHolder->m_splStatements.Add((CStatement *)spReceiver);
				spReceiver->m_spType = ((CSymbol_Function *)pSymbol)->m_spReturnType;	

				_ParseContext.m_pStr = pStr;
				return 1;

			}
			else if (pSymbol->m_ClassType & ESymbol_Constant)
			{
				TPtr<CStatement_Constant> spVariableStatement = MNew(CStatement_Constant);
				spVariableStatement->m_pFunction = _ParseContext.m_pFunctionScope->m_pFunction;
				spVariableStatement->m_pParentStatement = _ParseContext.m_pFunctionStatementHolder;
				_ParseContext.m_pFunctionStatementHolder->m_splStatements.Add((CStatement *)spVariableStatement);
				spVariableStatement->m_spSymbol = (CSymbol_Constant *)pSymbol;	

				TPtr<CStatement_Receiver> spReceiver = MNew(CStatement_Receiver);
				spReceiver->m_pFunction = _ParseContext.m_pFunctionScope->m_pFunction;
				spReceiver->m_pParentStatement = _ParseContext.m_pFunctionStatementHolder;
				_ParseContext.m_pFunctionStatementHolder->m_splStatements.Add((CStatement *)spReceiver);
				spReceiver->m_spType = ((CSymbol_Constant *)pSymbol)->m_spClass;

				_ParseContext.m_pStr = pStr;
				return 1;
			}
			else if (pSymbol->m_ClassType & ESymbol_Variable)
			{
				TPtr<CStatement_Variable> spVariableStatement = MNew(CStatement_Variable);
				spVariableStatement->m_pFunction = _ParseContext.m_pFunctionScope->m_pFunction;
				spVariableStatement->m_pParentStatement = _ParseContext.m_pFunctionStatementHolder;
				_ParseContext.m_pFunctionStatementHolder->m_splStatements.Add((CStatement *)spVariableStatement);
				spVariableStatement->m_spSymbol = (CSymbol_Variable *)pSymbol;	

				TPtr<CStatement_Receiver> spReceiver = MNew(CStatement_Receiver);
				spReceiver->m_pFunction = _ParseContext.m_pFunctionScope->m_pFunction;
				spReceiver->m_pParentStatement = _ParseContext.m_pFunctionStatementHolder;
				_ParseContext.m_pFunctionStatementHolder->m_splStatements.Add((CStatement *)spReceiver);
				spReceiver->m_spType = ((CSymbol_Variable *)pSymbol)->m_spClass;

				_ParseContext.m_pStr = pStr;
				return 1;
			}
			else if (pSymbol->m_ClassType & ESymbol_Class)
			{
				ParseWhiteSpace(pStr);

				pIdentifier = pStr;
				ParseIdentifier(pStr);
				pIdentifierEnd = pStr;

				if (pIdentifier == pIdentifierEnd)
					return 0;

				CStr Ident;
				Ident.Capture(pIdentifier, pIdentifierEnd - pIdentifier);

				if (_ParseContext.m_pParentSymbol->GetSymbol(Ident, false))
				{
					_ParseContext.SetError("Symbol " + Ident + " is already defined", pIdentifier);
					return -1;
				}

				TPtr<CStatement_LocalVariable> spLocalVar = MNew(CStatement_LocalVariable);
				spLocalVar->m_pFunction = _ParseContext.m_pFunctionScope->m_pFunction;
				spLocalVar->m_spSymbol = MNew2(CSymbol_VariableLocal, _ParseContext.m_pParentSymbol, Ident);
				spLocalVar->m_spSymbol->m_iParam = _ParseContext.m_pFunctionScope->m_pFunction->AllocateStackVariable();
				spLocalVar->m_spSymbol->m_spClass = (CSymbol_Class *)pSymbol;
				spLocalVar->m_pParentStatement = _ParseContext.m_pFunctionStatementHolder;
				_ParseContext.m_pFunctionStatementHolder->m_splStatements.Add((CStatement *)spLocalVar);
                
				_ParseContext.m_pStr = pStr;
				return 1;
			}
		}

		return 0;
	}

	int CParseHandler_ExpressionOperator::TryParse(CParseContext &_ParseContext)
	{
		const char *pStr = _ParseContext.m_pStr;

		ParseWhiteSpace(pStr);

		const char *pIdentifier = pStr;
		ParseNonIdent(pStr);
		const char *pIdentifierEnd = pStr;

		// We support either a totally non identifier character operator or a totally alphanumeric identifier i.e. --++ is valid and NOT is valid but +not- is not valid
		if (pIdentifier == pIdentifierEnd)
			ParseIdentifier(pStr);

		if (pIdentifier == pIdentifierEnd)
			return 0;

		CStr Ident;
		Ident.Capture(pIdentifier, pIdentifierEnd - pIdentifier);

		CSymbol *pSymbol = _ParseContext.m_pParentSymbol->GetSymbol(Ident, true);

		if (pSymbol)
		{

			if (pSymbol->m_ClassType & ESymbol_Operator)
			{
				CSymbol_Operator *pOperator = (CSymbol_Operator *)pSymbol;

				TPtr<CStatement_OperatorTemp> spStatement = MNew(CStatement_OperatorTemp);
				spStatement->m_pFunction = _ParseContext.m_pFunctionScope->m_pFunction;
				spStatement->m_pParentStatement = _ParseContext.m_pFunctionStatementHolder;
				_ParseContext.m_pFunctionStatementHolder->m_splStatements.Add((CStatement *)spStatement);
				spStatement->m_spSymbol = pOperator;

				_ParseContext.m_pStr = pStr;
				return 1;
			}
		}

		return 0;
	}

	class CSymbol_UserFunction_Parameter : public CSymbol_Variable
	{
	public:
		CSymbol_UserFunction_Parameter(CSymbol *_pParent, const char *_pName) : CSymbol_Variable(_pParent, _pName)
		{
			m_ClassType |= ESymbol_Variable_Script;
			m_iParameter = 0;
			m_pUserFunction = NULL;
		}

		CSymbol_UserFunction *m_pUserFunction;
		int m_iParameter;

		void Evaluate(CExecutionContext &_Context)
		{
			void *pDest = *((void **)_Context.GetFunctionStackPtr());
			m_pUserFunction->m_splArgumentTypes[m_iParameter]->Copy(pDest, *((const void **)((const uint8 *)_Context.GetUserFunctionStackPtr() + (m_iParameter + 1) * sizeof(void *))));
		}

		void EvaluateSet(CExecutionContext &_Context)
		{
			const void *pDest = *((const void **)_Context.GetFunctionStackPtr());
			m_pUserFunction->m_splArgumentTypes[m_iParameter]->Copy(*((void **)((const uint8 *)_Context.GetUserFunctionStackPtr() + (m_iParameter + 1) * sizeof(void *))), pDest);
		}

	};

	class CParseHandler_FunctionParameters : public CParseHandler
	{
		MRTC_DECLARE;
	public:
		
		CParseHandler_FunctionParameters(CContext *_pParser, const char *_pName) : CParseHandler(_pParser, _pName)
		{
		}

		virtual int TryParse(CParseContext &_ParseContext)
		{
			const char *pStr = _ParseContext.m_pStr;

			ParseWhiteSpace(pStr);

			if (!NStr::CharIsAnsiAlphabetical(*pStr))
				return 0;

			const char *pStartType = pStr;
			ParseIdentifier(pStr);
			CStr Str;
			Str.Capture(pStartType, pStr - pStartType);

			CSymbol *pType = _ParseContext.m_pParentSymbol->GetSymbol(Str, true);

			CSymbol_UserFunction *pUserFunc = (CSymbol_UserFunction *)_ParseContext.m_pParentSymbol;

			if (pType && (pType->m_ClassType & ESymbol_Class))
			{
				ParseWhiteSpace(pStr);
				const char *pStartIdentifier = pStr;
				ParseIdentifier(pStr);
				CStr Ident;
				Ident.Capture(pStartIdentifier, pStr - pStartIdentifier);

				if (!Ident.Len())
					return 0;

				TPtr<CSymbol_UserFunction_Parameter> spParameter = MNew2(CSymbol_UserFunction_Parameter, pUserFunc, Ident);
				spParameter->m_spClass = (CSymbol_Class *)pType;
				spParameter->m_iParameter = pUserFunc->m_splArgumentTypes.Len();
				spParameter->m_pUserFunction = pUserFunc;
				pUserFunc->m_splArgumentTypes.Add((CSymbol_Class *)pType);

				ParseWhiteSpace(pStr);
				if (*pStr == ',')
					++pStr;

				_ParseContext.m_pStr = pStr;

				return 1;
			}

			return 0;
		}
	};
	MRTC_IMPLEMENT(CParseHandler_FunctionParameters, CParseHandler);

	class CParseHandler_FunctionScopeEnter : public CParseHandler
	{
		MRTC_DECLARE;
	public:
		
		CParseHandler_FunctionScopeEnter(CContext *_pParser, const char *_pName) : CParseHandler(_pParser, _pName)
		{
		}

		virtual int TryParse(CParseContext &_ParseContext)
		{
			const char *pStr = _ParseContext.m_pStr;
			ParseWhiteSpace(pStr);
			if (*pStr == '{')
			{
				CParseContext Context = _ParseContext;
				Context.m_iParseDepth = _ParseContext.m_iParseDepth + 1;
				Context.m_pStr = pStr + 1;
				Context.m_pParserScope = this;


				Context.m_pFunctionScope = MNew(CStatement_Scope);
				Context.m_pFunctionScope->m_NameSpace.m_pParent = Context.m_pParentSymbol;
				Context.m_pParentSymbol = &Context.m_pFunctionScope->m_NameSpace;
				Context.m_pFunctionScope->m_pFunction = _ParseContext.m_pFunctionScope->m_pFunction;
				Context.m_pFunctionScope->m_pParentStatement = _ParseContext.m_pFunctionScope;
				_ParseContext.m_pFunctionScope->m_splStatements.Add(Context.m_pFunctionScope);

				// This holder holds the destruct functions
				Context.m_pFunctionStatementHolder = MNew(CStatement_Holder);
				Context.m_pFunctionStatementHolder->m_pFunction = Context.m_pFunctionScope->m_pFunction;
				Context.m_pFunctionStatementHolder->m_pParentStatement = Context.m_pFunctionScope;
				Context.m_pFunctionScope->m_splStatements.Add(Context.m_pFunctionStatementHolder);

				// This is where the expressions go
				Context.m_pFunctionStatementHolder = MNew(CStatement_Holder);
				Context.m_pFunctionStatementHolder->m_pFunction = Context.m_pFunctionScope->m_pFunction;
				Context.m_pFunctionStatementHolder->m_pParentStatement = Context.m_pFunctionScope;
				Context.m_pFunctionScope->m_splStatements.Add(Context.m_pFunctionStatementHolder);

				Context.m_pParserScope->Parse(Context);
				if (Context.m_Error)
				{
					
					_ParseContext.CopyError(Context);
					return -1;
				}
				_ParseContext.m_pStr = Context.m_pStr;
				_ParseContext.m_iParseDepth = Context.m_iParseDepth;
				if (Context.m_iParseDepth)
					return 1;
				else
					return 2;
			}
			else
				return 0;
		}
	};
	MRTC_IMPLEMENT(CParseHandler_FunctionScopeEnter, CParseHandler);

	class CParseHandler_FunctionScopeExit : public CParseHandler
	{
		MRTC_DECLARE;
	public:
		
		CParseHandler_FunctionScopeExit(CContext *_pParser, const char *_pName) : CParseHandler(_pParser, _pName)
		{
		}

		virtual int TryParse(CParseContext &_ParseContext)
		{
			const char *pStr = _ParseContext.m_pStr;
			ParseWhiteSpace(pStr);
			if (*pStr == '}')
			{
				if (_ParseContext.m_pFunctionStatementHolder->m_splStatements.Len())
				{
					_ParseContext.SetError("Missing ;", pStr);
					return -1;
				}

				_ParseContext.m_pParentSymbol = _ParseContext.m_pFunctionScope->m_NameSpace.m_pParent;

				--_ParseContext.m_iParseDepth;
				_ParseContext.m_pStr = pStr + 1;
				return 2;
			}
			else
				return 0;
		}
	};
	MRTC_IMPLEMENT(CParseHandler_FunctionScopeExit, CParseHandler);

	class CParseHandler_Function : public CParseHandler
	{
		MRTC_DECLARE;
	public:
		
		CParseHandler_Function(CContext *_pParser, const char *_pName) : CParseHandler(_pParser, _pName)
		{
		}

		spCParseHandler m_spFunctionBody;
		spCParseHandler m_spParameters;

		void Clear()
		{
			m_spFunctionBody->Clear();
			m_spParameters->Clear();
			m_spFunctionBody = NULL;
			m_spParameters = NULL;
		}

		virtual int TryParse(CParseContext &_ParseContext)
		{
			const char *pStr = _ParseContext.m_pStr;

			ParseWhiteSpace(pStr);

			if (!NStr::CharIsAnsiAlphabetical(*pStr))
				return false;

			const char *pStartType = pStr;
			ParseIdentifier(pStr);
			CStr Str;
			Str.Capture(pStartType, pStr - pStartType);

			CSymbol *pReturn = _ParseContext.m_pParentSymbol->GetSymbol(Str, true);

			if (pReturn && (pReturn->m_ClassType & ESymbol_Class))
			{
				CSymbol_Class *pReturnSymbol = (CSymbol_Class *)pReturn;
				ParseWhiteSpace(pStr);
				const char *pIdentifier = pStr;
				ParseIdentifier(pStr);
				const char *pIdentifierEnd = pStr;
				if (pIdentifier == pIdentifierEnd)
					return 0;
				ParseWhiteSpace(pStr);

				// This is a function
				if (*pStr == '(')
				{
					CStr Identifier;
					Identifier.Capture(pIdentifier, pIdentifierEnd - pIdentifier);

					spCSymbol spIdent = _ParseContext.m_pParentSymbol->GetSymbol(Identifier, false);

					if (spIdent)
					{
						_ParseContext.SetError("Identifier already defined", pIdentifier);
						return -1;
					}

					
					TPtr<CSymbol_UserFunction> spIdentifier = MNew2(CSymbol_UserFunction, (CSymbol *)NULL, Identifier);
					spIdentifier->m_spReturnType = pReturnSymbol;
					spIdent = spIdentifier;

					// Parse function parameters
					{
						const char *pStr1 = pStr;
						int iCurrentParathesis = 0;
						while (*pStr1)
						{
							if (*pStr1 == '(')
								++iCurrentParathesis;
							else if (*pStr1 == ')')
								--iCurrentParathesis;

							++pStr1;
							if (iCurrentParathesis == 0)
								break;
						}

						if (iCurrentParathesis > 0)
						{
							_ParseContext.SetError("End of file encountered searching for matching brace", pStr);
							return -1;
						}

						CStr Defs;
						Defs.Capture(pStr + 1, (pStr1 - pStr) - 2);

						pStr = pStr1;

						CParseContext Context = _ParseContext;
						Context.m_pStr = Defs.Str();
						Context.m_pParentSymbol = spIdent;
						Context.m_pParserScope = m_spParameters;

						M_TRY
						{
							_ParseContext.m_pParentSymbol->CastNameSpace()->AddSymbol(spIdentifier);
							m_spParameters->Parse(Context);
						}
						M_CATCH(
						catch (CCException)
						{
							_ParseContext.m_pParentSymbol->CastNameSpace()->RemoveSymbol(spIdentifier);
							throw;
						}
						)
						_ParseContext.m_pParentSymbol->CastNameSpace()->RemoveSymbol(spIdentifier);

						if (Context.m_Error)
						{
							_ParseContext.CopyError(Context);
							return -1;
						}
					}

					// Parse Function contents
					{
						ParseWhiteSpace(pStr);

						if (*pStr != '{')
						{
							_ParseContext.SetError("Expected {", pStr);
							return -1;
						}

						spIdentifier->m_nStackVariables = spIdentifier->m_splArgumentTypes.Len() + 1;

						CParseContext Context = _ParseContext;
						Context.m_pStr = pStr;
						Context.m_pParentSymbol = spIdent;
						Context.m_pParserScope = m_spFunctionBody;
						Context.m_pFunctionScope = &(spIdentifier->m_RootScope);
						spIdent->m_pParent = _ParseContext.m_pParentSymbol;
						m_spFunctionBody->Parse(Context);
						if (Context.m_Error)
						{
							_ParseContext.CopyError(Context);
							return -1;
						}
						CStr Err;
#if DScriptDebug >= 1
						M_TRACE("Preparse tree:\n\n");
						spIdentifier->m_RootScope.TraceInfo(0);
						M_TRACE("\n\nPostparse tree:\n\n");
#endif
						if (!spIdentifier->m_RootScope.PostParse(0, Err))
						{
							_ParseContext.SetError(Err, -1);
							return -1;
						}
#if DScriptDebug >= 1
						spIdentifier->m_RootScope.TraceInfo(0);
						M_TRACE("\n\n");
#endif
						spIdent->m_pParent = NULL;

						if (Context.m_iParseDepth != _ParseContext.m_iParseDepth)
						{
							_ParseContext.SetError("Unexpected end of file found while parsing for }", pStr);
							return -1;
						}

						_ParseContext.m_pStr = Context.m_pStr;
					}

					_ParseContext.m_pParentSymbol->CastNameSpace()->AddSymbol(spIdentifier);
					return 1;
				}

			}

			return 0;
		}
	};
	MRTC_IMPLEMENT(CParseHandler_Function, CParseHandler);

	class CParseHandler_VariableDef : public CParseHandler
	{
		MRTC_DECLARE;
	public:
		
		CParseHandler_VariableDef(CContext *_pParser, const char *_pName) : CParseHandler(_pParser, _pName)
		{
		}

		virtual int TryParse(CParseContext &_ParseContext)
		{
			return 0;
		}
	};
	MRTC_IMPLEMENT(CParseHandler_VariableDef, CParseHandler);

	class CParseHandler_ClassFunction : public CParseHandler
	{
		MRTC_DECLARE;
	public:
		
		CParseHandler_ClassFunction(CContext *_pParser, const char *_pName) : CParseHandler(_pParser, _pName)
		{
		}

		virtual int TryParse(CParseContext &_ParseContext)
		{
			return 0;
		}

	};
	MRTC_IMPLEMENT(CParseHandler_ClassFunction, CParseHandler);

	class CParseHandler_Class : public CParseHandler
	{
		MRTC_DECLARE;
	public:
		
		CParseHandler_Class(CContext *_pParser, const char *_pName) : CParseHandler(_pParser, _pName)
		{
		}

		virtual int TryParse(CParseContext &_ParseContext)
		{
			return 0;
		}
	};
	MRTC_IMPLEMENT(CParseHandler_Class, CParseHandler);

	class CParseHandler_ClassVariableDef : public CParseHandler
	{
		MRTC_DECLARE;
	public:
		
		CParseHandler_ClassVariableDef(CContext *_pParser, const char *_pName) : CParseHandler(_pParser, _pName)
		{
		}

		virtual int TryParse(CParseContext &_ParseContext)
		{
			return 0;
		}
	};
	MRTC_IMPLEMENT(CParseHandler_ClassVariableDef, CParseHandler);


	class CParseHandler_WhiteSpace : public CParseHandler
	{
		MRTC_DECLARE;
	public:
		
		CParseHandler_WhiteSpace(CContext *_pParser, const char *_pName) : CParseHandler(_pParser, _pName)
		{
		}

		virtual int TryParse(CParseContext &_ParseContext)
		{
			if (NStr::CharIsWhiteSpace(*_ParseContext.m_pStr)
				|| (_ParseContext.m_pStr[0] == '/' && (_ParseContext.m_pStr[1] == '/' || _ParseContext.m_pStr[1] == '*')))
			{
				ParseWhiteSpace(_ParseContext.m_pStr);
				return 1;
			}
			return 0;
		}
	};
	MRTC_IMPLEMENT(CParseHandler_WhiteSpace, CParseHandler);

	void CContext::RegisterLanguage()
	{
		m_spRootParserScope = MNew2(CParseHandler, this, "Root");

		MNew2(CParseHandler_WhiteSpace, this, "WhiteSpace");

		MNew2(CParseHandler_VariableDef, this, "VariableDef");

		MNew2(CParseHandler_Class, this, "Class");
		MNew2(CParseHandler_ClassVariableDef, this, "ClassVariableDef");
		MNew2(CParseHandler_ClassFunction, this, "ClassFunction");
		MNew2(CParseHandler_Function, this, "Function");
		MNew2(CParseHandler, this, "FunctionParameters");
		MNew2(CParseHandler_FunctionParameters, this, "FunctionParametersParse");
		MNew2(CParseHandler_Expression, this, "Expression");

		MNew2(CParseHandler_FunctionScopeEnter, this, "FunctionScopeEnter");
		MNew2(CParseHandler_FunctionScopeExit, this, "FunctionScopeExit");

		// User extendable
		MNew2(CParseHandler_Forward, this, "UserLang");
		MNew2(CParseHandler_Forward, this, "UserConstant");

		// Expression
		MNew2(CParseHandler_ExpressionPananthesisEnter, this, "ExpressionParanthesisEnter");
		MNew2(CParseHandler_ExpressionPananthesisExit, this, "ExpressionParanthesisExit");
		MNew2(CParseHandler_ExpressionIdentifier, this, "ExpressionIdentifier");
		MNew2(CParseHandler_ExpressionOperator, this, "ExpressionOperator");
		MNew2(CParseHandler_ExpressionStatementTerminate, this, "ExpressionStatementTerminate");
		MNew2(CParseHandler_ExpressionTerminate, this, "ExpressionTerminate");
		

		m_spRootParserScope->AddChild(GetScope("WhiteSpace"));
		m_spRootParserScope->AddChild(GetScope("Function"));
		m_spRootParserScope->AddChild(GetScope("VariableDef"));
		m_spRootParserScope->AddChild(GetScope("Class"));

		// Function
		{
			CParseHandler_Function *pFunction = GetScope<CParseHandler_Function>("Function");

			pFunction->m_spParameters = GetScope("FunctionParameters");
			pFunction->m_spParameters->AddChild(GetScope("FunctionParametersParse"));

			pFunction->m_spFunctionBody = GetScope("FunctionScopeEnter");

			{
				CParseHandler_FunctionScopeEnter *pFunctionScope = GetScope<CParseHandler_FunctionScopeEnter>("FunctionScopeEnter");

				pFunctionScope->AddChild(GetScope("FunctionScopeEnter"));
				pFunctionScope->AddChild(GetScope("FunctionScopeExit"));
				pFunctionScope->AddChild(GetScope("UserLang"));
				pFunctionScope->AddChild(GetScope("Expression"));

				GetScope("Expression")->AddChild(GetScope("ExpressionParanthesisEnter"));
				GetScope("Expression")->AddChild(GetScope("ExpressionParanthesisExit"));
				GetScope("Expression")->AddChild(GetScope("ExpressionStatementTerminate"));
				GetScope("Expression")->AddChild(GetScope("ExpressionTerminate"));
				GetScope("Expression")->AddChild(GetScope("ExpressionIdentifier"));				
				GetScope("Expression")->AddChild(GetScope("ExpressionOperator"));
				GetScope("Expression")->AddChild(GetScope("UserConstant"));
			}
		}

		{
			CParseHandler_Class *pClass = GetScope<CParseHandler_Class>("Class");
			pClass->AddChild(pClass);
			pClass->AddChild(GetScope("WhiteSpace"));
			pClass->AddChild(GetScope("Class"));
			pClass->AddChild(GetScope("ClassFunction"));
			pClass->AddChild(GetScope("VariableDef"));
		}

	}

	/*

	White space
	{
	}

	Comment
	{
	}

	Variable Definition
	{

	}

	Class
	{
		->Class
		Variable definitions
		Class functions
	}

	Function
	{
		Scope
		{
			->White space
			->Comment
			->Scope
			UserLanguageDefs
			{
				// Examples:
				if
				for
				while
				break
				throw
				...
			}
			Expression
			{
				->White space
				->Comment
				Variable definition
				User constant parsing (i.e strings, numbers etc)
				{
					...
				}
				Operators
				Paranthesis
			}
		}
	}

	*/

}
