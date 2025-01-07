
#include "PCH.h"

#include "WServer_Core.h"
#include "../WDataRes_Core.h"

void CWorld_ServerCore::ClearStatistics()
{
	MAUTOSTRIP(CWorld_ServerCore_ClearStatistics, MAUTOSTRIP_VOID);
	m_spWData->ClearClassStatistics();

#ifdef SERVER_STATS
	m_TInterpolate.Reset();
	m_TExecute.Reset();
	m_TSimulate.Reset();
	m_TClients.Reset();
	m_NumSimulate = 0;

	m_nFunc_IntersectPrim = 0;
	m_nFunc_MovePhysical = 0;
	m_nFunc_MovePhysicalQuick = 0;
	m_nFunc_IntersectWorld = 0;
	m_nFunc_IntersectLine = 0;
	m_nFunc_GetMedium = 0;
	m_nFunc_Selection = 0;

	m_TFuncTotal_MovePhysical.Reset();
	m_TFuncTotal_IntersectWorld.Reset();
	m_TFuncTotal_IntersectPrim.Reset();
	m_TFuncTotal_IntersectLine.Reset();
	m_TFuncTotal_GetMedium.Reset();
	m_TFuncTotal_Selection.Reset();
	ClearFrameStatistics();
#endif
}

void CWorld_ServerCore::ClearFrameStatistics()
{
	MAUTOSTRIP(CWorld_ServerCore_ClearFrameStatistics, MAUTOSTRIP_VOID);
#ifdef SERVER_STATS
	m_TFunc_IntersectPrim.Reset();
	m_TFunc_MovePhysical.Reset();
	m_TFunc_IntersectWorld.Reset();
	m_TFunc_IntersectLine.Reset();
	m_TFunc_GetMedium.Reset();
	m_TFunc_Selection.Reset();
#endif
}

void CWorld_ServerCore::AccumulateFrameStatistics()
{
	MAUTOSTRIP(CWorld_ServerCore_AccumulateFrameStatistics, MAUTOSTRIP_VOID);
#ifdef SERVER_STATS
	m_TFuncTotal_IntersectPrim += m_TFunc_IntersectPrim;
	m_TFuncTotal_MovePhysical += m_TFunc_MovePhysical;
	m_TFuncTotal_IntersectWorld += m_TFunc_IntersectWorld;
	m_TFuncTotal_IntersectLine += m_TFunc_IntersectLine;
	m_TFuncTotal_GetMedium += m_TFunc_GetMedium;
	m_TFuncTotal_Selection += m_TFunc_Selection;
	ClearFrameStatistics();
#endif
}

void CWorld_ServerCore::DumpStatistics()
{
	MAUTOSTRIP(CWorld_ServerCore_DumpStatistics, MAUTOSTRIP_VOID);
#ifdef SERVER_STATS
	if (m_NumSimulate && (m_spMapData!=NULL))
	{
		fp32 Mult = 1000.0;

		LogFile("-------------------------------------------------------------------");
		LogFile(" SERVER STATISTICS");
		LogFile("-------------------------------------------------------------------");
		LogFile(CStrF("Number of ticks:              %6d", m_NumSimulate));
		LogFile(CStrF("Object heap:                  %6d", m_lspObjects.Len()));
		LogFile(CStrF("GUIDs used:                   %6d", m_NextGUID));
		LogFile(" ");

		LogFile(CStr("TOTALS:                              Time    Time/Tick"));
		LogFile(CStrF("Simulation:                   %8.2f ms, %8.2f ms", m_TSimulate.GetTime() * Mult, m_TSimulate.GetTime() * Mult / m_NumSimulate));
		LogFile(CStrF("Clients:                      %8.2f ms, %8.2f ms", m_TClients.GetTime() * Mult, m_TClients.GetTime() * Mult / m_NumSimulate));
		LogFile(CStrF("Execution:                    %8.2f ms, %8.2f ms", m_TExecute.GetTime() * Mult, m_TExecute.GetTime() * Mult / m_NumSimulate));
		LogFile(" ");

		LogFile(     "FUNCTIONS:                           Time     Time/Exe    Time/Tick      nX   nX/Tick    Quick%%");
		LogFile(CStrF("MovePhysical:                 %8.2f ms, %8.2f ms, %8.2f ms, %6.d, %8.2f, %8.2f", 
			m_TFunc_MovePhysical.GetTime() * Mult, 
			m_TFunc_MovePhysical.GetTime() * Mult / m_nFunc_MovePhysical,
			m_TFunc_MovePhysical.GetTime() * Mult / m_NumSimulate,
			m_nFunc_MovePhysical, fp32(m_nFunc_MovePhysical) / fp32(m_NumSimulate),
			100*fp32(m_nFunc_MovePhysicalQuick) / fp32(m_nFunc_MovePhysical)));

		LogFile(CStrF("IntersectWorld:               %8.2f ms, %8.2f ms, %8.2f ms, %6.d, %8.2f", 
			m_TFunc_IntersectWorld.GetTime() * Mult,
			m_TFunc_IntersectWorld.GetTime() * Mult / m_nFunc_IntersectWorld,
			m_TFunc_IntersectWorld.GetTime() * Mult/m_NumSimulate,
			m_nFunc_IntersectWorld, fp32(m_nFunc_IntersectWorld) / fp32(m_NumSimulate)));

		LogFile(CStrF("IntersectPrim:                %8.2f ms, %8.2f ms, %8.2f ms, %6.d, %8.2f", 
			m_TFunc_IntersectPrim.GetTime() * Mult,
			m_TFunc_IntersectPrim.GetTime() * Mult / m_nFunc_IntersectPrim,
			m_TFunc_IntersectPrim.GetTime() * Mult/m_NumSimulate,
			m_nFunc_IntersectPrim, fp32(m_nFunc_IntersectPrim) / fp32(m_NumSimulate)));

		LogFile(CStrF("IntersectLine:                %8.2f ms, %8.2f ms, %8.2f ms, %6.d, %8.2f", 
			m_TFunc_IntersectLine.GetTime() * Mult,
			m_TFunc_IntersectLine.GetTime() * Mult / m_nFunc_IntersectLine,
			m_TFunc_IntersectLine.GetTime() * Mult/m_NumSimulate,
			m_nFunc_IntersectLine, fp32(m_nFunc_IntersectLine) / fp32(m_NumSimulate)));

		LogFile(CStrF("GetMedium:                %8.2f ms, %8.2f ms, %8.2f ms, %6.d, %8.2f", 
			m_TFunc_GetMedium.GetTime() * Mult,
			m_TFunc_GetMedium.GetTime() * Mult / m_nFunc_GetMedium,
			m_TFunc_GetMedium.GetTime() * Mult/m_NumSimulate,
			m_nFunc_GetMedium, fp32(m_nFunc_GetMedium) / fp32(m_NumSimulate)));

/*		LogFile(CStrF("MovePhysical:                %6d", m_nFunc_MovePhysical) +
			T_String("", m_TFunc_MovePhysical) + ", " + T_String("", m_TFunc_MovePhysical / fp64(m_nFunc_MovePhysical)) + CStrF(", (%.2f%% quick)", fp32(m_nFunc_MovePhysicalQuick) / fp32(m_nFunc_MovePhysical) ));
		LogFile(CStrF("IntersectWorld:              %6d", m_nFunc_IntersectWorld) +
			T_String("", m_TFunc_IntersectWorld) + ", " + T_String("", m_TFunc_IntersectWorld / fp64(m_nFunc_IntersectWorld)));
		LogFile(CStrF("IntersectPrim:              %6d", m_nFunc_IntersectPrim) +
			T_String("", m_TFunc_IntersectPrim) + ", " + T_String("", m_TFunc_IntersectPrim / fp64(m_nFunc_IntersectPrim)));
*/
		LogFile(" ");
//               CWObject                                0,     0,     0,      0     -   0.00 ms,  -
		LogFile("CLASS-STATISTICS:                      nX  X/Sim   nUpd   bytes  b/Upd        Time        T/Tick        Time/X");

		int nRc = m_spMapData->GetNumResources();
		for(int iRc = 0; iRc < nRc; iRc++)
		{
			CWResource* pRc = m_spMapData->GetResourceAsync(iRc);
			if (!pRc || !(pRc->m_iRcClass == WRESOURCE_CLASS_WOBJECTCLASS))
				continue;
			
			MRTC_CRuntimeClass_WObject* pRTC = m_spMapData->GetResource_Class(iRc);
			CWorldData::CWD_ClassStatistics* pCS = m_spMapData->GetResource_ClassStatistics(iRc);
			if (pCS && pRTC)
			{
				CStr sTimePerExe = (pCS->m_nExecute) ? CStrF("%9.2f ms, ", pCS->m_ExecuteTime.GetTime() * Mult / fp64(pCS->m_nExecute)).GetStr() : "        - ms, ";
				CStr sExeTime = CStrF("%9.2f ms, ", 1000.0*pCS->m_ExecuteTime.GetTime());
				CStr sExeTimePerTick = CStrF("%9.2f ms, ", 1000.0*pCS->m_ExecuteTime.GetTime() / fp32(m_NumSimulate));
				CStr sByterPerUpdate = (pCS->m_nNetUpdates) ?
					CStrF("%7.2f", fp32(pCS->m_NetUpdate) / fp32(pCS->m_nNetUpdates)) : CStr("      -");


				LogFile(CStrF("%-32s   %6d, %5d, %5d, %6d", (char*)pRTC->m_ClassName,
					pCS->m_nExecute, pCS->m_nExecute / m_NumSimulate,
					pCS->m_nNetUpdates, pCS->m_NetUpdate) + 
					sByterPerUpdate + sExeTime + sExeTimePerTick + sTimePerExe);
			}
		}

		LogFile("-------------------------------------------------------------------");
	}
#endif
	ClearStatistics();
}


void CWorld_ServerCore::Dump(int _DumpFlags)
{
	MAUTOSTRIP(CWorld_ServerCore_Dump, MAUTOSTRIP_VOID);
	if(_DumpFlags & WDUMP_SERVER)
	{
		LogFile("-------------------------------------------------------------------");
		LogFile("                            Server-Dump");
		LogFile("-------------------------------------------------------------------");
		LogFile("");

		if(_DumpFlags & WDUMP_OBJECTS)
		{
			LogFile("Server Game-objects:");
			LogFile("--------------------");
			for(int iObj = 0; iObj < m_lspObjects.Len(); iObj++)
			{
				CWObject* pObj = m_lspObjects[iObj];
				if (pObj)
				{
					LogFile(pObj->Dump(m_spMapData, _DumpFlags));
				}
			}
			LogFile("");
			LogFile("");
		}

		if(_DumpFlags & WDUMP_NETREG)
		{
			CRegistry *pReg = Registry_GetClientRoot(0);
			if(pReg)
			{
				LogFile("Server Client 0 Reg:");
				LogFile("--------------------");
				pReg->XRG_LogDump(1);
				LogFile("");
				LogFile("");
			}
		}

		if (_DumpFlags & WDUMP_WDATA)
		{
			LogFile("Server Mapdata:");
			LogFile("---------------");
			m_spMapData->Dump(-1);
			LogFile("");
			LogFile("");
		}
	}

	if(_DumpFlags & WDUMP_MIRRORS)
	{
		for(int i = 0; i < m_lspClients.Len(); i++)
			if (m_lspClients[i] != NULL)
			{
				LogFile("-------------------------------------------------------------------");
				LogFile(CStrF("                        ClientMirror%d-Dump", i));
				LogFile("-------------------------------------------------------------------");
				LogFile("");
				m_lspClients[i]->Dump(_DumpFlags);
			}
	}
}


