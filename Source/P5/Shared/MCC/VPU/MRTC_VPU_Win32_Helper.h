

MRTCDLLEXPORT void M_ARGLISTCALL MRTC_VPU_Win32_Trace(const char *, ...);
MRTCDLLEXPORT void MRTC_VPU_Win32_Assert(const char* _pMsg, const char* _pFile, int _Line);
MRTCDLLEXPORT void MRTC_VPU_Win32__MRTC_SpinLock_Lock(void* _pLock);
MRTCDLLEXPORT void MRTC_VPU_Win32__MRTC_SpinLock_Unlock(void* _pLock);
MRTCDLLEXPORT void MRTC_VPU_Win32__NThread_CSpinLock_Lock(void* _pLock);
MRTCDLLEXPORT void MRTC_VPU_Win32__NThread_CSpinLock_Unlock(void* _pLock);
MRTCDLLEXPORT void* MRTC_VPU_Win32__GetScratchBuffer(int _Size);


