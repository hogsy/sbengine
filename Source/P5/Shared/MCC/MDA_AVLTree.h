
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/


#ifndef __MDA_AVLTREE
#define __MDA_AVLTREE

#include "MMemMgrPool.h"

/*
 *  Which of a given node's subtrees is higher?
 */
enum AVLSKEW 
{
	AVLSKEW_NONE,	
	AVLSKEW_LEFT,
	AVLSKEW_RIGHT
};

enum AVLRES
{
	AVLRES_ERROR = 0,
	AVLRES_OK,
	AVLRES_BALANCE,
};

extern CDA_Pool_Static TreeablePool;

class CDA_AVLTreeable;
class CDA_AVLTree;

class MCCDLLEXPORT CDA_AVLTreeableMember : public CDA_Poolable
{
public:

	// We can do this because we know that TreeableMember will be 4 byte aligned
	CDA_AVLTreeableMember *m_pLeft;
	CDA_AVLTreeableMember *m_pRight;
	CDA_AVLTree* m_pTree;
	CDA_AVLTreeable *m_pTreeable;
	CDA_AVLTreeableMember *m_pNextTreaable;

	// CHECKME: Removed struct {}
	mint m_Folded_m_pPrevTreeable:(sizeof(mint)*8-2);
	mint m_Skew:2;

	M_INLINE CDA_AVLTreeableMember *GetPrevTreaable()
	{
		return (CDA_AVLTreeableMember *)(m_Folded_m_pPrevTreeable<< 2);
	}

	M_INLINE void SetPrevTreaable(CDA_AVLTreeableMember *_Member)
	{
		m_Folded_m_pPrevTreeable = (mint)((((mint)_Member) >> 2));
	}

	void Link(CDA_AVLTreeable *_pTreable);

	CDA_AVLTreeableMember(CDA_AVLTree* _pTree, CDA_AVLTreeable *_pTreable);
	~CDA_AVLTreeableMember();
};

class MCCDLLEXPORT CDA_AVLTreeable
{
public:
	CDA_AVLTreeableMember *DA__m_pFirstTreeable;

	void AVLTree_Insert(CDA_AVLTree *_Tree);
	void AVLTree_Remove(CDA_AVLTree *_Tree);

	CDA_AVLTreeable();
	virtual ~CDA_AVLTreeable();
};


class MCCDLLEXPORT CDA_AVLTree
{
	friend class CDA_AVLTreeable;
	friend class CDA_AVLTreeableMember;
public:
	CDA_AVLTreeableMember *m_pRoot;
	
	CDA_AVLTreeableMember * SearchExact(CDA_AVLTreeableMember *_Object, void *_Key);
	CDA_AVLTreeableMember * SearchSmallestLarger(CDA_AVLTreeableMember *_Object, void *_Key, CDA_AVLTreeableMember** BestFit);
	CDA_AVLTreeableMember * SearchLargerSmallest(CDA_AVLTreeableMember *_Object, void *_Key, CDA_AVLTreeableMember** BestFit);

	AVLRES Remove(CDA_AVLTreeableMember **_Object, void *_Key);
	AVLRES Insert(CDA_AVLTreeableMember **_Object, CDA_AVLTreeableMember *_ObjectToInsert);

	static int FindLowest(CDA_AVLTreeableMember *target, CDA_AVLTreeableMember **_Object, AVLRES *res);
	static int FindHighest(CDA_AVLTreeableMember *target, CDA_AVLTreeableMember **_Object, AVLRES *res);
	static AVLRES RightShrunk(CDA_AVLTreeableMember **_Object);
	static AVLRES LeftShrunk(CDA_AVLTreeableMember **_Object);
	static AVLRES RightGrown(CDA_AVLTreeableMember **_Object);
	static AVLRES LeftGrown(CDA_AVLTreeableMember **_Object);
	static void RotateRight(CDA_AVLTreeableMember **_Object);
	static void RotateLeft(CDA_AVLTreeableMember **_Object);
	static void Swap(CDA_AVLTreeableMember *_Object1, CDA_AVLTreeableMember *_Object2);

public:
	CDA_AVLTree();

	CDA_AVLTreeable* FindExact(void *_Key);
	CDA_AVLTreeable* FindSmallestLarger(void *_Key);
	CDA_AVLTreeable* FindLargerSmallest(void *_Key);
	CDA_AVLTreeable* FindLargest();

	CDA_AVLTreeableMember* GetRoot();

	virtual int Compare(CDA_AVLTreeable *_Object, void *_Key) pure;
	virtual void *GetValue(CDA_AVLTreeable *_Object) pure;

			// 150 % marginal over could possably fit in memory (the tree is not perfect)
	template <typename CData>
	class CIterator
	{
	public:
		
		int m_iStack;
		const CDA_AVLTreeableMember *m_pStack[32];

		CIterator()
		{
			// Reset iterator
			m_iStack = -1;
		}

		CIterator(const CIterator &_Tree)
		{
			m_iStack = _Tree.m_iStack;

			for (int i = 0; i < m_iStack; ++i)
				m_pStack[i] = _Tree.m_pStack[i];
		}

		CIterator(const CDA_AVLTree &_Tree)
		{
			StartForward(&_Tree);
		}

		CIterator(const CDA_AVLTree *_pTree)
		{
			StartForward(_pTree);
		}

		void StartForward(const CDA_AVLTree &_Tree)
		{
			StartForward(&_Tree);
		}

		void StartForward(const CDA_AVLTree *_pTree)
		{					
			// Find the smallest item in tree, and build stack
			const CDA_AVLTreeableMember *pCurrent = _pTree->m_pRoot;
			int iStack = -1;
			
			while (pCurrent)
			{
				m_pStack[++iStack] = pCurrent;
				pCurrent = pCurrent->m_pLeft;
			}

			m_iStack = iStack;
		}

		void SetRoot(const CDA_AVLTree &_pTree)
		{
			SetRoot(_pTree);
		}

		void SetRoot(const CDA_AVLTree *_pTree)
		{
			m_pStack[0] = _pTree->m_pRoot;
			m_iStack = 0;
		}

		void Next()
		{
			if (m_iStack < 0)
			{
				// We are already done
				return;
			}
			
			const CDA_AVLTreeableMember *pCurrent = m_pStack[m_iStack];

			pCurrent = pCurrent->m_pRight;
			// Decrease stack so we overwrite the current stack
			--m_iStack;
			while (pCurrent)
			{
				m_pStack[++m_iStack] = pCurrent;
				pCurrent = pCurrent->m_pLeft;
			}
		}

		void StartBackward(const CDA_AVLTree &_Tree)
		{
			StartBackward(&_Tree);
		}

		void StartBackward(const CDA_AVLTree *_pTree)
		{					
			// Find the smallest item in tree, and build stack
			const CDA_AVLTreeableMember *pCurrent = _pTree->m_pRoot;
			int iStack = -1;
			
			while (pCurrent)
			{
				m_pStack[++iStack] = pCurrent;
				pCurrent = pCurrent->m_pRight;
			}

			m_iStack = iStack;
		}

		void Prev()
		{
			if (m_iStack < 0)
			{
				// We are already done

				return;
			}
			
			const CDA_AVLTreeableMember *pCurrent = m_pStack[m_iStack];

			pCurrent = pCurrent->m_pLeft;
			// Decrease stack so we overwrite the current stack
			--m_iStack;
			while (pCurrent)
			{
				m_pStack[++m_iStack] = pCurrent;
				pCurrent = pCurrent->m_pRight;
			}
		}


		CData *GetCurrent() const
		{
			if (m_iStack >= 0)
				return (CData *)m_pStack[m_iStack]->m_pTreeable;
			else
				return NULL;
		}

		operator CData *() const
		{
			return GetCurrent();
		}

		CData * operator ->() const
		{
			return GetCurrent();
		}

		void operator ++()
		{
			Next();
		}
		
		void operator --()
		{
			Prev();
		}
	};
};

#endif // __MDA_AVLTREE


