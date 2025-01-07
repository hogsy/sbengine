/*
Memory error tracking code for Nintendo Dolphin (GameCube).

  Author:			Steffo Sandberg
  Copyright:	Starbreeze Studios AB 2002
  
	History:
	020709:		Wrote this line. 17 days left to GC-Enclave-alpha!!!  =)
*/
#ifdef PLATFORM_DOLPHIN

#include <dolphin.h>
#include <OSMemory.h>

/*
ProtectionErrorHandler().

*/
bool g_bLoocking = false;
static void ProtectionErrorHandler( OSError error, OSContext *context, u32 dsisr, u32 dar )
{
	OSContext exceptionContext;
    OSClearContext(&exceptionContext);
    OSSetCurrentContext(&exceptionContext);
	
	if (!g_bLoocking)
	{
#pragma unused (error)
		//	g_ObjectManagerContainer.Lock();
		
		CSecureMem *pSecureMem = &MRTC_GetObjectManager()->m_MemoryProtect.m_Protect;
		
		u32 Addr = (u32)OSPhysicalToCached(dar);
		
		for (int i = 0; i < CSecure_MaxSections; ++i)
		{
			if (pSecureMem->m_Sections[i].m_bActive && Addr >= ((u32)pSecureMem->m_Sections[i].m_pMemPtr) && Addr < (((u32)pSecureMem->m_Sections[i].m_pMemPtr) + pSecureMem->m_Sections[i].m_nBytes))
			{
				OSReport("Hit addr\n");
				//pSecureMem->m_Sections[i].LaunchBadError(context, dsisr, dar);
			}		
		}	
	}
    OSSetCurrentContext(context);
    OSClearContext(&exceptionContext);
//	g_ObjectManagerContainer.Unlock();
}

/*
enable().

*/
void  CSecureSection::Enable( uint8 *_pPtr, uint32 _nBytes )
{
	if( this->m_bActive )
		return;
	
	M_ASSERT( _pPtr, "NULL POINTER!!!" );
	M_ASSERT( _nBytes, "LOGIC MISMATCH!!!" );
	
	this->m_pMemPtr = _pPtr;
	this->m_nBytes  = _nBytes;
	
	/*  Set the protection. (Only reading is allowed) */  
	OSProtectRange( this->m_section, m_pMemPtr, m_nBytes, OS_PROTECT_CONTROL_READ );
	this->m_bActive = true;
}

/*
disable().

*/
void  CSecureSection::Disable( void )
{
	if( !this->m_bActive )
		return;
	
	/*  Disable protection. */  
	OSProtectRange( this->m_section, m_pMemPtr, m_nBytes, OS_PROTECT_CONTROL_RDWR );
	this->m_bActive = false;
}

/*
Lock().
Turn off protection.
*/
uint8 *CSecureSection::Lock( void )
{
	g_bLoocking = true;
	OSProtectRange( this->m_section, m_pMemPtr, m_nBytes, OS_PROTECT_CONTROL_RDWR );
	g_bLoocking = false;
	return( this->m_pMemPtr );
}

/*
Unlock().
Turn protection back on.
*/
void  CSecureSection::Unlock( void )
{
	
	g_bLoocking = true;
	OSProtectRange( this->m_section, m_pMemPtr, m_nBytes, OS_PROTECT_CONTROL_READ );
	g_bLoocking = false;
}


/*
CSecureMem().
Constructor.
*/
CSecureMem::CSecureMem()
{
	/*  Set all sections. */  
	for( uint32 i=0; i<CSecure_MaxSections; i++ )
		this->m_Sections[i].SetSection( (CSecureMemSection)i );
	
	/*  Set error handler.  */  
	OSSetErrorHandler( OS_ERROR_PROTECTION, (OSErrorHandler)ProtectionErrorHandler );
	
	/*  Nothing has been done.  */  
}

/*
~CSecureMem().
Destructor.
*/
CSecureMem::~CSecureMem()
{
}

/*
Create().
Secure this portion of memory
*/
CSecureSection  *CSecureMem::Create( uint8 *_pPtr, uint32 _nBytes, CSecureMemSection section )
{
	if( section < 0 || section > CSecure_MaxSections )
		Error_static( "CSecureMem", "Section out of bounds" );
	
	this->m_Sections[ section ].Enable( _pPtr, _nBytes );
	return( &this->m_Sections[ section ] );
}

/*
destroy().

*/
void  CSecureMem::Destroy( CSecureMemSection section )
{
	if( section < 0 || section > CSecure_MaxSections )
		Error_static( "CSecureMem", "Section out of bounds" );
	
	this->m_Sections[ section ].Disable();
}

/*
*/
uint8 *CSecureMem::Lock( CSecureMemSection section )
{
	if( section < 0 || section > CSecure_MaxSections )
		Error_static( "CSecureMem", "Section out of bounds" );
	
	return( this->m_Sections[ section ].Lock() );
}

/*
*/
void  CSecureMem::Unlock( CSecureMemSection section )
{
	if( section < 0 || section > CSecure_MaxSections )
		Error_static( "CSecureMem", "Section out of bounds" );
	
	this->m_Sections[ section ].Unlock();
}


/*
*/
void  CSecureSection::LaunchBadError( OSContext *pContext, u32 dsisr, u32 dar )
{
	u32 i;
	u32 *p;
	
	OSReport( "\n\n----------------------- CSecureMem Report (0x%08x) ----------------------\n", pContext );
	OSReport(     "-----------------------      In secure section %d    ------------------------\n\n ", this->m_section );
	
	for( i = 0; i < 16; i++ )
		OSReport("r%-2d  = 0x%08x (%14d)  r%-2d  = 0x%08x (%14d)\n", i, pContext->gpr[i], pContext->gpr[i], i+16, pContext->gpr[ i+16 ], pContext->gpr[ i+16 ] );
	
	OSReport( "LR   = 0x%08x                   CR   = 0x%08x\n", pContext->lr, pContext->cr );
	OSReport( "SRR0 = 0x%08x                   SRR1 = 0x%08x\n", pContext->srr0, pContext->srr1 );
	OSReport( "DSISR= 0x%08x                   DAR  = 0x%08x\n", dsisr, dar );
	
	//  Dump stack crawl (at most 16 levels)
	OSReport("\nAddress:      Back Chain    LR Save\n");
	for( i = 0, p = (u32*) pContext->gpr[1]; p && (u32)p != 0xffffffff && i++ < 16; p=(u32*) *p )
		OSReport( "0x%08x:   0x%08x    0x%08x\n", p, p[0], p[1] );
	
	OSReport( "\nInstruction at 0x%x (read from SRR0) attempted to access "
		"invalid address 0x%x (read from DAR)\n", pContext->srr0, dar );
	
	OSHalt( "\n\nMemory error!\n\n" );
}

#endif

int MRTC_MemoryProtect::ProtectRange(void *_pMemory, mint _nBytes)
{
#ifndef M_STATICINIT
	g_ObjectManagerContainer.Lock();
#endif
#ifdef PLATFORM_DOLPHIN
	for (int i = 0; i < CSecure_MaxSections; ++i)
	{
		if (!m_Protect.m_Sections[i].m_bAllocated)
		{
			m_Protect.m_Sections[i].m_bAllocated = true;
			m_Protect.Create((uint8*)_pMemory, _nBytes, (CSecureMemSection)i);
			return i;
		}		
	}
#endif
	return -1;
#ifndef M_STATICINIT
	g_ObjectManagerContainer.Unlock();
#endif
}

void MRTC_MemoryProtect::Lock(int _Range)
{
#ifndef M_STATICINIT
	g_ObjectManagerContainer.Lock();
#endif
#ifdef PLATFORM_DOLPHIN
	M_ASSERT(_Range >= 0 && _Range < CSecure_MaxSections, "Error, range is not allocated");
	m_Protect.Lock((CSecureMemSection)_Range);
#endif
#ifndef M_STATICINIT
	g_ObjectManagerContainer.Unlock();
#endif
}

void MRTC_MemoryProtect::UnLock(int _Range)
{
#ifndef M_STATICINIT
	g_ObjectManagerContainer.Lock();
#endif
#ifdef PLATFORM_DOLPHIN
	M_ASSERT(_Range >= 0 && _Range < CSecure_MaxSections, "Error, range is not allocated");
	m_Protect.Unlock((CSecureMemSection)_Range);
#endif
#ifndef M_STATICINIT
	g_ObjectManagerContainer.Unlock();
#endif
}
