#ifndef _INC_WNAVGRAPH
#define _INC_WNAVGRAPH


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_NavGraph_Builder
|__________________________________________________________________________________________________
\*************************************************************************************************/

//Don't build navgraph on consoles (memory-heavy)
//#ifndef PLATFORM_CONSOLE

#include "XRNavGraph.h"
#include "XRBlockNavGrid.h"
#include "XRBlockNavInst.h"



//Provides functionality for building a navgraph and writing it to file
class CXR_NavGraph_Builder : public CXR_NavGraph_Writer
{
	MRTC_DECLARE;

public:	
	//Container class for node position/cost pair
	class CPosCost
	{
	public:
		CVec3Dint16 m_Pos;
		uint16 m_Cost;
		
		CPosCost(const CVec3Dint16& _Pos = 0, uint16 _Cost = 0)
		{
			m_Pos = _Pos;
			m_Cost = _Cost;	
		}

		void Set(const CVec3Dint16& _Pos = 0, uint16 _Cost = 0)
		{
			m_Pos = _Pos;
			m_Cost = _Cost;	
		}

		const CPosCost& operator=(const CPosCost& _PosCost)
		{
			m_Pos = _PosCost.m_Pos;
			m_Cost = _PosCost.m_Cost;
			return *this;
		}
	};

protected:

	//Keeps track of whether positions in a 3D matrix have been checked or not
	class CCheckMatrix
	{
	private:
		//The bit matrix
		TThinArray<int64> m_lMatrix;

		//Dimension factors: position (x,y,z) is at "bit-index" x * m_iYZ + y * m_iZ + z
		int32 m_iYZ;
		int32 m_iZ;

		//Base height of check matrix
		int16 m_BaseHeight;

	public:
		//Constructor. Sets dimensions and base height and resets all bits
		CCheckMatrix(int16 _x = 0, int16 _y = 0, int16 _z = 0, int16 _BaseHeight = 0);

		//Get value at given position
		bool Get(int16 _x, int16 _y, int16 _z);

		//Set value at given position to 1 and 0 respectively
		void Set(int16 _x, int16 _y, int16 _z);
		void Reset(int16 _x, int16 _y, int16 _z);
		// Sets all points within _x +-_dx, _z +-_dz, _z +-_dz
		void Set(int16 _x, int16 _y, int16 _z,int16 _dx, int16 _dy, int16 _dz);
		// Sets/resets the entire matrix
		void Set();
		void Reset();

		//Check if given position is in check matrix
		bool ValidPos(int16 _x, int16 _y, int16 _z);

		//Get layer bottom and top grid heights
		int16 GetLayerBottom();
		int16 GetLayerTop();
	};


	//Pathfinder used for building navgraph by performing specialized breadth-first 
	//and A* searches on existing navgrid 
	class CNB_Pathfinder : public CXBN_SearchInstance
	{
	protected:
		//Special search node flags to use in AStarFindEdge
		enum {
			WBN_NODE_JUMPPATH = WBN_NODE_MAX << 1,
			WBN_NODE_FALLPATH = WBN_NODE_MAX << 2,
		};

		//Size of searcher (in cells)
		int m_iBaseSize;
		int m_iHeight;

		//Current search mode for use in CanPrune and VisitNode methods
		enum {
			SEARCH_DEFAULT = 0,
			SEARCH_PLACENODES_BF,
			SEARCH_FINDEDGE_AS,
		};
		int m_iSearchMode;

		//Default: check if position has a node on hash (A* mode for normal searches)
		//Check if position has a node on hash or is checked in check matrix if _iParam is 1 (breadth-first mode for placing nodes)
		virtual bool CanPrune(const CVec3Dint16& _Pos);	

		//Insert node in hash. Don't mark as visited until we expand node.
		virtual void VisitNode(CXBN_SearchNode * _pN);

	public:

		//Check matrix to use
		CCheckMatrix * m_pCheck;

		CNB_Pathfinder();

		//Set initialization stuff. Must be called before using any of the other methods, for proper params to be set
		void Create(CXR_BlockNav * _pBlockNav, int _TileHeap, int _MaxNodes, int _iBaseSize, int _iHeight);

		//Must only be called before each node placing turn
		void Reset(CCheckMatrix * _pCheck);
		
		//Find closest traversable ground position in grid given grid position
		CVec3Dint16 FindPathPosition(const CVec3Dint16& _Start, int _Radius);

		//Rough scan of the layer starting at _MinStart value and going down _CheckHeight levels. 
		//The _TightCheckMax/Min values spans the box where the scan should be more careful.
		//CVec3Dint16 FindStartPosition(CVec3Dint16& _MinStart, int16 _CheckHeight, const CVec3Dint16& _TightCheckMax, const CVec3Dint16& _TightCheckMin);
		CVec3Dint16 FindStartPosition(CVec3Dint16& _MinStart, int16 _CheckHeight);

		//Finds traversable ground position that hasn't been visited yet
		// *** CVec3Dint16 FindStartPosition(TArray<spCRegistry> _lspObjects, int& _iMinObject, CVec3Dint16& _MinStart);

		// Scan map for suitable scenepoints/playerstarts(?) to use to nudge graphnodes
		// *** int32 FindNodePosCandidates(TArray<spCRegistry> _lspObjects);

		//Check if we can move in a straight line from one position to another, without jumping or falling. Returns cost if successful, else 0. 
		int StraightPath(const CVec3Dint16& _From, const CVec3Dint16& _To);

		//Finds all unvisited positions at given depth that can be reached from given position.
		//found positions in air is expanded until a ground position is found, or deleted if no such can be found
		//The result is added to the given array and the number of found valid positions is returned.
		int BreadthFirstFindPositions(const CVec3Dint16& _StartPos, int _iDepth, TThinArray<CPosCost> * _plResult);

		//Checks if there's a path from the origin to the destination that is shorter than the given ground distance. 
		//or, if ending the path with a fall or jump, shorter than the given jump and fall distances. If the ground tail 
		//param is set, paths with a fall or jump are allowed to have this number of ground cells after the jump/fall
		//Unless the optional basesize and height params are set, the preset values are used. If the optional result
		//pointer is given, this is set to any found path (i.e. sequence of navgrid positions)
		//Returns one of the below results and sets given cost variable to cost of path (0 on failure)
		enum {
			NO_EDGE = 0,	//Can't find any valid edge
			UNDIRECTED_EDGE,//Found edge without any jumps or falls
			DIRECTED_EDGE,	//Found edge with one or more jumps or falls (there might still be an edge in the opposite direction, but it can't be traced exactly backwards)
		};
		int AStarFindEdge(const CVec3Dint16& _From, const CVec3Dint16& _To, int _iGroundDist, int _iJumpDist, int _iFallDist, int _iGroundTail, int& _iCost, int _BaseSize = -1, int _Height = -1, TArray<CVec3Dint16> * _plResult = NULL);

		//The maximum fall height permitted by pathfinding, in cells
		int MaxFall();

		//The maximum distance one can jump according to pathfinding, in cells. 
		int MaxJump();

		//Minimum "world unit" cost of traversal between the given grid positions
		int MinCost(CVec3Dint16 _From, CVec3Dint16 _To);

		//Check if one can move between given positions in a straight line without having to jump or fall too much
		//Optional arguments are base size and height if these should differ from the preset ones.
		bool IsGroundTraversable(CVec3Dint16 _From, CVec3Dint16 _To, int _BaseSize = -1, int _Height = -1);

		// Simpler pos checker, checks IsValidGround and IsTraversable
		bool IsValidGroundTraversable(CVec3Dint16 _From);

		//Check traversability of given sequence of grid positions
		bool CheckTraversability(int _nPos, const CVec3Dint16 * _lPos, int _BaseSize, int _Height);

		CVec3Dint16 GetGridPosition(CVec3Dfp32 _Pos);
	};


	//Search node for construction graph 
	class CConstructionSearchNode
	{
	public:
		//Node index in m_lCNodes array
		int16 m_iNode;
		int16 m_iHeuristicCost;
		int16 m_iCost;

		CConstructionSearchNode(int16 _iNode = -1, int16 _iHeuristicCost = 0, int16 _iCost = 0);
		int GetPriority();
	};

	//Edge representation while in the construction stage
	class CConstructionEdge
	{
	public:
		//Origin and destination (Indices into m_lCNodes list)
		int16 m_iFrom; 
		int16 m_iTo;

		//Approximate cost of traversing edge
		uint16 m_Cost;

		//Size groups that can traverse edge
		uint8 m_Sizes;
	};

	//Node representation while in the construction stage
	class CConstructionNode
	{
	private:
		//Must use special class for prio queue
		class CNeighbourCandidate
		{
		public:
			int32 m_iDistSqr;
			int16 m_iNode;  //Index into m_lCNodes list
			CNeighbourCandidate(int16 _iNode = -1, int32 _iDistSqr = 0)
			{
				m_iNode = _iNode;
				m_iDistSqr = _iDistSqr;
			}
		};

		//First empty positions in edge list and neighbour candidate list
		int m_iFirstEmpty;
		int m_iEmptyCandidate;

		//Helper to QSort_r
		void Swap(TThinArray<CNeighbourCandidate>* _pNCs, int _i, int _j);

		//Should make a template of these two, but it's faster to just copy code for now ;)
		//Sorts the neighbour candidates ascendingly based on their distsqr values
		void QSortNCs_r(TThinArray<CNeighbourCandidate>* _pNCs, int _iStart, int _iEnd);
		//Sorts the nodes edge indices according to cost of the corresponding edges (which are given in the _pCEdges list pointer)
		void QSortEdges_r(TThinArray<int32>* _piEdges, int _iStart, int _iEnd, TThinArray<CConstructionEdge>* _pCEdges);

	public:
		CVec3Dint16	m_GridPos;
		uint8 m_HeightLevels;
		int8 m_Flags;

		//Indices into m_lCEdges list, -1 if culled
		TThinArray<int32> m_lEdges;	

		//Potential neighbours. -1 if removed
		TThinArray<CNeighbourCandidate> m_lNeighbourCandidates;

		CConstructionNode();

		//Add edge index
		void AddEdge(int32 _iEdge);

		//Add neighbour candidate index
		void AddNeighbourCandidate(int16 _iNode, int32 _iDistSqr);

		//Sort neighbour candidates ascendingly according to distsqr
		void SortNeighbourCandidates();

		//Sort indices of edges ascendingly according to cost, given the construction edge array that the indices point into
		void SortEdges(TThinArray<CConstructionEdge>* _pCEdges);

		//Number of edges and neighbour candidates
		int NumEdges();
		int NumNeighbourCandidates();
	};

	//Found objects in layer
	CVec3Dint16 m_ObjectBoundMin;
	CVec3Dint16 m_ObjectBoundMax;
	TArray<CVec3Dint16> m_lObjGridPos;

	//The construction graph
	TThinArray<CConstructionNode> m_lCNodes;
	TThinArray<CConstructionEdge> m_lCEdges;

	//The first empty slots in the nodes and edges arrays
	int16 m_iEmptyNode;
	int32 m_iEmptyEdge;

	//Allocates space for the specified number of nodes and edges, respectively
	void AllocateNodeSpace(int _n);
	void AllocateEdgeSpace(int _n);
	
	//Place nodes from navnodes and clear _Radius around each
	//Return nbr of actually placed nodes.

	int FindNodeCandidatesFromRegistry(TArray<spCRegistry>& _lspObjects, CNB_Pathfinder& _Search);
	void PlaceNodesFromRegistry(CCheckMatrix* _pCheck, int _CheckRadius, int _CheckHeight);
	CVec3Dint16 FindStartPosition(CCheckMatrix& _Check, int _Radius, int _Height);
	// Looks in m_lObjGridPos for nodes within _Radius(grid units). Remove from m_lObjGridPos when _bRemove == true
	bool AdjustNodePosition(CCheckMatrix* _pCheck,CVec3Dint16& _Pos,int _Radius,int _Height,bool _bRemove = true);

	//Place new node at given position, allocating more space if necessary. Return index.
	int PlaceNode(const CVec3Dint16& _Start);			

	//Place new edge from one node to another with given cost, allocating more space if necessary Return index.
	int PlaceEdge(int16 _iFrom, int16 _iTo, uint16 _iCost);			

	//Cover the entire traversable area that can be reached from the given start node (index) with nodes
	void PlaceNodes_r(CNB_Pathfinder * _pPathFinder, int _iParet);

	//Check if there's an edge from the given origin to the given destination
	bool HasEdgeTo(int16 _iFrom, int16 _iTo);

	//Check if there is a "good" edge-path from the origin to the destination
	bool HasEdgePathStepLimit(int16 _iFrom, int16 _iTo);

	//Check if there is a path from the given origin to the destination and place edge(s) as appropriate
	void FindEdge(CNB_Pathfinder * _pPathFinder, int16 _iFrom, int16 _iTo);

	//Check if there's an edge path from the origin to the destination that has a total cost lower than the 
	//given one and does not traverse any edge wisth higher than the given edge cost.
	bool HasEdgePathCostLimit(CNB_Pathfinder * _pPathFinder, int16 _iFrom, int16 _iTo, int _MaxCost, int _MaxEdgeCost);

	//Fill the given list with the size groups in the given sizes flag field, sorted descendingly by base size. Optionally exclude the given size group from list.
	void SortSizeGroups(int _Sizes, TArray<int>* _pRes, int _iExcludeSize = -1);

	//Check if existing edge is traversible for characters of given sizes and set size flags accordingly
	void CheckEdgeSizes(CNB_Pathfinder * _pPathFinder, CConstructionEdge * _pEdge, int _nSizes, const int * _lSizes);

public:
	
	//Clear normal and construction graph, respectively
	virtual void Clear();
	virtual void ClearConstruction();
	
	//Construct graph from given blocknav, starting point, desired minimum distance between nodes
	//and desired size groups
	virtual void BuildGraph(CXR_BlockNav* _pBlockNav,TArray<spCRegistry>& _lspObjects,const CVec3Dint16& _Start, int _iMinDist, int _Sizes);

};




//#endif



#endif
