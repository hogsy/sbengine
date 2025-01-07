#include "PCH.h"
#include "MScript.h"

#if DScriptDebug>0
#pragma  optimize("",off)
#pragma  inline_depth(0)
#endif

namespace NScript
{
	class CSymbol_ConstantStr : public CSymbol_Constant
	{
	public:
		CSymbol_ConstantStr(CSymbol *_pParent, const char *_pName) : CSymbol_Constant(_pParent, _pName)
		{
		}
		CStr m_Data;
		const char *m_pData;

		void Evaluate(CExecutionContext &_Context)
		{
			void *pDest = *((void **)_Context.GetFunctionStackPtr());
			*((const char **)pDest) = m_pData;
		}

	};

	class CParseHandler_ConstantStr : public CParseHandler
	{
		MRTC_DECLARE;
	public:
		CParseHandler_ConstantStr(CContext *_pParser, const char *_pName) : CParseHandler(_pParser, _pName)
		{
		}	

		virtual int TryParse(CParseContext &_ParseContext)
		{
			const char *pStr = _ParseContext.m_pStr;
			ParseWhiteSpace(pStr);

			char EndChar = '"';
			if ((*pStr) == '"')
				;
			else if ((*pStr) == '\'')
			{
				EndChar = '\'';
			}
			else
				return 0;

			const char *pStrStart = pStr;
			CStr Str;
			++pStr;
			while (*pStr)
			{
				if (*pStr == '\\' && *(pStr + 1) == EndChar)
				{
					Str += EndChar;
					pStr += 2;
				}
				if (*pStr == EndChar)
				{
					++pStr;
					break;
				}

				Str += *pStr;
				++pStr;
			}

			CSymbol *pSymbol = _ParseContext.m_pParentSymbol->GetSymbol("str", true);

			if (!pSymbol || !(pSymbol->m_ClassType & ESymbol_Class))
			{
				_ParseContext.SetError("str symbol needs to be a class (and especially it must point to a const char * class)", pStrStart);
				return -1;
			}

			TPtr<CSymbol_ConstantStr> spConstant = MNew2(CSymbol_ConstantStr, (CSymbol *)NULL, "Constant");
			spConstant->m_Data = Str;
			spConstant->m_pData = spConstant->m_Data.Str();
			spConstant->m_spClass = (CSymbol_Class *)pSymbol;

			TPtr<CStatement_Constant> spVariableStatement = MNew(CStatement_Constant);
			spVariableStatement->m_pParentStatement = _ParseContext.m_pFunctionStatementHolder;
			_ParseContext.m_pFunctionStatementHolder->m_splStatements.Add((CStatement *)spVariableStatement);
			spVariableStatement->m_spSymbol = spConstant;	

			TPtr<CStatement_Receiver> spReceiver = MNew(CStatement_Receiver);
			spReceiver->m_pParentStatement = _ParseContext.m_pFunctionStatementHolder;
			_ParseContext.m_pFunctionStatementHolder->m_splStatements.Add((CStatement *)spReceiver);
			spReceiver->m_spType = ((CSymbol_Class *)pSymbol);

			_ParseContext.m_pStr = pStr;

			return 1;
		}
	};
	MRTC_IMPLEMENT(CParseHandler_ConstantStr, CParseHandler);

	template <typename t_CInt>
	class CSymbol_ConstantInt : public CSymbol_Constant
	{
	public:
		CSymbol_ConstantInt(CSymbol *_pParent, const char *_pName) : CSymbol_Constant(_pParent, _pName)
		{
		}
		t_CInt m_Data;

		void Evaluate(CExecutionContext &_Context)
		{
			void *pDest = *((void **)_Context.GetFunctionStackPtr());
			*((t_CInt *)pDest) = m_Data;
		}
	};

	class CParseHandler_ConstantInt : public CParseHandler
	{
		MRTC_DECLARE;
	public:
		CParseHandler_ConstantInt(CContext *_pParser, const char *_pName) : CParseHandler(_pParser, _pName)
		{
		}	

		virtual int TryParse(CParseContext &_ParseContext)
		{
			const char *pStr = _ParseContext.m_pStr;
			ParseWhiteSpace(pStr);

			if (!NStr::CharIsNumber(*pStr))
				return 0;

			const char *pNumberStart = pStr;
			const char *pStrStart = pStr;
			if (pStr[0] == '0' && (pStr[1] == 'x' || pStr[1] == 'b' || pStr[1] == 'o'))
				pStr += 2;

			while (*pStr && NStr::CharIsNumber(*pStr))
				++pStr;

			CStr Number;
			Number.Capture(pNumberStart, pStr - pNumberStart);

			pNumberStart = pStr;
			while (*pStr && (NStr::CharIsNumber(*pStr) || NStr::CharIsAnsiAlphabetical(*pStr)))
				++pStr;

			CStr Symbol;
			Symbol.Capture(pNumberStart, pStr - pNumberStart);

			const char *pSymbolName = "int32";
			if (Symbol == "i8")
				pSymbolName = "int8";
			else if (Symbol == "i16")
				pSymbolName = "int16";
			else if (Symbol == "i32")
				pSymbolName = "int32";
			else if (Symbol == "i64" || Symbol == "l")
				pSymbolName = "int64";
			else if (Symbol == "ui8")
				pSymbolName = "uint8";
			else if (Symbol == "ui16")
				pSymbolName = "uint16";
			else if (Symbol == "ui32")
				pSymbolName = "uint32";
			else if (Symbol == "ui64" || Symbol == "ul")
				pSymbolName = "uint64";
			else if (Symbol != "")
			{
				_ParseContext.SetError(CStrF("Unrecognized integer point suffix (%s) while parsing an integer constanst", Symbol.Str()), pStrStart);
				return -1;
			}

			CSymbol *pSymbol = _ParseContext.m_pParentSymbol->GetSymbol(pSymbolName, true);

			if (!pSymbol || !(pSymbol->m_ClassType & ESymbol_Class))
			{
				_ParseContext.SetError(CStrF("Could not find match for symbol (%s) while parsing an interger constanst", pSymbolName), pStrStart);
				return -1;
			}

			TPtr<CSymbol_Constant> spConstant;
			
			if (strcmp(pSymbolName, "int8") == 0)
			{
				TPtr<CSymbol_ConstantInt<int8> > spC = MNew2(CSymbol_ConstantInt<int8>, (CSymbol *)NULL, "Constant");
				spC->m_Data = NStr::StrToInt(Number.Str(), (int8)0);
				spConstant = spC;
			}
			else if (strcmp(pSymbolName, "int16") == 0)
			{
				TPtr<CSymbol_ConstantInt<int16> > spC = MNew2(CSymbol_ConstantInt<int16>, (CSymbol *)NULL, "Constant");
				spC->m_Data = NStr::StrToInt(Number.Str(), (int16)0);
				spConstant = spC;
			}
			else if (strcmp(pSymbolName, "int32") == 0)
			{
				TPtr<CSymbol_ConstantInt<int32> > spC = MNew2(CSymbol_ConstantInt<int32>, (CSymbol *)NULL, "Constant");
				spC->m_Data = NStr::StrToInt(Number.Str(), (int32)0);
				spConstant = spC;
			}
			else if (strcmp(pSymbolName, "int64") == 0)
			{
				TPtr<CSymbol_ConstantInt<int64> > spC = MNew2(CSymbol_ConstantInt<int64>, (CSymbol *)NULL, "Constant");
				spC->m_Data = NStr::StrToInt(Number.Str(), (int64)0);
				spConstant = spC;
			}
			else if (strcmp(pSymbolName, "uint8") == 0)
			{
				TPtr<CSymbol_ConstantInt<uint8> > spC = MNew2(CSymbol_ConstantInt<uint8>, (CSymbol *)NULL, "Constant");
				spC->m_Data = NStr::StrToInt(Number.Str(), (uint8)0);
				spConstant = spC;
			}
			else if (strcmp(pSymbolName, "uint16") == 0)
			{
				TPtr<CSymbol_ConstantInt<uint16> > spC = MNew2(CSymbol_ConstantInt<uint16>, (CSymbol *)NULL, "Constant");
				spC->m_Data = NStr::StrToInt(Number.Str(), (uint16)0);
				spConstant = spC;
			}
			else if (strcmp(pSymbolName, "uint32") == 0)
			{
				TPtr<CSymbol_ConstantInt<uint32> > spC = MNew2(CSymbol_ConstantInt<uint32>, (CSymbol *)NULL, "Constant");
				spC->m_Data = NStr::StrToInt(Number.Str(), (uint32)0);
				spConstant = spC;
			}
			else if (strcmp(pSymbolName, "uint64") == 0)
			{
				TPtr<CSymbol_ConstantInt<uint64> > spC = MNew2(CSymbol_ConstantInt<uint64>, (CSymbol *)NULL, "Constant");
				spC->m_Data = NStr::StrToInt(Number.Str(), (uint64)0);
				spConstant = spC;
			}
			else
			{
				TPtr<CSymbol_ConstantInt<int32> > spC = MNew2(CSymbol_ConstantInt<int32>, (CSymbol *)NULL, "Constant");
				spC->m_Data = NStr::StrToInt(Number.Str(), (int32)0);
				spConstant = spC;
			}

			spConstant->m_spClass = (CSymbol_Class *)pSymbol;

			TPtr<CStatement_Constant> spVariableStatement = MNew(CStatement_Constant);
			spVariableStatement->m_pParentStatement = _ParseContext.m_pFunctionStatementHolder;
			_ParseContext.m_pFunctionStatementHolder->m_splStatements.Add((CStatement *)spVariableStatement);
			spVariableStatement->m_spSymbol = spConstant;	

			TPtr<CStatement_Receiver> spReceiver = MNew(CStatement_Receiver);
			spReceiver->m_pParentStatement = _ParseContext.m_pFunctionStatementHolder;
			_ParseContext.m_pFunctionStatementHolder->m_splStatements.Add((CStatement *)spReceiver);
			spReceiver->m_spType = ((CSymbol_Class *)pSymbol);

			_ParseContext.m_pStr = pStr;

			return 1;
		}
	};
	MRTC_IMPLEMENT(CParseHandler_ConstantInt, CParseHandler);

	template <typename t_CFloat>
	class CSymbol_ConstantFloat : public CSymbol_Constant
	{
	public:
		CSymbol_ConstantFloat(CSymbol *_pParent, const char *_pName) : CSymbol_Constant(_pParent, _pName)
		{
		}
		t_CFloat m_Data;

		void Evaluate(CExecutionContext &_Context)
		{
			void *pDest = *((void **)_Context.GetFunctionStackPtr());
			*((t_CFloat *)pDest) = m_Data;
		}
	};

	class CParseHandler_ConstantFloat : public CParseHandler
	{
		MRTC_DECLARE;
	public:
		CParseHandler_ConstantFloat(CContext *_pParser, const char *_pName) : CParseHandler(_pParser, _pName)
		{
		}	

		virtual int TryParse(CParseContext &_ParseContext)
		{
			const char *pStr = _ParseContext.m_pStr;
			ParseWhiteSpace(pStr);

			if (!NStr::CharIsNumber(*pStr))
				return 0;

			//4.0
			//0.0e10
			//0.0e-10
			//1e6

			const char *pNumberStart = pStr;
			const char *pStrStart = pStr;

			ParseNumeric(pStr);
			if (*pStr == 'e')
			{
				++pStr;
				if (!(NStr::CharIsNumber(*pStr) || *pStr == '-'))
					return 0;

				if (*pStr == '-')
					++pStr;

				ParseNumeric(pStr);
			}
			else if (*pStr == '.')
			{
				++pStr;
				ParseNumeric(pStr);

				if (*pStr == 'e')
				{
					++pStr;
					if (*pStr == '-')
						++pStr;
					ParseNumeric(pStr);
				}
			}
			else
				return 0;

			CStr Number;
			Number.Capture(pNumberStart, pStr - pNumberStart);

			pNumberStart = pStr;
			while (*pStr && (NStr::CharIsNumber(*pStr) || NStr::CharIsAnsiAlphabetical(*pStr)))
				++pStr;

			CStr Symbol;
			Symbol.Capture(pNumberStart, pStr - pNumberStart);

			const char *pSymbolName = "fp64";
			if (Symbol == "f")
				pSymbolName = "fp32";
			else if (Symbol == "fp32")
				pSymbolName = "fp32";
			else if (Symbol == "fp64")
				pSymbolName = "fp64";
			else if (Symbol != "")
			{
				_ParseContext.SetError(CStrF("Unrecognized floating point suffix (%s) while parsing a float constanst", Symbol.Str()), pStrStart);
				return -1;
			}


			CSymbol *pSymbol = _ParseContext.m_pParentSymbol->GetSymbol(pSymbolName, true);

			if (!pSymbol || !(pSymbol->m_ClassType & ESymbol_Class))
			{
				_ParseContext.SetError(CStrF("Could not find match for symbol (%s) while parsing a float constanst", pSymbolName), pStrStart);
				return -1;
			}

			TPtr<CSymbol_Constant> spConstant;
			
			if (strcmp(pSymbolName, "fp32") == 0)
			{
				TPtr<CSymbol_ConstantFloat<fp32> > spC = MNew2(CSymbol_ConstantFloat<fp32>, (CSymbol *)NULL, "Constant");
				spC->m_Data = NStr::StrToFloat(Number.Str(), (fp32)0.0f);
				spConstant = spC;
			}
			else // if (strcmp(pSymbolName, "fp64") == 0)
			{
				TPtr<CSymbol_ConstantFloat<fp64> > spC = MNew2(CSymbol_ConstantFloat<fp64>, (CSymbol *)NULL, "Constant");
				spC->m_Data = NStr::StrToFloat(Number.Str(), (fp64)0.0);
				spConstant = spC;
			}

			spConstant->m_spClass = (CSymbol_Class *)pSymbol;

			TPtr<CStatement_Constant> spVariableStatement = MNew(CStatement_Constant);
			spVariableStatement->m_pParentStatement = _ParseContext.m_pFunctionStatementHolder;
			_ParseContext.m_pFunctionStatementHolder->m_splStatements.Add((CStatement *)spVariableStatement);
			spVariableStatement->m_spSymbol = spConstant;	

			TPtr<CStatement_Receiver> spReceiver = MNew(CStatement_Receiver);
			spReceiver->m_pParentStatement = _ParseContext.m_pFunctionStatementHolder;
			_ParseContext.m_pFunctionStatementHolder->m_splStatements.Add((CStatement *)spReceiver);
			spReceiver->m_spType = ((CSymbol_Class *)pSymbol);

			_ParseContext.m_pStr = pStr;

			return 1;
		}
	};
	MRTC_IMPLEMENT(CParseHandler_ConstantFloat, CParseHandler);

	void g_RegisterConstantParsers(CRegisterContext & _RegContext)
	{
		_RegContext.m_pContext->AddUserConstantParser(MNew2(CParseHandler_ConstantStr, _RegContext.m_pContext, "String"));
		_RegContext.m_pContext->AddUserConstantParser(MNew2(CParseHandler_ConstantFloat, _RegContext.m_pContext, "Floats"));
		_RegContext.m_pContext->AddUserConstantParser(MNew2(CParseHandler_ConstantInt, _RegContext.m_pContext, "Integers"));
	}

}
