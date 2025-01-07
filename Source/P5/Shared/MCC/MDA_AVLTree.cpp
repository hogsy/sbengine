
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



#ifdef PLATFORM_CONSOLE
	MDA_StaticPool(TreeablePool,sizeof(CDA_AVLTreeableMember), 4096)
#else
	MDA_StaticPool(TreeablePool,sizeof(CDA_AVLTreeableMember), 65536)
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDA_AVLTreeableMember
|__________________________________________________________________________________________________
\*************************************************************************************************/

CDA_AVLTreeableMember::CDA_AVLTreeableMember(CDA_AVLTree* _pTree, CDA_AVLTreeable *_pTreable)
{
	m_pLeft = NULL;
	m_pRight = NULL;
	m_pTree = NULL;
	m_Skew = AVLSKEW_NONE;

	m_pTreeable = NULL;

	Link(_pTreable);

	if (_pTree->Insert(&_pTree->m_pRoot, this) != AVLRES_ERROR)
	{
		M_ASSERT(m_pTree == _pTree, "Error inserting member in tree");
	}
}

CDA_AVLTreeableMember::~CDA_AVLTreeableMember()
{
}

void CDA_AVLTreeableMember::Link(CDA_AVLTreeable *_pTreeable)
{
	if (m_pTreeable)
	{
		if (m_pNextTreaable)
			m_pNextTreaable->m_Folded_m_pPrevTreeable = m_Folded_m_pPrevTreeable;
		
		if (m_Folded_m_pPrevTreeable)
			GetPrevTreaable()->m_pNextTreaable = m_pNextTreaable;
		else
			m_pTreeable->DA__m_pFirstTreeable = m_pNextTreaable;
	}
	
	m_pTreeable = _pTreeable;
	
	if (m_pTreeable)
	{

		m_Folded_m_pPrevTreeable = 0;

		if (_pTreeable->DA__m_pFirstTreeable)
			_pTreeable->DA__m_pFirstTreeable->SetPrevTreaable(this);
		
		m_pNextTreaable = _pTreeable->DA__m_pFirstTreeable;
		_pTreeable->DA__m_pFirstTreeable = this;
	}

}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDA_AVLTreeable
|__________________________________________________________________________________________________
\*************************************************************************************************/


CDA_AVLTreeable::CDA_AVLTreeable()
{
	DA__m_pFirstTreeable = NULL;
}

CDA_AVLTreeable::~CDA_AVLTreeable()
{
	while (DA__m_pFirstTreeable)
	{
		CDA_AVLTree *Tree = DA__m_pFirstTreeable->m_pTree;

		Tree->Remove(&Tree->m_pRoot, Tree->GetValue(DA__m_pFirstTreeable->m_pTreeable));
	}
}

void CDA_AVLTreeable::AVLTree_Insert(CDA_AVLTree *_Tree)
{
	newP(CDA_AVLTreeableMember, &TreeablePool) (_Tree, this);
}

void CDA_AVLTreeable::AVLTree_Remove(CDA_AVLTree *_Tree)
{
	CDA_AVLTreeableMember *CurrentMember = DA__m_pFirstTreeable;

	while (CurrentMember)
	{

		if (CurrentMember->m_pTree == _Tree)
		{
			deleteP(CurrentMember, &TreeablePool);
			return;
		}

		CurrentMember = CurrentMember->m_pNextTreaable;
	}
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDA_AVLTree
|__________________________________________________________________________________________________
\*************************************************************************************************/

CDA_AVLTree::CDA_AVLTree()
{
	m_pRoot = NULL;
}

/*
*  avlrotleft: perform counterclockwise rotation
*
*  Parameters:
*
*    _Object           Address of a pointer to a node
*/
void CDA_AVLTree::RotateLeft(CDA_AVLTreeableMember **_Object)
{
	CDA_AVLTreeableMember *tmp = *_Object;
	*_Object = (*_Object)->m_pRight;
	tmp->m_pRight = (*_Object)->m_pLeft;
	(*_Object)->m_pLeft = tmp;
}

/*
*  avlrotright: perform clockwise rotation
*
*  Parameters:
*
*    _Object           Address of a pointer to a node
*/
void CDA_AVLTree::RotateRight(CDA_AVLTreeableMember **_Object)
{
	CDA_AVLTreeableMember *tmp = *_Object;
	*_Object = (*_Object)->m_pLeft;
	tmp->m_pLeft = (*_Object)->m_pRight;
	(*_Object)->m_pRight = tmp;
}

void CDA_AVLTree::Swap(CDA_AVLTreeableMember *_Object1, CDA_AVLTreeableMember *_Object2)
{


}

/*
*  avlleftgrown: helper function for avlinsert
*
*  Parameters:
*
*    _Object           Address of a pointer to a node. This node's m_pLeft 
*                subtree has just grown due to item insertion; its 
*                "m_Skew" flag needs adjustment, and the local tree 
*                (the subtree of which this node is the root node) may 
*                have become unAVLRES_BALANCEd.
*
*  Return values:
*
*    AVLRES_OK          The local tree could be reAVLRES_BALANCEd or was AVLRES_BALANCEd 
*                from the start. The parent activations of the avlinsert 
*                activation that called this function may assume the 
*                entire tree is valid.
*
*    AVLRES_BALANCE     The local tree was AVLRES_BALANCEd, but has grown in height.
*                Do not assume the entire tree is valid.
*/
AVLRES CDA_AVLTree::LeftGrown(CDA_AVLTreeableMember **_Object)
{
	switch ((*_Object)->m_Skew) 
	{
	case AVLSKEW_LEFT:
		if ((*_Object)->m_pLeft->m_Skew == AVLSKEW_LEFT) 
		{
			(*_Object)->m_Skew = (*_Object)->m_pLeft->m_Skew = AVLSKEW_NONE;
			RotateRight(_Object);
		}	
		else 
		{
			switch ((*_Object)->m_pLeft->m_pRight->m_Skew) 
			{
			case AVLSKEW_LEFT:
				(*_Object)->m_Skew = AVLSKEW_RIGHT;
				(*_Object)->m_pLeft->m_Skew = AVLSKEW_NONE;
				break;
				
			case AVLSKEW_RIGHT:
				(*_Object)->m_Skew = AVLSKEW_NONE;
				(*_Object)->m_pLeft->m_Skew = AVLSKEW_LEFT;
				break;
				
			default:
				(*_Object)->m_Skew = AVLSKEW_NONE;
				(*_Object)->m_pLeft->m_Skew = AVLSKEW_NONE;
			}

			(*_Object)->m_pLeft->m_pRight->m_Skew = AVLSKEW_NONE;

			RotateLeft(& (*_Object)->m_pLeft);
			RotateRight(_Object);
		}
		return AVLRES_OK;
		
	case AVLSKEW_RIGHT:
		(*_Object)->m_Skew = AVLSKEW_NONE;
		return AVLRES_OK;
		
	default:
		(*_Object)->m_Skew = AVLSKEW_LEFT;
		return AVLRES_BALANCE;
	}
}

/*
*  avlrightgrown: helper function for avlinsert
*
*  See avlleftgrown for details.
*/
AVLRES CDA_AVLTree::RightGrown(CDA_AVLTreeableMember **_Object)
{
	switch ((*_Object)->m_Skew) 
	{
	case AVLSKEW_LEFT:					
		(*_Object)->m_Skew = AVLSKEW_NONE;
		return AVLRES_OK;
		
	case AVLSKEW_RIGHT:
		if ((*_Object)->m_pRight->m_Skew == AVLSKEW_RIGHT) 
		{	
			(*_Object)->m_Skew = (*_Object)->m_pRight->m_Skew = AVLSKEW_NONE;
			RotateLeft(_Object);
		}
		else 
		{
			switch ((*_Object)->m_pRight->m_pLeft->m_Skew) 
			{
			case AVLSKEW_RIGHT:
				(*_Object)->m_Skew = AVLSKEW_LEFT;
				(*_Object)->m_pRight->m_Skew = AVLSKEW_NONE;
				break;
				
			case AVLSKEW_LEFT:
				(*_Object)->m_Skew = AVLSKEW_NONE;
				(*_Object)->m_pRight->m_Skew = AVLSKEW_RIGHT;
				break;
				
			default:
				(*_Object)->m_Skew = AVLSKEW_NONE;
				(*_Object)->m_pRight->m_Skew = AVLSKEW_NONE;
			}
			(*_Object)->m_pRight->m_pLeft->m_Skew = AVLSKEW_NONE;

			RotateRight(& (*_Object)->m_pRight);
			RotateLeft(_Object);
		}
		return AVLRES_OK;
		
	default:
		(*_Object)->m_Skew = AVLSKEW_RIGHT;
		return AVLRES_BALANCE;
	}
}

/*
*  avlinsert: insert a node into the AVL tree.
*
*  Parameters:
*
*    _Object           Address of a pointer to a node.
*
*    d           Item to be inserted.
*
*  Return values:
*
*    nonzero     The item has been inserted. The excact value of 
*                nonzero yields is of no concern to user code; when
*                avlinsert recursively calls itself, the number 
*                returned tells the parent activation if the AVL tree 
*                may have become unAVLRES_BALANCEd; specifically:
*
*      AVLRES_OK        None of the subtrees of the node that _Object points to 
*                has grown, the AVL tree is valid.
*
*      AVLRES_BALANCE   One of the subtrees of the node that _Object points to 
*                has grown, the node's "m_Skew" flag needs adjustment,
*                and the AVL tree may have become unAVLRES_BALANCEd.
*
*    zero        The datum provided could not be inserted, either due 
*                to AVLKEY collision (the tree already contains another
*                item with which the same AVLKEY is associated), or
*                due to insufficient memory.
*/   
AVLRES CDA_AVLTree::Insert(CDA_AVLTreeableMember **_Object, CDA_AVLTreeableMember *_ObjectToInsert)
{
	AVLRES tmp;
	
	if (!(*_Object)) 
	{
		if (!((*_Object) = _ObjectToInsert)) 
		{
			return AVLRES_ERROR;
		}

		(*_Object)->m_pLeft = (*_Object)->m_pRight = NULL;
		(*_Object)->m_Skew = AVLSKEW_NONE;
		(*_Object)->m_pTree = this;

		return AVLRES_BALANCE;
	}
	
	if (Compare((*_Object)->m_pTreeable, GetValue(_ObjectToInsert->m_pTreeable)) < 0) 
	{
		if ((tmp = Insert(&(*_Object)->m_pLeft, _ObjectToInsert)) == AVLRES_BALANCE) 
		{
			return LeftGrown(_Object);
		}
		return tmp;
	}

	if (Compare((*_Object)->m_pTreeable, GetValue(_ObjectToInsert->m_pTreeable)) > 0) 
	{
		if ((tmp = Insert(&(*_Object)->m_pRight, _ObjectToInsert)) == AVLRES_BALANCE) 
		{
			return RightGrown(_Object);
		}
		return tmp;
	}

	return AVLRES_ERROR;
}

/*
*  avlleftshrunk: helper function for avlremove and avlfindlowest
*
*  Parameters:
*
*    _Object           Address of a pointer to a node. The node's m_pLeft
*                subtree has just shrunk due to item removal; its
*                "m_Skew" flag needs adjustment, and the local tree
*                (the subtree of which this node is the root node) may
*                have become unAVLRES_BALANCEd.
*
*   Return values:
*
*    AVLRES_OK          The parent activation of the avlremove activation
*                that called this function may assume the entire
*                tree is valid.
*
*    AVLRES_BALANCE     Do not assume the entire tree is valid.
*/                
AVLRES CDA_AVLTree::LeftShrunk(CDA_AVLTreeableMember **_Object)
{
	switch ((*_Object)->m_Skew) 
	{
	case AVLSKEW_LEFT:
		(*_Object)->m_Skew = AVLSKEW_NONE;
		return AVLRES_BALANCE;
		
	case AVLSKEW_RIGHT:
		if ((*_Object)->m_pRight->m_Skew == AVLSKEW_RIGHT) 
		{
			(*_Object)->m_Skew = (*_Object)->m_pRight->m_Skew = AVLSKEW_NONE;
			RotateLeft(_Object);
			return AVLRES_BALANCE;
		}
		else if ((*_Object)->m_pRight->m_Skew == AVLSKEW_NONE) 
		{
			(*_Object)->m_Skew = AVLSKEW_RIGHT;
			(*_Object)->m_pRight->m_Skew = AVLSKEW_LEFT;
			RotateLeft(_Object);
			return AVLRES_OK;
		}
		else 
		{
			switch ((*_Object)->m_pRight->m_pLeft->m_Skew) 
			{
			case AVLSKEW_LEFT:
				(*_Object)->m_Skew = AVLSKEW_NONE;
				(*_Object)->m_pRight->m_Skew = AVLSKEW_RIGHT;
				break;
				
			case AVLSKEW_RIGHT:
				(*_Object)->m_Skew = AVLSKEW_LEFT;
				(*_Object)->m_pRight->m_Skew = AVLSKEW_NONE;
				break;
				
			default:
				(*_Object)->m_Skew = AVLSKEW_NONE;
				(*_Object)->m_pRight->m_Skew = AVLSKEW_NONE;
			}

			(*_Object)->m_pRight->m_pLeft->m_Skew = AVLSKEW_NONE;

			RotateRight(& (*_Object)->m_pRight);
			RotateLeft(_Object);
			return AVLRES_BALANCE;
		}
		
	default:
		(*_Object)->m_Skew = AVLSKEW_RIGHT;
		return AVLRES_OK;
	}
}

/*
*  avlrightshrunk: helper function for avlremove and avlfindhighest
*
*  See avlleftshrunk for details.
*/
AVLRES CDA_AVLTree::RightShrunk(CDA_AVLTreeableMember **_Object)
{
	switch ((*_Object)->m_Skew) 
	{
	case AVLSKEW_RIGHT:
		(*_Object)->m_Skew = AVLSKEW_NONE;
		return AVLRES_BALANCE;
		
	case AVLSKEW_LEFT:
		if ((*_Object)->m_pLeft->m_Skew == AVLSKEW_LEFT) 
		{
			(*_Object)->m_Skew = (*_Object)->m_pLeft->m_Skew = AVLSKEW_NONE;
			RotateRight(_Object);
			return AVLRES_BALANCE;
		}
		else if ((*_Object)->m_pLeft->m_Skew == AVLSKEW_NONE) 
		{
			(*_Object)->m_Skew = AVLSKEW_LEFT;
			(*_Object)->m_pLeft->m_Skew = AVLSKEW_RIGHT;
			RotateRight(_Object);
			return AVLRES_OK;
		}
		else 
		{
			switch ((*_Object)->m_pLeft->m_pRight->m_Skew) 
			{
			case AVLSKEW_LEFT:
				(*_Object)->m_Skew = AVLSKEW_RIGHT;
				(*_Object)->m_pLeft->m_Skew = AVLSKEW_NONE;
				break;
				
			case AVLSKEW_RIGHT:
				(*_Object)->m_Skew = AVLSKEW_NONE;
				(*_Object)->m_pLeft->m_Skew = AVLSKEW_LEFT;	
				break;
				
			default:
				(*_Object)->m_Skew = AVLSKEW_NONE;
				(*_Object)->m_pLeft->m_Skew = AVLSKEW_NONE;
			}
			(*_Object)->m_pLeft->m_pRight->m_Skew = AVLSKEW_NONE;

			RotateLeft(& (*_Object)->m_pLeft);
			RotateRight(_Object);
			return AVLRES_BALANCE;
		}
		
	default:
		(*_Object)->m_Skew = AVLSKEW_LEFT;
		return AVLRES_OK;
	}
}

/*
*  avlfindhighest: replace a node with a subtree's highest-ranking item.
*
*  Parameters:
*
*    target      Pointer to node to be replaced.
*
*    _Object           Address of pointer to subtree.
*
*    res         Pointer to variable used to tell the caller whether
*                further checks are necessary; analog to the return
*                values of avlleftgrown and avlleftshrunk (see there). 
*
*  Return values:
*
*    1           A node was found; the target node has been replaced.
*
*    0           The target node could not be replaced because
*                the subtree provided was empty.
*
*/
int CDA_AVLTree::FindHighest(CDA_AVLTreeableMember *target, CDA_AVLTreeableMember **_Object, AVLRES *res)
{
	*res = AVLRES_BALANCE;
	if (!(*_Object)) 
	{
		return 0;
	}

	if ((*_Object)->m_pRight) 
	{
		if (!FindHighest(target, &(*_Object)->m_pRight, res)) 
		{
			return 0;
		}

		if (*res == AVLRES_BALANCE) 
		{
			*res = RightShrunk(_Object);
		}

		return 1;
	}

/*
// Original code
	target->d  = (*_Object)->d;
	tmp = *_Object;
	*_Object = (*_Object)->left;
	free(tmp);
	return 1;
*/

	// Ta bort _Object ur trädet, och sätt på targets plats, target tas bort helt

	CDA_AVLTreeableMember *tmp;
	tmp = *_Object;
	target->Link(tmp->m_pTreeable);
	tmp->Link(NULL);
	*_Object = (*_Object)->m_pLeft;

	deleteP(tmp, &TreeablePool);
	
	return 1;
}

/*
*  avlfindlowest: replace node with a subtree's lowest-ranking item.
*
*  See avlfindhighest for the details.
*/
int CDA_AVLTree::FindLowest(CDA_AVLTreeableMember *target, CDA_AVLTreeableMember **_Object, AVLRES *res)
{
	
	*res = AVLRES_BALANCE;

	if (!(*_Object)) 
	{
		return 0;
	}

	if ((*_Object)->m_pLeft) 
	{
		if (!FindLowest(target, &(*_Object)->m_pLeft, res)) 
		{
			return 0;
		}

		if (*res == AVLRES_BALANCE) 
		{
			*res = LeftShrunk(_Object);
		}

		return 1;
	}

	// Ta bort _Object ur trädet, och sätt på targets plats, target tas bort helt

	CDA_AVLTreeableMember *tmp;
	tmp = *_Object;
	target->Link(tmp->m_pTreeable);
	tmp->Link(NULL);
	*_Object = (*_Object)->m_pRight;
	tmp->m_pTree = NULL;
	deleteP(tmp, &TreeablePool);

	return 1;
}

/*
*  avlremove: remove an item from the tree.
*
*  Parameters:
*
*    _Object           Address of a pointer to a node.
*
*    key         AVLKEY of item to be removed.
*
*  Return values:
*
*    nonzero     The item has been removed. The exact value of 
*                nonzero yields if of no concern to user code; when
*                avlremove recursively calls itself, the number
*                returned tells the parent activation if the AVL tree
*                may have become unAVLRES_BALANCEd; specifically:
*
*      AVLRES_OK        None of the subtrees of the node that _Object points to
*                has shrunk, the AVL tree is valid.
*
*      AVLRES_BALANCE   One of the subtrees of the node that _Object points to
*                has shrunk, the node's "m_Skew" flag needs adjustment,
*                and the AVL tree may have become unAVLRES_BALANCEd.
*
*   zero         The tree does not contain an item yielding the
*                AVLKEY value provided by the caller.
*/
AVLRES CDA_AVLTree::Remove(CDA_AVLTreeableMember **_Object, void *_Key)
{
	AVLRES tmp = AVLRES_BALANCE;
	
	if (!(*_Object)) 
	{
		return AVLRES_ERROR;
	}

	int Comp = Compare((*_Object)->m_pTreeable, _Key);
	if (Comp < 0) 
	{
		if ((tmp = Remove(& (*_Object)->m_pLeft, _Key)) == AVLRES_BALANCE) 
		{
			return LeftShrunk(_Object);
		}
		return tmp;
	}
	else if (Comp > 0) 
	{
		if ((tmp = Remove(& (*_Object)->m_pRight, _Key)) == AVLRES_BALANCE) 
		{
			return RightShrunk(_Object);
		}
		return tmp;
	}

	if ((*_Object)->m_pLeft) 
	{
		if (FindHighest((*_Object), &((*_Object)->m_pLeft), &tmp)) 
		{
			if (tmp == AVLRES_BALANCE) 
			{
				tmp = LeftShrunk(_Object);
			}
			return tmp;
		}
	}

	if ((*_Object)->m_pRight) 
	{
		if (FindLowest((*_Object), &((*_Object)->m_pRight), &tmp)) 
		{
			if (tmp == AVLRES_BALANCE) 
			{
				tmp = RightShrunk(_Object);
			}
			return tmp;
		}
	}

	(*_Object)->Link(NULL);
	
	deleteP((*_Object), &TreeablePool);

	*_Object = NULL;

	return AVLRES_BALANCE;
}


CDA_AVLTreeableMember * CDA_AVLTree::SearchExact(CDA_AVLTreeableMember *_Object, void *_Key)
{
	if (!_Object) 
	{
		return NULL;
	}

	if (Compare(_Object->m_pTreeable, _Key) < 0)
	{
		return SearchExact(_Object->m_pLeft, _Key);
	}

	if (Compare(_Object->m_pTreeable, _Key) > 0) 
	{
		return SearchExact(_Object->m_pRight, _Key);
	}

	return _Object;
}



CDA_AVLTreeable* CDA_AVLTree::FindExact(void *_Key)
{
	CDA_AVLTreeableMember *Member = SearchExact(m_pRoot, _Key);
	if (Member)
		return Member->m_pTreeable;
	else
		return NULL;
}


CDA_AVLTreeableMember * CDA_AVLTree::SearchSmallestLarger(CDA_AVLTreeableMember *_Object, void *_Key, CDA_AVLTreeableMember** BestFit)
{
	if (!_Object) 
	{
		return NULL;
	}

	int CompareRes = Compare(_Object->m_pTreeable, _Key);

	if (CompareRes <= 0)
	{
		if (*BestFit)
		{
			if (Compare((*BestFit)->m_pTreeable, GetValue(_Object->m_pTreeable)) < 0)
			{
				*BestFit = _Object;
			}
		}
		else
		{
			*BestFit = _Object;
		}
	}

	if (CompareRes < 0)
	{
		return SearchSmallestLarger(_Object->m_pLeft, _Key, BestFit);
	}

	if (CompareRes > 0) 
	{
		
		return SearchSmallestLarger(_Object->m_pRight, _Key, BestFit);
	}

	*BestFit = _Object;

	return _Object;
}


CDA_AVLTreeable* CDA_AVLTree::FindSmallestLarger(void *_Key)
{
	CDA_AVLTreeableMember* BestFit = NULL;

	SearchSmallestLarger(m_pRoot, _Key, &BestFit);

	if (BestFit)
		return BestFit->m_pTreeable;
	else		
		return NULL;
}

CDA_AVLTreeableMember* CDA_AVLTree::SearchLargerSmallest(CDA_AVLTreeableMember *_Object, void *_Key, CDA_AVLTreeableMember** BestFit)
{

	if (!_Object) 
	{
		return NULL;
	}

	int CompareRes = Compare(_Object->m_pTreeable, _Key);

	if (CompareRes >= 0) 
	{
		if (*BestFit)
		{
			if (Compare((*BestFit)->m_pTreeable, GetValue(_Object->m_pTreeable)) > 0)
			{
				*BestFit = _Object;
			}
		}
		else
		{
			*BestFit = _Object;
		}
	}

	if (CompareRes < 0)
	{
		return SearchLargerSmallest(_Object->m_pLeft, _Key, BestFit);
	}

	if (CompareRes > 0) 
	{
		return SearchLargerSmallest(_Object->m_pRight, _Key, BestFit);
	}
	
	*BestFit = _Object;

	return _Object;

}

CDA_AVLTreeable* CDA_AVLTree::FindLargerSmallest(void *_Key)
{
	CDA_AVLTreeableMember* BestFit = NULL;

	SearchLargerSmallest(m_pRoot, _Key, &BestFit);

	if (BestFit)
		return BestFit->m_pTreeable;
	else
		return NULL;
}


CDA_AVLTreeable* CDA_AVLTree::FindLargest()
{
	CDA_AVLTreeableMember* BestFit = m_pRoot;

	if (!BestFit)
		return NULL;

	while(BestFit->m_pRight)
		BestFit = BestFit->m_pRight;


	if (BestFit)
		return BestFit->m_pTreeable;
	else
		return NULL;
}


int CDA_AVLTree::Compare(CDA_AVLTreeable *_Object, void *_Key)
{
	Error_static("CDA_AVLTree::Compare", "Need to overide");
	return 0;
}

void *CDA_AVLTree::GetValue(CDA_AVLTreeable *_Object)
{
	Error_static("CDA_AVLTree::Compare", "Need to overide");
	return NULL;
}

CDA_AVLTreeableMember* CDA_AVLTree::GetRoot()
{
	return m_pRoot;
}
