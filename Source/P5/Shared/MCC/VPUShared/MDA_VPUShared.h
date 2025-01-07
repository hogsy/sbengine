

#ifndef MACRO_INC_MDA_VPUShared_h
#define MACRO_INC_MDA_VPUShared_h

#include "MCC_VPUShared.h"


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TArrayPtr
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*
	// Range-check modes:				Debug	Release	RTM/RTMD
	TAP								//	No		No		No
	TAP_RCD							//	Yes		No		No
	TAP_RCNRTM						//  Yes		Yes		No
	TAP_RCA							//	Yes		Yes		Yes
*/

#ifdef _DEBUG
	#define TAP TArrayPtr
	#define TAP_RCA TArrayPtrRC
	#define TAP_RCD TArrayPtrRC
	#define TAP_RCNRTM TArrayPtrRC
#elif defined M_RTM
	#define TAP TArrayPtr
	#define TAP_RCA TArrayPtrRC
	#define TAP_RCD TArrayPtr
	#define TAP_RCNRTM TArrayPtr
#else
	#define TAP TArrayPtr
	#define TAP_RCA TArrayPtrRC
	#define TAP_RCD TArrayPtrRC
	#define TAP_RCNRTM TArrayPtr
#endif

template<class T>
class TArrayPtr
{
public:
	T* m_pArray;
	int m_Len;

	int Len() const
	{ 
		return m_Len;
	};

	TArrayPtr()
	{
		m_pArray = NULL;
		m_Len = 0;
	}

	TArrayPtr(T* _pArray, int _Len)
		: m_pArray(_pArray)
		, m_Len(_Len)
	{
	}

	void Set(T* _pArray, int _Len)
	{
		m_pArray = _pArray;
		m_Len = _Len;
	}

	TArrayPtr operator + (int _n)
	{
		M_ASSERT(m_Len - _n >= 0, "!");

		TArrayPtr Ret;
		Ret.m_pArray = m_pArray + _n;
		Ret.m_Len = m_Len - _n;
		return Ret;
	}

	template<class t_CType>
	TArrayPtr(TArrayPtr<t_CType>& _Array)
	{
		m_pArray = _Array.m_pArray;
		m_Len = _Array.m_Len;
	}

	template<class t_CType>
	TArrayPtr& operator= (TArrayPtr<t_CType>& _Array)
	{
		m_pArray = _Array.m_pArray;
		m_Len = _Array.m_Len;
		return *this;
	}

	template<class TArray>
	M_FORCEINLINE TArrayPtr(TArray& _Array)
	{
		m_pArray = _Array.GetBasePtr();
		m_Len = _Array.Len();
	}

	template<class TArray>
	M_FORCEINLINE TArrayPtr& operator= (TArray& _Array)
	{
		m_pArray = _Array.GetBasePtr();
		m_Len = _Array.Len();
		return *this;
	}

	M_FORCEINLINE T& operator[](int _iElem)
	{
		return m_pArray[_iElem];
	}
	M_FORCEINLINE const T& operator[](int _iElem) const
	{
		return m_pArray[_iElem];
	}

	M_FORCEINLINE int ListSize() { return m_Len * sizeof(T); };
	M_FORCEINLINE T* GetBasePtr() { return m_pArray; };
};

template<class T>
class TArrayPtrRC
{
public:
	T* m_pArray;
	int m_Len;

	int Len() const
	{ 
		return m_Len;
	};

	TArrayPtrRC()
	{
		m_pArray = NULL;
		m_Len = 0;
	}

	TArrayPtrRC(T* _pArray, int _Len)
		: m_pArray(_pArray)
		, m_Len(_Len)
	{
	}

	void Set(T* _pArray, int _Len)
	{
		m_pArray = _pArray;
		m_Len = _Len;
	}

	TArrayPtrRC operator + (int _n)
	{
		M_ASSERT(m_Len - _n >= 0, "!");

		TArrayPtrRC Ret;
		Ret.m_pArray = m_pArray + _n;
		Ret.m_Len = m_Len - _n;
		return Ret;
	}

	template<class TArray>
	TArrayPtrRC(TArray& _Array)
	{
		m_pArray = _Array.GetBasePtr();
		m_Len = _Array.Len();
	}

	template<class TArray>
	TArrayPtrRC& operator= (TArray& _Array)
	{
		m_pArray = _Array.GetBasePtr();
		m_Len = _Array.Len();
		return *this;
	}

	template<class t_CType>
	TArrayPtrRC(TArrayPtrRC<t_CType>& _Array)
	{
		m_pArray = _Array.m_pArray;
		m_Len = _Array.m_Len;
	}

	template<class t_CType>
	TArrayPtrRC& operator= (TArrayPtrRC<t_CType>& _Array)
	{
		m_pArray = _Array.m_pArray;
		m_Len = _Array.m_Len;
		return *this;
	}

	T& operator[](int _iElem)
	{
		if (uint(_iElem) >= m_Len)
			ThrowError(CStrF("Index out of range. %d/%d", _iElem, m_Len));
		return m_pArray[_iElem];
	}
	const T& operator[](int _iElem) const
	{
		if (uint(_iElem) >= m_Len)
			ThrowError(CStrF("Index out of range. %d/%d", _iElem, m_Len));
		return m_pArray[_iElem];
	}

	M_FORCEINLINE int ListSize() { return m_Len * sizeof(T); };
	M_FORCEINLINE T* GetBasePtr() { return m_pArray; };
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TThinArray
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	TemplateClass:		Extremely lightweight array class.
						
	Parameters:			
		T:				Class
						
	Comments:			Consumes 4 bytes when array len is zero.
						Consumes (16 + sizeof(T) * len) if len > 0.
                        (8 bytes memory manager overhead included)
\*____________________________________________________________________*/

#ifdef PLATFORM_XENON
template <class T, int TAlign, int TExtraSpace = 0, typename t_CAllocator = DIdsDefaultAllocator, int bMemoryManagerAligned = TAlign <= 32>
#else
template <class T, int TAlign, int TExtraSpace = 0, typename t_CAllocator = DIdsDefaultAllocator, int bMemoryManagerAligned = 1>
#endif
class TThinArrayDataAligned
{
public:
	enum
	{
		ALIGN = (TAlign + 3) & ~3,				// Round to dwords
		ALIGNDWORDS = ALIGN >> 2,				// Below we assume sizeof(*this) is 4
	};

	uint32 m_Len;

	M_INLINE void Clear()
	{
		m_Len = 0;
	}

	M_INLINE TThinArrayDataAligned()
	{
		Clear();
	}

	M_INLINE T* Data()
	{
		return (T*)(this + ALIGNDWORDS);
	}

	M_INLINE const T* Data() const
	{
		return (T*)(this + ALIGNDWORDS);
	}

	static void Destroy(TThinArrayDataAligned* _pArray)
	{
		if (!_pArray)
			return;

		// Destruct all elements
		T* pT = _pArray->Data();
		for(int i = _pArray->m_Len-1; i >= 0; i--)
			pT[i].~T();

		t_CAllocator::Free(_pArray);
	}

	static TThinArrayDataAligned* Create(int _Len)
	{
		if (!_Len)
			return NULL;

		int Size = sizeof(TThinArrayDataAligned) * ALIGNDWORDS + sizeof(T) * _Len + TExtraSpace;
		TThinArrayDataAligned* pArray = (TThinArrayDataAligned*) t_CAllocator::AllocAlign(Size, ALIGN);
//		TThinArrayDataAligned* pArray = (TThinArrayDataAligned*) DNew(uint8) uint8[Size];		// Replace this with an alloc that takes alignment as a parameter
		if (!pArray) 
		{
			if (t_CAllocator::CanFail())
				return NULL;
			else
				ThrowError("Out of memory.");
		}

		pArray->Clear();

		// Construct all elements
		M_TRY
		{
			T* pT = pArray->Data();
			for(int i = 0; i < _Len; i++)
			{

				new (&pT[i]) T;

				pArray->m_Len++;
			}
		}
		M_CATCH(
		catch(CCException)
		{
			Destroy(pArray);
			throw;
		}
		)

		return pArray;
	}
};

// Partially specialize if memory manager cannot do alignment
template <class T, int TAlign, int TExtraSpace, typename t_CAllocator>
class TThinArrayDataAligned<T, TAlign, TExtraSpace, t_CAllocator, 0>
{
public:
	enum
	{
		ALIGN = (TAlign + 3) & ~3,				// Round to dwords
		ALIGNDWORDS = ALIGN >> 2,				// Below we assume sizeof(*this) is 4
		MEMMANALIGN = 32
	};

	uint32 m_Len;

	M_INLINE void Clear()
	{
		m_Len = 0;
	}

	M_INLINE TThinArrayDataAligned()
	{
		Clear();
	}

	M_INLINE T* Data()
	{
		mint Ret = (mint)(this + 1);
		Ret = (Ret + (ALIGN - 1)) & (~(ALIGN -1));
		return (T*)Ret;
	}

	M_INLINE const T* Data() const
	{
		mint Ret = (mint)(this + 1);
		Ret = (Ret + (ALIGN - 1)) & (~(ALIGN -1));
		return (T*)Ret;
	}

	static void Destroy(TThinArrayDataAligned* _pArray)
	{
		if (!_pArray)
			return;

		// Destruct all elements
		T* pT = _pArray->Data();
		for(int i = _pArray->m_Len-1; i >= 0; i--)
			pT[i].~T();

		t_CAllocator::Free(_pArray);
	}

	static TThinArrayDataAligned* Create(int _Len)
	{
		if (!_Len)
			return NULL;

		int Size = ALIGN * 2 + sizeof(T) * _Len + TExtraSpace;
		TThinArrayDataAligned* pArray = (TThinArrayDataAligned*) t_CAllocator::AllocAlign(Size, ALIGN);
//		TThinArrayDataAligned* pArray = (TThinArrayDataAligned*) DNew(uint8) uint8[Size];		// Replace this with an alloc that takes alignment as a parameter
		if (!pArray) 
		{
			if (t_CAllocator::CanFail())
				return NULL;
			else
				ThrowError("Out of memory.");
		}

		pArray->Clear();

		// Construct all elements
		M_TRY
		{
			T* pT = pArray->Data();
			for(int i = 0; i < _Len; i++)
			{

				new (&pT[i]) T;

				pArray->m_Len++;
			}
		}
		M_CATCH(
		catch(CCException)
		{
			Destroy(pArray);
			throw;
		}
		)

		return pArray;
	}
};

template <class T, int TExtraSpace = 0, typename t_CAllocator = DIdsDefaultAllocator>
class CThinArrayDataAutoAligned : public TThinArrayDataAligned<T, M_ALIGNMENTOF(T), TExtraSpace, t_CAllocator>
{
public:
	static void Destroy(CThinArrayDataAutoAligned* _pArray)
	{
		TThinArrayDataAligned<T, M_ALIGNMENTOF(T), TExtraSpace, t_CAllocator>::Destroy((TThinArrayDataAligned<T, M_ALIGNMENTOF(T), TExtraSpace, t_CAllocator>*) _pArray);
	}

	static CThinArrayDataAutoAligned* Create(int _Len)
	{
		return (CThinArrayDataAutoAligned*) TThinArrayDataAligned<T, M_ALIGNMENTOF(T), TExtraSpace, t_CAllocator>::Create(_Len);
	}
};

// -------------------------------------------------------------------
template <class T, typename t_CAllocator = DIdsDefaultAllocator, class TArrayData = CThinArrayDataAutoAligned<T, 0, t_CAllocator> >
class TThinArray
{
protected:

	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Class:				Holds array len and data in a single memory block.
							
		Comments:			Dynamic length structure
	\*____________________________________________________________________*/


	// -------------------------------------------------------------------
	TCDynamicPtr<typename t_CAllocator::CPtrHolder, TArrayData > m_pArray;
//	TArrayData* m_pArray;

	void CopyElements(T*,const T*,int);
	void CopyElements(T*,T*,int);

public:
	typedef T TElement;

	M_INLINE TThinArray();
	M_INLINE TThinArray(const TThinArray&);
	M_INLINE ~TThinArray();
	const TThinArray& operator=(const TThinArray&);

	void Clear();						// Set length to zero. All data is freed.
	void Destroy() { Clear(); }
	bint SetLen(int _Len);				// Set array length. Overlapping indices are copied to the new data-block.
	TCDynamicPtr<typename t_CAllocator::CPtrHolder, TArrayData > &GetArray()
	{
		return m_pArray;
	}

	void Add(const TThinArray<T, t_CAllocator, TArrayData> &_Src);

	M_INLINE int Len() const;					// Returns length of array.
	M_INLINE int ListSize() const;				// Returns sizeof(T) * Length
	M_INLINE bool ValidPos(int _Pos) const;		// Returns true if _Pos is a valid index.

	M_INLINE T* GetBasePtr();					// Returns pointer to element 0
	M_INLINE const T* GetBasePtr() const;		// Returns pointer to element 0
	M_INLINE T& operator[](int);					// Return reference to element x
	M_INLINE const T& operator[](int) const;		// Return const reference to element x

//	void WriteEntry(class CDataFile& _DFile, const char* _pEntryName, int _Version);
//	bool ReadEntry(class CDataFile& _DFile, const char* _pEntryName, int _CurrentVersion);
	M_FORCEINLINE int ListAllocatedSize() const { return ListSize(); };	// Returns sizeof(T) * Length
	void ListSwap(TThinArray<T, t_CAllocator, TArrayData> &_Src) { Swap(m_pArray, _Src.m_pArray); };
};

template <class T, int TAlign, int TExtraSpace = 0, typename t_CAllocator = DIdsDefaultAllocator> 
class TThinArrayAlign : public TThinArray<T, t_CAllocator, TThinArrayDataAligned<T, TAlign, TExtraSpace, t_CAllocator> >
{
};

// -------------------------------------------------------------------
//  Implementation:
// -------------------------------------------------------------------

template <class T, typename t_CAllocator, class TArrayData>
void TThinArray<T, t_CAllocator, TArrayData>::CopyElements(T* _pDst, const T* _pSrc, int _n)
{
	for(int i = 0; i < _n; i++)
		_pDst[i] = _pSrc[i];
}

template <class T, typename t_CAllocator, class TArrayData>
void TThinArray<T, t_CAllocator, TArrayData>::CopyElements(T* _pDst, T* _pSrc, int _n)
{
	for(int i = 0; i < _n; i++)
		_pDst[i] = _pSrc[i];
}

template <class T, typename t_CAllocator, class TArrayData>
void TThinArray<T, t_CAllocator, TArrayData>::Add(const TThinArray<T, t_CAllocator, TArrayData> &_Src)
{
	int nStart = Len();
	int nNeeded = nStart + _Src.Len();
	if (nNeeded != Len())
	{
		SetLen(nNeeded);
	}

	CopyElements(GetBasePtr() + nStart, _Src.GetBasePtr(), _Src.Len());
    
}

template <class T, typename t_CAllocator, class TArrayData>
M_INLINE TThinArray<T, t_CAllocator, TArrayData>::TThinArray()
{
	m_pArray = NULL;
}

template <class T, typename t_CAllocator, class TArrayData>
M_INLINE TThinArray<T, t_CAllocator, TArrayData>::TThinArray(const TThinArray& _Array)
{
	m_pArray = NULL;
	*this = _Array;
}

template <class T, typename t_CAllocator, class TArrayData>
M_INLINE TThinArray<T, t_CAllocator, TArrayData>::~TThinArray()
{
	Clear();
}

template <class T, typename t_CAllocator, class TArrayData>
const TThinArray<T, t_CAllocator, TArrayData>& TThinArray<T, t_CAllocator, TArrayData>::operator=(const TThinArray<T, t_CAllocator, TArrayData>& _Array)
{
	if (Len() != _Array.Len())
	{
		Clear();
		SetLen(_Array.Len());
	}

	CopyElements(GetBasePtr(), _Array.GetBasePtr(), Len());

	return *this;
}

// -------------------------------------------------------------------
template <class T, typename t_CAllocator, class TArrayData>
void TThinArray<T, t_CAllocator, TArrayData>::Clear()
{
	if (m_pArray)
		TArrayData::Destroy(m_pArray);
	m_pArray = NULL;
}

template <class T, typename t_CAllocator, class TArrayData>
bint TThinArray<T, t_CAllocator, TArrayData>::SetLen(int _Len)
{
	if (Len() == _Len) 
		return true;

	if (!_Len)
	{
		Clear();
		return true;
	}

	TArrayData* pNew = TArrayData::Create(_Len);
	if (!pNew) 
	{
		if (t_CAllocator::CanFail())
			return false;
		else
			ThrowError("Out of memory.");
	}

	M_TRY
	{
		int nCopy = Min(Len(), _Len);
		if (nCopy) CopyElements(pNew->Data(), m_pArray->Data(), nCopy);

		Clear();
	}
	M_CATCH(
	catch(CCException)
	{
		TArrayData::Destroy(pNew);
		throw;
	}
	)
	m_pArray = pNew;
	return true;
}

// -------------------------------------------------------------------
template <class T, typename t_CAllocator, class TArrayData>
M_INLINE int TThinArray<T, t_CAllocator, TArrayData>::Len() const
{
	return (m_pArray) ? m_pArray->m_Len : 0;
}

template <class T, typename t_CAllocator, class TArrayData>
M_INLINE int TThinArray<T, t_CAllocator, TArrayData>::ListSize() const
{
	return (m_pArray) ? m_pArray->m_Len * sizeof(T) : 0;
}

template <class T, typename t_CAllocator, class TArrayData>
M_INLINE bool TThinArray<T, t_CAllocator, TArrayData>::ValidPos(int _Pos) const
{
	return (_Pos < Len()) && (_Pos >= 0);
}

// -------------------------------------------------------------------
template <class T, typename t_CAllocator, class TArrayData>
M_INLINE T* TThinArray<T, t_CAllocator, TArrayData>::GetBasePtr()
{
	return (m_pArray) ? m_pArray->Data() : NULL;
}

template <class T, typename t_CAllocator, class TArrayData>
M_INLINE const T* TThinArray<T, t_CAllocator, TArrayData>::GetBasePtr() const
{
	return (m_pArray) ? m_pArray->Data() : NULL;
}

template <class T, typename t_CAllocator, class TArrayData>
M_INLINE T& TThinArray<T, t_CAllocator, TArrayData>::operator[](int _Pos)
{
#ifdef M_BOUNDCHECK
	if (!ValidPos(_Pos))
		ThrowError(CStrF("Index out of range. %d/%d", _Pos, Len()));
#endif

	return m_pArray->Data()[_Pos];
}

template <class T, typename t_CAllocator, class TArrayData>
M_INLINE const T& TThinArray<T, t_CAllocator, TArrayData>::operator[](int _Pos) const
{
#ifdef M_BOUNDCHECK
	if (!ValidPos(_Pos))
		ThrowError(CStrF("Index out of range. %d/%d", _Pos, Len()));
#endif

	return m_pArray->Data()[_Pos];
}

/*
template <class T, typename t_CAllocator, class TArrayData>
void TThinArray<T, t_CAllocator, TArrayData>::Write(CCFile& _File)
{
	T* pArray = GetBasePtr();
	int Len = Length();

	for(int i = 0; i < Len; i++)
		
}

template <class T, typename t_CAllocator, class TArrayData>
void TThinArray<T, t_CAllocator, TArrayData>::WriteEntry(CDataFile& _DFile, const char* _pEntryName, int _Version)
{
	_DFile.WriteArrayEntry(_pEntryName, GetBasePtr(), Length(), _Version);
}

template <class T, typename t_CAllocator, class TArrayData>
bool TThinArray<T, t_CAllocator, TArrayData>::ReadEntry(CDataFile& _DFile, const char* _pEntryName, int _CurrentVersion)
{
	bool bRes = true;
	_pDFile->PushPosition();

	if (_pDFile->GetNext("SOLID_PLANES"))
	{
		int Len = _DFile.GetUserData();
		SetLen(Len);
		_DFile.ReadArray(GetBasePtr(), _CurrentVersion);
	}
	else
		bRes = false;

	_pDFile->PopPosition();
	return bRes;
}
*/

template<int _Size> class intsize { public: };
template<> class intsize<8>  { public: typedef uint8  Type; };
template<> class intsize<16> { public: typedef uint16 Type; };
template<> class intsize<32> { public: typedef uint32 Type; };
template<> class intsize<64> { public: typedef uint64 Type; };


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TBitArray
|
| Static array of bits. Smallest possible Elemsize is automatically chosen..
|__________________________________________________________________________________________________
\*************************************************************************************************/
#define DBitArrayCheckIndex(i) M_ASSERT(((i) >> ElemShift) < BufferLen, "TBitArray: Index out of range!");
template<int NumBits>
class TBitArray
{
	enum { 
		ElemSize = (NumBits < 16 ? 1 : NumBits < 32 ? 2 : NumBits < 64 ? 4 : 8),
		ElemBits = ElemSize * 8,
		ElemMask = ElemBits - 1,
		ElemShift = DNumBits(((NumBits < 16 ? 1 : NumBits < 32 ? 2 : NumBits < 64 ? 4 : 8)) * 8 - 1),
		BufferLen = (NumBits + ElemMask) / ElemBits,
	};
	typedef typename intsize<ElemBits>::Type ElemType;

public:
	ElemType m_Buffer[BufferLen];

	bool IsClear() const
	{
		ElemType tmp=0;
		for (uint i = 0; i < BufferLen; i++)
			tmp|=m_Buffer[i];
		return tmp==0;
	}

	void Clear()
	{
		for (uint i = 0; i < BufferLen; i++)
			m_Buffer[i] = 0;
	}

	void ClearFirstBits(uint nBits)
	{
		uint BufferCount=(nBits+ElemMask)/ ElemBits;
		for (uint i = 0; i < BufferCount; i++)
			m_Buffer[i] = 0;
	}

	bint GetThenSet1(uint _iBit)
	{

		uint index = _iBit >> ElemShift;
		ElemType mask = M_BitTD<ElemType>(_iBit & ElemMask);
		bint retval= (m_Buffer[index] & mask) ? 1 : 0;
		m_Buffer[index] |= mask;
		return retval;
	}

	bint Get(uint _iBit) const
	{
		DBitArrayCheckIndex(_iBit);
		return (m_Buffer[_iBit >> ElemShift] & M_BitTD<ElemType>(_iBit & ElemMask)) ? 1 : 0;
	}

	void Set1(uint _iBit)
	{
		DBitArrayCheckIndex(_iBit);
		m_Buffer[_iBit >> ElemShift] |= M_BitTD<ElemType>(_iBit & ElemMask);
	}

	void Set0(uint _iBit)
	{
		DBitArrayCheckIndex(_iBit);
		m_Buffer[_iBit >> ElemShift] &= ~M_BitTD<ElemType>(_iBit & ElemMask);
	}

	void Set(uint _iBit, bint _Value)
	{
		DBitArrayCheckIndex(_iBit);
		ElemType mask = M_BitTD<ElemType>(_iBit >> ElemShift);
		ElemType& d = m_Buffer[_iBit >> ElemShift];
		d = (d & ~mask) | (_Value ? mask : 0);
	}
};


template<> 
class TBitArray<32>
{
public:
	uint32 m_Data;

	bool IsClear() const 
	{ 
		return m_Data==0; 
	}

	void Clear()
	{
		m_Data=0;
	}

	uint8 Get(uint _iBit) const
	{
		M_ASSERT(_iBit<32,"BitArray index out of range.");
		return (m_Data >>_iBit) & 1;
	}

	void Set1(uint _iBit)
	{
		M_ASSERT(_iBit<32,"BitArray index out of range.");
		m_Data|=M_BitD(_iBit);
	}

	void Set0(uint _iBit)
	{
		M_ASSERT(_iBit<32,"BitArray index out of range.");
		m_Data&=~M_BitD(_iBit);
	}
};


#endif

