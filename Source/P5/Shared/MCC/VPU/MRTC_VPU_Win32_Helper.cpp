#include "../Mrtc.h"

#ifdef PLATFORM_XBOX
	#include <xtl.h>
#elif defined PLATFORM_WIN
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif


void M_ARGLISTCALL MRTC_VPU_Win32_Trace(const char * _pStr, ...)
{
#if defined(M_Profile) || defined (PLATFORM_WIN)
	static int ms_iTrace = 0;	// Line count

	char lBuffer[4096];


	if (_pStr)
	{
		va_list arg;
		va_start(arg, _pStr);
		//#if defined(PLATFORM_XBOX)
		CStrBase::vsnprintf((char*) &lBuffer[0], 4095-9, _pStr, arg); 
		//#else
		//		CStrBase::dopr((char*) &lBuffer[0], 4095, _pStr, arg); 
		//#endif
		lBuffer[4095] = 0;
	}
	else
		lBuffer[0] = 0;
/*
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
#ifndef MRTC_ENABLE_REMOTEDEBUGGER_STATIC
	if (MRTC_GetObjectManager() && MRTC_GetObjectManager()->GetRemoteDebugger())
#endif
		if (MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_Trace)
			MRTC_GetRD()->SendData(ERemoteDebug_Trace, lBuffer, strlen(lBuffer) + 1, false, false);
#endif
*/
#if 0//def PLATFORM_WIN_PC
	char buf[100];
	DWORD nSize = 100;
	if (GetComputerName(buf, &nSize))
		OutputDebugStringA(CFStrF("[%s] ", buf));
#endif
	OutputDebugStringA(lBuffer);
	//#if defined(_DEBUG) && defined(PLATFORM_XBOX)
	//	OS_Sleep(2);
	//#endif
#endif
}



MRTCDLLEXPORT void MRTC_VPU_Win32_Assert(const char* _pMsg, const char* _pFile, int _Line)
{
	MRTC_SystemInfo::OS_Assert(_pMsg, _pFile, _Line);
}

MRTCDLLEXPORT void MRTC_VPU_Win32__MRTC_SpinLock_Lock(void* _pLock)
{
	MRTC_SpinLock* pLock=(MRTC_SpinLock*) _pLock;
	pLock->Lock();
}

MRTCDLLEXPORT void MRTC_VPU_Win32__MRTC_SpinLock_Unlock(void* _pLock)
{
	MRTC_SpinLock* pLock=(MRTC_SpinLock*) _pLock;
	pLock->Unlock();
}

MRTCDLLEXPORT void MRTC_VPU_Win32__NThread_CSpinLock_Lock(void* _pLock)
{
	NThread::CSpinLock* Lock=(NThread::CSpinLock*)_pLock;
	Lock->Lock();
}

MRTCDLLEXPORT void MRTC_VPU_Win32__NThread_CSpinLock_Unlock(void* _pLock)
{
	NThread::CSpinLock* Lock=(NThread::CSpinLock*) _pLock;
	Lock->Unlock();
}

MRTCDLLEXPORT void* MRTC_VPU_Win32__GetScratchBuffer(int _Size)
{
	return (void*) MRTC_ScratchPadManager::Get(_Size);
}


