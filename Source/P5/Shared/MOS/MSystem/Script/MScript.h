
#ifndef DInc_MScriptNew_h
#define DInc_MScriptNew_h

#include "../SysInc.h"

#define DScriptDebug 0

#if DScriptDebug>0
#pragma  optimize("",off)
#pragma  inline_depth(0)
#endif

#include "MScript_MultiType.h"
#include "MScript_Convert.h"

namespace NScript
{

	template <typename t_CChar>
	void ParseWhiteSpace(const t_CChar *&_pStr)
	{
		// Remove white space
	//	while (*_pStr && *_pStr == ' ')
	//		++_pStr;

		const t_CChar *pStr = _pStr;
		while (*pStr)
		{
			while (*pStr && NStr::CharIsWhiteSpace(*pStr))
				++pStr;

			if (pStr[0] == '/' && pStr[1] == '/')
			{
				// To end of line comment
				pStr += 2;

				while (*pStr)
				{
					if (*pStr == 0x0d || *pStr == 0x0a)
					{
						++pStr;
						// We are done
						break;
					}
					++pStr;
				}
			}
			else if ((pStr[0] == '/' && pStr[1] == '*'))
			{
				// /**/ Comment

				pStr += 2;
				while (*pStr)
				{
					if (pStr[0] == '*' && pStr[1] == '/')
					{
						pStr += 2;
						break;
					}
					++pStr;
				}
			}
			else
				break;
	        
		}

		_pStr = pStr;
	}

	template <typename t_CChar>
	void ParseAlphaNumeric(const t_CChar *&_pStr)
	{

		while (*_pStr && (NStr::CharIsAnsiAlphabetical(*_pStr) || NStr::CharIsNumber(*_pStr) || *_pStr == '_'))
			++_pStr;
	}

	template <typename t_CChar>
	void ParseNumeric(const t_CChar *&_pStr)
	{
		while (*_pStr && NStr::CharIsNumber(*_pStr))
			++_pStr;
	}	

	template <typename t_CChar>
	void ParseIdentifier(const t_CChar *&_pStr)
	{
		if (!(NStr::CharIsAnsiAlphabetical(*_pStr) || *_pStr == '_'))
			return;

		ParseAlphaNumeric(_pStr);
	}

	template <typename t_CChar>
	void ParseNonIdent(const t_CChar *&_pStr)
	{
		while (*_pStr && !NStr::CharIsWhiteSpace(*_pStr) && !NStr::CharIsAnsiAlphabetical(*_pStr) && !NStr::CharIsNumber(*_pStr) && *_pStr != '(' && *_pStr != ')' && *_pStr != '{' && *_pStr != '}' && *_pStr != ':' && *_pStr != ';' && *_pStr != '"' && *_pStr != '\'')
			++_pStr;
	}


	class CContext;

	enum ESymbol
	{
		ESymbol_Class_Registered = DBit(0),
		ESymbol_Class_Script = DBit(1),
		ESymbol_Function_Registered = DBit(2),
		ESymbol_Function_Script = DBit(3),
		ESymbol_ClassFunction_Registered = DBit(4),
		ESymbol_ClassFunction_Script = DBit(5),
		ESymbol_Variable_Script = DBit(6),
		ESymbol_Variable_Registered = DBit(7),
		ESymbol_Operator = DBit(8),
		ESymbol_Constant = DBit(9),
		ESymbol_NameSpace = DBit(10),
		ESymbol_Class = ESymbol_Class_Registered | ESymbol_Class_Script,
		ESymbol_Function = ESymbol_Function_Registered | ESymbol_Function_Script,
		ESymbol_ClassFunction = ESymbol_ClassFunction_Registered | ESymbol_ClassFunction_Script,
		ESymbol_Variable = ESymbol_Variable_Script | ESymbol_Variable_Registered,
		ESymbol_All = ESymbol_Constant | ESymbol_Class | ESymbol_Function | ESymbol_ClassFunction | ESymbol_Variable | ESymbol_Operator | ESymbol_NameSpace,
	};

	class CEvaluator
	{
	public:
		virtual void Evaluate(class CExecutionContext &_ExecutionContext) pure;
		virtual CStr GetDesc() pure;

	};

	class CSymbol : public CEvaluator, public CReferenceCount
	{
	public:

		int m_ClassType;

		CStr m_Name;

		CSymbol *m_pParent;

		CSymbol(CSymbol *_pParent, const char *_pName);
		class CCompare
		{
		public:
			DIdsPInlineS static aint Compare(const CSymbol *_pFirst, const CSymbol *_pSecond, void *_pContext)
			{
				return CStrBase::stricmp(_pFirst->m_Name, _pSecond->m_Name);
			}

			DIdsPInlineS static aint Compare(const CSymbol *_pTest, const char * _pKey, void *_pContext)
			{
				return CStrBase::stricmp(_pTest->m_Name, _pKey);
			}
		};

		DIdsTreeAVLAligned_Link(CSymbol, m_SymbolLink, const char *, CCompare);

		virtual ~CSymbol();

		virtual CSymbol *GetSymbol(const char *_pName, bool _bCheckParent)
		{
			if (m_pParent && _bCheckParent)
				return m_pParent->GetSymbol(_pName, true);
			return NULL;
		}

		virtual void Evaluate(class CExecutionContext &_Context)
		{
			Error("Evaluate", "This symbol isn't evaluatable");  

		}

		class CSymbol_NameSpace *CastNameSpace()
		{
			if (m_ClassType & ESymbol_NameSpace)
				return (class CSymbol_NameSpace *)this;
			else
				return NULL;
		}

		virtual CStr GetDesc()
		{
			return "Symbol:\t\t" + m_Name;
		}

		virtual void TraceInfo(int _Depth)
		{
		}

	};

	typedef TPtr<CSymbol> spCSymbol;
	typedef DIdsTreeAVLAligned_Iterator(CSymbol, m_SymbolLink, const char *, CSymbol::CCompare) CTreeIter;

	class CSymbol_NameSpace : public CSymbol
	{
	public:

		DIdsTreeAVLAligned_Tree(CSymbol, m_SymbolLink, const char *, CSymbol::CCompare) m_Children;
		
		CSymbol_NameSpace(CSymbol *_pParent, const char *_pName) : CSymbol(_pParent, _pName)
		{
			m_ClassType |= ESymbol_NameSpace;
		}

		virtual ~CSymbol_NameSpace()
		{
			CTreeIter Iter = m_Children;
			while (Iter)
			{
				Iter->m_pParent = NULL;
				++Iter;
			}

			m_Children.DeRefAll();
		}

		virtual CSymbol *GetSymbol(const char *_pName, bool _bCheckParent)
		{
			CSymbol *pSymbol = m_Children.FindEqual(_pName);
			if (pSymbol)
				return pSymbol;

			if (_bCheckParent)
				return CSymbol::GetSymbol(_pName, _bCheckParent);
			else
				return NULL;
		}

		virtual bool AddSymbol(CSymbol *_pSymbol)
		{
			if (m_Children.FindEqual(_pSymbol->m_Name.Str()))
				return false; // The symbol already exists

			_pSymbol->MRTC_AddRef();
			m_Children.f_Insert(_pSymbol);		
			_pSymbol->m_pParent = this;

			return true;
		}

		virtual void RemoveSymbol(CSymbol *_pSymbol)
		{
			if (_pSymbol->m_SymbolLink.IsInTree())
			{
				m_Children.f_Remove(_pSymbol);
				_pSymbol->MRTC_RemoveRef();
				_pSymbol->m_pParent = NULL;
			}
		}

		virtual TList_Vector<CStr> GetSymbols(int _Type = ESymbol_All)
		{
			TList_Vector<CStr> RetList;

			CTreeIter Iter = m_Children;

			while (Iter)
			{
				if (Iter->m_ClassType & _Type)
				{
					RetList.Add(Iter->m_Name);
				}
				++Iter;
			}

			return RetList;
		}

		virtual CStr GetDesc()
		{
			return "Namespace:\t" + m_Name;
		}
	};

	typedef TPtr<CSymbol_NameSpace> spCSymbol_NameSpace;

	class CFunctionCaller
	{
	public:
		int m_nArguments;
		const char **m_pArgTypes;
		const char *m_pReturnType;

		virtual void Call(void *_pArgs, void *_pReturn) pure;
	};

	class CSymbol_Class : public CSymbol_NameSpace
	{
	public:
		CSymbol_Class(CSymbol *_pParent, const char *_pName) : CSymbol_NameSpace(_pParent, _pName){}
		int m_DataSize;

		virtual void Construct(void *_pMemory) pure;
		virtual void Destruct(void *_pMemory) pure;
		virtual void Copy(void *_pDest, const void *_pSource) pure;

		virtual CStr GetDesc()
		{
			return "Class:\t\t" + m_Name;
		}

	};


	typedef TPtr<CSymbol_Class> spCSymbol_Class;

	class CSymbol_Function : public CSymbol_NameSpace
	{
	public:
		CSymbol_Function(CSymbol *_pParent, const char *_pName) : CSymbol_NameSpace(_pParent, _pName)
		{
			m_nStackVariables = 0;
		}

		virtual CStr GetDesc()
		{
			CStr Str = CStrF("%s %s(", m_spReturnType->m_Name.Str(), m_Name.Str());

			for (int i = 0; i < m_splArgumentTypes.Len(); ++i)
			{
				if (i)
					Str += ", ";
				Str += m_splArgumentTypes[i]->m_Name;
			}

			Str += ")";

			return "Function:\t" + Str;
		}

		TList_Vector<spCSymbol_Class> m_splArgumentTypes;
		spCSymbol_Class m_spReturnType;
		int m_nStackVariables;

	};

	typedef TPtr<CSymbol_Function> spCSymbol_Function;

	class CSymbol_Variable : public CSymbol
	{
	public:
		CSymbol_Variable(CSymbol *_pParent, const char *_pName) : CSymbol(_pParent, _pName)
		{
		}


		spCSymbol_Class m_spClass; // Every variable has a type

		virtual void Evaluate(CExecutionContext &_Context) pure;
		virtual void EvaluateSet(CExecutionContext &_Context) pure;

		virtual CStr GetDesc()
		{
			CStr Str = CStrF("%s %s", m_spClass->m_Name.Str(), m_Name.Str());
			return "Variable\t" + Str;
		}

	};

	class CSymbol_Constant : public CSymbol
	{
	public:

		CSymbol_Constant(CSymbol *_pParent, const char *_pName) : CSymbol(_pParent, _pName)
		{
			m_ClassType |= ESymbol_Constant;
		}

		spCSymbol_Class m_spClass; // Every constant has a type

		virtual void Evaluate(CExecutionContext &_Context) pure;

		virtual CStr GetDesc()
		{
			CStr Str = CStrF("%s %s", m_spClass->m_Name.Str(), m_Name.Str());
			return "Constant:\t" + Str;
		}

	};

	typedef TPtr<CSymbol_Variable> spCSymbol_Variable;

	enum EOperatorType
	{
		EOperatorType_UnaryPrefix,
		EOperatorType_VariablePrefix,
		EOperatorType_VariablePostfix,
		EOperatorType_Binary,
		EOperatorType_Assignment,
	};

	class CSymbol_Operator : public CSymbol_NameSpace
	{
	public:
		CSymbol_Operator(CSymbol *_pParent, const char *_pName) : CSymbol_NameSpace(_pParent, _pName)
		{
			m_ClassType |= ESymbol_Operator;
		}

		virtual CStr GetDesc()
		{
			CStr Str = CStrF("%s", m_Name.Str());
			return "CSymbol_Operator:\t" + Str;
		}
	};

	typedef TPtr<CSymbol_Variable> spCSymbol_Variable;
}

#include "MScript_RegisterContext.h"

namespace NScript
{
	class CExecutionContext;

	class CFunctionContext
	{
	public:
		int m_iFunctionStackPosition;
		int m_iUserFunctionStackPosition;
	};

	class CScopeDestruct
	{
		int m_iDestructListStart;
        int m_nDestructors;
	};

	class CExecutionContext : public CReferenceCount
	{
	public:

		CContext *m_pParser;
		spCSymbol m_spRootSymbol;

		TList_Vector<uint8> m_Stack;

		int m_iCurrentStackPosition;
		int m_iFunctionStackPosition;
		int m_iUserFunctionStackPosition;

		CScopeDestruct m_ScopeDestruct;

		int m_iBreakCount;

		void Push(const void *_pData, mint _Size)
		{
#if DScriptDebug >= 2
			M_TRACEALWAYS("Push %d = %d\n", _Size, m_iCurrentStackPosition);
#endif
			void *pStack = Alloc(_Size);
			memcpy(pStack, _pData, _Size);
		}

		void Pop(void *_pData, mint _Size)
		{			
			void *pStack = GetStackPtr(GetStackPosition() - _Size);
			memcpy(_pData, pStack, _Size);
			Free(_Size);
#if DScriptDebug >= 2
			M_TRACEALWAYS("Pop %d = %d\n", _Size, m_iCurrentStackPosition);
#endif
		}


		template <typename t_CType>
		void Push(const t_CType &_Context)
		{
			Push(&_Context, sizeof(_Context));
		}

		template <typename t_CType>
		void Pop(t_CType &_Context)
		{			
			Pop(&_Context, sizeof(_Context));
		}

		void PushScopeDestruct()
		{
			Push(m_ScopeDestruct);
		}
		void PopScopeDestruct()
		{
			Pop(m_ScopeDestruct);
		}

		void PushFunctionContext()
		{
			CFunctionContext Context;
			Context.m_iFunctionStackPosition = m_iFunctionStackPosition;
			Context.m_iUserFunctionStackPosition = m_iUserFunctionStackPosition;
			Push(Context);
		}

		void PopFunctionContext()
		{
			CFunctionContext Context;
			Pop(Context);
			m_iFunctionStackPosition = Context.m_iFunctionStackPosition;
			m_iUserFunctionStackPosition = Context.m_iUserFunctionStackPosition;
		}

		static void PopFuncPos(CExecutionContext &_ExecutionContext)
		{
			_ExecutionContext.PopFunctionContext();
		}

		void PushFunctionContextPoper()
		{
			PushTask(PopFuncPos, "PopFuncPos");
		}

		class CExecutionTask : public CEvaluator
		{
		public:
			void (*m_pTask)(CExecutionContext &_ExecutionContext);
			const char *m_pDesc;

			void Evaluate(CExecutionContext &_ExecutionContext)
			{
				_ExecutionContext.Free(sizeof(CExecutionTask));
				m_pTask(_ExecutionContext);
			}
			CStr GetDesc()
			{
				return m_pDesc;
			}

		};

		void PushTask( void (*_pTask)(CExecutionContext &_ExecutionContext) , const char *_pDesc)
		{
			CExecutionTask Task;
			Task.m_pTask = _pTask;
			Task.m_pDesc = _pDesc;
			void *pEval = GetStackPtr();
			Push(Task);
			PushEvaluator((CExecutionTask *)pEval);
		}

		void PushEvaluator(CEvaluator *_pEval)
		{
#if DScriptDebug >= 2
			if (_pEval)
				M_TRACEALWAYS("@@@ Push(x) = %s ", _pEval->GetDesc().Str());
#endif
			Push(_pEval);
		}

		void PopEvaluator(CEvaluator *&_pEval)
		{
			Pop(_pEval);
		}

		int GetFunctionPosition()
		{
			return m_iFunctionStackPosition;
		}

		void SetFunctionPosition(int _Position)
		{
			m_iFunctionStackPosition = _Position;
		}

		int GetUserFunctionPosition()
		{
			return m_iUserFunctionStackPosition;
		}

		void SetUserFunctionPosition(int _Position)
		{
			m_iUserFunctionStackPosition = _Position;
		}

		int GetStackPosition()
		{
			return m_iCurrentStackPosition;
		}

		mint m_UserData;

		void *Alloc(int _Size)
		{
			if (m_iCurrentStackPosition + _Size> m_Stack.Len())
			{
				M_ASSERT(0, "Stack overflow");
				return NULL;
			}

			m_iCurrentStackPosition += _Size;
			return m_Stack.GetBasePtr() + (m_iCurrentStackPosition - _Size);	
		}

		void Free(int _Size)
		{
			m_iCurrentStackPosition -= _Size;

			
			if (m_iCurrentStackPosition < 0)
			{
				// Throw exception
				M_ASSERT(0, "Stack underflow");
			}
		}

		void *GetStackPtr()
		{
			return m_Stack.GetBasePtr() + m_iCurrentStackPosition;		
		}

		void *GetStackPtr(int _iPos)
		{
			return m_Stack.GetBasePtr() + _iPos;
		}

		void *GetFunctionStackPtr()
		{
			return m_Stack.GetBasePtr() + m_iFunctionStackPosition;		
		}

		void *GetUserFunctionStackPtr()
		{
			return m_Stack.GetBasePtr() + m_iUserFunctionStackPosition;		
		}

		CExecutionContext();
		~CExecutionContext();
		bool Execute(const char* _pFuncName, void * _pArgs = NULL, int _nArg = 0, void* _pReturnValue = NULL);
		bool Execute();	// Executes main()
		bool Refresh();	// Continue paused execution
	};

	typedef TPtr<CExecutionContext> spCExecutionContext;

	class CContext;
	class SYSTEMDLLEXPORT CScriptInstance : public CReferenceCount
	{
		friend class CContext;
		TPtr<CSymbol_NameSpace> m_spRootNameSpace;
	public:

		CContext* m_pParser;

		CScriptInstance(CContext* _pParser);
		~CScriptInstance();

		virtual spCExecutionContext CreateExecutionContext(int _StackSize = 1024*2);

	};

	typedef TPtr<CScriptInstance> spCScriptInstance;


	class CStatement : public CEvaluator, public CReferenceCount
	{
	public:
		virtual ~CStatement()
		{
		}

		class CSymbol_UserFunction *m_pFunction;
		CStatement *m_pParentStatement;
		TList_Vector< TPtr<CStatement> > m_splStatements;

		int GetStatementPlace(CStatement *_pStatement)
		{
			for (int i = 0; i < m_splStatements.Len(); ++i)
			{
				if (m_splStatements[i] == _pStatement)
					return i;
			}

			return -1;
		}

		virtual class CStatement_Scope *GetScope(int &_iPlaceInScope);

		virtual int PostParse(int _iParent, CStr &_ErrorReturn)
		{
			for (int i = 0;i < m_splStatements.Len(); ++i)
			{
				int Ret = m_splStatements[i]->PostParse(i, _ErrorReturn);
				if (!Ret)
					return false;
				if (Ret >= 2 && i >= Ret-2)
					i -= Ret-1;
			}

			return true;
		}

		virtual CSymbol_Class *GetReturnType()
		{
			return NULL;
		}

		virtual CSymbol *GetSymbol()
		{
			return NULL;
		}

		virtual CStr GetDesc()
		{
			return "CStatement";
		}

		virtual void TraceInfo(int _Depth)
		{
			CStr Str;
			Str += "          ";
			for (int i= 0; i < _Depth + 1; ++i)
			{
				Str += '\t';
			}
			M_TRACEALWAYS("%s%s\n", Str.Str(), GetDesc().Str());
			for (int i = 0;i < m_splStatements.Len(); ++i)
			{
				m_splStatements[i]->TraceInfo(_Depth + 1);
			}
		}
	};

	typedef TPtr<CStatement> spCStatement;

	class CStatement_Receiver : public CStatement
	{
	public:

		spCSymbol_Class m_spType;

		static void Epilogue(CExecutionContext &_ExecutionContext)
		{
 			void **pReturnStack = (void **)_ExecutionContext.GetFunctionStackPtr();
			CSymbol_Class *pType;
			_ExecutionContext.Pop(pType);			
			pType->Destruct(*pReturnStack);
			_ExecutionContext.PopFunctionContext();
			_ExecutionContext.Free(pType->m_DataSize + sizeof(void *));
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
			_ExecutionContext.PushTask(Epilogue, "ReceiverEpilogue");

			_ExecutionContext.PushEvaluator(m_splStatements[0]);
		}

		virtual int PostParse(int _iParent, CStr &_ErrorReturn)
		{	
			M_ASSERT(m_splStatements.Len() == 1, "Must be so you know");		
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
			return "CParseHandler_Receiver (" + m_spType->m_Name + ")";
		}
	};

	class CStatement_Scope : public CStatement
	{
	public:	

		class CSymbol_ScopeNameSpace : public CSymbol_NameSpace
		{
		public:

			CSymbol_ScopeNameSpace() : 
			CSymbol_NameSpace(NULL, "Root")
			{
				m_pParent = NULL;
			}

			CSymbol *m_pParent;

			virtual CSymbol *GetSymbol(const char *_pName, bool _bCheckParent)
			{			
				CSymbol *pSymbol = CSymbol_NameSpace::GetSymbol(_pName, _bCheckParent);
				if (pSymbol)
					return pSymbol;

				return m_pParent->GetSymbol(_pName, _bCheckParent);
			}
		};

		CStatement_Scope()
		{
		}

		virtual CStatement_Scope *GetScope(int &_iPlaceInScope)
		{
			return this;
		}

		CSymbol_ScopeNameSpace m_NameSpace;

		virtual void Evaluate(CExecutionContext &_ExecutionContext)
		{
			for (int i = m_splStatements.Len() -1;i >= 0; --i)
			{
				_ExecutionContext.PushEvaluator(m_splStatements[i]);
			}
		}
		virtual CStr GetDesc()
		{
			return "CStatement_Scope";
		}

		virtual int PostParse(int _iParent, CStr &_ErrorReturn)
		{
			spCStatement spTemp = m_splStatements[0];
			m_splStatements.Del(0);
			m_splStatements.Add(spTemp);

			return CStatement::PostParse(_iParent, _ErrorReturn);
		}

	};

	typedef TPtr<CStatement_Scope> spCScriptContextScope;


	class CStatement_Holder : public CStatement
	{
	public:
		void Evaluate(CExecutionContext &_ExecutionContext)
		{
			M_ASSERT(0, "No holders should be left");
		}

		virtual int PostParse(int _iParent, CStr &_ErrorReturn)
		{
			int Ret = CStatement::PostParse(_iParent, _ErrorReturn);
			if (!Ret)
				return false;
			else if (Ret >= 2)
				return Ret;

			if (m_splStatements.Len() == 1)
			{
				spCStatement spThis = this;
				spCStatement spTemp = m_splStatements[0];
				spTemp->m_pParentStatement = m_pParentStatement;
				m_pParentStatement->m_splStatements[_iParent] = spTemp;
				return 1;
			}
			else if (m_splStatements.Len() == 0)
			{
				spCStatement spThis = this;
				m_pParentStatement->m_splStatements.Del(_iParent);
				return 2;
			}
			else
			{
				spCStatement spThis = this;
				m_pParentStatement->m_splStatements.Del(_iParent);
				int iInsert = _iParent;
				for (int i = 0; i < m_splStatements.Len(); ++i)
				{
					m_splStatements[i]->m_pParentStatement = m_pParentStatement;
					m_pParentStatement->m_splStatements.Insert(iInsert++, m_splStatements[i]);
				}
				return 2;
			}

		}
		virtual CStr GetDesc()
		{
			return "CStatement_Holder";
		}

	};

	class CStatement_UserFunctionRoot : public CStatement_Scope
	{
	public:

		int PostParse(int _iParent, CStr &_ErrorReturn)
		{
			M_ASSERT(m_splStatements.Len() == 1, "Must be so");

			TList_Vector< TPtr<CStatement> > splStateMents = m_splStatements[0]->m_splStatements;

			m_splStatements = splStateMents;

			for (int i = 0; i < m_splStatements.Len(); ++i)
			{
				m_splStatements[i]->m_pParentStatement = this;
			}

			return CStatement_Scope::PostParse(_iParent, _ErrorReturn);
		}
	};

	class CStatement_Constant : public CStatement
	{
	public:

		TPtr<CSymbol_Constant> m_spSymbol;

		virtual void Evaluate(CExecutionContext &_ExecutionContext)
		{			
			m_spSymbol->Evaluate(_ExecutionContext);
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
//			M_ASSERT(_iParent == 0, "Must be so you know");
			//M_ASSERT(m_pParentStatement->m_splStatements.Len() == 2, "Must be so you know");		

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
			return "CStatement_Constant (" + m_spSymbol->m_spClass->m_Name + " " + m_spSymbol->m_Name + ")";
		}

	};

	class CSymbol_UserFunction : public CSymbol_Function
	{
	public:
		CSymbol_UserFunction(CSymbol *_pParent, const char *_pName) : CSymbol_Function(_pParent, _pName)
		{
			m_ClassType |= ESymbol_Function_Script;

			m_RootScope.m_pFunction = this;
		}

		~CSymbol_UserFunction()
		{
			m_RootScope.m_splStatements.Clear();
		}

		int AllocateStackVariable()
		{
			return m_nStackVariables++;
		}

		CStatement_UserFunctionRoot m_RootScope;

		void Evaluate(CExecutionContext &_Context)
		{			
			_Context.PushFunctionContext();
			_Context.SetUserFunctionPosition(_Context.GetFunctionPosition());
			_Context.PushFunctionContextPoper();
			_Context.PushEvaluator(&m_RootScope);
		}
/*
		void Call(CExecutionContext &_ExecutionContext, void *_pArgs, void *_pReturn)
		{
			int iFunction = _ExecutionContext.GetStackPosition();
			void **pRet = (void **)_ExecutionContext.Alloc(sizeof(void *) * m_nStackVariables);
			*pRet = _pReturn;
			memcpy(pRet + 1, _pArgs, m_splArgumentTypes.Len() * sizeof(void *));

			_ExecutionContext.PushFunctionContext();
			_ExecutionContext.SetFunctionPosition(iFunction);
			_ExecutionContext.SetUserFunctionPosition(iFunction);
			m_RootScope.Evaluate(_ExecutionContext);
			_ExecutionContext.PopFunctionContext();
			_ExecutionContext.Free(sizeof(void *) * m_nStackVariables);
		}
*/
		virtual void TraceInfo(int _Depth)
		{
			m_RootScope.TraceInfo(_Depth);
		}

	};

	class CParseHandler;

	class CParseContext
	{
	public:

		const char *m_pOriginalStr;
		const char *m_pStr;
		CSymbol *m_pParentSymbol;
		CParseHandler *m_pParserScope;
		CStatement_Scope *m_pFunctionScope;
		CStatement_Holder *m_pFunctionStatementHolder;

		int m_Error;
		int m_ErrorCharacter;
		CStr m_ErrorStr;
		int m_iParseDepth;
		int m_iParanthesis;

		CParseContext()
		{
			m_iParseDepth = 0;
			m_Error = 0;
			m_pOriginalStr = m_pStr = "";
			m_pParentSymbol = NULL;
			m_pParserScope = NULL;
			m_iParanthesis = 0;
		}

		void SetError(const char *_pError, int _ErrorCharacter)
		{
			m_ErrorStr = _pError;
			m_Error = 1;
			m_ErrorCharacter = _ErrorCharacter;
		}

		void SetError(const char *_pError, const char *_pErrorChar)
		{
			m_ErrorStr = _pError;
			m_Error = 1;
			m_ErrorCharacter = _pErrorChar - m_pOriginalStr;
		}

		void CopyError(const CParseContext &_Src)
		{
			m_Error = _Src.m_Error;
			m_ErrorStr = _Src.m_ErrorStr;
			m_ErrorCharacter = _Src.m_ErrorCharacter;
		}

		void SetParse(const char *_pParse)
		{
			m_pOriginalStr = m_pStr = _pParse;
		}

		CParseContext & operator = (const CParseContext &_Src)
		{
			m_pStr = _Src.m_pStr;
			m_pParentSymbol = _Src.m_pParentSymbol;
			m_pParserScope = _Src.m_pParserScope;
			m_Error = _Src.m_Error;
			m_ErrorStr = _Src.m_ErrorStr;
			m_pOriginalStr = _Src.m_pOriginalStr;
			m_ErrorCharacter = _Src.m_ErrorCharacter;
			m_iParseDepth = _Src.m_iParseDepth;
			m_iParanthesis = _Src.m_iParanthesis;
			m_pFunctionScope = _Src.m_pFunctionScope;
			m_pFunctionStatementHolder = _Src.m_pFunctionStatementHolder;
			return *this;
		}

		CParseContext(const CParseContext &_Src)
		{
			m_pStr = _Src.m_pStr;
			m_pParentSymbol = _Src.m_pParentSymbol;
			m_pParserScope = _Src.m_pParserScope;
			m_Error = _Src.m_Error;
			m_ErrorStr = _Src.m_ErrorStr;
			m_iParseDepth = _Src.m_iParseDepth;
			m_iParanthesis = _Src.m_iParanthesis;
			m_pFunctionScope = _Src.m_pFunctionScope;
			m_pFunctionStatementHolder = _Src.m_pFunctionStatementHolder;
			m_pOriginalStr = _Src.m_pOriginalStr;
			m_ErrorCharacter = _Src.m_ErrorCharacter;
		}
	};


	class CParseHandler : public CReferenceCount
	{
		MRTC_DECLARE;
	public:

		CContext *m_pParser;

		CStr m_Name;

		class CCompare
		{
		public:
			DIdsPInlineS static aint Compare(const CParseHandler *_pFirst, const CParseHandler *_pSecond, void *_pContext)
			{
				return strcmp(_pFirst->m_Name, _pSecond->m_Name);
			}

			DIdsPInlineS static aint Compare(const CParseHandler *_pTest, const char * _pKey, void *_pContext)
			{
				return strcmp(_pTest->m_Name, _pKey);
			}
		};

		DIdsTreeAVLAligned_Link(CParseHandler, m_NameLink, const char *, CCompare);

		TList_Vector < TPtr<CParseHandler> > m_splChildren;

		CParseHandler(CContext *_pParser, const char *_pName);

		virtual ~CParseHandler();

		void AddChild(TPtr<CParseHandler> _spChild)
		{
			M_ASSERT(_spChild, "");

			m_splChildren.Add(_spChild);
		}

		virtual void Clear()
		{
			for (int i = 0; i < m_splChildren.Len(); ++i)
			{
				if (m_splChildren[i] != this)
					m_splChildren[i]->Clear();
			}

			m_splChildren.Clear();
		}

		virtual int TryParse(CParseContext &_ParserContext) { return 0;} 
		virtual void Parse(CParseContext &_ParserContext);

	};

	typedef TPtr<CParseHandler> spCParseHandler;



	// The parsing class itself.
	class SYSTEMDLLEXPORT CContext : public CReferenceCount
	{
	public:
		MRTC_DECLARE;

		friend class CScriptInstance;
		friend class CRegisterContext;

	public:

		spCSymbol_NameSpace m_spRootNameSpace;

		spCParseHandler m_spRootParserScope;
		spCParseHandler m_spFillout;

		DIdsTreeAVLAligned_Tree(CParseHandler, m_NameLink, const char *, CParseHandler::CCompare) m_NamedScopes;
		spCParseHandler GetScope(const char *_pName);

		virtual spCExecutionContext CreateExecutionContext(int _StackSize = 1024*2);

		void AddUserLangParser(spCParseHandler _spParserScope)
		{
			GetScope("UserLang")->AddChild(_spParserScope);
		}

		void AddUserConstantParser(spCParseHandler _spParserScope)
		{
			GetScope("UserConstant")->AddChild(_spParserScope);
		}

		template <typename t_CType>
		t_CType *GetScope(const char *_pName)
		{
			CParseHandler *pScope = m_NamedScopes.FindEqual(_pName);
			if (pScope)
			{
				return TDynamicCast<t_CType>(pScope);
			}
			else
				return NULL;
		}

		void RegisterLanguage();

		bool AddSymbol(CSymbol *_pSymbol);
		void RemoveSymbol(CSymbol *_pSymbol);

		void AddOperator(CStr _Name, int _OperatorType, CSymbol_Function *_pFunction);


		// 
		CContext();
		virtual ~CContext();

		spCRegisterContext GetRegisterContext();

		spCScriptInstance BuildScript(const char *_pNameSpace, const char* _pStr, const char *_pSourceIdentifier, int _CharacterOffset = 0, int _LineOffset = 0, bool _ShowErrors = true);
		
		CSymbol_NameSpace *GetRootNameSpace();

		void TraceSymbolTree(bool _TraceFunctionStatements);
	};

	class SYSTEMDLLEXPORT CClient : public CReferenceCount 
	{
	public:
		class CRegistered : public CReferenceCount
		{
		public:
			spCRegisterContext m_spRegisterContext;
			CContext *m_pParser;
		};

		typedef TPtr<CRegistered> spCRegistered;

		TArray<spCRegistered> m_splAutoRegistered;

		CClient()
		{
		}

		virtual ~CClient() {}

		virtual void RegisterToParser(CContext *_pParser)
		{
			spCRegistered spReg = MNew(CRegistered);
			spReg->m_spRegisterContext = _pParser->GetRegisterContext();
			spReg->m_pParser = _pParser;

			M_TRY
			{
				Register(*spReg->m_spRegisterContext);
				m_splAutoRegistered.Add(spReg);
			}
			M_CATCH(
			catch (CCException)
			{
			}		
			)

		}

		virtual void UnRegister()
		{
			m_splAutoRegistered.Clear();
		}

		virtual void UnRegister(CContext *_pParser)
		{
			for (int i = 0; i < m_splAutoRegistered.Len(); ++i)
			{
				CContext *pParser = m_splAutoRegistered[i]->m_pParser;
				if (pParser == _pParser)
				{
					m_splAutoRegistered.Del(i);
					return;
				}
			}
		}

		virtual void Register(CRegisterContext & _RegContext) pure;
	};

	typedef TPtr<CClient> spCClient;
}

typedef NScript::CClient CScriptClient;
typedef NScript::spCClient spCScriptClient;
typedef NScript::CScriptInstance CScript;
typedef NScript::spCScriptInstance spCScript;
typedef NScript::CExecutionContext CScriptExecutionContext;
typedef NScript::spCExecutionContext spCScriptExecutionContext;
typedef NScript::CContext CScriptContext;


#if DScriptDebug>0
#pragma  optimize("",on)
#pragma  inline_depth(16)
#endif

#endif

