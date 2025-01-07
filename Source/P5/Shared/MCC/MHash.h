#ifndef _INC_MHASH
#define _INC_MHASH

#include "MCC.h"

// -------------------------------------------------------------------
//  THash
// -------------------------------------------------------------------
#define THASH_HASH	1
#define THASH_LARGE 2

template<class TLink, class TInfo>
class MCCDLLEXPORT THash : public CReferenceCount
{
protected:
	class CHashIDInfo : public TInfo
	{
	public:
		TLink m_Flags;
		TLink m_iHashBucket;
		TLink m_iPrev;
		TLink m_iNext;

		CHashIDInfo()
		{
			m_Flags = 0;
			m_iPrev = -1;
			m_iNext = -1;
			m_iHashBucket = -1;
		}
	};

	int m_MaxIDs;
	TList_Vector<TLink> m_lHash;
	TLink* m_pHash;
	TLink m_iFirstLarge;
	bool m_bUseLarge;

	TList_Vector<CHashIDInfo> m_lIDInfo;
	CHashIDInfo* m_pIDInfo;

public:
	THash()
	{
		m_MaxIDs = 0;
		m_pHash = NULL;
		m_pIDInfo = NULL;
		m_iFirstLarge = -1;
		m_bUseLarge = false;
	}

	~THash()
	{
/*		if (m_pHash)
		{ 
			delete[] m_pHash;
			m_pHash = NULL;
		}*/
	}

	int GetLinkCount(int _ID)
	{
		int nLinks = 0;
		while(_ID != -1)
		{
			nLinks++;
			_ID = m_pIDInfo[_ID].m_iNext;
		}
		return nLinks;
	}

	int GetMaxIDs() const
	{
		return m_MaxIDs;
	}

	void Create(int _MaxIDs, int _nBoxes, bool _bUseLarge)
	{
		m_lHash.SetLen(_nBoxes);
		m_pHash = m_lHash.GetBasePtr();
		FillChar(m_pHash, m_lHash.ListSize(), -1);
//		FillW(m_pHash, _nBoxes, -1);

		m_lIDInfo.Clear();
		m_lIDInfo.SetLen(_MaxIDs);
		m_pIDInfo = m_lIDInfo.GetBasePtr();
		m_MaxIDs = _MaxIDs;
		m_bUseLarge = _bUseLarge;
	}

protected:
	void Insert(int _ID, int _iBucket)
	{
		CHashIDInfo* pID = &m_lIDInfo[_ID];
		pID->m_iHashBucket = _iBucket;
		pID->m_Flags = THASH_HASH;
		pID->m_iPrev = -1;
		pID->m_iNext = m_pHash[_iBucket];
		if (pID->m_iNext != -1)
			m_pIDInfo[pID->m_iNext].m_iPrev = _ID;
		m_pHash[_iBucket] = _ID;
	}

	void InsertLarge(int _ID)
	{
		CHashIDInfo* pID = &m_lIDInfo[_ID];
		pID->m_Flags = THASH_LARGE;
		pID->m_iHashBucket = -1;
		pID->m_iPrev = -1;
		pID->m_iNext = m_iFirstLarge;
		if (pID->m_iNext != -1)
			m_pIDInfo[pID->m_iNext].m_iPrev = _ID;
		m_iFirstLarge = _ID;
	}

public:

	void Remove(int _ID)
	{
		if (!m_pHash) Error("Insert", "Not initialized.");

		if ((_ID < 0) || (_ID >= m_MaxIDs)) Error("Remove", CStrF("ID out of range. (%d/%d)", _ID, m_MaxIDs));

		CHashIDInfo* pID = &m_pIDInfo[_ID];
		if (pID->m_Flags & THASH_HASH)
		{
			if (pID->m_iPrev != -1)
				m_pIDInfo[pID->m_iPrev].m_iNext = pID->m_iNext;
			else
				m_pHash[pID->m_iHashBucket] = pID->m_iNext;

			if (pID->m_iNext != -1)
				m_pIDInfo[pID->m_iNext].m_iPrev = pID->m_iPrev;
		}
		else if (pID->m_Flags & THASH_LARGE)
		{
			if (pID->m_iPrev != -1)
				m_pIDInfo[pID->m_iPrev].m_iNext = pID->m_iNext;
			else
				m_iFirstLarge = pID->m_iNext;

			if (pID->m_iNext != -1)
				m_pIDInfo[pID->m_iNext].m_iPrev = pID->m_iPrev;

/*			if (_ID == m_iFirstLarge)
				m_iFirstLarge = pID->m_iNext;
			else
				m_pIDInfo[pID->m_iNext].m_iPrev = pID->m_iPrev;*/
		}

		pID->m_Flags = 0;
	}

	CStr GetString() const
	{
		int nHashed = 0;
		int nLarge = 0;
		for(int i = 0; i < m_lIDInfo.Len(); i++)
			if (m_lIDInfo[i].m_Flags & THASH_HASH)
				nHashed++;
			else if (m_lIDInfo[i].m_Flags & THASH_LARGE)
				nLarge++;

		return CStrF("nHashed %d, nLarge %d, MaxIDs %d", nHashed, nLarge, m_lIDInfo.Len());
	}
};

// -------------------------------------------------------------------
//  THash2D
// -------------------------------------------------------------------
class CHash2DNull
{
public:
	int m_Hirr;
	CHash2DNull() {};
	~CHash2DNull() {};
};

class MCCDLLEXPORT CHash2D : public THash<int32, CHash2DNull>
{
protected:
	int m_BoxSize;
	int m_BoxShiftSize;
	int m_BoxAndSize;
	int m_nBoxAndX;
	int m_nBoxAndY;
	int m_nBoxesX;
	int m_nBoxesY;
	int m_nBoxes;
	int m_HashAndSizeX;
	int m_HashAndSizeY;

public:
	CHash2D();
	~CHash2D();
	void Create(int _nBoxes, int _BoxShiftSize, int _MaxIDs, bool _bUseLarge);
	
	void Insert(int _ID, const CVec3Dfp32& _Min, const CVec3Dfp32& _Max);
	int EnumerateBox(const CVec3Dfp32& _Min, const CVec3Dfp32& _Max, int32* _pEnumRetIDs, int _MaxEnumIDs);
};

//----------------------------------------------------------------
// CStringHash
//----------------------------------------------------------------
class CStringHashElement
{
public:
	CStr m_Str;
};

class MCCDLLEXPORT CStringHash : public THash<int16, CStringHashElement>
{
protected:
	bool m_bCaseSensitive;
	int GetHashIndex(const char* _pStr);

public:
	void Create(int _nIndices, bool _bCaseSensitive = true);
	void Insert(int _Index, CStr _Str);
	int GetIndex(const char* _pStr);
};

typedef TPtr<CStringHash> spCStringHash;


#endif // _INC_MHASH
