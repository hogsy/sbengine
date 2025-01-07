
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
|	File:			Short_desscription															|
|																								|
|	Author:			Erik Olofsson																|
|																								|
|	Copyright:		A&O Software Design, 2002													|
|																								|
|	Contents:		A_list_of_classes_functions_etc_defined_in_file								|
|																								|
|	Comments:		Longer_description_not_mandatory											|
|																								|
|	History:																					|
|		020317:		Created File																|
\*_____________________________________________________________________________________________*/

#ifndef DIds_Inc_Mda_Ids_TreeAVL_h
#define DIds_Inc_Mda_Ids_TreeAVL_h

#define DIdsAssert M_ASSERT
#define DIdsPInlineS M_INLINE
#define DIdsPInlineM M_INLINE
#define DIdsPInlineL M_INLINE
#define DIdsPNoInline
#define DIdsPInlineXL
#define DIdsDefaultAllocator NIds::NMem::CAllocator_Heap<0> 

#define DIdsPDebugBreak M_BREAKPOINT

#if defined(_DEBUG)
#define DIdsDebug
#endif

#if defined(_DEBUG) && !defined(M_MIXEDDEBUG)
#define DIdsTreeCheckRemove
#endif

//typedef unsigned int mint;
typedef char ch8;
#define DNP 0

class CDefaultPointerHolder
{
public:
	union
	{
		void *m_pData;
		const char *m_pCharPointerData;
	};

	DIdsPInlineS void * Get() const
	{
		return m_pData;
	}

	DIdsPInlineS void Set(void *_pAddress)
	{
		m_pData = _pAddress;
	}

	DIdsPInlineS void Set(CDefaultPointerHolder &_Address)
	{
		m_pData = _Address.m_pData;
	}        
};

template <typename t_CPointer, typename t_CTyped>
class TCDynamicPtr
{
public:
    t_CPointer m_PtrData;

	// We cannot have consturtors, that would make us non aggregate
/*
	DIdsPInlineS TCDynamicPtr()
	{
	}

	DIdsPInlineS TCDynamicPtr(t_CTyped *_pSetTo)
	{
		m_PtrData.Set((void *)_pSetTo);
	}
	*/

	DIdsPInlineS TCDynamicPtr &operator = (t_CTyped *_pSetTo)
	{
		m_PtrData.Set((void *)_pSetTo);
		return *this;
	}

	DIdsPInlineS bint operator == (t_CTyped *_pCompareTo)
	{
		return (t_CTyped *)m_PtrData.Get() == _pCompareTo;
	}
    
	/*const TCDynamicPtr &operator = (const t_CTyped *_pSetTo)
	{
		m_PtrData = (void *)_pSetTo;
		return *this;
	}*/

	DIdsPInlineS operator t_CTyped * () const
	{			
		return (t_CTyped *)m_PtrData.Get();
	}

	DIdsPInlineS t_CTyped * operator -> () const
	{
		return (t_CTyped *)m_PtrData.Get();
	}
    
};

namespace NIds
{


	namespace NMem
	{
		template <int t_Dummy>
		class CAllocator_Heap
		{
		public:

			DIdsPInlineS static mint StaticAddresses()
			{
				return 0;
			}

			typedef CDefaultPointerHolder CPtrHolder;
			
			DIdsPInlineS static mint GranularityAlloc()
			{
				return 4;
			}

			DIdsPInlineS static mint GranularityCommit()
			{
				return 4096;
			}

			DIdsPInlineS static mint GranularityProtect()
			{
				return 4096;
			}

			DIdsPInlineS static mint Size(void *_pBlock)
			{
#ifdef M_COMPILING_ON_VPU
				M_BREAKPOINT; // Should not be called
				return 0;
#else
				return MRTC_MemSize(_pBlock);
#endif
			}

			DIdsPInlineS static bint CanCommit()
			{
				return false;
			}

			DIdsPInlineS static bint CanProtect()
			{
				return false;
			}

			DIdsPInlineS static bint CanFail()
			{
				return false;
			}

			DIdsPInlineS static void Protect(void *_pMem, mint _Size, auint _Protect)
			{

			}

			DIdsPInlineS static void *AllocDebug(mint _Size, mint _Location, const ch8 *_pFile, aint _Line, aint _Flags = 0)
			{
#ifdef M_COMPILING_ON_VPU
				M_BREAKPOINT; // Should not be called
				return 0;

#else
				return M_ALLOCDEBUG(_Size, _pFile, _Line);
#endif
			}

			DIdsPInlineS static void *Alloc(mint _Size, mint _Location = 0)
			{
#ifdef M_COMPILING_ON_VPU
				M_BREAKPOINT; // Should not be called
				return 0;
#else
				return M_ALLOC(_Size);
#endif
			}

			DIdsPInlineS static void *AllocAlignDebug(mint _Size, mint _Align, mint _Location, const ch8 *_pFile, aint _Line, aint _Flags = 0)
			{
#ifdef M_COMPILING_ON_VPU
				M_BREAKPOINT; // Should not be called
				return 0;
#else
				return M_ALLOCDEBUGALIGN(_Size, _Align, _pFile, _Line);
#endif
			}

			DIdsPInlineS static void *AllocAlign(mint _Size, mint _Align, mint _Location = 0)
			{
#ifdef M_COMPILING_ON_VPU
				M_BREAKPOINT; // Should not be called
				return 0;
#else
				return M_ALLOCALIGN(_Size, _Align);
#endif
			}

			DIdsPInlineS static void *Realloc(void *_pMem, mint _Size, mint _Location = 0, mint _OldSize = 0, mint _OldLocation = 0)
			{
#ifdef M_COMPILING_ON_VPU
				M_BREAKPOINT; // Should not be called
				return 0;
#else
				return M_REALLOC(_pMem, _Size);
#endif
			}

			DIdsPInlineS static void *ReallocDebug(void *_pMem, mint _Size, mint _Location, mint _OldSize, mint _OldLocation, const ch8 *_pFile, aint _Line, aint _Flags = 0)
			{
#ifdef M_COMPILING_ON_VPU
				M_BREAKPOINT; // Should not be called
				return 0;
#else
				return M_REALLOCDEBUG(_pMem, _Size, _pFile, _Line);
#endif
			}

			DIdsPInlineS static void *Resize(void *_pMem, mint _Size, mint _Location = 0, mint _OldSize = 0, mint _OldLocation = 0)
			{
#ifdef M_COMPILING_ON_VPU
				M_BREAKPOINT; // Should not be called
				return 0;
#else
				return M_REALLOC(_pMem, _Size);
#endif
			}

			DIdsPInlineS static void *ResizeDebug(void *_pMem, mint _Size, mint _Location, mint _OldSize, mint _OldLocation, const ch8 *_pFile, aint _Line, aint _Flags = 0)
			{
#ifdef M_COMPILING_ON_VPU
				M_BREAKPOINT; // Should not be called
				return 0;
#else
				return M_REALLOCDEBUG(_pMem, _Size, _pFile, _Line);
#endif
			}

			DIdsPInlineS static void *AllocNoCommit(mint _Size, mint _Location = 0)
			{
#ifdef M_COMPILING_ON_VPU
				M_BREAKPOINT; // Should not be called
				return 0;
#else
				return M_ALLOC(_Size);
#endif
			}

			DIdsPInlineS static void Commit(void *_pMem, mint _Size)
			{
			}

			DIdsPInlineS static void Decommit(void *_pMem, mint _Size)
			{
			}

			DIdsPInlineS static void Free(void *_pBlock, mint _Size = 0, mint _Location = 0)
			{
#ifdef M_COMPILING_ON_VPU
				M_BREAKPOINT; // Should not be called
#else
				MRTC_MemFree(_pBlock);
#endif
			}
		};	

		class CAllocator_Virtual
		{
		public:

			DIdsPInlineS static mint StaticAddresses()
			{
				return 0;
			}

			typedef CDefaultPointerHolder CPtrHolder;
			
			DIdsPInlineS static mint GranularityAlloc()
			{
#ifdef PLATFORM_XENON
				return 128;
#elif defined(PLATFORM_PS3)
				return 128;
#else
				return 4096;
#endif
			}

			DIdsPInlineS static mint GranularityCommit()
			{
				return 4096;
			}

			DIdsPInlineS static mint GranularityProtect()
			{
				return 4096;
			}

			DIdsPInlineS static mint Size(void *_pBlock)
			{
#ifdef M_COMPILING_ON_VPU
				M_BREAKPOINT; // Should not be called
				return 0;

#else
				return MRTC_SystemInfo::OS_MemSize(_pBlock);
#endif
			}

			DIdsPInlineS static bint CanCommit()
			{
				return false;
			}

			DIdsPInlineS static bint CanProtect()
			{
				return false;
			}

			DIdsPInlineS static void Protect(void *_pMem, mint _Size, auint _Protect)
			{

			}

			DIdsPInlineS static void *AllocDebug(mint _Size, mint _Location, const ch8 *_pFile, aint _Line, aint _Flags = 0)
			{
#ifdef M_COMPILING_ON_VPU
				M_BREAKPOINT; // Should not be called
				return 0;

#else
				return MRTC_SystemInfo::OS_Alloc(_Size, 1);
#endif
			}

			DIdsPInlineS static void *Alloc(mint _Size, mint _Location = 0)
			{
#ifdef M_COMPILING_ON_VPU
				M_BREAKPOINT; // Should not be called
				return 0;
#else
				return MRTC_SystemInfo::OS_Alloc(_Size, 1);
#endif
			}

			DIdsPInlineS static void *AllocAlignDebug(mint _Size, mint _Align, mint _Location, const ch8 *_pFile, aint _Line, aint _Flags = 0)
			{
#ifdef M_COMPILING_ON_VPU
				M_BREAKPOINT; // Should not be called
				return 0;
#else
				return MRTC_SystemInfo::OS_Alloc(_Size, _Align);
#endif
			}

			DIdsPInlineS static void *AllocAlign(mint _Size, mint _Align, mint _Location = 0)
			{
#ifdef M_COMPILING_ON_VPU
				M_BREAKPOINT; // Should not be called
				return 0;
#else
				return MRTC_SystemInfo::OS_Alloc(_Size, _Align);
#endif
			}

			DIdsPInlineS static void *Realloc(void *_pMem, mint _Size, mint _Location = 0, mint _OldSize = 0, mint _OldLocation = 0)
			{
				M_BREAKPOINT; // Should not be called
				return 0;
			}

			DIdsPInlineS static void *ReallocDebug(void *_pMem, mint _Size, mint _Location, mint _OldSize, mint _OldLocation, const ch8 *_pFile, aint _Line, aint _Flags = 0)
			{
				M_BREAKPOINT; // Should not be called
				return 0;
			}

			DIdsPInlineS static void *Resize(void *_pMem, mint _Size, mint _Location = 0, mint _OldSize = 0, mint _OldLocation = 0)
			{
				M_BREAKPOINT; // Should not be called
				return 0;
			}

			DIdsPInlineS static void *ResizeDebug(void *_pMem, mint _Size, mint _Location, mint _OldSize, mint _OldLocation, const ch8 *_pFile, aint _Line, aint _Flags = 0)
			{
				M_BREAKPOINT; // Should not be called
				return 0;
			}

			DIdsPInlineS static void *AllocNoCommit(mint _Size, mint _Location = 0)
			{
#ifdef M_COMPILING_ON_VPU
				M_BREAKPOINT; // Should not be called
				return 0;
#else
				return MRTC_SystemInfo::OS_Alloc(_Size, 1);
#endif
			}

			DIdsPInlineS static void Commit(void *_pMem, mint _Size)
			{
			}

			DIdsPInlineS static void Decommit(void *_pMem, mint _Size)
			{
			}

			DIdsPInlineS static void Free(void *_pBlock, mint _Size = 0, mint _Location = 0)
			{
#ifdef M_COMPILING_ON_VPU
				M_BREAKPOINT; // Should not be called
#else
				return MRTC_SystemInfo::OS_Free(_pBlock);
#endif
			}
		};	
	}


	namespace NTree
	{

		template <typename t_CData, typename t_CKey>
		class CTreeCompare_Default
		{
		public:

			DIdsPInlineS static aint Compare(const t_CData *_pFirst, const t_CData *_pSecond, void *_pContext)
			{
				if (*_pFirst > *_pSecond)
					return 1;
				if (*_pFirst < *_pSecond)
					return -1;

				return 0;
			}

			DIdsPInlineS static aint Compare(const t_CData *_pTest, t_CKey &_Key, void *_pContext)
			{
				if (_Key > *_pTest)
					return 1;
				if (_Key < *_pTest)
					return -1;

				return 0;
			}
		};


		/************************************************************************************************\
		||¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯||
		|| Simple linked tree
		||______________________________________________________________________________________________||
		\************************************************************************************************/


		
		/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		|	Template:			This class is a template to make it go through			|
		|						the compiler.											|
		|																				|
		|	Parameters:																	|
		|		t_Dummy:		Not used												|
		\*_____________________________________________________________________________*/
		template <typename t_CPointerHolder = CDefaultPointerHolder>
		class TCAVLLinkData
		{
		public:
			typedef TCDynamicPtr<t_CPointerHolder, TCAVLLinkData> CData;

			enum EAVLTreeSkew
			{
				 EAVLTreeSkew_None		= 2
				,EAVLTreeSkew_Left		= 0
				,EAVLTreeSkew_Right		= 1
				,EAVLTreeSkew_NotInTree	= 3
			};
			CData m_pNext[2];
			aint m_Skew;

			DIdsPInlineS static bint GetPtrIsFast()
			{
				return true;
			}

			DIdsPInlineS void f_Construct()
			{
				m_Skew = EAVLTreeSkew_NotInTree;
			}

			DIdsPInlineS CData &GetRight()
			{
				return m_pNext[1];
			}
			DIdsPInlineS CData &GetLeft()
			{
				return m_pNext[0];
			}

			DIdsPInlineS CData &GetNext(aint _iIndex)
			{
				return m_pNext[_iIndex];
			}

			DIdsPInlineS TCAVLLinkData *GetRightP() const
			{
				return m_pNext[1];
			}
			DIdsPInlineS TCAVLLinkData *GetLeftP() const
			{
				return m_pNext[0];
			}

			DIdsPInlineS TCAVLLinkData *GetNextP(aint _iIndex) const
			{
				return m_pNext[_iIndex];
			}

			DIdsPInlineS aint GetSkew() const
			{
				return m_Skew;
			}

			DIdsPInlineS bint f_IsBalanced() const
			{
				return m_Skew == EAVLTreeSkew_None;
			}


			DIdsPInlineS void SetRight(TCAVLLinkData *_pRight)
			{
				m_pNext[1] = _pRight;
			}

			DIdsPInlineS void SetLeft(TCAVLLinkData *_pLeft)
			{
				m_pNext[0] = _pLeft;
			}

			DIdsPInlineS void SetNext(int _iIndex, TCAVLLinkData *_pPtr)
			{
				m_pNext[_iIndex] = _pPtr;
			}

			DIdsPInlineS static void Assign(CData &_pDest, TCAVLLinkData *_pSrc)
			{
				_pDest = _pSrc;
			}

			DIdsPInlineS static void Assign(CData &_pDest, CData &_pSrc)
			{
				_pDest = _pSrc;
			}

			DIdsPInlineS void SetSkew(mint _Skew)
			{
				m_Skew = _Skew;
			}

			DIdsPInlineS static TCAVLLinkData *GetPtr(const CData &_Src)
			{
				return (TCAVLLinkData *)_Src;
			}

			DIdsPInlineS void SetAll(TCAVLLinkData *_pLeft, TCAVLLinkData *_pRight, mint _Skew)
			{
				m_pNext[0] = _pLeft;
				m_pNext[1] = _pRight;
				m_Skew = _Skew;
			}

			DIdsPInlineS void f_Clear()
			{
				m_pNext[0] = 0;
				m_pNext[1] = 0;
				m_Skew = EAVLTreeSkew_None;
			}

		};

		/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		|	Template:			This class is a template so some more					|
		|						functions can be inlined								|
		|																				|
		|	Parameters:																	|
		|		t_Dummy:		description												|	
		\*_____________________________________________________________________________*/
		template <typename t_CPointerHolder = CDefaultPointerHolder>
		class TCAVLLinkAlignedData
		{
		public:

			typedef TCDynamicPtr<t_CPointerHolder, TCAVLLinkAlignedData> CData;

			enum EAVLTreeSkew
			{
				 EAVLTreeSkew_None		= 2
				,EAVLTreeSkew_Left		= 0
				,EAVLTreeSkew_Right		= 1
				,EAVLTreeSkew_NotInTree	= 3
			};

			CData m_pNext[2];

			DIdsPInlineS static bint GetPtrIsFast()
			{
				return false;
			}

			DIdsPInlineS void f_Construct()
			{
				SetAll(0, 0, EAVLTreeSkew_NotInTree);
			}

			DIdsPInlineS CData &GetRight()
			{
				return m_pNext[1];
			}
			DIdsPInlineS CData &GetLeft()
			{
				return m_pNext[0];
			}

			DIdsPInlineS CData &GetNext(aint _iIndex)
			{
				return m_pNext[_iIndex];
			}

			DIdsPInlineS TCAVLLinkAlignedData *GetRightP() const
			{
				return (TCAVLLinkAlignedData *)((mint)(TCAVLLinkAlignedData *)m_pNext[1] & (~0x1));
			}
			DIdsPInlineS TCAVLLinkAlignedData *GetLeftP() const
			{
				return (TCAVLLinkAlignedData *)((mint)(TCAVLLinkAlignedData *)m_pNext[0] & (~0x1));
			}

			DIdsPInlineS TCAVLLinkAlignedData *GetNextP(aint _iIndex) const
			{
				return (TCAVLLinkAlignedData *)((mint)(TCAVLLinkAlignedData *)m_pNext[_iIndex] & (~0x1));
			}

			DIdsPInlineS mint GetSkew() const
			{
				return ((mint)(TCAVLLinkAlignedData *)m_pNext[0] & 1 | ((mint)(TCAVLLinkAlignedData *)m_pNext[1] & 1) << 1);
			}

			DIdsPInlineS bint f_IsBalanced() const
			{
				return GetSkew() == EAVLTreeSkew_None;
			}


			DIdsPInlineS static TCAVLLinkAlignedData *GetPtr(const CData &_Src)
			{
				return (TCAVLLinkAlignedData *)((mint)(TCAVLLinkAlignedData *)_Src & (~0x1));
			}

			DIdsPInlineS void SetRight(TCAVLLinkAlignedData *_pRight)
			{
				DIdsAssert((!((mint)_pRight & 1)), "We can only save ptrs that are aligned on 2 bytes");
				m_pNext[1] = (TCAVLLinkAlignedData *)(((mint)(TCAVLLinkAlignedData *)m_pNext[1] & 1) | (mint)_pRight);
			}

			DIdsPInlineS void SetLeft(TCAVLLinkAlignedData *_pLeft)
			{
				DIdsAssert(!((mint)_pLeft & 1), "We can only save ptrs that are aligned on 2 bytes");
				m_pNext[0] = (TCAVLLinkAlignedData *)(((mint)(TCAVLLinkAlignedData *)m_pNext[0] & 1) | (mint)_pLeft);
			}

			DIdsPInlineS void SetNext(int _iIndex, TCAVLLinkAlignedData *_pPtr)
			{
				DIdsAssert(!((mint)_pPtr & 1), "We can only save ptrs that are aligned on 2 bytes");
				m_pNext[_iIndex] = (TCAVLLinkAlignedData *)(((mint)(TCAVLLinkAlignedData *)m_pNext[_iIndex] & 1) | (mint)_pPtr);
			}

			DIdsPInlineS void SetRight(CData &_pRight)
			{
				m_pNext[1] = (TCAVLLinkAlignedData *)(((mint)(TCAVLLinkAlignedData *)m_pNext[1] & 1) | ((mint)(TCAVLLinkAlignedData *)_pRight & (~0x1)));
			}

			DIdsPInlineS void SetLeft(CData &_pLeft)
			{
				m_pNext[0] = (TCAVLLinkAlignedData *)(((mint)(TCAVLLinkAlignedData *)m_pNext[0] & 1) | ((mint)(TCAVLLinkAlignedData *)_pLeft & (~0x1)));
			}

			DIdsPInlineS static void Assign(CData &_Dest, TCAVLLinkAlignedData *_pSrc)
			{
				DIdsAssert(!((mint)_pSrc & 1), "We can only save ptrs that are aligned on 2 bytes");
				_Dest = (TCAVLLinkAlignedData *)(((mint)(TCAVLLinkAlignedData *)_Dest & 1) | (mint)_pSrc);
			}

			DIdsPInlineS static void Assign(CData &_Dest, CData &_Src)
			{
				_Dest = (TCAVLLinkAlignedData *)(((mint)(TCAVLLinkAlignedData *)_Dest & 1) | ((mint)(TCAVLLinkAlignedData *)_Src & (~0x1)));
			}

			DIdsPInlineS void SetSkew(mint _Skew)
			{
				DIdsAssert(!(_Skew & (~3)), "We can only save 2 bits");
				m_pNext[0] = (TCAVLLinkAlignedData *)(((mint)(TCAVLLinkAlignedData *)m_pNext[0]&(~1)) | (_Skew & 1));
				m_pNext[1] = (TCAVLLinkAlignedData *)(((mint)(TCAVLLinkAlignedData *)m_pNext[1]&(~1)) | (_Skew >> 1));
			}

			DIdsPInlineS void SetAll(TCAVLLinkAlignedData *_pLeft, TCAVLLinkAlignedData *_pRight, mint _Skew)
			{
				DIdsAssert(!(_Skew & (~3)), "We can only save 2 bits");
				m_pNext[0] = (TCAVLLinkAlignedData *)((mint)_pLeft | (_Skew & 1));
				m_pNext[1] = (TCAVLLinkAlignedData *)((mint)_pRight | (_Skew >> 1));
			}
			DIdsPInlineS void f_Clear()
			{
				m_pNext[0] = (TCAVLLinkAlignedData *)(EAVLTreeSkew_None & 1);
				m_pNext[1] = (TCAVLLinkAlignedData *)(EAVLTreeSkew_None >> 1);
			}


		};
		
		template <typename t_CData, typename t_CKey, typename t_CLinkData, typename t_CTranslator, typename t_CCompare, typename t_CAllocator>
		class TCAVLTreeParams
		{
			public:
			
			typedef t_CData CData;
			typedef t_CKey CKey;
			typedef t_CLinkData CLinkData;
			typedef t_CTranslator CTranslator;
			typedef t_CCompare CCompare;
			typedef t_CAllocator CAllocator;
			
		};
		
		template <typename t_CParamsTree>
		class TCAVLTreeAggregate
		{
		public:
			typedef typename t_CParamsTree::CData CData;
			typedef typename t_CParamsTree::CKey CKey;
			typedef typename t_CParamsTree::CLinkType CLinkType;
			typedef typename t_CParamsTree::CLinkData CLinkData;
			typedef typename CLinkData::CData CLinkCData;
			typedef typename t_CParamsTree::CTranslator CTranslator;
			typedef typename t_CParamsTree::CCompare CCompare;
			typedef typename t_CParamsTree::CAllocator CAllocator;
			
			CLinkCData m_Root;

			DIdsPInlineS static CLinkData *LinkFromMember(CData *_pMember)
			{				
				return ((CLinkData *)(((uint8 *)_pMember) + CTranslator::GetOffset()));
			}

			DIdsPInlineS static CData *MemberFromLink(CLinkData *_pLink)
			{				
				return ((CData *)(((uint8 *)_pLink) - CTranslator::GetOffset()));
			}

			DIdsPInlineS static CLinkData *LinkFromMemberConst(const CData *_pMember)
			{				
				return ((const CLinkData *)(((uint8 *)_pMember) + CTranslator::GetOffset()));
			}

			DIdsPInlineS static CData *MemberFromLinkConst(const CLinkData *_pLink)
			{				
				return ((CData *)(((uint8 *)_pLink) - CTranslator::GetOffset()));
			}



			DIdsPInlineS void f_Construct()
			{
				CLinkData::Assign(m_Root, (CLinkData *)DNP);
			}

			void RUnlink(CLinkCData &_pUnlink)
			{
				if (CLinkData::GetPtr(_pUnlink)->GetLeftP())
					RUnlink(CLinkData::GetPtr(_pUnlink)->GetLeft());

				if (CLinkData::GetPtr(_pUnlink)->GetRightP())
					RUnlink(CLinkData::GetPtr(_pUnlink)->GetRight());

				CLinkData::GetPtr(_pUnlink)->SetSkew(CLinkData::EAVLTreeSkew_NotInTree);
			}

			DIdsPInlineS void f_Destruct()
			{
				if (CLinkData::GetPtr(m_Root))
					RUnlink(m_Root);
			}

#ifdef DIdsTreeCheckRemove
			bint m_bRemoveSuccess;
			template <typename t_CContext>
#else
			template <typename t_CContext>
			static 
#endif
			bint fr_Remove(CLinkCData &_pObject, CLinkData *_pObjectToRemove, t_CContext _Context)
			{
				CLinkData* pObj = CLinkData::GetPtr(_pObject);

				DIdsAssert(pObj, "Object was not found in tree");

				if (pObj != _pObjectToRemove)
				{
					aint Compare = CCompare::Compare(MemberFromLink(_pObjectToRemove), MemberFromLink(pObj), _Context);
					if (Compare < 0) 
					{
						if (fr_Remove(pObj->GetLeft(), _pObjectToRemove, _Context)) 
						{
							return LeftShrunk(_pObject);
						}
						return false;
					}
					else if (Compare > 0) 
					{
						if (fr_Remove(pObj->GetRight(), _pObjectToRemove, _Context)) 
						{
							return RightShrunk(_pObject);
						}
						return false;
					}
				}
				
#ifdef DIdsTreeCheckRemove
				m_bRemoveSuccess = true;
#endif
				if (pObj->GetLeftP()) 
				{
					CLinkData* pHighestObject;
					bint bLeftShrunk = BalanceHighest(pHighestObject, pObj->GetLeft());

					// Remove target from tree
					pHighestObject->SetSkew(pObj->GetSkew());
					pObj->SetSkew(CLinkData::EAVLTreeSkew_NotInTree);
					// Remove from last place
					// Link in on targets place
					pHighestObject->SetLeft(pObj->GetLeft());
					pHighestObject->SetRight(pObj->GetRight());
					CLinkData::Assign(_pObject, pHighestObject);

					if (bLeftShrunk)
					{
						return LeftShrunk(_pObject);
					}
					else
						return false;
				}

				if (pObj->GetRightP()) 
				{
					CLinkData* pLowestObject;

					bint bRightShrunk = BalanceLowest(pLowestObject, pObj->GetRight());

					// Remove target from tree
					pLowestObject->SetSkew(pObj->GetSkew());
					pObj->SetSkew(CLinkData::EAVLTreeSkew_NotInTree);
					// Link in on targets place
					pLowestObject->SetLeft(pObj->GetLeft());
					pLowestObject->SetRight(pObj->GetRight());
					CLinkData::Assign(_pObject, pLowestObject);

					if (bRightShrunk)
					{
						return RightShrunk(_pObject);
					}
					else
						return false;
				}

				pObj->SetSkew(CLinkData::EAVLTreeSkew_NotInTree);

				CLinkData::Assign(_pObject, (CLinkData *)DNP);

				return true;
			}

#if 0
			class CStackObj
			{
				mint m_pStack;
			public:
				DIdsPInlineS void SetAll(CLinkCData *_pPtr, mint _Larger)
				{
					m_pStack = ((mint)_pPtr | _Larger);
				}
				DIdsPInlineS CLinkCData *GetStack() const
				{
					return (CLinkCData *)(m_pStack & (~mint(1)));
				}
				DIdsPInlineS mint GetCompare() const
				{
					return m_pStack & mint(1);
				}
			};
#else
			class CStackObj
			{
				CLinkCData * m_pStack;
				aint m_Compare;
			public:
				DIdsPInlineS void SetAll(CLinkCData *_pPtr, aint _Larger)
				{
					m_pStack = _pPtr;
					m_Compare = _Larger;
				}
				DIdsPInlineS void SetStack(CLinkCData *_pPtr)
				{
					m_pStack = _pPtr;
				}
				DIdsPInlineS CLinkCData *GetStack() const
				{
					return m_pStack;
				}
				DIdsPInlineS aint GetCompare() const
				{
					return m_Compare;
				}
			};
#endif

			DIdsPInlineM static bint fp_BalanceLowest(CLinkData* &_pLowestObject, CLinkCData &_pObject, CStackObj *_pStack)
			{
				CStackObj *pStack = _pStack;

				CLinkCData *pObject = &_pObject;

				CLinkData *pObj = CLinkData::GetPtr(_pObject);
				while (pObj->GetLeftP())
				{
					pStack->SetStack(pObject);
					++pStack;
					pObject = &(pObj->GetLeft());
					pObj = CLinkData::GetPtr(*pObject);
				}

				// Save the object that we want at targets place
				_pLowestObject = pObj;
				// Remove pObject from the tree
				CLinkData::Assign(*pObject, pObj->GetRight());

				while (pStack - _pStack)
				{
					--pStack;
					if (!LeftShrunk(*pStack->GetStack()))
						return false;					
				}
				return true;
			}
			
			DIdsPInlineM static bint fp_BalanceHighest(CLinkData* &_pHighestObject, CLinkCData &_pObject, CStackObj *_pStack)
			{
				CStackObj *pStack = _pStack;

				CLinkCData *pObject = &_pObject;

				CLinkData *pObj = CLinkData::GetPtr(_pObject);
				while (pObj->GetRightP())
				{
					pStack->SetStack(pObject);
					++pStack;
					pObject = &(pObj->GetRight());
					pObj = CLinkData::GetPtr(*pObject);
				}

				// Save the object that we want at targets place
				_pHighestObject = pObj;
				// Remove pObject from the tree
				CLinkData::Assign(*pObject, pObj->GetLeft());

				while (pStack - _pStack)
				{
					--pStack;
					if (!RightShrunk(*pStack->GetStack()))
						return false;					
				}
				return true;

			}

			template <typename t_CContext>
#ifndef DIdsTreeCheckRemove
			static 
#endif
			void fp_Remove(CLinkCData &_pObject, CLinkData *_pObjectToRemove, t_CContext _Context)
			{

				const int Size = ((sizeof(void *) * 8) - TCountBits<sizeof(CLinkData)>::ENumBits + 1) * 2;
				CStackObj Stack[Size]; // Depth of perfect tree * 2
				CStackObj *pStack = Stack;

				CLinkCData *pObject = &_pObject;
//				int iStack = 0;

				while (1)
				{
					CLinkData *pObj = CLinkData::GetPtr(*pObject);
					DIdsAssert(pObj, "Object not found");
					if (pObj == _pObjectToRemove)
					{
		#ifdef DIdsTreeCheckRemove
						m_bRemoveSuccess = true;
		#endif
						if (pObj->GetLeftP()) 
						{
							CLinkData* pHighestObject;
							bint bLeftShrunk = fp_BalanceHighest(pHighestObject, pObj->GetLeft(), pStack);

							// Remove target from tree
							pHighestObject->SetSkew(pObj->GetSkew());
							pObj->SetSkew(CLinkData::EAVLTreeSkew_NotInTree);
							// Remove from last place
							// Link in on targets place
							pHighestObject->SetLeft(pObj->GetLeft());
							pHighestObject->SetRight(pObj->GetRight());
							CLinkData::Assign(*pObject, pHighestObject);

							if (bLeftShrunk)
							{
								if (!LeftShrunk(*pObject))
									return;
							}
							else
								return;
						}
						else if (pObj->GetRightP()) 
						{
							CLinkData* pLowestObject;

							bint bRightShrunk = fp_BalanceLowest(pLowestObject, pObj->GetRight(), pStack);

							// Remove target from tree
							pLowestObject->SetSkew(pObj->GetSkew());
							pObj->SetSkew(CLinkData::EAVLTreeSkew_NotInTree);
							// Link in on targets place
							pLowestObject->SetLeft(pObj->GetLeft());
							pLowestObject->SetRight(pObj->GetRight());
							CLinkData::Assign(*pObject, pLowestObject);

							if (bRightShrunk)
							{
								if (!RightShrunk(*pObject))
									return;
							}
							else
								return;
						}
						else
						{

							pObj->SetSkew(CLinkData::EAVLTreeSkew_NotInTree);

							CLinkData::Assign(*pObject, (CLinkData *)DNP);
						}
						break;
					}

					aint Compare = CCompare::Compare(MemberFromLink(_pObjectToRemove), MemberFromLink(pObj), _Context);
					pStack->SetAll(pObject, Compare);

					if (Compare > 0)
					{
						pObject = &pObj->GetRight();
					}
					else
					{
						pObject = &pObj->GetLeft();
					}
					++pStack;
				}
				while (pStack - Stack)
				{
					--pStack;
//					const CStackObj &StackObj = Stack[iStack];
					if (pStack->GetCompare() > 0)
					{
						if (!RightShrunk(*(pStack->GetStack())))
							break;
					}
					else
					{
						if (!LeftShrunk(*(pStack->GetStack())))
							break;
					}
				}
			}

			template <int t_Direction>
			static void fp_RemoveRotate3(CLinkCData *_pObject)
			{
				CLinkData *pD;
				CLinkData *pB;
				CLinkData *pC;
				CLinkData *pE;
				CLinkData *pF;

				pF = CLinkData::GetPtr(*_pObject);
				pB = pF->GetNextP(1-t_Direction);
				pD = pB->GetNextP(t_Direction);
				pC = pD->GetNextP(1-t_Direction);
				pE = pD->GetNextP(t_Direction);

				CLinkData::Assign(*_pObject, pD);

				pD->SetNext(1-t_Direction, pB);
				pD->SetNext(t_Direction, pF);
				pB->SetNext(t_Direction, pC);
				pF->SetNext(1-t_Direction, pE);

				pB->SetSkew(CLinkData::EAVLTreeSkew_None);
				pF->SetSkew(CLinkData::EAVLTreeSkew_None);
				int DSkew = pD->GetSkew();
				if (DSkew == t_Direction)
					pB->SetSkew(1-t_Direction);
				else if (DSkew == 1-t_Direction)
					pF->SetSkew(t_Direction);
				pD->SetSkew(CLinkData::EAVLTreeSkew_None);

			}

			template <int t_Direction>
			static void fp_RemoveRotate2(CLinkCData *_pObject)
			{
				CLinkData *pD;
				CLinkData *pB;
				CLinkData *pC;

				pD = CLinkData::GetPtr(*_pObject);
				pB = pD->GetNextP(1-t_Direction);
				pC = pB->GetNextP(t_Direction);

				CLinkData::Assign(*_pObject, pB);
				pB->SetNext(t_Direction, pD);
				pD->SetNext(1-t_Direction, pC);

				if (pB->f_IsBalanced())
				{
					pB->SetSkew(t_Direction);
					pD->SetSkew(1-t_Direction);
				}
				else
				{
					pB->SetSkew(CLinkData::EAVLTreeSkew_None);
					pD->SetSkew(CLinkData::EAVLTreeSkew_None);
				}
			}

			template <typename t_CContext>
			DIdsPInlineM static CLinkCData *fp_RemoveRebalance(CLinkCData *_pTop, CLinkCData *_pTarget, CLinkData *_pObjectToRemove, t_CContext _Context)
			{
				// each node from treep down towards target, but
				// excluding the last, will have a subtree grow
				// and need rebalancing

				CLinkData *pTarget = CLinkData::GetPtr(*_pTarget);
				CLinkCData *pObject = _pTop;
				CLinkData *pObj = CLinkData::GetPtr(*pObject);

				while (1)
				{
					if (CCompare::Compare(MemberFromLink(_pObjectToRemove), MemberFromLink(pObj), _Context) > 0)
					{
						if (!pObj->GetRightP())
							break;
						if (pObj->f_IsBalanced())
							pObj->SetSkew(0);
						else if (pObj->GetSkew() == 1)
							pObj->SetSkew(CLinkData::EAVLTreeSkew_None);
						else
						{
							if (pObj->GetLeftP()->GetSkew() == 1)
								fp_RemoveRotate3<1>(pObject);
							else
								fp_RemoveRotate2<1>(pObject);
							if (pObj == pTarget)
								_pTarget = &(CLinkData::GetPtr(*pObject)->GetRight());
						}
						pObject = &pObj->GetRight();
						pObj = CLinkData::GetPtr(*pObject);
					}
					else
					{
						if (!pObj->GetLeftP())
							break;
						if (pObj->f_IsBalanced())
							pObj->SetSkew(1);
						else if (pObj->GetSkew() == 0)
							pObj->SetSkew(CLinkData::EAVLTreeSkew_None);
						else
						{
							if (pObj->GetRightP()->GetSkew() == 0)
								fp_RemoveRotate3<0>(pObject);
							else
								fp_RemoveRotate2<0>(pObject);
							if (pObj == pTarget)
								_pTarget = &(CLinkData::GetPtr(*pObject)->GetLeft());
						}
						pObject = &pObj->GetLeft();
						pObj = CLinkData::GetPtr(*pObject);
					}				

				}

				return _pTarget;
			}

			template <typename t_CContext>
#ifndef DIdsTreeCheckRemove
			static 
#endif
			void fp_RemoveLowStack(CLinkCData &_pObject, CLinkData *_pObjectToRemove, t_CContext _Context)
			{
#ifdef DIdsTreeCheckRemove
				m_bRemoveSuccess = false;
#endif
				CLinkCData *pObject = &_pObject;
				CLinkCData *pPathTop = pObject;
				CLinkCData *pTarget = DNP;
				CLinkData *pObj = CLinkData::GetPtr(*pObject);

				int Dir = 0;
				while (pObj)
				{
					if (pObj == _pObjectToRemove)
						pTarget = pObject;

					if (CCompare::Compare(MemberFromLink(_pObjectToRemove), MemberFromLink(pObj), _Context) > 0)
					{
						if (!pObj->GetRightP())
						{
							Dir = 1;
							break;
						}
						if (pObj->f_IsBalanced() || (pObj->GetSkew() == 0 && pObj->GetLeftP()->f_IsBalanced()))
							pPathTop = pObject;
						pObject = &pObj->GetRight();
					}
					else
					{
						if (!pObj->GetLeftP())
						{
							Dir = 0;
							break;
						}
						if (pObj->f_IsBalanced() || (pObj->GetSkew() == 1 && pObj->GetRightP()->f_IsBalanced()))
							pPathTop = pObject;
						pObject = &pObj->GetLeft();
					}
					pObj = CLinkData::GetPtr(*pObject);
				}

				DIdsAssert(pTarget, "Target not found in tree");

#ifdef DIdsTreeCheckRemove
				m_bRemoveSuccess = true;
#endif
				pTarget = fp_RemoveRebalance(pPathTop, pTarget, _pObjectToRemove, _Context);

				CLinkData *pObjDel = CLinkData::GetPtr(*pTarget);
				CLinkData::Assign(*pTarget, pObj);
				CLinkData::Assign(*pObject, pObj->GetNextP(1-Dir));

				pObj->SetLeft(pObjDel->GetLeft());
				pObj->SetRight(pObjDel->GetRight());
				pObj->SetSkew(pObjDel->GetSkew());

				pObjDel->SetSkew(CLinkData::EAVLTreeSkew_NotInTree);
			}


			template <int t_Direction>
			DIdsPInlineS static CLinkData *fp_Rotate2(CLinkCData *_pTop)
			{
				CLinkData *pBP;
				CLinkData *pCP;
				CLinkData *pDP;

				pBP = CLinkData::GetPtr(*_pTop);
				pDP = pBP->GetNextP(t_Direction);
				pCP = pDP->GetNextP(1-t_Direction);
				CLinkData::Assign(*_pTop, pDP);
				pDP->SetNext(1-t_Direction, pBP);
				pBP->SetNext(t_Direction, pCP);
				pBP->SetSkew(CLinkData::EAVLTreeSkew_None);
				pDP->SetSkew(CLinkData::EAVLTreeSkew_None);
				return pDP->GetNextP(t_Direction);
			}


			static void fp_Rotate3Short(CLinkCData *_pTop, int t_Direction)
			{
				CLinkData *pBP;
				CLinkData *pCP;
				CLinkData *pDP;
				CLinkData *pEP;
				CLinkData *pFP;

				pBP = CLinkData::GetPtr(*_pTop);
				pFP = pBP->GetNextP(t_Direction);
				pDP = pFP->GetNextP(1-t_Direction);

				pCP = pDP->GetNextP(1-t_Direction);
				pEP = pDP->GetNextP(t_Direction);

				CLinkData::Assign(*_pTop, pDP);

				pDP->SetNext(1-t_Direction, pBP);
				pDP->SetNext(t_Direction, pFP);
				pBP->SetNext(t_Direction, pCP);
				pFP->SetNext(1-t_Direction, pEP);

				pDP->SetSkew(CLinkData::EAVLTreeSkew_None);
				pBP->SetSkew(CLinkData::EAVLTreeSkew_None);
				pFP->SetSkew(CLinkData::EAVLTreeSkew_None);
			}

			static CLinkData *fp_Rotate3(CLinkCData *_pTop, int t_Direction, int t_Third)
			{
				CLinkData *pBP;
				CLinkData *pCP;
				CLinkData *pDP;
				CLinkData *pEP;
				CLinkData *pFP;

				pBP = CLinkData::GetPtr(*_pTop);
				pFP = pBP->GetNextP(t_Direction);
				pDP = pFP->GetNextP(1-t_Direction);

				pCP = pDP->GetNextP(1-t_Direction);
				pEP = pDP->GetNextP(t_Direction);

				CLinkData::Assign(*_pTop, pDP);

				pDP->SetNext(1-t_Direction, pBP);
				pDP->SetNext(t_Direction, pFP);
				pBP->SetNext(t_Direction, pCP);
				pFP->SetNext(1-t_Direction, pEP);

				pDP->SetSkew(CLinkData::EAVLTreeSkew_None);

//				CLinkData *pTree0 = pDP->GetNextP(_Direction);
//				CLinkData *pTree1 = pDP->GetNextP(1-_Direction);

				if (t_Third == t_Direction)
				{
					pBP->SetSkew(1-t_Direction);
					pFP->SetSkew(CLinkData::EAVLTreeSkew_None);
					return pFP->GetNextP(1-t_Direction);
				}
				else
				{
					pBP->SetSkew(CLinkData::EAVLTreeSkew_None);
					pFP->SetSkew(t_Direction);
					return pBP->GetNextP(t_Direction);
				}
			}

			template <typename t_CContext>
			DIdsPInlineS static void fp_RebalancePathLowStack(CLinkData *_pPath, CLinkData *_pObjectToInsert, t_CContext _Context)
			{
				// Each node in path is currently balanced.
				// Until we find target, mark each node as longer
				// in the direction of target because we know we have
				// inserted target there
				CLinkData *pPath = _pPath;

				while (pPath && pPath != _pObjectToInsert) 
				{
					aint Compare = CCompare::Compare(MemberFromLink(_pObjectToInsert), MemberFromLink(pPath), _Context);
					if (Compare > 0)
					{
						pPath->SetSkew(1);
						pPath = pPath->GetRightP();
					}
					else
					{
						pPath->SetSkew(0);
						pPath = pPath->GetLeftP();
					}
				}
			}

			template <typename t_CContext>
			DIdsPInlineS static void fp_RebalanceCase3LowStack(int _Direction, CLinkData *pPath, CLinkCData *_pTop, CLinkData *_pObjectToInsert, t_CContext _Context)
			{
				CLinkData *pObj = pPath;
				if (pObj == _pObjectToInsert)
				{
					fp_Rotate3Short(_pTop, _Direction);
				}
				else
				{
					if (CCompare::Compare(MemberFromLink(_pObjectToInsert), MemberFromLink(pObj), _Context) > 0)
						pObj = fp_Rotate3(_pTop, _Direction, 1);
					else
						pObj = fp_Rotate3(_pTop, _Direction, 0);
					fp_RebalancePathLowStack(pObj, _pObjectToInsert, _Context);
				}
			}

			template <typename t_CContext>
			DIdsPInlineS static void fp_RebalanceLowStack(CLinkCData *_pTop, CLinkData *_pObjectToInsert, t_CContext _Context)
			{
				CLinkData *pObj = CLinkData::GetPtr(*_pTop);
				if (pObj->f_IsBalanced())
				{
					if (pObj == _pObjectToInsert)
						return;
				}
				else if (CCompare::Compare(MemberFromLink(_pObjectToInsert), MemberFromLink(pObj), _Context) > 0)
				{
					if (!pObj->GetSkew())
					{
						pObj->SetSkew(CLinkData::EAVLTreeSkew_None);
						pObj = pObj->GetRightP();
					}
					else
					{
						pObj = pObj->GetRightP();
						if (CCompare::Compare(MemberFromLink(_pObjectToInsert), MemberFromLink(pObj), _Context) > 0)
						{
							pObj = fp_Rotate2<1>(_pTop);
						}
						else
						{
							fp_RebalanceCase3LowStack(1, pObj->GetLeftP(), _pTop, _pObjectToInsert, _Context);
							return;
						}
					}
				}
				else
				{
					if (pObj->GetSkew())
					{
						pObj->SetSkew(CLinkData::EAVLTreeSkew_None);
						pObj = pObj->GetLeftP();
					}
					else
					{
						pObj = pObj->GetLeftP();
						if (CCompare::Compare(MemberFromLink(_pObjectToInsert), MemberFromLink(pObj), _Context) < 0)
						{
							pObj = fp_Rotate2<0>(_pTop);
						}
						else
						{
							fp_RebalanceCase3LowStack(0,pObj->GetRightP(), _pTop, _pObjectToInsert, _Context);
							return;
						}
					}
				}

				fp_RebalancePathLowStack(pObj, _pObjectToInsert, _Context);
			}

			template <typename t_CContext>
			static void fp_InsertLowStack(CLinkCData &_pObject, CLinkData *_pObjectToInsert, t_CContext _Context)
			{
				// insert the target into the tree, returning 1 on success or 0 if it
				// already existed
				
				CLinkCData *pTree = &_pObject;
				CLinkCData *pPathTop = pTree;
				CLinkData *pObj = CLinkData::GetPtr(*pTree);
				while (pObj)
				{
					
					if (pObj->GetSkew() != CLinkData::EAVLTreeSkew_None)
						pPathTop = pTree;

					aint Compare = CCompare::Compare(MemberFromLink(_pObjectToInsert), MemberFromLink(pObj), _Context);
					if (Compare > 0)
					{
						pTree = &(pObj->GetRight());
						pObj = CLinkData::GetPtr(*pTree);
					}
					else
					{
						DIdsAssert(Compare != 0, "Tree does not support inserting two equal objects");
						pTree = &(pObj->GetLeft());
						pObj = CLinkData::GetPtr(*pTree);
					}
				}
				_pObjectToInsert->f_Clear();
				CLinkData::Assign(*pTree, _pObjectToInsert);
				fp_RebalanceLowStack(pPathTop, _pObjectToInsert, _Context);
			}


			template <typename t_CContext>
			static bint fr_Insert(CLinkCData &_pObject, CLinkData *_pObjectToInsert, t_CContext _Context)
			{
				if (!CLinkData::GetPtrIsFast())
				{
					if (CLinkData *pObj = CLinkData::GetPtr(_pObject))
					{
						aint Compare = CCompare::Compare(MemberFromLink(_pObjectToInsert), MemberFromLink(pObj), _Context);
						if (Compare < 0) 
						{
							if (fr_Insert(pObj->GetLeft(), _pObjectToInsert, _Context)) 
							{
								return LeftGrown(_pObject);
							}
						}
						else
						{
							DIdsAssert(Compare != 0, "Tree does not support inserting two equal objects");
							if (fr_Insert(pObj->GetRight(), _pObjectToInsert, _Context)) 
							{
								return RightGrown(_pObject);
							}
						}
						return false;
					}
					else
					{
						DIdsAssert(_pObjectToInsert, "You cannot insert a null object");
						_pObjectToInsert->f_Clear();
						CLinkData::Assign(_pObject, _pObjectToInsert);

						return true;
					}
				}
				else
				{

					if (!CLinkData::GetPtr(_pObject)) 
					{
						DIdsAssert(_pObjectToInsert, "You cannot insert a null object");

						_pObjectToInsert->SetAll(DNP, DNP, CLinkData::EAVLTreeSkew_None);

						CLinkData::Assign(_pObject, _pObjectToInsert);

						return true;
					}

					aint Compare = CCompare::Compare(MemberFromLink(_pObjectToInsert), MemberFromLink(CLinkData::GetPtr(_pObject)), _Context);
					if (Compare < 0) 
					{
						if (fr_Insert(CLinkData::GetPtr(_pObject)->GetLeft(), _pObjectToInsert, _Context)) 
						{
							return LeftGrown_Inl(_pObject);
						}
					}
					else
					{
						DIdsAssert(Compare != 0, "Tree does not support inserting two equal objects");
						if (fr_Insert(CLinkData::GetPtr(_pObject)->GetRight(), _pObjectToInsert, _Context)) 
						{
							return RightGrown_Inl(_pObject);
						}
					}
					return false;
				}
			}

			template <typename t_CContext>
			DIdsPInlineM static void fp_Insert(CLinkCData &_pObject, CLinkData *_pObjectToInsert, t_CContext _Context)
			{

				const int Size = ((sizeof(void *) * 8) - TCountBits<sizeof(CLinkData)>::ENumBits + 1) * 2;
				CStackObj Stack[Size]; // Depth of perfect tree * 2
				CStackObj *pStack = Stack;

				CLinkCData *pObject = &_pObject;
//				int iStack = 0;

				while (1)
				{
					CLinkData *pObj = CLinkData::GetPtr(*pObject);
					if (!pObj)
					{
						_pObjectToInsert->f_Clear();
						CLinkData::Assign(*pObject, _pObjectToInsert);
						break;
					}

					aint Compare = CCompare::Compare(MemberFromLink(_pObjectToInsert), MemberFromLink(pObj), _Context);
					pStack->SetAll(pObject, Compare);

					if (Compare > 0)
					{
						pObject = &pObj->GetRight();
					}
					else
					{
						DIdsAssert(CCompare::Compare(MemberFromLink(_pObjectToInsert), MemberFromLink(pObj), _Context) != 0, "Tree does not support inserting two equal objects");
						pObject = &pObj->GetLeft();
					}
					++pStack;
				}
				while (pStack - Stack)
				{
					--pStack;
//					const CStackObj &StackObj = Stack[iStack];
					if (pStack->GetCompare() > 0)
					{
						if (!RightGrown_Inl(*(pStack->GetStack())))
							break;
					}
					else
					{
						if (!LeftGrown_Inl(*(pStack->GetStack())))
							break;
					}
				}
			}

			template <typename t_CContext>
			DIdsPInlineS void fr_Insert(CData *_pToInsert, t_CContext _Context = (void *)DNP)
			{
				CLinkData * pToInsert = LinkFromMember(_pToInsert);
				((CLinkType *)pToInsert)->SetTree(this);
				DIdsAssert(pToInsert->GetSkew() == CLinkData::EAVLTreeSkew_NotInTree, "Must not be in tree already");
				
				fr_Insert(m_Root, pToInsert, _Context);
			}


			template <typename t_CContext>
			DIdsPInlineS void f_Insert(CData *_pToInsert, t_CContext _Context = (void *)DNP)
			{
				CLinkData * pToInsert = LinkFromMember(_pToInsert);
				((CLinkType *)pToInsert)->SetTree(this);
				DIdsAssert(pToInsert->GetSkew() == CLinkData::EAVLTreeSkew_NotInTree, "Must not be in tree already");
				
				fp_Insert(m_Root, pToInsert, _Context);
			}

			template <typename t_CContext>
			DIdsPInlineS void f_InsertLowStack(CData *_pToInsert, t_CContext _Context = (void *)DNP)
			{
				CLinkData * pToInsert = LinkFromMember(_pToInsert);
				((CLinkType *)pToInsert)->SetTree(this);
				DIdsAssert(pToInsert->GetSkew() == CLinkData::EAVLTreeSkew_NotInTree, "Must not be in tree already");
				
				fp_InsertLowStack(m_Root, pToInsert, _Context);
			}

			DIdsPInlineS void f_Insert(CData &_ToInsert) 
			{
				f_Insert(_ToInsert, (void *)DNP);
			}

			template <typename t_CContext>
			DIdsPInlineS void f_Insert(CData &_ToInsert, t_CContext _Context = (void *)DNP)
			{
				f_Insert(&_ToInsert, _Context);
			}

			DIdsPInlineS void f_Insert(CData *_pToInsert)
			{
				f_Insert(_pToInsert, (void *)DNP);
			}

			DIdsPInlineS void fr_Insert(CData &_ToInsert) 
			{
				fr_Insert(_ToInsert, (void *)DNP);
			}

			template <typename t_CContext>
			DIdsPInlineS void fr_Insert(CData &_ToInsert, t_CContext _Context = (void *)DNP)
			{
				fr_Insert(&_ToInsert, _Context);
			}

			DIdsPInlineS void fr_Insert(CData *_pToInsert)
			{
				fr_Insert(_pToInsert, (void *)DNP);
			}

			DIdsPInlineS void f_InsertLowStack(CData &_ToInsert) 
			{
				f_InsertLowStack(_ToInsert, (void *)DNP);
			}

			template <typename t_CContext>
			DIdsPInlineS void f_InsertLowStack(CData &_ToInsert, t_CContext _Context = (void *)DNP)
			{
				f_InsertLowStack(&_ToInsert, _Context);
			}

			DIdsPInlineS void f_InsertLowStack(CData *_pToInsert)
			{
				f_InsertLowStack(_pToInsert, (void *)DNP);
			}


			
			DIdsPInlineS void f_RemoveLowStack(CData &_ToRemove)
			{
				f_RemoveLowStack(_ToRemove, (void *)DNP);
			}

			template <typename t_CContext>
			DIdsPInlineS void f_RemoveLowStack(CData &_ToRemove, t_CContext _Context = (void *)DNP)
			{
				f_RemoveLowStack(&_ToRemove, _Context);
			}

			DIdsPInlineS void f_RemoveLowStack(CData *_pToRemove)
			{
				f_RemoveLowStack(_pToRemove, (void *)DNP);
			}

			template <typename t_CContext>
			DIdsPInlineS void f_RemoveLowStack(CData *_pToRemove, t_CContext _Context = (void *)DNP)
			{
#ifdef DIdsTreeCheckRemove
				m_bRemoveSuccess = false;
#endif
				CLinkData * pToRemove = LinkFromMember(_pToRemove);

				fp_RemoveLowStack(m_Root, pToRemove, _Context);
#ifdef DIdsTreeCheckRemove
				DIdsAssert(m_bRemoveSuccess, "Failed to remove the item");
#endif
			}


			
			DIdsPInlineS void f_Remove(CData &_ToRemove)
			{
				f_Remove(_ToRemove, (void *)DNP);
			}

			template <typename t_CContext>
			DIdsPInlineS void f_Remove(CData &_ToRemove, t_CContext _Context = (void *)DNP)
			{
				f_Remove(&_ToRemove, _Context);
			}

			DIdsPInlineS void f_Remove(CData *_pToRemove)
			{
				f_Remove(_pToRemove, (void *)DNP);
			}

			template <typename t_CContext>
			DIdsPInlineS void f_Remove(CData *_pToRemove, t_CContext _Context = (void *)DNP)
			{
#ifdef DIdsTreeCheckRemove
				m_bRemoveSuccess = false;
#endif
				CLinkData * pToRemove = LinkFromMember(_pToRemove);

				fp_Remove(m_Root, pToRemove, _Context);
#ifdef DIdsTreeCheckRemove
				DIdsAssert(m_bRemoveSuccess, "Failed to remove the item");
#endif
			}


			DIdsPInlineS void fr_Remove(CData &_ToRemove)
			{
				fr_Remove(_ToRemove, (void *)DNP);
			}

			template <typename t_CContext>
			DIdsPInlineS void fr_Remove(CData &_ToRemove, t_CContext _Context = (void *)DNP)
			{
				fr_Remove(&_ToRemove, _Context);
			}

			DIdsPInlineS void fr_Remove(CData *_pToRemove)
			{
				fr_Remove(_pToRemove, (void *)DNP);
			}

			template <typename t_CContext>
			DIdsPInlineS void fr_Remove(CData *_pToRemove, t_CContext _Context = (void *)DNP)
			{
#ifdef DIdsTreeCheckRemove
				m_bRemoveSuccess = false;
#endif
				CLinkData * pToRemove = LinkFromMember(_pToRemove);

				fr_Remove(m_Root, pToRemove, _Context);
#ifdef DIdsTreeCheckRemove
				DIdsAssert(m_bRemoveSuccess, "Failed to remove the item");
#endif
			}

			DIdsPInlineM static bint RightShrunk(CLinkCData &_pObject)
			{
				CLinkData *pObj = CLinkData::GetPtr(_pObject);

				switch (pObj->GetSkew()) 
				{
				case CLinkData::EAVLTreeSkew_Right:
					pObj->SetSkew(CLinkData::EAVLTreeSkew_None);
					return true;

				case CLinkData::EAVLTreeSkew_Left:
					switch (pObj->GetLeftP()->GetSkew())
					{
					case CLinkData::EAVLTreeSkew_Left:
						pObj->GetLeftP()->SetSkew(CLinkData::EAVLTreeSkew_None);
						pObj->SetSkew(CLinkData::EAVLTreeSkew_None);
						RotateRight(_pObject);
						return true;
					case CLinkData::EAVLTreeSkew_Right:
						switch (pObj->GetLeftP()->GetRightP()->GetSkew()) 
						{
						case CLinkData::EAVLTreeSkew_Left:
							pObj->SetSkew(CLinkData::EAVLTreeSkew_Right);
							pObj->GetLeftP()->SetSkew(CLinkData::EAVLTreeSkew_None);
							break;
						case CLinkData::EAVLTreeSkew_Right:
							pObj->SetSkew(CLinkData::EAVLTreeSkew_None);
							pObj->GetLeftP()->SetSkew(CLinkData::EAVLTreeSkew_Left);
							break;
						default:
							pObj->SetSkew(CLinkData::EAVLTreeSkew_None);
							pObj->GetLeftP()->SetSkew(CLinkData::EAVLTreeSkew_None);
						}
						pObj->GetLeftP()->GetRightP()->SetSkew(CLinkData::EAVLTreeSkew_None);

						RotateLeft(pObj->GetLeft());
						RotateRight(_pObject);
						return true;
					default:
						pObj->SetSkew(CLinkData::EAVLTreeSkew_Left);
						pObj->GetLeftP()->SetSkew(CLinkData::EAVLTreeSkew_Right);
						RotateRight(_pObject);
						return false;
					}

				default:
					pObj->SetSkew(CLinkData::EAVLTreeSkew_Left);
					return false;
				}		
			}

			DIdsPInlineM static bint LeftShrunk(CLinkCData &_pObject)
			{
				CLinkData *pObj = CLinkData::GetPtr(_pObject);
				switch (pObj->GetSkew()) 
				{
				case CLinkData::EAVLTreeSkew_Left:
					pObj->SetSkew(CLinkData::EAVLTreeSkew_None);
					return true;
					
				case CLinkData::EAVLTreeSkew_Right:

					switch (pObj->GetRightP()->GetSkew())
					{
					case CLinkData::EAVLTreeSkew_Right:
						pObj->SetSkew(CLinkData::EAVLTreeSkew_None);
						pObj->GetRightP()->SetSkew(CLinkData::EAVLTreeSkew_None);
						RotateLeft(_pObject);
						return true;
					case CLinkData::EAVLTreeSkew_Left:
						switch (pObj->GetRightP()->GetLeftP()->GetSkew()) 
						{
						case CLinkData::EAVLTreeSkew_Left:
							pObj->SetSkew(CLinkData::EAVLTreeSkew_None);
							pObj->GetRightP()->SetSkew(CLinkData::EAVLTreeSkew_Right);
							break;							
						case CLinkData::EAVLTreeSkew_Right:
							pObj->SetSkew(CLinkData::EAVLTreeSkew_Left);
							pObj->GetRightP()->SetSkew(CLinkData::EAVLTreeSkew_None);
							break;							
						default:
							pObj->SetSkew(CLinkData::EAVLTreeSkew_None);
							pObj->GetRightP()->SetSkew(CLinkData::EAVLTreeSkew_None);
						}
						
						pObj->GetRightP()->GetLeftP()->SetSkew(CLinkData::EAVLTreeSkew_None);
						
						RotateRight(pObj->GetRight());
						RotateLeft(_pObject);
						return true;
					default:
						pObj->SetSkew(CLinkData::EAVLTreeSkew_Right);
						pObj->GetRightP()->SetSkew(CLinkData::EAVLTreeSkew_Left);
						RotateLeft(_pObject);
						return false;
					}
					
				default:
					pObj->SetSkew(CLinkData::EAVLTreeSkew_Right);
					return false;
				}		
			}

			static bint RightGrown(CLinkCData &_pObject)
			{
				return RightGrown_Inl(_pObject);
			}
			
			DIdsPInlineM static bint RightGrown_Inl(CLinkCData &_pObject)
			{
				CLinkData *pObj = CLinkData::GetPtr(_pObject);
				switch (pObj->GetSkew()) 
				{
				case CLinkData::EAVLTreeSkew_Left:					
					pObj->SetSkew(CLinkData::EAVLTreeSkew_None);
					return false;
					
				case CLinkData::EAVLTreeSkew_Right:
					{

						if (pObj->GetRightP()->GetSkew() == CLinkData::EAVLTreeSkew_Right) 
						{	
							pObj->SetSkew(CLinkData::EAVLTreeSkew_None);
							pObj->GetRightP()->SetSkew(CLinkData::EAVLTreeSkew_None);
							RotateLeft(_pObject);
						}
						else 
						{
							switch (pObj->GetRightP()->GetLeftP()->GetSkew()) 
							{
							case CLinkData::EAVLTreeSkew_Right:
								pObj->SetSkew(CLinkData::EAVLTreeSkew_Left);
								pObj->GetRightP()->SetSkew(CLinkData::EAVLTreeSkew_None);
								break;
							case CLinkData::EAVLTreeSkew_Left:
								pObj->SetSkew(CLinkData::EAVLTreeSkew_None);
								pObj->GetRightP()->SetSkew(CLinkData::EAVLTreeSkew_Right);
								break;
							default:
								pObj->SetSkew(CLinkData::EAVLTreeSkew_None);
								pObj->GetRightP()->SetSkew(CLinkData::EAVLTreeSkew_None);
							}
							pObj->GetRightP()->GetLeftP()->SetSkew(CLinkData::EAVLTreeSkew_None);

							RotateRight(pObj->GetRight());
							RotateLeft(_pObject);
						}
						return false;
					}

				default:
					pObj->SetSkew(CLinkData::EAVLTreeSkew_Right);
					return true;
				}
			}

			static bint LeftGrown(CLinkCData &_pObject)
			{
				return LeftGrown_Inl(_pObject);
			}

			DIdsPInlineM static bint LeftGrown_Inl(CLinkCData &_pObject)
			{
				CLinkData *pObj = CLinkData::GetPtr(_pObject);
				switch (pObj->GetSkew()) 
				{
				case CLinkData::EAVLTreeSkew_Right:
					pObj->SetSkew(CLinkData::EAVLTreeSkew_None);
					return false;
					
				case CLinkData::EAVLTreeSkew_Left:
					{
						if (pObj->GetLeftP()->GetSkew() == CLinkData::EAVLTreeSkew_Left) 
						{
							pObj->SetSkew(CLinkData::EAVLTreeSkew_None);
							pObj->GetLeftP()->SetSkew(CLinkData::EAVLTreeSkew_None);
							RotateRight(_pObject);
						}	
						else 
						{
							switch (pObj->GetLeftP()->GetRightP()->GetSkew()) 
							{
							case CLinkData::EAVLTreeSkew_Left:
								pObj->SetSkew(CLinkData::EAVLTreeSkew_Right);
								pObj->GetLeftP()->SetSkew(CLinkData::EAVLTreeSkew_None);
								break;							
							case CLinkData::EAVLTreeSkew_Right:
								pObj->SetSkew(CLinkData::EAVLTreeSkew_None);
								pObj->GetLeftP()->SetSkew(CLinkData::EAVLTreeSkew_Left);
								break;							
							default:
								pObj->SetSkew(CLinkData::EAVLTreeSkew_None);
								pObj->GetLeftP()->SetSkew(CLinkData::EAVLTreeSkew_None);
							}

							pObj->GetLeftP()->GetRightP()->SetSkew(CLinkData::EAVLTreeSkew_None);

							RotateLeft(pObj->GetLeft());
							RotateRight(_pObject);
						}
						return false;
					}
					
				default:
					pObj->SetSkew(CLinkData::EAVLTreeSkew_Left);
					return true;
				}		
			}
			
			DIdsPInlineM static void RotateRight(CLinkCData &_pObject)
			{
				CLinkData *tmp = CLinkData::GetPtr(_pObject);
				CLinkData *pObj = tmp->GetLeftP();
				CLinkData::Assign(_pObject, pObj);
				tmp->SetLeft(pObj->GetRight());
				pObj->SetRight(tmp);
			}
			
			DIdsPInlineM static void RotateLeft(CLinkCData &_pObject)
			{
				CLinkData *tmp = CLinkData::GetPtr(_pObject);
				CLinkData *pObj = tmp->GetRightP();
				CLinkData::Assign(_pObject, pObj);
				tmp->SetRight(pObj->GetLeft());
				pObj->SetLeft(tmp);
			}


			static bint BalanceLowest(CLinkData* &_pLowestObject, CLinkCData &_pObject)
			{
				CLinkData *pObject = CLinkData::GetPtr(_pObject);
				
				if (pObject->GetLeftP()) 
				{
					if (BalanceLowest(_pLowestObject, pObject->GetLeft())) 
					{
						return LeftShrunk(_pObject);
					}
					else
						return false;
				}

				// Save the object that we want at targets place
				_pLowestObject = pObject;

				// Remove pObject from the tree
				CLinkData::Assign(_pObject, pObject->GetRight());

				return true;
			}
			
			static bint BalanceHighest(CLinkData* &_pHighestObject, CLinkCData &_pObject)
			{
				CLinkData *pObject = CLinkData::GetPtr(_pObject);

				if (pObject->GetRightP()) 
				{
					if (BalanceHighest(_pHighestObject, pObject->GetRight()))
					{
						return RightShrunk(_pObject);
					}
					else 
						return false;
				}

				// Save the object that we want at targets place
				_pHighestObject = pObject;

				// Remove pObject from the tree
				CLinkData::Assign(_pObject, pObject->GetLeft());

				return true;
			}


			DIdsPInlineS bint IsEmpty() const 
			{
				return !(CLinkData::GetPtr(m_Root));
			}

			DIdsPInlineS bint HasOneMember() const 
			{
				CLinkData * pObject = CLinkData::GetPtr(m_Root);
				if (pObject)
				{
					if (!pObject->GetLeftP() && !pObject->GetRightP())
						return true;
				}
				return false;
			}

			DIdsPInlineS CData *GetRoot() const
			{				
				if (CLinkData::GetPtr(m_Root))
					return MemberFromLink(CLinkData::GetPtr(m_Root));
				else
					return DNP;
			}

			DIdsPInlineS static CData *GetRight(CData *_pObject)
			{
				CLinkData * pObject = LinkFromMember(_pObject);
				
				if (pObject->GetRightP())
					return MemberFromLink(pObject->GetRightP());
				else
					return DNP;
			}

			DIdsPInlineS static CData *GetLeft(CData *_pObject)
			{
				CLinkData * pObject = LinkFromMember(_pObject);
				
				if (pObject->GetLeftP())
					return MemberFromLink(pObject->GetLeftP());
				else
					return DNP;
			}

		public:

			
			DIdsPInlineS CData* FindEqual(const CKey &_Key) const
			{
				return FindEqual(_Key, (void *)DNP);
			}

			template <typename t_CContext>
			DIdsPInlineM CData* FindEqual(const CKey &_Key, t_CContext _Context = (void *)DNP) const
			{
				CLinkData* pCurrentSearch = CLinkData::GetPtr(m_Root);

				while (pCurrentSearch)
				{
					aint Compare = CCompare::Compare(MemberFromLink(pCurrentSearch), _Key, _Context);

					if (Compare < 0) 
					{
						pCurrentSearch = pCurrentSearch->GetRightP();
					}
					else if (Compare > 0)
					{
						pCurrentSearch = pCurrentSearch->GetLeftP();
					}
					else
					{
						return MemberFromLink(pCurrentSearch);
					}
				}
				
				return DNP;
			}

			DIdsPInlineS CData* FindSmallestGreaterThanEqual(const CKey &_Key) const
			{
				return FindSmallestGreaterThanEqual(_Key, (void *)DNP);
			}

			template <typename t_CContext>
			DIdsPInlineL CData* FindSmallestGreaterThanEqual(const CKey &_Key, t_CContext _Context = (void *)DNP) const
			{
				CLinkData *pBestFit = DNP;

				CLinkData* pCurrentSearch = CLinkData::GetPtr(m_Root);

				while (pCurrentSearch)
				{
					aint Compare = CCompare::Compare(MemberFromLink(pCurrentSearch), _Key, _Context);

					if (Compare < 0)
					{
						pCurrentSearch = pCurrentSearch->GetRightP();
					}
					else if (Compare > 0)
					{
						DIdsAssert(!pBestFit || (CCompare::Compare(MemberFromLink(pCurrentSearch), MemberFromLink(pBestFit), _Context) < 0), "Tree is damaged");
						pBestFit = pCurrentSearch;
						pCurrentSearch = pCurrentSearch->GetLeftP();
					}
					else
					{
						// Equal match
						return MemberFromLink(pCurrentSearch);
					}
				}

				if (pBestFit)
					return MemberFromLink(pBestFit);
				else
					return DNP;
			}

			DIdsPInlineS CData* FindLargestLessThanEqual(const CKey &_Key) const
			{
				return FindLargestLessThanEqual(_Key, (void *)DNP);
			}

			template <typename t_CContext>
			DIdsPInlineL CData* FindLargestLessThanEqual(const CKey &_Key, t_CContext _Context = (void *)DNP) const
			{
				CLinkData *pBestFit = DNP;

				CLinkData* pCurrentSearch = CLinkData::GetPtr(m_Root);

				while (pCurrentSearch)
				{
					aint Compare = CCompare::Compare(MemberFromLink(pCurrentSearch), _Key, _Context);

					if (Compare > 0)
					{
						pCurrentSearch = pCurrentSearch->GetLeftP();
					}
					else if (Compare < 0) 
					{
						DIdsAssert(!pBestFit || (CCompare::Compare(MemberFromLink(pCurrentSearch), MemberFromLink(pBestFit), _Context) > 0), "Tree is damaged");
						pBestFit = pCurrentSearch;
						pCurrentSearch = pCurrentSearch->GetRightP();
					}					
					else
					{
						return MemberFromLink(pCurrentSearch);
					}
				}

				if (pBestFit)
					return MemberFromLink(pBestFit);
				else
					return DNP;
			}

			CData* FindSmallest() const
			{
				CLinkData *pBestFit = CLinkData::GetPtr(m_Root);

				if (pBestFit)
				{
					while (1)
					{
						CLinkData *pNext = pBestFit->GetLeftP();

						if (!pNext)
							return MemberFromLink(pBestFit);

						pBestFit = pNext;
					}
				}

				return DNP;
			}

			CData* FindLargest() const
			{
				CLinkData *pBestFit = CLinkData::GetPtr(m_Root);

				if (pBestFit)
				{
					while (1)
					{
						CLinkData *pNext = pBestFit->GetRightP();

						if (!pNext)
							return MemberFromLink(pBestFit);

						pBestFit = pNext;
					}
				}

				return DNP;
			}

			/************************************************************************************************\
			||¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯||
			|| Iterator
			||______________________________________________________________________________________________||
			\************************************************************************************************/

			// 150 % marginal over could possably fit in memory (the tree is not perfect)
#			define DIdsTreeAvlDefaultIteratorRecursionDepth (sizeof(void *)*12)
			template <aint _RecursionDepth = DIdsTreeAvlDefaultIteratorRecursionDepth>
			class TIterator
			{
			public:
				
				typedef typename t_CParamsTree::CData CData;
				typedef typename t_CParamsTree::CKey CKey;
				typedef typename t_CParamsTree::CLinkType CLinkType;
				typedef typename t_CParamsTree::CLinkData CLinkData;
				typedef typename CLinkData::CData CLinkCData;
				typedef typename t_CParamsTree::CTranslator CTranslator;
				typedef typename t_CParamsTree::CCompare CCompare;
				typedef typename t_CParamsTree::CAllocator CAllocator;
				
				aint m_iStack;
				const CLinkData *m_pStack[_RecursionDepth];

				DIdsPInlineS TIterator()
				{
					// Reset iterator
					m_iStack = -1;
				}

				DIdsPInlineM TIterator(const TIterator &_Tree)
				{
					m_iStack = _Tree.m_iStack;

					for (aint i = 0; i <= m_iStack; ++i)
						m_pStack[i] = _Tree.m_pStack[i];
				}

				DIdsPInlineS TIterator(const TCAVLTreeAggregate &_Tree)
				{
					StartForward(&_Tree);
				}

				DIdsPInlineS TIterator(const TCAVLTreeAggregate *_pTree)
				{
					StartForward(_pTree);
				}

				DIdsPInlineS void StartForward(const TCAVLTreeAggregate &_Tree)
				{
					StartForward(&_Tree);
				}

				DIdsPInlineM void StartForward(const TCAVLTreeAggregate *_pTree)
				{					
					// Find the smallest item in tree, and build stack
					const CLinkData *pCurrent = CLinkData::GetPtr(_pTree->m_Root);
					aint iStack = -1;
					
					while (pCurrent)
					{
						m_pStack[++iStack] = pCurrent;
						pCurrent = pCurrent->GetLeftP();
					}

					m_iStack = iStack;
				}

				void SetRoot(const TCAVLTreeAggregate &_Tree)
				{
					SetRoot(&_Tree);
				}

				void SetRoot(const TCAVLTreeAggregate *_pTree)
				{
					m_pStack[0] = CLinkData::GetPtr(_pTree->m_Root);
					m_iStack = 0;
				}

				bint FindEqualForward(const CKey &_Key)
				{
					return FindEqualForward(_Key, (void *)DNP);
				}

				template <typename t_CContext>
				bint FindEqualForward(const CKey &_Key, t_CContext _Context = (void *)DNP)
				{
					aint iStack = m_iStack;

					const CLinkData* pCurrentSearch = m_pStack[iStack];

					while (pCurrentSearch)
					{
						aint Compare = CCompare::Compare(MemberFromLinkConst(pCurrentSearch), _Key, _Context);

						if (Compare < 0) 
						{
							pCurrentSearch = pCurrentSearch->GetRightP();
						}
						else if (Compare > 0)
						{
							m_pStack[iStack++] = pCurrentSearch;
							pCurrentSearch = pCurrentSearch->GetLeftP();
						}
						else
						{
							m_pStack[iStack] = pCurrentSearch;
							// We found something set stack
							m_iStack = iStack;
							return true;
						}
					}
					return false;
				}

				bint FindSmallestGreaterThanEqualForward(const CKey &_Key)
				{
					return FindSmallestGreaterThanEqualForward(_Key, (void *)DNP);
				}

				template <typename t_CContext>
				bint FindSmallestGreaterThanEqualForward(const CKey &_Key, t_CContext _Context = (void *)DNP)
				{
					const CLinkData* pBestFit = DNP;

					aint iStack = m_iStack;

					const CLinkData* pCurrentSearch = m_pStack[iStack];

					while (pCurrentSearch)
					{
						aint Compare = CCompare::Compare(MemberFromLinkConst(pCurrentSearch), _Key, _Context);

						if (Compare < 0)
						{
							pCurrentSearch = pCurrentSearch->GetRightP();
						}
						else if (Compare > 0)
						{
							DIdsAssert(!pBestFit || (CCompare::Compare(MemberFromLinkConst(pCurrentSearch), MemberFromLinkConst(pBestFit), _Context) < 0), "Tree is damaged");
							pBestFit = pCurrentSearch;
							m_pStack[iStack++] = pCurrentSearch;
							pCurrentSearch = pCurrentSearch->GetLeftP();
						}
						else
						{
							m_pStack[iStack] = pCurrentSearch;
							m_iStack = iStack;
							// Equal match
							return true;
						}
					}

					if (pBestFit)
					{
						m_iStack = iStack-1;
						return true;
					}
					else
						return false;
				}

				bint FindLargestLessThanEqualForward(const CKey &_Key)
				{
					return FindLargestLessThanEqualForward(_Key, (void *)DNP);
				}

				template <typename t_CContext>
				bint FindLargestLessThanEqualForward(const CKey &_Key, t_CContext _Context = (void *)DNP)
				{
					const CLinkData *pBestFit = DNP;

					aint iStack = m_iStack;
					aint iBestFitStack = -1;

					const CLinkData* pCurrentSearch = m_pStack[iStack];

					while (pCurrentSearch)
					{
						aint Compare = CCompare::Compare(MemberFromLinkConst(pCurrentSearch), _Key, _Context);

						if (Compare > 0)
						{
							m_pStack[iStack++] = pCurrentSearch;
							pCurrentSearch = pCurrentSearch->GetLeftP();
						}
						else if (Compare < 0) 
						{
							DIdsAssert(!pBestFit || (CCompare::Compare(MemberFromLinkConst(pCurrentSearch), MemberFromLinkConst(pBestFit), _Context) > 0), "Tree is damaged");
							pBestFit = pCurrentSearch;
							iBestFitStack = iStack;
							pCurrentSearch = pCurrentSearch->GetRightP();
						}					
						else
						{
							m_pStack[iStack] = pCurrentSearch;
							m_iStack = iStack;
							// Equal match
							return true;
						}
					}

					if (pBestFit)
					{
						m_pStack[iBestFitStack] = pBestFit;
						m_iStack = iBestFitStack;
						return true;
					}
					else
						return false;
				}


				DIdsPInlineM void Next()
				{
					if (m_iStack < 0)
					{
						// We are already done
						return;
					}
					
					const CLinkData *pCurrent = m_pStack[m_iStack];

					pCurrent = pCurrent->GetRightP();
					// Decrease stack so we overwrite the current stack
					--m_iStack;
					while (pCurrent)
					{
						m_pStack[++m_iStack] = pCurrent;
						pCurrent = pCurrent->GetLeftP();
					}
				}

				DIdsPInlineS void StartBackward(const TCAVLTreeAggregate &_Tree)
				{
					StartBackward(&_Tree);
				}

				DIdsPInlineM void StartBackward(const TCAVLTreeAggregate *_pTree)
				{					
					// Find the smallest item in tree, and build stack
					const CLinkData *pCurrent = CLinkData::GetPtr(_pTree->m_Root);
					aint iStack = -1;
					
					while (pCurrent)
					{
						m_pStack[++iStack] = pCurrent;
						pCurrent = pCurrent->GetRightP();
					}

					m_iStack = iStack;
				}

				DIdsPInlineM void Prev()
				{
					if (m_iStack < 0)
					{
						// We are already done

						return;
					}
					
					const CLinkData *pCurrent = m_pStack[m_iStack];

					pCurrent = pCurrent->GetLeftP();
					// Decrease stack so we overwrite the current stack
					--m_iStack;
					while (pCurrent)
					{
						m_pStack[++m_iStack] = pCurrent;
						pCurrent = pCurrent->GetRightP();
					}
				}


				DIdsPInlineS CData *GetCurrent() const
				{
					if (m_iStack >= 0)
						return MemberFromLinkConst(m_pStack[m_iStack]);
					else
						return DNP;
				}

				DIdsPInlineS operator CData *() const
				{
					return GetCurrent();
				}

				DIdsPInlineS CData * operator ->() const
				{
					return GetCurrent();
				}

				DIdsPInlineS void operator ++()
				{
					Next();
				}
				
				DIdsPInlineS void operator --()
				{
					Prev();
				}
			};

			DIdsPInlineM mint f_GetLen() const
			{
				TIterator<DIdsTreeAvlDefaultIteratorRecursionDepth> Iter(this);
				mint Len = 0;
				while (Iter)
				{
					++Len;
					++Iter;
				}
				return Len;
			}
			



			void f_DeleteAll()
			{
				f_DeleteAll((void *)DNP);
			}

			template <typename t_CContext>
			void f_DeleteAll(t_CContext _Context = (void *)DNP)
			{
				while (GetRoot())
				{
					CData *pData = GetRoot();
					f_Remove(pData, _Context);
					pData->~CData();
					CAllocator::Free(pData);
				}
			}

			void RemoveAll()
			{
                RemoveAll((void *)DNP);
			}

			template <typename t_CContext>
			void RemoveAll(t_CContext _Context = (void *)DNP)
			{
				while (GetRoot())
				{
					f_Remove(GetRoot(), _Context);
				}
			}

			void DeRefAll(void *_pContext = DNP)
			{
				while (GetRoot())
				{
					CData *pData = GetRoot();
					f_Remove(pData, _pContext);
					pData->MRTC_RemoveRef();
				}
			}



            /************************************************************************************************\
			||¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯||
			|| Debug functions
			||______________________________________________________________________________________________||
			\************************************************************************************************/

			template <typename t_CContext>
			bint CheckTreeRecurse(CLinkData *_pCurrent, bint _bBreak, t_CContext _Context)
			{
				CLinkData *pLeft = _pCurrent->GetLeftP();
				CLinkData *pRight = _pCurrent->GetRightP();

				if (pLeft)
				{
					if (CCompare::Compare(MemberFromLink(_pCurrent), MemberFromLink(pLeft), _Context) <= 0)
					{
						if (_bBreak)
						{
							DIdsPDebugBreak;
						}
						return false;
					}
					if (!CheckTreeRecurse(pLeft, _bBreak, _Context))
						return false;
				}				
				if (pRight)
				{
					if (CCompare::Compare(MemberFromLink(_pCurrent), MemberFromLink(pRight), _Context) >= 0)
					{
						if (_bBreak)
						{
							DIdsPDebugBreak;
						}
						return false;
					}
					if (!CheckTreeRecurse(pRight, _bBreak, _Context))
						return false;
				}

				return true;
			}

			bint CheckTree(bint _bBreak)
			{
				return CheckTree(_bBreak, (void *)DNP);
			}

			template <typename t_CContext>
			bint CheckTree(bint _bBreak, t_CContext _Context = (void *)DNP)
			{
				if (CLinkData::GetPtr(m_Root))
				{
					bint bFail = !CheckTreeRecurse(CLinkData::GetPtr(m_Root), _bBreak, _Context);
					if (bFail)
						return false;
				}

				TIterator<DIdsTreeAvlDefaultIteratorRecursionDepth> Iter(this);

				while (Iter)
				{
					if (Iter.m_pStack[Iter.m_iStack]->GetSkew() == CLinkData::EAVLTreeSkew_NotInTree)
					{
						if (_bBreak)
						{
							DIdsPDebugBreak;
						}
						return false;
					}

					++Iter;
				}

				return true;
			}	

		};

		template <typename t_CParams>
		class TCAVLTree : public TCAVLTreeAggregate<t_CParams>
		{
		public:
			TCAVLTree()
			{
				TCAVLTreeAggregate<t_CParams>::f_Construct();
			}

			~TCAVLTree()
			{
				TCAVLTreeAggregate<t_CParams>::f_Destruct();
			}
		};

		template <typename t_CParamsLink>
		class TCAVLLinkAggregate
		{
		public:
			typename t_CParamsLink::CLinkData m_Link;
			class CParams : public t_CParamsLink
			{
				public:
				typedef TCAVLLinkAggregate CLinkType;			
			};
			
			DIdsPInlineS void f_Construct()
			{
				m_Link.f_Construct();
			}

			DIdsPInlineS void f_Destruct(TCAVLTreeAggregate<CParams> *_pTree)
			{
				DIdsAssert(_pTree, "You have to supply the tree to the destructor");
				_pTree->f_Remove(this);
			}

			bint IsInTree()
			{
				return m_Link.GetSkew() != t_CParamsLink::CLinkData::EAVLTreeSkew_NotInTree;
			}

			typename t_CParamsLink::CData *GetRight()
			{
				if (m_Link.GetRightP())
					return TCAVLTreeAggregate<CParams>::MemberFromLink(m_Link.GetRightP());
				else
					return DNP;

			}

			typename t_CParamsLink::CData *GetLeft()
			{
				if (m_Link.GetLeftP())
					return TCAVLTreeAggregate<CParams>::MemberFromLink(m_Link.GetLeftP());
				else
					return DNP;
			}

			DIdsPInlineS TCAVLLinkAggregate<CParams> *GetNext()
			{
				return (TCAVLLinkAggregate<CParams>)m_Link.m_pNext;
			}

			DIdsPInlineS TCAVLLinkAggregate<CParams> *GetPrev()
			{
				return (TCAVLLinkAggregate<CParams> *)m_Link.m_pPrev;
			}

			DIdsPInlineS void SetTree(TCAVLTreeAggregate<CParams> * _pTree)
			{				
			}

		};

		template <typename t_CParamsLink>
		class TCAVLLink : public TCAVLLinkAggregate<t_CParamsLink>
		{
		public:
			TCAVLLink()
			{
				TCAVLLinkAggregate<t_CParamsLink>::f_Construct();
			}

#			ifdef DIdsDebug
				~TCAVLLink()
				{
					aint Skew = TCAVLLinkAggregate<t_CParamsLink>::m_Link.GetSkew();
					DIdsAssert(Skew == t_CParamsLink::CLinkData::EAVLTreeSkew_NotInTree, "You have to remove the link from the tree yourself,\n or if you are willing to sacrifice sizeof(void *) bytes you can use TCAVLLinkAR");
				}
#			endif
		};

		template <typename t_CParamsLink>
		class TCAVLLinkARAggregate
		{
		public:
			typename t_CParamsLink::CLinkData m_Link;
			class CParams : public t_CParamsLink
			{
			public:
				typedef TCAVLLinkARAggregate CLinkType;			
			};
			
			TCAVLTreeAggregate<CParams> *m_pTree;
			DIdsPInlineS void f_Construct()
			{
				m_Link.f_Construct();
			}

			DIdsPInlineS void f_Destruct()
			{
				if (m_Link.GetSkew() != t_CParamsLink::CLinkData::EAVLTreeSkew_NotInTree)
					m_pTree->f_Remove(this);
			}

			bint IsInTree()
			{
				return m_Link.GetSkew() != t_CParamsLink::CLinkData::EAVLTreeSkew_NotInTree;
			}

			DIdsPInlineS TCAVLLinkARAggregate<t_CParamsLink> *GetNext()
			{
				return (TCAVLLinkARAggregate<t_CParamsLink> *)m_Link.m_pNext;
			}

			DIdsPInlineS TCAVLLinkARAggregate<t_CParamsLink> *GetPrev()
			{
				return (TCAVLLinkARAggregate<t_CParamsLink> *)m_Link.m_pPrev;
			}

			DIdsPInlineS void SetTree(TCAVLTreeAggregate<CParams> *_pTree)
			{				
				m_pTree = _pTree;
			}
		};

		template <typename t_CParamsLink>
		class TCAVLLinkAR : public TCAVLLinkARAggregate<t_CParamsLink>
		{
		public:
			TCAVLLinkAR()
			{
				TCAVLLinkARAggregate<t_CParamsLink>::f_Construct();
			}
			~TCAVLLinkAR()
			{
				TCAVLLinkARAggregate<t_CParamsLink>::f_Destruct();
			}
		};

#		define DIdsTreeAVL_Trans(_Class, _Member) \
				class CAVLTree_Translator##_Member \
				{\
				public:\
					DIdsPInlineS static aint GetOffset()\
					{\
						const _Class *pPtr = 0;\
						return (aint)(void *)(&((pPtr)->_Member));\
					}\
				};\

        /***************************************************************************************************\
        |¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|
        | Pointer holder is void *																			|
        |___________________________________________________________________________________________________|
        \***************************************************************************************************/
#		define DIdsTreeAVL_LinkType(_cClass, _Member, _cKey, _cCompare) NIds::NTree::TCAVLLink< NIds::NTree::TCAVLTreeParams<_cClass, _cKey, NIds::NTree::TCAVLLinkData<CDefaultPointerHolder>, _cClass::CAVLTree_Translator##_Member, _cCompare, DIdsDefaultAllocator > >
#		define DIdsTreeAVL_Member(_cClass, _Member, _cKey, _cCompare) DIdsTreeAVL_LinkType(_cClass, _Member, _cKey, _cCompare) _Member;

#		define DIdsTreeAVL_Link(_cClass, _Member, _cKey, _cCompare) \
					DIdsTreeAVL_Trans(_cClass, _Member) \
					DIdsTreeAVL_Member(_cClass, _Member, _cKey, _cCompare)


#		define DIdsTreeAVLAligned_LinkType(_cClass, _Member, _cKey, _cCompare) NIds::NTree::TCAVLLink< NIds::NTree::TCAVLTreeParams<_cClass, _cKey, NIds::NTree::TCAVLLinkAlignedData<CDefaultPointerHolder>, _cClass::CAVLTree_Translator##_Member, _cCompare, DIdsDefaultAllocator > >
#		define DIdsTreeAVLAligned_Member(_cClass, _Member, _cKey, _cCompare) DIdsTreeAVLAligned_LinkType(_cClass, _Member, _cKey, _cCompare) _Member;

#		define DIdsTreeAVLAligned_Link(_cClass, _Member, _cKey, _cCompare) \
					DIdsTreeAVL_Trans(_cClass, _Member) \
					DIdsTreeAVLAligned_Member(_cClass, _Member, _cKey, _cCompare)
					
					
#		define DIdsTreeAVLA_LinkType(_cClass, _Member, _cKey, _cCompare) NIds::NTree::TCAVLLinkAggregate< NIds::NTree::TCAVLTreeParams<_cClass, _cKey, NIds::NTree::TCAVLLinkData<CDefaultPointerHolder>, _cClass::CAVLTree_Translator##_Member, _cCompare, DIdsDefaultAllocator > >
#		define DIdsTreeAVLA_Member(_cClass, _Member, _cKey, _cCompare) DIdsTreeAVLA_LinkType(_cClass, _Member, _cKey, _cCompare) _Member;

#		define DIdsTreeAVLA_Link(_cClass, _Member, _cKey, _cCompare) \
					DIdsTreeAVL_Trans(_cClass, _Member) \
					DIdsTreeAVLA_Member(_cClass, _Member, _cKey, _cCompare)

#		define DIdsTreeAVLAlignedA_LinkType(_cClass, _Member, _cKey, _cCompare) NIds::NTree::TCAVLLinkAggregate< NIds::NTree::TCAVLTreeParams<_cClass, _cKey, NIds::NTree::TCAVLLinkAlignedData<CDefaultPointerHolder>, _cClass::CAVLTree_Translator##_Member, _cCompare, DIdsDefaultAllocator > >
#		define DIdsTreeAVLAlignedA_Member(_cClass, _Member, _cKey, _cCompare) DIdsTreeAVLAlignedA_LinkType(_cClass, _Member, _cKey, _cCompare) _Member;
					
#		define DIdsTreeAVLAlignedA_Link(_cClass, _Member, _cKey, _cCompare) \
					DIdsTreeAVL_Trans(_cClass, _Member) \
					DIdsTreeAVLAlignedA_Member(_cClass, _Member, _cKey, _cCompare)
					
					
#		define DIdsTreeAVL_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare) NIds::NTree::TCAVLLink< NIds::NTree::TCAVLTreeParams<_cClass, _cKey, NIds::NTree::TCAVLLinkData<CDefaultPointerHolder>, typename _cClass::CAVLTree_Translator##_Member, _cCompare, DIdsDefaultAllocator > >
#		define DIdsTreeAVL_Member_FromTemplate(_cClass, _Member, _cKey, _cCompare) DIdsTreeAVL_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare) _Member;
					
#		define DIdsTreeAVL_Link_FromTemplate(_cClass, _Member, _cKey, _cCompare) \
					DIdsTreeAVL_Trans(_cClass, _Member) \
					DIdsTreeAVL_Member_FromTemplate(_cClass, _Member, _cKey, _cCompare)
					
					
#		define DIdsTreeAVLAligned_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare) NIds::NTree::TCAVLLink< NIds::NTree::TCAVLTreeParams<_cClass, _cKey, NIds::NTree::TCAVLLinkAlignedData<CDefaultPointerHolder>, typename _cClass::CAVLTree_Translator##_Member, _cCompare, DIdsDefaultAllocator > >
#		define DIdsTreeAVLAligned_Member_FromTemplate(_cClass, _Member, _cKey, _cCompare) DIdsTreeAVLAligned_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare) _Member;
					
#		define DIdsTreeAVLAligned_Link_FromTemplate(_cClass, _Member, _cKey, _cCompare) \
					DIdsTreeAVL_Trans(_cClass, _Member) \
					DIdsTreeAVLAligned_Member_FromTemplate(_cClass, _Member, _cKey, _cCompare)
					
					
#		define DIdsTreeAVLA_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare) NIds::NTree::TCAVLLinkAggregate< NIds::NTree::TCAVLTreeParams<_cClass, _cKey, NIds::NTree::TCAVLLinkData<CDefaultPointerHolder>, typename _cClass::CAVLTree_Translator##_Member, _cCompare, DIdsDefaultAllocator > >
#		define DIdsTreeAVLA_Member_FromTemplate(_cClass, _Member, _cKey, _cCompare) DIdsTreeAVLA_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare) _Member;
					
#		define DIdsTreeAVLA_Link_FromTemplate(_cClass, _Member, _cKey, _cCompare) \
					DIdsTreeAVL_Trans(_cClass, _Member) \
					DIdsTreeAVLA_Member_FromTemplate(_cClass, _Member, _cKey, _cCompare)
					
#		define DIdsTreeAVLAlignedA_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare) NIds::NTree::TCAVLLinkAggregate< NIds::NTree::TCAVLTreeParams<_cClass, _cKey, NIds::NTree::TCAVLLinkAlignedData<CDefaultPointerHolder>, typename _cClass::CAVLTree_Translator##_Member, _cCompare, DIdsDefaultAllocator > >
#		define DIdsTreeAVLAlignedA_Member_FromTemplate(_cClass, _Member, _cKey, _cCompare) DIdsTreeAVLAlignedA_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare) _Member;
					
#		define DIdsTreeAVLAlignedA_Link_FromTemplate(_cClass, _Member, _cKey, _cCompare) \
					DIdsTreeAVL_Trans(_cClass, _Member) \
					DIdsTreeAVLAlignedA_Member_FromTemplate(_cClass, _Member, _cKey, _cCompare)
					
										
#		define DIdsTreeAVL_Tree(_cClass, _Member, _cKey, _cCompare) NIds::NTree::TCAVLTree<DIdsTreeAVLA_LinkType(_cClass, _Member, _cKey, _cCompare)::CParams>
#		define DIdsTreeAVLAligned_Tree(_cClass, _Member, _cKey, _cCompare) NIds::NTree::TCAVLTree<DIdsTreeAVLAlignedA_LinkType(_cClass, _Member, _cKey, _cCompare)::CParams>
#		define DIdsTreeAVLA_Tree(_cClass, _Member, _cKey, _cCompare) NIds::NTree::TCAVLTreeAggregate<DIdsTreeAVLA_LinkType(_cClass, _Member, _cKey, _cCompare)::CParams>
#		define DIdsTreeAVLAlignedA_Tree(_cClass, _Member, _cKey, _cCompare) NIds::NTree::TCAVLTreeAggregate<DIdsTreeAVLAlignedA_LinkType(_cClass, _Member, _cKey, _cCompare)::CParams>

#		define DIdsTreeAVL_Tree_FromTemplate(_cClass, _Member, _cKey, _cCompare) NIds::NTree::TCAVLTree<typename DIdsTreeAVLA_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare)::CParams>
#		define DIdsTreeAVLAligned_Tree_FromTemplate(_cClass, _Member, _cKey, _cCompare) NIds::NTree::TCAVLTree<typename DIdsTreeAVLAlignedA_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare)::CParams>
#		define DIdsTreeAVLA_Tree_FromTemplate(_cClass, _Member, _cKey, _cCompare) NIds::NTree::TCAVLTreeAggregate<typename DIdsTreeAVLA_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare)::CParams>
#		define DIdsTreeAVLAlignedA_Tree_FromTemplate(_cClass, _Member, _cKey, _cCompare) NIds::NTree::TCAVLTreeAggregate<typename DIdsTreeAVLAlignedA_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare)::CParams>

#		define DIdsTreeAVL_Iterator(_cClass, _Member, _cKey, _cCompare) DIdsTreeAVLA_Tree(_cClass, _Member, _cKey, _cCompare)::TIterator<DIdsTreeAvlDefaultIteratorRecursionDepth>
#		define DIdsTreeAVL_IteratorDepth(_cClass, _Member, _cKey, _cCompare, _Depth) DIdsTreeAVLA_Tree(_cClass, _Member, _cKey, _cCompare)::TIterator<_Depth>
#		define DIdsTreeAVLAligned_Iterator(_cClass, _Member, _cKey, _cCompare) DIdsTreeAVLAlignedA_Tree(_cClass, _Member, _cKey, _cCompare)::TIterator<DIdsTreeAvlDefaultIteratorRecursionDepth>
#		define DIdsTreeAVLAligned_IteratorDepth(_cClass, _Member, _cKey, _cCompare, _Depth) DIdsTreeAVLAlignedA_Tree(_cClass, _Member, _cKey, _cCompare)::TIterator<_Depth>

#		define DIdsTreeAVL_Iterator_FromTemplate(_cClass, _Member, _cKey, _cCompare) typename DIdsTreeAVLA_Tree_FromTemplate(_cClass, _Member, _cKey, _cCompare)::template TIterator<DIdsTreeAvlDefaultIteratorRecursionDepth>
#		define DIdsTreeAVL_IteratorDepth_FromTemplate(_cClass, _Member, _cKey, _cCompare, _Depth) typename DIdsTreeAVLA_Tree_FromTemplate(_cClass, _Member, _cKey, _cCompare)::template TIterator<_Depth>
#		define DIdsTreeAVLAligned_Iterator_FromTemplate(_cClass, _Member, _cKey, _cCompare) typename DIdsTreeAVLAlignedA_Tree_FromTemplate(_cClass, _Member, _cKey, _cCompare)::template TIterator<DIdsTreeAvlDefaultIteratorRecursionDepth>
#		define DIdsTreeAVLAligned_IteratorDepth_FromTemplate(_cClass, _Member, _cKey, _cCompare, _Depth) typename DIdsTreeAVLAlignedA_Tree_FromTemplate(_cClass, _Member, _cKey, _cCompare)::template TIterator<_Depth>
					
					
#		ifndef DIdsPNoShortCuts
#			define DAVL_Link(_cClass, _Member, _cKey, _cCompare) DIdsTreeAVL_Link(_cClass, _Member, _cKey, _cCompare)
#			define DAVL_Tree(_cClass, _Member, _cKey, _cCompare) DIdsTreeAVL_Tree(_cClass, _Member, _cKey, _cCompare)
#			define DAVLA_Link(_cClass, _Member, _cKey, _cCompare) DIdsTreeAVLA_Link(_cClass, _Member, _cKey, _cCompare)
#			define DAVLA_Tree(_cClass, _Member, _cKey, _cCompare) DIdsTreeAVLA_Tree(_cClass, _Member, _cKey, _cCompare)
#			define DAVLAligned_Link(_cClass, _Member, _cKey, _cCompare) DIdsTreeAVLAligned_Link(_cClass, _Member, _cKey, _cCompare)
#			define DAVLAligned_Tree(_cClass, _Member, _cKey, _cCompare) DIdsTreeAVLAligned_Tree(_cClass, _Member, _cKey, _cCompare)
#			define DAVLAlignedA_Link(_cClass, _Member, _cKey, _cCompare) DIdsTreeAVLAlignedA_Link(_cClass, _Member, _cKey, _cCompare)
#			define DAVLAlignedA_Tree(_cClass, _Member, _cKey, _cCompare) DIdsTreeAVLAlignedA_Tree(_cClass, _Member, _cKey, _cCompare)
#		endif

        /***************************************************************************************************\
        |¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|
        | Pointer holder is variable																		|
        |___________________________________________________________________________________________________|
        \***************************************************************************************************/

#		define DIdsTreeAVLAllocator_LinkType(_cClass, _Member, _cKey, _cCompare, _cAllocator) NIds::NTree::TCAVLLink< NIds::NTree::TCAVLTreeParams<_cClass, _cKey, NIds::NTree::TCAVLLinkData< _cAllocator::CPtrHolder >, _cClass::CAVLTree_Translator##_Member, _cCompare, _cAllocator > >
#		define DIdsTreeAVLAllocator_Member(_cClass, _Member, _cKey, _cCompare, _cAllocator) DIdsTreeAVLAllocator_LinkType(_cClass, _Member, _cKey, _cCompare, _cAllocator) _Member;
					
#		define DIdsTreeAVLAllocator_Link(_cClass, _Member, _cKey, _cCompare, _cAllocator) \
					DIdsTreeAVL_Trans(_cClass, _Member) \
					DIdsTreeAVLAllocator_Member(_cClass, _Member, _cKey, _cCompare, _cAllocator)
					
					
#		define DIdsTreeAVLAlignedAllocator_LinkType(_cClass, _Member, _cKey, _cCompare, _cAllocator) NIds::NTree::TCAVLLink< NIds::NTree::TCAVLTreeParams<_cClass, _cKey, NIds::NTree::TCAVLLinkAlignedData< _cAllocator::CPtrHolder >, _cClass::CAVLTree_Translator##_Member, _cCompare, _cAllocator > >
#		define DIdsTreeAVLAlignedAllocator_Member(_cClass, _Member, _cKey, _cCompare, _cAllocator) DIdsTreeAVLAlignedAllocator_LinkType(_cClass, _Member, _cKey, _cCompare, _cAllocator) _Member;
					
#		define DIdsTreeAVLAlignedAllocator_Link(_cClass, _Member, _cKey, _cCompare, _cAllocator) \
					DIdsTreeAVL_Trans(_cClass, _Member) \
					DIdsTreeAVLAlignedAllocator_Member(_cClass, _Member, _cKey, _cCompare, _cAllocator)
					
					
#		define DIdsTreeAVLAllocatorA_LinkType(_cClass, _Member, _cKey, _cCompare, _cAllocator) NIds::NTree::TCAVLLinkAggregate< NIds::NTree::TCAVLTreeParams<_cClass, _cKey, NIds::NTree::TCAVLLinkData< _cAllocator::CPtrHolder >, _cClass::CAVLTree_Translator##_Member, _cCompare, _cAllocator > >
#		define DIdsTreeAVLAllocatorA_Member(_cClass, _Member, _cKey, _cCompare, _cAllocator) DIdsTreeAVLAllocatorA_LinkType(_cClass, _Member, _cKey, _cCompare, _cAllocator) _Member;
					
#		define DIdsTreeAVLAllocatorA_Link(_cClass, _Member, _cKey, _cCompare, _cAllocator) \
					DIdsTreeAVL_Trans(_cClass, _Member) \
					DIdsTreeAVLAllocatorA_Member(_cClass, _Member, _cKey, _cCompare, _cAllocator)
					
#		define DIdsTreeAVLAlignedAllocatorA_LinkType(_cClass, _Member, _cKey, _cCompare, _cAllocator) NIds::NTree::TCAVLLinkAggregate< NIds::NTree::TCAVLTreeParams<_cClass, _cKey, NIds::NTree::TCAVLLinkAlignedData< _cAllocator::CPtrHolder >, _cClass::CAVLTree_Translator##_Member, _cCompare, _cAllocator > >
#		define DIdsTreeAVLAlignedAllocatorA_Member(_cClass, _Member, _cKey, _cCompare, _cAllocator) DIdsTreeAVLAlignedAllocatorA_LinkType(_cClass, _Member, _cKey, _cCompare, _cAllocator) _Member;
					
#		define DIdsTreeAVLAlignedAllocatorA_Link(_cClass, _Member, _cKey, _cCompare, _cAllocator) \
					DIdsTreeAVL_Trans(_cClass, _Member) \
					DIdsTreeAVLAlignedAllocatorA_Member(_cClass, _Member, _cKey, _cCompare, _cAllocator)
					
					
					
					
					
					
#		define DIdsTreeAVLAllocator_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) NIds::NTree::TCAVLLink< NIds::NTree::TCAVLTreeParams<_cClass, _cKey, NIds::NTree::TCAVLLinkData< typename _cAllocator::CPtrHolder >, typename _cClass::CAVLTree_Translator##_Member, _cCompare, _cAllocator > >
#		define DIdsTreeAVLAllocator_Member_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) DIdsTreeAVLAllocator_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) _Member;
					
#		define DIdsTreeAVLAllocator_Link_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) \
					DIdsTreeAVL_Trans(_cClass, _Member) \
					DIdsTreeAVLAllocator_Member_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator)
					
					
#		define DIdsTreeAVLAlignedAllocator_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) NIds::NTree::TCAVLLink< NIds::NTree::TCAVLTreeParams<_cClass, _cKey, NIds::NTree::TCAVLLinkAlignedData< typename _cAllocator::CPtrHolder >, typename _cClass::CAVLTree_Translator##_Member, _cCompare, _cAllocator > >
#		define DIdsTreeAVLAlignedAllocator_Member_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) DIdsTreeAVLAlignedAllocator_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) _Member;
					
#		define DIdsTreeAVLAlignedAllocator_Link_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) \
					DIdsTreeAVL_Trans(_cClass, _Member) \
					DIdsTreeAVLAlignedAllocator_Member_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator)
					
					
#		define DIdsTreeAVLAllocatorA_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) NIds::NTree::TCAVLLinkAggregate< NIds::NTree::TCAVLTreeParams<_cClass, _cKey, NIds::NTree::TCAVLLinkData< typename _cAllocator::CPtrHolder >, typename _cClass::CAVLTree_Translator##_Member, _cCompare, _cAllocator > >
#		define DIdsTreeAVLAllocatorA_Member_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) DIdsTreeAVLAllocatorA_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) _Member;
					
#		define DIdsTreeAVLAllocatorA_Link_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) \
					DIdsTreeAVL_Trans(_cClass, _Member) \
					DIdsTreeAVLAllocatorA_Member_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator)
					
#		define DIdsTreeAVLAlignedAllocatorA_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) NIds::NTree::TCAVLLinkAggregate< NIds::NTree::TCAVLTreeParams<_cClass, _cKey, NIds::NTree::TCAVLLinkAlignedData< typename _cAllocator::CPtrHolder >, typename _cClass::CAVLTree_Translator##_Member, _cCompare, _cAllocator > >
#		define DIdsTreeAVLAlignedAllocatorA_Member_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) DIdsTreeAVLAlignedAllocatorA_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) _Member;
					
#		define DIdsTreeAVLAlignedAllocatorA_Link_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) \
					DIdsTreeAVL_Trans(_cClass, _Member) \
					DIdsTreeAVLAlignedAllocatorA_Member_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator)
					
					
					
					
#		define DIdsTreeAVLAllocator_Tree(_cClass, _Member, _cKey, _cCompare, _cAllocator) NIds::NTree::TCAVLTree<DIdsTreeAVLAllocatorA_LinkType(_cClass, _Member, _cKey, _cCompare, _cAllocator)::CParams>
#		define DIdsTreeAVLAlignedAllocator_Tree(_cClass, _Member, _cKey, _cCompare, _cAllocator) NIds::NTree::TCAVLTree<DIdsTreeAVLAlignedAllocatorA_LinkType(_cClass, _Member, _cKey, _cCompare, _cAllocator)::CParams>
#		define DIdsTreeAVLAllocatorA_Tree(_cClass, _Member, _cKey, _cCompare, _cAllocator) NIds::NTree::TCAVLTreeAggregate<DIdsTreeAVLAllocatorA_LinkType(_cClass, _Member, _cKey, _cCompare, _cAllocator)::CParams>
#		define DIdsTreeAVLAlignedAllocatorA_Tree(_cClass, _Member, _cKey, _cCompare, _cAllocator) NIds::NTree::TCAVLTreeAggregate<DIdsTreeAVLAlignedAllocatorA_LinkType(_cClass, _Member, _cKey, _cCompare, _cAllocator)::CParams>

#		define DIdsTreeAVLAllocator_Tree_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) NIds::NTree::TCAVLTree<typename DIdsTreeAVLAllocatorA_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator)::CParams>
#		define DIdsTreeAVLAlignedAllocator_Tree_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) NIds::NTree::TCAVLTree<typename DIdsTreeAVLAlignedAllocatorA_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator)::CParams>
#		define DIdsTreeAVLAllocatorA_Tree_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) NIds::NTree::TCAVLTreeAggregate<typename DIdsTreeAVLAllocatorA_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator)::CParams>
#		define DIdsTreeAVLAlignedAllocatorA_Tree_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) NIds::NTree::TCAVLTreeAggregate< typename DIdsTreeAVLAlignedAllocatorA_LinkType_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator)::CParams >
					
#		define DIdsTreeAVLAllocator_Iterator(_cClass, _Member, _cKey, _cCompare, _cAllocator) DIdsTreeAVLAllocatorA_Tree(_cClass, _Member, _cKey, _cCompare, _cAllocator)::TIterator<DIdsTreeAvlDefaultIteratorRecursionDepth>
#		define DIdsTreeAVLAllocator_IteratorDepth(_cClass, _Member, _cKey, _cCompare, _Depth, _cAllocator) DIdsTreeAVLAllocatorA_Tree(_cClass, _Member, _cKey, _cCompare, _cAllocator)::TIterator<_Depth>
#		define DIdsTreeAVLAlignedAllocator_Iterator(_cClass, _Member, _cKey, _cCompare, _cAllocator) DIdsTreeAVLAlignedAllocatorA_Tree(_cClass, _Member, _cKey, _cCompare, _cAllocator)::TIterator<DIdsTreeAvlDefaultIteratorRecursionDepth>
#		define DIdsTreeAVLAlignedAllocator_IteratorDepth(_cClass, _Member, _cKey, _cCompare, _Depth, _cAllocator) DIdsTreeAVLAlignedAllocatorA_Tree(_cClass, _Member, _cKey, _cCompare, _cAllocator)::TIterator<_Depth>
					
#		define DIdsTreeAVLAllocator_Iterator_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) typename DIdsTreeAVLAllocatorA_Tree_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator)::template TIterator<DIdsTreeAvlDefaultIteratorRecursionDepth>
#		define DIdsTreeAVLAllocator_IteratorDepth_FromTemplate(_cClass, _Member, _cKey, _cCompare, _Depth, _cAllocator) typename DIdsTreeAVLAllocatorA_Tree_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator)::template TIterator<_Depth>
#		define DIdsTreeAVLAlignedAllocator_Iterator_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator) typename DIdsTreeAVLAlignedAllocatorA_Tree_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator)::template TIterator<DIdsTreeAvlDefaultIteratorRecursionDepth>
#		define DIdsTreeAVLAlignedAllocator_IteratorDepth_FromTemplate(_cClass, _Member, _cKey, _cCompare, _Depth, _cAllocator) typename DIdsTreeAVLAlignedAllocatorA_Tree_FromTemplate(_cClass, _Member, _cKey, _cCompare, _cAllocator)::template TIterator<_Depth>
					
#		ifndef DIdsPNoShortCuts
#			define DAVLAllocator_Link(_cClass, _Member, _cKey, _cCompare, _Allocator) DIdsTreeAVLAllocator_Link(_cClass, _Member, _cKey, _cCompare, _Allocator)
#			define DAVLAllocator_Tree(_cClass, _Member, _cKey, _cCompare, _Allocator) DIdsTreeAVLAllocator_Tree(_cClass, _Member, _cKey, _cCompare, _Allocator)
#			define DAVLAllocatorA_Link(_cClass, _Member, _cKey, _cCompare, _Allocator) DIdsTreeAVLAllocatorA_Link(_cClass, _Member, _cKey, _cCompare, _Allocator)
#			define DAVLAllocatorA_Tree(_cClass, _Member, _cKey, _cCompare, _Allocator) DIdsTreeAVLAllocatorA_Tree(_cClass, _Member, _cKey, _cCompare, _Allocator)
#			define DAVLAlignedAllocator_Link(_cClass, _Member, _cKey, _cCompare, _Allocator) DIdsTreeAVLAlignedAllocator_Link(_cClass, _Member, _cKey, _cCompare, _Allocator)
#			define DAVLAlignedAllocator_Tree(_cClass, _Member, _cKey, _cCompare, _Allocator) DIdsTreeAVLAlignedAllocator_Tree(_cClass, _Member, _cKey, _cCompare, _Allocator)
#			define DAVLAlignedAllocatorA_Link(_cClass, _Member, _cKey, _cCompare, _Allocator) DIdsTreeAVLAlignedAllocatorA_Link(_cClass, _Member, _cKey, _cCompare, _Allocator)
#			define DAVLAlignedAllocatorA_Tree(_cClass, _Member, _cKey, _cCompare, _Allocator) DIdsTreeAVLAlignedAllocatorA_Tree(_cClass, _Member, _cKey, _cCompare, _Allocator)
#		endif

};


	namespace NList
	{		

		/************************************************************************************************\
		||¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯||
		|| Double linked list
		||______________________________________________________________________________________________||
		\************************************************************************************************/
		template <typename t_CAllocator>
		class TCDLinkAggregateListNoPrevPtrList
		{
		public:
			TCDynamicPtr<typename t_CAllocator::CPtrHolder, TCDLinkAggregateListNoPrevPtrList> m_pNextvPtr;
		};

		template <typename t_CAllocator>
		class TCDLinkAggregateListNoPrevPtr
		{
		public:
			TCDynamicPtr<typename t_CAllocator::CPtrHolder, TCDLinkAggregateListNoPrevPtr> m_pNextvPtr;
			TCDynamicPtr<typename t_CAllocator::CPtrHolder, TCDLinkAggregateListNoPrevPtr> m_pPrevPtr;

			DIdsPInlineXL void Link(TCDLinkAggregateListNoPrevPtr *_pLinkAfter)
			{
				if (_pLinkAfter != this)
				{
					if (GetNext())
					{	// We unlink first in case we already are in a list
						UnlinkInternal();
					}

					LinkNoUnlink(_pLinkAfter);
				}
			}

			DIdsPInlineS void LinkNoUnlink(TCDLinkAggregateListNoPrevPtr *_pLinkAfter)
			{
				if (_pLinkAfter->GetNext() == _pLinkAfter)
				{
					// Empty list
					SetPrevInit(this);
					SetNextInit(_pLinkAfter);
					_pLinkAfter->SetNext(this);
				}
				else if (_pLinkAfter->IsListLink())
				{
					SetPrevInit(_pLinkAfter->GetNext()->m_pPrevPtr);
					SetNextInit(_pLinkAfter->GetNext());
					GetNext()->SetPrev(this);
					_pLinkAfter->SetNext(this);
				}
				else if (_pLinkAfter->GetNext()->IsListLink())
				{
					SetPrevInit(_pLinkAfter);
					SetNextInit(_pLinkAfter->GetNext());
					GetNext()->GetNext()->SetPrev(this);
					m_pPrevPtr->SetNext(this);
				}
				else
				{
					SetPrevInit(_pLinkAfter);
					SetNextInit(_pLinkAfter->GetNext());
					GetNext()->SetPrev(this);
					m_pPrevPtr->SetNext(this);
				}
			}

			DIdsPInlineXL void UnlinkInternal()
			{
				if (GetPrev()->IsListLink())
				{
					if (GetNext()->IsListLink())
					{
						// Alone in list
						GetNext()->SetNext(GetNext());
					}
					else
					{
						GetNext()->SetPrev(m_pPrevPtr);
						GetPrev()->SetNext(GetNext());
					}
				}
				else if (GetNext()->IsListLink())
				{
					GetNext()->GetNext()->SetPrev(m_pPrevPtr);
					m_pPrevPtr->SetNext(GetNext());
				}
				else
				{
					// Between two usual blocks
					GetNext()->SetPrev(m_pPrevPtr);
					m_pPrevPtr->SetNext(GetNext());
				}
			}

			DIdsPInlineS void TransferList(TCDLinkAggregateListNoPrevPtr *_pFirst, TCDLinkAggregateListNoPrevPtr *_pLast)
			{
				if (_pFirst->IsListLink())
				{
                    // Empty list
					Construct();
				}
				else
				{
					SetNextInitListLink(_pFirst);
					_pFirst->SetPrev(_pLast);
					_pLast->SetNext(this);
				}
			}

			DIdsPInlineXL void Unlink()
			{
				if (GetNext())
				{
					UnlinkInternal();
	                SetNextInit(DNP);
#					ifdef DIdsDebug
						// Clear the prev ptr in debug so ppl don't get confused about garbled data :)
					SetPrevInit(DNP);
#					endif
				}
			}

			DIdsPInlineS void Construct()
			{
				SetNextInit(DNP);
#				ifdef DIdsDebug
					// Clear the prev ptr in debug so ppl don't get confused about garbled data :)
					SetPrevInit(DNP);
#				endif
			}

			DIdsPInlineS void Destruct()
			{
				if (GetNext())
				{
					UnlinkInternal();
#					ifdef DIdsDebug
	                SetNextInit(DNP);
					SetPrevInit(DNP);
#					endif
				}
			}

			DIdsPInlineS void SetNext(TCDLinkAggregateListNoPrevPtr *_pNext)
			{
				m_pNextvPtr = (TCDLinkAggregateListNoPrevPtr *)((((mint)(TCDLinkAggregateListNoPrevPtr *)m_pNextvPtr) & 1) | (mint)_pNext);
			}
			DIdsPInlineS void SetNextInit(TCDLinkAggregateListNoPrevPtr *_pNext)
			{
				m_pNextvPtr = _pNext;
			}
			DIdsPInlineS void SetNextInitListLink(TCDLinkAggregateListNoPrevPtr *_pNext)
			{
				m_pNextvPtr = (TCDLinkAggregateListNoPrevPtr *)((mint)_pNext | 1);
			}

			DIdsPInlineS void SetPrev(TCDLinkAggregateListNoPrevPtr *_pPrev)
			{
				DIdsAssert(!IsListLink(), "");
				m_pPrevPtr = _pPrev;
			}
			DIdsPInlineS void SetPrevInit(TCDLinkAggregateListNoPrevPtr *_pPrev)
			{
				DIdsAssert(!IsListLink(), "");
				m_pPrevPtr = _pPrev;
			}
			DIdsPInlineS void SetPrevInitListLink(TCDLinkAggregateListNoPrevPtr *_pPrev)
			{
//				DIdsAssert(!IsListLink(), "");
//				m_pPrevPtr = _pPrev;
			}

			DIdsPInlineS bint IsListLink() const
			{
				return (mint)(TCDLinkAggregateListNoPrevPtr *)m_pNextvPtr & 1;
			}

			DIdsPInlineS TCDLinkAggregateListNoPrevPtr *GetNext()
			{
				return (TCDLinkAggregateListNoPrevPtr *)((mint)(TCDLinkAggregateListNoPrevPtr *)m_pNextvPtr & (~1));
			}

			DIdsPInlineS TCDLinkAggregateListNoPrevPtr *GetPrev()
			{
				if (IsListLink())
				{
					if (GetNext()->IsListLink())
						return this;
					else
						return GetNext()->m_pPrevPtr;
				}
				else
				{
					if (m_pPrevPtr->GetNext()->IsListLink())
						return m_pPrevPtr->GetNext();
					else
						return m_pPrevPtr;
				}
			}

			DIdsPInlineS bint IsAloneInList()
			{
				return m_pPrevPtr == this;
			}

			DIdsPInlineS bint IsInList()
			{
				return GetNext() != DNP;
			}
		};

		template <typename t_CAllocator>
		class TCDLinkAggregate
		{
		public:
			TCDynamicPtr<typename t_CAllocator::CPtrHolder, TCDLinkAggregate> m_pNextPtr;
			TCDynamicPtr<typename t_CAllocator::CPtrHolder, TCDLinkAggregate> m_pPrevPtr;

			DIdsPInlineXL void Link(TCDLinkAggregate *_pLinkAfter)
			{
				if (_pLinkAfter != this)
				{
					if (GetNext())
					{	// We unlink first in case we already are in a list
						GetNext()->SetPrev(GetPrev());
						GetPrev()->SetNext(GetNext());
					}
					TCDLinkAggregate *pNext = _pLinkAfter->GetNext();
					SetPrevInit(_pLinkAfter);
					SetNext(pNext);
					pNext->SetPrev(this);
					_pLinkAfter->SetNext(this);
				}
			}

			DIdsPInlineS void LinkNoUnlink(TCDLinkAggregate *_pLinkAfter)
			{
				TCDLinkAggregate *pNext = _pLinkAfter->GetNext();
				SetPrevInit(_pLinkAfter);
				SetNext(pNext);
				pNext->SetPrev(this);
				_pLinkAfter->SetNext(this);
			}

			DIdsPInlineXL void Unlink()
			{
				if (GetNext())
				{
					GetNext()->SetPrev(GetPrev());
					GetPrev()->SetNext(GetNext());
	                SetNext(DNP);
#					ifdef DIdsDebug
						// Clear the prev ptr in debug so ppl don't get confused about garbled data :)
					SetPrevInit(DNP);
#					endif
				}
			}

			DIdsPInlineS void Construct()
			{
				SetNext(DNP);
#				ifdef DIdsDebug
					// Clear the prev ptr in debug so ppl don't get confused about garbled data :)
					SetPrevInit(DNP);
#				endif
			}

			DIdsPInlineS void Destruct()
			{
				if (GetNext())
				{
					GetNext()->SetPrev(GetPrev());
					GetPrev()->SetNext(GetNext());
#					ifdef DIdsDebug
	                SetNext(DNP);
					SetPrevInit(DNP);
#					endif
				}
			}

			DIdsPInlineS void SetNext(TCDLinkAggregate *_pNext)
			{
				m_pNextPtr = _pNext;
			}
			DIdsPInlineS void SetNextInit(TCDLinkAggregate *_pNext)
			{
				m_pNextPtr = _pNext;
			}
			DIdsPInlineS void SetNextInitListLink(TCDLinkAggregate *_pNext)
			{
				m_pNextPtr = _pNext;
			}
			DIdsPInlineS void SetPrev(TCDLinkAggregate *_pPrev)
			{
				m_pPrevPtr = (TCDLinkAggregate *)(((mint)(TCDLinkAggregate *)m_pPrevPtr & 1) | (mint)_pPrev);
			}
			DIdsPInlineS void SetPrevInit(TCDLinkAggregate *_pPrev)
			{
				m_pPrevPtr = _pPrev;
			}
			DIdsPInlineS void SetPrevInitListLink(TCDLinkAggregate *_pPrev)
			{
				m_pPrevPtr = (TCDLinkAggregate *)((mint)_pPrev | 1);
			}

			DIdsPInlineS bint IsListLink() const
			{
				return ((mint)(TCDLinkAggregate *)m_pPrevPtr & 1);
			}

			DIdsPInlineS TCDLinkAggregate *GetNext() const
			{
				return m_pNextPtr;
			}

			DIdsPInlineS TCDLinkAggregate *GetPrev() const
			{
				return (TCDLinkAggregate *)((mint)(TCDLinkAggregate *)m_pPrevPtr & (~1));
			}

			DIdsPInlineS bint IsAloneInList()
			{
				return (GetPrev()->IsListLink()) && (GetNext()->IsListLink());
			}

			DIdsPInlineS void TransferList(TCDLinkAggregate *_pFirst, TCDLinkAggregate *_pLast)
			{
				SetNextInitListLink(_pFirst);
				if (GetNext()) GetNext()->SetPrev(this);
                SetPrevInitListLink(_pLast);
				if (GetPrev()) GetPrev()->SetNext(this);
			}

			DIdsPInlineS bint IsInList()
			{
				return GetNext() != DNP;
			}
		};

		template <typename t_CLink>
		class TDLink
		{
		public:
			t_CLink m_Link;

			DIdsPInlineS TDLink()
			{
				m_Link.Construct();
			}

			DIdsPInlineS ~TDLink()
			{
				m_Link.Destruct();
			}
			DIdsPInlineS void Link(t_CLink *_pLinkAfter)
			{
				m_Link.Link(_pLinkAfter);
			}

			DIdsPInlineS void LinkNoUnlink(t_CLink *_pLinkAfter)
			{
				m_Link.LinkNoUnlink(_pLinkAfter);
			}

			DIdsPInlineS void Unlink()
			{
				m_Link.Unlink();
			}

			DIdsPInlineS void SetNext(t_CLink *_pNext)
			{
				m_Link.SetNext(_pNext);
			}
			DIdsPInlineS void SetNextInit(t_CLink *_pNext)
			{
				m_Link.SetNextInit(_pNext);
			}
			DIdsPInlineS void SetNextInitListLink(t_CLink *_pNext)
			{
				m_Link.SetNextInitListLink(_pNext);
			}
			DIdsPInlineS void SetPrev(t_CLink *_pPrev)
			{
				m_Link.SetPrev(_pPrev);
			}
			DIdsPInlineS void SetPrevInit(t_CLink *_pPrev)
			{
				m_Link.SetPrevInit(_pPrev);
			}
			DIdsPInlineS void SetPrevInitListLink(t_CLink *_pPrev)
			{
				m_Link.SetPrevInitListLink(_pPrev);
			}

			DIdsPInlineS bint IsListLink()
			{
				return m_Link.IsListLink();
			}

			DIdsPInlineS void SetListLink()
			{
				m_Link.SetListLink();
			}

			DIdsPInlineS t_CLink *GetNext()
			{
				return m_Link.GetNext();
			}

			DIdsPInlineS t_CLink *GetPrev()
			{
				return m_Link.GetPrev();
			}

			DIdsPInlineS bint IsAloneInList()
			{
				return m_Link.IsAloneInList();
			}

			DIdsPInlineS bint IsInList()
			{
				return m_Link.IsInList();
			}
		};

		template <typename t_CData, typename t_CTranslator, typename t_CLink, typename t_CLinkInList, bint t_bAutoDelete, typename t_CAllocator>
		class TCDLinkListAggregate
		{
		public:
//			typedef typename t_CAllocator::CPtrHolder CPtrHolder;

			DIdsPInlineS static t_CLink *LinkFromMember(t_CData *_pMember)
			{				
				return ((t_CLink *)(((uint8 *)_pMember) + t_CTranslator::GetOffset()));
			}

			DIdsPInlineS static t_CData *MemberFromLink(t_CLink *_pLink)
			{				
				return ((t_CData *)(((uint8 *)_pLink) - t_CTranslator::GetOffset()));
			}

			DIdsPInlineS static const t_CLink *LinkFromMemberConst(const t_CData *_pMember)
			{				
				return ((const t_CLink *)(((uint8 *)_pMember) + t_CTranslator::GetOffset()));
			}

			DIdsPInlineS static const t_CData *MemberFromLinkConst(const t_CLink *_pLink)
			{				
				return ((const t_CData *)(((uint8 *)_pLink) - t_CTranslator::GetOffset()));
			}


			// Have first and last as dummylinks so they don't have destructors and constructors
//			t_CLink m_First;
//			t_CLink m_Last;
			t_CLinkInList m_Link;

			DIdsPInlineS t_CLink *GetLink() const
			{
				return (t_CLink *)&m_Link;
			}

			DIdsPInlineS void Construct()
			{
				GetLink()->SetNextInitListLink(GetLink());
				GetLink()->SetPrevInitListLink(GetLink());
			}

			DIdsPInlineS void Destruct()
			{
				if (t_bAutoDelete)
					DeleteAll();
				else
					Clear();
			}

			void Clear()
			{
				while (!GetLink()->GetNext()->IsListLink())
				{
					GetLink()->GetNext()->Unlink();
				}
			}

			DIdsPInlineM void Transfer(TCDLinkListAggregate *_pFrom)
			{
				DIdsAssert(IsEmpty(), "We must be empty to be able to transfer from another list");
				if (_pFrom->IsEmpty())
					return;
				UnsafeTransfer(_pFrom);
				_pFrom->Construct();
			}

			DIdsPInlineM void UnsafeTransfer(TCDLinkListAggregate *_pFrom)
			{
				GetLink()->TransferList(_pFrom->GetLink()->GetNext(), _pFrom->GetLink()->GetPrev());
			}

			DIdsPInlineS void Transfer(TCDLinkListAggregate &_From)
			{
				Transfer(&_From);
			}

			DIdsPInlineS void UnsafeTransfer(TCDLinkListAggregate &_From)
			{
				UnsafeTransfer(&_From);
			}

			DIdsPInlineS t_CData *GetFirst()
			{
				if (!GetLink()->GetNext()->IsListLink())
					return MemberFromLink(GetLink()->GetNext());
				else
					return DNP;
			}

			DIdsPInlineS t_CData *GetLast()
			{
				if (!GetLink()->GetPrev()->IsListLink())
					return MemberFromLink(GetLink()->GetPrev());
				else 
					return DNP;
			}

			DIdsPInlineS static t_CData *GetNext(t_CData *_pCurrent)
			{					
				t_CLink *pNext = TCDLinkListAggregate::LinkFromMember(_pCurrent)->GetNext();
				if (pNext && !pNext->IsListLink())
					return TCDLinkListAggregate::MemberFromLink(pNext);
				else
					return DNP;
			}
			
			DIdsPInlineS static t_CData *GetPrev(t_CData *_pCurrent)
			{					
				t_CLink *pPrev = TCDLinkListAggregate::LinkFromMember(_pCurrent)->GetPrev();
				if (pPrev && !pPrev->IsListLink())
					return TCDLinkListAggregate::MemberFromLink(pPrev);
				else
					return DNP;
			}
			
			DIdsPInlineS const t_CData *GetFirstConst() const
			{
				if (!GetLink()->GetNext()->IsListLink())
					return MemberFromLinkConst(GetLink()->GetNext());
				else
					return DNP;
			}

			DIdsPInlineS const t_CData *GetLastConst() const
			{
				if (!GetLink()->GetPrev()->IsListLink())
					return MemberFromLinkConst(GetLink()->GetPrev());
				else 
					return DNP;
			}

			void DeleteAll()
			{
				while(t_CData *pCurrent = GetLast())
				{
					pCurrent->~t_CData();
					t_CAllocator::Free(pCurrent);
				}
			}

			DIdsPInlineS bint IsEmpty()
			{
				return GetLink()->GetNext()->IsListLink();
			}

			/***************************************************************************************************\
            |¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|
            | Inserts																							|
            |___________________________________________________________________________________________________|
            \***************************************************************************************************/

			// Insert 
			DIdsPInlineS void Insert(t_CData *_pToInsert)
			{				
				LinkFromMember(_pToInsert)->Link(GetLink()->GetPrev());
			}
			DIdsPInlineS void Insert(t_CData &_ToInsert)
			{
				Insert(&_ToInsert);
			}

			template <typename t_CSortClass>
			void InsertSorted(t_CData &_ToInsert, void *_pContext = DNP)
			{
				InsertSorted<t_CSortClass>(&_ToInsert, _pContext);
			}

			template <typename t_CSortClass>
			void InsertSorted(t_CData *_pToInsert, void *_pContext = DNP)
			{
				t_CLink *pToLink = LinkFromMember(_pToInsert);
				pToLink->Unlink();

				t_CLink *pCurrent = GetLink()->GetNext();
				t_CLink *pLast = GetLink();

				while (!pCurrent->IsListLink())
				{
					if (t_CSortClass::Compare(_pContext, MemberFromLink(pCurrent), _pToInsert) > 0)
						break;
					pLast = pCurrent;					
					pCurrent = pCurrent->GetNext();
				}

				pToLink->Link(pLast);
			}


			// InsertTail
			DIdsPInlineS void InsertTail(t_CData *_pToInsert)
			{
				Insert(_pToInsert);
			}
			DIdsPInlineS void InsertTail(t_CData &_ToInsert)
			{
				InsertTail(&_ToInsert);
			}

			// InsertHead
			DIdsPInlineS void InsertHead(t_CData *_pToInsert)
			{
				LinkFromMember(_pToInsert)->Link(GetLink());
			}
			DIdsPInlineS void InsertHead(t_CData &_ToInsert)
			{
				InsertHead(&_ToInsert);
			}

			// InsertHead
			DIdsPInlineS static void InsertAfter(t_CData *_pToInsert, t_CData *_pToInsertAfter)
			{
				DIdsAssert(LinkFromMember(_pToInsertAfter)->IsInList(), "The object has to be in a list to be able to be inserted");
				LinkFromMember(_pToInsert)->Link(LinkFromMember(_pToInsertAfter));
			}
			DIdsPInlineS static void InsertAfter(t_CData &_ToInsert, t_CData *_pToInsertAfter)
			{
				InsertAfter(&_ToInsert, _pToInsertAfter);
			}
			DIdsPInlineS static void InsertAfter(t_CData *_pToInsert, t_CData &_ToInsertAfter)
			{
				InsertAfter(_pToInsert, &_ToInsertAfter);
			}
			DIdsPInlineS static void InsertAfter(t_CData &_ToInsert, t_CData &_ToInsertAfter)
			{
				InsertAfter(&_ToInsert, &_ToInsertAfter);
			}

			DIdsPInlineS void Push(t_CData *_pData)
			{
				InsertHead(_pData);
			}

			/***************************************************************************************************\
            |¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|
            | Unsafe Inserts																					|
            |___________________________________________________________________________________________________|
            \***************************************************************************************************/

			// Insert 
			DIdsPInlineS void UnsafeInsert(t_CData *_pToInsert)
			{				
				LinkFromMember(_pToInsert)->LinkNoUnlink(GetLink()->GetPrev());
			}
			DIdsPInlineS void UnsafeInsert(t_CData &_ToInsert)
			{
				UnsafeInsert(&_ToInsert);
			}

			template <typename t_CSortClass>
			void UnsafeInsertSorted(t_CData &_ToInsert, void *_pContext = DNP)
			{
				UnsafeInsertSorted<t_CSortClass>(&_ToInsert, _pContext);
			}

			template <typename t_CSortClass>
			void UnsafeInsertSorted(t_CData *_pToInsert, void *_pContext = DNP)
			{
				t_CLink *pCurrent = GetLink()->GetNext();

				while (!pCurrent->IsListLink())
				{
					if (t_CSortClass::Compare(_pContext, MemberFromLink(pCurrent), _pToInsert) > 0)
						break;
					pCurrent = pCurrent->GetNext();
				}

				if (pCurrent && !pCurrent->IsListLink())
				{
					t_CLink *pToLink = LinkFromMember(_pToInsert);
					pToLink->LinkNoUnlink(pCurrent);
				}
				else
				{
					t_CLink *pToLink = LinkFromMember(_pToInsert);
					pToLink->LinkNoUnlink(GetLink());
				}
			}


			// InsertTail
			DIdsPInlineS void UnsafeInsertTail(t_CData *_pToInsert)
			{
				UnsafeInsert(_pToInsert);
			}
			DIdsPInlineS void UnsafeInsertTail(t_CData &_ToInsert)
			{
				UnsafeInsertTail(&_ToInsert);
			}

			// InsertHead
			DIdsPInlineS void UnsafeInsertHead(t_CData *_pToInsert)
			{
				LinkFromMember(_pToInsert)->LinkNoUnlink(GetLink());
			}
			DIdsPInlineS void UnsafeInsertHead(t_CData &_ToInsert)
			{
				UnsafeInsertHead(&_ToInsert);
			}

			// InsertHead
			DIdsPInlineS static void UnsafeInsertAfter(t_CData *_pToInsert, t_CData *_pToInsertAfter)
			{
				DIdsAssert(LinkFromMember(_pToInsertAfter)->IsInList(), "The object has to be in a list to be able to be inserted");
				LinkFromMember(_pToInsert)->LinkNoUnlink(LinkFromMember(_pToInsertAfter));
			}
			DIdsPInlineS static void UnsafeInsertAfter(t_CData &_ToInsert, t_CData *_pToInsertAfter)
			{
				UnsafeInsertAfter(&_ToInsert, _pToInsertAfter);
			}
			DIdsPInlineS static void UnsafeInsertAfter(t_CData *_pToInsert, t_CData &_ToInsertAfter)
			{
				UnsafeInsertAfter(_pToInsert, &_ToInsertAfter);
			}
			DIdsPInlineS static void UnsafeInsertAfter(t_CData &_ToInsert, t_CData &_ToInsertAfter)
			{
				UnsafeInsertAfter(&_ToInsert, &_ToInsertAfter);
			}

			DIdsPInlineS void UnsafePush(t_CData *_pData)
			{
				UnsafeInsertHead(_pData);
			}

            /***************************************************************************************************\
            |¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|
            | Removes																							|
            |___________________________________________________________________________________________________|
            \***************************************************************************************************/

			DIdsPInlineS static void Remove(t_CData *_pToRemove)
			{
				LinkFromMember(_pToRemove)->Unlink();
			}

			DIdsPInlineS static void Remove(t_CData &_pToRemove)
			{
				Remove(&_pToRemove);
			}

			DIdsPInlineM t_CData *Pop()
			{
				if (!GetLink()->GetNext()->IsListLink())
				{
					t_CLink *pToRemove = GetLink()->GetNext();

					pToRemove->Unlink();

					return MemberFromLink(pToRemove);
				}
				else
					return DNP;
			}


			void Reverse()
			{
				t_CLink *pCurrent = GetLink()->GetNext();
				while (!pCurrent->IsListLink())
				{
					t_CLink *pTemp = pCurrent;
					pCurrent = pCurrent->GetNext();
					pTemp->Link(GetLink());
				}
			}
			
			//element *listsort(element *list, aint is_circular, aint is_double) {

			typedef aint FMergeCompare(void *_pContext, void *_pFirst, void *_pSecond);
			class CMergeCallbackSort
			{
			public:
				void *m_pContext;
				FMergeCompare *m_pSortFunction;
				DIdsPInlineS static aint Compare(void *_pContext, void *_pFirst, void *_pSecond)
				{					
					return ((CMergeCallbackSort *)_pContext)->m_pSortFunction(((CMergeCallbackSort *)_pContext)->m_pContext, _pFirst, _pSecond);
				}
			};

			void MergeSortCallback(FMergeCompare *_pSortfunction, void *_pContext = DNP, aint _InsertionBits = 3)
			{
				CMergeCallbackSort SortContext;
				SortContext.m_pContext = _pContext;
				SortContext.m_pSortFunction = _pSortfunction;
				MergeSort<CMergeCallbackSort>(&SortContext, _InsertionBits);
			}
				
			/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
			|	Function:			Sorts the linked list								|
			|																			|
			|	Template params:														|
			|		t_CSortClass:	The class that implements the compare funtion.		|
			|						The funtion must be static and be called Compare.	|
			|						It takes parameters									|
			|						(void *_pContext, void *_pFirst, void *_pSecond)	|
			|						and returns less that 0 if _pFirst is less than		|
			|						_pSecond, more than 0 if _pSecond is more than		|
			|						_pFirst, and 0 if the items are equal.				|
			|																			|
			|						  You can inline the compare function with good		|
			|						results, if the compare is small.					|
			|																			|
			|	Parameters:																|
			|		_pContext:		The context that is sent to the compare function	|
			|																			|
			|		_InsertionBits:	The sort uses insertion sort to sort				|
			|						1 << _InsertionBits parts of the list, then uses	|
			|						mergesort to sort the rest.							|
			|																			|
			|						  If you have an expensive sort function you		|
			|						should use 2 for _InsertionBits, wich will mostly	|
			|						use the smallest number of compares for a given		|
			|						sort session. 4 or largen Will be fastest for		|
			|						small compare funcions, but 3 is probably a good	|
			|						compromise that will work good on most compares.	|
			|																			|
			|	Returns:			description											|
			|																			|
			|	Comments:			You can use this method instead of the callback		|
			|						method when you want the compare function to be		|
			|						able to be inlined, at cost of code size.			|
			|																			|
			\*_________________________________________________________________________*/
			template <typename t_CSortClass>
			void MergeSort(void *_pContext = DNP, aint _InsertionBits = 3)
			{
				// We use mergesort as a stable and predictably performing algorithm, needing no extra heapspace or stackspace
				// O = n(log n)

				// Empty list ??
				if (IsEmpty())
					return;

				// Remove reference to last link
//				GetLink()->GetPrev()->SetNext(DNP);

				// Start with insertion sorting a bit first

				aint MergeSize = 1 << (_InsertionBits);

				if (_InsertionBits > 0)
				{
					const aint SortSize = 1 << _InsertionBits;

					t_CLink *pCurrent = GetLink()->GetNext();
					t_CLinkInList LastLink;
					t_CLink *pLastLink = (t_CLink *)&LastLink;
//					LastLink.SetNext(DNP);
//					LastLink.SetPrev(GetLink());
					pLastLink->SetNextInitListLink(pLastLink);
					pLastLink->SetPrevInitListLink(pLastLink);
//					GetLink()->SetNext(pLastLink);
					aint NumSorted = 0;

					while (!pCurrent->IsListLink())
					{
						// Add first member

						++NumSorted;
						t_CLink *pTemp = pCurrent->GetNext();
						pCurrent->LinkNoUnlink(pLastLink->GetPrev());
						pCurrent = pTemp;

						if (pCurrent->IsListLink())
							break;

						for (aint i = 1; i < SortSize; ++i)
						{
							aint NumChecked = i;
							++NumSorted;

							t_CLink *pTemp = pLastLink->GetPrev();

							while (NumChecked)
							{
								if (t_CSortClass::Compare(_pContext,MemberFromLink(pCurrent),MemberFromLink(pTemp)) >= 0)
									break;

								--NumChecked;
								pTemp = pTemp->GetPrev();
							}

							t_CLink *pTempNext = pCurrent->GetNext();
							pCurrent->LinkNoUnlink(pTemp);
							pCurrent = pTempNext;
							if (pCurrent->IsListLink())
								break;
						}
					}

					GetLink()->TransferList(pLastLink->GetNext(), pLastLink->GetPrev());
					if (NumSorted <= MergeSize)
					{						
						// Add last link
						return;
					}

					// Remove last link
//					LastLink.GetPrev()->SetNext(DNP);
				}

				DIdsAssert(_InsertionBits >= 0, "Has to be at least 1");
				

				t_CLink List;
				List.SetNext(GetLink()->GetNext());
				GetLink()->GetPrev()->SetNext(DNP);
				
				while (1) 
				{
					t_CLink *pFirst = List.GetNext();
					t_CLink *pLast = &List;
					
					aint MergesDone = 0;  // count number of merges we do in this pass
					
					while (pFirst) 
					{
						MergesDone++;  // there exists a merge to be done
						// step 'MergeSize' places along from pFirst
						t_CLink *pSecond = pFirst;
						aint MergeSizeFirst = 0;
						aint MergeSizeSecond = MergeSize;
						while (MergeSizeFirst < MergeSize) 
						{
							MergeSizeFirst++;
							pSecond = pSecond->GetNext();
							if (!pSecond)
								break;
						}
						
						// if pSecond hasn't fallen off end, we have two lists to merge

						if (!pSecond)
						{
							// End of list
							pLast->SetNext(pFirst);
							pFirst->SetPrev(pLast);
							pLast = DNP;
//							pLast = pFirst;
//							pFirst = pFirst->m_pNext;
//							MergeSizeFirst--;
						}
						else
						{			
							if (t_CSortClass::Compare(_pContext,MemberFromLink(pSecond->GetPrev()),MemberFromLink(pSecond)) <= 0) 
							{
								// Lists already sorted
								pLast->SetNext(pFirst);
								pFirst->SetPrev(pLast);
								while (MergeSizeSecond * (mint)pSecond)
								{
									--MergeSizeSecond;
									pLast = pSecond;
									pSecond = pSecond->GetNext();
								}
							}
							else
							{
								
								DIdsAssert(MergeSizeSecond && MergeSizeFirst, "Hula");
								// now we have two lists; merge them 
								while (1) 
								{
									// decide whether m_pNext element of merge comes from pFirst or pSecond 
									if (t_CSortClass::Compare(_pContext,MemberFromLink(pFirst),MemberFromLink(pSecond)) <= 0) 
									{
										// First element of pFirst is lower (or same); pTemp must come from pFirst.
										pLast->SetNext(pFirst);
										pFirst->SetPrev(pLast);
										pLast = pFirst;
										pFirst = pFirst->GetNext();
										MergeSizeFirst--;
										if (!MergeSizeFirst)
										{
											pLast->SetNext(pSecond);
											pSecond->SetPrev(pLast);

											while (MergeSizeSecond * (mint)pSecond)
											{
												--MergeSizeSecond;
												pLast = pSecond;
												pSecond = pSecond->GetNext();
											}
											break;
										}
									} 
									else 
									{
										// First element of pSecond is lower; pTemp must come from pSecond.
										pLast->SetNext(pSecond);
										pSecond->SetPrev(pLast);
										pLast = pSecond;
										pSecond = pSecond->GetNext();
										MergeSizeSecond--;
										if (!pSecond || !MergeSizeSecond)
										{
											pLast->SetNext(pFirst);
											pFirst->SetPrev(pLast);
											
											while (MergeSizeFirst) 
											{
												MergeSizeFirst--;
												pLast = pFirst;
												pFirst = pFirst->GetNext();
											}
											break;
										}
									}
								}
							}
						}
						
						// now pFirst has stepped 'MergeSize' places along, and pSecond has too
						pFirst = pSecond;
					}
					
					
					if (pLast)
						pLast->SetNext(DNP);
					
					// If we have done only one merge, we're finished.
					if (MergesDone <= 1)   // allow for MergesDone==0, the empty list case
					{
						GetLink()->TransferList(List.GetNext(), pLast);
//						pLast->SetNext(GetLink());
//						GetLink()->SetPrev(pLast);
						return;
					}
					
					// Otherwise repeat, merging lists twice the size
					MergeSize <<= 1;
				}
			}

			typedef aint FBucketGetIndex(void *_pContext, aint _Place, void *_pItem);
			class CBucketCallbackSort
			{
			public:
				void *m_pContext;
				FBucketGetIndex *m_pSortFunction;
				DIdsPInlineS static aint GetIndex(void *_pContext, aint _Place, void *_pItem)
				{					
					return ((CBucketCallbackSort *)_pContext)->m_pSortFunction(((CBucketCallbackSort *)_pContext)->m_pContext, _pItem);
				}
			};

			template <aint t_BucketSize>
			void BucketSortCallback(FBucketGetIndex *_pSortfunction, void *_pContext = DNP)
			{
				CBucketCallbackSort SortContext;
				SortContext.m_pContext = _pContext;
				SortContext.m_pSortFunction = _pSortfunction;
				BucketSort<CBucketCallbackSort, t_BucketSize>(&SortContext);
			}


			/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
			|	Function:			Sorts the linked list								|
			|																			|
			|	Template params:														|
			|		t_CSortClass:	The class that implements the index funtion.		|
			|						The funtion must be static and be called GetIndex.	|
			|						It takes parameters									|
			|						(void *_pContext, aint _Place, void *_pItem)		|
			|						and returns the least significant index for _pItem	|
			|																			|
			|						  You can inline the compare function with good		|
			|						results, if the compare is small.					|
			|																			|
			|	Parameters:																|
			|		_pContext:		The context that is sent to the compare function	|
			|																			|
			|		_InsertionBits:	The sort uses insertion sort to sort				|
			|						1 << _InsertionBits parts of the list, then uses	|
			|						mergesort to sort the rest.							|
			|																			|
			|						  If you have an expensive sort function you		|
			|						should use 2 for _InsertionBits, wich will mostly	|
			|						use the smallest number of compares for a given		|
			|						sort session. 4 or largen Will be fastest for		|
			|						small compare funcions, but 3 is probably a good	|
			|						compromise that will work good on most compares.	|
			|																			|
			|	Returns:			description											|
			|																			|
			|	Comments:			You can use this method instead of the callback		|
			|						method when you want the compare function to be		|
			|						able to be inlined, at cost of code size.			|
			|																			|
			\*_________________________________________________________________________*/

			template <typename t_CSortClass, aint t_BucketSize>
			void BucketSort(aint _NumPlaces, void *_pContext = DNP)
			{
				if (IsEmpty())
					return;

				t_CLink *Bucket[t_BucketSize];

				// Reset buckets
				for (aint i = 0; i < t_BucketSize; ++i)
				{
					Bucket[i] = DNP;
				}

				// Put list in first bucket

				for(aint Place = 0; Place < _NumPlaces; ++Place)
				{					
					// Remove last link
					GetLink()->GetPrev()->SetNext(DNP);

					t_CLink *pCurrent = GetLink()->GetNext();

					while (pCurrent)
					{
						aint Index = t_CSortClass::GetIndex(_pContext, Place, MemberFromLink(pCurrent));
						t_CLink **pBucket = &Bucket[Index];

						t_CLink *pNext = pCurrent->GetNext();

						if (*pBucket)
							(*pBucket)->SetPrev(pCurrent);
						pCurrent->SetNext(*pBucket);
						(*pBucket) = pCurrent;

						pCurrent = pNext;
					}

					
					// Rebuild list

					GetLink()->SetNext(GetLink());
					GetLink()->SetPrev(GetLink());
					
					for (aint i = 0; i < t_BucketSize; ++i)
					{
						if (Bucket[i])
						{
							t_CLink *pTemp = Bucket[i];
							Bucket[i] = DNP;
							t_CLink *pLinkAfter = GetLink()->GetPrev();

							while (pTemp)
							{
								t_CLink *pTempNext = pTemp->GetNext();
								pTemp->LinkNoUnlink(pLinkAfter);
								pTemp = pTempNext;
							}
						}
					}
				}

			}

			bint CheckList(bint _bBreak)
			{
				t_CLink *pCurrent = GetLink();
				t_CLink *pPrev = pCurrent;
				pCurrent = pCurrent->GetNext();
				while (!pCurrent->IsListLink())
				{
					if (pCurrent->GetPrev() != pPrev)
					{
						if (_bBreak)
							M_BREAKPOINT;
						return false;
					}
					pPrev = pCurrent;
					pCurrent = pCurrent->GetNext();
				}
				pCurrent = GetLink();
				pPrev = pCurrent;
				pCurrent = pCurrent->GetPrev();
				while (!pCurrent->IsListLink())
				{
					if (pCurrent->GetNext() != pPrev)
					{
						if (_bBreak)
							M_BREAKPOINT;
						return false;
					}
					pPrev = pCurrent;
					pCurrent = pCurrent->GetPrev();
				}
				return true;
			}
			
			class CIterator
			{
			public:
				t_CLink *m_pCurrent;
#ifdef DIdsDebug
				t_CData *m_pDebugCurrent;
#endif

				CIterator()
				{
					m_pCurrent = DNP;
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
				}

				CIterator(TCDLinkListAggregate &_List)
				{
					m_pCurrent = _List.GetLink()->GetNext();
					if (m_pCurrent->IsListLink())
						m_pCurrent = DNP;
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
				}

				CIterator(const CIterator& _Copy)
				{
					m_pCurrent = _Copy.m_pCurrent;
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
				}

				CIterator(t_CData *_pStart)
				{
					m_pCurrent = TCDLinkListAggregate::LinkFromMember(_pStart);
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
				}

				CIterator(t_CData &_Start)
				{
					m_pCurrent = TCDLinkListAggregate::LinkFromMember(&_Start);
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
				}

				CIterator& operator = (TCDLinkListAggregate &_List)
				{
					m_pCurrent = _List.GetLink()->GetNext();
					if (m_pCurrent->IsListLink())
						m_pCurrent = DNP;
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif

					return *this;
				}

				CIterator& operator = (const CIterator& _Copy)
				{
					m_pCurrent = _Copy.m_pCurrent;
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
					return *this;
				}

				CIterator& operator = (t_CData &_Start)
				{
					m_pCurrent = TCDLinkListAggregate::LinkFromMember(&_Start);

#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
					return *this;
				}
				
				CIterator& operator = (t_CData *_pStart)
				{
					m_pCurrent = TCDLinkListAggregate::LinkFromMember(_pStart);

#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
					return *this;
				}

				DIdsPInlineS static t_CData *GetNext(t_CData *_pCurrent)
				{					
					t_CLink *pNext = TCDLinkListAggregate::LinkFromMember(_pCurrent)->GetNext();
					if (pNext)
						return TCDLinkListAggregate::MemberFromLink(pNext);
					else
						return DNP;
				}
				
				DIdsPInlineS static t_CData *GetPrev(t_CData *_pCurrent)
				{					
					t_CLink *pPrev = TCDLinkListAggregate::LinkFromMember(_pCurrent)->GetPrev();
					if (pPrev)
						return TCDLinkListAggregate::MemberFromLink(pPrev);
					else
						return DNP;
				}
				
				void Reverse(TCDLinkListAggregate &_List)
				{
					m_pCurrent = _List.GetLink()->GetPrev();
					if (m_pCurrent->IsListLink())
						m_pCurrent = DNP;
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
				}

				DIdsPInlineS t_CData *GetCurrent() const
				{
					if (m_pCurrent)
						return TCDLinkListAggregate::MemberFromLink(m_pCurrent);
					else
						return DNP;
				}

				DIdsPInlineS operator t_CData *() const
				{
					return GetCurrent();
				}

				DIdsPInlineS t_CData * operator ->() const
				{
					return GetCurrent();
				}

				DIdsPInlineS t_CData * operator [](aint _nPlaces) const
				{
					if (m_pCurrent)
					{
						t_CLink *pCurrent = m_pCurrent;
						if (_nPlaces > 0)
						{
							while (_nPlaces)
							{
								pCurrent = pCurrent->GetNext();
								if (pCurrent->IsListLink())
								{
									DIdsAssert(0, "You tried to access an element outside the list");
									return DNP;
								}
								--_nPlaces;
							}
						}
						else 
						{
							while (_nPlaces)
							{
								pCurrent = pCurrent->GetPrev();
								if (pCurrent->IsListLink())
								{
									DIdsAssert(0, "You tried to access an element outside the list");
									return DNP;
								}
								++_nPlaces;
							}
						}
						return TCDLinkListAggregate::MemberFromLink(pCurrent);
					}
					DIdsAssert(0, "You tried to access an element outside the list");
					return DNP;
				}

				DIdsPInlineS void operator +=(aint _nPlaces)
				{
					if (m_pCurrent)
					{
						while (_nPlaces)
						{
							m_pCurrent = m_pCurrent->GetNext();
							if (m_pCurrent->IsListLink())
							{
								m_pCurrent = DNP;
								break;
							}
							--_nPlaces;
						}
					}
					DIdsAssert(!_nPlaces, "You tried to access an element outside the list");
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
				}
				DIdsPInlineS void operator ++()
				{
					if (m_pCurrent)
					{
						m_pCurrent = m_pCurrent->GetNext();
						if (m_pCurrent->IsListLink())
							m_pCurrent = DNP;
					}
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
				}

				DIdsPInlineS void operator --()
				{
					if (m_pCurrent)
					{
						m_pCurrent = m_pCurrent->GetPrev();
						if (m_pCurrent->IsListLink())
							m_pCurrent = DNP;
					}
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif

				}

				DIdsPInlineS void operator -=(aint _nPlaces)
				{
					if (m_pCurrent)
					{
						while (_nPlaces)
						{
							m_pCurrent = m_pCurrent->GetPrev();
							if (m_pCurrent->IsListLink())
								m_pCurrent = DNP;
							--_nPlaces;
						}
					}
					DIdsAssert(!_nPlaces, "You tried to access an element outside the list");
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
				}

				void Delete()
				{
					t_CData *pCurrent = GetCurrent();
					++(*this);

					pCurrent->~t_CData();
					t_CAllocator::Free(pCurrent);
				}

				void Remove()
				{
					t_CLink *pLink = m_pCurrent;
					++(*this);
					pLink->Unlink();
				}
			};

            CIterator GetIter()
			{
				return CIterator(*this);
			}

			class CIteratorConst
			{
			public:
				const t_CLink *m_pCurrent;
#ifdef DIdsDebug
				const t_CData *m_pDebugCurrent;
#endif
				

				CIteratorConst()
				{
					m_pCurrent = DNP;
#ifdef DIdsDebug
					m_pDebugCurrent = DNP;
#endif
				}

				CIteratorConst(const CIteratorConst& _Copy)
				{
					m_pCurrent = _Copy.m_pCurrent;
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
				}

				CIteratorConst(const TCDLinkListAggregate &_List)
				{
					m_pCurrent = _List.GetLink()->GetNext();
					if (m_pCurrent->IsListLink())
						m_pCurrent = DNP;
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
				}

				CIteratorConst(const t_CData *_pStart)
				{
					m_pCurrent = TCDLinkListAggregate::LinkFromMemberConst(_pStart);
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
				}

				CIteratorConst(const t_CData &_Start)
				{
					m_pCurrent = TCDLinkListAggregate::LinkFromMemberConst(&_Start);
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
				}

				CIteratorConst& operator = (const TCDLinkListAggregate &_List)
				{
					m_pCurrent = _List.GetLink()->GetNext();
					if (m_pCurrent->IsListLink())
						m_pCurrent = DNP;
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
					return *this;
				}

				CIteratorConst& operator = (const CIteratorConst& _Copy)
				{
					m_pCurrent = _Copy.m_pCurrent;
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
					return *this;
				}

				CIteratorConst& operator = (const t_CData &_Start)
				{
					m_pCurrent = TCDLinkListAggregate::LinkFromMemberConst(&_Start);
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif

					return *this;
				}
				
				CIteratorConst& operator = (const t_CData *_pStart)
				{
					m_pCurrent = TCDLinkListAggregate::LinkFromMemberConst(_pStart);
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif

					return *this;
				}
				
				void Reverse(const TCDLinkListAggregate &_List)
				{
					m_pCurrent = _List.GetLink()->GetPrev();
					if (m_pCurrent->IsListLink())
						m_pCurrent = DNP;
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
				}

				DIdsPInlineS const t_CData *GetCurrent() const
				{
					if (m_pCurrent)
						return TCDLinkListAggregate::MemberFromLinkConst(m_pCurrent);
					else
						return DNP;
				}

				DIdsPInlineS operator const t_CData *() const
				{
					return GetCurrent();
				}

				DIdsPInlineS const t_CData * operator ->() const
				{
					return GetCurrent();
				}

				DIdsPInlineS const t_CData * operator [](aint _nPlaces) const
				{
					if (m_pCurrent)
					{
						const t_CLink *pCurrent = m_pCurrent;
						if (_nPlaces > 0)
						{
							while (_nPlaces)
							{
								pCurrent = pCurrent->GetNext();
								if (pCurrent->IsListLink())
								{
									return DNP;
								}
								--_nPlaces;
							}
						}
						else 
						{
							while (_nPlaces)
							{
								pCurrent = pCurrent->GetPrev();
								if (pCurrent->IsListLink())
								{
									return DNP;
								}
								++_nPlaces;
							}
						}
						return TCDLinkListAggregate::MemberFromLinkConst(pCurrent);
					}
					return DNP;
				}

				DIdsPInlineS void operator +=(aint _nPlaces)
				{
					if (m_pCurrent)
					{
						while (_nPlaces)
						{
							m_pCurrent = m_pCurrent->GetNext();
							if (m_pCurrent->IsListLink())
							{
								m_pCurrent = DNP;
								break;
							}
							--_nPlaces;
						}
					}
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
					DIdsAssert(!_nPlaces, "You tried to access an element outside the list");
				}
				DIdsPInlineS void operator ++()
				{
					if (m_pCurrent)
					{
						m_pCurrent = m_pCurrent->GetNext();
						if (m_pCurrent->IsListLink())
							m_pCurrent = DNP;
					}
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
				}

				DIdsPInlineS void operator --()
				{
					if (m_pCurrent)
					{
						m_pCurrent = m_pCurrent->GetPrev();
						if (m_pCurrent->IsListLink())
							m_pCurrent = DNP;
					}
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
				}

				DIdsPInlineS void operator -=(aint _nPlaces)
				{
					if (m_pCurrent)
					{
						while (_nPlaces)
						{
							m_pCurrent = m_pCurrent->GetPrev();
							if (m_pCurrent->IsListLink())
								m_pCurrent = DNP;
							--_nPlaces;
						}
					}
					DIdsAssert(!_nPlaces, "You tried to access an element outside the list");
#ifdef DIdsDebug
					m_pDebugCurrent = GetCurrent();
#endif
				}
			};

		};

		template <typename t_CData, typename t_CTranslator, typename t_CLink, typename t_CLinkInList, bint t_bAutoDelete, typename t_CAllocator>
		class TCDLinkList : public TCDLinkListAggregate<t_CData, t_CTranslator, t_CLink, t_CLinkInList, t_bAutoDelete, t_CAllocator>
		{			
		public:
			DIdsPInlineS TCDLinkList()
			{
				TCDLinkListAggregate<t_CData, t_CTranslator, t_CLink, t_CLinkInList, t_bAutoDelete, t_CAllocator>::Construct();
			}

			DIdsPInlineS ~TCDLinkList()
			{
				TCDLinkListAggregate<t_CData, t_CTranslator, t_CLink, t_CLinkInList, t_bAutoDelete, t_CAllocator>::Destruct();
			}
		};



#		define DIdsListLinkD_Trans(_Class, _Member) \
				class CDLinkTranslator##_Member \
				{\
				public:\
					DIdsPInlineS static aint GetOffset()\
					{\
						const _Class *pPtr = 0;\
						return (aint)(void *)(&((pPtr)->_Member));\
					}\
				};

//						(((uint8*)(&(pClass->_Member)) - (uint8*)(pClass)));
//					      (((uint8*)(&((_Class *)0x0)->_Member) - (uint8*)((_Class *)0x0)));
				
		/***************************************************************************************************\
		|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|
		| Fast																								|
		|___________________________________________________________________________________________________|
		\***************************************************************************************************/

#		define DIdsListLinkAllocatorD_LinkType(_Allocator) NIds::NList::TDLink< NIds::NList::TCDLinkAggregate<_Allocator> >
#		define DIdsListLinkAllocatorD_Member(_Member, _Allocator) DIdsListLinkAllocatorD_LinkType(_Allocator) _Member;
	
#		define DIdsListLinkAllocatorDA_LinkType(_Allocator) NIds::NList::TCDLinkAggregate<_Allocator>
#		define DIdsListLinkAllocatorDA_Member(_Member, _Allocator) DIdsListLinkAllocatorDA_LinkType _Member;

#		define DIdsListLinkAllocatorD_Link(_Class, _Member, _Allocator) \
				DIdsListLinkD_Trans(_Class, _Member) \
				DIdsListLinkAllocatorD_Member(_Member, _Allocator)

#		define DIdsListLinkAllocatorDA_Link(_Class, _Member, _Allocator) \
				DIdsListLinkD_Trans(_Class, _Member) \
				DIdsListLinkAllocatorDA_Member(_Member, _Allocator)

#		define DIdsListLinkAllocatorD_List(_Class, _Member, _Allocator) NIds::NList::TCDLinkList<_Class, _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregate<_Allocator>, NIds::NList::TCDLinkAggregate<_Allocator>, false, _Allocator>
#		define DIdsListLinkAllocatorD_ListAutoDelete(_Class, _Member, _Allocator) NIds::NList::TCDLinkList<_Class, _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregate<_Allocator>, NIds::NList::TCDLinkAggregate<_Allocator>, true, _Allocator>
#		define DIdsListLinkAllocatorD_List_FromTemplate(_Class, _Member, _Allocator) NIds::NList::TCDLinkList<_Class, typename _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregate<_Allocator>, NIds::NList::TCDLinkAggregate<_Allocator>, false, _Allocator>
#		define DIdsListLinkAllocatorD_ListAutoDelete_FromTemplate(_Class, _Member, _Allocator) NIds::NList::TCDLinkList<_Class, typename _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregate<_Allocator>, NIds::NList::TCDLinkAggregate<_Allocator>, true, _Allocator>

#		define DIdsListLinkAllocatorDA_List(_Class, _Member, _Allocator) NIds::NList::TCDLinkListAggregate<_Class, _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregate<_Allocator>, NIds::NList::TCDLinkAggregate<_Allocator>, false, _Allocator>
#		define DIdsListLinkAllocatorDA_ListAutoDelete(_Class, _Member, _Allocator) NIds::NList::TCDLinkListAggregate<_Class, _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregate<_Allocator>, NIds::NList::TCDLinkAggregate<_Allocator>, true, _Allocator>
#		define DIdsListLinkAllocatorDA_List_FromTemplate(_Class, _Member, _Allocator) NIds::NList::TCDLinkListAggregate<_Class, typename _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregate<_Allocator>, NIds::NList::TCDLinkAggregate<_Allocator>, false, _Allocator>
#		define DIdsListLinkAllocatorDA_ListAutoDelete_FromTemplate(_Class, _Member, _Allocator) NIds::NList::TCDLinkListAggregate<_Class, typename _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregate<_Allocator>, NIds::NList::TCDLinkAggregate<_Allocator>, true, _Allocator>

#		define DIdsListLinkAllocatorD_Iter(_Class, _Member, _Allocator) DIdsListLinkAllocatorDA_List(_Class, _Member, _Allocator)::CIterator
#		define DIdsListLinkAllocatorD_IterConst(_Class, _Member, _Allocator) DIdsListLinkAllocatorDA_List(_Class, _Member, _Allocator)::CIteratorConst
#		define DIdsListLinkAllocatorD_Iter_FromTemplate(_Class, _Member, _Allocator) typename DIdsListLinkAllocatorDA_List_FromTemplate(_Class, _Member, _Allocator)::CIterator
#		define DIdsListLinkAllocatorD_IterConst_FromTemplate(_Class, _Member, _Allocator) typename DIdsListLinkAllocatorDA_List_FromTemplate(_Class, _Member, _Allocator)::CIteratorConst
					
#		ifndef DIdsPNoShortCuts
#			define DLinkAllocatorD_Link(_Class, _Member, _Allocator) DIdsListLinkAllocatorD_Link(_Class, _Member, _Allocator)
#			define DLinkAllocatorD_Iter(_Class, _Member, _Allocator) DIdsListLinkAllocatorD_Iter(_Class, _Member, _Allocator)
#			define DLinkAllocatorD_IterConst(_Class, _Member, _Allocator) DIdsListLinkAllocatorD_IterConst(_Class, _Member, _Allocator)
#			define DLinkAllocatorD_List(_Class, _Member, _Allocator) DIdsListLinkAllocatorD_List(_Class, _Member, _Allocator)
#			define DLinkAllocatorD_ListAutoDelete(_Class, _Member, _Allocator) DIdsListLinkAllocatorD_ListAutoDelete(_Class, _Member, _Allocator)
#			define DLinkAllocatorDA_Link(_Class, _Member, _Allocator) DIdsListLinkAllocatorDA_Link(_Class, _Member, _Allocator)
#			define DLinkAllocatorDA_List(_Class, _Member, _Allocator) DIdsListLinkAllocatorDA_List(_Class, _Member, _Allocator)
#			define DLinkAllocatorDA_ListAutoDelete(_Class, _Member, _Allocator) DIdsListLinkAllocatorDA_ListAutoDelete(_Class, _Member, _Allocator)
#		endif

        /***************************************************************************************************\
        |¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|
        | List Takes Less Space, but slower																	|
        |___________________________________________________________________________________________________|
        \***************************************************************************************************/

#		define DIdsListLinkAllocatorDS_LinkType(_Allocator) NIds::NList::TDLink< NIds::NList::TCDLinkAggregateListNoPrevPtr<_Allocator> >
#		define DIdsListLinkAllocatorDS_Member(_Member, _Allocator) DIdsListLinkAllocatorDS_LinkType(_Allocator) _Member;
	
#		define DIdsListLinkAllocatorDSA_LinkType(_Allocator) NIds::NList::TCDLinkAggregateListNoPrevPtr<_Allocator>
#		define DIdsListLinkAllocatorDSA_Member(_Member, _Allocator) DIdsListLinkAllocatorDSA_LinkType(_Allocator) _Member;

#		define DIdsListLinkAllocatorDS_Link(_Class, _Member, _Allocator) \
				DIdsListLinkD_Trans(_Class, _Member) \
				DIdsListLinkAllocatorDS_Member(_Member, _Allocator)

#		define DIdsListLinkAllocatorDSA_Link(_Class, _Member, _Allocator) \
				DIdsListLinkD_Trans(_Class, _Member) \
				DIdsListLinkAllocatorDSA_Member(_Member, _Allocator)

#		define DIdsListLinkAllocatorDS_List(_Class, _Member, _Allocator) NIds::NList::TCDLinkList<_Class, _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregateListNoPrevPtr<_Allocator>, NIds::NList::TCDLinkAggregateListNoPrevPtrList<_Allocator>, false, _Allocator>
#		define DIdsListLinkAllocatorDS_ListAutoDelete(_Class, _Member, _Allocator) NIds::NList::TCDLinkList<_Class, _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregateListNoPrevPtr<_Allocator>, NIds::NList::TCDLinkAggregateListNoPrevPtrList<_Allocator>, true, _Allocator>
#		define DIdsListLinkAllocatorDS_List_FromTemplate(_Class, _Member, _Allocator) NIds::NList::TCDLinkList<_Class, typename _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregateListNoPrevPtr<_Allocator>, NIds::NList::TCDLinkAggregateListNoPrevPtrList<_Allocator>, false, _Allocator>
#		define DIdsListLinkAllocatorDS_ListAutoDelete_FromTemplate(_Class, _Member, _Allocator) NIds::NList::TCDLinkList<_Class, typename _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregateListNoPrevPtr<_Allocator>, NIds::NList::TCDLinkAggregateListNoPrevPtrList<_Allocator>, true, _Allocator>

#		define DIdsListLinkAllocatorDSA_List(_Class, _Member, _Allocator) NIds::NList::TCDLinkListAggregate<_Class, _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregateListNoPrevPtr<_Allocator>, NIds::NList::TCDLinkAggregateListNoPrevPtrList<_Allocator>, false, _Allocator>
#		define DIdsListLinkAllocatorDSA_ListAutoDelete(_Class, _Member, _Allocator) NIds::NList::TCDLinkListAggregate<_Class, _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregateListNoPrevPtr<_Allocator>, NIds::NList::TCDLinkAggregateListNoPrevPtrList<_Allocator>, true, _Allocator>
#		define DIdsListLinkAllocatorDSA_List_FromTemplate(_Class, _Member, _Allocator) NIds::NList::TCDLinkListAggregate<_Class, typename _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregateListNoPrevPtr<_Allocator>, NIds::NList::TCDLinkAggregateListNoPrevPtrList<_Allocator>, false, _Allocator>
#		define DIdsListLinkAllocatorDSA_ListAutoDelete_FromTemplate(_Class, _Member, _Allocator) NIds::NList::TCDLinkListAggregate<_Class, typename _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregateListNoPrevPtr<_Allocator>, NIds::NList::TCDLinkAggregateListNoPrevPtrList<_Allocator>, true, _Allocator>

#		define DIdsListLinkAllocatorDS_Iter(_Class, _Member, _Allocator) DIdsListLinkAllocatorDSA_List(_Class, _Member, _Allocator)::CIterator
#		define DIdsListLinkAllocatorDS_IterConst(_Class, _Member, _Allocator) DIdsListLinkAllocatorDSA_List(_Class, _Member, _Allocator)::CIteratorConst
#		define DIdsListLinkAllocatorDS_Iter_FromTemplate(_Class, _Member, _Allocator) DIdsListLinkAllocatorDSA_List_FromTemplate(_Class, _Member, _Allocator)::CIterator
#		define DIdsListLinkAllocatorDS_IterConst_FromTemplate(_Class, _Member, _Allocator) DIdsListLinkAllocatorDSA_List_FromTemplate(_Class, _Member, _Allocator)::CIteratorConst

#		ifndef DIdsPNoShortCuts
#			define DLinkAllocatorDS_Link(_Class, _Member, _Allocator) DIdsListLinkAllocatorDS_Link(_Class, _Member, _Allocator)
#			define DLinkAllocatorDS_Iter(_Class, _Member, _Allocator) DIdsListLinkAllocatorDS_Iter(_Class, _Member, _Allocator)
#			define DLinkAllocatorDS_IterConst(_Class, _Member, _Allocator) DIdsListLinkAllocatorDS_IterConst(_Class, _Member, _Allocator)
#			define DLinkAllocatorDS_List(_Class, _Member, _Allocator) DIdsListLinkAllocatorDS_List(_Class, _Member, _Allocator)
#			define DLinkAllocatorDS_ListAutoDelete(_Class, _Member, _Allocator) DIdsListLinkAllocatorDS_ListAutoDelete(_Class, _Member, _Allocator)
#			define DLinkAllocatorDSA_Link(_Class, _Member, _Allocator) DIdsListLinkAllocatorDSA_Link(_Class, _Member, _Allocator)
#			define DLinkAllocatorDSA_List(_Class, _Member, _Allocator) DIdsListLinkAllocatorDSA_List(_Class, _Member, _Allocator)
#			define DLinkAllocatorDSA_ListAutoDelete(_Class, _Member, _Allocator) DIdsListLinkAllocatorDSA_ListAutoDelete(_Class, _Member, _Allocator)
#		endif


		/***************************************************************************************************\
		|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|
		| Fast																								|
		|___________________________________________________________________________________________________|
		\***************************************************************************************************/

#		define DIdsListLinkD_LinkType NIds::NList::TDLink< NIds::NList::TCDLinkAggregate< DIdsDefaultAllocator > >
#		define DIdsListLinkD_Member(_Member) DIdsListLinkD_LinkType _Member;
	
#		define DIdsListLinkDA_LinkType NIds::NList::TCDLinkAggregate< DIdsDefaultAllocator >
#		define DIdsListLinkDA_Member(_Member) DIdsListLinkDA_LinkType _Member;

#		define DIdsListLinkD_Link(_Class, _Member) \
				DIdsListLinkD_Trans(_Class, _Member) \
				DIdsListLinkD_Member(_Member)

#		define DIdsListLinkDA_Link(_Class, _Member) \
				DIdsListLinkD_Trans(_Class, _Member) \
				DIdsListLinkDA_Member(_Member)

#		define DIdsListLinkD_List(_Class, _Member) NIds::NList::TCDLinkList<_Class, _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregate< DIdsDefaultAllocator >, NIds::NList::TCDLinkAggregate< DIdsDefaultAllocator >, false, DIdsDefaultAllocator >
#		define DIdsListLinkD_ListAutoDelete(_Class, _Member) NIds::NList::TCDLinkList<_Class, _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregate< DIdsDefaultAllocator >, NIds::NList::TCDLinkAggregate< DIdsDefaultAllocator >, true, DIdsDefaultAllocator >
#		define DIdsListLinkD_List_FromTemplate(_Class, _Member) NIds::NList::TCDLinkList<_Class, typename _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregate< DIdsDefaultAllocator >, NIds::NList::TCDLinkAggregate< DIdsDefaultAllocator >, false, DIdsDefaultAllocator >
#		define DIdsListLinkD_ListAutoDelete_FromTemplate(_Class, _Member) NIds::NList::TCDLinkList<_Class, typename _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregate< DIdsDefaultAllocator >, NIds::NList::TCDLinkAggregate< DIdsDefaultAllocator >, true, DIdsDefaultAllocator >

#		define DIdsListLinkDA_List(_Class, _Member) NIds::NList::TCDLinkListAggregate<_Class, _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregate< DIdsDefaultAllocator >, NIds::NList::TCDLinkAggregate< DIdsDefaultAllocator >, false, DIdsDefaultAllocator >
#		define DIdsListLinkDA_ListAutoDelete(_Class, _Member) NIds::NList::TCDLinkListAggregate<_Class, _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregate< DIdsDefaultAllocator >, NIds::NList::TCDLinkAggregate< DIdsDefaultAllocator >, true, DIdsDefaultAllocator >
#		define DIdsListLinkDA_List_FromTemplate(_Class, _Member) NIds::NList::TCDLinkListAggregate<_Class, typename _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregate< DIdsDefaultAllocator >, NIds::NList::TCDLinkAggregate< DIdsDefaultAllocator >, false, DIdsDefaultAllocator >
#		define DIdsListLinkDA_ListAutoDelete_FromTemplate(_Class, _Member) NIds::NList::TCDLinkListAggregate<_Class, typename _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregate< DIdsDefaultAllocator >, NIds::NList::TCDLinkAggregate< DIdsDefaultAllocator >, true, DIdsDefaultAllocator >

#		define DIdsListLinkD_Iter(_Class, _Member) DIdsListLinkDA_List(_Class, _Member)::CIterator
#		define DIdsListLinkD_IterConst(_Class, _Member) DIdsListLinkDA_List(_Class, _Member)::CIteratorConst
#		define DIdsListLinkD_Iter_FromTemplate(_Class, _Member) typename DIdsListLinkDA_List_FromTemplate(_Class, _Member)::CIterator
#		define DIdsListLinkD_IterConst_FromTemplate(_Class, _Member) typename DIdsListLinkDA_List_FromTemplate(_Class, _Member)::CIteratorConst

					
#		ifndef DIdsPNoShortCuts
#			define DLinkD_Trans(_Class, _Member) DIdsListLinkD_Trans(_Class, _Member)
#			define DLinkD_Link(_Class, _Member) DIdsListLinkD_Link(_Class, _Member)
#			define DLinkD_Iter(_Class, _Member) DIdsListLinkD_Iter(_Class, _Member)
#			define DLinkD_IterConst(_Class, _Member) DIdsListLinkD_IterConst(_Class, _Member)
#			define DLinkD_List(_Class, _Member) DIdsListLinkD_List(_Class, _Member)
#			define DLinkD_ListAutoDelete(_Class, _Member) DIdsListLinkD_ListAutoDelete(_Class, _Member)
#			define DLinkDA_Link(_Class, _Member) DIdsListLinkDA_Link(_Class, _Member)
#			define DLinkDA_List(_Class, _Member) DIdsListLinkDA_List(_Class, _Member)
#			define DLinkDA_ListAutoDelete(_Class, _Member) DIdsListLinkDA_ListAutoDelete(_Class, _Member)
#		endif

        /***************************************************************************************************\
        |¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|
        | List Takes Less Space, but slower																	|
        |___________________________________________________________________________________________________|
        \***************************************************************************************************/

#		define DIdsListLinkDS_LinkType NIds::NList::TDLink< NIds::NList::TCDLinkAggregateListNoPrevPtr< DIdsDefaultAllocator > >
#		define DIdsListLinkDS_Member(_Member) DIdsListLinkDS_LinkType _Member;
	
#		define DIdsListLinkDSA_LinkType NIds::NList::TCDLinkAggregateListNoPrevPtr< DIdsDefaultAllocator >
#		define DIdsListLinkDSA_Member(_Member) DIdsListLinkDSA_LinkType _Member;

#		define DIdsListLinkDS_Link(_Class, _Member) \
				DIdsListLinkD_Trans(_Class, _Member) \
				DIdsListLinkDS_Member(_Member)

#		define DIdsListLinkDSA_Link(_Class, _Member) \
				DIdsListLinkD_Trans(_Class, _Member) \
				DIdsListLinkDSA_Member(_Member)

#		define DIdsListLinkDS_List(_Class, _Member) NIds::NList::TCDLinkList<_Class, _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregateListNoPrevPtr< DIdsDefaultAllocator >, NIds::NList::TCDLinkAggregateListNoPrevPtrList< DIdsDefaultAllocator >, false, DIdsDefaultAllocator >
#		define DIdsListLinkDS_ListAutoDelete(_Class, _Member) NIds::NList::TCDLinkList<_Class, _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregateListNoPrevPtr< DIdsDefaultAllocator >, NIds::NList::TCDLinkAggregateListNoPrevPtrList< DIdsDefaultAllocator >, true, DIdsDefaultAllocator >
#		define DIdsListLinkDS_List_FromTemplate(_Class, _Member) NIds::NList::TCDLinkList<_Class, typename _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregateListNoPrevPtr< DIdsDefaultAllocator >, NIds::NList::TCDLinkAggregateListNoPrevPtrList< DIdsDefaultAllocator >, false, DIdsDefaultAllocator >
#		define DIdsListLinkDS_ListAutoDelete_FromTemplate(_Class, _Member) NIds::NList::TCDLinkList<_Class, typename _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregateListNoPrevPtr< DIdsDefaultAllocator >, NIds::NList::TCDLinkAggregateListNoPrevPtrList< DIdsDefaultAllocator >, true, DIdsDefaultAllocator >

#		define DIdsListLinkDSA_List(_Class, _Member) NIds::NList::TCDLinkListAggregate<_Class, _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregateListNoPrevPtr< DIdsDefaultAllocator >, NIds::NList::TCDLinkAggregateListNoPrevPtrList< DIdsDefaultAllocator >, false, DIdsDefaultAllocator >
#		define DIdsListLinkDSA_ListAutoDelete(_Class, _Member) NIds::NList::TCDLinkListAggregate<_Class, _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregateListNoPrevPtr< DIdsDefaultAllocator >, NIds::NList::TCDLinkAggregateListNoPrevPtrList< DIdsDefaultAllocator >, true, DIdsDefaultAllocator >
#		define DIdsListLinkDSA_List_FromTemplate(_Class, _Member) NIds::NList::TCDLinkListAggregate<_Class, typename _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregateListNoPrevPtr< DIdsDefaultAllocator >, NIds::NList::TCDLinkAggregateListNoPrevPtrList< DIdsDefaultAllocator >, false, DIdsDefaultAllocator >
#		define DIdsListLinkDSA_ListAutoDelete_FromTemplate(_Class, _Member) NIds::NList::TCDLinkListAggregate<_Class, typename _Class::CDLinkTranslator##_Member, NIds::NList::TCDLinkAggregateListNoPrevPtr< DIdsDefaultAllocator >, NIds::NList::TCDLinkAggregateListNoPrevPtrList< DIdsDefaultAllocator >, true, DIdsDefaultAllocator >

#		define DIdsListLinkDS_Iter(_Class, _Member) DIdsListLinkDSA_List(_Class, _Member)::CIterator
#		define DIdsListLinkDS_IterConst(_Class, _Member) DIdsListLinkDSA_List(_Class, _Member)::CIteratorConst
#		define DIdsListLinkDS_Iter_FromTemplate(_Class, _Member) DIdsListLinkDSA_List_FromTemplate(_Class, _Member)::CIterator
#		define DIdsListLinkDS_IterConst_FromTemplate(_Class, _Member) DIdsListLinkDSA_List_FromTemplate(_Class, _Member)::CIteratorConst

#		ifndef DIdsPNoShortCuts
#			define DLinkDS_Link(_Class, _Member) DIdsListLinkDS_Link(_Class, _Member)
#			define DLinkDS_Iter(_Class, _Member) DIdsListLinkDS_Iter(_Class, _Member)
#			define DLinkDS_IterConst(_Class, _Member) DIdsListLinkDS_IterConst(_Class, _Member)
#			define DLinkDS_List(_Class, _Member) DIdsListLinkDS_List(_Class, _Member)
#			define DLinkDS_ListAutoDelete(_Class, _Member) DIdsListLinkDS_ListAutoDelete(_Class, _Member)
#			define DLinkDSA_Link(_Class, _Member) DIdsListLinkDSA_Link(_Class, _Member)
#			define DLinkDSA_List(_Class, _Member) DIdsListLinkDSA_List(_Class, _Member)
#			define DLinkDSA_ListAutoDelete(_Class, _Member) DIdsListLinkDSA_ListAutoDelete(_Class, _Member)
#		endif

	};	

	namespace NList
	{		

		/************************************************************************************************\
		||¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯||
		|| Single linked list
		||______________________________________________________________________________________________||
		\************************************************************************************************/

		// Had to make this one a template so the functions won't get compiled at header time
//		template <aint t_Dummy = 0>

		template <typename t_CAllocator>
		class TCSLinkAggr
		{
		public:
			TCDynamicPtr<typename t_CAllocator::CPtrHolder, TCSLinkAggr> m_pNext;

			DIdsPInlineS void Construct()
			{
#				ifdef M_Profile
					m_pNext = DNP;
#				endif
			}

			DIdsPInlineS void Destruct()
			{
				DIdsAssert(!m_pNext, "You have to take care of it. No auto delete");
			}

			DIdsPInlineS void Link(TCSLinkAggr *_pLinkAfter)
			{
				DIdsAssert(!m_pNext, "Singly linked list cannot be automaticly unlinked, you have to remove it from the list manually");
				m_pNext = _pLinkAfter->m_pNext;
				_pLinkAfter->m_pNext = (TCSLinkAggr *)this;
			}

			DIdsPInlineS void Unlink(TCSLinkAggr *_pPrev)
			{
				DIdsAssert(_pPrev, "You better know what you are doing");
				_pPrev->m_pNext = m_pNext;
#				ifdef M_Profile
					m_pNext = DNP;
#				endif
			}

			DIdsPInlineS void UnsafeLink(TCSLinkAggr *_pLinkAfter)
			{
				m_pNext = _pLinkAfter->m_pNext;
				_pLinkAfter->m_pNext = (TCSLinkAggr *)this;
			}

			DIdsPInlineS void UnsafeUnlink(TCSLinkAggr *_pPrev)
			{
				_pPrev->m_pNext = m_pNext;
#				ifdef M_Profile
					m_pNext = DNP;
#				endif
			}

			DIdsPInlineS TCSLinkAggr *GetNext()
			{
				return m_pNext;
			}
		};


		template <typename t_CAllocator>
		class TCSLink : public TCSLinkAggr<t_CAllocator>
		{
		public:
			DIdsPInlineS TCSLink()
			{
				TCSLinkAggr<t_CAllocator>::Construct();
			}

			DIdsPInlineS ~TCSLink()
			{
				TCSLinkAggr<t_CAllocator>::Destruct();
			}
		};

		template <typename t_CAllocator>
		class TCSLinkListData_Last
		{
		public:
			TCSLinkAggr<t_CAllocator> m_First;
			TCDynamicPtr<typename t_CAllocator::CPtrHolder, TCSLinkAggr<t_CAllocator> > m_pLast;
			DIdsPInlineS TCSLinkAggr<t_CAllocator> &GetFirst()
			{
				return m_First;
			}
			DIdsPInlineS const TCSLinkAggr<t_CAllocator> &GetFirstConst() const
			{
				return m_First;
			}
			DIdsPInlineS TCSLinkAggr<t_CAllocator> *GetLast()
			{
				return m_pLast;
			}
			DIdsPInlineS const TCSLinkAggr<t_CAllocator> *GetLastConst() const
			{
				return m_pLast;
			}
			DIdsPInlineS void SetLast(TCSLinkAggr<t_CAllocator> *_pLast)
			{				
				m_pLast = _pLast;
			}
			DIdsPInlineS static bint HasLast()
			{
				return true;
			}
		};

		template <typename t_CAllocator>
		class TCSLinkListData
		{
		public:
			TCSLinkAggr<t_CAllocator> m_First;
			DIdsPInlineS TCSLinkAggr<t_CAllocator> &GetFirst()
			{
				return m_First;
			}
			DIdsPInlineS TCSLinkAggr<t_CAllocator> *GetLast()
			{
				return DNP;
			}
			DIdsPInlineS const TCSLinkAggr<t_CAllocator> &GetFirstConst() const
			{
				return m_First;
			}
			DIdsPInlineS const TCSLinkAggr<t_CAllocator> *GetLastConst() const
			{
				return DNP;
			}
			DIdsPInlineS void SetLast(TCSLinkAggr<t_CAllocator> *_pLast)
			{				
			}
			DIdsPInlineS static bint HasLast()
			{
				return false;
			}

		};

		template <typename t_CData, typename t_CTranslator, typename t_CListData, bint t_bAutoDelete, typename t_CAllocator>
		class TCSLinkListAggregate
		{
		public:

			t_CListData m_Data;

			DIdsPInlineS static TCSLinkAggr<t_CAllocator> *LinkFromMember(t_CData *_pMember)
			{				
				return ((TCSLinkAggr<t_CAllocator> *)(((uint8 *)_pMember) + t_CTranslator::GetOffset()));
			}

			DIdsPInlineS static t_CData *MemberFromLink(TCSLinkAggr<t_CAllocator> *_pLink)
			{				
				return ((t_CData *)(((uint8 *)_pLink) - t_CTranslator::GetOffset()));
			}

			DIdsPInlineS static const TCSLinkAggr<t_CAllocator> *LinkFromMemberConst(const t_CData *_pMember)
			{				
				return ((const TCSLinkAggr<t_CAllocator> *)(((uint8 *)_pMember) + t_CTranslator::GetOffset()));
			}

			DIdsPInlineS static const t_CData *MemberFromLinkConst(const TCSLinkAggr<t_CAllocator> *_pLink)
			{				
				return ((const t_CData *)(((uint8 *)_pLink) - t_CTranslator::GetOffset()));
			}

			void Construct()
			{
				m_Data.GetFirst().m_pNext = DNP;
				if (m_Data.HasLast())
				{
					m_Data.SetLast(&m_Data.GetFirst());
				}
			}

			void Destruct()
			{
				if (t_bAutoDelete)
					DeleteAll();
				else
					Clear();
			}

			void Clear()
			{
				while (m_Data.GetFirst().m_pNext)
				{
					m_Data.GetFirst().m_pNext->Unlink(&m_Data.GetFirst());
				}
				if (m_Data.HasLast())
					m_Data.SetLast(&m_Data.GetFirst());
			}


			void DeleteAll()
			{
				while (m_Data.GetFirst().m_pNext)
				{			
					t_CData *pToDelete = MemberFromLink(m_Data.GetFirst().m_pNext);
					m_Data.GetFirst().m_pNext->Unlink(&m_Data.GetFirst());

					pToDelete->~t_CData();
					t_CAllocator::Free(pToDelete);
				}

				if (m_Data.HasLast())
					m_Data.SetLast(&m_Data.GetFirst());
			}

			void Remove(t_CData *_pToRemove)
			{
				TCSLinkAggr<t_CAllocator> * pToRemove = LinkFromMember(_pToRemove);
				TCSLinkAggr<t_CAllocator> *pCurrent = &m_Data.GetFirst();
				while (pCurrent)
				{
					TCSLinkAggr<t_CAllocator> *pNext = pCurrent->m_pNext;
					if (pNext == pToRemove)
					{
						if (m_Data.HasLast())
						{
							if (!pToRemove->m_pNext)
								m_Data.SetLast(pCurrent);
						}
						pNext->Unlink(pCurrent);
						break;
					}
					pCurrent = pNext;
				}
			}

			void RemoveFirst()
			{
				if (m_Data.GetFirst().m_pNext)
				{
					if (m_Data.HasLast())
					{
						if (m_Data.GetFirst().m_pNext == m_Data.GetLast())
							m_Data.SetLast( &m_Data.GetFirst());
					}
					m_Data.GetFirst().m_pNext->Unlink(&m_Data.GetFirst());
				}
			}

			DIdsPInlineM t_CData *Pop()
			{
				if (m_Data.GetFirst().m_pNext)
				{
					TCSLinkAggr<t_CAllocator> * pToRemove = m_Data.GetFirst().m_pNext;
					if (m_Data.HasLast())
					{
						if (pToRemove == m_Data.GetLast())
							m_Data.SetLast(&m_Data.GetFirst());
					}
					pToRemove->UnsafeUnlink(&m_Data.GetFirst());
					return MemberFromLink(pToRemove);
				}
				else
					return DNP;
			}

			DIdsPInlineS void Push(t_CData *_pData)
			{
				UnsafeInsertHead(_pData);
			}

			void Delete(t_CData *_pToDelete)
			{
				Remove(_pToDelete);

				_pToDelete->~t_CData();
				t_CAllocator::Free(_pToDelete);
			}

			DIdsPInlineS t_CData *GetFirst()
			{
				if (m_Data.GetFirst().m_pNext)
					return MemberFromLink(m_Data.GetFirst().m_pNext);
				else
					return DNP;
			}

			DIdsPInlineS t_CData *GetLast()
			{
				if (m_Data.HasLast())
				{
					if (m_Data.GetLast() != &m_Data.GetFirst())
						return MemberFromLink(m_Data.GetLast());
					else 
						return DNP;
				}
				else
				{
					TCSLinkAggr<t_CAllocator> *pCurrent = m_Data.GetFirst().m_pNext;
					if (pCurrent)
					{
						while (pCurrent->m_pNext)
						{
							pCurrent = pCurrent->m_pNext;
						}

						return MemberFromLink(pCurrent);
					}
					else
						return DNP;
				}
			}

			DIdsPInlineS const t_CData *GetFirstConst() const
			{
				if (m_Data.GetFirst().m_pNext)
					return MemberFromLink(m_Data.GetFirst().m_pNext);
				else
					return DNP;
			}

			DIdsPInlineS const t_CData *GetLastConst() const
			{
				if (m_Data.HasLast())
				{
					if (m_Data.GetLast() != &m_Data.GetFirst())
						return MemberFromLink(m_Data.GetLast());
					else 
						return DNP;
				}
				else
				{
					TCSLinkAggr<t_CAllocator> *pCurrent = m_Data.GetFirst().m_pNext;
					if (pCurrent)
					{
						while (pCurrent->m_pNext)
						{
							pCurrent = pCurrent->m_pNext;
						}

						return MemberFromLink(pCurrent);
					}
					else
						return DNP;
				}
			}

			DIdsPInlineS bint IsEmpty()
			{
				return (m_Data.GetFirst().m_pNext == DNP);
			}

			//===================================
			// Inserts
			//===================================

			DIdsPInlineS void Insert(t_CData *_pToInsert)
			{
				if (m_Data.HasLast())
				{
					TCSLinkAggr<t_CAllocator> *pToLink = LinkFromMember(_pToInsert);
					pToLink->Link(m_Data.GetLast());
					m_Data.SetLast(pToLink);
				}
				else
				{
					InsertHead(_pToInsert);
				}
			}

			DIdsPInlineS void Insert(t_CData &_ToInsert)
			{
				Insert(&_ToInsert);
			}

			template <class t_CSortClass>
			void InsertSorted(t_CData &_ToInsert)
			{
				InsertSorted<t_CSortClass>(&_ToInsert);
			}

			template <class t_CSortClass>
			void InsertSorted(t_CData *_pToInsert)
			{
				TCSLinkAggr<t_CAllocator> *pCurrent = m_Data.GetFirst().m_pNext;
				if (pCurrent)
				{
					while (t_CSortClass::Compare(MemberFromLink(pCurrent), _pToInsert) > 0)
					{
						pCurrent = pCurrent->m_pNext;
					}

					TCSLinkAggr<t_CAllocator> *pToLink = LinkFromMember(_pToInsert);
					pToLink->Link(pCurrent);
				}

				if (pCurrent)
				{
					TCSLinkAggr<t_CAllocator> *pToLink = LinkFromMember(_pToInsert);
					pToLink->Link(pCurrent);
				}
				else
				{
					TCSLinkAggr<t_CAllocator> *pToLink = LinkFromMember(_pToInsert);
					pToLink->Link(&m_Data.GetFirst());
				}
			}
			
			// InsertTail
			DIdsPInlineS void InsertTail(t_CData *_pToInsert)
			{
				if (m_Data.HasLast())
				{
					TCSLinkAggr<t_CAllocator> *pToLink = LinkFromMember(_pToInsert);
					pToLink->Link(m_Data.GetLast());
					m_Data.SetLast(pToLink);
				}
				else
				{
					TCSLinkAggr<t_CAllocator> *pCurrent = m_Data.GetFirst().m_pNext;
					if (pCurrent)
					{
						while (pCurrent->m_pNext)
						{
							pCurrent = pCurrent->m_pNext;
						}

						TCSLinkAggr<t_CAllocator> *pToLink = LinkFromMember(_pToInsert);
						pToLink->Link(pCurrent);
					}
					else
					{
						TCSLinkAggr<t_CAllocator> *pToLink = LinkFromMember(_pToInsert);
						pToLink->Link(&m_Data.GetFirst());
					}

				}
			}
			DIdsPInlineS void InsertTail(t_CData &_ToInsert)
			{
				InsertTail(&_ToInsert);
			}

			// InsertHead
			DIdsPInlineS void InsertHead(t_CData *_pToInsert)
			{
				TCSLinkAggr<t_CAllocator> *pToLink = LinkFromMember(_pToInsert);

				pToLink->Link(&m_Data.GetFirst());

				if (m_Data.HasLast())
				{
					if (!pToLink->m_pNext)
						m_Data.SetLast(pToLink);
				}
			}
			DIdsPInlineS void InsertHead(t_CData &_ToInsert)
			{
				InsertHead(&_ToInsert);
			}

			// InsertHead
			DIdsPInlineS void InsertAfter(t_CData *_pToInsert, t_CData *_pToInsertAfter)
			{
				TCSLinkAggr<t_CAllocator> *pToLink = LinkFromMember(_pToInsert);
				TCSLinkAggr<t_CAllocator> *pToLinkAfter = LinkFromMember(_pToInsertAfter);

				pToLink->Link(&pToLinkAfter);

				if (m_Data.HasLast())
				{
					if (!pToLink->m_pNext)
						m_Data.SetLast(pToLink);
				}

			}
			DIdsPInlineS void InsertAfter(t_CData &_ToInsert, t_CData *_pToInsertAfter)
			{
				InsertAfter(&_ToInsert, _pToInsertAfter);
			}
			DIdsPInlineS void InsertAfter(t_CData *_pToInsert, t_CData &_ToInsertAfter)
			{
				InsertAfter(_pToInsert, &_ToInsertAfter);
			}
		
			DIdsPInlineS void InsertAfter(t_CData &_ToInsert, t_CData &_ToInsertAfter)
			{
				InsertAfter(&_ToInsert, &_ToInsertAfter);
			}

			//===================================
			// Unsafe Inserts
			//===================================

			DIdsPInlineS void UnsafeInsert(t_CData *_pToInsert)
			{
				if (m_Data.HasLast())
				{
					TCSLinkAggr<t_CAllocator> *pToLink = LinkFromMember(_pToInsert);
					pToLink->UnsafeLink(m_Data.GetLast());
					m_Data.SetLast(pToLink);
				}
				else
				{
					UnsafeInsertHead(_pToInsert);
				}
			}

			DIdsPInlineS void UnsafeInsert(t_CData &_ToInsert)
			{
				UnsafeInsert(&_ToInsert);
			}
			
			// InsertTail
			DIdsPInlineS void UnsafeInsertTail(t_CData *_pToInsert)
			{
				if (m_Data.HasLast())
				{
					TCSLinkAggr<t_CAllocator> *pToLink = LinkFromMember(_pToInsert);
					pToLink->UnsafeLink(m_Data.GetLast());
					m_Data.SetLast(pToLink);
				}
				else
				{
					TCSLinkAggr<t_CAllocator> *pCurrent = m_Data.GetFirst().m_pNext;
					if (pCurrent)
					{
						while (pCurrent->m_pNext)
						{
							pCurrent = pCurrent->m_pNext;
						}

						TCSLinkAggr<t_CAllocator> *pToLink = LinkFromMember(_pToInsert);
						pToLink->UnsafeLink(pCurrent);
					}
					else
					{
						TCSLinkAggr<t_CAllocator> *pToLink = LinkFromMember(_pToInsert);
						pToLink->UnsafeLink(&m_Data.GetFirst());
					}
				}
			}
			DIdsPInlineS void UnsafeInsertTail(t_CData &_ToInsert)
			{
				UnsafeInsertTail(&_ToInsert);
			}

			// InsertHead
			DIdsPInlineS void UnsafeInsertHead(t_CData *_pToInsert)
			{
				TCSLinkAggr<t_CAllocator> *pToLink = LinkFromMember(_pToInsert);

				pToLink->UnsafeLink(&m_Data.GetFirst());

				if (m_Data.HasLast())
				{
					if (!pToLink->m_pNext)
						m_Data.SetLast(pToLink);
				}
			}
			DIdsPInlineS void UnsafeInsertHead(t_CData &_ToInsert)
			{
				UnsafeInsertHead(&_ToInsert);
			}

			// InsertHead
			DIdsPInlineS void UnsafeInsertAfter(t_CData *_pToInsert, t_CData *_pToInsertAfter)
			{
				TCSLinkAggr<t_CAllocator> *pToLink = LinkFromMember(_pToInsert);
				TCSLinkAggr<t_CAllocator> *pToLinkAfter = LinkFromMember(_pToInsertAfter);

				pToLink->UnsafeLink(&pToLinkAfter);

				if (m_Data.HasLast())
				{
					if (!pToLink->m_pNext)
						m_Data.SetLast(pToLink);
				}
			}
			DIdsPInlineS void UnsafeInsertAfter(t_CData &_ToInsert, t_CData *_pToInsertAfter)
			{
				UnsafeInsertAfter(&_ToInsert, _pToInsertAfter);
			}
			DIdsPInlineS void UnsafeInsertAfter(t_CData *_pToInsert, t_CData &_ToInsertAfter)
			{
				UnsafeInsertAfter(_pToInsert, &_ToInsertAfter);
			}
			DIdsPInlineS void UnsafeInsertAfter(t_CData &_ToInsert, t_CData &_ToInsertAfter)
			{
				UnsafeInsertAfter(&_ToInsert, &_ToInsertAfter);
			}



			
			void Reverse()
			{
				TCSLinkAggr<t_CAllocator> *pCurrent = m_Data.GetFirst().m_pNext;
				m_Data.GetFirst().m_pNext = DNP;
				if (pCurrent)
				{
					if (m_Data.HasLast())
					{
						m_Data.SetLast(pCurrent);
					}
					
					while (pCurrent)
					{
						TCSLinkAggr<t_CAllocator> *pTemp = pCurrent;
						pCurrent = pCurrent->m_pNext;
						pTemp->UnsafeLink(&m_Data.GetFirst());
					}
				}
			}
			
			//element *listsort(element *list, aint is_circular, aint is_double) {
			
			typedef aint FMergeCompare(void *_pContext, void *_pFirst, void *_pSecond);
			class CMergeCallbackSort
			{
			public:
				void *m_pContext;
				FMergeCompare *m_pSortFunction;
				DIdsPInlineS static aint Compare(void *_pContext, void *_pFirst, void *_pSecond)
				{					
					return ((CMergeCallbackSort *)_pContext)->m_pSortFunction(((CMergeCallbackSort *)_pContext)->m_pContext, _pFirst, _pSecond);
				}
			};

			void MergeSortCallback(FMergeCompare *_pSortfunction, void *_pContext = DNP, aint _InsertionBits = 3)
			{
				CMergeCallbackSort SortContext;
				SortContext.m_pContext = _pContext;
				SortContext.m_pSortFunction = _pSortfunction;
				MergeSort<CMergeCallbackSort>(&SortContext, _InsertionBits);
			}
				
			/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
			|	Function:			Sorts the linked list								|
			|																			|
			|	Template params:														|
			|		t_CSortClass:	The class that implements the compare funtion.		|
			|						The funtion must be static and be called Compare.	|
			|						It takes parameters									|
			|						(void *_pContext, void *_pFirst, void *_pSecond)	|
			|						and returns less that 0 if _pFirst is less than		|
			|						_pSecond, more than 0 if _pSecond is more than		|
			|						_pFirst, and 0 if the items are equal.				|
			|																			|
			|						  You can inline the compare function with good		|
			|						results, if the compare is small.					|
			|																			|
			|	Parameters:																|
			|		_pContext:		The context that is sent to the compare function	|
			|																			|
			|		_InsertionBits:	The sort uses insertion sort to sort				|
			|						1 << _InsertionBits parts of the list, then uses	|
			|						mergesort to sort the rest.							|
			|																			|
			|						  If you have an expensive sort function you		|
			|						should use 2 for _InsertionBits, wich will mostly	|
			|						use the smallest number of compares for a given		|
			|						sort session. 4 or largen Will be fastest for		|
			|						small compare funcions, but 3 is probably a good	|
			|						compromise that will work good on most compares.	|
			|																			|
			|	Returns:			description											|
			|																			|
			|	Comments:			You can use this method instead of the callback		|
			|						method when you want the compare function to be		|
			|						able to be inlined, at cost of code size.			|
			|																			|
			\*_________________________________________________________________________*/

			template <class t_CSortClass>
			void MergeSort(void *_pContext = DNP, aint _InsertionBits = 3)
			{
				// We use mergesort as a stable and predictably performing algorithm, needing no extra heapspace or stackspace
				// O = n(log n)

				// Empty list ??
				if (IsEmpty())
					return;

				// Start with insertion sorting a bit first
				aint MergeSize = 1 << (_InsertionBits);

				if (_InsertionBits > 0)
				{
					const aint SortSize = 1 << _InsertionBits;

					TCSLinkAggr<t_CAllocator> *pCurrent = m_Data.GetFirst().m_pNext;
					TCSLinkAggr<t_CAllocator> *pLinkAfter = &m_Data.GetFirst();
					TCSLinkAggr<t_CAllocator> *pLast = &m_Data.GetFirst();
					m_Data.GetFirst().m_pNext = DNP;
					aint NumSorted = 0;

					while (pCurrent)
					{
						// Add first member

						++NumSorted;
						TCSLinkAggr<t_CAllocator> *pTemp = pCurrent->m_pNext;
						pCurrent->UnsafeLink(pLinkAfter);
						pLast = pCurrent;
						pCurrent = pTemp;

						if (!pCurrent)
							break;

						for (aint i = 1; i < SortSize; ++i)
						{
							aint NumChecked = i;
							++NumSorted;

							TCSLinkAggr<t_CAllocator> *pTemp = pLinkAfter->m_pNext;
							TCSLinkAggr<t_CAllocator> *pTempAfter = pLinkAfter;

							while (NumChecked)
							{
								if (t_CSortClass::Compare(_pContext,MemberFromLink(pCurrent),MemberFromLink(pTemp)) <= 0)
									break;
								--NumChecked;
								pTempAfter = pTemp;
								pTemp = pTemp->m_pNext;
							}

							TCSLinkAggr<t_CAllocator> *pTempNext = pCurrent->m_pNext;
							pCurrent->UnsafeLink(pTempAfter);
							if (!pCurrent->m_pNext)
								pLast = pCurrent;
							pCurrent = pTempNext;
							if (!pCurrent)
								break;
						}

						pLinkAfter = pLast;
					}

					if (m_Data.HasLast())
					{
						m_Data.SetLast(pLast);
					}

					if (NumSorted <= MergeSize)
					{
						// Done sorting
						return;
					}

				}

				DIdsAssert(_InsertionBits >= 0, "Has to be at least 1");
				
				
				while (1) 
				{
					TCSLinkAggr<t_CAllocator> *pFirst = m_Data.GetFirst().m_pNext;
					TCSLinkAggr<t_CAllocator> *pLast = &m_Data.GetFirst();
					
					aint MergesDone = 0;  // count number of merges we do in this pass
					
					while (pFirst) 
					{
						MergesDone++;  // there exists a merge to be done
						// step `MergeSize' places along from pFirst
						TCSLinkAggr<t_CAllocator> *pSecond = pFirst;
						TCSLinkAggr<t_CAllocator> *pLastFirst = pFirst;
						aint MergeSizeFirst = 0;
						aint MergeSizeSecond = MergeSize;
						while (MergeSizeFirst < MergeSize) 
						{
							MergeSizeFirst++;
							pLastFirst = pSecond;
							pSecond = pSecond->m_pNext;
							if (!pSecond)
								break;
						}
						
						// if pSecond hasn't fallen off end, we have two lists to merge

						if (!pSecond)
						{
							// End of list
							pLast->m_pNext = pFirst;
							pLast = DNP;
//							pLast = pFirst;
//							pFirst = pFirst->m_pNext;
//							MergeSizeFirst--;
						}
						else
						{			
							if (t_CSortClass::Compare(_pContext,MemberFromLink(pLastFirst),MemberFromLink(pSecond)) <= 0) 
							{
								// Lists already sorted
								pLast->m_pNext = pFirst;
								while (MergeSizeSecond * (mint)pSecond)
								{
									--MergeSizeSecond;
									pLast = pSecond;
									pSecond = pSecond->m_pNext;
								}
							}
							else
							{
								
								DIdsAssert(MergeSizeSecond && MergeSizeFirst, "Hula");
								// now we have two lists; merge them 
								while (1) 
								{
									// decide whether m_pNext element of merge comes from pFirst or pSecond 
									if (t_CSortClass::Compare(_pContext,MemberFromLink(pFirst),MemberFromLink(pSecond)) <= 0) 
									{
										// First element of pFirst is lower (or same); pTemp must come from pFirst.
										pLast->m_pNext = pFirst;
										pLast = pFirst;
										pFirst = pFirst->m_pNext;
										MergeSizeFirst--;
										if (!MergeSizeFirst)
										{
											pLast->m_pNext = pSecond;
											
											while (MergeSizeSecond * (mint)pSecond)
											{
												--MergeSizeSecond;
												pLast = pSecond;
												pSecond = pSecond->m_pNext;
											}
											break;
										}
									} 
									else 
									{
										// First element of pSecond is lower; pTemp must come from pSecond.
										pLast->m_pNext = pSecond;
										pLast = pSecond;
										pSecond = pSecond->m_pNext;
										MergeSizeSecond--;
										if (!pSecond || !MergeSizeSecond)
										{
											pLast->m_pNext = pFirst;
											
											while (MergeSizeFirst) 
											{
												MergeSizeFirst--;
												pLast = pFirst;
												pFirst = pFirst->m_pNext;
											}
											break;
										}
									}
								}
							}
						}
						
						// now pFirst has stepped `MergeSize' places along, and pSecond has too
						pFirst = pSecond;
					}
					
					
					if (pLast)
						pLast->m_pNext = DNP;
					
					// If we have done only one merge, we're finished.
					if (MergesDone <= 1)   // allow for MergesDone==0, the empty list case
					{
						if (m_Data.HasLast())
						{
							m_Data.SetLast(pLast);
						}
						return;
					}
					
					// Otherwise repeat, merging lists twice the size
					MergeSize <<= 1;
				}
			}

			typedef aint FBucketGetIndex(void *_pContext, aint _Place, void *_pItem);
			class CBucketCallbackSort
			{
			public:
				void *m_pContext;
				FBucketGetIndex *m_pSortFunction;
				DIdsPInlineS static aint GetIndex(void *_pContext, aint _Place, void *_pItem)
				{					
					return ((CBucketCallbackSort *)_pContext)->m_pSortFunction(((CBucketCallbackSort *)_pContext)->m_pContext, _pItem);
				}
			};

			template <aint t_BucketSize>
			void BucketSortCallback(FBucketGetIndex *_pSortfunction, void *_pContext = DNP)
			{
				CBucketCallbackSort SortContext;
				SortContext.m_pContext = _pContext;
				SortContext.m_pSortFunction = _pSortfunction;
				BucketSort<CBucketCallbackSort, t_BucketSize>(&SortContext);
			}


			/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
			|	Function:			Sorts the linked list								|
			|																			|
			|	Template params:														|
			|		t_CSortClass:	The class that implements the index funtion.		|
			|						The funtion must be static and be called GetIndex.	|
			|						It takes parameters									|
			|						(void *_pContext, aint _Place, void *_pItem)		|
			|						and returns the least significant index for _pItem	|
			|																			|
			|						  You can inline the compare function with good		|
			|						results, if the compare is small.					|
			|																			|
			|	Parameters:																|
			|		_pContext:		The context that is sent to the compare function	|
			|																			|
			|		_InsertionBits:	The sort uses insertion sort to sort				|
			|						1 << _InsertionBits parts of the list, then uses	|
			|						mergesort to sort the rest.							|
			|																			|
			|						  If you have an expensive sort function you		|
			|						should use 2 for _InsertionBits, wich will mostly	|
			|						use the smallest number of compares for a given		|
			|						sort session. 4 or largen Will be fastest for		|
			|						small compare funcions, but 3 is probably a good	|
			|						compromise that will work good on most compares.	|
			|																			|
			|	Returns:			description											|
			|																			|
			|	Comments:			You can use this method instead of the callback		|
			|						method when you want the compare function to be		|
			|						able to be inlined, at cost of code size.			|
			|																			|
			\*_________________________________________________________________________*/
			template <class t_CSortClass, aint t_BucketSize>
			void BucketSort(aint _NumPlaces, void *_pContext = DNP)
			{
				if (IsEmpty())
					return;

				TCSLinkAggr<t_CAllocator> *Bucket[t_BucketSize];

				// Reset buckets
				for (aint i = 0; i < t_BucketSize; ++i)
				{
					Bucket[i] = DNP;
				}

				// Put list in first bucket

				for(aint Place = 0; Place < _NumPlaces; ++Place)
				{					
					// Remove last link
					TCSLinkAggr<t_CAllocator> *pCurrent = m_Data.GetFirst().m_pNext;

					while (pCurrent)
					{
						aint Index = t_CSortClass::GetIndex(_pContext, Place, MemberFromLink(pCurrent));
						TCSLinkAggr<t_CAllocator> **pBucket = &Bucket[Index];

						TCSLinkAggr<t_CAllocator> *pNext = pCurrent->m_pNext;

						pCurrent->m_pNext = (*pBucket);
						(*pBucket) = pCurrent;

						pCurrent = pNext;
					}

					
					// Rebuild list

					m_Data.GetFirst().m_pNext = DNP;
					TCSLinkAggr<t_CAllocator> *pLast = &m_Data.GetFirst();
					
					for (aint i = 0; i < t_BucketSize; ++i)
					{
						if (Bucket[i])
						{
							TCSLinkAggr<t_CAllocator> *pTemp = Bucket[i];
							Bucket[i] = DNP;
							TCSLinkAggr<t_CAllocator> *pLinkAfter = pLast;
							pLast = pTemp;

							while (pTemp)
							{
								TCSLinkAggr<t_CAllocator> *pTempNext = pTemp->m_pNext;
								pTemp->UnsafeLink(pLinkAfter);
								pTemp = pTempNext;
							}
						}
					}
					if (m_Data.HasLast())
					{
						m_Data.SetLast(pLast);
					}
				}

			}

			class CIterator
			{
			public:
				TCSLinkAggr<t_CAllocator> *m_pCurrent;

				CIterator()
				{
					m_pCurrent = DNP;
				}

				CIterator(const TCSLinkListAggregate<t_CData, t_CTranslator, t_CListData, t_bAutoDelete, t_CAllocator> &_List)
				{
					m_pCurrent = _List.m_Data.GetFirstConst().m_pNext;
				}

				CIterator& operator = (const TCSLinkListAggregate<t_CData, t_CTranslator, t_CListData, t_bAutoDelete, t_CAllocator> &_List)
				{
					m_pCurrent = _List.m_Data.GetFirstConst().m_pNext;
					return *this;
				}
				
				DIdsPInlineS t_CData *GetCurrent()
				{
					if (m_pCurrent)
						return TCSLinkListAggregate<t_CData, t_CTranslator, t_CListData, t_bAutoDelete, t_CAllocator>::MemberFromLink(m_pCurrent);
					else
						return DNP;
				}

				DIdsPInlineS operator t_CData *()
				{
					return GetCurrent();
				}

				DIdsPInlineS t_CData * operator ->()
				{
					return GetCurrent();
				}

				DIdsPInlineS void operator ++()
				{
					if (m_pCurrent)
					{
						m_pCurrent = m_pCurrent->m_pNext;
					}
				}

			};

			class CIteratorConst
			{
			public:
				const TCSLinkAggr<t_CAllocator> *m_pCurrent;

				CIteratorConst()
				{
					m_pCurrent = DNP;
				}

				CIteratorConst(const TCSLinkListAggregate<t_CData, t_CTranslator, t_CListData, t_bAutoDelete, t_CAllocator> &_List)
				{
					m_pCurrent = _List.m_Data.GetFirstConst().m_pNext;
				}

				CIteratorConst& operator = (const TCSLinkListAggregate<t_CData, t_CTranslator, t_CListData, t_bAutoDelete, t_CAllocator> &_List)
				{
					m_pCurrent = _List.m_Data.GetFirstConst().m_pNext;
					return *this;
				}
				
				DIdsPInlineS const t_CData *GetCurrent()
				{
					if (m_pCurrent)
						return TCSLinkListAggregate<t_CData, t_CTranslator, t_CListData, t_bAutoDelete, t_CAllocator>::MemberFromLink(m_pCurrent);
					else
						return DNP;
				}

				DIdsPInlineS operator const t_CData *()
				{
					return GetCurrent();
				}

				DIdsPInlineS const t_CData * operator ->()
				{
					return GetCurrent();
				}

				DIdsPInlineS void operator ++()
				{
					if (m_pCurrent)
					{
						m_pCurrent = m_pCurrent->m_pNext;
					}
				}

			};


		};

		template <typename t_CData, typename t_CTranslator, typename t_CListData, bint t_bAutoDelete, typename t_CAllocator>
		class TCSLinkList : public TCSLinkListAggregate<t_CData, t_CTranslator, t_CListData, t_bAutoDelete, t_CAllocator>
		{
		public:
			TCSLinkList()
			{
				TCSLinkListAggregate<t_CData, t_CTranslator, t_CListData, t_bAutoDelete, t_CAllocator>::Construct();
			}
			~TCSLinkList()
			{
				TCSLinkListAggregate<t_CData, t_CTranslator, t_CListData, t_bAutoDelete, t_CAllocator>::Destruct();
			}
		};

#		define DIdsListLinkS_Trans(_Class, _Member) \
				class CSLinkTranslator##_Member \
				{\
				public:\
					DIdsPInlineS static aint GetOffset()\
					{\
						const _Class *pPtr = 0;\
						return (aint)(void *)(&((pPtr)->_Member));\
					}\
				};\


        /***************************************************************************************************\
        |¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|
        | Macros for void * Pointer Holder																	|
        |___________________________________________________________________________________________________|
        \***************************************************************************************************/


#		define DIdsListLinkS_Link(_Class, _Member) \
				DIdsListLinkS_Trans(_Class, _Member) \
				NIds::NList::TCSLink< DIdsDefaultAllocator > _Member;

#		define DIdsListLinkSA_Link(_Class, _Member) \
				DIdsListLinkS_Trans(_Class, _Member) \
				NIds::NList::TCSLinkAggr< DIdsDefaultAllocator > _Member;


#		define DIdsListLinkS_Member(_Class, _Member) \
				NIds::NList::TCSLink< DIdsDefaultAllocator > _Member;
#		define DIdsListLinkSA_Member(_Class, _Member) \
				NIds::NList::TCSLinkAggr< DIdsDefaultAllocator > _Member;

#		define DIdsListLinkS_LinkType() \
				NIds::NList::TCSLink< DIdsDefaultAllocator >
#		define DIdsListLinkSA_LinkType() \
				NIds::NList::TCSLinkAggr< DIdsDefaultAllocator >

		// Link with pLast

#		define DIdsListLinkS_Iter(_Class, _Member) DIdsListLinkS_List(_Class, _Member)::CIterator
#		define DIdsListLinkS_List(_Class, _Member) NIds::NList::TCSLinkList<_Class, _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData_Last< DIdsDefaultAllocator >, false, DIdsDefaultAllocator >
#		define DIdsListLinkS_ListAutoDelete(_Class, _Member) NIds::NList::TCSLinkList<_Class, _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData_Last< DIdsDefaultAllocator >, true, DIdsDefaultAllocator >

#		define DIdsListLinkSA_List(_Class, _Member) NIds::NList::TCSLinkListAggregate<_Class, _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData_Last< DIdsDefaultAllocator >, false, DIdsDefaultAllocator >
#		define DIdsListLinkSA_ListAutoDelete(_Class, _Member) NIds::NList::TCSLinkListAggregate<_Class, _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData_Last< DIdsDefaultAllocator >, true, DIdsDefaultAllocator >

#		define DIdsListLinkS_Iter_FromTemplate(_Class, _Member) DIdsListLinkS_List_Fromtemplate(_Class, _Member)::CIterator
#		define DIdsListLinkS_List_Fromtemplate(_Class, _Member) NIds::NList::TCSLinkList<_Class, typename _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData_Last< DIdsDefaultAllocator >, false, DIdsDefaultAllocator >
#		define DIdsListLinkS_ListAutoDelete_FromTemplate(_Class, _Member) NIds::NList::TCSLinkList<_Class, typename _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData_Last< DIdsDefaultAllocator >, true, DIdsDefaultAllocator >

#		define DIdsListLinkSA_List_FromTemplate(_Class, _Member) NIds::NList::TCSLinkListAggregate<_Class, typename _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData_Last< DIdsDefaultAllocator >, false, DIdsDefaultAllocator >
#		define DIdsListLinkSA_ListAutoDelete_FromTemplate(_Class, _Member) NIds::NList::TCSLinkListAggregate<_Class, typename _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData_Last< DIdsDefaultAllocator >, true, DIdsDefaultAllocator >
					
		// Link Without pLast

#		define DIdsListLinkS_IterNoLastPtr(_Class, _Member) DIdsListLinkS_ListNoLastPtr(_Class, _Member)::CIterator
#		define DIdsListLinkS_ListNoLastPtr(_Class, _Member) NIds::NList::TCSLinkList<_Class, _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData< DIdsDefaultAllocator >, false, DIdsDefaultAllocator >
#		define DIdsListLinkS_ListAutoDeleteNoLastPtr(_Class, _Member) NIds::NList::TCSLinkList<_Class, _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData< DIdsDefaultAllocator >, true, DIdsDefaultAllocator >

#		define DIdsListLinkSA_ListNoLastPtr(_Class, _Member) NIds::NList::TCSLinkListAggregate<_Class, _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData< DIdsDefaultAllocator >, false, DIdsDefaultAllocator >
#		define DIdsListLinkSA_ListAutoDeleteNoLastPtr(_Class, _Member) NIds::NList::TCSLinkListAggregate<_Class, _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData< DIdsDefaultAllocator >, true, DIdsDefaultAllocator >

#		define DIdsListLinkS_IterNoLastPtr_FromTemplate(_Class, _Member) DIdsListLinkS_ListNoLastPtr_FromTemplate(_Class, _Member)::CIterator
#		define DIdsListLinkS_ListNoLastPtr_FromTemplate(_Class, _Member) NIds::NList::TCSLinkList<_Class, typename _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData< DIdsDefaultAllocator >, false, DIdsDefaultAllocator >
#		define DIdsListLinkS_ListAutoDeleteNoLastPtr_FromTemplate(_Class, _Member) NIds::NList::TCSLinkList<_Class, typename _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData< DIdsDefaultAllocator >, true, DIdsDefaultAllocator >

#		define DIdsListLinkSA_ListNoLastPtr_FromTemplate(_Class, _Member) NIds::NList::TCSLinkListAggregate<_Class, typename _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData< DIdsDefaultAllocator >, false, DIdsDefaultAllocator >
#		define DIdsListLinkSA_ListAutoDeleteNoLastPtr_FromTemplate(_Class, _Member) NIds::NList::TCSLinkListAggregate<_Class, typename _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData< DIdsDefaultAllocator >, true, DIdsDefaultAllocator >
					
#		ifndef DIdsPNoShortCuts
#			define DLinkS_Link(_Class, _Member) DIdsListLinkS_Link(_Class, _Member)
#			define DLinkS_Trans(_Class, _Member) DIdsListLinkS_Trans(_Class, _Member)
#			define DLinkS_Member(_Class, _Member) DIdsListLinkS_Member(_Class, _Member)
#			define DLinkS_LinkType(_Class, _Member) DIdsListLinkS_LinkType(_Class, _Member)
#			define DLinkS_Iter(_Class, _Member) DIdsListLinkS_Iter(_Class, _Member)
#			define DLinkS_List(_Class, _Member) DIdsListLinkS_List(_Class, _Member)
#			define DLinkS_ListAutoDelete(_Class, _Member) DIdsListLinkS_ListAutoDelete(_Class, _Member)
#			define DLinkS_IterNoLastPtr(_Class, _Member) DIdsListLinkS_IterNoLastPtr(_Class, _Member)
#			define DLinkS_ListNoLastPtr(_Class, _Member) DIdsListLinkS_ListNoLastPtr(_Class, _Member)
#			define DLinkS_ListAutoDeleteNoLastPtr(_Class, _Member) DIdsListLinkS_ListAutoDeleteNoLastPtr(_Class, _Member)
#			define DLinkSA_Link(_Class, _Member) DIdsListLinkSA_Link(_Class, _Member)
#			define DLinkSA_Member(_Class, _Member) DIdsListLinkSA_Member(_Class, _Member)
#			define DLinkSA_LinkType(_Class, _Member) DIdsListLinkSA_LinkType(_Class, _Member)
#			define DLinkSA_List(_Class, _Member) DIdsListLinkSA_List(_Class, _Member)
#			define DLinkSA_ListAutoDelete(_Class, _Member) DIdsListLinkSA_ListAutoDelete(_Class, _Member)
#			define DLinkSA_ListNoLastPtr(_Class, _Member) DIdsListLinkSA_ListNoLastPtr(_Class, _Member)
#			define DLinkSA_ListAutoDeleteNoLastPtr(_Class, _Member) DIdsListLinkSA_ListAutoDeleteNoLastPtr(_Class, _Member)
#		endif

        /***************************************************************************************************\
        |¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|
        | Macros for variable Pointer Holder																|
        |___________________________________________________________________________________________________|
        \***************************************************************************************************/


#		define DIdsListLinkAllocatorS_Member(_Member, _Allocator) \
				NIds::NList::TCSLink<_Allocator> _Member;
#		define DIdsListLinkAllocatorSA_Member(_Member, _Allocator) \
				NIds::NList::TCSLinkAggr<_Allocator> _Member;

#		define DIdsListLinkAllocatorS_Link(_Class, _Member, _Allocator) \
				DIdsListLinkS_Trans(_Class, _Member) \
				DIdsListLinkAllocatorS_Member(_Member, _Allocator)

// List link: | Single | Aggregate | Pointer Holder
#		define DIdsListLinkAllocatorSA_Link(_Class, _Member, _Allocator) \
				DIdsListLinkS_Trans(_Class, _Member) \
				DIdsListLinkAllocatorSA_Member(_Member, _Allocator)


#		define DIdsListLinkAllocatorS_LinkType(_Allocator) \
				NIds::NList::TCSLink<_Allocator>
#		define DIdsListLinkAllocatorSA_LinkType(_Allocator) \
				NIds::NList::TCSLinkAggr<_Allocator>

		// Link with pLast

#		define DIdsListLinkAllocatorS_Iter(_Class, _Member, _Allocator) DIdsListLinkAllocatorS_List(_Class, _Member, _Allocator)::CIterator
#		define DIdsListLinkAllocatorS_List(_Class, _Member, _Allocator) NIds::NList::TCSLinkList<_Class, _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData_Last<_Allocator>, false, _Allocator>
#		define DIdsListLinkAllocatorS_ListAutoDelete(_Class, _Member, _Allocator) NIds::NList::TCSLinkList<_Class, _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData_Last<_Allocator>, true, _Allocator>

#		define DIdsListLinkAllocatorSA_List(_Class, _Member, _Allocator) NIds::NList::TCSLinkListAggregate<_Class, _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData_Last<_Allocator>, false, _Allocator>
#		define DIdsListLinkAllocatorSA_ListAutoDelete(_Class, _Member, _Allocator) NIds::NList::TCSLinkListAggregate<_Class, _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData_Last<_Allocator>, true, _Allocator>

#		define DIdsListLinkAllocatorS_Iter_FromTemplate(_Class, _Member, _Allocator) DIdsListLinkAllocatorS_List_Fromtemplate(_Class, _Member, _Allocator)::CIterator
#		define DIdsListLinkAllocatorS_List_Fromtemplate(_Class, _Member, _Allocator) NIds::NList::TCSLinkList<_Class, typename _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData_Last<_Allocator>, false, _Allocator>
#		define DIdsListLinkAllocatorS_ListAutoDelete_FromTemplate(_Class, _Member, _Allocator) NIds::NList::TCSLinkList<_Class, typename _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData_Last<_Allocator>, true, _Allocator>

#		define DIdsListLinkAllocatorSA_List_FromTemplate(_Class, _Member, _Allocator) NIds::NList::TCSLinkListAggregate<_Class, typename _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData_Last<_Allocator>, false, _Allocator>
#		define DIdsListLinkAllocatorSA_ListAutoDelete_FromTemplate(_Class, _Member, _Allocator) NIds::NList::TCSLinkListAggregate<_Class, typename _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData_Last<_Allocator>, true, _Allocator>
					
		// Link Without pLast

#		define DIdsListLinkAllocatorS_IterNoLastPtr(_Class, _Member, _Allocator) DIdsListLinkAllocatorS_ListNoLastPtr(_Class, _Member, _Allocator)::CIterator
#		define DIdsListLinkAllocatorS_ListNoLastPtr(_Class, _Member, _Allocator) NIds::NList::TCSLinkList<_Class, _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData<_Allocator>, false, _Allocator>
#		define DIdsListLinkAllocatorS_ListAutoDeleteNoLastPtr(_Class, _Member, _Allocator) NIds::NList::TCSLinkList<_Class, _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData<_Allocator>, true, _Allocator>

#		define DIdsListLinkAllocatorSA_ListNoLastPtr(_Class, _Member, _Allocator) NIds::NList::TCSLinkListAggregate<_Class, _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData<_Allocator>, false, _Allocator>
#		define DIdsListLinkAllocatorSA_ListAutoDeleteNoLastPtr(_Class, _Member, _Allocator) NIds::NList::TCSLinkListAggregate<_Class, _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData<_Allocator>, true, _Allocator>

#		define DIdsListLinkAllocatorS_IterNoLastPtr_FromTemplate(_Class, _Member, _Allocator) DIdsListLinkAllocatorS_ListNoLastPtr_FromTemplate(_Class, _Member, _Allocator)::CIterator
#		define DIdsListLinkAllocatorS_ListNoLastPtr_FromTemplate(_Class, _Member, _Allocator) NIds::NList::TCSLinkList<_Class, typename _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData<_Allocator>, false, _Allocator>
#		define DIdsListLinkAllocatorS_ListAutoDeleteNoLastPtr_FromTemplate(_Class, _Member, _Allocator) NIds::NList::TCSLinkList<_Class, typename _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData<_Allocator>, true, _Allocator>

#		define DIdsListLinkAllocatorSA_ListNoLastPtr_FromTemplate(_Class, _Member, _Allocator) NIds::NList::TCSLinkListAggregate<_Class, typename _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData<_Allocator>, false, _Allocator>
#		define DIdsListLinkAllocatorSA_ListAutoDeleteNoLastPtr_FromTemplate(_Class, _Member, _Allocator) NIds::NList::TCSLinkListAggregate<_Class, typename _Class::CSLinkTranslator##_Member, NIds::NList::TCSLinkListData<_Allocator>, true, _Allocator>
					
#		ifndef DIdsPNoShortCuts
#			define DLinkAllocatorS_Link(_Class, _Member) DIdsListLinkAllocatorS_Link(_Class, _Member)
#			define DLinkAllocatorS_Member(_Class, _Member) DIdsListLinkAllocatorS_Member(_Class, _Member)
#			define DLinkAllocatorS_LinkType(_Class, _Member) DIdsListLinkAllocatorS_LinkType(_Class, _Member)
#			define DLinkAllocatorS_Iter(_Class, _Member) DIdsListLinkAllocatorS_Iter(_Class, _Member)
#			define DLinkAllocatorS_List(_Class, _Member) DIdsListLinkAllocatorS_List(_Class, _Member)
#			define DLinkAllocatorS_ListAutoDelete(_Class, _Member) DIdsListLinkAllocatorS_ListAutoDelete(_Class, _Member)
#			define DLinkAllocatorS_IterNoLastPtr(_Class, _Member) DIdsListLinkAllocatorS_IterNoLastPtr(_Class, _Member)
#			define DLinkAllocatorS_ListNoLastPtr(_Class, _Member) DIdsListLinkAllocatorS_ListNoLastPtr(_Class, _Member)
#			define DLinkAllocatorS_ListAutoDeleteNoLastPtr(_Class, _Member) DIdsListLinkAllocatorS_ListAutoDeleteNoLastPtr(_Class, _Member)
#			define DLinkAllocatorSA_Link(_Class, _Member) DIdsListLinkAllocatorSA_Link(_Class, _Member)
#			define DLinkAllocatorSA_Member(_Class, _Member) DIdsListLinkAllocatorSA_Member(_Class, _Member)
#			define DLinkAllocatorSA_LinkType(_Class, _Member) DIdsListLinkAllocatorSA_LinkType(_Class, _Member)
#			define DLinkAllocatorSA_List(_Class, _Member) DIdsListLinkAllocatorSA_List(_Class, _Member)
#			define DLinkAllocatorSA_ListAutoDelete(_Class, _Member) DIdsListLinkAllocatorSA_ListAutoDelete(_Class, _Member)
#			define DLinkAllocatorSA_ListNoLastPtr(_Class, _Member) DIdsListLinkAllocatorSA_ListNoLastPtr(_Class, _Member)
#			define DLinkAllocatorSA_ListAutoDeleteNoLastPtr(_Class, _Member) DIdsListLinkAllocatorSA_ListAutoDeleteNoLastPtr(_Class, _Member)
#		endif

	};	


};

#endif // DIds_Inc_Mda_Ids_TreeAVL_h
