#ifndef WDYNAMICSENGINEBASE_H
#define WDYNAMICSENGINEBASE_H

#include "PCH.h"

#if 0
template <typename T>
class CSingleLinkable
{
public:
	CSingleLinkable<T> *m_pNext;
	T m_Value;

	void Add(CSingleLinkable<T> **_ppHead)
	{
		m_pNext = *_pHead;
		*_ppHead = this;
	}
};
#endif

template <typename T>
class TDisjointSet
{
public:

	struct Node
	{
		Node *pNext;
		Node *pHead;
		Node *pTail;
		int Size;
		T Value;
	};

	M_INLINE static void MakeSet(Node *_pNode)
	{
		_pNode->pHead = _pNode;
		_pNode->pTail = _pNode;
		_pNode->pNext = NULL;
		_pNode->Size = 1;
	}

	M_INLINE static Node *FindSet(Node *_pNode)
	{
		return _pNode->pHead;
	}

	M_INLINE static int Size(Node *_pNode)
	{
		return _pNode->Size;
	}

	M_INLINE static void Union(Node *_pX, Node *_pY)
	{
		Node *pSetX = FindSet(_pX);
		Node *pSetY = FindSet(_pY);

		if (Size(pSetX) < Size(pSetY))
		{
			Node *pTmp = pSetX;
			pSetX = pSetY;
			pSetY = pTmp;
		}

		pSetX->pTail->pNext = pSetY;
		pSetX->pTail = pSetY->pTail;
		pSetX->Size = pSetX->Size + pSetY->Size;

		while (pSetY != NULL)
		{
			pSetY->pHead = pSetX;
			pSetY = pSetY->pNext;
		}
	}


};

template <typename T> class CDoubleLinkable;

template <typename T>
class CDoubleLinkedList
{
public:
	CDoubleLinkable<T> *m_pFirst, *m_pLast;

	M_INLINE CDoubleLinkedList()
	{
		m_pFirst = NULL;
		m_pLast = NULL;
	}

};

template <typename T>
class CDoubleLinkable
{
public:
	CDoubleLinkable<T> *m_pNext, *m_pPrev, *m_pFirst, *m_pLast;
	CDoubleLinkedList<T> **m_ppList;

	/*
	union
	{
		CDoubleLinkable<T> *m_pNext, *m_pLast;
	};

	union
	{
		CDoubleLinkable<T> *m_pPrev, *m_pFirst;
	};

	CDoubleLinkable<T> **m_pHead;
*/
	T m_Value;

	CDoubleLinkable()
	{
		m_pNext = NULL;
		m_pPrev = NULL;
		m_pFirst = this;
		m_pLast = this;
		m_ppList = NULL;
	}

	M_FORCEINLINE bool IsEmpty()
	{
		return m_pNext == NULL && m_pPrev == NULL;
	}

	M_FORCEINLINE void Insert(CDoubleLinkable<T> *_pLinkable)
	{
		if (m_pNext)
		{
			m_pNext->m_pPrev = _pLinkable;
		}

		_pLinkable->m_pNext = m_pNext;
		m_pNext = _pLinkable;
		_pLinkable->m_pPrev = this;
	}

	M_FORCEINLINE void Concat(CDoubleLinkable<T> *_pLinkable)
	{
		if (m_pFirst == _pLinkable->m_pFirst) return;

		m_pLast->m_pNext = _pLinkable->m_pFirst;
		_pLinkable->m_pFirst->m_pPrev = m_pLast;
		
		_pLinkable->m_pFirst = m_pFirst;
		m_pLast = _pLinkable->m_pLast;
	}
};


#endif
