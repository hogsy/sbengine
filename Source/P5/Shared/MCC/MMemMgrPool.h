
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/

#ifndef __INC_MMEMMGRPOOL
#define __INC_MMEMMGRPOOL

#include "MDA.h"

// -------------------------------------------------------------------
//  TStaticHeap
// -------------------------------------------------------------------
template<class T>
class TStaticHeap
{
protected:
	TArray<T> m_lElem;
	TArray<int> m_liElemFree;
	int* m_piElemFree;
	int m_iNextFree;

public:
	TStaticHeap();
	void Create(int _MaxElem);		// Create/Clear heap.
	void Clear();					// Clear

	bool IsFull();

	T* New();
	void Delete(T*);
};

// -------------------------------------------------------------------
//  TStaticHeap, Implementation
// -------------------------------------------------------------------
template<class T>
TStaticHeap<T>::TStaticHeap()
{
	m_piElemFree = NULL;
	m_iNextFree = 0;
}

template<class T>
void TStaticHeap<T>::Create(int _MaxElem)
{
	m_lElem.QuickSetLen(_MaxElem);
	m_liElemFree.QuickSetLen(_MaxElem);
	m_piElemFree = m_liElemFree.GetBasePtr();
	Clear();
}

template<class T>
void TStaticHeap<T>::Clear()
{
	int nElem = m_lElem.Len();
	m_iNextFree = 0;
	for(int i = 0; i < nElem; i++)
		m_piElemFree[i] = i;
}

template<class T>
bool TStaticHeap<T>::IsFull()
{
	return (m_iNextFree >= m_lElem.Len()) ? true : false;
}

template<class T>
T* TStaticHeap<T>::New()
{
	if (IsFull()) return NULL;
//ConOutL(CStrF("TStaticHeap<T>::New, Base %.8x, iNext %d, iElem %d", m_lElem.GetBasePtr(), m_iNextFree, m_piElemFree[m_iNextFree]));
	int iElem = m_piElemFree[m_iNextFree++];
	return &m_lElem.GetBasePtr()[iElem];
//	return new ((void*)&m_lElem.GetBasePtr()[m_piElemFree[m_iNextFree++]])T;	// Placement new. APA!, DET FUNKAR INTE! GRRR
}

template<class T>
void TStaticHeap<T>::Delete(T* _pElem)
{
	//	if (!m_iNextFree) Error("Delete", "Nothing is allocated.");
	if (!m_iNextFree) return;	// Silently drop the error.

//	_pElem->~T();	// Destruct
	int Index = (_pElem - m_lElem.GetBasePtr());
	if (Index < 0 || Index >= m_lElem.Len())
		Error_static("TStaticHeap::Delete", "Invalid pointer.");
	m_iNextFree--;
	m_piElemFree[m_iNextFree] = Index;
}

/*-----------------------------------------------
     File name    : DA__Pool.h
     Author       : 
  
     Description  : 
  -----------------------------------------------*/

class MCCDLLEXPORT CDA_Poolable;

#ifdef MRTC_MULTITHREADED

#define MDA_PoolLock(x) M_LOCK(x->m_Lock)

#define MDA_PoolParamList 0, NULL, NULL, 0

#else

#define MDA_PoolLock(x)

#define MDA_PoolParamList 0, NULL, NULL,0 

#endif

#define MDA_StaticPool(Name, ElementSize, AllocationSize) CDA_Pool_Static Name = {ElementSize, AllocationSize, MDA_PoolParamList};

class MCCDLLEXPORT CDA_Pool_Static
{
public:
	class CElement
	{
	public:
		class CElement *Next;
	};

	class CAllocation
	{
	public:
		class CAllocation *Next;
#ifdef CPU_QWORD_ALIGNMENT
		uint8 _paddata[4];
#endif
	};


	uint32 m_ElementSize;
	uint32 m_AllocationSize;
	int m_UsedElements;

	CElement *m_pFirstFreeElement;
	CAllocation *m_pFirstAllocation;
	int m_NumAllocations;

#ifdef MRTC_MULTITHREADED
	NThread::CMutualAggregate m_Lock;
#endif

	void Init(uint32 _ElementSize, uint32 _AllocationSize);
	
	void FlushUnused();

	void *New();
	void Delete(void *_pToDelete);
	
};

class MCCDLLEXPORT CDA_Pool : public CDA_Pool_Static
{
public:


/*	CDA_Pool(uint32 _ElementSize, uint32 _AllocationSize);*/
	CDA_Pool();
#ifndef DA__GLOBAL_ALLOC
	~CDA_Pool();
#endif
	
};

template <class _tcPoolable>
class TDA_Pool : public CDA_Pool
{
public:

	TDA_Pool(int _GrowSize = 4096)
	{
		Init(sizeof(_tcPoolable), _GrowSize);
	}

	_tcPoolable *New()
	{

		return new(CDA_Pool_Static::New()) _tcPoolable;
	}

	template <typename t_CType0>
	_tcPoolable *New(t_CType0 &_Param0)
	{

		return new(CDA_Pool_Static::New()) _tcPoolable(_Param0);
	}

	template <typename t_CType0, typename t_CType1>
	_tcPoolable *New(t_CType0 & _Param0, t_CType1 & _Param1)
	{

		return new(CDA_Pool_Static::New()) _tcPoolable(_Param0, _Param1);
	}

	template <typename t_CType0, typename t_CType1, typename t_CType2>
	_tcPoolable *New(t_CType0 & _Param0, t_CType1 & _Param1, t_CType2 & _Param2)
	{

		return new(CDA_Pool_Static::New()) _tcPoolable(_Param0, _Param1, _Param2);
	}

	template <typename t_CType0, typename t_CType1, typename t_CType2, typename t_CType3>
	_tcPoolable *New(t_CType0 & _Param0, t_CType1 & _Param1, t_CType2 & _Param2, t_CType3 & _Param3)
	{

		return new(CDA_Pool_Static::New()) _tcPoolable(_Param0, _Param1, _Param2, _Param3);
	}

	template <typename t_CType0, typename t_CType1, typename t_CType2, typename t_CType3, typename t_CType4>
	_tcPoolable *New(t_CType0 & _Param0, t_CType1 & _Param1, t_CType2 & _Param2, t_CType3 & _Param3, t_CType4 & _Param4)
	{

		return new(CDA_Pool_Static::New()) _tcPoolable(_Param0, _Param1, _Param2, _Param3, _Param4);
	}


	void Delete(_tcPoolable *_pToDelete)
	{
		_pToDelete->~_tcPoolable();

		CDA_Pool_Static::Delete(_pToDelete);
	}
};

#define newP(Class, Pool) new(Pool) Class
//#define deleteP(Pointer, Pool) {void *Temp = (void *)Pointer;delete Pointer; CDA_Poolable::operator delete(Temp, Pool);}
#define deleteP(Pointer, Pool) {void *Temp = (void *)Pointer;delete Pointer; CDA_Poolable::PoolDelete(Temp, Pool);}


class MCCDLLEXPORT CDA_PoolableCompatibleDelete
{
public:
	CDA_Pool_Static *m_pPool;	

/*	void* COMPILER_ARGLISTCALL operator new(mint _Size)
	{
		// Not supported
	//	M_BREAKPOINT;
		return NULL;
	}*/

	void operator delete(void *Block);

/*	void* COMPILER_ARGLISTCALL operator new [] (mint _Size)
	{
		// Not supported
		M_BREAKPOINT;
		return NULL;
	}
	void COMPILER_ARGLISTCALL operator delete [] (void *Block)
	{
		// Not supported
		M_BREAKPOINT;
	}*/



	void* operator new(mint _Size, CDA_Pool_Static *Pool);
	void* operator new(mint _Size, CDA_Pool_Static &Pool);
	void operator delete(void * pMem, CDA_Pool_Static *Pool) { operator delete(pMem);}
	void operator delete(void * pMem, CDA_Pool_Static &Pool) { operator delete(pMem);}

};

class MCCDLLEXPORT CDA_PoolableCompatibleDeleteVirtual : public CDA_PoolableCompatibleDelete
{
public:
	~CDA_PoolableCompatibleDeleteVirtual()
	{
		M_ASSERT(m_pPool != NULL, "you have inherited in the wrong order");
	}

};


class MCCDLLEXPORT CDA_PoolableCompatibleDeleteCompatibleCRef: public CReferenceCount
{
public:
	CDA_Pool_Static *m_pPool;	

/*	void* COMPILER_ARGLISTCALL operator new(mint _Size)
	{
		// Not supported
	//	M_BREAKPOINT;
		return NULL;
	}*/

	void operator delete(void *Block);

/*	void* COMPILER_ARGLISTCALL operator new [] (mint _Size)
	{
		// Not supported
		M_BREAKPOINT;
		return NULL;
	}
	void COMPILER_ARGLISTCALL operator delete [] (void *Block)
	{
		// Not supported
		M_BREAKPOINT;
	}*/



	void* operator new(mint _Size, CDA_Pool_Static *Pool);
	void* operator new(mint _Size, CDA_Pool_Static &Pool);
	void operator delete(void * pMem, CDA_Pool_Static *Pool) { operator delete(pMem);}
	void operator delete(void * pMem, CDA_Pool_Static &Pool) { operator delete(pMem);}

	virtual void MRTC_Delete();

};


class MCCDLLEXPORT CDA_Poolable
{
public:
	

/*	void* COMPILER_ARGLISTCALL operator new(mint _Size)
	{
		// Not supported
	//	M_BREAKPOINT;
		return NULL;
	}*/

	void operator delete(void *Block)
	{

	}

/*	void* COMPILER_ARGLISTCALL operator new [] (mint _Size)
	{
		// Not supported
		M_BREAKPOINT;
		return NULL;
	}
	void COMPILER_ARGLISTCALL operator delete [] (void *Block)
	{
		// Not supported
		M_BREAKPOINT;
	}*/



	void* operator new(mint _Size, class CDA_Pool_Static *Pool);
	void* operator new(mint _Size, class CDA_Pool_Static &Pool);
	void operator delete(void * pMem, CDA_Pool_Static *Pool) { operator delete(pMem);}
	void operator delete(void * pMem, CDA_Pool_Static &Pool) { operator delete(pMem);}
	static void* PoolNew(mint _Size, class CDA_Pool_Static *Pool);
	static void* PoolNew(mint _Size, class CDA_Pool_Static &Pool);
	static void PoolDelete(void *Block, class CDA_Pool_Static *Pool);
	static void PoolDelete(void *Block, class CDA_Pool_Static &Pool);

//oid Delete(CDA_Pool *Pool);

};

class MRTC_ThreadContext
{
public:
	NThread::CMutual m_EventMember_Lock;
	TDA_Pool<NThread::CEventAutoResetReportableAggregate::CReportListMember> m_EventMember_Pool;
};

#endif // __INC_MMEMMGRPOOL
