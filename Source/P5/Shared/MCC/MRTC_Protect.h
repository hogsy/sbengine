
/*
	Memory error tracking code for Nintendo Dolphin (GameCube).

	Author:			Steffo Sandberg
	Copyright:	Starbreeze Studios AB 2002

	History:
		020709:		Wrote this line. 17 days left to GC-Enclave-alpha!!!  =)


  Usage:


  char *pMemoryArray = new char[1024*1024];

  CSecureSection *pMemSection = g_pSecureMemGC->create( (uint8 *)pMemoryArray, 1024*1024, CSecure_Section_0 );

  //  Illegal:
  pMemoryArray[1024] = 0xff;

  //  Legal:
  uint8 *pLockedData = pMem->Lock();
  pLockedData[1024] = 0xff;
  pMem->Unlock();
  
	g_pSecureMemGC->destroy( CSecure_Section_0 );
	delete  pMemoryArray;
	
*/
#if defined PLATFORM_DOLPHIN


typedef enum _CSecureMemSection
{
	CSecure_Section_0 = 0,
		CSecure_Section_1,
		CSecure_Section_2,
		CSecure_Section_3,
		CSecure_MaxSections
} CSecureMemSection;


/*
CSecureSection.

*/
class CSecureSection
{
public:
	uint8 *m_pMemPtr;
	uint32 m_nBytes;
	int m_bActive:1;
	int m_bAllocated:1;
	
	CSecureMemSection m_section;
	
	CSecureSection() : m_bActive(false), m_bAllocated(false) {};
	~CSecureSection() {};
	
	void  SetSection( CSecureMemSection _section )  { m_section = _section; };
	void  Enable( uint8 *_pPtr, uint32 _nBytes );
	void  Disable( void );
	
	uint8 *Lock( void );
	void  Unlock( void );
	
	void  LaunchBadError( OSContext *pContext, u32 dsisr, u32 dar );
};

/*
CSecureMem.

*/
class CSecureMem
{
public:
	CSecureSection  m_Sections[ CSecure_MaxSections ];
	
	CSecureMem();
	~CSecureMem();
	
	CSecureSection  *Create( uint8 *_pPtr, uint32 _nBytes, CSecureMemSection section );
	void  Destroy( CSecureMemSection section );
	
	/*  The legitimite way to write data to a secure memorysection. */  
	uint8 *Lock( CSecureMemSection section );
	void  Unlock( CSecureMemSection section );
};

extern  CSecureMem  *g_pSecureMemGC;

#endif

class MRTC_MemoryProtect
{
public:
#if defined PLATFORM_DOLPHIN
	CSecureMem m_Protect;
#endif
	int ProtectRange(void *_pMemory, mint _nBynes);
	void Lock(int _Section);
	void UnLock(int _Section);
};
