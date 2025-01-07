#include "PCH.h"

#include "MHash.h"

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

void CHash2D::Insert(int _ID, const CVec3Dfp4& _Min, const CVec3Dfp4& _Max)
{
	if (!m_pHash) Error("Insert", "Not initialized.");
	// Object med radie större än lådstorleken hamnar i large-listan.
	if ((_ID < 0) || (_ID > m_MaxIDs)) Error("Insert", CStrF("ID out of range. (%d/%d)", _ID, m_MaxIDs));
	
	CHashIDInfo* pID = &m_pIDInfo[_ID];
	if (pID->m_Flags & (THASH_HASH | THASH_LARGE)) Remove(_ID);
	
	int MaxSize = 0;
	if (m_bUseLarge)
	{
		if (_Max.k[0] - _Min.k[0] + 1.0f > MaxSize) MaxSize = _Max.k[0] - _Min.k[0] + 1.0f;
		if (_Max.k[1] - _Min.k[1] + 1.0f > MaxSize) MaxSize = _Max.k[1] - _Min.k[1] + 1.0f;
	}
	
	int x = (_Max.k[0] + _Min.k[0]) * 0.5f;
	int y = (_Max.k[1] + _Min.k[1]) * 0.5f;
	
	int iBucket =
		((x & m_HashAndSizeX) >> m_BoxShiftSize) +
		((y & m_HashAndSizeY) >> m_BoxShiftSize) * m_nBoxesX;
	
	//	pID->m_pObj = _pObj;
	
	if (!m_bUseLarge || MaxSize < m_BoxSize)
		THash<int32, CHash2DNull>::Insert(_ID, iBucket);
	else
		THash<int32, CHash2DNull>::InsertLarge(_ID);
}

int CHash2D::EnumerateBox(const CVec3Dfp4& _Min, const CVec3Dfp4& _Max, int32* _pEnumRetIDs, int _MaxEnumIDs)
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
			const CBox3Dfp4* pBox = pObj->GetAbsBoundBox();
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
				const CBox3Dfp4* pBox = pObj->GetAbsBoundBox();
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

//----------------------------------------------------------------
// CStringHash
//----------------------------------------------------------------
int CStringHash::GetHashIndex(const char* _pStr)
{
	char Str[2048];
	const char* pS = NULL;
	if (m_bCaseSensitive)
		pS = _pStr;
	else
	{
		if (_pStr) 
			strcpy(Str, _pStr);
		else
			Str[0] = 0;
		strupr(Str);
		pS = Str;
	}

	int Hash = 0;
	int l = CStr::StrLen(pS);
	for(int i = 0; i < l; i++)
		Hash += pS[i];

	return Hash & 255;
}


void CStringHash::Create(int _nIndices, bool _bCaseSensitive)
{
	m_bCaseSensitive = _bCaseSensitive;
	THash<int16, CStringHashElement>::Create(_nIndices, 256, false);
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

