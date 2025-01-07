
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
#include "MCC.h"
#include "MMemMgrHeap.h"
#include "MDA_Hash.h"
#include "MDA_Hash2D.h"

// -------------------------------------------------------------------
//  CHash2D
// -------------------------------------------------------------------
CHash2D::CHash2D()
{
	m_BoxSize = 0;
	m_BoxShiftSize = 0;
	m_BoxAndSize = 0;
	m_nBoxAndX = 0;
	m_nBoxAndY = 0;
	m_nBoxesX = 0;
	m_nBoxesY = 0;
	m_nBoxes = 0;
	m_HashAndSizeX = 0;
	m_HashAndSizeY = 0;
	m_iFirstLarge = -1;
}

CHash2D::~CHash2D()
{
}

void CHash2D::Create(int _nBoxes, int _BoxShiftSize, int _MaxIDs, bool _bUseLarge)
{
	m_BoxShiftSize = _BoxShiftSize;
	m_BoxSize = (1 << m_BoxShiftSize);
	m_BoxAndSize = m_BoxSize-1;

	m_iFirstLarge = -1;

	m_nBoxesX = _nBoxes;
	m_nBoxesY = _nBoxes;
	m_nBoxes = m_nBoxesX*m_nBoxesY;
	m_HashAndSizeX = m_nBoxesX*m_BoxSize-1;
	m_HashAndSizeY = m_nBoxesY*m_BoxSize-1;

	m_nBoxAndX = m_nBoxesX - 1;
	m_nBoxAndY = m_nBoxesY - 1;

	// Call super
	THash<int32, CHash2DNull>::Create(_MaxIDs, m_nBoxes, _bUseLarge);
}

void CHash2D::Insert(int _ID, const CVec3Dfp32& _Min, const CVec3Dfp32& _Max)
{
	if (!m_pHash) Error("Insert", "Not initialized.");
	// Object med radie större än lådstorleken hamnar i large-listan.
	if ((_ID < 0) || (_ID > m_MaxIDs)) Error("Insert", CStrF("ID out of range. (%d/%d)", _ID, m_MaxIDs));
	
	CHashIDInfo* pID = &m_pIDInfo[_ID];
	if (pID->m_Flags & (THASH_HASH | THASH_LARGE)) Remove(_ID);
	
	int MaxSize = 0;
	if (m_bUseLarge)
	{
		if (_Max.k[0] - _Min.k[0] + 1.0f > MaxSize) MaxSize = TruncToInt(_Max.k[0] - _Min.k[0] + 1.0f);
		if (_Max.k[1] - _Min.k[1] + 1.0f > MaxSize) MaxSize = TruncToInt(_Max.k[1] - _Min.k[1] + 1.0f);
	}
	
	int x = TruncToInt((_Max.k[0] + _Min.k[0]) * 0.5f);
	int y = TruncToInt((_Max.k[1] + _Min.k[1]) * 0.5f);
	
	int iBucket =
		((x & m_HashAndSizeX) >> m_BoxShiftSize) +
		((y & m_HashAndSizeY) >> m_BoxShiftSize) * m_nBoxesX;
	
	//	pID->m_pObj = _pObj;
	
	if (!m_bUseLarge || MaxSize < m_BoxSize)
		THash<int32, CHash2DNull>::Insert(_ID, iBucket);
	else
		THash<int32, CHash2DNull>::InsertLarge(_ID);
}

int CHash2D::EnumerateBox(const CVec3Dfp32& _Min, const CVec3Dfp32& _Max, int32* _pEnumRetIDs, int _MaxEnumIDs)
{
	if (!m_pHash) Error("Insert", "Not initialized.");
	int nIDs = 0;
	
	// Add all large IDs
	if (m_bUseLarge)
	{
		int ID = m_iFirstLarge;
		while(ID != -1)
		{
			if (nIDs >= _MaxEnumIDs)
				Error("EnumerateBox", CStrF("Out of enumeration-space. (1) (%d/%d)", nIDs, _MaxEnumIDs));
			
			int CurID(ID);
			ID = m_pIDInfo[ID].m_iNext;
			
			/*		CWObject* pObj = m_pIDInfo[CurID].m_pObj;
			const CBox3Dfp32* pBox = pObj->GetAbsBoundBox();
			if (pBox->m_Min.k[0] > _Max.k[0]) continue;
			if (pBox->m_Min.k[1] > _Max.k[1]) continue;
			if (pBox->m_Min.k[2] > _Max.k[2]) continue;
			if (pBox->m_Max.k[0] < _Min.k[0]) continue;
			if (pBox->m_Max.k[1] < _Min.k[1]) continue;
			if (pBox->m_Max.k[2] < _Min.k[2]) continue;*/
			
			//LogFile(CStrF("Added %d, %d", CurID, nIDs));
			
			_pEnumRetIDs[nIDs] = CurID;
			nIDs++;
		}
	}
	
	
	// Add all hashed IDs within the box.
	int x1, x2, y1, y2;
	if (_Max.k[0] - _Min.k[0] + 3.0f + m_BoxSize*2 > m_nBoxesX*m_BoxSize)
	{
		x1 = 0;
		x2 = m_nBoxesX-1;
	}
	else
	{
		x1 = ((int(_Min.k[0] - m_BoxSize) >> m_BoxShiftSize) - 1) & m_nBoxAndX;
		x2 = ((int(_Max.k[0] + m_BoxSize) >> m_BoxShiftSize) + 1) & m_nBoxAndX;
		if (x2 == x1)
		{
			x1 = 0;
			x2 = m_nBoxesX-1;
		}
	}

	if (_Max.k[1] - _Min.k[1] + 3.0f + m_BoxSize*2 > m_nBoxesY*m_BoxSize)
	{
		y1 = 0;
		y2 = m_nBoxesX-1;
	}
	else
	{
		y1 = ((int(_Min.k[1] - m_BoxSize) >> m_BoxShiftSize) - 1) & m_nBoxAndY;
		y2 = ((int(_Max.k[1] + m_BoxSize) >> m_BoxShiftSize) + 1) & m_nBoxAndY;
		if (y2 == y1)
		{
			y1 = 0;
			y2 = m_nBoxesY-1;
		}
	}

	int y = y1;
	while(1)
	{
		int x = x1;
		while(1)
		{

			int ID = m_pHash[y * m_nBoxesX + x];
			while(ID != -1)
			{
				if (nIDs >= _MaxEnumIDs)
					Error("EnumerateBox", CStrF("Out of enumeration-space. (2) (%d/%d)", nIDs, _MaxEnumIDs));

				int CurID(ID);
				ID = m_pIDInfo[ID].m_iNext;

/*				CWObject* pObj = m_pIDInfo[CurID].m_pObj;
				const CBox3Dfp32* pBox = pObj->GetAbsBoundBox();
				if (pBox->m_Min.k[0] > _Max.k[0]) continue;
				if (pBox->m_Min.k[1] > _Max.k[1]) continue;
				if (pBox->m_Min.k[2] > _Max.k[2]) continue;
				if (pBox->m_Max.k[0] < _Min.k[0]) continue;
				if (pBox->m_Max.k[1] < _Min.k[1]) continue;
				if (pBox->m_Max.k[2] < _Min.k[2]) continue;*/

				_pEnumRetIDs[nIDs++] = CurID;
			}
			if (x == x2) break;
			x = (x + 1) & m_nBoxAndX;
		}

		if (y == y2) break;
		y = (y + 1) & m_nBoxAndY;
	}

	return nIDs;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CStringHash
|__________________________________________________________________________________________________
\*************************************************************************************************/
static M_INLINE uint32 StringHash_GetHashIndex(const char* _pStr, bool _bCaseSensitive, uint32 _Mask)
{
	if (!_pStr)
		return 0;

	uint32 Hash = 5381;
	if (_bCaseSensitive)
	{
		for (uint i=0; _pStr[i] != 0; i++)
			Hash = Hash*33 + _pStr[i];
	}
	else
	{
		for (uint i=0; _pStr[i] != 0; i++)
			Hash = Hash*33 + CStrBase::clwr(_pStr[i]);
	}
	Hash -= 5381;	// So empty string == 0
	return Hash & _Mask;
}


int CStringHash::GetHashIndex(const char* _pStr)
{
	return StringHash_GetHashIndex(_pStr, m_bCaseSensitive, m_HashAnd);
}


void CStringHash::Create(int _nIndices, bool _bCaseSensitive, int _HashShiftSize)
{
	m_HashAnd = (1 << _HashShiftSize) - 1;
	m_bCaseSensitive = _bCaseSensitive;
	THash<int16, CStringHashElement>::Create(_nIndices, (1 << _HashShiftSize), false);
}

void CStringHash::Insert(int _Index, CStr _Str)
{
	THash<int16, CStringHashElement>::Remove(_Index);
	THash<int16, CStringHashElement>::Insert(_Index, GetHashIndex(_Str));
	CHashIDInfo* pID = &m_pIDInfo[_Index];
	pID->m_Str = _Str;
}

int CStringHash::GetIndex(const char* _pStr)
{
	int ID = m_pHash[GetHashIndex(_pStr)];
	while(ID != -1)
	{
		if (m_bCaseSensitive)
		{	if (m_pIDInfo[ID].m_Str.Compare(_pStr) == 0) return ID; }
		else
		{	if (m_pIDInfo[ID].m_Str.CompareNoCase(_pStr) == 0) return ID; }

		ID = m_pIDInfo[ID].m_iNext;
	}

	return -1;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CStringHashConst
|__________________________________________________________________________________________________
\*************************************************************************************************/


int CStringHashConst::GetHashIndex(const char* _pStr)
{
	return StringHash_GetHashIndex(_pStr, m_bCaseSensitive, m_HashAnd);
}


void CStringHashConst::Create(int _nIndices, bool _bCaseSensitive, int _HashShiftSize)
{
	m_HashAnd = (1 << _HashShiftSize) - 1;
	m_bCaseSensitive = _bCaseSensitive;
	THash<int16, CStringHashConstElement>::Create(_nIndices, (1 << _HashShiftSize), false);
}

void CStringHashConst::Insert(int _Index, const char* _pStr)
{
	THash<int16, CStringHashConstElement>::Remove(_Index);
	CHashIDInfo* pID = &m_pIDInfo[_Index];
	if (m_bCaseSensitive)
	{
		THash<int16, CStringHashConstElement>::Insert(_Index, GetHashIndex(_pStr));
		pID->m_pStr = _pStr;
	}
	else
	{
		uint32 Hash = StringToHash(_pStr);
		int iPrev = GetIndex(Hash);
		if ((iPrev != -1) && (iPrev != _Index))
		{
#ifndef M_RTM
			if (CStrBase::CompareNoCase(_pStr, m_pIDInfo[iPrev].m_pStrDebug))
			{
				uint32 Hash2 = StringToHash(m_pIDInfo[iPrev].m_pStrDebug);
				Error("Insert", CStrF("String hash for '%s' (Hash %08x, Id %d) and '%s' (Hash %08x, Id %d) colliding", _pStr, Hash, _Index, m_pIDInfo[iPrev].m_pStrDebug, Hash2, iPrev));
			}
#else
//			Error("Insert", CStrF("String hash not unique for '%s'", _pStr));
#endif
			return;
		}
		THash<int16, CStringHashConstElement>::Insert(_Index, Hash & m_HashAnd);
		pID->m_StrHash = Hash;
	}
#ifndef M_RTM
	pID->m_pStrDebug = _pStr;
#endif
}

int CStringHashConst::GetIndex(const char* _pStr)
{
	if (m_bCaseSensitive)
	{
		int ID = m_pHash[GetHashIndex(_pStr)];
		while(ID != -1)
		{
			if (CStrBase::Compare(m_pIDInfo[ID].m_pStr, _pStr) == 0)
				return ID;
			ID = m_pIDInfo[ID].m_iNext;
		}
	}
	else
	{
		uint32 Hash = StringToHash(_pStr);
		int ID = m_pHash[Hash & m_HashAnd];
		while(ID != -1)
		{
			if (m_pIDInfo[ID].m_StrHash == Hash)
				return ID;
			ID = m_pIDInfo[ID].m_iNext;
		}
	}

	return -1;
}

int CStringHashConst::GetIndex(uint32 _StrHash)
{
	if (m_bCaseSensitive)
		Error("GetIndex", "Not supported with case-sensitive hashing.");

	int ID = m_pHash[_StrHash & m_HashAnd];
	while(ID != -1)
	{
		if (m_pIDInfo[ID].m_StrHash == _StrHash)
			return ID;
		ID = m_pIDInfo[ID].m_iNext;
	}

	return -1;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMap16
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CMap16::Create(int16 _MaxID, int _HashBits)
{
	m_MaxIDs = 0;
	m_pHash = NULL;
	m_pIDInfo = NULL;
	m_iFirstLarge = -1;
	m_bUseLarge = false;

	THash<int16, CMap16Entry>::Create(_MaxID, 1 << _HashBits, false);

	m_HashBits = _HashBits;
	m_HashAnd = (1 << _HashBits) - 1;
}

void CMap16::Insert(int16 _ID, uint16 _Value)
{
	THash<int16, CMap16Entry>::Remove(_ID);

	// Validate
/*	{
		if (m_pIDInfo[_ID].m_Flags != 0)
		{
			M_BREAKPOINT;
		}


		int iHash = _Value & m_HashAnd;
		const CMap16::CHashIDInfo* pIDInfo = m_pIDInfo;

		int ID = m_pHash[iHash];
		while(ID != -1)
		{
			const CHashIDInfo& IDInfo = pIDInfo[ID];
			if (ID == IDInfo.m_iNext)
			{
				M_BREAKPOINT;
			}

			ID = IDInfo.m_iNext;
		}
	}*/


	THash<int16, CMap16Entry>::Insert(_ID, _Value & m_HashAnd);
	m_pIDInfo[_ID].m_Value = _Value;

	// Validate
/*	{
		int iHash = _Value & m_HashAnd;
		const CMap16::CHashIDInfo* pIDInfo = m_pIDInfo;

		int ID = m_pHash[iHash];
		while(ID != -1)
		{
			const CHashIDInfo& IDInfo = pIDInfo[ID];
			if (ID == IDInfo.m_iNext)
			{
				M_BREAKPOINT;
			}

			ID = IDInfo.m_iNext;
		}
	}*/
}

bool CMap16::GetValue(int16 _ID, uint16& _RetValue) const
{
	M_ASSERT(_ID >= 0 && _ID < m_MaxIDs, "!");

	CHashIDInfo* pID = &m_pIDInfo[_ID];
	if (pID->m_Flags & THASH_HASH)
	{
		_RetValue = pID->m_Value;
		return true;
	}
	else
		return false;
}

int16 CMap16::GetIndex(uint16 _Value) const
{
	int iHash = _Value & m_HashAnd;

	const CMap16::CHashIDInfo* pIDInfo = m_pIDInfo;

	int ID = m_pHash[iHash];
	while(ID != -1)
	{
		const CHashIDInfo& IDInfo = pIDInfo[ID];
		if (IDInfo.m_Value == _Value)
			return ID;

		ID = IDInfo.m_iNext;
	}

	return -1;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMap32
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CMap32::Create(int16 _MaxID, int _HashBits)
{
	m_MaxIDs = 0;
	m_pHash = NULL;
	m_pIDInfo = NULL;
	m_iFirstLarge = -1;
	m_bUseLarge = false;

	THash<int16, CMap32Entry>::Create(_MaxID, 1 << _HashBits, false);

	m_HashBits = _HashBits;
	m_HashAnd = (1 << _HashBits) - 1;
}

void CMap32::Insert(int16 _ID, uint32 _Value)
{
	THash<int16, CMap32Entry>::Remove(_ID);
	THash<int16, CMap32Entry>::Insert(_ID, _Value & m_HashAnd);
	m_pIDInfo[_ID].m_Value = _Value;
}

bool CMap32::GetValue(int16 _ID, uint32& _RetValue) const
{
	M_ASSERT(_ID >= 0 && _ID < m_MaxIDs, "!");

	const CHashIDInfo* pID = &m_pIDInfo[_ID];
	if (pID->m_Flags & THASH_HASH)
	{
		_RetValue = pID->m_Value;
		return true;
	}
	else
		return false;
}

int16 CMap32::GetIndex(uint32 _Value) const
{
	int iHash = _Value & m_HashAnd;

	const CMap32::CHashIDInfo* pIDInfo = m_pIDInfo;

	int ID = m_pHash[iHash];
	while(ID != -1)
	{
		const CHashIDInfo& IDInfo = pIDInfo[ID];
		if (IDInfo.m_Value == _Value)
			return ID;

		ID = IDInfo.m_iNext;
	}

	return -1;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CIndexPool16
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CIndexPool16::Create(int _MaxObjects)
{
	MAUTOSTRIP(CIndexPool16_Create, MAUTOSTRIP_VOID);
	m_MaxIDs = 0;
	m_pHash = NULL;
	m_pIDInfo = NULL;
	m_iFirstLarge = -1;
	m_bUseLarge = false;

	THash<int16, CIndexPool16Elem>::Create(_MaxObjects, 1, true);
}

void CIndexPool16::Insert(int _iElem)
{
	MAUTOSTRIP(CIndexPool16_Insert, MAUTOSTRIP_VOID);
	THash<int16, CIndexPool16Elem>::Remove(_iElem);
	THash<int16, CIndexPool16Elem>::InsertLarge(_iElem);
	CHashIDInfo* pID = &m_pIDInfo[_iElem];
	pID->m_iElem = _iElem;
}

void CIndexPool16::Remove(int _iElem)
{
	MAUTOSTRIP(CIndexPool16_Remove, MAUTOSTRIP_VOID);
	THash<int16, CIndexPool16Elem>::Remove(_iElem);
}

void CIndexPool16::RemoveAll()
{
	MAUTOSTRIP(CIndexPool16_RemoveAll, MAUTOSTRIP_VOID);

	int ID = m_iFirstLarge;
	while(ID != -1)
	{
		int iNext = m_pIDInfo[ID].m_iNext;
		m_pIDInfo[ID].ClearLinkage();
		ID = iNext;
	}

	m_iFirstLarge = -1;
}

int CIndexPool16::EnumAll(uint16* _piElem, int _MaxElem)
{
	MAUTOSTRIP(CIndexPool16_EnumAll, 0);
	int nElem = 0;
	int ID = m_iFirstLarge;

	while(ID != -1)
	{
		if (nElem >= _MaxElem)
			Error("EnumAll", CStrF("Out of enumeration-space. (1) (%d/%d)", nElem, _MaxElem));

		int CurID(ID);
		ID = m_pIDInfo[ID].m_iNext;

		_piElem[nElem] = m_pIDInfo[CurID].m_iElem;
		nElem++;
	}

	return nElem;
}

