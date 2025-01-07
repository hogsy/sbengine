#include "PCH.h"

#include "XRNavGraphBuilder.h"
#include "XRBlockNav.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_NavGraph_Builder
|__________________________________________________________________________________________________
\*************************************************************************************************/

//Don't build navgraph on consoles (memory-heavy)
//#ifndef PLATFORM_CONSOLE

MRTC_IMPLEMENT_DYNAMIC(CXR_NavGraph_Builder, CXR_NavGraph_Writer);

//Clear graph
void CXR_NavGraph_Builder::Clear()
{
	MAUTOSTRIP(CXR_NavGraph_Builder_Clear, MAUTOSTRIP_VOID);
	m_lNodes.Clear();
	m_lEdges.Clear();
};

void CXR_NavGraph_Builder::ClearConstruction()
{
	MAUTOSTRIP(CXR_NavGraph_Builder_ClearConstruction, MAUTOSTRIP_VOID);
	m_lObjGridPos.Clear();
	m_ObjectBoundMin = CVec3Dint16(-1);
	m_ObjectBoundMax = CVec3Dint16(-1);

	m_lCNodes.Clear();
	m_lCEdges.Clear();
	m_iEmptyNode = 0;
	m_iEmptyEdge = 0;
};



//Allocates space for the specified number of nodes and edges, respectively
void CXR_NavGraph_Builder::AllocateNodeSpace(int _n)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_AllocateNodeSpace, MAUTOSTRIP_VOID);
	m_lCNodes.SetLen(_n + m_lCNodes.Len());
};
void CXR_NavGraph_Builder::AllocateEdgeSpace(int _n)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_AllocateEdgeSpace, MAUTOSTRIP_VOID);
	m_lCEdges.SetLen(_n + m_lCEdges.Len());
};


//Place node at given position, allocating more space if necessary
int CXR_NavGraph_Builder::PlaceNode(const CVec3Dint16& _Pos)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_PlaceNode, 0);
	if (m_iEmptyNode >= m_lCNodes.Len())
	{
		int iNew = Max((int)(m_lCNodes.Len() * 0.5f), (int)(m_iEmptyNode - m_lCNodes.Len() + 1));
		AllocateNodeSpace(iNew);
		AllocateEdgeSpace(iNew * 4);
	}

	m_lCNodes[m_iEmptyNode].m_GridPos = _Pos;
	m_lCNodes[m_iEmptyNode].m_lEdges.Clear();

	m_iEmptyNode++;
	return m_iEmptyNode - 1;
};	
		

//Place edge from one node to another, allocating more space if necessary
int CXR_NavGraph_Builder::PlaceEdge(int16 _iFrom, int16 _iTo, uint16 _iCost)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_PlaceEdge, 0);
	if (!m_lCNodes.ValidPos(_iFrom) ||
		!m_lCNodes.ValidPos(_iTo))
		return -1;

	//Allocate space for edge if necessary
	if (m_iEmptyEdge >= m_lCEdges.Len())
	{
		int iNew = Max((int)(m_lCEdges.Len() * 0.5f), (int)(m_iEmptyEdge - m_lCEdges.Len() + 1));
		AllocateEdgeSpace(iNew);
	}

	//Add edge
	m_lCEdges[m_iEmptyEdge].m_iFrom = _iFrom;
	m_lCEdges[m_iEmptyEdge].m_iTo = _iTo;
	m_lCEdges[m_iEmptyEdge].m_Cost = _iCost;
	m_lCEdges[m_iEmptyEdge].m_Sizes = (1 << CXR_NavGraph::SIZE_DEFAULT); //Currently all edges are default size, other size info is added after edge has been placed

	//Add edge to node
	m_lCNodes[_iFrom].AddEdge(m_iEmptyEdge);

	m_iEmptyEdge++;
	return m_iEmptyEdge - 1;
};			


//Helper class; stack of node candidates
class CNodeCandidateStack
{
protected:
	class CNodeCandidate
	{
	public:
		int16 m_iParent;
		uint16 m_Cost;
		CVec3Dint16 m_Pos;
		//int32 m_iHashNext;
		//int32 m_iHashPrev;
		CNodeCandidate(int16 _iParent = -1, uint16 _Cost = 0, const CVec3Dint16& _Pos = -1)
		{
			m_iParent = _iParent;
			m_Cost = _Cost;
			m_Pos = _Pos;
			//m_iHashNext = -1;
			//m_iHashPrev = -1;
		}
	};

	//Node candidate list
	TThinArray<CNodeCandidate> m_lNCs;
	int m_iTop;

/*	//2D position hash (with 2^16 slots)
	TThinArray<int32> m_liHash;

	int Hash_Value(const CVec3Dint16& _Pos)
	{
		return ((_Pos[0] & 255) << 8) + (_Pos[1] & 255);
	};

	//Add neighbour candidate with given index and position to hash
	bool Hash_Add(int32 _iIndex, const CVec3Dint16& _Pos)
	{
		int iHash = Hash_Value(_Pos);	
		if (!m_liHash.ValidPos(iHash))
			return false;

		m_lNCs[_iIndex].m_iHashNext = m_liHash[iHash];		
		m_lNCs[_iIndex].m_iHashPrev = -1;
		if (m_lNCs.ValidPos(m_liHash[iHash]))
			m_lNCs[m_liHash[iHash]].m_iHashPrev = _iIndex;		
		m_liHash[iHash] = _iIndex;
		return true;
	};

	//Remove neighbour candidate with given index and position from hash
	bool Hash_Remove(int32 _iIndex)
	{
		if (!m_lNCs.ValidPos(_iIndex))
			return false;

		if (m_lNCs[_iIndex].m_iHashPrev == -1)
		{
			//Remove from hash list
			int iHash = Hash_Value(m_lNCs[_iIndex].m_Pos);
			if (m_liHash.ValidPos(iHash))
				m_liHash[iHash] = m_lNCs[_iIndex].m_iHashNext;
		}
		else if (m_lNCs.ValidPos(m_lNCs[_iIndex].m_iHashPrev))
		{
			//Move link from previous to this to next
			m_lNCs[m_lNCs[_iIndex].m_iHashPrev].m_iHashNext = m_lNCs[_iIndex].m_iHashNext;
		}

		if (m_lNCs.ValidPos(m_lNCs[_iIndex].m_iHashNext))
			//Move link from next to this to previous
			m_lNCs[m_lNCs[_iIndex].m_iHashNext].m_iHashPrev	= m_lNCs[_iIndex].m_iHashPrev;

		return true;
	};

	//Check if given position is in hash
	bool Hash_Find(const CVec3Dint16& _Pos)
	{
		//Check if position is valid
		int iHash = Hash_Value(_Pos);	
		if (!m_liHash.ValidPos(iHash))
			return false;

		//Check element positions
		int iIndex = m_liHash[iHash];
		while (m_lNCs.ValidPos(iIndex))
		{
			if (m_lNCs[iIndex].m_Pos == _Pos)
				return true;

			iIndex = m_lNCs[iIndex].m_iHashNext;		
		}
		return false;
	};*/

public:
	CNodeCandidateStack()
	{
		m_lNCs.Clear();
		m_iTop = 0;
		/*
		m_liHash.SetLen(1 << 16);
		for (int i = 0; i < m_liHash.Len(); i++)
			m_liHash[i] = -1;
		*/
	};

	void Allocate(int _n)
	{
		if (m_iTop + _n >= m_lNCs.Len())
			m_lNCs.SetLen(m_iTop + _n);
	}

	void Push(int16 _iParent, uint16 _Cost, const CVec3Dint16& _Pos)
	{
		//Only push new positions
		//if (!Hash_Find(_Pos))
		{
			if (m_iTop >= m_lNCs.Len())
				m_lNCs.SetLen(m_iTop);

			m_lNCs[m_iTop] = CNodeCandidate(_iParent, _Cost, _Pos);
			//Hash_Add(m_iTop, _Pos);
			m_iTop++;
		}
	};

	bool Pop(int16& _iParent, uint16& _Cost, CVec3Dint16& _Pos)
	{
		if (m_iTop > 0)
		{
			m_iTop--;
			_iParent = m_lNCs[m_iTop].m_iParent;
			_Cost = m_lNCs[m_iTop].m_Cost;
			_Pos = m_lNCs[m_iTop].m_Pos;
			//Hash_Remove(m_iTop);
			return true;
		}
		else
		{
			return false;
		}
	};
};


//Cover the entire traversable area that can be reached from the given start node with nodes
void CXR_NavGraph_Builder::PlaceNodes_r(CNB_Pathfinder * _pPathFinder, int _iParent)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_PlaceNodes_r, MAUTOSTRIP_VOID);
	//Iterative variant

	//All current node candidates. Allocate a hefty amout to start with allocating repeatedly leaks memory it seems...
	CNodeCandidateStack NCStack;
	NCStack.Allocate(200000);

	//First node has already been placed; find child node candidates and push onto stack
	{
		TThinArray<CPosCost> lPos;
		//LogFile(CStrF("BF Parent %d (%d,%d,%d)", _iParent, m_lCNodes[_iParent].m_GridPos[0], m_lCNodes[_iParent].m_GridPos[1], m_lCNodes[_iParent].m_GridPos[2]));
		int nPos = _pPathFinder->BreadthFirstFindPositions(m_lCNodes[_iParent].m_GridPos, m_MinDist, &lPos);
		//Push in reverse order, so that first found node will be checked first
		NCStack.Allocate(nPos);
		for (int i = nPos - 1; i >= 0; i--)
		{
			NCStack.Push(_iParent, lPos[i].m_Cost, lPos[i].m_Pos);
		}
	}

	//Check node candidates for validity and additional node candidates, until we run out of them
	int16 iParent;
	uint16 Cost;
	CVec3Dint16 Pos; 
	while (NCStack.Pop(iParent, Cost, Pos))
	{
		//Place child node and mark as visited if position is unvisited
		if (!_pPathFinder->m_pCheck->Get(Pos[0], Pos[1], Pos[2]))
		{
			//LogFile(CStrF("Place node at (%d,%d,%d)", Pos[0], Pos[1], Pos[2]));
			AdjustNodePosition(_pPathFinder->m_pCheck,Pos,8,true);
			if (!_pPathFinder->IsValidGroundTraversable(Pos))
			{
				continue;
			}
			int iChild = PlaceNode(Pos);
			_pPathFinder->m_pCheck->Set(Pos[0], Pos[1], Pos[2]);
 
			//Place edge from parent to child 
			PlaceEdge(iParent, iChild, Cost);

			//Push all node candidates of this node onto stack
			//LogFile(CStrF("Add node %d (%d,%d,%d)", iChild, Pos[0], Pos[1], Pos[2]));
			TThinArray<CPosCost> lPos;
			int nPos = _pPathFinder->BreadthFirstFindPositions(Pos, m_MinDist, &lPos);

			//Push in reverse order, so that first found node will be checked first
			NCStack.Allocate(nPos);
			for (int i = nPos - 1; i >= 0; i--)
			{
				NCStack.Push(iChild, lPos[i].m_Cost, lPos[i].m_Pos);
			}

			//Wrap progress at 8192 nodes
			MRTC_SetProgress((fp32)(m_iEmptyNode % 8192) / (fp32)8192);
		}
	}


	//End of iterative variant
	return;

/*	//Recursive variant...
	//Find next node position at MinDist depth, marking all visited cells as such
	//TArray<CPosCost> lPos;
	//int nPos;
try {
	LogFile(CStrF("BF Parent %d (%d,%d,%d)", _iParent, m_lCNodes[_iParent].m_GridPos[0], m_lCNodes[_iParent].m_GridPos[1], m_lCNodes[_iParent].m_GridPos[2]));
	nPos = _pPathFinder->BreadthFirstFindPositions(m_lCNodes[_iParent].m_GridPos, m_MinDist, &lPos);
	LogFile(CStrF("BF done, found %d node candidates.", nPos));
}
catch (CCException& err)
{
	LogFile(CStr("BF: ") + CStr(err.GetExceptionInfo().GetString()));
}

try {
	//Place nodes until all positions at given depth has been visited
	int iCurPos = 0;
	while (iCurPos < nPos)
	{
		//Place child node and mark as visited if position is in check matrix
		if (_pPathFinder->m_pCheck->ValidPos(lPos[iCurPos].m_Pos[0], lPos[iCurPos].m_Pos[1], lPos[iCurPos].m_Pos[2]))
		{
			LogFile(CStrF("Place node at (%d,%d,%d)", lPos[iCurPos].m_Pos[0], lPos[iCurPos].m_Pos[1], lPos[iCurPos].m_Pos[2]));

			int iChild = PlaceNode(lPos[iCurPos].m_Pos);
			LogFile("Set check");
			_pPathFinder->m_pCheck->Set(lPos[iCurPos].m_Pos[0], lPos[iCurPos].m_Pos[1], lPos[iCurPos].m_Pos[2]);

			//Place edge from parent to child 
			LogFile("Place Edge");
			PlaceEdge(_iParent, iChild, lPos[iCurPos].m_Cost);

			//Continue placing nodes from this new node
			LogFile("Place nodes from this node.");
			PlaceNodes_r(_pPathFinder, iChild);
		}

		//Find next unvisited position index
		iCurPos++;
try{
		while ( (iCurPos < nPos) &&
				_pPathFinder->m_pCheck->Get(lPos[iCurPos].m_Pos[0], lPos[iCurPos].m_Pos[1], lPos[iCurPos].m_Pos[2]) )
		{
			iCurPos++;
		}
}
catch (CCException& err)
{
	LogFile(CStr("Increment: ") + CStr(err.GetExceptionInfo().GetString()));
}
	}
	//No more nodes to place from here
}
catch (CCException& err)
{
	LogFile(CStr("Placenodes: ") + CStr(err.GetExceptionInfo().GetString()));
}
*/
};


//Check if there's an edge from the given origin to the given destination
bool CXR_NavGraph_Builder::HasEdgeTo(int16 _iFrom, int16 _iTo)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_HasEdgeTo, false);
	for (int i = 0; i < m_lCNodes[_iFrom].NumEdges(); i++)
	{
		int iEdge = m_lCNodes[_iFrom].m_lEdges[i];
		
		if (!m_lCEdges.ValidPos(iEdge))
		{
			LogFile(CStrF("(NavGraph::HasEdgeTo) Warning: Invalid edge! (%d, %d, %d)", iEdge, i, _iFrom));
			return false;
		}

		if (m_lCEdges[iEdge].m_iTo == _iTo)
			return true;
	}
	return false;
};


//Helper to HasEdgePathStepLimit
int16 StepDistance(const CVec3Dint16& _From, const CVec3Dint16& _To);
int16 StepDistance(const CVec3Dint16& _From, const CVec3Dint16& _To)
{
	MAUTOSTRIP(StepDistance, 0);
	return Abs(_From[0] - _To[0]) + Abs(_From[1] - _To[1]) + Abs(_From[2] - _To[2]); 
}


//Check if there is a "good" edge-path from the origin to the destination
bool CXR_NavGraph_Builder::HasEdgePathStepLimit(int16 _iFrom, int16 _iTo)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_HasEdgePathStepLimit, false);
	//Fail immediately if start node has no edges
	if (m_lCNodes[_iFrom].NumEdges() == 0)
		return false;

	//We must find a path with no more than double the number of cell steps necessary
	int StepDist = StepDistance(m_lCNodes[_iFrom].m_GridPos, m_lCNodes[_iTo].m_GridPos); 
	int MaxCost = StepDist + StepDist;

	//Open and closed nodes with first empty index. Never expand more than 64 nodes
	TThinArray<CConstructionSearchNode> lNodes;
	lNodes.SetLen(64);
	lNodes[0] = CConstructionSearchNode(_iFrom, StepDist, 0);
	int iFirstEmpty = 1;

	//Open nodes
	TPriorityQueue2<CConstructionSearchNode> QOpen;
	QOpen.Create(false,64);

	//Current node
	CConstructionSearchNode * pN = &(lNodes[0]);

	//Search until we find path, there's no more nodes to search or we fill node list
	while (pN)
	{
		//Are we at destination?
		if (pN->m_iNode == _iTo)
		{
			//Jupp, found path
			return true;
		}
		else
		{
			//Expand node
			for (int i = 0; i < m_lCNodes[pN->m_iNode].NumEdges(); i++)
			{
				//Prune nodes with greater than max cost. Note that I don't prune existing nodes
				//since any loop will be swiftly pruned due to cost (but I prune self-loops to be sure)
				if (!m_lCEdges.ValidPos(m_lCNodes[pN->m_iNode].m_lEdges[i]))
				{
					LogFile(CStrF("(NavGraph::HasEdgePathStepLimit) Warning: Invalid edge! (%d, %d, %d)", m_lCNodes[pN->m_iNode].m_lEdges[i], i, pN->m_iNode));
				}
				else
				{
					int iDest = m_lCEdges[m_lCNodes[pN->m_iNode].m_lEdges[i]].m_iTo;
					if (iDest != pN->m_iNode)
					{
						int iHeuristic = StepDistance(m_lCNodes[iDest].m_GridPos, m_lCNodes[_iTo].m_GridPos);
						int iCost = pN->m_iCost + StepDistance(m_lCNodes[pN->m_iNode].m_GridPos, m_lCNodes[iDest].m_GridPos);
						if (iHeuristic + iCost <= MaxCost)
						{
							//Add node to open list if possible
							if (iFirstEmpty < lNodes.Len())
							{
								lNodes[iFirstEmpty] = CConstructionSearchNode(iDest, iHeuristic, iCost);
								QOpen.Push(&(lNodes[iFirstEmpty]));
								iFirstEmpty++;
							}
							else
							{
								//Node list is full
								//LogFile(CStr("(NavGraph::HasEdgePathStepLimit) Warning: Node list full. Pathfinding incomplete."));
								return false;
							}
						}
					}
				}
			}

			pN = QOpen.Pop();
		}
	}

	//No path found
	return false;
};


//Check if there is a path from the given origin to the destination and place edge(s) as appropriate
void CXR_NavGraph_Builder::FindEdge(CNB_Pathfinder * _pPathFinder, int16 _iFrom, int16 _iTo)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_FindEdge, MAUTOSTRIP_VOID);
	//Search for edge. First check straight ground traversability
	int Cost;
	if ((Cost = _pPathFinder->StraightPath(m_lCNodes[_iFrom].m_GridPos, m_lCNodes[_iTo].m_GridPos)))
	{
		//Edge from this node to neighbour that can be traversed backwards
		PlaceEdge(_iFrom, _iTo, Cost);
		if (!HasEdgeTo(_iTo, _iFrom))
			PlaceEdge(_iTo, _iFrom, Cost);
	}
	else
	{
		//No straight traversability, use proper pathfinding
		int iEdge = _pPathFinder->AStarFindEdge(m_lCNodes[_iFrom].m_GridPos, m_lCNodes[_iTo].m_GridPos, 2 * m_MinDist -1, m_MinDist - 1 + _pPathFinder->MaxJump(), m_MinDist - 1 + _pPathFinder->MaxFall(), m_MinDist, Cost);
		if (iEdge == CNB_Pathfinder::DIRECTED_EDGE)
		{
			//Edge from this node to neighbour, but not backtraceable one. Place edge.
			PlaceEdge(_iFrom, _iTo, Cost);
		}
		else if (iEdge == CNB_Pathfinder::UNDIRECTED_EDGE)
		{
			//Edge from this node to neighbour that can be reversed exactly as is, place edges in both directions
			PlaceEdge(_iFrom, _iTo, Cost);
			if (!HasEdgeTo(_iTo, _iFrom))
				PlaceEdge(_iTo, _iFrom, Cost);
		}
	}
};

//Check if theer's an edge path from the origin to the destination that has a total cost lower than the 
//given one and does not traverse any edge wisth higher than the given edge cost.
bool CXR_NavGraph_Builder::HasEdgePathCostLimit(CNB_Pathfinder * _pPathFinder, int16 _iFrom, int16 _iTo, int _MaxCost, int _MaxEdgeCost)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_HasEdgePathCostLimit, false);
	//Fail immediately if start node has no edges
	if (m_lCNodes[_iFrom].NumEdges() == 0)
		return false;

	//Open and closed nodes with first empty index. Never expand more than 64 nodes
	TThinArray<CConstructionSearchNode> lNodes;
	lNodes.SetLen(64);
	lNodes[0] = CConstructionSearchNode(_iFrom, _pPathFinder->MinCost(m_lCNodes[_iFrom].m_GridPos, m_lCNodes[_iTo].m_GridPos), 0);
	int iFirstEmpty = 1;

	//Open nodes
	TPriorityQueue2<CConstructionSearchNode> QOpen;
	QOpen.Create(false,64);

	//Current node
	CConstructionSearchNode * pN = &(lNodes[0]);

	//Search until we find path, there's no more nodes to search or we fill node list
	while (pN)
	{
		//Are we at destination?
		if (pN->m_iNode == _iTo)
		{
			//Jupp, found path
			return true;
		}
		else
		{
			//Expand node
			for (int i = 0; i < m_lCNodes[pN->m_iNode].NumEdges(); i++)
			{
				//Prune edges with greater than max edge cost and edges which increase path cost 
				//beyond max total cost. Note that I don't prune existing nodes since any loop will 
				//be swiftly pruned due to cost (but I prune self-loops to be sure)
				if (m_lCEdges.ValidPos(m_lCNodes[pN->m_iNode].m_lEdges[i]) &&
					(m_lCEdges[m_lCNodes[pN->m_iNode].m_lEdges[i]].m_Cost <= _MaxEdgeCost))
				{
					int iDest = m_lCEdges[m_lCNodes[pN->m_iNode].m_lEdges[i]].m_iTo;
					if (iDest != pN->m_iNode)
					{
						int Heuristic = _pPathFinder->MinCost(m_lCNodes[iDest].m_GridPos, m_lCNodes[_iTo].m_GridPos);
						int Cost = pN->m_iCost + m_lCEdges[m_lCNodes[pN->m_iNode].m_lEdges[i]].m_Cost;
						if (Heuristic + Cost <= _MaxCost)
						{
							//Add node to open list if possible
							if (iFirstEmpty < lNodes.Len())
							{
								lNodes[iFirstEmpty] = CConstructionSearchNode(iDest, Heuristic, Cost);
								QOpen.Push(&(lNodes[iFirstEmpty]));
								iFirstEmpty++;
							}
							else
							{
								//Node list is full
								//LogFile(CStr("(NavGraph::HasEdgePathCostLimit) Warning: Node list full. Pathfinding incomplete."));
								return false;
							}
						}
					}
				}
			}

			pN = QOpen.Pop();
		}
	}

	//No path found
	return false;
};


//Fill the given list with the size groups in the given sizes flag field, sorted descendingly by base size. Optionally exclude the given size group from list.
void CXR_NavGraph_Builder::SortSizeGroups(int _Sizes, TArray<int>* _pRes, int _iExcludeSize)
{
	if (!_pRes)
		return;

	_pRes->Clear();

	_Sizes &= ~(1 << _iExcludeSize);
	int iSize = 0;
	while (_Sizes != 0)
	{
		if (_Sizes & 0x1)
		{
			_pRes->Add(iSize);
		}
		iSize++;
		_Sizes >>= 1;
	}

	//Sort (simple)
	for (int i = 0; i < _pRes->Len(); i++)
	{
		int iMax = i;
		int MaxBSize = CXR_NavGraph::GetBaseSize((*_pRes)[i]);
		int BSize;
		for (int j = i + 1; j < _pRes->Len(); j++)
		{
			BSize = CXR_NavGraph::GetBaseSize((*_pRes)[j]);
			if (BSize > MaxBSize)
			{
				iMax = j;
				MaxBSize = BSize;
			}
		}
		if (iMax != i)
		{
			//Swap
			int Tmp = (*_pRes)[iMax];
			(*_pRes)[iMax] = (*_pRes)[i];
			(*_pRes)[i] = Tmp;
		}
	}
};


//Check if existing edge is traversible for characters of given sizes and set size flags accordingly
void CXR_NavGraph_Builder::CheckEdgeSizes(CNB_Pathfinder * _pPathFinder, CConstructionEdge * _pEdge, int _nSizes, const int * _lSizes)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CheckEdge, MAUTOSTRIP_VOID);

	//Safety check...
	if (!m_lCNodes.ValidPos(_pEdge->m_iFrom) || !m_lCNodes.ValidPos(_pEdge->m_iTo))
		return;

	//Check each size group, starting with the largest (since the chance we can reuse a path of a 
	//larger size group is greater than the reverse). Keep last found path and just try to validate
	//this before doing a complete pathfinding. If we find a straight path we can skip further checks
	//(i.e. for smaller size groups) as long as they have lower height as well.
	int StraightPathHeight = 0;
	TArray<CVec3Dint16> CachedPath;
	int EdgeRes = CNB_Pathfinder::NO_EDGE;
	CVec3Dint16 From = m_lCNodes[_pEdge->m_iFrom].m_GridPos;
	CVec3Dint16 To = m_lCNodes[_pEdge->m_iTo].m_GridPos;
	int BaseSize, Height, SizeFlag, Dummy;
	CConstructionEdge * pMirrorEdge = NULL; //Edge that goes in reverse direction, set if needed
	bool bMirrorCheck = false;
	for (int i = 0; i < _nSizes; i++)
	{
		BaseSize = CXR_NavGraph::GetBaseSize(_lSizes[i]);
		Height = CXR_NavGraph::GetHeight(_lSizes[i]);
		SizeFlag = (1 << _lSizes[i]);

		//We might already have set size info if this is a mirror edge and the mirror has already been checked
		if (_pEdge->m_Sizes & SizeFlag)
		{
			//No need to set size or check for mirror
			continue;
		}
		//Check for previous straight path
		else if (StraightPathHeight >= Height)
		{
			//We already have found a straight path of suitable checked height for larger size, so we 
			//are guaranteed to have a straight path for this size as well
			EdgeRes = CNB_Pathfinder::UNDIRECTED_EDGE;
		}
		//Check for straight path
		else if (_pPathFinder->IsGroundTraversable(From, To, BaseSize, Height))
		{
			//Found straight path with greater height than previous
			StraightPathHeight = Height;
			EdgeRes = CNB_Pathfinder::UNDIRECTED_EDGE;
		}
		else 
		{
			//Try cached path if any
			if (_pPathFinder->CheckTraversability(CachedPath.Len(), CachedPath.GetBasePtr(), BaseSize, Height))
			{
				//Keep previous edge result
			}
			else
			{
				//Cached path no good, clear it and try proper pathfinding
				CachedPath.Clear();
				EdgeRes = _pPathFinder->AStarFindEdge(From, To, 2 * m_MinDist - 1, m_MinDist - 1 + _pPathFinder->MaxJump(), m_MinDist - 1 + _pPathFinder->MaxFall(), m_MinDist, Dummy, BaseSize, Height, &CachedPath);
			}
		}

		if (EdgeRes != CNB_Pathfinder::NO_EDGE)
		{
			//Edge is traversable for this size!
			_pEdge->m_Sizes |= SizeFlag;

			//If undirected, set size info for edge in opposite direction as well
			if (CNB_Pathfinder::UNDIRECTED_EDGE)
			{
				if (!bMirrorCheck)
				{
					//Find node this edge goes to and try to find a mirrored edge that comes back to the node this edge atarts from
					bMirrorCheck = true;
					CConstructionNode * pToNode = &(m_lCNodes[_pEdge->m_iTo]);
					for (int j = 0; j < pToNode->NumEdges(); j++)
					{
						if (m_lCEdges.ValidPos(pToNode->m_lEdges[j]) &&
							(m_lCEdges[pToNode->m_lEdges[j]].m_iTo == _pEdge->m_iFrom))
						{
							pMirrorEdge = &(m_lCEdges[pToNode->m_lEdges[j]]);
							break;
						}
					}
				}
				if (pMirrorEdge)
				{
					//Mirror edge found, set same size info
					pMirrorEdge->m_Sizes |= SizeFlag;
				}
			}
		}
	}
};

int CXR_NavGraph_Builder::FindNodeCandidatesFromRegistry(TArray<spCRegistry>& _lspObjects, CNB_Pathfinder& _Search)
{
	m_lObjGridPos.Clear();
	m_ObjectBoundMin = CVec3Dint16(-1);
	m_ObjectBoundMax = CVec3Dint16(-1);
	TArray<CVec3Dint16> lNavnode;		// Store NAVNODE here
	TArray<CVec3Dint16> lRoam;			// Store ROAM scenepoints here
	TArray<CVec3Dint16> lTactical;		// Store TACTICAL scenepoints here

	// _lspObjects is a list of CRegistry for each object
	// We iterate through them and retrieve their positions
	for (int i = 0; i < _lspObjects.Len(); i++)
	{
		CRegistry* pReg = _lspObjects[i];
		CStr Class = pReg->GetValue("CLASSNAME");
		Class.MakeUpperCase();
		if (Class.Find("NAVNODE") != -1)
		{
			CRegistry *pOrigin = pReg->FindChild("ORIGIN");
			if (pOrigin)
			{
				CVec3Dfp32 Pos;
				pOrigin->GetThisValueaf(3, Pos.k);
				CVec3Dint16 GridPos = _Search.GetGridPosition(Pos);
				GridPos = _Search.FindPathPosition(GridPos,5);
				if (_Search.IsValidGroundTraversable(GridPos))
				{
					lNavnode.Add(GridPos);
				}
			}
		}
		else if (Class.Find("SCENEPOINT") != -1)
		{	// Find the pos and radius to calculate it's extent
			CStr Type = pReg->GetValue("TYPE");
			Type.MakeUpperCase();
			if ((Type.Find("LOOK") != -1)||(Type.Find("DYNAMIC") != -1)||(Type.Find("WALKCLIMB") != -1)||(Type.Find("JUMPCLIMB") != -1))
			{
				continue;
			}

			// Skip perfect placement scenepoints
			CStr RadiusString = pReg->GetValue("RADIUS");
			if ((RadiusString != "")&&(RadiusString.Val_fp64() <= 8.0f))
			{
				continue;
			}

			// Only ROAM, TACTICAL and COVER are eligible
			if ((Type.Find("ROAM") != -1)||(Type.Find("TACTICAL") != -1)||(Type.Find("COVER") != -1))
			{	
				bool bAddToRoam = true;
				if (Type.Find("ROAM") != -1)
				{	// Skip scenepoints with SitFlag or Crouch
					CRegistry *pFlags = pReg->FindChild("FLAGS");
					if (pFlags)
					{
						CStr SFlags = pFlags->GetThisValue();
						SFlags.MakeUpperCase();
						if ((SFlags.Find("SIT") != -1)||(SFlags.Find("CROUCH") != -1)||(SFlags.Find("LOWPRIO") != -1))
						{
							continue;
						}
					}
					bAddToRoam = true;
				}
				else
				{	// Skip scenepoints with TACFLAGS or Crouch
					if (pReg->FindChild("TACFLAGS"))
					{
						continue;
					}
					CRegistry *pFlags = pReg->FindChild("FLAGS");
					if (pFlags)
					{
						CStr SFlags = pFlags->GetThisValue();
						SFlags.MakeUpperCase();
						if (SFlags.Find("CROUCH") != -1)
						{
							continue;
						}
					}
					bAddToRoam = false;
				}

				CRegistry *pOrigin = pReg->FindChild("ORIGIN");
				if (pOrigin)
				{
					CVec3Dfp32 Pos;
					pOrigin->GetThisValueaf(3, Pos.k);
					CVec3Dint16 GridPos = _Search.GetGridPosition(Pos);
					GridPos = _Search.FindPathPosition(GridPos,5);
					if (_Search.IsValidGroundTraversable(GridPos))
					{
						if (bAddToRoam)
						{
							lRoam.Add(GridPos);
						}
						else
						{
							lTactical.Add(GridPos);
						}
					}
				}
			}
		}
	}
	for(int i = 0; i < lNavnode.Len(); i++)
	{
		m_lObjGridPos.Add(lNavnode[i]);
	}
	for(int i = 0; i < lRoam.Len(); i++)
	{
		m_lObjGridPos.Add(lRoam[i]);
	}
	for(int i = 0; i < lTactical.Len(); i++)
	{
		m_lObjGridPos.Add(lTactical[i]);
	}

	return(m_lObjGridPos.Len());
};

CVec3Dint16 CXR_NavGraph_Builder::FindStartPosition(CCheckMatrix& _Check, int _Radius, int _Height)
{
	CVec3Dint16 Pos = CVec3Dint16(-1);
	int iFound = -1;
	for (int i = 0; i < m_lObjGridPos.Len(); i++)
	{
		Pos = m_lObjGridPos[i];
		if (!_Check.Get(Pos[0],Pos[1],Pos[2]))
		{
			iFound = i;
			m_lObjGridPos.Del(i);
			break;
		}
	}
	if (iFound >= 0)
	{	// Step through every pos and delete each node that is within _Radius
		for (int i = m_lObjGridPos.Len()-1; i >= 0; i--)
		{
			if ((Abs(Pos[0]-m_lObjGridPos[i][0]) <= _Radius)&&
				(Abs(Pos[1]-m_lObjGridPos[i][1]) <= _Radius)&&
				((Pos[2]-m_lObjGridPos[i][2]) <= _Height))
			{
				m_lObjGridPos.Del(i);
			}
		}

		return(Pos);
	}

	return(CVec3Dint16(-1));
};

void CXR_NavGraph_Builder::PlaceNodesFromRegistry(CCheckMatrix* _pCheck, int _CheckRadius, int _CheckHeight)
{
	int iMaxSqr = Sqr(_CheckRadius);
	for (int i = 0; i < m_lObjGridPos.Len(); i++)
	{
		CVec3Dint16 Pos = m_lObjGridPos[i];
		if ((!_pCheck->ValidPos(Pos[0],Pos[1],Pos[2]))||(_pCheck->Get(Pos[0],Pos[1],Pos[2]))) {continue;}
		bool bTooClose = false;
		for (int j = m_iEmptyNode-1; j >= 0; j--)
		{
			CVec3Dint16 jPos = m_lCNodes[j].m_GridPos;
			if (Pos.DistanceSqr(jPos) < iMaxSqr)
			{
				bTooClose = true;
				break;
			}
		}
		if (!bTooClose)
		{
			PlaceNode(Pos);
			if (_CheckRadius > 0)
			{
				_pCheck->Set(Pos[0],Pos[1],Pos[2],_CheckRadius,_CheckRadius,_CheckHeight);
			}
			else
			{
				_pCheck->Set(Pos[0],Pos[1],Pos[2]);
			}
		}
	}
};

// Adjust nodeposition to navnode within _Radius
bool CXR_NavGraph_Builder::AdjustNodePosition(CCheckMatrix* _pCheck,CVec3Dint16& _Pos,int _Radius,int _Height,bool _bRemove)
{
	for (int i = 0; i < m_lObjGridPos.Len(); i++)
	{
		CVec3Dint16 Pos = m_lObjGridPos[i];
		if (!_pCheck->Get(Pos[0],Pos[1],Pos[2]))
		{
			if ((Abs(_Pos[0]-Pos[0]) <= _Radius)&&
				(Abs(_Pos[1]-Pos[1]) <= _Radius)&&
				((_Pos[2]-Pos[2]) <= _Height))
			{
				_Pos = Pos;
				if (_bRemove)
				{
					m_lObjGridPos.Del(i);
				}
				return(true);
			}
		}
	}

	return(false);
};


//Construct graph, given navgrid, blocknav, starting point and desired minimum distance between nodes
void CXR_NavGraph_Builder::BuildGraph(CXR_BlockNav* _pBlockNav,TArray<spCRegistry>& _lspObjects,const CVec3Dint16& _Start, int _iMinDist, int _Sizes)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_BuildGraph, MAUTOSTRIP_VOID);
	TProfileDef(T);//DEBUG
	{
		TMeasureProfile(T);//DEBUG

		LogFile(CStrF("(NavGraph) Building graph. Grid dimensions: %d x %d x %d (%d Cells).", _pBlockNav->GridDimensions()[0], _pBlockNav->GridDimensions()[1], _pBlockNav->GridDimensions()[2], _pBlockNav->GridDimensions()[0] * _pBlockNav->GridDimensions()[1] * _pBlockNav->GridDimensions()[2]));
		if (!_pBlockNav)
		{
			LogFile("(NavGraph) WARNING: Invalid parameters, cannot build graph.");
			return;
		};

		//Set min distance and supported sizes. Default size is always included.
		m_MinDist = _iMinDist;
		_Sizes |= (1 << CXR_NavGraph::SIZE_DEFAULT);
		m_SupportedSizes = _Sizes;

		//Reset construction and normal graph 
		ClearConstruction();
		Clear();

		//Allocate space for nodes and edges. Start out with appropriately large number
		AllocateNodeSpace(_pBlockNav->GridDimensions()[0] * _pBlockNav->GridDimensions()[1] / Sqr(m_MinDist));
		AllocateEdgeSpace(4 * m_lCNodes.Len());

		//Currently we always use default size when building graph (even if this size wasn't given)
		//Set "square radius" and height in cells
		int iBaseSize = CXR_NavGraph::GetBaseSize(CXR_NavGraph::SIZE_DEFAULT);
		int iHeight = CXR_NavGraph::GetHeight(CXR_NavGraph::SIZE_DEFAULT);

		//Create search instance with appropriately large number of nodes allocated
		//to make sure any pathfinds will never run out of allocated space
		CNB_Pathfinder Search;
		// Search.Create(_pBlockNav, 1024, Min((int)(4 * Sqr(m_MinDist)), 32767), iBaseSize, iHeight);
		Search.Create(_pBlockNav, 1024, 32767, iBaseSize, iHeight);
		//Create initial check matrix. Check matrices should be at least 32 cells high but preferrably no more 
		//than 512 MCells (64 MByte)(i.e. xy-size of grid can be 4048x4048 cells before we get larger matrices)
		int CheckHeight = Min((int)(_pBlockNav->GridDimensions()[2]), Max(32, (int)((uint64)(1<<29) / ((uint64)(_pBlockNav->GridDimensions()[0]) * (uint64)(_pBlockNav->GridDimensions()[1])))));
		//int CheckHeight = Min((int)(_pBlockNav->GridDimensions()[2]), Max(32, (int)((uint64)(1<<24) / ((uint64)(_pBlockNav->GridDimensions()[0]) * (uint64)(_pBlockNav->GridDimensions()[1])))));//TEST! REPLACE WITH ABOVE LINE!
		int StartCheckLayer = _Start[2] / CheckHeight;
		int CurCheckLayer = StartCheckLayer;	
		int nCheckLayers = _pBlockNav->GridDimensions()[2] / CheckHeight + ((_pBlockNav->GridDimensions()[2] % CheckHeight) ? 1 : 0);
		LogFile(CStrF("Creating initial check matrix. Matrix height %d, %d subdivisions necessary", CheckHeight, nCheckLayers));
		CCheckMatrix Check = CCheckMatrix(_pBlockNav->GridDimensions()[0], _pBlockNav->GridDimensions()[1], CheckHeight, StartCheckLayer * CheckHeight);
		Search.Reset(&Check);

		//Set start position search min index and position. Start positions will firstly be searched for
		//at object positions, then the entire map will be scanned roughly to hopefully find any missed positions
		int iMinstart = 0;
		CVec3Dint16 MinStart = CVec3Dint16(0, 0, Check.GetLayerTop());

		//Find traversable ground position where first node can be placed
		TProfileDef(T_Nodes);
		{
			TMeasureProfile(T_Nodes);
			LogFile("(NavGraph) Placing nodes...");
			LogFile(CStr("(NavGraph) Find initial start position."));
			CVec3Dint16 Start = Search.FindPathPosition(_Start, 20);
			FindNodeCandidatesFromRegistry(_lspObjects, Search);
			if (Start == CVec3Dint16(-1))
			{
				Start = FindStartPosition(Check,m_MinDist,10);
			}
			Check.Set(Start[0], Start[1], Start[2]);
			// We use a coarser grid to ensure that (most) navnodes etc WILL be used
			//Place nodes until all potentially traversable areas have been visited
			while (CurCheckLayer < nCheckLayers)
			{
				while (Start != CVec3Dint16(-1))
				{
					LogFile(CStrF("(NavGraph) Place nodes starting with (%d, %d, %d)", Start[0], Start[1], Start[2]));
					int iNode = PlaceNode(Start);
					Check.Set(Start[0], Start[1], Start[2]);
					PlaceNodesFromRegistry(&Check,m_MinDist,10);
					//Traverse grid from start position, placing nodes as appropriate
					PlaceNodes_r(&Search, iNode);
					LogFile(CStrF("(NavGraph) Placed nodes starting with (%d, %d, %d). %d nodes placed in total.", Start[0], Start[1], Start[2], m_iEmptyNode));
					
					//Find new start node and set min start position accordingly
					CVec3Dint16 RegStart = FindStartPosition(Check,m_MinDist,10);
					if (RegStart == CVec3Dint16(-1))
					{
						RegStart = Search.FindStartPosition(Start,10);
					}
					Start = RegStart;
					if (Check.Get(Start[0],Start[1],Start[2]))
					{
						Start = CVec3Dint16(-1);
					}
				}

				//Check next layer
				if (CurCheckLayer == StartCheckLayer)
				{
					//Next layer is bottom layer (if start layer wasn't bottom of course)
					CurCheckLayer = (StartCheckLayer == 0) ? 1 : 0;
				}
				else
				{
					//Just check next layer, but skip start layer, since that was already checked in first iteration
					CurCheckLayer++;
					if (CurCheckLayer == StartCheckLayer)
						CurCheckLayer++;
				}
				if (CurCheckLayer < nCheckLayers)
				{
					Check = CCheckMatrix(_pBlockNav->GridDimensions()[0], _pBlockNav->GridDimensions()[1], CheckHeight, CurCheckLayer * CheckHeight);
					Search.Reset(&Check);
					MinStart = CVec3Dint16(0, 0, Check.GetLayerTop());
					Start = Search.FindStartPosition(MinStart,CheckHeight);
				}
			}

			//Cut down node list to current "filled" size (not really necessary but wtf...)
			m_lCNodes.SetLen(m_iEmptyNode);
		}
		//LogFile(CStrF("(NavGraph) Finished placing nodes, %d nodes placed in total (%f seconds).", m_iEmptyNode, T_Nodes.GetTime()));

		TProfileDef(T_Edges);
		int nCulled = 0;
		{
			TMeasureProfile(T_Edges);
			LogFile("(NavGraph) Placing edges...");

			//Set up hash for keeping track of node proximity
			/*
			LogFile("(NavGraph) Set up node proximity hash.");
			CProximityHash NodeHash(m_lCNodes.Len(), CVec3Dint16(iMaxDist), CVec3Dint16(_pBlockNav->GridDimensions()[0], _pBlockNav->GridDimensions()[1], _pBlockNav->GridDimensions()[2]));
			for (int i = 0; i < m_lCNodes.Len(); i++)
			{
				NodeHash.Add(i, m_lCNodes[i].m_GridPos);
			}
			*/

			//Place edges between adjacent nodes

			//Set "maximum distance" of an edge
			// int iMaxDist = Max((int)(2 * m_MinDist - 1), (int)(m_MinDist - 1 + Max(Search.MaxFall(), Search.MaxJump())));
			int iMaxDist = 2 * m_MinDist - 1;
			int MaxDistSqr = Sqr(iMaxDist);
			int MinDistSqr = Sqr(iMaxDist / 2);
			int iDistSqr;

			// Loop through EVERY node pair and if min/max range permits check for edges
			TProfileDef(T_FirstPass);
			{
				for (int i = 0; i < m_lCNodes.Len(); i++)
				{
					MRTC_SetProgress((fp32)i / (fp32)m_lCNodes.Len());
					for (int j = 0; j < m_lCNodes.Len(); j++)
					{
						if (i == j) { continue;}
						CVec3Dint16 iPos = m_lCNodes[i].m_GridPos;
						CVec3Dint16 jPos = m_lCNodes[j].m_GridPos;
						iDistSqr = Sqr(iPos[0]-jPos[0])+Sqr(iPos[1]-jPos[1])+Sqr(2*(iPos[2]-jPos[2]));
						// iDistSqr = iPos.DistanceSqr(jPos);
						if ((iDistSqr <= MaxDistSqr)&&(iDistSqr >= MinDistSqr))
						{
							if (!HasEdgeTo(i,j))
							{
								FindEdge(&Search, i, j);
							}
							if (!HasEdgeTo(j,i))
							{
								FindEdge(&Search, j, i);
							}
						}
					}
				}
			}
			//Check which nodes are destination for edges
			for (int i = 0; i < m_lCEdges.Len(); i++)
			{
				if (m_lCNodes.ValidPos(m_lCEdges[i].m_iTo))
				{
					m_lCNodes[m_lCEdges[i].m_iTo].m_Flags |= CXR_NavGraph_Node::FLAGS_ISEDGEDESTINATION;
				}
			}

			LogFile(CStrF("(NavGraph) %d edges placed (%f seconds).", m_iEmptyEdge, T_FirstPass.GetTime()));

			//First pass: Check for neighbours in all adjacent hash-boxes, but only make complete checks for very close neighbour candidates
			/*
			TProfileDef(T_FirstPass);
			{ 
				TMeasureProfile(T_FirstPass);
				LogFile("(NavGraph) First pass: placing edges between nearby nodes");
				for (int i = 0; i < m_lCNodes.Len(); i++)
				{
					MRTC_SetProgress((fp32)i / (fp32)m_lCNodes.Len());
					int iNeighbour;
					CVec3Dint16 Pos = m_lCNodes[i].m_GridPos;
					for (int x = Max((int)(Pos[0] - iMaxDist), 0); x <= Min((int)(Pos[0] + iMaxDist),(int)(_pBlockNav->GridDimensions()[0] - 1)); x += iMaxDist)
						for (int y = Max((int)(Pos[1] - iMaxDist), 0); y <= Min((int)(Pos[1] + iMaxDist),(int)(_pBlockNav->GridDimensions()[1] - 1)); y += iMaxDist)
							for (int z = Max((int)(Pos[2] - iMaxDist), 0); z <= Min((int)(Pos[2] + iMaxDist),(int)(_pBlockNav->GridDimensions()[2] - 1)); z += iMaxDist)
							{
								iNeighbour = NodeHash.GetFirst(CVec3Dint16(x, y, z));
								while (m_lCNodes.ValidPos(iNeighbour))
								{
									//Check if neighbour is close enough to be a true neighbour and that there's not already an edge to it.
									if ( (iNeighbour != i) &&
										((iDistSqr = Pos.DistanceSqr(m_lCNodes[iNeighbour].m_GridPos)) < MaxDistSqr) &&
										!HasEdgeTo(i, iNeighbour) )
									{
										//Check potential neighbours that are very close immediately (double height differences for this check)
										if (iDistSqr + 3 * Sqr(Pos[2] - m_lCNodes[iNeighbour].m_GridPos[2]) < VeryCloseSqr)
										{
											FindEdge(&Search, i, iNeighbour);
										}
										else
										{
											//Add to neighbour candidates of this node
											m_lCNodes[i].AddNeighbourCandidate(iNeighbour, iDistSqr);
										}
									}
									iNeighbour = NodeHash.GetNext();
								}
							}
				}
			}
			*/
			//LogFile(CStrF("(NavGraph) First pass finished, %d edges placed (%f seconds).", m_iEmptyEdge, T_FirstPass.GetTime()));

			//Second pass: check remaining neighbour candidates sorted by proximity
			/*
			TProfileDef(T_SecondPass);
			{
				TMeasureProfile(T_SecondPass);
				LogFile("(NavGraph) Second pass: placing remaining edges.");
				for (int i = 0; i < m_lCNodes.Len(); i++)
				{
					MRTC_SetProgress((fp32)i / (fp32)m_lCNodes.Len());
					int iNeighbour;
					m_lCNodes[i].SortNeighbourCandidates();

					for (int j = 0; j < m_lCNodes[i].NumNeighbourCandidates(); j++)
					{
						iNeighbour = m_lCNodes[i].m_lNeighbourCandidates[j].m_iNode;
						if (!m_lCNodes.ValidPos(iNeighbour))
							LogFile(CStrF("(NavGraph)Invalid neighbour candidate! (%d, %d)", iNeighbour, i));
	 					else
						{
							//LogFile(CStrF("(NavGraph)Valid neighbour candidate: (%d, %d)", iNeighbour, i));
							//Skip candidate if we've already placed an edge to it
							if (!HasEdgeTo(i, iNeighbour) &&
								!HasEdgePathStepLimit(i, iNeighbour))
							{
								FindEdge(&Search, i, iNeighbour);
							}
						}
					};
				}
			}
			*/
			//LogFile(CStrF("(NavGraph) Second pass finished, %d edges placed in total (%f seconds).", m_iEmptyEdge, T_SecondPass.GetTime()));

			//Cut down edge list to current "filled" size (not really necessary but wtf...)
			m_lCEdges.SetLen(m_iEmptyEdge);

			//Cull unnecessary edges
			LogFile(CStr("(NavGraph) Third pass: culling unnecessary nodes and edges."));
			TProfileDef(T_ThirdPass);
			{
				TMeasureProfile(T_ThirdPass);

				//Remove any nodes that don't have any edges from or to them
				//TODO!!!
                //for (i = 0; i < m_lCNodes.Len(); i++)
				//{
				//	if (!(m_lCNodes[i].m_Flags & CXR_NavGraph_Node::FLAGS_ISEDGEDESTINATION) &&
				//		(m_lCNodes[i].NumEdges() == 0))
				//	{
				//		//Cull node
				//		//TODO!!!!
				//	}
				//}
				//Sort all edges of all remaining nodes....
				for (int i = 0; i < m_lCNodes.Len(); i++)
				{
					MRTC_SetProgress((fp32)i / (fp32)(2 * m_lCNodes.Len()));

					//Sort edges, cheapest first
 					m_lCNodes[i].SortEdges(&m_lCEdges);
				}

				//Then cull those that can be replaced by a cheaper or slightly more expensive edge path
				for (int i = 0; i < m_lCNodes.Len(); i++)
				{
					MRTC_SetProgress((fp32)(i + m_lCNodes.Len()) / (fp32)(2 * m_lCNodes.Len()));

					//DEBUG
					//LogFile(CStrF("Node %d before culling:", i));
					//{for (int j = 0; j < m_lCNodes[i].NumEdges(); j++)
					//{
					//	if (m_lCEdges.ValidPos(m_lCNodes[i].m_lEdges[j]))
					//		LogFile(CStrF("Edge to %d, Cost %d", m_lCEdges[m_lCNodes[i].m_lEdges[j]].m_iTo, m_lCEdges[m_lCNodes[i].m_lEdges[j]].m_Cost));
					//}}
					//DEBUG

					//Cull any edges that can be replaced with an sequence of edges (where all edges are cheaper than 
					//the current edge) that is only slightly more expensive than the edge itself. 
					//Thus the cheapest edge of a node is never culled and need not be checked.
					for (int j = 1; j < m_lCNodes[i].NumEdges(); j++)
					{
						if (m_lCEdges.ValidPos(m_lCNodes[i].m_lEdges[j]))
						{
							CConstructionEdge * pEdge = &(m_lCEdges[m_lCNodes[i].m_lEdges[j]]);

							//If an edge is ground traversable, it will be far less likely to be culled
							fp32 CostFactor = 1.4f;//Search.IsGroundTraversable(m_lCNodes[i].m_GridPos, m_lCNodes[pEdge->m_iTo].m_GridPos) ? 1.4 : 3.0;

							if (HasEdgePathCostLimit(&Search, i, pEdge->m_iTo, int(pEdge->m_Cost * CostFactor), int(pEdge->m_Cost - 1)))
							{
								//Cull edge
								m_lCNodes[i].m_lEdges[j] = -1;
								nCulled++;
							}
						}
					}

					//DEBUG
					//LogFile(CStrF("Node %d after culling:", i));
					//{for (int j = 0; j < m_lCNodes[i].NumEdges(); j++)
					//{
					//	if (m_lCEdges.ValidPos(m_lCNodes[i].m_lEdges[j]))
					//		LogFile(CStrF("Edge to %d, Cost %d", m_lCEdges[m_lCNodes[i].m_lEdges[j]].m_iTo, m_lCEdges[m_lCNodes[i].m_lEdges[j]].m_Cost));
					//}}
					//LogFile("");
					//DEBUG
				}
			}
			LogFile(CStrF("(NavGraph) Third pass finished, %d edges culled (%f seconds).", nCulled, T_ThirdPass.GetTime()));

			//Decorate edges with size info if we have other size groups than default
			if (_Sizes & ~(1 << CXR_NavGraph::SIZE_DEFAULT))
			{
				LogFile(CStr("(NavGraph) Fourth pass: Set size info on edges."));
				TProfileDef(T_FourthPass);
				TMeasureProfile(T_FourthPass);

				//Set up additional size group list, starting with largest base size
				TArray<int> lSizeGroups;
				SortSizeGroups(_Sizes, &lSizeGroups, CXR_NavGraph::SIZE_DEFAULT);
				LogFile(CStr("(NavGraph) Additional Size Groups:"));
				{
					for (int i = 0; i < lSizeGroups.Len(); i++)
					{
						int BaseSize = int((CXR_NavGraph::GetBaseSize(lSizeGroups[i]) * 2 + 1) * _pBlockNav->UnitsPerCell());
						int Height = int(CXR_NavGraph::GetHeight(lSizeGroups[i]) * _pBlockNav->UnitsPerCell());
						LogFile(CStrF("(NavGraph)    %s (Base: %d, Height:%d world units).", CXR_NavGraph::ms_lTranslateSizeGroup[lSizeGroups[i]], BaseSize, Height));
					}
				}
		
				//Check each size group for each edge of each node.
				CConstructionEdge * pEdge;
				for (int i = 0; i < m_lCNodes.Len(); i++)
				{
					for (int j = 0; j < m_lCNodes[i].NumEdges(); j++)
					{
						if (m_lCEdges.ValidPos(m_lCNodes[i].m_lEdges[j]))
						{
							pEdge = &(m_lCEdges[m_lCNodes[i].m_lEdges[j]]);
							CheckEdgeSizes(&Search, pEdge, lSizeGroups.Len(), lSizeGroups.GetBasePtr());
						}
					}

					MRTC_SetProgress((fp32)i / (fp32)m_lCNodes.Len());
				}

				//LogFile(CStrF("(NavGraph) Fourth pass finished (%f seconds).", T_FourthPass.GetTime()));
			}
			else
			{
				LogFile(CStr("(NavGraph) Fourth pass skipped: No additional size groups."));
			}
		}
		//LogFile(CStrF("(NavGraph) Finished placing edges, %d edges placed (%f seconds).", m_iEmptyEdge - nCulled, T_Edges.GetTime()));

		TProfileDef(T_Build);
		{
			TMeasureProfile(T_Build);
			
			//Build navgraph from construction graph
			m_lNodes.SetLen(m_lCNodes.Len());
			m_lEdges.SetLen(m_lCEdges.Len());
			int iEmptyEdge = 0;
			for (int i = 0; i < m_lCNodes.Len(); i++)
			{
				m_lNodes[i].m_GridPos = m_lCNodes[i].m_GridPos;
				m_lNodes[i].m_iiEdges = iEmptyEdge;
				int j;
				int nEdges = 0;
				for (j = 0; j < m_lCNodes[i].NumEdges(); j++)
				{
					//Check if edge index is valid (i.e. hasn't been culled)
					int iEdge = m_lCNodes[i].m_lEdges[j];
					if (m_lCEdges.ValidPos(iEdge))
					{
						m_lEdges[iEmptyEdge].m_iFrom = m_lCEdges[iEdge].m_iFrom;
						m_lEdges[iEmptyEdge].m_iTo = m_lCEdges[iEdge].m_iTo;

						//Convert the navgrid cost in the construction graph to navgraph cost, and clamp it at max cost
						m_lEdges[iEmptyEdge].m_Cost = Min(CXR_NavGraph_Edge::MAX_COST, (int)(m_lCEdges[iEdge].m_Cost / CXR_NavGraph_Edge::COST_FACTOR));

						m_lEdges[iEmptyEdge].m_Sizes = m_lCEdges[iEdge].m_Sizes;

						iEmptyEdge++;
						nEdges++;
					}
				}
				m_lNodes[i].m_nEdges = nEdges;
				m_lNodes[i].m_HeightLevels = m_lCNodes[i].m_HeightLevels;
				m_lNodes[i].m_Flags = m_lCNodes[i].m_Flags;
			};
			m_lEdges.SetLen(iEmptyEdge);
		}
		//LogFile(CStrF("(NavGraph) Copied graph (%f seconds).", T_Build.GetTime()));

		//Clear construction graph
		ClearConstruction();

		//DEBUG
	}
	LogFile(CStrF("(NavGraph) Finished: %d Nodes, %d Edges. Total build time: %.f seconds.", m_lNodes.Len(), m_lEdges.Len(), T.GetTime())); //DEBUG
	//LogFile("");
	//DEBUG
};

//CNB_Pathfinder; Auxiliary pathfinding class/////////////////////////////////////////

CXR_NavGraph_Builder::CNB_Pathfinder::CNB_Pathfinder()
	:CXBN_SearchInstance()
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CNB_Pathfinder_CNB_Pathfinder, MAUTOSTRIP_VOID);
	m_iBaseSize = 0;
	m_iHeight = 1;
	m_pCheck = NULL;
	m_iSearchMode = SEARCH_DEFAULT;
}


//Set initialization stuff
void CXR_NavGraph_Builder::CNB_Pathfinder::Create(CXR_BlockNav* _pBlockNav, int _TileHeap, int _MaxNodes, int _iBaseSize, int _iHeight)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CNB_Pathfinder_Create, MAUTOSTRIP_VOID);
	CXBN_SearchInstance::Create((_pBlockNav) ? _pBlockNav->GetGrid() : NULL, _pBlockNav, _TileHeap, _MaxNodes);
	m_iBaseSize = _iBaseSize;
	m_iHeight = _iHeight;
	m_iSearchMode = SEARCH_DEFAULT;
};

void CXR_NavGraph_Builder::CNB_Pathfinder::Reset(CCheckMatrix * _pCheck)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CNB_Pathfinder_Reset, MAUTOSTRIP_VOID);
	m_pCheck = _pCheck;
};



//Check if position has a node on hash if _iParam is 0 (A* mode for normal searches)
//Check if position has a node on hash or is checked in check matrix if _iParam is 1 (breadth-first mode for placing nodes)
bool CXR_NavGraph_Builder::CNB_Pathfinder::CanPrune(const CVec3Dint16& _Pos)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CNB_Pathfinder_CanPrune, false);
	//Different pruning check due to search mode
	if (m_iSearchMode == SEARCH_PLACENODES_BF)
	{
		if (m_pCheck &&
			m_pCheck->Get(_Pos[0], _Pos[1], _Pos[2]))
		{
			//Already visited, prune
			return true;
		}
	}

	//Check if the node has already been pushed onto (and possibly off) queue
	const CXBN_SearchNode* pNodeCell = Hash_GetNode(_Pos[0], _Pos[1], _Pos[2]);
	if (pNodeCell)
	{
		// For now, prune all nodes we've already visited. Will fix to full A*.
		return true;	
	}
	else
		return false;
};	


//Insert node in hash. Don't mark as visited until we expand node.
void CXR_NavGraph_Builder::CNB_Pathfinder::VisitNode(CXBN_SearchNode* _pN)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CNB_Pathfinder_VisitNode, MAUTOSTRIP_VOID);
	if (_pN->m_iNode)
	Hash_InsertNode(_pN->m_iNode);
};


//Helper "constant" array
static int s_i2DDirs[8][2] = 
{   //Straights
	{1,0},{0,1},{0,-1},{-1,0},
	//Diagonals
	{1,1},{1,-1},{-1,1},{-1,-1}	
};

CVec3Dint16 CXR_NavGraph_Builder::CNB_Pathfinder::GetGridPosition(CVec3Dfp32 _Pos)
{
	if (!m_pBlockNav) 
		return CVec3Dint16(-1);
	
	return(m_pBlockNav->GetGridPosition(_Pos));
};

//Find closest traversable ground position in grid given grid position within a radius of 20 cells
CVec3Dint16 CXR_NavGraph_Builder::CNB_Pathfinder::FindPathPosition(const CVec3Dint16& _Start, int _Radius)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CNB_Pathfinder_FindPathPosition, CVec3Dint16(0,0,0));

	if (_Start == CVec3Dint16(-1))
		return _Start;

	if (!m_pBlockNav) 
		return CVec3Dint16(-1);

	//Set params
	m_Params.m_iBaseSize = m_iBaseSize;
	m_Params.m_iHeight = m_iHeight;
	m_Params.m_bWallClimb = false;
	if (IsOutsideGridBoundaries(_Start))
		return CVec3Dint16(-1);

	//Pass 0
	if ( !m_pCheck->Get(_Start[0], _Start[1], _Start[2]) &&
		 IsOnGround(_Start) && 
		 IsTraversable(_Start) )
		return _Start;
	else
	{
		//Pass 1+
		uint16 MaxUp = _Radius;
		uint16 MaxDown = _Radius;
		CVec3Dint16 Check;
		int iCheckRadius;
		int iLevel;
		for (int i = 1; i <= _Radius; i++)
		{
			//Check straight down if possible
			if (i <= MaxDown)
			{
				Check = _Start - CVec3Dint16(0,0,i);
				if (IsOutsideGridBoundaries(Check))
				{
					//We can't go further downwards, since we're alredy below the grid floor
					MaxDown = i;
				
					//Debug_RenderWire(_Pos, m_pBlockNav->GetWorldPosition(Check), 0xffffff00, 5);//DEBUG
				}
				else if ((!m_pCheck->Get(Check[0], Check[1], Check[2]))&&
						  (IsOnGround(Check))&& 
						  (IsTraversable(Check)))
				{
					//Debug_RenderWire(_Pos, m_pBlockNav->GetWorldPosition(Check), 0xffff0000, 5);//DEBUG

					//Found traversable ground cell!
					return Check;
				};
			};

			//Check straight up if possible
			if (i <= MaxUp)
			{
				Check = _Start + CVec3Dint16(0,0,i);
				if (IsOutsideGridBoundaries(Check))
				{
					//We can't go further upwards, since we're alredy above the grid ceiling
					MaxUp = i;
				
					//Debug_RenderWire(_Pos, m_pBlockNav->GetWorldPosition(Check), 0xffffff00, 5);//DEBUG
				}
				else if ( !m_pCheck->Get(Check[0], Check[1], Check[2]) &&
						  IsOnGround(Check) && 
						  IsTraversable(Check) )
				{
					//Debug_RenderWire(_Pos, m_pBlockNav->GetWorldPosition(Check), 0xffff0000, 5);//DEBUG

					//Found traversable ground cell!
					return Check;
				}
			};

			//Check around the center column if appropriate 
			iCheckRadius = 1;
			while ( (iCheckRadius <= i) &
				    (iCheckRadius <= _Radius) )
			{
				//Set the height offset level we're currently checking at
				iLevel = i - iCheckRadius;

				//Should we check below? (or at same level if iLevel == 0)
				if (iLevel <= MaxDown)
				{
					for (int j = 0; j < 8; j++)
					{
						//Check cells around the center column and below the center 
						Check[0] = _Start[0] + s_i2DDirs[j][0] * iCheckRadius;
						Check[1] = _Start[1] + s_i2DDirs[j][1] * iCheckRadius;
						Check[2] = _Start[2] - iLevel;
						if ( !m_pCheck->Get(Check[0], Check[1], Check[2]) &&
							 IsOnGround(Check) && 
							 IsTraversable(Check) )
						{
							//Debug_RenderWire(_Pos, m_pBlockNav->GetWorldPosition(Check), 0xffff0000, 5);//DEBUG

							//Found traversable ground cell!
							return Check;
						};
						
						//Debug_RenderWire(_Pos, m_pBlockNav->GetWorldPosition(Check), 0xffffff00, 5);//DEBUG
					};
				};

				//Should we check above? (Don't check if we're at level 0, since we've already checked those)
				if (iLevel && (iLevel <= MaxUp))
				{
					for (int j = 0; j < 8; j++)
					{
						//Check cells around the center column and below the center 
						Check[0] = _Start[0] + s_i2DDirs[j][0] * iCheckRadius;
						Check[1] = _Start[1] + s_i2DDirs[j][1] * iCheckRadius;
						Check[2] = _Start[2] + iLevel;
						if ( !m_pCheck->Get(Check[0], Check[1], Check[2]) &&
							 IsOnGround(Check) && 
							 IsTraversable(Check) )
						{
							//Debug_RenderWire(_Pos, m_pBlockNav->GetWorldPosition(Check), 0xffff0000, 5);//DEBUG

							//Found traversable ground cell!
							return Check;
						};

						//Debug_RenderWire(_Pos, m_pBlockNav->GetWorldPosition(Check), 0xffffff00, 5);//DEBUG
					};
				};
				
				iCheckRadius++;
			};
			
			//Stop checking if we cannot check anything else
			if (i >= Max(MaxDown, MaxUp))
				break;
		};
		
		//Fail if we can't check anything else
		return CVec3Dint16(-1);
	};
};


//Finds traversable ground position that hasn't been visited yet by brute-force search
//Rough scan of the layer starting at _MinStart value and going down _CheckHeight levels. 
//The _TightCheckMax/Min values spans the box where the scan should be more careful.
CVec3Dint16 CXR_NavGraph_Builder::CNB_Pathfinder::FindStartPosition(CVec3Dint16& _MinStart, int16 _CheckHeight)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CNB_Pathfinder_FindStartPosition, CVec3Dint16(0,0,0));
	if (!m_pCheck)
		return CVec3Dint16(-1);	
	
	//Set params.
	m_Params.m_iBaseSize = m_iBaseSize;
	m_Params.m_iHeight = m_iHeight;
	m_Params.m_bWallClimb = false;
	// Note: We should mark graphnode edges that rely on m_bJump being true so that bots don't use them when idle.
	// Idle bots cannot pathfind down jumps and would therefore be unable to follow certain edges.
	// Due to some really steep staricase designs in P5 we need to have this enabled, otherwise the graphing will fail
	m_Params.m_bJump = false;
	
	//Scan over every 32'th xy position and every 8'th level or every 8'th xy position and every level 
	//in the tight area
	//This should be optimized in a better way...fix?
	CVec3Dint16 Pos;
	bool bFirstLevel = true;
	bool bFirstRow = true;
	int StartZ = Min((int)_MinStart[2], (int)(m_pBlockNav->GridDimensions()[2] - m_iHeight));
	int StopZ = Max(0, _MinStart[2] - _CheckHeight);
	int x,y,z;
	for (z = StartZ; z >= StopZ; z -= 1)
	{
		//If we're still on first level then use minstart value
		int YStart = (bFirstLevel) ? Max((int)_MinStart[1], (int)m_iBaseSize) : m_iBaseSize;
		for (y = YStart; y < m_pBlockNav->GridDimensions()[1] - m_iBaseSize - 1; y += 16)
		{
			//If we're still on first row then use minstart value
			int XStart = (bFirstRow) ? Max((int)_MinStart[0], (int)m_iBaseSize) : m_iBaseSize;
			for (x = XStart; x < m_pBlockNav->GridDimensions()[0] - m_iBaseSize - 1; x += 16)
			{
				Pos = CVec3Dint16(x, y, z);
				
				if ( !m_pCheck->Get(x, y, z) && 
					 IsOnGround(Pos) && 
					 IsTraversable(Pos) )
				{
					_MinStart = Pos;
					return Pos;
				}
			}
			bFirstRow = false;
		}
		bFirstLevel = false;
	}

	//No valid ground position found
	_MinStart = Pos;
	return CVec3Dint16(-1);	
};

//Check if we can move in a straight line from one position to another, without jumping or falling
//Note that I use crouch height here, so this path might not actually be straight line traversable, 
//but it is definitly traversable.
int CXR_NavGraph_Builder::CNB_Pathfinder::StraightPath(const CVec3Dint16& _From, const CVec3Dint16& _To)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CNB_Pathfinder_StraightPath, 0);
	if (!m_pBlockNav)
		return false;
	
	if (m_pBlockNav->IsGroundTraversableImpl(_From,_To, m_iBaseSize, m_iHeight, false, 1, 1, NULL))
	{
		//Success! Cost is simply grid distance multiplied by appropriate modifiers, 
		//since we travel in straight path, without any falls and jumps (might be along edges though, but I'll ignore that for now)
		return int(WBN_DISTANCEFACTOR * Length3(WBN_COSTFRACTIONS * (_From[0] - _To[0]), WBN_COSTFRACTIONS * (_From[1] - _To[1]), WBN_COSTFRACTIONS * (_From[2] - _To[2])));
	}
	else
	{
		return 0;
	}
};


//Finds all unvisited positions at given depth that can be reached from given position.
//found positions in air is expanded until a ground position is found, or deleted if no such can be found
//The result is added to the given array and the number of found valid positions is returned.
int CXR_NavGraph_Builder::CNB_Pathfinder::BreadthFirstFindPositions(const CVec3Dint16& _StartPos, int _iDepth, TThinArray<CPosCost> * _plResult)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CNB_Pathfinder_BreadthFirstFindPositions, 0);
	//Since I use the priority queue of the base class, this isn't proper breadth first. 
	//Pathfinding structure could do with a rewrite...

	if (!_plResult || !m_pCheck || !m_pBlockNav)
		return 0;

	//Init queue, hash and node heap index
	m_PQueue.Create(false, m_MaxNodes);
	Hash_Clear();
	m_iNodeNext = 0;

	//Set up params
	m_Params.m_Src = _StartPos;
	m_Params.m_Dst = _StartPos;	//"Destination" is start position, so all node distances will be distance to start
	m_Params.m_iBaseSize = m_iBaseSize;
	m_Params.m_iHeight = m_iHeight;
	m_Params.m_bWallClimb = false;
	m_Params.m_bJump = false;

	//Create start node and add it to hash
	CXBN_SearchNode * pN = NewNode();
	if (!pN) 
		return 0;
	if (!IsOnGround(_StartPos))
		pN->m_Flags |= WBN_NODE_AIRBORNE;
	pN->m_Distance = 0;
	pN->m_Pos = _StartPos;
	Hash_InsertNode(pN->m_iNode);

	//Search until we find node at wanted depth 
	m_iSearchMode = SEARCH_PLACENODES_BF;
	_iDepth = int(_iDepth * WBN_COSTFRACTIONS * WBN_DISTANCEFACTOR); //Convert node-depth to depth in world units
	while (pN && (pN->m_Distance < _iDepth))
	{
		//DEBUG
		//if (pN && m_pServer && m_pServer->Path_GetBlockNav() && pN->m_pParent)
		//	m_pServer->Debug_RenderWire(m_pServer->Path_GetBlockNav()->GetWorldPosition(pN->m_pParent->m_Pos),
		//								m_pServer->Path_GetBlockNav()->GetWorldPosition(pN->m_Pos), 
		//								0xffff0000, 1000);
		//DEBUG

		//Mark expanded nodes as visited
		m_pCheck->Set(pN->m_Pos[0], pN->m_Pos[1], pN->m_Pos[2]);
		
		//Expand node, pushing children onto queue
		PushChildren(pN);
		
		pN = m_PQueue.Pop();
	}
	//Reset search mode
	m_iSearchMode = SEARCH_DEFAULT;

	//Did search fail?
	if (!pN)
	{
		return 0;
	}
	else
	{
		//Found node at wanted depth (or deeper). Return this and all other nodes currently on queue.
		//(i.e. all nodes with wanted depth). 
		int iEmpty = 0;
		_plResult->SetLen(64);
		while (pN)
		{
			//Is this air cell?
			if (pN->m_Flags & WBN_NODE_AIRBORNE)
			{
				//DEBUG
				//if (pN && m_pServer && m_pServer->Path_GetBlockNav() && pN->m_pParent)
				//{
				//	m_pServer->Debug_RenderWire(m_pServer->Path_GetBlockNav()->GetWorldPosition(pN->m_pParent->m_Pos),
				//								m_pServer->Path_GetBlockNav()->GetWorldPosition(pN->m_Pos), 
				//								0xffff0000, 1000);
				//}
				//DEBUG

				//Mark air cells as visited and expand them (we only want ground positions in result) 
				m_pCheck->Set(pN->m_Pos[0], pN->m_Pos[1], pN->m_Pos[2]);
				PushChildren(pN);
			}
			else
			{
				//DEBUG
				//if (pN && m_pServer && m_pServer->Path_GetBlockNav() && pN->m_pParent)
				//	m_pServer->Debug_RenderWire(m_pServer->Path_GetBlockNav()->GetWorldPosition(pN->m_pParent->m_Pos),
				//								m_pServer->Path_GetBlockNav()->GetWorldPosition(pN->m_Pos), 
				//								0xffff0000, 1000);
				//DEBUG

				//Add position of ground node.
				if (iEmpty >= _plResult->Len())
					_plResult->SetLen(iEmpty + 64);
				(*_plResult)[iEmpty] = CPosCost(pN->m_Pos, pN->GetPriority());
				iEmpty++;
			}

			_plResult->SetLen(iEmpty);
			pN = m_PQueue.Pop();
		}

		return _plResult->Len();
	}
};



//Checks if there's a path from the origin to the destination that is shorter than the given ground distance. 
//or, if ending the path with a fall or jump, shorter than the given jump and fall distances. If the ground tail 
//param is set, paths with a fall or jump are allowed to have this number of ground cells after the jump/fall
int CXR_NavGraph_Builder::CNB_Pathfinder::AStarFindEdge(const CVec3Dint16& _From, const CVec3Dint16& _To, int _iGroundDist, int _iJumpDist, int _iFallDist, int _iGroundTail, int& _iCost, int _BaseSize, int _Height, TArray<CVec3Dint16> * _plResult)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CNB_Pathfinder_AStarFindEdge, 0);
	if (!m_pBlockNav)
		return NO_EDGE;

	//Init queue, hash and node heap index
	m_PQueue.Create(false, m_MaxNodes);
	Hash_Clear();
	m_iNodeNext = 0;

	//Set up params
	m_Params.m_Src = _From;
	m_Params.m_Dst = _To;	
	m_Params.m_iBaseSize = ((_BaseSize == -1) ? m_iBaseSize : _BaseSize);
	m_Params.m_iHeight = ((_Height == -1) ? m_iHeight : _Height);
	m_Params.m_bWallClimb = false;
	m_Params.m_bJump = false;

	//Unless default size, check traversability of destination (if origin is intraversable we'll find out soon enough anyway)
	if ((m_Params.m_iBaseSize > CXR_NavGraph::GetBaseSize(CXR_NavGraph::SIZE_DEFAULT)) ||
		(m_Params.m_iHeight > CXR_NavGraph::GetHeight(CXR_NavGraph::SIZE_DEFAULT)))
	{
		if (!IsTraversable(m_Params.m_Dst))
		{
			//Intraversable destination, fail at once
			return NO_EDGE;
		}
	}

	//Create start node and add it to hash
	CXBN_SearchNode * pN = NewNode();
	if (!pN) 
		return NO_EDGE;
	if (!IsOnGround(_From))
		pN->m_Flags |= WBN_NODE_AIRBORNE;
	pN->m_Distance = RoundToInt(WBN_DISTANCEFACTOR * Length3(WBN_COSTFRACTIONS *(m_Params.m_Dst[0] - m_Params.m_Src[0]), WBN_COSTFRACTIONS * (m_Params.m_Dst[1] - m_Params.m_Src[1]), WBN_COSTFRACTIONS * (m_Params.m_Dst[2] - m_Params.m_Src[2])));
	pN->m_Pos = _From;
	Hash_InsertNode(pN->m_iNode);

	//Square max distances for faster comparisons and clamp ground tail to max value of searchnode counter
	int iGroundDistSqr = Sqr(_iGroundDist);
	int iJumpDistSqr = Sqr(_iJumpDist);
	int iFallDistSqr = Sqr(_iFallDist);
	int iGroundTail = Min((int)_iGroundTail, 15);

	int iMaxCost = int(_iGroundDist * 4 * WBN_COSTFRACTIONS * WBN_DISTANCEFACTOR);

	//Only use fall and jump distances if these are longer than ground distance
	bool bUseSpecialFlags = (_iGroundDist < Max(_iJumpDist, _iFallDist));

	//Search until we find a path or there are no more valid nodes to expand
	m_iSearchMode = SEARCH_FINDEDGE_AS;
	while (pN)
	{
		//DEBUG
		//if (pN && m_pServer && m_pServer->Path_GetBlockNav() && pN->m_pParent)
		//	m_pServer->Debug_RenderWire(m_pServer->Path_GetBlockNav()->GetWorldPosition(pN->m_pParent->m_Pos),
		//								m_pServer->Path_GetBlockNav()->GetWorldPosition(pN->m_Pos), 
		//								0xffff0000, 1000);
		//DEBUG

		//Have we reached destination?
		if (pN->m_Pos == m_Params.m_Dst)
		{
			if (_plResult)
			{
				//Get result. First get reverse result (end to start sequence), then reverse it
				TArray<CVec3Dint16> lReverse;
				lReverse.SetGrow(32); //More efficient adding, waste some memory
				CXBN_SearchNode * pNode = pN;
				while (pNode) 
				{
					lReverse.Add(pNode->m_Pos);
					pNode = pNode->m_pParent;
				}
				int Length = lReverse.Len();
			 	_plResult->SetLen(Length);
				for (int i = 0; i < Length; i++)
				{
					//Set result to start to end sequence
					(*_plResult)[i][0] = lReverse[Length - i - 1][0];
					(*_plResult)[i][1] = lReverse[Length - i - 1][1];
					(*_plResult)[i][2] = lReverse[Length - i - 1][2];
				}
			}

			//We found path which is close enough to destination. Reset search mode, set cost and 
			//check if found path is undirected (i.e. can be traced backwards)
			m_iSearchMode = SEARCH_DEFAULT;
			_iCost = pN->GetPriority();
			while (pN)
			{	// If this node and previous are both airborne or jump
				if ((pN->m_Flags & (WBN_NODE_AIRBORNE | WBN_PARENT_NODE_JUMP))&&
					(pN->m_pParent)&&(pN->m_pParent->m_Flags & (WBN_NODE_AIRBORNE | WBN_PARENT_NODE_JUMP)))
					//Cannot trace falls and jump exactly backwards
					return DIRECTED_EDGE;

				pN = pN->m_pParent;
			}
			//Path can be traced exactly backwards
			return UNDIRECTED_EDGE;
		}

		//Prune nodes with too high cost
		if (pN->GetPriority() < iMaxCost)
		{
			if (bUseSpecialFlags)
			{
				//Set any special flags
				if ((pN->m_Flags & WBN_PARENT_NODE_JUMP))
				{
					//Jump node
					pN->m_Flags |= WBN_NODE_JUMPPATH;
					pN->m_Counter = iGroundTail;
				}
				else if (pN->m_Flags & WBN_NODE_AIRBORNE)
				{
					//Air node; jump or fall node
					pN->m_Counter = iGroundTail;
					if (pN->m_pParent && (pN->m_pParent->m_Flags & WBN_NODE_JUMPPATH))
					{
						//Jump node
						pN->m_Flags |= WBN_NODE_JUMPPATH;
					}
					else
					{
						//Fall node
						pN->m_Flags |= WBN_NODE_FALLPATH;
					}
				}
				else if (pN->m_pParent)
				{
					//Ground node, propagate jump/fall paths from parents until counter is exhausted
					if (pN->m_pParent->m_Counter > 0)
					{
						if (pN->m_pParent->m_Flags & WBN_NODE_JUMPPATH)
						{
							pN->m_Flags |= WBN_NODE_JUMPPATH;
							pN->m_Counter = pN->m_pParent->m_Counter - 1;
						}
						else if (pN->m_pParent->m_Flags & WBN_NODE_FALLPATH)
						{
							pN->m_Flags |= WBN_NODE_FALLPATH;
							pN->m_Counter = pN->m_pParent->m_Counter - 1;
						}
					}
				}
			}

			//Only expand nodes that are close enough to origin. Use jump, fall or ground distance, as appropriate
			int iMaxDist = (pN->m_Flags & WBN_NODE_JUMPPATH) ? iJumpDistSqr : ((pN->m_Flags & WBN_NODE_FALLPATH) ? iFallDistSqr : iGroundDistSqr);
			if (m_Params.m_Src.DistanceSqr(pN->m_Pos) < iMaxDist)
				//Expand node, pushing children onto queue
				PushChildren(pN);
		}
		
		pN = m_PQueue.Pop();
	}

	//Reset search mode and fail
	m_iSearchMode = SEARCH_DEFAULT;
	_iCost = 0;
	return NO_EDGE;
};


//The maximum fall height permitted by pathfinding, in cells
int CXR_NavGraph_Builder::CNB_Pathfinder::MaxFall()
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CNB_Pathfinder_MaxFall, 0);
	//Should really be calculated consistently with pathfinding, but wtf.
	return 25;
};


//The maximum distance one can jump according to pathfinding, in cells. 
int CXR_NavGraph_Builder::CNB_Pathfinder::MaxJump()
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CNB_Pathfinder_MaxJump, 0);
	return 49;//Approx sqrt(sqr(30 * sqrt(2)) + sqr(MaxFall)) since we can jump diagonally and fall quite a bit
};

//Minimum "world unit" cost of traversal between the given nodes
int CXR_NavGraph_Builder::CNB_Pathfinder::MinCost(CVec3Dint16 _From, CVec3Dint16 _To)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CNB_Pathfinder_MinCost, 0);
	return RoundToInt(WBN_DISTANCEFACTOR*Length3(WBN_COSTFRACTIONS*(_From[0] - _To[0]), 
												 WBN_COSTFRACTIONS*(_From[1] - _To[1]), 
												 WBN_COSTFRACTIONS*(_From[2] - _To[2])));
}


//Check if one can move between given positions in a straight line without having to jump or fall too much
bool CXR_NavGraph_Builder::CNB_Pathfinder::IsGroundTraversable(CVec3Dint16 _From, CVec3Dint16 _To, int _BaseSize, int _Height)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CNB_Pathfinder_IsGroundTraversable, false);
	if (_BaseSize == -1)
		_BaseSize = m_iBaseSize;
	if (_Height == -1)
		_Height = m_iHeight;

	return m_pBlockNav->IsGroundTraversableImpl(_From, _To, _BaseSize, _Height, 1, 1);
};

//Check traversability of given sequence of grid positions
bool CXR_NavGraph_Builder::CNB_Pathfinder::CheckTraversability(int _nPos, const CVec3Dint16 * _lPos, int _BaseSize, int _Height)
{	
	//Check start
	if ((_nPos < 1) || !m_pBlockNav->IsTraversableImpl(_lPos[0], _BaseSize, _Height, false))
		return false;
	
	//Check rest
	CVec3Dint16 dPos;
	for (int i = 1; i < _nPos; i++)
	{
		if (!m_pBlockNav->IsTraversableImpl(_lPos[i], _BaseSize, _Height, false, _lPos[i - 1][0] - _lPos[i][0], _lPos[i - 1][1] - _lPos[i][1], _lPos[i - 1][2] - _lPos[i][2]))
			return false;
	}

	//Traversable all the way!
	return true;
};

bool CXR_NavGraph_Builder::CNB_Pathfinder::IsValidGroundTraversable(CVec3Dint16 _Start)
{
	if (IsOnGround(_Start))
	{
		if (IsTraversable(_Start))
		{
			return(true);
		}
	}
	return(false);
};


//CCheckMatrix; auxiliary bitmatrix class for keeping track of visited positions//////

//Constructor. Sets dimensions and resets all bits
CXR_NavGraph_Builder::CCheckMatrix::CCheckMatrix(int16 _x, int16 _y, int16 _z, int16 _BaseHeight)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CCheckMatrix_ctor, MAUTOSTRIP_VOID);
	//Set length
	int64 Len = (int64)_x * (int64)_y * (int64)_z;	
	Len = (Len >> 6) + ((Len & 63) ? 1 : 0);
	m_lMatrix.SetLen(Len);
	
	//Set dimension factors
	m_iYZ = _y * _z;
	m_iZ = _z;

	m_BaseHeight = _BaseHeight;

	//Reset all data
	for (int i = 0; i < Len; i++)
	{
		m_lMatrix[i] = 0;
	}
};

//Get value at given position
bool CXR_NavGraph_Builder::CCheckMatrix::Get(int16 _x, int16 _y, int16 _z)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CCheckMatrix_Get, false);
	int64 iIndex = (int64)_x * (int64)m_iYZ + (int64)_y * (int64)m_iZ + (int64)(_z - m_BaseHeight);	
	int8 iSlot = iIndex & 63;
	iIndex = iIndex >> 6;

	if (!m_lMatrix.ValidPos(iIndex) || (_z < m_BaseHeight))
		//Ouside of check matrix, consider checked.
		return true;

	return (m_lMatrix[iIndex] & ((int64)1 << iSlot)) != 0;
};

//Set value at given position to 1
void CXR_NavGraph_Builder::CCheckMatrix::Set(int16 _x, int16 _y, int16 _z)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CCheckMatrix_Set, MAUTOSTRIP_VOID);
	int64 iIndex = (int64)_x * (int64)m_iYZ + (int64)_y * (int64)m_iZ + (int64)(_z - m_BaseHeight);	
	int8 iSlot = iIndex & 63;
	iIndex = iIndex >> 6;

	if (!m_lMatrix.ValidPos(iIndex) || (_z < m_BaseHeight))
		return;

	m_lMatrix[iIndex] |= ((int64)1 << iSlot);
};

// Sets all points within _x +-_dx, _z +-_dz, _z +-_dz
void CXR_NavGraph_Builder::CCheckMatrix::Set(int16 _x, int16 _y, int16 _z,int16 _dx, int16 _dy, int16 _dz)
{
	for (int z = _z; z <= _z+_dz; z++)
		for (int y = _y-_dy; y <= _y+_dy; y++)
			for (int x = _x-_dx; x <= _x+_dx; x++)
			{
				Set(x, y, z);
			}
};


void CXR_NavGraph_Builder::CCheckMatrix::Set()
{
	//Set all data
	int Len = m_lMatrix.Len();
	for (int i = 0; i < Len; i++)
	{
		m_lMatrix[i] = -1;
	}
	int64 test = m_lMatrix[0];	// ***
	test = 0;
};

void CXR_NavGraph_Builder::CCheckMatrix::Reset()
{
	//Reset all data
	int Len = m_lMatrix.Len();
	for (int i = 0; i < Len; i++)
	{
		m_lMatrix[i] = 0;
	}
};

//Set value at given position to 0
void CXR_NavGraph_Builder::CCheckMatrix::Reset(int16 _x, int16 _y, int16 _z)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CCheckMatrix_Reset, MAUTOSTRIP_VOID);
	int64 iIndex = (int64)_x * (int64)m_iYZ + (int64)_y * (int64)m_iZ + (int64)(_z - m_BaseHeight);	
	int8 iSlot = iIndex & 63;
	iIndex = iIndex >> 6;
	
	if (!m_lMatrix.ValidPos(iIndex) || (_z < m_BaseHeight))
		return;

	m_lMatrix[iIndex] &= ~((int64)1 << iSlot);
};


//Check if given position is in check matrix
bool CXR_NavGraph_Builder::CCheckMatrix::ValidPos(int16 _x, int16 _y, int16 _z)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CCheckMatrix_ValidPos, false);
	int64 iIndex = (int64)_x * (int64)m_iYZ + (int64)_y * (int64)m_iZ + (int64)(_z - m_BaseHeight);	
	iIndex = iIndex >> 6;
	
	return m_lMatrix.ValidPos(iIndex)  && (_z >= m_BaseHeight);
};

//Get layer bottom and top grid heights
int16 CXR_NavGraph_Builder::CCheckMatrix::GetLayerBottom()
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CCheckMatrix_GetLayerBottom, 0);
	return m_BaseHeight;
};
int16 CXR_NavGraph_Builder::CCheckMatrix::GetLayerTop()
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CCheckMatrix_GetLayerTop, 0);
	return m_BaseHeight + m_iZ - 1;
};


//CConstructionSearchNode///////////////////////////////////////////////

CXR_NavGraph_Builder::CConstructionSearchNode::CConstructionSearchNode(int16 _iNode, int16 _iHeuristicCost, int16 _iCost)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CConstructionSearchNode_ctor, MAUTOSTRIP_VOID);
	m_iNode = _iNode;
	m_iHeuristicCost = _iHeuristicCost;
	m_iCost = _iCost;
};


int CXR_NavGraph_Builder::CConstructionSearchNode::GetPriority()
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CConstructionSearchNode_GetPriority, 0);
	return m_iHeuristicCost + m_iCost;
};



//CConstructionNode/////////////////////////////////////////////////////

CXR_NavGraph_Builder::CConstructionNode::CConstructionNode()
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CConstructionNode_ctor, MAUTOSTRIP_VOID);
	m_iFirstEmpty = 0;	
	m_iEmptyCandidate = 0;
	m_HeightLevels = 0;
	m_Flags = 0;
};

//Helper to QSort_r
void CXR_NavGraph_Builder::CConstructionNode::Swap(TThinArray<CNeighbourCandidate>* _pNCs, int _i, int _j)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CConstructionNode_Swap, MAUTOSTRIP_VOID);
	//Swap item
	CNeighbourCandidate Tmp = (*_pNCs)[_i];
	(*_pNCs)[_i] = (*_pNCs)[_j];
	(*_pNCs)[_j] = Tmp;
};


//Sorts the neighbour candidates ascendingly based on their distsqr values
void CXR_NavGraph_Builder::CConstructionNode::QSortNCs_r(TThinArray<CNeighbourCandidate>* _pNCs, int _iStart, int _iEnd)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CConstructionNode_QSortNCs_r, MAUTOSTRIP_VOID);
	//Check for final case
	if (_iStart >= _iEnd)
	{
		return;
	}

	//Get pivot value
	int Pivot = (*_pNCs)[((_iEnd - _iStart) / 2) + _iStart].m_iDistSqr;

	//Loop through list until indices cross
	int iStart = _iStart;
	int iEnd = _iEnd;
	while (iStart <= iEnd)
	{
        //Find the first value that is greater than or equal to the pivot .
        while( (iStart < _iEnd ) && 
			   ((*_pNCs)[iStart].m_iDistSqr < Pivot) )
           iStart++;

        //Find the last value that is smaller than or equal to the pivot .
        while( (iEnd > _iStart) && 
			   ((*_pNCs)[iEnd].m_iDistSqr > Pivot) )
           iEnd--;

        //If the indexes have not crossed, swap stuff
        if( iStart <= iEnd ) 
        {
           Swap(_pNCs, iStart, iEnd);
           iStart++;
           iEnd--;
        }
	}

	//Sort left partition if end index hasn't reached start
	if( _iStart < iEnd )
		QSortNCs_r(_pNCs, _iStart, iEnd);

	//Sort right partition if start index hasn't reached end
	if( iStart < _iEnd )
		QSortNCs_r(_pNCs, iStart, _iEnd);
};

//Sorts edge indices ascendingly based on the costs of their corresponding construction edges 
void CXR_NavGraph_Builder::CConstructionNode::QSortEdges_r(TThinArray<int32>* _pEdges, int _iStart, int _iEnd, TThinArray<CConstructionEdge>* _pCEdges)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CConstructionNode_QSortEdges_r, MAUTOSTRIP_VOID);
	//Check for final case
	if (_iStart >= _iEnd)
	{
		return;
	}

	//Get pivot value
	int iEdge = (*_pEdges)[((_iEnd - _iStart) / 2) + _iStart];
	int Pivot = (*_pCEdges)[iEdge].m_Cost;

	//Loop through list until indices cross
	int iStart = _iStart;
	int iEnd = _iEnd;
	while (iStart <= iEnd)
	{
        //Find the first value that is greater than or equal to the pivot .
        while( (iStart < _iEnd ) && 
			   ((*_pCEdges)[(*_pEdges)[iStart]].m_Cost < Pivot) )
           iStart++;

        //Find the last value that is smaller than or equal to the pivot .
        while( (iEnd > _iStart) && 
			   ((*_pCEdges)[(*_pEdges)[iEnd]].m_Cost > Pivot) )
           iEnd--;

        //If the indexes have not crossed, swap end and start edge indices
        if( iStart <= iEnd ) 
        {
			int32 tmp = (*_pEdges)[iStart];
			(*_pEdges)[iStart] = (*_pEdges)[iEnd];
			(*_pEdges)[iEnd] = tmp;
			iStart++;
			iEnd--;
        }
	}

	//Sort left partition if end index hasn't reached start
	if( _iStart < iEnd )
		QSortEdges_r(_pEdges, _iStart, iEnd, _pCEdges);

	//Sort right partition if start index hasn't reached end
	if( iStart < _iEnd )
		QSortEdges_r(_pEdges, iStart, _iEnd, _pCEdges);
};


//Add edge index
void CXR_NavGraph_Builder::CConstructionNode::AddEdge(int32 _iEdge)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CConstructionNode_AddEdge, MAUTOSTRIP_VOID);
	if (m_lEdges.Len() <= m_iFirstEmpty)
	{
		//Increase edge space for this node
		m_lEdges.SetLen(2 * m_iFirstEmpty + 1);
	}
	m_lEdges[m_iFirstEmpty] = _iEdge;
	m_iFirstEmpty++;
};

//Add neighbour candidate index
void CXR_NavGraph_Builder::CConstructionNode::AddNeighbourCandidate(int16 _iNode, int32 _iDistSqr)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CConstructionNode_AddNeighbourCandidate, MAUTOSTRIP_VOID);
	if (m_lNeighbourCandidates.Len() <= m_iEmptyCandidate)
	{
		//Increase neighbour candidate space for this node
		m_lNeighbourCandidates.SetLen(2 * m_iEmptyCandidate + 1);
	}
	m_lNeighbourCandidates[m_iEmptyCandidate] = CNeighbourCandidate(_iNode, _iDistSqr);
	m_iEmptyCandidate++;

	//LogFile("NCs:");
	//CNeighbourCandidate * pNCs[20];
	//for (int i = 0; i < Min(20, m_iEmptyCandidate); i++)
	//{
	//	pNCs[i] = &(m_lNeighbourCandidates[i]);
	//	LogFile(CStrF("%d", pNCs[i]->m_iNode));
	//}
};

//Sort neighbour candidates
void CXR_NavGraph_Builder::CConstructionNode::SortNeighbourCandidates()
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CConstructionNode_SortNeighbourCandidates, MAUTOSTRIP_VOID);
	//Sort candidates
	QSortNCs_r(&m_lNeighbourCandidates, 0, m_iEmptyCandidate - 1);
};

//Sort edges ascedingly according to cost
void CXR_NavGraph_Builder::CConstructionNode::SortEdges(TThinArray<CConstructionEdge>* _pCEdges)
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CConstructionNode_SortEdges, MAUTOSTRIP_VOID);
	QSortEdges_r(&m_lEdges, 0, m_iFirstEmpty - 1, _pCEdges);
};


int CXR_NavGraph_Builder::CConstructionNode::NumEdges()
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CConstructionNode_NumEdges, 0);
	return m_iFirstEmpty;
};

int CXR_NavGraph_Builder::CConstructionNode::NumNeighbourCandidates()
{
	MAUTOSTRIP(CXR_NavGraph_Builder_CConstructionNode_NumNeighbourCandidates, 0);
	return m_iEmptyCandidate;
};

//#endif
