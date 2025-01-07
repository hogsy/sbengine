
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/

#ifndef _INC_MDA_HASH
#define _INC_MDA_HASH

//#include "MDA.h"
#include "MMemMgrPool.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| THash
|__________________________________________________________________________________________________
\*************************************************************************************************/

#define THASH_HASH	1
#define THASH_LARGE 2

template<class TLink, class TInfo>
class MCCDLLEXPORT THash : public CReferenceCount
{
public:
	class CHashIDInfo : public TInfo
	{
	public:
		TLink m_Flags;
		TLink m_iHashBucket;
		TLink m_iPrev;
		TLink m_iNext;

		void ClearLinkage()
		{
			m_Flags = 0;
			m_iPrev = -1;
			m_iNext = -1;
			m_iHashBucket = -1;
		}

		CHashIDInfo()
		{
			ClearLinkage();
		}
	};

protected:
	int m_MaxIDs;
	TArray<TLink> m_lHash;
	TLink* m_pHash;
	TLink m_iFirstLarge;
	bool m_bUseLarge;

	TArray<CHashIDInfo> m_lIDInfo;
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

	void Clear()
	{
		m_lHash.Clear();
		m_lIDInfo.Clear();
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

//		m_lIDInfo.Clear();
		m_lIDInfo.SetLen(_MaxIDs);
		CHashIDInfo* pIDI = m_lIDInfo.GetBasePtr();
		for(int i = 0; i < _MaxIDs; i++)
			pIDI[i].ClearLinkage();

		m_pIDInfo = m_lIDInfo.GetBasePtr();
		m_MaxIDs = _MaxIDs;
		m_bUseLarge = _bUseLarge;
	}

	void Resize(int _MaxIDs)
	{
		m_lIDInfo.SetLen(_MaxIDs);
		m_pIDInfo = m_lIDInfo.GetBasePtr();
		for (int i = m_MaxIDs; i < _MaxIDs; i++)
			m_pIDInfo[i].ClearLinkage();
		m_MaxIDs = _MaxIDs;
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
#ifndef M_RTM
		if (!m_pHash) Error("Insert", "Not initialized.");

		if ((_ID < 0) || (_ID >= m_MaxIDs)) Error("Remove", CStrF("ID out of range. (%d/%d)", _ID, m_MaxIDs));
#endif

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



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CStringHash
|__________________________________________________________________________________________________
\*************************************************************************************************/
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

	int m_HashAnd;

public:
	void Create(int _nIndices, bool _bCaseSensitive = true, int _HashShiftSize = 8);
	void Insert(int _Index, CStr _Str);
	int GetIndex(const char* _pStr);
	int GetIndex(uint32 _StrHash);
};

typedef TPtr<CStringHash> spCStringHash;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CStringHashConst
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CStringHashConstElement
{
public:
	union
	{
		const char* m_pStr;
		uint32 m_StrHash;
	};
#ifndef M_RTM
	const char* m_pStrDebug;
#endif
};

class MCCDLLEXPORT CStringHashConst : public THash<int16, CStringHashConstElement>
{
protected:
	bool m_bCaseSensitive;
	int GetHashIndex(const char* _pStr);

	int m_HashAnd;

public:
	void Create(int _nIndices, bool _bCaseSensitive = true, int _HashShiftSize = 8);
	void Insert(int _Index, const char* _pStr);
	int GetIndex(const char* _pStr);
	int GetIndex(uint32 _StrHash);
};

typedef TPtr<CStringHashConst> spCStringHashConst;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMap16
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CMap16Entry
{
public:
	uint16 m_Value;
};

class MCCDLLEXPORT CMap16 : public THash<int16, CMap16Entry>
{
	uint16 m_HashBits;
	uint16 m_HashAnd;

public:
	void Create(int16 _MaxID, int _HashBits);

	void Insert(int16 _ID, uint16 _Value);
	bool GetValue(int16 _ID, uint16& _RetValue) const;
	int16 GetIndex(uint16 _Value) const;					// -1 == Not found
};

typedef TPtr<CMap16> spCMap16;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMap32
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CMap32Entry
{
public:
	uint32 m_Value;
};

class MCCDLLEXPORT CMap32 : public THash<int16, CMap32Entry>
{
	uint16 m_HashBits;
	uint16 m_HashAnd;

public:
	void Create(int16 _MaxID, int _HashBits);

	void Insert(int16 _ID, uint32 _Value);
	bool GetValue(int16 _ID, uint32& _RetValue) const;
	int16 GetIndex(uint32 _Value) const;					// -1 == Not found
};

typedef TPtr<CMap32> spCMap32;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CIndexPool16
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CIndexPool16Elem
{
public:
	int16 m_iElem;
};

class CIndexPool16 : public THash<int16, CIndexPool16Elem>
{
public:
	void Create(int _MaxElements);
	void Insert(int _iElem);
	void Remove(int _iElem);
	void RemoveAll();
	int EnumAll(uint16* _piElem, int _MaxElem);

	const CHashIDInfo* GetLinks() const { return m_pIDInfo; };
	int GetFirst() const { return m_iFirstLarge; };
};



#endif // _INC_MDA_HASH
