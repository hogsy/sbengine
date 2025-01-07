/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Part of MRTC.h

	Author:			Magnus Högdahl

	Copyright:		Starbreeze Studios AB 1996,2003

	Contents:

	Comments:		Wrapper for OS stuff

	History:	
		030711:		File header added

\*____________________________________________________________________________________________*/

#ifndef MACRO_INC_MRTC_System_h
#define MACRO_INC_MRTC_System_h

#include "MRTC_Bit.h"

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
		ENetTCPState_Read			= M_Bit(0) // Data is awailable for reading
		,ENetTCPState_Write			= M_Bit(1) // More data can now be sent
		,ENetTCPState_Connection	= M_Bit(2) // A new connection is available for accept
		,ENetTCPState_Closed		= M_Bit(3) // The connection has been lost
	};
}

class MRTC_SystemInfo
{
public:
	uint32 m_nCPU;
//	uint32 m_CPUFrequency;
#ifndef	PLATFORM_PS2
	uint64 m_CPUFrequencyu;
	fp32 m_CPUFrequencyfp;
	fp32 m_CPUFrequencyRecp;

	uint64 m_OSFrequencyu;
	fp32 m_OSFrequencyfp;
	fp32 m_OSFrequencyRecp;
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
	fp64 CPU_MeasureFrequencyOnce();

public:
	MRTC_SystemInfo();
	~MRTC_SystemInfo();

	// Frequency
#ifdef	PLATFORM_PS2
	static inline int32 GetCPUFrequencyu()
	{
		return 294912000;
	}
	static inline fp32 GetCPUFrequencyfp()
	{
		return 294912000.0f;
	}
	static inline fp32 GetCPUFrequencyRecp()
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
	static void CPU_AdvanceClock( fp32 _DeltaTime = 1.0f / 20.0f );	// This advances the fixed clock (fp32 is in seconds.. usually 1/20th of a second)

	static int64 OS_Clock();
	uint64 OS_ClockFrequencyInt() const;
	fp32 OS_ClockFrequencyFloat() const;
	fp32 OS_ClockFrequencyRecp() const;

	void OS_ClockFrequencyUpdate();

	static int64 CPU_Clock();
	uint64 CPU_ClockFrequencyInt() const;
	fp32 CPU_ClockFrequencyFloat() const;
	fp32 CPU_ClockFrequencyRecp() const;

	static void OS_NamedEvent_Begin(const char* _pName, uint32 _Color);
	static void OS_NamedEvent_End();

	static MRTC_SystemInfo& MRTC_GetSystemInfo();

	static void RD_GetServerName(char *_pName);
	static void RD_ClientInit(void *_pPacket, mint &_Size);
	static void RD_PeriodicUpdate();

	// -------------------------------------------------------------------
	// Memory
	
	static void* OS_Alloc(uint32 _Size, uint32 _Alignment);
	static mint OS_MemSize(void *_pBlock);
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
	
	static void* OS_ThreadCreate(uint32(M_STDCALL*_pfnEntryPoint)(void*), int _StackSize, void* _pContext, int _ThreadPriority, const char* _pName);
	static int OS_ThreadDestroy(void* _hThread);
	static void OS_ThreadTerminate(void* _hThread, int _ExitCode);
	static bool OS_ThreadIsRunning(void* _hThread);
	static void OS_ThreadExit(int _ExitCode);
	static int OS_ThreadGetExitCode(void* _hThread);

	static void *Semaphore_Alloc(mint _InitialCount, mint _MaximumCount);
	static void Semaphore_Free(void *_pSemaphore);
	static void Semaphore_Increase(void * _pSemaphore, mint _Count);
	static void Semaphore_Wait(void * _pSemaphore);
	static bint Semaphore_WaitTimeout(void * _pSemaphore, fp64 _Timeout);
	static bint Semaphore_TryWait(void * _pSemaphore);

	static void *Event_Alloc(bint _InitialSignal, bint _bAutoReset);
	static void Event_Free(void *_pEvent);
	static void Event_SetSignaled(void * _pEvent);
	static void Event_ResetSignaled(void * _pEvent);
	static void Event_Wait(void * _pEvent);
	static bint Event_WaitTimeout(void * _pEvent, fp64 _Timeout);
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
	static void Thread_SetProcessor(uint32 _ThreadID, uint32 _Processor);
	static uint32 Thread_GetCurrentID();

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
	static void OS_EnableAutoCoredumpOnException(bool _bEnabled);

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



/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Class for managing time and timing
						
	Comments:			
\*____________________________________________________________________*/


// CCFile isn't defined yet, so let's declare a dummy function for writing a fp64 to a file
void File_WriteLE(class CCFile* _pFile, fp64 _Value);
void File_ReadLE(class CCFile* _pFile, fp64& _Value);

#include "MRTC_System_PS3.h"

#endif //MACRO_INC_MRTC_System_h
