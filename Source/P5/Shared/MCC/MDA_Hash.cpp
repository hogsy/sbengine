
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

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDA_Hash
|__________________________________________________________________________________________________
\*************************************************************************************************/



#ifdef PLATFORM_CONSOLE
MDA_StaticPool(g_HashablePool,sizeof(CDA_HashableMember), 4096)
#else
MDA_StaticPool(g_HashablePool,sizeof(CDA_HashableMember), 65536)
#endif


CDA_Hash::CDA_Hash()
{
	m_NumHashBits = 0;
	m_ppHash = NULL;
	m_IdOffset = 0;
	m_HashBits = 0;
	m_NumElements = 0;
	m_NumHashed = 0;
	m_AutomaticGrowth = 0;
}

CDA_Hash::CDA_Hash(uint8 _NumHashBits, mint _IdOffset, uint8 _AutomaticGrowth)
{
	m_NumHashBits = 0;
	m_ppHash = NULL;
	m_IdOffset = 0;
	m_HashBits = 0;
	m_NumElements = 0;
	m_NumHashed = 0;
	m_AutomaticGrowth = 0;

	InitHash(_NumHashBits, _IdOffset, _AutomaticGrowth);

}

CDA_Hash::~CDA_Hash()
{
	
	if (m_ppHash)
	{
		for (mint i = 0; i < m_NumElements; ++i)
		{
			while (m_ppHash[i])
				deleteP(m_ppHash[i], g_HashablePool);
		}

		delete [] m_ppHash;
	}

}

void CDA_Hash::InitHash(uint8 _NumHashBits, mint _IdOffset,uint8 _AutomaticGrowth)
{
	m_NumHashBits = _NumHashBits;
	
	CDA_HashableMember **Old_Hash = m_ppHash;
	mint Old_Elements = m_NumElements;
	
	m_NumElements = (mint)1 << _NumHashBits;

	m_AutomaticGrowth = _AutomaticGrowth;
	
	m_HashBits = ((mint)1 << _NumHashBits) - 1;
	
	m_IdOffset = _IdOffset;
	
	m_ppHash = MDA_NEW_DEBUG_NOLEAK CDA_HashableMember *[m_NumElements];
	
	for (mint i = 0; i < m_NumElements; ++i)
	{
		m_ppHash[i] = NULL;
	}
	
	if (Old_Hash)
	{
		for (mint i = 0; i < Old_Elements; ++i)
		{
			while (Old_Hash[i])
			{
				CDA_Hashable *m_pHashable = Old_Hash[i]->m_pHashable;

				deleteP(Old_Hash[i], g_HashablePool);

//				deleteP(Old_Hash[i], g_HashablePool);
				m_pHashable->Hash_Insert(this);
			}
		}
		
		delete [] Old_Hash;
	}
}


CDA_Hashable *CDA_Hash::Get(void *_pHashData)
{
	
	if (!m_ppHash)
		return NULL;

	CDA_HashableMember *pHashableMember = m_ppHash[GetHash(_pHashData) & m_HashBits];

	while (pHashableMember)
	{
		if (IsSame(((uint8 *)pHashableMember->m_pHashable + m_IdOffset), _pHashData))
			return pHashableMember->m_pHashable;
			
		pHashableMember = pHashableMember->m_pHash_Next;
	}

	return NULL;

}

CDA_HashableMember *CDA_Hash::GetMember(void *_pHashData)
{
	if (!m_ppHash)
		return NULL;

	CDA_HashableMember *pHashableMember = m_ppHash[GetHash(_pHashData) & m_HashBits];
	
	while (pHashableMember)
	{
		if (IsSame(((uint8 *)pHashableMember->m_pHashable + m_IdOffset), _pHashData))
			return pHashableMember;
			
		pHashableMember = pHashableMember->m_pHash_Next;
	}

	return NULL;
}

void CDA_Hash::MoveToLoc(CDA_Hash *NewLoc)
{

}


#ifdef _DEBUG
void CDA_Hash::ShowSpread()
{
	if (m_ppHash)
	{
		for (mint i = 0; i < m_NumElements; ++i)
		{
			CDA_HashableMember *Current = m_ppHash[i];
			int Num = 0;
			while (Current)
			{
				++Num;
				Current = Current->m_pHash_Next;
			}
			M_TRACEALWAYS("m_ppHash %d, %d", i, Num);
		}

	}
}
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDA_IDHash
|__________________________________________________________________________________________________
\*************************************************************************************************/


CDA_IDHash::CDA_IDHash()
{
	m_IdSize = 0;
}

CDA_IDHash::CDA_IDHash(uint8 _NumHashBits, mint _IdOffset, mint _IdSize, uint8 _AutomaticGrowth)
{
	m_IdSize = 0;

	InitHash(_NumHashBits, _IdOffset, _IdSize, _AutomaticGrowth);
}

void CDA_IDHash::InitHash(uint8 _NumHashBits, mint _IdOffset, mint _IdSize, uint8 _AutomaticGrowth)
{
	m_IdSize = _IdSize;
	CDA_Hash::InitHash(_NumHashBits, _IdOffset, _AutomaticGrowth);
}


CDA_Hashable *CDA_IDHash::Get(void *_pHashData)
{
	
	CDA_HashableMember *pHashableMember = m_ppHash[GetHash(_pHashData) & m_HashBits];
	
	switch (m_IdSize)
	{
	case 4:
		while (pHashableMember)
		{
			if (*((uint32 *)((uint8 *)pHashableMember->m_pHashable + m_IdOffset)) == (*((uint32 *)_pHashData)))
				return pHashableMember->m_pHashable;
			
			pHashableMember = pHashableMember->m_pHash_Next;
		}
		break;
	case 2:
		while (pHashableMember)
		{
			if (*((uint16 *)((uint8 *)pHashableMember->m_pHashable + m_IdOffset)) == (*((uint16 *)_pHashData)))
				return pHashableMember->m_pHashable;
			
			pHashableMember = pHashableMember->m_pHash_Next;
		}
		break;
	case 1:
		while (pHashableMember)
		{
			if (*((uint8 *)((uint8 *)pHashableMember->m_pHashable + m_IdOffset)) == (*((uint8 *)_pHashData)))
				return pHashableMember->m_pHashable;
			
			pHashableMember = pHashableMember->m_pHash_Next;
		}
		break;
	default:
		while (pHashableMember)
		{
			if (memcmp((uint8 *)pHashableMember->m_pHashable + m_IdOffset,_pHashData,m_IdSize) == 0)
				return pHashableMember->m_pHashable;
			
			pHashableMember = pHashableMember->m_pHash_Next;
		}
		break;
	}

	return NULL;

}

CDA_HashableMember *CDA_IDHash::GetMember(void *_pHashData)
{
	CDA_HashableMember *pHashableMember = m_ppHash[GetHash(_pHashData) & m_HashBits];
	
	switch (m_IdSize)
	{
	case 4:
		while (pHashableMember)
		{
			if (*((uint32 *)((uint8 *)pHashableMember->m_pHashable + m_IdOffset)) == (*((uint32 *)_pHashData)))
				return pHashableMember;
			
			pHashableMember = pHashableMember->m_pHash_Next;
		}
		break;
	case 2:
		while (pHashableMember)
		{
			if (*((uint16 *)((uint8 *)pHashableMember->m_pHashable + m_IdOffset)) == (*((uint16 *)_pHashData)))
				return pHashableMember;
			
			pHashableMember = pHashableMember->m_pHash_Next;
		}
		break;
	case 1:
		while (pHashableMember)
		{
			if (*((uint8 *)((uint8 *)pHashableMember->m_pHashable + m_IdOffset)) == (*((uint8 *)_pHashData)))
				return pHashableMember;
			
			pHashableMember = pHashableMember->m_pHash_Next;
		}
		break;
	default:
		while (pHashableMember)
		{
			if (memcmp((uint8 *)pHashableMember->m_pHashable + m_IdOffset,_pHashData,m_IdSize) == 0)
				return pHashableMember;
			
			pHashableMember = pHashableMember->m_pHash_Next;
		}
		break;
	}

	return NULL;
}

mint CDA_IDHash::GetHash(void *_pHashData)
{
	switch (m_IdSize)
	{
	case 4:
		if (m_NumHashBits > 16)
		{
			return *((uint32 *)_pHashData);
		}
		else if (m_NumHashBits > 8)
		{
			uint16 Ret = (uint16)((*((uint32 *)_pHashData)) >> 16);
			Ret += (uint16)((*((uint32 *)_pHashData)) & 0xFFFF);
			return Ret;
		}
		else
		{
			uint8 Ret = (*((uint8 *)_pHashData));
			Ret += (*((uint8 *)_pHashData + 1));
			Ret += (*((uint8 *)_pHashData + 2));
			Ret += (*((uint8 *)_pHashData + 3));
			return Ret;
		}
		break;
	case 2:
		if (m_NumHashBits > 8)
		{
			return *((uint16 *)_pHashData);
		}
		else
		{
			uint8 Ret = (uint8)((*((uint32 *)_pHashData)) >> 8);
			Ret += (uint8)((*((uint32 *)_pHashData)) & 0xFF);
			return Ret;
		}
		break;
	case 1:
			return *((uint8 *)_pHashData);
		break;
	default:
		if (m_NumHashBits > 16)
		{
			mint Ret = 0;
			int Total = m_IdSize;
			int Num = (Total >> 2);
			Total -= Num << 2;

			uint8 *IDStep = (uint8 *)_pHashData;

			while (Num)
			{
				Ret += *((uint32 *)IDStep);
				--Num;
				IDStep += 4;
			}
			
			Num = (Total >> 1);
			Total -= Num << 1;

			while (Num)
			{
				Ret += *((uint16 *)IDStep);
				--Num;
				IDStep += 2;
			}			

			Num = Total;

			while (Num)
			{
				Ret += *((uint8 *)IDStep);
				--Num;
				++IDStep;
			}			

			return Ret;
		}
		else if (m_NumHashBits > 8)
		{
			uint16 Ret = 0;
			int Total = m_IdSize;
			int Num = (Total >> 1);
			Total -= Num << 1;

			uint8 *IDStep = (uint8 *)_pHashData;

			while (Num)
			{
				Ret += *((uint16 *)IDStep);
				--Num;
				IDStep += 2;
			}			

			Num = Total;

			while (Num)
			{
				Ret += *((uint8 *)IDStep);
				--Num;
				++IDStep;
			}			

			return Ret;
		}
		else
		{
			uint8 Ret = 0;
			int Num = m_IdSize;

			uint8 *IDStep = (uint8 *)_pHashData;

			while (Num)
			{
				Ret += *((uint8 *)IDStep);
				--Num;
				++IDStep;
			}			

			return Ret;
		}
		break;
	}

}

void CDA_IDHash::MoveToLoc(CDA_Hash *NewLoc)
{

}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDA_Hashable
|__________________________________________________________________________________________________
\*************************************************************************************************/
		

CDA_Hashable::CDA_Hashable()
{
	m_pDA_Hash_First = NULL;
}

CDA_Hashable::~CDA_Hashable()
{
	if (m_pDA_Hash_First)
		deleteP(m_pDA_Hash_First, g_HashablePool);
}


void CDA_Hashable::MoveToLoc(CDA_Hashable *_pNewLoc)
{

/*	*NewLoc = *this;

	if (*(NewLoc->IDHash_First) == this)
	{
		*(NewLoc->IDHash_First) = NewLoc;
	}

	if (NewLoc->IDHash_Prev)
	{
		NewLoc->IDHash_Prev->IDHash_Next = NewLoc;
	}

	if (NewLoc->IDHash_Next)
	{
		NewLoc->IDHash_Next->IDHash_Prev = NewLoc;
	}
	*/

}

void CDA_Hashable::Hash_Insert(CDA_Hash &_Hash)
{

	Hash_Insert(&_Hash);

}

void CDA_Hashable::Hash_Insert(CDA_Hash *_pHash)
{

	M_ASSERT(!_pHash->Get((void *)((mint)this + _pHash->m_IdOffset)), "Object ID already in m_ppHash");

	CDA_HashableMember *pHashableMember = newP (CDA_HashableMember, g_HashablePool);

	pHashableMember->m_pHashable_Prev = NULL;

	if (m_pDA_Hash_First)
		m_pDA_Hash_First->m_pHashable_Prev = pHashableMember;

	pHashableMember->m_pHashable_Next = m_pDA_Hash_First;
	m_pDA_Hash_First = pHashableMember;
		

	pHashableMember->m_pHashable = this;

	pHashableMember->m_pHash = _pHash;

	pHashableMember->m_ppHash_First = &_pHash->m_ppHash[ _pHash->GetHash((void *)((mint)this + _pHash->m_IdOffset)) & _pHash->m_HashBits];
	
	pHashableMember->m_pHash_Prev = NULL;
	
	if (*pHashableMember->m_ppHash_First)
		(*pHashableMember->m_ppHash_First)->m_pHash_Prev = pHashableMember;
	
	pHashableMember->m_pHash_Next = (*pHashableMember->m_ppHash_First);
	
	(*pHashableMember->m_ppHash_First) = pHashableMember;
	
	++_pHash->m_NumHashed;
	
	if (_pHash->m_AutomaticGrowth)
	{
		if (_pHash->m_AutomaticGrowth == 2)
		{
			if (_pHash->m_NumHashed > _pHash->m_NumElements)
			{
				_pHash->InitHash(_pHash->m_NumHashBits + 1, _pHash->m_IdOffset, _pHash->m_AutomaticGrowth);
			}
			else if (_pHash->m_NumHashed < (_pHash->m_NumElements >> 1))
			{
				_pHash->InitHash(_pHash->m_NumHashBits - 1, _pHash->m_IdOffset, _pHash->m_AutomaticGrowth);
			}
		}
		else
		{
			if (_pHash->m_NumHashed > (_pHash->m_NumElements << 1))
			{
				_pHash->InitHash(_pHash->m_NumHashBits + 1, _pHash->m_IdOffset, _pHash->m_AutomaticGrowth);
			}
			else if (_pHash->m_NumHashed < _pHash->m_NumElements)
			{
				_pHash->InitHash(_pHash->m_NumHashBits - 1, _pHash->m_IdOffset, _pHash->m_AutomaticGrowth);
			}
		}
	}

}

void CDA_Hashable::Hash_Remove(CDA_Hash &_Hash)
{

	Hash_Remove(&_Hash);

}

void CDA_Hashable::Hash_Remove(CDA_Hash *_pHash)
{
	CDA_HashableMember *pHashableMember = _pHash->GetMember((void *)((mint)this + _pHash->m_IdOffset));

#ifdef _DEBUG

	CDA_HashableMember *pHashableMember2 = m_pDA_Hash_First;

	while (pHashableMember2)
	{
		if (pHashableMember2->m_pHash == _pHash)
			break;
	}

	M_ASSERT(pHashableMember2 == pHashableMember, "Hash does not correspond to member to delete.");

#endif

	if (pHashableMember)
		deleteP(pHashableMember, g_HashablePool);
}
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDA_HashableMember
|__________________________________________________________________________________________________
\*************************************************************************************************/

	

CDA_HashableMember::CDA_HashableMember()
{
}

CDA_HashableMember::~CDA_HashableMember()
{

	if (m_pHashable_Next)
		m_pHashable_Next->m_pHashable_Prev = m_pHashable_Prev;

	if (m_pHashable_Prev)
		m_pHashable_Prev->m_pHashable_Next = m_pHashable_Next;
	else
		m_pHashable->m_pDA_Hash_First = m_pHashable_Next;

	if (m_pHash_Next)
		m_pHash_Next->m_pHash_Prev = m_pHash_Prev;
	
	if (m_pHash_Prev)
		m_pHash_Prev->m_pHash_Next = m_pHash_Next;
	else
		(*m_ppHash_First) = m_pHash_Next;
	
	--m_pHash->m_NumHashed;
	
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Small hash
|__________________________________________________________________________________________________
\*************************************************************************************************/

CDA_HashSmall::CDA_HashSmall()
{
	m_NumHashBits = 0;
	m_ppHash = NULL;
	m_IdOffset = 0;
	m_HashBits = 0;
	m_NumElements = 0;
	m_NumHashed = 0;
	m_AutomaticGrowth = 0;
}

CDA_HashSmall::CDA_HashSmall(uint8 _NumHashBits, mint _IdOffset, uint8 _AutomaticGrowth)
{
	m_NumHashBits = 0;
	m_ppHash = NULL;
	m_IdOffset = 0;
	m_HashBits = 0;
	m_NumElements = 0;
	m_NumHashed = 0;
	m_AutomaticGrowth = 0;

	InitHash(_NumHashBits, _IdOffset, _AutomaticGrowth);
}

CDA_HashSmall::~CDA_HashSmall()
{
	
	if (m_ppHash)
	{
		for (mint i = 0; i < m_NumElements; ++i)
		{
			while (m_ppHash[i]->m_pHashable)
				m_ppHash[i]->Hash_Insert(NULL, NULL);
		}

		delete [] m_ppHash;
	}

}

void CDA_HashSmall::InitHash(uint8 _NumHashBits, mint _IdOffset,uint8 _AutomaticGrowth)
{
	m_NumHashBits = _NumHashBits;
	
	CDA_HashLink **Old_Hash = m_ppHash;
	mint Old_Elements = m_NumElements;

	m_HashLink.m_ppHash_First = (CDA_HashLink **)this;
	m_HashLink.m_pHash_Next = NULL;
	m_HashLink.m_pHash_Prev = NULL;
	m_HashLink.m_pHashable = NULL;
	
	m_NumElements = (mint)1 << _NumHashBits;

	m_AutomaticGrowth = _AutomaticGrowth;
	
	m_HashBits = ((mint)1 << _NumHashBits) - 1;
	
	m_IdOffset = _IdOffset;
	
	m_ppHash = MDA_NEW_DEBUG_NOLEAK CDA_HashLink *[m_NumElements];
	
	for (mint i = 0; i < m_NumElements; ++i)
	{
		m_ppHash[i] = &m_HashLink;
	}
	
	if (Old_Hash)
	{
		for (mint i = 0; i < Old_Elements; ++i)
		{
			while (Old_Hash[i] && Old_Hash[i]->m_pHashable)
			{
				if (Old_Hash[i]->m_pHashable)
					Old_Hash[i]->Hash_Insert(this, Old_Hash[i]->m_pHashable);
			}
		}
		
		delete [] Old_Hash;
	}
}

void *CDA_HashSmall::Get(void *_pHashData)
{
	
	if (!m_ppHash)
		return NULL;

	CDA_HashLink *pHashableMember = m_ppHash[GetHash(_pHashData) & m_HashBits];

	while (pHashableMember && pHashableMember->m_pHashable)
	{
		if (IsSame(((uint8 *)pHashableMember->m_pHashable + m_IdOffset), _pHashData))
			return pHashableMember->m_pHashable;
			
		pHashableMember = pHashableMember->m_pHash_Next;
	}

	return NULL;

}

CDA_HashLink::CDA_HashLink()
{
	m_pHashable = NULL;
}

CDA_HashLink::~CDA_HashLink()
{
	if (m_pHashable)
		Hash_Insert(NULL, NULL);
}

void CDA_HashLink::Hash_Insert(CDA_HashSmall *_pHash, void *_Hashable)
{	
	if (m_pHashable)
	{		
		CDA_HashLink *_pCurrent = m_pHash_Next;

		while (_pCurrent->m_pHashable)
			_pCurrent = _pCurrent->m_pHash_Next;

		if (m_pHash_Next)
			m_pHash_Next->m_pHash_Prev = m_pHash_Prev;
		
		if (m_pHash_Prev)
			m_pHash_Prev->m_pHash_Next = m_pHash_Next;
		else
			(*m_ppHash_First) = m_pHash_Next;
		
		--((CDA_HashSmall *)_pCurrent->m_ppHash_First)->m_NumHashed;
		
		m_pHashable = NULL;
	}
	
	if (_pHash && _Hashable)
	{
		if (!_pHash->m_ppHash)
			_pHash->InitHash(_pHash->m_NumHashBits+1, _pHash->m_IdOffset, _pHash->m_AutomaticGrowth);

		M_ASSERT(!_pHash->Get((void *)((mint)_Hashable + _pHash->m_IdOffset)), "Object ID already in m_ppHash");
		
		m_pHashable = _Hashable;
		
		m_ppHash_First = &_pHash->m_ppHash[ _pHash->GetHash((void *)((mint)_Hashable + _pHash->m_IdOffset)) & _pHash->m_HashBits];
		
		m_pHash_Prev = NULL;
		
		if (*m_ppHash_First)
			(*m_ppHash_First)->m_pHash_Prev = this;
		
		m_pHash_Next = (*m_ppHash_First);
		
		(*m_ppHash_First) = this;
		
		++_pHash->m_NumHashed;
		
		if (_pHash->m_AutomaticGrowth)
		{
			if (_pHash->m_AutomaticGrowth == 2)
			{
				if (_pHash->m_NumHashed > _pHash->m_NumElements)
				{
					_pHash->InitHash(_pHash->m_NumHashBits + 1, _pHash->m_IdOffset, _pHash->m_AutomaticGrowth);
				}
				else if (_pHash->m_NumHashed < (_pHash->m_NumElements >> 1))
				{
					_pHash->InitHash(_pHash->m_NumHashBits - 1, _pHash->m_IdOffset, _pHash->m_AutomaticGrowth);
				}
			}
			else
			{
				if (_pHash->m_NumHashed > (_pHash->m_NumElements << 1))
				{
					_pHash->InitHash(_pHash->m_NumHashBits + 1, _pHash->m_IdOffset, _pHash->m_AutomaticGrowth);
				}
				else if (_pHash->m_NumHashed < _pHash->m_NumElements)
				{
					_pHash->InitHash(_pHash->m_NumHashBits - 1, _pHash->m_IdOffset, _pHash->m_AutomaticGrowth);
				}
			}
		}		
	}	
}

void CDA_HashLink::RemoveFromHash()
{
	Hash_Insert(NULL, NULL);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CStringHash
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*  MMMM..k..  bizarro world..     =)  */

#ifdef NEVER
CStringHash::CStringHash()
{
	InitHash(1, (DWORD)(&((CStringHashLink *)(0x70000000))->m_Str) - 0x70000000, 2);
}

CStringHash::~CStringHash()
{
}

bool CStringHash::IsSame(void *_pFirst, void *_pSecond)
{
	CStrBase* pFirst = (CStrBase *)_pFirst;
	CStrBase* pSecond = (CStrBase *)_pSecond;

	if (m_bCaseSensitive)
		return pFirst->Compare(pSecond->Str()) == 0;
	else
		return pFirst->CompareNoCase(pSecond->Str()) == 0;
}

mint CStringHash::GetHash(void *_pHashData)
{
	CStrBase* pE = (CStrBase *)_pHashData;

	CCRC32Checker Checker;

	if (m_bCaseSensitive)
	{
		Checker.AddBlock(pE->Str(), pE->Len());
	}
	else
	{
		char Str[2048];
		strcpy(Str, pE->Str());
		CStrBase::strupr(Str);
		Checker.AddBlock(Str, strlen(Str));
	}
	
	return Checker.m_CurrentCRC;
}
/*
int CStringHash::GetHashIndex(const char* _pStr)
{
	const char* pS = NULL;
	if (m_bCaseSensitive)
		pS = _pStr;
	else
	{
		char Str[2048];
		if (_pStr)
			strcpy(Str, _pStr);
		else
			Str[0] = 0;
			
		CStrBase::strupr(Str);
		pS = Str;
	}

	int Hash = 0;
	int l = CStr::StrLen(pS);
	for(int i = 0; i < l; i++)
		Hash += pS[i];

	return Hash & 255;
}
*/

void CStringHash::Create(int _nIndices, bool _bCaseSensitive)
{
	m_bCaseSensitive = _bCaseSensitive;
}

void CStringHash::Insert(int _Index, CStr _Str, CStringHashLink& _Link)
{
	CStringHashLink * pPtr = (CStringHashLink *)Get(&_Str);
	if (pPtr)
	{
		if (pPtr != &_Link)
		{
			return;
			Error("CStringHash::Insert", CStrF("Another object with same string is already in hash. (%s)", _Str.Str()));
		}
		else
			return;
	}

	_Link.m_Str = _Str;
	_Link.m_Index = _Index;
	_Link.m_HashLink.Hash_Insert(this, &_Link);
}

class CStringContainer : public CStrBase
{
public:
	const char *m_ContainedStr;

	CStringContainer(const char * _Str)
	{
		m_ContainedStr = _Str;
	}

	virtual char operator[](int i) const { return NULL;};
	virtual operator char* () { return (char*)m_ContainedStr;};
	virtual operator const char* () const  { return m_ContainedStr;};
	virtual char* GetStr() { return (char*)m_ContainedStr;};
	virtual const char* GetStr() const { return m_ContainedStr;};
	virtual const char* Str() const { return m_ContainedStr;};

	virtual operator wchar*() { return (wchar*)m_ContainedStr;};
	virtual operator const wchar*() const { return (wchar*)m_ContainedStr;};

	virtual void* StrAny(int _Pos = 0) { return 0;};
	virtual const void* StrAny(int _Pos = 0) const  { return 0;};

	virtual void Clear() {};

	virtual int Len() const { return strlen(m_ContainedStr);};
	virtual void Capture(const char* _pStr) {};
	virtual void Capture(const char* _pStr, int _Len) {};
	virtual void CaptureFormated(const char* _pStr, ...) {};

	virtual void MakeUnique() {};
};

int CStringHash::GetIndex(const char *_Str)
{
	CStringContainer SearchStr(_Str);
	CStringHashLink * pPtr = (CStringHashLink *)Get(&SearchStr);
	if (pPtr)
		return pPtr->m_Index;
	else
		return -1;
}

#endif

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
	THash<int16, CStringHashConstElement>::Insert(_Index, GetHashIndex(_pStr));
	CHashIDInfo* pID = &m_pIDInfo[_Index];
	pID->m_pStr = _pStr;
}

int CStringHashConst::GetIndex(const char* _pStr)
{
	int ID = m_pHash[GetHashIndex(_pStr)];
	while(ID != -1)
	{
		if (m_bCaseSensitive)
		{	if (CStrBase::Compare(m_pIDInfo[ID].m_pStr, _pStr) == 0) return ID; }
		else
		{	if (CStrBase::CompareNoCase(m_pIDInfo[ID].m_pStr, _pStr) == 0) return ID; }

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

bool CMap32::GetValue(int16 _ID, uint32& _RetValue)
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

int16 CMap32::GetIndex(uint32 _Value)
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

