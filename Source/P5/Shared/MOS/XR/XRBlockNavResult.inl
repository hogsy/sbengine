
#ifndef BLOCKNAV_VPU_COMPILE
	#include "MFloat.h"
#endif

#include "XRBlockNav.h"
#include "XRBlockNavInst.h"


// #pragma  optimize("",off)/
// #pragma inline_depth(0)

//Constants
//Used by CXBN_SearchResult::IsIntermediateNode
const int NO_INTERMEDIATENODES = 3;



/////CXBN_SearchResultNode/////

CXBN_SearchResultNode::CXBN_SearchResultNode(CXBN_SearchNode* _Node, fp32 _NodeSize, CVec3Dfp32 _Offset, int _Flags)
{
	MAUTOSTRIP(CXBN_SearchResultNode_ctor, MAUTOSTRIP_VOID);
	//Set position to the corresponding world position of the search node's grid position
	m_Pos[0] = (((fp32)_Node->m_Pos[0]) * _NodeSize) + _Offset[0] + (_NodeSize / 2);
	m_Pos[1] = (((fp32)_Node->m_Pos[1]) * _NodeSize) + _Offset[1] + (_NodeSize / 2);
	m_Pos[2] = (((fp32)_Node->m_Pos[2]) * _NodeSize) + _Offset[2] + (_NodeSize / 2);

	//Set grid position
	m_GridPos = _Node->m_Pos;

	//Set flags 
	m_Flags = _Flags;
	//Set other values to default
	m_MoveX = 1;
	m_MoveY = 0;
};

/////CXBN_SearchResultCore/////

void CXBN_SearchResultCore::Create(CXBN_SearchResultNode* _pNodes, int _NumNodes, int _Cost, fp32 _NodeSize)
{
	MAUTOSTRIP(CXBN_SearchResult_Create1, MAUTOSTRIP_VOID);
	Reset();

	m_NodeSize = _NodeSize;
	m_iPostProcessCount = 0;
	m_Cost = _Cost;
	m_lNodes.SetLen(_NumNodes);
	m_SafeLength = _NumNodes;
	for(int i = 0; i < _NumNodes; i++)
		m_lNodes[i] = _pNodes[i];
}

void CXBN_SearchResultCore::Create(CXBN_SearchNode* _pEndNode, CXBN_SearchInstance* _pSearch, bool _IsPartialPath)
{
	MAUTOSTRIP(CXBN_SearchResult_Create2, MAUTOSTRIP_VOID);
	Reset();
		
	//If we get an empty path then quit 
	if (!_pEndNode)
		return;

	//Remove any airborne or parent-jump nodes from the end of the path if it's a partial 
	//path. No path should end with a jump or an airborne node unless the actual destination 
	//is in mid-air.
	CXBN_SearchNode * pEndNode = _pEndNode;
	if (_IsPartialPath)
	{
		while (((pEndNode->m_Flags & WBN_NODE_AIRBORNE) || (pEndNode->m_Flags & WBN_PARENT_NODE_JUMP)) &&
				//Never remove starting node
				pEndNode->m_pParent)
		{
			pEndNode = pEndNode->m_pParent;	
		};
	};
	
	//Remove garbage nodes from the path (start and end nodes are never considered garbage)
	//and check length of edited path while at it.
	CXBN_SearchNode* pNode = pEndNode;
	int32 i = 1;
	while (pNode->m_pParent)
	{
		//Should parent of current node be removed?
		while (IsGarbageNode(pNode->m_pParent, pNode))
			//Parent node is garbage, remove it.
			pNode->m_pParent = pNode->m_pParent->m_pParent; 

		pNode = pNode->m_pParent;	
		i++;
	};

	//Set node size and cost
	m_NodeSize = _pSearch->m_pGrid->m_UnitsPerCell;
	m_Cost = _pEndNode->m_Cost;

	//Get offset
	CVec3Dfp32 Offset = _pSearch->m_pGrid->m_Origin;

	//Set up and fill the list with the path, starting from the back
	pNode = pEndNode;
	m_lNodes.SetLen(i);
	int Flags = 0;
	i--;
	for (; i >= 0; i--)
	{
		//Do we have to crouch at node?
		if (NeedToCrouch(pNode, _pSearch))
			Flags |= CXBN_SearchResultNode::CROUCH;

		//Are we airborne at node? 
		if (pNode->m_Flags & WBN_NODE_AIRBORNE) 
			Flags |= CXBN_SearchResultNode::AIRBORNE;

		//Add node
		m_lNodes[i] = CXBN_SearchResultNode(pNode, m_NodeSize, Offset, Flags);

		//Reset flags
		Flags = 0;

		//Should we jump at parent node?
		if (pNode->m_Flags & WBN_PARENT_NODE_JUMP) 
			Flags |= CXBN_SearchResultNode::JUMP;

		pNode = pNode->m_pParent;
	};

	//First and last nodes probably have slightly different positions, 
	//as exactly specified by the search parameters.
	m_lNodes[0].m_Pos = _pSearch->m_Params.m_SrcPos;
	//We might not have reached the real destination
	if (_pSearch->m_pDstNode)
		m_lNodes[m_lNodes.Len() - 1].m_Pos = _pSearch->m_Params.m_DstPos;

	//Postprocessing; get extra info about path to use when following path.
	//This part should perhaps be made a separate method later so that 
	//the user can choose when and how much postprocessing he wants to do.
	//For now it's a helper method.
	m_iPostProcessCount = 0;

	//PostProcess(_pSearch);
};


//Helper to Create. Checks if we have to crouch at the given search node.
bool CXBN_SearchResultCore::NeedToCrouch(CXBN_SearchNode* _pNode, CXBN_SearchInstance* _pSearch)
{
	MAUTOSTRIP(CXBN_SearchResult_NeedToCrouch, false);
	//Check if there is enough traversable space above the node to stand up

	//The position just on top of the already checked space of the node	
	CVec3Dint16 Pos = _pNode->m_Pos + CVec3Dint16(0, 0, _pSearch->m_Params.m_iHeight);
	
	//We're at the grid ceiling  (Can this ever happen? /mh If map is built funnily, but hopefully never :) /ao)
	if (_pNode->m_Pos[2] > _pSearch->m_pGrid->m_CellGridDim[2] - _pSearch->m_Params.m_iFullHeight)
		return true;

	for(int x = Pos[0] - _pSearch->m_Params.m_iBaseSize; x <= Pos[0] + _pSearch->m_Params.m_iBaseSize; x++)
		for(int y = Pos[1] - _pSearch->m_Params.m_iBaseSize; y <= Pos[1] + _pSearch->m_Params.m_iBaseSize; y++)
			for(int z = Pos[2]; z < Pos[2] + _pSearch->m_Params.m_iFullHeight - _pSearch->m_Params.m_iHeight; z++)
			{
				int Value = _pSearch->GetCellValue(x, y, z);
				if (!(Value & XR_CELL_AIR) || (Value & XR_CELL_DIE)) 
					//Not enough space, we need to crouch
					return true;
			};
	//Enough space to stand up straight
	return false;
};



//Helper to Create. Checks if the given node's parent should be removed from the path.
bool CXBN_SearchResultCore::IsGarbageNode(CXBN_SearchNode* _pNode, CXBN_SearchNode* _pSuccessor)
{
	MAUTOSTRIP(CXBN_SearchResult_IsGarbageNode, false);
	//A garbage node is in general a node which is completely unnecessary to
	//keep in the path, and one that might well cause unwanted difficulties
	//with movement.
	//I'll have to expand the definition of what should count as a garbage node
	//as I find more special cases, but these are the ones so far:
	//  Airborne nodes in stairs and shallow slopes
	
	//Start node is never considered garbage
	if (!_pNode->m_pParent)
		return false;

	//End Node is never considered garbage
	if (!_pSuccessor)
		return false;

	//Nodes which come diectly after a jump ara never considered garbage
	if (_pNode->m_Flags & WBN_PARENT_NODE_JUMP)
		return false;

	//Is the node an airborne node in a stair or slope where we don't jump?
	if (//Is the node an isolated airborne node?
		(_pNode->m_Flags & WBN_NODE_AIRBORNE) && 
		!(_pSuccessor->m_Flags & WBN_NODE_AIRBORNE) &&
		!(_pNode->m_pParent->m_Flags & WBN_NODE_AIRBORNE))
		return true;

	//Not a garbage node
	return false;
};



/////CXBN_SearchResult/////

#ifndef BLOCKNAV_VPU_COMPILE
	MRTC_IMPLEMENT_DYNAMIC(CXBN_SearchResult, CReferenceCount)

//Helper function to PostProcess_SetEdgeInfo; Checks if cell at given position is fall cell
bool CXBN_SearchResult::IsFallCell(int x, int y, int z, CXBN_SearchInstance* _pSearch)
{
	MAUTOSTRIP(CXBN_SearchResult_IsFallCell, false);
	//Checks up to three cells below current cell. If we have only air cells or
	//0..2 air cells followed by a slip/die cell, then this is a fall cell.

	if (_pSearch->IsOutsideGridBoundaries(CVec3Dint16(x,y,z)))
		//Cells outside the grid is considered fall cells by default.
		return true;
	
	int CellBelow = _pSearch->GetCellValue(x, y, z - 1);
	
	if ((CellBelow & XR_CELL_SLIP) || (CellBelow & XR_CELL_DIE))
		//Slip/die cell below i.e. fall cell
		return true;
	if (CellBelow & XR_CELL_AIR)
	{
		//Air cell below, check one cell further below
		CellBelow = _pSearch->GetCellValue(x, y, z - 2);
		if ((CellBelow & XR_CELL_SLIP) || (CellBelow & XR_CELL_DIE))
			//Slip/die cell below air cell, i.e. fall cell
			return true;

		if (CellBelow & XR_CELL_AIR)
		{
			//Air cell below, check one cell further below
			if (_pSearch->IsOutsideGridBoundaries(CVec3Dint16(x,y,z-1)))
				return true;
			CellBelow = _pSearch->GetCellValue(x, y, z - 3);
			if ((CellBelow & XR_CELL_AIR) || (CellBelow & XR_CELL_SLIP) || (CellBelow & XR_CELL_DIE))
				//Slip/die/air cell below two air cells, i.e. fall cell
				return true;
		} 

	} 

	//Not a fall cell
	return false;
};


//The movement direction when rotating the given movement direction 90 degrees clockwise.
//The first two indices are the x- and y movement direction + 1 (i.e. in the interval 0..2
//instead of -1..1), and the last the x (0) or y (1) values of the rotated movement direction.
int ms_RotateClockwise[3][3][2] =
{
	//MoveX = -1, MoveY = -1..1
	{{-1,1}, {0,1}, {1,1}},
    //MoveX = 0, MoveY = -1, 1 ((0,0) should be undefined, but is treated as (1,0))
	{{-1,0}, {0,-1}, {1,0}},
	//MoveX = 1, MoveY = -1..1
	{{-1,-1}, {0,-1}, {1,-1}}
};


//Sets any right, left, front or back edge flags as appropriate for the given node
void CXBN_SearchResult::PostProcess_SetEdgeInfo(int _iNode, CXBN_SearchInstance* _pSearch)
{
	MAUTOSTRIP(CXBN_SearchResult_PostProcess_SetEdgeInfo, MAUTOSTRIP_VOID);
	//When airborne, there are no edges of course
	if (m_lNodes[_iNode].m_Flags & CXBN_SearchResultNode::AIRBORNE)
		return;
	
	//Temps
	int x, y;
	int mx = m_lNodes[_iNode].m_MoveX;
	int my = m_lNodes[_iNode].m_MoveY;

	//Checks the four cells at the front, right, left and back center the entitys base
	//for any fall cells. (i.e. cells with no ground for at least 2 cells down, slipcells or diecells)
	//In the below examples the basesize = 1, F,R,L and B denotes the cell to be checked to the 
	//front, right, left and back, respectively. X is the center of the base. The numbers in 
	//parenthesis denote the (MoveX,MoveY) values for the example.
	//xy-plane:
	//			L F          L
	//	  (1,1): X	 (1,0): BXF
	//			B R          R

	//Check front
	x = m_lNodes[_iNode].m_GridPos[0] + mx * _pSearch->m_Params.m_iBaseSize; 
	y = m_lNodes[_iNode].m_GridPos[1] + my * _pSearch->m_Params.m_iBaseSize; 
	if (IsFallCell(x, y, m_lNodes[_iNode].m_GridPos[2], _pSearch))
		m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::FRONTEDGE;

	//Check back
	x = m_lNodes[_iNode].m_GridPos[0] - mx * _pSearch->m_Params.m_iBaseSize; 
	y = m_lNodes[_iNode].m_GridPos[1] - my * _pSearch->m_Params.m_iBaseSize; 
	if (IsFallCell(x, y, m_lNodes[_iNode].m_GridPos[2], _pSearch))
		m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::BACKEDGE;
	
	//Check right
	x = m_lNodes[_iNode].m_GridPos[0] + ms_RotateClockwise[mx+1][my+1][0] * _pSearch->m_Params.m_iBaseSize; 
	y = m_lNodes[_iNode].m_GridPos[1] + ms_RotateClockwise[mx+1][my+1][1] * _pSearch->m_Params.m_iBaseSize; 
	if (IsFallCell(x, y, m_lNodes[_iNode].m_GridPos[2], _pSearch))
		m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::RIGHTEDGE;
	
	//Check left
	x = m_lNodes[_iNode].m_GridPos[0] - ms_RotateClockwise[mx+1][my+1][0] * _pSearch->m_Params.m_iBaseSize; 
	y = m_lNodes[_iNode].m_GridPos[1] - ms_RotateClockwise[mx+1][my+1][1] * _pSearch->m_Params.m_iBaseSize; 
	if (IsFallCell(x, y, m_lNodes[_iNode].m_GridPos[2], _pSearch))
		m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::LEFTEDGE;
};
	

//Helper function to PostProcess_SetWallInfo; Checks if cell at given position is wall cell
//(i.e. if there are any solids at between stepheight and crouchheight cells above the cell)
bool CXBN_SearchResult::IsWallCell(int x, int y, int z, CXBN_SearchInstance* _pSearch)
{
	MAUTOSTRIP(CXBN_SearchResult_IsWallCell, false);
	//Anything outside the grid cannot be a wall
	if (_pSearch->IsOutsideGridBoundaries(CVec3Dint16(x,y,z)) ||
	    _pSearch->IsOutsideGridBoundaries(CVec3Dint16(x,y,z + _pSearch->m_Params.m_iHeight - 1)))
		return false;
	
	for (int i=2; i < _pSearch->m_Params.m_iHeight; i++)
		if(!(_pSearch->GetCellValue(x, y, z + i) & XR_CELL_AIR))
			//Found solid!
			return true;

	//No solids found
	return false;
};


//Sets any right, left, front or back wall flags as appropriate for the given node
void CXBN_SearchResult::PostProcess_SetWallInfo(int _iNode, CXBN_SearchInstance* _pSearch)
{
	MAUTOSTRIP(CXBN_SearchResult_PostProcess_SetWallInfo, MAUTOSTRIP_VOID);
	//Temps/Iterators
	int x, y, i;
	int bs = _pSearch->m_Params.m_iBaseSize + 1;
	int mx = m_lNodes[_iNode].m_MoveX;
	int my = m_lNodes[_iNode].m_MoveY;

	//Checks the cells around the node for any wall cells.
	//In the below examples the basesize = 1, F,R,L and B denotes the cells to be checked to the 
	//front, right, left and back, respectively. X is the center of the base. The numbers in 
	//parenthesis denote the (MoveX,MoveY) values for the example.
	//xy-plane:
	//			LL FF         LLL   
	//			L	F		 B	 F
	//	  (1,1):  X	  (1,0): B X F
	//			B	R		 B	 F
	//			BB RR         RRR
	
	//Check front
	x = m_lNodes[_iNode].m_GridPos[0] + mx * bs; 
	y = m_lNodes[_iNode].m_GridPos[1] + my * bs; 
	if (IsWallCell(x, y, m_lNodes[_iNode].m_GridPos[2], _pSearch))
		m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::FRONTWALL;
	else
		if ((mx != 0) || (mx != 0))
		{
			//Movement direction parallell to a coordinate axis
			for (i = 1; i < bs; i++)
			{
				if (IsWallCell(x + i * my, y + i * mx, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::FRONTWALL;
				if (IsWallCell(x - i * my, y - i * mx, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::FRONTWALL;
			};
		} else
		{
			//Movement direction diagonal over coordinate axes
			for (i = 1; i < bs; i++)
			{
				if (IsWallCell(x, y - i * my, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::FRONTWALL;
				if (IsWallCell(x - i * mx, y, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::FRONTWALL;
			};
		};
	
	//Check back
	x = m_lNodes[_iNode].m_GridPos[0] - mx * bs; 
	y = m_lNodes[_iNode].m_GridPos[1] - my * bs; 
	if (IsWallCell(x, y, m_lNodes[_iNode].m_GridPos[2], _pSearch))
		m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::BACKWALL;
	else
		if ((mx != 0) || (mx != 0))
		{
			//Movement direction parallell to a coordinate axis
			for (i = 1; i < bs; i++)
			{
				if (IsWallCell(x + i * my, y + i * mx, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::BACKWALL;
				if (IsWallCell(x - i * my, y - i * mx, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::BACKWALL;
			};
		} else
		{
			//Movement direction diagonal over coordinate axes
			for (i = 1; i < bs; i++)
			{
				if (IsWallCell(x, y + i * my, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::BACKWALL;
				if (IsWallCell(x + i * mx, y, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::BACKWALL;
			};
		};			

	//Rotate mx and my
	x = mx;
    mx = ms_RotateClockwise[mx+1][my+1][0];
	my = ms_RotateClockwise[x+1][my+1][1];

	//Check right
	x = m_lNodes[_iNode].m_GridPos[0] + mx * bs; 
	y = m_lNodes[_iNode].m_GridPos[1] + my * bs; 
	if (IsWallCell(x, y, m_lNodes[_iNode].m_GridPos[2], _pSearch))
		m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::RIGHTWALL;
	else
		if ((mx != 0) || (mx != 0))
		{
			//Movement direction parallell to a coordinate axis
			for (i = 1; i < bs; i++)
			{
				if (IsWallCell(x + i * my, y + i * mx, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::RIGHTWALL;
				if (IsWallCell(x - i * my, y - i * mx, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::RIGHTWALL;
			};
		} else
		{
			//Movement direction diagonal over coordinate axes
			for (i = 1; i < bs; i++)
			{
				if (IsWallCell(x, y - i * my, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::RIGHTWALL;
				if (IsWallCell(x - i * mx, y, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::RIGHTWALL;
			};
		};			

	//Check left
	x = m_lNodes[_iNode].m_GridPos[0] - mx * bs; 
	y = m_lNodes[_iNode].m_GridPos[1] - my * bs; 
	if (IsWallCell(x, y, m_lNodes[_iNode].m_GridPos[2], _pSearch))
		m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::LEFTWALL;
	else
		if ((mx != 0) || (mx != 0))
		{
			//Movement direction parallell to a coordinate axis
			for (i = 1; i < bs; i++)
			{
				if (IsWallCell(x + i * my, y + i * mx, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::LEFTWALL;
				if (IsWallCell(x - i * my, y - i * mx, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::LEFTWALL;
			};
		} else
		{
			//Movement direction diagonal over coordinate axes
			for (i = 1; i < bs; i++)
			{
				if (IsWallCell(x, y + i * my, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::LEFTWALL;
				if (IsWallCell(x + i * mx, y, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::LEFTWALL;
			};
		};			
};


//Helper function to PostProcess_SetDangerInfo; Checks if cell at given position is dangerous
//(i.e. if there are any die cells at between 0 and crouchheight cells above the cell)
bool CXBN_SearchResult::IsDangerCell(int x, int y, int z, CXBN_SearchInstance* _pSearch)
{
	MAUTOSTRIP(CXBN_SearchResult_IsDangerCell, false);
	//Anything outside the grid is automatically considered to be a die cell
	if (_pSearch->IsOutsideGridBoundaries(CVec3Dint16(x,y,z)) ||
	    _pSearch->IsOutsideGridBoundaries(CVec3Dint16(x,y,z + _pSearch->m_Params.m_iHeight - 1)))
		return true;
	
	for (int i=0; i < _pSearch->m_Params.m_iHeight; i++)
		if(_pSearch->GetCellValue(x, y, z + i) & XR_CELL_DIE)
			//Found die cell!
			return true;

	//No die cells found
	return false;
};


//Sets any right, left, front or back danger flags as appropriate for the given node
void CXBN_SearchResult::PostProcess_SetDangerInfo(int _iNode, CXBN_SearchInstance* _pSearch)
{
	MAUTOSTRIP(CXBN_SearchResult_PostProcess_SetDangerInfo, MAUTOSTRIP_VOID);
	//Temps/Iterators
	int x, y, i;
	int bs = _pSearch->m_Params.m_iBaseSize + 1;
	int mx = m_lNodes[_iNode].m_MoveX;
	int my = m_lNodes[_iNode].m_MoveY;

	//Checks the cells around the node for any danger cells.
	//In the below examples the basesize = 1, F,R,L and B denotes the cells to be checked to the 
	//front, right, left and back, respectively. X is the center of the base. The numbers in 
	//parenthesis denote the (MoveX,MoveY) values for the example.
	//xy-plane:
	//			LL FF         LLL   
	//			L	F		 B	 F
	//	  (1,1):  X	  (1,0): B X F
	//			B	R		 B	 F
	//			BB RR         RRR
	
	//Check front
	x = m_lNodes[_iNode].m_GridPos[0] + mx * bs; 
	y = m_lNodes[_iNode].m_GridPos[1] + my * bs; 
	if (IsDangerCell(x, y, m_lNodes[_iNode].m_GridPos[2], _pSearch))
		m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::FRONTDANGER;
	else
		if ((mx != 0) || (mx != 0))
		{
			//Movement direction parallell to a coordinate axis
			for (i = 1; i < bs; i++)
			{
				if (IsDangerCell(x + i * my, y + i * mx, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::FRONTDANGER;
				if (IsDangerCell(x - i * my, y - i * mx, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::FRONTDANGER;
			};
		} else
		{
			//Movement direction diagonal over coordinate axes
			for (i = 1; i < bs; i++)
			{
				if (IsDangerCell(x, y - i * my, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::FRONTDANGER;
				if (IsDangerCell(x - i * mx, y, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::FRONTDANGER;
			};
		};
	
	//Check back
	x = m_lNodes[_iNode].m_GridPos[0] - mx * bs; 
	y = m_lNodes[_iNode].m_GridPos[1] - my * bs; 
	if (IsDangerCell(x, y, m_lNodes[_iNode].m_GridPos[2], _pSearch))
		m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::BACKDANGER;
	else
		if ((mx != 0) || (mx != 0))
		{
			//Movement direction parallell to a coordinate axis
			for (i = 1; i < bs; i++)
			{
				if (IsDangerCell(x + i * my, y + i * mx, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::BACKDANGER;
				if (IsDangerCell(x - i * my, y - i * mx, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::BACKDANGER;
			};
		} else
		{
			//Movement direction diagonal over coordinate axes
			for (i = 1; i < bs; i++)
			{
				if (IsDangerCell(x, y + i * my, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::BACKDANGER;
				if (IsDangerCell(x + i * mx, y, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::BACKDANGER;
			};
		};			

	//Rotate mx and my
	x = mx;
    mx = ms_RotateClockwise[mx+1][my+1][0];
	my = ms_RotateClockwise[x+1][my+1][1];

	//Check right
	x = m_lNodes[_iNode].m_GridPos[0] + mx * bs; 
	y = m_lNodes[_iNode].m_GridPos[1] + my * bs; 
	if (IsDangerCell(x, y, m_lNodes[_iNode].m_GridPos[2], _pSearch))
		m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::RIGHTDANGER;
	else
		if ((mx != 0) || (mx != 0))
		{
			//Movement direction parallell to a coordinate axis
			for (i = 1; i < bs; i++)
			{
				if (IsDangerCell(x + i * my, y + i * mx, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::RIGHTDANGER;
				if (IsDangerCell(x - i * my, y - i * mx, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::RIGHTDANGER;
			};
		} else
		{
			//Movement direction diagonal over coordinate axes
			for (i = 1; i < bs; i++)
			{
				if (IsDangerCell(x, y - i * my, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::RIGHTDANGER;
				if (IsDangerCell(x - i * mx, y, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::RIGHTDANGER;
			};
		};			

	//Check left
	x = m_lNodes[_iNode].m_GridPos[0] - mx * bs; 
	y = m_lNodes[_iNode].m_GridPos[1] - my * bs; 
	if (IsDangerCell(x, y, m_lNodes[_iNode].m_GridPos[2], _pSearch))
		m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::LEFTDANGER;
	else
		if ((mx != 0) || (mx != 0))
		{
			//Movement direction parallell to a coordinate axis
			for (i = 1; i < bs; i++)
			{
				if (IsDangerCell(x + i * my, y + i * mx, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::LEFTDANGER;
				if (IsDangerCell(x - i * my, y - i * mx, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::LEFTDANGER;
			};
		} else
		{
			//Movement direction diagonal over coordinate axes
			for (i = 1; i < bs; i++)
			{
				if (IsDangerCell(x, y + i * my, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::LEFTDANGER;
				if (IsDangerCell(x + i * mx, y, m_lNodes[_iNode].m_GridPos[2], _pSearch))
					m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::LEFTDANGER;
			};
		};			
};


//Movement direction "index" from 0 to 7, clockwise starting at (1,0). 
//The indices are the x- and y movement direction + 1 (i.e. in the interval 0..2
//instead of -1..1).
int ms_MoveDirIndex[3][3] = 
{
	//MoveX = -1, MoveY = -1..1
	{3, 4, 5},
    //MoveX = 0, MoveY = -1, 1 ((0,0) should be undefined, but is treated as (1,0))
	{2, 0, 6},
	//MoveX = 1, MoveY = -1..1
	{1, 0, 7}
};
		
//Sets any right or left turn flags as appropriate for the given node
void CXBN_SearchResult::PostProcess_SetTurnInfo(int _iNode, CXBN_SearchInstance* _pSearch)
{
	MAUTOSTRIP(CXBN_SearchResult_PostProcess_SetTurnInfo, MAUTOSTRIP_VOID);
	//A turn node is a node where you change movement direction by more than 45 degrees
	//If there is no previous node then, obviously we cannot have changed direction
	if (_iNode == 0)
		return;

	//Relative movement direction index of current node, modulo 8 
	//(i.e. 1..3 is turn right 5..7 is turn left) 
	int RelMoveDir = ms_MoveDirIndex[m_lNodes[_iNode].m_MoveX + 1][m_lNodes[_iNode].m_MoveY + 1] -
					 ms_MoveDirIndex[m_lNodes[_iNode - 1].m_MoveX + 1][m_lNodes[_iNode - 1].m_MoveY + 1];
	if (RelMoveDir < 0)
		RelMoveDir += 8;

	//No turn if movement direction indices differ by 1 
	if ((RelMoveDir <= 1) || (RelMoveDir == 7))
		return;

	//We have a turn, check in which direction. If 180 degree turn both flags should be set.
	//Check right
	if ((RelMoveDir <= 4))
		m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::RIGHTTURN;
	//Check left
	if ((RelMoveDir >= 4))
		m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::LEFTTURN;
};

//Sets the stop flag if the given node is a stopp node.
void CXBN_SearchResult::PostProcess_SetStopInfo(int _iNode, CXBN_SearchInstance* _pSearch)
{
	MAUTOSTRIP(CXBN_SearchResult_PostProcess_SetStopInfo, MAUTOSTRIP_VOID);
	//A stop node is in general a node where there is risk of slipping and getting 
	//off the path if velocity is not reduced to zero (or very litle) at the node.
	//I'll have to expand the definition of what should count as a stop node
	//as I find more special cases when one might have to stop, but these are 
	//the ones so far:
	//  Nodes where you make a sharp turn when landing.
	//  Nodes where you make a sharp turn and jump.
	//  Nodes where you should fall straight down.

	//If this is the first or last node then it's already been set to a stop node
	if ((_iNode == 0) || (_iNode == m_lNodes.Len() - 1))
		return;

	// We can jump, no sissy stops
	if ((_pSearch)&&(_pSearch->m_Params.m_bWallClimb))
	{
		return;
	}

	//Sharp turn when landing?
	if ((m_lNodes[_iNode - 1].m_Flags & CXBN_SearchResultNode::AIRBORNE) &&
		(m_lNodes[_iNode].m_Flags & (CXBN_SearchResultNode::RIGHTTURN | CXBN_SearchResultNode::LEFTTURN)))
		m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::STOP_;

	//Sharp turn when jumping?
	if ((m_lNodes[_iNode].m_Flags & CXBN_SearchResultNode::JUMP) &&
		(m_lNodes[_iNode].m_Flags & (CXBN_SearchResultNode::RIGHTTURN | CXBN_SearchResultNode::LEFTTURN)))
		m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::STOP_;

	//Should we fall straight down?
	if ((m_lNodes[_iNode].m_GridPos[0] == m_lNodes[_iNode + 1].m_GridPos[0]) &&
		(m_lNodes[_iNode].m_GridPos[1] == m_lNodes[_iNode + 1].m_GridPos[1]))
		m_lNodes[_iNode].m_Flags |= CXBN_SearchResultNode::STOP_;
};


//Helper method to the PostProcess method. Checks if the node is an intermediate 
//node after the node specified by _Origin.
//This method is unfortunately heavily dependent on the exact usage of the PostProcess method.
bool CXBN_SearchResult::IsIntermediateNode(int _Node, int _Origin)
{
	MAUTOSTRIP(CXBN_SearchResult_IsIntermediateNode, false);
	//An intermediate node is a node that can be ignored when following a path
	//The node before a series of intermediate nodes is the "origin", and any 
	//nodes directly after this node is intermediate if it has the same flags as 
	//the origin, and the next node can be safely reached by moving in a straight line 
	//from the origin, and a few other criteria is met (see below)

	// First node is ALWAYS intermediate
	if (_Node == 0) 
		return true;

	//The first and last nodes are never an intermediate node
	if ((_Node == m_lNodes.Len() - 1)) 
		return false;

	//An airborne node succeeding another airborne node or a jump node
	//is always intermediate.
	if ((m_lNodes[_Node].m_Flags & CXBN_SearchResultNode::AIRBORNE) &&
		(m_lNodes[_Node - 1].m_Flags & (CXBN_SearchResultNode::AIRBORNE | CXBN_SearchResultNode::JUMP)))
		return true;

	//The flags of an intermediate node must be the same as the origin
	if (m_lNodes[_Node].m_Flags != m_lNodes[_Origin].m_Flags)
		return false;

	//Stop, jump, turn and front/back danger nodes are never intermediate,
	if (m_lNodes[_Node].m_Flags &
		(CXBN_SearchResultNode::JUMP | CXBN_SearchResultNode::RIGHTTURN | 
		 CXBN_SearchResultNode::LEFTTURN | CXBN_SearchResultNode::FRONTDANGER | 
		 CXBN_SearchResultNode::BACKDANGER | CXBN_SearchResultNode::STOP_))
		return false;

	//A non-crouch node before a crouch node is never an intermediate node
	if ((m_lNodes[_Node + 1].m_Flags & CXBN_SearchResultNode::CROUCH) &&
		!(m_lNodes[_Node].m_Flags & CXBN_SearchResultNode::CROUCH))
		return false;

	//When following a wall, edge or danger zone, the nodes are intermediate as long 
	//as we don't change movement direction.
	if (m_lNodes[_Node].m_Flags & 
		(CXBN_SearchResultNode::RIGHTWALL | CXBN_SearchResultNode::LEFTWALL |
		 CXBN_SearchResultNode::RIGHTEDGE | CXBN_SearchResultNode::LEFTEDGE |
		 CXBN_SearchResultNode::RIGHTDANGER | CXBN_SearchResultNode::LEFTDANGER))
		return ((m_lNodes[_Node].m_MoveX == m_lNodes[_Origin].m_MoveX) && 
				(m_lNodes[_Node].m_MoveY == m_lNodes[_Origin].m_MoveY));

	//If none of the above apply, there may be any number of intermediate nodes on a 
	//straight line (i.e. the same movement direction as the origin), or there may be 
	//a sequence of up to NO_INTERMEDIATENODES with some intermediate nodes that have 
	//a 45 movement direction difference with the origin. 
	
	//There is a better criterium for this, but it's computationally more expensive and 
	//more complex to formulate, so I'll leave it like this until later. Will fix.
	
	//Reset m_bHasTurned if this is first node after origin
	if (_Node == _Origin + 1)
		m_bHasTurned = false;

	if ((m_lNodes[_Node].m_MoveX == m_lNodes[_Origin].m_MoveX) && 
		(m_lNodes[_Node].m_MoveY == m_lNodes[_Origin].m_MoveY))
		//We're moving in the same direction as the origin
		if (m_bHasTurned && 
			(_Node - _Origin > NO_INTERMEDIATENODES))
			//We have already turned (at least twice) and this node
			//would make the sequence of intermediate nodes too long 
			//to be safe.
			return false;
		else
			//It's okay!
			return true;
	else
	{
		//We're making a turn.
		m_bHasTurned = true;

		//Is the turn within allowed angle from the origins movement direction?
		if ((((ms_MoveDirIndex[m_lNodes[_Node].m_MoveX + 1][m_lNodes[_Node].m_MoveY + 1] + 1) % 8) !=
			 ((ms_MoveDirIndex[m_lNodes[_Origin].m_MoveX + 1][m_lNodes[_Origin].m_MoveY + 1]) % 8)) &&
			(((ms_MoveDirIndex[m_lNodes[_Node].m_MoveX + 1][m_lNodes[_Node].m_MoveY + 1]) % 8) !=
			 ((ms_MoveDirIndex[m_lNodes[_Origin].m_MoveX + 1][m_lNodes[_Origin].m_MoveY + 1] + 1) % 8)))
			//Nope, the turn is not allowed
			return false;
		else
			//The turn was allowed. If this node is sufficiently close to the 
			//origin, then it's intermediate.
			return (_Node - _Origin <= NO_INTERMEDIATENODES);
	};
};				   	
				

//Adds extra information to the search result to better the movement system performance
void CXBN_SearchResult::PostProcess(CXBN_SearchInstance* _pSearch, fp64 _Time)
{
	MAUTOSTRIP(CXBN_SearchResult_PostProcess, MAUTOSTRIP_VOID);

	return;

	//This method is currently called from Create, but is designed to later be able to
	//be called independently from Create. 

	//Extra info:
	//  Node movementdirection:					 The approximate direction entity should move in at node.
	//
	//  Stop nodes:								 Entity should try to have low or no velocity.
	//
	//  Left, right, front and back edge nodes   Entity runs risk of falling if not careful.
	//
	//  Left, right, front and back wall nodes   Entity cannot move much in the direction specified and 
	//											 runs risk of getting stuck in small irreguarities not
	//											 detected by the grid. A node with a wall on one side and
	//											 an edge on the other is a ledge; it might be a good idea
	//											 to press oneself against the wall when moving past.
	//
	//	Left, right, front and back danger nodes Entity runs risk of getting damaged if not careful.
	//
	//  Left and right turn nodes:				 Entity must make a sharp turn, and might easily get stuck.
	//
	//  Intermediate nodes:						 These can be ignored when following the path.

	//Start timers
#ifndef PLATFORM_VPU
	CMTime T, T2;
	CMTime FreqTime = CMTime::CreateFromSeconds(_Time);
	int bTimeOut = _Time > 0.0f;
	TStart(T);
	T2 = T;
#endif

	if (m_iPostProcessCount == 0)
	{
		//First node is by definition a stop node, and we have no velocity 
		m_lNodes[0].m_Flags |= CXBN_SearchResultNode::STOP_;
		//Set movement direction if we have at least one more node
		if (m_lNodes.Len() > 1)
		{
			m_lNodes[0].m_MoveX = 
				(m_lNodes[0].m_GridPos[0] == m_lNodes[1].m_GridPos[0]) ?
				0 : ((m_lNodes[0].m_GridPos[0] < m_lNodes[1].m_GridPos[0]) ? 1 : -1);
			m_lNodes[0].m_MoveY = 
				(m_lNodes[0].m_GridPos[1] == m_lNodes[1].m_GridPos[1]) ?
				0 : ((m_lNodes[0].m_GridPos[1] < m_lNodes[1].m_GridPos[1]) ? 1 : -1);
		};
		//Set movement direction to default (1,0) if we are falling
		if ((m_lNodes[0].m_MoveX == 0) && (m_lNodes[0].m_MoveY == 0))
		{
			m_lNodes[0].m_MoveX = 1;
			m_lNodes[0].m_MoveY = 0;
		};

		m_iPostProcessCount = 1;
	};

#ifndef PLATFORM_VPU
	//Timeout?
	TStop(T);
	if (bTimeOut && (T.Compare(FreqTime) > 0))
		return;
	T = T2;
#endif

	//Are we in the first pass?
	if (m_iPostProcessCount < m_lNodes.Len())
	{
		//First pass over all nodes but the first and the last; sets
		//node movementdirection and velocity..
		for (int i = m_iPostProcessCount; i < m_lNodes.Len() - 1; i++)
		{
			//Set movement direction towards next node
			m_lNodes[i].m_MoveX = 
				(m_lNodes[i].m_GridPos[0] == m_lNodes[i + 1].m_GridPos[0]) ? 
				0 : ((m_lNodes[i].m_GridPos[0] < m_lNodes[i + 1].m_GridPos[0]) ? 1 : -1); 
			m_lNodes[i].m_MoveY = 
				(m_lNodes[i].m_GridPos[1] == m_lNodes[i + 1].m_GridPos[1]) ? 
				0 : ((m_lNodes[i].m_GridPos[1] < m_lNodes[i + 1].m_GridPos[1]) ? 1 : -1);

			if ((m_lNodes[i].m_MoveX == 0) && (m_lNodes[i].m_MoveY == 0))
			{
				//We're falling, or jumping, set movedir to same as previous node
				m_lNodes[i].m_MoveX = m_lNodes[i - 1].m_MoveX;
				m_lNodes[i].m_MoveY = m_lNodes[i - 1].m_MoveY;
			};

			#ifndef PLATFORM_VPU
				//Timeout?
				TStop(T);
				if (bTimeOut && (T.Compare(FreqTime) > 0))
				{
					m_iPostProcessCount = i;
					return;
				};
				T = T2;
			#endif
		};

		//Last node is always stop node, with zero velocity and same movedir as previous node
		m_lNodes[m_lNodes.Len() - 1].m_Flags |= CXBN_SearchResultNode::STOP_;
		m_lNodes[m_lNodes.Len() - 1].m_MoveX = m_lNodes[m_lNodes.Len() - 2].m_MoveX;
		m_lNodes[m_lNodes.Len() - 1].m_MoveY = m_lNodes[m_lNodes.Len() - 2].m_MoveY;

		//First pass done
		m_iPostProcessCount = m_lNodes.Len();
	};

	//Are we in the second pass?
	if (m_iPostProcessCount < 2 * m_lNodes.Len())
	{
		//Second pass finds stop, edge, wall, danger and turn nodes.
		for (int i = m_iPostProcessCount - m_lNodes.Len(); i < m_lNodes.Len(); i++)
		{
			//Is this an edge node?
			//Skip this test for now fix!
			//PostProcess_SetEdgeInfo(i, _pSearch);
			
			//Is this a wall node?
			//Skip this test for now fix!
			//PostProcess_SetWallInfo(i, _pSearch);

			//Is this a danger node?
			//Skip this test for now fix!
			//PostProcess_SetDangerInfo(i, _pSearch);
			
			//Is this a turn node?
			PostProcess_SetTurnInfo(i, _pSearch);

			//Is this a stop node?
			PostProcess_SetStopInfo(i, _pSearch);
			
			//Timeout?
			#ifndef PLATFORM_VPU
				TStop(T);
				if (bTimeOut && (T.Compare(FreqTime) > 0))
				{
					m_iPostProcessCount = i + m_lNodes.Len();
					return;
				};
				T = T2;
			#endif
		};

		//Second pass done
		m_iPostProcessCount = 2 * m_lNodes.Len();
	};

	//Are we in the third pass?
	if (m_iPostProcessCount < 3 * m_lNodes.Len())
	{
		//Third pass finds intermediate nodes. We do not need to check the last node.
		int j;
		for (int i = m_iPostProcessCount - 2 * m_lNodes.Len(); i < m_lNodes.Len() - 1; i++)
		{
			//Scan forward to find the next non-intermediate node
			j = i + 1;
			while (IsIntermediateNode(j,i))
			{
				m_lNodes[j].m_Flags |= CXBN_SearchResultNode::INTERMEDIATE;
				j++;
			};
			
			i = j - 1;
			
			//Timeout?
			#ifndef PLATFORM_VPU
				TStop(T);
				if (bTimeOut && (T.Compare(FreqTime) > 0))
				{
					m_iPostProcessCount = i + 2 * m_lNodes.Len();
					return;
				};
				T = T2;
			#endif
		};

		//Third pass done
		m_iPostProcessCount = 3 * m_lNodes.Len();
	};

	//Fourth pass; just set safe length of path
	int UnSafe = CXBN_SearchResultNode::AIRBORNE |
				 CXBN_SearchResultNode::JUMP |
				 CXBN_SearchResultNode::RIGHTEDGE |
				 CXBN_SearchResultNode::LEFTEDGE |
				 CXBN_SearchResultNode::FRONTEDGE |
				 CXBN_SearchResultNode::BACKEDGE |
				 CXBN_SearchResultNode::RIGHTDANGER |
				 CXBN_SearchResultNode::LEFTDANGER |
				 CXBN_SearchResultNode::FRONTDANGER |
				 CXBN_SearchResultNode::BACKDANGER;
	m_SafeLength = 0;
	while ((m_SafeLength < m_lNodes.Len()) &&
		   !(m_lNodes[m_SafeLength].m_Flags & UnSafe))
	{
		m_SafeLength++;
	}
	

	#ifndef PLATFORM_VPU
		TStop(T);
		//ConOut(CStrF("Final PostProcess %.f ms", T / GetCPUFrequency() * 1000.0f));
	#endif
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Checks if position is at search node
						
	Parameters:			
		_NodeIndex:		The index to the node
		_Pos:			The position to be checked against
		_Up:			The local upvector
						
	Returns:			True if the position is considered to be at 
						the node, false otherwise.
\*____________________________________________________________________*/
bool CXBN_SearchResult::AtNode(int _NodeIndex, CVec3Dfp32 _Pos, CVec3Dfp32 _Up)
{
	MAUTOSTRIP(CXBN_SearchResult_AtNode, false);
	//As the difference in height between the node position and the actual position 
	//of a character can easily differ due to relatively low cell resolution (compared
	//to the actual world), this difference is reduced by two node sizes for the purpose 
	//of checking if a position is at a node.
	CVec3Dfp32 Diff = m_lNodes[_NodeIndex].GetPosition() - _Pos;
	fp32 upDiff = Diff * _Up;	// Height diff
	Diff -= _Up * upDiff;		// Diff is now strictly flat
	if (upDiff >= 0.0f)
	{
		upDiff -= 3 * m_NodeSize;
		upDiff = Max(0.0f,upDiff);
	}
	else
	{	// *** Maybe we should not check up here? Never ligt nodepos more than 1 size?
		if (_Up[2] >= 0.996f)
		{
			upDiff += 3 * m_NodeSize;
			upDiff = Min(0.0f,upDiff);
		}
		else
		{
			upDiff += 1 * m_NodeSize;
		}
	}
	Diff += _Up * upDiff;
	return (Sqr(m_NodeSize) >= Diff.LengthSqr());
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Checks if position is at search node in the
						XY-plane.
						
	Parameters:			
		_NodeIndex:		The index to the node
		_Pos:			The position to be checked against
						
	Returns:			True if the position is considered to be at 
						the node, false otherwise.
\*____________________________________________________________________*/
bool CXBN_SearchResult::AtNodeXY(int _NodeIndex, CVec3Dfp32 _Pos)
{
	MAUTOSTRIP(CXBN_SearchResult_AtNodeXY, false);
	_Pos -= m_lNodes[_NodeIndex].GetPosition();
	_Pos[2] = 0;
	return (Sqr(m_NodeSize) >= _Pos.LengthSqr());
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Copy search result content into this object
						
	Parameters:			
		_pSearcResult:	The search resuilt to copy from
\*____________________________________________________________________*/
void CXBN_SearchResult::Copy(CXBN_SearchResult * _pSearchResult)
{
	MAUTOSTRIP(CXBN_SearchResult_Copy, MAUTOSTRIP_VOID);
	Reset();

	if (!_pSearchResult)
		return;

	_pSearchResult->m_lNodes.Duplicate(&m_lNodes);
	m_NodeSize = _pSearchResult->m_NodeSize;
	m_Cost = _pSearchResult->m_Cost;
};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Debug routine to print search result to file
						
	Parameters:			
		_FileName:		The desired file to be printed to.

	Comments:			Any old file is overwritten.
\*____________________________________________________________________*/
void CXBN_SearchResult::PrintToFile(CStr _FileName)
{
	MAUTOSTRIP(CXBN_SearchResult_PrintToFile, MAUTOSTRIP_VOID);
	CCFile File;
	File.Open(_FileName, CFILE_WRITE);

	File.Writeln("Search result:");
	CStr Str = " ";

	for (int i = 0; i < m_lNodes.Len(); i++)
	{
		File.Writeln(CStrF("Node %d: ", i) + (m_lNodes[i].GetPosition()).GetString());

		Str = CStrF("         (%d,%d,%d) MoveDir(%d,%d) ", 
				   m_lNodes[i].m_GridPos[0], 
				   m_lNodes[i].m_GridPos[1], 
				   m_lNodes[i].m_GridPos[2], 
				   m_lNodes[i].m_MoveX, 
				   m_lNodes[i].m_MoveY);
		
		if (m_lNodes[i].GetFlags() & CXBN_SearchResultNode::AIRBORNE)
			Str += " Airborne";
		if (m_lNodes[i].GetFlags() & CXBN_SearchResultNode::JUMP)
			Str += " Jump";
		if (m_lNodes[i].GetFlags() & CXBN_SearchResultNode::CROUCH)
			Str += " Crouch";
		if (m_lNodes[i].GetFlags() & CXBN_SearchResultNode::INTERMEDIATE)
			Str += " Intermediate";
		if (m_lNodes[i].GetFlags() & CXBN_SearchResultNode::STOP_)
			Str += " Stop";
		if (m_lNodes[i].GetFlags() & CXBN_SearchResultNode::RIGHTTURN)
			Str += " RightTurn";
		if (m_lNodes[i].GetFlags() & CXBN_SearchResultNode::LEFTTURN)
			Str += " LeftTurn";
		if (m_lNodes[i].GetFlags() & CXBN_SearchResultNode::RIGHTEDGE)
			Str += " RightEdge";
		if (m_lNodes[i].GetFlags() & CXBN_SearchResultNode::LEFTEDGE)
			Str += " LeftEdge";
		if (m_lNodes[i].GetFlags() & CXBN_SearchResultNode::FRONTEDGE)
			Str += " FrontEdge";
		if (m_lNodes[i].GetFlags() & CXBN_SearchResultNode::BACKEDGE)
			Str += " BackEdge";
		if (m_lNodes[i].GetFlags() & CXBN_SearchResultNode::RIGHTWALL)
			Str += " RightWall";
		if (m_lNodes[i].GetFlags() & CXBN_SearchResultNode::LEFTWALL)
			Str += " LeftWall";
		if (m_lNodes[i].GetFlags() & CXBN_SearchResultNode::FRONTWALL)
			Str += " FrontWall";
		if (m_lNodes[i].GetFlags() & CXBN_SearchResultNode::BACKWALL)
			Str += " BackWall";
		if (m_lNodes[i].GetFlags() & CXBN_SearchResultNode::RIGHTDANGER)
			Str += " RightDanger";
		if (m_lNodes[i].GetFlags() & CXBN_SearchResultNode::LEFTDANGER)
			Str += " LeftDanger";
		if (m_lNodes[i].GetFlags() & CXBN_SearchResultNode::FRONTDANGER)
			Str += " FrontDanger";
		if (m_lNodes[i].GetFlags() & CXBN_SearchResultNode::BACKDANGER)
			Str += " BackDanger";
		
		File.Writeln(Str);
	};

	File.Writeln("***********************************");

	File.Close();
};

#endif
