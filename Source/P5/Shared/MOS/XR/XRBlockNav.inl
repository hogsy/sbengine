
#include "XRBlockNav.h"
#include "XRBlockNavInst.h"

#ifndef BLOCKNAV_VPU_COMPILE
	#include "../Classes/Render/MWireContainer.h"

	#include "XRBlockNavGrid.h"
	#include "../../MCC/MRTC_VPUManager.h"
	#include "../Classes/GameWorld/VPUWorkerNavGrid_Params.h"
	#include "MFloat.h"
#endif

// TODO: clean up logging -kma

//#include "Server/WServer.h"

// #pragma optimize("",off)

#ifndef BLOCKNAV_VPU_COMPILE
	MRTC_IMPLEMENT_DYNAMIC(CXR_BlockNav, CReferenceCount);
	MRTC_IMPLEMENT_DYNAMIC(CXR_BlockNavSearcher, CReferenceCount);
#endif


#define GETATXYZ(pGrid,x,y,z) (pGrid)->GetAt(CVec3Dint32(x,y,z))

// -------------------------------------------------------------------
//  CXBN_SearchInstance
// -------------------------------------------------------------------

const int CXBN_SearchInstance::WBN_COSTFRACTIONS = 16;
const fp32 CXBN_SearchInstance::WBN_DISTANCEFACTOR = 1.0f;
const fp32 CXBN_SearchInstance::WBN_COSTFACTOR = 0.45f;
const int CXBN_SearchInstance::WBN_FALLDAMAGE_COSTFACTOR = 8;
const int CXBN_SearchInstance::WBN_EDGE_COST = 4;
const int CXBN_SearchInstance::WBN_LONGJUMP_COSTFACTOR = 3;

CXBN_SearchNode* CXBN_SearchInstance::NewNode()
{
	MAUTOSTRIP(CXBN_SearchInstance_NewNode, NULL);
	MSCOPESHORT(CXBN_SearchInstance::NewNode);
	if (m_iNodeNext >= m_lNodeHeap.Len())
		return NULL;
	CXBN_SearchNode* pN = &m_lNodeHeap[m_iNodeNext];

	pN->Construct();
	pN->m_iNode = m_iNodeNext;
	m_iNodeNext++;
	return pN;
}

int CXBN_SearchInstance::GetCellValue(int _x, int _y, int _z)
{
	MAUTOSTRIP(CXBN_SearchInstance_GetCellValue, 0);
	return m_pGrid->GetAt(CVec3Dint32(_x, _y, _z));
}

//Checks if the given position is outside or too close to the grid boundaries
bool CXBN_SearchInstance::IsOutsideGridBoundaries(const CVec3Dint16& _Pos)
{
	MAUTOSTRIP(CXBN_SearchInstance_IsOutsideGridBoundaries, false);
	return m_pBlockNav->IsOutsideGridBoundariesImpl(_Pos, m_Params.m_iBaseSize, m_Params.m_iHeight);
};


//Checks if the node specified by _Pos is traversable. The optional _dx, _dy and _dz arguments
//are used to indicate that we are checking to see if we can move from the node at 
//_Pos + (_dx, _dy. _dz) to this node, which means that several cells have probably already 
//been checked for traversability, and some 
//for traversability, thus there are fewer cells that need be checked (since the previously
//checked node have several overlapping cells with this node)
bool CXBN_SearchInstance::IsTraversable(const CVec3Dint16& _Pos, int _dx, int _dy, int _dz)
{
	MAUTOSTRIP(CXBN_SearchInstance_IsTraversable, false);
	return m_pBlockNav->IsTraversableImpl(_Pos, m_Params.m_iBaseSize, m_Params.m_iHeight, m_Params.m_bWallClimb, _dx, _dy, _dz);
};


//Checks if the node specified by the given position is a ground node, i.e. if any of the 
//cells in the entitys base, when standing on the node has solid cells beneath them. This
//method assumes that the node is traversable. The _Count argument is set to the number of 
//non-ground cells found. 
bool CXBN_SearchInstance::IsOnGround(const CVec3Dint16& _Pos, int& _Count)
{
	MAUTOSTRIP(CXBN_SearchInstance_IsOnGround, false);
	return m_pBlockNav->IsOnGroundImpl(_Pos, m_Params.m_iBaseSize, m_Params.m_bWallClimb, _Count);
};

//Default function for ground checking, when count of non-ground nodes are unwanted.
bool CXBN_SearchInstance::IsOnGround(const CVec3Dint16& _Pos)
{
	MAUTOSTRIP(CXBN_SearchInstance_IsOnGround_2, false);
	return m_pBlockNav->IsOnGroundImpl(_Pos, m_Params.m_iBaseSize, m_Params.m_bWallClimb);
};



//Check if position already has been visited. This should be fixed to full A* eventually 
//Use of virtual method won't allow compiler to inline pushnode, so this might not be a good solution. Must test.
bool CXBN_SearchInstance::CanPrune(const CVec3Dint16& _Pos)
{	
	MAUTOSTRIP(CXBN_SearchInstance_CanPrune, false);
	//Check if the node has already been pushed
	const CXBN_SearchNode* pNodeCell = Hash_GetNode(_Pos[0], _Pos[1], _Pos[2]);
	if (pNodeCell)// && pNodeCell->m_Flags)
	{
		// For now, prune all nodes we've already visited. Will fix to full A*.
		return true;	
	}
	else
		return false;
}


//This is run when node has been successfully created and pushed. Inserts node in hash for this class.
void CXBN_SearchInstance::VisitNode(CXBN_SearchNode* _pN)
{
	MAUTOSTRIP(CXBN_SearchInstance_VisitNode, MAUTOSTRIP_VOID);
	Hash_InsertNode(_pN->m_iNode);
};


//Adds a node to the open priority queue if it's traversable 
//and the current path is the best way to get there.
int CXBN_SearchInstance::PushNode(CXBN_SearchNode* _pN, int _dx, int _dy, int _dz, int& _Flags, const CVec3Dint16& _Vel, int _ExtraCost)
{
	MAUTOSTRIP(CXBN_SearchInstance_PushNode, 0);
	MSCOPESHORT(CXBN_SearchInstance::PushNode);
	//_dz is 4 times the actual grid position difference, to enable "fractions" 
	//(actually fourths) of a cell position for more accurate jump parabels.
	//Z is four times the actual z grid position
	int Z = _pN->m_Pos[2]*4 + _pN->m_ZPosFrac + _dz;
	//The grid z-position (with the fraction truncated)
	int ZInt = Z >> 2;
	//The remainder
	int ZFrac = Z & 3;

	//The grid position of the node to be pushed
	CVec3Dint16 Origin(_pN->m_Pos[0] + _dx, _pN->m_Pos[1] + _dy, ZInt);

	// Check grid boundaries
	if (IsOutsideGridBoundaries(Origin))
		return PUSHNODE_OUTOFBOUNDS;

	//Check if this node can be pruned 
	//(Check how this will affect efficiency. Might be necessary to use method pointer or copy code to navgraph construction)
	if (CanPrune(Origin))
		//This won't allow true A*, but neither will used data structures. Fix.
		return PUSHNODE_EXIST;	

	//Check if node is non-traversable. A node is traversable only if all cells within the size of the
	//entity doing the pathfinding are non-blocked (i.e. deadly or contains a non-traversable medium)
	if (!IsTraversable(Origin, -_dx, -_dy, _pN->m_Pos[2] - ZInt))
		return PUSHNODE_BLOCKED;

	//Check if node can be created
	CXBN_SearchNode* pChild = NewNode();
	if (!pChild) 
	{
		m_iCauseOfFailure = CXR_BlockNavSearcher::SEARCH_NODE_ALLOC_FAIL;
		return PUSHNODE_ALLOCFAIL;
	};
		
	//Set the new node's velocities
	pChild->m_XVel = _Vel[0];
	pChild->m_YVel = _Vel[1];
	pChild->m_ZVel = _Vel[2];

	if (pChild->m_ZVel < -11) 
	{
		return PUSHNODE_BLOCKEDDAMAGE;
	}
		

	//The cost of going to the node from last node. Just use XY-cost.
	//int dCost = RoundToInt(WBN_COSTFACTOR*Length3(WBN_COSTFRACTIONS*_dx, WBN_COSTFRACTIONS*_dy, WBN_COSTFRACTIONS*(_dz/4)));
	int dCost;
	if (m_Params.m_bWallClimb)
	{
		dCost = RoundToInt(WBN_COSTFACTOR*Length3(WBN_COSTFRACTIONS*_dx, WBN_COSTFRACTIONS*_dy, WBN_COSTFRACTIONS*(_dz/4)));
		// dCost = RoundToInt(WBN_COSTFACTOR*(Length2(WBN_COSTFRACTIONS*_dx, WBN_COSTFRACTIONS*_dy)+WBN_COSTFRACTIONS*Abs(_dz)));
	}
	else
	{
		dCost = RoundToInt(WBN_COSTFACTOR*Length2(WBN_COSTFRACTIONS*_dx, WBN_COSTFRACTIONS*_dy));
	}

	//Increase cost dramatically if going to this node will incur some falling damage
	if (!m_Params.m_bWallClimb)
	{
		/*
		if ((!m_Params.m_bJump)&&(pChild->m_ZVel < -4))
		{
			return PUSHNODE_BLOCKED;
		}
		*/
		if (pChild->m_ZVel < -8)
		{
			if (!m_Params.m_bJump)
			{
				return PUSHNODE_BLOCKED;
			}
			else
			{
				dCost += WBN_FALLDAMAGE_COSTFACTOR * (-(pChild->m_ZVel + 8));
			}
		}
	}
	
	// Hm, could we somehow combine this call with the above, or precompile it somehow?
	if (!m_Params.m_bWallClimb) // *** DEBUG: Extra cost near walls for wallclimbers too ***
	{
		if (!m_pBlockNav->IsTraversableImpl(Origin,m_Params.m_iBaseSize+1,m_Params.m_iHeight+2, m_Params.m_bWallClimb, -_dx, -_dy, _pN->m_Pos[2] - ZInt))
		{	// Expensive
			dCost += 20;
		}
	}

	//Check if we're on ground and if so, increase the cost slightly if we're close to an edge 
	int NonGroundCells = 0;
	if (IsOnGround(Origin, NonGroundCells))
	{	// Add extra cost if more than half groundcells are in the air
		if (2 * NonGroundCells >= Sqr(2 * m_Params.m_iBaseSize + 1))
		{	// Really? What about walking on stairs?
			dCost += 20;
		}
	} 
	else
	{
		if ((!m_Params.m_bWallClimb)&&(!m_Params.m_bJump)&&(_pN->m_Flags & WBN_NODE_AIRBORNE))
		{	// Not allowed to go from airborde to airborne when not allowed to jump
			return PUSHNODE_BLOCKED;
		}

		_Flags |= WBN_NODE_AIRBORNE;
		dCost += 20;
		if ((_pN->m_Flags & WBN_NODE_LONGJUMP) ||
			(_Flags & WBN_NODE_LONGJUMP))
		{
			_Flags |= WBN_NODE_LONGJUMP;
			dCost *= WBN_LONGJUMP_COSTFACTOR;
		}
	}


	//The minimum remaining distance.
	// XY-distance only for nonwallclimbers
	int Distance;
	if (m_Params.m_bWallClimb)
	{
		
		Distance = RoundToInt(WBN_DISTANCEFACTOR*Length3(WBN_COSTFRACTIONS*(m_Params.m_Dst[0] - Origin[0]), 
								WBN_COSTFRACTIONS*(m_Params.m_Dst[1] - Origin[1]),
								WBN_COSTFRACTIONS*(m_Params.m_Dst[2] - Origin[2])));
	}
	else
	{
		Distance = RoundToInt(WBN_DISTANCEFACTOR*Length2(WBN_COSTFRACTIONS*(m_Params.m_Dst[0] - Origin[0]), 
								WBN_COSTFRACTIONS*(m_Params.m_Dst[1] - Origin[1])));
	}

	//Set other node data
	pChild->m_Flags = _Flags;
	pChild->m_pParent = _pN;
	pChild->m_Cost = _pN->m_Cost + dCost + _ExtraCost;
	pChild->m_Distance = Distance;
	pChild->m_Pos[0] = Origin[0];
	pChild->m_Pos[1] = Origin[1];
	pChild->m_Pos[2] = Origin[2];
	pChild->m_ZPosFrac = ZFrac;


	//Add node to open queue
	m_PQueue.Push(pChild);

	//Node has been visited
	VisitNode(pChild);

	return PUSHNODE_OK;
}


int ms_GroundChildren[][3] =
{
	{-1,-1,0},
	{-1,0,0},
	{-1,1,0},
	{0,1,0},
	{1,1,0},
	{1,0,0},
	{1,-1,0},
	{0,-1,0}
};


//Adds all reachable children of a node 
void CXBN_SearchInstance::PushChildren(CXBN_SearchNode* _pN)
{
	MAUTOSTRIP(CXBN_SearchInstance_PushChildren, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXBN_SearchInstance::PushChildren);
	//Grid boundaries need not be checked since any pushed nodes have their boundaries 
	//checked as well as the source position.

	CXR_BlockNav_GridProvider* M_RESTRICT pGrid = m_pGrid;

	//Are we on ground?
	if (!(_pN->m_Flags & WBN_NODE_AIRBORNE))
	{
		// We're on the ground
		int nGround = sizeof(ms_GroundChildren) / 12;
		//Check surroundings for children
		int dx, dy, flags;
		for(int i = 0; i < nGround; i++)
		{
			dx = ms_GroundChildren[i][0];
			dy = ms_GroundChildren[i][1];
			//Try to walk horizontally to neighbour. If we bump into something, 
			//then we try to move upwards by stepping or jumping instead.
			flags = 0;
			if (PushNode(_pN, dx, dy, 0, flags, 0) == PUSHNODE_BLOCKED)
			{
				//Try to take a step upwards (1 cell)
				//This is actually not really safe, since going up one cell might 
				//mean going up 15.9999... units and the stepsize is only 12.5
				if (PushNode(_pN, dx, dy, 4, flags, 8) == PUSHNODE_BLOCKED)
				{
					// One cell down?
					if (PushNode(_pN, dx, dy, -4, flags, 8) == PUSHNODE_BLOCKED)
					{
						/*
						// *** I removed this expansion as I couldn't possibly see why we want
						// the AI to believe they can traverse up 16+ unit heights, no matter how expensive. This
						// WILL have effects on pathfinding but hopefully only disallow paths they cannot physically traverse ***
						//Since we don't jump we must sometimes risk two-cell step. This is really unsafe, thus expensive.
						PushNode(_pN, dx, dy, 8, 0, 0, 128);
						{

							//Try to jump to different heights (2, 4, 6 and 7 cells) 
							//Hack to remove jump pathfinding
							//if (PushNode(_pN, dx, dy, 8, WBN_PARENT_NODE_JUMP, 0) == PUSHNODE_BLOCKED)
							//if (PushNode(_pN, dx, dy, 16, WBN_PARENT_NODE_JUMP, 0, 16) == PUSHNODE_BLOCKED)
							//if (PushNode(_pN, dx, dy, 24, WBN_PARENT_NODE_JUMP, 0, 32) == PUSHNODE_BLOCKED)
							//PushNode(_pN, dx, dy, 28, WBN_PARENT_NODE_JUMP, 0, 64);
						}
						*/
					}
					flags = 0;
				}
			}
			if (flags &  WBN_NODE_AIRBORNE)
			{	// This one is not expensive as hugging the ground sure beats jumping
				flags = 0;
				PushNode(_pN, dx, dy, -4, flags, 0);
			}
			if (m_iCauseOfFailure == CXR_BlockNavSearcher::SEARCH_NODE_ALLOC_FAIL)
			{
				return;
			}
		}

		if (m_Params.m_bWallClimb)
		{
			// *** DEBUG: Maybe this spawns too many nodes? ***
			int dx, dy, flags;
			for(int i = 0; i < nGround; i++)
			{
				flags = 0;
				dx = ms_GroundChildren[i][0];
				dy = ms_GroundChildren[i][1];
				if (GETATXYZ(pGrid, _pN->m_Pos[0]+dx,_pN->m_Pos[1]+dy,_pN->m_Pos[2]+1) & XR_CELL_WALL)
				{
					PushNode(_pN, dx, dy, 1, flags, 0);
					if (m_iCauseOfFailure == CXR_BlockNavSearcher::SEARCH_NODE_ALLOC_FAIL)
					{
						return;
					}
				}
				if (GETATXYZ(pGrid, _pN->m_Pos[0]+dx,_pN->m_Pos[1]+dy,_pN->m_Pos[2]-1) & XR_CELL_WALL)
				{
					PushNode(_pN, dx, dy, -1, flags, 0);
					if (m_iCauseOfFailure == CXR_BlockNavSearcher::SEARCH_NODE_ALLOC_FAIL)
					{
						return;
					}
				}
			}
			
			// Straight up/down
			if (GETATXYZ(pGrid, _pN->m_Pos[0],_pN->m_Pos[1],_pN->m_Pos[2]) & XR_CELL_WALL)
			{	// Vertical
				PushNode(_pN, 0, 0, 4, flags, 0);
				if (m_iCauseOfFailure == CXR_BlockNavSearcher::SEARCH_NODE_ALLOC_FAIL)
				{
					return;
				}
				PushNode(_pN, 0, 0, -4, flags, 0);
				if (m_iCauseOfFailure == CXR_BlockNavSearcher::SEARCH_NODE_ALLOC_FAIL)
				{
					return;
				}
			}
		}

		//FIX! Long-jumping is currently very unsafe...
		//This only makes jumps in n*45 degree angles (n = {0..7}) to the grid axes, and
		//won't work at all for entities with basesize 0. Will fix. 
		//Should also jump when encountering die cells etc. Will fix.
		//Jumps are currently made with no distinction ta the speed te entity has at the node. 
		//Thus we might not be able to jump as far as the pathfinding believes. Will fix.

		//Do a long jump if there's no ground, or is solid slip, 2 cells under center. 
		//(1 cell probably means that we're on a stair or shallow slope)
		/* No jumping
		if (GETATXYZ(_pN->m_Pos[0], _pN->m_Pos[1], _pN->m_Pos[2]-1) & (XR_CELL_AIR | XR_CELL_SLIP))
		{
			for(int i = 0; i < nGround; i++)
			{
				dx = ms_GroundChildren[i][0];
				dy = ms_GroundChildren[i][1];
				if (//Jump away from the edge
					!(GETATXYZ(_pN->m_Pos[0]-dx, _pN->m_Pos[1]-dy, _pN->m_Pos[2]-1) & XR_CELL_AIR) 
					//..but don't jump in stairs or shallow slopes
				    && (GETATXYZ(_pN->m_Pos[0]+dx, _pN->m_Pos[1]+dy, _pN->m_Pos[2]-2) & XR_CELL_AIR)
					)
					//To reach this child node we have to jump. Thus, if this node is in the path chosen, 
					//the jump should be performed at it's parent node (i.e. _pN)
					PushNode(_pN, dx, dy, 4, WBN_PARENT_NODE_JUMP | WBN_NODE_LONGJUMP, CVec3Dint16(dx,dy,4), 0);
			}
		}
		*/
	}
	else
	{
		// Airborne, just fall
		int flags;
		if (_pN->m_XVel || _pN->m_YVel || _pN->m_ZVel < -4)
		{
			//Airborne with speed
			int XVel = _pN->m_XVel;
			int YVel = _pN->m_YVel;
			int flags;
			for(int i = 1; i <= 4; i++)
			{
				flags = 0;
				if (PushNode(_pN, XVel, YVel, _pN->m_ZVel + (i-1)*4, flags, CVec3Dint16(XVel, YVel, _pN->m_ZVel-1)) == PUSHNODE_BLOCKED)
				{
					//We hit something in mid-air, try to fall down
					if (_pN->m_ZVel < -i*4)
						continue;
					else
					{
						// No velocity, just fall
						PushNode(_pN, 0, 0, -1*4, flags, CVec3Dint16(0,0,_pN->m_ZVel - 4));
					}
				}
				else
				{
					break;
				}
				if (m_iCauseOfFailure == CXR_BlockNavSearcher::SEARCH_NODE_ALLOC_FAIL)
				{
					return;
				}
			}
		}
		else
		{
			// No velocity, just starting to fall
			flags = 0;
			if (PushNode(_pN, 0, 0, -1*4, flags, CVec3Dint16(0,0,_pN->m_ZVel - 4)) == PUSHNODE_BLOCKED)
			{
				//Cannot fall straight down. Check for slide in current move direction.
				CXBN_SearchNode * pParent = _pN->m_pParent;
				int dx = 0;
				int dy = 0;
				int i = 0;
				//Search for last movement direction a bit backwards (since we can slide after a vertical fall)
				while (pParent && (i++ < 2) && !(dx || dy))
				{
  					dx = _pN->m_Pos[0] - pParent->m_Pos[0];
					dy = _pN->m_Pos[1] - pParent->m_Pos[1];
					pParent = pParent->m_pParent;
				}
				if (dx || dy)
				{
					//Push slide node. Slight extra cost.
					PushNode(_pN, dx, dy, 0, flags, CVec3Dint16(0,0,_pN->m_ZVel), 16);
					if (m_iCauseOfFailure == CXR_BlockNavSearcher::SEARCH_NODE_ALLOC_FAIL)
					{
						return;
					}
				}
			}
			else
			{
				if (m_iCauseOfFailure == CXR_BlockNavSearcher::SEARCH_NODE_ALLOC_FAIL)
				{
					return;
				}
			}
		}
	}
}


bool CXBN_SearchInstance::InUse(int _GameTick)
{
	MAUTOSTRIP(CXBN_SearchInstance_InUse, false);
	if ((_GameTick == -1) || (m_LastReleasedTick == -1))
		return m_bInUse;
	else
		return m_bInUse || (_GameTick == m_LastReleasedTick);
};


void CXBN_SearchInstance::Release(int _GameTick)
{
	MAUTOSTRIP(CXBN_SearchInstance_Release, MAUTOSTRIP_VOID);
	m_bInUse = false;
	m_LastReleasedTick = _GameTick;
};


CXBN_SearchInstance::CXBN_SearchInstance()
{
	MAUTOSTRIP(CXBN_SearchInstance_ctor, MAUTOSTRIP_VOID);
	m_pGrid = NULL;
	m_MaxNodes = 0;
	m_bInUse = false;
	m_LastReleasedTick = -1;
}

void CXBN_SearchInstance::Create(CXR_BlockNav_GridProvider* _pGrid, CXR_BlockNav* _pBlockNav, int _TileHeap, int _MaxNodes)
{
	MAUTOSTRIP(CXBN_SearchInstance_Create, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXBN_SearchInstance::Create);
	//Max nodes must be clamped at maximum of int16 to prevent overflow
	if (_MaxNodes > 32767)
	{
		#ifndef BLOCKNAV_VPU_COMPILE
				ConOutL("(CXBN_SearchInstance::Create) WARNING: Maxnodes cannot be greater than 32767.");
		#endif
		_MaxNodes = 32767;
	}
	m_MaxNodes = _MaxNodes;
	m_pGrid = _pGrid;
	m_pBlockNav	 = _pBlockNav;

	m_lNodeHeap.SetLen(m_MaxNodes);
	m_iNodeNext = 0;

	m_pDstNode = NULL;
	m_pLastNode = NULL;

	Hash_Clear();
}


bool CXBN_SearchInstance::Search_Create(const CXBN_SearchParams& _Params)
{
	MAUTOSTRIP(CXBN_SearchInstance_Search_Create, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXBN_SearchInstance::Search_Create);
	//Fail by default if navigation grid is invalid
	if (!m_pGrid)
	{
		return(false);
	};

	m_bInUse = true;

	m_PQueue.Create(false, m_MaxNodes);
	Hash_Clear();
	m_iNodeNext = 0;

	m_pDstNode = NULL;
	m_Params = _Params;

	//Transform source pos to grid-space. Cell coordinates are at lowest corner of cell space
	//so cell position should be truncated rather than rounded. Note that the source and/or 
	//destinations may be untraversable in the grid although they are not in the world. This 
	//is left to the user to compensate.
	CVec3Dfp32 SrcPos = m_Params.m_SrcPos;
	SrcPos -= m_pGrid->m_Origin;
	SrcPos *= m_pGrid->m_CellsPerUnit;
	m_Params.m_Src = CVec3Dint16(TruncToInt(SrcPos[0]), TruncToInt(SrcPos[1]), TruncToInt(SrcPos[2]));

//ConOutL(CStrF("Src %d,%d,%d",m_Params.m_Src[0],m_Params.m_Src[1],m_Params.m_Src[2]) + ", " + m_Params.m_SrcPos.GetString() );

	// Transform dest pos to grid-space.
	CVec3Dfp32 DstPos = m_Params.m_DstPos;
	DstPos -= m_pGrid->m_Origin;
	DstPos *= m_pGrid->m_CellsPerUnit;
	m_Params.m_Dst = CVec3Dint16(TruncToInt(DstPos[0]), TruncToInt(DstPos[1]), TruncToInt(DstPos[2]));

	//Set the base "square radius" of the entity
	fp32 sz = Max(m_Params.m_SizeWU[0], m_Params.m_SizeWU[1]);
	sz = Max(0.0f, (fp32)(sz - m_pGrid->m_UnitsPerCell));	// *** Will this ever change the value of sz? ***
	m_Params.m_iBaseSize = int16(M_Ceil(0.5f * sz * m_pGrid->m_CellsPerUnit - _FP32_EPSILON)); //The last epsilon is added due to inaccuracies that cause basesize to become too large

	//Set the height of the entity. This is the smallest height the entity can be, usually the crouching height.
	m_Params.m_iHeight = int16(M_Ceil(m_Params.m_SizeWU[2] * m_pGrid->m_CellsPerUnit - _FP32_EPSILON));

	//Set the full height of the entity. 
	m_Params.m_iFullHeight = int16(M_Ceil(m_Params.m_iFullHeightWU * m_pGrid->m_CellsPerUnit - _FP32_EPSILON));

	//If the source it outside the grid, the search should fail immediately
	if (IsOutsideGridBoundaries(m_Params.m_Src))
	{
		m_iCauseOfFailure = CXR_BlockNavSearcher::SEARCH_SOURCE_OUTOFBOUNDS;
		return(false);
	};

	//If the destination is outside the grid the search should fail immediately
	if (IsOutsideGridBoundaries(m_Params.m_Dst))
	{
		m_iCauseOfFailure = CXR_BlockNavSearcher::SEARCH_DESTINATION_OUTOFBOUNDS;
		return(false);
	};

	//If the destination is non-traversable, the search should fail immediately
	//unless we allow partials
	if (!IsTraversable(m_Params.m_Dst))
	{
		if (!m_Params.m_bPartial)
		{
			#ifndef BLOCKNAV_VPU_COMPILE
				ConOut("Search_Create cannot reach destination");
			#endif
			m_iCauseOfFailure = CXR_BlockNavSearcher::SEARCH_DESTINATION_INTRAVERSABLE;
			return(false);
		}
		else
		{	// Trace from _Params.m_DstPos to _Params.m_SrcPos for 2 * m_iBaseSize cells
			// If we find a traversable cell we stop and if not we fail
			CVec3Dfp32 Diff = m_Params.m_SrcPos - m_Params.m_DstPos;

			if (Abs(Diff.k[0]) >= Abs(Diff.k[1]))
			{	// Scale by 1/x
				Diff = Diff / Diff.k[0];
			}
			else
			{	// Scale by 1/y
				Diff = Diff / Diff.k[1];
			}
			CVec3Dfp32 CurPos;
			CurPos[0] = m_Params.m_Dst[0];
			CurPos[1] = m_Params.m_Dst[1];
			CurPos[2] = m_Params.m_Dst[2];
			int MaxSteps = int(sz * m_pGrid->m_CellsPerUnit * 2);
			bool validDest = false;
			/*
			for (int steps = 0; steps < MaxSteps; steps++)
			{
				CurPos += Diff;
				m_Params.m_Dst = CVec3Dint16(TruncToInt(CurPos[0]), TruncToInt(CurPos[1]), TruncToInt(CurPos[2]));
				if (IsTraversable(m_Params.m_Dst))
				{
					validDest = true;
					break;
				}
			}
			*/
			// *** The above approach doesn't work well when there's a wall between src and dst
			// Instead we do a couple of 8 facing expanding checks
			CVec3Dint16 TestDst;
			for (int steps = 1; steps <= MaxSteps; steps++)
			{
				// +X
				TestDst = m_Params.m_Dst;
				TestDst[0] += steps;
				if (IsTraversable(TestDst))
				{
					validDest = true;
					m_Params.m_Dst = TestDst;
					break;
				}
				// -X
				TestDst = m_Params.m_Dst;
				TestDst[0] -= steps;
				if (IsTraversable(TestDst))
				{
					validDest = true;
					m_Params.m_Dst = TestDst;
					break;
				}
				// +Y
				TestDst = m_Params.m_Dst;
				TestDst[1] += steps;
				if (IsTraversable(TestDst))
				{
					validDest = true;
					m_Params.m_Dst = TestDst;
					break;
				}
				// -Y
				TestDst = m_Params.m_Dst;
				TestDst[1] -= steps;
				if (IsTraversable(TestDst))
				{
					validDest = true;
					m_Params.m_Dst = TestDst;
					break;
				}

				// +X+Y
				TestDst = m_Params.m_Dst;
				TestDst[0] += steps;
				TestDst[1] += steps;
				if (IsTraversable(TestDst))
				{
					validDest = true;
					m_Params.m_Dst = TestDst;
					break;
				}
				// +X-Y
				TestDst = m_Params.m_Dst;
				TestDst[0] += steps;
				TestDst[1] -= steps;
				if (IsTraversable(TestDst))
				{
					validDest = true;
					m_Params.m_Dst = TestDst;
					break;
				}
				// -X+Y
				TestDst = m_Params.m_Dst;
				TestDst[0] -= steps;
				TestDst[1] += steps;
				if (IsTraversable(TestDst))
				{
					validDest = true;
					m_Params.m_Dst = TestDst;
					break;
				}
				// -X-Y
				TestDst = m_Params.m_Dst;
				TestDst[0] -= steps;
				TestDst[1] -= steps;
				if (IsTraversable(TestDst))
				{
					validDest = true;
					m_Params.m_Dst = TestDst;
					break;
				}
			}
			if (!validDest)
			{
				#ifndef BLOCKNAV_VPU_COMPILE
					ConOut("Search_Create cannot reach destination after tracing 2 widths");
				#endif
				m_iCauseOfFailure = CXR_BlockNavSearcher::SEARCH_DESTINATION_INTRAVERSABLE;
				return(false);
			}
		}
	}

	//Although the source node might be intraversable (due to grid resolution)
	//it might have children that are not. Therefore, the search need not fail
	//automatically if the source is intraversable, but if it fails immediately
	//it should be known that this was the cause
	if (!IsTraversable(m_Params.m_Src))
		m_iCauseOfFailure = CXR_BlockNavSearcher::SEARCH_SOURCE_INTRAVERSABLE;
	else
		//If the source and destination is ok, then the only possible way for 
		//the search to fail is if there is no path, or the start node couldn't
		//be created, which will be checked below
		m_iCauseOfFailure = CXR_BlockNavSearcher::SEARCH_NO_PATH;


//ConOutL(CStrF("Dst %d,%d,%d",m_Params.m_Dst[0],m_Params.m_Dst[1],m_Params.m_Dst[2]) + ", " + m_Params.m_DstPos.GetString() );

	// Create source node an push it on the OPEN list
	CXBN_SearchNode* pN = NewNode();
	if (!pN) 
	{
		m_iCauseOfFailure = CXR_BlockNavSearcher::SEARCH_NODE_ALLOC_FAIL;
		return(false);
	};

	int Distance;
	if (m_Params.m_bWallClimb)
	{
		Distance = RoundToInt(WBN_DISTANCEFACTOR*Length3(WBN_COSTFRACTIONS*(m_Params.m_Dst[0] - m_Params.m_Src[0]), 
								WBN_COSTFRACTIONS*(m_Params.m_Dst[1] - m_Params.m_Src[1]),
								WBN_COSTFRACTIONS*(m_Params.m_Dst[2] - m_Params.m_Src[2])));
	}
	else
	{
		Distance = RoundToInt(WBN_DISTANCEFACTOR*Length2(WBN_COSTFRACTIONS*(m_Params.m_Dst[0] - m_Params.m_Src[0]), 
								WBN_COSTFRACTIONS*(m_Params.m_Dst[1] - m_Params.m_Src[1])));
	}
	pN->Construct();
	if (!IsOnGround(m_Params.m_Src))
	{
		pN->m_Flags |= WBN_NODE_AIRBORNE;
	}
	pN->m_Distance = Distance;
	pN->m_Pos[0] = m_Params.m_Src[0];
	pN->m_Pos[1] = m_Params.m_Src[1];
	pN->m_Pos[2] = m_Params.m_Src[2];


//ConOutL(CStrF("m_spNodeGrid->GetWriteableCell( %d, %d, %d )", m_Params.m_Src[0], m_Params.m_Src[1], m_Params.m_Src[2]));

	m_pLastNode = pN;
	Hash_InsertNode(pN->m_iNode);
	m_PQueue.Push(pN);

	m_nExpandedNodes = 0;
	return(true);
}


int CXBN_SearchInstance::Search_Execute(int& _nNodeExpansions, int _nMaxExpansions,int _nMaxSlop)
{
	MAUTOSTRIP(CXBN_SearchInstance_Search_Execute, 0);
	MSCOPESHORT(CXBN_SearchInstance::Search_Execute);
	if (m_pDstNode)
	{
		return CXR_BlockNavSearcher::SEARCH_DONE;
	}

	//Fail by default if navigation grid or blocknav is invalid 
	if (!m_pGrid || !m_pBlockNav)
	{
		return CXR_BlockNavSearcher::SEARCH_NO_NAVGRID;
	};

	//Always expand at least two nodes. This criterium only exist
	//to make sure we can check if the search failed due to an
	//intraversable start node.
	if (_nNodeExpansions < 2)
		_nNodeExpansions = 2;

	bool bCheckMax = _nMaxExpansions > 0;

	CXBN_SearchNode* pN = m_PQueue.Pop();
	while(pN)
	{
		//Are we close enough to destination?
		if (Abs(pN->m_Pos[0] - m_Params.m_Dst[0]) <= _nMaxSlop &&
			Abs(pN->m_Pos[1] - m_Params.m_Dst[1]) <= _nMaxSlop &&
			Abs(pN->m_Pos[2] - m_Params.m_Dst[2]) <= _nMaxSlop)
		{
			// Yeaaaaaaahhh..  we're home
			//ConOutL("Found path");
			m_pDstNode = pN;
			return CXR_BlockNavSearcher::SEARCH_DONE;
		}

		//Expand node
		PushChildren(pN);
		_nNodeExpansions--;
		m_nExpandedNodes++;
		if (m_iCauseOfFailure == CXR_BlockNavSearcher::SEARCH_NODE_ALLOC_FAIL)
		{	// No more nodes, we bail
			return m_iCauseOfFailure;
		}

		if (bCheckMax && (m_nExpandedNodes > _nMaxExpansions))
		{
			//We've reached max expansions limit
			return CXR_BlockNavSearcher::SEARCH_NO_PATH;
		}

		if (_nNodeExpansions <= 0)
		{
			// Break search here..
			return CXR_BlockNavSearcher::SEARCH_IN_PROGRESS;
		}

		m_pLastNode = pN;
		pN = m_PQueue.Pop();

		if (pN && (m_iCauseOfFailure == CXR_BlockNavSearcher::SEARCH_SOURCE_INTRAVERSABLE))
			//Source was definitly traversable, the default cause of failure is no path
			m_iCauseOfFailure = CXR_BlockNavSearcher::SEARCH_NO_PATH;
	}
	
	// Didn't find a path..  :(
	return m_iCauseOfFailure;
}


//Returns true if search is done and sets _Res to the search result
//If search is not done, false is returned and if _ReturnPartial is true
//then _Res is set to the current best partial path
bool CXBN_SearchInstance::Search_Get(CXBN_SearchResultCore* _pRes, bool _ReturnPartial)
{
	/*
#ifdef PLATFORM_VPU
	return false;
#else*/

	MAUTOSTRIP(CXBN_SearchInstance_Search_Get, false);
	MSCOPESHORT(CXBN_SearchInstance::Search_Get);
	//Fail by default if navigation grid is invalid
	if (!m_pGrid || !_pRes)
	{
		return false;
	};

	//Is search done?
	if(m_pDstNode)
	{
		//Search is done, create _pRes with path ending in destination node (m_pDstNode) and return true
		_pRes->Create(m_pDstNode, this);
		return true;		
	}
	else
	{
		//Search is not done, should we return partial result?
		if (_ReturnPartial) 
		{
			//Create _pRes with path ending in node at least a bit away from origin of search
			//and closest to the destination of a number of sample nodes. 
			int16 Threshold = 32*32;
			int16 iCloseToDest = -1;
			uint16 MinDistDest = 32767;
			int16 iFarFromOrg = -1;
			int MaxDistOrgSqr = 0;
			int DistOrgSqr;

			//First sample node is next node to be examined 
			CXBN_SearchNode* pN = m_PQueue.Pop();
			if (pN && ((DistOrgSqr = pN->m_Pos.DistanceSqr(m_Params.m_Src)) > Threshold))
			{
				iCloseToDest = pN->m_iNode;
				MinDistDest = pN->m_Distance;
				iFarFromOrg = pN->m_iNode;
				MaxDistOrgSqr = DistOrgSqr;
			}
			
			 //Second sample node is last visited node
			if (m_pLastNode && ((DistOrgSqr = m_pLastNode->m_Pos.DistanceSqr(m_Params.m_Src)) > Threshold))
			{
				if (m_pLastNode->m_Distance < MinDistDest)
				{
					iCloseToDest = m_pLastNode->m_iNode;
					MinDistDest = m_pLastNode->m_Distance;
				}
				if (DistOrgSqr > MaxDistOrgSqr)
				{
					iFarFromOrg = m_pLastNode->m_iNode;
					MaxDistOrgSqr = DistOrgSqr;
				}
			}
			
			//Sample a bunch of nodes 
			int nNodes = Min(m_iNodeNext, m_lNodeHeap.Len()) - 1;
			int Step = (nNodes / 20) + 1; //Try max 20 nodes
			for (int i = 5; i < nNodes; i += Step) //First couple of nodes cannot be interesting, due to threshold, so start at index 5 
			{
				//Is node valid (all should be)?
				pN = &(m_lNodeHeap[i]);
				if (pN->m_Pos[0] != -12345)
				{
					DistOrgSqr = pN->m_Pos.DistanceSqr(m_Params.m_Src);
					if (DistOrgSqr > Threshold)
					{
						if (pN->m_Distance < MinDistDest)
						{
							iCloseToDest = pN->m_iNode;
							MinDistDest = pN->m_Distance;
						}
						if (DistOrgSqr > MaxDistOrgSqr)
						{
							iFarFromOrg = pN->m_iNode;
							MaxDistOrgSqr = DistOrgSqr;
						}
					}
				}
			}

			//Did we find a best node (i.e. a node closer to destination than origin)?
			if ((iCloseToDest != -1) && (MinDistDest < m_lNodeHeap[0].m_Distance))
			{
				_pRes->Create(&(m_lNodeHeap[iCloseToDest]), this, true);
			}
			//If no best node was found, just use the one farthest away from origin
			else if (iFarFromOrg != -1)
			{
				_pRes->Create(&(m_lNodeHeap[iFarFromOrg]), this, true);
			}
			else 
			{
				//Can't find any usable node (none beyond threshold)
				_pRes->Reset();
			}
			return false;
		}
		else
			return false;
	};
};

int CXBN_SearchInstance::Search_GetUser()
{
	MAUTOSTRIP(CXBN_SearchInstance_Search_GetUser, 0);
	if (m_bInUse)
		return m_Params.m_iUser;
	else
		return 0;
};

void CXBN_SearchInstance::Debug_RenderNode(CWireContainer* _pWC, CXBN_SearchNode* _pN)
{
#ifndef BLOCKNAV_VPU_COMPILE
	MAUTOSTRIP(CXBN_SearchInstance_Debug_RenderNode, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXBN_SearchInstance::Debug_RenderNode);
	CVec3Dfp32 Pos = m_pGrid->m_Origin + m_pGrid->GetCellPosition(CVec3Dint32(_pN->m_Pos[0], _pN->m_Pos[1], _pN->m_Pos[2]));
	//Pos += PlayerOffs;
//	const fp32 Size = 8;
	CPixel32 Color = (_pN->m_Flags & WBN_PARENT_NODE_JUMP) ? 0xff007f00 : (_pN->m_Flags & WBN_NODE_AIRBORNE) ? 0xff007f7f : 0xff00007f;

	if (_pN->m_pParent)
	{
		CXBN_SearchNode* _pNP = _pN->m_pParent;
		CVec3Dfp32 PosParent = m_pGrid->m_Origin + m_pGrid->GetCellPosition(CVec3Dint32(_pNP->m_Pos[0], _pNP->m_Pos[1], _pNP->m_Pos[2]));
		//PosParent += PlayerOffs;
		
		//The positions are at the (xmin, ymin, zmin) corner of the cell.
		//The wire should be drawn through the center of the cell; 
		//thus the positions are shifted one half of a cell side along all axes
		CVec3Dfp32 Offset = CVec3Dfp32(m_pGrid->m_UnitsPerCell / 2);
		_pWC->RenderWire(Pos + Offset, PosParent + Offset, Color, 0.0f, false);
	}
#endif
}

void CXBN_SearchInstance::Debug_Render(CWireContainer* _pWC)
{
#ifndef BLOCKNAV_VPU_COMPILE
	MAUTOSTRIP(CXBN_SearchInstance_Debug_Render, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXBN_SearchInstance::Debug_Render);
	if (!m_pDstNode)
	{
		for(int i = 0; i < m_iNodeNext; i++)
			Debug_RenderNode(_pWC, &m_lNodeHeap[i]);
	}
	else
	{
		CXBN_SearchNode* pN = m_pDstNode;
		while(pN)
		{
			Debug_RenderNode(_pWC, pN);
			pN = pN->m_pParent;
		}
	}
#endif
}

// -------------------------------------------------------------------
//  CXR_BlockNav
// -------------------------------------------------------------------
CXR_BlockNav::CXR_BlockNav()
{
	MAUTOSTRIP(CXR_BlockNav_ctor, MAUTOSTRIP_VOID);
	// m_pWServer = NULL;
	m_pGrid = NULL;
}

CXR_BlockNav::~CXR_BlockNav()
{

}

void CXR_BlockNav::Create(class CXR_BlockNav_GridProvider *_pGrid)
{
	m_pGrid = _pGrid;
}

int CXR_BlockNav::GetCellValue(int _x, int _y, int _z)
{
	MAUTOSTRIP(CXR_BlockNav_GetCellValue, 0);
	//Fail by default if navigation grid is invalid
	if (!m_pGrid)
		return 0;

	return m_pGrid->GetAt(CVec3Dint32(_x, _y, _z));
};


//Helper
bool CXR_BlockNav::IsOutsideGridBoundariesImpl(const CVec3Dint16& _Pos, int _iBaseSize, int _iHeight)
{
	MAUTOSTRIP(CXR_BlockNav_IsOutsideGridBoundariesImpl, false);
	MSCOPESHORT(CXR_BlockNav::IsOutsideGridBoundariesImpl);
	//If navigation grid is invalid then we are always outside it's boundaries
	if (!m_pGrid)
		return true;
	
	return (_Pos[0] < _iBaseSize || _Pos[1] < _iBaseSize || _Pos[2] < 0 ||
			_Pos[0] > m_pGrid->m_CellGridDim[0] - 1 - _iBaseSize ||
			_Pos[1] > m_pGrid->m_CellGridDim[1] - 1 - _iBaseSize ||
			_Pos[2] > m_pGrid->m_CellGridDim[2] - _iHeight);
};


//Checks if the given position is outside or too close to the grid boundaries.
//The _iBaseSize and _iHeight arguments are the base side length of the entitys ounding box and it's
//height in world units.
bool CXR_BlockNav::IsOutsideGridBoundaries(const CVec3Dint16& _Pos, int _iBaseSize, int _iHeight)
{
	MAUTOSTRIP(CXR_BlockNav_IsOutsideGridBoundaries, false);
	MSCOPESHORT(CXR_BlockNav::IsOutsideGridBoundaries);
	//If navigation grid is invalid then we are always outside it's boundaries
	if (!m_pGrid)
		return true;
	
	//Set the base "square radius" of the entity
	fp32 sz = Max(0.0f, (fp32)(_iBaseSize - m_pGrid->m_UnitsPerCell));
	_iBaseSize = int(M_Ceil(0.5f * sz * m_pGrid->m_CellsPerUnit - _FP32_EPSILON));

	//Set the height of the entity. This is the smallest height the entity can be, usually the crouching height.
	_iHeight = int(M_Ceil(_iHeight * m_pGrid->m_CellsPerUnit - _FP32_EPSILON));

	return IsOutsideGridBoundariesImpl(_Pos, _iBaseSize, _iHeight);
};


//Checks if the node specified by _Pos is traversable. The optional _dx, _dy and _dz arguments
//are used to indicate that we are checking to see if we can move from the node at 
//_Pos + (_dx, _dy. _dz) to this node, which means that several cells have probably already 
//been checked for traversability, and some for traversability, thus there are fewer cells that 
//need be checked (since the previously checked node have several overlapping cells with this node)
//The _iBaseSize and _iHeight arguments are the base side length of the entitys ounding box and it's
//height in world units.
bool CXR_BlockNav::IsTraversable(const CVec3Dint16& _Pos, int _iBaseSize, int _iHeight, bool _bClimb, int _dx, int _dy, int _dz)
{
	MAUTOSTRIP(CXR_BlockNav_IsTraversable, false);
	MSCOPESHORT(CXR_BlockNav::IsTraversable);
	//Fail by default if navigation grid is invalid or given position is outside grid boundaries
	if (!m_pGrid || IsOutsideGridBoundaries(_Pos, int(_iBaseSize + m_pGrid->m_UnitsPerCell * Max(Abs(_dx), Abs(_dy))), int(_iHeight + m_pGrid->m_UnitsPerCell * Abs(_dz))))
		return false;
	
	//Set the base "square radius" of the entity
	fp32 sz = Max(0.0f, (fp32)(_iBaseSize - m_pGrid->m_UnitsPerCell));
	_iBaseSize = int(M_Ceil(0.5f * sz * m_pGrid->m_CellsPerUnit - _FP32_EPSILON));

	//Set the height of the entity. This is the smallest height the entity can be, usually the crouching height.
	_iHeight = int(M_Ceil(_iHeight * m_pGrid->m_CellsPerUnit - _FP32_EPSILON));

	//Fail by default if navigation grid is invalid or given position is outside grid boundaries
	if (!m_pGrid || IsOutsideGridBoundariesImpl(_Pos, _iBaseSize + Max(Abs(_dx), Abs(_dy)), _iHeight + Abs(_dz)))
		return false;

	return IsTraversableImpl(_Pos, _iBaseSize, _iHeight, _bClimb, _dx, _dy, _dz);
};


//Helper to IsTraversableImpl
bool CXR_BlockNav::Is3x3x4Traversable(bool _bClimb, int _x, int _y, int _z)
{
	MAUTOSTRIP(CXR_BlockNav_Is3x3x4Traversable, false);
	MSCOPESHORT(CXR_BlockNav::Is3x3x4Traversable);
	//Currently designed to work for maps compiled with or without the traversable flag. 
	//This is inefficient for those without the traversable flag of course. 

	CXR_BlockNav_GridProvider* M_RESTRICT pGrid = m_pGrid;

	if (GETATXYZ(pGrid, _x, _y, _z) & XR_CELL_TRAVERSABLE)
	{
		return true;
	}
	else
	{
		if ((_bClimb)&&(GETATXYZ(pGrid, _x, _y, _z) & XR_CELL_WALL))
		{
			return true;
		}

		for (int x = _x - CHARSIZE; x <= _x + CHARSIZE; x++)
		{
			for (int y = _y - CHARSIZE; y <= _y + CHARSIZE; y++)
			{
				for (int z = _z; z < _z + CHARHEIGHT; z++)
				{
					int Value = GETATXYZ(pGrid, x, y, z);
					if (!(Value & XR_CELL_AIR) || (Value & XR_CELL_DIE)) 
					{
						return false;
					}
				};
			};
		};
		//All cells are traversable
		return true;
	}
};


//Helper to IsTraversableImpl for critters smaller than a normal character
bool CXR_BlockNav::IsTraversableSmall(const CVec3Dint16& _Pos, int _iBaseSize, int _iHeight, bool _bClimb, int _dx, int _dy, int _dz)
{
	MAUTOSTRIP(CXR_BlockNav_IsTraversableSmall, false);
	MSCOPESHORT(CXR_BlockNav::IsTraversableSmall);
	//Iterators
	int x, y, z;
	CXR_BlockNav_GridProvider* M_RESTRICT pGrid = m_pGrid;

	if (!(_dx | _dy | _dz)) 
	{
		if ((_bClimb)&&(GETATXYZ(pGrid, _Pos[0], _Pos[1], _Pos[2]) & XR_CELL_WALL))
		{
			return true;
		}

		//_dx, _dy and -dz are 0: default checking of the cells around the position only
		for(x = _Pos[0] - _iBaseSize; x <= _Pos[0] + _iBaseSize; x++)
			for(y = _Pos[1] - _iBaseSize; y <= _Pos[1] + _iBaseSize; y++)
				for(z = _Pos[2]; z < _Pos[2] + _iHeight; z++)
				{
					int Value = GETATXYZ(pGrid, x, y, z);
					/*				
					if (_bClimb)
					{
						if (Value & XR_CELL_WALL)
						{
							return true;
						}
					}
					*/
					if (!(Value & XR_CELL_AIR)||(Value & XR_CELL_DIE))
					{
						//Non-traversable cell!
						return false;
					}
				};

		//All cells are traversable!
		return true;
	}
	else
	{
		//Although not always necessary, I.ll make sure all cells in the box 
		//described by the extreme points of the previous and current node are 
		//checked, except for any intermediate cells below the current node.
		//Some of the cells we need to check has already been checked though, 
		//usually most of them :) 
	
		//Loop boundaries
		int MinX, MaxX, MinY, MaxY, MinZ, MaxZ;

		//First check all cells where there's no overlap in the xy-plane between 
		//the previous and current node, except for any cells in the current node
		//which are below the previous node.
		if (_dz > 0)
		{	
			MinZ = _Pos[2] + _dz;
			MaxZ = _Pos[2] + _dz + _iHeight; 
			
		} else //_dz <= 0
		{
			MinZ = _Pos[2];
			MaxZ = _Pos[2] + _iHeight;
		};

		//Check columns
		if (_dx)
		{
			if (_dx > 0) 
			{	
				MinX = _Pos[0] - _iBaseSize;
				MaxX = _Pos[0] + _dx - _iBaseSize - 1;
			}
			else //_dx < 0
			{
				MinX = _Pos[0] + _dx + _iBaseSize + 1;
				MaxX = _Pos[0] + _iBaseSize;
			};
			if (_dy > 0)
			{
				MinY = _Pos[1] - _iBaseSize;
				MaxY = _Pos[1] + _dy + _iBaseSize;
			}
			else //_dy <= 0
			{
				MinY = _Pos[1] + _dy - _iBaseSize;
				MaxY = _Pos[1] + _iBaseSize;
			};

// ConOut("Checking columns...");
// ConOut(CStrF("Checking Cells (%d-%d,%d-%d,%d-%d)", MinX, MaxX, MinY, MaxY, MinZ, MaxZ-1));
		
			for(x = MinX; x <= MaxX; x++)
				for(y = MinY; y <= MaxY; y++)
					for(z = MinZ; z < MaxZ; z++)
					{
						int Value = GETATXYZ(pGrid, x, y, z);
						if (_bClimb)
						{
							if (Value & XR_CELL_WALL)
							{
								return true;
							}
						}
						if (!(Value & XR_CELL_AIR)||(Value & XR_CELL_DIE))
						{
							//Non-traversable cell!
							return false;
						}
					};
		};

		//Check rows, except any cells which have already been checked in the previous statement 
		if (_dy)
		{
			MinX = _Pos[0] + _dx - _iBaseSize;
			MaxX = _Pos[0] + _dx + _iBaseSize;
			if (_dy > 0)
			{
				MinY = _Pos[1] - _iBaseSize;
				MaxY = _Pos[1] + _dy - _iBaseSize - 1;
			} else //_dy < 0
			{
				MinY = _Pos[1] + _dy + _iBaseSize + 1;
				MaxY = _Pos[1] + _iBaseSize;
			};

// ConOut("Checking rows...");
// ConOut(CStrF("Checking Cells (%d-%d,%d-%d,%d-%d)", MinX, MaxX, MinY, MaxY, MinZ, MaxZ-1));
			for(x = MinX; x <= MaxX; x++)
				for(y = MinY; y <= MaxY; y++)
					for(z = MinZ; z < MaxZ; z++)
					{
						int Value = GETATXYZ(pGrid, x, y, z);
						if (_bClimb)
						{
							if (Value & XR_CELL_WALL)
							{
								return true;
							}
						}
						if (!(Value & XR_CELL_AIR)||(Value & XR_CELL_DIE))
						{
							//Non-traversable cell!
							return false;
						}
					};
		};

		//If these cells were ok, then check xy- but non-xz- and yz-plane overlapping cells 
		//and cells below the previous node. 
		if (_dz)
		{
			if (_dz > 0)
			{
				//We have descended, check all cells in the current node below the previous node
				MinX = _Pos[0] - _iBaseSize;
				MaxX = _Pos[0] + _iBaseSize;
				MinY = _Pos[1] - _iBaseSize;
				MaxY = _Pos[1] + _iBaseSize;
				MinZ = _Pos[2];
				MaxZ = _Pos[2] + _dz;
			} else //_dz < 0
			{
				//We have ascended, check all cells above the previous node that are lower than 
				//the top of the current node.
				MinX = _Pos[0] + _dx - _iBaseSize;
				MaxX = _Pos[0] + _dx + _iBaseSize;
				MinY = _Pos[1] + _dy - _iBaseSize;
				MaxY = _Pos[1] + _dy + _iBaseSize;
				MinZ = _Pos[2] + _dz + _iHeight;
				MaxZ = _Pos[2] + _iHeight;
			};

// ConOut("Checking top/bottom...");
// ConOut(CStrF("Checking Cells (%d-%d,%d-%d,%d-%d)", MinX, MaxX, MinY, MaxY, MinZ, MaxZ-1));
			for(x = MinX; x <= MaxX; x++)
				for(y = MinY; y <= MaxY; y++)
					for(z = MinZ; z < MaxZ ; z++)
					{
						int Value = GETATXYZ(pGrid, x, y, z);
						if (_bClimb)
						{
							if (Value & XR_CELL_WALL)
							{
								return true;
							}
						}
						if (!(Value & XR_CELL_AIR)||(Value & XR_CELL_DIE))
						{
							//Non-traversable cell!
							return false;
						}
					};
		};
		
		//All non-overlapping cells are traversable!
		return true;
	};
};


//Helper to IsTraversableImpl for critters bigger than a normal character
bool CXR_BlockNav::IsTraversableBig(const CVec3Dint16& _Pos, int _iBaseSize, int _iHeight, bool _bClimb, int _dx, int _dy, int _dz)
{
	MAUTOSTRIP(CXR_BlockNav_IsTraversableBig, false);
	MSCOPESHORT(CXR_BlockNav::IsTraversableBig);
	//Larger than normal character
	//Iterators
	int x, y, z;
	CXR_BlockNav_GridProvider* M_RESTRICT pGrid = m_pGrid;

	if (!(_dx | _dy | _dz)) 
	{
		//_dx, _dy and -dz are 0: default checking of the cells around the position only
		int MaxX = _Pos[0] + _iBaseSize;
		int MaxY = _Pos[1] + _iBaseSize;
		int MaxZ = _Pos[2] + _iHeight - 1;
		for (x = _Pos[0] - _iBaseSize + CHARSIZE; x <= MaxX - CHARSIZE; x += Max(1, Min((int)CHARSIDESIZE, MaxX - x - CHARSIZE)))
			for (y = _Pos[1] - _iBaseSize + CHARSIZE; y <= MaxY - CHARSIZE; y += Max(1, Min((int)CHARSIDESIZE, MaxY - y - CHARSIZE)))
				for (z = _Pos[2]; z < MaxZ - CHARHEIGHT + 2; z += Max(1, Min((int)CHARHEIGHT, MaxZ - z - CHARHEIGHT + 1)))
				{
					if (!Is3x3x4Traversable(_bClimb, x, y, z))
						return false;
				};

		//All cells are traversable!
		return true;
	} 
	else
	{
		//This part isn't optimized yet! FIX!

		//Although not always necessary, I.ll make sure all cells in the box 
		//described by the extreme points of the previous and current node are 
		//checked, except for any intermediate cells below the current node.
		//Some of the cells we need to check has already been checked though, 
		//usually most of them :) 
	
		//Loop boundaries
		int MinX, MaxX, MinY, MaxY, MinZ, MaxZ;

		//First check all cells where there's no overlap in the xy-plane between 
		//the previous and current node, except for any cells in the current node
		//which are below the previous node.
		if (_dz > 0)
		{	
			MinZ = _Pos[2] + _dz;
			MaxZ = _Pos[2] + _dz + _iHeight; 
			
		}
		else //_dz <= 0
		{
			MinZ = _Pos[2];
			MaxZ = _Pos[2] + _iHeight;
		};

		//Check columns
		if (_dx)
		{
			if (_dx > 0) 
			{	
				MinX = _Pos[0] - _iBaseSize;
				MaxX = _Pos[0] + _dx - _iBaseSize - 1;
			}
			else //_dx < 0
			{
				MinX = _Pos[0] + _dx + _iBaseSize + 1;
				MaxX = _Pos[0] + _iBaseSize;
			};
			if (_dy > 0)
			{
				MinY = _Pos[1] - _iBaseSize;
				MaxY = _Pos[1] + _dy + _iBaseSize;
			}
			else //_dy <= 0
			{
				MinY = _Pos[1] + _dy - _iBaseSize;
				MaxY = _Pos[1] + _iBaseSize;
			};

// ConOut("Checking columns...");
// ConOut(CStrF("Checking Cells (%d-%d,%d-%d,%d-%d)", MinX, MaxX, MinY, MaxY, MinZ, MaxZ-1));

			for(x = MinX; x <= MaxX; x++)
			{
				for(y = MinY; y <= MaxY; y++)
				{
					for(z = MinZ; z < MaxZ; z++)
					{
						int Value = GETATXYZ(pGrid, x, y, z);
						if (_bClimb)
						{
							if (Value & XR_CELL_WALL)
							{
								return true;
							}
						}
						if (!(Value & XR_CELL_AIR)||(Value & XR_CELL_DIE))
						{
							//Non-traversable cell!
							return false;
						}
					}
				}
			}
		};

		//Check rows, except any cells which have already been checked in the previous statement 
		if (_dy)
		{
			MinX = _Pos[0] + _dx - _iBaseSize;
			MaxX = _Pos[0] + _dx + _iBaseSize;
			if (_dy > 0)
			{
				MinY = _Pos[1] - _iBaseSize;
				MaxY = _Pos[1] + _dy - _iBaseSize - 1;
			}
			else //_dy < 0
			{
				MinY = _Pos[1] + _dy + _iBaseSize + 1;
				MaxY = _Pos[1] + _iBaseSize;
			};

// ConOut("Checking rows...");
// ConOut(CStrF("Checking Cells (%d-%d,%d-%d,%d-%d)", MinX, MaxX, MinY, MaxY, MinZ, MaxZ-1));
			for(x = MinX; x <= MaxX; x++)
				for(y = MinY; y <= MaxY; y++)
					for(z = MinZ; z < MaxZ; z++)
					{
						int Value = GETATXYZ(pGrid, x, y, z);
						if (_bClimb)
						{
							if (Value & XR_CELL_WALL)
							{
								return true;
							}
						}
						if (!(Value & XR_CELL_AIR)||(Value & XR_CELL_DIE))
						{
							//Non-traversable cell!
							return false;
						}
					};
		};

		//If these cells were ok, then check xy- but non-xz- and yz-plane overlapping cells 
		//and cells below the previous node. 
		if (_dz)
		{
			if (_dz > 0)
			{
				//We have descended, check all cells in the current node below the previous node
				MinX = _Pos[0] - _iBaseSize;
				MaxX = _Pos[0] + _iBaseSize;
				MinY = _Pos[1] - _iBaseSize;
				MaxY = _Pos[1] + _iBaseSize;
				MinZ = _Pos[2];
				MaxZ = _Pos[2] + _dz;
			}
			else //_dz < 0
			{
				//We have ascended, check all cells above the previous node that are lower than 
				//the top of the current node.
				MinX = _Pos[0] + _dx - _iBaseSize;
				MaxX = _Pos[0] + _dx + _iBaseSize;
				MinY = _Pos[1] + _dy - _iBaseSize;
				MaxY = _Pos[1] + _dy + _iBaseSize;
				MinZ = _Pos[2] + _dz + _iHeight;
				MaxZ = _Pos[2] + _iHeight;
			};

// ConOut("Checking top/bottom...");
// ConOut(CStrF("Checking Cells (%d-%d,%d-%d,%d-%d)", MinX, MaxX, MinY, MaxY, MinZ, MaxZ-1));
			for(x = MinX; x <= MaxX; x++)
				for(y = MinY; y <= MaxY; y++)
					for(z = MinZ; z < MaxZ ; z++)
					{
						int Value = GETATXYZ(pGrid, x, y, z);
						if (_bClimb)
						{
							if (Value & XR_CELL_WALL)
							{
								return true;
							}
						}
						if (!(Value & XR_CELL_AIR)||(Value & XR_CELL_DIE))
						{
							//Non-traversable cell!
							return false;
						}
					};
		};
		
		//All non-overlapping cells are traversable!
		return true;
	};
};


//Helper to IsTraversable; checks traversability for movement in xy-plane for normal sized characters
//Params are always the position to check to, and the diff to a previous node that already have been checked
bool CXR_BlockNav::IsTraversableLevel(bool _bClimb, int16 _x, int16 _y, int16 _z, int16 _dx, int16 _dy)
{
	MAUTOSTRIP(CXR_BlockNav_IsTraversableLevel, false);
	MSCOPESHORT(CXR_BlockNav::IsTraversableLevel);
	//Although not strictly necessary I'll check all positions spanned by the 
	//of the previous and current position. Since this is almost always the case
	//I've optimized the algorithm for the case when there's no movement longer than 
	//the base size of the character, and use brute force otherwise
	int XDiff = Abs(_dx);
	int YDiff = Abs(_dy);
	if ((XDiff <= CHARSIDESIZE) && (YDiff <= CHARSIDESIZE))
	{
		//Check current position
		if (!Is3x3x4Traversable(_bClimb, _x, _y, _z))
			return false;

		if (_dx != 0)
		{
			//Check position at same y-coord as current
			if (!Is3x3x4Traversable(_bClimb, _x + _dx, _y, _z))
				return false;
		}

		if (_dy != 0)
		{
			//Check position at same x-coord as current
			if (!Is3x3x4Traversable(_bClimb, _x, _y + _dy, _z))
				return false;
		}

		//All check successful
		return true;
	}
	else
	{
		//This checks previous position as well, which is unneccessary... fix
		int xdir = (_dx < 0) ? -1 : 1;
		int ydir = (_dy < 0) ? -1 : 1;
		for (int dx = 0; dx <= XDiff; dx += Max(1, Min((int)CHARSIDESIZE, XDiff - dx)))
			for (int dy = 0; dy <= YDiff; dy += Max(1, Min((int)CHARSIDESIZE, YDiff - dy)))
			{
				if (!Is3x3x4Traversable(_bClimb, _x + (xdir * dx), _y + (ydir * dy), _z))
					return false;
			}
		//All checks successful
		return true;
	}
};


//Helper to IsTraversable; checks vertical traversability for normal sized characters
//Params are always the position to check to, and the diff to a previous node that already have been checked
bool CXR_BlockNav::IsTraversableVertical(bool _bClimb, int16 _x, int16 _y, int16 _z, int16 _dz)
{
	MAUTOSTRIP(CXR_BlockNav_IsTraversableVertical, false);
	MSCOPESHORT(CXR_BlockNav::IsTraversableVertical);
	//Check position we move to first then any intermediates in the "chimney"
	int Diff = Abs(_dz);
	int Dir = (_dz < 0) ? -1 : 1;
	for (int dz = 0; dz < Diff; dz += CHARHEIGHT)
	{
		if (!Is3x3x4Traversable(_bClimb, _x, _y, _z + (Dir * dz)))
			//Found untraversable position
			return false;
	}
	//All positions traversable
	return true;
};


//Checks traversability while assuming that the given coordinates are within the grid. Base size and height are in cells.
bool CXR_BlockNav::IsTraversableImpl(const CVec3Dint16& _Pos, int _iBaseSize, int _iHeight, bool _bClimb, int _dx, int _dy, int _dz)
{	
	MAUTOSTRIP(CXR_BlockNav_IsTraversableImpl, false);
	MSCOPESHORT(CXR_BlockNav::IsTraversableImpl);
	//Check the cells around a node. If an entity has a basesize of 1 then the following figure 
	//shows which cells need to be checked:
	// xy-plane:			xz- and yz-plane:
	//			XXX								XXX
	//			XOX								XXX
	//			XXX								XXX
	//			 								XOX
	//where the cell marked with an O is the node center. If we are moving from one node to another
	//then some cells need not be checked again, since they were checked at the previous node, but 
	//some extra cells need to be checked to insure that the entitys corresponding bounding box 
	//can pass from the first to the second node without colliding with anything. In the example 
	//the previous node center is marked with an 1 and the cells already checked with -. This node 
	//center is marked with a 2 and the cells that need to be checked are marked with X for the normal 
	//cells and Z for those extra cells that need to be checked.
	// xy-plane:			xz- and yz-plane:
	//			ZXXX							ZXXX
	//			--2X							---X
	//			-1-X							---X
	//			---Z							--2X
	//											-1-	
	//The _dx, _dy and _dz values are all -1 for this move.

	//Fail by default if there's no grid
	if (!m_pGrid)
		return false;

	//Special methods for small and big critters
	if ((_iBaseSize != CHARSIZE) ||
		(_iHeight != CHARHEIGHT))
	{
		if ((_iBaseSize < CHARSIZE) ||
			(_iHeight < CHARHEIGHT))
			return IsTraversableSmall(_Pos, _iBaseSize, _iHeight, _bClimb, _dx, _dy, _dz);
		else
			return IsTraversableBig(_Pos, _iBaseSize, _iHeight, _bClimb, _dx, _dy, _dz);
	};

	//The below is optimized for character size (currently 3x3x4 cells)

	if ((_dx | _dy | _dz) == 0) 
	{
		//No move, just check traversability for given position
		return Is3x3x4Traversable(_bClimb, _Pos[0], _Pos[1], _Pos[2]);
	} 
	else
	{
		//Although not always necessary, I.ll make sure all cells in the box 
		//described by the extreme points of the previous and current node are 
		//checked, except for any intermediate cells below the current node when 
		//ascending or below the previous node when descending.
		//Some of the cells we need to check has already been checked though, 

		//Check if we're ascending...
		if (_dz < 0)
		{
			//Ascending, must check any intermediate nodes above the previous node... 
			if (!IsTraversableVertical(_bClimb, _Pos[0] + _dx, _Pos[1] + _dy, _Pos[2], _dz))
				return false;

			//..and xy-move in current node plane (if we've moved horizontally)
			if ((_dx | _dy) != 0)
			{
				if (!IsTraversableLevel(_bClimb, _Pos[0], _Pos[1], _Pos[2], _dx, _dy))
					return false;
			}
		}
		//...or descending...
		else if (_dz > 0)
		{
			//Descending, must check xy move in previous node plane (if any)...
			if ((_dx | _dy) != 0)
			{
				if (!IsTraversableLevel(_bClimb, _Pos[0], _Pos[1], _Pos[2] + _dz, _dx, _dy))
					return false;
			}

			//...and any intermediate nodes above the current node
			if (!IsTraversableVertical(_bClimb, _Pos[0], _Pos[1], _Pos[2], _dz))
				return false;
		}
		//...or moving levelly
		else
		{
			//Just check xy-move (duh!)
			if (!IsTraversableLevel(_bClimb, _Pos[0], _Pos[1], _Pos[2], _dx, _dy))
				return false;
		}

		//All checks successful
		return true;
	}

/*Faulty code below
		//Start in current position and check backwards towards the position we moved from 
		//(Pos + (dx,dy,dz)). This position has already been checked.
		if (!Is3x3x4Traversable(_Pos[0], _Pos[1], _Pos[2]))
			return false;

		//This algorithm is designed for short move (no more than the size of the bounding box) 
		//but can handle long moves less efficiently
		if ((Abs(_dx) <= CHARSIDESIZE) && (Abs(_dy) <= CHARSIDESIZE) && (Abs(_dz) <= CHARHEIGHT))
		{
			//Short move
			//Check if we're moving up, down or level
			if (_dz == 0)
			{
				//Moving levelly, check cells in this plane
				if ((_dx != 0) && (_dy != 0))
				{
					//Moving diagonally. We need only check the two cells at (posX, prevY, posZ)
					//and (prevX, posY, posZ) since we're only moving a short distance
					if ((!Is3x3x4Traversable(_Pos[0], _Pos[1] + _dy, _Pos[2])) ||															
						(!Is3x3x4Traversable(_Pos[0] + _dx, _Pos[1], _Pos[2])))
						return false;
					else
						return true;
				}
				else
				{
					//We have already checked the current cell and the previous cell has been checked before, so we're done
					return true;
				};

			}
			else if ((_dx == 0) && (_dy == 0))
			{
				//We're moving straight up or down a short distance, ao we have already cehcked all necessary cells
				return true;
			}
			else
			{
				//Moving up/down and also in the xy plane. Must check additional cells at the same plane
				//as current cell if we're moving up (dz<0) or same plane as previous cell if we're moving down (dz > 0).
				int z;
				if (_dz < 0)
				{
					//Moving up. Check cell straight above previous cell
					z = _Pos[2];
					if (!Is3x3x4Traversable(_Pos[0] + _dx, _Pos[1] + _dy, z))
						return false;
				}
				else
				{
					//Moving down. Check cell straight above current cell
					z = _Pos[2] + _dz;
					if (!Is3x3x4Traversable(_Pos[0], _Pos[1], z))
						return false;
				};

				//Now check cells in the above detemined plane, if we're moving diagonally 
				//(otherwise we've already checked) all necessary cells already.
				if ((_dx != 0) && (_dy != 0))
				{
					//Moving diagonally. Check two cells as above
					if ((!Is3x3x4Traversable(_Pos[0], _Pos[1] + _dy, z)) ||															
						(!Is3x3x4Traversable(_Pos[0] + _dx, _Pos[1], z)))
						return false;
					else
						return true;
				}
				else
				{
					//Everything already checked!
					return true;
				};
			};
 		}
		else
		{

			//We're moving a long distance which this algorithm isn't designed for. Use brute force.
			int MinX = _Pos[0] - _iBaseSize;
			int MaxX = _Pos[0] + _iBaseSize;
			int MinY = _Pos[1] - _iBaseSize;
			int MaxY = _Pos[1] + _iBaseSize;
			int MinZ = _Pos[2];
			int MaxZ = _Pos[2] + _iHeight - 1;

			if (_dx > 0)
				MaxX += _dx;
			else //_dx <= 0
				MinX += _dx;

			if (_dy > 0)
				MaxY += _dy;
			else //_dy <= 0
				MinY += _dy;

			if (_dz > 0)
				MaxZ += _dz;
			else if (_dz < -_iHeight)
				MinZ += _dz + _iHeight;

			for (int x = MinX + CHARSIZE; x <= MaxX - CHARSIZE; x += Max(1, Min((int)CHARSIDESIZE, MaxX - x - CHARSIZE)))
				for (int y = MinY + CHARSIZE; y <= MaxY - CHARSIZE; y += Max(1, Min((int)CHARSIDESIZE, MaxY - y - CHARSIZE)))
					for (int z = MinZ; z < MaxZ - CHARHEIGHT + 2; z += Max(1, Min((int)CHARHEIGHT, MaxZ - z - CHARHEIGHT + 1)))
					{
						if (!Is3x3x4Traversable(x, y, z))
							return false;
					};

			//All cells are traversable!
			return true;
		};
	*/
};




//Checks if an entity with the given base size and height (in world units) can travel in a straight line from 
//the start to the end position, following the ground with no steps higher than _iMaxStep units and no falls greater than _iMaxFall units
//If the _pRes argument is supplied, this value is set to the furthest position along the line from the start to the end that
//could be reached.
bool CXR_BlockNav::IsGroundTraversableImpl(const CVec3Dint16& _Start, const CVec3Dint16& _End, int _iBaseSize, int _iHeight, bool _bClimb, int _iMaxStep, int _iMaxFall, CVec3Dint16* _pRes)
{
	MAUTOSTRIP(CXR_BlockNav_IsGroundTraversableImpl, false);
	MSCOPESHORT(CXR_BlockNav::IsGroundTraversableImpl);
	//DEBUG
	//fp32 debugdur = 5.0f;
 	//m_pWServer->Debug_RenderWire(GetWorldPosition(_Start), GetWorldPosition(_End), 0xff00ff00, debugdur);//DEBUG
	//DEBUG

	if (_pRes)
 		*_pRes = _Start;

	//Fail by default if navigation grid is invalid
	if (!m_pGrid)
		return false;

	//Check that start and end nodes are inside grid, on ground and traversable
	// We do not check if _End is traversible/on ground if _pRes was supplied
	// as we are probably interested in how close to _End we got
	if (IsOutsideGridBoundariesImpl(_Start, _iBaseSize, _iHeight)||
		IsOutsideGridBoundariesImpl(_End, _iBaseSize, _iHeight)||
		!IsOnGroundImpl(_Start, _iBaseSize, _bClimb)||
		!IsTraversableImpl(_Start, _iBaseSize, _iHeight, _bClimb)||
		((_pRes == NULL) && ((!IsOnGroundImpl(_End, _iBaseSize, _bClimb))||(!IsTraversableImpl(_End, _iBaseSize, _iHeight, _bClimb)))))
		return false;
	

	// There is a 3D case and the regular 2D case
	if (_bClimb)
	{
		if ((_Start[0] == _End[0])&&(_Start[1] == _End[1])&&(_Start[2] == _End[2]))
		{
			return true;
		}
	}
	else
	{
		if ((_Start[0] == _End[0])&&(_Start[1] == _End[1]))
		{	//Same position in the xy plane, and both start and end is on ground.
			//Thus start and end must be on different levels with a floor in between or the same position
			if (_Start[2] == _End[2])
				return true;
			else
				return false;
		};
	}
	
	//To ensure that the direct route from _Start to _End is clear, we must step through two paths that 
	//encapsulate the straight line between the points, checking these paths for ground traversability.
	CVec3Dint16 CurPos[2] = {_Start, _Start};
	CVec3Dint16 PrevPos[2];
	
	//The number of cell steps necessary to reach the end
	int iSteps;
	if (_bClimb)
	{
		iSteps = Max3(Abs(_Start[0] - _End[0]), Abs(_Start[1] - _End[1]), Abs(_Start[2] - _End[2]));
	}
	else
	{
		iSteps = Max(Abs(_Start[0] - _End[0]), Abs(_Start[1] - _End[1]));
	}
	// The increment we step in each coordinate each step reaching _End after iSteps steps.
	CVec3Dfp32 k;
	k[0] = (_End[0] - _Start[0]) / (fp32)iSteps;
	k[1] = (_End[1] - _Start[1]) / (fp32)iSteps;
	if (_bClimb)
	{
		k[2] = (_End[2] - _Start[2]) / (fp32)iSteps;
	}
	else
	{
		k[2] = 0.0f;
	}

	//Step through paths (start at first step beyond start position, end at end position)
	fp32 x,y,z;	
	int i = 0;
	CVec3Dint16 Start = _Start;
	for (i = 1; i <= iSteps; i++)
	{
		PrevPos[0] = CurPos[0];
		PrevPos[1] = CurPos[1];
		
		x = i * k[0] + Start[0];
		y = i * k[1] + Start[1];
		z = i * k[2] + Start[2];

		//Get new positions of encapsulating paths z-positions 
		//M_Ceil and M_Floor are unsafe for large floats
		int xTrunc = (int)x;
		CurPos[0][0] = xTrunc + int(M_Ceil((x - xTrunc) - _FP32_EPSILON));
		CurPos[1][0] = xTrunc + int(M_Floor((x - xTrunc) + _FP32_EPSILON));
		int yTrunc = (int)y;
		CurPos[0][1] = yTrunc + int(M_Ceil((y - yTrunc) - _FP32_EPSILON));
		CurPos[1][1] = yTrunc + int(M_Floor((y - yTrunc) + _FP32_EPSILON));
		int zTrunc = (int)z;
		CurPos[0][2] = zTrunc + int(M_Ceil((z - zTrunc) - _FP32_EPSILON));
		CurPos[1][2] = zTrunc + int(M_Floor((z- zTrunc) + _FP32_EPSILON));

		//Get z-coord of current positions, and check if those positions are traversable
		//Brute force traversability checking, this can be optimized quite a bit, might fix.
		int upSteps = 0;
		int downSteps = 0;
		for (int k = 0; k < 2; k++)
		{
			//Previous position was ground traversable
			if (_pRes)
				*_pRes = PrevPos[k];

			if (IsOutsideGridBoundariesImpl(CurPos[k], _iBaseSize, _iHeight))
				return false;

			if (IsOnGroundImpl(CurPos[k], _iBaseSize, _bClimb))
			{
				//Current position is  on ground or _in_ ground. In the latter case, set current 
				//position to closest traversable position above current position or fail if we 
				//move higher than our maximum step size.
				int j = 0;
				while (!IsTraversableImpl(CurPos[k], _iBaseSize, _iHeight, _bClimb, PrevPos[k][0] - CurPos[k][0], PrevPos[k][1] - CurPos[k][1], PrevPos[k][2] - CurPos[k][2]))
				{
					j++;
					if (j > _iMaxStep)
					{
						//No traversable position below step height!
						return false;
					}
					else
						//Check cell above
						CurPos[k][2]++;

					if (IsOutsideGridBoundariesImpl(CurPos[k], _iBaseSize, _iHeight))
						return false;
				};
				//We've found a traversable position, carry on!
				upSteps = Max(upSteps,j);
			} 
			else 
			{
				//Fail if we do not allow falls
				if (_iMaxFall == 0)
					return false;

				//Current position is not on ground, check positions below or fail if we move lower 
				//than our maximum fall height. We should actually also check positions above to cover 
				//the case of a small gap followed by a ledge above our current position, but that is 
				//so ridiculously uncommon so I'll just ignore it :)
				int j = 1;
				CurPos[k][2]--;
		
				if (IsOutsideGridBoundariesImpl(CurPos[k], _iBaseSize, _iHeight))
					return false;
				
				while (!IsOnGroundImpl(CurPos[k], _iBaseSize, _bClimb))
				{
					j++;
					if (j > _iMaxFall)
					{
						//No ground position above fall height!
						return false;
					}
					else
						//Check cell below
						CurPos[k][2]--;
		
					if (IsOutsideGridBoundariesImpl(CurPos[k], _iBaseSize, _iHeight))
						return false;
				};

				//We've found a ground position below, check for traversability
				if (!IsTraversableImpl(CurPos[k], _iBaseSize, _iHeight, _bClimb, PrevPos[k][0] - CurPos[k][0], PrevPos[k][1] - CurPos[k][1], PrevPos[k][2] - CurPos[k][2]))
				{
					//Not traversable, fail
					return false;
				}
				//Position was traversable, carry on!
				downSteps = Max(downSteps,j);
			};
		};
		// If we're here both directions worked
		if (upSteps >= downSteps)
		{
			Start[2] += upSteps;
		}
		else
		{
			Start[2] -= downSteps;
		}
	};
	
	if (_bClimb)
	{
		if (_pRes)
			*_pRes = _End;
		return true;
	}
	else
	{
		//We've stepped to the end's xy-position, check that we're at correct level as well
		if(CurPos[0][2] == _End[2])
		{
			//We made it to the end!
			if (_pRes)
				*_pRes = _End;
			return true;
		}
		else
		{	//We are straight above or below end position, and can't fall more or step higher
			if (_pRes)
				*_pRes = CurPos[0];
			return false;
		}
	}
};


//Checks if an entity with the given base size and height (in world units) can travel in a straight line from 
//the start to the end position, following the ground with no steps higher than _iMaxStep units and no falls greater than _iMaxFall units
bool CXR_BlockNav::IsGroundTraversable(const CVec3Dfp32& _Start, const CVec3Dfp32& _End, int _iBaseSize, int _iHeight, bool _bClimb, int _iMaxStep, int _iMaxFall, CVec3Dfp32 * _pRes)
{
	MAUTOSTRIP(CXR_BlockNav_IsGroundTraversable, false);
	MSCOPESHORT(CXR_BlockNav::IsGroundTraversable);
	
	//Set the base "square radius" of the entity
	fp32 sz;
	/*
	// *** Don't know why this code was added, i guess ut should read '_bClimb != false' instead
	// *** On the other hand I cannot understand why climbers should be considered smaller!
	if (_bClimb == false)
	{	// 1+1  cells wider
		sz = Max(0.0f, (fp32)(_iBaseSize + m_pGrid->m_UnitsPerCell));
	}
	else
	*/
	{
		sz = Max(0.0f, (fp32)(_iBaseSize - m_pGrid->m_UnitsPerCell));
	}
	_iBaseSize = int(M_Ceil(0.5f * sz * m_pGrid->m_CellsPerUnit - _FP32_EPSILON));

	//Set the height of the entity. This is the smallest height the entity can be, usually the crouching height.
	_iHeight = Max(0, (int)M_Ceil(_iHeight * m_pGrid->m_CellsPerUnit - _FP32_EPSILON));

	//Convert max step size and max fall height to cells
	_iMaxStep = Max(0, (int)(_iMaxStep * m_pGrid->m_CellsPerUnit));
	_iMaxFall = Max(0, (int)(_iMaxFall * m_pGrid->m_CellsPerUnit));

	CVec3Dint16 Res = GetGridPosition(_Start);
	if (IsGroundTraversableImpl(GetGridPosition(_Start), GetGridPosition(_End), _iBaseSize, _iHeight, _bClimb, _iMaxStep, _iMaxFall, (_pRes) ? &Res : NULL))
	{
		if (_pRes)
			*_pRes = _End;
		return true;
	}
	else
	{
		if (_pRes)
		{
			if (Res == GetGridPosition(_Start))
				*_pRes = _Start;
			else
				*_pRes = GetWorldPosition(Res);
		}
		return false;
	};
};


// What constitutes being on ground? (current pos is at x y z)
// If z-1 is not air nor slip we are on ground
// If z-1 is air or slip but z-2 is solid we are on ground (if at least one other cell has ground beneath it)
// Wallclimbers are on 'ground' if xyz or any adjacent cell is a wall tile
bool CXR_BlockNav::IsOnGroundImpl(const CVec3Dint16& _Pos, int _iBaseSize, bool _bClimb)
{
	MAUTOSTRIP(CXR_BlockNav_IsOnGroundImpl, false);
	MSCOPESHORT(CXR_BlockNav::IsOnGroundImpl);

	//Fail by default if navgrid is invalid
	CXR_BlockNav_GridProvider* M_RESTRICT pGrid = m_pGrid;
	if (!pGrid)
		return false;

	//Look for ground cells and count non-ground cells
	int CellBelow; 
	int CellBelow2;

	if (_bClimb)
	{
		for(int x = _Pos[0] - _iBaseSize; x <= _Pos[0] + _iBaseSize; x++)
		{
			for(int y = _Pos[1] - _iBaseSize; y <= _Pos[1] + _iBaseSize; y++)
			{
				if ((GETATXYZ(pGrid, x, y, _Pos[2]) & XR_CELL_WALL)||(GETATXYZ(pGrid, x, y, _Pos[2]+1) & XR_CELL_WALL))
				{
					return true;
				}
			}
		}
	}

	bool bGround = false;
	CellBelow = GETATXYZ(pGrid, _Pos[0], _Pos[1], _Pos[2]-1);
	CellBelow2 = GETATXYZ(pGrid, _Pos[0], _Pos[1], _Pos[2]-2);

	if ((_iBaseSize == 1)&&(CellBelow & XR_CELL_TRAVERSABLE))
	{	// XR_CELL_TRAVERSABLE means all 9 cells below are air or slip
		return(false);
	}

	if ((!CellBelow)||((!CellBelow2)&&((CellBelow & XR_CELL_AIR)||(CellBelow & XR_CELL_SLIP))))
	{	// Cell below is solid or cell below is air or slip and the cell below THAT is solid
		bGround = true;
	}

	return bGround;
};

//Checks if the node specified by the given position is a ground node, i.e. if any of the cells
//in the square area with a side of _BaseSize units, when standing on the node has solid cells 
//beneath them. This method makes no assumptions about the node being traversable though.
bool CXR_BlockNav::IsOnGround(const CVec3Dint16& _Pos, int _iBaseSize, bool _bClimb)
{
	MAUTOSTRIP(CXR_BlockNav_IsOnGround, false);
	MSCOPESHORT(CXR_BlockNav::IsOnGround);
	//Fail by default if navigation grid is invalid or given grid position is outside grid boundaries
	if (!m_pGrid || IsOutsideGridBoundaries(_Pos, _iBaseSize))
		return false;
	
	//Set the base "square radius" of the entity
	fp32 sz = Max(0.0f, (fp32)(_iBaseSize - m_pGrid->m_UnitsPerCell));
	_iBaseSize = int(M_Ceil(0.5f * sz * m_pGrid->m_CellsPerUnit - _FP32_EPSILON));

	return IsOnGroundImpl(_Pos, _iBaseSize, _bClimb);
};


// What constitutes being on ground? (current pos is at x y z)
// If z-1 is not air nor slip we are on ground
// If z-1 is air or slip but z-2 is solid we are on ground (if at least one other cell has ground beneath it)
// Wallclimbers are on 'ground' if xyz or any adjacent cell is a wall tile
// _Count is set to the nbr of non groundcells
bool CXR_BlockNav::IsOnGroundImpl(const CVec3Dint16& _Pos, int _iBaseSize, bool _bClimb, int& _Count)
{
	MAUTOSTRIP(CXR_BlockNav_IsOnGroundImpl_2, false);
	MSCOPESHORT(CXR_BlockNav::IsOnGroundImpl);
	
	//Reset count
	_Count = 0;

	//Fail by default if navgrid is invalid
	CXR_BlockNav_GridProvider* M_RESTRICT pGrid = m_pGrid;
	if (!pGrid)
		return false;

	//Look for ground cells and count non-ground cells
	int CellBelow; 
	int CellBelow2;
	
	if (_bClimb)
	{
		for(int x = _Pos[0] - _iBaseSize; x <= _Pos[0] + _iBaseSize; x++)
		{
			for(int y = _Pos[1] - _iBaseSize; y <= _Pos[1] + _iBaseSize; y++)
			{
				if ((GETATXYZ(pGrid, x, y, _Pos[2]) & XR_CELL_WALL)||(GETATXYZ(pGrid, x, y, _Pos[2]+1) & XR_CELL_WALL))
				{
					_Count = 0;
					return true;
				}
			}
		}
	}
	
	bool bGround = false;
	CellBelow = GETATXYZ(pGrid, _Pos[0], _Pos[1], _Pos[2]-1);
	CellBelow2 = GETATXYZ(pGrid, _Pos[0], _Pos[1], _Pos[2]-2);

	if ((_iBaseSize == 1)&&(CellBelow & XR_CELL_TRAVERSABLE))
	{	// XR_CELL_TRAVERSABLE means all 9 cells below are traverseable
		_Count = 0;
		return(false);
	}

	if ((!CellBelow)||((!CellBelow2)&&((CellBelow & XR_CELL_AIR)||(CellBelow & XR_CELL_SLIP))))
	{	// Cell below is solid or cell below is air or slip and the cell below THAT is solid
		bGround = true;
	}

	// Loop through all 9 cells (skipping xy) to count the nbr of nongroundcells
	for(int x = _Pos[0] - _iBaseSize; x <= _Pos[0] + _iBaseSize; x++)
	{
		for(int y = _Pos[1] - _iBaseSize; y <= _Pos[1] + _iBaseSize; y++)
		{
			CellBelow = GETATXYZ(pGrid, x, y, _Pos[2]-1);
			CellBelow2 = GETATXYZ(pGrid, x, y, _Pos[2]-1);

			if ((CellBelow & (XR_CELL_SLIP | XR_CELL_AIR))&&
				(CellBelow2 & (XR_CELL_SLIP | XR_CELL_AIR)))
			{
				_Count++;
			}
		}
	}

	return bGround;
};


//Checks if the node specified by the given position is a ground node, i.e. if any of the cells
//in the square area with a side of _BaseSize units, when standing on the node has solid cells 
//beneath them. This method makes no assumptions about the node being traversable though.The _Count 
//argument is set to the number of non-ground cells found. 
bool CXR_BlockNav::IsOnGround(const CVec3Dint16& _Pos, int _iBaseSize, bool _bClimb, int& _Count)
{
	MAUTOSTRIP(CXR_BlockNav_IsOnGround_2, false);
	MSCOPESHORT(CXR_BlockNav::IsOnGround);
	//Fail by default if navigation grid is invalid or given grid position is outside grid boundaries
	if (!m_pGrid || IsOutsideGridBoundaries(_Pos, _iBaseSize))
		return false;
	
	//Set the base "square radius" of the entity
	fp32 sz = Max(0.0f, (fp32)(_iBaseSize - m_pGrid->m_UnitsPerCell));
	_iBaseSize = int(M_Ceil(0.5f * sz * m_pGrid->m_CellsPerUnit - _FP32_EPSILON));

	return IsOnGroundImpl(_Pos, _iBaseSize, _bClimb, _Count);
};



//Gets the corresponding navigation grid position of the given world position
CVec3Dint16 CXR_BlockNav::GetGridPosition(const CVec3Dfp32& _WorldPos)
{
	MAUTOSTRIP(CXR_BlockNav_GetGridPosition, CVec3Dint16(0,0,0));
	MSCOPESHORT(CXR_BlockNav::GetGridPosition);
	//Fail by default if navigation grid is invalid 
	if (!m_pGrid)
		return CVec3Dint16(-1);

	CVec3Dfp32 Pos = _WorldPos;
	Pos -= m_pGrid->m_Origin;
	Pos *= m_pGrid->m_CellsPerUnit;

	//Add small epsilon to avoid possible trunc error
	CVec3Dint16 Res(TruncToInt(Pos[0]+0.01f), TruncToInt(Pos[1]+0.01f), TruncToInt(Pos[2]+0.01f)); 

	//Fail if the position is outside the navigation grid
	if (IsOutsideGridBoundaries(Res))
		return CVec3Dint16(-1);

	//Success!
	return Res;
};


//Gets the corresponding world position of the cell center of the given navigation grid position
CVec3Dfp32 CXR_BlockNav::GetWorldPosition(const CVec3Dint16& _GridPos)
{
	MAUTOSTRIP(CXR_BlockNav_GetWorldPosition, CVec3Dfp32());
	MSCOPESHORT(CXR_BlockNav::GetWorldPosition);
	//Fail by default if navigation grid is invalid or is outside grid boundaries
	if (!m_pGrid || IsOutsideGridBoundaries(_GridPos, 0, 1))
		return CVec3Dfp32(_FP32_MAX,_FP32_MAX,_FP32_MAX);

	//Set position to the corresponding world position of the search node's grid position
	CVec3Dfp32 Res(_GridPos[0], _GridPos[1], _GridPos[2]);
	Res *= m_pGrid->m_UnitsPerCell;
	Res += m_pGrid->m_Origin + CVec3Dfp32(m_pGrid->m_UnitsPerCell / 2);
	
	return Res;
};


fp32 CXR_BlockNav::UnitsPerCell()
{
	MAUTOSTRIP(CXR_BlockNav_UnitsPerCell, 0.0f);
	//Fail by default if navigation grid is invalid
	if (!m_pGrid)
		return 0.0f;

	return m_pGrid->m_UnitsPerCell;
};


//Returns the "square radius" in cells given base size in world units. This is the number of cells outside 
//the center cell which the base will reach
int CXR_BlockNav::GridSquareRadius(fp32 _BaseSize)
{
	MAUTOSTRIP(CXR_BlockNav_GridSquareRadius, 0);
	if (!m_pGrid)
		return 0;

	_BaseSize = Max(0.0f, (fp32)(_BaseSize - m_pGrid->m_UnitsPerCell));
	return int(M_Ceil(0.5f * _BaseSize * m_pGrid->m_CellsPerUnit - _FP32_EPSILON));
};


//Returns the height in cells given height in world units
int CXR_BlockNav::GridHeightCeil(fp32 _Height)
{
	MAUTOSTRIP(CXR_BlockNav_GridHeightCeil, 0);
	if (!m_pGrid)
		return 0;

	return int(Max(0.0f, M_Ceil(_Height * m_pGrid->m_CellsPerUnit - _FP32_EPSILON)));
};

int CXR_BlockNav::GridHeightFloor(fp32 _Height)
{
	MAUTOSTRIP(CXR_BlockNav_GridHeightFloor, 0);
	if (!m_pGrid)
		return 0;

	return int(Max(0.0f, _Height * m_pGrid->m_CellsPerUnit));
};


//Grid dimensions of navgrid
CVec3Dint16 CXR_BlockNav::GridDimensions()
{
	MAUTOSTRIP(CXR_BlockNav_GridDimensions, CVec3Dint16(0,0,0));
	if (!m_pGrid)
		return CVec3Dint16(-1);
	
	return CVec3Dint16(m_pGrid->m_CellGridDim[0], m_pGrid->m_CellGridDim[1], m_pGrid->m_CellGridDim[2]);
};

//Get grid pointer (ugly, shouldn't be used but I have no other choice when builing navgraph)
CXR_BlockNav_GridProvider* CXR_BlockNav::GetGrid()
{
	MAUTOSTRIP(CXR_BlockNav_GetGrid, NULL);
	return m_pGrid;	
}

/*
void CXR_BlockNav::Debug_Render(CWireContainer* _pWC)
{

}*/



/*
CXBN_SearchInstance* CXR_BlockNavSearcher::SEARCH_GetInstance(int _iSearch)
{
#ifdef PLATFORM_VPU
	return m_lspSearches.ValidPos(_iSearch) ? m_lspSearches[_iSearch] : (TPtr<CXBN_SearchInstance>)NULL;
#else
	return m_lspSearches.ValidPos(_iSearch) ? m_lspSearches[_iSearch] : (TPtr<CVPUSearchInstance>)NULL;
#endif
}*/

/*
#include "WNavGraph/WNavGraph.h"

void CXR_BlockNav::BuildNavGraph(const CVec3Dfp32& _Pos, int _iMode)
{
	MAUTOSTRIP(CXR_BlockNav_BuildNavGraph, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_BlockNav::BuildNavGraph);
	switch (_iMode)
	{
	case 0:
		{
			if (m_pWServer && m_pWServer->m_spWorldData)
			{
				//Build...
				CXR_NavGraph_Builder Graph;
				Graph.BuildGraph(this, m_pWServer, GetGridPosition(_Pos), 32, (1 << CXR_NavGraph::NUM_SIZEGROUPS) - 1);

				//...write...
				CStr WorldName = m_pWServer->m_spWorldData->GetWorld();
				WorldName = m_pWServer->m_spWorldData->ResolveFileName(WorldName);
				WorldName = WorldName.GetPath() + "worlds\\" + WorldName.GetFilenameNoExt() + ".XNG";
				CDataFile DFile;
				DFile.Create(WorldName);
				Graph.Write(&DFile);
				DFile.Close();

				//...and debug render graph
				Graph.DebugRender(m_pWServer);
			}
		}
		break;
	case 1:
		{
			if (m_pWServer && m_pWServer->m_spWorldData && m_pGrid)
			{
				//Read,...
				CXR_NavGraph_Builder Graph;
				CStr WorldName = m_pWServer->m_spWorldData->GetWorld();
				WorldName = m_pWServer->m_spWorldData->ResolveFileName(WorldName);
				WorldName = WorldName.GetPath() + "worlds\\" + WorldName.GetFilenameNoExt() + ".XNG";
				if(!CDiskUtil::FileExists(WorldName))
				{
					ConOutL("ERROR No such file " + WorldName);
					return;
				}
				CDataFile DFile;
				DFile.Open(WorldName);
				Graph.Read(&DFile);
				DFile.Close();

				//...build hash...
				Graph.BuildHash(CVec3Dint16(Graph.GetMinDistance(), Graph.GetMinDistance(), Graph.GetMinDistance() * 5), 
								CVec3Dint16(m_pGrid->m_CellGridDim[0], m_pGrid->m_CellGridDim[1], m_pGrid->m_CellGridDim[2]));
				
				//...and debug render graph
				Graph.DebugRender(m_pWServer);

			};
		}
		break;
	default:
		{
		
		};
	};
}
*/

