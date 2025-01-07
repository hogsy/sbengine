
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



CDA_Pool::~CDA_Pool()
{
	{
		MDA_PoolLock(this);

		M_ASSERT(!m_UsedElements, "You must delete all elements before deleting the pool");


		while (m_pFirstAllocation)
		{
			CAllocation *pMem = m_pFirstAllocation;
			m_pFirstAllocation = m_pFirstAllocation->Next;
			MRTC_SystemInfo::OS_Free(pMem);
		}
	}
	m_Lock.Destruct();
}

CDA_Pool::CDA_Pool()
{
	m_ElementSize = 0;
	m_AllocationSize = 0;
	m_UsedElements = 0;

	m_pFirstFreeElement = 0;
	m_pFirstAllocation = 0;
	m_NumAllocations = 0;

	m_Lock.Construct();
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDA_Pool_Static
|__________________________________________________________________________________________________
\*************************************************************************************************/

#if defined(PLATFORM_XENON) || defined(PLATFORM_PS3)
enum 
{
	EPageSize = 65535
};
#else
enum
{
	EPageSize = 4095

};
#endif

void CDA_Pool_Static::Init(uint32 _ElementSize, uint32 _AllocationSize)
{
	MDA_PoolLock(this);
	
	M_ASSERT(_ElementSize >= 4, "The element must be at least 4 bytes in size");

	m_ElementSize = _ElementSize;
	m_AllocationSize = (_AllocationSize + EPageSize) & (~EPageSize);
	m_NumAllocations = 0;

//	MDA_PoolUnLock(this);
}

void *CDA_Pool_Static::New()
{
	MDA_PoolLock(this);

	if (!m_pFirstFreeElement)
	{
		m_AllocationSize = (m_AllocationSize + EPageSize) & (~EPageSize);

		// CHECKME:
		CDA_Pool_Static::CAllocation *NewAlloc = (CDA_Pool_Static::CAllocation *)MRTC_SystemInfo::OS_Alloc(m_AllocationSize, true);

		NewAlloc->Next = m_pFirstAllocation;
		m_pFirstAllocation = NewAlloc;
		++m_NumAllocations;

		int NumElements = (m_AllocationSize - sizeof(CDA_Pool_Static::CAllocation)) / m_ElementSize;
		
		CDA_Pool_Static::CElement *CurrentElement = (CDA_Pool_Static::CElement *)(NewAlloc + 1);
		
		for (int i = 0; i < NumElements; ++i)
		{
			CurrentElement->Next = m_pFirstFreeElement;
			m_pFirstFreeElement = CurrentElement;
			
			CurrentElement = (CDA_Pool_Static::CElement *)((uint8 *)CurrentElement + m_ElementSize);
		}			
//		TRACE ("Allocated %x\n", NewAlloc);
	}

	CDA_Pool_Static::CElement *NewElement = m_pFirstFreeElement;
	
	++m_UsedElements;
/*	if ((uint32)_Pool == 0x0164fb00)
	{
		M_TRACE ("_Pool(%x)->m_UsedElements %d\n", _Pool, m_UsedElements);
	}*/
	m_pFirstFreeElement = NewElement->Next;

//	MDA_PoolUnLock(this);

	return NewElement;	
}

void CDA_Pool_Static::Delete(void *_pToDelete)
{
	MDA_PoolLock(this);

	CDA_Pool_Static::CElement *DeleteElement = (CDA_Pool_Static::CElement *)_pToDelete;

	DeleteElement->Next = m_pFirstFreeElement;
	m_pFirstFreeElement = DeleteElement;

//	TRACE ("Returned %x\n", DeleteElement);

	--m_UsedElements;

/*	if ((uint32)_Pool == 0x0164fb00)
	{
		M_TRACE ("_Pool(%x)->m_UsedElements %d\n", _Pool, m_UsedElements);
	}
*/
//	MDA_PoolUnLock(this);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDA_Poolable
|__________________________________________________________________________________________________
\*************************************************************************************************/
void* CDA_Poolable::PoolNew(mint _Size, CDA_Pool_Static *_Pool)
{
	MDA_PoolLock(_Pool);

	M_ASSERT(_Size == _Pool->m_ElementSize, "Size missmatch.");

	if (!_Pool->m_pFirstFreeElement)
	{
		_Pool->m_AllocationSize = (_Pool->m_AllocationSize + EPageSize) & (~EPageSize);

		// CHECKME:
		CDA_Pool_Static::CAllocation *NewAlloc = (CDA_Pool_Static::CAllocation *)MRTC_SystemInfo::OS_Alloc(_Pool->m_AllocationSize, true);

		NewAlloc->Next = _Pool->m_pFirstAllocation;
		_Pool->m_pFirstAllocation = NewAlloc;
		++_Pool->m_NumAllocations;

		int NumElements = (_Pool->m_AllocationSize - sizeof(CDA_Pool_Static::CAllocation)) / _Pool->m_ElementSize;
		
		CDA_Pool_Static::CElement *CurrentElement = (CDA_Pool_Static::CElement *)(NewAlloc + 1);
		
		for (int i = 0; i < NumElements; ++i)
		{
			CurrentElement->Next = _Pool->m_pFirstFreeElement;
			_Pool->m_pFirstFreeElement = CurrentElement;
			
			CurrentElement = (CDA_Pool_Static::CElement *)((uint8 *)CurrentElement + _Pool->m_ElementSize);
		}			
//		TRACE ("Allocated %x\n", NewAlloc);
	}

	CDA_Pool_Static::CElement *NewElement = _Pool->m_pFirstFreeElement;
	
	++_Pool->m_UsedElements;
/*	if ((uint32)_Pool == 0x0164fb00)
	{
		M_TRACE ("_Pool(%x)->m_UsedElements %d\n", _Pool, _Pool->m_UsedElements);
	}*/
	_Pool->m_pFirstFreeElement = NewElement->Next;

//	MDA_PoolUnLock(_Pool);

	return NewElement;	
}

void* CDA_Poolable::PoolNew(mint _Size, CDA_Pool_Static &_Pool)
{
	return CDA_Poolable::PoolNew(_Size, &_Pool);
}

void* CDA_Poolable::operator new(mint _Size, class CDA_Pool_Static *Pool)
{
	return CDA_Poolable::PoolNew(_Size, Pool);
}

void* CDA_Poolable::operator new(mint _Size, class CDA_Pool_Static &Pool)
{
	return CDA_Poolable::PoolNew(_Size, &Pool);
}

void CDA_Poolable::PoolDelete(void *Block, CDA_Pool_Static &_Pool)
{
	CDA_Poolable::PoolDelete(Block, &_Pool);
}


void CDA_Poolable::PoolDelete(void *Block, CDA_Pool_Static *_Pool)
{
	MDA_PoolLock(_Pool);

	CDA_Pool_Static::CElement *DeleteElement = (CDA_Pool_Static::CElement *)Block;

	DeleteElement->Next = _Pool->m_pFirstFreeElement;
	_Pool->m_pFirstFreeElement = DeleteElement;

//	TRACE ("Returned %x\n", DeleteElement);

	--_Pool->m_UsedElements;

/*	if ((uint32)_Pool == 0x0164fb00)
	{
		M_TRACE ("_Pool(%x)->m_UsedElements %d\n", _Pool, _Pool->m_UsedElements);
	}
*/
//	MDA_PoolUnLock(_Pool);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDA_PoolableCompatibleDelete
|__________________________________________________________________________________________________
\*************************************************************************************************/


void* CDA_PoolableCompatibleDelete::operator new(mint _Size, CDA_Pool_Static *Pool)
{
	MDA_PoolLock(Pool);

	M_ASSERT(_Size == Pool->m_ElementSize, "Size missmatch.");

	if (!Pool->m_pFirstFreeElement)
	{
		Pool->m_AllocationSize = (Pool->m_AllocationSize + EPageSize) & (~EPageSize);

		// CHECKME:
		CDA_Pool_Static::CAllocation *NewAlloc = (CDA_Pool_Static::CAllocation *)MRTC_SystemInfo::OS_Alloc(Pool->m_AllocationSize, true);

		NewAlloc->Next = Pool->m_pFirstAllocation;
		Pool->m_pFirstAllocation = NewAlloc;
		++Pool->m_NumAllocations;

		int NumElements = (Pool->m_AllocationSize - sizeof(CDA_Pool_Static::CAllocation)) / Pool->m_ElementSize;
		
		CDA_Pool_Static::CElement *CurrentElement = (CDA_Pool_Static::CElement *)(NewAlloc + 1);
		
		for (int i = 0; i < NumElements; ++i)
		{
			CurrentElement->Next = Pool->m_pFirstFreeElement;
			Pool->m_pFirstFreeElement = CurrentElement;
			
			CurrentElement = (CDA_Pool_Static::CElement *)((uint8 *)CurrentElement + Pool->m_ElementSize);
		}			
//		TRACE ("Allocated %x\n", NewAlloc);
	}

	CDA_Pool_Static::CElement *NewElement = Pool->m_pFirstFreeElement;
	
	++Pool->m_UsedElements;
//	TRACE ("Pool(%x)->m_UsedElements %d\n", Pool, Pool->m_UsedElements);
	Pool->m_pFirstFreeElement = NewElement->Next;

	((CDA_PoolableCompatibleDelete *)(NewElement))->m_pPool = Pool;

//	MDA_PoolUnLock(Pool);

	return NewElement;	
}

void* CDA_PoolableCompatibleDelete::operator new(mint _Size, CDA_Pool_Static &Pool)
{
	return CDA_PoolableCompatibleDelete::operator new(_Size, &Pool);
}


void CDA_PoolableCompatibleDelete::operator delete(void *Block)
{
	CDA_Pool_Static *Pool = ((CDA_PoolableCompatibleDelete *)(Block))->m_pPool;

	MDA_PoolLock(Pool);

	CDA_Pool_Static::CElement *DeleteElement = (CDA_Pool_Static::CElement *)Block;

	DeleteElement->Next = Pool->m_pFirstFreeElement;
	Pool->m_pFirstFreeElement = DeleteElement;

//	TRACE ("Returned %x\n", DeleteElement);

	--Pool->m_UsedElements;

//	TRACE ("Pool(%x)->m_UsedElements %d\n", Pool, Pool->m_UsedElements);

//	MDA_PoolUnLock(Pool);
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDA_PoolableCompatibleDelete
|__________________________________________________________________________________________________
\*************************************************************************************************/


void* CDA_PoolableCompatibleDeleteCompatibleCRef::operator new(mint _Size, CDA_Pool_Static *Pool)
{
	MDA_PoolLock(Pool);

	M_ASSERT(_Size == Pool->m_ElementSize, "Size missmatch.");

	if (!Pool->m_pFirstFreeElement)
	{
		Pool->m_AllocationSize = (Pool->m_AllocationSize + EPageSize) & (~EPageSize);

		// CHECKME:
		CDA_Pool_Static::CAllocation *NewAlloc = (CDA_Pool_Static::CAllocation *)MRTC_SystemInfo::OS_Alloc(Pool->m_AllocationSize, true);

		NewAlloc->Next = Pool->m_pFirstAllocation;
		Pool->m_pFirstAllocation = NewAlloc;
		++Pool->m_NumAllocations;

		int NumElements = (Pool->m_AllocationSize - sizeof(CDA_Pool_Static::CAllocation)) / Pool->m_ElementSize;
		
		CDA_Pool_Static::CElement *CurrentElement = (CDA_Pool_Static::CElement *)(NewAlloc + 1);
		
		for (int i = 0; i < NumElements; ++i)
		{
			CurrentElement->Next = Pool->m_pFirstFreeElement;
			Pool->m_pFirstFreeElement = CurrentElement;
			
			CurrentElement = (CDA_Pool_Static::CElement *)((uint8 *)CurrentElement + Pool->m_ElementSize);
		}			
//		TRACE ("Allocated %x\n", NewAlloc);
	}

	CDA_Pool_Static::CElement *NewElement = Pool->m_pFirstFreeElement;
	
	++Pool->m_UsedElements;
//	TRACE ("Pool(%x)->m_UsedElements %d\n", Pool, Pool->m_UsedElements);
	Pool->m_pFirstFreeElement = NewElement->Next;

	((CDA_PoolableCompatibleDeleteCompatibleCRef *)(NewElement))->m_pPool = Pool;
//	MDA_PoolUnLock(Pool);

	return NewElement;	
}

void* CDA_PoolableCompatibleDeleteCompatibleCRef::operator new(mint _Size, CDA_Pool_Static &Pool)
{
	return CDA_PoolableCompatibleDeleteCompatibleCRef::operator new(_Size, &Pool);
}

void CDA_PoolableCompatibleDeleteCompatibleCRef::MRTC_Delete()
{
	if(this)
	{
		if (MRTC_IsDynamic())
			MRTC_GetRuntimeClass()->DelInstance();
		delete this;
	}
}

void CDA_PoolableCompatibleDeleteCompatibleCRef::operator delete(void *Block)
{
	CDA_Pool_Static *Pool = ((CDA_PoolableCompatibleDeleteCompatibleCRef *)(Block))->m_pPool;
	MDA_PoolLock(Pool);

	CDA_Pool_Static::CElement *DeleteElement = (CDA_Pool_Static::CElement *)Block;

	DeleteElement->Next = Pool->m_pFirstFreeElement;
	Pool->m_pFirstFreeElement = DeleteElement;

//	TRACE ("Returned %x\n", DeleteElement);

	--Pool->m_UsedElements;

//	TRACE ("Pool(%x)->m_UsedElements %d\n", Pool, Pool->m_UsedElements);

//	MDA_PoolUnLock(Pool);
}



