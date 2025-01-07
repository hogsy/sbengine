#ifndef _INC_WNAVGRAPH_PATHFINDER
#define _INC_WNAVGRAPH_PATHFINDER

#include "../../../XR/XRNavGraph.h"
#include "MDA_PQueue.h"

class CWorld_NavGraph_Path;
class CWorld_Server;

//Simple hash which hashes indexes on position
class C3DHash
{
private:
	class CHashElt
	{
	public:
		int16 m_iIndex;
		int16 m_iIndex2;
		int16 m_iNext;
		
		CHashElt()
		{
			m_iIndex = -1;
			m_iIndex2 = -1;
			m_iNext = -1;
		};
	};

	//All elements in the hash
	TThinArray<CHashElt> m_lHashElts;

	//Index of first "empty" element in element list
	int16 m_iFirstEmpty;
	
	//Iteration hashelt index, when getting nodes of a box
	int16 m_iNextElt;

	//The hash list (each slot holds the index to the first belonging element)
	TThinArray<int16> m_liHash;

	//Get hash list index given position
	int16 HashValue(const CVec3Dint16& _Pos);

public:
	C3DHash(){};

	//Set max size of hash
	C3DHash(int _Size);

	//Add index with position. Fails if position is invalid or hash is full
	bool Add(int16 _iIndex, int16 _iIndex2, const CVec3Dint16& _Pos);

	//Check if index with given position is in hash
	bool Find(int16 _iIndex, const CVec3Dint16& _Pos);

	//Get corresponding index2 of index with given position, or -1 if not in hash
	int16 GetIndex2(int16 _iIndex, const CVec3Dint16& _Pos);

	//Clear hash elements
	void Clear();
};


//Pathfinder for navgraph, virtual base class. All distances and positions are in the navgrid.
//Cannot be abstract class and still dynamically declared in mrtc? 
class CWorld_Navgraph_Pathfinder : public CReferenceCount
{

	MRTC_DECLARE;


public:
	//Initialize pathfinder with number of search instances and max number of expanded nodes
	virtual void Create(CXR_NavGraph * _pGraph, int _nSearchInstances, int _MaxNodes) pure;

	//Get approximate minimum distance between nodes. If correctly built any traversable
	//position in the world should be within this distance from a navgraph node
	virtual int GetMinDistance() pure;

	//Fill the provided list with all node-indices within the minimum distance of the 
	//given position, sorted with closest node first. Return the number of nodes
	//Optionally we can cull nodes with no edges leading to or from them that can be 
	//used by agents of given size group
	virtual int GetNodesAt(const CVec3Dint16& _Pos, TThinArray<int16>* _pRes, bool _bMustHaveFromEdges = false, bool _bMustHaveToEdges = false, int _SizeGroup = CXR_NavGraph::SIZE_DEFAULT) pure;

	//Get node grid position, given index. Returns -1 vector on invalid index.
	virtual CVec3Dint16	GetNodePos(int16 _iNode) pure;
	
	//Create search, given node indices to the origin and destination and optionally alternate search 
	//destinations and maximum cost (approx cell distance) of search.
	//Returns index to search instance or -1 if no search instance is available
	virtual int Search_Create(int _iOrigin, int _iDestination, int _Size = CXR_NavGraph::SIZE_DEFAULT, int _iUser = 0, int _iMaxCost = -1, int _GameTick = -1) pure;
	virtual int Search_Create(int _iOrigin, int _iDestination, int _Size, const int16* _lAlternates, int _nAlternates, int _iUser = 0, int _iMaxCost = -1, int _GameTick = -1) pure;

	//Perform previously created search, given instance index and max number of nodes
	//to expand in one go. Reduces the the latter value by the number of nodes expanded
	//and returns one of the values below. 
	enum {
		SEARCH_UNAVAILABLE = -1, 
		SEARCH_NO_PATH = 0,
		SEARCH_FOUND_PATH,
		SEARCH_FOUND_ALTERNATE,
		SEARCH_IN_PROGRESS,
		SEARCH_INVALID,
	};
	virtual int Search_Execute(int _iSearchInstance, int& _nNodeExpansions, int * _pAlternateFound = NULL) pure;

	//Set the supplied path to the search result. If the partial flag is true, we do this even if 
	//search isn't done. Succeeds if search is done, fails otherwise.
	virtual bool Search_Get(int _iSearchInstance, CWorld_NavGraph_Path * _pRes, bool _bPartial = false) pure;

	//Set supplied path to the found path that ends in given node if such exists. Fails otherwise.
	virtual bool Search_Get(int _iSearchInstance, CWorld_NavGraph_Path * _pRes, int _iEndNode) pure;

	//Release given search instance
	virtual void Search_Destroy(int _iSearchInstance, int _GameTick = -1) pure;

	//Get user of give search instance
	virtual int Search_GetUser(int _iSearchInstance) pure;

	//Get Number of search instances
	virtual int Search_GetNum() pure;

	//Invalidate edge (duh) between given nodes fro given sizes. This should be handled automatically by checking edges through dirty tiles actually.
	virtual void InvalidateEdge(int _iFrom, int _iTo, int _Sizes) pure;
	
	//Set edge cost (given cost is "distance"-relative) depending on size (Only default can set cost for now). This should be handled automatically later.
	virtual void SetEdgeCost(int _iFrom, int _iTo, int _Cost, int _Sizes) pure;

	//Get best size group supported by graph, given base size square radius and height in grid cells
	virtual int GetBestSize(int _BaseSize, int _Height) pure;

	//Debug render search instance
	virtual void DebugRender(int _Search, CWorld_Server * _pServer) pure;

	//Debug render graph
	virtual void DebugRenderGraph(CWorld_Server * _pServer, const CVec3Dint16& _GridPos, int _GridRadius, fp32 _Duration, int _Size = CXR_NavGraph::SIZE_INVALID) pure;
};


//Basic AStar pathfinder for navgraph
class CWorld_Navgraph_Pathfinder_AStar : public CWorld_Navgraph_Pathfinder
{ 
	MRTC_DECLARE;

protected:
	//Search instance for AStar search
	class CSearchInstance_AStar
	{
	protected:
		//Search nodes
		class CSearchNode_AStar
		{
		public:
			//This and parent node; indices into m_pGraph->m_lNodes
			int16 m_iNode;
			CSearchNode_AStar * m_pParent;

			//Cost and heuristic cost
			int16 m_Cost;
			int16 m_Heuristic; 

			CSearchNode_AStar(int16 _iNode = -1, CSearchNode_AStar * _pParent = NULL, int16 _Cost = 0, int16 _Heuristic = 0)
			{
				m_iNode = _iNode;
				m_pParent = _pParent;
				m_Cost = _Cost;
				m_Heuristic = _Heuristic;
			};
			
			//Needed for prio queue
			int GetPriority()
			{
				return m_Cost + m_Heuristic;
			}
		};

	protected:
		//The NavGraph and pathfinder
		CXR_NavGraph * m_pGraph;
		CWorld_Navgraph_Pathfinder_AStar * m_pPF;

		//Search node list, prio queue and hash
		TThinArray<CSearchNode_AStar> m_lSearchNodes;
		TPriorityQueue2<CSearchNode_AStar> m_PQueue;
		C3DHash m_NodeHash;

		//First empty position in node list
		int m_iFirstEmpty;

		//Size group of searching agent
		int m_Size;

		//Max cost of search
		int m_MaxCost;

		//User ID of search instance
		int m_iUser;

		//Origin and destination (indices into m_pGraph->m_lNodes)
		int16 m_iOrigin;
		int16 m_iDest;

		//Search status (one of the CWorld_Navgraph_Pathfinder search statuses)
		int m_Status;

		//Alternates
		TArray<int16> m_lAlternates;

		//Approx distance to destination from given node
		int16 Heuristic(int16 _iNode);

		//Destination search node
		CSearchNode_AStar * m_pDest;

		//Is this instance allocated?
		bool m_bAllocated;

		//The server game tick the instance was last released
		int m_LastReleasedTick;

	public:
		void Init(CXR_NavGraph * _pGraph, CWorld_Navgraph_Pathfinder_AStar * _pPF, int _nNodes);
		void Search_Reset();
		void Search_Create(int _iOrigin, int _iDestination, int _Size, const int16* _lAlternates, int _nAlternates, int _iUser = 0, int _iMaxCost = -1);
		int Search_Execute(int& _nNodeExpansions, int * _pAlternateFound = NULL);
		bool Search_Get(CWorld_NavGraph_Path * _pRes, int _iEndNode = -1, bool _bPartial = false);
		int Search_GetUser();
		
		bool IsAllocated(int _GameTick = -1);
		void Allocate();
		void Release(int _GameTick = -1);

		//Debug render search instance
		virtual void DebugRender(CWorld_Server * _pServer);
	};

	//The search instances
	TThinArray<CSearchInstance_AStar> m_lSearchInstances;

	//The static navgraph
	CXR_NavGraph * m_pGraph;

	//Dynamic edge data, this must be fixed properly later. There's a corresponding edge state for 
	//each edge in static graph, with same index of course
	TThinArray<CXR_NavGraph_EdgeState> m_lEdgeStates;	
	
	//Helper to QSort_r
	void Swap(TThinArray<int16>* _pItems, TThinArray<int>* _pValues, int _i, int _j);

	//Sorts the first array ascendingly based on the values in the second array
	void QSort_r(TThinArray<int16>* _pItems, TThinArray<int>* _pValues, int _iStart, int _iEnd);

public:
	//Class specific methods

	//Check if given edge can be reliably traversed by agent of given size
	bool TraversableEdge(uint16 _iEdge, int _Size);

	//Get current cost of edge
	int EdgeCost(uint16 _iEdge); 

	//Check if there are any edges traversable by agents of given size leading away from node
	bool CheckFromEdges(int16 _iNode, int _Size);

	//Check if there are any edges traversable by agents of given size with node as destination
	bool CheckToEdges(int16 _iNode, int _Size);

public:
	//Inherited pathfinder interface
	virtual void Create(CXR_NavGraph * _pGraph, int _nSearchInstances, int _MaxNodes);

	virtual int GetMinDistance();
	virtual int GetNodesAt(const CVec3Dint16& _Pos, TThinArray<int16>* _pRes, bool _bMustHaveFromEdges = false, bool _bMustHaveToEdges = false, int _SizeGroup = CXR_NavGraph::SIZE_DEFAULT);
	virtual CVec3Dint16	GetNodePos(int16 _iNode);
	
	virtual int Search_Create(int _iOrigin, int _iDestination, int _Size = CXR_NavGraph::SIZE_DEFAULT, int _iUser = 0, int _iMaxCost = -1, int _GameTick = -1);
	virtual int Search_Create(int _iOrigin, int _iDestination, int _Size, const int16* _lAlternates, int _nAlternates, int _iUser = 0, int _iMaxCost = -1, int _GameTick = -1);
	virtual int Search_Execute(int _iSearchInstance, int& _nNodeExpansions, int * _pAlternateFound = NULL);
	virtual bool Search_Get(int _iSearchInstance, CWorld_NavGraph_Path * _pRes, bool _bPartial = false);
	virtual bool Search_Get(int _iSearchInstance, CWorld_NavGraph_Path * _pRes, int _iEndNode);
	virtual void Search_Destroy(int _iSearchInstance, int _GameTick = -1);
	virtual int Search_GetUser(int _iSearchInstance);
	virtual int Search_GetNum();
	virtual void InvalidateEdge(int _iFrom, int _iTo, int _Sizes);
	virtual void SetEdgeCost(int _iFrom, int _iTo, int _Cost, int _Sizes);
	virtual int GetBestSize(int _BaseSize, int _Height);
	virtual void DebugRender(int _Search, CWorld_Server * _pServer);
	virtual void DebugRenderGraph(CWorld_Server * _pServer, const CVec3Dint16& _GridPos, int _GridRadius, fp32 _Duration, int _Size = CXR_NavGraph::SIZE_INVALID);
};


//Navgraph path. Just holds info about node indices in path
class CWorld_NavGraph_Path : public CReferenceCount
{
	MRTC_DECLARE;

protected:
	//The navgraph this path is in
	CXR_NavGraph * m_pGraph;

	//The path of nodes
	TArray<int16> m_liPath;

public:
	CWorld_NavGraph_Path();
	
	//Clear path
	virtual void Reset();

	//Get node index at given position in path, or -1 if there is no such position
	virtual int16 GetNode(int _iPos);

	//Get grid position of node at given position in path, or CVec3Dint16(-1) if there is no such position
	virtual CVec3Dint16 GetNodePosition(int _iPos);

	//Check if given position is within the given distance (in grid cells) from the given node position in path
	virtual bool IsAt(const CVec3Dint16& _Pos, int _iPos, fp32 _Radius);

	//Get length of path
	virtual int GetLength();

	//Build path, given node index list
	virtual void BuildPath(CXR_NavGraph * _pGraph, const int16* _lPath, int _nPathLen);

	//Build path from other path data
	virtual void Copy(CWorld_NavGraph_Path * _pPath);
};

typedef TPtr<CWorld_NavGraph_Path> spCWorld_NavGraph_Path;


#endif
