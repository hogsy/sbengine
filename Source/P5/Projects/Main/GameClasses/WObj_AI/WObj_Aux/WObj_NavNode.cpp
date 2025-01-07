#include "PCH.h"

#include "WObj_NavNode.h"
#include "../AICore.h"

#ifndef M_DISABLE_TODELETE

//Constructor
CWObject_NavNode::CWObject_NavNode()
	: CWObject()
{
};

//Get object "parameters" from keys
void CWObject_NavNode::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	if (_pKey->GetThisName().CompareSubStr("NAV_NEIGHBOUR") == 0)
	{
		//Add non-duplicates
		CStr Name = m_pWServer->World_MangleTargetName(_pKey->GetThisValue());
		for (int i = 0; i < m_lsNeighbours.Len(); i++)
		{
			//This target name is already in neighbours list
			if (m_lsNeighbours[i] == Name)
				return;
		}
		m_lsNeighbours.Add(Name);		
	}
	else
		CWObject::OnEvalKey(_pKey);
};


//Set up object 
void CWObject_NavNode::OnSpawnWorld()
{
	CWObject::OnSpawnWorld();

	//Get object indices for neighbours. List of neighbours names should not contain any duplicates
	for (int i = 0; i < m_lsNeighbours.Len(); i++)
	{
		int iSel;
		const int16* piObjects;
		int nObjects;

		//Set up selection
		try
		{
			TSelection<CSelection::LARGE_BUFFER> Selection;
		}
		catch (CCException) //Assume this is stack overflow... 
		{
			//Since we couldn't set up a selection we can't add any neighbours.
			return;
		};
		
		//Add all objects to selection, get selection result and free selection
		m_pWServer->Selection_AddTarget(Selection, m_lsNeighbours[i]);
		nObjects = m_pWServer->Selection_Get(Selection, &piObjects);
	
		//Add all found neighbours
		for (int i = 0; i < nObjects; i++)
		{
			m_liNeighbours.Add(*piObjects);

			//Get next object index
			piObjects++;
		};
	}
};


//Object message interface
int CWObject_NavNode::OnMessage(const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJMSG_GETNEIGHBOURS:
		{
			//Returns number of neighbours and sets provided data to pointer to neighbours array
			*(int *)_Msg.m_pData = (int)m_liNeighbours.GetBasePtr();
			return m_liNeighbours.Len();
		}
	default:
		return CWObject::OnMessage(_Msg);
	}
};


MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_NavNode, CWObject, 0x0100);






//CAI_NavNodePathfinder::CNavNode///////////////////////////
CAI_NavNodePathfinder::CNavNode::CNavNode(int _iObject)
{
	m_iObject = _iObject;
};

//CAI_NavNodePathfinder::CEdge///////////////////////////
CAI_NavNodePathfinder::CEdge::CEdge(int _iDest, int _iCost)
{
	m_iDest = _iDest;
	m_iCost = _iCost;
};



//CAI_NavNodePathfinder

//Constructor
CAI_NavNodePathfinder::CAI_NavNodePathfinder(CWorld_Server	* _pServer, int _iStartNode)
{
	m_pServer = _pServer;
	if (m_pServer)
	{
		BuildGraph(m_pServer->Object_Get(_iStartNode));
	}
};


//Calculates cost between two navnodes. Cost equals distance for base class..
int CAI_NavNodePathfinder::GetCost(CWObject * pFrom, CWObject * pTo)
{
	if (!pFrom || !pTo)
		return 0;

	return pFrom->GetPosition().Distance(pTo->GetPosition());
};


//Builds internal navgraph
void CAI_NavNodePathfinder::BuildGraph(CWObject * _pStartNode)
{
	if (!_pStartNode)
		return;

	//Create start node	and buil graph recursively from this
	m_lGraph.Clear();
	m_lGraph.Insert(0, CNavNode(_pStartNode->m_iObject));
	BuildGraph_r(_pStartNode, 0);

	//DEBUG
	for (int i = 0; i < m_lGraph.Len(); i++)
	{
		CNavNode * pNode = &(m_lGraph[i]);
		for (int j = 0; j < pNode->m_lEdges.Len(); j++)
		{
			CEdge * pEdge = &(pNode->m_lEdges[j]);
			CNavNode * pDest = &(m_lGraph[pEdge->m_iDest]);
		}
	}
	//DEBUG
};

//Depth-first recursive building of graph
void CAI_NavNodePathfinder::BuildGraph_r(CWObject * _pParent, int _iIndex)
{
	for (int k = 0; k < m_lGraph.Len(); k++)
	{
		CNavNode * pNode = &(m_lGraph[k]);
		for (int l = 0; l < pNode->m_lEdges.Len(); l++)
		{
			CEdge * pEdge = &(pNode->m_lEdges[l]);
			CNavNode * pDest = &(m_lGraph[pEdge->m_iDest]);
		}
	}

	//Get neighbours
	int nNeighBours;
	int * piNeighBours;
	CWObject_Message Msg(CWObject_NavNode::OBJMSG_GETNEIGHBOURS);
	Msg.m_pData = &piNeighBours;
	nNeighBours = _pParent->OnMessage(Msg);
	CWObject * pNeighBour;
	for (int i = 0; i < nNeighBours; i++)
	{
		//Get neighbour
		pNeighBour = m_pServer->Object_Get(piNeighBours[i]);
		if (pNeighBour)
		{
			//Check if neighbour is already in graph
			int iChild = GetGraphIndex(pNeighBour->m_iObject);
			if (m_lGraph.ValidPos(iChild))
			{
				//Neighbour is in graph, add edge
				m_lGraph[_iIndex].m_lEdges.Add(CEdge(iChild, GetCost(_pParent, pNeighBour)));

	CNavNode * pNode = &(m_lGraph[_iIndex]);
	CEdge * pEdge = &(m_lGraph[_iIndex].m_lEdges[i]);
	CNavNode * pDest = &(m_lGraph[pEdge->m_iDest]);
			}
			else
			{
				//Neighbour is not in graph, add new node and add edge to this
				int iNew = m_lGraph.Len();
				m_lGraph.Insert(iNew, CNavNode(pNeighBour->m_iObject));
				m_lGraph[_iIndex].m_lEdges.Add(CEdge(iNew, GetCost(_pParent, pNeighBour)));

	CNavNode * pNode = &(m_lGraph[_iIndex]);
	CEdge * pEdge = &(m_lGraph[_iIndex].m_lEdges[i]);
	CNavNode * pDest = &(m_lGraph[pEdge->m_iDest]);

				//Expand neighbour 
				BuildGraph_r(pNeighBour, iNew);
			}
		}
	CNavNode * pNode = &(m_lGraph[0]);
	CEdge * pEdge = &(m_lGraph[0].m_lEdges[0]);
	CNavNode * pDest = &(m_lGraph[pEdge->m_iDest]);
	}
};


//Get navgraph index of given world index object or -1 if object is not in graph
int CAI_NavNodePathfinder::GetGraphIndex(int _iNode)
{
	for (int i = 0; i < m_lGraph.Len(); i++)
	{
		if (m_lGraph[i].m_iObject == _iNode)
			return i;
	}
	return -1;
};


//Get pointer to graph node given world index of corresponding NavNode object
CAI_NavNodePathfinder::CNavNode * CAI_NavNodePathfinder::GetNode(int _iNode)
{
	for (int i = 0; i < m_lGraph.Len(); i++)
	{
		if (m_lGraph[i].m_iObject == _iNode)
			return &(m_lGraph[i]);
	}
	return NULL;
};


//Returns the number of children of the node corresponding to the given world index and sets 
//the given result pointer to the list of children
int CAI_NavNodePathfinder::GetChildren(int _iNode, int * _piChildren)
{
	CNavNode * pNode = GetNode(_iNode);
	if (!pNode)
		return 0;

	TArray<int> liChildren;
	liChildren.SetLen(pNode->m_lEdges.Len());
	for (int i = 0; i < pNode->m_lEdges.Len(); i++)
	{
		liChildren[i] = m_lGraph[pNode->m_lEdges[i].m_iDest].m_iObject;	
	}

	_piChildren = liChildren.GetBasePtr();
	return liChildren.Len();
};


//Get random child node world index of node corresponding to given object index
int CAI_NavNodePathfinder::GetRandomChild(int _iNode)
{
	CNavNode * pNode = GetNode(_iNode);
	if (!pNode)
		return 0;

	int rnd = Random * 0.999f * pNode->m_lEdges.Len();
	return m_lGraph[pNode->m_lEdges[rnd].m_iDest].m_iObject;
};


//Get the object index of the nav node closest to th given position
int CAI_NavNodePathfinder::GetClosestNode(CVec3Dfp32 _Pos)
{
	if (!m_pServer)
		return 0;

	fp32 MinDistSqr = _FP32_MAX;
	fp32 DistSqr;
	int iNode = 0;
	CWObject * pNode;
	for (int i = 0; i < m_lGraph.Len(); i++)
	{
		pNode = m_pServer->Object_Get(m_lGraph[i].m_iObject);
		if ( pNode &&
			 ((DistSqr = pNode->GetPosition().DistanceSqr(_Pos)) < MinDistSqr) )
		{
			//Closer node found
			iNode = m_lGraph[i].m_iObject;
			MinDistSqr = DistSqr;
		}
	}
	return iNode;
};




//Debug stuff
void CAI_NavNodePathfinder::DebugRenderEdge(CNavNode * _pStart, int _iEdge, CPixel32 _iClr, fp32 _Time)
{
	if (!m_pServer)
		return;

	CWObject * pStart = m_pServer->Object_Get(_pStart->m_iObject);
	CWObject * pDest = m_pServer->Object_Get(m_lGraph[_pStart->m_lEdges[_iEdge].m_iDest].m_iObject);
	if (pStart && pDest)
		m_pServer->Debug_RenderWire(pStart->GetPosition(), pDest->GetPosition(), _iClr, _Time);
};
void CAI_NavNodePathfinder::DebugRenderNodes(CPixel32 _iClr, fp32 _Time)
{
	if (!m_pServer)
		return;

	//Render nodes
	for (int i = 0; i < m_lGraph.Len(); i++)
	{
		CWObject * pNode = m_pServer->Object_Get(m_lGraph[i].m_iObject);
		if (pNode)
		{
			m_pServer->Debug_RenderVertex(pNode->GetPosition(), _iClr, _Time);	
		}
	}
};
void CAI_NavNodePathfinder::DebugRenderEdges(CPixel32 _iClr, fp32 _Time)
{
	if (!m_pServer)
		return;

	//Render egdes
	for (int i = 0; i < m_lGraph.Len(); i++)
	{
		CWObject * pFrom = m_pServer->Object_Get(m_lGraph[i].m_iObject);
		if (pFrom)
		{
			for (int j = 0; j < m_lGraph[i].m_lEdges.Len(); j++)
			{
				CWObject * pTo = m_pServer->Object_Get(m_lGraph[m_lGraph[i].m_lEdges[j].m_iDest].m_iObject);
				m_pServer->Debug_RenderWire(pFrom->GetPosition(), pTo->GetPosition(), _iClr, _Time);
			}
		}
	};
};
void CAI_NavNodePathfinder::DebugRenderGraph(CPixel32 _iClr, fp32 _Time)
{
	//Render graph
	DebugRenderNodes(_iClr, _Time);	
	DebugRenderEdges(_iClr, _Time);	
};




//CAI_NavNodePathfinder_AStar::CSearchNode///////////////////////////////////

//Constructor
CAI_NavNodePathfinder_AStar::CSearchNode::CSearchNode(CNavNode * _pNode, CSearchNode * _pParent, int _iHereCost, int _iHeuristicCost, int _iIndex)
{
	m_pNode = _pNode;
	m_pParent = _pParent;
	m_iHereCost = _iHereCost;
	m_iHeuristicCost = _iHeuristicCost;
	m_iIndex = _iIndex;
};


//Get cost 
int CAI_NavNodePathfinder_AStar::CSearchNode::GetPriority()
{
	return m_iHereCost + m_iHeuristicCost;
};


//CAI_NavNodePathfinder_AStar///////////////////////////////////

//Constructor
CAI_NavNodePathfinder_AStar::CAI_NavNodePathfinder_AStar(CWorld_Server	* _pServer, int _StartNode)
	: CAI_NavNodePathfinder(_pServer, _StartNode)
{
};


//Admissable heuristic cost of getting from given start node to given end node
int CAI_NavNodePathfinder_AStar::Heuristic(CNavNode * _pFrom, CNavNode * _pTo)
{
	//Get corresponding navnode objects
	CWObject * pFrom = m_pServer->Object_Get(_pFrom->m_iObject);
	if (!pFrom)
		return -1;
	CWObject * pTo = m_pServer->Object_Get(_pTo->m_iObject);
	if (!pTo)
		return -1;

	CVec3Dfp32 Diff = pFrom->GetPosition() - pTo->GetPosition();

/*
	// Backmans pythagoras
#define ASTAR_HEURISTIC_BLOAT	1.0f
	if ((Diff[0] >= Diff[1]) && (Diff[0] >= Diff[2]))
	{	// x largest
		return(ASTAR_HEURISTIC_BLOAT * (Diff[0] + 0.5 * (Diff[1] + Diff[2])));
	}
	else if ((Diff[1] > Diff[0]) && (Diff[1] > Diff[2]))
	{	// y largest
		return(ASTAR_HEURISTIC_BLOAT * (Diff[1] + 0.5 * (Diff[0] + Diff[2])));
	}
	else
	{	// z largest
		return(ASTAR_HEURISTIC_BLOAT * (Diff[2] + 0.5 * (Diff[0] + Diff[1])));
	}
*/

	// The old Heuristic below was ALWAYS returning too low an estimate, therefore the A* was
	// way much too slow. The above formula is largest component plus half of the other two
	// which is a cheap and fast version of pythagoras.
	return Max(M_Fabs(Diff[0]), Max(M_Fabs(Diff[1]), M_Fabs(Diff[2])));
};


//Use A* algorithm to find path.
bool CAI_NavNodePathfinder_AStar::FindPath(int _iStart, int _iDest, CAI_NavNodePath * _pPath)
{
	if (!m_pServer)
		return 0;

	//Assert that start and end nodes are in graph
	CNavNode * pStartNode = GetNode(_iStart);
	if (!pStartNode)
		return 0;
	CNavNode * pEndNode = GetNode(_iDest);
	if (!pEndNode)
		return 0;

	//Set up open and closed lists, and add start node to them
	m_lNodes.Clear();
	m_lNodes.SetLen(m_lGraph.Len());
	m_lNodes[0] = CSearchNode(pStartNode, NULL, 0, Heuristic(pStartNode, pEndNode), 0);
	m_lOpen.Create(false, m_lGraph.Len());
	m_lOpen.Push(&(m_lNodes[0]));
	int iLast = 0;

	CSearchNode * n[50];//DEBUG
	
	//Search!
	bool bSuccess = false;
	CSearchNode * pCurrent = NULL;
	CNavNode * pChild;
	bool bFound;
	while (!m_lOpen.IsEmpty())
	{
		//DEBUG
		for (int i = 0; i <= Min(50, iLast); i++)
		{
			n[i] = &(m_lNodes[i]);
		}
		//DEBUG

		//Get best node on open list
		pCurrent = m_lOpen.Pop();

		//Check if we've found path
		if (pCurrent->m_pNode == pEndNode)
		{
			//We found the end!
			bSuccess = true;
			break;
		}
		else
		{
			//Current node is not end. Expand it.
			for (int i = 0; i < pCurrent->m_pNode->m_lEdges.Len(); i++)
			{
				//Get child
				pChild = &(m_lGraph[pCurrent->m_pNode->m_lEdges[i].m_iDest]);
				
				//Check if child has already been visited (currently we don't reevaluate 
				//already expanded nodes, which isn't quite proper A*)
				bFound = false;
				for (int j = 0; j <= iLast; j++)
				{
					if (pChild == m_lNodes[j].m_pNode)
					{
						bFound = true;
						break;
					}
				}
				
				//If the child hasn't already been visited, then add it to open list
				if (!bFound)
				{
					iLast++;
					m_lNodes[iLast] = CSearchNode(pChild, pCurrent, pCurrent->m_iHereCost + pCurrent->m_pNode->m_lEdges[i].m_iCost, Heuristic(pChild, pEndNode), iLast);
					m_lOpen.Push(&(m_lNodes[iLast]));

					DebugRenderEdge(pCurrent->m_pNode, i, 0xff00ff00);
				}
			}
		}
	}

	//Did we find a path?
	if (bSuccess && pCurrent)
	{
		//Build result and return with success
		int iLength = 1;
		CSearchNode * pCur = pCurrent;
		while (pCur->m_pParent)
		{
			pCur = pCur->m_pParent;
			iLength++;
		}

		if (_pPath)
		{
			TArray<int> liRes;
			liRes.SetLen(iLength);
			pCur = pCurrent;
			for (int i = iLength - 1; i >= 0; i--)
			{
				liRes[i] = pCur->m_pNode->m_iObject;
				pCur = pCur->m_pParent;
			}
			_pPath->SetPath(liRes.GetBasePtr(), iLength);
		}

		return true;
	}
	else
	{
		//Couldn't find path
		return false;
	};
};	




//CAI_NavNodePath/////////////////////////////

CAI_NavNodePath::CAI_NavNodePath(CWorld_Server * _pServer)
{
	m_pServer = _pServer;
};


//Build path from given integer list and length
void CAI_NavNodePath::SetPath(const int * _piNodes, int _nLength)
{
	if (!_piNodes || (_nLength < 0))
		return;

	m_liNavNodes.Clear();
	m_liNavNodes.Insertx(0, _piNodes, _nLength);
};


//Add node to end of path
void CAI_NavNodePath::AddNode(int _iNode)
{
	m_liNavNodes.Add(_iNode);
}

//Get node object at position
CWObject * CAI_NavNodePath::GetNode(int _iPos)
{
	if (!m_liNavNodes.ValidPos(_iPos) || !m_pServer)
		return NULL;

	return m_pServer->Object_Get(m_liNavNodes[_iPos]);
};


//Get node index at position
int CAI_NavNodePath::GetNodeIndex(int _iPos)
{
	if (!m_liNavNodes.ValidPos(_iPos))
		return NULL;

	return m_liNavNodes[_iPos];
};


//Check if given position is within the given distance of the given node
bool CAI_NavNodePath::IsAt(CVec3Dfp32 _Pos, int _iPos, fp32 _Distance)
{
	CWObject * pNode = GetNode(_iPos);
	if (pNode)
		return (CAI_Core::XYDistanceSqr(pNode->GetPosition(), _Pos) < Sqr(_Distance));
		//return (pNode->GetPosition().DistanceSqr(_Pos) < Sqr(_Distance));
	else
		//Invalid node
		return false;
};


//Number of navnodes in path
int CAI_NavNodePath::Length()
{
	return m_liNavNodes.Len();
};


//Remove all nodes from path
void CAI_NavNodePath::Clear()
{
	m_liNavNodes.Clear();	
};
#endif
