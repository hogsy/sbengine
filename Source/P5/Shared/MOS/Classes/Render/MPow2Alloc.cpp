#include "PCH.h"
#include "MPow2Alloc.h"

// ----------------------------------------------------------------
//  CP2H_Node
// ----------------------------------------------------------------
CP2H_Node::CP2H_Node()
{
	Clear();
}

void CP2H_Node::Clear()
{
	m_PosX1 = 0;
	m_PosY1 = 0;
	m_Width = 0;
	m_Height = 0;

	m_iNodeNext = -1;
	m_iNodePrev = -1;
	m_iNodeParent = -1;
	m_iNodeMap = -1;

	m_TimeStamp = 0;

	m_iNodeRoot = -1;
	m_UserID = -1;

	m_iNode1 = -1;
	m_iNode2 = -1;
}

CP2H_Node::~CP2H_Node()
{
}

void CP2H_Node::SetRect(CRct _Rect)
{
	m_PosX1 = _Rect.p0.x;
	m_PosY1 = _Rect.p0.y;
	m_Width = _Rect.p1.x - _Rect.p0.x;
	m_Height = _Rect.p1.y - _Rect.p0.y;
	int wLog2 = Log2(m_Width);
	int hLog2 = Log2(m_Height);
	m_iNodeMap = hLog2*8 + wLog2;

}

int CP2H_Node::IsLeaf()
{
	return ((m_iNode1 == -1) && (m_iNode2 == -1));
}
// ----------------------------------------------------------------
//#define CPOW2ALLOC_CHECKINTEGRITY_LINK
//#define CPOW2ALLOC_FAILNOTIFY

// ----------------------------------------------------------------
//  CPow2Alloc
// ----------------------------------------------------------------
void CPow2Alloc::Create(int _nBlocks, int _nHeaps, int _HeapExtent, int _nNodesHeap, void* _pContext)
{
	m_piBlockNodes = NULL;
	m_pNodes = NULL;
	m_nBlocks = _nBlocks;
	m_nHeaps = _nHeaps;
	m_HeapExtent = _HeapExtent;
	m_HeapExtentLog2 = Log2(_HeapExtent);
	m_HeapExtentInverse = 1.0/fp32(_HeapExtent);
	m_pContext = _pContext;
	m_Time = 1234;
	if (!IsPow2(m_HeapExtent)) Error_static("CPow2Alloc::Create", "Heap squares must be 2^x.");

	FillW(&m_iNodeMap, sizeof(m_iNodeMap) >> 1, -1);
	FillW(&m_iNodeMapEnd, sizeof(m_iNodeMapEnd) >> 1, -1);

	m_piBlockNodes = DNew(int16) int16[_nBlocks];
	if (!m_piBlockNodes) Error_static("CPow2Alloc::Create", "Out of memory.");
	FillW(m_piBlockNodes, _nBlocks, -1);
//	LogFile(CStrF("Consuming %d bytes for ID-Table", 2*_nBlocks));

	m_pNodes = DNew(CP2H_Node) CP2H_Node[_nNodesHeap];
	if (!m_pNodes) Error_static("CPow2Alloc::Create", "Out of memory.");
//	LogFile(CStrF("Consuming %d bytes for %d CP2H_Node.", sizeof(CP2H_Node)*_nNodesHeap, _nNodesHeap));

	m_spNodeAllocHeap = MNew1(CIDHeap, _nNodesHeap);
	if (!m_spNodeAllocHeap) Error_static("CPow2Alloc::Create", "Out of memory.");

	{
		CP2H_Node RootNode;
		RootNode.SetRect(CRct(0, 0, m_HeapExtent, m_HeapExtent));

		m_iHeapTreeRoot.SetLen(_nHeaps);
		for(int iHeap = 0; iHeap < m_nHeaps; iHeap++)
		{
			int iNode = m_spNodeAllocHeap->AllocID();
			if (iNode != iHeap) Error_static("CPow2Alloc::Create", "Unexpected ID.");
			m_pNodes[iNode].Clear();
			m_iHeapTreeRoot[iHeap] = iNode;
			RootNode.m_iNodeRoot = iNode;
			m_pNodes[iNode] = RootNode;
			Link(iNode);
		}
	}

	m_lHeapAllocated.SetLen(m_nHeaps);
	FillChar(&m_lHeapAllocated[0], m_lHeapAllocated.ListSize(), 0);

	m_TimeLock.Reset();
	m_TimeStamp.Reset();
	m_TimeRelink.Reset();
	m_nLocks = 0;
	m_nStamps = 0;
}

/*CPow2Alloc::CPow2Alloc(int _nBlocks, int _nHeaps, int _HeapExtent, int _nNodesHeap, void* _pContext)
{
	Init(_nBlocks, _nHeaps, _HeapExtent, _nNodesHeap, _pContext);
}*/

CPow2Alloc::CPow2Alloc()
{
	m_pContext = NULL;
}

CPow2Alloc::~CPow2Alloc()
{
//	UnlockAll();
//LogFile("CPow2Alloc::~CPow2Alloc (Begin)");
//	for(int i = 0; i < m_nBlocks; i++)
//		if (m_piBlockNodes[i] >= 0) FreeLeaf(m_piBlockNodes[i]);

//LogFile("CPow2Alloc::~CPow2Alloc (Progress 1)");
	if (m_pNodes) { delete[] m_pNodes; m_pNodes = NULL; }
//LogFile("CPow2Alloc::~CPow2Alloc (Progress 2)");
	if (m_piBlockNodes) { delete[] m_piBlockNodes; m_piBlockNodes = NULL; }
//LogFile("CPow2Alloc::~CPow2Alloc (End)");
}

void CPow2Alloc::Tick()
{
	m_Time++;
	if (m_Time > 0x7f000000)
	{
/*		UnlockAll();
		for(int i = 0; i < m_nBlocks; i++)
			if (m_piBlockNodes[i] >= 0) FreeBlock(i);
*/		
		m_Time = 123;
	}

/*	CStr s("Allocated: ");
	for(int i = 0; i < m_nHeaps; i++)
		s += CStrF("%d, ", m_lHeapAllocated[i]);
	LogFile(s);*/

#ifdef CPOW2ALLOC_FAILNOTIFY
//	LogFile(T_String("Lock: ", m_TimeLock) + T_String(", Stamp: ", m_TimeStamp) + T_String(", Relink: ", m_TimeRelink) + CStrF(", nLocks %d, nStamps %d", m_nLocks, m_nStamps));
#endif

	m_TimeLock.Reset();
	m_TimeStamp.Reset();
	m_TimeRelink.Reset();
	m_nLocks = 0;
	m_nStamps = 0;
}

int CPow2Alloc::Lock(int _ID, CRct& _DestRect, CVec2Dfp32& _TxtScale, CVec2Dfp32& _TxtAdd)
{
	if (m_piBlockNodes[_ID] == -1) 
		m_piBlockNodes[_ID] = Allocate(_ID);

	m_TimeLock.Start();
	m_nLocks++;
	if (m_piBlockNodes[_ID] != -1)
	{
		// Allocated
		int iNode = m_piBlockNodes[_ID];
		CP2H_Node* pN = &m_pNodes[iNode];
		_DestRect.p0.x = pN->m_PosX1;
		_DestRect.p0.y = pN->m_PosY1;
		_DestRect.p1.x = pN->m_PosX1 + pN->m_Width;
		_DestRect.p1.y = pN->m_PosY1 + pN->m_Height;

		fp32 Inv = m_HeapExtentInverse;
		_TxtScale.k[0] = fp32(pN->m_Width) * Inv;
		_TxtScale.k[1] = fp32(pN->m_Height) * Inv;
		_TxtAdd.k[0] = fp32(pN->m_PosX1) * Inv;
		_TxtAdd.k[1] = fp32(pN->m_PosY1) * Inv;
		TimeStamp(iNode);

//LogFile(CStrF("Lock %d, Node: %d, Root: %d", _ID, iNode, m_pNodes[iNode].m_iNodeRoot));
		if (m_pNodes[iNode].m_iNodeRoot < 0)
			ConOut(CStrF("Weird iNodeRoot: %d", m_pNodes[iNode].m_iNodeRoot));
		m_TimeLock.Stop();
		return m_pNodes[iNode].m_iNodeRoot;
	}
	m_TimeLock.Stop();
	return -1;
}

int CPow2Alloc::Allocate(int _ID)
{
//	LogFile(CStrF("(CP2Alloc::Allocate) ID %d", _ID));

	// Free nodes
	while(m_spNodeAllocHeap->MaxAvail() < 20)
	{
		int BestTime = 1;
		int iBestNode = -1;
		for(int i = 0; i < 64; i++)
		{
			int iNode = m_iNodeMapEnd[i];
			if ((iNode >= 0) && 
				(m_Time - m_pNodes[iNode].m_TimeStamp > BestTime) && 
				(!m_pNodes[iNode].IsLeaf()))
			{
				iBestNode = iNode;
				BestTime = m_Time - m_pNodes[iNode].m_TimeStamp;
			}
		}

		if (iBestNode < 0) 
		{
//			ConOut(CStr("Failed to free nodes by oldest node-map."));
			int i = 0;
			while((m_spNodeAllocHeap->MaxAvail() < 20) && (i < 64))
			{
				int BestTime = 1;
				int iBestNode = -1;
				int iNode = m_iNodeMapEnd[i];
				while((m_spNodeAllocHeap->MaxAvail() < 20) && 
					  (iNode > -1) && 
					  (m_Time - m_pNodes[iNode].m_TimeStamp >= 2))
				{
					if (!m_pNodes[iNode].IsLeaf())
					{
//						ConOut(CStrF("APA free tree %d", iBestNode));
						DelTree(m_pNodes[iNode].m_iNode1);
						DelTree(m_pNodes[iNode].m_iNode2);
						m_pNodes[iNode].m_iNode1 = -1;
						m_pNodes[iNode].m_iNode2 = -1;
					}
					iNode = m_pNodes[iNode].m_iNodePrev;
				}
				i++;
			}

			if (m_spNodeAllocHeap->MaxAvail() >= 20) break;

#ifdef CPOW2ALLOC_FAILNOTIFY
			// DUMP!
			LogFile(CStrF("Time is: %d", m_Time));
			for(i = 0; i < 8; i++)
			{
				CStr s;
				for(int j = 0; j < 8; j++)
				{
					int iNode = m_iNodeMapEnd[i*8 + j];
					if (m_pNodes[iNode].IsLeaf()) iNode = -1;
					int Time = (iNode >= 0) ? m_pNodes[iNode].m_TimeStamp : 0;
					s += CStrF("(%d, %d), ", iNode, Time);
				}
				LogFile(s);
			}

			ConOut("Failed to free up some nodes.");
			LogFile("Failed to free up some nodes.");
#endif
			// Out of nodes, can't free any, NO alloc!
			return -1;
		}

		DelTree(m_pNodes[iBestNode].m_iNode1);
		DelTree(m_pNodes[iBestNode].m_iNode2);
		m_pNodes[iBestNode].m_iNode1 = -1;
		m_pNodes[iBestNode].m_iNode2 = -1;
//		ConOut(CStrF("Freeing tree %d", iBestNode));
	}

	// BHA!, allocate...
	CPnt BlockSize = GetBlockSize(_ID);
	int w = Log2(BlockSize.x);
	int h = Log2(BlockSize.y);
	if ((w < 0) || (w >= 8)) Error_static("CPow2Alloc::Lock", "Invalid block width.");
	if ((h < 0) || (h >= 8)) Error_static("CPow2Alloc::Lock", "Invalid block height.");
	int iNodeMap = h*8 + w;

	// Go through the iNodeMapEnd
	int iNode = -1;
	int heapshift = m_HeapExtentLog2+1;
	int BestDist = 10000000;
	int BestdTime = 1;
	for(int ph = h; ph < heapshift; ph++)
	{
		int hDist = ph-h;

		for(int pw = w; ((pw < heapshift)); pw++)
		{
			int iAvail = pw + ph*8;
			if (m_iNodeMapEnd[iAvail] != -1)
			{
				int iN = m_iNodeMapEnd[iAvail];
				int dT = m_Time-m_pNodes[iN].m_TimeStamp;
				if (dT > BestdTime)
				{
					iNode = iN;
					BestDist = pw-w + hDist;
					BestdTime = dT;
				}
				else
					if ((dT == BestdTime) && (pw-w + hDist < BestDist))
					{
						iNode = iN;
						BestDist = pw-w + hDist;
						BestdTime = dT;
					}

				if ((m_pNodes[iN].m_Width != (1 << pw)) ||
					(m_pNodes[iN].m_Height != (1 << ph)))
					LogFile(CStrF("Invalid link! %d, (%d, %d)", iN, BlockSize.x, BlockSize.y));
			}
		}
	}

	// Did we fail misserably?
	if (iNode == -1) 
	{
#ifdef CPOW2ALLOC_FAILNOTIFY
		ConOut(CStrF("(CP2Alloc::Allocate) ID %d  (FAILURE!)", _ID));
		LogFile(CStrF("(CP2Alloc::Allocate) ID %d  (FAILURE!)", _ID));
#endif
		return -1;		// Forgett it, no space left and nothing can be freed!
	}

	int iLeaf = AllocAtNode(iNode, _ID, w, h);
#ifdef CPOW2ALLOC_FAILNOTIFY
	if (iLeaf == -1) 
	{
		ConOut(CStrF("(CP2Alloc::Allocate) ID %d  (FAILURE! (2))  Avail: %d", _ID, m_spNodeAllocHeap->MaxAvail()));
		LogFile(CStrF("(CP2Alloc::Allocate) ID %d  (FAILURE! (2))  Avail: %d", _ID, m_spNodeAllocHeap->MaxAvail()));
	}
#endif
	return iLeaf;
}

void CPow2Alloc::FreeLeaf(int _iNode)
{
	if (m_pNodes[_iNode].m_UserID != -1)
	{
		m_lHeapAllocated[m_pNodes[_iNode].m_iNodeRoot] -= 
			m_pNodes[_iNode].m_Width * m_pNodes[_iNode].m_Height;

		m_piBlockNodes[m_pNodes[_iNode].m_UserID] = -1;

		FreeBlock(m_pNodes[_iNode].m_UserID);
		RelinkToEnd(_iNode);
	}
}

void CPow2Alloc::DelTree(int _iNode)
{
	if (_iNode < 0) return;

	if (!m_pNodes[_iNode].IsLeaf())
	{
		DelTree(m_pNodes[_iNode].m_iNode1);
		DelTree(m_pNodes[_iNode].m_iNode2);
		m_pNodes[_iNode].m_iNode1 = -1;
		m_pNodes[_iNode].m_iNode2 = -1;
	}
	else
		FreeLeaf(_iNode);

	Unlink(_iNode);
	m_spNodeAllocHeap->FreeID(_iNode);
}

int CPow2Alloc::CreateNode(CRct _Rect, int _iParent, int _iRoot)
{
	CP2H_Node Node;
	Node.SetRect(_Rect);

	int iNode = m_spNodeAllocHeap->AllocID();
	if (iNode < 0) 
	{
		if (m_spNodeAllocHeap->MaxAvail() > 0)
		{
			ConOut("(CPow2Alloc::CreateNode) INTERNAL ERROR!");
			LogFile("(CPow2Alloc::CreateNode) INTERNAL ERROR!");
		}

		ConOut("(CPow2Alloc::CreateNode) Out of nodes!");
		LogFile("(CPow2Alloc::CreateNode) Out of nodes!");
		return -1;
	}
	Node.m_iNodeRoot = _iRoot;
	Node.m_iNodeParent = _iParent;
	m_pNodes[iNode] = Node;
	Link(iNode);
	RelinkToEnd(iNode);
	return iNode;
}

int CPow2Alloc::CreateLeaf(int _iNode, int _ws, int _hs)
{
	int w = 1 << _ws;
	int h = 1 << _hs;

	if ((w > m_pNodes[_iNode].m_Width) ||
		(h > m_pNodes[_iNode].m_Height))
		Error_static("CPow2Alloc::CreateLeaf", CStrF("Internal error. (%dx%d does not fit into %dx%d)", w, h, 
			m_pNodes[_iNode].m_Width, m_pNodes[_iNode].m_Height));


	if ((w == m_pNodes[_iNode].m_Width) &&
		(h == m_pNodes[_iNode].m_Height))
		return _iNode;


	if (w <  m_pNodes[_iNode].m_Width)
	{
		CRct r(m_pNodes[_iNode].m_PosX1, m_pNodes[_iNode].m_PosY1, 
			m_pNodes[_iNode].m_PosX1 + m_pNodes[_iNode].m_Width, m_pNodes[_iNode].m_PosY1 + m_pNodes[_iNode].m_Height);
		int rw = r.GetWidth();
		int rh = r.GetHeight();
		CRct r1(r.p0, CPnt(r.p0.x + (rw>>1), r.p0.y + rh));
		CRct r2(r.p0.x + (rw>>1), r.p0.y, r.p0.x + rw, r.p1.y);

//LogFile(CStrF("Splitting: %dx%d-%dx%d", r.p0.x, r.p0.y, r.p1.x, r.p1.y));
//LogFile(CStrF("R1: %d,%d-%d,%d", r1.p0.x, r1.p0.y, r1.p1.x, r1.p1.y));
//LogFile(CStrF("R2: %d,%d-%d,%d", r2.p0.x, r2.p0.y, r2.p1.x, r2.p1.y));
if (r1.GetWidth() != rw >> 1) LogFile("Width missmatch R1!");
if (r1.GetHeight() != rh) LogFile("Height missmatch R1!");
if (r2.GetWidth() != rw >> 1) LogFile("Width missmatch R2!");
if (r2.GetHeight() != rh) LogFile("Height missmatch R2!");

		if ((r1.p0.x < r.p0.x) || (r1.p0.y < r.p0.y) || (r1.p1.x > r.p1.x) || (r1.p1.y > r.p1.y) ||
			(r2.p0.x < r.p0.x) || (r2.p0.y < r.p0.y) || (r2.p1.x > r.p1.x) || (r2.p1.y > r.p1.y))
			Error_static("CPow2Alloc::CreateLeaf", "R1 or R2 not inside.");

		m_pNodes[_iNode].m_iNode1 = CreateNode(r1, _iNode, m_pNodes[_iNode].m_iNodeRoot);
		m_pNodes[_iNode].m_iNode2 = CreateNode(r2, _iNode, m_pNodes[_iNode].m_iNodeRoot);
		if ((m_pNodes[_iNode].m_iNode1 < 0) || (m_pNodes[_iNode].m_iNode2 < 0))
		{
			DelTree(m_pNodes[_iNode].m_iNode1);
			DelTree(m_pNodes[_iNode].m_iNode2);
			m_pNodes[_iNode].m_iNode1 = -1;
			m_pNodes[_iNode].m_iNode1 = -2;
			return -1;
		}

		return CreateLeaf(m_pNodes[_iNode].m_iNode1, _ws, _hs);
	}
	else
		if (h <  m_pNodes[_iNode].m_Height)
		{
			CRct r(m_pNodes[_iNode].m_PosX1, m_pNodes[_iNode].m_PosY1, 
				m_pNodes[_iNode].m_PosX1 + m_pNodes[_iNode].m_Width, m_pNodes[_iNode].m_PosY1 + m_pNodes[_iNode].m_Height);
			int rw = r.GetWidth();
			int rh = r.GetHeight();
			CRct r1(r.p0, CPnt(r.p1.x, r.p0.y + (rh>>1)));
			CRct r2(r.p0.x, r.p0.y + (rh>>1), r.p1.x, r.p1.y);
//LogFile(CStrF("Splitting: %dx%d-%dx%d", r.p0.x, r.p0.y, r.p1.x, r.p1.y));
//LogFile(CStrF("R1: %d,%d-%d,%d", r1.p0.x, r1.p0.y, r1.p1.x, r1.p1.y));
//LogFile(CStrF("R2: %d,%d-%d,%d", r2.p0.x, r2.p0.y, r2.p1.x, r2.p1.y));
if (r1.GetWidth() != rw) LogFile("Width missmatch R1!");
if (r1.GetHeight() != rh >> 1) LogFile("Height missmatch R1!");
if (r2.GetWidth() != rw) LogFile("Width missmatch R2!");
if (r2.GetHeight() != rh >> 1) LogFile("Height missmatch R2!");

		if ((r1.p0.x < r.p0.x) || (r1.p0.y < r.p0.y) || (r1.p1.x > r.p1.x) || (r1.p1.y > r.p1.y) ||
			(r2.p0.x < r.p0.x) || (r2.p0.y < r.p0.y) || (r2.p1.x > r.p1.x) || (r2.p1.y > r.p1.y))
			Error_static("CPow2Alloc::CreateLeaf", "R1 or R2 not inside.");

			m_pNodes[_iNode].m_iNode1 = CreateNode(r1, _iNode, m_pNodes[_iNode].m_iNodeRoot);
			m_pNodes[_iNode].m_iNode2 = CreateNode(r2, _iNode, m_pNodes[_iNode].m_iNodeRoot);
			if ((m_pNodes[_iNode].m_iNode1 < 0) || (m_pNodes[_iNode].m_iNode2 < 0))
			{
				DelTree(m_pNodes[_iNode].m_iNode1);
				DelTree(m_pNodes[_iNode].m_iNode2);
				m_pNodes[_iNode].m_iNode1 = -1;
				m_pNodes[_iNode].m_iNode1 = -2;
				return -1;
			}

			return CreateLeaf(m_pNodes[_iNode].m_iNode1, _ws, _hs);
		}

	Error_static("CPow2Alloc::CreateLeaf", "Can't be here.");
	return -1;
}

int CPow2Alloc::AllocAtNode(int _iNode, int _ID, int _ws, int _hs)
{
	if (!m_pNodes[_iNode].IsLeaf())
	{
		DelTree(m_pNodes[_iNode].m_iNode1);
		DelTree(m_pNodes[_iNode].m_iNode2);
		m_pNodes[_iNode].m_iNode1 = -1;
		m_pNodes[_iNode].m_iNode2 = -1;
	}

	int iLeaf = CreateLeaf(_iNode, _ws, _hs);
	if (iLeaf >= 0) 
	{
//	LogFile(CStrF("(CP2Alloc::AllocAtNode) ID %d, (%dx%d), iNode %d, Map %d (%dx%d)", 
//		_ID, 1<<_ws, 1<<_hs, _iNode, m_pNodes[_iNode].m_iNodeRoot, m_pNodes[_iNode].m_AbsPos.GetWidth(), m_pNodes[_iNode].m_AbsPos.GetHeight()));

		m_lHeapAllocated[m_pNodes[iLeaf].m_iNodeRoot] += 
			m_pNodes[iLeaf].m_Width * m_pNodes[iLeaf].m_Height;

		m_pNodes[iLeaf].m_UserID = _ID;
		AllocBlock(_ID, iLeaf);
		TimeStamp(iLeaf);
	}
	return iLeaf;
}
// -------------------------------------------------------------------
void CPow2Alloc::CheckIntegrity(int _Type)
{
	for(int y = 0; y < 8; y++)
		for(int x = 0; x < 8; x++)
		{
			int iNode = m_iNodeMap[x + y*8];
			int iNodeLast = -1;
			int w = 1 << x;
			int h = 1 << y;
			int nLoops = 0;
			while(iNode != -1)
			{
				if ((m_pNodes[iNode].m_Width != w) ||
					(m_pNodes[iNode].m_Height != h))
					LogFile(CStrF("(CheckIntegrity) Invalid node size: %dx%d", m_pNodes[iNode].m_Width, m_pNodes[iNode].m_Height));

				if (m_pNodes[iNode].m_iNodeMap != x+y*8)
					LogFile(CStrF("(CheckIntegrity) Invalid node map index: %d (%d)", m_pNodes[iNode].m_iNodeMap, x+y*8));

				LogFile(CStrF("Node %d, Prev %d, Next %d, Last %d", iNode, m_pNodes[iNode].m_iNodePrev, m_pNodes[iNode].m_iNodeNext, iNodeLast));
				if (iNodeLast != m_pNodes[iNode].m_iNodePrev)
					Error_static("CPow2Alloc::CheckIntegrity", "Invalid prev-link.");
				iNodeLast = iNode;
				if (m_pNodes[iNode].m_iNodeNext != -1)
					if (m_pNodes[iNode].m_TimeStamp <
						m_pNodes[m_pNodes[iNode].m_iNodeNext].m_TimeStamp)
						Error_static("CPow2Alloc::CheckIntegrity", CStrF("Node %d, Next %d, Time %d, NextTime %d", 
							iNode, m_pNodes[iNode].m_iNodeNext, m_pNodes[iNode].m_TimeStamp,
							m_pNodes[m_pNodes[iNode].m_iNodeNext].m_TimeStamp));

				iNode = m_pNodes[iNode].m_iNodeNext;
				nLoops++;
				if (nLoops > m_nBlocks)
				{
					LogFile("(CheckIntegrity) Possible link loop.");
					break;
				}
			}

			if (m_iNodeMapEnd[x + y*8] != iNodeLast)
				LogFile("(CheckIntegrity) Invalid NodeMapEnd index.");
		}
}

// -------------------------------------------------------------------
void CPow2Alloc::Link(int _iNode)
{
#ifdef CPOW2ALLOC_CHECKINTEGRITY_LINK
	CheckIntegrity(0);
	LogFile("Pre-Link");
#endif
	CP2H_Node* pN = &m_pNodes[_iNode];
	if (pN->m_iNodeNext != -1) Error_static("CPow2Alloc::Link", "Node already linked. (iNodeNext != -1)");
	if (pN->m_iNodePrev != -1) Error_static("CPow2Alloc::Link", "Node already linked. (iNodePrev != -1)");

	int iNodeMap = m_pNodes[_iNode].m_iNodeMap;

	pN->m_iNodeNext = m_iNodeMap[iNodeMap];
	m_iNodeMap[iNodeMap] = _iNode;

	if (pN->m_iNodeNext != -1)
		m_pNodes[pN->m_iNodeNext].m_iNodePrev = _iNode;
	else
		m_iNodeMapEnd[iNodeMap] = _iNode;
#ifdef CPOW2ALLOC_CHECKINTEGRITY_LINK
	CheckIntegrity(0);
	LogFile("Post-Link");
#endif
}

void CPow2Alloc::Unlink(int _iNode)
{
#ifdef CPOW2ALLOC_CHECKINTEGRITY_LINK
	CheckIntegrity(0);
	LogFile("Pre-Unlink");
#endif
	CP2H_Node* pN = &m_pNodes[_iNode];
	int iNodeMap = pN->m_iNodeMap;

	// Ugh!
	if (pN->m_iNodePrev == -1)
		m_iNodeMap[iNodeMap] = pN->m_iNodeNext;
	else
		m_pNodes[pN->m_iNodePrev].m_iNodeNext = pN->m_iNodeNext;

	if (pN->m_iNodeNext == -1)
		m_iNodeMapEnd[iNodeMap] = pN->m_iNodePrev;
	else
		m_pNodes[pN->m_iNodeNext].m_iNodePrev = pN->m_iNodePrev;


	pN->m_iNodeNext = -1;
	pN->m_iNodePrev = -1;
#ifdef CPOW2ALLOC_CHECKINTEGRITY_LINK
	CheckIntegrity(0);
	LogFile("Post-Unlink");
#endif
}

void CPow2Alloc::RelinkToBegin(int _iNode)
{
#ifdef CPOW2ALLOC_CHECKINTEGRITY_LINK
	CheckIntegrity(0);
	LogFile("Pre-RelinkBegin");
#endif
	CP2H_Node* pN = &m_pNodes[_iNode];
	if (pN->m_iNodePrev == -1) return;

	int iNodeMap = pN->m_iNodeMap;
	int iFirst = m_iNodeMap[iNodeMap];
	m_iNodeMap[iNodeMap] = _iNode;
	m_pNodes[iFirst].m_iNodePrev = _iNode;

	// Unlink
	m_pNodes[pN->m_iNodePrev].m_iNodeNext = pN->m_iNodeNext;
	if (pN->m_iNodeNext == -1)
		m_iNodeMapEnd[iNodeMap] = pN->m_iNodePrev;
	else
		m_pNodes[pN->m_iNodeNext].m_iNodePrev = pN->m_iNodePrev;


	pN->m_iNodeNext = iFirst;
	pN->m_iNodePrev = -1;
#ifdef CPOW2ALLOC_CHECKINTEGRITY_LINK
	CheckIntegrity(0);
	LogFile("Post-RelinkBegin");
#endif
}

void CPow2Alloc::RelinkToEnd(int _iNode)
{
#ifdef CPOW2ALLOC_CHECKINTEGRITY_LINK
	CheckIntegrity(0);
	LogFile(CStrF("Pre-ReLinkEnd %d", _iNode));
#endif
	CP2H_Node* pN = &m_pNodes[_iNode];
	if (pN->m_iNodeNext == -1) return;

	int iNodeMap = pN->m_iNodeMap;
	int iLast = m_iNodeMapEnd[iNodeMap];
	m_iNodeMapEnd[iNodeMap] = _iNode;
	m_pNodes[iLast].m_iNodeNext = _iNode;

	// Unlink
	if (pN->m_iNodePrev == -1)
		m_iNodeMap[iNodeMap] = pN->m_iNodeNext;
	else
		m_pNodes[pN->m_iNodePrev].m_iNodeNext = pN->m_iNodeNext;

	m_pNodes[pN->m_iNodeNext].m_iNodePrev = pN->m_iNodePrev;


	pN->m_iNodePrev = iLast;
	pN->m_iNodeNext= -1;
#ifdef CPOW2ALLOC_CHECKINTEGRITY_LINK
	CheckIntegrity(0);
	LogFile(CStrF("Post-ReLinkEnd %d", _iNode));
#endif
}

void CPow2Alloc::TimeStamp(int _iNode)
{
	int iParent = _iNode;
	while((iParent != -1) && (m_pNodes[iParent].m_TimeStamp < m_Time))
	{
		m_nStamps++;
		CP2H_Node* pN = &m_pNodes[iParent];
		if (pN->m_iNodePrev != -1)
		{
			int iNodeMap = pN->m_iNodeMap;
			int iFirst = m_iNodeMap[iNodeMap];
			m_iNodeMap[iNodeMap] = iParent;
			m_pNodes[iFirst].m_iNodePrev = iParent;

			// Unlink
			m_pNodes[pN->m_iNodePrev].m_iNodeNext = pN->m_iNodeNext;
			if (pN->m_iNodeNext == -1)
				m_iNodeMapEnd[iNodeMap] = pN->m_iNodePrev;
			else
				m_pNodes[pN->m_iNodeNext].m_iNodePrev = pN->m_iNodePrev;

			pN->m_iNodeNext = iFirst;
			pN->m_iNodePrev = -1;
		}
		pN->m_TimeStamp = m_Time;
		iParent = pN->m_iNodeParent;
	}
}

/*
void CPow2Alloc::TimeStamp(int _iNode)
{
//	m_TimeStamp -= GetCPUClock();
//CStr s("TimeStamp: ");
	int iParent = _iNode;
	while((iParent != -1) && (m_pNodes[iParent].m_TimeStamp < m_Time))
	{
//s += CStrF("%d, ", iParent);
		m_nStamps++;
//	m_TimeRelink -= GetCPUClock();
		RelinkToBegin(iParent);
//	T_Stop(m_TimeRelink);
		m_pNodes[iParent].m_TimeStamp = m_Time;
		iParent = m_pNodes[iParent].m_iNodeParent;
	}
//LogFile(s);
//	T_Stop(m_TimeStamp);
}

*/



