#include "PCH.h"
#include "WClient.h"

// -------------------------------------------------------------------
//  CWC_Player
// -------------------------------------------------------------------
void CWC_Player::Clear()
{
	MAUTOSTRIP(CWC_Player_Clear, MAUTOSTRIP_VOID);
	m_Flags = 0;
	m_spCmdQueue = MNew1(CCmdQueue, int32(CWCLIENT_DEFAULTCONTROLQUEUE));
	if (!m_spCmdQueue) MemError("Clear");

	m_ServerControlSerial = 0x7fff;
	m_ServerControlAccumTime = 0;
}

void CWC_Player::SetName(const char* _pName)
{
	MAUTOSTRIP(CWC_Player_SetName, MAUTOSTRIP_VOID);
	mint len = strlen(_pName);
	if (len > PLAYER_MAXNAMELEN) 
		m_Name = CStr(_pName).Copy(0, PLAYER_MAXNAMELEN);
	else
		m_Name = _pName;
}

CStr CWC_Player::GetName()
{
	MAUTOSTRIP(CWC_Player_GetName, CStr());
	return m_Name;
}

void CWC_Player::operator= (const CWC_Player& _p)
{
	MAUTOSTRIP(CWC_Player_operator_assign, MAUTOSTRIP_VOID);
	m_Flags = _p.m_Flags;
	m_Name = _p.m_Name;
	m_spCmdQueue = MNew1(CCmdQueue, int32(CWCLIENT_DEFAULTCONTROLQUEUE));
	if (!m_spCmdQueue) MemError("Clear");
}

CWC_Player::CWC_Player()
{
	MAUTOSTRIP(CWC_Player_ctor, MAUTOSTRIP_VOID);
	Clear();
}

void CWC_Player::AddToPacket(CNetMsg& _Packet)
{
	MAUTOSTRIP(CWC_Player_AddToPacket, MAUTOSTRIP_VOID);
	_Packet.AddInt16(m_Flags);
	int NameLen = m_Name.Len();
	_Packet.AddInt8(NameLen);
	_Packet.AddData((char*)m_Name, NameLen);
}

void CWC_Player::GetFromPacket(const CNetMsg& _Packet, int &_Pos)
{
	MAUTOSTRIP(CWC_Player_GetFromPacket, MAUTOSTRIP_VOID);
	m_Flags = _Packet.GetInt16(_Pos);
	int NameLen = _Packet.GetInt8(_Pos);
	if (NameLen > PLAYER_MAXNAMELEN) Error("GetFromPacket", "Invalid packet.");
	char Name[PLAYER_MAXNAMELEN];
	_Packet.GetData(&Name, NameLen, _Pos);
	m_Name.Capture(Name, NameLen);
}

bool CWC_Player::Equal(const CWC_Player* _pOld)
{
	MAUTOSTRIP(CWC_Player_Equal, false);
	if (m_Flags != _pOld->m_Flags) return false;
	if (m_Flags)
	{
		if (m_Name != _pOld->m_Name) return false;
	}
	return true;
}

void CWC_Player::DeltaAddToPacket(CNetMsg& _Packet, const CWC_Player* _pOld)
{
	MAUTOSTRIP(CWC_Player_DeltaAddToPacket, MAUTOSTRIP_VOID);
	int Upd = 0;
	if (m_Flags != _pOld->m_Flags) Upd |= PLAYER_DELTAUPD_FLAGS;
	if (m_Flags)
	{
		if (m_Name != _pOld->m_Name) Upd |= PLAYER_DELTAUPD_NAME;
	}

//LogFile(CStrF("(CWC_Player::DeltaAddToPacket) Upd %d, %d, %d", Upd, m_Flags, m_iObject));
	_Packet.AddInt8(Upd);
	if (Upd & PLAYER_DELTAUPD_FLAGS) _Packet.AddInt16(m_Flags);

	if (Upd & PLAYER_DELTAUPD_NAME)
	{
		int NameLen = m_Name.Len();
		_Packet.AddInt8(NameLen);
		_Packet.AddData((char*)m_Name, NameLen);
	}
}

void CWC_Player::DeltaGetFromPacket(const CNetMsg& _Packet, int &_Pos)
{
	MAUTOSTRIP(CWC_Player_DeltaGetFromPacket, MAUTOSTRIP_VOID);
	int Upd = _Packet.GetInt8(_Pos);
	if (Upd & PLAYER_DELTAUPD_FLAGS) m_Flags = _Packet.GetInt16(_Pos);

	if (Upd & PLAYER_DELTAUPD_NAME)
	{
		int NameLen = _Packet.GetInt8(_Pos);
		if (NameLen > PLAYER_MAXNAMELEN) Error("DeltaGetFromPacket", "Invalid packet.");
		char Name[PLAYER_MAXNAMELEN];
		_Packet.GetData(&Name, NameLen, _Pos);
		m_Name.Capture(Name, NameLen);
	}

	if (!m_Flags) Clear();
}

CStr CWC_Player::Dump()
{
	MAUTOSTRIP(CWC_Player_Dump, CStr());
	return CStrF("(%.24s), Flags %.4d", (char*) m_Name, m_Flags);
}

// -------------------------------------------------------------------
//  CNetTransferBufferBase
// -------------------------------------------------------------------
CNetTransferBufferBase::CNetTransferBufferBase()
{
	MAUTOSTRIP(CNetTransferBufferBase_ctor, MAUTOSTRIP_VOID);
	m_bOut = false;
	m_bDynamic = false;
}

void CNetTransferBufferBase::Create(int _Size, bool _bOut, bool _bDynamic)
{
	MAUTOSTRIP(CNetTransferBufferBase_Create, MAUTOSTRIP_VOID);
	m_lBuffer.SetLen(_Size);
	m_bOut = _bOut;
	m_bDynamic = _bDynamic;
}

void CNetTransferBufferBase::Create(TArray<uint8> _lBuffer, bool _bOut, bool _bDynamic)
{
	MAUTOSTRIP(CNetTransferBufferBase_Create_2, MAUTOSTRIP_VOID);
	m_lBuffer = _lBuffer;	// Shared lists.
	m_bOut = _bOut;
	m_bDynamic = _bDynamic;
}

TArray<uint8>& CNetTransferBufferBase::GetBuffer()
{
	return m_lBuffer;
}

void CNetTransferBufferBase::InitializeTransfer(CNetTransferInstance& _Transfer)
{
	MAUTOSTRIP(CNetTransferBufferBase_InitializeTransfer, MAUTOSTRIP_VOID);
	_Transfer.Clear();
}

bool CNetTransferBufferBase::AddPacket(CNetTransferInstance& _Transfer, const CNetMsg& _Packet)
{
	MAUTOSTRIP(CNetTransferBufferBase_AddPacket, false);
	MSCOPESHORT(CNetTransferBufferBase::AddPacket);
	// Sanity check
	if (_Packet.m_Data[0] != (_Transfer.m_IOPos & 0xff))
		Error_static ("CNetTransferBufferBase::AddPacket", CStrF("Invalid CNetTransferBufferBase packet (Pos %d, Max %d, MsgSize %d, Check %d)", _Transfer.m_IOPos, m_lBuffer.Len(), _Packet.m_MsgSize, _Packet.m_Data[0] ));

	// Check that the new packet fit
	int Size = _Packet.m_MsgSize-1;
	if (_Transfer.m_IOPos + Size > m_lBuffer.Len()) 
	{
		if (m_bDynamic)
		{
			m_lBuffer.SetLen(Max(m_lBuffer.Len() * 2, _Transfer.m_IOPos + Size * 2));

		}
		else
			Error_static("CNetTransferBufferBase::AddPacket", CStrF("CNetTransferBufferBase overflow. (Pos %d, Max %d, MsgSize %d)", _Transfer.m_IOPos, m_lBuffer.Len(), Size));
	}

	int DataPos = 1;
	if (!_Transfer.m_IOPos)
	{
		// First packet, get header data.
		m_Size = _Packet.GetInt16(DataPos);
		Size -= 2;
	}

	// Copy data
	memcpy(&m_lBuffer[_Transfer.m_IOPos], &_Packet.m_Data[DataPos], Size);

//LogFile(CStrF("   -AddPacket %d->%d, %d, %d", _Transfer.m_IOPos, _Transfer.m_IOPos+Size, m_Size, Size));

	_Transfer.m_IOPos += Size;
	return (_Transfer.m_IOPos == m_Size);
}

bool CNetTransferBufferBase::GetPacket(CNetTransferInstance& _Transfer, CNetMsg& _Packet, int _MsgType)
{
	MAUTOSTRIP(CNetTransferBufferBase_GetPacket, false);
	// Complete?
	if (_Transfer.m_IOPos >= m_Size) return false;

	// Contruct packet
	_Packet.m_MsgType = _MsgType;
	int Size = Min(m_Size - _Transfer.m_IOPos, (int) sizeof(_Packet.m_Data) - 1);
	_Packet.AddInt8(_Transfer.m_IOPos);		// Value for sanity check.

	if (!_Transfer.m_IOPos)
	{
		// First packet, get header data.
		_Packet.AddInt16(m_Size);
		Size -= 2;
	}

	_Packet.AddData(&GetBuffer()[_Transfer.m_IOPos], Size);
	_Transfer.m_LastGetSize = Size;

//	LogFile(CStrF("    -GetPacket %d, %d, check %d", _Transfer.m_IOPos, Size, _Packet.m_Data[0]));
	return true;
}

bool CNetTransferBufferBase::GetNext(CNetTransferInstance& _Transfer)
{
	MAUTOSTRIP(CNetTransferBufferBase_GetNext, false);
	CNetMsg Dummy(0);
	int Size = _Transfer.m_LastGetSize;

//LogFile(CStrF("    -GetNext %d->%d, %d", _Transfer.m_IOPos, _Transfer.m_IOPos + Size, Size));

	_Transfer.m_IOPos += Size;
	return (_Transfer.m_IOPos == m_Size);
}

// -------------------------------------------------------------------
//  CWObjectUpdateBuffer
// -------------------------------------------------------------------
CWObjectUpdateBuffer::CWObjectUpdateBuffer()
{
	MAUTOSTRIP(CWObjectUpdateBuffer_ctor, MAUTOSTRIP_VOID);
	m_Size = DELTABUFFER_HEADERDATA;
	m_nObjects = 0;
	m_nSimTicks = 0;
	m_ControlSerial = 0;
	m_ControlAccumTime = 0;

	Create(WORLD_MAXDELTAFRAMESIZE, false, false);
}

void CWObjectUpdateBuffer::Clear()
{
	MAUTOSTRIP(CWObjectUpdateBuffer_Clear, MAUTOSTRIP_VOID);
	m_Size = DELTABUFFER_HEADERDATA;
	m_nObjects = 0;
	m_Transfer.Clear();
	m_nSimTicks = 0;
	m_ControlSerial = 0;
	m_ControlAccumTime = 0;
}

bool CWObjectUpdateBuffer::AddDeltaUpdate(uint8* _pBuffer, int _Size)
{
	MAUTOSTRIP(CWObjectUpdateBuffer_AddDeltaUpdate, false);
	// Adds a chunk of data to the buffer.

	if (_Size + m_Size > WORLD_MAXDELTAFRAMESIZE) return false;

	memcpy(&GetBuffer()[m_Size], _pBuffer, _Size);
	m_Size += _Size;
	m_nObjects++;
	return true;
}

bool CWObjectUpdateBuffer::GetPacket(CNetMsg& _Packet, int _MsgType)
{
	MAUTOSTRIP(CWObjectUpdateBuffer_GetPacket, false);
	// If this is the first packet, store total size, number of packets and simulation frames in the begining.
	if (!m_Transfer.m_IOPos)
	{
		int iData = 0;
		GetBuffer()[iData++] = m_nObjects & 0xff;
		GetBuffer()[iData++] = (m_nObjects >> 8) & 0xff;
		GetBuffer()[iData++] = m_nSimTicks & 0xff;
		GetBuffer()[iData++] = m_ControlSerial & 0xff;
		GetBuffer()[iData++] = (m_ControlSerial >> 8) & 0xff;
		GetBuffer()[iData++] = m_ControlAccumTime & 0xff;
		GetBuffer()[iData++] = (m_ControlAccumTime >> 8) & 0xff;

		GetBuffer()[iData++] = m_SimulationTick & 0xff;
		GetBuffer()[iData++] = (m_SimulationTick >> 8) & 0xff;
		GetBuffer()[iData++] = (m_SimulationTick >> 16) & 0xff;
		GetBuffer()[iData++] = (m_SimulationTick >> 24) & 0xff;

		if (iData != DELTABUFFER_HEADERDATA)
			Error("GetPacket", "Internal error.");
	}

	return CNetTransferBufferBase::GetPacket(m_Transfer, _Packet, _MsgType);
}

bool CWObjectUpdateBuffer::GetNext()
{
	MAUTOSTRIP(CWObjectUpdateBuffer_GetNext, false);
	return CNetTransferBufferBase::GetNext(m_Transfer);
}

bool CWObjectUpdateBuffer::AddPacket(const CNetMsg& _Packet)
{
	MAUTOSTRIP(CWObjectUpdateBuffer_AddPacket, false);
	int IOPos = m_Transfer.m_IOPos;
	bool bRet = CNetTransferBufferBase::AddPacket(m_Transfer, _Packet);

	// If it's the first package, extract total size, number of packages 
	// and the number of simulations that should be peformed before this update is unpacked.
	if (!IOPos)
	{
		int iData = 0;
		m_nObjects = GetBuffer()[iData] + (GetBuffer()[iData+1] << 8);	iData += 2;
		m_nSimTicks = GetBuffer()[iData++];
		m_ControlSerial = GetBuffer()[iData] + (GetBuffer()[iData+1] << 8); iData += 2;
		m_ControlAccumTime = GetBuffer()[iData] + (GetBuffer()[iData+1] << 8);	iData += 2;
		m_SimulationTick = GetBuffer()[iData] + (GetBuffer()[iData+1] << 8) + (GetBuffer()[iData+2] << 16) + (GetBuffer()[iData+3] << 24);	iData += 4;

		if (iData != DELTABUFFER_HEADERDATA)
			Error("AddPacket", "Internal error.");
	}

	// LogFile(CStrF("   -AddPacket %d->%d, %d, %d, %d", m_IOPos, m_IOPos+Size, m_Size, m_nObjects, Size));

	return bRet;
}

// -------------------------------------------------------------------
//  CDemoMsgStream
// -------------------------------------------------------------------
CDemoMsgStream::CDemoMsgStream()
{
	MAUTOSTRIP(CDemoMsgStream_ctor, MAUTOSTRIP_VOID);
	Clear();
}

void CDemoMsgStream::Clear()
{
	MAUTOSTRIP(CDemoMsgStream_Clear, MAUTOSTRIP_VOID);
	m_lMsgBuffer.Clear();
	m_BufferPos = 0;
}

void CDemoMsgStream::Read(CDataFile* _pDFile)
{
	MAUTOSTRIP(CDemoMsgStream_Read, MAUTOSTRIP_VOID);
	Clear();
	_pDFile->PushPosition();
	if (!_pDFile->GetNext("MESSAGESTREAM"))
		Error("Demo_ReadMsgStream", "Corrupt demo-file.");

	m_lMsgBuffer.SetLen(_pDFile->GetEntrySize());
	_pDFile->GetFile()->Read(m_lMsgBuffer.GetBasePtr(), _pDFile->GetEntrySize());

	_pDFile->PopPosition();
	m_BufferPos = 0;
}

bool CDemoMsgStream::MsgAvail() const
{
	MAUTOSTRIP(CDemoMsgStream_MsgAvail, false);
	return (m_BufferPos < m_lMsgBuffer.Len());
}

fp32 CDemoMsgStream::GetNextTimeStamp() const
{
	MAUTOSTRIP(CDemoMsgStream_GetNextTimeStamp, 0.0f);
	if (m_BufferPos >= m_lMsgBuffer.Len())
		Error("GetNextTimeStamp", "Out of messages.");

	return *((fp32*) &m_lMsgBuffer[m_BufferPos]);
}

const CNetMsg* CDemoMsgStream::GetNextMsg()
{
	MAUTOSTRIP(CDemoMsgStream_GetNextMsg, NULL);
	if (m_BufferPos >= m_lMsgBuffer.Len())
		Error("GetNextTimeStamp", "Out of messages.");

	m_BufferPos += sizeof(fp32);
	int OldPos = m_BufferPos;
	m_BufferPos += ((CNetMsg*) &m_lMsgBuffer[OldPos])->GetSize();

	// LogFile(CStrF("   step %d -> %d, %d", OldPos, m_BufferPos, ((CNetMsg*) &m_lMsgBuffer[OldPos])->GetSize() ));

	return (CNetMsg*) &m_lMsgBuffer[OldPos];
}

