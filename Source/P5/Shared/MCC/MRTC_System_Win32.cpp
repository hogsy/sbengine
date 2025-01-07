#include "PCH.h"

#if defined( PLATFORM_WIN )
# include <io.h>
# include <direct.h>

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| System stuff
|__________________________________________________________________________________________________
\*************************************************************************************************/
__declspec( thread ) mint g_ThreadTop = 0;

DIdsPInlineS static bint IsGoodStackPtr(void *_pAddr, mint _Len, mint _StackStart)
{
	mint StackStart = _StackStart;
	mint StackEnd = (mint)(void *)&StackStart;
	mint AddrStart = (mint)_pAddr;
	mint AddrEnd = AddrStart + _Len;

	if (AddrEnd < AddrStart)
		return false;

	return AddrEnd <= StackStart && AddrStart >= StackEnd;
}


//#define M_FINDNASTYACCESSESTODELETEDMEMORY
#ifdef PLATFORM_XBOX
#include <xtl.h>
//#ifdef M_Profile
#		include <XbDm.h>
#		pragma comment(lib, "XbDm.lib")
//#endif
#endif

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
#	ifdef PLATFORM_XBOX
#		include <XbDm.h>
#		pragma comment(lib, "XbDm.lib")
#		ifdef PLATFORM_XBOX1
#			ifdef M_Profile
#				pragma comment(lib, "xonline.lib")
#			else
#				pragma comment(lib, "xonlinels.lib")
#			endif
#		endif
#	elif defined(PLATFORM_WIN_PC)
#		include <Winsock2.h>
#		pragma comment(lib, "ws2_32.lib")
#	endif
#endif

#ifdef PLATFORM_WIN_PC
#include <mmsystem.h>

#define ENABLE_STACKTRACING

#include <tlhelp32.h>
#include <Winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#ifdef ENABLE_STACKTRACING
#include <DbgHelp.h>
#include <Shellapi.h>


class CStackTraceInfo
{
public:
	const ch8 *m_pFunctionName;
	const ch8 *m_pModuleName;
	const ch8 *m_pSourceFileName;
	aint m_SourceLine;
};

template<class t_CData>
	mint StrLen(const t_CData *_pStr)
{
	const t_CData *pStr = _pStr;
	while (*pStr)
		++pStr;
	return pStr - _pStr;;
}

template<class t_CData>
	M_INLINE bint CharIsWhiteSpace(const t_CData _Character)
{
	switch (_Character)
	{
	case 32 : return true;
	case 8 : return true;
	case 9 : return true;
	case 10 : return true;
	case 13 : return true;
	}
	return false;			
}

void gfs_GetProgramPath(CFStr &_Str)
{
	CFStr CommandLine;

	int nArgs = 0;

	const char *pCommandLine = GetCommandLineA();
	
	{
		const char *pStr = pCommandLine;
		const char *pStrStart = pStr;
		const char *pStrEnd;
		if (*pStr == '"')
		{
			++pStr;
			pStrStart = pStr;
			while (*pStr && *pStr != '"')
				++pStr;
			pStrEnd = pStr;
		}
		else
		{
			while (*pStr && !CharIsWhiteSpace(*pStr))
				++pStr;
			pStrEnd = pStr;
		}
		int Lenn = pStrEnd - pStrStart;

		for (int i = 0; i < Lenn; ++i)
		{
			CommandLine.GetStr()[i] = pStrStart[i];
		}
		CommandLine.GetStr()[Lenn] = 0;

		CFStr FullFileName;
		ch8 *pFileName;
		char Temp;
		uint32 nNeeded = GetFullPathNameA(CommandLine, 0, &Temp, &pFileName);

		if (nNeeded == 0)
			Win32Err_static("Windows returned an error from GetFullPathName");

		if (!GetFullPathNameA(CommandLine, nNeeded, FullFileName.GetStr(), &pFileName))
			Win32Err_static("Windows returned an error from GetFullPathName");

		FullFileName.GetStr()[nNeeded] = 0;

		CommandLine = FullFileName;
//		CommandLine.Replace('\\', '/');
	}

	_Str = CommandLine;
}

void gfs_GetProgramPath(CStr &_Str)
{
	CStr CommandLine;

	int nArgs = 0;

	LPWSTR* pArgs = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	if (pArgs)
	{
		CommandLine = (wchar *)*pArgs;
		CommandLine = CommandLine.Ansi();
		GlobalFree(pArgs);

		CStr FullFileName;
		ch8 *pFileName;
		char Temp;
		uint32 nNeeded = GetFullPathNameA(CommandLine, 0, &Temp, &pFileName);

		if (nNeeded == 0)
			Win32Err_static("Windows returned an error from GetFullPathName");

		if (!GetFullPathNameA(CommandLine, nNeeded, FullFileName.GetBuffer(nNeeded+1), &pFileName))
			Win32Err_static("Windows returned an error from GetFullPathName");

		CommandLine = FullFileName;
//		CommandLine.Replace('\\', '/');
	}

	_Str = CommandLine;
}

void gfs_GetProgramDirectory(CStr &_Str)
{
	CStr Ret;
	gfs_GetProgramPath(Ret);

	int iFind = Ret.FindReverse("\\");

	if (iFind >= 0)
	{
		Ret.GetStr()[iFind] = 0;
	}
	_Str = Ret;
}

CStr gf_GetProgramDirectory()
{
	CStr Ret;
	gfs_GetProgramDirectory(Ret);
	return Ret;
}

CStr gf_GetProgramPath()
{
	CStr CommandLine;

	gfs_GetProgramPath(CommandLine);

	return CommandLine; 
}


void *WindowFileHelper(const char *_pFileName, bool _bRead, bool _bWrite, bool _bCreate, bool _bTruncate, bool _bDeferClose)
{
	DWORD Access = 0;
	if (_bRead)
		Access |= GENERIC_READ;
	if (_bWrite)
		Access |= GENERIC_WRITE;

	DWORD Dispo = OPEN_EXISTING;
	if (_bCreate && _bTruncate)
		Dispo = CREATE_ALWAYS;
	else if (!_bCreate && _bTruncate)
		Dispo = TRUNCATE_EXISTING;
	else if (_bCreate)
		Dispo = OPEN_ALWAYS;

	//  | FILE_FLAG_NO_BUFFERING
	return CreateFileA(_pFileName, Access, FILE_SHARE_READ, NULL, Dispo, 0, NULL);
}

	class CStackTraceContext
	{
	public:
		CStackTraceContext()
		{
			m_bInitialized = false;
			SymInitialize = DNP;
			SymCleanup = DNP;
			m_hDbgHelp = DNP;
			m_pSymbolInfo = DNP;
			MiniDumpWriteDump = DNP;
			m_bFailedInitialize = false;
			m_hProcess = INVALID_HANDLE_VALUE;
		}

		~CStackTraceContext()
		{
			if (m_bInitialized)
				SymCleanup(m_hProcess);

			if (m_hProcess != INVALID_HANDLE_VALUE)
			{
				CloseHandle(m_hProcess);
			}

			if (m_pSymbolInfo)
				free(m_pSymbolInfo);

			if (m_hDbgHelp)
				FreeLibrary(m_hDbgHelp);

			while (m_TraceInfoTree.GetRoot())
			{
				CLocalStackTraceInfo *pInfo = m_TraceInfoTree.GetRoot();
				if (pInfo->m_pFunctionName)
					free((ch8 *)pInfo->m_pFunctionName);
				if (pInfo->m_pModuleName)
					free((ch8 *)pInfo->m_pModuleName);
				if (pInfo->m_pSourceFileName)
					free((ch8 *)pInfo->m_pSourceFileName);

				m_TraceInfoTree.f_Remove(pInfo);

				delete pInfo;
			}

		}

		class CAVLCompare_CLocalStackTraceInfo;

		class CLocalStackTraceInfo : public CStackTraceInfo
		{
		public:
			CLocalStackTraceInfo()
			{
				m_pFunctionName = DNP;
				m_pModuleName = DNP;
				m_pSourceFileName = DNP;
				m_SourceLine = 0;
				m_RefCount = 1;
			}

			DIdsTreeAVLAligned_Link(CLocalStackTraceInfo, m_AvlLink, mint, CAVLCompare_CLocalStackTraceInfo);
			DLinkDS_Link(CLocalStackTraceInfo, m_UnusedLink);
			mint m_Address;
			mint m_RefCount;
		};

		class CAVLCompare_CLocalStackTraceInfo
		{
		public:
			static aint Compare(const CLocalStackTraceInfo *_pFirst, const CLocalStackTraceInfo *_pSecond, void *_pContext)
			{
				return _pFirst->m_Address - _pSecond->m_Address;
			}

			static aint Compare(const CLocalStackTraceInfo *_pTest, const mint &_Key, void *_pContext)
			{
				return _pTest->m_Address - _Key;
			}
		};

		DIdsTreeAVLAligned_Tree(CLocalStackTraceInfo, m_AvlLink, mint, CAVLCompare_CLocalStackTraceInfo) m_TraceInfoTree;
		DLinkDS_List(CLocalStackTraceInfo, m_UnusedLink) m_Unused;
		typedef DLinkDS_Iter(CLocalStackTraceInfo, m_UnusedLink) CInfoIter;

		MRTC_CriticalSection m_Lock;

		typedef BOOL (__stdcall *PENUMLOADED_MODULES_CALLBACK64)(PSTR ModuleName,DWORD64 ModuleBase,ULONG ModuleSize,PVOID UserContext);
		typedef BOOL (__stdcall *PREAD_PROCESS_MEMORY_ROUTINE64)(HANDLE      hProcess,DWORD64     qwBaseAddress,PVOID       lpBuffer,DWORD       nSize,LPDWORD     lpNumberOfBytesRead);
		typedef PVOID(__stdcall *PFUNCTION_TABLE_ACCESS_ROUTINE64)(HANDLE  hProcess,DWORD64 AddrBase);
		typedef DWORD64(__stdcall *PGET_MODULE_BASE_ROUTINE64)(HANDLE  hProcess,DWORD64 Address);
		typedef DWORD64(__stdcall *PTRANSLATE_ADDRESS_ROUTINE64)(HANDLE    hProcess,HANDLE    hThread,LPADDRESS64 lpaddr);
		typedef BOOL(CALLBACK *PSYMBOL_REGISTERED_CALLBACK64)(HANDLE  hProcess,ULONG   ActionCode,ULONG64 CallbackData,ULONG64 UserContext);

		typedef struct _tagSTACKFRAME64 {
			ADDRESS64   AddrPC;               // program counter
			ADDRESS64   AddrReturn;           // return address
			ADDRESS64   AddrFrame;            // frame pointer
			ADDRESS64   AddrStack;            // stack pointer
			ADDRESS64   AddrBStore;           // backing store pointer
			PVOID       FuncTableEntry;       // pointer to pdata/fpo or NULL
			DWORD64     Params[4];            // possible arguments to the function
			BOOL        Far;                  // WOW far call
			BOOL        Virtual;              // is this a virtual frame?
			DWORD64     Reserved[3];
			KDHELP64    KdHelp;
		} STACKFRAME64, *LPSTACKFRAME64;

		typedef BOOL (__stdcall FSymInitialize)(IN HANDLE hProcess, IN PSTR UserSearchPath, IN BOOL fInvadeProcess);
		typedef DWORD (__stdcall FSymSetOptions)(IN DWORD   SymOptions);
		typedef DWORD (__stdcall FSymGetOptions)();
		typedef BOOL (__stdcall FSymCleanup)(IN HANDLE hProcess);
		typedef BOOL (__stdcall FSymGetSymFromAddr64)(IN HANDLE hProcess,IN DWORD64 Address,OUT PDWORD64 Displacement,IN OUT PIMAGEHLP_SYMBOL64 Symbol);
		typedef BOOL (__stdcall FSymGetLineFromAddr64)(IN HANDLE hProcess,IN DWORD64 dwAddr, OUT PDWORD pdwDisplacement, OUT PIMAGEHLP_LINE64 Line);
		typedef BOOL (__stdcall FSymGetLineFromAddr64)(IN HANDLE hProcess,IN DWORD64 dwAddr, OUT PDWORD pdwDisplacement, OUT PIMAGEHLP_LINE64 Line);
		typedef BOOL (__stdcall FSymGetModuleInfo64)(IN HANDLE hProcess,IN DWORD64 qwAddr, OUT PIMAGEHLP_MODULE64 ModuleInfo);
		typedef BOOL (__stdcall FMiniDumpWriteDump)(HANDLE hProcess,DWORD ProcessId,HANDLE hFile,MINIDUMP_TYPE DumpType,PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
		typedef BOOL (__stdcall FEnumerateLoadedModules64)(HANDLE hProcess,PENUMLOADED_MODULES_CALLBACK64 EnumLoadedModulesCallback,PVOID UserContext);
		typedef BOOL (__stdcall FStackWalk64)(DWORD MachineType,HANDLE hProcess,HANDLE hThread,LPSTACKFRAME64 StackFrame,
			PVOID ContextRecord,PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine, PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress);

		typedef BOOL (__stdcall FSymRegisterCallback64)(HANDLE hProcess,PSYMBOL_REGISTERED_CALLBACK64 CallbackFunction,ULONG64 UserContext);




		FSymSetOptions *SymSetOptions;
		FSymGetOptions *SymGetOptions;
		FSymInitialize *SymInitialize;
		FSymCleanup *SymCleanup;
		FSymGetSymFromAddr64 *SymGetSymFromAddr64;
		FSymGetLineFromAddr64 *SymGetLineFromAddr64;
		FSymGetModuleInfo64 *SymGetModuleInfo64;
		FMiniDumpWriteDump *MiniDumpWriteDump;
		FEnumerateLoadedModules64 *EnumerateLoadedModules64;
		FStackWalk64 *StackWalk64;
		FSymRegisterCallback64 *SymRegisterCallback64;
		void *SymFunctionTableAccess64;
		void *SymGetModuleBase64;

		HMODULE m_hDbgHelp;
		IMAGEHLP_SYMBOL64 *m_pSymbolInfo;
		HANDLE m_hProcess;

		bint m_bInitialized;
		bint m_bFailedInitialize;

		static BOOL CALLBACK MyCallback(HANDLE hProcess,ULONG ActionCode,ULONG64 CallbackData,ULONG64 UserContext)
		{
			if (ActionCode == CBA_DEBUG_INFO)
			{
				LogFile((const char *)CallbackData);
			}

			return false;
		}

		bint Init()
		{
			M_LOCK(m_Lock);
			if (m_bInitialized)
				return true;

			if (m_bFailedInitialize)
				return false;

			if (!m_hDbgHelp)
				m_hDbgHelp = LoadLibraryA("Dbghelp.dll");

			if (!m_hDbgHelp)
			{
				m_bFailedInitialize = true;
				M_TRACEALWAYS("StackTrace: Failed to load DbgHelp.dll\n");
				return false;
			}

			SymInitialize = (FSymInitialize*)GetProcAddress(m_hDbgHelp, "SymInitialize");
			StackWalk64 =  (FStackWalk64*)GetProcAddress(m_hDbgHelp, "StackWalk64");
			SymCleanup = (FSymCleanup*)GetProcAddress(m_hDbgHelp, "SymCleanup");
			SymGetSymFromAddr64 = (FSymGetSymFromAddr64*)GetProcAddress(m_hDbgHelp, "SymGetSymFromAddr64");
			SymGetLineFromAddr64 = (FSymGetLineFromAddr64*)GetProcAddress(m_hDbgHelp, "SymGetLineFromAddr64");
			SymGetModuleInfo64 = (FSymGetModuleInfo64*)GetProcAddress(m_hDbgHelp, "SymGetModuleInfo64");
			MiniDumpWriteDump = (FMiniDumpWriteDump*)GetProcAddress(m_hDbgHelp, "MiniDumpWriteDump");
			EnumerateLoadedModules64 = (FEnumerateLoadedModules64*)GetProcAddress(m_hDbgHelp, "EnumerateLoadedModules64");
			SymFunctionTableAccess64 = GetProcAddress(m_hDbgHelp, "SymFunctionTableAccess64");
			SymGetModuleBase64 =  GetProcAddress(m_hDbgHelp, "SymGetModuleBase64");
			SymSetOptions = (FSymSetOptions *)GetProcAddress(m_hDbgHelp, "SymSetOptions");
			SymGetOptions = (FSymGetOptions *)GetProcAddress(m_hDbgHelp, "SymGetOptions");
			SymRegisterCallback64 = (FSymRegisterCallback64 *)GetProcAddress(m_hDbgHelp, "SymRegisterCallback64");

			if (!SymInitialize || !SymCleanup || !SymGetSymFromAddr64 || !SymGetLineFromAddr64 || !SymGetModuleInfo64 || !MiniDumpWriteDump || !EnumerateLoadedModules64 || !StackWalk64 || !SymFunctionTableAccess64 || !SymGetModuleBase64 || !SymSetOptions || !SymGetOptions || !SymRegisterCallback64)
			{
				m_bFailedInitialize = true;
				FreeLibrary(m_hDbgHelp);
				m_hDbgHelp = DNP;
				M_TRACEALWAYS("---------------------------------------------------------------------------------------------------------------------\n");
				for (int i = 0; i < 25; ++i)
				{
					M_TRACEALWAYS("StackTrace: DbgHelp.dll does not contain the needed functions\n");

				}
				M_TRACEALWAYS("---------------------------------------------------------------------------------------------------------------------\n");
				return false;
			}			

			CStr Strings;
			if (CDiskUtil::DirectoryExists("Z:\\Files\\Symbols"))
			{
				Strings = "symsrv*symsrv.dll*Z:\\Files\\Symbols*http://msdl.microsoft.com/download/symbols";
			}
			else
			{
				gfs_GetProgramDirectory(Strings);

				CStr TempStr;
				GetEnvironmentVariableA("_NT_SYMBOL_PATH", TempStr.GetBuffer(32768), 32768);

				if (TempStr.Len())
					Strings = Strings + ";" + TempStr;
				else
				{

					GetEnvironmentVariableA("_NT_ALTERNATE_SYMBOL_PATH", TempStr.GetBuffer(32768), 32768);

					if (TempStr.Len())
						Strings = Strings + ";" + TempStr;
					else
					{
						GetEnvironmentVariableA("SystemRoot", TempStr.GetBuffer(32768), 32768);

						if (TempStr.Len())
							Strings = Strings + ";" + TempStr + "\\Symbols";

						GetEnvironmentVariableA("PATH", TempStr.GetBuffer(32768), 32768);

						if (TempStr.Len())
							Strings = Strings + ";" + TempStr;
					}
				}
			}


			m_hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, GetCurrentProcessId());
			if (m_hProcess == INVALID_HANDLE_VALUE)
			{
				M_TRACEALWAYS("StackTrace: SymInitialize failed\n");
				m_bFailedInitialize = true;
				return false;
			}

			if (!SymInitialize(m_hProcess, (ch8 *)(const ch8 *)Strings, true))
			{
				M_TRACEALWAYS("StackTrace: SymInitialize failed\n");
				m_bFailedInitialize = true;
				return false;
			}

			SymRegisterCallback64(m_hProcess, MyCallback, NULL);

			uint32 OldOpt = SymGetOptions();
			uint32 NewOpt = OldOpt;

			NewOpt &= ~(SYMOPT_DEFERRED_LOADS); // Remove
			NewOpt |= SYMOPT_CASE_INSENSITIVE | SYMOPT_IGNORE_CVREC | SYMOPT_DEBUG; // Add

			uint32 Test = SymSetOptions(OldOpt);
			uint32 Test1 = SymGetOptions();

			if (!m_pSymbolInfo)
			{
				m_pSymbolInfo = (IMAGEHLP_SYMBOL64 *)malloc(sizeof(IMAGEHLP_SYMBOL64) + 4096);
			}

			m_bInitialized = true;
			return true;
		}


		CLocalStackTraceInfo *Debug_AquireStackTraceInfo(mint _Address)
		{
			M_LOCK(m_Lock);

			if (!m_bInitialized)
				if (!Init())
					return DNP;

			CLocalStackTraceInfo *pLocalInfo = m_TraceInfoTree.FindEqual(_Address);
			if (pLocalInfo)
			{
				if ((++pLocalInfo->m_RefCount) == 1)
				{
					pLocalInfo->m_UnusedLink.Unlink();
				}
				return pLocalInfo;
			}

			DWORD64 Displacement;			
			m_pSymbolInfo->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
			m_pSymbolInfo->MaxNameLength = 4096;
			if (!SymGetSymFromAddr64(m_hProcess, _Address, &Displacement, m_pSymbolInfo))
				return DNP;

			pLocalInfo = DNew(CLocalStackTraceInfo) CLocalStackTraceInfo;
			pLocalInfo->m_Address = _Address;
			m_TraceInfoTree.f_Insert(pLocalInfo);

			mint Len = strlen(m_pSymbolInfo->Name);
			ch8 *pStr;
			pLocalInfo->m_pFunctionName = pStr = (ch8 *)malloc(Len + 1);
			memcpy(pStr, m_pSymbolInfo->Name, Len);
			pStr[Len] = 0;

			{
				IMAGEHLP_LINE64 LineInfo;
				LineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
				DWORD Displacement;
				if (SymGetLineFromAddr64(m_hProcess, _Address, &Displacement, &LineInfo))
				{
					mint Len = strlen(LineInfo.FileName);
					pLocalInfo->m_pSourceFileName = pStr = (ch8 *)malloc(Len + 1);
					memcpy(pStr, LineInfo.FileName, Len);
					pStr[Len] = 0;
					pLocalInfo->m_SourceLine = LineInfo.LineNumber;
				}
			}
			{
				IMAGEHLP_MODULE64 ModuleInfo;
				ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);
				if (SymGetModuleInfo64(m_hProcess, _Address, &ModuleInfo))
				{
					mint Len = strlen(ModuleInfo.ImageName);
					pLocalInfo->m_pModuleName = pStr = (ch8 *)malloc(Len + 1);
					memcpy(pStr, ModuleInfo.ImageName, Len);
					pStr[Len] = 0;
				}
			}
			return pLocalInfo;                
		}

		void RemoveUnused()
		{			
			CInfoIter Iter = m_Unused;
			while (Iter)
			{		
				CLocalStackTraceInfo *pInfo = Iter;
				pInfo->m_UnusedLink.Unlink();

				m_TraceInfoTree.f_Remove(pInfo);

				if (pInfo->m_pFunctionName)
					free((ch8 *)pInfo->m_pFunctionName);
				if (pInfo->m_pModuleName)
					free((ch8 *)pInfo->m_pModuleName);
				if (pInfo->m_pSourceFileName)
					free((ch8 *)pInfo->m_pSourceFileName);

				delete pInfo;

				Iter = m_Unused;
			}			
		}

		void Debug_ReleaseStackTraceInfo(CStackTraceInfo *_pInfo)
		{
			CLocalStackTraceInfo *pInfo = (CLocalStackTraceInfo *)_pInfo;
			M_LOCK(m_Lock);

			DIdsAssert(m_bInitialized, "If we are here we should be initialized");

			if ((--(pInfo)->m_RefCount) == 0)
				m_Unused.Insert(pInfo);

//			if (m_Timer.GetTime() > 10.0)
//			{
				RemoveUnused();
//				m_Timer.Start();
//			}

		}

	};

#endif


#ifdef MRTC_ENABLE_REMOTEDEBUGGER
#ifdef PLATFORM_WIN_PC
#include "psapi.h"
#pragma comment(lib, "psapi.lib")
#endif
//#define OLDREMOTEDEBUGGER
#endif

#ifdef PLATFORM_WIN_PC
void *GetVirtualAddressFromRVA(mint _Base, mint _Rva, IMAGE_NT_HEADERS *_pHeader)
{
	bool bFound = false; 

	PIMAGE_SECTION_HEADER pSectionHeader = IMAGE_FIRST_SECTION( _pHeader ); 
	mint Rva = _Rva;

	for( int i = 0; i < _pHeader->FileHeader.NumberOfSections; i++, pSectionHeader++ )
	{
		DWORD SectionSize = pSectionHeader->Misc.VirtualSize; 

		if( SectionSize == 0 ) // compensate for Watcom linker strangeness, according to Matt Pietrek 
			SectionSize = pSectionHeader->SizeOfRawData; 

		if( ( Rva >= pSectionHeader->VirtualAddress ) && 
			( Rva < pSectionHeader->VirtualAddress + SectionSize ) ) 
		{
			// Yes, the RVA belongs to this section 
			bFound = true; 
			break; 
		}
	}

	if( !bFound ) 
	{
		return NULL;
	}

	mint Diff = (mint)( pSectionHeader->VirtualAddress - pSectionHeader->PointerToRawData ); 
	mint FileOffset = Rva - Diff; 

	return (void *)(_Base + _Rva);
//	return (void *)((mint)_Base+FileOffset);
}
#endif

class MRTC_SystemInfoInternal
{
public:
#ifdef ENABLE_STACKTRACING
	CStackTraceContext *m_pStackTraceContext;
	CStackTraceContext *GetStackTraceContext()
	{
		if (!m_pStackTraceContext)
		{
			m_pStackTraceContext = DNew(CStackTraceContext) CStackTraceContext;
		}
		return m_pStackTraceContext;
	}
#endif
	CStr m_LogFileName;

#ifdef PLATFORM_WIN_PC
	class CThreadTrackingContext
	{
	public:
		int m_bInit;

		CThreadTrackingContext()
		{
			m_bInit = false;
			m_bCopyToUpdate = true;
		}

		~CThreadTrackingContext()
		{
			for (int i = 0; i < m_Threads.Len(); ++i)
			{
				CloseHandle(m_Threads[i].m_Thread);
			}
			m_Threads.Clear();
			for (int i = 0; i < m_ThreadsCopy.Len(); ++i)
			{
				CloseHandle(m_ThreadsCopy[i].m_Thread);
			}
			m_ThreadsCopy.Clear();
		}

		class CThread
		{
		public:
			uint32 m_ThreadID;
			HANDLE m_Thread;
		};

		TArray<CThread> m_Threads;
		TArray<CThread> m_ThreadsCopy;
		bint m_bCopyToUpdate;

		void UpdateCopy()
		{
			M_LOCK(m_Lock);
			if (m_bCopyToUpdate)
			{
				m_bCopyToUpdate = false;
				for (int i = 0; i < m_ThreadsCopy.Len(); ++i)
				{
					CloseHandle(m_ThreadsCopy[i].m_Thread);
				}
				m_ThreadsCopy.Clear();
				m_ThreadsCopy.SetLen(m_Threads.Len());
				for (int i = 0; i < m_Threads.Len(); ++i)
				{
					m_ThreadsCopy[i].m_ThreadID = m_Threads[i].m_ThreadID;
					DuplicateHandle(GetCurrentProcess(), m_Threads[i].m_Thread, GetCurrentProcess(), &m_ThreadsCopy[i].m_Thread, 0, false, DUPLICATE_SAME_ACCESS);
				}
			}
		}

		MRTC_CriticalSection m_Lock;

		void RegisterThread()
		{
			M_LOCK(m_Lock);

			if (!m_bInit)
				Init();

			CThread NewThread;
			NewThread.m_ThreadID = GetCurrentThreadId();

			for (int i = 0; i < m_Threads.Len(); ++i)
			{
				if (m_Threads[i].m_ThreadID == NewThread.m_ThreadID)
					return;
			}
			m_bCopyToUpdate = true;

			OutputDebugString(CFStrF("Adding thread ID to tracking context: %d\n", NewThread.m_ThreadID));
			NewThread.m_Thread = OpenThread(THREAD_ALL_ACCESS, false, NewThread.m_ThreadID);
			m_Threads.Add(NewThread);
		}

		void UnregisterThread()
		{
			M_LOCK(m_Lock);

			uint32 ThreadID = GetCurrentThreadId();

			for (int i = 0; i < m_Threads.Len(); ++i)
			{
				if (m_Threads[i].m_ThreadID == ThreadID)
				{
					OutputDebugString(CFStrF("Removing thread ID from tracking context: %d\n", m_Threads[i].m_ThreadID));
					CloseHandle(m_Threads[i].m_Thread);
					m_Threads.Del(i);
					return;
				}
			}

			m_bCopyToUpdate = true;
		}

		void Init()
		{
			m_bInit = true;
			m_Threads.Clear();
			uint32 ThisProcessID = GetCurrentProcessId();
			HANDLE SnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, ThisProcessID);
			if (SnapShot != INVALID_HANDLE_VALUE)
			{
				THREADENTRY32 Thread;
				memset(&Thread, 0, sizeof(Thread));
				Thread.dwSize = sizeof(Thread);

				uint32 ThisThreadID = GetCurrentThreadId();
				 
				if (Thread32First(SnapShot, &Thread))
				{				
					while (1)
					{
						if (Thread.th32OwnerProcessID == ThisProcessID && Thread.th32ThreadID != ThisThreadID)
						{
							CThread NewThread;
							NewThread.m_ThreadID = Thread.th32ThreadID;
							OutputDebugString(CFStrF("Adding thread ID to tracking context: %d\n", NewThread.m_ThreadID));
							NewThread.m_Thread = OpenThread(THREAD_ALL_ACCESS, false, NewThread.m_ThreadID);
							m_Threads.Add(NewThread);
						}
						if (!Thread32Next(SnapShot, &Thread))
							break;
					}
				}

				CloseHandle(SnapShot);
			}
		}

	};

	CThreadTrackingContext m_ThreadTrackingContext;
#endif

	void PostCreate()
	{
		
	}


#ifdef OLDREMOTEDEBUGGER
	static LPTOP_LEVEL_EXCEPTION_FILTER ms_OldFilter;
	static LONG WINAPI FilterExceptions(_EXCEPTION_POINTERS *ExceptionInfo)
	{
		M_TRACEALWAYS("HHHHHHHHHHMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMmm\n");
		MRTC_SystemInfo::MRTC_GetSystemInfo().m_pInternalData->UpdateSend();
		uint32 Data[3];
		Data[0] = ExceptionInfo->ExceptionRecord->ExceptionCode;
		Data[1] = ExceptionInfo->ExceptionRecord->ExceptionInformation[0];
		Data[2] = ExceptionInfo->ExceptionRecord->ExceptionInformation[1];
#ifdef CPU_AMD64
		MRTC_GetRD()->SendData(ERemoteDebug_UnhandeledException, Data, sizeof(Data), true, false, ExceptionInfo->ContextRecord->Rbp);
#else
		MRTC_GetRD()->SendData(ERemoteDebug_UnhandeledException, Data, sizeof(Data), true, false, ExceptionInfo->ContextRecord->Ebp);
#endif

		MRTC_SystemInfo::MRTC_GetSystemInfo().m_pInternalData->UpdateSend();

		if (ms_OldFilter)
			return ms_OldFilter(ExceptionInfo);
		else
			return EXCEPTION_CONTINUE_SEARCH;
	}
#endif

#ifdef PLATFORM_WIN_PC

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	class CWin32Debugger : public MRTC_Thread
	{
	public:

		class CLoadedDll
		{
		public:
			mint m_Address;
			mint m_Size;
			mint m_bTagged;
			CLoadedDll()
			{
				m_bTagged = 0;
			}

			class CCompare
			{
			public:
				static aint Compare(const CLoadedDll *_pFirst, const CLoadedDll *_pSecond, void *_pContext)
				{
					return _pFirst->m_Address - _pSecond->m_Address;
				}

				static aint Compare(const CLoadedDll *_pTest, const mint &_Key, void *_pContext)
				{
					return _pTest->m_Address - _Key;
				}
			};
			DIdsTreeAVLAligned_Link(CLoadedDll, m_AvlLink, mint, CCompare);
		};

		DIdsTreeAVLAligned_Tree(CLoadedDll, m_AvlLink, mint, CLoadedDll::CCompare) m_LoadedDlls;

		TCPool<CLoadedDll> m_Pool;
		
		MRTC_MutualWriteManyRead m_Lock;

		CWin32Debugger()
		{
			Thread_Create();
			
			m_Recursive = 0;
			m_RecursiveDll = 0;
			m_bForceUpdate = true;
		}

		~CWin32Debugger()
		{
			Thread_Destroy();
			while (m_LoadedDlls.GetRoot())
			{
				CLoadedDll *pDll = m_LoadedDlls.GetRoot();
				m_LoadedDlls.f_Remove(pDll);
				m_Pool.Delete(pDll);
			}
		}

		MRTC_Event m_Event;
		int m_bForceUpdate;

		int Thread_Main()
		{
			MRTC_SystemInfo::Thread_SetName("MRTC Win32 Debugger");
			CMTime NextUpdate = CMTime::GetCPU() + CMTime::CreateFromSeconds(2.0);
			while (!Thread_IsTerminating())
			{
				m_Event.WaitTimeout(0.050f);


				if (m_bForceUpdate)
				{
					m_bForceUpdate = false;
					Update();
				}

				if (!MRTC_GetRD()->m_EnableFlags)
					return 0;
				CMTime Now = CMTime::GetCPU();
				if (Now.Compare(NextUpdate) > 0)
				{
					Update();
					NextUpdate = Now + CMTime::CreateFromSeconds(1.0f);
				}
			}
			return 0;
		}

		void AddDll(const ch8 *_pName, mint _Addr, mint _Size)
		{
			bool bSuccess;
			M_MWMR_MutualLock_MutualUnlock(m_Lock, bSuccess);
			if (!bSuccess || m_RecursiveDll)
				return;
			IMAGE_NT_HEADERS *pHeader = NULL;
			// Look for header information

			uint32 *pSearch = (uint32 *)_Addr;
//			if (!IsBadReadPtr(pSearch, _Size))
			if (!IsBadReadPtr(pSearch, 4096))
			{
				for (int i = 0; i < 4096; i += 4)
				{
					uint32 Find1 = '\0EP\0';
					if (*pSearch == Find1)
					{
						pHeader = (IMAGE_NT_HEADERS *)pSearch;
						break;
					}			
					++pSearch;
				}
			}

			if (!pHeader)
			{
				CLoadedDll *pDll = m_LoadedDlls.FindEqual(_Addr);
				if (pDll)
					pDll->m_bTagged = 2;
				else
				{
					++m_RecursiveDll;
					pDll = m_Pool.New();
					--m_RecursiveDll;
					pDll->m_Address = _Addr;
					pDll->m_bTagged = 2;
					pDll->m_Size = _Size;
					m_LoadedDlls.f_Insert(pDll);
					OutputDebugString(CFStrF("Failed to find PE header for module: %s address 0x%x\n", _pName, _Addr));
				}
			}
			else
			{
				mint Address = (uint32)_Addr;
				CLoadedDll *pDll = m_LoadedDlls.FindEqual(Address);
				if (pDll)
					pDll->m_bTagged = true;
				else
				{
					++m_RecursiveDll;
					pDll = m_Pool.New();
					--m_RecursiveDll;
					OutputDebugString(CFStrF("Sent module load 0x%08x(%d): %s ID: 0x%08x TWO: 0x%08x\n", _Addr, _Size, _pName, pHeader->FileHeader.TimeDateStamp, _Size));
					pDll->m_Address = Address;
					pDll->m_bTagged = true;
					pDll->m_Size = _Size;
					m_LoadedDlls.f_Insert(pDll);

					uint8 Packet[1024];
					uint8 *pPacket = Packet;
					mint Len = strlen(_pName);
					*((uint32 *)pPacket) = (uint32)(mint)_Addr; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
					*((uint32 *)pPacket) = _Size; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
					*((uint32 *)pPacket) = pHeader->FileHeader.TimeDateStamp; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
					*((uint32 *)pPacket) = Len; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
					memcpy(pPacket, _pName, Len); pPacket += Len;
					MRTC_GetRD()->SendData(ERemoteDebug_DllLoad, Packet, pPacket - Packet, false, false);
				}
			}
		}

		void Update()
		{
			// New address, try to find it in loaded dlls
				
			bool bSuccess;
			M_MWMR_MutualLock_MutualUnlock(m_Lock, bSuccess);
			if (m_Recursive || !bSuccess)
			{
				return;
			}

			++m_Recursive;
			DIdsTreeAVLAligned_Iterator(CLoadedDll, m_AvlLink, mint, CLoadedDll::CCompare) Iter = m_LoadedDlls;

			while (1)
			{
				DIdsTreeAVLAligned_Iterator(CLoadedDll, m_AvlLink, mint, CLoadedDll::CCompare) Iter = m_LoadedDlls;
				while (Iter)
				{
					if (Iter->m_bTagged != 2)
					{
						CLoadedDll *pDll = Iter;

						MODULEINFO ModInfo;
						if (!GetModuleInformation(GetCurrentProcess(), (HMODULE)pDll->m_Address, &ModInfo, sizeof(ModInfo)) || ModInfo.SizeOfImage != ModInfo.SizeOfImage)
						{
							 
							OutputDebugString(CFStrF("Sent module unload 0x%08x(%d)\n", pDll->m_Address, pDll->m_Size));
							
							uint8 Packet[16];
							uint8 *pPacket = Packet;
							*((uint32 *)pPacket) = pDll->m_Address; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
							MRTC_GetRD()->SendData(ERemoteDebug_DllUnload, Packet, pPacket - Packet, false, false);

							m_LoadedDlls.f_Remove(pDll);
							m_Pool.Delete(pDll);
							break;
						}
					}
					++Iter;
				}
				if (!Iter)
					break;
			}

			--m_Recursive;
		}

		void AddAddress(mint _Address)
		{
			MEMORY_BASIC_INFORMATION MemoryInfo;
			int Size = sizeof(MemoryInfo);
			memset(&MemoryInfo, 0, Size);
			int nBytes = VirtualQuery((const void *)_Address, &MemoryInfo, Size);
			if (nBytes != sizeof(MemoryInfo))
			{
				return;
			}
			else
			{	
				if (MemoryInfo.Type != MEM_IMAGE)
				{
					AddDll("Dummy Dll", (mint)MemoryInfo.AllocationBase, (mint)MemoryInfo.RegionSize);
					return;
				}
				mint Base = (mint)MemoryInfo.AllocationBase;
				IMAGE_NT_HEADERS *pHeader = NULL;
				// Look for header information

				uint32 *pSearch = (uint32 *)Base;
				for (int i = 0; i < 4096; i += 4)
				{
					uint32 Find1 = '\0EP\0';
					if (*pSearch == Find1)
					{
						pHeader = (IMAGE_NT_HEADERS *)pSearch;
						break;
					}
					++pSearch;
				}

				if (!pHeader)
				{
					AddDll("Dummy Dll", Base, (mint)MemoryInfo.RegionSize);
					return;
				}

				const char *pName;
				CFStr NameTemp;
				if (Base == 0x00400000)
				{
					gfs_GetProgramPath(NameTemp);
					NameTemp = NameTemp.GetFilename();
					pName = NameTemp;
				}
				else
				{

					IMAGE_EXPORT_DIRECTORY *pExport = (IMAGE_EXPORT_DIRECTORY *)GetVirtualAddressFromRVA(Base, pHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress, pHeader);

					if( !pExport) 
					{
						AddDll("Dummy Dll", (mint)MemoryInfo.AllocationBase, (mint)MemoryInfo.RegionSize);
						return;
					}

					pName = (const char *)GetVirtualAddressFromRVA(Base, pExport->Name, pHeader);

					if (!pName)
					{
						AddDll("Dummy Dll", (mint)MemoryInfo.AllocationBase, (mint)MemoryInfo.RegionSize);
						return;
					}
				}

				CFStr Namnet;
				Namnet.Capture(pName);
				Namnet = Namnet.GetFilename();

				AddDll(Namnet, Base, pHeader->OptionalHeader.SizeOfImage);
			}
		}

		int m_Recursive;
		int m_RecursiveDll;
		void CheckAddress(mint _Address)
		{
			if (!MRTC_GetRD()->m_EnableFlags)
				return;

			{
				bool bSuccess;
				M_MWMR_Lock_Unlock(m_Lock, bSuccess);
				if (!bSuccess)
					return;
				CLoadedDll *pDll = m_LoadedDlls.FindLargestLessThanEqual(_Address);

				// Address already exists
				if (pDll && _Address >= pDll->m_Address && _Address < pDll->m_Address + pDll->m_Size)
					return;
			}

			AddAddress(_Address);
		}
	};

	uint64 m_DebuggerData[(sizeof(CWin32Debugger) + 7) / 8];
	CWin32Debugger *m_pDebugger;
#endif

#endif

#ifdef M_FINDNASTYACCESSESTODELETEDMEMORY
	class CVirtualHeap
	{
	public:

		CVirtualHeap()
		{
			for (int i = 0; i < 524288; ++i)
			{
				m_PageInfos[i].m_Type = 0; // Neither allocated or free
				m_PageInfos[i].m_PagePrev = 0; 
				m_PageInfos[i].m_PageNext = 0;
			}

			m_FirstPage = 0;
			// Allocate 1 GIG of memory for usage
			NeedMore((1024*1024*1024) / 4096);
		}

		class CPageInfo
		{
		public:
			uint32 m_Type:2;
			uint32 m_PagePrev:30;
			uint32 m_PageNext;
			int32 m_NextFreePage;
		};

		CPageInfo m_PageInfos[524288]; // Saves next free block and when allocated saves the size of the block

		class CSizeClass
		{
		public:
			uint32 m_Size;
			int32 m_FirstFreePage;

			CSizeClass()
			{
				m_FirstFreePage = 0;
			}

			class CCompare
			{
			public:

				DIdsPInlineS static aint Compare(const CSizeClass *_pFirst, const CSizeClass *_pSecond, void *_pContext)
				{
					return _pFirst->m_Size - _pSecond->m_Size;
				}

				DIdsPInlineS static aint Compare(const CSizeClass *_pTest, uint32 _Key, void *_pContext)
				{
					return _pTest->m_Size - _Key;
				}

			};

			DIdsTreeAVLAligned_Link(CSizeClass, m_Link, uint32, CCompare);

		};

		TCPool<CSizeClass, 128, NThread::CLock, CPoolType_Freeable, CAllocator_Virtual> m_SizeClassPool;

		DIdsTreeAVLAligned_Tree(CSizeClass, m_Link, uint32, CSizeClass::CCompare) m_SizeClasses;

		CSizeClass *GetSizeClass(uint32 _Size)
		{
			CSizeClass *pSizeClass = m_SizeClasses.FindEqual(_Size);
			if (!pSizeClass)
			{
				pSizeClass = m_SizeClassPool.New();
				pSizeClass->m_Size = _Size;
				m_SizeClasses.Insert(pSizeClass);
			}

			return pSizeClass;
		}

		void TraceSizeClasses()
		{
			DIdsTreeAVLAligned_Iterator(CSizeClass, m_Link, uint32, CSizeClass::CCompare) Iter = m_SizeClasses;

			M_TRACE("-------------------------------------------------\n");
			while (Iter)
			{
				M_TRACE("%d \n", Iter->m_Size);

				++Iter;
			}
		}

		int m_FirstPage;

		MRTC_CriticalSection m_Lock;
		void NeedMore(int _Size)
		{
			if (_Size < 32768)
				_Size = 32768; // Allocate at least 128 MiB of memory

			void *pMemory = VirtualAlloc(NULL, _Size * 4096, MEM_RESERVE, PAGE_READWRITE);
			int32 Page = ((uint32)pMemory) / 4096;

			if (!m_FirstPage)
				m_FirstPage = Page;

			CSizeClass *pSizeClass = GetSizeClass(_Size);
			m_PageInfos[Page].m_Type = 1; // Free
			m_PageInfos[Page].m_PagePrev = Page - 1; // No prev page
			m_PageInfos[Page].m_PageNext = Page + _Size; // No next page
			m_PageInfos[Page].m_NextFreePage = pSizeClass->m_FirstFreePage;
			m_PageInfos[m_PageInfos[Page].m_PageNext].m_PagePrev = Page;
			pSizeClass->m_FirstFreePage = Page;
		}

		void *Alloc(uint32 _Size)
		{
			M_LOCK(m_Lock);
			if (_Size < 1)
				_Size = 1;
			_Size = (_Size + 4095) / 4096; // Number of pages			

			CSizeClass *pSizeClass = m_SizeClasses.FindSmallestGreaterThanEqual(_Size);

			if (!pSizeClass)
			{
				NeedMore(_Size);
				pSizeClass = m_SizeClasses.FindSmallestGreaterThanEqual(_Size);

				if (!pSizeClass)
				{
					OS_TraceRaw("Out of memory\n");
					M_BREAKPOINT; // Out of memory
				}
			}

			int Size = pSizeClass->m_Size;
			int OurPage = pSizeClass->m_FirstFreePage;


			if (0)
			{
				MEMORY_BASIC_INFORMATION MemoryInfo;
				int Size = sizeof(MemoryInfo);
				memset(&MemoryInfo, 0, Size);
				int nBytes = VirtualQuery((void *)(OurPage*4096), &MemoryInfo, Size);
				if (nBytes != sizeof(MemoryInfo))
				{
				}
				else
				{	
					M_ASSERT(MemoryInfo.State == MEM_RESERVE, "Memory is already allocated");
				}
			}


			pSizeClass->m_FirstFreePage = m_PageInfos[OurPage].m_NextFreePage;
			if (!pSizeClass->m_FirstFreePage)
			{
				// Empty size class
				m_SizeClasses.Remove(pSizeClass);
				m_SizeClassPool.Delete(pSizeClass);
			}

			int FreePage = OurPage + _Size;
			int NextPage = m_PageInfos[OurPage].m_PageNext;
			m_PageInfos[OurPage].m_PageNext = FreePage;
			m_PageInfos[OurPage].m_Type = 2; // Allocated block

			Size -= _Size;

			if (Size > 0)
			{
				// Lets return the free page to the pool
				
				CSizeClass *pSizeClass = GetSizeClass(Size);
				m_PageInfos[FreePage].m_PagePrev = OurPage;
				m_PageInfos[FreePage].m_PageNext = NextPage;
				m_PageInfos[FreePage].m_Type = 1;
				m_PageInfos[FreePage].m_NextFreePage = pSizeClass->m_FirstFreePage;
				m_PageInfos[NextPage].m_PagePrev = FreePage;
				pSizeClass->m_FirstFreePage = FreePage;
			}

//			CheckPages();

			void *pAddr = (void *)(OurPage * 4096);
			VirtualAlloc(pAddr, _Size * 4096, MEM_COMMIT, PAGE_READWRITE);
			//TraceSizeClasses();

			return pAddr;
		}

		void RemoveSizeClass(int _Page)
		{
			uint32 Size = m_PageInfos[_Page].m_PageNext - _Page;
			CSizeClass *pSizeClass = m_SizeClasses.FindEqual(Size);
			int iPrev = 0;
			int iCurrent = pSizeClass->m_FirstFreePage;
			while (iCurrent)
			{
				int iNext = m_PageInfos[iCurrent].m_NextFreePage;
				if (iCurrent == _Page)
				{
					if (iPrev)
					{
						m_PageInfos[iPrev].m_NextFreePage = iNext;
					}
					else
					{
						pSizeClass->m_FirstFreePage = iNext;
					}
				}
				iPrev = iCurrent;
				iCurrent = iNext;
			}

			if (!pSizeClass->m_FirstFreePage)
			{
				m_SizeClasses.Remove(pSizeClass);
				m_SizeClassPool.Delete(pSizeClass);
			}
		}

		void CheckPages()
		{
			int iCurrentPage = m_FirstPage;
			int iLastPage = m_FirstPage-1;
			int LastType = 0;
			while (iCurrentPage)
			{
				if (iLastPage != m_PageInfos[iCurrentPage].m_PagePrev)
				{
					M_BREAKPOINT; // Link error
				}
				if (LastType == 1 && m_PageInfos[iCurrentPage].m_Type == 1)
				{
					M_BREAKPOINT; // These pages should be together
				}
				iLastPage = iCurrentPage;
				LastType = m_PageInfos[iCurrentPage].m_Type;
				iCurrentPage = m_PageInfos[iCurrentPage].m_PageNext;
				if (iCurrentPage && iCurrentPage <= iLastPage)
					M_BREAKPOINT; // Error
			}
		}

		void Free(void *_pMem)
		{
			M_LOCK(m_Lock);
			int Page = ((uint32)_pMem) / 4096;
			if (((uint32)_pMem - (Page * 4096)) != 0)
			{
				// An invalid address was sent to free
				M_BREAKPOINT;
			}

			if (m_PageInfos[Page].m_Type != 2)
			{
				// Error this page isn't allocated
				M_BREAKPOINT;
			}

			m_PageInfos[Page].m_Type = -1;

			int First = Page;
			int Size = m_PageInfos[Page].m_PageNext - Page;
			int PrevPage = m_PageInfos[Page].m_PagePrev;
			int NextPage = m_PageInfos[Page].m_PageNext;

			void *pAddr = _pMem;

			VirtualFree(pAddr, Size * 4096, MEM_DECOMMIT);

			if (PrevPage && m_PageInfos[PrevPage].m_Type == 1)
			{
				int USize = m_PageInfos[PrevPage].m_PageNext - PrevPage;
				RemoveSizeClass(PrevPage);
				if (0)
				{
					MEMORY_BASIC_INFORMATION MemoryInfo;
					int Size = sizeof(MemoryInfo);
					memset(&MemoryInfo, 0, Size);
					int nBytes = VirtualQuery((void *)(PrevPage*4096), &MemoryInfo, Size);
					if (nBytes != sizeof(MemoryInfo))
					{
					}
					else
					{	
						M_ASSERT(MemoryInfo.State == MEM_RESERVE, "Memory is already allocated");
					}
				}
				Size += USize;
				First = PrevPage;
			}

			if (NextPage && m_PageInfos[NextPage].m_Type == 1)
			{
				int USize = m_PageInfos[NextPage].m_PageNext - NextPage;
				RemoveSizeClass(NextPage);
				if (0)
				{
					MEMORY_BASIC_INFORMATION MemoryInfo;
					int Size = sizeof(MemoryInfo);
					memset(&MemoryInfo, 0, Size);
					int nBytes = VirtualQuery((void *)(NextPage*4096), &MemoryInfo, Size);
					if (nBytes != sizeof(MemoryInfo))
					{
					}
					else
					{	
						M_ASSERT(MemoryInfo.State == MEM_RESERVE, "Memory is already allocated");
					}
				}
				Size += USize;
				m_PageInfos[NextPage].m_Type = -1;
			}

			m_PageInfos[First].m_PageNext = First + Size;
			m_PageInfos[First].m_Type = 1;

			if (m_PageInfos[First].m_PageNext)
				m_PageInfos[m_PageInfos[First].m_PageNext].m_PagePrev = First;
			
			CSizeClass *pSizeClass = GetSizeClass(Size);
			
			m_PageInfos[First].m_NextFreePage = pSizeClass->m_FirstFreePage;
			pSizeClass->m_FirstFreePage = First;

//			CheckPages();
//			TraceSizeClasses();
		}

		uint32 MemSize(const void *_pMem)
		{
			M_LOCK(m_Lock);
			int Page = ((uint32)_pMem) / 4096;
			if (m_PageInfos[Page].m_Type != 2)
			{
				// Error this page isn't allocated
				M_BREAKPOINT;
			}

			return (m_PageInfos[Page].m_PageNext - Page) * 4096;
		}
	};

	CVirtualHeap m_VirtualHeap;
#endif

#ifdef ENABLE_STACKTRACING
	LPTOP_LEVEL_EXCEPTION_FILTER m_pThisExceptionFilter;
	static bool ms_bHandleException;
	static bool ms_bAutoCoredump;
	static LONG WINAPI UnhandledException(struct _EXCEPTION_POINTERS *_pExceptionInfo);
#endif


	class CTCPContext
#ifdef PLATFORM_WIN_PC
		: public MRTC_Thread
#endif
	{
	public:
		void *m_hThread;
		uint32 m_ThreadID;
#ifdef PLATFORM_WIN_PC
		NThread::CEvent m_ThreadStartEvent;
#endif
		bint m_bInitFailed;

		CTCPContext()
		{
			m_bInitFailed = false;
			m_hThread = DNP;
#ifdef PLATFORM_WIN_PC
			m_ThreadStartEvent.ResetSignaled();
			Thread_Create(NULL, 64*1024, MRTC_THREAD_PRIO_HIGHEST);
			m_ThreadStartEvent.Wait();
#else
			Init();
#endif
		}

		~CTCPContext()
		{
#ifdef PLATFORM_WIN_PC
			Thread_RequestTermination();
			PostThreadMessage(m_ThreadID, WM_QUIT, 0, 0);
			Thread_Destroy();
#endif
			m_SocketTree.f_DeleteAll();
		}

#ifndef PLATFORM_WIN_PC
		void Init()
		{
#ifdef PLATFORM_XBOX
#ifdef M_Profile // disable this if you want to run
			XNetStartupParams xnsp;
			memset( &xnsp, 0, sizeof( xnsp ) );
			xnsp.cfgSizeOfStruct = sizeof( XNetStartupParams );
			xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
			INT erro = XNetStartup( &xnsp );
#else
			INT erro = XNetStartup( NULL );
#endif

			XNADDR AddrXn;
			while (XNetGetTitleXnAddr(&AddrXn) == XNET_GET_XNADDR_PENDING)
			{
				Sleep(10);
			}

			M_TRACEALWAYS("Title IPAddr: %d.%d.%d.%d\n", 
				AddrXn.ina.S_un.S_un_b.s_b1, 
				AddrXn.ina.S_un.S_un_b.s_b2, 
				AddrXn.ina.S_un.S_un_b.s_b3, 
				AddrXn.ina.S_un.S_un_b.s_b4);
#endif
			m_hThread = GetCurrentThread();
			m_ThreadID = GetCurrentThreadId();

			{
				WORD wVersionRequested;
				WSADATA wsaData;
				aint err;
				
				wVersionRequested = MAKEWORD( 2, 2 );
				
				err = WSAStartup( wVersionRequested, &wsaData );

				if ( err != 0 ) 
				{
					m_bInitFailed = true;
					return;
				}
				
				if (LOBYTE( wsaData.wVersion ) != 2 ||
					HIBYTE( wsaData.wVersion ) != 2 ) 
				{
					WSACleanup( );
					m_bInitFailed = true;
					return; 
				}
			}
		}
#endif

		class CTCPSocket
#ifndef PLATFORM_WIN_PC
			: public MRTC_Thread
#endif
		{
		public:
			MRTC_CriticalSection m_Lock;

			CTCPSocket()
			{
				m_State = 0;
				m_pReportTo = DNP;
				m_pSocket = DNP;
#ifndef PLATFORM_WIN_PC
				m_bListenSocket = false;
				m_bConnectionSocket = false;
				m_SocketEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
				Thread_Create(NULL, 16384, MRTC_THREAD_PRIO_HIGHEST);
#endif
			}

			~CTCPSocket()
			{
#ifndef PLATFORM_WIN_PC
				Thread_Destroy();
				CloseHandle(m_SocketEvent);
#endif

				if (m_pSocket && !(m_State & M_Bit(31)))
					closesocket((SOCKET)m_pSocket);
			}

#ifndef PLATFORM_WIN_PC
			virtual int Thread_Main()
			{
				MRTC_SystemInfo::Thread_SetName("MRTC Socket reporter");

				while (!Thread_IsTerminating())
				{
					if (m_pSocket)
					{
						TIMEVAL TimeOut;
						TimeOut.tv_sec = 0;
						TimeOut.tv_usec = 0;
						fd_set Read;
						Read.fd_count = 1;
						Read.fd_array[0] = (SOCKET)m_pSocket;
						fd_set Write;
						Write.fd_count = 1;
						Write.fd_array[0] = (SOCKET)m_pSocket;
						select(0, &Read, &Write, NULL, &TimeOut);

						{
							DLockTyped(NThread::CMutual, m_Lock);

							uint32 LastState = m_State;

							if (Read.fd_count)
							{
								// Connection, Data, Closed
								// Determine if closed
								sockaddr_in AddrIn; 
								int Len = sizeof(AddrIn);
								if (m_bConnectionSocket && getpeername((SOCKET)m_pSocket, (sockaddr *)&AddrIn, &Len) == WSAENOTCONN)
									m_State |= NNet::ENetTCPState_Closed;

								// Always send read and connection because there is no way to determine which it is
								if (m_bListenSocket)
									m_State |= NNet::ENetTCPState_Connection;
								else
									m_State |= NNet::ENetTCPState_Read; 
							}

							if (Write.fd_count)
							{
								m_State |= NNet::ENetTCPState_Write;
							}
							if (m_State != LastState)
							{
								if (m_pReportTo)
									m_pReportTo->Signal();
							}
						}

						WaitForSingleObject(m_SocketEvent, 100);

					}
					else
						Sleep(50);
				}
				return 0;
			}
#endif

			class CAVLCompare_CTCPSocket
			{
			public:

				static aint Compare(const CTCPSocket *_pFirst, const CTCPSocket *_pSecond, void *_pContext)
				{
					return (mint)(_pFirst->m_pSocket) - (mint)(_pSecond->m_pSocket);
				}

				static aint Compare(const CTCPSocket *_pTest, const void *_Key, void *_pContext)
				{
					return (mint)_pTest->m_pSocket - (mint)_Key;
				}
			};


			void *m_pSocket;
			DIdsTreeAVLAligned_Link(CTCPSocket, m_TreeLink, void *, CAVLCompare_CTCPSocket);

			NThread::CEventAutoResetReportableAggregate *m_pReportTo;

			uint32 m_State;
#ifndef PLATFORM_WIN_PC
			WSAEVENT m_SocketEvent;
			uint32 m_bListenSocket:1;
			uint32 m_bConnectionSocket:1;
#endif
		};

		DIdsTreeAVLAligned_Tree(CTCPSocket, m_TreeLink, void *, CTCPSocket::CAVLCompare_CTCPSocket) m_SocketTree;
		typedef DIdsTreeAVLAligned_Iterator(CTCPSocket, m_TreeLink, void *, CTCPSocket::CAVLCompare_CTCPSocket) CSocketIter;
		NThread::CMutual m_Lock;

		void f_CheckFailed()
		{
			if (m_bInitFailed)
				Error_static(__FUNCTION__, "Initziation of WinSock has faild, cannot use net");
		}

		bint f_ResolveAddres(const ch8 *_pAddress, NNet::CNetAddressIPv4 &_Address)
		{
#ifdef PLATFORM_XBOX
			f_CheckFailed();


			int iIP = 0;
			const ch8 *pParse = _pAddress;
			while (*pParse)
			{
				int iAddress = NStr::StrToIntParse(pParse, -1, ".");
				if (iAddress < 0)
					break;

				_Address.m_IP[iIP] = iAddress;

				if (++iIP == 4)
				{
					if (*pParse == 0)
						return true;
					else 
						break;
				}
				if (*pParse == '.')
					++pParse;
				else
					break;
			}

			XNDNS * pxndns = NULL;
			INT err = XNetDnsLookup(_pAddress, NULL, &pxndns);
			while (pxndns->iStatus == WSAEINPROGRESS)
			{
				Sleep(1);
			}

			if (pxndns->iStatus != 0 || pxndns->cina < 1)
			{
				XNetDnsRelease(pxndns);
				
				// An error occurred.  One of the following:
				// 	pxndns->iStatus == WSAHOST_NOT_FOUND - No such host
				//    pxndns->iStatus == WSAETIMEDOUT - No response from DNS server(s)
				return false;
			}
			// Don't dereference pxndns after this point

			_Address.m_IP[0] = pxndns->aina[0].S_un.S_un_b.s_b1;
			_Address.m_IP[1] = pxndns->aina[0].S_un.S_un_b.s_b2;
			_Address.m_IP[2] = pxndns->aina[0].S_un.S_un_b.s_b3;
			_Address.m_IP[3] = pxndns->aina[0].S_un.S_un_b.s_b4;

			XNetDnsRelease(pxndns);

			return true;
#else
			f_CheckFailed();

			hostent *pHost = gethostbyname(_pAddress);

			if (!pHost)
			{
				return false;
			}

			_Address.m_IP[0] = (*pHost->h_addr_list)[0];
			_Address.m_IP[1] = (*pHost->h_addr_list)[1];
			_Address.m_IP[2] = (*pHost->h_addr_list)[2];
			_Address.m_IP[3] = (*pHost->h_addr_list)[3];

			return true;
#endif
		}

		CTCPSocket *f_Connect(const NNet::CNetAddressTCPv4 &_Address, NThread::CEventAutoResetReportableAggregate *_pReportTo)
		{
			f_CheckFailed();

			SOCKET Sock = socket(AF_INET, SOCK_STREAM, 0);
			if (Sock == INVALID_SOCKET)
				return NULL;

			sockaddr_in Address;
			Address.sin_family = AF_INET;
/*			Address.sin_addr.S_un.S_addr = INADDR_ANY;
			Address.sin_port = 0; // Any port

			if (bind(Sock, (sockaddr *)&Address, sizeof(Address)))
			{
				closesocket(Sock);
				Error_static(__FUNCTION__,CFStr256::FormatStr("Could not bind socket, windows returned: {}", DIdsStrFTStr(CFStr256(), GetLastErrorStr(WSAGetLastError()))));
			}*/

			Address.sin_addr.S_un.S_un_b.s_b1 = _Address.m_IP[0];
			Address.sin_addr.S_un.S_un_b.s_b2 = _Address.m_IP[1];
			Address.sin_addr.S_un.S_un_b.s_b3 = _Address.m_IP[2];
			Address.sin_addr.S_un.S_un_b.s_b4 = _Address.m_IP[3];
			Address.sin_port = htons(_Address.m_Port);

			if (connect(Sock, (sockaddr *)&Address, sizeof(Address)))
			{
				closesocket(Sock);
				return NULL;
			}

			int Buf = 512*1024;
			if (setsockopt(Sock, SOL_SOCKET, SO_RCVBUF, (char *)&Buf, sizeof(Buf)))
			{
				closesocket(Sock);
				return NULL;
			}

			if (setsockopt(Sock, SOL_SOCKET, SO_SNDBUF, (char *)&Buf, sizeof(Buf)))
			{
				closesocket(Sock);
				return NULL;
			}

			BOOL NoDelay = true;

			if (setsockopt(Sock, IPPROTO_TCP, TCP_NODELAY, (char *)&NoDelay, sizeof(NoDelay)))
			{
				closesocket(Sock);
				return NULL;
			}

			CTCPSocket *pReturn = DNew(CTCPSocket) CTCPSocket;

			pReturn->m_pReportTo = _pReportTo;
			pReturn->m_pSocket = (void *)Sock;
			{
				DLockTyped(NThread::CMutual, m_Lock);
				m_SocketTree.f_Insert(pReturn);
			}


#ifdef PLATFORM_WIN_PC
			if (WSAAsyncSelect(Sock, m_hReportWnd, WM_USER, FD_READ | FD_WRITE | FD_CLOSE))
			{
				delete pReturn;
				return NULL;
			}
#else
			pReturn->m_bConnectionSocket = true;
			if (WSAEventSelect(Sock, pReturn->m_SocketEvent, FD_READ | FD_WRITE | FD_CLOSE))
			{
				delete pReturn;
				return NULL;
			}
#endif

			return pReturn;
		}

		CTCPSocket *f_Bind(const NNet::CNetAddressUDPv4 &_Address, NThread::CEventAutoResetReportableAggregate *_pReportTo)
		{
			f_CheckFailed();

			SOCKET Sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if (Sock == INVALID_SOCKET)
			{
				HRESULT Resse = WSAGetLastError();
				return NULL;
			}

			sockaddr_in Address;
			ZeroMemory(&Address, sizeof(Address));
			Address.sin_family = AF_INET;
			Address.sin_addr.S_un.S_un_b.s_b1 = _Address.m_IP[0];
			Address.sin_addr.S_un.S_un_b.s_b2 = _Address.m_IP[1];
			Address.sin_addr.S_un.S_un_b.s_b3 = _Address.m_IP[2];
			Address.sin_addr.S_un.S_un_b.s_b4 = _Address.m_IP[3];
			Address.sin_port = htons(_Address.m_Port);

			if (bind(Sock, (sockaddr *)&Address, sizeof(Address)))
			{
				closesocket(Sock);
				return NULL;
			}

			int Buf = 4*1024;
			if (setsockopt(Sock, SOL_SOCKET, SO_RCVBUF, (char *)&Buf, sizeof(Buf)))
			{
				closesocket(Sock);
				return NULL;
			}

			if (setsockopt(Sock, SOL_SOCKET, SO_SNDBUF, (char *)&Buf, sizeof(Buf)))
			{
				closesocket(Sock);
				return NULL;
			}

			if (_Address.m_bBroadcast)
			{
				BOOL Broadcast = 1;
				if (setsockopt(Sock, SOL_SOCKET, SO_BROADCAST, (char*) &Broadcast, sizeof(Broadcast)))
				{
					closesocket(Sock);
					return NULL;
				}
			}


			CTCPSocket *pReturn = DNew(CTCPSocket) CTCPSocket;

			pReturn->m_pReportTo = _pReportTo;
			pReturn->m_pSocket = (void *)Sock;
			{
				DLockTyped(NThread::CMutual, m_Lock);
				m_SocketTree.f_Insert(pReturn);
			}

#ifdef PLATFORM_WIN_PC
			if (WSAAsyncSelect(Sock, m_hReportWnd, WM_USER, FD_READ | FD_WRITE | FD_CLOSE))
			{
				delete pReturn;
				return NULL;
			}
#else
			if (WSAEventSelect(Sock, pReturn->m_SocketEvent, FD_READ | FD_WRITE | FD_CLOSE))
			{
				delete pReturn;
				return NULL;
			}
#endif

			return pReturn;
		}

		CTCPSocket *f_Listen(const NNet::CNetAddressTCPv4 &_Address, NThread::CEventAutoResetReportableAggregate *_pReportTo)
		{
			f_CheckFailed();

			SOCKET Sock = socket(AF_INET, SOCK_STREAM, 0);
			if (Sock == INVALID_SOCKET)
				Error_static(__FUNCTION__,"Could not create socket");

			sockaddr_in Address;
			Address.sin_family = AF_INET;
			Address.sin_addr.S_un.S_un_b.s_b1 = _Address.m_IP[0];
			Address.sin_addr.S_un.S_un_b.s_b2 = _Address.m_IP[1];
			Address.sin_addr.S_un.S_un_b.s_b3 = _Address.m_IP[2];
			Address.sin_addr.S_un.S_un_b.s_b4 = _Address.m_IP[3];
			Address.sin_port = htons(_Address.m_Port);

			if (bind(Sock, (sockaddr *)&Address, sizeof(Address)))
			{
				closesocket(Sock);
				return NULL;
			}

			if (listen(Sock, 32))
			{
				closesocket(Sock);
				return NULL;
			}
	
			CTCPSocket *pReturn = DNew(CTCPSocket) CTCPSocket;

			pReturn->m_pReportTo = _pReportTo;
			pReturn->m_pSocket = (void *)Sock;
			{
				DLockTyped(NThread::CMutual, m_Lock);
				m_SocketTree.f_Insert(pReturn);
			}

#ifdef PLATFORM_WIN_PC
			if (WSAAsyncSelect(Sock, m_hReportWnd, WM_USER, FD_ACCEPT | FD_CLOSE))
			{
				delete pReturn;
				return NULL;
			}
#else
			pReturn->m_bListenSocket = true;
			if (WSAEventSelect(Sock, pReturn->m_SocketEvent, FD_ACCEPT | FD_CLOSE))
			{
				delete pReturn;
				return NULL;
			}
#endif

			return pReturn;
		}

		CTCPSocket *f_Accept(CTCPSocket *_pSocket, NThread::CEventAutoResetReportableAggregate *_pReportTo)
		{			
			f_CheckFailed();

			SOCKET Sock = accept((SOCKET)_pSocket->m_pSocket, DNP, 0);
			if (Sock == INVALID_SOCKET)
			{
				int LastError = WSAGetLastError();
				if (LastError == WSAEWOULDBLOCK)
					return DNP;

				return NULL;
			}

			int Buf = 512*1024;
			if (setsockopt(Sock, SOL_SOCKET, SO_RCVBUF, (char *)&Buf, sizeof(Buf)))
			{
				closesocket(Sock);
				return NULL;
			}

			if (setsockopt(Sock, SOL_SOCKET, SO_SNDBUF, (char *)&Buf, sizeof(Buf)))
			{
				closesocket(Sock);
				return NULL;
			}

			BOOL NoDelay = true;

			if (setsockopt(Sock, IPPROTO_TCP, TCP_NODELAY, (char *)&NoDelay, sizeof(NoDelay)))
			{
				closesocket(Sock);
				return NULL;
			}

			CTCPSocket *pReturn = DNew(CTCPSocket) CTCPSocket;

			pReturn->m_pReportTo = _pReportTo;
			pReturn->m_pSocket = (void *)Sock;
			{
				DLockTyped(NThread::CMutual, m_Lock);
				m_SocketTree.f_Insert(pReturn);
			}

#ifdef PLATFORM_WIN_PC
			if (WSAAsyncSelect(Sock, m_hReportWnd, WM_USER, FD_READ | FD_WRITE | FD_CLOSE))
			{
				delete pReturn;
				return NULL;
			}
#else
			pReturn->m_bConnectionSocket = true;
			if (WSAEventSelect(Sock, pReturn->m_SocketEvent, FD_READ | FD_WRITE | FD_CLOSE))
			{
				delete pReturn;
				return NULL;
			}
#endif

			return pReturn;
		}

		void f_SetReportTo(CTCPSocket *_pSocket, NThread::CEventAutoResetReportableAggregate *_pReportTo)
		{
			{
				DLockTyped(NThread::CMutual, _pSocket->m_Lock);
				_pSocket->m_pReportTo = _pReportTo;

				// Signal once so the new report to gets to update
				if (_pSocket->m_pReportTo)
					_pSocket->m_pReportTo->Signal();
			}
		}

		void *f_InheritHandle(CTCPSocket *_pSocket, NThread::CEventAutoResetReportableAggregate *_pReportTo)
		{
			CTCPSocket *pReturn = DNew(CTCPSocket) CTCPSocket;

			pReturn->m_pReportTo = _pReportTo;
			pReturn->m_pSocket = (void *)_pSocket->m_pSocket;
			pReturn->m_State = _pSocket->m_State;

#ifdef PLATFORM_WIN_PC
			if (WSAAsyncSelect((SOCKET)pReturn->m_pSocket, m_hReportWnd, WM_USER, FD_READ | FD_WRITE | FD_CLOSE))
			{
				delete pReturn;
				return NULL;
			}
#else
			pReturn->m_bListenSocket = _pSocket->m_bListenSocket;
			pReturn->m_bConnectionSocket = _pSocket->m_bConnectionSocket;
			if (WSAEventSelect((SOCKET)pReturn->m_pSocket, pReturn->m_SocketEvent, FD_READ | FD_WRITE | FD_CLOSE))
			{
				delete pReturn;
				return NULL;
			}
#endif

			{
				DLockTyped(NThread::CMutual, _pSocket->m_Lock);
				_pSocket->m_State |= M_Bit(31);
			}

			{
				DLockTyped(NThread::CMutual, m_Lock);
				m_SocketTree.f_Insert(pReturn);
			}
			if (pReturn->m_pReportTo)
				pReturn->m_pReportTo->Signal();

			return pReturn;
		}


		bint f_Close(CTCPSocket *_pSocket)
		{
			{
				DLockTyped(NThread::CMutual, m_Lock);
				m_SocketTree.f_Remove(_pSocket);
			}

			// Make sure that the report thread isn't using our socket
			{
				DLockTyped(NThread::CMutual, _pSocket->m_Lock);
			}

			delete _pSocket;

			return m_SocketTree.IsEmpty();
		}

		uint32 f_GetState(CTCPSocket *_pSocket)
		{
			DLockTyped(NThread::CMutual, _pSocket->m_Lock);
			uint32 State = _pSocket->m_State & DBitRange(0, 30);
			_pSocket->m_State &= ~DBitRange(0, 30);
			return State;
		}

		int f_Receive(CTCPSocket *_pSocket, void *_pData, int _DataLen)
		{
			int Ret = recv((SOCKET)_pSocket->m_pSocket, (char *)_pData, _DataLen, 0);

			if (Ret == SOCKET_ERROR)
			{
				int Error = WSAGetLastError();
				if (Error != WSAEWOULDBLOCK)
				{
					return 0;
					//Error_static(__FUNCTION__,"Could not revc from socket");
				}
				else
					return 0;
			}

			return Ret;
		}

		int f_Send(CTCPSocket *_pSocket, const void *_pData, int _DataLen)
		{
			int Ret = send((SOCKET)_pSocket->m_pSocket, (const char *)_pData, _DataLen, 0);

			if (Ret == SOCKET_ERROR)
			{
				int Error = WSAGetLastError();
				if (Error != WSAEWOULDBLOCK)
				{
					return 0;
					//Error_static(__FUNCTION__,"Could not sendfrom socket");
				}
				else
					return 0;
			}

			return Ret;
		}


		int f_Receive(CTCPSocket *_pSocket, NNet::CNetAddressUDPv4 &_Address, void *_pData, int _DataLen)
		{
			sockaddr_in Address;
			Address.sin_family = AF_INET;

			int Size = sizeof(Address);
			int Ret = recvfrom((SOCKET)_pSocket->m_pSocket, (char *)_pData, _DataLen, 0, (sockaddr *)&Address, &Size);

			if (Ret == SOCKET_ERROR)
			{
				int Error = WSAGetLastError();
				if (Error != WSAEWOULDBLOCK)
				{
					return 0;
//					Error_static(__FUNCTION__,"Could not revc from socket");
				}
				else
					return 0;
			}

			_Address.m_IP[0] = Address.sin_addr.S_un.S_un_b.s_b1;
			_Address.m_IP[1] = Address.sin_addr.S_un.S_un_b.s_b2;
			_Address.m_IP[2] = Address.sin_addr.S_un.S_un_b.s_b3;
			_Address.m_IP[3] = Address.sin_addr.S_un.S_un_b.s_b4;
			_Address.m_Port = htons(Address.sin_port);

			return Ret;
		}

		int f_Send(CTCPSocket *_pSocket, const NNet::CNetAddressUDPv4 &_Address, const void *_pData, int _DataLen)
		{
			sockaddr_in Address;
			Address.sin_family = AF_INET;
			Address.sin_addr.S_un.S_un_b.s_b1 = _Address.m_IP[0];
			Address.sin_addr.S_un.S_un_b.s_b2 = _Address.m_IP[1];
			Address.sin_addr.S_un.S_un_b.s_b3 = _Address.m_IP[2];
			Address.sin_addr.S_un.S_un_b.s_b4 = _Address.m_IP[3];
			Address.sin_port = htons(_Address.m_Port);

			int Ret = sendto((SOCKET)_pSocket->m_pSocket, (const char *)_pData, _DataLen, 0, (sockaddr *)&Address, sizeof(Address));

			if (Ret == SOCKET_ERROR)
			{
				int Error = WSAGetLastError();
				if (Error != WSAEWOULDBLOCK)
				{
					return 0;
//					Error_static(__FUNCTION__,"Could not sendfrom socket");
				}
				else
					return 0;
			}

			return Ret;
		}



#ifdef PLATFORM_WIN_PC
		HWND m_hReportWnd;

		static LRESULT WINAPI SocketWindowProc(HWND _hWnd, UINT _Message, WPARAM _wParam, LPARAM _lParam)
		{
			return DefWindowProc(_hWnd, _Message, _wParam, _lParam);
		}
#endif

#ifdef PLATFORM_WIN_PC
		virtual int Thread_Main()
		{
			MRTC_SystemInfo::Thread_SetName("MRTC Sockets");
			m_hThread = GetCurrentThread();
			m_ThreadID = GetCurrentThreadId();

			{
				WORD wVersionRequested;
				WSADATA wsaData;
				aint err;
				
				wVersionRequested = MAKEWORD( 2, 2 );
				
				err = WSAStartup( wVersionRequested, &wsaData );

				if ( err != 0 ) 
				{
					m_bInitFailed = true;
					m_ThreadStartEvent.SetSignaled();
					return 0;
				}
				
				if (LOBYTE( wsaData.wVersion ) != 2 ||
					HIBYTE( wsaData.wVersion ) != 2 ) 
				{
					WSACleanup( );
					m_bInitFailed = true;
					m_ThreadStartEvent.SetSignaled();
					return 0; 
				}
			}

			mint ProcessId = (mint)GetCurrentProcessId();

			CFStr FormatFormat;
			FormatFormat = CFStrF("MOSPID_%08x_THIS_%08x", ProcessId, (mint)this);

			CFStr ClassName = CFStrF("IdsSocketReportClass_%s", FormatFormat.Str());

			WNDCLASSA WndClass;
			memset(&WndClass, 0, sizeof(WndClass));
			WndClass.lpszClassName = ClassName ;
			WndClass.lpfnWndProc = SocketWindowProc;
			WndClass.hInstance = NULL;
			if (!RegisterClassA(&WndClass))
			{
				m_bInitFailed = true;
				m_ThreadStartEvent.SetSignaled();
				return 0;
			}

			m_hReportWnd = CreateWindowA(ClassName, ClassName, 0, 0, 0, 0, 0, 0, 0, 0, 0);

			if (!m_hReportWnd)
			{
				UnregisterClassA(ClassName, NULL);
				m_bInitFailed = true;
				m_ThreadStartEvent.SetSignaled();
				return 0;
			}

			m_ThreadStartEvent.SetSignaled();

			while (!Thread_IsTerminating())
			{
				int32 Ret;

				MSG Message;

				Ret = GetMessage( &Message, NULL, 0, 0 );
				if (Ret == -1 || Ret == 0 || Message.message == WM_QUIT)
				{
					// handle the error and possibly exit
					goto ExitThread;
				}
				else if (Message.message == WM_USER)
				{
					void *hSocket = (void *)Message.wParam;

					CTCPSocket *pSocket;
					{
						DLockTyped(NThread::CMutual, m_Lock);
						pSocket = m_SocketTree.FindEqual(hSocket);

						if (pSocket)
						{
							DLockTyped(NThread::CMutual, pSocket->m_Lock);
							{

								DUnlockTyped(NThread::CMutual, m_Lock);
//									mint Error = WSAGETSELECTERROR(Message.lParam);
								mint Event = WSAGETSELECTEVENT(Message.lParam);

								{
									if (Event & FD_READ)
										pSocket->m_State |= NNet::ENetTCPState_Read;
									if (Event & FD_WRITE)
										pSocket->m_State |= NNet::ENetTCPState_Write;
									if (Event & FD_ACCEPT)
										pSocket->m_State |= NNet::ENetTCPState_Connection;
									if (Event & FD_CLOSE)
										pSocket->m_State |= NNet::ENetTCPState_Closed;

									if (pSocket->m_pReportTo)
										pSocket->m_pReportTo->Signal();
								}
							}
						}

					}
				}
				else
				{
					TranslateMessage(&Message); 
					DispatchMessage(&Message); 
				}
			}

ExitThread:
			if (m_hReportWnd)
				DestroyWindow(m_hReportWnd);

//			if (g_hDllInstance)
			UnregisterClassA(ClassName, NULL);
			return 0;
		}
#endif
	};

	CTCPContext *m_pTCPContext;

	CTCPContext *GetTCPContext()
	{		
		if (!m_pTCPContext)
		{
			m_pTCPContext = DNew(CTCPContext) CTCPContext();
		}
		return m_pTCPContext;
	}

	void DestroyTCPContext()
	{		
		if (m_pTCPContext)
		{
			delete m_pTCPContext;
			m_pTCPContext = NULL;
		}
	}
	MRTC_SystemInfoInternal()
	{

#ifdef M_STATIC
		MRTC_ObjectManager::m_pSystemInfo->m_pInternalData = this;
#endif

#ifdef ENABLE_STACKTRACING
		m_pThisExceptionFilter = NULL;
		m_pStackTraceContext = NULL;
#endif
#ifdef PLATFORM_WIN_PC
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
		m_pDebugger = NULL;
#endif
#endif

#ifdef OLDREMOTEDEBUGGER
		m_pThread = NULL;
		StartRemoteDebugger();
#endif
	}
	~MRTC_SystemInfoInternal()
	{
#ifdef PLATFORM_WIN_PC
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
		if (m_pDebugger)
			m_pDebugger->~CWin32Debugger();
#endif
#endif
#ifdef OLDREMOTEDEBUGGER
		CloseNet();
#endif
	}
};


void gf_PostCreateSystem()
{
#ifdef OLDREMOTEDEBUGGER
	MRTC_SystemInfo::MRTC_GetSystemInfo().m_pInternalData->ms_OldFilter = SetUnhandledExceptionFilter(MRTC_SystemInfoInternal::FilterExceptions);
#endif
}

#ifdef MRTC_DLL
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| This is a DLL
|__________________________________________________________________________________________________
\*************************************************************************************************/
class MRTC_DLLProcessInfo
{
public:
	MRTC_ObjectManager* m_pObjectManager;		// Pointer to each process' object-manager.
	int m_ProcessID;
	int m_nRef;
	MRTC_DLLProcessInfo* m_pNext;
	MRTC_DLLProcessInfo* m_pPrev;
};

// -------------------------------------------------------------------
//  ..and this is the ONE accessible global:
// -------------------------------------------------------------------
MRTC_DLLProcessInfo* g_pFirstProcess;

// -------------------------------------------------------------------
MRTC_DLLProcessInfo* MRTC_DLLFindProcess()
{
#ifdef PLATFORM_XBOX
	return g_pFirstProcess;
#else
	int ProcessID = GetCurrentProcessId();
	MRTC_DLLProcessInfo* pI = g_pFirstProcess;
	while(pI)
	{
		if (pI->m_ProcessID == ProcessID) return pI;
		pI = pI->m_pNext;
	}
	return NULL;
#endif
}

// -------------------------------------------------------------------
void MRTC_DLLAttachProcess(MRTC_ObjectManager* _pObjMgr)
{
	MRTC_DLLProcessInfo* pInfo = MRTC_DLLFindProcess();
	if (pInfo)
	{
		if (pInfo->m_pObjectManager != _pObjMgr) Error_static("MRTC_DLLAttachProcess", "MRTC: Multiple DLL-attachments from process but with different object-managers.");
		pInfo->m_nRef++;
	}
	else
	{
		MRTC_DLLProcessInfo* pInfo = DNew(MRTC_DLLProcessInfo) MRTC_DLLProcessInfo;
		if (!pInfo) Error_static("MRTC_DLLAttachProcess", "Out of memory.");
#ifdef PLATFORM_XBOX
		pInfo->m_ProcessID = 0;
#else
		pInfo->m_ProcessID = GetCurrentProcessId();
#endif
		pInfo->m_nRef = 1;
		pInfo->m_pObjectManager = _pObjMgr;
		pInfo->m_pPrev = NULL;
		pInfo->m_pNext = g_pFirstProcess;
		g_pFirstProcess = pInfo;
	}
}

void MRTC_DLLDetachProcess(MRTC_ObjectManager* _pObjMgr)
{
	MRTC_DLLProcessInfo* pInfo = MRTC_DLLFindProcess();
	if (!pInfo) return;

	pInfo->m_nRef--;
	if (pInfo->m_nRef <= 0)
	{
		if (pInfo->m_pNext)
			pInfo->m_pNext->m_pPrev = pInfo->m_pPrev;
		if (pInfo->m_pPrev)
			pInfo->m_pPrev->m_pNext = pInfo->m_pNext;
		else 
			g_pFirstProcess = pInfo->m_pNext;
		
		delete pInfo;
	}
}

// -------------------------------------------------------------------
BOOL WINAPI DllEntryPoint(HINSTANCE _hInstDLL, DWORD _fdwReason, LPVOID _pvReserved)
{
#ifdef PLATFORM_XBOX
//	OutputDebugString(CFStrF("DllEntryPoint (%s, %.8X, %.8X)\n", __FILE__, _hInstDLL, GetCurrentThreadId()));
#else
//	OutputDebugString(CFStrF("DllEntryPoint (%s, %.8X, %.8X, %.8X)\n", __FILE__, _hInstDLL, GetCurrentThreadId(), GetCurrentProcessId()));
#endif

	switch(_fdwReason)
	{
	case DLL_PROCESS_ATTACH :
	#ifdef MRTC_ENABLE_REMOTEDEBUGGER
		MRTC_GetRD()->ModuleFinish();
	#endif
//		OutputDebugString("DLL_PROCESS_ATTACH\n");
		break;
	case DLL_PROCESS_DETACH :
//		OutputDebugString("DLL_PROCESS_DETACH\n");
		break;
	case DLL_THREAD_ATTACH :
//		OutputDebugString("DLL_THREAD_ATTACH\n");
		MRTC_SystemInfo::MRTC_GetSystemInfo().m_pInternalData->m_ThreadTrackingContext.RegisterThread();
		break;
	case DLL_THREAD_DETACH :
//		OutputDebugString("DLL_THREAD_DETACH\n");
		MRTC_SystemInfo::MRTC_GetSystemInfo().m_pInternalData->m_ThreadTrackingContext.UnregisterThread();
		break;
	}
	return TRUE;											
//	return MCC_ClassLibraryInit(_hInstDLL, _fdwReason, _pvReserved);
}

BOOL WINAPI DllMain(HINSTANCE _hInstDLL, DWORD _fdwReason, LPVOID _pvReserved)
{
//	OutputDebugString("DllMain...\n");
    return DllEntryPoint(_hInstDLL, _fdwReason, _pvReserved);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Not a DLL
|__________________________________________________________________________________________________
\*************************************************************************************************/
#endif

#ifdef ENABLE_STACKTRACING

bool MRTC_SystemInfoInternal::ms_bHandleException = true;
bool MRTC_SystemInfoInternal::ms_bAutoCoredump = false;

void gf_ResetUnhandeledExceptionFilter()
{
	if (MRTC_SystemInfo::MRTC_GetSystemInfo().m_pInternalData->m_pThisExceptionFilter)
	{
		LPTOP_LEVEL_EXCEPTION_FILTER pOld = SetUnhandledExceptionFilter(MRTC_SystemInfo::MRTC_GetSystemInfo().m_pInternalData->m_pThisExceptionFilter);
		if (pOld != MRTC_SystemInfo::MRTC_GetSystemInfo().m_pInternalData->m_pThisExceptionFilter)
		{
			M_TRACEALWAYS("Reset unhandled exception filter old = 0x%08x\n", pOld);
		}
	}
}

class CSetUnhandledExceptionFilter
{
public:
	CSetUnhandledExceptionFilter()
	{
		if (!MRTC_SystemInfo::MRTC_GetSystemInfo().m_pInternalData->m_pThisExceptionFilter)
			MRTC_SystemInfo::MRTC_GetSystemInfo().m_pInternalData->m_pThisExceptionFilter = &MRTC_SystemInfoInternal::UnhandledException;
		gf_ResetUnhandeledExceptionFilter();
//		else
//			SetUnhandledExceptionFilter(&MRTC_SystemInfoInternal::UnhandledException);
	}
};

CSetUnhandledExceptionFilter gs_CSetUnhandledExceptionFilter;

#endif



__declspec(noinline) void gf_ModuleAdd()
{
#ifndef PLATFORM_XENON
	OSVERSIONINFO m_OSInfo;	
	m_OSInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&m_OSInfo);

	if (m_OSInfo.dwMajorVersion == 5 && m_OSInfo.dwMinorVersion == 1 && m_OSInfo.dwBuildNumber == 2600)
	{
		if (!m_OSInfo.szCSDVersion || m_OSInfo.szCSDVersion[0] == 0)
		{
			MessageBox(NULL, "This game requires Windows XP Service Pack 1 or later to be installed or to be run.", "SbzEngine", MB_OK|MB_ICONERROR);
			ExitProcess(0);
		}
	}	
#endif
}

#ifdef OLDREMOTEDEBUGGER
void gf_FlushRemoteDebugger()
{
	if( MRTC_GetRD()->m_EnableFlags )
	{
		MRTC_SystemInfo::MRTC_GetSystemInfo().m_pInternalData->UpdateSend();	
		MRTC_SystemInfo::OS_Sleep(2000);
	}
}
#endif

#ifdef PLATFORM_XBOX
void DestroyOsInfo();
void gf_PreDestroySystem()
{
	DestroyOsInfo();
}
#endif


#ifdef OLDREMOTEDEBUGGER
LPTOP_LEVEL_EXCEPTION_FILTER MRTC_SystemInfoInternal::ms_OldFilter = NULL;
#endif

MRTC_SystemInfo::MRTC_SystemInfo()
{
/*
	CRITICAL_SECTION Test;
	InitializeCriticalSection(&Test);
	int Test1 = 0;
	EnterCriticalSection(&Test);
	int Test2 = 0;
	LeaveCriticalSection(&Test);
	int Test3 = 0;*/
#ifdef M_STATIC
	MRTC_ObjectManager::m_pSystemInfo = this;
#endif
	m_nCPU = 1;
	m_CPUFrequencyu = 1;
	m_CPUFrequencyfp = 1;
	m_CPUFrequencyRecp = 1;
	m_CPUFeatures = 0;
	m_CPUFeaturesEnabled = 0xffffffff;
	m_CPUName[0] = 0;
	m_CPUNameWithFeatures[0] = 0;

	static mint SysInfoData[(sizeof(MRTC_SystemInfoInternal) + sizeof(mint) - 1) / sizeof(mint)];

	m_pInternalData = new(SysInfoData) MRTC_SystemInfoInternal;

	m_pThreadContext = NULL;

	CPU_Detect();
}

MRTC_SystemInfo::~MRTC_SystemInfo()
{
	m_pInternalData->~MRTC_SystemInfoInternal();

	if (m_pThreadContext)
		m_pThreadContext->~MRTC_ThreadContext();
}

void MRTC_SystemInfo::PostCreate()
{
	m_pInternalData->PostCreate();
}

#ifndef PLATFORM_XENON

int MRTC_SystemInfo::OS_ThreadSpinCount()
{
	if (MRTC_GetSystemInfo().m_nCPU > 1)
		return 400;
	else
		return 0;
}
#endif

void MRTC_SystemInfo::RD_GetServerName(char *_pName)
{
#ifdef PLATFORM_XBOX
	XNADDR AddrXn;
	XNetGetTitleXnAddr(&AddrXn);
	strncpy(_pName, CFStrF("RDS Xenon %d.%d.%d.%d", AddrXn.ina.S_un.S_un_b.s_b1, AddrXn.ina.S_un.S_un_b.s_b2, AddrXn.ina.S_un.S_un_b.s_b3, AddrXn.ina.S_un.S_un_b.s_b4), 31);
	_pName[31] = 0;
#else
	CFStr CompName;
	DWORD Size = 100;
	GetComputerName(CompName, &Size);
	strncpy(_pName, CFStrF("RDS Win32 %s", CompName.Str()), 31);
	_pName[31] = 0;
#endif

}
#ifdef PLATFORM_WIN_PC
#include <tlhelp32.h>
#endif

static bint IsGoodStackPtr(void *_pData, mint _Size, mint _StackLower, mint _StackUpper)
{
	if ((mint)_pData >= _StackLower && (mint)_pData + _Size <= _StackUpper && (mint)_pData <= _StackUpper)
		return true;
	return false;
}


void MRTC_SystemInfo::OS_SendProfilingSnapshot(uint32 _ID)
{
#ifdef PLATFORM_XENON

	static iTrace = 0;

	CMTime TimeStart;
	TimeStart = CMTime::GetCPU();

	DWORD Threads[256];
	DWORD ThreadNum = 256;
	DmGetThreadList(Threads, &ThreadNum);
	uint32 ThisThreadID = GetCurrentThreadId();

	uint8 Packet[1024];

	for (int i = 0; i < ThreadNum; ++i)
	{
		DWORD ThreadID = Threads[i];
		if (ThreadID != ThisThreadID)
		{
		
			HANDLE ThreadHandle = OpenThread(THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT|THREAD_QUERY_INFORMATION, false, ThreadID);
			if (ThreadHandle)
			{
				BOOL Ret = SuspendThread(ThreadHandle);
				if (Ret >= 0)
				{
					M_ASSERT(Ret >= 0, "");

					CONTEXT ThreadContext;
					ZeroMemory(&ThreadContext, sizeof(ThreadContext));
					ThreadContext.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
					Ret = DmGetThreadContext(ThreadID, &ThreadContext);
					M_ASSERT(Ret, "");

					DM_THREADINFOEX ThreadInfo;
					ZeroMemory(&ThreadInfo, sizeof(ThreadInfo));
					ThreadInfo.Size = sizeof(ThreadInfo);
					
					Ret = DmGetThreadInfoEx(ThreadID, &ThreadInfo);
					M_ASSERT(Ret, "");

					uint8 *pPacket = Packet;
					uint8 *pPacketMax = pPacket+1024-4;

					*((uint32 *)pPacket) = _ID; SwapLE(*((uint32 *)pPacket)); pPacket += 4; // SnapID
					*((uint32 *)pPacket) = ThreadID; SwapLE(*((uint32 *)pPacket)); pPacket += 4; // SnapID
					*((uint32 *)pPacket) = ThreadInfo.ThreadNameLength; SwapLE(*((uint32 *)pPacket)); pPacket += 4; // SnapID
					memcpy(pPacket, ThreadInfo.ThreadNameAddress, ThreadInfo.ThreadNameLength); pPacket += (ThreadInfo.ThreadNameLength + 3) & ~3; // Align on 4 bytes
					*((uint32 *)pPacket) = ThreadContext.Iar; SwapLE(*((uint32 *)pPacket)); pPacket += 4;

					mint StackFrame = ThreadContext.Gpr1;
					
					mint StackStart = (mint)ThreadInfo.StackLimit;
					mint StackEnd =  (mint)ThreadInfo.StackBase;

					// First stackframe is same as Iar and might be NULL
					if (IsGoodStackPtr((void *)StackFrame, sizeof(mint) * 2, StackStart, StackEnd))
						StackFrame = *((mint *)(StackFrame));

					while (pPacket < pPacketMax && StackFrame)
					{
						if (!IsGoodStackPtr((void *)StackFrame, sizeof(mint) * 2, StackStart, StackEnd))
							break;

						mint CodePtr = *((mint *)(StackFrame - 8));
						*((uint32 *)pPacket) = CodePtr; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
						StackFrame = *((mint *)(StackFrame));
					}

					Ret = ResumeThread(ThreadHandle);
					M_ASSERT(Ret >= 0, "");

					MRTC_GetRD()->SendDataRaw(ERemoteDebug_ProfileStackTrace, Packet, pPacket - Packet);
				}

				Ret = CloseHandle(ThreadHandle);
				M_ASSERT(Ret, "");

			}

		}
	
	}

	// Alternate Processor
	if (_ID & 1)
		XSetThreadProcessor(GetCurrentThread(), 5);
	else
		XSetThreadProcessor(GetCurrentThread(), 4);
	
	TimeStart = CMTime::GetCPU() - TimeStart;
	if ((iTrace % 10000) == 0)
		M_TRACEALWAYS("Snapshot time %f ms\n", TimeStart.GetTime()*1000.0);
	++iTrace;
#endif
#ifdef PLATFORM_WIN_PC
#if defined(MRTC_ENABLE_REMOTEDEBUGGER)
#if 1
	MRTC_SystemInfoInternal *pData = MRTC_GetSystemInfo().m_pInternalData;

	pData->m_ThreadTrackingContext.UpdateCopy();

	CMTime TimeStart;
	TimeStart = CMTime::GetCPU();
	uint32 ThisProcessID = GetCurrentProcessId();
	THREADENTRY32 Thread;
	memset(&Thread, 0, sizeof(Thread));
	Thread.dwSize = sizeof(Thread);

	uint32 ThisThreadID = GetCurrentThreadId();

	int Len = pData->m_ThreadTrackingContext.m_ThreadsCopy.Len();
	MRTC_SystemInfoInternal::CThreadTrackingContext::CThread *pThreads = pData->m_ThreadTrackingContext.m_ThreadsCopy.GetBasePtr();
		
	uint8 Packet[1024];
	for (int i = 0; i < Len; ++i)
	{
		if (pThreads[i].m_ThreadID != ThisThreadID)
		{
			HANDLE ThreadHandle = pThreads[i].m_Thread;
			M_ASSERT(ThreadHandle, "");
			BOOL Ret = SuspendThread(ThreadHandle);
			if (Ret >= 0)
			{
				M_ASSERT(Ret >= 0, "");

				CONTEXT ThreadContext;
				ZeroMemory(&ThreadContext, sizeof(ThreadContext));
				ThreadContext.ContextFlags = CONTEXT_CONTROL;
				Ret = GetThreadContext(ThreadHandle, &ThreadContext);
				M_ASSERT(Ret, "");

				uint8 *pPacket = Packet;
				uint8 *pPacketMax = pPacket+1024-4;

				*((uint32 *)pPacket) = ThreadContext.Eip; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
				mint StackFrame = ThreadContext.Ebp;
				
				MEMORY_BASIC_INFORMATION MemoryInfo;
				int Size = sizeof(MemoryInfo);
				memset(&MemoryInfo, 0, Size);

				int nBytes = VirtualQuery((const void *)(mint)ThreadContext.Esp, &MemoryInfo, Size);
				M_ASSERT(nBytes == sizeof(MemoryInfo), "");
	//					M_ASSERT(MemoryInfo.BaseAddress == MemoryInfo.AllocationBase, "");

				mint StackStart = (mint)MemoryInfo.BaseAddress;
				mint StackEnd =  StackStart+MemoryInfo.RegionSize;

				while (pPacket < pPacketMax && StackFrame)
				{
					if (!IsGoodStackPtr((void *)StackFrame, sizeof(mint) * 2, StackStart, StackEnd))
						break;

					mint CodePtr = *((mint *)(StackFrame + sizeof(mint)));
					*((uint32 *)pPacket) = CodePtr; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
					StackFrame = *((mint *)(StackFrame));
				}


				Ret = ResumeThread(ThreadHandle);
				M_ASSERT(Ret >= 0, "");

				MRTC_GetRD()->SendData(ERemoteDebug_ProfileStackTrace, Packet, pPacket - Packet, false, false);
			}

		}
	}
	
	TimeStart = CMTime::GetCPU() - TimeStart;
//	M_TRACEALWAYS("Snapshot time %f ms\n", TimeStart.GetTime()*1000.0);


#else
	MRTC_SystemInfoInternal *pData = MRTC_GetSystemInfo().m_pInternalData;

	CMTime TimeStart;
	TimeStart = CMTime::GetCPU();
	uint32 ThisProcessID = GetCurrentProcessId();
	HANDLE SnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, ThisProcessID);
	if (SnapShot != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 Thread;
		memset(&Thread, 0, sizeof(Thread));
		Thread.dwSize = sizeof(Thread);

		uint32 ThisThreadID = GetCurrentThreadId();
		 
		if (Thread32First(SnapShot, &Thread))
		{				
//			uint8 Packet[1024];
			while (1)
			{
				if (Thread.th32OwnerProcessID == ThisProcessID && Thread.th32ThreadID != ThisThreadID)
				{
					/*
					HANDLE ThreadHandle = OpenThread(THREAD_ALL_ACCESS, false, Thread.th32ThreadID);
					M_ASSERT(ThreadHandle, "");
					BOOL Ret = SuspendThread(ThreadHandle);
					M_ASSERT(Ret >= 0, "");

					CONTEXT ThreadContext;
					ZeroMemory(&ThreadContext, sizeof(ThreadContext));
					ThreadContext.ContextFlags = CONTEXT_CONTROL;
					Ret = GetThreadContext(ThreadHandle, &ThreadContext);
					M_ASSERT(Ret, "");

					uint8 *pPacket = Packet;
					uint8 *pPacketMax = pPacket+1024-4;

					*((uint32 *)pPacket) = ThreadContext.Eip; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
					mint StackFrame = ThreadContext.Ebp;
					
					MEMORY_BASIC_INFORMATION MemoryInfo;
					int Size = sizeof(MemoryInfo);
					memset(&MemoryInfo, 0, Size);

					int nBytes = VirtualQuery((const void *)(mint)ThreadContext.Esp, &MemoryInfo, Size);
					M_ASSERT(nBytes == sizeof(MemoryInfo), "");
//					M_ASSERT(MemoryInfo.BaseAddress == MemoryInfo.AllocationBase, "");

					mint StackStart = (mint)MemoryInfo.BaseAddress;
					mint StackEnd =  StackStart+MemoryInfo.RegionSize;

					while (pPacket < pPacketMax && StackFrame)
					{
						if (!IsGoodStackPtr((void *)StackFrame, sizeof(mint) * 2, StackStart, StackEnd))
							break;

						mint CodePtr = *((mint *)(StackFrame + sizeof(mint)));
						*((uint32 *)pPacket) = CodePtr; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
						StackFrame = *((mint *)(StackFrame));
					}


					Ret = ResumeThread(ThreadHandle);
					M_ASSERT(Ret >= 0, "");
					Ret = CloseHandle(ThreadHandle);
					M_ASSERT(Ret, "");*/

				}
				if (!Thread32Next(SnapShot, &Thread))
					break;
			}
		}

		CloseHandle(SnapShot);
	}
	
	TimeStart = CMTime::GetCPU() - TimeStart;
	M_TRACEALWAYS("Snapshot time %f ms\n", TimeStart.GetTime()*1000.0);
#endif
	
/*	uint8 *pPacket = Packet;
	mint Len = strlen(_pName);
	*((uint32 *)pPacket) = _ID; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
	*((uint32 *)pPacket) = _Size; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
	*((uint32 *)pPacket) = pHeader->FileHeader.TimeDateStamp; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
	*((uint32 *)pPacket) = Len; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
	memcpy(pPacket, _pName, Len); pPacket += Len;
	MRTC_GetRD()->SendData(ERemoteDebug_DllLoad, Packet, pPacket - Packet, false, false);
	ERemoteDebug_ProfileStackTrace
*/
#endif
#endif
}

void MRTC_SystemInfo::RD_ClientInit(void *_pPacket, mint &_Size)
{
#if defined(MRTC_ENABLE_REMOTEDEBUGGER)
	uint8 *pPacket = (uint8 *)_pPacket;

	uint32 *pSize = (uint32 *)pPacket; pPacket+= 4;
	*((uint32 *)pPacket) = ERemoteDebug_Init; SwapLE(*((uint32 *)pPacket)); pPacket += sizeof(uint32);

	// Platform
#ifdef PLATFORM_XENON
	strcpy((char *)pPacket, "Xenon");
	pPacket += strlen((char *)pPacket)+1;
#elif defined(PLATFORM_XBOX1)
	strcpy((char *)pPacket, "Xbox");
	pPacket += strlen((char *)pPacket)+1;
#else
	strcpy((char *)pPacket, "Win32");
	pPacket += strlen((char *)pPacket)+1;
#endif

	// Debug info
#ifdef PLATFORM_XBOX
/*		int ErrRet;
	PDM_WALK_MODULES pWalkMod = NULL;
	DMN_MODLOAD modLoad;

	while( XBDM_NOERR == (ErrRet = DmWalkLoadedModules(&pWalkMod, &modLoad)) ) 
	{
		M_TRACEALWAYS("Loaded Module %s base 0x%x size %d\n", modLoad.Name, modLoad.BaseAddress, modLoad.Size);
	}
	if (ErrRet != XBDM_ENDOFLIST)
	{
		
	}
	DmCloseLoadedModules(pWalkMod);*/

	uint32 *pnModules = (uint32 *)pPacket; pPacket += 4;
	*pnModules = 0;

	{
		HRESULT error;
		PDM_WALK_MODULES pWalkMod = NULL;
		DMN_MODLOAD modLoad;

		while( XBDM_NOERR == (error = DmWalkLoadedModules(&pWalkMod, &modLoad)) ) 
		{
			M_TRACEALWAYS("0x%08x(%d): %s ID: 0x%08x TWO: 0x%08x\n", modLoad.BaseAddress, modLoad.Size, modLoad.Name, modLoad.TimeStamp, modLoad.Size);

			IMAGE_NT_HEADERS *pHeader = NULL;
			// Look for header information

			uint32 *pSearch = (uint32 *)modLoad.BaseAddress;
			for (int i = 0; i < modLoad.Size; i += 4)
			{
				uint32 Find1;
				((uint8 *)&Find1)[0] = 'P';
				((uint8 *)&Find1)[1] = 'E';
				((uint8 *)&Find1)[2] = 0;
				((uint8 *)&Find1)[3] = 0;
				if (*pSearch == Find1)
				{
					pHeader = (IMAGE_NT_HEADERS *)pSearch;
					break;
				}
				++pSearch;
			}

			if (strcmp(modLoad.Name, "xboxkrnl.exe") == 0)
				strcpy(modLoad.Name, "xboxkrnld.exe");

			if (strcmp(modLoad.Name, "xam.xex") == 0)
				strcpy(modLoad.Name, "xamd.xex");
			if (strcmp(modLoad.Name, "xnet.xex") == 0)
				strcpy(modLoad.Name, "xnetd.xex");

			int Len = strlen(modLoad.Name);

			*((uint32 *)pPacket) = (uint32)modLoad.BaseAddress; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
			if (pHeader)
			{
//				IMAGE_NT_HEADERS Temp;
//				Temp = *pHeader;
				uint32 Size = pHeader->OptionalHeader.SizeOfImage;
				SwapLE(Size);
				*((uint32 *)pPacket) = Size; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
			}
			else
			{
				*((uint32 *)pPacket) = modLoad.Size; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
			}

			*((uint32 *)pPacket) = modLoad.TimeStamp; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
			*((uint32 *)pPacket) = Len; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
			memcpy(pPacket, modLoad.Name, Len); pPacket += Len;
			
			++(*pnModules);			
		}

		SwapLE((*pnModules));
		if (error != XBDM_ENDOFLIST)
		{
			// Handle error.
		}
		DmCloseLoadedModules(pWalkMod);
	}

#else

	HANDLE SnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
	if (SnapShot != INVALID_HANDLE_VALUE)
	{
	
		MODULEENTRY32 Module;
		memset(&Module, 0, sizeof(Module));
		Module.dwSize = sizeof(Module);
		uint32 *pnModules = (uint32 *)pPacket; pPacket += 4;
		*pnModules = 0;

#if 1
		if (Module32First(SnapShot, &Module))
		{			
			while (1)
			{
				IMAGE_NT_HEADERS *pHeader = NULL;
				// Look for header information

				uint32 *pSearch = (uint32 *)Module.modBaseAddr;
				for (int i = 0; i < Module.modBaseSize; i += 4)
				{
					uint32 Find1 = '\0EP\0';
					if (*pSearch == Find1)
					{
						pHeader = (IMAGE_NT_HEADERS *)pSearch;
						break;
					}
					++pSearch;
				}

				if (!pHeader)
				{
					M_TRACEALWAYS("Failed to find PE header for module: %s\n", Module.szModule);
				}
				else
				{
					M_TRACEALWAYS("0x%08x(%d): %s ID: 0x%08x TWO: 0x%08x\n", Module.modBaseAddr, Module.modBaseSize, Module.szModule, pHeader->FileHeader.TimeDateStamp, Module.modBaseSize);
					mint Len = strlen(Module.szModule);
					*((uint32 *)pPacket) = (uint32)(mint)Module.modBaseAddr; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
					*((uint32 *)pPacket) = Module.modBaseSize; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
					*((uint32 *)pPacket) = pHeader->FileHeader.TimeDateStamp; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
					*((uint32 *)pPacket) = Len; SwapLE(*((uint32 *)pPacket)); pPacket += 4;
					memcpy(pPacket, Module.szModule, Len); pPacket += Len;
					
					++(*pnModules);

				}
				// Limit number of modules sent here
				if ((*pnModules) > 32)
					break;

				if (!Module32Next(SnapShot, &Module))
					break;

			}
		}
#endif
		SwapLE((*pnModules));

		CloseHandle(SnapShot);
	}

	MRTC_SystemInfoInternal *pData = MRTC_GetSystemInfo().m_pInternalData;
	if (pData->m_pDebugger)
	{
		MRTC_SystemInfoInternal::CWin32Debugger * pDebuggah = pData->m_pDebugger;
		pData->m_pDebugger = NULL;
		pDebuggah->~CWin32Debugger();
	}

	pData->m_pDebugger = new (pData->m_DebuggerData) MRTC_SystemInfoInternal::CWin32Debugger();
#endif

	_Size = *pSize = (pPacket - (uint8 *)_pPacket);
	SwapLE(*pSize);

#ifdef PLATFORM_XBOX
	// Send initial statistics
	DM_MEMORY_STATISTICS Stats;
	ZeroMemory(&Stats, sizeof(Stats));
	Stats.cbSize = sizeof(Stats);

	if (!DmQueryMemoryStatistics(&Stats))
	{
		int Err = GetLastError();
	}

	gf_RDSendRegisterPhysicalHeap(0, "Main", 0, 0);
	gf_RDSendRegisterHeap((mint)g_ObjectManagerContainer.m_pMemoryManager, "Main", 0, 0);
#ifdef PLATFORM_XBOX
	gf_RDSendRegisterPhysicalHeap(1, "Contiguous", 0, 0);
#endif
#ifdef PLATFORM_XBOX1
	gf_RDSendRegisterHeap((mint)g_ObjectManagerContainer.m_pGraphicsHeap, "Graphics", 0, 0);
#ifndef M_Profile
	gf_RDSendRegisterHeap((mint)g_ObjectManagerContainer.m_pGraphicsHeapCached, "Graphics Cached", 0, 0);
#endif
#endif
	gf_RDSendRegisterPhysicalHeap(4, "Total Stats", 0, 0);

	{
		DRDCategory("Available");
		gf_RDSendPhysicalAlloc((void *)1, Stats.AvailablePages*4096, 4, gf_RDGetSequence(), 0);
	}
	{
		DRDCategory("Stack");
		gf_RDSendPhysicalAlloc((void *)2, Stats.StackPages*4096, 4, gf_RDGetSequence(), 0);
	}
	{
		DRDCategory("Virtual Page Tables");
		gf_RDSendPhysicalAlloc((void *)3, Stats.VirtualPageTablePages*4096, 4, gf_RDGetSequence(), 0);
	}
	{
		DRDCategory("System Page Tables");
		gf_RDSendPhysicalAlloc((void *)4, Stats.SystemPageTablePages*4096, 4, gf_RDGetSequence(), 0);
	}
	{
		DRDCategory("Pool");
		gf_RDSendPhysicalAlloc((void *)5, Stats.PoolPages*4096, 4, gf_RDGetSequence(), 0);
	}
	{
		DRDCategory("Mapped Virtual Memory");
		gf_RDSendPhysicalAlloc((void *)6, Stats.VirtualMappedPages*4096, 4, gf_RDGetSequence(), 0);
	}
	{
		DRDCategory("Image");
		gf_RDSendPhysicalAlloc((void *)7, Stats.ImagePages*4096, 4, gf_RDGetSequence(), 0);
	}
	{
		DRDCategory("File Cache");
		gf_RDSendPhysicalAlloc((void *)8, Stats.FileCachePages*4096, 4, gf_RDGetSequence(), 0);
	}
	{
		DRDCategory("Contiguous");
		gf_RDSendPhysicalAlloc((void *)9, Stats.ContiguousPages*4096, 4, gf_RDGetSequence(), 0);
	}
	{
		DRDCategory("Debugger");
		gf_RDSendPhysicalAlloc((void *)10, Stats.DebuggerPages*4096, 4, gf_RDGetSequence(), 0);
	}
	
	{
//		DRDCategory("Contiguous at start");
//		gf_RDSendPhysicalAlloc((void *)0, Stats.ContiguousPages*4096, 1, gf_RDGetSequence(), 0);
	}

	int Mega = 0;
#endif
#endif

}

void MRTC_SystemInfo::RD_PeriodicUpdate()
{
#ifdef PLATFORM_XBOX
	DM_MEMORY_STATISTICS Stats;
	Stats.cbSize = sizeof(Stats);

	if (!DmQueryMemoryStatistics(&Stats))
	{
		int Err = GetLastError();
	}
	{
		DRDCategory("Available");
		gf_RDSendPhysicalFree((void *)1, 4, gf_RDGetSequence());
		gf_RDSendPhysicalAlloc((void *)1, Stats.AvailablePages*4096, 4, gf_RDGetSequence(), 0);
	}
	{
		DRDCategory("Stack");
		gf_RDSendPhysicalFree((void *)2, 4, gf_RDGetSequence());
		gf_RDSendPhysicalAlloc((void *)2, Stats.StackPages*4096, 4, gf_RDGetSequence(), 0);
	}
	{
		DRDCategory("Virtual Page Tables");
		gf_RDSendPhysicalFree((void *)3, 4, gf_RDGetSequence());
		gf_RDSendPhysicalAlloc((void *)3, Stats.VirtualPageTablePages*4096, 4, gf_RDGetSequence(), 0);
	}
	{
		DRDCategory("System Page Tables");
		gf_RDSendPhysicalFree((void *)4, 4, gf_RDGetSequence());
		gf_RDSendPhysicalAlloc((void *)4, Stats.SystemPageTablePages*4096, 4, gf_RDGetSequence(), 0);
	}
	{
		DRDCategory("Pool");
		gf_RDSendPhysicalFree((void *)5, 4, gf_RDGetSequence());
		gf_RDSendPhysicalAlloc((void *)5, Stats.PoolPages*4096, 4, gf_RDGetSequence(), 0);
	}
	{
		DRDCategory("Mapped Virtual Memory");
		gf_RDSendPhysicalFree((void *)6, 4, gf_RDGetSequence());
		gf_RDSendPhysicalAlloc((void *)6, Stats.VirtualMappedPages*4096, 4, gf_RDGetSequence(), 0);
	}
	{
		DRDCategory("Image");
		gf_RDSendPhysicalFree((void *)7, 4, gf_RDGetSequence());
		gf_RDSendPhysicalAlloc((void *)7, Stats.ImagePages*4096, 4, gf_RDGetSequence(), 0);
	}
	{
		DRDCategory("File Cache");
		gf_RDSendPhysicalFree((void *)8, 4, gf_RDGetSequence());
		gf_RDSendPhysicalAlloc((void *)8, Stats.FileCachePages*4096, 4, gf_RDGetSequence(), 0);
	}
	{
		DRDCategory("Contiguous");
		gf_RDSendPhysicalFree((void *)9, 4, gf_RDGetSequence());
		gf_RDSendPhysicalAlloc((void *)9, Stats.ContiguousPages*4096, 4, gf_RDGetSequence(), 0);
	}
	{
		DRDCategory("Debugger");
		gf_RDSendPhysicalFree((void *)10, 4, gf_RDGetSequence());
		gf_RDSendPhysicalAlloc((void *)10, Stats.DebuggerPages*4096, 4, gf_RDGetSequence(), 0);
	}
#endif
}

int64 MRTC_SystemInfo::OS_Clock()
{
	int64 Tick;
	QueryPerformanceCounter((LARGE_INTEGER *)&Tick);
	return Tick;
}

uint64 MRTC_SystemInfo::OS_ClockFrequencyInt() const
{
	return m_OSFrequencyu;
}

fp32 MRTC_SystemInfo::OS_ClockFrequencyFloat() const
{
	return m_OSFrequencyfp;
}

fp32 MRTC_SystemInfo::OS_ClockFrequencyRecp() const
{
	return m_OSFrequencyRecp;
}

#ifdef CPU_AMD64
extern "C" mint AMD64_GetRDTSC();
#endif

int64 MRTC_SystemInfo::CPU_Clock()
{
#ifdef CPU_X86
	int64 Tick;
	__asm
	{
		rdtsc
		mov dword ptr [Tick], eax
		mov dword ptr [Tick+4], edx
	}
	//4611686018427387904l +  * 10
	return Tick;
#elif defined(CPU_AMD64)
	return AMD64_GetRDTSC(); 
#else
	int64 Tick;
	QueryPerformanceCounter((LARGE_INTEGER *)&Tick);
	return Tick;
#endif
}
uint64 MRTC_SystemInfo::CPU_ClockFrequencyInt() const
{
	return m_CPUFrequencyu;
}

fp32 MRTC_SystemInfo::CPU_ClockFrequencyFloat() const
{
	return m_CPUFrequencyfp;
}
fp32 MRTC_SystemInfo::CPU_ClockFrequencyRecp() const
{
	return m_CPUFrequencyRecp;
}

// -------------------------------------------------------------------
#if defined(PLATFORM_XENON) && defined(USE_PIX)
void MRTC_SystemInfo::OS_NamedEvent_Begin(const char* _pName, uint32 _Color)
{
	PIXBeginNamedEvent(_Color, _pName);
}
void MRTC_SystemInfo::OS_NamedEvent_End()
{
	PIXEndNamedEvent();
}

#else
void MRTC_SystemInfo::OS_NamedEvent_Begin(const char* _pName, uint32 _Color)
{
}
void MRTC_SystemInfo::OS_NamedEvent_End()
{
}

#endif

// -------------------------------------------------------------------
void MRTC_SystemInfo::CPU_Detect()
{
#ifdef PLATFORM_XENON
	m_nCPU = 6;
	m_CPUFeatures = 0;
#else 
	// -------------------------------------------------------------------
#ifdef PLATFORM_WIN_PC
	SYSTEM_INFO SysInf;
	GetSystemInfo(&SysInf);
	m_nCPU = SysInf.dwNumberOfProcessors;
	m_nCPU = Max(1, (int)m_nCPU);
#else
	m_nCPU = 1;
#endif

#ifndef CPU_AMD64
	char ID[13];
	ID[12] = 0;
	int CPUIDSupport = 0;
	int Flags = 0;
	int Features = 0;
	int CoreCount = 1;
	int LogicalPerPhysical = 0;
	__asm
	{
		mov eax, 0
		cpuid
		mov dword ptr[CPUIDSupport], eax
		mov dword ptr[ID], ebx
		mov dword ptr[ID+4], edx
		mov dword ptr[ID+8], ecx
		
		cmp eax, 1
		jl NoPentium

		mov eax, 1
		cpuid
		mov [Flags], eax
		mov [Features], edx
		mov eax, ebx
		shr eax, 16
		and eax, 255
		mov [LogicalPerPhysical], eax

		cmp dword ptr[CPUIDSupport],4
		jl NoCoreCount
		mov eax, 4
		xor ebx, ebx
		xor ecx, ecx
		xor edx, edx
		cpuid
		shr eax, 26
		add eax, 1
		mov [CoreCount], eax


NoCoreCount:

NoPentium:
	}

	memcpy(m_CPUID, ID, 13);
	int Stepping = Flags & 0x0f;
	m_CPUModel = (Flags & 0xf0) >> 4;
	m_CPUFamily = (Flags & 0xf00) >> 8;

	if (Stepping >= 6) m_CPUFeatures |= CPU_FEATURE_P6;
	if (Features & 0x00800000) m_CPUFeatures |= CPU_FEATURE_MMX;
	if (Features & 0x02000000) m_CPUFeatures |= CPU_FEATURE_SSE;
	if (Features & 0x04000000) m_CPUFeatures |= CPU_FEATURE_SSE2;
	if (!strncmp(ID, "GenuineIntel", 12) && Features & 0x10000000)
	{
		int HT_Enabled = 0;
		LogicalPerPhysical /= CoreCount;
		if(LogicalPerPhysical >= 1)
		{
			uint32 PHY_ID_MASK = ~0 << (LogicalPerPhysical - 1);
			uint32 PHY_ID_SHIFT = (LogicalPerPhysical - 1);

			HANDLE hCurrentProcessHandle = GetCurrentProcess();
			DWORD dwSystemAffinity, dwProcessAffinity;
			GetProcessAffinityMask(hCurrentProcessHandle, &dwSystemAffinity, &dwProcessAffinity);

			if(dwSystemAffinity == dwProcessAffinity)
			{
				// Can only do this properly if affinity isn't messed with
				DWORD dwAffinityMask = 1;
				while(dwAffinityMask != 0 && dwAffinityMask < dwProcessAffinity)
				{
					if(dwAffinityMask & dwProcessAffinity)
					{
						if(SetProcessAffinityMask(hCurrentProcessHandle, dwAffinityMask))
						{
							uint32 APIC_ID, LOG_ID, PHY_ID;
							::Sleep(1);	// Re-schedule
							__asm
							{
								mov eax, 1
								cpuid
								shr ebx, 24
								mov dword ptr [APIC_ID], ebx
							}
							LOG_ID = APIC_ID & ~PHY_ID_MASK;
							PHY_ID = APIC_ID >> PHY_ID_SHIFT;
							if(LOG_ID != 0)
							{
								HT_Enabled = 1;
								break;
							}
						}
					}
					dwAffinityMask <<= 1;
				}

				SetProcessAffinityMask(hCurrentProcessHandle, dwProcessAffinity);
			}
		}
		if(HT_Enabled)
		{
			m_CPUFeatures |= CPU_FEATURE_HT;
			m_nCPU = Max(1, (int)(m_nCPU / 2));
		}
	}
#endif

#endif

	CPU_MeasureFrequency();
	CPU_CreateNames();
}


fp64 MRTC_SystemInfo::CPU_MeasureFrequencyOnce()
{
	// -------------------------------------------------------------------
	fp64 Freq = 1000000.0;
	M_TRY
	{
		CMTime_Generic<CMTimerFuncs_CPU> t;
		LARGE_INTEGER Time;
		LARGE_INTEGER TimeStart;
		LARGE_INTEGER TimeStop;

		LARGE_INTEGER TimerFreq;
		QueryPerformanceFrequency(&TimerFreq);

		QueryPerformanceCounter(&Time);
		uint64 EndTime = Time.QuadPart + (TimerFreq.QuadPart / 100);
		
		QueryPerformanceCounter(&TimeStart);
		QueryPerformanceCounter(&Time);
#ifdef CPU_X86
		__asm { xor eax,eax };
#endif
		t.Start();
#ifdef CPU_X86
		__asm { xor eax,eax };
#endif

		while (EndTime > Time.QuadPart)
		{
			QueryPerformanceCounter(&Time);
		}

		QueryPerformanceCounter(&TimeStop);
#ifdef CPU_X86
		__asm { xor eax,eax };
#endif
		t.Stop();
#ifdef CPU_X86
		__asm { xor eax,eax };
#endif

		//This doesn't compile. Might be vc++ version difference or something...
		//uint64 Elapse = TimeStop.QuadPart - TimeStart.QuadPart;				
		fp64 Elapse = TimeStop.QuadPart - TimeStart.QuadPart;				

//				fp64 f = ((fp64)t*1000.0/(fp32)10.0*1.0);
		Freq = (fp64)t.GetCycles() / (Elapse / (fp64)TimerFreq.QuadPart);
//			g_pOS->pCon->Write(CStrF("Clocks:  %d   Freq2: %.0f", t, f));
//				Freq = Max(f, 1000000.0);

	}
	M_CATCH(
	catch(CCException)
	{
		throw;
	};
	)
	return Freq;
}

void MRTC_SystemInfo::CPU_MeasureFrequency()
{
	const int nTests = 11;
	fp64 FreqTab[nTests];
	int i,j;
	for (i = 0; i < nTests; i++) 
		FreqTab[i] = CPU_MeasureFrequencyOnce();

	// Bubble-bobble
	for (i = 0; i < (nTests-1); i++)
		for (j = i+1; j < nTests; j++)
			if (FreqTab[i] > FreqTab[j]) Swap(FreqTab[i], FreqTab[j]);

	fp32 Freq = FreqTab[nTests >> 1];// * 10.0;
	m_CPUFrequencyfp = Freq;
	m_CPUFrequencyu = Freq;

	m_CPUFrequencyRecp = 1.0 / Freq;

	LARGE_INTEGER TimerFreq;
	QueryPerformanceFrequency(&TimerFreq);

	m_OSFrequencyu = TimerFreq.QuadPart;
	m_OSFrequencyfp = m_OSFrequencyu;
	m_OSFrequencyRecp = 1.0 / (fp64)m_OSFrequencyu;

	M_TRACE("OS frequency %f MHz\n", m_OSFrequencyfp / 1000000.0);
	M_TRACE("CPU frequency %f MHz\n", m_CPUFrequencyfp / 1000000.0);

}

void MRTC_SystemInfo::OS_ClockFrequencyUpdate()
{
	LARGE_INTEGER TimerFreq;
	QueryPerformanceFrequency(&TimerFreq);

	m_OSFrequencyu = TimerFreq.QuadPart;
	m_OSFrequencyfp = m_OSFrequencyu;
	m_OSFrequencyRecp = 1.0 / (fp64)m_OSFrequencyu;
}

void MRTC_SystemInfo::CPU_CreateNames()
{

#ifdef CPU_X86
	// -------------------------------------------------------------------
#ifdef CPU_X86
	CFStr CPUName = CFStrF("%s, %dth generation, Model %d", (char*) m_CPUID, m_CPUFamily, m_CPUModel);
	CFStr CPUNameFeatures = CPUName;
#elif defined(CPU_AMD64)
	CFStr CPUName("AMD64 (EMT64)");
	CFStr CPUNameFeatures = CPUName;
#else
	#error "Implement this"
#endif

#ifdef CPU_X86
	if (m_CPUFeatures & CPU_FEATURE_MMX) CPUNameFeatures += ", MMX";
	if (!(m_CPUFeaturesEnabled & CPU_FEATURE_MMX)) CPUNameFeatures += "(disabled)";
	if (m_CPUFeatures & CPU_FEATURE_SSE) CPUNameFeatures += ", SSE";
	if (!(m_CPUFeaturesEnabled & CPU_FEATURE_SSE)) CPUNameFeatures += "(disabled)";
	if (m_CPUFeatures & CPU_FEATURE_SSE2) CPUNameFeatures += ", SSE2";
	if (!(m_CPUFeaturesEnabled & CPU_FEATURE_SSE2)) CPUNameFeatures += "(disabled)";
	if (m_CPUFeatures & CPU_FEATURE_HT) CPUNameFeatures += ", HT";
	if (!(m_CPUFeaturesEnabled & CPU_FEATURE_HT)) CPUNameFeatures += "(disabled)";
#elif defined(CPU_AMD64)
	CPUNameFeatures += ", MMX";
	CPUNameFeatures += ", SSE";
	CPUNameFeatures += ", SSE2";
#else
	#error "Implement this"
#endif

	CPUNameFeatures += CFStrF(", %.0f Mhz", m_CPUFrequencyfp / 1000000.0);

	strcpy(m_CPUName, CPUName.Str());
	strcpy(m_CPUNameWithFeatures, CPUNameFeatures.Str());
#endif
}

/*
fp64 MRTC_SystemInfo::CPU_GetTime()
{
#ifdef CONSTANT_FRAME_RATE
	extern fp64 g_SimulatedTime;
	return g_SimulatedTime;
#else
	return fp64(CPU_GetClock()) * MRTC_GetObjectManager()->m_SystemInfo.m_CPUFrequencyRecp;
#endif
}
*/


void* MRTC_SystemInfo::OS_GetProcessID()
{
#ifdef PLATFORM_WIN_PC
	return (void*)(mint)::GetCurrentProcessId();
#else
	return 0;
#endif
}

void* MRTC_SystemInfo::OS_GetThreadID()
{
	return (void*)(mint)::GetCurrentThreadId();
}

void MRTC_SystemInfo::OS_Yeild()
{

#if defined( PLATFORM_XENON )
	SwitchToThread();
#else
	Sleep(0);
#endif

}

void MRTC_SystemInfo::OS_Sleep(int _Milliseconds)
{
	if (_Milliseconds < 1)
		_Milliseconds = 1;
	::Sleep(_Milliseconds);
}

// -------------------------------------------------------------------

void* MRTC_SystemInfo::OS_ThreadCreate(uint32(M_STDCALL*_pfnEntryPoint)(void*), int _StackSize, void* _pContext, int _Priority, const char* _pName)
{
	DWORD Prio;
	if (_Priority < MRTC_THREAD_PRIO_LOW)
		Prio = THREAD_PRIORITY_IDLE;
	else if (_Priority < MRTC_THREAD_PRIO_BELOWNORMAL)
		Prio = THREAD_PRIORITY_LOWEST;
	else if (_Priority < MRTC_THREAD_PRIO_NORMAL)
		Prio = THREAD_PRIORITY_BELOW_NORMAL;
	else if (_Priority < MRTC_THREAD_PRIO_ABOVENORMAL)
		Prio = THREAD_PRIORITY_NORMAL;
	else if (_Priority < MRTC_THREAD_PRIO_HIGHEST)
		Prio = THREAD_PRIORITY_ABOVE_NORMAL;
	else if (_Priority < MRTC_THREAD_PRIO_TIMECRITICAL)
		Prio = THREAD_PRIORITY_HIGHEST;
	else
		Prio = THREAD_PRIORITY_TIME_CRITICAL;

#ifdef MRTC_ENABLE_MSCOPE
	if (_StackSize < 128*1024)
		_StackSize = 128*1024;
#endif
	
	uint32 ThreadID = 0;
	void* hThread = ::CreateThread(NULL, _StackSize, _pfnEntryPoint, _pContext, 0, &ThreadID);
	::SetThreadPriority(hThread, Prio);	
	return hThread;
}

int MRTC_SystemInfo::OS_ThreadDestroy(void* _hThread)
{
	while(1)
	{
		MRTC_GOM()->m_pGlobalLock->Lock();
		if (!OS_ThreadIsRunning(_hThread))
		{
			int ExitCode = OS_ThreadGetExitCode(_hThread);
			::CloseHandle(_hThread);
			MRTC_GOM()->m_pGlobalLock->Unlock();
			return ExitCode;
		}
		MRTC_GOM()->m_pGlobalLock->Unlock();
		OS_Sleep(1);
	}
}

void MRTC_SystemInfo::OS_ThreadTerminate(void* _hThread, int _ExitCode)
{
#ifdef PLATFORM_XBOX
	// FIXME: No TerminateThread on Xbox
	M_ASSERT(0, "?");
#else
	TerminateThread(_hThread, _ExitCode);
#endif
}

bool MRTC_SystemInfo::OS_ThreadIsRunning(void* _hThread)
{
	M_ASSERT(MRTC_THREAD_STILLACTIVE == STILL_ACTIVE, "?");

	uint32 ExitCode = 0;
	GetExitCodeThread(_hThread, &ExitCode);
	return (ExitCode == MRTC_THREAD_STILLACTIVE);
}

void MRTC_SystemInfo::OS_ThreadExit(int _ExitCode)
{
	::ExitThread(_ExitCode);
}

int MRTC_SystemInfo::OS_ThreadGetExitCode(void* _hThread)
{
	uint32 ExitCode = 0;
	::GetExitCodeThread(_hThread, &ExitCode);
	return ExitCode;
}

// -------------------------------------------------------------------

void* MRTC_SystemInfo::OS_MutexOpen(const char* _pName)
{
#ifdef PLATFORM_WIN_PC
	void* hLock = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, _pName);

	if (!hLock)
	{
		hLock = CreateMutexA(NULL, FALSE, _pName);
	}
	
	if (!hLock)
	{
		hLock = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, _pName);
	}
	
	if (!hLock)
	{
		exit(-1);
	}

	return hLock;

#else
	static uint8 lCSMem[sizeof(MRTC_CriticalSection)];
	static MRTC_CriticalSection* pCSMem = NULL;

	if (!pCSMem)
	{
	
		pCSMem = new((void*)lCSMem)MRTC_CriticalSection;
	
	}

	return pCSMem;

#endif
}

void MRTC_SystemInfo::OS_MutexClose(void* _hLock)
{
#ifdef PLATFORM_WIN_PC
	CloseHandle(_hLock);
#else
	// Nada
#endif
}

void MRTC_SystemInfo::OS_MutexLock(void* _hLock)
{
#ifdef PLATFORM_WIN_PC
	WaitForSingleObject(_hLock, INFINITE);

#else
	MRTC_CriticalSection* pCS = (MRTC_CriticalSection*) _hLock;
	pCS->Lock();

#endif
}

bool MRTC_SystemInfo::OS_MutexTryLock(void* _hLock)
{
#ifdef PLATFORM_WIN_PC
	if(WaitForSingleObject(_hLock, 0) == WAIT_OBJECT_0)
		return true;

	return false;
#else
	MRTC_CriticalSection* pCS = (MRTC_CriticalSection*) _hLock;
	return pCS->TryLock() != 0;
#endif
}

void MRTC_SystemInfo::OS_MutexUnlock(void* _hLock)
{
#ifdef PLATFORM_WIN_PC
	ReleaseMutex(_hLock);

#else
	MRTC_CriticalSection* pCS = (MRTC_CriticalSection*) _hLock;
	pCS->Unlock();

#endif
}

void MRTC_SystemInfo::OS_Assert(const char* _pMsg, const char* _pFile, int _Line)
{
	if (MRTC_GetObjectManager()->m_bAssertHandler)
	{
		CFStr Msg = CFStrF("%s (%s:%d)", _pMsg, _pFile, _Line);
		Error_static("Assert failed", Msg);
	}
	else
	{
//	LogFile i assert är precis skitdåligt om den assertar i filsystemet. /mh
//		LogFile(CFStrF("ERROR: Assert failed - %s  (%s:%d)", _pMsg, _pFile, _Line).Str());
		OS_Trace("%s(%d) : ERROR: Assert failed - %s\n", _pFile, _Line, _pMsg);
#ifndef PLATFORM_CONSOLE
		if (MRTC_GetObjectManager()->m_bBreakOnAssert)
		{
			M_BREAKPOINT;
		}
#endif
	}
}

void MRTC_SystemInfo::OS_TraceRaw(const char * _pMsg)
{
#if defined(M_Profile) || defined (PLATFORM_WIN)
	OutputDebugStringA(_pMsg);
#endif
}

void M_ARGLISTCALL MRTC_SystemInfo::OS_Trace(const char * _pStr, ...)
{
#if defined(M_Profile) || defined (PLATFORM_WIN)
	static int ms_iTrace = 0;	// Line count

	char lBuffer[4096];

#if defined(PLATFORM_XBOX)
//	CStrBase::snprintf((char*) lBuffer, 4095, "(%.6d) ", ms_iTrace); 
//	ms_iTrace++;
#endif

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

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
#ifndef MRTC_ENABLE_REMOTEDEBUGGER_STATIC
	if (MRTC_GetObjectManager() && MRTC_GetObjectManager()->GetRemoteDebugger())
#endif
		if (MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_Trace)
			MRTC_GetRD()->SendData(ERemoteDebug_Trace, lBuffer, strlen(lBuffer) + 1, false, false);
#endif
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



class CWin32File;
class MRTC_Win32AsyncInstance
{
	void operator delete(void *)
	{
		M_BREAKPOINT;
	}
	void *operator new(mint _Size)
	{
		M_BREAKPOINT;
	}
public:
	void *operator new(mint _Size, void *_pPtr)
	{
		return _pPtr;
	}
	OVERLAPPED m_Overlapped;
	DLinkDS_Link(MRTC_Win32AsyncInstance, m_LinkReadQueue);
	DLinkDS_Link(MRTC_Win32AsyncInstance, m_FileLink);
	CWin32File *m_pW32File;
	void *m_pFilen;
	void *m_pData;
	int m_Length;
	int m_bCompleted;
	int m_bPending;
	int m_bError;
	int m_bWrite;
#ifdef PLATFORM_XBOX
	uint32 m_BytesTransfered;
	uint32 m_ErrorCode;
#endif

	~MRTC_Win32AsyncInstance();

	bool IsCompleted()
	{
		if (m_bError)
			return true;

		if (m_bPending)
			return false;

		if (m_bCompleted)
		{
			return true;
		}

#ifdef PLATFORM_XBOX
		m_bCompleted = Volatile(m_Overlapped.hEvent) == NULL;
#else
		if (HasOverlappedIoCompleted(&m_Overlapped))
		{
			m_bCompleted = true;
		}
#endif
		return m_bCompleted != 0;
	}

	int64 BytesProcessed()
	{
		if (m_bError)
		{
			FileError_static("BytesProcessed", "The file read returned an error", 0);
			return -1;
		}

		if (!m_bCompleted)
		{
			FileError_static("BytesProcessed", "Cannot be called before the file is done", 0);
			return -1;
		}

#ifdef errorchance
		if ((MRTC_RAND() % errorchance) == 0)
			FileError_static("BytesProcessed", "File Processing Failed", 0);
#endif

#ifdef PLATFORM_XBOX
		DWORD nBytes = m_BytesTransfered;

		if (m_ErrorCode) 
		{ 
			// deal with the error code 
			switch (m_ErrorCode) 
			{ 
			case ERROR_HANDLE_EOF: 
				{ 
					return nBytes;
				} 
				break;
			case ERROR_IO_PENDING: 
			case ERROR_IO_INCOMPLETE:
				{ 
					return -1;
				}
				break;
			default:
				FileError_static("BytesProcessed", "File Processing Failed", 0);
				return -1;
			} 
		} 
			
		return nBytes;
#else
		DWORD nBytes = -1;
		int Success = GetOverlappedResult(m_pFilen, &m_Overlapped, &nBytes, false);

		if (!Success) 
		{ 
			// deal with the error code 
			switch (GetLastError()) 
			{ 
			case ERROR_HANDLE_EOF: 
				{ 
					return nBytes;
				} 
				break;
			case ERROR_IO_PENDING: 
			case ERROR_IO_INCOMPLETE:
				{ 
					return -1;
				}
				break;
			default:
				FileError_static("BytesProcessed", "File Processing Failed", 0);
				return -1;
			} 
		} 
			
		return nBytes;
#endif
	}
};

class CWin32File
{
public:
	void *m_pFileHandle;
	MRTC_CriticalSection m_Lock;
	DLinkDS_List(MRTC_Win32AsyncInstance, m_FileLink) m_Requests;
	DLinkDS_Link(CWin32File, m_CloseLink);
	DWORD m_OpenAttribs;
	int m_iDrive;
	int m_bDeferClose;
};

MRTC_Win32AsyncInstance::~MRTC_Win32AsyncInstance()
{
	M_LOCK(m_pW32File->m_Lock);
	m_FileLink.Unlink();
}


#ifdef PLATFORM_WIN_PC

	class COSInfoClass
	{
	public:
		int m_bHasInit;
		OSVERSIONINFO m_OSInfo;	

		M_INLINE void Init()
		{
			if (!m_bHasInit)
			{
				m_OSInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
				GetVersionEx(&m_OSInfo);
				m_bHasInit = true;
				if (!IsNT())
				{
					MessageBox(NULL, "WinME or lower is not supported", "Error", MB_OK|MB_ICONERROR);
					ExitProcess(0);
				}
			}
		}

		M_INLINE bool IsNT()
		{
			Init();
			return (m_OSInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
		}


	};
	static COSInfoClass g_OsInfo = {0};

#elif defined(PLATFORM_XBOX)
	//
	// XBOX
	//

	class COSInfoClass
	{
	public:

		class CDriveManager : public MRTC_Thread_Core
		{
		public:
			int m_bQuit;

			CDriveManager()
			{
				m_bQuit = 0;
				Thread_Create(NULL, 32768, MRTC_THREAD_PRIO_HIGHEST);
			}

			~CDriveManager()
			{
				m_bQuit = 1;
				m_Event.Signal();
				Thread_Destroy();
			}

			DLinkDS_List(MRTC_Win32AsyncInstance, m_LinkReadQueue) m_QueuedReadRequests;
			MRTC_CriticalSection m_Lock;
			MRTC_Event m_Event;

			
			static VOID CALLBACK FileIOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
			{
//				M_TRACEALWAYS("Io completion\n");
				MRTC_Win32AsyncInstance *pRequest = (MRTC_Win32AsyncInstance *)lpOverlapped->hEvent;
				pRequest->m_BytesTransfered = dwNumberOfBytesTransfered;
				pRequest->m_ErrorCode = dwErrorCode;
				Volatile(pRequest->m_Overlapped.hEvent) = NULL;
#ifdef PLATFORM_XENON
				if (dwErrorCode)
					M_TRACEALWAYS("Error retured from FileIOCompletionRoutine: %d\n", dwErrorCode);
#endif
			}


			int Thread_Main()
			{
				MRTC_SystemInfo::Thread_SetName("MRTC Drive manager");
				while (1)
				{
					m_Event.WaitTimeout(-1.0);

					M_TRY
					{
						if (MRTC_GetObjectManager()->m_pByteStreamManager)
							if (GetStreamManager()->ServiceDrives(false))
								Sleep(500);
					}
					M_CATCH(
					catch (CCExceptionFile)
					{
					}
					)

					{
						M_LOCK(m_Lock);
						while (!m_QueuedReadRequests.IsEmpty())
						{
							DLinkDS_Iter(MRTC_Win32AsyncInstance, m_LinkReadQueue) Iter = m_QueuedReadRequests;
							MRTC_Win32AsyncInstance *pRequest = Iter;
							pRequest->m_LinkReadQueue.Unlink();
							{
								M_UNLOCK(m_Lock);

								pRequest->m_Overlapped.hEvent = (HANDLE)pRequest;
								pRequest->m_Overlapped.Internal = 0;
								pRequest->m_Overlapped.InternalHigh = 0;

								bool Success;
								if (pRequest->m_bWrite)
									Success = WriteFileEx(pRequest->m_pFilen, pRequest->m_pData, pRequest->m_Length, &pRequest->m_Overlapped, FileIOCompletionRoutine) != 0;
								else
									Success = ReadFileEx(pRequest->m_pFilen, pRequest->m_pData, pRequest->m_Length, &pRequest->m_Overlapped, FileIOCompletionRoutine) != 0;

								if (!Success) 
								{ 
									// deal with the error code 
									switch (int dwError = GetLastError()) 
									{ 
										case ERROR_HANDLE_EOF: 
										{
											pRequest->m_bCompleted = true;
										} 
										break;
									
									case ERROR_IO_PENDING: 
										{ 
										}
										break;
									default:
#ifdef PLATFORM_XENON
										M_TRACEALWAYS("Error retured from read/write: %d\n", dwError);
#endif
										pRequest->m_bError = true;
									}
								}
								//else
								//{
								//	pRequest->m_bCompleted = true;
								//}	
								pRequest->m_bPending = false;

							}
						}
					}
					if (m_bQuit)
						return 0;
				}
			}

			void QueueRequest(MRTC_Win32AsyncInstance *_AsyncQueue, int _Length, void *_pData)
			{
				M_ASSERT(_pData, "Error");
				if(!_pData)
					M_BREAKPOINT;

				_AsyncQueue->m_pData = _pData;
				_AsyncQueue->m_Length = _Length;

				{
					M_LOCK(m_Lock);
					m_QueuedReadRequests.Insert(_AsyncQueue);
				}

				m_Event.Signal();
			}
		};

		class CFileManager : public MRTC_Thread_Core
		{
		public:
			int m_bQuit;
			CDriveManager m_DriveDVD;
			CDriveManager m_DriveHD;

			CFileManager()
			{
				m_bQuit = 0;
				Thread_Create(NULL, 32768, MRTC_THREAD_PRIO_HIGHEST);
			}

			~CFileManager()
			{
				m_bQuit = 1;
				m_Event.Signal();
				Thread_Destroy();

				while (!m_FilesForClose.IsEmpty())
				{
					UpdateDelete();
				}

			}

			DLinkDS_List(CWin32File, m_CloseLink) m_FilesForClose;
			MRTC_CriticalSection m_Lock;
			MRTC_Event m_Event;

			void UpdateDelete()
			{
				M_LOCK(m_Lock);
				DLinkDS_Iter(CWin32File, m_CloseLink) Iter = m_FilesForClose;
				while (Iter)
				{
					CWin32File *pFile = Iter;
					++Iter;
					{
						M_UNLOCK(m_Lock);	
						{
							bint IsEmpty;
							{
								M_LOCK(pFile->m_Lock);
								IsEmpty = pFile->m_Requests.IsEmpty();
							}

							if (IsEmpty)
							{
								{
									M_LOCK(m_Lock);
									pFile->m_CloseLink.Unlink();
								}

								CloseHandle(pFile->m_pFileHandle);
								delete pFile;
							}
						}
					}
				}
			}

			int Thread_Main()
			{
				MRTC_SystemInfo::Thread_SetName("MRTC File manager");
				while (1)
				{
					m_Event.WaitTimeout(0.100f);

					if (m_bQuit)
						return 0;

					UpdateDelete();
					M_TRY
					{
						if (MRTC_GetObjectManager()->m_pByteStreamManager)
							if (GetStreamManager()->ServiceDrives(false))
								Sleep(500);
					}
					M_CATCH(
					catch (CCExceptionFile)
					{
					}
					)
				}
			}

			void CloseFile(CWin32File *_pFile)
			{
				
				{
					M_LOCK(m_Lock);
					m_FilesForClose.Insert(_pFile);
				}

				m_Event.Signal();
			}
		};

		CFileManager *m_pFileManager;

		M_INLINE void Init()
		{
			if (!m_pFileManager)
			{
				m_pFileManager = DNew(CFileManager) CFileManager;
			}
		}

		void Destroy()
		{
			if (m_pFileManager)
				delete m_pFileManager;
		}

		void CloseFile(CWin32File *_pFile)
		{
			Init();
			m_pFileManager->CloseFile(_pFile);
		}

		void QueueRequest(MRTC_Win32AsyncInstance *_AsyncQueue, int _Length, void *_pData)
		{
			Init();
			if (_AsyncQueue->m_pW32File->m_iDrive == 0)
				m_pFileManager->m_DriveHD.QueueRequest(_AsyncQueue, _Length, _pData);
			else
				m_pFileManager->m_DriveDVD.QueueRequest(_AsyncQueue, _Length, _pData);
		}

		M_INLINE bool IsNT(){return true;}
	};

	static COSInfoClass g_OsInfo = {0};

	void DestroyOsInfo()
	{
		g_OsInfo.Destroy();
	}


#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| File
|__________________________________________________________________________________________________
\*************************************************************************************************/
CFStr ReplaceSlashes(const char* _pFileName)
{
	CFStr ret;
	ret = _pFileName;
	char* pDest = ret.GetStr();
	for (; *pDest; pDest++)
		if (*pDest == '/')
			*pDest = '\\';

	if (_pFileName[0] && pDest[-1] == '\\')
			pDest[-1] = 0;
	return ret;
}

void TraceLastError()
{
#ifdef PLATFORM_WIN32_PC
	LPVOID lpMsgBuf;						
#endif
	int Err = GetLastError();			
	if (Err != 0)							
	{										
#ifdef PLATFORM_WIN32_PC
		int Len = FormatMessage(			
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
			NULL, Err,						
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
			(LPTSTR) &lpMsgBuf, 0, NULL);	
			CStr ErrorStr;

		if (Len > 0)	
		{
			M_TRACEALWAYS("Win32: %s\n", (char*) lpMsgBuf);
			LocalFree( lpMsgBuf );
		}
		else			
#endif
			M_TRACEALWAYS("Invalid Win32 error code %d\n", Err); 

	}	
}

#ifdef PLATFORM_XENON
__declspec(thread) int g_IgnoreFileOpen = 0;
#endif
void *MRTC_SystemInfo::OS_FileOpen(const char *_pFileName, bool _bRead, bool _bWrite, bool _bCreate, bool _bTruncate, bool _bDeferClose)
{
	DWORD Access = 0;
	if (_bRead)
		Access |= GENERIC_READ;
	if (_bWrite)
		Access |= GENERIC_WRITE;

	DWORD Dispo = OPEN_EXISTING;
	if (_bCreate && _bTruncate)
		Dispo = CREATE_ALWAYS;
	else if (!_bCreate && _bTruncate)
		Dispo = TRUNCATE_EXISTING;
	else if (_bCreate)
		Dispo = OPEN_ALWAYS;

	CWin32File *pFile = DNew(CWin32File) CWin32File;

#ifdef PLATFORM_XENON
	if (!g_IgnoreFileOpen)
		M_TRACEALWAYS("Opening: %s\n", _pFileName);
//	if (!strstr(_pFileName, ".swwc") && !strstr(_pFileName, ".xwc")) 
#endif

	//  | FILE_FLAG_NO_BUFFERING
#ifdef PLATFORM_XBOX
	CFStr FileName = ReplaceSlashes(_pFileName);
	pFile->m_pFileHandle = CreateFile(FileName, Access, FILE_SHARE_READ, NULL, Dispo, FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL);
	pFile->m_OpenAttribs = Access;
	pFile->m_bDeferClose = _bDeferClose && !(Access & GENERIC_WRITE);
	if (_pFileName[0] == 'd' || _pFileName[0] == 'D')
		pFile->m_iDrive = 1;
	else
		pFile->m_iDrive = 0;
//	OS_Trace("Drive %d\n", pFile->m_iDrive);
#else
	if (g_OsInfo.IsNT())
	{
		if (GetStreamManager()->GetCacheEnable() == 0)
			pFile->m_pFileHandle = CreateFileA(_pFileName, Access, FILE_SHARE_READ, NULL, Dispo, FILE_FLAG_OVERLAPPED, NULL);
		else
			pFile->m_pFileHandle = CreateFileA(_pFileName, Access, FILE_SHARE_READ, NULL, Dispo, FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL);

		pFile->m_OpenAttribs = Access;
		pFile->m_bDeferClose = _bDeferClose && !(Access & GENERIC_WRITE);
	}
#endif

	if (pFile->m_pFileHandle == INVALID_HANDLE_VALUE)
	{
		M_TRACEALWAYS("Failed to open file: %s\n", _pFileName);
		TraceLastError();
		HRESULT Resse = GetLastError();
		delete pFile;

		return NULL;
	}
	else
	{
		return pFile;
	}
}

void MRTC_SystemInfo::OS_FileClose(void *_pFile)
{
	CWin32File *pFile = (CWin32File *)_pFile;
#ifdef PLATFORM_XBOX

	if (pFile->m_bDeferClose)
		g_OsInfo.CloseFile(pFile);
	else
	{
		{
			M_LOCK(pFile->m_Lock);
			while (!pFile->m_Requests.IsEmpty())
			{
				M_UNLOCK(pFile->m_Lock);
				OS_Sleep(1);
			}
		}
		CloseHandle(pFile->m_pFileHandle);
		delete pFile;
	}
#else
	if (g_OsInfo.IsNT())
	{
		CloseHandle(pFile->m_pFileHandle);
		delete pFile;
	}
#endif
}

fint MRTC_SystemInfo::OS_FileSize(void *_pFile)
{
	if (g_OsInfo.IsNT())
	{
		CWin32File *pFile = (CWin32File *)_pFile;
		LARGE_INTEGER LG;
		LG.LowPart = GetFileSize(pFile->m_pFileHandle, (DWORD *)&LG.HighPart);

		if (LG.LowPart == -1 && GetLastError() != NO_ERROR)
		{
			FileError_static("MRTC_SystemInfo::OS_FileSize", "Could not get filesize for file", 0);
		}

		return LG.QuadPart;
	}
	else
	{
			return 0;
	}
}

bool MRTC_SystemInfo::OS_DirectoryExists(const char *_pPath)
{
#if 0
	WIN32_FIND_DATA Data;
	HANDLE Ret = FindFirstFile(_pPath, &Data);
	if (Ret != INVALID_HANDLE_VALUE)
	{

		 FindClose(Ret);

		if (!(Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			return true;
		else
			return false;
	}
	else
		return false;
#else
	CFStr Path = ReplaceSlashes(_pPath);
	DWORD Attribs = GetFileAttributesA(Path);
	if (Attribs != 0xffffffff && (Attribs & FILE_ATTRIBUTE_DIRECTORY))
		return true;
	else
		return false;
#endif
}

bool MRTC_SystemInfo::OS_FileExists(const char *_pPath)
{
#if 0
	WIN32_FIND_DATA Data;
	HANDLE Ret = FindFirstFile(_pPath, &Data);
	if (Ret != INVALID_HANDLE_VALUE)
	{

		 FindClose(Ret);

		if (!(Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			return true;
		else
			return false;
	}
	else
		return false;
#else
	CFStr Path = ReplaceSlashes(_pPath);
	DWORD Attribs = GetFileAttributesA(Path);
	if (Attribs != 0xffffffff && !(Attribs & FILE_ATTRIBUTE_DIRECTORY))
		return true;
	else
		return false;
#endif
}

//#define errorchance 511
#ifndef PLATFORM_XBOX

#endif



typedef TCPoolAggregate<MRTC_Win32AsyncInstance, 128, NThread::CMutualAggregate> CAsyncRequestPool;
DPoolStaticImpl(CAsyncRequestPool, g_AsyncRequestPool);


fint MRTC_SystemInfo::OS_FileAsyncBytesProcessed(void *_pAsyncIntance)
{
	if (g_OsInfo.IsNT())
	{
		return ((MRTC_Win32AsyncInstance*)_pAsyncIntance)->BytesProcessed();
	}
	else
	{
		return 0;
	}
}
void *MRTC_SystemInfo::OS_FileAsyncRead(void *_pFile, void *_pData, fint _DataSize, fint _FileOffset)
{
	M_ASSERT(_DataSize <= 0xffffffff, "Cannot read more that 4 gigs, but we don't have that much memory space anyways");

#ifdef PLATFORM_XBOX

	
		MRTC_Win32AsyncInstance *pNewInstance = g_AsyncRequestPool.New();
	

		LARGE_INTEGER LG;
		LG.QuadPart = _FileOffset;

		CWin32File *pFile = (CWin32File *)_pFile;

		pNewInstance->m_pW32File = pFile;
		pNewInstance->m_pFilen = pFile->m_pFileHandle;
		pNewInstance->m_Overlapped.hEvent = NULL;
		pNewInstance->m_Overlapped.Offset = LG.LowPart;
		pNewInstance->m_Overlapped.OffsetHigh = LG.HighPart;
		pNewInstance->m_bCompleted = false;
		pNewInstance->m_bPending = true;
		pNewInstance->m_bError = false;
		pNewInstance->m_bWrite = false;
		{
			M_LOCK(pFile->m_Lock);
			pFile->m_Requests.Insert(pNewInstance);
		}

		g_OsInfo.QueueRequest(pNewInstance, _DataSize, _pData);

		return pNewInstance;
#else


	if (g_OsInfo.IsNT())
	{
	
		MRTC_Win32AsyncInstance *pNewInstance = g_AsyncRequestPool.New();
	

		LARGE_INTEGER LG;
		LG.QuadPart = _FileOffset;

		CWin32File *pFile = (CWin32File *)_pFile;

		pNewInstance->m_pW32File = pFile;
		pNewInstance->m_pFilen = pFile->m_pFileHandle;
		pNewInstance->m_Overlapped.hEvent = NULL;
		pNewInstance->m_Overlapped.Offset = LG.LowPart;
		pNewInstance->m_Overlapped.OffsetHigh = LG.HighPart;
		pNewInstance->m_bCompleted = false;
		pNewInstance->m_bPending = false;
		pNewInstance->m_bError = false;

		bool Success;
		Success = ReadFile(pFile->m_pFileHandle, _pData, _DataSize, NULL, &pNewInstance->m_Overlapped) != 0;

	#ifdef errorchance
		if ((MRTC_RAND() % errorchance) == 0)
			FileError_static("MRTC_SystemInfo::OS_FileAsyncRead", "File Read Failed", 0);
	#endif
	
		if (!Success) 
		{ 
			// deal with the error code 
			switch (int dwError = GetLastError()) 
			{ 
		        case ERROR_HANDLE_EOF: 
				{
					pNewInstance->m_bCompleted = true;
				} 
				break;
			
	        case ERROR_IO_PENDING: 
				{ 
				}
				break;
			default:
				g_AsyncRequestPool.Delete(pNewInstance);
				FileError_static("MRTC_SystemInfo::OS_FileAsyncRead", "File Read Failed", 0);
				return NULL;
			}
		}
		else
		{
			pNewInstance->m_bCompleted = true;
		}	
	
		return pNewInstance;
	}
	else
	{
		return NULL;
	}
#endif
}

void *MRTC_SystemInfo::OS_FileAsyncWrite(void *_pFile, const void *_pData, fint _DataSize, fint _FileOffset)
{
#ifdef PLATFORM_XBOX

	
		MRTC_Win32AsyncInstance *pNewInstance = g_AsyncRequestPool.New();
	

		LARGE_INTEGER LG;
		LG.QuadPart = _FileOffset;

		CWin32File *pFile = (CWin32File *)_pFile;

		pNewInstance->m_pW32File = pFile;
		pNewInstance->m_pFilen = pFile->m_pFileHandle;
		pNewInstance->m_Overlapped.hEvent = NULL;
		pNewInstance->m_Overlapped.Offset = LG.LowPart;
		pNewInstance->m_Overlapped.OffsetHigh = LG.HighPart;
		pNewInstance->m_bCompleted = false;
		pNewInstance->m_bPending = true;
		pNewInstance->m_bError = false;
		pNewInstance->m_bWrite = true;
		{
			M_LOCK(pFile->m_Lock);
			pFile->m_Requests.Insert(pNewInstance);
		}

		g_OsInfo.QueueRequest(pNewInstance, _DataSize, (void *)_pData);	

		return pNewInstance;
#else

	M_ASSERT(_DataSize <= 0xffffffff, "Cannot read more that 4 gigs, but we don't have that much memory space anyways");

	
	MRTC_Win32AsyncInstance *pNewInstance = g_AsyncRequestPool.New();
	

	if (g_OsInfo.IsNT())
	{
		LARGE_INTEGER LG;
		LG.QuadPart = _FileOffset;

		CWin32File *pFile = (CWin32File *)_pFile;

		pNewInstance->m_pW32File = pFile;
		pNewInstance->m_pFilen = pFile->m_pFileHandle;
		pNewInstance->m_Overlapped.hEvent = NULL;
		pNewInstance->m_Overlapped.Offset = LG.LowPart;
		pNewInstance->m_Overlapped.OffsetHigh = LG.HighPart;
		pNewInstance->m_bCompleted = false;
		pNewInstance->m_bPending = false;
		pNewInstance->m_bError = false;

		bool Success = WriteFile(pFile->m_pFileHandle, _pData, _DataSize, NULL, &pNewInstance->m_Overlapped) != 0;
	
	#ifdef errorchance
		if ((MRTC_RAND() % errorchance) == 0)
			FileError_static("MRTC_SystemInfo::OS_FileAsyncWrite", "File Write Failed", 0);
	#endif

		if (!Success) 
		{ 
			// Deal with the error code 
			switch (int dwError = GetLastError()) 
			{ 
		        case ERROR_HANDLE_EOF: 
				{
					pNewInstance->m_bCompleted = true;
				} 
				break;
			
		        case ERROR_IO_PENDING: 
				{ 
				}
				break;
			default:
				g_AsyncRequestPool.Delete(pNewInstance);
				FileError_static("MRTC_SystemInfo::OS_FileAsyncWrite", "File Write Failed", 0);
				return NULL;
			}
		}
		else
		{
			pNewInstance->m_bCompleted = true;
		}	
	
		return pNewInstance;
	}
	else
	{
		return NULL;
	}
#endif
}

void MRTC_SystemInfo::OS_FileAsyncClose(void *_pAsyncInstance)
{
	if (g_OsInfo.IsNT())
	{
		while (!((MRTC_Win32AsyncInstance*)_pAsyncInstance)->IsCompleted())
		{
			OS_Sleep(1);
		}

		g_AsyncRequestPool.Delete((MRTC_Win32AsyncInstance*)_pAsyncInstance);
	}
}

bool MRTC_SystemInfo::OS_FileAsyncIsFinished(void *_pAsyncInstance)
{
	return ((MRTC_Win32AsyncInstance*)_pAsyncInstance)->IsCompleted();
}

bool MRTC_SystemInfo::OS_FileSetFileSize(const char *_pFileName, fint _FileSize)
{
	if (*_pFileName == 13)
	{
		CWin32File *pFile = *((CWin32File **)(_pFileName + 1));

		if (pFile->m_pFileHandle == INVALID_HANDLE_VALUE)
		{
			FileError_static("MRTC_SystemInfo::OS_FileSetFileSize", "Could not set file size", 0);
			return false;
		}

		LARGE_INTEGER LG;
		LG.QuadPart = _FileSize;
		// Test for failure
		if (SetFilePointer(pFile->m_pFileHandle, LG.LowPart, &LG.HighPart, FILE_BEGIN) == 0xFFFFFFFF 
			&& 
			GetLastError() != NO_ERROR )
		{
			FileError_static("MRTC_SystemInfo::OS_FileSetFileSize", "Could not set file size", 0);
			return false;
		}
	 		
		if (!SetEndOfFile(pFile->m_pFileHandle))
		{
			FileError_static("MRTC_SystemInfo::OS_FileSetFileSize", "Could not set file size", 0);
			return false;
		}
		
		return true;
	}
	else
	{
		HANDLE TempFile = CreateFileA(_pFileName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

		if (TempFile == INVALID_HANDLE_VALUE)
		{
			FileError_static("MRTC_SystemInfo::OS_FileSetFileSize", "Could not set file size", 0);
			return false;
		}

		LARGE_INTEGER LG;
		LG.QuadPart = _FileSize;
		// Test for failure
		if (SetFilePointer(TempFile, LG.LowPart, &LG.HighPart, FILE_BEGIN) == 0xFFFFFFFF 
			&& 
			GetLastError() != NO_ERROR )
		{
			CloseHandle(TempFile);
			FileError_static("MRTC_SystemInfo::OS_FileSetFileSize", "Could not set file size", 0);
			return false;
		}
	 		
		if (!SetEndOfFile(TempFile))
		{
			CloseHandle(TempFile);
			FileError_static("MRTC_SystemInfo::OS_FileSetFileSize", "Could not set file size", 0);
			return false;
		}
		CloseHandle(TempFile);
		
		return true;
	}
}

int MRTC_SystemInfo::OS_FileOperationGranularity(const char *_pPath)
{

#ifdef PLATFORM_XBOX1

	if (_pPath[0] == 'D')
		return 2048;
	else
		return 512;
#elif defined(PLATFORM_XENON)

	return 2048;
//	char TempPath[4];
//	strncpy(TempPath, _pPath, 3);
//	TempPath[3] = 0;
//	return XGetDiskSectorSize(TempPath);
#else
/*	char TempPath[_MAX_PATH];
	strncpy(TempPath, _pPath, _MAX_PATH);

	char *pTemp = strrchr(TempPath, '\\');

	if (pTemp)
		*pTemp = 0;
*/
//	if (g_OsInfo.IsNT())
//	{
	char TempPath[_MAX_PATH];
		GetFullPathNameA(_pPath, _MAX_PATH, TempPath, NULL);
	TempPath[3] = 0;

	DWORD SectorSize = 0;
		GetDiskFreeSpaceA(TempPath, NULL, &SectorSize, NULL, NULL);

	if (!SectorSize)
	{
		SectorSize = 512;
		M_TRACE("WARNING: Could not determine SectorSize for file\r");
	}

	return SectorSize;
//	}
//	else
//		return 1;
#endif

}

void MRTC_ReferenceSymbol(...)
{
	va_list arg;
	va_start(arg, arg);
	ch8 Str[2];
	Str[0] = (mint)arg;
	Str[1] = 0;
	CharLowerA(Str);
}

bool MRTC_SystemInfo::OS_FileGetTime(void *_pFile, int64& _TimeCreate, int64& _TimeAccess, int64& _TimeWrite)
{
	CWin32File *pFile = (CWin32File *)_pFile;

	int Result = GetFileTime(pFile->m_pFileHandle, (LPFILETIME)&_TimeCreate, (LPFILETIME)&_TimeAccess, (LPFILETIME)&_TimeWrite);
	if (!Result)
	{
		FileError_static("MRTC_SystemInfo::OS_FileGetTime", "Could not get file time", 0);
	}
	return Result != 0;
}

bool MRTC_SystemInfo::OS_FileSetTime(void *_pFile, const int64& _TimeCreate, const int64& _TimeAccess, const int64& _TimeWrite)
{
	CWin32File *pFile = (CWin32File *)_pFile;
	int Result = SetFileTime(pFile->m_pFileHandle, (LPFILETIME)&_TimeCreate, (LPFILETIME)&_TimeAccess, (LPFILETIME)&_TimeWrite);
	if (!Result)
	{
		FileError_static("MRTC_SystemInfo::OS_FileSetTime", "Could not set file time", 0);
	}
	return Result != 0;
}

void MRTC_SystemInfo::OS_FileGetDrive(const char *_pFileName, char *_pDriveName)
{
#ifdef PLATFORM_XBOX1
	if (_pFileName[0] == 'd' || _pFileName[0] == 'D' )
	{
		strcpy(_pDriveName, "DVD");
	}
	else
	{
		strcpy(_pDriveName, "HD");
	}
#elif defined PLATFORM_XENON
	strcpy(_pDriveName, "HD");
#else
	char TempPath[_MAX_PATH];
	GetFullPathNameA(_pFileName, _MAX_PATH, TempPath, NULL);
	TempPath[3] = 0;
	strcpy(_pDriveName, TempPath);
#endif

}

bool MRTC_SystemInfo::OS_DirectoryChange(const char* _pPath)
{
#ifdef PLATFORM_XBOX
	Error_static("MRTC_SystemInfo::OS_DirectoryChange", "Unsupported");
	return false;
#else
	return (_chdir(_pPath) == 0);
#endif
}

bool MRTC_SystemInfo::OS_DirectoryCreate(const char* _pPath)
{
#ifdef PLATFORM_XBOX
	return (CreateDirectory(_pPath, NULL) != FALSE);
#else
	return (_mkdir(_pPath) == 0);
#endif
}

char* MRTC_SystemInfo::OS_DirectoryGetCurrent(char* _pBuf, int _MaxLength)
{
#ifdef PLATFORM_XBOX
	Error_static("MRTC_SystemInfo::OS_DirectoryGetCurrent", "Unsupported");
	return NULL;
#else
	return _getcwd(_pBuf, _MaxLength - 1);
#endif
}

bool MRTC_SystemInfo::OS_DirectoryRemove(const char* _pPath)
{
	return (_rmdir(_pPath) == 0);
}

const char* MRTC_SystemInfo::OS_DirectorySeparator()
{
	return "\\";
}

bool MRTC_SystemInfo::OS_FileRemove(const char* _pPath)
{
	return (DeleteFile(_pPath) != FALSE);
}

bool MRTC_SystemInfo::OS_FileRename(const char* _pFrom, const char* _pTo)
{
	return (MoveFile(_pFrom, _pTo) != FALSE);
}

#ifdef CPU_AMD64
extern "C" mint AMD64_GetRBP();
#endif
mint MRTC_SystemInfo::OS_TraceStack(mint *_pCallStack, int _nMaxDepth, mint _ebp)
{
#ifdef PLATFORM_XENON
	DmCaptureStackBackTrace(_nMaxDepth, (void **)_pCallStack);
	int nCap = 0;
	for (int i = 0; i < _nMaxDepth; ++i)
	{
		if (_pCallStack[i] == 0)
			return i;
	}
	return 0;
#else

	mint ThreadTop = g_ThreadTop;
	if (!ThreadTop)
	{
		MEMORY_BASIC_INFORMATION MemoryInfo;
		int Size = sizeof(MemoryInfo);
		memset(&MemoryInfo, 0, Size);
		int nBytes = VirtualQuery(&MemoryInfo, &MemoryInfo, Size);
		if (nBytes != sizeof(MemoryInfo))
		{
			ThreadTop = g_ThreadTop = (mint)&MemoryInfo + 1024*1024; // Just guess (not more that 1 MB)
			int ErrTemp = GetLastError();
		}
		else
		{	
			ThreadTop = g_ThreadTop = (mint)MemoryInfo.BaseAddress + (mint)MemoryInfo.RegionSize;
		}
	}

	mint ThreadBottom = (mint)&ThreadBottom;

	mint *pStack = _pCallStack;
	MRTC_SystemInfoInternal *pData = MRTC_GetSystemInfo().m_pInternalData;

	try
	{
		mint RegFramePtr;
		if (_ebp != 0xffffffff)
			RegFramePtr = _ebp;
		else
		{
#ifdef CPU_AMD64
			RegFramePtr = AMD64_GetRBP();
#else
			__asm
			{
				mov RegFramePtr, ebp
			}
#endif
		}

		RegFramePtr = *((mint *)(RegFramePtr));
		while (_nMaxDepth && RegFramePtr <= ThreadTop && RegFramePtr >= ThreadBottom)
		{
			*pStack = *((mint *)(RegFramePtr + 4));
			if (!(*pStack))
				break;
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
			else if (pData->m_pDebugger)
				pData->m_pDebugger->CheckAddress(*pStack);
#endif
			++pStack;
			RegFramePtr = *((mint *)(RegFramePtr));
			--_nMaxDepth;			
		}
	}
	catch(...)
	{
	}
	return pStack - _pCallStack;
#endif
}

#ifdef OLDREMOTEDEBUGGER
void MRTC_SystemInfo::OS_SendDebugPacket(const void *_pMessageData, int _MessageSize)
{
	MRTC_SystemInfoInternal *pData = MRTC_GetSystemInfo().m_pInternalData;
	if (!pData->m_bNetUp)
		return;
	int Ret = pData->Send(_pMessageData, _MessageSize);
	if (Ret != _MessageSize)
	{
		// Don't try any more
		pData->CloseNet();
	}
}
#endif

void *MRTC_SystemInfo::Semaphore_Alloc(mint _InitialCount, mint _MaximumCount)
{
	HANDLE hSema = CreateSemaphore(NULL, _InitialCount, _MaximumCount, NULL);
//	M_TRACE("SEMA(%x): Create %x\n", GetCurrentThreadId(), hSema);
    return hSema;
}

void MRTC_SystemInfo::Semaphore_Free(void *_pSemaphore)
{
//	CFStr256 Temp;
//	Temp.Format("Closing handle {}\n", DIdsStrFInt(Temp, (mint)_pSemaphore));
//	OutputDebugStringA(Temp);
//	M_TRACE("SEMA(%x): Close %x\n", GetCurrentThreadId(), _pSemaphore);
    CloseHandle(_pSemaphore);
}
		

void MRTC_SystemInfo::Semaphore_Increase(void * _pSemaphore, mint _Count)
{
//	M_TRACE("SEMA(%x): Increase %x with %d\n", GetCurrentThreadId(), _pSemaphore, _Count);
    ReleaseSemaphore(_pSemaphore, _Count, NULL);
}

void MRTC_SystemInfo::Semaphore_Wait(void * _pSemaphore)
{
//	M_TRACE("SEMA(%x): Wait %x\n", GetCurrentThreadId(), _pSemaphore);
    WaitForSingleObject(_pSemaphore, INFINITE);
}

bint MRTC_SystemInfo::Semaphore_WaitTimeout(void * _pSemaphore, fp64 _Timeout)
{
//	M_TRACE("SEMA(%x): Wait timeout %x timeout: %f\n", GetCurrentThreadId(), _pSemaphore, _Timeout);
	if (_Timeout < 0)
		return WaitForSingleObjectEx(_pSemaphore, ((-_Timeout) * 1000.0), true) == WAIT_OBJECT_0;
	else
		return WaitForSingleObject(_pSemaphore, (_Timeout * 1000.0)) == WAIT_OBJECT_0;
}

bint MRTC_SystemInfo::Semaphore_TryWait(void * _pSemaphore)
{
//	M_TRACE("SEMA(%x): TryWait %x\n", GetCurrentThreadId(), _pSemaphore);
    return WaitForSingleObject(_pSemaphore, 0) == WAIT_OBJECT_0;
}


void *MRTC_SystemInfo::Event_Alloc(bint _InitialSignal, bint _bAutoReset)
{
	return CreateEventA(NULL, !_bAutoReset, _InitialSignal, NULL);
}

void MRTC_SystemInfo::Event_Free(void *_pEvent)
{
	CloseHandle(_pEvent);
}

void MRTC_SystemInfo::Event_SetSignaled(void * _pEvent)
{
	SetEvent(_pEvent);
}

void MRTC_SystemInfo::Event_ResetSignaled(void * _pEvent)
{
	ResetEvent(_pEvent);
}

void MRTC_SystemInfo::Event_Wait(void * _pEvent)
{
	WaitForSingleObject(_pEvent, INFINITE);
}

bint MRTC_SystemInfo::Event_WaitTimeout(void * _pEvent, fp64 _Timeout)
{
	if (_Timeout < 0)
	{
		int TimeOut = ((-_Timeout) * 1000.0);
		return WaitForSingleObjectEx(_pEvent, TimeOut, true) == WAIT_OBJECT_0;
	}
	else
	{
		int TimeOut = (_Timeout * 1000.0);
		return WaitForSingleObjectEx(_pEvent, TimeOut, false) == WAIT_OBJECT_0;
	}
}

bint MRTC_SystemInfo::Event_TryWait(void * _pEvent)
{
	return WaitForSingleObject(_pEvent, 0) == WAIT_OBJECT_0;
}


extern "C" LONG __cdecl _InterlockedExchangeAdd(IN OUT LONG volatile *Addend, IN LONG Value);
extern "C" LONG __cdecl _InterlockedCompareExchange(IN OUT LONG volatile *Addend, IN LONG Value, IN LONG _CompareTo);
extern "C" LONG __cdecl _InterlockedExchange(IN OUT LONG volatile *Addend, IN LONG Value);

#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedExchange)


#if !defined(M_RTM) || defined(M_RtmDebug) || defined(M_Profile)
	#ifdef PLATFORM_XENON
		# define CHECK_ADDRESS(_Ptr) if (((mint)_Ptr & 3) || ((mint)_Ptr < 65536)) M_BREAKPOINT_FATAL
	#else
		# define CHECK_ADDRESS(_Ptr) if (((mint)_Ptr & 3) || ((mint)_Ptr < 65536)) M_BREAKPOINT
	#endif
#else
# define CHECK_ADDRESS(_Ptr)
#endif

#ifdef M_SEPARATETYPE_smint

#if defined(PLATFORM_WIN64)

smint MRTC_SystemInfo::Atomic_Increase(volatile smint *_pDest)
{
	CHECK_ADDRESS(_pDest);
	return InterlockedExchangeAdd64((volatile LONG64 *)_pDest, 1);
}

smint MRTC_SystemInfo::Atomic_Decrease(volatile smint *_pDest)
{
	CHECK_ADDRESS(_pDest);
	return InterlockedExchangeAdd64((volatile LONG64 *)_pDest, -1);
}

smint MRTC_SystemInfo::Atomic_Add(volatile smint *_pDest, smint _Add)
{
	CHECK_ADDRESS(_pDest);
	return InterlockedExchangeAdd64((volatile LONG64 *)_pDest, _Add);
}

smint MRTC_SystemInfo::Atomic_Exchange(volatile smint *_pDest, smint _SetTo)
{
	CHECK_ADDRESS(_pDest);
	return InterlockedExchange64((volatile LONG64 *)_pDest, _SetTo);
}

smint MRTC_SystemInfo::Atomic_IfEqualExchange(volatile smint *_pDest, smint _CompareTo, smint _SetTo)
{
	CHECK_ADDRESS(_pDest);
	return InterlockedCompareExchange64((volatile LONG64 *)_pDest, _SetTo, _CompareTo);
}
#else

smint MRTC_SystemInfo::Atomic_Increase(volatile smint *_pDest)
{
	CHECK_ADDRESS(_pDest);
	return _InterlockedExchangeAdd((volatile LONG *)_pDest, 1);
}

smint MRTC_SystemInfo::Atomic_Decrease(volatile smint *_pDest)
{
	CHECK_ADDRESS(_pDest);
	return _InterlockedExchangeAdd((volatile LONG *)_pDest, -1);
}

smint MRTC_SystemInfo::Atomic_Add(volatile smint *_pDest, smint _Add)
{
	CHECK_ADDRESS(_pDest);
	return _InterlockedExchangeAdd((volatile LONG *)_pDest, _Add);
}

smint MRTC_SystemInfo::Atomic_Exchange(volatile smint *_pDest, smint _SetTo)
{
	CHECK_ADDRESS(_pDest);
	return _InterlockedExchange((volatile LONG *)_pDest, _SetTo);
}

smint MRTC_SystemInfo::Atomic_IfEqualExchange(volatile smint *_pDest, smint _CompareTo, smint _SetTo)
{
	CHECK_ADDRESS(_pDest);
	return _InterlockedCompareExchange((volatile LONG *)_pDest, _SetTo, _CompareTo);
}

#endif


#endif


int32 MRTC_SystemInfo::Atomic_Increase(volatile int32 *_pDest)
{
	CHECK_ADDRESS(_pDest);
	return _InterlockedExchangeAdd(_pDest, 1);
}

int32 MRTC_SystemInfo::Atomic_Decrease(volatile int32 *_pDest)
{
	CHECK_ADDRESS(_pDest);
	return _InterlockedExchangeAdd(_pDest, -1);
}

int32 MRTC_SystemInfo::Atomic_Add(volatile int32 *_pDest, int32 _Add)
{
	CHECK_ADDRESS(_pDest);
	return _InterlockedExchangeAdd(_pDest, _Add);
}

int32 MRTC_SystemInfo::Atomic_Exchange(volatile int32 *_pDest, int32 _SetTo)
{
	CHECK_ADDRESS(_pDest);
	return _InterlockedExchange(_pDest, _SetTo);
}

int32 MRTC_SystemInfo::Atomic_IfEqualExchange(volatile int32 *_pDest, int32 _CompareTo, int32 _SetTo)
{
	CHECK_ADDRESS(_pDest);
	return _InterlockedCompareExchange(_pDest, _SetTo, _CompareTo);
}


aint MRTC_SystemInfo::Thread_LocalAlloc()
{
	aint Index = TlsAlloc();

	if (Index == TLS_OUT_OF_INDEXES)
		Error_static(M_FUNCTION, "Out of thread local indices");

	return Index;
}


typedef struct tagTHREADNAME_INFO {
    DWORD dwType;     // Must be 0x1000
    LPCSTR szName;    // Pointer to name (in user address space)
    DWORD dwThreadID; // Thread ID (-1 for caller thread)
    DWORD dwFlags;    // Reserved for future use; must be zero
} THREADNAME_INFO;

void MRTC_SystemInfo::Thread_SetName(const char *_pName)
{
#ifdef M_Profile

#ifdef PLATFORM_XENON
	if (!DmIsDebuggerPresent())
		return;
#endif
    THREADNAME_INFO info;
	
    info.dwType = 0x1000;
    info.szName = _pName;
    info.dwThreadID = GetCurrentThreadId();
    info.dwFlags = 0;
	
    __try
    {
        RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(DWORD), (DWORD *)&info );
    }
    __except( EXCEPTION_CONTINUE_EXECUTION )
    {
    }
#endif
}


void MRTC_SystemInfo::Thread_SetProcessor(uint32 _ThreadID, uint32 _Processor)
{
#ifdef PLATFORM_XENON
	HANDLE hThread = OpenThread(0, 0, _ThreadID);
	XSetThreadProcessor(hThread, _Processor);
	CloseHandle(hThread);
#endif
}

uint32 MRTC_SystemInfo::Thread_GetCurrentID()
{
	return GetCurrentThreadId();
}

void MRTC_SystemInfo::Thread_SetProcessor(uint32 _Processor)
{
#ifdef PLATFORM_XENON
	XSetThreadProcessor(GetCurrentThread(), _Processor);
#endif
}

void MRTC_SystemInfo::Thread_LocalFree(aint _Index)
{
	TlsFree(_Index);
}

void MRTC_SystemInfo::Thread_LocalSetValue(aint _Index, mint _Value)
{
	TlsSetValue(_Index, (void *)_Value);
}

mint MRTC_SystemInfo::Thread_LocalGetValue(aint _Index)
{
	return (mint)TlsGetValue(_Index);
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Network
|__________________________________________________________________________________________________
\*************************************************************************************************/

bint MRTC_SystemInfo::CNetwork::gf_ResolveAddres(const ch8 *_pAddress, NNet::CNetAddressIPv4 &_Address)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_ResolveAddres(_pAddress, _Address);
}

void *MRTC_SystemInfo::CNetwork::gf_Connect(const NNet::CNetAddressTCPv4 &_Address, NThread::CEventAutoResetReportableAggregate *_pReportTo)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_Connect(_Address, _pReportTo);
}

void *MRTC_SystemInfo::CNetwork::gf_Bind(const NNet::CNetAddressUDPv4 &_Address, NThread::CEventAutoResetReportableAggregate *_pReportTo)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_Bind(_Address, _pReportTo);
}

void *MRTC_SystemInfo::CNetwork::gf_Listen(const NNet::CNetAddressTCPv4 &_Address, NThread::CEventAutoResetReportableAggregate *_pReportTo)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_Listen(_Address, _pReportTo);
}

void *MRTC_SystemInfo::CNetwork::gf_Accept(void *_pSocket, NThread::CEventAutoResetReportableAggregate *_pReportTo)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_Accept((MRTC_SystemInfoInternal::CTCPContext::CTCPSocket *)_pSocket, _pReportTo);
}

void MRTC_SystemInfo::CNetwork::gf_SetReportTo(void *_pSocket, NThread::CEventAutoResetReportableAggregate *_pReportTo)
{
	MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_SetReportTo((MRTC_SystemInfoInternal::CTCPContext::CTCPSocket *)_pSocket, _pReportTo);
}

void *MRTC_SystemInfo::CNetwork::gf_InheritHandle(void *_pSocket, NThread::CEventAutoResetReportableAggregate *_pReportTo)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_InheritHandle((MRTC_SystemInfoInternal::CTCPContext::CTCPSocket *)_pSocket, _pReportTo);
}



uint32 MRTC_SystemInfo::CNetwork::gf_GetState(void *_pSocket)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_GetState((MRTC_SystemInfoInternal::CTCPContext::CTCPSocket *)_pSocket);
}

void MRTC_SystemInfo::CNetwork::gf_Close(void *_pSocket)
{
	if (MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_Close((MRTC_SystemInfoInternal::CTCPContext::CTCPSocket *)_pSocket))
	{
		// Destry the ctp context if no more sockets are created
		MRTC_GetSystemInfo().m_pInternalData->DestroyTCPContext();
	}
}

int MRTC_SystemInfo::CNetwork::gf_Receive(void *_pSocket, void *_pData, int _DataLen)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_Receive((MRTC_SystemInfoInternal::CTCPContext::CTCPSocket *)_pSocket, _pData, _DataLen);
}

int MRTC_SystemInfo::CNetwork::gf_Send(void *_pSocket, const void *_pData, int _DataLen)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_Send((MRTC_SystemInfoInternal::CTCPContext::CTCPSocket *)_pSocket, _pData, _DataLen);
}

int MRTC_SystemInfo::CNetwork::gf_Receive(void *_pSocket, NNet::CNetAddressUDPv4 &_Address, void *_pData, int _DataLen)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_Receive((MRTC_SystemInfoInternal::CTCPContext::CTCPSocket *)_pSocket, _Address, _pData, _DataLen);
}

int MRTC_SystemInfo::CNetwork::gf_Send(void *_pSocket, const NNet::CNetAddressUDPv4 &_Address, const void *_pData, int _DataLen)
{
	return MRTC_GetSystemInfo().m_pInternalData->GetTCPContext()->f_Send((MRTC_SystemInfoInternal::CTCPContext::CTCPSocket *)_pSocket, _Address, _pData, _DataLen);
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Heap
|__________________________________________________________________________________________________
\*************************************************************************************************/


#ifdef MRTC_ENABLE_REMOTEDEBUGGER

#ifdef PLATFORM_XBOX1
LPVOID WINAPI XMemAlloc(SIZE_T dwSize, DWORD dwAllocAttributes)
{
	void *pData = XMemAllocDefault(dwSize, dwAllocAttributes);

	XALLOC_ATTRIBUTES Attribs = (*((XALLOC_ATTRIBUTES *)&dwAllocAttributes));

	 //&& Attribs.dwAllocatorId != eXALLOCAllocatorId_XBDM && Attribs.dwAllocatorId != eXALLOCAllocatorId_XBOXKERNEL
	if (pData)
	{
		dwSize = XMemSizeDefault(pData, dwAllocAttributes);

		if (Attribs.dwMemoryType == XALLOC_MEMTYPE_HEAP)
		{
			gf_RDSendHeapAlloc(pData, (dwSize + 7) & (~7), MRTC_GetMemoryManager(), 0);
		}
		else
		{
			if ((mint)pData & 0x80000000)
				gf_RDSendPhysicalAlloc(pData, dwSize, 1, 0);
			else
				gf_RDSendPhysicalAlloc(pData, dwSize, 0, 0);
		}
	}
	
	return pData;
}

VOID WINAPI XMemFree(PVOID pAddress, DWORD dwAllocAttributes)
{
	XALLOC_ATTRIBUTES Attribs = (*((XALLOC_ATTRIBUTES *)&dwAllocAttributes));
	 //&& Attribs.dwAllocatorId != eXALLOCAllocatorId_XBDM && Attribs.dwAllocatorId != eXALLOCAllocatorId_XBOXKERNEL
	if (pAddress)
	{
		if (Attribs.dwMemoryType == XALLOC_MEMTYPE_HEAP)
		{
			gf_RDSendHeapFree(pAddress, MRTC_GetMemoryManager());
		}
		else
		{
			if ((mint)pAddress & 0x80000000)
				gf_RDSendPhysicalFree(pAddress, 1);
			else
				gf_RDSendPhysicalFree(pAddress, 0);
		}
	}

	XMemFreeDefault(pAddress, dwAllocAttributes);
}

SIZE_T WINAPI XMemSize(PVOID pAddress, DWORD dwAllocAttributes)
{
	return XMemSizeDefault(pAddress, dwAllocAttributes);
}

#endif
#endif



int g_LowestFree = 128000000;
#ifdef M_Profile
void UpdateLowestFree()
{
	MEMORYSTATUS Win32MemoryStatus;
	Win32MemoryStatus.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&Win32MemoryStatus);

	if (Win32MemoryStatus.dwAvailPhys < g_LowestFree)
		g_LowestFree = Win32MemoryStatus.dwAvailPhys;
}
#else
M_INLINE void UpdateLowestFree()
{
}
#endif
uint32 MRTC_SystemInfo::OS_PhysicalMemoryLowestFree()
{
	return g_LowestFree;
}


void* MRTC_SystemInfo::OS_AllocGPU(uint32 _Size, bool _bCached)
{
#ifdef PLATFORM_XENON
	return M_ALLOCDEBUGALIGN(_Size, 128, __FILE__, __LINE__);
/*	int Flags = PAGE_READWRITE | MEM_LARGE_PAGES;
	Flags |= MEM_LARGE_PAGES;
	if (!_bCached)
		Flags |= PAGE_WRITECOMBINE;
	void *pData = XPhysicalAlloc(_Size, MAXULONG_PTR, 0, Flags);
	if (pData)
	{
		gf_RDSendPhysicalAlloc(pData, _Size, 0, gf_RDGetSequence(), 0);
	}
	UpdateLowestFree();
#ifdef PLATFORM_XENON
	if (!pData) 
	{
		OS_TraceRaw("Out of memory\n");
		M_BREAKPOINT; // Out of memory
	}
#endif

	return pData;*/
#else
	return OS_Alloc(_Size, 128);
//	return M_ALLOC(_Size);
#endif
}

void MRTC_SystemInfo::OS_FreeGPU(void *_pMem)
{
#ifdef PLATFORM_XENON
	return MRTC_GetMemoryManager()->Free(_pMem);
//	XPhysicalFree(_pMem);
#else
	OS_Free(_pMem);
//	MRTC_GetMemoryManager()->Free(_pMem);
#endif
}


//extern "C" void MmQueryAdressProtect(void *);
#ifdef PLATFORM_XENON
#define TOTAL_SAVE_EXTRA  0
//#define TOTAL_SAVE_EXTRA 8*1024*1024

#define TOTAL_SAVE_XENONPUSHBUFFER 8*1024*1024
#define TOTAL_SAVE_SAFETY 1*1024*1024

class CPhysicalMemoryContext
{
public:

	bint m_bInit;
	NThread::CMutual m_Lock;

	class CAllocator : public CAllocator_Virtual
	{
	public:
		static mint ms_Pos;
		static mint ms_Data[(24*1024) / sizeof(mint)];
		DIdsPInlineS static void *Alloc(mint _Size, mint _Location = 0)
		{
			mint Size = sizeof(ms_Data);
			mint Left = Size - ms_Pos;
			if (_Size > Left)
				M_BREAKPOINT; // Out of memory
			mint Ret = (mint)ms_Data + ms_Pos;
			ms_Pos += _Size;
			return (void *)Ret;
		}

		DIdsPInlineS static void *AllocAlignDebug(mint _Size, mint _Align, mint _Location, const ch8 *_pFile, aint _Line, aint _Flags = 0)
		{
			return Alloc(_Size);
		}

		DIdsPInlineS static void *AllocAlign(mint _Size, mint _Align, mint _Location = 0)
		{
			return Alloc(_Size);
		}
		DIdsPInlineS static void *AllocNoCommit(mint _Size, mint _Location = 0)
		{
			return Alloc(_Size);
		}
		DIdsPInlineS static void Free(void *_pBlock, mint _Size = 0, mint _Location = 0)
		{
		}
		DIdsPInlineS static mint Size(void *_pBlock)
		{
			return 0;
		}
	};

	typedef TCSimpleBlockManager<CAllocator, CPoolType_Growing> CBlockManager;
	CBlockManager m_BlockManagers[4];
	mint m_ValidBuffers;

	void Init()
	{
		if (!m_bInit)
		{
			new(this) CPhysicalMemoryContext;

			MEMORYSTATUS MemoryStatus;
			MemoryStatus.dwLength = sizeof(MEMORYSTATUS);
			GlobalMemoryStatus(&MemoryStatus);

			mint SaveMemory = TOTAL_SAVE_SAFETY + TOTAL_SAVE_XENONPUSHBUFFER + TOTAL_SAVE_EXTRA;

			int64 Alloc16M = AlignDown(MemoryStatus.dwAvailPhys - SaveMemory, 16*1024*1024);
			Alloc16M = MaxMT(0, Alloc16M);

			mint pData = NULL;
			while (Alloc16M > 0)
			{
				pData = (mint)XPhysicalAlloc(Alloc16M, MAXULONG_PTR, 0, PAGE_READWRITE | MEM_16MB_PAGES);//MEM_16MB_PAGES);
				if (pData)
					break;
				Alloc16M -= 16*1024*1024;
			}			

			GlobalMemoryStatus(&MemoryStatus);

			int64 Alloc64K = AlignDown(MemoryStatus.dwAvailPhys - SaveMemory, 64*1024);
			Alloc64K = MaxMT(0, Alloc64K);

			mint pData2 = NULL;
			while (Alloc64K > 0)
			{
				pData2 = (mint)XPhysicalAlloc(Alloc64K, MAXULONG_PTR, 0, PAGE_READWRITE | MEM_LARGE_PAGES);//MEM_16MB_PAGES);
				if (pData2)
					break;
				Alloc64K -= 64*1024;
			}

			GlobalMemoryStatus(&MemoryStatus);


			M_TRACEALWAYS("Found %f MiB of physical memory\n", fp32(Alloc16M+Alloc64K) / (1024.0 * 1024.0));

#ifdef M_Profile
			DM_MEMORY_STATISTICS Stats;
			ZeroMemory(&Stats, sizeof(Stats));
			Stats.cbSize = sizeof(Stats);
			if (DmQueryMemoryStatistics(&Stats))
			{
				M_TRACEALWAYS("Memory Stats:\n");
				
				mint Total = Stats.AvailablePages + Stats.ContiguousPages + Stats.VirtualMappedPages + Stats.ImagePages + Stats.DebuggerPages + Stats.FileCachePages
					+ Stats.StackPages + Stats.VirtualPageTablePages + Stats.SystemPageTablePages + Stats.PoolPages;

				M_TRACEALWAYS("Available           : %f KiB\n", fp32(Stats.AvailablePages * 4 * 1024) / (1024.0));
				M_TRACEALWAYS("Physical            : %f KiB\n", fp32(Stats.ContiguousPages * 4 * 1024) / (1024.0));
				M_TRACEALWAYS("Virtual Mapped      : %f KiB\n", fp32(Stats.VirtualMappedPages * 4 * 1024) / (1024.0));
				M_TRACEALWAYS("Image               : %f KiB\n", fp32(Stats.ImagePages * 4 * 1024) / (1024.0));
				M_TRACEALWAYS("Debugger            : %f KiB\n", fp32(Stats.DebuggerPages * 4 * 1024) / (1024.0));
				M_TRACEALWAYS("File Cache          : %f KiB\n", fp32(Stats.FileCachePages * 4 * 1024) / (1024.0));
				M_TRACEALWAYS("Stack               : %f KiB\n", fp32(Stats.StackPages * 4 * 1024) / (1024.0));
				M_TRACEALWAYS("Virtual Page Tables : %f KiB\n", fp32(Stats.VirtualPageTablePages * 4 * 1024) / (1024.0));
				M_TRACEALWAYS("System Page Tables  : %f KiB\n", fp32(Stats.SystemPageTablePages * 4 * 1024) / (1024.0));
				M_TRACEALWAYS("Pool                : %f KiB\n", fp32(Stats.PoolPages * 4 * 1024) / (1024.0));
				M_TRACEALWAYS("Total               : %f KiB\n", fp32(Total * 4 * 1024) / (1024.0));
			}
#endif
			m_BlockManagers[0].Create("Physical 0", pData, Alloc16M, 128);
			m_BlockManagers[1].Create("Physical 1", pData2, Alloc64K, 128);
			m_ValidBuffers = 2;
			m_bInit = true;
		}
	}

	void Init2()
	{
		DLock(m_Lock);

		MEMORYSTATUS MemoryStatus;
		MemoryStatus.dwLength = sizeof(MEMORYSTATUS);
		GlobalMemoryStatus(&MemoryStatus);

		mint SaveMemory = TOTAL_SAVE_SAFETY + TOTAL_SAVE_EXTRA;

		int64 Alloc64K = AlignDown(MemoryStatus.dwAvailPhys - SaveMemory, 64*1024);
		Alloc64K = MaxMT(0, Alloc64K);
		mint pData2 = NULL;
		while (Alloc64K > 0)
		{
			pData2 = (mint)XPhysicalAlloc(Alloc64K, MAXULONG_PTR, 0, PAGE_READWRITE | MEM_LARGE_PAGES);//MEM_16MB_PAGES);
			if (pData2)
				break;
			Alloc64K -= 64*1024;
		}

		GlobalMemoryStatus(&MemoryStatus);

		int64 Alloc64K2 = AlignDown(MemoryStatus.dwAvailPhys - SaveMemory, 64*1024);
		Alloc64K2 = MaxMT(0, Alloc64K2);

		mint pData3 = NULL;
		while (Alloc64K2 > 0)
		{
			pData3 = (mint)XPhysicalAlloc(Alloc64K2, MAXULONG_PTR, 0, PAGE_READWRITE | MEM_LARGE_PAGES);//MEM_16MB_PAGES);
			if (pData3)
				break;
			Alloc64K2 -= 64*1024;
		}

		GlobalMemoryStatus(&MemoryStatus);

		M_TRACEALWAYS("Found %f MiB of physical memory\n", fp32(Alloc64K+Alloc64K2) / (1024.0 * 1024.0));

#ifdef M_Profile
		DM_MEMORY_STATISTICS Stats;
		ZeroMemory(&Stats, sizeof(Stats));
		Stats.cbSize = sizeof(Stats);
		if (DmQueryMemoryStatistics(&Stats))
		{
			M_TRACEALWAYS("Memory Stats:\n");
			
			mint Total = Stats.AvailablePages + Stats.ContiguousPages + Stats.VirtualMappedPages + Stats.ImagePages + Stats.DebuggerPages + Stats.FileCachePages
				+ Stats.StackPages + Stats.VirtualPageTablePages + Stats.SystemPageTablePages + Stats.PoolPages;

			M_TRACEALWAYS("Available           : %f KiB\n", fp32(Stats.AvailablePages * 4 * 1024) / (1024.0));
			M_TRACEALWAYS("Physical            : %f KiB\n", fp32(Stats.ContiguousPages * 4 * 1024) / (1024.0));
			M_TRACEALWAYS("Virtual Mapped      : %f KiB\n", fp32(Stats.VirtualMappedPages * 4 * 1024) / (1024.0));
			M_TRACEALWAYS("Image               : %f KiB\n", fp32(Stats.ImagePages * 4 * 1024) / (1024.0));
			M_TRACEALWAYS("Debugger            : %f KiB\n", fp32(Stats.DebuggerPages * 4 * 1024) / (1024.0));
			M_TRACEALWAYS("File Cache          : %f KiB\n", fp32(Stats.FileCachePages * 4 * 1024) / (1024.0));
			M_TRACEALWAYS("Stack               : %f KiB\n", fp32(Stats.StackPages * 4 * 1024) / (1024.0));
			M_TRACEALWAYS("Virtual Page Tables : %f KiB\n", fp32(Stats.VirtualPageTablePages * 4 * 1024) / (1024.0));
			M_TRACEALWAYS("System Page Tables  : %f KiB\n", fp32(Stats.SystemPageTablePages * 4 * 1024) / (1024.0));
			M_TRACEALWAYS("Pool                : %f KiB\n", fp32(Stats.PoolPages * 4 * 1024) / (1024.0));
			M_TRACEALWAYS("Total               : %f KiB\n", fp32(Total * 4 * 1024) / (1024.0));
		}
#endif
		m_BlockManagers[2].Create("Physical 2", pData2, Alloc64K, 128);
		m_BlockManagers[3].Create("Physical 3", pData3, Alloc64K2, 128);
		m_ValidBuffers = 4;
	}

	void *Alloc(mint _Size, mint _Alignment)
	{
		Init();
		DLock(m_Lock);
		for (mint i = m_ValidBuffers-1; i >= 0; --i)
		{
			CBlockManager::CBlock *pBlock = m_BlockManagers[i].Alloc(_Size, _Alignment);
			if (pBlock)
				return (void *)(mint)pBlock->GetMemory();
		}
		M_BREAKPOINT; // Out of memory
		return NULL;
	}
	void Free(void *_pMemory)
	{
		DLock(m_Lock);
		for (mint i = 0; i < m_ValidBuffers; ++i)
		{
			mint Memory = (mint)_pMemory;
			if (Memory >= m_BlockManagers[i].m_pBlockStart && Memory < m_BlockManagers[i].m_pBlockEnd)
			{
				CBlockManager::CBlock *pBlock = m_BlockManagers[i].GetBlockFromAddress(Memory);
				m_BlockManagers[i].Free(pBlock);
				return;
			}
		}

		M_BREAKPOINT; // Memory not found
	}
	mint Size(void *_pMemory)
	{
		DLock(m_Lock);
		for (mint i = 0; i < m_ValidBuffers; ++i)
		{
			mint Memory = (mint)_pMemory;
			if (Memory >= m_BlockManagers[i].m_pBlockStart && Memory < m_BlockManagers[i].m_pBlockEnd)
			{
				CBlockManager::CBlock *pBlock = m_BlockManagers[i].GetBlockFromAddress(Memory);
				return m_BlockManagers[i].GetBlockSize(pBlock);
			}
		}
		M_BREAKPOINT; // Memory not found
		return 0;
	}
};
mint g_PhysicalMemoryContext[(sizeof(CPhysicalMemoryContext) + sizeof(mint) - 1) / sizeof(mint)] = {0};
mint CPhysicalMemoryContext::CAllocator::ms_Data[(24*1024) / sizeof(mint)];
mint CPhysicalMemoryContext::CAllocator::ms_Pos = 0;

mint gf_GetFreePhysicalMemory()
{
	CPhysicalMemoryContext *pContext = (CPhysicalMemoryContext *)g_PhysicalMemoryContext;
	pContext->Init();
	DLock(pContext->m_Lock);

	mint FreeMem = 0;
	for (mint i = 0; i < pContext->m_ValidBuffers; ++i)
	{
		FreeMem += pContext->m_BlockManagers[i].GetFreeMemory();
	}
	return FreeMem;
}

mint gf_GetLargestBlockPhysicalMemory()
{
	CPhysicalMemoryContext *pContext = (CPhysicalMemoryContext *)g_PhysicalMemoryContext;
	pContext->Init();
	DLock(pContext->m_Lock);
	mint FreeMem = 0;
	for (mint i = 0; i < pContext->m_ValidBuffers; ++i)
	{
		FreeMem = MaxMT(FreeMem, pContext->m_BlockManagers[i].GetLargestFreeBlock());
	}
	return FreeMem;
}

void gf_GetExtraPhysicalMemory()
{
	CPhysicalMemoryContext *pContext = (CPhysicalMemoryContext *)g_PhysicalMemoryContext;
	pContext->Init2();

	// Alloc more to memory manager
	
	CDA_MemoryManager *pMemMan = MRTC_GetMemoryManager();

	mint ToSave = 128*1024;
	mint FreeMem = gf_GetFreePhysicalMemory();
	while (FreeMem > ToSave)
	{
		mint ToAlloc = AlignDown(gf_GetLargestBlockPhysicalMemory()-4096, 4096);
		ToAlloc = Min(ToAlloc, FreeMem - ToSave);
		pMemMan->InitStatic(ToAlloc);
		FreeMem = gf_GetFreePhysicalMemory();
	}
	
}


#endif

mint MRTC_SystemInfo::OS_MemSize(void *_pBlock)
{
#ifdef PLATFORM_XENON
	CPhysicalMemoryContext *pContext = (CPhysicalMemoryContext *)g_PhysicalMemoryContext;
	return pContext->Size(_pBlock);
#else
	MEMORY_BASIC_INFORMATION MemoryInfo;
	int Size = sizeof(MemoryInfo);
	memset(&MemoryInfo, 0, Size);
	int nBytes = VirtualQuery((void *)_pBlock, &MemoryInfo, Size);
	if (nBytes != sizeof(MemoryInfo))
		ThrowError("VirtualQuery failed");

	return MemoryInfo.RegionSize;
#endif
}

void* MRTC_SystemInfo::OS_Alloc(uint32 _Size, uint32 _Alignment)
{
#ifdef PLATFORM_XENON

	CPhysicalMemoryContext *pContext = (CPhysicalMemoryContext *)g_PhysicalMemoryContext;
	pContext->Init();
	return pContext->Alloc(_Size, _Alignment);
#else
	/*
#if defined(PLATFORM_XENON) && defined(M_Profile) && 0
	void *pData = DmAllocatePool(_Size);
	if (pData)
	{
		gf_RDSendPhysicalAlloc(pData, _Size, 0, gf_RDGetSequence(), 0);
	}
	UpdateLowestFree();
	return pData;
	
#else*/

	int Flags = 0;
#ifdef PLATFORM_XENON
	if (_Size > 16*1024*1024)
	{
		void *pData = XPhysicalAlloc(_Size, MAXULONG_PTR, 0, PAGE_READWRITE | MEM_LARGE_PAGES);//MEM_16MB_PAGES);
//		MmQueryAdressProtect(pData);

		if (pData)
		{
			gf_RDSendPhysicalAlloc(pData, _Size, 0, gf_RDGetSequence(), 0);
		}
		UpdateLowestFree();
#ifdef PLATFORM_XENON
		if (!pData) 
		{
			OS_TraceRaw("Out of memory\n");
			M_BREAKPOINT; // Out of memory
		}
	#endif
		return pData;
	}
	else
	{
		void *pData = XPhysicalAlloc(_Size, MAXULONG_PTR, 0, PAGE_READWRITE);
		if (pData)
		{
			gf_RDSendPhysicalAlloc(pData, _Size, 0, gf_RDGetSequence(), 0);
		}
		UpdateLowestFree();
#ifdef PLATFORM_XENON
		if (!pData) 
		{
			OS_TraceRaw("Out of memory\n");
			M_BREAKPOINT; // Out of memory
		}
	#endif
		return pData;
	}
#else
	{
#ifdef PLATFORM_XENON
		Flags |= MEM_LARGE_PAGES;
#endif
		void *pData = VirtualAlloc(NULL, _Size, MEM_TOP_DOWN | MEM_COMMIT | Flags, PAGE_READWRITE);
		if (pData)
		{
			gf_RDSendPhysicalAlloc(pData, _Size, 0, gf_RDGetSequence(), 0);
		}
		UpdateLowestFree();
#ifdef PLATFORM_XENON
		if (!pData) 
		{
			OS_TraceRaw("Out of memory\n");
			M_BREAKPOINT; // Out of memory
		}
	#endif
		return pData;
	}
#endif
#endif
//#endif
}

void MRTC_SystemInfo::OS_Free(void* _pMem)
{
#ifdef PLATFORM_XENON
	CPhysicalMemoryContext *pContext = (CPhysicalMemoryContext *)g_PhysicalMemoryContext;
	return pContext->Free(_pMem);
#else
	/*
#if defined(PLATFORM_XENON) && defined(M_Profile) && 0
	if (_pMem)
	{
		gf_RDSendPhysicalFree(_pMem, gf_RDGetSequence(), 0);
	}
	DmFreePool(_pMem);
#else*/
	if (_pMem)
	{
		gf_RDSendPhysicalFree(_pMem, gf_RDGetSequence(), 0);
	}
#ifdef PLATFORM_XENON
	XPhysicalFree(_pMem);
#else
	VirtualFree(_pMem, 0, MEM_RELEASE);
#endif
#endif
//#endif
}

bool MRTC_SystemInfo::OS_Commit(void *_pMem, uint32 _Size, bool _bCommited)
{
	return VirtualAlloc(_pMem, _Size, _bCommited ? MEM_COMMIT : MEM_DECOMMIT, PAGE_READWRITE) == 0;
}

uint32 MRTC_SystemInfo::OS_CommitGranularity()
{
	return 4096;
}

#ifdef PLATFORM_XBOX

//void *g_hDefaultHeap = NULL;

void* MRTC_SystemInfo::OS_HeapAlloc(uint32 _Size)
{
	void *pData = LocalAlloc(0, _Size);
	UpdateLowestFree();
	return pData;
}

void* MRTC_SystemInfo::OS_HeapAllocAlign(uint32 _Size, uint32 _Align)
{
	M_ASSERT(_Align <= 32, "");
	void *pData = LocalAlloc(0, _Size);
	UpdateLowestFree();
	return pData;
}

void* MRTC_SystemInfo::OS_HeapRealloc(void *_pMem, uint32 _Size)
{	
	if(!_pMem)
		return OS_HeapAlloc(_Size);

	void *pData = LocalReAlloc(_pMem, _Size, LMEM_MOVEABLE);

	UpdateLowestFree();
	return pData;	
}

void* MRTC_SystemInfo::OS_HeapReallocAlign(void *_pMem, uint32 _Size, uint32 _Align)
{	
	M_ASSERT(_Align <= 32, "");
	if(!_pMem)
		return OS_HeapAlloc(_Size);

	void *pData = LocalReAlloc(_pMem, _Size, LMEM_MOVEABLE);

	UpdateLowestFree();
	return pData;	
}

//BOOL WINAPI RtlFreeHeap(IN HANDLE hHeap,IN DWORD dwFlags,IN LPVOID lpMem);	

void MRTC_SystemInfo::OS_HeapFree(void *_pMem)
{
	LocalFree(_pMem);	
}


uint32 MRTC_SystemInfo::OS_HeapSize(const void *_pMem)
{
	return LocalSize((HLOCAL)_pMem);	
}

uint32 MRTC_SystemInfo::OS_PhysicalMemoryFree()
{
	MEMORYSTATUS Win32MemoryStatus;
	Win32MemoryStatus.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&Win32MemoryStatus);

	return Win32MemoryStatus.dwAvailPhys;
}

uint32 MRTC_SystemInfo::OS_PhysicalMemorySize()
{
	MEMORYSTATUS Win32MemoryStatus;
	Win32MemoryStatus.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&Win32MemoryStatus);

	return Win32MemoryStatus.dwTotalPhys;
}

uint32 MRTC_SystemInfo::OS_PhysicalMemoryUsed()
{
	MEMORYSTATUS Win32MemoryStatus;
	Win32MemoryStatus.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&Win32MemoryStatus);

	return Win32MemoryStatus.dwTotalPhys - Win32MemoryStatus.dwAvailPhys;
}
uint32 MRTC_SystemInfo::OS_PhysicalMemoryLargestFree()
{
	MEMORYSTATUS Win32MemoryStatus;
	Win32MemoryStatus.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&Win32MemoryStatus);

	return Win32MemoryStatus.dwAvailPhys;
}

#elif defined(PLATFORM_WIN_PC)

// DLMalloc has buuuuugs under xp toooo bad
// Installed dlmalloc version found in cygwin
//#define USE_DLMALLOC

#ifdef USE_DLMALLOC

#define USE_DL_PREFIX        1
#define __STD_C 1
#define LACKS_UNISTD_H
#define malloc_getpagesize 4096
#pragma warning(push)
//#pragma warning(disable:4308)
//#pragma warning(disable:4307)
extern "C" static struct malloc_state *GetAV();
extern "C"
{
	#include "dlmallocpc.c"
}
#pragma warning(pop)

class CDefaultMemoryManager
{
public:
	CDefaultMemoryManager()
	{
		m_GVGM.Init();
		m_GV.Init();
	}

	MRTC_CriticalSection m_Lock;
	DLM_GlobalVars_Gm m_GVGM;
	DLM_GlobalVars m_GV;
};

static uint8 gs_DLMemoryManager[sizeof(CDefaultMemoryManager)];
static CDefaultMemoryManager *gs_pDLMemoryManager = NULL;

M_INLINE static CDefaultMemoryManager *GetDefaultManager()
{
	if (gs_pDLMemoryManager)
		return gs_pDLMemoryManager;
	if (MRTC_GetMemoryManager()->m_pDefaultManager)
	{
		gs_pDLMemoryManager = MRTC_GetMemoryManager()->m_pDefaultManager;
		return gs_pDLMemoryManager;
	}
	
	MRTC_GetMemoryManager()->m_pDefaultManager = new(gs_DLMemoryManager) CDefaultMemoryManager;
	

	gs_pDLMemoryManager = MRTC_GetMemoryManager()->m_pDefaultManager;
	return gs_pDLMemoryManager;
}


extern "C" struct DLM_GlobalVars_Gm *GetGlobalVars_GM()
{
	return &GetDefaultManager()->m_GVGM;
}

extern "C" struct DLM_GlobalVars *GetGlobalVars()
{
	return &GetDefaultManager()->m_GV;
}

#endif

#ifdef M_FINDNASTYACCESSESTODELETEDMEMORY


void* MRTC_SystemInfo::OS_HeapAlloc(uint32 _Size)
{
	MRTC_SystemInfoInternal *pData = MRTC_GetSystemInfo().m_pInternalData;

	return pData->m_VirtualHeap.Alloc(_Size);
}


void* MRTC_SystemInfo::OS_HeapAllocAlign(uint32 _Size, uint32 _Align)
{
	MRTC_SystemInfoInternal *pData = MRTC_GetSystemInfo().m_pInternalData;

	return pData->m_VirtualHeap.Alloc(_Size);
}
void* MRTC_SystemInfo::OS_HeapRealloc(void *_pMem, uint32 _Size)
{	
	MRTC_SystemInfoInternal *pData = MRTC_GetSystemInfo().m_pInternalData;
	

	uint32 Size = OS_HeapSize(_pMem);
	void *pNewBlock = pData->m_VirtualHeap.Alloc(_Size);
	memcpy(pNewBlock, _pMem, Min(Size, _Size));
	pData->m_VirtualHeap.Free(_pMem);
	return pNewBlock;
}

//BOOL WINAPI RtlFreeHeap(IN HANDLE hHeap,IN DWORD dwFlags,IN LPVOID lpMem);	

void MRTC_SystemInfo::OS_HeapFree(void *_pMem)
{
	MRTC_SystemInfoInternal *pData = MRTC_GetSystemInfo().m_pInternalData;
	pData->m_VirtualHeap.Free(_pMem);
}

uint32 MRTC_SystemInfo::OS_HeapSize(const void *_pMem)
{
	MRTC_SystemInfoInternal *pData = MRTC_GetSystemInfo().m_pInternalData;
	return pData->m_VirtualHeap.MemSize(_pMem);
}


#else

#define MRTC_ALIGNEDHEAPALLOC

// -------------------------------------------------------------------
// Aligned win32 heap allocation
// -------------------------------------------------------------------
#ifdef MRTC_ALIGNEDHEAPALLOC

static void* AlignedHeapAlloc(mint _Size, mint _Align)
{
	_Size = (_Size+3) & ~3;

	int Align = 4;
	if (!(_Size & 0x0f))
		Align = 16;
	else if (!(_Size & 0x07))
		Align = 8;

	Align = Max(Align, (int)_Align);
	M_ASSERT(Align < 256, "!");
	uint32 AlignSize = _Size + Max(4, Align);

	void *pMem = HeapAlloc(GetProcessHeap(), 0, AlignSize);
	if (!pMem) 
	{
#ifdef PLATFORM_CONSOLE
		MRTC_SystemInfo::OS_TraceRaw("Out of memory\n");
		M_BREAKPOINT; // Out of memory
#endif
		return NULL;
	}

	if (mint(pMem) & 0x03)
		__asm int 3;

	mint pAddr = (mint) pMem;
	mint pAddrAligned = (pAddr+4+Align-1) & ~(Align-1);
	uint32 AlignDelta = uint32(pAddrAligned - pAddr);
	M_ASSERT(AlignDelta <= AlignSize, "!");
	M_ASSERT(pAddrAligned+_Size <= pAddr+AlignSize , "!");

	uint32* pDelta = (uint32*)(pAddrAligned-4);
	*pDelta = AlignDelta + 0xafafaf00;
	MemSetD(pDelta-((AlignDelta-4) >> 2), 0x77777777, ((AlignDelta-4) >> 2));

	void* pData = (void*)pAddrAligned;

//	M_TRACE("HeapAlloc %.8x, Aligned %.8x, Alloc Ptr %.8x, Returned %.8x, Delta %.4x\n", _Size, AlignSize, pMem, pData, AlignDelta);
	return pData;
}

static void AlignedHeapFree(void* _pMem)
{
	uint32 AlignDelta = *(((uint32*)_pMem)-1);
	if ((AlignDelta & 0xffffff00) != 0xafafaf00)
	{
		M_TRACE("(OS_HeapFree) Corrupt block pointer %.8x.\n", _pMem);
		M_BREAKPOINT;
	}
	AlignDelta &= 0xff;

	if (AlignDelta > 16)
	{
		M_TRACE("(OS_HeapFree) Corrupt block alignment %d, %.8x.\n", AlignDelta, _pMem);
		M_BREAKPOINT;
	}

	void* pDelete = (void*)(mint(_pMem) - AlignDelta);

//	M_TRACE("HeapFree Alloc Ptr %.8x, Returned %.8x, Delta %.4x\n", pDelete, _pMem, AlignDelta);

	HeapFree(GetProcessHeap(), 0, pDelete);
}

static mint AlignedHeapSize(const void* _pMem)
{
	uint32 AlignDelta = *(((uint32*)_pMem)-1);
	if ((AlignDelta & 0xffffff00) != 0xafafaf00)
	{
		M_TRACE("(OS_HeapSize) Corrupt block pointer %.8x.\n", _pMem);
		M_BREAKPOINT;
	}
	AlignDelta &= 0xff;

	if (AlignDelta > 16)
	{
		MRTC_SystemInfo::OS_TraceRaw("(OS_HeapSize) Invalid block alignment.\n");
		M_BREAKPOINT;
	}
	void* pMem = (void*)(mint(_pMem) - AlignDelta);

//	M_TRACE("HeapSize Alloc Ptr %.8x, Returned %.8x, Delta %.4x\n", pMem, _pMem, AlignDelta);

	mint size = HeapSize(GetProcessHeap(), 0, pMem);
	size -= AlignDelta;

//	M_TRACE("Size %.6x, Returned %.6x\n", size+AlignDelta, size);
	return size;
}

static void* AlignedHeapRealloc(void* _pMem, mint _Size, mint _Align)
{
/*	if (_Align == 4)
	{
		
	}
	else*/
	{
		void* pData = AlignedHeapAlloc(_Size, _Align);
		if (!pData) 
		{
#ifdef PLATFORM_CONSOLE
			MRTC_SystemInfo::OS_TraceRaw("Out of memory\n");
			M_BREAKPOINT; // Out of memory
#endif
			return NULL;
		}


		mint OldSize = MRTC_SystemInfo::OS_HeapSize(_pMem);
		mint CopySize = Min(OldSize, _Size);
		memcpy(pData, _pMem, CopySize);

		AlignedHeapFree(_pMem);
		return pData;
	}
}

#endif

// -------------------------------------------------------------------
void* MRTC_SystemInfo::OS_HeapAlloc(uint32 _Size)
{
#ifdef USE_DLMALLOC
	M_LOCK(GetDefaultManager()->m_Lock);
	return dlmalloc(_Size);
#else

#ifdef MRTC_ALIGNEDHEAPALLOC
	void* pData = AlignedHeapAlloc(_Size, 4);
#else
	void *pData = HeapAlloc(GetProcessHeap(), 0, _Size);

#endif

#ifdef PLATFORM_CONSOLE
	if (!pData) 
	{
		OS_TraceRaw("Out of memory\n");
		M_BREAKPOINT; // Out of memory
	}
#endif
	return pData;
#endif
}


void* MRTC_SystemInfo::OS_HeapAllocAlign(uint32 _Size, uint32 _Align)
{
#ifdef USE_DLMALLOC
	M_LOCK(GetDefaultManager()->m_Lock);
	return dlmemalign(_Align, _Size);
#else
#ifdef PLATFORM_WIN
	// Well, well, well... orka
	if (_Align > 16)
		_Align = 16;
#endif
	M_ASSERT(_Align <= 16, "!");

#ifdef MRTC_ALIGNEDHEAPALLOC
	void* pData = AlignedHeapAlloc(_Size, _Align);

#else
	void *pData = HeapAlloc(GetProcessHeap(), 0, _Size);

#endif

#ifdef PLATFORM_CONSOLE
	if (!pData) 
	{
		OS_TraceRaw("Out of memory\n");
		M_BREAKPOINT; // Out of memory
	}
#endif
	return pData;
#endif
}
void* MRTC_SystemInfo::OS_HeapRealloc(void *_pMem, uint32 _Size)
{	
#ifdef USE_DLMALLOC
	M_LOCK(GetDefaultManager()->m_Lock);
	return dlrealloc(_pMem, _Size);
#else
	if(!_pMem)
		return OS_HeapAlloc(_Size);

#ifdef MRTC_ALIGNEDHEAPALLOC
	void *pData = AlignedHeapRealloc(_pMem, _Size, 4);
	
#else

	void *pData = HeapReAlloc(GetProcessHeap(), 0, _pMem, _Size);
#endif

#ifdef PLATFORM_CONSOLE
	if (!pData) 
	{
		OS_TraceRaw("Out of memory\n");
		M_BREAKPOINT; // Out of memory
	}
#endif
	return pData;	
#endif
}

void* MRTC_SystemInfo::OS_HeapReallocAlign(void *_pMem, uint32 _Size, uint32 _Align)
{	
#ifdef USE_DLMALLOC
	M_LOCK(GetDefaultManager()->m_Lock);
	return dlrealloc(_pMem, _Size);
#else
	if(!_pMem)
		return OS_HeapAlloc(_Size);

#ifdef MRTC_ALIGNEDHEAPALLOC
	void *pData = AlignedHeapRealloc(_pMem, _Size, _Align);
	
#else

	void *pData = HeapReAlloc(GetProcessHeap(), 0, _pMem, _Size);
#endif

#ifdef PLATFORM_CONSOLE
	if (!pData) 
	{
		OS_TraceRaw("Out of memory\n");
		M_BREAKPOINT; // Out of memory
	}
#endif
	return pData;	
#endif
}

//BOOL WINAPI RtlFreeHeap(IN HANDLE hHeap,IN DWORD dwFlags,IN LPVOID lpMem);	

void MRTC_SystemInfo::OS_HeapFree(void *_pMem)
{
	if (!_pMem)
		return;

#ifdef USE_DLMALLOC
	M_LOCK(GetDefaultManager()->m_Lock);
	dlfree(_pMem);
#else

#ifdef MRTC_ALIGNEDHEAPALLOC
	AlignedHeapFree(_pMem);

#else

	HeapFree(GetProcessHeap(), 0, _pMem);
#endif

#endif
}

uint32 MRTC_SystemInfo::OS_HeapSize(const void *_pMem)
{
	if (!_pMem)
		return 0;

#ifdef USE_DLMALLOC
	M_LOCK(GetDefaultManager()->m_Lock);
	return malloc_usable_size((void*)_pMem);
#else

#ifdef MRTC_ALIGNEDHEAPALLOC
	return AlignedHeapSize(_pMem);

#else

	return HeapSize(GetProcessHeap(), 0, _pMem);
#endif

#endif
}
#endif

uint32 MRTC_SystemInfo::OS_PhysicalMemoryUsed()
{
	MEMORYSTATUS Win32MemoryStatus;
	Win32MemoryStatus.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&Win32MemoryStatus);

	return Win32MemoryStatus.dwTotalPhys - Win32MemoryStatus.dwAvailPhys;
}

uint32 MRTC_SystemInfo::OS_PhysicalMemoryFree()
{
	MEMORYSTATUS Win32MemoryStatus;
	Win32MemoryStatus.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&Win32MemoryStatus);

	return Win32MemoryStatus.dwAvailPhys;
}

uint32 MRTC_SystemInfo::OS_PhysicalMemorySize()
{
	MEMORYSTATUS Win32MemoryStatus;
	Win32MemoryStatus.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&Win32MemoryStatus);

	return Win32MemoryStatus.dwTotalPhys;
}

uint32 MRTC_SystemInfo::OS_PhysicalMemoryLargestFree()
{
	MEMORYSTATUS Win32MemoryStatus;
	Win32MemoryStatus.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&Win32MemoryStatus);

	return Win32MemoryStatus.dwAvailPhys;
}

#endif

#ifdef PLATFORM_WIN_PC
void gf_SetLogFileName(CStr _FileName)
{
	MRTC_SystemInfoInternal *pLocalSys = MRTC_SystemInfo::MRTC_GetSystemInfo().m_pInternalData;
	if (pLocalSys->m_LogFileName == "")
		pLocalSys->m_LogFileName = _FileName;
}
#endif

///////////////////////////////////////////////////////
/**
 * @file   excep.cpp
 * @author Sammartino <ryan.sammartino@vugames.com>
 * @date   Thu Feb 19 14:39:42 2004
 *
 * @brief  An exception handler for Xbox.
 *
 * Call excep_init to initialise the exception handler.  When a
 * crash occurs, a file will be written to the tite's T: drive
 * called "crash_log_YYYY_MM_DD_HH_MM_SS.txt", where YYYYY MM DD HH
 * MM SS is the date in ISO format.  This file can then be used to
 * find the source of the crash.
 *
 * Note that the stack information may not be very useful in Release
 * builds.  If you really want good stack info, you need to compile
 * with the /Oy- flag.
 */
#if defined(ENABLE_STACKTRACING)

#define NL "\015\012"


#if CPU_BITS > 32
#define CPU_FORMATHEX CPU_HEXES_STR "I64"
#else
#define CPU_FORMATHEX CPU_HEXES_STR
#endif
template <typename t_CInt>
DIdsPInlineS t_CInt ByteSwap(t_CInt _In)
{
	t_CInt Temp;
	uint8 *pTemp = (uint8 *)&Temp;
	uint8 *pIn = (uint8 *)&_In;
	const int Size = sizeof(t_CInt);
	for (int i = 0; i < Size; ++i)
	{
		pTemp[Size - i - 1] = pIn[i];
	}
	return Temp;
}

CStr FormatStr(const char *_pStr, ...)
{
	CStr Ret;

	int nSize = 0;
	va_list args;
	va_start(args, _pStr);

	_vsnprintf( Ret.GetBuffer(2048), 2048, _pStr, args);

	return Ret;		
}


CStr GetProtectionFlags(DWORD Protect)
{
	CStr Temp;

	if (Protect & PAGE_EXECUTE)
		Temp = Temp + (Temp.Len() ? ", Execute" : "Execute");

	if (Protect & PAGE_EXECUTE_READ)
		Temp = Temp + (Temp.Len() ? ", ExecuteRead" : "ExecuteRead");

	if (Protect & PAGE_EXECUTE_READWRITE)
		Temp = Temp + (Temp.Len() ? ", ExecuteReadWrite" : "ExecuteReadWrite");

	if (Protect & PAGE_EXECUTE_WRITECOPY)
		Temp = Temp + (Temp.Len() ? ", ExecuteWriteCopy" : "ExecuteWriteCopy");

	if (Protect & PAGE_NOACCESS)
		Temp = Temp + (Temp.Len() ? ", NoAccess" : "NoAccess");

	if (Protect & PAGE_READONLY)
		Temp = Temp + (Temp.Len() ? ", Read" : "Read");

	if (Protect & PAGE_READWRITE)
		Temp = Temp + (Temp.Len() ? ", ReadWrite" : "ReadWrite");

	if (Protect & PAGE_WRITECOPY)
		Temp = Temp + (Temp.Len() ? ", WriteCopy" : "WriteCopy");

	if (Protect & PAGE_GUARD)
		Temp = Temp + (Temp.Len() ? ", Guard" : "Guard");

	if (Protect & PAGE_NOCACHE)
		Temp = Temp + (Temp.Len() ? ", NoCache" : "NoCache");

	if (Protect & PAGE_WRITECOMBINE)
		Temp = Temp + (Temp.Len() ? ", WriteCombine" : "WriteCombine");
	
	return Temp;
}

CStr GetMemoryInfoStr(const MEMORY_BASIC_INFORMATION &_MemoryInfo)
{
	CStr Temp;
	Temp += FormatStr("   Addr: 0x%0"CPU_FORMATHEX"x"" Size: 0x%0"CPU_FORMATHEX"x" " Protect: %s" NL, (mint)_MemoryInfo.BaseAddress, (mint)_MemoryInfo.RegionSize, GetProtectionFlags(_MemoryInfo.Protect).Str());

	return Temp;
}

namespace NTemp
{
	typedef struct _IMAGEHLP_MODULE64 {  DWORD SizeOfStruct;  DWORD64 BaseOfImage;  DWORD ImageSize;  DWORD TimeDateStamp;  DWORD CheckSum;  DWORD NumSyms;  SYM_TYPE SymType;  TCHAR ModuleName[32];  TCHAR ImageName[256];  TCHAR LoadedImageName[256];  TCHAR LoadedPdbName[256];  DWORD CVSig;  TCHAR CVData[MAX_PATH*3];  DWORD PdbSig;  GUID PdbSig70;  DWORD PdbAge;  BOOL PdbUnmatched;  BOOL DbgUnmatched;  BOOL LineNumbers;  BOOL GlobalSymbols;  BOOL TypeInfo;  BOOL SourceIndexed;  BOOL Publics;
	} IMAGEHLP_MODULE64, *PIMAGEHLP_MODULE64;
}

class CEnumSymbolsContext
{
public:
	CStr m_String;
	TArray<mint> m_Addresses;
};

BOOL CALLBACK EnumModulesCallback(PSTR ModuleName,DWORD64 ModuleBase,ULONG ModuleSize,PVOID UserContext)
{
	CEnumSymbolsContext *pContext = (CEnumSymbolsContext *)UserContext;
	CStr *pStr = &(pContext->m_String);

	*pStr += FormatStr("%s Base: 0x%0"CPU_FORMATHEX"x"" Size: 0x%0"CPU_FORMATHEX"x" NL, ModuleName, (mint)ModuleBase, (mint)ModuleSize);

	MRTC_SystemInfoInternal *pLocalSys = MRTC_SystemInfo::MRTC_GetSystemInfo().m_pInternalData;
	CStackTraceContext *pStackTracer = pLocalSys->GetStackTraceContext();

	// Find date timestamp
	{
		IMAGE_NT_HEADERS *pHeader = NULL;
		uint32 *pData = (uint32 *)ModuleBase;
		uint32 Find = 'P' + ('E' << 8) + ('\0' << 16) + ('\0' << 24);
		for (int i = 0; i < ModuleSize; i += 4, pData += 1)
		{				
			if (*pData == Find)
			{
				pHeader = (IMAGE_NT_HEADERS *)pData;
				break;
			}
		}

		if (pHeader)
		{
			SYSTEMTIME SysTime1970;
			SysTime1970.wYear = 1970;
			SysTime1970.wMonth = 1;
			SysTime1970.wDay = 1;
			SysTime1970.wHour = 0;
			SysTime1970.wMinute = 0;
			SysTime1970.wSecond = 0;
			SysTime1970.wMilliseconds = 0;

			FILETIME Time;

			SystemTimeToFileTime(&SysTime1970, &Time);

			((uint64 &)Time) += (uint64)pHeader->FileHeader.TimeDateStamp * 10000000i64;
			FILETIME Local;
			FileTimeToLocalFileTime(&Time, &Local);
			SYSTEMTIME SysTime;
			FileTimeToSystemTime(&Local, &SysTime);
			*pStr += FormatStr("File Time Stamp: %d-%02d-%02d %02d:%02d:%02d" NL, SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute, SysTime.wSecond);
		}		
	}

	bool bTrySymbol = true;

	pContext->m_Addresses.Add(ModuleBase);

	if (bTrySymbol)
	{
		NTemp::IMAGEHLP_MODULE64 Info;
		memset(&Info, 0, sizeof(Info));
		Info.SizeOfStruct = sizeof(Info);

		if (pStackTracer->SymGetModuleInfo64(pStackTracer->m_hProcess, ModuleBase, (IMAGEHLP_MODULE64 *)&Info))
		{
			*pStr += FormatStr("Loaded Image: %s" NL, Info.LoadedImageName);
			*pStr += FormatStr("Loaded Pdb: %s" NL, Info.LoadedPdbName);
			bTrySymbol = false;
		}
		else
		{
			HRESULT Err = GetLastError();
			int i = 0;
		}
	}

	// Walk module memory space

	mint MemoryWalk = ModuleBase;
	MEMORY_BASIC_INFORMATION MemoryInfo;
	int Size = sizeof(MemoryInfo);
	memset(&MemoryInfo, 0, Size);
	int nBytes = VirtualQuery((void *)MemoryWalk, &MemoryInfo, Size);
	while (nBytes == sizeof(MemoryInfo) && MemoryInfo.AllocationBase == (void *)ModuleBase)
	{
		*pStr += GetMemoryInfoStr(MemoryInfo);

		MemoryWalk += MemoryInfo.RegionSize;
		nBytes = VirtualQuery((void *)MemoryWalk, &MemoryInfo, Size);
	}

	return true;
}


class CCrashDialog : public MRTC_Thread
{
public:

	static CCrashDialog *ms_pThis;
	CCrashDialog()
	{
		ms_pThis = this;
	}

	CStr m_Message;
	CStr m_Title;

	HFONT f_CreatePointFontIndirect(const LOGFONTW* lpLogFont)
	{
		HDC hDC;
		hDC = ::GetDC(NULL);

		// convert nPointSize to logical units based on pDC
		LOGFONTW logFont = *lpLogFont;
		POINT pt;
		pt.y = ::GetDeviceCaps(hDC, LOGPIXELSY) * logFont.lfHeight;
		pt.y /= 720;    // 72 points/inch, 10 decipoints/point
		pt.x = 0;
		::DPtoLP(hDC, &pt, 1);
		POINT ptOrg = { 0, 0 };
		::DPtoLP(hDC, &ptOrg, 1);
		logFont.lfHeight = -Abs(pt.y - ptOrg.y);

		ReleaseDC(NULL, hDC);

		return CreateFontIndirectW(&logFont);
	}

	HFONT f_CreatePointFont(int nPointSize, const wchar_t *lpszFaceName)
	{

		LOGFONTW logFont;
		memset(&logFont, 0, sizeof(LOGFONT));
		logFont.lfCharSet = DEFAULT_CHARSET;
		logFont.lfHeight = nPointSize;
		NStr::StrCopy(logFont.lfFaceName, lpszFaceName);

		return f_CreatePointFontIndirect(&logFont);
	}

	enum
	{
#undef ID_TEXT
#undef ID_EDIT
#undef IDB_Yes
#undef IDB_No
		ID_TEXT = 200,
		ID_EDIT = 201,
		IDB_Yes,
		IDB_No,
	};

	HFONT m_NormalFont;

	static INT_PTR CALLBACK msp_HandleMessages(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
		case WM_SYSCOMMAND:
			{
				switch (wParam)
				{
				case SC_CLOSE:

					HWND hTmp = GetDlgItem(hwndDlg, ID_EDIT);
					GetWindowText(hTmp, ms_pThis->m_EditData.GetBuffer(8192), 8192);
					EndDialog(hwndDlg, 0);
					return 1;
				};
			}
			break;
		case WM_TIMER:
			{
				return 1;
			}
			break;
		case WM_INITDIALOG:
			{

				ms_pThis->m_hWnd = hwndDlg;
				
				POINT Pnt;
				GetCursorPos(&Pnt);
				HMONITOR hMon = MonitorFromPoint(Pnt, MONITOR_DEFAULTTONEAREST);
				MONITORINFO MonInfo;
				memset(&MonInfo, 0, sizeof(MonInfo));
				MonInfo.cbSize = sizeof(MonInfo);
				GetMonitorInfo(hMon, &MonInfo);

				RECT Rect;
				GetWindowRect(hwndDlg, &Rect);

				int Width = Rect.right - Rect.left;
				int Height = Rect.bottom - Rect.top;
				int MonWidth = MonInfo.rcWork.right - MonInfo.rcWork.left;
				int MonHeight = MonInfo.rcWork.bottom - MonInfo.rcWork.top;

				SendMessage(hwndDlg, WM_SETFONT, (WPARAM)ms_pThis->m_NormalFont, 0);
				HWND hTmp;

				hTmp = GetDlgItem(hwndDlg, ID_TEXT);
				SendMessage(hTmp, WM_SETFONT, (WPARAM)ms_pThis->m_NormalFont, 0);

				if (!ms_pThis->m_bOnlyMessage)
				{
					hTmp = GetDlgItem(hwndDlg, IDB_Yes);
					SendMessage(hTmp, WM_SETFONT, (WPARAM)ms_pThis->m_NormalFont, 0);
					hTmp = GetDlgItem(hwndDlg, IDB_No);
					SendMessage(hTmp, WM_SETFONT, (WPARAM)ms_pThis->m_NormalFont, 0);
					hTmp = GetDlgItem(hwndDlg, ID_EDIT);
					SendMessage(hTmp, WM_SETFONT, (WPARAM)ms_pThis->m_NormalFont, 0);
				}
				
//					SendDlgItemMessage(hwndDlg, IDB_Yes, BM_SETSTYLE, BS_PUSHBUTTON, (LONG)TRUE);
//					SendDlgItemMessage(hwndDlg, IDB_No, BM_SETSTYLE, BS_DEFPUSHBUTTON, (LONG)TRUE);
				if (!ms_pThis->m_bOnlyMessage)
				{
					SendMessage(hwndDlg, DM_SETDEFID, IDB_Yes,0L);
				}

				SetWindowPos(hwndDlg, NULL, MonInfo.rcWork.left + MonWidth / 2 - Width / 2, MonInfo.rcWork.top + MonHeight / 2 - Height / 2, 0, 0, SWP_NOSIZE|SWP_NOZORDER);

				SetTimer(hwndDlg, 0, 100, DNP);
				BringWindowToTop(hwndDlg);
				return 1;
			}
			break;
		case WM_CLOSE:
			{
				if (!ms_pThis->m_bOnlyMessage)
				{
					HWND hTmp = GetDlgItem(hwndDlg, ID_EDIT);
					GetWindowText(hTmp, ms_pThis->m_EditData.GetBuffer(8192), 8192);
				}
				EndDialog(hwndDlg, 0);
				return 1;
			}
			break;
		case WM_COMMAND:
			{
				int Control = LOWORD(wParam);
				int Message = HIWORD(wParam);
//					HWND Window = (HWND)lParam;
				if (Message == BN_CLICKED)
				{
					if (Control == IDB_Yes)
					{
						if (!ms_pThis->m_bOnlyMessage)
						{
							HWND hTmp = GetDlgItem(hwndDlg, ID_EDIT);
							GetWindowText(hTmp, ms_pThis->m_EditData.GetBuffer(8192), 8192);
						}
						EndDialog(hwndDlg, 1);
						return 1;
					}
					else if (Control == IDB_No)
					{
						if (!ms_pThis->m_bOnlyMessage)
						{
							HWND hTmp = GetDlgItem(hwndDlg, ID_EDIT);
							GetWindowText(hTmp, ms_pThis->m_EditData.GetBuffer(8192), 8192);
						}
						EndDialog(hwndDlg, 0);
						return 1;
					}
				}

			}
			break;

		}
		return 0;
	}


	LRESULT DisplayMyMessage(HINSTANCE hinst, HWND hwndOwner, 
		LPCSTR lpszMessage, LPCSTR lpszTitle, bint _bMessageOnly)
	{
//		Sleep (20000);
		HGLOBAL hgbl;
		LPDLGTEMPLATE lpdt;
		LPDLGITEMTEMPLATE lpdit;
		LPWORD lpw;
		LPWSTR lpwsz;
		LRESULT ret;

		hgbl = GlobalAlloc(GMEM_ZEROINIT, 2048);
		if (!hgbl)
			return -1;
	 
		lpdt = (LPDLGTEMPLATE)GlobalLock(hgbl);
	 
		// Define a dialog box.
		int StaticY = 30;
		int EditY = 50;
		int Width = 250;
		if (_bMessageOnly)
			EditY = 0;
		int yPos = StaticY + EditY;
	 
		lpdt->style = WS_POPUP | WS_BORDER | WS_SYSMENU
					   | DS_MODALFRAME | WS_CAPTION;
		if (_bMessageOnly)
			lpdt->cdit = 1;  // number of controls
		else
			lpdt->cdit = 4;  // number of controls
		lpdt->x  = 10;  lpdt->y  = 10;
		lpdt->cx = Width; 
		lpdt->cy = 75 + yPos;

		lpw = (LPWORD) (lpdt + 1);
		*lpw++ = 0;   // no menu
		*lpw++ = 0;   // predefined dialog box class (by default)

		lpwsz = (LPWSTR) lpw;
		lpw = (LPWORD)NStr::StrCopy((wchar_t *)lpw, lpszTitle);
		++lpw;

		if (!_bMessageOnly)
		{
			//-----------------------
			// Define an Yes button.
			//-----------------------
			lpw = AlignUp (lpw, 4); // align DLGITEMTEMPLATE on DWORD boundary
			lpdit = (LPDLGITEMTEMPLATE) lpw;
			lpdit->x  = 65+25; lpdit->y  = 55 + yPos;
			lpdit->cx = 35; lpdit->cy = 12;
			lpdit->id = IDB_Yes;  // OK button identifier
			lpdit->style = WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON;

			lpw = (LPWORD) (lpdit + 1);
			*lpw++ = 0xFFFF;
			*lpw++ = 0x0080;    // button class

			lpwsz = (LPWSTR) lpw;
			lpw = (LPWORD)NStr::StrCopy((wchar_t *)lpw, "Yes");
			++lpw;
			*lpw++ = 0x0000; // Creation Data

			//-----------------------
			// Define an No button.
			//-----------------------
			lpw = AlignUp (lpw, 4); // align DLGITEMTEMPLATE on DWORD boundary
			lpdit = (LPDLGITEMTEMPLATE) lpw;
			lpdit->x  = 105+25; lpdit->y  = 55 + yPos;
			lpdit->cx = 35; lpdit->cy = 12;
			lpdit->id = IDB_No;  // OK button identifier
			lpdit->style = WS_CHILD | WS_VISIBLE;

			lpw = (LPWORD) (lpdit + 1);
			*lpw++ = 0xFFFF;
			*lpw++ = 0x0080;    // button class

			lpwsz = (LPWSTR) lpw;
			lpw = (LPWORD)NStr::StrCopy((wchar_t *)lpw, "No");
			++lpw;
			*lpw++ = 0x0000; // Creation Data
		}

		//-----------------------
		// Define a static text control.
		//-----------------------
		lpw = AlignUp (lpw, 4); // align DLGITEMTEMPLATE on DWORD boundary
		lpdit = (LPDLGITEMTEMPLATE) lpw;
		lpdit->x  = 7; lpdit->y  = 7;
		lpdit->cx = Width - 20; lpdit->cy = 45 + StaticY;
		lpdit->id = ID_TEXT;  // text identifier
		lpdit->style = WS_CHILD | WS_VISIBLE | SS_LEFT;

		lpw = (LPWORD) (lpdit + 1);
		*lpw++ = 0xFFFF;
		*lpw++ = 0x0082;                         // static class

		lpw = (LPWORD)NStr::StrCopy((wchar_t *)lpw, lpszMessage);
		++lpw;
		*lpw++ = 0x0000; // Creation Data

		if (!_bMessageOnly)
		{
			//-----------------------
			// Define a edit control.
			//-----------------------
			lpw = AlignUp (lpw, 4); // align DLGITEMTEMPLATE on DWORD boundary
			lpdit = (LPDLGITEMTEMPLATE) lpw;
			lpdit->x  = 7; lpdit->y  = 55 + StaticY;
			lpdit->cx = Width - 15; lpdit->cy = EditY - 10;
			lpdit->id = ID_EDIT;  // text identifier
			lpdit->style = WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_BORDER | ES_WANTRETURN;
	//		lpdit->style = WS_CHILD | WS_VISIBLE | SS_LEFT;

			lpw = (LPWORD) (lpdit + 1);
			*lpw++ = 0xFFFF;
			*lpw++ = 0x0081;                         // edit class

			lpw = (LPWORD)NStr::StrCopy((wchar_t *)lpw, "Write comments for bugreport here. Please describe what you were doing when the program crashed.");
			++lpw;
			*lpw++ = 0x0000; // Creation Data
		}

		GlobalUnlock(hgbl);

		m_NormalFont = f_CreatePointFont(80, L"MS Sans Serif");
		ret = DialogBoxIndirectW(hinst, (LPDLGTEMPLATE) hgbl, hwndOwner, (DLGPROC) msp_HandleMessages);
		DeleteObject(m_NormalFont);

		GlobalFree(hgbl);

		return ret; 
	}

	DIdsPNoInline bint DisplayMessage(CStr _Title, CStr _Message, bint _bOnlyMessage)
	{
		int Return = DisplayMyMessage((HINSTANCE)NULL, NULL, _Message, _Title, _bOnlyMessage);

		if (Return == -1)
		{
		}

		if (Return == 1)
			return 1;
		else
			return 0;
	}

	bint m_bOnlyMessage;
	bint m_bRet;
	CStr m_EditData;
	HWND m_hWnd;

	void StartInfoDisplay(CStr _Info)
	{
		m_Message = _Info;
		m_bOnlyMessage = true;
		Thread_Create();
	}

	void DisplayInfo(CStr _Info)
	{
		HWND hTmp = GetDlgItem(m_hWnd, ID_TEXT);
		::SetWindowText(hTmp, _Info.Str());
	}

	void StopInfoDisplay(bint _bClose)
	{
		if (_bClose)
			SendMessage(m_hWnd, WM_CLOSE, 0, 0);

		Thread_Destroy();
	}

	int Thread_Main()
	{
		m_bRet = DisplayMessage(m_Title, m_Message, m_bOnlyMessage);
		M_EXPORTBARRIER;
		return 0;
	}

	bint DoMessage()
	{
		m_bOnlyMessage = false;
		Thread_Create();
		Thread_Destroy();
		M_IMPORTBARRIER;
		return m_bRet;
	}
};

CCrashDialog *CCrashDialog::ms_pThis = NULL;

#include <mapi.h>

class CSendFileTo
{
public:
    bool SendMail(CStr _To, CStr _ToEmail, CStr _Subject, CStr _Body, CStr _Attach, CStr _Attach1, CStr _Attach0Desc, CStr _Attach1Desc)
    {

        HINSTANCE hMAPI = ::LoadLibraryA("MAPI32.DLL");
        if (!hMAPI)
            return false;

        // Grab the exported entry point for the MAPISendMail function
        ULONG (PASCAL *SendMail)(ULONG, ULONG_PTR, MapiMessage*, FLAGS, ULONG);
        (FARPROC&)SendMail = GetProcAddress(hMAPI, "MAPISendMail");
        if (!SendMail)
            return false;

		MapiRecipDesc Recipient;
        ::ZeroMemory(&Recipient, sizeof(Recipient));
		CStr To = CStrF("%s<%s>", _To.Str(), _ToEmail.Str());
		Recipient.lpszName = To;
//		Recipient.lpszAddress = _ToEmail;
		Recipient.ulRecipClass = MAPI_TO;
				
        MapiFileDesc fileDesc[2];
        ::ZeroMemory(&fileDesc, sizeof(fileDesc));
        fileDesc[0].nPosition = (ULONG)-1;
        fileDesc[0].lpszPathName = _Attach;
        fileDesc[0].lpszFileName = _Attach0Desc;
        fileDesc[1].nPosition = (ULONG)-1;
        fileDesc[1].lpszPathName = _Attach1;
        fileDesc[1].lpszFileName = _Attach1Desc;

        MapiMessage message;
        ::ZeroMemory(&message, sizeof(message));
        message.lpszSubject = _Subject;
		message.lpszNoteText = _Body;
		message.lpRecips = &Recipient;
		message.nRecipCount = 1;

		if (!_Attach.IsEmpty() && CDiskUtil::FileExists(_Attach))
		{
			++message.nFileCount;
			message.lpFiles = fileDesc;
		}
		if (!_Attach1.IsEmpty() && CDiskUtil::FileExists(_Attach1))
		{
			++message.nFileCount;
			if (message.nFileCount == 2)
				message.lpFiles = fileDesc;
			else
				message.lpFiles = fileDesc + 1;
		}

        // Ok to send
        int nError = SendMail(0, NULL, &message, MAPI_LOGON_UI|MAPI_DIALOG, 0);

        if (nError != SUCCESS_SUCCESS)
              return false;

        return true;
    }
};

static CStr EncodeEmail(CStr _In)
{
	const uint8 *pParse = (const uint8 *)_In.Str();
	mint Len = _In.Len();
	mint nEncode = 0;
	while (*pParse)
	{
		if (*pParse < 32)
			++nEncode;
		++pParse;
	}
	CStr Ret;
	pParse = (const uint8 *)_In.Str();
	uint8 *pStr = (uint8 *)Ret.GetBuffer(Len + nEncode*2+1);
	while (*pParse)
	{
		if (*pParse < 32)
		{
			CFStr Temp = CFStrF("%%%02x", *pParse);
			*(pStr++) = Temp[0];
			*(pStr++) = Temp[1];
			*(pStr++) = Temp[2];
		}
		else
		{
			*pStr = *pParse;
			++pStr;
		}
		++pParse;
	}
	*pStr = NULL;
	return Ret;
}
LONG WINAPI MRTC_SystemInfoInternal::UnhandledException(struct _EXCEPTION_POINTERS *_pExceptionInfo)
{

	CCrashDialog CrashDialog;

	if(!MRTC_SystemInfoInternal::ms_bHandleException)
	{
		// If exception handling isn't wanted, just keel over and die
		ExitProcess(0);
	}

	MRTC_SystemInfoInternal *pLocalSys = MRTC_SystemInfo::MRTC_GetSystemInfo().m_pInternalData;
	CStackTraceContext *pStackTracer = pLocalSys->GetStackTraceContext();

	CStr FileName;
	CStr FileNameDumpMini;
	CStr FileNameLog;
	CStr FileNameDump;


	typedef enum _MINIDUMP_TYPE {
			MiniDumpNormal                         = 0x00000000,
			MiniDumpWithDataSegs                   = 0x00000001,
			MiniDumpWithFullMemory                 = 0x00000002,
			MiniDumpWithHandleData                 = 0x00000004,
			MiniDumpFilterMemory                   = 0x00000008,
			MiniDumpScanMemory                     = 0x00000010,
			MiniDumpWithUnloadedModules            = 0x00000020,
			MiniDumpWithIndirectlyReferencedMemory = 0x00000040,
			MiniDumpFilterModulePaths              = 0x00000080,
			MiniDumpWithProcessThreadData          = 0x00000100,
			MiniDumpWithPrivateReadWriteMemory     = 0x00000200,
			MiniDumpWithoutOptionalData            = 0x00000400,
			MiniDumpWithFullMemoryInfo             = 0x00000800,
			MiniDumpWithThreadInfo                 = 0x00001000,
			MiniDumpWithCodeSegs                   = 0x00002000,
			MiniDumpWithoutManagedState            = 0x00004000,
		} MINIDUMP_TYPE;

	CStr ExceptionInfo;
	ExceptionInfo += "Unhandeled excepion"NL NL;

	// 
	// Type
	//
	CStr Code = "Unknown";

	switch (_pExceptionInfo->ExceptionRecord->ExceptionCode)
	{		
	case EXCEPTION_ACCESS_VIOLATION:
		{
			if (_pExceptionInfo->ExceptionRecord->ExceptionInformation[0])
				Code = FormatStr("Access violation trying to write to address: 0x%0"CPU_FORMATHEX"x", 
				(mint)_pExceptionInfo->ExceptionRecord->ExceptionInformation[1]);
			else
				Code = FormatStr("Access violation trying to read to address: 0x%0"CPU_FORMATHEX"x", 
				(mint)_pExceptionInfo->ExceptionRecord->ExceptionInformation[1]);
		}
		break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: Code = "Array bounds exceeded";break;
	case EXCEPTION_BREAKPOINT: Code = "Breakpoint";break;
	case EXCEPTION_DATATYPE_MISALIGNMENT: Code = "Datatype misalignment";break;
	case EXCEPTION_FLT_DENORMAL_OPERAND: Code = "Float denormal operand";break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO: Code = "Float divide by zero";break;
	case EXCEPTION_FLT_INEXACT_RESULT: Code = "Float inexact result";break;
	case EXCEPTION_FLT_INVALID_OPERATION: Code = "Float invalid operation";break;
	case EXCEPTION_FLT_OVERFLOW: Code = "Float overflow";break;
	case EXCEPTION_FLT_STACK_CHECK: Code = "Float stack check";break;
	case EXCEPTION_FLT_UNDERFLOW: Code = "Float underflow";break;
	case EXCEPTION_ILLEGAL_INSTRUCTION: Code = "Illegal instruction";break;
	case EXCEPTION_IN_PAGE_ERROR: Code = "In page error";break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO: Code = "Integer divide by zero";break;
	case EXCEPTION_INT_OVERFLOW: Code = "Integer overflow";break;
	case EXCEPTION_INVALID_DISPOSITION: Code = "Invalid disposition";break;
	case EXCEPTION_NONCONTINUABLE_EXCEPTION: Code = "Noncontinuable exception";break;
	case EXCEPTION_PRIV_INSTRUCTION: Code = "Priviledged instruction";break;
	case EXCEPTION_SINGLE_STEP: Code = "Single step";break;
	case EXCEPTION_STACK_OVERFLOW: Code = "Stack overflow";break;
	default:
		{
			if (_pExceptionInfo->ExceptionRecord->NumberParameters > 2)
			{
				CCException *pException = (CCException *)_pExceptionInfo->ExceptionRecord->ExceptionInformation[1];
				if (!IsBadReadPtr(&pException->m_Magic, 4))
				{
					if (pException->m_Magic == 0xfe58105a)
					{					
						Code = CStr("Starbreeze CCException"NL NL"Location: ") + pException->m_Info.m_Location + NL"Message: " + pException->m_Info.m_Message + NL"SourcePos: " + pException->m_Info.m_SourcePos;
					}
				}
			}
			break;
		}
	}
	CStr CrashStr;
	CrashStr = CStr() + "Program has crashed with an unhandeled exception."NL NL + Code + NL NL "Do you want to create bug report?";

	CStr CommentText;
	if(ms_bAutoCoredump == false)
	{
		CrashDialog.m_Message = CrashStr;
		CrashDialog.m_Title = "Crash";

		if (!CrashDialog.DoMessage())
			ExitProcess(0);
		else
		{
			CommentText = CrashDialog.m_EditData;
			ExceptionInfo += CStr("") + NL NL + "Comments: " NL NL + CommentText + "" NL NL;
		}
	}

	CStr InfoDisplayText = "Please wait while a crash log is beeing generated" NL NL;
	if (!ms_bAutoCoredump)
		CrashDialog.StartInfoDisplay(InfoDisplayText + "Writing mini crash dump");

	pStackTracer->Init();
	CStr ComputerName;
	CStr ProgramName;
	{
		CStr CompName;
		CCFile File;
		SYSTEMTIME DateTime;
		GetLocalTime(&DateTime);

		uint32 Sizee = 1024;
		GetComputerName(CompName.GetBuffer(1024), &Sizee);
		ComputerName = CompName;

		ProgramName = gf_GetProgramPath().GetFilenameNoExt();
		CompName = CompName + "_" + ProgramName;
		
		CStr Temp;

		Temp = FormatStr("\\%s_%d-%02d-%02d_%02d.%02d.%02d.%03d", CompName.Str(), DateTime.wYear, DateTime.wMonth, DateTime.wDay, DateTime.wHour, DateTime.wMinute, DateTime.wSecond, DateTime.wMilliseconds);

		CStr CommonCrashDumps;
		CommonCrashDumps = "Z:\\Files\\CrashDumps";
		
		if (CDiskUtil::DirectoryExists(CommonCrashDumps))
		{
			Temp = CommonCrashDumps + Temp;
		}
		else
		{
			Temp = gf_GetProgramDirectory() + Temp;
		}
		
		FileName = Temp + ".crashlog.txt";
		FileNameDump = Temp + ".full.dmp";
		FileNameDumpMini = Temp + ".mini.dmp";
		FileNameLog = Temp + ".log.txt";

		if (pStackTracer->MiniDumpWriteDump)
		{
			MINIDUMP_EXCEPTION_INFORMATION Info;
			Info.ClientPointers = false;
			Info.ExceptionPointers = _pExceptionInfo;
			Info.ThreadId = GetCurrentThreadId();

//			if (MessageBox(NULL, "Do you want to create a minidump?", "Crash", MB_YESNO | MB_ICONERROR) == IDYES)
			if (1)
			{
				void *pFile = WindowFileHelper(FileNameDumpMini, false, true, true, true, false);
				if (pFile)
				{
					::MINIDUMP_TYPE DumpType = (::MINIDUMP_TYPE )(MiniDumpWithHandleData | MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules);
					if (!pStackTracer->MiniDumpWriteDump(pStackTracer->m_hProcess, GetCurrentProcessId(), pFile, DumpType, &Info, NULL, NULL))
					{
						MessageBox(NULL, "Failed to mini dump program", "Error", MB_OK);
					}
					CloseHandle(pFile);
				}
				else 
					FileNameDumpMini = "";
			}
			else 
				FileNameDumpMini = "";
		}
	}

	if (CDiskUtil::FileExists(pLocalSys->m_LogFileName))
		CopyFileA(pLocalSys->m_LogFileName, FileNameLog, false);
	if (!ms_bAutoCoredump)
		CrashDialog.DisplayInfo(InfoDisplayText + "Doing stack trace");

	CStr MiniExceptionInfo;
	MiniExceptionInfo += "Exception type: " + Code + NL NL;

	bool bCanContinue = !(_pExceptionInfo->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE) && _pExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT;
//		if (!bCanContinue)
//			MiniExceptionInfo += "Exception is noncontinuable"NL;


	//
	// Exception address
	//
	CStackTraceInfo *pAddressInfo = pStackTracer->Debug_AquireStackTraceInfo((mint)_pExceptionInfo->ExceptionRecord->ExceptionAddress);

	if (pAddressInfo)
	{
		MiniExceptionInfo += FormatStr("Exception address: 0x%0"CPU_FORMATHEX"x (%s!%s)", (mint)_pExceptionInfo->ExceptionRecord->ExceptionAddress
			, pAddressInfo->m_pModuleName, pAddressInfo->m_pFunctionName);
		pStackTracer->Debug_ReleaseStackTraceInfo(pAddressInfo);
	}
	else
	{
		MiniExceptionInfo += FormatStr("Exception address: 0x%0"CPU_FORMATHEX"x", (mint)_pExceptionInfo->ExceptionRecord->ExceptionAddress);
	}		

	ExceptionInfo += MiniExceptionInfo + NL NL;
	//
	// Stack trace
	//
	ExceptionInfo += "StackTrace: " NL;
	int iMaxDepth = 1024;

	mint ThreadTop = g_ThreadTop;
	if (!ThreadTop)
	{
		MEMORY_BASIC_INFORMATION MemoryInfo;
		int Size = sizeof(MemoryInfo);
		memset(&MemoryInfo, 0, Size);
		int nBytes = VirtualQuery(&MemoryInfo, &MemoryInfo, Size);
		if (nBytes != sizeof(MemoryInfo))
		{
			ThreadTop = g_ThreadTop = (mint)&MemoryInfo + 1024*1024; // Just guess (not more that 1 MB)
			int ErrTemp = GetLastError();
		}
		else
		{	
			ThreadTop = g_ThreadTop = (mint)MemoryInfo.BaseAddress + (mint)MemoryInfo.RegionSize;
		}
	}
	mint StackStart = ThreadTop;
	mint StackEnd = (((mint)&StackStart) - 15) &(~15);

#if 1
	uint32 Platform = IMAGE_FILE_MACHINE_I386;
#ifdef CPU_AMD64
	Platform = IMAGE_FILE_MACHINE_AMD64;
#endif

	CStr StackTrace;
	CStackTraceContext::STACKFRAME64 StackFrame;
	memset(&StackFrame, 0, sizeof(StackFrame));
#ifdef CPU_X86
	{ ADDRESS64 Temp = {_pExceptionInfo->ContextRecord->Eip, 0, AddrModeFlat}; StackFrame.AddrPC = Temp; }
	{ ADDRESS64 Temp = {_pExceptionInfo->ContextRecord->Ebp, 0, AddrModeFlat}; StackFrame.AddrFrame = Temp; }
	{ ADDRESS64 Temp = {_pExceptionInfo->ContextRecord->Esp, 0, AddrModeFlat}; StackFrame.AddrStack = Temp; }
//		StackFrame.AddrBStore = _pExceptionInfo->ContextRecord->RsBSP;
#elif defined(CPU_AMD64)
	{ ADDRESS64 Temp = {_pExceptionInfo->ContextRecord->Rip, 0, AddrModeFlat}; StackFrame.AddrPC = Temp; }
	{ ADDRESS64 Temp = {_pExceptionInfo->ContextRecord->Rbp, 0, AddrModeFlat}; StackFrame.AddrFrame = Temp; }
	{ ADDRESS64 Temp = {_pExceptionInfo->ContextRecord->Rsp, 0, AddrModeFlat}; StackFrame.AddrStack = Temp; }
//		StackFrame.AddrBStore = _pExceptionInfo->ContextRecord->RsBSP;
#else
#error "Implement this"
#endif

	void *pContext = _pExceptionInfo->ContextRecord;
	while (iMaxDepth)
	{
		if (!pStackTracer->StackWalk64(Platform, pStackTracer->m_hProcess, GetCurrentThread(), &StackFrame, pContext, NULL, (CStackTraceContext::PFUNCTION_TABLE_ACCESS_ROUTINE64 )pStackTracer->SymFunctionTableAccess64, (CStackTraceContext::PGET_MODULE_BASE_ROUTINE64 )pStackTracer->SymGetModuleBase64, NULL))
			break;
		pContext = NULL;
		uint64 CodePtr = StackFrame.AddrPC.Offset;

		CStackTraceInfo *pAddressInfo = pStackTracer->Debug_AquireStackTraceInfo(CodePtr);

		if (pAddressInfo)
		{
			StackTrace += FormatStr("0x%0"CPU_FORMATHEX"x %s!%s"NL"%s:%d" NL, (mint)CodePtr
				, pAddressInfo->m_pModuleName, pAddressInfo->m_pFunctionName
				, pAddressInfo->m_pSourceFileName, pAddressInfo->m_SourceLine
				);
			pStackTracer->Debug_ReleaseStackTraceInfo(pAddressInfo);
		}
		else
		{
			StackTrace += FormatStr("0x%0"CPU_FORMATHEX"x" NL, (mint)CodePtr);
		}		
		StackTrace += FormatStr("StackFrame: 0x%0"CPU_FORMATHEX"x" NL, (mint)StackFrame.AddrFrame.Offset);
		StackTrace += "" NL;

//			LastCode = CodePtr;

		--iMaxDepth;			

	}
#else
	try
	{
#ifdef CPU_X86
		mint StackFrame = _pExceptionInfo->ContextRecord->Ebp;
#elif defined(CPU_AMD64)
		mint StackFrame = _pExceptionInfo->ContextRecord->Rbp;
#else
#error "Implement this"
#endif
		mint LastCode = (mint)_pExceptionInfo->ExceptionRecord->ExceptionAddress;
		while (iMaxDepth)
		{
			if (!IsGoodStackPtr((void *)StackFrame, sizeof(mint) * 2, StackStart))
				break;
			mint CodePtr = *((mint *)(StackFrame + sizeof(mint)));

			CStackTraceInfo *pAddressInfo = pStackTracer->Debug_AquireStackTraceInfo(LastCode);

			if (pAddressInfo)
			{
				StackTrace += FormatStr("0x%0"CPU_FORMATHEX"x %s!%s"NL"%s:%d" NL, (mint)LastCode
					, pAddressInfo->m_pModuleName, pAddressInfo->m_pFunctionName
					, pAddressInfo->m_pSourceFileName, pAddressInfo->m_SourceLine
					);
				pStackTracer->Debug_ReleaseStackTraceInfo(pAddressInfo);
			}
			else
			{
				StackTrace += FormatStr("0x%0"CPU_FORMATHEX"x" NL, (mint)LastCode);
			}		
			StackTrace += FormatStr("StackFrame: 0x%0"CPU_FORMATHEX"x" NL, (mint)StackFrame);
			StackTrace += "" NL;

			LastCode = CodePtr;

			StackFrame = *((mint *)(StackFrame));
			--iMaxDepth;			
		}
	}
	catch(...)
	{
	}
#endif

	ExceptionInfo += StackTrace;
	//
	// Register information
	//
#ifdef CPU_X86
	if (_pExceptionInfo->ContextRecord->ContextFlags & CONTEXT_INTEGER)
	{
		ExceptionInfo += FormatStr("Integer registers:"NL
			"Edi=0x%0"CPU_FORMATHEX"x Esi=0x%0"CPU_FORMATHEX"x Eax=0x%0"CPU_FORMATHEX"x"NL
			"Ebx=0x%0"CPU_FORMATHEX"x Ecx=0x%0"CPU_FORMATHEX"x Edx=0x%0"CPU_FORMATHEX"x"NL NL,
			(mint)_pExceptionInfo->ContextRecord->Edi,
			(mint)_pExceptionInfo->ContextRecord->Esi,
			(mint)_pExceptionInfo->ContextRecord->Eax,
			(mint)_pExceptionInfo->ContextRecord->Ebx,
			(mint)_pExceptionInfo->ContextRecord->Ecx,
			(mint)_pExceptionInfo->ContextRecord->Edx
			);
	}
	if (_pExceptionInfo->ContextRecord->ContextFlags & CONTEXT_CONTROL)
	{
		ExceptionInfo += FormatStr("Control registers:"NL
			"Eip=0x%0"CPU_FORMATHEX"x Ebp=0x%0"CPU_FORMATHEX"x Esp=0x%0"CPU_FORMATHEX"x" NL
			"SegCs=0x%0"CPU_FORMATHEX"x SegSs=0x%0"CPU_FORMATHEX"x EFlags=0x%0"CPU_FORMATHEX"x"NL NL,
			(mint)_pExceptionInfo->ContextRecord->Eip,
			(mint)_pExceptionInfo->ContextRecord->Ebp,
			(mint)_pExceptionInfo->ContextRecord->Esp,
			(mint)_pExceptionInfo->ContextRecord->SegCs,
			(mint)_pExceptionInfo->ContextRecord->SegSs,
			(mint)_pExceptionInfo->ContextRecord->EFlags
			);
	}

	if (_pExceptionInfo->ContextRecord->ContextFlags & CONTEXT_DEBUG_REGISTERS)
	{
		ExceptionInfo += FormatStr("Debug registers:"NL
			"Dr0=0x%0"CPU_FORMATHEX"x Dr1=0x%0"CPU_FORMATHEX"x Dr2=0x%0"CPU_FORMATHEX"x"NL
			"Dr3=0x%0"CPU_FORMATHEX"x Dr6=0x%0"CPU_FORMATHEX"x Dr7=0x%0"CPU_FORMATHEX"x"NL,
			(mint)_pExceptionInfo->ContextRecord->Dr0,
			(mint)_pExceptionInfo->ContextRecord->Dr1,
			(mint)_pExceptionInfo->ContextRecord->Dr2,
			(mint)_pExceptionInfo->ContextRecord->Dr3,
			(mint)_pExceptionInfo->ContextRecord->Dr6,
			(mint)_pExceptionInfo->ContextRecord->Dr7
			);
	}

	if (_pExceptionInfo->ContextRecord->ContextFlags & CONTEXT_SEGMENTS)
	{
		ExceptionInfo += FormatStr("Segment registers:" NL
			"SegGs=0x%0"CPU_FORMATHEX"x SegFs=0x%0"CPU_FORMATHEX"x" NL
			"SegEs=0x%0"CPU_FORMATHEX"x SegDs=0x%0"CPU_FORMATHEX"x" NL NL,
			(mint)_pExceptionInfo->ContextRecord->SegGs,
			(mint)_pExceptionInfo->ContextRecord->SegFs,
			(mint)_pExceptionInfo->ContextRecord->SegEs,
			(mint)_pExceptionInfo->ContextRecord->SegDs
			);
	}

	if (_pExceptionInfo->ContextRecord->ContextFlags & CONTEXT_FLOATING_POINT)
	{
		FLOATING_SAVE_AREA &FloatSaveArea = _pExceptionInfo->ContextRecord->FloatSave;

		ExceptionInfo += FormatStr("Floating point registers:" NL
			"ControlWord=0x%04x StatusWord=0x%04x TagWord=0x%04x" NL
			"ErrorOffset=0x%0"CPU_FORMATHEX"x ErrorSelector=0x%0"CPU_FORMATHEX"x DataOffset=0x%0"CPU_FORMATHEX"x" NL
			"DataSelector=0x%0"CPU_FORMATHEX"x Cr0NpxState=0x%0"CPU_FORMATHEX"x" NL,
			FloatSaveArea.ControlWord&0xffff,
			FloatSaveArea.StatusWord&0xffff,
			FloatSaveArea.TagWord&0xffff,
			(mint)FloatSaveArea.ErrorOffset,
			(mint)FloatSaveArea.ErrorSelector,
			(mint)FloatSaveArea.DataOffset,
			(mint)FloatSaveArea.DataSelector,
			(mint)FloatSaveArea.Cr0NpxState
			);

		ExceptionInfo += FormatStr(
			"St0=0x%04x%016I64x St1=0x%04x%016I64x" NL
			"St2=0x%04x%016I64x St3=0x%04x%016I64x" NL
			"St4=0x%04x%016I64x St5=0x%04x%016I64x" NL
			"St6=0x%04x%016I64x St7=0x%04x%016I64x" NL NL,
			*((uint16 *)&FloatSaveArea.RegisterArea[0*10+8]),
			*((uint64 *)&FloatSaveArea.RegisterArea[0*10]),
			*((uint16 *)&FloatSaveArea.RegisterArea[1*10+8]),
			*((uint64 *)&FloatSaveArea.RegisterArea[1*10]),
			*((uint16 *)&FloatSaveArea.RegisterArea[2*10+8]),
			*((uint64 *)&FloatSaveArea.RegisterArea[2*10]),
			*((uint16 *)&FloatSaveArea.RegisterArea[3*10+8]),
			*((uint64 *)&FloatSaveArea.RegisterArea[3*10]),
			*((uint16 *)&FloatSaveArea.RegisterArea[4*10+8]),
			*((uint64 *)&FloatSaveArea.RegisterArea[4*10]),
			*((uint16 *)&FloatSaveArea.RegisterArea[5*10+8]),
			*((uint64 *)&FloatSaveArea.RegisterArea[5*10]),
			*((uint16 *)&FloatSaveArea.RegisterArea[6*10+8]),
			*((uint64 *)&FloatSaveArea.RegisterArea[6*10]),
			*((uint16 *)&FloatSaveArea.RegisterArea[7*10+8]),
			*((uint64 *)&FloatSaveArea.RegisterArea[7*10])
			);
	}

	if (_pExceptionInfo->ContextRecord->ContextFlags & CONTEXT_EXTENDED_REGISTERS)
	{
		ExceptionInfo += FormatStr("SSE registers:" NL
			"MXCSR=0x%08x" NL
			"Xmm0=0x%016I64x%016I64x Xmm1=0x%016I64x%016I64x" NL
			"Xmm2=0x%016I64x%016I64x Xmm3=0x%016I64x%016I64x" NL
			"Xmm4=0x%016I64x%016I64x Xmm5=0x%016I64x%016I64x" NL
			"Xmm6=0x%016I64x%016I64x Xmm7=0x%016I64x%016I64x" NL NL,
			*((uint32 *)&_pExceptionInfo->ContextRecord->ExtendedRegisters[24]),
			*((uint64 *)&_pExceptionInfo->ContextRecord->ExtendedRegisters[10*16+8]),
			*((uint64 *)&_pExceptionInfo->ContextRecord->ExtendedRegisters[10*16]),
			*((uint64 *)&_pExceptionInfo->ContextRecord->ExtendedRegisters[11*16+8]),
			*((uint64 *)&_pExceptionInfo->ContextRecord->ExtendedRegisters[11*16]),
			*((uint64 *)&_pExceptionInfo->ContextRecord->ExtendedRegisters[12*16+8]),
			*((uint64 *)&_pExceptionInfo->ContextRecord->ExtendedRegisters[12*16]),
			*((uint64 *)&_pExceptionInfo->ContextRecord->ExtendedRegisters[13*16+8]),
			*((uint64 *)&_pExceptionInfo->ContextRecord->ExtendedRegisters[13*16]),
			*((uint64 *)&_pExceptionInfo->ContextRecord->ExtendedRegisters[14*16+8]),
			*((uint64 *)&_pExceptionInfo->ContextRecord->ExtendedRegisters[14*16]),
			*((uint64 *)&_pExceptionInfo->ContextRecord->ExtendedRegisters[15*16+8]),
			*((uint64 *)&_pExceptionInfo->ContextRecord->ExtendedRegisters[15*16]),
			*((uint64 *)&_pExceptionInfo->ContextRecord->ExtendedRegisters[16*16+8]),
			*((uint64 *)&_pExceptionInfo->ContextRecord->ExtendedRegisters[16*16]),
			*((uint64 *)&_pExceptionInfo->ContextRecord->ExtendedRegisters[17*16+8]),
			*((uint64 *)&_pExceptionInfo->ContextRecord->ExtendedRegisters[17*16])
			);
	}
#endif

	// Dll map
	{
		if (pStackTracer->EnumerateLoadedModules64)
		{
			CEnumSymbolsContext Context;
			Context.m_String = "Modules:"NL;
			if (!pStackTracer->EnumerateLoadedModules64(pStackTracer->m_hProcess, EnumModulesCallback, &Context))
			{
				HRESULT Error = GetLastError();
				M_TRACEALWAYS("%d\n", Error);
			}
			Context.m_String += NL;
			ExceptionInfo += Context.m_String;
		}
	}



	//
	// Stack
	//

	CStr Stack;
	int iRowSize = 32;
	int iRow = iRowSize;			
	Stack += FormatStr("0x%0"CPU_FORMATHEX"x: ", (mint)StackEnd);
	while (StackEnd < StackStart)
	{
		int iMax = Min((int)StackStart - (int)StackEnd, iRow);
		if (iMax >= 8)
		{

			Stack += FormatStr("%0*I64x ", 16, ByteSwap(*((uint64 *)StackEnd)));
			StackEnd += 8;
			iRow -= 8;
		}
		else if (iMax >= 4)
		{
			Stack += FormatStr("%08x", ByteSwap(*((uint32 *)StackEnd)));
			StackEnd += 4;
			iRow -= 4;
		}
		else if (iMax >= 2)
		{
			Stack += FormatStr("%04x", ByteSwap(*((uint16 *)StackEnd)));
			StackEnd += 2;
			iRow -= 2;
		}
		else if (iMax >= 1)
		{
			Stack += FormatStr("%02x", *((uint8 *)StackEnd));
			StackEnd += 1;
			iRow -= 1;
		}

		if (iRow <= 0)
		{
			iRow = iRowSize;
			Stack += "" NL;
			Stack += FormatStr("0x%0"CPU_FORMATHEX"x: ", (mint)StackEnd);
		}
	}

	ExceptionInfo += Stack;


	{

		CCFile File;
		File.Open(FileName, CFILE_WRITE | CFILE_BINARY | CFILE_TRUNC);
		File.Write(ExceptionInfo.Str(), ExceptionInfo.Len());
		File.Close();
	}

	if (!ms_bAutoCoredump)
		CrashDialog.DisplayInfo(InfoDisplayText + "Writing full dump");
	// Mini dump
	{
//#ifdef M_Profile
		if (pStackTracer->MiniDumpWriteDump)
		{
			MINIDUMP_EXCEPTION_INFORMATION Info;
			Info.ClientPointers = false;
			Info.ExceptionPointers = _pExceptionInfo;
			Info.ThreadId = GetCurrentThreadId();

			if (1)// || MessageBox(NULL, "Do you want to create a full dump?", "Crash", MB_YESNO | MB_ICONERROR) == IDYES)
			{
				void *pFile = WindowFileHelper(FileNameDump, false, true, true, true, false);
				if (pFile)
				{
					
					::MINIDUMP_TYPE DumpType = (::MINIDUMP_TYPE)(MiniDumpWithFullMemory | MiniDumpWithFullMemoryInfo | MiniDumpWithHandleData | MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules);

					if (!pStackTracer->MiniDumpWriteDump(pStackTracer->m_hProcess, GetCurrentProcessId(), pFile, DumpType, &Info, NULL, NULL))
					{
						MessageBox(NULL, "Failed to dump program", "Error", MB_OK);
					}
					CloseHandle(pFile);
				}
				else
					FileNameDump = "";
			}
			else
				FileNameDump = "";
		}
//#endif
	}

	//
	// Message box
	//
	const ch8 *pProgramName = ProgramName;
	if (!pProgramName)
		pProgramName = "The program";

	bint bContinue = false;
	if (!ms_bAutoCoredump)
	{
		CrashDialog.DisplayInfo(InfoDisplayText + "Waiting for user to send email");

//		MailTo += StackTrace;
#if 1
		CStr Body = CommentText + NL NL + MiniExceptionInfo + NL NL +"Crash logs have been generated: " NL NL
		+ "file://" + FileName + "" NL
		+ "file://" + FileNameDumpMini + NL
		+ "file://" + FileNameDump + NL 
		+ "file://" + FileNameLog + NL
		NL + "Programmer Help:" NL
		"MODPATH=symsrv*symsrv.dll*c:\\Symbols*z:\\Files\\Symbols*http://msdl.microsoft.com/download/symbols"+ NL
		"http://tiger.starbreeze.com/wiki/index.php/Using_crash_dumps";
		
		Body += NL NL"Call stack:" NL NL;

		Body += StackTrace;

		CSendFileTo SendTo;
		bint bMailSent = SendTo.SendMail("Programmerare", "programmerare@starbreeze.com", CStrF("Crash report for %s running on %s", ProgramName.Str(), ComputerName.Str()), Body, FileName, FileNameLog, "CrashLog.txt", "Log.txt");
#else
		CStr MailTo;
		MailTo = CStrF("mailto:Programmers<programmerare@starbreeze.com>?subject=%s&attachment=\"%s\"&body=", CStrF("Crash report for %s running on %s", ProgramName.Str(), ComputerName.Str()).Str(),
			FileName.Str());
		MessageBox(NULL, MailTo.Str(), "", MB_OK);
//&attachment="\\myhost\myfolder\myfile.lis"
		MailTo += CommentText + NL NL + MiniExceptionInfo + NL NL +"Crash logs have been generated: " NL NL +
			
		"file://" + FileName + NL +
		"file://" + FileNameDumpMini + NL +
		"file://" + FileNameLog + NL +
		"file://" + FileNameDump + NL
		;

		CStr EncodedEmail = EncodeEmail(MailTo);
		EncodedEmail = EncodedEmail.Unicode();
		ShellExecuteW(NULL, L"open", (LPCWSTR)EncodedEmail.StrW(), NULL, NULL, SW_SHOWNORMAL);
#endif

		CStr SentMail = bMailSent?"A bug report mail was sent":"No bug report mail was sent";


		CStr Message = "Crash dumps have been generated: " NL NL +
			FileName + NL +
			FileNameDumpMini + NL +
			FileNameDump + NL +
			FileNameLog + NL +
			NL + SentMail + NL NL "You can now close this window.";

		CrashDialog.DisplayInfo(Message);

/*		if (bCanContinue)
			bContinue = MessageBoxA(NULL, 
			CStr(pProgramName) + " has encountered an unhandled exception. Crash logs have been generated: " NL NL +
			FileName + "" NL
//#ifdef M_Profile
			+ FileNameDumpMini + "" NL +
			FileNameDump
//#endif
			+ NL NL "Do you want to continue execution?"
			, "Exception", MB_YESNO | MB_ICONERROR) == IDYES;
		else
			MessageBoxA(NULL, 
			CStr(pProgramName) + " has encountered an unhandled exception. Crash logs have been generated: " NL NL +
			FileName + "" NL
//#ifdef M_Profile
			+ FileNameDumpMini + "" NL +
			FileNameDump
//#endif
			, "Exception", MB_OK | MB_ICONERROR);*/

	}

	if (!ms_bAutoCoredump)
		CrashDialog.StopInfoDisplay(false);

	if (bContinue)
	{
#ifdef CPU_X86
		_pExceptionInfo->ContextRecord->Eip++;
#elif defined(CPU_AMD64)
		_pExceptionInfo->ContextRecord->Rip++;
#else
#error "Implement this"
#endif
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	else
		ExitProcess(0);

	return EXCEPTION_CONTINUE_SEARCH;
}

#endif

#ifdef ENABLE_STACKTRACING
void MRTC_SystemInfo::OS_EnableUnhandledException(bool _bEnable)
{
	MRTC_SystemInfoInternal::ms_bHandleException	= _bEnable;
}

void MRTC_SystemInfo::OS_EnableAutoCoredumpOnException(bool _bEnabled)
{
	MRTC_SystemInfoInternal::ms_bAutoCoredump = _bEnabled;
}

#endif

#endif	// PLATFORM_WIN_PC

