#include "PCH.h"

#include "WNavGraph_Pathfinder.h"
#include "../../../XR/XRBlockNav.h"
#include "../Server/WServer.h"

MRTC_IMPLEMENT(CWorld_Navgraph_Pathfinder, CReferenceCount);

MRTC_IMPLEMENT_DYNAMIC(CWorld_Navgraph_Pathfinder_AStar, CWorld_Navgraph_Pathfinder);


//Get hash list index given position
int16 C3DHash::HashValue(const CVec3Dint16& _Pos)
{
	MAUTOSTRIP(C3DHash_HashValue, 0);
	return ((_Pos[0] & 0x7) << 6) + ((_Pos[1] & 0x7) << 3) + (_Pos[2] & 0x7);
};

//Set max size of hash
C3DHash::C3DHash(int _Size)
{
	MAUTOSTRIP(C3DHash_ctor, MAUTOSTRIP_VOID);
	m_lHashElts.SetLen(_Size);
	m_liHash.SetLen(512);
	Clear();
};

//Add index with position. Fails if position is invalid or hash is full
bool C3DHash::Add(int16 _iIndex, int16 _iIndex2, const CVec3Dint16& _Pos)
{
	MAUTOSTRIP(C3DHash_Add, false);
	//Check if element array is full
	if (m_iFirstEmpty >= m_lHashElts.Len())
		return false;

	//Check if position is valid
	int iHash = HashValue(_Pos);	
	if (!m_liHash.ValidPos(iHash))
		return false;

	//Add node index to element list
	m_lHashElts[m_iFirstEmpty].m_iIndex = _iIndex;		
	m_lHashElts[m_iFirstEmpty].m_iIndex2 = _iIndex2;		

	//Insert node into hash list
	m_lHashElts[m_iFirstEmpty].m_iNext = m_liHash[iHash];		
	m_liHash[iHash] = m_iFirstEmpty;

	m_iFirstEmpty++;
	return true;
};


//Check if index with given position is in hash
bool C3DHash::Find(int16 _iIndex, const CVec3Dint16& _Pos)
{
	MAUTOSTRIP(C3DHash_Find, false);
	//Check if position is valid
	int iHash = HashValue(_Pos);	
	if (!m_liHash.ValidPos(iHash))
		return false;

	//Check elements
	int iElt = m_liHash[iHash];
	while (m_lHashElts.ValidPos(iElt))
	{
		if (m_lHashElts[iElt].m_iIndex == _iIndex)
			return true;

		iElt = m_lHashElts[iElt].m_iNext;		
	}
	return false;
};


//Get corresponding index2 of index with given position, or -1 if not in hash
int16 C3DHash::GetIndex2(int16 _iIndex, const CVec3Dint16& _Pos)
{
	MAUTOSTRIP(C3DHash_GetIndex2, 0);
	//Check if position is valid
	int iHash = HashValue(_Pos);	
	if (!m_liHash.ValidPos(iHash))
		return -1;

	//Check elements
	int iElt = m_liHash[iHash];
	while (m_lHashElts.ValidPos(iElt))
	{
		if (m_lHashElts[iElt].m_iIndex == _iIndex)
			return m_lHashElts[iElt].m_iIndex2;

		iElt = m_lHashElts[iElt].m_iNext;		
	}
	return -1;
};


//Clear hash elements
void C3DHash::Clear()
{
	MAUTOSTRIP(C3DHash_Clear, MAUTOSTRIP_VOID);
	m_iFirstEmpty = 0;
	for (int i = 0; i < m_liHash.Len(); i++)
		m_liHash[i] = -1;
};


//Approx distance to destination from given node 
int16 CWorld_Navgraph_Pathfinder_AStar::CSearchInstance_AStar::Heuristic(int16 _iNode)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_CSearchInstance_AStar_Heuristic, 0);
	int iRes = m_pGraph->m_lNodes[m_iDest].m_GridPos.Distance(m_pGraph->m_lNodes[_iNode].m_GridPos);
	return Min(iRes, 32767);
};


void CWorld_Navgraph_Pathfinder_AStar::CSearchInstance_AStar::Init(CXR_NavGraph * _pGraph, CWorld_Navgraph_Pathfinder_AStar * _pPF, int _nNodes)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_CSearchInstance_AStar_Init, MAUTOSTRIP_VOID);
	m_pGraph = _pGraph;
	m_pPF = _pPF;
	m_lSearchNodes.SetLen(_nNodes);	
	m_bAllocated = false;
	m_NodeHash = C3DHash(_nNodes);
	Search_Reset();
};


void CWorld_Navgraph_Pathfinder_AStar::CSearchInstance_AStar::Search_Reset()
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_CSearchInstance_AStar_Search_Reset, MAUTOSTRIP_VOID);
	m_PQueue.Create(false, m_lSearchNodes.Len());
	m_NodeHash.Clear();		
	m_iFirstEmpty = 0;

	m_iOrigin = -1;
	m_iDest = -1;
	
	m_lAlternates.Clear();	
	m_MaxCost = -1;
	m_pDest = NULL;

	m_Status = CWorld_Navgraph_Pathfinder::SEARCH_INVALID;
};


void CWorld_Navgraph_Pathfinder_AStar::CSearchInstance_AStar::Search_Create(int _iOrigin, int _iDestination, int _Size, const int16* _lAlternates, int _nAlternates, int _iUser, int _MaxCost)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_CSearchInstance_AStar_Search_Create, MAUTOSTRIP_VOID);
	Search_Reset();
	if (!m_pGraph->m_lNodes.ValidPos(_iOrigin) ||
		!m_pGraph->m_lNodes.ValidPos(_iDestination) ||
		(_Size == CXR_NavGraph::SIZE_INVALID))
	{
		m_Status = CWorld_Navgraph_Pathfinder::SEARCH_INVALID;
		return;
	}

	m_Size = _Size;
	m_iUser = _iUser;

	m_iOrigin = _iOrigin;
	m_iDest	= _iDestination;

	m_lAlternates.Clear();
	if (_lAlternates)
		m_lAlternates.Insertx(0, _lAlternates, _nAlternates);
	
	m_MaxCost = _MaxCost;

	//Create start node, and add it to hash and queue
	m_lSearchNodes[m_iFirstEmpty] = CSearchNode_AStar(m_iOrigin, NULL, 0, Heuristic(m_iOrigin));
	m_NodeHash.Add(m_iOrigin, m_iFirstEmpty, m_pGraph->m_lNodes[m_iOrigin].m_GridPos);
	m_PQueue.Push(&(m_lSearchNodes[m_iFirstEmpty]));
	m_iFirstEmpty++;

	m_Status = CWorld_Navgraph_Pathfinder::SEARCH_IN_PROGRESS;
};


int CWorld_Navgraph_Pathfinder_AStar::CSearchInstance_AStar::Search_Execute(int& _nNodeExpansions, int * _pAlternateFound)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_CSearchInstance_AStar_Search_Execute, 0);
	if ((m_Status != CWorld_Navgraph_Pathfinder::SEARCH_IN_PROGRESS) &&
		(m_Status != CWorld_Navgraph_Pathfinder::SEARCH_FOUND_ALTERNATE))
	{
		//Search is done
		return m_Status;
	}

	//Search! (We always allow one node expansion, regardless of given value)
	CSearchNode_AStar * pN = m_PQueue.Pop();
	while (pN)
	{
		//Are we done?
		if (pN->m_iNode == m_iDest)
		{
			//Path found!
			m_pDest = pN;
			m_Status = CWorld_Navgraph_Pathfinder::SEARCH_FOUND_PATH;
			return CWorld_Navgraph_Pathfinder::SEARCH_FOUND_PATH;
		}

		//Expand current node
		_nNodeExpansions--;
		uint16 iEdge;
		int16 iChild; 
		int Cost;
		int HCost;
		int i;
		for (i = 0; i < m_pGraph->m_lNodes[pN->m_iNode].m_nEdges; i++)
		{	
			iEdge = m_pGraph->m_lNodes[pN->m_iNode].m_iiEdges + i;
			iChild = m_pGraph->m_lEdges[iEdge].m_iTo;
			if (m_pGraph->m_lNodes.ValidPos(iChild) &&
			    m_pPF->TraversableEdge(iEdge, m_Size) &&
			    !m_NodeHash.Find(iChild, m_pGraph->m_lNodes[iChild].m_GridPos))
			{
				Cost = pN->m_Cost + (m_pPF->EdgeCost(iEdge) * CXR_NavGraph_Edge::DISTANCE_FACTOR), 
				HCost = Heuristic(iChild);
				if ((m_MaxCost < 0) ||
					(Cost + HCost <= m_MaxCost))
				{
					//Check if we've run out of node space
					if (m_iFirstEmpty >= m_lSearchNodes.Len())
					{
						//Can't allocate any more nodes!
						m_Status = CWorld_Navgraph_Pathfinder::SEARCH_NO_PATH;
						return CWorld_Navgraph_Pathfinder::SEARCH_NO_PATH;
					}
			
					//Valid edge found, add child	
					m_lSearchNodes[m_iFirstEmpty] = CSearchNode_AStar(iChild, pN, Cost, HCost);
					m_NodeHash.Add(iChild, m_iFirstEmpty, m_pGraph->m_lNodes[iChild].m_GridPos);
					m_PQueue.Push(&(m_lSearchNodes[m_iFirstEmpty]));
					m_iFirstEmpty++;
				}
			}	
		}

		if (_pAlternateFound)
		{	
			//Check if current node is alternate destination (note that we must expand them before 
			//we can be allowed to break for any found alternates, so that search can continue)
			for (i = 0; i < m_lAlternates.Len(); i++)
			{
				if (pN->m_iNode == m_lAlternates[i])
				{
					//Found alternate!
					*_pAlternateFound = m_lAlternates[i];	
					m_Status = CWorld_Navgraph_Pathfinder::SEARCH_FOUND_ALTERNATE;
					return CWorld_Navgraph_Pathfinder::SEARCH_FOUND_ALTERNATE;
				}
			}
		}

		//Should we break now?
		if (_nNodeExpansions <= 0)
		{
			m_Status = CWorld_Navgraph_Pathfinder::SEARCH_IN_PROGRESS;
			return CWorld_Navgraph_Pathfinder::SEARCH_IN_PROGRESS;
		}

		//Get next node
		pN = m_PQueue.Pop();
		
	}
	//No more nodes, fail
	m_Status = CWorld_Navgraph_Pathfinder::SEARCH_NO_PATH;
	return CWorld_Navgraph_Pathfinder::SEARCH_NO_PATH;
};


bool CWorld_Navgraph_Pathfinder_AStar::CSearchInstance_AStar::Search_Get(CWorld_NavGraph_Path * _pRes, int _iEndNode, bool _bPartial)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_CSearchInstance_AStar_Search_Get, false);
	if (!_pRes || !m_pGraph || (m_Status == CWorld_Navgraph_Pathfinder::SEARCH_INVALID))	
		return false;

	//End search node defines path
	CSearchNode_AStar * pEndNode = NULL;

	//Should we get path ending in specific node?
	if (_iEndNode != -1)
	{
		if (!m_pGraph->m_lNodes.ValidPos(_iEndNode))
			return false;
		
		//Find search node index in hash
		int iSearchNode = m_NodeHash.GetIndex2(_iEndNode, m_pGraph->m_lNodes[_iEndNode].m_GridPos);
		pEndNode = (m_lSearchNodes.ValidPos(iSearchNode)) ? &(m_lSearchNodes[iSearchNode]) : NULL;
	}
	else if ((m_Status = CWorld_Navgraph_Pathfinder::SEARCH_FOUND_PATH) && m_pDest)
	{
		//Get found path
		pEndNode = m_pDest;
	}
	else if (_bPartial)
	{
		//Get path ending close to destination 
		CVec3Dint16 DestPos = m_pGraph->m_lNodes[m_iDest].m_GridPos;			
		int Threshold = int(m_pGraph->m_lNodes[m_iOrigin].m_GridPos.DistanceSqr(DestPos) * (0.5f * 0.5f)); //Threshold at half distance from origin to destination			

		//Iterate over search nodes, starting from last node. Find first node which is
		//within threshold value from destination, or node closest to detination. 
		//Never iterate over more than 128 nodes.
		int16 MinDistSqr = 32767;
		int16 DistSqr;
		int iEnd = Min(m_iFirstEmpty, m_lSearchNodes.Len()) - 1;
		int16 iMinNode = iEnd;
		int Step = ((iEnd + 1) / 128) + 1;
		for (int i = iEnd; i >= 0; i -= Step)
		{
			//Is node valid?
			if (m_pGraph->m_lNodes.ValidPos(m_lSearchNodes[i].m_iNode))
			{
				DistSqr = m_pGraph->m_lNodes[m_lSearchNodes[i].m_iNode].m_GridPos.DistanceSqr(DestPos);
				if (DistSqr < Threshold)
				{
					//Found node within threshold!
					pEndNode = &(m_lSearchNodes[i]);
				}
				else if ( (DistSqr < MinDistSqr) &&
						  (DistSqr > 0) ) //Precaution againt modest overflow)	
				{
					//Found best candidate as yet
					iMinNode = i;
					MinDistSqr = DistSqr;
				};
			}
		}
		if (!pEndNode)
		{
			//Couldn't find node beyond threshold, use best node
			pEndNode = &(m_lSearchNodes[iMinNode]);
		}
	}

	if (!pEndNode)
		return false;

	//Count length of path
	int iLen = 0;
	CSearchNode_AStar * pN = pEndNode;
	while (pN)
	{
		iLen++;
		pN = pN->m_pParent;
	}

	//Build result
	TThinArray<int16> lRes;
	lRes.SetLen(iLen);
	pN = pEndNode;
	for (int i = iLen - 1; i >= 0; i--)
	{
		lRes[i] = pN->m_iNode;
		pN = pN->m_pParent;
	}
	_pRes->BuildPath(m_pGraph, lRes.GetBasePtr(), iLen);

	return true;
};

//User of search instance
int CWorld_Navgraph_Pathfinder_AStar::CSearchInstance_AStar::Search_GetUser()
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_CSearchInstance_AStar_Search_GetUser, 0);
	if (m_bAllocated)
		return m_iUser;
	else
		return 0;
};


bool CWorld_Navgraph_Pathfinder_AStar::CSearchInstance_AStar::IsAllocated(int _GameTick)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_CSearchInstance_AStar_IsAllocated, false);
	if ((_GameTick == -1) || (m_LastReleasedTick == -1))
		return m_bAllocated;
	else
		return m_bAllocated || (_GameTick == m_LastReleasedTick);
};


void CWorld_Navgraph_Pathfinder_AStar::CSearchInstance_AStar::Allocate()
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_CSearchInstance_AStar_Allocate, MAUTOSTRIP_VOID);
	m_bAllocated = true;
};


void CWorld_Navgraph_Pathfinder_AStar::CSearchInstance_AStar::Release(int _GameTick)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_CSearchInstance_AStar_Release, MAUTOSTRIP_VOID);
	m_bAllocated = false;
	m_LastReleasedTick = _GameTick;
};


//Debug render search instance
void CWorld_Navgraph_Pathfinder_AStar::CSearchInstance_AStar::DebugRender(CWorld_Server * _pServer)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_CSearchInstance_AStar_DebugRender, MAUTOSTRIP_VOID);
	CXR_BlockNavSearcher* pSearcher = (_pServer) ? _pServer->Path_GetBlockNavSearcher() : NULL;
	CXR_BlockNav* pGridPF = (pSearcher) ? pSearcher->GetBlockNav() : NULL;
	if (!pGridPF)
		return;

	//Render all search nodes in search node list
	CVec3Dfp32 From, To;
	for (int i = 0; i < m_iFirstEmpty; i++)
	{
		if (m_lSearchNodes[i].m_pParent && 
			m_pGraph->m_lNodes.ValidPos(m_lSearchNodes[i].m_pParent->m_iNode) &&
			m_pGraph->m_lNodes.ValidPos(m_lSearchNodes[i].m_iNode))
		{
			From = pGridPF->GetWorldPosition(m_pGraph->m_lNodes[m_lSearchNodes[i].m_pParent->m_iNode].m_GridPos);
			To = pGridPF->GetWorldPosition(m_pGraph->m_lNodes[m_lSearchNodes[i].m_iNode].m_GridPos);
			_pServer->Debug_RenderWire(From, To, 0xffffff00, 0.05f);
		}
	}
};



//Helper to QSort_r
void CWorld_Navgraph_Pathfinder_AStar::Swap(TThinArray<int16>* _pItems, TThinArray<int>* _pValues, int _i, int _j)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_Swap, MAUTOSTRIP_VOID);
	int iTmp;

	//Swap item
	iTmp = (*_pItems)[_i];
	(*_pItems)[_i] = (*_pItems)[_j];
	(*_pItems)[_j] = iTmp;

	//Swap value
	iTmp = (*_pValues)[_i];
	(*_pValues)[_i] = (*_pValues)[_j];
	(*_pValues)[_j] = iTmp;
};


//Sorts the first array ascendingly based on the values in the second array
void CWorld_Navgraph_Pathfinder_AStar::QSort_r(TThinArray<int16>* _pItems, TThinArray<int>* _pValues, int _iStart, int _iEnd)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_QSort_r, MAUTOSTRIP_VOID);
	//Check for final case
	if (_iStart >= _iEnd)
	{
		return;
	}

	//Get pivot value
	int Pivot = (*_pValues)[(_iEnd - _iStart) / 2];

	//Loop through list until indices cross
	int iStart = _iStart;
	int iEnd = _iEnd;
	while (iStart <= iEnd)
	{
        //Find the first value that is greater than or equal to the pivot .
        while( (iStart < _iEnd ) && 
			   ((*_pValues)[iStart] < Pivot) )
           iStart++;

        //Find the last value that is smaller than or equal to the pivot .
        while( (iEnd > _iStart) && 
			   ((*_pValues)[iEnd] > Pivot) )
           iEnd--;

        //If the indexes have not crossed, swap stuff
        if( iStart <= iEnd ) 
        {
           Swap(_pItems, _pValues, iStart, iEnd);
           iStart++;
           iEnd--;
        }
	}

	//Sort left partition if end index hasn't reached start
	if( _iStart < iEnd )
		QSort_r(_pItems, _pValues, _iStart, iEnd);

	//Sort right partition if start index hasn't reached end
	if( iStart < _iEnd )
		QSort_r(_pItems, _pValues, iStart, _iEnd);
};


//Check if given edge can be reliably traversed by agent of given size
bool CWorld_Navgraph_Pathfinder_AStar::TraversableEdge(uint16 _iEdge, int _Size)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_TraversableEdge, false);
	if (!m_lEdgeStates.ValidPos(_iEdge)) //If edge state is valid, then edge is valid
		return false;

//	int sz = m_pGraph->m_lEdges[_iEdge].m_Sizes;

	return (m_lEdgeStates[_iEdge].m_bValid &&						//Is edge traversability known...
			m_lEdgeStates[_iEdge].m_bTraversable &&					//...and true...
			(m_pGraph->m_lEdges[_iEdge].m_Sizes & (1 << _Size)));	//...and applies to character of given size 
};


//Get current cost of edge
int CWorld_Navgraph_Pathfinder_AStar::EdgeCost(uint16 _iEdge)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_EdgeCost, 0);
	if (!m_lEdgeStates.ValidPos(_iEdge))
		return 4095;

	return m_lEdgeStates[_iEdge].m_Cost;
}; 


//Check if node has any edges traversable by agents of given size
bool CWorld_Navgraph_Pathfinder_AStar::CheckFromEdges(int16 _iNode, int _Size)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_CheckFromEdges, false);
	if (_Size == CXR_NavGraph::SIZE_INVALID)
		return false;

	for (int i = 0; i < m_pGraph->m_lNodes[_iNode].m_nEdges; i++)
	{
		int iEdge = m_pGraph->m_lNodes[_iNode].m_iiEdges + i;
		if (m_pGraph->m_lEdges.ValidPos(iEdge) &&
			TraversableEdge(iEdge, _Size))
		{
			//Found edge which is traversable by given size
			return true;
		}
	}
	
	//Couldn't find any traversable edges
	return false;
};


//Check if there are any edges traversable by agents of given size with node as destination
bool CWorld_Navgraph_Pathfinder_AStar::CheckToEdges(int16 _iNode, int _Size)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_CheckToEdges, false);
	if (_Size == CXR_NavGraph::SIZE_INVALID)
		return false;

	//Current navgraph implementation only allows us to check if there are any edges with node 
	//as destination, not which those edges are, unless we want to do it expensively.
	//Saving to-edge info in node class for this purpose only would be wasteful.
	int NodeFlags = m_pGraph->m_lNodes[_iNode].m_Flags;
	return (NodeFlags & CXR_NavGraph_Node::FLAGS_ISEDGEDESTINATION) != 0;	
};




//Set up pathfinder for usage
void CWorld_Navgraph_Pathfinder_AStar::Create(CXR_NavGraph * _pGraph, int _nSearchInstances, int _MaxNodes)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_Create, MAUTOSTRIP_VOID);
	m_pGraph = _pGraph;

	if (!m_pGraph)
		return;

	//Set up search instances
	m_lSearchInstances.SetLen(_nSearchInstances);
	int i;
	for (i = 0; i < m_lSearchInstances.Len(); i++)
	{
		m_lSearchInstances[i].Init(m_pGraph, this, _MaxNodes);
	}

	//Set up edge states
	m_lEdgeStates.SetLen(m_pGraph->m_lEdges.Len());
	for (i = 0; i < m_lEdgeStates.Len(); i++)
	{
		m_lEdgeStates[i].m_Cost = m_pGraph->m_lEdges[i].m_Cost;
		m_lEdgeStates[i].m_bValid = 1;
		m_lEdgeStates[i].m_bTraversable = 1;
	}
};


//Get approximate minimum distance between nodes. If correctly built any traversable
//position in the world should be within this distance from a navgraph node
int CWorld_Navgraph_Pathfinder_AStar::GetMinDistance()
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_GetMinDistance, 0);
	if (!m_pGraph)
		return 0;
	
	return m_pGraph->GetMinDistance(); 
};


//Get node grid position, given index
CVec3Dint16	CWorld_Navgraph_Pathfinder_AStar::GetNodePos(int16 _iNode)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_GetNodePos, CVec3Dint16(0,0,0));

	if (!m_pGraph || !m_pGraph->m_lNodes.ValidPos(_iNode))
		return CVec3Dint16(-1);
	return m_pGraph->m_lNodes[_iNode].m_GridPos;
};


//Fill the provided list with all node-indices within the minimum distance of the 
//given position, sorted with closest node first. Return the number of nodes
//Optionally we can cull nodes with no edges leading to or from them that can be 
//used by agents of given size group
int CWorld_Navgraph_Pathfinder_AStar::GetNodesAt(const CVec3Dint16& _Pos, TThinArray<int16>* _pRes, bool _bMustHaveFromEdges, bool _bMustHaveToEdges, int _SizeGroup)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_GetNodesAt, 0);
	//Get all nodes within min distance and save their respective distances to the position
	TThinArray<int16> Res;
	Res.SetLen(8);
	TThinArray<int> ResDistSqr;
	ResDistSqr.SetLen(8);
	int iFirstEmpty = 0;
	int MinDist = m_pGraph->GetMinDistance();
	int MinDistSqr = Sqr(MinDist);
	int iNode;
	int DistSqr;
	bool bFound;
	for (int x = -MinDist; x <= MinDist; x += MinDist)
		for (int y = -MinDist; y <= MinDist; y += MinDist)
			for (int z = -MinDist; z <= MinDist; z += MinDist)
			{
				iNode = m_pGraph->m_NodeHash.GetFirst(_Pos + CVec3Dint16(x, y, z));
				while (iNode != -1)
				{
					//Check if node is within min distance 
					DistSqr = _Pos.DistanceSqr(m_pGraph->m_lNodes[iNode].m_GridPos);
					if (DistSqr <= MinDistSqr)
					{
						//Check optional criteria
						if ((!_bMustHaveFromEdges || CheckFromEdges(iNode, _SizeGroup)) &&
							(!_bMustHaveToEdges || CheckToEdges(iNode, _SizeGroup)))
						{
							//Check that node hasn't already been found
							bFound = false;
							for (int i = 0; i < iFirstEmpty; i++)
							{
								if (Res[i] == iNode)
								{
									bFound = true;
									break;
								}
							}

							if (!bFound)
							{
								//Allocate more space if neccessary
								if (iFirstEmpty >= Res.Len())
								{
									Res.SetLen(Res.Len() * 2);
									ResDistSqr.SetLen(Res.Len());
								}

								//Add node
								Res[iFirstEmpty] = iNode;
								ResDistSqr[iFirstEmpty] = DistSqr;
								iFirstEmpty++;
							}
						}
					}
					iNode = m_pGraph->m_NodeHash.GetNext();
				}	
			}

	//Did we find any nodes?
	if (iFirstEmpty)
	{
		//Sort result according to distance
		QSort_r(&Res, &ResDistSqr, 0, iFirstEmpty - 1);

		//Copy result
		_pRes->SetLen(iFirstEmpty);
		for (int i = 0; i < iFirstEmpty; i++)
			(*_pRes)[i] = Res[i];
		return iFirstEmpty;
	}
	else
	{
		//No found nodes. Clear result and fail
		_pRes->Clear();
		return 0;
	}

	
};


//Create search, given node indices to the origin and destination and optionally alternate search 
//destinations and maximum cost (approx cell distance) of search.
//Returns index to search instance or -1 if no search instance is available
int CWorld_Navgraph_Pathfinder_AStar::Search_Create(int _iOrigin, int _iDestination, int _Size, int _iUser, int _iMaxCost, int _GameTick)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_Search_Create, 0);
	return Search_Create(_iOrigin, _iDestination, _Size, NULL, 0, _iUser, _iMaxCost, _GameTick);	
};
int CWorld_Navgraph_Pathfinder_AStar::Search_Create(int _iOrigin, int _iDestination, int _Size, const int16* _lAlternates, int _nAlternates, int _iUser, int _iMaxCost, int _GameTick)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_Search_Create_2, 0);
	if (!m_pGraph || !m_pGraph->m_lNodes.ValidPos(_iOrigin) || !m_pGraph->m_lNodes.ValidPos(_iDestination))	
		return -1;

	for (int i = 0; i < m_lSearchInstances.Len(); i++)
	{
		if (!m_lSearchInstances[i].IsAllocated(_GameTick))
		{
			//Available search instance found
			m_lSearchInstances[i].Allocate();
			m_lSearchInstances[i].Search_Create(_iOrigin, _iDestination, _Size, _lAlternates, _nAlternates, _iUser, _iMaxCost);
			return i;
		}
	}

	//No search instances available
	return -1;
};


//Perform previously created search, given instance index, max number of nodes and search slop
//to expand in one go. Reduces the the latter value by the number of nodes expanded
//and returns one of the values below. 
int CWorld_Navgraph_Pathfinder_AStar::Search_Execute(int _iSearchInstance, int& _nNodeExpansions, int * _pAlternateFound)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_Search_Execute, 0);
	if (!m_lSearchInstances.ValidPos(_iSearchInstance) ||
		!m_lSearchInstances[_iSearchInstance].IsAllocated())
		return SEARCH_UNAVAILABLE;

	return m_lSearchInstances[_iSearchInstance].Search_Execute(_nNodeExpansions, _pAlternateFound);
};


//Set the supplied path to the search result. If the partial flag is true, we do this even if 
//search isn't done. Succeeds if search is done, fails otherwise.
bool CWorld_Navgraph_Pathfinder_AStar::Search_Get(int _iSearchInstance, CWorld_NavGraph_Path * _pRes, bool _bPartial)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_Search_Get, false);
	if (!_pRes ||
		!m_lSearchInstances.ValidPos(_iSearchInstance) ||
		!m_lSearchInstances[_iSearchInstance].IsAllocated())
		return false;

	return m_lSearchInstances[_iSearchInstance].Search_Get(_pRes, -1, _bPartial);
};


//Set supplied path to the found path that ends in given node if such exists. Fails otherwise.
bool CWorld_Navgraph_Pathfinder_AStar::Search_Get(int _iSearchInstance, CWorld_NavGraph_Path * _pRes, int _iEndNode)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_Search_Get_2, false);
	if (!_pRes ||
		!m_lSearchInstances.ValidPos(_iSearchInstance) ||
		!m_lSearchInstances[_iSearchInstance].IsAllocated())
		return false;

	return m_lSearchInstances[_iSearchInstance].Search_Get(_pRes, _iEndNode);
};


//Release given search instance
void CWorld_Navgraph_Pathfinder_AStar::Search_Destroy(int _iSearchInstance, int _GameTick)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_Search_Destroy, MAUTOSTRIP_VOID);
	if (m_lSearchInstances.ValidPos(_iSearchInstance))
	{
		m_lSearchInstances[_iSearchInstance].Release(_GameTick);
		m_lSearchInstances[_iSearchInstance].Search_Reset();
	}
};


//Get user of give search instance
int CWorld_Navgraph_Pathfinder_AStar::Search_GetUser(int _iSearchInstance)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_Search_GetUser, 0);
	if (!m_lSearchInstances.ValidPos(_iSearchInstance))
		return 0;
	return m_lSearchInstances[_iSearchInstance].Search_GetUser();
};

//Get Number of search instances
int CWorld_Navgraph_Pathfinder_AStar::Search_GetNum()
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_Search_GetNum, 0);
	return m_lSearchInstances.Len();
};


//Invalidate edge (duh) between given nodes. This should be handled automatically by checking edges through dirty tiles actually.
void CWorld_Navgraph_Pathfinder_AStar::InvalidateEdge(int _iFrom, int _iTo, int _Sizes)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_InvalidateEdge, MAUTOSTRIP_VOID);
	if (!m_pGraph || 
		!m_pGraph->m_lNodes.ValidPos(_iFrom) || 
		!m_pGraph->m_lNodes.ValidPos(_iTo))
		return;

	//Only default size are allowed to invalidate edge currently
	if (!(_Sizes & CXR_NavGraph::SIZE_DEFAULT))
		return;

	for (int i = 0; i < m_pGraph->m_lNodes[_iFrom].m_nEdges; i++)
	{
		int iEdge = m_pGraph->m_lNodes[_iFrom].m_iiEdges + i;
		if (m_pGraph->m_lEdges.ValidPos(iEdge) && 
			(m_pGraph->m_lEdges[iEdge].m_iTo == _iTo))
		{
			m_lEdgeStates[iEdge].m_bValid = false;
			break;
		}
	}
};


//Set edge cost (given cost is grid cost) This should be handled automatically later.
void CWorld_Navgraph_Pathfinder_AStar::SetEdgeCost(int _iFrom, int _iTo, int _Cost, int _Sizes)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_SetEdgeCost, MAUTOSTRIP_VOID);
	if (!m_pGraph || 
		!m_pGraph->m_lNodes.ValidPos(_iFrom) || 
		!m_pGraph->m_lNodes.ValidPos(_iTo))
		return;

	//Only default size are allowed to set cost currently
	if (!(_Sizes & CXR_NavGraph::SIZE_DEFAULT))
		return;

	for (int i = 0; i < m_pGraph->m_lNodes[_iFrom].m_nEdges; i++)
	{
		int iEdge = m_pGraph->m_lNodes[_iFrom].m_iiEdges + i;
		if (m_pGraph->m_lEdges.ValidPos(iEdge) && 
			(m_pGraph->m_lEdges[iEdge].m_iTo == _iTo))
		{
//			int prevcost = m_lEdgeStates[iEdge].m_Cost;
			m_lEdgeStates[iEdge].m_Cost = Min(_Cost / CXR_NavGraph_Edge::COST_FACTOR, CXR_NavGraph_Edge::MAX_COST);
			break;
		}
	}
};


//Get best size group supported by graph, given base size square radius and height in grid cells
int CWorld_Navgraph_Pathfinder_AStar::GetBestSize(int _BaseSize, int _Height)
{
	//Get first size that is big enough for given size
	int Size = 0;
	int Sizes = m_pGraph->GetSupportedSizes();
	while (Sizes != 0)
	{
		//Check if size is supported by graph
		if (Sizes & 0x1)
		{
			//Supported size, check dimensions
			if ((CXR_NavGraph::GetBaseSize(Size) >= _BaseSize) &&
				(CXR_NavGraph::GetHeight(Size) >= _Height))
			{
				//Found fitting size group!
				return Size;
			}
		}
		//Try next size group
		Size++;
		Sizes >>= 1;
	}

	//Couldn't find suitable size group
	return CXR_NavGraph::SIZE_INVALID;
};


//Debug render search instance
void CWorld_Navgraph_Pathfinder_AStar::DebugRender(int _iSearch, CWorld_Server * _pServer)
{
	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_DebugRender, MAUTOSTRIP_VOID);
	if (m_lSearchInstances.ValidPos(_iSearch))
	{
		m_lSearchInstances[_iSearch].DebugRender(_pServer);
	}
};

//Debug helper to DebugRenderGraph (can be changed in watch)
static int sDefaultNavGraphSize = CXR_NavGraph::SIZE_DEFAULT;

//Debug render navgraph around a position
void CWorld_Navgraph_Pathfinder_AStar::DebugRenderGraph(CWorld_Server * _pServer, const CVec3Dint16& _GridPos, int _GridRadius, fp32 _Duration, int _Size)
{
	if (_Size == CXR_NavGraph::SIZE_INVALID)//DEBUG
		_Size = sDefaultNavGraphSize;		//DEBUG

	MAUTOSTRIP(CWorld_Navgraph_Pathfinder_AStar_DebugRenderGraph, MAUTOSTRIP_VOID);
	if (_pServer && _pServer->Path_GetBlockNav())
	{
		int i;
		for (i = 0; i < m_pGraph->m_lNodes.Len(); i++)
		{
			if ((Abs(m_pGraph->m_lNodes[i].m_GridPos[0] - _GridPos[0]) <= _GridRadius) &&
				(Abs(m_pGraph->m_lNodes[i].m_GridPos[1] - _GridPos[1]) <= _GridRadius))
			{
				CVec3Dfp32 Wpos = _pServer->Path_GetBlockNav()->GetWorldPosition(m_pGraph->m_lNodes[i].m_GridPos);
				_pServer->Debug_RenderWire(Wpos, Wpos + CVec3Dfp32(0,0,50), 0xffffff00, _Duration);

				for (int j = 0; j < m_pGraph->m_lNodes[i].m_nEdges; j++)
				{
					//If size is invalid, all edges are rendered and short vertical lines are rendered above 
					//each edge denoting size info. Otherwise only edges usable by given size group are rendered
					int iEdge = m_pGraph->m_lNodes[i].m_iiEdges + j;
					int Sizes = m_pGraph->m_lEdges[iEdge].m_Sizes;
				 	if (_Size == CXR_NavGraph::SIZE_INVALID)
					{
						//Render all edges
						CVec3Dfp32 WTo = _pServer->Path_GetBlockNav()->GetWorldPosition(m_pGraph->m_lNodes[m_pGraph->m_lEdges[iEdge].m_iTo].m_GridPos);
						_pServer->Debug_RenderWire(Wpos + CVec3Dfp32(0,0,50), WTo, 0xff00ff00, _Duration);
						
						//Render short vertical lines for additional size groups. Blue if size group can use edge, yellow otherwise.
						CVec3Dfp32 Dir = (WTo - (Wpos + CVec3Dfp32(0,0,50))) * 0.05f;
						CVec3Dfp32 Pos = Wpos + CVec3Dfp32(0,0,50) + Dir;
						for (int k = 0; k < CXR_NavGraph::NUM_SIZEGROUPS; k++)
						{
							Pos += Dir;
							_pServer->Debug_RenderWire(Pos, Pos + CVec3Dfp32(0,0,10), (Sizes & (1 << k)) ? 0xff0000ff : 0xffffff00, _Duration);
						}
					}
					else if (Sizes & (1 << _Size))
					{
						//Size can use edge, render!
						CVec3Dfp32 WTo = _pServer->Path_GetBlockNav()->GetWorldPosition(m_pGraph->m_lNodes[m_pGraph->m_lEdges[iEdge].m_iTo].m_GridPos);
						if (m_lEdgeStates[iEdge].m_bValid)
						{

							_pServer->Debug_RenderWire(Wpos + CVec3Dfp32(0.0f,0.0f,50.0f), WTo, 0xff00ff00, _Duration);
#ifndef M_RTM
							CVec3Dfp32 MidPos = (Wpos + CVec3Dfp32(0.0f,0.0f,50.0f)) * 0.9f + WTo * 0.1f;
							CStr Value = CStrF(" %d",m_lEdgeStates[iEdge].m_Cost);
							_pServer->Debug_RenderText(MidPos, Value, 0xff00ff00, _Duration);
#endif
						}
						else
						{
							_pServer->Debug_RenderWire(Wpos + CVec3Dfp32(0,0,50), WTo, 0xffff0000, _Duration);
						}
					}

#ifndef M_RTM
					CXR_NavGraph_Node Node = m_pGraph->m_lNodes[i];//DEBUG
					CXR_NavGraph_Edge Edge = m_pGraph->m_lEdges[iEdge];//DEBUG
#endif		
				}
			}
		}
	}
};





MRTC_IMPLEMENT_DYNAMIC(CWorld_NavGraph_Path, CReferenceCount);

CWorld_NavGraph_Path::CWorld_NavGraph_Path()
{
	MAUTOSTRIP(CWorld_NavGraph_Path_ctor, MAUTOSTRIP_VOID);
	m_pGraph = NULL;
};

//Clear path
void CWorld_NavGraph_Path::Reset()
{
	MAUTOSTRIP(CWorld_NavGraph_Path_Reset, MAUTOSTRIP_VOID);
	m_liPath.Clear();
};

//Get node index at given position in path, or -1 if there is no such index
int16 CWorld_NavGraph_Path::GetNode(int _iPos)
{
	MAUTOSTRIP(CWorld_NavGraph_Path_GetNode, 0);
	if (!m_liPath.ValidPos(_iPos))
		return -1;

	return m_liPath[_iPos];
};

//Get grid position of node at given position in path, or CVec3Dint16(-1) if there is no such position
CVec3Dint16 CWorld_NavGraph_Path::GetNodePosition(int _iPos)
{
	MAUTOSTRIP(CWorld_NavGraph_Path_GetNodePosition, CVec3Dint16(0,0,0));

	if (!m_pGraph || !m_liPath.ValidPos(_iPos) || !m_pGraph->m_lNodes.ValidPos(m_liPath[_iPos]))
		return CVec3Dint16(-1);

	return m_pGraph->m_lNodes[m_liPath[_iPos]].m_GridPos;
};

//Check if given position is within the given distance (in grid cells) from the given node position in path
bool CWorld_NavGraph_Path::IsAt(const CVec3Dint16& _Pos, int _iPos, fp32 _Radius)
{
	MAUTOSTRIP(CWorld_NavGraph_Path_IsAt, false);
	if (!m_pGraph || !m_liPath.ValidPos(_iPos) || !m_pGraph->m_lNodes.ValidPos(m_liPath[_iPos]))
		return false;

	//Just using CVec3Dint16 distancesqr will result in overflow quite often...
	CVec3Dint16 Diff = m_pGraph->m_lNodes[m_liPath[_iPos]].m_GridPos - _Pos;
	uint32 DistSqr = Sqr((int32)Diff[0]) + Sqr((int32)Diff[1]) + Sqr((int32)Diff[2]);
	return (DistSqr < Sqr(_Radius));
};


//Get length of path
int CWorld_NavGraph_Path::GetLength()
{
	MAUTOSTRIP(CWorld_NavGraph_Path_GetLength, 0);
	return m_liPath.Len();
};


//Build path, given node index list
void CWorld_NavGraph_Path::BuildPath(CXR_NavGraph * _pGraph, const int16* _lPath, int _nPathLen)
{
	MAUTOSTRIP(CWorld_NavGraph_Path_BuildPath, MAUTOSTRIP_VOID);
	Reset();
	m_pGraph = _pGraph;
	m_liPath.Insertx(0, _lPath, _nPathLen);
};


//Build path from other path data
void CWorld_NavGraph_Path::Copy(CWorld_NavGraph_Path * _pPath)
{
	MAUTOSTRIP(CWorld_NavGraph_Path_Copy, MAUTOSTRIP_VOID);
	Reset();

	if (!_pPath)
		return;

	_pPath->m_liPath.Duplicate(&m_liPath);
	m_pGraph = _pPath->m_pGraph;
};



