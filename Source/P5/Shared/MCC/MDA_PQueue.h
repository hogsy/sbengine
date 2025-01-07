
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/

#ifndef _INC_MPQUEUE
#define _INC_MPQUEUE

//#include "MCC.h"


// -------------------------------------------------------------------
//  TPriorityQueue2
// -------------------------------------------------------------------
// A priority-queue based on a binary heap.
// No memory allocation except on creation.

template<class TTYPE, typename TCONTAINER>
class TPriorityQueue2Base
{
	// The class T must implement: int GetPriority() const
protected:
	static int IndexRoot() { return 1; };
	static int IndexParent(int i) { return i >> 1; };
	static int IndexLeft(int i) { return i*2; };
	static int IndexRight(int i) { return i*2+1; };

	bool m_bIsAscendingHeap;
	int m_CurrentSize;
	int m_MaxElem;
	TCONTAINER m_lpElem;
	TTYPE** m_ppElem;

public:
	TPriorityQueue2Base();
	void Create(bool _bAscending, int _MaxElements);
	bool IsFull() const;
	bool IsEmpty() const;

	bool Push(TTYPE* _pElem);
	TTYPE* Pop();
};

#ifndef PLATFORM_VPU
	template<class T> class TPriorityQueue2 : public TPriorityQueue2Base<T, TArray<T*> > {};
#endif

// -------------------------------------------------------------------
//  TPriorityQueue2, Implementation
// -------------------------------------------------------------------
template<class TTYPE, typename TCONTAINER>
TPriorityQueue2Base<TTYPE,TCONTAINER>::TPriorityQueue2Base()
{
	m_bIsAscendingHeap = true;
	m_CurrentSize = 0;
}

template<class TTYPE, typename TCONTAINER>
void TPriorityQueue2Base<TTYPE,TCONTAINER>::Create(bool _bAscending, int _MaxElem)
{
	m_MaxElem = _MaxElem;
	m_bIsAscendingHeap = _bAscending;
	m_lpElem.SetLen(_MaxElem + 1); // +1 because the first push doesn't seem to use index 0 - JA
	m_ppElem = m_lpElem.GetBasePtr();
	//FillChar(m_lpElem.GetBasePtr(), m_lpElem.ListSize(), 0);	// Not necessary, but we don't like to have random data around.
	m_CurrentSize = 0;
}

template<class TTYPE, typename TCONTAINER>
bool TPriorityQueue2Base<TTYPE,TCONTAINER>::IsFull() const
{
	return (m_CurrentSize == m_MaxElem) ? true : false;
}

template<class TTYPE, typename TCONTAINER>
bool TPriorityQueue2Base<TTYPE,TCONTAINER>::IsEmpty() const
{
	return (m_CurrentSize) ? false : true;
}

template<class TTYPE, typename TCONTAINER>
bool TPriorityQueue2Base<TTYPE,TCONTAINER>::Push(TTYPE* _pElem)
{
	if(IsFull())
	{
		return false;
	}
	else
	{
		// Allocate and store element.
		TTYPE* pElem = _pElem;

		m_CurrentSize++;
		int i = m_CurrentSize;
		int Priority = _pElem->GetPriority();

		if(m_bIsAscendingHeap)
		{
			while(
				(i > IndexRoot()) &&
				(m_ppElem[IndexParent(i)]->GetPriority() < Priority))
			{
				m_ppElem[i] = m_ppElem[IndexParent(i)];
				i = IndexParent(i);
			}
		}
		else
		{
			while(
				(i > IndexRoot()) &&
				(m_ppElem[IndexParent(i)]->GetPriority() > Priority))
			{
				m_ppElem[i] = m_ppElem[IndexParent(i)];
				i = IndexParent(i);
			}
		}

		m_ppElem[i] = pElem;
		return true;
	}
}

template<class TTYPE, typename TCONTAINER>
TTYPE* TPriorityQueue2Base<TTYPE,TCONTAINER>::Pop()
{
	int i;

//	void *pMaxElement;
//	void *pLastElement;
	 
	if(IsEmpty())
	{
		return NULL;
	}

	TTYPE* pMaxElem = m_ppElem[IndexRoot()];
	TTYPE* pLastElem = m_ppElem[m_CurrentSize--];

	if(m_bIsAscendingHeap)
	{
		int iChild;
		for(i = IndexRoot(); IndexLeft(i) <= m_CurrentSize; i = iChild)
	    {
	    	iChild = IndexLeft(i);
	    	if( (iChild != m_CurrentSize) &&
				(m_ppElem[iChild + 1]->GetPriority() > m_ppElem[iChild]->GetPriority()) ) iChild++;
	    
	    	if(pLastElem->GetPriority() < m_ppElem[iChild]->GetPriority() )
				m_ppElem[i] = m_ppElem[iChild];
	    	else
	    		break;
	    }
	}
	else
	{
		int iChild;
		for(i = IndexRoot(); IndexLeft(i) <= m_CurrentSize; i = iChild)
	    {
	    	iChild = IndexLeft(i); 
	    	if( (iChild != m_CurrentSize) &&
				(m_ppElem[iChild + 1]->GetPriority() < m_ppElem[iChild]->GetPriority()) ) iChild++;
	    
	    	if(pLastElem->GetPriority() > m_ppElem[iChild]->GetPriority() )
				m_ppElem[i] = m_ppElem[iChild];
	    	else
	    		break;
	    }
	}

	m_ppElem[i] = pLastElem;
    return pMaxElem;
}

#endif // _INC_MPQUEUE
