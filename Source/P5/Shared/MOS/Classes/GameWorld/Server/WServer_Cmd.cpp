
#include "PCH.h"

#include "WServer_Core.h"

void CWorld_ServerCore::Con_svinfo(int _Info)
{
	MAUTOSTRIP(CWorld_ServerCore_Con_svinfo, MAUTOSTRIP_VOID);
	switch(_Info)
	{
	case 0 :
		{
			ConOut(CStrF("Object heap:§x200%d", m_lspObjects.Len()));
			ConOut(CStrF("Objects free:§x200%d", m_spObjectHeap->MaxAvail()));
			ConOut(CStrF("Next GUID:§x200%d", m_NextGUID));
			break;
		}
	case 1 :
		{
			ConOut("Clients:");
			for(int i = 0; i < m_lspClientInfo.Len(); i++)
			{
				CWServer_ClientInfo* pC = m_lspClientInfo[i];
				if (pC != NULL)
				{
					CRegistry* pCV = Registry_GetClientVar(i);
					ConOut(CStrF("Client %d, Name %s, Team %s, State %d, Player %d, CLS %d, hConnection %d", 
						i, (char*) pCV->GetValue("NAME"), (char*) pCV->GetValue("TEAM"), pC->m_State, pC->m_iPlayer, pC->m_ChangeLevelState, pC->m_hConnection));
				}
				else
					ConOut(CStrF("Client %d, Unused.", i));
			}
			break;
		}
	default :
		ConOut("Server info");
		ConOut("0 : Object allocation.");
		ConOut("1 : Clients.");
	}
}

void CWorld_ServerCore::Con_svsay(CStr _s)
{
	MAUTOSTRIP(CWorld_ServerCore_Con_svsay, MAUTOSTRIP_VOID);
	Net_ConOut(_s);
}

void CWorld_ServerCore::Con_svvars()
{
	MAUTOSTRIP(CWorld_ServerCore_Con_svvars, MAUTOSTRIP_VOID);
	CRegistry* pReg = Registry_GetServer();
	if (pReg)
	{
		for(int i = 0; i < pReg->GetNumChildren(); i++)
		{
			if (pReg->GetChild(i)->GetNumChildren() == 0)
				ConOut(CStrF("%s = %s", (char*)pReg->GetName(i), (char*)pReg->GetValue(i)));
		}
	}
}

void CWorld_ServerCore::Con_svclvars(int _iClient)
{
	MAUTOSTRIP(CWorld_ServerCore_Con_svclvars, MAUTOSTRIP_VOID);
	CRegistry* pCV = Registry_GetClientVar(m_iRConClient);
	if (pCV)
	{
		for(int i = 0; i < pCV->GetNumChildren(); i++)
		{
			if (pCV->GetChild(i)->GetNumChildren() == 0)
				ConOut(CStrF("%s = %s", (char*)pCV->GetName(i), (char*)pCV->GetValue(i)));
		}
	}
	else
		ConOut("(CWorld_ServerCore::Con_svclvars) Invalid client index.");
}

void CWorld_ServerCore::Con_svgame(CStr _s)
{
	MAUTOSTRIP(CWorld_ServerCore_Con_svgame, MAUTOSTRIP_VOID);
	m_NextGameType = _s;
	ConOut("Game-type will be changed upon restarting.");
}

void CWorld_ServerCore::Con_svrestart()
{
	MAUTOSTRIP(CWorld_ServerCore_Con_svrestart, MAUTOSTRIP_VOID);
	if (m_WorldName != "")
		World_Change(m_WorldName, SERVER_CHANGEWORLD_KEEPRESOURCES);
	else
		ConOut("Server is not running.");
}

void CWorld_ServerCore::Con_svclientvar(int _iClient, CStr _key, CStr _value)
{
	MAUTOSTRIP(CWorld_ServerCore_Con_svclientvar, MAUTOSTRIP_VOID);
	if (m_iRConClient < 0) return;
	Net_SetClientVar(m_iRConClient, _key, _value);
	Message_SendToTarget(CWObject_Message(OBJSYSMSG_GAME_CLIENTVARCHANGED, _iClient), WSERVER_GAMEOBJNAME);
}

void CWorld_ServerCore::Con_svset(CStr _key, CStr _value)
{
	MAUTOSTRIP(CWorld_ServerCore_Con_svset, MAUTOSTRIP_VOID);
	Registry_GetServer()->SetValue(_key, _value);
	OnRegistryChanged();
}

void CWorld_ServerCore::Con_svphysrender(int _Flags)
{
	MAUTOSTRIP(CWorld_ServerCore_Con_svphysrender, MAUTOSTRIP_VOID);
	m_PhysRenderFlags = _Flags;
}

void CWorld_ServerCore::Con_svperfgraph(int _Flags)
{
	MAUTOSTRIP(CWorld_ServerCore_Con_svperfgraph, MAUTOSTRIP_VOID);
#ifndef DEF_DISABLE_PERFGRAPH
	m_PerfGraph = _Flags;
#endif
}

void CWorld_ServerCore::Con_svobjdestroy(int _iObj)
{
	MAUTOSTRIP(CWorld_ServerCore_Con_svobjdestroy, MAUTOSTRIP_VOID);

	Object_Destroy(_iObj);
}

void CWorld_ServerCore::Con_svcharacterpush(int32 _bPush)
{
	m_bCharacterPush = _bPush;
}


void CWorld_ServerCore::Register(CScriptRegisterContext & _RegContext)
{
	MAUTOSTRIP(CWorld_ServerCore_Register, MAUTOSTRIP_VOID);
	_RegContext.RegFunction("sv_info", this, &CWorld_ServerCore::Con_svinfo);
	_RegContext.RegFunction("sv_say ", this, &CWorld_ServerCore::Con_svsay);
	_RegContext.RegFunction("sv_vars", this, &CWorld_ServerCore::Con_svvars);
	_RegContext.RegFunction("sv_clvars", this, &CWorld_ServerCore::Con_svclvars);
	_RegContext.RegFunction("sv_game", this, &CWorld_ServerCore::Con_svgame);
	_RegContext.RegFunction("sv_restart", this, &CWorld_ServerCore::Con_svrestart);
	_RegContext.RegFunction("sv_clientvar", this, &CWorld_ServerCore::Con_svclientvar);
	_RegContext.RegFunction("sv_set", this, &CWorld_ServerCore::Con_svset);
	_RegContext.RegFunction("sv_physrender", this, &CWorld_ServerCore::Con_svphysrender);
	_RegContext.RegFunction("sv_perfgraph", this, &CWorld_ServerCore::Con_svperfgraph);
	_RegContext.RegFunction("sv_objdestroy", this, &CWorld_ServerCore::Con_svobjdestroy);

	_RegContext.RegFunction("sv_characterpush", this, &CWorld_ServerCore::Con_svcharacterpush);

	_RegContext.RegFunction("sv_clearstats", this, &CWorld_ServerCore::ClearStatistics);
	_RegContext.RegFunction("sv_dumpstats", this, &CWorld_ServerCore::DumpStatistics);
}

