#ifndef _INC_WOBJ_NAVNODE
#define _INC_WOBJ_NAVNODE

#include "../AI_Def.h"

class CAI_NavNodePath;

#ifndef M_DISABLE_TODELETE

//Navigation node, for alternative navigation purposes
class CWObject_NavNode : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;

protected:
	//Neighbouring nodes
	TArray<int> m_liNeighbours;
	
	//Target names of neighbouring nodes
	TArray<CStr> m_lsNeighbours;

public:
	//Constructor
	CWObject_NavNode();

	//Get object "parameters" from keys
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);

	//Set up object 
	virtual void OnSpawnWorld();

	//Object messages
	enum {
		//Returns number of neighbours and sets provided data to pointer to neighbours array
		OBJMSG_GETNEIGHBOURS = OBJMSGBASE_NAVNODE,		
	};

	//Object message interface
	virtual aint OnMessage(const CWObject_Message& _Msg);
};



//Virtual base class for pathfinding on a navnode graph (completely unoptimized)
class CAI_NavNodePathfinder
{
protected:
	//Server pointer
	CWorld_Server * m_pServer;

	//Internal representation of directed NavNode-graph edges
	class CNavNode;
	class CEdge
	{
	public:
		CEdge(int _iDest = NULL, int _iCost = -1);

		//Destination node graph index
		int m_iDest;

		//Cost of traversing edge
		int m_iCost;

		//Debug stuff
		void DebugRenderEdge(CPixel32 _iClr, fp32 _Time);
	};

	//Internal representation of NavNode-graph nodes
	class CNavNode
	{
	public:
		CNavNode(int _iObject = 0);

		//Edges from node
		TArray<CEdge> m_lEdges;

		//World index of corresponding node
		int m_iObject;

		//Get cost of getting from this node to the given child, or 0 if child is invalid
		int GetEdgeCost(CNavNode * _pChild);
	};

	//The navgraph
	TArray<CNavNode> m_lGraph;

	//Get navgraph index of given world index object or -1 if object is not in graph
	int GetGraphIndex(int _iNode);

	//Get pointer to graph node given world index of corresponding NavNode object
	CNavNode * GetNode(int _iNode);

	//Calculates cost between two navnodes. Cost equals distance for base class..
	virtual int GetCost(CWObject * pFrom, CWObject * pTo);

	//Builds internal navgraph from given start node object
	virtual void BuildGraph(CWObject * _pStartNode);
	virtual void BuildGraph_r(CWObject * _pParent, int _iIndex);

public:
	//Constructor
	CAI_NavNodePathfinder(CWorld_Server	* _pServer = NULL, int _StartNode = 0);

	//Find path given the world object indices of the start and destination node. 
	//If successful, sets the given path object to the found path
	virtual bool FindPath(int _iStart, int _iDest, CAI_NavNodePath * _pPath) = 0;	

	//Returns the number of children of the node corresponding to the given world index and sets 
	//the given result pointer to the list of children. 
	virtual int GetChildren(int _iNode, int * _piChildren);

	//Get random child node world index of node corresponding to given object index
	virtual int GetRandomChild(int _iNode);

	//Get the object index of the nav node closest to the given position
	virtual int GetClosestNode(CVec3Dfp32 _Pos);

	//Debug stuff
	void DebugRenderEdge(CNavNode * _pStart, int _iEdge, CPixel32 _iClr, fp32 _Time = 5.0f);
	void DebugRenderNodes(CPixel32 _iClr, fp32 _Time = 0.05f);
	void DebugRenderEdges(CPixel32 _iClr, fp32 _Time = 0.05f);
	void DebugRenderGraph(CPixel32 _iClr, fp32 _Time = 0.05f);
};


//Base class for A*-pathfinding 
class CAI_NavNodePathfinder_AStar : public CAI_NavNodePathfinder
{
protected:

	//Search node for A* search
	class CSearchNode
	{
	public:
		CSearchNode(CNavNode * _pNode = NULL, CSearchNode * _pParent = NULL, int _iHereCost = -1, int _iHeuristicCost = -1, int _iIndex = 0);

		//Parent node
		CSearchNode * m_pParent;

		//Graph node
		CNavNode * m_pNode;

		//Get cost 
		int GetPriority();

		//The cost of getting here and heuristic cost of getting to destination
		int m_iHereCost;
		int m_iHeuristicCost;

		//Index in node (closed) list
		int m_iIndex;
	}; 

	//Open and closed lists (All expanded nodes will be stored on closed list)
	TPriorityQueue2<CSearchNode> m_lOpen;
	TArray<CSearchNode> m_lNodes;

	//Admissable heuristic cost of getting from one node to the other
	int Heuristic(CNavNode * _pFrom, CNavNode * _pTo);

public:
	//Constructor
	CAI_NavNodePathfinder_AStar(CWorld_Server	* _pServer = NULL, int _StartNode = 0);

	//Use A* algorithm to find path.
	virtual bool FindPath(int _iStart, int _iDest, CAI_NavNodePath * _pPath);	
};


//Simple class for holding and using a path of successive (NavNode) objects
class CAI_NavNodePath
{
protected:
	//The server
	CWorld_Server * m_pServer;

	//The path
	TArray<int> m_liNavNodes;

public:
	//Constructor
	CAI_NavNodePath(CWorld_Server * _pServer = NULL);

	//Build path from given integer list and length
	void SetPath(const int * _piNodes, int _nLength);

	//Add node to end of path
	void AddNode(int _iNode);

	//Get node object at position
	CWObject * GetNode(int _iPos);

	//Get node index at position
	int GetNodeIndex(int _iPos);

	//Check if given position is within the given distance of the given node
	bool IsAt(CVec3Dfp32 _Pos, int _iPos, fp32 _Distance);

	//Number of navnodes in path
	int Length();

	//Remove all nodes from path
	void Clear();
};



#endif

#endif

