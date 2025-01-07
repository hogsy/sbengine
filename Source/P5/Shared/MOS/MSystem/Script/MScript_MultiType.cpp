
#include "PCH.h"
#include "MScript.h"

#if DScriptDebug>0
#pragma  optimize("",off)
#pragma  inline_depth(0)
#endif
/*
const char *CScriptMultiType::SafeGetStr(CStr &_TempStr)
{
	switch (m_Type)
	{
	case EType_String:
		{
			return m_Data.m_pString;
		}
	case EType_Integer:
		{
			_TempStr.CaptureFormated("%d", m_Data.m_Integer);
			return _TempStr.Str();
		}
	case EType_Float:
		{
			_TempStr.CaptureFormated("%f", m_Data.m_Float);
			return _TempStr.Str();
		}

	};

	return "";
}

int32 CScriptMultiType::SafeGetInteger()
{
	switch (m_Type)
	{
	case EType_String:
		{
			return NStr::StrToInt(m_Data.m_pString, (int32)0);
		}
	case EType_Integer:
		{
			return m_Data.m_Integer;
		}
	case EType_Float:
		{
			return m_Data.m_Float;
		}

	};

	return 0;
}

fp32 CScriptMultiType::SafeGetFloat()
{
	switch (m_Type)
	{
	case EType_String:
		{
			return NStr::StrToFloat(m_Data.m_pString, (fp32)0);
		}
	case EType_Integer:
		{
			return m_Data.m_Integer;
		}
	case EType_Float:
		{
			return m_Data.m_Float;
		}

	};

	return 0;
}
*/

