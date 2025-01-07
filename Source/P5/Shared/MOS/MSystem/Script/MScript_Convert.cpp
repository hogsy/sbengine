#include "PCH.h"

#include "MScript.h"
#if DScriptDebug>0
#pragma  optimize("",off)
#pragma  inline_depth(0)
#endif

/*
template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, int _Ret)
{
	_SRet = _Ret;
}

template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, int8 _Ret)
{
	_SRet = _Ret;
}

template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, int16 _Ret)
{
	_SRet = _Ret;
}

template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, int32 _Ret)
{
	_SRet = _Ret;
}

template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, int64 _Ret)
{
	_SRet = (int32)_Ret;
}

template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, uint8 _Ret)
{
	_SRet = _Ret;
}

template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, uint16 _Ret)
{
	_SRet = _Ret;
}

template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, uint32 _Ret)
{
	_SRet = (int32)_Ret;
}

template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, uint64 _Ret)
{
	_SRet = (int32)_Ret;
}

template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, fp4 _Ret)
{
	_SRet = _Ret;
}


template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, fp8 _Ret)
{
	_SRet = (fp4)_Ret;
}

template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, const char * _Ret)
{
	_TempStr = _Ret;
	_SRet = _TempStr;
}

template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, const CStr &_Ret)
{
	_SRet = _Ret;
}

template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, CStr _Ret)
{
	_SRet = _Ret;
}

template <> void ScriptReturnConvert(CScriptMultiType &_SRet, CStr &_TempStr, CScriptMultiType &_Ret)
{
	_SRet = _Ret;
}
*/
/*
template <>
CScriptTypeConvert::operator <int8>()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToInt(m_Combined.GetStr(), (int8)0);
	}
	else
		return m_Combined.SafeGetInteger();
}

template <>
CScriptTypeConvert::operator int16()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToInt(m_Combined.GetStr(), (int16)0);
	}
	else
		return m_Combined.SafeGetInteger();
}

template <>
CScriptTypeConvert::operator int32()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToInt(m_Combined.GetStr(), (int32)0);
	}
	else
		return m_Combined.SafeGetInteger();
}

template <>
CScriptTypeConvert::operator int64()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToInt(m_Combined.GetStr(), (int64)0);
	}
	else
		return m_Combined.SafeGetInteger();
}

template <>
CScriptTypeConvert::operator uint8()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToInt(m_Combined.GetStr(), (uint8)0);
	}
	else
		return m_Combined.SafeGetInteger();
}

template <>
CScriptTypeConvert::operator uint16()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToInt(m_Combined.GetStr(), (uint16)0);
	}
	else
		return m_Combined.SafeGetInteger();
}

template <>
CScriptTypeConvert::operator uint32()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToInt(m_Combined.GetStr(), (uint32)0);
	}
	else
		return m_Combined.SafeGetInteger();
}

template <>
CScriptTypeConvert::operator uint64()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToInt(m_Combined.GetStr(), (uint64)0);
	}
	else
		return m_Combined.SafeGetInteger();
}

template <>
CScriptTypeConvert::operator fp4()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToFloat(m_Combined.GetStr(), (fp4)0);
	}
	else
		return m_Combined.SafeGetFloat();
}

template <>
CScriptTypeConvert::operator fp8()
{
	if (m_Combined.IsString())
	{
		return NStr::StrToFloat(m_Combined.GetStr(), (fp8)0);
	}
	else
		return m_Combined.SafeGetFloat();
}

template <>
CScriptTypeConvert::operator const char *()
{
	return m_Combined.SafeGetStr(m_TempStr);
}

template <>
CScriptTypeConvert::operator CStr &()
{
	m_TempStr = m_Combined.SafeGetStr(m_TempStr);
	return m_TempStr;
}
*/


