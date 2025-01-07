/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Part of MRTC.h

	Author:			Magnus Högdahl

	Copyright:		Starbreeze Studios AB 1996,2003

	Contents:

	Comments:		Wrapper for OS stuff

	History:	
		030711:		File header added

\*____________________________________________________________________________________________*/

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_SystemInfo
|__________________________________________________________________________________________________
\*************************************************************************************************/

#define MRTC_THREAD_STILLACTIVE 0x00000103

enum 
{
	MRTC_THREAD_PRIO_IDLE = 0,
	MRTC_THREAD_PRIO_LOW = 0x1000,
	MRTC_THREAD_PRIO_BELOWNORMAL = 0x4000,
	MRTC_THREAD_PRIO_NORMAL = 0x7000,
	MRTC_THREAD_PRIO_ABOVENORMAL = 0xa000,
	MRTC_THREAD_PRIO_HIGHEST = 0xd000,
	MRTC_THREAD_PRIO_TIMECRITICAL = 0xffff
};

namespace NThread
{
	class CLock;
	class CEventAutoResetAggregate;
	class CEventAutoResetReportableAggregate;

	template <typename t_CEvent, typename t_CBase = CLock>
	class TMutual;

	typedef TMutual<CEventAutoResetAggregate> CMutual;

}
typedef NThread::CMutual MRTC_CriticalSection; 

namespace NNet
{
	class CNetAddressIPv4
	{
	public:
        uint8 m_IP[4];

		CNetAddressIPv4()
		{
			m_IP[0] = 0;
			m_IP[1] = 0;
			m_IP[2] = 0;
			m_IP[3] = 0;
		}

		CNetAddressIPv4(uint8 _0, uint8 _1, uint8 _2, uint8 _3)
		{
			m_IP[0] = _0;
			m_IP[1] = _1;
			m_IP[2] = _2;
			m_IP[3] = _3;
		}

		CNetAddressIPv4(const CNetAddressIPv4 &_Src)
		{
			m_IP[0] = _Src.m_IP[0];
			m_IP[1] = _Src.m_IP[1];
			m_IP[2] = _Src.m_IP[2];
			m_IP[3] = _Src.m_IP[3];
		}

		CNetAddressIPv4 &operator = (const CNetAddressIPv4 &_Src)
		{
			m_IP[0] = _Src.m_IP[0];
			m_IP[1] = _Src.m_IP[1];
			m_IP[2] = _Src.m_IP[2];
			m_IP[3] = _Src.m_IP[3];

			return *this;
		}

	};

	class CNetAddressTCPv4 : public CNetAddressIPv4
	{
	public:

		uint16 m_Port;

		CNetAddressTCPv4()
		{
			m_Port = 0;
		}

		CNetAddressTCPv4(const CNetAddressTCPv4 &_Src)
			:CNetAddressIPv4(_Src)
		{
			m_Port = _Src.m_Port;
		}

		CNetAddressTCPv4(const CNetAddressIPv4 &_Src)
			:CNetAddressIPv4(_Src)
		{
			m_Port = 0;
		}

		CNetAddressTCPv4 &operator = (const CNetAddressTCPv4 &_Src)
		{
			(*this) = *((CNetAddressIPv4 *)&_Src);

			m_Port = _Src.m_Port;

			return *this;
		}

		CNetAddressTCPv4 &operator = (const CNetAddressIPv4 &_Src)
		{
			m_IP[0] = _Src.m_IP[0];
			m_IP[1] = _Src.m_IP[1];
			m_IP[2] = _Src.m_IP[2];
			m_IP[3] = _Src.m_IP[3];

			return *this;
		}
	};

	class CNetAddressUDPv4 : public CNetAddressIPv4
	{
	public:

		uint16 m_Port;
		uint16 m_bBroadcast;

		CNetAddressUDPv4()
		{
			m_Port = 0;
			m_bBroadcast = false;
		}

		CNetAddressUDPv4(const CNetAddressUDPv4 &_Src) : CNetAddressIPv4(_Src)
		{
			(*this) = *((CNetAddressIPv4 *)&_Src);
			m_Port = _Src.m_Port;
			m_bBroadcast = _Src.m_bBroadcast;
		}

		CNetAddressUDPv4(const CNetAddressIPv4 &_Src)
			:CNetAddressIPv4(_Src)
		{
			m_Port = 0;
			m_bBroadcast = 0;
		}

		CNetAddressUDPv4 &operator = (const CNetAddressUDPv4 &_Src)
		{
			(*this) = *((CNetAddressIPv4 *)&_Src);

			m_Port = _Src.m_Port;
			m_bBroadcast = _Src.m_bBroadcast;

			return *this;
		}

		CNetAddressUDPv4 &operator = (const CNetAddressIPv4 &_Src)
		{
			*((CNetAddressIPv4 *)this) = *((CNetAddressIPv4 *)&_Src);

			return *this;
		}
	};


	enum ENetTCPState
	{
		ENetTCPState_Read			= DBit(0) // Data is awailable for reading
		,ENetTCPState_Write			= DBit(1) // More data can now be sent
		,ENetTCPState_Connection	= DBit(2) // A new connection is available for accept
		,ENetTCPState_Closed		= DBit(3) // The connection has been lost
	};
}

class MRTC_SystemInfo
{
public:
	uint32 m_nCPU;
//	uint32 m_CPUFrequency;
#ifndef	PLATFORM_PS2
	uint64 m_CPUFrequencyu;
	fp4 m_CPUFrequencyfp;
	fp4 m_CPUFrequencyRecp;

	uint64 m_OSFrequencyu;
	fp4 m_OSFrequencyfp;
	fp4 m_OSFrequencyRecp;
#endif
	uint32 m_CPUFeatures;
	uint32 m_CPUFeaturesEnabled;
	char m_CPUName[64];
	char m_CPUNameWithFeatures[64];

#ifdef CPU_X86
	char m_CPUID[13];
	uint32 m_CPUFamily;
	uint32 m_CPUModel;
#endif

	class MRTC_SystemInfoInternal *m_pInternalData;

	class MRTC_ThreadContext *m_pThreadContext;

//	NMem::TCPoolAggregate<NThread::CEventAutoResetReportableAggregate::CReportListMember>

protected:
	fp8 CPU_MeasureFrequencyOnce();

public:
	MRTC_SystemInfo();
	~MRTC_SystemInfo();

	// Frequency
#ifdef	PLATFORM_PS2
	static inline int32 GetCPUFrequencyu()
	{
		return 294912000;
	}
	static inline fp4 GetCPUFrequencyfp()
	{
		return 294912000.0f;
	}
	static inline fp4 GetCPUFrequencyRecp()
	{
		return 1.0f / 294912000.0f;
	}
#else	// PLATFORM_PS2
#endif	// PLATFORM_PS2

	// -------------------------------------------------------------------
	// CPU

	void PostCreate();

	void CPU_Detect();
	void CPU_MeasureFrequency();
	void CPU_CreateNames();
	const char* CPU_GetName(bool _bIncludeFeatures = true);
	void CPU_DisableFeatures(int);
	void CPU_EnableFeatures(int);
	static void CPU_AdvanceClock( fp4 _DeltaTime = 1.0f / 20.0f );	// This advances the fixed clock (fp4 is in seconds.. usually 1/20th of a second)

	static int64 OS_Clock();
	uint64 OS_ClockFrequencyInt() const;
	fp4 OS_ClockFrequencyFloat() const;
	fp4 OS_ClockFrequencyRecp() const;

	void OS_ClockFrequencyUpdate();

	static int64 CPU_Clock();
	uint64 CPU_ClockFrequencyInt() const;
	fp4 CPU_ClockFrequencyFloat() const;
	fp4 CPU_ClockFrequencyRecp() const;

//	static fp8 CPU_GetTime();

	static MRTC_SystemInfo& MRTC_GetSystemInfo();

	static void RD_GetServerName(char *_pName);
	static void RD_ClientInit(void *_pPacket, mint &_Size);
	static void RD_PeriodicUpdate();

	// -------------------------------------------------------------------
	// Memory
	
	static void* OS_Alloc(uint32 _Size, bool _bCommit);
	static void OS_Free(void *_pMem);
	static bool OS_Commit(void *_pMem, uint32 _Size, bool _bCommited);
	static uint32 OS_CommitGranularity();
	static void OS_HeapPush(int _MemoryType = 0);
	static void OS_HeapPop();

#ifdef M_THREADSPINCOUNT
	M_INLINE static int OS_ThreadSpinCount()
	{
		return M_THREADSPINCOUNT;
	}
#else
	static int OS_ThreadSpinCount();
#endif

	static void* OS_AllocGPU(uint32 _Size, bool _bCached);
	static void OS_FreeGPU(void *_pMem);

	static void* OS_HeapAlloc(uint32 _Size);
	static void* OS_HeapAllocAlign(uint32 _Size, uint32 _Align);
	static void* OS_HeapRealloc(void *_pMem, uint32 _Size);
	static void* OS_HeapReallocAlign(void *_pMem, uint32 _Size, uint32 _Align);
	static void OS_HeapFree(void *_pMem);
	static uint32 OS_HeapSize(const void *_pMem);
	static uint32 OS_PhysicalMemoryLowestFree();
	static uint32 OS_PhysicalMemoryFree();
	static uint32 OS_PhysicalMemorySize();
	static uint32 OS_PhysicalMemoryUsed();
	static uint32 OS_PhysicalMemoryLargestFree();

	static mint OS_TraceStack(mint *_pCallStack, int _MaxStack, mint _ebp = 0xffffffff);

	static void OS_SendProfilingSnapshot(uint32 _ID);

	// -------------------------------------------------------------------
	// Process / Thread

	static void* OS_GetProcessID();
	static void* OS_GetThreadID();
	static void OS_Sleep(int _Milliseconds);
	static void OS_Yeild();
	
	static void* OS_ThreadCreate(uint32(M_STDCALL*_pfnEntryPoint)(void*), int _StackSize, void* _pContext, int _ThreadPriority);
	static int OS_ThreadDestroy(void* _hThread);
	static void OS_ThreadTerminate(void* _hThread, int _ExitCode);
	static bool OS_ThreadIsRunning(void* _hThread);
	static void OS_ThreadExit(int _ExitCode);
	static int OS_ThreadGetExitCode(void* _hThread);

	static void *Semaphore_Alloc(mint _InitialCount, mint _MaximumCount);
	static void Semaphore_Free(void *_pSemaphore);
	static void Semaphore_Increase(void * _pSemaphore, mint _Count);
	static void Semaphore_Wait(void * _pSemaphore);
	static bint Semaphore_WaitTimeout(void * _pSemaphore, fp8 _Timeout);
	static bint Semaphore_TryWait(void * _pSemaphore);

	static void *Event_Alloc(bint _InitialSignal, bint _bAutoReset);
	static void Event_Free(void *_pEvent);
	static void Event_SetSignaled(void * _pEvent);
	static void Event_ResetSignaled(void * _pEvent);
	static void Event_Wait(void * _pEvent);
	static bint Event_WaitTimeout(void * _pEvent, fp8 _Timeout);
	static bint Event_TryWait(void * _pEvent);

#ifdef M_SEPARATETYPE_smint
	static smint Atomic_Increase(volatile smint *_pDest);
	static smint Atomic_Decrease(volatile smint *_pDest);
	static smint Atomic_Exchange(volatile smint *_pDest, smint _SetTo);
	static smint Atomic_Add(volatile smint *_pDest, smint _Add);
	static smint Atomic_IfEqualExchange(volatile smint *_pDest, smint _CompareTo, smint _SetTo);

#endif
	static int32 Atomic_Increase(volatile int32 *_pDest);
	static int32 Atomic_Decrease(volatile int32 *_pDest);
	static int32 Atomic_Exchange(volatile int32 *_pDest, int32 _SetTo);
	static int32 Atomic_Add(volatile int32 *_pDest, int32 _Add);
	static int32 Atomic_IfEqualExchange(volatile int32 *_pDest, int32 _CompareTo, int32 _SetTo);

	static void Thread_SetName(const char *_pName);
	static void Thread_SetProcessor(uint32 _Processor);

	static aint Thread_LocalAlloc();
	static void Thread_LocalFree(aint _Index);
	static void Thread_LocalSetValue(aint _Index, mint _Value);
	static mint Thread_LocalGetValue(aint _Index);


	class CNetwork
	{
	public:
		static bint gf_ResolveAddres(const ch8 *_pAddress, NNet::CNetAddressIPv4 &_Address);
		static void *gf_Bind(const NNet::CNetAddressUDPv4 &_Address, NThread::CEventAutoResetReportableAggregate *_pReportTo); // Only bind allowed on UDP sockets as they are connectionless
		static void *gf_Connect(const NNet::CNetAddressTCPv4 &_Address, NThread::CEventAutoResetReportableAggregate *_pReportTo); // Report to the supplied event when new data is received or when we are ready to send new data
		static void *gf_Listen(const NNet::CNetAddressTCPv4 &_Address, NThread::CEventAutoResetReportableAggregate *_pReportTo); // Report to the supplied event when a new connection has arrived
		static void *gf_Accept(void *_pSocket, NThread::CEventAutoResetReportableAggregate *_pReportTo); // Report to the supplied event when new data is received or when we are ready to send new data
		static void gf_SetReportTo(void *_pSocket, NThread::CEventAutoResetReportableAggregate *_pReportTo); // Report to the supplied event when new data is received or when we are ready to send new data				
		static uint32 gf_GetState(void *_pSocket); // Get the state of data available
		static void gf_Close(void *_pSocket); // Closes the socket and connection
		static int gf_Receive(void *_pSocket, void *_pData, int _DataLen); // Returns bytes received
		static int gf_Send(void *_pSocket, const void *_pData, int _DataLen); // Returns bytes sent

		static int gf_Receive(void *_pSocket, NNet::CNetAddressUDPv4 &_Address, void *_pData, int _DataLen); // Returns bytes received
		static int gf_Send(void *_pSocket, const NNet::CNetAddressUDPv4 &_Address, const void *_pData, int _DataLen); // Returns bytes sent

		static void *gf_InheritHandle(void *_pSocket, NThread::CEventAutoResetReportableAggregate *_pReportTo);
	};

	// -------------------------------------------------------------------
	// Syncrotization

	static void* OS_MutexOpen(const char* _pName);
	static void OS_MutexClose(void*);
	static void OS_MutexLock(void*);
	static void OS_MutexUnlock(void*);
	static bool OS_MutexTryLock(void*);

	// -------------------------------------------------------------------
	// Debug

	static void OS_Assert(const char*, const char* _pFile = NULL, int _Line = 0);
	static void M_ARGLISTCALL OS_Trace(const char *, ...);
	static void OS_TraceRaw(const char *);
	static void OS_EnableUnhandledException(bool _bEnabled);

	// -------------------------------------------------------------------
	// File

	static char* OS_DirectoryGetCurrent(char* _pBuf, int _MaxLength);
	static bool OS_DirectoryChange(const char* _pPath);
	static bool OS_DirectoryCreate(const char* _pPath);
	static bool OS_DirectoryRemove(const char* _pPath);
	static bool OS_DirectoryExists(const char *_pPath);
	static const char* OS_DirectorySeparator();

	static void *OS_FileOpen(const char *_pFilenName, bool _bRead, bool _bWrite, bool _bCreate, bool _bTruncate, bool _bDeferClose); // Returns file handle
	static void OS_FileClose(void *_pFile); // Closes file handle
	static void OS_FileGetDrive(const char *_pFilenName, char *_pDriveName); // Returns file handle
	static fint OS_FileSize(void *_pFile); // Checks if an async operation has finished
	static void *OS_FileAsyncRead(void *_pFile, void *_pData, fint _DataSize, fint _FileOffset); // Returns async handle that later can be checked for success
	static void *OS_FileAsyncWrite(void *_pFile, const void *_pData, fint _DataSize, fint _FileOffset); // Returns async handle that later can be checked for success
	static void OS_FileAsyncClose(void *_pAsyncInstance); // Closes an async operation
	static bool OS_FileAsyncIsFinished(void *_pAsyncInstance); // Checks if an async operation has finished
	static fint OS_FileAsyncBytesProcessed(void *_pAsyncInstance); // Checks if an async operation has finished
	static bool OS_FileSetFileSize(const char *_pFilenName, fint _FileSize); // Checks if an async operation has finished
	static int OS_FileOperationGranularity(const char *_pPath);
	static bool OS_FileExists(const char *_pPath);
	static fint OS_FilePosition( const char *_pFileName );	// Return position of file on disc in units
	static bool OS_FileRemove(const char* _pPath);
	static bool OS_FileRename(const char* _pFrom, const char* _pTo);
	

	static bool OS_FileGetTime(void *_pFile, int64& _TimeCreate, int64& _TimeAccess, int64& _TimeWrite);
	static bool OS_FileSetTime(void *_pFile, const int64& _TimeCreate, const int64& _TimeAccess, const int64& _TimeWrite);
};

#define DetectCPU() MRTC_SystemInfo::MRTC_GetSystemInfo().CPU_Detect()
#ifdef CPU_FEATURES_FORCE
	#define GetCPUFeatures() CPU_FEATURES_FORCE	
#else
	#define GetCPUFeatures() MRTC_SystemInfo::MRTC_GetSystemInfo().m_CPUFeatures
#endif
#define DisableCPUFeatures(Features) MRTC_SystemInfo::MRTC_GetSystemInfo().CPU_DisableFeatures(Features)
#define EnableCPUFeatures(Features) MRTC_SystemInfo::MRTC_GetSystemInfo().CPU_EnableFeatures(Features)
#define GetCPUName(_bIncludeFeatures) MRTC_SystemInfo::MRTC_GetSystemInfo().CPU_GetName(_bIncludeFeatures)
//#define GetCPUFrequency() MRTC_SystemInfo::MRTC_GetSystemInfo().m_CPUFrequencyd
//#define GetCPUFrequencyu() MRTC_SystemInfo::MRTC_GetSystemInfo().m_CPUFrequencyu
#ifdef	PLATFORM_PS2
	#define MGetCPUFrequencyFp() MRTC_SystemInfo::GetCPUFrequencyfp()
	#define MGetCPUFrequencyRecp() MRTC_SystemInfo::GetCPUFrequencyRecp()
	#define MGetCPUFrequencyInt() MRTC_SystemInfo::GetCPUFrequencyu()
#else	// PLATFORM_PS2
	#define MGetCPUFrequencyFp() MRTC_SystemInfo::MRTC_GetSystemInfo().CPU_ClockFrequencyFloat()
	#define MGetCPUFrequencyRecp() MRTC_SystemInfo::MRTC_GetSystemInfo().CPU_ClockFrequencyRecp()
	#define MGetCPUFrequencyInt() MRTC_SystemInfo::MRTC_GetSystemInfo().CPU_ClockFrequencyInt()
	#define MGetOSFrequencyFp() MRTC_SystemInfo::MRTC_GetSystemInfo().OS_ClockFrequencyFloat()
	#define MGetOSFrequencyRecp() MRTC_SystemInfo::MRTC_GetSystemInfo().OS_ClockFrequencyRecp()
	#define MGetOSFrequencyInt() MRTC_SystemInfo::MRTC_GetSystemInfo().OS_ClockFrequencyInt()
#endif	// PLATFORM_PS2
#define MGetCPUClock() MRTC_SystemInfo::CPU_Clock()
#define MGetOSClock() MRTC_SystemInfo::OS_Clock()
//#define GetCPUTime() MRTC_SystemInfo::CPU_GetTime()


class CMTimerFuncs_OS
{
public:
	M_INLINE static uint64 Clock()
	{
		return MGetOSClock();
	}
	M_INLINE static uint64 Frequency()
	{
		return MGetOSFrequencyInt();
	}
	M_INLINE static fp4 FrequencyFloat()
	{
		return MGetOSFrequencyFp();
	}
	M_INLINE static fp4 FrequencyRecp()
	{
		return MGetOSFrequencyRecp();
	}
};

class CMTimerFuncs_CPU
{
public:
	M_INLINE static uint64 Clock()
	{
		return MGetCPUClock();
	}
	M_INLINE static uint64 Frequency()
	{
		return MGetCPUFrequencyInt();
	}
	M_INLINE static fp4 FrequencyFloat()
	{
		return MGetCPUFrequencyFp();
	}
	M_INLINE static fp4 FrequencyRecp()
	{
		return MGetCPUFrequencyRecp();
	}
};

template <class t_CTimeFuncs = CMTimerFuncs_CPU>
class TCMStopTimer
{
public:
	int64 m_StartTime;
	int64 m_EndTime;

	TCMStopTimer(fp4 _Duration)
	{
		Start(_Duration);
	}

	void Start(fp4 _Duration)
	{
		m_StartTime = t_CTimeFuncs::Clock();
		m_EndTime = m_StartTime + int64(_Duration * t_CTimeFuncs::FrequencyFloat());
	}

	void Reset(fp4 _Duration)
	{
		m_StartTime = m_EndTime;
		m_EndTime = m_StartTime + int64(_Duration * t_CTimeFuncs::FrequencyFloat());
	}

	bool Done()
	{
		return t_CTimeFuncs::Clock() >= m_EndTime;
	}
};

typedef TCMStopTimer<> CMStopTimer;

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Class for managing time and timing
						
	Comments:			
\*____________________________________________________________________*/

#ifdef	PLATFORM_PS2

#include "MRTC_System_ps2.h"

#else

// CCFile isn't defined yet, so let's declare a dummy function for writing a fp8 to a file
void File_WriteLE(class CCFile* _pFile, fp8 _Value);
void File_ReadLE(class CCFile* _pFile, fp8& _Value);



template <class t_CTimeFuncs = CMTimerFuncs_OS>
class CMTime_Generic
{
private:
	int64 m_Time;
public:
	CMTime_Generic()
	{
		m_Time = 0;
	}

	CMTime_Generic(CMTime_Generic<t_CTimeFuncs> const&_Time)
	{
		m_Time = _Time.m_Time;
	}

	bool operator != (CMTime_Generic<t_CTimeFuncs> const&_Time)
	{
		return m_Time != _Time.m_Time;
	}

	CMTime_Generic<t_CTimeFuncs> &operator = (CMTime_Generic<t_CTimeFuncs> const&_Time)
	{
		m_Time = _Time.m_Time;
		return *this;
	}

	CMTime_Generic<t_CTimeFuncs> operator + (CMTime_Generic<t_CTimeFuncs> const&_Time) const
	{
		CMTime_Generic<t_CTimeFuncs> Time;
		Time.m_Time = m_Time + _Time.m_Time;
		return Time;
	}

	CMTime_Generic<t_CTimeFuncs> operator - (CMTime_Generic<t_CTimeFuncs> const&_Time) const
	{
		CMTime_Generic<t_CTimeFuncs> Time;
		Time.m_Time = m_Time - _Time.m_Time;
		return Time;
	}

	CMTime_Generic<t_CTimeFuncs> &operator += (CMTime_Generic<t_CTimeFuncs> const&_Time)
	{
		m_Time += _Time.m_Time;
		return *this;
	}

	CMTime_Generic<t_CTimeFuncs> &operator -= (CMTime_Generic<t_CTimeFuncs> const&_Time)
	{
		m_Time -= _Time.m_Time;
		return *this;
	}

	void Reset()
	{
		m_Time = 0;
	}

	bool IsReset() const
	{
		return m_Time == 0;
	}

	static CMTime_Generic<t_CTimeFuncs> CreateInvalid()
	{
		CMTime_Generic<t_CTimeFuncs> Invalid;
		Invalid.MakeInvalid();
		return Invalid;
	}

	void MakeInvalid()
	{
		m_Time = -1;
	}

	bool IsInvalid() const
	{
		return m_Time == -1;
	}

	void ResetMax()
	{
		m_Time = 0x7fffffffffffffffLL;
	}

	void Snapshot()
	{
		m_Time = t_CTimeFuncs::Clock();
	}

	void Start()
	{
		m_Time -= t_CTimeFuncs::Clock();
	}

	void Stop()
	{
		m_Time += t_CTimeFuncs::Clock();
	}

	void Add(CMTime_Generic<t_CTimeFuncs> &_Time)
	{
		m_Time += _Time.m_Time;
	}

	fp4 GetTime() const
	{
		return (fp4)m_Time * t_CTimeFuncs::FrequencyRecp();
	}

	fp8 GetTimefp8() const
	{
		return (fp8)m_Time * fp8(t_CTimeFuncs::FrequencyRecp());
	}

	int64 GetCycles() const
	{
		return m_Time;
	}

	int Compare(CMTime_Generic<t_CTimeFuncs> const&_Time) const
	{		
		if (m_Time < _Time.m_Time)
			return -1;
		else if (m_Time > _Time.m_Time)
			return 1;
		return 0;
	}

	int Compare(fp4 _Time) const
	{
		return Compare( CreateFromSeconds( _Time ) );
	}

	// Default error margin is 100 micro seconds
	bool AlmostEqual(const CMTime_Generic<t_CTimeFuncs>& _Time, fp4 _Epsilon = 0.0001f) const
	{
		int64 nDiff = m_Time - _Time.m_Time;
		int64 nEpsilon = _Epsilon * t_CTimeFuncs::FrequencyFloat();
		return (nDiff > -nEpsilon  && nDiff < nEpsilon);
	}

	CMTime_Generic<t_CTimeFuncs> Modulus(fp4 _Modulus) const
	{
		CMTime_Generic<t_CTimeFuncs> Ret;
		CMTime_Generic<t_CTimeFuncs> Modul = CMTime_Generic<t_CTimeFuncs>::CreateFromSeconds( _Modulus );
		Ret.m_Time = m_Time % Modul.m_Time;
		return Ret;
	}

	CMTime_Generic<t_CTimeFuncs> Modulus(CMTime_Generic<t_CTimeFuncs> _Modulus) const
	{
		CMTime_Generic<t_CTimeFuncs> Ret;
		Ret.m_Time = m_Time % _Modulus.m_Time;
		return Ret;
	}

	CMTime_Generic<t_CTimeFuncs> ModulusScaled(fp4 _Scale, fp4 _Modulus) const
	{
		CMTime_Generic<t_CTimeFuncs> Ret;
		_Modulus = _Modulus * (1.0f / _Scale);

		CMTime_Generic<t_CTimeFuncs> Modul = CMTime_Generic<t_CTimeFuncs>::CreateFromSeconds( _Modulus );
		int64 Int64Scale = (_Scale * 65536.0f);
		// This isn't really safe, should be done with 128 bit ints

		Ret.m_Time = (((m_Time % Modul.m_Time)) * Int64Scale) >> 16;
		return Ret;
	}

	CMTime_Generic<t_CTimeFuncs> Scale(fp4 _Scale) const
	{
		CMTime_Generic<t_CTimeFuncs> Ret;

		int64 Int64Scale = (_Scale * 65536.0f);
		// This isn't really safe, should be done with 128 bit ints

		Ret.m_Time = (m_Time * Int64Scale) >> 16;
		return Ret;
	}


	fp4 GetTimeFraction(fp4 _Modulus) const
	{
		return GetTimeModulus(_Modulus) * (1.0f / _Modulus);
	}

	int GetNumModulus(fp4 _Modulus) const
	{
		int64 Int64Modulus = _Modulus * t_CTimeFuncs::FrequencyFloat();

		return m_Time / Int64Modulus;
	}

	int GetNumTicks(fp4 _TicksPerSec) const
	{
		int64 Int64Scale = (_TicksPerSec * 65536.0f);
		// This isn't really safe, should be done with 128 bit ints
		int64 Ticks = (m_Time * Int64Scale) >> 16;

		return Ticks / t_CTimeFuncs::Frequency();
	}

	fp4 GetTimeModulus(fp4 _Modulus) const
	{
		int64 Int64Modulus = _Modulus * t_CTimeFuncs::FrequencyFloat();

		int64 Time = m_Time % Int64Modulus;

		return (fp4)Time * t_CTimeFuncs::FrequencyRecp();
	}

	fp4 GetTimeModulusScaled(fp4 _Scale, fp4 _Modulus) const
	{
		_Modulus = _Modulus * (1.0f / _Scale);

		int64 Int64Modulus = _Modulus * t_CTimeFuncs::FrequencyFloat();

		int64 Time = m_Time % Int64Modulus;

		fp4 Modded = (fp4)Time * t_CTimeFuncs::FrequencyRecp();
		return Modded * _Scale;
	}

	fp4 GetTimeSqrModulusScaled(fp4 _Scale, fp4 _Modulus) const
	{
		_Scale *= GetTime();
		_Modulus = _Modulus * (1.0f / _Scale);

		int64 Int64Modulus = _Modulus * t_CTimeFuncs::FrequencyFloat();

		int64 TimeSqr = m_Time;

		int64 Time = TimeSqr % Int64Modulus;

		fp4 Modded = (fp4)Time * t_CTimeFuncs::FrequencyRecp();
		return Modded * _Scale;
	}

	CMTime_Generic<t_CTimeFuncs> Max(CMTime_Generic<t_CTimeFuncs> const&_Time) const
	{
		CMTime_Generic<t_CTimeFuncs> Ret;
		Ret.m_Time = ::Max(m_Time, _Time.m_Time);
		return Ret;
	}

	CMTime_Generic<t_CTimeFuncs> Min(CMTime_Generic<t_CTimeFuncs> const&_Time) const
	{
		CMTime_Generic<t_CTimeFuncs> Ret;
		Ret.m_Time = ::Min(m_Time, _Time.m_Time);
		return Ret;
	}

	static CMTime_Generic<t_CTimeFuncs> GetCPU()
	{
		CMTime_Generic<t_CTimeFuncs> Ret;
		Ret.m_Time = t_CTimeFuncs::Clock();
		return Ret;
	}

	static CMTime_Generic<t_CTimeFuncs> CreateFromSeconds(fp4 _Seconds)
	{
		CMTime_Generic<t_CTimeFuncs> Ret;

		Ret.m_Time = _Seconds * t_CTimeFuncs::FrequencyFloat();

		return Ret;
	}

	static CMTime_Generic<t_CTimeFuncs> CreateFromTicks(int64 _Ticks, fp4 _TickLength, fp4 _TickFraction = 0.0f)
	{
		CMTime_Generic<t_CTimeFuncs> Ret;

		Ret.m_Time = _TickLength * t_CTimeFuncs::FrequencyFloat();
		Ret.m_Time *= _Ticks;
		Ret.m_Time += (_TickFraction * _TickLength) * t_CTimeFuncs::FrequencyFloat();

		return Ret;
	}

	// This should be rewritten to pack in 1 MHz int64 precission or something to be platform independent (if its to slow to use fp8 here)
	void Pack(uint8 *&_pPtr) const
	{
		fp8 Time = (fp8)m_Time * fp8(t_CTimeFuncs::FrequencyRecp());
		(*(fp8*)_pPtr) = Time;
		_pPtr += sizeof(Time);
	}

	void Unpack(const uint8 *&_pPtr)
	{
		fp8 Time = (*(fp8*)_pPtr);
		m_Time = (Time * fp8(t_CTimeFuncs::FrequencyFloat()));
		_pPtr += sizeof(Time);
	}

	void Write(class CCFile* _pFile) const
	{
		fp8 Time = (fp8)m_Time * fp8(t_CTimeFuncs::FrequencyRecp());
		File_WriteLE(_pFile, Time);
	}

	void Read(class CCFile* _pFile)
	{
		fp8 Time;
		File_ReadLE(_pFile, Time);
		m_Time = (Time * fp8(t_CTimeFuncs::FrequencyFloat()));
	}

};

template <class t_CTimeFuncs = CMTimerFuncs_OS>
class CMTime_fp8
{
private:
	fp8 m_Time;
public:
	CMTime_fp8()
	{
		m_Time = 0;
	}
	CMTime_fp8(CMTime_fp8<t_CTimeFuncs> const &_Time)
	{
		m_Time = _Time.m_Time;
	}
	bool operator != (CMTime_fp8<t_CTimeFuncs> const &_Time)
	{
		return m_Time != _Time.m_Time;
	}

	CMTime_fp8<t_CTimeFuncs> &operator = (CMTime_fp8<t_CTimeFuncs> const  &_Time)
	{
		m_Time = _Time.m_Time;
		return *this;
	}

	CMTime_fp8<t_CTimeFuncs> operator + (CMTime_fp8<t_CTimeFuncs> const  &_Time) const
	{
		CMTime_fp8 Time;
		Time.m_Time = m_Time + _Time.m_Time;
		return Time;
	}

	CMTime_fp8<t_CTimeFuncs> operator - (CMTime_fp8<t_CTimeFuncs> const  &_Time) const
	{
		CMTime_fp8<t_CTimeFuncs> Time;
		Time.m_Time = m_Time - _Time.m_Time;
		return Time;
	}

	CMTime_fp8<t_CTimeFuncs> &operator += (CMTime_fp8<t_CTimeFuncs> const  &_Time)
	{
		m_Time += _Time.m_Time;
		return *this;
	}

	CMTime_fp8<t_CTimeFuncs> &operator -= (CMTime_fp8<t_CTimeFuncs> const  &_Time)
	{
		m_Time -= _Time.m_Time;
		return *this;
	}

	void Reset()
	{
		m_Time = 0;
	}

	bool IsReset() const
	{
		return m_Time == 0;
	}

	static CMTime_fp8<t_CTimeFuncs> CreateInvalid()
	{
		CMTime_fp8<t_CTimeFuncs> Invalid;
		Invalid.MakeInvalid();
		return Invalid;
	}

	void MakeInvalid()
	{
		m_Time = -1.0;
	}

	bool IsInvalid() const
	{
		return m_Time == -1.0;
	}

	void ResetMax()
	{
		m_Time = 0x7fffffffffffffffLL;
	}

	void Snapshot()
	{
		m_Time = (fp8)t_CTimeFuncs::Clock() * (fp8)t_CTimeFuncs::FrequencyRecp();
	}

	void Start()
	{
		m_Time -= (fp8)t_CTimeFuncs::Clock() * (fp8)t_CTimeFuncs::FrequencyRecp();
	}

	void Stop()
	{
		m_Time += (fp8)t_CTimeFuncs::Clock() * (fp8)t_CTimeFuncs::FrequencyRecp();
	}

	void Add(CMTime_fp8<t_CTimeFuncs> &_Time)
	{
		m_Time += _Time.m_Time;
	}

	fp4 GetTime() const
	{
		return m_Time;
	}

	fp8 GetTimefp8() const
	{
		return m_Time;
	}

	int64 GetCycles() const
	{
		return (int64)(m_Time * (fp8)t_CTimeFuncs::FrequencyFloat());
	}

	int Compare(CMTime_fp8<t_CTimeFuncs> const  &_Time) const
	{		
		if (m_Time < _Time.m_Time)
			return -1;
		else if (m_Time > _Time.m_Time)
			return 1;
		return 0;
	}

	int Compare(fp4 _Time) const
	{
		return Compare( CreateFromSeconds( _Time ) );
	}

	CMTime_fp8<t_CTimeFuncs> Modulus(fp4 _Modulus) const
	{
		CMTime_fp8<t_CTimeFuncs> Ret;
		Ret.m_Time = fmod(m_Time, (fp8)_Modulus);
		return Ret;
	}

	CMTime_fp8<t_CTimeFuncs> Modulus(CMTime_fp8<t_CTimeFuncs> _Modulus) const
	{
		CMTime_fp8<t_CTimeFuncs> Ret;
		Ret.m_Time = fmod(m_Time, _Modulus.m_Time);
		return Ret;
	}

	CMTime_fp8<t_CTimeFuncs> ModulusScaled(fp4 _Scale, fp4 _Modulus) const
	{
		CMTime_fp8<t_CTimeFuncs> Ret;
		Ret.m_Time = fmod(m_Time * _Scale, (fp8)_Modulus);
		return Ret;
	}

	CMTime_fp8<t_CTimeFuncs> Scale(fp4 _Scale) const
	{
		CMTime_fp8<t_CTimeFuncs> Ret;
		Ret.m_Time = m_Time * _Scale;
		return Ret;
	}


	fp4 GetTimeFraction(fp4 _Modulus) const
	{
		return GetTimeModulus(_Modulus) * (1.0f / _Modulus);
	}

	int GetNumModulus(fp4 _Modulus) const
	{
		return (int)(m_Time / _Modulus);
	}

	int GetNumTicks(fp4 _TicksPerSec) const
	{
		return (int)(m_Time * _TicksPerSec);
	}

	fp4 GetTimeModulus(fp4 _Modulus) const
	{
		return fmod(m_Time, (fp8)_Modulus);
	}

	fp4 GetTimeModulusScaled(fp4 _Scale, fp4 _Modulus) const
	{
		return fmod(m_Time * _Scale, (fp8)_Modulus);
	}

	fp4 GetTimeSqrModulusScaled(fp4 _Scale, fp4 _Modulus) const
	{
		return fmod(m_Time * m_Time * _Scale, (fp8)_Modulus);
	}

	bool AlmostEqual(const CMTime_fp8<t_CTimeFuncs>& _Time, fp4 _Epsilon = 0.0001f) const
	{
		fp8 fDiff = m_Time - _Time.m_Time;
		return (fDiff > -_Epsilon  && fDiff < _Epsilon);
	}

	CMTime_fp8<t_CTimeFuncs> Max(CMTime_fp8<t_CTimeFuncs> const  &_Time) const
	{
		CMTime_fp8<t_CTimeFuncs> Ret;
		Ret.m_Time = ::Max(m_Time, _Time.m_Time);
		return Ret;
	}

	CMTime_fp8<t_CTimeFuncs> Min(CMTime_fp8<t_CTimeFuncs> const  &_Time) const
	{
		CMTime_fp8<t_CTimeFuncs> Ret;
		Ret.m_Time = ::Min(m_Time, _Time.m_Time);
		return Ret;
	}

	static CMTime_fp8<t_CTimeFuncs> GetCPU()
	{
		CMTime_fp8<t_CTimeFuncs> Ret;
		Ret.m_Time = (fp8)t_CTimeFuncs::Clock() * (fp8)t_CTimeFuncs::FrequencyRecp();
		return Ret;
	}

	static CMTime_fp8<t_CTimeFuncs> CreateFromSeconds(fp4 _Seconds)
	{
		CMTime_fp8<t_CTimeFuncs> Ret;

		Ret.m_Time = _Seconds;

		return Ret;
	}

	static CMTime_fp8<t_CTimeFuncs> CreateFromTicks(int64 _Ticks, fp4 _TickLength, fp4 _TickFraction = 0.0f)
	{
		CMTime_fp8<t_CTimeFuncs> Ret;
		Ret.m_Time = _TickLength * _Ticks + (_TickFraction * _TickLength);
		return Ret;
	}

	// This should be rewritten to pack in 1 MHz int64 precission or something to be platform independent (if its to slow to use fp8 here)
	void Pack(uint8 *&_pPtr) const
	{
		memcpy(_pPtr, &m_Time, sizeof(m_Time));
		_pPtr += sizeof(m_Time);
	}

	void Unpack(const uint8 *&_pPtr)
	{
		memcpy(&m_Time, _pPtr, sizeof(m_Time));
		_pPtr += sizeof(m_Time);
	}

	void Write(class CCFile* _pFile) const
	{
		File_WriteLE(_pFile, m_Time);
	}

	void Read(class CCFile* _pFile)
	{
		File_ReadLE(_pFile, m_Time);
	}

};

#if defined(PLATFORM_CONSOLE) || !defined(M_Profile)
typedef	CMTime_fp8<>	CMTime;
#else
typedef	CMTime_Generic<>	CMTime;
#endif

typedef	CMTime_Generic<>	CMTimeCPU;

#endif

template <typename t_CTimerClass>
class TCMTimeScope
{
public:
	t_CTimerClass &m_Time;
	
	TCMTimeScope(t_CTimerClass &_Time, bool _bReset = false) : m_Time(_Time)
	{
		if (_bReset)
			m_Time.Reset();

		m_Time.Start();
	}

	~TCMTimeScope()
	{
		m_Time.Stop();
	}
};

typedef TCMTimeScope<CMTime> CMTimeScope;

#ifdef	PLATFORM_PS2
// fast memcpy functions
// data has to be aligned
static M_INLINE void PS2QWordCopy( uint128* _pDest, const uint128* _pSrc, unsigned int _nCount )
{
	unsigned int odd = _nCount & 3;
	_nCount -= odd;
	while( _nCount > 0 )
	{
		register uint128 a, b, c, d;
		
		a	= _pSrc[0];
		b	= _pSrc[1];
		c	= _pSrc[2];
		d	= _pSrc[3];
		_pDest[0]	= a;
		_pDest[1]	= b;
		_pDest[2]	= c;
		_pDest[3]	= d;
		
		_pDest	+= 4;
		_pSrc	+= 4;
		_nCount	-= 4;
	}
	
	if( odd )
	{
		while( odd-- > 0 )
		{
			*_pDest++	= *_pSrc++;
		}
	}
}

static M_INLINE void PS2WordCopy( void* _pDest, const void* _pSrc, unsigned int _nWords )
{
	register const uint32 *pSrc = (const uint32*)_pSrc;
	register uint32 *pDest = (uint32*)_pDest;
	unsigned int odd = _nWords & 3;
	_nWords	-= odd;
	
	while( _nWords > 0 )
	{
		register uint32 a, b, c, d;
		a	= pSrc[0];
		b	= pSrc[1];
		c	= pSrc[2];
		d	= pSrc[3];
		pDest[0]	= a;
		pDest[1]	= b;
		pDest[2]	= c;
		pDest[3]	= d;
		_nWords	-= 4;
		pDest	+= 4;
		pSrc	+= 4;
	}
	
	if( odd )
	{
		while( odd-- > 0 )
		{
			*pDest++	= *pSrc++;
		}
	}
}

#endif


#	define TStartAdd(timevar) {timevar.Start();}
#	define TMeasureC(timevar, _Class) TCMTimeScope<_Class> TimeScope(timevar);
#	define TMeasureResetC(timevar, _Class) TCMTimeScope<_Class> TimeScope(timevar, 1);

#	define TMeasure(timevar) CMTimeScope TimeScope(timevar);
#	define TMeasureReset(timevar) CMTimeScope TimeScope(timevar, 1);
#	define TStart(timevar) {(timevar).Reset(); (timevar).Start();}
#	define TStop(timevar) {(timevar).Stop();}
#	define TCycles(name, timevar) CStrF("%s %d c", name, (timevar).GetCycles())
#	define TString(name, timevar) CStrF("%s %0.2f ms", name, (timevar).GetTime()*1000.0)
#	define TMbPerSec(timevar, bytes) CStrF("%0.2f Mb/s", (fp4(bytes)/1000000)/((timevar).GetTime()))
#	define TxPerSec(timevar, tick, unit) CStrF("%0.2f%s", fp4(tick)/((timevar).GetTime()), unit)

#ifdef M_Profile
#	define MIncProfile(var) ++var
#	define MAddProfile(var, add) var += add;
#	define TProfileDef(timevar) CMTime timevar
#	define TStartAddProfile(timevar) TStartAdd(timevar)
#	define TMeasureProfile(timevar) TMeasure(timevar)
#	define TMeasureResetProfile(timevar) TMeasureReset(timevar)
#	define TStartProfile(timevar) TStart(timevar)
#	define TStopProfile(timevar) TStop(timevar)
#	define TAddProfile(timevar, time) timevar.Add(time)
#else
#	define MIncProfile(var) ((void)0)
#	define MAddProfile(var, add) ((void)0)
#	define TMeasureProfile(timevar) ((void)0);
#	define TMeasureResetProfile(timevar) ((void)0);
#	define TProfileDef(timevar) ((void)0)
#	define TStartAddProfile(timevar) ((void)0)
#	define TStartProfile(timevar) ((void)0)
#	define TStopProfile(timevar) ((void)0)
#	define TAddProfile(timevar, time) ((void)0)
#endif

