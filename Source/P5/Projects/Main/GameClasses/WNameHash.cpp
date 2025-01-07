#include "PCH.h"
#include "WNameHash.h"


//
// Autovar packing/unpacking
//
void CNameHash::Pack(uint8*& _pD) const
{
	TAutoVar_Pack(m_Hash, _pD);

#ifdef _DEBUG
	CFStr Name = m_Name;
	TAutoVar_Pack(Name, _pD);
#endif
}

void CNameHash::Unpack(const uint8*& _pD)
{
	TAutoVar_Unpack(m_Hash, _pD);

#ifdef _DEBUG
	CFStr Name;
	TAutoVar_Unpack(Name, _pD);
	M_ASSERT(StringToHash(Name.Str()) == m_Hash, "[CNameHash::Unpack] Hash I/O mismatch!");
	*this = Name;
#endif
}


void CNameHash::Write(CCFile* _pFile) const
{
	_pFile->WriteLE(m_Hash);

#ifdef _DEBUG
	m_Name.Write(_pFile);
#endif
}


void CNameHash::Read(CCFile* _pFile)
{
	_pFile->ReadLE(m_Hash);

#ifdef _DEBUG
	m_Name.Read(_pFile);
	M_ASSERT(StringToHash(m_Name.Str()) == m_Hash, "[CNameHash::Read] Hash I/O mismatch!");
	DetectCollision(m_Name);
#endif
}



//
// Debug stuff
//
#ifdef _DEBUG

struct Entry { uint32 m_HashValue; CStr m_Str; };
class CollisionDetector : public THash<int16, Entry>
{
	typedef THash<int16, Entry> parent;
	int m_nIndex;
public:
	CollisionDetector()
	{
		m_nIndex = 0;
		parent::Create(10000, M_Bit(8), false);
	}

	void Check(const char* _pStr, uint32 _HashValue)
	{
		uint8 HashIndex = uint8(_HashValue) ^ uint8(_HashValue >> 8) ^ uint8(_HashValue >> 16) ^ uint8(_HashValue >> 24);
		int ID = parent::m_pHash[HashIndex];
		while (ID != -1)
		{
			const CHashIDInfo& IDInfo = parent::m_pIDInfo[ID];
			if (IDInfo.m_HashValue == _HashValue)
			{
				M_ASSERT( IDInfo.m_Str.CompareNoCase(_pStr) == 0, "Colliding hashes detected!");
				break;
			}
			ID = IDInfo.m_iNext;
		}
		if (ID == -1)
		{
			M_ASSERT(m_nIndex < 10000, "Out of hash entries");
			ID = m_nIndex++;
			parent::Insert(ID, HashIndex);
			parent::m_pIDInfo[ID].m_HashValue = _HashValue;
			parent::m_pIDInfo[ID].m_Str = _pStr;
		}
	}
};
static CollisionDetector s_CollisionDetector;

void CNameHash::DetectCollision(const char* _pStr)
{
	s_CollisionDetector.Check(_pStr, StringToHash(_pStr));
}


#endif // _DEBUG


