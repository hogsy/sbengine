#include "PCH.h"
#include "WClassCmd.h"
#include "WPackets.h"

// #define ENABLE_CMDLOG
#ifndef M_RTM
//#define ENABLE_CMDQUEUEWARNINGS
#endif


void CCmd::Create(int _Cmd, int _Size, int _dTime)
{
	MAUTOSTRIP(CCmd_Create, MAUTOSTRIP_VOID);
	m_Cmd = _Cmd;
	m_Size = _Size;
	m_dTime = _dTime;
}

void CCmd::Create_StateBits(uint32 _Bits, int _dTime)
{
	MAUTOSTRIP(CCmd_Create_StateBits, MAUTOSTRIP_VOID);
	m_Cmd = CONTROL_STATEBITS;
	m_Size = 4;
	m_dTime = _dTime;
	m_Data[0] = _Bits & 0xff;
	m_Data[1] = (_Bits >> 8) & 0xff;
	m_Data[2] = (_Bits >> 16) & 0xff;
	m_Data[3] = (_Bits >> 24) & 0xff;
}

void CCmd::Create_Cmd0(int _Cmd, int _dTime)
{
	MAUTOSTRIP(CCmd_Create_Cmd0, MAUTOSTRIP_VOID);
	m_Cmd = CONTROL_CMD;
	m_Size = 1;
	m_dTime = _dTime;
	m_Data[0] = _Cmd;
}

void CCmd::Create_Cmd1(int _Cmd, int _d0, int _dTime)
{
	MAUTOSTRIP(CCmd_Create_Cmd1, MAUTOSTRIP_VOID);
	m_Cmd = CONTROL_CMD;
	m_Size = 2;
	m_dTime = _dTime;
	m_Data[0] = _Cmd;
	m_Data[1] = _d0;
}

void CCmd::Create_Cmd2(int _Cmd, int _d0, int _d1, int _dTime)
{
	MAUTOSTRIP(CCmd_Create_Cmd2, MAUTOSTRIP_VOID);
	m_Cmd = CONTROL_CMD;
	m_Size = 3;
	m_dTime = _dTime;
	m_Data[0] = _Cmd;
	m_Data[1] = _d0;
	m_Data[2] = _d1;
}

void CCmd::Create_Cmd3(int _Cmd, int _d0, int _d1, int _d2, int _dTime)
{
	MAUTOSTRIP(CCmd_Create_Cmd3, MAUTOSTRIP_VOID);
	m_Cmd = CONTROL_CMD;
	m_Size = 4;
	m_dTime = _dTime;
	m_Data[0] = _Cmd;
	m_Data[1] = _d0;
	m_Data[2] = _d1;
	m_Data[3] = _d2;
}

void CCmd::Create_Look(const CVec3Dfp32& _v, int _dTime)
{
	MAUTOSTRIP(CCmd_Create_Look, MAUTOSTRIP_VOID);
	CNetMsg Msg;
	Msg.AddVecInt16_Max32768(_v);

	m_Cmd = CONTROL_LOOK;
	m_Size = Msg.m_MsgSize;
	m_dTime = _dTime;
	memcpy(m_Data, Msg.m_Data, Msg.m_MsgSize);
}

void CCmd::Create_Move(const CVec3Dfp32& _v, int _dTime)
{
	MAUTOSTRIP(CCmd_Create_Move, MAUTOSTRIP_VOID);
	CNetMsg Msg;
	Msg.AddVecInt16_Max32768(_v);

	m_Cmd = CONTROL_MOVE;
	m_Size = Msg.m_MsgSize;
	m_dTime = _dTime;
	memcpy(m_Data, Msg.m_Data, Msg.m_MsgSize);
}

/*
void CCmd::Create_AIDebugMsg(int _Cmd, int _d0, const CVec3Dfp32& _Dir, const CVec3Dfp32& _Pos, int _dTime)
{
	MAUTOSTRIP(CCmd_Create_AIDebugMsg, MAUTOSTRIP_VOID);
	CNetMsg Msg;
	Msg.GetInt32(_d0);
	Msg.AddVecInt16_Max32768(_Dir);
	Msg.AddVecInt16_Max32768(_Pos);

	m_Cmd = _Cmd;
	m_Size = Msg.m_MsgSize;
	m_dTime = _dTime;
	memcpy(m_Data, Msg.m_Data, Msg.m_MsgSize);
}
*/

#ifdef WADDITIONALCAMERACONTROL
void CCmd::Create_LookAdditional(const CVec3Dfp32& _v, int _dTime)
{
	MAUTOSTRIP(CCmd_Create_LookAdditional, MAUTOSTRIP_VOID);
	CNetMsg Msg;
	Msg.AddVecInt16_Max32768(_v);

	m_Cmd = CONTROL_LOOKADDITIONAL;
	m_Size = Msg.m_MsgSize;
	m_dTime = _dTime;
	memcpy(m_Data, Msg.m_Data, Msg.m_MsgSize);
}

void CCmd::Create_MoveAdditional(const CVec3Dfp32& _v, int _dTime)
{
	MAUTOSTRIP(CCmd_Create_MoveAdditional, MAUTOSTRIP_VOID);
	CNetMsg Msg;
	Msg.AddVecInt16_Max32768(_v);

	m_Cmd = CONTROL_MOVEADDITIONAL;
	m_Size = Msg.m_MsgSize;
	m_dTime = _dTime;
	memcpy(m_Data, Msg.m_Data, Msg.m_MsgSize);
}

void CCmd::Create_StateBitsAdditional(uint32 _Bits, int _dTime)
{
	MAUTOSTRIP(CCmd_Create_StateBitsAdditional, MAUTOSTRIP_VOID);
	m_Cmd = CONTROL_STATEBITSADDITIONAL;
	m_Size = 4;
	m_dTime = _dTime;
	m_Data[0] = _Bits & 0xff;
	m_Data[1] = (_Bits >> 8) & 0xff;
	m_Data[2] = (_Bits >> 16) & 0xff;
	m_Data[3] = (_Bits >> 24) & 0xff;
}
#endif

void CCmd::Create_Aim(const CVec3Dfp32& _v, int _dTime)
{
	MAUTOSTRIP(CCmd_Create_Aim, MAUTOSTRIP_VOID);
	CNetMsg Msg;
	Msg.AddVecInt16_Max32768(_v);

	m_Cmd = CONTROL_AIM;
	m_Size = Msg.m_MsgSize;
	m_dTime = _dTime;
	memcpy(m_Data, Msg.m_Data, Msg.m_MsgSize);
}

// -------------------------------------------------------------------
//  CCmdQueue
// -------------------------------------------------------------------
CCmdQueue::CCmdQueue(int _QueueLen) :
	TQueue<CCmd>(_QueueLen)
{
	MAUTOSTRIP(CCmdQueue_ctor, MAUTOSTRIP_VOID);
	m_iSendTail = 0;
	m_iTmpTail = 0;
//	m_SendTime = 0;
	m_LastAccumTime = 0;
	m_LastTmpAccumTime = 0;
	m_Serial = 0;
};

bool CCmdQueue::AddCmd(const CCmd& _Cmd)
{
	MAUTOSTRIP(CCmdQueue_AddCmd, false);
	CCmd Cmd = _Cmd;
	Cmd.m_ID = m_Serial;
	m_Serial++;
//ConOutL(CStrF("(%.8x->CCmdQueue::AddCmd) ID %d, dTime %d, Ctrl %d, Size %d", this, _Cmd.m_ID, _Cmd.m_dTime, _Cmd.m_Cmd, _Cmd.m_Size));
	return Put(Cmd);
}

bool CCmdQueue::AddCmds(const CControlFrame& _Msg)
{
	MAUTOSTRIP(CCmdQueue_AddCmds, false);
	int Pos = _Msg.GetCmdStartPos();
	while(Pos < _Msg.m_MsgSize)
	{
//		ConOutL(CStrF("(CCmdQueue::AddCmds) Pos %d, Size %d", Pos, _Msg.m_MsgSize));
		CCmd Cmd;
//		int CmdNr, Size, dTime;
//		_Msg.GetCmd(Pos, CmdNr, Size, dTime);
		_Msg.GetCmd(Pos, Cmd);
// ConOutL(CStrF("(%.8x->CCmdQueue::AddCmds) Pos %d, Size %d, ID %d, dTime %d, Ctrl %d, Size %d", this, Pos, _Msg.m_MsgSize, Cmd.m_ID, Cmd.m_dTime, Cmd.m_Cmd, Cmd.m_Size));
		if (!Put(Cmd)) return false;

//		Pos += Cmd.m_Size;
	}
	return true;
}

bool CCmdQueue::GetFrame(CControlFrame& _Ctrl, fp32 _dTime, int _StopID)
{
	MAUTOSTRIP(CCmdQueue_GetFrame, false);
// ConOutL("(CCmdQueue::GetFrame) Begin");
//	fp32 QTime = GetTotalCmdTime();
//	if (QTime + m_LastAccumTime < _dTime) return false;

	_Ctrl.m_MsgSize = _Ctrl.GetCmdStartPos();

	fp32 Time = m_LastAccumTime;
	while(m_iHead != m_iTail && Time < _dTime)
	{
		CCmd Cmd = m_lBuffer[m_iTail];
		fp32 Dur = fp32(Cmd.m_dTime) * 0.001f;

#ifdef ENABLE_CMDLOG
ConOutL(CStrF("GetFrame Cmd %d, ID %d, dT %d, Time %f, dTime %f", Cmd.m_Cmd, Cmd.m_ID, Cmd.m_dTime, Time, _dTime));
#endif
		if (Time + Dur >= _dTime - 0.0000001f) break;
		Time += Dur;
		if (Time < 0.0f) Time = 0;
		Cmd.m_dTime = Min(255, Max(int(0), int(Time / _dTime * 128.0f) ));

		if (!_Ctrl.AddCmd(Cmd))
		{
#ifdef ENABLE_CMDQUEUEWARNINGS
			ConOutL("(CCmdQueue::GetFrame) Unable to add CMD to frame.");
#endif
			Time -= Dur;
			break;
		}
m_lBuffer[m_iTail].Create(255, 0, 255);	// Destroy used Cmd.
		IncPtr(m_iTail);

		if (Cmd.m_ID == _StopID) break;
	}
	m_LastAccumTime = Time - _dTime;
//	if (m_LastAccumTime < 0.0f)
//		ConOut(CStrF("(CCmdQueue::GetFrame) m_LastAccumTime = %f, Time %f, dTime %f", m_LastAccumTime, Time, _dTime));
	
// ConOutL("(CCmdQueue::GetFrame) End");
	return m_iHead != m_iTail || _Ctrl.m_MsgSize != 0;
//	return _Ctrl.m_MsgSize != 0;
}

bool CCmdQueue::FrameAvail()
{
	MAUTOSTRIP(CCmdQueue_FrameAvail, false);
	return m_iHead != m_iTail;
}


void CCmdQueue::DelTailCmd(int _ID, int _AccumTime)
{
	MAUTOSTRIP(CCmdQueue_DelTailCmd, MAUTOSTRIP_VOID);
	MSCOPESHORT(DelTailCmd);
//ConOutL(CStrF("(CCmdQueue::DelTailCmd) DelTail ID %d, AccumTime %d", _ID, _AccumTime));
	int ID = _ID & 0xff;
	bool bFound = false;
	int iTail = m_iTail;
	while(m_iHead != iTail)
	{
		if (m_lBuffer[iTail].m_ID == ID)
		{
			bFound = true;
			break;
		}
		IncPtr(iTail);
	}

	if (!bFound && !MaxPut())
	{
#ifdef ENABLE_CMDQUEUEWARNINGS
		ConOutL(CStrF("(CCmdQueue::DelTailCmd) Deleting all frames.   %d, Reason: %s", _ID, (!bFound) ? "Frame not found" : "Queue full."));
#endif
		m_Serial += m_lBuffer.Len() + 10;
/*		m_iHead = 0;
		m_iTail = 0;
		m_iSendTail = 0;
		m_iTmpTail = 0;
		m_LastAccumTime = 0;
		m_LastTmpAccumTime = 0;*/

		m_iTail = m_iSendTail;
		m_iTmpTail = m_iSendTail;
		m_LastAccumTime = 0;
		m_LastTmpAccumTime = 0;
	}

	if (bFound)
	{
		while(m_iHead != m_iTail && m_lBuffer[m_iTail].m_ID != ID)
		{
//ConOutL(CStrF("(CCmdQueue::DelTailCmd) Deleting cmd ID %d", m_lBuffer[m_iTail].m_ID));
			
			m_lBuffer[m_iTail].Create(255, 0, 255);	// Destroy used Cmd.
			IncPtr(m_iTail);
		}

		if(m_iHead != m_iTail && m_lBuffer[m_iTail].m_ID == ID)
		{
//ConOutL(CStrF("(CCmdQueue::DelTailCmd) Deleting cmd ID %d (last)", m_lBuffer[m_iTail].m_ID));
			m_lBuffer[m_iTail].Create(255, 0, 255);	// Destroy used Cmd.
			IncPtr(m_iTail);
		}

		m_LastAccumTime = fp32(_AccumTime) / 1000.0f;
	}
}

void CCmdQueue::DelHead()
{
	MAUTOSTRIP(CCmdQueue_DelHead, MAUTOSTRIP_VOID);
	m_iHead = (m_iHead - 1 + m_lBuffer.Len()) % m_lBuffer.Len();
}

CMTime CCmdQueue::GetLastSendTime()
{
	MAUTOSTRIP(CCmdQueue_GetLastSendTime, 0.0f);
	return m_SendTime;
}

void CCmdQueue::SetLastSendTime(CMTime _Time)
{
	MAUTOSTRIP(CCmdQueue_SetLastSendTime, MAUTOSTRIP_VOID);
	m_SendTime = _Time;
}


bool CCmdQueue::GetSendFrame(CControlFrame& _Ctrl, fp32 _dTime, int _StopID)
{
	MAUTOSTRIP(CCmdQueue_GetSendFrame, false);
	// _StopID is not used.

	// ConOutL("(CCmdQueue::GetSendFrame) Begin");
	_Ctrl.m_MsgSize = _Ctrl.GetCmdStartPos();

	while(m_iHead != m_iSendTail)
	{
		if (!_Ctrl.AddCmd(m_lBuffer[m_iSendTail]))
		{
#ifdef ENABLE_CMDQUEUEWARNINGS
			ConOutL("(CCmdQueue::GetSendFrame) Unable to add CMD to send-frame.");
#endif
			break;
		}
		IncPtr(m_iSendTail);
	}
	
	// ConOutL("(CCmdQueue::GetSendFrame) End");
	return _Ctrl.m_MsgSize != 0;
}


void CCmdQueue::ResetTempTail()
{
	MAUTOSTRIP(CCmdQueue_ResetTempTail, MAUTOSTRIP_VOID);
	m_iTmpTail = m_iTail;
	m_LastTmpAccumTime = m_LastAccumTime;
}

bool CCmdQueue::GetTempFrame(CControlFrame& _Ctrl, fp32 _dTime, int _StopID)
{
	MAUTOSTRIP(CCmdQueue_GetTempFrame, false);
	_Ctrl.m_MsgSize = _Ctrl.GetCmdStartPos();

	int nCmd = 0;
	fp32 Time = m_LastTmpAccumTime;
	while(m_iHead != m_iTmpTail && Time < _dTime)
	{
		CCmd Cmd = m_lBuffer[m_iTmpTail];
		fp32 Dur = fp32(Cmd.m_dTime) * 0.001f;
#ifdef ENABLE_CMDLOG
ConOutL(CStrF("TEMPFrame Cmd %d, ID %d, dT %d, Time %f, dTime %f", Cmd.m_Cmd, Cmd.m_ID, Cmd.m_dTime, Time, _dTime));
#endif
		if (Time + Dur >= _dTime-0.0000001f) break;
		Time += Dur;
		if (Time < 0.0f) Time = 0;
		Cmd.m_dTime = Min(255, Max(int(0), int(Time / _dTime * 128.0f) ));
		if (!_Ctrl.AddCmd(Cmd))
		{
#ifdef ENABLE_CMDQUEUEWARNINGS
			ConOutL(CStrF("(CCmdQueue::GetTempFrame) Unable to add CMD to temp-frame. (nCmd %d, Accum %f)", nCmd, m_LastTmpAccumTime ));
#endif
			Time -= Dur;
			break;
		}

		nCmd++;
		IncPtr(m_iTmpTail);

		if (Cmd.m_ID == _StopID) break;
	}
	m_LastTmpAccumTime = Time - _dTime;

	return m_iHead != m_iTmpTail || _Ctrl.m_MsgSize != 0;
//	return _Ctrl.m_MsgSize != 0;
}

bool CCmdQueue::GetEmptyTempFrame(CControlFrame& _Ctrl, fp32 _dTime)
{
	MAUTOSTRIP(CCmdQueue_GetTempFrame, false);
	_Ctrl.m_MsgSize = _Ctrl.GetCmdStartPos();

	fp32 Time = m_LastTmpAccumTime;
	while(m_iHead != m_iTmpTail && Time < _dTime)
	{
		CCmd Cmd = m_lBuffer[m_iTmpTail];
		fp32 Dur = fp32(Cmd.m_dTime) * 0.001f;
		if (Time + Dur < _dTime-0.0000001f)
		{
			// There is a new command before the _dTime window expires..
			return false;
		}

		break;
	}
	m_LastTmpAccumTime = Time - _dTime;

	return true;
}

bool CCmdQueue::TempFrameAvail()
{
	MAUTOSTRIP(CCmdQueue_TempFrameAvail, false);
	return m_iHead != m_iTmpTail;
}

fp32 CCmdQueue::GetTotalCmdTime()
{
	MAUTOSTRIP(CCmdQueue_GetTotalCmdTime, 0.0f);
	// Returns queue duration in seconds.
	fp32 T = 0;
	CCmd* pCmd = GetTail();
	while(pCmd)
	{
		T += pCmd->m_dTime;
		pCmd = GetNext(pCmd);
	}

	return T / 1000.0f;
}

fp32 CCmdQueue::GetTotalTempCmdTime()
{
	MAUTOSTRIP(CCmdQueue_GetTotalTempCmdTime, 0.0f);
	// Returns queue duration in seconds.
	fp32 T = 0;
	int iTail = m_iTmpTail;
	while(iTail != m_iHead)
	{
		T += m_lBuffer[iTail].m_dTime;
		IncPtr(iTail);
	}

	return T / 1000.0f;
}

fp32 CCmdQueue::GetTotalCmdTime_IncAccum()
{
	MAUTOSTRIP(CCmdQueue_GetTotalCmdTime_IncAccum, 0.0f);
	return GetTotalCmdTime() + m_LastAccumTime;
}

fp32 CCmdQueue::GetTotalTempCmdTime_IncAccum()
{
	MAUTOSTRIP(CCmdQueue_GetTotalTempCmdTime_IncAccum, 0.0f);
	return GetTotalTempCmdTime() + m_LastTmpAccumTime;
}

void CCmdQueue::GetAllNoTime(CControlFrame& _Ctrl)
{
	MAUTOSTRIP(CCmdQueue_GetTempFrame, false);
	_Ctrl.m_MsgSize = _Ctrl.GetCmdStartPos();

	int nCmd = 0;
	while(m_iHead != m_iTmpTail)
	{
		CCmd Cmd = m_lBuffer[m_iTmpTail];
//		fp32 Dur = fp32(Cmd.m_dTime) * 0.001f;
#ifdef ENABLE_CMDLOG
	ConOutL(CStrF("TEMPFrame Cmd %d, ID %d, dT %d, Time %f, dTime %f", Cmd.m_Cmd, Cmd.m_ID, Cmd.m_dTime, Time, _dTime));
#endif
		Cmd.m_dTime = 0;
		if (!_Ctrl.AddCmd(Cmd))
		{
#ifdef ENABLE_CMDQUEUEWARNINGS
			ConOutL(CStrF("(CCmdQueue::GetTempFrame) Unable to add CMD to temp-frame. (nCmd %d, Accum %f)", nCmd, m_LastTmpAccumTime ));
#endif
			break;
		}

		nCmd++;
		IncPtr(m_iTmpTail);
	}
}


// -------------------------------------------------------------------
//  CControlFrame
// -------------------------------------------------------------------
CControlFrame::CControlFrame()
{
	MAUTOSTRIP(CControlFrame_ctor, MAUTOSTRIP_VOID);
	m_MsgType = WPACKET_CONTROLS;
	m_MsgSize = GetCmdStartPos();
//	m_Data[0] = 0;
//	m_Time = 0;
}

void CControlFrame::operator= (const CNetMsg& _Msg)
{
	MAUTOSTRIP(CControlFrame_operator_assign, MAUTOSTRIP_VOID);
	CNetMsg::operator=(_Msg);
}

void CControlFrame::operator= (const CControlFrame& _Ctrl)
{
	MAUTOSTRIP(CControlFrame_operator_assign_2, MAUTOSTRIP_VOID);
	CNetMsg::operator=(_Ctrl);
//	m_Time = _Ctrl.m_Time;
}

/*
void CControlFrame::SetFrameID(int _ID)
{
	MAUTOSTRIP(CControlFrame_SetFrameID, MAUTOSTRIP_VOID);
	m_Data[0] = _ID;
}

int CControlFrame::GetFrameID() const
{
	MAUTOSTRIP(CControlFrame_GetFrameID, 0);
	return m_Data[0];
}
*/

int CControlFrame::GetCmdStartPos() const
{
	MAUTOSTRIP(CControlFrame_GetCmdStartPos, 0);
	return 0;
}

bool CControlFrame::AddCmd(int _Cmd, int _Size, int _dTime)
{
	MAUTOSTRIP(CControlFrame_AddCmd, false);
	if (GetSpaceLeft() < 4) return false;
	AddInt8(_Cmd);
	AddInt8(_Size);
	AddInt8(Max(0, Min(255, _dTime)));
	AddInt8(0);
	return true;
}

bool CControlFrame::AddCmd_StateBits(uint32 _Bits, int _dTime)
{
	MAUTOSTRIP(CControlFrame_AddCmd_StateBits, false);
	if (GetSpaceLeft() < 8) return false;
	AddInt8(CONTROL_STATEBITS);
	AddInt8(4);
	AddInt8(Max(0, Min(255, _dTime)));
	AddInt8(0);
	AddInt32(_Bits);
	return true;
}

bool CControlFrame::AddCmd_Look(const CVec3Dfp32& _v, int _dTime)
{
	MAUTOSTRIP(CControlFrame_AddCmd_Look, false);
	if (GetSpaceLeft() < 10) return false;
	AddInt8(CONTROL_LOOK);
	AddInt8(6);
	AddInt8(Max(0, Min(255, _dTime)));
	AddInt8(0);
	AddVecInt16_Max32768(_v);
	return true;
}

bool CControlFrame::AddCmd_Move(const CVec3Dfp32& _v, int _dTime)
{
	MAUTOSTRIP(CControlFrame_AddCmd_Move, false);
	if (GetSpaceLeft() < 10) return false;
	AddInt8(CONTROL_MOVE);
	AddInt8(6);
	AddInt8(Max(0, Min(255, _dTime)));
	AddInt8(0);
	AddVecInt16_Max32768(_v);
	return true;
}

bool CControlFrame::AddCmd(const CCmd& _Cmd)
{
	MAUTOSTRIP(CControlFrame_AddCmd_2, false);
	if (m_MsgSize + _Cmd.m_Size + 4 > WPACKET_MAXPACKETSIZE) return false;
// ConOutL(CStrF("(%.8x->CControlFrame::AddCmd) Pos %d, ID %d, dTime %d, Ctrl %d, Size %d", this, m_MsgSize, _Cmd.m_ID, _Cmd.m_dTime, _Cmd.m_Cmd, _Cmd.m_Size));
	memcpy(&m_Data[m_MsgSize], &_Cmd, _Cmd.m_Size+4);
	m_MsgSize += _Cmd.m_Size+4;
	return true;
}

void CControlFrame::GetCmd(int& _Pos, int& _Cmd, int& _Size, int& _dTime) const
{
	MAUTOSTRIP(CControlFrame_GetCmd, MAUTOSTRIP_VOID);
	_Cmd = m_Data[_Pos++];
	_Size = m_Data[_Pos++];
	_dTime = m_Data[_Pos++];
	_Pos++;
}

void CControlFrame::GetCmd(int& _Pos, CCmd& _Cmd) const
{
	MAUTOSTRIP(CControlFrame_GetCmd_2, MAUTOSTRIP_VOID);
	int Size = m_Data[_Pos + 1];
	memcpy(&_Cmd, &m_Data[_Pos], Size+4);
// ConOutL(CStrF("(CControlFrame::GetCmd) Pos %d, Size %d, Cmd %d", _Pos, Size, _Cmd.m_Cmd));
	_Pos += (Size + 4);
}

