
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



namespace NIds
{
	namespace NMem
	{		

		class CPoolType_Growing
		{
		public:

			template <typename t_CAllocator>
			class TCBlock
			{
			public:
				DIdsListLinkAllocatorSA_Link(TCBlock, m_FreeLink, t_CAllocator);
			};

			template <typename t_CAllocator, mint t_DataSize, mint t_GrowSize, mint t_Alignment>
			class TCChunk
			{
			public:
				typedef TCChunk CChunk;
				typedef typename t_CAllocator::CPtrHolder CPtrHolder;
				DIdsListLinkAllocatorS_Link(CChunk, m_Link, t_CAllocator);

				enum 
				{
					EDataSizeRaw = t_DataSize < sizeof(TCBlock<t_CAllocator>) ? sizeof(TCBlock<t_CAllocator>) : t_DataSize,
					EDataSize = ((EDataSizeRaw + t_Alignment - 1) / t_Alignment) * t_Alignment
				};

				DIdsPInlineL TCChunk(DIdsListLinkAllocatorSA_ListNoLastPtr_FromTemplate(TCBlock<t_CAllocator>, m_FreeLink, t_CAllocator) &_Blocks, mint _Size)
				{
					mint ThisSize = t_CAllocator::Size(this);
					if (!ThisSize)
						ThisSize = _Size;

					uint8 *_pEndBlock = AlignUp((uint8 *)(this + 1), t_Alignment);
					uint8 *_pCurrentBlock = (_pEndBlock + (((((uint8 *)(this) + ThisSize) - _pEndBlock) / EDataSize) * EDataSize)) - EDataSize;
					
					while (_pCurrentBlock >= _pEndBlock)
					{
						TCBlock<t_CAllocator> *pToInsert = (TCBlock<t_CAllocator> *)_pCurrentBlock;
						_Blocks.UnsafeInsertHead(pToInsert);
						_pCurrentBlock -= EDataSize;
					}
				}

				~TCChunk()
				{
				}
         	};


			template <typename t_CAllocator, mint t_GrowSize, mint t_DataSize, mint t_Alignment>
			class TCPoolImp
			{
			public:
				typedef TCChunk<t_CAllocator, t_DataSize, t_GrowSize, t_Alignment> CChunk;
				typedef typename t_CAllocator::CPtrHolder CPtrHolder;

				DIdsListLinkAllocatorSA_ListNoLastPtr_FromTemplate(CChunk, m_Link, t_CAllocator) m_Chunks;
				DIdsListLinkAllocatorSA_ListNoLastPtr_FromTemplate(TCBlock<t_CAllocator>, m_FreeLink, t_CAllocator) m_FreeBlocks;
#				ifdef DIdsTreeCheckRemove
					aint m_NumUsed;
#				endif
				
				enum 
				{
					EDataSizeRaw = t_DataSize < sizeof(TCBlock<t_CAllocator>) ? sizeof(TCBlock<t_CAllocator>) : t_DataSize,
					EDataSize = ((EDataSizeRaw + t_Alignment - 1) / t_Alignment) * t_Alignment
				};

				void Construct()
				{
					m_Chunks.Construct();
					m_FreeBlocks.Construct();
#					ifdef DIdsTreeCheckRemove
						m_NumUsed = 0;
#					endif
				}

				void Destruct()
				{
#					ifdef DIdsTreeCheckRemove
						DIdsAssert(m_NumUsed == 0, "Memory leak in pool!");
#					endif
					// Make Freeblocks delete faster
					m_FreeBlocks.m_Data.GetFirst().m_pNext = DNP;
					m_FreeBlocks.m_Data.SetLast(DNP);
					m_FreeBlocks.Destruct();

					CChunk *pChunkToDelete = m_Chunks.Pop();
					while (pChunkToDelete )
					{
						pChunkToDelete->~CChunk();
						t_CAllocator::Free(pChunkToDelete);						
						pChunkToDelete = m_Chunks.Pop();
					}
					m_Chunks.Destruct();
				}

				void AddChunk(void *_pMem, mint _Size)
				{
					CChunk *pNewChunk = new(_pMem) CChunk(m_FreeBlocks, _Size);
					m_Chunks.Insert(pNewChunk);
				}
				
				void *GetBlock()
				{
					TCBlock<t_CAllocator> * pBlock = m_FreeBlocks.Pop();
					
					if (pBlock)
					{
#						ifdef DIdsTreeCheckRemove
							++m_NumUsed;
#						endif
						return pBlock;
					}
					else
					{
	
						mint Size = ((((sizeof(CChunk) + t_GrowSize * EDataSize) - 1) / t_CAllocator::GranularityAlloc()) + 1) * t_CAllocator::GranularityAlloc();
						void *pMem = t_CAllocator::AllocAlign(Size, t_Alignment);

						if (pMem)
						{	
							AddChunk(pMem, Size);
							pBlock = m_FreeBlocks.Pop();
							DIdsAssert(pBlock, "Must succed");
#							ifdef DIdsTreeCheckRemove
								++m_NumUsed;
#							endif
							return pBlock;
						}
						else
						{
							return DNP;
						}
					}
				}
				
				bint BlockIsAllocated(void *_pBlock)
				{
					M_BREAKPOINT; // Not supported for this pool type
					return true;
				}
				void ReturnBlock(void *_pBlock)
				{
					TCBlock<t_CAllocator> *pBlock = (TCBlock<t_CAllocator> *)_pBlock;
					// We are adding to a full block
					m_FreeBlocks.Push(pBlock);
#					ifdef DIdsTreeCheckRemove
						--m_NumUsed;
#					endif
				}

			};			

		};

		class CPoolType_Freeable
		{
		public:

			class CPool;

			template <typename t_CAllocator>
			class TCBlock
			{
			public:
				DIdsListLinkS_Trans(TCBlock, m_FreeLink);
				union
				{
					void *m_pChunk;
					DIdsListLinkAllocatorSA_Member(m_FreeLink, t_CAllocator);
				};
#ifdef M_Profile
				uint32 m_bAllocated;
#endif
			};

			template <typename t_CAllocator, mint t_DataSize, mint t_GrowSize, mint t_Alignment>
			class TCChunk
			{
			public:
				typedef TCChunk CChunk;
				typedef typename t_CAllocator::CPtrHolder CPtrHolder;
				DIdsListLinkAllocatorD_Link(CChunk, m_Link, t_CAllocator);
				DIdsListLinkAllocatorS_ListNoLastPtr_FromTemplate(TCBlock<t_CAllocator>, m_FreeLink, t_CAllocator) m_FreeBlocks;
				aint m_NumUsed;

				enum
				{
					EDataSizeRaw = sizeof(TCBlock<t_CAllocator>) + t_DataSize,
					EDataSize = ((EDataSizeRaw + t_Alignment - 1) / t_Alignment) * t_Alignment
				};

				DIdsPInlineL TCChunk(mint _Size)
				{
					mint ThisSize = t_CAllocator::Size(this);
					if (!ThisSize)
						ThisSize = _Size;

					uint8 *_pEndBlock = AlignUp((uint8 *)(this + 1), t_Alignment);
					uint8 *_pCurrentBlock = (_pEndBlock + (((((uint8 *)(this) + ThisSize) - _pEndBlock) / EDataSize) * EDataSize)) - EDataSize;

					m_NumUsed = 0;					
					
					while (_pCurrentBlock >= _pEndBlock)
					{
						TCBlock<t_CAllocator> *pToInsert = (TCBlock<t_CAllocator> *)_pCurrentBlock;
#ifdef M_Profile
						pToInsert->m_bAllocated = false;
#endif
						m_FreeBlocks.UnsafeInsertHead(pToInsert);
						_pCurrentBlock -= EDataSize;
					}

				}

				~TCChunk()
				{
					DIdsAssert(m_NumUsed == 0, "Memory leak in pool!");
				}
			};


			template <typename t_CAllocator, mint t_GrowSize, mint t_DataSize, mint t_Alignment>
			class TCPoolImp
			{
			public:
				typedef TCChunk<t_CAllocator, t_DataSize, t_GrowSize, t_Alignment> CChunk;
				typedef typename t_CAllocator::CPtrHolder CPtrHolder;
				DIdsListLinkAllocatorDA_List_FromTemplate(CChunk, m_Link, t_CAllocator) m_Chunks;
				DIdsListLinkAllocatorDA_List_FromTemplate(CChunk, m_Link, t_CAllocator) m_FreeChunks;
				DIdsListLinkAllocatorDA_List_FromTemplate(CChunk, m_Link, t_CAllocator) m_EmptyChunks;
				
				enum
				{
					EDataSizeRaw = sizeof(TCBlock<t_CAllocator>) + t_DataSize,
					EDataSize = ((EDataSizeRaw + t_Alignment - 1) / t_Alignment) * t_Alignment
				};

				void Construct()
				{
					m_Chunks.Construct();
					m_FreeChunks.Construct();
					m_EmptyChunks.Construct();
				}

				void Destruct()
				{
					DIdsAssert(m_Chunks.IsEmpty(), "Memory leak");
					DIdsAssert(m_FreeChunks.IsEmpty(), "Memory leak");

					CChunk *pChunkToDelete = m_Chunks.Pop();
					while (pChunkToDelete)
					{
						pChunkToDelete->~CChunk();
						t_CAllocator::Free(pChunkToDelete);
						pChunkToDelete = m_Chunks.Pop();
					}
					pChunkToDelete = m_FreeChunks.Pop();
					while (pChunkToDelete)
					{
						pChunkToDelete->~CChunk();
						t_CAllocator::Free(pChunkToDelete);
						pChunkToDelete = m_FreeChunks.Pop();
					}
					pChunkToDelete = m_EmptyChunks.Pop();
					while (pChunkToDelete)
					{
						pChunkToDelete->~CChunk();
						t_CAllocator::Free(pChunkToDelete);
						pChunkToDelete = m_EmptyChunks.Pop();
					}
					m_Chunks.Destruct();
					m_FreeChunks.Destruct();
					m_EmptyChunks.Destruct();
				}

				bint ContainsBlock(void *_pBlock)
				{
					DIdsListLinkAllocatorDA_List_FromTemplate(CChunk, m_Link, t_CAllocator) *lLists[] = {&m_Chunks, &m_FreeChunks, &m_EmptyChunks};

					for (mint i = 0; i < sizeof(lLists) / sizeof(DIdsListLinkAllocatorDA_List_FromTemplate(CChunk, m_Link, t_CAllocator) *); ++i)
					{
						DIdsListLinkAllocatorD_Iter_FromTemplate(CChunk, m_Link, t_CAllocator) Iter = *lLists[i];
						while (Iter)
						{
							if ((mint)_pBlock >= (mint)(CChunk *)Iter && (mint)_pBlock < (mint)(CChunk *)Iter + t_CAllocator::Size((CChunk *)Iter))
								return true;
							++Iter;
						}
					}

					return false;				
				}
				
				void *GetBlock()
				{
					DIdsAssert(t_GrowSize > 1, "GrowSize must be larger that 1, or the logic does not work");
//					DIdsListLinkD_Iter(TCChunk<>, m_Link) Iter = m_FreeChunks;

					CChunk *pChunk = m_FreeChunks.GetFirst();
					
					TCBlock<t_CAllocator> * pBlock = DNP;
					
					if (pChunk)
					{
						pBlock = pChunk->m_FreeBlocks.Pop();
					}

					if (!pBlock)
					{
						pChunk = m_EmptyChunks.GetFirst();
						if (pChunk)
						{
							pBlock = pChunk->m_FreeBlocks.Pop();
						}
					}
					
					
					if (pBlock)
					{
						if (pChunk->m_FreeBlocks.IsEmpty())
						{
							// Full block
							m_Chunks.Insert(pChunk);
						}
						else if (pChunk->m_NumUsed == 0)
						{
							m_FreeChunks.Insert(pChunk);
						}
						++pChunk->m_NumUsed;
						pBlock->m_pChunk = pChunk;
#ifdef M_Profile
						pBlock->m_bAllocated = true;
#endif
						return ((uint8*)(pBlock + 1));
					}
					else
					{
						mint Size = ((((sizeof(CChunk) + t_GrowSize * EDataSize) - 1) / t_CAllocator::GranularityAlloc()) + 1) * t_CAllocator::GranularityAlloc();
						void *pMem = t_CAllocator::AllocAlign(Size, t_Alignment);

						if (pMem)
						{	
							CChunk *pNewChunk = new(pMem) CChunk(Size);

							pBlock = pNewChunk->m_FreeBlocks.Pop();
							DIdsAssert(pBlock, "Must succed");
							m_FreeChunks.Insert(pNewChunk);
							
							++pNewChunk->m_NumUsed;
							pBlock->m_pChunk = pNewChunk;
#ifdef M_Profile
							pBlock->m_bAllocated = true;
#endif
							return ((uint8*)(pBlock + 1));
						}
						else
						{
							return DNP;
						}
					}
				}

				bint BlockIsAllocated(void *_pBlock)
				{
#ifdef M_Profile
					TCBlock<t_CAllocator> *pBlock = (TCBlock<t_CAllocator> *)((uint8 *)_pBlock - (sizeof(TCBlock<t_CAllocator>)));
					return pBlock->m_bAllocated;
#else
					M_BREAKPOINT; // Not supported
					return true;				
#endif
				}

				void ReturnBlock(void *_pBlock)
				{
#ifdef _DEBUG
					DIdsAssert(ContainsBlock(_pBlock), "Must be part of pool");
#endif
					TCBlock<t_CAllocator> *pBlock = (TCBlock<t_CAllocator> *)((uint8 *)_pBlock - (sizeof(TCBlock<t_CAllocator>)));
#ifdef M_Profile
					pBlock->m_bAllocated = false;
#endif
					CChunk *pChunk = ((CChunk *)(pBlock->m_pChunk));
					if (pChunk->m_FreeBlocks.IsEmpty())
					{
						// We are adding to a full block
						pChunk->m_FreeBlocks.Push(pBlock);
						--pChunk->m_NumUsed;
						m_FreeChunks.InsertHead(pChunk);
					}
					else
					{						
						pChunk->m_FreeBlocks.Push(pBlock);
						--pChunk->m_NumUsed;
						
						if (pChunk->m_NumUsed == 0)
						{
							if (!m_EmptyChunks.IsEmpty())
							{
								// Remove the excess chunk
								CChunk *pChunkToDelete = m_EmptyChunks.Pop();
								pChunkToDelete->~CChunk();
								t_CAllocator::Free(pChunkToDelete);
							}
							m_EmptyChunks.Insert(pChunk);
						}
					}
				}

			};			

		};

		class CAggregateData
		{
		public:

			mint m_bDoneInit;

			DIdsPInlineS void InitDone()
			{
				m_bDoneInit = true;

			}

			DIdsPInlineS bint ShouldInit()
			{
				if (m_bDoneInit)
					return false;
				else
					return true;
			}
		};

		template <typename t_CData, aint t_GrowSize = 128, typename t_CLockType = NThread::CLock, typename t_CPoolType = CPoolType_Freeable, typename t_CAllocator = CAllocator_Virtual, typename t_CAggregateData = CAggregateData>
		class TCPoolAggregate
		{
		public:
			t_CAggregateData m_AggregateData;
			t_CLockType m_Lock;
			typename t_CPoolType::template TCPoolImp<t_CAllocator, t_GrowSize, sizeof(t_CData), M_ALIGNMENTOF(t_CData)> m_Pool;

			void Construct()
			{
				m_Pool.Construct();
				m_Lock.Construct();
			}

			void Destruct()
			{
				if (!m_AggregateData.ShouldInit())
				{
					m_Pool.Destruct();
					m_Lock.Destruct();
				}
			}

			DIdsPInlineS void Delete(t_CData *_pToDelete)
			{
				_pToDelete->~t_CData();
//				delete _pToDelete;
				DLockTyped_FromTemplate(t_CLockType, m_Lock);
				ReturnBlock(_pToDelete);
			}


			t_CData *New()
			{
				void *pMem;
				{
					DLockTyped_FromTemplate(t_CLockType, m_Lock);
					pMem = GetBlock();
				}
				t_CData *pObject = new(pMem) t_CData;
				return pObject;
			}

			template <typename t_CParam0>
			t_CData *New(t_CParam0 &_Param0)
			{
				void *pMem;
				{
					DLockTyped_FromTemplate(t_CLockType, m_Lock);
					pMem = GetBlock();
				}
				t_CData *pObject = new(pMem) t_CData(_Param0);
				return pObject;
			}

			template <typename t_CParam0, typename t_CParam1>
			t_CData *New(t_CParam0 &_Param0, t_CParam1 &_Param1)
			{
				void *pMem;
				{
					DLockTyped_FromTemplate(t_CLockType, m_Lock);
					pMem = GetBlock();
				}
				t_CData *pObject = new(pMem) t_CData(_Param0, _Param1);
				return pObject;
			}

			template <typename t_CParam0, typename t_CParam1, typename t_CParam2>
			t_CData *New(t_CParam0 &_Param0, t_CParam1 &_Param1, t_CParam2 &_Param2)
			{
				void *pMem;
				{
					DLockTyped_FromTemplate(t_CLockType, m_Lock);
					pMem = GetBlock();
				}
				t_CData *pObject = new(pMem) t_CData(_Param0, _Param1, _Param2);
				return pObject;
			}

			template <typename t_CParam0, typename t_CParam1, typename t_CParam2, typename t_CParam3>
			t_CData *New(t_CParam0 &_Param0, t_CParam1 &_Param1, t_CParam2 &_Param2, t_CParam3 &_Param3)
			{
				void *pMem;
				{
					DLockTyped_FromTemplate(t_CLockType, m_Lock);
					pMem = GetBlock();
				}
				t_CData *pObject = new(pMem) t_CData(_Param0, _Param1, _Param2, _Param3);
				return pObject;
			}

			DIdsPInlineM void *GetBlock()
			{
				if (m_AggregateData.ShouldInit())
				{
					m_Pool.Construct();
					m_AggregateData.InitDone();
				}

				void *pBlock = m_Pool.GetBlock();

				DIdsAssert(pBlock, "Memory error ??");

				return pBlock;
			}

			DIdsPInlineS void ReturnBlock(void * _pBlock)
			{
				m_Pool.ReturnBlock(_pBlock);
			}

			DIdsPInlineS bint BlockIsAllocated(t_CData *_pBlock)
			{
				return m_Pool.BlockIsAllocated(_pBlock);
			}

		};

#		define DIdsMemPoolStaticImpl(_PoolType, _PoolName) _PoolType _PoolName = {0};

#		ifndef DIdsPNoShortCuts
#			define DPoolStaticImpl(_PoolType, _PoolName) DIdsMemPoolStaticImpl(_PoolType, _PoolName)
#		endif

		class CNoAggregateData
		{
		public:
			DIdsPInlineS static void InitDone()
			{
			}

			DIdsPInlineS static bint ShouldInit()
			{
				return false;
			}
		};

		template <typename t_CData, mint t_GrowSize = 128, typename t_CLockType = NThread::CLock, typename t_CPoolType = CPoolType_Freeable, typename t_CAllocator = DIdsDefaultAllocator>
		class TCPool : public TCPoolAggregate<t_CData, t_GrowSize, t_CLockType, t_CPoolType, t_CAllocator, CNoAggregateData>
		{
		public:
			TCPool()
			{
				TCPoolAggregate<t_CData, t_GrowSize, t_CLockType, t_CPoolType, t_CAllocator, CNoAggregateData>::Construct();
			}			
			~TCPool()
			{
				TCPoolAggregate<t_CData, t_GrowSize, t_CLockType, t_CPoolType, t_CAllocator, CNoAggregateData>::Destruct();
			}			
		};

		
		/*»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»»*\
		|	TemplateClass:		Base class for enabling pooling functionality			|
		|																				|
		|	Parameters:																	|
		|		t_CData:		The class that is inheriting from this class			|
		|																				|
		|	Comments:			Make sure that you inherit from this class				|
		|						first, otherwise the delete wont work					|
		\*_____________________________________________________________________________*/
		template <typename t_CPool>
		class TBCPool
		{
		public:
			void * operator new(mint _NumBytes, const t_CPool &_Pool)
			{
				return _Pool.GetBlock(_NumBytes);
			}

#			if (DIdsPCompilerNeedsOperatorDelete != 0)
				void operator delete(void *_pToDelete, const t_CPool &_Pool)
				{
					_Pool.ReturnBlock(_pToDelete);
				}
#			endif // (DIdsPCompilerNeedsOperatorDelete != 0)

			void operator delete(void *_pToDelete)
			{
				// We do nothing here, as we dont know the pool
			}

		};

	};	
};

using namespace NIds::NMem;


class MRTC_ThreadContext
{
public:
	NThread::CMutual m_EventMember_Lock;
	TCPool<NThread::CEventAutoResetReportableAggregate::CReportListMember> m_EventMember_Pool;
};

#endif // __INC_MMEMMGRPOOL
