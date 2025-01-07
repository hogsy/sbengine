#ifndef DInc_MNetwork_h
#define DInc_MNetwork_h


enum
{
	// State
	ENetworkPacketStatus_Closed = DBit(0),
	ENetworkPacketStatus_Connected = DBit(1),
	ENetworkPacketStatus_Timeout = DBit(2),

	// Data available / can be sent
	ENetworkPacketStatus_Receive = DBit(3),
	ENetworkPacketStatus_Send = DBit(4),
	ENetworkPacketStatus_Connection = DBit(5),

	//
	ENetworkChannelFlag_AutoBufferSize = DBit(0),
	ENetworkChannelFlag_Reliable = DBit(1),
	ENetworkChannelFlag_OutOfOrder = DBit(2),

	ENetworkMaxPacketSize = 2048,
	ENetworkMaxServerInfoSize = ENetworkMaxPacketSize - 32,

};


class CNetwork_Address
{
public:
	virtual void Delete() pure;

	virtual int Compare(const CNetwork_Address &_Other) pure;
	virtual void Copy(const CNetwork_Address &_Source) pure;

	virtual CStr GetStr() pure;
};

class CNetwork_Packet
{
public:
	CNetwork_Packet()
	{
		m_pAddress = NULL;
		m_pData = NULL;
		m_Size = 0;
		m_iChannel = 0;
	}

	CNetwork_Address *m_pAddress;
	void *m_pData;
	mint m_Size;
	int m_iChannel;
};

class CNetwork_Socket
{
public:
	virtual void Delete() pure;

	virtual bint Send(const CNetwork_Packet &_Packet) pure;
	virtual bint Receive(CNetwork_Packet &_Packet) pure;

	virtual uint32 GetStatus() pure;
};

class CNetwork_ListenSocket : public CNetwork_Socket
{
public:

	virtual CNetwork_Socket *Accept(NThread::CEventAutoResetReportableAggregate *_pReportTo) pure;
	virtual void Refuse() pure;
};

class CNetwork_Device : public CReferenceCount
{
	MRTC_DECLARE;
public:

	virtual bint ConnectionLess() pure;

	virtual CNetwork_ListenSocket *Listen(CNetwork_Address *_pAddress, NThread::CEventAutoResetReportableAggregate *_pReportTo) pure;
	virtual CNetwork_Socket *Connect(CNetwork_Address *_pAddress, NThread::CEventAutoResetReportableAggregate *_pReportTo) pure;

	virtual CNetwork_Address *Address_Resolve(CStr _Address) pure;

	virtual CNetwork_Address *Address_Alloc() pure;

};

typedef TPtr<CNetwork_Device> spCNetwork_Device;

class CNetwork_ServerInfo
{
public:
	CNetwork_ServerInfo()
	{
		m_Name[0] = 0;
		m_InfoSize = 0;
	}

	CNetwork_ServerInfo(const CNetwork_ServerInfo &_Src)
	{
		m_Address = _Src.m_Address;
		m_InfoSize = _Src.m_InfoSize;
		memcpy(m_Name, _Src.m_Name, sizeof(m_Name));
		memcpy(m_ServerInfo, _Src.m_ServerInfo, m_InfoSize);
	}

	CNetwork_ServerInfo &operator = (const CNetwork_ServerInfo &_Src)
	{
		m_Address = _Src.m_Address;
		m_InfoSize = _Src.m_InfoSize;
		memcpy(m_Name, _Src.m_Name, sizeof(m_Name));
		memcpy(m_ServerInfo, _Src.m_ServerInfo, m_InfoSize);
		return *this;
	}

	CNetwork_ServerInfo(const ch8 *_pName, const void *_pData, mint _DataLen)
	{
		memset(m_Name, 0, 32);
		strncpy(m_Name, _pName, 31);
		m_InfoSize = MinMT(_DataLen, (mint)ENetworkMaxServerInfoSize);
		memcpy(m_ServerInfo, _pData, m_InfoSize);
	}

	CStr m_Address;
	ch8 m_Name[32];
	uint8 m_ServerInfo[ENetworkMaxServerInfoSize];
	uint32 m_InfoSize;
};

typedef TPtr<CNetwork_ServerInfo> spCNetwork_ServerInfo;


class CNetwork : public CReferenceCount
{
	MRTC_DECLARE;
public:
	virtual void Create(CNetwork_Device *_pDevice) pure;

	// Server-only:
	virtual int Server_Listen(CNetwork_Address *_pAddress, NThread::CEventAutoResetReportableAggregate *_pReportTo) pure;

	virtual bint Server_Connection_Avail(int _hConnection) pure;
	virtual int Server_Connection_Accept(int _hConnection, NThread::CEventAutoResetReportableAggregate *_pReportTo) pure;
	virtual void Server_Connection_Refuse(int _hConnection) pure;

	virtual void Server_ClearInfo(int _hConnection) pure;
	virtual void Server_SetInfo(int _hConnection, const CNetwork_ServerInfo &_Info) pure;

	// Client-only:
	virtual int Client_Create(CNetwork_Address *_pAddress, NThread::CEventAutoResetReportableAggregate *_pReportTo) pure;					// Returns client handle.
	virtual bint Client_QueryConnection(int _hConnection, CNetwork_Address *_pAddress) pure;
	virtual CNetwork_ServerInfo* Client_OpenConnection(int _hConnection, CNetwork_Address * _pAddress, fp4 _TimeOut) pure;

	virtual bint Client_ServerInfoAvail(int _hConnection) pure;
	virtual CNetwork_ServerInfo* Client_GetServerInfo(int _hConnection) pure;
	virtual void Client_GetServerInfoRelease(int _hConnection) pure;


	///////////////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual void Connection_Close(int _hConnection) pure;
	virtual uint32 Connection_Status(int _hConnection) pure;
	virtual fp4 Connection_GetPing(int _hConnection) pure;
	//
	virtual int Connection_ChannelAlloc(int _hConnection, uint32 _Flags, fp4 _RatePriority, fp4 _MaxRate) pure;
	virtual void Connection_ChannelSetBufferSize(int _hConnection, int _iChannel, mint _Size) pure;
	virtual void Connection_ChannelSetNumPackets(int _hConnection, int _iChannel, mint _nPackets) pure;
	//
	virtual bint Connection_PacketSend(int _hConnection, CNetwork_Packet *_Packet) pure;
	virtual CNetwork_Packet *Connection_PacketReceive(int _hConnection) pure;
	virtual void Connection_PacketRelease(int _hConnection, CNetwork_Packet *_Packet) pure;
	virtual void Connection_Flush(int _hConnection) pure; // Flushes the send queue and sends any queued data
	//
	virtual void Connection_SetRate(int _hConnection, fp4 _BytesPerSecond) pure;

	virtual void Connection_TraceQueues(int _hConnection) pure;
	virtual mint Connection_GetQueuedOutSize(int _hConnection) pure;
	virtual mint Connection_GetQueuedInSize(int _hConnection) pure;

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual int Global_GetThreadID() pure;
	virtual void Global_SetRate(fp4 _BytesPerSecond) pure;
	virtual CNetwork_Address *Global_ResolveAddress(CStr _Address) pure;

	virtual void Refresh() pure;
	// Interface

};

typedef TPtr<CNetwork> spCNetwork;

#endif //DInc_MNetwork_h
