
#include "MWin32Registry.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWin32_Registry
|__________________________________________________________________________________________________
\*************************************************************************************************/


CWin32_Registry::CWin32_Registry(int _Root, const char *_pRoot)
{
	if (_pRoot)
	{
		m_Root.Capture(_pRoot);
	}
	m_RootKey = HKEY_LOCAL_MACHINE;
	switch(_Root)
	{
	case EWin32RegRoot_LocalMachine:
		m_RootKey = HKEY_LOCAL_MACHINE;
		break;
	case EWin32RegRoot_CurrentUser:
		m_RootKey = HKEY_CURRENT_USER;
		break;
	case EWin32RegRoot_Classes:
		m_RootKey = HKEY_CLASSES_ROOT;
		break;
	}
}

CStr CWin32_Registry::GetPath(const char *_pKey)
{
	CStr SubKey;
	if (_pKey)
	{
		if (m_Root.Len())
			SubKey = m_Root + "\\" + _pKey;
		else
			SubKey = _pKey;
	}
	else
		SubKey = m_Root;

	return SubKey;
}


uint32 CWin32_Registry::Read_uint32(const char *_pSubKey, const char* _pKeyValue)
{
	DWORD Temp = 0;
	
	ReadRegKey(_pSubKey, _pKeyValue, &Temp, NULL, NULL);
	
	return Temp;
}

CStr CWin32_Registry::Read_Str(const char *_pSubKey, const char* _pKeyValue)
{
	CStr Temp;
	
	ReadRegKey(_pSubKey, _pKeyValue, NULL, &Temp, NULL);
	
	return Temp;
}

TArray<uint8> CWin32_Registry::Read_Bin(const char *_pSubKey, const char* _pKeyValue)
{
	TArray<uint8> Temp;
	
	ReadRegKey(_pSubKey, _pKeyValue, NULL, NULL, &Temp);
	
	return Temp;
}

bool CWin32_Registry::KeyExists(const char *_pSubKey)
{
	DWORD ValueSize=0;
	HKEY PathKey = NULL;
	CStr SubKey = GetPath(_pSubKey);
	
	try
	{
		if (RegOpenKeyExA(m_RootKey,SubKey,0,KEY_READ,&PathKey) != ERROR_SUCCESS)
		{
			return false;
		}
	}
	catch (CCException)
	{			
		if (PathKey)
			RegCloseKey(PathKey);
		return false;
	}
	if (PathKey)
		RegCloseKey(PathKey);
	return true;
}

bool CWin32_Registry::ValueExists(const char *_pSubKey, const char *_pKeyValue)
{
	DWORD ValueSize=0;
	HKEY PathKey = NULL;
	DWORD ValueType;
	CStr SubKey = GetPath(_pSubKey);
	
	try
	{
		if (RegOpenKeyExA(m_RootKey,SubKey,0,KEY_READ,&PathKey) != ERROR_SUCCESS)
		{
			return false;
		}
		
		if (RegQueryValueExA(PathKey,_pKeyValue,0,&ValueType,NULL,NULL) != ERROR_SUCCESS)
		{
			if (PathKey)
				RegCloseKey(PathKey);
			return false;
		}
	}
	catch (CCException)
	{			
		if (PathKey)
			RegCloseKey(PathKey);
		return false;
	}
	if (PathKey)
		RegCloseKey(PathKey);
	return true;
}

void CWin32_Registry::ReadRegKey(const char *_pSubKey, const char* _pKeyValue, DWORD *_pDword, CStr *_pStr, TArray<uint8> *_pMemory)
{		
	DWORD ValueSize=0;
	HKEY PathKey = NULL;
	DWORD ValueType;
	CStr SubKey = GetPath(_pSubKey);
	
	try
	{
		if (RegOpenKeyExA(m_RootKey,SubKey,0,KEY_READ,&PathKey) != ERROR_SUCCESS)
			Error_static("CWin32_Registry", "RegOpenKeyEx");
		
		if (RegQueryValueExA(PathKey,_pKeyValue,0,&ValueType,NULL,NULL) != ERROR_SUCCESS)
			Error_static("CWin32_Registry", "RegQueryValueEx");
		
		if ((ValueType == REG_DWORD) || (ValueType == REG_DWORD_LITTLE_ENDIAN) || (ValueType == REG_DWORD_BIG_ENDIAN))
		{
			if (!_pDword)
				Error_static("ReadRegKey", "Wrong valuetype");
			
			ValueSize=4;
			if (RegQueryValueExA(PathKey,_pKeyValue,0,NULL,(unsigned char*)_pDword,&ValueSize) != ERROR_SUCCESS)
				Error_static("CWin32_Registry", "RegQueryValueEx");
		}
		else if ((ValueType==REG_SZ) || (ValueType == REG_BINARY))
		{
			if (RegQueryValueExA(PathKey,_pKeyValue,0,NULL,NULL,&ValueSize)==ERROR_SUCCESS)
			{
				if (_pStr)
				{
					if (RegQueryValueExA(PathKey,_pKeyValue,0,NULL,(uint8 *)_pStr->GetBuffer(ValueSize),&ValueSize) != ERROR_SUCCESS)
						Error_static("CWin32_Registry", "RegQueryValueEx");
				}
				else if (_pMemory)
				{
					_pMemory->SetLen(ValueSize);
					if (RegQueryValueExA(PathKey,_pKeyValue,0,NULL,_pMemory->GetBasePtr(),&ValueSize) != ERROR_SUCCESS)
						Error_static("CWin32_Registry", "RegQueryValueEx");
				}
				else
					Error_static("ReadRegKey","Wrong valuetype");
				
			}
			else
				Error_static("CWin32_Registry", "RegQueryValueEx");
		}
		else
			Error_static("ReadRegKey","Unknown valuetype");
		
		RegCloseKey(PathKey);			
	}
	catch (CCException)
	{			
		if (PathKey)
			RegCloseKey(PathKey);
		throw;
	}
	
}

spCRegistry CWin32_Registry::Read_Registry(const char *_pSubKey)
{
	spCRegistry spTemp = DNew(CRegistry_Dynamic) CRegistry_Dynamic;

	spTemp->SetThisValue(Read_Str(_pSubKey, NULL));

	TArray<CStr> Children;

	EnumKeys(_pSubKey, Children);

	for (int i = 0; i < Children.Len(); ++i)
	{
		spCRegistry spChild = Read_Registry(CStr("") + _pSubKey + "\\" + Children[i]);
		spChild->SetThisKey(Children[i], spChild->GetThisValue());
		spTemp->AddKey(spChild);

	}

	return spTemp;
}

void CWin32_Registry::Write(const char *_pSubKey, CRegistry *_pReg)
{
	// Start by deleting old
	if (KeyExists(_pSubKey))
		RecursiveDeleteKey(_pSubKey);
	// Write our value
	Write(_pSubKey, NULL, _pReg->GetThisValue());

	// Write any children
	for (int i = 0; i < _pReg->GetNumChildren(); ++i)
	{
		CRegistry *pChild = _pReg->GetChild(i);
		CStr Key = CStr("") + _pSubKey + "\\" + pChild->GetThisName();
		// Write Key
		Write(Key, pChild);
	}        
}

void CWin32_Registry::RecursiveDeleteKey(const char *_pSubKey)
{
	// Delete all children keys
	{
		TArray<CStr> Children;

		EnumKeys(_pSubKey, Children);

		for (int i = 0; i < Children.Len(); ++i)
		{
			RecursiveDeleteKey(CStr() + _pSubKey + "\\" + Children[i]);
		}
	}

	// Delete all contained values
	TArray<CStr> Values;
	EnumKeyValues(_pSubKey, Values);
	for (int i = 0; i < Values.Len(); ++i)
	{
		DeleteKeyValue(_pSubKey, Values[i]);
	}
	// Delete Self
	DeleteKey(_pSubKey);
}

void CWin32_Registry::Write(const char *_pSubKey, const char* _pKeyValue, uint32 _Value)
{
	WriteRegKey(_pSubKey, _pKeyValue, &_Value, NULL, NULL);
}

void CWin32_Registry::Write(const char *_pSubKey, const char* _pKeyValue, const char *_pValue)
{
	CStr Temp;
	Temp.Capture(_pValue);
	
	WriteRegKey(_pSubKey, _pKeyValue, NULL, &Temp, NULL);
}

void CWin32_Registry::Write(const char *_pSubKey, const char* _pKeyValue, TArray<uint8> &_Data)
{
	WriteRegKey(_pSubKey, _pKeyValue, NULL, NULL, &_Data);
}

void CWin32_Registry::Write(const char *_pSubKey, const char* _pKeyValue, CStr &_pValue)
{
	WriteRegKey(_pSubKey, _pKeyValue, NULL, &_pValue, NULL);
}

void CWin32_Registry::WriteRegKey(const char* _pSubKey, const char* _pKeyValue, const DWORD *_pDword, const CStr *_pStr, const TArray<uint8> *_pMemory)
{
	HKEY PathKey = NULL;
	CStr SubKey = GetPath(_pSubKey);

	try
	{					
		if (RegCreateKeyExA(m_RootKey,SubKey,0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&PathKey,NULL) != ERROR_SUCCESS)
			Error_static("CWin32_Registry", "RegCreateKeyEx")
			
			if (_pDword)
			{
				if (RegSetValueExA(PathKey,_pKeyValue,0,REG_DWORD,(unsigned char*)_pDword,4) != ERROR_SUCCESS)
					Error_static("CWin32_Registry", "RegSetValueEx")
			}
			else if (_pStr)
			{				
				if (RegSetValueExA(PathKey,_pKeyValue,0,REG_SZ,(uint8 *)_pStr->Str(),_pStr->Len()) != ERROR_SUCCESS)
					Error_static("CWin32_Registry", "RegSetValueEx")
			}
			else if (_pMemory)
			{
				if (RegSetValueExA(PathKey,_pKeyValue,0,REG_BINARY,_pMemory->GetBasePtr(),_pMemory->Len()) != ERROR_SUCCESS)
					Error_static("CWin32_Registry", "RegSetValueEx")
			}
			else
			{
				Error_static("WriteRegKey","Nothing to write");
			}
			
			RegCloseKey(PathKey);
	}
	catch (CCException)
	{			
		if (PathKey)
			RegCloseKey(PathKey);
		throw;
	}
}

void CWin32_Registry::EnumKeyValues(const char *_pSubKey, TArray<CStr> &_lKeys)
{
	HKEY PathKey = NULL;
	CStr SubKey = GetPath(_pSubKey);

	try
	{
		if (RegOpenKey(m_RootKey,SubKey,&PathKey) != ERROR_SUCCESS)
			Error_static("CWin32_Registry", "RegOpenKey");
		
		int i = 0;
		while(1)
		{
			char Buf[1024];
			DWORD Size = 1024;
			if(RegEnumValue(PathKey, i++, Buf, &Size, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
				break;
			_lKeys.Add(Buf);
		}
	}
	catch (CCException)
	{			
		if (PathKey)
			RegCloseKey(PathKey);
		throw;
	}
	if (PathKey)
		RegCloseKey(PathKey);
}

void CWin32_Registry::EnumKeys(const char *_pSubKey, TArray<CStr> &_lKeys)
{
	HKEY PathKey = NULL;
	CStr SubKey = GetPath(_pSubKey);

	try
	{
		if (RegOpenKey(m_RootKey,SubKey,&PathKey) != ERROR_SUCCESS)
			Error_static("CWin32_Registry", "RegOpenKey");
		
		int i = 0;
		while(1)
		{
			char Buf[1024];
			DWORD Size = 1024;
			if(RegEnumKeyEx(PathKey, i++, Buf, &Size, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
				break;
			_lKeys.Add(Buf);
		}
	}
	catch (CCException)
	{			
		if (PathKey)
			RegCloseKey(PathKey);
		throw;
	}
	if (PathKey)
		RegCloseKey(PathKey);
}

void CWin32_Registry::DeleteKey(const char *_pSubKey)
{
	CStr SubKey = GetPath(_pSubKey);
	RegDeleteKey(m_RootKey, SubKey);
}

void CWin32_Registry::DeleteKeyValue(const char *_pSubKey, const char* _pKeyValue)
{
	HKEY PathKey = NULL;
	CStr SubKey = GetPath(_pSubKey);

	try
	{
		if (RegOpenKey(m_RootKey,SubKey,&PathKey) != ERROR_SUCCESS)
			Error_static("CWin32_Registry", "RegOpenKey");

		if(RegDeleteValue(PathKey, _pKeyValue) != ERROR_SUCCESS)
			Error_static("CWin32_Registry", "RegDeleteValue");
	}
	catch (CCException)
	{			
		if (PathKey)
			RegCloseKey(PathKey);
		throw;
	}
	if (PathKey)
		RegCloseKey(PathKey);
}
