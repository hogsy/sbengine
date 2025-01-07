
#include "PCH.h"

#include "MNetwork.h"
MRTC_IMPLEMENT(CNetwork, CReferenceCount);
MRTC_IMPLEMENT(CNetwork_Device, CReferenceCount);

#define DDefaultHeapSize 32*1024
#define DDefaultNumPackets 1024
//extern void gfs_GetProgramPath(CStr &_Str);
//#define TRACEME M_TRACEALWAYS
#define TRACEME 1 ? (void)0 : MRTC_SystemInfo::OS_Trace
//#define CHECKQUEUES

class CNetworkCore : public CNetwork
{
	MRTC_DECLARE;
public:

	CNetworkCore()
	{
		m_pThread = NULL;
		m_ThreadID = 0;
	}

	~CNetworkCore()
	{
		if (m_pThread)
			delete m_pThread;
	}

	TPtr<CNetwork_Device> m_spDevice;
	virtual void Create(CNetwork_Device *_pDevice)
	{
		m_spDevice = _pDevice;
		m_ConnectionIDs.Create(32768, true);

		
		m_pThread = DNew(CWorkerThread) CWorkerThread(this);
		m_pThread->Thread_Create(NULL, 32768, MRTC_THREAD_PRIO_HIGHEST);
	}

	class CWorkerThread : public MRTC_Thread
	{
	public:

		CNetworkCore *m_pNet;
		CWorkerThread(CNetworkCore *_pNet)
		{
			m_pNet = _pNet;
		}
		~CWorkerThread()
		{
			Thread_Destroy();
		}

		int Thread_Main()
		{
			return m_pNet->Thread_Main();
		}
	};

	enum
	{
		EConnectionType_None = 0,
		EConnectionType_Server,
		EConnectionType_Client,

		EMessageInternal_Query = 0,
		EMessageInternal_ConnectionInfo,
		EMessageInternal_Connect,
		EMessageInternal_Disconnect,
		EMessageInternal_Refuse,

/*
		NET_PACKET_QUERY				1
		NET_PACKET_CONNECTIONINFO		2

		NET_PACKET_OPENCONNECTION		3

		NET_PACKET_DATA					5
		NET_PACKET_DATACONFIRM			6
		NET_PACKET_DATARESEND			7

		NET_PACKET_DISCONNECT			8
*/
	};

	/*
	Channel:5
	ThisID?:1
	Ack?:1
	Resend?:1
	?ThisID:16
	?AckID:16
	?ResendID:16

	Size:14
	InternalData:1
	StopBit:1

	Size:14
	InternalData:1
	StopBit:1

	Size:14
	InternalData:1
	StopBit:1
    
	Channel:5
	ThisID?:1
	Ack?:1
	Resend?:1
	?ThisID:16
	?AckID:16
	?ResendID:16

	Size:14
	InternalData:1
	StopBit:1

	Size:14
	InternalData:1
	StopBit:1

	Size:14
	InternalData:1
	StopBit:1

	nt8 Message:3;
	uint8 Channel:5;
	uint8 Size:11;
	uint8 SerialThis:13;

	uint8 Message:3;
	uint8 Size0:5;

	uint8 Size1:4;
	uint8 Serial0:4;

				uint8 Serial0:8;*/
	class CConnection : public CReferenceCount
	{
	public:

		CNetworkCore *m_pNet;
		DLinkD_Link(CConnection, m_Link);

		MRTC_CriticalSection m_AcceptLock;
		DLinkD_Link(CConnection, m_AcceptLink);
		DLinkD_List(CConnection, m_AcceptLink) m_Available;

		MRTC_CriticalSection m_DeleteLock;

		class CStatUpdater
		{
		public:

			enum
			{
				EQueueLength = 4
			};
			fp8 m_Value;
			fp8 m_ValuePerSec[EQueueLength];
			CMTime m_Timer;
			CMTime m_QueueTime;
			CMTime m_LastValueTime;
			int m_iCurrentQueue;
			fp8 m_Average;
			MRTC_CriticalSection m_Lock;

			CStatUpdater()
			{
				for (int i = 0; i < EQueueLength; ++i)
				{
					m_ValuePerSec[i] = 0.0f;
				}
				m_iCurrentQueue = 0;
				m_Average = 0.0;
				m_QueueTime = CMTime::CreateFromSeconds(0.25f);
				m_LastValueTime = CMTime::GetCPU();
			}

			void AddValue(fp8 _Value, CMTime _Now)
			{
				m_Value += _Value;

                if (_Now.Compare(m_Timer) > 0)
				{
					m_ValuePerSec[m_iCurrentQueue] = m_Value / (_Now - m_LastValueTime).GetTime();
					m_LastValueTime = _Now;
					m_Value = 0.0f;

					m_Timer = _Now + m_QueueTime;
					m_iCurrentQueue++;
					if (m_iCurrentQueue >= EQueueLength)
						m_iCurrentQueue = 0;

					fp8 Average = 0;
					for (int i = 0; i < EQueueLength; ++i)
					{
						Average += m_ValuePerSec[i];
					}

					Average /= EQueueLength;

					{
						M_LOCK(m_Lock);
						m_Average = Average;
					}
				}
			}

			fp8 GetValue()
			{
				M_LOCK(m_Lock);
				return m_Average;
			}

		};



		class CPingQueue
		{
		public:
			enum
			{
				EQueueLength = 8
			};
			fp8 m_Time[EQueueLength];
			int m_nTime[EQueueLength];
			CMTime m_Timer;
			CMTime m_QueueTime;
			int m_iCurrentQueue;
			fp8 m_Average;
			MRTC_CriticalSection m_Lock;

			CPingQueue()
			{
				for (int i = 0; i < EQueueLength; ++i)
				{
					m_Time[i] = 0.0f;
					m_nTime[i] = 0;
				}
				m_iCurrentQueue = 0;
				m_Average = 0.0f;
				m_QueueTime = CMTime::CreateFromSeconds(0.25f);
			}

			void AddPing(fp8 _Ping, CMTime _Now)
			{
                if (_Now.Compare(m_Timer) > 0)
				{
					m_Timer = _Now + m_QueueTime;
					m_iCurrentQueue++;
					if (m_iCurrentQueue >= EQueueLength)
						m_iCurrentQueue = 0;

					m_Time[m_iCurrentQueue] = 0;
					m_nTime[m_iCurrentQueue] = 0;

					int iAverage = 0;
					fp8 Average = 0;
					for (int i = 0; i < EQueueLength; ++i)
					{
						if (m_nTime[i])
						{
							++iAverage;
							Average += m_Time[i] / ((fp8)m_nTime[i]);
						}
					}
					if (iAverage > 0)
						Average /= iAverage;

					{
						M_LOCK(m_Lock);
						m_Average = Average;
					}
				}
				m_Time[m_iCurrentQueue] += _Ping;
				++m_nTime[m_iCurrentQueue];
			}

			fp8 GetPing()
			{
				M_LOCK(m_Lock);
				return m_Average;
			}
		};

		CPingQueue m_PingQueue;

		CConnection(CNetworkCore *_pNet)
		{
			m_pNet = _pNet;
			m_ConnectionType = EConnectionType_None;
			m_pReportTo = NULL;
			m_pSocket = NULL;
			m_bStuffed = false;
			m_pTempAddress_Receive = _pNet->m_spDevice->Address_Alloc();
			m_pConnectedAddress = NULL;
			m_RoutingIDs.Create(256);
			m_RoutingIDs.ForceAllocID(255);
			m_ConnectionStatus = 0;
			m_ConnectionID = 255;
			m_bOwnSocket = false;
			//			m_MaxPacketSize = 1472;
#ifdef PLATFORM_XENON
			m_MaxPacketSize = 1264;
#else
			m_MaxPacketSize = 1472;
#endif
			m_Time_LastReceive = CMTime::GetCPU();	
			m_iChannel_Default = m_ChannelsSend.ChannelAlloc(100.0f, -1.0f, true, false);
			M_ASSERT(m_iChannel_Default == 0, "Must be zero");
			m_iChannel_InternalUnreliable = m_ChannelsSend.ChannelAlloc(0.2f, -1.0f, false, true);
			m_iChannel_InternalReliable = m_ChannelsSend.ChannelAlloc(0.1f, -1.0f, true, false);

			m_bForceSendUpdate = false;

			m_pParentConnection = NULL;
			m_iParentRoute = -1;
		}

		~CConnection()
		{
			if (m_pSocket && m_bOwnSocket)
				m_pSocket->Delete();

			if (m_pParentConnection && m_iParentRoute >= 0)
			{
				M_LOCK(m_pParentConnection->m_AcceptLock);
				M_TRACEALWAYS("Freeing route: %d %d\n", m_iParentRoute, m_pParentConnection->m_RoutingTable[m_iParentRoute]);
				m_pParentConnection->m_RoutingTable[m_iParentRoute] = -1;
				m_pParentConnection->m_RoutingIDs.FreeID(m_iParentRoute);
			}

			m_pTempAddress_Receive->Delete();
			if (m_pConnectedAddress)
				m_pConnectedAddress->Delete();
		}

		bint ChangeConnectionStatus(uint32 _Add, uint32 _Remove)
		{
			M_LOCK(m_MiscLock);
			uint32 OldConnStatus = Volatile(m_ConnectionStatus);
			uint32 NewStatus = (OldConnStatus & ~(_Remove)) | _Add;
			if (OldConnStatus != NewStatus)
			{
				Volatile(m_ConnectionStatus) = NewStatus;
				return true;
			}
			return false;
		}

		uint32 GetConnecitonStatus()
		{
			uint32 Status;
			{
				M_LOCK(m_MiscLock);
				Status = Volatile(m_ConnectionStatus);
				Volatile(m_ConnectionStatus) = Status & (~(ENetworkPacketStatus_Send | ENetworkPacketStatus_Receive | ENetworkPacketStatus_Connection));
			}

			return Status;
		}

		int32 m_iChannel_InternalUnreliable;
		int32 m_iChannel_InternalReliable;
		int32 m_iChannel_Default;

		MRTC_CriticalSection m_MiscLock;

		uint32 m_ConnectionType;
		uint32 m_ConnectionStatus;
		uint32 m_ConnectionID;
		mint m_MaxPacketSize;
		uint32 m_bOwnSocket:1;

		CNetwork_Socket *m_pSocket;

		CConnection *m_pParentConnection;
		int m_iParentRoute;

		NThread::CEventAutoResetReportableAggregate *m_pReportTo;

		CMTime m_Time_LastReceive;

		class CDataQueue
		{
		public:

			CDataQueue()
			{
				Clear();
			}
			~CDataQueue()
			{
				Clear();
			}

			class CPacket
			{
			public:
				CPacket()
				{
					m_Memory = 0;
					m_Size = 0;
					m_AllocSize = 0;
					m_PacketID = 0;
					m_bInternal = 0;
					m_pAddress = NULL;
					m_bResend = false;
				}
				DLinkD_Link(CPacket, m_Link); // 2
				DLinkD_Link(CPacket, m_LinkResend); // 2
				class CCompare
				{
				public:
					DIdsPInlineS static aint Compare(const CPacket *_pFirst, const CPacket *_pSecond, void *_pContext)
					{
						return (aint)_pFirst->m_PacketID - (aint)_pSecond->m_PacketID;
					}

					DIdsPInlineS static aint Compare(const CPacket *_pTest, uint32 _Key, void *_pContext)
					{
						return (aint)_pTest->m_PacketID - (aint)_Key;
					}
				};
				DAVLAligned_Link(CPacket, m_TreeLink, uint32, CCompare); // 2

				mint m_Memory;	// 3
				uint32 m_Size:16; //4
				uint32 m_AllocSize:16;
				uint32 m_PacketID:16; //5
				uint32 m_bInternal:1;
				uint32 m_bAckedOrReceived:1;
				uint32 m_bResend:1;
				CNetwork_Address *m_pAddress;//6

				CMTime m_LastSendTime;

				// 40 Byte size
			};

			void SetNumPackets(mint _nPackets)
			{
				while (1)
				{
					CPacket *pPacket = m_PacketQueue.Pop();

					if (!pPacket)
						break;
					if (pPacket->m_pAddress)
					{
						pPacket->m_pAddress->Delete();
						pPacket->m_pAddress = NULL;
					}
				}
				m_FreePackets.Clear();

				m_Packets.SetLen(_nPackets);
				for (int i = 0; i < m_Packets.Len(); ++i)
				{
					m_FreePackets.Insert(m_Packets[i]);
				}
			}

			void Clear()
			{
				while (1)
				{
					CPacket *pPacket = m_PacketQueue.Pop();

					if (!pPacket)
						break;
					if (pPacket->m_pAddress)
					{
						pPacket->m_pAddress->Delete();
						pPacket->m_pAddress = NULL;
					}
				}
				m_Heap.Clear();
				m_HeapProduce = 0;
				m_HeapConsume = 0;
				m_LastPacketID = -1;
//				m_bResend = false;
				m_QueuedSize = 0;
				m_nQueuedPackets = 0;
				m_nUnackedPackets = 0;
				m_nAckedPackets = 0;
				m_PacketSize = 0;
			}

			int GetNextPacketID()
			{
				int ID = m_LastPacketID + 1;

				if (ID >= (1 << 16))
					ID = 0;

				return ID;
			}

			MRTC_CriticalSection m_QueueLock;
			// Start Queue lock
            TThinArray<CPacket> m_Packets;
			DLinkD_List(CPacket, m_Link) m_PacketQueue;
			DLinkD_List(CPacket, m_Link) m_FreePackets;

			TThinArray<uint8> m_Heap;
			mint m_HeapProduce;
			mint m_HeapConsume;
			mint m_HeapSpace;
			mint m_QueuedSize;
			NThread::CAtomicInt m_nQueuedPackets;
			int m_nUnackedPackets;
			int m_nAckedPackets;
			int m_PacketSize;
			// Stop Queue lock 

			DLinkD_List(CPacket, m_Link) m_PacketSentQueue;

			DLinkD_List(CPacket, m_LinkResend) m_Packet_ResendQueue;
			DLinkD_List(CPacket, m_LinkResend) m_Packet_ResendCheckQueue;

			DAVLAligned_Tree(CPacket, m_TreeLink, uint32, CPacket::CCompare) m_PacketTree; // 2


//			int m_bResend;
			int m_LastPacketID;
			CMTime m_ResendTime;

			void SetHeapSize(mint _Size)
			{
				m_Heap.SetLen(_Size);

                m_HeapProduce = 0;
				m_HeapConsume = 0;
				m_HeapSpace = m_Heap.Len();
			}

			aint Alloc(mint _Size)
			{
				if (_Size > m_HeapSpace)
					return -1;

				mint Size = m_Heap.Len();
				mint MaxToEnd = Size - m_HeapProduce;
				if (_Size <= MaxToEnd)
				{
					aint Ret = m_HeapProduce;
					m_HeapProduce += _Size;
					m_HeapSpace -= _Size;
					m_QueuedSize += _Size;
					return Ret;
				}
				else
				{
					// Wrapround

					if (_Size > (m_HeapSpace - MaxToEnd))
						return -1;

					m_HeapProduce = _Size;
					m_HeapSpace -= MaxToEnd + _Size;
					m_QueuedSize += _Size;

					return 0;
				}
			}

			void Free(aint _Pos, mint _Size)
			{
				if (m_HeapConsume != _Pos)
				{
					m_HeapSpace += (m_Heap.Len() - m_HeapConsume) + _Size;
					m_HeapConsume = _Size;
					m_QueuedSize -= _Size;
				}
				else
				{
					m_HeapSpace += _Size;
					m_HeapConsume += _Size;
					m_QueuedSize -= _Size;
				}
			}

			CPacket *PushPacket(const void *_pPacket, mint _Size, int _PacketID)
			{
				CPacket *pPacket;
				aint Data;
				{
					M_LOCK(m_QueueLock);
					pPacket = m_FreePackets.Pop();

					if (!pPacket)
						return NULL;

					Data = Alloc(_Size);
					if (Data < 0)
					{
						m_FreePackets.Push(pPacket);
						return NULL;
					}
				}

				if (_pPacket)
					memcpy(m_Heap.GetBasePtr() + Data, _pPacket, _Size);
				pPacket->m_Memory = Data;
				pPacket->m_PacketID = _PacketID;
				pPacket->m_AllocSize = pPacket->m_Size = _Size;
				pPacket->m_bAckedOrReceived = false;
				pPacket->m_bResend = false;
				pPacket->m_bInternal = false;
				pPacket->m_pAddress = NULL;

				return pPacket;
			}

			CPacket *PushPacket(int _PacketID)
			{
				CPacket *pPacket;
				{
					M_LOCK(m_QueueLock);
					pPacket = m_FreePackets.Pop();

					if (!pPacket)
						return NULL;
				}

				pPacket->m_Memory = 0;
				pPacket->m_PacketID = _PacketID;
				pPacket->m_AllocSize = 0;
				pPacket->m_bAckedOrReceived = false;
				pPacket->m_bResend = false;
				pPacket->m_bInternal = false;
				pPacket->m_pAddress = NULL;

				return pPacket;
			}

			CPacket *GetHeadPacket()
			{
				CPacket *pPacket = m_Packet_ResendQueue.GetFirst();
				if (pPacket)
					return pPacket;
				return m_PacketQueue.GetFirst();
			}

			void PopHeadPacket()
			{
/*				if (m_PacketSentQueue.GetFirst() || m_Packet_ResendQueue.GetFirst())
				{
					M_ASSERT(0, "You cannot pop the head packet if there exists packets in the sent queue (circular heap would break)");
					return;
				}*/

				CNetwork_Address *pAddr = NULL;
				{
					M_LOCK(m_QueueLock);
					CPacket *pPacket = m_PacketQueue.Pop();

					if (pPacket)
					{
						TRACEME("(%x)Dec nQueued 1 (%d)\n", MRTC_SystemInfo::OS_GetProcessID(), m_nQueuedPackets.Get());
						Free(pPacket->m_Memory, pPacket->m_AllocSize);
						pAddr = pPacket->m_pAddress;
						pPacket->m_pAddress = NULL;
						if (pPacket->m_TreeLink.IsInTree())
							m_PacketTree.f_Remove(pPacket);
						m_FreePackets.Insert(pPacket);
						--m_nQueuedPackets;
#ifdef CHECKQUEUES
						CheckQueues();
#endif
					}
				}
				if (pAddr)
				{
					pAddr->Delete();
				}

			}

			void PushHeadPacketToSentQueue()
			{				
#ifdef CHECKQUEUES
				M_LOCK(m_QueueLock);/// ADDADAD
#endif
                CPacket *pPacket = m_Packet_ResendQueue.Pop();
				if (pPacket)
				{
					TRACEME("(%x)Dec nQueued 2 (%d)\n", MRTC_SystemInfo::OS_GetProcessID(), m_nQueuedPackets.Get());
					pPacket->m_LastSendTime = CMTime::GetCPU();
					pPacket->m_bResend = false;
					m_Packet_ResendCheckQueue.InsertTail(pPacket);
					--m_nQueuedPackets;
#ifdef CHECKQUEUES
					CheckQueues();
#endif
				}
				else
				{
					{
						M_LOCK(m_QueueLock);
						pPacket = m_PacketQueue.Pop();
					}

					if (pPacket)
					{
						M_ASSERT(!pPacket->m_TreeLink.IsInTree(), "");
						
						m_PacketTree.f_Insert(pPacket);

						TRACEME("(%x)Dec nQueued 3 (%d)\n", MRTC_SystemInfo::OS_GetProcessID(), m_nQueuedPackets.Get());
						m_PacketSentQueue.InsertTail(pPacket);
						pPacket->m_LastSendTime = CMTime::GetCPU();
						m_Packet_ResendCheckQueue.InsertTail(pPacket);
						--m_nQueuedPackets;
#ifdef CHECKQUEUES
						CheckQueues();
#endif
					}
				}
			}

			/*
			void PushTailSentPacketToQueue()
			{
                CPacket *pPacket = m_PacketSentQueue.GetLast();

				if (pPacket)
				{
					++m_nQueuedPackets;
					m_Packet_ResendQueue.InsertTail(pPacket);
				}
			}*/

			CPacket *GetTailSentPacket()
			{
				return m_PacketSentQueue.GetLast();
			}

			void PopHeadSentPacket()
			{
                CPacket *pPacket = m_PacketSentQueue.Pop();

				CNetwork_Address *pAddr = NULL;
				if (pPacket)
				{
					M_LOCK(m_QueueLock);

					Free(pPacket->m_Memory, pPacket->m_AllocSize);
					pAddr = pPacket->m_pAddress;
					pPacket->m_pAddress = NULL;
					m_FreePackets.Insert(pPacket);
				}
				if (pAddr)
					pAddr->Delete();
			}

			CPacket *GetHeadPacketSent()
			{
				return m_PacketSentQueue.GetFirst();
			}

			void CheckQueues()
			{
//				return;
				M_LOCK(m_QueueLock);

				int nQueued = 0;

				{
					DLinkD_Iter(CPacket, m_Link) Iter = m_PacketQueue;
					while (Iter)
					{
						++nQueued;
						++Iter;
					}
				}
				{
					DLinkD_Iter(CPacket, m_LinkResend) Iter = m_Packet_ResendQueue;
					while (Iter)
					{
						++nQueued;
						++Iter;
					}
				}

				if (nQueued != m_nQueuedPackets.Get())
				{
					TraceQueues();
					M_BREAKPOINT;
				}

			}

			void TraceQueues()
			{
				M_TRACEALWAYS("m_nQueuedPackets: %d\n", m_nQueuedPackets.Get());
				M_TRACEALWAYS("m_nUnackedPackets: %d\n", m_nUnackedPackets);


				DLinkD_Iter(CPacket, m_Link) Iter = m_PacketQueue;
				M_TRACEALWAYS("m_PacketQueue:\n");
				while (Iter)
				{
					M_TRACEALWAYS("%d %d\n", Iter->m_PacketID, Iter->m_bAckedOrReceived);
					++Iter;
				}
				Iter = m_PacketSentQueue;
				M_TRACEALWAYS("m_PacketSentQueue:\n");
				while (Iter)
				{
					M_TRACEALWAYS("%d %d\n", Iter->m_PacketID, Iter->m_bAckedOrReceived);
					++Iter;
				}
                
				// Resend queue
				
				{
					DLinkD_Iter(CPacket, m_LinkResend) Iter = m_Packet_ResendQueue;
					M_TRACEALWAYS("m_Packet_ResendQueue:\n");
					while (Iter)
					{
						M_TRACEALWAYS("%d\n", Iter->m_PacketID);
						++Iter;
					}
					Iter = m_Packet_ResendCheckQueue;
					M_TRACEALWAYS("m_Packet_ResendCheckQueue:\n");
					while (Iter)
					{
						M_TRACEALWAYS("%d %f\n", Iter->m_PacketID, Iter->m_LastSendTime.GetTime());
						++Iter;
					}
				}
				{
					DIdsTreeAVLAligned_Iterator(CPacket, m_TreeLink, uint32, CPacket::CCompare) Iter = m_PacketTree;
					M_TRACEALWAYS("m_PacketTree:\n");
					while (Iter)
					{
						M_TRACEALWAYS("%d\n", Iter->m_PacketID);
						++Iter;
					}
				}
				M_TRACEALWAYS("----------------------------------------\n");



			}

		};

		class CReceiveChannelHandler
		{
		public:
			~CReceiveChannelHandler()
			{
				for (int i = 0; i < m_Channels.Len(); ++i)
				{
					if (m_Channels[i])
						delete m_Channels[i];
				}
			}

			class CChannel
			{
			public:
				CChannel()
				{
				}
				CDataQueue m_Queue;
				DLinkD_Link(CChannel, m_Link);
				int m_iChn;
				CStatUpdater m_StatAcks;
				CStatUpdater m_PacketSize;
			};
			
			MRTC_CriticalSection m_AvailableLock;
			DLinkD_List(CChannel, m_Link) m_AvailablePacketChannels;

			CChannel *GetFirstAvailable()
			{
				M_LOCK(m_AvailableLock);
				CChannel *pChn = m_AvailablePacketChannels.GetFirst();

				return pChn;
			}

			fp8 UpdateAckStats(const CMTime &_Now, fp8 &_PacketSize, fp8 &_BufferSize)
			{
				fp8 All = 0;
				fp8 AllPacket = 0;
				_BufferSize = 0;
				for (int i = 0; i < m_Channels.Len(); ++i)
				{
					if (m_Channels[i])
					{
						int nAcks = m_Channels[i]->m_Queue.m_nAckedPackets;
						int PacketSize = m_Channels[i]->m_Queue.m_PacketSize;
						m_Channels[i]->m_Queue.m_nAckedPackets = 0;
						m_Channels[i]->m_Queue.m_PacketSize = 0;
						m_Channels[i]->m_StatAcks.AddValue(nAcks, _Now);
						m_Channels[i]->m_PacketSize.AddValue(PacketSize, _Now);						
						mint Len = m_Channels[i]->m_Queue.m_Heap.Len();
						if (Len > _BufferSize)
							_BufferSize = Len;
						
						fp8 Value = m_Channels[i]->m_StatAcks.GetValue();
						All += Value;						
						Value = m_Channels[i]->m_PacketSize.GetValue();
						AllPacket += Value;						
					}						
				}
				_PacketSize = AllPacket;
				return All;
			}

			void InsertAvailable(CChannel *_pChn)
			{
				M_LOCK(m_AvailableLock);
				m_AvailablePacketChannels.Insert(_pChn);
			}

			TThinArray<CChannel *> m_Channels;

			CChannel *GetChannel(int _iChannel)
			{
				if (_iChannel >= 16)
					return NULL;
                if (_iChannel >= m_Channels.Len())
				{
					int nLast = m_Channels.Len();
					m_Channels.SetLen(_iChannel + 1);
					for (int i = nLast; i < m_Channels.Len(); ++i)
						m_Channels[i] = NULL;
				}

				if (!m_Channels[_iChannel])
				{
					m_Channels[_iChannel] = DNew(CChannel) CChannel;
					m_Channels[_iChannel]->m_iChn = _iChannel;					
					m_Channels[_iChannel]->m_Queue.SetHeapSize(DDefaultHeapSize);
					m_Channels[_iChannel]->m_Queue.SetNumPackets(DDefaultNumPackets);
				}
				return m_Channels[_iChannel];
			}

		};
		CReceiveChannelHandler m_ChannelsReceive;

		bint QueueIncomingInternalMessage(int _iChannel, int _PacketID)
		{
			CReceiveChannelHandler::CChannel *pQueue = m_ChannelsReceive.GetChannel(_iChannel);

			if (!pQueue)
				return false;

			{
				// Insert new packets
				if (_PacketID < 0)
				{
					return true;
				}
				else
				{
					{
						CDataQueue::CPacket *pPacket = pQueue->m_Queue.m_PacketTree.FindEqual(_PacketID);

						if (!pPacket)
						{
							int CurrentID = pQueue->m_Queue.GetNextPacketID();
							// Check for packets that have propably already been received and just requeue them for acksending
							int Diff = CurrentID - _PacketID;

							if ((Diff <= 0 && Diff > -32768) || Diff > 32768)
							{
								while (1)
								{
									if (CurrentID == _PacketID)
									{
										pPacket = pQueue->m_Queue.PushPacket(_PacketID);
										if (pPacket)
										{
											pQueue->m_Queue.m_Packet_ResendCheckQueue.InsertTail(pPacket);
											++pQueue->m_Queue.m_nAckedPackets;
											if ((++pQueue->m_Queue.m_nUnackedPackets)*4 > m_MaxPacketSize)
											{
												if (m_pParentConnection)
													m_pParentConnection->m_bForceSendUpdate = true;
												else
													m_bForceSendUpdate = true;
											}
										}
										else
										{
											--CurrentID;
											if (CurrentID < 0)
												CurrentID = (1 << 16) - 1;
										}
										break;
									}
									else
									{
										pPacket = pQueue->m_Queue.PushPacket(CurrentID);
										if (!pPacket)
										{
											--CurrentID;
											if (CurrentID < 0)
												CurrentID = (1 << 16) - 1;
											break;
										}
										pQueue->m_Queue.m_PacketTree.f_Insert(pPacket);
									}
									++CurrentID;
									if (CurrentID >= (1 << 16))
										CurrentID = 0;
								}
								pQueue->m_Queue.m_LastPacketID = CurrentID;
								return true;
							}
							else
							{
								pPacket = pQueue->m_Queue.PushPacket(_PacketID);
								if (pPacket)
								{
									pQueue->m_Queue.m_Packet_ResendCheckQueue.InsertTail(pPacket);
									if ((++pQueue->m_Queue.m_nUnackedPackets)*4 > m_MaxPacketSize)
									{
										if (m_pParentConnection)
											m_pParentConnection->m_bForceSendUpdate = true;
										else
											m_bForceSendUpdate = true;
									}
								}
								return false;
							}
						}
						else 
						{					
							pQueue->m_Queue.m_PacketTree.f_Remove(pPacket);
							++pQueue->m_Queue.m_nAckedPackets;
							pQueue->m_Queue.m_Packet_ResendCheckQueue.InsertTail(pPacket);
							if ((++pQueue->m_Queue.m_nUnackedPackets)*4 > m_MaxPacketSize)
							{
								if (m_pParentConnection)
									m_pParentConnection->m_bForceSendUpdate = true;
								else
									m_bForceSendUpdate = true;
							}
							return true;
						}
					}
				}
			}
		}

		void QueueIncomingMessage(int _iChannel, const void *_pPacket, mint _Size, int _PacketID)
		{
			CReceiveChannelHandler::CChannel *pQueue = m_ChannelsReceive.GetChannel(_iChannel);

			if (!pQueue)
				return;

			{
				// Insert new packets
				if (_PacketID < 0)
				{
					CDataQueue::CPacket *pPacket = pQueue->m_Queue.PushPacket(_pPacket, _Size, _PacketID);
					// TODO: Only Update
					if (pPacket)
					{
						if (!pQueue->m_Link.IsInList())
						{
							m_ChannelsReceive.InsertAvailable(pQueue);
						}
						{
							M_LOCK(pQueue->m_Queue.m_QueueLock);
							pQueue->m_Queue.m_PacketQueue.InsertTail(pPacket);
							TRACEME("(%x)Inc nQueued 2 (%d)\n", MRTC_SystemInfo::OS_GetProcessID(), pQueue->m_Queue.m_nQueuedPackets.Get());
							++pQueue->m_Queue.m_nQueuedPackets;
#ifdef CHECKQUEUES
							pQueue->m_Queue.CheckQueues();
#endif
						}
					}

					if (ChangeConnectionStatus(ENetworkPacketStatus_Receive, 0))
					{
						if (m_pReportTo)
						{
	//						M_TRACEALWAYS("(%x) Signalling receive\n", MRTC_SystemInfo::OS_GetProcessID());
							m_pReportTo->Signal();
						}
					}				
				}
				else
				{
					{
						CDataQueue::CPacket *pPacket = pQueue->m_Queue.m_PacketTree.FindEqual(_PacketID);

						if (!pPacket)
						{
							int CurrentID = pQueue->m_Queue.GetNextPacketID();
							// Check for packets that have propably already been received and just requeue them for acksending
							int Diff = CurrentID - _PacketID;

							if ((Diff <= 0 && Diff > -32768) || Diff > 32768)
							{
								while (1)
								{
									if (CurrentID == _PacketID)
									{
										pPacket = pQueue->m_Queue.PushPacket(_pPacket, _Size, _PacketID);
										if (pPacket)
										{
											pPacket->m_bAckedOrReceived = true;
											pQueue->m_Queue.m_PacketSentQueue.InsertTail(pPacket);
											pQueue->m_Queue.m_Packet_ResendCheckQueue.InsertTail(pPacket);
											++pQueue->m_Queue.m_nAckedPackets;
											pQueue->m_Queue.m_PacketSize += pPacket->m_Size;
											if ((++pQueue->m_Queue.m_nUnackedPackets)*4 > m_MaxPacketSize)
											{
												if (m_pParentConnection)
													m_pParentConnection->m_bForceSendUpdate = true;
												else
													m_bForceSendUpdate = true;
											}
										}
										else
										{
											--CurrentID;
											if (CurrentID < 0)
												CurrentID = (1 << 16) - 1;
										}
										break;
									}
									else
									{
										pPacket = pQueue->m_Queue.PushPacket(NULL, m_MaxPacketSize, CurrentID);
										if (!pPacket)
										{
											--CurrentID;
											if (CurrentID < 0)
												CurrentID = (1 << 16) - 1;
											break;
										}
										pQueue->m_Queue.m_PacketTree.f_Insert(pPacket);
										pQueue->m_Queue.m_PacketSentQueue.InsertTail(pPacket);
									}
									++CurrentID;
									if (CurrentID >= (1 << 16))
										CurrentID = 0;
								}
								pQueue->m_Queue.m_LastPacketID = CurrentID;
							}
							else
							{
//								M_TRACEALWAYS("Resending ack for packet %d CurrentID: %d\n", _PacketID, CurrentID);
//								pQueue->m_Queue.TraceQueues();

								pPacket = pQueue->m_Queue.PushPacket(_PacketID);
								if (pPacket)
								{
									pQueue->m_Queue.m_Packet_ResendCheckQueue.InsertTail(pPacket);
									if ((++pQueue->m_Queue.m_nUnackedPackets)*4 > m_MaxPacketSize)
									{
										if (m_pParentConnection)
											m_pParentConnection->m_bForceSendUpdate = true;
										else
											m_bForceSendUpdate = true;
									}
								}
							}
						}
						else 
						{					
							M_ASSERT(!pPacket->m_bAckedOrReceived, "");
							memcpy(pQueue->m_Queue.m_Heap.GetBasePtr() + pPacket->m_Memory, _pPacket, _Size);
							pPacket->m_Size = _Size;
							pPacket->m_bAckedOrReceived = true;
//							pQueue->m_Queue.m_PacketSentQueue.InsertTail(pPacket);
							++pQueue->m_Queue.m_nAckedPackets;
							pQueue->m_Queue.m_PacketSize += _Size;
							pQueue->m_Queue.m_Packet_ResendCheckQueue.InsertTail(pPacket);
							pQueue->m_Queue.m_PacketTree.f_Remove(pPacket);
							if ((++pQueue->m_Queue.m_nUnackedPackets)*4 > m_MaxPacketSize)
							{
								if (m_pParentConnection)
									m_pParentConnection->m_bForceSendUpdate = true;
								else
									m_bForceSendUpdate = true;
							}
						}
					}
				}
			}
		}

		class CSendChannelHandler
		{
		public:
			class CChannel
			{
			public:
				CDataQueue m_Queue;
				fp4 m_Priority;
				fp4 m_MaxRate;
				uint32 m_bReliable:1;
				uint32 m_bOutOfOrder:1;
				MRTC_CriticalSection m_StuffedLock;
				int m_Stuffed;
			};

			TThinArray<CChannel *> m_Channels;			
			uint8 m_ChannelPrio[16];

			~CSendChannelHandler()
			{
				for (int i = 0; i < m_Channels.Len(); ++i)
				{
					if (m_Channels[i])
						delete m_Channels[i];
				}
			}

			int CompareChannel(int _iFirst, int _iSecond)
			{
				if (_iFirst >= m_Channels.Len())
				{
					if (_iSecond >= m_Channels.Len())
						return 0;
					else
						return 1;
				}
				else
				{
					if (_iSecond >= m_Channels.Len())
						return -1;
					else
					{
						if (!m_Channels[_iFirst])
						{
							if (!m_Channels[_iSecond])
								return 0;
							else
								return 1;
						}						
						else
						{
							if (m_Channels[_iSecond]->m_Priority < 0)
								return -1;
							else
							{
								if (m_Channels[_iSecond]->m_Priority < m_Channels[_iFirst]->m_Priority)
									return 1;
								else if (m_Channels[_iSecond]->m_Priority > m_Channels[_iFirst]->m_Priority)
									return -1;
								else
									return 0;
							}
						}
					}
				}
			}

			void SortChannels()
			{
				for (int i = 0; i < 16; ++i)
				{
					m_ChannelPrio[i] = i;
				}

				for (int i = 0; i < 15; i++)
				{
					for (int j = i+1; j < 16; j++)
					{
						if (CompareChannel(m_ChannelPrio[i], m_ChannelPrio[j]) > 0)
						{
							 Swap(m_ChannelPrio[i], m_ChannelPrio[j]);
						}
					}
				}
			}

			int ChannelAlloc(fp4 _Priority, fp4 _MaxRate, bint _bReliable, bint _bOutOfOrder)
			{
				M_ASSERT(!_bReliable || (_bReliable && !_bOutOfOrder), "");
				int Length = m_Channels.Len();

				for (int i = 0; i < Length; ++i)
				{
					if (!m_Channels[i])
					{
						m_Channels[i] = DNew(CChannel) CChannel;
						m_Channels[i]->m_Priority = _Priority;
						m_Channels[i]->m_MaxRate = _MaxRate;
						m_Channels[i]->m_bReliable = _bReliable;
						m_Channels[i]->m_bOutOfOrder = _bOutOfOrder;
						m_Channels[i]->m_Stuffed = 0;
						m_Channels[i]->m_Queue.SetHeapSize(DDefaultHeapSize);
						m_Channels[i]->m_Queue.SetNumPackets(DDefaultNumPackets);

						SortChannels();

						return i;
					}
				}
				if (Length >= 16)
					return NULL;

				m_Channels.SetLen(Length + 1);

				m_Channels[Length] = DNew(CChannel) CChannel;
				m_Channels[Length]->m_Priority = _Priority;
				m_Channels[Length]->m_MaxRate = _MaxRate;
				m_Channels[Length]->m_bReliable = _bReliable;
				m_Channels[Length]->m_bOutOfOrder = _bOutOfOrder;
				m_Channels[Length]->m_Stuffed = 0;
				m_Channels[Length]->m_Queue.SetHeapSize(DDefaultHeapSize);
				m_Channels[Length]->m_Queue.SetNumPackets(DDefaultNumPackets);

				SortChannels();

				return Length;
			}

			void ChannelFree(int _iChannel)
			{
				delete m_Channels[_iChannel];
				m_Channels[_iChannel] = NULL;
			}

			CChannel *GetChannel(int _iChannel)
			{
                if (_iChannel >= m_Channels.Len())
					return NULL;
				return m_Channels[_iChannel];
			}

		};

		CSendChannelHandler m_ChannelsSend;

		bint QueueOutgoingMessage(int _iChannel, const void *_pPacket, mint _Size, CNetwork_Address *_pAddress)
		{
			CSendChannelHandler::CChannel *pQueue = m_ChannelsSend.GetChannel(_iChannel);

			if (!pQueue)
				return false;
			
			int NextPacketID = pQueue->m_Queue.GetNextPacketID();
			CDataQueue::CPacket *pPacket = pQueue->m_Queue.PushPacket(_pPacket, _Size, NextPacketID);
			if (pPacket)
			{
				pQueue->m_Queue.m_LastPacketID = NextPacketID;
				pPacket->m_pAddress = _pAddress;
				{
					M_LOCK(pQueue->m_Queue.m_QueueLock);
					pQueue->m_Queue.m_PacketQueue.InsertTail(pPacket);
					TRACEME("(%x)Inc nQueued 3 (%d)\n", MRTC_SystemInfo::OS_GetProcessID(), pQueue->m_Queue.m_nQueuedPackets.Get());
					++pQueue->m_Queue.m_nQueuedPackets;
#ifdef CHECKQUEUES
					pQueue->m_Queue.CheckQueues();
#endif
				}
				m_bForceSendUpdate = true;

				return true;
			}

			M_LOCK(pQueue->m_StuffedLock);
			if (Volatile(pQueue->m_Stuffed) == 0)
			{
				m_bForceFlushSend = true;
				Volatile(pQueue->m_Stuffed) = 1;
				m_pNet->m_MainEvent.Signal();
			}
			return false;
		}

		bint QueueOutgoingInternalMessage(int _iChannel, const void *_pPacket, mint _Size, CNetwork_Address *_pAddress)
		{
			CSendChannelHandler::CChannel *pQueue = m_ChannelsSend.GetChannel(_iChannel);

			if (!pQueue)
				return false;
			
			int NextPacketID = pQueue->m_Queue.GetNextPacketID();
			CDataQueue::CPacket *pPacket = pQueue->m_Queue.PushPacket(_pPacket, _Size, NextPacketID);
			if (pPacket)
			{
				pPacket->m_pAddress = _pAddress;
				pPacket->m_bInternal = true;
				pQueue->m_Queue.m_LastPacketID = NextPacketID;
				{
					M_LOCK(pQueue->m_Queue.m_QueueLock);
					pQueue->m_Queue.m_PacketQueue.InsertTail(pPacket);
					TRACEME("(%x)Inc nQueued 4 (%d)\n", MRTC_SystemInfo::OS_GetProcessID(), pQueue->m_Queue.m_nQueuedPackets.Get());
					++pQueue->m_Queue.m_nQueuedPackets;
#ifdef CHECKQUEUES
					pQueue->m_Queue.CheckQueues();
#endif
				}

				return true;
			}
			return false;
		}

		mint CalulateQueuedData()
		{
			int ReceiveChannels = m_ChannelsReceive.m_Channels.Len();
			mint Size = 0;
			// Build a packet and send it
			for (int i = 0; i < 16; ++i)
			{
				// Loop through all channels and add data
				int iCh = m_ChannelsSend.m_ChannelPrio[i];

				bint bSend = false;
                if (iCh < ReceiveChannels && m_ChannelsReceive.m_Channels[iCh])
				{					
					CDataQueue *pReceiveQueue = &m_ChannelsReceive.m_Channels[iCh]->m_Queue;

					bSend = true;
					if (pReceiveQueue->m_nUnackedPackets)
						Size += pReceiveQueue->m_nUnackedPackets*2 + 2;
				}

				CSendChannelHandler::CChannel *pChn = m_ChannelsSend.GetChannel(iCh);
				/*
				if (pChn || iResend > 0 || iAck > 0)
				{
					Size += 1;
					if (iResend)
						Size += 2;
					if (iAck)
						Size += 2;
				}*/

				if (pChn && !pChn->m_bOutOfOrder)
				{
					if (pChn->m_bReliable)
						Size += 2; // ThisID

					bSend = true;
					Size += pChn->m_Queue.m_QueuedSize; // Data
					Size += pChn->m_Queue.m_nQueuedPackets.Get()*2; // Data headers
				}

				if (bSend)
					Size += 1; // Channel header
			}            

			return Size;
		}

		uint8 m_TempSend[ENetworkMaxPacketSize + 32];
		CNetwork_Packet m_TempSendPacket;
		NThread::CAtomicInt m_bForceFlushSend;
		NThread::CAtomicInt m_bForceSendUpdate;

		
		void FlushSend()
		{
			m_bForceFlushSend.Exchange(1);
		}

		void Internal_FlushSend(bint _bForce)
		{
			mint Data = 0;
			if (m_bStuffed && m_TempSendPacket.m_pAddress)
			{
				m_bStuffed = !m_pSocket->Send(m_TempSendPacket);

				if (m_bStuffed)
					return;
				else
					m_TempSendPacket.m_pAddress = NULL;
			}

			if (!_bForce)
			{
				Data = CalulateQueuedData();
				if (Data < m_MaxPacketSize)
					return;
			}

			// Send out of order packets

			{
				// Build a packet and send it
				for (int i = 0; i < 16; ++i)
				{
					// Loop through all channels and add data
					int iCh = m_ChannelsSend.m_ChannelPrio[i];

					CSendChannelHandler::CChannel *pChn = m_ChannelsSend.GetChannel(iCh);
					if (pChn && pChn->m_bOutOfOrder && pChn->m_Queue.m_nQueuedPackets.Get())
					{
						uint8 *pPacket = m_TempSend;
						uint8 *pPacketStart = pPacket;

						*pPacket = m_ConnectionID; ++pPacket;
						uint8 *pChannelHeader = pPacket;
						*pChannelHeader = iCh | DBit(7); ++pPacket;

	//					uint16 *pLastHeader = NULL;
						while (pChn->m_Queue.m_nQueuedPackets.Get())
						{
							mint Size = pPacket - pPacketStart;
							CDataQueue::CPacket *pHeadPacket = pChn->m_Queue.GetHeadPacket();
							if (Size + pHeadPacket->m_Size + 2 > m_MaxPacketSize)
							{
								M_TRACEALWAYS("Threw away out of order packet not fitting in packet size\n");
								pChn->m_Queue.PopHeadPacket();
								continue;
							}

							uint16 Header = pHeadPacket->m_Size | DBit(15);
							if (pHeadPacket->m_bInternal)
								Header |= DBit(14);
							SwapLE(Header);
							*((uint16 *)pPacket) = Header; pPacket += 2;
							memcpy(pPacket, pChn->m_Queue.m_Heap.GetBasePtr() + pHeadPacket->m_Memory, pHeadPacket->m_Size);
							pPacket += pHeadPacket->m_Size;

							m_TempSendPacket.m_pAddress = pHeadPacket->m_pAddress;
							m_TempSendPacket.m_pData = m_TempSend;
							m_TempSendPacket.m_Size = pPacket - pPacketStart;
							m_bStuffed = !m_pSocket->Send(m_TempSendPacket);
							m_TempSendPacket.m_pAddress = NULL;

							if (m_bStuffed)
								return;

							pChn->m_Queue.PopHeadPacket();

							// Report that more packets can be sent
							M_LOCK(pChn->m_StuffedLock);
							if (Volatile(pChn->m_Stuffed))
							{
								mint Heap = Volatile(pChn->m_Queue.m_HeapSpace);
								if (Volatile(pChn->m_Stuffed) && Heap >= pChn->m_Queue.m_Heap.Len() / 2)
								{
									Volatile(pChn->m_Stuffed) = 0;
									if (ChangeConnectionStatus(ENetworkPacketStatus_Send, 0))
									{
//										M_TRACEALWAYS("(%x) Signaled Unstuffed 1\n", MRTC_SystemInfo::OS_GetProcessID());
										if (m_pReportTo)
											m_pReportTo->Signal();
									}
								}
							}
						}
					}
				}
			}

			// Send usual packets
			int ReceiveChannels = m_ChannelsReceive.m_Channels.Len();
			bint bWork = true;
			while (bWork)
			{
				bWork = false;

				uint8 *pPacket = m_TempSend;
				uint8 *pPacketStart = pPacket;

				*pPacket = m_ConnectionID; ++pPacket;
				bint bSend = false;
				
				// Build a packet and send it
				for (int i = 0; i < 16; ++i)
				{
					// Loop through all channels and add data
					int iCh = m_ChannelsSend.m_ChannelPrio[i];

					CDataQueue *pReceiveQueue = NULL;
					CReceiveChannelHandler::CChannel *pReceiveChannel = NULL;
					bint bForceSend = false;
					if (iCh < ReceiveChannels && m_ChannelsReceive.m_Channels[iCh])
					{
						pReceiveQueue = &m_ChannelsReceive.m_Channels[iCh]->m_Queue;
						pReceiveChannel = m_ChannelsReceive.m_Channels[iCh];
						if (pReceiveQueue->m_nUnackedPackets)
							bForceSend = true;
					}

					CSendChannelHandler::CChannel *pChn = m_ChannelsSend.GetChannel(iCh);
					if (pChn && pChn->m_bOutOfOrder)
						pChn = NULL;

					bint bQueuedPackets = false;
					if (pChn)
						bQueuedPackets = pChn->m_Queue.m_nQueuedPackets.Get();
					if (bQueuedPackets || bForceSend)
					{
						uint8 *pChannelHeader = pPacket;
						*pChannelHeader = iCh; ++pPacket;

						if (pReceiveQueue && pReceiveQueue->m_nUnackedPackets)
						{
							mint Size = pPacket - pPacketStart;
							int nRoomFor = ((aint)(m_MaxPacketSize - Size - 2))/2;
							int nAck = Min(nRoomFor, pReceiveQueue->m_nUnackedPackets);

							if (nRoomFor > 0)
							{
								uint16 Temp = nAck;
								SwapLE(Temp);

								*((uint16 *)pPacket) = Temp; pPacket += 2;

								bSend = true;
								*pChannelHeader |= DBit(6);

								while (nRoomFor > 0)
								{
									CDataQueue::CPacket *pPack = pReceiveQueue->m_Packet_ResendCheckQueue.Pop();
									if (!pPack)
										break;

									--pReceiveQueue->m_nUnackedPackets;
									--nRoomFor;

									uint16 ID = pPack->m_PacketID;
									SwapLE(ID);
									*((uint16 *)pPacket) = ID; pPacket += 2;

									if (!pPack->m_Link.IsInList()) // This is a ack only packet don't do anything with it
									{
										M_ASSERT(!pPack->m_AllocSize, "");
										M_LOCK(pReceiveQueue->m_QueueLock);
										pReceiveQueue->m_FreePackets.Insert(pPack);
									}
								}

								// Check for packets available for consumption
								bint bAvailable = false;
								while (1)
								{
									CDataQueue::CPacket *pPack = pReceiveQueue->m_PacketSentQueue.GetFirst();

									if (!pPack)
										break;

									if (!pPack->m_bAckedOrReceived || pPack->m_LinkResend.IsInList())
									{
										break;
									}
									else
									{
										bAvailable = true;
										M_LOCK(pReceiveQueue->m_QueueLock);
										pReceiveQueue->m_PacketQueue.InsertTail(pPack);
										TRACEME("(%x)Inc nQueued 5 (%d)\n", MRTC_SystemInfo::OS_GetProcessID(), pReceiveQueue->m_nQueuedPackets.Get());
										++pReceiveQueue->m_nQueuedPackets;
#ifdef CHECKQUEUES
										pReceiveQueue->CheckQueues();
#endif

									}
								}

								if (bAvailable)
								{
									if (!pReceiveChannel->m_Link.IsInList())
									{
										m_ChannelsReceive.InsertAvailable(pReceiveChannel);
									}
									if (ChangeConnectionStatus(ENetworkPacketStatus_Receive, 0))
									{
										if (m_pReportTo)
										{
					//						M_TRACEALWAYS("(%x) Signalling receive\n", MRTC_SystemInfo::OS_GetProcessID());
											m_pReportTo->Signal();
										}
									}				
								}
							}

						}

						if (pChn && bQueuedPackets)
						{
#ifdef CHECKQUEUES
							pChn->m_Queue.CheckQueues();
#endif
							if (pChn->m_bReliable)
							{
								*pChannelHeader |= DBit(5);
							}

							uint16 *pLastHeader = NULL;
							while (1)
							{
								CDataQueue::CPacket *pHeadPacket = pChn->m_Queue.GetHeadPacket();
								if (!pHeadPacket)
									break;
								mint Size = pPacket - pPacketStart;
								if (Size + pHeadPacket->m_Size + 2 + pChn->m_bReliable * 2 > m_MaxPacketSize)
								{
									bWork = true;
									break;
								}
								uint16 Header = pHeadPacket->m_Size;
								if (pHeadPacket->m_bInternal)
									Header |= DBit(14);
								pLastHeader = (uint16 *)pPacket; pPacket += 2;

								SwapLE(Header);
								*pLastHeader = Header;

								if (pChn->m_bReliable)
								{
									uint16 ID = pHeadPacket->m_PacketID;
									SwapLE(ID);
									*((uint16 *)pPacket) = ID; pPacket += 2;
								}

								memcpy(pPacket, pChn->m_Queue.m_Heap.GetBasePtr() + pHeadPacket->m_Memory, pHeadPacket->m_Size);
								pPacket += pHeadPacket->m_Size;

								if (pChn->m_bReliable)
									pChn->m_Queue.PushHeadPacketToSentQueue();
								else
								{
									pChn->m_Queue.PopHeadPacket();

									// Report that more packets can be sent
									M_LOCK(pChn->m_StuffedLock);
									if (Volatile(pChn->m_Stuffed))
									{
										mint Heap = Volatile(pChn->m_Queue.m_HeapSpace);
										if (Volatile(pChn->m_Stuffed) && Heap >= pChn->m_Queue.m_Heap.Len() / 2)
										{
											Volatile(pChn->m_Stuffed) = 0;
											if (ChangeConnectionStatus(ENetworkPacketStatus_Send, 0))
											{
//													M_TRACEALWAYS("(%x) Signaled Unstuffed 1\n", MRTC_SystemInfo::OS_GetProcessID());
												if (m_pReportTo)
													m_pReportTo->Signal();
											}
										}
									}
								}
							}

							if (pLastHeader)
							{
								bSend = true;
								*pChannelHeader |= DBit(7); // We are sending data with this channel
								uint16 Header = *pLastHeader;
								SwapLE(Header);
								Header |= DBit(15);
								SwapLE(Header);
								*pLastHeader = Header;
							}
						}

						// The data didn't fit, lets forgeddaboutit
						mint Size = pPacket - pPacketStart;
						if (Size > m_MaxPacketSize)
						{
							bWork = true;
							pPacket = pChannelHeader;
						}
					}

				}

				if (bSend && m_pConnectedAddress)
				{
					m_TempSendPacket.m_pAddress = m_pConnectedAddress;
					m_TempSendPacket.m_pData = m_TempSend;
					m_TempSendPacket.m_Size = pPacket - pPacketStart;
					m_bStuffed = !m_pSocket->Send(m_TempSendPacket);

					if (m_bStuffed)
						return;
				}

				if (!_bForce && Data < m_MaxPacketSize)
					break;
			}



		}

		uint32 m_bStuffed;

		virtual bool Update()
		{
			return Update(0);
			// Do nothing as default
		}

		uint8 m_TempBuffer_Receive[ENetworkMaxPacketSize];
		CNetwork_Address *m_pTempAddress_Receive;
		CNetwork_Address *m_pConnectedAddress;
		TThinArray<int> m_RoutingTable;
		CIDHeap m_RoutingIDs;

		void PacketReceive(const CNetwork_Packet &_Packet)
		{
			// TODO: Check address is the same

//			M_TRACEALWAYS("0x%08x: Packet receive %d\n", MRTC_SystemInfo::OS_GetProcessID(), _Packet.m_Size);
			if (m_pConnectedAddress && m_pConnectedAddress->Compare(*_Packet.m_pAddress) != 0)
			{
				M_TRACEALWAYS("0x%08x: Threw away packet\n", MRTC_SystemInfo::OS_GetProcessID());
				return;
			}

			bint bUpdateLastReceive = true;

			CMTime Now = CMTime::GetCPU();

			uint8 *pPacket = (uint8 *)_Packet.m_pData;
			uint8 *pPacketEnd = pPacket + _Packet.m_Size;
			++pPacket; // Connection
			while (pPacket < pPacketEnd)
			{
				int H1 = *pPacket; ++pPacket;
				int iChannel = H1 & DBitRange(0,4);

				bint bReliable = H1 & DBit(5);
				int nAck = 0;
				if (H1 & DBit(6))
				{
					uint16 Temp = *((uint16 *)pPacket); pPacket += 2;
					SwapLE(Temp);
					nAck = Temp;
				}

				if (nAck) // Normal Acks
				{
					CSendChannelHandler::CChannel *pChn = m_ChannelsSend.GetChannel(iChannel);
					if (pChn)
					{
//						if (nAck)
//							M_TRACEALWAYS("0x%08x: Received %d acks\n", MRTC_SystemInfo::OS_GetProcessID(), nAck);

						while (nAck)
						{
							uint16 ID = *((uint16 *)pPacket); pPacket += 2;
							SwapLE(ID);

							CDataQueue::CPacket *pPack = pChn->m_Queue.m_PacketTree.FindEqual(ID);

							if (pPack)
							{
								pPack->m_bAckedOrReceived = true;

								// Remove from resend queue and packet tree
								if (pPack->m_bResend)
								{
									M_ASSERT(pPack->m_LinkResend.IsInList(), "");
									pPack->m_bResend = false;
									--pChn->m_Queue.m_nQueuedPackets;
								}
								pChn->m_Queue.m_PacketTree.f_Remove(pPack);
								pPack->m_LinkResend.Unlink();

								// Add ping information
								m_PingQueue.AddPing((Now - pPack->m_LastSendTime).GetTime(),Now);
							}

							--nAck;
						}

						while (1)
						{
							CDataQueue::CPacket *pPack = pChn->m_Queue.GetHeadPacketSent();

							if (pPack && pPack->m_bAckedOrReceived)
								pChn->m_Queue.PopHeadSentPacket();
							else
								break;
						}

						// Report that more packets can be sent
						{
							M_LOCK(pChn->m_Queue.m_QueueLock);
							{
								M_LOCK(pChn->m_StuffedLock);
								if (Volatile(pChn->m_Stuffed))
								{
									mint Heap = Volatile(pChn->m_Queue.m_HeapSpace);
									if (Volatile(pChn->m_Stuffed) && Heap >= pChn->m_Queue.m_Heap.Len() / 2)
									{
										Volatile(pChn->m_Stuffed) = 0;
										if (ChangeConnectionStatus(ENetworkPacketStatus_Send, 0))
										{
		//													M_TRACEALWAYS("(%x) Signaled Unstuffed 1\n", MRTC_SystemInfo::OS_GetProcessID());
											if (m_pReportTo)
												m_pReportTo->Signal();
										}
									}
								}
							}
						}
					}
				}

				bint bHasData = H1 & DBit(7);

				bint bContinue = bHasData;
				while (pPacket < pPacketEnd && bContinue)
				{

					uint16 H1 = *((uint16 *)pPacket); pPacket += 2;
					SwapLE(H1);

					int iPacket = -1;
					if (bReliable)
					{
						uint16 ID = *((uint16 *)pPacket); pPacket += 2;
						SwapLE(ID);
						iPacket = ID;
					}

					int Size = H1 & DBitRange(0,13);
					if (pPacket + Size > pPacketEnd)
						break; // Size error

					bint bInternalData = H1 & DBit(14);
					bContinue = !(H1 & DBit(15));

					if (bInternalData && QueueIncomingInternalMessage(iChannel, iPacket))
					{
						uint8 *pInternal = pPacket;
						uint8 *pInternalEnd = pInternal + Size;
						uint8 Message = *pInternal; ++pInternal;
						switch (Message)
						{
						case EMessageInternal_Query:
							{
								if (m_pParentConnection)
								{
									CNetwork_ServerInfo Info;
									if (m_pParentConnection->GetServerInfo(Info))
									{
										uint8 Temp[ENetworkMaxServerInfoSize+32+4+1];
										uint8 *pDest = Temp;
										*pDest = EMessageInternal_ConnectionInfo; ++pDest;
										memcpy(pDest, Info.m_Name, 32); pDest += 32;
										*((uint32 *)pDest) = Info.m_InfoSize;
										SwapLE(*((uint32 *)pDest));
										pDest += 4;
										memcpy(pDest, Info.m_ServerInfo, Info.m_InfoSize); pDest += Info.m_InfoSize;

										QueueOutgoingInternalMessage(m_iChannel_InternalReliable, Temp, pDest - Temp, NULL);
									}
								}
								else if (m_ConnectionType == EConnectionType_Server)
								{
									CNetwork_ServerInfo Info;

									if (_Packet.m_pAddress && GetServerInfo(Info))
									{
										CNetwork_Address *pAddress = m_pNet->m_spDevice->Address_Alloc();
										pAddress->Copy(*_Packet.m_pAddress);
										uint8 Temp[ENetworkMaxServerInfoSize+32+4+1];
										uint8 *pDest = Temp;
										*pDest = EMessageInternal_ConnectionInfo; ++pDest;
										memcpy(pDest, Info.m_Name, 32); pDest += 32;
										*((uint32 *)pDest) = Info.m_InfoSize;
										SwapLE(*((uint32 *)pDest));
										pDest += 4;
										memcpy(pDest, Info.m_ServerInfo, Info.m_InfoSize); pDest += Info.m_InfoSize;

										if (!QueueOutgoingInternalMessage(m_iChannel_InternalUnreliable, Temp, pDest - Temp, pAddress))
										{
											pAddress->Delete();
										}                                        
									}
								}
							}
							break;
						case EMessageInternal_ConnectionInfo:
							{
								if (m_ConnectionType == EConnectionType_Client)
								{
									CNetwork_ServerInfo Info;

									uint8 *pDest = pInternal;
									if ((pDest + 36) <= pInternalEnd)
									{
										memcpy(Info.m_Name, pDest, 32); pDest += 32;
										Info.m_InfoSize = *((uint32 *)pDest); pDest += 4;
										SwapLE(Info.m_InfoSize);										
										if ((pDest + Info.m_InfoSize) <= pInternalEnd)
										{
											memcpy(Info.m_ServerInfo, pDest, Info.m_InfoSize); pDest += Info.m_InfoSize;
											M_ASSERT(pDest == pInternalEnd, "Should match");
											if (!m_pConnectedAddress)
											{
												Info.m_Address = _Packet.m_pAddress->GetStr();
												QueueServerInfo(&Info);
											}
											else
												SetServerInfo(Info);
										}
									}
								}
							}
							break;
						case EMessageInternal_Connect:
							{
								M_TRACEALWAYS("Received EMessageInternal_Connect\n");
								if (m_ConnectionType == EConnectionType_Server)
								{
									int iConn = m_pNet->AllocID();

									bint bFailed = false;

									if (iConn < 0)
									{
										bFailed = true;
									}
									else
									{
										int iRoute = m_RoutingIDs.AllocID();

										if (iRoute < 0)
										{
											bFailed = true;
											m_pNet->FreeID(iConn);
										}
										else
										{
											if (iRoute >= m_RoutingTable.Len())
											{
												int LastLen = m_RoutingTable.Len();
												m_RoutingTable.SetLen(iRoute + 1);

												for (int i = LastLen; i < m_RoutingTable.Len(); ++i)
													m_RoutingTable[i] = -1;
											}
											m_RoutingTable[iRoute] = iConn;											

											M_TRACEALWAYS("Alloc route: %d %d\n", iRoute, iConn);

											spCConnection spConn = MNew1(CConnection, m_pNet);
											spConn->m_pSocket = m_pSocket; // Same socket as us										
											spConn->m_pParentConnection = this;
											spConn->m_iParentRoute = iRoute;
											spConn->m_pConnectedAddress = m_pNet->m_spDevice->Address_Alloc();
											spConn->m_pConnectedAddress->Copy(*_Packet.m_pAddress);
											{
												M_LOCK(m_pNet->m_ConnectionsLock);
												m_pNet->m_splConnections[iConn] = spConn;
												m_pNet->m_Connections.Insert(spConn);
											}

											{
												M_LOCK(m_AcceptLock);
												m_Available.Insert(spConn);
											}

											if (ChangeConnectionStatus(ENetworkPacketStatus_Connection, 0))
											{
												if (m_pReportTo)
													m_pReportTo->Signal();
											}
										}
									}

									if (bFailed)
									{
										CNetwork_Address *pAddress = m_pNet->m_spDevice->Address_Alloc();
										pAddress->Copy(*_Packet.m_pAddress);
										uint8 Temp[1];
										Temp[0] = EMessageInternal_Refuse;

										if (!QueueOutgoingInternalMessage(m_iChannel_InternalUnreliable, Temp, 1, pAddress))
										{
											pAddress->Delete();
										}
									}
								}
								else
								{
									if (!m_pConnectedAddress)
									{
										m_ConnectionID = *pInternal;
										m_pConnectedAddress = m_pNet->m_spDevice->Address_Alloc();
										m_pConnectedAddress->Copy(*_Packet.m_pAddress);
										ChangeConnectionStatus(ENetworkPacketStatus_Connected, ENetworkPacketStatus_Closed);
									}
									else
										bUpdateLastReceive = false;
								}
							}
							break;
						case EMessageInternal_Disconnect:
							{
								M_TRACEALWAYS("Received EMessageInternal_Disconnect\n");
								if (m_ConnectionType != EConnectionType_Server)
								{
									ChangeConnectionStatus(ENetworkPacketStatus_Closed, ENetworkPacketStatus_Connected);
									if (m_pReportTo)
										m_pReportTo->Signal();
									if (m_pConnectedAddress)
									{
										m_pConnectedAddress->Delete();
										m_pConnectedAddress = NULL;
									}
								}
							}
							break;
						case EMessageInternal_Refuse:
							{
								if (m_ConnectionType != EConnectionType_Server && !m_pConnectedAddress)
								{
									M_TRACEALWAYS("Received EMessageInternal_Refuse\n");
									ChangeConnectionStatus(ENetworkPacketStatus_Closed, ENetworkPacketStatus_Connected);
									if (m_pReportTo)
										m_pReportTo->Signal();
									if (m_pConnectedAddress)
									{
										m_pConnectedAddress->Delete();
										m_pConnectedAddress = NULL;
									}
								}
								else
									bUpdateLastReceive = false;

							}
							break;
						}
					}
					else
					{
						QueueIncomingMessage(iChannel, pPacket, Size, iPacket);
											
					}

					pPacket += Size;
				}
				
			}

			if (bUpdateLastReceive)
				m_Time_LastReceive = Now;
		}

		void UpdateReceive()
		{			
			while (1)
			{
				CNetwork_Packet Packet;
				Packet.m_pData = m_TempBuffer_Receive;
				Packet.m_Size = ENetworkMaxPacketSize;
				Packet.m_iChannel = 0;
				Packet.m_pAddress = m_pTempAddress_Receive;
				if (!m_pSocket->Receive(Packet))
					break;

				if (Packet.m_Size < 4) // We need at least 4 byte header
					continue;

				uint8 *pPacket = m_TempBuffer_Receive;

				int Connection = *pPacket; ++pPacket;

				if (Connection == 255)
				{
					// Ment for this connection
					PacketReceive(Packet);
				}
				else
				{
					// Ment for another connection
					if (Connection < m_RoutingTable.Len())
					{
						int iConnection = m_RoutingTable[Connection];
						if (iConnection >= 0)
						{
							CConnection *pConn = m_pNet->GetConnection(iConnection);
							if (pConn)
								pConn->PacketReceive(Packet);							

//							pConn->Update();
						}
					}
				}

			}
		}

		int Accept()
		{
			M_LOCK(m_AcceptLock);

			CConnection *pConn = m_Available.Pop();
			if (pConn)
			{
				uint8 Temp[2];
				Temp[0] = EMessageInternal_Connect;
				Temp[1] = pConn->m_iParentRoute;
				CNetwork_Address *pAddr = m_pNet->m_spDevice->Address_Alloc();
				pAddr->Copy(*pConn->m_pConnectedAddress);

				int iConn = m_RoutingTable[pConn->m_iParentRoute];
				M_ASSERT(iConn >= 0, "");
				if (!pConn->QueueOutgoingInternalMessage(pConn->m_iChannel_InternalUnreliable, Temp, 2, pAddr))
				{
					pAddr->Delete();

					CNetwork_Address *pAddress = m_pNet->m_spDevice->Address_Alloc();
					pAddress->Copy(*pConn->m_pConnectedAddress);
					uint8 Temp[1];
					Temp[0] = EMessageInternal_Refuse;

					if (!QueueOutgoingInternalMessage(m_iChannel_InternalUnreliable, Temp, 1, pAddress))
					{
						pAddress->Delete();
					}

					m_pNet->Connection_Close(iConn);
					m_pNet->m_MainEvent.Signal();

					return -1;
				}
				m_pNet->m_MainEvent.Signal();
				return iConn;
			}
			return -1;
		}

		void Refuse()
		{
			CConnection *pConn;
			{
				M_LOCK(m_AcceptLock);
				pConn = m_Available.Pop();
			}
			if (pConn)
			{
				int iConn = m_RoutingTable[pConn->m_iParentRoute];
				M_ASSERT(iConn >= 0, "");

				CNetwork_Address *pAddress = m_pNet->m_spDevice->Address_Alloc();
				pAddress->Copy(*pConn->m_pConnectedAddress);
				uint8 Temp[1];
				Temp[0] = EMessageInternal_Refuse;

				if (!QueueOutgoingInternalMessage(m_iChannel_InternalUnreliable, Temp, 1, pAddress))
				{
					pAddress->Delete();
				}

				m_pNet->Connection_Close(iConn);
				m_pNet->m_MainEvent.Signal();
			}
		}

		void UpdateSend()
		{
			Internal_FlushSend(false);
			for (int i = 0; i < m_RoutingTable.Len(); ++i)
			{
				if (m_RoutingTable[i] >= 0)
				{
					m_pNet->m_ConnectionsLock.Lock();
					CConnection *pConn;
					M_TRY
					{
						pConn = m_pNet->m_splConnections[m_RoutingTable[i]];
					}
					M_CATCH(
					catch (CCException)
					{
						m_pNet->m_ConnectionsLock.Unlock();
						throw;
					}
					)
					{
						M_LOCK(pConn->m_DeleteLock);
						m_pNet->m_ConnectionsLock.Unlock();
						pConn->Internal_FlushSend(false);
						pConn->UpdateResend();
					}
				}
			}
		}

		void UpdateResend()
		{
//			return;
			CMTime Now = CMTime::GetCPU();
			fp8 Ping = m_PingQueue.GetPing();
			CMTime TimeOut;
			if (Ping > 0.0)
				TimeOut = Now - CMTime::CreateFromSeconds(m_PingQueue.GetPing()*2.0);
			else
				TimeOut = Now - CMTime::CreateFromSeconds(0.5);

			for (int i = 0; i < 16; ++i)
			{
				CSendChannelHandler::CChannel *pChn = m_ChannelsSend.GetChannel(i);
				if (pChn && !pChn->m_bOutOfOrder)
				{
					while (1)
					{
						CDataQueue::CPacket *pPacket = pChn->m_Queue.m_Packet_ResendCheckQueue.GetFirst();

						if (!pPacket)
							break;

						if (pPacket->m_LastSendTime.Compare(TimeOut) < 0)
						{
	#ifdef CHECKQUEUES
							M_LOCK(pChn->m_Queue.m_QueueLock);/// ADDADAD
	#endif
							//M_TRACEALWAYS("(%x)Inc nQueued 7 (%d)\n", MRTC_SystemInfo::OS_GetProcessID(), pChn->m_Queue.m_nQueuedPackets.Get());
							pChn->m_Queue.m_Packet_ResendQueue.InsertTail(pPacket);
							pPacket->m_bResend = true;
							++pChn->m_Queue.m_nQueuedPackets;
							m_bForceFlushSend = true;
	#ifdef CHECKQUEUES
							pChn->m_Queue.CheckQueues();
	#endif
						}
						else
							break;					
					}
				}
			}
		}

		void UpdateMisc()
		{
			if ((CMTime::GetCPU() - m_Time_LastReceive).GetTime() > 10.0)
				ChangeConnectionStatus(ENetworkPacketStatus_Timeout, 0);
			else
				ChangeConnectionStatus(0, ENetworkPacketStatus_Timeout);

			if (m_ConnectionType == EConnectionType_Server)
			{
				bint bChanged = GetServerInfoChanged();
				if (bChanged)
				{
					CNetwork_ServerInfo Info;
					if (GetServerInfo(Info))
					{

						uint8 Temp[ENetworkMaxServerInfoSize+32+4+1];
						uint8 *pDest = Temp;
						*pDest = EMessageInternal_ConnectionInfo; ++pDest;
						memcpy(pDest, Info.m_Name, 32); pDest += 32;
						*((uint32 *)pDest) = Info.m_InfoSize;
						SwapLE(*((uint32 *)pDest));
						pDest += 4;
						memcpy(pDest, Info.m_ServerInfo, Info.m_InfoSize); pDest += Info.m_InfoSize;

						for (int i = 0; i < m_RoutingTable.Len(); ++i)
						{
							if (m_RoutingTable[i] >= 0)
							{
								m_pNet->m_ConnectionsLock.Lock();
								CConnection *pConn;
								M_TRY
								{
									pConn = m_pNet->m_splConnections[m_RoutingTable[i]];
								}
								M_CATCH(
								catch (CCException)
								{
									m_pNet->m_ConnectionsLock.Unlock();
									throw;
								}
								)
								{
									M_LOCK(pConn->m_DeleteLock);
									m_pNet->m_ConnectionsLock.Unlock();
									{
										if (!QueueOutgoingInternalMessage(m_iChannel_InternalReliable, Temp, pDest - Temp, NULL))
										{
											// TODO: Handle this somehow
										}
									}
								}
							}
						}
					}
				}
			}
		}

		CMTime m_LastForce;
		bool Update(uint32 _Status)
		{
			if (_Status & ENetworkPacketStatus_Receive)
			{
                UpdateReceive();
			}

			UpdateResend();

			int ForceSendUpdate = m_bForceSendUpdate.Exchange(0);

			if (_Status & ENetworkPacketStatus_Send || ForceSendUpdate)
			{
                UpdateSend();
			}

			UpdateMisc();

			int bForce = m_bForceFlushSend.Exchange(0);

			CMTime Now = CMTime::GetCPU();

			fp8 PacketSize = 0;
			fp8 BufferSize = 0;
			fp8 Acks = m_ChannelsReceive.UpdateAckStats(Now, PacketSize, BufferSize);
//			fp8 Scale = 1.0;
			fp8 UpdateTime = 0.05f;
			if (Acks > 0)
			{
//				UpdateTime = (BufferSize / PacketSize) / 16;
//				UpdateTime = m_MaxPacketSize / PacketSize * 4;
				UpdateTime = Min(m_MaxPacketSize / PacketSize * 2, (BufferSize / PacketSize) / 16);
//				UpdateTime = 4.0 / Acks;
//				(BufferSize / 4)
//				Scale = 7000.0 / (PacketSize / Acks);
				
//				int Fula = 0;
			}

			if (UpdateTime > 0.05f)
				UpdateTime = 0.05f;
			
			if (bForce || Now.Compare(m_LastForce + CMTime::CreateFromSeconds(UpdateTime)) > 0)
			{
				m_LastForce = Now;
                Internal_FlushSend(true);
			}
			return m_bStuffed != 0;
		}

		virtual bint GetServerInfo(CNetwork_ServerInfo &_Dest)
		{
			return false;
		}

		virtual void SetServerInfo(const CNetwork_ServerInfo &_Src)
		{

		}

		virtual void QueueServerInfo(CNetwork_ServerInfo *_pInfo)
		{
		}

		bint GetServerInfoChanged()
		{
			return false;
		}

	};

	typedef TPtr<CConnection> spCConnection;

	MRTC_CriticalSection m_ConnectionsLock;
	MRTC_CriticalSection m_ThreadUpdateLock;
	TList_Vector<spCConnection> m_splConnections;
	CIDHeap m_ConnectionIDs;
	DLinkD_List(CConnection, m_Link) m_Connections;

	class CServer : public CConnection
	{
	public:

		CServer(CNetworkCore *_pNet) : CConnection(_pNet)
		{
			m_ConnectionType = EConnectionType_Server;
			m_ServerInfoValid = false;
			m_bServerInfoChanged = false;
		}

		~CServer()
		{
		}

		MRTC_CriticalSection m_ServerInfoLock;
		CNetwork_ServerInfo m_ServerInfo;
		int m_ServerInfoValid;
		int m_bServerInfoChanged;
		bint GetServerInfo(CNetwork_ServerInfo &_Dest)
		{
			M_LOCK(m_ServerInfoLock);
			if (!m_ServerInfoValid)
				return false;
			_Dest = m_ServerInfo;
			return true;
		}

		bint GetServerInfoChanged()
		{
			M_LOCK(m_ServerInfoLock);
			bint bChanged = m_bServerInfoChanged;
			m_bServerInfoChanged = false;
			return bChanged;
		}

		void SetServerInfo(const CNetwork_ServerInfo &_Src)
		{
			M_LOCK(m_ServerInfoLock);
			m_ServerInfo = _Src;
			m_ServerInfoValid = true;
			m_bServerInfoChanged = true;
		}

		void ClearServerInfo()
		{
			M_LOCK(m_ServerInfoLock);
			m_ServerInfoValid = false;
		}

		virtual bool Update()
		{
			uint32 Status = m_pSocket->GetStatus();

			return Update(Status);
		}

		bool Update(uint32 _Status)
		{
			return CConnection::Update(_Status);
		}

	};

	typedef TPtr<CServer> spCServer;

	class CClient : public CConnection
	{
	public:
		CClient(CNetworkCore *_pNet) : CConnection(_pNet)
		{
			m_ConnectionType = EConnectionType_Client;
			m_ServerInfoValid = false;
		}

		~CClient()
		{
		}


		MRTC_CriticalSection m_ServerInfoLock;
		CNetwork_ServerInfo m_ServerInfo;
		int m_ServerInfoValid;
		bint GetServerInfo(CNetwork_ServerInfo &_Dest)
		{
			M_LOCK(m_ServerInfoLock);
			if (!m_ServerInfoValid)
				return false;
			_Dest = m_ServerInfo;
			return true;
		}

		void SetServerInfo(const CNetwork_ServerInfo &_Src)
		{
			M_LOCK(m_ServerInfoLock);
			m_ServerInfo = _Src;
			m_ServerInfoValid = true;
		}

		class CServerInfoQueue
		{
		public:
			DLinkD_Link(CServerInfoQueue, m_Link);			

			CNetwork_ServerInfo m_ServerInfo;
		};

		MRTC_CriticalSection m_QueueLock;
		DLinkD_List(CServerInfoQueue, m_Link) m_InfoQueue;

		void QueueServerInfo(CNetwork_ServerInfo *_pInfo)
		{
			CServerInfoQueue *pQueue = DNew(CServerInfoQueue) CServerInfoQueue;
			pQueue->m_ServerInfo = *_pInfo;
			M_LOCK(m_QueueLock);            
			m_InfoQueue.Insert(pQueue);
		}

		CNetwork_ServerInfo *GetServerInfoQueueHead()
		{
			M_LOCK(m_QueueLock);

			CServerInfoQueue *pInfo = m_InfoQueue.GetFirst();

			if (pInfo)
			{
				return &pInfo->m_ServerInfo;
			}

			return NULL;
		}

		void PopServerInfoQueue()
		{
			CServerInfoQueue *pInfo;
			{
				M_LOCK(m_QueueLock);
				pInfo = m_InfoQueue.Pop();
			}

			if (pInfo)
				delete pInfo;

		}
		
	
		virtual bool Update()
		{
			uint32 Status = m_pSocket->GetStatus();

			return Update(Status);
		}

		bool Update(uint32 _Status)
		{
			return CConnection::Update(_Status);
		}
	};

	typedef TPtr<CClient> spCClient;

    int AllocID()
	{
		M_LOCK(m_ConnectionsLock);

		int iConnection = m_ConnectionIDs.AllocID();
		if (iConnection < 0)
			return -1;

		if (m_splConnections.Len() <= iConnection)
			m_splConnections.SetLen(iConnection + 1);

		return iConnection;
	}

	void FreeID(int _ID)
	{
		M_LOCK(m_ConnectionsLock);
		m_ConnectionIDs.FreeID(_ID);
	}

	NThread::CEventAutoResetReportable m_MainEvent;
	CWorkerThread *m_pThread;

	int Global_GetThreadID()
	{
		while (Volatile(m_ThreadID) == 0)
			MRTC_SystemInfo::OS_Sleep(1);
		return m_ThreadID;
	}


	int m_ThreadID;
	int Thread_Main()
	{
		MRTC_SystemInfo::Thread_SetName("NetworkCore worker");

		m_ThreadID = (int)(mint)MRTC_SystemInfo::OS_GetThreadID();
		m_pThread->m_QuitEvent.ReportTo(&m_MainEvent);
//		CStr Program;
//		gfs_GetProgramPath(Program);
//		Program = Program.GetFilenameNoExt();

		while (!m_pThread->Thread_IsTerminating())
		{
			bool bStuffed = false;
			{
				
				M_LOCK(m_ThreadUpdateLock);
				{
					M_LOCK(m_ConnectionsLock);

					DLinkD_Iter(CConnection, m_Link) Iter = m_Connections;
					while (Iter)
					{
						CConnection *pConn = Iter;
						++Iter;
						{
							M_LOCK(pConn->m_DeleteLock);
							{
								M_UNLOCK(m_ConnectionsLock);
								bStuffed |= pConn->Update();
							}
						}
					}
				}
			}
            
			if (m_MainEvent.WaitTimeout(0.001f))
			{
//				MRTC_SystemInfo::OS_TraceRaw(CFStrF("[%s] Waking up from signal\n", Program.Str()));
			}
//			else
//				MRTC_SystemInfo::OS_TraceRaw(CFStrF("[%s] Waking up from timeout\n", Program.Str()));
//			if (!bStuffed)
//			m_MainEvent.WaitTimeout(0.05f);
		}

		return 0;
	}

	// Server-only:
	virtual int Server_Listen(CNetwork_Address *_pAddress, NThread::CEventAutoResetReportableAggregate *_pReportTo)
	{
		int iServer = AllocID();
		if (iServer < 0)
			return -1;

		spCServer spServer = MNew1(CServer, this);
		CNetwork_ListenSocket *pSocket = m_spDevice->Listen(_pAddress, &m_MainEvent);
		if (!pSocket)
		{
			FreeID(iServer);
			return -1;
		}
		spServer->m_pSocket = pSocket;
		spServer->m_bOwnSocket = true;
		spServer->m_pReportTo = _pReportTo;

		{
			M_LOCK(m_ConnectionsLock);
			m_splConnections[iServer] = spServer;
			m_Connections.Insert(spServer);
		}

		return iServer;
	}

	CConnection *GetConnection(int _hConnection)
	{
		M_LOCK(m_ConnectionsLock);

		if (_hConnection < 0 || _hConnection >= m_splConnections.Len())
			return NULL;

        return m_splConnections[_hConnection];
	}

	virtual bint Server_Connection_Avail(int _hConnection)
	{
		CConnection *pConn = GetConnection(_hConnection);

		if (!pConn || pConn->m_ConnectionType != EConnectionType_Server)
			return false;
		
		return pConn->m_Available.GetFirst() != NULL;
	}

	virtual int Server_Connection_Accept(int _hConnection, NThread::CEventAutoResetReportableAggregate *_pReportTo)
	{
		CConnection *pConn = GetConnection(_hConnection);

		if (!pConn || pConn->m_ConnectionType != EConnectionType_Server)
			return -1;

		int iAccept = pConn->Accept();
		if (iAccept >= 0)
		{
			CConnection *pAccept = GetConnection(iAccept);
			pAccept->m_pReportTo = _pReportTo;

		}

		return iAccept;		
	}

	virtual void Server_Connection_Refuse(int _hConnection)
	{
		CConnection *pConn = GetConnection(_hConnection);

		if (!pConn || pConn->m_ConnectionType != EConnectionType_Server)
			return;

		pConn->Refuse();		
	}

	virtual void Server_ClearInfo(int _hConnection)
	{
		CServer *pConn = (CServer *)GetConnection(_hConnection);

		if (!pConn || pConn->m_ConnectionType != EConnectionType_Server)
			return;

		pConn->ClearServerInfo();
	}

	virtual void Server_SetInfo(int _hConnection, const CNetwork_ServerInfo &_Info)
	{
		CServer *pConn = (CServer *)GetConnection(_hConnection);

		if (!pConn || pConn->m_ConnectionType != EConnectionType_Server)
			return;

		pConn->SetServerInfo(_Info);
	}

	// Client-only:
	virtual int Client_Create(CNetwork_Address *_pAddress, NThread::CEventAutoResetReportableAggregate *_pReportTo)
	{
		int iClient = AllocID();
		if (iClient < 0)
			return -1;

		spCClient spClient = MNew1(CClient, this);
		CNetwork_ListenSocket *pSocket = m_spDevice->Listen(_pAddress, &m_MainEvent);
		if (!pSocket)
		{
			FreeID(iClient);
			return -1;
		}
		spClient->m_pSocket = pSocket;
		spClient->m_bOwnSocket = true;
		spClient->m_pReportTo = _pReportTo;


		{
			M_LOCK(m_ConnectionsLock);
			m_splConnections[iClient] = spClient;
			m_Connections.Insert(spClient);
		}

		return iClient;
	}

	virtual bint Client_QueryConnection(int _hConnection, CNetwork_Address *_pAddress)
	{
		CConnection *pConn = GetConnection(_hConnection);

		if (!pConn || pConn->m_ConnectionType != EConnectionType_Client)
			return false;

		CNetwork_Address *pAddress = m_spDevice->Address_Alloc();
		pAddress->Copy(*_pAddress);
		uint8 Temp[1];
		Temp[0] = EMessageInternal_Query;

		if (!pConn->QueueOutgoingInternalMessage(pConn->m_iChannel_InternalUnreliable, Temp, 1, pAddress))
		{
			pAddress->Delete();
			return false;
		}

		return true;
	}

	virtual CNetwork_ServerInfo* Client_OpenConnection(int _hConnection, CNetwork_Address * _pAddress, fp4 _TimeOut)
	{
		CClient *pConn = (CClient *)GetConnection(_hConnection);

		if (!pConn || pConn->m_ConnectionType != EConnectionType_Client)
			return NULL;

		CNetwork_Address *pAddress = m_spDevice->Address_Alloc();
		pAddress->Copy(*_pAddress);
		uint8 Temp[1];
		Temp[0] = EMessageInternal_Connect;

		if (!pConn->QueueOutgoingInternalMessage(pConn->m_iChannel_InternalUnreliable, Temp, 1, pAddress))
		{
			pAddress->Delete();
		}

		CMTime Timeout = CMTime::GetCPU() + CMTime::CreateFromSeconds(_TimeOut);

		bool bAskServerInfo = false;

		while (CMTime::GetCPU().Compare(Timeout) < 0)
		{
			pConn->FlushSend();
			m_MainEvent.Signal();

			uint32 Status = pConn->GetConnecitonStatus();

			if (Status & ENetworkPacketStatus_Closed) // Connection failed
				break;

			if (Status & ENetworkPacketStatus_Connected)
			{
				if (!bAskServerInfo)
				{
					uint8 Temp[1];
					Temp[0] = EMessageInternal_Query;
					if (pConn->QueueOutgoingInternalMessage(pConn->m_iChannel_InternalReliable, Temp, 1, NULL))
						bAskServerInfo = true;
				}
			}

			CNetwork_ServerInfo Info;
			if (pConn->GetServerInfo(Info))
			{
				return &pConn->m_ServerInfo;
			}

			MRTC_SystemInfo::OS_Sleep(5);
		}

		return NULL;
	}

	virtual bint Client_ServerInfoAvail(int _hConnection)
	{
		CClient *pConn = (CClient *)GetConnection(_hConnection);

		if (!pConn || pConn->m_ConnectionType != EConnectionType_Client)
			return false;
		
		return pConn->GetServerInfoQueueHead() != NULL;
	}
	
	virtual CNetwork_ServerInfo* Client_GetServerInfo(int _hConnection)
	{
		CClient *pConn = (CClient *)GetConnection(_hConnection);

		if (!pConn || pConn->m_ConnectionType != EConnectionType_Client)
			return NULL;
		
		return pConn->GetServerInfoQueueHead();
	}

	virtual void Client_GetServerInfoRelease(int _hConnection)
	{
		CClient *pConn = (CClient *)GetConnection(_hConnection);

		if (!pConn || pConn->m_ConnectionType != EConnectionType_Client)
			return;
		
		pConn->PopServerInfoQueue();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual void Connection_Close(int _hConnection)
	{
		CConnection *pConn = GetConnection(_hConnection);

		if (pConn)
		{
			M_LOCK(m_ThreadUpdateLock); // Lock for deleting connections
			{
				M_LOCK(m_ConnectionsLock); // Lock for connectios
				{
					pConn->m_DeleteLock.Lock(); // Make sure we aren't updating on another thread
					m_splConnections[_hConnection] = NULL;
					FreeID(_hConnection);
				}
			}
		}
		
	}

	virtual uint32 Connection_Status(int _hConnection)
	{
		CConnection *pConn = GetConnection(_hConnection);

		if (pConn)
			return pConn->GetConnecitonStatus();
		else
			return ENetworkPacketStatus_Closed;
	}

	virtual fp4 Connection_GetPing(int _hConnection)
	{
		CConnection *pConn = GetConnection(_hConnection);

		if (pConn)
			return pConn->m_PingQueue.GetPing();
		else
			return 0;
	}

	//
	virtual int Connection_ChannelAlloc(int _hConnection, uint32 _Flags, fp4 _RatePriority, fp4 _MaxRate)
	{
		M_LOCK(m_ThreadUpdateLock);
		CConnection *pConn = GetConnection(_hConnection);

		if (pConn)
			return pConn->m_ChannelsSend.ChannelAlloc(_RatePriority, _MaxRate, (_Flags & ENetworkChannelFlag_Reliable) != 0, (_Flags & ENetworkChannelFlag_OutOfOrder) != 0);
		else
			return -1;
	}

	virtual void Connection_ChannelSetBufferSize(int _hConnection, int _iChannel, mint _Size)
	{
		M_LOCK(m_ThreadUpdateLock);
		CConnection *pConn = GetConnection(_hConnection);

		if (pConn)
		{
			CConnection::CSendChannelHandler::CChannel *pChn = pConn->m_ChannelsSend.GetChannel(_iChannel);
			if (pChn)
			{
				pChn->m_Queue.SetHeapSize(_Size);
			}
		}
			
	}

	virtual void Connection_ChannelSetNumPackets(int _hConnection, int _iChannel, mint _nPackets)
	{
		M_LOCK(m_ThreadUpdateLock);
		CConnection *pConn = GetConnection(_hConnection);

		if (pConn)
		{
			CConnection::CSendChannelHandler::CChannel *pChn = pConn->m_ChannelsSend.GetChannel(_iChannel);
			if (pChn)
			{
				pChn->m_Queue.SetNumPackets(_nPackets);
			}
		}
	}

	//
	virtual bint Connection_PacketSend(int _hConnection, CNetwork_Packet *_pPacket)
	{
		CConnection *pConn = GetConnection(_hConnection);

		if (pConn)
			return pConn->QueueOutgoingMessage(_pPacket->m_iChannel, _pPacket->m_pData, _pPacket->m_Size, _pPacket->m_pAddress);

		return false;
	}

	CNetwork_Packet m_TempReceivePacket;
	
	virtual CNetwork_Packet *Connection_PacketReceive(int _hConnection)
	{
		CConnection *pConn = GetConnection(_hConnection);

		if (pConn)
		{
			while (1)
			{
				CConnection::CReceiveChannelHandler::CChannel *pChn = pConn->m_ChannelsReceive.GetFirstAvailable();
				if (pChn)
				{
					if (!pChn->m_Queue.m_nQueuedPackets.Get())
					{
						M_LOCK(pConn->m_ChannelsReceive.m_AvailableLock);
						pChn->m_Link.Unlink();
					}
					else
					{
//						M_LOCK(pChn->m_Queue.m_QueueLock);
						CConnection::CDataQueue::CPacket *pPacket = pChn->m_Queue.m_PacketQueue.GetFirst();
						M_ASSERT(pPacket, "Must be if we have queued packets");
						m_TempReceivePacket.m_iChannel = pChn->m_iChn;
						m_TempReceivePacket.m_pAddress = pPacket->m_pAddress;
						m_TempReceivePacket.m_pData = pChn->m_Queue.m_Heap.GetBasePtr() + pPacket->m_Memory;
						m_TempReceivePacket.m_Size = pPacket->m_Size;
						return &m_TempReceivePacket;
					}
				}
				else
					break;
			}
		}

		return NULL;
	}

	virtual void Connection_PacketRelease(int _hConnection, CNetwork_Packet *_pPacket)
	{
		CConnection *pConn = GetConnection(_hConnection);

		if (pConn)
		{
			CConnection::CReceiveChannelHandler::CChannel *pChn = pConn->m_ChannelsReceive.GetFirstAvailable();
			if (pChn)
			{
				pChn->m_Queue.PopHeadPacket();

				if (!pChn->m_Queue.m_nQueuedPackets.Get())
				{
					M_LOCK(pConn->m_ChannelsReceive.m_AvailableLock);
					pChn->m_Link.Unlink();
				}
			}
		}
	}

	virtual void Connection_TraceQueues(int _hConnection)
	{
		CConnection *pConn = GetConnection(_hConnection);

		if (pConn)
		{
			for (int i = 0; i < pConn->m_ChannelsReceive.m_Channels.Len(); ++i)
			{
				if (pConn->m_ChannelsReceive.m_Channels[i])
					pConn->m_ChannelsReceive.m_Channels[i]->m_Queue.TraceQueues();
			}

			for (int i = 0; i < pConn->m_ChannelsSend.m_Channels.Len(); ++i)
			{
				if (pConn->m_ChannelsSend.m_Channels[i])
					pConn->m_ChannelsSend.m_Channels[i]->m_Queue.TraceQueues();
			}
		}
	}

	virtual mint Connection_GetQueuedOutSize(int _hConnection)
	{
		CConnection *pConn = GetConnection(_hConnection);

		if (pConn)
		{
			mint Size = 0;
			for (int i = 0; i < pConn->m_ChannelsSend.m_Channels.Len(); ++i)
			{
				if (pConn->m_ChannelsSend.m_Channels[i])
				{
					M_LOCK(pConn->m_ChannelsSend.m_Channels[i]->m_Queue.m_QueueLock);
					Size += pConn->m_ChannelsSend.m_Channels[i]->m_Queue.m_QueuedSize;
				}
			}

			return Size;

		}
		return 0;
	}

	virtual mint Connection_GetQueuedInSize(int _hConnection)
	{
		CConnection *pConn = GetConnection(_hConnection);

		if (pConn)
		{
			mint Size = 0;
			for (int i = 0; i < pConn->m_ChannelsReceive.m_Channels.Len(); ++i)
			{
				if (pConn->m_ChannelsReceive.m_Channels[i])
				{
					M_LOCK(pConn->m_ChannelsReceive.m_Channels[i]->m_Queue.m_QueueLock);
					Size += pConn->m_ChannelsReceive.m_Channels[i]->m_Queue.m_QueuedSize;
				}
			}

			return Size;
		}
		return 0;
	}

	virtual void Connection_Flush(int _hConnection)
	{
		CConnection *pCon = GetConnection(_hConnection);
		if (pCon)
		{
			pCon->FlushSend();
			m_MainEvent.Signal();
		}
	}

	//
	virtual void Connection_SetRate(int _hConnection, fp4 _BytesPerSecond)
	{
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual void Global_SetRate(fp4 _BytesPerSecond)
	{
	}

	virtual CNetwork_Address *Global_ResolveAddress(CStr _Address)
	{
		return m_spDevice->Address_Resolve(_Address);
	}

	virtual void Refresh()
	{
		m_MainEvent.Signal();
	}
};

MRTC_IMPLEMENT_DYNAMIC(CNetworkCore, CNetwork);

class CNetwork_Device_UDP : public CNetwork_Device
{
	MRTC_DECLARE;
public:

	class CAddress : public CNetwork_Address
	{
	public:

		NNet::CNetAddressUDPv4 m_Address;

		virtual void Delete()
		{
			delete this;
		}

		virtual int Compare(const CNetwork_Address &_Other)
		{
			return memcmp(&m_Address, &(((const CAddress &)_Other).m_Address), sizeof(m_Address));
		}

		virtual void Copy(const CNetwork_Address &_Source)
		{
			m_Address = ((CAddress &)_Source).m_Address;
		}

		CStr GetStr()
		{
			return CStrF("%d.%d.%d.%d:%d", m_Address.m_IP[0], m_Address.m_IP[1], m_Address.m_IP[2], m_Address.m_IP[3], m_Address.m_Port);
		}

	};

	class CSocket : public CNetwork_ListenSocket
	{
	public:

		CSocket()
		{
			m_pSocket = NULL;
		}

        ~CSocket()
		{
			if (m_pSocket)
				MRTC_SystemInfo::CNetwork::gf_Close(m_pSocket);
		}

		void *m_pSocket;

		virtual void Delete()
		{
			delete this;
		}

		virtual CNetwork_Socket *Accept(NThread::CEventAutoResetReportableAggregate *_pReportTo)
		{
			Error_static(__FUNCTION__, "Accept cannot be called on a connectionless socket");
			return NULL;
		}

		virtual void Refuse()
		{
			Error_static(__FUNCTION__, "Refuse cannot be called on a connectionless socket");
		}

		virtual bint Send(const CNetwork_Packet &_Packet)
		{
			if (!_Packet.m_pAddress)
				Error_static(__FUNCTION__, "Address must be specefied to send to a connectionless socket");
			return MRTC_SystemInfo::CNetwork::gf_Send(m_pSocket, ((CAddress *)_Packet.m_pAddress)->m_Address, _Packet.m_pData, _Packet.m_Size);
		}

		virtual bint Receive(CNetwork_Packet &_Packet)
		{
			if (_Packet.m_pAddress)
			{
				int nBytes = MRTC_SystemInfo::CNetwork::gf_Receive(m_pSocket, ((CAddress *)_Packet.m_pAddress)->m_Address, _Packet.m_pData, _Packet.m_Size);
				_Packet.m_Size = nBytes;
				return nBytes != 0;
			}
			else
			{
				NNet::CNetAddressUDPv4 Addr;
				int nBytes = MRTC_SystemInfo::CNetwork::gf_Receive(m_pSocket, Addr, _Packet.m_pData, _Packet.m_Size);
				_Packet.m_Size = nBytes;
				return nBytes != 0;
			}
		}

		virtual uint32 GetStatus()
		{
			if (!m_pSocket)
				return ENetworkPacketStatus_Closed;

			uint32 Status = 0;
			uint32 State = MRTC_SystemInfo::CNetwork::gf_GetState(m_pSocket);
			if (State & NNet::ENetTCPState_Read)
				Status |= ENetworkPacketStatus_Receive;
			if (State & NNet::ENetTCPState_Write)
				Status |= ENetworkPacketStatus_Send;
			if (State & NNet::ENetTCPState_Closed)
				Status |= ENetworkPacketStatus_Closed;
			if (State & NNet::ENetTCPState_Connection)
				Status |= ENetworkPacketStatus_Connection;

			return Status;
		}
	};

	virtual bint ConnectionLess()
	{
		return true;
	}

	virtual CNetwork_ListenSocket *Listen(CNetwork_Address *_pAddress, NThread::CEventAutoResetReportableAggregate *_pReportTo)
	{
		void *pSocket = MRTC_SystemInfo::CNetwork::gf_Bind(((CAddress *)_pAddress)->m_Address, _pReportTo);

		if (pSocket)
		{
			CSocket *pSock = DNew(CSocket) CSocket;
			pSock->m_pSocket = pSocket;
			return pSock;
		}
		else
			return NULL;
	}

	virtual CNetwork_Socket *Connect(CNetwork_Address *_pAddress, NThread::CEventAutoResetReportableAggregate *_pReportTo)
	{
		Error_static(__FUNCTION__, "Connect cannot be called on an connectionless socket");
		return NULL;
	}

	virtual CNetwork_Address *Address_Alloc()
	{
		CAddress *pAddr = DNew(CAddress) CAddress;
		return pAddr;
	}

	virtual CNetwork_Address *Address_Resolve(CStr _Address)
	{
		CStr Addr = _Address.GetStrSep(";");
		CStr Flags = _Address.GetStrSep(";"); 
		int iColon = Addr.Find(":");
		if (iColon >= 0)
		{
			CAddress *pAddr = DNew(CAddress) CAddress;
			CStr Address = Addr.Left(iColon);
			CStr Port = Addr.CopyFrom(iColon+1);
			if (Address == "*") // Any address
			{
				pAddr->m_Address.m_IP[0] = 0;
				pAddr->m_Address.m_IP[1] = 0;
				pAddr->m_Address.m_IP[2] = 0;
				pAddr->m_Address.m_IP[3] = 0;
			}
			else if (Address == "@") // Broadcast address
			{
				pAddr->m_Address.m_IP[0] = 255;
				pAddr->m_Address.m_IP[1] = 255;
				pAddr->m_Address.m_IP[2] = 255;
				pAddr->m_Address.m_IP[3] = 255;
			}
			else
			{
				if (!MRTC_SystemInfo::CNetwork::gf_ResolveAddres(Address, pAddr->m_Address))
				{
					delete pAddr;
					return NULL;
				}
			}

			pAddr->m_Address.m_Port = Port.Val_int();

			while (Flags != "")
			{
				CStr Flag = Flags.GetStrSep(",");

				if (Flag.CompareNoCase("Broadcast") == 0)
				{
					pAddr->m_Address.m_bBroadcast = true;
				}
			}

			return pAddr;
		}

		return NULL;
	}
};

MRTC_IMPLEMENT_DYNAMIC(CNetwork_Device_UDP, CNetwork_Device);
