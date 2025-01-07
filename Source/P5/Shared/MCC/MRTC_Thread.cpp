
#ifdef PLATFORM_DOLPHIN
extern void EnableStackProtection(OSThread* _pThread);
#endif


MRTC_Thread_Store_Member::~MRTC_Thread_Store_Member()
{
	DLock(m_pThreadStore->m_Lock);
	m_LinkThreadStore.Unlink();
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_Thread_Core
|__________________________________________________________________________________________________
\*************************************************************************************************/
/*fp8 MRTC_Thread_Core::GetTime()
{
	return GetCPUClock() / GetCPUFrequency();
}*/

MRTC_Thread_Core::MRTC_Thread_Core()
{
	m_Thread_State = 0;
	m_Thread_pContext = NULL;
	m_hThread = NULL;
}

MRTC_Thread_Core::~MRTC_Thread_Core()
{
	Thread_Destroy();
}

bool MRTC_Thread_Core::Thread_IsTerminated()
{
	if (!m_hThread)
		return true;

//	return !MRTC_SystemInfo::OS_ThreadIsRunning(m_hThread);
	return (m_Thread_State & MRTC_THREAD_TERMINATED) != 0;
}

void MRTC_Thread_Core::Thread_RequestTermination()
{
	m_Thread_State |= MRTC_THREAD_TERMINATING;
	m_QuitEvent.Signal();
}

void MRTC_Thread_Core::Thread_OnCreate()
{
}

void MRTC_Thread_Core::Thread_OnDestroy()
{
}

MRTC_Thread_Store *MRTC_Thread_Core::Thread_GetStore()
{
	return (MRTC_Thread_Store *)MRTC_SystemInfo::Thread_LocalGetValue(MRTC_GetObjectManager()->m_ThreadStorageIndex);
}

void MRTC_Thread_Core::Thread_AllocStore()
{
	MRTC_Thread_Store *pStore = (MRTC_Thread_Store *)MRTC_SystemInfo::Thread_LocalGetValue(MRTC_GetObjectManager()->m_ThreadStorageIndex);
	if (!pStore)
	{
		MRTC_SystemInfo::Thread_LocalSetValue(MRTC_GetObjectManager()->m_ThreadStorageIndex, (mint)(DNew(MRTC_Thread_Store) MRTC_Thread_Store));
	}
}

void MRTC_Thread_Core::Thread_FreeStore()
{
	MRTC_Thread_Store *pStore = (MRTC_Thread_Store *)MRTC_SystemInfo::Thread_LocalGetValue(MRTC_GetObjectManager()->m_ThreadStorageIndex);
	if (pStore)
	{
		delete pStore;

		MRTC_SystemInfo::Thread_LocalSetValue(MRTC_GetObjectManager()->m_ThreadStorageIndex, NULL);
	}
}


uint32 MRTC_Thread_Core::Thread_Proc(void* _pContext)
{
	MRTC_SystemInfo::Thread_SetProcessor(5);

#ifdef PLATFORM_WIN_PC
//	if (MRTC_GetMemoryManager()->m_RunningDebugRuntime)
//		_controlfp(0, _EM_INVALID | _EM_ZERODIVIDE);
#endif

	MRTC_Thread_Core* pThread = (MRTC_Thread_Core*)_pContext;
	pThread->Thread_Exit(pThread->Thread_Main());
	return 0; // Never executes
}

void MRTC_Thread_Core::Thread_Create(void* _pContext, int _StackSize, int _Priority)
{
	M_LOCK(*(MRTC_GetObjectManager()->m_pGlobalLock));

	if (m_hThread) Error_static("MRTC_Thread_Win32::Thread_Create", "Already created.");

	m_Thread_pContext = _pContext;
	Thread_OnCreate();

	m_Thread_State |= MRTC_THREAD_CREATED;

//	M_TRACEALWAYS("(MRTC_Thread_Win32::Thread_Create) Creating thread.\n");


	m_hThread = MRTC_SystemInfo::OS_ThreadCreate(Thread_Proc, _StackSize + DRDMaxScopeMaxStackSize + DRDMaxCategoryMaxStackSize + DRDMaxSendStackSize, this, _Priority);
	if (!m_hThread)
	{
		Thread_OnDestroy();
		m_Thread_State &= ~MRTC_THREAD_CREATED;
		Error_static("MRTC_Thread_Win32::Thread_Create", "Failed to create thread.");
	}

#ifdef PLATFORM_DOLPHIN
	EnableStackProtection((OSThread*)m_hThread);
#endif

//	M_TRACEALWAYS("(MRTC_Thread_Win32::Thread_Create) Done.\n");
}

void MRTC_Thread_Core::Thread_Exit(int _ExitCode)
{
	// Free the store
	Thread_FreeStore();

	m_Thread_State |= MRTC_THREAD_TERMINATED;
	MRTC_SystemInfo::OS_ThreadExit(_ExitCode);
}

void MRTC_Thread_Core::Thread_Destroy()
{
	M_LOCK(*(MRTC_GetObjectManager()->m_pGlobalLock));
	M_TRY
	{
		if (m_hThread)
		{
//			M_TRACEALWAYS("(MRTC_Thread_Core::Thread_Destroy) Begin\n");

			Thread_RequestTermination();

			// The thread has 20s to exit before we nuke it.
			CMTime RefTime = CMTime::GetCPU();
			CMTime LastLogTime = RefTime;
			int nLog = 0;
			CMTime Time;

			while(((Time = CMTime::GetCPU()) - RefTime).Compare(20.0f) < 0)
			{
				if (Thread_IsTerminated()) 
					break;

				{
					// Must unlock to allow other thread to return
					M_UNLOCK(*(MRTC_GetObjectManager()->m_pGlobalLock));
					MRTC_SystemInfo::OS_Sleep(5);	// Wait for 5ms
				}

				if ((Time - LastLogTime).Compare(1.0f) > 0)
				{
					LastLogTime = Time;
//					M_TRACEALWAYS(CStrF("(MRTC_Thread_Win32::Thread_Destroy) Waiting for thread %.8x to terminate. (%d)\n", m_hThread, nLog));
					nLog++;
				}
			}

			if (!Thread_IsTerminated())
			{
				// Ok, here we have no choice except going for the big nuke.
				LogFile("-------------------------------------------------------------------");
				LogFile("(MRTC_Thread_Core::Thread_Destroy) CRITICAL ERROR: TERMINATING THREAD!");
				LogFile("-------------------------------------------------------------------");
				MRTC_SystemInfo::OS_ThreadTerminate(m_hThread, 0);
			}
//			M_TRACEALWAYS("(MRTC_Thread_Win32::Thread_Destroy) CloseHandle\n");
			MRTC_SystemInfo::OS_ThreadDestroy(m_hThread);

//			M_TRACEALWAYS("(MRTC_Thread_Win32::Thread_Destroy) Thread_OnDestroy\n");
			Thread_OnDestroy();

//			M_TRACEALWAYS("(MRTC_Thread_Win32::Thread_Destroy) Done\n");
		}
		m_hThread = 0;
		m_Thread_State = 0;
		m_Thread_pContext = NULL;
	}
	M_CATCH(
	catch(CCException)
	{
	}
	)
}

void MRTC_Thread_Core::Thread_Sleep(fp4 _Time)
{
	MRTC_SystemInfo::OS_Sleep((mint)(_Time * 1000.0f));
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_CriticalSection
|__________________________________________________________________________________________________
\*************************************************************************************************/

#if 0
/*
IMPLEMENT_OPERATOR_NEW(MRTC_CriticalSection);
*/

MRTC_CriticalSection::MRTC_CriticalSection()
{
#ifdef PLATFORM_WIN
#if 0
	m_LockValue = -1;
	m_ThreadLock = 0;
	m_OwningThread = 0;
#else
	InitializeCriticalSection((CRITICAL_SECTION*)m_Mutex);
#endif
#elif defined PLATFORM_DOLPHIN
	OSInitMutex(&m_Mutex);
#elif defined PLATFORM_PS2
	struct SemaParam semaParam;
	semaParam.initCount	= 1;
	semaParam.maxCount = 1;
	semaParam.currentCount = 0;
	semaParam.attr = 0;
	semaParam.numWaitThreads = 0;
	semaParam.option = 0;
	m_LockCount = 0;
	m_ThreadID = -1;
	m_Sema = CreateSema( &semaParam );
#endif
}

MRTC_CriticalSection::MRTC_CriticalSection(const char *_Description, bool _ShowLocking)
{
#ifdef PLATFORM_WIN
#if 0
	m_LockValue = -1;
	m_ThreadLock = 0;
	m_OwningThread = 0;
#else
	InitializeCriticalSection((CRITICAL_SECTION*)m_Mutex);
#endif
#elif defined PLATFORM_DOLPHIN
	OSInitMutex(&m_Mutex);
#elif defined PLATFORM_PS2
	struct SemaParam semaParam;
	semaParam.initCount	= 1;
	semaParam.maxCount = 1;
	semaParam.currentCount = 0;
	semaParam.attr = 0;
	semaParam.numWaitThreads = 0;
	semaParam.option = 0;
	m_LockCount = 0;
	m_ThreadID = -1;
	m_Sema = CreateSema( &semaParam );
#endif
}

MRTC_CriticalSection::~MRTC_CriticalSection()
{
#ifdef PLATFORM_WIN
	DeleteCriticalSection((CRITICAL_SECTION*)m_Mutex);
#elif defined PLATFORM_DOLPHIN
#elif defined PLATFORM_PS2
	DeleteSema( m_Sema );
#else
#error "Implement this!"
#endif	
}

void MRTC_CriticalSection::Init(const char *_Description, bool _ShowLocking)
{
}

#define MRTC_SPINLOCKTIME 400

#ifdef PLATFORM_WIN
// -------------------------------------------------------------------

#if 0
int32 MRTC_CriticalSection::Win32_InterlockedIncrement(volatile int32* _pValue)
{
	return ::InterlockedIncrement((LONG *)_pValue);
}

int32 MRTC_CriticalSection::Win32_InterlockedDecrement(volatile int32* _pValue)
{
	return ::InterlockedDecrement((LONG *)_pValue);
}

void MRTC_CriticalSection::Win32_Sleep()
{
#if(_WIN32_WINNT >= 0x0400 || defined(PLATFORM_XBOX))
	::SwitchToThread();
#elif defined PLATFORM_WIN
	::Sleep(1);
#endif
}

void MRTC_CriticalSection::InternalLock()
{
	if ((int)&m_LockValue & 3)
		Error("Lock", "MRTC_CriticalSection has to be alignen on a 4 byte boundrary");

StartLock:
	LONG Result = Win32_InterlockedIncrement(&m_LockValue);
	if (Result == 0)
	{
		return;
	}
	else if (Result > 0)
	{
		// Undo last try to lock
		Win32_InterlockedDecrement((LONG *)&m_LockValue);

		if (MRTC_SystemInfo::MRTC_GetSystemInfo().m_nCPU > 1)
		{
			while (1)
			{
				int i = MRTC_SPINLOCKTIME;
				while (i)
				{
					if (m_LockValue < 0)
						goto StartLock;
					--i;
				}
				Win32_Sleep();
			}				
		}
		else
		{
			while (1)
			{
				if (m_LockValue < 0)
					goto StartLock;
				Win32_Sleep();
			}				
		}
	}
	else
	{
		M_ASSERT(0, "Error in MRTC_CriticalSection, lock count too small");
	}
}

void MRTC_CriticalSection::InternalUnlock()
{
	Win32_InterlockedDecrement(&m_LockValue);
}

void MRTC_CriticalSection::Lock()
{
//	MRTC_GetMemoryManager();
//	g_ObjectManagerContainer.Lock();
//	return;
	if ((int)&m_LockValue & 3)
		Error("Lock", "MRTC_CriticalSection has to be alignen on a 4 byte boundrary");

	StartLock:
	InternalLock();
	if (!m_ThreadLock)
	{
		++m_ThreadLock;
		m_OwningThread = GetCurrentThreadId();
		InternalUnlock();
		return;
	}
	else if (m_ThreadLock > 0)
	{
		if (m_OwningThread == GetCurrentThreadId())
		{
			++m_ThreadLock;
			InternalUnlock();
			return;
		}
		else
		{
			InternalUnlock();
			if (MRTC_SystemInfo::MRTC_GetSystemInfo().m_nCPU > 1)
			{
				while (1)
				{
					int i = MRTC_SPINLOCKTIME;
					while (i)
					{
						InternalLock();
						if (m_ThreadLock <= 0)
						{
							InternalUnlock();
							goto StartLock;
						}
						InternalUnlock();
						--i;
					}
					Win32_Sleep();
				}				
			}
			else
			{
				while (1)
				{
					InternalLock();
					if (m_LockValue <= 0)
					{
						InternalUnlock();
						goto StartLock;
					}
					InternalUnlock();

					Win32_Sleep();
				}				
			}
		}
	}
	else
	{
		M_ASSERT(0, "Error in MRTC_CriticalSection, lock count too small");
	}
}

void MRTC_CriticalSection::Unlock()
{
	InternalLock();
	M_ASSERT(m_OwningThread == GetCurrentThreadId(), CStrF("MRTC_CriticalSection::Unlock() The Thread does not own the object (%x != %x)", m_OwningThread, GetCurrentThreadId()));
	--m_ThreadLock;
	InternalUnlock();
}

bool MRTC_CriticalSection::TryLock()
{
	M_ASSERT(!(((int)(&m_LockValue)) &0x3), "MRTC_CriticalSection has to be aligned on a 4 byte boundrary");

	LONG Result = Win32_InterlockedIncrement((LONG *)&m_LockValue);

	if (Result == 0)
	{
		m_OwningThread = GetCurrentThreadId();
		return true;
	}
	else if (Result > 0)
	{
		if (m_OwningThread == GetCurrentThreadId())
			return true;
		else
		{
			// Undo last try to lock
			Win32_InterlockedDecrement((LONG *)&m_LockValue);

			return false;
		}
	}
	else
	{
		M_ASSERT(0, "Error in MRTC_CriticalSection, lock count too small");
		return false;
	}
}
#else

void MRTC_CriticalSection::Lock()
{
	EnterCriticalSection((CRITICAL_SECTION*)m_Mutex);
}

void MRTC_CriticalSection::Unlock()
{
	LeaveCriticalSection((CRITICAL_SECTION*)m_Mutex);
}

bool MRTC_CriticalSection::TryLock()
{
	M_ASSERT(0, "");
//	return TryEnterCriticalSection((CRITICAL_SECTION*)m_Mutex) != 0;
	return true;
}

#endif

#elif defined PLATFORM_DOLPHIN
// -------------------------------------------------------------------
void MRTC_CriticalSection::Lock()
{
	OSLockMutex(&m_Mutex);
}

void MRTC_CriticalSection::Unlock()
{
	OSUnlockMutex(&m_Mutex);
}

bool MRTC_CriticalSection::TryLock()
{
	return OSTryLockMutex(&m_Mutex);
}

#elif defined PLATFORM_PS2
// -------------------------------------------------------------------
void MRTC_CriticalSection::Lock()
{
	if( m_ThreadID == ::GetThreadId() )
	{
		m_LockCount++;
		return;
	}

	WaitSema( m_Sema );
	m_ThreadID = ::GetThreadId();
	m_LockCount++;
}

void MRTC_CriticalSection::Unlock()
{
	M_ASSERT( m_ThreadID == ::GetThreadId(), "Thread not owning criticalsection tried to unlock it" );

	if( m_LockCount == 1 )
	{
		m_ThreadID = -1;
		SignalSema( m_Sema );
	}
	m_LockCount--;
}

bool MRTC_CriticalSection::TryLock()
{
	Error_static( "MRTC_CriticalSection::TryLock", "Implement MRTC_CriticalSection::TryLock" );
}

#else
// -------------------------------------------------------------------
	#error "Implement this"
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_Event
|__________________________________________________________________________________________________
\*************************************************************************************************/
#ifdef PLATFORM_WIN


MRTC_Event::MRTC_Event()
{
	m_hEvent = CreateEvent(NULL, false, false, NULL);
	M_ASSERT(m_hEvent, "Create failed");
}

MRTC_Event::~MRTC_Event()
{
	CloseHandle(m_hEvent);
}

bool MRTC_Event::Wait(int _TimeOut)
{
	if (_TimeOut < 0)
		_TimeOut = 0;
	if (_TimeOut < 1)
		_TimeOut = 1;

	int Ret = WaitForSingleObjectEx(m_hEvent, _TimeOut & 0x0fffffff, (_TimeOut & 0x70000000) != 0);
	if (Ret == WAIT_OBJECT_0)
		return true;
	else
		return false;
}

void MRTC_Event::Signal()
{
	SetEvent(m_hEvent);
}


//AR-CHANGE
#elif defined PLATFORM_DOLPHIN

MRTC_Event::MRTC_Event()
{
	OSInitSemaphore(&m_Sema, 0); // start nonsignaled
}

MRTC_Event::~MRTC_Event()
{
}

bool MRTC_Event::Wait(int _TimeOut)
{
	if (_TimeOut < 1)
		_TimeOut = 1;

	int64 StopVal = GetCPUClock() + (_TimeOut * GetCPUFrequency() / 1000.0);
	while (OSTryWaitSemaphore(&m_Sema) <= 0)
	{
		if (GetCPUClock() >= StopVal)
			return false;

		MRTC_SystemInfo::OS_Sleep(5);
	}
	return true;
}

void MRTC_Event::Signal()
{
	OSSignalSemaphore(&m_Sema);
}

#elif defined PLATFORM_PS2

MRTC_Event::MRTC_Event()
{
	struct SemaParam semaParam;
	semaParam.initCount	= 0;
	semaParam.maxCount = 1;
	semaParam.currentCount = 0;
	semaParam.attr = 0;
	semaParam.numWaitThreads = 0;
	semaParam.option = 0;
	m_Sema = CreateSema(&semaParam);

	if( m_Sema < 0 ) Error( "MRTC_Event::MRTC_Event", "Failed to allocate Semaphore" );
}

MRTC_Event::~MRTC_Event()
{
	DeleteSema(m_Sema);
}

//JK-TODO: Make sure this one works
bool MRTC_Event::Wait( int _TimeOut )
{
	int64 StartVal = MRTC_SystemInfo::CPU_GetClock();

	while( PollSema( m_Sema ) != m_Sema )
	{
		if( ( ( 1000 * ( MRTC_SystemInfo::CPU_GetClock() - StartVal ) ) * MRTC_SystemInfo::MRTC_GetSystemInfo().GetCPUFrequencyRecp() ) >= _TimeOut )
		{
			return false;
		}
		// Reschedule all threads
		RotateThreadReadyQueue(3);
	}

	return true;
}

void MRTC_Event::Signal()
{
	SignalSema( m_Sema );
}

#else

#error "Implement this"

#endif
#endif

namespace NThread
{

	MRTC_ThreadContext *GetThreadContext()
	{
		if (!MRTC_SystemInfo::MRTC_GetSystemInfo().m_pThreadContext)
		{
			static uint64 Data[(sizeof(MRTC_ThreadContext) + sizeof(uint64) - 1)/sizeof(uint64)];
			MRTC_SystemInfo::MRTC_GetSystemInfo().m_pThreadContext = new(Data) MRTC_ThreadContext;
		}

		return MRTC_SystemInfo::MRTC_GetSystemInfo().m_pThreadContext;
	}

    /***************************************************************************************************\
    |¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|
    | Reportable																						|
    |___________________________________________________________________________________________________|
    \***************************************************************************************************/

	void CEventAutoResetReportableAggregate::ReportTo(CEventAutoResetReportableAggregate *_pReportTo)
	{
		CReportListMember *pReportMember;
		{
			DIdsLockTyped(CMutual, GetThreadContext()->m_EventMember_Lock);
			pReportMember = GetThreadContext()->m_EventMember_Pool.New();
		}			

		// Lock 
		{
			DIdsLockTyped(CMutual, m_Lock);
			{
				DIdsLockTyped(CMutual, _pReportTo->m_Lock);				

				pReportMember->m_pReportTo = _pReportTo;
				pReportMember->m_pReportFrom = this;
				m_ReportTo.Insert(pReportMember);
				_pReportTo->m_ReportFrom.Insert(pReportMember);				
			}
		}
	}

	void CEventAutoResetReportableAggregate::ClearReportTo()
	{

		// Lock 
		{
			DIdsLockTyped(CMutual, m_Lock);
			{
				DIdsListLinkD_List(CReportListMember, m_LinkReportTo)::CIterator Iter(m_ReportTo);

				while (Iter)
				{

					CReportListMember *pList = Iter;
					++Iter;
					///CEventAutoResetReportableAggregate *pFrom = pList->m_pReportFrom; == this
					CEventAutoResetReportableAggregate *pTo = pList->m_pReportTo;
					{
						DIdsLockTyped(CMutual, pTo->m_Lock);					
						{
							DIdsLockTyped(CMutual, GetThreadContext()->m_EventMember_Lock);
							GetThreadContext()->m_EventMember_Pool.Delete(pList);
						}			
					
					}
				}
			}
		}
	}

	void CEventAutoResetReportableAggregate::ClearReportFrom()
	{

		// Lock 
		{
			DIdsLockTyped(CMutual, m_Lock);
			{
				DIdsListLinkD_List(CReportListMember, m_LinkReportFrom)::CIterator Iter(m_ReportFrom);

				while (Iter)
				{

					CReportListMember *pList = Iter;
					++Iter;
					///CEventAutoResetReportableAggregate *pFrom = pList->m_pReportFrom; == this
					CEventAutoResetReportableAggregate *pFrom = pList->m_pReportFrom;
					{
						DIdsLockTyped(CMutual, pFrom->m_Lock);					
						{
							DIdsLockTyped(CMutual, GetThreadContext()->m_EventMember_Lock);
							GetThreadContext()->m_EventMember_Pool.Delete(pList);
						}			
					
					}
				}
			}
		}
	}
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_MutualWriteManyRead
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_MutualWriteManyRead::MRTC_MutualWriteManyRead()
{
//	m_Locked.Init("MRTC_MutualWriteManyRead::m_Locked", false);
	m_LockCount = 0;
	m_WantAccessLock = 0;
	m_bInvalid = false;
	m_bLockedAcces = false;
	m_OwningThread = 0;
	m_RecurseLock = 0;
}

MRTC_MutualWriteManyRead::~MRTC_MutualWriteManyRead()
{	
	Make_Invalid();
	{
		M_LOCK(m_Locked);
	}
}

bool MRTC_MutualWriteManyRead::Lock()
{
	{
		M_LOCK(m_Locked);
		if (m_bInvalid)
		{
			return false;
		}

		if (m_OwningThread == (int)(mint)MRTC_SystemInfo::OS_GetThreadID())
		{
			++m_RecurseLock;
			return true;
		}

		while (m_WantAccessLock)
		{
			if (m_bInvalid)
			{
				return false;
			}

			{
				M_UNLOCK(m_Locked);

				MRTC_SystemInfo::OS_Sleep(1);
			}
		}	

		M_ASSERT (m_LockCount < 10, "Sanity check");

		++m_LockCount;
	}

	return true;	
}

bool MRTC_MutualWriteManyRead::MutualWanted()
{
	{
		M_LOCK(m_Locked);
		if (m_bInvalid || m_WantAccessLock)
		{
			return true;
		}
	}
	return false;
}

void MRTC_MutualWriteManyRead::ResetLockCount()
{
	{
		M_LOCK(m_Locked);

		m_LockCount = 0;
	}			
}

void MRTC_MutualWriteManyRead::Unlock()
{
	{
		M_LOCK(m_Locked);
		
		if (m_RecurseLock)
		{
			M_ASSERT (m_OwningThread == (int)(mint)MRTC_SystemInfo::OS_GetThreadID(), "Sanity check");
			M_ASSERT (m_RecurseLock > 0, "Sanity check");
			--m_RecurseLock;
			return;
		}

		M_ASSERT (m_LockCount > 0, "Unlocked more than locked");
		
		--m_LockCount;
	}

}
int MRTC_MutualWriteManyRead::TryMutualUse_Lock()
{
	{
		M_LOCK(m_Locked);
		
		if (m_bInvalid)
		{
			return 0;
		}

		if (m_LockCount)
		{
			return 2;
		}

		return MutualUse_Lock();
	}
}

//#define SpinCountNum 4096

bool MRTC_MutualWriteManyRead::MutualUse_Lock()
{
	{
		M_LOCK(m_Locked);
		
		if (m_bInvalid)
		{
			return false;
		}
		
	//	int SpinCount = SpinCountNum;
		++m_WantAccessLock;

		if (m_OwningThread == (int)(mint)MRTC_SystemInfo::OS_GetThreadID())
		{
			++m_RecurseLock;
			return true;
		}

		while (m_bLockedAcces || m_LockCount)
		{
			if (m_bInvalid)
			{
				--m_WantAccessLock;
				
				return false;
			}

			{
				M_UNLOCK(m_Locked);

				MRTC_SystemInfo::OS_Sleep(1);
			}
		}
		
		m_bLockedAcces = true;
		m_OwningThread = (int)(mint)MRTC_SystemInfo::OS_GetThreadID();
		++m_RecurseLock;

		/*	if (Event_Locked)
		{
		if (m_LockCount)
			{
				
				m_Locked.Unlock();
				WaitForSingleObject(Event_Locked,INFINITE);
			}
			else
				m_Locked.Unlock();
			
		}
		else
		{
			
			while (m_LockCount)
			{
				m_Locked.Unlock();
	#if(_WIN32_WINNT >= 0x0400)
				SwitchToThread();
	#else
				Sleep(0);
	#endif
				m_Locked.Lock();
			}
			
			m_Locked.Unlock();
			
		}*/
	}
		
	return true;
	
}

void MRTC_MutualWriteManyRead::MutualUse_Unlock()
{
	
	{
		M_LOCK(m_Locked);
		
		--m_WantAccessLock;

		M_ASSERT (m_OwningThread == (int)(mint)MRTC_SystemInfo::OS_GetThreadID(), "Sanity check");
		M_ASSERT (m_WantAccessLock >= 0, "Sanity check");
		if ((--m_RecurseLock) == 0)
		{
			m_bLockedAcces = false;
			m_OwningThread = 0;
		}
	}
	
}

bool MRTC_MutualWriteManyRead::UnlockAndMutualUse_Lock()
{
	
	{
		M_LOCK(m_Locked);

		if (m_bInvalid)
		{
			return false;
		}

		--m_LockCount;
		++m_WantAccessLock;

		while (m_bLockedAcces)
		{
			M_UNLOCK(m_Locked);
			MRTC_SystemInfo::OS_Sleep(1);
		}

		m_bLockedAcces = true;
		
	//	int SpinCount = SpinCountNum;
		
		while (m_LockCount)
		{
			M_UNLOCK(m_Locked);
			MRTC_SystemInfo::OS_Sleep(1);
		}
	}
	
	return true;
	
}

void MRTC_MutualWriteManyRead::MutualUse_UnlockAndLock()
{

	{
		M_LOCK(m_Locked);

		++m_LockCount;
		
		--m_WantAccessLock;
		
		M_ASSERT (m_WantAccessLock >= 0, "Sanity check");

		m_bLockedAcces = false;

		M_ASSERT(m_LockCount == 1, "4k");
	}

	
}

bool MRTC_MutualWriteManyRead::UnlockAndMake_Invalid()
{

	{
		M_LOCK(m_Locked);

		if (m_bInvalid)
		{
			return false;
		}

		m_bInvalid = true;

		Unlock();

		while (m_LockCount || m_WantAccessLock)
		{
			MRTC_SystemInfo::OS_Sleep(1);
		}
	}

	return true;

}


bool MRTC_MutualWriteManyRead::Make_Invalid()
{
	{
		M_LOCK(m_Locked);
		m_bInvalid = true;

		while (m_LockCount || m_WantAccessLock)
		{
			M_UNLOCK(m_Locked);
			MRTC_SystemInfo::OS_Sleep(1);
		}
	}

	return true;
}

bool MRTC_MutualWriteManyRead::Make_InvalidNoWait()
{
	{
		M_LOCK(m_Locked);

		m_bInvalid = true;
	}

	return true;
}

bool MRTC_MutualWriteManyRead::IsInvalid()
{
	bool IsInValid;
	{
		M_LOCK(m_Locked);
		IsInValid = m_bInvalid && !(m_LockCount || m_WantAccessLock);
	}
	return IsInValid;
}

void MRTC_MutualWriteManyRead::Make_Valid()
{

	{
		M_LOCK(m_Locked);

		m_bInvalid = false;
	}
}
