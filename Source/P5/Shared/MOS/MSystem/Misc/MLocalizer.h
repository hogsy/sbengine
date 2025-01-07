
#ifndef __INC_MLOCALIZER

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			String localization
					
	Author:			Magnus Högdahl
					
	Copyright:		Starbreeze Studios AB 2001
					
	Comments:		This file contains a set of functions to substitute ("localize")
					string keys using the application-global string table(s). Please
					note that this functionality can be used for other things than
					just localization, for example character names, locations, company
					names, dates, etc, so when a change is needed, only a single string
					needs to be edited rather than all occurances of that string. 
					It is simply a string key ("alias") substitution system.

					In the string "§LKEYNAME", "§L" is a token signaling a 
					string key, and KEYNAME is the name of the key. If more text
					is desired after the keyname it must be terminated with §pq.
					For ex. "§LKEYNAME§pq More Text §LANOTHERKEY§pq more text."

					Parameter example:

					String: "§LCOPY§p02001§p1Starbreeze Studios AB§pq"
					Key:	*COPY "Copyright §p0, §p1"

					Result:	"Copyright 2001, Starbreeze Studios AB"

					"§px", where x is an integer between 0 to 9, is a token following 
					a string key for defining key-parameters. The last parameter
					must be terminated with "§pq". The parameters can be inserted 
					anywhere in the key using the same §px token.

					String keys can contain other string-keys. A string is substituted
					until no string keys are present in the result string.

					You _can_ create infinite loops by referring to either the same 
					string key, or by other means referring to a key previously 
					substituted in the string. In such cases the localizer will either 
					lock-up or crash. (for ex: *COPY "§LCOPY§LCOPY")

					
	History:		
		010727:		Created File
\*____________________________________________________________________________________________*/

#include "MCC.h"
#include "../MSystem.h"

// These are used internally, exported for "just in case"
void SYSTEMDLLEXPORT M_CDECL Localize_GetKey(const wchar*& _pS, CFStr& _Dst);
CStr SYSTEMDLLEXPORT M_CDECL Localize_FindKeyValue(const char* _pKey);
bool SYSTEMDLLEXPORT M_CDECL Localize_KeyExists(const char* _pKey);
int SYSTEMDLLEXPORT M_CDECL Localize_GetParams(const wchar*& _pS, const wchar** _plpParams, int* _plParamLen);
void SYSTEMDLLEXPORT M_CDECL Localize_SubstituteKeys(const wchar* _pSrc, wchar* _pDst, int _MaxLen);

// Localize_HasKey() can be used to determine if a string needs to be processed by Localize_Str
bool SYSTEMDLLEXPORT M_CDECL Localize_HasKey(const char* _pStr);
bool SYSTEMDLLEXPORT M_CDECL Localize_HasKey(const wchar* _pStr);
bool SYSTEMDLLEXPORT M_CDECL Localize_HasKey(CStr _Str);

// These are the functions you should use
void SYSTEMDLLEXPORT M_CDECL Localize_Str(const wchar* _pSrc, wchar* _pDst, int _MaxLen);
void SYSTEMDLLEXPORT M_CDECL Localize_Str(const char* _pSrc, wchar* _pDst, int _MaxLen);
void SYSTEMDLLEXPORT M_CDECL Localize_Str(CStr _Str, wchar* _pDst, int _MaxLen);
CStr SYSTEMDLLEXPORT M_CDECL Localize_Str(CStr _Str);

#endif
