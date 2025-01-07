
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Description:	-
					
	Author:			Magnus Högdahl
	Maintainer:		Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		Linked list classes
					Three classes
					Heap classes
					
	Comments:		
	
		template class	TLinkSP				000423	Ok			Generalized link-code from CSolid
		template class	TThinArray			010214	Ok			May not be DLL memory handling safe.
		template class	TList_Vector		9609??	Ok			Impl. vector, last revision jan 2000
		template class	TList_BitVector		99????	Incomplete	May not be DLL memory handling safe.
		template class	TList_Linked		9609??	Ok
		template class	TQueue				960914	Ok

		template class	TTree				961219
		template class	TTree_Var			961221
		template class	TSearchTree			961219

		template class	CIDHeap				97????	Ok			Helper class to implement fixed array object allocation.
		class			CKeyContainer		97????	Ok
		class			CKeyContainerNode	97????	Ok			n-Dimensional tree of CKeyContainer. These work alright, but new code should use CRegistry instead.
					
	History:		
		960901:		Created File
		010208:		Converted documentation format and added comments
		010214:		TThinArray added
		030505:		Updated Comments
\*_____________________________________________________________________________________________*/


#ifndef _INC_MOS_DA
#define _INC_MOS_DA

#include "MMisc.h"
#include "MCCInc.h"

#if defined(M_RTM) && defined(PLATFORM_CONSOLE)
#else
#define M_BOUNDCHECK
#endif


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Template:			Doubly linked list compatible with smart ptrs						
						
	Parameters:			
		T:				The Class that is to be linked
						
	Comments:			This class requires that the class linked supports
						smart ptrs
\*_____________________________________________________________________________*/

template<class T>
class TLinkSP
{
protected:
	// Prev/Next pointers
	T* m_pLSPPrev;
	TPtr<T> m_spLSPNext;

public:
	virtual T* GetThis()
	{
		LogFile("(TLinkSP::GetThis) Pure virtual function call.");
		Error_static("TLinkSP::GetThis", "Pure virtual function call.");
		return NULL;	// Get rid of warnings.
	}

	TLinkSP()
	{
		m_pLSPPrev = NULL;
	}

	~TLinkSP()
	{
		// Iterate from tail to avoid a destructor recursion which may potentially run out of stack.
		T* pTail = m_spLSPNext;
		if (!pTail) return;

		while(pTail->m_spLSPNext != NULL)
			pTail = pTail->m_spLSPNext;

		while(pTail && (pTail != this))
		{
			T* pPrev = pTail->m_pLSPPrev;
			if (!pPrev) break;
			pPrev->m_spLSPNext = NULL;
			pTail = pPrev;
		}
	}

	// Get prev/next
	const T* GetNext() const { return m_spLSPNext; }
	T* GetNext() { return m_spLSPNext; }
	const T* GetPrev() const { return m_pLSPPrev; }
	T* GetPrev() { return m_pLSPPrev; }

	// Get end of chain
	T* GetTail()
	{
		T* pObj = GetThis();
		while(pObj->m_spLSPNext != NULL)
			pObj = pObj->m_spLSPNext;
		return pObj;
	}

	// Get start of chain
	T* GetHead()
	{
		T* pObj = GetThis();
		while(pObj->m_pLSPPrev)
			pObj = pObj->m_pLSPPrev;
		return pObj;
	}

	// Unlink this from chain
	void Unlink()
	{
		if (m_pLSPPrev) m_pLSPPrev->m_spLSPNext = m_spLSPNext;
		if (m_spLSPNext != NULL) m_spLSPNext->m_pLSPPrev = m_pLSPPrev;
		m_spLSPNext = NULL;
		m_pLSPPrev = NULL;
	}

	// Unlink this from chain
	void UnlinkPrev()
	{
		if (m_pLSPPrev) m_pLSPPrev->m_spLSPNext = m_spLSPNext;
		m_pLSPPrev = NULL;
	}

	// Unlink this and it's tailing chain
	void UnlinkWithTail()
	{
		if (m_pLSPPrev) m_pLSPPrev->m_spLSPNext = NULL;
		m_pLSPPrev = NULL;
	}

	// Insert this and it's tailing chain after _spLink
	void LinkChainAfter(T* _pLink)
	{
		if (!_pLink) Error_static(CStrF("%s::LinkChainAfter", GetThis()->MRTC_ClassName()), "spLink == NULL");
		UnlinkPrev();
		T* pTail = GetThis();
		while(pTail->m_spLSPNext) pTail = pTail->m_spLSPNext;
		pTail->m_spLSPNext = _pLink->m_spLSPNext;
		if (pTail->m_spLSPNext) pTail->m_spLSPNext->m_pLSPPrev = pTail;
		m_pLSPPrev = _pLink;
		_pLink->m_spLSPNext = GetThis();
	}

	// Link this after _spLink, this is unlinked from it's current chain
	void LinkAfter(T* _pLink)
	{
		if (!_pLink) Error_static(CStrF("%s::LinkAfter", GetThis()->MRTC_ClassName()), "spLink == NULL");
		Unlink();
		m_spLSPNext = _pLink->m_spLSPNext;
		if (m_spLSPNext != NULL) m_spLSPNext->m_pLSPPrev = GetThis();
		m_pLSPPrev = _pLink;
		_pLink->m_spLSPNext = GetThis();
	}

	// Link this before _spLink, this is unlinked from it's current chain
	void LinkBefore(TPtr<T> _spLink)
	{
		if (!_spLink) Error_static(CStrF("%s::LinkBefore", GetThis()->MRTC_ClassName()), "spLink == NULL");
		Unlink();
		m_spLSPNext = _spLink;
		m_pLSPPrev = _spLink->m_pLSPPrev;
		if (m_pLSPPrev) m_pLSPPrev->m_spLSPNext = GetThis();
		_spLink->m_pLSPPrev = GetThis();
	}

	int GetChainLen()
	{
		int nLinks = 0;
		T* pObj = GetThis();
		while(pObj)
		{
			nLinks++;
			pObj = pObj->m_spLSPNext;
		}
		return nLinks;
	}
};

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
			Error_static("TArrayPtr::operator[]", CStrF("Index out of range. %d/%d", _iElem, m_Len));
		return m_pArray[_iElem];
	}
	const T& operator[](int _iElem) const
	{
		if (uint(_iElem) >= m_Len)
			Error_static("TArrayPtr::operator[]", CStrF("Index out of range. %d/%d", _iElem, m_Len));
		return m_pArray[_iElem];
	}
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
template <class T, int TAlign, int TExtraSpace = 0>
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

		delete[] (uint8*)_pArray;
	}

	static TThinArrayDataAligned* Create(int _Len)
	{
		if (!_Len)
			return NULL;

		int Size = sizeof(TThinArrayDataAligned) * ALIGNDWORDS + sizeof(T) * _Len + TExtraSpace;
		TThinArrayDataAligned* pArray = (TThinArrayDataAligned*) M_ALLOCALIGN(Size, ALIGN);
//		TThinArrayDataAligned* pArray = (TThinArrayDataAligned*) DNew(uint8) uint8[Size];		// Replace this with an alloc that takes alignment as a parameter
		if (!pArray) Error_static("TThinArrayDataAligned::Create", "Out of memory.");

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

template <class T, int TExtraSpace = 0>
class CThinArrayDataAutoAligned : public TThinArrayDataAligned<T, M_ALIGNMENTOF(T)>
{
public:
	static void Destroy(CThinArrayDataAutoAligned* _pArray)
	{
		TThinArrayDataAligned<T, M_ALIGNMENTOF(T)>::Destroy((TThinArrayDataAligned<T, M_ALIGNMENTOF(T), TExtraSpace>*) _pArray);
	}

	static CThinArrayDataAutoAligned* Create(int _Len)
	{
		return (CThinArrayDataAutoAligned*) TThinArrayDataAligned<T, M_ALIGNMENTOF(T), TExtraSpace>::Create(_Len);
	}
};

// -------------------------------------------------------------------
template <class T, class TArrayData = CThinArrayDataAutoAligned<T> >
class TThinArray
{
protected:

	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Class:				Holds array len and data in a single memory block.
							
		Comments:			Dynamic length structure
	\*____________________________________________________________________*/


	// -------------------------------------------------------------------
	TArrayData* m_pArray;

	void CopyElements(T*,const T*,int);

public:
	M_INLINE TThinArray();
	M_INLINE TThinArray(const TThinArray&);
	M_INLINE ~TThinArray();
	const TThinArray& operator=(const TThinArray&);

	void Clear();						// Set length to zero. All data is freed.
	void Destroy() { Clear(); }
	void SetLen(int _Len);				// Set array length. Overlapping indices are copied to the new data-block.

	void Add(const TThinArray<T, TArrayData> &_Src);

	M_INLINE int Len() const;					// Returns length of array.
	M_INLINE int ListSize() const;				// Returns sizeof(T) * Length
	M_INLINE bool ValidPos(int _Pos) const;		// Returns true if _Pos is a valid index.

	M_INLINE T* GetBasePtr();					// Returns pointer to element 0
	M_INLINE const T* GetBasePtr() const;		// Returns pointer to element 0
	M_INLINE T& operator[](int);					// Return reference to element x
	M_INLINE const T& operator[](int) const;		// Return const reference to element x

//	void WriteEntry(class CDataFile& _DFile, const char* _pEntryName, int _Version);
//	bool ReadEntry(class CDataFile& _DFile, const char* _pEntryName, int _CurrentVersion);
};

template <class T, int TAlign> 
class TThinArrayAlign : public TThinArray<T, TThinArrayDataAligned<T, TAlign> >
{
};

// -------------------------------------------------------------------
//  Implementation:
// -------------------------------------------------------------------

template <class T, class TArrayData>
void TThinArray<T, TArrayData>::CopyElements(T* _pDst, const T* _pSrc, int _n)
{
	for(int i = 0; i < _n; i++)
		_pDst[i] = _pSrc[i];
}

template <class T, class TArrayData>
void TThinArray<T, TArrayData>::Add(const TThinArray<T, TArrayData> &_Src)
{
	int nStart = Len();
	int nNeeded = nStart + _Src.Len();
	if (nNeeded != Len())
	{
		SetLen(nNeeded);
	}

	CopyElements(GetBasePtr() + nStart, _Src.GetBasePtr(), _Src.Len());
    
}

template <class T, class TArrayData>
M_INLINE TThinArray<T, TArrayData>::TThinArray()
{
	m_pArray = NULL;
}

template <class T, class TArrayData>
M_INLINE TThinArray<T, TArrayData>::TThinArray(const TThinArray& _Array)
{
	m_pArray = NULL;
	*this = _Array;
}

template <class T, class TArrayData>
M_INLINE TThinArray<T, TArrayData>::~TThinArray()
{
	Clear();
}

template <class T, class TArrayData>
const TThinArray<T, TArrayData>& TThinArray<T, TArrayData>::operator=(const TThinArray<T, TArrayData>& _Array)
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
template <class T, class TArrayData>
void TThinArray<T, TArrayData>::Clear()
{
	if (m_pArray)
		TArrayData::Destroy(m_pArray);
	m_pArray = NULL;
}

template <class T, class TArrayData>
void TThinArray<T, TArrayData>::SetLen(int _Len)
{
	if (Len() == _Len) return;

	if (!_Len)
	{
		Clear();
		return;
	}

	TArrayData* pNew = TArrayData::Create(_Len);
	if (!pNew) Error_static("SetLen", "Out of memory.");

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
}

// -------------------------------------------------------------------
template <class T, class TArrayData>
M_INLINE int TThinArray<T, TArrayData>::Len() const
{
	return (m_pArray) ? m_pArray->m_Len : 0;
}

template <class T, class TArrayData>
M_INLINE int TThinArray<T, TArrayData>::ListSize() const
{
	return (m_pArray) ? m_pArray->m_Len * sizeof(T) : 0;
}

template <class T, class TArrayData>
M_INLINE bool TThinArray<T, TArrayData>::ValidPos(int _Pos) const
{
	return (_Pos < Len()) && (_Pos >= 0);
}

// -------------------------------------------------------------------
template <class T, class TArrayData>
M_INLINE T* TThinArray<T, TArrayData>::GetBasePtr()
{
	return (m_pArray) ? m_pArray->Data() : NULL;
}

template <class T, class TArrayData>
M_INLINE const T* TThinArray<T, TArrayData>::GetBasePtr() const
{
	return (m_pArray) ? m_pArray->Data() : NULL;
}

template <class T, class TArrayData>
M_INLINE T& TThinArray<T, TArrayData>::operator[](int _Pos)
{
#ifdef M_BOUNDCHECK
	if (!ValidPos(_Pos))
		Error_static("TThinArray::operator[]", CStrF("Index out of range. %d/%d", _Pos, Len()));
#endif

	return m_pArray->Data()[_Pos];
}

template <class T, class TArrayData>
M_INLINE const T& TThinArray<T, TArrayData>::operator[](int _Pos) const
{
#ifdef M_BOUNDCHECK
	if (!ValidPos(_Pos))
		Error_static("TThinArray::operator[] const", CStrF("Index out of range. %d/%d", _Pos, Len()));
#endif

	return m_pArray->Data()[_Pos];
}

/*
template <class T, class TArrayData>
void TThinArray<T, TArrayData>::Write(CCFile& _File)
{
	T* pArray = GetBasePtr();
	int Len = Length();

	for(int i = 0; i < Len; i++)
		
}

template <class T, class TArrayData>
void TThinArray<T, TArrayData>::WriteEntry(CDataFile& _DFile, const char* _pEntryName, int _Version)
{
	_DFile.WriteArrayEntry(_pEntryName, GetBasePtr(), Length(), _Version);
}

template <class T, class TArrayData>
bool TThinArray<T, TArrayData>::ReadEntry(CDataFile& _DFile, const char* _pEntryName, int _CurrentVersion)
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

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CListVectorCore
|__________________________________________________________________________________________________
\*************************************************************************************************/

#define TLIST_VECTOR_DEFAULTGROW 16

#define TLIST_VECTOR_MEMORYLOG		1
#define TLIST_VECTOR_RECURSIONPROT	2

class MCCDLLEXPORT CListVectorData
{
public:
	int m_Len;
	int m_AllocLen;
	int m_nGrow;
	int m_ElemSize;
	int32 m_nRef;
	void* m_pList;

	CListVectorData();
	dllvirtual void Delete() { if (this) delete this; };

	virtual void* AllocObjects(int _nObjects) pure;
	virtual void FreeScalar(void* _pObj, int _nElem) pure;
	virtual void ResetObjects(void* _pObj, int _nElem) pure;
	virtual void Copy(const void* _pSrc, void* _pDest) const pure;
	virtual const char* ClassName() const pure;
};

class CListVectorCoreDummy
{
public:
};

template <typename TParent>
class MCCDLLEXPORT CListVectorCore : public TParent
{
protected:
	CListVectorData* m_pData;

	void* GetElement(void* m_pList, int _iElem);
	void CopyElements(const void* _pSrc, void* _pDest, int _nElems) const;
	void InsertxElements(int _Pos, const void* _pElem, int _nElem, bool _bInitGrow);
	void InsertxBlank(int _Pos, int _nElem, bool _bRespectGrow = false);

#ifdef M_BOUNDCHECK
	void RangeCheckError(int _Pos) const;
#endif

public:
	CListVectorCore();
	~CListVectorCore();

	static int m_GlobalFlags;

	static void SetGlobalFlags(int _Flg);
	static int GetGlobalFlags();

	// General routines
	M_INLINE int Len() const 
	{ 
		return (m_pData) ? m_pData->m_Len : 0; 
	};
	M_INLINE bool ValidPos(int _Pos) const 
	{ 
		return m_pData && (_Pos >= 0) && (_Pos < m_pData->m_Len); 
	};
	void Clear();
	void Delx(int _Pos, int _nDel, bool _bAlwaysKeepData = false);
	void Del(int _Pos);

protected:
	void Core_SetLen(int _Len);
	void Core_GrowLen(int _Len);
	void Core_QuickSetLen(int _Len);				// No adjustment of allocated-size if possible.

public:
	int GetGrow() const;
	M_INLINE int ListSize() const
	{
		if (!m_pData) return 0;return m_pData->m_Len * m_pData->m_ElemSize;
	}
	M_INLINE int ElementSize() const 
	{
		return (m_pData) ? m_pData->m_ElemSize : 0; 
	};
	M_INLINE int ListRef() const 
	{ 
		return (m_pData) ? m_pData->m_nRef : 0; 
	};
	void OptimizeMemory();
	int ListAllocatedSize() const;
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TList_Vector (TArray)
| Denna är "referens-säker". Alltså, en lista med CStr'ar eller TPtr<> fungerar.
|__________________________________________________________________________________________________
\*************************************************************************************************/
/*
class TList_Vector_CNonAligned
{
public:
	template <class t_CData>
	M_INLINE static void *AllocObjects(int _nObjects, t_CData* _Type)
	{
		return DNew(t_CData) t_CData[_nObjects];
	}

	template <class t_CData>
	M_INLINE static void FreeScalar(void* _pObj, int _nElem, t_CData* _Type)
	{
		delete[] ((t_CData*)_pObj);
	}
};

template <int t_Align = 4>
class TList_Vector_CAligned
{
public:
	template <class t_CData>
	static void *AllocObjects(int _nObjects, t_CData* _Type)
	{
		int Size = _nObjects * sizeof(t_CData) + t_Align + sizeof (void *);
		void *pOrg = M_ALLOC(Size);
		t_CData *pAligned = (t_CData *)(((((unsigned int)(pOrg)) + sizeof(void *)) + (t_Align - 1)) & (~(t_Align - 1)));
		*(((unsigned int *)pAligned) - 1) = (unsigned int)pOrg;
		for (int i = 0; i < _nObjects; ++i)
		{

			new((pAligned+i)) t_CData;

		}

		return pAligned;
	}

	template <class t_CData>
	static void FreeScalar(void* _pObj, int _nElem, t_CData* _Type)
	{
		for (int i = 0; i < _nElem; ++i)
		{
			(((t_CData *)_pObj)+i)->~t_CData();
		}

		MRTC_MemFree((void *)(*(((unsigned int *)_pObj) - 1)));
	}
};
*/
template <int TExtraSpace = 0>
class TList_Vector_AutoAlign
{
public:
	template <class T>
	static void *AllocObjects(int _nObjects, T* _Type)
	{
		int Size = _nObjects * sizeof(T) + TExtraSpace;
//		void *pOrg = M_ALLOC(Size);
		void *pOrg = M_ALLOCALIGN(Size, M_ALIGNMENTOF(T));
		T *pAligned = (T *)pOrg;
		for (int i = 0; i < _nObjects; ++i)
		{

			new((pAligned+i)) T;

		}

		return pAligned;
	}

	template <class T>
	static void FreeScalar(void* _pObj, int _nElem, T* _Type)
	{
		for (int i = 0; i < _nElem; ++i)
		{
			(((T *)_pObj)+i)->~T();
		}

		MRTC_MemFree(_pObj);
	}
};

template <class T, class TAlignClass = TList_Vector_AutoAlign<>, class TCoreParent = CObj>
class TList_Vector : public CListVectorCore<TCoreParent>
{
	// -------------------------------------------------------------------
	class CListData : public CListVectorData
	{
		void* AllocObjects(int _nObjects)
		{
			CListVectorData::AllocObjects(_nObjects);
			return TAlignClass::AllocObjects(_nObjects, (T*)NULL);
		}

		void ResetObjects(void* _pObj, int _nElem)
		{
			for (int i = 0; i < _nElem; ++i)
			{
				(((T *)_pObj)+i)->~T();
				new (((T *)_pObj)+i) T;
			}
		}

		void FreeScalar(void* _pObj, int _nElem)
		{
			if (_pObj)
			{
				CListVectorData::FreeScalar(_pObj, _nElem);
				TAlignClass::FreeScalar(_pObj, _nElem, (T*)NULL);
			}
		}

		void Copy(const void* _pSrc, void* _pDest) const
		{
			*((T*)_pDest) = *((T*)_pSrc);
		}

		const char* ClassName() const
		{
#ifdef COMPILER_RTTI
			return typeid(T).name();
#else
			return "Unknown type";
#endif
		}
	};

	// -------------------------------------------------------------------
public:
	typedef CListVectorCore<TCoreParent> CSuper;

	TList_Vector();
	~TList_Vector();

	//dllvirtual 
		void Init();
	//dllvirtual 
		void Destroy();

	TList_Vector(const TList_Vector& _List);	// Does not duplicate!, makes a reference.
	void operator= (const TList_Vector& _List);

	// Additional class specific
	int Add(const T& _Elem);
	void Insertx(int _Pos, const T* _pElem, int _nElem);
	void Insert(int _Pos, const T& _Elem);

	M_INLINE T& operator[] (int _Pos);
	M_INLINE const T& operator[] (int _Pos) const;

	M_INLINE T* GetBasePtr();
	M_INLINE const T* GetBasePtr() const;
	void Duplicate(TList_Vector* _pDestList) const;
	void Add(const TList_Vector* _pList);
	void Add(const TList_Vector &_pList)
	{
		Add(&_pList);
	}
	void Insert(int _Pos, const TList_Vector* _pList);

	void SetLen(int _Len);
	void GrowLen(int _Len);					// Set length of array but realloc using grow size if needed
	void SetGrow(int _nGrow);
	int GetGrow();
	void QuickSetLen(int _Len);				// No adjustment of allocated-size if possible.


	// Utility function that makes sure list is at least '_Len' elements long
	void SetMinLen(uint _Len)				
	{
		QuickSetLen( Max(_Len, (uint)CListVectorCore<TCoreParent>::Len()) );
	}

	static void Private_QSortSwap(TAP_RCNRTM<T> &_List, int _i0, int _i1)
	{
		T Temp = _List[_i0];
		_List[_i0] = _List[_i1];
		_List[_i1] = Temp;
	}

	template <typename t_CSort, typename t_CContext>
	static void Private_QSortHash_r(TAP_RCNRTM<T> &_List, int _iStart, int _iEnd, t_CContext *_pContext)
	{
		MAUTOSTRIP(CXR_NavGraph_Builder_CConstructionNode_QSortNCs_r, MAUTOSTRIP_VOID);
		//Check for final case
		if (_iStart >= _iEnd)
		{
			return;
		}

		//Get pivot value
		T Pivot = _List[(_iEnd + _iStart) / 2];

		//Loop through list until indices cross
		int iStart = _iStart;
		int iEnd = _iEnd;
		do 
		{
			//Find the first value that is greater than or equal to the pivot .
			while( (t_CSort::Compare(_pContext, _List[iStart], Pivot) < 0) )
			iStart++;

			//Find the last value that is smaller than or equal to the pivot .
			while( (t_CSort::Compare(_pContext, _List[iEnd], Pivot) > 0) )
			iEnd--;

			//If the indexes have not crossed, swap stuff
			if( iStart <= iEnd ) 
			{
				if (iStart != iEnd)
					Private_QSortSwap(_List, iStart, iEnd);
				iStart++;
				iEnd--;
			}
		}
		while (iStart <= iEnd);

		//Sort left partition if end index hasn't reached start
		if( _iStart < iEnd )
			Private_QSortHash_r<t_CSort, t_CContext>(_List, _iStart, iEnd, _pContext);

		//Sort right partition if start index hasn't reached end
		if( iStart < _iEnd )
			Private_QSortHash_r<t_CSort, t_CContext>(_List, iStart, _iEnd, _pContext);
	};

	template <typename t_CSort, typename t_CContext>
	void QSort(t_CContext *_pContext, int _nMax = -1)
	{
		TAP_RCNRTM<T> Tap(*this);
		if (_nMax < 0)
			Private_QSortHash_r<t_CSort, t_CContext> (Tap, 0, CSuper::Len()-1, _pContext);
		else
			Private_QSortHash_r<t_CSort, t_CContext> (Tap, 0, MinMT(_nMax, CSuper::Len())-1, _pContext);
	}
	template <typename t_CSort>
	void QSort(int _nMax = -1)
	{
		TAP_RCNRTM<T> Tap(*this);
		if (_nMax < 0)
			Private_QSortHash_r<t_CSort, const void * > (Tap, 0, CSuper::Len()-1, (const void *)NULL);
		else
			Private_QSortHash_r<t_CSort, const void * > (Tap, 0, MinMT(_nMax, CSuper::Len())-1, (const void *)NULL);
	}

	template <typename t_CSort, typename t_CContext, typename t_CFind>
	int BinarySearch(const t_CFind &_ToFind, t_CContext *_pContext, int _nMax = -1)
	{
		TAP_RCNRTM<T> Tap(*this);

		int Len = Tap.Len();
		if (_nMax >= 0)
			Len = Min(_nMax, Len);
		int Low = 0;
		int High = Len;

		while(Low < High)
		{
			int Mid = (Low + High) >> 1;
			if(t_CSort::Compare(_pContext, Tap[Mid], _ToFind) < 0)
				Low = Mid + 1;
			else
				High = Mid;
		}
		if(Low >= 0 && Low < Len && t_CSort::Compare(_pContext, Tap[Low], _ToFind) == 0)
			return Low;
		else
			return -1;

	}

	template <typename t_CSort, typename t_CFind>
	int BinarySearch(const t_CFind &_ToFind, int _nMax = -1)
	{
		return BinarySearch<t_CSort, t_CFind, const void *>(_ToFind, (const void *)0, _nMax);
	}
};

#define TArray TList_Vector

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TList_Vector (TArray), Implementation
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*
template <class T>
void* TList_Vector<T>::Alloc(int _nObjects)
{
	return DNew(T) T[_nObjects];
}

template <class T>
void TList_Vector<T>::Free(void* _pObj)
{
	delete ((T*)_pObj);
}

template <class T>
void TList_Vector<T>::FreeScalar(void* _pObj)
{
	delete[] ((T*)_pObj);
}

template <class T>
void TList_Vector<T>::Copy(const void* _pSrc, void* _pDest) const
{
	*((T*)_pDest) = *((T*)_pSrc);
}
*/
// -------------------------------------------------------------------
template <class T, class TAlignClass, class TCoreParent>
void TList_Vector<T, TAlignClass, TCoreParent>::Init()
{
	Destroy();

	CListVectorCore<TCoreParent>::m_pData = DNew(CListData) CListData;
	if (!CListVectorCore<TCoreParent>::m_pData) MemError_static("Init");
	CListVectorCore<TCoreParent>::m_pData->m_nRef = 1;
	CListVectorCore<TCoreParent>::m_pData->m_ElemSize = sizeof(T);
}

template <class T, class TAlignClass, class TCoreParent>
void TList_Vector<T, TAlignClass, TCoreParent>::Destroy()
{
	if (CListVectorCore<TCoreParent>::m_pData && CListVectorCore<TCoreParent>::m_pData->m_nRef)
	{
//		m_pData->m_nRef--;
//		if (!m_pData->m_nRef)
		if (!(MRTC_SystemInfo::Atomic_Decrease(&CListVectorCore<TCoreParent>::m_pData->m_nRef) - 1))
		{
			if (CListVectorCore<TCoreParent>::m_pData->m_pList) CListVectorCore<TCoreParent>::m_pData->FreeScalar(CListVectorCore<TCoreParent>::m_pData->m_pList, CListVectorCore<TCoreParent>::m_pData->m_AllocLen);
			CListVectorCore<TCoreParent>::m_pData->m_pList = NULL;
			CListVectorCore<TCoreParent>::m_pData->Delete();
		}
		CListVectorCore<TCoreParent>::m_pData = NULL;
	}
}

template <class T, class TAlignClass, class TCoreParent>
TList_Vector<T, TAlignClass, TCoreParent>::TList_Vector()
{
	CListVectorCore<TCoreParent>::m_pData = NULL;
//	Init();
//	m_pData->m_ElemSize = sizeof(T);
};

template <class T, class TAlignClass, class TCoreParent>
TList_Vector<T, TAlignClass, TCoreParent>::~TList_Vector()
{
	Destroy();
}

template <class T, class TAlignClass, class TCoreParent>
TList_Vector<T, TAlignClass, TCoreParent>::TList_Vector(const TList_Vector<T, TAlignClass, TCoreParent>& _List)
{
	operator=(_List);
}

template <class T, class TAlignClass, class TCoreParent>
void TList_Vector<T, TAlignClass, TCoreParent>::operator= (const TList_Vector<T, TAlignClass, TCoreParent>& _List)
{
	if (!_List.m_pData) const_cast<TList_Vector<T, TAlignClass, TCoreParent>&>(_List).Init();
	MRTC_SystemInfo::Atomic_Increase(&_List.m_pData->m_nRef);
//	_List.m_pData->m_nRef++;
	Destroy();
	CListVectorCore<TCoreParent>::m_pData = _List.m_pData;
}

template <class T, class TAlignClass, class TCoreParent>
int TList_Vector<T, TAlignClass, TCoreParent>::Add(const T& _Elem)
{
	if (!CListVectorCore<TCoreParent>::m_pData) Init();
	InsertxElements(CListVectorCore<TCoreParent>::m_pData->m_Len, &_Elem, 1, true);
	return CListVectorCore<TCoreParent>::m_pData->m_Len-1;
}

template <class T, class TAlignClass, class TCoreParent>
void TList_Vector<T, TAlignClass, TCoreParent>::Insertx(int _Pos, const T* _pElem, int _nElem)
{
	if (!CListVectorCore<TCoreParent>::m_pData) Init();
	InsertxElements(_Pos, _pElem, _nElem, false);
}

template <class T, class TAlignClass, class TCoreParent>
void TList_Vector<T, TAlignClass, TCoreParent>::Insert(int _Pos, const T& _Elem)
{
	if (!CListVectorCore<TCoreParent>::m_pData) Init();
	InsertxElements(_Pos, &_Elem, 1, true);
}

template <class T, class TAlignClass, class TCoreParent>
M_INLINE T& TList_Vector<T, TAlignClass, TCoreParent>::operator[] (int _Pos)
{
#ifdef M_BOUNDCHECK
	if (!CListVectorCore<TCoreParent>::m_pData || (_Pos < 0) || (_Pos >= CListVectorCore<TCoreParent>::m_pData->m_Len)) CListVectorCore<TCoreParent>::RangeCheckError(_Pos);
#endif
	return ((T*)CListVectorCore<TCoreParent>::m_pData->m_pList)[_Pos]; 
}

template <class T, class TAlignClass, class TCoreParent>
M_INLINE const T& TList_Vector<T, TAlignClass, TCoreParent>::operator[] (int _Pos) const
{
#ifdef M_BOUNDCHECK
	if (!CListVectorCore<TCoreParent>::m_pData || (_Pos < 0) || (_Pos >= CListVectorCore<TCoreParent>::m_pData->m_Len)) CListVectorCore<TCoreParent>::RangeCheckError(_Pos);
#endif
	return ((T*)CListVectorCore<TCoreParent>::m_pData->m_pList)[_Pos]; 
}

template <class T, class TAlignClass, class TCoreParent>
M_INLINE T* TList_Vector<T, TAlignClass, TCoreParent>::GetBasePtr()
{
	return (CListVectorCore<TCoreParent>::m_pData) ? (T*) CListVectorCore<TCoreParent>::m_pData->m_pList : NULL;
}

template <class T, class TAlignClass, class TCoreParent>
M_INLINE const T* TList_Vector<T, TAlignClass, TCoreParent>::GetBasePtr() const
{
	return (CListVectorCore<TCoreParent>::m_pData) ? (T*) CListVectorCore<TCoreParent>::m_pData->m_pList : NULL;
}

template <class T, class TAlignClass, class TCoreParent>
void TList_Vector<T, TAlignClass, TCoreParent>::Duplicate(TList_Vector* _pDestList) const
{
	_pDestList->SetLen(CListVectorCore<TCoreParent>::Len());
	if (CListVectorCore<TCoreParent>::m_pData && CListVectorCore<TCoreParent>::Len()) CopyElements(CListVectorCore<TCoreParent>::m_pData->m_pList, _pDestList->m_pData->m_pList, CListVectorCore<TCoreParent>::m_pData->m_Len);
}

template <class T, class TAlignClass, class TCoreParent>
void TList_Vector<T, TAlignClass, TCoreParent>::Add(const TList_Vector* _pList)
{
	if (!CListVectorCore<TCoreParent>::m_pData) Init();
	Insertx(CListVectorCore<TCoreParent>::Len(), _pList->GetBasePtr(), _pList->Len());
}

template <class T, class TAlignClass, class TCoreParent>
void TList_Vector<T, TAlignClass, TCoreParent>::Insert(int _Pos, const TList_Vector* _pList)
{
	if (!CListVectorCore<TCoreParent>::m_pData) Init();
	Insertx(_Pos, _pList->GetBasePtr(), _pList->Len());
}

template <class T, class TAlignClass, class TCoreParent>
void TList_Vector<T, TAlignClass, TCoreParent>::SetLen(int _Len)
{
	if (!CListVectorCore<TCoreParent>::m_pData && !_Len) return;
	if (!CListVectorCore<TCoreParent>::m_pData) Init();
	CListVectorCore<TCoreParent>::Core_SetLen(_Len);
}

template <class T, class TAlignClass, class TCoreParent>
void TList_Vector<T, TAlignClass, TCoreParent>::GrowLen(int _Len)
{
	if (!CListVectorCore<TCoreParent>::m_pData && !_Len) return;
	if (!CListVectorCore<TCoreParent>::m_pData) Init();
	CListVectorCore<TCoreParent>::Core_GrowLen(_Len);
}

template <class T, class TAlignClass, class TCoreParent>
void TList_Vector<T, TAlignClass, TCoreParent>::SetGrow(int _nGrow)
{
	if (!CListVectorCore<TCoreParent>::m_pData) Init();
	CListVectorCore<TCoreParent>::m_pData->m_nGrow = _nGrow;
}

template <class T, class TAlignClass, class TCoreParent>
int TList_Vector<T, TAlignClass, TCoreParent>::GetGrow()
{
	if (!CListVectorCore<TCoreParent>::m_pData) return TLIST_VECTOR_DEFAULTGROW;
	return CListVectorCore<TCoreParent>::m_pData->m_nGrow;
}

template <class T, class TAlignClass, class TCoreParent>
void TList_Vector<T, TAlignClass, TCoreParent>::QuickSetLen(int _Len)
{
	if (!CListVectorCore<TCoreParent>::m_pData && !_Len) return;
	if (!CListVectorCore<TCoreParent>::m_pData) Init();
	CListVectorCore<TCoreParent>::Core_QuickSetLen(_Len);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Some predefined lists.
|__________________________________________________________________________________________________
\*************************************************************************************************/

//typedef TList_Vector<CVec3Dfp4> lCVec3Dfp4;
//typedef TList_Vector<CVec3Dfp8> lCVec3Dfp8;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TList_SortableVector
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	TemplateClass:		Short_desscription
						
	Parameters:			
		T:				Class
		TIndex:			Predefined
						
	Comments:			
						NOTE: This one will commit kamikaze with more than 64k elements.
						The class T must implement the function: int Compare(const T& _Elem) const;
\*____________________________________________________________________*/

template<class T, class TIndex = uint16>
class TList_SortableVector : public TList_Vector<T>
{
	class CTreeNode
	{
	public:
		uint16 m_iElem;
		uint16 m_iNodeLess;
		uint16 m_iNodeGreater;
	};

	TList_Vector<CTreeNode> m_lNodes;

	static int M_ARGLISTCALL QSortCompare(const void* _pElem1, const void* _pElem2);

public:
	// Some new neat routines
	void Sort(bool _bDecreasing = false);
	void Sort(int _iStart, int _iStop, bool _bDecreasing = false);
	int Find(const T& _Elem, int _iLastElement = -1);
	void BuildSearchTree(bool _bAllowScramble = false);
	void DestroySearchTree();
};

//----------------------------------------------------------------
template<class T, class TIndex>
int M_ARGLISTCALL TList_SortableVector<T, TIndex>::QSortCompare(const void* _pElem1, const void* _pElem2)
{
	const T* pE1 = (T*)_pElem1;
	return pE1->Compare(*(const T*) _pElem2);
}

template<class T, class TIndex>
void TList_SortableVector<T, TIndex>::Sort(bool _bDecreasing)
{
	if (TList_SortableVector<T, TIndex>::Len()) qsort(TList_SortableVector<T, TIndex>::GetBasePtr(), TList_SortableVector<T, TIndex>::Len(), TList_SortableVector<T, TIndex>::ElementSize(), QSortCompare);
}

template<class T, class TIndex>
void TList_SortableVector<T, TIndex>::Sort(int _iStart, int _iStop, bool _bDecreasing)
{
	if (TList_SortableVector<T, TIndex>::Len()) qsort(TList_SortableVector<T, TIndex>::GetBasePtr() + _iStart, _iStop - _iStart + 1, TList_SortableVector<T, TIndex>::ElementSize(), QSortCompare);
}

template<class T, class TIndex>
int TList_SortableVector<T, TIndex>::Find(const T& _Elem, int _iLastElement)
{
	if (m_lNodes.Len())
	{
		// Not implemented.
		Error_static("TList_SortableVector<T, TIndex>::Find", "Not implemented.");
		return -1;
	}
	else
	{
		// Brute force.
		for(int i = _iLastElement+1; i < TList_SortableVector<T, TIndex>::Len(); i++)
			if ((*this)[i].Compare(_Elem) == 0) return i;
		return -1;
	}
}

template<class T, class TIndex>
void TList_SortableVector<T, TIndex>::BuildSearchTree(bool _bAllowScramble)
{
	// Not implemented.
}

template<class T, class TIndex>
void TList_SortableVector<T, TIndex>::DestroySearchTree()
{
	// Not implemented.
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Some predefined lists.
|__________________________________________________________________________________________________
\*************************************************************************************************/
typedef TList_SortableVector<CStr> lCStr;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TList_SimpleSortableVector
|__________________________________________________________________________________________________
\*************************************************************************************************/

template<class T>
class TList_SimpleSortableVector : public TList_Vector<T>
{
	static int M_ARGLISTCALL QSortCompare(const void* _pElem1, const void* _pElem2);

public:
	void Sort(bool _bDecreasing = false);
	int BinarySearch(const T& _Elem);
};

//----------------------------------------------------------------
template<class T>
int M_ARGLISTCALL TList_SimpleSortableVector<T>::QSortCompare(const void* _pElem1, const void* _pElem2)
{
	if((*(const T*)_pElem1) > (*(const T*)_pElem2))
		return 1;
	else if((*(const T*)_pElem1) < (*(const T*)_pElem2))
		return -1;
	else
		return 0;
}

template<class T>
void TList_SimpleSortableVector<T>::Sort(bool _bDecrease)
{
	if(TList_SimpleSortableVector<T>::Len() > 0)
		qsort(TList_SimpleSortableVector<T>::GetBasePtr(), TList_SimpleSortableVector<T>::Len(), TList_SimpleSortableVector<T>::ElementSize(), QSortCompare);
}

template<class T>
int TList_SimpleSortableVector<T>::BinarySearch(const T& _Elem)
{
	int Low = 0;
	int High = TList_SimpleSortableVector<T>::Len();

	while(Low < High)
	{
		int Mid = (Low + High) >> 1;
		if((*this)[Mid] < _Elem)
			Low = Mid + 1;
		else
			High = Mid;
	}
	if((*this)[Low] == _Elem)
		return Low;
	else
		return -1;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TList_Linked
| Denna verka vara "referens-säker". Alltså, en lista med CStr'ar fungerar.
|__________________________________________________________________________________________________
\*************************************************************************************************/

template <class T>
class TList_Linked : public CObj
{
	struct entry 
	{
		entry* prev;
		entry* next;
		T element;

		entry()
		{
			prev = NULL;
			next = NULL;
		};
		
		entry(const T &e)
		{
			prev = NULL;
			next = NULL;
			element = e;
		};
	};

	struct TLL_List
	{
		MRTC_CriticalSection m_Lock;
		int32 nentrys;
		entry* first;
		entry* last;
		int32 refcnt;
//		int refcnt;

		TLL_List()
		{
			nentrys = 0;
			first = NULL;
			last = NULL;
			refcnt = 1;
		};
	};

	TLL_List* p;

public:	
	TList_Linked()
	{
		p = DNew(TLL_List) TLL_List;
		if (p == NULL) MemError("-");
	};

	~TList_Linked()
	{
		Destroy();
	};

	//JK-NOTE: 2005-07-04 - Does this operator overload work as it is intended? the refcount and assignment things look horribly broken, possible memoryleak?
	TList_Linked& operator= (const TList_Linked& list)
	{
		// Increase refcnt first so 'a = a' assignments works
		MRTC_SystemInfo::Atomic_Increase(&list.p->refcnt);
		Destroy();

		M_LOCK(list.p->m_Lock);
		*this = list;
		return *this;
	};

	void Destroy()
	{
		// If this is the last reference to listdata then delete everything
		if((MRTC_SystemInfo::Atomic_Decrease(&p->refcnt) - 1) == 0)
		{
			Clear();
			delete p;
		}
		p = NULL;
	}

	void Clear()
	{
		M_LOCK(p->m_Lock);
		entry* e = p->first;
		entry* next;

		while (e != NULL)
		{
			next = e->next;
			delete e;
			e = next;
		};
		p->nentrys = 0;

		p->first = NULL;
		p->last = NULL;
	};

	int Len() const
	{
		return p->nentrys;
	};

	int Add(const T& elem)
	{
		M_LOCK(p->m_Lock);
		entry* e = DNew(entry) entry(elem);
		if (e == NULL) MemError("Add");
		if (p->last == NULL) {
			p->first = e;
			p->last = e;
		} 
		else
		{
			p->last->next = e;
			e->prev = p->last;
			p->last = e;
		};
		p->nentrys++;
		return (p->nentrys - 1);
	};

	void Insert(int pos, const T& elem)
	{
		M_LOCK(p->m_Lock);
		if ((pos < 0) || (pos >= p->nentrys))
			Error("Insert", CStrF("Invalid position: %d/%d", pos, p->nentrys));
		entry* e = p->first;
		for (int i = 0; i < pos; e = e->next) i++;
		
		if (e == NULL) {
			Add(elem);
		} 
		else
		{
			entry* newe = DNew(entry) entry(elem);
			if (newe == NULL) MemError("Insert");
			newe->next = e;
			newe->prev = e->prev;
			e->prev = newe;
			if (newe->prev != NULL) 
				newe->prev->next = newe;
			else 
				p->first = newe;

			if (newe->next != NULL) 
				newe->next->prev = newe;
			else
				p->last = newe;
			p->nentrys++;
		};
	};

	void Del(int pos)
	{
		M_LOCK(p->m_Lock);
		if ((pos < 0) || (pos >= p->nentrys))
			Error("Del", CStrF("Invalid position: %d/%d", pos, p->nentrys));
		entry* e = p->first;
		for (int i = 0; i < pos; e = e->next) i++;

		if (e->prev != NULL) 
			e->prev->next = e->next;
		else
			p->first = e->next;

		if (e->next != NULL) 
			e->next->prev = e->prev;
		else
			p->last = e->prev;

		delete e;
		e = NULL;
		p->nentrys--;
	};

	T& operator[] (int pos) 
	{
		M_LOCK(p->m_Lock);
		if ((pos < 0) || (pos >= p->nentrys))
			Error("[]", CStrF("Invalid position: %d/%d", pos, p->nentrys));
		entry* e = p->first;
		for (int i = 0; i < pos; e = e->next) i++;

		return(e->element);
	};

	const T& operator[] (int pos) const
	{
		M_LOCK(p->m_Lock);
		if ((pos < 0) || (pos >= p->nentrys))
			Error("[]", CStrF("Invalid position: %d/%d", pos, p->nentrys));
		entry* e = p->first;
		for (int i = 0; i < pos; e = e->next) i++;

		return(e->element);
	};

	void Sort(int order, int method)
	{
	};

	int Operate(int f(int nr, T& elem, void* AppData), void* AppData)
	{
		entry* e = p->first;
		int ret;
		for (int i = 0; i < p->nentrys; i++)
		{
			ret = f(i, e->element, AppData);
			if (ret != 0) return(ret);
			e = e->next;
		};
		return(0);
	};

};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TQueue
|__________________________________________________________________________________________________
\*************************************************************************************************/

template<class T>
class TQueue : public CReferenceCount
{
protected:
	TList_Vector<T> m_lBuffer;
	int m_iHead;
	int m_iTail;

	inline void IncPtr(int &ptr)
	{
		++ptr %= m_lBuffer.Len();
	};

public:
	void Create(int _nElem)
	{
		m_iHead = 0;
		m_iTail = 0;
		m_lBuffer.SetLen(_nElem);
	}

	TQueue()
	{
		m_iHead = 0;
		m_iTail = 0;
	}

	TQueue(int _nElem)
	{
		Create(_nElem);
	}

	int MaxPut() const
	{
		if (m_iHead == m_iTail)
			return m_lBuffer.Len()-1;
		else
			return (2*m_lBuffer.Len() - 1 - (m_iHead - m_iTail)) % m_lBuffer.Len();
	}

	bool Empty() const
	{
		return (m_iHead == m_iTail);
	}

	int GetTailIndex() const { return m_iTail; };
	int GetHeadIndex() const { return m_iHead; };

	T* GetTail()
	{
		if (Empty()) return NULL;
		return &m_lBuffer[m_iTail];
	}
	
	T* GetHead()
	{
		if (Empty()) return NULL;
		return &m_lBuffer[(m_iHead-1+m_lBuffer.Len()) % m_lBuffer.Len()];
	}
	
	T* GetNext(T* pE)
	{
		// RA: "en pekare är alltid en pekare!"
		int iE = int((uint8*)pE - (uint8*)m_lBuffer.GetBasePtr()) / sizeof(T);
		IncPtr(iE);
		if (iE == m_iHead) return NULL;
		return &m_lBuffer[iE];
	}
	
	T* GetPrev(T* pE)
	{
		int iE = int((uint8*)pE - (uint8*)m_lBuffer.GetBasePtr()) / sizeof(T);
		if (iE == m_iTail) return NULL;
		iE = (iE-1+m_lBuffer.Len()) % m_lBuffer.Len();
		return &m_lBuffer[iE];
	}
	
	bool Put(const T& x)
	{
		if (MaxPut() < 1) return false;
		m_lBuffer[m_iHead] = x;
		IncPtr(m_iHead);
		return true;
	}
		
	const T& Peek()
	{
		if (Empty()) Error("Peek", "Queue is empty.");
		return m_lBuffer[m_iTail];
	}

	const T& Get()
	{
		if (Empty()) Error("Get", "Queue is empty.");

		int iOldTail = m_iTail;
		IncPtr(m_iTail);
		return m_lBuffer[iOldTail];
	}

	const T& GetElem(int _iE)
	{
		return m_lBuffer[_iE];
	}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CStr_List_Vector
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CStr_List_Vector : public TList_Vector<CStr>
{
public:
	void AddSeparatedString(CStr s, const CStr& separator)
	{
		while (s != (const char*) "")
			Add(s.Separate(separator));
	};
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	TemplateClass:		A template for a general tree with n children.
						
	Parameters:			
		T:				Hmmm
		NR_CHILDREN:	Hmm Hmm Hmmmm
						
	Comments:			
						Standard tree node.

						The SetChild and SetParent functions should be used with care.
						The are only needed in special cases.

\*____________________________________________________________________*/

template <class T, int NR_CHILDREN>
class TTNode 
{

	T Element;

	uint8 NrChildren;
	TTNode* Children[NR_CHILDREN];

	TTNode* Parent;

	// This one must be overriden if you want to
	// inherit from this class.
	dllvirtual TTNode* NewNode(const T& Element)
	{
		return DNew(TTNode) TTNode(Element,this);
	}

public:

	TTNode(const T Elem, TTNode* Parent)
	{
		Element=Elem;
		TTNode::Parent=Parent;

		NrChildren=0;
		for (uint8 i=0; i<NR_CHILDREN; i++)
			Children[i]=NULL;
	}

	virtual ~TTNode()
	{
		for (uint8 i=0; i<NR_CHILDREN; i++)
			if (Children[i]!=NULL) delete Children[i];
	}

	T& GetElement() { return Element; };
	void SetElement(const T Elem) { Element=Elem; };

	uint8 GetNrChildren() const { return NrChildren; };

	void AddChild(uint8 Index, const T& Element)
	{
		if (Children[Index]!=NULL) Error_static("TTNode::AddChild","Child already exists.");
		Children[Index]=NewNode(Element);
		if (Children[Index]==NULL) Error_static("TTNode::AddChild","Out of memory.");
		NrChildren++;
	}

	TTNode* GetChild(uint8 Index) const { return Children[Index]; };
	void SetChild(uint8 Index,TTNode* Child)
	{ 
		Children[Index]=Child;
		if (Child!=NULL) Child->SetParent(this);
	}

	TTNode* GetParent() const { return Parent; };
	void SetParent(TTNode* Par) { Parent=Par; };

	void DeleteSubTree(uint8 Index)
	{
		if (Children[Index]==NULL) Error_static("TTNode::DeleteSubTree","Child doesn't exist.");
		delete Children[Index];
		Children[Index]=NULL;
		NrChildren--;
	}

	void AttachSubTree(uint8 Index, TTNode& SubTree)
	{
		if (Children[Index]!=NULL) Error_static("TTNode::AttachSubTree","Child already exists.");
		Children[Index]=&SubTree;
		if (Children[Index]==NULL) Error_static("TTNode::AttachSubTree","Out of memory.");
		NrChildren++;
		SubTree.SetParent(this);
	}

	/*
	// Here's a slow general GetHeight procedure.
	uint32 _GetHeight(TTNode& Node, uint16 RelativeLevel) const
	{
		uint32 Height=0;

		for (uint8 i=0; i<NR_CHILDREN; i++)
			if (Node.GetChild(i)!=NULL)
				Height=Max(Height,_GetHeight(*Node.GetChild(i),RelativeLevel+1));

		// Have we found a leaf?
		if (Height==0)
			return RelativeLevel;
		else
			return Height;

	}

	uint32 GetHeight() { return _GetHeight(*this,0); };

	// Here's a slow general GetLevel procedure.
	virtual uint32 GetLevel() const
	{
		uint32 Level=0;
		TTNode* N=Parent;
		while (N!=NULL) {
			N=N->GetParent();
			Level++;
		}

		return Level;
	}

	// Returns the number of nodes in the Node:s subtree + itself.
	virtual uint32 _GetOrder(TTNode& Node) const
	{
		uint32 Order=1;

		for (uint8 i=0; i<NR_CHILDREN; i++)
			if (Node.GetChild(i)!=NULL)
				Order+=_GetOrder(*Node.GetChild(i));

		return Order;
	}

	uint32 GetOrder() { return _GetOrder(*this); };*/
};

// Tree traversion orders.
enum eTreeOrder {PREORDER,POSTORDER,INORDER,REVERSE_INORDER,USER_DEFINED};

// Abstract tree class.
template <class T, class TREE_NODE, int NR_CHILDREN>
class TTree : public CReferenceCount {
protected:

	TREE_NODE* Tree;

// Overridables.

	// Insert must be implemented by the user.
	virtual TREE_NODE& _Insert(TREE_NODE& Node, const T& Element) pure;

	// Here's a slow general Find procedure that works for all trees.
	virtual TREE_NODE* _Find(TREE_NODE& Node, const T& Elem) const
	{
		
		if (Node.GetElement()==Elem)
			return &Node;
		else {
			TREE_NODE* Node2=NULL;
			for (uint8 i=0; i<NR_CHILDREN; i++)
				if (Node.GetChild(i)!=NULL) {
					Node2=_Find(*(TREE_NODE*)Node.GetChild(i),Elem);
					if (Node2!=NULL) break;
				}
			
			return Node2;
		}

	}

	// Use this function if you want to traverse the tree in some
	// non-standard way.
	virtual void _MakeList(TREE_NODE& Node, TList_Linked<T>& List) const {};

public:

// Overridables

	TTree() { Tree=NULL; };
	virtual ~TTree() { if (Tree!=NULL) delete Tree; };

	virtual void Clear() { if (Tree!=NULL) delete Tree; Tree=NULL; };

	TREE_NODE* GetRoot() const { return Tree; };

/*	uint32 GetOrder() const { if (Tree!=NULL) return Tree->GetOrder(); else return 0; };

	uint32 GetHeight() const { if (Tree!=NULL) return Tree->GetHeight(); else return 0; };*/

	TREE_NODE& Insert(const T& Element)
	{ 
		if (Tree==NULL) {
			Tree=DNew(TREE_NODE) TREE_NODE(Element,NULL);
			if (Tree==NULL) MemError("Insert");
			return *Tree;
		}
		else
			return _Insert(*Tree,Element);
	}

	TREE_NODE& Insert(const T* Element)
	{
		if (Tree==NULL) {
			Tree=DNew(TREE_NODE) TREE_NODE(*Element,NULL);
			if (Tree==NULL) MemError("Insert");
			return *Tree;
		}
		else
			return _Insert(*Tree,*Element);
	}

	void DeleteSubTree(TREE_NODE* Node)
	{
		if (Node==NULL) Error("DeleteSubTree","Can't delete using a NULL pointer.");

		TREE_NODE* Parent=(TREE_NODE*)Node->GetParent();

		if (Parent==NULL) {
			delete Tree;
			Tree=NULL;
		}
		else {
			for (uint8 i=0; i<NR_CHILDREN; i++)
				if (Parent->GetChild(i)==Node) {
					Parent->DeleteSubTree(i);
					break;
				}
		}

	}

	TREE_NODE* Find(const T& Elem) const
	{
		if (Tree==NULL) return NULL;
		return _Find(*Tree,Elem);
	}

	TREE_NODE* FindInSubtree(TREE_NODE* Node, const T& Elem) const
	{
		if (Node==NULL) return NULL;
		return _Find(*Node,Elem);
	}

	// This procedure traverses the tree in a certain order and
	// converts it to a list.
	void MakeList(TREE_NODE& Node, TList_Linked<T>& List,eTreeOrder Order) const
	{
		switch (Order) {
		case PREORDER: {
			List.Add(Node.GetElement());
			for (aint i=0; i<NR_CHILDREN; i++)
				if (Node.GetChild(i)!=NULL)
					MakeList(*(TREE_NODE*)Node.GetChild(i),List,Order);
			break;
		}

		case POSTORDER: {
			for (aint i=0; i<NR_CHILDREN; i++)
				if (Node.GetChild(i)!=NULL)
					MakeList(*(TREE_NODE*)Node.GetChild(i),List,Order);
			List.Add(Node.GetElement());
			break;
		}

		case INORDER: {
			if ((NR_CHILDREN & 1)==1) Error("MakeList","INORDER not defined for an uneven amount of children.");
			aint i;
			for (i=0; i<(NR_CHILDREN >> 1); i++)
				if (Node.GetChild(i)!=NULL)
					MakeList(*(TREE_NODE*)Node.GetChild(i),List,Order);

			List.Add(Node.GetElement());

			for (i=(NR_CHILDREN >> 1); i<NR_CHILDREN; i++)
				if (Node.GetChild(i)!=NULL)
					MakeList(*(TREE_NODE*)Node.GetChild(i),List,Order);
			break;
		}

		case REVERSE_INORDER: {
			if ((NR_CHILDREN & 1)==1) Error("MakeList","REVERSE_INORDER not defined for an uneven amount of children.");
			aint i;
			for (i=NR_CHILDREN-1; i>=(NR_CHILDREN >> 1); i--)
				if (Node.GetChild(i)!=NULL)
					MakeList(*(TREE_NODE*)Node.GetChild(i),List,Order);

			List.Add(Node.GetElement());

			for (i=(NR_CHILDREN >> 1)-1; i>=0; i--)
				if (Node.GetChild(i)!=NULL)
					MakeList(*(TREE_NODE*)Node.GetChild(i),List,Order);
			break;
		}

		case USER_DEFINED: {
			_MakeList(Node,List);
			break;
		}
		} // End switch

	}

	void MakeList(TList_Linked<T>& List,eTreeOrder Order) const
	{
		if (Tree!=NULL) {
			List.Clear();
			MakeList(*Tree,List,Order);
		}
	}

};

#define STANDARD_NODE(T,NR_CHILDREN)	TTNode<T,NR_CHILDREN>
#define STANDARD_TREE(T,NR_CHILDREN) \
	TTree<T,STANDARD_NODE(T,NR_CHILDREN),NR_CHILDREN>



/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	TemplateClass:		A template for a general tree with variable amount of children.
						
	Parameters:			
		T:				description
						
	Comments:			
						CLASS AUTHOR:	Daniel Hansson
						STARTED     :	961221
						LAST UPDATED:	970211
\*____________________________________________________________________*/

// Varible amount of children tree node.
template <class T>
class TTNode_Var {

	T Element;

	typedef TTNode_Var<T> __TN_V;
	TList_Vector<__TN_V*> Children;

	TTNode_Var* Parent;

	// This one must be overriden if you want to
	// inherit from this class.
	dllvirtual __TN_V* NewNode(const T& Element)
	{
		return DNew(TTNode_Var) TTNode_Var(Element,this);
	}

public:

	TTNode_Var(const T Elem, __TN_V* Parent)
	{
		Children.SetGrow(1);
		Element=Elem;
		TTNode_Var::Parent=Parent;
	}

	dllvirtual ~TTNode_Var()
	{
		if (Children.Len()>0) {
			for (uint16 i=0; i<Children.Len(); i++)
				if (Children[i]!=NULL) delete Children[i];
		}
	}

	T& GetElement() { return Element; };
	void SetElement(const T Elem) { Element=Elem; };

	uint16 GetNrChildren() const { return Children.Len(); };

	uint16 AddChild(const T& Element)
	{
		uint16 Index=Children.Add(NewNode(Element));
		if (Children[Index]==NULL) Error_static("TTNode_Var::AddChild","Out of memory.");
		return Index;
	}

	TList_Vector<__TN_V*>& GetChildren() { return Children; };
	__TN_V* GetChild(uint16 Index) const { return Children[Index]; };
	void SetChild(uint16 Index,__TN_V* Child)
	{
		Children[Index]=Child;
		if (Child!=NULL) Child->SetParent(this);
	}

	__TN_V* GetParent() const { return Parent; };
	void SetParent(__TN_V* Par) { Parent=Par; };

	void DeleteSubTree(uint16 Index)
	{
		if (Index>=Children.Len()) Error_static("TTNode_Var::DeleteSubTree","Child doesn't exist.");
		delete Children[Index];
		Children.Del(Index);
	}

	uint16 AttachSubTree(__TN_V& SubTree)
	{
		uint16 Index=Children.Add(&SubTree);
		if (Children[Index]==NULL) Error_static("TTNode_Var::AttachSubTree","Out of memory.");
		SubTree.SetParent(this);
		return Index;
	}

	/*
	// Here's a slow general GetHeight procedure.
	uint32 _GetHeight(__TN_V& Node, uint32 RelativeLevel) const
	{
		uint32 Height=0;

		for (uint16 i=0; i<Node.GetNrChildren(); i++)
			Height=Max(Height,_GetHeight(*Node.GetChild(i),RelativeLevel+1));

		// Have we found a leaf?
		if (Height==0)
			return RelativeLevel;
		else
			return Height;

	}

	uint32 GetHeight() { return _GetHeight(*this,0); };

	// Here's a slow general GetLevel procedure.
	virtual uint32 GetLevel() const
	{
		uint32 Level=0;
		__TN_V* N=Parent;
		while (N!=NULL) {
			N=N->GetParent();
			Level++;
		}

		return Level;
	}

	// Returns the number of nodes in the Node:s subtree + itself.
	virtual uint32 _GetOrder(__TN_V& Node) const
	{
		uint32 Order=1;
		
		for (uint16 i=0; i<Node.GetNrChildren(); i++)
			Order+=_GetOrder(*Node.GetChild(i));

		return Order;
	}

	uint32 GetOrder() { return _GetOrder(*this); };*/
};

// Abstract tree class.
template <class T, class TREE_NODE, int cp>
class TTree_Var : public CReferenceCount {
protected:

	TREE_NODE* Tree;

// Overridables.

	// Insert must be implemented by the user.
	virtual TREE_NODE& _Insert(TREE_NODE& Node, const T& Element) pure;

	// Here's a slow general Find procedure that works for all trees.
	virtual TREE_NODE* _Find(TREE_NODE& Node, const T& Elem) const
	{
		
		if (Node.GetElement()==Elem)
			return &Node;
		else {
			TREE_NODE* Node2=NULL;
			for (uint16 i=0; i<Node.GetNrChildren(); i++) {
				Node2=_Find(*(TREE_NODE*)Node.GetChild(i),Elem);
				if (Node2!=NULL) break;
			}
			
			return Node2;
		}

	}

	// Use this function if you want to traverse the tree in some
	// non-standard way.
	virtual void _MakeList(TREE_NODE& Node, TList_Linked<T>& List) const {};

public:

// Overridables

	TTree_Var() { Tree=NULL; };
	virtual ~TTree_Var() { if (Tree!=NULL) delete Tree; };

	dllvirtual void Clear() { if (Tree!=NULL) delete Tree; Tree=NULL; };

	TREE_NODE* GetRoot() const { return Tree; };

/*	uint32 GetOrder() const { if (Tree!=NULL) return Tree->GetOrder(); else return 0; };

	uint32 GetHeight() const { if (Tree!=NULL) return Tree->GetHeight(); else return 0; };
*/
	TREE_NODE& Insert(const T& Element)
	{ 
		if (Tree==NULL) {
			Tree=DNew(TREE_NODE) TREE_NODE(Element,NULL);
			if (Tree==NULL) MemError("Insert");
			return *Tree;
		}
		else
			return _Insert(*Tree,Element);
	}

	TREE_NODE& Insert(const T* Element)
	{
		if (Tree==NULL) {
			Tree=DNew(TREE_NODE) TREE_NODE(*Element,NULL);
			if (Tree==NULL) MemError("Insert");
			return *Tree;
		}
		else
			return _Insert(*Tree,*Element);
	}

	void DeleteSubTree(TREE_NODE* Node)
	{
		if (Node==NULL) Error("DeleteSubTree","Can't delete using a NULL pointer.");

		TREE_NODE* Parent=(TREE_NODE*)Node->GetParent();

		if (Parent==NULL) {
			delete Tree;
			Tree=NULL;
		}
		else {
			for (uint16 i=0; i<Node->GetNrChildren(); i++)
				if (Parent->GetChild(i)==Node) {
					Parent->DeleteSubTree(i);
					break;
				}
		}

	}

	TREE_NODE* Find(const T& Elem) const
	{
		if (Tree==NULL) return NULL;
		return _Find(*Tree,Elem);
	}

	TREE_NODE* FindInSubtree(TREE_NODE* Node, const T& Elem) const
	{
		if (Node==NULL) return NULL;
		return _Find(*Node,Elem);
	}

	// This procedure traverses the tree in a certain order and
	// converts it to a list.
	void MakeList(TREE_NODE& Node, TList_Linked<T>& List,eTreeOrder Order) const
	{
		switch (Order) {
		case PREORDER: {
			List.Add(Node.GetElement());
			for (aint i=0; i<Node.GetNrChildren(); i++)
				MakeList(*(TREE_NODE*)Node.GetChild(i),List,Order);
			break;
		}

		case POSTORDER: {
			for (aint i=0; i<Node.GetNrChildren(); i++)
				MakeList(*(TREE_NODE*)Node.GetChild(i),List,Order);
			List.Add(Node.GetElement());
			break;
		}

		case INORDER: {
			if ((Node.GetNrChildren() & 1)==1) Error("MakeList","INORDER not defined for an uneven amount of children.");
			aint i;
			for (i=0; i<(Node.GetNrChildren() >> 1); i++)
				MakeList(*(TREE_NODE*)Node.GetChild(i),List,Order);

			List.Add(Node.GetElement());

			for (i=(Node.GetNrChildren() >> 1); i<Node.GetNrChildren(); i++)
				MakeList(*(TREE_NODE*)Node.GetChild(i),List,Order);
			break;
		}

		case REVERSE_INORDER: {
			if ((Node.GetNrChildren() & 1)==1) Error("MakeList","REVERSE_INORDER not defined for an uneven amount of children.");
			aint i;
			for (i=Node.GetNrChildren()-1; i>=(Node.GetNrChildren() >> 1); i--)
				MakeList(*(TREE_NODE*)Node.GetChild(i),List,Order);

			List.Add(Node.GetElement());

			for (i=(Node.GetNrChildren() >> 1)-1; i>=0; i--)
				MakeList(*(TREE_NODE*)Node.GetChild(i),List,Order);
			break;
		}

		case USER_DEFINED: {
			_MakeList(Node,List);
			break;
		}
		} // End switch

	}

	void MakeList(TList_Linked<T>& List,eTreeOrder Order) const
	{
		if (Tree!=NULL) {
			List.Clear();
			MakeList(*Tree,List,Order);
		}
	}

};

#define STANDARD_NODE_VAR(T)	TTNode_Var<T>
#define STANDARD_TREE_VAR(T)	TTree_Var<T,STANDARD_NODE_VAR(T),0>

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Different derivates of the two abstract tree classes:
|__________________________________________________________________________________________________
\*************************************************************************************************/



/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	TemplateClass:		A template for a binary search tree with sorting capabilities.
						
	Parameters:			
		_param1:		description
						
	Comments:			
						CLASS AUTHOR:	Daniel Hansson
						STARTED	   :	961219
						LAST UPDATED:	970210	
\*____________________________________________________________________*/

#define BINARY_NODE	STANDARD_NODE(T,2)

template <class T,int cp>
class TSearchTree : public STANDARD_TREE(T,2) {
protected:

	BINARY_NODE& _Insert(BINARY_NODE& Node,const T& Elem)
	{
		if (Elem<Node.GetElement()) {

			if (Node.GetChild(0)==NULL) {
				Node.AddChild(0,Elem);
				return *Node.GetChild(0);
			}
			else
				return _Insert(*Node.GetChild(0),Elem);

		}
		else {

			if (Node.GetChild(1)==NULL) {
				Node.AddChild(1,Elem);
				return *Node.GetChild(1);
			}
			else
				return _Insert(*Node.GetChild(1),Elem);

		}
	}

	BINARY_NODE* _Find(BINARY_NODE& Node, const T& Elem) const
	{

		if (Node.GetElement()==Elem)
			return &Node;
		else {
			if (Elem<Node.GetElement())
				if (Node.GetChild(0)!=NULL)
					return _Find(*Node.GetChild(0),Elem);
				else
					return NULL;
			else
				if (Node.GetChild(1)!=NULL)
					return _Find(*Node.GetChild(1),Elem);
				else
					return NULL;
		}

	}

public:

	void GetListSortedInc(TList_Linked<T>& List) const
	{
		MakeList(List,INORDER);
	}

	void GetListSortedDec(TList_Linked<T>& List) const
	{
		MakeList(List,REVERSE_INORDER);
	}

	void SortListInc(TList_Linked<T>& List)
	{
		STANDARD_TREE(T,2)::Clear();
		for (uint16 i=0; i<List.Len(); i++)
			Insert(List[i]);

		List.Clear();

		GetListSortedInc(List);

		STANDARD_TREE(T,2)::Clear();
	}

	void SortListDec(TList_Linked<T>& List)
	{
		STANDARD_TREE(T,2)::Clear();
		for (uint16 i=0; i<List.Len(); i++)
			Insert(List[i]);

		List.Clear();

		GetListSortedDec(List);

		STANDARD_TREE(T,2)::Clear();
	}

	// Deletes a node - not a subtree
	// without changing the properties
	// of the search tree.
	void DeleteNode(BINARY_NODE& Node)
	{
		BINARY_NODE* ReplaceWith;

		if (Node.GetChild(0)==NULL)
			ReplaceWith=Node.GetChild(1);
		else if (Node.GetChild(1)==NULL)
			ReplaceWith=Node.GetChild(0);
		else if (Node.GetChild(0)->GetChild(1)==NULL) {
			ReplaceWith=Node.GetChild(0);
			ReplaceWith->SetChild(1,Node.GetChild(1));
		}
		else if (Node.GetChild(1)->GetChild(0)==NULL) {
			ReplaceWith=Node.GetChild(1);
			ReplaceWith->SetChild(0,Node.GetChild(0));
		}
		else {
			// Find the smallest element in the right subtree
			// and replace Node with it.
			ReplaceWith=Node.GetChild(1);
			while (ReplaceWith->GetChild(0)!=NULL)
				ReplaceWith=ReplaceWith->GetChild(0);

			ReplaceWith->GetParent()->SetChild(0,ReplaceWith->GetChild(1));
			ReplaceWith->SetChild(0,Node.GetChild(0));
			ReplaceWith->SetChild(1,Node.GetChild(1));
		}

		Node.SetChild(0,NULL);
		Node.SetChild(1,NULL);

		if (Node.GetParent()==NULL) {
			STANDARD_TREE(T,2)::Tree=ReplaceWith;
			if (ReplaceWith!=NULL)
				ReplaceWith->SetParent(NULL);
		}
		else {
			if (Node.GetParent()->GetChild(0)==&Node)
				Node.GetParent()->SetChild(0,ReplaceWith);
			else
				Node.GetParent()->SetChild(1,ReplaceWith);
		}

		delete &Node;
	}

};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CIDHeap
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CCFile;

class MCCDLLEXPORT CIDHeap : public CReferenceCount
{
	TList_Vector<int32> m_lIDAlloc;
	int32* m_pIDAlloc;
	int m_LenMinusOne;

	int32 m_bDynamic;
	int32 m_AllocPos;
	int32 m_MaxIDCapacity;
	int32 m_IDCapacity;
	int32 m_nAllocated;

public:
	
	DECLARE_OPERATOR_NEW
	

	CIDHeap();
	CIDHeap(int _MaxIDCapacity, bool _Dynamic = false);
	~CIDHeap();
	void Create(int _MaxIDCapacity, bool _Dynamic = false);

	int AllocID();
	int AllocIDRange(int _nIDs);
	bool ForceAllocID(int _ID);
	void FreeID(int _ID);
	int IsAllocated(int _ID);
	int MaxAvail();
	int MaxCapacity();

	void Read(CCFile* _pFile);
	void Write(CCFile* _pFile);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CKeyContainer
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CKeyContainer;
typedef TPtr<CKeyContainer> spCKeyContainer;

class MCCDLLEXPORT CKeyContainer : public CReferenceCount
{
	TList_Vector<CStr> m_lKeyNames;
	TList_Vector<CStr> m_lKeyValues;

public:
	
	DECLARE_OPERATOR_NEW
	

	CKeyContainer();
	~CKeyContainer();
	void Create(const TList_Vector<CStr>& _lKeyNames, const TList_Vector<CStr>& _lKeyValues);
	spCKeyContainer Duplicate() const;

	void operator= (const CKeyContainer& _KC);

	void AddScriptKey(const CStr _KeyString);
	int AddKey(const CStr _KeyName, const CStr _KeyValue);
	void SetKeyValue(int _iIndex, CStr _Value);
	void DeleteKey(const CStr _KeyName);
	void DeleteKey(int _iIndex);

	int GetnKeys() const;
	int GetKeyIndex(CStr _Key) const;
	CStr GetKeyName(int _iKey) const;
	CStr GetKeyValue(int _iKey) const;

	void Write(CCFile* _pFile) const;
	void Read(CCFile* _pFile);
};

typedef TPtr<CKeyContainer> spCKeyContainer;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CKeyContainerNode
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CDataFile;

class CKeyContainerNode;
typedef TPtr<CKeyContainerNode> spCKeyContainerNode;

class MCCDLLEXPORT CKeyContainerNode : public CReferenceCount
{
	TList_Vector<spCKeyContainerNode> m_lspSubKeys;
	spCKeyContainer m_spKeys;
	spCKeyContainer m_spData;

public:
	
	DECLARE_OPERATOR_NEW
	

	CKeyContainerNode();
	~CKeyContainerNode();

	spCKeyContainerNode Duplicate() const;

	// Access
	spCKeyContainer GetKeys();
	spCKeyContainer GetData();
	int GetNumChildren();
	spCKeyContainerNode GetChild(int _iChild);

	void DeleteChild(int _iChild);
	int AddChild(CKeyContainerNode *_pNode);

	// IO
	int ReadFromMemory_r(const char* _pStr, int _Size, bool _bEnterScope);
	void ReadFromScript(CCFile* _pFile, bool _bEnterScope = true);
	void ReadFromScript(CStr _Filename, bool _bEnterScope = true);

	void WriteToDataFile(CDataFile* _pDFile);
	void WriteToScript(CCFile* _pFile, int _RecurseDepth = 0, bool _bIndent = true);
	void WriteToScript(CStr _Filename);

	// Operations
	void DeleteData();
	void DeleteKeys();
	void DeleteKeys(CStr _StartingWith, CStr _Containing);

	CKeyContainerNode* FindContainer(CStr _Key, CStr _Value);

	CKeyContainerNode& operator = (CKeyContainerNode& _Other);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TObjectPool
|__________________________________________________________________________________________________
\*************************************************************************************************/
template<class T>
class TObjectPool;


template<class T>
class TObjectPoolAllocator
{
	class TObjectPool<T>* m_pPool;
	uint32 m_iObj;

public:
	TObjectPoolAllocator(class TObjectPool<T>* _pPool);
	~TObjectPoolAllocator();
	T* GetObject();
};



template<class T>
class TObjectPool : public CReferenceCount
{
	TThinArray<T> m_lObj;
	TThinArray<uint16> m_liFreeObjs;
	uint16 m_nFreeObj;
	MRTC_CriticalSection m_Lock;

protected:
	uint32 Alloc()
	{
		M_LOCK(m_Lock);
		if (!m_nFreeObj)
		{
#ifdef M_Profile
			CMTime Time = CMTime::GetCPU();
#endif

			while(!m_nFreeObj)
			{
				M_UNLOCK(m_Lock);
				MRTC_SystemInfo::OS_Sleep(1);

#ifdef M_Profile
				CMTime Time2 = CMTime::GetCPU();
				Time2 -= Time;

				if (Time2.GetTime() > 1.0f)
				{
					M_TRACEALWAYS("(TObjectPool::Alloc) Potential deadlock\n");
					Time += CMTime::CreateFromSeconds(1.0f);
				}
#endif
			}
		}
		
		return m_liFreeObjs[--m_nFreeObj];
	}
	
	void Free(uint32 _iObj)
	{
		M_LOCK(m_Lock);
		m_liFreeObjs[m_nFreeObj++] = _iObj;
	}
	
	T* GetObject(uint32 _iObj)
	{
		return &m_lObj[_iObj];
	}

public:
	TObjectPool()
	{
		m_nFreeObj = 0;
	}
	
	void Create(int _nObj)
	{
		M_LOCK(m_Lock);
		if (m_liFreeObjs.Len() != m_nFreeObj)
			Error("Create", "Cannot recreate object while in use.");

		m_lObj.SetLen(_nObj);
		m_liFreeObjs.SetLen(_nObj);
		for(int i = 0; i < _nObj; i++)
			m_liFreeObjs[i] = i;
		m_nFreeObj = _nObj;
	}

	TThinArray<T>& SyncronousGetArray()
	{
		return m_lObj;
	}
	
	friend class TObjectPoolAllocator<T>;
};

template<class T>
TObjectPoolAllocator<T>::TObjectPoolAllocator(class TObjectPool<T>* _pPool)
{
	m_pPool = _pPool;
	m_iObj = m_pPool->Alloc();
}

template<class T>
TObjectPoolAllocator<T>::~TObjectPoolAllocator()
{
	m_pPool->Free(m_iObj);
}

template<class T>
T* TObjectPoolAllocator<T>::GetObject()
{
	return m_pPool->GetObject(m_iObj);
}



template<int _Size> class intsize { public: };
template<> class intsize<8>  { public: typedef uint8  Type; };
template<> class intsize<16> { public: typedef uint16 Type; };
template<> class intsize<32> { public: typedef uint32 Type; };
template<> class intsize<64> { public: typedef uint64 Type; };

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TStaticArray
|__________________________________________________________________________________________________
\*************************************************************************************************/
template <class T, int MaxLen>
class TStaticArray
{
protected:
	T m_Data[MaxLen];
	int m_Len;

public:
	enum { NumBits = (DNumBits(MaxLen) + 7) & ~7 };
	typedef typename intsize<NumBits>::Type LenType;

	TStaticArray()
	{
		m_Len = 0;
	}

	TStaticArray(const TStaticArray &_Source)
	{
		*this = _Source;
	}

	const TStaticArray& operator=(const TStaticArray &_Source)
	{
		m_Len = _Source.m_Len;
		for(int i = 0; i < m_Len; i++)
			m_Data[i] = _Source.m_Data[i];
		return *this;
	}

	void Clear()								// Set length to zero.
	{
		SetLen(0);
	}

	void SetLen(uint _Len)						// Set array length.
	{
		if (_Len > MaxLen)
			Error_static("TStaticArray::SetLen", "Too many elements");
		m_Len = _Len;
	}

	int Len() const								// Returns length of array.
	{
		return m_Len;
	}

	int GetMax() const							// Returns maximum length of array
	{
		return MaxLen;
	}

	bool ValidPos(int _Pos) const				// Returns true if _Pos is a valid index.
	{
		return (_Pos >= 0 && _Pos < m_Len);
	}

	T* GetBasePtr()								// Returns pointer to element 0
	{
		return m_Data;
	}

	const T* GetBasePtr() const					// Returns pointer to element 0
	{
		return m_Data;
	}

	T& operator[](int _Index)					// Return reference to element x
	{
#ifdef M_BOUNDCHECK
		if (!ValidPos(_Index))
			Error_static("TThinArray::operator[]", CStrF("Index out of range. %d/%d", _Index, Len()));
#endif
		return m_Data[_Index];
	}

	const T& operator[](int _Index) const		// Return const reference to element x
	{
#ifdef M_BOUNDCHECK
		if (!ValidPos(_Index))
			Error_static("TThinArray::operator[]", CStrF("Index out of range. %d/%d", _Index, Len()));
#endif
		return m_Data[_Index];
	}

	int Add(const T& _Elem)
	{
		if(m_Len < GetMax())
		{
			m_Data[m_Len] = _Elem;
			return m_Len++;
		}
		else
			Error_static("TStaticArray::Add", "Reached maximum length");
		return 0;
	}

	int Add(const T* _pElem, int _nElem)
	{
		if(m_Len + _nElem >= GetMax())
			Error_static("TStaticArray::Add", "Reached maximum length");

		for(int i = 0; i < _nElem; i++)
			m_Data[m_Len++] = _pElem[i];
		m_Len += _nElem;
		return m_Len - _nElem;
	}

	void Del(int _Index)
	{
#ifdef M_BOUNDCHECK
		if(!ValidPos(_Index))
			Error_static("TStaticArray::Del", "Range check error");
#endif

		for(int i = _Index; i < m_Len - 1; i++)
			m_Data[i] = m_Data[i + 1];
		m_Len--;
	}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| TBitArray
|
| Static array of bits. Smallest possible Elemsize is automatically chosen..
|__________________________________________________________________________________________________
\*************************************************************************************************/
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

	void Clear()
	{
		for (uint i = 0; i < BufferLen; i++)
			m_Buffer[i] = 0;
	}

#define DBitArrayCheckIndex(i) M_ASSERT(((i) >> ElemShift) < BufferLen, "TBitArray: Index out of range!");

	uint8 Get(uint _iBit) const
	{
		DBitArrayCheckIndex(_iBit);
		return (m_Buffer[_iBit >> ElemShift] >> (_iBit & ElemMask)) & 1;
	}

	void Set1(uint _iBit)
	{
		DBitArrayCheckIndex(_iBit);
		m_Buffer[_iBit >> ElemShift] |= (ElemType(1) << (_iBit & ElemMask));
	}

	void Set0(uint _iBit)
	{
		DBitArrayCheckIndex(_iBit);
		m_Buffer[_iBit >> ElemShift] &= ~(ElemType(1) << (_iBit & ElemMask));
	}

	void Set(uint _iBit, uint8 _Value)
	{
		DBitArrayCheckIndex(_iBit);
		m_Buffer[_iBit >> ElemShift] = 
			(m_Buffer[_iBit >> ElemShift] & ~(ElemType(1) << (_iBit & ElemMask))) | (ElemType(_Value) << (_iBit & ElemMask));
	}
};


// -------------------------------------------------------------------
#endif	// _INC_MOS_DA






