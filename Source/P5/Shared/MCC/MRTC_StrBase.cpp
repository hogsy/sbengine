#if defined(PLATFORM_DOLPHIN) && defined(COMPILER_CODEWARRIOR)
# include <cstdarg>
//using namespace std;
using std::wcstoul;
using std::wcstod;
using std::wcscmp;
using std::wcsstr;
using std::wcslen;
using std::wcscpy;
#endif

#include <errno.h>

// -------------------------------------------------------------------
//  CStrBase
// -------------------------------------------------------------------
const char CStrBase::ms_CitationMark[] = { 34, 0 };
const char CStrBase::ms_WhiteSpaces[] = { 8, 9, 10, 13, 32, 0 };

void CStrBase::mfscpy(void* _pDst, int _DstFmt, const void* _pSrc, int _SrcFmt)
{
	MAUTOSTRIP(CStrBase_mfscpy, MAUTOSTRIP_VOID);
	int i = 0;
	while(true)
	{
		int ch = 0;
		if (_SrcFmt == CSTR_FMT_ANSI)
			ch = ((unsigned char*)_pSrc)[i];
		else if (_SrcFmt == CSTR_FMT_UNICODE)
			ch = ((wchar*)_pSrc)[i];

		if (_DstFmt == CSTR_FMT_ANSI)
			((char*)_pDst)[i] = (ch > 255) ? '¦' : ch;
		else if (_DstFmt == CSTR_FMT_UNICODE)
			((wchar*)_pDst)[i] = ch;

		if (!ch) return;
		i++;
	}
}

void CStrBase::mfsncpy(void* _pDst, int _DstFmt, const void* _pSrc, int _SrcFmt, int _Count)
{
	MAUTOSTRIP(CStrBase_mfsncpy, MAUTOSTRIP_VOID);
	for(int i = 0; i < _Count; i++)
	{
		int ch = 0;
		if (_SrcFmt == CSTR_FMT_ANSI)
			ch = ((unsigned char*)_pSrc)[i];
		else if (_SrcFmt == CSTR_FMT_UNICODE)
			ch = ((wchar*)_pSrc)[i];

		if (_DstFmt == CSTR_FMT_ANSI)
			((unsigned char*)_pDst)[i] = (ch > 255) ? '¦' : ch;
		else if (_DstFmt == CSTR_FMT_UNICODE)
			((wchar*)_pDst)[i] = ch;
	}
}

char CStrBase::clwr(char _c)
{
	MAUTOSTRIP(CStrBase_clwr, 0);
	uint8 c = _c;
	if (c >= 'A' && c <= 'Z')
		return c + 'a' - 'A';
	if (c >= 0xc0 && c <= 0xde)
		return c + 0xe0 - 0xc0;
	return c;
}

char CStrBase::cupr(char _c)
{
	MAUTOSTRIP(CStrBase_cupr, 0);
	uint8 c = _c;
	if (c >= 'a' && c <= 'z')
		return c + 'A' - 'a';
	if (c >= 0xe0 && c <= 0xfe)
		return c + 0xc0 - 0xe0;
	return c;
}

wchar CStrBase::wclwr(wchar _c)
{
	MAUTOSTRIP(CStrBase_wclwr, 0);
	if (_c < 256)
		return (unsigned char)clwr((char)_c);
	else
		return _c;
}

wchar CStrBase::wcupr(wchar _c)
{
	MAUTOSTRIP(CStrBase_wcupr, 0);
	if (_c < 256)
		return (unsigned char)cupr((char)_c);
	else
		return _c;
}

void CStrBase::strupr(char* _pStr)
{
	MAUTOSTRIP(CStrBase_strupr, MAUTOSTRIP_VOID);
#ifdef PLATFORM_WIN_PC
	_strupr(_pStr);
#else
	while(*_pStr)
	{
		*_pStr = cupr(*_pStr);
		_pStr++;
	}
#endif
}

void CStrBase::strlwr(char* _pStr)
{
	MAUTOSTRIP(CStrBase_strlwr, MAUTOSTRIP_VOID);
	while(*_pStr)
	{
		*_pStr = clwr(*_pStr);
		_pStr++;
	}
}

void CStrBase::wcsupr(wchar* _pStr)
{
	MAUTOSTRIP(CStrBase_wcsupr, MAUTOSTRIP_VOID);
	while(*_pStr)
	{
		*_pStr = wcupr(*_pStr);
		_pStr++;
	}
}

void CStrBase::wcslwr(wchar* _pStr)
{
	MAUTOSTRIP(CStrBase_wcslwr, MAUTOSTRIP_VOID);
	while(*_pStr)
	{
		*_pStr = wclwr(*_pStr);
		_pStr++;
	}
}

int CStrBase::stricmp(const char* _s0, const char* _s1)
{
	MAUTOSTRIP(CStrBase_stricmp, 0);
	while(*_s0 && *_s1)
	{
		unsigned char c0 = cupr(*_s0++);
		unsigned char c1 = cupr(*_s1++);
		if (c0 > c1)
			return 1;
		else if (c0 < c1)
			return -1;
	}

	if (!*_s0 && !*_s1)
		return 0;
	if (!*_s0)
		return -1;
	else if (!*_s1)
		return 1;

	return 0;
}

int CStrBase::strnicmp(const char* _s0, const char* _s1, mint _MaxLen)
{
	MAUTOSTRIP(CStrBase_stricmp, 0);
	while(*_s0 && *_s1 && _MaxLen-- > 0)
	{
		unsigned char c0 = cupr(*_s0++);
		unsigned char c1 = cupr(*_s1++);
		if (c0 > c1)
			return 1;
		else if (c0 < c1)
			return -1;
	}
	if(_MaxLen == 0)
		return 0;

	if (!*_s0 && !*_s1)
		return 0;
	if (!*_s0)
		return -1;
	else if (!*_s1)
		return 1;

	return 0;
}

int CStrBase::wcsicmp(const wchar* _s0, const wchar* _s1)
{
	MAUTOSTRIP(CStrBase_wcsicmp, 0);
	while(*_s0 && *_s1)
	{
		wchar c0 = wcupr(*_s0++);
		wchar c1 = wcupr(*_s1++);
		if (c0 > c1)
			return 1;
		else if (c0 < c1)
			return -1;
	}

	if (!*_s0 && !*_s1)
		return 0;
	if (!*_s0)
		return -1;
	else if (!*_s1)
		return 1;

	return 0;
}

const wchar *CStrBase::wcsstr( const wchar *_s0, const wchar *_s1 )
{
	MAUTOSTRIP(CStrBase_wcsstr, NULL);
	const wchar *_s = _s1, *_c = _s0, *_f = _s0;
	while(*_c && *_s)
	{
		wchar c0 = wcupr(*_c++);
		wchar c1 = wcupr(*_s);
		if (c0 == c1)
		{
			_s++;
		}
		else
		{
			_s	= _s1;
			_f	= _c;
		}
	}

	if( !*_c)
		return NULL;

	if (!*_s)
		return _f;

	return NULL;
}

int CStrBase::wcsncmp( const wchar *_s0, const wchar *_s1, int _num )
{
	MAUTOSTRIP(CStrBase_wcsncmp, 0);

	while( *_s0 && *_s1 && _num-- )
	{
		wchar s0 = *_s0;
		wchar s1 = *_s1;
		wchar t = s0 - s1;
		if( t )
		{
			if( s1 > s0 )
				return -1;
			else
				return 1;
		}
		_s0++;
		_s1++;
	}
	return 0;
}

unsigned long CStrBase::wcstoul(const wchar *_s0, wchar **_endp, int _base)
{
	MAUTOSTRIP(CStrBase_wcstoul, 0);
	// ToDo: Use _base

	unsigned long value = 0;

	const wchar *zero = WTEXT("0");
	const wchar *nine = WTEXT("9");

	while( *_s0 )
	{
		if( ( *_s0 < *zero ) || ( *_s0 > *nine ) )
		{
			break;
		}

		value *= 10;
		value += *_s0 - *zero;
		_s0++;
	}

	if (_endp)
		*_endp = (wchar*)_s0;

	return value;
}

double CStrBase::wcstod(const wchar *_s0, wchar **_endp)
{
	MAUTOSTRIP(CStrBase_wcstod, 0.0);
#ifdef PLATFORM_WIN32
	return ::wcstod( (const wchar_t*)_s0, (wchar_t**)_endp );
#elif !defined(COMPILER_GNU)
	return ::wcstod( _s0, _endp );
#else
	char *pBuf = DNew(char) char[wcslen(_s0) + 1], *p;
	mfscpy(pBuf, CSTR_FMT_ANSI, _s0, CSTR_FMT_UNICODE);
	double Ret = strtod(pBuf, &p);
	if(_endp)
		*_endp = (wchar *)_s0 + (p - pBuf);
	delete[] pBuf;
	return Ret;
#endif
}

int CStrBase::wcscmp(const wchar *_s0, const wchar *_s1)
{
	MAUTOSTRIP(CStrBase_wcscmp, 0);
	while( *_s0 && *_s1 )
	{
		if( *_s0 != *_s1 )
		{
			return ((*_s0 - *_s1)<0)?-1:1;
		}
		_s0++;
		_s1++;
	}

	if( !*_s0 && *_s1)
		return -1;
	else if( !*_s1 && *_s0)
		return 1;
	return 0;
}

int CStrBase::wcslen(const wchar *_s0)
{
	MAUTOSTRIP(CStrBase_wcslen, 0);
	int len;

	if( !_s0 )
		return 0;

//	if( !*_s0 )	// This should never be allowed to happend!!! wchar strings should always have their damn tag!
//		return 0;

	for( len = 0; _s0[len]; len++ )
	{
	}
	
	return len;	// Remember to remove the 0xFFFE tag :p
}

void CStrBase::wcscpy(wchar *_dest, const wchar *_src)
{
	MAUTOSTRIP(CStrBase_wcscpy, MAUTOSTRIP_VOID);
	while( *_src )
	{
		*_dest	= *_src;
		_dest++;
		_src++;
	}
	*_dest	= 0;
}

////////////////////////////////////////////////////////
// HACK. vswprintf version that works on PS2
// parses the string for % and does a unicode
// string copy in case of %s
// Not thoroughly tested!
////////////////////////////////////////////////////////

//--- Returns true on special format flags
bool isOptionalFormatSpecifier(int c);
bool isOptionalFormatSpecifier(int c)
{
	MAUTOSTRIP(isOptionalFormatSpecifier, false);
	return c == '+' || c == '-' || c == ' ' || c == '#' || c >= '0' && c <= '9' || c == '.' || c == 'l';
}

//--- Returns true on "normal" specifiers
bool isFormatSpecifier(int c);
bool isFormatSpecifier(int c)
{
	MAUTOSTRIP(isFormatSpecifier, false);
	return	c >= 'c' && c <= 'g' ||	// c, d, e, f, g
			c == 'i'			 ||
			c >= 'n' && c <= 'p' ||	// n, o, p
			c == 's' || c == 'u' ||
			c == 'x' || c == 'C' ||
			c == 'E' || c == 'G' ||
			c == 'S' || c == 'X';
}

//--- Copy parts of the source string to a buffer
bool CopyPart(wchar *&_pBuffer, int _BufSize, int _Count, int _Size, int &nWritten, const void *_pSource);
bool CopyPart(wchar *&_pBuffer, int _BufSize, int _Count, int _Size, int &nWritten, const void *_pSource)
{
	MAUTOSTRIP(CopyPart, false);
	if(_Count <= 0)
		return true;

	bool bRet = true;
	nWritten += _Count;
	if(nWritten >= _BufSize)
	{
		bRet = false;
		_Count -= nWritten - _BufSize;
	}
	if(_Size == sizeof(char))
		CStrBase::mfsncpy(_pBuffer, CSTR_FMT_UNICODE, _pSource, CSTR_FMT_ANSI, _Count);
	else
		CStrBase::mfsncpy(_pBuffer, CSTR_FMT_UNICODE, _pSource, CSTR_FMT_UNICODE, _Count);
	_pBuffer += _Count;
	return bRet;
}

//--- vswprintf implementation
mint CStrBase::vswprintf(wchar *_pBuffer, int _BufSize, const wchar *_pStr, va_list _arg)
{
	MAUTOSTRIP(CStrBase_vswprintf, 0);
	char *pBuf = DNew(char) char[_BufSize];
	int nWritten = 0;
	int StrLen = wcslen(_pStr);
	const wchar *pStart = _pStr;
	while(nWritten < _BufSize && StrLen > 0)
	{
		while(StrLen-- && *_pStr++ != '%')
		{
		}
		// Copy string up to % or end
		if(!CopyPart(_pBuffer, _BufSize, _pStr - pStart - ((_pStr[-1]=='%')?1:0), sizeof(wchar), nWritten, pStart))
			break;
		if(StrLen < 0)
			break;
		// Did we find a special char?
		if(_pStr[-1] == '%')
		{
			int nBuf;
			int iType = 0;
			while(isOptionalFormatSpecifier(_pStr[iType])) iType++;
			if(isFormatSpecifier(_pStr[iType]) == false || iType > 20)	// break on invalid format or ridiculously long format
			{
				if(!CopyPart(_pBuffer, _BufSize, iType + 1, sizeof(wchar), nWritten, _pStr))
					break;
			}
			else if(_pStr[iType] != 's')								// sprintf the format alone, uness it's a %s
			{
				char pFormat[25] = "%";
				int i;
				for(i = 0; i <= iType; i++)
					pFormat[i+1] = _pStr[i];
				pFormat[i+1] = 0;
				if(_pStr[iType] == 'e' || _pStr[iType] == 'f' || _pStr[iType] == 'g' || _pStr[iType] == 'G')
				{
					nBuf = sprintf(pBuf, pFormat, *(double *)_arg);
					va_arg(_arg, double);
				}
				else
				{
					nBuf = sprintf(pBuf, pFormat, *(int *)_arg);
					va_arg(_arg, int);
				}
				if(!CopyPart(_pBuffer, _BufSize, nBuf, sizeof(char), nWritten, pBuf))
					break;
			}
			else														// It's a %s, unicode copy the string manually
			{
				if(!CopyPart(_pBuffer, _BufSize, wcslen(*(wchar **)_arg), sizeof(wchar), nWritten, *(int **)_arg))
					break;
				va_arg(_arg, int);
			}
			_pStr += iType + 1;		// skip format type
		}
		pStart = _pStr;
	}
	if(nWritten < _BufSize)
		*_pBuffer = 0;
	delete[] pBuf;
	return nWritten;
}

/*
int CStrBase::swscanf( const wchar *_src, const wchar *_format, ... )
{
	MAUTOSTRIP(CStrBase_swscanf, 0);
#ifndef PLATFORM_PS2
	return ::swscanf( _src, _format, ... );
#else
	Error_static( "CStrBase::swscanf", "Implement this" );
	return 0;
#endif
}
*/
char* CStrBase::GetStr()
{
	MAUTOSTRIP(CStrBase_GetStr, NULL);
	Error("GetStr", "Invalid call.");
	return NULL;
};

const char* CStrBase::GetStr() const
{
	MAUTOSTRIP(CStrBase_GetStr_2, NULL);
	Error("GetStr", "Invalid call.");
	return NULL;
};

const char* CStrBase::Str() const
{
	MAUTOSTRIP(CStrBase_Str, NULL);
	Error("Str", "Invalid call.");
	return NULL;
};

const void* CStrBase::StrData() const
{
	Error("GetStrW", "Invalid call.");
	return NULL;
}

wchar* CStrBase::GetStrW()
{
	MAUTOSTRIP(CStrBase_GetStrW, NULL);
	Error("GetStrW", "Invalid call.");
	return NULL;
};

const wchar* CStrBase::GetStrW() const
{
	MAUTOSTRIP(CStrBase_GetStrW_2, NULL);
	Error("GetStrW", "Invalid call.");
	return NULL;
};

const wchar* CStrBase::StrW() const
{
	MAUTOSTRIP(CStrBase_StrW, NULL);
	Error("StrW", "Invalid call.");
	return NULL;
};

int CStrBase::GetFmt() const
{
	MAUTOSTRIP(CStrBase_GetFmt, 0);
	if (IsUnicode())
		return CSTR_FMT_UNICODE;
	else
		return CSTR_FMT_ANSI;
}

int CStrBase::IsAnsi() const
{
	MAUTOSTRIP(CStrBase_IsAnsi, 0);
	return true;
}

int CStrBase::IsUnicode() const
{
	MAUTOSTRIP(CStrBase_IsUnicode, 0);
	return false;
}

int CStrBase::IsPureAnsi() const
{
	MAUTOSTRIP(CStrBase_IsPureAnsi, 0);
	return true;
}

// -------------------------------------------------------------------
int CStrBase::Val_int(const char* _p, int& _Value)
{
	MAUTOSTRIP(CStrBase_Val_int, 0);
	char* pStop = NULL;
	errno = 0;
	_Value = strtoul(_p, &pStop, 0);
	if (errno != 0) _Value = 0;
	return (pStop) ? pStop - _p : 0;
}

int CStrBase::Val_fp8(const char* _p, fp8& _Value)
{
	MAUTOSTRIP(CStrBase_Val_fp8, 0);
	char* pStop = NULL;
	errno = 0;
	_Value = strtod(_p, &pStop);
	if (errno != 0) _Value = 0;
	return (pStop) ? pStop-_p : 0;
}

int CStrBase::Val_int(const wchar* _p, int& _Value)
{
	MAUTOSTRIP(CStrBase_Val_int_2, 0);
	wchar* pStop = NULL;
	errno = 0;
	_Value = wcstoul(_p, &pStop, 0);
	if (errno != 0) _Value = 0;
	return (pStop) ? pStop-_p : 0;
}

int CStrBase::Val_fp8(const wchar* _p, fp8& _Value)
{
	MAUTOSTRIP(CStrBase_Val_fp8_2, 0);
	wchar* pStop = NULL;
	errno = 0;
	_Value = wcstod(_p, &pStop);
	if (errno != 0) _Value = 0;
	return (pStop) ? pStop-_p : 0;
}

int CStrBase::Val_int(const void* _p, int& _Value, int _Fmt)
{
	MAUTOSTRIP(CStrBase_Val_int_3, 0);
	if (_Fmt == CSTR_FMT_ANSI)
		return Val_int((const char*)_p, _Value);
	else if (_Fmt == CSTR_FMT_UNICODE)
		return Val_int((const wchar*)_p, _Value);
	else
		return 0;
}

int CStrBase::Val_fp8(const void* _p, fp8& _Value, int _Fmt)
{
	MAUTOSTRIP(CStrBase_Val_fp8_3, 0);
	if (_Fmt == CSTR_FMT_ANSI)
		return Val_fp8((const char*)_p, _Value);
	else if (_Fmt == CSTR_FMT_UNICODE)
		return Val_fp8((const wchar*)_p, _Value);
	else
		return 0;
}

int CStrBase::Val_int() const
{
	MAUTOSTRIP(CStrBase_Val_int_4, 0);
	if (IsAnsi())
	{
		const char* p = GetStr();
		if (!p) return 0;
		errno = 0;
		int res = strtoul(p, NULL, 0);
		if (errno != 0) return 0;
		return res;
	}
	else
	{
		const wchar* p = GetStrW();
		if (!p) return 0;
		errno = 0;
		int res = wcstoul(p, NULL, 0);
		if (errno != 0) return 0;
		return res;
	}
};

fp8 CStrBase::Val_fp8() const
{
	MAUTOSTRIP(CStrBase_Val_fp8_4, 0.0);
	if (IsAnsi())
	{
		const char* p = GetStr();
		if (!p) return 0;
		errno = 0;
		fp8 res = strtod(p, NULL);
		if (errno != 0) return 0;
		return res;
	}
	else
	{
		const wchar* p = GetStrW();
		if (!p) return 0;
		errno = 0;
		fp8 res = wcstod(p, NULL);
		if (errno != 0) return 0;
		return res;
	}
};

// -------------------------------------------------------------------
#define CSTR_COMPARE_NULL_AND_ZERO_LEN(_s0, _s1) \
{ \
	if (!(_s0) && !(_s1)) return 0; \
	if (!(_s0)) \
	{ \
		if((_s1)[0] == 0) \
			return 0; \
		else \
			return -1; \
	} \
	if (!(_s1)) \
	{ \
		if((_s0)[0] == 0) \
			return 0; \
		else \
			return 1; \
	}\
}

// -------------------------------------------------------------------
int CStrBase::Compare(const char* _s0, const char* _s1)
{
	MAUTOSTRIP(CStrBase_Compare, 0);
	CSTR_COMPARE_NULL_AND_ZERO_LEN(_s0, _s1);
	int Res = strcmp(_s0, _s1);
	if (Res < 0)
		Res = -1;
	else if (Res > 0)
		Res = 1;
	return Res;
}

int CStrBase::Compare(const wchar* _s0, const char* _s1)
{
	MAUTOSTRIP(CStrBase_Compare_2, 0);
	CSTR_COMPARE_NULL_AND_ZERO_LEN(_s0, _s1);

	int i = 0;
	while(1)
	{
		if ((uint32)_s0[i] < (uint32)_s1[i])
			return -1;
		else if ((uint32)_s0[i] > (uint32)_s1[i])
			return 1;

		if (!_s0[i]) break;
		i++;
	}
	return 0;
}

int CStrBase::Compare(const char* _s0, const wchar* _s1)
{
	MAUTOSTRIP(CStrBase_Compare_3, 0);

	CSTR_COMPARE_NULL_AND_ZERO_LEN(_s0, _s1);

	int i = 0;
	while(1)
	{
		if ((uint32)_s0[i] < (uint32)_s1[i])
			return -1;
		else if ((uint32)_s0[i] > (uint32)_s1[i])
			return 1;

		if (!_s0[i]) break;
		i++;
	}
	return 0;
}

int CStrBase::Compare(const wchar* _s0, const wchar* _s1)
{
	MAUTOSTRIP(CStrBase_Compare_4, 0);
	
	CSTR_COMPARE_NULL_AND_ZERO_LEN(_s0, _s1);

	int Res = wcscmp(_s0, _s1);
	if (Res < 0)
		Res = -1;
	else if (Res > 0)
		Res = 1;
	return Res;
}


int CStrBase::Compare(const CStrBase& _s0, const char* _s1)
{
	MAUTOSTRIP(CStrBase_Compare_5, 0);
	if (_s0.IsAnsi())
		return Compare(_s0.GetStr(), _s1);
	else
		return Compare(_s0.GetStrW(), _s1);
}

int CStrBase::Compare(const CStrBase& _s0, const wchar* _s1)
{
	MAUTOSTRIP(CStrBase_Compare_6, 0);
	if (_s0.IsAnsi())
		return Compare(_s0.GetStr(), _s1);
	else
		return Compare(_s0.GetStrW(), _s1);
}

int CStrBase::Compare(const CStrBase& _s0, const CStrBase& _s1)
{
	MAUTOSTRIP(CStrBase_Compare_7, 0);
	if (_s0.IsAnsi())
	{
		if (_s1.IsAnsi())
			return Compare(_s0.GetStr(), _s1.GetStr());
		else
			return Compare(_s0.GetStr(), _s1.GetStrW());
	}
	else
	{
		if (_s1.IsAnsi())
			return Compare(_s0.GetStrW(), _s1.GetStr());
		else
			return Compare(_s0.GetStrW(), _s1.GetStrW());
	}
}

// -------------------------------------------------------------------
int CStrBase::CompareNoCase(const char* _s0, const char* _s1)
{
	MAUTOSTRIP(CStrBase_CompareNoCase, 0);
	if (!_s0 && !_s1) return 0;
	if (!_s0)
	{
		if(_s1[0] == 0)
			return 0;
		else
			return -1;
	}
	if (!_s1)
	{
		if(_s0[0] == 0)
			return 0;
		else
			return 1;
	}
	return stricmp(_s0, _s1);
}

int CStrBase::CompareNoCase(const wchar* _s0, const char* _s1)
{
	MAUTOSTRIP(CStrBase_CompareNoCase_2, 0);
	wchar Str1[1024];
	int l = StrLen(_s1);
	if (l > 1023) Error_static("CStrBase::CompareNoCase", "Max compare length is 1023.");
	mfsncpy(Str1, CSTR_FMT_UNICODE, _s1, CSTR_FMT_ANSI, l);
	Str1[l] = 0;

	return CompareNoCase(_s0, Str1);
}

int CStrBase::CompareNoCase(const char* _s0, const wchar* _s1)
{
	MAUTOSTRIP(CStrBase_CompareNoCase_3, 0);
	wchar Str0[1024];
	int l = StrLen(_s0);
	if (l > 1023) Error_static("CStrBase::CompareNoCase", "Max compare length is 1023.");
	mfsncpy(Str0, CSTR_FMT_UNICODE, _s0, CSTR_FMT_ANSI, l);
	Str0[l] = 0;

	return CompareNoCase(Str0, _s1);
}

int CStrBase::CompareNoCase(const wchar* _s0, const wchar* _s1)
{
	MAUTOSTRIP(CStrBase_CompareNoCase_4, 0);
	if (!_s0 && !_s1) return 0;
	if (!_s0)
	{
		if(_s1[0] == 0)
			return 0;
		else
			return -1;
	}
	if (!_s1)
	{
		if(_s0[0] == 0)
			return 0;
		else
			return 1;
	}
	return wcsicmp(_s0, _s1);
}

int CStrBase::CompareNoCase(const CStrBase& _s0, const char* _s1)
{
	MAUTOSTRIP(CStrBase_CompareNoCase_5, 0);
	if (_s0.IsAnsi())
		return CompareNoCase(_s0.GetStr(), _s1);
	else
		return CompareNoCase(_s0.GetStrW(), _s1);
}

int CStrBase::CompareNoCase(const CStrBase& _s0, const wchar* _s1)
{
	MAUTOSTRIP(CStrBase_CompareNoCase_6, 0);
	if (_s0.IsAnsi())
		return CompareNoCase(_s0.GetStr(), _s1);
	else
		return CompareNoCase(_s0.GetStrW(), _s1);
}

int CStrBase::CompareNoCase(const CStrBase& _s0, const CStrBase& _s1)
{
	MAUTOSTRIP(CStrBase_CompareNoCase_7, 0);
	if (_s0.IsAnsi())
	{
		if (_s1.IsAnsi())
			return CompareNoCase(_s0.GetStr(), _s1.GetStr());
		else
			return CompareNoCase(_s0.GetStr(), _s1.GetStrW());
	}
	else
	{
		if (_s1.IsAnsi())
			return CompareNoCase(_s0.GetStrW(), _s1.GetStr());
		else
			return CompareNoCase(_s0.GetStrW(), _s1.GetStrW());
	}
}

// -------------------------------------------------------------------
int CStrBase::Compare(const char* _pStr) const
{
	MAUTOSTRIP(CStrBase_Compare_8, 0);
	return Compare(*this, _pStr);
}

int CStrBase::Compare(const wchar* _pStr) const
{
	MAUTOSTRIP(CStrBase_Compare_9, 0);
	return Compare(*this, _pStr);
}

int CStrBase::Compare(const CStrBase& _s) const
{
	MAUTOSTRIP(CStrBase_Compare_10, 0);
	return Compare(_s.GetStr());
}

int CStrBase::CompareNoCase(const char* _pStr) const
{
	MAUTOSTRIP(CStrBase_CompareNoCase_8, 0);
	return CompareNoCase(*this, _pStr);
}

int CStrBase::CompareNoCase(const wchar* _pStr) const
{
	MAUTOSTRIP(CStrBase_CompareNoCase_9, 0);
	return CompareNoCase(*this, _pStr);
}

int CStrBase::CompareNoCase(const CStrBase& _s) const
{
	MAUTOSTRIP(CStrBase_CompareNoCase_10, 0);
	return CompareNoCase(*this, _s);
}

// -------------------------------------------------------------------
int operator==(const CStrBase &x, const char* s)
{ 
	MAUTOSTRIP(operator_eq, 0);
	return CStrBase::Compare(x, s) == 0;
}

int operator!=(const CStrBase &x, const char* s)
{
	MAUTOSTRIP(operator_noteq, 0);
	return CStrBase::Compare(x, s) != 0;
}

int operator==(const CStrBase &x, const CStrBase& y)
{
	MAUTOSTRIP(operator_eq_2, 0);
	return CStrBase::Compare(x, y) == 0;
}

int operator!=(const CStrBase &x, const CStrBase& y)
{
	MAUTOSTRIP(operator_noteq_2, 0);
	return CStrBase::Compare(x, y) != 0;
}

int operator<(const CStrBase& x, const CStrBase& y)
{
	MAUTOSTRIP(operator_lt, 0);
	return CStrBase::Compare(x, y) == -1;
}

int operator>(const CStrBase& x, const CStrBase& y)
{
	MAUTOSTRIP(operator_gt, 0);
	return CStrBase::Compare(x, y) == 1;
}

int operator<=(const CStrBase& x, const CStrBase& y)
{
	MAUTOSTRIP(operator_lteq, 0);
	return CStrBase::Compare(x, y) <= 0;
}

int operator>=(const CStrBase& x, const CStrBase& y)
{
	MAUTOSTRIP(operator_gteq, 0);
	return CStrBase::Compare(x, y) >= 0;
}

// -------------------------------------------------------------------
int CStrBase::GetNumMatches(char ch) const
{
	MAUTOSTRIP(CStrBase_GetNumMatches, 0);
	if (IsAnsi())
	{
		const char* p = GetStr();
		if (!p) return 0;
		int c = 0; int i = 0;
		while(p[i]) if (p[i++] == ch) c++;
		return c;
	}
	else if (IsUnicode())
	{
		const wchar* p = GetStrW();
		if (!p) return 0;
		int c = 0; int i = 0;
		while(p[i]) if ((uint32)p[i++] == (uint32)ch) c++;
		return c;
	}
	else
		return 0;
}

int CStrBase::GetNumMatches(const char* _pSubStr) const
{
	MAUTOSTRIP(CStrBase_GetNumMatches_2, 0);
	if (!_pSubStr) return -1;
	
	if (IsAnsi())
	{
		const char* p = GetStr();
		if (!p) return -1;

		int c = 0;
		while(p)
		{
			p = strstr(p, _pSubStr);
			if (p)
			{
				c++;
				p++;
			}
		}
		return c;
	}
	else if (IsUnicode())
	{
		const wchar* p = GetStrW();
		if (!p) return -1;

		wchar SubStr[1024];
		mint l = strlen(_pSubStr);
		if (l > 1023) Error("GetNumMatches", "Max sub string length is 1023.");
		mfscpy(SubStr, CSTR_FMT_UNICODE, _pSubStr, CSTR_FMT_ANSI);

		int c = 0;
		while(p)
		{
			p = wcsstr(p, SubStr);
			if (p)
			{
				c++;
				p++;
			}
		}
		return c;
	}
	else
		return -1;
}

int CStrBase::Find(const char* _pSubStr) const
{
	MAUTOSTRIP(CStrBase_Find, 0);
	if (!_pSubStr) return -1;

	if (IsAnsi())
	{
		const char* p = GetStr();
		if (!p) return -1;
		char* pPos = strstr((char *)p, _pSubStr);
		if (!pPos) return -1;
		return (pPos - p);
	}
	else
	{
		wchar SubStr[1024];
		mint l = strlen(_pSubStr);
		if (l > 1023) Error("Find", "Max sub string length is 1023.");
		mfscpy(SubStr, CSTR_FMT_UNICODE, _pSubStr, CSTR_FMT_ANSI);

		const wchar* p = GetStrW();
		if (!p) return -1;
		const wchar* pPos = wcsstr(p, SubStr);
		if (!pPos) return -1;
		return (pPos - p);
	}
};

int CStrBase::FindFrom(int _iStart, const char* _pSubStr) const
{
	if(!_pSubStr) return -1;

	if (IsAnsi())
	{
		const char* p = GetStr();
		if(!p) return -1;
		char* pPos = strstr((char*)p + _iStart, _pSubStr);
		if (!pPos) return -1;
		return (pPos - p);
	}
	else
	{
		wchar SubStr[1024];
		mint l = strlen(_pSubStr);
		if (l > 1023) Error("Find", "Max sub string length is 1023.");

		mfscpy(SubStr, CSTR_FMT_UNICODE, _pSubStr, CSTR_FMT_ANSI);

		const wchar* p = GetStrW();
		if (!p) return -1;
		const wchar* pPos = wcsstr(p + _iStart, SubStr);
		if (!pPos) return -1;
		return (pPos - p);
	}
}

int CStrBase::FindReverse(const char* _pSubStr) const
{
	MAUTOSTRIP(CStrBase_FindReverse, 0);
	if (!_pSubStr) return -1;

	if (IsAnsi())
	{
		const char* p = GetStr();
		if (!p) return -1;
		const char* pPos = NULL;
		while(p != NULL)
		{
			p = strstr(p, _pSubStr);
			if (p)
			{
				pPos = p;
				p++;
			}
		}
		if (!pPos) return -1;
		return (pPos - GetStr());
	}
	else if (IsUnicode())
	{
		wchar SubStr[1024];
		mint l = strlen(_pSubStr);
		if (l > 1023) Error("FindReverse", "Max sub string length is 1023.");
		mfscpy(SubStr, CSTR_FMT_UNICODE, _pSubStr, CSTR_FMT_ANSI);

		const wchar* p = GetStrW();
		if (!p) return -1;
		const wchar* pPos = NULL;
		while(p != NULL)
		{
			p = wcsstr(p, SubStr);
			if (p)
			{
				pPos = p;
				p++;
			}
		}
		if (!pPos) return -1;
		return (pPos - GetStrW());
	}
	else
		return -1;
};

int CStrBase::FindOneOf(const char* _pStrCharSet) const
{
	MAUTOSTRIP(CStrBase_FindOneOf, 0);
	if (!_pStrCharSet) return -1;

	if (IsAnsi())
	{
		const char* p = GetStr();
		if (!p) return -1;
		
		int l = StrLen(p);
		int setl = StrLen(_pStrCharSet);

		for(int i = 0; i < l; i++)
		{
			char ch = p[i];
			for(int is = 0; is < setl; is++)
				if (_pStrCharSet[is] == ch) return i;
		}

		return -1;
	}
	else if (IsUnicode())
	{
		const wchar* p = GetStrW();
		if (!p) return -1;
		
		int l = StrLen(p);
		int setl = StrLen(_pStrCharSet);

		for(int i = 0; i < l; i++)
		{
			char ch = p[i];
			for(int is = 0; is < setl; is++)
				if ((uint32)_pStrCharSet[is] == (uint32)ch) return i;
		}

		return -1;
	}
	else
		return -1;
};

int CStrBase::CompareSubStr(const CStrBase& _SubStr, int _Pos) const
{
	if (_SubStr.IsAnsi())
		return CompareSubStr(_SubStr.Str(), _Pos);
	else
		return CompareSubStr(_SubStr.StrW(), _Pos);
}

void CStrBase::Trim()
{
	MAUTOSTRIP(CStrBase_Trim, MAUTOSTRIP_VOID);
	// Optimerar inte allokeringslängden.
	MakeUnique();
	int len = Len();
	if (!len) return;

	if (IsAnsi())
	{
		char* pStr = GetStr();
		if (!pStr) return;

		int iSrc = 0;
		while(pStr[iSrc] && IsWhiteSpace(pStr[iSrc])) iSrc++;
		int iStart = iSrc;
		if (iStart == len)
		{
			pStr[0] = 0;
			return;
		}
		iSrc = len-1;
		while((iSrc > iStart) && IsWhiteSpace(pStr[iSrc])) iSrc--;

		int nlen = iSrc - iStart + 1;
		for(int i = 0; i < nlen; i++)
			pStr[i] = pStr[i + iStart];
		pStr[nlen] = 0;
	}
	else if (IsUnicode())
	{
		wchar* pStr = GetStrW();
		if (!pStr) return;

		int iSrc = 0;
		while(pStr[iSrc] && IsWhiteSpace(pStr[iSrc])) iSrc++;
		int iStart = iSrc;
		if (iStart == len)
		{
			pStr[0] = 0;
			return;
		}
		iSrc = len-1;
		while((iSrc > iStart) && IsWhiteSpace(pStr[iSrc])) iSrc--;

		int nlen = iSrc - iStart + 1;
		for(int i = 0; i < nlen; i++)
			pStr[i] = pStr[i + iStart];
		pStr[nlen] = 0;
	}
}

void CStrBase::MakeUpperCase()
{
	MAUTOSTRIP(CStrBase_MakeUpperCase, MAUTOSTRIP_VOID);
	MakeUnique();
	if (IsAnsi())
	{
		if (GetStr()) strupr(GetStr());
	}
	else if (IsUnicode())
	{
		if (GetStrW()) wcsupr(GetStrW());
	}
}

bool CStrBase::IsNumeric()
{
	MAUTOSTRIP(CStrBase_IsNumeric, MAUTOSTRIP_VOID);
	// Supports 0x format
	int len = Len();
	if(!len)
		return true;

	if(IsAnsi())
	{
		const char *pSt = GetStr();
		int i;
		bool bHex = false;
		if(pSt[0] == '0' && pSt[1] == 'x')
		{
			bHex = true;
			i = 2;
		}
		else
			i = 0;
		while(true)
		{
			char ch = pSt[i++];
			if(ch == 0)
				return true;
			if(IsDigit(ch))
				continue;
			if(bHex && ((ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')))
				continue;
			return false;
		}
	}
	else if(IsUnicode())
	{
		const wchar *pSt = GetStrW();
		int i;
		bool bHex = false;
		if(pSt[0] == '0' && pSt[1] == 'x')
		{
			bHex = true;
			i = 2;
		}
		else
			i = 0;
		while(true)
		{
			char ch = pSt[i++];
			if(ch == 0)
				return true;
			if(IsDigit(ch))
				continue;
			if(bHex && ((ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')))
				continue;
			return false;
		}
	}
	return true;
}

void CStrBase::MakeLowerCase()
{
	MAUTOSTRIP(CStrBase_MakeLowerCase, MAUTOSTRIP_VOID);
	MakeUnique();
	if (IsAnsi())
	{
		if (GetStr()) strlwr(GetStr());
	}
	else if (IsUnicode())
	{
		if (GetStrW()) wcslwr(GetStrW());
	}
}

int CStrBase::GetIntSep(const char* _pSeparator)
{
	MAUTOSTRIP(CStrBase_GetIntSep, 0);
	int i = Find(_pSeparator);
	int sl = StrLen(_pSeparator);
	if (i >= 0)
	{
		MakeUnique();

		int v = 0;
		int n = Val_int(StrAny(), v, GetFmt());
		if (n)
			mfscpy(StrAny(), GetFmt(), StrAny(i+sl), GetFmt());
		else
			Clear();	// A parsing error occured. Clearing will avoid inifinte iterations over this function.

		return v;
	}
	else
	{
		int v = Val_int();
		Clear();
		return v;
	}
}

fp8 CStrBase::Getfp8Sep(const char* _pSeparator)
{
	MAUTOSTRIP(CStrBase_Getfp8Sep, 0.0);
	int i = Find(_pSeparator);
	int sl = StrLen(_pSeparator);
	if (i >= 0)
	{
		MakeUnique();

		fp8 v = 0;
		int n = Val_fp8(StrAny(), v, GetFmt());
		if (n)
			mfscpy(StrAny(), GetFmt(), StrAny(i+sl), GetFmt());
		else
			Clear();	// A parsing error occured. Clearing will avoid inifinte iterations over this function.

		return v;
	}
	else
	{
		fp8 v = Val_fp8();
		Clear();
		return v;
	}
}

int CStrBase::GetIntMSep(const char* _pSeparators)
{
	MAUTOSTRIP(CStrBase_GetIntMSep, 0);
	int i = FindOneOf(_pSeparators);
	int sl = 1;
	if (i >= 0)
	{
		MakeUnique();

		int v = 0;
		int n = Val_int(StrAny(), v, GetFmt());
		if (n)
			mfscpy(StrAny(), GetFmt(), StrAny(i+sl), GetFmt());
		else
			Clear();	// A parsing error occured. Clearing will avoid inifinte iterations over this function.

		return v;
	}
	else
	{
		int v = Val_int();
		Clear();
		return v;
	}
}

fp8 CStrBase::Getfp8MSep(const char* _pSeparators)
{
	MAUTOSTRIP(CStrBase_Getfp8MSep, 0.0);
	int i = FindOneOf(_pSeparators);
	int sl = 1;
	if (i >= 0)
	{
		MakeUnique();

		fp8 v = 0;
		int n = Val_fp8(StrAny(), v, GetFmt());
		if (n)
			mfscpy(StrAny(), GetFmt(), StrAny(i+sl), GetFmt());
		else
			Clear();	// A parsing error occured. Clearing will avoid inifinte iterations over this function.

		return v;
	}
	else
	{
		fp8 v = Val_fp8();
		Clear();
		return v;
	}
}

// -------------------------------------------------------------------
bool CStrBase::IsWhiteSpace(char _ch)
{
	MAUTOSTRIP(CStrBase_IsWhiteSpace, false);
	switch(_ch)
	{
	case 8 : return true;
	case 9 : return true;
	case 10 : return true;
	case 13 : return true;
	case 32 : return true;
	default : return false;
	}
}

bool CStrBase::IsWhiteSpace(wchar _ch)
{
	MAUTOSTRIP(CStrBase_IsWhiteSpace_2, false);
	if (_ch > 255)
		return false;
	else
		return IsWhiteSpace(char(_ch));
}

bool CStrBase::IsDigit(char _ch)
{
	MAUTOSTRIP(CStrBase_IsDigit, false);
	return (_ch >= '0' && _ch <= '9');
}

bool CStrBase::IsDigit(wchar _ch)
{
	MAUTOSTRIP(CStrBase_IsDigit_2, false);
	return (_ch >= '0' && _ch <= '9');
}

int CStrBase::SkipADigit(const char* pStr, int pos, int len)
{
	MAUTOSTRIP(CStrBase_SkipADigit, 0);
	int bNumber = FALSE;
	while (pos < len) 
	{
		if ((pStr[pos] >= '0') && (pStr[pos] <= '9'))
			bNumber = TRUE;
		else
		{
//			if (pStr[pos] == '-')
//				if (bNumber) return pos;
//			else
			{
				if (pStr[pos] != '.')
					if (bNumber) return pos;
			}
		}
		pos++;
	}
	return pos;
}

int CStrBase::GoToDigit(const char* pStr, int pos, int len)
{
	MAUTOSTRIP(CStrBase_GoToDigit, 0);
	while (pos < len) 
	{
		if ((pStr[pos] == '-') || ((pStr[pos] >= '0') && (pStr[pos] <= '9')) || (pStr[pos] == '.')) return pos;
		pos++;
	}
	return pos;
}

mint CStrBase::StrLen(const char* _pStr)
{
	MAUTOSTRIP(CStrBase_StrLen, 0);
	if (!_pStr) return 0;
	return strlen(_pStr);
}

mint CStrBase::StrLen(const wchar* _pStr)
{
	MAUTOSTRIP(CStrBase_StrLen_2, 0);
	if (!_pStr) return 0;
	return wcslen(_pStr);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:	StringToHash.

	Comments:	Creates a 32 bit ID from a character string.

	Arguments:	_pString  - input string.

	Notes:		If there's problems with colliding hashes, just replace 
	            the algorithm used.. 
	            The one I use now is called "djb2". It's very simple,
	            but works really well.

	Author:		Anton Ragnarsson
\*____________________________________________________________________*/
uint32 CStrBase::StrHash(const char* _pString)
{
	uint32 Hash = 5381;
	if (_pString)
		for (uint32 i=0; _pString[i] != 0; i++)
			Hash = Hash*33 + CStrBase::clwr(_pString[i]);
	Hash -= 5381;	// So empty string == 0
	return Hash;
}

uint32 CStrBase::StrHash() const
{
	return StrHash(Str());
}

int CStrBase::TranslateInt(const char* _pVal, const char** _pStrings)
{
	MAUTOSTRIP(CStrBase_TranslateInt, 0);
//	int Flags = 0;
	CFStr Val(_pVal);
	Val.MakeUpperCase();

	CFStr s = Val.GetStrMSep(" ,+");
	s.Trim();
	if (s.Left(2) == "0X" ||
		(s[0] >= '0' && s[0] <= '9'))
	{
		return s.Val_int();
	}
	else
		for(int i = 0; _pStrings[i]; i++)
			if (s.CompareNoCase(_pStrings[i]) == 0)
				return i;
	return -1;
}

int CStrBase::TranslateFlags(const char* _pVal, const char** _pStrings)
{
	MAUTOSTRIP(CStrBase_TranslateFlags, 0);
	int Flags = 0;
	CFStr Val(_pVal);
	Val.MakeUpperCase();

	while(Val != "")
	{
		CFStr s = Val.GetStrMSep(" ,+");
		s.Trim();
		if ((s.Left(2) == "0X") ||
			((s[0] >= '0') && (s[0] <= '9')) || 
			(s[0] == '-'))
		{
			Flags |= s.Val_int();
		}
		else
			for(int i = 0; _pStrings[i]; i++)
				if (s.CompareNoCase(_pStrings[i]) == 0)
					Flags |= (1 << i);
	}
	return Flags;
}

uint16 CStrBase::Ascii2Sjis(char _Char)
{
	MAUTOSTRIP(CStrBase_Ascii2Sjis, 0);
	uint16 ascii_special[33][2] =
	{
		{ 0x8140, ' ' },
		{ 0x8149, '!' },
		{ 0x8168, '"' },
		{ 0x8194, '#' },
		{ 0x8190, '$' },
		{ 0x8193, '%' },
		{ 0x8195, '&' },
		{ 0x8166, '\''},
		{ 0x8169, '(' },
		{ 0x816a, ')' },
		{ 0x8196, '*' },
		{ 0x817b, '+' },
		{ 0x8143, ',' },
		{ 0x817c, '-' },
		{ 0x8144, '.' },
		{ 0x815e, '/' },
		{ 0x8146, ':' },
		{ 0x8147, ';' },
		{ 0x8171, '<' },
		{ 0x8181, '=' },
		{ 0x8172, '>' },
		{ 0x8148, '?' },
		{ 0x8197, '@' },
		{ 0x816d, '[' },
		{ 0x818f, '\\'},
		{ 0x816e, ']' },
		{ 0x814f, '^' },
		{ 0x8151, '_' },
		{ 0x8165, '`' },
		{ 0x816f, '{' },
		{ 0x8162, '|' },
		{ 0x8170, '}' },
		{ 0x8150, '~' },
	};

	uint16 ascii_table[3][2] =
	{
		{ 0x824f, 0x30 },	// 0-9
		{ 0x8260, 0x41 },	// A-Z
		{ 0x8281, 0x61 },	// a-z
	};

	unsigned char stmp;
	unsigned char stmp2 = 0;

	if((_Char >= 0x20) && (_Char <= 0x2f))
		stmp2 = 1;
	else if((_Char >= 0x30) && (_Char <= 0x39))
		stmp = 0;
	else if((_Char >= 0x3a) && (_Char <= 0x40))
		stmp2 = 11;
	else if((_Char >= 0x41) && (_Char <= 0x5a))
		stmp = 1;
	else if((_Char >= 0x5b) && (_Char <= 0x60))
		stmp2 = 37;
	else if((_Char >= 0x61) && (_Char <= 0x7a))
		stmp = 2;
	else if((_Char >= 0x7b) && (_Char <= 0x7e))
		stmp2 = 63;
	else 
	{
//		scePrintf("Bad ASCII code 0x%x\n", _Char);
		return 0;
	}

	if(stmp2)
	   	return ascii_special[_Char - 0x20 - (stmp2 - 1)][0];

	return ascii_table[stmp][0] + _Char - ascii_table[stmp][1];
}

//////////////////////////////////////////////////
void CStrBase::AsciiString2Sjis(const char *_pAsciiStr, int16 *_pSjisStr, int _Count)
{
	MAUTOSTRIP(CStrBase_AsciiString2Sjis, MAUTOSTRIP_VOID);
	int i;
	mint Length = strlen(_pAsciiStr);
	if(Length >= _Count)
		Length = _Count - 1;
	for(i = 0; i < Length; i++)
	{
		uint16 Sjis = Ascii2Sjis(_pAsciiStr[i]);
		_pSjisStr[i] = (Sjis >> 8) | (Sjis << 8);
	}
	_pSjisStr[i] = 0x0000;
}

int CStrBase::TranslateInt(const char** _pStrings)
{
	MAUTOSTRIP(CStrBase_TranslateInt_2, 0);
	if(!IsAnsi())
		Error("TranslateInt", "Unicode is not supported.");
	return TranslateInt(GetStr(), _pStrings);
}

int CStrBase::TranslateFlags(const char** _pStrings)
{
	MAUTOSTRIP(CStrBase_TranslateFlags_2, 0);
	if(!IsAnsi())
		Error("TranslateFlags", "Unicode is not supported.");
	return TranslateFlags(GetStr(), _pStrings);
}

void CStrBase::CreateFromInt(int _iInt, const char** _pStrings)
{
	MAUTOSTRIP(CStrBase_CreateFromInt, MAUTOSTRIP_VOID);
	for(int i = 0; i < _iInt; i++)
		if(_pStrings[i] == NULL)
		{
			CaptureFormated("%i", _iInt);
			return;
		}

	Capture(_pStrings[_iInt]);
}

void CStrBase::CreateFromFlags(int _iFlags, const char** _pStrings)
{
	MAUTOSTRIP(CStrBase_CreateFromFlags, MAUTOSTRIP_VOID);
	CFStr Flags;
	int iIndex = 0;
	while(_iFlags)
	{
		if(_pStrings[iIndex] == NULL)
		{
			if(Flags.Len() > 0)
				Flags += "+";
			Flags += CFStrF("0x%x",_iFlags);
			break;
		}
		if(_iFlags & 1)
		{
			if(Flags.Len() > 0)
				Flags += "+";
			Flags += _pStrings[iIndex];
		}
		_iFlags >>= 1;
		iIndex++;
	}
	Capture(Flags);
}

/*
 * Copyright Patrick Powell 1995
 * This code is based on code written by Patrick Powell (papowell@astart.com)
 * It may be used for any purpose as long as this notice remains intact
 * on all source code distributions
 */

/**************************************************************
 * Original:
 * Patrick Powell Tue Apr 11 09:48:21 PDT 1995
 * A bombproof version of doprnt (dopr) included.
 * Sigh.  This sort of thing is always nasty do deal with.  Note that
 * the version here does not include floating point...
 *
 * snprintf() is used instead of sprintf() as it does limit checks
 * for string length.  This covers a nasty loophole.
 *
 * The other functions are there to prevent NULL pointers from
 * causing nast effects.
 *
 * More Recently:
 *  Brandon Long <blong@fiction.net> 9/15/96 for mutt 0.43
 *  This was ugly.  It is still ugly.  I opted out of floating point
 *  numbers, but the formatter understands just about everything
 *  from the normal C string format, at least as far as I can tell from
 *  the Solaris 2.5 printf(3S) man page.
 *
 *  Brandon Long <blong@fiction.net> 10/22/97 for mutt 0.87.1
 *    Ok, added some minimal floating point support, which means this
 *    probably requires libm on most operating systems.  Don't yet
 *    support the exponent (e,E) and sigfig (g,G).  Also, fmtint()
 *    was pretty badly broken, it just wasn't being exercised in ways
 *    which showed it, so that's been fixed.  Also, formated the code
 *    to mutt conventions, and removed dead code left over from the
 *    original.  Also, there is now a builtin-test, just compile with:
 *           gcc -DTEST_SNPRINTF -o snprintf snprintf.c -lm
 *    and run snprintf for results.
 * 
 *  Thomas Roessler <roessler@guug.de> 01/27/98 for mutt 0.89i
 *    The PGP code was using unsigned hexadecimal formats. 
 *    Unfortunately, unsigned formats simply didn't work.
 *
 *  Michael Elkins <me@cs.hmc.edu> 03/05/98 for mutt 0.90.8
 *    The original code assumed that both snprintf() and vsnprintf() were
 *    missing.  Some systems only have snprintf() but not vsnprintf(), so
 *    the code is now broken down under HAVE_SNPRINTF and HAVE_VSNPRINTF.
 *
 *  Andrew Tridgell (tridge@samba.org) Oct 1998
 *    fixed handling of %.0f
 *    added test for HAVE_LONG_DOUBLE
 *
 **************************************************************/

//#include "config.h"

#define _VA_LIST_DEFINED
#define HAVE_STDARG_H

#if !defined(HAVE_SNPRINTF) || !defined(HAVE_VSNPRINTF)

#include <string.h>
#include <ctype.h>
// #include <sys/types.h>

/* Define this as a fall through, HAVE_STDARG_H is probably already set */

#define HAVE_VARARGS_H


/* varargs declarations: */

#if defined(HAVE_STDARG_H)
	#include <stdarg.h>
	#define HAVE_STDARGS    /* let's hope that works everywhere (mj) */
	#define VA_LOCAL_DECL   va_list ap
	#define VA_START(f)     va_start(ap, f)
	#define VA_SHIFT(v,t)  ;   /* no-op for ANSI */
	#define VA_END          va_end(ap)

#elif defined(HAVE_VARARGS_H)
	#include <varargs.h>
	#undef HAVE_STDARGS
	#define VA_LOCAL_DECL   va_list ap
	#define VA_START(f)     va_start(ap)      /* f is ignored! */
	#define VA_SHIFT(v,t) v = va_arg(ap,t)
	#define VA_END        va_end(ap)

#else
	/*XX ** NO VARARGS ** XX*/
	#error "?"
#endif

#ifdef	CPU_SUPPORT_FP8
#ifdef HAVE_LONG_DOUBLE
#define LDOUBLE long double
#else
#define LDOUBLE double
#endif
#else
#define	LDOUBLE	float
#endif


static void dopr (char *buffer, mint maxlen, const char *format, 
                  va_list args);
static void fmtstr (char *buffer, mint *currlen, mint maxlen,
		    char *value, int flags, int min, int max);
static void fmtint (char *buffer, mint *currlen, mint maxlen,
		    int64 value, int base, int min, int max, int flags);
static void fmtfp (char *buffer, mint *currlen, mint maxlen,
		   LDOUBLE fvalue, int min, int max, int flags);
static void dopr_outch (char *buffer, mint *currlen, mint maxlen, char c );

/*
 * dopr(): poor man's version of doprintf
 */

/* format read states */
#define DP_S_DEFAULT 0
#define DP_S_FLAGS   1
#define DP_S_MIN     2
#define DP_S_DOT     3
#define DP_S_MAX     4
#define DP_S_MOD     5
#define DP_S_CONV    6
#define DP_S_DONE    7

/* format flags - Bits */
#define DP_F_MINUS 	(1 << 0)
#define DP_F_PLUS  	(1 << 1)
#define DP_F_SPACE 	(1 << 2)
#define DP_F_NUM   	(1 << 3)
#define DP_F_ZERO  	(1 << 4)
#define DP_F_UP    	(1 << 5)
#define DP_F_UNSIGNED 	(1 << 6)

/* Conversion Flags */
#define DP_C_SHORT   1
#define DP_C_LONG    2
#define DP_C_LDOUBLE 3

#define char_to_int(p) (p - '0')

#ifdef MAX
#undef MAX
#endif

#define MAX(p,q) ((p >= q) ? p : q)

static void dopr (char *buffer, mint maxlen, const char *format, va_list args)
{
	MAUTOSTRIP(dopr, MAUTOSTRIP_VOID);
  char ch;
  int64 value;
  LDOUBLE fvalue;
  char *strvalue;
  int min;
  int max;
  int state;
  int flags;
  int cflags;
  mint currlen;
  
  state = DP_S_DEFAULT;
  currlen = flags = cflags = min = 0;
  max = -1;
  ch = *format++;

  while (state != DP_S_DONE)
  {
    if ((ch == '\0') || (currlen >= maxlen)) 
      state = DP_S_DONE;

    switch(state) 
    {
    case DP_S_DEFAULT:
      if (ch == '%') 
	state = DP_S_FLAGS;
      else 
	dopr_outch (buffer, &currlen, maxlen, ch);
      ch = *format++;
      break;
    case DP_S_FLAGS:
      switch (ch) 
      {
      case '-':
	flags |= DP_F_MINUS;
        ch = *format++;
	break;
      case '+':
	flags |= DP_F_PLUS;
        ch = *format++;
	break;
      case ' ':
	flags |= DP_F_SPACE;
        ch = *format++;
	break;
      case '#':
	flags |= DP_F_NUM;
        ch = *format++;
	break;
      case '0':
	flags |= DP_F_ZERO;
        ch = *format++;
	break;
      default:
	state = DP_S_MIN;
	break;
      }
      break;
    case DP_S_MIN:
	  if (CStrBase::IsDigit(ch)) 
      {
	min = 10*min + char_to_int (ch);
	ch = *format++;
      } 
      else if (ch == '*') 
      {
	min = va_arg (args, int);
	ch = *format++;
	state = DP_S_DOT;
      } 
      else 
	state = DP_S_DOT;
      break;
    case DP_S_DOT:
      if (ch == '.') 
      {
	state = DP_S_MAX;
	ch = *format++;
      } 
      else 
	state = DP_S_MOD;
      break;
    case DP_S_MAX:
      if (CStrBase::IsDigit(ch)) 
      {
	if (max < 0)
	  max = 0;
	max = 10*max + char_to_int (ch);
	ch = *format++;
      } 
      else if (ch == '*') 
      {
	max = va_arg (args, int);
	ch = *format++;
	state = DP_S_MOD;
      } 
      else 
	state = DP_S_MOD;
      break;
    case DP_S_MOD:
      /* Currently, we don't support Long Long, bummer */
      switch (ch) 
      {
      case 'h':
	cflags = DP_C_SHORT;
	ch = *format++;
	break;
      case 'l':
	cflags = DP_C_LONG;
	ch = *format++;
	break;
      case 'L':
	cflags = DP_C_LDOUBLE;
	ch = *format++;
	break;
      default:
	break;
      }
      state = DP_S_CONV;
      break;
    case DP_S_CONV:
      switch (ch) 
      {
      case 'd':
      case 'i':
	if (cflags == DP_C_SHORT) 
#ifdef COMPILER_GNU
		value = (short int)va_arg (args, int);
#else
		value = va_arg (args, short int);
#endif
	else if (cflags == DP_C_LONG)
	  value = va_arg (args, int64);
	else
	  value = va_arg (args, int);
	fmtint (buffer, &currlen, maxlen, value, 10, min, max, flags);
	break;
      case 'o':
	flags |= DP_F_UNSIGNED;
	if (cflags == DP_C_SHORT)
#ifdef COMPILER_GNU
		value = (unsigned short int)va_arg (args, int);
#else
		value = va_arg (args, unsigned short int);
#endif
	else if (cflags == DP_C_LONG)
	  value = va_arg (args, uint64);
	else
	  value = va_arg (args, unsigned int);
	fmtint (buffer, &currlen, maxlen, value, 8, min, max, flags);
	break;
      case 'u':
	flags |= DP_F_UNSIGNED;
	if (cflags == DP_C_SHORT)
#ifdef COMPILER_GNU
		value = (unsigned short int)va_arg (args, int);
#else
		value = va_arg (args, unsigned short int);
#endif
	else if (cflags == DP_C_LONG)
	  value = va_arg (args, uint64);
	else
	  value = va_arg (args, unsigned int);
	fmtint (buffer, &currlen, maxlen, value, 10, min, max, flags);
	break;
      case 'X':
	flags |= DP_F_UP;
      case 'x':
	flags |= DP_F_UNSIGNED;
	if (cflags == DP_C_SHORT)
#ifdef COMPILER_GNU
		value = (unsigned short int)va_arg (args, int);
#else
		value = va_arg (args, unsigned short int);
#endif
	else if (cflags == DP_C_LONG)
	  value = va_arg (args, uint64);
	else
	  value = va_arg (args, unsigned int);
	fmtint (buffer, &currlen, maxlen, value, 16, min, max, flags);
	break;
      case 'f':
	if (cflags == DP_C_LDOUBLE)
	  fvalue = va_arg (args, LDOUBLE);
	else
#ifdef	CPU_SUPPORT_FP8
	  fvalue = va_arg (args, double);
#else
	  fvalue = va_arg (args, float);
#endif
	/* um, floating point? */
	fmtfp (buffer, &currlen, maxlen, fvalue, min, max, flags);
	break;
      case 'E':
	flags |= DP_F_UP;
      case 'e':
	if (cflags == DP_C_LDOUBLE)
	  fvalue = va_arg (args, LDOUBLE);
	else
#ifdef	CPU_SUPPORT_FP8
	  fvalue = va_arg (args, double);
#else
	  fvalue = va_arg (args, float);
#endif
	break;
      case 'G':
	flags |= DP_F_UP;
      case 'g':
	if (cflags == DP_C_LDOUBLE)
	  fvalue = va_arg (args, LDOUBLE);
	else
#ifdef	CPU_SUPPORT_FP8
	  fvalue = va_arg (args, double);
#else
	  fvalue = va_arg (args, float);
#endif
	break;
      case 'c':
	dopr_outch (buffer, &currlen, maxlen, va_arg (args, int));
	break;
      case 's':
	strvalue = va_arg (args, char *);
	if (max < 0) 
	  max = maxlen; /* ie, no max */
	fmtstr (buffer, &currlen, maxlen, strvalue, flags, min, max);
	break;
      case 'p':
	strvalue = (char*)va_arg (args, void *);
	fmtint (buffer, &currlen, maxlen, (mint) strvalue, 16, min, max, flags);
	break;
      case 'n':
	if (cflags == DP_C_SHORT) 
	{
	  short int *num;
	  num = va_arg (args, short int *);
	  *num = currlen;
        } 
	else if (cflags == DP_C_LONG) 
	{
	  int64 *num;
	  num = va_arg (args, int64 *);
	  *num = currlen;
        } 
	else 
	{
	  int *num;
	  num = va_arg (args, int *);
	  *num = currlen;
        }
	break;
      case '%':
	dopr_outch (buffer, &currlen, maxlen, ch);
	break;
      case 'w':
	/* not supported yet, treat as next char */
	ch = *format++;
	break;
      default:
	/* Unknown, skip */
	break;
      }
      ch = *format++;
      state = DP_S_DEFAULT;
      flags = cflags = min = 0;
      max = -1;
      break;
    case DP_S_DONE:
      break;
    default:
      /* hmm? */
      break; /* some picky compilers need this */
    }
  }
  if (currlen < maxlen - 1) 
    buffer[currlen] = '\0';
  else 
    buffer[maxlen - 1] = '\0';
}

static void fmtstr (char *buffer, mint *currlen, mint maxlen,
		    char *value, int flags, int min, int max)
{
	MAUTOSTRIP(fmtstr, MAUTOSTRIP_VOID);
  int padlen, strln;     /* amount to pad */
  int cnt = 0;
  
  if (value == 0)
  {
    value = "<NULL>";
  }

  for (strln = 0; value[strln]; ++strln) {}; /* strlen */
  padlen = min - strln;
  if (padlen < 0) 
    padlen = 0;
  if (flags & DP_F_MINUS) 
    padlen = -padlen; /* Left Justify */

  while ((padlen > 0) && (cnt < max)) 
  {
    dopr_outch (buffer, currlen, maxlen, ' ');
    --padlen;
    ++cnt;
  }
  while (*value && (cnt < max)) 
  {
    dopr_outch (buffer, currlen, maxlen, *value++);
    ++cnt;
  }
  while ((padlen < 0) && (cnt < max)) 
  {
    dopr_outch (buffer, currlen, maxlen, ' ');
    ++padlen;
    ++cnt;
  }
}

/* Have to handle DP_F_NUM (ie 0x and 0 alternates) */

static void fmtint (char *buffer, mint *currlen, mint maxlen,
		    int64 value, int base, int min, int max, int flags)
{
	MAUTOSTRIP(fmint, MAUTOSTRIP_VOID);
  int signvalue = 0;
  uint64 uvalue;
  char convert[20];
  int place = 0;
  int spadlen = 0; /* amount to space pad */
  int zpadlen = 0; /* amount to zero pad */
  int caps = 0;
  
  if (max < 0)
    max = 0;

  uvalue = value;

  if(!(flags & DP_F_UNSIGNED))
  {
    if( value < 0 ) {
      signvalue = '-';
      uvalue = -value;
    }
    else
      if (flags & DP_F_PLUS)  /* Do a sign (+/i) */
	signvalue = '+';
    else
      if (flags & DP_F_SPACE)
	signvalue = ' ';
  }
  
  if (flags & DP_F_UP) caps = 1; /* Should characters be upper case? */

  do {
    convert[place++] =
      (caps? "0123456789ABCDEF":"0123456789abcdef")
      [uvalue % (uint64)base  ];
    uvalue = (uvalue / (uint64)base );
  } while(uvalue && (place < 20));
  if (place == 20) place--;
  convert[place] = 0;

  zpadlen = max - place;
  spadlen = min - MAX (max, place) - (signvalue ? 1 : 0);
  if (zpadlen < 0) zpadlen = 0;
  if (spadlen < 0) spadlen = 0;
  if (flags & DP_F_ZERO)
  {
    zpadlen = MAX(zpadlen, spadlen);
    spadlen = 0;
  }
  if (flags & DP_F_MINUS) 
    spadlen = -spadlen; /* Left Justifty */

#ifdef DEBUG_SNPRINTF
  dprint (1, (debugfile, "zpad: %d, spad: %d, min: %d, max: %d, place: %d\n",
      zpadlen, spadlen, min, max, place));
#endif

  /* Spaces */
  while (spadlen > 0) 
  {
    dopr_outch (buffer, currlen, maxlen, ' ');
    --spadlen;
  }

  /* Sign */
  if (signvalue) 
    dopr_outch (buffer, currlen, maxlen, signvalue);

  /* Zeros */
  if (zpadlen > 0) 
  {
    while (zpadlen > 0)
    {
      dopr_outch (buffer, currlen, maxlen, '0');
      --zpadlen;
    }
  }

  /* Digits */
  while (place > 0) 
    dopr_outch (buffer, currlen, maxlen, convert[--place]);
  
  /* Left Justified spaces */
  while (spadlen < 0) {
    dopr_outch (buffer, currlen, maxlen, ' ');
    ++spadlen;
  }
}

static LDOUBLE abs_val (LDOUBLE value)
{
	MAUTOSTRIP(abs_val, 0.0);
  LDOUBLE result = value;

  if (value < 0)
    result = -value;

  return result;
}

static LDOUBLE pow10 (int exp)
{
	MAUTOSTRIP(pow10, 0.0);
  LDOUBLE result = 1;

  while (exp)
  {
    result *= 10;
    exp--;
  }
  
  return result;
}

static int64 s_round (LDOUBLE value)
{
	MAUTOSTRIP(s_round, 0);
  int64 intpart;

  intpart = (int64)value;
  value = value - intpart;
  if (value >= 0.5)
    intpart++;

  return intpart;
}

static void fmtfp (char *buffer, mint *currlen, mint maxlen,
		   LDOUBLE fvalue, int min, int max, int flags)
{
	MAUTOSTRIP(fmtfp, MAUTOSTRIP_VOID);
  int signvalue = 0;
  LDOUBLE ufvalue;
  char iconvert[20];
  char fconvert[20];
  int iplace = 0;
  int fplace = 0;
  int padlen = 0; /* amount to pad */
  int zpadlen = 0; 
  int caps = 0;
  int64 intpart;
  int64 fracpart;
  
  if (FloatIsNAN(fvalue))
  {
	  dopr_outch(buffer, currlen, maxlen, 'N');
	  dopr_outch(buffer, currlen, maxlen, 'a');
	  dopr_outch(buffer, currlen, maxlen, 'N');
	  return;
  }
  if (FloatIsInfinite(fvalue))
  {
	  if (FloatIsNeg(fvalue))
		  dopr_outch(buffer, currlen, maxlen, '-');
	  dopr_outch(buffer, currlen, maxlen, 'I');
	  dopr_outch(buffer, currlen, maxlen, 'N');
	  dopr_outch(buffer, currlen, maxlen, 'F');
	  return;
  }

  /* 
   * AIX manpage says the default is 0, but Solaris says the default
   * is 6, and sprintf on AIX defaults to 6
   */
  if (max < 0)
    max = 6;

  ufvalue = abs_val (fvalue);

  if (fvalue < 0)
    signvalue = '-';
  else
    if (flags & DP_F_PLUS)  /* Do a sign (+/i) */
      signvalue = '+';
    else
      if (flags & DP_F_SPACE)
	signvalue = ' ';

#if 0
  if (flags & DP_F_UP) caps = 1; /* Sh*ould characters be upper case? */
#endif

  intpart = (int64)ufvalue;

  /* 
   * Sorry, we only support 9 digits past the decimal because of our 
   * conversion method
   */
  if (max > 18)
    max = 18;

  /* We "cheat" by converting the fractional part to integer by
   * multiplying by a factor of 10
   */
  fracpart = s_round ((pow10 (max)) * (ufvalue - intpart));

  if (fracpart >= pow10 (max))
  {
    intpart++;
    fracpart -= (int64)pow10 (max);
  }

#ifdef DEBUG_SNPRINTF
  dprint (1, (debugfile, "fmtfp: %f =? %d.%d\n", fvalue, intpart, fracpart));
#endif

  /* Convert integer part */
  do {
    iconvert[iplace++] =
      (caps? "0123456789ABCDEF":"0123456789abcdef")[intpart % 10];
    intpart = (intpart / 10);
  } while(intpart && (iplace < 20));
  if (iplace == 20) iplace--;
  iconvert[iplace] = 0;

  /* Convert fractional part */
  do {
    fconvert[fplace++] =
      (caps? "0123456789ABCDEF":"0123456789abcdef")[fracpart % 10];
    fracpart = (fracpart / 10);
  } while(fracpart && (fplace < 20));
  if (fplace == 20) fplace--;
  fconvert[fplace] = 0;

  /* -1 for decimal point, another -1 if we are printing a sign */
  padlen = min - iplace - max - 1 - ((signvalue) ? 1 : 0); 
  zpadlen = max - fplace;
  if (zpadlen < 0)
    zpadlen = 0;
  if (padlen < 0) 
    padlen = 0;
  if (flags & DP_F_MINUS) 
    padlen = -padlen; /* Left Justifty */

  if ((flags & DP_F_ZERO) && (padlen > 0)) 
  {
    if (signvalue) 
    {
      dopr_outch (buffer, currlen, maxlen, signvalue);
      --padlen;
      signvalue = 0;
    }
    while (padlen > 0)
    {
      dopr_outch (buffer, currlen, maxlen, '0');
      --padlen;
    }
  }
  while (padlen > 0)
  {
    dopr_outch (buffer, currlen, maxlen, ' ');
    --padlen;
  }
  if (signvalue) 
    dopr_outch (buffer, currlen, maxlen, signvalue);

  while (iplace > 0) 
    dopr_outch (buffer, currlen, maxlen, iconvert[--iplace]);

  /*
   * Decimal point.  This should probably use locale to find the correct
   * char to print out.
   */
  if (max > 0)
  {
    dopr_outch (buffer, currlen, maxlen, '.');
    
	while (zpadlen > 0)
	{
	  dopr_outch (buffer, currlen, maxlen, '0');
	  --zpadlen;
	}
    
	while (fplace > 0) 
      dopr_outch (buffer, currlen, maxlen, fconvert[--fplace]);
  }

  while (padlen < 0) 
  {
    dopr_outch (buffer, currlen, maxlen, ' ');
    ++padlen;
  }
}

static void dopr_outch (char *buffer, mint *currlen, mint maxlen, char c)
{
	MAUTOSTRIP(dopr_outch, MAUTOSTRIP_VOID);
  if (*currlen < maxlen)
    buffer[(*currlen)++] = c;
}

#ifndef HAVE_VSNPRINTF
mint CStrBase::vsnprintf (char *str, mint count, const char *fmt, va_list args)
{
	MAUTOSTRIP(CStrBase_vsnprintf, 0);
  str[0] = 0;
  dopr(str, count, fmt, args);
  return(strlen(str));
}
#endif /* !HAVE_VSNPRINTF */

#ifndef HAVE_SNPRINTF
/* VARARGS3 */
#ifdef HAVE_STDARGS
mint CStrBase::snprintf (char *str,mint count,const char *fmt,...)
#else
mint CStrBase::snprintf (va_alist) va_dcl
#endif
{
	MAUTOSTRIP(CStrBase_snprintf, 0);
#ifndef HAVE_STDARGS
  char *str;
  mint count;
  char *fmt;
#endif
  VA_LOCAL_DECL;
    
  VA_START (fmt);
  VA_SHIFT (str, char *);
  VA_SHIFT (count, mint );
  VA_SHIFT (fmt, char *);
  (void) CStrBase::vsnprintf(str, count, fmt, ap);
  VA_END;
  return(strlen(str));
}
#endif /* !HAVE_SNPRINTF */
#endif /* !HAVE_SNPRINTF */

#ifdef HAVE_VSNPRINTF
int CStrBase::vsnprintf(char *str, mint count, const char *fmt, va_list args)
{
	return ::vsnprintf(str, count, fmt, args);
}
#endif

#ifdef HAVE_SNPRINTF
int CStrBase::snprintf(char *str,mint count,const char *fmt,...)
{
	int len;
	va_list ap;
	va_start(ap, fmt);
	len = CStrBase::vsnprintf(str, count, fmt, ap);
	va_end(ap);
	return len;
}
#endif

#include "MRTC_Protect.cpp"
