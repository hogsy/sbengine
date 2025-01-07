
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

#include "MDA.h"
#include "MDA_LinkedList.h"



//DA_DLLEXPORT CDA_Pool LinkablePool(sizeof(CDA_LinkableMember), 65536);

#ifdef PLATFORM_CONSOLE
MDA_StaticPool(LinkablePool,sizeof(CDA_LinkableMember), 4096)
#else
MDA_StaticPool(LinkablePool,sizeof(CDA_LinkableMember), 65536)
#endif


CDA_LinkableMember::CDA_LinkableMember()
{
	m_pLinkable = NULL;
	m_pLinkedList_Next = NULL;
	m_pLinkedList_Prev = NULL;

	m_pLinkedList = NULL;
	m_pLinkable_Next = NULL;
	m_pLinkable_Prev = NULL;
}

CDA_LinkableMember::~CDA_LinkableMember()
{
	M_ASSERT(m_pLinkable, "Måste");
	{
		if (m_pLinkable_Next)
			m_pLinkable_Next->m_pLinkable_Prev = m_pLinkable_Prev;

		if (m_pLinkable_Prev)
			m_pLinkable_Prev->m_pLinkable_Next = m_pLinkable_Next;
		else 
			m_pLinkable->m_pDA_Linkable_First = m_pLinkable_Next;

	}

	M_ASSERT(m_pLinkedList, "Måste");
	{
		CDA_LinkedListIterator *CurrentIterator = m_pLinkedList->m_pFirstIterator;
		
		while (CurrentIterator)
		{
			if (CurrentIterator->m_pCurrentMember == this)
				CurrentIterator->m_pCurrentMember = CurrentIterator->m_pCurrentMember->m_pLinkedList_Next;

			CurrentIterator = CurrentIterator->m_pNext;
		}

		if (m_pLinkedList_Next)
			m_pLinkedList_Next->m_pLinkedList_Prev = m_pLinkedList_Prev;
		else
			m_pLinkedList->m_pLastMember = m_pLinkedList_Prev;

		if (m_pLinkedList_Prev)
			m_pLinkedList_Prev->m_pLinkedList_Next = m_pLinkedList_Next;
		else 
			m_pLinkedList->m_pFirstMember = m_pLinkedList_Next;

		--m_pLinkedList->m_NumMembers;
	}
}

void CDA_LinkableMember::Delete()
{
	deleteP(this, &LinkablePool);
}


CDA_Linkable::CDA_Linkable()
{

	m_pDA_Linkable_First = NULL;

}

CDA_Linkable::~CDA_Linkable()
{

	while (m_pDA_Linkable_First)
		deleteP(m_pDA_Linkable_First, &LinkablePool);

}


void CDA_Linkable::List_RemoveFromAllLists()
{
	while (m_pDA_Linkable_First)
		deleteP(m_pDA_Linkable_First, &LinkablePool);
}

void CDA_Linkable::List_Insert(CDA_LinkedList &List)
{
	List_Insert(&List);
}

void CDA_Linkable::List_Remove(CDA_LinkedList &List)
{
	List_Remove(&List);
}

CDA_LinkableMember *CDA_Linkable::List_GetMember(CDA_LinkedList &List)
{
	return List_GetMember(&List);
}

CDA_LinkableMember *CDA_Linkable::List_GetMember(CDA_LinkedList *List)
{

	CDA_LinkableMember *Current = m_pDA_Linkable_First;

	while (Current)
	{

		if (Current->m_pLinkedList == List)
			return Current;

		Current = Current->m_pLinkable_Next;
	}

	return NULL;
}


void CDA_Linkable::List_Insert(CDA_LinkedList *List)
{
	M_ASSERT(!List_GetMember(List), "Make sure its not already in list");

//	CDA_LinkableMember *Current = new(&LinkablePool) CDA_LinkableMember;
	CDA_LinkableMember *Current = new(&LinkablePool) CDA_LinkableMember;

	// Linkabel
	Current->m_pLinkable = this;
	Current->m_pLinkable_Prev = NULL;

	if (m_pDA_Linkable_First)
		m_pDA_Linkable_First->m_pLinkable_Prev = Current;

	Current->m_pLinkable_Next = m_pDA_Linkable_First;
	m_pDA_Linkable_First = Current;

	// List
	Current->m_pLinkedList = List;
	Current->m_pLinkedList_Next = NULL;

	if (List->m_pLastMember)
		List->m_pLastMember->m_pLinkedList_Next = Current;
	else
		List->m_pFirstMember = Current;

	Current->m_pLinkedList_Prev = List->m_pLastMember;
	List->m_pLastMember = Current;
	++List->m_NumMembers;
}

void CDA_Linkable::List_InsertAfter(CDA_LinkedList *List, CDA_Linkable* Member)
{
	M_ASSERT(!List_GetMember(List), "Make sure its not already in list");
	
	CDA_LinkableMember *Current = new(&LinkablePool) CDA_LinkableMember;
	
	// Linkabel
	Current->m_pLinkable = this;
	Current->m_pLinkable_Prev = NULL;
	
	if (m_pDA_Linkable_First)
		m_pDA_Linkable_First->m_pLinkable_Prev = Current;
	
	Current->m_pLinkable_Next = m_pDA_Linkable_First;
	m_pDA_Linkable_First = Current;
	
	// List
	if (Member)
	{		
		CDA_LinkableMember *InsertAfterMember = Member->List_GetMember(List);
		
		M_ASSERT(InsertAfterMember, "Make sure its the same list");

		Current->m_pLinkedList = List;
		Current->m_pLinkedList_Prev = InsertAfterMember;
		Current->m_pLinkedList_Next = InsertAfterMember->m_pLinkedList_Next;

		if (InsertAfterMember->m_pLinkedList_Next)
			InsertAfterMember->m_pLinkedList_Next->m_pLinkedList_Prev = Current;
		else
			List->m_pLastMember = Current;

		InsertAfterMember->m_pLinkedList_Next = Current;

	}
	else
	{		
		Current->m_pLinkedList = List;
		Current->m_pLinkedList_Prev = NULL;
		
		if (List->m_pFirstMember)
			List->m_pFirstMember->m_pLinkedList_Prev = Current;
		else
			List->m_pLastMember = Current;
		
		Current->m_pLinkedList_Next = List->m_pFirstMember;
		List->m_pFirstMember = Current;
	}

	++List->m_NumMembers;
}

void CDA_Linkable::List_InsertAfter(CDA_LinkedList *List, CDA_LinkableMember* Member)
{
	M_ASSERT(!List_GetMember(List), "Make sure its not already in list");
	
	CDA_LinkableMember *Current = new(&LinkablePool) CDA_LinkableMember;
	
	// Linkabel
	Current->m_pLinkable = this;
	Current->m_pLinkable_Prev = NULL;
	
	if (m_pDA_Linkable_First)
		m_pDA_Linkable_First->m_pLinkable_Prev = Current;
	
	Current->m_pLinkable_Next = m_pDA_Linkable_First;
	m_pDA_Linkable_First = Current;
	
	// List
	if (Member)
	{		
		M_ASSERT(Member->m_pLinkedList == List, "List missmatch.");
		Current->m_pLinkedList = List;
		Current->m_pLinkedList_Prev = Member;
		Current->m_pLinkedList_Next = Member->m_pLinkedList_Next;

		if (Member->m_pLinkedList_Next)
			Member->m_pLinkedList_Next->m_pLinkedList_Prev = Current;
		else
			List->m_pLastMember = Current;

		Member->m_pLinkedList_Next = Current;

	}
	else
	{		
		Current->m_pLinkedList = List;
		Current->m_pLinkedList_Prev = NULL;
		
		if (List->m_pFirstMember)
			List->m_pFirstMember->m_pLinkedList_Prev = Current;
		else
			List->m_pLastMember = Current;
		
		Current->m_pLinkedList_Next = List->m_pFirstMember;
		List->m_pFirstMember = Current;
	}

	++List->m_NumMembers;
}

void CDA_Linkable::MoveToLoc(CDA_Linkable *NewLoc)
{

	*NewLoc = *this;

	CDA_LinkableMember *Current = NewLoc->m_pDA_Linkable_First;

	while (Current)
	{
		Current->m_pLinkable = this;

		Current = Current->m_pLinkable_Next;
	}

}

void CDA_Linkable::List_Swap(CDA_LinkedList *List, CDA_Linkable *Member)
{
	CDA_LinkableMember *First = List_GetMember(List);
	CDA_LinkableMember *Second = Member->List_GetMember(List);

	List_Swap(First, Second);
}

void CDA_Linkable::List_Swap(CDA_LinkableMember *First, CDA_LinkableMember *Second)
{
	CDA_LinkableMember *Temp = First->m_pLinkedList_Prev;
	First->m_pLinkedList_Prev = Second->m_pLinkedList_Prev;
	Second->m_pLinkedList_Prev = Temp;

	Temp = First->m_pLinkedList_Next;
	First->m_pLinkedList_Next = Second->m_pLinkedList_Next;
	Second->m_pLinkedList_Next = Temp;

	// Fix last and first pointers

	if (!First->m_pLinkedList_Prev)
		First->m_pLinkedList->m_pFirstMember = First;
	else if (!Second->m_pLinkedList_Prev)
		Second->m_pLinkedList->m_pFirstMember = Second;

	if (!First->m_pLinkedList_Next)
		First->m_pLinkedList->m_pLastMember = First;
	else if (!Second->m_pLinkedList_Next)
		Second->m_pLinkedList->m_pLastMember = Second;
}

void CDA_Linkable::List_InsertSortedStartHead(CDA_LinkedList *List)
{
	CDA_LinkableMember *Current = List->m_pFirstMember;

	CDA_LinkableMember *InsertAfther = NULL;

	while (Current)
	{
		// Stop after equal
		if (List->Compare(this, Current->m_pLinkable) > 0)
			break;

		InsertAfther = Current;
		Current = Current->m_pLinkedList_Next;
	}
	
	List_InsertAfter(List, InsertAfther);
}

void CDA_Linkable::List_InsertSortedStartTail(CDA_LinkedList *List)
{
	CDA_LinkableMember *Current = List->m_pLastMember;

	while (Current)
	{
		// Stop before equal
		if (List->Compare(this, Current->m_pLinkable) <= 0)
			break;

		Current = Current->m_pLinkedList_Prev;
	}
	
	List_InsertAfter(List, Current);
}

void CDA_Linkable::List_Remove(CDA_LinkedList *List)
{
	CDA_LinkableMember *Current = List_GetMember(List);

	if (Current)
		deleteP(Current, &LinkablePool);
}




CDA_LinkedList::CDA_LinkedList()
{

	m_pFirstIterator = NULL;
	m_pFirstMember = NULL;
	m_pLastMember = NULL;
	m_NumMembers = 0;

}

CDA_LinkedList::~CDA_LinkedList()
{
	while (m_pFirstMember)
		deleteP(m_pFirstMember, &LinkablePool);

	while (m_pFirstIterator)
		m_pFirstIterator->SetList(NULL);
}

void CDA_LinkedList::DeleteMembers()
{
	
	while (m_pFirstMember)
		delete m_pFirstMember->m_pLinkable;

}


void CDA_LinkedList::RemoveMembers()
{
	while (m_pFirstMember)
		deleteP(m_pFirstMember, &LinkablePool);
}

CDA_LinkedAutoDeleteList::~CDA_LinkedAutoDeleteList()
{
	DeleteMembers();
}


int CDA_LinkedList::Compare(CDA_Linkable *First, CDA_Linkable *Second)
{
	M_ASSERT(0, "You have to overide this function to enable sorting");

	return 0;
}
	

// N to (N^2)/2
void CDA_LinkedList::Sort()
{

	CDA_LinkableMember *FirstInNewList = NULL;
	CDA_LinkableMember *LastInNewList = NULL;

	CDA_LinkableMember *Current;
	CDA_LinkableMember *CurrentInNewList;

	//DWORD CurrentInNewList;

	//DWORD Current;

	// Loop through all in list
	while (m_pFirstMember)
	{

		Current = m_pFirstMember;
		// Search back to front to ptomote fast sorting of already sorted lists
		CurrentInNewList = LastInNewList;

		while (CurrentInNewList)
		{
			if (Compare(CurrentInNewList->m_pLinkable, Current->m_pLinkable)<=0)
				break;
			CurrentInNewList = CurrentInNewList->m_pLinkedList_Prev;
		}

		// First remove from old list

		if (Current->m_pLinkedList_Next)
			Current->m_pLinkedList_Next->m_pLinkedList_Prev = Current->m_pLinkedList_Prev;
		else
			m_pLastMember = Current->m_pLinkedList_Prev;

		if (Current->m_pLinkedList_Prev)
			Current->m_pLinkedList_Prev->m_pLinkedList_Next = Current->m_pLinkedList_Next;
		else
			m_pFirstMember = Current->m_pLinkedList_Next;

		if (CurrentInNewList)
		{
			// Insert after spot
			Current->m_pLinkedList_Next = CurrentInNewList->m_pLinkedList_Next;
			
			if (CurrentInNewList->m_pLinkedList_Next)
				CurrentInNewList->m_pLinkedList_Next->m_pLinkedList_Prev = Current;
			else
				LastInNewList = Current;
			
			CurrentInNewList->m_pLinkedList_Next = Current;
			
			Current->m_pLinkedList_Prev = CurrentInNewList;

		}
		else
		{
			// Insert first in list
			
			Current->m_pLinkedList_Prev = NULL;

			if (FirstInNewList)
				FirstInNewList->m_pLinkedList_Prev = Current;
			else
				LastInNewList = Current;
			
			Current->m_pLinkedList_Next = FirstInNewList;
			FirstInNewList = Current;
		}		

	}

	m_pFirstMember = FirstInNewList;
	m_pLastMember = LastInNewList;
	
}


void CDA_LinkedListIterator::Destructor()
{
	if (m_pList)
		SetList(NULL);
}

void CDA_LinkedListIterator::SetList(CDA_LinkedList *_List)
{
	if (m_pList)
	{
		if (m_pNext)
			m_pNext->m_pPrev = m_pPrev;
		
		if (m_pPrev)
			m_pPrev->m_pNext = m_pNext;
		else
			m_pList->m_pFirstIterator = m_pNext;
	}
	
	m_pList = _List;
	
	if (m_pList)
	{
		m_pPrev = NULL;
		
		if (m_pList->m_pFirstIterator)
			m_pList->m_pFirstIterator->m_pPrev = this;
		
		m_pNext = m_pList->m_pFirstIterator;
		
		m_pList->m_pFirstIterator = this;
		
		m_pCurrentMember = m_pList->m_pFirstMember;
	}
	else
	{
		m_pCurrentMember = NULL;
	}
}

CDA_LinkedListIterator::CDA_LinkedListIterator()
{
	m_pCurrentMember = NULL;
	m_pList = NULL;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Small linked list
|__________________________________________________________________________________________________
\*************************************************************************************************/

CDA_Link::CDA_Link()
{
	m_pLinkable = NULL;
	m_pLinkedList_Next = NULL;
	m_pLinkedList_Prev = NULL;
}

CDA_Link::~CDA_Link()
{
	Unlink();

}


void CDA_Link::LinkSortedStartHead(CDA_LinkedList_Small *List, void *_pLinkable)
{
	Unlink();
	CDA_Link *Current = List->m_FirstMember.m_pLinkedList_Next;

	CDA_Link *InsertAfter = NULL;

	while (Current->m_pLinkable)
	{
		// Stop after equal
		if (List->Compare(_pLinkable, Current->m_pLinkable) > 0)
			break;

		InsertAfter = Current;
		Current = Current->m_pLinkedList_Next;
	}
	
	LinkAfter(List, InsertAfter, _pLinkable);
}

void CDA_Link::LinkSortedStartTail(CDA_LinkedList_Small *List, void *_pLinkable)
{
	Unlink();
	CDA_Link *Current = List->m_LastMember.m_pLinkedList_Prev;

	while (Current->m_pLinkable)
	{
		// Stop before equal
		if (List->Compare(_pLinkable, Current->m_pLinkable) <= 0)
			break;

		Current = Current->m_pLinkedList_Prev;
	}
	
	LinkAfter(List, Current, _pLinkable);
}



CDA_LinkedList_Small::CDA_LinkedList_Small()
{
	m_FirstMember.m_pLinkedList_Next = &m_LastMember;
	m_LastMember.m_pLinkedList_Prev = &m_FirstMember;
}

CDA_LinkedList_Small::~CDA_LinkedList_Small()
{
	while (m_FirstMember.m_pLinkedList_Next->m_pLinkable)
		m_FirstMember.m_pLinkedList_Next->Unlink();
}


bool CDA_LinkedList_Small::IsEmpty()
{
	return m_FirstMember.m_pLinkedList_Next == &m_LastMember;
}

void CDA_LinkedList_Small::DeleteMembers()
{
	while (m_FirstMember.m_pLinkedList_Next->m_pLinkable)
		DeleteElement(m_FirstMember.m_pLinkedList_Next->m_pLinkable);
}


void CDA_LinkedList_Small::RemoveMembers()
{
	while (m_FirstMember.m_pLinkedList_Next->m_pLinkable)
		m_FirstMember.m_pLinkedList_Next->Unlink();
}

CDA_LinkedAutoDeleteList_Small::~CDA_LinkedAutoDeleteList_Small()
{
	DeleteMembers();
}


int CDA_LinkedList_Small::Compare(void *First, void *Second)
{
	M_ASSERT(0, "You have to overide this function to enable sorting");

	return 0;
}
	
void CDA_LinkedList_Small::DeleteElement(void *_pElement)
{
	Error_static("CDA_LinkedList_Small::DeleteElement", "You have to overide this function to enable deleting");
}
	

// N to (N^2)/2
void CDA_LinkedList_Small::Sort()
{

	CDA_Link FirstInNewList;
	CDA_Link LastInNewList;

	FirstInNewList.m_pLinkedList_Next = &LastInNewList;
	LastInNewList.m_pLinkedList_Prev = &FirstInNewList;

	CDA_Link *Current;
	CDA_Link *CurrentInNewList;

	// Loop through all in list
	while (m_FirstMember.m_pLinkedList_Next->m_pLinkable)
	{

		Current = m_FirstMember.m_pLinkedList_Next;
		// Search back to front to ptomote fast sorting of already sorted lists
		CurrentInNewList = LastInNewList.m_pLinkedList_Prev;

		while (CurrentInNewList->m_pLinkedList_Prev && CurrentInNewList->m_pLinkable)
		{
			if (Compare(CurrentInNewList->m_pLinkable, Current->m_pLinkable)<=0)
				break;
			CurrentInNewList = CurrentInNewList->m_pLinkedList_Prev;
		}


		Current->m_pLinkedList_Next->m_pLinkedList_Prev = Current->m_pLinkedList_Prev;
		Current->m_pLinkedList_Prev->m_pLinkedList_Next = Current->m_pLinkedList_Next;

		// First remove from old list
		// List
		Current->m_pLinkedList_Prev = CurrentInNewList;
		Current->m_pLinkedList_Next = CurrentInNewList->m_pLinkedList_Next;

		CurrentInNewList->m_pLinkedList_Next->m_pLinkedList_Prev = Current;
		CurrentInNewList->m_pLinkedList_Next = Current;

	}

	m_FirstMember = FirstInNewList;
	m_FirstMember.m_pLinkedList_Next->m_pLinkedList_Prev = &m_FirstMember;
	m_LastMember = LastInNewList;
	m_LastMember.m_pLinkedList_Prev->m_pLinkedList_Next = &m_LastMember;

}


void CDA_LinkedListIterator_Small::SetList(CDA_LinkedList_Small *_List)
{
	if (_List)
	{
		m_pCurrentMember = _List->m_FirstMember.m_pLinkedList_Next;
		if (!m_pCurrentMember->m_pLinkedList_Next)
			m_pCurrentMember = NULL;
	}
	else
	{
		m_pCurrentMember = NULL;
	}
}

CDA_LinkedListIterator_Small::CDA_LinkedListIterator_Small()
{
	m_pCurrentMember = NULL;
}

