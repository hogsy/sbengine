#ifndef __WPHYSTATE_HASH_H
#define __WPHYSTATE_HASH_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			PhysState hashing helper classes

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWO_EnumParams_Box
					CWO_EnumParams_Line
					CWO_Hash
					CWO_SpaceEnum
\*____________________________________________________________________________________________*/

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| 
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_EnumParams_Box
{
public:
	CWO_EnumParams_Box() : m_RigidBodyID(0xFFFF) {};
	CBox3Dfp32 m_Box;
	uint16 m_ObjectFlags;
	uint16 m_ObjectIntersectFlags;
	uint16 m_ObjectNotifyFlags;
	uint16 m_RigidBodyID; // Needed for Broad phase early out
};

class CWO_EnumParams_Line
{
public:
	CVec3Dfp32 m_V0;
	CVec3Dfp32 m_V1;
	uint16 m_ObjectFlags;
	uint16 m_ObjectIntersectFlags;
	uint16 m_ObjectNotifyFlags;
};

class CWObject_CoreData;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Hash
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_Hash_Settings
{
public:
	int m_BoxSize;
	int m_BoxShiftSize;
	int m_BoxAndSize;
	int m_nBoxAndX;
	int m_nBoxAndY;
	int m_nBoxesX;
	int m_nBoxesY;
	int m_nBoxes : 30;
	int m_bIsServerCWObject : 1;
	int m_bUseLarge : 1;
	int m_HashAndSizeX;
	int m_HashAndSizeY;
	fp32 m_BoxesPerUnit;
	fp32 m_MaxXEnum;
	fp32 m_MaxYEnum;

	uint16 m_FirstLarge;

	inline void Init(int _nBoxes, int _BoxShiftSize, int _MaxIDs, bool _bUseLarge, bool _bIsServerCWObject)
	{
		m_BoxShiftSize = _BoxShiftSize;
		m_BoxSize = (1 << m_BoxShiftSize);
		m_BoxAndSize = m_BoxSize-1;

		m_nBoxesX = _nBoxes;
		m_nBoxesY = _nBoxes;
		m_nBoxes = m_nBoxesX * m_nBoxesY;
		m_HashAndSizeX = m_nBoxesX * m_BoxSize-1;
		m_HashAndSizeY = m_nBoxesY * m_BoxSize-1;

		m_nBoxAndX = m_nBoxesX - 1;
		m_nBoxAndY = m_nBoxesY - 1;

		m_BoxesPerUnit = 1.0f / fp32(m_BoxSize);

		m_bIsServerCWObject = _bIsServerCWObject != 0;
		m_bUseLarge = _bUseLarge;

		m_FirstLarge = 0;

		m_MaxXEnum = (m_nBoxesX-2) * m_BoxSize;
		m_MaxYEnum = (m_nBoxesY-2) * m_BoxSize;
	}
};


//
// Read Only version
//
class CWO_HashLink
{
public:
	uint16 m_iNext;
	uint16 m_iPrev;
};

template<typename TSPECIALIZATION>
class TWO_HashElement : public TSPECIALIZATION
{
public:
	mutable uint16 m_iHash[4];
};

template<typename TSPECIALIZATION>
class TWO_Hash_RO : public CWO_Hash_Settings
{
protected:
	typedef TWO_HashElement<typename TSPECIALIZATION::ElementMixinType> CWO_HashElement;

	typename TSPECIALIZATION::StorageType m_Data;

	__inline int GetLinkIndex(int _ID, int _InternalLinkNo)
	{
		return (_ID << 2) + _InternalLinkNo;
	}

	__inline int GetHashIndex(int _x, int _y)
	{
		return _x + (_y*CWO_Hash_Settings::m_nBoxesX) + 1;
	}

	__inline int GetLargeHashIndex()
	{
		return CWO_Hash_Settings::m_nBoxes+1;
	}

	__inline bool BoxHashEnum(const CWO_EnumParams_Box& _Params, int iHash, typename TSPECIALIZATION::EnumerateReturnType *_pEnumRetValue, int _MaxEnumRets, int &_nIDs);

public:
	TWO_Hash_RO();
	~TWO_Hash_RO();

	void Create(int _nBoxes, int _BoxShiftSize, int _MaxIDs, bool _bUseLarge, bool _bIsServerCWObject);

	int EnumerateLine(const CWO_EnumParams_Line& _Params, int16* _pEnumRetIDs, int _MaxEnumIDs, bool _bVisBox);
	int EnumerateBox(const CWO_EnumParams_Box& _Params, typename TSPECIALIZATION::EnumerateReturnType *_pEnumRetValue, int _MaxEnumRets);
};

//
// Write extented version
//
#ifndef PLATFORM_VPU
template<typename TSPECIALIZATION>
class TWO_Hash_RW : public TWO_Hash_RO<TSPECIALIZATION>
{
protected:
public:
	typedef TWO_Hash_RO<TSPECIALIZATION> CWO_Hash_RO;
	typedef TWO_HashElement<typename TSPECIALIZATION::ElementMixinType> CWO_HashElement;

	TWO_Hash_RW();
	~TWO_Hash_RW();
	void Create(int _nBoxes, int _BoxShiftSize, int _MaxIDs, bool _bUseLarge, bool _bIsServerCWObject);
	
	void Insert(CWObject_CoreData* _pObj);
	void Remove(int _ID);
};
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_SpaceEnum
|__________________________________________________________________________________________________
\*************************************************************************************************/
template<typename TSPECIALIZATION>
class TWO_SpaceEnum_RO : public CReferenceCount
{
protected:
	typename TSPECIALIZATION::HashType1 m_Hash1;
	typename TSPECIALIZATION::HashType2 m_Hash2;

public:
	TWO_SpaceEnum_RO() {};
	~TWO_SpaceEnum_RO() {};
	void Init(int _nBoxesSmall, int _BoxShiftSizeSmall, int _nBoxesLarge, int _BoxShiftSizeLarge, int _nObjects, bool _bIsServerCWObject);
	int EnumerateLine(const CWO_EnumParams_Line& _Params, int16* _pEnumRetIDs, int _MaxEnumIDs, bool _bVisBox);
	int EnumerateBox(const CWO_EnumParams_Box& _Params, typename TSPECIALIZATION::EnumerateReturnType *_pEnumRetIDs, int _MaxEnumIDs);
};

//
#ifndef PLATFORM_VPU
template<typename TSPECIALIZATION>
class TWO_SpaceEnum_RW : public TWO_SpaceEnum_RO<TSPECIALIZATION>
{
public:
	typedef TWO_SpaceEnum_RO<TSPECIALIZATION> CWO_SpaceEnum_RO;

	TWO_SpaceEnum_RW();
	~TWO_SpaceEnum_RW();
	void Create(int _nBoxesSmall, int _BoxShiftSizeSmall, int _nBoxesLarge, int _BoxShiftSizeLarge, int _nObjects, bool _bIsServerCWObject);
	void Insert(CWObject_CoreData* _pObj);
	void Remove(int _ID);
};

class CWO_SpaceEnumSpecialization_Common
{	
public:
	class CHashSpecialization
	{
	public:
		// internal storage type
		class CElementMixin
		{
		public:
			class CWObject_CoreData* m_pObj;
			int m_ObjectFlags;
			int m_ObjectIntersectFlags;
			int m_IntersectNotifyFlags;
			uint16 m_RigidBodyID;

			inline void Assign(CWObject_CoreData *_pObj);
			inline void Release();
			inline const CBox3Dfp32 *GetBox() const;
 			
			inline bool BoxEnumCheck(const CWO_EnumParams_Box& _Params, const CWO_Hash_Settings &_Settings) const;

			inline int16 GetReturnValue(int ID) const { return ID; }
		

		};
		
		typedef CElementMixin ElementMixinType;
		typedef int16 EnumerateReturnType;

		class CStorage
		{
		public:
			TThinArray<CWO_HashLink> m_lLinks;
			TThinArray<TWO_HashElement<ElementMixinType> > m_lElements;
			TThinArray<int16> m_lHash;
			TThinArray<uint32> m_lTags;

			inline void Init(int _nBoxes, int _MaxIDs);
			inline CWO_HashLink &GetLink(int index) { return m_lLinks[index]; }
			inline TWO_HashElement<ElementMixinType> &GetElement(int index) { return m_lElements[index]; }
			inline int16 &GetHash(int index) { return m_lHash[index]; }

			inline void TagElement(int _ID) { m_lTags[_ID>>5] |= 1<<(_ID&31); }
			inline bool IsElementTagged(int _ID) { return (m_lTags[_ID>>5] & (M_BitD(_ID&31))) != 0; }
			inline void FinalizeEnum(EnumerateReturnType *_pRetValues, int _NumValues)
			{
				for(int i = 0; i < m_lTags.Len(); i++)
					m_lTags[i] = 0;
			}
		};

		typedef CStorage StorageType;
	};

	typedef int16 EnumerateReturnType;
	typedef TWO_Hash_RW<CHashSpecialization> HashType1;
	typedef TWO_Hash_RW<CHashSpecialization> HashType2;
};

typedef TWO_SpaceEnum_RW<CWO_SpaceEnumSpecialization_Common> CWO_SpaceEnum;

#endif // PLATFORM_VPU

#include "WHash.inl"

#endif // __INC


