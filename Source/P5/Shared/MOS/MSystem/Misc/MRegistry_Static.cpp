#include "PCH.h"
#include "MRegistry_Static.h"

MRTC_IMPLEMENT( CRegistry_Static, CRegistry );

void CRegistry_Static::Init(CStaticRegistryHandler* _pOwner)
{

	new (this) CRegistry_Static();

	m_pOwner = _pOwner;
	m_nChildOffset = 0;
	m_nChildren = 0;
	m_nNameSize = 0;
	m_ValueType = e_Value_String;
	m_nValueSize = 0;
}


static inline int CalcSize(uint32 _nNameSize, uint8 _ValueType, uint32 _nValueSize)
{
	int nSize = sizeof(CRegistry_Static) + _nNameSize;
	if (_ValueType == e_Value_String)
		nSize += _nValueSize;
	return (nSize + 3) & ~3;
}


int CRegistry_Static::GetSize() const
{
	return CalcSize(m_nNameSize, m_ValueType, m_nValueSize);
}


const CRegistry_Static* CRegistry_Static::GetChildFromIndex(int _iChild) const
{
	const CRegistry_Static* pChild = (const CRegistry_Static*)((const uint8*)this + m_nChildOffset);
	for (uint32 i=0; i<_iChild; i++)
		pChild = pChild->GetNext();
	return pChild;
}


CRegistry_Static* CRegistry_Static::GetChildFromIndex(int _iChild)
{
	CRegistry_Static* pChild = (CRegistry_Static*)((uint8*)this + m_nChildOffset);
	for (uint32 i=0; i<_iChild; i++)
		pChild = pChild->GetNext();
	return pChild;
}


const CRegistry_Static* CRegistry_Static::GetNext() const
{
	return (const CRegistry_Static*)((const uint8*)this + GetSize());
}


CRegistry_Static* CRegistry_Static::GetNext()
{
	return (CRegistry_Static*)((uint8*)this + GetSize());
}


const char* CRegistry_Static::GetNameStrData() const
{
	return (m_nNameSize ? (char*)this + sizeof(CRegistry_Static) : 0);
}


char* CRegistry_Static::GetNameStrData()
{
	return (m_nNameSize ? (char*)this + sizeof(CRegistry_Static) : 0);
}


const char* CRegistry_Static::GetValueStrData() const
{
	return (m_nValueSize ? (char*)this + sizeof(CRegistry_Static) + m_nNameSize : 0);
}


char* CRegistry_Static::GetValueStrData()
{
	return (m_nValueSize ? (char*)this + sizeof(CRegistry_Static) + m_nNameSize : 0);
}


void CRegistry_Static::UpdateOffsets(uint32 _BaseOffset, int _DeltaOffset)
{
	M_ASSERT(_BaseOffset < m_pOwner->m_nCapacity, "!");
	M_ASSERT( (uint32(this) - uint32(m_pOwner->m_pData)) < m_pOwner->m_nCapacity, "!");
	M_ASSERT((m_nChildren == 0) || (m_nChildOffset > 0 && m_nChildOffset < m_pOwner->m_nCapacity), "!");

	CRegistry_Static* pChild = GetChildFromIndex(0);
	for (uint32 i=0; i<m_nChildren; i++)
	{
		uint32 nChildOffset = uint32(pChild) - uint32(this);
		M_ASSERT(nChildOffset < m_pOwner->m_nCapacity, "!");
		if (_BaseOffset >= nChildOffset) // do not update offsets for elements inside the block to be moved
			pChild->UpdateOffsets(_BaseOffset - nChildOffset, _DeltaOffset);
		pChild = pChild->GetNext();
	}

	if (m_nChildOffset > _BaseOffset) // is our offset inside the block to be moved
		m_nChildOffset += _DeltaOffset;
}


#if defined(PLATFORM_DOLPHIN) && !defined(M_RTM)
int CRegistry_Static::DebugDump(int _nLevel) const
{
	int size = GetSize();
	OSReport("%*s*%s ", _nLevel*4, "", m_nNameSize ? GetNameStrData() : "(null)");
	if (m_ValueType == e_Value_String)
		OSReport("%s (string)", m_nValueSize ? GetValueStrData() : "(null)");
	else if (m_ValueType == e_Value_Int)
		OSReport("%d (int)", m_nValue);
	else if (m_ValueType == e_Value_Float)
		OSReport("%f (float)", m_fValue);
	else
		OSReport("<error>");

	OSReport(" (%d)\n", size);

	if (m_nChildren)
	{
		OSReport("%*s{\n", _nLevel*4, "");
		const CRegistry_Static* pChild = GetChildFromIndex(0);
		for (int i=0; i<m_nChildren; i++)
		{
			size += pChild->DebugDump(_nLevel+1);
			pChild = pChild->GetNext();
		}
		OSReport("%*s}\n", _nLevel*4, "");
	}
	return size;
}
#endif


const CRegistry* CRegistry_Static::GetDir(const char* _pDir) const
{
	CFStr Dir = _pDir;
	const CRegistry* pReg = this;
	while(Dir != "")
	{
		const CRegistry* pChild = pReg->FindChild(Dir.GetStrSep("\\"));
		if (!pChild) return NULL;
		pReg = pChild;
	}
	return pReg;	
}


CRegistry* CRegistry_Static::GetDir(const char* _pDir)
{
	CFStr Dir = _pDir;
	CRegistry* pReg = this;
	while (Dir != "")
	{
		CRegistry* pChild = pReg->FindChild(Dir.GetStrSep("\\"));
		if (!pChild) return NULL;
		pReg = pChild;
	}
	return pReg;	
}


void CRegistry_Static::operator= (const CRegistry& _Reg)
{
	Error_static("CRegistry_Static::operator=", "Not implemented!");
}


int CRegistry_Static::operator== (const CRegistry& _Reg)// const
{
	if (GetThisValue() != _Reg.GetThisValue())
		return false;
	if (GetThisName() != _Reg.GetThisName())
		return false;
	if (GetNumChildren() != _Reg.GetNumChildren())
		return false;

	for (int i = 0; i < GetNumChildren(); i++)
		if (*GetChild(i) != *_Reg.GetChild(i))
			return false;

	return true;
}


int CRegistry_Static::operator!= (const CRegistry& _Reg)// const
{
	return !(*this == _Reg);
}


spCRegistry CRegistry_Static::Duplicate() const
{
	spCRegistry spR = REGISTRY_CREATE;
	if (!spR) Error_static("Duplicate", "Out of memory");
	*spR = *this;
	return spR;
}


void CRegistry_Static::Clear()
{
	M_ASSERT(m_pOwner, "!");

	// clear children
	int nTotalChildSize = 0;
	for (int i=m_nChildren; i>0; i--)
	{
		M_ASSERT(m_pOwner, "!");
		CRegistry_Static* pChild = GetChildFromIndex(i - 1);
		pChild->Clear();
		nTotalChildSize += pChild->GetSize();
	}
	int nChildSize2 = (uint32)GetChildFromIndex(m_nChildren) - (uint32)GetChildFromIndex(0);
	M_ASSERT(nTotalChildSize == nChildSize2, "!");

	M_ASSERT(m_pOwner, "!");
	if (nTotalChildSize)
		m_pOwner->UpdateSize(GetChildFromIndex(0), 0, nTotalChildSize);

	// clear self
	m_nChildren = 0;
	m_nChildOffset = 0;
	M_ASSERT(m_pOwner, "!");
	m_pOwner->UpdateSize(this, CalcSize(0, e_Value_String, 0), GetSize());
	m_nNameSize = 0;
	m_ValueType = e_Value_String;
	m_nValueSize = 0;
}

void CRegistry_Static::CreateIndexed(int)
{
	Error_static("CRegistry_Static::CreateIndexed", "Not implemented!");
}


int CRegistry_Static::GetNumChildren() const
{
	return m_nChildren;
}


void CRegistry_Static::SetNumChildren(int _nChildren)
{
	if (_nChildren > m_nChildren)
	{ // clear and add more children
		int nNew = _nChildren - m_nChildren;
		CRegistry_Static* pChild = GetChildFromIndex(0);
		for (int i=0; i<m_nChildren; i++)
		{
			pChild->Clear();
			pChild = pChild->GetNext();
		}
		m_pOwner->Elem_Add(this, nNew, sizeof(CRegistry_Static));
	}
	else if (_nChildren < m_nChildren)
	{ // clear and remove some children
		CRegistry_Static* pChild = GetChildFromIndex(0);
		for (int i=0; i<m_nChildren; i++)
		{
			pChild->Clear();
			pChild = pChild->GetNext();
		}
		pChild = GetChildFromIndex(_nChildren);
		m_pOwner->UpdateSize(pChild, 0, (uint32)GetChildFromIndex(m_nChildren) - (uint32)pChild);
		m_nChildren = _nChildren;
		if (_nChildren == 0)
			m_nChildOffset = 0;
	}
	else
	{ // just clear the existing ones..
		CRegistry_Static* pChild = GetChildFromIndex(0);
		for (int i=0; i<m_nChildren; i++)
		{
			pChild->Clear();
			pChild = pChild->GetNext();
		}
	}
}


const CRegistry* CRegistry_Static::GetChild(int _iChild) const
{
	if (_iChild < 0 || _iChild >= m_nChildren)
		Error_static("CRegistry_Static::GetChild", "Index out of range");

	const CRegistry_Static* pChild = GetChildFromIndex(0);
	for (uint32 i=0; i<_iChild; i++)
		pChild = pChild->GetNext();
	return pChild;
}


CRegistry* CRegistry_Static::GetChild(int _iChild)
{
	if (_iChild < 0 || _iChild >= m_nChildren)
		Error_static("CRegistry_Static::GetChild", "Index out of range");

	CRegistry_Static* pChild = GetChildFromIndex(0);
	for (uint32 i=0; i<_iChild; i++)
		pChild = pChild->GetNext();
	return pChild;
}


const CRegistry* CRegistry_Static::GetChild2(int _iChild) const
{
	const CRegistry_Static* pChild = GetChildFromIndex(0);
	for (uint32 i=0; i<_iChild; i++)
		pChild = pChild->GetNext();
	return pChild;
}


int CRegistry_Static::GetNumNodes_r() const
{
	int nNodes = 1;
	const CRegistry_Static* pChild = GetChildFromIndex(0);
	for (int i=0; i<m_nChildren; i++)
	{
		nNodes += pChild->GetNumNodes_r();
		pChild = pChild->GetNext();
	}
	return nNodes;
}


void CRegistry_Static::EnableHashing(bool _bRecursive)
{
	Error_static("CRegistry_Static::EnableHashing", "Not implemented!");
}


void CRegistry_Static::EnableAutoHashing(bool _bRecursive)
{
	Error_static("CRegistry_Static::EnableAutoHashing", "Not implemented!");
}


bool CRegistry_Static::IsDirty()
{
	Error_static("CRegistry_Static::IsDirty", "Not implemented!");
}


void CRegistry_Static::ClearDirty()
{
	Error_static("CRegistry_Static::ClearDirty", "Not implemented!");
}


void CRegistry_Static::SetDirty()
{
	Error_static("CRegistry_Static::SetDirty", "Not implemented!");
}


void CRegistry_Static::SetThisUserFlags(int _Value)
{
	Error_static("CRegistry_Static::SetThisUserFlags", "Not implemented!");
}


int CRegistry_Static::GetThisUserFlags() const
{
	Error_static("CRegistry_Static::GetThisUserFlags", "Not implemented!");
}


void CRegistry_Static::SetUserFlags(int _iKey, int _Value)
{
	Error_static("CRegistry_Static::SetUserFlags", "Not implemented!");
}


int CRegistry_Static::GetUserFlags(int _iKey) const
{
	Error_static("CRegistry_Static::GetUserFlags", "Not implemented!");
}


int CRegistry_Static::GetType() const
{
	if (m_ValueType == e_Value_Int)
		return REGISTRY_TYPE_INT;
	else if (m_ValueType == e_Value_Float)
		return REGISTRY_TYPE_FLOAT;
	else
		return REGISTRY_TYPE_STR;
}


void CRegistry_Static::Sort(bool _bReversedOrder, bool _bSortByName)
{
	Error_static("CRegistry_Static::Sort", "Not implemented!");
}


int CRegistry_Static::FindIndex(const char* _pKey) const
{
	const CRegistry_Static* pChild = GetChildFromIndex(0);
	for (uint32 i=0; i<m_nChildren; i++)
	{
		if (CStrBase::CompareNoCase(_pKey, pChild->GetNameStrData()) == 0)
			return i;

		pChild = pChild->GetNext();
	}
	return -1;
}


int CRegistry_Static::FindIndex(const char* _pKey, const char* _pValue, bool _bCaseSensitiveValue) const
{
	const CRegistry_Static* pChild = GetChildFromIndex(0);
	for (int i=0; i<m_nChildren; i++)
	{
		if (CStrBase::CompareNoCase(_pKey, pChild->GetNameStrData()) == 0)
		{
			if (_bCaseSensitiveValue)
				{ if (pChild->GetThisValue() == _pValue) return i; }
			else
				{ if (pChild->GetThisValue().CompareNoCase(_pValue) == 0) return i; }
		}
	}
	return -1;	
}


const CRegistry* CRegistry_Static::FindChild(const char* _pKey) const
{
	const CRegistry_Static* pChild = GetChildFromIndex(0);
	for (uint32 i=0; i<m_nChildren; i++)
	{
		if (CStrBase::CompareNoCase(_pKey, pChild->GetNameStrData()) == 0)
			return pChild;

		pChild = pChild->GetNext();
	}
	return NULL;
}


CRegistry* CRegistry_Static::FindChild(const char* _pKey)
{
	CRegistry_Static* pChild = GetChildFromIndex(0);
	for (uint32 i=0; i<m_nChildren; i++)
	{
		if (CStrBase::CompareNoCase(_pKey, pChild->GetNameStrData()) == 0)
			return pChild;

		pChild = pChild->GetNext();
	}
	return NULL;
}


const CRegistry* CRegistry_Static::FindChild(const char* _pKey, const char* _pValue, bool _bCaseSensitiveValue) const
{
	const CRegistry_Static* pChild = GetChildFromIndex(0);
	for (int i=0; i<m_nChildren; i++)
	{
		if (CStrBase::CompareNoCase(_pKey, pChild->GetNameStrData()) == 0)
		{
			if (_bCaseSensitiveValue)
				{ if (pChild->GetThisValue() == _pValue) return pChild; }
			else
				{ if (pChild->GetThisValue().CompareNoCase(_pValue) == 0) return pChild; }
		}
		pChild = pChild->GetNext();
	}
	return NULL;
}


CRegistry* CRegistry_Static::FindChild(const char* _pKey, const char* _pValue, bool _bCaseSensitiveValue)
{
	CRegistry_Static* pChild = GetChildFromIndex(0);
	for (int i=0; i<m_nChildren; i++)
	{
		if (CStrBase::CompareNoCase(_pKey, pChild->GetNameStrData()) == 0)
		{
			if (_bCaseSensitiveValue)
				{ if (pChild->GetThisValue() == _pValue) return pChild; }
			else
				{ if (pChild->GetThisValue().CompareNoCase(_pValue) == 0) return pChild; }
		}
		pChild = pChild->GetNext();
	}
	return NULL;
}


bool CRegistry_Static::ValidChild(int _iChild) const
{
	return (_iChild >= 0) && (_iChild < m_nChildren);
}


const CRegistry* CRegistry_Static::Find(const char* _pKey) const
{
	if (!_pKey) return NULL;

	if (strstr(_pKey, "\\"))
		return GetDir(_pKey);
	else
		return FindChild(_pKey);
}


CRegistry* CRegistry_Static::Find(const char* _pKey)
{
	if (!_pKey) return NULL;

	if (strstr(_pKey, "\\"))
		return GetDir(_pKey);
	else
		return FindChild(_pKey);
}


CRegistry* CRegistry_Static::CreateDir(const char* _pPath)
{
	CFStr Dir = _pPath;
	CRegistry_Static* pReg = this;
	while (Dir != "")
	{
		CFStr Name = Dir.GetStrSep("\\");
		CRegistry_Static* pChild = safe_cast<CRegistry_Static>(pReg->Find(Name));
		if (!pChild)
			pChild = m_pOwner->Elem_Add(pReg, Name, int32(0));

		pReg = pChild;
	}
	return pReg;
}


bool CRegistry_Static::DeleteDir(const char* _pPath)
{
	Error_static("CRegistry_Static::DeleteDir", "Not implemented!");
}


void CRegistry_Static::CopyDir(const CRegistry* _pReg, bool _bRecursive)
{
	Error_static("CRegistry_Static::CopyDir", "Not implemented!");
}


spCRegistry CRegistry_Static::EvalTemplate_r(const CRegistry* _pReg) const
{
	Error_static("CRegistry_Static::EvalTemplate_r", "Not implemented!");
}


void CRegistry_Static::SetThisKey(const char* _pValue)
{
	m_pOwner->Elem_Set(this, _pValue);
}


void CRegistry_Static::SetThisKey(const wchar* _pValue)
{
	Error_static("CRegistry_Static::SetThisKey(unicode)", "Not implemented!");
}


void CRegistry_Static::SetThisKey(CStr _Value)
{
	if (!_Value.IsAnsi())
		Error_static("CRegistry_Static::SetThisKey(unicode)", "Not implemented!");
	m_pOwner->Elem_Set(this, _Value.GetStr());
}


void CRegistry_Static::SetThisKeyi(int32 _Value)
{
	m_pOwner->Elem_Set(this, _Value);
}


void CRegistry_Static::SetThisKeyf(fp32 _Value)
{
	m_pOwner->Elem_Set(this, _Value);
}


void CRegistry_Static::SetThisKeyd(const uint8* _pValue, int _Size, bool _bQuick)
{
	Error_static("CRegistry_Static::SetThisKey(uint8*,int,bool)", "Not implemented!");
}


void CRegistry_Static::SetThisKeyd(TArray<uint8> _lValue, bool _bReference)
{
	Error_static("CRegistry_Static::SetThisKey(TArray<uint8>,bool)", "Not implemented!");
}


void CRegistry_Static::SetThisKey(const char* _pName, const char* _pValue)
{
	m_pOwner->Elem_SetName(this, _pName);
	m_pOwner->Elem_Set(this, _pValue);
}


void CRegistry_Static::SetThisKey(const char* _pName, const wchar* _pValue)
{
	Error_static("CRegistry_Static::SetThisKey(unicode)", "Not implemented!");
	m_pOwner->Elem_SetName(this, _pName);
}


void CRegistry_Static::SetThisKey(const char* _pName, CStr _Value)
{
	if (!_Value.IsAnsi())
		Error_static("CRegistry_Static::SetThisKey(unicode)", "Not implemented!");
	m_pOwner->Elem_SetName(this, _pName);
	m_pOwner->Elem_Set(this, _Value.GetStr());
}


void CRegistry_Static::SetThisKeyi(const char* _pName, int32 _Value)
{
	m_pOwner->Elem_SetName(this, _pName);
	m_pOwner->Elem_Set(this, _Value);
}


void CRegistry_Static::SetThisKeyf(const char* _pName, fp32 _Value)
{
	m_pOwner->Elem_SetName(this, _pName);
	m_pOwner->Elem_Set(this, _Value);
}


void CRegistry_Static::SetThisKeyd(const char* _pName, const uint8* _pValue, int _Size, bool _bQuick)
{
	Error_static("CRegistry_Static::SetThisKey(data)", "Not implemented!");
}


void CRegistry_Static::SetThisKeyd(const char* _pName, TArray<uint8> _lValue, bool _bReference)
{
	Error_static("CRegistry_Static::SetThisKey(data)", "Not implemented!");
}


void CRegistry_Static::SetThisKey(const CRegistry* _pReg)
{
	m_pOwner->Elem_SetName(this, _pReg->GetThisName());

	int Type = _pReg->GetType();
	if (Type == REGISTRY_TYPE_INT)
		m_pOwner->Elem_Set(this, _pReg->GetThisValuei());
	else if (Type == REGISTRY_TYPE_FLOAT)
		m_pOwner->Elem_Set(this, _pReg->GetThisValuef());
	else
		m_pOwner->Elem_Set(this, _pReg->GetThisValue());

	SetNumChildren(_pReg->GetNumChildren());
	CRegistry_Static* pChild = GetChildFromIndex(0);
	for (int i=0; i<m_nChildren; i++)
	{
		pChild->SetThisKey(_pReg->GetChild(i));
		pChild = pChild->GetNext();
	}
}


void CRegistry_Static::RenameThisKey(const char* _pName)
{
	m_pOwner->Elem_SetName(this, _pName);
}


void CRegistry_Static::AddReg(spCRegistry _spReg)
{
	AddKey(_spReg); //NOTE: Is this correct?
}


void CRegistry_Static::AddKey(const char* _pName, const char* _pValue)
{
	m_pOwner->Elem_Add(this, _pName, _pValue);
}


void CRegistry_Static::AddKey(const char* _pName, const wchar* _pValue)
{
	Error_static("CRegistry_Static::AddKey(unicode)", "Not implemented!");
}


void CRegistry_Static::AddKey(const char* _pName, CStr _Value)
{
	if (!_Value.IsAnsi())
		Error_static("CRegistry_Static::AddKey(unicode)", "Not implemented!");
	m_pOwner->Elem_Add(this, _pName, _Value.GetStr());
}


void CRegistry_Static::AddKeyi(const char* _pName, int32 _Value)
{
	m_pOwner->Elem_Add(this, _pName, _Value);
}


void CRegistry_Static::AddKeyf(const char* _pName, fp32 _Value)
{
	m_pOwner->Elem_Add(this, _pName, _Value);
}


void CRegistry_Static::AddKeyd(const char* _pName, const uint8* _pValue, int _Size, bool _bQuick)
{
	Error_static("CRegistry_Static::AddKeyd", "Not implemented!");
}


void CRegistry_Static::AddKeyd(const char* _pName, TArray<uint8> _lValue, bool _bReference)
{
	Error_static("CRegistry_Static::AddKeyd", "Not implemented!");
}


void CRegistry_Static::AddKey(const CRegistry* _pReg)
{
	CRegistry_Static* pChild = m_pOwner->Elem_Add(this, _pReg->GetThisName(), int32(0));
	pChild->SetThisKey(_pReg);
}


void CRegistry_Static::SetValue(const char* _pName, const char* _pValue)
{
	CRegistry* pR = Find(_pName);
	if (!pR)
		AddKey(_pName, _pValue);
	else
		pR->SetThisKey(_pValue);
}


void CRegistry_Static::SetValue(const char* _pName, const wchar* _pValue)
{
	CRegistry* pR = Find(_pName);
	if (!pR)
		AddKey(_pName, _pValue);
	else
		pR->SetThisKey(_pValue);
}


void CRegistry_Static::SetValue(const char* _pName, CStr _Value)
{
	CRegistry* pR = Find(_pName);
	if (!pR)
		AddKey(_pName, _Value.GetStr());
	else
		pR->SetThisKey(_Value.GetStr());
}


void CRegistry_Static::SetValuei(const char* _pName, int32 _Value)
{
	CRegistry* pR = Find(_pName);
	if (!pR)
		AddKeyi(_pName, _Value);
	else
		pR->SetThisKeyi(_Value);
}


void CRegistry_Static::SetValuef(const char* _pName, fp32 _Value)
{
	CRegistry* pR = Find(_pName);
	if (!pR)
		AddKeyf(_pName, _Value);
	else
		pR->SetThisKeyf(_Value);
}


void CRegistry_Static::SetValued(const char* _pName, const uint8* _pValue, int _Size, bool _bQuick)
{
	CRegistry* pR = Find(_pName);
	if (!pR)
		AddKeyd(_pName, _pValue, _Size, _bQuick);
	else
		pR->SetThisKeyd(_pValue, _Size, _bQuick);
}


void CRegistry_Static::SetValued(const char* _pName, TArray<uint8> _lValue, bool _bReference)
{
	CRegistry* pR = Find(_pName);
	if (!pR)
		AddKeyd(_pName, _lValue, _bReference);
	else
		pR->SetThisKeyd(_lValue, _bReference);
}


void CRegistry_Static::SetValue(const CRegistry* _pReg)
{
	CRegistry* pR = Find(_pReg->GetThisName());
	if (!pR)
		AddKey(_pReg);
	else
		safe_cast<CRegistry_Static>(pR)->SetThisKey(_pReg);
}


void CRegistry_Static::SetValue(int _iKey, const char* _pValue)
{
	m_pOwner->Elem_Set(GetChildFromIndex(_iKey), _pValue);
}


void CRegistry_Static::SetValue(int _iKey, const wchar* _pValue)
{
	Error_static("CRegistry_Static::SetValue(unicode)", "Not implemented!");
}


void CRegistry_Static::SetValue(int _iKey, CStr _Value)
{
	if (!_Value.IsAnsi())
		Error_static("CRegistry_Static::SetValue(unicode)", "Not implemented!");
	m_pOwner->Elem_Set(GetChildFromIndex(_iKey), _Value.GetStr());		
}


void CRegistry_Static::SetValuei(int _iKey, int32 _Value)
{
	m_pOwner->Elem_Set(GetChildFromIndex(_iKey), _Value);
}


void CRegistry_Static::SetValuef(int _iKey, fp32 _Value)
{
	m_pOwner->Elem_Set(GetChildFromIndex(_iKey), _Value);
}


void CRegistry_Static::SetValued(int _iKey, const uint8* _pValue, int _Size, bool _bQuick)
{
	Error_static("CRegistry_Static::SetValued", "Not implemented!");
}


void CRegistry_Static::SetValued(int _iKey, TArray<uint8> _lValue, bool _bReference)
{
	Error_static("CRegistry_Static::SetValued", "Not implemented!");
}


void CRegistry_Static::SetValue(int _iKey, const CRegistry* _pReg)
{
	GetChildFromIndex(_iKey)->SetThisKey(_pReg);
}


void CRegistry_Static::RenameKey(const char* _pName, const char* _pNewName)
{
	m_pOwner->Elem_SetName(safe_cast<CRegistry_Static>(FindChild(_pName)), _pNewName);
}


void CRegistry_Static::RenameKey(int _iKey, const char* _pNewName)
{
	m_pOwner->Elem_SetName(GetChildFromIndex(_iKey), _pNewName);
}


CStr CRegistry_Static::GetThisName() const
{
	return GetNameStrData();
}


const char* CRegistry_Static::GetThisName2() const
{
	return GetNameStrData();
}


CStr CRegistry_Static::GetThisValue() const
{
	if (m_ValueType == e_Value_Int)
		return CStrF("%d", m_nValue);
	else if (m_ValueType == e_Value_Float)
		return CStrF("%f", m_fValue);
	else
		return GetValueStrData();
}


int32 CRegistry_Static::GetThisValuei() const
{
	if (m_ValueType == e_Value_Int)
		return m_nValue;
	else if (m_ValueType == e_Value_Float)
		return (int32)m_fValue;
	else
	{
		int Tmp;
		CStrBase::Val_int(GetValueStrData(), Tmp);
		return Tmp;
	}
}


fp32 CRegistry_Static::GetThisValuef() const
{
	if (m_ValueType == e_Value_Float)
		return m_fValue;
	else if (m_ValueType == e_Value_Int)
		return (fp32)m_nValue;
	else
	{
		fp64 Tmp;
		CStrBase::Val_fp64(GetValueStrData(), Tmp);
		return Tmp;
	}
}


const TArray<uint8> CRegistry_Static::GetThisValued() const
{
	Error_static("CRegistry_Static::GetThisValued", "Not implemented!");
}


TArray<uint8> CRegistry_Static::GetThisValued()
{
	Error_static("CRegistry_Static::GetThisValued", "Not implemented!");
}


void CRegistry_Static::DeleteKey(const char* _pName)
{
	CRegistry_Static* pChild = safe_cast<CRegistry_Static>(FindChild(_pName));
	if (pChild)
	{
		pChild->Clear();
		m_pOwner->UpdateSize(pChild, 0, pChild->GetSize());
		if (--m_nChildren == 0)
			m_nChildOffset = 0;
	}
}


void CRegistry_Static::DeleteKey(int _iKey)
{
	CRegistry_Static* pChild = GetChildFromIndex(_iKey);
	if (pChild)
	{
		pChild->Clear();
		m_pOwner->UpdateSize(pChild, 0, pChild->GetSize());
		if (--m_nChildren == 0)
			m_nChildOffset = 0;
	}
}


spCRegistry CRegistry_Static::XRG_AddParseKey(CStr _Key, CStr _Value, CStr _ThisFileName)
{
	Error_static("CRegistry_Static::XRG_AddParseKey", "Not implemented!");
}


int CRegistry_Static::XRG_Parse(char* _pOrgData, char* _pData, int _Size, CStr _ThisFileName)
{
	Error_static("CRegistry_Static::XRG_Parse", "Not implemented!");
}


int CRegistry_Static::XRG_Parse(wchar* _pOrgData, wchar* _pData, int _Size, CStr _ThisFileName)
{
	Error_static("CRegistry_Static::XRG_Parse", "Not implemented!");
}

void XRG_Read(const CStr& _Filename, TArray<CStr> _lDefines, bool _bSlashIsEscape);
{
	Error_static("CRegistry_Static::XRG_Read", "Not implemented!");
}

void XRG_Read(CCFile *_pFile, CStr _ThisFileName, TArray<CStr> _lDefines, bool _bSlashIsEscape);
{
	Error_static("CRegistry_Static::XRG_Read", "Not implemented!");
}

void XRG_Read(const CStr& _Filename);
{
	Error_static("CRegistry_Static::XRG_Read", "Not implemented!");
}

void CRegistry_Static::ReadSimple(CCFile* _pFile)
{
	Error_static("CRegistry_Static::ReadSimple", "Not implemented!");
}


void CRegistry_Static::ReadSimple(CStr _FileName)
{
	Error_static("CRegistry_Static::ReadSimple", "Not implemented!");
}


void CRegistry_Static::ReadRegistryDir(CStr _Dir, CCFile* _pFile)
{
	Error_static("CRegistry_Static::ReadRegistryDir", "Not implemented!");
}


void CRegistry_Static::ReadRegistryDir(CStr _Dir, CStr _Filename)
{
	Error_static("CRegistry_Static::ReadRegistryDir", "Not implemented!");
}


void CRegistry_Static::Read(CCFile* _pFile)
{
	Error_static("CRegistry_Static::Read", "Not implemented!");
}


void CRegistry_Static::Write(CCFile* _pFile) const
{
	Error_static("CRegistry_Static::Write", "Not implemented!");
}


bool CRegistry_Static::Read(CDataFile* _pDFile, const char* _pEntryName)
{
	Error_static("CRegistry_Static::Read", "Not implemented!");
}


void CRegistry_Static::Write(CDataFile* _pDFile, const char* _pEntryName) const
{
	Error_static("CRegistry_Static::Write", "Not implemented!");
}


void CRegistry_Static::Read(const char* _pFileName)
{
	Error_static("CRegistry_Static::Read", "Not implemented!");
}


void CRegistry_Static::Write(const char* _pFileName) const
{
	Error_static("CRegistry_Static::Write", "Not implemented!");
}








CStaticRegistryHandler::CStaticRegistryHandler(uint32 _Size)
 : m_nCapacity(_Size)
{
	m_pData = DNew(uint8) uint8[m_nCapacity];
	GetRoot()->Init(this);
	m_nSize = GetRoot()->GetSize();
}


CStaticRegistryHandler::~CStaticRegistryHandler()
{
	delete[] m_pData;
}


void CStaticRegistryHandler::UpdateSize(CRegistry_Static* _pElem, uint32 _nNewSize, uint32 _nOldSize)
{
	M_ASSERT((_nNewSize & 3) == 0, "Unaligned size!");
//	_nNewSize = (_nNewSize + 3) & ~3;
	if (_nOldSize != _nNewSize)
	{
		int nElemOffset = (uint32)_pElem - (uint32)m_pData;// + Min(_nNewSize, _nOldSize);
		M_ASSERT((nElemOffset & 3) == 0, "Unaligned object!");
		GetRoot()->UpdateOffsets(nElemOffset, _nNewSize - _nOldSize);

		uint8* pDest = (uint8*)_pElem + _nNewSize;
		uint8* pSrc  = (uint8*)_pElem + _nOldSize;
		int nBytesToMove = m_nSize - nElemOffset - _nOldSize;
		M_ASSERT(nBytesToMove >= 0, "!");
		memmove(pDest, pSrc, nBytesToMove);

		m_nSize += (_nNewSize - _nOldSize);
		M_ASSERT(m_nSize <= m_nCapacity, "UpdateSize: bad m_nSize!");
	}
}


void CStaticRegistryHandler::Elem_SetName(CRegistry_Static* _pElem, const char* _pName)
{
	int Len = (_pName && _pName[0]) ? strlen(_pName)+1 : 0;

	int nNewSize = CalcSize(Len, _pElem->m_ValueType, _pElem->m_nValueSize);
	UpdateSize(_pElem, nNewSize, _pElem->GetSize());
	_pElem->m_nNameSize = Len;

	if (Len > 0)
		strcpy(_pElem->GetNameStrData(), _pName);
}


void CStaticRegistryHandler::Elem_Set(CRegistry_Static* _pElem, const char* _pValue)
{
	int Len = (_pValue && _pValue[0]) ? strlen(_pValue)+1 : 0;

	int nNewSize = CalcSize(_pElem->m_nNameSize, e_Value_String, Len);
	UpdateSize(_pElem, nNewSize, _pElem->GetSize());
	_pElem->m_ValueType = e_Value_String;
	_pElem->m_nValueSize = Len;

	if (Len > 0)
		strcpy(_pElem->GetValueStrData(), _pValue);
}


void CStaticRegistryHandler::Elem_Set(CRegistry_Static* _pElem, int32 _nValue)
{
	int nNewSize = CalcSize(_pElem->m_nNameSize, e_Value_Int, 0);
	UpdateSize(_pElem, nNewSize, _pElem->GetSize());
	_pElem->m_ValueType = e_Value_Int;
	_pElem->m_nValue = _nValue;
}


void CStaticRegistryHandler::Elem_Set(CRegistry_Static* _pElem, fp32 _fValue)
{
	int nNewSize = CalcSize(_pElem->m_nNameSize, e_Value_Float, 0);
	UpdateSize(_pElem, nNewSize, _pElem->GetSize());
	_pElem->m_ValueType = e_Value_Float;
	_pElem->m_fValue = _fValue;
}


CRegistry_Static* CStaticRegistryHandler::Elem_Add(CRegistry_Static* _pElem, uint32 _Num, uint32 _Size)
{
	_Size = ((_Size+3)&~3);

	CRegistry_Static* pFirstNew = NULL;
	if (_pElem->m_nChildOffset == 0)
	{
		// create new child array
		_pElem->m_nChildOffset = (uint32)(m_pData + m_nSize) - (uint32)_pElem;
		pFirstNew = _pElem->GetChildFromIndex(0);
		m_nSize += _Num * _Size;
	}
	else
	{
		// expand existing child array
		CRegistry_Static* pLastChild = _pElem->GetChildFromIndex(_pElem->m_nChildren - 1);
		int nOldSize = pLastChild->GetSize();
		UpdateSize(pLastChild, nOldSize + _Num * _Size, nOldSize);
		pFirstNew = pLastChild->GetNext();
	}

	CRegistry_Static* pChild = pFirstNew;
	for (uint32 i=0; i<_Num; i++)
	{
		pChild->Init(this); //NOTE: If _Size isn't default-size, the element must be initalized to match the value of _Size
		pChild = pChild->GetNext();
	}
	_pElem->m_nChildren += _Num;
	return pFirstNew;
}


CRegistry_Static* CStaticRegistryHandler::Elem_Add(CRegistry_Static* _pElem, const char* _pChildName, const char* _pChildValue)
{
	int NameLen = (_pChildName && _pChildName[0]) ? strlen(_pChildName)+1 : 0;
	int ValueLen = (_pChildValue && _pChildValue[0]) ? strlen(_pChildValue)+1 : 0;
	CRegistry_Static* pNew = Elem_Add(_pElem, 1, sizeof(CRegistry_Static) + NameLen + ValueLen);

	pNew->m_nNameSize = NameLen;
	if (NameLen > 0)
		strcpy(pNew->GetNameStrData(), _pChildName);

	pNew->m_ValueType = e_Value_String;
	pNew->m_nValueSize = ValueLen;
	if (ValueLen > 0)
		strcpy(pNew->GetValueStrData(), _pChildValue);

	return pNew;
}


CRegistry_Static* CStaticRegistryHandler::Elem_Add(CRegistry_Static* _pElem, const char* _pChildName, int32 _nValue)
{
	int NameLen = (_pChildName && _pChildName[0]) ? strlen(_pChildName)+1 : 0;
	CRegistry_Static* pNew = Elem_Add(_pElem, 1, sizeof(CRegistry_Static) + NameLen);

	pNew->m_nNameSize = NameLen;
	if (NameLen > 0)
		strcpy(pNew->GetNameStrData(), _pChildName);

	pNew->m_ValueType = e_Value_Int;
	pNew->m_nValue = _nValue;
	return pNew;
}


CRegistry_Static* CStaticRegistryHandler::Elem_Add(CRegistry_Static* _pElem, const char* _pChildName, fp32 _fValue)
{
	int NameLen = (_pChildName && _pChildName[0]) ? strlen(_pChildName)+1 : 0;
	CRegistry_Static* pNew = Elem_Add(_pElem, 1, sizeof(CRegistry_Static) + NameLen);

	pNew->m_nNameSize = NameLen;
	if (NameLen > 0)
		strcpy(pNew->GetNameStrData(), _pChildName);

	pNew->m_ValueType = e_Value_Float;
	pNew->m_fValue = _fValue;
	return pNew;
}

