
// -------------------------------------------------------------------
//  LogToSystemLog
// -------------------------------------------------------------------
void LogToSystemLog(const CStr& _s)
{
	MSCOPE(::LogToSystemLog, MRTC);
	MSCOPE_DISABLE;

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	if (MRTC_GetRD() && (MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_LogFile))
		MRTC_GetRD()->SendData(ERemoteDebug_LogFile, _s.Str(), _s.Len() + 1, false, false);
#endif

#if defined(PLATFORM_CONSOLE)
//	M_TRACEALWAYS("%s\n", _s.Str() );

#ifndef PLATFORM_XBOX
	if (_s.IsAnsi())
		M_TRACEALWAYS("%s\n", _s.Str());
	else
		M_TRACEALWAYS("%s\n", _s.Ansi().Str());
#endif
	
//	MRTC_SystemInfo::OS_Sleep(5);

#else
	MRTC_ObjectManager* pObjMgr = MRTC_GetObjectManager();
	CReferenceCount* pObj = pObjMgr->GetRegisteredObject("SYSTEM.LOG");

	#if defined(COMPILER_RTTI) || defined(M_FAKEDYNAMICCAST)
		ILogFile* pLog = TDynamicCast<ILogFile>(pObj);
	#else
		ILogFile* pLog = (ILogFile*)(pObj);
	#endif

	if (pLog)
	{
		M_LOCK(*(MRTC_GetObjectManager()->m_pGlobalLock));
		pLog->Log(_s);
	}


	if (_s.Find("ERROR:") >= 0)
	{
		pObj = pObjMgr->GetRegisteredObject("SYSTEM.ERRORLOG");

		#if defined(COMPILER_RTTI) || defined(M_FAKEDYNAMICCAST)
			pLog = TDynamicCast<ILogFile>(pObj);
		#else
			pLog = (ILogFile*)(pObj);
		#endif

		if (pLog)
		{
			M_LOCK(*(MRTC_GetObjectManager()->m_pGlobalLock));
			pLog->Log(_s);
		}

	}
#endif
}

void LogToSystemLog(const char* _pStr)
{
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	if (MRTC_GetRD() && (MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_LogFile))
		MRTC_GetRD()->SendData(ERemoteDebug_LogFile, _pStr, strlen(_pStr) + 1, false, false);
#endif
	MSCOPE(::LogToSystemLog, MRTC);
	MSCOPE_DISABLE;

#if defined(PLATFORM_CONSOLE)
	//&& !defined(PLATFORM_XBOX) 
	M_TRACEALWAYS("%s\n", (_pStr) ? _pStr : "");

#else
	MRTC_ObjectManager* pObjMgr = MRTC_GetObjectManager();
	CReferenceCount* pObj = pObjMgr->GetRegisteredObject("SYSTEM.LOG");

	#if defined(COMPILER_RTTI) || defined(M_FAKEDYNAMICCAST)
		ILogFile* pLog = TDynamicCast<ILogFile>(pObj);
	#else
		ILogFile* pLog = (ILogFile*)(pObj);
	#endif

	if (pLog)
	{
		M_LOCK(*(MRTC_GetObjectManager()->m_pGlobalLock));
		pLog->Log(_pStr);
	}


	if (strstr(_pStr, "ERROR:"))
	{
		pObj = pObjMgr->GetRegisteredObject("SYSTEM.ERRORLOG");

		#if defined(COMPILER_RTTI) || defined(M_FAKEDYNAMICCAST)
			pLog = TDynamicCast<ILogFile>(pObj);
		#else
			pLog = (ILogFile*)(pObj);
		#endif

		if (pLog)
		{
			M_LOCK(*(MRTC_GetObjectManager()->m_pGlobalLock));
			pLog->Log(_pStr);
		}

	}
#endif
}

// -------------------------------------------------------------------
//  Progress helper-functions
// -------------------------------------------------------------------
void MRTC_InstallProgressHandler(spCReferenceCount _spObj)
{
	MRTC_ObjectManager* pOM = MRTC_GOM();
	MACRO_GetRegisterObject(CReferenceCount, _pObj, "SYSTEM.PROGRESS");
	if (_pObj) 
	{
		pOM->UnregisterObject(NULL, "SYSTEM.PROGRESS");
		IProgress* pP = TDynamicCast<IProgress>((CReferenceCount*) _spObj);
		pP->m_pNextProgress = TDynamicCast<IProgress>(_pObj);
		pOM->RegisterObject(_spObj, "SYSTEM.PROGRESS");
	}
	else
		pOM->RegisterObject(_spObj, "SYSTEM.PROGRESS");
}

void MRTC_RemoveProgressHandler(spCReferenceCount _spObj)
{
	MRTC_ObjectManager* pOM = MRTC_GOM();
	MACRO_GetRegisterObject(CReferenceCount, _pObj, "SYSTEM.PROGRESS");
	CReferenceCount* _pLast = NULL;
	while(_pObj)
	{
		if (_pObj == (CReferenceCount *)_spObj) break;
		_pLast = _pObj;
		_pObj = TDynamicCast<CReferenceCount>(TDynamicCast<IProgress>(_pObj)->m_pNextProgress);
	}

	if (!_pObj) Error_static("::RemoveProgressHandler", "Progress handler not installed.");

	if (_pLast)
		TDynamicCast<IProgress>(_pLast)->m_pNextProgress = TDynamicCast<IProgress>(_pObj)->m_pNextProgress;
	else
	{
		CReferenceCount* pRef = TDynamicCast<CReferenceCount>(TDynamicCast<IProgress>(_pObj)->m_pNextProgress);
		pOM->UnregisterObject(NULL, "SYSTEM.PROGRESS");
		if(pRef) pOM->RegisterObject(pRef, "SYSTEM.PROGRESS");
	}
}

void MRTC_PushProgress(const char* _pLevelName)
{
	MACRO_GetRegisterObject(IProgress, pProgress, "SYSTEM.PROGRESS");
	if (pProgress) pProgress->Push(_pLevelName);
}

void MRTC_PopProgress()
{
	MACRO_GetRegisterObject(IProgress, pProgress, "SYSTEM.PROGRESS");
	if (pProgress) pProgress->Pop();
}

void MRTC_SetProgress(fp4 _p, const char* _pTaskName)
{
	MACRO_GetRegisterObject(IProgress, pProgress, "SYSTEM.PROGRESS");
	if (pProgress) pProgress->SetProgress(_p, _pTaskName);
}

void MRTC_SetProgressText(const char* _pTaskName)
{
	MACRO_GetRegisterObject(IProgress, pProgress, "SYSTEM.PROGRESS");
	if (pProgress) pProgress->SetProgressText(_pTaskName);
}

void MRTC_InitProgressCount(int _Count, const char* _pTaskName)
{
	MACRO_GetRegisterObject(IProgress, pProgress, "SYSTEM.PROGRESS");
	if (pProgress) pProgress->InitProgressCount(_Count, _pTaskName);
}

void MRTC_IncProgress(const char* _pTaskName)
{
	MACRO_GetRegisterObject(IProgress, pProgress, "SYSTEM.PROGRESS");
	if (pProgress) pProgress->IncProgress(_pTaskName);
}


const uint32 g_IndexRamp32[INDEXRAMPLEN] = 
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 
};

const uint16 g_IndexRamp16[INDEXRAMPLEN] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 
};




#ifdef SSE_MEMCPY 

bool CheckSame(void *Dest, const void *Source, mint Size)
{
	char *DestPtr = (char *)Dest;
	char *SourcePtr = (char *)Source;
	char *EndSourcePtr = (char *)(((DWORD)Source) + Size);
	
	while (SourcePtr < EndSourcePtr)
	{
		if(*((char *)DestPtr) != *((char *)SourcePtr))
			return false;
		++SourcePtr;
		++DestPtr;
	}

	return true;
}

void * __cdecl memcpy(void *Dest, const void *Source, mint Size)
{
	if (!Size || !Dest || !Source)
		return Dest;

	char *DestPtr = (char *)Dest;
	char *SourcePtr = (char *)Source;
	char *EndSourcePtr = (char *)(((DWORD)Source) + Size);
	
	// Read performance is more important than write performance since writes are cached in a good way
	
	// Do Prologue
	if ((DWORD)SourcePtr & 15)
	{
		if ((DWORD)SourcePtr & 1)
		{
			*((char *)DestPtr) = *((char *)SourcePtr);
			++SourcePtr;
			++DestPtr;
		}
		
		if ((EndSourcePtr - SourcePtr) < 2)
			goto Epilogue;
		
		if ((DWORD)SourcePtr & 2)
		{
			*((WORD *)DestPtr) = *((WORD *)SourcePtr);
			SourcePtr += 2;
			DestPtr += 2;
		}
		
		if ((EndSourcePtr - SourcePtr) < 4)
			goto Epilogue;
		
		if ((DWORD)SourcePtr & 4)
		{
			*((DWORD *)DestPtr) = *((DWORD *)SourcePtr);
			SourcePtr += 4;
			DestPtr += 4;
		}
		
		if ((EndSourcePtr - SourcePtr) < 8)
			goto Epilogue;
		
		if ((DWORD)SourcePtr & 8)
		{
			//			*((DWORD *)DestPtr) = *((DWORD *)SourcePtr);
			*((__int64 *)DestPtr) = *((__int64 *)SourcePtr);
			SourcePtr += 8;
			DestPtr += 8;
		}
		
	}
	
	// Do main copy
	if ((EndSourcePtr - SourcePtr) < 32)
		goto Epilogue;
	
	if ((DWORD)DestPtr & 15)
	{
		// Unaligned dest
		int Num = ((((DWORD)EndSourcePtr) - (DWORD)SourcePtr)) / 32;
		
		__asm 
		{
			mov esi, SourcePtr;
			mov edi, DestPtr;
			mov ecx, Num
Loop1:
			movaps  xmm0, [esi];
			movaps  xmm1, [esi+16];
			movups [edi], xmm0;
			movups [edi+16], xmm1;
			
			lea edi, [edi+32]
			lea esi, [esi+32]
				
			dec ecx
			jne Loop1
				
			mov SourcePtr, esi
			mov DestPtr, edi
				
		}			
	}
	else
	{
		// All aligned ok
		int Num = ((((DWORD)EndSourcePtr) - (DWORD)SourcePtr)) / 32;
		
		__asm 
		{
			mov esi, SourcePtr;
			mov edi, DestPtr;
			mov ecx, Num
Loop2:
			movaps  xmm0, [esi];
			movaps  xmm1, [esi+16];
			movaps [edi], xmm0;
			movaps [edi+16], xmm1;
			
			lea edi, [edi+32]
			lea esi, [esi+32]
				
			dec ecx
			jne Loop2
				
			mov SourcePtr, esi
			mov DestPtr, edi
				
		}			
	}
	
Epilogue:

	int NumLeft = EndSourcePtr - SourcePtr;

	while (NumLeft >= 4)
	{
		*((DWORD *)DestPtr) = *((DWORD *)SourcePtr);
		SourcePtr += 4;
		DestPtr += 4;
		NumLeft -= 4;
	}

	if (NumLeft >= 2)
	{
		*((WORD *)DestPtr) = *((WORD *)SourcePtr);
		SourcePtr += 2;
		DestPtr += 2;
	}
	
	if (SourcePtr < EndSourcePtr)
	{
		*((char *)DestPtr) = *((char *)SourcePtr);
		++SourcePtr;
		++DestPtr;
	}

	SCB_ASSERT(EndSourcePtr == SourcePtr, "memcpy");
	SCB_ASSERT(CheckSame(Dest, Source, Size), "memcpy");
	
	return (void *)Dest;
}

#endif

#ifdef MRTC_AUTOSTRIPLOGGER
// -------------------------------------------------------------------
//  CUsageLogger
// -------------------------------------------------------------------
#ifdef PLATFORM_DOLPHIN
FILE* g_UsageLoggerFile = NULL; // HACK, move into class..
#elif defined( PLATFORM_PS2 )
int g_UsageLoggerFile = -1;
#endif

CUsageLogger::CUsageLogger(const char *_pLogString, int _MaxEntries) :
	m_pLogString(_pLogString),
	m_iStringHashIndex(0)
{
	m_pStringHash = MNew(CStringHash);
	m_pStringHash->Create(_MaxEntries, false);
}
	
CUsageLogger::~CUsageLogger()
{
	delete m_pStringHash;
}

void
CUsageLogger::Log(const char *_pIdentifier)
{
	bool bIgnoreHash = (m_pStringHash->GetMaxIDs() >= 10000); //HACK!
	if (bIgnoreHash || m_pStringHash->GetIndex(_pIdentifier) == -1)
	{
		// not found, log and insert
#ifdef PLATFORM_DOLPHIN
		if (!g_UsageLoggerFile)
			g_UsageLoggerFile = fopen("UsageLogger.log", "w");

		if (g_UsageLoggerFile)
		fprintf(g_UsageLoggerFile, "%s%s\n", m_pLogString, _pIdentifier);
		else
			OSReport("%s%s\n", m_pLogString, _pIdentifier);
/*
#elif defined( PLATFORM_PS2 )
		if( g_UsageLoggerFile < 0 )
			g_UsageLoggerFile = sceOpen( "host0:/UsageLogger.log", SCE_WRONLY );
		
		if( g_UsageLoggerFile >= 0 )
		{
			sceWrite( g_UsageLoggerFile, m_pLogString, strlen( m_pLogString ) );
			sceWrite( g_UsageLoggerFile, _pIdentifier, strlen( _pIdentifier ) );
		}
		else
			printf( "%s%s\n", m_pLogString, _pIdentifier );			
*/
#else		
		MRTC_SystemInfo::OS_Trace("%s%s\n", m_pLogString, _pIdentifier);
#endif
		if (!bIgnoreHash)
			m_pStringHash->Insert(m_iStringHashIndex++, _pIdentifier);
	}
}

// -------------------------------------------------------------------
// AutoStrip logging
// -------------------------------------------------------------------

void MRTC_AutoStripLog(const char *_pFunc)
{
	CUsageLogger* pAutostripLogger = MRTC_GetObjectManager()->GetAutostripLogger();
	if (pAutostripLogger)
		pAutostripLogger->Log(_pFunc);
}

#endif
/*
extern "C" void __cdecl _ftol2(void)
{
	float Temp;
	__asm
	{

		fstp [Temp]
	    cvtss2si eax,[Temp]
	}
}
*/
