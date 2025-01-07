#ifndef _INC_MREGISTRYBASE
#define _INC_MREGISTRYBASE

#include "../SysInc.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			CRegistryBase
					
	Author:			Magnus Högdahl
					
	Copyright:		Starbreeze Studios 2002

 	Creation Date:	2002-04-10

	Contents:		CRegistryBase
					
	Comments:

\*____________________________________________________________________________________________*/


class SYSTEMDLLEXPORT CRegistryBase : public CReferenceCount
{
public:
	virtual void Clear() pure;
	virtual int GetNumChildren() const pure;

	virtual CRegistryBase* GetChild_Base(int _iChild) pure;
	virtual const CRegistryBase* GetChild_Base(int _iChild) const pure;

	virtual CRegistryBase* GetChild(int _iChild) pure;
	virtual const CRegistryBase* GetChild(int _iChild) const pure;

	virtual int FindIndex(const char* _pKey) const pure;
	virtual int FindIndex(const char* _pKey, const char* _pValue, bool _bCaseSensitiveValue = true) const pure;

	virtual CRegistryBase* FindChild_Base(const char* _pKey) pure;
	virtual const CRegistryBase* FindChild_Base(const char* _pKey) const pure;

	virtual CRegistryBase* FindChild_Base(const char* _pKey, const char* _pValue, bool _bCaseSensitiveValue = true) pure;
	virtual const CRegistryBase* FindChild_Base(const char* _pKey, const char* _pValue, bool _bCaseSensitiveValue = true) const pure;

	virtual bool ValidChild(int _iChild) const pure;

	// Search with pathname  (ex. pSys->GetRegistry()->Find("SORCERY\\USER\\NAME");   )
	virtual CRegistry* Find(const char* _pKey) pure;
	virtual const CRegistry* Find(const char* _pKey) const pure;

	// Getting value from this
	virtual CStr GetThisName() const pure;

	virtual CStr GetThisValue() const pure;
	virtual int32 GetThisValuei() const pure;
	virtual fp32 GetThisValuef() const pure;
	virtual const TArray<uint8> GetThisValued() const pure;
	virtual TArray<uint8> GetThisValued() pure;

	// Getting value from a child
	virtual CStr GetName(int _iKey) const pure;

	virtual CStr GetValue(int _iKey) const pure;
	virtual int32 GetValuei(int _iKey) const pure;
	virtual fp32 GetValuef(int _iKey) const pure;
	virtual const TArray<uint8> GetValued(int _iKey) const pure;
	virtual TArray<uint8> GetValued(int _iKey) pure;

	virtual CStr GetValue(const char* _pName, const char* _pDefVal) const pure;
	virtual int32 GetValuei(const char* _pName, int32 _DefVal) const pure;
	virtual fp32 GetValuef(const char* _pName, fp32 _DefVal) const pure;

	virtual CStr GetValue(const char* _pName) const pure;	// Equivalent to GetValue(_pName, "", 0)
	virtual int32 GetValuei(const char* _pName) const pure;	// Equivalent to GetValue(_pName, 0, 0)
	virtual fp32 GetValuef(const char* _pName) const pure;	// Equivalent to GetValue(_pName, 0.0f, 0)
	virtual const TArray<uint8> GetValued(const char* _pName) const pure;
	virtual TArray<uint8> GetValued(const char* _pName) pure;
};

typedef TPtr<CRegistryBase> spCRegistryBase;
typedef TList_Vector<spCRegistryBase> lspCRegistryBase;

#endif // _INC_MREGISTRYBASE

