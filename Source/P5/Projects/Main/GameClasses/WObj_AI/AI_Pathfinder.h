#ifndef _INC_AI_PATHFINDER
#define _INC_AI_PATHFINDER

#include "../../../../Shared/MOS/XR/XRBlockNav.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WNavGraph/WNavGraph_Pathfinder.h"
#include "AI_Auxiliary.h"



class CAI_Path;

// Max range for smoothing (to save on long IsTraversable calls)
#define AI_PATHFIND_MAXSMOOTH	160.0f
// #define AI_PATHFIND_MAXSMOOTH	256.0f

//Pathfinding handler for AI; uses the second layer navgraph combined with the navgrid for navigation
class CAI_Pathfinder
{
protected:
	//"Constant" helper array
	static int ms_i2DDirs[8][2];	
	
	//Character data in world units
	int m_BaseSize;
	int m_CrouchHeight;
	int m_FullHeight;
	int m_StepHeight;
	int m_JumpHeight;

	//Best fitting navgraph size group 
	int m_SizeGroup;

	//The navgraph pathfinder, search instance index and resource handler
	class CWorld_Navgraph_Pathfinder * m_pGraphPF;
	int m_iGraphPFInstance;
	class CAI_Resource_Pathfinding * m_pPFGraph_ResourceHandler;

	//The block grid pathfinder, search instance index and resource handler

	
	//class CXR_BlockNav *m_pBlockNav;
	class CXR_BlockNavSearcher *m_pGridPF;
	int m_iGridPFInstance;
	class CAI_Resource_Pathfinding * m_pPFGrid_ResourceHandler;

	//The using AI
	class CAI_Core * m_pAI;

	//The current navgraph path (must be smart pointer since it's defined in another dll :P)
	spCWorld_NavGraph_Path m_spGraphPath;

	//The current and next navgrid search results (must be smart pointer since it's defined in another dll :P)
	spCXBN_SearchResult m_spGridPath;

	//Do we have a straight path to destination
	bool m_bStraightPath;

	//The current origin and destination 
	CVec3Dfp32 m_Origin;
	CVec3Dfp32 m_Destination;

	//Should we get partial path when finished failed search?
	bool m_bPartialPath;

	//Priority when requesting search instance
	uint8 m_Prio;

	//Current general search status. This is either one of the pathfinding statuses or:
	enum {
		NO_SEARCH = -1,
	};
	int m_SearchStatus;

	//Grid search status. This is one of the CXR_BlockNav search statuses or one of the below.
	enum {
		SEARCH_TIMED_OUT			= -1,
		SEARCH_INSTANCE_UNAVAILABLE	= -2,
	};
	int m_GridSearchStatus;

	//Graph search status. This is one of CWNavGraph_Pathfinder search statuses
	int m_GraphSearchStatus;

	//Current combined search mode
	enum {
		MODE_FIND_STARTNODECANDIDATES = 0,
		MODE_FIND_STRAIGHTPATH_STARTNODE,
		MODE_FIND_STARTNODE,
		MODE_FIND_ENDNODECANDIDATES,
		MODE_FIND_STRAIGHTPATH_ENDNODE,
		MODE_FIND_ENDNODE,
		MODE_FIND_GRAPHPATH,
		MODE_CHECK_ALTERNATES_STRAIGHT,
		MODE_CHECK_ALTERNATES,
		MODE_TRY_NEW_NODES,
		MODE_CULL_STARTNODES,
		MODE_CULL_ENDNODES,
		MODE_GRIDSEARCH_ONLY,
	};
	int m_SearchMode;

	//Max number of nodes (grid and graph alike) that can be expanded in one go
	int m_nExpandLimit;

	//Time constraint
	CMTime m_MaxTime;

	//Index to first "straightpath-unchecked" start/end/alternate node candidate
	int m_iFirstUncheckedStartNode;
	int m_iFirstUncheckedEndNode;
	int m_iFirstUncheckedAlternate;

	//Current start and end node indices
	int16 m_iStartNode;
	int16 m_iEndNode;

	//Current start and end node candidates. Removed candidates have index -1
	TThinArray<int16> m_liStartNodes;
	TThinArray<int16> m_liEndNodes;

	//Culling index
	int m_iCurCull;

	//Do we have straight path to current start node and from current end node, respectively
	bool m_bStraightToStartNode;
	bool m_bStraightFromEndNode;

	//Alternate end nodes reached 
	TArray<int16> m_liAlternates;

	//Valid end nodes and currently used index
	TArray<int16> m_liValidEndNodes;
	int m_iCurValidEndNode;

	//Best partial graph path found with accompanying flag for direct path to first node or grid path
	spCWorld_NavGraph_Path m_spPartialGraphPath;
	bool m_bStraightPartialPath;
	spCXBN_SearchResult m_spPartialGridPath;
	
	//Current grid pathfinding origin and destination
	CVec3Dfp32 m_GridOrigin;
	CVec3Dfp32 m_GridDest;

	//If we're within a "navgraph node distance" of this position we shouldn't use graph pathfinding
	//This is used to circumvent repeatedly failed graph path following due to the fact that we don't 
	//want to invalidate edges for large characters
	CVec3Dfp32 m_GraphSkipPosition;

protected:
	//Release search instances. If specified only grid or graph instance will be released.
	enum {
		RELEASE_GRID	= 0x1,
		RELEASE_GRAPH	= 0x2,
		RELEASE_ALL		= 0x3,
	};
	void ReleaseSearch(int _iMode = RELEASE_ALL);

	//Request grid search instance and, if successful, create grid search instance with given parameters
	//and return instance index. Return 0 on failure.
	//_bPartial when true will allow destionations to be invalid positions
	int GetGridSearch(const CVec3Dfp32& _From, const CVec3Dfp32& _To, bool _bPartial, bool _bWallclimb);

	//Grid pathfinding wrapper. Initiates and executes search with the given number of node expansions. 
	//Sets grid search status. If provided with a search result, sets this to path when done (optionally
	//partial path on failure). Reduces number of node expansions appropriately and returns pathfinding status.
	int GridFindPath(const CVec3Dfp32& _From, const CVec3Dfp32& _To, int& _nNodeExpansions, const CMTime& _Time, CXBN_SearchResult * _pRes, bool _bPartial, bool _bWallClimb, int _MaxExpansions = -1);

	//Request graph search instance and, if successful, create graph search instance with given parameters
	//and return search instance index. Returns -1 on failure.
	int GetGraphSearch(int _iFrom, int _iTo, int _iSize = CXR_NavGraph::SIZE_DEFAULT, const int16* _lAlternates = NULL, int _nAlternates = 0, int _MaxCost = -1);

	//Use grid pathfinding only
	int GridSearchOnly(int& _nNodeExpansions, const CMTime& _Time);

	//Find start node candidates.
	int FindStartNodeCandidates(int& _nNodeExpansions, const CMTime& _Time); 	

	//Find first start node that can be reached by straight path from origin
	int FindStraightPathStartNode(int& _nNodeExpansions, const CMTime& _Time); 	

	//Find the first start node candidate that can be reached from origin by grid pathfinding in one go
	int FindStartNode(int& _nNodeExpansions, const CMTime& _Time);

	//Find end node candidates
	int FindEndNodeCandidates(int& _nNodeExpansions, const CMTime& _Time); 	

	//Find first end node from which destination can be reached by straight path
	int FindStraightPathEndNode(int& _nNodeExpansions, const CMTime& _Time); 	

	//Find the first end node candidate from which one can reach the destination by grid pathfinding in one go
	int FindEndNode(int& _nNodeExpansions, const CMTime& _Time);

	//Find path from current start node to current end node, or other end node candidates as backup
	int FindGraphPath(int& _nNodeExpansions, const CMTime& _Time);

	//Check if destination can be reached from any alternate end node by straight traversability
	int CheckAlternatesStraight(int& _nNodeExpansions, const CMTime& _Time);

	//Check if destination can be reached from any alternate end node by pathfinding
	int CheckAlternates(int& _nNodeExpansions, const CMTime& _Time);

	//Try to find path with new end- and/or startnode
	int TryNewNodes(int& _nNodeExpansions, const CMTime& _Time);

	//Remove all start nodes that can be swiftly reached from the current start node
	int CullStartNodes(int& _nNodeExpansions, const CMTime& _Time);

	//Remove all end nodes that can be swiftly reached from the current end node
	int CullEndNodes(int& _nNodeExpansions, const CMTime& _Time);

	friend class CAI_Path; //DEBUG purposes only, remove

public:

	//Constructor
	CAI_Pathfinder();
	
	//Switch AI user
	void ReInit(CAI_Core * _pAI);

	//Set up pathfinder for searching (_MaxTime is in microseconds)
	void Init(CAI_Core * _pAI, CWorld_Navgraph_Pathfinder * _pGraphPathfinder, CXR_BlockNavSearcher * _pGridPathfinder, CAI_Resource_Pathfinding * _pPFGraphResourceHandler, CAI_Resource_Pathfinding * _pPFGridResourceHandler, int _BaseSize, int _CrouchHeight, int _FullHeight, int _StepHeight, int _JumpHeight, int _nExpandLimit, fp32 _MaxTime);

	//Reset pathfinder for new search. Optionally we can save origin and destination
	void Reset(bool _bKeepPositions = false);

	//Set graph skip position when we want character to use grid pathfinding only 
	//until he's moved away from the given position
	void SetGraphSkipPosition(const CVec3Dfp32& _Pos);

	//Can pathfinder be used?
	bool IsValid();
	
	CVec3Dfp32& GetOrigin() { return(m_Origin);}
	CVec3Dfp32& GetDestination() { return(m_Destination);}

	//Find path from one position to another returns one the below results. Once a path has been 
	//found it can be used via other methods below. The given priority is used when requesting search 
	//instances. Returns pathfinding status:
	enum { 
		NO_PATH = 0, //Couldn't find path (although there might be a partial one)
		IN_PROGRESS, //Still searching
		PAUSED,		 //Waiting for resource to get available	
		FOUND_PATH,	 //Successfully completed
		INVALID,	 //There's something rotten in the kingdom of Denmark...	
	};
	int FindPath(const CVec3Dfp32& _From, const CVec3Dfp32& _To, uint8 _Priority, bool _bPartial, bool _bWallClimb);

	//Find path from current origin to destination, with current priority. 
	//Once a path has been found it can be used via other methods below.
	int FindPath();

	//Finds path as above, but always restarts pathfinding
	int FindNewPath(const CVec3Dfp32& _From, const CVec3Dfp32& _To, uint8 _Priority, bool _bPartial, bool _bWallClimb);
	
	// Returns the origin of the current or last pathfind
	CVec3Dfp32 GetPathfindOrigin();

	//Set supplied path object to the found path. Succeeds if search is done, fails otherwise (but will set
	//partial path if seacrh was specified as such),
	bool GetPath(CAI_Path * _pPath);	

	//Check if we can make a simple ground move in a straight line from one position to another.
	//If a partial position pointer is supplied, this will hold the farthest position along the 
	//line to the destination that can be reached by a straight line ground move.
	bool GroundTraversable(const CVec3Dfp32& _From, const CVec3Dfp32& _To, CVec3Dfp32* _pPartial = NULL);

	//Check if we must crouch when moving from the given position in the given direction (assuming the position is traversible)
	bool MustCrouch(const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir);

	//Returns the closest position above or below the given position which is a traversable ground cell, 
	//or fails with CVec3Dfp32(_FP32_MAX). If the optional _iMaxDiff argument is greater than -1, then this is 
	//the maximum height-difference in cells tolerated. If the ordinary algorithm fails and the optional 
	//_iRadius argument is greater than 0, it will continue to check _iRadius cell-columns in eight directions 
	//outward from the given position.
	CVec3Dfp32 GetPathPosition(const CVec3Dfp32& _Pos, int _iMaxDiff, int _iRadius = 0);

	//Gets traversable ground position (according to the navigation grid) of the given object. 
	//This assumes the object is on the ground, unless the _bAirborne argument is set to true.
	CVec3Dfp32 GetPathPosition(CWObject * _pObj, bool _bAirborne = false);

	//Gets traversable ground position within the given radius and height, or returns the given position if this fails.
	CVec3Dfp32 SafeGetPathPosition(const CVec3Dfp32& _Pos, int _iMaxDiff, int _iRadius = 0);

	//Attempts to find a ground-traversable position (i.e. one that can be reached by travelling in a straight line 
	//along the ground) in the given heading at the given distance. If this fails, it will try to find the closest 
	//position at that distance or closer, trying new heading with the given angle interval (90 degrees as default). 
	//If the _angle argument is specified it will only try to find positions within the arc of that angle, centered 
	//around the heading and if the _MinDist argument is given it will only try to find closer positions down to 
	//this minimum distance. Returns CVec3Dfp32(_FP32_MAX) if no position is found. If the nGroundCells argument is specified 
	//the position must also have at least that many cround cells.
	CVec3Dfp32 FindGroundPosition(CWObject * _pObject, fp32 _Heading, fp32 _Distance, fp32 _MinDist = 0.0f, fp32 _Interval = 0.25f, fp32 _Angle = 1.0f, int nGroundCells = 1);

	//Handle messages
	aint OnMessage(const CWObject_Message& _Msg);

	bool IsOnGround(const CVec3Dfp32& _Pos);

	//Grid search wrappers for use externally
	int GridSearch_Create(const CVec3Dfp32& _From, const CVec3Dfp32& _To);
	int GridSearch_Execute(int _iSearch, int& _nNodeExpansions, int _nMaxExpansions = -1);
	bool GridSearch_Get(int _iSearch, CXBN_SearchResult* _pRes, bool _ReturnPartial = false);
	void GridSearch_Release(int _iSearch);

	//Destroy any unsanctioned searches
	void CleanGridPFResources();
	void CleanGraphPFResources();

	//Some private accessors
	CWorld_Navgraph_Pathfinder * GraphPF();
	CXR_BlockNavSearcher* GridPF();
	class CXR_BlockNav *GetBlockNav() { return m_pGridPF?m_pGridPF->GetBlockNav():0; }
	int ExpansionLimit();
	CMTime TimeLimit();
	int GetGraphSizeGroup();

	// 
	enum
	{
		FRONT = 0,
		RIGHT = 1,
		BACK = 2,
		LEFT = 3,
		RIGHT_45 = 4,
		LEFT_45 = 5, 
	};

	// Returns true if one can straightline move _pRange units in _Dir direction, _pPos is the final position
	// If false _pRange holds traversable range and _pPos holds final position
	bool GroundTraversableDirection(int _Dir, fp32* _pRange, CVec3Dfp32* _pPos = NULL);
	// Helper for GroundTraversableDirection() above, returns one of the FRONT,RIGHT,BACK,LEFT
	int GetDir4(CVec3Dfp32 _Pos);
	// Helper for GroundTraversableDirection() above, returns one of the FRONT,RIGHT,BACK,LEFT,LEFT_45,RIGHT_45
	int GetDir6(CVec3Dfp32 _Pos);
	// Returns movestick for a particular movedir
	CVec3Dfp32 GetMove6(int32 _Dir);

	//Debug render (graph) search
	void Debug_Render(CWorld_Server * _pServer);

	CStr m_DebugInfo;//DEBUG
};


//Path handler; provides functionality for using pathfinding result
class CAI_Path
{
protected:
	//Constants
	static const fp32 ms_ATSUBDESTDISTSQR;
	static const fp32 ms_ATSWITCHDESTDISTSQR;
	static const fp32 ms_ATSWITCHDESTHALFDIST;

	//The pathfinder that generated the path
	CAI_Pathfinder* m_pPathfinder;

	int m_iCall;	// Call nbr since new path

	//Final destination
	CVec3Dfp32 m_Destination;

	//Is there a direct path to current destination?
	bool m_bDirectPath;
		
	//The navgraph path
	spCWorld_NavGraph_Path m_spGraphPath;

	//The current and next blocknav grid path
	spCXBN_SearchResult m_spCurGridPath;
	spCXBN_SearchResult m_spNextGridPath;

	//Current and next graph path indices
	int m_iCurGraphPos;
	int m_iNextGraphPos;

	//Current node index in grid path
	int m_iGridPos;

	//Grid search instance
	int m_iGridPFInstance;

	//The node position we're currently moving towards and the position of the node to move towards afterwards
	CVec3Dfp32 m_SubDest;
	CVec3Dfp32 m_NextSubDest;

	//Is there direct path from current sub destination to next?
	bool m_bNextDirectPath;

	//Current search status; either one of CXR_BlockNav search statuse or:
	enum {
		NO_SEARCH = -1,	
	};
	int m_SearchStatus;

	//Are we switching path
	// ***
public:
	// ***
	bool m_bSwitchingPath;
	int m_DebugReason;
	enum
	{
		NORMAL_PATH = 0,
		DIRECT_PATH,
		DIRECTNEXT_PATH,
		SWITCHING_PATH,
		SWITCHINGSTEPS_PATH,
		SWITCHINGACROSS_PATH,
	};
	// ***
protected:
	// ***

	//Destination of path switch move
	CVec3Dfp32 m_SwitchDest;

	//Previous action mode and flags
	int m_PrevActionMode;
	int m_PrevActionFlags;

	//Free used resources when done searching
	void ReleaseGridSearch();

	//Squared distance between position, except that z-distance is reduced by half a cell size
	fp32 GridNodeDistSqr(const CVec3Dfp32& _Pos1, const CVec3Dfp32& _Pos2);

	//Iterates through given grid path, starting at given index until a non-airborne node is 
	//found or end is reached. Also modifies the supplied action flags as appropriate.
	int GetLandingNode(CXBN_SearchResult * _pPath, int _iPos, int& _ActionFlags);

public:
	//Constructor
	CAI_Path();

	//Clear path
	void Reset();

	//Build path from pathfinder data. If graph path is valid, then this is assumed to be a combined 
	//path where the given grid path is path to first node and the direct path flag indicates whether 
	//first node can be reached by a straight line ground move. The _Dest parameter is final destination.
	void Build(CAI_Pathfinder * _pPF, const CVec3Dfp32& _Dest, CWorld_NavGraph_Path * _pGraphPath, CXBN_SearchResult * _pStartGridPath, bool _bStartDirect = false);
	void Build(CAI_Pathfinder * _pPF, const CVec3Dfp32& _Dest, CXBN_SearchResult * _pGridPath);
	void Build(CAI_Pathfinder * _pPF, const CVec3Dfp32& _Dest);

	//Check if path is valid to follow
	bool IsValid();

	//Get current world position to go to, based on position we're at and set provided action mode
	//(what we should do right now) and action flags (modifier to action mode) appropriately. If we 
	//are at end of path, the provided bDone flag is set true, otherwise false. The node radius 
	//parameter defines the max distance from a graph node that one considers to be "at" the node.
	enum {
		//Action modes
		NORMAL = 0,
		AIRBORNE,
		/*JUMP,*/
		STOP,
		CAREFUL,
		PAUSE	
	};
	enum {
		//Action flags
		NONE		= 0, 
		CROUCH		= 0x1,
	};

	CVec3Dfp32 GetCurDestination(const CVec3Dfp32& _Pos, int& ActionMode, int& _ActionFlags, bool& _bDone, fp32 _StepLength, fp32 _NodeRadius = 128.0f);
	// Forwards the path to the next node that is closest to _Pos
	// _bFromStart Search from the very start of the path instead of where we are at
	// _MaxLength < _FP32_MAX: we return the closest pos in that range
	// _MaxLength == _FP32_MAX: We return thr first pos that is no longer closing on us (does that even make sense?)
	bool FindNextClosestNode(const CVec3Dfp32& _Pos, bool _bFromStart, fp32 _MaxLength = _FP32_MAX);
	// Propel path at least _Length units and returns the new position
	// Returns an invalid pos if it coludn't propel the path this far
	CVec3Dfp32 PropelPath(const CVec3Dfp32& _Pos, fp32 _Length, bool _bMove = true);
	// This code steps ahead until a node at least _Length away is found.
	// It then checks for traversability between his current pos and there and returns true if it IS traversable
	CVec3Dfp32 PropelPathToFirstNodeOutsideRange(CVec3Dfp32& _Pos, fp32 _Range, bool _bMove = true);
	// This code steps ahead until a node at least _Length away is found.
	// It then checks for traversability between his current pos and there and returns true if it IS traversable 
	bool CheckShortcut(fp32 _Length, CVec3Dfp32* _pSubDest);
	//Handle messages
	aint OnMessage(const CWObject_Message& _Msg);

	//Debug render graph path, current grid path and next grid path, given position of path user and server to debug render
	void Debug_Render(const CVec3Dfp32& _Pos, CWorld_Server * _pServer);

	CStr m_DebugInfo;//DEBUG
};



#endif

