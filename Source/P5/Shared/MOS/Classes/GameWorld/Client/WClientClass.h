#ifndef __WCLIENTCLASS_H
#define __WCLIENTCLASS_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Misc Client helper classes

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWC_Player
					CNetTransferInstance
					CNetTransferBufferBase
					CWObjectUpdateBuffer
					CDemoMsgStream
\*____________________________________________________________________________________________*/

#include "../WClassCmd.h"

// -------------------------------------------------------------------
//  CWC_Player
// -------------------------------------------------------------------
#define PLAYER_FLAGS_INUSE		1
#define PLAYER_FLAGS_PLAYER		2
#define PLAYER_FLAGS_OBSERVER	4

#define PLAYER_MAXNAMELEN		63

#define PLAYER_DELTAUPD_FLAGS	1
#define PLAYER_DELTAUPD_IOBJECT	2
#define PLAYER_DELTAUPD_FRAGS	4
#define PLAYER_DELTAUPD_NAME	8

class CWC_Player : public CReferenceCount
{
public:
	int16	m_Flags;
	CStr	m_Name;
	uint8	m_iTeam;

	TPtr<CCmdQueue> m_spCmdQueue;
//	TPtr<TQueue<CControlFrame> > m_spControlQueue;
	CControlFrame m_CurrentControl;
	int m_ServerControlSerial;
	int m_ServerControlAccumTime;

//	CStatusBarInfo m_StatusBar;

	void Clear();
	void SetName(const char* _pName);
	CStr GetName();

	void operator= (const CWC_Player& _p);
	CWC_Player();
	void AddToPacket(CNetMsg& _Packet);
	void GetFromPacket(const CNetMsg& _Packet, int &_Pos);

	bool Equal(const CWC_Player* _pOld);
	void DeltaAddToPacket(CNetMsg& _Packet, const CWC_Player* _pOld);
	void DeltaGetFromPacket(const CNetMsg& _Packet, int &_Pos);

	CStr Dump();
};


// -------------------------------------------------------------------
//  CNetTransferBuffer
// -------------------------------------------------------------------
class CNetTransferInstance
{
public:
	int m_IOPos;
	int m_LastGetSize;

	void Clear()
	{
		m_IOPos = 0;
		m_LastGetSize = 0;
	}

	CNetTransferInstance()
	{
		Clear();
	}
};

// -------------------------------------------------------------------
class CNetTransferBufferBase
{
	TArray<uint8> m_lBuffer;
	bool m_bOut;
	bool m_bDynamic;
public:
	int m_Size;

	CNetTransferBufferBase();

	void Create(int _Size, bool _bOut, bool _bDynamic);
	void Create(TArray<uint8> _lBuffer, bool _bOut, bool _bDynamic);

	TArray<uint8>& GetBuffer();

	void InitializeTransfer(CNetTransferInstance& _Transfer);						// Prepare transfer instance.
	bool AddPacket(CNetTransferInstance& _Transfer, const CNetMsg& _Packet);		// Add incoming packet to buffer.

	bool GetPacket(CNetTransferInstance& _Transfer, CNetMsg& _Packet, int _MsgType);	// Get outgoing packet from the current IO position.
	bool GetNext(CNetTransferInstance& _Transfer);									// Advance IO position when a packet was succesfully sent.
};


class CNetTransferBuffer : public CNetTransferBufferBase, public CReferenceCount
{
};

typedef TPtr<CNetTransferBuffer> spCNetTransferBuffer;

// -------------------------------------------------------------------
//  CWObjectUpdateBuffer (formerly CDeltaUpdateBuffer)
// -------------------------------------------------------------------

// A CWObjectUpdateBuffer is a utility class to simplify network transfer of big datablocks.
// It has no 'delta-update' functionality, as the name implies.

#define WORLD_MAXDELTAFRAMESIZE 4096		// FIXME: Should be defined dynamically
#define DELTABUFFER_HEADERDATA 11

#if !defined(M_RTM) || defined(M_Profile)
# define ENABLE_CLIENTUPDATE_SIZECHECK
#endif

class CWObjectUpdateBuffer : protected CNetTransferBufferBase, public CReferenceCount
{
public:
	CNetTransferInstance m_Transfer;	// The object-update buffer is client-specific, so we only need a sole instance.

	int m_nObjects;
	int m_nSimTicks;
	int m_ControlSerial;
	int m_ControlAccumTime;
	int m_SimulationTick;

	CWObjectUpdateBuffer();

	int GetSize() { return m_Size; };
	int GetDataSize() { return m_Size - DELTABUFFER_HEADERDATA; };
	uint8* GetObjectBuffer() { return &GetBuffer()[DELTABUFFER_HEADERDATA]; };
	uint8* GetBufferPos() { return &GetBuffer()[m_Size]; };
	int BufferLeft() { return WORLD_MAXDELTAFRAMESIZE - m_Size; };
	void IncSize(int _Size) { m_Size += _Size; };

	void Clear();
	bool AddDeltaUpdate(uint8* _pBuffer, int _Size);

	// Out
	bool GetPacket(CNetMsg& _Packet, int _MsgType);
	bool GetNext();

	// In
	bool AddPacket(const CNetMsg& _Packet);
};

typedef TPtr<CWObjectUpdateBuffer> spCWObjectUpdateBuffer;

// -------------------------------------------------------------------
//  CDemoMsgStream
// -------------------------------------------------------------------

// A CDemoMsgStream is a buffer containing recorded messages.

class CDemoMsgStream : public CReferenceCount
{
	TArray<uint8> m_lMsgBuffer;
	int m_BufferPos;

public:
	CDemoMsgStream();
	void Clear();
	void Read(CDataFile* _pDFile);
	bool MsgAvail() const;
	fp32 GetNextTimeStamp() const;
	const CNetMsg* GetNextMsg();
};

#endif
