
#ifndef M_INCMWIN32REGISTRY_H
#define M_INCMWIN32REGISTRY_H

#include "MCC.h"
#include <Windows.h>
#include "../../MSystem/Misc/MRegistry.h"

enum EWin32RegRoot
{
	 EWin32RegRoot_LocalMachine = 0
	,EWin32RegRoot_CurrentUser = 1
	,EWin32RegRoot_Classes = 2
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:		A helper class for reading and writing to the windows 
				registry
						
	Comments:	Create the class with the root key, and the root path
				you want to manipulate the registry. 
				
				If you want to manipulate the default value of a key 
				send NULL to _pKeyValue.
				
				_pRoot sent to constructor should NOT have \ at the 
				end.
					
\*____________________________________________________________________*/
class CWin32_Registry
{
	HKEY m_RootKey;
	CStr m_Root;

	CStr GetPath(const char *_pKey);
public:
	
	CWin32_Registry(int _Root = EWin32RegRoot_LocalMachine, const char *_pRoot = NULL);

	template <class t_CReturnType>
	t_CReturnType Read(const char *_pSubKey, const char* _pKeyValue)
	{
		TArray<uint8> TempList;

		ReadRegKey(_pSubKey, _pKeyValue, NULL, NULL, &TempList);

		if (TempList.Len() != sizeof(t_CReturnType))
			Error_static("Read_fp32", "Wrong size");

		return *((t_CReturnType *)TempList.GetBasePtr());
	}

#if !defined(_MSC_VER) || (defined(_MSC_VER) && _MSC_VER > 1200)
	template <>
	uint32 Read<uint32>(const char *_pSubKey, const char* _pKeyValue)
	{
		DWORD Temp = 0;
		
		ReadRegKey(_pSubKey, _pKeyValue, &Temp, NULL, NULL);
		
		return Temp;
	}

	template <>
	CStr Read<CStr>(const char *_pSubKey, const char* _pKeyValue)
	{
		CStr Temp;
		
		ReadRegKey(_pSubKey, _pKeyValue, NULL, &Temp, NULL);
		
		return Temp;
	}

	template <>
	TArray<uint8> Read<TArray<uint8> >(const char *_pSubKey, const char* _pKeyValue)
	{
		TArray<uint8> Temp;
		
		ReadRegKey(_pSubKey, _pKeyValue, NULL, NULL, &Temp);
		
		return Temp;
	}
#endif

	uint32 Read_uint32(const char *_pSubKey, const char* _pKeyValue);
	CStr Read_Str(const char *_pSubKey, const char* _pKeyValue);
	TArray<uint8> Read_Bin(const char *_pSubKey, const char* _pKeyValue);
	spCRegistry Read_Registry(const char *_pSubKey);
	
	void ReadRegKey(const char *_pSubKey, const char* _pKeyValue, DWORD *_pDword, CStr *_pStr, TArray<uint8> *_pMemory);
	bool KeyExists(const char *_pSubKey);
	bool ValueExists(const char *_pSubKey, const char *_pKeyValue);

	template <class t_CReturnType>
	void Write(const char *_pSubKey, const char* _pKeyValue, t_CReturnType &_Value)
	{
		TArray<uint8> TempList;
		TempList.SetLen(sizeof(_Value));
		memcpy(TempList.GetBasePtr(), &_Value, sizeof(_Value));
		WriteRegKey(_pSubKey, _pKeyValue, NULL, NULL, &TempList);
	}

	void Write(const char *_pSubKey, const char* _pKeyValue, uint32 _Value);
	void Write(const char *_pSubKey, const char* _pKeyValue, const char *_pValue);
	void Write(const char *_pSubKey, const char* _pKeyValue, CStr &_pValue);
	void Write(const char *_pSubKey, const char* _pKeyValue, TArray<uint8> &_Data);

	void Write(const char *_pSubKey, CRegistry *_pReg);
	
	void WriteRegKey(const char* _pSubKey, const char* _pKeyValue, const DWORD *_pDword, const CStr *_pStr, const TArray<uint8> *_pMemory);	

	void EnumKeyValues(const char *_pSubKey, TArray<CStr> &_lKeys);
	void EnumKeys(const char *_pSubKey, TArray<CStr> &_lKeys);
	void DeleteKeyValue(const char *_pSubKey, const char* _pKeyValue);
	void DeleteKey(const char *_pSubKey);
	void RecursiveDeleteKey(const char *_pSubKey);
};

#endif // M_INCMWIN32REGISTRY_H
