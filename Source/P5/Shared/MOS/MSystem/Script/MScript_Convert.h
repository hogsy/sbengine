#ifndef DInc_MScript_Convert_h
#define DInc_MScript_Convert_h


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
M_FORCEINLINE CScriptTypeConvert::operator fp32()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToFloat(m_Combined.GetStr(), (fp32)0);
	}
	else
		return m_Combined.SafeGetFloat();
}

template <>
M_FORCEINLINE CScriptTypeConvert::operator fp64()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToFloat(m_Combined.GetStr(), (fp64)0);
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
template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, fp32 _Ret);
template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, fp64 _Ret);
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
DScriptType(fp32);
DScriptType(fp64);
DScriptType2(const char *, str);
DScriptType(CStr);

#endif // DInc_MScript_Convert_h
