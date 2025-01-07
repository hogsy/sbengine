#ifndef _INC_MREGISTRY_STATIC
#define _INC_MREGISTRY_STATIC

#include "MRegistry.h"

// This registry is designed for the gamestate registry.
// It's probably not extremely small, nor extremely fast,
// but it will always reside in a static buffer. (no fragmentation)

class CRegistry_Static;
class CStaticRegistryHandler;


enum
{
	e_Value_String,
	e_Value_Int,
	e_Value_Float,
};

class SYSTEMDLLEXPORT CRegistry_Static : public CRegistry
{
	MRTC_DECLARE;
private:
	friend class CStaticRegistryHandler;

	CStaticRegistryHandler* m_pOwner; // pointer back to the handler-class
	uint32 m_nChildOffset;            // child node offset in bytes
	uint16 m_nChildren;               // number of children (stored after this)
	uint8  m_nNameSize;               // name data size
	uint8  m_ValueType;               // what kind of data is stored in the union?
	union
	{
		uint32 m_nValueSize;          // string data size
		int32  m_nValue;              // direct value
		fp4    m_fValue;              // direct value
	};

	void Init(CStaticRegistryHandler* _pOwner);
	int GetSize() const;
	void UpdateOffsets(uint32 _BaseOffset, int _DeltaOffset);

	const CRegistry_Static* GetChildFromIndex(int _iChild) const;
	      CRegistry_Static* GetChildFromIndex(int _iChild);

	const CRegistry_Static* GetNext() const;
	      CRegistry_Static* GetNext();

	const char* GetNameStrData() const;
	      char* GetNameStrData();

	const char* GetValueStrData() const;
	      char* GetValueStrData();

 #if defined(PLATFORM_DOLPHIN) && !defined(M_RTM)
	int DebugDump(int _nLevel=0) const;
 #endif

	void operator= (const CRegistry_Static& _Reg);

public:
	int MRTC_AddRef()               { return 100; }
	int MRTC_DelRef()               { return 100; }
	int MRTC_ReferenceCount() const { return 100; }

	virtual const CRegistry* GetDir(const char* _pDir) const;
	virtual CRegistry* GetDir(const char* _pDir);
	virtual void operator= (const CRegistry& _Reg);
	virtual int operator== (const CRegistry& _Reg); // const
	virtual int operator!= (const CRegistry& _Reg); // const
	virtual spCRegistry Duplicate() const;
	virtual void Clear();
	virtual void CreateIndexed(int);
	virtual int GetNumChildren() const;
	virtual void SetNumChildren(int _nChildren);
	virtual const CRegistry* GetChild(int _iChild) const;
	virtual CRegistry* GetChild(int _iChild);
	virtual const CRegistry* GetChild2(int _iChild) const;
	virtual int GetNumNodes_r() const;
	virtual void EnableHashing(bool _bRecursive = false);
	virtual void EnableAutoHashing(bool _bRecursive = true);
	virtual bool IsDirty();
	virtual void ClearDirty();
	virtual void SetDirty();
	virtual void SetThisUserFlags(int _Value);
	virtual int GetThisUserFlags() const;
	virtual void SetUserFlags(int _iKey, int _Value);
	virtual int GetUserFlags(int _iKey) const;
	virtual int GetType() const;
	virtual void Sort(bool _bReversedOrder = false, bool _bSortByName = true);
	virtual int FindIndex(const char* _pKey) const;
	virtual int FindIndex(const char* _pKey, const char* _pValue, bool _bCaseSensitiveValue = true) const;
	virtual const CRegistry* FindChild(const char* _pKey) const;
	virtual CRegistry* FindChild(const char* _pKey);
	virtual const CRegistry* FindChild(const char* _pKey, const char* _pValue, bool _bCaseSensitiveValue = true) const;
	virtual CRegistry* FindChild(const char* _pKey, const char* _pValue, bool _bCaseSensitiveValue = true);
	virtual bool ValidChild(int _iChild) const;
	virtual const CRegistry* Find(const char* _pKey) const;
	virtual CRegistry* Find(const char* _pKey);
	virtual CRegistry* CreateDir(const char* _pPath);
	virtual bool DeleteDir(const char* _pPath);
	virtual void CopyDir(const CRegistry* _pReg, bool _bRecursive = true);
	virtual spCRegistry EvalTemplate_r(const CRegistry* _pReg) const;
	virtual void SetThisKey(const char* _pValue);
	virtual void SetThisKey(const wchar* _pValue);
	virtual void SetThisKey(CStr _Value);
	virtual void SetThisKeyi(int32 _Value);
	virtual void SetThisKeyf(fp4 _Value);
	virtual void SetThisKeyd(const uint8* _pValue, int _Size, bool _bQuick = true);
	virtual void SetThisKeyd(TArray<uint8> _lValue, bool _bReference = true);
	virtual void SetThisKey(const char* _pName, const char* _pValue);
	virtual void SetThisKey(const char* _pName, const wchar* _pValue);
	virtual void SetThisKey(const char* _pName, CStr _Value);
	virtual void SetThisKeyi(const char* _pName, int32 _Value);
	virtual void SetThisKeyf(const char* _pName, fp4 _Value);
	virtual void SetThisKeyd(const char* _pName, const uint8* _pValue, int _Size, bool _bQuick = true);
	virtual void SetThisKeyd(const char* _pName, TArray<uint8> _lValue, bool _bReference = true);
	        void SetThisKey(const CRegistry* _pReg);
	virtual void RenameThisKey(const char* _pName);
	virtual void AddReg(spCRegistry _spReg);
	virtual void AddKey(const char* _pName, const char* _pValue);
	virtual void AddKey(const char* _pName, const wchar* _pValue);
	virtual void AddKey(const char* _pName, CStr _Value);
	virtual void AddKeyi(const char* _pName, int32 _Value);
	virtual void AddKeyf(const char* _pName, fp4 _Value);
	virtual void AddKeyd(const char* _pName, const uint8* _pValue, int _Size, bool _bQuick = true);
	virtual void AddKeyd(const char* _pName, TArray<uint8> _lValue, bool _bReference = true);
	virtual void AddKey(const CRegistry* _pReg);
	virtual void SetValue(const char* _pName, const char* _pValue);
	virtual void SetValue(const char* _pName, const wchar* _pValue);
	virtual void SetValue(const char* _pName, CStr _Value);
	virtual void SetValuei(const char* _pName, int32 _Value);
	virtual void SetValuef(const char* _pName, fp4 _Value);
	virtual void SetValued(const char* _pName, const uint8* _pValue, int _Size, bool _bQuick = true);
	virtual void SetValued(const char* _pName, TArray<uint8> _lValue, bool _bReference = true);
	virtual void SetValue(const CRegistry* _pReg);
	virtual void SetValue(int _iKey, const char* _pValue);
	virtual void SetValue(int _iKey, const wchar* _pValue);
	virtual void SetValue(int _iKey, CStr _Value);
	virtual void SetValuei(int _iKey, int32 _Value);
	virtual void SetValuef(int _iKey, fp4 _Value);
	virtual void SetValued(int _iKey, const uint8* _pValue, int _Size, bool _bQuick = true);
	virtual void SetValued(int _iKey, TArray<uint8> _lValue, bool _bReference = true);
	virtual void SetValue(int _iKey, const CRegistry* _pReg);
	virtual void RenameKey(const char* _pName, const char* _pNewName);
	virtual void RenameKey(int _iKey, const char* _pNewName);
	virtual CStr GetThisName() const;
	virtual const char* GetThisName2() const;
	virtual CStr GetThisValue() const;
	virtual int32 GetThisValuei() const;
	virtual fp4 GetThisValuef() const;
	virtual const TArray<uint8> GetThisValued() const;
	virtual TArray<uint8> GetThisValued();
	virtual void DeleteKey(const char* _pName);
	virtual void DeleteKey(int _iKey);
	virtual spCRegistry XRG_AddParseKey(CStr _Key, CStr _Value, CRegistry_ParseContext& _ParseContext);
	virtual int XRG_Parse(char* _pOrgData, char* _pData, int _Size, CRegistry_ParseContext& _ParseContext);
	virtual int XRG_Parse(wchar* _pOrgData, wchar* _pData, int _Size, CRegistry_ParseContext& _ParseContext);
	virtual void XRG_Read(const CStr& _Filename, TArray<CStr> _lDefines, bool _bSlashIsEscape = true);
	virtual void XRG_Read(CCFile *_pFile, CStr _ThisFileName, TArray<CStr> _lDefines, bool _bSlashIsEscape = true);
	virtual void XRG_Read(const CStr& _Filename);
	virtual void ReadSimple(CCFile* _pFile);
	virtual void ReadSimple(CStr _FileName);
	virtual void ReadRegistryDir(CStr _Dir, CCFile* _pFile);
	virtual void ReadRegistryDir(CStr _Dir, CStr _Filename);
	virtual void Read(CCFile* _pFile);
	virtual void Write(CCFile* _pFile) const;
	virtual bool Read(CDataFile* _pDFile, const char* _pEntryName);
	virtual void Write(CDataFile* _pDFile, const char* _pEntryName) const;
	virtual void Read(const char* _pFileName);
	virtual void Write(const char* _pFileName) const;
};


class SYSTEMDLLEXPORT CStaticRegistryHandler
{
	friend class CRegistry_Static;

private:
	uint8* m_pData;
	uint32 m_nCapacity;
	uint32 m_nSize;

	void UpdateSize(CRegistry_Static* _pElem, uint32 _nNewSize, uint32 _nOldSize);

	void Elem_SetName(CRegistry_Static* _pElem, const char* _pName);
	void Elem_Set(CRegistry_Static* _pElem, const char* _pValue);
	void Elem_Set(CRegistry_Static* _pElem, int32 _nValue);
	void Elem_Set(CRegistry_Static* _pElem, fp4 _fValue);

	CRegistry_Static* Elem_Add(CRegistry_Static* _pElem, uint32 _Num, uint32 _Size);
	CRegistry_Static* Elem_Add(CRegistry_Static* _pElem, const char* _pChildName, const char* _pChildValue);
	CRegistry_Static* Elem_Add(CRegistry_Static* _pElem, const char* _pChildName, int32 _nValue);
	CRegistry_Static* Elem_Add(CRegistry_Static* _pElem, const char* _pChildName, fp4 _fValue);

public:
	CStaticRegistryHandler(uint32 _Size);
	~CStaticRegistryHandler();
	CRegistry_Static* GetRoot() { return (CRegistry_Static*)m_pData; }
};



#endif // _INC_MREGISTRY_STATIC
