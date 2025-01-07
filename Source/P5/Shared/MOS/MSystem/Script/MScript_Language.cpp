
#include "PCH.h"

#include "MScript.h"

#if DScriptDebug>0
#pragma  optimize("",off)
#pragma  inline_depth(0)
#endif

namespace NScript
{

	class CStatement_Return : public CStatement
	{
	public:

		CStatement_Return()
		{
		}

		virtual void Evaluate(CExecutionContext &_ExecutionContext)
		{
		}

		virtual int PostParse(int _iParent, CStr &_ErrorReturn)
		{
			if (_iParent != 0)
			{
				_ErrorReturn = "Return statment cannot be in the middle of an expression";
			}

			for (int i = 1; i < m_pParentStatement->m_splStatements.Len(); ++i)
			{
				spCStatement spTemp = m_pParentStatement->m_splStatements[i];
				spTemp->m_pParentStatement = this;
				m_splStatements.Add(spTemp);
			}

			m_pParentStatement->m_splStatements.SetLen(1);

			if (!CStatement::PostParse(_iParent, _ErrorReturn))
				return false;

/*			if (m_splStatements.Len() != 1 && !m_bVoid)
			{
				_ErrorReturn = "return statement requires a value be returned";
				return false;
			}
			else if (m_splStatements.Len() && m_bVoid)
			{
				_ErrorReturn = "return statement cannot return a value in a void function";
				return false;
			}*/

			return true;
		}


		virtual CStr GetDesc()
		{
			return "CStatement_Return";
		}

	};

	class CParseHandler_UserLang_Return : public CParseHandler
	{
		MRTC_DECLARE;
	public:
		
		CParseHandler_UserLang_Return(CContext *_pParser, const char *_pName) : CParseHandler(_pParser, _pName)
		{
		}

		int TryParse(CParseContext &_ParseContext)
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

			if (Ident != "return")
				return 0;

			TPtr<CStatement_Return> spLocalVar = MNew(CStatement_Return);
			spLocalVar->m_pParentStatement = _ParseContext.m_pFunctionStatementHolder;
			_ParseContext.m_pFunctionStatementHolder->m_splStatements.Add((CStatement *)spLocalVar);
			
			_ParseContext.m_pStr = pStr;
			return 1;
		}

	};

	void g_RegisterConstantParsers(CRegisterContext & _RegContext);

	class CScriptLanguageCore : public NScript::CClient
	{
		MRTC_DECLARE;

		class CMath : public CClient
		{
			// Commands.
			static fp64 Cos(fp64 x) { return M_Cos(x); };
			static fp64 Sin(fp64 x) { return M_Sin(x); };
			static fp64 Tan(fp64 x) { return M_Tan(x); };
			static fp64 Acos(fp64 x) { return M_ACos(x); };
			static fp64 Asin(fp64 x) { return M_ASin(x); };
			static fp64 Atan(fp64 x) { return M_ATan(x); };
			static fp64 Cosh(fp64 x) { return M_Cosh(x); };
			static fp64 Sinh(fp64 x) { return M_Sinh(x); };
			static fp64 Tanh(fp64 x) { return M_Tanh(x); };
			static fp64 Exp(fp64 x) { return M_Exp(x); };
			static fp64 Abs(fp64 x) { return M_Fabs(x); };
			static fp64 Log(fp64 x) { return M_Log(x); };
			static fp64 Log10(fp64 x) { return M_Log10(x); };
			static fp64 Floor(fp64 x) { return ::Floor(x); };
			static fp64 Ceil(fp64 x) { return ::Ceil(x); };
			static fp64 Mod(fp64 x,fp64 y) { return M_FMod(x,y); };
			static void Srand(int32 Seed) { MRTC_GetRand()->InitRand(Seed); };
			static int32 Rand() { return MRTC_RAND(); };

			public:

			void Register(CRegisterContext & _RegContext)
			{
				_RegContext.RegFunction("cos", Cos);
				_RegContext.RegFunction("sin", Sin);
				_RegContext.RegFunction("tan", Tan);
				_RegContext.RegFunction("acos", Acos);
				_RegContext.RegFunction("asin", Asin);
				_RegContext.RegFunction("atan", Atan);
				_RegContext.RegFunction("cosh", Cosh);
				_RegContext.RegFunction("sinh", Sinh);
				_RegContext.RegFunction("tanh", Tanh);
				_RegContext.RegFunction("exp", Exp);
				_RegContext.RegFunction("abs", Abs);
				_RegContext.RegFunction("log", Log);
				_RegContext.RegFunction("log10", Log10);
				_RegContext.RegFunction("floor", Floor);
				_RegContext.RegFunction("ceil", Ceil); 
				_RegContext.RegFunction("mod", Mod); 
				_RegContext.RegFunction("srand", Srand);
				_RegContext.RegFunction("rand", Rand);

				// Math constants.
				_RegContext.RegConstant("PI",3.14159265358979323846f);
				_RegContext.RegConstant("RAND_MAX",RAND_MAX);
			}
		};

		CMath m_Math;

		class CStdTypes : public CClient
		{
			public:

			class CVoidClass : public CScriptUserClass
			{
			public:
				CVoidClass()
				{
					m_DataSize = 0;
				}
				void Construct(void *_pMemory)
				{
				}

				void Destruct(void *_pMemory)
				{
				}

				void Copy(void *_pDest, const void *_pSource)
				{
				}
			};

			void Register(CRegisterContext & _RegContext)
			{
				// Register common variable types

				_RegContext.RegClass("void", MNew(CVoidClass));

#ifdef M_SEPARATETYPE_int
				_RegContext.RegClass<int>("int");
#endif
				_RegContext.RegClass<int8>("int8");
				_RegContext.RegClass<int16>("int16");
				_RegContext.RegClass<int32>("int32");
				_RegContext.RegClass<int64>("int64");
				_RegContext.RegClass<uint8>("uint8");
				_RegContext.RegClass<uint16>("uint16");
				_RegContext.RegClass<uint32>("uint32");
				_RegContext.RegClass<uint64>("uint64");
				_RegContext.RegClass<fp32>("fp32");
				_RegContext.RegClass<fp64>("fp64");
				_RegContext.RegClass<const char *>("str");
				_RegContext.RegClass<CStr>("CStr");
	

				// Constants
				_RegContext.RegConstant("true",1);
				_RegContext.RegConstant("false",0);
			}
		};

		CStdTypes m_StdTypes;
#ifdef PLATFORM_WIN
	#pragma warning(push)
	#pragma warning(disable:4146)
#endif

		template <typename t_CType>
		class TCIntOperators : public CClient
		{
			static t_CType Add(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 + _Var1;
			}

			static t_CType Substract(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 - _Var1;
			}

			static t_CType Multiply(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 * _Var1;
			}

			static t_CType Divide(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 / _Var1;
			}

			static t_CType Modulus(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 % _Var1;
			}

			static int Equal(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 == _Var1;
			}

			static int NotEqual(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 != _Var1;
			}

			static int Less(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 < _Var1;
			}

			static int Greater(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 > _Var1;
			}

			static int LessEqual(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 <= _Var1;
			}

			static int GreaterEqual(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 >= _Var1;
			}

			static int LeftShift(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 << _Var1;
			}

			static int RightShift(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 >> _Var1;
			}

			static int BitwiseAnd(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 & _Var1;
			}

			static int BitwiseOr(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 | _Var1;
			}

			static int BitwiseXor(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 ^ _Var1;
			}

			// Assignment Operators
			static t_CType Assign(t_CType _Var0, t_CType _Var1)
			{
				return _Var1;
			}

			static t_CType PostPlusPlus(t_CType _Var0)
			{
				return _Var0 + 1;
			}

			static t_CType PostMinusMinus(t_CType _Var0)
			{
				return _Var0 - 1;
			}

			static t_CType PrePlusPlus(t_CType _Var0)
			{
				return _Var0 + 1;
			}

			static t_CType PreMinusMinus(t_CType _Var0)
			{
				return _Var0 - 1;
			}

			// Prefix
			static t_CType BitwiseComplement(t_CType _Var0)
			{
				return ~_Var0;
			}

			static t_CType Negate(t_CType _Var0)
			{
				return -_Var0;
			}

			static t_CType Convert(t_CType _Var0)
			{
				return _Var0;
			}

			public:
			void Register(CRegisterContext & _RegContext)
			{
				// Operators
				_RegContext.RegBinaryOperator("+",Add);
				_RegContext.RegBinaryOperator("-",Substract);
				_RegContext.RegBinaryOperator("*",Multiply);
				_RegContext.RegBinaryOperator("/",Divide);
				_RegContext.RegBinaryOperator("%",Modulus);
				_RegContext.RegBinaryOperator("==",Equal);
				_RegContext.RegBinaryOperator("!=",NotEqual);
				_RegContext.RegBinaryOperator("<",Less);
				_RegContext.RegBinaryOperator(">",Greater);
				_RegContext.RegBinaryOperator("<=",LessEqual);
				_RegContext.RegBinaryOperator(">=",GreaterEqual);
				_RegContext.RegBinaryOperator("<<", LeftShift);
				_RegContext.RegBinaryOperator(">>", RightShift);
				_RegContext.RegBinaryOperator("&",BitwiseAnd);
				_RegContext.RegBinaryOperator("|",BitwiseOr);
				_RegContext.RegBinaryOperator("^",BitwiseXor);

				// Assignment Operators
				_RegContext.RegAssignmentOperator("=",Assign);
				_RegContext.RegAssignmentOperator("*=",Multiply);
				_RegContext.RegAssignmentOperator("/=",Divide);
				_RegContext.RegAssignmentOperator("%=",Modulus);
				_RegContext.RegAssignmentOperator("+=",Add);
				_RegContext.RegAssignmentOperator("-=",Substract);
				_RegContext.RegAssignmentOperator("<<=",LeftShift);
				_RegContext.RegAssignmentOperator(">>=",RightShift);
				_RegContext.RegAssignmentOperator("&=",BitwiseAnd);
				_RegContext.RegAssignmentOperator("^=",BitwiseXor);
				_RegContext.RegAssignmentOperator("|=",BitwiseOr);

				// Postfix
				_RegContext.RegVarPostfixOperator("++",PostPlusPlus);
				_RegContext.RegVarPostfixOperator("--",PostMinusMinus);
				_RegContext.RegVarPrefixOperator("++",PrePlusPlus);
				_RegContext.RegVarPrefixOperator("--",PreMinusMinus);

				// Prefix
				_RegContext.RegUnaryPrefixOperator("~",BitwiseComplement);
				_RegContext.RegUnaryPrefixOperator("-",Negate);

#ifdef M_SEPARATETYPE_int
				_RegContext.RegConversion<t_CType, int>();
#endif
				_RegContext.RegConversion<t_CType, int8>();
				_RegContext.RegConversion<t_CType, int16>();
				_RegContext.RegConversion<t_CType, int32>();
				_RegContext.RegConversion<t_CType, int64>();
				_RegContext.RegConversion<t_CType, uint8>();
				_RegContext.RegConversion<t_CType, uint16>();
				_RegContext.RegConversion<t_CType, uint32>();
				_RegContext.RegConversion<t_CType, uint64>();
				_RegContext.RegConversion<t_CType, fp32>();
				_RegContext.RegConversion<t_CType, fp64>();

				_RegContext.RegFunction(CStr("Convert_") + ScriptTypeName<t_CType>(), Convert);

			}
		};

		class CStrOperators : public CClient
		{

			static CStr Add_CStr_str(CStr _Var0, const char *_Var1)
			{
				return _Var0 + _Var1;
			}

			static CStr Add_str_CStr(const char * _Var0, CStr _Var1)
			{
				return CStr(_Var0) + _Var1;
			}

			static CStr Add_str_str(const char * _Var0, const char * _Var1)
			{
				return CStr(_Var0) + CStr(_Var1);
			}

			static CStr Add_CStr_CStr(CStr _Var0, CStr _Var1)
			{
				return _Var0 + _Var1;
			}

			// Assignment Operators
			static CStr Assign_CStr_CStr(CStr _Var0, CStr _Var1)
			{
				return _Var1;
			}

			static CStr Assign_CStr_str(CStr _Var0, const char *_Var1)
			{
				return _Var1;
			}

			static int Equal_CStr_CStr(CStr _Var0, CStr _Var1)
			{
				return _Var0 == _Var1;
			}

			static int Equal_str_str(const char *_Var0, const char *_Var1)
			{
				return CStr(_Var0) == CStr(_Var1);
			}

			static int Equal_str_CStr(const char *_Var0, CStr _Var1)
			{
				return CStr(_Var0) == _Var1;
			}

			static int Equal_CStr_str(CStr _Var0, const char * _Var1)
			{
				return _Var0 == CStr(_Var1);
			}


			static int NotEqual_CStr_CStr(CStr _Var0, CStr _Var1)
			{
				return _Var0 != _Var1;
			}

			static int NotEqual_str_str(const char *_Var0, const char *_Var1)
			{
				return CStr(_Var0) != CStr(_Var1);
			}

			static int NotEqual_str_CStr(const char *_Var0, CStr _Var1)
			{
				return CStr(_Var0) != _Var1;
			}

			static int NotEqual_CStr_str(CStr _Var0, const char * _Var1)
			{
				return _Var0 != CStr(_Var1);
			}


			public:
			void Register(CRegisterContext & _RegContext)
			{
				// Operators
				_RegContext.RegBinaryOperator("+",Add_CStr_CStr);
				_RegContext.RegBinaryOperator("+",Add_CStr_str);
				_RegContext.RegBinaryOperator("+",Add_str_str);
				_RegContext.RegBinaryOperator("+",Add_str_CStr);

				_RegContext.RegBinaryOperator("==",Equal_CStr_CStr);
				_RegContext.RegBinaryOperator("==",Equal_str_CStr);
				_RegContext.RegBinaryOperator("==",Equal_CStr_str);
				_RegContext.RegBinaryOperator("==",Equal_str_str);

				_RegContext.RegBinaryOperator("!=",NotEqual_CStr_CStr);
				_RegContext.RegBinaryOperator("!=",NotEqual_str_CStr);
				_RegContext.RegBinaryOperator("!=",NotEqual_CStr_str);
				_RegContext.RegBinaryOperator("!=",NotEqual_str_str);

				// Assignment Operators
				_RegContext.RegAssignmentOperator("=",Assign_CStr_CStr);
				_RegContext.RegAssignmentOperator("=",Assign_CStr_str);

			}
		};

#ifdef PLATFORM_WIN
	#pragma warning(pop)
#endif

#ifdef M_SEPARATETYPE_int
		TCIntOperators<int> m_IntOperators_int;
#endif
		TCIntOperators<int8> m_IntOperators_int8;
		TCIntOperators<int16> m_IntOperators_int16;
		TCIntOperators<int32> m_IntOperators_int32;
		TCIntOperators<int64> m_IntOperators_int64;
		TCIntOperators<uint8> m_IntOperators_uint8;
		TCIntOperators<uint16> m_IntOperators_uint16;
		TCIntOperators<uint32> m_IntOperators_uint32;
		TCIntOperators<uint64> m_IntOperators_uint64;

		template <typename t_CType>
		class TCFloatOperators : public CClient
		{
			static t_CType Add(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 + _Var1;
			}

			static t_CType Substract(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 - _Var1;
			}

			static t_CType Multiply(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 * _Var1;
			}

			static t_CType Divide(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 / _Var1;
			}

			static int Equal(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 == _Var1;
			}

			static int NotEqual(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 != _Var1;
			}

			static int Less(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 < _Var1;
			}

			static int Greater(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 > _Var1;
			}

			static int LessEqual(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 <= _Var1;
			}

			static int GreaterEqual(t_CType _Var0, t_CType _Var1)
			{
				return _Var0 >= _Var1;
			}

			// Assignment Operators
			static t_CType Assign(t_CType _Var0, t_CType _Var1)
			{
				return _Var1;
			}

			static t_CType PostPlusPlus(t_CType _Var0)
			{
				return _Var0 + 1;
			}

			static t_CType PostMinusMinus(t_CType _Var0)
			{
				return _Var0 - 1;
			}

			static t_CType PrePlusPlus(t_CType _Var0)
			{
				return _Var0 + 1;
			}

			static t_CType PreMinusMinus(t_CType _Var0)
			{
				return _Var0 - 1;
			}

			static t_CType Negate(t_CType _Var0)
			{
				return -_Var0;
			}

			static t_CType Convert(t_CType _Var0)
			{
				return _Var0;
			}

			public:
			void Register(CRegisterContext & _RegContext)
			{
				// Operators
				_RegContext.RegBinaryOperator("+",Add);
				_RegContext.RegBinaryOperator("-",Substract);
				_RegContext.RegBinaryOperator("*",Multiply);
				_RegContext.RegBinaryOperator("/",Divide);
				_RegContext.RegBinaryOperator("==",Equal);
				_RegContext.RegBinaryOperator("!=",NotEqual);
				_RegContext.RegBinaryOperator("<",Less);
				_RegContext.RegBinaryOperator(">",Greater);
				_RegContext.RegBinaryOperator("<=",LessEqual);
				_RegContext.RegBinaryOperator(">=",GreaterEqual);

				// Assignment Operators
				_RegContext.RegAssignmentOperator("=",Assign);
				_RegContext.RegAssignmentOperator("*=",Multiply);
				_RegContext.RegAssignmentOperator("/=",Divide);
				_RegContext.RegAssignmentOperator("+=",Add);
				_RegContext.RegAssignmentOperator("-=",Substract);

				// Postfix
				_RegContext.RegVarPostfixOperator("++",PostPlusPlus);
				_RegContext.RegVarPostfixOperator("--",PostMinusMinus);
				_RegContext.RegVarPrefixOperator("++",PrePlusPlus);
				_RegContext.RegVarPrefixOperator("--",PreMinusMinus);

				// Prefix
				_RegContext.RegUnaryPrefixOperator("-",Negate);

#ifdef M_SEPARATETYPE_int
				_RegContext.RegConversion<t_CType, int>();
#endif
				_RegContext.RegConversion<t_CType, int8>();
				_RegContext.RegConversion<t_CType, int16>();
				_RegContext.RegConversion<t_CType, int32>();
				_RegContext.RegConversion<t_CType, int64>();
				_RegContext.RegConversion<t_CType, uint8>();
				_RegContext.RegConversion<t_CType, uint16>();
				_RegContext.RegConversion<t_CType, uint32>();
				_RegContext.RegConversion<t_CType, uint64>();
				_RegContext.RegConversion<t_CType, fp32>();
				_RegContext.RegConversion<t_CType, fp64>();

				_RegContext.RegFunction(CStr("Convert_") + ScriptTypeName<t_CType>(), Convert);

			}
		};

		TCFloatOperators<fp32> m_FloatOperators_fp32;
		TCFloatOperators<fp64> m_FloatOperators_fp64;

		class CBooleanOperators : public CClient
		{
			static int And(int _Var0, int _Var1)
			{
				return _Var0 && _Var1;
			}

			static int Or(int _Var0, int _Var1)
			{
				return _Var0 || _Var1;
			}

			static int Not(int _Var0)
			{
				return !_Var0;
			}

			public:
			void Register(CRegisterContext & _RegContext)
			{
				_RegContext.RegBinaryOperator("&&",And);
				_RegContext.RegBinaryOperator("||",Or);
				_RegContext.RegUnaryPrefixOperator("!",Not);
			}
		};

		CBooleanOperators m_BoleanOperators;

		class CConversions : public CClient
		{
			public:
			void Register(CRegisterContext & _RegContext)
			{
				_RegContext.RegConversion<CStr, const char *>();
				_RegContext.RegConversion<const char *, CStr>();
			}
		};

		CConversions m_Conversions;

		CStrOperators m_StrOperators;

	/*
		// Commands.
		static CScriptMultiType IfElse(CScriptInstance& _Script, CScriptNode& _Node);
		static CScriptMultiType For(CScriptInstance& _Script, CScriptNode& _Node);
		static CScriptMultiType While(CScriptInstance& _Script, CScriptNode& _Node);
		static CScriptMultiType DoWhile(CScriptInstance& _Script, CScriptNode& _Node);
		static CScriptMultiType Switch(CScriptInstance& _Script, CScriptNode& _Node);

		// Exception handling commands.
		static int32 TryCatch(CScriptNode& _Node, CScriptInstance& _Script);
		static int32 Throw(CScriptNode& _Node, CScriptInstance& _Script);

		// Numerical operators.
		static int32 Equal(fp64 a, fp64 b);
		static int32 NotEqual(fp64 a, fp64 b);
		static int32 LessThan(fp64 a, fp64 b);
		static int32 GreaterThan(fp64 a, fp64 b);
		static int32 LessEqual(fp64 a, fp64 b);
		static int32 GreaterEqual(fp64 a, fp64 b);
		static int32 Not(int32 a);
		static CScriptMultiType And(CScriptInstance& _Script, CScriptNode& _Node);
		static CScriptMultiType Or(CScriptInstance& _Script, CScriptNode& _Node);
		static int32 BitwiseAnd(int32 a, int32 b);
		static int32 BitwiseOr(int32 a, int32 b);

		static CScriptMultiType PrePlusPlus(CScriptInstance& _Script, CScriptNode& _Node);
		static CScriptMultiType PreMinusMinus(CScriptInstance& _Script, CScriptNode& _Node);
		static CScriptMultiType PostPlusPlus(CScriptInstance& _Script, CScriptNode& _Node);
		static CScriptMultiType PostMinusMinus(CScriptInstance& _Script, CScriptNode& _Node);

		// String operators.
		static int32 StrEqual(CStr& a, CStr& b);
		static int32 StrNotEqual(CStr& a, CStr& b);
		static int32 StrLessThan(CStr& a, CStr& b);
		static int32 StrGreaterThan(CStr& a, CStr& b);
		static int32 StrLessEqual(CStr& a, CStr& b);
		static int32 StrGreaterEqual(CStr& a, CStr& b);*/

	public:

		virtual void RegisterToParser(CContext *_pParser)
		{
			// Types are required first
			m_StdTypes.RegisterToParser(_pParser);
			m_BoleanOperators.RegisterToParser(_pParser);
			m_Conversions.RegisterToParser(_pParser);
			m_StrOperators.RegisterToParser(_pParser);
			CClient::RegisterToParser(_pParser);
#ifdef M_SEPARATETYPE_int
			m_IntOperators_int.RegisterToParser(_pParser);
#endif
			m_IntOperators_int8.RegisterToParser(_pParser);
			m_IntOperators_int16.RegisterToParser(_pParser);
			m_IntOperators_int32.RegisterToParser(_pParser);
			m_IntOperators_int64.RegisterToParser(_pParser);
			m_IntOperators_uint8.RegisterToParser(_pParser);
			m_IntOperators_uint16.RegisterToParser(_pParser);
			m_IntOperators_uint32.RegisterToParser(_pParser);
			m_IntOperators_uint64.RegisterToParser(_pParser);
			m_FloatOperators_fp32.RegisterToParser(_pParser);
			m_FloatOperators_fp64.RegisterToParser(_pParser);
			m_Math.RegisterToParser(_pParser);
		}

		virtual void UnRegister(CContext *_pParser)
		{
			m_Math.UnRegister(_pParser);
#ifdef M_SEPARATETYPE_int
			m_IntOperators_int.UnRegister(_pParser);
#endif
			m_IntOperators_int8.UnRegister(_pParser);
			m_IntOperators_int16.UnRegister(_pParser);
			m_IntOperators_int32.UnRegister(_pParser);
			m_IntOperators_int64.UnRegister(_pParser);
			m_IntOperators_uint8.UnRegister(_pParser);
			m_IntOperators_uint16.UnRegister(_pParser);
			m_IntOperators_uint32.UnRegister(_pParser);
			m_IntOperators_uint64.UnRegister(_pParser);
			m_FloatOperators_fp32.UnRegister(_pParser);
			m_FloatOperators_fp64.UnRegister(_pParser);
			CClient::UnRegister(_pParser);
			m_StrOperators.UnRegister(_pParser);
			m_Conversions.UnRegister(_pParser);
			m_BoleanOperators.UnRegister(_pParser);
			m_StdTypes.UnRegister(_pParser);
		}

		virtual void UnRegister()
		{
			m_Math.UnRegister();
#ifdef M_SEPARATETYPE_int
			m_IntOperators_int.UnRegister();
#endif
			m_IntOperators_int8.UnRegister();
			m_IntOperators_int16.UnRegister();
			m_IntOperators_int32.UnRegister();
			m_IntOperators_int64.UnRegister();
			m_IntOperators_uint8.UnRegister();
			m_IntOperators_uint16.UnRegister();
			m_IntOperators_uint32.UnRegister();
			m_IntOperators_uint64.UnRegister();
			m_FloatOperators_fp32.UnRegister();
			m_FloatOperators_fp64.UnRegister();
			CClient::UnRegister();
			m_StrOperators.UnRegister();
			m_Conversions.UnRegister();
			m_BoleanOperators.UnRegister();
			m_StdTypes.UnRegister();
		}

		void Register(CRegisterContext & _RegContext)
		{
			g_RegisterConstantParsers(_RegContext);
			// Constant parsers
			_RegContext.m_pContext->AddUserLangParser(MNew2(CParseHandler_UserLang_Return, _RegContext.m_pContext, "return"));
	/*		// Core language functionality
			_RegContext.RegLanguageConstruct("if ( @expr(int)@ ) @stat@ [else @stat@]",IfElse);
			_RegContext.RegLanguageConstruct("for ( [@stat@]; [@expr(int)@]; [@stat@]) @stat@",For);
			_RegContext.RegLanguageConstruct("while (@expr(int)@) @stat@",While);
			_RegContext.RegLanguageConstruct("do @stat@ while ( @expr(int)@ )",DoWhile);
			_RegContext.RegLanguageConstruct("switch ( @expr(int)@ ) { *(case @expr(int)@ : @stat@) [default : @stat@] }",Switch);

			// Conditional Expression
			_RegContext.RegLanguageConstruct("@expr(int)@ ? @expr@ : @expr@", Conditional);

			// Exceptions
			_RegContext.RegLanguageConstruct("try @stat@ catch ([@typedecl@]) @stat@", TryCatch);
			_RegContext.RegLanguageConstruct("throw @expr@", Throw);

			// Flow contol
			_RegContext.RegLanguageConstruct("return [@expr@] ;",Return);
			_RegContext.RegLanguageConstruct("break ;",Break);*/


		}
	};

	MRTC_IMPLEMENT(CParseHandler_UserLang_Return, CParseHandler);

}

using namespace NScript;
MRTC_IMPLEMENT_DYNAMIC(CScriptLanguageCore, CClient);

