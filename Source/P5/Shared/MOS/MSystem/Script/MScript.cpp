 
#include "PCH.h"

#include "MScript.h"

#if DScriptDebug>0
#pragma  optimize("",off)
#pragma  inline_depth(0)
#endif

namespace NScript
{
	MRTC_IMPLEMENT(CParseHandler, CReferenceCount);

	CScriptInstance::CScriptInstance(CContext* _pParser)
	{
		m_pParser = _pParser;
	}

	CScriptInstance::~CScriptInstance()
	{
		if (m_spRootNameSpace)
			m_pParser->m_spRootNameSpace->RemoveSymbol(m_spRootNameSpace);
	}

	spCExecutionContext CScriptInstance::CreateExecutionContext(int _StackSize)
	{
		spCExecutionContext spExecutionContext = MNew(CExecutionContext);

		spExecutionContext->m_pParser = m_pParser;
		spExecutionContext->m_spRootSymbol = m_spRootNameSpace;
		spExecutionContext->m_Stack.SetLen(_StackSize);
		
		return spExecutionContext;
	}



	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



	void CClient::Register(CRegisterContext & _RegContext)
	{

	}


	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




	CSymbol::CSymbol(CSymbol *_pParent, const char *_pName)
	{
		m_Name = _pName;
		m_pParent = _pParent;
		if (m_pParent && (m_pParent->m_ClassType & ESymbol_NameSpace))	
			((CSymbol_NameSpace *)m_pParent)->AddSymbol(this);

		m_ClassType = 0;
	}

	CSymbol::~CSymbol()
	{
		if (m_pParent && (m_pParent->m_ClassType & ESymbol_NameSpace))
			((CSymbol_NameSpace *)m_pParent)->RemoveSymbol(this);
	}


	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CExecutionContext::CExecutionContext()
	{
		m_pParser = NULL;
		m_iCurrentStackPosition = 0;
		m_iFunctionStackPosition = 0;
		m_iUserFunctionStackPosition = 0;
	}

	CExecutionContext::~CExecutionContext()
	{
	}

	static void CleanupExecute(CExecutionContext &_Context)
	{
		int nArg;
		_Context.Pop(nArg);
		_Context.Free((nArg + 1) * sizeof(void *));
	}
	
	bool CExecutionContext::Execute(const char* _pFuncName, void * _pArgs, int _nArg, void* _pReturnValue)
	{
		TProfileDef(Timer);
		{
			TMeasureProfile(Timer)

			spCSymbol spSymbol = m_spRootSymbol->GetSymbol(_pFuncName, false);
			if (!spSymbol || !(spSymbol->m_ClassType & ESymbol_Function))
			{
				M_TRACEALWAYS("\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\nScript Execute Error: Function not found (%s)\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n", _pFuncName);
				ConOut(CStrF("Script Execute Error: Function not found (%s)", _pFuncName));
				return false;
			}

			CSymbol_Function *pSymbol = ((CSymbol_Function *)(CSymbol *)spSymbol);
			if (pSymbol->m_splArgumentTypes.Len() != _nArg)
			{
				M_TRACEALWAYS("\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\nScript Execute Error: Number of arguments missmatch (%s)\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n", _pFuncName);
				ConOut(CStrF("Script Execute Error: Number of arguments missmatch (%s)", _pFuncName));
				return false;
			}

			CEvaluator *pEval = 0;
			PushEvaluator(pEval);
			PushFunctionContext();
			PushFunctionContextPoper();
			SetFunctionPosition(GetStackPosition());
			Push(&_pReturnValue, sizeof(void *));
			Push(_pArgs, sizeof(void *) * _nArg);
			Push(_nArg);
			PushTask(CleanupExecute, "CleanupExecute");
			PushEvaluator(spSymbol);
		}

#ifdef M_Profile
//		M_TRACEALWAYS("%s\n", TString("Script Execute Setup Time", Timer).Str());
#endif
	//		((CSymbol_Function *)(CSymbol *)spSymbol)->Call(*this, _pArgs, _pReturnValue);
		
		return Refresh();
	}

	bool CExecutionContext::Execute()
	{
		return Execute("main");
	}

	bool CExecutionContext::Refresh()
	{
		TProfileDef(Timer);
		{
			TMeasureProfile(Timer)
			CEvaluator *pEval;
			PopEvaluator(pEval);
			int iCurrent = 0;
			while (pEval)
			{
	#if DScriptDebug >= 2
				M_TRACEALWAYS("#### Eval(%d) = %d ", iCurrent, m_iCurrentStackPosition);
				M_TRACEALWAYS("%s\n", pEval->GetDesc().Str());
	#endif
				pEval->Evaluate(*this);
				PopEvaluator(pEval);
				++iCurrent;
			}

			if (m_iCurrentStackPosition != 0
				|| m_iFunctionStackPosition != 0
				|| m_iUserFunctionStackPosition != 0)
			{
				M_TRACEALWAYS("\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\nScript Execute Error: Stack error\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n");
				ConOut(CStr("Script Execute Error: Stack error"));
			}
		}
#ifdef M_Profile
//		M_TRACEALWAYS("%s\n", TString("Script Execute Time", Timer).Str());
#endif
		
		return true;
	}

	CContext::CContext()
	{
		m_spRootNameSpace = MNew2(CSymbol_NameSpace, (CSymbol *)0, "Root");
		RegisterLanguage();
	}

	CContext::~CContext()
	{
		m_spRootParserScope->Clear();
		m_NamedScopes.DeRefAll();
	}

	void TraceSymbolTree_r(int _Depth, CSymbol_NameSpace *_pNameSpace, bool _TraceFunctionStatements)
	{
		DIdsTreeAVLAligned_Iterator(CSymbol, m_SymbolLink, const char *, CSymbol::CCompare) Iter = _pNameSpace->m_Children;

		while (Iter)
		{
			CStr Str;
			Str += CStrF("0x%08x", Iter->m_ClassType);
			for (int i= 0; i < _Depth + 1; ++i)
			{
				Str += '\t';
			}
			M_TRACEALWAYS("%s%s\n", Str.Str(), Iter->GetDesc().Str());
			if (_TraceFunctionStatements)
			{
				Iter->TraceInfo(_Depth);
			}
			if (Iter->CastNameSpace())
				TraceSymbolTree_r(_Depth + 1, Iter->CastNameSpace(), _TraceFunctionStatements);

			++Iter;
		}

	}

	void CContext::TraceSymbolTree(bool _TraceFunctionStatements)
	{
		TraceSymbolTree_r(0, m_spRootNameSpace, _TraceFunctionStatements);
	    
	}

	int FindLineFromCharacter(const char *_pStr, int &_Character)
	{
		int Line = 0;

		const char *pStr = _pStr;

		int iChar = 0;
		int iCharTemp = 0;

		while (*pStr && iChar < _Character)
		{
			if (*pStr == '\n')
			{
				++Line;
				iCharTemp = iChar;
			}

			++pStr;
			++iChar;
		}

		_Character = iChar - iCharTemp;

		return Line;
	}

#define ErrorColor "099"
#define ErrorColorScript "0ff"
	void TraceScriptError(const char *_pStr, int _Line, int _Character, const char *_pError)
	{
		int Line = 0;
		const char *pStrStart = _pStr;
		const char *pStr = _pStr;
		while (1)
		{
			if (*pStr == '\r' || *pStr == '\n' || *pStr == 0)
			{
				CStr TempTrace;
				if (pStr - pStrStart)
				{
					TempTrace.Capture(pStrStart, pStr - pStrStart);
					M_TRACEALWAYS("%s\n", TempTrace.Str());
					LogFile(CStrF("%s", TempTrace.Str()));
					ConOut("§c"ErrorColorScript + TempTrace);
					if (Line == _Line)
					{
						CStr Temp;
						char *pBuffer = Temp.GetBuffer(_Character + 2);
						memset(pBuffer, ' ', _Character + 1);
						pBuffer[_Character] = '^';
						pBuffer[_Character + 1] = 0;
						M_TRACEALWAYS("%s - %s\n", pBuffer, _pError);
						LogFile(CStrF("%s - %s", pBuffer, _pError));
						Temp.Capture(TempTrace.Str(), _Character);
						ConOut("§A0" + Temp + CStrF("§AF^ - §cf88%s", _pError));
					}
					++Line;
				}
				else
				{
					++Line;
					M_TRACEALWAYS("\n");
				}

				if (*pStr == 0)
					break;

				while (*pStr == '\r' || *pStr == '\n')
					++pStr;
				pStrStart = pStr;
			}
			else
				++pStr;
		}
	}

	spCScriptInstance CContext::BuildScript(const char *_pNameSpace, const char* _pStr, const char *_pSourceIdentifier, int _CharacterOffset, int _LineOffset, bool _bShowErrors)
	{
		TProfileDef(Timer);
		spCScriptInstance spScript;
		{
			TMeasureProfile(Timer)
			spScript = MNew1(CScriptInstance,this);

			{	
				spCSymbol spNameSpace = m_spRootNameSpace->GetSymbol(_pNameSpace, false);

				if (spNameSpace)
				{
					if (_bShowErrors)
					{
						M_TRACEALWAYS("\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\nParse error: %s\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n", "Parser namespace alrealy exists");
						ConOut(CStrF("Parse error: %s", "Parser namespace already exists"));
					}
					return NULL;
				}
			}

			CParseContext Context;

			spScript->m_spRootNameSpace = MNew2(CSymbol_NameSpace, m_spRootNameSpace, _pNameSpace);	

			Context.SetParse(_pStr);
			Context.m_pParentSymbol = spScript->m_spRootNameSpace;
			Context.m_pParserScope = m_spRootParserScope;

			m_spRootParserScope->Parse(Context);

			if (Context.m_Error)
			{
				if (_bShowErrors)
				{
					int Character = Context.m_ErrorCharacter;
					int Line = -1;
					if (Character >= 0)
					{
						Line = FindLineFromCharacter(_pStr, Character);
					}
	//				ConOut(CStrF("§cf55Parse error: %s", Context.m_ErrorStr.Str()));
					ConOut(CStrF("§c"ErrorColor"Error at: %s(%d)", _pSourceIdentifier, Line + 1));

					M_TRACEALWAYS("\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX Parse error XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
					if (Line >= 0)
					{
						if (strchr(_pSourceIdentifier, '\\'))
						{
							ConOut(CStrF("§cf88Parse error: %s", Context.m_ErrorStr.Str()));
							LogFile(CStrF("%s(%d) : %s", _pSourceIdentifier, Line + 1 + _LineOffset, Context.m_ErrorStr.Str()));
							M_TRACEALWAYS("%s(%d) : %s\n", _pSourceIdentifier, Line + 1 + _LineOffset, Context.m_ErrorStr.Str());
						}
						else
						{
							M_TRACEALWAYS("Script Source: %s\nScript Text:\n", _pSourceIdentifier);
							LogFile(CStrF("Script Source: %s\nScript Text:", _pSourceIdentifier));
							TraceScriptError(_pStr, Line, Character, Context.m_ErrorStr);
						}
					}
					else
					{
						M_TRACEALWAYS("%s(-1) : %s\n", _pSourceIdentifier, Context.m_ErrorStr.Str());
						LogFile(CStrF("%s(-1) : %s", _pSourceIdentifier, Context.m_ErrorStr.Str()));
						ConOut(CStrF("§cf88Parse error: %s", Context.m_ErrorStr.Str()));
					}
					M_TRACEALWAYS(  "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n", Context.m_ErrorStr.Str());
				}
				return NULL;
			}
		}

#ifdef M_Profile
//		M_TRACEALWAYS("%s\n", TString("Script Build Time", Timer).Str());
#endif

		return spScript;
	}

	CSymbol_NameSpace *CContext::GetRootNameSpace()
	{
		return m_spRootNameSpace;
	}

	spCRegisterContext CContext::GetRegisterContext()
	{
		spCRegisterContext spRegContext = MNew(CRegisterContext);		
		spRegContext->Create(m_spRootNameSpace, this);

		return spRegContext;
	}

	spCExecutionContext CContext::CreateExecutionContext(int _StackSize)
	{
		spCExecutionContext spExecutionContext = MNew(CExecutionContext);

		spExecutionContext->m_pParser = this;
		spExecutionContext->m_spRootSymbol = m_spRootNameSpace;
		spExecutionContext->m_Stack.SetLen(_StackSize);
		
		return spExecutionContext;
	}

	spCParseHandler CContext::GetScope(const char *_pName)
	{
		return m_NamedScopes.FindEqual(_pName);
	}

	CParseHandler::CParseHandler(CContext *_pParser, const char *_pName)
	{
		m_pParser = _pParser;
		m_Name = _pName;
		MRTC_AddRef();
		m_pParser->m_NamedScopes.f_Insert(this);
	}

	CParseHandler::~CParseHandler()
	{
		if (m_NameLink.IsInTree())
		{
			m_pParser->m_NamedScopes.f_Remove(this);
		}

		Clear();
	}

	void CParseHandler::Parse(CParseContext &_ParseContext)
	{
		CParseContext Context = _ParseContext;
		while (*Context.m_pStr)
		{
			int i;
			CParseHandler *pParserScopeCurrent = Context.m_pParserScope;
			
			const char *pStartParse = Context.m_pStr;
			ParseWhiteSpace(pStartParse);
			int iRet = 0;
			for (i = 0; i < pParserScopeCurrent->m_splChildren.Len(); ++i)
			{
				iRet = pParserScopeCurrent->m_splChildren[i]->TryParse(Context);
				if (iRet)
					break;
			}

			if (i == pParserScopeCurrent->m_splChildren.Len())
			{
				const ch8 *pIdent = pStartParse;
				ParseIdentifier(pIdent);
				if (pIdent == pStartParse && *pIdent)
				{
					++pIdent;
				}
				CStr Ident;
				Ident.Capture(pStartParse, pIdent - pStartParse);
				Context.SetError(CStrF("Unrecognized statement: %s", Ident.Str()), pStartParse);
			}

			if (Context.m_Error)
			{
				_ParseContext.CopyError(Context);
				break;			
			}
			if (iRet == 2)
				break;
		}

		_ParseContext.m_pStr = Context.m_pStr;
		_ParseContext.m_iParseDepth = Context.m_iParseDepth;
	}

	CStatement_Scope *CStatement::GetScope(int &_iPlaceInScope)
	{
		if (m_pParentStatement)
		{
			CStatement_Scope *pRet = m_pParentStatement->GetScope(_iPlaceInScope);
			if (_iPlaceInScope == -1 && pRet)
			{
				_iPlaceInScope = pRet->GetStatementPlace(this);
			}

			return pRet;
		}

		return NULL;
	}
	
}

MRTC_IMPLEMENT_DYNAMIC(CScriptContext, CReferenceCount);

