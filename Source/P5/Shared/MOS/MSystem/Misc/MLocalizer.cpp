#include "PCH.h"
#include "MLocalizer.h"
#include "MRegistry.h"

#ifdef PLATFORM_DOLPHIN
 #include <wstring.h>
 using namespace std;
#endif

// #define LOCALIZER_DISABLE

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Extracts a localization key and converts it
						to ANSI.
						
	Parameters:			
		_pS:			Source string
		_Dst:			Target string						

	Comments:			Used internally by Localize_Str.
\*____________________________________________________________________*/
void SYSTEMDLLEXPORT M_CDECL Localize_GetKey(const wchar*& _pS, CFStr& _Dst)
{
	MAUTOSTRIP(Localize_GetKey, MAUTOSTRIP_VOID);

	const wchar* pS = _pS;
	while(*pS && *pS != wchar((uint8)'§') && *pS != wchar((uint8)' ') && *pS != wchar((uint8)'\n') && *pS != wchar((uint8)':') && *pS != wchar((uint8)'.') && *pS != wchar((uint8)',') && *pS != wchar((uint8)')'))
		pS++;

	int Len = pS - _pS;
	if (Len >= _Dst.GetMax())
		Error_static("Localize_GetKey", CStrF(WTEXT("Too long localization key name (%ls)"), _pS));

	CStrBase::mfsncpy(_Dst.GetStr(), CSTR_FMT_ANSI, _pS, CSTR_FMT_UNICODE, Len);
	_Dst.GetStr()[Len] = 0;

	_pS += Len;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Performs a look up of a localization key
						in the application-global string tables.
						
	Parameters:			
		_pKey:			Key to look up.
						
	Returns:			Localization key value

	Comments:			Used internally by Localize_Str.
\*____________________________________________________________________*/
CStr SYSTEMDLLEXPORT M_CDECL Localize_FindKeyValue(const char* _pKey)
{	
	MAUTOSTRIP(Localize_FindKeyValue, CStr());

	MACRO_GetSystemRegistry(pReg);
	if (!pReg)
		return CStr(_pKey);

	CRegistry* pSLContainer = pReg->FindChild("STRINGTABLES");
	if (!pSLContainer)
		return CStr(_pKey);

	// Loop backwards to allow new stringtables to override previously added stringtables.
	for(int iSL = pSLContainer->GetNumChildren()-1; iSL >= 0; iSL--)
	{
		CRegistry* pSL = pSLContainer->GetChild(iSL);
		CRegistry* pString = pSL->FindChild(_pKey);
		if (pString)
			return pString->GetThisValue();
	}

	return CStr(_pKey);

/*	REGISTRY
		ENV
		STRINGTABLES
			SYSTEM
			GAMECONTEXT_ENG
			GAMECONTEXT_JAP
*/					
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Performs a look up of a localization key
						in the application-global string tables.
						
	Parameters:			
		_pKey:			Key to look up.
						
	Returns:			True if key exists
\*____________________________________________________________________*/
bool SYSTEMDLLEXPORT M_CDECL Localize_KeyExists(const char* _pKey)
{
	MAUTOSTRIP(Localize_KeyExists, false);

	MACRO_GetSystemRegistry(pReg);
	if (!pReg)
		return false;

	CRegistry* pSLContainer = pReg->FindChild("STRINGTABLES");
	if (!pSLContainer)
		return false;

	// Loop backwards to allow new stringtables to override previously added stringtables.
	for(int iSL = pSLContainer->GetNumChildren()-1; iSL >= 0; iSL--)
	{
		CRegistry* pSL = pSLContainer->GetChild(iSL);
		CRegistry* pString = pSL->FindChild(_pKey);
		if (pString)
			return true;
	}

	return false;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Extracts addresses and lengths of localization 
						key parameters.
						
	Parameters:			
		_pS:			Source string
		_plpParams:		Target parameter pointer array
		_plParamLen:	Target parameter length array
						
	Returns:			Number of parameters found.

	Comments:			Used internally by Localize_Str.
\*____________________________________________________________________*/
int SYSTEMDLLEXPORT M_CDECL Localize_GetParams(const wchar*& _pS, const wchar** _plpParams, int* _plParamLen)
{
	MAUTOSTRIP(Localize_GetParams, 0);

	int nParams = 0;
	while(_pS[0] == wchar((uint8)'§') &&
		_pS[1] == wchar((uint8)'p') &&
		_pS[2] != 0)
	{
		_pS += 3;
		if (_pS[-1] == 'q')
			break;

		const wchar* pEnd = CStrBase::wcsstr(_pS, WTEXT("§p"));
		int Len = (pEnd) ? pEnd - _pS : CStrBase::StrLen(_pS);

		_plpParams[nParams] = _pS;
		_plParamLen[nParams] = Len;
		nParams++;

		_pS += Len;
	}

	return nParams;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Substitute localization keys using the application-
						global string tables.
						
	Parameters:			
		_pSrc:			Source format string
		_pDst:			Target string buffer
		_MaxLen:		Size of target string buffer
						
	Comments:			No memory allocation is performed.

						IMPORTANT: MAXLEN IS CURRENTLY IGNORED!
\*____________________________________________________________________*/
void SYSTEMDLLEXPORT M_CDECL Localize_SubstituteKeys(const wchar* _pSrc, wchar* _pDst, int _MaxLen)
{
	MAUTOSTRIP(Localize_SubstituteKeys, MAUTOSTRIP_VOID);

	const wchar* pS = _pSrc;
	wchar *pD = _pDst;

	for(;*pS;)
	{
		if (pD - _pDst >= _MaxLen-1)
			break;

		if (*pS == wchar((uint8)'§'))
		{
			pS++;
			wchar Cmd = *pS;

			switch(Cmd)
			{
			case 'L' :
				{
					// Localization string key
					pS++;
					CFStr Key;
					Localize_GetKey(pS, Key);
					CStr KeyVal = Localize_FindKeyValue(Key);

					const wchar* lpParams[16];
					int lParamLen[16];
					int nParams = Localize_GetParams(pS, &lpParams[0], &lParamLen[0]);

					if (KeyVal.IsUnicode())
					{
						// Localization key value is in UNICODE format
						const wchar* pKeyVal = KeyVal.StrW();
						while(*pKeyVal)
						{
							if (pKeyVal[0] == wchar((uint8)'§') &&
								pKeyVal[1] == wchar((uint8)'p') &&
								pKeyVal[2] >= wchar('0') && pKeyVal[2] <= wchar('9') &&
								nParams > 0)
							{
								int iParam = Min(Max(pKeyVal[2] - wchar((uint8)'0'), 0), 9);
								pKeyVal += 3;
								if (iParam < nParams)
								{
									CStrBase::mfsncpy(pD, CSTR_FMT_UNICODE, lpParams[iParam], CSTR_FMT_UNICODE, lParamLen[iParam]);
									pD += lParamLen[iParam];
								}
							}
							else
							{
								const wchar* pPos = CStrBase::wcsstr(pKeyVal+1, WTEXT("§"));
								int nCopy = (pPos) ? (pPos - pKeyVal) : CStrBase::StrLen(pKeyVal);
								CStrBase::mfsncpy(pD, CSTR_FMT_UNICODE, pKeyVal, CSTR_FMT_UNICODE, nCopy);
								pD += nCopy;
								pKeyVal += nCopy;
							}
						}
					}
					else
					{
						// Localization key value is in ANSI format
						const char* pKeyVal = KeyVal.Str();
						while(*pKeyVal)
						{
							char moo = '§';
							if (pKeyVal[0] == moo &&
								pKeyVal[1] == 'p' &&
								pKeyVal[2] >= '0' && pKeyVal[2] <= '9' &&
								nParams > 0)
							{
								int iParam = Min(Max(pKeyVal[2] - '0', 0), 9);
								pKeyVal += 3;
								if (iParam < nParams)
								{
									CStrBase::mfsncpy(pD, CSTR_FMT_UNICODE, lpParams[iParam], CSTR_FMT_UNICODE, lParamLen[iParam]);
									pD += lParamLen[iParam];
								}
							}
							else
							{
								char* pPos = strchr((char *)pKeyVal+1, moo);
								int nCopy = (pPos) ? (pPos - pKeyVal) : CStrBase::StrLen(pKeyVal);
								CStrBase::mfsncpy(pD, CSTR_FMT_UNICODE, pKeyVal, CSTR_FMT_ANSI, nCopy);
								pD += nCopy;
								pKeyVal += nCopy;
							}
						}
					}
					
				}
				break;

			case 'p' :
			case 'P' :
				{
					// We should not get here unless the syntax is erronous.
					pS++;
				}
				break;
			case 'R' :
				{
					MACRO_GetSystemRegistry(pReg);
					if (pReg)
					{
						++pS;
						char TempStr[256];
						char *pTmp = TempStr;
						while (*pS && pTmp < (TempStr + 255))
						{
							if (*pS == L'*')
							{
								++pS;
								break;
							}
							*pTmp = *pS;
							++pTmp;
							++pS;
						}
						*pTmp = 0;

						CRegistry *pRegistry = pReg->Find(TempStr);

						if (pRegistry)
						{
							CStr Str = pRegistry->GetThisValue();
							if (Str.IsAnsi())
								CStrBase::mfscpy(pD, CSTR_FMT_UNICODE, (const char *)Str, CSTR_FMT_ANSI);
							else
								CStrBase::wcscpy(pD, (wchar *)Str);

							pD += Str.Len();
						}
						else
						{
							const wchar *pStr = WTEXT("NOVAL");
							CStrBase::wcscpy(pD, pStr); pD += CStr::StrLen(pStr);
						}
                        
						break;
					}
				}
			default :
				{
					// This command is not recognized by the localizer, just write it back to the destination string.
					*pD++ = (uint8)'§';
					*pD++ = Cmd;
					pS++;
				}
			}
		}
		else
		{
			// Just copy the char to destination.
			*pD++ = *pS++;
		}
	}

	// Terminate
	*pD++ = 0;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Returns true if a string needs localization.						
\*____________________________________________________________________*/
bool SYSTEMDLLEXPORT M_CDECL Localize_HasKey(const char* _pStr)
{
	MAUTOSTRIP(Localize_HasKey, false);
	const char *pFind = _pStr;
	while (1)
	{
		switch(*pFind)
		{
		case 0xa7:	//§ char
			{
				char ch = *pFind;
				if (ch == 'L' || ch == 'R')
					return true;
			}
			break;
		case 0:
			return false;
		}
		++pFind;
	}
	return false;
	
//	return strstr(_pStr, "§L") != NULL;
}

bool SYSTEMDLLEXPORT M_CDECL Localize_HasKey(const wchar* _pStr)
{
	MAUTOSTRIP(Localize_HasKey, false);
	const wchar *pFind = _pStr;
	while (1)
	{
		switch(*pFind)
		{
		case 0x00a7:	//§ char
			{
				wchar ch = pFind[1];
				if (ch == L'L' || ch == L'R')
					return true;
				break;
			}
		case 0:
			return false;
		}
		pFind++;
	}
	return false;
	
//	return CStrBase::wcsstr(_pStr, WTEXT("§L")) != NULL;
}

bool SYSTEMDLLEXPORT M_CDECL Localize_HasKey(CStr _Str)
{
	MAUTOSTRIP(Localize_HasKey, false);

	if (_Str.IsAnsi())
		return Localize_HasKey(_Str.Str());
	else
		return Localize_HasKey(_Str.StrW());
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Localize unicode string by substituting
						localization keys until there are none
						present in the result string.
\*____________________________________________________________________*/
void SYSTEMDLLEXPORT M_CDECL Localize_Str(const wchar* _pSrc, wchar* _pDst, int _MaxLen)
{
	MAUTOSTRIP(Localize_Str, MAUTOSTRIP_VOID);

#ifdef LOCALIZER_DISABLE
	CStrBase::mfscpy(_pDst, CSTR_FMT_UNICODE, _pSrc, CSTR_FMT_UNICODE);
#else
	wchar Buffer[1024];
	_MaxLen = Min(1023, _MaxLen);

	wchar* Buffer0 = const_cast<wchar*>(_pSrc);
	wchar* Buffer1 = _pDst;

	while(Localize_HasKey(Buffer0))
	{
		Localize_SubstituteKeys(Buffer0, Buffer1, _MaxLen);

		Swap(Buffer0, Buffer1);
		if (Buffer1 == _pSrc)
			Buffer1 = Buffer;
	}

	if(Buffer1 == _pDst)
		CStrBase::mfscpy(_pDst, CSTR_FMT_UNICODE, Buffer0, CSTR_FMT_UNICODE);
#endif
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Converts localization key to wchar and then 
						calls the wchar counterpart of this function.												
\*____________________________________________________________________*/
void SYSTEMDLLEXPORT M_CDECL Localize_Str(const char* _pSrc, wchar* _pDst, int _MaxLen)
{
	MAUTOSTRIP(Localize_Str, MAUTOSTRIP_VOID);

	wchar Buffer[1024];

	mint Len = MinMT(1024-1, CStrBase::StrLen(_pSrc));
	CStrBase::mfsncpy(Buffer, CSTR_FMT_UNICODE, _pSrc, CSTR_FMT_ANSI, Len);
	Buffer[Len] = 0;

	Localize_Str(Buffer, _pDst, _MaxLen);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Converts localization key to wchar and then 
						calls the wchar counterpart of this function.												
\*____________________________________________________________________*/
void SYSTEMDLLEXPORT M_CDECL Localize_Str(CStr _Str, wchar* _pDst, int _MaxLen)
{
	MAUTOSTRIP(Localize_Str, MAUTOSTRIP_VOID);

	if (_Str.IsAnsi())
		Localize_Str(_Str.Str(), _pDst, _MaxLen);
	else
		Localize_Str(_Str.StrW(), _pDst, _MaxLen);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			A CStr wrapper for Localize_Str	
\*____________________________________________________________________*/
CStr SYSTEMDLLEXPORT M_CDECL Localize_Str(CStr _Str)
{
	MAUTOSTRIP(Localize_Str, CStr());

	wchar Buffer[1024];
	if (_Str.IsAnsi())
		Localize_Str(_Str.Str(), Buffer, 1024);
	else
		Localize_Str(_Str.StrW(), Buffer, 1024);

	CStr Ret;
	Ret.Capture(Buffer);
	return Ret;
}
