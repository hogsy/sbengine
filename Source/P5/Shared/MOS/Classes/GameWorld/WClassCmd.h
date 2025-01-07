#ifndef __WCLASSCMD_H
#define __WCLASSCMD_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Misc client -> server input strucutres

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios, 2003

	Contents:		CCmd
					CControlFrame
					CCmdQueue
\*_____________________________________________________________________________________________*/

#include "WClass.h"

// -------------------------------------------------------------------
/*
	Command flow:

	- A CCmd is generated from executing key-binding.
	- The CCmd is added to client's Queue of CCmds.
	- x times per second CCmds are stuffed into CControlFrames and sent to the server.
	- The server extracts all CCmds from the incomming CControlFrame and adds them to the respective player's queue of CCmds.
	- Before each simulation, the server copies all relevant CCmds from the queue to the 'current' control-frame for the player.

*/
// -------------------------------------------------------------------
#define CWCLIENT_MAXCMDSIZE		32

class CCmd
{
public:
	uint8 m_Cmd;
	uint8 m_Size;
	uint8 m_dTime;				// This value is sometimes the time from the previous cmd, and sometimes the time until the next cmd
	uint8 m_ID;
	uint8 m_Data[CWCLIENT_MAXCMDSIZE];

	CCmd()
	{
		m_Cmd = 0;
		m_Size = 0;
		m_dTime = 0;
		m_ID = 0;
	}

	void Create(int _Cmd, int _Size, int _dTime);
	void Create_StateBits(uint32 _Bits, int _dTime);
	void Create_Cmd0(int _Cmd, int _dTime);
	void Create_Cmd1(int _Cmd, int _d0, int _dTime);
	void Create_Cmd2(int _Cmd, int _d0, int _d1, int _dTime);
	void Create_Cmd3(int _Cmd, int _d0, int _d1, int _d2, int _dTime);
	void Create_Look(const CVec3Dfp32& _v, int _dTime);
	void Create_Move(const CVec3Dfp32& _v, int _dTime);
	// void Create_AIDebugMsg(int _Cmd, int _d0, const CVec3Dfp32& _Dir, const CVec3Dfp32& _Pos, int _dTime);

#ifdef WADDITIONALCAMERACONTROL
	void Create_LookAdditional(const CVec3Dfp32& _v, int _dTime);
	void Create_MoveAdditional(const CVec3Dfp32& _v, int _dTime);
	void Create_StateBitsAdditional(uint32 _Bits, int _dTime);
#endif

	void Create_Aim(const CVec3Dfp32& _v, int _dTime);
};

// -------------------------------------------------------------------
enum
{
	CONTROL_CMD = 0,			// 1 byte cmd, 3 byte data
	CONTROL_STATEBITS,			// 1 word data
	CONTROL_LOOK,				// 3 words vector
	CONTROL_MOVE,				// 3 words vector
	CONTROL_AIM,				// 3 words vector
	CONTROL_ANALOG,				// 1 uint8 value

#ifdef WADDITIONALCAMERACONTROL
	CONTROL_LOOKADDITIONAL,				// 3 words vector
	CONTROL_MOVEADDITIONAL,				// 3 words vector
	CONTROL_STATEBITSADDITIONAL,		// 1 word data
#endif

	CONTROL_LAST,
};

class CControlFrame : public CNetMsg
{
public:
	CControlFrame();
	void operator= (const CNetMsg&);
	void operator= (const CControlFrame&);
//	void SetFrameID(int _ID);
//	int GetFrameID() const;

	int GetCmdStartPos() const;
	bool AddCmd(int _Cmd, int _Size, int _dTime);
	bool AddCmd_StateBits(uint32 _Bits, int _dTime);
	bool AddCmd_Look(const CVec3Dfp32& _v, int _dTime);
	bool AddCmd_Move(const CVec3Dfp32& _v, int _dTime);

	bool AddCmd(const CCmd& _Cmd);

	void GetCmd(int& _Pos, int& _Cmd, int& _Size, int& _dTime) const;
	void GetCmd(int& _Pos, CCmd& _Cmd) const;
	void GetCmd(int& _Pos, CControlFrame& _Cmd) const;
};

// -------------------------------------------------------------------
class CCmdQueue : public TQueue<CCmd>
{
public:
	int m_iSendTail;
	int m_iTmpTail;
	fp32 m_LastAccumTime;
	fp32 m_LastTmpAccumTime;
	CMTime m_SendTime;
	int m_Serial;

	CCmdQueue(int _QueueLen);

	bool AddCmd(const CCmd& _Cmd);
	bool AddCmds(const CControlFrame& _Ctrl);
	bool GetFrame(CControlFrame& _Ctrl, fp32 _dTime, int _StopID = -1);
	bool FrameAvail();
	void DelTailCmd(int _ID, int _AccumTime);
	void DelHead();

	CMTime GetLastSendTime();
	void SetLastSendTime(CMTime _Time);
	bool GetSendFrame(CControlFrame& _Ctrl, fp32 _dTime, int _StopID = -1);

	void ResetTempTail();
	bool GetTempFrame(CControlFrame& _Ctrl, fp32 _dTime, int _StopID = -1);
	bool GetEmptyTempFrame(CControlFrame& _Ctrl, fp32 _dTime);
	bool TempFrameAvail();

	fp32 GetTotalCmdTime();
	fp32 GetTotalTempCmdTime();
	fp32 GetTotalCmdTime_IncAccum();
	fp32 GetTotalTempCmdTime_IncAccum();

	void GetAllNoTime(CControlFrame& _Ctrl);
};

#endif //_INC_WCLASSCMD
