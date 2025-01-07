
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Part of MRTC.h

	Author:			Magnus Högdahl

	Copyright:		Starbreeze Studios AB 1996,2003

	Contents:

	Comments:

	History:	
		030711:		File header added

\*____________________________________________________________________________________________*/


#ifndef MACRO_INC_MRTC_Thread_h
#define MACRO_INC_MRTC_Thread_h

#include "VPUShared/MRTC_VPUShared_Thread.h"


//#define MRTC_LOCK_TIMING

namespace NThread
{

	class CEventAutoResetAggregate
	{
	public:
		void *m_pSemaphore;
		DIdsPInlineS void ConstructDontCreate()
		{
			m_pSemaphore = DNP;
		}

		DIdsPInlineS void ConstructIfNotCreated()
		{
			if (!m_pSemaphore)
				m_pSemaphore = MRTC_SystemInfo::Event_Alloc(false, true);
		}

		DIdsPInlineS bint IsCreated()
		{
			return m_pSemaphore != 0;
		}

		DIdsPInlineS bint IsCreated() volatile
		{
			return Volatile(m_pSemaphore) != 0;
		}

		DIdsPInlineS void Construct()
		{
			m_pSemaphore = MRTC_SystemInfo::Event_Alloc(false, true);
		}

		DIdsPInlineS void Destruct()
		{
			if (m_pSemaphore)
				MRTC_SystemInfo::Event_Free(m_pSemaphore);
		}

		DIdsPInlineS void SetSemaphore(void *_pSemaphore)
		{
			if (m_pSemaphore)
				MRTC_SystemInfo::Event_Free(m_pSemaphore);

			m_pSemaphore = _pSemaphore;
		}
		DIdsPInlineS void Signal()
		{
			MRTC_SystemInfo::Event_SetSignaled(m_pSemaphore);
		}

		DIdsPInlineS void Wait()
		{
			MRTC_SystemInfo::Event_Wait(m_pSemaphore);
		}

		// Returns true if the wait timed out
		DIdsPInlineS bint WaitTimeout(fp64 _Timeout)
		{
			return MRTC_SystemInfo::Event_WaitTimeout(m_pSemaphore, _Timeout);
		}

		DIdsPInlineS bint TryWait()
		{
			return MRTC_SystemInfo::Event_TryWait(m_pSemaphore);
		}
	};

	class CEventAutoReset : public CEventAutoResetAggregate
	{
	public:
		CEventAutoReset()
		{
			Construct();
		}
		~CEventAutoReset()
		{
			Destruct();
		}
	};


	class CEventAggregate
	{
	public:
		void *m_pEvent;
		DIdsPInlineS void ConstructDontCreate()
		{
			m_pEvent = DNP;
		}

		DIdsPInlineS void ConstructIfNotCreated(bint _bInitialSignal = false)
		{
			if (!m_pEvent)
				m_pEvent = MRTC_SystemInfo::Event_Alloc(_bInitialSignal, false);
		}

		DIdsPInlineS bint IsCreated()
		{
			return m_pEvent != 0;
		}

		DIdsPInlineS bint IsCreated() volatile
		{
			return Volatile(m_pEvent) != 0;
		}

		DIdsPInlineS void Construct(bint _bInitialSignal = false)
		{
			m_pEvent = MRTC_SystemInfo::Event_Alloc(_bInitialSignal, false);
		}

		DIdsPInlineS void Destruct()
		{
			if (m_pEvent)
				MRTC_SystemInfo::Event_Free(m_pEvent);
		}

		DIdsPInlineS void SetEvent(void *_pEvent)
		{
			if (m_pEvent)
				MRTC_SystemInfo::Event_Free(m_pEvent);

			m_pEvent = _pEvent;
		}

		DIdsPInlineS void SetSignaled()
		{
			MRTC_SystemInfo::Event_SetSignaled(m_pEvent);
		}

		DIdsPInlineS void ResetSignaled()
		{
			MRTC_SystemInfo::Event_ResetSignaled(m_pEvent);
		}

		DIdsPInlineS void Wait()
		{
			MRTC_SystemInfo::Event_Wait(m_pEvent);
		}

		DIdsPInlineS bint WaitTimeout(fp64 _Timeout)
		{
			return MRTC_SystemInfo::Event_WaitTimeout(m_pEvent, _Timeout);
		}

		DIdsPInlineS bint TryWait()
		{
			return MRTC_SystemInfo::Event_TryWait(m_pEvent);
		}
	};

	class CEvent : public CEventAggregate
	{
	public:
		CEvent()
		{
			Construct();
		}
		~CEvent()
		{
			Destruct();
		}
	};

	class CScopeLock;
	class CScopeUnlock;
	class CScopeLockRead;
	class CScopeUnlockRead;

	class CLock
	{
	public:
		DIdsPInlineS static void Lock()
		{
		}

		DIdsPInlineS static void Unlock()
		{
		}

		DIdsPInlineS static void Construct()
		{
		}

		DIdsPInlineS static void Destruct()
		{
		}
		class CScopeLocker
		{
		public:
			DIdsPInlineS CScopeLocker(CLock &_Lock)
			{
			}
		};
		class CScopeUnlocker
		{
		public:
			DIdsPInlineS CScopeUnlocker(CLock &_Lock)
			{
			}
		};
		class CScopeLockerRead
		{
		public:
			DIdsPInlineS CScopeLockerRead(CLock &_Lock)
			{
			}
		};
		class CScopeUnlockerRead
		{
		public:
			DIdsPInlineS CScopeUnlockerRead(CLock &_Lock)
			{
			}
		};
	};


	class CLockVirtual : public CLock
	{
	public:

		virtual void Construct(void * _pSemaphore)
		{

		}

		virtual void Construct()
		{

		}

		virtual void Destruct()
		{
		}

		virtual void Lock()
		{
		}

		virtual void Unlock()
		{
		}

		virtual void LockRead()
		{
		}

		virtual void UnlockRead()
		{
		}

		typedef CScopeLock CScopeLocker;
		typedef CScopeUnlock CScopeUnlocker;
		typedef CScopeLockRead CScopeLockerRead;
		typedef CScopeUnlockRead CScopeUnlockerRead;
	};





	class CAtomicSmintAggregate
	{
		smint volatile m_Int;
	public:

		void Construct()
		{
			m_Int = 0;
		}
		
		void Construct(smint _Init)
		{
			m_Int = _Init;
		}

		void Destruct()
		{
		}

		// Returns old value
		DIdsPInlineS smint Increase()
		{
			return MRTC_SystemInfo::Atomic_Increase(&m_Int);
		}

		// Returns old value
		DIdsPInlineS smint Decrease()
		{
			return MRTC_SystemInfo::Atomic_Decrease(&m_Int);
		}

		// Returns old value
		DIdsPInlineS smint Exchange(smint _Number)
		{
			return MRTC_SystemInfo::Atomic_Exchange(&m_Int, _Number);
		}

		// Returns old value
		DIdsPInlineS smint Add(smint _Number)
		{
			return MRTC_SystemInfo::Atomic_Add(&m_Int, _Number);
		}

		// Returns the value in the variable
		DIdsPInlineS smint IfEqualExchange(smint _CompareTo, smint _SetTo)
		{
			return MRTC_SystemInfo::Atomic_IfEqualExchange(&m_Int, _CompareTo, _SetTo);
		}

		DIdsPInlineS smint Get()
		{
			return m_Int;
		}

		DIdsPInlineS smint operator ++()
		{
			return Increase() + 1;
		}

		DIdsPInlineS smint operator --()
		{
			return Decrease() - 1;
		}

		DIdsPInlineS smint operator += (smint _ToAdd)
		{
			return Add(_ToAdd) + _ToAdd;
		}

		DIdsPInlineS smint operator -= (smint _ToSub)
		{
			return Add((0-_ToSub)) - _ToSub;
		}

	};

	class CAtomicSmint : public CAtomicSmintAggregate
	{
	public:
		CAtomicSmint()
		{
			Construct();
		}

		CAtomicSmint(smint _Init)
		{
			Construct(_Init);
		}
	};

	template <typename t_CEvent, typename t_CBase = CLock>
	class TCMutualAggregate : public t_CBase
	{
	public:
		CAtomicIntAggregate m_nLocked;
		CAtomicIntAggregate m_nCreated;
		volatile mint m_ThreadID;
		volatile aint m_nRecurse;
#ifdef MRTC_LOCK_TIMING
		volatile uint64 m_TimeSpentInLock;
		volatile uint64 m_LockCount;
#endif // MRTC_LOCK_TIMING
		t_CEvent m_Event;

		M_INLINE void ClearTime()
		{
#ifdef MRTC_LOCK_TIMING
			m_TimeSpentInLock = 0;
			m_LockCount = 0;
#endif // MRTC_LOCK_TIMING
		}

		void Construct()
		{
			m_nLocked.Construct(0);
			m_nCreated.Construct(0);
			m_nRecurse = 0;
			m_ThreadID = 0;
			m_Event.ConstructDontCreate();
			ClearTime();
		}

		void Construct(void * _pSemaphore)
		{
			m_nLocked.Construct(0);
			m_nCreated.Construct(0);
			m_nRecurse = 0;
			m_ThreadID = 0;
			m_Event.Construct(_pSemaphore);
			ClearTime();
		}			

		void Destruct()
		{
			m_Event.Destruct();
			m_nLocked.Destruct();
			m_nCreated.Destruct();
		}

		DIdsPNoInline void WaitForIt(int32 _nCreate)
		{
			if(_nCreate == 0)
			{
				m_Event.ConstructIfNotCreated();
				m_nCreated.Exchange(2);
			}
			else
			{
				while (!Volatile(m_Event).IsCreated())
				{					
					MRTC_SystemInfo::OS_Sleep(0);
				}
			}
			{
				M_NAMEDEVENT("LockWait", 0);
				m_Event.Wait();
			}
		}

		bint OwnsLock()
		{
			mint CurrentThread = (mint)MRTC_SystemInfo::OS_GetThreadID();

			if(m_ThreadID == CurrentThread)
			{
				return true;
			}
			return false;
		}

		bint TryLock()
		{
			mint CurrentThread = (mint)MRTC_SystemInfo::OS_GetThreadID();

			if(m_ThreadID == CurrentThread)
			{
				m_nRecurse++;
				return true;
			}

			// Try to take the lock
			if(m_nLocked.IfEqualExchange(0, 1) > 0)
			{
				// Failed lock
				return false;
			}

			m_ThreadID = CurrentThread;
			m_nRecurse = 1;
			M_IMPORTBARRIER;
			return true;
		}

		uint32 LockRecurse()
		{
			mint CurrentThread = (mint)MRTC_SystemInfo::OS_GetThreadID();

			if(m_ThreadID == CurrentThread)
			{
				m_nRecurse++;
				return m_nRecurse;
			}

			// Try to take the lock
			bint bLocked = 0;
			int nCount = MRTC_SystemInfo::OS_ThreadSpinCount();
			if (nCount)
				while(!(bLocked = TryLock()) && nCount--) {}

			if(!bLocked)
			{
				if(m_nLocked.Increase() > 0)
				{
					int32 nCreated = m_nCreated.IfEqualExchange(0, 1);
					if(nCreated < 2)
						WaitForIt(nCreated);
					else
						m_Event.Wait();
				}
				m_ThreadID = CurrentThread;
				m_nRecurse = 1;
				M_IMPORTBARRIER;
			}
			return 1;
		}

		void Lock()
		{
			mint CurrentThread = (mint)MRTC_SystemInfo::OS_GetThreadID();

			if(m_ThreadID == CurrentThread)
			{
				m_nRecurse++;
				return;
			}

#ifdef MRTC_LOCK_TIMING
			uint64 TimeStart = CMTimerFuncs_OS::Clock();
#endif	// MRTC_LOCK_TIMING

			// Try to take the lock
			bint bLocked = 0;
			int nCount = 400;
			while(!(bLocked = TryLock()) && nCount--) {}

			if(!bLocked)
			{
				if(m_nLocked.Increase() > 0)
				{
					int32 nCreated = m_nCreated.IfEqualExchange(0, 1);
					if(nCreated < 2)
						WaitForIt(nCreated);
					else
						m_Event.Wait();
				}
				m_ThreadID = CurrentThread;
				m_nRecurse = 1;
				M_IMPORTBARRIER;
			}
#ifdef MRTC_LOCK_TIMING
			m_LockCount++;
			m_TimeSpentInLock += CMTimerFuncs_OS::Clock() - TimeStart;
#endif // MRTC_LOCK_TIMING
		}

		DIdsPNoInline void SignalIt()
		{
			// Wait for it to be created, because it should be
			while (!Volatile(m_Event).IsCreated())
			{					
				MRTC_SystemInfo::OS_Sleep(0);
			}
			// Someone is waiting
			m_Event.Signal();
		}
		void Unlock()
		{
			M_ASSERT(m_ThreadID == (mint)MRTC_SystemInfo::OS_GetThreadID(), "Only locking thread is allowed to unlock");

			if ((--m_nRecurse) == 0)
			{
				m_ThreadID = 0;
				M_EXPORTBARRIER;
				if (m_nLocked.Decrease() > 1)
				{
					SignalIt();
				}
			}
		}

		uint32 UnlockRecurse()
		{
			M_ASSERT(m_ThreadID == (mint)MRTC_SystemInfo::OS_GetThreadID(), "Only locking thread is allowed to unlock");

			if ((--m_nRecurse) == 0)
			{
				m_ThreadID = 0;
				M_EXPORTBARRIER;
				if (m_nLocked.Decrease() > 1)
				{
					SignalIt();
				}

				return 0;
			}

			return m_nRecurse;
		}

		typedef TCScopeLock<TCMutualAggregate> CScopeLocker;
		typedef TCScopeUnlock<TCMutualAggregate> CScopeUnlocker;
		typedef TCScopeLockRead<TCMutualAggregate> CScopeLockerRead;
		typedef TCScopeUnlockRead<TCMutualAggregate> CScopeUnlockerRead;

	};

	template <typename t_CEvent, typename t_CBase>
	class TMutual : public TCMutualAggregate<t_CEvent, t_CBase>
	{
	public:
		TMutual()
		{
			TCMutualAggregate<t_CEvent, t_CBase>::Construct();
		}

		TMutual(void * _pSemaphore)
		{
			TCMutualAggregate<t_CEvent, t_CBase>::Construct(_pSemaphore);
		}
		
		~TMutual()
		{
			TCMutualAggregate<t_CEvent, t_CBase>::Destruct();
		}
	};

	typedef TCMutualAggregate<CEventAutoResetAggregate> CMutualAggregate;
	typedef TMutual<CEventAutoResetAggregate> CMutual;


	/***************************************************************************************************\
	|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯|
	| Reportable																						|
	|___________________________________________________________________________________________________|
	\***************************************************************************************************/

	class CEventAutoResetReportableAggregate
	{
	public:
		void *m_pSemaphore;

		class CReportListMember
		{
		public:
			CEventAutoResetReportableAggregate *m_pReportTo;
			CEventAutoResetReportableAggregate *m_pReportFrom;
			DIdsListLinkD_Link(CReportListMember, m_LinkReportFrom);
			DIdsListLinkD_Link(CReportListMember, m_LinkReportTo);
		};

		CMutual m_Lock;

		DIdsListLinkD_List(CReportListMember, m_LinkReportTo) m_ReportTo;
		DIdsListLinkD_List(CReportListMember, m_LinkReportFrom) m_ReportFrom;

		/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		|	Function:			Makes a link from the event to another event			|
		|																				|
		|	Parameters:																	|
		|		_pReportTo:		The parameter to report to								|
		|																				|
		|	Comments:			Make sure that you don't create circle reports. Other	|
		|						than hang when an event gets signaled it could cause	|
		|						a deadlock when calling the ReportTo function. It goes	|
		|						without saying that you have to make sure that one of	|
		|						the objects don't get deleted while in the function		|
		\*_____________________________________________________________________________*/

		void ReportTo(CEventAutoResetReportableAggregate *_pReportTo);
		void ClearReportTo();
		void ClearReportFrom();


		DIdsPInlineS void ConstructDontCreate()
		{
			m_pSemaphore = DNP;
		}

		DIdsPInlineS void ConstructIfNotCreated()
		{
			if (!m_pSemaphore)
				m_pSemaphore = MRTC_SystemInfo::Event_Alloc(false, true);
		}

		DIdsPInlineS bint IsCreated()
		{
			return m_pSemaphore != 0;
		}		


		DIdsPInlineS bint IsCreated() volatile
		{
			return Volatile(m_pSemaphore) != 0;
		}		


		DIdsPInlineS void Construct()
		{
			m_pSemaphore = MRTC_SystemInfo::Event_Alloc(false, true);
		}

		DIdsPInlineS void Destruct()
		{
			ClearReportTo();
			ClearReportFrom();

			if (m_pSemaphore)
				MRTC_SystemInfo::Event_Free(m_pSemaphore);
		}

		DIdsPInlineS void SetSemaphore(void *_pSemaphore)
		{
			if (m_pSemaphore)
				MRTC_SystemInfo::Event_Free(m_pSemaphore);

			m_pSemaphore = _pSemaphore;
		}
		void Signal()
		{
			MRTC_SystemInfo::Event_SetSignaled(m_pSemaphore);
			{
				DLockTyped(CMutual, m_Lock);
				DIdsListLinkD_List(CReportListMember, m_LinkReportTo)::CIterator Iter(m_ReportTo);

				while (Iter)
				{
					Iter->m_pReportTo->Signal();
					++Iter;
				}
			}
		}

		DIdsPInlineS void Wait()
		{
			MRTC_SystemInfo::Event_Wait(m_pSemaphore);

		}

		DIdsPInlineS bint WaitTimeout(fp64 _Timeout)
		{
			return MRTC_SystemInfo::Event_WaitTimeout(m_pSemaphore, _Timeout);
		}

		DIdsPInlineS bint TryWait()
		{
			return MRTC_SystemInfo::Event_TryWait(m_pSemaphore);
		}
	};

	class CEventAutoResetReportable : public CEventAutoResetReportableAggregate
	{
	public:
		CEventAutoResetReportable()
		{
			Construct();
		}
		~CEventAutoResetReportable()
		{
			Destruct();
		}
	};

	class CSpinLockReentrantAggregate : public CSpinLockAggregate
	{
	public:
		void* m_ThreadID;
		int m_nRecursion;

		void Constuct()
		{
			CSpinLockAggregate::Construct();
			m_ThreadID = 0;
			m_nRecursion = 0;
		}

		void Destruct()
		{
			CSpinLockAggregate::Destruct();
		}

		void Lock()
		{
			void* ThreadID = MRTC_SystemInfo::OS_GetThreadID();
			if(m_ThreadID == ThreadID)
			{
				m_nRecursion++;
				return;
			}
			CSpinLockAggregate::Internal_Lock();
			m_ThreadID = ThreadID;
			m_nRecursion = 1;
			M_IMPORTBARRIER;
		}

		void Unlock()
		{
			if((--m_nRecursion) == 0)
			{
				m_ThreadID = 0;
				M_EXPORTBARRIER;
				CSpinLockAggregate::Internal_Unlock();
			}
		}
	};

	class CSpinLockReentrant : public CSpinLockReentrantAggregate
	{
	public:
		CSpinLockReentrant()
		{
			CSpinLockReentrantAggregate::Construct();
		}

		~CSpinLockReentrant()
		{
			CSpinLockReentrantAggregate::Destruct();
		}
	};

}

#define M_LOCK(x) DLock(x)
#define M_LOCK2(x, y) NThread::CScopeLock ScopeLockMutual1_y##(_ToLock)

// Obs, dont use this to unlock an object... no need... only to unlock in an context
#define M_UNLOCK(x) DUnlock(x)
#define M_UNLOCK2(x, y) NThread::CScopeUnlock ScopeUnlockMutual1_y##(_ToUnlock)

class MRTC_MutualWriteManyRead
{
private:
	NThread::CMutual m_Locked;
	volatile int m_LockCount;
	volatile int m_WantAccessLock;
	volatile int m_bLockedAcces;
	volatile int m_bInvalid;
	volatile int m_OwningThread;
	volatile int m_RecurseLock;

public:

	// The caller must implement for a false return. If false is returned the object cannot be used.
	bool Lock();
	void Unlock();

	bool MutualWanted();

	void ResetLockCount();

	// The caller must implement for a false return. If false is returned the object cannot be used.
	int TryMutualUse_Lock();
	bool MutualUse_Lock();
	void MutualUse_Unlock();

	// The caller must implement for a false return. If false is returned the object cannot be used.
	bool UnlockAndMutualUse_Lock();
	void MutualUse_UnlockAndLock();

	// The caller must implement for a false return. If false is returned the object cannot be used.
	bool UnlockAndMake_Invalid();
	bool Make_Invalid();
	bool Make_InvalidNoWait();
	bool IsInvalid();
	void Make_Valid();

	MRTC_MutualWriteManyRead();
	~MRTC_MutualWriteManyRead();
};

#define M_MWMR_Lock_Unlock(ToLock, Success) MRTC_MutualWriteManyRead_LockHelp MWMR_Locker_Object(ToLock, Success, MRTC_MutualWriteManyRead_LockHelp::E_Lock, MRTC_MutualWriteManyRead_LockHelp::E_Unlock);
#define M_MWMR2_Lock_Unlock(ToLock, Success, Name) MRTC_MutualWriteManyRead_LockHelp MWMR_Locker_Object_Name##(ToLock, Success, MRTC_MutualWriteManyRead_LockHelp::E_Lock, MRTC_MutualWriteManyRead_LockHelp::E_Unlock);

#define M_MWMR_Unlock_Lock(ToLock, Success) MRTC_MutualWriteManyRead_LockHelp MWMR_Locker_Object(ToLock, Success, MRTC_MutualWriteManyRead_LockHelp::E_Unlock, MRTC_MutualWriteManyRead_LockHelp::E_Lock);
#define M_MWMR2_Unlock_Lock(ToLock, Success, Name) MRTC_MutualWriteManyRead_LockHelp MWMR_Locker_Object_Name##(ToLock, Success, MRTC_MutualWriteManyRead_LockHelp::E_Unlock, MRTC_MutualWriteManyRead_LockHelp::E_Lock);

#define M_MWMR_MutualLock_MutualUnlock(ToLock, Success) MRTC_MutualWriteManyRead_LockHelp MWMR_Locker_Object(ToLock, Success, MRTC_MutualWriteManyRead_LockHelp::E_MutualLock, MRTC_MutualWriteManyRead_LockHelp::E_MutualUnlock);
#define M_MWMR2_MutualLock_MutualUnlock(ToLock, Success, Name) MRTC_MutualWriteManyRead_LockHelp MWMR_Locker_Object_Name##(ToLock, Success, MRTC_MutualWriteManyRead_LockHelp::E_MutualLock, MRTC_MutualWriteManyRead_LockHelp::E_MutualUnlock);

#define M_MWMR_MutualUnlock_MutualLock(ToLock, Success) MRTC_MutualWriteManyRead_LockHelp MWMR_Locker_Object(ToLock, Success, MRTC_MutualWriteManyRead_LockHelp::E_MutualUnlock, MRTC_MutualWriteManyRead_LockHelp::E_MutualLock);
#define M_MWMR2_MutualUnlock_MutualLock(ToLock, Success, Name) MRTC_MutualWriteManyRead_LockHelp MWMR_Locker_Object_Name##(ToLock, Success, MRTC_MutualWriteManyRead_LockHelp::E_MutualUnlock, MRTC_MutualWriteManyRead_LockHelp::E_MutualLock);

#define M_MWMR_UnlockAndMutualLock_MutualUnlockAndLock(ToLock, Success) MRTC_MutualWriteManyRead_LockHelp MWMR_Locker_Object(ToLock, Success, MRTC_MutualWriteManyRead_LockHelp::E_Unlock_And_MutualLock, MRTC_MutualWriteManyRead_LockHelp::E_MutualUnlock_And_Lock);
#define M_MWMR2_UnlockAndMutualLock_MutualUnlockAndLock(ToLock, Success, Name) MRTC_MutualWriteManyRead_LockHelp MWMR_Locker_Object_Name##(ToLock, Success, MRTC_MutualWriteManyRead_LockHelp::E_Unlock_And_MutualLock, MRTC_MutualWriteManyRead_LockHelp::E_MutualUnlock_And_Lock);

#define M_MWMR_MutualUnlockAndLock_UnlockAndMutualLock(ToLock, Success) MRTC_MutualWriteManyRead_LockHelp MWMR_Locker_Object(ToLock, Success, MRTC_MutualWriteManyRead_LockHelp::E_MutualUnlock_And_Lock, MRTC_MutualWriteManyRead_LockHelp::E_Unlock_And_MutualLock);
#define M_MWMR2_MutualUnlockAndLock_UnlockAndMutualLock(ToLock, Success, Name) MRTC_MutualWriteManyRead_LockHelp MWMR_Locker_Object_Name##(ToLock, Success, MRTC_MutualWriteManyRead_LockHelp::E_MutualUnlock_And_Lock, MRTC_MutualWriteManyRead_LockHelp::E_Unlock_And_MutualLock);

class MRTC_MutualWriteManyRead_LockHelp
{
private:
	MRTC_MutualWriteManyRead *m_ToLock;
	bool *m_pSuccess;
	int16 m_ConstructAction;
	int16 m_DestructAction;

public:

	enum EActions
	{
		E_Lock,
		E_Unlock,
		E_MutualLock,
		E_MutualUnlock,
		E_MutualUnlock_And_Lock,
		E_Unlock_And_MutualLock,
		E_Unlock_And_MakeInvalid
	};

	MRTC_MutualWriteManyRead_LockHelp(MRTC_MutualWriteManyRead &_ToLock, bool &_bSuccess, EActions _ConstructAction, EActions _DestructAction)
	{
		m_ToLock = &_ToLock;
		m_ConstructAction = _ConstructAction;
		m_DestructAction = _DestructAction;
		m_pSuccess = &_bSuccess;

		switch (m_ConstructAction)
		{
		case E_Lock:
			*m_pSuccess = m_ToLock->Lock();
			break;
		case E_Unlock:
			*m_pSuccess = true;
			 m_ToLock->Unlock();
			break;
		case E_MutualLock:
			*m_pSuccess = m_ToLock->MutualUse_Lock();
			break;
		case E_MutualUnlock:
			*m_pSuccess = true;
			m_ToLock->MutualUse_Unlock();
			break;
		case E_MutualUnlock_And_Lock:
			*m_pSuccess = true;
			m_ToLock->MutualUse_UnlockAndLock();
			break;
		case E_Unlock_And_MutualLock:
			*m_pSuccess = m_ToLock->UnlockAndMutualUse_Lock();
			break;
		case E_Unlock_And_MakeInvalid:
			*m_pSuccess = m_ToLock->UnlockAndMake_Invalid();
			break;
		}
	}
	
	void MakeInvalid()
	{
		*m_pSuccess = false;
	}
	
	~MRTC_MutualWriteManyRead_LockHelp()
	{
		if (*m_pSuccess)
		{
			switch (m_DestructAction)
			{
			case E_Lock:
				*m_pSuccess = m_ToLock->Lock();
				break;
			case E_Unlock:
				*m_pSuccess = true;
				m_ToLock->Unlock();
				break;
			case E_MutualLock:
				*m_pSuccess = m_ToLock->MutualUse_Lock();
				break;
			case E_MutualUnlock:
				*m_pSuccess = true;
				m_ToLock->MutualUse_Unlock();
				break;
			case E_MutualUnlock_And_Lock:
				*m_pSuccess = true;
				m_ToLock->MutualUse_UnlockAndLock();
				break;
			case E_Unlock_And_MutualLock:
				*m_pSuccess = m_ToLock->UnlockAndMutualUse_Lock();
				break;
			case E_Unlock_And_MakeInvalid:
				*m_pSuccess = m_ToLock->UnlockAndMake_Invalid();
				break;
			}
		}
	}
};

typedef NThread::CSpinLockReentrant MRTC_SpinLock;
typedef NThread::CMutual MRTC_CriticalSection; 
typedef NThread::CEventAutoReset MRTC_Event;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Thread stuff
|__________________________________________________________________________________________________
\*************************************************************************************************/

// -------------------------------------------------------------------
//  MRTC_Thread_Core
// -------------------------------------------------------------------
enum
{
	MRTC_THREAD_CREATED			= 1,
	MRTC_THREAD_TERMINATING		= 2,
	MRTC_THREAD_TERMINATED		= 4,
};

class MRTC_Thread_Store;
class MRTC_Thread_Store_Member
{
public:
	MRTC_Thread_Store *m_pThreadStore;
	MRTC_Thread_Store_Member(MRTC_Thread_Store *_pThreadStore)
	{
		m_pThreadStore = _pThreadStore;
	}

	virtual ~MRTC_Thread_Store_Member();

	DLinkDS_Link(MRTC_Thread_Store_Member, m_LinkThreadStore);
};

class MRTC_Thread_Store
{
public:
	NThread::CMutual m_Lock;

	DLinkDS_List(MRTC_Thread_Store_Member, m_LinkThreadStore) m_Members;

	~MRTC_Thread_Store()
	{
		DLock(m_Lock);

		m_Members.DeleteAll();
	}
};



class MRTC_Thread_Core
// : public CReferenceCount
{
protected:
	int m_Thread_State;
	void* m_Thread_pContext;
	void* m_hThread;

public:
	NThread::CEventAutoResetReportable m_QuitEvent;

//	fp64 GetTime();

	virtual const char* Thread_GetName() const {return "Unknown";}

public:
	MRTC_Thread_Core();
	~MRTC_Thread_Core();

	static MRTC_Thread_Store *Thread_GetStore();
	static void Thread_AllocStore();
	static void Thread_FreeStore();

	void *Thread_GetHandle()
	{
		return m_hThread;
	}

	// Can be called by both thread and creating thread.
	virtual int Thread_GetState() { return m_Thread_State; };
	virtual void* Thread_GetContext() { return m_Thread_pContext; };
	virtual bool Thread_IsCreated() { return (m_Thread_State & MRTC_THREAD_CREATED) != 0; };
	virtual bool Thread_IsTerminating() { return (m_Thread_State & MRTC_THREAD_TERMINATING) != 0; };
	virtual bool Thread_IsTerminated();

	virtual void Thread_RequestTermination();

	// Called by the creating thread
	virtual void Thread_Create(void* _pContext = NULL, int _StackSize = 16384, int _Priority = MRTC_THREAD_PRIO_ABOVENORMAL);
	virtual void Thread_Destroy();
  	virtual void Thread_OnCreate();							// Called before thread is created (even if thread creation fails.)
	virtual void Thread_OnDestroy();						// Called after thread has terminated.  - " -

	// Called by the new thread
	virtual void Thread_Sleep(fp32 _Time);				// In seconds
	virtual int Thread_Main() pure;
	virtual void Thread_Exit(int _ExitCode);

	static uint32 M_STDCALL Thread_Proc(void* _pContext);
};

typedef MRTC_Thread_Core MRTC_Thread;

template <typename t_CType>
class TMRTC_ThreadLocal
{
	aint m_ThreadLocalStorage;

	NThread::CMutual m_Lock;

	class CThreadStoreMember : public MRTC_Thread_Store_Member
	{
	public:
		TMRTC_ThreadLocal<t_CType> *m_pThreadLocal;
		CThreadStoreMember(TMRTC_ThreadLocal<t_CType> * _pThreadLocal, MRTC_Thread_Store *_pStore)
			:MRTC_Thread_Store_Member(_pStore)
		{
			m_pThreadLocal = _pThreadLocal;
		}

		~CThreadStoreMember()
		{
			DLock(m_pThreadLocal->m_Lock);
			m_LinkThreadLocal.Unlink();
		}

		DLinkDS_Link(CThreadStoreMember, m_LinkThreadLocal);
		t_CType m_Data;
	};

	DIdsListLinkDS_List_FromTemplate(CThreadStoreMember, m_LinkThreadLocal) m_CreatedMembers;

	virtual t_CType *fp_Get()
	{
		t_CType *pValue = (t_CType *)MRTC_SystemInfo::Thread_LocalGetValue(m_ThreadLocalStorage);

		if (pValue)
			return pValue;

		MRTC_Thread_Store *pThreadStore = MRTC_Thread_Core::Thread_GetStore();
		if (!pThreadStore)
		{
			MRTC_Thread_Core::Thread_AllocStore();
			pThreadStore = MRTC_Thread_Core::Thread_GetStore();
		}

		CThreadStoreMember *pMember = DNew(CThreadStoreMember) CThreadStoreMember(this, pThreadStore);

		{
			DLock(pThreadStore->m_Lock);
			pThreadStore->m_Members.Insert(pMember);
		}

		{
			DLock(m_Lock);
			m_CreatedMembers.Insert(pMember);
		}
		MRTC_SystemInfo::Thread_LocalSetValue(m_ThreadLocalStorage, (mint)&pMember->m_Data);

		return &pMember->m_Data;
	}

public:

	TMRTC_ThreadLocal()
	{
		m_ThreadLocalStorage = MRTC_SystemInfo::Thread_LocalAlloc();
	}

	~TMRTC_ThreadLocal()
	{
		MRTC_SystemInfo::Thread_LocalFree(m_ThreadLocalStorage);
		DLock(m_Lock);
		m_CreatedMembers.DeleteAll();
	}

	M_INLINE bint Allocated()
	{
		t_CType *pThreadStore = (t_CType *)MRTC_SystemInfo::Thread_LocalGetValue(m_ThreadLocalStorage);

		return pThreadStore != 0;
	}

	M_INLINE t_CType *Get()
	{
		t_CType *pThreadStore = (t_CType *)MRTC_SystemInfo::Thread_LocalGetValue(m_ThreadLocalStorage);

		if (pThreadStore)
			return pThreadStore;
		
		return fp_Get();
	}


	M_INLINE t_CType *operator -> ()
	{
		return Get();
	}

	M_INLINE t_CType &operator * ()
	{
		return *Get();
	}


};

/*
class MRTC_CriticalSection_Lock_Unlock
{
private:
	MRTC_CriticalSection *m_ToLock;
public:

	MRTC_CriticalSection_Lock_Unlock(MRTC_CriticalSection &_ToLock)
	{
		m_ToLock = &_ToLock;
		m_ToLock->Lock();
	}
	// ToTest if you have succeded call LockSucess
	MRTC_CriticalSection_Lock_Unlock(MRTC_CriticalSection &_ToLock, bool OnlyTry)
	{
		m_ToLock = &_ToLock;
		if (OnlyTry)
		{
			if (!m_ToLock->TryLock())
				m_ToLock = NULL;
		}
		else
			m_ToLock->Lock();
	}

	MRTC_CriticalSection_Lock_Unlock(MRTC_CriticalSection *_ToLock)
	{
		m_ToLock = _ToLock;
		m_ToLock->Lock();
	}
	// ToTest if you have succeded call LockSucess
	MRTC_CriticalSection_Lock_Unlock(MRTC_CriticalSection *_ToLock, bool OnlyTry)
	{
		m_ToLock = _ToLock;
		if (OnlyTry)
		{
			if (!m_ToLock->TryLock())
				m_ToLock = NULL;
		}
		else
			m_ToLock->Lock();
	}

	bool LockSucess()
	{
		return m_ToLock != NULL;
	}


	~MRTC_CriticalSection_Lock_Unlock()
	{
		if (m_ToLock)
			m_ToLock->Unlock();
	}
};


// Use this class if you want to temporary unlock an object in for example an loop where you want to giva another thread a chance to lock the object
class MRTC_CriticalSection_Unlock_Lock
{
private:
	MRTC_CriticalSection *m_ToUnlock;
public:

	MRTC_CriticalSection_Unlock_Lock(MRTC_CriticalSection &_ToUnlock)
	{
		m_ToUnlock = &_ToUnlock;
		m_ToUnlock->Unlock();
	}

	MRTC_CriticalSection_Unlock_Lock(MRTC_CriticalSection *_ToUnlock)
	{
		m_ToUnlock = _ToUnlock;
		m_ToUnlock->Unlock();
	}

	~MRTC_CriticalSection_Unlock_Lock()
	{
		m_ToUnlock->Lock();
	}
};
*/

#endif //MACRO_INC_MRTC_Thread_h
