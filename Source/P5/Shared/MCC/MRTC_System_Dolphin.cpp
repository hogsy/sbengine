
#ifdef	PLATFORM_DOLPHIN

static void Dolphin_InitHeap();
#define MRTC_OSALLOC_TOTALSIZE (100*1024)
static char* s_pOSAlloc_Heap = NULL;
static uint32 s_nOSAlooc_Pos = 0;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| System stuff
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_SystemInfo::MRTC_SystemInfo()
{
	m_nCPU = 1;
	m_CPUFrequency = 1;
	m_CPUFrequencyd = 1;
	m_CPUFrequencyRecp = 1;
	m_CPUFeatures = 0;
	m_CPUFeaturesEnabled = 0xffffffff;
	m_CPUName[0] = 0;
	m_CPUNameWithFeatures[0] = 0;

	CPU_Detect();
}

void MRTC_SystemInfo::CPU_Detect()
{
	CPU_MeasureFrequency();
	CPU_CreateNames();
}

fp8 MRTC_SystemInfo::CPU_MeasureFrequencyOnce()
{
	return OS_CORE_CLOCK;
}

void MRTC_SystemInfo::CPU_MeasureFrequency()
{
	m_CPUFrequency = OS_CORE_CLOCK;
	m_CPUFrequencyd = OS_CORE_CLOCK;

	m_CPUFrequencyRecp = 1.0 / m_CPUFrequencyd;
}


void MRTC_SystemInfo::CPU_CreateNames()
{
	CFStr CPUName("IBM PowerPC 750");
	CFStr CPUNameFeatures = CPUName;

	CPUNameFeatures += CFStrF(", %.0f Mhz", m_CPUFrequency / 1000000.0);

	strcpy(m_CPUName, CPUName.Str());
	strcpy(m_CPUNameWithFeatures, CPUNameFeatures.Str());
}

static bool s_bFirst = true;
static OSTime s_nTime0;
static OSTime s_nClockMark;
void MRTC_SystemInfo_CPU_SetClockMark();
void MRTC_SystemInfo_CPU_ResetClockToMark();
void MRTC_SystemInfo_CPU_SetClockMark()     { s_nClockMark = OSGetTime(); }
void MRTC_SystemInfo_CPU_ResetClockToMark() { s_nTime0 += (OSGetTime() - s_nClockMark); }

int64 MRTC_SystemInfo::CPU_GetClock()
{
	if (s_bFirst) 
	{ // there's code that doesn't like things like negative time.. 
	  // so let's start clock at 0..
		s_bFirst = false;
		s_nTime0 = OSGetTime();
	}
	return (OSGetTime() - s_nTime0) * (OS_CORE_CLOCK / OS_TIMER_CLOCK);
}

fp8 MRTC_SystemInfo::CPU_GetTime()
{
#ifdef CONSTANT_FRAME_RATE
	extern fp8 g_SimulatedTime;
	return g_SimulatedTime;
#else
	return fp8(CPU_GetClock()) * MRTC_GetObjectManager()->m_SystemInfo.m_CPUFrequencyRecp;
#endif
}

void* MRTC_SystemInfo::OS_Alloc(uint32 _Size, bool _bCommit)
{
#ifdef M_RTM
	static int total = 0;
	total += _Size;
	OSReport("MRTC_SystemInfo::OS_Alloc, _Size=%d, _bCommit=%d, total=%d\n", _Size, _bCommit, total);

	Dolphin_InitHeap();
	s_nOSAlooc_Pos += _Size;
	if (s_nOSAlooc_Pos > MRTC_OSALLOC_TOTALSIZE)
	{
		OSReport("MRTC_SystemInfo::OS_Alloc, failed allocating %d bytes. Reserved heap size=%d\n", _Size, MRTC_OSALLOC_TOTALSIZE);
		OSHalt("OS_Alloc heap too small!");
	}
	return (s_pOSAlloc_Heap - s_nOSAlooc_Pos);
#else
	void *pMem = OS_HeapAlloc(_Size);
	if (pMem == NULL)
	{
		OSReport("MRTC_SystemInfo::OS_Alloc, failed allocating %d bytes.\n", _Size);
		OSHalt("Out of memory.");
	}
	return pMem;
#endif
}

bool MRTC_SystemInfo::OS_Commit(void *_pMem, uint32 _Size, bool _bCommited)
{
	return false;
}

uint32 MRTC_SystemInfo::OS_CommitGranularity()
{
	return false;
}


void MRTC_SystemInfo::OS_Free(void* _pMem)
{
	OS_HeapFree( _pMem );
}

void* MRTC_SystemInfo::OS_GetProcessID()
{
	return 0;
}

void* MRTC_SystemInfo::OS_GetThreadID()
{
	return ::OSGetCurrentThread();
}


//void MRTC_SystemInfo::OS_Sleep(int _Milliseconds)
//{
//	int64 until;
//
//	until = GetCPUClock() + _Milliseconds * GetCPUFrequency() / 1000;
//
//	while( until > GetCPUClock() )
//	{
//		::OSYieldThread();
//	}
//}

// -------------------------------------------------------------------

class MRTC_DolphinThreadInfo : public OSThread
{
public:
	void* m_pStack;
	int m_StackSize;
	volatile int m_ExitCode;

	MRTC_DolphinThreadInfo()
	{
		m_pStack = NULL;
		m_StackSize = 0;
		m_ExitCode = 0;
	}

	~MRTC_DolphinThreadInfo()
	{
		if (m_pStack)
			delete[] m_pStack;
	}
};

typedef void*(*PFNDOLPHINTHREAD)(void*);


void* MRTC_SystemInfo::OS_ThreadCreate(uint32(M_STDCALL*_pfnEntryPoint)(void*), int _StackSize, void* _pContext, int _ThreadPriority)
{
	MRTC_DolphinThreadInfo* pThread = DNew(MRTC_DolphinThreadInfo) MRTC_DolphinThreadInfo;
	if (!pThread) return NULL;

	pThread->m_pStack = DNew(uint8) uint8[_StackSize];
	pThread->m_StackSize = _StackSize;
	if (!pThread->m_pStack)
	{
		delete pThread;
		return NULL;
	}

	int Prio;
	if (_ThreadPriority < MRTC_THREAD_PRIO_LOW)
		Prio = 31;
	else if (_ThreadPriority < MRTC_THREAD_PRIO_BELOWNORMAL)
		Prio = 30;
	else if (_ThreadPriority < MRTC_THREAD_PRIO_NORMAL)
		Prio = 24;
	else if (_ThreadPriority < MRTC_THREAD_PRIO_ABOVENORMAL)
		Prio = 16;
	else if (_ThreadPriority < MRTC_THREAD_PRIO_HIGHEST)
		Prio = 8;
	else if (_ThreadPriority < MRTC_THREAD_PRIO_TIMECRITICAL)
		Prio = 4;
	else
		Prio = 0;

	::OSCreateThread(
		pThread, 
		(PFNDOLPHINTHREAD) _pfnEntryPoint, 
		_pContext, 
		(uint8*)pThread->m_pStack + (_StackSize - 4), 
		_StackSize - 4, 
		Prio, 
		OS_THREAD_ATTR_DETACH
	);
/*
OSThread*  thread,
    void*    (*func)(void*),
    void*      param,
    void*      stackBase,
    u32        stackSize,
    OSPriority priority,
    u16        attribute)*/
    
    
	pThread->m_ExitCode = MRTC_THREAD_STILLACTIVE;
	::OSResumeThread(pThread);
	return pThread;
}
#if 0
class CThreadManager
{
public:	

	static CThreadManager *ms_pThis;
	CThreadManager()
	{
		ms_pThis = this;
		OSCreateAlarm(&m_Alarm);
		m_bAlarmSet = false;
	}
	~CThreadManager()
	{
		OSCancelAlarm(&m_Alarm);
	}

	class CQueuedThread
	{
	public:
		CDA_Link m_Link;
		OSTime m_WakeupTime;				
		OSThread *m_pThread;
	};

	DA_LLSI(CQueuedThread);

	TDA_Pool<CQueuedThread> m_Pool;

	OSAlarm m_Alarm;
	int m_bAlarmSet;

	CDA_LinkedList_Small m_SleepingThreads;

	static void Handler(OSAlarm* alarm, OSContext* context)
	{
		//////////////////////////////////////////////////
		// Disable sceduling and interupts
//		BOOL enabled;
//		enabled = OSDisableInterrupts();
//		OSDisableScheduler();
		//////////////////////////////////////////////////

		ms_pThis->Update();
		ms_pThis->UpdateAlarm();

		//////////////////////////////////////////////////
		// Enable scedueling and interupts
//		enabled = OSDisableInterrupts();
//		OSEnableScheduler();
//		OSYieldThread();
//		OSRestoreInterrupts(enabled);
		//////////////////////////////////////////////////
	}

	void UpdateAlarm()
	{
		if (m_bAlarmSet)
		{
			m_bAlarmSet = false;
			OSCancelAlarm(&m_Alarm);
		}

		// Loop through all sleeping threads and find the next time we need to wakeup
		SICQueuedThread Iter = m_SleepingThreads;

		OSTime Soonest = 0x7fffffffffffffff;
		
		while (Iter)
		{
			if (Iter->m_WakeupTime < Soonest)
			{
				Soonest = Iter->m_WakeupTime;
			}		
			
			++Iter;
		}

		/*if (Soonest < OSGetTime())
		{
			Update();
			
			Soonest = 0x7fffffffffffffff;
			
			while (Iter)
			{
				if (Iter->m_WakeupTime < Soonest)
				{
					Soonest = Iter->m_WakeupTime;
				}		
				
				++Iter;
			}
		}*/

		if (Soonest != 0x7fffffffffffffff)
		{
			OSSetAlarm(&m_Alarm, Soonest-OSGetTime(), Handler);
			m_bAlarmSet = true;
		}
	}

	void Update()
	{		
		// Loop through all sleeping threads and find the next time we need to wakeup
		SICQueuedThread Iter = m_SleepingThreads;

		OSTime Now = OSGetTime();
		
		while (Iter)
		{
			if (Iter->m_WakeupTime <= Now)
			{
				CQueuedThread *pThread = Iter;
				++Iter;
				OSResumeThread(pThread->m_pThread);
				m_Pool.Delete(pThread);
			}
			else
			{
				++Iter;
			}
		}
	}

	void SleepThread(float _SleepTime)
	{
		CQueuedThread *pQueuedThread = m_Pool.New();

		//////////////////////////////////////////////////
		// Disable sceduling and interupts
	/*	BOOL enabled;
		enabled = OSDisableInterrupts();
		OSDisableScheduler();
		OSRestoreInterrupts(enabled);*/
		//////////////////////////////////////////////////

		OSThread *pThread = OSGetCurrentThread();

		pQueuedThread->m_Link.Link(m_SleepingThreads, pQueuedThread);
		pQueuedThread->m_WakeupTime = OSGetTime() + OSMillisecondsToTicks(_SleepTime);

		UpdateAlarm();

		OSSuspendThread(pThread);
		//////////////////////////////////////////////////
		// Enable scedueling and interupts
/*		enabled = OSDisableInterrupts();
		OSEnableScheduler();
		OSYieldThread();
		OSRestoreInterrupts(enabled);*/
		//////////////////////////////////////////////////
	}	
};

CThreadManager *CThreadManager::ms_pThis=NULL;

class CThreadManagerWrapper
{
public:
	CThreadManager *m_pThreadManager;
	uint8 m_ThreadManger[sizeof(CThreadManager)];
	
	void Create()
	{
		if (!m_pThreadManager)
			m_pThreadManager = new(m_ThreadManger) CThreadManager;
	}

	CThreadManager * operator *()
	{
		Create();
		return m_pThreadManager;
	}

	CThreadManager * operator ->()
	{
		Create();
		return m_pThreadManager;
	}

};

CThreadManagerWrapper gs_ThreadManager = {0};

#endif

#define SENTINAL_VALUE  0xbeafdead
struct my_alarm
{
    OSAlarm alarm;
    OSThread *pThread;
    s32     Sentinal;
};

static void DelayCallback(OSAlarm* alarm,OSContext* Context)
{
    my_alarm *pAlarm;

    (void)Context;

    pAlarm = (my_alarm*)alarm;

    ASSERT(pAlarm->Sentinal == (s32)SENTINAL_VALUE);
    OSResumeThread(pAlarm->pThread);
}

//--------------------------------------------------------------------------
static void eng_DelayThread(s32 milliseconds)
{
//    ASSERT(g_ThreadVars);
    if (milliseconds <=0 )
        return;

    my_alarm myalarm;

    OSCreateAlarm(&myalarm.alarm);
    OSSetAlarmTag(&myalarm.alarm, 1);
    myalarm.Sentinal = SENTINAL_VALUE;
    myalarm.pThread  = OSGetCurrentThread();

	OSSetAlarm(&myalarm.alarm, OSMillisecondsToTicks(milliseconds), DelayCallback);
    OSSuspendThread(OSGetCurrentThread());
}



void MRTC_SystemInfo::OS_Sleep(int _Milliseconds)
{
	eng_DelayThread(_Milliseconds);
	//gs_ThreadManager->SleepThread(_Milliseconds);
/*	int64 until;

	until = GetCPUClock() + _Milliseconds * GetCPUFrequency() / 1000;

	while( until > GetCPUClock() )
	{
		::OSYieldThread();
	}*/
}


int MRTC_SystemInfo::OS_ThreadDestroy(void* _hThread)
{
	// Waits until thread terminates and returns exit code.

	MRTC_DolphinThreadInfo* pThread = (MRTC_DolphinThreadInfo*)_hThread;
	while(1)
	{
		MRTC_GOM()->m_pGlobalLock->Lock();
		if (pThread->m_ExitCode != MRTC_THREAD_STILLACTIVE)
		{
			//::OSCancelThread(pThread);  <-- AR: this shouldn't be needed, since OSExitThread has already been executed.
			MRTC_GOM()->m_pGlobalLock->Unlock();

			int ExitCode = pThread->m_ExitCode;
			delete pThread;
			return ExitCode;
		}
		MRTC_GOM()->m_pGlobalLock->Unlock();
		OS_Sleep(1);
	}
}

void MRTC_SystemInfo::OS_ThreadTerminate(void* _hThread, int _ExitCode)
{
	MRTC_GOM()->m_pGlobalLock->Lock();
	MRTC_DolphinThreadInfo* pThread = (MRTC_DolphinThreadInfo*)_hThread;
	pThread->m_ExitCode = _ExitCode;
	::OSCancelThread(pThread);	// All locks held by this thread will be released, so no corresponding GlobalUnlock is needed.
}

bool MRTC_SystemInfo::OS_ThreadIsRunning(void* _hThread)
{
	MRTC_DolphinThreadInfo* pThread = (MRTC_DolphinThreadInfo*)_hThread;
	return pThread->m_ExitCode == MRTC_THREAD_STILLACTIVE;
}

void MRTC_SystemInfo::OS_ThreadExit(int _ExitCode)
{
	MRTC_GOM()->m_pGlobalLock->Lock();
	MRTC_DolphinThreadInfo* pThread = (MRTC_DolphinThreadInfo*)::OSGetCurrentThread();
	pThread->m_ExitCode = _ExitCode;
	::OSExitThread((void*)_ExitCode);	// All locks held by this thread will be released, so no corresponding GlobalUnlock is needed.
}

int MRTC_SystemInfo::OS_ThreadGetExitCode(void* _hThread)
{
	MRTC_DolphinThreadInfo* pThread = (MRTC_DolphinThreadInfo*)_hThread;
	return pThread->m_ExitCode;
}

// -------------------------------------------------------------------

void* MRTC_SystemInfo::OS_MutexOpen(const char* _pName)
{
	static uint8 lCSMem[sizeof(MRTC_CriticalSection)];
	static MRTC_CriticalSection* pCSMem = NULL;

	if (!pCSMem)
	{
	
		pCSMem = new((void*)lCSMem)MRTC_CriticalSection;
	
	}

	return pCSMem;
}

void MRTC_SystemInfo::OS_MutexClose(void* _hLock)
{
}

void MRTC_SystemInfo::OS_MutexLock(void* _hLock)
{
	MRTC_CriticalSection* pCS = (MRTC_CriticalSection*) _hLock;
	pCS->Lock();
}

void MRTC_SystemInfo::OS_MutexUnlock(void* _hLock)
{
	MRTC_CriticalSection* pCS = (MRTC_CriticalSection*) _hLock;
	pCS->Unlock();
}

void MRTC_SystemInfo::OS_Assert(const char* _pMsg)
{
	OS_Trace("MRTC Assert: %s\n", _pMsg);

	*((int *)(NULL)) = 0;
}

void M_ARGLISTCALL MRTC_SystemInfo::OS_Trace(const char * _pStr, ...)
{
	char lBuffer[4096];

	if (_pStr)
	{
		va_list arg;
		va_start(arg, _pStr);
		CStrBase::vsnprintf((char*) &lBuffer[0], 4095, _pStr, arg); 
		lBuffer[4095] = 0;
	}
	else
		lBuffer[0] = 0;

	OSReport(lBuffer);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| File
|__________________________________________________________________________________________________
\*************************************************************************************************/

#ifndef M_RTM
#define MRTC_ENABLE_HOSTIO
#endif

//	SS:	Moved all this stuff into MFile_Dolphin.h & MFile_Dolphin.cpp.
#include	"MFile_Dolphin.h"

#include "dlmalloc.c"

static int s_nTotalHeapSize = -1;
static uint8 s_DLMutex[sizeof(MRTC_CriticalSection)];
static MRTC_CriticalSection * s_pDLMutex = NULL;


static void Dolphin_InitHeap()
{
	if (s_nTotalHeapSize == -1)
	{
		char* pArenaLo = (char*)OSGetArenaLo();
		char* pArenaHi = (char*)OSGetArenaHi();

		s_pDLMutex = new (s_DLMutex) MRTC_CriticalSection;

#if defined(MRTC_ENABLE_HOSTIO) || defined(MRTC_AUTOSTRIPLOGGER)
		// create heap for runtime library..
		const int nReserved = 256*1024;
		char* pHeapEnd   = pArenaLo + nReserved;
		void* pHeapStart = OSInitAlloc(pArenaLo, pHeapEnd, 1);
		OSHeapHandle hHeap = OSCreateHeap(pHeapStart, pHeapEnd);
		OSSetCurrentHeap(hHeap);
		pArenaLo += nReserved;
#endif

#if defined(USE_VIRTUAL_MEMORY)
		// give memory to VM-heap.. TODO: get size properly
		pArenaHi -= 0x200000;
#endif

#ifdef M_RTM
		// give some memory to OS_Alloc..
		s_pOSAlloc_Heap = pArenaHi;
		pArenaHi -= MRTC_OSALLOC_TOTALSIZE;
#endif

		// and use the rest for ourselves..
		s_nTotalHeapSize = pArenaHi - pArenaLo;
		OSReport("MRTC: Available memory %.2f MB\n", s_nTotalHeapSize / float(1<<20));

		dlmalloc_initstatic(pArenaLo, s_nTotalHeapSize);
		OSSetArenaLo(pArenaHi);
	}
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Heap
|__________________________________________________________________________________________________
\*************************************************************************************************/

void* MRTC_SystemInfo::OS_HeapAlloc(uint32 _Size)
{
	Dolphin_InitHeap();
	M_LOCK(*s_pDLMutex);
	return dlmalloc(_Size);
}

void* MRTC_SystemInfo::OS_HeapAllocAlign(uint32 _Size, uint32 _Align)
{
	Dolphin_InitHeap();
	M_LOCK(*s_pDLMutex);
	return dlmemalign(_Align, _Size);
}

void* MRTC_SystemInfo::OS_HeapRealloc(void *_pMem, uint32 _Size)
{	
	Dolphin_InitHeap();
	M_LOCK(*s_pDLMutex);
	return dlrealloc(_pMem, _Size);
}

void MRTC_SystemInfo::OS_HeapFree(void *_pMem)
{
	Dolphin_InitHeap();
	M_LOCK(*s_pDLMutex);
	dlfree(_pMem);
}

uint32 MRTC_SystemInfo::OS_HeapSize(const void *_pMem)
{
	Dolphin_InitHeap();
	M_LOCK(*s_pDLMutex);
	return dlmalloc_usable_size((void*)_pMem);
}

uint32 MRTC_SystemInfo::OS_PhysicalMemoryFree()
{
	Dolphin_InitHeap();
	M_LOCK(*s_pDLMutex);
	return dlmallinfo().fordblks + dlmalloc_heapfree();
}

uint32 MRTC_SystemInfo::OS_PhysicalMemorySize()
{
	Dolphin_InitHeap();
	M_LOCK(*s_pDLMutex);
	return s_nTotalHeapSize;
}

uint32 MRTC_SystemInfo::OS_PhysicalMemoryLargestFree()
{
	Dolphin_InitHeap();
	M_LOCK(*s_pDLMutex);
	return dlmallinfo().lrgblk;
}

uint32 MRTC_SystemInfo::OS_PhysicalMemoryUsed()
{
	Dolphin_InitHeap();
	M_LOCK(*s_pDLMutex);
	mallinfo mi = dlmallinfo();
	return mi.uordblks + mi.hblkhd;
}




#endif	// PLATFORM_DOLPHIN
