
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/

#ifndef __INC_MEMMGRHEAP
#define __INC_MEMMGRHEAP

//#include "MCC.h"
#include "MDA_Linkedlist.h"
#include "MDA_AVLTree.h"
#include "MDA_Hash.h"

//#ifndef MDA_DEFRAGMEMORY_ALIGN
//#define MDA_DEFRAGMEMORY_ALIGN 8
//#endif

#ifndef MDA_DEFRAGMEMORY_MAXBLOCKSIZE
#define MDA_DEFRAGMEMORY_MAXBLOCKSIZE 512*2048
#endif

#ifndef MDA_DEFRAGMEMORY_TRAGETBLOCKSIZE
#define MDA_DEFRAGMEMORY_TRAGETBLOCKSIZE 32*2048
#endif

// Precheck has to be at least 4 uint8s

// Temp!, alignment will be type sensitive eventually
/*#define MDA_ALIGNMENT 16
#define MDA_DEBUG_BLOCK_PRECHECK 16
#define MDA_DEBUG_BLOCK_POSTCHECK 16*/


#if defined(CPU_AMD64) || defined(PLATFORM_XENON) || defined(PLATFORM_PS3)
#define MDA_ALIGNMENT 16
#define MDA_DEBUG_BLOCK_PRECHECK 16
#define MDA_DEBUG_BLOCK_POSTCHECK 16
#else

#ifdef CPU_QWORD_ALIGNMENT
#define MDA_ALIGNMENT 8
#define MDA_DEBUG_BLOCK_PRECHECK 8
#define MDA_DEBUG_BLOCK_POSTCHECK 8
#else
#define MDA_ALIGNMENT 4
#define MDA_DEBUG_BLOCK_PRECHECK 4
#define MDA_DEBUG_BLOCK_POSTCHECK 4
#endif

#endif

#ifdef _DEBUG
#define MDA_NEW_DEBUG_NOLEAK new(5, __FILE__, __LINE__, (SDA_Defraggable *)NULL)
#else
#ifndef M_RTM
#define MDA_NEW_DEBUG_NOLEAK new(5, 0, 0)
#else
#define MDA_NEW_DEBUG_NOLEAK new
#endif
#endif


class CDA_MemoryManager_SizeClass;
class CDA_MemoryManager;
class CDA_Defraggable;
#ifdef M_SUPPORTMEMORYDEBUG

namespace NMemMgr
{

	class CMemTrack_Class
	{	
	public:
		CFStr m_ClassName;
		DLinkD_Link(CMemTrack_Class, m_ILink);

		CDA_HashLink m_HashLink;

		class CMemTrack_Child *m_pChild;

		class CAllocData
		{
		public:
			int m_TimesAlloced;
			int m_TimesDeleted;
			mint m_MemUsed;
			bool IsSame(CAllocData &Data)
			{
				if (Data.m_MemUsed != m_MemUsed)
					return false;
				if (Data.m_TimesAlloced != m_TimesAlloced)
					return false;
				if (Data.m_TimesDeleted != m_TimesDeleted)
					return false;

				return true;
			}
		};
		CAllocData m_AllocData;
		CAllocData m_LastAllocData;

		CMemTrack_Class();
		~CMemTrack_Class();
	};

	typedef DLinkD_Iter(CMemTrack_Class, m_ILink) SICMemTrack_Class;

	class CMemTrackHash : public TCDA_HashType<CMemTrack_Class, CDA_HashSmall, CFStr> 
	{
		virtual bool IsSame(void *_First, void *_Second)
		{
			CFStr *Obj1 = (CFStr *)_First;
			CFStr *Obj2 = (CFStr *)_Second;

			return strcmp(Obj1->GetStr(), Obj2->GetStr()) == 0;
		}

		virtual mint GetHash(void *_HashData)
		{
			CFStr *Obj = (CFStr *)_HashData;
			CMDA_CRC32Checker Checker;
			Checker.AddBlock(Obj->Str(), Obj->Len());
			return Checker.m_CurrentCRC;
		}
	};

	class CMemTrack_Child
	{
	public:
		CMemTrack_Child()
		{
			m_ChildrenHash.InitHash(2, (mint)&((CMemTrack_Class *)0x70000000)->m_ClassName - (mint)((CDA_Hashable *)(CMemTrack_Class *)0x70000000), 2);
		}
		~CMemTrack_Child()
		{
			{
				SICMemTrack_Class Iterator = m_Children;
				while (Iterator)
				{
					delete ((NMemMgr::CMemTrack_Class *)Iterator);
					Iterator = m_Children;
				}
			}
		}

		CMemTrackHash m_ChildrenHash;

		DLinkD_List(CMemTrack_Class, m_ILink) m_Children;
	};

	class CMemTrack_UniquePair
	{
	public:
		void Clear()
		{
			m_pClass = NULL;
			m_pChildClass = NULL;
		}				
		CMemTrack_Class *m_pClass;
		CMemTrack_Class *m_pChildClass;
	};
};

#endif

enum
{
	CDA_HEAPBLOCK_FREE		= 1,
	CDA_HEAPBLOCK_DEBUG		= 2,
};

struct M_ALIGN(16) SDA_Defraggable
{
	SDA_Defraggable *m_pNextBlock;
//	union
//	{
		struct SDegraggablePtr0
		{
			mint m_Pointer:(sizeof(mint)*8-2);
			mint m_Flags:2;
		};
//			mint m_bIsFree:1;
//			mint m_bIsDebug:1;
/*
		struct SDegraggablePtr1
		{
			mint m_PointerFillout:30;
			mint m_Flags:2;
		};*/

		SDegraggablePtr0 m_p0;
//		SDegraggablePtr1 m_p1;

		// if both free and debug is set this is a aligned block and the m_Pointer is a ptr to the real defraggable
//	};

	M_INLINE bool IsFree()
	{
		return m_p0.m_Flags == CDA_HEAPBLOCK_FREE;
	}

	M_INLINE bool IsDebug()
	{
		return m_p0.m_Flags == CDA_HEAPBLOCK_DEBUG;
	}

	M_INLINE void SetFree()
	{
		m_p0.m_Flags |= CDA_HEAPBLOCK_FREE;
	}

	M_INLINE void ClearFree()
	{
		m_p0.m_Flags &= ~CDA_HEAPBLOCK_FREE;
	}

	M_INLINE void SetDebug()
	{
		m_p0.m_Flags |= CDA_HEAPBLOCK_DEBUG;
	}

	M_INLINE void ClearDebug()
	{
		m_p0.m_Flags &= ~CDA_HEAPBLOCK_DEBUG;
	}

	M_INLINE SDA_Defraggable *m_pPrevBlockGet()
	{
		return (SDA_Defraggable *)(m_p0.m_Pointer << 2);
	}
	M_INLINE void m_pPrevBlockSet(SDA_Defraggable *_Pointer)
	{
//		MDA_ASSERT(!((mint)_Pointer & 0x3), "Pointer to a Defraggable has to be aligned at least to a mint");
		m_p0.m_Pointer = ((mint)_Pointer) >> 2;
	}
};

struct SDA_DefraggableFreeLink
{
	SDA_DefraggableFreeLink *m_pPrevFree;
	SDA_DefraggableFreeLink *m_pNextFree;

	void AddFirst(CDA_MemoryManager_SizeClass *_SizeClass);
	void Remove();
};

struct SDA_DefraggableFree
{
	SDA_Defraggable m_BlockLink;
	SDA_DefraggableFreeLink m_FreeLink;
};

struct SDA_DefraggableRelease
{
	SDA_Defraggable m_BlockLink;

};

#ifdef M_SUPPORTMEMORYDEBUG

struct SDA_DefraggableDebug_Pre
{
	unsigned char m_PreBlockCheck[MDA_DEBUG_BLOCK_PRECHECK];
};



struct SDA_DefraggableDebug_Post
{
	unsigned char m_PostBlockCheck[MDA_DEBUG_BLOCK_POSTCHECK];

	const char *m_pFile;
	uint16 m_Line;
	uint16 m_BlockType;
	uint32 m_SequenceId;
	NMemMgr::CMemTrack_UniquePair m_TrackingPair;
};

struct SDA_DefraggableDebug
{
	SDA_Defraggable m_BlockLink;

	SDA_DefraggableDebug_Pre m_Pre;

	// This are actually saved at end of block just here for sizeof goodness

	SDA_DefraggableDebug_Post m_Post;
};
#endif

class CDA_MemoryManager;

//JK-TEMP: Moved this to the end of the header file otherwise GNU won't compile
// Debug operator
/*
void* operator new(mint _Size, int _Block, int Dummy, int Dummy2);
void* operator new[](mint _Size, int _Block, int Dummy, int Dummy2);
#ifdef COMPILER_NEEDOPERATORDELETE
	void operator delete(void *Block, int _Block, int Dummy, int Dummy2);
#endif

void* operator new(mint _Size, int _Block, const char *_File, int _FileNumber, SDA_Defraggable *_MakeThisUnique);
void* operator new[](mint _Size, int _Block, const char *_File, int _FileNumber, SDA_Defraggable *_MakeThisUnique);
#ifdef COMPILER_NEEDOPERATORDELETE
	void operator delete(void *Block, int _Block, const char *_File, int _FileNumber, SDA_Defraggable *_MakeThisUnique);
#endif
*/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Inherit to be able to alloc this object in an
						defraggable memory manager. 
						
	Comments:			This class has to be the "first" inherited class
						(in case of multiple inheritance) if it is to 
						work, otherwise assumptions of members gets 
						wrong and it will result in a crash.
\*____________________________________________________________________*/
enum EDA_Defraggable_MoveMethods
{
	EDA_Defraggable_MoveMethod_CantMove
	,EDA_Defraggable_MoveMethod_OldObjectIntact
	,EDA_Defraggable_MoveMethod_OldObjectTrashed
	,EDA_Defraggable_MoveMethod_OldObjectTrashed_WithPre
	,EDA_Defraggable_MoveMethod_OldObjectTrashed_WithPost
	,EDA_Defraggable_MoveMethod_OldObjectTrashed_WithPreAndPost
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Inherit this class if you want to be able to
						defrag an object in a heap (CDA_MemoryManager)
						
	Comments:			You have to inherit this class first in the 
						order of inherited objects in case of multiple 
						inheritance , or defragging and deleting will 
						not work.
\*____________________________________________________________________*/

class MCCDLLEXPORT CDA_Defraggable
{
public:

	CDA_MemoryManager *m_pMemoryManager;

//	CDA_Defraggable();
//	virtual ~CDA_Defraggable();

	virtual void MoveObject(void *NewLocation);
	virtual void PreMoveObject(mint &_Context);
	virtual void PostMoveObject(mint _Context);
	virtual EDA_Defraggable_MoveMethods GetMoveObjectMethodsAllowed() pure;

	void Realloc(mint _NewSize);
	void Shrink(mint _NewSize);



	// Dummy new constructor
	void* operator new(mint _Size) throw();

	// The destructor for all
	void operator delete(void *Block);
	// The destructor for all
	void operator delete[] (void *Block);

	void* operator new(mint _Size, CDA_MemoryManager *MemManager);
	void* operator new(mint _Size, CDA_MemoryManager *MemManager, mint _ExtraSize);
	void* operator new[] (mint _Size, CDA_MemoryManager *MemManager);
	void* operator new[] (mint _Size, CDA_MemoryManager *MemManager, mint _ExtraSize);

#ifdef COMPILER_NEEDOPERATORDELETE
	void operator delete(void *Block,CDA_MemoryManager *MemManager);
	void operator delete(void *Block, CDA_MemoryManager *MemManager, mint _ExtraSize);
	void operator delete[] (void *Block,CDA_MemoryManager *MemManager);
	void operator delete[] (void *Block, CDA_MemoryManager *MemManager, mint _ExtraSize);
#endif

};

class MCCDLLEXPORT CDA_MemoryManager_SizeClass
{
public:
	mint m_Size;
	CDA_MemoryManager *m_pMemoryManager;

	class CCompare
	{
	public:
		static aint Compare(CDA_MemoryManager_SizeClass *_pFirst, CDA_MemoryManager_SizeClass *_pSecond, void *_pContext)
		{
			if (_pFirst->m_Size > _pSecond->m_Size)
				return 1;
			else if (_pFirst->m_Size < _pSecond->m_Size)
				return -1;
			return 0;
		}

		static aint Compare(CDA_MemoryManager_SizeClass *_pFirst, mint _Key, void *_pContext)
		{
			if (_pFirst->m_Size > _Key)
				return 1;
			else if (_pFirst->m_Size < _Key)
				return -1;
			return 0;
		}
	};

	DIdsTreeAVLAligned_Link(CDA_MemoryManager_SizeClass, m_TreeLink, mint, CCompare);

	int m_nContainedDefraggable;
	SDA_DefraggableFreeLink m_FirstFreeBlock;

	CDA_MemoryManager_SizeClass(CDA_MemoryManager *_pMemoryManager);
	~CDA_MemoryManager_SizeClass();
};

typedef DIdsTreeAVLAligned_Iterator(CDA_MemoryManager_SizeClass, m_TreeLink, mint, CDA_MemoryManager_SizeClass::CCompare) SICDA_MemoryManager_SizeClass;


class MCCDLLEXPORT CDA_MemoryManager_HeapChunk
{
public:
	CDA_MemoryManager *m_pMemmanager;
	void *m_pHeap;
	void *m_pHeapEnd;
	SDA_Defraggable *m_pFirstBlock;

	class CCompare
	{
	public:
		static aint Compare(CDA_MemoryManager_HeapChunk *_pFirst, CDA_MemoryManager_HeapChunk *_pSecond, void *_pContext)
		{
			if ((mint)_pFirst->m_pHeap > (mint)_pSecond->m_pHeap)
				return 1;
			else if ((mint)_pFirst->m_pHeap < (mint)_pSecond->m_pHeap)
				return -1;
			return 0;
		}

		static aint Compare(CDA_MemoryManager_HeapChunk *_pFirst, void * _Key, void *_pContext)
		{
			if ((mint)_pFirst->m_pHeap > (mint)_Key)
				return 1;
			else if ((mint)_pFirst->m_pHeap < (mint)_Key)
				return -1;
			return 0;
		}
	};

	DIdsTreeAVLAligned_Link(CDA_MemoryManager_HeapChunk, m_TreeLink, void *, CCompare);

	CDA_MemoryManager_HeapChunk(CDA_MemoryManager *_pMemmanager, mint _Size);
	~CDA_MemoryManager_HeapChunk();
};
typedef DIdsTreeAVLAligned_Iterator(CDA_MemoryManager_HeapChunk, m_TreeLink, void *, CDA_MemoryManager_HeapChunk::CCompare) SICDA_MemoryManager_HeapChunk;

// We need to store all strings for debug allocs, because the module that did the allocation might be unloaded when we check memory

class MCCDLLEXPORT CDA_MemoryManager_DebugAllocFile : public CDA_Hashable
{
public:
	CDA_MemoryManager_DebugAllocFile()
	{

	}
	~CDA_MemoryManager_DebugAllocFile()
	{

	}
	DLinkD_Link(CDA_MemoryManager_DebugAllocFile, m_ILink);

	const char *m_pOriginalPtr;
	char m_FileName[_MAX_PATH];
};

typedef DLinkD_Iter(CDA_MemoryManager_DebugAllocFile, m_ILink) SICDA_MemoryManager_DebugAllocFile;

enum EDA_MemoryManager_DefragErrors
{
	EDA_MemoryManager_DefragErrors_OK
	,EDA_MemoryManager_DefragErrors_NotEnoughTempSpaceToDefragIn
	,EDA_MemoryManager_DefragErrors_OtherError
};

#define MRTC_ALIGN(x) CDA_Alignment AutoSetAlignment(x);

class MCCDLLEXPORT CDA_Alignment
{
	CDA_MemoryManager *m_pMemoryManger;
public:
	
	CDA_Alignment(int _Alignment, CDA_MemoryManager *_pMemoryManger = NULL);

	~CDA_Alignment();
};

class MCCDLLEXPORT CDA_MemoryManager
{
	friend class CDA_MemoryManager_HeapChunk;
	friend class CDA_Alignment;
	friend class CInputContext_Dolphin; // for sending mem-info to the PC
public:
	MRTC_CriticalSection m_Lock;

	int m_bDestroyCalled;
	void PrivateInit();
	mint GetBlockSize(SDA_Defraggable *Block);
	mint UserBlockSize(SDA_Defraggable *_Block);
	CDA_MemoryManager_HeapChunk *GetChunkFromMem(void *Mem);
	CDA_MemoryManager_SizeClass *GetFreeSizeClass(mint _Size);

	static void* MemFromBlock(SDA_Defraggable* _Block);

	void *AllocImp(mint _Size, mint _Alignment);
#ifdef M_SUPPORTMEMORYDEBUG
	void *AllocDebugImp(mint _Size, mint _Alignment, uint16 _BlockType, const char *File, int Line);
#endif

	mint m_GlobalAlign;
	void SetGlobalAlignment(mint _Align = 0);

public:
	void Destroy();

	MRTC_CriticalSection &GetLock(){return m_Lock;}
	// Sizes
	TDA_Pool<CDA_MemoryManager_SizeClass> m_SizesPool;

	DIdsTreeAVLAligned_Tree(CDA_MemoryManager_SizeClass, m_TreeLink, mint, CDA_MemoryManager_SizeClass::CCompare) m_SizesFreeTree;
	CDA_MemoryManager_SizeClass* GetSizeClass(mint _SizeNeeded);

#ifdef M_SUPPORTMEMORYDEBUG
	// Debug string locations
	TDA_Pool<CDA_MemoryManager_DebugAllocFile>  m_DebugStringsPool;
	
	TCDA_IDHashType<CDA_MemoryManager_DebugAllocFile> m_DebugStringsHash;
	DLinkD_List(CDA_MemoryManager_DebugAllocFile, m_ILink) m_DebugStings;
	int m_bDebugHashInit;
	int m_bDebugMemory;
	int m_bShowAllocs;		
	CFStr m_ShowAllocContext;
#endif


	// Alloc tracking

#ifdef M_SUPPORTMEMORYDEBUG
	int m_MemTracking_bOn;
	int m_MemTracking_StartMem;

	TDA_Pool<NMemMgr::CMemTrack_Class> m_MemTracking_TrackedPool;
	TDA_Pool<NMemMgr::CMemTrack_Child> m_MemTracking_TrackedChildPool;


	NMemMgr::CMemTrackHash m_MemTracking_Classes;
	DLinkD_List(NMemMgr::CMemTrack_Class, m_ILink) m_MemTracking_ClassesList;

	void MemTracking_TrackAlloc(NMemMgr::CMemTrack_UniquePair &Track, mint Size);
	void MemTracking_TrackDelete(NMemMgr::CMemTrack_UniquePair &Track, mint Size);

	void M_CDECL MemTracking_ReportMemUsage(int Verbose);

	void M_CDECL MemTracking_Start();
	void M_CDECL MemTracking_Stop();
	void M_CDECL MemTracking_Report(bool Verbose);

	struct SMemInfo
	{
		CFStr  m_Name;
		mint m_nAllocBytes;
		mint m_nAllocNum;
	};
	void MemTracking_GetMemUsageInfo(TList_Vector<SMemInfo> &_StrList, const char* _pClassName = NULL);

	mint m_SequenceMark;
	int GetSequenceMark() const { return m_SequenceMark; }

#endif

	// Chunks
	TDA_Pool<CDA_MemoryManager_HeapChunk> m_ChunksPool;
	DIdsTreeAVLAligned_Tree(CDA_MemoryManager_HeapChunk, m_TreeLink, void *, CDA_MemoryManager_HeapChunk::CCompare) m_ChunksTree;

	mint m_AllocatedMem;
	mint m_AlignAlloc;
	mint m_FreeMem;
	mint m_FreeMemMin;
	mint m_DynamicGrowSize;
	mint m_CommitGranularity;

	class CDefaultMemoryManager *m_pDefaultManager;
#ifdef MRTC_DEFAULTMAINHEAP
	int m_bUseDefaultMainHeap;
#endif

	int m_bCanDefrag;
	int m_bDynamicSize;
#ifdef M_SUPPORTMEMORYDEBUG
	int m_bMemDebug;
#endif
	int m_bValidateHeap;

	void ValidateHeap();
	bool IsHeapOk();

	int m_RunningDebugRuntime;
	int m_RunningReleaseRuntime;
	
	CDA_MemoryManager();
	CDA_MemoryManager(mint _StaticHeapSize);
	virtual ~CDA_MemoryManager() pure;


	void InitStatic(mint _HeapSize);
	virtual void *AllocHeap(mint Size, bool _bCommit);
	virtual void FreeHeap(void *Block);
	virtual bool HeapCommit(void *_pMem, mint _Size, bool _bCommited);
	virtual mint HeapCommitGranularity();

	bool CommitMemory(void *_pMemStart, void *_pMemEnd);
	void DeCommitMemory(void *_pMemStart, void *_pMemEnd);

#ifdef MRTC_DEFAULTMAINHEAP
	int m_UsedMemory;
#endif

	mint GetFreeMem();
	mint GetUsedMem();
	mint GetMinFreeMem();
	mint GetLargestFreeMem();

#ifdef M_SUPPORTMEMORYDEBUG
	void DisplayLeaks();
	bool CheckMemory();
	void ShowAllocs(const char *_pMemoryContext);
	void HideAllocs();
#endif

	EDA_MemoryManager_DefragErrors Defrag(bool _bFreeBottom);

	void *Alloc(mint _Size);
	void *AllocAlign(mint _Size, mint _Align); // An aligned block cant have debuginfo
#ifdef M_SUPPORTMEMORYDEBUG
	void *AllocDebug(mint _Size, uint16 _BlockType, const char *File, int Line);
	void *AllocDebugAlign(mint _Size, mint _Align, uint16 _BlockType, const char *File, int Line);
#endif
	
	void *Realloc(void *_Block, mint _Size);
	void *ReallocAlign(void *_Block, mint _Size, mint _Align);
#ifdef M_SUPPORTMEMORYDEBUG
	void *ReallocDebug(void *_Block, mint _Size, uint16 _BlockType, const char *File, int Line);
	void *ReallocDebugAlign(void *_Block, mint _Size, mint _Align, uint16 _BlockType, const char *File, int Line);
#endif
	mint MemorySize(const void *_Block);
	void Free(void *_Block);

};



class CBlockManager
{
private:

	class CFreeLink
	{
	public:

		CFreeLink *m_pNext;
		CFreeLink *m_pPrev;

		bool IsInList()
		{
			return m_pNext != NULL;
		}

		bool LoneInList()
		{
			return (m_pPrev->m_pPrev == NULL
				&&
				m_pNext->m_pNext == NULL);
		}


		void LinkAfter(CFreeLink *_pLink)
		{
			m_pNext = _pLink->m_pNext;
			m_pPrev = _pLink;			
			_pLink->m_pNext = this;
			m_pNext->m_pPrev = this;
		}
		void Remove()
		{
			m_pNext->m_pPrev = m_pPrev;
			m_pPrev->m_pNext = m_pNext;
			m_pNext = NULL;
		}
	};

	class CFreeList
	{
	public:
		CFreeLink m_First;
		CFreeLink m_Last;

		CFreeList()
		{
			m_First.m_pNext = &m_Last;
			m_First.m_pPrev = NULL;
			m_Last.m_pNext = NULL;
			m_Last.m_pPrev = &m_First;
		}

		bool IsEmpty()
		{
			return m_First.m_pNext == &m_Last;

		}
	};

	class CBlockSize;

	public:
	class CBlock
	{
		friend class CBlockManager;
		friend class CBlockSize;
	private:
		CFreeLink m_FreeLink;
		CBlock *m_pPrevBlock;
		CBlock *m_pNextBlock;
	public:
		void * m_pMemory;
	};
	private:

	TDA_Pool<CBlock> m_BlockPool;

	class CBlockSize
	{
	public:
		CFreeList m_FreeList;
		mint m_Size;

		class CCompare
		{
		public:
			static aint Compare(CBlockSize *_pFirst, CBlockSize *_pSecond, void *_pContext)
			{
				if (_pFirst->m_Size > _pSecond->m_Size)
					return 1;
				else if (_pFirst->m_Size < _pSecond->m_Size)
					return -1;
				return 0;
			}

			static aint Compare(CBlockSize *_pFirst, mint _Key, void *_pContext)
			{
				if (_pFirst->m_Size > _Key)
					return 1;
				else if (_pFirst->m_Size < _Key)
					return -1;
				return 0;
			}
		};

		DIdsTreeAVLAligned_Link(CBlockSize, m_TreeLink, mint, CBlockSize::CCompare);

		static CBlockSize *BlockSizeFromFreeList(CFreeLink *_pFreeList)
		{
			M_OFFSET(CBlockSize, m_FreeList, Offset);

			return (CBlockSize *)((uint8*)_pFreeList - Offset);
		}

		CBlockSize()
		{
		}
		~CBlockSize()
		{
			M_ASSERT(m_FreeList.IsEmpty(), "MUust");
		}

		CBlock *GetBlock()
		{
			CBlock *pBlock = (CBlock *)m_FreeList.m_First.m_pNext;
			pBlock->m_FreeLink.Remove();
			pBlock->m_FreeLink.m_pNext = NULL;
			return pBlock;
		}

		void AddBlock(CBlock *_pBlock)
		{
			_pBlock->m_FreeLink.LinkAfter(&m_FreeList.m_First);
		}
	};

	TDA_Pool<CBlockSize> m_BlockSizePool;


	DIdsTreeAVLAligned_Tree(CBlockSize, m_TreeLink, mint, CBlockSize::CCompare) m_FreeSizeTree;
	
	void *m_pBlockStart;
	void *m_pBlockEnd;
	CBlock *m_pFirstBlock;
	mint m_Align;
	mint m_bAutoDefrag:1;

	virtual void MoveBlock(void *_To, void*_From, mint _Size);

	void RemoveFreeBlock(CBlock *_pBlock);

	CBlockSize *GetSizeBlock(mint _BlockSize);

	void DeleteBlock(CBlock *_pBlock);

	public:

	void Defrag();

	mint GetBlockSize(CBlock *_pBlock);

	CBlock *Alloc(mint _Size);
	void Free(CBlock *&_pBlock);
	void Create(void *_pMemory, mint _Blocksize, mint _Alignment, bool _bAutoDefrag);

};

#ifdef PLATFORM_DOLPHIN
class CBlockManagerAram : public CBlockManager
{
	virtual void MoveBlock(void *_From, void*_To, mint _Size);
};
typedef CBlockManager::CBlock CAramBlock;
#ifndef USE_VIRTUAL_MEMORY
extern CBlockManagerAram g_AramManagerMisc;
#endif
extern CBlockManager g_AramManagerSound;
#endif

/*class CDA_MemoryManager_Container
{
public:
	CDA_MemoryManager *m_pManager;
	void* m_hLock;

	void Lock();
	void Unlock();

	CDA_MemoryManager *GetManager();
};


extern CDA_MemoryManager_Container g_MemoryManager;
*/

// Debug operator
void* operator new(mint _Size, int _Block, int Dummy, int Dummy2);
void* operator new[](mint _Size, int _Block, int Dummy, int Dummy2);
#ifdef COMPILER_NEEDOPERATORDELETE
	void operator delete(void *Block, int _Block, int Dummy, int Dummy2);
#endif

void* operator new(mint _Size, int _Block, const char *_File, int _FileNumber, SDA_Defraggable *_MakeThisUnique);
void* operator new[](mint _Size, int _Block, const char *_File, int _FileNumber, SDA_Defraggable *_MakeThisUnique);
#ifdef COMPILER_NEEDOPERATORDELETE
	void operator delete(void *Block, int _Block, const char *_File, int _FileNumber, SDA_Defraggable *_MakeThisUnique);
#endif

#endif // __INC_MEMMGRHEAP
