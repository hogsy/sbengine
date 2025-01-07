
#include "PCH.h"
#include "MRTC_RemoteDebug.h"

#include "MNetwork.h"


#if defined(MRTC_ENABLE_REMOTEDEBUGGER)

class MRTC_RemoteDebugInternal : public MRTC_Thread
{
public:

	class CProfiler : public MRTC_Thread
	{
	public:

		CProfiler()
		{
			m_SleepTime = 0;
			m_SnapID = 0;

		}

		int Thread_Main()
		{
			MRTC_SystemInfo::Thread_SetName("Remote debug profiler");
			MRTC_SystemInfo::Thread_SetProcessor(4);
			MRTC_RemoteDebugChannel *pChn = MRTC_GetRD()->CreateDebugChannel(51);
			while (!Thread_IsTerminating())
			{
				mint Size;
				const void *pData = pChn->GetHeadPacketData(Size);
				if (pData)
				{
					CCFile File;
					File.Open((void *)pData, Size, Size, CFILE_READ);
					uint16 Message;
					File.ReadLE(Message);
					// Handle settings
					switch (Message)
					{
					case 1:
						{
							// Set sleep time
							File.ReadLE(m_SleepTime);
						};
						break;
					}
				}

				MRTC_SystemInfo::OS_SendProfilingSnapshot(m_SnapID);

				++m_SnapID;

				if (m_SleepTime)
					MRTC_SystemInfo::OS_Sleep(m_SleepTime);
				else 
					MRTC_SystemInfo::OS_Yeild();
			}
			pChn->Delete();
			return 0;
		}

		uint32 m_SleepTime;
		uint32 m_SnapID;
	};

	CProfiler m_Profiler;

	enum
	{
		EMaxPacketSize = 1152,
		ETempBufferSize = 128*1024
	};

	spCNetwork m_spNetwork;
	uint8 m_TempBuffer[ETempBufferSize];
	uint8 m_TempBufferCopy[ETempBufferSize];
	void *m_pPacketBuffer;
	void *m_pPacketBufferCopy;
	mint m_TempBufferPos;
	mint m_PacketBufferPos;
	mint m_TempBufferSize;
	bint m_bCreateSuccess;
	int m_Recurse;
	int m_FlushTempBufferRecurse;
	int m_iServerConnection;
	int m_iClientConnection;
	int m_bPendingDisconnect;
	int m_iDebugChannel;

	uint64 m_HeapSecuence;

//	int m_ThreadLookAside0;
//	int m_ThreadLookAside1;
	CMTime m_NextPeriodicUpdate;

//	MRTC_CriticalSection m_NetworkLock;
	MRTC_CriticalSection m_Lock;
	MRTC_CriticalSection m_FlushLock;

	NThread::CEventAutoResetReportable m_Event;

	MRTC_RemoteDebugInternal()
	{
		m_bCreateSuccess = true;
		m_Recurse = 0;
		m_HeapSecuence = 0;
		// Alloc a 512 KiB temp buffer;
		m_TempBufferSize = ETempBufferSize;
//		m_pTempBuffer = MRTC_SystemInfo::OS_Alloc(m_TempBufferSize, true);
		m_TempBufferPos = 0;
		m_PacketBufferPos = 0;
		m_FlushTempBufferRecurse = 0;
		m_iServerConnection = -1;
		m_iClientConnection = -1;
		m_bPendingDisconnect = 0;
		m_iDebugChannel = -1;
		m_ModuleInit = 0;
	}

	~MRTC_RemoteDebugInternal()
	{
		Thread_Destroy();

		m_DebugChannelTree.f_DeleteAll();

		if (m_spNetwork)
		{
			if (m_iClientConnection >= 0)
				m_spNetwork->Connection_Close(m_iClientConnection);
			if (m_iServerConnection >= 0)
				m_spNetwork->Connection_Close(m_iServerConnection);
			m_iServerConnection = -1;
			m_iClientConnection = -1;
		}
	}

	__declspec(thread) static int m_iCurrentScopeStack;
	__declspec(thread) static int m_iCurrentCategoryStack;
#ifdef MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTSCOPE
	__declspec(thread) static MRTC_RemoteDebugScope *m_ScopeStack[DRDMaxScopeMaxStack];
#endif
#ifdef MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCATEGORY
	__declspec(thread) static MRTC_RemoteDebugCategory *m_CatogoryStack[DRDMaxCategoryMaxStack];
#endif

	void InitClientConnection(CNetwork *_pNetwork,int _iClient)
	{

		uint8 InitPacket[2048];
		mint Size = 1024;

		MRTC_SystemInfo::RD_ClientInit(InitPacket, Size);

		if (Size > 1024)
			M_BREAKPOINT;

		CNetwork_Packet Pack;
		Pack.m_pData = InitPacket;
		Pack.m_Size = Size;
		if (!_pNetwork->Connection_PacketSend(_iClient, &Pack))
			m_bPendingDisconnect = true;
	}

	void PreInitClientConnection(CNetwork *_pNetwork, int _iClient)
	{
		_pNetwork->Connection_ChannelSetBufferSize(_iClient, 0, 512*1024);
		_pNetwork->Connection_ChannelSetNumPackets(_iClient, 0, 3000);
		m_iDebugChannel = _pNetwork->Connection_ChannelAlloc(_iClient, ENetworkChannelFlag_Reliable, 0.5, 0.0);
		_pNetwork->Connection_ChannelSetBufferSize(_iClient, m_iDebugChannel, 512*1024);
		_pNetwork->Connection_ChannelSetNumPackets(_iClient, m_iDebugChannel, 3000);
	}

	void Create(int _Port)
	{
		if (!m_spNetwork)
		{

			spCNetwork spNetwork = (CNetwork *)MRTC_GetObjectManager()->CreateObject("CNetworkCore");
			M_ASSERT(spNetwork, "Mega?");
			if (!spNetwork)
			{
				m_bCreateSuccess = false;
				return;
			}
			CNetwork_Device *pDevice = (CNetwork_Device *)MRTC_GetObjectManager()->CreateObject("CNetwork_Device_UDP");
			M_ASSERT(pDevice, "Mega?");

			if (!pDevice)
			{
				m_bCreateSuccess = false;
				return;
			}

			spNetwork->Create(pDevice);
//			m_ThreadLookAside0 = m_spNetwork->Global_GetThreadID();
			m_bCreateSuccess = true;
			m_pPacketBuffer = MRTC_SystemInfo::OS_Alloc(4*1024, true); // We need EMaxPacketSize bytes a packet

			CNetwork_Address *pAddr = spNetwork->Global_ResolveAddress(CStrF("*:%d", _Port));
			if (!pAddr)
			{
				MRTC_SystemInfo::OS_TraceRaw("Failed resolve remote debugger listen address.");
				return;
			}

			int iServer = spNetwork->Server_Listen(pAddr, &m_Event);
			int iClient = -1;
			pAddr->Delete();
			if (iServer < 0)
			{
				m_bCreateSuccess = false;
				m_TempBufferPos = 0;
				MRTC_GetRD()->m_EnableFlags = 0;
				MRTC_SystemInfo::OS_TraceRaw("Failed to create remote debugger listen socket.");
				return;
			}

			char ServerName[32];
			MRTC_SystemInfo::RD_GetServerName(ServerName);

			spNetwork->Server_SetInfo(iServer, CNetwork_ServerInfo(ServerName, NULL, 0));

           // Wait 500 ms for a client to connect if no client connects, disable remote debugger and clear temp queue
			{
				{
					//JK-NOTE: m_Lock has not been taken yet, cannot unlock it
//					M_UNLOCK(m_Lock);
					CMTime Timeout = CMTime::GetCPU() + CMTime::CreateFromSeconds(1.5);
					while (CMTime::GetCPU().Compare(Timeout) < 0)
					{
						if (spNetwork->Server_Connection_Avail(iServer)) 
						{
							iClient = spNetwork->Server_Connection_Accept(iServer, &m_Event);
							if (iClient >= 0)
							{
								PreInitClientConnection(spNetwork, iClient);
								spNetwork->Connection_Flush(iClient);
								InitClientConnection(spNetwork, iClient);
							}
						}
					}					
				}

				if (iClient >= 0)
				{
				}
				else
				{
					m_TempBufferPos = 0;
					MRTC_GetRD()->m_EnableFlags = 0;
				}

			}

			{
				M_LOCK(m_Lock);
				m_spNetwork = spNetwork;
				m_iServerConnection = iServer;
				m_iClientConnection = iClient;
			}

			// Create thread to handle periodic events
			Thread_Create();
		}
	}


	/*
	void FlushBufferNoLock()
	{
		if (m_PacketBufferPos > 0)
		{
			if (m_FlushLock.TryLock())
			{
				m_Lock.Unlock();
				M_ASSERT(m_PacketBufferPos <= EMaxPacketSize, "We should not be able to have a buffer larger than this");
				CNetwork_Packet Pack;
				Pack.m_pData = m_pPacketBuffer;
				Pack.m_Size = m_PacketBufferPos;
				CMTime Timeout = CMTime::GetCPU() + CMTime::CreateFromSeconds(5.0);
				{
					while (!m_spNetwork->Connection_PacketSend(m_iClientConnection, &Pack))
					{				
						m_Event.WaitTimeout(0.050f);
						if (CMTime::GetCPU().Compare(Timeout) > 0)
						{
							MRTC_SystemInfo::OS_TraceRaw("RD: Timed out trying to send packet\n");
							m_bPendingDisconnect = true;
							break;
						}
					}
				}
				m_PacketBufferPos = 0;
				m_FlushLock.Unlock();
				m_Lock.Lock();
			}
		}
	}

	void FlushTempBufferNoLock()
	{
		if (m_TempBufferPos > 0)
		{
			if (m_FlushLock.TryLock())
			{
				++m_FlushTempBufferRecurse;

				memcpy(m_TempBufferCopy, m_TempBuffer, m_TempBufferPos);
				mint Size = m_TempBufferPos;
				m_TempBufferPos = 0;
				uint8 *pPacket = m_TempBufferCopy;
				{
					m_FlushLock.Lock();
					m_Lock.Unlock();

					while (Size)
					{
						int ToSend = MinMT(Size, EMaxPacketSize);
						CNetwork_Packet Pack;
						Pack.m_pData = pPacket;
						Pack.m_Size = ToSend;
						Size -= ToSend;
						pPacket += ToSend;
						CMTime Timeout = CMTime::GetCPU() + CMTime::CreateFromSeconds(5.0);
						{
							while (!m_spNetwork->Connection_PacketSend(m_iClientConnection, &Pack))
							{
								m_Event.WaitTimeout(0.050f);
								if (CMTime::GetCPU().Compare(Timeout) > 0)
								{
									MRTC_SystemInfo::OS_TraceRaw("RD: Timed out trying to send packet\n");
									m_bPendingDisconnect = true;
									break;
								}
							}
						}
					}
					m_FlushLock.Unlock();
					m_Lock.Lock();
				}

				--m_FlushTempBufferRecurse;
			}
		}
	}*/

	void FlushBuffer()
	{
		if (m_PacketBufferPos > 0)
		{
			M_LOCK(m_FlushLock);
			M_UNLOCK(m_Lock);
//			m_FlushLock.Lock();
//			m_Lock.Unlock();
			M_ASSERT(m_PacketBufferPos <= EMaxPacketSize, "We should not be able to have a buffer larger than this");
			CNetwork_Packet Pack;
			Pack.m_pData = m_pPacketBuffer;
			Pack.m_Size = m_PacketBufferPos;
			CMTime Timeout = CMTime::GetCPU() + CMTime::CreateFromSeconds(10.0);
			{
				while (!m_spNetwork->Connection_PacketSend(m_iClientConnection, &Pack))
				{	
					m_Event.WaitTimeout(0.050f);
					if (CMTime::GetCPU().Compare(Timeout) > 0)
					{
						MRTC_SystemInfo::OS_TraceRaw("RD: Timed out trying to send packet\n");
						m_bPendingDisconnect = true;
						break;
					}
				}
			}
			m_PacketBufferPos = 0;
//			m_FlushLock.Unlock();
//			m_Lock.Lock();
		}
	}

	void FlushTempBuffer()
	{
		if (m_TempBufferPos > 0)
		{
			if (m_TempBufferPos <= (EMaxPacketSize - m_PacketBufferPos))
			{
				memcpy((uint8 *)m_pPacketBuffer + m_PacketBufferPos, m_TempBuffer, m_TempBufferPos);
				m_PacketBufferPos += m_TempBufferPos;
				m_TempBufferPos = 0;
				return ;
			}

			FlushBuffer();

			++m_FlushTempBufferRecurse;
			
			memcpy(m_TempBufferCopy, m_TempBuffer, m_TempBufferPos);
			mint Size = m_TempBufferPos;
			m_TempBufferPos = 0;
			uint8 *pPacket = m_TempBufferCopy;
			{
				M_LOCK(m_FlushLock);
				M_UNLOCK(m_Lock);
//				m_FlushLock.Lock();
//				m_Lock.Unlock();

				while (Size && !m_bPendingDisconnect)
				{
					int ToSend = MinMT(Size, EMaxPacketSize);
					CNetwork_Packet Pack;
					Pack.m_pData = pPacket;
					Pack.m_Size = ToSend;
					Size -= ToSend;
					pPacket += ToSend;
					CMTime Timeout = CMTime::GetCPU() + CMTime::CreateFromSeconds(10.0f);
					{
						while (!m_spNetwork->Connection_PacketSend(m_iClientConnection, &Pack))
						{
							m_Event.WaitTimeout(0.050f);
							if (CMTime::GetCPU().Compare(Timeout) > 0)
							{
								MRTC_SystemInfo::OS_TraceRaw("RD: Timed out trying to send packet\n");
								m_bPendingDisconnect = true;
								break;
							}
						}
					}
				}
//				m_FlushLock.Unlock();
//				m_Lock.Lock();
			}

			--m_FlushTempBufferRecurse;
		}
	}

	MRTC_MutualWriteManyRead m_QueuLock;
	void QueuePacketOnlyBuffer(const void *_pPacket, mint _Size)
	{
		if (!m_bCreateSuccess || m_bPendingDisconnect)
			return;

		bool bSuccess;
		M_MWMR_Lock_Unlock(m_QueuLock, bSuccess);
		if (!bSuccess)
			return;

		CMTime Timeout = CMTime::GetCPU() + CMTime::CreateFromSeconds(15.0);
		while (1)
		{
			{
				M_LOCK(m_Lock);

				mint Maxxa = m_TempBufferSize - m_TempBufferPos;
				if (_Size <= Maxxa)
				{
					memcpy((uint8 *)m_TempBuffer + m_TempBufferPos, _pPacket, _Size);

					m_TempBufferPos += _Size;
					
					return;
				}
			}

			if (CMTime::GetCPU().Compare(Timeout) > 0)
			{
				m_bPendingDisconnect = true;
				MRTC_SystemInfo::OS_TraceRaw("RDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRD: Failed to queue packet\n");
				break;
			}
			else
			{
				MRTC_SystemInfo::OS_Sleep(1);
			}
		}
	}

	void QueuePacket(const void *_pPacket, mint _Size)
	{
		if (!m_bCreateSuccess || m_bPendingDisconnect)
			return;

		bool bSuccess;
		M_MWMR_Lock_Unlock(m_QueuLock, bSuccess);
		if (!bSuccess)
			return;

		CMTime Timeout = CMTime::GetCPU() + CMTime::CreateFromSeconds(15.0);
		while (1)
		{
			{
				aint ModuleInit = ModuleInitGet();
				M_LOCK(m_Lock);

				if (m_spNetwork && m_iClientConnection >= 0 && m_Recurse == 0 && ModuleInit == 0)
				{
					m_Recurse = 1;
					M_ASSERT(m_FlushTempBufferRecurse == 0, "Not good if we get a recurse here");
					FlushTempBuffer();

					if (!m_bPendingDisconnect)
					{
						mint Size = _Size;
						uint8 *pPacket = (uint8 *)_pPacket;

						while (Size)
						{
							mint ToSend = MinMT(Size, EMaxPacketSize - m_PacketBufferPos);
							memcpy((uint8 *)m_pPacketBuffer + m_PacketBufferPos, pPacket, ToSend);
							pPacket += ToSend;
							m_PacketBufferPos += ToSend;
							Size -= ToSend;
							if (m_PacketBufferPos == EMaxPacketSize)
							{
								FlushBuffer();
								if (m_bPendingDisconnect)
								{
									m_Recurse = 0;
									return;
								}
							}
						}
						m_Recurse = 0;
						return;
					}
					else
					{
						m_Recurse = 0;
						return;
					}

				}
				else
				{

					mint Maxxa = m_TempBufferSize - m_TempBufferPos;
					if (_Size <= Maxxa)
					{
						memcpy((uint8 *)m_TempBuffer + m_TempBufferPos, _pPacket, _Size);

						m_TempBufferPos += _Size;
						
						return;
					}
					else if (ModuleInit != 0)
					{
//						MRTC_SystemInfo::OS_TraceRaw("RDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRD: Queue full\n");
					}
				}
			}

			if (CMTime::GetCPU().Compare(Timeout) > 0)
			{
				m_bPendingDisconnect = true;
				MRTC_SystemInfo::OS_TraceRaw("RDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRDRD: Failed to queue packet\n");
				break;
			}
			else
			{
				MRTC_SystemInfo::OS_Sleep(1);
			}

		}
	}
	
#ifdef MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTSCOPE
	void AddScope(MRTC_RemoteDebugScope *_pScope)
	{
		if (m_iCurrentScopeStack >= (DRDMaxScopeMaxStack - 1))
		{
			M_BREAKPOINT; // Scope stack overflow, no way to recover
			return ;
		}
		++m_iCurrentScopeStack;
		m_ScopeStack[m_iCurrentScopeStack] = _pScope;
	}

	void RemoveScope()
	{
		--m_iCurrentScopeStack;
	}
#endif

#ifdef MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCATEGORY
	void AddCategory(MRTC_RemoteDebugCategory *_pCategory)
	{
		if (m_iCurrentCategoryStack >= (DRDMaxCategoryMaxStack - 1))
		{
			M_BREAKPOINT; // Scope stack overflow, no way to recover
			return ;
		}
		++m_iCurrentCategoryStack;
		m_CatogoryStack[m_iCurrentCategoryStack] = _pCategory;
	}

	void RemoveCategory()
	{
		--m_iCurrentCategoryStack;
	}
#endif

	class CRemoteDebugChannel : public MRTC_RemoteDebugChannel
	{
	public:
		class CCompare
		{
		public:
			M_INLINE static int Compare(const CRemoteDebugChannel *_pFirst, const CRemoteDebugChannel *_pSecond, void *_pContext)
			{
				if (_pFirst->m_iChannel > _pSecond->m_iChannel)
					return 1;
				else if (_pFirst->m_iChannel < _pSecond->m_iChannel)
					return -1;

				return 0;
			}
			M_INLINE static int Compare(const CRemoteDebugChannel *_pTest, int _Key, void *_pContext)
			{
				if (_Key > _pTest->m_iChannel)
					return -1;
				else if (_Key < _pTest->m_iChannel)
					return 1;

				return 0;
			}
		};

		~CRemoteDebugChannel()
		{
			m_InQueue.DeleteAll();
			m_OutQueue.DeleteAll();
			if (m_pInPacket)
			{
				delete m_pInPacket;
				m_pInPacket = NULL;
			}
			if (m_pOutPacket)
			{
				delete m_pOutPacket;
				m_pOutPacket = NULL;
			}
		}

		DIdsTreeAVLAligned_Link(CRemoteDebugChannel, m_TreeLink, int, CCompare);

		CRemoteDebugChannel(MRTC_RemoteDebugInternal *_pRD, int _iChannel)
		{
			m_pRD = _pRD;
			m_iChannel = _iChannel;
			m_pInPacket = NULL;
			m_pOutPacket = NULL;
			m_InPacketPtr = 0;
			m_OutTempPtr = 8;
			*((uint32 *)m_OutTemp) = m_iChannel;
			*((uint32 *)(m_OutTemp+4)) = 0;
			SwapLE(*((uint32 *)m_OutTemp));
		}

		int m_iChannel;
		MRTC_RemoteDebugInternal *m_pRD;

		MRTC_CriticalSection m_Lock;

		class CPacket
		{
		public:
			TThinArray<uint8> m_Data;
			DLinkD_Link(CPacket, m_Link);
		};

		DLinkD_List(CPacket, m_Link) m_InQueue;
		DLinkD_List(CPacket, m_Link) m_OutQueue;

		CPacket *m_pInPacket;
		uint8 *m_pInPacketPtr;
		mint m_InPacketSize;
		mint m_InPacketPtr;
		uint8 m_InSize[4];

		void DataIn(const void *_pData, mint _Size)
		{
			uint8 *pData = (uint8 *)_pData;
			while (_Size)
			{
				if (m_InPacketPtr >= 4)
				{
					mint ToGet = Min(m_InPacketSize - (m_InPacketPtr - 4), _Size);
					memcpy(m_pInPacketPtr + m_InPacketPtr - 4, pData, ToGet);

					m_InPacketPtr += ToGet;
					pData += ToGet;
					_Size -= ToGet;

					if (m_InPacketPtr == (m_InPacketSize + 4))
					{
						M_LOCK(m_Lock);
						m_InQueue.Insert(m_pInPacket);
						m_pInPacket = NULL;
						m_InPacketPtr = 0;
					}
				}
				else
				{
					mint ToGet = Min(4 - m_InPacketPtr, _Size);
					memcpy(m_InSize + m_InPacketPtr, pData, ToGet);

					m_InPacketPtr += ToGet;
					pData += ToGet;
					_Size -= ToGet;

					if (m_InPacketPtr == 4)
					{
						m_pInPacket = DNew(CPacket) CPacket();
						uint32 Len = *((uint32 *)m_InSize);
						SwapLE(Len);
						m_InPacketSize = Len;
						m_pInPacket->m_Data.SetLen(Len);
						m_pInPacketPtr = m_pInPacket->m_Data.GetBasePtr();
					}
				}
			}
		}

		CPacket *m_pOutPacket;
		uint8 *m_pOutPacketPtr;
		mint m_OutPacketSize;
		mint m_OutPacketPtr;
		uint8 m_OutSize[4];
		uint8 m_OutTemp[EMaxPacketSize];
		mint m_OutTempPtr;

		void DataOut()
		{
			while (1)
			{
				if (!m_pOutPacket)
				{
					M_LOCK(m_Lock);
					m_pOutPacket = m_OutQueue.Pop();				
					if (!m_pOutPacket)
						break;
					m_pOutPacketPtr = m_pOutPacket->m_Data.GetBasePtr();
					m_OutPacketSize = m_pOutPacket->m_Data.Len();
					m_OutPacketPtr = 0;
					*((uint32 *)m_OutSize) = m_OutPacketSize;
					
					SwapLE(*((uint32 *)m_OutSize));
				}

				if(m_pOutPacket)
				{
					if (m_OutPacketPtr < 4)
					{
						mint ToSend = MinMT(4 - m_OutPacketPtr, EMaxPacketSize - m_OutTempPtr);
						memcpy(m_OutTemp + m_OutTempPtr, m_OutSize + m_OutPacketPtr, ToSend);
						m_OutPacketPtr += ToSend;
						m_OutTempPtr += ToSend;
					}

					if (m_OutPacketPtr >= 4)
					{
						mint ToSend = MinMT(m_OutPacketSize - (m_OutPacketPtr - 4), EMaxPacketSize - m_OutTempPtr);
						memcpy(m_OutTemp + m_OutTempPtr, m_pOutPacketPtr + m_OutPacketPtr - 4, ToSend);
						m_OutPacketPtr += ToSend;
						m_OutTempPtr += ToSend;
					}

					if (m_OutPacketPtr == m_OutPacketSize + 4)
					{
//						MRTC_SystemInfo::OS_TraceRaw(CFStrF("Added message Size: %d\n", m_OutPacketPtr));
						delete m_pOutPacket;
						m_pOutPacket = NULL;
					}
				}

				if (m_OutTempPtr == EMaxPacketSize)
				{					
					CNetwork_Packet Pack;
					Pack.m_pData = m_OutTemp;
					Pack.m_Size = m_OutTempPtr;
					Pack.m_iChannel = m_pRD->m_iDebugChannel;

//					MRTC_SystemInfo::OS_TraceRaw(CFStrF("Sending full debug packet Size: %d Ptr %d Size %d ID %d\n", m_OutTempPtr - 4, m_OutPacketPtr, m_OutPacketSize, (*((uint32 *)(m_OutTemp+4)))));
					if (!m_pRD->m_spNetwork->Connection_PacketSend(m_pRD->m_iClientConnection, &Pack))
					{
//						MRTC_SystemInfo::OS_TraceRaw("Failed to send debug channel packet\n");
						return;
					}

					m_OutTempPtr = 8;
					++(*((uint32 *)(m_OutTemp+4)));
				}
			}

			if (m_OutTempPtr > 8)
			{					
				M_ASSERT(!m_pOutPacket, "Should not arrive here otherwise");
//				MRTC_SystemInfo::OS_TraceRaw(CFStrF("Sending non-full debug packet Size: %d Ptr %d Size %d ID%d\n", m_OutTempPtr - 8, m_OutPacketPtr, m_OutPacketSize, (*((uint32 *)(m_OutTemp+4)))));
				CNetwork_Packet Pack;
				Pack.m_pData = m_OutTemp;
				Pack.m_Size = m_OutTempPtr;
				Pack.m_iChannel = m_pRD->m_iDebugChannel;

				if (!m_pRD->m_spNetwork->Connection_PacketSend(m_pRD->m_iClientConnection, &Pack))
				{
//					MRTC_SystemInfo::OS_TraceRaw("Failed to send debug channel packet\n");
					return;
				}

				m_OutTempPtr = 8;
				++(*((uint32 *)(m_OutTemp+4)));
			}
		}

		virtual void SendPacket(const void *_pData, mint _Size)
		{
			CPacket *pPacket = DNew(CPacket) CPacket();
			pPacket->m_Data.SetLen(_Size);
			memcpy(pPacket->m_Data.GetBasePtr(), _pData, _Size);

			{
				M_LOCK(m_Lock);
				m_OutQueue.Insert(pPacket);
			}
		}

		virtual const void *GetHeadPacketData(mint &_Size)
		{
			M_LOCK(m_Lock);
			CPacket *pPacket = m_InQueue.GetFirst();
			if (pPacket)
			{
				_Size = pPacket->m_Data.Len();
				return pPacket->m_Data.GetBasePtr();
			}
			else
				return NULL;
		}

		void Clear()
		{
			m_InQueue.DeleteAll();
			m_OutQueue.DeleteAll();
			if (m_pInPacket)
			{
				delete m_pInPacket;
				m_pInPacket = NULL;
			}
			if (m_pOutPacket)
			{
				delete m_pOutPacket;
				m_pOutPacket = NULL;
			}
			m_InPacketPtr = 0;
			m_OutTempPtr = 8;
			*((uint32 *)(m_OutTemp+4)) = 0;
		}

		virtual void PopPacket()
		{
			M_LOCK(m_Lock);
			CPacket *pPacket = m_InQueue.Pop();

			if (pPacket)
				delete pPacket;
		}

		virtual void Delete()
		{
			{
                M_LOCK(m_pRD->m_DebugChannelLock);
				m_pRD->m_DebugChannelTree.f_Remove(this);
			}

			delete this;
		}
	};

	MRTC_CriticalSection m_DebugChannelLock;
	DIdsTreeAVLAligned_Tree(CRemoteDebugChannel, m_TreeLink, int, CRemoteDebugChannel::CCompare) m_DebugChannelTree;

	void UpdateChannelSend()
	{
		M_LOCK(m_DebugChannelLock);

		DIdsTreeAVLAligned_Iterator(CRemoteDebugChannel, m_TreeLink, int, CRemoteDebugChannel::CCompare) Iter = m_DebugChannelTree;

		while (Iter)
		{
			Iter->DataOut();

			++Iter;
		}
	}

	void ClearChannels()
	{
		M_LOCK(m_DebugChannelLock);

		DIdsTreeAVLAligned_Iterator(CRemoteDebugChannel, m_TreeLink, int, CRemoteDebugChannel::CCompare) Iter = m_DebugChannelTree;

		while (Iter)
		{
			Iter->Clear();

			++Iter;
		}
	}

	CRemoteDebugChannel *GetDebugChannel(int _iChannel)
	{
		CRemoteDebugChannel *pChannel;
		{
			M_LOCK(m_DebugChannelLock);
			pChannel = m_DebugChannelTree.FindEqual(_iChannel);
		}

		if (!pChannel)
			pChannel = DNew(CRemoteDebugChannel) CRemoteDebugChannel(this, _iChannel);
		else
			return pChannel;

		if (pChannel)
		{
			M_LOCK(m_DebugChannelLock);
			m_DebugChannelTree.f_Insert(pChannel);
		}
		return pChannel;
	}

	MRTC_RemoteDebugChannel *CreateDebugChannel(int _iChannel)
	{
		return GetDebugChannel(_iChannel);
	}

	void Flush()
	{
		{

			if (!m_bCreateSuccess || m_bPendingDisconnect)
				return;

			bool bSuccess;
			M_MWMR_Lock_Unlock(m_QueuLock, bSuccess);
			if (!bSuccess)
				return;

			M_LOCK(m_Lock);

			if (m_spNetwork && m_iClientConnection >= 0 && m_Recurse == 0)
			{
				m_Recurse = 1;
				M_ASSERT(m_FlushTempBufferRecurse == 0, "Not good if we get a recurse here");
				FlushTempBuffer();
				if (!m_bPendingDisconnect)
					FlushBuffer();
				m_Recurse = 0;
			}
		}
//		M_LOCK(m_FlushLock);
	//	if (m_spNetwork && m_iClientConnection >= 0 )
	//		FlushBuffer();
	//	if (m_spNetwork && m_iClientConnection >= 0 && !m_bPendingDisconnect)
	//		FlushTempBuffer();
		if (m_spNetwork && m_iClientConnection >= 0 && !m_bPendingDisconnect )
			m_spNetwork->Connection_Flush(m_iClientConnection);

	}

	MRTC_CriticalSection m_ModuleInitLock;
	aint m_ModuleInit;

	aint ModuleInitGet()
	{
		M_LOCK(m_ModuleInitLock);
		return m_ModuleInit;
	}

	void ModuleInit()
	{
		M_LOCK(m_ModuleInitLock);
		++m_ModuleInit;
	}

	void ModuleFinish()
	{
		M_LOCK(m_ModuleInitLock);
		--m_ModuleInit;
	}


	MRTC_CriticalSection m_HeapSequenceLock;
	uint64 GetHeapSequence()
	{
		M_LOCK(m_HeapSequenceLock);
		++m_HeapSecuence;
		return m_HeapSecuence;
	}

	void UpdatePeriodic(const CMTime &_Now)
	{
		{
			Flush(); // Flush every 50 ms

//			m_spNetwork->Refresh();

			if (m_iClientConnection >= 0)
			{
				// Check for tiemout
				uint32 Status = m_spNetwork->Connection_Status(m_iClientConnection);
				if (Status & (ENetworkPacketStatus_Timeout | ENetworkPacketStatus_Closed) || m_bPendingDisconnect)
				{
					Disconnect();
				}
				else
				{
					while (1)
					{
						CNetwork_Packet *pPacket = m_spNetwork->Connection_PacketReceive(m_iClientConnection);
						if (pPacket)
						{
							if (pPacket->m_iChannel == m_iDebugChannel)
							{
								uint32 iChannel = *((uint32 *)pPacket->m_pData);
								SwapLE(iChannel);
								CRemoteDebugChannel *pChn = GetDebugChannel(iChannel);
								if (pChn)
								{
									pChn->DataIn((uint8 *)pPacket->m_pData + 4, pPacket->m_Size - 4);
								}
							}
							else
							{
								uint8 *pData = (uint8 *)pPacket->m_pData;
								uint8 *pDataEnd = pData + pPacket->m_Size;

								int Message = *pData; ++pData;

								switch (Message)
								{
								case ERemoteDebugClient_KeepAlive:
									{
										uint8 Packet[8];
										uint8 *pPkt = Packet;
										*((uint32 *)pPkt) = 8; SwapLE(*((uint32 *)pPkt)); pPkt+= 4;
										*((uint32 *)pPkt) = ERemoteDebug_KeepAlive; SwapLE(*((uint32 *)pPkt)); pPkt+= 4;
										QueuePacket(Packet, 8);
									}
									break;
								case ERemoteDebugClient_EnableFlags:
									{
										if (pData + 4 <= pDataEnd)
										{
											uint32 Enable = *((uint32 *)pData);
											SwapLE(Enable);
											MRTC_GetRD()->m_EnableFlags = Enable;
										}
									}
									break;
								}
							}

							m_spNetwork->Connection_PacketRelease(m_iClientConnection, pPacket);
						}
						else
							break;
					}
					// Refuse other connection attempts
					int nMax = 10;
					while (m_spNetwork->Server_Connection_Avail(m_iServerConnection) && nMax)
					{
						--nMax;
						{
							bool bSuccess;
							M_MWMR_MutualLock_MutualUnlock(m_QueuLock, bSuccess);
							M_ASSERT(m_Recurse == 0, "");
							m_Recurse = 1;

						}

						m_spNetwork->Server_Connection_Refuse(m_iServerConnection);

						{
							M_LOCK(m_Lock);
							m_Recurse = 0;
						}

					}

					UpdateChannelSend();
				}
			}

			if (m_iClientConnection < 0)
			{
				if (m_spNetwork->Server_Connection_Avail(m_iServerConnection))
				{
					int iClient = m_spNetwork->Server_Connection_Accept(m_iServerConnection, &m_Event);

					ClearChannels();

					if (iClient >= 0)
					{
						m_spNetwork->Connection_Flush(iClient);
						PreInitClientConnection(m_spNetwork, iClient);
						{
							bool bSuccess;
							M_MWMR_MutualLock_MutualUnlock(m_QueuLock, bSuccess);
							M_ASSERT(bSuccess, "");
							m_Recurse = 1;
							MRTC_GetRD()->m_EnableFlags = 0xffffffff & ~ERDEnableFlag_Profiling;
							m_iClientConnection = iClient;
							m_PacketBufferPos = 0; // Clear old data
							m_TempBufferPos = 0;
							InitClientConnection(m_spNetwork, iClient);
							m_Recurse = 0;
						}
					}
				}
			}
		}

		if (_Now.Compare(m_NextPeriodicUpdate) >= 0)
		{
			m_NextPeriodicUpdate = _Now + CMTime::CreateFromSeconds(1.0);

//			m_ThreadLookAside1 = 0;

			MRTC_SystemInfo::RD_PeriodicUpdate();

//			m_ThreadLookAside1 = (int)MRTC_SystemInfo::OS_GetThreadID();
		}

		if (MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_Profiling)
		{
			if (!m_Profiler.Thread_IsCreated())
				m_Profiler.Thread_Create(NULL,16384, MRTC_THREAD_PRIO_TIMECRITICAL);
		}
		else
		{
			if (m_Profiler.Thread_IsCreated())
				m_Profiler.Thread_Destroy();
		}

	}

	void Disconnect()
	{
		M_LOCK(m_Lock);
		MRTC_GetRD()->m_EnableFlags = 0;
		int iConnection = m_iClientConnection;
		m_iClientConnection = -1;
		m_bPendingDisconnect = false;
		m_spNetwork->Connection_Close(iConnection);
		m_PacketBufferPos = 0; // Clear old data
		m_TempBufferPos = 0;
	}

	int Thread_Main()
	{
		MRTC_SystemInfo::Thread_SetName("Remote debug");
//		m_ThreadLookAside1 = (int)MRTC_SystemInfo::OS_GetThreadID();
		fp4 Periodicity = 1.0/60.0;
		CMTime NextPeriodic = CMTime::CreateFromSeconds(Periodicity) + CMTime::GetCPU();
		while (!Thread_IsTerminating())
		{
			CMTime Now = CMTime::GetCPU();
			if (Now.Compare(NextPeriodic) > 0)
			{
				UpdatePeriodic(Now);
				NextPeriodic = Now + CMTime::CreateFromSeconds(Periodicity);
			}

			MRTC_SystemInfo::OS_Sleep(1000/120);				
		}

		return 0;
	}
};

__declspec(thread) int MRTC_RemoteDebugInternal::m_iCurrentScopeStack = -1;
__declspec(thread) int MRTC_RemoteDebugInternal::m_iCurrentCategoryStack = -1;
#ifdef MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTSCOPE
__declspec(thread) MRTC_RemoteDebugScope *MRTC_RemoteDebugInternal::m_ScopeStack[DRDMaxScopeMaxStack];
#endif

#ifdef MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCATEGORY
__declspec(thread) MRTC_RemoteDebugCategory *MRTC_RemoteDebugInternal::m_CatogoryStack[DRDMaxCategoryMaxStack];
#endif



#ifndef MRTC_ENABLE_REMOTEDEBUGGER_STATIC
MRTC_RemoteDebug* MRTC_ObjectManager::GetRemoteDebugger()
{
#ifdef MRTC_DISABLE_REMOTEDEBUG
	return NULL;
#endif
	if ((mint)m_pRemoteDebugger == 1)
		return NULL;
	if (!m_pRemoteDebugger)
	{
		static int Recurse = 0;
		if (Recurse)
			return NULL;
		++Recurse;
		static uint64 s_RemoteData[(sizeof(MRTC_RemoteDebug) + 7) / 8];
		MRTC_RemoteDebug *pDebugger = new((void *)s_RemoteData) MRTC_RemoteDebug;
		//pDebugger->m_EnableFlags = 0xffffffff & ~ERDEnableFlag_SendCRTAllocs;
		pDebugger->m_EnableFlags = 0xffffffff & (~ERDEnableFlag_Profiling);
		static uint64 s_RemoteDataInternal[(sizeof(MRTC_RemoteDebugInternal) + 7) / 8];
		pDebugger->m_pInternalData = new((void *)s_RemoteDataInternal) MRTC_RemoteDebugInternal;
		m_pRemoteDebugger = pDebugger;
		--Recurse;
	}
	return m_pRemoteDebugger;
}
M_INLINE MRTC_RemoteDebugInternal *MRTC_RemoteDebug::GetInternal()
{
	return m_pInternalData;
}
#else
MRTC_RemoteDebug gs_RemoteDebugger = {0,0xffffffff & (~ERDEnableFlag_Profiling)};

M_INLINE MRTC_RemoteDebugInternal *MRTC_RemoteDebug::GetInternal()
{
#ifdef MRTC_DISABLE_REMOTEDEBUG
	return NULL;
#else
	if (!m_pInternalData)
	{
		static int Recurse = 0;
		if (Recurse)
			return NULL;
		++Recurse;
		static uint64 s_RemoteDataInternal[(sizeof(MRTC_RemoteDebugInternal) + 7) / 8];
		m_pInternalData = new((void *)s_RemoteDataInternal) MRTC_RemoteDebugInternal;
		--Recurse;
	}
	return m_pInternalData;
#endif
}
#endif

void MRTC_RemoteDebug::Destroy()
{
	m_EnableFlags = 0;
	if (m_pInternalData)
	{
		m_pInternalData->~MRTC_RemoteDebugInternal();
		m_pInternalData = NULL;
	}
}


uint64 MRTC_RemoteDebug::GetSequence()
{
	return GetInternal()->GetHeapSequence();
}

MRTC_RemoteDebugChannel *MRTC_RemoteDebug::CreateDebugChannel(int _iChannel)
{
	MRTC_RemoteDebugInternal *pInternal = GetInternal();
	return pInternal->CreateDebugChannel(_iChannel);
}

void MRTC_RemoteDebug::Flush()
{
	MRTC_RemoteDebugInternal *pInternal = GetInternal();
	pInternal->Flush();
}

void MRTC_RemoteDebug::Create(int _Port)
{
	MRTC_RemoteDebugInternal *pInternal = GetInternal();
	pInternal->Create(_Port);
}

void MRTC_RemoteDebug::ModuleInit()
{
	MRTC_RemoteDebugInternal *pInternal = GetInternal();
	pInternal->ModuleInit();
}

void MRTC_RemoteDebug::ModuleFinish()
{
	MRTC_RemoteDebugInternal *pInternal = GetInternal();
	pInternal->ModuleFinish();
}

void MRTC_RemoteDebug::SendDataOnlyBuffer(uint32 _Message, const void *_pData, mint _DataSize)
{
	MRTC_RemoteDebugInternal *pInternal = GetInternal();

	uint8 PacketData[DRDMaxSendStackSize]; // Packet Packet Size
	aint nNeededData = _DataSize + sizeof(uint32) + sizeof(uint32);

	if (nNeededData > 8192)
	{
		M_BREAKPOINT; // Just die
		return ;
	}

	uint8 *pPacket;
	pPacket = PacketData;

	uint8 *pCurrent = pPacket;
	*((uint32 *)pCurrent) = _DataSize + sizeof(uint32)*2; SwapLE(*((uint32 *)pCurrent)); pCurrent += sizeof(uint32); // Packet Size
	*((uint32 *)pCurrent) = _Message; SwapLE(*((uint32 *)pCurrent)); pCurrent += sizeof(uint32); // Message ID
	memcpy(pCurrent, _pData, _DataSize); pCurrent += _DataSize;// Message Data

	pInternal->QueuePacketOnlyBuffer(pPacket, (pCurrent - pPacket));
}

void MRTC_RemoteDebug::SendDataRaw(uint32 _Message, const void *_pData, mint _DataSize)
{
	MRTC_RemoteDebugInternal *pInternal = GetInternal();

	uint8 PacketData[DRDMaxSendStackSize]; // Packet Packet Size
	aint nNeededData = _DataSize + sizeof(uint32) + sizeof(uint32);

	if (nNeededData > 8192)
	{
		M_BREAKPOINT; // Just die
		return ;
	}

	uint8 *pPacket;
	pPacket = PacketData;

	uint8 *pCurrent = pPacket;
	*((uint32 *)pCurrent) = _DataSize + sizeof(uint32)*2; SwapLE(*((uint32 *)pCurrent)); pCurrent += sizeof(uint32); // Packet Size
	*((uint32 *)pCurrent) = _Message; SwapLE(*((uint32 *)pCurrent)); pCurrent += sizeof(uint32); // Message ID
	memcpy(pCurrent, _pData, _DataSize); pCurrent += _DataSize;// Message Data

	pInternal->QueuePacket(pPacket, (pCurrent - pPacket));
}

void MRTC_RemoteDebug::SendData(uint32 _Message, const void *_pData, mint _DataSize, bool _bAttachStackTrace, bool _bSendCategoryScope, mint _Ebp)
{
	MRTC_RemoteDebugInternal *pInternal = GetInternal();
	mint StackTrace[128];
	mint nStack = 0;
	mint nCategory = _bSendCategoryScope ? pInternal->m_iCurrentCategoryStack + 1:0;
//	mint nScope = _bSendCategoryScope ? pInternal->m_iCurrentScopeStack + 1:0;
	mint nScope = 0;

	if (_bAttachStackTrace)
	{
		nStack = MRTC_SystemInfo::OS_TraceStack(StackTrace, 128, _Ebp);
	}

	uint8 PacketData[DRDMaxSendStackSize]; // Packet Packet Size
	aint nNeededData = _DataSize + sizeof(uint32) * 7;
	nNeededData += sizeof(uint64) * nStack;

	if (nNeededData > 8192)
	{
		M_BREAKPOINT; // Just die
		return ;
	}

	uint8 *pPacket;
	pPacket = PacketData;

	uint8 *pCurrent = pPacket;
	uint32 *pPacketSize = (uint32 *)pCurrent; pCurrent += sizeof(uint32); // Packet Size
	*((uint32 *)pCurrent) = _Message; SwapLE(*((uint32 *)pCurrent)); pCurrent += sizeof(uint32); // Message ID
	*((uint32 *)pCurrent) = (uint32)(mint)MRTC_SystemInfo::OS_GetThreadID(); SwapLE(*((uint32 *)pCurrent)); pCurrent += sizeof(uint32); // Message ID
	*((uint32 *)pCurrent) = _DataSize; SwapLE(*((uint32 *)pCurrent)); pCurrent += sizeof(uint32); // DataSize
	*((uint32 *)pCurrent) = nStack; SwapLE(*((uint32 *)pCurrent)); pCurrent += sizeof(uint32); // Num stacktrace data
	*((uint32 *)pCurrent) = nCategory; SwapLE(*((uint32 *)pCurrent)); pCurrent += sizeof(uint32); // Num Categories
	*((uint32 *)pCurrent) = nScope; SwapLE(*((uint32 *)pCurrent)); pCurrent += sizeof(uint32); // Num Scopes
	memcpy(pCurrent, _pData, _DataSize); pCurrent += _DataSize;// Message Data

	for (int i = 0; i < nStack; ++i)
	{
		uint64 Stack = StackTrace[i];
		SwapLE(Stack);
		*((uint64 *)pCurrent) = Stack; pCurrent += sizeof(uint64); // Num Scopes
	}

#ifdef MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCATEGORY
	if (nCategory)
	{
		int Len = CStr::StrLen(pInternal->m_CatogoryStack[nCategory-1]->m_pName)+1;
		*((uint32 *)pCurrent) = Len; SwapLE(*((uint32 *)pCurrent)); pCurrent += sizeof(uint32);
		memcpy(pCurrent, pInternal->m_CatogoryStack[nCategory-1]->m_pName, Len); pCurrent += Len;
	}
#endif

#ifdef MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTSCOPE
	for (int i = 0; i < nScope; ++i)
	{
		int Len = CStr::StrLen(pInternal->m_ScopeStack[i]->m_pName)+1;
		*((uint32 *)pCurrent) = Len; SwapLE(*((uint32 *)pCurrent)); pCurrent += sizeof(uint32);
		memcpy(pCurrent, pInternal->m_ScopeStack[i]->m_pName, Len); pCurrent += Len;
	}
#endif
	*pPacketSize = (pCurrent - pPacket);
	SwapLE(*pPacketSize); 

	pInternal->QueuePacket(pPacket, (pCurrent - pPacket));
}

#ifdef MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCATEGORY
MRTC_RemoteDebugCategory::MRTC_RemoteDebugCategory(const char *_pCategory)
{
//	if (!(MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_Category))
//		return;
	m_pName = _pCategory;
	if (MRTC_GetRD())
		MRTC_GetRD()->GetInternal()->AddCategory(this);
}

MRTC_RemoteDebugCategory::~MRTC_RemoteDebugCategory()
{
//	if (!(MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_Category))
//		return;
	if (MRTC_GetRD())
		MRTC_GetRD()->GetInternal()->RemoveCategory();
}
#endif

#ifdef MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTSCOPE
MRTC_RemoteDebugScope::MRTC_RemoteDebugScope(const char *_pScope)
{
//	if (!(MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_Scope))
//		return;
	m_pName = _pScope;
	if (MRTC_GetRD())
		MRTC_GetRD()->GetInternal()->AddScope(this);
}

MRTC_RemoteDebugScope::~MRTC_RemoteDebugScope()
{
//	if (!(MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_Scope))
//		return;
	if (MRTC_GetRD())
		MRTC_GetRD()->GetInternal()->RemoveScope();
}
#endif




void gf_RDSendRegisterPhysicalHeap(mint _Heap, const char *_pName)
{
	if (!MRTC_GetRD() || !(MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_PhysicalMemory))
		return;
	uint64 Data[128];
	Data[0] = _Heap; SwapLE(Data[0]); 
	strcpy((char *)(Data+1), _pName);
	MRTC_GetRD()->SendData(ERemoteDebug_RegisterPhysicalHeap, Data, 9+strlen(_pName), false, false);
}

void gf_RDSendRegisterHeap(mint _Heap, const char *_pName)
{
	if (!MRTC_GetRD() || !(MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_HeapMemory))
		return;
	uint64 Data[128];
	Data[0] = _Heap;SwapLE(Data[0]); 
	strcpy((char *)(Data+1), _pName);
	MRTC_GetRD()->SendData(ERemoteDebug_RegisterHeap, Data, 9+strlen(_pName), false, false);
}

inline void ByteSwap_uint64_Test(uint64& _Value)
{
	uint64 Temp = _Value;
	_Value = _byteswap_uint64(Temp);
}

inline void ByteSwap_uint64_Test(int64& _Value)
{
	uint64 Temp = _Value;
	_Value = _byteswap_uint64(Temp);
}

void gf_RDSendPhysicalAlloc(void *_pData, mint _Size, mint _Heap, uint64 _Sequence, uint32 _Type)
{
#ifndef MRTC_ENABLE_REMOTEDEBUGGER_STATIC
	if (!MRTC_GetObjectManager() || !MRTC_GetObjectManager()->GetRemoteDebugger() || !MRTC_SystemInfo::MRTC_GetSystemInfo().m_pInternalData)
		return;
#endif
	if (_pData == NULL)
		M_BREAKPOINT;

	if (!(MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_PhysicalMemory))
		return;
	if (!(MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_SendCRTAllocs) && (_Type == 2))
		return;
	uint64 Data[5];
	static uint64 *spData;
	spData = Data;
	Data[0] = (mint)_pData;
	Data[1] = _Size;
	Data[2] = _Heap;
	Data[3] = _Type;
	Data[4] = _Sequence;
	SwapLE(Data[0]);
	SwapLE(Data[1]);
	SwapLE(Data[2]);
	SwapLE(Data[3]);
	SwapLE(Data[4]);
	{
		if (!(MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_PhysicalMemory))
			return;
		MRTC_GetRD()->SendData(ERemoteDebug_AllocPhysical, Data, sizeof(Data), true, true);
	}
}

void gf_RDSendHeapAlloc(void *_pData, mint _Size, void *_pHeap, uint64 _Sequence, uint32 _Type)
{
#ifndef MRTC_ENABLE_REMOTEDEBUGGER_STATIC
	if (MRTC_SystemInfo::MRTC_GetSystemInfo().m_pInternalData) 
#endif

	{
		if (!MRTC_GetRD() || !(MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_SendCRTAllocs) && (_Type == 2))
			return;

		if (_pData == NULL)
			M_BREAKPOINT;

		uint64 Data[5];
		Data[0] = (mint)_pData;
		Data[1] = _Size;
		Data[2] = (mint)_pHeap;
		Data[3] = _Type;
		Data[4] = _Sequence;

		SwapLE(Data[0]);
		SwapLE(Data[1]);
		SwapLE(Data[2]);
		SwapLE(Data[3]);
		SwapLE(Data[4]);

		if (!(MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_HeapMemory))
			return;
		MRTC_GetRD()->SendData(ERemoteDebug_AllocHeap, Data, sizeof(Data), true, true);
	}
}

void gf_RDSendHeapClear(void *_pHeap)
{
	if (!MRTC_GetRD() || !(MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_HeapMemory))
		return;

	uint64 Data[1];
	Data[0] = (mint)_pHeap;
	SwapLE(Data[0]);

	MRTC_GetRD()->SendData(ERemoteDebug_ClearHeap, Data, sizeof(Data), false, false);
}

void gf_RDSendHeapFree(void *_pData, void *_pHeap, uint64 _Sequence)
{
	if (!MRTC_GetRD() || !(MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_HeapMemory))
		return;
	if (_pData == NULL)
		M_BREAKPOINT;

	uint64 Data[3];
	Data[0] = (mint)_pData;
	Data[1] = (mint)_pHeap;
	Data[2] = _Sequence;
	SwapLE(Data[0]);
	SwapLE(Data[1]);
	SwapLE(Data[2]);
	

	MRTC_GetRD()->SendData(ERemoteDebug_FreeHeap, Data, sizeof(Data), false, false);
}

void gf_RDSendPhysicalFree(void *_pData, mint _Heap, uint64 _Sequence)
{
	if (!MRTC_GetRD() || !(MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_PhysicalMemory))
		return;
	if (_pData == NULL)
		M_BREAKPOINT;
	uint64 Data[3];
	Data[0] = (mint)_pData;
	Data[1] = _Heap;
	Data[2] = _Sequence;
	SwapLE(Data[0]);
	SwapLE(Data[1]);
	SwapLE(Data[2]);

	MRTC_GetRD()->SendData(ERemoteDebug_FreePhysical, Data, sizeof(Data), false, false);
}

#endif
