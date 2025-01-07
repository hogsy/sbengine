#ifndef _INC_XRBLOCKNAVINST
#define _INC_XRBLOCKNAVINST



// #include "WMapData.h"
/*
#include "XRBlockNavGrid.h"
#include "MDA_PQueue.h"
*/
#include "XRBlockNav.h"
// WBN = World block navigation

#ifndef BLOCKNAV_VPU_COMPILE
	#define BLOCKNAV_USE_VPU
#endif

// -------------------------------------------------------------------

enum
{
	//WBN_NODE_OPEN = 1,  //Open and closed flags are unnecessary
	//WBN_NODE_CLOSED = 2,
	WBN_NODE_AIRBORNE = 1,
	WBN_PARENT_NODE_JUMP = 2,
	WBN_NODE_LONGJUMP = 4,

	WBN_NODE_MAX = 4, //This must be updated or else navgraph building will be faulty
};

class CXBN_SearchNode			// 24 bytes
{
public:
	CXBN_SearchNode* m_pParent;
	CVec3Dint16 m_Pos;
	int16 m_iNode;
	int16 m_iHashPrev;
	int16 m_iHashNext;
	int m_XVel : 2;
	int m_YVel : 2;
	int m_ZVel : 8;
	int m_ZPosFrac : 4; 
	uint8 m_Flags : 4;
	uint8 m_Counter : 4;
	uint32 m_Cost : 16;
	uint32 m_Distance : 16;

	void Construct()
	{
		m_pParent = NULL;
		m_Pos[0] = -12345;
		m_Pos[1] = -12345;
		m_Pos[2] = -12345;
		m_Flags = 0;
		m_Counter = 0;
		m_Cost = 0;
		m_Distance = 0;
		m_XVel = 0;
		m_YVel = 0;
		m_ZVel = 0;
		m_ZPosFrac = 0; 
		m_iHashPrev = -1;
		m_iHashNext = -1;
	}

	CXBN_SearchNode()
	{
		Construct();
	}

	// Needed for TPriorityQueue2<>
	int GetPriority()
	{
		return m_Cost + m_Distance;
	}
};

/*
class CXBN_SearchNodePtr
{
public:
	CXBN_SearchNode* m_pN;

	void Clear()
	{
		m_pN = NULL;
	}

	CXBN_SearchNodePtr()
	{
		m_pN = NULL;
	}
};

typedef TDynamicBlockNavGrid<CXBN_SearchNodePtr> CXBN_SearchNodeGrid;
*/

// forward declarations
class CXR_BlockNav;
class CWireContainer;


// -------------------------------------------------------------------
class CXBN_SearchInstance : public CReferenceCount
{
protected:
	static const int WBN_COSTFRACTIONS;
	static const fp32 WBN_DISTANCEFACTOR;
	static const fp32 WBN_COSTFACTOR;
	static const int WBN_FALLDAMAGE_COSTFACTOR;
	static const int WBN_EDGE_COST;
	static const int WBN_LONGJUMP_COSTFACTOR;

	//Is search instance in use?
	bool m_bInUse;

	//Last server tick when instance was released
	int m_LastReleasedTick;

	int m_MaxNodes;

	//This holds the cause that the search failed, if it runs out of nodes to search. Normally
	//it's CXR_BlockNav::SEARCH_NO_PATH, but there are some special case exceptions
	int m_iCauseOfFailure;

#ifdef PLATFORM_VPU
	TThinArray_VPU<CXBN_SearchNode, BLOCKNAV_VPU_NUMNODES> m_lNodeHeap;
	TThinArray_VPU<int16, 1<<12> m_liHashFirstNode;
	TPriorityQueue2Base<CXBN_SearchNode, TThinArray_VPU<CXBN_SearchNode*, BLOCKNAV_VPU_NUMNODES> > m_PQueue; // VPU TODO: Crapylon-kma
#else
	TThinArray<CXBN_SearchNode> m_lNodeHeap;
	TThinArray<int16> m_liHashFirstNode;
	TPriorityQueue2<CXBN_SearchNode> m_PQueue; // VPU TODO: Crapylon-kma
#endif
	int m_iNodeNext;


	CXBN_SearchParams m_Params;
//	TPtr<CXBN_SearchNodeGrid> m_spNodeGrid;
	CXR_BlockNav_GridProvider* m_pGrid;
	CXR_BlockNav *m_pBlockNav;	


//	CXBN_SearchNode* m_pOpenFirst;
	CXBN_SearchNode* m_pDstNode;

	//The last visited node. Used for getting partial search results when there's no complete path
	CXBN_SearchNode* m_pLastNode;

	//Number of expanded nodes in total
	int m_nExpandedNodes;

	enum
	{
		PUSHNODE_OK = 0,
		PUSHNODE_BLOCKEDDAMAGE,
		PUSHNODE_ALLOCFAIL,
		PUSHNODE_EXIST, 
		PUSHNODE_BLOCKED,
		PUSHNODE_OUTOFBOUNDS,

		PUSHNODE_FLAGS_AIR = 1,
	};

	CXBN_SearchNode* NewNode();

	M_INLINE int Hash_GetKey(int _x, int _y, int _z)
	{
		return (_x & 15) + ((_y & 15) << 4) + ((_z & 15) << 8);
	}

	void Hash_Clear()
	{
		m_liHashFirstNode.SetLen(1 << 12);

		// TODO: fix this.. this ain't nice
#ifdef PLATFORM_VPU
		for(int i = 0; i < m_liHashFirstNode.Len(); i++)
			m_liHashFirstNode[i] = -1;
#else
		FillW(m_liHashFirstNode.GetBasePtr(), m_liHashFirstNode.Len(), -1);
#endif
	}

	M_INLINE void Hash_InsertNode(int _iNode)
	{
		CXBN_SearchNode* pN = &m_lNodeHeap[_iNode];
		int iHashKey = Hash_GetKey(pN->m_Pos[0], pN->m_Pos[1], pN->m_Pos[2]);
		pN->m_iHashPrev = -1;
		pN->m_iHashNext = m_liHashFirstNode[iHashKey];
		if (pN->m_iHashNext != -1)
			m_lNodeHeap[pN->m_iHashNext].m_iHashPrev = _iNode;
		m_liHashFirstNode[iHashKey] = _iNode;
	}

	M_INLINE CXBN_SearchNode* Hash_GetNode(int _x, int _y, int _z)
	{
		int iHash = Hash_GetKey(_x, _y, _z);
		int iNode = m_liHashFirstNode[iHash];
		while(iNode >= 0)
		{
			CXBN_SearchNode* pN = &m_lNodeHeap.GetBasePtr()[iNode];
			if (pN->m_Pos[0] == _x &&
				pN->m_Pos[1] == _y &&
				pN->m_Pos[2] == _z)
			{
				// ConOut(CStrF("Hash hit %d, %d, %d, %d", pN->m_iNode, _x, _y, _z));
				return pN;
			}
			
			iNode = pN->m_iHashNext;
		}
		return NULL;
	}


	int GetCellValue(int _x, int _y, int _z);
	bool IsOutsideGridBoundaries(const CVec3Dint16& _Pos);
	bool IsTraversable(const CVec3Dint16& _Pos, int _dx = 0, int _dy = 0, int _dz = 0);
	bool IsOnGround(const CVec3Dint16& _Pos);
	bool IsOnGround(const CVec3Dint16& _Pos, int& _iCount);
	
	//Check if position already has been visited. This should be fixed to full A* eventually 
	//Use of virtual method won't allow compiler to inline pushnode, so this might not be a good solution. Must test.
	virtual bool CanPrune(const CVec3Dint16& _Pos);	

	//This is run when node has been successfully created and pushed. Inserts node in hash for this class.
	virtual void VisitNode(CXBN_SearchNode* _pN);

	int PushNode(CXBN_SearchNode* _pN, int _dx, int _dy, int _dz, int& _Flags, const CVec3Dint16& _Vel, int _ExtraCost = 0);
	void PushChildren(CXBN_SearchNode* _pN);

	//The search result needs to use some of the private methods
	friend class CXBN_SearchResultCore;
	friend class CXBN_SearchResult;
	//Friendship is unfortunately not inherited so we cannot declare friend class CXR_BlockNav
	friend class CXR_BlockNav;

public:
	bool InUse(int _GameTick = -1);
	void Release(int _GameTick = -1);
	M_INLINE int GetCauseOfFailure() { return(m_iCauseOfFailure);};
	CXBN_SearchInstance();
	virtual ~CXBN_SearchInstance() {}
	virtual void Create(CXR_BlockNav_GridProvider* _Grid, CXR_BlockNav* _pBlockNav, int _TileHeap, int _MaxNodes);
	virtual bool Search_Create(const CXBN_SearchParams& _Params);
	virtual int Search_Execute(int& _nNodeExpansions, int _nMaxExpansions, int _nMaxSlop = 2);
	virtual bool Search_Get(CXBN_SearchResultCore*  _pRes, bool _ReturnPartial = false);
	virtual int Search_GetUser();
	virtual void Debug_RenderNode(CWireContainer* _pWC, CXBN_SearchNode* _pN);
	virtual void Debug_Render(CWireContainer* _pWC);
};

typedef TPtr<CXBN_SearchInstance> spCXBN_SearchInstance;

#endif // _INC_XRBLOCKNAVINST
