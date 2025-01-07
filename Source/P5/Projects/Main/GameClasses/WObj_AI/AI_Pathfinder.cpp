#include "PCH.h"

#include "AI_Pathfinder.h"
#include "AICore.h"
#include "../../../../Shared/MOS/Classes/GameWorld/Server/WServer.h"

CAI_Pathfinder::CAI_Pathfinder()
{
	MAUTOSTRIP(CAI_Pathfinder_ctor, MAUTOSTRIP_VOID);
	m_pGraphPF = NULL;
	m_pGridPF = NULL;
	m_pAI = NULL;
	m_pPFGraph_ResourceHandler = NULL;	
	m_pPFGrid_ResourceHandler = NULL;	

	m_iGraphPFInstance = -1;
	m_iGridPFInstance = 0;

	m_BaseSize = 1;
	m_CrouchHeight = 1;
	m_FullHeight = 1;
	m_StepHeight = 0;
	m_JumpHeight = 0;
	m_SizeGroup = CXR_NavGraph::SIZE_DEFAULT;
	
	m_nExpandLimit = 256;	// *** Should be 64 ***
	// One millisecond
	m_MaxTime = CMTime::CreateFromSeconds(AI_PATHFIND_MICROSECONDS / 1000000.0f);
	//Get search result and graph path objects
	m_spGridPath = (CXBN_SearchResult *)MRTC_GetObjectManager()->CreateObject("CXBN_SearchResult");
	m_spGraphPath = (CWorld_NavGraph_Path *)MRTC_GetObjectManager()->CreateObject("CWorld_NavGraph_Path");

	Reset();
}


//Switch AI user
void CAI_Pathfinder::ReInit(CAI_Core * _pAI)
{
	MAUTOSTRIP(CAI_Pathfinder_ReInit, MAUTOSTRIP_VOID);
	m_pAI = _pAI;
};


//Set up pathfinder for searching
void CAI_Pathfinder::Init(CAI_Core * _pAI, CWorld_Navgraph_Pathfinder * _pGraphPathfinder, CXR_BlockNavSearcher * _pGridPathfinder, CAI_Resource_Pathfinding * _pPFGraphResourceHandler, CAI_Resource_Pathfinding * _pPFGridResourceHandler, int _BaseSize, int _CrouchHeight, int _FullHeight, int _StepHeight, int _JumpHeight, int _nExpandLimit, fp32 _MaxTime)
{
	MAUTOSTRIP(CAI_Pathfinder_Init, MAUTOSTRIP_VOID);
	m_pGraphPF = _pGraphPathfinder;
	m_pGridPF = _pGridPathfinder;
	m_pAI = _pAI;
	m_pPFGraph_ResourceHandler = _pPFGraphResourceHandler;	
	m_pPFGrid_ResourceHandler = _pPFGridResourceHandler;	
	
	m_BaseSize = _BaseSize;
	m_CrouchHeight = _CrouchHeight;
	m_FullHeight = _FullHeight;
	m_StepHeight = _StepHeight;
	m_JumpHeight = _JumpHeight;
	m_SizeGroup = ((m_pGraphPF && m_pGridPF) ? m_pGraphPF->GetBestSize(GetBlockNav()->GridSquareRadius(m_BaseSize), GetBlockNav()->GridHeightCeil(m_CrouchHeight)) : CXR_NavGraph::SIZE_INVALID);
	
	m_nExpandLimit = _nExpandLimit;

	m_MaxTime = CMTime::CreateFromSeconds(_MaxTime / 1000000.0f);

	Reset();
	m_GraphSkipPosition = CVec3Dfp32(_FP32_MAX);
};


//Reset pathfinder for new search
void CAI_Pathfinder::Reset(bool _bKeepPositions)
{
	MAUTOSTRIP(CAI_Pathfinder_Reset, MAUTOSTRIP_VOID);
	ReleaseSearch();

	m_bStraightPath = false;
	m_bPartialPath = false;
	m_Prio = CAI_Resource_Pathfinding::PRIO_MIN;

	if (!_bKeepPositions)
	{
		m_Origin = CVec3Dfp32(_FP32_MAX);
		m_Destination = CVec3Dfp32(_FP32_MAX);
	}
	
	if (m_spGraphPath)
		m_spGraphPath->Reset();
	else
		m_spGraphPath = (CWorld_NavGraph_Path *)MRTC_GetObjectManager()->CreateObject("CWorld_NavGraph_Path");
	if (m_spGridPath)
		m_spGridPath->Reset();
	else
		m_spGridPath = (CXBN_SearchResult *)MRTC_GetObjectManager()->CreateObject("CXBN_SearchResult");

	m_SearchStatus = NO_SEARCH;
	m_GridSearchStatus = CXR_BlockNavSearcher::SEARCH_INSTANCE_INVALID;
	m_GraphSearchStatus = CWorld_Navgraph_Pathfinder::SEARCH_INVALID;
	m_SearchMode = MODE_FIND_STARTNODECANDIDATES;

	m_iStartNode = -1;
	m_iEndNode = -1;
	m_liEndNodes.Clear();
	m_liStartNodes.Clear();

	m_iFirstUncheckedStartNode = 0;
	m_iFirstUncheckedEndNode = 0;
	m_iFirstUncheckedAlternate = 0;

	m_bStraightToStartNode = false;
	m_bStraightFromEndNode = false;

	m_liAlternates.Clear();
	m_liValidEndNodes.Clear();	
	m_iCurValidEndNode = 0;
	m_iCurCull = 0;

	// Changed height?
	// This had to be added to fight the 'million' calls to CAI_Core init
	if (m_pAI)
	{
		m_BaseSize = (int)m_pAI->GetBaseSize();
		m_CrouchHeight = (int)m_pAI->GetCrouchHeight();
		m_FullHeight = (int)m_pAI->GetHeight();
		m_StepHeight = (int)m_pAI->GetStepHeight();
		m_JumpHeight = (int)m_pAI->GetJumpHeight();
		if (m_pAI->m_DeviceStance.GetIdleStance() < CAI_Device_Stance::IDLESTANCE_COMBAT)
		{
			m_JumpHeight = m_StepHeight;	
		}
		m_SizeGroup = ((m_pGraphPF && m_pGridPF) ? m_pGraphPF->GetBestSize(GetBlockNav()->GridSquareRadius(m_BaseSize), GetBlockNav()->GridHeightCeil(m_CrouchHeight)) : CXR_NavGraph::SIZE_INVALID);
	}
};


//Set graph skip position when we want character to use grid pathfinding only 
//until he's moved away from the given position
void CAI_Pathfinder::SetGraphSkipPosition(const CVec3Dfp32& _Pos)
{
	m_GraphSkipPosition = _Pos;
}



//Can pathfinder be used?
bool CAI_Pathfinder::IsValid()
{
	MAUTOSTRIP(CAI_Pathfinder_IsValid, false);
	//We actually don't need a valid graph pathfinder, since we can get by on grid pathfinding only.
	return m_pAI && m_pGridPF; 
};
	


//Release search instances. If specified only grid or graph instance will be released.
void CAI_Pathfinder::ReleaseSearch(int _iMode)
{
	MAUTOSTRIP(CAI_Pathfinder_ReleaseSearch, MAUTOSTRIP_VOID);
	if (!m_pAI || !m_pAI->m_pGameObject)
		return;
	
	if (_iMode & RELEASE_GRAPH)
	{
		if (m_pGraphPF)
			m_pGraphPF->Search_Destroy(m_iGraphPFInstance, m_pAI->GetAITick());
		m_iGraphPFInstance = -1;
		if (m_pPFGraph_ResourceHandler)
			m_pPFGraph_ResourceHandler->Release(m_pAI->m_pGameObject->m_iObject, m_pAI->m_pServer);
	}

	if (_iMode & RELEASE_GRID)
	{
		if (m_pGridPF)
			m_pGridPF->Search_Destroy(m_iGridPFInstance, m_pAI->GetAITick());
		m_iGridPFInstance = 0;
		if (m_pPFGrid_ResourceHandler)
			m_pPFGrid_ResourceHandler->Release(m_pAI->m_pGameObject->m_iObject, m_pAI->m_pServer);
		m_GridOrigin = CVec3Dfp32(_FP32_MAX);
		m_GridDest = CVec3Dfp32(_FP32_MAX);
	}
};


//Request grid search instance and, if successful, create grid search instance with given parameters
//and return instance index. Return 0 on failure.
int CAI_Pathfinder::GetGridSearch(const CVec3Dfp32& _From, const CVec3Dfp32& _To, bool _bPartial, bool _bWallclimb)
{
	MAUTOSTRIP(CAI_Pathfinder_GetGridSearch, 0);
	if (!m_pAI || !m_pAI->m_pGameObject || !m_pPFGrid_ResourceHandler || !m_pGridPF)
		return 0;

	//Release any previously held search
	ReleaseSearch(RELEASE_GRID);

	//Request search instance
	if (m_pPFGrid_ResourceHandler->Request(m_pAI->m_pGameObject->m_iObject, m_Prio, m_pAI->m_pServer))
	{
		//Request granted. Create search instance
		CXBN_SearchParams Params(_From, 
								 _To, 
								 CVec3Dint16(m_BaseSize, m_BaseSize, m_CrouchHeight),
								 m_FullHeight);
		Params.m_bPartial = _bPartial;
		Params.m_bWallClimb = _bWallclimb;
		if (m_JumpHeight > m_StepHeight)
		{
			Params.m_bJump = true;
		}
		else
		{
			Params.m_bJump = false;
		}

		int iSearch = m_pGridPF->Search_Create(Params, m_pAI->GetAITick());
		if (iSearch == 0)
		{
			//Couldn't create search! Something's fishy. Clean grid searches and try again
			CleanGridPFResources();
			iSearch = m_pGridPF->Search_Create(Params, -1);
			if (iSearch == 0)
			{
				//Thwarted again! Nothing to do but release resource...
				ReleaseSearch(RELEASE_GRID);
			}
		}
		return iSearch;
	}
	else
	{
		//Request denied
		return 0;
	}
};


//Grid pathfinding wrapper. Returns blocknav searchstatus or one of the extra statuses below
int CAI_Pathfinder::GridFindPath(const CVec3Dfp32& _From, const CVec3Dfp32& _To, int& _nNodeExpansions, const CMTime& _Time, CXBN_SearchResult * _pRes, bool _bPartial, bool _bWallClimb, int _MaxExpansions)
{
	MAUTOSTRIP(CAI_Pathfinder_GridFindPath, 0);
	if ((_From == CVec3Dfp32(_FP32_MAX)) ||
		(_To == CVec3Dfp32(_FP32_MAX)) ||
		(_nNodeExpansions <= 0))
	{
		//Cannot perform search with these params
		ReleaseSearch(RELEASE_GRID);
		return INVALID;
	}

	//Is this a new search?
	if ( (m_iGridPFInstance == 0) ||
		 (_From != m_GridOrigin) ||
		 (_To != m_GridDest) )
	{
		//Try to set up search
		m_iGridPFInstance = GetGridSearch(_From, _To, _bPartial, _bWallClimb);
		if (m_iGridPFInstance != 0)
		{
			//New search started
			m_GridOrigin = _From;
			m_GridDest = _To;
			m_GridSearchStatus = CXR_BlockNavSearcher::SEARCH_IN_PROGRESS;
		}
	}
		 
	//Search if possible
	if (m_iGridPFInstance != 0)
	{
		//Expand no more than 8 nodes at a time, checking time in between
		int nNodeExpSave;
		while (_nNodeExpansions > 0)
		{
			//Search, but keep track of number of node expansions
			nNodeExpSave = 0;
			if (_nNodeExpansions > 8)
			{
				nNodeExpSave = _nNodeExpansions - 8;
				_nNodeExpansions = 8;
			}
			// nSlop is the radius of the unit measured in cells
			int nSlop = (int)(m_BaseSize / (2 * GetBlockNav()->UnitsPerCell()));
			if (nSlop < 2)
			{
				nSlop = 2;
			}
			m_GridSearchStatus = m_pGridPF->Search_Execute(m_iGridPFInstance, _nNodeExpansions, _MaxExpansions, nSlop);
			_nNodeExpansions += nNodeExpSave;

			//Break unless search is in progress
			if (m_pGridPF->Search_IsAsync() || m_GridSearchStatus != CXR_BlockNavSearcher::SEARCH_IN_PROGRESS)
				break;

			//Break if time has expired
			CMTime T = _Time;
			T.Stop();
			if (T.Compare(m_MaxTime) > 0)
			{
				break;
			}
		}
	}
	else
	{
		//Failed to set up search
		ReleaseSearch(RELEASE_GRID);
		m_GridSearchStatus = SEARCH_INSTANCE_UNAVAILABLE;
	}

	//Handle result
	switch (m_GridSearchStatus)
	{
	case CXR_BlockNavSearcher::SEARCH_DONE:
		{
			//Path found. 
			if (_pRes)
				m_pGridPF->Search_Get(m_iGridPFInstance, _pRes);
			ReleaseSearch(RELEASE_GRID);
			
			//_Time.Stop();
			//ConOutL(CStrF("Pathfinding done (GridFindPath, Mode %d). Searched for %f micsec", m_SearchMode, _Time * 1000000 / GetCPUFrequency()));//DEBUG
			return FOUND_PATH;
		}
	case SEARCH_INSTANCE_UNAVAILABLE:
	case SEARCH_TIMED_OUT:
		{
			//Waiting
			ReleaseSearch(RELEASE_GRID);
			return PAUSED;
		}
	case CXR_BlockNavSearcher::SEARCH_IN_PROGRESS:
		{
			//Searching
			//_Time.Stop();
			//ConOutL(CStrF("Pathfinding in progress (GridFindPath, Mode %d). Searched for %f micsec", m_SearchMode, _Time * 1000000 / GetCPUFrequency()));//DEBUG
			return IN_PROGRESS;
		}
	case CXR_BlockNavSearcher::SEARCH_NO_PATH:
	case CXR_BlockNavSearcher::SEARCH_NODE_ALLOC_FAIL:
		{
			//No path can be found right now (except perhaps partial)
			if (_bPartial && _pRes)
				m_pGridPF->Search_Get(m_iGridPFInstance, _pRes, true);
			ReleaseSearch(RELEASE_GRID);
			//_Time.Stop();
			//ConOutL(CStrF("Pathfinding failed (GridFindPath, Mode %d). Searched for %f micsec", m_SearchMode, _Time * 1000000 / GetCPUFrequency()));//DEBUG
			return NO_PATH;
		}

	default:
		{
			ReleaseSearch(RELEASE_GRID);
			//_Time.Stop();
			//ConOutL(CStrF("Pathfinding invalid (GridFindPath, Mode %d). Searched for %f micsec", m_SearchMode, _Time * 1000000 / GetCPUFrequency()));//DEBUG
			return INVALID;
		}
	}

	return 0;
};


//Request graph search instance and, if successful, create graph search instance with given parameters
//and return search instance index. Returns -1 on failure.
int CAI_Pathfinder::GetGraphSearch(int _iFrom, int _iTo, int _iSize, const int16* _lAlternates, int _nAlternates, int _MaxCost)
{
	MAUTOSTRIP(CAI_Pathfinder_GetGraphSearch, 0);
	if (!m_pAI || !m_pAI->m_pGameObject || !m_pPFGraph_ResourceHandler || !m_pGraphPF)
		return -1;

	//Release any previously held search
	ReleaseSearch(RELEASE_GRAPH);

	if (m_pPFGraph_ResourceHandler->Request(m_pAI->m_pGameObject->m_iObject, m_Prio, m_pAI->m_pServer))
	{
		//Request granted, create search instance
		int iSearch = m_pGraphPF->Search_Create(_iFrom, _iTo, _iSize, _lAlternates, _nAlternates, m_pAI->m_pGameObject->m_iObject, _MaxCost, m_pAI->GetAITick());
		if (iSearch == -1)
		{
			//Couldn't create search! Something's fishy. Clean graph searches and try again
			CleanGraphPFResources();
			iSearch = m_pGraphPF->Search_Create(_iFrom, _iTo, _iSize, _lAlternates, _nAlternates, m_pAI->m_pGameObject->m_iObject, _MaxCost, -1);
			if (iSearch == -1)
			{
				//Thwarted again! Nothing to do but release resource...
				ReleaseSearch(RELEASE_GRAPH);
			}
		}
		return iSearch;
	}
	else
	{
		//Request denied
		return -1;
	}
};


bool CAI_Pathfinder::IsOnGround(const CVec3Dfp32& _Pos)
{
	if ((GridPF())&&(GetBlockNav()->IsOnGround(GetBlockNav()->GetGridPosition(_Pos),m_BaseSize, m_pAI->m_bWallClimb)))
	{
		return(true);
	}

	return(false);
};

//Use grid pathfinding only
int CAI_Pathfinder::GridSearchOnly(int& _nNodeExpansions, const CMTime& _Time)
{
	MAUTOSTRIP(CAI_Pathfinder_GridSearchOnly, 0);
	m_SearchMode = MODE_GRIDSEARCH_ONLY;
	
	m_SearchStatus = GridFindPath(m_Origin, m_Destination, _nNodeExpansions, _Time, m_spGridPath, m_bPartialPath, m_pAI->m_bWallClimb);
	return m_SearchStatus;
};


//Find start node candidates and check for straight traversability
int CAI_Pathfinder::FindStartNodeCandidates(int& _nNodeExpansions, const CMTime& _Time)
{
	MAUTOSTRIP(CAI_Pathfinder_FindStartNodeCandidates, 0);
	m_SearchMode = MODE_FIND_STARTNODECANDIDATES;

	//Find all node-positions within minimum distance of origin. (The result is sorted)
	m_pGraphPF->GetNodesAt(GetBlockNav()->GetGridPosition(m_Origin), &m_liStartNodes, true, false, m_SizeGroup);
	m_iFirstUncheckedStartNode = 0;

	//Did we find any nodes?
	if (m_liStartNodes.Len())
	{
		//Jupp, check for straight paths to candidates
		return FindStraightPathStartNode(_nNodeExpansions, _Time);
	}
	else
	{
		//No nodes could be found, use grid pathfinding only
		return GridSearchOnly(_nNodeExpansions, _Time);
	}
}; 	


//Find first start node that can be reached by straight path from origin
int CAI_Pathfinder::FindStraightPathStartNode(int& _nNodeExpansions, const CMTime& _Time)
{
	MAUTOSTRIP(CAI_Pathfinder_FindStraightPathStartNode, 0);
	m_SearchMode = MODE_FIND_STRAIGHTPATH_STARTNODE;

	//Find the closest node, if any, that there's a straight path to
	for (int i = m_iFirstUncheckedStartNode; i < m_liStartNodes.Len(); i++)
	{
		if (GroundTraversable(m_Origin, GetBlockNav()->GetWorldPosition(m_pGraphPF->GetNodePos(m_liStartNodes[i]))))
		{
			//Found start node and remove it from start node candidate list
			m_iStartNode = m_liStartNodes[i];
			m_liStartNodes[i] = -1;
			m_iFirstUncheckedStartNode = i + 1;

			//Start node can be reached by straight path
			m_bStraightToStartNode = true;
			if (m_spGridPath)
				m_spGridPath->Reset();
			
			//If there is a current end node, we can go straight to graph search, 
			//otherwise we must find end nodes
			if (m_iEndNode != -1)
				return FindGraphPath(_nNodeExpansions, _Time);
			else
				//Find end node candidates
				return FindEndNodeCandidates(_nNodeExpansions, _Time);
		}

		//Check time
		CMTime T = _Time;
		T.Stop();
		if (T.Compare(m_MaxTime) > 0)
		{
			//Stop pathfinding
			//ConOutL(CStrF("Pathfinding timeout (FindStraightPathStartNode, %d searches). Searched for %f micsec", i - m_iFirstUncheckedStartNode + 1, T * 1000000 / GetCPUFrequency()));//DEBUG
			m_iFirstUncheckedStartNode = i + 1;
			m_SearchStatus = IN_PROGRESS;		
			return IN_PROGRESS;
		}
	}

	//Couldn't find straight path to any start node, try with proper pathfinding
	return FindStartNode(_nNodeExpansions, _Time);
}; 	


//Find the first start node candidate that can be reached from origin by grid pathfinding in one go
int CAI_Pathfinder::FindStartNode(int& _nNodeExpansions, const CMTime& _Time)
{
	MAUTOSTRIP(CAI_Pathfinder_FindStartNode, 0);
	//We haven't found a start node yet, keep searching among candidates to find the 
	//closest node which can be reached from the origin in a limited number of steps
	//Remove those that cannot be reached.
	m_SearchMode = MODE_FIND_STARTNODE;

	for (int i = 0; i < m_liStartNodes.Len(); i++)
	{
		if (m_liStartNodes[i] != -1)
		{
			//Check for path with no more than 4 * mindist expanded nodes 
			int PF = GridFindPath(m_Origin, GetBlockNav()->GetWorldPosition(m_pGraphPF->GetNodePos(m_liStartNodes[i])), _nNodeExpansions, _Time, m_spGridPath, false, m_pAI->m_bWallClimb, 4 * m_pGraphPF->GetMinDistance());
			if (PF == FOUND_PATH)
			{
				//Reachable startnode found!
				m_iStartNode = m_liStartNodes[i];
				m_liStartNodes[i] = -1;

				//Start node can only be reached by pathfinding
				m_bStraightToStartNode = false;
		
				//If there is a current end node, we can go straight to graph search, 
				//otherwise we must find end nodes
				if (m_iEndNode != -1)
					return FindGraphPath(_nNodeExpansions, _Time);
				else
					//Find end node candidates
					return FindEndNodeCandidates(_nNodeExpansions, _Time);
			}
			else if (PF == IN_PROGRESS)
			{
				//Didn't finish this frame
				m_SearchStatus = IN_PROGRESS;		
				return IN_PROGRESS;
			}
			else if (PF == PAUSED)
			{
				//Didn't finish this frame
				m_SearchStatus = IN_PROGRESS;		
				return PAUSED;
			}
			else
			{
				//Node not reached. Remove!
				m_liStartNodes[i] = -1;
			}

			//Should we break now?
			if (_nNodeExpansions <= 0)
			{
				//_Time.Stop();
				//ConOutL(CStrF("Pathfinding nodexp. break (FindStartNode). Searched for %f micsec", _Time * 1000000 / GetCPUFrequency()));//DEBUG
				m_SearchStatus = IN_PROGRESS;		
				return IN_PROGRESS;
			}

			//Check time
			CMTime T = _Time;
			T.Stop();
			if (T.Compare(m_MaxTime) > 0)
			{
				//Stop pathfinding
				//ConOutL(CStrF("Pathfinding timeout (FindStartNode). Searched for %f micsec", T * 1000000 / GetCPUFrequency()));//DEBUG
				m_SearchStatus = IN_PROGRESS;		
				return IN_PROGRESS;
			}
		}
	};

	//There were no valid start nodes. Use grid pathfinding only.
	return GridSearchOnly(_nNodeExpansions, _Time);
};


//Find end node candidates
int CAI_Pathfinder::FindEndNodeCandidates(int& _nNodeExpansions, const CMTime& _Time)
{
	MAUTOSTRIP(CAI_Pathfinder_FindEndNodeCandidates, 0);
	m_SearchMode = MODE_FIND_ENDNODECANDIDATES;

	//Find all node-positions within minimum distance of destination. (The result is sorted)
	m_pGraphPF->GetNodesAt(GetBlockNav()->GetGridPosition(m_Destination), &m_liEndNodes, false, true, m_SizeGroup);
	m_iFirstUncheckedEndNode = 0;

	//Did we find any nodes?
	if (m_liEndNodes.Len())
	{
		//Jupp, check for straight paths from candidates
		return FindStraightPathEndNode(_nNodeExpansions, _Time);
	}
	else
	{
		//No nodes could be found, use grid pathfinding only
		return GridSearchOnly(_nNodeExpansions, _Time);
	}
}; 	


//Find first end node from which destination can be reached by straight path
int CAI_Pathfinder::FindStraightPathEndNode(int& _nNodeExpansions, const CMTime& _Time)
{
	MAUTOSTRIP(CAI_Pathfinder_FindStraightPathEndNode, 0);
	m_SearchMode = MODE_FIND_STRAIGHTPATH_ENDNODE;

	//Find the closest node, if any, that there's a straight path from
	for (int i = m_iFirstUncheckedEndNode; i < m_liEndNodes.Len(); i++)
	{
		if (GroundTraversable(GetBlockNav()->GetWorldPosition(m_pGraphPF->GetNodePos(m_liEndNodes[i])), m_Destination))
		{
			//Found end node; remove it from end node candidate list
			m_iEndNode = m_liEndNodes[i];
			m_liValidEndNodes.Add(m_liEndNodes[i]);
			m_iFirstUncheckedEndNode = i + 1;
			m_liEndNodes[i] = -1;
			m_bStraightFromEndNode = true;
			
			//Find path
			return FindGraphPath(_nNodeExpansions, _Time);
		}

		//Check time
		CMTime T = _Time;
		T.Stop();
		if (T.Compare(m_MaxTime) > 0)
		{
			//Stop pathfinding
			//ConOutL(CStrF("Pathfinding timeout (FindStraightPathEndNode, %d searches). Searched for %f micsec", i - m_iFirstUncheckedEndNode + 1, T * 1000000 / GetCPUFrequency()));//DEBUG
			m_iFirstUncheckedEndNode = i + 1;
			m_SearchStatus = IN_PROGRESS;		
			return IN_PROGRESS;
		}
	}

	//Couldn't find straight path from any end node, try with proper pathfinding
	return FindEndNode(_nNodeExpansions, _Time);
}; 	


//Find the first end node candidate from which one can reach the destination by grid pathfinding in one go
int CAI_Pathfinder::FindEndNode(int& _nNodeExpansions, const CMTime& _Time)
{
	MAUTOSTRIP(CAI_Pathfinder_FindEndNode, 0);
	//We haven't found an end node yet, keep searching among candidates to find the 
	//closest node from which the destination can be reached in one search. 
	//Remove those that cannot be reached.
	m_SearchMode = MODE_FIND_ENDNODE;

	for (int i = 0; i < m_liEndNodes.Len(); i++)
	{
		if (m_liEndNodes[i] != -1)
		{
			//Check for path with no more than Sqr(mindist) / 2 expanded nodes 
			int PF = GridFindPath(GetBlockNav()->GetWorldPosition(m_pGraphPF->GetNodePos(m_liEndNodes[i])), m_Destination, _nNodeExpansions, _Time, NULL, false, m_pAI->m_bWallClimb, Sqr(m_pGraphPF->GetMinDistance()) / 2);
			if (PF == FOUND_PATH)
			{
				//Reachable endnode found!
				m_iEndNode = m_liEndNodes[i];
				m_liValidEndNodes.Add(m_liEndNodes[i]);
				m_liEndNodes[i] = -1;
				m_bStraightFromEndNode = false;

				//Find end node candidates
				return FindGraphPath(_nNodeExpansions, _Time);
			}
			else if (PF == IN_PROGRESS)
			{
				//Didn't finish this frame
				m_SearchStatus = IN_PROGRESS;		
				return IN_PROGRESS;
			}
			else if (PF == PAUSED)
			{
				//Didn't finish this frame
				m_SearchStatus = IN_PROGRESS;		
				return PAUSED;
			}
			else
			{
				//Node not reached. Remove!
				m_liEndNodes[i] = -1;
			}

			//Should we break now?
			if (_nNodeExpansions <= 0)
			{
				//_Time.Stop()
				//ConOutL(CStrF("Pathfinding nodexp. break (FindEndNode). Searched for %f micsec", _Time * 1000000 / GetCPUFrequency()));//DEBUG
				m_SearchStatus = IN_PROGRESS;		
				return IN_PROGRESS;
			}

			//Check time
			CMTime T = _Time;
			T.Stop();
			if (T.Compare(m_MaxTime) > 0)
			{
				//Stop pathfinding
				//ConOutL(CStrF("Pathfinding timeout (FindEndNode). Searched for %f micsec", T * 1000000 / GetCPUFrequency()));//DEBUG
				m_SearchStatus = IN_PROGRESS;		
				return IN_PROGRESS;
			}
		}
	};

	//Couldn't find any (new) valid end node. Clear candidate list.
	m_liEndNodes.Clear();

	//Check if we've previously found any valid end nodes
	if (m_liValidEndNodes.Len())
	{
		//We've already tried current start node with all valid end nodes.
		m_iCurValidEndNode = m_liValidEndNodes.Len();
		
		//Try new start node
		return TryNewNodes(_nNodeExpansions, _Time);
	}
	else
	{
		//There were no valid end nodes. Try to grid pathfinding only. However if we are interested 
		//in a partial path then path to first end node candidate is pretty good, so save this for later

		return GridSearchOnly(_nNodeExpansions, _Time);
	}
};


//Find path from current start node to current end node, or other end node candidates as backup
int CAI_Pathfinder::FindGraphPath(int& _nNodeExpansions, const CMTime& _Time)
{
	MAUTOSTRIP(CAI_Pathfinder_FindGraphPath, 0);
	//Try to find path from current start node to current end node. 
	//If search comes across current end node path has been found.
	//If search comes across one of the alternate end nodes, check if 
	//destination can be reached from this node. If so, path has been 
	//found, otherwise, remove that alternate end node.
	if (m_SearchMode != MODE_FIND_GRAPHPATH)
	{
		//Just switched to this mode
		m_SearchMode = MODE_FIND_GRAPHPATH;

		//Release previous search if not already released
		if (m_iGraphPFInstance != -1)
		{
			ReleaseSearch(RELEASE_GRAPH);
		}
	}

	//Start new search if we don't have valid search instance
	if (m_iGraphPFInstance == -1)
	{
		//Try to get search instance with the end node candidates as alternate destinations
		m_iGraphPFInstance = GetGraphSearch(m_iStartNode, m_iEndNode, m_SizeGroup, m_liEndNodes.GetBasePtr(), m_liEndNodes.Len());
		m_liAlternates.Clear();
		if (m_iGraphPFInstance == -1)
		{
			//Failed to get search instance. Try later.
			//_Time.Stop();
			//ConOutL(CStrF("Pathfinding no graph inst. (FindGraphPath). Searched for %f micsec", _Time * 1000000 / GetCPUFrequency()));//DEBUG
			m_SearchStatus = IN_PROGRESS;
			return PAUSED; 
		}
	}

	//Search until we get a decisive result or run out of node expansions
	int nPFs = 0; //DEBUG
	int iAlternateFound = -1;
	bool bFail = false;
	while (!bFail)
	{
		//Should we break now?
		if (_nNodeExpansions <= 0)
		{
			//_Time.Stop();
			//ConOutL(CStrF("Pathfinding node exp. break (FindGraphPath, %d PFs). Searched for %f micsec", nPFs, _Time * 1000000 / GetCPUFrequency()));//DEBUG
			m_SearchStatus = IN_PROGRESS;		
			return IN_PROGRESS;
		}

		//Check time
		CMTime T = _Time;
		T.Stop();
		if (T.Compare(m_MaxTime) > 0)
		{
			//Stop pathfinding
			//ConOutL(CStrF("Pathfinding timeout (FindGraphPath, %d PFs). Searched for %f micsec", nPFs, T * 1000000 / GetCPUFrequency()));//DEBUG
			m_SearchStatus = IN_PROGRESS;		
			return IN_PROGRESS;
		}

		//Get and check result
		m_GraphSearchStatus = m_pGraphPF->Search_Execute(m_iGraphPFInstance, _nNodeExpansions, &iAlternateFound);
		nPFs++;
		switch (m_GraphSearchStatus)
		{
		case CWorld_Navgraph_Pathfinder::SEARCH_FOUND_PATH:
			{
				//We're done! Yehaa! Get result and free search.
				m_SearchStatus = FOUND_PATH;
				m_pGraphPF->Search_Get(m_iGraphPFInstance, m_spGraphPath);
				ReleaseSearch();
				//_Time.Stop();
				//ConOutL(CStrF("Pathfinding done (FindGraphPath). Searched for %f micsec", _Time * 1000000 / GetCPUFrequency()));//DEBUG
				return FOUND_PATH;				
			};
		case CWorld_Navgraph_Pathfinder::SEARCH_FOUND_ALTERNATE:
			{
				//We've reached an alternate node, save this and continue
				m_liAlternates.Add(iAlternateFound);
				break;
			};
		case CWorld_Navgraph_Pathfinder::SEARCH_UNAVAILABLE:
			{
				//Search instance have become unavailable. Weird, but release search and stop for this turn.
				ReleaseSearch();
				m_SearchStatus = IN_PROGRESS;
				return PAUSED;
			};
		case CWorld_Navgraph_Pathfinder::SEARCH_IN_PROGRESS:
			{
				//Just keep going
				break;
			};
		case CWorld_Navgraph_Pathfinder::SEARCH_NO_PATH:
		case CWorld_Navgraph_Pathfinder::SEARCH_INVALID:
		default:
			{
				//Stop searching
				bFail = true;
			};
		}
	}

	//No path could be found with these start and end nodes. We must try other combinations.
	//First check if we've reached any alternates
	if (m_liAlternates.Len())
	{
		//If the current end node had straight traversability to the destination, 
		//there's a chance the alternates do as well
		m_iFirstUncheckedAlternate = 0;
		if (m_bStraightFromEndNode)
			return CheckAlternatesStraight(_nNodeExpansions, _Time);
		else
			return CheckAlternates(_nNodeExpansions, _Time);
	}
	else
	{
		//We can't find a path from current start node to end node, We must try with other end nodes 
		//and/or other start nodes. First get partial result, if so specified, and release search
		if (m_bPartialPath)
			m_pGraphPF->Search_Get(m_iGraphPFInstance, m_spGraphPath, true);
		ReleaseSearch();
		return TryNewNodes(_nNodeExpansions, _Time);
	}
}


//Check if destination can be reached from any alternate end node by straight traversability
int CAI_Pathfinder::CheckAlternatesStraight(int& _nNodeExpansions, const CMTime& _Time)
{
	MAUTOSTRIP(CAI_Pathfinder_CheckAlternatesStraight, 0);
	m_SearchMode = MODE_CHECK_ALTERNATES_STRAIGHT;

	//Find if there's any alternate with a straight path to destination
	for (int i = m_iFirstUncheckedAlternate; i < m_liAlternates.Len(); i++)
	{
		if (GroundTraversable(GetBlockNav()->GetWorldPosition(m_pGraphPF->GetNodePos(m_liAlternates[i])), m_Destination))
		{
			//Found valid alternate node, so we should have path. Get path to alternate
			if (m_pGraphPF->Search_Get(m_iGraphPFInstance, m_spGraphPath, m_liAlternates[i]))
			{
				//We successfully got a path from start node to alternate, so we're done!
				m_SearchStatus = FOUND_PATH;
				ReleaseSearch();
				//_Time.Stop();
				//ConOutL(CStrF("Pathfinding done (CheckAlternatesStraight). Searched for %f micsec", _Time * 1000000 / GetCPUFrequency()));//DEBUG
				return FOUND_PATH;				
			}
			else
			{
				//Something wasn't kosher...This alternate hasn't in fact been visited, so it's a false alternate
				m_liAlternates[i] = -1;
			}
		}

		//Check time
		CMTime T = _Time;
		T.Stop();
		if (T.Compare(m_MaxTime) > 0)
		{
			//Stop pathfinding
			//ConOutL(CStrF("Pathfinding timeout (CheckAlternatesStraight, %d searches). Searched for %f micsec", i - m_iFirstUncheckedAlternate + 1, T * 1000000 / GetCPUFrequency()));//DEBUG
			m_iFirstUncheckedAlternate = i + 1;
			m_SearchStatus = IN_PROGRESS;		
			return IN_PROGRESS;
		}
	}

	//Couldn't find straight path from any alternate node, try with proper pathfinding
	return CheckAlternates(_nNodeExpansions, _Time);
};


//Check if destination can be reached from any alternate end node
int CAI_Pathfinder::CheckAlternates(int& _nNodeExpansions, const CMTime& _Time)
{
	MAUTOSTRIP(CAI_Pathfinder_CheckAlternates, 0);
	//We've reached some alternate end nodes and must check if they're valid by pathfinding
	m_SearchMode = MODE_CHECK_ALTERNATES;

	int i = 0;
	while ( (i < m_liAlternates.Len()) &&
			(_nNodeExpansions > 0) )
	{
		if (m_liAlternates[i] != -1)
		{
			//Check time
			CMTime T = _Time;
			T.Stop();
			if (T.Compare(m_MaxTime) > 0)
			{
				//Stop pathfinding
				//ConOutL(CStrF("Pathfinding timeout (CheckAlternates). Searched for %f micsec", T * 1000000 / GetCPUFrequency()));//DEBUG
				m_SearchStatus = IN_PROGRESS;		
				return IN_PROGRESS;
			}

			//Check for path with no more than 4 * mindist expanded nodes 
			int PF = GridFindPath(GetBlockNav()->GetWorldPosition(m_pGraphPF->GetNodePos(m_liAlternates[i])), m_Destination, _nNodeExpansions, _Time, NULL, false, m_pAI->m_bWallClimb, 4 * m_pGraphPF->GetMinDistance());
			if (PF == FOUND_PATH)
			{
				//Found valid alternate node, so we should have path. Get path to alternate
				if (m_pGraphPF->Search_Get(m_iGraphPFInstance, m_spGraphPath, m_liAlternates[i]))
				{
					//We successfully got a path from start node to alternate, so we're done!
					m_SearchStatus = FOUND_PATH;
					ReleaseSearch();
					//_Time.Stop();
					//ConOutL(CStrF("Pathfinding done (CheckAlternates). Searched for %f micsec", _Time * 1000000 / GetCPUFrequency()));//DEBUG
					return FOUND_PATH;				
				}
				else
				{
					//Something wasn't kosher...This alternate hasn't in fact been visited, so it's a false alternate
					m_liAlternates[i] = -1;
				}
			}
			else if (PF == IN_PROGRESS)
			{
				//Didn't finish this frame 
				m_SearchStatus = IN_PROGRESS;		
				return IN_PROGRESS;
			}
			else if (PF == PAUSED)
			{
				//Can't search right now, waiting for resource
				m_SearchStatus = IN_PROGRESS;		
				return PAUSED;
			}
			else
			{
				//Node not reached. Remove from both alternates and end node candidates!
				for (int j = 0; j < m_liEndNodes.Len(); j++)
				{
					if (m_liAlternates[i] == m_liEndNodes[j])
					{
						m_liEndNodes[j] = -1;
						break;
					}
				}
				m_liAlternates[i] = -1;
			}
		}

		//Try next alternate
		i++;
	}

	//Failed to find valid alternate, get partial path if so specified and free search
	if (m_bPartialPath)
		m_pGraphPF->Search_Get(m_iGraphPFInstance, m_spGraphPath, true);
	ReleaseSearch();
	return TryNewNodes(_nNodeExpansions, _Time);		
};


//Try to find path with new end- and/or startnode
int CAI_Pathfinder::TryNewNodes(int& _nNodeExpansions, const CMTime& _Time)
{
	MAUTOSTRIP(CAI_Pathfinder_TryNewNodes, 0);
	m_SearchMode = MODE_TRY_NEW_NODES;

	//Cull end node candidates if there are any left
	if (m_liEndNodes.Len())
	{
		return CullEndNodes(_nNodeExpansions, _Time);
	}
	//Are there any valid endnodes?
	else if (m_liValidEndNodes.Len())
	{
		//Should we try next valid end node with current start node?
		if (m_iCurValidEndNode < m_liValidEndNodes.Len())
		{
			m_iEndNode = m_liValidEndNodes[m_iCurValidEndNode];
			m_iCurValidEndNode++;
			return FindGraphPath(_nNodeExpansions, _Time);				
		}
		else
		{
			//We've tried all valid end nodes with this start node, must get new start node
			m_iCurValidEndNode = 0;
			m_iEndNode = m_liValidEndNodes[0];
			
			//Cull start nodes 
			return CullStartNodes(_nNodeExpansions, _Time);
		}
	}
	else
	{
		//No valid end nodes. Try grid search
		return GridSearchOnly(_nNodeExpansions, _Time);
	}
};


//Remove all start nodes that can be swiftly reached from the current start node
int CAI_Pathfinder::CullStartNodes(int& _nNodeExpansions, const CMTime& _Time)
{
	MAUTOSTRIP(CAI_Pathfinder_CullStartNodes, 0);
	//Cull any nodes among the start candidates that can be swiftly reached from the 
	//current start node. Then try to find new start node.
	if (m_SearchMode != MODE_CULL_STARTNODES)
	{
		m_SearchMode = MODE_CULL_STARTNODES;

		//Make sure graph search is released when we enter culling mode
		ReleaseSearch(RELEASE_GRAPH);
	};
	
	//Cull until all nodes have been checked
//	bool bStopCull = false;
	while (m_iCurCull < m_liStartNodes.Len())
	{
		if (m_liStartNodes[m_iCurCull] != -1)
		{
			//Set up new search if this isn't done yet
			if (m_iGraphPFInstance == -1)
			{
				//Try to get search instance with the end node candidates as alternate destinations
				m_iGraphPFInstance = GetGraphSearch(m_iStartNode, m_liStartNodes[m_iCurCull], m_SizeGroup, m_liStartNodes.GetBasePtr(), m_liStartNodes.Len(), 4 * m_pGraphPF->GetMinDistance());
				if (m_iGraphPFInstance == -1)
				{
					//Failed to get search instance. Skip culling since it's only.a performance 
					//enhancing feature anyway, and we might be able to proceed
					break;
				}
			}

			//Search while there's node expansions left
			int iAlternateFound;
			bool bStop = false;
			while (!bStop)
			{
				//Should we break now?
				if (_nNodeExpansions <= 0)
				{
					m_SearchStatus = IN_PROGRESS;		
					//_Time.Stop();
					//ConOutL(CStrF("Pathfinding node exp. break (CullStartNodes). Searched for %f micsec", _Time * 1000000 / GetCPUFrequency()));//DEBUG
					return IN_PROGRESS;
				}
				
				//Check time
				CMTime T = _Time;
				T.Stop();
				if (T.Compare(m_MaxTime) > 0)
				{
					//Stop pathfinding
					//ConOutL(CStrF("Pathfinding timeout (CullStartNodes). Searched for %f micsec", T * 1000000 / GetCPUFrequency()));//DEBUG
					m_SearchStatus = IN_PROGRESS;		
					return IN_PROGRESS;
				}

				//Search
				int PF = m_pGraphPF->Search_Execute(m_iGraphPFInstance, _nNodeExpansions, &iAlternateFound);
				switch (PF)
				{
				case CWorld_Navgraph_Pathfinder::SEARCH_FOUND_PATH:
					{
						//Cull!
						m_liStartNodes[m_iCurCull] = -1;
						ReleaseSearch(RELEASE_GRAPH);
						bStop = true;
						break;
					};
				case CWorld_Navgraph_Pathfinder::SEARCH_FOUND_ALTERNATE:
					{
						//We've reached an alternate node, cull this and continue
						for (int i = 0; i < m_liStartNodes[i]; i++)
						{
							if (iAlternateFound == m_liStartNodes[i])
							{
								m_liStartNodes[i] = -1;
								break;
							}
						}
						break;
					};
				case CWorld_Navgraph_Pathfinder::SEARCH_IN_PROGRESS:
					{
						//Just keep going
						break;
					};
				case CWorld_Navgraph_Pathfinder::SEARCH_UNAVAILABLE:
				case CWorld_Navgraph_Pathfinder::SEARCH_NO_PATH:
				case CWorld_Navgraph_Pathfinder::SEARCH_INVALID:
				default:
					{
						//Stop searching
						bStop = true;
						break;
					};
				}
			}
		}

		//Check next node
		m_iCurCull++;
	}

	//Culling done. Find new valid start node. If there's straight path to current start node, there might be one to next.
	m_iCurCull = 0;
	if (m_bStraightToStartNode)
		return FindStraightPathStartNode(_nNodeExpansions, _Time);
	else
		return FindStartNode(_nNodeExpansions, _Time);
};


//Remove all end nodes that can be swiftly reached from the current end node
int CAI_Pathfinder::CullEndNodes(int& _nNodeExpansions, const CMTime& _Time)
{
	MAUTOSTRIP(CAI_Pathfinder_CullEndNodes, 0);
	//Cull any nodes among the end candidates that can be swiftly reached from the 
	//current end node. Then try to find new start node.
	m_SearchMode = MODE_CULL_ENDNODES;

	//Cull any nodes among the end candidates that can be swiftly reached from the 
	//current end node. Then try to find new end node.
	if (m_SearchMode != MODE_CULL_ENDNODES)
	{
		m_SearchMode = MODE_CULL_ENDNODES;

		//Make sure graph search is released when we enter culling mode
		ReleaseSearch(RELEASE_GRAPH);
	};
	
	//Cull until all nodes have been checked
//	bool bStopCull = false;
	while (m_iCurCull < m_liEndNodes.Len())
	{
		if (m_liEndNodes[m_iCurCull] != -1)
		{
			//Set up new search if this isn't done yet
			if (m_iGraphPFInstance == -1)
			{
				//Try to get search instance with the end node candidates as alternate destinations
				m_iGraphPFInstance = GetGraphSearch(m_iEndNode, m_liEndNodes[m_iCurCull], m_SizeGroup, m_liEndNodes.GetBasePtr(), m_liEndNodes.Len(), 4 * m_pGraphPF->GetMinDistance());
				if (m_iGraphPFInstance == -1)
				{
					//Failed to get search instance. Skip culling since it's only.a performance 
					//enhancing feature anyway, and we might be able to proceed
					break;
				}
			}

			//Search while there's node expansions left
			int iAlternateFound;
			bool bStop = false;
			while (!bStop)
			{
				//Should we break now?
				if (_nNodeExpansions <= 0)
				{
					m_SearchStatus = IN_PROGRESS;		
					//_Time.Stop();
					//ConOutL(CStrF("Pathfinding node exp. break (CullEndNodes). Searched for %f micsec", _Time * 1000000 / GetCPUFrequency()));//DEBUG
					return IN_PROGRESS;
				}
				
				//Check time
				CMTime T = _Time;
				T.Stop();
				if (T.Compare(m_MaxTime) > 0)
				{
					//Stop pathfinding
					//ConOutL(CStrF("Pathfinding timeout (CullEndNodes). Searched for %f micsec", T * 1000000 / GetCPUFrequency()));//DEBUG
					m_SearchStatus = IN_PROGRESS;		
					return IN_PROGRESS;
				}

				//Search
				int PF = m_pGraphPF->Search_Execute(m_iGraphPFInstance, _nNodeExpansions, &iAlternateFound);
				switch (PF)
				{
				case CWorld_Navgraph_Pathfinder::SEARCH_FOUND_PATH:
					{
						//Cull!
						m_liEndNodes[m_iCurCull] = -1;
						ReleaseSearch(RELEASE_GRAPH);
						bStop = true;
						break;
					};
				case CWorld_Navgraph_Pathfinder::SEARCH_FOUND_ALTERNATE:
					{
						//We've reached an alternate node, cull this and continue
						for (int i = 0; i < m_liEndNodes[i]; i++)
						{
							if (iAlternateFound == m_liEndNodes[i])
							{
								m_liEndNodes[i] = -1;
								break;
							}
						}
						break;
					};
				case CWorld_Navgraph_Pathfinder::SEARCH_IN_PROGRESS:
					{
						//Just keep going
						break;
					};
				case CWorld_Navgraph_Pathfinder::SEARCH_UNAVAILABLE:
				case CWorld_Navgraph_Pathfinder::SEARCH_NO_PATH:
				case CWorld_Navgraph_Pathfinder::SEARCH_INVALID:
				default:
					{
						//Stop searching
						bStop = true;
						break;
					};
				}
			}
		}

		//Check next node
		m_iCurCull++;
	}

	//Culling done. Find new valid end node. If there's straight path from current end node, there might be one to next.
	m_iCurCull = 0;
	if (m_bStraightFromEndNode)
		return FindStraightPathEndNode(_nNodeExpansions, _Time);
	else
		return FindEndNode(_nNodeExpansions, _Time);
};



//Find path from one position to another returns one the below results. Once a path has been 
//found it can be used via other methods below. The given priority is used when requesting search 
//instances. Returns pathfinding status:
int CAI_Pathfinder::FindPath(const CVec3Dfp32& _From, const CVec3Dfp32& _To, uint8 _Priority, bool _bPartial, bool _bWallClimb)
{
	MAUTOSTRIP(CAI_Pathfinder_FindPath, 0);
	//Check if this is new search
	if ( (m_SearchStatus != IN_PROGRESS) ||
		 (_From != m_Origin) ||
		 (_To != m_Destination) )
	{
		//Set up for new search
		Reset();
		m_Origin = _From;
		m_Destination = _To;

		//Reset graph skip position if origin is far enough away
		if (m_GraphSkipPosition != CVec3Dfp32(_FP32_MAX))
		{
			fp32 MinDistSqr = (m_pGraphPF && m_pGridPF) ? (m_pGraphPF->GetMinDistance() * GetBlockNav()->UnitsPerCell()) : 256.0f;
			if (m_GraphSkipPosition.DistanceSqr(_From) > MinDistSqr)
			{
				//Reset
				m_GraphSkipPosition = CVec3Dfp32(_FP32_MAX);
			}
		}
	}
	m_bPartialPath = _bPartial;
	m_Prio = _Priority;

	return FindPath();
}


//Find path from current origin to destination, with current priority. 
//Once a path has been found it can be used via other methods below.
int CAI_Pathfinder::FindPath()
{
	MAUTOSTRIP(CAI_Pathfinder_FindPath_2, 0);
	//Abort invalid searches
	if (!m_pAI || !m_pAI->m_pGameObject || !m_pGridPF || (m_SearchStatus == INVALID) || (m_pAI->m_UseFlags & CAI_Core::USE_NOPATHFINDING))
	{
		m_SearchStatus = INVALID;
		return INVALID;
	}

	//Number node-expansions left this frame. I count groundtraversable-checks as free (fix?) .
	int nNodeExpansions = m_nExpandLimit;

	//Time searched so far this frame. We should break when time exceeds m_MaxTime
	CMTime Time;
	Time.Start();

	//Is this new search?
	if (m_SearchStatus != IN_PROGRESS)
	{
		//Search is in progress
		m_SearchStatus = IN_PROGRESS;

		//If somewhat close check for straight traversability. Z-distance is weighed a bit heavier.
		int DistSqr = (int)(m_Origin.DistanceSqr(m_Destination) + Sqr(m_Origin[2] - m_Destination[2]));
		if (DistSqr < 768*768)
		{
			CVec3Dfp32 StopPos;
			if (GroundTraversable(m_Origin, m_Destination,&StopPos))
			{
				//Straight path found!
				m_bStraightPath = true;
				m_SearchStatus = FOUND_PATH;
				//Time.Stop(); Not used anyways
				return FOUND_PATH;
			}
			else if ((m_BaseSize > 24)&&
					(m_Origin.DistanceSqr(StopPos) >= 0.25f * m_Origin.DistanceSqr(m_Destination)))
			{
				//Straight path found!
				// m_Destination = StopPos;
				m_bStraightPath = true;
				m_SearchStatus = FOUND_PATH;
				//Time.Stop(); Not used anyways
				return FOUND_PATH;
			}
			else if (m_bPartialPath)
			{
				// *** TBD ***
				// If the straight path fails we should check if we got stopped close enough to
				// our destination to allow it.
				fp32 diff = m_Destination.Distance(StopPos);
				if (diff <= m_BaseSize)
				{
					//Straight path found!
					m_bStraightPath = true;
					m_SearchStatus = FOUND_PATH;
					//Time.Stop(); Not used anyways
					m_Destination = StopPos;
					return FOUND_PATH;
				}
			}
		}

		//If quite close or if there is no navgraph, or if bot size is unsupported by graph, 
		//or if we fulfil other graph skip criteria	use grid pathfinding only
		if (!m_pGraphPF || 
			(DistSqr < 256*256) || 
			(m_SizeGroup == CXR_NavGraph::SIZE_INVALID) ||
			(m_GraphSkipPosition != CVec3Dfp32(_FP32_MAX)))
		{
			//No straight path, use grid pathfinding
			return GridSearchOnly(nNodeExpansions, Time);
		}
		else
		{
			//Check time
			CMTime T = Time;
			T.Stop();
			if (T.Compare(m_MaxTime) > 0)
			{
				//Stop pathfinding
				//ConOutL(CStrF("Pathfinding timeout (FindStartNodeCandidates). Searched for %f micsec", T * 1000000 / GetCPUFrequency()));//DEBUG
				m_SearchMode = MODE_FIND_STARTNODECANDIDATES;
				m_SearchStatus = IN_PROGRESS;		
				return IN_PROGRESS;
			}

			//Start full navgraph-navgrid pathfinding
			return FindStartNodeCandidates(nNodeExpansions, Time);
		}
	}
	else
	{
		
#ifndef M_RTM
		m_pAI->Debug_RenderWire(m_Origin,m_Destination,kColorRed,0.0f);

#endif
		//Not new search, continue were we left off
		switch (m_SearchMode)
		{
			case MODE_GRIDSEARCH_ONLY:
				return GridSearchOnly(nNodeExpansions, Time);
			case MODE_FIND_STARTNODECANDIDATES: 
				return FindStartNodeCandidates(nNodeExpansions, Time); 
			case MODE_FIND_STRAIGHTPATH_STARTNODE: 
				return FindStraightPathStartNode(nNodeExpansions, Time); 
			case MODE_FIND_STARTNODE: 
				return FindStartNode(nNodeExpansions, Time); 
			case MODE_FIND_ENDNODECANDIDATES: 
				return FindEndNodeCandidates(nNodeExpansions, Time); 
			case MODE_FIND_STRAIGHTPATH_ENDNODE: 
				return FindStraightPathEndNode(nNodeExpansions, Time); 
			case MODE_FIND_ENDNODE: 
				return FindEndNode(nNodeExpansions, Time); 
			case MODE_FIND_GRAPHPATH: 
				return FindGraphPath(nNodeExpansions, Time); 
			case MODE_CHECK_ALTERNATES_STRAIGHT:
				return CheckAlternatesStraight(nNodeExpansions, Time);
			case MODE_CHECK_ALTERNATES:
				return CheckAlternates(nNodeExpansions, Time);
			case MODE_TRY_NEW_NODES:
				return TryNewNodes(nNodeExpansions, Time);
			case MODE_CULL_STARTNODES: 
				return CullStartNodes(nNodeExpansions, Time); 
			case MODE_CULL_ENDNODES: 
				return CullEndNodes(nNodeExpansions, Time); 
			default:
				return INVALID;
		}
	}
};


//Finds path as above, but always restarts pathfinding
int CAI_Pathfinder::FindNewPath(const CVec3Dfp32& _From, const CVec3Dfp32& _To, uint8 _Priority, bool _bPartial, bool _bWallClimb)
{
	MAUTOSTRIP(CAI_Pathfinder_FindNewPath, 0);
	m_SearchStatus = NO_SEARCH;
	return FindPath(_From, _To, _Priority, _bPartial, _bWallClimb);
};

CVec3Dfp32 CAI_Pathfinder::GetPathfindOrigin()
{
	if ((m_SearchStatus == IN_PROGRESS)||(m_SearchStatus == PAUSED)||(m_SearchStatus == FOUND_PATH))
	{
		return(m_Origin);
	}
	else
	{
		return(CVec3Dfp32(_FP32_MAX));
	}
}

//Set supplied path object to the found path. Succeeds if search is done, fails otherwise (but will set
//partial path if search was specified as such),
bool CAI_Pathfinder::GetPath(CAI_Path * _pPath)
{
	MAUTOSTRIP(CAI_Pathfinder_GetPath, false);
	if (!IsValid() || !_pPath || (m_pAI->m_UseFlags & CAI_Core::USE_NOPATHFINDING))
		return false;

	if (m_SearchStatus == FOUND_PATH)
	{
		//Get complete result
		if (m_bStraightPath)
		{
			//Straight path all the way to destination
			_pPath->Build(this, m_Destination);
		}
		else if (m_SearchMode == MODE_GRIDSEARCH_ONLY)
		{
			//Grid path only
			_pPath->Build(this, m_Destination, m_spGridPath);
		}
		else
		{
			//Combined path
			_pPath->Build(this, m_Destination, m_spGraphPath, m_spGridPath, m_bStraightToStartNode);
		}
		return true;
	}
	else if (m_bPartialPath)
	{
		//Try to get partial path
		if (m_SearchMode == MODE_GRIDSEARCH_ONLY)
		{
			//Grid pathfinding only, get result if not already gotten
			if (m_spGridPath->GetLength() == 0)
				m_pGridPF->Search_Get(m_iGridPFInstance, m_spGridPath, true);
			
			if (m_spGridPath->GetLength())
				_pPath->Build(this, m_spGridPath->GetNode(m_spGridPath->GetLength() - 1).GetPosition(), m_spGridPath);
			else
				_pPath->Reset();
		}
		else 
		{
			//Combined search; try to get graph path, or grid path as appropriate
			if (m_spGraphPath->GetLength() == 0)
			{
				//We haven't failed a graph search yet (or we coudn't find even a partial path with previous searches)
				//Get partial path of graph search if we're performing one now. Don't get partial if we're culling etc.
				if ((m_SearchMode == MODE_FIND_GRAPHPATH) &&
					(m_iGraphPFInstance != -1))
				{
					m_pGraphPF->Search_Get(m_iGraphPFInstance, m_spGraphPath, true);
				}

				//Have we got a graph path now?
				if (m_spGraphPath->GetLength() > 0)
				{
					//Use graph path
					CVec3Dfp32 Dest = GetBlockNav()->GetWorldPosition(m_spGraphPath->GetNodePosition(m_spGraphPath->GetLength() - 1));
					_pPath->Build(this, Dest, m_spGraphPath, m_spGridPath, m_bStraightToStartNode);
				}
				else
				{
					//We haven't got a valid graph path, use path to first node (if any)
					CVec3Dint16 GridDest = m_pGraphPF->GetNodePos(m_iStartNode);
					if (GridDest != CVec3Dint16(-1))
					{
						CVec3Dfp32 Dest = GetBlockNav()->GetWorldPosition(GridDest);
						if (m_bStraightToStartNode)
							_pPath->Build(this, Dest);
						else if (m_spGridPath->GetLength())
							_pPath->Build(this, Dest, m_spGridPath);
						else
							_pPath->Reset();
					}
					else
					{
						//Couldn't even get start node position
						_pPath->Reset();
					}
				}
			}
			else
			{
				//Use graph path
				CVec3Dfp32 Dest = GetBlockNav()->GetWorldPosition(m_spGraphPath->GetNodePosition(m_spGraphPath->GetLength() - 1));
				_pPath->Build(this, Dest, m_spGraphPath, m_spGridPath, m_bStraightToStartNode);
			}
		}

		return false;
	}
	else
	{
		//No path
		_pPath->Reset();
		return false;
	}
};	

//Check if we can make a simple ground move in a straight line from one position to another.
//If a partial position pointer is supplied, this will hold the farthest position along the 
//line to the destination that can be reached by a straight line ground move.
bool CAI_Pathfinder::GroundTraversable(const CVec3Dfp32& _From, const CVec3Dfp32& _To, CVec3Dfp32* _pPartial)
{
	MAUTOSTRIP(CAI_Pathfinder_GroundTraversable, false);
	if (m_pAI->m_UseFlags & CAI_Core::USE_NOPATHFINDING)
		return true;
	else
	{
		bool bClimb = m_pAI->m_bWallClimb;
		return m_pGridPF && GetBlockNav()->IsGroundTraversable(_From, _To, m_BaseSize, m_CrouchHeight, bClimb, m_StepHeight, m_JumpHeight, _pPartial);
	}
};

int CAI_Pathfinder::GetDir4(CVec3Dfp32 _Pos)
{
	MAUTOSTRIP(CAI_Pathfinder_GetDir4, 1);

	if (!m_pAI->m_pGameObject)
	{
		return(FRONT);
	}

	CVec3Dfp32 Dir = (_Pos - m_pAI->GetBasePos()).Normalize();
	CMat4Dfp32 Mat;
	m_pAI->GetBaseMat(Mat);
	CVec3Dfp32 Fwd = Mat.GetRow(0);
	CVec3Dfp32 Left = Mat.GetRow(1);
	if (Dir * Fwd >= _SIN45)
	{
		return(FRONT);
	}
	else if (Dir * Fwd <= -_SIN45)
	{
		return(BACK);
	}
	else if (Dir * Left >= _SIN45)
	{
		return(LEFT);
	}
	else
	{
		return(RIGHT);
	}
};

// Returns movestick for a particular movedir
CVec3Dfp32 CAI_Pathfinder::GetMove6(int32 _Dir)
{
	CVec3Dfp32 rMove = CVec3Dfp32(0.0f,0.0f,0.0f);
	switch(_Dir)
	{
	case FRONT:
		rMove[0] = 1.0f;
		rMove[1] = 0.0f;
		break;
	case RIGHT:
		rMove[0] = 0.0f;
		rMove[1] = -1.0f;
		break;
	case BACK:
		rMove[0] = -1.0f;
		rMove[1] = 0.0f;
		break;
	case LEFT:
		rMove[0] = -0.0f;
		rMove[1] = 1.0f;
		break;
	case RIGHT_45:
		rMove[0] = -_SIN45;
		rMove[1] = _SIN45;
		break;
	case LEFT_45:
		rMove[0] = _SIN45;
		rMove[1] = _SIN45;
		break;
	default:
		rMove[0] = 0.0f;
		rMove[1] = 0.0f;
	}
	return(rMove);
};

int CAI_Pathfinder::GetDir6(CVec3Dfp32 _Pos)
{
	MAUTOSTRIP(CAI_Pathfinder_GetDir6, 1);

	if (!m_pAI->m_pGameObject)
	{
		return(FRONT);
	}

	CVec3Dfp32 Dir = _Pos - m_pAI->GetBasePos();
	Dir[2] = 0.0f;
	Dir.Normalize();
	CMat4Dfp32 Mat;
	m_pAI->GetBaseMat(Mat);
	CVec3Dfp32 Fwd = Mat.GetRow(0);
	CVec3Dfp32 Left = Mat.GetRow(1);
	fp32 DotFwd = Dir * Fwd;

	if (DotFwd >= _SIN60)
	{
		return(FRONT);
	}
	else if (DotFwd <= -_SIN45)
	{
		return(BACK);
	}
	else if (DotFwd >= _SIN30)
	{
		if (Dir * Left >= 0.0f)
		{
			return(LEFT_45);
		}
		else
		{
			return(RIGHT_45);
		}
	}
	else
	{
		if (Dir * Left >= 0.0f)
		{
			return(LEFT);
		}
		else
		{
			return(RIGHT);
		}
	}
};

bool CAI_Pathfinder::GroundTraversableDirection(int _Dir, fp32* _pRange, CVec3Dfp32* _pPos)
{
	MAUTOSTRIP(CAI_Pathfinder_GroundTraversableDirection, false);
	if ((m_pAI->m_UseFlags & CAI_Core::USE_NOPATHFINDING)||(!m_pGridPF)||(!_pRange))
		return true;

	CVec3Dfp32 Pos = m_pAI->GetBasePos();
	CVec3Dfp32 Dir;
	CMat4Dfp32 Mat;
	m_pAI->GetHeadMat(Mat);
	switch(_Dir)
	{
	case FRONT:
		Dir = Mat.GetRow(0);
		break;

	case BACK:
		Dir = -Mat.GetRow(0);
		break;

	case RIGHT:
		Dir = -Mat.GetRow(1);
		break;

	case LEFT:
		Dir = Mat.GetRow(1);
		break;

	case LEFT_45:
		Dir = Mat.GetRow(1);
		Dir += Mat.GetRow(0);
		break;

	case RIGHT_45:
		Dir = -Mat.GetRow(1);
		Dir += Mat.GetRow(0);
		break;

	default:
		return(false);
		break;
	}
	CVec3Dfp32 Stop;
	Dir[2] = 0.0f;
	Dir.Normalize();
	// We add 8 to get some navgrid rounding margin (OK, maybe 8 is a bit too much but...)
	bool bResult = GroundTraversable(Pos,Pos+Dir*((*_pRange)+8.0f),&Stop);
#ifndef M_RTM
	if (m_pAI->DebugTarget())
	{
		if (bResult)
		{
			m_pAI->Debug_RenderVertex(Stop,kColorGreen,6.0f);
		}
		else
		{
			m_pAI->Debug_RenderVertex(Stop,kColorRed,6.0f);
		}
	}
#endif
	if (_pPos)
	{
		*_pPos = Stop;
	}
	if (!bResult)
	{
		*_pRange = (Stop - Pos).Length();
	}

	return(bResult);
};

//Check if we must crouch when moving from the given position in the given direction (assuming the position is traversible)
bool CAI_Pathfinder::MustCrouch(const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir)
{
	MAUTOSTRIP(CAI_Pathfinder_MustCrouch, false);
	//A bit incorrect. Might fix...

	//No need to crouch if we're as tall when standing
	if (m_FullHeight <= m_CrouchHeight)
		return false;

	if (!m_pGridPF || (m_pAI->m_UseFlags & CAI_Core::USE_NOPATHFINDING))
		return false;

	CVec3Dfp32 DirN = _Dir;
	DirN.Normalize();
	DirN *= GetBlockNav()->UnitsPerCell();

	//Convert position and direction into grid coordinates
	CVec3Dint16 Pos = GetBlockNav()->GetGridPosition(_Pos);
	CVec3Dint16 Dir = GetBlockNav()->GetGridPosition(_Pos + DirN) - Pos;

	if ((Dir[0] == 0) && (Dir[1] == 0))
	{
		//Moving up or down, don't need to crouch (but we might already be crouching)
		return false;
	}
	else
	{
		//Check traversability above position in direction.
		//When using _dx and _dy, we might actually check cells to the side, causing us to crouch unnecessarily
		//Thus lets try only using _dz and hoping any bugs this might cause will be handled by escape sequece moves
		//Perhaps we should make this a template choice, so that you can decide to use the safe check per character
		//return !m_pGridPF->IsTraversable(Pos + Dir + CVec3Dint16(0,0, m_pGridPF->GridHeightCeil(m_CrouchHeight + 1)), m_BaseSize, Height, -Dir[0], -Dir[1], -Dir[2]);
		//return !m_pGridPF->IsTraversable(Pos + Dir + CVec3Dint16(0,0, m_pGridPF->GridHeightCeil(m_CrouchHeight + 1)), m_BaseSize, Height, 0, 0, -Dir[2]);
		int Height = m_FullHeight - m_CrouchHeight;
		return !GetBlockNav()->IsTraversable(Pos + Dir + CVec3Dint16(0,0, GetBlockNav()->GridHeightCeil(m_CrouchHeight)), m_BaseSize, Height, 0, 0, -Dir[2]);
	};
};



//Helper "constant" array
int CAI_Pathfinder::ms_i2DDirs[8][2] = 
{   //Straights
	{1,0},{0,1},{0,-1},{-1,0},
	//Diagonals
	{1,1},{1,-1},{-1,1},{-1,-1}	
};


//Returns the closest position above or below the given position which is a traversable ground cell, 
//or fails with CVec3Dfp32(_FP32_MAX). If the optional _iMaxDiff argument is greater than -1, then this is 
//the maximum height-difference in cells tolerated. If the ordinary algorithm fails and the optional 
//_iRadius argument is greater than 0, it will continue to check _iRadius cell-columns in eight directions 
//outward from the given position.
CVec3Dfp32 CAI_Pathfinder::GetPathPosition(const CVec3Dfp32& _Pos, int _iMaxDiff, int _iRadius)
{
	MAUTOSTRIP(CAI_Pathfinder_GetPathPosition, CVec3Dfp32());

	if ((m_pAI->m_UseFlags & CAI_Core::USE_NOPATHFINDING) || (_Pos == CVec3Dfp32(_FP32_MAX)))
		return _Pos;

	if (!m_pGridPF) 
		//No navigation grid
		return CVec3Dfp32(_FP32_MAX);

	//Find the closest ground cell in the same xy-position
	CVec3Dint16 GridPos = GetBlockNav()->GetGridPosition(_Pos);

	//Pass 0
	if ( GetBlockNav()->IsOnGround(GridPos, m_BaseSize, m_pAI->m_bWallClimb) && 
		 GetBlockNav()->IsTraversable(GridPos, m_BaseSize, m_CrouchHeight, m_pAI->m_bWallClimb)
	   )
		return _Pos;
	else
	{
		//Pass 1-_iMaxDiff
		if (_iMaxDiff == -1) 
			_iMaxDiff = 20;
		uint16 MaxUp = _iMaxDiff;
		uint16 MaxDown = _iMaxDiff;
		CVec3Dint16 Check;
		int iCheckRadius;
		int iLevel;
		for (int i = 1; i <= _iMaxDiff; i++)
		{
			//Check straight down if possible
			if (i <= MaxDown)
			{
				Check = GridPos - CVec3Dint16(0,0,i);
				if (GetBlockNav()->IsOutsideGridBoundaries(Check))
				{
					//We can't go further downwards, since we're alredy below the grid floor
					MaxDown = i;
				}
				else if ( GetBlockNav()->IsOnGround(Check, m_BaseSize, m_pAI->m_bWallClimb) && 
						  GetBlockNav()->IsTraversable(Check, m_BaseSize, m_CrouchHeight, m_pAI->m_bWallClimb) )
				{
					//m_pAI->Debug_RenderWire(_Pos, m_pGridPF->GetWorldPosition(Check), 0xffff0000, 100);//DEBUG

					//Found traversable ground cell!
					return GetBlockNav()->GetWorldPosition(Check);
				}
				//m_pAI->Debug_RenderWire(_Pos, GetBlockNav()->GetWorldPosition(Check), 0xffffff00, 100);//DEBUG
			};

			//Check straight up if possible
			if (i <= MaxUp)
			{
				Check = GridPos + CVec3Dint16(0,0,i);
				if (GetBlockNav()->IsOutsideGridBoundaries(Check))
				{
					//We can't go further upwards, since we're alredy above the grid ceiling
					MaxUp = i;
				}
				else if ( GetBlockNav()->IsOnGround(Check, m_BaseSize, m_pAI->m_bWallClimb) && 
						  GetBlockNav()->IsTraversable(Check, m_BaseSize, m_CrouchHeight, m_pAI->m_bWallClimb) )
				{
					//m_pAI->Debug_RenderWire(_Pos, GetBlockNav()->GetWorldPosition(Check), 0xffff0000, 100);//DEBUG

					//Found traversable ground cell!
					return GetBlockNav()->GetWorldPosition(Check);
				}
				//m_pAI->Debug_RenderWire(_Pos, GetBlockNav()->GetWorldPosition(Check), 0xffffff00, 100);//DEBUG
			};

			//Check around the center column if appropriate 
			iCheckRadius = 1;
			while ( (iCheckRadius <= i) &
 				    (iCheckRadius <= _iRadius) )
			{
				//Set the height offset level we're currently checking at
				iLevel = i - iCheckRadius;

				//Should we check below? (or at same level if iLevel == 0)
				if (iLevel <= MaxDown)
				{
					for (int j = 0; j < 8; j++)
					{
						//Check cells around the center column and below the center 
						Check[0] = GridPos[0] + ms_i2DDirs[j][0] * iCheckRadius;
						Check[1] = GridPos[1] + ms_i2DDirs[j][1] * iCheckRadius;
						Check[2] = GridPos[2] - iLevel;
						if ( GetBlockNav()->IsOnGround(Check, m_BaseSize, m_pAI->m_bWallClimb) && 
							 GetBlockNav()->IsTraversable(Check, m_BaseSize, m_CrouchHeight, m_pAI->m_bWallClimb) )
						{
							//m_pAI->Debug_RenderWire(_Pos, GetBlockNav()->GetWorldPosition(Check), 0xffff0000, 100);//DEBUG

							//Found traversable ground cell!
							return GetBlockNav()->GetWorldPosition(Check);
						};
						
						//m_pAI->Debug_RenderWire(_Pos, GetBlockNav()->GetWorldPosition(Check), 0xffffff00, 100);//DEBUG
						
					};
				};

				//Should we check above? (Don't check if we're at level 0, since we've already checked those)
				if (iLevel && (iLevel <= MaxUp))
				{
					for (int j = 0; j < 8; j++)
					{
						//Check cells around the center column and below the center 
						Check[0] = GridPos[0] + ms_i2DDirs[j][0] * iCheckRadius;
						Check[1] = GridPos[1] + ms_i2DDirs[j][1] * iCheckRadius;
						Check[2] = GridPos[2] + iLevel;
						if ( GetBlockNav()->IsOnGround(Check, m_BaseSize, m_pAI->m_bWallClimb) && 
							 GetBlockNav()->IsTraversable(Check, m_BaseSize, m_CrouchHeight, m_pAI->m_bWallClimb) )
						{
							//m_pAI->Debug_RenderWire(_Pos, GetBlockNav()->GetWorldPosition(Check), 0xffff0000, 100);//DEBUG

							//Found traversable ground cell!
							return GetBlockNav()->GetWorldPosition(Check);
						};

						//m_pAI->Debug_RenderWire(_Pos, GetBlockNav()->GetWorldPosition(Check), 0xffffff00, 100);//DEBUG
	
					};
				};
				
				iCheckRadius++;
			};
			
			//Stop checking if we cannot check anything else
			if (i >= Max(MaxDown, MaxUp))
				break;
		};
		
		//Fail if we can't check anything else
		return CVec3Dfp32(_FP32_MAX);
	};
};


//Gets path position of the given object. This assumes the object is on the ground, unless the _bAirborne argument is set to true.
CVec3Dfp32 CAI_Pathfinder::GetPathPosition(CWObject* _pObj, bool _bAirborne)
{
	MAUTOSTRIP(CAI_Pathfinder_GetPathPosition_2, CVec3Dfp32());
	if ( !_pObj)
		//No object
		return CVec3Dfp32(_FP32_MAX);

	//Since the object obviously is somewhere in the world (we assume it has physics)
	//we must find a path position within a square radius of one cell-distance. If it's
	//airborne, then we might have to check the entire height of the grid though.
	if (_bAirborne)
		return GetPathPosition(m_pAI->GetBasePos(_pObj), 8, 1);
	else
		return GetPathPosition(m_pAI->GetBasePos(_pObj), 1, 1);
};


//Gets traversable ground position within the given radius and height, or returns the given position if this fails.
CVec3Dfp32 CAI_Pathfinder::SafeGetPathPosition(const CVec3Dfp32& _Pos, int _iMaxDiff, int _iRadius)
{
	MAUTOSTRIP(CAI_Pathfinder_SafeGetPathPosition, CVec3Dfp32());
	CVec3Dfp32 Res = GetPathPosition(_Pos, _iMaxDiff, _iRadius);
	if (INVALID_POS(Res))
		return _Pos;
	else
		return Res;
};



//Attempts to find a ground-traversable position (i.e. one that can be reached by travelling in a straight line 
//along the ground) from the object in the given heading at the given distance. If this fails, it will try to find 
//the closest position at that distance or closer, trying new heading with the given angle interval (90 degrees as default). 
//If the _angle argument is specified it will only try to find positions within the arc of that angle, centered 
//around the heading and if the _MinDist argument is given it will only try to find closer positions down to 
//this minimum distance. Returns CVec3Dfp32(_FP32_MAX) if no position is found. If the nGroundCells argument is specified 
//the position must also have at least that many cround cells.
CVec3Dfp32 CAI_Pathfinder::FindGroundPosition(CWObject* _pObject, fp32 _Heading, fp32 _Distance, fp32 _MinDist, fp32 _Interval, fp32 _Angle, int nGroundCells)
{
	MAUTOSTRIP(CAI_Pathfinder_FindGroundPosition, CVec3Dfp32());
	if (!_pObject)
		return CVec3Dfp32(_FP32_MAX);

	if (m_pAI->m_UseFlags & CAI_Core::USE_NOPATHFINDING)
		return (m_pAI->GetBasePos(_pObject) + (CVec3Dfp32(M_Cos(_Heading*2*_PI), M_Sin(-_Heading*2*_PI), 0) * _Distance));

	if (m_pGridPF)
	{
		//Ugly, but it works...
		fp32 dHeading = 0;
		fp32 Heading;
		CVec3Dfp32 Pos;
		CVec3Dfp32 Partial;
		//The direction we primarily search is randomly determined
		int8 iDir = (Random > 0.5f) ? 1 : -1;
		bool bIncr = true;
		while (dHeading <= _Angle)
		{
			Heading = _Heading + (iDir * dHeading);
			Pos = GetPathPosition(m_pAI->GetBasePos(_pObject) + (CVec3Dfp32(M_Cos(Heading*2*_PI) , M_Sin(-Heading*2*_PI), 0) * _Distance), 5, 0);

			if (GetBlockNav()->IsGroundTraversable(GetPathPosition(_pObject), Pos, m_BaseSize, m_CrouchHeight, false, m_StepHeight, m_JumpHeight, &Partial))
			{
				//Found position!
				//Debug_RenderWire(Pos, Pos + CVec3Dfp32(0,0,100), 0xffffffff, 10);//DEBUG
				return Pos;
			}
			else
			{
				//Was the ground traversable position closest to the wanted position far away enough?
				if ( !_MinDist &&
					 ((m_pAI->GetBasePos(_pObject)).DistanceSqr(Partial) >= Sqr(_MinDist)) )
				{
					//Found closer position
					//Debug_RenderWire(Partial, Partial + CVec3Dfp32(0,0,100), 0xffffffff, 10);//DEBUG
					return Partial;
				}
			};

			//If angle-delta is 0.5, then there is no point in checking the other direction, so we're done
			if (dHeading == 0.5f)
				break;

			//Did not find any position in this pass, set up next pass
			//Toggle direction
			iDir = -iDir;
			//Should we increase angle-delta? 
			if (bIncr)
				dHeading += _Interval;
			bIncr = !bIncr;
		};

		//No position can be found
		return CVec3Dfp32(_FP32_MAX);
	}
	else
		//We're effectively blind without a navgrid, so just use position _Distance units away in specified _Heading
		return GetPathPosition(m_pAI->GetBasePos(_pObject) + (CVec3Dfp32(M_Cos(_Heading*2*_PI), M_Sin(-_Heading*2*_PI), 0) * _Distance), 10, 0) ;
};


//Handles incoming messages
aint CAI_Pathfinder::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CAI_Pathfinder_OnMessage, 0);
	switch(_Msg.m_Msg)
	{
	case CAI_Core::OBJMSG_RELEASEPATH:
		{
			if ((_Msg.m_Param0 == CAI_Resource_Pathfinding::GRID) &&
				(m_iGridPFInstance != 0))
			{
				ReleaseSearch(RELEASE_GRID);
				return 1;
			}
			else if ((_Msg.m_Param0 == CAI_Resource_Pathfinding::GRAPH) &&
					 (m_iGraphPFInstance != -1))
			{
				ReleaseSearch(RELEASE_GRAPH);
				return 1;
			}
			else
				return 1;
		}
	default:
		return 0;
	}
};


int CAI_Pathfinder::GridSearch_Create(const CVec3Dfp32& _From, const CVec3Dfp32& _To)
{
	MAUTOSTRIP(CAI_Pathfinder_GridSearch_Create, 0);
	if (!m_pAI || !m_pAI->m_pGameObject || !m_pPFGrid_ResourceHandler || !m_pGridPF || (m_pAI->m_UseFlags & CAI_Core::USE_NOPATHFINDING))
		return 0;

	//Request search instance
	if (m_pPFGrid_ResourceHandler->Request(m_pAI->m_pGameObject->m_iObject, m_Prio, m_pAI->m_pServer))
	{
		//Request granted. Create search instance
		CXBN_SearchParams Params(_From, 
								 _To, 
								 CVec3Dint16(m_BaseSize, m_BaseSize, m_CrouchHeight),
								 m_FullHeight);
		Params.m_bWallClimb = m_pAI->m_bWallClimb;
		int iSearch = m_pGridPF->Search_Create(Params, m_pAI->GetAITick());
		if (iSearch == 0)
		{
			//Couldn't create search! Something's fishy. Clean grid searches and try again
			CleanGridPFResources();
			iSearch = m_pGridPF->Search_Create(Params, -1);
			if (iSearch == 0)
			{
				//Thwarted again! Nothing to do but release resource...
				ReleaseSearch(RELEASE_GRID);
			}
		}
		return iSearch;
	}
	else
	{
		//Reuest denied
		return 0;
	}
};

int CAI_Pathfinder::GridSearch_Execute(int _iSearch, int& _nNodeExpansions, int _nMaxExpansions)
{
	MAUTOSTRIP(CAI_Pathfinder_GridSearch_Execute, 0);
	if (!m_pGridPF)
		return CXR_BlockNavSearcher::SEARCH_NO_NAVGRID;

	return m_pGridPF->Search_Execute(_iSearch, _nNodeExpansions, _nMaxExpansions);
};

bool CAI_Pathfinder::GridSearch_Get(int _iSearch, CXBN_SearchResult* _pRes, bool _ReturnPartial)
{
	MAUTOSTRIP(CAI_Pathfinder_GridSearch_Get, false);
	if (!m_pGridPF)
		return false;

	return m_pGridPF->Search_Get(_iSearch, _pRes, _ReturnPartial);
};

void CAI_Pathfinder::GridSearch_Release(int _iSearch)
{
	MAUTOSTRIP(CAI_Pathfinder_GridSearch_Release, MAUTOSTRIP_VOID);
	if (m_pGridPF)
		m_pGridPF->Search_Destroy(_iSearch, m_pAI->GetAITick());
	if (m_pPFGrid_ResourceHandler)
		m_pPFGrid_ResourceHandler->Release(m_pAI->m_pGameObject->m_iObject, m_pAI->m_pServer, true);
	m_GridOrigin = CVec3Dfp32(_FP32_MAX);
	m_GridDest = CVec3Dfp32(_FP32_MAX);
};


//Destroy any unsanctioned grid searches
void CAI_Pathfinder::CleanGridPFResources()
{
	MAUTOSTRIP(CAI_Pathfinder_CleanGridPFResources, MAUTOSTRIP_VOID);
	if (!m_pPFGrid_ResourceHandler || !m_pGridPF)
		return;

	//Check all grid search instances for unsactioned users. 
	for (int i = 0; i < m_pGridPF->Search_GetNum(); i++)
	{
		//Searches begin at index 1
		if (!m_pPFGrid_ResourceHandler->SanctionedUser(m_pGridPF->Search_GetUser(i + 1)))
		{
			//Found unsanctioned use, destroy!
			m_pGridPF->Search_Destroy(i + 1, -1);
		}
	}
};

//Destroy any unsanctioned graph searches
void CAI_Pathfinder::CleanGraphPFResources()
{
	MAUTOSTRIP(CAI_Pathfinder_CleanGraphPFResources, MAUTOSTRIP_VOID);
	if (!m_pPFGraph_ResourceHandler || !m_pGraphPF)
		return;

	//Check all grid search instances for unsactioned users. 
	for (int i = 0; i < m_pGraphPF->Search_GetNum(); i++)
	{
		if (!m_pPFGraph_ResourceHandler->SanctionedUser(m_pGraphPF->Search_GetUser(i)))
		{
			//Found unsanctioned use, destroy!
			m_pGraphPF->Search_Destroy(i, -1);
		}
	}
};




//Some private accessors
CWorld_Navgraph_Pathfinder * CAI_Pathfinder::GraphPF()
{
	MAUTOSTRIP(CAI_Pathfinder_GraphPF, NULL);
	return m_pGraphPF;
};
CXR_BlockNavSearcher * CAI_Pathfinder::GridPF()
{
	MAUTOSTRIP(CAI_Pathfinder_GraphPF, NULL);
	return m_pGridPF;
};
int CAI_Pathfinder::ExpansionLimit()
{
	MAUTOSTRIP(CAI_Pathfinder_ExpansionLimit, 0);
	return m_nExpandLimit;
};
CMTime CAI_Pathfinder::TimeLimit()
{
	MAUTOSTRIP(CAI_Pathfinder_TimeLimit, 0.0f);
	return m_MaxTime;
};
int CAI_Pathfinder::GetGraphSizeGroup()
{
	MAUTOSTRIP(CAI_Pathfinder_GraphSizeGroup, CXR_NavGraph::SIZE_DEFAULT);
	return m_SizeGroup;
};



//Debug render (graph) search
void CAI_Pathfinder::Debug_Render(CWorld_Server* _pServer)
{
	MAUTOSTRIP(CAI_Pathfinder_Debug_Render, MAUTOSTRIP_VOID);
	if (!_pServer || !m_pAI || !m_pGraphPF || !m_pGridPF)
		return;

#ifndef M_RTM
	CVec3Dint16 Invalid = CVec3Dint16(-1);
	_pServer->Debug_RenderWire(m_Origin,m_Destination,kColorRed,0.0f);

	//Do we have a graph search in progress?
	if ((m_iGraphPFInstance != -1) &&
		(m_SearchStatus == IN_PROGRESS))
	{
		CVec3Dint16 Start = m_pGraphPF->GetNodePos(m_iStartNode);
		if (Start != Invalid)
			_pServer->Debug_RenderWire(GetBlockNav()->GetWorldPosition(Start), GetBlockNav()->GetWorldPosition(Start) + CVec3Dfp32(0,0,100), kColorGreen, 0.05f);

		CVec3Dint16 End = m_pGraphPF->GetNodePos(m_iEndNode);
		if (End != Invalid)
			_pServer->Debug_RenderWire(GetBlockNav()->GetWorldPosition(End), GetBlockNav()->GetWorldPosition(End) + CVec3Dfp32(0,0,100), kColorGreen, 0.05f);

		m_pGraphPF->DebugRender(m_iGraphPFInstance, _pServer);
	}
#endif
};




//CAI_Path///////////////////////////////////////////////////////////////////////////

//Constants
const fp32 CAI_Path::ms_ATSUBDESTDISTSQR = 16.0f*16.0f;
const fp32 CAI_Path::ms_ATSWITCHDESTDISTSQR = 8.0f*8.0f;
const fp32 CAI_Path::ms_ATSWITCHDESTHALFDIST = 4.0f;


//Constructor
CAI_Path::CAI_Path()
{
	MAUTOSTRIP(CAI_Path_ctor, MAUTOSTRIP_VOID);
	m_spGraphPath = (CWorld_NavGraph_Path *)MRTC_GetObjectManager()->CreateObject("CWorld_NavGraph_Path");
	m_spCurGridPath = (CXBN_SearchResult *)MRTC_GetObjectManager()->CreateObject("CXBN_SearchResult");
	m_spNextGridPath = (CXBN_SearchResult *)MRTC_GetObjectManager()->CreateObject("CXBN_SearchResult");
	m_pPathfinder = NULL;
	Reset();
};


//Clear path
void CAI_Path::Reset()
{
	MAUTOSTRIP(CAI_Path_Reset, MAUTOSTRIP_VOID);
	ReleaseGridSearch();
	m_iCall = 0;
	if (m_pPathfinder)
	{
		m_pPathfinder = NULL;
	}
	m_Destination = CVec3Dfp32(_FP32_MAX);
	m_bDirectPath = false;
	if (m_spGraphPath)
		m_spGraphPath->Reset();
	if (m_spCurGridPath)
		m_spCurGridPath->Reset();
	if (m_spNextGridPath)
		m_spNextGridPath->Reset();
	m_iCurGraphPos = -1;
	m_iNextGraphPos = -1;
	m_iGridPos = -1;
	m_SubDest = CVec3Dfp32(_FP32_MAX);
	m_NextSubDest = CVec3Dfp32(_FP32_MAX);
	m_bNextDirectPath = false;
	m_SearchStatus = NO_SEARCH;
	m_bSwitchingPath = false;
	m_SwitchDest = CVec3Dfp32(_FP32_MAX);
	m_PrevActionMode = NORMAL;
	m_PrevActionFlags = NONE;	
};

//Release search instances. If specified only grid or graph instance will be released.
void CAI_Path::ReleaseGridSearch()
{
	MAUTOSTRIP(CAI_Path_ReleaseGridSearch, MAUTOSTRIP_VOID);
	if (m_pPathfinder)
		m_pPathfinder->GridSearch_Release(m_iGridPFInstance);
	m_iGridPFInstance = 0;
};


//Squared distance between position, except that z-distance is reduced by a cell size
fp32 CAI_Path::GridNodeDistSqr(const CVec3Dfp32& _Pos1, const CVec3Dfp32& _Pos2)
{
	MAUTOSTRIP(CAI_Path_GridNodeDistSqr, 0.0f);
	CVec3Dfp32 Diff = _Pos1 - _Pos2;
	Diff[2] = Max(0.0f, M_Fabs(Diff[2]) - m_pPathfinder->GridPF()->GetBlockNav()->UnitsPerCell());
	return Diff.LengthSqr();
};


//Iterates through given grid path, starting at given index until a non-airborne node is 
//found or end is reached
int CAI_Path::GetLandingNode(CXBN_SearchResult * _pPath, int _iPos, int& _ActionFlags)
{
	MAUTOSTRIP(CAI_Path_GetLandingNode, 0);
	if ((_iPos < 0) || (_iPos >= _pPath->GetLength()))
		return _iPos;
	else
	{
		for (int i = _iPos; i < _pPath->GetLength(); i++)
		{
			if (_pPath->GetNode(i).GetFlags() & CXBN_SearchResultNode::CROUCH)
				_ActionFlags |= CROUCH;				

			if (!(_pPath->GetNode(i).GetFlags() & CXBN_SearchResultNode::AIRBORNE))
			{
				//Landing node found!
				return i;
			}
		}
		return _pPath->GetLength() - 1;
	}
};



//Build path from pathfinder data. If graph path is valid, then this is assumed to be a combined 
//path where the given grid path is path to first node and the direct path flag indicates whether 
//first node can be reached by a straight line ground move.
void CAI_Path::Build(CAI_Pathfinder * _pPF, const CVec3Dfp32& _Dest, CWorld_NavGraph_Path * _pGraphPath, CXBN_SearchResult * _pStartGridPath, bool _bStartDirect)
{
	MAUTOSTRIP(CAI_Path_Build, MAUTOSTRIP_VOID);
	Reset();

	//Create path objects if not previously created
	if (!m_spGraphPath)
		m_spGraphPath = (CWorld_NavGraph_Path *)MRTC_GetObjectManager()->CreateObject("CWorld_NavGraph_Path");
	if (!m_spCurGridPath)
		m_spCurGridPath = (CXBN_SearchResult *)MRTC_GetObjectManager()->CreateObject("CXBN_SearchResult");
	if (!m_spNextGridPath)
		m_spNextGridPath = (CXBN_SearchResult *)MRTC_GetObjectManager()->CreateObject("CXBN_SearchResult");

	m_pPathfinder = _pPF;
	m_Destination = _Dest;
	m_bDirectPath = _bStartDirect;
	if (m_spGraphPath)
		m_spGraphPath->Copy(_pGraphPath);
	
	//Only get grid path if we don't have direct path
	if (m_spCurGridPath)
	{
		if (_bStartDirect)	
			m_spCurGridPath->Reset();
		else
			m_spCurGridPath->Copy(_pStartGridPath);
	}

	if (m_spNextGridPath)
		m_spNextGridPath->Reset();

	m_iCall = 0;
#ifndef M_RTM
	if (CAI_Core::ms_bMovementVerbose)
	{
		//This is done to distinguish first step on a straight path so that we don't needlessly debug print
        if (m_bDirectPath && (m_spGraphPath->GetLength() == 0))
			m_SearchStatus = CXR_BlockNavSearcher::SEARCH_INSTANCE_INVALID;
	}
#endif
};

//Grid path only
void CAI_Path::Build(CAI_Pathfinder * _pPF, const CVec3Dfp32& _Dest, CXBN_SearchResult * _pGridPath)
{
	MAUTOSTRIP(CAI_Path_Build_2, MAUTOSTRIP_VOID);
	Build(_pPF, _Dest, NULL, _pGridPath);
};

//Direct path all the way
void CAI_Path::Build(CAI_Pathfinder * _pPF, const CVec3Dfp32& _Dest)
{
	MAUTOSTRIP(CAI_Path_Build_3, MAUTOSTRIP_VOID);
	Build(_pPF, _Dest, NULL, NULL, true);
};


//Check if path is valid to follow
bool CAI_Path::IsValid()
{
	MAUTOSTRIP(CAI_Path_IsValid, false);
	if (!m_pPathfinder || !m_spGraphPath || !m_spCurGridPath || !m_spNextGridPath)
		return false;

	return m_bDirectPath || 
		   m_spGraphPath->GetLength() ||
		   m_spCurGridPath->GetLength();
};

//Get current world position to go to, based on position we're at, 
//and set provided flags-integer to current action flags
CVec3Dfp32 CAI_Path::GetCurDestination(const CVec3Dfp32& _Pos, int& _ActionMode, int& _ActionFlags, bool& _bDone, fp32 _StepLength, fp32 _NodeRadius)
{
	MAUTOSTRIP(CAI_Path_GetCurDestination, CVec3Dfp32());
	// ***
	m_DebugReason = 0;
	// ***

	if(!m_pPathfinder)
	{
		ConOutL("(CAI_Path::GetCurDestination) Empty pathfinder");
		return CVec3Dfp32(_FP32_MAX);
	}

	enum
	{
		SMOOTH_SKIP_INTERMEDIATE	= 1,	// Skip CXBN_SearchResultNode::INTERMEDIATE nodes
		SMOOTH_CUT_ACROSS			= 2,	// Try to across to next gridpath
		SMOOTH_STRAIGHTEN_SEGMENT_1	= 4,	// Cut across segments when at node
		SMOOTH_STRAIGHTEN_START		= 8,	// Check start to random node 
		SMOOTH_STRAIGHTEN_SEGMENT_2	= 16,	// Cut across segments when not at node
		SMOOTH_STRAIGHTEN_END		= 32,	// Check cur to end node
		SMOOTH_STRAIGHTEN_FIRST_LEG	= 64,	// Check start to node 48+ units away on firstframe only
	};
	int smoothFlags = 0;
	// smoothFlags |= SMOOTH_SKIP_INTERMEDIATE;	// FIXME: Buggy!

	// Only one type of smoothing per call
	int modFive = m_iCall % 5;
	if (m_iCall == 0)
	{
		smoothFlags |= SMOOTH_STRAIGHTEN_FIRST_LEG;
	}
	else
	{
		switch(modFive)
		{
			case 0:
				smoothFlags |= SMOOTH_CUT_ACROSS;
			break;

			case 1:
				smoothFlags |= SMOOTH_STRAIGHTEN_SEGMENT_1;
			break;

			case 2:
				smoothFlags |= SMOOTH_STRAIGHTEN_START;
			break;

			case 3:
				smoothFlags |= SMOOTH_STRAIGHTEN_SEGMENT_2;
			break;

			case 4:
				smoothFlags |= SMOOTH_STRAIGHTEN_END;
			break;
		}
	}

	int N,iStart,iEnd;
	CVec3Dfp32 StartPos,EndPos;
	// ***
	// #undef Random
	// #define Random 0.45f
	// ***

	//Set "return params" to default values
 	_ActionMode = NORMAL;
 	_ActionFlags = NONE;
	_bDone = false;

	//DEBUG
	//m_pPathfinder->m_pAI->DebugMsg == 100;
	//m_DebugInfo = "GetCurDestination ";
	//uint64 T;
	//T_Start(T);
#ifndef M_RTM
	bool bDebugPrint = false;
	if (CAI_Core::ms_bMovementVerbose)
	{
		m_DebugInfo = CStrF("AIMV (GTime: %3.2f)", m_pPathfinder->m_pAI->m_pServer->GetGameTime().GetTime()) + m_pPathfinder->m_pAI->GetDebugName(m_pPathfinder->m_pAI->m_pGameObject);
	}
#endif
	//DEBUG

	//Create path objects if not present
	if (!m_spGraphPath)
		m_spGraphPath = (CWorld_NavGraph_Path *)MRTC_GetObjectManager()->CreateObject("CWorld_NavGraph_Path");
	if (!m_spCurGridPath)
		m_spCurGridPath = (CXBN_SearchResult *)MRTC_GetObjectManager()->CreateObject("CXBN_SearchResult");
	if (!m_spNextGridPath)
		m_spNextGridPath = (CXBN_SearchResult *)MRTC_GetObjectManager()->CreateObject("CXBN_SearchResult");

	if (!IsValid() || (_Pos == CVec3Dfp32(_FP32_MAX)))
	{
#ifndef M_RTM
		if (CAI_Core::ms_bMovementVerbose)
		{
			m_DebugInfo += CStr(" GetCurDestination invalid.");
			ConOutL(m_DebugInfo);
		}
#endif
		return CVec3Dfp32(_FP32_MAX);
	}

	//Are we at destination?
	fp32 DestDistSqr = GridNodeDistSqr(_Pos, m_Destination);
	if (DestDistSqr < Sqr(_StepLength/4))
	{
		_ActionMode = CAREFUL;
		_bDone = true;
		m_PrevActionMode = NORMAL;
		m_PrevActionFlags = NONE;
		return m_Destination;
	}
	//Are we almost at destination?
	else if (DestDistSqr < Sqr(_StepLength/2))
	{
		_ActionMode = STOP;
		_bDone = true;
		m_PrevActionMode = NORMAL;
		m_PrevActionFlags = NONE;
		return m_Destination;
	}
	//Straight move?
	else if (m_bDirectPath && (m_spGraphPath->GetLength() == 0))
	{
		//Check if we have to crouch
		if (m_pPathfinder->MustCrouch(_Pos, m_Destination - _Pos))
		{
			m_PrevActionFlags |= CROUCH;
			_ActionFlags |= CROUCH;
		}
		else
		{
			m_PrevActionFlags &= ~CROUCH;
			_ActionFlags = NONE;
		}
		m_PrevActionMode = NORMAL;
		return m_Destination;
	}
	else 
	{
		CVec3Dint16 GridPos = m_pPathfinder->GridPF()->GetBlockNav()->GetGridPosition(_Pos);

		//Get current grid path position if invalid
		if (m_iGridPos == -1)
		{
			//Assume first node, we check for overstep anyway...
			m_iGridPos = 0;
		}
		
		// Check straight to end direct path
		if (smoothFlags & SMOOTH_STRAIGHTEN_END)
		{	// We check if we can go straight towards the end
			StartPos = m_pPathfinder->SafeGetPathPosition(_Pos, 1, 1);
			EndPos = m_pPathfinder->SafeGetPathPosition(m_Destination, 1, 1);
			if ((StartPos.DistanceSqr(EndPos) <= Sqr(AI_PATHFIND_MAXSMOOTH))&&(m_pPathfinder->GroundTraversable(StartPos,EndPos)))
			{
				m_PrevActionMode = NORMAL;
				m_spGraphPath->Reset();
				m_bDirectPath = true;
				return m_Destination;
			}
		}

		//Is this a combined search?
		if (m_spGraphPath->GetLength() > 0)
		{
			if (m_iCurGraphPos == -1)
			{
				//Assume we start at first node...fix?
				m_iCurGraphPos = 0;
				m_iNextGraphPos = 0;
				m_SubDest = m_pPathfinder->GridPF()->GetBlockNav()->GetWorldPosition(m_spGraphPath->GetNodePosition(0));
			}

			//Check if we should start finding a path to next graph node. 
			if ((m_iNextGraphPos == m_iCurGraphPos) && 
				(m_spGraphPath->IsAt(GridPos, m_iNextGraphPos, _NodeRadius / m_pPathfinder->GridPF()->GetBlockNav()->UnitsPerCell())))
			{
				//Time to find path to next graph node, 
				m_iNextGraphPos++;
				
				//...reset stuff,..
				m_spNextGridPath->Reset();
				m_bNextDirectPath = false;
				
				//...get next node position (if we are at last node, next "node" is destination)...
				if (m_iNextGraphPos < m_spGraphPath->GetLength())
				{
					m_NextSubDest = m_pPathfinder->GridPF()->GetBlockNav()->GetWorldPosition(m_spGraphPath->GetNodePosition(m_iNextGraphPos));
				}
				else
				{
					m_NextSubDest = m_Destination;
				}

				//...and check how to get there.
				CVec3Dfp32 PathPos = m_pPathfinder->SafeGetPathPosition(_Pos, 1, 1);
				if (m_pPathfinder->GroundTraversable(PathPos, m_NextSubDest))
				{
					//Straight path from here to start of next graphnode!
					// ***
					m_DebugReason = DIRECT_PATH;
					// ***
					m_bDirectPath = true;
					m_SubDest = m_NextSubDest;
					m_bSwitchingPath = false;
					m_SwitchDest = CVec3Dfp32(_FP32_MAX);

					//This means we're starting to travel towards next node immediately
					m_iCurGraphPos = m_iNextGraphPos;
				}
				else if (m_pPathfinder->GroundTraversable(m_SubDest, m_NextSubDest))
				{
					//Direct path from current to next sub destination
					// ***
					m_DebugReason = DIRECTNEXT_PATH;
					// ***
					m_bNextDirectPath = true;
				}
				else 
				{
					//Make sure we start grid pathfinding from the graph node we're at to next node
					m_SearchStatus = CXR_BlockNavSearcher::SEARCH_INSTANCE_INVALID;
				}
			}

			//Should we try to start pathfinding?
			if (m_SearchStatus == CXR_BlockNavSearcher::SEARCH_INSTANCE_INVALID)
			{
				m_iGridPFInstance = m_pPathfinder->GridSearch_Create(m_SubDest, m_NextSubDest);
				if (m_iGridPFInstance == 0)
					m_SearchStatus = CXR_BlockNavSearcher::SEARCH_INSTANCE_INVALID;
				else
					m_SearchStatus = CXR_BlockNavSearcher::SEARCH_IN_PROGRESS;
			}
					
			//Should we perform pathfinding for next path?
			if ((m_iGridPFInstance != 0) &&
				(m_SearchStatus = CXR_BlockNavSearcher::SEARCH_IN_PROGRESS))
			{
				int nExpansions = m_pPathfinder->ExpansionLimit();
				CMTime MaxTime = m_pPathfinder->TimeLimit();
				CMTime T;
				T.Start();

				//Expand no more than 8 nodes at a time, checking time in between
				int nNodeExpSave;
				while (nExpansions > 0)
				{
					//Search, but keep track of number of node expansions
					nNodeExpSave = 0;
					if (nExpansions > 8)
					{
						nNodeExpSave = nExpansions - 8;
						nExpansions = 8;
					}
					m_SearchStatus = m_pPathfinder->GridSearch_Execute(m_iGridPFInstance, nExpansions);
					nExpansions += nNodeExpSave;

					//Break unless search is in progress
					if (m_SearchStatus != CXR_BlockNavSearcher::SEARCH_IN_PROGRESS)
						break;

					//Break if time has expired
					CMTime T2 = T;
					T2.Stop();
					if (T2.Compare(MaxTime) > 0)
						break;
				}
				
				switch (m_SearchStatus)
				{
				case CXR_BlockNavSearcher::SEARCH_DONE:
					{
						//Found next path, get it
						m_pPathfinder->GridSearch_Get(m_iGridPFInstance, m_spNextGridPath);
						ReleaseGridSearch();
						//Notify graph pathfinder of edge cost (note that m_iNextGraphPos or m_iCurGrapPos might be invalid positions, but this is handled correctly by graph path and pathfinder)
						if (m_spNextGridPath->GetCost() > 0)
							m_pPathfinder->GraphPF()->SetEdgeCost(m_spGraphPath->GetNode(m_iCurGraphPos), m_spGraphPath->GetNode(m_iNextGraphPos), m_spNextGridPath->GetCost(), (1 << m_pPathfinder->GetGraphSizeGroup()));						
						m_SearchStatus = NO_SEARCH;
					};
					break;
				case CXR_BlockNavSearcher::SEARCH_IN_PROGRESS:
					{
						//Still searching
					}
					break;
				case CXR_BlockNavSearcher::SEARCH_INSTANCE_INVALID:
				case CXR_BlockNavSearcher::SEARCH_INSTANCE_DESTROYED:
					{
						//Cannot search, try again next time
						m_SearchStatus = CXR_BlockNavSearcher::SEARCH_INSTANCE_INVALID;
					}
					break;
				case CXR_BlockNavSearcher::SEARCH_SOURCE_INTRAVERSABLE:
				case CXR_BlockNavSearcher::SEARCH_DESTINATION_INTRAVERSABLE:
					{	// Do NOT mark edge as invalid when source or dest intraversible			
						m_pPathfinder->Reset();
						CVec3Dfp32 SubDest = m_SubDest;
						Reset();
						_ActionMode = PAUSE;
						m_iCall++;
						return SubDest;
					}
					break;
				default:
					{
						//Search has failed, notify graph pathfinder that invalid edge has been found
						//Edge is from previous graph node to current (note that m_iNextGraphPos or m_iCurGraphpos might be invalid positions, but this is handled correctly by graph path and pathfinder)
#ifndef M_RTM
						if ((m_pPathfinder)&&(m_pPathfinder->m_pAI)&&(m_pPathfinder->m_pAI->DebugTarget()))
						{
							CStr Name = m_pPathfinder->m_pAI->m_pGameObject->GetName();
							ConOut(CStr("§f80 Navgraph edge invalidated: ")+Name);
						}
#endif
						m_pPathfinder->GraphPF()->InvalidateEdge(m_spGraphPath->GetNode(m_iCurGraphPos), m_spGraphPath->GetNode(m_iNextGraphPos), (1 << m_pPathfinder->GetGraphSizeGroup()));

						//m_pPathfinder->m_pAI->Debug_RenderWire(m_SubDest + CVec3Dfp32(0,0,100), m_NextSubDest, 0xffffff00, 1000);
						//m_pPathfinder->m_pAI->Debug_RenderWire(m_NextSubDest + CVec3Dfp32(0,0,200), m_NextSubDest, 0xffffff00, 1000);

						//We cannot use this path anymore, since it's based on false premises. 
						//If not default size, we must also use grid pathfinding until we get clear of the corrupt area
						if (m_pPathfinder->GetGraphSizeGroup() != CXR_NavGraph::SIZE_DEFAULT)
							m_pPathfinder->SetGraphSkipPosition(_Pos);
 						m_pPathfinder->Reset();
						CVec3Dfp32 SubDest = m_SubDest;
						Reset();
						_ActionMode = PAUSE;
						m_iCall++;
						return SubDest;
					}
				}
			}

			//Check if we can start switching path. Only switch path if we've already have path to next graph node.
			if (!m_bSwitchingPath &&
  	  			(m_bNextDirectPath || m_spNextGridPath->GetLength()))
			{ 
				//Always switch path when at end of grid path
				if (!m_bDirectPath &&
					(m_iGridPos >= m_spCurGridPath->GetLength() - 1))
				{
					m_bSwitchingPath = true;
					m_SwitchDest = _Pos;
				} 
				//Don't switch path when jumping or airborne, unless at end of path
				else if (!m_bDirectPath &&
						 ((m_spCurGridPath->GetNode(m_iGridPos).GetFlags() & CXBN_SearchResultNode::JUMP) ||
						  (m_spCurGridPath->GetNode(m_iGridPos).GetFlags() & CXBN_SearchResultNode::AIRBORNE)))
				{
					//Don't switch path
				}
				else if (m_bNextDirectPath)
				{
					//Direct path to next node, check if we can cut across from here to a point on 
					//the next path at same distance from current destination as we are now
					CVec3Dfp32 PathPos = m_pPathfinder->SafeGetPathPosition(_Pos, 1, 1);
					CVec3Dfp32 CutAcrossPos;
					fp32 DistSqr = m_SubDest.DistanceSqr(_Pos);
					if (DistSqr < m_SubDest.DistanceSqr(m_NextSubDest))
					{
						//We try to cut across to position on next path at same distance from subdest as our current position
						CutAcrossPos = m_pPathfinder->SafeGetPathPosition(m_SubDest + (m_NextSubDest - m_SubDest).Normalize() * M_Sqrt(DistSqr), 1, 1);				
					}
					else
					{
						//We try to cut across to next sub destination straight away
						CutAcrossPos = m_NextSubDest;
					}

					//Check if we can cut across
					if (m_pPathfinder->GroundTraversable(PathPos, CutAcrossPos))
					{
						//Start switching path!
						// ***
						m_DebugReason = SWITCHING_PATH;
						// ***

						m_bSwitchingPath = true;
						m_SwitchDest = CutAcrossPos;
					}
					//If we're close to subdest we should always "cut across" to a bit 
					//beyond subdest to avoid sliding needlessly againts orners
					else if (GridNodeDistSqr(_Pos, m_SubDest) < ms_ATSUBDESTDISTSQR)
					{
						//Veer out from corner by switching to position suitably beyond subdest so 
						//we'll be close or have passed subdest when done switching path.
						m_bSwitchingPath = true;
						m_SwitchDest = m_pPathfinder->SafeGetPathPosition(m_SubDest + (m_SubDest - _Pos).Normalize() * ms_ATSWITCHDESTHALFDIST, 1, 1);				
					}
				}
				else if (m_spNextGridPath->GetLength())
				{
					//Grid path to next node, check if we can go to node at about as far along the path
					//As we are distant to node now.
					CVec3Dfp32 PathPos = m_pPathfinder->SafeGetPathPosition(_Pos, 1, 1);
					int Steps = 0;
					// Steps = (int)(m_SubDest.Distance(_Pos) / m_pPathfinder->GridPF()->GetBlockNav()->UnitsPerCell() - 1);
					// Steps = Max(0, Min(Steps, m_spNextGridPath->GetSafeLength() - 2));
					// Insane assumtions above, use this instead?
					Steps = (int)(1 + m_spNextGridPath->GetLength() * (0.5f + Random));
					Steps = Max(0,Min(Steps,m_spNextGridPath->GetLength() - 2));
					if (m_pPathfinder->GroundTraversable(PathPos, m_spNextGridPath->GetNode(Steps).GetPosition()))
					{
						//Start switching path!
						// ***
						m_DebugReason = SWITCHINGSTEPS_PATH;
						// ***

						m_bSwitchingPath = true;
						m_SwitchDest = m_spNextGridPath->GetNode(Steps).GetPosition();
						// ***
						m_iGridPos = Steps;
						// ***
					}
					//If we're close to subdest we should always "cut across" to a bit 
					//beyond subdest to avoid sliding needlessly againts corners
					else if (GridNodeDistSqr(_Pos, m_SubDest) < ms_ATSUBDESTDISTSQR)
					{
						//Veer out from corner by switching to position suitably beyond subdest so 
						//we'll be close or have passed subdest when done switching path.
						// ***
						m_DebugReason = SWITCHINGACROSS_PATH;
						// ***

						m_bSwitchingPath = true;
						m_SwitchDest = m_pPathfinder->SafeGetPathPosition(m_SubDest + (m_SubDest - _Pos).Normalize() * ms_ATSWITCHDESTHALFDIST, 1, 1);				
					}
				}
				//If neither of the above cases were true, then we have no path to switch to

				//If we did switch path, then we're on our way to the next node
				if (m_bSwitchingPath)
				{
					//We're now following the next path!
					if (m_spNextGridPath->GetLength())
					{
						//Assume that were at first node (although we probably ain't), since we will check for overstep anyway)
						m_iGridPos = 0;
						m_spCurGridPath->Copy(m_spNextGridPath);
						m_iCall = 0;
						smoothFlags = SMOOTH_STRAIGHTEN_FIRST_LEG;
					}
					m_spNextGridPath->Reset();
					m_SubDest = m_NextSubDest;
					INVALIDATE_POS(m_NextSubDest);
					m_bDirectPath = m_bNextDirectPath;
					m_bNextDirectPath = false;
					m_iCurGraphPos = m_iNextGraphPos;
				}
			}

			//Check if we're done switching path
			if (m_bSwitchingPath && 
				(GridNodeDistSqr(_Pos, m_SwitchDest) < ms_ATSWITCHDESTDISTSQR))
			{
				m_bSwitchingPath = false;
				INVALIDATE_POS(m_SwitchDest);
			}

			//Check if we're in the process of switching path 
			if (m_bSwitchingPath)
			{
				//Direct move. Check if we have to crouch
				if (m_pPathfinder->MustCrouch(_Pos, m_SwitchDest - _Pos))
				{
					m_PrevActionFlags |= CROUCH;
					_ActionFlags |= CROUCH;
				}
				else
				{
					m_PrevActionFlags &= ~CROUCH;
					_ActionFlags = NONE;
				}

				m_PrevActionMode = NORMAL;
#ifndef M_RTM
				if (CAI_Core::ms_bMovementVerbose && bDebugPrint)
				{
					if (_ActionFlags & CROUCH)
						ConOutL(m_DebugInfo +  CStr(" Crouching."));
					else
						ConOutL(m_DebugInfo +  CStr(" Moving to switchdest."));
				}
#endif
				if (smoothFlags & SMOOTH_CUT_ACROSS)
				{
					// Here we try random, straight line cut across moves
					if (m_spNextGridPath->GetLength() > 0)
					{	// Try to optimize the path a bit
						// See if we can move in a straight path halfway betw cur and last node
						int iNextPos = (int)(m_spNextGridPath->GetLength() * Random * 0.999f);
						// OK; can we go from LastPos to NextPos?
						CVec3Dfp32 NextPos;
						if ((iNextPos > m_iGridPos)&&(iNextPos < m_spNextGridPath->GetLength())&&(m_spNextGridPath->GetNodePos(iNextPos,&NextPos)))
						{
							// Don't call expensive GroundTraversable unless fairly close (AI_PATHFIND_MAXSMOOTH units)
							if ((_Pos.DistanceSqr(NextPos) <= Sqr(AI_PATHFIND_MAXSMOOTH))&&(m_pPathfinder->GroundTraversable(_Pos,NextPos)))
							{
								m_iGridPos = iNextPos;
								m_SwitchDest = NextPos;
							}
						}
					}
				}
				m_iCall++;
				return m_SwitchDest;
			}
			
			//Are we at sub destination?
			fp32 SubDestDistSqr = GridNodeDistSqr(_Pos, m_SubDest);
			if (SubDestDistSqr < ms_ATSUBDESTDISTSQR)
			{
				//Jupp, check if we can start on the next path, or if we must wait for pathfinding to complete
				if (m_bNextDirectPath || (m_spNextGridPath->GetLength() > 0))
				{ 
					if (m_spNextGridPath->GetLength() > 0)
					{
						//Assume that we are at first node (although we probably ain't), since we will check for overstep anyway)
						m_iGridPos = 0;
						m_spCurGridPath->Copy(m_spNextGridPath);
					}
					m_spNextGridPath->Reset();
					m_SubDest = m_NextSubDest;
					m_NextSubDest = CVec3Dfp32(_FP32_MAX);
					m_bDirectPath = m_bNextDirectPath;
					m_bNextDirectPath = false;
					m_iCurGraphPos = m_iNextGraphPos;

					//Cancel any path switching
					m_bSwitchingPath = false;
					m_SwitchDest = CVec3Dfp32(_FP32_MAX);
				}
				else
				{
					//Pathfinding incomplete. Wait.

					_ActionMode = PAUSE;
					m_PrevActionMode = NORMAL; 
					m_PrevActionFlags = NONE;
					
					//Make sure we're trying to search
					if (m_SearchStatus == NO_SEARCH)
						m_SearchStatus = CXR_BlockNavSearcher::SEARCH_INSTANCE_INVALID;
					m_iCall++;
					return m_SubDest;
				}
			}
			
			//Check if we're making a direct move towards current node
			if (m_bDirectPath)
			{
				//Direct move. Check if we have to crouch
				if (m_pPathfinder->MustCrouch(_Pos, m_SubDest - _Pos))
				{
					m_PrevActionFlags |= CROUCH;
					_ActionFlags |= CROUCH;
				}
				else
				{
					m_PrevActionFlags &= ~CROUCH;
					_ActionFlags = NONE;
				}
				m_PrevActionMode = NORMAL;
				m_iCall++;
				return m_SubDest;
			}
			//Grid path following when not at sub destination is handled below
		}

		//If we get here we're not at (sub-) destination.and we don't have a straight path
		//Thus we should have a grid path
		if (m_spCurGridPath->GetLength() == 0)
		{
			//No grid path! Aaargh!
			Reset();
			_ActionMode = PAUSE;
			return (m_SubDest != CVec3Dfp32(_FP32_MAX)) ? m_SubDest : m_Destination;
		}

		// Grid pathfinder from hereon
		//Are we at current node?
		bool bAtNode = false;
		if (m_iGridPos >= m_spCurGridPath->GetLength())
		{	// This is really bisarre!
			m_iGridPos = m_spCurGridPath->GetLength()-1;
		}

		// *** START: Straighten first leg ***
		// First node, see if we can straightpath move to say 48 units away, if so we skip to there
		if ((m_iGridPos == 0)&&(m_iCall == 0)&&(smoothFlags & SMOOTH_STRAIGHTEN_FIRST_LEG)&&(m_spCurGridPath)&&(!m_bSwitchingPath)&&(_ActionMode == 0))
		{	// Step ahead 48 units and do a straightpath check
			CVec3Dfp32 PrevNodePos,NodePos;
			PrevNodePos = _Pos;
			NodePos = _Pos;
			fp32 RangeStepped = 0.0f;
			m_spCurGridPath->GetNodePos(m_iGridPos,&PrevNodePos);
			int32 N = m_spCurGridPath->GetLength()-1;
			for (int32 i = m_iGridPos+1; i < N; i++)
			{
				if (m_spCurGridPath->GetNodePos(i,&NodePos))
				{
					RangeStepped += (NodePos-PrevNodePos).Length();
					PrevNodePos = NodePos;
					if (RangeStepped >= 64.0f)
					{	// Can we do a straightline?
						if (m_pPathfinder->GroundTraversable(_Pos,NodePos))
						{
							m_spCurGridPath->SkipNodes(m_iGridPos,i);
							break;
						}
					}
				}
			}
		}
		// *** END: Straighten first leg ***

		CVec3Dfp32 Up = m_pPathfinder->m_pAI->GetUp();
		if (m_spCurGridPath->AtNode(m_iGridPos, _Pos, Up))
		{
			bAtNode = true;
		}
		else 
		{
			//Perhaps we have overstepped and are at a later node?
			for (int i = m_iGridPos + 1; i < m_spCurGridPath->GetLength(); i++)
			{
				if (m_spCurGridPath->AtNode(i, _Pos, Up))
				{	//Yup! We did overstep and are at a later node.
					bAtNode = true;
					m_iGridPos = i;
					break;
				};
			};
		}
		
		//Were we at node?
		if (bAtNode)
		{
			//We must crouch at any node that is a crouch node or has a crounch node successor
			if ((m_spCurGridPath->GetNode(m_iGridPos).GetFlags() & CXBN_SearchResultNode::CROUCH) ||
				((m_iGridPos + 1 < m_spCurGridPath->GetLength()) && (m_spCurGridPath->GetNode(m_iGridPos + 1).GetFlags() & CXBN_SearchResultNode::CROUCH)))
			{
				m_PrevActionFlags |= CROUCH;
			}
			else
			{
				//Need not crouch (anymore?)
				m_PrevActionFlags &= ~CROUCH;
			}

			//Is this last node?
			if (m_iGridPos == m_spCurGridPath->GetLength() - 1)
			{
				//We're at end of grid path. This is weird since this means we should be at 
				//either final destination or a sub destination. 
				Reset();
				_ActionMode = CAREFUL;
				return (m_SubDest != CVec3Dfp32(_FP32_MAX)) ? m_SubDest : m_Destination;
			}
			else
			{
				if (m_spCurGridPath->GetNode(m_iGridPos).GetFlags() & CXBN_SearchResultNode::AIRBORNE)
				{
					m_PrevActionMode = AIRBORNE;
				}
				else
				{
					m_PrevActionMode = NORMAL;
				}
				
				//Continue to next non-intermediate node (or last node)
				int iLastPos = m_iGridPos;
				m_iGridPos++;
				int iStart = m_iGridPos;
				int iEnd = iStart;
				
				if ((smoothFlags & SMOOTH_SKIP_INTERMEDIATE)&&(!m_bSwitchingPath))
				{	// *** Should we do this when not smoothing too? ***
					while ((m_spCurGridPath->GetNode(iEnd).GetFlags() & CXBN_SearchResultNode::INTERMEDIATE) &&
						(iEnd < m_spCurGridPath->GetLength()))
					{
						iEnd++;
					}

					// We can safely delete iLastPos+1 to m_iGridPos-1 to really skip them
					if (iEnd > iStart+1)
					{
						m_spCurGridPath->SkipNodes(iStart+1,iEnd);
					}
				}

				if ((!m_bSwitchingPath)&&(smoothFlags & SMOOTH_STRAIGHTEN_SEGMENT_1))
				{
					// *** Check all nodes from m_iGridPos to iNextPos for AIRBORNE,JUMP or CROUCH ***
					// Move this call to inbetween nodes and call every few frames to spread the cpu load ***
					int N = m_spCurGridPath->GetLength();
					if ((_ActionMode == 0)&&(m_iGridPos < N-2))
					{	// Try to optimize the path a bit
						// See if we can move in a straight path halfway betw cur and last node
						int iNextPos = (int)(m_iGridPos + ((N-1 - m_iGridPos) * Random * 0.999f));
						// OK; can we go from LastPos to NextPos?
						CVec3Dfp32 LastPos,NextPos;
						if ((iNextPos > m_iGridPos) && (iNextPos < m_spCurGridPath->GetLength())&&
							(m_spCurGridPath->GetNodePos(iLastPos,&LastPos))&&
							(m_spCurGridPath->GetNodePos(iNextPos,&NextPos)))
						{
							// Don't call expensive GroundTraversable unless fairly close (AI_PATHFIND_MAXSMOOTH units)
							if ((LastPos.DistanceSqr(NextPos) <= Sqr(AI_PATHFIND_MAXSMOOTH))&&(m_pPathfinder->GroundTraversable(LastPos,NextPos)))
							{
								m_iGridPos = iNextPos;
							}
						}
					}
				}

				//If we're getting airborne or jumping, continue to landing node
				if (_ActionMode == AIRBORNE)
				{
					m_iGridPos = GetLandingNode(m_spCurGridPath, m_iGridPos, m_PrevActionFlags);					
				}
				//Are we moving (on ground) towards a stop node?
				else if (m_spCurGridPath->GetNode(m_iGridPos).GetFlags() & CXBN_SearchResultNode::STOP_)
				{
					m_PrevActionMode = STOP;
				}
			}
		}

		if (!bAtNode)
		{
			// We alternate between checking next gridpos and random pairs
			if (!m_bSwitchingPath)
			{
				if ((m_iCall > 0)&&(smoothFlags & SMOOTH_STRAIGHTEN_START))
				{
					// We check if we can go straight towards a future node from our current position
					StartPos = m_pPathfinder->SafeGetPathPosition(_Pos, 1, 1);
					// N = m_spCurGridPath->GetLength();
					// Min of nbr of nodes and AI_PATHFIND_MAXSMOOTH / 8 
					N = Min(int(AI_PATHFIND_MAXSMOOTH * 0.125f), m_spCurGridPath->GetLength());
					iEnd = (int)(m_iGridPos + (N - m_iGridPos) * Random * 0.999f);
					if (m_spCurGridPath->GetNodePos(iEnd,&EndPos))
					{	// Don't call expensive GroundTraversable unless fairly close (AI_PATHFIND_MAXSMOOTH units)
						if ((StartPos.DistanceSqr(EndPos) <= Sqr(AI_PATHFIND_MAXSMOOTH))&&(m_pPathfinder->GroundTraversable(StartPos,EndPos)))
						{
							m_spCurGridPath->SkipNodes(m_iGridPos+1,iEnd-1);
						}
					}
				}
				
				if (smoothFlags & SMOOTH_STRAIGHTEN_SEGMENT_2)
				{
					// Then we waste some CPU on smoothing a random subsequent node pair
					N = m_spCurGridPath->GetLength();
					iStart = (int)(m_iGridPos + ((N - m_iGridPos) * Random * 0.999f));
					iEnd = (int)(iStart + ((N - iStart) * Random) * 0.999f);
					if ((iStart >= m_iGridPos)&&(iEnd > iStart+1)&&(iEnd < N))
					{

						if ((m_spCurGridPath->GetNodePos(iStart,&StartPos))&&
							(m_spCurGridPath->GetNodePos(iEnd,&EndPos)))
						{
							// Don't call expensive GroundTraversable unless fairly close (AI_PATHFIND_MAXSMOOTH units)
							if ((StartPos.DistanceSqr(EndPos) <= Sqr(AI_PATHFIND_MAXSMOOTH))&&(m_pPathfinder->GroundTraversable(StartPos,EndPos)))
							{
								m_spCurGridPath->SkipNodes(iStart+1,iEnd-1);
							}
						}
					}
				}

				if (smoothFlags & SMOOTH_STRAIGHTEN_END)
				{
					// We check if we can go straight towards a future node from our current position
					iStart = m_iGridPos;
					iEnd = m_spCurGridPath->GetLength()-1;
					if ((m_spCurGridPath->GetNodePos(iStart,&StartPos))&&
						(m_spCurGridPath->GetNodePos(iEnd,&EndPos)))
					{	// Don't call expensive GroundTraversable unless fairly close (AI_PATHFIND_MAXSMOOTH units)
						if ((StartPos.DistanceSqr(EndPos) <= Sqr(AI_PATHFIND_MAXSMOOTH))&&(m_pPathfinder->GroundTraversable(StartPos,EndPos)))
						{
							m_spCurGridPath->SkipNodes(m_iGridPos+1,iEnd-1);
						}
					}
				}
			}
		}

		//Maintain action mode (this is changed above when a new node is reached)
		_ActionMode = m_PrevActionMode;

		//Maintain crouching flag from previous (or possibly this) turn
		if (m_PrevActionFlags & CROUCH)
			_ActionFlags |= CROUCH;
		//Move towards current node position
		m_iCall++;
		return m_spCurGridPath->GetNode(m_iGridPos).GetPosition();
	}
};

CVec3Dfp32 CAI_Path::PropelPath(const CVec3Dfp32& _Pos, fp32 _Length, bool _bMove)
{
	CVec3Dfp32 PrevNodePos,NodePos;
	if ((m_spCurGridPath)&&(IsValid())&&(!m_bSwitchingPath))
	{
		fp32 RangeStepped = 0.0f;
		int32 N = m_spCurGridPath->GetLength()-1;
		INVALIDATE_POS(NodePos);
		if (m_bDirectPath)
		{	// Two(?) cases
			if (N <= 0)
			{	// 1: There are no grid positions: m_Destination is the one
				if (_Pos.DistanceSqr(m_Destination) >= Sqr(_Length))
				{
					NodePos = _Pos + ((m_Destination-_Pos).Normalize() * (_Length+8.0f));
				}
			}
			else
			{	// 2: There are gridpositions: m_SubDest is the one
				if (_Pos.DistanceSqr(m_SubDest) >= Sqr(_Length))
				{
					NodePos = _Pos + ((m_SubDest-_Pos).Normalize() * (_Length+8.0f));
				}
			}
			return(NodePos);
		}
		
		PrevNodePos = _Pos;
		for (int32 i = m_iGridPos+1; i < N; i++)
		{
			if (m_spCurGridPath->GetNodePos(i,&NodePos))
			{
				RangeStepped += (NodePos-PrevNodePos).Length();
				PrevNodePos = NodePos;
				if (RangeStepped >= _Length)
				{	// We have moved the required length (not neccessarily in a straight line though)
					if (_bMove)
					{
						m_iGridPos = i;
					}
					return(NodePos);
				}
			}
		}
	}

	INVALIDATE_POS(NodePos);
	return(NodePos);
};

CVec3Dfp32 CAI_Path::PropelPathToFirstNodeOutsideRange(CVec3Dfp32& _Pos, fp32 _Range, bool _bMove)
{
	CVec3Dfp32 NodePos;
	
	if ((m_bSwitchingPath)&&(VALID_POS(m_SwitchDest))&&(_Pos.DistanceSqr(m_SwitchDest) >= Sqr(_Range)))
	{
		return(m_SwitchDest);
	}
	if ((m_spCurGridPath)&&(IsValid())&&(!m_bSwitchingPath))
	{
		fp32 RangeStepped = 0.0f;
		int32 N = m_spCurGridPath->GetLength()-1;
		INVALIDATE_POS(NodePos);

		if (m_bDirectPath)
		{	// Two(?) cases
			if (N <= 0)
			{	// 1: There are no grid positions: m_Destination is the one
				if (_Pos.DistanceSqr(m_Destination) >= Sqr(_Range))
				{
					NodePos = _Pos + ((m_Destination-_Pos).Normalize() * (_Range+8.0f));
				}
			}
			else
			{	// 2: There are gridpositions: m_SubDest is the one
				if (_Pos.DistanceSqr(m_SubDest) >= Sqr(_Range))
				{
					NodePos = _Pos + ((m_SubDest-_Pos).Normalize() * (_Range+8.0f));
				}
			}
			return(NodePos);
		}

		NodePos = _Pos;
		for (int32 i = m_iGridPos; i < N; i++)
		{
			if (m_spCurGridPath->GetNodePos(i,&NodePos))
			{
				if (_Pos.DistanceSqr(NodePos) > Sqr(_Range))
				{
					if (_bMove)
					{
						m_iGridPos = i;
					}
					return(NodePos);
				}
			}
		}
	}
	else if ((VALID_POS(m_Destination))&&(_Pos.DistanceSqr(m_Destination) > Sqr(_Range)))
	{
		return(m_Destination);	
	}

	INVALIDATE_POS(NodePos);
	return(NodePos);
};

// This code steps ahead until a node at least _Length away is found.
// It then checks for traversability between his current pos and there and returns true if it IS traversable 
bool CAI_Path::CheckShortcut(fp32 _Length, CVec3Dfp32* _pSubDest)
{
	CVec3Dfp32 StartPos,PrevNodePos,NodePos;
	CXBN_SearchResult* pGridPath = m_spCurGridPath;

	if (m_bSwitchingPath)
	{
		pGridPath = m_spNextGridPath;
	}
	if ((pGridPath)||(pGridPath->GetLength() == 0))
	{
		return(false);
	}

	if ((m_pPathfinder)&&(m_pPathfinder->m_pAI)&&(m_iGridPos >= 0)&&(IsValid()))
	{
		StartPos = m_pPathfinder->m_pAI->GetBasePos();
		pGridPath->GetNodePos(m_iGridPos,&PrevNodePos);
		fp32 RangeStepped = (PrevNodePos-StartPos).Length();
		int32 N = pGridPath->GetLength()-1;
		for (int32 i = m_iGridPos+1; i < N; i++)
		{
			if (pGridPath->GetNodePos(i,&NodePos))
			{
				RangeStepped += (NodePos-PrevNodePos).Length();
				PrevNodePos = NodePos;
				if (RangeStepped >= _Length)
				{	// We have moved the required length (not necessarily in a straight line though)
					if (m_pPathfinder->GroundTraversable(StartPos,NodePos))
					{
						m_iGridPos = i;
						m_SubDest = NodePos;
						if (m_bSwitchingPath)
						{
							m_SwitchDest = NodePos;
						}
						if (_pSubDest)
						{
							*_pSubDest = NodePos;
						}
						return(true);
					}
				}
			}
		}
	}
	INVALIDATE_POS(NodePos);
	return(false);
};

// Forwards the path to the next node that is closest to _Pos
// _bFromStart Search from the very start of the path instead of where we are at
// _MaxLength < _FP32_MAX: we return the closest pos in that range
// _MaxLength == _FP32_MAX: We return thr first pos that is no longer closing on us (does that even make sense?)
bool CAI_Path::FindNextClosestNode(const CVec3Dfp32& _Pos, bool _bFromStart, fp32 _MaxLength)
{
	if ((m_spCurGridPath)&&(IsValid())&&(!m_bSwitchingPath))
	{
		fp32 RangeStepped = 0.0f;
		fp32 BestRangeSqr = Sqr(1000000.0f);
		int32 BestIndex = -1;
		CVec3Dfp32 PrevNodePos,NodePos;
		if (_bFromStart)
		{
			m_iGridPos = 0;
		}
		m_spCurGridPath->GetNodePos(m_iGridPos,&PrevNodePos);
		int32 N = m_spCurGridPath->GetLength();
		for (int32 i = m_iGridPos; i < N; i++)
		{
			if (m_spCurGridPath->GetNodePos(i,&NodePos))
			{
				RangeStepped += (NodePos-PrevNodePos).Length();
				PrevNodePos = NodePos;
				if (RangeStepped > _MaxLength)
				{
					if (BestIndex != -1)
					{
						m_iGridPos = i;
					}
					return(true);
				}
				else if ((_MaxLength == _FP32_MAX)&&(_Pos.DistanceSqr(NodePos) > BestRangeSqr))
				{
					if (BestIndex != -1)
					{
						m_iGridPos = i;
					}
					return(true);
				}
				if (_Pos.DistanceSqr(NodePos) < BestRangeSqr)
				{
					BestRangeSqr = _Pos.DistanceSqr(NodePos);
					BestIndex = i;
				}
			}
		}
	}

	return(false);
};

//Handles incoming messages
aint CAI_Path::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CAI_Path_OnMessage, 0);
	switch(_Msg.m_Msg)
	{
	case CAI_Core::OBJMSG_RELEASEPATH:
		{
			if ((_Msg.m_Param0 == CAI_Resource_Pathfinding::GRID) &&
				(m_iGridPFInstance != 0))
			{
				//Release grid and make sure we tríes to start search again, when possible
				ReleaseGridSearch();
				if (m_SearchStatus != NO_SEARCH)
					m_SearchStatus = CXR_BlockNavSearcher::SEARCH_INSTANCE_INVALID;
				return 1;
			}
			else
				return 0;
		}
	default:
		return 0;
	}
};


//Debug render graph path, current grid path and next grid path
void CAI_Path::Debug_Render(const CVec3Dfp32& _Pos, CWorld_Server * _pServer)
{
	if (!_pServer || !m_pPathfinder || !m_pPathfinder->GridPF())
		return;

	CVec3Dint16 Invalid = CVec3Dint16(-1);

	//Do we have a graph path?
	if (m_spGraphPath->GetLength() && m_pPathfinder->GraphPF())
	{
		//Render graph path
		CVec3Dint16 From, To;
		for (int i = 1; i < m_spGraphPath->GetLength(); i++)
		{
			From = m_pPathfinder->GraphPF()->GetNodePos(m_spGraphPath->GetNode(i-1));
			To = m_pPathfinder->GraphPF()->GetNodePos(m_spGraphPath->GetNode(i));
			if ((From != Invalid) && (To != Invalid))
			{
				_pServer->Debug_RenderWire(m_pPathfinder->GetBlockNav()->GetWorldPosition(From), m_pPathfinder->GetBlockNav()->GetWorldPosition(To), kColorGreen, 0.05f);
			}
		}

		//Render next grid path or direct path if any
		if (m_bNextDirectPath)
		{
			_pServer->Debug_RenderWire(m_SubDest, m_NextSubDest, kColorBlue, 0.05f);
		}
		else if (m_spNextGridPath->GetLength() > 0)
		{
			CVec3Dfp32 From, To;
			for (int i = 1; i < m_spNextGridPath->GetLength(); i++)
			{
				From = m_spNextGridPath->GetNode(i - 1).GetPosition();
				To = m_spNextGridPath->GetNode(i).GetPosition();
				_pServer->Debug_RenderWire(From, To, kColorYellow, 0.05f);
			}
		}

		//Render switch path if any
		if (m_bSwitchingPath)
		{
			_pServer->Debug_RenderWire(_Pos, m_SwitchDest, kColorCyan, 0.05f);
		}
	}

	//Render direct path to destination or subdestination if any
	if (m_bDirectPath)
	{
		if (m_SubDest == CVec3Dfp32(_FP32_MAX))
			_pServer->Debug_RenderWire(_Pos, m_Destination, kColorCyan, 0.05f);
		else
			_pServer->Debug_RenderWire(_Pos, m_SubDest, kColorCyan, 0.05f);
	}
	//Render current grid path if any
	else if (m_spCurGridPath->GetLength() > 0)
	{
		CVec3Dfp32 From, To;
		for (int i = 1; i < m_spCurGridPath->GetLength(); i++)
		{
			From = m_spCurGridPath->GetNode(i - 1).GetPosition();
			To = m_spCurGridPath->GetNode(i).GetPosition();
			if (i >= m_iGridPos)
			{
				_pServer->Debug_RenderWire(From, To, kColorCyan, 0.05f);
			}
			else
			{
				_pServer->Debug_RenderWire(From, To, kColorPurple, 0.05f);
			}
		}
	}
}
