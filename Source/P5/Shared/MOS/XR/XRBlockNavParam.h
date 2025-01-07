/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Auxiliary classes for search instances
					
	Author:			Magnus Högdahl
					Anders Olsson
					
	Copyright:		
					
	Contents:		CXBN_SearchParams
					CXBN_SearchResultNode
					CXBN_SearchResult
					
	Comments:		
					
	History:
		01????		Created file
		010209:		Included comments
\*____________________________________________________________________________________________*/

#ifndef _INC_WBLOCKNAVPARAM
#define _INC_WBLOCKNAVPARAM

// #include "WMapData.h"
#include "XRBlockNavGrid_Shared.h"

class CXBN_SearchNode;
class CXBN_SearchInstance;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXBN_SearchParams
|__________________________________________________________________________________________________
\*************************************************************************************************/
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Search parameters
						
	Comments:			Container class for any parameters of a search
						instance (CXBN_SearchInstance). The public 
						values are set by the user. The privates are 
						set by the search instance when the search is 
						created.
\*____________________________________________________________________*/
class M_ALIGN(16) CXBN_SearchParams
{
public:
	//These first values will be set by the search instance when the search is created
	
	//The object ID of the user (used for identification purposes only)
	int m_iUser;

	//The navigation grid coordinates
	CVec3Dint16 m_Src;
	CVec3Dint16 m_Dst;


	//The "square radius" of the entitys base size in cells. If the entitys base fits into one cell this is 0. 
	//If it fits into 2 or 3 cells it's 1 and so on...
	int16 m_iBaseSize;

	//The height in cells when as small as possible. 
	int16 m_iHeight;

	//The full height of the entity in cells
	int16 m_iFullHeight;

	//The search instance sets the above values, so it needs to be able to access them
	friend class CXBN_SearchInstance;

	//These last values are set by the initializer

	//The world space coordinates
	CVec3Dfp32 m_SrcPos;
	CVec3Dfp32 m_DstPos;
	bool m_bPartial;		// The search allows partials ie we don't require m_DstPos to be valid
	bool m_bWallClimb;
	bool m_bJump;			// When true we may jump down ledges etc
	CVec3Dint16 m_SizeWU;	//The dimensions of the entitys bounding box when as small as possible (i.e. crouching) in world units
	int16 m_iFullHeightWU;	//The full height of the entity in world units

	//The search result must have access to some of these protected values
	//friend class CXBN_SearchResult;
public:

	CXBN_SearchParams()
	{
		CVec3Dfp32 a(0);
		CVec3Dfp32 b(0);
		CVec3Dint16 c(1);
		CXBN_SearchParams(a, b, c, 1);
	}

	CXBN_SearchParams(CVec3Dfp32 _SrcPos, CVec3Dfp32 _DstPos, CVec3Dint16 _Size, int16 _FullHeight, int _iUser = 0)
	{
		m_iUser = _iUser;
		m_SrcPos = _SrcPos;
		m_DstPos = _DstPos;
		m_SizeWU = _Size;
		m_iFullHeightWU = _FullHeight;
		m_bPartial = false;
		m_bWallClimb = false;
		m_bJump = false;
	}
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXBN_SearchResultNode			
|__________________________________________________________________________________________________
\*************************************************************************************************/
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Info container for search result node
\*____________________________________________________________________*/

//Holds data about a node in the found search path
class M_ALIGN(16) CXBN_SearchResultNode //: public CReferenceCount			
{
protected:
	//The world position of the node
	CVec3Dfp32 m_Pos;

	//The grid position of the node, only used for post-processing
	CVec3Dint16 m_GridPos;

	//State flags
	int32 m_Flags;
	
	//Approximate movement direction in xy-plane at node. The values can not be 0 at the same time.
	int m_MoveX : 2; //-1..1
	int m_MoveY : 2; //-1..1

	//Search result class must be able to set these
	friend class CXBN_SearchResultCore;
	friend class CXBN_SearchResult;

public:
	
	//State flag constants
	enum 
	{
		AIRBORNE		= 0x1,
		JUMP			= 0x2,
	    CROUCH			= 0x4,

		INTERMEDIATE	= 0x8,
		STOP_			= 0x10, // Stop is a bad name, and should not be used
		
		RIGHTTURN		= 0x20,
		LEFTTURN 		= 0x40,
		
		RIGHTEDGE		= 0x80,
		LEFTEDGE		= 0x100,
		FRONTEDGE		= 0x200,
		BACKEDGE		= 0x400,
		
		RIGHTWALL		= 0x800,
		LEFTWALL		= 0x1000,
		FRONTWALL		= 0x2000,
		BACKWALL		= 0x4000,

		RIGHTDANGER		= 0x80000,
		LEFTDANGER		= 0x10000,
		FRONTDANGER		= 0x20000,
		BACKDANGER		= 0x40000,
	};

	//Access to privates...
	CVec3Dint16 GetGridPosition(){return m_GridPos;};
	CVec3Dfp32 GetPosition(){return m_Pos;};
	int GetFlags(){return m_Flags;};
	void SetFlags(int32 _Flags){m_Flags = _Flags;};

	int GetMoveX(){return m_MoveX;};
	int GetMoveY(){return m_MoveY;};

	CXBN_SearchResultNode(){};

	//Constructs a result node from a search node
	CXBN_SearchResultNode(CXBN_SearchNode* _Node, fp32 _NodeSize, CVec3Dfp32 _Offset, int _Flags);

};

//typedef TPtr<CXBN_SearchResultNode> spCXBN_SearchResultNode;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXBN_SearchResult			
|__________________________________________________________________________________________________
\*************************************************************************************************/
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Info container for search result
						
	Comments:			Holds info about a search result and provides 
						suitable methods to retrieve it. The search 
						result is created by the user, but filled with 
						data by the search instance when the 
						CXBN_SearchInstance::Search_Get method is 
						called.
\*____________________________________________________________________*/

//#ifndef PLATFORM_VPU

class CXBN_SearchResultCore : public CReferenceCount
{
protected:
	//The size of a node is considered to be the the side length of a navigation grid cell
	fp32 m_NodeSize;

	//Grid cost of entire path
	uint32 m_Cost;

	//Length up until last safely traversed node
	int m_SafeLength;

	//The search result
#ifdef PLATFORM_VPU
	TThinArray_VPU<CXBN_SearchResultNode,128> m_lNodes;
#else
	TArray<CXBN_SearchResultNode> m_lNodes;
#endif

	//Counts how far the post processing have proceeded. As the post processing makes
	//three passes over the nodes, this value is in the interval 0..3*<Number of nodes>-1
	int m_iPostProcessCount;

	//Helper to Create. Checks if we have to crouch at the given search node.
	bool NeedToCrouch(CXBN_SearchNode* _pNode, CXBN_SearchInstance* _pSearch); // STAYS

	//Helper to Create. Checks if the given node should be removed from the path.
	bool IsGarbageNode(CXBN_SearchNode* _pNode, CXBN_SearchNode* _pSuccessor); // STAYS

	//Creates a search result, given the last search node of a path and a search instance.
	//If the search result created is only a partial path, the last argument should be set to true
	//A non-created search result is only garbage.
	void Create(CXBN_SearchNode* _pEndNode, CXBN_SearchInstance* _pSearch, bool _IsPartialPath = false);

	//Search instance must be able to create search results
	friend class CXBN_SearchInstance;

public:
	void Create(CXBN_SearchResultNode* _pNodes, int _NumNodes, int _Cost, fp32 _NodeSize);

	int GetLength() { return m_lNodes.Len(); }
	int GetPostProcessCount() { return m_iPostProcessCount; }

	//Get (copy of) search result node
	CXBN_SearchResultNode GetNode(int _Index)
	{
		if ((_Index < 0) ||
			(_Index >= m_lNodes.Len()))
			//Invalid index
			return CXBN_SearchResultNode();
		
		return m_lNodes[_Index];
	};

	// Get pos of node _Index, returns true if _Index is valid
	bool GetNodePos(int _Index,CVec3Dfp32* _rpPps)
	{
		if ((_Index < 0) ||
			(_Index >= m_lNodes.Len()))
		{		
			//Invalid index
			*_rpPps = CVec3Dfp32(_FP32_MAX);
			return(false);
		}
		else
		{
			*_rpPps = m_lNodes[_Index].m_Pos;
			return(true);
		}
	};
	
	// Removes the nodes _iStart+1 to _iEnd-1
	void SkipNodes(int _iStart,int _iEnd)
	{
		if ((_iStart+1 <= _iEnd-1)&&(m_lNodes.Len() >= _iEnd))
		{
			int N = _iEnd - (_iStart+1);
			m_lNodes.Delx(_iStart+1,N);
		}
	}

	int32 LastNodeInside(int _iStart,fp32 _Range)
	{
		int32 N = m_lNodes.Len();
		if ((_iStart >= 0)&&(_iStart+1 < N))
		{
			CVec3Dfp32 StartPos = m_lNodes[_iStart].m_Pos;
			fp32 RngSqr = Sqr(_Range);
			int32 i = _iStart+1;
			while (i < N)
			{
				if (m_lNodes[i].m_Pos.DistanceSqr(StartPos) > RngSqr)
				{
					return(i-1);
				}
				i++;
			}
		}
		else
		{
			return(-1);
		}
	}

	CXBN_SearchResultCore() { Reset(); }

	// reset
	void Reset() { m_Cost = 0; m_lNodes.Clear(); m_SafeLength = 0; }

	//Get path cost
	uint32 GetCost() const { return m_Cost; }

};

#ifndef PLATFORM_VPU

//Holds the result of a search in an array of CXBN_SearchResultNodes
class CXBN_SearchResult : public CXBN_SearchResultCore
{
	MRTC_DECLARE;
protected:
	/*
	//The size of a node is considered to be the the side length of a navigation grid cell
	fp32 m_NodeSize;

	//Grid cost of entire path
	uint32 m_Cost;

	//Length up until last safely traversed node
	int m_SafeLength;

	//Counts how far the post processing have proceeded. As the post processing makes
	//three passes over the nodes, this value is in the interval 0..3*<Number of nodes>-1
	int m_iPostProcessCount;
	*/

	//Helper function to PostProcess_SetEdgeInfo; Checks if cell at given position is fall cell
	bool IsFallCell(int x, int y, int z, CXBN_SearchInstance* _pSearch);

	//Sets any right, left, front or back edge flags as appropriate for the given node
	void PostProcess_SetEdgeInfo(int _iNode, CXBN_SearchInstance* _pSearch);

	//Helper function to PostProcess_SetWallInfo; Checks if cell at given position is wall cell
	bool IsWallCell(int x, int y, int z, CXBN_SearchInstance* _pSearch);

	//Sets any right, left, front or back wall flags as appropriate for the given node
	void PostProcess_SetWallInfo(int _iNode, CXBN_SearchInstance* _pSearch);

	//Helper function to PostProcess_SetDangerInfo; Checks if cell at given position is dangerous
	bool IsDangerCell(int x, int y, int z, CXBN_SearchInstance* _pSearch);
	
	//Sets any right, left, front or back danger flags as appropriate for the given node
	void PostProcess_SetDangerInfo(int _iNode, CXBN_SearchInstance* _pSearch);
	
	//Sets any right or left turn flags as appropriate for the given node
	void PostProcess_SetTurnInfo(int _iNode, CXBN_SearchInstance* _pSearch);

	//Sets the stop flag if the given node is a stopp node.
	void PostProcess_SetStopInfo(int _iNode, CXBN_SearchInstance* _pSearch);
	
	//"Private" var for the IsIntermediateNode method
	bool m_bHasTurned;

	//Helper method to the PostProcess method. Checks if the node is an intermediate 
	//node after the node specified by _Origin.
	bool IsIntermediateNode(int _Node, int _Origin);
	
	//Adds non-vital information to the search result. The _Time argument can be
	//specified to limit the duration (in seconds) of the post-processing
	void PostProcess(CXBN_SearchInstance* _pSearch, fp64 _Time = -1.0f);

	/*
	//Helper to Create. Checks if we have to crouch at the given search node.
	bool NeedToCrouch(CXBN_SearchNode* _pNode, CXBN_SearchInstance* _pSearch);
	
	//Helper to Create. Checks if the given node should be removed from the path.
	bool IsGarbageNode(CXBN_SearchNode* _pNode, CXBN_SearchNode* _pSuccessor);

	//Creates a search result, given the last search node of a path and a search instance.
	//If the search result created is only a partial path, the last argument should be set to true
	//A non-created search result is only garbage.
	void Create(CXBN_SearchNode* _pEndNode, CXBN_SearchInstance* _pSearch, bool _IsPartialPath = false);
	*/

	//Search instance must be able to create search results
	friend class CXBN_SearchInstance;

public:
	int GetLength(){return m_lNodes.Len();};
	int GetPostProcessCount(){return m_iPostProcessCount;};

	//Get (copy of) search result node
	CXBN_SearchResultNode GetNode(int _Index)
	{
		if ((_Index < 0) ||
			(_Index >= m_lNodes.Len()))
			//Invalid index
			return CXBN_SearchResultNode();
		
		return m_lNodes[_Index];
	};

	// Get pos of node _Index, returns true if _Index is valid
	bool GetNodePos(int _Index,CVec3Dfp32* _rpPps)
	{
		if ((_Index < 0) ||
			(_Index >= m_lNodes.Len()))
		{		
			//Invalid index
			*_rpPps = CVec3Dfp32(_FP32_MAX);
			return(false);
		}
		else
		{
			*_rpPps = m_lNodes[_Index].m_Pos;
			return(true);
		}
	};
	
	// Removes the nodes _iStart+1 to _iEnd-1
	void SkipNodes(int _iStart,int _iEnd)
	{
		if ((_iStart+1 <= _iEnd-1)&&(m_lNodes.Len() >= _iEnd))
		{
			int N = _iEnd - (_iStart+1);
			m_lNodes.Delx(_iStart+1,N);
		}
	}

	int32 LastNodeInside(int _iStart,fp32 _Range)
	{
		int32 N = m_lNodes.Len();
		if ((_iStart >= 0)&&(_iStart+1 < N))
		{
			CVec3Dfp32 StartPos = m_lNodes[_iStart].m_Pos;
			fp32 RngSqr = Sqr(_Range);
			int32 i = _iStart+1;
			while (i < N)
			{
				if (m_lNodes[i].m_Pos.DistanceSqr(StartPos) > RngSqr)
				{
					return(i-1);
				}
				i++;
			}
		}
		else
		{
			return(-1);
		}
	}

	CXBN_SearchResult(){};

	//Copy content of given path into this path
	virtual void Copy(CXBN_SearchResult * _pSearchResult);

	//Is the given position at the given nodes position?
	virtual bool AtNode(int _NodeIndex, CVec3Dfp32 _Pos, CVec3Dfp32 _Up);

	//Is the given position at the given nodes position in the XY-plane?
	virtual bool AtNodeXY(int _NodeIndex, CVec3Dfp32 _Pos);

	//"Safe" length, i.e. length of path from start to first non-airborne/jump/edge/danger node
	virtual int GetSafeLength(){return m_SafeLength;};

	//Debug routine; prints all node coordinates to the specified file
	virtual void PrintToFile(CStr _FileName);
};

typedef TPtr<CXBN_SearchResult> spCXBN_SearchResult;

#endif

#endif
