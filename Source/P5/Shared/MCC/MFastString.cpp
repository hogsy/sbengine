#include "MFastStr.h"

class CFStrCore
{
public:
	int m_Len;

	virtual int GetMax() const = 0;
	virtual char* GetStr() const = 0;

	CFStrCore()
	{
		Clear();
	}

	void Capture(const char* _pStr)
	{
		int i = 0;
		do
		{
			m_Str[i] = _pStr[i];
			i++;
			if (i >= GetMax())
			{
				m_Str[i-1] = 0;
				break;
			}
		}
		while(_pStr[i])
		m_Len = i;
	}

	void Capture(const char* _pStr, int _Len)
	{
		if (_Len > GetMax())
			_Len = GetMax();
		memcpy(GetStr(), _pStr, _Len);
		GetStr()[_Len] = 0;
		m_Len = _Len;
	}

	CFStrCore::CFStrCore(char ch)
	{
		m_Str[0] = ch;
		m_Len = 1;
	}

	CFStrCore::CFStrCore(char ch, int n)
	{
		n = Min(Max(0, n), GetMax());
		FillChar(&m_Str[0], n, ch);
		m_Len = n;
	};

	CFStrCore::CFStrCore(const char* x, ...)
	{
		Clear();
		if (!x) return;

		char buffer[1024];
		va_list arg;
		va_start(arg,x);
		vsprintf((char*) &buffer, x, arg); 
		Capture(buffer);
	};

	CFStrCore(const CStr& _s)
	{
		const char* pS = (char*) _s;
		if (!pS)
		{
			m_Len = 0;
			m_Str[0] = 0;
		}
		else
			Capture(pS);
	}

	void CFStrCore::operator=(const char* s)
	{
		Capture(s);
	}

	void CFStrCore::operator=(const CFStrCore& s)
	{
		Capture(s.GetStr(), s.m_Len);
	}

	char CFStrCore::operator[](int i) const
	{
		if ((i < 0) || (i >= m_Len)) return 0;
		return m_Str[i];
	};

	CFStrCore::operator char* () const
	{
		return &m_Str[0];
	};

	int CFStrCore::Val_int()
	{
		errno = 0;
		int res = strtoul(m_Str, NULL, 0);
		if (errno != 0) return 0;
		return res;
	};

	fp64 CFStrCore::Val_fp64()
	{
		errno = 0;
		fp64 res = strtod(m_Str, NULL);
		if (errno != 0) return 0;
		return res;
	};

	void CFStrCore::Clear()
	{
		m_Str[0] = 0;
		m_Len = 0;
	}

	CFStrCore& CFStrCore::operator+=(const char* x)
	{
		if (!x) return *this;
		int l = strlen(x);
		int nCopy = Min(GetMax(), l + m_Len) - m_Len;
		
		for(int i = 0; i < nCopy; i++)
			m_Str[i + m_Len] = x[i];

		m_Len += nCopy;
		m_Str[m_Len] = 0;
		return *this;
	}
}



