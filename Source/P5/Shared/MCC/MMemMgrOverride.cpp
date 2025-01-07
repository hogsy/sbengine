
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/


#ifdef PLATFORM_WIN
	#include <crtdbg.h>
#endif

#include "MCC.h"


#if defined(PLATFORM_DOLPHIN) && defined(USE_VIRTUAL_MEMORY)
# define MACRO_GetDefaultMemoryManager() MRTC_GetVirtualHeap()
#else
# define MACRO_GetDefaultMemoryManager() MRTC_GetMemoryManager()
#endif

#ifndef M_RTM
void* operator new(mint _Size, int _Block, int Dummy, int Dummy2)
{
# ifdef M_SUPPORTMEMORYDEBUG
	return MACRO_GetDefaultMemoryManager()->AllocDebug(_Size, _Block, NULL, 0);
# else
	return MACRO_GetDefaultMemoryManager()->Alloc(_Size);
# endif
}

void* operator new[](mint _Size, int _Block, int Dummy, int Dummy2)
{
# ifdef M_SUPPORTMEMORYDEBUG
	return MACRO_GetDefaultMemoryManager()->AllocDebug(_Size, _Block, NULL, 0);
# else
	return MACRO_GetDefaultMemoryManager()->Alloc(_Size);
# endif
}

# ifdef COMPILER_NEEDOPERATORDELETE
void operator delete(void *Block, int _Block, int Dummy, int Dummy2)
{
	MACRO_GetDefaultMemoryManager()->Free(Block);
}
# endif

void* operator new(mint _Size, int _Block, const char *_File, int _FileNumber, SDA_Defraggable *_MakeThisUnique)
{
# ifdef M_SUPPORTMEMORYDEBUG
	return MACRO_GetDefaultMemoryManager()->AllocDebug(_Size, _Block, _File, _FileNumber);
# else
	return MACRO_GetDefaultMemoryManager()->Alloc(_Size);
# endif
}

void* operator new[](mint _Size, int _Block, const char *_File, int _FileNumber, SDA_Defraggable *_MakeThisUnique)
{
# ifdef M_SUPPORTMEMORYDEBUG
	return MACRO_GetDefaultMemoryManager()->AllocDebug(_Size, _Block, _File, _FileNumber);
# else
	return MACRO_GetDefaultMemoryManager()->Alloc(_Size);
# endif
}

# ifdef COMPILER_NEEDOPERATORDELETE
void operator delete(void *Block, int _Block, const char *_File, int _FileNumber, SDA_Defraggable *_MakeThisUnique)
{
	MACRO_GetDefaultMemoryManager()->Free(Block);
}
# endif

#endif


//#ifndef PLATFORM_XBOX
#if 0
#ifndef PLATFORM_WIN32_PC
# ifndef _AFX6
#  ifdef COMPILER_GNU
void * M_CDECL operator new (mint size) throw(std::bad_alloc)
#  else
void * M_CDECL operator new (mint size)
#  endif
{
	return MACRO_GetDefaultMemoryManager()->Alloc(size);
}

#  ifdef COMPILER_GNU
void M_CDECL operator delete (void * ptr) throw( )
#  else
void M_CDECL operator delete (void * ptr)
#  endif
{
	MACRO_GetDefaultMemoryManager()->Free(ptr);
}

# endif

#ifdef COMPILER_GNU
void * M_CDECL operator new[] (mint size) throw(std::bad_alloc)
#else
void * M_CDECL operator new[] (mint size)
#endif
{
	return MACRO_GetDefaultMemoryManager()->Alloc(size);
}

#ifdef COMPILER_GNU
void M_CDECL operator delete[] (void * ptr) throw()
#else
void M_CDECL operator delete[] (void * ptr)
#endif
{
	MACRO_GetDefaultMemoryManager()->Free(ptr);
}

#endif

#endif
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CDA_MemoryManager_Container
|__________________________________________________________________________________________________
\*************************************************************************************************/
extern SDA_Defraggable *BlockFromMem(const void *_Block);

#ifdef PLATFORM_XBOX1

//@@YAHXZ
//?__CxxSetUnhandledExceptionFilter@@YIXXZ
//?__CxxSetUnhandledExceptionFilter@@YGXXZ
/*
int M_CDECL __CxxSetUnhandledExceptionFilter(void)
{

	return 1;
}

int M_CDECL __CxxRestoreUnhandledExceptionFilter(void)
{

	return 1;
}
*/
//void __stdcall SetUnhandledExceptionFilter(void);
/*
extern "C" 
{	 
	extern LPTOP_LEVEL_EXCEPTION_FILTER XapiCurrentTopLevelFilter;

	LPTOP_LEVEL_EXCEPTION_FILTER WINAPI SetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
	{
		LPTOP_LEVEL_EXCEPTION_FILTER pOldFilter = XapiCurrentTopLevelFilter;
			
//		XapiCurrentTopLevelFilter = lpTopLevelExceptionFilter;

		return pOldFilter;
	}

//	extern DWORD KiIDT;
//	typedef void * (__stdcall Typefromhell)(void *);
	typedef void * (__stdcall *Typefromhell)(void *);
	typedef void * (__stdcall Typefromhell2)(void *);

	extern Typefromhell2 _imp__MmGetPhysicalAddress;
	extern Typefromhell2 _imp__MmGetVirtualAddress;

};*/
/*
typedef LONG (WINAPI *PTOP_LEVEL_EXCEPTION_FILTER)(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    );
*/

#if 0
void __declspec(naked) GetPhysicalAddressNaked(void)
{
	__asm
	{		
		push        ebp
		mov         ebp,esp
		mov         ecx,dword ptr [ebp+8]
		mov         eax,ecx
		shr         eax,0x14
		and         eax,0x0FFC
		sub         eax,0x3FD00000
		mov         eax,dword ptr [eax]
		test        al,1
		push        esi
		je          Invalid0
		test        al,al
		js          Invalid2
		mov         eax,ecx
		shr         eax,0Ah
		and         eax,0x003ffffc
		sub         eax,0x40000000
		mov         eax,dword ptr [eax]
		test        al,1
		jne         Invalid1
Invalid0:
		push        ecx
		push        0x80012810
//		call        0x8002ed34
		pop         ecx
		pop         ecx
		xor         eax,eax
		jmp         Invalid3
Invalid1:
		and         ecx,0x0FFF
		jmp         Invalid4
Invalid2:
		and         ecx,0x003fffff
Invalid4:
		and         eax,0FFFFF000h
		mov         esi,ecx
		add         esi,eax
		mov         eax,esi
		shr         eax,0x0C
		cmp         eax,dword ptr [0x8004fff8]
		ja          Invalid5
		lea         eax,[eax*4-0x7C010000]
		mov         eax,dword ptr [eax]
		test        al,1
		jne         Invalid5
		test        ax,(0x80028f42)
		jne         Invalid5
		push        0
		push        0x5C
		push        0x800127ec
		push        0x800127cc
//		call        0x8002fb5f
Invalid5:
		mov         eax,esi
Invalid3:
		pop         esi
		pop         ebp
		ret         4
	}
}

void *GetPhysicalAddress(void *In)
{

	DWORD EAXMap = (DWORD)In;
	DWORD ECXMap = EAXMap;
	DWORD ESIMap;
	DWORD Ret = 0;

	EAXMap = EAXMap >> 20;
	EAXMap = EAXMap & 0x0FFC;
	EAXMap = EAXMap - 0x3FD00000;
	EAXMap = *((DWORD*)EAXMap);

	if ((EAXMap&0xff) == 1)
		goto Invalid0;

	if ((EAXMap&0xff) & 0x80)
		goto Invalid2;

	EAXMap = ECXMap;
	EAXMap = EAXMap >> 0xA;
	EAXMap = EAXMap & 0x003ffffc;
	EAXMap = EAXMap - 0x40000000;
	EAXMap = *((DWORD*)EAXMap);

	if ((EAXMap&0xff) != 1)
		goto Invalid1;

Invalid0:
	OutputDebugString("GetPhysicalAddress failed\n");
	Ret = NULL;
	goto Invalid3;

Invalid1:
	ECXMap = ECXMap & 0x0fff;
	goto Invalid4;

Invalid2:
	ECXMap = ECXMap & 0x003fffff;

Invalid4:
	EAXMap = EAXMap & 0x0FFFFF000;
	ESIMap = ECXMap;
	ESIMap = ESIMap + EAXMap;
	EAXMap = ESIMap;
	EAXMap = EAXMap >> 0xC;

	if (EAXMap < (*((DWORD *)0x8004fff8)))
		goto Invalid5;

	EAXMap = EAXMap*4-0x7C010000;
	EAXMap = *((DWORD *)EAXMap);

	if ((EAXMap&0xff) != 1)
		goto Invalid5;

	if ((EAXMap&0xffff) != *((WORD *)0x80028f42))
		goto Invalid5;

	OutputDebugString("Lock Count error\n");
	M_ASSERT(0, "?");

Invalid5:
	Ret = ESIMap;

Invalid3:

	return (void *)Ret;

/*

80028EC9   push        ebp
80028ECA   mov         ebp,esp
80028ECC   mov         ecx,dword ptr [ebp+8]
80028ECF   mov         eax,ecx
80028ED1   shr         eax,14h
80028ED4   and         eax,0FFCh
80028ED9   sub         eax,3FD00000h
80028EDE   mov         eax,dword ptr [eax]
80028EE0   test        al,1
80028EE2   push        esi
80028EE3   je          (Invalid0)
80028EE5   test        al,al
80028EE7   js          (Invalid2)
80028EE9   mov         eax,ecx
80028EEB   shr         eax,0Ah
80028EEE   and         eax,(003ffffc)
80028EF3   sub         eax,40000000h
80028EF8   mov         eax,dword ptr [eax]
80028EFA   test        al,1
80028EFC   jne         (Invalid1)
Invalid0:
80028EFE   push        ecx
80028EFF   push        offset string "MmGetPhysicalAddress failed, bas"... (80012810)
80028F04   call        DbgPrint (8002ed34)
80028F09   pop         ecx
80028F0A   pop         ecx
80028F0B   xor         eax,eax
80028F0D   jmp         InvalidAddress+5Dh (Invalid3)
Invalid1:
80028F0F   and         ecx,0FFFh
80028F15   jmp         InvalidAddress+1Fh (Invalid4)
Invalid2:
80028F17   and         ecx,(003fffff)
Invalid4:
80028F1D   and         eax,0FFFFF000h
80028F22   mov         esi,ecx
80028F24   add         esi,eax
80028F26   mov         eax,esi
80028F28   shr         eax,0Ch
80028F2B   cmp         eax,dword ptr [_MmHighestPhysicalPage (8004fff8)]
80028F31   ja          InvalidAddress+5Bh (Invalid5)
80028F33   lea         eax,[eax*4-7C010000h]
80028F3A   mov         eax,dword ptr [eax]
80028F3C   test        al,1
80028F3E   jne         InvalidAddress+5Bh (Invalid5)
80028F40   test        ax,(80028f42)
80028F44   jne         InvalidAddress+5Bh (Invalid5)
80028F46   push        0
80028F48   push        5Ch
80028F4A   push        offset string "d:\\xbox\\private\\ntos\\mmx\\physica"... (800127ec)
80028F4F   push        offset string "PageFrame->Busy.LockCount != 0" (800127cc)
80028F54   call        RtlAssert (8002fb5f)
Invalid5:
80028F59   mov         eax,esi
Invalid3:
80028F5B   pop         esi
80028F5C   pop         ebp
80028F5D   ret         4
*/
}

struct SDescriptorEntry
{
	uint32 Offset_Byte0:8;
	uint32 Offset_Byte1:8;
	uint32 LSB_HandlerSelector:8;
	uint32 MSB_HandlerSelector:8;
	uint32 Reserved:8;
	uint32 AccessByte:8;
	uint32 Offset_Byte2:8;
	uint32 Offset_Byte3:8;
};

static SDescriptorEntry *g_DescriptorTable;

static DWORD g_LastFunc;

void __declspec(naked) PageFaultExceptionHandler(void)
{
	__asm
	{
		push eax
		push cs
		pushfd
		call g_LastFunc
		iret
	}
}


class CDA_MemoryManagerVirtual : public CDA_MemoryManager
{
public:
	CDA_MemoryManagerVirtual()
	{
	//	LPTOP_LEVEL_EXCEPTION_FILTER 
		
//		XapiCurrentTopLevelFilter = ExceptionFilter;
//		SetUnhandledExceptionFilter(ExceptionFilter);
//		__CxxSetUnhandledExceptionFilter();
	//	BYTE Code[] = {0xE8,0x32,0x64,0x00,0x00};
	//	VirtualProtect((void *)(0x80025DF4 & (~4095)), 8192, PAGE_EXECUTE_READWRITE, NULL);

	//	memcpy(((BYTE *)0x80025DF4), Code, sizeof(Code));

/*		struct SDescriptorEntryDesc
		{
			uint16 Limit;
			uint32 Address;
		};
		volatile SDescriptorEntryDesc DescriptorTableDesc;

		__asm
		{
			SIDT [DescriptorTableDesc]
		}
	//	memcpy(DescriptorTable,(void *)KiIDT,DescriptorTableDesc.Limit);

//		Typefromhell GetPhysicalAddress2 = NULL;
//		*((DWORD *)(&GetPhysicalAddress2)) = (DWORD)GetPhysicalAddressNaked;

		g_DescriptorTable = (SDescriptorEntry *)XPhysicalAlloc(256*sizeof(SDescriptorEntry), MAXULONG_PTR, 0, PAGE_READWRITE);
		
		memcpy(g_DescriptorTable,(void *)0x8004aec8,DescriptorTableDesc.Limit + 8);
		g_LastFunc = 
			g_DescriptorTable[13].Offset_Byte0
			|
			g_DescriptorTable[13].Offset_Byte1 << 8
			|
			g_DescriptorTable[13].Offset_Byte2 << 16
			|
			g_DescriptorTable[13].Offset_Byte3 << 24
			;
		
		DWORD Handler = (DWORD)PageFaultExceptionHandler;
//		g_DescriptorTable[13].Offset_Byte0 = Handler & 0xff;
//		g_DescriptorTable[13].Offset_Byte1 = (Handler >> 8) & 0xff;
//		g_DescriptorTable[13].Offset_Byte2 = (Handler >> 16) & 0xff;
//		g_DescriptorTable[13].Offset_Byte3 = (Handler >> 24) & 0xff;

//		memcpy((void *)0x8004aec8,g_DescriptorTable,DescriptorTableDesc.Limit);
//		void * Test = GetPhysicalAddress2((void *)0x8004aec8);

		Typefromhell GetPhysicalAddress2 = NULL;
		*((DWORD *)(&GetPhysicalAddress2)) = *((DWORD*)_imp__MmGetPhysicalAddress);
		DWORD Test = (DWORD)GetPhysicalAddress2((void *)g_DescriptorTable);
		DWORD Test2 = (DWORD)GetPhysicalAddress2((void *)0x8004aec8);
		
//		DWORD Test = (DWORD)GetPhysicalAddress(g_DescriptorTable);
		DescriptorTableDesc.Address = Test;
		__asm
		{
//			sfence
//			LIDT [DescriptorTableDesc]
		}

//		void * Test = ((Typefromhell)_imp__MmGetPhysicalAddress)((void *)0x8004aec8);
//		Test = (*_imp__MmGetVirtualAddress)((void *)DescriptorTableDesc.Address);
		
		*/

				

//		*((int *)(NULL)) = 0;

	}

	~CDA_MemoryManagerVirtual()
	{
		
	}
	
	
	virtual void *AllocHeap(uint32 Size)
	{
		return VirtualAlloc(NULL, Size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	}
	
	virtual void FreeHeap(void *Block)
	{
		VirtualFree(Block, 0, MEM_RELEASE);
	}
	
	
	static LONG __stdcall ExceptionFilter(EXCEPTION_POINTERS *ExceptionInfo)
	{

		return EXCEPTION_CONTINUE_SEARCH;

	}

};

#endif

/*
8002C22B   push        ebp
8002C22C   mov         ebp,esp
8002C22E   sub         esp,10h
8002C231   cmp         byte ptr [_KiPCR+24h (8004ac30)],2
8002C238   push        edi
8002C239   jb          MmAccessFault+1Ch (8002c247)
8002C23B   mov         dword ptr [status],0D0000006h
8002C242   jmp         ReturnStatusCode+0Bh (8002c308)
8002C247   mov         edi,dword ptr [VirtualAddress]
8002C24A   cmp         edi,10000h
8002C250   mov         dword ptr [status],0C0000005h
8002C257   jb          ReturnStatusCode+0Bh (8002c308)
8002C25D   cmp         edi,7FFDFFFFh
8002C263   ja          ReturnStatusCode+0Bh (8002c308)
8002C269   push        ebx
8002C26A   mov         ebx,offset _MmAddressSpaceLock (8004b148)
8002C26F   push        ebx
8002C270   call        _RtlEnterCriticalSectionAndRegion@4 (80030540)
8002C275   call        _KeRaiseIrqlToDpcLevel@0 (8001ca60)
8002C27A   mov         byte ptr [OldIrql],al
8002C27D   mov         eax,edi
8002C27F   shr         eax,14h
8002C282   and         eax,0FFCh
8002C287   test        byte ptr [eax-3FD00000h],1
8002C28E   je          MmAccessFault+0BFh (8002c2ea)
8002C290   shr         edi,0Ah
8002C293   and         edi,offset CTerrainPatch::GetWallMarkPolys+5CEh (003ffffc)
8002C299   sub         edi,40000000h
8002C29F   push        esi
8002C2A0   mov         esi,dword ptr [edi]
8002C2A2   test        esi,esi
8002C2A4   mov         dword ptr [PointerPte],edi
8002C2A7   je          MmAccessFault+0BEh (8002c2e9)
8002C2A9   mov         ecx,esi
8002C2AB   and         ecx,21Bh
8002C2B1   mov         dword ptr [PteProtectionMask],ecx
8002C2B4   call        MiDecodePteProtectionMask (80029e7d)
8002C2B9   test        ah,1
8002C2BC   je          MmAccessFault+0BEh (8002c2e9)
8002C2BE   mov         ecx,eax
8002C2C0   and         ecx,0FFFFFEFFh
8002C2C6   lea         edx,[PteProtectionMask]
8002C2C9   call        MiMakePteProtectionMask (80029d86)
8002C2CE   and         esi,0FFFFFDE4h
8002C2D4   or          esi,dword ptr [PteProtectionMask]
8002C2D7   mov         dword ptr [edi],esi
8002C2D9   mov         eax,dword ptr [PointerPte]
8002C2DC   shl         eax,0Ah
8002C2DF   invlpg      [eax]
8002C2E2   mov         dword ptr [status],80000001h
8002C2E9   pop         esi
8002C2EA   mov         cl,byte ptr [OldIrql]
8002C2ED   call        @KfLowerIrql@4 (8001caa0)
8002C2F2   push        ebx
8002C2F3   call        _RtlLeaveCriticalSectionAndRegion@4 (800305b0)
8002C2F8   cmp         dword ptr [status],0
8002C2FC   pop         ebx
ReturnStatusCode:
8002C2FD   jge         ReturnStatusCode+3Ah (8002c337)
8002C2FF   cmp         dword ptr [status],80000001h
8002C306   je          ReturnStatusCode+3Ah (8002c337)
8002C308   mov         eax,dword ptr [TrapInformation]
8002C30B   test        eax,eax
8002C30D   je          ReturnStatusCode+28h (8002c325)
8002C30F   push        dword ptr [eax+3Ch]
8002C312   push        eax
8002C313   push        dword ptr [VirtualAddress]
8002C316   push        offset string "MM: page fault touching %p, trap"... (80013744)
8002C31B   call        DbgPrint (8002ed34)
8002C320   add         esp,10h
8002C323   jmp         ReturnStatusCode+3Ah (8002c337)
8002C325   push        0
8002C327   push        dword ptr [VirtualAddress]
8002C32A   push        offset string "MM: page fault touching %p, trap"... (80013718)
8002C32F   call        DbgPrint (8002ed34)
8002C334   add         esp,0Ch
8002C337   mov         eax,dword ptr [status]
8002C33A   pop         edi
8002C33B   leave
8002C33C   ret         0Ch

  */
// Overide from hell... or the XBox API outputs a whole lot as output debug string
//WINBASEAPI LONG WINAPI UnhandledExceptionFilter(IN struct _EXCEPTION_POINTERS *ExceptionInfo)
//{
	//return CDA_MemoryManagerVirtual::ExceptionFilter(ExceptionInfo);
//}




#endif	// PLATFORM_XBOX


#ifdef PLATFORM_XBOX1

extern "C"
{
/*	__declspec(noinline) int __cdecl _heap_init (int mtflag)
	{
		MRTC_CreateObjectManager();
		return 1;
	}*/
//#pragma init_seg(user)

	/*
	#pragma data_seg(".CRT$RIZ")
	void *__xri_z[] = { 0 };

	#pragma data_seg(".CRT$RIA")
	void *__xri_a[] = { 0 };

	#pragma data_seg(".CRT$XIZ")
	void *__xi_z[] = { 0 };

	#pragma data_seg(".CRT$XIA")
	void *__xi_a[] = { 0 };

	#pragma data_seg(".CRT$XCZ")
	void *__xc_z[] = { 0 };

	#pragma data_seg(".CRT$XCA")
	void *__xc_a[] = { 0 };
	#pragma data_seg()
	
	__declspec(naked) int __cdecl _rtinit(void *)
	{	
	//(5373A0h) //
		//(5373ACh) 
		__asm
		{
			push        esi  
			push        edi  
			mov         eax,offset __xri_a 
			mov         edi,offset __xri_z 
			cmp         eax,edi 
			mov         esi,eax 
			jae         ExitRtInit
		LabelLoop:
			mov         eax,dword ptr [esi] 
			test        eax,eax 
			je          Label0
			cmp         eax,0FFFFFFFFh 
			je          Label0
			call        eax  
			Label0:
			add         esi,4 
			cmp         esi,edi 
			jb          LabelLoop
			ExitRtInit:
			pop         edi  
			pop         esi  
			ret              
		}
	}*/
}
#endif


#ifdef PLATFORM_XBOX

#ifdef MRTC_MEMMANAGEROVERRIDE


extern "C" LPVOID WINAPI XMemAlloc(SIZE_T dwSize, DWORD dwAllocAttributes)
{
	XALLOC_ATTRIBUTES Attr = (XALLOC_ATTRIBUTES &)dwAllocAttributes;

	if (Attr.dwMemoryType == XALLOC_MEMTYPE_HEAP)
	{
		CDA_MemoryManager * MemMan = MRTC_GetMemoryManager();
		if (Attr.dwHeapTracksAttributes)
		{
#ifdef M_SUPPORTMEMORYDEBUG
			void *pMem = MemMan->AllocDebug(dwSize + 16, 1, __FILE__, __LINE__);
#else
			void *pMem = MemMan->Alloc(dwSize + 16);
#endif
			if (pMem)
			{
				mint Size = MemMan->MemorySize(pMem);
				if (Attr.dwZeroInitialize)
					memset(pMem, 0, Size);
//				gf_RDSendHeapAlloc(pMem, Size, MemMan, 0);
				*((uint32 *)pMem) = dwAllocAttributes;
				return (void *)((mint)pMem + 16);
			}
			else
				return NULL;
		}
		else
		{
#ifdef M_SUPPORTMEMORYDEBUG
			void *pMem = MemMan->AllocDebug(dwSize, 1, __FILE__, __LINE__);
#else
			void *pMem = MemMan->Alloc(dwSize);
#endif
			if (pMem)
			{
				mint Size = MemMan->MemorySize(pMem);
				if (Attr.dwZeroInitialize)
					memset(pMem, 0, Size);
//				gf_RDSendHeapAlloc(pMem, Size, MemMan, 0);
				return pMem;
			}
			else
				return NULL;
		}
	}
	else
	{
		M_ASSERT(Attr.dwMemoryType == XALLOC_MEMTYPE_PHYSICAL, "");
		void *pMem = XMemAllocDefault(dwSize, dwAllocAttributes);
		if (pMem)
		{
			uint32 Size = XMemSizeDefault(pMem, dwAllocAttributes);
	//		if ((mint)pMem & 0x80000000)
			gf_RDSendPhysicalAlloc(pMem, Size, 1, gf_RDGetSequence(), 0);
	//		else
	//			gf_RDSendPhysicalAlloc(pData, dwSize, 0, 0);

			return pMem;
		}
		else
			return NULL;
	}
    
}

extern "C" VOID WINAPI XMemFree(PVOID pAddress, DWORD dwAllocAttributes)
{
	XALLOC_ATTRIBUTES Attr = (XALLOC_ATTRIBUTES &)dwAllocAttributes;

	if (Attr.dwMemoryType == XALLOC_MEMTYPE_HEAP)
	{
		CDA_MemoryManager * MemMan = MRTC_GetMemoryManager();
		if (Attr.dwHeapTracksAttributes)
		{
			void *pMem = (void *)((mint)pAddress - 16);
			MemMan->Free(pMem);
//			if (pMem)
//				gf_RDSendHeapFree(pMem, MemMan);
		}
		else
		{
			void *pMem = pAddress;
			MemMan->Free(pMem);
//			if (pMem)
//				gf_RDSendHeapFree(pMem, MemMan);
		}
	}
	else
	{
		M_ASSERT(Attr.dwMemoryType == XALLOC_MEMTYPE_PHYSICAL, "");
		XMemFreeDefault(pAddress, dwAllocAttributes);
		if (pAddress)
			gf_RDSendPhysicalFree(pAddress, 1, gf_RDGetSequence());
	}
}

extern "C" SIZE_T WINAPI XMemSize(PVOID pAddress,DWORD dwAllocAttributes)
{
	XALLOC_ATTRIBUTES Attr = (XALLOC_ATTRIBUTES &)dwAllocAttributes;

	if (Attr.dwMemoryType == XALLOC_MEMTYPE_HEAP)
	{
		CDA_MemoryManager * MemMan = MRTC_GetMemoryManager();
		if (Attr.dwHeapTracksAttributes)
		{
			void *pMem = (void *)((mint)pAddress - 16);
			return MemMan->MemorySize(pMem) - 16;
		}
		else
		{
			void *pMem = pAddress;
			return MemMan->MemorySize(pMem);
		}
	}
	else
		return XMemSizeDefault(pAddress, dwAllocAttributes);
}

/*
extern "C" VOID WINAPI XSetAttributesOnHeapAlloc(PVOID pBaseAddress, DWORD dwAllocAttributes)
{
	XALLOC_ATTRIBUTES Attr = (XALLOC_ATTRIBUTES &)dwAllocAttributes;

	if (Attr.dwMemoryType == XALLOC_MEMTYPE_HEAP)
	{
		CDA_MemoryManager * MemMan = MRTC_GetMemoryManager();
		if (Attr.dwHeapTracksAttributes)
		{
			void *pMem = (void *)((mint)pBaseAddress - 4);
			*((uint32 *)pMem) = dwAllocAttributes;
		}
	}
}

extern "C" DWORD WINAPI XGetAttributesOnHeapAlloc(PVOID pBaseAddress)
{
	CDA_MemoryManager * MemMan = MRTC_GetMemoryManager();
	void *pMem = (void *)((mint)pBaseAddress - 4);
	return *((uint32 *)pMem);
}
*/

#endif

#else


#ifdef MRTC_MEMMANAGEROVERRIDE //Enables DA_ memory manager




#undef malloc
#undef free
#undef realloc
#undef calloc
#undef memalign
#undef _msize
#undef _expand

#undef _malloc_dbg
#undef _free_dbg
#undef _realloc_dbg
#undef _calloc_dbg
#undef memalign_dbg
#undef _msize_dbg
#undef _expand_dbg
#undef _nh_malloc
#undef _heap_alloc


#undef _CrtSetBreakAlloc
#undef _CrtSetDbgBlockType
#undef _CrtSetAllocHook
#undef _CrtCheckMemory
#undef _CrtSetDbgFlag
#undef _CrtDoForAllClientObjects
#undef _CrtIsValidPointer
#undef _CrtIsValidHeapPointer
#undef _CrtIsMemoryBlock
#undef _CrtReportBlockType
#undef _CrtSetDumpClient
#undef _CrtMemCheckpoint
#undef _CrtMemDifference
#undef _CrtMemDumpAllObjectsSince
#undef _CrtDumpMemoryLeaks
#undef _CrtMemDumpStatistics

#if defined(PLATFORM_DOLPHIN)

	#ifdef __cplusplus        /* hh  971206 */
		#ifdef _MSL_USING_NAMESPACE
			namespace std {
		#endif
		extern "C" {
	#endif
#elif defined(PLATFORM_PS3)
namespace std
{
	extern "C"
	{
#else

	extern "C"
	{
		
#endif
	
#ifndef _CRTNOALIAS
#define _CRTNOALIAS
#endif
#ifndef _CRTRESTRICT
#define _CRTRESTRICT
#endif

	_CRTIMP _CRTNOALIAS _CRTRESTRICT void * M_CDECL malloc(mint );
	_CRTIMP _CRTNOALIAS void M_CDECL free(void *);
	_CRTIMP _CRTNOALIAS _CRTRESTRICT void * M_CDECL realloc(void *, mint);
	_CRTIMP _CRTNOALIAS _CRTRESTRICT void * M_CDECL calloc(mint, mint);
	_CRTIMP _CRTNOALIAS _CRTRESTRICT void * M_CDECL memalign(mint, mint);
	
	/*************************************************************************************************\
	|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
	| malloc
	|__________________________________________________________________________________________________
	\*************************************************************************************************/
	
	_CRTIMP _CRTNOALIAS _CRTRESTRICT void * M_CDECL malloc (mint sz)
	{		
		void * addr = M_ALLOC(sz);		
		return addr;
	}

	void * M_CDECL _malloc_base (mint sz)
	{		
		void * addr = M_ALLOC(sz);		
		return addr;
	}

#ifndef M_RTM
	void * M_CDECL _malloc_dbg (mint sz, int BlockType, const char *Filename, int Line) 
	{
		CDA_MemoryManager * MemMan = MACRO_GetDefaultMemoryManager();
#ifdef M_SUPPORTMEMORYDEBUG
		void * addr = MemMan->AllocDebug(sz, BlockType, Filename, Line);
#else
		void * addr = MemMan->Alloc(sz);
#endif
		return addr;
	}
#endif

/*
_GetProcessHeap@0 
_GlobalAlloc@8 
_GlobalReAlloc@12 
_HeapCompact@8 
_HeapCreate@12 
_HeapDestroy@4 
_HeapFree@12 
_HeapLock@4 
_HeapUnlock@4 
_HeapValidate@12 
_HeapWalk@8 
_LocalAlloc@8 
_LocalFree@4 
_LocalHandle@4 
_LocalLock@4 
_LocalReAlloc@12 
_LocalSize@4 
_LocalUnlock@4 
_RtlAllocateHeap@12 
_RtlCompactHeap@8 
_RtlCreateHeap@24 
_RtlDestroyHeap@4 
_RtlFreeHeap@12 
_RtlLockHeap@4 
_RtlReAllocateHeap@16 
_RtlRip@12 
_RtlSizeHeap@12 
_RtlUnlockHeap@4 
_RtlValidateHeap@12 
_RtlWalkHeap@8 
_SetLastError@4 
_XapiSetLastNTError@4 

_XSetAttributesOnHeapAlloc@8 
_XGetAttributesOnHeapAlloc@4

??_C@_0CI@CDFKIAGP@LocalAlloc?$CI?$CJ?5invalid?5parameter?5?$CI@ 
??_C@_0CK@DAJFENGH@LocalReAlloc?$CI?$CJ?5invalid?5parameter@  
??_C@_0CJ@FOEMNOKO@GlobalAlloc?$CI?$CJ?5invalid?5parameter?5@ 
??_C@_0CL@GOKHKKIN@GlobalReAlloc?$CI?$CJ?5invalid?5paramete@  

_GetProcessHeap@0 
_GlobalAlloc@8 
_GlobalReAlloc@12 
_HeapCompact@8 
_HeapCreate@12 
_HeapDestroy@4 
_HeapFree@12 
_HeapLock@4 
_HeapUnlock@4 
_HeapValidate@12 
_HeapWalk@8 
_LocalAlloc@8 
_LocalFree@4 
_LocalHandle@4 
_LocalLock@4 
_LocalReAlloc@12 
_LocalSize@4 
_LocalUnlock@4 
_RtlAllocateHeap@12 
_RtlCompactHeap@8 
_RtlCreateHeap@24 
_RtlDestroyHeap@4 
_RtlFreeHeap@12 
_RtlLockHeap@4 
_RtlReAllocateHeap@16 
_RtlRip@12 
_RtlSizeHeap@12 
_RtlUnlockHeap@4 
_RtlValidateHeap@12 
_RtlWalkHeap@8 

_SetLastError@4 
_XapiProcessHeap 
_XapiSetLastNTError@4 

_XGetAttributesOnHeapAlloc@4 
_XSetAttributesOnHeapAlloc@8 


??_C@_0CI@CDFKIAGP@LocalAlloc?$CI?$CJ?5invalid?5parameter?5?$CI@ 
??_C@_0CJ@FOEMNOKO@GlobalAlloc?$CI?$CJ?5invalid?5parameter?5@ 
??_C@_0CK@DAJFENGH@LocalReAlloc?$CI?$CJ?5invalid?5parameter@ 
??_C@_0CL@GOKHKKIN@GlobalReAlloc?$CI?$CJ?5invalid?5paramete@ 


*/
	

#ifdef PLATFORM_XBOX1


// Fixed this.... This should only be active on the XBOX, as its not working on PC due to these functions beeing linked as dll linkage
#ifndef MRTC_DEFAULTMAINHEAP
	HANDLE XapiProcessHeap = NULL;

	HANDLE WINAPI HeapCreate(DWORD flOptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize)
	{
		MRTC_CreateObjectManager();
		static int Dummy = 0;
//		if (Dummy == 1)
//			M_ASSERT(0,"");

		CDA_MemoryManager * MemMan = MACRO_GetDefaultMemoryManager();
		++Dummy;
		return MemMan;
	}
	

	HLOCAL WINAPI LocalAlloc(UINT uFlags,SIZE_T uBytes)
	{
		CDA_MemoryManager * MemMan = MACRO_GetDefaultMemoryManager();
		void * addr = MemMan->Alloc(uBytes);	
		if (uFlags & LMEM_ZEROINIT)
			memset(addr, 0, uBytes);
		return addr;
	}

	HLOCAL WINAPI LocalFree(HLOCAL hMem)
	{
		CDA_MemoryManager * MemMan = MACRO_GetDefaultMemoryManager();
		MemMan->Free(hMem);	
		return NULL;
	}

	HLOCAL WINAPI LocalHandle(LPCVOID pMem)
	{
		return (HLOCAL)pMem;
	}

	LPVOID WINAPI LocalLock(HLOCAL hMem)
	{
		CDA_MemoryManager * MemMan = MACRO_GetDefaultMemoryManager();
		MemMan->GetLock().Lock();
		return hMem;
	}

	BOOL WINAPI LocalUnlock(HLOCAL hMem)
	{
		CDA_MemoryManager * MemMan = MACRO_GetDefaultMemoryManager();
		MemMan->GetLock().Unlock();
		return true;
	}	

	HLOCAL WINAPI LocalReAlloc(HLOCAL hMem,SIZE_T uBytes,UINT uFlags)
	{
		CDA_MemoryManager * MemMan = MACRO_GetDefaultMemoryManager();

		int Size = MemMan->MemorySize(hMem);

		if (uFlags & LMEM_MOVEABLE)
		{
			hMem = MemMan->Realloc(hMem, uBytes);
		}
		else
		{

			if (uBytes > Size)
				return NULL;
		}

		if (uFlags & LMEM_ZEROINIT)
			memset(((uint8 *)hMem) + Size, 0, uBytes - Size); 

		return hMem;
	}

	SIZE_T WINAPI LocalSize(HLOCAL hMem)
	{
		CDA_MemoryManager * MemMan = MACRO_GetDefaultMemoryManager();
		return MemMan->MemorySize(hMem);
	}

	
	struct RTL_HEAP_PARAMETERS
	{
		DWORD Length;
		DWORD SegmentReserve;
		DWORD SegmentCommit;
		DWORD DeCommitFreeBlockThreshold;
		DWORD DeCommitTotalFreeThreshold;
		DWORD MaximumAllocationSize;
		DWORD VirtualMemoryThreshold;
		DWORD InitialCommit;
		DWORD InitialReserve;
		DWORD CommitRoutine;
		DWORD Reserved[2];
	};

	HANDLE WINAPI RtlCreateHeap(unsigned long Param1, void * Param2, unsigned long Param3, unsigned long Param4, void * Param5, RTL_HEAP_PARAMETERS * Param6)
	{
		return HeapCreate(0,0,0);
	}
	
	BOOL WINAPI RtlDestroyHeap(IN OUT HANDLE hHeap)
	{
		M_ASSERT(0,"");
		return true;
	}
	
	SIZE_T WINAPI RtlCompactHeap(IN HANDLE hHeap,IN DWORD dwFlags)
	{
		M_ASSERT(0,"");
		return 0;
	}	
	
	SIZE_T WINAPI RtlDebugCompactHeap(IN HANDLE hHeap,IN DWORD dwFlags)
	{
		M_ASSERT(0,"");
		return 0;
	}	
	
	BOOL WINAPI RtlLockHeap(IN HANDLE hHeap)
	{
		M_ASSERT(0,"");
		return false;
	}
	
	BOOL WINAPI RtlUnlockHeap(IN HANDLE hHeap)
	{
		M_ASSERT(0,"");
		return false;
	}

	BOOL WINAPI RtlValidateHeap(HANDLE hHeap,DWORD dwFlags,LPCVOID lpMem)
	{
		M_ASSERT(0,"");
		return false;
	}

	
	BOOL WINAPI RtlWalkHeap(IN HANDLE hHeap, IN OUT LPPROCESS_HEAP_ENTRY lpEntry)
	{
		M_ASSERT(0,"");
		return false;
	}

	BOOL WINAPI RtlDebugWalkHeap(IN HANDLE hHeap, IN OUT LPPROCESS_HEAP_ENTRY lpEntry)
	{
		M_ASSERT(0,"");
		return false;
	}

	BOOL WINAPI RtlZeroHeap(IN HANDLE hHeap, IN DWORD Testar)
	{
		M_ASSERT(0,"");
		return false;
	}
	
	LPVOID WINAPI RtlAllocateHeap( IN HANDLE hHeap, IN DWORD dwFlags, IN SIZE_T dwBytes)
	{
		//M_ASSERT(0,"");
		CDA_MemoryManager * MemMan = MACRO_GetDefaultMemoryManager();
		void * addr = MemMan->Alloc(dwBytes);

		if(dwFlags&HEAP_ZERO_MEMORY)
			memset(addr, 0, dwBytes);
		return addr;
	}
	
	LPVOID WINAPI RtlReAllocateHeap(IN HANDLE hHeap,IN DWORD dwFlags,IN LPVOID lpMem,IN SIZE_T dwBytes)
	{
		M_ASSERT(0,"");
		return NULL;
	}
	
	BOOL WINAPI RtlFreeHeap(IN HANDLE hHeap,IN DWORD dwFlags,IN LPVOID lpMem)
	{
		//M_ASSERT(0,"");
		CDA_MemoryManager * MemMan = MACRO_GetDefaultMemoryManager();
		MemMan->Free(lpMem);
		
		return false;
	}
	
	BOOL WINAPI RtlFreeHeapSlowly(IN HANDLE hHeap,IN DWORD dwFlags,IN LPVOID lpMem)
	{
		M_ASSERT(0,"");
		
		return false;
	}

	SIZE_T WINAPI RtlSizeHeap(IN HANDLE hHeap,IN DWORD dwFlags,IN LPCVOID lpMem)
	{
		M_ASSERT(0,"");
		return NULL;
	}


	
	SIZE_T WINAPI HeapCompact(HANDLE hHeap, DWORD dwFlags)
	{
		return RtlCompactHeap(hHeap, dwFlags);
	}

	BOOL WINAPI HeapDestroy(HANDLE hHeap)
	{
		return RtlDestroyHeap(hHeap);
	}

	BOOL WINAPI HeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem)
	{
		return RtlFreeHeap(hHeap, dwFlags, lpMem);
	}

	BOOL WINAPI HeapLock(HANDLE hHeap)
	{
		return RtlLockHeap(hHeap);
	}

	BOOL WINAPI HeapUnlock(HANDLE hHeap)
	{
		return RtlUnlockHeap(hHeap);
	}

	BOOL WINAPI HeapValidate(HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem)
	{
		return RtlValidateHeap(hHeap, dwFlags, lpMem);
	}

	BOOL WINAPI HeapWalk(HANDLE hHeap, LPPROCESS_HEAP_ENTRY lpEntry)
	{
		return RtlWalkHeap(hHeap, lpEntry);
	}

#ifdef M_SUPPORTMEMORYDEBUG

	VOID WINAPI XSetAttributesOnHeapAlloc(PVOID pBaseAddress, DWORD dwAllocAttributes)
	{
	
		CDA_MemoryManager * MemMan = MACRO_GetDefaultMemoryManager();
		SDA_Defraggable *Block;
		Block = BlockFromMem(pBaseAddress);

		if (!Block->IsDebug())
		M_ASSERT(0,"");

		int Size = MemMan->GetBlockSize(Block);
		SDA_DefraggableDebug_Post *pPost = (SDA_DefraggableDebug_Post *)((uint32)Block + Size - sizeof(SDA_DefraggableDebug_Post));

		pPost->m_SequenceId = dwAllocAttributes;

	}

	DWORD WINAPI XGetAttributesOnHeapAlloc(PVOID pBaseAddress)
	{
	
		CDA_MemoryManager * MemMan = MACRO_GetDefaultMemoryManager();
		SDA_Defraggable *Block;
		Block = BlockFromMem(pBaseAddress);

		if (!Block->IsDebug())
		M_ASSERT(0,"");

		int Size = MemMan->GetBlockSize(Block);
		SDA_DefraggableDebug_Post *pPost = (SDA_DefraggableDebug_Post *)((uint32)Block + Size - sizeof(SDA_DefraggableDebug_Post));

		return pPost->m_SequenceId;

	}
#endif


#endif
#endif

	
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| calloc
|__________________________________________________________________________________________________
\*************************************************************************************************/
	
	void * M_CDECL calloc (mint nelem, mint elsize)
	{
		void * addr = M_ALLOC(nelem * elsize);
		// Zero out the malloc'd block.
		memset (addr, 0, nelem * elsize);
		return addr;
	}

#ifndef M_RTM
	void * M_CDECL _calloc_dbg (mint nelem, mint elsize, int BlockType, const char *Filename, int Line) 
	{
		CDA_MemoryManager * MemMan = MACRO_GetDefaultMemoryManager();
#ifdef M_SUPPORTMEMORYDEBUG
		void * addr = MemMan->AllocDebug(nelem * elsize, BlockType, Filename, Line);
#else
		void * addr = MemMan->Alloc(nelem * elsize);
#endif
		// Zero out the malloc'd block.
		memset (addr, 0, nelem * elsize);
		return addr;
	}
#endif
		
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| free
|__________________________________________________________________________________________________
\*************************************************************************************************/

	void M_CDECL free (void * ptr)
	{
		CDA_MemoryManager * MemMan = MACRO_GetDefaultMemoryManager();
		MemMan->Free(ptr);
	}

	void M_CDECL _free_base (void * ptr)
	{
		CDA_MemoryManager * MemMan = MACRO_GetDefaultMemoryManager();
		MemMan->Free(ptr);
	}

	void M_CDECL _free_dbg (void * ptr, int) 
	{
		CDA_MemoryManager * MemMan = MACRO_GetDefaultMemoryManager();
		MemMan->Free(ptr);
	}	
	
	
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| memalign
|__________________________________________________________________________________________________
\*************************************************************************************************/

	void * M_CDECL memalign (mint alignment, mint size)
	{
		CDA_MemoryManager * MemMan = MACRO_GetDefaultMemoryManager();
		void * addr = MemMan->AllocAlign(size, alignment);

		return addr;
	}	
	
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| realloc
|__________________________________________________________________________________________________
\*************************************************************************************************/

	void * M_CDECL realloc (void * ptr, mint sz)
	{
		return M_REALLOC(ptr, sz);
	}
	
	void * M_CDECL _realloc_base (void * ptr, mint sz)
	{
		return M_REALLOC(ptr, sz);
	}
	
	
#ifndef M_RTM
	void * M_CDECL _realloc_dbg (void * ptr, mint sz, int BlockType, const char *Filename, int Line) 
	{
		CDA_MemoryManager *MemMan = MACRO_GetDefaultMemoryManager();

#ifdef M_SUPPORTMEMORYDEBUG
		return MemMan->ReallocDebug(ptr, sz, BlockType, Filename, Line);
#else
		return MemMan->Realloc(ptr, sz);
#endif
	}
#endif
	
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| _msize
|__________________________________________________________________________________________________
\*************************************************************************************************/
	
	mint M_CDECL _msize(void *mem)
	{
		CDA_MemoryManager *MemMan = MACRO_GetDefaultMemoryManager();
		return MemMan->MemorySize(mem);
	}

	mint M_CDECL _msize_base(void *mem)
	{
		CDA_MemoryManager *MemMan = MACRO_GetDefaultMemoryManager();
		return MemMan->MemorySize(mem);
	}

	mint M_CDECL _msize_dbg (void * ptr, int blockType) 
	{
		CDA_MemoryManager *MemMan = MACRO_GetDefaultMemoryManager();
		return MemMan->MemorySize(ptr);
	}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| _expand
|__________________________________________________________________________________________________
\*************************************************************************************************/
	
	void* M_CDECL _expand(void *mem , mint size)
	{
		return M_REALLOC(mem, size);
	}
	
	void* M_CDECL _expand_base(void *mem , mint size)
	{
		return M_REALLOC(mem, size);
	}
	
#ifndef M_RTM
	void* M_CDECL _expand_dbg(void *userData, mint newSize, int blockType, const char *filename, int linenumber)
	{
		CDA_MemoryManager *MemMan = MACRO_GetDefaultMemoryManager();

#ifdef M_SUPPORTMEMORYDEBUG
		return MemMan->ReallocDebug(userData, newSize, blockType, filename, linenumber);
#else
		return MemMan->Realloc(userData, newSize);
#endif
	}
#endif
	
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| _nh_malloc
|__________________________________________________________________________________________________
\*************************************************************************************************/
	
	void * M_CDECL _nh_malloc (
        mint nSize,
        int nhFlag
        )
	{
		return malloc (nSize);
	}
	
	void * M_CDECL _nh_malloc_base (
        mint nSize,
        int nhFlag
        )
	{
		return malloc (nSize);
	}
	
#ifndef M_RTM
	void * M_CDECL _nh_malloc_dbg (
        mint nSize,
        int nhFlag,
        int nBlockUse,
        const char * szFileName,
        int nLine
        )
	{
		return _malloc_dbg (nSize, nBlockUse, szFileName, nLine);
	}
#endif
	
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| _heap_alloc
|__________________________________________________________________________________________________
\*************************************************************************************************/
	
	void * M_CDECL _heap_alloc(
        mint nSize
        )
	{
		return malloc (nSize);
	}
	
	void * M_CDECL _heap_alloc_base(
        mint nSize
        )
	{
		return malloc (nSize);
	}
	
#ifndef M_RTM
	void * M_CDECL _heap_alloc_dbg(
        mint nSize,
        int nBlockUse,
        const char * szFileName,
        int nLine
        )
	{
		return _malloc_dbg (nSize, nBlockUse, szFileName, nLine);
	}
#endif
	
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| _free_lk
|__________________________________________________________________________________________________
\*************************************************************************************************/

	void M_CDECL _free_lk(
        void * pUserData
        )
	{
		free (pUserData);
	}
	
	void M_CDECL _free_lk_base(
        void * pUserData
        )
	{
		free (pUserData);
	}
	
	void M_CDECL _free_dbg_lk(
        void * pUserData,
        int nBlockUse
        )
	{
		free (pUserData);
	}
	
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Debug routines
|__________________________________________________________________________________________________
\*************************************************************************************************/
#if defined(PLATFORM_WIN)
	long M_CDECL _CrtSetBreakAlloc(
        long lNewBreakAlloc
        )
	{
        return 0;
	}
	
	void M_CDECL _CrtSetDbgBlockType(
        void * pUserData,
        int nBlockUse
        )
	{
	}
	
	typedef int (M_CDECL * _CRT_ALLOC_HOOK)(int, void *, mint, int, long, const unsigned char *, int);
	
	_CRT_ALLOC_HOOK M_CDECL _CrtSetAllocHook(
        _CRT_ALLOC_HOOK pfnNewHook
        )
	{
        return NULL;
	}
	
	
	int M_CDECL CheckBytes(
        unsigned char * pb,
        unsigned char bCheck,
        mint nSize
        )
	{
        return 1;
	}
	
	
	int M_CDECL _CrtCheckMemory(
        void
        )
	{
#ifdef M_SUPPORTMEMORYDEBUG
		CDA_MemoryManager *MemMan = MACRO_GetDefaultMemoryManager();
		
        return MemMan->CheckMemory();
#else
		return true;
#endif
	}
	
	int M_CDECL _CrtSetDbgFlag(
        int fNewBits
        )
	{
		return _CRTDBG_LEAK_CHECK_DF;
	}
	
	
	void M_CDECL _CrtDoForAllClientObjects(
        void (*pfn)(void *, void *),
        void * pContext
        )
	{
	}
	
	
	int M_CDECL _CrtIsValidPointer(
        const void * pv,
        unsigned int nBytes,
        int bReadWrite
        )
	{
        return (
            pv != NULL
#ifdef _WIN32
            && !IsBadReadPtr(pv, nBytes) &&
            (!bReadWrite || !IsBadWritePtr((LPVOID)pv, nBytes))
#endif  /* _WIN32 */
            );
	}
	
	int M_CDECL _CrtIsValidHeapPointer(
        const void * pUserData
        )
	{
		return true;
	}
	
	
	int M_CDECL _CrtIsMemoryBlock(
        const void * pUserData,
        unsigned int nBytes,
        long * plRequestNumber,
        char ** pszFileName,
        int * pnLine
        )
	{
		
        return NULL;
	}
	
	
	int M_CDECL _CrtReportBlockType(
		const void * pUserData
		)
	{
		return 0;
	}
	
	typedef void (M_CDECL * _CRT_DUMP_CLIENT)(void *, mint);
	
	_CRT_DUMP_CLIENT M_CDECL _CrtSetDumpClient(
        _CRT_DUMP_CLIENT pfnNewDump
        )
	{
        return NULL;
	}
	
	void M_CDECL _CrtMemCheckpoint(
        _CrtMemState * state
        )
	{
		
	}
	
	
	int M_CDECL _CrtMemDifference(
        _CrtMemState * state,
        const _CrtMemState * oldState,
        const _CrtMemState * newState
        )
	{
        return false;
	}
	
	
	void M_CDECL _CrtMemDumpAllObjectsSince(const _CrtMemState * state)
	{
		
	}
	
	int M_CDECL _CrtDumpMemoryLeaks(void)
	{
#ifndef M_STATICINIT
		MRTC_ObjectManager* pOM = MRTC_GetObjectManager();
		g_ObjectManagerContainer.Lock();
		
		int Ref = pOM->m_ModuleCount;

#ifndef MRTC_DLL
		if (MRTC_GetObjectManager()->m_pThreadPoolManagerInternal)
		{
			delete MRTC_GetObjectManager()->m_pThreadPoolManagerInternal;
			MRTC_GetObjectManager()->m_pThreadPoolManagerInternal = NULL;
		}
		if (MRTC_GetObjectManager()->m_pForgiveContextInternal)
		{
			delete MRTC_GetObjectManager()->m_pForgiveContextInternal;
			MRTC_GetObjectManager()->m_pForgiveContextInternal = NULL;
		}

		#ifdef MRTC_ENABLE_REMOTEDEBUGGER
		#ifndef MRTC_ENABLE_REMOTEDEBUGGER_STATIC
			if (MRTC_GetObjectManager()->m_pRemoteDebugger && mint(MRTC_GetObjectManager()->m_pRemoteDebugger) != 1)
			{
				MRTC_GetObjectManager()->m_pRemoteDebugger->Destroy();
				MRTC_GetObjectManager()->m_pRemoteDebugger->~MRTC_RemoteDebug();
				MRTC_GetObjectManager()->m_pRemoteDebugger = (MRTC_RemoteDebug *)1;
			}
		#endif
		#endif
#endif


		if (!Ref)
		{
#ifdef M_SUPPORTMEMORYDEBUG
			#ifdef MRTC_ENABLE_REMOTEDEBUGGER
			#ifndef MRTC_ENABLE_REMOTEDEBUGGER_STATIC
			if (MRTC_GetObjectManager()->m_pRemoteDebugger && mint(MRTC_GetObjectManager()->m_pRemoteDebugger) != 1)
				{
					MRTC_GetObjectManager()->m_pRemoteDebugger->Destroy();
					MRTC_GetObjectManager()->m_pRemoteDebugger->~MRTC_RemoteDebug();
					MRTC_GetObjectManager()->m_pRemoteDebugger = (MRTC_RemoteDebug *)1;
				}
			#endif
			#endif

			delete MRTC_GetObjectManager()->m_pByteStreamManager;
			MRTC_GetObjectManager()->m_pByteStreamManager = NULL;
			MRTC_GetObjectManager()->~MRTC_ObjectManager();
			MACRO_GetDefaultMemoryManager()->DisplayLeaks();
			g_MRTC_ModuleReference.~MRTC_ModuleRefCount();
			M_TRACEALWAYS("Memory leaks checked\n");
#endif
		}
		else if (Ref < 0)
		{
		//	M_TRACEALWAYS("Second unload of module ??\n");
		}

		g_ObjectManagerContainer.Unlock();
#endif
		
		return FALSE;   /* no leaked objects */
	}
	
	void M_CDECL _CrtMemDumpStatistics(const _CrtMemState * state)
	{
	}
#endif // PLATFORM_WIN
#if defined(PLATFORM_PS3) || defined(PLATFORM_DOLPHIN)
	}
#endif
}

#endif
#endif
