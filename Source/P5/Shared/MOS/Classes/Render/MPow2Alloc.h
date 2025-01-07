
#ifndef _INC_MPow2Alloc
#define _INC_MPow2Alloc

#include "../../Mos.h"

// -------------------------------------------------------------------
//  CPow2Alloc
// -------------------------------------------------------------------
/*
	CPow2Alloc is a two-dimensional heap manager capable of 
	allocating and locking 2^x by 2^y rectangles in an 
	arbitrary number of heaps with 2^x by 2^y sizes.

	It's key facility is to place allocated blocks at the 
	oldest unused area, eventhough the area might be occupied 
	by an abritrary number of smaller blocks. Allocation is
	instantenously without iterative searching for space.

	To use, inherit CPow2Alloc and implement AllocBlock(), 
	FreeBlock() and GetBlockSize().

*/

// -------------------------------------------------------------------
class CP2H_Node				// 32 bytes
{
public:
	uint8 m_PosX1;
	uint8 m_PosY1;
	uint8 m_Width;
	uint8 m_Height;

	// Alloc map links
	int16 m_iNodeNext;		// -1 = void
	int16 m_iNodePrev;		// -1 = void
	int16 m_iNodeParent;	// -1 = void
	int16 m_iNodeMap;		// 0..63 index

	int32 m_TimeStamp;		// Area last used time

	int32 m_iNodeRoot;		// -1 = Error!
	int32 m_UserID;			// -1 = Empty
							// (Empty != -1) && ((m_iNode1 != -1) || (m_iNode2 != -1))  can't exist!

	int16 m_iNode1;			// -1 = Void
	int16 m_iNode2;			// -1 = Void

	int32 __m_Paddan1;


	CP2H_Node();
	~CP2H_Node();
	void Clear();
	void SetRect(CRct _Rect);
	int IsLeaf();
};

// -------------------------------------------------------------------
class CPow2Alloc
{
protected:
	int m_nHeaps;
	int m_HeapExtent;
	int m_HeapExtentLog2;
	fp32 m_HeapExtentInverse;
	int m_nBlocks;
	int m_Time;
	int16* m_piBlockNodes;

	int16 m_iNodeMap[8*8];
	int16 m_iNodeMapEnd[8*8];

	CP2H_Node* m_pNodes;
	TPtr<CIDHeap> m_spNodeAllocHeap;
	TArray<int32> m_lHeapAllocated;

	TArray<int16> m_iHeapTreeRoot;

	// Timing/stats.
	CMTime m_TimeLock;
	CMTime m_TimeStamp;
	CMTime m_TimeRelink;
	int m_nLocks;
	int m_nStamps;

protected:
	void* m_pContext;

	virtual void AllocBlock(int _ID, int _iNode) pure;
	virtual void FreeBlock(int _ID) {};
	virtual CPnt GetBlockSize(int _ID) pure;

	void FreeLeaf(int _iNode);
	void DelTree(int _iNode);
	int CreateNode(CRct _Rect, int _iParent, int _iRoot);
	int CreateLeaf(int _iNode, int _ws, int _hs);
	int AllocAtNode(int _iNode, int _ID, int _ws, int _hs);
	int Allocate(int _ID);

	void CheckTree(int _iNode);
	void CheckIntegrity(int _Type);

	void Link(int _iNode);
	void Unlink(int _iNode);
	void RelinkToBegin(int _iNode);
	void RelinkToEnd(int _iNode);
	void TimeStamp(int _iNode);

public:
	void Create(int _nBlocks, int _nHeaps, int _HeapExtent, int _nNodesHeap, void* _pContext);
	CPow2Alloc();
	~CPow2Alloc();

	void Tick();
	int Lock(int _ID, CRct& _DestRect, CVec2Dfp32& _TxtScale, CVec2Dfp32& _TxtAdd);	
	// Returns heap nr., _DestRect is where _ID is in Heap[nr].
};

// -------------------------------------------------------------------
#endif // _INC_MPow2Alloc
