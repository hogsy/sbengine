
#include "PCH.h"


#include "WClient_Core.h"
#include "../../../XR/XREngineVar.h"
#include "../WPackets.h"

fp32 CWorld_ClientCore::GetCmdDeltaTime()
{
	MAUTOSTRIP(CWorld_ClientCore_GetCmdDeltaTime, 0.0);
	// NOTE: Returns delta in ms, unlike all other time that is in seconds.

//	CMTime Time = CMTime::GetCPU();
	CMTime Time;
	Time.Snapshot();

	if (m_ClientMode & WCLIENT_MODE_PAUSE)
	{
		m_LastCmdTime = Time;
		return 0;
	}

	if (m_LastCmdTime.IsReset())
	{
		m_LastCmdTime = Time;
		return 0;
	}
	else
	{
		fp32 dTime = (Time - m_LastCmdTime).GetTime() * m_TimeScale *1000.0;
		if (dTime > 250)
		{
			dTime = 250;
			m_LastCmdTime = Time;
		}
		else
		{
			dTime = M_Floor(dTime);
			m_LastCmdTime += CMTime::CreateFromSeconds(dTime / (m_TimeScale * 1000.0));
		}
		return dTime;
	}
}

bool CWorld_ClientCore::AddCmd(const CCmd& _Cmd)
{
	MAUTOSTRIP(CWorld_ClientCore_AddCmd, false);
	if (m_ClientMode & WCLIENT_MODE_PAUSE)
		return false;

	return AddCmd_Forced(_Cmd);
}

bool CWorld_ClientCore::AddCmd_Forced(const CCmd& _Cmd)
{
	MAUTOSTRIP(CWorld_ClientCore_AddCmd_Forced, false);
	if (_Cmd.m_Size > CWCLIENT_MAXCMDSIZE)
		Error("AddCmd", "Invalid CCmd size. You've probably trashed your stackframe. lol!");

	if (!m_LocalPlayer.m_spCmdQueue->AddCmd(_Cmd))
	{
		ConOut("(CWorld_Client::Net_OnMessage) Cmd buffer full. Zonking it...");
		while(!m_LocalPlayer.m_spCmdQueue->Empty())
			m_LocalPlayer.m_spCmdQueue->Get();

		return false;
	}
	return true;
}

// -------------------------------------------------------------------
//  Some generic client player-commands
// -------------------------------------------------------------------
void CWorld_ClientCore::Con_Cmd(int _cmd)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_Cmd, MAUTOSTRIP_VOID);
	CCmd Cmd;
	Cmd.Create_Cmd0(_cmd, int(GetCmdDeltaTime()));
	AddCmd(Cmd);
}

void CWorld_ClientCore::Con_Cmd2(int _cmd, int d0)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_Cmd2, MAUTOSTRIP_VOID);
	CCmd Cmd;
	Cmd.Create_Cmd1(_cmd, d0, int(GetCmdDeltaTime()));
	AddCmd(Cmd);
}

void CWorld_ClientCore::Con_Cmd3(int _cmd, int d0, int d1)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_Cmd3, MAUTOSTRIP_VOID);
	CCmd Cmd;
	Cmd.Create_Cmd2(_cmd, d0, d1, int(GetCmdDeltaTime()));
	AddCmd(Cmd);
}

void CWorld_ClientCore::Con_Cmd4(int _cmd, int d0, int d1, int d2)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_Cmd4, MAUTOSTRIP_VOID);
	CCmd Cmd;
	Cmd.Create_Cmd3(_cmd, d0, d1, d2, int(GetCmdDeltaTime()));
	AddCmd(Cmd);
}

// -------------------------------------------------------------------
void CWorld_ClientCore::Con_Cmd_Forced(int _cmd)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_Cmd_Forced, MAUTOSTRIP_VOID);
	CCmd Cmd;
	Cmd.Create_Cmd0(_cmd, int(GetCmdDeltaTime()));
	AddCmd_Forced(Cmd);
}

void CWorld_ClientCore::Con_Cmd2_Forced(int _cmd, int d0)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_Cmd2_Forced, MAUTOSTRIP_VOID);
	CCmd Cmd;
	Cmd.Create_Cmd1(_cmd, d0, int(GetCmdDeltaTime()));
	AddCmd_Forced(Cmd);
}

void CWorld_ClientCore::Con_Cmd3_Forced(int _cmd, int d0, int d1)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_Cmd3_Forced, MAUTOSTRIP_VOID);
	CCmd Cmd;
	Cmd.Create_Cmd2(_cmd, d0, d1, int(GetCmdDeltaTime()));
	AddCmd_Forced(Cmd);
}

void CWorld_ClientCore::Con_Cmd4_Forced(int _cmd, int d0, int d1, int d2)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_Cmd4_Forced, MAUTOSTRIP_VOID);
	CCmd Cmd;
	Cmd.Create_Cmd3(_cmd, d0, d1, d2, int(GetCmdDeltaTime()));
	AddCmd_Forced(Cmd);
}

// -------------------------------------------------------------------
void CWorld_ClientCore::Con_NetLoad(int _v)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_NetLoad, MAUTOSTRIP_VOID);
	m_bNetLoad = _v;
}

void CWorld_ClientCore::Con_NetShowMsg(int _v)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_NetShowMsg, MAUTOSTRIP_VOID);
	m_bNetShowMsg = _v;
}

void CWorld_ClientCore::Con_NetRate(int _Rate)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_NetRate, MAUTOSTRIP_VOID);
	if (m_spNetwork != NULL && m_hConnection >= 0)
	{
		if (_Rate >= 500 && _Rate < 1000000)
		{
//			m_spNetwork->SetConnectionRate(m_hConnection, _Rate);
			m_spNetwork->Connection_SetRate(m_hConnection, 100000);

			MACRO_GetSystemEnvironment(pEnv);
			if (pEnv) pEnv->SetValuei("NET_RATE", _Rate);

			Net_SetClientVar("RATE", CStrF("%d", _Rate));
		}
		else
			ConOut("Invalid rate.");
	}
	else
		ConOut("Client is not using the network.");
}

void CWorld_ClientCore::Con_NoDraw(int _v)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_NoDraw, MAUTOSTRIP_VOID);
	m_ClientDrawDisable = _v;
}

void CWorld_ClientCore::Con_Say(CStr _s)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_Say, MAUTOSTRIP_VOID);
	if (!_s.Len()) return;
	CNetMsg Msg(WPACKET_SAY);
	Msg.AddInt16(0);
	Msg.AddStr((const char*)_s);
	Net_PutMsg(Msg);
}

void CWorld_ClientCore::Con_SayTeam(CStr _s)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_SayTeam, MAUTOSTRIP_VOID);
	if (!_s.Len()) return;
	CNetMsg Msg(WPACKET_SAY);
	Msg.AddInt16(1);
	Msg.AddStr((const char*)_s);
	Net_PutMsg(Msg);
}

void CWorld_ClientCore::Con_ShowObjPhys(int _v)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_ShowObjPhys, MAUTOSTRIP_VOID);
	m_bShowObjPhys = _v;
}

void CWorld_ClientCore::Con_ShowGraphs(int _v)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_ShowGraphs, MAUTOSTRIP_VOID);
	m_ShowGraphs = _v;
}

void CWorld_ClientCore::Con_PerfGraph(int _v)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_PerfGraph, MAUTOSTRIP_VOID);
#ifndef DEF_DISABLE_PERFGRAPH
	m_PerfGraph = _v;
#endif
}

void CWorld_ClientCore::Con_PhysRender(int _v)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_PhysRender, MAUTOSTRIP_VOID);
	m_bPhysRender = _v;
}

void CWorld_ClientCore::Con_HeadLightRange(fp32 _v)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_HeadLightRange, MAUTOSTRIP_VOID);
	m_HeadLightRange = _v;
}

void CWorld_ClientCore::Con_HeadLightIntensity(fp32 _r, fp32 _g, fp32 _b)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_HeadLightIntensity, MAUTOSTRIP_VOID);
	m_HeadLightIntensity = CVec3Dfp32(_r, _g, _b);
}

void CWorld_ClientCore::Con_TimeScale(fp32 _v)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_TimeScale, MAUTOSTRIP_VOID);
	if (m_RecMode != WCLIENT_RECMODE_PLAYBACK)
	{
		ConOut("Can only change client time-scale for demo playback. Use sv_timescale for all other situations.");
		return;
	}
	m_TimeScale = _v;
	m_TickRealTime = m_TickTime / m_TimeScale;
}

void CWorld_ClientCore::Con_RCon_Password(CStr _s)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_RCon_Password, MAUTOSTRIP_VOID);
	m_RCon_Password = _s;
	ConOut("Remote control password changed to: " + _s);
}

void CWorld_ClientCore::Con_RCon(CStr _Cmd)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_RCon, MAUTOSTRIP_VOID);
	if (!m_RCon_Password.Len())
	{
		ConOut("No remote control password.");
		return;
	}
	if (!_Cmd.Len()) return;

	CNetMsg Msg(WPACKET_COMMAND);
	Msg.AddStr((char*)m_RCon_Password);
	Msg.AddStr((const char*)_Cmd);
	Net_PutMsg(Msg);
}

void CWorld_ClientCore::Con_AddBufferSkip(int _v)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_AddBufferSkip, MAUTOSTRIP_VOID);
	if (m_spEngine != NULL)
		m_spEngine->SetVar(XR_ENGINE_VBSKIP, m_spEngine->GetVar(XR_ENGINE_VBSKIP) + _v);
}

void CWorld_ClientCore::Con_Set(CStr _k, CStr _v)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_Set, MAUTOSTRIP_VOID);
	Net_SetClientVar(_k, _v);
}

void CWorld_ClientCore::Con_Name(CStr _s)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_Name, MAUTOSTRIP_VOID);
	Net_SetClientVar("NAME", _s);
}

void CWorld_ClientCore::Con_Team(CStr _s)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_Team, MAUTOSTRIP_VOID);
	Net_SetClientVar("TEAM", _s);
}

void CWorld_ClientCore::Con_SetXYZCameraValues(fp32 _x, fp32 _y, fp32 _z)
{
	MAUTOSTRIP(CWordl_ClientCore_SetXYZCameraValues, MAUTOSTRIP_VOID);
	CVec3Dfp32 CameraTweak(_x, _y, _z);
	CStr Str = CameraTweak.GetString();
	m_spGameReg->SetValue("XYZ_Camera", Str);
}

void CWorld_ClientCore::Con_OpenGameMenu()
{
	MAUTOSTRIP(CWorld_ClientCore_Con_OpenGameMenu, MAUTOSTRIP_VOID);
	if(GetClientState() == WCLIENT_STATE_INGAME)
	{
		CStr Menu = "GAMEMENU";
		const CRegistry *pReg = Registry_Get(WCLIENT_REG_GAMESTATE);
		if(pReg)
		{
			pReg = pReg->FindChild("GAMEMENU");
			if(pReg)
				Menu = pReg->GetThisValue();
		}

		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if(pSys && pSys->GetEnvironment())
		{
			CStr St2 = pSys->GetEnvironment()->GetValue("GUI_INGAMEMENU");
			if(St2 != "")
				Menu = St2;
		}
			

		ConExecute(CStrF("cg_cubeseq(\"fromgame\"); cg_rootmenu(\"%s\")", Menu.Str()));
//		ConExecute("pause(1)");
	}
}

void CWorld_ClientCore::Con_SyncOnClientRender(int _v)
{
	m_bSyncOnClientRender = _v != 0;
	ConOut(CStrF("OnClientRender calls no %s", _v?"Sync":"Async"));
}

void CWorld_ClientCore::Register(CScriptRegisterContext & _RegContext)
{
	MAUTOSTRIP(CWorld_ClientCore_Register, MAUTOSTRIP_VOID);
//	ConOutL(CStrF("(CWorld_ClientCore::Register) Prefix %s, ControlPrefix %s", (char*)m_CmdPrefix, (char*)m_CmdPrefix_Controls ));

	CStr CmdPrefix = m_CmdPrefix;
	CStr CmdPrefix_Controls = m_CmdPrefix_Controls;

	// Generic player control commands (no cl_ prefix)
	_RegContext.RegFunction(CmdPrefix_Controls + "impulse", this, &CWorld_ClientCore::Con_Cmd);
	_RegContext.RegFunction(CmdPrefix_Controls + "cmd", this, &CWorld_ClientCore::Con_Cmd);
	_RegContext.RegFunction(CmdPrefix_Controls + "cmd2", this, &CWorld_ClientCore::Con_Cmd2);
	_RegContext.RegFunction(CmdPrefix_Controls + "cmd3", this, &CWorld_ClientCore::Con_Cmd3);
	_RegContext.RegFunction(CmdPrefix_Controls + "cmd4", this, &CWorld_ClientCore::Con_Cmd4);

	_RegContext.RegFunction(CmdPrefix_Controls + "cmd_forced", this, &CWorld_ClientCore::Con_Cmd_Forced);
	_RegContext.RegFunction(CmdPrefix_Controls + "cmd2_forced", this, &CWorld_ClientCore::Con_Cmd2_Forced);
	_RegContext.RegFunction(CmdPrefix_Controls + "cmd3_forced", this, &CWorld_ClientCore::Con_Cmd3_Forced);
	_RegContext.RegFunction(CmdPrefix_Controls + "cmd4_forced", this, &CWorld_ClientCore::Con_Cmd4_Forced);

	// Client commands without cl_ prefix.
	_RegContext.RegFunction(CmdPrefix + "say", this, &CWorld_ClientCore::Con_Say);
	_RegContext.RegFunction(CmdPrefix + "say_team", this, &CWorld_ClientCore::Con_SayTeam);
	_RegContext.RegFunction(CmdPrefix + "name", this, &CWorld_ClientCore::Con_Name);
	_RegContext.RegFunction(CmdPrefix + "team", this, &CWorld_ClientCore::Con_Team);

	// Client commands
	_RegContext.RegFunction(CmdPrefix + "netload", this, &CWorld_ClientCore::Con_NetLoad);
	_RegContext.RegFunction(CmdPrefix + "netshowmsg", this, &CWorld_ClientCore::Con_NetShowMsg);
	_RegContext.RegFunction(CmdPrefix + "netrate", this, &CWorld_ClientCore::Con_NetRate);
	_RegContext.RegFunction(CmdPrefix + "nodraw", this, &CWorld_ClientCore::Con_NoDraw);
	_RegContext.RegFunction(CmdPrefix + "showobjphys", this, &CWorld_ClientCore::Con_ShowObjPhys);
	_RegContext.RegFunction(CmdPrefix + "showgraphs", this, &CWorld_ClientCore::Con_ShowGraphs);
	_RegContext.RegFunction(CmdPrefix + "perfgraph", this, &CWorld_ClientCore::Con_PerfGraph);
	_RegContext.RegFunction(CmdPrefix + "physrender", this, &CWorld_ClientCore::Con_PhysRender);
	_RegContext.RegFunction(CmdPrefix + "headlightrange", this, &CWorld_ClientCore::Con_HeadLightRange);
	_RegContext.RegFunction(CmdPrefix + "headlightintensity", this, &CWorld_ClientCore::Con_HeadLightIntensity);
	_RegContext.RegFunction(CmdPrefix + "timescale", this, &CWorld_ClientCore::Con_TimeScale);
	_RegContext.RegFunction(CmdPrefix + "opengamemenu", this, &CWorld_ClientCore::Con_OpenGameMenu);

	_RegContext.RegFunction(CmdPrefix + "rcon_password", this, &CWorld_ClientCore::Con_RCon_Password);
	_RegContext.RegFunction(CmdPrefix + "rcon", this, &CWorld_ClientCore::Con_RCon);

	_RegContext.RegFunction(CmdPrefix + "addvbskip", this, &CWorld_ClientCore::Con_AddBufferSkip);

	_RegContext.RegFunction(CmdPrefix + "set", this, &CWorld_ClientCore::Con_Set);

	_RegContext.RegFunction("cl_synconclientrender", this, &CWorld_ClientCore::Con_SyncOnClientRender);
	_RegContext.RegFunction("cl_clearstats", this, &CWorld_ClientCore::ClearStatistics);
	_RegContext.RegFunction("cl_dumpstats", this, &CWorld_ClientCore::DumpStatistics);

	_RegContext.RegFunction("camera_tweak", this, &CWorld_ClientCore::Con_SetXYZCameraValues);
}

