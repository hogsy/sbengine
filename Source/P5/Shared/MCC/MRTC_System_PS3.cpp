
// Need to implement MRTC_SystemInfo

#include <sys/time_util.h>
#include <sys/sys_time.h>
//#include <network.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
//#include <netex/netset.h>
#include <netex/net.h>
#include <netex/errno.h>
#include <arpa/inet.h>
 
void DebugBreak()
{
	asm volatile("tw 31, 1, 1");
}
/*
template <int _nElementSize, int _nCount>
class TAllocPool
{
	enum { FlagCount = (_nCount + 63) / 64 };
protected:
	uint64 m_AllocFlags[FlagCount];
	uint8	m_aData[_nElementSize * _nCount];

public:
	void Init()
	{
		memset(m_AllocFlags, 0, sizeof(uint64) * FlagCount);
	}
	void* AllocPrim(int _CheckSize)
	{
		M_ASSERT(_CheckSize == _nElementSize, "Primitives must have a set size");
		for(int i = 0; i < FlagCount; i++)
		{
			if(m_AllocFlags[i] != ~(0UL))
			{
				for(int iBit = 0; iBit < 64; iBit++)
				{
					if(!(m_AllocFlags[i] & (1 << iBit)))
					{
						m_AllocFlags[i] |= (1 << iBit);
						uint64 iElem = i * 64 + iBit;
						return m_aData + iElem * _nElementSize;
					}
				}
			}
		}

		MRTC_SystemInfo::OS_Assert("Out of primitives");
		return NULL;
	}

	void FreePrim(void* _pMem)
	{
		uint8* pMem = (uint8*)_pMem;
		aint iPrim = (aint)(pMem - &m_aData[0]) / _nElementSize;
		
		uint64 iPrimOffset = iPrim / 64;
		uint64 iPrimBit = iPrim % 63;
		m_AllocFlags[iPrimOffset] &= ~iPrimBit;
	}
};
*/

struct sys_event_t
{
	int32 m_Signaled;
	int32 m_AutoReset;	// If this is false then woken threads should signal the condition to wake the others
	sys_mutex_t m_Mutex;
	sys_cond_t m_Condition;
};
struct sys_sem_t
{
	int32 m_CurrentCount;
	int32 m_MaxCount;
	sys_mutex_t m_Mutex;
	sys_cond_t m_Condition;
};
union UPrimitive
{
	sys_event_t m_Event;
	sys_sem_t m_Sem;
};

class MRTC_SystemInfoInternal
{
public:
	TDA_Pool<UPrimitive> m_PrimitivePool;
	MRTC_SystemInfoInternal()
	{
		m_pTCPContext = NULL;
//		m_PrimitivePool.Init();
	}

	class CTCPContext
	{
	public:
		bint m_bInitFailed;
//		sys_netset_id_t m_sid;

		CTCPContext() : m_bInitFailed(false)//, m_sid(0)
		{
			Init();
		}

		~CTCPContext()
		{
			m_SocketTree.f_DeleteAll();
/*
			if(!m_bInitFailed)
			{
				int ret = sys_netset_if_down(m_sid, -1, 0);
				if(ret < 0)
					MRTC_SystemInfo::OS_Assert(CFStrF("sys_netset_if_down failed(%d, %d)\n", ret, sys_net_errno).Str());
				ret = sys_netset_close(m_sid, 0);
				if(ret < 0)
					MRTC_SystemInfo::OS_Assert(CFStrF("sys_netset_if_close failed(%d, %d)\n", ret, sys_net_errno).Str());
				sys_net_finalize_network();
			}
*/		}
 
		void Init()
		{
/*
			m_bInitFailed = true;
			if(sys_net_initialize_network() != 0)
				return;

			m_sid = -1;
			do
			{
				static char g_settings[] =
					"type nic\n"
					"my_name \"my_hostname1\"\n"	// "my_name \"MAC:00:04:1f:01:02:03\"\n"
					"dhcp\n"
					"\n"
					"vendor \"SCE\"\n"
					"product \"Gigabit Ethernet\"\n"
					"phy_config auto\n"; 
				sys_netset_id_t sid = sys_netset_open(g_settings, NULL, 0);
				if(sid < 0)
					break;
				int ret = sys_netset_if_up(sid, 15 * 1000, 0);
				if(ret < 0)
				{
					sys_netset_close(sid, 0);
					break;
				}
				m_sid = sid;

			} while(0);

			if(m_sid < 0)
			{
				sys_net_finalize_network();
				return;
			}
*/
		}

		class CTCPSocket : public MRTC_Thread
		{
		public:
			MRTC_CriticalSection m_Lock;

			CTCPSocket()
			{
				m_State = 0;
				m_pReportTo = NULL;
				m_Socket = NULL;
				m_bListenSocket = false;
				m_bConnectionSocket = false;
				Thread_Create(NULL, 16384, MRTC_THREAD_PRIO_HIGHEST);
			}

			~CTCPSocket()
			{
				Thread_Destroy();
				if(m_Socket && !(m_State & DBit(31)))
				{
					socketclose(m_Socket);
					m_Socket = 0;
				}
			}

			virtual int Thread_Main()
			{
				while(!Thread_IsTerminating())
				{
					if(m_Socket)
					{
						fd_set ReadSet;
						FD_SET(m_Socket, &ReadSet);
						fd_set WriteSet;
						FD_SET(m_Socket, &WriteSet);
						timeval timeout;
						timeout.tv_sec = 0;
						timeout.tv_usec = 100000;
						socketselect(1, &ReadSet, &WriteSet, NULL, &timeout);
						{
							DIdsLockTyped(NThread::CMutual, m_Lock);

							uint32 LastState = m_State;
							if(FD_ISSET(m_Socket, &ReadSet))
							{
								sockaddr_in AddrIn;
								int Len = sizeof(AddrIn);
								int Error;
								if(m_bConnectionSocket && (Error = recv(m_Socket, NULL, 0, 0)) <= 0)
								{
									int NetErrno = sys_net_errno;
									if(Error == 0 || NetErrno == SYS_NET_ECONNABORTED)
										m_State |= NNet::ENetTCPState_Closed;
								}

								if(m_bListenSocket)
									m_State |= NNet::ENetTCPState_Connection;
								else
									m_State |= NNet::ENetTCPState_Read;
							}
							if(FD_ISSET(m_Socket, &WriteSet))
							{
								m_State |= NNet::ENetTCPState_Write;
							}
							if(m_State != LastState)
							{
								if(m_pReportTo)
									m_pReportTo->Signal();
							}
						}
					}
					else
						MRTC_SystemInfo::OS_Sleep(50);
				}
				return 0;
			}

			class CAVLCompare_CTCPSocket
			{
			public:

				static aint Compare(const CTCPSocket *_pFirst, const CTCPSocket *_pSecond, void *_pContext)
				{
					return (_pFirst->m_Socket) - (_pSecond->m_Socket);
				}

				static aint Compare(const CTCPSocket *_pTest, const void *_Key, void *_pContext)
				{
					return _pTest->m_Socket - (int)(mint)_Key;
				}
			};

			int m_Socket;
			DIdsTreeAVLAligned_Link(CTCPSocket, m_TreeLink, void *, CAVLCompare_CTCPSocket);

			NThread::CEventAutoResetReportableAggregate *m_pReportTo;

			uint32 m_State;
			uint32 m_bListenSocket:1;
			uint32 m_bConnectionSocket:1;
		};

		DIdsTreeAVLAligned_Tree(CTCPSocket, m_TreeLink, void *, CTCPSocket::CAVLCompare_CTCPSocket) m_SocketTree;
		typedef DIdsTreeAVLAligned_Iterator(CTCPSocket, m_TreeLink, void *, CTCPSocket::CAVLCompare_CTCPSocket) CSocketIter;
		NThread::CMutual m_Lock;

		void f_CheckFailed()
		{
			if (m_bInitFailed)
				Error_static(__FUNCTION__, "Initialization of network has failed, cannot use net");
		}

		bint f_ResolveAddres(const ch8 *_pAddress, NNet::CNetAddressIPv4 &_Address)
		{
			f_CheckFailed();
			uint32 Address = inet_addr(_pAddress);
			if(Address == ~0)
				return false;
			_Address.m_IP[0] = (uint8)(Address & 0xff);
			_Address.m_IP[1] = (uint8)((Address >> 8) & 0xff);
			_Address.m_IP[2] = (uint8)((Address >> 16) & 0xff);
			_Address.m_IP[3] = (uint8)((Address >> 24) & 0xff);
			return true;
		}

		CTCPSocket* f_Connect(const NNet::CNetAddressTCPv4 &_Address, NThread::CEventAutoResetReportableAggregate *_pReportTo)
		{
			f_CheckFailed();

			int sock = socket(AF_INET, SOCK_STREAM, 0);
			if(sock < 0)
				return NULL;

			sockaddr_in Address;
			Address.sin_family = AF_INET;
			Address.sin_addr.s_addr = (_Address.m_IP[0]) | (_Address.m_IP[1] << 8) | (_Address.m_IP[2] << 16) | (_Address.m_IP[3] << 24);
			Address.sin_port = _Address.m_Port;
			if(connect(sock, (sockaddr*)&Address, sizeof(Address)) < 0)
			{
				socketclose(sock);
				return NULL;
			}

			int Buf = 512 * 1024;
			if(setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (const char*)&Buf, sizeof(Buf)) < 0)
			{
				socketclose(sock);
				return NULL;
			}

			if(setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (const char*)&Buf, sizeof(Buf)) < 0)
			{
				socketclose(sock);
				return NULL;
			}

			int bNoDelay = true;
			if(setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&bNoDelay, sizeof(bNoDelay)) < 0)
			{
				socketclose(sock);
				return NULL;
			}

			CTCPSocket* pReturn = DNew(CTCPSocket) CTCPSocket;
			pReturn->m_Socket = sock;
			pReturn->m_pReportTo = _pReportTo;
			{
				DIdsLockTyped(NThread::CMutual, m_Lock);
				m_SocketTree.f_Insert(pReturn);
			}

			return pReturn;
		}

		CTCPSocket* f_Bind(const NNet::CNetAddressUDPv4 &_Address, NThread::CEventAutoResetReportableAggregate *_pReportTo)
		{
			f_CheckFailed();

			int sock = socket(AF_INET, SOCK_STREAM, 0);
			if(sock < 0)
				return NULL;

			sockaddr_in Address;
			Address.sin_family = AF_INET;
			Address.sin_addr.s_addr = _Address.m_IP[0] | (_Address.m_IP[1] << 8) | (_Address.m_IP[2] << 16) | (_Address.m_IP[3] << 24);
			Address.sin_port = _Address.m_Port;

			if(bind(sock, (sockaddr*)&Address, sizeof(Address)) < 0)
			{
				socketclose(sock);
				return NULL;
			}

			int Buf = 4*1024;
			if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&Buf, sizeof(Buf)))
			{
				socketclose(sock);
				return NULL;
			}

			if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&Buf, sizeof(Buf)))
			{
				socketclose(sock);
				return NULL;
			}

			if (_Address.m_bBroadcast)
			{
				int Broadcast = 1;
				if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*) &Broadcast, sizeof(Broadcast)))
				{
					socketclose(sock);
					return NULL;
				}
			}

			CTCPSocket *pReturn = DNew(CTCPSocket) CTCPSocket;

			pReturn->m_pReportTo = _pReportTo;
			pReturn->m_Socket = sock;
			{
				DIdsLockTyped(NThread::CMutual, m_Lock);
				m_SocketTree.f_Insert(pReturn);
			}

			return pReturn;
		}

		CTCPSocket* f_Listen(const NNet::CNetAddressTCPv4 &_Address, NThread::CEventAutoResetReportableAggregate *_pReportTo)
		{
			f_CheckFailed();

			int sock = socket(AF_INET, SOCK_STREAM, 0);
			if(sock < 0)
				return NULL;

			sockaddr_in Address;
			Address.sin_family = AF_INET;
			Address.sin_addr.s_addr = _Address.m_IP[0] | (_Address.m_IP[1] << 8) | (_Address.m_IP[2] << 16) | (_Address.m_IP[3] << 24);
			Address.sin_port = _Address.m_Port;

			if(bind(sock, (sockaddr*)&Address, sizeof(Address)) < 0)
			{
				socketclose(sock);
				return NULL;
			}

			if(listen(sock, 32) < 0)
			{
				socketclose(sock);
				return NULL;
			}

			CTCPSocket *pReturn = DNew(CTCPSocket) CTCPSocket;

			pReturn->m_pReportTo = _pReportTo;
			pReturn->m_Socket = sock;
			{
				DIdsLockTyped(NThread::CMutual, m_Lock);
				m_SocketTree.f_Insert(pReturn);
			}

			return pReturn;
		}

		CTCPSocket* f_Accept(CTCPSocket *_pSocket, NThread::CEventAutoResetReportableAggregate *_pReportTo)
		{
			f_CheckFailed();

			int sock = accept(_pSocket->m_Socket, NULL, 0);
			if(sock < 0)
				return NULL;


			int Buf = 512*1024;
			if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&Buf, sizeof(Buf)))
			{
				socketclose(sock);
				return NULL;
			}

			if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&Buf, sizeof(Buf)))
			{
				socketclose(sock);
				return NULL;
			}

			int NoDelay = true;

			if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&NoDelay, sizeof(NoDelay)))
			{
				socketclose(sock);
				return NULL;
			}

			CTCPSocket *pReturn = DNew(CTCPSocket) CTCPSocket;

			pReturn->m_pReportTo = _pReportTo;
			pReturn->m_Socket = sock;
			{
				DIdsLockTyped(NThread::CMutual, m_Lock);
				m_SocketTree.f_Insert(pReturn);
			}

			return pReturn;
		}

		void f_SetReportTo(CTCPSocket *_pSocket, NThread::CEventAutoResetReportableAggregate *_pReportTo)
		{
			{
				DIdsLockTyped(NThread::CMutual, _pSocket->m_Lock);
				_pSocket->m_pReportTo = _pReportTo;

				if(_pSocket->m_pReportTo)
					_pSocket->m_pReportTo->Signal();
			}
		}

		void* f_InheritHandle(CTCPSocket *_pSocket, NThread::CEventAutoResetReportableAggregate *_pReportTo)
		{
			CTCPSocket *pReturn = DNew(CTCPSocket) CTCPSocket;

			pReturn->m_pReportTo = _pReportTo;
			pReturn->m_Socket = _pSocket->m_Socket;
			pReturn->m_State = _pSocket->m_State;

			{
				DIdsLockTyped(NThread::CMutual, _pSocket->m_Lock);
				_pSocket->m_State |= DBit(31);
			}

			{
				DIdsLockTyped(NThread::CMutual, m_Lock);
				m_SocketTree.f_Insert(pReturn);
			}
			if (pReturn->m_pReportTo)
				pReturn->m_pReportTo->Signal();

			return pReturn;
		}

		bint f_Close(CTCPSocket *_pSocket)
		{
			{
				DIdsLockTyped(NThread::CMutual, m_Lock);
				m_SocketTree.f_Remove(_pSocket);
			}

			// Make sure that the report thread isn't using our socket
			{
				DIdsLockTyped(NThread::CMutual, _pSocket->m_Lock);
			}

			delete _pSocket;

			return m_SocketTree.IsEmpty();
		}

		uint32 f_GetState(CTCPSocket *_pSocket)
		{
			DIdsLockTyped(NThread::CMutual, _pSocket->m_Lock);
			uint32 State = _pSocket->m_State & DBitRange(0, 30);
			_pSocket->m_State &= ~DBitRange(0, 30);
			return State;
		}

		int f_Receive(CTCPSocket *_pSocket, void *_pData, int _DataLen)
		{
			int Ret = recv(_pSocket->m_Socket, (char *)_pData, _DataLen, 0);

			if (Ret < 0)
			{
				int Error = sys_net_errno;
				if (Error != SYS_NET_EWOULDBLOCK)
				{
					return 0;
					//Error_static(__FUNCTION__,"Could not revc from socket");
				}
				else
					return 0;
			}

			return Ret;
		}

		int f_Send(CTCPSocket *_pSocket, const void *_pData, int _DataLen)
		{
			int Ret = send(_pSocket->m_Socket, (const char *)_pData, _DataLen, 0);

			if (Ret < 0)
			{
				int Error = sys_net_errno;
				if (Error != SYS_NET_EWOULDBLOCK)
				{
					return 0;
					//Error_static(__FUNCTION__,"Could not sendfrom socket");
				}
				else
					return 0;
			}

			return Ret;
		}

		int f_Receive(CTCPSocket *_pSocket, NNet::CNetAddressUDPv4 &_Address, void *_pData, int _DataLen)
		{
			sockaddr_in Address;
			Address.sin_family = AF_INET;

			socklen_t Size = sizeof(Address);
			int Ret = recvfrom(_pSocket->m_Socket, (char *)_pData, _DataLen, 0, (sockaddr *)&Address, &Size);

			if (Ret < 0)
			{
				int Error = sys_net_errno;
				if (Error != SYS_NET_EWOULDBLOCK)
				{
					return 0;
//					Error_static(__FUNCTION__,"Could not revc from socket");
				}
				else
					return 0;
			}

			_Address.m_IP[0] = (uint8)(Address.sin_addr.s_addr & 0xff);
			_Address.m_IP[1] = (uint8)((Address.sin_addr.s_addr >> 8) & 0xff);
			_Address.m_IP[2] = (uint8)((Address.sin_addr.s_addr >> 16) & 0xff);
			_Address.m_IP[3] = (uint8)((Address.sin_addr.s_addr >> 24) & 0xff);
			_Address.m_Port = Address.sin_port;

			return Ret;
		}

		int f_Send(CTCPSocket *_pSocket, const NNet::CNetAddressUDPv4 &_Address, const void *_pData, int _DataLen)
		{
			sockaddr_in Address;
			Address.sin_family = AF_INET;
			Address.sin_addr.s_addr = _Address.m_IP[0] | (_Address.m_IP[1] << 8) | (_Address.m_IP[2] << 16) | (_Address.m_IP[3] << 24);
			Address.sin_port = _Address.m_Port;

			int Ret = sendto(_pSocket->m_Socket, (const char *)_pData, _DataLen, 0, (sockaddr *)&Address, sizeof(Address));

			if (Ret < 0)
			{
				int Error = sys_net_errno;
				if (Error != SYS_NET_EWOULDBLOCK)
				{
					return 0;
//					Error_static(__FUNCTION__,"Could not sendfrom socket");
				}
				else
					return 0;
			}

			return Ret;
		}
	};

	CTCPContext* m_pTCPContext;

	CTCPContext* GetTCPContext()
	{
		if(!m_pTCPContext)
		{
			m_pTCPContext = DNew(CTCPContext) CTCPContext();
		}

		return m_pTCPContext;
	}

	void DestroyTCPContext()
	{
		if(m_pTCPContext)
		{
			delete m_pTCPContext;
			m_pTCPContext = NULL;
		}
	}
};

MRTC_SystemInfo::MRTC_SystemInfo()
{
#ifdef M_STATIC
	MRTC_ObjectManager::m_pSystemInfo = this;
#endif
	m_nCPU = 2;
	m_CPUFrequencyu = 1;
	m_CPUFrequencyfp = 1;
	m_CPUFrequencyRecp = 1;
	m_CPUFeatures = 0;
	m_CPUFeaturesEnabled = 0xffffffff;
	m_CPUName[0] = 0;
	m_CPUNameWithFeatures[0] = 0;

	static mint SysInfoData[(sizeof(MRTC_SystemInfoInternal) + sizeof(mint) - 1) / sizeof(mint)];

	m_pInternalData = new(SysInfoData) MRTC_SystemInfoInternal;

	m_pThreadContext = NULL;

	CPU_MeasureFrequency();
}

MRTC_SystemInfo::~MRTC_SystemInfo()
{
	m_pInternalData->~MRTC_SystemInfoInternal();

	if (m_pThreadContext)
		m_pThreadContext->~MRTC_ThreadContext();
}

void MRTC_SystemInfo::PostCreate()
{
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Clock
|__________________________________________________________________________________________________
\*************************************************************************************************/

void MRTC_SystemInfo::CPU_CreateNames()
{
	CFStr CPUName("CELL PowerPC");
	CFStr CPUNameFeatures = CPUName;

	CPUNameFeatures += CFStrF(", %.0f Mhz", sys_time_get_timebase_frequency() / 1000000.0);

	strcpy(m_CPUName, CPUName.Str());
	strcpy(m_CPUNameWithFeatures, CPUNameFeatures.Str());
}

void MRTC_SystemInfo::CPU_MeasureFrequency()
{
	m_CPUFrequencyu = sys_time_get_timebase_frequency();
	m_CPUFrequencyfp = m_CPUFrequencyu;
	m_CPUFrequencyRecp = 1.0f / m_CPUFrequencyfp;
	printf("PS3 CPU Frequency: %u\n", (uint32)m_CPUFrequencyu);
}

int64 MRTC_SystemInfo::CPU_Clock()
{
	int64 result;
	asm volatile("mftb %0": [result] "=r"(result));
	return result;
}

uint64 MRTC_SystemInfo::CPU_ClockFrequencyInt() const
{
	return m_CPUFrequencyu;
}

fp4 MRTC_SystemInfo::CPU_ClockFrequencyFloat() const
{
	return m_CPUFrequencyfp;
}

fp4 MRTC_SystemInfo::CPU_ClockFrequencyRecp() const
{
	return m_CPUFrequencyRecp;
}

fp4 MRTC_SystemInfo::OS_ClockFrequencyRecp() const
{
	return m_CPUFrequencyRecp;
}

uint64 MRTC_SystemInfo::OS_ClockFrequencyInt() const
{
	return m_CPUFrequencyu;
}

fp4 MRTC_SystemInfo::OS_ClockFrequencyFloat() const
{
	return m_CPUFrequencyfp;
}

int64 MRTC_SystemInfo::OS_Clock()
{
	int64 result;
	asm volatile("mftb %0": [result] "=r"(result));
	return result;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Memorymanagement
|__________________________________________________________________________________________________
\*************************************************************************************************/

void* MRTC_SystemInfo::OS_HeapAlloc(uint32 _Size)
{
	return malloc(_Size);
}

void* MRTC_SystemInfo::OS_HeapAllocAlign(uint32 _Size, uint32 _Align)
{
	return memalign(_Align, _Size);
}

void* MRTC_SystemInfo::OS_HeapRealloc(void *_pMem, uint32 _Size)
{
	return realloc(_pMem, _Size);
}

void MRTC_SystemInfo::OS_HeapFree(void *_pMem)
{
	return free(_pMem);
}

uint32 MRTC_SystemInfo::OS_HeapSize(const void *_pMem)
{
	DebugBreak(); // implement this (with dlmalloc?)
//	return _msize(_pMem);
	return 0;
}

void* MRTC_SystemInfo::OS_Alloc(uint32 _Size, bool _bCommit)
{
	sys_addr_t MemoryAddr;
	_Size = (_Size + 65535) & ~65535;
	
	if (sys_memory_allocate(_Size, SYS_MEMORY_PAGE_SIZE_64K, &MemoryAddr) != CELL_OK)
		return NULL;

	return (void *)MemoryAddr;
}

void MRTC_SystemInfo::OS_Free(void *_pMem)
{
	sys_memory_free((sys_addr_t)_pMem);
}

#include <sys/memory.h>
void* MRTC_SystemInfo::OS_AllocGPU(uint32 _Size, bool _bCached)
{
	sys_addr_t MemoryAddr;
	_Size = (_Size + 65535) & ~65535;
	if (sys_memory_allocate(_Size, SYS_MEMORY_PAGE_SIZE_64K, &MemoryAddr) != CELL_OK)
		return NULL;

	return (void *)MemoryAddr;
}

void MRTC_SystemInfo::OS_FreeGPU(void *_pMem)
{
	sys_memory_free((sys_addr_t)_pMem);
}

bool MRTC_SystemInfo::OS_Commit(void *_pMem, uint32 _Size, bool _bCommited)
{
	DebugBreak();
	return false;
}

uint32 MRTC_SystemInfo::OS_CommitGranularity()
{
	return 64 * 1024;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Atomic
|__________________________________________________________________________________________________
\*************************************************************************************************/

int32 MRTC_SystemInfo::Atomic_Increase(volatile int32 *_pDest)
{
	int32 result, temp;
	asm volatile(
	".Latomic_inc_loop%=:\n"
		"lwarx %0, 0, %2\n"
		"addi %1, %0, 1\n"
		"stwcx. %1, 0, %2\n"
		"bne-	.Latomic_inc_loop%=\n"
		: "=&r"(result), "=&r"(temp)
		: "r"(_pDest)
		: "memory"
		);
	return result;
}

int32 MRTC_SystemInfo::Atomic_Decrease(volatile int32 *_pDest)
{
	int32 result, temp;
	asm volatile(
	".Latomic_dec_loop%=:\n"
		"lwarx %0, 0, %2\n"
		"subi %1, %0, 1\n"
		"stwcx. %1, 0, %2\n"
		"bne-	.Latomic_dec_loop%=\n"
		: "=&r"(result), "=&r"(temp)
		: "r"(_pDest)
		: "memory"
		);
	return result;
}

int32 MRTC_SystemInfo::Atomic_Add(volatile int32 *_pDest, int32 _Add)
{
	int32 result, temp;
	asm volatile(
	".Latomic_add_loop%=:\n"
		"lwarx %0, 0, %2\n"
		"add %1, %0, %2\n"
		"stwcx. %1, 0, %2\n"
		"bne-	.Latomic_add_loop%=\n"
		: "=&r"(result), "=&r"(temp)
		: "r"(_pDest)
		: "memory"
		);
	return result;
}

int32 MRTC_SystemInfo::Atomic_IfEqualExchange(volatile int32 *_pDest, int32 _CompareTo, int32 _SetTo)
{
	int32 result;
	asm volatile(
	".Latomic_iex_loop%=:\n"
		"lwarx %0, 0, %1\n"
		"cmpw %0, %2\n"
		"bne- .Latomic_iex_done%=\n"
		"stwcx. %3, 0, %1\n"
		"bne- .Latomic_iex_loop%=\n"
	".Latomic_iex_done%=:\n"
		: "=&r"(result)
		: "r"(_pDest), "r"(_CompareTo), "r"(_SetTo)
		: "memory"
		);
	
	return result;
}

int32 MRTC_SystemInfo::Atomic_Exchange(volatile int32 *_pDest, int32 _SetTo)
{
	int32 result;
	asm volatile(
	".Latomic_xch_loop%=:\n"
		"lwarx %0, 0, %1\n"
		"stwcx. %2, 0, %1\n"
		"bne-	.Latomic_xch_loop%=\n"
		: "=&r"(result)
		: "r"(_pDest), "r"(_SetTo)
		: "memory"
		);
	return result;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Thread
|__________________________________________________________________________________________________
\*************************************************************************************************/
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <pthread.h>

void* MRTC_SystemInfo::OS_GetThreadID()
{
	sys_ppu_thread_t Thread;
	sys_ppu_thread_get_id(&Thread);
	return (void *)(aint)Thread;
}


void* MRTC_SystemInfo::OS_GetProcessID()
{
	return (void *)(aint)sys_process_getpid();
}


class CPS3Thread
{
public:
	pthread_t m_ThreadID;
	uint32(M_STDCALL*m_pfnEntryPoint)(void*);
	void *m_pContext;

	CPS3Thread()
	{
		m_ThreadID = 0;
	}
	~CPS3Thread()
	{
		if (m_ThreadID)
		{
			pthread_detach(m_ThreadID);
		}
	}

	static int MCCPrioToPS3Prio(int _Priority)
	{
		if (_Priority < MRTC_THREAD_PRIO_LOW)
			return 1536;
		else if (_Priority < MRTC_THREAD_PRIO_BELOWNORMAL)
			return 1152;
		else if (_Priority < MRTC_THREAD_PRIO_NORMAL)
			return 1001;
		else if (_Priority < MRTC_THREAD_PRIO_ABOVENORMAL)
			return 901;
		else if (_Priority < MRTC_THREAD_PRIO_HIGHEST)
			return 850;
		else if (_Priority < MRTC_THREAD_PRIO_TIMECRITICAL)
			return 699;
		else
			return 0;
	}

	static void * fs_ThreadProc(void * _Context)
	{
		CPS3Thread *pContext = (CPS3Thread *)_Context;

		uint32 ExitCode = pContext->m_pfnEntryPoint(pContext->m_pContext);
		return (void *)(aint)ExitCode;
	}
};

void* MRTC_SystemInfo::OS_ThreadCreate(uint32(M_STDCALL*_pfnEntryPoint)(void*), int _StackSize, void* _pContext, int _ThreadPriority)
{
	CPS3Thread *pThread = DNew(CPS3Thread) CPS3Thread;
	pThread->m_pContext = _pContext;
	pThread->m_pfnEntryPoint = _pfnEntryPoint;

	pthread_attr_t PThreadAttr;
	pthread_attr_init(&PThreadAttr);
	pthread_attr_setstacksize(&PThreadAttr, _StackSize);
	sched_param Param;
	Param.sched_priority = CPS3Thread::MCCPrioToPS3Prio(_ThreadPriority);
	pthread_attr_setschedparam(&PThreadAttr, &Param);
//	PThreadAttr.
	int Ret = pthread_create(&pThread->m_ThreadID, &PThreadAttr, CPS3Thread::fs_ThreadProc, pThread);
	if (Ret == 0)
	{
		pthread_attr_destroy(&PThreadAttr);
		return pThread;
	}

	pthread_attr_destroy(&PThreadAttr);
	delete pThread;
	return NULL;
}

int MRTC_SystemInfo::OS_ThreadDestroy(void* _hThread)
{
	CPS3Thread *pThread = (CPS3Thread *)_hThread;
	void* ExitCode = 0;
	pthread_join(pThread->m_ThreadID, &ExitCode);
	delete pThread;
	return (int)(aint)ExitCode;
}

void MRTC_SystemInfo::OS_ThreadExit(int _ExitCode)
{
	pthread_exit((void *)(aint)_ExitCode);
}

int MRTC_SystemInfo::OS_ThreadGetExitCode(void* _hThread)
{
	// Not supported and not used
	DebugBreak();
	return 0;
}

bool MRTC_SystemInfo::OS_ThreadIsRunning(void* _hThread)
{
	if (_hThread)
		return true;
	return false;
}

void MRTC_SystemInfo::OS_ThreadTerminate(void* _hThread, int _ExitCode)
{
	// Not supported
	DebugBreak();
}

#include <pthread.h>

class CThreadLocalStoreList
{
public:
	class CThreadLocalStoreThread
	{
	public:
		CThreadLocalStoreThread(void* _pThread) : m_pNext(0), m_pThreadID(_pThread){memset(m_Data, 0, sizeof(uint64) * 64);}
		CThreadLocalStoreThread* m_pNext;
		void* m_pThreadID;
		uint64 m_Data[64];
	};

	aint Alloc()
	{
		if(m_pLock == 0)
		{
			static uint8 LockData[sizeof(NThread::CMutualAggregate)];
			m_pLock = (NThread::CMutualAggregate*)LockData;
			m_pLock->Construct();
		}

		M_LOCK(*m_pLock);
		for(uint64 i = 0; i < 64; i++)
		{
			uint64 Mask = 1 << i;
			if(!(m_UsedIndices & Mask))
			{
				m_UsedIndices |= Mask;
				return i;
			}
		}

		MRTC_SystemInfo::OS_Assert("Out of thread local storage");
		return ~0;
	}

	void Free(uint64 _Index)
	{
		M_LOCK(*m_pLock);
		uint64 Mask = 1 << _Index;
		if(!(m_UsedIndices & Mask))
		{
			MRTC_SystemInfo::OS_Assert("Freeing non-allocate TLS");
		}
		m_UsedIndices = m_UsedIndices & ~Mask;
	}

	mint GetValue(uint64 _Index)
	{
		M_LOCK(*m_pLock);
		void* pThreadID = MRTC_SystemInfo::OS_GetThreadID();
		CThreadLocalStoreThread* pThread = GetStore(pThreadID);
		return pThread->m_Data[_Index];
	}

	void SetValue(uint64 _Index, mint _Value)
	{
		M_LOCK(*m_pLock);
		void* pThreadID = MRTC_SystemInfo::OS_GetThreadID();
		CThreadLocalStoreThread* pThread = GetStore(pThreadID);
		pThread->m_Data[_Index]	= _Value;
	}

private:
	CThreadLocalStoreThread* GetStore(void* _pThread)
	{
		for(CThreadLocalStoreThread* pT = m_pThreadList; pT; pT = pT->m_pNext)
		{
			if(pT->m_pThreadID == _pThread)
				return pT;
		}

		CThreadLocalStoreThread* pThread = DNew(CThreadLocalStoreThread) CThreadLocalStoreThread(_pThread);
		pThread->m_pNext = m_pThreadList;
		m_pThreadList = pThread;

		return pThread;
	}

	static CThreadLocalStoreThread* m_pThreadList;
	static NThread::CMutualAggregate* m_pLock;
	static uint64 m_UsedIndices;
//	uint8 m_LockData[sizeof(NThread::CMutualAggregate)];
//	NThread::CMutualAggregate m_Lock;
};


CThreadLocalStoreList::CThreadLocalStoreThread* CThreadLocalStoreList::m_pThreadList = 0;
NThread::CMutualAggregate* CThreadLocalStoreList::m_pLock = 0;
uint64 CThreadLocalStoreList::m_UsedIndices = 0;
CThreadLocalStoreList g_TLS;
aint MRTC_SystemInfo::Thread_LocalAlloc()
{
	return g_TLS.Alloc();
/*
	pthread_key_t key;
	int Error = pthread_key_create(&key, NULL);
#ifndef M_RTM
	switch(Error)
	{
	case 0:
		break;
	case EAGAIN: OS_Assert("Out of resources"); break;
	case ENOMEM: OS_Assert("Out of memory"); break;
	default: OS_Assert("Unknown error occured"); break;
	}
#endif
	return key;
*/
}

void MRTC_SystemInfo::Thread_LocalFree(aint _Index)
{
	g_TLS.Free(_Index);

//	pthread_key_delete((pthread_key_t)_Index);
}

mint MRTC_SystemInfo::Thread_LocalGetValue(aint _Index)
{
	return g_TLS.GetValue(_Index);
//	return (mint)pthread_getspecific(_Index);
}

void MRTC_SystemInfo::Thread_LocalSetValue(aint _Index, mint _Value)
{
	g_TLS.SetValue(_Index, _Value);
//	pthread_setspecific(_Index, (void*)_Value);
}

void MRTC_SystemInfo::Thread_SetName(const char *_pName)
{
}

void MRTC_SystemInfo::Thread_SetProcessor(uint32 _Processor)
{
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Mutex
|__________________________________________________________________________________________________
\*************************************************************************************************/

// No support for named muteces
#include <sys/synchronization.h>
void MRTC_SystemInfo::OS_MutexClose(void* _pMutex)
{
	sys_mutex_destroy((sys_mutex_t)(aint)_pMutex);
}

void MRTC_SystemInfo::OS_MutexLock(void* _pMutex)
{
	auint Error = sys_mutex_lock((sys_mutex_t)(aint)_pMutex, 0);

#ifndef M_RTM
	switch(Error)
	{
	case CELL_OK: break;
	default: OS_Assert("Unknown error occured");
	case EDEADLK: OS_Assert("Mutex deadlock occured, trying to lock already owned (non-recursive) mytex.");
	case ESRCH: OS_Assert("Trying to lock invalid mutex");
	}
#endif
}

void* MRTC_SystemInfo::OS_MutexOpen(const char* _pName)
{
	sys_mutex_t mutex;
	sys_mutex_attribute_t mutexattrib;
	sys_mutex_attribute_initialize(mutexattrib);
	mutexattrib.attr_protocol = SYS_SYNC_PRIORITY;
	mutexattrib.attr_recursive = SYS_SYNC_RECURSIVE;
	int Error = sys_mutex_create(&mutex, &mutexattrib);
	M_ASSERT(Error == CELL_OK, "!");
	return (void*)(aint)mutex;
}

bool MRTC_SystemInfo::OS_MutexTryLock(void* _pMutex)
{
	auint Error = sys_mutex_trylock((sys_mutex_t)(aint)_pMutex);
	if(Error == CELL_OK)
		return true;

#ifndef M_RTM
	switch(Error)
	{
	case CELL_OK: break;
	default: OS_Assert("Unknown error occured");
	case EDEADLK: OS_Assert("Mutex deadlock occured, trying to lock already owned (non-recursive) mytex.");
	case ESRCH: OS_Assert("Trying to lock invalid mutex");
	}
#endif
	return false;
}

void MRTC_SystemInfo::OS_MutexUnlock(void* _pMutex)
{
	sys_mutex_unlock((sys_mutex_t)(aint)_pMutex);
}

class CPS3MutexLocker
{
protected:
	sys_mutex_t* m_pMutex;
public:
	CPS3MutexLocker(sys_mutex_t* _pMutex) : m_pMutex(_pMutex)
	{
		int Error = sys_mutex_lock(*m_pMutex, 0);
#ifndef M_RTM
		if(Error != CELL_OK)
			MRTC_SystemInfo::OS_Assert("CPS3MutexLocker::CPS3MutexLocker - Error occured while trying to acquire mutex");
#endif
	}

	~CPS3MutexLocker()
	{
		int Error = sys_mutex_unlock(*m_pMutex);
#ifndef M_RTM
		if(Error != CELL_OK)
			MRTC_SystemInfo::OS_Assert("CPS3MutexLocker::~CPS3MutexLocker - Error occured while trying to release mutex");
#endif
	}
};

#define DLocker(obj) CPS3MutexLocker aa##__LINE__(&obj->m_Mutex);


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Semaphore
|__________________________________________________________________________________________________
\*************************************************************************************************/


void* MRTC_SystemInfo::Semaphore_Alloc(mint _InitialCount, mint _MaximumCount)
{
//	sys_sem_t* pSem = DNew(sys_sem_t) sys_sem_t;
	sys_sem_t* pSem = (sys_sem_t*)MRTC_SystemInfo::MRTC_GetSystemInfo().m_pInternalData->m_PrimitivePool.New();

	sys_mutex_attribute_t mutex_attr;
	sys_mutex_attribute_initialize(mutex_attr);
	mutex_attr.attr_protocol = SYS_SYNC_PRIORITY;
	mutex_attr.attr_recursive = SYS_SYNC_RECURSIVE;
	int Error = sys_mutex_create(&pSem->m_Mutex, &mutex_attr);
	if (Error != CELL_OK)
	{
		OS_Assert("sys_mutex_create");
		return NULL;
	}
	
	/* Create the condition variable */
	sys_cond_attribute_t cond_attr;
	sys_cond_attribute_initialize(cond_attr);
	Error = sys_cond_create(&pSem->m_Condition, pSem->m_Mutex, &cond_attr);
	if (Error != CELL_OK)
	{
		OS_Assert("sys_cond_create");
		return NULL;
	}
	
	pSem->m_CurrentCount = _InitialCount;
	pSem->m_MaxCount = _MaximumCount;
	
	return pSem;
}

void MRTC_SystemInfo::Semaphore_Free(void *_pSemaphore)
{
	sys_sem_t* pSem = (sys_sem_t*)_pSemaphore;
	sys_cond_destroy(pSem->m_Condition);
	sys_mutex_destroy(pSem->m_Mutex);
	MRTC_SystemInfo::MRTC_GetSystemInfo().m_pInternalData->m_PrimitivePool.Delete((UPrimitive *)pSem);
}

void MRTC_SystemInfo::Semaphore_Increase(void * _pSemaphore, mint _Count)
{
	sys_sem_t* pSem = (sys_sem_t*)_pSemaphore;
	DLocker(pSem);

	pSem->m_CurrentCount = Min(pSem->m_CurrentCount + 1, pSem->m_MaxCount);

	int Error = sys_cond_signal(pSem->m_Condition);
#ifndef M_RTM
	if(Error != CELL_OK)
		OS_Assert("Error occured while fiddling with semaphore");
#endif
}

bint MRTC_SystemInfo::Semaphore_TryWait(void * _pSemaphore)
{
	sys_sem_t* pSem = (sys_sem_t*)_pSemaphore;
	DLocker(pSem);

	bint Return = false;
	if(pSem->m_CurrentCount > 0)
	{
		pSem->m_CurrentCount--;
		Return = true;
	}

	return Return;
}

void MRTC_SystemInfo::Semaphore_Wait(void * _pSemaphore)
{
	sys_sem_t* pSem = (sys_sem_t*)_pSemaphore;
	DLocker(pSem);

	while(pSem->m_CurrentCount <= 0)
	{
		int Error = sys_cond_wait(pSem->m_Condition, 0);
#ifndef M_RTM
		if(Error != CELL_OK)
			OS_Assert("Error occured while fiddling with semaphore");
#endif
	}
	pSem->m_CurrentCount--;
}

bint MRTC_SystemInfo::Semaphore_WaitTimeout(void * _pSemaphore, fp8 _Timeout)
{
	sys_sem_t* pSem = (sys_sem_t*)_pSemaphore;
	DLocker(pSem);

	bool Return = true;
	if(pSem->m_CurrentCount <= 0)
	{
		int Error = sys_cond_wait(pSem->m_Condition, (aint)(_Timeout * 1000000.0));
		if(Error == CELL_OK)
		{
			pSem->m_CurrentCount--;
		}
		else if(Error == ETIMEDOUT)
		{
			Return = false;
		}
#ifndef M_RTM
		else
			OS_Assert("Error occured while fiddling with semaphore");
#endif
	}
	else
		pSem->m_CurrentCount--;

	return Return;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Event
|__________________________________________________________________________________________________
\*************************************************************************************************/

void* MRTC_SystemInfo::Event_Alloc(bint _InitialSignal, bint _bAutoReset)
{
//	sys_event_t* pEvent = DNew(sys_event_t) sys_event_t;
	sys_event_t* pEvent = (sys_event_t*)MRTC_SystemInfo::MRTC_GetSystemInfo().m_pInternalData->m_PrimitivePool.New();

	sys_mutex_attribute_t mutex_attr;
	sys_mutex_attribute_initialize(mutex_attr);
	mutex_attr.attr_protocol = SYS_SYNC_PRIORITY;
	mutex_attr.attr_recursive = SYS_SYNC_RECURSIVE;
	int Error = sys_mutex_create(&pEvent->m_Mutex, &mutex_attr);
	if (Error != CELL_OK)
	{
		OS_Assert("sys_mutex_create");
		return NULL;
	}
	
	/* Create the condition variable */
	sys_cond_attribute_t cond_attr;
	sys_cond_attribute_initialize(cond_attr);
	Error = sys_cond_create(&pEvent->m_Condition, pEvent->m_Mutex, &cond_attr);
	if (Error != CELL_OK)
	{
		OS_Assert("sys_cond_create");
		return NULL;
	}
	
	pEvent->m_Signaled = _InitialSignal;
	pEvent->m_AutoReset = _bAutoReset;
	
	return pEvent;
}

void MRTC_SystemInfo::Event_Free(void *_pEvent)
{
	sys_event_t* pEvent = (sys_event_t*)_pEvent;
	sys_cond_destroy(pEvent->m_Condition);
	sys_mutex_destroy(pEvent->m_Mutex);
	MRTC_SystemInfo::MRTC_GetSystemInfo().m_pInternalData->m_PrimitivePool.Delete((UPrimitive *)pEvent);
}

void MRTC_SystemInfo::Event_ResetSignaled(void * _pEvent)
{
	sys_event_t* pEvent = (sys_event_t*)_pEvent;
	DLocker(pEvent);
	pEvent->m_Signaled = 0;
}

void MRTC_SystemInfo::Event_SetSignaled(void * _pEvent)
{
	sys_event_t* pEvent = (sys_event_t*)_pEvent;
	DLocker(pEvent);
	pEvent->m_Signaled = 1;
	sys_cond_signal(pEvent->m_Condition);
}

bint MRTC_SystemInfo::Event_TryWait(void * _pEvent)
{
	sys_event_t* pEvent = (sys_event_t*)_pEvent;
	DLocker(pEvent);

	bint Return = false;
	if(pEvent->m_Signaled)
	{
		if(pEvent->m_AutoReset)
			pEvent->m_Signaled = 0;
		Return = true;
	}

	return Return;
}

void MRTC_SystemInfo::Event_Wait(void * _pEvent)
{
	sys_event_t* pEvent = (sys_event_t*)_pEvent;
	DLocker(pEvent);

	if(pEvent->m_Signaled == 0)
	{
		int Error = sys_cond_wait(pEvent->m_Condition, 0);
#ifndef M_RTM
		if(Error != CELL_OK)
			OS_Assert("MRTC_SystemInfo::Event_ResetSignaled - Error occured while trying to acquire mutex");
#endif
		// If event is not autoreset then signal all other threads waiting on it
		if(pEvent->m_AutoReset == 0)
			sys_cond_signal(pEvent->m_Condition);
	}
	
	if(pEvent->m_AutoReset)
		pEvent->m_Signaled = 0;
}

bint MRTC_SystemInfo::Event_WaitTimeout(void * _pEvent, fp8 _Timeout)
{
	sys_event_t* pEvent = (sys_event_t*)_pEvent;
	DLocker(pEvent);

	bint Return = true;
	if(pEvent->m_Signaled == 0)
	{
		int Error = sys_cond_wait(pEvent->m_Condition, (aint)(_Timeout * 1000000.0));
		if(Error == CELL_OK)
		{
			// If event is not autoreset then signal all other threads waiting on it
			if(pEvent->m_AutoReset == 0)
				sys_cond_signal(pEvent->m_Condition);
		}
		else if(Error == ETIMEDOUT)
		{
			Return = false;
		}
#ifndef M_RTM
		else
			OS_Assert("MRTC_SystemInfo::Event_ResetSignaled - Error occured while trying to acquire mutex");
#endif
	}
	
	if (Return)
	{
		if(pEvent->m_AutoReset)
			pEvent->m_Signaled = 0;

	}

	return Return;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Misc OS stuff
|__________________________________________________________________________________________________
\*************************************************************************************************/

void MRTC_SystemInfo::OS_Assert(const char* _pMsg, const char* _pFile, int _Line)
{
	OS_Trace("ERROR: Assert failed - %s (file: %s line: %d)", _pMsg, _pFile, _Line);
	DebugBreak();
}

#include <sys/timer.h>
void MRTC_SystemInfo::OS_Sleep(int _Milliseconds)
{
	sys_timer_usleep(_Milliseconds * 1000);
}

void MRTC_SystemInfo::OS_Trace(const char *_pStr, ...)
{
	char lBuffer[1024];

	if (_pStr)
	{
		va_list arg;
		va_start(arg, _pStr);
		CStrBase::vsnprintf((char*) &lBuffer[0], 1023-9, _pStr, arg); 
		lBuffer[1023] = 0;
	}
	else
		lBuffer[0] = 0;
/*
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
#ifndef MRTC_ENABLE_REMOTEDEBUGGER_STATIC
	if (MRTC_GetObjectManager() && MRTC_GetObjectManager()->GetRemoteDebugger())
#endif
		if (MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_Trace)
			MRTC_GetRD()->SendData(ERemoteDebug_Trace, lBuffer, strlen(lBuffer) + 1, false, false);
#endif
*/
	printf("%s", lBuffer);
}

void gf_ModuleAdd()
{
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| File
|__________________________________________________________________________________________________
\*************************************************************************************************/

#include <cell/fs/cell_fs_errno.h>
#include <cell/fs/cell_fs_file_api.h>

void PS3File_FixPath( char *_String )
{
	StrReplaceChar(_String, '\\', '/');

	int iInsert = 0;
	int iChar = 0;
	while(_String[iChar])
	{
		while((_String[iChar] == '/') && (_String[iChar + 1] == '/'))
		{
			iChar++;
		}

		if(iInsert != iChar)
			_String[iInsert] = _String[iChar];
		iInsert++;
		iChar++;
	}

	if((iInsert > 0) && (_String[iInsert - 1] == '/'))
		_String[iInsert - 1] = 0;
	else
		_String[iInsert] = 0;
}

struct CPS3AsyncHandle
{
	CPS3AsyncHandle()
	{
		m_pNext = NULL;
		m_Type = 0;
		m_Error = 0;
		m_bDone = false;
		m_fd = -1;
		m_pData = NULL;
		m_Offset = 0;
		m_DataSize = 0;
		m_ProcessedBytes = 0;
	}

	CPS3AsyncHandle* m_pNext;
	int m_Type;	// Read/Write
	int m_Error;

	bool m_bDone;
	int m_fd;
	void* m_pData;
	fint m_Offset;
	fint m_DataSize;
	fint m_ProcessedBytes;

	void CreateRead(int _fd, void* _pDest, fint _Pos, fint _Size)
	{
		m_Type = 1;
		m_fd = _fd;
		m_pData = _pDest;
		m_Offset = _Pos;
		m_DataSize = _Size;
	}

	void CreateWrite(int _fd, void* _pDest, fint _Pos, fint _Size)
	{
		m_Type = 2;
		m_fd = _fd;
		m_pData = _pDest;
		m_Offset = _Pos;
		m_DataSize = _Size;
	}
};

class CPS3AsyncFileIO : public MRTC_Thread_Core
{
protected:
	MRTC_CriticalSection m_Lock;
	CPS3AsyncHandle* m_pPendingTasksListHead;
	CPS3AsyncHandle* m_pPendingTasksListTail;
	NThread::CEventAutoReset m_AvailTasks;
public:
	CPS3AsyncFileIO()
	{
		m_pPendingTasksListHead = NULL;
		m_pPendingTasksListTail = NULL;
	}
	virtual int Thread_Main()
	{
		while(!Thread_IsTerminating())
		{
			m_AvailTasks.Wait();

			CPS3AsyncHandle* pActiveTaskList = NULL;
			if(m_pPendingTasksListHead)
			{
				M_LOCK(m_Lock);
				pActiveTaskList = m_pPendingTasksListHead;
				m_pPendingTasksListHead = NULL;
				m_pPendingTasksListTail = NULL;
			}
			while(pActiveTaskList)
			{
				CPS3AsyncHandle* pTask = pActiveTaskList;
				CPS3AsyncHandle* pNext = pTask->m_pNext;
				if(pTask->m_Type == 1)
				{
					uint64 FilePos;
					int Error = cellFsLseek(pTask->m_fd, pTask->m_Offset, CELL_FS_SEEK_SET, &FilePos);
					if(!Error)
					{
						uint64 ReadBytes;
						Error = cellFsRead(pTask->m_fd, pTask->m_pData, pTask->m_DataSize, &ReadBytes);
						if(!Error)
						{
							pTask->m_ProcessedBytes = ReadBytes;
						}
					}
					pTask->m_Error = Error;
				}
				else if(pTask->m_Type == 2)
				{
					uint64 FilePos;
					int Error = cellFsLseek(pTask->m_fd, pTask->m_Offset, CELL_FS_SEEK_SET, &FilePos);
					if(!Error)
					{
						uint64 WriteBytes;
						Error = cellFsWrite(pTask->m_fd, pTask->m_pData, pTask->m_DataSize, &WriteBytes);
						if(!Error)
						{
							pTask->m_ProcessedBytes = WriteBytes;
						}
					}
					pTask->m_Error = Error;
				}
				else
				{
					pTask->m_Error = -1;
				}
				pTask->m_bDone = true;
				M_EXPORTBARRIER;

				pActiveTaskList	= pNext;
			}
		}

		return 0;
	}

	void AddTask(CPS3AsyncHandle* _pTask)
	{
		if(!Thread_IsCreated())
			Thread_Create(NULL, 16384, MRTC_THREAD_PRIO_HIGHEST);
		M_LOCK(m_Lock);
		if(m_pPendingTasksListHead)
			m_pPendingTasksListTail->m_pNext = _pTask;
		else
			m_pPendingTasksListHead = _pTask;
		m_pPendingTasksListTail = _pTask;
		_pTask->m_pNext = NULL;

		m_AvailTasks.Signal();
	}
};

CPS3AsyncFileIO g_AsyncReader;

fint MRTC_SystemInfo::OS_FileAsyncBytesProcessed(void *_pAsyncInstance)
{
	CPS3AsyncHandle* pAsync = (CPS3AsyncHandle*)_pAsyncInstance;
	M_IMPORTBARRIER;
	return pAsync->m_ProcessedBytes;
}

void MRTC_SystemInfo::OS_FileAsyncClose(void *_pAsyncInstance)
{
	CPS3AsyncHandle* pAsync = (CPS3AsyncHandle*)_pAsyncInstance;
	delete pAsync;
}

bool MRTC_SystemInfo::OS_FileAsyncIsFinished(void *_pAsyncInstance)
{
	CPS3AsyncHandle* pAsync = (CPS3AsyncHandle*)_pAsyncInstance;
	M_IMPORTBARRIER;
	return pAsync->m_bDone;
}

//#define ASYNCIO

void* MRTC_SystemInfo::OS_FileAsyncRead(void *_pFile, void *_pData, fint _DataSize, fint _FileOffset)
{
	CPS3AsyncHandle* pAsync = DNew(CPS3AsyncHandle) CPS3AsyncHandle;
#ifdef ASYNCIO
	pAsync->CreateRead((int)((aint)_pFile), _pData, _FileOffset, _DataSize);
	g_AsyncReader.AddTask(pAsync);
#else
	pAsync->m_fd = (int)((aint)_pFile);
	pAsync->m_pData = _pData;
	pAsync->m_Offset = _FileOffset;
	pAsync->m_DataSize = _DataSize;
	uint64 FilePos;
	int Error = cellFsLseek(pAsync->m_fd, pAsync->m_Offset, CELL_FS_SEEK_SET, &FilePos);
	if(Error)
	{
		delete pAsync;
		return NULL;
	}
	uint64 ReadBytes;
	Error = cellFsRead(pAsync->m_fd, pAsync->m_pData, pAsync->m_DataSize, &ReadBytes);
	if(Error)
	{
		delete pAsync;
		return NULL;
	}
	pAsync->m_bDone = true;
	pAsync->m_ProcessedBytes = ReadBytes;
#endif

	return pAsync;
}

void* MRTC_SystemInfo::OS_FileAsyncWrite(void *_pFile, const void *_pData, fint _DataSize, fint _FileOffset)
{
	CPS3AsyncHandle* pAsync = DNew(CPS3AsyncHandle) CPS3AsyncHandle;
#ifdef ASYNCIO
	pAsync->CreateWrite((int)((aint)_pFile), const_cast<void*>(_pData), _FileOffset, _DataSize);
	g_AsyncReader.AddTask(pAsync);
#else
	pAsync->m_fd = (int)((aint)_pFile);
	pAsync->m_pData = const_cast<void*>(_pData);	// Big nasty but we don't want 2 diffrent datastructs for this
	pAsync->m_Offset = _FileOffset;
	pAsync->m_DataSize = _DataSize;
	uint64 FilePos;
	int Error = cellFsLseek(pAsync->m_fd, pAsync->m_Offset, CELL_FS_SEEK_SET, &FilePos);
	if(Error)
	{
		delete pAsync;
		return NULL;
	}
	uint64 WriteBytes;
	Error = cellFsWrite(pAsync->m_fd, pAsync->m_pData, pAsync->m_DataSize, &WriteBytes);
	if(Error)
	{
		delete pAsync;
		return NULL;
	}
	pAsync->m_bDone = true;
	pAsync->m_ProcessedBytes = WriteBytes;
#endif
	return pAsync;
}

void MRTC_SystemInfo::OS_FileClose(void *_pFile)
{
	int fd = (int)((aint)_pFile);
	cellFsClose(fd);
}

bool MRTC_SystemInfo::OS_FileExists(const char *_pPath)
{
	CFStr FileName = CFStrF("/app_home/%s", _pPath);
	PS3File_FixPath(FileName.GetStr());
	int fd;
	int Error = cellFsOpen(FileName.Str(), CELL_FS_O_RDONLY, &fd, NULL, 0);
	if(Error)
		return false;
	cellFsClose(fd);
	return true;
}

void MRTC_SystemInfo::OS_FileGetDrive(const char *_pFilenName, char *_pDriveName)
{
	memcpy(_pDriveName, "cellfs", 7);
}

static M_INLINE int64 TimeT2FileTime(time_t _time)
{
	return ((int64)_time) * 10000000LL + 116444736000000000LL;
}

bool MRTC_SystemInfo::OS_FileGetTime(void *_pFile, int64& _TimeCreate, int64& _TimeAccess, int64& _TimeWrite)
{
	int fd = (int)((aint)_pFile);
	CellFsStat fsstat;
	int Error = cellFsFstat(fd, &fsstat);
	if(Error)
		return false;

	_TimeCreate = TimeT2FileTime(fsstat.st_ctime);
	_TimeAccess = TimeT2FileTime(fsstat.st_atime);
	_TimeWrite = TimeT2FileTime(fsstat.st_mtime);

	return true;

}

void* gf_FileOpen(const char *_pFilenName, bool _bRead, bool _bWrite, bool _bCreate, bool _bTruncate, bool _bDeferClose)
{
	CFStr FileName = CFStrF("%s", _pFilenName);

	PS3File_FixPath(FileName.GetStr());

	int open_params = 0;
	if(_bRead && _bWrite)
		open_params = CELL_FS_O_RDWR;
	else if(_bRead)
		open_params = CELL_FS_O_RDONLY;
	else if(_bWrite)
		open_params = CELL_FS_O_WRONLY;

	if(_bCreate)
		open_params |= CELL_FS_O_CREAT;
	if(_bTruncate)
		open_params |= CELL_FS_O_TRUNC;

	int fd;
	int Error = cellFsOpen(FileName.Str(), open_params, &fd, NULL, 0);
	if(Error)
		return NULL;
	return (void*)(aint)fd;
}

void* MRTC_SystemInfo::OS_FileOpen(const char *_pFilenName, bool _bRead, bool _bWrite, bool _bCreate, bool _bTruncate, bool _bDeferClose)
{
	CFStr FileName = CFStrF("/app_home/%s", _pFilenName);
	return gf_FileOpen(FileName, _bRead, _bWrite, _bCreate, _bTruncate, _bDeferClose);
}

int MRTC_SystemInfo::OS_FileOperationGranularity(const char *_pPath)
{
	return 2048;
}

fint MRTC_SystemInfo::OS_FilePosition(const char *_pFileName)
{
	DebugBreak();
	return 0;
}

bool MRTC_SystemInfo::OS_FileSetFileSize(const char *_pFilenName, fint _FileSize)
{
	CFStr FileName = CFStrF("/app_home/%s", _pFilenName);
	PS3File_FixPath(FileName.GetStr());

	int Error = cellFsTruncate(FileName.Str(), _FileSize);
	if(Error)
		return false;

	return true;
}

bool MRTC_SystemInfo::OS_FileSetTime(void *_pFile, const int64& _TimeCreate, const int64& _TimeAccess, const int64& _TimeWrite)
{
	DebugBreak();
	return false;
}

fint MRTC_SystemInfo::OS_FileSize(void *_pFile)
{
	int fd = (int)((aint)_pFile);
	CellFsStat fsstat;
	int Error = cellFsFstat(fd, &fsstat);
	if(Error)
		return 0;

	return fsstat.st_size;
}

bool MRTC_SystemInfo::OS_FileRemove(const char* _pPath)
{
	CFStr FileName = CFStrF("/app_home/%s", _pPath);
	PS3File_FixPath(FileName.GetStr());
	int Error = cellFsUnlink(FileName.Str());
	if(Error)
		return false;
	return true;
}

bool MRTC_SystemInfo::OS_FileRename(const char* _pFrom, const char* _pTo)
{
	CFStr FileNameFrom = CFStrF("/app_home/%s", _pFrom);
	CFStr FileNameTo = CFStrF("/app_home/%s", _pTo);
	PS3File_FixPath(FileNameFrom.GetStr());
	PS3File_FixPath(FileNameTo.GetStr());

	int Error = cellFsRename(FileNameFrom.Str(), FileNameTo.Str());
	if(Error)
		return false;
	return true;
}

bool MRTC_SystemInfo::OS_DirectoryExists(const char *_pPath)
{
	CFStr FileName = CFStrF("/app_home/%s", _pPath);
	PS3File_FixPath(FileName.GetStr());
	int fd;
	int Error = cellFsOpendir(FileName.Str(), &fd);
	if(Error)
		return false;
	cellFsClosedir(fd);
	return true;
}

bool MRTC_SystemInfo::OS_DirectoryChange(const char* _pPath)
{
	DebugBreak();
	return false;
}

bool MRTC_SystemInfo::OS_DirectoryCreate(const char* _pPath)
{
	CFStr FileName = CFStrF("/app_home/%s", _pPath);
	PS3File_FixPath(FileName.GetStr());
	int Error = cellFsMkdir(FileName.Str(), 0x777);
	if(Error)
		return false;
	return true;
}

char* MRTC_SystemInfo::OS_DirectoryGetCurrent(char* _pBuf, int _MaxLength)
{
	DebugBreak();
	return NULL;
}

bool MRTC_SystemInfo::OS_DirectoryRemove(const char* _pPath)
{
	CFStr FileName = CFStrF("/app_home/%s", _pPath);
	PS3File_FixPath(FileName.GetStr());
	int Error = cellFsRmdir(FileName.Str());
	if(Error)
		return false;
	return true;
}

const char* MRTC_SystemInfo::OS_DirectorySeparator()
{
	return "/";
}

class CPS3_FindFile
{
public:
	CPS3_FindFile()
	{
		m_Files.SetGrow(512);
		m_iCurrentFile = 0;
	}
	
	class CFileEntry
	{
	public:
		
		CFileEntry()
		{
			m_FileSize = 0;
			m_bDirectory = 0;
			m_TimeCreate = 0;
			m_TimeAccess = 0;
			m_TimeWrite = 0;
		}

		CStr m_FileName;
		fint m_FileSize;
		bint m_bDirectory;
		int64 m_TimeCreate;
		int64 m_TimeAccess;
		int64 m_TimeWrite;
	};
	TList_Vector<CFileEntry> m_Files;

	int m_iCurrentFile;


	void OptimizeMemory()
	{
		m_Files.OptimizeMemory();
	}
};

bint PS3_MatchPattern(const char *_pFile, const char *_pPattern)
{
	CFStr Parse;
	Parse.Capture(_pFile);
	Parse.MakeLowerCase();
	CFStr Pattern;
	Pattern.Capture(_pPattern);
	Pattern.MakeLowerCase();

	const char *pParse = Parse.Str();
	const char *pPattern = Pattern.Str();

	while (*pParse && *pPattern)
	{
		if (*pPattern == '*')
		{
			++pPattern;
			while (*pParse && *pParse != *pPattern) // Search until we find pattern charater we don't allow such useless things like "*?.exe"
			{
				++pParse;
			}			
		}
		else if (*pPattern == '?')
		{
			++pPattern;
			++pParse;
		}
		else
		{
			if (*pPattern != *pParse)
				break; // break search failed

			++pPattern;
			++pParse;
		}
	}

	if (*pParse == *pPattern)
		return true;

	return false;
}

aint PS3File_FindFirst( const char *_pPath, char *_pRet, int &_FileSize, bool &_bDir)
{
	CFStr FileName = CFStrF("/app_home/%s", _pPath);
	PS3File_FixPath(FileName.GetStr());
	CFStr Directory = FileName;
	CFStr FindPart;

	int iFind = Directory.FindReverse("/");
	if (iFind < 0)
		return 0;

	FindPart.Capture(Directory.Str() + iFind + 1);
	Directory = Directory.Left(iFind);
	
	int DirFile;
	CellFsErrno Err = cellFsOpendir(Directory.Str(), &DirFile);
	if (Err == CELL_FS_SUCCEEDED)
	{
		CPS3_FindFile *pFindFile = DNew(CPS3_FindFile) CPS3_FindFile();

		while (1)
		{
			CellFsDirent DirectoryEntry;
			uint64 NRead = 0;
			cellFsReaddir(DirFile, &DirectoryEntry, &NRead);
			if (NRead)
			{
				CFStr FileName;
				FileName.Capture(DirectoryEntry.d_name);
				int nFileNameLen = FileName.Len();
				if(nFileNameLen == 0)
					continue;

				if(FileName[nFileNameLen - 1] == '/')
				{
					FileName.GetStr()[nFileNameLen - 1] = 0;
					nFileNameLen--;
				}

				if (FileName == "." || FileName == "..")
					continue;

				if (PS3_MatchPattern(FileName.Str(), FindPart.Str()))
				{
//					M_TRACEALWAYS("%s\n", FileName.Str());
					CPS3_FindFile::CFileEntry File;
					File.m_FileName = FileName;
					File.m_bDirectory = DirectoryEntry.d_type == CELL_FS_TYPE_DIRECTORY;

					CFStr FullFileName = Directory + "/" + FileName;
/*
					int DirFileTest;
					CellFsErrno DirErr = cellFsOpendir(FullFileName.Str(), &DirFileTest);
					if (DirErr == CELL_FS_SUCCEEDED)
					{
						CellFsDirent DirectoryEntry;
						uint64 NRead = 0;
						cellFsReaddir(DirFileTest, &DirectoryEntry, &NRead);
						if (NRead)
							File.m_bDirectory = true;
						cellFsClosedir(DirFileTest);
					}
*/

					CellFsStat fsstat;
					CellFsErrno Err = cellFsStat(FullFileName, &fsstat);
					if (Err == CELL_FS_SUCCEEDED)
					{
						File.m_FileSize = fsstat.st_size;
						File.m_TimeAccess = fsstat.st_atime;
						File.m_TimeCreate = fsstat.st_ctime;
						File.m_TimeWrite = fsstat.st_mtime;
					}

					pFindFile->m_Files.Add(File);
				}
			}
			else
				break;
		}

		cellFsClosedir(DirFile);

		if (!pFindFile->m_Files.Len())
		{
			delete pFindFile;
			pFindFile = NULL;
		}
		else
		{
			pFindFile->OptimizeMemory();
			strncpy(_pRet, pFindFile->m_Files[0].m_FileName.Str(), 1024);
			_FileSize = pFindFile->m_Files[0].m_FileSize;
			_bDir = pFindFile->m_Files[0].m_bDirectory;
			pFindFile->m_iCurrentFile = 1;
		}

		return (aint)pFindFile;
	}
	return 0;
}

aint PS3File_FindNext( aint _FindHandle, char *_pRet, int &_FileSize, bool &_bDir)
{
	CPS3_FindFile *pFindFile = (CPS3_FindFile *)_FindHandle;
	if (pFindFile->m_iCurrentFile < pFindFile->m_Files.Len())
	{
		strncpy(_pRet, pFindFile->m_Files[pFindFile->m_iCurrentFile].m_FileName.Str(), 1024);
		_FileSize = pFindFile->m_Files[pFindFile->m_iCurrentFile].m_FileSize;
		_bDir = pFindFile->m_Files[pFindFile->m_iCurrentFile].m_bDirectory;
		++pFindFile->m_iCurrentFile;
		return true;
	}
	return 0;
}

void PS3File_FindClose( aint _handle )
{
	CPS3_FindFile *pFindFile = (CPS3_FindFile *)_handle;
	delete pFindFile;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Network
|__________________________________________________________________________________________________
\*************************************************************************************************/

bint MRTC_SystemInfo::CNetwork::gf_ResolveAddres(const ch8 *_pAddress, NNet::CNetAddressIPv4 &_Address)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_ResolveAddres(_pAddress, _Address);
}

void *MRTC_SystemInfo::CNetwork::gf_Connect(const NNet::CNetAddressTCPv4 &_Address, NThread::CEventAutoResetReportableAggregate *_pReportTo)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_Connect(_Address, _pReportTo);
}

void *MRTC_SystemInfo::CNetwork::gf_Bind(const NNet::CNetAddressUDPv4 &_Address, NThread::CEventAutoResetReportableAggregate *_pReportTo)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_Bind(_Address, _pReportTo);
}

void *MRTC_SystemInfo::CNetwork::gf_Listen(const NNet::CNetAddressTCPv4 &_Address, NThread::CEventAutoResetReportableAggregate *_pReportTo)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_Listen(_Address, _pReportTo);
}

void *MRTC_SystemInfo::CNetwork::gf_Accept(void *_pSocket, NThread::CEventAutoResetReportableAggregate *_pReportTo)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_Accept((MRTC_SystemInfoInternal::CTCPContext::CTCPSocket *)_pSocket, _pReportTo);
}

void MRTC_SystemInfo::CNetwork::gf_SetReportTo(void *_pSocket, NThread::CEventAutoResetReportableAggregate *_pReportTo)
{
	MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_SetReportTo((MRTC_SystemInfoInternal::CTCPContext::CTCPSocket *)_pSocket, _pReportTo);
}

void *MRTC_SystemInfo::CNetwork::gf_InheritHandle(void *_pSocket, NThread::CEventAutoResetReportableAggregate *_pReportTo)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_InheritHandle((MRTC_SystemInfoInternal::CTCPContext::CTCPSocket *)_pSocket, _pReportTo);
}



uint32 MRTC_SystemInfo::CNetwork::gf_GetState(void *_pSocket)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_GetState((MRTC_SystemInfoInternal::CTCPContext::CTCPSocket *)_pSocket);
}

void MRTC_SystemInfo::CNetwork::gf_Close(void *_pSocket)
{
	if (MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_Close((MRTC_SystemInfoInternal::CTCPContext::CTCPSocket *)_pSocket))
	{
		// Destry the ctp context if no more sockets are created
		MRTC_GetSystemInfo().m_pInternalData->DestroyTCPContext();
	}
}

int MRTC_SystemInfo::CNetwork::gf_Receive(void *_pSocket, void *_pData, int _DataLen)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_Receive((MRTC_SystemInfoInternal::CTCPContext::CTCPSocket *)_pSocket, _pData, _DataLen);
}

int MRTC_SystemInfo::CNetwork::gf_Send(void *_pSocket, const void *_pData, int _DataLen)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_Send((MRTC_SystemInfoInternal::CTCPContext::CTCPSocket *)_pSocket, _pData, _DataLen);
}

int MRTC_SystemInfo::CNetwork::gf_Receive(void *_pSocket, NNet::CNetAddressUDPv4 &_Address, void *_pData, int _DataLen)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_Receive((MRTC_SystemInfoInternal::CTCPContext::CTCPSocket *)_pSocket, _Address, _pData, _DataLen);
}

int MRTC_SystemInfo::CNetwork::gf_Send(void *_pSocket, const NNet::CNetAddressUDPv4 &_Address, const void *_pData, int _DataLen)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_Send((MRTC_SystemInfoInternal::CTCPContext::CTCPSocket *)_pSocket, _Address, _pData, _DataLen);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Misc stuff
|__________________________________________________________________________________________________
\*************************************************************************************************/

void MRTC_ReferenceSymbol(...)
{
/*
	va_list arg;
	va_start(arg, arg);
	ch8 Str[2];
	Str[0] = (mint)arg;
	Str[1] = 0;
	CStrBase::IsDigit(Str[0]);
*/
}


extern void MRTC_CreateObjectManager();
extern void MRTC_DestroyObjectManager();

//void __attribute__((constructor)) gf_CallCreate() 
//{
	//MRTC_CreateObjectManager();
//}


class CInitFirst
{
public:
	CInitFirst()
	{
		MRTC_CreateObjectManager();
	}
	~CInitFirst()
	{
		MRTC_DestroyObjectManager();
	}
};

CInitFirst __attribute__((init_priority(101))) PS3Init;
