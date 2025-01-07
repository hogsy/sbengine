
#include "PCH.h"
#include "MRenderPS3_Heap.h"

namespace NRenderPS3GCM
{

CPS3_Heap CPS3_Heap::ms_This;

#if defined(M_Profile)
#define MCHECKHEAP
#define MHEAPASSERT(_x) {if (!(_x)) M_BREAKPOINT;}
#else
#define MHEAPASSERT(_x) ((void)(0))
#endif


static M_INLINE void ReportFailedAlloc(CPS3_Heap* _pHeap, int _Size)
{
	int HeapSize = mint(_pHeap->m_pBlockEnd) - mint(_pHeap->m_pBlockStart);
	int FreeMem = _pHeap->GetFreeMemory();
	int LargestFree = _pHeap->GetLargestFreeBlock();

	M_TRACEALWAYS("OUT OF VIDEO MEMORY!\n"
	              " - Tried to allocate: %d KiB.\n"
	              " - Heap size: %d KiB.\n"
	              " - Free mem: %d KiB.\n"
	              " - Largest free block: %d KiB.\n", _Size >> 10, HeapSize >> 10, FreeMem >> 10, LargestFree >> 10);
	M_BREAKPOINT;
}

const char* CPS3_Heap::GetStatus()
{
	M_LOCK(m_Lock);
	static char aBuf[512];
	CStrBase::snprintf(aBuf, 512, "GfxHeap: Size %d KiB Free %d KiB (L %d KiB) Mid %d KiB", (mint(m_pBlockEnd) - mint(m_pBlockStart)) >> 10, m_FreeMemory >> 10, GetLargestFreeBlock() >> 10, m_MemorySavedMidBlock >> 10);
	return aBuf;
}

CPS3_Heap::CBlock *CPS3_Heap::AllocInternal(CBlock *_pBlock, mint _Size, mint _Alignment, bint _bUseMidBlocks)
{
	DLock(m_Lock);
	if (_Alignment < m_Align)
		_Alignment = m_Align;

	// Align all allocated sizes to keep alignment in heap
	_Size = (_Size + (m_Align - 1)) & (~(m_Align - 1));
	mint OriginalSize = _Size;
	mint AlignmentSearch = _Alignment;

	// First try to allocate a block that is just the right size and see if we are good alignmentwise

	CBlockSize *pSizeClass;
	CBlock * pBlock = NULL;
	if (_bUseMidBlocks)
	{
		while (!pBlock)
		{
			pSizeClass = (CBlockSize *)m_FreeSizeTree.FindSmallestGreaterThanEqual(_Size);
			
			if (!pSizeClass)
			{
				ReportFailedAlloc(this, _Size);
				return NULL;
			}

			pBlock = pSizeClass->GetBlock(AlignmentSearch, true);
			if (!pBlock)
			{
				// No block met out alignment requirement, lets restart search with a size that will allow alignment
				if (_Size != OriginalSize)
					AlignmentSearch = m_Align; // The default alignment
				else
					_Size = OriginalSize + _Alignment;
			}
		}
	}
	else
	{
		CSizeTreeIter Iter;
		Iter.SetRoot(m_FreeSizeTree);
		Iter.FindSmallestGreaterThanEqualForward(_Size);
		mint CanAlignSize = OriginalSize + _Alignment;
		while (1)
		{
			pSizeClass = Iter;
			
			if (!pSizeClass)
			{
				ReportFailedAlloc(this, _Size);
				return NULL;
			}
			
			if (pSizeClass->m_Size < CanAlignSize)
			{
				pBlock = pSizeClass->GetBlock(_Alignment, false);
			}
			else
			{
				pBlock = pSizeClass->GetBlock(_Alignment, false);
				if (!pBlock)
					pBlock = pSizeClass->GetBlock(m_Align, false);
			}

			if (pBlock)
				break;

			++Iter;
		}
	}

	return AllocInternalFromBlock(_pBlock, pBlock, _Size, OriginalSize, _Alignment);
}

CPS3_Heap::CBlock *CPS3_Heap::AllocInternalFromBlock(CBlock *_pBlock, CBlock *_pAllocTo, mint _Size, mint _OrininalSize, mint _Alignment)
{
	CBlock *pBlock = _pAllocTo;
	int BlockSize = GetBlockSize(pBlock);
	
	RemoveFreeBlock(pBlock);
	m_FreeMemory -= BlockSize;
	

	if (pBlock->GetMemory() & (_Alignment - 1))
	{
		// We need to put a free block between our block and the previous block
		mint MemoryPos = (pBlock->GetMemory() + (_Alignment - 1)) & (~(_Alignment - 1));
		MHEAPASSERT(MemoryPos != pBlock->GetMemory());
		
		int SizeLeftPre = MemoryPos - pBlock->GetMemory();
		m_FreeMemory += SizeLeftPre;
		BlockSize -= SizeLeftPre;

		// Pre block
		{
			// Add remainder of block to free blocks
			CBlock * pNewBlock = m_BlockPool.New();
			++m_nBlocks;
			
			pNewBlock->SetMemory(pBlock->GetMemory());
			pNewBlock->m_pPrevBlock = pBlock->m_pPrevBlock;
			if (pNewBlock->m_pPrevBlock)
				pNewBlock->m_pPrevBlock->m_pNextBlock = pNewBlock;
			else
				m_pFirstBlock = pNewBlock;


			pNewBlock->m_pNextBlock = pBlock;
			pBlock->m_pPrevBlock = pNewBlock;
			
			pNewBlock->m_MidBlock = pBlock->m_MidBlock;

			AddFreeBlock(pNewBlock, SizeLeftPre);
		}
		pBlock->SetMemory(MemoryPos);

		
	}

	{
		int SizeLeft = BlockSize - _OrininalSize;
		m_FreeMemory += SizeLeft;
		
		if (SizeLeft > 0)
		{
			// Add remainder of block to free blocks
			CBlock * pNewBlock = m_BlockPool.New();
			++m_nBlocks;
			
			pNewBlock->SetMemory((mint)((uint8 *)pBlock->GetMemory() + _OrininalSize));
			pNewBlock->m_pNextBlock = pBlock->m_pNextBlock;
			if (pNewBlock->m_pNextBlock)
			{
				pNewBlock->m_pNextBlock->m_pPrevBlock = pNewBlock;
			}
			pBlock->m_pNextBlock = pNewBlock;
			pNewBlock->m_pPrevBlock = pBlock;
			pNewBlock->m_MidBlock = pBlock->m_MidBlock;

			AddFreeBlock(pNewBlock, SizeLeft);
		}
	}
	
	if (_pBlock)
	{
		mint SizeOrg = GetBlockSize(pBlock);
		CBlock *pNextBlock = pBlock->m_pNextBlock;
		CBlock *pPrevBlock = pBlock->m_pPrevBlock;
		_pBlock->m_Memory0 = pBlock->m_Memory0;
		_pBlock->m_MidBlock = pBlock->m_MidBlock;
//		_pBlock->m_pBlockInfo = pBlock->m_pBlockInfo;
		pNextBlock->m_pPrevBlock = _pBlock;
		pPrevBlock->m_pNextBlock = _pBlock;
		_pBlock->m_pNextBlock = pNextBlock;
		_pBlock->m_pPrevBlock = pPrevBlock;
		m_BlockPool.Delete(pBlock);
		--m_nBlocks;

		mint Size = GetBlockSize(_pBlock);
		if (SizeOrg != Size)
			M_BREAKPOINT;
		return _pBlock;
	}
	return pBlock;
}


CPS3_Heap::CBlock *CPS3_Heap::Alloc(mint _Size, mint _Alignment, CBlockInfo *_pBlockInfo, bint _bUseMidBlocks)
{
	CPS3_Heap::CBlock *pRet = AllocInternal(NULL, _Size, _Alignment, _bUseMidBlocks);
	if (pRet)
	{
		pRet->m_pBlockInfo = _pBlockInfo;

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
		gf_RDSendHeapAlloc((void *)pRet->GetMemory(), _Size, this, gf_RDGetSequence(), 0);
#endif

	}
#ifdef MCHECKHEAP
	CheckHeap(2);
#endif
	return pRet;
}

void CPS3_Heap::FreeInternal(CBlock *&_pBlock, bint _bKeepBlock)
{
	DLock(m_Lock);
	MHEAPASSERT(!_pBlock->m_FreeLink.IsInList());
	int BlockSize = GetBlockSize(_pBlock);
	mint pBlockPos = _pBlock->GetMemory();
	
	CBlock *pFinalBlock = _pBlock;
	CBlock *pPrevBlock = _pBlock->m_pPrevBlock;
	CBlock *pNextBlock = _pBlock->m_pNextBlock;
	bool bDeleteBlock1 = false;
	bool bDeleteBlock2 = false;
	
	m_FreeMemory += BlockSize;
	bint bMidBlock = _pBlock->m_MidBlock;
	
	if (pPrevBlock)
	{
		if (pPrevBlock->m_MidBlock)
			bMidBlock = true;
		if (pPrevBlock->m_FreeLink.IsInList())
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
		if (pNextBlock->m_MidBlock)
			bMidBlock = true;
		if (pNextBlock->m_FreeLink.IsInList())
		{
			BlockSize += GetBlockSize(pNextBlock);
			
			RemoveFreeBlock(pNextBlock);
			
			bDeleteBlock2 = true;
		}
	}

	CBlockSize *pSizeClass = GetSizeBlock(BlockSize);
	
	if (_bKeepBlock && pFinalBlock == _pBlock)
	{
		// Replace block
		pFinalBlock = m_BlockPool.New();
		++m_nBlocks;

		pFinalBlock->m_Memory0 = _pBlock->m_Memory0;
		pFinalBlock->m_pBlockInfo = NULL;
		CBlock *pNextBlock = _pBlock->m_pNextBlock;
		CBlock *pPrevBlock = _pBlock->m_pPrevBlock;
		if (pNextBlock)
			pNextBlock->m_pPrevBlock = pFinalBlock;
		if (pPrevBlock)
			pPrevBlock->m_pNextBlock = pFinalBlock;
		pFinalBlock->m_pNextBlock = pNextBlock;
		pFinalBlock->m_pPrevBlock = pPrevBlock;
	}
	else
		pFinalBlock->m_pBlockInfo = NULL;

	pFinalBlock->m_MidBlock = bMidBlock;

	pSizeClass->AddBlock(pFinalBlock);

	if (bDeleteBlock1)
	{
		if (_bKeepBlock)
		{
			UnlinkBlock(_pBlock);
		}
		else
		{
			DeleteBlock(_pBlock);
		}
	}
	if (bDeleteBlock2)
		DeleteBlock(pNextBlock);

	if (m_bAutoDefrag)
		Defrag();

#ifdef MCHECKHEAP
	// Reset Free Memory
	CheckHeap(2);
#endif
	if (!_bKeepBlock)
		_pBlock = NULL;
}

CPS3_Heap::CBlock *CPS3_Heap::GetNextRealBlockMidBlock_r(CBlock *_pBlock, bint _bCheckMidBlock)
{
	
	mint RealSize = 0;
	if (_pBlock->m_pBlockInfo)
		RealSize = _pBlock->m_pBlockInfo->GetRealSize();
	if (!RealSize)
		RealSize = GetBlockSize(_pBlock);

	CBlock *pNextRealBlock = _pBlock->m_pNextBlock;
	CBlockInfo *pInfo = _pBlock->m_pBlockInfo;
	mint EndAddress = _pBlock->GetMemory() + RealSize;

	while (pNextRealBlock && pNextRealBlock->GetMemory() != EndAddress)
	{
		if (pNextRealBlock->m_MidBlockStart)
		{
			pNextRealBlock = GetNextRealBlockMidBlock_r(pNextRealBlock);
			continue;
		}
#ifdef MCHECKHEAP
		if (pNextRealBlock->m_FreeLink.IsInList())
		{
			if (!pNextRealBlock->m_MidBlock)
				M_BREAKPOINT;
			if (pNextRealBlock->m_pBlockInfo)
				M_BREAKPOINT;
		}
		else
		{
			if (pNextRealBlock->m_pBlockInfo == pInfo)
			{
				if (_bCheckMidBlock && pNextRealBlock->m_MidBlock != _pBlock->m_MidBlock)
					M_BREAKPOINT;
			}
			else
			{
				if (!pNextRealBlock->m_MidBlock)
					M_BREAKPOINT;
			}
			if (!pNextRealBlock->m_pBlockInfo)
				M_BREAKPOINT;
		}
#endif
		pNextRealBlock = pNextRealBlock->m_pNextBlock;
	}
	return pNextRealBlock;
}

CPS3_Heap::CBlock *CPS3_Heap::GetNextRealBlock(CBlock *_pBlock)
{
#ifdef MCHECKHEAP
//	if (_pBlock->m_MidBlock && !_pBlock->m_MidBlockStart)
//		M_BREAKPOINT;
#endif

	if (_pBlock->m_MidBlockStart)
		return GetNextRealBlockMidBlock_r(_pBlock);
	else
		return _pBlock->m_pNextBlock; 
}

void CPS3_Heap::Free(CBlock *&_pBlock)
{
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	gf_RDSendHeapFree((void *)_pBlock->GetMemory(), this, gf_RDGetSequence());
#endif

	FreeInternal(_pBlock, false);
}



CPS3_Heap::CBlock *CPS3_Heap::FreeMidBlocks_r2(CBlock *_pBlock)
{
	CBlockInfo *pInfo = _pBlock->m_pBlockInfo;
	CBlock *pNextBlock = GetNextRealBlockMidBlock_r(_pBlock, false);
	_pBlock = _pBlock->m_pNextBlock;
	while (_pBlock != pNextBlock)
	{
		if (_pBlock->m_pBlockInfo == pInfo)
			_pBlock->m_MidBlock = false;
		if (_pBlock->m_MidBlockStart)
		{
			_pBlock = GetNextRealBlockMidBlock_r(_pBlock, false);
		}
		else
			_pBlock = _pBlock->m_pNextBlock;
	}

	return pNextBlock;
}

CPS3_Heap::CBlock *CPS3_Heap::FreeMidBlocks_r(CBlock *_pBlock, CBlockInfo *_pInfo, CBlock *_pEndBlock)
{
	_pBlock = _pBlock->m_pNextBlock;
	while (_pBlock->m_pBlockInfo != _pInfo && _pBlock != _pEndBlock)
	{
		_pBlock->m_MidBlock = false;
		if (_pBlock->m_MidBlockStart)
		{
			_pBlock = FreeMidBlocks_r2(_pBlock);
		}
		else
			_pBlock = _pBlock->m_pNextBlock;

	}    
	return _pBlock;
}


void CPS3_Heap::FreeMidBlocks(CBlock *_pBlock)
{
	DLock(m_Lock);

	CBlock *pBlock = _pBlock;
	CBlockInfo *pInfo = pBlock->m_pBlockInfo;
	CBlock *pNextRealBlock = GetNextRealBlock(pBlock);
	bint bPrevMidBlock = pBlock->m_MidBlock;
/*	if (pBlock->m_pPrevBlock)
	{
		if (pBlock->m_pPrevBlock->m_MidBlock || pBlock->m_pPrevBlock->m_MidBlockStart)
			bPrevMidBlock = true;
	}*/

	int CheckLevelSave = m_CheckHeapLevel;
	m_CheckHeapLevel = 1;

	while (pBlock != pNextRealBlock)
	{

		CBlock *pNextBlock = pBlock->m_pNextBlock; 

		if (pBlock->m_pBlockInfo == pInfo)
		{
			MHEAPASSERT(pBlock->m_MidBlock == bPrevMidBlock);
			CBlock *pBlockToFree = pBlock;
			if (!bPrevMidBlock)
			{
				pBlock = FreeMidBlocks_r(pBlock, pInfo, pNextRealBlock);
				FreeInternal(pBlockToFree, false);
				continue;
			}
			if (pNextBlock == pNextRealBlock)
			{
				FreeInternal(pBlockToFree, false);
				break;
			}
			if (pNextBlock->m_FreeLink.IsInList())
				pNextBlock = pNextBlock->m_pNextBlock;
			FreeInternal(pBlockToFree, false);
		}

		pBlock = pNextBlock;
	}

	m_CheckHeapLevel = CheckLevelSave;

#ifdef MCHECKHEAP
	CheckHeap(3);
#endif
}
#if 0
void CPS3_Heap::FreeMidBlock(CBlock *_pBlock, mint _Memory, mint _SavedSize)
{
	DLock(m_Lock);
	
	CBlock *pBlock = _pBlock;
	CBlockInfo *pInfo = pBlock->m_pBlockInfo;
	while (pBlock->GetMemory() < _Memory)
		pBlock = pBlock->m_pNextBlock;

	MHEAPASSERT(pBlock->m_pBlockInfo == pInfo);
	MHEAPASSERT(pBlock->GetMemory() == _Memory);
	MHEAPASSERT(!pBlock->m_FreeLink.IsInList());

	m_MemorySavedMidBlock -= _SavedSize;

//	pBlock->m_MidBlock = false;
	FreeInternal(pBlock, false);
}
#endif

CPS3_Heap::CBlock *CPS3_Heap::ReturnMidBlock(CBlock *_pBlock, mint _Memory, mint _Size, bint _bFirstBlock)
{
	DLock(m_Lock);
	CBlock *pBlock = _pBlock;
	while (pBlock->m_pNextBlock->GetMemory() < _Memory)
		pBlock = pBlock->m_pNextBlock;

	// Free block

	m_FreeMemory += _Size;
	m_MemorySavedMidBlock += _Size;
	
	CBlock * pNewBlock = m_BlockPool.New();
	++m_nBlocks;
	
	if (_bFirstBlock)
		pBlock->m_MidBlockStart = true;

	pNewBlock->SetMemory(_Memory);
	pNewBlock->m_pPrevBlock = pBlock;
	pNewBlock->m_pNextBlock = pBlock->m_pNextBlock;
	pNewBlock->m_MidBlock = true;
	if (pNewBlock->m_pNextBlock)
	{
		pNewBlock->m_pNextBlock->m_pPrevBlock = pNewBlock;
	}
	pBlock->m_pNextBlock = pNewBlock;
	
	AddFreeBlock(pNewBlock, _Size);

	// Next block

	CBlock * pRightBlock = m_BlockPool.New();
	++m_nBlocks;
	
	pRightBlock->SetMemory(_Memory + _Size);
	pRightBlock->m_pPrevBlock = pNewBlock;
	pRightBlock->m_pNextBlock = pNewBlock->m_pNextBlock;
	pRightBlock->m_pBlockInfo = _pBlock->m_pBlockInfo;
	pRightBlock->m_MidBlock = _pBlock->m_MidBlock;

	if (pRightBlock->m_pNextBlock)
	{
		pRightBlock->m_pNextBlock->m_pPrevBlock = pRightBlock;
	}
	pNewBlock->m_pNextBlock = pRightBlock;

#ifdef MCHECKHEAP
	CheckHeap(2);
#endif

	return pRightBlock;	
}

void CPS3_Heap::Create(void *_pMemory, mint _Blocksize, mint _Alignment, bool _bAutoDefrag)
{
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	gf_RDSendRegisterHeap((mint)this, CStrF("PS3 Video Heap %d KiB", _Blocksize >> 10), (mint)_pMemory, (mint)_pMemory + _Blocksize);
#endif

	DLock(m_Lock);
	m_bAutoDefrag = _bAutoDefrag;
	m_Align = _Alignment;
	m_pBlockStart = _pMemory;
	m_pBlockEnd = ((uint8 *)m_pBlockStart + _Blocksize);
	
	m_pFirstBlock = m_BlockPool.New();
	++m_nBlocks;
	
	m_pFirstBlock->SetMemory((mint)_pMemory);
	m_pFirstBlock->m_pNextBlock = NULL;
	m_pFirstBlock->m_pPrevBlock = NULL;
	
	CBlockSize *pSizeClass = m_BlockSizePool.New();
	pSizeClass->m_Size = _Blocksize;
	m_FreeSizeTree.f_Insert(pSizeClass);

	m_FreeMemory = _Blocksize;
	m_MemorySavedMidBlock = 0;
	m_CheckHeapLevel = 1;

	pSizeClass->AddBlock(m_pFirstBlock);

#ifdef MCHECKHEAP
	CheckHeap(1);
#endif
}

void CPS3_Heap::CheckHeap(int _Level, bint _bCheckAlignment)
{
	if (_Level > m_CheckHeapLevel)
		return;
	DLock(m_Lock);
	// Iterate all blocks
	CBlock *pCurrent = m_pFirstBlock;
	bint bLastFree = false;
	uint32 LastMem = 0;
	if (pCurrent)
		bLastFree = !pCurrent->m_FreeLink.IsInList();

	CBlock *pNextRealBlock = pCurrent;

	mint nBlocks = 0;
	while (pCurrent)
	{
		if (pCurrent == pNextRealBlock)
		{
			if (pCurrent->m_MidBlock)
			{
				M_BREAKPOINT; // Cannot be a midblock
			}
			pNextRealBlock = GetNextRealBlock(pCurrent);
		}
		++nBlocks;
		if (pCurrent->m_FreeLink.IsInList())
		{
			if (pCurrent->m_pBlockInfo)
				M_BREAKPOINT;
			if (bLastFree)
				M_BREAKPOINT;
		}
		else
		{
			if (!pCurrent->m_pBlockInfo)
			{
				M_BREAKPOINT;
			}
			else if (_bCheckAlignment)
			{
				mint Alignment = pCurrent->m_pBlockInfo->GetAlignment();
				mint Mem = pCurrent->GetMemory();
				if (Mem & (Alignment-1))
					M_BREAKPOINT; // Alignment error

				int Bla = 0;
			}

		}

		bLastFree = pCurrent->m_FreeLink.IsInList();

        if (LastMem > pCurrent->GetMemory())
			M_BREAKPOINT;

		LastMem = pCurrent->GetMemory();

		pCurrent = pCurrent->m_pNextBlock;
	}

	if (m_nBlocks != nBlocks)
		M_BREAKPOINT;

	pCurrent = m_pFirstBlock;
	while (pCurrent)
	{
		pCurrent = GetNextRealBlock(pCurrent);
	}

	DIdsTreeAVLAligned_Iterator(CBlockSize, m_AVLLink, mint, CBlockSize::CCompare) Iter = m_FreeSizeTree;

	while (Iter)
	{
		DLinkDS_Iter(CBlock, m_FreeLink) Iter2 = Iter->m_FreeList;

		mint BlockSize = GetBlockSize(Iter2);
		mint ShouldBeSize = Iter->m_Size;
		if (BlockSize != ShouldBeSize)
			M_BREAKPOINT;


		++Iter;
	}
}

/*


*/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Helps to defrag the blocks
						
	Comments:			
						Defrag strategy:

						Find all blocks with mid blocks and save the mid blocks 
						in a tree sorted by alignment and size.	Loop through 
						all blocks and find exact matches for the mid blocks 
						and fill them. If no match is found put them in a list
						sorted by alignment and size. Fill the remaining mid 
						blocks as good as we are able to

						Bin the different blocks according to alignment and 
						aggerate the blocks downwand so all blocks with 

\*_____________________________________________________________________________*/


class CDefragHelper
{
public:
	typedef CPS3_Heap::CBlock CBlock;
	CPS3_Heap *m_pHeap;
	CDefragHelper(CPS3_Heap *_pHeap)
	{
		m_pHeap = _pHeap;
	}

	~CDefragHelper()
	{
		DeleteMidBlocks();
	}

	void DeleteMidBlocks()
	{
		while (m_MidBlocks.GetRoot())
		{
			CMidBlock *pTemp = m_MidBlocks.GetRoot();
			m_MidBlocks.f_Remove(pTemp);
			m_MidBlockPool.Delete(pTemp);

		}
	}

	CMTime m_MemMoveTime;
	CMTime m_XMemCpyExtended;

	void MoveMem(mint _From, mint _To, mint _Size)
	{
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
		gf_RDSendHeapMove((void *)_From, (void *)_To, _Size, m_pHeap, m_pHeap, gf_RDGetSequence());
#endif
#ifndef PLATFORM_XENON
		memmove((void *)_To, (void *)_From, _Size);
#else
		if ((_From < _To && _To < _From + _Size) || (_From > _To && _To + _Size > _From))
		{
			TMeasure(m_MemMoveTime);
			memmove((void *)_To, (void *)_From, _Size);
		}
		else
		{
			TMeasure(m_XMemCpyExtended);
#if _XDK_VER >= 2732
			XMemCpy((void *)_To, (const void *)_From, _Size);
#else
			XMemCpyExtended((void *)_To, (const void *)_From, _Size);
#endif
		}
#endif
	}

	class CMidBlock
	{
	public:
		CBlock *m_pBlock;
		mint m_Alignment;
		mint m_Size;

		void UpdateVars(CPS3_Heap *_pHeap)
		{
			m_Alignment = 1;
			mint Memory = m_pBlock->GetMemory();
			while (!(Memory & (m_Alignment)))
				m_Alignment <<= 1;
			m_Size = _pHeap->GetBlockSize(m_pBlock);
		}

		class CCompare
		{
		public:
			DIdsPInlineS static aint Compare(const CMidBlock *_pFirst, const CMidBlock *_pSecond, void *_pContext)
			{
				aint Size = (aint)_pFirst->m_Size - (aint)_pSecond->m_Size;
				if (Size != 0)
					return Size;
				aint Align = (aint)_pFirst->m_Alignment - (aint)_pSecond->m_Alignment;
				if (Align != 0)
					return Align;
				return (aint) (mint(_pFirst->m_pBlock) >> 1) - (aint)(mint(_pSecond->m_pBlock) >> 1);
			}

			DIdsPInlineS static aint Compare(const CMidBlock *_pTest, const CMidBlock &_Key, void *_pContext)
			{
				aint Size = (aint)_pTest->m_Size - (aint)_Key.m_Size;
				if (Size != 0)
					return Size;
				aint Align = (aint)_pTest->m_Alignment - (aint)_Key.m_Alignment;
				if (Align != 0)
					return Align;
				return (aint) (mint(_pTest->m_pBlock) >> 1) - (aint)(mint(_Key.m_pBlock) >> 1);
			}
		};

		DAVLAligned_Link(CMidBlock, m_AVLLink, CMidBlock, CCompare);
	};

	DAVLAligned_Tree(CMidBlock, m_AVLLink, CMidBlock, CMidBlock::CCompare) m_MidBlocks;

	TCPool<CMidBlock> m_MidBlockPool;

	void FindMidBlocksAndEvacuate()
	{
		// Move blocks from mid regions
		{
			CBlock *pCurrent = m_pHeap->m_pFirstBlock;

			while (pCurrent)
			{
				CBlock *pNextBlock = pCurrent->m_pNextBlock;
				CBlock *pNextRealBlock = m_pHeap->GetNextRealBlock(pCurrent);

				if (pNextRealBlock && pNextRealBlock != pNextBlock) 
				{
					CPS3_Heap::CBlockInfo *pCurrentBlockInfo = pCurrent->m_pBlockInfo;
					pCurrent = pCurrent->m_pNextBlock;
					// We have found a mid block
					// Loop through all blocks within the mid block and relocate the blocks to outside of the area (of any mid block)
					mint NextRealBlockEnd = pNextRealBlock->GetMemory();
					while (pCurrent->GetMemory() < NextRealBlockEnd)
					{
						if (pCurrent->m_MidBlockStart)
						{
							pCurrent = m_pHeap->GetNextRealBlock(pCurrent);
							if (!pCurrent)
								break;
							continue;
						}

						if (pCurrent->m_FreeLink.IsInList())
						{
							// A free block
						}
						else
						{
							if (pCurrent->m_pBlockInfo != pCurrentBlockInfo)
							{
								if (pCurrent->m_MidBlockStart)
									M_BREAKPOINT;
								mint Size = m_pHeap->GetBlockSize(pCurrent);
								mint Alignment = pCurrent->m_pBlockInfo->GetAlignment();
//								Alignment = 1;  // We don't need to be aligned here
//								if (Alignment == 4096)
//								{
//									pCurrent = pCurrent->m_pNextBlock;
//									continue;
//								}

								CBlock *pTemp = pCurrent->m_pNextBlock;
								if (pTemp->m_FreeLink.IsInList())
									pTemp = pTemp->m_pNextBlock;

								// We have an allocated block within the mid block, lets move it
								mint Location = pCurrent->GetMemory();
#ifdef MCHECKHEAP
								m_pHeap->CheckHeap(3);
#endif
								--m_pHeap->m_nBlocks;
								m_pHeap->FreeInternal(pCurrent, true);
#ifdef MCHECKHEAP
								m_pHeap->CheckHeap(3);
#endif

								if (m_pHeap->AllocInternal(pCurrent, Size, Alignment, false) != pCurrent)
								{
									TraceFreeBlocks();
									M_BREAKPOINT; // Out of memory
								}
								++m_pHeap->m_nBlocks;

#ifdef MCHECKHEAP
								m_pHeap->CheckHeap(3);
#endif
								MoveMem(Location, pCurrent->GetMemory(), Size);
								pCurrent = pTemp;
								continue;

							}
						}

						pCurrent = pCurrent->m_pNextBlock;
					}
					if (pCurrent->GetMemory() != NextRealBlockEnd)
						M_BREAKPOINT;
					pNextRealBlock = pCurrent;

				}

				pCurrent = pNextRealBlock;
			}
		}

		// Save mid regions
		{
			CBlock *pCurrent = m_pHeap->m_pFirstBlock;

			while (pCurrent)
			{
				CBlock *pNextBlock = pCurrent->m_pNextBlock;
				CBlock *pNextRealBlock = m_pHeap->GetNextRealBlock(pCurrent);

				if (pNextRealBlock && pNextRealBlock != pNextBlock) 
				{
					pCurrent = pCurrent->m_pNextBlock;
					// We have found a mid block
					// Loop through all blocks within the mid block and relocate the blocks to outside of the area (of any mid block)
					while (pCurrent != pNextRealBlock)
					{
						if (pCurrent->m_MidBlockStart)
						{
							// Recurse
							pCurrent = m_pHeap->GetNextRealBlock(pCurrent);
							continue;
						}

						if (pCurrent->m_FreeLink.IsInList())
						{
							// A free block
							if (!pCurrent->m_MidBlock)
								M_BREAKPOINT;

							CMidBlock *pNewBlock = m_MidBlockPool.New();
							pNewBlock->m_pBlock = pCurrent;
							pNewBlock->UpdateVars(m_pHeap);
							m_MidBlocks.f_Insert(pNewBlock);
						}

						pCurrent = pCurrent->m_pNextBlock;
					}

				}

				pCurrent = pNextRealBlock;
			}
		}
	}

	void FillMidBlocks()
	{
		mint SavedSize = 0;
		mint TriedSave = 0;
		// First consider perfect matches
		{
			CBlock *pCurrent = m_pHeap->m_pFirstBlock;

			while (pCurrent)
			{
				CBlock *pNextBlock = pCurrent->m_pNextBlock;
#ifdef MCHECKHEAP
				m_pHeap->CheckHeap(3);
#endif
				CBlock *pNextRealBlock = m_pHeap->GetNextRealBlock(pCurrent);

				if (pNextRealBlock == pNextBlock) // Only cantidates are blocks that don't have mid blocks
				{
					if (!pCurrent->m_FreeLink.IsInList()) // Only allocated blocks
					{
						mint Size = m_pHeap->GetBlockSize(pCurrent);
						TriedSave += Size;
						mint Alignment = pCurrent->m_pBlockInfo->GetAlignment();
						mint ExtraSize = 0;

						while (Alignment >= m_pHeap->m_Align)
						{
							CMidBlock ToFind;

							ToFind.m_Alignment = Alignment;
							ToFind.m_Size = Size + ExtraSize;
							ToFind.m_pBlock = (CBlock *)mint(-1);

							CMidBlock *pMidBlock = m_MidBlocks.FindSmallestGreaterThanEqual(ToFind);

							if (pMidBlock && pMidBlock->m_Alignment >= Alignment)
							{
								// Lets move into this block
								mint Memory = pCurrent->GetMemory();

								if (pNextRealBlock && pNextRealBlock->m_FreeLink.IsInList())
									pNextRealBlock = pNextRealBlock->m_pNextBlock;
								--m_pHeap->m_nBlocks;
								m_pHeap->FreeInternal(pCurrent, true);
#ifdef MCHECKHEAP
								m_pHeap->CheckHeap(3);
#endif
								++m_pHeap->m_nBlocks;
								if (m_pHeap->AllocInternalFromBlock(pCurrent, pMidBlock->m_pBlock, Size + ExtraSize, Size, pCurrent->m_pBlockInfo->GetAlignment()) != pCurrent)
									M_BREAKPOINT;
#ifdef MCHECKHEAP
								m_pHeap->CheckHeap(3);
#endif
								
								SavedSize += Size;
								MoveMem(Memory, pCurrent->GetMemory(), Size);

								m_MidBlocks.f_Remove(pMidBlock);
								m_MidBlockPool.Delete(pMidBlock);

								if (pCurrent->m_pPrevBlock->m_FreeLink.IsInList())
								{
                                    CMidBlock *pNew = m_MidBlockPool.New();
									pNew->m_pBlock = pCurrent->m_pPrevBlock;
									pNew->UpdateVars(m_pHeap);
									m_MidBlocks.f_Insert(pNew);
								}

								if (pCurrent->m_pNextBlock->m_FreeLink.IsInList())
								{
                                    CMidBlock *pNew = m_MidBlockPool.New();
									pNew->m_pBlock = pCurrent->m_pNextBlock;
									pNew->UpdateVars(m_pHeap);
									m_MidBlocks.f_Insert(pNew);
								}
								break;
							}

							Alignment >>= 1;
							ExtraSize += Alignment;
						}
					}
				}


				pCurrent = pNextRealBlock;
			}
		}
		M_TRACEALWAYS("Save Size %d / %d | Tried to save %d\n", SavedSize, m_pHeap->GetSavedMidBlock(), TriedSave);

		DeleteMidBlocks();

	}

	CBlock *GetNextRealFreeBlock(CBlock *_pBlock)
	{
		CBlock *pBlock = m_pHeap->GetNextRealBlock(_pBlock);
		while (pBlock && !pBlock->m_FreeLink.IsInList())
			pBlock = m_pHeap->GetNextRealBlock(pBlock);

		return pBlock;
	}

	CBlock *MakeSpaceHelper(CBlock *_pBlock, CBlock *_pNextBlock)
	{
		if (_pBlock == _pNextBlock)
		{
			return _pBlock;
		}
		else
		{
			mint Size = m_pHeap->GetBlockSize(_pNextBlock);

			if (!_pNextBlock->m_FreeLink.IsInList())
				M_BREAKPOINT; // Not correct

            if (_pBlock->m_FreeLink.IsInList())
				_pBlock = _pBlock->m_pNextBlock;
			mint MemoryStart = _pBlock->GetMemory();
			mint MemoryEnd = _pNextBlock->GetMemory();

			MoveMem(MemoryStart, MemoryStart + Size, MemoryEnd - MemoryStart);

			CBlock *pFreeBlock = _pBlock->m_pPrevBlock;
			if (pFreeBlock && pFreeBlock->m_FreeLink.IsInList())
			{
				m_pHeap->RemoveFreeBlock(pFreeBlock);
				mint NewSize = m_pHeap->GetBlockSize(pFreeBlock) + Size;
				m_pHeap->AddFreeBlock(pFreeBlock, NewSize);
			}
			else
			{
				CBlock * pNewBlock = m_pHeap->m_BlockPool.New();
				++m_pHeap->m_nBlocks;

				pNewBlock->SetMemory(MemoryStart);
				pNewBlock->m_pPrevBlock = _pBlock->m_pPrevBlock;
				pNewBlock->m_pNextBlock = _pBlock;

				if (pNewBlock->m_pNextBlock)
					pNewBlock->m_pNextBlock->m_pPrevBlock = pNewBlock;

				if (pNewBlock->m_pPrevBlock)
					pNewBlock->m_pPrevBlock->m_pNextBlock = pNewBlock;
				else
					m_pHeap->m_pFirstBlock = pNewBlock;


				m_pHeap->AddFreeBlock(pNewBlock, Size);
				pFreeBlock = pNewBlock;
			}

			CBlock *pCurrent = _pBlock;
			while (pCurrent != _pNextBlock)
			{
				pCurrent->SetMemory(pCurrent->GetMemory() + Size);
				pCurrent = pCurrent->m_pNextBlock;
			}

			mint TestSize = m_pHeap->GetBlockSize(pFreeBlock);
			m_pHeap->RemoveFreeBlock(_pNextBlock);
			m_pHeap->DeleteBlock(_pNextBlock);

#ifdef MCHECKHEAP
			m_pHeap->CheckHeap(2);
#endif
			return pFreeBlock;
		}
	}

	CBlock *MakeSpace_r(CBlock *_pBlock)
	{
		CBlock *pNextFree = GetNextRealFreeBlock(_pBlock);

        if (!pNextFree)
			return _pBlock;
		
		pNextFree = MakeSpace_r(pNextFree);

		// Do everything in helper to save stack space
		return MakeSpaceHelper(_pBlock, pNextFree);
	}

	CBlock *MakeSpace(CBlock *_pBlock)
	{
		// Move all blocks as long to the right as possible
		CBlock *pCurrent = _pBlock;

		if (!pCurrent)
			pCurrent = m_pHeap->m_pFirstBlock;

		return MakeSpace_r(pCurrent);

#if 0
		// Find last block
		CBlock *pNextReal = m_pHeap->GetNextRealBlock(pCurrent);
		while (pNextReal)
		{
			pCurrent = pNextReal;
			pNextReal = m_pHeap->GetNextRealBlock(pCurrent);
		}

		// Walk backward and remove free blocks

		if (!pCurrent)
			pCurrent = m_pHeap->m_pFirstBlock;
		else
			pCurrent = pCurrent->m_pNextBlock;

		aint ToFree = _Space;
        if (pCurrent->m_FreeLink.IsInList())
			ToFree -= m_pHeap->GetBlockSize(pCurrent);

		if (ToFree <= 0)
			return;

		CBlock *pNextFreeBlock = GetNextRealFreeBlock(pCurrent);
		while (ToFree)
		{
			mint ThisFreeBlockSize = m_pHeap->GetBlockSize(pNextFreeBlock);
			ToFree -= ThisFreeBlockSize;
			break;
		}

		CBlock *pNextFreeBlock = GetNextRealFreeBlock(pCurrent);
		while (ToFree)
		{
			if (!pNextFreeBlock)
				M_BREAKPOINT; // Out of memory
			mint ThisFreeBlockSize = m_pHeap->GetBlockSize(pNextFreeBlock);
            // Move all block forward by the size

			CBlock *pNextReal = pCurrent;
			if (pNextReal->m_FreeLink.IsInList())
				pNextReal = m_pHeap->GetNextRealBlock(pCurrent);

			MoveMem(pNextReal->GetMemory(), pNextReal->GetMemory() + ThisFreeBlockSize, pNextFreeBlock->GetMemory() - pNextReal->GetMemory());
			CBlock *pNextRealPrev = pNextReal;
			CBlock *pCurFix = pNextReal;
			while (pCurFix != pNextFreeBlock)
			{
				pCurFix->SetMemory(pCurFix->GetMemory() + ThisFreeBlockSize);
				pNextRealPrev = pCurFix;
				pCurFix = pCurFix->m_pNextBlock;
			}

			m_pHeap->RemoveFreeBlock(pNextFreeBlock);
			m_pHeap->DeleteBlock(pNextFreeBlock);

			if (pNextReal == pCurrent)
			{
			}
			else
			{
			}

			ToFree -= ThisFreeBlockSize;
			pNextFreeBlock = GetNextRealFreeBlock(pCurrent);
		}
#endif

	}

	void RemoveRealBlock(CBlock *_pStart, CBlock *_pEnd)
	{
		CBlock * pNewBlock = m_pHeap->m_BlockPool.New();
		++m_pHeap->m_nBlocks;

		CBlock * pTemp = _pStart;
		while (pTemp != _pEnd)
		{
			pTemp = pTemp->m_pNextBlock;
			--m_pHeap->m_nBlocks;
		}

		pNewBlock->SetMemory(_pStart->GetMemory());
		pNewBlock->m_pPrevBlock = _pStart->m_pPrevBlock;
		pNewBlock->m_pNextBlock = _pEnd;
		pNewBlock->m_pBlockInfo = (CPS3_Heap::CBlockInfo *)1;

		if (pNewBlock->m_pNextBlock)
			pNewBlock->m_pNextBlock->m_pPrevBlock = pNewBlock;

		if (pNewBlock->m_pPrevBlock)
			pNewBlock->m_pPrevBlock->m_pNextBlock = pNewBlock;
		else
			m_pHeap->m_pFirstBlock = pNewBlock;

		// And just free it
#ifdef MCHECKHEAP
		m_pHeap->CheckHeap(3);
#endif
		m_pHeap->m_FreeMemory -= m_pHeap->GetBlockSize(pNewBlock);
		m_pHeap->FreeInternal(pNewBlock, false);
	}


	void CoalesceAndSortByAlignment()
	{
		mint ToFixMem = 0;
		CBlock *pStartBlock = m_pHeap->m_pFirstBlock;
		CBlock *pInsertInto = m_pHeap->m_pFirstBlock;
		for (mint Alignment = 16384; Alignment >= m_pHeap->m_Align; Alignment >>= 1)
		{
			CBlock *pCurrent = pInsertInto;

			while (pCurrent && (!pCurrent->m_pBlockInfo || m_pHeap->GetBlockAlignment(pCurrent) != Alignment))
			{
				// Find the first correct block
				pCurrent = m_pHeap->GetNextRealBlock(pCurrent);
			}

			while (pCurrent)
			{
				mint RealSize = 0;
				if (pCurrent->m_pBlockInfo)
					RealSize = pCurrent->m_pBlockInfo->GetRealSize();
				if (!RealSize)
					RealSize = m_pHeap->GetBlockSize(pCurrent);

				mint NeededSize = (RealSize + Alignment - 1) & (~(Alignment - 1)); // We need to align to our alignment

				CBlock *pNextRealBlock = m_pHeap->GetNextRealBlock(pCurrent);
				CBlock *pNextRealAllocBlock = pNextRealBlock;
				CBlock *pNextBlock = pNextRealBlock;

				while (pNextRealAllocBlock && pNextRealAllocBlock->m_FreeLink.IsInList())
					pNextRealAllocBlock = m_pHeap->GetNextRealBlock(pNextRealAllocBlock);


				while (pNextBlock && (!pNextBlock->m_pBlockInfo || m_pHeap->GetBlockAlignment(pNextBlock) != Alignment))
				{
					// Find the first correct block
					pNextBlock = m_pHeap->GetNextRealBlock(pNextBlock);
				}

				if (pCurrent != pInsertInto || 1)
				{

					// Find mid block
					if (1)
					{
						bint bFound = false;
						mint Size = m_pHeap->GetBlockSize(pCurrent);
						mint Alignment = m_pHeap->GetBlockAlignment(pCurrent);
						mint ExtraSize = 0;

						while (Alignment >= m_pHeap->m_Align)
						{
							CMidBlock ToFind;

							ToFind.m_Alignment = Alignment;
							ToFind.m_Size = Size + ExtraSize;
							ToFind.m_pBlock = (CBlock *)mint(-1);

							CMidBlock *pMidBlock = m_MidBlocks.FindSmallestGreaterThanEqual(ToFind);

							if (pMidBlock && pMidBlock->m_Alignment >= Alignment)
							{
								// Lets move into this block
								mint Memory = pCurrent->GetMemory();

								if (pNextRealBlock && pNextRealBlock->m_FreeLink.IsInList())
									pNextRealBlock = pNextRealBlock->m_pNextBlock;
								--m_pHeap->m_nBlocks;
								m_pHeap->FreeInternal(pCurrent, true);
#ifdef MCHECKHEAP
								m_pHeap->CheckHeap(3);
#endif
								++m_pHeap->m_nBlocks;
								if (m_pHeap->AllocInternalFromBlock(pCurrent, pMidBlock->m_pBlock, Size + ExtraSize, Size, pCurrent->m_pBlockInfo->GetAlignment()) != pCurrent)
									M_BREAKPOINT;
#ifdef MCHECKHEAP
								m_pHeap->CheckHeap(3);
#endif
								
								MoveMem(Memory, pCurrent->GetMemory(), Size);

								m_MidBlocks.f_Remove(pMidBlock);
								m_MidBlockPool.Delete(pMidBlock);

								if (pCurrent->m_pPrevBlock->m_FreeLink.IsInList())
								{
                                    CMidBlock *pNew = m_MidBlockPool.New();
									pNew->m_pBlock = pCurrent->m_pPrevBlock;
									pNew->UpdateVars(m_pHeap);
									m_MidBlocks.f_Insert(pNew);
								}

								if (pCurrent->m_pNextBlock->m_FreeLink.IsInList())
								{
                                    CMidBlock *pNew = m_MidBlockPool.New();
									pNew->m_pBlock = pCurrent->m_pNextBlock;
									pNew->UpdateVars(m_pHeap);
									m_MidBlocks.f_Insert(pNew);
								}
								bFound = true;
//								M_TRACEALWAYS("Midblocking 0x%08x + %d bytes\n", pCurrent->GetMemory(), m_pHeap->GetBlockSize(pCurrent));
								break;
							}

							Alignment >>= 1;
							ExtraSize += Alignment;
						}

						if (bFound)
						{
							pCurrent = pNextBlock;
							pStartBlock = pNextRealAllocBlock;
							continue;
						}
					}

#ifdef MCHECKHEAP
					m_pHeap->CheckHeap(3);
#endif
					if (!pInsertInto || m_pHeap->GetBlockSize(pInsertInto) < NeededSize || !pInsertInto->m_FreeLink.IsInList())
						pInsertInto = MakeSpace(pInsertInto);

#ifdef MCHECKHEAP
            		m_pHeap->CheckHeap(3);
#endif

//					M_TRACEALWAYS("Moveing 0x%08x + %d bytes\n", pCurrent->GetMemory(), m_pHeap->GetBlockSize(pCurrent));

					pNextRealBlock = m_pHeap->GetNextRealBlock(pCurrent);
					mint InsertSize = m_pHeap->GetBlockSize(pInsertInto);
					if (InsertSize <= NeededSize) // We demand that the block be larger because the code will become simpler
						M_BREAKPOINT; // Out of memory
					
					// Move memory

					CBlock *pEndBlock = pCurrent;
					// Find end of block
					while (pEndBlock->m_pNextBlock && pEndBlock->m_pNextBlock != pNextRealBlock)
						pEndBlock = pEndBlock->m_pNextBlock;

					aint Offset = (aint)pInsertInto->GetMemory() - (aint)pCurrent->GetMemory();

					mint InsertPos = pInsertInto->GetMemory();
					MoveMem(pCurrent->GetMemory(), InsertPos, RealSize);

					// Make hole at block
					RemoveRealBlock(pCurrent, pNextRealBlock);
#ifdef MCHECKHEAP
					m_pHeap->CheckHeap(3);
#endif

					// Unlink pInstertInto
					m_pHeap->RemoveFreeBlock(pInsertInto);

					InsertSize = m_pHeap->GetBlockSize(pInsertInto);

					// Link in blocks

					pCurrent->m_pPrevBlock = pInsertInto->m_pPrevBlock;
					if (pCurrent->m_pPrevBlock)
						pCurrent->m_pPrevBlock->m_pNextBlock = pCurrent;
					else
						m_pHeap->m_pFirstBlock = pCurrent;

					pEndBlock->m_pNextBlock = pInsertInto;
					if (pEndBlock->m_pNextBlock)
						pEndBlock->m_pNextBlock->m_pPrevBlock = pEndBlock;

					while (pCurrent != pInsertInto)
					{
						++m_pHeap->m_nBlocks;

						pCurrent->SetMemory((aint)pCurrent->GetMemory() + Offset);
						pCurrent = pCurrent->m_pNextBlock;
					}

					// Create a block to align if needed

					if (!pNextBlock)
					{
						// Find next used alignment
						Alignment >>= 1;
						
						for (; Alignment >= m_pHeap->m_Align; Alignment >>= 1)
						{
							pNextBlock = pInsertInto;

							while (pNextBlock && (!pNextBlock->m_pBlockInfo || m_pHeap->GetBlockAlignment(pNextBlock) != Alignment))
							{
								// Find the first correct block
								pNextBlock = m_pHeap->GetNextRealBlock(pNextBlock);
							}
							if (pNextBlock)
								break;
						}
						pNextRealAllocBlock = pNextBlock;
					}

			
					mint EndMemory = pInsertInto->GetMemory() + RealSize;

					if (pNextBlock && (EndMemory & (Alignment - 1)))
					{
						mint MemoryPos = (EndMemory + (Alignment - 1)) & (~(Alignment - 1));
						mint BlockSize = MemoryPos - EndMemory;
						InsertSize -= BlockSize;
						ToFixMem += BlockSize;

						// Pre block
						{
							// Add remainder of block to free blocks
							CBlock * pNewBlock = m_pHeap->m_BlockPool.New();
							++m_pHeap->m_nBlocks;
							
							pNewBlock->SetMemory(EndMemory);
							pNewBlock->m_pPrevBlock = pInsertInto->m_pPrevBlock;
							if (pNewBlock->m_pPrevBlock)
								pNewBlock->m_pPrevBlock->m_pNextBlock = pNewBlock;
							else
								m_pHeap->m_pFirstBlock = pNewBlock;

							pNewBlock->m_pNextBlock = pInsertInto;
							pInsertInto->m_pPrevBlock = pNewBlock;
							
							pNewBlock->m_MidBlock = 0;
							m_pHeap->AddFreeBlock(pNewBlock, BlockSize);

							++m_pHeap->m_nBlocks;
							CBlock * pNewDummy = m_pHeap->m_BlockPool.New();
							pNewDummy->m_pBlockInfo = (CPS3_Heap::CBlockInfo *)(1);
							pNewDummy->SetMemory(MemoryPos);
							pNewDummy->m_pPrevBlock = pNewBlock;
							pNewDummy->m_pNextBlock = pNewBlock->m_pNextBlock;
							if (pNewDummy->m_pPrevBlock)
								pNewDummy->m_pPrevBlock->m_pNextBlock = pNewDummy;
							else
								m_pHeap->m_pFirstBlock = pNewDummy;
							if (pNewDummy->m_pNextBlock)
								pNewDummy->m_pNextBlock->m_pPrevBlock = pNewDummy;

							{
								CMidBlock *pNewMid = m_MidBlockPool.New();
								pNewMid->m_pBlock = pNewBlock;
								pNewMid->UpdateVars(m_pHeap);
								m_MidBlocks.f_Insert(pNewMid);
							}
						}
						EndMemory = MemoryPos;
					}

					pInsertInto->SetMemory(EndMemory);
					InsertSize -= RealSize;
					InsertSize = m_pHeap->GetBlockSize(pInsertInto);


					m_pHeap->AddFreeBlock(pInsertInto, InsertSize);
#ifdef MCHECKHEAP
					m_pHeap->CheckHeap(3);
#endif
					pStartBlock = pNextRealAllocBlock;
				}
				else
				{
//					M_TRACEALWAYS("Skipping 0x%08x + %d bytes\n", pCurrent->GetMemory(), m_pHeap->GetBlockSize(pCurrent));
					pInsertInto = pNextRealBlock;
					pStartBlock = pNextRealAllocBlock;
				}

//				TraceFreeBlocks();

				pCurrent = pNextBlock;
			}
		}

#ifdef MCHECKHEAP
		m_pHeap->CheckHeap(3);
#endif

		DeleteMidBlocks();
		// Remove dummy blocks
		CBlock *pTemp = m_pHeap->m_pFirstBlock;
		while (pTemp)
		{
			CBlock *pToDelete = pTemp;
			pTemp = pTemp->m_pNextBlock;

			if (m_pHeap->GetBlockSize(pToDelete) == 0)
				m_pHeap->FreeInternal(pToDelete, false);
		}

		M_TRACEALWAYS("Defrag fix could yield another %d bytes\n", ToFixMem);

#ifdef MCHECKHEAP
		m_pHeap->CheckHeap(3);
#endif
	}

	void TraceFreeBlocks()
	{
		m_pHeap->TraceBlocks();
	}

};

void CPS3_Heap::TraceBlocks()
{
	// Remove dummy blocks
	CBlock *pTemp = m_pFirstBlock;
	while (pTemp)
	{
		if (pTemp->m_FreeLink.IsInList())
			M_TRACEALWAYS("0x%08x %d - %d bytes \n", pTemp->GetMemory(), pTemp->m_MidBlock, GetBlockSize(pTemp));
		else
			M_TRACEALWAYS("0x%08x %d + %8d bytes Align: %d bytes %d\n", pTemp->GetMemory(), pTemp->m_MidBlock, GetBlockSize(pTemp), GetBlockAlignment(pTemp));
		pTemp = pTemp->m_pNextBlock;
	}
}

void CPS3_Heap::Defrag()
{
	return;
#ifndef PLATFORM_PS3
	return;
#endif


	CMTime MemMoveTime;
	CMTime XMemCpyExtendedTime;
	CMTime Timer;
	CMTime Timer2;
	{
		TMeasure(Timer)
		DLock(CRCPS3GCM::ms_This.m_pTC->m_DeleteContainerLock);
		{
			DLock(CDCPS3GCM::ms_This);
			{
				DLock(CRCPS3GCM::ms_This.m_ContextTexture.m_DynamicLoadListLock);
				{

					DLock(m_Lock);

//					CRCPS3GCM::ms_This.m_Context_Attrib.m_bNeedVertexProgramUpdate = true;
//					CRCPS3GCM::ms_This.m_Context_Attrib.m_bPixelShaderCleared = true;
//					CRCPS3GCM::ms_This.m_AttribChanged = 0xffFFffFF;
//					D3DDevice_SetPixelShader(CRCPS3GCM::ms_This.m_pDevice, NULL);
//					CRCPS3GCM::ms_This.m_Context_Attrib.m_pLastSetVP = (CRenderXenon_VertexProgram *)1;
//					CRCPS3GCM::ms_This.m_Context_Render.m_iLastVBGeomID = -1;
//					CRCPS3GCM::ms_This.m_Context_Render.m_pCurrentDecl = (CContext_Render_VertexDecl *)1;
//					for (int i = 0; i < CRC_MAXTEXTURES; ++i)
//						CRCPS3GCM::ms_This.m_Context_Texture.m_iLastTexture[i] = -1;

					// Make pages read/write
					{
						TMeasure(Timer2);
						CDefragHelper Helper(this);
#ifdef MCHECKHEAP
						CheckHeap(1, true);
#endif
						Helper.FindMidBlocksAndEvacuate();
#ifdef MCHECKHEAP
						CheckHeap(1, true);
#endif
						Helper.FillMidBlocks();
#ifdef MCHECKHEAP
						CheckHeap(1, true);
#endif
						Helper.CoalesceAndSortByAlignment();
//						Helper.TraceFreeBlocks();
#ifdef MCHECKHEAP
						CheckHeap(1, true);
#endif
						MemMoveTime = Helper.m_MemMoveTime;
						XMemCpyExtendedTime = Helper.m_XMemCpyExtended;
					}


					CDCPS3GCM::ms_This.PostDefrag();
					CRCPS3GCM::ms_This.m_ContextGeometry.PostDefrag();
					CRCPS3GCM::ms_This.m_ContextFP.PostDefrag();
					CRCPS3GCM::ms_This.m_ContextVP.PostDefrag();
					CRCPS3GCM::ms_This.m_ContextTexture.PostDefrag();
//					CRCPS3GCM::ms_This.m_ContextAttrib.PostDefrag();
//					CRCPS3GCM::ms_This.m_ContextRender.PostDefrag();
				}
			}
		}
	}
	M_TRACEALWAYS("%s\n%s\n%s\n%s\n", TString("Defrag time:", Timer).Str(), TString("	DefragHelper time", Timer2).Str(), TString("       memmove time", MemMoveTime).Str(), TString("       XMemCpyExtended time", XMemCpyExtendedTime).Str());
	
	
}



void CPS3_Heap::MoveBlock(void *_To, void*_From, mint _Size)
{
	memmove(_To, _From, _Size);
}

void CPS3_Heap::AddFreeBlock(CBlock *_pBlock, mint _Size)
{
	CPS3_Heap::CBlockSize *pSizeClass = m_FreeSizeTree.FindEqual(_Size);
	if (!pSizeClass)
	{
		pSizeClass = m_BlockSizePool.New();
		pSizeClass->m_Size = _Size;
		m_FreeSizeTree.f_Insert(pSizeClass);
	}
	pSizeClass->AddBlock(_pBlock);
}

void CPS3_Heap::RemoveFreeBlock(CBlock *_pBlock)
{
	DLock(m_Lock);
	if (_pBlock->m_FreeLink.IsAloneInList())
	{				
		mint Size = GetBlockSize(_pBlock);
		CBlockSize *pSizeClass = (CBlockSize *)m_FreeSizeTree.FindEqual(Size);
		MHEAPASSERT(pSizeClass && pSizeClass->m_FreeList.GetFirst() == _pBlock);
		_pBlock->m_FreeLink.Unlink();
		m_FreeSizeTree.f_Remove(pSizeClass);
		m_BlockSizePool.Delete(pSizeClass);					
	}
	else
	{
		_pBlock->m_FreeLink.Unlink();
	}
}

CPS3_Heap::CBlockSize *CPS3_Heap::GetSizeBlock(mint _BlockSize)
{
	DLock(m_Lock);
	CBlockSize *pSizeClass = m_FreeSizeTree.FindEqual(_BlockSize);

	if (!pSizeClass)
	{
		pSizeClass = m_BlockSizePool.New();
		pSizeClass->m_Size = _BlockSize;
		m_FreeSizeTree.f_Insert(pSizeClass);
	}
	return pSizeClass;
}

void CPS3_Heap::UnlinkBlock(CBlock *_pBlock)
{
	if (_pBlock->m_pPrevBlock)
		_pBlock->m_pPrevBlock->m_pNextBlock = _pBlock->m_pNextBlock;
	else
		m_pFirstBlock = _pBlock->m_pNextBlock;

	if (_pBlock->m_pNextBlock)
		_pBlock->m_pNextBlock->m_pPrevBlock = _pBlock->m_pPrevBlock;
}

void CPS3_Heap::DeleteBlock(CBlock *_pBlock)
{
	DLock(m_Lock);
	UnlinkBlock(_pBlock);
	m_BlockPool.Delete(_pBlock);
	--m_nBlocks;
}

mint CPS3_Heap::GetBlockSize(CBlock *_pBlock)
{		
	DLock(m_Lock);
	if (_pBlock->m_pNextBlock)
		return (uint8 *)_pBlock->m_pNextBlock->GetMemory() - (uint8 *)_pBlock->GetMemory();
	else
		return (uint8 *)m_pBlockEnd - (uint8 *)_pBlock->GetMemory();
}


};	// namespace NRenderPS3GCM
