
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

#include "MCC.h"


CDA_Stack::CDA_Stack()
{

}

CDA_Stack::~CDA_Stack()
{

}

void CDA_Stack::Push(CDA_Linkable *Object)
{
	Object->List_InsertAfter(&List,(CDA_Linkable *)NULL);
}

CDA_Linkable *CDA_Stack::Get()
{
	if (List.m_pFirstMember)
		return List.m_pFirstMember->m_pLinkable;
	else
		return NULL;
}

void CDA_Stack::Pop()
{
	if (List.m_pFirstMember)
		List.m_pFirstMember->Delete();
}


CDA_CircularStack::CDA_CircularStack()
{

}

CDA_CircularStack::~CDA_CircularStack()
{

}

void CDA_CircularStack::Add(CDA_Linkable *Object)
{
	Object->List_InsertAfter(&List,(CDA_LinkableMember *)NULL);
}

void CDA_CircularStack::Remove(CDA_Linkable *Object)
{
	Object->List_Remove(&List);
}

CDA_Linkable *CDA_CircularStack::Get()
{
	if (List.m_pFirstMember)
		return List.m_pFirstMember->m_pLinkable;
	else
		return NULL;
}

void CDA_CircularStack::Next()
{
	if (List.m_pFirstMember)
	{

		CDA_Linkable *m_pLinkable = List.m_pFirstMember->m_pLinkable;
		deleteP(List.m_pFirstMember, &LinkablePool);

		m_pLinkable->List_InsertAfter(&List, List.m_pLastMember);
	}
}

void CDA_CircularStack::Prev()
{
	
	if (List.m_pLastMember)
	{
		CDA_Linkable *m_pLinkable = List.m_pLastMember->m_pLinkable;
		deleteP(List.m_pLastMember, &LinkablePool);

		m_pLinkable->List_InsertAfter(&List, (CDA_LinkableMember *)NULL);
	}
}
