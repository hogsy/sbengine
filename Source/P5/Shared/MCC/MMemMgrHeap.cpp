
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

#ifdef PLATFORM_DOLPHIN
#include "../MOS/MSystem/MSystem_Dolphin.h"
#endif
#ifdef	PLATFORM_PS2
#include "../MOS/MSystem/MInfoScreen_PS2.h"
#include <eekernel.h>
#endif


// #undef MRTC_ENABLE_REMOTEDEBUGGER


#define _FREE_BLOCK      0
#define _NORMAL_BLOCK    1
#define _CRT_BLOCK       2
#define _IGNORE_BLOCK    3
#define _CLIENT_BLOCK    4
#define _MAX_BLOCKS      5



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| SDA_DefraggableFree
|__________________________________________________________________________________________________
\*************************************************************************************************/
#ifndef M_RTM
#define DA__HEAPVALIDATE
#endif

#ifdef BROKEN_BLOCK_COUNTER
static int g_nBlocks = 0;
#endif

SDA_Defraggable *BlockFromMem(const void *_Block);
SDA_Defraggable *BlockFromMem(const void *_Block)
{
	if ((((SDA_Defraggable *)_Block) - 1)->m_p0.m_Flags == 0)
	{
		// Normal block
		return ((SDA_Defraggable *)_Block) - 1;
	}
	else
	{
		M_ASSERT(!(((SDA_Defraggable *)_Block) - 1)->IsFree(), "Memory error");
//		SDA_Defraggable *Block = (((SDA_Defraggable *)_Block) - 1);		
		// Has to be debug
		return (SDA_Defraggable *)(((mint)_Block) - (MDA_DEBUG_BLOCK_PRECHECK + sizeof(SDA_Defraggable)));
	}
}

void* CDA_MemoryManager::MemFromBlock(SDA_Defraggable *_Block)
{
	if (_Block->m_p0.m_Flags == 0)
	{
		// Normal block
		return (_Block + 1);
	}
	if (_Block->IsFree())
	{
		// Free Block
		return (void *)((mint)_Block + sizeof(SDA_DefraggableFree));
	}
	else
	{
		// Has to be debug
		return (void *)((mint)_Block + sizeof(SDA_Defraggable) + MDA_DEBUG_BLOCK_PRECHECK);
	}
}


void SDA_DefraggableFreeLink::AddFirst(CDA_MemoryManager_SizeClass *_SizeClass)
{
	if (_SizeClass->m_FirstFreeBlock.m_pNextFree)
		_SizeClass->m_FirstFreeBlock.m_pNextFree->m_pPrevFree = this;

	m_pNextFree = _SizeClass->m_FirstFreeBlock.m_pNextFree;
	m_pPrevFree = &_SizeClass->m_FirstFreeBlock;

	_SizeClass->m_FirstFreeBlock.m_pNextFree = this;
}

#define M_GETOFFSET(_Class_, _Member_) ((mint)(&((CDA_MemoryManager_SizeClass *)0x0f000000)->_Member_) - (mint)((CDA_MemoryManager_SizeClass *)0x0f000000))


void SDA_DefraggableFreeLink::Remove()
{
	if (m_pNextFree)
		m_pNextFree->m_pPrevFree = m_pPrevFree;
	
	M_ASSERT(m_pPrevFree, "Error in linked list");
	if(m_pPrevFree)	// Assert above should take care if this if
	{
		m_pPrevFree->m_pNextFree = m_pNextFree;
		if (!m_pPrevFree->m_pPrevFree && !m_pNextFree)
		{

			// Free list is free
//			CDA_MemoryManager_SizeClass *TempPtr = (CDA_MemoryManager_SizeClass *)0x0f000000;
//			aint Offset = (mint)(&TempPtr->m_FirstFreeBlock) - (mint)TempPtr;

//			CDA_MemoryManager_SizeClass *TheClass = (CDA_MemoryManager_SizeClass *)((mint)m_pPrevFree - Offset);

//			TheClass->m_pMemoryManager->m_SizesPool.Delete(TheClass);
			CDA_MemoryManager_SizeClass *TheClass = (CDA_MemoryManager_SizeClass *)((mint)m_pPrevFree - M_GETOFFSET(CDA_MemoryManager_SizeClass, m_FirstFreeBlock));

			TheClass->m_pMemoryManager->m_SizesPool.Delete(TheClass);
		}
	}
}

void ReportOutOfMemory(aint Size, CDA_MemoryManager* _pMem)
{
	M_TRACEALWAYS(CFStrF("OUT OF MEMORY: Requested %d, Free mem %d, Largest free block %d\n", 
		Size, _pMem->GetFreeMem(), _pMem->GetLargestFreeMem()));
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDA_Defraggable
|__________________________________________________________________________________________________
\*************************************************************************************************/


void CDA_Defraggable::MoveObject(void *NewLocation)
{
}


// Dummy new constructor
void* CDA_Defraggable::operator new(mint _Size) throw()
{
	M_ASSERT(0, "You have to supply the memory manager in the new operator. new (MemoryManager) CClass");
	return NULL;	
}

// The destructor for all
void CDA_Defraggable::operator delete(void *Block)
{
#ifdef PLATFORM_DOLPHIN
	(*((CDA_MemoryManager **)(((uint8 *)Block))))->Free(Block);
#else
	(*((CDA_MemoryManager **)(((uint8 *)Block) + sizeof(void *))))->Free(Block);
#endif
}


// The destructor for all
void CDA_Defraggable::operator delete[] (void *Block)
{
#ifdef PLATFORM_DOLPHIN
	(*((CDA_MemoryManager **)(((uint8 *)Block))))->Free(Block);
#else
	(*((CDA_MemoryManager **)(((uint8 *)Block) + sizeof(void *))))->Free(Block);
#endif
}

void* CDA_Defraggable::operator new(mint _Size, CDA_MemoryManager *MemManager)
{
//	void *Return = MemManager->Alloc(_Size);
	void* Return = M_MEMALLOC(MemManager, _Size);

	if (Return)
#ifdef PLATFORM_DOLPHIN
		*((CDA_MemoryManager **)(((uint8 *)Return))) = MemManager;
#else
		*((CDA_MemoryManager **)(((uint8 *)Return) + sizeof(void *))) = MemManager;
#endif

	return Return;
}

void* CDA_Defraggable::operator new(mint _Size, CDA_MemoryManager *MemManager, mint _ExtraSize)
{
//	void *Return = MemManager->Alloc(_Size + _ExtraSize);
	void* Return = M_MEMALLOC(MemManager, _Size + _ExtraSize);
	if (Return)
#ifdef PLATFORM_DOLPHIN
		*((CDA_MemoryManager **)(((uint8 *)Return))) = MemManager;
#else
		*((CDA_MemoryManager **)(((uint8 *)Return) + sizeof(void *))) = MemManager;
#endif

	return Return;
}

void* CDA_Defraggable::operator new[] (mint _Size, CDA_MemoryManager *MemManager)
{
//	void *Return = MemManager->Alloc(_Size);
	void* Return = M_MEMALLOC(MemManager, _Size);
	if (Return)
#ifdef PLATFORM_DOLPHIN
		*((CDA_MemoryManager **)(((uint8 *)Return))) = MemManager;
#else
		*((CDA_MemoryManager **)(((uint8 *)Return) + sizeof(void *))) = MemManager;
#endif

	return Return;
}

void* CDA_Defraggable::operator new[] (mint _Size, CDA_MemoryManager *MemManager, mint _ExtraSize)
{
//	void *Return = MemManager->Alloc(_Size + _ExtraSize);
	void* Return = M_MEMALLOC(MemManager, _Size + _ExtraSize);
	if (Return)
#ifdef PLATFORM_DOLPHIN
		*((CDA_MemoryManager **)(((uint8 *)Return))) = MemManager;
#else
		*((CDA_MemoryManager **)(((uint8 *)Return) + sizeof(void *))) = MemManager;
#endif

	return Return;
}

#ifdef COMPILER_NEEDOPERATORDELETE

void CDA_Defraggable::operator delete[] (void *Block, CDA_MemoryManager *MemManager, mint _ExtraSize)
{
	M_ASSERT(0, "Should not be called");
}

void CDA_Defraggable::operator delete[] (void *Block,CDA_MemoryManager *MemManager)
{
	M_ASSERT(0, "Should not be called");
}

void CDA_Defraggable::operator delete(void *Block, CDA_MemoryManager *MemManager, mint _ExtraSize)
{
	M_ASSERT(0, "Should not be called");
}

void CDA_Defraggable::operator delete(void *Block,CDA_MemoryManager *MemManager)
{
	M_ASSERT(0, "Should not be called");
}

#endif

void CDA_Defraggable::PreMoveObject(mint &_Context)
{

}

void CDA_Defraggable::PostMoveObject(mint _Context)
{

}

EDA_Defraggable_MoveMethods CDA_Defraggable::GetMoveObjectMethodsAllowed()
{
	return EDA_Defraggable_MoveMethod_OldObjectIntact;
}

void CDA_Defraggable::Realloc(mint _NewSize)
{
	
}

void CDA_Defraggable::Shrink(mint _NewSize)
{
//	m_pMemoryManager->Realloc((void *)this, _NewSize);
	M_MEMREALLOC(m_pMemoryManager, (void*)this, _NewSize);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDA_MemoryManager_SizeClass
|__________________________________________________________________________________________________
\*************************************************************************************************/

static mint CountTreeObjects(CDA_MemoryManager_SizeClass *Object)
{
	if (!Object)
		return 0;

	mint Objects = 0;
	Objects += CountTreeObjects(Object->m_TreeLink.GetLeft());
	Objects += CountTreeObjects(Object->m_TreeLink.GetRight());
	return Objects + 1;
}



static mint CheckChildrenIsGreaterThan(CDA_MemoryManager_SizeClass *Object, mint Value)
{
	if (!Object)
		return 1;

	if (Object->m_Size < Value)
		return 0;

	if (!CheckChildrenIsGreaterThan(Object->m_TreeLink.GetLeft(), Value))
		return 0;

	if (!CheckChildrenIsGreaterThan(Object->m_TreeLink.GetRight(), Value))
		return 0;

	return 1;
}

static mint CheckChildrenIsLessThan(CDA_MemoryManager_SizeClass *Object, mint Value)
{
	if (!Object)
		return 1;

	if (Object->m_Size > Value)
			return 0;

	if (!CheckChildrenIsLessThan(Object->m_TreeLink.GetLeft(), Value))
		return 0;

	if (!CheckChildrenIsLessThan(Object->m_TreeLink.GetRight(), Value))
		return 0;

	return 1;
}


static mint CheckTreeObjects(CDA_MemoryManager_SizeClass *Object)
{
	if (!Object)
		return 1;

	if (Object->m_TreeLink.GetLeft())
		if (Object->m_TreeLink.GetLeft()->m_Size > Object->m_Size)
			return 0;

	if (Object->m_TreeLink.GetRight())
		if (Object->m_TreeLink.GetRight()->m_Size < Object->m_Size)
			return 0;

	if (!CheckChildrenIsGreaterThan(Object->m_TreeLink.GetRight(), Object->m_Size))
		return 0;

	if (!CheckChildrenIsLessThan(Object->m_TreeLink.GetLeft(), Object->m_Size))
		return 0;

	if (!CheckTreeObjects(Object->m_TreeLink.GetLeft()))
		return 0;

	if (!CheckTreeObjects(Object->m_TreeLink.GetRight()))
		return 0;
	return 1;
}


static void ValidateTree(CDA_MemoryManager *Manager)
{
	if (!CheckTreeObjects(Manager->m_SizesFreeTreeNormal.GetRoot()))
		M_BREAKPOINT;
	if (!CheckTreeObjects(Manager->m_SizesFreeTreeFragments.GetRoot()))
		M_BREAKPOINT;
}


static int IsHeapsOK(CDA_MemoryManager *Manager)
{
	M_LOCK(Manager->GetLock());

	SICDA_MemoryManager_HeapChunk Iterator(Manager->m_ChunksTree);
	
	while(Iterator)
	{
		SDA_Defraggable *Current = Iterator->m_pFirstBlock;
		SDA_Defraggable *Last = NULL;

		while (Current)
		{
			CDA_MemoryManager_HeapChunk *Chunk;
	
			Chunk = Manager->m_ChunksTree.FindLargestLessThanEqual(Current);
	
			if (!Chunk || Current >= Chunk->m_pHeapEnd)
				return NULL;

			if (Last != Current->m_pPrevBlockGet())
				return 0;				

			Last = Current;

			Current = Current->m_pNextBlock;
		}		

		++Iterator;
	}

	return 1;
}


static void ValidateHeaps(CDA_MemoryManager *Manager)
{
	if (!IsHeapsOK(Manager))
		M_BREAKPOINT;
}

//int g_TraceMemoryManager = 0;

CDA_MemoryManager_SizeClass::CDA_MemoryManager_SizeClass(CDA_MemoryManager *_pMemoryManager)
{
//	if (g_TraceMemoryManager)
//		M_TRACEALWAYS("CDA_MemoryManager_SizeClass() 0x%08x\n", this);

	m_pMemoryManager = _pMemoryManager;

	m_Size = 0;
	m_nContainedDefraggable = 0;
	m_FirstFreeBlock.m_pNextFree = NULL;
	m_FirstFreeBlock.m_pPrevFree = NULL;

//	m_Link.Link(m_pMemoryManager->m_Sizes, this);

}



CDA_MemoryManager_SizeClass::~CDA_MemoryManager_SizeClass()
{
//	if (g_TraceMemoryManager)
//		M_TRACEALWAYS("~CDA_MemoryManager_SizeClass() 0x%08x\n", this);

	if (m_TreeLink.IsInTree())
	{
		CDA_MemoryManager_SizeClass *pSizeClass = m_pMemoryManager->m_SizesFreeTreeNormal.FindEqual(m_Size);
		if (pSizeClass == this)
			m_pMemoryManager->m_SizesFreeTreeNormal.f_RemoveLowStack(this, (void*)NULL);
		else
		{
			pSizeClass = m_pMemoryManager->m_SizesFreeTreeFragments.FindEqual(m_Size);
			if (pSizeClass == this)
				m_pMemoryManager->m_SizesFreeTreeFragments.f_RemoveLowStack(this, (void*)NULL);
			else
				M_BREAKPOINT; // Error!!!
		}

	}

	if (m_nContainedDefraggable)
	{
		M_TRACE("Leaked %d blocks of size %d\n", m_nContainedDefraggable, m_Size);	
	}


#ifdef DA__HEAPVALIDATE
	if (m_pMemoryManager->m_bValidateHeap)
		ValidateTree(m_pMemoryManager);
#endif
}



CDA_MemoryManager_SizeClass *CDA_MemoryManager::GetFreeSizeClass(mint _Size, bint _bFragment)
{
	CDA_MemoryManager_SizeClass *SizeClass;
	if (_bFragment)
	{
		SizeClass = (CDA_MemoryManager_SizeClass *)m_SizesFreeTreeFragments.FindEqual(_Size);
		if (!SizeClass)
		{
			SizeClass = m_SizesPool.New(this);
			SizeClass->m_Size = _Size;
			m_SizesFreeTreeFragments.f_InsertLowStack(SizeClass, (void*)NULL);
	#ifdef DA__HEAPVALIDATE
			if (m_bValidateHeap)
				ValidateHeap();
	#endif
		}
	}
	else
	{
		SizeClass = (CDA_MemoryManager_SizeClass *)m_SizesFreeTreeNormal.FindEqual(_Size);
		if (!SizeClass)
		{
			SizeClass = m_SizesPool.New(this);
			SizeClass->m_Size = _Size;
			m_SizesFreeTreeNormal.f_InsertLowStack(SizeClass, (void*)NULL);
	#ifdef DA__HEAPVALIDATE
			if (m_bValidateHeap)
				ValidateHeap();
	#endif
		}
	}

	return SizeClass;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDA_MemoryManager_HeapChunk
|__________________________________________________________________________________________________
\*************************************************************************************************/


CDA_MemoryManager_HeapChunk::CDA_MemoryManager_HeapChunk(CDA_MemoryManager *_pMemmanager, mint _Size)
{
	if (!_pMemmanager->m_CommitGranularity)
		_pMemmanager->m_CommitGranularity = MRTC_SystemInfo::OS_CommitGranularity();

	m_pMemmanager = _pMemmanager;
	{
		M_UNLOCK(m_pMemmanager->m_Lock);
		m_pHeap = m_pMemmanager->AllocHeap(_Size, true);
	}
	if (!m_pHeap)
		return;
	m_pMemmanager->m_ChunksTree.f_InsertLowStack(this, (void*)NULL);

#ifdef M_SUPPORTMEMORYDEBUG
	if (m_pMemmanager->m_bDebugMemory)
		memset(m_pHeap, 0xDD, _Size);
#endif

	m_pFirstBlock = (SDA_Defraggable *)m_pHeap;
	m_pFirstBlock->m_pPrevBlockSet(NULL);
	m_pFirstBlock->m_pNextBlock = NULL;
	m_pFirstBlock->SetFree();
	m_pFirstBlock->ClearDebug();

	CDA_MemoryManager_SizeClass *SizeClass = m_pMemmanager->GetFreeSizeClass(_Size, false);

	((SDA_DefraggableFree *)m_pFirstBlock)->m_FreeLink.AddFirst(SizeClass);

	m_pMemmanager->m_AllocatedMem += _Size;
	m_pMemmanager->m_FreeMem += _Size;

	m_pHeapEnd = (uint8 *)m_pHeap + _Size;

}
CDA_MemoryManager_HeapChunk::~CDA_MemoryManager_HeapChunk()
{

	if (m_TreeLink.IsInTree())
		m_pMemmanager->m_ChunksTree.f_RemoveLowStack(this, (void*)NULL);


#ifdef M_SUPPORTMEMORYDEBUG
	if (m_pMemmanager->m_bMemDebug)
	{
		if (m_pFirstBlock->m_pNextBlock)
		{
			M_TRACEALWAYS("Mem leak detected");
		}
	}	
#endif

	if (m_pHeap)
		m_pMemmanager->FreeHeap(m_pHeap);
	m_pHeap = NULL;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDA_MemoryManager
|__________________________________________________________________________________________________
\*************************************************************************************************/

void CDA_MemoryManager::PrivateInit()
{
	m_bDestroyCalled = false;
	
	m_CommitGranularity = 0;

	m_LargeBlockThreshold = 512 * 1024;


	m_AllocatedMem = 0;
	m_AlignAlloc = MDA_ALIGNMENT;
	m_FreeMemMin = 0xffffffff;
	m_FreeMem = 0;
#ifdef M_SUPPORTMEMORYDEBUG
	m_MemTracking_bOn = false;
	m_bMemDebug = false;
	m_bShowAllocs = false;
	m_SequenceMark = 1;
#endif

	m_bValidateHeap = false;

	m_RunningDebugRuntime = false;
	m_RunningReleaseRuntime = false;
#ifdef M_SUPPORTMEMORYDEBUG
	m_bDebugMemory = false;
#endif

	m_bCanDefrag = false;
	m_bDynamicSize = true;

	m_GlobalAlign = 0;

	m_DynamicGrowSize = 0x100000; // 1 meg


#ifdef MRTC_DEFAULTMAINHEAP
	m_pDefaultManager = NULL;
	m_bUseDefaultMainHeap = false;
#endif

	gf_RDSendRegisterHeap((mint)this, "Main", 0, 0);

}

void CDA_MemoryManager::SetGlobalAlignment(mint _Align)
{
	M_LOCK(m_Lock);
	m_GlobalAlign = _Align;
}

void CDA_MemoryManager::ValidateHeap()
{
	ValidateTree(this);
	ValidateHeaps(this);
}

bool CDA_MemoryManager::IsHeapOk()
{
	M_LOCK(m_Lock);
	return (::IsHeapsOK(this)) != 0;
}

CDA_MemoryManager::CDA_MemoryManager()
{
	PrivateInit();
}

CDA_MemoryManager::CDA_MemoryManager(mint _StaticHeapSize)
{
	PrivateInit();

	InitStatic(_StaticHeapSize);
}


CDA_MemoryManager::~CDA_MemoryManager()
{
	if (!m_bDestroyCalled)
	{
		M_ASSERT(0, "You have to call destroy in the inherited destructor, or else!!!");
	}
}

void CDA_MemoryManager::Destroy()
{
	M_LOCK(m_Lock);

#ifdef DA__HEAPVALIDATE
	if (m_bValidateHeap)
		ValidateHeap();
#endif

#ifdef M_SUPPORTMEMORYDEBUG
	{
		m_MemTracking_ClassesList.DeleteAll();
	}	
#endif

	{
		while (m_SizesFreeTreeFragments.GetRoot())
		{
			CDA_MemoryManager_SizeClass *pRoot = m_SizesFreeTreeFragments.GetRoot();
			m_SizesPool.Delete(pRoot);
		}
		while (m_SizesFreeTreeNormal.GetRoot())
		{
			CDA_MemoryManager_SizeClass *pRoot = m_SizesFreeTreeNormal.GetRoot();
			m_SizesPool.Delete(pRoot);
		}
	}

	{
		while (m_ChunksTree.GetRoot())
		{
			CDA_MemoryManager_HeapChunk *pRoot = m_ChunksTree.GetRoot();
			m_ChunksPool.Delete(pRoot);
		}
	}

#ifdef M_SUPPORTMEMORYDEBUG
	{
		CDA_MemoryManager_DebugAllocFile *pDebugAlloc = m_DebugStringTree.GetRoot();
		while (pDebugAlloc)
		{
			m_DebugStringTree.f_Remove(pDebugAlloc);
			m_DebugStringsPool.Delete(pDebugAlloc);
			pDebugAlloc = m_DebugStringTree.GetRoot();
		}
	}
#endif


	m_bDestroyCalled = true;
}

void CDA_MemoryManager::InitStatic(mint _HeapSize)
{
	if (_HeapSize < 128) 
		_HeapSize = 128;
	M_LOCK(m_Lock);

	CDA_MemoryManager_HeapChunk *pCunk = m_ChunksPool.New(this, _HeapSize);

	if (!pCunk || !pCunk->m_pHeap)
	{
		M_ASSERT(0, "Failed to alloc memory");
		if (pCunk)
		{
			m_ChunksPool.Delete(pCunk);
		}
	}

	mint Min = (mint)m_ChunksTree.FindSmallest()->m_pHeap;
	mint Max = (mint)m_ChunksTree.FindLargest()->m_pHeapEnd;

	gf_RDSendRegisterHeap((mint)this, "Main", Min, Max);

	m_bDynamicSize = false;

}

void *CDA_MemoryManager::AllocHeap(mint Size, bool _bCommit)
{
	return MRTC_SystemInfo::OS_Alloc(Size, 4096);
}

void CDA_MemoryManager::FreeHeap(void *Block)
{
	MRTC_SystemInfo::OS_Free(Block);
}

bool CDA_MemoryManager::HeapCommit(void *_pMem, mint _Size, bool _bCommited)
{
	return MRTC_SystemInfo::OS_Commit(_pMem, _Size, _bCommited);
}

mint CDA_MemoryManager::HeapCommitGranularity()
{
	return MRTC_SystemInfo::OS_CommitGranularity();
}


EDA_MemoryManager_DefragErrors CDA_MemoryManager::Defrag(bool _bFreeBottom)
{
	M_LOCK(m_Lock);

#ifdef DA__HEAPVALIDATE
	if (m_bValidateHeap)
		ValidateHeap();
#endif

	//	M_TRACEALWAYS(CStr("CDA_MemoryManager::Defrag();\n"));

	if (_bFreeBottom)
	{
		TProfileDef(Timer);
		{
			TMeasureProfile(Timer);
			// Iterate all chunks
			SICDA_MemoryManager_HeapChunk Iterator = m_ChunksTree;
			
			while(Iterator)
			{
			#ifdef DA__HEAPVALIDATE
				if (m_bValidateHeap)
					ValidateHeap();
			#endif
				// Iterate all blocks
				SDA_Defraggable *Current = Iterator->m_pFirstBlock;
				while (Current && Current->m_pNextBlock)
					Current = Current->m_pNextBlock;

				// Now we are at last block
				
				while (Current)
				{
				#ifdef DA__HEAPVALIDATE
					if (m_bValidateHeap)
						ValidateHeap();
				#endif

					if (Current->IsFree() && Current->m_pPrevBlockGet())
					{
						// Move the next block to this location
						SDA_Defraggable *BlockToMove = Current->m_pPrevBlockGet();
						
#ifdef DA__HEAPVALIDATE
						if (BlockToMove->IsFree())
							M_BREAKPOINT;
							// Internal error: Two sequential free blocks
#endif
						
						void *Mem = MemFromBlock(BlockToMove);
						
	#ifndef M_FAKEDYNAMICCAST
						M_ASSERT(TDynamicCast<CDA_Defraggable >((CDA_Defraggable *)Mem), "Internal error: Not a CDA_Degraggable object.");
	#endif
						
						CDA_Defraggable *Defraggable = (CDA_Defraggable *)Mem;
						
						EDA_Defraggable_MoveMethods MoveMethod = Defraggable->GetMoveObjectMethodsAllowed();
						
						mint MoveContext = 0;
						switch(MoveMethod)
						{
						case EDA_Defraggable_MoveMethod_CantMove:
							M_ASSERT(0, "We currently cant defrag heaps when one or more blocks is unmoveable.");
							break;
						case EDA_Defraggable_MoveMethod_OldObjectIntact:
							// Todo: fix this
							break;
						case EDA_Defraggable_MoveMethod_OldObjectTrashed_WithPre:
						case EDA_Defraggable_MoveMethod_OldObjectTrashed_WithPreAndPost:
							
							Defraggable->PreMoveObject(MoveContext);
							
						case EDA_Defraggable_MoveMethod_OldObjectTrashed:
						case EDA_Defraggable_MoveMethod_OldObjectTrashed_WithPost:
							
							// Do the trash copy	
						#ifdef DA__HEAPVALIDATE
							if (m_bValidateHeap)
								ValidateHeap();
						#endif

							
							SDA_Defraggable *NewBlock;
							if (BlockToMove->m_pPrevBlockGet() && BlockToMove->m_pPrevBlockGet()->IsFree())
							{
								SDA_DefraggableFree* pFreeBottom = (SDA_DefraggableFree*)BlockToMove->m_pPrevBlockGet();
								// Remove free blocks
								((SDA_DefraggableFree*)Current)->m_FreeLink.Remove();
								pFreeBottom->m_FreeLink.Remove();
								
								SDA_Defraggable *NextFromMove = Current->m_pNextBlock;
								SDA_Defraggable *PrevFromMove = pFreeBottom->m_BlockLink.m_pPrevBlockGet();
								mint Size = GetBlockSize(BlockToMove);
								mint FreeBlockSize = GetBlockSize(Current) + GetBlockSize(&pFreeBottom->m_BlockLink);
								NewBlock = (SDA_Defraggable*)(((uint8 *)Current) + GetBlockSize(Current) - Size);;

								memmove(NewBlock, BlockToMove, Size);
								
								// Reconstruct free block
								SDA_DefraggableFree* FreeBlock = pFreeBottom;
								
								FreeBlock->m_BlockLink.ClearDebug();
								FreeBlock->m_BlockLink.SetFree();
								
								FreeBlock->m_BlockLink.m_pNextBlock = NewBlock;
								FreeBlock->m_BlockLink.m_pPrevBlockSet(PrevFromMove);
								
								if (NextFromMove)
									NextFromMove->m_pPrevBlockSet(NewBlock);
								
								NewBlock->m_pNextBlock = NextFromMove;
								NewBlock->m_pPrevBlockSet(&FreeBlock->m_BlockLink);
								
								CDA_MemoryManager_SizeClass *LeftOverSizeClass = GetFreeSizeClass(FreeBlockSize, false);
								
								FreeBlock->m_FreeLink.AddFirst(LeftOverSizeClass);
								#ifdef DA__HEAPVALIDATE
									if (m_bValidateHeap)
										ValidateHeap();
								#endif
							}
							else 
							{
								mint Size = GetBlockSize(BlockToMove);
								mint FreeBlockSize = GetBlockSize(Current);
								
								// Remove free block
								((SDA_DefraggableFree*)Current)->m_FreeLink.Remove();
								
								SDA_Defraggable *NextFromMove = Current->m_pNextBlock;
								SDA_Defraggable *PrevFromMove = BlockToMove->m_pPrevBlockGet();
								NewBlock = (SDA_Defraggable*)(((uint8 *)Current) + FreeBlockSize - Size);;
								
								memmove(NewBlock, BlockToMove, Size);
								
								// Reconstruct free block
								SDA_DefraggableFree* FreeBlock = (SDA_DefraggableFree*)BlockToMove; //(SDA_DefraggableFree*)(((uint8 *)Current) + Size);
								
								FreeBlock->m_BlockLink.ClearDebug();
								FreeBlock->m_BlockLink.SetFree();

								FreeBlock->m_BlockLink.m_pPrevBlockSet(PrevFromMove);
								FreeBlock->m_BlockLink.m_pNextBlock = NewBlock;
								
								if (NextFromMove)
									NextFromMove->m_pPrevBlockSet(NewBlock);
								
								NewBlock->m_pNextBlock = NextFromMove;
								NewBlock->m_pPrevBlockSet(&FreeBlock->m_BlockLink);
								
								CDA_MemoryManager_SizeClass *LeftOverSizeClass = GetFreeSizeClass(FreeBlockSize, false);
								
								FreeBlock->m_FreeLink.AddFirst(LeftOverSizeClass);
							#ifdef DA__HEAPVALIDATE
								if (m_bValidateHeap)
									ValidateHeap();
							#endif
							}
							
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
							gf_RDSendHeapFree(Mem, this, gf_RDGetSequence());
#endif
							Mem = MemFromBlock(NewBlock);
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
							{
								uint16 BlockType = _NORMAL_BLOCK;
								mint Size = GetBlockSize(NewBlock);
#ifdef M_SUPPORTMEMORYDEBUG
								if(NewBlock->IsDebug())
									BlockType	= ((SDA_DefraggableDebug_Post *)((mint)NewBlock + Size - sizeof(SDA_DefraggableDebug_Post)))->m_BlockType;
#endif
								gf_RDSendHeapAlloc(Mem, Size, this, gf_RDGetSequence(), BlockType);
							}
#endif
							
							Defraggable = (CDA_Defraggable *)Mem;
							
							Defraggable->MoveObject(Mem);
							
						#ifdef DA__HEAPVALIDATE
							if (m_bValidateHeap)
								ValidateHeap();
						#endif
							
							break;
						default:
							break;
						}			
						
						
						switch(MoveMethod)
						{
						case EDA_Defraggable_MoveMethod_OldObjectTrashed_WithPreAndPost:
						case EDA_Defraggable_MoveMethod_OldObjectTrashed_WithPost:
							Defraggable->PostMoveObject(MoveContext);
							break;
						default:
							break;
						}				
						
					}
					
					Current = Current->m_pPrevBlockGet();
				}		
				
				++Iterator;
			}
		}
	}
	else
	{

		TProfileDef(Timer);
		{
			TMeasureProfile(Timer);
			// Iterate all chunks
			SICDA_MemoryManager_HeapChunk Iterator = m_ChunksTree;
			
			while(Iterator)
			{
			#ifdef DA__HEAPVALIDATE
				if (m_bValidateHeap)
					ValidateHeap();
			#endif
				// Iterate all blocks
				SDA_Defraggable *Current = Iterator->m_pFirstBlock;
				
				while (Current)
				{
				#ifdef DA__HEAPVALIDATE
					if (m_bValidateHeap)
						ValidateHeap();
				#endif

					if (Current->IsFree() && Current->m_pNextBlock)
					{
						// Move the next block to this location
						SDA_Defraggable *BlockToMove = Current->m_pNextBlock;
						
#ifdef DA__HEAPVALIDATE
						if (BlockToMove->IsFree())
							M_BREAKPOINT;

							// Internal error: Two sequential free blocks
#endif
						
						void *Mem = MemFromBlock(BlockToMove);
						
	#ifndef M_FAKEDYNAMICCAST
						M_ASSERT(TDynamicCast<CDA_Defraggable >((CDA_Defraggable *)Mem), "Internal error: Not a CDA_Degraggable object.");
	#endif
						
						CDA_Defraggable *Defraggable = (CDA_Defraggable *)Mem;
						
						EDA_Defraggable_MoveMethods MoveMethod = Defraggable->GetMoveObjectMethodsAllowed();
						
						mint MoveContext = 0;
						switch(MoveMethod)
						{
						case EDA_Defraggable_MoveMethod_CantMove:
							M_ASSERT(0, "We currently cant defrag heaps when one or more blocks is unmoveable.");
							break;
						case EDA_Defraggable_MoveMethod_OldObjectIntact:
							// Todo: fix this
							break;
						case EDA_Defraggable_MoveMethod_OldObjectTrashed_WithPre:
						case EDA_Defraggable_MoveMethod_OldObjectTrashed_WithPreAndPost:
							
							Defraggable->PreMoveObject(MoveContext);
							
						case EDA_Defraggable_MoveMethod_OldObjectTrashed:
						case EDA_Defraggable_MoveMethod_OldObjectTrashed_WithPost:
							
							// Do the trash copy	
						#ifdef DA__HEAPVALIDATE
							if (m_bValidateHeap)
								ValidateHeap();
						#endif

							
							if (BlockToMove->m_pNextBlock && BlockToMove->m_pNextBlock->IsFree())
							{
								// Remove free blocks
								((SDA_DefraggableFree*)Current)->m_FreeLink.Remove();
								((SDA_DefraggableFree*)BlockToMove->m_pNextBlock)->m_FreeLink.Remove();
								
								SDA_Defraggable *NextFromMove = BlockToMove->m_pNextBlock->m_pNextBlock;
								SDA_Defraggable *PrevFromMove = Current->m_pPrevBlockGet();
								mint Size = GetBlockSize(BlockToMove);
								mint FreeBlockSize = GetBlockSize(Current) + GetBlockSize(BlockToMove->m_pNextBlock);
								
								memmove(Current, BlockToMove, Size);
								
								// Reconstruct free block
								SDA_DefraggableFree* FreeBlock = (SDA_DefraggableFree*)(((uint8 *)Current) + Size);
								
								FreeBlock->m_BlockLink.ClearDebug();
								FreeBlock->m_BlockLink.SetFree();
								
								FreeBlock->m_BlockLink.m_pPrevBlockSet(Current);
								FreeBlock->m_BlockLink.m_pNextBlock = NextFromMove;
								
								if (NextFromMove)
									NextFromMove->m_pPrevBlockSet(&FreeBlock->m_BlockLink);
								
								Current->m_pNextBlock = &FreeBlock->m_BlockLink;
								Current->m_pPrevBlockSet(PrevFromMove);
								
								CDA_MemoryManager_SizeClass *LeftOverSizeClass = GetFreeSizeClass(FreeBlockSize, false);
								
								FreeBlock->m_FreeLink.AddFirst(LeftOverSizeClass);
								#ifdef DA__HEAPVALIDATE
									if (m_bValidateHeap)
										ValidateHeap();
								#endif
							}
							else 
							{
								mint Size = GetBlockSize(BlockToMove);
								mint FreeBlockSize = GetBlockSize(Current);
								
								// Remove free block
								((SDA_DefraggableFree*)Current)->m_FreeLink.Remove();
								
								SDA_Defraggable *NextFromMove = BlockToMove->m_pNextBlock;
								SDA_Defraggable *PrevFromMove = Current->m_pPrevBlockGet();
								
								memmove(Current, BlockToMove, Size);
								
								// Reconstruct free block
								SDA_DefraggableFree* FreeBlock = (SDA_DefraggableFree*)(((uint8 *)Current) + Size);
								
								FreeBlock->m_BlockLink.ClearDebug();
								FreeBlock->m_BlockLink.SetFree();
								
								FreeBlock->m_BlockLink.m_pPrevBlockSet(Current);
								FreeBlock->m_BlockLink.m_pNextBlock = NextFromMove;
								
								if (NextFromMove)
									NextFromMove->m_pPrevBlockSet(&FreeBlock->m_BlockLink);
								
								Current->m_pNextBlock = &FreeBlock->m_BlockLink;
								Current->m_pPrevBlockSet(PrevFromMove);
								
								CDA_MemoryManager_SizeClass *LeftOverSizeClass = GetFreeSizeClass(FreeBlockSize, false);
								
								FreeBlock->m_FreeLink.AddFirst(LeftOverSizeClass);
							#ifdef DA__HEAPVALIDATE
								if (m_bValidateHeap)
									ValidateHeap();
							#endif
							}
							
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
							gf_RDSendHeapFree(Mem, this, gf_RDGetSequence());
#endif
							Mem = MemFromBlock(Current);
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
							{
								uint16 BlockType = _NORMAL_BLOCK;
								mint Size = GetBlockSize(Current);
#ifdef M_SUPPORTMEMORYDEBUG
								if(Current->IsDebug())
									BlockType	= ((SDA_DefraggableDebug_Post *)((mint)Current + Size - sizeof(SDA_DefraggableDebug_Post)))->m_BlockType;
#endif
								gf_RDSendHeapAlloc(Mem, Size, this, gf_RDGetSequence(), BlockType);
							}
#endif
							
							Defraggable = (CDA_Defraggable *)Mem;
							
							Defraggable->MoveObject(Mem);
							
						#ifdef DA__HEAPVALIDATE
							if (m_bValidateHeap)
								ValidateHeap();
						#endif
							
							break;
						default:
							break;
						}			
						
						
						switch(MoveMethod)
						{
						case EDA_Defraggable_MoveMethod_OldObjectTrashed_WithPreAndPost:
						case EDA_Defraggable_MoveMethod_OldObjectTrashed_WithPost:
							Defraggable->PostMoveObject(MoveContext);
							break;
						default:
							break;
						}				
						
					}
					
					Current = Current->m_pNextBlock;
				}		
				
				++Iterator;
			}
		}
	}

#ifdef DA__HEAPVALIDATE
	if (m_bValidateHeap)
		ValidateHeap();
#endif

//	M_TRACEALWAYS(T_String("Deragtime", Timer));
//	M_TRACEALWAYS("\n");
	
	
	return EDA_MemoryManager_DefragErrors_OK;
}

static bool IsPowerOfTwo(mint Number)
{
	for (int CurrentNumber = 1; CurrentNumber; CurrentNumber = (CurrentNumber << 1))
	{
		if (Number == CurrentNumber)
			return true;

	}
	return false;
}

static void CheckMemoryToBeByte(uint8 *Memory, uint8 Byte, bool &Damage)
{
	if (Byte != *Memory)
	{		
		M_TRACEALWAYS(CFStrF("Damage: 0x%08x: 0x%02x resetting memory to 0x%02x\n", Memory, (int)*Memory, Byte));
		*Memory = Byte;
		Damage = true;
	}
}

static bool CheckMemoryToBe(void *Memory, uint8 Byte, int Len, const char *ErrorString, bool Break)
{
	uint8 *Mem = (uint8 *)Memory;
	
	bool Damage = false;

	if (Len > 4)
	{
		uint8 Test1 = Byte;
		uint16 Test2 = Byte | (Byte<<8);
		uint32 Test4 = Test2 | (Test2<<16);

		if ((mint)Mem & 1)
		{
			if (*Mem != Test1)
			{
				CheckMemoryToBeByte(Mem, Byte, Damage);
			}
			++Mem;
			Len-=1;
		}

		if ((mint)Mem & 2)
		{
			if (*((uint16 *)Mem) != Test2)
			{
				CheckMemoryToBeByte(Mem, Byte, Damage);
				CheckMemoryToBeByte(Mem+1, Byte, Damage);
			}
			Mem+=2;
			Len-=2;
		}

		while (Len > 4)
		{
			if (*((uint32 *)Mem) != Test4)
			{
				CheckMemoryToBeByte(Mem, Byte, Damage);
				CheckMemoryToBeByte(Mem+1, Byte, Damage);
				CheckMemoryToBeByte(Mem+2, Byte, Damage);
				CheckMemoryToBeByte(Mem+3, Byte, Damage);
			}
			Mem+=4;
			Len-=4;
		}

	}
	
	for (int i = 0; i < Len; ++i)
	{
		if (Mem[i] != Byte)
		{
			CheckMemoryToBeByte(Mem + i, Byte, Damage);
		}
	}

	if (Damage)
	{
		M_TRACEALWAYS(CFStrF("%s. (See above)\n",ErrorString));	
		if (Break)
			M_ASSERT(0, "?");
	}

	return !Damage;
}

bool CDA_MemoryManager::CommitMemory(void *_pMemStart, void *_pMemEnd)
{
	_pMemStart = (void *)(((((mint)_pMemStart) / m_CommitGranularity) + 1) * m_CommitGranularity);
	if (((uint8 *)_pMemEnd - (uint8 *)_pMemStart) >= m_CommitGranularity)
	{
		int CommitSize = (((((uint8 *)_pMemStart - (uint8 *)_pMemEnd)) / m_CommitGranularity) + 1) / m_CommitGranularity;
		if (!HeapCommit((uint8 *)_pMemStart, CommitSize, true))
		{
#ifdef M_SUPPORTMEMORYDEBUG
			
			MemTracking_ReportMemUsage(0);
			MemTracking_ReportMemUsage(1);
#endif
			return false;
		}			
	}

	return true;
}

void CDA_MemoryManager::DeCommitMemory(void *_pMemStart, void *_pMemEnd)
{
	_pMemStart = (void *)(((((mint)_pMemStart) / m_CommitGranularity) + 1) * m_CommitGranularity);
	if (((uint8 *)_pMemEnd - (uint8 *)_pMemStart) >= m_CommitGranularity)
	{
		int CommitSize = (((((uint8 *)_pMemEnd - (uint8 *)_pMemStart)) / m_CommitGranularity) + 1) / m_CommitGranularity;
		bool Commited = HeapCommit(_pMemStart, CommitSize, false);
		M_ASSERT(Commited, "Must be able to decommit blocks");
	}
}

void *CDA_MemoryManager::AllocImp(mint Size, mint _Alignment)
{
#ifdef MRTC_DEFAULTMAINHEAP
	if (m_bUseDefaultMainHeap)
	{
		MRTC_SystemInfo::OS_Assert("Cannot alloc with default heap");
		return NULL;
	}
#endif

	// Align to alloc granularity (4 uint8s default) (4 uint8s minimum) (power of 2)
	M_ASSERT(m_AlignAlloc >= 4, "Alloc granularity is too small");
	mint Alignment = _Alignment;
	if (Alignment < m_AlignAlloc)
		Alignment = m_AlignAlloc;

	mint SizeNeeded = Size + sizeof(SDA_DefraggableRelease);
	mint RealSizeNeeded = SizeNeeded;

	if (Alignment > m_AlignAlloc)
	{
		SizeNeeded += sizeof(SDA_DefraggableFree); // We can include a free block before (size is aligned on m_AlignAlloc)
		SizeNeeded += Alignment - m_AlignAlloc; // Be able to align to our alignment
	}

	SizeNeeded = ((SizeNeeded + (m_AlignAlloc - 1)) & (~(m_AlignAlloc - 1)));
	RealSizeNeeded = ((RealSizeNeeded + (m_AlignAlloc - 1)) & (~(m_AlignAlloc - 1)));

	if (SizeNeeded < sizeof(SDA_DefraggableFree))
		SizeNeeded = sizeof(SDA_DefraggableFree);
	if (RealSizeNeeded < sizeof(SDA_DefraggableFree))
		RealSizeNeeded = sizeof(SDA_DefraggableFree);

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	uint64 RDSequnce = 0;
#endif
	void *pRet;
	{
		M_LOCK(m_Lock);

	#ifdef DA__HEAPVALIDATE
		if (m_bValidateHeap)
		ValidateHeaps(this);
	#endif

		CDA_MemoryManager_SizeClass* SizeClass = GetSizeClass(SizeNeeded);
		if (!SizeClass)
			return NULL;

		M_ASSERT(SizeClass->m_FirstFreeBlock.m_pNextFree, "Heap search error");

		SDA_DefraggableFree *Block = (SDA_DefraggableFree *)((uint8 *)SizeClass->m_FirstFreeBlock.m_pNextFree - sizeof(SDA_Defraggable));

		mint SizeClassSize = SizeClass->m_Size;

		Block->m_FreeLink.Remove();

		mint LeftOverSize = 0;
		mint FinalBlockSize = SizeClassSize;

		if (SizeNeeded >= m_LargeBlockThreshold)
		{
			// Insert a free block before our block and adjust Block
			mint BlockAddress = (mint)(Block);
			mint EndAddress = (BlockAddress + FinalBlockSize) - (RealSizeNeeded);

			mint AlignedEndAddress = AlignDown(EndAddress, Alignment);

			mint StartDefraggable = AlignedEndAddress - sizeof(SDA_Defraggable);
			mint PreBlockSize = StartDefraggable - BlockAddress;
			if (PreBlockSize)
			{
				FinalBlockSize -= PreBlockSize;

				CDA_MemoryManager_SizeClass *LeftOverSizeClass = GetFreeSizeClass(PreBlockSize, true);

				SDA_DefraggableFree *NewBlock = Block;
				SDA_DefraggableFree *NextBlock = (SDA_DefraggableFree *)StartDefraggable;

				*((SDA_Defraggable *)NextBlock) = *((SDA_Defraggable *)NewBlock);

				NewBlock->m_BlockLink.SetFree();
				NewBlock->m_BlockLink.ClearDebug();
				
				NextBlock->m_BlockLink.m_pPrevBlockSet(&NewBlock->m_BlockLink);
				NewBlock->m_BlockLink.m_pNextBlock = &NextBlock->m_BlockLink;
				if (NextBlock->m_BlockLink.m_pNextBlock)
					NextBlock->m_BlockLink.m_pNextBlock->m_pPrevBlockSet(&NextBlock->m_BlockLink);

				NewBlock->m_FreeLink.AddFirst(LeftOverSizeClass);

				Block = NextBlock;
			}

			LeftOverSize = FinalBlockSize - RealSizeNeeded;

			if (LeftOverSize > sizeof(SDA_DefraggableFree))
			{
				FinalBlockSize -= LeftOverSize;

				CDA_MemoryManager_SizeClass *LeftOverSizeClass;
				if (LeftOverSize > m_LargeBlockThreshold)
					LeftOverSizeClass = GetFreeSizeClass(LeftOverSize, false);
				else
					LeftOverSizeClass = GetFreeSizeClass(LeftOverSize, true);

				SDA_DefraggableFree *NewBlock = (SDA_DefraggableFree *)(((uint8 *)Block) + FinalBlockSize);

				NewBlock->m_BlockLink.SetFree();
				NewBlock->m_BlockLink.ClearDebug();

				if (Block->m_BlockLink.m_pNextBlock)
					Block->m_BlockLink.m_pNextBlock->m_pPrevBlockSet(&NewBlock->m_BlockLink);

				NewBlock->m_BlockLink.m_pNextBlock = Block->m_BlockLink.m_pNextBlock;
				Block->m_BlockLink.m_pNextBlock = &NewBlock->m_BlockLink;
				NewBlock->m_BlockLink.m_pPrevBlockSet(&Block->m_BlockLink);		

				NewBlock->m_FreeLink.AddFirst(LeftOverSizeClass);
			}
		}
		else
		{
			if (Alignment > m_AlignAlloc)
			{
				// Insert a free block before our block and adjust Block
				mint BlockAddress = (mint)(Block);
				mint EndAddress = BlockAddress + sizeof(SDA_DefraggableFree) + sizeof(SDA_Defraggable);

				mint AlignedEndAddress = (EndAddress + (Alignment - 1)) & (~(Alignment - 1)); // Align

				mint StartDefraggable = AlignedEndAddress - sizeof(SDA_Defraggable);
				mint PreBlockSize = StartDefraggable - BlockAddress;
				if (PreBlockSize)
				{
					FinalBlockSize -= PreBlockSize;

					CDA_MemoryManager_SizeClass *LeftOverSizeClass = GetFreeSizeClass(PreBlockSize, false);

					SDA_DefraggableFree *NewBlock = Block;
					SDA_DefraggableFree *NextBlock = (SDA_DefraggableFree *)StartDefraggable;

					*((SDA_Defraggable *)NextBlock) = *((SDA_Defraggable *)NewBlock);

					NewBlock->m_BlockLink.SetFree();
					NewBlock->m_BlockLink.ClearDebug();
					
					NextBlock->m_BlockLink.m_pPrevBlockSet(&NewBlock->m_BlockLink);
					NewBlock->m_BlockLink.m_pNextBlock = &NextBlock->m_BlockLink;
					if (NextBlock->m_BlockLink.m_pNextBlock)
						NextBlock->m_BlockLink.m_pNextBlock->m_pPrevBlockSet(&NextBlock->m_BlockLink);

					NewBlock->m_FreeLink.AddFirst(LeftOverSizeClass);

					Block = NextBlock;
				}
			}

			LeftOverSize = FinalBlockSize - RealSizeNeeded;

			if (LeftOverSize > sizeof(SDA_DefraggableFree))
			{
				FinalBlockSize -= LeftOverSize;

				CDA_MemoryManager_SizeClass *LeftOverSizeClass = GetFreeSizeClass(LeftOverSize, false);

				SDA_DefraggableFree *NewBlock = (SDA_DefraggableFree *)(((uint8 *)Block) + FinalBlockSize);

				NewBlock->m_BlockLink.SetFree();
				NewBlock->m_BlockLink.ClearDebug();

				if (Block->m_BlockLink.m_pNextBlock)
					Block->m_BlockLink.m_pNextBlock->m_pPrevBlockSet(&NewBlock->m_BlockLink);

				NewBlock->m_BlockLink.m_pNextBlock = Block->m_BlockLink.m_pNextBlock;
				Block->m_BlockLink.m_pNextBlock = &NewBlock->m_BlockLink;
				NewBlock->m_BlockLink.m_pPrevBlockSet(&Block->m_BlockLink);		

				NewBlock->m_FreeLink.AddFirst(LeftOverSizeClass);
			}
		}


		m_FreeMem -= FinalBlockSize;
		if (m_FreeMem < m_FreeMemMin)
			m_FreeMemMin = m_FreeMem;
		
		Block->m_BlockLink.ClearFree();;
		Block->m_BlockLink.ClearDebug();

	#ifdef DA__HEAPVALIDATE
		if (m_bValidateHeap)
		ValidateHeaps(this);
	#endif

	#ifdef M_SUPPORTMEMORYDEBUG
		if (m_bDebugMemory)
			CheckMemoryToBe((void *)((mint)Block + sizeof(SDA_DefraggableFree)), 0xdd, MemorySize((void *)((mint)Block + sizeof(SDA_Defraggable))) - (sizeof(SDA_DefraggableFree) - sizeof(SDA_Defraggable)), "Freed memory was overwritten", true);
	#endif
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
		RDSequnce = gf_RDGetSequence();
#endif

		pRet = (void *)((mint)Block + sizeof(SDA_Defraggable));
	}

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	{
		mint BlockSize = GetBlockSize(BlockFromMem(pRet));
		uint16 BlockType = _NORMAL_BLOCK;
	#ifdef M_SUPPORTMEMORYDEBUG
		if(BlockFromMem(pRet)->IsDebug())
			BlockType	= ((SDA_DefraggableDebug_Post *)((mint)BlockFromMem(pRet) + BlockSize - sizeof(SDA_DefraggableDebug_Post)))->m_BlockType;
	#endif
		gf_RDSendHeapAlloc(pRet, BlockSize, this, RDSequnce, BlockType);
	}
#endif

	return pRet;
}

#include "MRTC_CallGraph.h"

#ifdef M_SUPPORTMEMORYDEBUG
void *CDA_MemoryManager::AllocDebugImp(mint Size, mint _Alignment, uint16 BlockType, const char *File, int Line)
{
//	return AllocImp(Size, _Alignment);
#ifdef MRTC_DEFAULTMAINHEAP
	if (m_bUseDefaultMainHeap)
	{
		MRTC_SystemInfo::OS_Assert("Cannot alloc aligned wth default heap");
		return NULL;
	}
#endif
	// Align to alloc granularity (4 uint8s default) (4 uint8s minimum) (power of 2)
	M_ASSERT(m_AlignAlloc >= 4, "Alloc granularity is too small");

	mint Alignment = _Alignment;
	if (Alignment < m_AlignAlloc)
		Alignment = m_AlignAlloc;

	mint SizeNeeded = Size + sizeof(SDA_DefraggableDebug);
	mint RealSizeNeeded = SizeNeeded;
	if (Alignment > m_AlignAlloc)
	{
		SizeNeeded += sizeof(SDA_DefraggableFree); // We can include a free block before (size is aligned on m_AlignAlloc)
		SizeNeeded += Alignment - m_AlignAlloc; // Be able to align to our alignment
	}

	SizeNeeded = ((SizeNeeded + (m_AlignAlloc - 1)) & (~(m_AlignAlloc - 1)));

	RealSizeNeeded = ((RealSizeNeeded + (m_AlignAlloc - 1)) & (~(m_AlignAlloc - 1)));

	if (SizeNeeded < sizeof(SDA_DefraggableFree))
		SizeNeeded = sizeof(SDA_DefraggableFree);
	if (RealSizeNeeded < sizeof(SDA_DefraggableFree))
		RealSizeNeeded = sizeof(SDA_DefraggableFree);

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	uint64 RDSequnce = 0;
#endif

	void *pRet;
	{
		M_LOCK(m_Lock);

	#ifdef DA__HEAPVALIDATE
		if (m_bValidateHeap)
			ValidateHeaps(this);
	#endif

		CDA_MemoryManager_SizeClass* SizeClass = GetSizeClass(SizeNeeded);

		if (!SizeClass)
			return NULL;

		M_ASSERT(SizeClass->m_FirstFreeBlock.m_pNextFree, "Heap search error");

		SDA_DefraggableFree *Block = (SDA_DefraggableFree *)((uint8 *)SizeClass->m_FirstFreeBlock.m_pNextFree - sizeof(SDA_Defraggable));

		mint SizeClassSize = SizeClass->m_Size;

		Block->m_FreeLink.Remove();
		Block->m_BlockLink.ClearFree();

		mint LeftOverSize = 0;
		mint FinalBlockSize = SizeClassSize;

		if (Alignment > m_AlignAlloc)
		{
			// Insert a free block before our block and adjust Block
			mint BlockAddress = (mint)(Block);
			mint EndAddress = BlockAddress + sizeof(SDA_DefraggableFree) + sizeof(SDA_Defraggable) + sizeof(SDA_DefraggableDebug_Pre);

			mint AlignedEndAddress = (EndAddress + (Alignment - 1)) & (~(Alignment - 1)); // Align

			mint StartDefraggable = AlignedEndAddress - (sizeof(SDA_Defraggable) + sizeof(SDA_DefraggableDebug_Pre));
			mint PreBlockSize = StartDefraggable - BlockAddress;

			if (PreBlockSize)
			{
				FinalBlockSize -= PreBlockSize;
				CDA_MemoryManager_SizeClass *LeftOverSizeClass = GetFreeSizeClass(PreBlockSize, false);

				SDA_DefraggableFree *NewBlock = Block;
				SDA_DefraggableFree *NextBlock = (SDA_DefraggableFree *)StartDefraggable;


				*((SDA_Defraggable *)NextBlock) = *((SDA_Defraggable *)NewBlock);

				NewBlock->m_BlockLink.SetFree();
				NewBlock->m_BlockLink.ClearDebug();
				
				NextBlock->m_BlockLink.m_pPrevBlockSet(&NewBlock->m_BlockLink);
				NewBlock->m_BlockLink.m_pNextBlock = &NextBlock->m_BlockLink;
				if (NextBlock->m_BlockLink.m_pNextBlock)
					NextBlock->m_BlockLink.m_pNextBlock->m_pPrevBlockSet(&NextBlock->m_BlockLink);

				NewBlock->m_FreeLink.AddFirst(LeftOverSizeClass);

				Block = NextBlock;
			}
		}

		LeftOverSize = FinalBlockSize - RealSizeNeeded;

		if (LeftOverSize > sizeof(SDA_DefraggableFree))
		{
			FinalBlockSize -= LeftOverSize;

			CDA_MemoryManager_SizeClass *LeftOverSizeClass = GetFreeSizeClass(LeftOverSize, false);

			SDA_DefraggableFree *NewBlock = (SDA_DefraggableFree *)(((uint8 *)Block) + FinalBlockSize);

			NewBlock->m_BlockLink.SetFree();
			NewBlock->m_BlockLink.ClearDebug();

			if (Block->m_BlockLink.m_pNextBlock)
				Block->m_BlockLink.m_pNextBlock->m_pPrevBlockSet(&NewBlock->m_BlockLink);

			NewBlock->m_BlockLink.m_pNextBlock = Block->m_BlockLink.m_pNextBlock;
			Block->m_BlockLink.m_pNextBlock = &NewBlock->m_BlockLink;
			NewBlock->m_BlockLink.m_pPrevBlockSet(&Block->m_BlockLink);		

			NewBlock->m_FreeLink.AddFirst(LeftOverSizeClass);
			
		}

		m_FreeMem -= FinalBlockSize;
		if (m_FreeMem < m_FreeMemMin)
			m_FreeMemMin = m_FreeMem;
		
		Block->m_BlockLink.SetDebug();

		SDA_DefraggableDebug_Pre *Pre = (SDA_DefraggableDebug_Pre *)((mint)Block + sizeof(SDA_Defraggable));
		SDA_DefraggableDebug_Post *Post = (SDA_DefraggableDebug_Post *)((mint)Block + FinalBlockSize - sizeof(SDA_DefraggableDebug_Post));

	#ifdef M_SUPPORTMEMORYDEBUG
		if (m_bDebugMemory)
		{
			CheckMemoryToBe((void *)((mint)Block + sizeof(SDA_DefraggableFree)), 0xdd, FinalBlockSize - sizeof(SDA_DefraggableFree), "Freed memory was overwritten", true);
			memset(Pre, 0xFD, FinalBlockSize - sizeof(SDA_Defraggable));
		}
	#endif
		Post->m_Line = Line;
		Post->m_pFile = File;
		Post->m_BlockType = BlockType;
	#ifndef PLATFORM_XBOX1
		Post->m_SequenceId = m_SequenceMark++;
	#endif

	#ifdef M_SUPPORTMEMORYDEBUG
		if (m_bDebugMemory && BlockType != 5 && (MRTC_GetObjectManager()->InMainThread()))
		{
			if (m_bShowAllocs)
			{
//				CFStr Class = "Unknown";
				const char* pClass = "Unknown";

	#ifdef MRTC_ENABLE_MSCOPE
				MRTC_ObjectManager* pManager = MRTC_GetObjectManager();
				
				if (pManager)
				{				
					MRTC_ContextStack *pContextStack = pManager->GetContexctStack().Get();
					for (int i = pContextStack->m_iContext - 1; i >= 0; --i)
					{
						if (pContextStack->m_lpContexts[i]->m_pMemoryCategory)
						{
							pClass = pContextStack->m_lpContexts[i]->m_pMemoryCategory;
							break;
						}
						
					}	
				}
	#endif

				if (m_ShowAllocContext == pClass)
				{
					M_TRACEALWAYS("%s(%d) : %s alloced %d bytes.\n", File, Line, pClass, Size);
				}
			}

			MemTracking_TrackAlloc(Post->m_TrackingPair, FinalBlockSize);
		}
		else
			Post->m_TrackingPair.Clear();
	#else
			Post->m_TrackingPair.Clear();
	#endif

		
		if (BlockType != 5 && File)
		{
			CDA_MemoryManager_DebugAllocFile *pFileMap = m_DebugStringTree.FindEqual(File);
			if (!pFileMap)
			{
				pFileMap = m_DebugStringsPool.New();
				strncpy(pFileMap->m_FileName, File, sizeof(pFileMap->m_FileName));
				pFileMap->m_pOriginalPtr = File;
				m_DebugStringTree.f_Insert(pFileMap);
			}
		}
		else
		{
			Post->m_pFile = NULL;
		}
		
		((SDA_Defraggable *)((mint)Pre + sizeof(SDA_DefraggableDebug_Pre) - sizeof(SDA_Defraggable)))->SetDebug();
		((SDA_Defraggable *)((mint)Pre + sizeof(SDA_DefraggableDebug_Pre) - sizeof(SDA_Defraggable)))->ClearFree();


#ifdef MRTC_ENABLE_REMOTEDEBUGGER
		RDSequnce = gf_RDGetSequence();
#endif

		pRet = (void *)((mint)Pre + sizeof(SDA_DefraggableDebug_Pre));
	}

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	{
		mint BlockSize = GetBlockSize(BlockFromMem(pRet));
		uint16 BlockType = _NORMAL_BLOCK;
		if(BlockFromMem(pRet)->IsDebug())
			BlockType	= ((SDA_DefraggableDebug_Post *)((mint)BlockFromMem(pRet) + BlockSize - sizeof(SDA_DefraggableDebug_Post)))->m_BlockType;
		gf_RDSendHeapAlloc(pRet, BlockSize, this, RDSequnce, BlockType);
	}
#endif

	return pRet;
}
#endif

void *CDA_MemoryManager::AllocAlign(mint Size, mint _Alignment)
{
#ifdef M_SUPPORTMEMORYDEBUG
		M_ASSERT(0, "!");
#endif

#ifdef MRTC_DEFAULTMAINHEAP
	if (m_bUseDefaultMainHeap)
	{
#ifdef BROKEN_BLOCK_COUNTER
		g_nBlocks++;
		if ((g_nBlocks & 0xfff) == 0)
		{
			M_TRACEALWAYS("(MRTC) nBlocks %d\n", g_nBlocks);
		}
#endif
		void *pMem = MRTC_SystemInfo::OS_HeapAllocAlign(Size, _Alignment);
//		int FreeMem = MRTC_SystemInfo::OS_PhysicalMemoryFree();
//		if (FreeMem < m_FreeMemMin)
//			m_FreeMemMin = FreeMem;

#ifdef PLATFORM_DOLPHIN
		if (!pMem)
		{
			uint32 Largest = MRTC_SystemInfo::OS_PhysicalMemoryLargestFree();
			CInfoScreen_GC& IS = CInfoScreen_GC::Get();
			IS.Activate();
			static char buf[128];
			snprintf(buf, 128, "Out of memory!\nTried to allocate %d bytes.\nAvailable memory: %d bytes.\nLargest free block: %d bytes.", Size, FreeMem, Largest);
			buf[127] = 0;
			IS.OutputText(buf);
			OSHalt("OutOfMemory");
		}
#elif defined( PLATFORM_PS2 )
		if( !pMem )
		{
			const uint32 FreeMem = MRTC_SystemInfo::OS_PhysicalMemoryFree();
			const uint32 LargestFree = MRTC_SystemInfo::OS_PhysicalMemoryLargestFree();
			char aBuf[512];
			sprintf( aBuf,
				"Out of memory!\n"
				"Tried to allocate %d bytes\n"
				"Available mrmory: %d bytes\n"
				"Largest free block: %d bytes", Size, FreeMem, LargestFree );
			scePrintf( aBuf );
			CInfoScreenPS2::Get().OutputText( aBuf );
			while( 1 ) {}
		}
#elif defined(PLATFORM_XENON)
		if (!pMem)
		{
			uint FreeMem = MRTC_SystemInfo::OS_PhysicalMemoryFree();
			uint LargestFree = MRTC_SystemInfo::OS_PhysicalMemoryLargestFree();
			M_TRACEALWAYS("OUT OF MAIN MEMORY!\n"
			              " - Tried to allocate %d KiB\n"
			              " - Free mem: %d KiB\n"
			              " - Largest free block: %d KiB\n", Size >> 10, FreeMem >> 10, LargestFree >> 10);
			M_BREAKPOINT;
		}
#endif

		m_UsedMemory += MemorySize(pMem) + 8;

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
		if (pMem)
			gf_RDSendHeapAlloc(pMem, ((MRTC_SystemInfo::OS_HeapSize(pMem)+8) + 7) & (~7), this, gf_RDGetSequence(), _NORMAL_BLOCK);
#endif

		return pMem;
	}
#endif

#ifdef M_SUPPORTMEMORYDEBUG
	if (m_bDebugMemory)
	{
		return AllocDebugImp(Size, _Alignment, _NORMAL_BLOCK, NULL, NULL);
	}
#endif

	return AllocImp(Size, _Alignment);
}

void *CDA_MemoryManager::Alloc(mint Size)
{
	if (m_GlobalAlign)
		return AllocAlign(Size, m_GlobalAlign);

#ifdef M_SUPPORTMEMORYDEBUG
		M_ASSERT(0, "!");
#endif

#ifdef MRTC_DEFAULTMAINHEAP
	if (m_bUseDefaultMainHeap)
	{
#ifdef BROKEN_BLOCK_COUNTER
		g_nBlocks++;
		if ((g_nBlocks & 0xfff) == 0)
		{
			M_TRACEALWAYS("(MRTC) nBlocks %d\n", g_nBlocks);
		}
#endif
		void *pMem = MRTC_SystemInfo::OS_HeapAlloc(Size);
//		int FreeMem = MRTC_SystemInfo::OS_PhysicalMemoryFree();
//		if (FreeMem < m_FreeMemMin)
//			m_FreeMemMin = FreeMem;

#ifdef PLATFORM_DOLPHIN
		if (!pMem)
		{
			uint32 Largest = MRTC_SystemInfo::OS_PhysicalMemoryLargestFree();
			CInfoScreen_GC& IS = CInfoScreen_GC::Get();
			IS.Activate();
			static char buf[128];
			snprintf(buf, 128, "Out of memory!\nTried to allocate %d bytes.\nAvailable memory: %d bytes.\nLargest free block: %d bytes.", Size, FreeMem, Largest);
			buf[127] = 0;
			IS.OutputText(buf);
			OSHalt("OutOfMemory");
		}
#elif defined( PLATFORM_PS2 )
		if( !pMem )
		{
			const uint32 FreeMem = MRTC_SystemInfo::OS_PhysicalMemoryFree();
			const uint32 LargestFree = MRTC_SystemInfo::OS_PhysicalMemoryLargestFree();
			char aBuf[512];
			sprintf( aBuf,
				"Out of memory!\n"
				"Tried to allocate %d bytes\n"
				"Available mrmory: %d bytes\n"
				"Largest free block: %d bytes", Size, FreeMem, LargestFree );
			scePrintf( aBuf );
			CInfoScreenPS2::Get().OutputText( aBuf );
			while( 1 ) {}
		}
#elif defined(PLATFORM_XENON)
		if (!pMem)
		{
			const uint32 FreeMem = MRTC_SystemInfo::OS_PhysicalMemoryFree();
			const uint32 LargestFree = MRTC_SystemInfo::OS_PhysicalMemoryLargestFree();
			M_TRACEALWAYS("Out of memory!\n"
			              "Tried to allocate %d bytes\n"
			              "Available mrmory: %d bytes\n"
			              "Largest free block: %d bytes", Size, FreeMem, LargestFree);
			M_BREAKPOINT;
		}
#endif

		m_UsedMemory += MemorySize(pMem) + 8;

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
		if (pMem)
			gf_RDSendHeapAlloc(pMem, ((MRTC_SystemInfo::OS_HeapSize(pMem)+8) + 7) & (~7), this, gf_RDGetSequence(), _NORMAL_BLOCK);
#endif

		return pMem;
	}
#endif

#ifdef M_SUPPORTMEMORYDEBUG
	if (m_bDebugMemory)
	{
		return AllocDebugImp(Size, m_AlignAlloc, _NORMAL_BLOCK, NULL, NULL);
	}
#endif

	return AllocImp(Size, m_AlignAlloc);
}

#ifdef M_SUPPORTMEMORYDEBUG

void *CDA_MemoryManager::AllocDebugAlign(mint _Size, mint _Align, uint16 _BlockType, const char *File, int Line)
{
#ifdef MRTC_DEFAULTMAINHEAP
	if (m_bUseDefaultMainHeap)
	{
		void *pMem = MRTC_SystemInfo::OS_HeapAllocAlign(Size, _Align);

		m_UsedMemory += MemorySize(pMem) + 8;

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
		gf_RDSendHeapAlloc(pMem, ((MRTC_SystemInfo::OS_HeapSize(pMem)+8) + 7) & (~7), this, gf_RDGetSequence(), _NORMAL_BLOCK);
#endif
		return pMem;
	}
#endif

	return AllocDebugImp(_Size, _Align, _BlockType, File, Line);
}

void *CDA_MemoryManager::AllocDebug(mint Size, uint16 _BlockType, const char *File, int Line)
{
	if (m_GlobalAlign)
		return AllocDebugAlign(Size, m_GlobalAlign, _BlockType, File, Line);

#ifdef MRTC_DEFAULTMAINHEAP
	if (m_bUseDefaultMainHeap)
	{
		void *pMem = MRTC_SystemInfo::OS_HeapAlloc(Size);
//		int FreeMem = MRTC_SystemInfo::OS_PhysicalMemoryFree();
//		if (FreeMem < m_FreeMemMin)
//			m_FreeMemMin = FreeMem;

		m_UsedMemory += MemorySize(pMem) + 8;

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
		gf_RDSendHeapAlloc(pMem, ((MRTC_SystemInfo::OS_HeapSize(pMem)+8) + 7) & (~7), this, gf_RDGetSequence(), _NORMAL_BLOCK);
#endif
		return pMem;
	}
#endif
//	if (m_MemTracking_bOn)
//		M_TRACEALWAYS("%s(%d) : Alloc %d bytes.\n", File, Line, Size);

	return AllocDebugImp(Size, m_AlignAlloc, _BlockType, File, Line);
}
#endif


CDA_MemoryManager_HeapChunk *CDA_MemoryManager::GetChunkFromMem(void *Mem)
{
	CDA_MemoryManager_HeapChunk *Chunk;
	
	Chunk = (CDA_MemoryManager_HeapChunk *)m_ChunksTree.FindLargestLessThanEqual(Mem);
	
	M_ASSERT(Chunk && Mem < Chunk->m_pHeapEnd, "Memory manager error, Could not find chunk memory belong to");	

	return Chunk;
}

mint CDA_MemoryManager::GetBlockSize(SDA_Defraggable *Block)
{
	if (Block->m_pNextBlock)
	{
		mint Size = (mint)Block->m_pNextBlock - (mint)Block;
		return Size;
	}
	else
	{
		M_LOCK(m_Lock);

		mint Size = (mint)GetChunkFromMem(Block)->m_pHeapEnd - (mint)Block;
		return Size;		
	}

}

mint CDA_MemoryManager::GetFreeMem()
{

#ifdef MRTC_DEFAULTMAINHEAP
	if (m_bUseDefaultMainHeap)
	{
#ifdef PLATFORM_WIN_PC
		return -m_UsedMemory;
#else
		return MRTC_SystemInfo::OS_PhysicalMemoryFree();
#endif
	}
#endif

	M_LOCK(m_Lock);

	mint Return = m_FreeMem;


	return Return;

}

mint CDA_MemoryManager::GetMinFreeMem()
{
/*#ifdef MRTC_DEFAULTMAINHEAP
	if (m_bUseDefaultMainHeap)
	{
		return MRTC_SystemInfo::OS_PhysicalMemoryFree();
	}
#endif*/

	M_LOCK(m_Lock);

	mint Return = m_FreeMemMin;

#ifdef MRTC_DEFAULTMAINHEAP
	if (m_bUseDefaultMainHeap)
	{
		m_FreeMemMin = MRTC_SystemInfo::OS_PhysicalMemoryFree();
	}
#endif

	if (m_FreeMem < Return)
		m_FreeMem = Return;

	return Return;

}

mint CDA_MemoryManager::GetUsedMem()
{
#ifdef MRTC_DEFAULTMAINHEAP
	if (m_bUseDefaultMainHeap)
	{
#ifdef PLATFORM_WIN_PC
		return m_UsedMemory;
#else
		return MRTC_SystemInfo::OS_PhysicalMemoryUsed();
#endif
	}
#endif

	M_LOCK(m_Lock);

	mint Return = m_AllocatedMem - m_FreeMem;


	return Return;

}

mint CDA_MemoryManager::GetLargestFreeMem()
{
#ifdef MRTC_DEFAULTMAINHEAP
	if (m_bUseDefaultMainHeap)
	{
		return MRTC_SystemInfo::OS_PhysicalMemoryLargestFree();
	}
#endif
	M_LOCK(m_Lock);

	CDA_MemoryManager_SizeClass *SizeClassNormal = (CDA_MemoryManager_SizeClass *)(m_SizesFreeTreeNormal.FindLargest());
	CDA_MemoryManager_SizeClass *SizeClassFragment = (CDA_MemoryManager_SizeClass *)(m_SizesFreeTreeFragments.FindLargest());

	mint SizeNormal = SizeClassNormal ? SizeClassNormal->m_Size : 0;
	mint SizeFragment = SizeClassFragment ? SizeClassFragment->m_Size : 0;

	return Max(SizeNormal, SizeFragment);
}

void CDA_MemoryManager::Free(void *_Block)
{
#ifdef MRTC_DEFAULTMAINHEAP
	if (m_bUseDefaultMainHeap)
	{
		if (_Block)
		{
			m_UsedMemory -= MemorySize(_Block) + 8;
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
			gf_RDSendHeapFree(_Block, this, gf_RDGetSequence());
#endif
		}

#ifdef BROKEN_BLOCK_COUNTER
		g_nBlocks--;
#endif

		MRTC_SystemInfo::OS_HeapFree(_Block);
		return;
	}
#endif
	
	if (!_Block)
		return;

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	gf_RDSendHeapFree(_Block, this, gf_RDGetSequence());
#endif
	
	M_LOCK(m_Lock);

#ifdef DA__HEAPVALIDATE
	if (m_bValidateHeap)
	ValidateHeaps(this);
#endif
	
	SDA_Defraggable *Block;
	SDA_Defraggable *StartBlock;
	SDA_Defraggable *EndBlock;
	EndBlock = StartBlock = Block = BlockFromMem(_Block);

	mint BlockSize = GetBlockSize(StartBlock);

#ifdef M_SUPPORTMEMORYDEBUG
	if (m_bDebugMemory)
	{
		if (Block->IsDebug())
		{
			SDA_DefraggableDebug_Pre *Pre = (SDA_DefraggableDebug_Pre *)((mint)Block + sizeof(SDA_Defraggable));
			SDA_DefraggableDebug_Post *Post = (SDA_DefraggableDebug_Post *)((mint)Block + BlockSize - sizeof(SDA_DefraggableDebug_Post));
#if defined(PLATFORM_XENON) || defined(PLATFORM_PS3)
			CheckMemoryToBe(Pre->m_PreBlockCheck, 0xFD, 7, "Damage at start of block when freeing", true);
			CheckMemoryToBe(Pre->m_PreBlockCheck + 7, 0xFE, 1, "Damage at start of block when freeing", true);
			CheckMemoryToBe(Pre->m_PreBlockCheck + 8, 0xFD, MDA_DEBUG_BLOCK_PRECHECK - 8, "Damage at start of block when freeing", true);
//			CheckMemoryToBe(((uint8*)Pre->m_PreBlockCheck) + MDA_DEBUG_BLOCK_PRECHECK - 1, 0xFE, 1, "Damage at start of block when freeing", true);
//			CheckMemoryToBe(((uint8*)Pre->m_PreBlockCheck) + MDA_DEBUG_BLOCK_PRECHECK - 1, 0xbd, 1, "Damage at start of block when freeing", true);
#else

			CheckMemoryToBe(Pre->m_PreBlockCheck, 0xFD, MDA_DEBUG_BLOCK_PRECHECK - 1, "Damage at start of block when freeing", true);
#ifdef CPU_BIGENDIAN
//			CheckMemoryToBe(((uint8*)Pre->m_PreBlockCheck) + MDA_DEBUG_BLOCK_PRECHECK - 1, 0xFE, 1, "Damage at start of block when freeing", true);
#else
//			CheckMemoryToBe(((uint8*)Pre->m_PreBlockCheck) + MDA_DEBUG_BLOCK_PRECHECK - 1, 0xbd, 1, "Damage at start of block when freeing", true);
#endif
#endif
			CheckMemoryToBe(Post->m_PostBlockCheck, 0xFD, MDA_DEBUG_BLOCK_POSTCHECK, "Damage at end of block when freeing", true);
	
			MemTracking_TrackDelete(Post->m_TrackingPair, BlockSize);

/*			if (m_MemTracking_bOn && Post && Post->m_pFile)
			{
				CDA_MemoryManager_DebugAllocFile *FileMap = m_DebugStringsHash.GetByID(&Post->m_pFile);
				if (FileMap)
					M_TRACEALWAYS("%s(%d) : Free %d bytes.\n", FileMap->m_FileName, Post->m_Line, BlockSize);
			}*/
		}
		
		memset((SDA_DefraggableFree *)Block + 1, 0xdd, GetBlockSize(StartBlock) - sizeof(SDA_DefraggableFree));
	}
#endif

	m_FreeMem += BlockSize;

	if (Block->m_pPrevBlockGet())
	{
		if (Block->m_pPrevBlockGet()->IsFree())
		{
			StartBlock = Block->m_pPrevBlockGet();
			((SDA_DefraggableFree *)StartBlock)->m_FreeLink.Remove();
		}
	}

	if (Block->m_pNextBlock)
	{
		if (Block->m_pNextBlock->IsFree())
		{
			EndBlock = Block->m_pNextBlock;
			((SDA_DefraggableFree *)EndBlock)->m_FreeLink.Remove();
		}
	}



	if (StartBlock != EndBlock)
	{
		// Knit together
		StartBlock->m_pNextBlock = EndBlock->m_pNextBlock;
		if (StartBlock->m_pNextBlock)
			StartBlock->m_pNextBlock->m_pPrevBlockSet(StartBlock);

#ifdef M_SUPPORTMEMORYDEBUG
		if (m_bDebugMemory)
		{
			if (StartBlock != Block)
			{
				memset(Block, 0xdd, sizeof(SDA_DefraggableFree));
			}
			
			if (EndBlock != Block)
			{
				memset(EndBlock, 0xdd, sizeof(SDA_DefraggableFree));
			}
		}
#endif
	}

/*
	if (m_bDynamicSize && !StartBlock->m_pNextBlock && !StartBlock->m_Pointer)
	{
		// Whole Block is free free the memory for good
		delete GetChunkFromMem(_Block);
		ValidateHeaps(this);

		return;
	}
*/
	((SDA_DefraggableFree *)StartBlock)->m_BlockLink.SetFree();
	((SDA_DefraggableFree *)StartBlock)->m_BlockLink.ClearDebug();
	((SDA_DefraggableFree *)StartBlock)->m_FreeLink.AddFirst(GetFreeSizeClass(GetBlockSize(StartBlock), false));


#ifdef DA__HEAPVALIDATE
	if (m_bValidateHeap)
	ValidateHeaps(this);
#endif

}

void *CDA_MemoryManager::ReallocAlign(void *_Block, mint _Size, mint _Alignment)
{
#ifdef MRTC_DEFAULTMAINHEAP
	if (m_bUseDefaultMainHeap)
	{
		if (_Block)
		{
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
			gf_RDSendHeapFree(_Block, this, gf_RDGetSequence());
#endif
			m_UsedMemory -= MemorySize(_Block) + 8;
		}

		void *pMem = MRTC_SystemInfo::OS_HeapReallocAlign(_Block, _Size, _Alignment);

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
		if (pMem)
			gf_RDSendHeapAlloc(pMem, ((MRTC_SystemInfo::OS_HeapSize(pMem)+8) + 7) & (~7), this, gf_RDGetSequence(), _NORMAL_BLOCK);
#endif

//		int FreeMem = MRTC_SystemInfo::OS_PhysicalMemoryFree();
//		if (FreeMem < m_FreeMemMin)
//			m_FreeMemMin = FreeMem;
		m_UsedMemory += MemorySize(pMem) + 8;
		return pMem;
	}
#endif

	if (!_Size)
	{
		Free(_Block);

		return NULL;
	}
		
	if (!_Block)
	{
		return AllocAlign(_Size, _Alignment);
	}

	void *NewBlock = AllocAlign(_Size, _Alignment);

	if (_Block && NewBlock)
	{
		mint OldSize = MemorySize(_Block);
		memcpy(NewBlock, _Block, Min(OldSize, _Size));
	}

	Free(_Block);

	return NewBlock;
}

void *CDA_MemoryManager::Realloc(void *_Block, mint _Size)
{
	if (m_GlobalAlign)
		return ReallocAlign(_Block, _Size, m_GlobalAlign);
#ifdef MRTC_DEFAULTMAINHEAP
	if (m_bUseDefaultMainHeap)
	{
		if (_Block)
		{
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
			gf_RDSendHeapFree(_Block, this, gf_RDGetSequence());
#endif
			m_UsedMemory -= MemorySize(_Block) + 8;
		}

		void *pMem = MRTC_SystemInfo::OS_HeapRealloc(_Block, _Size);

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
		if (pMem)
			gf_RDSendHeapAlloc(pMem, ((MRTC_SystemInfo::OS_HeapSize(pMem)+8) + 7) & (~7), this, gf_RDGetSequence(), _NORMAL_BLOCK);
#endif

//		int FreeMem = MRTC_SystemInfo::OS_PhysicalMemoryFree();
//		if (FreeMem < m_FreeMemMin)
//			m_FreeMemMin = FreeMem;
		m_UsedMemory += MemorySize(pMem) + 8;
		return pMem;
	}
#endif

	if (!_Size)
	{
		Free(_Block);

		return NULL;
	}
		
	if (!_Block)
	{
		return Alloc(_Size);
	}

	void *NewBlock = Alloc(_Size);

	if (_Block && NewBlock)
	{
		mint OldSize = MemorySize(_Block);
		memcpy(NewBlock, _Block, Min(OldSize, _Size));
	}

	Free(_Block);

	return NewBlock;
}

#ifdef M_SUPPORTMEMORYDEBUG

void *CDA_MemoryManager::ReallocDebugAlign(void *_Block, mint _Size, mint _Align, uint16 _BlockType, const char *File, int Line)
{
#ifdef MRTC_DEFAULTMAINHEAP
	if (m_bUseDefaultMainHeap)
	{
		if (_Block)
		{
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
			gf_RDSendHeapFree(_Block, this, gf_RDGetSequence());
#endif
			m_UsedMemory -= MemorySize(_Block) + 8;
		}

		void *pMem = MRTC_SystemInfo::OS_HeapReallocAlign(_Block, _Size, _Align);

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
		if (pMem)
			gf_RDSendHeapAlloc(pMem, ((MRTC_SystemInfo::OS_HeapSize(pMem)+8) + 7) & (~7), this, gf_RDGetSequence(), _NORMAL_BLOCK);
#endif
		m_UsedMemory += MemorySize(pMem) + 8;
		return pMem;
	}
#endif

	if (!_Size)
	{
		Free(_Block);

		return NULL;
	}
		
	if (!_Block)
	{
		return AllocDebugAlign(_Size, _Align, _BlockType, File, Line);
	}

	void *NewBlock = AllocDebugAlign(_Size, _Align, _BlockType, File, Line);

	if (_Block && NewBlock)
	{
		mint OldSize = MemorySize(_Block);
		memcpy(NewBlock, _Block, Min(OldSize, _Size));
	}

	Free(_Block);

	return NewBlock;
}

void *CDA_MemoryManager::ReallocDebug(void *_Block, mint _Size, uint16 _BlockType, const char *File, int Line)
{
	if (m_GlobalAlign)
		return ReallocDebugAlign(_Block, _Size, m_GlobalAlign, _BlockType, File, Line);

#ifdef MRTC_DEFAULTMAINHEAP
	if (m_bUseDefaultMainHeap)
	{
		if (_Block)
		{
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
			gf_RDSendHeapFree(_Block, this, gf_RDGetSequence());
#endif
			m_UsedMemory -= MemorySize(_Block) + 8;
		}

		void *pMem = MRTC_SystemInfo::OS_HeapRealloc(_Block, _Size);

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
		if (pMem)
			gf_RDSendHeapAlloc(pMem, ((MRTC_SystemInfo::OS_HeapSize(pMem)+8) + 7) & (~7), this, gf_RDGetSequence(), _NORMAL_BLOCK);
#endif
//		int FreeMem = MRTC_SystemInfo::OS_PhysicalMemoryFree();
//		if (FreeMem < m_FreeMemMin)
//			m_FreeMemMin = FreeMem;
		m_UsedMemory += MemorySize(pMem) + 8;
		return pMem;
	}
#endif

	if (!_Size)
	{
		Free(_Block);

		return NULL;
	}
		
	if (!_Block)
	{
		return AllocDebug(_Size, _BlockType, File, Line);
	}

	void *NewBlock = AllocDebug(_Size, _BlockType, File, Line);

	if (_Block && NewBlock)
	{
		mint OldSize = MemorySize(_Block);
		memcpy(NewBlock, _Block, Min(OldSize, _Size));
	}

	Free(_Block);

	return NewBlock;
}

#endif

mint CDA_MemoryManager::MemorySize(const void *_Block)
{

#ifdef MRTC_DEFAULTMAINHEAP
	if (m_bUseDefaultMainHeap)
	{
		if (_Block)			
			return MRTC_SystemInfo::OS_HeapSize(_Block);
		else
			return NULL;
	}
#endif

//	M_LOCK(m_Lock);

	SDA_Defraggable *Block;
	Block = BlockFromMem(_Block);

#ifdef M_SUPPORTMEMORYDEBUG
	if (Block->IsDebug())
		return GetBlockSize(Block) - sizeof(SDA_DefraggableDebug);
	else
#endif
		return GetBlockSize(Block) - ((mint)_Block - (mint)Block);
}

mint CDA_MemoryManager::UserBlockSize(SDA_Defraggable *_Block)
{
	M_LOCK(m_Lock);

#ifdef M_SUPPORTMEMORYDEBUG
	if (_Block->IsDebug())
	{
		return GetBlockSize(_Block) - sizeof(SDA_DefraggableDebug);
	}
	else
#endif
	if (_Block->IsFree())
	{
		return GetBlockSize(_Block) - sizeof(SDA_DefraggableFree);
	}
	else
	{
		return GetBlockSize(_Block) - sizeof(SDA_DefraggableRelease);
	}
}

CDA_MemoryManager_SizeClass* CDA_MemoryManager::GetSizeClass(mint _SizeNeeded)
{
	CDA_MemoryManager_SizeClass *SizeClass = (CDA_MemoryManager_SizeClass *)m_SizesFreeTreeNormal.FindSmallestGreaterThanEqual(_SizeNeeded);

	if (!SizeClass)
	{
		SizeClass = (CDA_MemoryManager_SizeClass *)m_SizesFreeTreeFragments.FindSmallestGreaterThanEqual(_SizeNeeded);

		if (SizeClass)
			return SizeClass;
	
		if (m_bCanDefrag)
			// The memory is defragmented, must defrag to fit element
			Defrag(false);
		else if (m_bDynamicSize)
		{
			CDA_MemoryManager_HeapChunk* pHeap = NULL;


			// Alloc new block
			if (_SizeNeeded > m_DynamicGrowSize || _SizeNeeded > 32768)
			{
				mint Size = ((_SizeNeeded + (4095))) & (~(4095));
				pHeap = m_ChunksPool.New(this, Size);
			}
			else
				pHeap = m_ChunksPool.New(this, m_DynamicGrowSize);

			if (!pHeap || !pHeap->m_pHeap)
			{
				if (pHeap)
				{
					m_ChunksPool.Delete(pHeap);
				}

				M_ASSERT(0, "?");
				return NULL;
			}
		}

		SizeClass = (CDA_MemoryManager_SizeClass *)m_SizesFreeTreeNormal.FindSmallestGreaterThanEqual(_SizeNeeded);
		
		if (!SizeClass)
		{

#if defined(PLATFORM_CONSOLE) || defined(M_SUPPORTMEMORYDEBUG)
			ReportOutOfMemory(_SizeNeeded, this);
#endif
#ifdef M_SUPPORTMEMORYDEBUG

			MemTracking_ReportMemUsage(0);
			MemTracking_ReportMemUsage(1);
#endif
			M_ASSERT(0, "?");
			return NULL;
		}
	
	}

	return SizeClass;
}

#ifdef M_SUPPORTMEMORYDEBUG
void CDA_MemoryManager::DisplayLeaks()
{

	M_LOCK(m_Lock);
	
	SICDA_MemoryManager_HeapChunk Iterator = m_ChunksTree;

	bool FoundLeak = false;
	
	if (m_RunningDebugRuntime)
	{
		
		while (Iterator)
		{
			// Walk block
			
			SDA_Defraggable *CurrentBlock = Iterator->m_pFirstBlock;
			
			while (CurrentBlock)
			{
				if (!CurrentBlock->IsFree())
				{
					if (m_RunningReleaseRuntime)
					{
						// Mixed runtime... we cant know if the crt has left something behind (wihch it has)
						M_TRACEALWAYS("\n");
						M_TRACEALWAYS("\n");
						M_TRACEALWAYS("==========================================================================================\n");
						M_TRACEALWAYS("|    There might be a memory leak, you should compile ALL modules in DEBUG to be sure    |\n");
						M_TRACEALWAYS("==========================================================================================\n");
						M_TRACEALWAYS("\n");
						M_TRACEALWAYS("\n");
						return;
					}
					
					unsigned char *Mem = (unsigned char *)MemFromBlock(CurrentBlock);
					mint Size = UserBlockSize(CurrentBlock);
					SDA_DefraggableDebug_Post *Post = (SDA_DefraggableDebug_Post *)((mint)CurrentBlock + GetBlockSize(CurrentBlock) - sizeof(SDA_DefraggableDebug_Post));

					if (!CurrentBlock->IsDebug() || (Post->m_BlockType != _CRT_BLOCK && Post->m_BlockType != 5))
					{
						// Damn runtime dont clean up
						if (!FoundLeak)
						{
							FoundLeak = true;
							M_TRACEALWAYS("\n");
							M_TRACEALWAYS("\n");
							M_TRACEALWAYS("==========================================================================================\n");
							M_TRACEALWAYS("|                                 MEMORY LEAKS DETECTED                                  |\n");
							M_TRACEALWAYS("==========================================================================================\n");
							M_TRACEALWAYS("\n");
							M_TRACEALWAYS("\n");
//							MRTC_SystemInfo::OS_Sleep(20000);
						}
						
						unsigned char TempStr[1024];
						mint TestSize = Size;
						if (TestSize > 256)
							TestSize = 256;
						unsigned char *Mem = (unsigned char *)MemFromBlock(CurrentBlock);
						
						const char *Class = "unknow class";
						/*
						try
						{
							CObj *Object = TDynamicCast<CObj >((CObj *)Mem);
							if (Object)
								Class = Object->MRTC_ClassName();
						}
						catch(...)
						{
							
						}*/
												
						if (CurrentBlock->IsDebug() && Post->m_pFile)
						{
							CDA_MemoryManager_DebugAllocFile *FileMap = m_DebugStringTree.FindEqual(Post->m_pFile);
							if (FileMap)
							{
								sprintf((char *)TempStr, "%s(%d) : %d leaked uint8s at (0x%08x). Type: %s. First %d uint8s:\n", FileMap->m_FileName, Post->m_Line, (mint)Size, (mint)Mem, Class, (mint)TestSize);
							}
							else
								sprintf((char *)TempStr, "?(?) : %d leaked uint8s at (0x%08x), Type: %s. First %d uint8s:\n", Size, (mint)Mem, Class, TestSize);
						}
						else
						{
							sprintf((char *)TempStr, "?(?) : %d leaked uint8s at (0x%08x), Type: %s. First %d uint8s:\n", Size, (mint)Mem, Class, TestSize);
						}
						M_TRACEALWAYS((char *)TempStr);
						
						M_TRACEALWAYS("<");
						memcpy(TempStr, Mem, TestSize);
						{
							for (int i = 0; i < TestSize; ++i)
							{
								if (TempStr[i] < 32)
									TempStr[i] = 32;
							}
							TempStr[TestSize] = 0;
						}
						
						M_TRACEALWAYS((char *)TempStr);
						M_TRACEALWAYS(">\n");
						{
							int i = 0;
							for (; i < TestSize; ++i)
							{
								unsigned char TempStr2[16];
								sprintf((char *)TempStr2, "%02X", ((unsigned char *)Mem)[i]);
								TempStr[i<<1] = TempStr2[0];
								TempStr[(i<<1) + 1] = TempStr2[1];
							}
							TempStr[(i<<1)] = 0;
						}
						M_TRACEALWAYS((char *)TempStr);
						M_TRACEALWAYS("\n\n");
					}
				}
				
				CurrentBlock = CurrentBlock->m_pNextBlock;
				
			}
			
			++Iterator;
			
		}
		
	}
	
}
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDA_Alignment
|__________________________________________________________________________________________________
\*************************************************************************************************/


CDA_Alignment::CDA_Alignment(int _Alignment, CDA_MemoryManager *_pMemoryManger)
{
	m_pMemoryManger = _pMemoryManger;
	
	if (m_pMemoryManger)
		m_pMemoryManger->SetGlobalAlignment(_Alignment);
	else
		MRTC_GetMemoryManager()->SetGlobalAlignment(_Alignment);
}

CDA_Alignment::~CDA_Alignment()
{
	if (m_pMemoryManger)
		m_pMemoryManger->SetGlobalAlignment();
	else
		MRTC_GetMemoryManager()->SetGlobalAlignment();
	
}

#ifdef M_SUPPORTMEMORYDEBUG
/*

NMemMgr::CMemTrack_Class::CMemTrack_Class()
{

	m_AllocData.m_TimesAlloced = 0;
	m_AllocData.m_TimesDeleted = 0;
	m_AllocData.m_MemUsed = 0;
	m_LastAllocData = m_AllocData;	
	m_pChild = NULL;
}

NMemMgr::CMemTrack_Class::~CMemTrack_Class()
{
	if (m_pChild)
		delete m_pChild;	
}*/

void M_CDECL CDA_MemoryManager::MemTracking_Start()
{
	M_LOCK(m_Lock);

	m_MemTracking_bOn = true;
	m_MemTracking_StartMem = GetUsedMem();

	
}

void M_CDECL CDA_MemoryManager::MemTracking_Stop()
{
	M_LOCK(m_Lock);

	m_MemTracking_bOn = false;

}

static CFStr GetMemStr(mint Mem)
{
	if (Mem < 1000)
	{
		return CFStrF("        %4d",Mem);
	}
	else if (Mem < 1000000)
	{
		return CFStrF("    %4d %.3d",Mem / 1000, Mem % 1000);
	}
	else
	{
		return CFStrF("%4d %.3d %.3d",Mem / 1000000, (Mem / 1000) % 1000, Mem % 1000);
	}
}

void CDA_MemoryManager::ShowAllocs(const char *_pMemoryContext)
{
	M_LOCK(m_Lock);

	m_ShowAllocContext = _pMemoryContext;
	m_bShowAllocs = true;
}

void CDA_MemoryManager::HideAllocs()
{
	M_LOCK(m_Lock);

	m_bShowAllocs = false;
}

void M_CDECL CDA_MemoryManager::MemTracking_Report(bool Verbose)
{
	M_LOCK(m_Lock);

	if (!m_MemTracking_bOn)
		return;
	
	
	NMemMgr::CMemTrack_ClassIter TrackIter = m_MemTracking_ClassesList;
	CFStr OutputString;

	bool First = true;


	while (TrackIter)
	{
//		if (TrackIter2->m_MemUsed)
		if (!TrackIter->m_AllocData.IsSame(TrackIter->m_LastAllocData) && TrackIter->m_ClassName.Compare("IGNORE") != 0)
		{
			if (First)
			{
				M_TRACEALWAYS("___________________________MEMORYTRACK_________________________:\n");
				First = false;
			}

			mint AllocDelta_Mem = TrackIter->m_AllocData.m_MemUsed - TrackIter->m_LastAllocData.m_MemUsed;
			int AllocDelta_NumA = TrackIter->m_AllocData.m_TimesAlloced - TrackIter->m_LastAllocData.m_TimesAlloced;
			int AllocDelta_NumD = TrackIter->m_AllocData.m_TimesDeleted - TrackIter->m_LastAllocData.m_TimesDeleted;
			TrackIter->m_LastAllocData = TrackIter->m_AllocData;
			
/*			const char *Woot = "";

			if (AllocDelta_Mem > 0)
			{
				Woot = "Memory Allocated ";
			}
			else if (AllocDelta_Mem < 0)
			{
				Woot = "Memory Deleted   ";
			}
			else
			{
				Woot = "Memory Activity  ";
			}*/

			if(AllocDelta_Mem != 0)
			{
				OutputString = CFStrF("%-20s, MemDelta: %s b, dAlloc %s = %s -%s, DeltaSinceStart: %s\n", 
					TrackIter->m_ClassName.Str(), 
					GetMemStr(AllocDelta_Mem).Str(), 
					GetMemStr(AllocDelta_NumA - AllocDelta_NumD).Str(), 
					GetMemStr(AllocDelta_NumA).Str(), 
					GetMemStr(AllocDelta_NumD).Str(), 
					GetMemStr(GetUsedMem() - m_MemTracking_StartMem).Str());
			
				M_TRACEALWAYS(OutputString);
			
				if (Verbose && TrackIter->m_pChild)
				{
					NMemMgr::CMemTrack_ClassIter TrackChildIter = TrackIter->m_pChild->m_Children;
				
					while (TrackChildIter)
					{
						if (!TrackChildIter->m_AllocData.IsSame(TrackChildIter->m_LastAllocData))
						{
						
							mint AllocDelta_Mem = TrackChildIter->m_AllocData.m_MemUsed - TrackChildIter->m_LastAllocData.m_MemUsed;
							int AllocDelta_NumA = TrackChildIter->m_AllocData.m_TimesAlloced - TrackChildIter->m_LastAllocData.m_TimesAlloced;
							int AllocDelta_NumD = TrackChildIter->m_AllocData.m_TimesDeleted - TrackChildIter->m_LastAllocData.m_TimesDeleted;
							TrackChildIter->m_LastAllocData = TrackChildIter->m_AllocData;
						
	/*						const char *Woot = "";
						
							if (AllocDelta_Mem > 0)
							{
								Woot = "Memory Allocated ";
							}
							else if (AllocDelta_Mem < 0)
							{
								Woot = "Memory Deleted   ";
							}
							else
							{
								Woot = "Memory Activity  ";
							}*/
						
							if(AllocDelta_Mem != 0)
							{
								OutputString = CFStrF("    %s b, dAllocs %s = %s -%s, %s\n", 
									GetMemStr(AllocDelta_Mem).Str(), 
									GetMemStr(AllocDelta_NumA - AllocDelta_NumD).Str(), 
									GetMemStr(AllocDelta_NumA).Str(), 
									GetMemStr(AllocDelta_NumD).Str(),
									TrackChildIter->m_ClassName.Str()
									);
								M_TRACEALWAYS(OutputString);
							}
						}
							++TrackChildIter;
					}

					M_TRACEALWAYS("\n");
				}

			}
			
		}
		
		++TrackIter;
	}

}

class CMemReportSortEntry
{
public:
	CFStr m_Msg;
	mint m_SortVal;

	int operator> (const CMemReportSortEntry& _E) const
	{
		return int(m_SortVal < _E.m_SortVal);
	}

	int operator< (const CMemReportSortEntry& _E) const
	{
		return int(m_SortVal > _E.m_SortVal);
	}
};

#define MACRO_QSORT(T)												\
static int M_CDECL QSortCompare_##T(const void* _pE1, const void* _pE2)	\
{																	\
	const T& E1 = *((T*)_pE1);										\
	const T& E2 = *((T*)_pE2);										\
																	\
	if (E1 < E2)													\
		return -1;													\
	else if (E1 > E2)												\
		return 1;													\
	return 0;														\
}																	\
																	\
static void QSort_##T(T* _pElem, int _nElem, int _bAscending)				\
{																	\
	qsort((void*) _pElem, _nElem, sizeof(T), QSortCompare_##T);		\
}

MACRO_QSORT(CMemReportSortEntry);



void M_CDECL CDA_MemoryManager::MemTracking_ReportMemUsage(int Verbose)
{
	
	M_LOCK(m_Lock);
	{
		
		static MRTC_CriticalSection LockasFromGCHell;
		
		M_LOCK(LockasFromGCHell);
			
			NMemMgr::CMemTrack_ClassIter TrackIter = m_MemTracking_ClassesList;
		CFStr OutputString;
		
		mint FoundMem = 0;
		M_TRACEALWAYS("Memory usage report:\n\n");
		
		//AR-CHANGE: made static, because CW doesn't like local data > 32KB
		static CMemReportSortEntry lMemReport[128];
		int nMemReport = 0;
		
		while (TrackIter )
		{
			//		if (TrackIter2->m_MemUsed)
//			int AllocDelta_Mem = TrackIter->m_LastAllocData.m_MemUsed - TrackIter->m_AllocData.m_MemUsed;
//			int AllocDelta_NumA = TrackIter->m_LastAllocData.m_TimesAlloced - TrackIter->m_AllocData.m_TimesAlloced;
//			int AllocDelta_NumD = TrackIter->m_LastAllocData.m_TimesDeleted - TrackIter->m_AllocData.m_TimesDeleted;
			
			FoundMem += TrackIter->m_AllocData.m_MemUsed;
			
			OutputString = CFStrF("%-20s, Memused: %s in %d allocations, Activity: %d Allocations, %d Deletions\n", 
				TrackIter->m_ClassName.Str(), 
				GetMemStr(TrackIter->m_AllocData.m_MemUsed).Str(),
				TrackIter->m_AllocData.m_TimesAlloced - TrackIter->m_AllocData.m_TimesDeleted,
				TrackIter->m_AllocData.m_TimesAlloced, 
				TrackIter->m_AllocData.m_TimesDeleted);
			
			if (Verbose)
			{
				M_TRACEALWAYS(OutputString);
			}
			else
			{
				lMemReport[nMemReport].m_Msg = OutputString;
				lMemReport[nMemReport].m_SortVal = TrackIter->m_AllocData.m_MemUsed;
				nMemReport++;
			}
			
			if (Verbose && TrackIter->m_pChild)
			{
				static CMemReportSortEntry lMemReportChild[256];
				int nMemReportChild = 0;
				NMemMgr::CMemTrack_ClassIter TrackChildIter = TrackIter->m_pChild->m_Children;
				
				while (TrackChildIter)
				{
					if (Verbose > 1 || TrackChildIter->m_AllocData.m_MemUsed > 0)
					{
						OutputString = CFStrF("    %7s, %-20s, in %d allocations, Activity: %d Allocations, %d Deletions\n", 
							GetMemStr(TrackChildIter->m_AllocData.m_MemUsed).Str(),
							TrackChildIter->m_ClassName.Str(), 					
							TrackChildIter->m_AllocData.m_TimesAlloced - TrackChildIter->m_AllocData.m_TimesDeleted,
							TrackChildIter->m_AllocData.m_TimesAlloced, 
							TrackChildIter->m_AllocData.m_TimesDeleted);
						
						lMemReportChild[nMemReportChild].m_Msg	= OutputString;
						lMemReportChild[nMemReportChild].m_SortVal	= TrackChildIter->m_AllocData.m_MemUsed;
						nMemReportChild++;
					}
					++TrackChildIter;
				}
				QSort_CMemReportSortEntry( lMemReportChild, nMemReportChild, 0 );
				M_TRACEALWAYS("\n");
				for( int i = 0; i < nMemReportChild; i++ )
				{
					M_TRACEALWAYS( lMemReportChild[i].m_Msg.Str());
				}
				M_TRACEALWAYS("\n");
			}
			
			++TrackIter;
		}
		
		if (!Verbose)
		{
			QSort_CMemReportSortEntry(lMemReport, nMemReport, 0);
			for(int i = 0; i < nMemReport; i++)
			{
				M_TRACEALWAYS(lMemReport[i].m_Msg.Str());
			}
		}
		
		
		OutputString = CFStrF("%-20s, Memused: %s\n", 
			"Other", 
			GetMemStr(GetUsedMem() - FoundMem).Str());
		
		M_TRACEALWAYS(OutputString);
		
		M_TRACEALWAYS("_______________________________________________________________\n");
		
		OutputString = CFStrF("%-20s, Memused: %s\n", 
			"Total", 
			GetMemStr(GetUsedMem()).Str());
		
		M_TRACEALWAYS(OutputString);
	}
	
}


void CDA_MemoryManager::MemTracking_GetMemUsageInfo(TArray<SMemInfo> &_Result, const char* _pClassName)
{
	M_LOCK(m_Lock);
	{
		NMemMgr::CMemTrack_ClassIter TrackIter = m_MemTracking_ClassesList;
		while (TrackIter)
		{
			SMemInfo tmp;
			if (_pClassName == NULL)
			{
				// brief info about each class
				tmp.m_Name        = TrackIter->m_ClassName;
				tmp.m_nAllocBytes = TrackIter->m_AllocData.m_MemUsed;
				tmp.m_nAllocNum   = TrackIter->m_AllocData.m_TimesAlloced - 
				                    TrackIter->m_AllocData.m_TimesDeleted;
				_Result.Add(tmp);
			}
			else if ((TrackIter->m_ClassName == _pClassName) && TrackIter->m_pChild)
			{
				// all info about specified class
				NMemMgr::CMemTrack_ClassIter TrackChildIter = TrackIter->m_pChild->m_Children;
				while (TrackChildIter)
				{
					if (TrackChildIter->m_AllocData.m_MemUsed > 0)
					{
						tmp.m_Name        = TrackChildIter->m_ClassName;
						tmp.m_nAllocBytes = TrackChildIter->m_AllocData.m_MemUsed;
						tmp.m_nAllocNum   = TrackChildIter->m_AllocData.m_TimesAlloced - 
						                    TrackChildIter->m_AllocData.m_TimesDeleted;
						_Result.Add(tmp);
					}
					++TrackChildIter;
				}
				break;
			}
			++TrackIter;
		}
	}
}



void CDA_MemoryManager::MemTracking_TrackAlloc(NMemMgr::CMemTrack_UniquePair &_TrackRet, mint _Size)
{
	
	CFStr Class = "Unknown";
	
#ifdef MRTC_ENABLE_MSCOPE
	MRTC_ObjectManager* pManager = MRTC_GetObjectManager();
	
	if (pManager)
	{
		
		MRTC_ContextStack *pContextStack = pManager->GetContexctStack().Get();
		for (int i = pContextStack->m_iContext - 1; i >= 0; --i)
		{
			if (pContextStack->m_lpContexts[i]->m_pMemoryCategory)
			{
				Class = pContextStack->m_lpContexts[i]->m_pMemoryCategory;
				break;
			}
			
		}	
	}
#endif
	
	if (Class.Compare("IGNORE") != 0)
	{
		// Put breakpoint here to break on allocations
		CFStr BreakHere = Class;
	}		
	
	
	/*	try 
	{
	CObj ObjectFromHell;
	void ( CObj::*Ptr1)(void)  = CObj::MRTC_VirtualCheck;
		Ptr1 = (((void ( CObj::** )(void))(*((mint *)&ObjectFromHell)))[1]);
		mint FirstPtr;
		memcpy(&FirstPtr, &Ptr1, 4);
		if (!IsBadReadPtr(((void *)(*((mint *)_pTrack->m_AllocatedAdress))), 8))
		{
			Ptr1 = (((void ( CObj::** )(void))(*((mint *)_pTrack->m_AllocatedAdress)))[1]);
			mint SecondPtr;
			memcpy(&SecondPtr, &Ptr1, 4);
			
			if (SecondPtr == FirstPtr)
			{
				CObj *Object = TDynamicCast<CObj > ((CObj *)_pTrack->m_AllocatedAdress);
				if (Object)
				{
					Class = CFStrF("%s", Object->MRTC_ClassName());
				}
			}
		}
		//		Class = typeid(Object).name();
	}
	catch (...)
	{
		
	}*/

	NMemMgr::CMemTrack_Class *Track = m_MemTracking_Classes.FindEqual(Class);
	
	if (!Track)
	{

		Track = m_MemTracking_TrackedPool.New();
		Track->m_ClassName = Class;		
		m_MemTracking_ClassesList.Insert(Track);
		m_MemTracking_Classes.f_Insert(Track);
	}

	++Track->m_AllocData.m_TimesAlloced;
	Track->m_AllocData.m_MemUsed += _Size;

	CFStr Stack;
	
#ifdef MRTC_ENABLE_MSCOPE
	if (pManager)
	{
		MRTC_ContextStack *pContextStack = pManager->GetContexctStack().Get();

		int nCtx = pContextStack->m_iContext;
		int MaxInc = nCtx - 3;
		if (MaxInc < 0)
			MaxInc = 0;
		bool First = true;
		for (int i = MaxInc; i < nCtx; ++i)
		{
			if (!First)
			{
				Stack += "->";
			}
			else
				First = false;

			Stack += pContextStack->m_lpContexts[i]->m_pName;

		}
	}
#endif

	if (!Track->m_pChild)
		Track->m_pChild = m_MemTracking_TrackedChildPool.New();

	NMemMgr::CMemTrack_Class *ChildTrack = Track->m_pChild->m_ChildrenTree.FindEqual(Stack);

	if (!ChildTrack)
	{
		ChildTrack = m_MemTracking_TrackedPool.New();
		ChildTrack->m_ClassName = Stack;
		Track->m_pChild->m_Children.Insert(ChildTrack);
		Track->m_pChild->m_ChildrenTree.f_Insert(ChildTrack);
	}

	++ChildTrack->m_AllocData.m_TimesAlloced;
	ChildTrack->m_AllocData.m_MemUsed += _Size;

	_TrackRet.m_pClass = Track;
	_TrackRet.m_pChildClass = ChildTrack;	
}


void CDA_MemoryManager::MemTracking_TrackDelete(NMemMgr::CMemTrack_UniquePair &_Track, mint _Size)
{
	if (_Track.m_pClass)
	{
		++_Track.m_pClass->m_AllocData.m_TimesDeleted;
		_Track.m_pClass->m_AllocData.m_MemUsed -= _Size;	
	}
	if (_Track.m_pChildClass)
	{
		++_Track.m_pChildClass->m_AllocData.m_TimesDeleted;
		_Track.m_pChildClass->m_AllocData.m_MemUsed -= _Size;	
	}
}


bool CDA_MemoryManager::CheckMemory()
{
	
	M_LOCK(m_Lock);
	
	bool HeapIsOk = true;
	
	if (m_bDebugMemory)
	{
		
		if (!IsHeapsOK(this))
		{
			M_TRACEALWAYS("Heap structure is broken\n");
			HeapIsOk = false;
		}
		
		SICDA_MemoryManager_HeapChunk Iterator = m_ChunksTree;
		
		while (Iterator)
		{
			// Walk block
			
			SDA_Defraggable *CurrentBlock = Iterator->m_pFirstBlock;
			
			while (CurrentBlock)
			{
				if (CurrentBlock->IsFree())
				{				
					if (!CheckMemoryToBe(((uint8*)CurrentBlock) + sizeof(SDA_DefraggableFree), 0xdd, GetBlockSize(CurrentBlock) - sizeof(SDA_DefraggableFree), "Damage in freed memory", false))
						HeapIsOk = false;
				}
				else if (CurrentBlock->IsDebug())
				{

					SDA_DefraggableDebug_Pre *Pre = (SDA_DefraggableDebug_Pre *)((mint)CurrentBlock + sizeof(SDA_Defraggable));
					SDA_DefraggableDebug_Post *Post = (SDA_DefraggableDebug_Post *)((mint)CurrentBlock + GetBlockSize(CurrentBlock) - sizeof(SDA_DefraggableDebug_Post));
#if defined(PLATFORM_XENON) || defined(PLATFORM_PS3)
					if (!CheckMemoryToBe(Pre->m_PreBlockCheck, 0xFD, 7, "Damage at start of block", false))
						HeapIsOk = false;
					if (!CheckMemoryToBe(Pre->m_PreBlockCheck+7, 0xFE, 1, "Damage at start of block", false))
						HeapIsOk = false;
					if (!CheckMemoryToBe(Pre->m_PreBlockCheck+8, 0xFD, MDA_DEBUG_BLOCK_PRECHECK - 8, "Damage at start of block", false))
						HeapIsOk = false;
//			CheckMemoryToBe(((uint8*)Pre->m_PreBlockCheck) + MDA_DEBUG_BLOCK_PRECHECK - 1, 0xFE, 1, "Damage at start of block when freeing", true);
//			CheckMemoryToBe(((uint8*)Pre->m_PreBlockCheck) + MDA_DEBUG_BLOCK_PRECHECK - 1, 0xbd, 1, "Damage at start of block when freeing", true);
#else
					if (!CheckMemoryToBe(Pre->m_PreBlockCheck, 0xFD, MDA_DEBUG_BLOCK_PRECHECK - 1, "Damage at start of block", false))
						HeapIsOk = false;
#endif
//					if (!CheckMemoryToBe(((uint8 *)Pre->m_PreBlockCheck) + MDA_DEBUG_BLOCK_PRECHECK - 1, 0xbd, 1, "Damage at start of block", false))
//						HeapIsOk = false;
					if (!CheckMemoryToBe(Post->m_PostBlockCheck, 0xFD, MDA_DEBUG_BLOCK_POSTCHECK, "Damage at end of block", false))
						HeapIsOk = false;
				}
				
				CurrentBlock = CurrentBlock->m_pNextBlock;			
				
			}
			
			++Iterator;
			
		}
		
		
	}
	
	return HeapIsOk;
}


#endif



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


