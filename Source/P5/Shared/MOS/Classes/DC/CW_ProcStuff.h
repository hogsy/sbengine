#ifndef __INC_CWAsmStuff
#define __INC_CWAsmStuff

extern "C"
{
	void* CW_GetProcAdress(...);
	int RunProc_int(void* _pOwner, void* _pCommand, void* _pCallstack, int _Parameters);
	float RunProc_float(void* _pOwner, void* _pCommand, void* _pCallstack, int _Parameters);
	void* RunProc_ptr(void* _pOwner, void* _pCommand, void* _pCallstack, int _Parameters);
	void RunProc(void* _pOwner, void* _pCommand, void* _pCallstack, int _Parameters);
}

#endif	// __INC_CWAsmStuff
