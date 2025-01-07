
#include "MRTC_CallGraph.h"
#include "MFile.h"

#ifdef M_Profile
MRTC_CallGraphEntry::~MRTC_CallGraphEntry()
{
	while (m_Children.GetRoot())
	{
		MRTC_CallGraphEntry *pToDelete = m_Children.GetRoot();
		m_Children.f_Remove(pToDelete);
		m_pThreadLocal->m_EntryPool.Delete(pToDelete);
	}
}


int64 MRTC_CallGraphEntry::AccumalateClockWaste_r()
{
	CChildIterator Iter = m_Children;

	while(Iter)
	{
		m_ClocksWasted += Iter->AccumalateClockWaste_r();
		++Iter;
	}

	return m_ClocksWasted;
}

void MRTC_CallGraphEntry::Write_r(CCFile* _pFile, int _Depth)
{
	CFStr Indent = CFStr(' ', _Depth*4);
	const fp64 CR = MGetCPUFrequencyRecp();
	_pFile->Writeln(CStrF("%s%s, %f (%f - %f)", Indent.Str(), m_Name.Str(), (m_Clocks - m_ClocksWasted) * CR, m_Clocks * CR, m_ClocksWasted * CR));

	CChildIterator Iter = m_Children;
	while(Iter)
	{
		Iter->Write_r(_pFile, _Depth+1);
		++Iter;
	}
}

void MRTC_CallGraphEntry::Log_r(int _Depth, int64 _ParentClocks, int64 _TotalClocks)
{
	if (!m_Clocks)
		return;

	CFStr Indent;
	for(int i = 0; i < _Depth; i++)
		strcpy(&Indent.GetStr()[i*4], "   .");

	LogFile(CStrF("%6.2f  %s%6.2f   %s (%d, %.2f)", 
		fp64(100.0) * fp64((m_Clocks - m_ClocksWasted)) / fp64(_TotalClocks), Indent.Str(), 
		fp64(100.0) * fp64((m_Clocks - m_ClocksWasted)) / fp64(_ParentClocks),
		m_Name.Str(), m_nCalls, m_ClocksWasted / m_Clocks));

	CChildIterator Iter = m_Children;
	int64 ChildClocks = 0;
	int64 ChildClocksW = 0;
	while(Iter)
	{
		Iter->Log_r(_Depth+1, m_Clocks - m_ClocksWasted, _TotalClocks);
		ChildClocks += Iter->m_Clocks;
		ChildClocksW += Iter->m_ClocksWasted;
		++Iter;
	}

	if (!m_Children.IsEmpty())
	{
		int64 Other = m_Clocks - ChildClocks;
		int64 OtherW = m_ClocksWasted - ChildClocksW;
		if (fp64(Other)  > (0.01 * fp64(_ParentClocks)) )
		{
			LogFile(CStrF("%6.2f     .%s%6.2f   %s (%.2f)", 
				fp64(100 * (Other - OtherW)) / fp64(_TotalClocks), Indent.Str(), 
				fp64(100 * (Other - OtherW)) / fp64(m_Clocks - m_ClocksWasted), "Other", fp64(OtherW) / fp64(Other)));
		}
	}
}


void MRTC_CallGraphEntry::Log_r(int _Depth, int64 _ParentClocks, int64 _TotalClocks, TArray<CStr> &_StrList)
{
	if (!m_Clocks)
		return;
	CFStr Indent;
	for(int i = 0; i < _Depth; i++)
		strcpy(&Indent.GetStr()[i*4], "   .");
	
	if (!m_nCalls)
		m_nCalls = 1;

	_StrList.Add(CStrF("#%.2i %6.2f  %s%6.2f   %s (%d, %.2f (%.2f) ms, %d (%d) cyc)", 
		_Depth,
		fp64(100 * (m_Clocks - m_ClocksWasted)) / fp64(_TotalClocks), Indent.Str(), 
		fp64(100 * (m_Clocks - m_ClocksWasted)) / fp64(_ParentClocks),
		m_Name.Str(), 
		m_nCalls, 
		(fp64((m_Clocks - m_ClocksWasted) / m_nCalls) / MGetCPUFrequencyFp()) * 1000.0, 
		(fp64(m_Clocks - m_ClocksWasted) / MGetCPUFrequencyFp()) * 1000.0, 
		(int)((((int64)m_Clocks - (int64)m_ClocksWasted)) / m_nCalls),
		(int)(((int64)m_Clocks - (int64)m_ClocksWasted))
		)
		);
	
	CChildIterator Iter = m_Children;
	int64 ChildClocks = 0;
	int64 ChildClocksW = 0;
	while(Iter)
	{
		Iter->Log_r(_Depth+1, m_Clocks - m_ClocksWasted, _TotalClocks, _StrList);
		ChildClocks += Iter->m_Clocks;
		ChildClocksW += Iter->m_ClocksWasted;
		++Iter;
	}

	if (!m_Children.IsEmpty())
	{
		int64 Other = m_Clocks - ChildClocks;
		int64 OtherW = m_ClocksWasted - ChildClocksW;
//		if (Other / _ParentClocks > 0.01f)
		{
			_StrList.Add(CStrF("#%.2i %6.2f     .%s%6.2f   %s (%.2f, %d cyc)", 
				_Depth + 1,
				fp64(100 * (Other - OtherW)) / fp64(_TotalClocks), Indent.Str(), 
				fp64(100 * (Other - OtherW)) / fp64(m_Clocks - m_ClocksWasted), "Other", 
				((fp64(Other - OtherW) / MGetCPUFrequencyFp()) * 1000.0), 
				(int)((int64)Other - (int64)OtherW)));
		}
	}
}


void MRTC_CallGraphEntry::Log2_r(int _Depth, int64 _ParentClocks, int64 _TotalClocks, TArray<CStr> &_StrList)
{
	if (!m_Clocks)
		return;
	const char* pName = m_Name.Str();
	_StrList.Add(CStrF("#%.2i %s %ld %d", 
		_Depth,
		pName[0] ? pName : "-", 
		m_Clocks - m_ClocksWasted,
		m_nCalls));

	CChildIterator Iter = m_Children;
	int64 ChildClocks = 0;
	int64 ChildClocksW = 0;
	while (Iter)
	{
		Iter->Log2_r(_Depth+1, m_Clocks - m_ClocksWasted, _TotalClocks, _StrList);
		ChildClocks += Iter->m_Clocks;
		ChildClocksW += Iter->m_ClocksWasted;
		++Iter;
	}

	if (!m_Children.IsEmpty())
	{
		int64 Other = m_Clocks - ChildClocks;
		int64 OtherW = m_ClocksWasted - ChildClocksW;

		_StrList.Add(CStrF("#%.2i %s %ld %d", 
			_Depth + 1,
			"Other", 
			Other - OtherW,
			-1));
	}
}

// Used together with MRTC_CallGraphEntry::Log2_r
static int GetNumLinesNeeded_r(MRTC_CallGraphEntry* _pEntry)
{
	int nLines = 1;

	MRTC_CallGraphEntry::CChildIterator Iter = _pEntry->m_Children;
	while (Iter)
	{
		nLines += GetNumLinesNeeded_r(Iter);
		++Iter;
	}

	if (!_pEntry->m_Children.IsEmpty())
		nLines++;

	return nLines;
}



MRTC_CallGraph_ThreadLocalData::MRTC_CallGraph_ThreadLocalData()
{
	m_pRoot = NULL;
	m_pCurrent = NULL;
	m_nDisable = 0;
	m_Depth = 0;
	m_State = 0;

	Clear();

	MRTC_ObjectManager* pObjMgr = MRTC_GetObjectManager();
	{
		M_LOCK(pObjMgr->m_pCallGraph->m_Lock);
		pObjMgr->m_pCallGraph->m_ThreadLocalList.Insert(this);
	}
}

MRTC_CallGraph_ThreadLocalData::~MRTC_CallGraph_ThreadLocalData()
{
	if (m_pRoot)
		m_EntryPool.Delete(m_pRoot);
	MRTC_ObjectManager* pObjMgr = MRTC_GetObjectManager();
	{
		M_LOCK(pObjMgr->m_pCallGraph->m_Lock);
		pObjMgr->m_pCallGraph->m_ThreadLocalList.Remove(this);
	}
}

void MRTC_CallGraph_ThreadLocalData::Clear()
{
	if (m_pRoot)
	{
		m_EntryPool.Delete(m_pRoot);
	}

	m_pRoot = m_EntryPool.New();
	m_pRoot->m_pThreadLocal = this;

	m_pCurrent = m_pRoot;
	m_ThreadName = CStrF("Thread:0x%08x", (int32)(mint)MRTC_SystemInfo::OS_GetThreadID());
	m_pCurrent->m_Name = m_ThreadName;
}


MRTC_CallGraph::MRTC_CallGraph()
{
	MRTC_ObjectManager* pObjMgr = MRTC_GetObjectManager();
	pObjMgr->m_pCallGraph = this;


	m_bLogOnFinish = false;
	m_EndTime = 0;
	m_nRunning = 0;
	Clear();
	MRTC_CallGraph_ThreadLocalData* pThreadLocal = m_ThreadLocal.Get();
	pThreadLocal->m_State |= MRTC_CG_ISRUNNING;

 #ifdef MRTC_ENABLE_MSCOPE
	{ // Measure an empty MSCOPE to reduce hidden overhead
		m_WasteCorrectionClocks = 0;
		uint8 buf[sizeof(MRTC_Context)];
		MRTC_Context& Context = *(MRTC_Context*)buf;
		for (int i=0; i<3; i++)
		{
			new(buf) MRTC_Context("(empty)");
			Context.~MRTC_Context();

		}
		m_WasteCorrectionClocks = (Context.m_Clocks - Context.m_ClocksWaste);
	}
 #endif
	pThreadLocal->m_State = 0;
}

MRTC_CallGraph::~MRTC_CallGraph()
{
}


void MRTC_CallGraph::Start(const fp64 _dTime, bool _bLog)
{
	M_TRACEALWAYS("Starting performance log for %d seconds\n", _dTime);
	BlockUntilNoThreadsRunning();
	m_bLogOnFinish = _bLog;
	m_EndTime = (int64)(_dTime * (fp64)MGetCPUFrequencyFp()) + MGetCPUClock();

	CThreadDataIter Iter = m_ThreadLocalList;

	while (Iter)
	{
		Iter->m_State &= ~MRTC_CG_ISRUNNING;
		Iter->m_State |= MRTC_CG_PENDINGSTART;
		++Iter;
	}
}


void MRTC_CallGraph_ThreadLocalData::Start()
{
	Clear();
	m_pRoot->m_Clocks = -MGetCPUClock();
	m_State &= ~MRTC_CG_PENDINGSTART;
	m_State |= MRTC_CG_ISRUNNING;
	static MRTC_CallGraph* pCallGraph = MRTC_GetObjectManager()->m_pCallGraph;
	pCallGraph->RegisterRunning();
}

void MRTC_CallGraph_ThreadLocalData::Stop()
{
	if (!(m_State & MRTC_CG_ISRUNNING)) 
		return;

	m_pRoot->m_Clocks += MGetCPUClock();
	m_State &= ~MRTC_CG_ISRUNNING;
	m_pRoot->AccumalateClockWaste_r();

	static MRTC_CallGraph* pCallGraph = MRTC_GetObjectManager()->m_pCallGraph;
	pCallGraph->UnRegisterRunning();
}

void MRTC_CallGraph_ThreadLocalData::Disable()
{
	MRTC_SystemInfo::Atomic_Increase(&m_nDisable);
}

void MRTC_CallGraph_ThreadLocalData::Enable()
{
	int nDisable = MRTC_SystemInfo::Atomic_Decrease(&m_nDisable) - 1;
	M_ASSERT(nDisable >= 0, "!");
}


void MRTC_CallGraph::Start()
{
	m_bLogOnFinish = false;
	m_EndTime = 0;

	m_ThreadLocal->Start();
}

void MRTC_CallGraph::Stop()
{
	m_ThreadLocal->Stop();
}

void MRTC_CallGraph::Disable()
{
	m_ThreadLocal->Disable();
}

void MRTC_CallGraph::Enable()
{
	m_ThreadLocal->Enable();
}

void MRTC_CallGraph::LogTimes()
{
	TArray<CStr> CallGraphStrings;
	GetStrList(CallGraphStrings);

	for (int i = 0; i < CallGraphStrings.Len(); ++i)
	{
		LogFile(CallGraphStrings[i]);
	}
}


void MRTC_CallGraph::TraceTimes()
{
	TArray<CStr> CallGraphStrings;
	GetStrList(CallGraphStrings);

	for (int i = 0; i < CallGraphStrings.Len(); ++i)
	{
		M_TRACEALWAYS("%s\n", CallGraphStrings[i].Str());
		MRTC_SystemInfo::OS_Sleep(5);
	}
}



void MRTC_CallGraph_ThreadLocalData::Push(const char* _pFunction)
{
	if ((m_State & MRTC_CG_PENDINGSTART) && !m_Depth)
	{
		static MRTC_CallGraph* pCallGraph = MRTC_GetObjectManager()->m_pCallGraph;
		if (MGetCPUClock() < pCallGraph->m_EndTime)
		{
			Clear();
			m_pRoot->m_Clocks = -MGetCPUClock();
			m_State &= ~MRTC_CG_PENDINGSTART;
			m_State |= MRTC_CG_ISRUNNING;
			static MRTC_CallGraph* pCallGraph = MRTC_GetObjectManager()->m_pCallGraph;
			pCallGraph->RegisterRunning();
		}
		else
		{
			m_State = 0;
			return;
		}
	}

	if (!(m_State & MRTC_CG_ISRUNNING))
		return;

	MRTC_CallGraphEntry* pE = NULL;
	
	pE = m_pCurrent->m_Children.FindEqual(_pFunction);

	if (pE)
	{
		m_pCurrent = pE;
	}
	else
	{
		MRTC_CallGraphEntry *pE = m_EntryPool.New();//new (MDA_NEW_DEBUG_NOLEAK uint8[sizeof(MRTC_CallGraphEntry)]) MRTC_CallGraphEntry;
		pE->m_pThreadLocal = this;

		pE->m_Name = _pFunction;
		pE->m_pParent = m_pCurrent;
		m_pCurrent->m_Children.f_Insert(pE);
		m_pCurrent = pE;
	}
}

void MRTC_CallGraph::BlockUntilNoThreadsRunning()
{
	if ((m_ThreadLocal->m_State) & (MRTC_CG_ISRUNNING | MRTC_CG_PENDINGSTART))
		M_BREAKPOINT; // You cannot call this while the current thread is inside a callgraph

	while (Volatile(m_nRunning))
	{
		MRTC_SystemInfo::OS_Sleep(10);
	}
}

void MRTC_CallGraph::GetStrList(TArray<CStr> &_StrList)
{
	BlockUntilNoThreadsRunning();

	_StrList.Clear();

	CThreadDataIter Iter = m_ThreadLocalList;

	while (Iter)
	{
		if (Iter->m_pRoot)
			Iter->m_pRoot->Log_r(0, Iter->m_pRoot->m_Clocks - Iter->m_pRoot->m_ClocksWasted, Iter->m_pRoot->m_Clocks - Iter->m_pRoot->m_ClocksWasted, _StrList);

		++Iter;
	}
}


void MRTC_CallGraph::GetStrList2(TArray<CStr> &_StrList)
{
	BlockUntilNoThreadsRunning();

	_StrList.Clear();
	CThreadDataIter Iter = m_ThreadLocalList;

	while (Iter)
	{
		if (Iter->m_pRoot)
		{
			int nLinesNeeded = GetNumLinesNeeded_r(Iter->m_pRoot);
			_StrList.SetGrow(nLinesNeeded+1);
			Iter->m_pRoot->Log2_r(0, Iter->m_pRoot->m_Clocks - Iter->m_pRoot->m_ClocksWasted, Iter->m_pRoot->m_Clocks - Iter->m_pRoot->m_ClocksWasted, _StrList);
		}
		++Iter;
	}
}


void MRTC_CallGraph_ThreadLocalData::AddClocks(int64 _Clocks)
{
	m_pCurrent->m_Clocks += _Clocks;
	m_pCurrent->m_nCalls++;
}

void MRTC_CallGraph_ThreadLocalData::Pop(int64 _WastedClocks)
{
	
	m_pCurrent = m_pCurrent->m_pParent;
	if (!m_pCurrent)
		Error_static(M_FUNCTION, "Call graph stack underflow.");

	static MRTC_CallGraph* pCallGraph = MRTC_GetObjectManager()->m_pCallGraph;

	if (m_pCurrent == m_pRoot && pCallGraph->m_EndTime > 0)
	{
		if (MGetCPUClock() > pCallGraph->m_EndTime)
		{
			m_pRoot->m_Clocks += MGetCPUClock();
			m_State &= ~MRTC_CG_ISRUNNING;
			m_pRoot->AccumalateClockWaste_r();

			pCallGraph->UnRegisterRunning();

		}
	}
}

void MRTC_CallGraph::RegisterRunning()
{
	M_LOCK(m_Lock);

	++m_nRunning;
}

void MRTC_CallGraph::UnRegisterRunning()
{
	M_LOCK(m_Lock);

	--m_nRunning;
	if (m_nRunning == 0 && m_bLogOnFinish)
	{
		CThreadDataIter Iter = m_ThreadLocalList;

		while (Iter)
		{
			if (Iter->m_pRoot)
			{
				Iter->m_pRoot->Log_r(0, Iter->m_pRoot->m_Clocks - Iter->m_pRoot->m_ClocksWasted, Iter->m_pRoot->m_Clocks - Iter->m_pRoot->m_ClocksWasted);
			}

			++Iter;
		}
	}
}


void MRTC_CallGraph::Clear()
{

}

#endif
