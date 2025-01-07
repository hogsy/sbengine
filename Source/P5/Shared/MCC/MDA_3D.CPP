
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
#include "MDA_3D.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CSpaceEnum_OctTree
|__________________________________________________________________________________________________
\*************************************************************************************************/
CSpaceEnum_OctTree::CSpaceEnum_OctTree(int _BoxSizeShift, int _MinBoxSizeShift, int _nMaxNodes, int _nMaxIDLinks, int _nIDs)
{
	m_MaxSize = 1 << _BoxSizeShift;
	m_MinSize = 1 << _MinBoxSizeShift;
	m_MaxLevel = _BoxSizeShift - _MinBoxSizeShift;

	m_lNodes.SetLen(_nMaxNodes);
	m_lIDInfo.SetLen(_nIDs);
	m_lIDLinks.SetLen(_nMaxIDLinks);

	m_pNodes = &m_lNodes[0];
	m_pIDLinks = &m_lIDLinks[0];

	m_spNodeHeap = MNew(CIDHeap);
	if (m_spNodeHeap == NULL) MemError("-");
	m_spIDLinkHeap = MNew(CIDHeap);
	if (m_spIDLinkHeap == NULL) MemError("-");
	
	m_RootBox.m_Min = CVec3Dint(-m_MaxSize >> 1);
	m_RootBox.m_Max = CVec3Dint(m_MaxSize >> 1);

	// Initialize root-node
	m_iRoot = m_spNodeHeap->AllocID();
	if (m_iRoot < 0) Error("-", "Internal error.");
	CSE_Node Root;
	m_pNodes[m_iRoot] = Root;
}

void CSpaceEnum_OctTree::CreateSubBox(int _Pos, const CBox3Dint& _Box, CBox3Dint& _SubBox)
{
	CVec3Dint Mid;
	_Box.GetCenter(Mid);
	
	for (int i = 0; i < 3; i++)
	{
		if (_Pos &(1 << i))
		{
			_SubBox.m_Min.k[i] = Mid.k[i];
			_SubBox.m_Max.k[i] = _Box.m_Max.k[i];
		}
		else
		{
			_SubBox.m_Min.k[i] = _Box.m_Min.k[i];
			_SubBox.m_Max.k[i] = Mid.k[i];
		}
	}
}



void CSpaceEnum_OctTree::CreateSubNode(int _iNode, int _iChild)
{
	CSE_Node* pNode = &m_pNodes[_iNode];
	if (pNode->m_iChildren[_iChild] != -1) return;

	int iChildNode = m_spNodeHeap->AllocID();
	if (iChildNode == -1) 
		Error("CreateSubNode", "Out of nodes!");

	pNode->m_iChildMask |= (1 << _iChild);
	pNode->m_iChildren[_iChild] = iChildNode;

	CSE_Node* pChild = &m_pNodes[iChildNode];
	pChild->m_iParent=  _iNode;
	pChild->m_iFirstID =  -1;
	pChild->m_iChildMask = 0;
	for(int i = 0; i < 8; pChild->m_iChildren[i++] = -1)
	{
	}
}

void CSpaceEnum_OctTree::RemoveEmptyChildren(CSE_Node* _pNode)
{
	if (!_pNode->m_iChildMask) return;

	for(int iChild = 0; iChild < 8; iChild++)
	{
		int	iChildNode = _pNode->m_iChildren[iChild];
		if (iChildNode != -1)
		{
			if (m_pNodes[iChildNode].m_iChildMask != 0) continue;
			if (m_pNodes[iChildNode].m_iFirstID != -1) continue;

			_pNode->m_iChildren[iChild] = -1;
			_pNode->m_iChildMask &= ~(1 << iChild);
			m_spNodeHeap->FreeID(iChildNode);
		}
	}
}

int CSpaceEnum_OctTree::InsertAtNode(int _ID, int _iNode, const CBox3Dint& _Box, const CBox3Dint& _NodeBox, int _Level)
{
#ifdef DEBUG_CSE_OCTTREE
	CStr Indent = CStrF(' ', _Level*3);
	LogFile(Indent + CStrF("(InsertAtNode) ID %d, iNode %d, Level %d", _ID, _iNode, _Level));
	LogFile(Indent + CStr("NodeBox : ") + _NodeBox.m_Min.GetString() + ", " + _NodeBox.m_Max.GetString());
#endif

	CSE_Node* pNode = &m_pNodes[_iNode];
	if ((_Level >= m_MaxLevel) || (_NodeBox.IsCovering(_Box)))
	{
		// (At max level) ||
		// (The box fill up this box entirly)
//fp64 Time; T_Start(Time);
		int iIDLink = m_spIDLinkHeap->AllocID();
		if (iIDLink < 0) Error("InsertAtNode", "Dissaster struck!");
		int iNext = pNode->m_iFirstID;
		if (iNext != -1) m_pIDLinks[iNext].m_iPrevID = iIDLink;
		pNode->m_iFirstID = iIDLink;
		CSE_IDLink* pLink = &m_pIDLinks[iIDLink];
		pLink->m_iNextID = iNext;
		pLink->m_ID = _ID;
		pLink->m_iPrevID = -1;

//T_Stop(Time);
//LogFile(T_String("Insert: ", Time*1000.0));

//	LogFile(CStrF("(InsertAtNode) INSERTING,  ID %d, iNode %d, Level %d, iLink %d", _ID, _iNode, _Level, iIDLink));
//	LogFile(CStr("Box : ") + _Box.m_Min.GetString() + ", " + _Box.m_Max.GetString() + ",   NodeBox : " + _NodeBox.m_Min.GetString() + ", " + _NodeBox.m_Max.GetString());

#ifdef DEBUG_CSE_OCTTREE
	LogFile(Indent + CStrF("(InsertAtNode) INSERTING,  ID %d, iNode %d, Level %d, iLink %d", _ID, _iNode, _Level, iIDLink));
	LogFile(Indent + CStr("Box : ") + _Box.m_Min.GetString() + ", " + _Box.m_Max.GetString());
	LogFile(Indent + CStr("NodeBox : ") + _NodeBox.m_Min.GetString() + ", " + _NodeBox.m_Max.GetString());
#endif
		return 1;
	}
	else
	{
		int nInsert = 0;

		// Split up, URK!
		int Mask = 0;
		CVec3Dint NodeMid;
		_NodeBox.GetCenter(NodeMid);
		if (_Box.m_Min.k[0] < NodeMid.k[0]) Mask |= 1;
		if (_Box.m_Max.k[0] > NodeMid.k[0]) Mask |= 2;
		if (_Box.m_Min.k[1] < NodeMid.k[1]) Mask |= 4;
		if (_Box.m_Max.k[1] > NodeMid.k[1]) Mask |= 8;
		if (_Box.m_Min.k[2] < NodeMid.k[2]) Mask |= 16;
		if (_Box.m_Max.k[2] > NodeMid.k[2]) Mask |= 32;
		if (Mask == 0) Error("InsertAtNode", "Internal error.");

#ifdef DEBUG_CSE_OCTTREE
	LogFile(Indent + CStrF("Mask %.4X, NodeMid: ", Mask) + NodeMid.GetString());
#endif
		CBox3Dint SubBox;
		if (Mask & 1)
		{
			if (Mask & 4)
			{
				if (Mask & 16)
				{
					CreateSubBox(0, _NodeBox, SubBox);
					if (pNode->m_iChildren[0] == -1) CreateSubNode(_iNode, 0);
					nInsert += InsertAtNode(_ID, pNode->m_iChildren[0], _Box, SubBox, _Level+1);
				}
				if (Mask & 32)
				{
					CreateSubBox(4, _NodeBox, SubBox);
					if (pNode->m_iChildren[4] == -1) CreateSubNode(_iNode, 4);
					nInsert += InsertAtNode(_ID, pNode->m_iChildren[4], _Box, SubBox, _Level+1);
				}
			}
			if (Mask & 8)
			{
				if (Mask & 16)
				{
					CreateSubBox(2, _NodeBox, SubBox);
					if (pNode->m_iChildren[2] == -1) CreateSubNode(_iNode, 2);
					nInsert += InsertAtNode(_ID, pNode->m_iChildren[2], _Box, SubBox, _Level+1);
				}
				if (Mask & 32)
				{
					CreateSubBox(4+2, _NodeBox, SubBox);
					if (pNode->m_iChildren[4+2] == -1) CreateSubNode(_iNode, 4+2);
					nInsert += InsertAtNode(_ID, pNode->m_iChildren[4+2], _Box, SubBox, _Level+1);
				}
			}
		}
		if (Mask & 2)
		{
			if (Mask & 4)
			{
				if (Mask & 16)
				{
					CreateSubBox(1, _NodeBox, SubBox);
					if (pNode->m_iChildren[1] == -1) CreateSubNode(_iNode, 1);
					nInsert += InsertAtNode(_ID, pNode->m_iChildren[1], _Box, SubBox, _Level+1);
				}
				if (Mask & 32)
				{
					CreateSubBox(4+1, _NodeBox, SubBox);
					if (pNode->m_iChildren[4+1] == -1) CreateSubNode(_iNode, 4+1);
					nInsert += InsertAtNode(_ID, pNode->m_iChildren[4+1], _Box, SubBox, _Level+1);
				}
			}
			if (Mask & 8)
			{
				if (Mask & 16)
				{
					CreateSubBox(2+1, _NodeBox, SubBox);
					if (pNode->m_iChildren[2+1] == -1) CreateSubNode(_iNode, 2+1);
					nInsert += InsertAtNode(_ID, pNode->m_iChildren[2+1], _Box, SubBox, _Level+1);
				}
				if (Mask & 32)
				{
					CreateSubBox(4+2+1, _NodeBox, SubBox);
					if (pNode->m_iChildren[4+2+1] == -1) CreateSubNode(_iNode, 4+2+1);
					nInsert += InsertAtNode(_ID, pNode->m_iChildren[4+2+1], _Box, SubBox, _Level+1);
				}
			}
		}
		
		return nInsert;
	}
}

int CSpaceEnum_OctTree::ReInsertAtNode(int _ID, int _iNode, const CBox3Dint& _Box, const CBox3Dint& _OldBox, const CBox3Dint& _NodeBox, int _Level)
{
#ifdef DEBUG_CSE_OCTTREE
	CStr Indent = CStrF(' ', _Level*3);
	LogFile(Indent + CStrF("(ReInsertAtNode) ID %d, iNode %d, Level %d", _ID, _iNode, _Level));
	LogFile(Indent + CStr("NodeBox : ") + _NodeBox.m_Min.GetString() + ", " + _NodeBox.m_Max.GetString());
#endif

	CSE_Node* pNode = &m_pNodes[_iNode];
	if (_Level >= m_MaxLevel) return 0;
	if ((_NodeBox.IsCovering(_Box)))
	{
		if (_NodeBox.IsCovering(_OldBox)) return 0;

		RemoveAtNode(_ID, _iNode, _OldBox, _NodeBox, _Level);

		// (At max level) ||
		// (The box fill up this box entirly)
//fp64 Time; T_Start(Time);
		int iIDLink = m_spIDLinkHeap->AllocID();
		if (iIDLink < 0) Error("ReInsertAtNode", "Dissaster struck!");
		int iNext = pNode->m_iFirstID;
		if (iNext != -1) m_pIDLinks[iNext].m_iPrevID = iIDLink;
		pNode->m_iFirstID = iIDLink;
		CSE_IDLink* pLink = &m_pIDLinks[iIDLink];
		pLink->m_iNextID = iNext;
		pLink->m_ID = _ID;
		pLink->m_iPrevID = -1;

//T_Stop(Time);
//LogFile(T_String("Insert: ", Time*1000.0));

//	LogFile(CStrF("(ReInsertAtNode) INSERTING,  ID %d, iNode %d, Level %d, iLink %d", _ID, _iNode, _Level, iIDLink));
//	LogFile(CStr("Box : ") + _Box.m_Min.GetString() + ", " + _Box.m_Max.GetString() + ",   NodeBox : " + _NodeBox.m_Min.GetString() + ", " + _NodeBox.m_Max.GetString());

#ifdef DEBUG_CSE_OCTTREE
	LogFile(Indent + CStrF("(ReInsertAtNode) INSERTING,  ID %d, iNode %d, Level %d, iLink %d", _ID, _iNode, _Level, iIDLink));
	LogFile(Indent + CStr("Box : ") + _Box.m_Min.GetString() + ", " + _Box.m_Max.GetString());
	LogFile(Indent + CStr("NodeBox : ") + _NodeBox.m_Min.GetString() + ", " + _NodeBox.m_Max.GetString());
#endif
		return 1;
	}
	else
	{
		int nInsert = 0;

		// Split up, URK!
		CVec3Dint NodeMid;
		_NodeBox.GetCenter(NodeMid);
		int Mask = 0;
		if (_Box.m_Min.k[0] < NodeMid.k[0]) Mask |= 1;
		if (_Box.m_Max.k[0] > NodeMid.k[0]) Mask |= 2;
		if (_Box.m_Min.k[1] < NodeMid.k[1]) Mask |= 4;
		if (_Box.m_Max.k[1] > NodeMid.k[1]) Mask |= 8;
		if (_Box.m_Min.k[2] < NodeMid.k[2]) Mask |= 16;
		if (_Box.m_Max.k[2] > NodeMid.k[2]) Mask |= 32;
		if (Mask == 0) Error("ReInsertAtNode", "Internal error.");

		int OldMask = 0;
		if (_OldBox.m_Min.k[0] < NodeMid.k[0]) OldMask |= 1;
		if (_OldBox.m_Max.k[0] > NodeMid.k[0]) OldMask |= 2;
		if (_OldBox.m_Min.k[1] < NodeMid.k[1]) OldMask |= 4;
		if (_OldBox.m_Max.k[1] > NodeMid.k[1]) OldMask |= 8;
		if (_OldBox.m_Min.k[2] < NodeMid.k[2]) OldMask |= 16;
		if (_OldBox.m_Max.k[2] > NodeMid.k[2]) OldMask |= 32;
		if (OldMask == 0) Error("ReInsertAtNode", "Internal error.");

#ifdef DEBUG_CSE_OCTTREE
	LogFile(Indent + CStrF("Mask %.4X, NodeMid: ", Mask) + NodeMid.GetString());
#endif
		CBox3Dint SubBox;
		int NewMask = Mask;
		Mask &= OldMask;
		if (Mask & 1)
		{
			if (Mask & 4)
			{
				if (Mask & 16)
				{
					CreateSubBox(0, _NodeBox, SubBox);
					if (pNode->m_iChildren[0] == -1) CreateSubNode(_iNode, 0);
					nInsert += ReInsertAtNode(_ID, pNode->m_iChildren[0], _Box, _OldBox, SubBox, _Level+1);
				}
				if (Mask & 32)
				{
					CreateSubBox(4, _NodeBox, SubBox);
					if (pNode->m_iChildren[4] == -1) CreateSubNode(_iNode, 4);
					nInsert += ReInsertAtNode(_ID, pNode->m_iChildren[4], _Box, _OldBox, SubBox, _Level+1);
				}
			}
			if (Mask & 8)
			{
				if (Mask & 16)
				{
					CreateSubBox(2, _NodeBox, SubBox);
					if (pNode->m_iChildren[2] == -1) CreateSubNode(_iNode, 2);
					nInsert += ReInsertAtNode(_ID, pNode->m_iChildren[2], _Box, _OldBox, SubBox, _Level+1);
				}
				if (Mask & 32)
				{
					CreateSubBox(4+2, _NodeBox, SubBox);
					if (pNode->m_iChildren[4+2] == -1) CreateSubNode(_iNode, 4+2);
					nInsert += ReInsertAtNode(_ID, pNode->m_iChildren[4+2], _Box, _OldBox, SubBox, _Level+1);
				}
			}
		}
		if (Mask & 2)
		{
			if (Mask & 4)
			{
				if (Mask & 16)
				{
					CreateSubBox(1, _NodeBox, SubBox);
					if (pNode->m_iChildren[1] == -1) CreateSubNode(_iNode, 1);
					nInsert += ReInsertAtNode(_ID, pNode->m_iChildren[1], _Box, _OldBox, SubBox, _Level+1);
				}
				if (Mask & 32)
				{
					CreateSubBox(4+1, _NodeBox, SubBox);
					if (pNode->m_iChildren[4+1] == -1) CreateSubNode(_iNode, 4+1);
					nInsert += ReInsertAtNode(_ID, pNode->m_iChildren[4+1], _Box, _OldBox, SubBox, _Level+1);
				}
			}
			if (Mask & 8)
			{
				if (Mask & 16)
				{
					CreateSubBox(2+1, _NodeBox, SubBox);
					if (pNode->m_iChildren[2+1] == -1) CreateSubNode(_iNode, 2+1);
					nInsert += ReInsertAtNode(_ID, pNode->m_iChildren[2+1], _Box, _OldBox, SubBox, _Level+1);
				}
				if (Mask & 32)
				{
					CreateSubBox(4+2+1, _NodeBox, SubBox);
					if (pNode->m_iChildren[4+2+1] == -1) CreateSubNode(_iNode, 4+2+1);
					nInsert += ReInsertAtNode(_ID, pNode->m_iChildren[4+2+1], _Box, _OldBox, SubBox, _Level+1);
				}
			}
		}

		// ------------------------------------------
		OldMask &= (~NewMask);
		if (OldMask & 1)
		{
			if (OldMask & 4)
			{
				if ((OldMask & 16) && (pNode->m_iChildren[0] != -1))
				{
					const int iChild = 0;
					CreateSubBox(iChild, _NodeBox, SubBox);
					RemoveAtNode(_ID, pNode->m_iChildren[iChild], _Box, SubBox, _Level+1);
				}
				if ((OldMask & 32) && (pNode->m_iChildren[4] != -1))
				{
					const int iChild = 4;
					CreateSubBox(iChild, _NodeBox, SubBox);
					RemoveAtNode(_ID, pNode->m_iChildren[iChild], _Box, SubBox, _Level+1);
				}
			}
			if (OldMask & 8)
			{
				if ((OldMask & 16) && (pNode->m_iChildren[2] != -1))
				{
					const int iChild = 2;
					CreateSubBox(iChild, _NodeBox, SubBox);
					RemoveAtNode(_ID, pNode->m_iChildren[iChild], _Box, SubBox, _Level+1);
				}
				if ((OldMask & 32) && (pNode->m_iChildren[4+2] != -1))
				{
					const int iChild = 4+2;
					CreateSubBox(iChild, _NodeBox, SubBox);
					RemoveAtNode(_ID, pNode->m_iChildren[iChild], _Box, SubBox, _Level+1);
				}
			}
		}
		if (OldMask & 2)
		{
			if (OldMask & 4)
			{
				if ((OldMask & 16) && (pNode->m_iChildren[1] != -1))
				{
					const int iChild = 1;
					CreateSubBox(iChild, _NodeBox, SubBox);
					RemoveAtNode(_ID, pNode->m_iChildren[iChild], _Box, SubBox, _Level+1);
				}
				if ((OldMask & 32) && (pNode->m_iChildren[4+1] != -1))
				{
					const int iChild = 4+1;
					CreateSubBox(iChild, _NodeBox, SubBox);
					RemoveAtNode(_ID, pNode->m_iChildren[iChild], _Box, SubBox, _Level+1);
				}
			}
			if (OldMask & 8)
			{
				if ((OldMask & 16) && (pNode->m_iChildren[2+1] != -1))
				{
					const int iChild = 2+1;
					CreateSubBox(iChild, _NodeBox, SubBox);
					RemoveAtNode(_ID, pNode->m_iChildren[iChild], _Box, SubBox, _Level+1);
				}
				if ((OldMask & 32) && (pNode->m_iChildren[4+2+1] != -1))
				{
					const int iChild = 4+2+1;
					CreateSubBox(iChild, _NodeBox, SubBox);
					RemoveAtNode(_ID, pNode->m_iChildren[iChild], _Box, SubBox, _Level+1);
				}
			}
		}


		return nInsert;
	}
}


void CSpaceEnum_OctTree::RemoveAtNode(int _ID, int _iNode, const CBox3Dint& _Box, const CBox3Dint& _NodeBox, int _Level)
{
#ifdef DEBUG_CSE_OCTTREE
	CStr Indent = CStrF(' ', _Level*3);
#endif

	CSE_Node* pNode = &m_pNodes[_iNode];
	if ((_Level >= m_MaxLevel) || (_NodeBox.IsCovering(_Box)))
	{
		// (At max level) ||
		// (The box fill up this box entirly)

		// Find this ID
		int nSeek = 0;
		int iLink = pNode->m_iFirstID;
		while ((iLink != -1) && (m_pIDLinks[iLink].m_ID != _ID))
		{
			iLink = m_pIDLinks[iLink].m_iNextID;
			nSeek++;
		}
#ifdef DEBUG_CSE_OCTTREE
	LogFile(Indent + CStrF("(RemoveAtNode) ID %d, iNode %d, Level %d, iFirstLink %d, nSeek %d", _ID, _iNode, _Level, pNode->m_iFirstID, nSeek));
	LogFile(Indent + CStr("Box : ") + _Box.m_Min.GetString() + ", " + _Box.m_Max.GetString());
#endif
		if (iLink == -1) Error("RemoveAtNode", "Internal error. (ID not found in node.)");

		// Unlink
		int iPrev = m_pIDLinks[iLink].m_iPrevID;
		int iNext = m_pIDLinks[iLink].m_iNextID;

		if (iPrev != -1)
			m_pIDLinks[iPrev].m_iNextID = iNext;
		else
			pNode->m_iFirstID = iNext;

		if (iNext != -1)
			m_pIDLinks[iNext].m_iPrevID = iPrev;

		// Free
		m_spIDLinkHeap->FreeID(iLink);
		return;
	}
	else
	{
		// Split up, URK!
		int Mask = 0;
		CVec3Dint NodeMid;
		_NodeBox.GetCenter(NodeMid);
		if (_Box.m_Min.k[0] < NodeMid.k[0]) Mask |= 1;
		if (_Box.m_Max.k[0] > NodeMid.k[0]) Mask |= 2;
		if (_Box.m_Min.k[1] < NodeMid.k[1]) Mask |= 4;
		if (_Box.m_Max.k[1] > NodeMid.k[1]) Mask |= 8;
		if (_Box.m_Min.k[2] < NodeMid.k[2]) Mask |= 16;
		if (_Box.m_Max.k[2] > NodeMid.k[2]) Mask |= 32;
		if (Mask == 0) Error("InsertAtNode", "Internal error.");

		CBox3Dint SubBox;
		if (Mask & 1)
		{
			if (Mask & 4)
			{
				if ((Mask & 16) && (pNode->m_iChildren[0] != -1))
				{
					const int iChild = 0;
					CreateSubBox(iChild, _NodeBox, SubBox);
					RemoveAtNode(_ID, pNode->m_iChildren[iChild], _Box, SubBox, _Level+1);
				}
				if ((Mask & 32) && (pNode->m_iChildren[4] != -1))
				{
					const int iChild = 4;
					CreateSubBox(iChild, _NodeBox, SubBox);
					RemoveAtNode(_ID, pNode->m_iChildren[iChild], _Box, SubBox, _Level+1);
				}
			}
			if (Mask & 8)
			{
				if ((Mask & 16) && (pNode->m_iChildren[2] != -1))
				{
					const int iChild = 2;
					CreateSubBox(iChild, _NodeBox, SubBox);
					RemoveAtNode(_ID, pNode->m_iChildren[iChild], _Box, SubBox, _Level+1);
				}
				if ((Mask & 32) && (pNode->m_iChildren[4+2] != -1))
				{
					const int iChild = 4+2;
					CreateSubBox(iChild, _NodeBox, SubBox);
					RemoveAtNode(_ID, pNode->m_iChildren[iChild], _Box, SubBox, _Level+1);
				}
			}
		}
		if (Mask & 2)
		{
			if (Mask & 4)
			{
				if ((Mask & 16) && (pNode->m_iChildren[1] != -1))
				{
					const int iChild = 1;
					CreateSubBox(iChild, _NodeBox, SubBox);
					RemoveAtNode(_ID, pNode->m_iChildren[iChild], _Box, SubBox, _Level+1);
				}
				if ((Mask & 32) && (pNode->m_iChildren[4+1] != -1))
				{
					const int iChild = 4+1;
					CreateSubBox(iChild, _NodeBox, SubBox);
					RemoveAtNode(_ID, pNode->m_iChildren[iChild], _Box, SubBox, _Level+1);
				}
			}
			if (Mask & 8)
			{
				if ((Mask & 16) && (pNode->m_iChildren[2+1] != -1))
				{
					const int iChild = 2+1;
					CreateSubBox(iChild, _NodeBox, SubBox);
					RemoveAtNode(_ID, pNode->m_iChildren[iChild], _Box, SubBox, _Level+1);
				}
				if ((Mask & 32) && (pNode->m_iChildren[4+2+1] != -1))
				{
					const int iChild = 4+2+1;
					CreateSubBox(iChild, _NodeBox, SubBox);
					RemoveAtNode(_ID, pNode->m_iChildren[iChild], _Box, SubBox, _Level+1);
				}
			}
		}
		RemoveEmptyChildren(pNode);
	}
}

int CSpaceEnum_OctTree::Insert(int _ID, const CVec3Dfp32& _Min, const CVec3Dfp32& _Max)
{
	int MaxW = TruncToInt(Max3(_Max.k[0] - _Min.k[0], _Max.k[1] - _Min.k[1], _Max.k[2] - _Min.k[2]));
	MaxW = (GetGEPow2(MaxW) >> 1);

	CSE_IDInfo* pIDInfo = &m_lIDInfo[_ID];


	int MinSizeAnd = Max(m_MinSize, MaxW) - 1;
	int And = 0xffffffff - MinSizeAnd;

	if (pIDInfo->m_IsInserted)
	{
		CBox3Dint Box;
		Box.m_Min.k[0] = (int)_Min.k[0] & And;
		Box.m_Min.k[1] = (int)_Min.k[1] & And;
		Box.m_Min.k[2] = (int)_Min.k[2] & And;
		Box.m_Max.k[0] = ((int)_Max.k[0] + m_MinSize) & And;
		Box.m_Max.k[1] = ((int)_Max.k[1] + m_MinSize) & And;
		Box.m_Max.k[2] = ((int)_Max.k[2] + m_MinSize) & And;
		if (!Box.IsCovering(m_RootBox)) 
		{
			if (m_RootBox.IsInside(Box))
			{ Error("Insert", "Box is partly outside the world."); }
			else
			{ Error("Insert", "Box is wholy outside the world."); }
		}

		int n = ReInsertAtNode(_ID, m_iRoot, Box, pIDInfo->m_Box, m_RootBox, 0);
		pIDInfo->m_Box = Box;
		return n;
	}

	pIDInfo->m_IsInserted = TRUE;
	pIDInfo->m_Box.m_Min.k[0] = (int)_Min.k[0] & And;
	pIDInfo->m_Box.m_Min.k[1] = (int)_Min.k[1] & And;
	pIDInfo->m_Box.m_Min.k[2] = (int)_Min.k[2] & And;
	pIDInfo->m_Box.m_Max.k[0] = ((int)_Max.k[0] + m_MinSize) & And;
	pIDInfo->m_Box.m_Max.k[1] = ((int)_Max.k[1] + m_MinSize) & And;
	pIDInfo->m_Box.m_Max.k[2] = ((int)_Max.k[2] + m_MinSize) & And;
	if (!pIDInfo->m_Box.IsCovering(m_RootBox)) 
	{
		if (m_RootBox.IsInside(pIDInfo->m_Box))
		{ Error("Insert", "Box is partly outside the world."); }
		else
		{ Error("Insert", "Box is wholy outside the world."); }
	}

	return InsertAtNode(_ID, m_iRoot, pIDInfo->m_Box, m_RootBox, 0);


#ifdef DEBUG_CSE_OCTTREE
	LogFile(CStrF("(Insert) ID %d", _ID));
	LogFile(CStr("Box : ") + pIDInfo->m_Box.m_Min.GetString() + ", " + pIDInfo->m_Box.m_Max.GetString());
	LogFile(CStr("RootBox : ") + m_RootBox.m_Min.GetString() + ", " + m_RootBox.m_Max.GetString());
#endif

}

void CSpaceEnum_OctTree::Remove(int _ID)
{
	CSE_IDInfo* pIDInfo = &m_lIDInfo[_ID];
	if (!pIDInfo->m_IsInserted) return;
//	if (!pIDInfo->m_IsInserted) Error("Insert", "ID not inserted.");

#ifdef DEBUG_CSE_OCTTREE
	LogFile(CStrF("(Remove) ID %d", _ID));
	LogFile(CStr("Box : ") + pIDInfo->m_Box.m_Min.GetString() + ", " + pIDInfo->m_Box.m_Max.GetString());
#endif

	pIDInfo->m_IsInserted = FALSE;
	RemoveAtNode(_ID, m_iRoot, pIDInfo->m_Box, m_RootBox, 0);
}

void CSpaceEnum_OctTree::EnumerateBox(CVec3Dfp32 _Min, CVec3Dfp32 _Max, int(*pfnEnumObjectsCallback)(int _ID))
{
}

// -------------------------------------------------------------------
const CSE_Node* CSpaceEnum_OctTree::GetRootPtr() const
{
	return m_pNodes;
}

CBox3Dint CSpaceEnum_OctTree::GetRootBox() const
{
	return m_RootBox;
}

