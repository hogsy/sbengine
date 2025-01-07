
#ifndef __MRENDERPS3_HEAP_H_INCLUDED
#define __MRENDERPS3_HEAP_H_INCLUDED

namespace NRenderPS3GCM
{


class CDefragHelper;
class CPS3_Heap
{
	friend class CDefragHelper;
private:
	class CBlockSize;

public:
	static CPS3_Heap ms_This;


	class CBlockInfo
	{
	public:
		virtual uint32 GetAlignment() pure;
		virtual uint32 GetRealSize() {return 0;}
	};

	class CBlock
	{
		friend class CPS3_Heap;
		friend class CBlockSize;
	public:
		DLinkDS_Link(CBlock, m_FreeLink);
		CBlock *m_pPrevBlock;
		CBlock *m_pNextBlock; // 8 bytes here
		CBlock()
		{
			m_MidBlock = false;
			m_pBlockInfo = NULL;
			m_MidBlockStart = false;
		}
	public:
		uint32 m_Memory0:30; // Aligned on
		uint32 m_MidBlock:1; // Aligned on
		uint32 m_MidBlockStart:1; // Aligned on
		mint GetMemory() const
		{
			return m_Memory0 << 2;
		}
		void SetMemory(mint _Memory)
		{
			m_Memory0 = _Memory >> 2;
		}
		CBlockInfo *m_pBlockInfo;
	};

private:

	TCPool<CBlock> m_BlockPool;

	class CBlockSize
	{
	public:

		static CBlockSize *BlockSizeFromFreeList(void *_pFreeList)
		{
			CBlockSize* pZero = 0;
			int Offset = (uint8*)&pZero->m_FreeList - (uint8*)(pZero);

			return (CBlockSize *)((uint8*)_pFreeList - Offset);
		}

		class CCompare
		{
		public:
			DIdsPInlineS static aint Compare(const CBlockSize *_pFirst, const CBlockSize *_pSecond, void *_pContext)
			{
				return (aint)_pFirst->m_Size - (aint)_pSecond->m_Size;
			}

			DIdsPInlineS static aint Compare(const CBlockSize *_pTest, mint _Key, void *_pContext)
			{
				return (aint)_pTest->m_Size - (aint)_Key;
			}
		};

		DAVLAligned_Link(CBlockSize, m_AVLLink, mint, CCompare);

		DLinkDS_List(CBlock, m_FreeLink) m_FreeList;
		mint m_Size;

		CBlockSize()
		{
		}
		~CBlockSize()
		{
			M_ASSERT(m_FreeList.IsEmpty(), "MUust");
		}

		CBlock *GetBlock(mint _Alignment, bint _bAllowMidBlocks)
		{
			if (_bAllowMidBlocks)
			{
				DLinkDS_Iter(CBlock, m_FreeLink) Iter = m_FreeList;
				while (Iter)
				{
					CBlock *pBlock = Iter;

					if (!(pBlock->GetMemory() & (_Alignment - 1)))
					{
						return pBlock;
					}
					++Iter;
				}
			}
			else
			{
				DLinkDS_Iter(CBlock, m_FreeLink) Iter = m_FreeList;
				while (Iter)
				{
					CBlock *pBlock = Iter;

					if (!(pBlock->GetMemory() & (_Alignment - 1)) && !pBlock->m_MidBlock)
					{
						return pBlock;
					}
					++Iter;
				}
			}

			return NULL;
		}


		void AddBlock(CBlock *_pBlock)
		{
//			m_FreeList.InsertSorted(_pBlock);
			m_FreeList.Push(_pBlock);
		}
	};

	TCPool<CBlockSize> m_BlockSizePool;

	DAVLAligned_Tree(CBlockSize, m_AVLLink, mint, CBlockSize::CCompare) m_FreeSizeTree;
	typedef DIdsTreeAVLAligned_Iterator(CBlockSize, m_AVLLink, mint, CBlockSize::CCompare) CSizeTreeIter;
	NThread::CMutual m_Lock;

public:

	CPS3_Heap()
	{
		m_pBlockStart = 0;
		m_pBlockEnd = 0;
		m_pFirstBlock = 0;
		m_Align = 0;
		m_bAutoDefrag = 0;
		m_FreeMemory = 0;
		m_MemorySavedMidBlock = 0;
		m_nBlocks = 0;
	}

	void *m_pBlockStart;
	void *m_pBlockEnd;
	CBlock *m_pFirstBlock;
	mint m_Align;
	mint m_bAutoDefrag:1;
	mint m_FreeMemory;
	mint m_MemorySavedMidBlock;
	mint m_nBlocks;
	int m_CheckHeapLevel;

	void MoveBlock(void *_To, void*_From, mint _Size);

	void RemoveFreeBlock(CBlock *_pBlock);
	void AddFreeBlock(CBlock *_pBlock, mint _Size);

	CBlockSize *GetSizeBlock(mint _BlockSize);

	void DeleteBlock(CBlock *_pBlock);
	void UnlinkBlock(CBlock *_pBlock);

	void CheckHeap(int _Level, bint _bCheckAlignment = false);

	CBlock *GetNextRealBlockMidBlock_r(CBlock *_pBlock, bint _bCheckMidBlock = true);
	CBlock *GetNextRealBlock(CBlock *_pBlock);
public:
	const char* GetStatus();

	mint GetFreeMemory()
	{
		DLock(m_Lock);
		return m_FreeMemory;
	}
	mint GetLargestFreeBlock()
	{
		DLock(m_Lock);
		CBlockSize *pLargest = m_FreeSizeTree.FindLargest();
		if (pLargest)
		{
			return pLargest->m_Size;
		}
		return 0;
	}

	mint GetSavedMidBlock()
	{
		DLock(m_Lock);
		return m_MemorySavedMidBlock;
	}

	void Lock()
	{
		m_Lock.Lock();
	}

	void Unlock()
	{
		m_Lock.Unlock();
	}

	mint GetBlockAlignment(CBlock *_pBlock)
	{
		return Max((uint32)_pBlock->m_pBlockInfo->GetAlignment(), (uint32)m_Align);
	}

	void Defrag();

	mint GetBlockSize(CBlock *_pBlock);

	void TraceBlocks();

	CBlock *AllocInternalFromBlock(CBlock *_pBlock, CBlock *_pAllocTo, mint _Size, mint _OrininalSize, mint _Alignment);
	CBlock *AllocInternal(CBlock *_pBlock, mint _Size, mint _Alignment, bint _bUseMidBlocks);
	CBlock *Alloc(mint _Size, mint _Alignment, CBlockInfo *_pBlockInfo, bint _bUseMidBlocks = true);
//	void FreeMidBlock(CBlock *_pBlock, mint _Memory, mint _SavedSize);
	CBlock *FreeMidBlocks_r(CBlock *_pBlock, CBlockInfo *_pInfo, CBlock *_pEndBlock);
	CBlock *FreeMidBlocks_r2(CBlock *_pBlock);
	void FreeMidBlocks(CBlock *_pBlock);
	void FreeInternal(CBlock *&_pBlock, bint _bKeepBlock);
	void Free(CBlock *&_pBlock);
	CBlock *ReturnMidBlock(CBlock *_pBlock, mint _Memory, mint _Size, bint _bFirstBlock);
	void Create(void *_pMemory, mint _Blocksize, mint _Alignment, bool _bAutoDefrag);

	uint GetHeapStart() {return (uint&)m_pBlockStart;}
	uint GetHeapEnd() {return (uint&)m_pBlockEnd;}

};

}; // namespace NRenderPS3GCM

#endif
