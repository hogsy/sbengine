#include "PCH.h"
#include "XRAnim.h"

#define DATAKEY_ALIGN(size) (((size) + 3) & ~3)

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_DataKey
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CXR_Anim_DataKey::SwapLE()
{
	::SwapLE(m_AbsTime);
	::SwapLE(m_Type);
	::SwapLE(m_nSize);
	::SwapLE(m_Param);

	SwapLE_Data();
}


void CXR_Anim_DataKey::SwapLE_Data()
{
	/*switch (m_Type)
	{  // Make sure to add SwapLE for types with 16/32/64-bit members in their data chunk!
	}*/
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_DataKey_Edit
|__________________________________________________________________________________________________
\*************************************************************************************************/
CXR_Anim_DataKey_Edit::CXR_Anim_DataKey_Edit(uint8 _Type, fp32 _AbsTime)
	: CXR_Anim_DataKey(_Type, _AbsTime)
{
	m_Data[0] = 0;
}

CXR_Anim_DataKey_Edit::CXR_Anim_DataKey_Edit(const CXR_Anim_DataKey& _Key)
	: CXR_Anim_DataKey(_Key)
{
	SetData(_Key.Data(), _Key.DataSize());
}


CXR_Anim_DataKey_Edit& CXR_Anim_DataKey_Edit::operator= (const CXR_Anim_DataKey& _Key)
{
	m_AbsTime = _Key.m_AbsTime;
	m_Type = _Key.m_Type;
	m_nSize = _Key.m_nSize;
	m_Param = _Key.m_Param;

	SetData(_Key.Data(), _Key.DataSize());
	return *this;
}


void CXR_Anim_DataKey_Edit::SetData(const char* _pData, uint8 _nSize)
{
	if (_nSize)
	{
		M_ASSERT(_pData, "wtf?!");
		M_ASSERT(_nSize < 64, "CXR_Anim_DataKey_Edit, data size too big!");
		memcpy(m_Data, _pData, _nSize);
		m_nSize = sizeof(CXR_Anim_DataKey) + _nSize;
	}
	else
	{
		m_nSize = sizeof(CXR_Anim_DataKey);
		MemSet(m_Data, 0, sizeof(m_Data));
	}
}


void CXR_Anim_DataKey_Edit::SetData(const CStr& _Data)
{
	M_ASSERT(_Data.IsAnsi(), "Non ansi string used for DataKey data");
	const char* pStr = _Data.Str();
	uint nLen = pStr ? (_Data.Len() + 1) : 0;
	SetData(pStr, nLen);
}


void CXR_Anim_DataKey_Edit::Read(CCFile* _pFile, uint _nVer)
{
	switch (_nVer)
	{
	case 0x0200:
	case 0x0201:
		{
			_pFile->ReadLE(m_AbsTime);
			_pFile->ReadLE(m_Type);
			_pFile->ReadLE(m_nSize);
			_pFile->ReadLE(m_Param);

			uint8 nDataSize = DataSize();
			if (nDataSize > 64)
				Error_static("CXR_Anim_DataKey_Edit::Read", "Too big extra data");

			if (nDataSize)
			{
				_pFile->Read(Data(), nDataSize);
				SwapLE_Data();

				if (_nVer >= 0x201) // Skip alignment bytes
					_pFile->RelSeek(DATAKEY_ALIGN(nDataSize) - nDataSize);
			}
		}
		break;

	default:
		Error_static("CXR_Anim_DataKey_Edit::Read", CStrF("Invalid version: %.4X", _nVer));
	}
}


void CXR_Anim_DataKey_Edit::Write(CCFile* _pFile) const
{
	_pFile->WriteLE(m_AbsTime);
	_pFile->WriteLE(m_Type);
	_pFile->WriteLE(m_nSize);
	_pFile->WriteLE(m_Param);

	uint8 nDataSize = DataSize();
	if (nDataSize)
	{
		const_cast<CXR_Anim_DataKey_Edit*>(this)->SwapLE_Data();
		_pFile->Write(Data(), DATAKEY_ALIGN(nDataSize));
		const_cast<CXR_Anim_DataKey_Edit*>(this)->SwapLE_Data();
	}
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_DataKeys
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CXR_Anim_DataKeys::RebuildIndex()
{
	uint nKeys = m_lpKeys.Len();
	CXR_Anim_DataKey* pCurr = (CXR_Anim_DataKey*)m_lData.GetBasePtr();
	for (uint iKey = 0; iKey < nKeys; iKey++)
	{
		m_lpKeys[iKey] = pCurr;
		pCurr = (CXR_Anim_DataKey*)((char*)pCurr + DATAKEY_ALIGN(pCurr->m_nSize));
	}
}


void CXR_Anim_DataKeys::Read(CCFile* _pFile, uint _nVer)
{
	MSCOPE(CXR_Anim_DataKeys::Read, ANIM_DATAKEY);
	switch (_nVer)
	{
	case 0x0200: // unaligned data
		{
			uint32 nKeys;
			_pFile->ReadLE(nKeys);
			if (nKeys > 10000)
				Error_static("CXR_Anim_DataKeys::Read", CStrF("Too many keys (%d - bad data?)", nKeys));
			m_lpKeys.SetLen(nKeys);

			uint32 nSize;
			_pFile->ReadLE(nSize);
			if (nSize > 30000)
				Error_static("CXR_Anim_DataKeys::Read", CStrF("Data size too large (%d bytes - bad data?)", nSize));
			TThinArray<uint8> DataArray;

			DataArray.SetLen(nSize);
			_pFile->Read(DataArray.GetBasePtr(), nSize);

			int nNeededSize = 0;
			const uint8* pSrc = DataArray.GetBasePtr();
			const uint8* pSrcEnd = pSrc + nSize;
			for (uint iKey = 0; pSrc < pSrcEnd; ++iKey)
			{
				CXR_Anim_DataKey *pCurr = (CXR_Anim_DataKey *)pSrc;
				nNeededSize += DATAKEY_ALIGN(pCurr->m_nSize);
				pSrc += DATAKEY_ALIGN(pCurr->m_nSize); //pCurr->m_nSize
			}
			m_lData.SetLen(nNeededSize);

			uint8* pDst = m_lData.GetBasePtr();
			pSrc = DataArray.GetBasePtr();
			for (uint iKey = 0; pSrc < pSrcEnd; ++iKey)
			{
				CXR_Anim_DataKey *pCurSrc = (CXR_Anim_DataKey *)pSrc;
				CXR_Anim_DataKey *pCurDst = (CXR_Anim_DataKey *)pDst;
				uint32 SizeAligned = DATAKEY_ALIGN(pCurSrc->m_nSize);
				memcpy(pCurDst, pCurSrc, pCurSrc->m_nSize);
				m_lpKeys[iKey] = pCurDst;
				pCurDst->SwapLE();
				nSize -= SizeAligned;
				pSrc += SizeAligned;//pCurSrc->m_nSize;
				pDst += SizeAligned;//DATAKEY_ALIGN(pCurSrc->m_nSize);
			}

			M_ASSERT(pDst == m_lData.GetBasePtr() + nNeededSize, "Something is amiss");

			if (nSize != 0)
				Error_static("CXR_Anim_DataKeys::Read", "Bad data! (size mismatch)");
		}
		break;

	case 0x201: // aligned data
		{
			uint32 nKeys;
			_pFile->ReadLE(nKeys);
			if (nKeys > 10000)
				Error_static("CXR_Anim_DataKeys::Read", CStrF("Too many keys (%d - bad data?)", nKeys));
			m_lpKeys.SetLen(nKeys);

			uint32 nSize;
			_pFile->ReadLE(nSize);
			if (nKeys > 30000)
				Error_static("CXR_Anim_DataKeys::Read", CStrF("Data size too large (%d bytes - bad data?)", nSize));
			m_lData.SetLen(nSize);

			_pFile->Read(m_lData.GetBasePtr(), nSize);

			CXR_Anim_DataKey* pCurr = (CXR_Anim_DataKey*)m_lData.GetBasePtr();
			for (uint iKey = 0; pCurr; ++iKey)
			{
				pCurr->SwapLE();
				m_lpKeys[iKey] = pCurr;
				nSize -= DATAKEY_ALIGN(pCurr->m_nSize);
				pCurr = (CXR_Anim_DataKey*)((char*)pCurr + DATAKEY_ALIGN(pCurr->m_nSize));
			}

			if (nSize != 0)
				Error_static("CXR_Anim_DataKeys::Read", "Bad data! (size mismatch)");
		}
		break;

	default:
		Error_static("CXR_Anim_DataKeys::Read", CStrF("Invalid version: %.4X", _nVer));
	}
}


void CXR_Anim_DataKeys::Write(CCFile* _pFile) const
{
	MSCOPE(CXR_Anim_DataKeys::Write, ANIM_DATAKEY);

	uint32 nKeys = m_lpKeys.Len();
	uint32 nSize = m_lData.Len();

	_pFile->WriteLE(nKeys);
	_pFile->WriteLE(nSize);

	for (uint i = 0; i < nKeys; i++)
	{
		CXR_Anim_DataKey_Edit Tmp;
		Tmp = *m_lpKeys[i];
		Tmp.Write(_pFile);
	}
}


void CXR_Anim_DataKeys::AddFromSequence(const CXR_Anim_DataKey_Sequence& _Seq)
{
	MSCOPE(CXR_Anim_DataKeys::AddFromSequence, ANIM_DATAKEY);

	uint nKeys = _Seq.GetNumKeys();
	CXR_Anim_DataKey*const* ppKeys = _Seq.GetKeys();

	uint nSize = 0;
	for (uint iKey = 0; iKey < nKeys; iKey++)
		nSize += DATAKEY_ALIGN(ppKeys[iKey]->m_nSize);

	uint nKeyOffset = m_lpKeys.Len();
	uint nDataOffset = m_lData.Len();

	m_lpKeys.SetLen( nKeyOffset + nKeys );
	m_lData.SetLen( nDataOffset + nSize );

	// Copy data
	for (uint iKey = 0; iKey < nKeys; iKey++)
	{
		CXR_Anim_DataKey* pDest = (CXR_Anim_DataKey*)(m_lData.GetBasePtr() + nDataOffset);
		memcpy(pDest, ppKeys[iKey], ppKeys[iKey]->m_nSize);
		nDataOffset += DATAKEY_ALIGN(ppKeys[iKey]->m_nSize);
	}

	// Rebuild index
	RebuildIndex();
}


uint CXR_Anim_DataKeys::GetDataSize() const
{
	return m_lData.ListSize();
}


CXR_Anim_DataKeys& CXR_Anim_DataKeys::operator=(const CXR_Anim_DataKeys& _DataKeys)
{
	MSCOPE(CXR_Anim_DataKeys::operator=, ANIM_DATAKEY);

	m_lData = _DataKeys.m_lData;
	m_lpKeys.SetLen( _DataKeys.m_lpKeys.Len() );
	RebuildIndex();
	return *this;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_DataKeys_Edit
|__________________________________________________________________________________________________
\*************************************************************************************************/
CXR_Anim_DataKeys_Edit::CXR_Anim_DataKeys_Edit()
{
}


CXR_Anim_DataKeys_Edit::~CXR_Anim_DataKeys_Edit()
{
}


void CXR_Anim_DataKeys_Edit::RebuildIndex()
{
	uint nKeys = m_lKeys.Len();
	CXR_Anim_DataKey_Edit* pKeys = m_lKeys.GetBasePtr();

	m_lpKeys.SetLen(nKeys);
	for (uint i = 0; i < nKeys; i++)
		m_lpKeys[i] = pKeys + i;
}


CXR_Anim_DataKeys_Edit& CXR_Anim_DataKeys_Edit::operator= (const CXR_Anim_DataKeys_Edit& _DataKeys)
{
	_DataKeys.m_lKeys.Duplicate(&m_lKeys);
	RebuildIndex();
	return *this;
}


void CXR_Anim_DataKeys_Edit::Read(CCFile* _pFile, uint _nVer)
{
	MSCOPE(CXR_Anim_DataKeys_Edit::Read, ANIM_DATAKEY);
	switch (_nVer)
	{
	case 0x0200: // unaligned data
	case 0x0201: // aligned data
		{
			uint32 nKeys, nSize;
			_pFile->ReadLE(nKeys);
			_pFile->ReadLE(nSize);
			if (nKeys > 10000)
				Error_static("CXR_Anim_DataKeys_Edit::Read", CStrF("Too many keys (%d - bad data?)", nKeys));

			m_lKeys.SetLen(nKeys);
			for (uint i = 0; i < nKeys; i++)
			{
				m_lKeys[i].Read(_pFile, _nVer);
				nSize -= DATAKEY_ALIGN(m_lKeys[i].m_nSize);
			}

			if (nSize != 0)
				Error_static("CXR_Anim_DataKeys_Edit::Read", "Bad data! (size mismatch)");

			for (uint i = 0; i < nKeys; i++)
			{
				if(m_lKeys[i].m_Type == ANIM_EVENT_TYPE_DIALOGUE)
				{	//Dialogues has lot of crap data stored in m_Data for some reason, so we validate it here to get rid of it save by save
					CStr StrData = m_lKeys[i].Data();
					int NeedGroundMaterial = StrData.Val_int();
					if(StrData.Len() > 1 || NeedGroundMaterial != 1)
						m_lKeys[i].SetData(NULL, 0);
				}
			}

			RebuildIndex();
		}
		break;

	default:
		Error_static("CXR_Anim_DataKeys_Edit::Read", CStrF("Invalid version: %.4X", _nVer));
	}
}


void CXR_Anim_DataKeys_Edit::Write(CCFile* _pFile) const
{
	MSCOPE(CXR_Anim_DataKeys_Edit::Write, ANIM_DATAKEY);

	uint32 nKeys = m_lpKeys.Len();
	uint32 nSize = 0;
	for (uint i = 0; i < nKeys; i++)
		nSize += DATAKEY_ALIGN(m_lKeys[i].m_nSize);

	_pFile->WriteLE(nKeys);
	_pFile->WriteLE(nSize);

	for (uint i = 0; i < nKeys; i++)
		m_lKeys[i].Write(_pFile);
}


uint CXR_Anim_DataKeys_Edit::GetNumEntries(fp32 _AbsTime) const
{
	uint nKeys = m_lpKeys.Len();
	CXR_Anim_DataKey* const* ppKeys = m_lpKeys.GetBasePtr();
	uint iKey, nEntries;
	for (iKey = 0, nEntries = 0; iKey < nKeys; ++iKey)
	{
		fp32 Diff = ppKeys[iKey]->m_AbsTime - _AbsTime;
		if (M_Fabs(Diff) < 0.0001f)
			nEntries++; // Found key!
		else if (ppKeys[iKey]->m_AbsTime > _AbsTime)
			break; // Went too far
	}
	return nEntries;
}


int CXR_Anim_DataKeys_Edit::FindKeyIndex(fp32 _AbsTime, uint _iEntry) const
{
	uint nKeys = m_lpKeys.Len();
	CXR_Anim_DataKey* const* ppKeys = m_lpKeys.GetBasePtr();
	uint iKey;
	for (iKey = 0; iKey < nKeys; ++iKey)
	{
		fp32 Diff = ppKeys[iKey]->m_AbsTime - _AbsTime;
		if (M_Fabs(Diff) < 0.0001f && _iEntry-- == 0)
			return iKey; // Found key!

		if (ppKeys[iKey]->m_AbsTime > _AbsTime)
			break;
	}
	return -(1 + int(iKey)); // not found, but the value is still useful
}


void CXR_Anim_DataKeys_Edit::Clear()
{
	m_lpKeys.Clear();
	m_lKeys.Clear();
}


void CXR_Anim_DataKeys_Edit::AddKey(const CXR_Anim_DataKey_Edit& _Key)
{
	int iKey = FindKeyIndex(_Key.m_AbsTime, uint(-1));
	M_ASSERT(iKey < 0, "wtf?"); // should be negative!

	iKey = -(1 + iKey);
	m_lKeys.Insert(iKey, _Key);
	RebuildIndex();
}


bool CXR_Anim_DataKeys_Edit::RemoveKey(fp32 _AbsTime, uint _iEntry)
{
	int iKey = FindKeyIndex(_AbsTime, _iEntry);
	if (iKey < 0)
		return false;

	m_lKeys.Del(iKey);
	RebuildIndex();
	return true;
}

void CXR_Anim_DataKeys_Edit::RemoveKey(int _iKey)
{
	m_lKeys.Del(_iKey);
	RebuildIndex();
}

uint CXR_Anim_DataKeys_Edit::AddKey(const CXR_Anim_DataKey1& _OldKey)
{
	// Convert old format key into new format...
	uint nAdded = 0;

	if (_OldKey.m_Sound != "")
	{
		CXR_Anim_DataKey_Edit key(ANIM_EVENT_TYPE_SOUND, _OldKey.m_AbsTime);
		key.SetData(_OldKey.m_Sound);
		AddKey(key);
		nAdded++;
	}

	if (_OldKey.m_iDialogue != 0)
	{
		CXR_Anim_DataKey_Edit key(ANIM_EVENT_TYPE_DIALOGUE, _OldKey.m_AbsTime);
		key.m_Param = _OldKey.m_iDialogue;
		AddKey(key);
		nAdded++;
	}

	if (_OldKey.m_EventParams[0] == CXR_Anim_DataKey1::ANIM_EVENT0TYPE_BREAKOUT)
	{
		CXR_Anim_DataKey_Edit key(ANIM_EVENT_TYPE_BREAKOUT, _OldKey.m_AbsTime);
		key.m_Param = _OldKey.m_EventParams[1];
		AddKey(key);
		nAdded++;
	}
	else if (_OldKey.m_EventParams[0] == CXR_Anim_DataKey1::ANIM_EVENT0TYPE_ENTRY)
	{
		CXR_Anim_DataKey_Edit key(ANIM_EVENT_TYPE_ENTRY, _OldKey.m_AbsTime);
		key.m_Param = _OldKey.m_EventParams[1];
		AddKey(key);
		nAdded++;
	}
	else if (_OldKey.m_EventParams[0] != 0 || _OldKey.m_EventParams[1] != 0)
	{
		LogFile(CStr("WARNING: Old event params not supported (data dropped)"));
	}
	return nAdded;
}


void CXR_Anim_DataKeys_Edit::AddFromSequence(const CXR_Anim_DataKey_Sequence& _Seq)
{
	MSCOPE(CXR_Anim_DataKeys_Edit::AddFromSequence, ANIM_DATAKEY);

	uint nKeys = _Seq.GetNumKeys();
	CXR_Anim_DataKey*const* ppKeys = _Seq.GetKeys();

	uint nOffset = m_lKeys.Len();
	m_lKeys.SetLen( nOffset + nKeys );

	// Copy data
	for (uint iKey = 0; iKey < nKeys; iKey++)
		m_lKeys[nOffset + iKey] = *ppKeys[iKey];

	// Rebuild index
	RebuildIndex();
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_DataKey_Sequence
|__________________________________________________________________________________________________
\*************************************************************************************************/


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_DataKey1 - for backwards compatibility
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CXR_Anim_DataKey1::Read(CCFile* _pF)
{
	MAUTOSTRIP(CXR_Anim_DataKey_Read, MAUTOSTRIP_VOID);
	uint32 Ver;
	_pF->ReadLE(Ver);
	switch(Ver)
	{
	case 0x100 :
		{
			_pF->ReadLE(m_AbsTime);				//NOTE: This is actually duration!
			m_Sound.Read(_pF);
			for(int i = 0; i < ANIM_DATAKEY_NUMPARAMS; i++)
				_pF->ReadLE(m_EventParams[i]);

			_pF->ReadLE(m_iDialogue);			// Sound-index for the sequence.
			int16 dummyInt;
			_pF->ReadLE(dummyInt);
		}
		break;

	case 0x101:
		{
			_pF->ReadLE(m_AbsTime);				//NOTE: This is actually duration!
			m_Sound.Read(_pF);
			for(int i = 0; i < ANIM_DATAKEY_NUMPARAMS; i++)
				_pF->ReadLE(m_EventParams[i]);
			_pF->ReadLE(m_iDialogue);			// Sound-index for the sequence.
		}
		break;

	default :
		Error_static("CXR_Anim_DataKey::Read", CStrF("Unsupported file-format version %.4x", Ver));
	}
}
