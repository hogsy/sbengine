/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WNameHash.h

	Author:			Anton Ragnarsson

	Copyright:		Copyright Starbreeze AB 2005

	Contents:		CNameHash

	Comments:		This class is used to store strings as a 32-bit integer.
					(In debug-mode, the source string is kept, and hash collisions are detected)

	History:		
		050304:		Created File
\*____________________________________________________________________________________________*/
#ifndef __WNameHash_h__
#define __WNameHash_h__


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CNameHash
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CNameHash
{
	uint32 m_Hash;
public:

	CNameHash() : m_Hash(0) {}

	explicit M_INLINE CNameHash(const char* _pStr)
	{
		*this = _pStr;
	}

	explicit M_INLINE CNameHash(const CStrBase& _Str)
	{
		*this = _Str;
	}

	operator uint32() const
	{
		return m_Hash; 
	}

	bool operator==(const CNameHash& _Other) const
	{
		return (m_Hash == _Other.m_Hash); 
	}

	bool operator==(const char* _pName) const
	{
		uint nHash = StringToHash(_pName); 
		return (m_Hash == nHash); 
	}

	bool operator!=(const CNameHash& _Other) const
	{
		return (m_Hash != _Other.m_Hash); 
	}

	bool operator!=(const char* _pName) const
	{
		uint nHash = StringToHash(_pName); 
		return (m_Hash != nHash); 
	}

	CNameHash& operator=(const char* _pName) 
	{ 
		m_Hash = StringToHash(_pName); 
#ifdef _DEBUG
		m_Name = _pName;
		DetectCollision(_pName);
#endif
		return *this; 
	}

	M_INLINE CNameHash& operator=(const CStrBase& _Str)
	{
		M_ASSERT(_Str.IsAnsi(), "String must be ansi!");
		return (*this = _Str.Str());
	}


	//
	// Debug stuff
	//
#ifdef _DEBUG
	CStr m_Name;
	void DetectCollision(const char* _pStr);
	const CStr& DbgName() const { return m_Name; }
#else
	CStr DbgName() const { return CStrF("hash{%08X}", m_Hash); }
#endif

	//
	// Autovar packing/unpacking
	//
	void Pack(uint8*& _pD) const;
	void Unpack(const uint8*& _pD);

	void Write(CCFile* _pFile) const;
	void Read(CCFile* _pFile);
};





#endif // __WNameHash_h__
