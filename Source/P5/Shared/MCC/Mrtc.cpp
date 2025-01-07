#define CDE_EXCLUDE		// Excludes this file from ToolDocen

#include "PCH.h"

#ifdef PLATFORM_WIN
#pragma warning(disable:4756)
#endif

#if defined(TARGET_PS2) || defined(TARGET_PS3)	// Should be COMPILER_GNU but that hasn't been defined yet
	#include <new>
#else
	#include <new.h>
#endif

#include "MRTC.h"
#include "MDA.h"
class CStream;
#include "MFileDef.h"
#include "MRTC_CallGraph.h"
#include "MMemMgrHeap.h"

//#ifndef MRTC_MEMORYDEBUG

//#ifdef PLATFORM_XBOX
//	#define MRTC_MEMMANAGEROVERRIDE		//Enables SCB memory manager
//	#define MRTC_MEMMANAGEROVERRIDE_MEMDEBUG	//Enables SCB memory statistics
//#endif

#ifdef PLATFORM_XENON

//	#ifndef M_RTM
	#define MRTC_MEMMANAGEROVERRIDE		//Enables SCB memory manager
	#define MRTC_MEMMANAGEROVERRIDE_MEMDEBUG	//Enables SCB memory statistics
//	#endif

#elif defined(PLATFORM_XBOX1)
	#ifndef M_RTM
	#define MRTC_MEMMANAGEROVERRIDE		//Enables SCB memory manager
	#define MRTC_MEMMANAGEROVERRIDE_MEMDEBUG	//Enables SCB memory statistics
	#endif

#elif defined(PLATFORM_WIN_PC)
	#define MRTC_MEMMANAGEROVERRIDE		//Enables SCB memory manager
	#ifdef _DEBUG
	#define MRTC_MEMMANAGEROVERRIDE_MEMDEBUG	//Enables SCB memory statistics
	#endif

#elif defined(PLATFORM_DOLPHIN)
//	#define MRTC_MEMMANAGEROVERRIDE		//Enables SCB memory manager
	#ifndef M_RTM
		#define MRTC_MEMMANAGEROVERRIDE_MEMDEBUG	//Enables SCB memory statistics
	#endif

#elif defined PLATFORM_PS2
//	#define MRTC_MEMMANAGEROVERRIDE		//Enables SCB memory manager
	#ifdef _DEBUG
		#define MRTC_MEMMANAGEROVERRIDE_MEMDEBUG	//Enables SCB memory statistics
	#endif

#elif defined PLATFORM_PS3
//	#define MRTC_MEMMANAGEROVERRIDE		//Enables SCB memory manager
	#ifdef _DEBUG
		#define MRTC_MEMMANAGEROVERRIDE_MEMDEBUG	//Enables SCB memory statistics
	#endif

#else
	#define MRTC_MEMMANAGEROVERRIDE		//Enables SCB memory manager
	#ifdef _DEBUG
		#define MRTC_MEMMANAGEROVERRIDE_MEMDEBUG	//Enables SCB memory statistics
	#endif
#endif

// #undef MRTC_MEMMANAGEROVERRIDE_MEMDEBUG

//#define MRTC_LOGCONTEXTMEMORY
//#define MRTC_LOGALLOCATION


#ifdef PLATFORM_XBOX

	#include <xtl.h>


#pragma comment(lib, "xboxkrnl.lib")
#if defined(_DEBUG) || defined(M_RtmDebug)
	#pragma comment(lib, "xapilibd.lib")
#elif defined(M_Profile)
	#pragma comment(lib, "xapilibi.lib")
#else
	#pragma comment(lib, "xapilib.lib")
#endif

#elif defined PLATFORM_WIN
	#define WIN32_LEAN_AND_MEAN		// Get mindre smäck när man includerar windows.h
	#include <windows.h>

#elif defined PLATFORM_SHINOBI
	#include <errno.h>
	#include <usrsnasm.h>
	#include <stdlib.h>
#elif defined PLATFORM_PS2
	#include <eekernel.h>
#endif


#if defined(PLATFORM_DOLPHIN) && defined(USE_VIRTUAL_MEMORY)
# define MACRO_GetDefaultMemoryManager() MRTC_GetVirtualHeap()
#else
# define MACRO_GetDefaultMemoryManager() MRTC_GetMemoryManager()
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Memory management
|__________________________________________________________________________________________________
\*************************************************************************************************/
int MRTC_MemAvail()
{
	
#ifdef MRTC_MEMMANAGEROVERRIDE
	return MACRO_GetDefaultMemoryManager()->GetUsedMem();
//	return SCB_g_MemoryManager.GetManager()->GetFreeMem();

#elif defined PLATFORM_XBOX
	MEMORYSTATUS Memory;
	Memory.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&Memory);
	return Memory.dwAvailPhys;

#else
	return 0;

#endif
};

int MRTC_MemUsed()
{
#ifdef MRTC_MEMMANAGEROVERRIDE
	return MACRO_GetDefaultMemoryManager()->GetUsedMem();
#elif defined(PLATFORM_PS2)
	return MACRO_GetDefaultMemoryManager()->GetUsedMem();
#endif
	return 0;
}


int MRTC_MemDelta()
{
#ifdef PLATFORM_XBOX
	static int LastUsed = 0;
	int Cur = LastUsed - MRTC_MemUsed();
	LastUsed = MRTC_MemUsed();
	return Cur;
#else
	return 0;
#endif
};

void MRTC_Assert(const char* _pMsg, const char* _pFile, int _Line)
{
	MRTC_SystemInfo::OS_Assert(_pMsg, _pFile, _Line);
}

void* MRTC_MemAlloc(size_t _nSize, size_t _Align)
{
	return MRTC_GetMemoryManager()->AllocAlign(_nSize, _Align);
}
void* MRTC_MemRealloc(void* _pMem, size_t _nSize, size_t _Align)
{
	return MRTC_GetMemoryManager()->ReallocAlign(_pMem, _nSize, _Align);
}
void MRTC_MemFree(void* _pMem)
{
	MRTC_GetMemoryManager()->Free(_pMem);
}
size_t MRTC_MemSize(void* _pMem)
{
	return MRTC_GetMemoryManager()->MemorySize(_pMem);
}

#ifdef M_SUPPORTMEMORYDEBUG
	void* MRTC_MemAlloc(size_t _nSize, size_t _Align, const char* _File, int _Line)
	{
		return MRTC_GetMemoryManager()->AllocDebugAlign(_nSize, _Align, 1, _File, _Line);
	}

	void* MRTC_MemRealloc(void* _pMem, size_t _nSize, size_t _Align, const char* _File, int _Line)
	{
		return MRTC_GetMemoryManager()->ReallocDebugAlign(_pMem, _nSize, _Align, 1, _File, _Line);
	}

#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| new / delete, PLATFORM_XBOX
|__________________________________________________________________________________________________
\*************************************************************************************************/

void* M_CDECL operator new(mint _nSize)
{
#if defined(MRTC_MEMORYDEBUG) && !defined(PLATFORM_PS3)
	if (!MRTC_GetObjectManager()->ForgiveDebugNew())
		M_BREAKPOINT; // Use DNew to allocate memory 
#endif

	return M_ALLOC(_nSize);
}
void M_CDECL operator delete(void* p)
{
	return MRTC_GetMemoryManager()->Free(p);
}


#ifdef PLATFORM_PS3
void* M_CDECL operator new(mint _nSize, const std::nothrow_t&)
{
	return M_ALLOC(_nSize);
}

void* M_CDECL operator new[](mint _nSize, const std::nothrow_t&)
{
	return M_ALLOC(_nSize);
}

void* M_CDECL operator new[](mint _nSize)
{
	return M_ALLOC(_nSize);
}

void M_CDECL operator delete[](void* p)
{
	return MRTC_GetMemoryManager()->Free(p);
}

void M_CDECL operator delete(void* p, const std::nothrow_t&)
{
	return MRTC_GetMemoryManager()->Free(p);
}

void M_CDECL operator delete[](void* p, const std::nothrow_t&)
{
	return MRTC_GetMemoryManager()->Free(p);
}
#endif

#if defined(MRTC_MEMORYDEBUG) || defined(_DEBUG)

	void* M_CDECL operator new(mint _nSize, mint _Alignment, const char* _pFileName, int _Line, CAlignmentNewDummy *_pDummy)
	{
		return M_ALLOCDEBUGALIGN(_nSize, _Alignment, _pFileName, _Line);
	}
	#ifdef COMPILER_NEEDOPERATORDELETE
		void M_CDECL operator delete(void* p, mint _Alignment, const char* _pFileName, int _Line, CAlignmentNewDummy *_pDummy)
		{
			return MRTC_GetMemoryManager()->Free(p);
		}
	#endif

	void* operator new[](mint _nSize, mint _Alignment, const char* _pFileName, int _Line, CAlignmentNewDummy *_pDummy)
	{
		return M_ALLOCDEBUGALIGN(_nSize, _Alignment, _pFileName, _Line);
	}
	#ifdef COMPILER_NEEDOPERATORDELETE
		void M_CDECL operator delete[](void* p, mint _Alignment, const char* _pFileName, int _Line, CAlignmentNewDummy *_pDummy)
		{
			return MRTC_GetMemoryManager()->Free(p);
		}
	#endif
#endif

void* M_CDECL operator new(mint _nSize, mint _Alignment, CAlignmentNewDummy *_pDummy) throw()
{
	return MRTC_GetMemoryManager()->AllocAlign(_nSize, _Alignment);
}

#ifdef COMPILER_NEEDOPERATORDELETE
	void M_CDECL operator delete(void* p, mint _Alignment, CAlignmentNewDummy *_pDummy) throw()
	{
		return MRTC_GetMemoryManager()->Free(p);
	}
#endif

void* M_CDECL operator new[](mint _nSize, mint _Alignment, CAlignmentNewDummy *_pDummy) throw()
{
	return MRTC_GetMemoryManager()->AllocAlign(_nSize, _Alignment);
}
#ifdef COMPILER_NEEDOPERATORDELETE
	void M_CDECL operator delete[](void* p, mint _Alignment, CAlignmentNewDummy *_pDummy) throw()
	{
		return MRTC_GetMemoryManager()->Free(p);
	}
#endif

//#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| new / delete, MRTC_MEMORYDEBUG
|__________________________________________________________________________________________________
\*************************************************************************************************/
#define MRTC_FREE_BLOCK 0
#define MRTC_NORMAL_BLOCK 1


#ifdef _DEBUG

class MRTC_DLLProcessInfo* MRTC_DLLFindProcess();

bool g_MemFailRecursion = false;

bool g_EnableMemCheck = false;

void* M_CDECL operator new(mint _nSize, const char* _pFileName, int _Line)
{

#ifdef M_SUPPORTMEMORYDEBUG
	if (g_EnableMemCheck && !g_MemFailRecursion && !MACRO_GetDefaultMemoryManager()->CheckMemory())
	{
		g_MemFailRecursion = true;
		LogFile(CStrF("(::operator new) Memory check failed before allocating %d bytes in file %s, line %d", _nSize, _pFileName, _Line));
		Error_static("::operator new", CStrF("Memory check failed before allocating %d bytes in file %s, line %d", _nSize, _pFileName, _Line));
		g_MemFailRecursion = false;
	}
#endif
/**	char* pStr = (char*)_malloc_dbg(CStr::StrLen(_pFileName)+1, _IGNORE_BLOCK, "MRTC.cpp::operator new (_malloc_dbg string)", 27);
	if (_pFileName)
		strcpy(pStr, _pFileName);
	else
		pStr[0] = 0;*/

	if (!_pFileName)
		M_ASSERT(0, "?");

#ifdef M_SUPPORTMEMORYDEBUG
	void* pAddr = MACRO_GetDefaultMemoryManager()->AllocDebug(_nSize, MRTC_NORMAL_BLOCK, _pFileName, _Line);
#else
	void* pAddr = MACRO_GetDefaultMemoryManager()->Alloc(_nSize);
#endif
#ifdef NEVER
	{
		if (!g_MemFailRecursion/* && !_CrtCheckMemory()*/)
		{
			g_MemFailRecursion = true;
	#ifdef MRTC_DLL
			if (MRTC_DLLFindProcess())
	#endif
			{
				M_TRACE(CFStrF("Alloc %.8x, Size %d, %s (%d)\n", pAddr, _nSize, _pFileName, _Line));
//				LogFile(CStrF("Alloc %.8x, Size %d, %s (%d)", pAddr, _nSize, _pFileName, _Line));
			}
			g_MemFailRecursion = false;
		}
	}
#endif
	return pAddr;
}

void M_CDECL operator delete(void* p, const char* _pFileName, int _Line)
{
/*	if (!g_MemFailRecursion && !_CrtCheckMemory())
	{
		g_MemFailRecursion = true;
		LogFile(CStrF("(::operator new) Memory check failed before deallocating block %.8x bytes in file %s, line %d", p, _pFileName, _Line));
		Error_static("::operator new", CStrF("Memory check failed before deallocating block %.8x bytes in file %s, line %d", p, _pFileName, _Line));
		g_MemFailRecursion = false;
	}*/
//	OutputDebugString(CFStrF("Free %.8x, %s (%d)\n", p, _pFileName, _Line));
	MACRO_GetDefaultMemoryManager()->Free(p);
}

void* operator new[](mint _nSize, const char* _pFileName, int _Line)
{
#ifdef M_SUPPORTMEMORYDEBUG
	return MACRO_GetDefaultMemoryManager()->AllocDebug(_nSize, MRTC_NORMAL_BLOCK, _pFileName, _Line);
#else
	return MACRO_GetDefaultMemoryManager()->Alloc(_nSize);
#endif
}

void M_CDECL operator delete[](void* p, const char* _pFileName, int _Line)
{
/*	if (!g_MemFailRecursion && !_CrtCheckMemory())
	{
		g_MemFailRecursion = true;
		LogFile(CStrF("(::operator new) Memory check failed before deallocating block %.8x bytes in file %s, line %d", p, _pFileName, _Line));
		Error_static("::operator new", CStrF("Memory check failed before deallocating block %.8x bytes in file %s, line %d", p, _pFileName, _Line));
		g_MemFailRecursion = false;
	}*/
//	OutputDebugString(CFStrF("Free %.8x, %s (%d)\n", p, _pFileName, _Line));
	MACRO_GetDefaultMemoryManager()->Free(p);
}

#endif



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Placement new / delete
|__________________________________________________________________________________________________
\*************************************************************************************************/

#if defined(COMPILER_PLACEMENT_NEW) && !defined(__PLACEMENT_NEW_INLINE)


	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Function:			placement new operator
	\*____________________________________________________________________*/
	void* operator new(mint size, void* p)
	{
//		LogFile(CStrF("::operator new(%d, %.8x)", size, p));
		return p;
	}


	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Function:			placement delete operator
	\*____________________________________________________________________*/
	void operator delete(void* p1, void* p2)
	{
//		LogFile(CStrF("::operator delete(%.8x, %.8x)", p1, p2));
	}

#endif
// -------------------------------------------------------------------


int gf_FindObject(MRTC_CRuntimeClass *_pRuntime, const char *_pObject)
{
	MRTC_CRuntimeClass *pRunTimeClass = _pRuntime;
	while (pRunTimeClass)
	{
		if (pRunTimeClass->m_ClassName == _pObject)
			return true;
		pRunTimeClass = pRunTimeClass->m_pClassBase;
	}
#ifdef _DEBUG
//	M_TRACEALWAYS("Dynamic cast of %s to %s failed\n", _pRuntime->m_ClassName, _pObject);
#endif
	return false;
}

#ifdef NEVER

void CPtrBase::Construct(CPtrBase *From)
{
	p = From->p;
	
	if (p != NULL)
		p->MRTC_AddRef();
	
};

void CPtrBase::Construct(CReferenceCount *From)
{
	p = From;
	if (p != NULL) 
		p->MRTC_AddRef();
	
};

void CPtrBase::Construct()
{
	p = NULL;
};

void CPtrBase::Destruct()
{
	if (p)
	{
		if (! p->MRTC_DelRef()) 
		{ 
			p->MRTC_Delete(); 
			p = NULL; 
		};
	}
	
};

void CPtrBase::Del()
{
	MCC_TPtrError("TPtr<?>::operator delete", "Delete is not allowed on smart-pointers.");
};

void CPtrBase::CheckP()
{
	if (!p) 
		MCC_TPtrError("TPtr<?>::operator *", "NULL pointer.");
};

void CPtrBase::Assign(const CPtrBase& _p)
{
	if (_p.p)
		_p.p->MRTC_AddRef();
	
	if (p)
		if (p->MRTC_DelRef() == 0)
		{
			p->MRTC_Delete();
			p = NULL;
		};
		
		p = _p.p;
		
}

void CPtrBase::Assign(CReferenceCount* _p)
{
	if (_p != NULL) 
		_p->MRTC_AddRef();
	
	if (p != NULL) 
	{
		if (p->MRTC_DelRef() == 0)  
		{ 
			p->MRTC_Delete(); 
			p = NULL; 
		};
	}
	
	p = _p;
}

#endif

CObj::CObj()
{
#if !defined(M_RTM) || defined(MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCLASSES)
	m_pRunTimeClass = NULL;
	m_pClassName = NULL;
#endif
}

CObj::~CObj()
{
#ifdef MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCLASSES
	if (m_pRunTimeClass && MRTC_GetRD() && (MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_CObjAllocations))
	{
		uint8 Data[4096];
		uint8 *pData = Data;

		mint Len = strlen(m_pClassName)+1;
		*((uint32 *)pData) = Len; SwapLE(*((uint32 *)pData)); pData += sizeof(uint32);
		memcpy(pData, m_pClassName, Len); pData += Len;
		Len = strlen(m_pRunTimeClass->m_ClassName)+1;
		*((uint32 *)pData) = Len; SwapLE(*((uint32 *)pData)); pData += sizeof(uint32);
		memcpy(pData, m_pRunTimeClass->m_ClassName, Len); pData += Len;
		MRTC_GetRD()->SendData(ERemoteDebug_RunTimeClassDestructor, Data, pData - Data, false, false);
	}
#endif
}


#if !defined(M_RTM) || defined(MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCLASSES)
void CObj::PostConstruct(const char *_pClassName)
{
	m_pRunTimeClass = MRTC_GetRuntimeClass();
	m_pClassName = _pClassName;
#ifdef MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCLASSES
	if (MRTC_GetRD() && (MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_CObjAllocations))
	{
		uint8 Data[4096];
		uint8 *pData = Data;

		mint Len = strlen(_pClassName)+1;
		*((uint32 *)pData) = Len; SwapLE(*((uint32 *)pData)); pData += sizeof(uint32);
		memcpy(pData, _pClassName, Len); pData += Len;
		Len = strlen(m_pRunTimeClass->m_ClassName)+1;
		*((uint32 *)pData) = Len; SwapLE(*((uint32 *)pData)); pData += sizeof(uint32);
		memcpy(pData, m_pRunTimeClass->m_ClassName, Len); pData += Len;
		MRTC_GetRD()->SendData(ERemoteDebug_RunTimeClassConstructor, Data, pData - Data, false, false);		
	}
#endif
}
#endif


const char* CObj::MRTC_ClassName() const 
{
	return MRTC_GetRuntimeClass()->m_ClassName;
};

// -------------------------------------------------------------------
CReferenceCount::CReferenceCount()
{
	m_MRTC_ReferenceCount = 0;
	m_MRTC_bDynamic = 0;
};

#if 0

#ifdef _DEBUG

	void* CReferenceCount::operator new(mint _Size, const char* _pFileName, int _Line)
	{
		return ::new (_pFileName, _Line)char[_Size];
	}

	void* CReferenceCount::operator new(mint _Size)
	{
		return MACRO_GetDefaultMemoryManager()->Alloc(_Size);
	}
#ifndef COMPILER_GNU_2
	void CReferenceCount::operator delete(void *p, const char* _pFileName, int _Line)
	{
		MACRO_GetDefaultMemoryManager()->Free(p);
	}
#endif	// COMPILER_GNU
#else

	#ifdef MRTC_MEMORYDEBUG
		void* CReferenceCount::operator new(mint _Size, const char* _pFn, const char* _pFile, int _Line)
		{
			return ::new char[_Size];
		}
	#endif

#if ( !defined( M_RTM ) && defined( PLATFORM_CONSOLE ) ) || !defined( PLATFORM_CONSOLE )
	void* CReferenceCount::operator new(mint _Size, const char* _pFileName, int _Line)
	{
		return MACRO_GetDefaultMemoryManager()->Alloc(_Size);
	}
#endif

	void* CReferenceCount::operator new(mint _Size)
	{
		return MACRO_GetDefaultMemoryManager()->Alloc(_Size);
	}
		

#endif
#ifndef	COMPILER_GNU_2
	void CReferenceCount::operator delete(void* p)
	{
		MACRO_GetDefaultMemoryManager()->Free(p);
	}

	void CReferenceCount::operator delete(void *, void* _pAddr)
	{
	}
#endif	// COMPILER_GNU

	void* CReferenceCount::operator new(mint, void* p)
	{
		return p;
	}

/*#ifdef COMPILER_NEEDOPERATORDELETE
	void CReferenceCount::operator delete(void*, void*)
	{
		M_ASSERT(0, "?");
	}
#endif
*/


#endif


void CVirtualRefCount::MRTC_Delete()
{
	delete this;
}

void CReferenceCount::MRTC_Delete()
{
	if(this)
	{
		if (MRTC_IsDynamic())
			MRTC_GetRuntimeClass()->DelInstance();
		delete this;
	}
};


CReferenceCount::operator TPtr<CReferenceCount> ()
{									
	CReferenceCount* pT = this;					
	TPtr<CReferenceCount> spT = pT;				
	return spT;						
}

void CReferenceCount::operator =(const CReferenceCount& _obj) {};
CReferenceCount::CReferenceCount(const CReferenceCount& obj) {};

// -------------------------------------------------------------------
//  The ONE classcontainer for this executable.
// -------------------------------------------------------------------
MRTC_ClassContainer g_ClassContainer = { 0 };

// -------------------------------------------------------------------
//  Implement MRTC for CObj & CReferenceCount
// -------------------------------------------------------------------
#ifdef M_RTM
MRTC_CRuntimeClass CObj::m_RuntimeClass = {"CObj", NULL, NULL, 0};
#else
MRTC_CRuntimeClass CObj::m_RuntimeClass = {"CObj", NULL, NULL, 0, 0};
#endif

MRTC_CClassInit g_ClassRegCObj(&CObj::m_RuntimeClass);

MRTC_CRuntimeClass* CObj::MRTC_GetRuntimeClass() const
{ 
	return &m_RuntimeClass;
};

MRTC_IMPLEMENT(CReferenceCount, CObj);

#ifndef COMPILER_CODEWARRIOR
#endif

// -------------------------------------------------------------------
//  MRTC_CRuntimeClass
// -------------------------------------------------------------------
#if 0
#ifdef M_RTM
MRTC_CRuntimeClass::MRTC_CRuntimeClass(char* _ClassName, int _Size, int _Version, CReferenceCount* (*_pfnCreateObject)(), MRTC_CRuntimeClass* _pClassBase)
#else
MRTC_CRuntimeClass::MRTC_CRuntimeClass(char* _ClassName, int _Size, int _Version, CReferenceCount* (*_pfnCreateObject)(), MRTC_CRuntimeClass* _pClassBase, const char *File, int Line)
#endif
:	m_ClassName(_ClassName),
	m_Size(_Size),
	m_Version(_Version),
	m_pfnCreateObject(_pfnCreateObject),
	m_pClassBase(_pClassBase),
	m_nDynamicInstances(0),
	m_pClassLess(NULL),
	m_pClassGreater(NULL)

{

/*
#ifndef M_RTM
	if (_pClassBase)
		M_TRACEALWAYS("%s(%d) : RC %s : %s\n", File, Line, _ClassName, (const char *)_pClassBase->m_ClassName);
	else
		M_TRACEALWAYS("%s(%d) : RC %s : None\n",File, Line, _ClassName);
#endif
*/
/*
	for (int i = 0; i < sizeof(MRTC_CRuntimeClass); ++i)
	{
		OutputDebugString(CStrF("%x", (int)((char *)this)[i]));
	}

	OutputDebugString("\n");*/
}
#endif




void CreateCObjCommon(CReferenceCount* _pObj, MRTC_CRuntimeClass *_pClass)
{
	if (_pObj) 
	{
		_pClass->AddInstance(); 
		_pObj->MRTC_SetDynamic(); 
	}
}


#if !defined(M_RTM) || defined(MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCLASSES)

	int MRTC_CRuntimeClass::AddInstance()
	{
		#ifdef MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCLASSES
			if (MRTC_GetRD() && (MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_RuntimeClassAllocations))
				MRTC_GetRD()->SendData(ERemoteDebug_RunTimeClassCreate, m_ClassName, strlen(m_ClassName)+1, false, false);
		#endif

		#ifndef M_RTM
			return m_nDynamicInstances++;
		#else
			return 0;
		#endif
	}

	int MRTC_CRuntimeClass::DelInstance()
	{
		#ifdef MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCLASSES
			if (MRTC_GetRD() && (MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_RuntimeClassAllocations))
				MRTC_GetRD()->SendData(ERemoteDebug_RunTimeClassDestroy, m_ClassName, strlen(m_ClassName)+1, false, false);
		#endif

		#ifndef M_RTM
			m_nDynamicInstances--;
		//	if (m_nDynamicInstances >= -1 && strcmp(m_ClassName, "CReferenceCount") == 0)
		//		OutputDebugString(CFStrF("(MRTC_CRuntimeClass::DelInstance) %s, %d\n", m_ClassName, m_nDynamicInstances));
			return m_nDynamicInstances;
		#else
			// M_RTM
			return 0;
		#endif 
	}
#endif // !defined(M_RTM) || defined(MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCLASSES)



MRTC_CClassInit::MRTC_CClassInit(MRTC_CRuntimeClass* _pRTC)
{
	_pRTC->m_ClassNameHash = StringToHash(_pRTC->m_ClassName);

//	OutputDebugString(CStrF("0x%x %s\n", &g_ClassContainer, _pRTC->m_ClassName));
//	M_TRACEALWAYS("MRTC_CClassInit %s\n", _pRTC->m_ClassName);
#ifdef _DEBUG
	MRTC_CRuntimeClass* pCurrent = _pRTC;
	int MaxDepth = 20;
	while (pCurrent)
	{
		if (!(--MaxDepth))
		{
			M_BREAKPOINT; // Circular reference in class
		}
		pCurrent = pCurrent->m_pClassBase;
	}
#endif

#ifdef MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCLASSES
	
	if (MRTC_GetRD() && (MRTC_GetRD()->m_EnableFlags & ERDEnableFlag_RuntimeClasses))
	{
		uint8 Data[4096];
		uint8 *pData = Data;

		mint Len = strlen(_pRTC->m_ClassName)+1;
		*((uint32 *)pData) = Len; SwapLE(*((uint32 *)pData)); pData += sizeof(uint32);
		memcpy(pData, _pRTC->m_ClassName, Len); pData += Len;
		if (_pRTC->m_pClassBase && _pRTC->m_pClassBase->m_ClassName)
		{
			Len = strlen(_pRTC->m_pClassBase->m_ClassName)+1;
			*((uint32 *)pData) = Len; SwapLE(*((uint32 *)pData)); pData += sizeof(uint32);
			memcpy(pData, _pRTC->m_pClassBase->m_ClassName, Len); pData += Len;
		}
		else
		{
			*((uint32 *)pData) = 0; SwapLE(*((uint32 *)pData)); pData += sizeof(uint32);
		}
		MRTC_GetRD()->SendData(ERemoteDebug_RunTimeClassRegister, Data, pData - Data, false, false);
	}

#endif

	g_ClassContainer.Insert(_pRTC);
}



#if 0 // Not used any more
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_ClassContainer
|__________________________________________________________________________________________________
\*************************************************************************************************/
void MRTC_ClassContainer::Insert_r(MRTC_CRuntimeClass* _pNode, MRTC_CRuntimeClass* _pRTC)
{
	int nCmp = CStrBase::stricmp(_pRTC->m_ClassName, _pNode->m_ClassName);
	if (!nCmp)
		Error_static("MRTC_ClassContainer::Insert_r", "Internal error. (Class allready registered.");

	if (nCmp < 0)
	{
		if (_pNode->m_pClassLess)
			Insert_r(_pNode->m_pClassLess, _pRTC);
		else
			_pNode->m_pClassLess = _pRTC;
	}
	else
	{
		if (_pNode->m_pClassGreater)
			Insert_r(_pNode->m_pClassGreater, _pRTC);
		else
			_pNode->m_pClassGreater = _pRTC;
	}
}

MRTC_CRuntimeClass* MRTC_ClassContainer::Find_r(MRTC_CRuntimeClass* _pNode, const char* _pName)
{
	if (!_pNode) return NULL;

	int nCmp = CStrBase::stricmp(_pName, _pNode->m_ClassName);
	if (!nCmp) return _pNode;

	if (nCmp < 0)
	{
		if (_pNode->m_pClassLess)
			return Find_r(_pNode->m_pClassLess, _pName);
		else
			return NULL;
	}
	else
	{
		if (_pNode->m_pClassGreater)
			return Find_r(_pNode->m_pClassGreater, _pName);
		else
			return NULL;
	}

	Error_static("MRTC_ClassContainer::Find_r", "Internal error.");
	return 0;
}

void MRTC_ClassContainer::FindPrefix_r(MRTC_CRuntimeClass* _pNode, const char *_pName, int _iLen, MRTC_CRuntimeClass **_pList, int _MaxSize, int &_Pos)
{
	if (!_pNode) return;

	int nCmp = strncmp(_pName, _pNode->m_ClassName, _iLen);
	if (!nCmp)
	{
		if (_pNode->m_pClassLess)
			FindPrefix_r(_pNode->m_pClassLess, _pName, _iLen, _pList, _MaxSize, _Pos);

		if(_Pos < _MaxSize)
			_pList[_Pos++] = _pNode;
		
		if (_pNode->m_pClassGreater)
			FindPrefix_r(_pNode->m_pClassGreater, _pName, _iLen, _pList, _MaxSize, _Pos);
		return;
	}

	if (nCmp < 0)
	{
		if (_pNode->m_pClassLess)
			FindPrefix_r(_pNode->m_pClassLess, _pName, _iLen, _pList, _MaxSize, _Pos);
	}
	else
	{
		if (_pNode->m_pClassGreater)
			FindPrefix_r(_pNode->m_pClassGreater, _pName, _iLen, _pList, _MaxSize, _Pos);
	}
}

void MRTC_ClassContainer::Dump_r(MRTC_CRuntimeClass* _pRTC, int _Flags)
{
	if (!_pRTC) return;

	// This way we'll get the classes in sorted order.
	Dump_r(_pRTC->m_pClassLess, _Flags);
	if (!(_Flags & 1) || (_pRTC->m_nDynamicInstances > 0))
		LogFile(CStrF("        Class '%s',  Parent '%s',  DynamicInstances %d", _pRTC->m_ClassName, (_pRTC->m_pClassBase) ? _pRTC->m_pClassBase->m_ClassName : "N/A", _pRTC->m_nDynamicInstances));
	Dump_r(_pRTC->m_pClassGreater, _Flags);
}
#endif


void MRTC_ClassContainer::Insert(MRTC_CRuntimeClass* _pRTC)
{
	// We need to construct it first
	_pRTC->m_Link.f_Construct();
	m_ClassTree.f_Insert(_pRTC);
}

#ifndef M_RTM
void MRTC_ClassContainer::Dump(int _Flags)
{
	CClassIter Iter = m_ClassTree;
	while (Iter)
	{
		if (!(_Flags & 1) || (Iter->m_nDynamicInstances > 0))
		{
			LogFile(CStrF("        Class '%s',  Parent '%s',  DynamicInstances %d", Iter->m_ClassName, (Iter->m_pClassBase) ? Iter->m_pClassBase->m_ClassName : "N/A", Iter->m_nDynamicInstances));
		}
		++Iter;
	}
}
#endif

MRTC_CRuntimeClass* MRTC_ClassContainer::Find(const char* _pName)
{
	return m_ClassTree.FindEqual(_pName);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_CClassRegistry
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_CClassRegistry::MRTC_CClassRegistry()
{
	m_pFirstCL = NULL;
	m_pFirstContainer = NULL;
/*	m_pFirstContainer = new MRTC_ClassContainerLink;
	if (!m_pFirstContainer) MemError("MRTC_CClassRegistry");

	m_pFirstContainer->m_pClassContainer = NULL;
	m_pFirstContainer->m_pNext = NULL;*/
}

MRTC_CClassRegistry::~MRTC_CClassRegistry()
{
	while(m_pFirstContainer) RemoveClassContainer(m_pFirstContainer->m_pClassContainer);
}

void MRTC_CClassRegistry::AddClassContainer(MRTC_ClassContainer* _pClassContainer)
{
	MRTC_ClassContainerLink* pLink = DNew(MRTC_ClassContainerLink) MRTC_ClassContainerLink;
	if (!pLink) MemError("AddClassContainer");

	pLink->m_pClassContainer = _pClassContainer;
	pLink->m_pNext = m_pFirstContainer;	
	m_pFirstContainer = pLink;

//	_pClassContainer->Dump_r(_pClassContainer->m_pClassRoot, 0);
}

void MRTC_CClassRegistry::RemoveClassContainer(MRTC_ClassContainer* _pClassContainer)
{
//	_pClassContainer->Dump_r(_pClassContainer->m_pClassRoot, 1);

	MRTC_ClassContainerLink* pLink = m_pFirstContainer;

	if (pLink->m_pClassContainer == _pClassContainer)
	{
		m_pFirstContainer = pLink->m_pNext;
		delete pLink;
		return;
	}

	MRTC_ClassContainerLink* pLast = pLink;
	pLink = pLink->m_pNext;
	while(pLink)
	{
		if (pLink->m_pClassContainer == _pClassContainer)
		{
			pLast->m_pNext = pLink->m_pNext;
			delete pLink;
			return;
		}
		pLast = pLink;
		pLink = pLink->m_pNext;
	}

	Error_static("MRTC_CClassRegistry::RemoveClassContainer", "Container not in class-registry.");
}

MRTC_ClassLibraryInfo* MRTC_CClassRegistry::FindClassLibrary(char* _pName)
{
	MRTC_ClassLibraryInfo* pCL = m_pFirstCL;
	while(pCL)
	{
		if (CStrBase::stricmp(pCL->m_pName, _pName) == 0) return pCL;
		pCL = pCL->m_pNextCL;
	}
	return NULL;
}

CReferenceCount* MRTC_CClassRegistry::CreateObject(const char* _pName)
{
	MRTC_CRuntimeClass* pRTC = GetRuntimeClass(_pName);

	// Found?
	if (!pRTC) 
	{
		return NULL;
	}

	// Dynamic create?
	if (!pRTC->m_pfnCreateObject) 
	{
		return NULL;
	}

	// Create object
	return (CReferenceCount*)pRTC->m_pfnCreateObject();
}

// -------------------------------------------------------------------
void MRTC_CClassRegistry::LoadClassLibrary(char* _pName)
{
#ifdef PLATFORM_WIN_PC

	LogFile(CStrF("(MRTC_CClassRegistry::LoadClassLibrary) %s", _pName));

	if (MRTC_ClassLibraryInfo* pCL = FindClassLibrary(_pName))
	{
		pCL->m_nRef++;
	}
	else
	{
//		OutputDebugString(CStrF("Load Class Lib %s\n", _pName));

		HINSTANCE hInst = LoadLibraryA(_pName);

		if (!hInst) 
			Win32Err_static("MRTC_CClassRegistry::LoadClassLibrary");
		
		void(*pfnDLLAttachProcess)(MRTC_ObjectManager* _pObjMgr) = 
			(void(*)(MRTC_ObjectManager*)) GetProcAddress(hInst, "MRTC_DLLAttachProcess");
		void(*pfnDLLDetachProcess)(MRTC_ObjectManager* _pObjMgr) = 
			(void(*)(MRTC_ObjectManager*)) GetProcAddress(hInst, "MRTC_DLLDetachProcess");
		MRTC_ClassContainer* (*pfnGetClassContainer)() = 
			(MRTC_ClassContainer* (*)()) GetProcAddress(hInst, "MRTC_GetClassContainer");

		if (!pfnGetClassContainer || !pfnDLLAttachProcess || !pfnDLLDetachProcess)
		{
			FreeLibrary(hInst);
			Error_static("MRTC_CClassRegistry::LoadClassLibrary", CStrF("Not a valid MRTC class-library DLL. (%s)", _pName));
		}

		pCL = DNew(MRTC_ClassLibraryInfo) MRTC_ClassLibraryInfo;
		if (!pCL)
		{
			FreeLibrary(hInst);
			Error_static("MRTC_CClassRegistry::LoadClassLibrary", "Out of memory.");
		}

		pCL->m_pName = DNew(char) char[strlen(_pName)+1];
		if (!pCL->m_pName)
		{
			delete pCL;
			FreeLibrary(hInst);
			Error_static("MRTC_CClassRegistry::LoadClassLibrary", "Out of memory.");
		}
		strcpy(pCL->m_pName, _pName);
		pCL->m_Handle = hInst;
		pCL->m_nRef = 1;
		pCL->m_pfnGetClassContainer = pfnGetClassContainer;
		pCL->m_pfnDLLAttachProcess = pfnDLLAttachProcess;
		pCL->m_pfnDLLDetachProcess = pfnDLLDetachProcess;

		MRTC_ClassContainer* pCC = pCL->m_pfnGetClassContainer();
		if (!pCC) 
		{
			FreeLibrary(hInst);
			delete pCL; pCL = NULL;
			Error_static("MRTC_CClassRegistry::LoadClassLibrary", "DLL provided an invalid class-container.");
		}

		LogFile("-------------------------------------------------------------------");
		LogFile(CStrF("(ADDING) %s, Dynamic classes:", _pName));
		AddClassContainer(pCC);
		LogFile("-------------------------------------------------------------------");

		pCL->m_pNextCL = m_pFirstCL;
		m_pFirstCL = pCL;

		pCL->m_pfnDLLAttachProcess(MRTC_GetObjectManager());
	}

	LogFile(CStr("(MRTC_CClassRegistry::LoadClassLibrary) Done."));

#else
	Error_static("MRTC_CClassRegistry::LoadClassLibrary", "Not supported on this platform.");
#endif
}

void MRTC_CClassRegistry::UnloadClassLibrary(char* _pName)
{
#ifdef PLATFORM_WIN_PC
	LogFile(CStrF("(MRTC_CClassRegistry::UnloadClassLibrary) %s", _pName));

	if (MRTC_ClassLibraryInfo* pCL = FindClassLibrary(_pName))
	{
		pCL->m_nRef--;
		if (!pCL->m_nRef)
		{
//			OutputDebugString(CStrF("Unload Class Lib %s\n", _pName));

			pCL->m_pfnDLLDetachProcess(MRTC_GetObjectManager());

			LogFile("-------------------------------------------------------------------");
			LogFile(CStrF("(REMOVING) %s, Dynamic classes:", _pName));
			RemoveClassContainer(pCL->m_pfnGetClassContainer());
			LogFile("-------------------------------------------------------------------");
			if (m_pFirstCL == pCL)
			{
				m_pFirstCL = pCL->m_pNextCL;
				pCL->m_pNextCL = NULL;
			}
			else
			{
				MRTC_ClassLibraryInfo* pPrev = m_pFirstCL;
				while((pPrev->m_pNextCL != pCL) && (pPrev->m_pNextCL != NULL)) pPrev = pPrev->m_pNextCL;
				if (!pPrev) Error_static("MRTC_CClassRegistry::UnloadClassLibrary", "Internal error.");

				pPrev->m_pNextCL = pCL->m_pNextCL;
				pCL->m_pNextCL = NULL;
			}

			FreeLibrary((HINSTANCE__*)pCL->m_Handle);
			delete pCL;
		}
	}
	else
		Error_static("MRTC_CClassRegistry::UnloadClassLibrary", CStrF("Invalid class-library. (%s)", _pName));

	LogFile(CStr("(MRTC_CClassRegistry::UnloadClassLibrary) Done."));

#else
	Error_static("MRTC_CClassRegistry::UnloadClassLibrary", "Not supported on this platform.");
#endif
}

MRTC_CRuntimeClass* MRTC_CClassRegistry::GetRuntimeClass(const char* _pName)
{
	// Loop through all class-containers for class *_pName.
	MRTC_ClassContainerLink* pLink = m_pFirstContainer;
	while(pLink)
	{
		MRTC_CRuntimeClass* pRTC = pLink->m_pClassContainer->Find(_pName);
		if (pRTC)
		{
#ifdef MRTC_AUTOSTRIPLOGGER
			// log which Runtime classes are actually used..
			static CUsageLogger* s_pRTCLogger = NULL;
			if (!s_pRTCLogger)
				s_pRTCLogger = DNew(CUsageLogger) CUsageLogger("<<<MRTC-Logger>>> name:", 1500);
			s_pRTCLogger->Log(_pName);
#endif		
			return pRTC;
		}
		pLink = pLink->m_pNext;
	}
	return NULL;
}

int MRTC_CClassRegistry::FindPrefix(const char *_pName, MRTC_CRuntimeClass **_pList, int _MaxSize)
{
	int Pos = 0;
	mint Len = strlen(_pName);
	if(Pos >= _MaxSize)
		return Pos;

	// Loop through all class-containers for class *_pName.
	MRTC_ClassContainerLink* pLink = m_pFirstContainer;
	while(pLink)
	{		
		MRTC_ClassContainer::CClassIter Iter = pLink->m_pClassContainer->m_ClassTree;

//		if (Iter.FindSmallestGreaterThanEqual(_pName, pLink->m_pClassContainer->m_ClassTree))
		{
			while (Iter)
			{
				MRTC_CRuntimeClass *pIter = Iter;
//				M_TRACE("%s\n", pIter->m_ClassName);

				if (CStrBase::strnicmp(pIter->m_ClassName, _pName, Len) == 0)
				{
					_pList[Pos++] = pIter;
					if(Pos >= _MaxSize)
						return Pos;
				}

				++Iter;
			}
		}
			
		pLink = pLink->m_pNext;
	}

	return Pos;
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_ContextStack
|__________________________________________________________________________________________________
\*************************************************************************************************/

// disable context stack until we have fixed threadsafe cs
#ifdef MRTC_ENABLE_MSCOPE
#  define MRTC_CONTEXTENABLE
//#  define MRTC_CONTEXTENABLE_CHECKTHREAD
#  define MRTC_CONTEXTENABLE_MEMUSAGE
#  define MRTC_CONTEXTENABLE_CONTEXTSTACK

#  if defined(PLATFORM_PS2)
#    undef MRTC_CONTEXTENABLE_CHECKTHREAD
#    if __ide_target("PS2 Profiler") || __ide_target("PS2 Profiler NoPCH")
#      undef MRTC_CONTEXTENABLE_MEMUSAGE
#      undef MRTC_CONTEXTENABLE_CONTEXTSTACK
#    endif
#  endif
#endif

#ifdef MRTC_ENABLE_MSCOPE

void MRTC_Context::Construct(const char* _pCtx)
{
 #ifdef MRTC_CONTEXTENABLE

  #ifdef MRTC_CONTEXTENABLE_MEMUSAGE
	m_MemUsedCreated = MRTC_MemUsed();
  #endif

	static MRTC_ObjectManager* pObjMgr = MRTC_GetObjectManager();

	MRTC_CallGraph* pCallGraph = pObjMgr->m_pCallGraph;
	MRTC_CallGraph_ThreadLocalData* pThreadLocal = pCallGraph->m_ThreadLocal.Get();

	if (pThreadLocal->m_State && !pThreadLocal->m_nDisable)
	{
		int64 Time = MRTC_SystemInfo::CPU_Clock();

		m_Clocks = -Time;
		m_ClocksWaste = -Time;	
		pThreadLocal->Push(_pCtx);
		
	 #ifdef MRTC_CONTEXTENABLE_CONTEXTSTACK
		MRTC_ContextStack *pContextStack = pObjMgr->GetContexctStack().Get();
		pContextStack->PushContext(this);
	 #endif

		pThreadLocal->m_Depth++;
		m_ClocksWaste += MRTC_SystemInfo::CPU_Clock();
	}
	else
	{
		pThreadLocal->m_Depth++;

	 #ifdef MRTC_CONTEXTENABLE_CONTEXTSTACK
		MRTC_ContextStack *pContextStack = pObjMgr->GetContexctStack().Get();
		pContextStack->PushContext(this);
	 #endif
	}
 #endif
}
#endif


MRTC_Context::~MRTC_Context()
{
#ifdef MRTC_CONTEXTENABLE
	static MRTC_ObjectManager* pObjMgr = MRTC_GetObjectManager();
	MRTC_CallGraph* pCallGraph = pObjMgr->m_pCallGraph;
	MRTC_CallGraph_ThreadLocalData* pThreadLocal = pCallGraph->m_ThreadLocal.Get();
	if (pThreadLocal->m_State & MRTC_CG_ISRUNNING && !pThreadLocal->m_nDisable)
	{
		int64 Time = MRTC_SystemInfo::CPU_Clock();

		MRTC_CallGraphEntry* pE = pThreadLocal->m_pCurrent;
		pE->m_nCalls++;
		m_ClocksWaste -= Time;

	 #ifdef MRTC_CONTEXTENABLE_CONTEXTSTACK
		MRTC_ContextStack *pContextStack = pObjMgr->GetContexctStack().Get();
		pContextStack->PopContext(this);
	 #endif

		pThreadLocal->Pop(0);
		pThreadLocal->m_Depth--;

		Time = MRTC_SystemInfo::CPU_Clock();
		m_ClocksWaste += Time;
		m_ClocksWaste += pCallGraph->m_WasteCorrectionClocks;
		m_Clocks += Time;

		if (m_ClocksWaste > m_Clocks)
			m_ClocksWaste = m_Clocks;

		pE->m_ClocksWasted += m_ClocksWaste;
		pE->m_Clocks += m_Clocks;

//		m_ClocksWaste += GetCPUClock();
	}
	else
	{
		pThreadLocal->m_Depth--;

	 #ifdef MRTC_CONTEXTENABLE_CONTEXTSTACK
		MRTC_ContextStack *pContextStack = pObjMgr->GetContexctStack().Get();
		pContextStack->PopContext(this);
	 #endif
	}
 #endif
}

// -------------------------------------------------------------------
MRTC_ContextStack::MRTC_ContextStack()
{
	m_iContext = 0;
}

CStr MRTC_ContextStack::GetContextStack()
{
	CStr s;
#ifdef MRTC_CONTEXTENABLE
	int nCtx = m_iContext;
	for(int i = 0; i < nCtx; i++)
	{
		if (s != "")
			s += "->";
		s += m_lpContexts[i]->m_pName;
	}
#endif
	return s;
}

CStr MRTC_ContextStack::GetLastContextStack()
{
#ifdef MRTC_CONTEXTENABLE
	return CStr(m_lpContexts[m_iContext-1]->m_pName);
#else
	return CStr();
#endif
}

void MRTC_ContextStack::PushContext(MRTC_Context* _pCtx)
{
	if (m_iContext >= MRTC_CONTEXT_STACK_DEPTH)
		Error("PushContext", "Stack overrun.");
	m_lpContexts[m_iContext] = _pCtx;
	m_iContext++;
}

void MRTC_ContextStack::PopContext(MRTC_Context* _pCtx)
{
#ifdef MRTC_LOGCONTEXTMEMORY
	int Used = MRTC_MemUsed();
	int dUsed= _pCtx->m_MemUsedCreated - Used;
	if (dUsed != 0)
	{
		CStr Context = GetContextStack();

		int Spaces = (m_iContext-1)*4;

		char Buffer[1024];
		memset(Buffer, ' ', Spaces);
		snprintf(&Buffer[Spaces], sizeof(Buffer)-Spaces, "(%d) Context delta mem %d, Used %d, %s\n", 
			g_iOutputDebugString, dUsed, Used, Context.Str());
		OutputDebugString(Buffer);
		g_iOutputDebugString++;
	}
#endif

	m_iContext--;
	if (m_lpContexts[m_iContext] != _pCtx)
		Error("PopContext", "Invalid context pop.");
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_ThreadPoolManager
|__________________________________________________________________________________________________
\*************************************************************************************************/
#include "MRTC_VPUManager.h"

class MRTC_ThreadPoolJob
{
public:
	volatile int m_nObj;
	void* m_pArg;
	MRTC_ThreadJobFunction m_pfnFunction;

	void Clear()
	{
		m_nObj = 0;
		m_pArg = 0;
		m_pfnFunction = 0;
	}
};

class MRTC_ThreadPoolThread : public MRTC_Thread_Core
{
public:
	MRTC_ThreadPoolThread()
	{
		m_ThreadID = 0;
		m_iThread = 0;
	}
	uint32 m_ThreadID;
	uint32 m_iThread;
	class MRTC_ThreadPoolManagerInternal* m_pManager;
	volatile uint32 m_nJobs;
	volatile uint64 m_Time;
	void* m_pSemaphore;
	NThread::CAtomicInt m_Busy;
	const char* Thread_GetName() const
	{
		return "Threadpool Thread";
	}

	virtual int Thread_Main();
};


class MRTC_ThreadPoolManagerInternal
{
	friend class MRTC_ThreadPoolThread;
protected:
	MRTC_ThreadPoolJob m_Job;
	NThread::CAtomicInt m_iJobsTodo;
	NThread::CAtomicInt m_iJobsDone;
//	NThread::CAtomicInt m_nThreadsIdle;
	MRTC_CriticalSection m_Lock;
	void* m_pSemaphore;
	NThread::CEventAutoReset m_JobDone;
	TThinArray<MRTC_ThreadPoolThread> m_lThreads;
	uint16 m_nThreads;
	uint16 m_nActiveThreads;
	const char* m_pName;
public:
	MRTC_VPUManager m_VPUManager;

public:
	void WakeUpThread()
	{
//		if (m_nThreadsIdle.Get() > 0)
			MRTC_SystemInfo::Semaphore_Increase(m_pSemaphore,1);
	}

	MRTC_ThreadPoolManagerInternal() : m_nThreads(0), m_nActiveThreads(0)
	{

		m_AvailableCPUs = 0x0;
		m_UsableCPUs = 0x0;
#ifdef PLATFORM_XENON
		m_AvailableCPUs = M_Bit(0) | M_Bit(1) | M_Bit(2) | M_Bit(3) | M_Bit(4) | M_Bit(5);
		m_UsableCPUs =  M_Bit(1) | M_Bit(2) | M_Bit(3) | M_Bit(4);
#endif
	}
	~MRTC_ThreadPoolManagerInternal()
	{
		Destroy();
	}
	void Create(int _nThreads, int _StackSize = 128*1024, uint32 _Priority = MRTC_THREAD_PRIO_ABOVENORMAL);
	void Destroy();

	NThread::CSpinLock m_SchedulingLock;
	uint32 m_AvailableCPUs;
	uint32 m_UsableCPUs;
	void WaitForThreadStart()
	{
#ifdef PLATFORM_XENON
		TAP_RCD<MRTC_ThreadPoolThread> pThread = m_lThreads;
		for (mint i = 0; i < pThread.Len(); ++i)
		{
			while (Volatile(pThread[i].m_ThreadID) == 0)
			{
				MRTC_SystemInfo::OS_Sleep(1);
			}
		}
#endif
	}
	void UpdateScheduling()
	{
#ifdef PLATFORM_XENON
		uint32 CPUs = m_UsableCPUs & m_AvailableCPUs;
		if (CPUs)
		{
			int iCPU = 0;
			while (!(CPUs & M_BitD(iCPU)))
			{
				++iCPU;
				if (iCPU == 32)
					iCPU = 0;
			}
			TAP_RCD<MRTC_ThreadPoolThread> pThread = m_lThreads;
			for (mint i = 0; i < pThread.Len(); ++i)
			{
				MRTC_SystemInfo::Thread_SetProcessor(pThread[i].m_ThreadID, iCPU);

				++iCPU;
				if (iCPU == 32)
					iCPU = 0;
				while (!(CPUs & M_BitD(iCPU)))
				{
					++iCPU;
					if (iCPU == 32)
						iCPU = 0;
				}
			}
		}
#endif
	}
	void EvacuateCPU(int _PhysicalCPU)
	{
#ifdef PLATFORM_XENON
		DLock(m_SchedulingLock);
		m_AvailableCPUs &= ~(uint32(M_BitD(_PhysicalCPU)));
		UpdateScheduling();
#endif
	}
	void RestoreCPU(int _PhysicalCPU)
	{
#ifdef PLATFORM_XENON
		DLock(m_SchedulingLock);
		m_AvailableCPUs |= (uint32(M_BitD(_PhysicalCPU)));
		UpdateScheduling();
#endif
	}

/*
	void VPU_BlockUntilIdle()
	{
		m_VPUManager.BlockUntilIdle();
	}
*/

	void Enable(bool _bEnable)
	{
		M_LOCK(m_Lock);
		if(_bEnable)
			m_nActiveThreads = m_nThreads;
		else
			m_nActiveThreads = 0;
	}

	int GetNumThreads()
	{
		return m_nThreads;
	}

	uint16 GetActiveThreads()
	{
		return m_nActiveThreads;
	}

	void RunOnEachInstance(int _nObj, void* _pArg, MRTC_ThreadJobFunction _pfnFunction, const char* _pName);

	void SignalJobDone()
	{
		m_JobDone.Signal();
	}

	int DoJob()
	{
		int nObj = m_Job.m_nObj;
		if (m_iJobsTodo.Get() < 1)
			return 0;
		if (m_iJobsDone.Get() == nObj)
			return 0;
		if (m_iJobsDone.Get() > nObj)		
			M_BREAKPOINT;
		int32 iJobsTodo = m_iJobsTodo.Decrease();
		if (iJobsTodo < 1)
		{
			return 0;
		}
		m_Job.m_pfnFunction(nObj-iJobsTodo, m_Job.m_pArg);
		int iJobsDone = m_iJobsDone.Increase();
		if (iJobsDone > nObj-1)
			M_BREAKPOINT;
		if (iJobsDone == nObj-1)
			return -1;
		return 1;
	}
};

void MRTC_ThreadPoolManagerInternal::Create(int _nThreads, int _StackSize, uint32 _Priority)
{
	Destroy();

	m_nThreads = _nThreads;
	m_nActiveThreads = _nThreads;
	if(_nThreads > 1)
	{
		m_iJobsDone.Construct(0);
		m_iJobsTodo.Construct(0);
		m_lThreads.SetLen(_nThreads);
		m_Job.Clear();

		m_pSemaphore = MRTC_SystemInfo::Semaphore_Alloc(0, 1024*1024);
		
		for(int i = 0; i < _nThreads; i++)
		{
			m_lThreads[i].m_iThread = i;
			m_lThreads[i].m_pManager = this;
			m_lThreads[i].m_pSemaphore = m_pSemaphore;
			m_lThreads[i].m_Busy.Exchange(0);
			m_lThreads[i].Thread_Create(NULL, 128*1024, _Priority);
		}
		m_VPUManager.SetMultiThreaded(true);
		WaitForThreadStart();
		UpdateScheduling();
	}
}

void MRTC_ThreadPoolManagerInternal::Destroy()
{
	for(int i = 0; i < m_lThreads.Len(); i++)
	{
		m_lThreads[i].Thread_Destroy();
	}
	m_lThreads.Destroy();
}

void MRTC_ThreadPoolManager::EvacuateCPU(int _PhysicalCPU)
{
	MRTC_ThreadPoolManagerInternal* pMgr = MRTC_GetObjectManager()->m_pThreadPoolManagerInternal;
	if(pMgr)
		pMgr->EvacuateCPU(_PhysicalCPU);
}

void MRTC_ThreadPoolManager::RestoreCPU(int _PhysicalCPU)
{
	MRTC_ThreadPoolManagerInternal* pMgr = MRTC_GetObjectManager()->m_pThreadPoolManagerInternal;
	if(pMgr)
		pMgr->RestoreCPU(_PhysicalCPU);
}

void MRTC_ThreadPoolManager::Create(int _nThreads, int _StackSize, uint32 _Priority)
{
	MRTC_ThreadPoolManagerInternal* pMgr = MRTC_GetObjectManager()->m_pThreadPoolManagerInternal;
	if(pMgr)
		pMgr->Create(_nThreads, _StackSize, _Priority);
}

void MRTC_ThreadPoolManager::Enable(bool _bEnable)
{
	MRTC_ThreadPoolManagerInternal* pMgr = MRTC_GetObjectManager()->m_pThreadPoolManagerInternal;
	if(pMgr)
		pMgr->Enable(_bEnable);
}

int MRTC_ThreadPoolManager::GetNumThreads()
{
	MRTC_ThreadPoolManagerInternal* pMgr = MRTC_GetObjectManager()->m_pThreadPoolManagerInternal;
	if(pMgr)
		return pMgr->GetNumThreads();
	else
		return 0;
}

void MRTC_ThreadPoolManager::ProcessEachInstance(int _nObj, void* _pArg, MRTC_ThreadJobFunction _pfnFunction, const char* _pName, bint _bSync)
{
	if(_nObj == 0)
		return;
	MSCOPESHORT(MRTC_ThreadPoolManager::ProcessEachInstance);
	MRTC_ThreadPoolManagerInternal* pMgr = MRTC_GetObjectManager()->m_pThreadPoolManagerInternal;
	if(!pMgr || pMgr->GetActiveThreads() < 2 || _bSync)
	{
		for(int i = 0; i < _nObj; i++)
			_pfnFunction(i, _pArg);
	}
	else
		pMgr->RunOnEachInstance(_nObj, _pArg, _pfnFunction, _pName);
}


void MRTC_ThreadPoolManagerInternal::RunOnEachInstance(int _nObj, void* _pArg, MRTC_ThreadJobFunction _pfnFunction, const char* _pName)
{
#ifdef THREADPOOL_NAMEDEVENTS
	M_NAMEDEVENT("ThreadPool_DoJob",0xff004400);
#endif
	if(_nObj == 0)
		return;

	M_LOCK(m_Lock);
	m_pName = _pName;
	m_Job.m_nObj = _nObj;
	m_Job.m_pArg = _pArg;
	m_Job.m_pfnFunction = _pfnFunction;
	m_nThreads++;		// Temp-increase number of threads
	M_EXPORTBARRIER;
	m_iJobsDone.Exchange(0);
	m_iJobsTodo.Exchange(_nObj);

	uint32 nJobs = 0;
	uint64 StartTime = CMTimerFuncs_OS::Clock();
	MRTC_SystemInfo::Semaphore_Increase(m_pSemaphore, m_nThreads - 1);
	{
		int32 State = 0;
		while((State = DoJob()) > 0) {nJobs++;}
		if(State == -1)
			SignalJobDone();
	}
	StartTime = CMTimerFuncs_OS::Clock() - StartTime;
	m_JobDone.Wait();
	m_nThreads--;	// Set number of threads back to normal number

	for (uint16 i = 0; i < m_nThreads;i++)
		while (m_lThreads[i].m_Busy.IfEqualExchange(0,0) == 1);

	m_Job.Clear();
	m_pName = NULL;

	static bool bDumpStats = false;
	if(bDumpStats)
	{
		M_TRACEALWAYS("Job '%s' - Size %d\r\n", _pName, _nObj);
		M_TRACEALWAYS("SystemThread did %d jobs in %lu ticks\r\n", nJobs, StartTime);
		for(int i = 0; i < m_nThreads; i++)
		{
			M_TRACEALWAYS("Thread %d did %d jobs in %lu ticks\r\n", i, m_lThreads[i].m_nJobs, m_lThreads[i].m_Time);
		}
	}
}


int MRTC_ThreadPoolThread::Thread_Main()
{
	// Create a 256KiB scratchpad so we have something to work with
	m_ThreadID = MRTC_SystemInfo::Thread_GetCurrentID();
	MRTC_SystemInfo::Thread_SetName(CStrF("ThreadPos%d", m_iThread));
	MRTC_ScratchPadManager::Get(256 * 1024);


	while(!Thread_IsTerminating())
	{
		if (!m_pManager->m_VPUManager.IsTaskWaiting(VpuWorkersContext) &&
			!m_pManager->m_VPUManager.IsTaskWaiting(VpuAIContext))
		{
			MRTC_SystemInfo::Semaphore_WaitTimeout(m_pSemaphore,0.1);
			if(Thread_IsTerminating())
				return 0;
		}
#ifndef PLATFORM_PS3
		{
#ifdef THREADPOOL_NAMEDEVENTS
			M_NAMEDEVENT("ThreadPool_VpuJob",0xff00cc00);
#endif
			bool bHasThreadPoolJobsTodo = m_pManager->m_iJobsTodo.Get()>0;

			while (!bHasThreadPoolJobsTodo && m_pManager->m_VPUManager.IsTaskWaiting(VpuWorkersContext))
			{
				m_pManager->m_VPUManager.RunTask(VpuWorkersContext);
				bHasThreadPoolJobsTodo = m_pManager->m_iJobsTodo.Get()>0;
			}
			while (!bHasThreadPoolJobsTodo && m_pManager->m_VPUManager.IsTaskWaiting(VpuAIContext))
			{
				m_pManager->m_VPUManager.RunTask(VpuAIContext);
				bHasThreadPoolJobsTodo = m_pManager->m_iJobsTodo.Get()>0;
			}
			if (!bHasThreadPoolJobsTodo)
				continue;
		}
#endif
		m_Busy.Increase();
		if (m_pManager->m_iJobsTodo.IfEqualExchange(0,0)!=0)
		{
#ifdef THREADPOOL_NAMEDEVENTS
			M_NAMEDEVENT("ThreadPool_DoJob",0xff004400);
#endif
			M_IMPORTBARRIER;
			uint64 StartTime = CMTimerFuncs_OS::Clock();
			m_nJobs = 0;
			int nState;
			while((nState = m_pManager->DoJob()) > 0) {m_nJobs++;}
			m_Time = CMTimerFuncs_OS::Clock() - StartTime;

			if(nState == -1)
				m_pManager->SignalJobDone();
		}
		m_Busy.Decrease();
	}
	return 0;
}


/*
int MRTC_ThreadPoolThread::Thread_Main()
{
// Create a 256KiB scratchpad so we have something to work with
#ifdef PLATFORM_XBOX

int iCPU = m_pManager->GetCPUCount();
MRTC_SystemInfo::Thread_SetProcessor(iCPU);
MRTC_SystemInfo::Thread_SetName(CStrF("ThreadPool%d", iCPU));
#endif
MRTC_ScratchPadManager::Get(256 * 1024);
m_QuitEvent.ReportTo(&m_Event);

while(!Thread_IsTerminating())
{
m_Event.Wait();
if(Thread_IsTerminating())
break;
#ifndef PLATFORM_PS3
if (m_pManager->m_Job.m_nObj)
{
#endif
//		int iObj = 0;
//		void* pArg = 0;
//		MRTC_ThreadJobFunction pfnFunction;
//		int nJobsLeft = -1;
int nState;
while((nState = m_pManager->DoJob()) > 0) {}
if(nState == -1)
m_pManager->SignalJobDone();


#ifndef PLATFORM_PS3
}
else
{
MRTC_VPUManager& pVPUMgr = m_pManager->m_VPUManager;
while(pVPUMgr.IsTaskWaiting() && m_pManager->m_Job.m_nObj==0)
{
pVPUMgr.RunTask();
}
}
#endif

}

return 0;
}


*/

uint16 MRTC_ThreadPoolManager::VPU_AddTask(const CVPU_JobDefinition& _JobDefinition,VpuContextId _ContextId,bool _Async,uint16 _LinkTaskId)
{
	MRTC_VPUManager& pVPUMgr = MRTC_GetObjectManager()->m_pThreadPoolManagerInternal->m_VPUManager;
	uint16 TaskId = pVPUMgr.AddTask(_JobDefinition,_Async,_ContextId,_LinkTaskId);
#ifndef PLATFORM_PS3
	MRTC_ThreadPoolManagerInternal* pMgr = MRTC_GetObjectManager()->m_pThreadPoolManagerInternal;
	pMgr->WakeUpThread();
#endif
	return TaskId;
}

bool MRTC_ThreadPoolManager::VPU_IsTaskComplete(uint16 _TaskId,VpuContextId _ContextId)
{
	MRTC_VPUManager& pVPUMgr = MRTC_GetObjectManager()->m_pThreadPoolManagerInternal->m_VPUManager;
	return pVPUMgr.IsTaskComplete(_TaskId,_ContextId);
}

void MRTC_ThreadPoolManager::VPU_BlockOnTask(uint16 _TaskId,VpuContextId _ContextId, FVPUCallManager *_pManager, void *_pManagerContext)
{
	MRTC_VPUManager& pVPUMgr = MRTC_GetObjectManager()->m_pThreadPoolManagerInternal->m_VPUManager;
	pVPUMgr.BlockOnTask(_TaskId,_ContextId, _pManager, _pManagerContext);
}

bool MRTC_ThreadPoolManager::VPU_TryBlockUntilIdle(VpuContextId _ContextId)
{
	//	MRTC_VPUManager& pVPUMgr = MRTC_GetObjectManager()->m_pThreadPoolManagerInternal->m_VPUManager;
	//	return pVPUMgr.TryBlockUntilIdle();
	return false;
}

#ifndef PLATFORM_PS3
void MRTC_ThreadPoolManager::VPU_RegisterContext(VpuContextId _ContextId, VpuWorkerFunction _pfnVpuWorker)
{
	MRTC_VPUManager& pVPUMgr = MRTC_GetObjectManager()->m_pThreadPoolManagerInternal->m_VPUManager;
	pVPUMgr.RegisterContext(_ContextId, _pfnVpuWorker);
}
#endif

/*
void MRTC_ThreadPoolManager::VPU_BlockUntilIdle()
{
	MRTC_ThreadPoolManagerInternal* pMgr = MRTC_GetObjectManager()->m_pThreadPoolManagerInternal;
	if(!pMgr)
	{
		return;
	}
	else
		pMgr->VPU_BlockUntilIdle();

}
*/


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_ScratchPadManager
|__________________________________________________________________________________________________
\*************************************************************************************************/

class MRTC_ScratchPad
{
public:
	TThinArrayAlign<uint8, 16> m_lData;

	uint8* GetPtr(uint32 _nSize)
	{
		if(_nSize > m_lData.Len())
		{
			m_lData.SetLen((_nSize + 4095) & ~4095);
		}
		return m_lData.GetBasePtr();
	}
};


class MRTC_ScratchPadManagerInternal
{
protected:
	TMRTC_ThreadLocal<MRTC_ScratchPad> m_Data;
public:
	uint8* Get(uint32 _Size)
	{
		return m_Data->GetPtr(_Size);
	}
};

class MRTC_ForgiveDebugNewInternal
{
public:
	class CForgiveDebugNew
	{
	public:
		int32 m_ForgiveNew;
		CForgiveDebugNew()
		{
			m_ForgiveNew = 0;
		}

	};

	TMRTC_ThreadLocal<CForgiveDebugNew> m_ForgiveDebugNew;
};



uint8* MRTC_ScratchPadManager::Get(uint32 _Size)
{
	return MRTC_GetObjectManager()->m_pScratchPadManagerInternal->Get(_Size);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_ObjectManager
|__________________________________________________________________________________________________
\*************************************************************************************************/

IMPLEMENT_OPERATOR_NEW(MRTC_ObjMgr_Entry);


MRTC_ObjMgr_Entry::MRTC_ObjMgr_Entry()
{
	m_nRef = 0;
	m_pNodeParent = NULL;
}

void MRTC_ObjMgr_Entry::operator= (const MRTC_ObjMgr_Entry& _Entry)
{
	m_nRef = _Entry.m_nRef;
	m_ObjectName = _Entry.m_ObjectName;
	m_spObj = _Entry.m_spObj;
	m_spNodeL = _Entry.m_spNodeL;
	m_spNodeG = _Entry.m_spNodeG;
	m_pNodeParent = _Entry.m_pNodeParent;
}

bool MRTC_ObjMgr_Entry::operator< (const MRTC_ObjMgr_Entry& _Entry) const
{
	return (m_ObjectName < _Entry.m_ObjectName) != 0;
}

MRTC_ObjMgr_Entry* MRTC_ObjMgr_Entry::Find_r(const char* _pObjName)
{
	int Comp = m_ObjectName.Compare(_pObjName);

	// Is this it?
	if (!Comp) return this;

	// No, recurse..
	if (Comp > 0)	// _pObjName < m_ObjectName
		return (m_spNodeL!=NULL) ? m_spNodeL->Find_r(_pObjName) : NULL;
	else
		return (m_spNodeG!=NULL) ? m_spNodeG->Find_r(_pObjName) : NULL;
}

void MRTC_ObjMgr_Entry::Insert_r(spMRTC_ObjMgr_Entry _spEntry)
{
	// Is spEntry less or greater than this node?
	if (*_spEntry < *this)
	{
		if (m_spNodeL!=NULL)
			m_spNodeL->Insert_r(_spEntry);
		else
		{
			m_spNodeL = _spEntry;
			_spEntry->m_pNodeParent = this;
		}
	}
	else
	{
		if (m_spNodeG!=NULL)
			m_spNodeG->Insert_r(_spEntry);
		else
		{
			m_spNodeG = _spEntry;
			_spEntry->m_pNodeParent = this;
		}
	}
}

void MRTC_ObjMgr_Entry::InsertTree_r(spMRTC_ObjMgr_Entry _spEntry)
{
	if (!_spEntry) return;
	
	if (_spEntry->m_spNodeL!=NULL)
		InsertTree_r(_spEntry->m_spNodeL);
	_spEntry->m_spNodeL = NULL;
	
	if (_spEntry->m_spNodeG!=NULL)
		InsertTree_r(_spEntry->m_spNodeG);
	_spEntry->m_spNodeG = NULL;
	
	Insert_r(_spEntry);
}

bool MRTC_ObjectManager::GetDllLoading()
{
	return DllLoading;
}

void MRTC_ObjectManager::SetDllLoading(bool LoadDlls)
{
	DllLoading = LoadDlls;
}

// -------------------------------------------------------------------
#include "MMemMgrHeap.h"



void MRTC_ObjectManager::ForgiveDebugNew(int32 _iAdd)
{
#ifndef	PLATFORM_PS3
	if (m_pForgiveContextInternal)
	{
		m_pForgiveContextInternal->m_ForgiveDebugNew->m_ForgiveNew += _iAdd;
	}
#endif
}

int32 MRTC_ObjectManager::ForgiveDebugNew()
{
#ifndef	PLATFORM_PS3
	if (!this)
		return NULL;
	if (m_pForgiveContextInternal)
	{
		return m_pForgiveContextInternal->m_ForgiveDebugNew->m_ForgiveNew;
	}	
	else
#endif
		return 0;
}


MRTC_ObjectManager::MRTC_ObjectManager()
{
	m_ThreadStorageIndex = MRTC_SystemInfo::Thread_LocalAlloc();

#ifdef M_Profile
	static uint8 TempData[sizeof(TMRTC_ThreadLocal<MRTC_ContextStack>)];
	m_ContextStackInternal = new(TempData) TMRTC_ThreadLocal<MRTC_ContextStack>;
#endif


#ifdef MRTC_AUTOSTRIPLOGGER
	m_pAutoStripLogger = NULL;
#endif
	m_bAssertHandler = false;
	m_bBreakOnAssert = true;
	DllLoading = true;
	m_ModuleCount = 0;
	m_pByteStreamManager = NULL;
	
#ifdef PLATFORM_WIN_PC
	m_Platform = 0;
	m_bXDFCreate = 0;
	m_Endian = 0;
#endif

	m_hMainThread = MRTC_SystemInfo::OS_GetThreadID();
	m_hScriptThread = NULL;

	m_pGlobalLock = new (MDA_NEW_DEBUG_NOLEAK uint8[sizeof(MRTC_CriticalSection)]) MRTC_CriticalSection;
	m_pObjMgrLock = new (MDA_NEW_DEBUG_NOLEAK uint8[sizeof(MRTC_CriticalSection)]) MRTC_CriticalSection;
	m_pGlobalStrLock = new (MDA_NEW_DEBUG_NOLEAK uint8[sizeof(MRTC_CriticalSection)]) MRTC_CriticalSection;

	// Due to alignment stuff we have to do it this way
	m_pThreadPoolManagerInternalMem = MDA_NEW_DEBUG_NOLEAK uint8[sizeof(MRTC_ThreadPoolManagerInternal) + 127];
	m_pThreadPoolManagerInternal = new ((void*)((((mint&)m_pThreadPoolManagerInternalMem) + 127) & ~127)) MRTC_ThreadPoolManagerInternal;
	m_pScratchPadManagerInternalMem = MDA_NEW_DEBUG_NOLEAK uint8[sizeof(MRTC_ScratchPadManagerInternal) + 127];
	m_pScratchPadManagerInternal = new ((void*)((((mint&)m_pScratchPadManagerInternalMem) + 127) & ~127)) MRTC_ScratchPadManagerInternal;

	m_pRand = new (MDA_NEW_DEBUG_NOLEAK uint8 [sizeof(CRand_MersenneTwister)]) CRand_MersenneTwister;

#ifndef PLATFORM_PS3
	m_pForgiveContextInternal = new (MDA_NEW_DEBUG_NOLEAK uint8[sizeof(MRTC_ForgiveDebugNewInternal)]) MRTC_ForgiveDebugNewInternal;
#endif

#ifdef M_Profile
	m_pCallGraph = new (MDA_NEW_DEBUG_NOLEAK uint8[sizeof(MRTC_CallGraph)]) MRTC_CallGraph;
	if (!m_pCallGraph)
		Error_static("-", "Out of memory.");
	m_pCallGraph->Clear();
#endif
}

MRTC_ObjectManager::~MRTC_ObjectManager()
{
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
#ifndef MRTC_ENABLE_REMOTEDEBUGGER_STATIC
	if (m_pRemoteDebugger && ((mint)m_pRemoteDebugger) != 1)
	{
		m_pRemoteDebugger->Destroy();
		m_pRemoteDebugger->~MRTC_RemoteDebug();
	}
#endif
#endif
	
#ifdef M_Profile
	if (m_pCallGraph)
	{
		delete m_pCallGraph;
		m_pCallGraph = NULL;
	}
#endif

#ifdef MRTC_AUTOSTRIPLOGGER
	delete m_pAutoStripLogger;
#endif

	if(m_pScratchPadManagerInternalMem)
		delete [] m_pScratchPadManagerInternalMem;

	if(m_pThreadPoolManagerInternalMem)
		delete [] m_pThreadPoolManagerInternalMem;

	if(m_pRand)
		delete m_pRand;

#ifndef PLATFORM_PS3
	if(m_pForgiveContextInternal)
	{
		delete m_pForgiveContextInternal;
		m_pForgiveContextInternal = NULL;
	}
#endif

	if (m_pByteStreamManager)
		delete m_pByteStreamManager;

	if (m_pObjMgrLock)
		delete m_pObjMgrLock;

	if (m_pGlobalLock)
		delete m_pGlobalLock;

	M_ASSERT(m_spRoot == NULL, "Objectmanager still contains object upon destruction");

#ifdef M_STATIC
	MRTC_ObjectManager::m_pSystemInfo->~MRTC_SystemInfo();
#endif

	MRTC_Thread_Core::Thread_FreeStore();
	MRTC_SystemInfo::Thread_LocalFree(m_ThreadStorageIndex);
}

class MRTC_ForgiveDebugNewContext *m_pForgiveContext;

bool MRTC_ObjectManager::InMainThread()
{
	return MRTC_SystemInfo::OS_GetThreadID() == m_hMainThread;
}

#ifdef MRTC_AUTOSTRIPLOGGER
CUsageLogger* MRTC_ObjectManager::GetAutostripLogger()
{
	if (!m_pAutoStripLogger)
		m_pAutoStripLogger = new (MDA_NEW_DEBUG_NOLEAK uint8[sizeof(CUsageLogger)])
								CUsageLogger("<<<MAUTOSTRIP-Logger>>> name:", 10000);
	return m_pAutoStripLogger;
}
#endif

MRTC_CClassRegistry* MRTC_ObjectManager::GetClassRegistry()
{
	return &m_ClassRegistry;
}

CReferenceCount* MRTC_ObjectManager::CreateObject(const char* _pClassName)
{
	M_LOCK(*m_pObjMgrLock);
	return m_ClassRegistry.CreateObject(_pClassName);
}

spCReferenceCount MRTC_ObjectManager::CreateObject(const char* _pClassName, const char* _pName)
{
	M_LOCK(*m_pObjMgrLock);
	// Check if there's already an object with that name
	spMRTC_ObjMgr_Entry spE;
	if (m_spRoot != NULL) m_spRoot->Find_r(_pName);
	if (spE != NULL)
	{
		spE->m_nRef++;
		return spE->m_spObj;
	}

	// No, create the object
	CObj* pObj = m_ClassRegistry.CreateObject(_pClassName);
	if (!pObj) return NULL;

	// Cast to a CRefCnt obj.
#if defined(COMPILER_RTTI) || defined(M_FAKEDYNAMICCAST)
	spCReferenceCount spObj = safe_cast<CReferenceCount>(pObj);
#else
	spCReferenceCount spObj = (CReferenceCount*)(pObj);
#endif
	if (!spObj) { delete pObj; return NULL; }

	// Create an entry
	spE = MNew(MRTC_ObjMgr_Entry);

	if (!spE) Error_static("MRTC_ObjectManager::CreateObject", "Out of memory.");
	spE->m_spObj = spObj;
	spE->m_ObjectName = _pName;
	spE->m_nRef = 1;

	// Insert it.
	if (!m_spRoot)
		m_spRoot = spE;
	else
		m_spRoot->Insert_r(spE);

	return spObj;
}

bool MRTC_ObjectManager::RegisterObject(spCReferenceCount _spObj, const char* _pName)
{
	M_LOCK(*m_pObjMgrLock);
	LogFile(CStrF("(MRTC_ObjectManager::RegisterObject) %s, %.8x", _pName, (CReferenceCount*)_spObj));

	// Check if there's already an object with that name, if so - return false.
	spMRTC_ObjMgr_Entry spE;
	if (m_spRoot != NULL) m_spRoot->Find_r(_pName);
	if (spE != NULL) return false;

	// Create an entry
	spE = MNew(MRTC_ObjMgr_Entry);
	if (spE == NULL) Error_static("MRTC_ObjectManager::CreateObject", "Out of memory.");
	spE->m_spObj = _spObj;
	spE->m_ObjectName = _pName;
	spE->m_nRef = 1;
	spE->m_pNodeParent = NULL;

	// Insert it.
	if (m_spRoot==NULL)
		m_spRoot = spE;
	else
		m_spRoot->Insert_r(spE);

	return true;
}



bool MRTC_ObjectManager::UnregisterObject(spCReferenceCount _spRef, const char* _pName)
{
	M_LOCK(*m_pObjMgrLock);
	if (_spRef != NULL) 
		LogFile(CStrF("(MRTC_ObjectManager::UnregisterObject) %s, %.8x, class %s", _pName, (CReferenceCount*)_spRef, (char*)_spRef->MRTC_ClassName()));
	else
		LogFile(CStrF("(MRTC_ObjectManager::UnregisterObject) %s, Unknown object", _pName));

	// Check if an object with that name, if so - return false.
	spMRTC_ObjMgr_Entry spE;
	if (m_spRoot != NULL) spE = m_spRoot->Find_r(_pName);
	if (spE == NULL) return false;

	// Count down ref and unregister object if ref == 0.
	spE->m_nRef--;
	if (spE->m_nRef < 0) Error_static("MRTC_ObjectManager::UnregisterObject", CStrF("nRef == %d", spE->m_nRef));
	if (!spE->m_nRef)
	{
		// First, unlink spE from it's parent.
		if (spE->m_pNodeParent)
		{
			MRTC_ObjMgr_Entry* pP = spE->m_pNodeParent;
			if (pP->m_spNodeL == spE) pP->m_spNodeL = NULL;
			if (pP->m_spNodeG == spE) pP->m_spNodeG = NULL;
			spE->m_pNodeParent = NULL;
		}
		else
			m_spRoot = NULL;

		if (spE->m_spNodeL != NULL) spE->m_spNodeL->m_pNodeParent = NULL;
		if (spE->m_spNodeG != NULL) spE->m_spNodeG->m_pNodeParent = NULL;
		// then insert spE's children into the tree.
		if (m_spRoot==NULL)
		{
			// No root exists, just use one of the children
			m_spRoot = spE->m_spNodeL;
			if (m_spRoot==NULL)
				m_spRoot = spE->m_spNodeG;
			else
				m_spRoot->InsertTree_r(spE->m_spNodeG);
		}
		else
		{
			// Root exists.
			m_spRoot->InsertTree_r(spE->m_spNodeL);
			m_spRoot->InsertTree_r(spE->m_spNodeG);
		}

		// Manually kill some sp's - not necessary, but could avoid some owning-loop 
		// hassle with sp's in very extreme cases.
		spE->m_spNodeL = NULL;
		spE->m_spNodeG = NULL;
		spE->m_spObj = NULL;
		spE = NULL;
	}
	return true;
}

void MRTC_ObjectManager::UnregisterAll()
{
	LogFile("(MRTC_ObjectManager::UnregisterAll) ...");

	while(m_spRoot!=NULL) UnregisterObject(m_spRoot->m_spObj, m_spRoot->m_ObjectName);
	//	m_spRoot = NULL;	// <- NUKE!
	LogFile("(MRTC_ObjectManager::UnregisterAll) Done.");
}

spCReferenceCount MRTC_ObjectManager::GetRegisteredObject(const char* _pName)
{
	M_LOCK(*m_pObjMgrLock);
	// Check for an object with that name, if not - return false.
	spMRTC_ObjMgr_Entry spE;
	if (m_spRoot != NULL) spE = m_spRoot->Find_r(_pName);
	if (spE == NULL) return NULL;
	return spE->m_spObj;
}

// -------------------------------------------------------------------
#ifdef	COMPILER_RTTI
void* MRTC_SafeCreateObject_NoEx(const char* _pClassName, const MRTC_TYPEINFOCLASS& _TypeInfo)
{
	CReferenceCount* pObj = (CReferenceCount*) MRTC_GOM()->CreateObject(_pClassName);
	if (!pObj) return NULL;
	if (typeid(*pObj) != _TypeInfo)
	{
		delete pObj;
		return NULL;
	}
	return pObj;
}

void* MRTC_SafeCreateObject(const char* _pClassName, const MRTC_TYPEINFOCLASS& _TypeInfo)
{
	CReferenceCount* pObj = (CReferenceCount*) MRTC_GOM()->CreateObject(_pClassName);
	if (!pObj) Error_static("MRTC_SafeCreateObject", CStrF("Could not instance object of class %s", _pClassName));
	if (typeid(*pObj) != _TypeInfo)
	{
		delete pObj;
		Error_static("MRTC_SafeCreateObject", CStrF("Object of %s expected, but class %s was created.", _TypeInfo.name(), _pClassName));
	}
	return pObj;
}
#else	// COMPILER_RTTI
void* MRTC_SafeCreateObject_NoEx(const char* _pClassName, const MRTC_CRuntimeClass* _pTypeInfo)
{
	CReferenceCount* pObj = (CReferenceCount*) MRTC_GOM()->CreateObject(_pClassName);
	if (!pObj) return NULL;
	if( strcmp( pObj->MRTC_ClassName(), _pTypeInfo->m_ClassName ) )
	{
		delete pObj;
		return NULL;
	}
	return pObj;
}

void* MRTC_SafeCreateObject(const char* _pClassName, const MRTC_CRuntimeClass* _pTypeInfo)
{
	CReferenceCount* pObj = (CReferenceCount*) MRTC_GOM()->CreateObject(_pClassName);
	if (!pObj) Error_static("MRTC_SafeCreateObject", CStrF("Could not instance object of class %s", _pClassName));
	if( strcmp( pObj->MRTC_ClassName(), _pTypeInfo->m_ClassName ) )
	{
#if	M_EXCEPTIONS
		const char *pClassName = pObj->MRTC_ClassName();
		delete pObj;
		Error_static("MRTC_SafeCreateObject", CStrF("Object of %s expected, but class %s was created.", _pTypeInfo->m_ClassName, pClassName));
#else
		delete pObj;
#endif
	}
	return pObj;
}
#endif	// COMPILER_RTTI

#ifdef MRTC_DLL

#else

class MRTC_AddClassContainer
{
public:
	MRTC_AddClassContainer()
	{
		M_LOCK(*(MRTC_GetObjectManager()->m_pObjMgrLock));
		MRTC_GetObjectManager()->GetClassRegistry()->AddClassContainer(&g_ClassContainer);
	}

	~MRTC_AddClassContainer()
	{
		M_LOCK(*(MRTC_GetObjectManager()->m_pObjMgrLock));
		MRTC_GetObjectManager()->GetClassRegistry()->RemoveClassContainer(&g_ClassContainer);
	}
};

MRTC_AddClassContainer g_AddClassContainer;

#endif


#ifdef PLATFORM_XENON
	MRTC_ObjectManager_Container g_ObjectManagerContainer = { NULL, NULL};
#elif defined(PLATFORM_XBOX1)
	MRTC_ObjectManager_Container g_ObjectManagerContainer = { NULL, NULL, NULL};
#elif defined(M_STATICINIT)
	MRTC_ObjectManager_Container g_ObjectManagerContainer = { NULL, NULL};
#else
	MRTC_ObjectManager_Container g_ObjectManagerContainer = { NULL, NULL, 0,0 };
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_ModuleRefCount
|__________________________________________________________________________________________________
\*************************************************************************************************/
#ifndef M_STATICINIT
#ifdef PLATFORM_WIN_PC
extern "C" void __cdecl __clean_type_info_names(void);
extern "C" uint __declspec(dllimport) __stdcall timeBeginPeriod( uint uPeriod);
extern "C" uint __declspec(dllimport) __stdcall timeEndPeriod( uint uPeriod);
#endif
class MRTC_ModuleRefCount
{
	int m_Dummy;
public:
	
	MRTC_ModuleRefCount()
	{
		MRTC_ObjectManager* pOM = MRTC_GetObjectManager();
		g_ObjectManagerContainer.Lock();
		pOM->m_ModuleCount++;
#ifdef PLATFORM_WIN_PC
		if (pOM->m_ModuleCount == 1)
			timeBeginPeriod(1);
#endif
		
#ifdef _DEBUG
		M_TRACEALWAYS("MRTC Memory manager module attach (%x) = %d\n (Debug)\n", &m_Dummy, pOM->m_ModuleCount);
#else
		M_TRACEALWAYS("MRTC Memory manager module attach (%x) = %d\n (Release)\n", &m_Dummy, pOM->m_ModuleCount);
#endif

	/*	try
		{
			int *Ptr = NULL;
			*Ptr = 0;
		}
		catch(...)
		{
		}
		Sleep(250);*/
		
		g_ObjectManagerContainer.Unlock();
	}
	
	
	
	~MRTC_ModuleRefCount()
	{
		MRTC_ObjectManager* pOM = MRTC_GetObjectManager();
		g_ObjectManagerContainer.Lock();
		pOM->m_ModuleCount--;

#ifdef PLATFORM_WIN_PC
		if (pOM->m_ModuleCount == 1)
			timeEndPeriod(1);
#endif
		M_TRACEALWAYS("MRTC Memory manager module detach (%x) = %d\n", &m_Dummy, pOM->m_ModuleCount);
#ifdef PLATFORM_WIN_PC
		__clean_type_info_names();
#endif

	/*	try
		{
			int *Ptr = NULL;
			*Ptr = 0;
		}
		catch(...)
		{
		}
		Sleep(250);*/
		g_ObjectManagerContainer.Unlock();
	}
	
};

MRTC_ModuleRefCount g_MRTC_ModuleReference;
#endif
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| MRTC_CreateMemManager
|__________________________________________________________________________________________________
\*************************************************************************************************/



#ifdef PLATFORM_XBOX1

class CRndrXBox_GraphicsHeap : public CDA_MemoryManager 
{
public:
	
	void *AllocHeap(uint32 Size, bool _bCommit)
	{
		M_ASSERT(_bCommit, "Must not use this heap with virtual memory");
		return D3D_AllocContiguousMemory(Size, 4096);
	}

	void FreeHeap(void *Block)
	{
		D3D_FreeContiguousMemory(Block);
	}

	bool HeapCommit(void *_pMem, uint32 _Size, bool _bCommited)
	{
		M_ASSERT(0, "Must not use this heap with virtual memory");
		return false;
	}


};
/*
class CRndrXBox_GraphicsHeapCached : public CDA_MemoryManager 
{
public:
	
	void *AllocHeap(uint32 Size, bool _bCommit)
	{
		M_ASSERT(_bCommit, "Must not use this heap with virtual memory");
		return XPhysicalAlloc(Size, MAXULONG_PTR, 0, PAGE_READWRITE); //| |PAGE_VIDEO PAGE_WRITECOMBINE | 
	}

	void FreeHeap(void *Block)
	{
		XPhysicalFree(Block);
	}

	bool HeapCommit(void *_pMem, uint32 _Size, bool _bCommited)
	{
		M_ASSERT(0, "Must not use this heap with virtual memory");
		return false;
	}


};
*/
#ifndef M_STATICINIT

CDA_MemoryManager* MRTC_GetGraphicsHeap()
{
	MRTC_GetObjectManager();

	return g_ObjectManagerContainer.m_pGraphicsHeap;
}
/*
CDA_MemoryManager* MRTC_GetGraphicsHeapCached()
{
#ifdef M_Profile
	MRTC_GetObjectManager();

	return g_ObjectManagerContainer.m_pGraphicsHeapCached;
#else
	return MRTC_GetMemoryManager();
#endif
}
*/
#endif



#endif


#if defined(PLATFORM_DOLPHIN) && defined(USE_VIRTUAL_MEMORY)
#include <dolphin/vm.h>

class CGameCube_VirtualHeap : public CDA_MemoryManager
{
	uint32 m_VMAddress;

public:
	enum { e_VMSize = 0x800000,   // 8MB aram memory (virtual memory)
	       e_RMSize = 0x200000 }; // 2MB main memory (swap space)

	CGameCube_VirtualHeap()
	{
	 	VMInit(e_RMSize, ARGetBaseAddress(), e_VMSize);
		VMSetPageReplacementPolicy(VM_PRP_RANDOM);
//		VMSetPageReplacementPolicy(VM_PRP_LRU);
		VMSetLogStatsCallback( LogPageMiss );
	 	m_VMAddress = 0x7E000000;
	}

	virtual void* AllocHeap(uint32 _Size, bool _bCommit)
	{
		_Size = (_Size + 4095) & ~4095;
		if (VMAlloc(m_VMAddress, _Size))
		{
			m_VMAddress += _Size;
			return (void*)(m_VMAddress - _Size);
		}
		return NULL;
	}

	virtual void FreeHeap(void* _pMem)
	{
		M_ASSERT(false, "Not supported!");
	}

	virtual bool HeapCommit(void* _pMem, uint32 _Size, bool _bCommited)
	{
		OSReport("HeapCommit, _pMem=0x%08X, _Size=%d, _bCommited=%d\n",
			_pMem, _Size, _bCommited);
		return false;
	}

	static void LogPageMiss(u32 exactVirtualAddress, u32 physicalAddress, u32 pageNumber, u32 pageMissLatency, BOOL pageSwappedOut)
	{
		OSReport("VMLogPageMiss, VM-addr: 0x%08X, phys-addr: 0x%08X, page: 0x%04X, latency: %d us, swapped: %s\n",
			exactVirtualAddress, physicalAddress, pageNumber, pageMissLatency, pageSwappedOut ? "yes" : "no");
	}
};


CDA_MemoryManager* MRTC_GetVirtualHeap()
{
	MRTC_GetObjectManager();
	return g_ObjectManagerContainer.m_pVirtualHeap;
}
#endif

class CDA_DefaultMemoryManager : public CDA_MemoryManager
{
public:
	CDA_DefaultMemoryManager()
	{

	}

	~CDA_DefaultMemoryManager()
	{
		Destroy();
	}

};

#ifdef PLATFORM_XBOX
extern void gf_PreDestroySystem();
extern mint gf_GetFreePhysicalMemory();
extern mint gf_GetLargestBlockPhysicalMemory();
#endif
void MRTC_DestroyObjectManager()
{
#ifdef PLATFORM_XBOX
	CByteStreamManager *pManager = g_ObjectManagerContainer.m_pManager->m_pByteStreamManager;
	g_ObjectManagerContainer.m_pManager->m_pByteStreamManager = NULL;
	delete pManager;
	gf_PreDestroySystem();
	g_ObjectManagerContainer.m_pManager->~MRTC_ObjectManager();
#ifdef PLATFORM_XBOX1
	g_ObjectManagerContainer.m_pGraphicsHeap->~CDA_MemoryManager();
#endif
	g_ObjectManagerContainer.m_pMemoryManager->~CDA_MemoryManager();	
#endif
}


void MRTC_CreateMemManager()
{
	g_ObjectManagerContainer.m_pMemoryManager = new(MRTC_SystemInfo::OS_Alloc(sizeof(CDA_DefaultMemoryManager), M_ALIGNMENTOF(CDA_DefaultMemoryManager)))CDA_DefaultMemoryManager;

#if 1//#ifdef MRTC_MEMMANAGEROVERRIDE_MEMDEBUG
#ifdef M_SUPPORTMEMORYDEBUG
	g_ObjectManagerContainer.m_pMemoryManager->m_bDebugMemory = true;
#endif
#endif

#ifdef _DEBUG
	g_ObjectManagerContainer.m_pMemoryManager->m_RunningDebugRuntime = true;
#else
	g_ObjectManagerContainer.m_pMemoryManager->m_RunningReleaseRuntime = true;
#endif

#ifdef PLATFORM_XBOX
	MEMORYSTATUS m_Win32MemoryStatus;
	m_Win32MemoryStatus.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&m_Win32MemoryStatus);

	if (m_Win32MemoryStatus.dwAvailPhys < 64*1024*1024)
		m_Win32MemoryStatus.dwTotalPhys = 64*1024*1024;
	

	M_TRACEALWAYS("(MRTC_CreateMemManager) Memory used %d MiB\n", (m_Win32MemoryStatus.dwTotalPhys - m_Win32MemoryStatus.dwAvailPhys) / 0x100000 );

	#if defined(M_RTM) && !defined(M_Profile)
		fp32 GraphicHeapSize = 19.4f;
	#elif defined(_DEBUG)
		fp32 GraphicHeapSize = 30.0f;
	#else
		fp32 GraphicHeapSize = 46.0f;
	#endif
	
//	GraphicHeapSize = 22.5f;
#ifdef PLATFORM_XENON

	uint32 LargestSize = gf_GetLargestBlockPhysicalMemory();
	uint32 GlobalHeap = AlignDown(gf_GetLargestBlockPhysicalMemory(), 4096);
	//m_Win32MemoryStatus.dwTotalPhys - (GraphicHeapSize * 0x100000 + 256 * 0x100000);
#else

#ifdef M_RTM
	int GlobalHeap = m_Win32MemoryStatus.dwTotalPhys - (GraphicHeapSize * 0x100000 + 16 * 0x100000);
#elif defined(_DEBUG)
	int GlobalHeap = m_Win32MemoryStatus.dwTotalPhys - (GraphicHeapSize * 0x100000 + 40 * 0x100000);
#else
	int GlobalHeap = m_Win32MemoryStatus.dwTotalPhys - (GraphicHeapSize * 0x100000 + 24 * 0x100000);
#endif
#endif

//	M_TRACEALWAYS("(MRTC_CreateMemManager) Graphics heap %d MiB\n", GraphicHeapSize );
#ifdef MRTC_DEFAULTMAINHEAP
	g_ObjectManagerContainer.m_pMemoryManager->m_bUseDefaultMainHeap = true;	
	M_TRACEALWAYS("(MRTC_CreateMemManager) Global heap not used\n");
#else
	g_ObjectManagerContainer.m_pMemoryManager->InitStatic(GlobalHeap);
	M_TRACEALWAYS("(MRTC_CreateMemManager) Global heap %d MiB\n", GlobalHeap / 0x100000 );
#endif


#ifdef PLATFORM_XBOX1
	#ifdef M_Profile

		g_ObjectManagerContainer.m_pGraphicsHeap = DNew(CRndrXBox_GraphicsHeap) CRndrXBox_GraphicsHeap;
	//	int IndexBufferHeapSize = 2;
		g_ObjectManagerContainer.m_pGraphicsHeap->InitStatic(((int)(GraphicHeapSize*1024.0)) * 1024);

	//	g_ObjectManagerContainer.m_pGraphicsHeapCached = DNew(CRndrXBox_GraphicsHeapCached) CRndrXBox_GraphicsHeapCached;
	//	g_ObjectManagerContainer.m_pGraphicsHeapCached->InitStatic(IndexBufferHeapSize * 0x100000, false);

	#else
		g_ObjectManagerContainer.m_pGraphicsHeap = DNew(CRndrXBox_GraphicsHeap) CRndrXBox_GraphicsHeap;
		g_ObjectManagerContainer.m_pGraphicsHeap->InitStatic(((int)(GraphicHeapSize*1024.0)) * 1024);
	#endif
#endif
#endif

#ifdef PLATFORM_PS3

#ifdef _DEBUG
	int GlobalHeap = 192 * 0x100000;
#else
	int GlobalHeap = 192 * 0x100000;
#endif

#ifdef MRTC_DEFAULTMAINHEAP
	g_ObjectManagerContainer.m_pMemoryManager->m_bUseDefaultMainHeap = true;	
	M_TRACEALWAYS("(MRTC_CreateMemManager) Global heap not used\n");
#else
	g_ObjectManagerContainer.m_pMemoryManager->InitStatic(GlobalHeap);
	M_TRACEALWAYS("(MRTC_CreateMemManager) Global heap %d MiB\n", GlobalHeap / 0x100000 );
#endif

#endif

#ifdef PLATFORM_DOLPHIN

	#ifdef USE_VIRTUAL_MEMORY
		void* pMem = MRTC_SystemInfo::OS_Alloc(sizeof(CGameCube_VirtualHeap), M_ALIGNMENTOF(CGameCube_VirtualHeap));
		g_ObjectManagerContainer.m_pVirtualHeap = new(pMem) CGameCube_VirtualHeap;
		g_ObjectManagerContainer.m_pVirtualHeap->InitStatic(CGameCube_VirtualHeap::e_VMSize-65536);
	#endif

	#ifdef MRTC_DEFAULTMAINHEAP
		g_ObjectManagerContainer.m_pMemoryManager->m_bUseDefaultMainHeap = true;
		g_ObjectManagerContainer.m_pMemoryManager->m_AllocatedMem	= MRTC_SystemInfo::OS_PhysicalMemorySize();
	#else	

		#ifdef M_RTM
			int nTotalMem = MRTC_SystemInfo::OS_PhysicalMemorySize() - 128*1024;
		#else
			int nTotalMem = MRTC_SystemInfo::OS_PhysicalMemorySize() - 1536*1024;
		#endif

		#if defined(USE_VIRTUAL_MEMORY)
			nTotalMem -= CGameCube_VirtualHeap::e_RMSize;
		#endif

		g_ObjectManagerContainer.m_pMemoryManager->InitStatic(nTotalMem);
	#endif

#endif

#ifdef PLATFORM_WIN_PC
#ifdef MRTC_DEFAULTMAINHEAP
	g_ObjectManagerContainer.m_pMemoryManager->m_bUseDefaultMainHeap = true;
#endif
#endif

#ifdef PLATFORM_XENON
#ifdef MRTC_DEFAULTMAINHEAP
	g_ObjectManagerContainer.m_pMemoryManager->m_bUseDefaultMainHeap = true;
#endif
#endif

#ifdef	PLATFORM_PS2
	#ifdef MRTC_DEFAULTMAINHEAP
		g_ObjectManagerContainer.m_pMemoryManager->m_bUseDefaultMainHeap = true;
		g_ObjectManagerContainer.m_pMemoryManager->m_AllocatedMem	= MRTC_SystemInfo::OS_PhysicalMemorySize();
	#else	

		int nTotalMem = MRTC_SystemInfo::OS_PhysicalMemorySize() - 1024*1024;

		g_ObjectManagerContainer.m_pMemoryManager->InitStatic(nTotalMem);
#endif
#endif	// PLATFORM_PS2

/*	int AvailPhys = MRTC_GetMemoryManager()->GetMinFreeMem();
	int nBlocks = 0;
	int BlockSize = 32768-16;
	while(new uint8[BlockSize])
		nBlocks++;
	int AvailPhys3 = MRTC_GetMemoryManager()->GetMinFreeMem();

	M_TRACEALWAYS("AvailPhys %d, %d, Blocks %d, BlockSizeIncHeader %d, BlockOverhead %d\n", 
		AvailPhys, AvailPhys3, nBlocks, 
		(AvailPhys-AvailPhys3) / nBlocks, 
		((AvailPhys-AvailPhys3) / nBlocks) - BlockSize);
*/
	// Test
/*	void* pBlocks[16384];

	int AvailPhys = MRTC_GetMemoryManager()->GetMinFreeMem();
	int BlockSize = (4);
	int nBlocks = Min(16384, (AvailPhys - 1024*1024) / BlockSize);
	{
		for(int i = 0; i < nBlocks; i++)
		{
			pBlocks[i] = MRTC_GetMemoryManager()->Alloc(BlockSize);
			if (!pBlocks[i])
			{
				M_BREAKPOINT;
			}
		}
	}

	int AvailPhys3 = MRTC_GetMemoryManager()->GetMinFreeMem();

	{
		for(int i = 0; i < nBlocks; i += 2)
		{
			MRTC_GetMemoryManager()->Free(pBlocks[i]);
		}
	}

	int AvailPhys2 = MRTC_GetMemoryManager()->GetMinFreeMem();

	M_TRACEALWAYS("AvailPhys %d, %d, %d, Blocks %d, BlockOverhead %d", AvailPhys, AvailPhys2, AvailPhys3, nBlocks, ((AvailPhys-AvailPhys3) / nBlocks) - BlockSize);
*/
 }

void gf_ModuleAdd();
static int g_ObjectManCreated = false;

// main.cpp
// sample Product ID 0x4212A901

#if __GNUC__ // GCC Compiler (PS2, GCN)

//#define DATA_SECTION __attribute__((section(".data")))

#else  	   // MS Compiler (PC, XBOX )
    
#define DATA_SECTION

#endif

struct FingerPrint
{
public:

	//marks start of fingerprint
	unsigned int m_ProductID;	
	unsigned int m_Fillout;	

	//fingerprint destination buffer
	char m_Fingerprint[256];  

	//marks end of fingerprint
	unsigned int m_Checksum;    
};

FingerPrint g_FingerPrint = {0xd8a7bac1, 0, "c 2004 Vivendi Universal Games", 2596};

#ifdef PLATFORM_WIN_PC
static int strncmp(const char*_a, const char* _b, int _Max)
{
	int i = 0;
	while(_a[i] && _b[i] && (i < _Max))
	{
		int Diff = _b[i] - _a[i];
		if(Diff != 0)
			return Diff;
		i++;
	}

	if(i == _Max)
		return 0;
	else if(_a)
		return -1;
	else if(_b)
		return 1;

	return 0;
}
#endif // PLATFORM_WIN_PC

// -------------------------------------------------------------------
void MRTC_CreateObjectManager()
{
	if (g_ObjectManCreated)
		return;

	unsigned int uiSum = 0,
		         ui        = 0;
	while( g_FingerPrint.m_Fingerprint[ ui ] )
		uiSum += g_FingerPrint.m_Fingerprint[ ui++ ];
	if( uiSum != g_FingerPrint.m_Checksum)
		*(int*)(0) = 0; // suicide

	gf_ModuleAdd();

#if defined(M_STATIC)
	MRTC_SystemInfo::MRTC_GetSystemInfo();
#endif
	g_ObjectManCreated = true;

	bool bCreated = false;

#ifndef M_STATICINIT
	if (!g_ObjectManagerContainer.m_bInitialized) 
#endif
	{

		CFStr EnvString;
		
		EnvString = CFStrF("MRTC_ObjectManagerProcessMutex:%x", (mint)MRTC_SystemInfo::OS_GetProcessID());

#ifndef M_STATICINIT
		g_ObjectManagerContainer.m_hLock = MRTC_SystemInfo::OS_MutexOpen(EnvString);
		g_ObjectManagerContainer.Lock();
#endif

#ifdef PLATFORM_DOLPHIN
		M_TRACEALWAYS("MRTC: OSInit\n");
		OSInit();
		OSInitFastCast();
		M_TRACEALWAYS("MRTC: DVDInit\n");
		DVDInit();
#endif

	
		M_TRACEALWAYS("MRTC: Creating object manager\n");

#ifdef PLATFORM_WIN_PC
		if (!g_ObjectManagerContainer.m_bInitialized) 
		{
			CFStr Allocator;
			Allocator = CFStrF("MRTC_ObjectManagerGlobalAllocator:%x", GetCurrentProcessId());
			
			{
				// Since environment is inherited from parent tasks we have to process the environment and nuke
				// all MRTC_ObjectManagerGlobalAllocator string that don't belong to this one
				const char* pEnvStrings = GetEnvironmentStringsA();

				CFStr Matcher("MRTC_ObjectManagerGlobalAllocator");
				int iPos = 0;
				while(pEnvStrings[iPos])
				{
					const char* pEnvString = pEnvStrings + iPos;
					if(!strncmp(pEnvString, Matcher.GetStr(), Matcher.Len()))
					{
						bool bNuke = false;
						if(strncmp(pEnvString, Allocator.GetStr(), Allocator.Len()))
							bNuke = true;
						else if(pEnvString[Allocator.Len()] != '=')
							bNuke = true;

						if(bNuke)
						{
							// No match, nuke this one
							CFStr EnvString(pEnvString);
							int iSplitter = EnvString.Find("=");
							EnvString.GetStr()[iSplitter] = 0;

							SetEnvironmentVariableA(EnvString.GetStr(), NULL);
						}
					}

					while(pEnvStrings[iPos])
						iPos++;

					iPos++;
				}

				FreeEnvironmentStringsA((LPSTR)pEnvStrings);
			}

			int NumChar = GetEnvironmentVariableA(Allocator, EnvString, 256);
			
			if (NumChar)
			{					
				mint len = strlen(EnvString);
				
				aint number = 0;
				int i = len - 1;
				int j = 0;
				
				int Base = 1;
				for (; i >= 0; --i, ++j)
				{
					number += (EnvString[i] - '0') * Base;
					Base *= 10;
				}
				
				g_ObjectManagerContainer.m_pManager = (MRTC_ObjectManager *)number;
				g_ObjectManagerContainer.m_pMemoryManager = g_ObjectManagerContainer.m_pManager->m_pMemoryManager;
			}
			else
			{
				bCreated = true;
				char *buf = (char *) MRTC_SystemInfo::OS_Alloc(sizeof(MRTC_ObjectManager), M_ALIGNMENTOF(MRTC_ObjectManager));

				g_ObjectManagerContainer.m_pManager = (MRTC_ObjectManager*) buf;
#ifdef MRTC_AUTOSTRIPLOGGER
				g_ObjectManagerContainer.m_pManager->m_pAutoStripLogger = NULL;
#endif // MRTC_AUTOSTRIPLOGGER
				g_ObjectManagerContainer.m_pManager->m_bAssertHandler = false;
				g_ObjectManagerContainer.m_pManager->m_bBreakOnAssert = true;
				g_ObjectManagerContainer.m_bInitialized = 1;
				MRTC_CreateMemManager();
				g_ObjectManagerContainer.m_pManager = new (buf) MRTC_ObjectManager;
				g_ObjectManagerContainer.m_pManager->m_pMemoryManager = g_ObjectManagerContainer.m_pMemoryManager;

#ifdef PLATFORM_XENON
				int nThreads = 4;
#else
				int nThreads = Max((uint32)0, MRTC_SystemInfo::MRTC_GetSystemInfo().m_nCPU - 2);	// Reserve 1 thread for system and 1 for rendering
#endif
				g_ObjectManagerContainer.m_pManager->m_pThreadPoolManagerInternal->Create(nThreads);
				EnvString = CFStrF("%d", (mint)buf);
				SetEnvironmentVariableA(Allocator, EnvString);
			}
			
			g_ObjectManagerContainer.m_bInitialized = 1;
		}
#else
		bCreated = true;
		char *buf = (char *) MRTC_SystemInfo::OS_Alloc(sizeof(MRTC_ObjectManager), M_ALIGNMENTOF(MRTC_ObjectManager));

		g_ObjectManagerContainer.m_pManager = (MRTC_ObjectManager*) buf;
#ifndef M_STATICINIT
		g_ObjectManagerContainer.m_bInitialized = 1;
#endif
		MRTC_CreateMemManager();
		g_ObjectManagerContainer.m_pManager = new (buf) MRTC_ObjectManager;
		g_ObjectManagerContainer.m_pManager->m_pMemoryManager = g_ObjectManagerContainer.m_pMemoryManager;
#ifdef PLATFORM_XENON
		int nThreads = 4;
#else
		int nThreads = MRTC_SystemInfo::MRTC_GetSystemInfo().m_nCPU - 1;
#endif
		g_ObjectManagerContainer.m_pManager->m_pThreadPoolManagerInternal->Create(nThreads);
		
#endif

#ifdef _DEBUG
		g_ObjectManagerContainer.m_pMemoryManager->m_RunningDebugRuntime = true;
#else
		g_ObjectManagerContainer.m_pMemoryManager->m_RunningReleaseRuntime = true;
#endif

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
		if (MRTC_GetRD())
			MRTC_GetRD()->ModuleInit();
#endif

#ifndef M_STATICINIT
		g_ObjectManagerContainer.Unlock();
#endif

		if (bCreated)
		{		
			MRTC_SystemInfo::MRTC_GetSystemInfo().PostCreate();

#ifndef PLATFORM_PS3
			MRTC_GetObjectManager()->ForgiveDebugNew(1);
#endif
		}
	}

};

#ifndef M_STATICINIT
MRTC_ObjectManager* MRTC_GetObjectManager()
{
	if (!g_ObjectManagerContainer.m_bInitialized) 
	{
		MRTC_CreateObjectManager();
	}

	return g_ObjectManagerContainer.m_pManager;
};


CDA_MemoryManager* MRTC_GetMemoryManager()
{
	if (!g_ObjectManagerContainer.m_bInitialized) 
		return MRTC_GetObjectManager()->m_pMemoryManager;
	else
		return g_ObjectManagerContainer.m_pMemoryManager;
}

MRTC_ClassContainer* MRTC_GetClassContainer();

MRTC_ClassContainer* MRTC_GetClassContainer()
{
//	OutputDebugString("MRTC_GetClassContainer...\n");
	return &g_ClassContainer;
}
#endif



// -------------------------------------------------------------------
void MCC_TPtrError(char* _pLocation, char* _pMsg)
{
	Error_static(_pLocation, _pMsg);
}

// -------------------------------------------------------------------

#include "MRTC_StrBase.cpp"
#include "MRTC_FStrBase.cpp"
#include "MRTC_Str.cpp"
#include "MRTC_CallGraph.cpp"
#ifndef PLATFORM_XBOX
#include "MRTC_RemoteDebug.cpp"
#endif
#include "MRTC_Exception.cpp"
#include "MRTC_System.cpp"
#include "MRTC_Misc.cpp"
#include "MRTC_Thread.cpp"

#include "MMemMgrOverride.cpp"

#ifdef M_SUPPORTMEMORYDEBUG


NMemMgr::CMemTrack_Class::CMemTrack_Class()
{

	m_AllocData.m_TimesAlloced = 0;
	m_AllocData.m_TimesDeleted = 0;
	m_AllocData.m_MemUsed = 0;
	m_LastAllocData = m_AllocData;	
	m_pChild = NULL;
}

NMemMgr::CMemTrack_Class::~CMemTrack_Class()
{
	if (m_pChild)
		delete m_pChild;	
}

#endif

#if defined(PLATFORM_WIN_PC) || defined(PLATFORM_XBOX) || defined(PLATFORM_PS3)

void File_WriteLE(CCFile* _pFile, fp64 _Value)
{
	_pFile->WriteLE(_Value);
}

void File_ReadLE(CCFile* _pFile, fp64& _Value)
{
	_pFile->ReadLE(_Value);
}


#elif defined(PLATFORM_PS2)

void CMTime::Write(CCFile *_pFile) const
{
	_pFile->WriteLE(m_aTime[0]);
	_pFile->WriteLE(m_aTime[1]);
}

void CMTime::Read(CCFile *_pFile)
{
	_pFile->ReadLE(m_aTime[0]);
	_pFile->ReadLE(m_aTime[1]);
}

#else
#error "Implement this"
#endif
