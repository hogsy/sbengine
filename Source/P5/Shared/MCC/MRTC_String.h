
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:				MRTC String Classes
					
	Author:				Magnus Högdahl
					
	Copyright:			Starbreeze Studios 1996 - 2001
					
	Contents:			Base class for strings
						General string class
						Fast string template
					
	History:		
		010205:			Added comments

		010610:			Unicode support

\*____________________________________________________________________________________________*/


//class IFile;

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

	template<typename t_CData1, typename t_CData2>
		DIdsPInlineM t_CData1 *StrCopy(t_CData1 *_pTo, const t_CData2 *_pFrom)
	{
		while (*_pFrom)
			*_pTo++ = *_pFrom++;
		*_pTo = 0;

		return _pTo;
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

template <typename t_CType>
void StrReplaceChar(t_CType *_pStr, t_CType _Find, t_CType _Replace)
{
	while (*_pStr)
	{
		if (*_pStr == _Find)
			*_pStr = _Replace;
		++_pStr;
	}
}

#if defined( UNICODE_WORKAROUND )
template <int tLine, int tSize, int tSizeOfFilename, int tSizeOfFunction>
struct TStrUnicodeize
{
	static const char* s_pOrig;
	static wchar m_UnicodeStr[tSize + 1];

	static const wchar* WText(const char* _pStr)
	{
		M_ASSERT(s_pOrig == NULL || s_pOrig == _pStr, "WTEXT input/type failed :(");
		if(!s_pOrig)
		{
			s_pOrig = _pStr;
			for(int i = 0; i < tSize; i++)
			{
				// Have to do proper masking or sign extension will screw us later
				m_UnicodeStr[i] = ((wchar)_pStr[i]) & 0x00ff;
			}
			m_UnicodeStr[tSize] = 0;
		}

		return m_UnicodeStr;
	}
};

template <int tLine, int tSize, int tSizeOfFilename, int tSizeOfFunction> const char* TStrUnicodeize<tLine, tSize, tSizeOfFilename, tSizeOfFunction>::s_pOrig = NULL;
template <int tLine, int tSize, int tSizeOfFilename, int tSizeOfFunction> wchar TStrUnicodeize<tLine, tSize, tSizeOfFilename, tSizeOfFunction>::m_UnicodeStr[tSize + 1];

#define WTEXT(quote) TStrUnicodeize<__LINE__, sizeof(quote), sizeof(__FILE__), sizeof(__FUNCTION__)>::WText(quote)
/*
M_INLINE static wchar *StrUnicodeize( char* _string, const int len )
{
	if( _string[len-2] == 'A' )
	{
		wchar *str = (wchar*)_string;
		const int chars = ( len - 1 ) >> 1;
		
		for( int i = 0; i < chars; i++ )
		{
			*str	= ((wchar)_string[chars+i]) & 0x00ff;
			str++;
		}
		*str	= 0;
	}

	return (wchar*)_string;
}

#define WTEXT(quote)	StrUnicodeize(quote quote "A", sizeof(quote quote "A"))
*/
#else
#define WTEXT(quote)	(const wchar*)L##quote
#endif

#define StringToHash CStrBase::StrHash

// Used to create unicode strings, example: CStr(WTEXT("Frames per second %.2f"), m_FPS);

enum
{
	CSTR_FMT_ANSI = 0,
	CSTR_FMT_UNICODE = 1
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Base class for strings
\*____________________________________________________________________*/

class CStrBase : public CObj
{
public:
	
	static const char ms_WhiteSpaces[];					// Static list of white space characters
	static const char ms_CitationMark[];				// Static list defining a citation mark character

public:
	CStrBase()
	{
	}
	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Replacement for missing runtime functions
	\*____________________________________________________________________*/
	static void mfscpy(void* _pDst, int _DstFmt, const void* _pSrc, int _SrcFmt);				// Multiformat string copy, strcpy and wcscpy in one function
	static void mfsncpy(void* _pDst, int _DstFmt, const void* _pSrc, int _SrcFmt, int _Count);	// Multiformat string copy, strncpy and wcsncpy in one function
	static char clwr(char _c);
	static char cupr(char _c);
	static wchar wclwr(wchar _c);
	static wchar wcupr(wchar _c);
	static void strupr(char* _pStr);
	static void strlwr(char* _pStr);
	static void wcsupr(wchar* _pStr);
	static void wcslwr(wchar* _pStr);
	static int stricmp(const char* _s0, const char* _s1);
	static int strnicmp(const char* _s0, const char* _s1, mint _Len);
	static int wcsicmp(const wchar* _s0, const wchar* _s1);

	static const wchar* wcsstr(const wchar *_s0, const wchar *_s1);
	static int wcsncmp(const wchar *_s0, const wchar *_s1, int _num);
	static unsigned long wcstoul(const wchar *_s0, wchar **_endp, int _base);
	static double wcstod(const wchar *_s0, wchar **_endp);
	static int wcscmp(const wchar *_s0, const wchar *_s1);
	static int wcslen(const wchar *_s0);
	static void wcscpy(wchar *_dest, const wchar *_src);
//	static int swscanf( const wchar *_src, const wchar *_format, ... );
	static mint vswprintf(wchar *_pbuffer, int _BufSize, const wchar *_pStr, va_list _arg);
	static void HexStr(char* _pDest, const void* _pMem, int _nBytes);
	static void HexStr(wchar* _pDest, const void* _pMem, int _nBytes);


	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Function:			String formating function with range-checking.
							Usage is identical to sprintf & vsprintf except
							for the count which is the size of the format
							destination buffer (str).
	\*____________________________________________________________________*/
	static mint snprintf (char *str, mint count, const char *fmt, ...);
	static mint vsnprintf (char *str, mint count, const char *fmt, va_list arg);

	virtual char operator[](int i) const pure;
	virtual operator char* () pure;
	virtual operator const char* () const pure;
	virtual char* GetStr();
	virtual const char* GetStr() const;
	virtual const char* Str() const;					// Always return a valid string.

	virtual operator wchar*() pure;						// Returns NULL on empty strings
	virtual operator const wchar*() const pure;			// Returns NULL on empty strings
	virtual wchar* GetStrW();							// Returns NULL on empty strings
	virtual const wchar* GetStrW() const;				// Returns NULL on empty strings
	virtual const wchar* StrW() const;					// Always return a valid string.

	virtual const void* StrData() const;					// Always return a valid string.

	virtual void* StrAny(int _Pos = 0) pure;			// Returns NULL on empty strings
	virtual const void* StrAny(int _Pos = 0) const pure;// Returns NULL on empty strings

	virtual int GetFmt() const;
	virtual int IsAnsi() const;
	virtual int IsUnicode() const;
	virtual int IsPureAnsi() const;						// Returns true if all characters in the string are in the [0..255] range. i.e, wether a convertion to ansi is lossless.

	virtual void Clear() pure;

	static int Val_int(const char* _p, int& _Value);	// Returns number of characters processed.
	static int Val_fp64(const char* _p, fp64& _Value);	// Returns number of characters processed.
	static int Val_int(const wchar* _p, int& _Value);	// Returns number of characters processed.
	static int Val_fp64(const wchar* _p, fp64& _Value);	// Returns number of characters processed.
	static int Val_int(const void* _p, int& _Value, int _Fmt);	// Returns number of characters processed.
	static int Val_fp64(const void* _p, fp64& _Value, int _Fmt);	// Returns number of characters processed.

	virtual int Val_int() const;
	virtual fp64 Val_fp64() const;

	virtual int Len() const pure;
	virtual void Capture(const char* _pStr) pure;
	virtual void Capture(const char* _pStr, int _Len) pure;
	virtual void CaptureFormated(const char* _pStr, ...) pure;

	virtual void MakeUnique() pure;

	static int Compare(const char*, const char*);		// -1 Less, 0 Equal, 1 Greater, goes for all Compare functions
	static int Compare(const wchar*, const char*);
	static int Compare(const char*, const wchar*);
	static int Compare(const wchar*, const wchar*);
	static int Compare(const CStrBase&, const char*);
	static int Compare(const CStrBase&, const wchar*);
	static int Compare(const CStrBase&, const CStrBase&);
	static int CompareNoCase(const char*, const char*);
	static int CompareNoCase(const wchar*, const char*);
	static int CompareNoCase(const char*, const wchar*);
	static int CompareNoCase(const wchar*, const wchar*);
	static int CompareNoCase(const CStrBase&, const char*);
	static int CompareNoCase(const CStrBase&, const wchar*);
	static int CompareNoCase(const CStrBase&, const CStrBase&);

	template<class T1, class T2> static int CompareSubStr(const T1* _pStr, int _Pos, const T2* _pSubStr);

	int Compare(const char* _pStr) const;
	int Compare(const wchar* _pStr) const;
	int Compare(const CStrBase& _s) const;
	int CompareNoCase(const char* _pStr) const;
	int CompareNoCase(const wchar* _pStr) const;
	int CompareNoCase(const CStrBase& _s) const;

	friend int operator==(const CStrBase &x, const char* s);
	friend int operator!=(const CStrBase &x, const char* s);
	friend int operator==(const CStrBase &x, const CStrBase& y);
	friend int operator!=(const CStrBase &x, const CStrBase& y);
	friend int operator<(const CStrBase& x, const CStrBase& y);
	friend int operator>(const CStrBase& x, const CStrBase& y);
	friend int operator<=(const CStrBase& x, const CStrBase& y);
	friend int operator>=(const CStrBase& x, const CStrBase& y);

	int GetNumMatches(char ch) const;					// Count matches of chars.
	int GetNumMatches(const char* _pSubStr) const;		// Count matches of sub-strings.
	int Find(const char* _pSubStr) const;				// ret. -1 om str. inte finns.
	int FindReverse(const char* _pSubStr) const;		// ret. -1 om str. inte finns.
	int FindOneOf(const char* _pStrCharSet) const;		// ret. -1 om str. inte finns.
	int FindFrom(int _iStart, const char* _pSubStr) const;
	template<class T2> int CompareSubStr(const T2* _pSubStr, int _Pos = 0) const;
	int CompareSubStr(const CStrBase& _SubStr, int _Pos = 0) const;

	void Trim();
	void MakeUpperCase();
	void MakeLowerCase();
	bool IsNumeric();

	int GetIntSep(const char* _pSeparator);				// Single sep.
	fp64 Getfp64Sep(const char* _pSeparator);
	int GetIntMSep(const char* _pSeparators);			// Multi sep.
	fp64 Getfp64MSep(const char* _pSeparators);

	static bool IsWhiteSpace(char _ch);
	static bool IsWhiteSpace(wchar _ch);
	static bool IsDigit(char _ch);
	static bool IsDigit(wchar _ch);

	static int SkipADigit(const char* pStr, int pos, int len);
	static int GoToDigit(const char* pStr, int pos, int len);

	static mint StrLen(const char*);
	static mint StrLen(const wchar*);

	static uint32 StrHash(const char*, uint32 _BaseHash = 0);
	uint32 StrHash() const;

	static int TranslateInt(const char* _pVal, const char** _pStrings);
	static int TranslateFlags(const char* _pVal, const char** _pStrings);

	// ASCII to Shift-Jis convertion. Used for memorycards on PS2 and GC
	static uint16 Ascii2Sjis(char _Char);
	static void AsciiString2Sjis(const char *_pAsciiStr, short *_pSjisStr, int _Count);

	int TranslateInt(const char** _pStrings);			// Unicode is not supported
	int TranslateFlags(const char** _pStrings);			// Unicode is not supported

	void CreateFromInt(int _iInt, const char** _pStrings);
	void CreateFromFlags(int _iFlags, const char** _pStrings);
};

template<class T1, class T2> 
int CStrBase::CompareSubStr(const T1* _pStr, int _Pos, const T2* _pSubStr)
{
	int Pos = 0;
	while(*_pStr && Pos < _Pos)
	{
		_pStr++;
		Pos++;
	}

	if (!(*_pStr))
		return -1;

	T1 c1;
	T2 c2;
	do
	{
		c1 = *_pStr;
		c2 = *_pSubStr;
		if (!c2)
			return 0;
		if (c1 < c2)
			return -1;
		else if (c1 > c2)
			return 1;
		_pStr++;
		_pSubStr++;
	}
	while(c1 && c2);

	if (c2)
		return -1;
	else
		return 0;
}

template<class T2> int CStrBase::CompareSubStr(const T2* _pSubStr, int _Pos) const
{
	if (IsAnsi())
		return CompareSubStr(Str(), _Pos, _pSubStr);
	else
		return CompareSubStr(StrW(), _Pos, _pSubStr);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Base class for fast strings
						
	Comments:			No memory allocation.
						No exceptions thrown.
\*____________________________________________________________________*/

class CFStrBase : public CStrBase
{
public:
	virtual int GetMax() const pure;

	virtual char operator[](int i) const;

	virtual void Clear();

	virtual int Len() const;
	virtual void Capture(const char* _pStr);
	virtual void Capture(const char* _pStr, int _Len);
	virtual void CaptureFormated(const char* _pStr, ...);
	virtual void MakeUnique() {};

	CFStrBase();

	void operator=(const char* s);
	void operator+=(const char*);

	void Copy(int _Start, int _Len, char* _pTarget) const;

	void Read(class CCFile* _pFile);
	void Write(class CCFile* _pFile) const;
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	TemplateClass:		Template for defining diffrent length fast strings
						
	Parameters:			
		TMaxLen:		The maximum number of characters the string can contain
\*____________________________________________________________________*/

template<int tMaxLen>
class TFStr : public CFStrBase
{
	char m_Str[tMaxLen]; // The string data. Private use const char * operator
public:
	enum {TMaxLen = tMaxLen};
	virtual int GetMax() const { return TMaxLen; };
	virtual operator char* () { return m_Str; };
	virtual operator const char* () const { return m_Str; };
	virtual char* GetStr() { return m_Str; };
	virtual const char* GetStr() const { return m_Str; };
	virtual const char* Str() const { return m_Str; };

	virtual operator wchar*() { return NULL; };
	virtual operator const wchar*() const { return NULL; };
	virtual wchar* GetStrW() { return NULL; };
	virtual const wchar* GetStrW() const { return NULL; };
	virtual const wchar* StrW() const { return NULL; };

	virtual void* StrAny(int _Pos = 0) { return &GetStr()[_Pos]; };
	virtual const void* StrAny(int _Pos = 0) const { return &GetStr()[_Pos]; };

	virtual int IsAnsi() const { return true; };
	virtual int IsUnicode() const { return false; };

	int operator==(const char* s)
	{
		return CStrBase::Compare(*this, s) == 0;
	}

	int operator!=(const char* s)
	{
		return CStrBase::Compare(*this, s) != 0;
	}

	int operator==(const CStrBase& _Str)
	{
		return CStrBase::Compare(*this, _Str) == 0;
	}

	int operator!=(const CStrBase& _Str)
	{
		return CStrBase::Compare(*this, _Str) != 0;
	}

	int operator<(const CStrBase& _Str)
	{
		return CStrBase::Compare(*this, _Str) == -1;
	}

	int operator>(const CStrBase& _Str)
	{
		return CStrBase::Compare(*this, _Str) == 1;
	}

	int operator<=(const CStrBase& _Str)
	{
		return CStrBase::Compare(*this, _Str) <= 0;
	}

	int operator>=(const CStrBase& _Str)
	{
		return CStrBase::Compare(*this, _Str) >= 0;
	}


	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Function:			Default constructor
	\*____________________________________________________________________*/
	TFStr()
	{
		m_Str[0] = 0;
	}


	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Function:			Constructor setting the first character to ch
							
		Parameters:			
			ch:				A character set at the first byte of the string
	\*____________________________________________________________________*/
	TFStr(char ch)
	{
		m_Str[0] = ch;
		m_Str[1] = 0;
	}


	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Function:			Constructor for filling a string with a specified character
							
		Parameters:			
			ch:				Character to fill string with
			n:				The number of bytes to fill string with
	\*____________________________________________________________________*/
	TFStr(char ch, int n)
	{
		n = Min(TMaxLen-1, n);
		memset(m_Str, ch, n);
		m_Str[n] = 0;
	}


	TFStr(const char* x)
	{
		Capture(x);
	}

	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Function:			Constructor for converting from "regular" strings.
							Will truncate if necessary.
							
	\*____________________________________________________________________*/
	TFStr( const CStrBase &_Other)
	{
		Capture(_Other.GetStr(), _Other.Len());
	}

	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Function:			Copys a other TFStr
							
		Parameters:			
			_s:				The other TFStr
	\*____________________________________________________________________*/
	TFStr(const TFStr& _s)
	{
		strncpy(m_Str, _s.m_Str, TMaxLen);
		m_Str[TMaxLen-1] = 0;
	}

	TFStr& operator= (const TFStr& _s)
	{
		strncpy(m_Str, _s.m_Str, TMaxLen);
		m_Str[TMaxLen-1] = 0;
		return *this;
	}

	M_FORCEINLINE bool IsEmpty() const
	{
		return (m_Str[0] == 0);
	}

//	friend TFStr operator+ (const TFStr& _s1, const TFStr& _s2);
	TFStr operator+ (const CStrBase& _Other)
	{
		int l1 = Len();
		int l2 = Min(GetMax()-1-l1, _Other.Len());
		TFStr<TMaxLen> Ret;
		char* p = Ret.GetStr();
		memcpy(&p[0], Str(), l1);
		memcpy(&p[l1], _Other.Str(), l2);
		p[l1+l2] = 0;
		return Ret;
	}

	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Function:			Gets a part of the string
							
		Parameters:			
			start:			Where to start getting the string
			len:			The number of characters to get
							
		Returns:			The copied string
	\*____________________________________________________________________*/
	TFStr Copy(int start, int len) const
	{
		TFStr Ret;
		CFStrBase::Copy(start, Min(len, Ret.GetMax()-1), Ret.GetStr());
		return Ret;
	}

	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Function:			Copies a part of the string
							
		Parameters:			
			start:			Where to start copying
							
		Returns:			The copied string
	\*____________________________________________________________________*/
	TFStr CopyFrom(int start) const
	{
		TFStr Ret;
		CFStrBase::Copy(start, Ret.GetMax()-1, Ret.GetStr());
		return Ret;
	}

	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Function:			Gets the first len characters of the string
							
		Parameters:			
			len:			The number of charcters to get
							
		Returns:			A copy of the characters
	\*____________________________________________________________________*/
	TFStr Left(int len) const
	{
		TFStr Ret;
		int l = Len();
		if (len > l) len = l;
		CFStrBase::Copy(0, Min(len, Ret.GetMax()-1), Ret.GetStr());
		return Ret;
	}

	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Function:			Gets the len number of characters from the end of the string
							
		Parameters:			
			len:			The number of characters to get
							
		Returns:			A copy of the characters
	\*____________________________________________________________________*/
	TFStr Right(int len) const
	{
		TFStr Ret;
		int l = Len();
		if (len > l) len = l;
		CFStrBase::Copy(l-len, Min(len, Ret.GetMax()-1), Ret.GetStr());
		return Ret;
	}

	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Function:			Deletes characters from the string
							
		Parameters:			
			start:			The first character to delete
			len:			The number of characters to get
							
		Returns:			A copy of the characters
	\*____________________________________________________________________*/
	TFStr Del(int start, int len)
	{
		TFStr Tmp;
		int orglen = Len();

		if (len < 0) len = 0;
		if (start < 0) 
		{ 
			len += start; 
			start = 0; 
		};
		if (len > orglen) len = orglen;
		if (start > orglen) start = orglen;
		if (start+len > orglen) len = orglen-start;

		if (orglen-len > 0)
		{
			char* p = Tmp.GetStr();
			memcpy(&p[0], Str(), start);
			memcpy(&p[start], &Str()[start+len], orglen-start-len);
			p[orglen-len]=0;
		}
		return Tmp;
	}

	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Function:			Inserts characters at pos
							
		Parameters:			
			pos:			The position to insert the string
			p:				The character to insert
							
		Returns:			A copy of the string
	\*____________________________________________________________________*/
	TFStr Insert(int pos, const char* p) const
	{
		TFStr Ret;
		int l = Len();
		int l2 = StrLen(p);
		if (pos < 0) pos = 0;
		if (pos >= l) pos = l;

		int l1 = pos;
		int t2 = Min(GetMax()-1-l1, l2);
		int t3 = Min(GetMax()-1-l1-t2, l-l1-t2);

		char* ps = Ret.GetStr();
		char* pd = Ret.GetStr();
		memcpy(&pd[0], &ps[0], l1);
		memcpy(&pd[pos], &p[0], t2);
		memcpy(&pd[pos+t2], &ps[pos], t3);
		pd[l1+t2+t3] = 0;

		return Ret;
	}

	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Function:			Converts the string to upper case
							
		Returns:			A copy of the string in upper case
	\*____________________________________________________________________*/
	TFStr UpperCase() const
	{
		TFStr s(*this);
		s.MakeUpperCase();
		return s;
	}

	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Function:			Converts the string to lower case
							
		Returns:			A copy of the string in lower case
	\*____________________________________________________________________*/
	TFStr LowerCase() const
	{
		TFStr s(*this);
		s.MakeLowerCase();
		return s;
	}

	TFStr GetStrSep(const char* _pSeparator)
	{
		int i = Find(_pSeparator);
		int sl = StrLen(_pSeparator);

		if (i >= 0)
		{
			TFStr Tmp;
			Tmp.Capture(GetStr(), i);
			int l = Len();
			char* p = GetStr();
			for(int j = 0; j < l-i-sl; j++)
				p[j] = p[j+i+sl];
			p[l-i-sl] = 0;
			return Tmp;
		}
		else
		{
			TFStr Tmp(*this);
			Clear();
			return Tmp;
		}
	}

	TFStr GetStrMSep(const char* _pSeparators)
	{
		int i = FindOneOf(_pSeparators);
		int sl = 1;

		if (i >= 0)
		{
			TFStr Tmp;
			Tmp.Capture(GetStr(), i);
			int l = Len();
			char* p = GetStr();
			for(int j = 0; j < l-i-sl; j++)
				p[j] = p[j+i+sl];
			p[l-i-sl] = 0;
			return Tmp;
		}
		else
		{
			TFStr Tmp(*this);
			Clear();
			return Tmp;
		}
	}

	static TFStr GetFilteredString(fp64 _f, int _iDecimals = -1)
	{
		TFStr s;
		switch(_iDecimals)
		{
		case 0:	s.CaptureFormated("%.0f", _f); break;
		case 1:	s.CaptureFormated("%.1f", _f); break;
		case 2:	s.CaptureFormated("%.2f", _f); break;
		case 3:	s.CaptureFormated("%.3f", _f); break;
		case 4:	s.CaptureFormated("%.4f", _f); break;
		case 5:	s.CaptureFormated("%.5f", _f); break;
		case 6:	s.CaptureFormated("%.6f", _f); break;
		default: s.CaptureFormated("%f", _f); break;
		}

		int dot = s.Find(".");
		if (dot >= 0)
		{
			int l = s.Len();
			int i;
			for(i = l-1; s[i] == '0'; i--)
			{
			}
			if (i == dot) i--;
			s = s.Copy(0, i+1);
		}
		return s;
	}

	TFStr GetFilename() const
	{
		MAUTOSTRIP(CStr_GetFilename, CStr());
		int o = FindReverse("/");
		int p = FindReverse("\\");
		if(o > p) p = o;
		if (p < 0) p = FindReverse(":");
		return Copy(p+1, Len());
	}

};


typedef TFStr<252> CFStr;
typedef TFStr<28> CFStr28;

// -------------------------------------------------------------------
//  CStr
// -------------------------------------------------------------------
class CStr : public CStrBase
{
public:
	// -------------------------------------------------------------------
	class CStrData			// 2 bytes + sizeof(chartype)*strlen
	{
	public:
		uint16 m_Data;
//		uint16 m_MRTC_ReferenceCount : 13;
//		uint16 m_bIsEmpty : 1;
//		uint16 m_bIsAllocated : 1;				// If true, the str has not been allocated through the memory manager. (Used for CRegistryConst, for ex.)
//		uint16 m_bIsUnicode : 1;

		M_INLINE uint16 f_MRTC_ReferenceCount() const
		{
			return (m_Data & DBitRange(0, 12)) >> 0;
		}
		M_INLINE void f_MRTC_ReferenceCount(uint16 _Data)
		{
			m_Data = (m_Data & (~DBitRangeTyped(0, 12, uint16))) | (_Data << 0);
		}

		M_INLINE uint16 f_bIsEmpty() const
		{
			return (m_Data & DBitRange(13, 13)) >> 13;
		}
		M_INLINE void f_bIsEmpty(uint16 _Data)
		{
			m_Data = (m_Data & (~DBitRangeTyped(13, 13, uint16))) | ((_Data&1) << 13);
		}

		M_INLINE uint16 f_bIsAllocated() const
		{
			return (m_Data & DBitRange(14, 14)) >> 14;
		}
		M_INLINE void f_bIsAllocated(uint16 _Data)
		{
			m_Data = (m_Data & (~DBitRangeTyped(14, 14, uint16))) | ((_Data&1) << 14);
		}

		M_INLINE uint16 f_bIsUnicode() const
		{
			return (m_Data & DBitRange(15, 15)) >> 15;
		}
		M_INLINE void f_bIsUnicode(uint16 _Data)
		{
			m_Data = (m_Data & (~DBitRangeTyped(15, 15, uint16))) | ((_Data&1) << 15);
		}

		int MRTC_AddRef();
		int MRTC_DelRef();
		int MRTC_ReferenceCount() const;
		void MRTC_Delete();

		mint GetNumAlloc() const;

		int IsEmpty() const { return f_bIsEmpty(); };
		int IsAnsi() const { return f_bIsUnicode() == 0; };
		int IsUnicode() const { return f_bIsUnicode(); };
		int GetFmt() const { return (IsAnsi()) ? CSTR_FMT_ANSI : CSTR_FMT_UNICODE; };

		char* Str() { return (!IsEmpty() && !IsUnicode()) ? (char*)(this+1) : NULL; };
		char* Str() const { return (!IsEmpty() && !IsUnicode()) ? (char*)(this+1) : NULL; };
		wchar* StrW() { return (!IsEmpty() && IsUnicode()) ? (wchar*)(this+1) : NULL; };
		wchar* StrW() const { return (!IsEmpty() && IsUnicode()) ? (wchar*)(this+1) : NULL; };

		void* StrData() { return !IsEmpty() ? (wchar*)(this+1) : NULL; };
		void* StrData() const { return !IsEmpty() ? (wchar*)(this+1) : NULL; };

		void* StrAny(int _Pos = 0)
		{
			if (IsUnicode())
				return &((wchar*)(this+1))[_Pos];
			else
				return &((char*)(this+1))[_Pos];
		};

		void* StrAny(int _Pos = 0) const
		{
			if (IsUnicode())
				return &((wchar*)(this+1))[_Pos];
			else
				return &((char*)(this+1))[_Pos];
		};

		// Non-ambigous debug versions
		const char* DStr() const { return Str(); };
		const wchar* DStrW() const { return StrW(); };
		const void* DStrAny() const { return StrAny(0); };

		CStrData()
		{
			f_MRTC_ReferenceCount(0);
			f_bIsUnicode(0);
		};
		
		~CStrData()
		{
		};

		static CStrData* Create(int _Len, int _Format = CSTR_FMT_ANSI);
		static CStrData* CreateW(int _Len);

		mint Len() const;
	};
	
	typedef TPtr2<CStrData, CStrData> spCStrData;

	spCStrData &GetStrData() const
	{
		return const_cast<spCStrData &>(m_spD);
	}

private:	
	// -------------------------------------------------------------------
	
	spCStrData m_spD;
	
	public:
		virtual char operator[](int i) const;
		virtual operator char*();							// Returns NULL on empty strings
		virtual operator const char*() const;				// Returns NULL on empty strings
		virtual char* GetStr();								// Returns NULL on empty strings
		virtual const char* GetStr() const;					// Returns NULL on empty strings
		virtual const char* Str() const;					// Always return a valid string.

		const char* GetString() const
		{
			return GetStr();
		}

		const wchar* GetStringW() const
		{
			return GetStrW();
		}

		virtual operator wchar*();							// Returns NULL on empty strings
		virtual operator const wchar*() const;				// Returns NULL on empty strings
		virtual wchar* GetStrW();							// Returns NULL on empty strings
		virtual const wchar* GetStrW() const;				// Returns NULL on empty strings
		virtual const wchar* StrW() const;					// Always return a valid string.

		virtual const void* StrData() const;					// Always return a valid string.

		virtual void* StrAny(int _Pos = 0);					// Returns NULL on empty strings
		virtual const void* StrAny(int _Pos = 0) const;		// Returns NULL on empty strings

		// Non-ambigous debug versions
		const char* DStr() const { return Str(); };
		const wchar* DStrW() const { return StrW(); };
		const void* DStrAny() const { return StrAny(0); };

		virtual void SetChar(int _Pos, int _Char);			// _Char is truncated to the current format of the string. Positions outside the string are ignored.

		virtual int IsAnsi() const;
		virtual int IsUnicode() const;
		virtual int IsPureAnsi() const;
		virtual CStr Ansi() const;							// No memory allocation if source string is ansi
		virtual CStr Unicode() const;						// No memory allocation if source string is unicode

	protected:
		spCStrData CreateUniqueStr(int _Len, CStrData* _pCurrent = NULL);
		spCStrData CreateUniqueStrW(int _Len, CStrData* _pCurrent = NULL);
		void SetCharNoCheck(int _Pos, int _Char);			// No error checking!, assumes _Pos is within range and all pointers are valid
		static int GetCombineFmt(const CStr& _s0, const CStr& _s1);

	public:
		virtual void Clear();

		virtual int Len() const;
		virtual void Capture(const char* _pStr);
		virtual void Capture(const char* _pStr, int _Len);
		virtual void CaptureFormated(const char* _pStr, ...);
		virtual void Capture(const wchar* _pStr);
		virtual void Capture(const wchar* _pStr, int _Len);
		virtual void CaptureFormated(const wchar* _pStr, ...);
		virtual void Capture(const void* _pStr, int _Len, int _Fmt);
		virtual void MakeUnique();

		friend int operator ==(const CStr &x, const char* s);
		friend int operator !=(const CStr &x, const char* s);
		friend int operator ==(const CStr &x, const CStr& y);
		friend int operator !=(const CStr &x, const CStr& y);
		friend int operator <(const CStr& x, const CStr& y);
		friend int operator >(const CStr& x, const CStr& y);
		friend int operator <=(const CStr& x, const CStr& y);
		friend int operator >=(const CStr& x, const CStr& y);

		CStr();
//		CStr(const char*, ...);
//		CStr(const wchar*, ...);
		CStr(const char*);
		CStr(const wchar*);
		CStr(char ch);
		CStr(wchar ch);
		CStr(char ch, int n);
		CStr(wchar ch, int n);
		CStr(const CStrBase&);
		CStr(const CStr&);
		CStr(CStrData*);									// Advanced usage. You'd better keep an eye on the refcounts and smartpointers.
		CStr& operator=(const char *s)
		{
			MAUTOSTRIP(CStr_operator_assign, *this);
			Capture(s);
			return *this;
		}
		CStr& operator=(const wchar *s)
		{
			MAUTOSTRIP(CStr_operator_assign_2, *this);
			Capture(s);
			return *this;
		}
		CStr& operator=(const CStrBase&);
		CStr& operator=(const CStr&);
		~CStr();

		M_FORCEINLINE bool IsEmpty() const
		{
			const CStrData* pData = m_spD.p;
			return (!pData || pData->f_bIsEmpty());
		}

		CStr& operator+=(const CStr&);
		friend CStr operator+(const CStr& _s1, const CStr& _s2);

		void ReplaceChar(wchar _Find, wchar _Replace)
		{
			MakeUnique();
			if (m_spD)
			{
				if (IsUnicode())
					StrReplaceChar(GetStrW(), _Find, _Replace);
				else
					StrReplaceChar(GetStr(), (char)_Find, (char)_Replace);
			}
		}
		CStr Copy(int start, int len) const;
		CStr Left(int len) const;
		CStr Right(int len) const;
		CStr Del(int start, int len) const;
		CStr Insert(int pos, const CStr &x) const;
		CStr CopyFrom(int start) const
		{
			MAUTOSTRIP(CStr_CopyFrom, CStr());
			return Copy(start, 100000000);
		}
		
		CStr SubStr(int pos, int len) const
		{
			return Copy(pos, len);
		}
		CStr LeftTo(int pos) const // Inclusive
		{
			return Left(pos + 1);
		}
		CStr RightFrom(int pos) const // Inclusive
		{
			return Right(Len() - pos);
		}
		CStr DelTo(int pos) const // Inclusive
		{
			return Del(0, pos + 1);
		}
		CStr DelFrom(int pos) const // Inclusive
		{
			return Del(pos, Len() - pos);
		}

		CStr Separate(const char* _pSeparator);
		
		CStr UpperCase() const;								// ret Uppercase of this
		CStr LowerCase() const;								// ret Lowercase of this
		
		CStr GetFilenameExtenstion() const;
		CStr GetFilename() const;
		CStr GetFilenameNoExt() const;
		CStr GetPath() const;
		CStr GetDevice() const;
		CStr ConcatRelativePath() const;					// Unicode is not supported
		
		CStr LTrim() const;									// Remove whitespace from left/right/both
		CStr RTrim() const;
		CStr GetStrSep(const char* _pSeparator);			// One separator.
		CStr GetStrMSep(const char* _pSeparators);			// Multiple single-character separators.
		CStr GetBounded(const char* _pBound);
		
//		int Scan(const char* _pFormat, ...) const;			// Read formated data from string, string must be ANSI
//		int Scan(const wchar* _pFormat, ...) const;			// Read formated data from string, string must be Unicode
		char *GetBuffer(int _Len);
		wchar *GetBufferW(int _Len);

		
		static CStr GetFilteredString(fp64 _f, int _iDecimals = -1);

		void ReadFromFile( CStr _Filename );
		void WriteToFile( CStr _Filename ) const;

		void Read(class CCFile* _pFile);
		void Write(class CCFile* _pFile) const;
		void DummyRead(class CCFile* _pFile) const;
};

CStr CStrF(const char*, ...);
CStr CStrF(const wchar*, ...);
CFStr CFStrF(const char*, ...);

#include "MRTC_StringHash.h"


