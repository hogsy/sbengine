
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/

#ifndef __INC_MDA_LINKEDLIST
#define __INC_MDA_LINKEDLIST

#include "MMemMgrPool.h"

class MCCDLLEXPORT CDA_Linkable;
class MCCDLLEXPORT CDA_LinkableMember;
class MCCDLLEXPORT CDA_LinkedList;
class MCCDLLEXPORT CDA_LinkedListIterator;

extern CDA_Pool_Static LinkablePool;

class MCCDLLEXPORT CDA_LinkableMember : public CDA_Poolable
{
public:
	CDA_Linkable *m_pLinkable;
	CDA_LinkableMember *m_pLinkedList_Next;
	CDA_LinkableMember *m_pLinkedList_Prev;

	CDA_LinkedList *m_pLinkedList;
	CDA_LinkableMember *m_pLinkable_Next;
	CDA_LinkableMember *m_pLinkable_Prev;

	void Delete();

	CDA_LinkableMember();
	~CDA_LinkableMember();

};

class MCCDLLEXPORT CDA_LinkedListIterator
{
public:
	CDA_LinkableMember *m_pCurrentMember;

	CDA_LinkedList *m_pList;

	CDA_LinkedListIterator *m_pNext;
	CDA_LinkedListIterator *m_pPrev;

	void Destructor();
	void SetList(CDA_LinkedList *_pList);
	CDA_LinkedListIterator();
};


#define DA_LLI(ClassName) typedef TCDA_LinkedListIterator<ClassName> I##ClassName

template <class TMember> class TCDA_LinkedListIterator : public CDA_LinkedListIterator
{
public:
	M_INLINE TCDA_LinkedListIterator(CDA_LinkedList &_List)
	{
		SetList(&_List);
	};

	TCDA_LinkedListIterator()
	{
	};

	~TCDA_LinkedListIterator()
	{
		Destructor();
	};

	M_INLINE void operator ++ () 
	{
		if (m_pCurrentMember)
			m_pCurrentMember = m_pCurrentMember->m_pLinkedList_Next;

	};

	M_INLINE void operator -- () 
	{
		if (m_pCurrentMember)
			m_pCurrentMember = m_pCurrentMember->m_pLinkedList_Prev;
	};

	M_INLINE operator TMember* () 
	{
		if (m_pCurrentMember)
			return (TMember *)m_pCurrentMember->m_pLinkable;
		else
			return NULL;
	};

	M_INLINE TMember& operator* () 
	{ 
		if (m_pCurrentMember)
			return *((TMember *)m_pCurrentMember->m_pLinkable);
		else
			return NULL;
	};

	M_INLINE TMember* operator-> () 
	{ 
		if (m_pCurrentMember)
			return (TMember *)m_pCurrentMember->m_pLinkable; 
		else
			return NULL;
	};

	//TCDA_ReferenceType& 
	M_INLINE void operator= (CDA_LinkedList* _p) 
	{
		SetList(_p);
	};

	M_INLINE void operator= (CDA_LinkedList &_p) 
	{
		SetList(&_p);
	};

	M_INLINE void operator= (const TCDA_LinkedListIterator& _p)
	{
		SetList(_p.m_pCurrentMember->m_pList);
		m_pCurrentMember = _p.m_pCurrentMember;
	};


};


class MCCDLLEXPORT CDA_LinkedList : public CReferenceCount
{
public:

	CDA_LinkableMember *m_pFirstMember;
	CDA_LinkableMember *m_pLastMember;
	int m_NumMembers;

	CDA_LinkedListIterator *m_pFirstIterator;

	void DeleteMembers();
	void RemoveMembers();

	// Has to be overidden to enable sorting functions
	virtual int Compare(CDA_Linkable *_pFirst, CDA_Linkable *_pSecond);
	
	void MoveToLoc(CDA_LinkedList *_pNewLoc);

	void Sort();

	CDA_LinkedList();
	virtual ~CDA_LinkedList();

};


class MCCDLLEXPORT CDA_LinkedAutoDeleteList : public CDA_LinkedList
{
public:

	virtual ~CDA_LinkedAutoDeleteList();

};


class MCCDLLEXPORT CDA_Linkable
{
public:
	CDA_LinkableMember *m_pDA_Linkable_First;

	CDA_Linkable();
	virtual ~CDA_Linkable();
	
	void MoveToLoc(CDA_Linkable *_pNewLoc);

	void List_InsertAfter(CDA_LinkedList *_pList, CDA_Linkable* _pMember);
	void List_InsertAfter(CDA_LinkedList *_pList, CDA_LinkableMember* _pMember);
	void List_InsertSortedStartHead(CDA_LinkedList *_pList);
	void List_InsertSortedStartTail(CDA_LinkedList *_pList);
	void List_Swap(CDA_LinkedList *_pList, CDA_Linkable *_pMember);
	static void List_Swap(CDA_LinkableMember *_pFirst, CDA_LinkableMember *_pSecond);

	void List_Insert(CDA_LinkedList *_pList);
	void List_Insert(CDA_LinkedList &_pList);

	void List_Remove(CDA_LinkedList *_pList);
	void List_Remove(CDA_LinkedList &_pList);

	void List_RemoveFromAllLists();

	CDA_LinkableMember *List_GetMember(CDA_LinkedList &_pList);
	CDA_LinkableMember *List_GetMember(CDA_LinkedList *_pList);

};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Small linked list
|__________________________________________________________________________________________________
\*************************************************************************************************/

class MCCDLLEXPORT CDA_LinkedList_Small;
class MCCDLLEXPORT CDA_LinkedListIterator_Small;

class MCCDLLEXPORT CDA_LinkedList_Small
{
public:
	class MCCDLLEXPORT CDA_Link
	{
		friend class CDA_LinkedList_Small;
		friend class CDA_LinkedListIterator_Small;

	private:
		void *m_pLinkable;
		CDA_Link *m_pLinkedList_Next;
		CDA_Link *m_pLinkedList_Prev;

	public:
		CDA_Link();
		~CDA_Link();

		bool IsInList()
		{
			return m_pLinkable != 0;
		}

		void LinkSortedStartHead(CDA_LinkedList_Small *_pList, void *_pLinkable);
		void LinkSortedStartTail(CDA_LinkedList_Small *_pList, void *_pLinkable);

		M_INLINE void Link(CDA_LinkedList_Small &List, void *_pLinkable)
		{
			Link(&List, _pLinkable);
		}

		M_INLINE void Link(CDA_LinkedList_Small *List, void *_pLinkable)
		{
			Unlink();

			m_pLinkable = _pLinkable;

			// List
			m_pLinkedList_Next = &List->m_LastMember;
			m_pLinkedList_Prev = List->m_LastMember.m_pLinkedList_Prev;

			List->m_LastMember.m_pLinkedList_Prev->m_pLinkedList_Next = this;
			List->m_LastMember.m_pLinkedList_Prev = this;
		}

		M_INLINE void LinkLast(CDA_LinkedList_Small *_pList, void *_pLinkable)
		{
			Unlink();
			LinkAfter(_pList, _pList->m_LastMember.m_pLinkedList_Prev, _pLinkable);
		}

		M_INLINE void LinkLast(CDA_LinkedList_Small &_pList, void *_pLinkable)
		{
			Unlink();
			LinkAfter(&_pList, _pList.m_LastMember.m_pLinkedList_Prev, _pLinkable);
		}

		void LinkAfter(CDA_LinkedList_Small *_pList, CDA_Link* Member, void *_pLinkable)
		{
			M_ASSERT(Member != this, "Bad karma");
			Unlink();

			m_pLinkable = _pLinkable;

			// List
			if (!Member)
				Member = &_pList->m_FirstMember;

			m_pLinkedList_Prev = Member;
			m_pLinkedList_Next = Member->m_pLinkedList_Next;

			Member->m_pLinkedList_Next->m_pLinkedList_Prev = this;
			Member->m_pLinkedList_Next = this;
		}

		M_INLINE void Unlink()
		{
			if (m_pLinkable)
			{
				M_ASSERT(m_pLinkedList_Next && m_pLinkedList_Prev, "Not happy");

				if (m_pLinkedList_Next)
					m_pLinkedList_Next->m_pLinkedList_Prev = m_pLinkedList_Prev;

				if (m_pLinkedList_Prev)
					m_pLinkedList_Prev->m_pLinkedList_Next = m_pLinkedList_Next;

				m_pLinkable = NULL;
			}
		}


		M_INLINE void Swap(CDA_Link *Member)
		{
			Swap(this, Member);
		}

		M_INLINE void Swap(CDA_Link *First, CDA_Link *Second)
		{
			CDA_Link *p1 = First->m_pLinkedList_Prev;
			CDA_Link *p4 = Second->m_pLinkedList_Next;

			p1->m_pLinkedList_Next = Second;
			Second->m_pLinkedList_Next = First;
			First->m_pLinkedList_Next = p4;

			Second->m_pLinkedList_Prev = p1;
			First->m_pLinkedList_Prev = Second;
			p4->m_pLinkedList_Prev = First;
		}

	};

	// Contains a first and last link from the start
	CDA_Link m_FirstMember;
	CDA_Link m_LastMember;

	void DeleteMembers();
	void RemoveMembers();

	// Has to be overidden to enable sorting functions
	virtual int Compare(void *_pFirst, void *_pSecond);

	// Has to be overidden to support deleting of members
	virtual void DeleteElement(void *_pElement);
	
	void Sort();
	
	bool IsEmpty();

	CDA_LinkedList_Small();
	virtual ~CDA_LinkedList_Small();

};

typedef CDA_LinkedList_Small::CDA_Link CDA_Link;

class MCCDLLEXPORT CDA_LinkedListIterator_Small
{
public:
	CDA_Link *m_pCurrentMember;

	void SetList(CDA_LinkedList_Small *_pList);
	CDA_LinkedListIterator_Small();

	M_INLINE CDA_Link *Next(CDA_Link *_pSource)
	{
		if (!_pSource->m_pLinkedList_Next->m_pLinkedList_Next)
			return NULL;
		return _pSource->m_pLinkedList_Next;
	};

	M_INLINE CDA_Link *Prev(CDA_Link *_pSource)
	{
		if (!_pSource->m_pLinkedList_Prev->m_pLinkedList_Prev)
			return NULL;
		return _pSource->m_pLinkedList_Prev;
	};

	M_INLINE void *Linkable(CDA_Link *_pSource)
	{
		return _pSource->m_pLinkable;
	};
};


#define DA_LLSI(ClassName) typedef TCDA_LinkedListIterator_Small<ClassName> SI##ClassName

template <class TMember> class TCDA_LinkedListIterator_Small : public CDA_LinkedListIterator_Small
{
public:
	M_INLINE TCDA_LinkedListIterator_Small(CDA_LinkedList_Small &_List)
	{
		SetList(&_List);
	};

	TCDA_LinkedListIterator_Small()
	{
	};

	M_INLINE void operator ++ () 
	{
		if (m_pCurrentMember)
			m_pCurrentMember = Next(m_pCurrentMember);

	};

	M_INLINE void operator -- () 
	{
		if (m_pCurrentMember)
			m_pCurrentMember = Prev(m_pCurrentMember);
	};

	M_INLINE operator TMember* () 
	{
		if (m_pCurrentMember)
			return (TMember *)Linkable(m_pCurrentMember);
		else
			return NULL;
	};

	M_INLINE TMember& operator* () 
	{ 
		if (m_pCurrentMember)
			return *((TMember *)Linkable(m_pCurrentMember));
		else
			return NULL;
	};

	M_INLINE TMember* operator-> () 
	{ 
		if (m_pCurrentMember)
			return (TMember *)Linkable(m_pCurrentMember); 
		else
			return NULL;
	};

	//TCDA_ReferenceType& 
	M_INLINE void operator= (CDA_LinkedList_Small* _p) 
	{
		SetList(_p);
	};

	M_INLINE void operator= (CDA_LinkedList_Small &_p) 
	{
		SetList(&_p);
	};

	M_INLINE void operator= (const TCDA_LinkedListIterator_Small& _p)
	{
		SetList(_p.m_pCurrentMember->m_pList);
		m_pCurrentMember = _p.m_pCurrentMember;
	};


};


class MCCDLLEXPORT CDA_LinkedAutoDeleteList_Small : public CDA_LinkedList
{
public:

	virtual ~CDA_LinkedAutoDeleteList_Small();

};


#endif //__INC_MDA_LINKEDLIST
