
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
#include "MRTC_RemoteDebug.h"
#include "MMemMgrPool.h"
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

		class CCompare
		{
		public:
			static aint Compare(CMemTrack_Class *_pFirst, CMemTrack_Class *_pSecond, void *_pContext)
			{
				return _pFirst->m_ClassName.CompareNoCase(_pSecond->m_ClassName);
			}

			static aint Compare(CMemTrack_Class *_pFirst, const ch8 *_Key, void *_pContext)
			{
				return _pFirst->m_ClassName.CompareNoCase(_Key);
			}
		};

		DIdsTreeAVLAligned_Link(CMemTrack_Class, m_TreeLink, const ch8 *, CMemTrack_Class::CCompare);

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

	typedef DLinkD_Iter(CMemTrack_Class, m_ILink) CMemTrack_ClassIter;

	class CMemTrack_Child
	{
	public:
		CMemTrack_Child()
		{
		}
		~CMemTrack_Child()
		{
			{
				CMemTrack_ClassIter Iterator = m_Children;
				while (Iterator)
				{
					delete ((NMemMgr::CMemTrack_Class *)Iterator);
					Iterator = m_Children;
				}
			}
		}

		DIdsTreeAVLAligned_Tree(CMemTrack_Class, m_TreeLink, const ch8 *, CMemTrack_Class::CCompare) m_ChildrenTree;
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

class MCCDLLEXPORT CDA_MemoryManager_DebugAllocFile
{
public:
	CDA_MemoryManager_DebugAllocFile()
	{

	}
	~CDA_MemoryManager_DebugAllocFile()
	{

	}

	class CCompare
	{
	public:
		static aint Compare(CDA_MemoryManager_DebugAllocFile *_pFirst, CDA_MemoryManager_DebugAllocFile *_pSecond, void *_pContext)
		{
			if (_pFirst->m_pOriginalPtr > _pSecond->m_pOriginalPtr)
				return 1;
			else if (_pFirst->m_pOriginalPtr < _pSecond->m_pOriginalPtr)
				return -1;
			return 0;
		}

		static aint Compare(CDA_MemoryManager_DebugAllocFile *_pFirst, const char *_Key, void *_pContext)
		{
			if (_pFirst->m_pOriginalPtr > _Key)
				return 1;
			else if (_pFirst->m_pOriginalPtr < _Key)
				return -1;
			return 0;
		}
	};

	DIdsTreeAVLAligned_Link(CDA_MemoryManager_DebugAllocFile, m_TreeLink, const char *, CCompare);

	const char *m_pOriginalPtr;
	char m_FileName[_MAX_PATH];
};

typedef DIdsTreeAVLAligned_Iterator(CDA_MemoryManager_DebugAllocFile, m_TreeLink, const char *, CDA_MemoryManager_DebugAllocFile::CCompare) CDA_MemoryManager_DebugAllocFileIter;

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
	CDA_MemoryManager_SizeClass *GetFreeSizeClass(mint _Size, bint _bFragment);

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
	TCPool<CDA_MemoryManager_SizeClass, 128, NThread::CLock, CPoolType_Freeable, CAllocator_Virtual> m_SizesPool;

	DIdsTreeAVLAligned_Tree(CDA_MemoryManager_SizeClass, m_TreeLink, mint, CDA_MemoryManager_SizeClass::CCompare) m_SizesFreeTreeNormal;
	DIdsTreeAVLAligned_Tree(CDA_MemoryManager_SizeClass, m_TreeLink, mint, CDA_MemoryManager_SizeClass::CCompare) m_SizesFreeTreeFragments;
	CDA_MemoryManager_SizeClass* GetSizeClass(mint _SizeNeeded);

#ifdef M_SUPPORTMEMORYDEBUG
	// Debug string locations
	TCPool<CDA_MemoryManager_DebugAllocFile, 128, NThread::CLock, CPoolType_Freeable, CAllocator_Virtual>  m_DebugStringsPool;
	
	DIdsTreeAVLAligned_Tree(CDA_MemoryManager_DebugAllocFile, m_TreeLink, const char *, CDA_MemoryManager_DebugAllocFile::CCompare) m_DebugStringTree;
	int m_bDebugMemory;
	int m_bShowAllocs;		
	CFStr m_ShowAllocContext;
#endif


	// Alloc tracking

#ifdef M_SUPPORTMEMORYDEBUG
	int m_MemTracking_bOn;
	int m_MemTracking_StartMem;

	TCPool<NMemMgr::CMemTrack_Class, 128, NThread::CLock, CPoolType_Freeable, CAllocator_Virtual> m_MemTracking_TrackedPool;
	TCPool<NMemMgr::CMemTrack_Child, 128, NThread::CLock, CPoolType_Freeable, CAllocator_Virtual> m_MemTracking_TrackedChildPool;


	DIdsTreeAVLAligned_Tree(NMemMgr::CMemTrack_Class, m_TreeLink, const ch8 *, NMemMgr::CMemTrack_Class::CCompare) m_MemTracking_Classes;
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
	void MemTracking_GetMemUsageInfo(TArray<SMemInfo> &_StrList, const char* _pClassName = NULL);

	mint m_SequenceMark;
	int GetSequenceMark() const { return m_SequenceMark; }

#endif

	// Chunks
	TCPool<CDA_MemoryManager_HeapChunk, 128, NThread::CLock, CPoolType_Freeable, CAllocator_Virtual> m_ChunksPool;
	DIdsTreeAVLAligned_Tree(CDA_MemoryManager_HeapChunk, m_TreeLink, void *, CDA_MemoryManager_HeapChunk::CCompare) m_ChunksTree;

	mint m_AllocatedMem;
	mint m_AlignAlloc;
	mint m_FreeMem;
	mint m_FreeMemMin;
	mint m_DynamicGrowSize;
	mint m_CommitGranularity;
	mint m_LargeBlockThreshold;

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


template <typename t_CAllocator = DIdsDefaultAllocator, typename t_CPoolType = CPoolType_Freeable>
class TCSimpleBlockManager
{


public:
	class CBlockSize;

	class CBlock
	{
		friend class TCSimpleBlockManager;
		friend class CBlockSize;
	private:
		class CCompare
		{
		public:
			DIdsPInlineS static aint Compare(const CBlock *_pFirst, const CBlock *_pSecond, void *_pContext)
			{
				if (_pFirst->m_Memory0 > _pSecond->m_Memory0)
					return 1;
				else if (_pFirst->m_Memory0 < _pSecond->m_Memory0)
					return -1;
				return 0;
			}

			DIdsPInlineS static aint Compare(const CBlock *_pTest, uint32 _Key, void *_pContext)
			{
				if (_pTest->GetMemory() > _Key)
					return 1;
				else if (_pTest->GetMemory() < _Key)
					return -1;
				return 0;
			}
		};

		DIdsTreeAVLAligned_Link_FromTemplate(CBlock, m_AVLLink, uint32, CCompare);
		CBlock *m_pPrevBlock;
		CBlock *m_pNextBlock; // 8 bytes here
	public:
		uint32 m_Memory0:31;
		uint32 m_Free:1;
		uint32 GetMemory() const
		{
			return m_Memory0 << 1;
		}
		void SetMemory(uint32 _Memory)
		{
			m_Memory0 = _Memory >> 1;
		}
//		CBlockInfo *m_pBlockInfo;
	};

public:

	typedef DIdsTreeAVLAligned_Iterator_FromTemplate(CBlock, m_AVLLink, uint32, typename CBlock::CCompare) CBlockIter;

	class CBlockSize
	{
	public:

		class CCompare
		{
		public:
			DIdsPInlineS static aint Compare(const CBlockSize *_pFirst, const CBlockSize *_pSecond, void *_pContext)
			{
				if (_pFirst->m_Size > _pSecond->m_Size)
					return 1;
				else if (_pFirst->m_Size < _pSecond->m_Size)
					return -1;
				return 0;
			}

			DIdsPInlineS static aint Compare(const CBlockSize *_pTest, uint32 _Key, void *_pContext)
			{
				if (_pTest->m_Size > _Key)
					return 1;
				else if (_pTest->m_Size < _Key)
					return -1;
				return 0;
			}
		};

		DIdsTreeAVLAligned_Link_FromTemplate(CBlockSize, m_AVLLink, uint32, CCompare);
		DIdsTreeAVLAligned_Tree_FromTemplate(CBlock, m_AVLLink, uint32, typename CBlock::CCompare) m_FreeList;
		uint32 m_Size;

		CBlockSize()
		{
		}
		~CBlockSize()
		{
			M_ASSERT(m_FreeList.IsEmpty(), "MUust");
		}

		CBlock *GetBlock(uint32 _Alignment)
		{
			CBlockIter Iter = m_FreeList;
			while (Iter)
			{
				CBlock *pBlock = Iter;

				if (!(pBlock->GetMemory() & (_Alignment - 1)))
				{
					return pBlock;
				}
				++Iter;
			}

			return NULL;
		}

		void AddBlock(CBlock *_pBlock)
		{
			m_FreeList.f_Insert(_pBlock);
			_pBlock->m_Free = true;
		}
	};

	TCPool<CBlockSize, 128, NThread::CLock, t_CPoolType, t_CAllocator> m_BlockSizePool;
	TCPool<CBlock, 128, NThread::CLock, t_CPoolType, t_CAllocator> m_BlockPool;

	DIdsTreeAVLAligned_Tree_FromTemplate(CBlockSize, m_AVLLink, uint32, typename CBlockSize::CCompare) m_FreeSizeTree;
	DIdsTreeAVLAligned_Tree_FromTemplate(CBlock, m_AVLLink, uint32, typename CBlock::CCompare) m_BlockTree;

	uint32 m_pBlockStart;
	uint32 m_pBlockEnd;
	uint32 m_Align;
	uint32 m_FreeMemory;
	CBlock *m_pFirstBlock;

	void CheckHeap()
	{
	#ifndef _DEBUG
		return;
	#endif
		// Iterate all blocks
		CBlock *pCurrent = m_pFirstBlock;
		bint bLastFree = false;
		uint32 LastMem = 0;
		if (pCurrent)
			bLastFree = !pCurrent->m_Free;
		
		while (pCurrent)
		{
			if (pCurrent->m_Free)
			{
				if (bLastFree)
					M_BREAKPOINT;
			}

			bLastFree = pCurrent->m_Free;

			if (LastMem > pCurrent->GetMemory())
				M_BREAKPOINT;

			LastMem = pCurrent->GetMemory();

			pCurrent = pCurrent->m_pNextBlock;
		}

		DIdsTreeAVLAligned_Iterator_FromTemplate(CBlockSize, m_AVLLink, uint32, typename CBlockSize::CCompare) Iter = m_FreeSizeTree;

		while (Iter)
		{
			CBlockIter Iter2 = Iter->m_FreeList;

			uint32 BlockSize = GetBlockSize(Iter2);
			uint32 ShouldBeSize = Iter->m_Size;
			if (BlockSize != ShouldBeSize)
				M_BREAKPOINT;


			++Iter;
		}
	}


	void RemoveFreeBlock(CBlock *_pBlock)
	{
		CBlockSize *pSizeClass = (CBlockSize *)m_FreeSizeTree.FindEqual(GetBlockSize(_pBlock));
		pSizeClass->m_FreeList.f_Remove(_pBlock);
		_pBlock->m_Free = false;

		if (pSizeClass->m_FreeList.IsEmpty())
		{				
			m_FreeSizeTree.f_Remove(pSizeClass);
			m_BlockSizePool.Delete(pSizeClass);					
		}
	}

	CBlockSize *GetSizeBlock(uint32 _BlockSize)
	{
		CBlockSize *pSizeClass = m_FreeSizeTree.FindEqual(_BlockSize);

		if (!pSizeClass)
		{
			pSizeClass = m_BlockSizePool.New();
			pSizeClass->m_Size = _BlockSize;
			m_FreeSizeTree.f_Insert(pSizeClass);
		}
		return pSizeClass;
	}

	void DeleteBlock(CBlock *_pBlock)
	{
		if (_pBlock->m_pPrevBlock)
			_pBlock->m_pPrevBlock->m_pNextBlock = _pBlock->m_pNextBlock;
		else
			m_pFirstBlock = _pBlock;

		if (_pBlock->m_pNextBlock)
			_pBlock->m_pNextBlock->m_pPrevBlock = _pBlock->m_pPrevBlock;

		m_BlockPool.Delete(_pBlock);
	}

public:


	TCSimpleBlockManager()
	{
		m_pBlockStart = NULL;
		m_pBlockEnd = NULL;
		m_pFirstBlock = NULL;
		gf_RDSendRegisterHeap((mint)this, "TCSimpleBlockManager", 0, 0);
	}

	~TCSimpleBlockManager()
	{	

		// Memory leak
		M_ASSERT(m_pFirstBlock->m_pNextBlock == NULL, "[TCSimpleBlockManager] Memory leak! (1)");
		M_ASSERT(m_pFirstBlock->m_Free, "[TCSimpleBlockManager] Memory leak! (2)");

		RemoveFreeBlock(m_pFirstBlock);
		m_BlockPool.Delete(m_pFirstBlock);
	}

	uint32 GetFreeMemory()
	{
		return m_FreeMemory;
	}
	uint32 GetLargestFreeBlock()
	{
		CBlockSize *pLargest = m_FreeSizeTree.FindLargest();
		if (pLargest)
		{
			return pLargest->m_Size;
		}
		return 0;
	}

	CBlock *GetBlockFromAddress(uint32 _Address)
	{
		return m_BlockTree.FindEqual(_Address);
	}

	uint32 GetBlockSize(CBlock *_pBlock)
	{		
		if (_pBlock->m_pNextBlock)
			return _pBlock->m_pNextBlock->GetMemory() - _pBlock->GetMemory();
		else
			return m_pBlockEnd - _pBlock->GetMemory();
	}

	CBlock *Alloc(uint32 _Size, uint32 _Alignment)
	{
		if (_Alignment < m_Align)
			_Alignment = m_Align;

		// Align all allocated sizes to keep alignment in heap
		_Size = (_Size + (m_Align - 1)) & (~(m_Align - 1));
		uint32 OriginalSize = _Size;
		uint32 AlignmentSearch = _Alignment;

		// First try to allocate a block that is just the right size and see if we are good alignmentwise

	RestartSearch:

		CBlockSize *pSizeClass = (CBlockSize *)m_FreeSizeTree.FindSmallestGreaterThanEqual(_Size);
		
		if (!pSizeClass)
		{
			return NULL;
		}
		
		CBlock * pBlock = pSizeClass->GetBlock(AlignmentSearch);
		if (!pBlock)
		{
			// No block met out alignment requirement, lets restart search with a size that will allow alignment
			if (_Size != OriginalSize)
				AlignmentSearch = m_Align; // The default alignment
			else
				_Size = OriginalSize + _Alignment;
			goto RestartSearch;
		}

		uint32 BlockSize = pSizeClass->m_Size;
		
		RemoveFreeBlock(pBlock);
		m_FreeMemory -= BlockSize;
		

		if (pBlock->GetMemory() & (_Alignment - 1))
		{
			// We need to put a free block between our block and the previous block
			uint32 MemoryPos = (pBlock->GetMemory() + (_Alignment - 1)) & (~(_Alignment - 1));
			M_ASSERT(MemoryPos != pBlock->GetMemory(), "Duh");
			uint32 SizeLeftPre = MemoryPos - pBlock->GetMemory();
			m_FreeMemory += SizeLeftPre;
			BlockSize -= SizeLeftPre;

			// Pre block
			{
				// Add remainder of block to free blocks
				CBlock * pNewBlock = m_BlockPool.New();
				
				pNewBlock->SetMemory(pBlock->GetMemory());
				pNewBlock->m_pPrevBlock = pBlock->m_pPrevBlock;
				if (pNewBlock->m_pPrevBlock)
				{
					pNewBlock->m_pPrevBlock->m_pNextBlock = pNewBlock;
				}

				pNewBlock->m_pNextBlock = pBlock;
				pBlock->m_pPrevBlock = pNewBlock;
				
				CBlockSize *pSizeClass = (CBlockSize *)m_FreeSizeTree.FindEqual(SizeLeftPre);
				
				if (!pSizeClass)
				{
					pSizeClass = m_BlockSizePool.New();
					pSizeClass->m_Size = SizeLeftPre;
					m_FreeSizeTree.f_Insert(pSizeClass);
				}
				
				pSizeClass->AddBlock(pNewBlock);
			}
			pBlock->SetMemory(MemoryPos);

			
		}

		{
			uint32 SizeLeft = BlockSize - OriginalSize;
			m_FreeMemory += SizeLeft;
			
			if (SizeLeft > 0)
			{
				// Add remainder of block to free blocks
				CBlock * pNewBlock = m_BlockPool.New();
				
				pNewBlock->SetMemory((uint32)((uint32)pBlock->GetMemory() + OriginalSize));
				pNewBlock->m_pNextBlock = pBlock->m_pNextBlock;
				if (pNewBlock->m_pNextBlock)
				{
					pNewBlock->m_pNextBlock->m_pPrevBlock = pNewBlock;
				}
				pBlock->m_pNextBlock = pNewBlock;
				pNewBlock->m_pPrevBlock = pBlock;
				
				CBlockSize *pSizeClass = (CBlockSize *)m_FreeSizeTree.FindEqual(SizeLeft);
				
				if (!pSizeClass)
				{
					pSizeClass = m_BlockSizePool.New();
					pSizeClass->m_Size = SizeLeft;
					m_FreeSizeTree.f_Insert(pSizeClass);
				}
				
				pSizeClass->AddBlock(pNewBlock);
			}
		}
		
	#ifdef M_Profile
		CheckHeap();
	#endif
		m_BlockTree.f_Insert(pBlock);

		gf_RDSendHeapAlloc(pBlock, OriginalSize, this, gf_RDGetSequence(), 0);

		return pBlock;
	}
	void Free(CBlock *&_pBlock)
	{
		gf_RDSendHeapFree(_pBlock, this, gf_RDGetSequence());
		m_BlockTree.f_Remove(_pBlock);

		M_ASSERT(!_pBlock->m_Free, "Must not be free already");
		uint32 BlockSize = GetBlockSize(_pBlock);
		uint32 pBlockPos = _pBlock->GetMemory();
		CBlock *pFinalBlock = _pBlock;
		CBlock *pPrevBlock = _pBlock->m_pPrevBlock;
		CBlock *pNextBlock = _pBlock->m_pNextBlock;
		bool bDeleteBlock1 = false;
		bool bDeleteBlock2 = false;
		
		m_FreeMemory += BlockSize;
		
		if (pPrevBlock)
		{
			if (pPrevBlock->m_Free)
			{
				BlockSize += GetBlockSize(pPrevBlock);
				pBlockPos = pPrevBlock->GetMemory();
				pFinalBlock = pPrevBlock;
				
				RemoveFreeBlock(pPrevBlock);
				
				bDeleteBlock1 = true;
			}
		}
		
		if (pNextBlock)
		{
			if (pNextBlock->m_Free)
			{
				BlockSize += GetBlockSize(pNextBlock);
				
				RemoveFreeBlock(pNextBlock);
				
				bDeleteBlock2 = true;
			}
		}
		
		CBlockSize *pSizeClass = GetSizeBlock(BlockSize);
		
		pSizeClass->AddBlock(pFinalBlock);

		if (bDeleteBlock1)
			DeleteBlock(_pBlock);
		if (bDeleteBlock2)
			DeleteBlock(pNextBlock);

	#ifdef M_Profile
		// Reset Free Memory
		CheckHeap();
	#endif
		_pBlock = NULL;
	}

	void Create(const char *_pHeapName, uint32 _pStartAddress, uint32 _Blocksize, uint32 _Alignment)
	{
		m_Align = _Alignment;
		m_pBlockStart = _pStartAddress;
		m_pBlockEnd = m_pBlockStart + _Blocksize;
		
		m_pFirstBlock = m_BlockPool.New();
		
		m_pFirstBlock->SetMemory(_pStartAddress);
		m_pFirstBlock->m_pNextBlock = NULL;
		m_pFirstBlock->m_pPrevBlock = NULL;
		
		CBlockSize *pSizeClass = m_BlockSizePool.New();
		pSizeClass->m_Size = _Blocksize;
		m_FreeSizeTree.f_Insert(pSizeClass);

		m_FreeMemory = _Blocksize;

		pSizeClass->AddBlock(m_pFirstBlock);

		gf_RDSendRegisterHeap((mint)this, _pHeapName, _pStartAddress, _pStartAddress+_Blocksize);

	#ifdef M_Profile
		CheckHeap();
	#endif
	}

};








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
