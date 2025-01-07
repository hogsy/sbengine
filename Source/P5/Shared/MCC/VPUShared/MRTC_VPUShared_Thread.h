
#ifndef MACRO_INC_MRTC_VPUShared_Thread_h
#define MACRO_INC_MRTC_VPUShared_Thread_h

template < typename t_CType >
t_CType volatile& Volatile(t_CType& _Var)
{
    return const_cast<t_CType volatile&>(_Var);
}

namespace NThread
{

	class CAtomicIntAggregate
	{
		int32 volatile m_Int;
	public:

		void Construct()
		{
			m_Int = 0;
		}
		
		void Construct(int32 _Init)
		{
			m_Int = _Init;
		}

		void Destruct()
		{
		}

		// Returns old value
		M_INLINE int32 Increase()
		{
			return MRTC_SystemInfo::Atomic_Increase(&m_Int);
		}

		// Returns old value
		M_INLINE int32 Decrease()
		{
			return MRTC_SystemInfo::Atomic_Decrease(&m_Int);
		}

		// Returns old value
		M_INLINE int32 Exchange(int32 _Number)
		{
			return MRTC_SystemInfo::Atomic_Exchange(&m_Int, _Number);
		}

		// Returns old value
		M_INLINE int32 Add(int32 _Number)
		{
			return MRTC_SystemInfo::Atomic_Add(&m_Int, _Number);
		}

		// Returns the value in the variable
		M_INLINE int32 IfEqualExchange(int32 _CompareTo, int32 _SetTo)
		{
			return MRTC_SystemInfo::Atomic_IfEqualExchange(&m_Int, _CompareTo, _SetTo);
		}

		M_INLINE int32 Get()
		{
			return m_Int;
		}

		M_INLINE int32 operator ++()
		{
			return Increase() + 1;
		}

		M_INLINE int32 operator --()
		{
			return Decrease() - 1;
		}

		M_INLINE int32 operator += (int32 _ToAdd)
		{
			return Add(_ToAdd) + _ToAdd;
		}

		M_INLINE int32 operator -= (int32 _ToSub)
		{
			return Add((0-_ToSub)) - _ToSub;
		}

	};

	class CAtomicInt : public CAtomicIntAggregate
	{
	public:
		CAtomicInt()
		{
			Construct();
		}

		CAtomicInt(int32 _Init)
		{
			Construct(_Init);
		}
	};

	class CSpinLockAggregate
	{
	public:
		CAtomicInt m_Lock;
#ifdef MRTC_LOCK_TIMING
		volatile uint64 m_TimeSpentInLock;
		volatile uint64 m_LockCount;
#endif // MRTC_LOCK_TIMING
		M_INLINE void ClearTime()
		{
#ifdef MRTC_LOCK_TIMING
			m_TimeSpentInLock = 0;
			m_LockCount = 0;
#endif // MRTC_LOCK_TIMING
		}
		void Construct()
		{
			m_Lock.Exchange(0);
			ClearTime();
		}
		void Destruct()
		{
		}

		void Lock()
		{
			Internal_Lock();
			M_IMPORTBARRIER;
		}

		void Unlock()
		{
			M_EXPORTBARRIER;
			Internal_Unlock();
		}

		void Internal_Lock()
		{
#ifdef MRTC_LOCK_TIMING
			uint64 TimeStart = CMTimerFuncs_OS::Clock();
#endif // MRTC_LOCK_TIMING
			while(m_Lock.IfEqualExchange(0, 1) != 0) {}
#ifdef MRTC_LOCK_TIMING
			m_TimeSpentInLock += CMTimerFuncs_OS::Clock() - TimeStart;
			m_LockCount++;
#endif // MRTC_LOCK_TIMING
		}

		void Internal_Unlock()
		{
			m_Lock.Exchange(0);
		}
	};

	class CSpinLock : public CSpinLockAggregate
	{
	public:
		CSpinLock()
		{
			Construct();
		}
		~CSpinLock()
		{
			Destruct();
		}
	};


	template <typename t_CLock>
	class TCScopeLock
	{
		t_CLock *m_pLock;
	public:

		TCScopeLock(t_CLock &_Lock)
		{
			_Lock.Lock();
			m_pLock = &_Lock;
		}

		~TCScopeLock()
		{
			m_pLock->Unlock();
		}
	};

	template <typename t_CLock>
	class TCScopeUnlock
	{
		t_CLock *m_pLock;
	public:
		TCScopeUnlock(t_CLock &_Lock)
		{
			_Lock.Unlock();
			m_pLock = &_Lock;
		}

		~TCScopeUnlock()
		{
			m_pLock->Lock();
		}
	};

	template <typename t_CLock>
	class TCScopeLockRead
	{
		t_CLock *m_pLock;
	public:

		TCScopeLockRead(t_CLock &_Lock)
		{
			_Lock.LockRead();
			m_pLock = &_Lock;
		}

		~TCScopeLockRead()
		{
			m_pLock->UnlockRead();
		}
	};

	template <typename t_CLock>
	class TCScopeUnlockRead
	{
		t_CLock *m_pLock;
	public:
		TCScopeUnlockRead(t_CLock &_Lock)
		{
			_Lock.UnlockRead();
			m_pLock = &_Lock;
		}

		~TCScopeUnlockRead()
		{
			m_pLock->LockRead();
		}
	};

	typedef void (FLock)(void *_pLock);


	class CScopeLock
	{
		void *m_pLock;
		FLock *m_pUnlockFunc;
	public:

		template <typename t_Lock>
		class TLocker
		{
		public:
			static void Locker(void *_pLock)
			{
				((t_Lock *)_pLock)->Unlock();
			}
		};

		template <typename t_Lock>
		CScopeLock(t_Lock &_Lock)
		{
			_Lock.Lock();
			m_pLock = &_Lock;
			m_pUnlockFunc = TLocker<t_Lock>::Locker;
		}

		~CScopeLock()
		{
			m_pUnlockFunc(m_pLock);
		}
	};

	class CScopeUnlock
	{
		void *m_pLock;
		FLock *m_pLockFunc;
	public:

		template <typename t_Lock>
		class TLocker
		{
		public:
			static void Locker(void *_pLock)
			{
				((t_Lock *)_pLock)->Lock();
			}
		};

		template <typename t_Lock>
		CScopeUnlock(t_Lock &_Lock)
		{
			_Lock.Unlock();
			m_pLock = &_Lock;
			m_pLockFunc = TLocker<t_Lock>::Locker;
		}

		~CScopeUnlock()
		{
			m_pLockFunc(m_pLock);
		}
	};

	class CScopeLockRead
	{
		void *m_pLock;
		FLock *m_pUnlockFunc;
	public:

		template <typename t_Lock>
		class TLocker
		{
		public:
			static void Locker(void *_pLock)
			{
				((t_Lock *)_pLock)->UnlockRead();
			}
		};

		template <typename t_Lock>
		CScopeLockRead(t_Lock &_Lock)
		{
			_Lock.LockRead();
			m_pLock = &_Lock;
			m_pUnlockFunc = TLocker<t_Lock>::Locker;
		}

		~CScopeLockRead()
		{
			m_pUnlockFunc(m_pLock);
		}
	};

	class CScopeUnlockRead
	{
		void *m_pLock;
		FLock *m_pLockFunc;
	public:

		template <typename t_Lock>
		class TLocker
		{
		public:
			static void Locker(void *_pLock)
			{
				((t_Lock *)_pLock)->LockRead();
			}
		};

		template <typename t_Lock>
		CScopeUnlockRead(t_Lock &_Lock)
		{
			_Lock.UnlockRead();
			m_pLock = &_Lock;
			m_pLockFunc = TLocker<t_Lock>::Locker;
		}

		~CScopeUnlockRead()
		{
			m_pLockFunc(m_pLock);
		}
	};

#	define DLock(_ToLock) NThread::CScopeLock ScopeLockMutual1(_ToLock)
#	define DUnlock(_ToUnlock) NThread::CScopeUnlock ScopeUnlockMutual1(_ToUnlock)
#	define DLockTyped(_Type, _ToLock) _Type::CScopeLocker ScopeLockMutualTyped1(_ToLock)
#	define DUnlockTyped(_Type, _ToUnlock) _Type::CScopeUnlocker ScopeUnlockMutualTyped1(_ToUnlock)
#	define DLockRead(_ToLock) NThread::CScopeLockRead ScopeLockReadMutual1(_ToLock)
#	define DUnlockRead(_ToUnlock) NThread::CScopeUnlockRead ScopeUnlockReadMutual1(_ToUnlock)
#	define DLockTypedRead(_Type, _ToLock) _Type::CScopeLockerRead ScopeLockReadMutualTyped1(_ToLock)
#	define DUnlockTypedRead(_Type, _ToUnlock) _Type::CScopeUnlockerRead ScopeUnlockReadMutualTyped1(_ToUnlock)

#	define DLockTyped_FromTemplate(_Type, _ToLock) typename _Type::CScopeLocker ScopeLockMutualTyped1(_ToLock)
#	define DUnlockTyped_FromTemplate(_Type, _ToUnlock) typename _Type::CScopeUnlocker ScopeUnlockMutualTyped1(_ToUnlock)

}

#endif // MACRO_INC_MRTC_VPUShared_Thread_h
