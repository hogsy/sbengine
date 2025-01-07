#ifndef DInc_MScript_Convert_h
#define DInc_MScript_Convert_h

namespace NStr
{

	/************************************************************************************************\
	||¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯||
	|| Character funcs
	||______________________________________________________________________________________________||
	\************************************************************************************************/

	template<class t_CData>
		mint StrLen(const t_CData *_pStr)
	{
		const t_CData *pStr = _pStr;
		while (*pStr)
			++pStr;
		return pStr - _pStr;;
	}

	template<class t_CData>
		DIdsPInlineM t_CData CharUpperCase(t_CData _Character)
	{
		if (_Character >= 'a' && _Character <= 'z')
			_Character -= 'a' - 'A';

		return _Character;
	}

	template<class t_CData>
		DIdsPInlineM t_CData CharLowerCase(t_CData _Character)
	{
		if (_Character >= 'A' && _Character <= 'A')
			_Character += 'a' - 'A';

		return _Character;
	}

	template<class t_CData>
		DIdsPInlineM bint CharIsWhiteSpace(const t_CData _Character)
	{
		switch (_Character)
		{
		case 32 : return true;
		case 8 : return true;
		case 9 : return true;
		case 10 : return true;
		case 13 : return true;
		}
		return false;			
	}

	template<class t_CData>
		DIdsPInlineL bint CharIsAlphabetical(const t_CData _Character)
	{
		if (_Character >= 'A' && _Character <= 'Z')
			return true;
		if (_Character >= 'a' && _Character <= 'z')
			return true;

//		if (_Character >= 0x7f || _Character < 0)
//			return true;
		if (_Character & ~0x7f)
			return true;

		return false;

	}

	template<class t_CData>
		DIdsPInlineL bint CharIsAnsiAlphabetical(const t_CData _Character)
	{
		if (_Character >= 'A' && _Character <= 'Z')
			return true;
		if (_Character >= 'a' && _Character <= 'z')
			return true;

		return false;

	}

	template<class t_CData>
		DIdsPInlineM bint CharIsNumber(const t_CData _Character)
	{
		if (_Character >= '0' && _Character <= '9')
			return true;

		return false;
	}

	template<class t_CData, class t_CReturn, class t_CTerminator>
		t_CReturn StrToFloatParse(const t_CData *&_pStr, t_CReturn _FailValue, t_CTerminator *_pStrTerminators = (const ch8 *)DNP)
	{
		t_CReturn DestNumber = 0.0;
		t_CReturn DestDecimals = 0.0;
		t_CReturn DestExponent = 0.0;
		t_CReturn DecimalPlace = 1.0;
		const t_CData *&pParseStr = _pStr;

		aint SearchMode = 0;
		t_CReturn Sign = 1.0;
		t_CReturn ExponentSign = 1.0;
		aint bFoundNum = false;

		// Parse for characters, and end if str terminator is found
		while ((*pParseStr))
		{
			if (_pStrTerminators)
			{
				const t_CTerminator *pStrTerminators = _pStrTerminators;

				while (*pStrTerminators)
				{
					if ((*pParseStr) == (*pStrTerminators))
						goto Return;

					++pStrTerminators;
				}
			}

			switch (SearchMode)
			{
			case 0:
				{
					if ((*pParseStr) == '+')
					{
						Sign = 1.0;
					}
					else if ((*pParseStr) == '-')
					{
						Sign = -1.0;
					}
					else if ((*pParseStr) == 'e' || (*pParseStr) == 'E')
					{
						SearchMode = 3;
						DestNumber = 1.0;
					}
					else if (CharIsNumber((*pParseStr)))
					{							
						SearchMode = 1;
						--pParseStr;
					}						
					else if ((*pParseStr) == '.')
					{							
						SearchMode = 2;
					}
					else if (!CharIsWhiteSpace((*pParseStr)))
						return _FailValue;
				}
				break;
			case 1:
				// Search Pre number
				{
					if ((*pParseStr) >= '0' && (*pParseStr) <= '9')
					{
						DestNumber *= t_CReturn(10.0);
						DestNumber += (t_CReturn)((*pParseStr) - '0');
						bFoundNum = true;
					}
					else if ((*pParseStr) == '.')
					{							
						SearchMode = 2;
					}
					else if ((*pParseStr) == 'e' || (*pParseStr) == 'E')
					{
						SearchMode = 3;
					}
					else if (CharIsWhiteSpace((*pParseStr)))
					{
						SearchMode = 5;
					}
					else
					{
						return _FailValue;
					}
				}
				break;
			case 2:
				// Post
				{
					if ((*pParseStr) >= '0' && (*pParseStr) <= '9')
					{
						DecimalPlace *= t_CReturn(10.0);
						DestDecimals += ((t_CReturn)((*pParseStr) - '0')) / DecimalPlace;
						bFoundNum = true;
					}
					else if ((*pParseStr) == 'e' || (*pParseStr) == 'E')
					{							
						SearchMode = 3;
					}
					else if (CharIsWhiteSpace((*pParseStr)))
					{
						SearchMode = 5;
					}
					else
					{
						return _FailValue;
					}
				}
				break;
			case 3:
				// Exponent
				{
					if ((*pParseStr) >= '0' && (*pParseStr) <= '9')
					{
						DestExponent *= t_CReturn(10.0);
						DestExponent += (t_CReturn)((*pParseStr) - '0');
						bFoundNum = true;
					}
					else if ((*pParseStr) == '-')
					{							
						ExponentSign = -1.0;
					}
					else if ((*pParseStr) == '+')
					{							
						ExponentSign = -1.0;
					}
					else if (CharIsWhiteSpace((*pParseStr)))
					{
						SearchMode = 5;
					}
					else
					{
						return _FailValue;
					}
				}
				break;

			case 5:
				// The end
				{
					if (!CharIsWhiteSpace((*pParseStr)))
						DestNumber = _FailValue;
				}
				break;
			}

			++pParseStr;

		}	

Return:

		if (bFoundNum)
		{			
			return (DestNumber + DestDecimals) * M_Pow((t_CReturn)10.0, DestExponent * ExponentSign) * Sign;
		}
		else
		{
			return _FailValue;
		}
	}

	template<class t_CData, class t_CReturn>
		t_CReturn StrToFloatParse(const t_CData *&_pStr, t_CReturn _FailValue)
	{
		return StrToFloatParse(_pStr, _FailValue, (const ch8 *)NULL);
	}

	template<class t_CData, class t_CReturn, class t_CTerminator>
		t_CReturn StrToFloat(const t_CData *_pStr, t_CReturn _FailValue, t_CTerminator *_pStrTerminators = (const ch8 *)DNP)
	{
		const t_CData *pParse = _pStr;
		return StrToFloatParse(pParse, _FailValue, _pStrTerminators);
	}

	template<class t_CData, class t_CReturn>
		t_CReturn StrToFloat(const t_CData *_pStr, t_CReturn _FailValue)
	{
		const t_CData *pParse = _pStr;
		return StrToFloatParse(pParse, _FailValue, (const ch8 *)NULL);
	}


	template<class t_CData, class t_CReturn, class t_CTerminator>
		DIdsPInlineS t_CReturn StrToInt(const t_CData *_pStr, t_CReturn _FailValue, t_CTerminator *_pStrTerminators = (const ch8 *)DNP)
	{
		const t_CData *pStr = _pStr;
		return StrToIntParse(pStr, _FailValue, _pStrTerminators);
	}

	template<class t_CData, class t_CReturn>
		DIdsPInlineS t_CReturn StrToInt(const t_CData *_pStr, t_CReturn _FailValue)
	{
		const t_CData *pStr = _pStr;
		return StrToIntParse(pStr, _FailValue, (const ch8 *)DNP);
	}

	template<class t_CData, class t_CReturn, class t_CTerminator>
		t_CReturn StrToIntParse(const t_CData *&_pStr, t_CReturn _FailValue, t_CTerminator *_pStrTerminators = (const ch8 *)DNP)
	{
		t_CReturn DestNumber = 0;
		const t_CData *pParseStr = _pStr;

		aint SearchMode = 0;
		t_CReturn Sign = 1;

		aint bFoundNum = false;

		// Parse for characters, and end if str terminator is found
		while ((*pParseStr))
		{
			if (_pStrTerminators)

			{
				const t_CTerminator *pStrTerminators = _pStrTerminators;

				while (*pStrTerminators)
				{
					if ((*pParseStr) == (*pStrTerminators))
						goto Return;

					++pStrTerminators;
				}
			}

			switch (SearchMode)
			{
			case 0:
				{
					if ((*pParseStr) == '+')
					{
						Sign = 1;
					}
					else if ((*pParseStr) == '-')
					{
						Sign = -1;
					}
					else if (CharIsNumber((*pParseStr)))
					{							
						if (CharIsAlphabetical((*(pParseStr+1))))
						{
							if ((*pParseStr) == '0' && (((*(pParseStr + 1)) == 'x') || ((*(pParseStr + 1)) == 'X')))
							{
								// HexString
								SearchMode = 2;

								++pParseStr;
							}
							else if ((*pParseStr) == '0' && (((*(pParseStr + 1)) == 'b') || ((*(pParseStr + 1)) == 'B')))
							{
								// Binary
								SearchMode = 3;

								++pParseStr;
							}
							else if ((*pParseStr) == '0' && (((*(pParseStr + 1)) == 'o') || ((*(pParseStr + 1)) == 'O')))
							{
								// Octal
								SearchMode = 4;

								++pParseStr;
							}
							else
							{
								// Base 10
								SearchMode = 1;

								continue;
							}
						}
						else
						{
							// Base 10
							SearchMode = 1;

							continue;
						}
					}						
					else if (!CharIsWhiteSpace((*pParseStr)))
					{
						//_pStr = pParseStr;
						return _FailValue;
					}
				}
				break;
			case 1:
				// Base 10
				{
					while ((*pParseStr) && (*pParseStr) >= '0' && (*pParseStr) <= '9')
					{
						DestNumber *= 10;
						DestNumber += (*pParseStr) - '0';
						bFoundNum = true;
						++pParseStr;
					}

					goto Return;
				}
				break;
			case 2:
				// Hex
				{
					while ((*pParseStr))
					{
						if ((*pParseStr) >= '0' && (*pParseStr) <= '9')
						{
							aint Num = (*pParseStr) - '0';

							DestNumber *= 16;
							DestNumber += Num;
							bFoundNum = true;
						}
						else if ((*pParseStr) >= 'a' && (*pParseStr) <= 'f')
						{
							aint Num = ((*pParseStr) - 'a') + 10;

							DestNumber *= 16;
							DestNumber += Num;										  
							bFoundNum = true;
						}
						else if ((*pParseStr) >= 'A' && (*pParseStr) <= 'F')
						{
							aint Num = ((*pParseStr) - 'A') + 10;

							DestNumber *= 16;
							DestNumber += Num;										  
							bFoundNum = true;
						}
						else
						{
							goto Return;
						}
						++pParseStr;
					}
				}
				break;
			case 3:
				// Binary
				{
					while ((*pParseStr))
					{
						if ((*pParseStr) >= '0' && (*pParseStr) <= '1')
						{
							aint Num = (*pParseStr) - '0';

							DestNumber *= 2;
							DestNumber += Num;
							bFoundNum = true;
						}
						else
						{
							goto Return;
						}
						++pParseStr;
					}
				}
				break;
			case 4:
				// Octal
				{
					while ((*pParseStr))
					{
						if ((*pParseStr) >= '0' && (*pParseStr) <= '7')
						{
							aint Num = (*pParseStr) - '0';

							DestNumber *= 8;
							DestNumber += Num;
							bFoundNum = true;
						}
						else
						{
							goto Return;
						}
						++pParseStr;
					}
				}
				break;
			case 5:
				// The end
				{

					if (!CharIsWhiteSpace((*pParseStr)))
					{
						_pStr = pParseStr;
						return _FailValue;
					}
				}
				break;
			}

			if (*pParseStr)
				++pParseStr;

		}	

Return:

		_pStr = pParseStr;

		if (bFoundNum)			
			return DestNumber * Sign;
		else
			return _FailValue;
	}

	template<class t_CData, class t_CReturn>
		t_CReturn StrToIntParse(const t_CData *&_pStr, t_CReturn _FailValue)
	{
		return StrToIntParse(_pStr, _FailValue, (const ch8 *)DNP);
	}

	template<class t_CData, class t_CReturn>
		DIdsPInlineS t_CReturn StrToIntBase10(const t_CData *_pStr, t_CReturn _FailValue)
	{
		const t_CData *pStr = _pStr;
		return StrToIntBase10Parse(pStr, _FailValue);
	}


	template<class t_CData, class t_CReturn>
		t_CReturn StrToIntBase10Parse(const t_CData *&_pStr, t_CReturn _FailValue)
	{
		t_CReturn DestNumber = 0;
		const t_CData *pParseStr = _pStr;

		t_CReturn Sign = 1;

		aint bFoundNum = false;

		if ((*pParseStr) == '-')
		{
			++pParseStr;
			Sign = -1;
		}

		// Parse for characters, and end if str terminator is found
		while ((*pParseStr))
		{
			if ((*pParseStr) < '0' || (*pParseStr) > '9')
				break;

			DestNumber *= 10;
			DestNumber += (*pParseStr) - '0';
			bFoundNum = true;
			++pParseStr;
		}	

		_pStr = pParseStr;

		if (bFoundNum)			
			return DestNumber * Sign;
		else
			return _FailValue;
	}


	template<class t_CData, class t_CReturn>
		DIdsPInlineL t_CReturn StrToIntBase10NoSign(const t_CData *_pStr, t_CReturn _FailValue)
	{
		t_CReturn DestNumber = 0;
		//	const t_CData *pParseStr = _pStr;

		// Parse for characters, and end if str terminator is found
		while ((*_pStr) >= '0' && (*_pStr) <= '9')
		{
			DestNumber *= 10;
			DestNumber += (*(_pStr++)) - '0';
		}	

		//	_pStr = pParseStr;

		return DestNumber;
	}


	template<class t_CData, class t_CReturn>
		DIdsPInlineL t_CReturn StrToIntBase10ParseNoSign(const t_CData *&_pStr, t_CReturn _FailValue)
	{
		t_CReturn DestNumber = 0;
		const t_CData *pParseStr = _pStr;

		// Parse for characters, and end if str terminator is found
		while ((*pParseStr) >= '0' && (*pParseStr) <= '9')
		{
			DestNumber *= 10;
			DestNumber += (*(pParseStr++)) - '0';
		}	

		_pStr = pParseStr;

		return DestNumber;
	}
}

/*
class CScriptTypeConvert
{
public:

	CScriptMultiType m_Combined;
	CStr m_TempStr;

	CScriptTypeConvert(const CScriptMultiType &_Src):
		m_Combined(_Src)
	{
	}

	template <typename t_CType>
	operator t_CType()
	{
		CThisTypeIsNotDefined Type; // You have to implement this type conversion
	}

	template <int _Len>
	operator typename TFStr<_Len>()
	{
		TFStr<_Len> String;
		String.Capture(m_Combined.SafeGetStr(m_TempStr));
		return String;
	}


};


template <>
M_FORCEINLINE CScriptTypeConvert::operator int8()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToInt(m_Combined.GetStr(), (int8)0);
	}
	else
		return m_Combined.SafeGetInteger();
}

template <>
M_FORCEINLINE CScriptTypeConvert::operator int()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToInt(m_Combined.GetStr(), (int)0);
	}
	else
		return m_Combined.SafeGetInteger();
}

template <>
M_FORCEINLINE CScriptTypeConvert::operator int16()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToInt(m_Combined.GetStr(), (int16)0);
	}
	else
		return m_Combined.SafeGetInteger();
}

template <>
M_FORCEINLINE CScriptTypeConvert::operator int32()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToInt(m_Combined.GetStr(), (int32)0);
	}
	else
		return m_Combined.SafeGetInteger();
}

template <>
M_FORCEINLINE CScriptTypeConvert::operator int64()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToInt(m_Combined.GetStr(), (int64)0);
	}
	else
		return m_Combined.SafeGetInteger();
}

template <>
M_FORCEINLINE CScriptTypeConvert::operator uint8()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToInt(m_Combined.GetStr(), (uint8)0);
	}
	else
		return m_Combined.SafeGetInteger();
}

template <>
M_FORCEINLINE CScriptTypeConvert::operator uint16()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToInt(m_Combined.GetStr(), (uint16)0);
	}
	else
		return m_Combined.SafeGetInteger();
}

template <>
M_FORCEINLINE CScriptTypeConvert::operator uint32()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToInt(m_Combined.GetStr(), (uint32)0);
	}
	else
		return m_Combined.SafeGetInteger();
}

template <>
M_FORCEINLINE CScriptTypeConvert::operator uint64()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToInt(m_Combined.GetStr(), (uint64)0);
	}
	else
		return m_Combined.SafeGetInteger();
}

template <>
M_FORCEINLINE CScriptTypeConvert::operator fp4()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToFloat(m_Combined.GetStr(), (fp4)0);
	}
	else
		return m_Combined.SafeGetFloat();
}

template <>
M_FORCEINLINE CScriptTypeConvert::operator fp8()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToFloat(m_Combined.GetStr(), (fp8)0);
	}
	else
		return m_Combined.SafeGetFloat();
}

template <>
M_FORCEINLINE CScriptTypeConvert::operator const char *()
{
	return m_Combined.SafeGetStr(m_TempStr);
}

template <>
M_FORCEINLINE CScriptTypeConvert::operator CStr &()
{
	m_TempStr = m_Combined.SafeGetStr(m_TempStr);
	return m_TempStr;
}

template <>
M_FORCEINLINE CScriptTypeConvert::operator CScriptMultiType &()
{
	return m_Combined;
}

template <typename t_CConvert>
void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, t_CConvert _Ret)
{
	CThisTypeIsNotDefined Type; // You have to implement this type conversion
}

template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, int _Ret);
template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, int8 _Ret);
template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, int16 _Ret);
template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, int32 _Ret);
template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, int64 _Ret);
template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, uint8 _Ret);
template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, uint16 _Ret);
template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, uint32 _Ret);
template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, uint64 _Ret);
template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, fp4 _Ret);
template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, fp8 _Ret);
template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, const char * _Ret);
template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, const CStr &_Ret);
template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, CStr _Ret);
template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, CScriptMultiType &_Ret);
*/

/*
template <typename t_CConvert>
class TCScriptTypeName
{
public:
	CThisTypeIsNotDefined Type; // You have to implement this type name
};


#define DScriptType(_Type) 
template <>
class TCScriptTypeName<_Type>
{
public:
	static const char *ms_pName;
};
template <>
const char *TCScriptTypeName<_Type>::ms_pName

template <> const char *ScriptTypeName<_Type>() { return #_Type;}
#define DScriptType2(_Type, _Name) template <> const char *ScriptTypeName<_Type>() { return #_Name;}
*/

template <typename t_CConvert>
static const char *ScriptTypeName()
{
	CGenerateCompileTimeError<int>::GenerateError(); // You have to implement this type name
	return "";
}

#define DScriptType(_Type) template <> M_INLINE static const char *ScriptTypeName<_Type>() { return #_Type;}
#define DScriptType2(_Type, _Name) template <> M_INLINE static const char *ScriptTypeName<_Type>() { return #_Name;}

DScriptType(void);
#ifdef M_SEPARATETYPE_int
DScriptType(int);
#endif
DScriptType(int8);
DScriptType(int16);
DScriptType(int32);
DScriptType(int64);
DScriptType(uint8);
DScriptType(uint16);
DScriptType(uint32);
DScriptType(uint64);
DScriptType(fp4);
DScriptType(fp8);
DScriptType2(const char *, str);
DScriptType(CStr);

#endif // DInc_MScript_Convert_h
