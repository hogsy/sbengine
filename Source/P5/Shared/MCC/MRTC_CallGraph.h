
#ifndef __INC_MRTC_CALLGRAPH
#define __INC_MRTC_CALLGRAPH

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Call graph profiling stuff

	Author:			Magnus Högdahl

	Copyright:		Starbreeze Studios AB 1996,2003

	Contents:

	Comments:

	History:	
		030711:		File header added

\*____________________________________________________________________________________________*/

#include "MDA.h"
#include "MCC.h"
#ifdef M_Profile

class MRTC_CallGraph_ThreadLocalData;

class MRTC_CallGraphEntry
{
public:

	class CCompare
	{
	public:
		DIdsPInlineS static aint Compare(const MRTC_CallGraphEntry *_pFirst, const MRTC_CallGraphEntry *_pSecond, void *_pContext)
		{
			return CStrBase::stricmp(_pFirst->m_Name.Str(), _pSecond->m_Name.Str());
		}

		DIdsPInlineS static aint Compare(const MRTC_CallGraphEntry *_pTest, const char * _pKey, void *_pContext)
		{
			return CStrBase::stricmp(_pTest->m_Name.Str(), _pKey);
		}
	};
	DIdsTreeAVLAligned_Link(MRTC_CallGraphEntry, m_AVLLink, const char *, CCompare);


	int64 m_Clocks;
	int64 m_ClocksWasted;

	DIdsTreeAVLAligned_Tree(MRTC_CallGraphEntry, m_AVLLink, const char *, CCompare) m_Children;
	typedef DIdsTreeAVLAligned_Iterator(MRTC_CallGraphEntry, m_AVLLink, const char *, CCompare) CChildIterator;

	aint m_nCalls;
	CStr m_Name;
	MRTC_CallGraphEntry* m_pParent;
	MRTC_CallGraph_ThreadLocalData *m_pThreadLocal;

	MRTC_CallGraphEntry()
	{
		m_Clocks = 0;
		m_ClocksWasted = 0;
		m_nCalls = 0;
	}

	~MRTC_CallGraphEntry();

	virtual MRTC_CallGraphEntry* GetThis()
	{
		return this;
	}

	int64 AccumalateClockWaste_r();
	void Log_r(int _Depth, int64 _ParentClocks, int64 _TotalClocks);	// Returns accumulated clockwaste
	void Log_r(int _Depth, int64 _ParentClocks, int64 _TotalClocks, TArray<CStr> &_StrList);
	void Log2_r(int _Depth, int64 _ParentClocks, int64 _TotalClocks, TArray<CStr> &_StrList);
	void Write_r(CCFile* _pFile, int _Depth);
};

class MRTC_CallGraph_ThreadLocalData
{
public:
	MRTC_CallGraph_ThreadLocalData();
	~MRTC_CallGraph_ThreadLocalData();

	DLinkD_Link(MRTC_CallGraph_ThreadLocalData, m_Link);
	TCPool<MRTC_CallGraphEntry> m_EntryPool;
	MRTC_CallGraphEntry* m_pRoot;
	MRTC_CallGraphEntry* m_pCurrent;
	int32 m_nDisable;
	int m_Depth;
	int m_State;
	CStr m_ThreadName;

	void Push(const char* _pFunction);
	void AddClocks(int64 _Clocks);
	void AddWaste(int64 _Clocks);
	void Pop(int64 _ClocksWasted);
	void Clear();
	void Start();
	void Stop();
	void Disable();
	void Enable();
};

enum
{
	MRTC_CG_ISRUNNING = 1,
	MRTC_CG_PENDINGSTART = 2,
};

class MRTC_CallGraph : public CReferenceCount
{
public:

	TMRTC_ThreadLocal<MRTC_CallGraph_ThreadLocalData> m_ThreadLocal;

	NThread::CMutual m_Lock;
	DLinkD_List(MRTC_CallGraph_ThreadLocalData, m_Link) m_ThreadLocalList;
	typedef DLinkD_Iter(MRTC_CallGraph_ThreadLocalData, m_Link) CThreadDataIter;

	int64 m_EndTime;
	aint m_nRunning;
	bool m_bLogOnFinish;
	uint64 m_WasteCorrectionClocks;

	void BlockUntilNoThreadsRunning();

	void RegisterRunning();
	void UnRegisterRunning();

	MRTC_CallGraph();
	~MRTC_CallGraph();

	void LogTimes();
	void TraceTimes();

	void Start(const fp64 _dTime, bool _bLog = true);

	// Start a measure that lasts only till root is Reached
	void Start();
	void Stop();
	void Disable();
	void Enable();

	void GetStrList(TArray<CStr> &_StrList);
	void GetStrList2(TArray<CStr>& _StrList);

	void Clear();
};

class MRTC_CallGraphMeasure
{

public:
	MRTC_CallGraphMeasure()
	{
		MRTC_GetObjectManager()->m_pCallGraph->Start();
	}

	~MRTC_CallGraphMeasure()
	{
		MRTC_GetObjectManager()->m_pCallGraph->Stop();
	}
};

class MRTC_CallGraphMeasureWrite
{

public:
	MRTC_CallGraphMeasureWrite()
	{
		MRTC_GetObjectManager()->m_pCallGraph->Start();
	}

	~MRTC_CallGraphMeasureWrite()
	{
		MRTC_GetObjectManager()->m_pCallGraph->Stop();
		MRTC_GetObjectManager()->m_pCallGraph->LogTimes();
		
	}
};

class MRTC_CallGraphMeasureTrace
{

public:
	MRTC_CallGraphMeasureTrace()
	{
		MRTC_GetObjectManager()->m_pCallGraph->Start();
	}

	~MRTC_CallGraphMeasureTrace()
	{
		MRTC_GetObjectManager()->m_pCallGraph->Stop();
		MRTC_GetObjectManager()->m_pCallGraph->TraceTimes();
		
	}
};


class MRTC_CallGraphDisable
{
public:
	MRTC_CallGraphDisable()
	{
		MRTC_GetObjectManager()->m_pCallGraph->Disable();
	}

	~MRTC_CallGraphDisable()
	{
		MRTC_GetObjectManager()->m_pCallGraph->Enable();		
	}
};


#define M_CALLGRAPH MRTC_CallGraphMeasure CallGraph;
#define M_CALLGRAPHW MRTC_CallGraphMeasureWrite CallGraph;
#define M_CALLGRAPHT MRTC_CallGraphMeasureTrace CallGraph;
#define MSCOPE_DISABLE MRTC_CallGraphDisable CallGraphDisable;

#else

#define M_CALLGRAPH ((void)0)
#define M_CALLGRAPHW ((void)0)
#define M_CALLGRAPHT ((void)0)
#define MSCOPE_DISABLE ((void)0)

#endif

#endif // __INC_MRTC_CALLGRAPH
