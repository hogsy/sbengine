
#ifndef DInc_MScript_MultiType_h
#define DInc_MScript_MultiType_h

/*
class CScriptMultiType
{
public:
	enum EType
	{
		EType_Invalid,
		EType_String,
		EType_Integer,
		EType_Float
	};

	int m_Type;

	union UData
	{
		const char *m_pString;
		int32 m_Integer;
		fp4 m_Float;
	};

	UData m_Data;

	M_INLINE CScriptMultiType()
	{
		m_Type = EType_Invalid;
	}

	M_INLINE CScriptMultiType(const CScriptMultiType &_Src)
	{
		m_Data = _Src.m_Data;
		m_Type = _Src.m_Type;
	}

	M_INLINE CScriptMultiType& operator =(const CScriptMultiType &_Src)
	{
		m_Data = _Src.m_Data;
		m_Type = _Src.m_Type;
		return *this;
	}

	M_INLINE CScriptMultiType(const char *_pStr)
	{
		m_Data.m_pString = _pStr;
		m_Type = EType_String;
	}

	M_INLINE CScriptMultiType(int32 _Value)
	{
		m_Data.m_Integer = _Value;
		m_Type = EType_Integer;
	}

	M_INLINE CScriptMultiType(fp4 _Value)
	{
		m_Data.m_Float = _Value;
		m_Type = EType_Float;
	}	

	M_INLINE CScriptMultiType (int _Value)
	{
		m_Data.m_Integer = _Value;
		m_Type = EType_Integer;
	}	

	M_INLINE CScriptMultiType & operator =(const char *_pStr)
	{
		m_Data.m_pString = _pStr;
		m_Type = EType_String;
		return *this;
	}

	M_INLINE CScriptMultiType & operator =(int32 _Value)
	{
		m_Data.m_Integer = _Value;
		m_Type = EType_Integer;
		return *this;
	}

	M_INLINE CScriptMultiType & operator =(fp4 _Value)
	{
		m_Data.m_Float = _Value;
		m_Type = EType_Float;
		return *this;
	}	

	M_INLINE CScriptMultiType & operator =(int _Value)
	{
		m_Data.m_Integer = _Value;
		m_Type = EType_Integer;
		return *this;
	}	

	M_INLINE bool IsInteger()
	{
		return m_Type == EType_Integer;
	}

	M_INLINE bool IsFloat()
	{
		return m_Type == EType_Float;
	}

	M_INLINE bool IsString()
	{
		return m_Type == EType_String;
	}

	M_INLINE const char *GetStr()
	{
		M_ASSERT(m_Type == EType_String, "");
		return m_Data.m_pString;
	}
	
	M_INLINE int32 GetInteger()
	{
		M_ASSERT(m_Type == EType_Integer, "");
		return m_Data.m_Integer;
	}

	M_INLINE fp4 GetFloat()
	{
		M_ASSERT(m_Type == EType_Float, "");
		return m_Data.m_Float;
	}

	const char *SafeGetStr(CStr &_TempStr);
	int32 SafeGetInteger();
	fp4 SafeGetFloat();
	
};
*/

#endif // DInc_MScript_MultiType_h
