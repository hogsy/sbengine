
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/

#include "PCH.h"
#include "MDA.h"
#include "MFile.h"

/*#ifdef COMPILER_CODEWARRIOR
#include "CW_Breakpoint.h"
#endif*/

void ThinArray_DoMemException(uint _OldLen, uint _NewLen, uint _SizeOfT)
{
	M_TRACE("(TThinArrayDataAligned::Create) Failed allocation of %d objects, %d bytes. (sizeof=%d)\n", _NewLen, _NewLen*_SizeOfT, _SizeOfT);
	Error_static("TThinArray<>::SetLen", "Out of memory.");
}

//#define DEBUG_MEMORY

int g_GlobalArrayAlloc = 0;

CArrayData::CArrayData()
{
	m_Len = 0;
	m_AllocLen = 0;
	m_nGrow = TArray_DEFAULTGROW;
#ifndef M_RTM
	m_nAllocCount = 0;
#endif
	m_pList = NULL;
	m_ElemSize = 0;
	m_nRef = 0;
}

void* CArrayData::AllocObjects(int _nObjects)
{
#ifdef DEBUG_MEMORY
	g_GlobalArrayAlloc += _nObjects*m_ElemSize;
	if ((CArrayCore::m_GlobalFlags & TArray_MEMORYLOG | TArray_RECURSIONPROT) == TArray_MEMORYLOG)
	{
		CArrayCore::m_GlobalFlags |= TArray_RECURSIONPROT;
		LogFile(CStrF("Allocate %d, %d Total, %d %s", _nObjects*m_ElemSize, g_GlobalArrayAlloc, _nObjects, ClassName()));
		CArrayCore::m_GlobalFlags &= ~TArray_RECURSIONPROT;
	}
#endif
	return NULL;
}

void CArrayData::ResetObjects(void* _pObj, int _nElem)
{
}

void CArrayData::FreeScalar(void* _pObj, int _nElem)
{
#ifdef DEBUG_MEMORY
	g_GlobalArrayAlloc -= _nElem*m_ElemSize;
	if(_nElem) 
		if ((CArrayCore::m_GlobalFlags & TArray_MEMORYLOG | TArray_RECURSIONPROT) == TArray_MEMORYLOG)
		{
			CArrayCore::m_GlobalFlags |= TArray_RECURSIONPROT;
			LogFile(CStrF("Free %d, %d Total, %d %s", _nElem*m_ElemSize, g_GlobalArrayAlloc, _nElem, ClassName()));
			CArrayCore::m_GlobalFlags &= ~TArray_RECURSIONPROT;
		}
#endif
}

#define DLVCImp CObj
#include "MdaCListVectorCoreImp.h"
#undef DLVCImp
#define DLVCImp CArrayCoreDummy
#include "MdaCListVectorCoreImp.h"
#undef DLVCImp

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CIDHeap
|__________________________________________________________________________________________________
\*************************************************************************************************/


IMPLEMENT_OPERATOR_NEW(CIDHeap)


CIDHeap::CIDHeap()
{
	m_nAllocated = 0;
	m_bDynamic = false;
	m_MaxIDCapacity = 0;
	m_IDCapacity = 0;
	m_pIDAlloc = NULL;
	m_LenMinusOne = 0;
	m_AllocPos = 0;
}

CIDHeap::CIDHeap(int _MaxIDCapacity, bool _Dynamic)
{
	Create(_MaxIDCapacity, _Dynamic);
};

CIDHeap::~CIDHeap()
{
	m_lIDAlloc.Clear();
};

void CIDHeap::Create(int _MaxIDCapacity, bool _Dynamic)
{
	m_nAllocated = 0;
	m_bDynamic = _Dynamic;
	m_MaxIDCapacity = _MaxIDCapacity;
	if (m_MaxIDCapacity == 0)
		M_TRACEALWAYS("(CIDHeap::Create) WARNING: IDHeap capacity == 0, is this the intent?\n");
	if ((m_MaxIDCapacity & 31) != 0) Error("-", "Capacity must be 32X.")
	m_IDCapacity = (m_bDynamic) ? MinMT(256, m_MaxIDCapacity) : MaxMT(32, m_MaxIDCapacity);
	m_lIDAlloc.SetLen((m_IDCapacity >> 5) + 1);
	m_pIDAlloc = &m_lIDAlloc[0];
	m_LenMinusOne = m_lIDAlloc.Len() - 1;
	FillChar(&m_lIDAlloc[0], m_lIDAlloc.ListSize(), 0);
	m_AllocPos = 0;
}

int CIDHeap::AllocID()
{
	if (!m_pIDAlloc)
		M_BREAKPOINT;

	int32* pIDAlloc = m_pIDAlloc;
	int AllocArrSize = m_LenMinusOne;

	int Pos = m_AllocPos;
	uint32 Mask;
	while (((Mask = pIDAlloc[Pos]) == 0xffffffff)) Pos++;

	if (Pos >= AllocArrSize)
	{
		Pos = 0;
		while (((Mask = pIDAlloc[Pos]) == 0xffffffff)) Pos++;

		if (Pos >= AllocArrSize)
		{
			if (m_bDynamic)
			{
				if (m_IDCapacity == m_MaxIDCapacity) Error("AllocID", "Not allowed to expand ID heap!");
				int OldCap = m_IDCapacity;
				m_IDCapacity = MinMT(m_IDCapacity*2, m_MaxIDCapacity); 
				int NewLen = (m_IDCapacity >> 5) + 1;
				m_lIDAlloc.SetLen(NewLen);
				m_pIDAlloc = &m_lIDAlloc[0];
				m_LenMinusOne = m_lIDAlloc.Len() - 1;
				pIDAlloc = m_pIDAlloc;
				for (int i = (OldCap >> 5); i < NewLen; pIDAlloc[i++] = 0) {};
				Pos = OldCap >> 5;
				Mask = pIDAlloc[Pos];
			}
			else
			{
				return -1;
				Error("AllocID", "Out of IDs!");
			};
		};
	}
	m_AllocPos = Pos;
	int bit = BitScanFwd32(~Mask);
	
	if (bit < 0) 
		M_ASSERT(0, "?");

	//Error("AllocID", CStrF("Internal error: %.8X", m_lIDAlloc[m_AllocPos]));

	Mask |= (1 << bit);
	pIDAlloc[Pos] = Mask;
	m_nAllocated++;
	return (Pos*32 + bit);
};

#define ISFREE(Pos) (m_pIDAlloc[(Pos) >> 5] & (1 << ((Pos) & 0x1f)))

int CIDHeap::AllocIDRange(int _nIDs)
{
	if (m_bDynamic)
		Error("AllocIDRange", "Not supported.");

	if (!_nIDs)
		return -1;

	int Pos = m_AllocPos << 5;
	int StartPos = Pos;
	int nAlloc = 0;

	do
	{
		if (ISFREE(Pos))
			nAlloc++;
		else
			nAlloc = 0;

		if (nAlloc == _nIDs)
		{
			for(int i = 0; i < nAlloc; i++)
				ForceAllocID(Pos - i);
			m_AllocPos = Pos >> 5;
			return Pos-nAlloc+1;
		}
	
		Pos++;
		if (Pos >= m_IDCapacity)
		{
			Pos = 0;
			nAlloc = 0;
		}
	} 
	while(Pos != StartPos);

	return -1;
}


bool CIDHeap::ForceAllocID(int _ID)
{
	if (!m_pIDAlloc)
		M_BREAKPOINT;

	if ((_ID < 0) || (_ID >= m_IDCapacity)) Error("ForceAllocID", "ID out of range.");
	int pos = (_ID >> 5);
	int bit = (_ID & 31);
	if(m_pIDAlloc[pos] & (1 << bit))
		return false;
	else
	{
		m_pIDAlloc[pos] |= (1 << bit);
		m_nAllocated++;
		return true;
	}
}

void CIDHeap::FreeID(int _ID)
{
	if (!m_pIDAlloc)
		M_BREAKPOINT;
	int32* pIDAlloc = m_pIDAlloc;

	if ((_ID < 0) || (_ID >= m_IDCapacity)) Error("FreeID", "ID out of range.");
	int pos = (_ID >> 5);
	int bit = (_ID & 31);
	if (!(pIDAlloc[pos] & (1 << bit)))
#ifdef _DEBUG
	{
		M_ASSERT(0, "?");
	}
#else
	Error("FreeID", CStrF("ID %d is not allocated! (Capacity %d, Allocated %d)", _ID, m_IDCapacity, m_nAllocated));
#endif

	pIDAlloc[pos] &= ~(1 << bit);
	m_nAllocated--;
	
	// Prioritize allocation from the bottom.
	if (pos < m_AllocPos)
		m_AllocPos = pos;
};

int CIDHeap::IsAllocated(int _ID)
{
	if (!m_pIDAlloc)
		M_BREAKPOINT;
	if ((_ID < 0) || (_ID >= m_IDCapacity)) Error("FreeID", "ID out of range.");
	int pos = (_ID >> 5);
	int bit = (_ID & 31);
	return m_pIDAlloc[pos] & (1 << bit);
}

int CIDHeap::MaxAvail()
{
	return m_IDCapacity - m_nAllocated;
}

int CIDHeap::MaxCapacity()
{
	return m_MaxIDCapacity;
}

void CIDHeap::Read(CCFile* _pF)
{
	int32 Len;
	_pF->ReadLE(Len);
	m_lIDAlloc.SetLen(Len);
	for(int i = 0; i < Len; i++) _pF->ReadLE(m_lIDAlloc[i]);
//	_pF->ReadLE(m_lIDAlloc.GetBasePtr(), m_lIDAlloc.Len());
	m_pIDAlloc = m_lIDAlloc.GetBasePtr();
	m_LenMinusOne = m_lIDAlloc.Len()-1;

	_pF->ReadLE(m_bDynamic);
	_pF->ReadLE(m_AllocPos);
	_pF->ReadLE(m_MaxIDCapacity);
	_pF->ReadLE(m_IDCapacity);
	_pF->ReadLE(m_nAllocated);
}

void CIDHeap::Write(CCFile* _pF)
{
	int32 Len = m_lIDAlloc.Len();
	_pF->WriteLE(Len);
	for(int i = 0; i < Len; i++) _pF->WriteLE(m_lIDAlloc[i]);
//	_pF->WriteLE(m_lIDAlloc.GetBasePtr(), m_lIDAlloc.Len());
	_pF->WriteLE(m_bDynamic);
	_pF->WriteLE(m_AllocPos);
	_pF->WriteLE(m_MaxIDCapacity);
	_pF->WriteLE(m_IDCapacity);
	_pF->WriteLE(m_nAllocated);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CKeyContainer
|__________________________________________________________________________________________________
\*************************************************************************************************/


IMPLEMENT_OPERATOR_NEW(CKeyContainer);


CKeyContainer::CKeyContainer()
{
}

CKeyContainer::~CKeyContainer()
{
	m_lKeyNames.Destroy();
	m_lKeyValues.Destroy();
}

void CKeyContainer::Create(const TArray<CStr>& _lKeyNames, const TArray<CStr>& _lKeyValues)
{
	int nKeys = _lKeyNames.Len();
	if (_lKeyValues.Len() != nKeys) Error("-", "Number of keys/values mismatch.");

	m_lKeyNames.Clear();
	m_lKeyValues.Clear();
	m_lKeyNames.Add(&_lKeyNames);
	m_lKeyValues.Add(&_lKeyValues);

/*	for(int i = 0; i < nKeys; i++)
	{
		m_lKeyNames.Add(_lKeyNames[i]);
		m_lKeyValues.Add(_lKeyValues[i]);
	}*/
}

void CKeyContainer::operator= (const CKeyContainer& _KC)
{
	Create(_KC.m_lKeyNames, _KC.m_lKeyValues);
}

spCKeyContainer CKeyContainer::Duplicate() const
{
	spCKeyContainer spKC = MNew(CKeyContainer);
	if (!spKC) MemError("Duplicate");
	TArray<CStr> lKeyNames, lKeyValues;
	lKeyNames.SetLen(m_lKeyNames.Len());
	lKeyValues.SetLen(m_lKeyValues.Len());
	for(uint i = 0; i < lKeyNames.Len(); i++)
	{
		lKeyNames[i] = m_lKeyNames[i];
		lKeyValues[i] = m_lKeyValues[i];
	}
	spKC->Create(lKeyNames, lKeyValues);
	return spKC;
}

void CKeyContainer::AddScriptKey(const CStr _s)
{
/*	CStr s2 = _s;
	CStr Var = s2.GetBounded("\"");
	Var.MakeUpperCase();
	CStr Value = s2.GetBounded("\"");

	if (Var != "") AddKey(Var, Value);*/

	int len = _s.Len();
	int i = 0;
	while((i < len) && (_s[i] != '\"')) i++;	// "
	if (i >= len) return;
	int i1 = i++;

	while((i < len) && (_s[i] != '\"')) i++;	// "
	if (i >= len) return;
	int i2 = i++;

	while((i < len) && (_s[i] != '\"')) i++;	// "
	if (i >= len) return;
	int i3 = i++;

	while((i < len) && (_s[i] != '\"')) i++;	// "
	if (i >= len) return;
	int i4 = i++;

	CStr Var = CStr('*', i2-i1-1);
	CStr Value = CStr('*', i4-i3-1);

	memcpy((char*)Var, &((const char*)_s)[i1+1], i2-i1-1);
	Var.MakeUpperCase();
	memcpy((char*)Value, &((const char*)_s)[i3+1], i4-i3-1);
	AddKey(Var, Value);
}

int CKeyContainer::AddKey(const CStr _KeyName, const CStr _KeyValue)
{
	int i = GetKeyIndex(_KeyName);
	if (i >= 0)
	{
		m_lKeyValues[i] = _KeyValue;
		return i;
	}
	else
	{
		m_lKeyNames.Add(_KeyName);
		return m_lKeyValues.Add(_KeyValue);
	}
}

void CKeyContainer::SetKeyValue(int _iIndex, CStr _Value)
{
	if (_iIndex < 0 && _iIndex >= GetnKeys())
		Error("DeleteKeys", CStrF("Invalid key-index %d/%d", _iIndex, GetnKeys()) );
	m_lKeyValues[_iIndex] = _Value;
}

void CKeyContainer::DeleteKey(const CStr _KeyName)
{
	int iKey = GetKeyIndex(_KeyName);
	if (iKey < 0) return;
	m_lKeyNames.Del(iKey);
	m_lKeyValues.Del(iKey);
}

void CKeyContainer::DeleteKey(int _iIndex)
{
	if (_iIndex < 0 && _iIndex >= GetnKeys())
		Error("DeleteKeys", CStrF("Invalid key-index %d/%d", _iIndex, GetnKeys()) );
	m_lKeyNames.Del(_iIndex);
	m_lKeyValues.Del(_iIndex);
}

int CKeyContainer::GetnKeys() const
{
	return m_lKeyNames.Len();
}

int CKeyContainer::GetKeyIndex(CStr _Key) const
{
	int nKeys = m_lKeyNames.Len();
	for(int i = 0; i < nKeys; i++)
		if (m_lKeyNames[i].CompareNoCase(_Key) == 0) return i;
	return -1;
}

CStr CKeyContainer::GetKeyName(int _iKey) const
{
	return m_lKeyNames[_iKey];
}

CStr CKeyContainer::GetKeyValue(int _iKey) const
{
	return m_lKeyValues[_iKey];
}

void CKeyContainer::Write(CCFile* _pFile) const
{
	uint32 nKeys = m_lKeyNames.Len();
	_pFile->WriteLE(nKeys);
	for(uint32 i = 0; i < nKeys; i++)
	{
//		m_lKeyNames[i].Write(_pFile);
//		m_lKeyValues[i].Write(_pFile);

		uint32 lk = m_lKeyNames[i].Len();
		_pFile->WriteLE(lk);
		if (lk) _pFile->Write((const char*)m_lKeyNames[i], lk+1);

		uint32 lv = m_lKeyValues[i].Len();
		_pFile->WriteLE(lv);
		if (lv) _pFile->Write((const char*)m_lKeyValues[i], lv+1);
	}
}

void CKeyContainer::Read(CCFile* _pFile)
{
	m_lKeyNames.Clear();
	m_lKeyValues.Clear();

	CStr EmptyKey("");

	int32 nKeys = 0;
	_pFile->ReadLE(nKeys);
	m_lKeyNames.SetGrow( nKeys );
	m_lKeyValues.SetGrow( nKeys );
	for(int i = 0; i < nKeys; i++)
	{
//		m_lKeyNames[i].Read(_pFile);
//		m_lKeyValues[i].Read(_pFile);

		uint32 lk = 0;
		_pFile->ReadLE(lk);
		CFStr skey;
		_pFile->Read((char*)skey, lk+1);
		m_lKeyNames.Add((char *)skey);

		uint32 lv = 0;
		_pFile->ReadLE(lv);
		if(lv > 0)
		{
			CFStr sval;
			_pFile->Read((char*)sval, lv+1);
			m_lKeyValues.Add((char *)sval);
		}
		else
			m_lKeyValues.Add(EmptyKey);
	}
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CKeyContainerNode
|__________________________________________________________________________________________________
\*************************************************************************************************/


IMPLEMENT_OPERATOR_NEW(CKeyContainerNode);


CKeyContainerNode::CKeyContainerNode()
{
	m_spKeys = MNew(CKeyContainer);
	if (!m_spKeys) MemError("-");
	m_spData = MNew(CKeyContainer);
	if (!m_spData) MemError("-");
}

CKeyContainerNode::~CKeyContainerNode()
{
	m_spKeys = NULL;
	m_spData = NULL;
	m_lspSubKeys.Clear();
}

spCKeyContainerNode CKeyContainerNode::Duplicate() const
{
	spCKeyContainerNode spNode = MNew(CKeyContainerNode);
	if (!spNode) MemError("Duplicate");

	spNode->m_spKeys = m_spKeys->Duplicate();
	spNode->m_spData = m_spData->Duplicate();
	spNode->m_lspSubKeys.SetLen(m_lspSubKeys.Len());
	for(int i = 0; i < m_lspSubKeys.Len(); i++)
		spNode->m_lspSubKeys[i] = m_lspSubKeys[i]->Duplicate();

	return spNode;
}

spCKeyContainer CKeyContainerNode::GetKeys()
{
	return m_spKeys;
}

spCKeyContainer CKeyContainerNode::GetData()
{
	return m_spData;
}

int CKeyContainerNode::GetNumChildren()
{
	return m_lspSubKeys.Len();
}

spCKeyContainerNode CKeyContainerNode::GetChild(int _iChild)
{
	return m_lspSubKeys[_iChild];
}

void CKeyContainerNode::DeleteChild(int _iChild)
{
	m_lspSubKeys.Del(_iChild);
}

int CKeyContainerNode::AddChild(CKeyContainerNode *_pNode)
{
	return m_lspSubKeys.Add(_pNode);
}

CKeyContainerNode& CKeyContainerNode::operator = (CKeyContainerNode& _Other)
{
	m_spKeys	= _Other.m_spKeys;
	m_spData	= _Other.m_spData;
	int nSubKeys = _Other.m_lspSubKeys.Len();
	m_lspSubKeys.SetLen(nSubKeys);
	for(int i = 0; i < nSubKeys; i++)
		m_lspSubKeys[i] = _Other.m_lspSubKeys[i];

	return *this;
}

static int FindSeq(const char* p, int _Len, const char* pSeq, int _SeqLen)
{
	int i;
	for( i = 0; i < _Len-_SeqLen; i++)
	{
		int j;
		for(j = 0; j < _SeqLen; j++)
			if (p[i+j] != pSeq[j]) break;
		if (j == _SeqLen) return i;
	}
	return -1;
}

static int FindChar(const char* p, int _Len, char ch)
{
	for(int i = 0; i < _Len; i++)
		if (p[i] == ch) return i;
	return -1;
}
/*
CStr ParseEscSeq(const char* p, int& _Pos, int _Len)
{
	char Return[8192];
	nRet = 0;

	while(_Pos < _Len)
	{
		if (p[_Pos] == '\')
		{
			_Pos++;
			if (_Pos >= _Len) Error_static("::ParseEscSeq", "Unexpected end-of-file in escape-sequence (1).");

			switch(p[_Pos])
			{
			case '\' :
			case '"' :
				Return[nRet++] = p[_Pos];
				break;

			default :
				Error_static("::ParseEscSeq", "Invalid escape-code.");
			}
			_Pos++;
		}
		else if (p[_Pos] == '"')
		{
			CStr Ret;
			Ret.Capture(Return, nRet);
			_Pos++;
			return Ret;
		}
		else 
			Return[nRet++] = p[_Pos++];

		if (nRet >= 8191)
			Error_static("::ParseEscSeq", "Too long string.");
	}
}

bool NeedEscSeq(const char* p, int _Len)
{
	for(int i = 0; i < _Len; i++)
		if (p[i] == '"' || p[i] == '\\') return true;
	return false;
}

CStr CreateEscSeq(const char* p, int _Len)
{
	char Return[8192];
	nRet = 0;

	int Pos = 0;
	while(Pos < _Len)
	{
		if (p[Pos] == '"' || p[Pos] == '\')
		{
			Return[nRet++] = '\';
			Return[nRet++] = p[Pos];
		}
		else
			Return[nRet++] = p[Pos];

		Pos++;
	}

	CStr Ret;
	Ret.Capture(Return, nRet);
	return Ret;
}
*/
static CStr ParseQuote(const char* p, int& _Pos, int _Len, uint _Line)
{
	if (strncmp(p + _Pos, "\"{{{", 4) == 0)
	{
		_Pos += 4;
		int End = FindSeq(p + _Pos, _Len - _Pos, "}}}\"", 4);
		if (End < 0) Error_static("ParseQuote", CStrF("Line(%d) Unexpected end-of-file in quote.", _Line));
		CStr s;
		s.Capture(p + _Pos, End);
		_Pos += End + 4;
		return s;
	}
	else if (strncmp(p + _Pos, "\"", 1) == 0)
	{
		_Pos += 1;
		int End = FindSeq(p + _Pos, _Len - _Pos, "\"", 1);
		if (End < 0) Error_static("ParseQuote", CStrF("Line(%d) Unexpected end-of-file in quote.", _Line));
		CStr s;
		s.Capture(p + _Pos, End);
		_Pos += End + 1;
		return s;
	}
	else
	{
		Error_static("ParseQuote", CStrF("Line(%d) Not a quote.", _Line));
	}

	return CStr();  // <- doh., warning... SS
}

static void Parse_WhiteSpace(const char* p, int& _Pos, int _Len)
{
	while((_Pos < _Len) && CStr::IsWhiteSpace(p[_Pos])) _Pos++;
}

static uint Parse_Lines(const char* p, int _Pos, int _Len)
{
	uint Lines = 0;
	for(int i = 0; i < _Len; i++)
		if (p[i] == 10) Lines++;
	return Lines;
}

static void ParseComment_Semicolon(const char* p, int& _Pos, int _Len)
{
	int End = FindChar(p + _Pos, _Len - _Pos, 10);	// EOL can be 13, 10 or just 10.
	if (End < 0)
		_Pos = _Len;
	else
		_Pos += End+1;
}

static void ParseComment_SlashAstrix(const char* p, int& _Pos, int _Len, uint _Line)
{
	int End = FindSeq(p + _Pos, _Len - _Pos, "*/", 2);
	if (End < 0)
	{
		Error_static("ParseComment_SlashAstrix", CStrF("Line(%d) Unexpected end-of-file in /*  */ comment.", _Line));
	}
	else
		_Pos += End+2;
}

static void ParseComment_SlashSlash(const char* p, int& _Pos, int _Len)
{
	int End = FindChar(p + _Pos, _Len - _Pos, 10);	// EOL can be 13, 10 or just 10.
	if (End < 0)
		_Pos = _Len;
	else
		_Pos += End+1;
}

int CKeyContainerNode::ReadFromMemory(const char* _pStr, int _Size, bool _bEnterScope)
{
	uint Line = 1;
	return ReadFromMemory_r(_pStr, _Size, _bEnterScope, Line);
}

int CKeyContainerNode::ReadFromMemory_r(const char* _pStr, int _Len, bool _bEnterScope, uint& _Line)
{
	bool bInScope = !_bEnterScope;

	CStr Q[2];
	int nQ = 0;

	int Pos = 0;
	int PosStart = 0;
	while(Pos < _Len)
	{
		PosStart = Pos;
		Parse_WhiteSpace(_pStr, Pos, _Len);
		_Line += Parse_Lines(_pStr + PosStart, PosStart, Pos - PosStart);
		if (Pos >= _Len) break;
		
		if (strncmp(_pStr + Pos, ";", 1) == 0)
		{	// ;
			ParseComment_Semicolon(_pStr, Pos, _Len); _Line++;
		}

		else if (strncmp(_pStr + Pos, "//", 2) == 0)
		{	// //
			ParseComment_SlashSlash(_pStr, Pos, _Len); _Line++;
		}

		else if (strncmp(_pStr + Pos, "/*", 2) == 0)
		{	/* */
			PosStart = Pos;
			ParseComment_SlashAstrix(_pStr, Pos, _Len, _Line);
			_Line += Parse_Lines(_pStr + PosStart, PosStart, Pos - PosStart);
		}

		else if (strncmp(_pStr + Pos, "\"", 1) == 0)
		{
			// Quote ""
			if (!bInScope)
				Error("ReadFromMemory_r", CStrF("Line(%d): Unexpected quote outside scope.", _Line));

			PosStart = Pos;
			Q[nQ++] = ParseQuote(_pStr, Pos, _Len, _Line);
			_Line += Parse_Lines(_pStr + PosStart, PosStart, Pos - PosStart);
			if (nQ == 2)
			{
				Q[0].MakeUpperCase();
				m_spKeys->AddKey(Q[0], Q[1]);
				nQ = 0;
			}
		}
		else if (strncmp(_pStr + Pos, "{", 1) == 0)
		{
			// {
			Pos++;
			if (bInScope)
			{
				spCKeyContainerNode spNode = MNew(CKeyContainerNode);
				if (!spNode) MemError("ReadFromMemory_r");
				m_lspSubKeys.Add(spNode);
//				if (m_lspSubKeys.Len() > 64) m_lspSubKeys.SetGrow(256);
				m_lspSubKeys.SetGrow(Max(256, GetGEPow2(m_lspSubKeys.Len())));
				Pos += spNode->ReadFromMemory_r(&_pStr[Pos], _Len - Pos, false, _Line);
			}
			else
				bInScope = true;
		}
		else if (strncmp(_pStr + Pos, "}", 1) == 0)
		{
			// }
			Pos++;
			if (bInScope)
				return Pos;
			else
				Error("ReadFromMemory_r", CStrF("Line(%d): Unexpected '}'", _Line));
		}
		else if (strncmp(_pStr + Pos, "(", 1) == 0)
		{
			// Data (
			if (!bInScope)
				Error("ReadFromMemory_r", CStrF("Line(%d): Unexpected key-data outside scope.", _Line));

			int End = FindChar(_pStr + Pos, _Len - Pos, 10);	// EOL can be 13, 10 or just 10.
			if (End < 0) Error("ReadFromMemory_r", CStrF("Line(%d) Unexpected end-of-file in key-data.", _Line));

			_Line++;

			CStr s;
			s.Capture(&_pStr[Pos], End);
			m_spData->AddKey(CStrF("%d", m_spData->GetnKeys()), s);
			Pos += End+1;
		}
		else
		{
			// Syntax error
			int End = FindChar(_pStr + Pos, _Len - Pos, 10);	// EOL can be 13, 10 or just 10.
			CStr s;
			if (End < 0)
				s.Capture(_pStr + Pos, _Len - Pos);
			else
				s.Capture(_pStr + Pos, End);
			Error("ReadFromMemory_r", CStrF("(Line: %d) Syntax error: %s", _Line, s.GetStr()));
			_Line++;
		}
	}

//	if (!_bEnterScope && bInScope)
//		Error("ReadFromMemory_r", "Unexpected end-of-file in scope.");

	return Pos;
}

#ifdef ALDRIG_I_LIVET

int CKeyContainerNode::ReadFromMemory_r(const char* _pStr, int _Size, bool _bEnterScope)
{
	bool bInScope = !_bEnterScope;

	int Pos = 0;
	int PosNextLine = 0;

	while(PosNextLine < _Size)
	{
		Pos = PosNextLine;
		int PosE = Pos;
		// Find EOL.
		while((PosE < _Size) && (_pStr[PosE] != char(10))) PosE++;
		PosNextLine = PosE+1;

		// Find last char.
		while((PosE > Pos) && CStr::IsWhiteSpace(_pStr[PosE-1])) PosE--;

		// Find first char.
		while((Pos < PosE) && CStr::IsWhiteSpace(_pStr[Pos])) Pos++;

		// Anything left?
		if (PosE - Pos <= 0) continue;

/*		CStr s = _pFile->Readln();
		s.Trim();
		if (!s.Len()) continue;*/

		if (_pStr[Pos] == ';') continue;

		{
			int PosComment = FindSeq(&_pStr[Pos], PosE - Pos, "////", 2);
//			if (PosComment >= 0)
			if (PosComment == 0)
				PosE = Pos + PosComment;
			if (PosE - Pos <= 0) continue;
		}

//		CStr s;
//		s.Capture(&_pStr[Pos], PosE - Pos);
//LogFile(s);

/*		if (_pStr[Pos] == '/' && 
			_pStr[Pos+1] == '*')
		{
		}*/

		// Filter out comments
/*		if (s[0] == ';') continue;
		if (s.Find("////") >= 0)
		{
			s = s.GetStrSep("////");
			s.Trim();
			if (!s.Len()) continue;
		}
*/
		int Len = PosE - Pos;
		if (Len == 1 && _pStr[Pos] == '{')
		{
			if (bInScope)
			{
				spCKeyContainerNode spNode = DNew(CKeyContainerNode) CKeyContainerNode;
				if (!spNode) MemError("ReadFromText");
				m_lspSubKeys.Add(spNode);
//				if (m_lspSubKeys.Len() > 64) m_lspSubKeys.SetGrow(256);
				m_lspSubKeys.SetGrow(Max(256, GetGEPow2(m_lspSubKeys.Len())));
				PosNextLine += spNode->ReadFromMemory_r(&_pStr[PosNextLine], _Size-PosNextLine, false);
			}
			else
				bInScope = true;
		}
		else if (Len == 1 && _pStr[Pos] == '}')
		{
			if (bInScope)
				return PosNextLine;
			else
				Error("ReadFromText", "Unexpected '}'");
		}
		else if (FindChar(&_pStr[Pos], PosE - Pos, '"') >= 0)
		{
			if (bInScope)
			{
				CStr s;
				s.Capture(&_pStr[Pos], PosE - Pos);
				m_spKeys->AddScriptKey(s);
			}
		}
		else
		{
			if (bInScope)
			{
				CStr s;
				s.Capture(&_pStr[Pos], PosE - Pos);
				m_spData->AddKey(CStrF("%d", m_spData->GetnKeys()), s);
			}
		}


		//----------------------------------------------------
/*		if (s == "{")
		{
			if (bInScope)
			{
				spCKeyContainerNode spNode = DNew(CKeyContainerNode) CKeyContainerNode;
				if (!spNode) MemError("ReadFromText");
				m_lspSubKeys.Add(spNode);
				PosNextLine += spNode->ReadFromMemory_r(&_pStr[PosNextLine], _Size-PosNextLine, false);
			}
			else
				bInScope = true;
		}
		else if (s == "}")
		{
			if (bInScope)
				return PosNextLine;
			else
				Error("ReadFromText", "Unexpected '}'");
		}
		else if (s.Find("\"") >= 0)
		{
			if (bInScope)
				m_spKeys->AddScriptKey(s);
		}
		else
		{
			if (bInScope)
				m_spData->AddKey(CStrF("%d", m_spData->GetnKeys()), s);
		}*/
	}
	return PosNextLine;
}

#endif

void CKeyContainerNode::ReadFromScript(CCFile* _pFile, bool _bEnterScope)
{
	TArray<char> lFile;

	int len = _pFile->Length() - _pFile->Pos();
	lFile.SetLen(len);
	_pFile->Read(lFile.GetBasePtr(), len);

	uint Line = 1;
	ReadFromMemory_r(lFile.GetBasePtr(), len, _bEnterScope, Line);

/*	bool bInScope = !_bEnterScope;

	while(!_pFile->EndOfFile())
	{
		CStr s = _pFile->Readln();
		s.Trim();
		if (!s.Len()) continue;

		// Filter out comments
		if (s[0] == ';') continue;
		if (s.Find("////") >= 0)
		{
			s = s.GetStrSep("////");
			s.Trim();
			if (!s.Len()) continue;
		}

		if (s == "{")
		{
			if (bInScope)
			{
				spCKeyContainerNode spNode = DNew(CKeyContainerNode) CKeyContainerNode;
				if (!spNode) MemError("ReadFromText");
				m_lspSubKeys.Add(spNode);
				spNode->ReadFromScript(_pFile, false);
			}
			else
				bInScope = true;
		}
		else if (s == "}")
		{
			if (bInScope)
				return;
			else
				Error("ReadFromText", "Unexpected '}'");
		}
		else if (s.Find("\"") >= 0)
		{
			if (bInScope)
				m_spKeys->AddScriptKey(s);
		}
		else
		{
			if (bInScope)
				m_spData->AddKey(CStrF("%d", m_spData->GetnKeys()), s);
		}
	}*/
}

void CKeyContainerNode::ReadFromScript(CStr _Filename, bool _bEnterScope)
{
	MSCOPE(CKeyContainerNode::ReadFromScript, OGIER);
	CCFile File;
	File.Open(_Filename, CFILE_READ);
	ReadFromScript(&File, _bEnterScope);
	File.Close();
}

void CKeyContainerNode::WriteToDataFile(CDataFile* _pDFile)
{
	_pDFile->BeginEntry("NODEDATA");
	m_spKeys->Write(_pDFile->GetFile());
	m_spData->Write(_pDFile->GetFile());
	_pDFile->EndEntry(m_lspSubKeys.Len());

	for(int i = 0; i < m_lspSubKeys.Len(); i++)
	{
		_pDFile->BeginEntry("NODE");
		_pDFile->EndEntry(0);
		_pDFile->BeginSubDir();
			m_lspSubKeys[i]->WriteToDataFile(_pDFile);
		_pDFile->EndSubDir();
	}
}

void CKeyContainerNode::WriteToScript(CCFile* _pFile, int _RecurseDepth, bool _bIndent)
{
	CStr IndBrackets;
	CStr Ind(char(9), 1);
	if (_bIndent)
	{
		Ind = CStr(char(9), _RecurseDepth+1);
		IndBrackets = CStr(char(9), _RecurseDepth);
	}

	_pFile->Writeln(IndBrackets + "{");

	{
		int nKeys = m_spKeys->GetnKeys();
		for(int i = 0; i < nKeys; i++)
		{
			CStr Key = m_spKeys->GetKeyName(i);
			CStr Value = m_spKeys->GetKeyValue(i);
			if (Key.Find("\"") >= 0 ||
				Value.Find("\"") >= 0)
			{
				if (Key.Find("\"") >= 0)
					_pFile->Writeln(CStrF("%s\"{{{%s}}}\" \"{{{%s}}}\"", (char*)Ind, (char*)m_spKeys->GetKeyName(i), (char*)m_spKeys->GetKeyValue(i) ));
				else
					_pFile->Writeln(CStrF("%s\"%s\" \"{{{%s}}}\"", (char*)Ind, (char*)m_spKeys->GetKeyName(i), (char*)m_spKeys->GetKeyValue(i) ));
			}
			else
				_pFile->Writeln(CStrF("%s\"%s\" \"%s\"", (char*)Ind, (char*)m_spKeys->GetKeyName(i), (char*)m_spKeys->GetKeyValue(i) ));
		}
	}
	{
		int nKeys = m_spData->GetnKeys();
		for(int i = 0; i < nKeys; i++)
			_pFile->Writeln(Ind + m_spData->GetKeyValue(i));
	}

	for(int i = 0; i < m_lspSubKeys.Len(); i++)
		m_lspSubKeys[i]->WriteToScript(_pFile, _RecurseDepth+1, _bIndent);

	_pFile->Writeln(IndBrackets + "}");
}

void CKeyContainerNode::WriteToScript(CStr _Filename)
{
	MSCOPE(CKeyContainerNode::WriteToScript, OGIER);
	CCFile File;
	M_TRY
	{
		File.Open(_Filename, CFILE_WRITE);
	}
	M_CATCH(
	catch(CCException _Ex)
	{
		throw;
	}
	)
	WriteToScript(&File);
	File.Close();
}

// Operations
void CKeyContainerNode::DeleteData()
{
	m_spData = MNew(CKeyContainer);
	if (!m_spData) MemError("-");

	for(int i = 0; i < m_lspSubKeys.Len(); i++)
		m_lspSubKeys[i]->DeleteData();
}

void CKeyContainerNode::DeleteKeys()
{
	m_spKeys = MNew(CKeyContainer);
	if (!m_spKeys) MemError("-");

	for(int i = 0; i < m_lspSubKeys.Len(); i++)
		m_lspSubKeys[i]->DeleteKeys();
}

void CKeyContainerNode::DeleteKeys(CStr _StartingWith, CStr _Containing)
{
	int slen = _StartingWith.Len();
	int clen = _Containing.Len();
	{
		int nKeys = m_spKeys->GetnKeys();
//LogFile(CStrF("DeleteKeys: %d, %d, %d, ", slen, clen, nKeys) + _StartingWith + ", " + _Containing);
		for(int i = nKeys-1; i >= 0; i--)
		{
			CStr Key = m_spKeys->GetKeyName(i);

			if (slen)
			{
				if ((Key.Copy(0, slen)) == _StartingWith)
				{
					m_spKeys->DeleteKey(Key);
					continue;
				}
			}

			if (clen)
			{
				if (Key.Find(_Containing) != -1)
				{
					m_spKeys->DeleteKey(Key);
					continue;
				}
			}
		}
	}

	for(int i = 0; i < m_lspSubKeys.Len(); i++)
		m_lspSubKeys[i]->DeleteKeys(_StartingWith, _Containing);
}

CKeyContainerNode* CKeyContainerNode::FindContainer(CStr _Key, CStr _Value)
{
	int iKey = m_spKeys->GetKeyIndex(_Key);
	if (iKey >= 0)
		if (m_spKeys->GetKeyValue(iKey) == _Value) return this;

	for(int i = 0; i < m_lspSubKeys.Len(); i++)
	{
		CKeyContainerNode* pN = m_lspSubKeys[i]->FindContainer(_Key, _Value);
		if (pN) return pN;
	}
	return NULL;
}

