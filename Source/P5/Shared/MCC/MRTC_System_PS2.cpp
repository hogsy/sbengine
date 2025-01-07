
#ifdef	PLATFORM_PS2

#define PS2_USE_HOST
//#define	PS2_USE_METAFILE

#define	CPU_FREQUENCY	294912000L

#include <sifdev.h>
#include "msystem_ps2.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| System stuff
|__________________________________________________________________________________________________
\*************************************************************************************************/

void MRTC_SystemInfo::PostCreate()
{
}

enum
{
	SCRATCHPAD_UPPER	= 16384
};
static int32 ScratchPad_Pos = 0;

extern "C" void FooBreak();

uint32 ScratchPad_Alloc( int32 _Bytes )
{
	int32 SPPos = ScratchPad_Pos;
	int32 Aligned = (_Bytes + 15)&~15;
	if( ( Aligned + SPPos ) > SCRATCHPAD_UPPER )
		FooBreak();

	uint32 MemPos = 0x70000000 + SPPos;
	ScratchPad_Pos	= SPPos + Aligned;

	return MemPos;
}

void ScratchPad_Free( int32 _Bytes )
{
	int32 SPPos = ScratchPad_Pos;
	int32 Aligned = (_Bytes + 15)&~15;
	if( ( SPPos - Aligned ) < 0 )
		FooBreak();

	ScratchPad_Pos	= SPPos - Aligned;
}

////////////////////////////////////////////////////////
//        % operator for int64
/*
int64 Time_GetModulusWork( int64 _Time, int64 _Size )
{
	int64 Counter = _Size;
	int bit = 0;
	
	if( _Time < Counter )
	{
		return _Time;
	}
	
	while( Counter < _Time )
	{
		Counter <<= 1;
		bit++;
	}
	
	// We always go 1 step to far.. compensate
	bit--;
	Counter	>>= 1;
	int modulus = 1 << bit;
	int64 Remainder = _Time - Counter;
	
	while( bit >= 0 && Remainder > 0 )
	{
		if( Counter <= Remainder )
		{
			modulus += 1 << bit;
			Remainder	-= Counter;
		}
		
		bit--;
		Counter	>>= 1;
	}
	
	return Remainder;
}


int64 Time_GetModulus( int64 _Time, int64 _Size )
{
	if( _Time < 0 )
	{
		return -Time_GetModulusWork( -_Time, _Size );
	}
	return Time_GetModulusWork( _Time, _Size );
}

////////////////////////////////////////////////////////
//      / operator for int64

int Time_GetModulusCountWork( int64 _Time, int64 _Size )
{
	int64 Counter = _Size;
	int bit = 0;
	
	if( _Time < Counter )
	{
		return 0;
	}
	
	while( Counter < _Time )
	{
		Counter <<= 1;
		bit++;
	}
	
	// We always go 1 step to far.. compensate
	bit--;
	Counter	>>= 1;
	int modulus = 1 << bit;
	int64 Remainder = _Time - Counter;
	
	while( bit >= 0 && Remainder > 0 )
	{
		if( Counter <= Remainder )
		{
			modulus += 1 << bit;
			Remainder	-= Counter;
		}
		
		bit--;
		Counter	>>= 1;
	}
	
	return modulus;
}

int Time_GetModulusCount( int64 _Time, int64 _Size )
{
	if( _Time < 0 )
	{
		return -Time_GetModulusCountWork( -_Time, _Size );
	}
	return Time_GetModulusCountWork( _Time, _Size );
}
*/
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

MRTC_SystemInfo::MRTC_SystemInfo()
{
	m_nCPU = 1;
	m_CPUFeatures = 0;
	m_CPUFeaturesEnabled = 0xffffffff;
	m_CPUName[0] = 0;
	m_CPUNameWithFeatures[0] = 0;

	CPU_Detect();
}

MRTC_SystemInfo::~MRTC_SystemInfo()
{
}

void MRTC_SystemInfo::CPU_Detect()
{
	CPU_CreateNames();
}

void MRTC_SystemInfo::CPU_MeasureFrequency()
{
}
/*
const uint64 MRTC_SystemInfo::GetCPUFrequencyu()
{
	return CPU_FREQUENCY;
}

const fp4 MRTC_SystemInfo::GetCPUFrequencyfp()
{
	return CPU_FREQUENCY;
}

const fp4 MRTC_SystemInfo::GetCPUFrequencyRecp()
{
	return (1.0f/CPU_FREQUENCY);
}
*/


void MRTC_SystemInfo::CPU_CreateNames()
{
	CFStr CPUName("MIPS R5900 for PS2");
	CFStr CPUNameFeatures = CPUName;

	CPUNameFeatures += CFStrF(", %.0f Mhz", CPU_FREQUENCY / 1000000.0);

	strcpy(m_CPUName, CPUName.Str());
	strcpy(m_CPUNameWithFeatures, CPUNameFeatures.Str());
}

static int64 Clock = 0;
int64 MRTC_SystemInfo::CPU_GetClock()
{
	return Clock;
}

int64 LastTick = 0;
void MRTC_SystemInfo::CPU_AdvanceClock( fp4 _DeltaTime)
{
//	_DeltaTime	= Min( _DeltaTime, ( 1.0f / 20.0f ) );	// Clamp with 20 fps as floor
//	Clock	+= (CPU_FREQUENCY * _DeltaTime );

	int64 Tick = CPU_GetCycles();
	uint32 Delta = Tick - LastTick;
	if( Delta > ( CPU_FREQUENCY / 30 ) )
		Delta	= (CPU_FREQUENCY / 30);
	LastTick = Tick;

	Clock	+= Delta;
}

static int64 value = 0;
static uint32 last = 0;
int64 MRTC_SystemInfo::CPU_GetCycles()
{
	uint32 v;

	asm ("	mfc0 v,$9 ");

	value += ((uint64) ( v - last ) );
	last = v;

	return value;
}
/*
fp8 MRTC_SystemInfo::CPU_GetTime()
{
#ifdef CONSTANT_FRAME_RATE
	extern fp8 g_SimulatedTime;
	return g_SimulatedTime;
#else
	const uint64 clock = CPU_GetClock();
	const uint32 freq = CPU_FREQUENCY;
	const uint32 val = clock / freq;	// Only fraction left
	const uint32 remainder = clock - ( val * freq );

	fp4 time = fp4(val) + ( fp4(remainder) / fp4(freq) );

	return fp8( time );
#endif
}
*/
void* MRTC_SystemInfo::OS_Alloc(uint32 _Size, bool _bCommit)
{
	void *pMem = OS_HeapAlloc( _Size );
	if( pMem == NULL )
	{
		scePrintf( "MRTC_SystemInfo::OS_Alloc, failed to allocated %d bytes (%d Free, %d Largest)\n", _Size, OS_PhysicalMemoryFree(), OS_PhysicalMemoryLargestFree() );
		while( 1 ) {}
	}
	return pMem;
}

bool MRTC_SystemInfo::OS_Commit(void *_pMem, uint32 _Size, bool _bCommited)
{
	return false;
}

uint32 MRTC_SystemInfo::OS_CommitGranularity()
{
	return false;
}

void MRTC_SystemInfo::OS_Free(void* _pMem)
{
	OS_HeapFree( _pMem );
}

void* MRTC_SystemInfo::OS_GetProcessID()
{
	return 0;
}

void* MRTC_SystemInfo::OS_GetThreadID()
{
	return (void*)::GetThreadId();
}

void MRTC_SystemInfo::OS_Sleep(int _Milliseconds)
{
	int64 until;

	until = CPU_GetCycles() + _Milliseconds * MRTC_SystemInfo::MRTC_GetSystemInfo().GetCPUFrequencyu() / 1000;

	while( until > CPU_GetCycles() )
	{
		RotateThreadReadyQueue(1);
	}
}

// -------------------------------------------------------------------

class MRTC_PS2ThreadInfo : public CObj
{
	int m_Thread;
	uint8 *m_StackPtr;
	int32 m_Priority;

public:

	MRTC_PS2ThreadInfo( uint32(*_pfnEntryPoint)(void*), int _StackSize, int32 _Priority )
	{

		m_StackPtr = DNew(uint8 ) uint8 [_StackSize + 20];
		if (!m_StackPtr) MemError("MRTC_PS2ThreadInfo::MRTC_PS2ThreadInfo");

		m_Priority = _Priority;

		struct ThreadParam threadParam;

		threadParam.attr = 0;
		threadParam.currentPriority = 0;
		threadParam.entry = (void (*)(void *))_pfnEntryPoint;
		threadParam.gpReg = &_gp;
		threadParam.initPriority = 3 + _Priority;
		threadParam.option = 0;
		threadParam.stack = (void*)(( (uint32)m_StackPtr + 16 ) & ~0xf);
		threadParam.stackSize = _StackSize;
		threadParam.status = 0;
		threadParam.waitId = 0;
		threadParam.waitType = 0;
		threadParam.wakeupCount = 0;

		m_Thread = ::CreateThread(&threadParam);
		if (m_Thread < 0) Error("MRTC_PS2ThreadInfo::MRTC_PS2ThreadInfo", "Failed to create new thread.");
	}

	~MRTC_PS2ThreadInfo()
	{
		::DeleteThread( m_Thread );
		delete [] m_StackPtr;
	}

	void StartThread() const
	{
		::StartThread( m_Thread, NULL );
	}

	int GetStatus() const
	{
		struct ThreadParam threadParam;

		::ReferThreadStatus( m_Thread, &threadParam );

		return (threadParam.status == THS_WAIT )?MRTC_THREAD_STILLACTIVE:0;
	}
};

void* MRTC_SystemInfo::OS_ThreadCreate(uint32(M_STDCALL*_pfnEntryPoint)(void*), int _StackSize, void* _pContext, int _ThreadPriority)
{
	int32 Prio;

	if (_ThreadPriority < MRTC_THREAD_PRIO_LOW)
		Prio = 180;
	else if (_ThreadPriority < MRTC_THREAD_PRIO_BELOWNORMAL)
		Prio = 150;
	else if (_ThreadPriority < MRTC_THREAD_PRIO_NORMAL)
		Prio = 120;
	else if (_ThreadPriority < MRTC_THREAD_PRIO_ABOVENORMAL)
		Prio = 90;
	else if (_ThreadPriority < MRTC_THREAD_PRIO_HIGHEST)
		Prio = 60;
	else if (_ThreadPriority < MRTC_THREAD_PRIO_TIMECRITICAL)
		Prio = 30;
	else
		Prio = 0;

	MRTC_PS2ThreadInfo *pThread = MNew3(MRTC_PS2ThreadInfo, _pfnEntryPoint, _StackSize, Prio );

	pThread->StartThread();

	return (void*)pThread;
}

int MRTC_SystemInfo::OS_ThreadDestroy(void* _hThread)
{
	MRTC_PS2ThreadInfo *pThread = (MRTC_PS2ThreadInfo*)_hThread;
	while(1)
	{
		MRTC_GOM()->m_pGlobalLock->Lock();
		if (!OS_ThreadIsRunning(_hThread))
		{
//			int ExitCode = OS_ThreadGetExitCode(_hThread);
			int ExitCode = 0;
			delete [] pThread;
			MRTC_GOM()->m_pGlobalLock->Unlock();
			return ExitCode;
		}
		MRTC_GOM()->m_pGlobalLock->Unlock();
		OS_Sleep(1);
	}
}

void MRTC_SystemInfo::OS_ThreadTerminate(void* _hThread, int _ExitCode)
{
	//JK-TODO: No TerminateThread on Xbox
	M_ASSERT(0, "No TerminateThread on PS2");
}

bool MRTC_SystemInfo::OS_ThreadIsRunning(void* _hThread)
{
	MRTC_PS2ThreadInfo *pThread = (MRTC_PS2ThreadInfo*)_hThread;

	return (MRTC_THREAD_STILLACTIVE == pThread->GetStatus());
}

void MRTC_SystemInfo::OS_ThreadExit(int _ExitCode)
{
	// No such thing as a thread exit code on PS2
	return;
}

int MRTC_SystemInfo::OS_ThreadGetExitCode(void* _hThread)
{
	return 0;
}

// -------------------------------------------------------------------

void* MRTC_SystemInfo::OS_MutexOpen(const char* _pName)
{
	static uint8 lCSMem[sizeof(MRTC_CriticalSection)];
	static MRTC_CriticalSection* pCSMem = NULL;

	if (!pCSMem)
	{
	
		pCSMem = new((void*)lCSMem)MRTC_CriticalSection;
	
	}

	return pCSMem;
}

void MRTC_SystemInfo::OS_MutexClose(void* _hLock)
{
}

void MRTC_SystemInfo::OS_MutexLock(void* _hLock)
{
	MRTC_CriticalSection* pCS = (MRTC_CriticalSection*) _hLock;
	pCS->Lock();
}

void MRTC_SystemInfo::OS_MutexUnlock(void* _hLock)
{
	MRTC_CriticalSection* pCS = (MRTC_CriticalSection*) _hLock;
	pCS->Unlock();
}

void MRTC_SystemInfo::OS_Assert(const char* _pMsg)
{
	OS_Trace("MRTC Assert: %s\n", _pMsg);

#if defined( M_RTM )
	char szTmp[ 512 ];
	sprintf( szTmp, "OS_Assert: %s\n", _pMsg );
	CInfoScreenPS2::Get().OutputText( szTmp );
#endif
	*((int *)(NULL)) = 0;
}

void M_ARGLISTCALL MRTC_SystemInfo::OS_Trace(const char * _pStr, ...)
{
	char lBuffer[4096];

	if (_pStr)
	{
		va_list arg;
		va_start(arg, _pStr);
		CStrBase::vsnprintf((char*) &lBuffer[0], 4095, _pStr, arg); 
		lBuffer[4095] = 0;
	}
	else
		lBuffer[0] = 0;

	printf( "%s", lBuffer );
/*
	int nPos = 0;
	int nLen = strlen( lBuffer );
	while( nPos < nLen )
	{
		char aBuf[32];
		int nCount = Min( 31, ( nLen - nPos ) );
		strncpy( aBuf, lBuffer + nPos, nCount );
		aBuf[nCount] = 0;
		scePrintf( "%s", aBuf );
		
		nPos	+= nCount;
	}
*/
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| File
|__________________________________________________________________________________________________
\*************************************************************************************************/

#define PS2_CACHE_SIZE	( 8 * 2048 )

#include <libcdvd.h>

class MRTC_PS2AsyncInstance;

#ifdef	PS2_USE_METAFILE
class MRTC_PS2Metafile
{
	struct sFileEntry
	{
		char *			m_Filename;
		uint32			m_FilePosition;
		uint32			m_Filesize;
	};

	struct sHashEntry
	{
		uint32			m_Hash;
		uint32			m_Index;
	};

protected:
	char aDirMask[1024];	// JK-NOTE: This will steal 1k static data... lower this?

	char *			m_Cache;
	sceCdRMode		m_ReadMode;
	unsigned int	m_LSN[8];

	uint32			m_CurrentFile;			// Holds index of the file which is currently read/cached
	uint32			m_CurrentFileOffset;	// Where in the file are we currently reading/caching

	uint32			m_CurrentCacheOffset;
	uint32			m_CurrentCachePos;

	sFileEntry *	m_FileTable;
	sHashEntry *	m_HashTable;
	uint32 *		m_HashTableSize;
	uint32			m_FileEntries;
	char *			m_MemoryChunk;

	int32 ReadBytes( char *_Address, uint32 _Size )
	{
		M_ASSERT( m_Cache != NULL, "No metafiles located!" );
		uint32 done = 0;

		while( done < _Size )
		{
			uint32 size = ( PS2_CACHE_SIZE - m_CurrentCacheOffset )<(_Size-done)?( PS2_CACHE_SIZE - m_CurrentCacheOffset ):(_Size-done);

			memcpy( _Address + done, m_Cache + m_CurrentCacheOffset, size );

			done					+= size;
			m_CurrentCacheOffset	+= size;

			if( m_CurrentCacheOffset == PS2_CACHE_SIZE )
			{
				Cache( m_CurrentCachePos + 8, m_Cache );
				m_CurrentCachePos	+= 8;
				CacheSync();
			}
		}

		return _Size;
	}

public:
	MRTC_PS2Metafile() : m_Cache( NULL ) {}

	void Cache( int _LSN, char *_Cache )
	{
//		while( sceCdSync( 0 ) == 1 );
		sceCdRead( _LSN, 8, _Cache, &m_ReadMode );
		CacheSync();
		m_CurrentCacheOffset	= 0;
	}

	void Cache( int _LSN, char *_Cache, uint32 _Size )
	{
//		while( sceCdSync( 0 ) == 1 );
		sceCdRead( _LSN, _Size, _Cache, &m_ReadMode );
//		CacheSync();
		m_CurrentCacheOffset	= 0;
	}

	void CacheSync()
	{
		sceCdSync( 0 );
	}

	void CacheFlush()
	{
		m_CurrentCachePos		= -1;
		m_CurrentCacheOffset	= PS2_CACHE_SIZE;
	}

	int32 ReadBytes( uint32 _FileIndex, char *_Address, uint32 _Size )
	{
		if( m_CurrentFile != _FileIndex )
		{
			return -1;
		}

		return ReadBytes( _Address, _Size );
	}

	static char *str_lwr( char *_String )
	{
		char *ret = _String;
		while( *_String )
		{
			if( ( *_String >= 'A' ) && ( *_String <= 'Z' ) )
			{
				*_String	= *_String - 'A' + 'a';
			}
			else if( *_String == '\\' )
			{
				*_String	= '/';
			}
			_String++;
		}
		
		return ret;
	}

	void Init()
	{
		MSCOPESHORT(MRTC_PS2Metafile::Init);
		char aCache[PS2_CACHE_SIZE] __attribute__((aligned(64)));
		sceCdlFILE file;
		int i = 0;

		m_Cache	= (char*)aCache;
		
		while( 1 )
		{
			CFStr TmpStr;
			
			TmpStr = CFStrF( "\\DATA%.3d.SBZ;1", i );
			strcpy( file.name, TmpStr.GetStr() );
			
			if( !sceCdSearchFile( &file, aBuf ) )
			{
#ifndef	M_RTM
				int err;
				switch( ( err = sceCdGetError( ) ) )
				{
				default: MRTC_SystemInfo::OS_Trace( CStrF("Errorcode '%d'\n", err ).GetStr() ); break;
				case SCECdErNO: MRTC_SystemInfo::OS_Trace( "No error\n" ); break;
				case SCECdErABRT: MRTC_SystemInfo::OS_Trace( "sceCdSearchFile:: SCECdErABRT\n" ); break;
				case SCECdErNORDY: MRTC_SystemInfo::OS_Trace( "sceCdSearchFile:: SCECdErNORDY\n" ); break;
				case SCECdErCUD: MRTC_SystemInfo::OS_Trace( "sceCdSearchFile:: SCECdErCUD\n" ); break;
				case SCECdErREAD: MRTC_SystemInfo::OS_Trace( "sceCdSearchFile:: SCECdErREAD\n" ); break;
				case SCECdErTRMOPN: MRTC_SystemInfo::OS_Trace( "sceCdSearchFile:: SCECdErTRMOPN\n" ); break;
				case SCECdErREADCFR: MRTC_SystemInfo::OS_Trace( "sceCdSearchFile:: SCECdErREADCFR\n" ); break;
				}
#endif
				break;
			}

			extern void PS2_InitPAKFile( uint32 _LSN, uint32 _Size );
			
			PS2_InitPAKFile( file.lsn, file.size );

			m_LSN[i++]	= file.lsn;
		}

		M_ASSERT( i, "Unable to find any metafile on disc!" );

		m_ReadMode.trycount		= 32;
		m_ReadMode.datapattern	= SCECdSecS2048;
		m_ReadMode.spindlctrl	= SCECdSpinNom;

		MRTC_SystemInfo::OS_Trace( "Found a metafile split into %d chunks.\n", i );

		CacheSync();
		m_CurrentCachePos	= m_LSN[0];
		Cache( m_LSN[0], m_Cache );
		CacheSync();

		uint32 headerstart, headersizebytes, headersizesectors;
		uint32 remapstart, remapsizebytes, remapsizesectors;
		uint32 entries;

		memcpy( &headerstart, m_Cache + 32, 4 );
		memcpy( &headersizebytes, m_Cache + 36, 4 );
		memcpy( &headersizesectors, m_Cache + 40, 4 );
		memcpy( &remapstart, m_Cache + 44, 4 );
		memcpy( &remapsizebytes, m_Cache + 48, 4 );
		memcpy( &remapsizesectors, m_Cache + 52, 4 );
		memcpy( &entries, m_Cache + 56, 4 );

		m_MemoryChunk	= DNew(char ) char [headersizebytes + entries * sizeof( sHashEntry )];

		m_FileTable		= (sFileEntry*)m_MemoryChunk;
		m_HashTable		= (sHashEntry*)( m_MemoryChunk + headersizebytes );
		m_FileEntries	= entries;

		CacheSync();
		m_CurrentCachePos	= m_LSN[0] + headerstart;
		Cache( m_LSN[0] + headerstart, m_Cache );
		CacheSync();

		ReadBytes( m_MemoryChunk, headersizebytes );

		CacheSync();
		m_CurrentCachePos	= m_LSN[0] + remapstart;
		Cache( m_LSN[0] + remapstart, m_Cache );
		CacheSync();

		char *pNames = m_MemoryChunk + sizeof( sFileEntry ) * entries;

		for( i = 0; i < entries; i++ )
		{
			int pakfile;
			ReadBytes( (char*)&pakfile, 4 );
			ReadBytes( (char*)&m_HashTable[i], 8 );
			m_FileTable[i].m_FilePosition	+= m_LSN[pakfile];
			m_FileTable[i].m_Filename		= pNames;
			pNames							+= strlen( pNames ) + 1;
			int pad = (int)pNames;
			pad		= ( 4 - ( pad & 3 ) ) & 3;
			pNames	+= pad;
		}
		CacheFlush();
	}

	uint32 HashName( const char *_File ) const
	{
		M_ASSERT( m_Cache != NULL, "No metafiles located!" );
		unsigned int hash = 5381;

		while( *_File )
		{
			int c = *_File;
			hash	= ( ( hash << 5 ) + hash ) + c;
			_File++;
		}

		return hash;
	}

	uint32 GetFileIndex( const char *_File ) const
	{
		M_ASSERT( m_Cache != NULL, "No metafiles located!" );
		char aName[1024];
		strcpy( aName, _File );
		str_lwr( aName );

		const uint32 hash = HashName( aName );
		uint32 bottom = 0, top = m_FileEntries - 1;
		uint32 idx = ( top + bottom ) >> 1;

		while( hash != m_HashTable[idx].m_Hash )
		{
			const uint32 idxhash = m_HashTable[idx].m_Hash;
			if( hash < idxhash ) {
				top	= idx - 1;
			} else if( hash > idxhash ) {
				bottom	= idx + 1;
			} else {
				break;
			}
			if( top < bottom ) {
				return -1;
			}
			idx = ( top + bottom ) >> 1;
		}

		uint32 cnt = 1;
		uint32 i = idx + 1;

		while( m_HashTable[i].m_Hash == hash )
		{
			cnt++;
			i++;
		}

		i	= idx - 1;
		while( m_HashTable[i].m_Hash == hash )
		{
			cnt++;
			i--;
		}
		i++;

		int index = m_HashTable[i].m_Index;
		if( cnt > 1 )
		{
			// Multiple entries of same hashvalue
			do
			{
				if( !strcmp( m_FileTable[index].m_Filename, aName ) )
				{
					break;
				}
				i++;
				index = m_HashTable[i].m_Index;
			} while( --cnt );

			if( cnt == 0 )
			{
				// Could not find file
				return -1;
			}
		}

		return index;
	}

	uint32 GetFileLSN( const char *_File ) const
	{
		uint32 idx = GetFileIndex( _File );
		if( idx != -1 )
		{
			return m_FileTable[idx].m_FilePosition;
		}
		return -1;
	}

	void CacheFile( const char *_File, char *_Cache )
	{
		M_ASSERT( m_Cache != NULL, "No metafiles located!" );
		uint32 idx = GetFileIndex( _File );
		if( idx != -1 )
		{
			m_CurrentFile		= idx;
			Cache( m_FileTable[idx].m_FilePosition, _Cache );
//			CacheSync();
		}
	}

	uint32 GetFileSize( const char *_File ) const
	{
		M_ASSERT( m_Cache != NULL, "No metafiles located!" );
		uint32 idx = GetFileIndex( _File );
		if( idx != -1 )
		{
			return m_FileTable[idx].m_Filesize;
		}
		return 0;
	}

	bool FileExists( const char *_File ) const
	{
		M_ASSERT( m_Cache != NULL, "No metafiles located!" );
		uint32 idx = GetFileIndex( _File );

		return (idx != -1);
	}

	int DirBegin( const char *_Path, char *_aDir, int& _Size, bool& _Directory )
	{
		strcpy( aDirMask, _Path );
		str_lwr( aDirMask );

		int len = strlen( aDirMask );

		return DirNext( -1, _aDir, _Size, _Directory );
	}

	int DirNext( int handle, char *_aDir, int& _Size, bool& _Directory )
	{
		int len = strlen( aDirMask );
		int dirlen = 0;
		if( handle >= 0 )
		{
			dirlen = ( strstr( m_FileTable[handle].m_Filename + len + 1, "/" ) - m_FileTable[handle].m_Filename );
		}

		for( int i = handle + 1; i < m_FileEntries; i++ )
		{
			if( !strncmp( aDirMask, m_FileTable[i].m_Filename, len ) )
			{
				// Check if there are more subdirs under this one.
				char *c = m_FileTable[i].m_Filename + len + 1;
				while( *c )
				{
					if( *c == '/' )
						break;
					c++;
				}

				if( !*c )
				{
					char *c = m_FileTable[i].m_Filename + len + 1;
					if( *c )
					{
						_Size	= m_FileTable[i].m_Filesize;
						_Directory = false;
						strcpy( _aDir, c );
						return i;
					}
				}
				else
				{
					if( ( handle == -1 ) || strncmp( m_FileTable[handle].m_Filename + len + 1, m_FileTable[i].m_Filename + len + 1, dirlen - len - 1 ) )
					{
						// New directory
						_Directory	= true;
						_Size		= 0;
						strncpy( _aDir, m_FileTable[i].m_Filename + len + 1, c - ( m_FileTable[i].m_Filename + len + 1 ) );
						_aDir[ (int)( c - ( m_FileTable[i].m_Filename + len + 1 ) ) ] = 0;
						return i;
					}
				}
			}
		}
		return -1;
	}
};

MRTC_PS2Metafile *PS2_Metafile = NULL;

class MRTC_PS2FileInfo
{
public:
	uint32	m_Filesize;
	uint32	m_Fileposition;
	uint32	m_FileLSN;
	uint32	m_DataOffset;

	MRTC_PS2FileInfo( const char *_File )
	{
		m_Filesize		= PS2_Metafile->GetFileSize( _File );
		m_FileLSN		= PS2_Metafile->GetFileLSN( _File );
		m_Fileposition	= 0;
		m_DataOffset	= 0;
	}

	void Destroy()
	{
	}

	int64 GetSize() const
	{
		return m_Filesize;
	}

	static bool FileExists( const char *_File )
	{
		return PS2_Metafile->FileExists( _File );
	}
};

void *MRTC_SystemInfo::OS_FileOpen(const char *_pFilenName, bool _bRead, bool _bWrite, bool _bCreate, bool _bTruncate, bool _bInMemory)
{
	MRTC_PS2FileInfo *pFile;

	pFile = DNew(MRTC_PS2FileInfo) MRTC_PS2FileInfo( _pFilenName );

	if( !pFile ) MRTC_SystemInfo::OS_Trace( "Failed to open file!!! (%s)\n", _pFilenName );

#ifndef	M_RTM
	if( !pFile ) Error_static( "MRTC_SystemInfo::OS_FileOpen", "Out of memory" );
#endif

	return pFile;
}

class MRTC_PS2AsyncReaderMetafile
{
private:
	MRTC_PS2AsyncInstance *m_pInstance;
	MRTC_PS2AsyncInstance *m_pInstanceList;
	MRTC_CriticalSection m_CriticalSection;

	MRTC_PS2AsyncInstance* GetNextUnfinishedInstance();
public:
	MRTC_PS2AsyncReaderMetafile() : m_pInstance( NULL ) {}

	void AddInstance( MRTC_PS2AsyncInstance *_pInstance );
	void RemoveInstance( MRTC_PS2AsyncInstance *_pInstance );
	bool IsCompleted( MRTC_PS2AsyncInstance *_pInstance );
};

typedef MRTC_PS2AsyncReaderMetafile	MRTC_PS2AsyncReader;

#elif	defined( PS2_USE_HOST )

class MRTC_PS2FileInfo
{
public:
	int m_File;
	MRTC_PS2FileInfo( int _Handle ) : m_File( _Handle ) {}

	void Destroy()
	{
		sceClose( m_File );
	}

	int64 GetSize() const
	{
		int nPos, nSize;
		nPos = sceLseek( m_File, 0, SEEK_CUR );
		nSize = sceLseek( m_File, 0, SEEK_END );
		sceLseek( m_File, nPos, SEEK_SET );

		return nSize;
	}

	static bool FileExists( const char *_pPath )
	{
		char aFilename[1024];
		sprintf( aFilename, "host0:%s", _pPath );
		int handle = sceOpen( aFilename, SCE_RDONLY );
		if( handle < 0 )
			return false;
		sceClose( handle );
		return true;
	}
};

void *MRTC_SystemInfo::OS_FileOpen(const char *_pFilenName, bool _bRead, bool _bWrite, bool _bCreate, bool _bTruncate, bool _bInMemory)
{
	MRTC_PS2FileInfo *pFile;
	char aFilename[1024];
	sprintf( aFilename, "host0:%s", _pFilenName );

	int mode = 0;

	if( _bRead && _bWrite )
		mode = SCE_RDWR;
	else if( _bRead )
		mode = SCE_RDONLY;
	else if( _bWrite )
		mode = SCE_WRONLY;
	else
		return NULL;

	if( _bCreate )
		mode |= SCE_CREAT;

	if( _bTruncate )
		mode |= SCE_TRUNC;
	else
		mode |= SCE_APPEND;

	int handle = sceOpen( aFilename, mode );

	if( handle < 0 )
		return NULL;

	pFile = DNew(MRTC_PS2FileInfo) MRTC_PS2FileInfo( handle );

	if( !pFile ) Error_static( "MRTC_SystemInfo::OS_FileOpen", "Out of memory" );

	return pFile;

}

//JK-NOTE: The PS2 can only have 1 async reading in the queue so we need a class that manages reading
//JK-NOTE: try to implement this with callbacks and chains might improve performance
class MRTC_PS2AsyncReaderHost
{
private:
	MRTC_PS2AsyncInstance *m_pInstance;
	MRTC_CriticalSection m_CriticalSection;

public:
	MRTC_PS2AsyncReaderHost() : m_pInstance( NULL ) {}

	bool IsCompleted( MRTC_PS2AsyncInstance *_pInstance );
	void AddInstance( MRTC_PS2AsyncInstance *_pInstance ){}	// Host does not require instancehandling since it can't be async
	void RemoveInstance( MRTC_PS2AsyncInstance *_pInstance ){}	// Host does not require instancehandling since it can't be async
};

typedef MRTC_PS2AsyncReaderHost	MRTC_PS2AsyncReader;

#endif	// PS2_FILE______

void PS2Metafile_Allocate();
void PS2Metafile_Allocate()
{
	MSCOPESHORT(PS2Metafile_Allocate);
#if	defined(PS2_USE_METAFILE)
	if( !PS2_Metafile )
	{
		PS2_Metafile	= DNew(MRTC_PS2Metafile) MRTC_PS2Metafile;
		PS2_Metafile->Init();
	}
#endif
}

int PS2File_FindNext( int handle, char *_aDir, int& _Size, bool& _Directory );
int PS2File_FindNext( int handle, char *_aDir, int& _Size, bool& _Directory )
{
#if	defined(PS2_USE_METAFILE)
	return PS2_Metafile->DirNext( handle, _aDir, _Size, _Directory );
#elif	defined(PS2_USE_HOST)

	struct sce_dirent dirent;
	int NameLen = sceDread( handle, &dirent );
	if( NameLen > 0 )
	{
		strncpy( _aDir, dirent.d_name, NameLen );
		_aDir[NameLen]	= 0;
		if( ( dirent.d_stat.st_mode & 0xf000 ) & 0x1000 )
			_Directory	= true;
		else
		{
			_Directory	= false;
			_Size	= dirent.d_stat.st_size;
		}
		
		return handle;
	}
	else
		return -1;
	
	

/*
	int nPos = 0;
	for( ;; )
	{
		char aName[256];
		char c;
		if( ps2file_findcache == ps2file_findcachepos )
		{
			int nRead = sceRead( ps2file_findhandle, ps2file_findcachedata, 1024 );
			if( nRead > 0 )
			{
				ps2file_findcache	= nRead;
				ps2file_findcachepos	= 0;
			}
			else
			{
				return -1;
			}
		}

		c	= ps2file_findcachedata[ps2file_findcachepos++];
		if( c != 10 )
		{
			aName[nPos++]	= c;
		}
		else
		{
			aName[nPos]	= 0;
			char *pSize = aName + 2;
			char *pName = pSize;

			while( *pName != ' ' )
				pName++;
			
			*pName	= 0;
			pName++;

			strcpy( _aDir, pName );
			if( aName[0] == 'D' )
			{
				_Directory	= true;
				_Size		= 0;
			}
			else
			{
				_Directory = false;
				_Size	= atoi( pSize );
			}

			return handle;
		}
	}
*/
#else
	#error "implement this"
#endif
}

int PS2File_FindFirst( const char *_Path, char *_aDir, int& _Size, bool& _Directory );
int PS2File_FindFirst( const char *_Path, char *_aDir, int& _Size, bool& _Directory )
{
#if	defined(PS2_USE_METAFILE)
	return PS2_Metafile->DirBegin( _Path, _aDir, _Size, _Directory );
#elif	defined(PS2_USE_HOST)
	int handle;
	char aBuf[1024];
	sprintf( aBuf, "host0:%s", _Path );
//	sprintf( aBuf, "host0:", _Path );

	handle	= sceDopen( aBuf );
	if( handle >= 0 )
		return PS2File_FindNext( handle, _aDir, _Size, _Directory );

	return -1;

#else
	#error "implement this"
#endif
}

void PS2File_FindClose( int _handle );
void PS2File_FindClose( int _handle )
{
#if		defined(PS2_USE_METAFILE)
#elif	defined(PS2_USE_HOST)
	sceDclose( _handle );
#else
#error	"implement this"
#endif
}

void PS2File_FixPath( char *_String );
void PS2File_FixPath( char *_String )
{
	while( *_String )
	{
		if( ( *_String >= 'A' ) && ( *_String <= 'Z' ) )
		{
			*_String	= *_String - 'A' + 'a';
		}
		else if( *_String == '\\' )
		{
			*_String	= '/';
		}
		_String++;
	}
}

void MRTC_SystemInfo::OS_FileClose( void *_pFile )
{
	MRTC_PS2FileInfo *pFile = (MRTC_PS2FileInfo*)_pFile;
	pFile->Destroy();
	delete pFile;
}

fint MRTC_SystemInfo::OS_FileSize( void *_pFile )
{
	MRTC_PS2FileInfo *pFile = (MRTC_PS2FileInfo*)_pFile;
	return pFile->GetSize();
}

bool MRTC_SystemInfo::OS_FileExists( const char *_pPath )
{
	return MRTC_PS2FileInfo::FileExists( _pPath );
}

fint MRTC_SystemInfo::OS_FilePosition( const char *_pFileName )
{
#ifdef	PS2_USE_METAFILE
	return PS2_Metafile->GetFileLSN( _pFileName );
#else
	return 0;
#endif
}

// There can only be one instance of this class, it is used for async reading
static MRTC_PS2AsyncReader g_AsyncReader;

class MRTC_PS2AsyncInstance// : public CDA_Poolable
{
public:
	MRTC_PS2AsyncInstance *m_Prev;	// Pointer to previous instance
	MRTC_PS2AsyncInstance *m_Next;	// Pointer to next instance
	int32 m_Position, m_DataSize;	// These both need to be sector aligned for now
	int32 m_ReadBytes;
	void *m_Data;
	bool m_bCompleted;
	MRTC_PS2FileInfo *m_pFile;

	MRTC_PS2AsyncInstance()
	{
		m_Prev	= NULL;
		m_Next	= NULL;
	}

	bool IsCompleted()
	{
		return g_AsyncReader.IsCompleted( this );
	}

	fint BytesProcessed()
	{
		if (!m_bCompleted)
		{
			FileError_static("BytesProcessed", "Cannot be called before the file is done", 0);
			return -1;
		}


#ifdef errorchance
		if ((MRTC_RAND() % errorchance) == 0)
			FileError_static("BytesProcessed", "File Processing Failed", 0);
#endif

		return m_DataSize;
	}
};

#if defined( PS2_USE_METAFILE )

bool MRTC_PS2AsyncReaderMetafile::IsCompleted( MRTC_PS2AsyncInstance *_pInstance )
{
	m_CriticalSection.Lock();

	extern bool PS2_FileIsCompleted( uint32 _LSN, uint32 _Size );
	extern void PS2_FileRead( uint32 _LSN, uint32 _Size, void* _pDest );
	extern void PS2_FileGetData( void* _Dest, uint32 _Size );

	if( ( m_pInstance == NULL ) || PS2_FileIsCompleted( m_pInstance->m_pFile->m_FileLSN, m_pInstance->m_DataSize ) )
	{
		if( m_pInstance )
		{
/*
			if( ( m_pInstance->m_ReadBytes + 32768 ) < m_pInstance->m_DataSize )
			{
				PS2_FileGetData( (uint8*)m_pInstance->m_Data + m_pInstance->m_ReadBytes, 32768 );
				m_pInstance->m_ReadBytes	+= 32768;
				int data = Min( 32768, ( m_pInstance->m_DataSize - m_pInstance->m_ReadBytes ) );
				PS2_FileRead( m_pInstance->m_pFile->m_FileLSN + ( ( m_pInstance->m_Position + m_pInstance->m_ReadBytes ) >> 11 ), data >> 11 );
			}
			else
*/
			{
//				PS2_FileGetData( (uint8*)m_pInstance->m_Data + m_pInstance->m_ReadBytes, m_pInstance->m_DataSize - m_pInstance->m_ReadBytes );
				m_pInstance->m_ReadBytes = m_pInstance->m_DataSize;
				m_pInstance->m_Position	+= m_pInstance->m_DataSize;
				m_pInstance->m_bCompleted = true;
				m_pInstance	= NULL;				
			}
		}

		if( !m_pInstance )
		{
			m_pInstance	= GetNextUnfinishedInstance();

			if( m_pInstance )
			{
//				int data = Min( 32768, m_pInstance->m_DataSize );
				PS2_FileRead( m_pInstance->m_pFile->m_FileLSN + ( m_pInstance->m_Position >> 11 ), m_pInstance->m_DataSize, m_pInstance->m_Data );
			}
		}
	}

	m_CriticalSection.Unlock();

	return _pInstance->m_bCompleted;
}

MRTC_PS2AsyncInstance* MRTC_PS2AsyncReaderMetafile::GetNextUnfinishedInstance()
{
	MRTC_PS2AsyncInstance *Instance;

	// Only called from 1 place, shouldn't require a criticalsection since the callee is already owning it
//	m_CriticalSection.Lock();

	Instance	= m_pInstanceList;
	while( Instance )
	{
		if( !Instance->m_bCompleted )
			break;

		Instance	= Instance->m_Next;
	}

//	m_CriticalSection.Unlock();

	return Instance;
}

void MRTC_PS2AsyncReaderMetafile::AddInstance( MRTC_PS2AsyncInstance *_pInstance )
{
	m_CriticalSection.Lock();

	//JK-TODO: Should sort by priority when inserting so searching will be faster

	// Always add to tail of chain (at the end of the correct priority)

	register MRTC_PS2AsyncInstance *Prev, *Instance;

	Prev	= NULL;
	for( Instance = m_pInstanceList; Instance; Instance = Instance->m_Next )
	{
		Prev	= Instance;
	}

	if( Prev )
	{
		Prev->m_Next			= _pInstance;
		_pInstance->m_Prev		= Prev;
	}
	else
	{
		m_pInstanceList			= _pInstance;
		_pInstance->m_Prev		= NULL;
	}
	_pInstance->m_Next		= NULL;

	m_CriticalSection.Unlock();
}

void MRTC_PS2AsyncReaderMetafile::RemoveInstance( MRTC_PS2AsyncInstance *_pInstance )
{
	m_CriticalSection.Lock();

	if( !m_pInstanceList )
	{
		//JK-NOTE: This should never fucking occur!
		M_TRACEALWAYS( "MRTC_PS2AsyncReaderMetafile::RemoveInstance called with an empty instancelist");
	m_CriticalSection.Unlock();
		return;
	}

	if( _pInstance == m_pInstanceList )
	{
		m_pInstanceList	= _pInstance->m_Next;
		if( m_pInstanceList )
		{
			m_pInstanceList->m_Prev	= NULL;
		}
	}
	else
	{
#ifndef M_RTM
		//JK-DEBUG: Make sure the thing real resides in the list, make this debug only?
		register MRTC_PS2AsyncInstance *Instance;

		for( Instance = m_pInstanceList; Instance; Instance = Instance->m_Next )
			if( Instance == _pInstance )
				break;



		if( !Instance )
		{
			M_TRACEALWAYS( "Tried to remove readinstance which didn't reside in the readlist." );
		}
		else
#endif	// M_RTM
		{
			if( _pInstance->m_Prev )	// This if isn't required since the first if "guarantees" that this is true
				_pInstance->m_Prev->m_Next	= _pInstance->m_Next;
			if( _pInstance->m_Next )
				_pInstance->m_Next->m_Prev	= _pInstance->m_Prev;
		}
	}
	_pInstance->m_Next	= NULL;
	_pInstance->m_Prev	= NULL;

	if( !_pInstance->m_bCompleted )
		M_TRACEALWAYS( "Remove instance from readlist without having been completed." );

	m_CriticalSection.Unlock();
}

void *MRTC_SystemInfo::OS_FileAsyncRead( void *_pFile, void *_pData, fint _DataSize, fint _FileOffset )
{
	M_ASSERT(_DataSize <= 0xffffffff, "Cannot read more that 4 gigs, but we don't have that much memory space anyways");

	MRTC_PS2AsyncInstance *pNewInstance = DNew(MRTC_PS2AsyncInstance) MRTC_PS2AsyncInstance;

	MRTC_PS2FileInfo *pFile = (MRTC_PS2FileInfo*)_pFile;
	pNewInstance->m_pFile = pFile;
	pNewInstance->m_Position = _FileOffset;
	pNewInstance->m_DataSize = _DataSize;
	pNewInstance->m_ReadBytes = 0;
	pNewInstance->m_bCompleted = false;
	pNewInstance->m_Data = _pData;

	g_AsyncReader.AddInstance( pNewInstance );
//	pNewInstance->IsCompleted();	// Initiate reading of instance if reader isn't busy

	return pNewInstance;
}

#elif defined( PS2_USE_HOST )

bool MRTC_PS2AsyncReaderHost::IsCompleted( MRTC_PS2AsyncInstance *_pInstance )
{
	m_CriticalSection.Lock();

	if( _pInstance->m_bCompleted )
		{
			m_CriticalSection.Unlock();
		return true;
	}

	sceLseek( _pInstance->m_pFile->m_File, _pInstance->m_Position, SEEK_SET );
	if( sceRead( _pInstance->m_pFile->m_File, _pInstance->m_Data, _pInstance->m_DataSize ) == -1 )
		Error_static("MRTC_PS2AsyncReader::IsCompleted", "Error occured while reading.");	// Error occured while reading

	_pInstance->m_bCompleted = true;

	m_CriticalSection.Unlock();

	return true;
}

void *MRTC_SystemInfo::OS_FileAsyncRead( void *_pFile, void *_pData, fint _DataSize, fint _FileOffset )
{
	M_ASSERT(_DataSize <= 0xffffffff, "Cannot read more that 4 gigs, but we don't have that much memory space anyways");

	MRTC_PS2AsyncInstance *pNewInstance = DNew(MRTC_PS2AsyncInstance) MRTC_PS2AsyncInstance;

	MRTC_PS2FileInfo *pFile = (MRTC_PS2FileInfo*)_pFile;
	pNewInstance->m_pFile = pFile;
	pNewInstance->m_Position = _FileOffset;
	pNewInstance->m_DataSize = _DataSize;
	pNewInstance->m_bCompleted = false;
	pNewInstance->m_Data = _pData;

	return pNewInstance;
}

#else
#error "This should not occur!"
#endif

fint MRTC_SystemInfo::OS_FileAsyncBytesProcessed(void *_pAsyncIntance)
{
	return ((MRTC_PS2AsyncInstance*)_pAsyncIntance)->BytesProcessed();
}

#if defined( PS2_USE_HOST )
	// HM note: had to get the screenshot to host somehow
	void *MRTC_SystemInfo::OS_FileAsyncWrite( void *_pFile, const void *_pData, fint _DataSize, fint _FileOffset )
	{
		M_ASSERT(_DataSize <= 0xffffffff, "Cannot read more that 4 gigs, but we don't have that much memory space anyways");
	
		// Instead of creating a request, just write it straight away (FileOffset ignored)
		
		MRTC_PS2FileInfo *pFile = (MRTC_PS2FileInfo*)_pFile;
	
		if( sceWrite( pFile->m_File, _pData, _DataSize ) < 0 )
		{
			Error_static("MRTC_SystemInfo::OS_FileAsyncWrite", "Error occured while writing." );
		}
		return NULL;
	}

#else
//JK-TODO: Implement PS2 harddrive support.
void *MRTC_SystemInfo::OS_FileAsyncWrite( void *_pFile, const void *_pData, fint _DataSize, fint _FileOffset )
{
	return NULL;
}
#endif
void MRTC_SystemInfo::OS_FileAsyncClose( void *_pAsyncInstance )
{
	// host transfers cannot be async so this should return immediately
	while( !((MRTC_PS2AsyncInstance*)_pAsyncInstance)->IsCompleted() )
	{
		OS_Sleep(1);
	}

	g_AsyncReader.RemoveInstance( (MRTC_PS2AsyncInstance*)_pAsyncInstance );

//	deleteP(((MRTC_PS2AsyncInstance*)_pAsyncInstance), g_AsyncRequestPool);
	delete _pAsyncInstance;
}

bool MRTC_SystemInfo::OS_FileAsyncIsFinished( void *_pAsyncInstance )
{
	return ((MRTC_PS2AsyncInstance*)_pAsyncInstance)->IsCompleted();
}

//JK-TODO: Implement PS2 harddrive support
bool MRTC_SystemInfo::OS_FileSetFileSize( const char *_pFilename, fint _FileSize )
{
	return false;
}

int MRTC_SystemInfo::OS_FileOperationGranularity( const char *_pPath )
{
/*
	if( _pPath )
	{
		if( !stricmp( _pPath, "cdrom0" ) )
			return 2048;
		else if( !stricmp( _pPath, "host0" ) )
			return 1;
	}
*/
	return ( 16 * 2048 );	// Assume CD/DVD
}

bool MRTC_SystemInfo::OS_FileGetTime( void *_pFile, int64& _TimeCreate, int64& _TimeAccess, int64& _TimeWrite )
{
	return false;
}

bool MRTC_SystemInfo::OS_FileSetTime( void *_pFile, const int64& _TimeCreate, const int64& _TimeAccess, const int64& _TimeWrite )
{
	return false;
}

void MRTC_SystemInfo::OS_FileGetDrive( const char *_pFilename, char *_pDriveName )
{
	// extract host0, cdrom0, whatever
	char *pPos = strstr( _pFilename, ":" );
	if( pPos )
	{
		int32 nPos = (int32)( pPos - _pFilename );
		strncpy( _pDriveName, _pFilename, nPos );
		_pDriveName[nPos] = 0;
	}
	else
		_pDriveName[0] = 0;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Heap
|__________________________________________________________________________________________________
\*************************************************************************************************/

#include "dlmalloc.c"

static int s_nTotalHeapSize = -1;
static uint8 s_DLMutex[sizeof(MRTC_CriticalSection)];
static MRTC_CriticalSection * s_pDLMutex = NULL;
extern "C" char _end[];
extern "C" char _stack_size[], _stack[];

static void PS2_InitHeap()
{
	if (s_nTotalHeapSize == -1)
	{
/*
		char* pArenaLo = (char*)&_end;				// Temporary solution, move low 0.5MB upwards so system has some room to play in
		char* pArenaHi = (char*)((128*1024*1024)-(int)&_stack_size);	// All memory minus stack

		s_pDLMutex = new (s_DLMutex) MRTC_CriticalSection;

		s_nTotalHeapSize = pArenaHi - pArenaLo;

		dlmalloc_initstatic(pArenaLo, s_nTotalHeapSize);
		printf("MRTC: Available memory %.2f MB @ 0x%.8X\n", s_nTotalHeapSize / float(1<<20), pArenaLo);
*/

		s_pDLMutex = new (s_DLMutex) MRTC_CriticalSection;

		int elf_end = (int)&_end;
		int stack_size = (int)&_stack_size;
		int stack = (int)&_stack;
		if( stack == -1 ) stack = 128 * 1024 * 1024 - stack_size;
		int work_area = 64*1024;	// Give system 512k to play with
		int heap_size = ( stack ) - ( elf_end + stack_size + work_area + 32768 ); 

		s_nTotalHeapSize = heap_size;
				
		void* pTemp = malloc( work_area );
		void* pMem = malloc( heap_size );
		free( pTemp );
		dlmalloc_initstatic(pMem, s_nTotalHeapSize);
		char aBuf[128];
		sprintf( aBuf, "MRTC: Available memory %.2f MB @ 0x%.8X, elf ended at 0x%x\n", s_nTotalHeapSize / float(1<<20), pMem, elf_end );
		scePrintf( aBuf );
	}
}


void* MRTC_SystemInfo::OS_HeapAlloc(uint32 _Size)
{
	PS2_InitHeap();
	M_LOCK(*s_pDLMutex);
	return dlmalloc(_Size);
}

void* MRTC_SystemInfo::OS_HeapAllocAlign(uint32 _Size, uint32 _Align)
{
	PS2_InitHeap();
	M_LOCK(*s_pDLMutex);
	return dlmemalign(_Align, _Size);
}

void* MRTC_SystemInfo::OS_HeapRealloc(void *_pMem, uint32 _Size)
{	
	PS2_InitHeap();
	M_LOCK(*s_pDLMutex);
	return dlrealloc(_pMem, _Size);
}

void MRTC_SystemInfo::OS_HeapFree(void *_pMem)
{
	PS2_InitHeap();
	M_LOCK(*s_pDLMutex);
	dlfree(_pMem);
}

uint32 MRTC_SystemInfo::OS_HeapSize(const void *_pMem)
{
	PS2_InitHeap();
	M_LOCK(*s_pDLMutex);
	return dlmalloc_usable_size((void*)_pMem);
}

uint32 MRTC_SystemInfo::OS_PhysicalMemoryFree()
{
	PS2_InitHeap();
	M_LOCK(*s_pDLMutex);
	return dlmallinfo().fordblks + dlmalloc_heapfree();
}

uint32 MRTC_SystemInfo::OS_PhysicalMemoryLargestFree()
{
	PS2_InitHeap();
	M_LOCK(*s_pDLMutex);
	return dlmallinfo().lrgblk;
}

uint32 MRTC_SystemInfo::OS_PhysicalMemoryUsed()
{
	PS2_InitHeap();
	M_LOCK(*s_pDLMutex);
	mallinfo mi = dlmallinfo();
	return mi.uordblks + mi.hblkhd;
}

uint32 MRTC_SystemInfo::OS_PhysicalMemorySize()
{
	PS2_InitHeap();
	M_LOCK(*s_pDLMutex);
	return s_nTotalHeapSize;
}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// PS2 time class

#ifdef	DEBUG_PS2_TIME

union PS2TimeUnion
{
	int64 m_Time64;
	struct
	{
		uint32 lo;
		int32 hi;
	} m_Time32;
};

CMTime_PS2::CMTime_PS2()
{
	Reset();
}

CMTime_PS2::CMTime_PS2( bool _bReset )
{
	if( _bReset )
		Reset();
}

CMTime_PS2::CMTime_PS2(const CMTime_PS2 &_Time)
{
	m_aTime[0]	= _Time.m_aTime[0];
	m_aTime[1]	= _Time.m_aTime[1];
}

bool CMTime_PS2::operator != (const CMTime_PS2 &_Time)
{
	return ( m_aTime[0] != _Time.m_aTime[0] ) || ( m_aTime[1] != _Time.m_aTime[1] );
}

CMTime_PS2& CMTime_PS2::operator = (const CMTime_PS2 &_Time)
{
	m_aTime[0]	= _Time.m_aTime[0];
	m_aTime[1]	= _Time.m_aTime[1];
	return *this;
}

CMTime_PS2 CMTime_PS2::operator + (const CMTime_PS2 &_Time) const
{
	PS2TimeUnion Time1;
	PS2TimeUnion Time2;

	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= _Time.m_aTime[0];
	Time2.m_Time32.hi	= _Time.m_aTime[1];

	Time1.m_Time64	= Time1.m_Time64 + Time2.m_Time64;

	CMTime_PS2 Time( false );
	Time.m_aTime[0]	= Time1.m_Time32.lo;
	Time.m_aTime[1]	= Time1.m_Time32.hi;
	return Time;
}

CMTime_PS2 CMTime_PS2::operator - (const CMTime_PS2 &_Time) const
{
	PS2TimeUnion Time1;
	PS2TimeUnion Time2;

	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= _Time.m_aTime[0];
	Time2.m_Time32.hi	= _Time.m_aTime[1];
	
	Time1.m_Time64	= Time1.m_Time64 - Time2.m_Time64;

	CMTime_PS2 RetTime( false );
	RetTime.m_aTime[0] = Time1.m_Time32.lo;
	RetTime.m_aTime[1] = Time1.m_Time32.hi;

	return RetTime;
}

CMTime_PS2& CMTime_PS2::operator += (const CMTime_PS2 &_Time)
{
	PS2TimeUnion Time1;
	PS2TimeUnion Time2;

	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= _Time.m_aTime[0];
	Time2.m_Time32.hi	= _Time.m_aTime[1];

	Time1.m_Time64	= Time1.m_Time64 + Time2.m_Time64;

	m_aTime[0]	= Time1.m_Time32.lo;
	m_aTime[1]	= Time1.m_Time32.hi;

	return *this;
}

CMTime_PS2& CMTime_PS2::operator -= (const CMTime_PS2 &_Time)
{
	PS2TimeUnion Time1;
	PS2TimeUnion Time2;

	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= _Time.m_aTime[0];
	Time2.m_Time32.hi	= _Time.m_aTime[1];
	
	Time1.m_Time64	= Time1.m_Time64 - Time2.m_Time64;

	m_aTime[0]	= Time1.m_Time32.lo;
	m_aTime[1]	= Time1.m_Time32.hi;

	return *this;
}

void CMTime_PS2::Reset()
{
	m_aTime[0]	= 0;
	m_aTime[1]	= 0;
}

bool CMTime_PS2::IsReset() const
{
	return (m_aTime[0] == 0 ) && ( m_aTime[1] == 0 );
}

CMTime_PS2 CMTime_PS2::CreateInvalid()
{
	CMTime_PS2 Invalid( false );
	Invalid.MakeInvalid();
	return Invalid;
}

void CMTime_PS2::MakeInvalid()
{
	m_aTime[0]	= 0xffffffff;
	m_aTime[1]	= 0xffffffff;
}

bool CMTime_PS2::IsInvalid() const
{
	return ( m_aTime[0] == 0xffffffff ) && ( m_aTime[1] == 0xffffffff );
}

void CMTime_PS2::ResetMax()
{
	m_aTime[0]	= 0xffffffff;
	m_aTime[1]	= 0x7fffffff;
}

void CMTime_PS2::Snapshot()
{
	PS2TimeUnion Time;
	
	Time.m_Time64 = MGetCPUClock();

	m_aTime[0] = Time.m_Time32.lo;
	m_aTime[1] = Time.m_Time32.hi;
}

void CMTime_PS2::Start()
{
	PS2TimeUnion Time;
	
	Time.m_Time64 = -MGetCPUClock();

	m_aTime[0] = Time.m_Time32.lo;
	m_aTime[1] = Time.m_Time32.hi;
}

void CMTime_PS2::Stop()
{
	CMTime_PS2 moo( false );
	PS2TimeUnion Time;
	
	Time.m_Time64 = MGetCPUClock();
	moo.m_aTime[0] = Time.m_Time32.lo;
	moo.m_aTime[1] = Time.m_Time32.hi;

	*this	+= moo;
}

void CMTime_PS2::Add(CMTime_PS2 &_Time)
{
	*this += _Time;
}

fp4 CMTime_PS2::GetTime() const
{
	uint32 lo;
	int32 hi;

	fp4 Time0, Time1;
	fp4 Recp = MGetCPUFrequencyRecp();

	lo	= m_aTime[0];
	hi	= (int32)m_aTime[1];
	if( hi < 0 )
	{
		Recp = -Recp;
		lo	= -lo;
		hi	= -hi;
	}
	
	Time0 = lo * Recp;
	Time1 = hi * (Recp * (fp4)(2147483648.0f*2.0f));
	
	return Time0 + Time1;
}

int64 CMTime_PS2::GetCycles() const
{
	PS2TimeUnion Time;

	Time.m_Time32.lo	= m_aTime[0];
	Time.m_Time32.hi	= m_aTime[1];

	return Time.m_Time64;
}

int CMTime_PS2::Compare(const CMTime_PS2 &_Time) const
{

//	if (m_Time < _Time.m_Time)
//		return -1;
//	else if (m_Time > _Time.m_Time)
//		return 1;

	int32 hi0 = (int32)m_aTime[1];
	int32 hi1 = (int32)_Time.m_aTime[1];
	if( hi0 < hi1 )
		return -1;
	else if( hi0 > hi1 )
		return 1;
	else
	{
		uint32 lo0 = m_aTime[0];
		uint32 lo1 = _Time.m_aTime[0];
		if( ((int32)m_aTime[1]) < 0 )
		{
			// Time is negative
			lo0	= -lo0;
			lo1	= -lo1;
		}
		if( lo0 < lo1 )
			return -1;
		else if( lo0 > lo1 )
			return 1;
	}

	return 0;
}

int CMTime_PS2::Compare(fp4 _Time) const
{
	return Compare( CreateFromSeconds( _Time ) );
}

CMTime_PS2 CMTime_PS2::Modulus(fp4 _Modulus) const
{
	CMTime_PS2 Modul = CMTime_PS2::CreateFromSeconds( _Modulus );
	PS2TimeUnion Time1, Time2, RetTime;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= Modul.m_aTime[0];
	Time2.m_Time32.hi	= Modul.m_aTime[1];
	
	int NegMod = 0;
	if( Time2.m_Time64 < 0 )
	{
		NegMod = 1;
		Time2.m_Time64 = -Time2.m_Time64;
	}
	
//	RetTime.m_Time64 = Time1.m_Time64 % Time2.m_Time64;
	RetTime.m_Time64 = Time_GetModulus( Time1.m_Time64, Time2.m_Time64 );
	if( NegMod )
	{
		RetTime.m_Time64 = -RetTime.m_Time64;
	}

	CMTime_PS2 Ret( false );
	Ret.m_aTime[0]	= RetTime.m_Time32.lo;
	Ret.m_aTime[1]	= RetTime.m_Time32.hi;
	
	return Ret;
}

CMTime_PS2 CMTime_PS2::Modulus(const CMTime_PS2& _Modulus) const
{
	PS2TimeUnion Time1, Time2, RetTime;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= _Modulus.m_aTime[0];
	Time2.m_Time32.hi	= _Modulus.m_aTime[1];

	int NegMod = 0;
	if( Time2.m_Time64 < 0 )
	{
		NegMod = 1;
		Time2.m_Time64 = -Time2.m_Time64;
	}
	
//	RetTime.m_Time64 = Time1.m_Time64 % Time2.m_Time64;
	RetTime.m_Time64 = Time_GetModulus( Time1.m_Time64, Time2.m_Time64 );
	if( NegMod )
	{
		RetTime.m_Time64 = -RetTime.m_Time64;
	}

	CMTime_PS2 Ret( false );
	Ret.m_aTime[0]	= RetTime.m_Time32.lo;
	Ret.m_aTime[1]	= RetTime.m_Time32.hi;

	return Ret;
}

CMTime_PS2 CMTime_PS2::ModulusScaled(fp4 _Scale, fp4 _Modulus) const
{
	_Modulus = _Modulus * (1.0f / _Scale);

	CMTime_PS2 Modul = CMTime_PS2::CreateFromSeconds( _Modulus );
	int64 Int64Scale = (_Scale * 65536.0f);
	// This isn't really safe, should be done with 128 bit ints

	PS2TimeUnion Time1, Time2, RetTime;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= Modul.m_aTime[0];
	Time2.m_Time32.hi	= Modul.m_aTime[1];

//	RetTime.m_Time64 = (((Time1.m_Time64 % Time2.m_Time64)) * Int64Scale) >> 16;
	RetTime.m_Time64 = ((Time_GetModulus(Time1.m_Time64, Time2.m_Time64)) * Int64Scale) >> 16;

	CMTime_PS2 Ret( false );
	Ret.m_aTime[0]	= RetTime.m_Time32.lo;
	Ret.m_aTime[1]	= RetTime.m_Time32.hi;

	return Ret;
}

CMTime_PS2 CMTime_PS2::Scale(fp4 _Scale) const
{
/*
	CMTime_PS2 Ret;

	int64 Int64Scale = (_Scale * 65536.0f);
	// This isn't really safe, should be done with 128 bit ints

	PS2TimeUnion Time1, RetTime;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];

	RetTime.m_Time64 = (Time1.m_Time64 * Int64Scale) >> 16;
	Ret.m_aTime[0]	= RetTime.m_Time32.lo;
	Ret.m_aTime[1]	= RetTime.m_Time32.hi;

	return Ret;
*/
	int32 Scale = _Scale * 65536.0f;
	PS2TimeUnion Time1, RetTime;
	Time1.m_Time32.lo = m_aTime[0];
	Time1.m_Time32.hi = m_aTime[1];
	
	RetTime.m_Time64 = PS2DoubleMul( Time1.m_Time64, Scale ) >> 16;

	CMTime_PS2 Ret( false );
	Ret.m_aTime[0] = RetTime.m_Time32.lo;
	Ret.m_aTime[1] = RetTime.m_Time32.hi;
	
	return Ret;
}

fp4 CMTime_PS2::GetTimeFraction(fp4 _Modulus) const
{
	return GetTimeModulus(_Modulus) * (1.0f / _Modulus);
}

int CMTime_PS2::GetNumModulus(fp4 _Modulus) const
{
	CMTime_PS2 Modulus = CMTime_PS2::CreateFromSeconds( _Modulus );

	PS2TimeUnion Time1, Time2, RetTime;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= Modulus.m_aTime[0];
	Time2.m_Time32.hi	= Modulus.m_aTime[1];

//	return Time1.m_Time64 / Int64Modulus;
	return Time_GetModulusCount( Time1.m_Time64, Time2.m_Time64 );
}

int CMTime_PS2::GetNumTicks(fp4 _TicksPerSec) const
{
	int64 Int64Scale = (_TicksPerSec * 65536.0f);
	// This isn't really safe, should be done with 128 bit ints
	PS2TimeUnion Time1;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];

	int64 Ticks = (Time1.m_Time64 * Int64Scale) >> 16;

//	return Ticks / MGetCPUFrequencyInt();
	return Time_GetModulusCount( Ticks, MGetCPUFrequencyInt() );
}

fp4 CMTime_PS2::GetTimeModulus(fp4 _Modulus) const
{
	CMTime_PS2 Modulus = CMTime_PS2::CreateFromSeconds( _Modulus );

	PS2TimeUnion Time1, Time2;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= Modulus.m_aTime[0];
	Time2.m_Time32.hi	= Modulus.m_aTime[1];

//	int64 Time = Time1.m_Time64 % Int64Modulus;
	Time1.m_Time64 = Time_GetModulus( Time1.m_Time64, Time2.m_Time64 );
	
	fp4 Recp = MGetCPUFrequencyRecp();;
	if( Time1.m_Time64 < 0 )
	{
		Time1.m_Time64	= -Time1.m_Time64;
		Recp = -Recp;
		
	}

	fp4 RetTime0 = Time1.m_Time32.lo * Recp;
	fp4 RetTime1 = Time1.m_Time32.hi * (Recp * (fp4)(2147483648.0f*2.0f));
	return (RetTime0 + RetTime1);
//	return (fp4)Time * MGetCPUFrequencyRecp();
}

fp4 CMTime_PS2::GetTimeModulusScaled(fp4 _Scale, fp4 _Modulus) const
{
	if( _Scale == 0.0f )
	{
		// Specialcase this fucker
		return 0.0f;
	}
	fp4 ScaleSign = 1.0f;
	if( _Scale < 0.0f )
	{
		ScaleSign = -ScaleSign;
		_Scale = -_Scale;
	}

	_Modulus = _Modulus * (1.0f / _Scale);

	const fp4 InvCPUFrequency = MGetCPUFrequencyRecp();
	
//	int64 Int64Modulus = _Modulus * CPUFrequency;
	CMTime_PS2 Modulus = CMTime_PS2::CreateFromSeconds( _Modulus );

	PS2TimeUnion Time1, Time2;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= Modulus.m_aTime[0];
	Time2.m_Time32.hi	= Modulus.m_aTime[1];

	int NegMod = 0;
	if( Time2.m_Time64 < 0 )
	{
		NegMod = 1;
		Time2.m_Time64 = -Time2.m_Time64;
	}

//	int64 Time = Time_GetModulus( Time1.m_Time64, Int64Modulus );
	int64 Time = Time_GetModulus( Time1.m_Time64, Time2.m_Time64 );
	if( NegMod )
	{
		Time = -Time;
	}

	int64 Remainder;
	int64 Count = Time_GetModulusCountAndRemainder( Time, MGetCPUFrequencyInt(), Remainder );

//	fp4 Modded = (fp4)Time * MGetCPUFrequencyRecp();
	fp4 Modded = (int)Count + ((fp4)((int)Remainder)) * InvCPUFrequency;
	return Modded * ScaleSign * _Scale;
}

fp4 CMTime_PS2::GetTimeSqrModulusScaled(fp4 _Scale, fp4 _Modulus) const
{
	if( _Scale == 0.0f )
	{
		// Specialcase this fucker
		return 0.0f;
	}
	fp4 ScaleSign = 1.0f;
	if( _Scale < 0.0f )
	{
		ScaleSign = -ScaleSign;
		_Scale = -_Scale;
	}

	_Scale *= GetTime();

	_Modulus = _Modulus * (1.0f / _Scale);

	const fp4 InvCPUFrequency = MGetCPUFrequencyRecp();

//	int64 Int64Modulus = _Modulus * CPUFrequency;
	CMTime_PS2 Modulus = CMTime_PS2::CreateFromSeconds( _Modulus );

	PS2TimeUnion Time1, Time2;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= Modulus.m_aTime[0];
	Time2.m_Time32.hi	= Modulus.m_aTime[1];

	int NegMod = 0;
	if( Time2.m_Time64 < 0 )
	{
		NegMod = 1;
		Time2.m_Time64 = -Time2.m_Time64;
	}

	int64 TimeSqr = Time1.m_Time64;

//	int64 Time = TimeSqr % Int64Modulus;
	int64 Time;
	Time = Time_GetModulus( TimeSqr, Time2.m_Time64 );

	if( NegMod )
	{
		Time = -Time;
	}

	int64 Remainder;
	int64 Count = Time_GetModulusCountAndRemainder( Time, MGetCPUFrequencyInt(), Remainder );

//	fp4 Modded = (fp4)Time * MGetCPUFrequencyRecp();
	fp4 Modded = (int)Count + ((fp4)((int)Remainder)) * InvCPUFrequency;
	return Modded * ScaleSign * _Scale;
}

CMTime_PS2 CMTime_PS2::Max(const CMTime_PS2 &_Time) const
{
	PS2TimeUnion Time1, Time2, RetTime;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= _Time.m_aTime[0];
	Time2.m_Time32.hi	= _Time.m_aTime[1];

	RetTime.m_Time64 = ::Max(Time1.m_Time64, Time2.m_Time64);

	CMTime_PS2 Ret( false );
	Ret.m_aTime[0] = RetTime.m_Time32.lo;
	Ret.m_aTime[1] = RetTime.m_Time32.hi;

	return Ret;
}

CMTime_PS2 CMTime_PS2::Min(const CMTime_PS2 &_Time) const
{
	PS2TimeUnion Time1, Time2, RetTime;
	
	Time1.m_Time32.lo	= m_aTime[0];
	Time1.m_Time32.hi	= m_aTime[1];
	Time2.m_Time32.lo	= _Time.m_aTime[0];
	Time2.m_Time32.hi	= _Time.m_aTime[1];

	RetTime.m_Time64 = ::Min(Time1.m_Time64, Time2.m_Time64);

	CMTime_PS2 Ret( false );
	Ret.m_aTime[0] = RetTime.m_Time32.lo;
	Ret.m_aTime[1] = RetTime.m_Time32.hi;

	return Ret;
}


CMTime_PS2 CMTime_PS2::GetCPU()
{
	PS2TimeUnion Time1;
	Time1.m_Time64 = MRTC_SystemInfo::CPU_GetCycles();

	CMTime_PS2 Ret( false );
	Ret.m_aTime[0]	= Time1.m_Time32.lo;
	Ret.m_aTime[1]	= Time1.m_Time32.hi;
	return Ret;
}

CMTime_PS2 CMTime_PS2::CreateFromSeconds(fp4 _Seconds)
{
	int IntPart = (int)_Seconds;
	fp4 FracPart = _Seconds - (fp4)IntPart;
	int freqint = 294912000;
	fp4 freqfp4 = 294912000;
	uint64 _hi, _lo;
	fp4 tmp;
	asm("
		mult	IntPart, freqint
		mul.s	tmp, FracPart, freqfp4
		cvt.w.s	tmp,tmp
		mfhi	_hi
		mflo	_lo
		pextlw	_lo, _hi, _lo
		mfc1	_hi, tmp
		daddu	_lo, _lo, _hi
		");

	PS2TimeUnion Time1;
	Time1.m_Time64 = _lo;

	CMTime_PS2 Ret( false );
	Ret.m_aTime[0]	= Time1.m_Time32.lo;
	Ret.m_aTime[1]	= Time1.m_Time32.hi;

	return Ret;
}

CMTime_PS2 CMTime_PS2::CreateFromTicks(int _Ticks, fp4 _TickLength, fp4 _TickFraction )
{
	fp4 freqfp4 = 294912000.0f;
	fp4 TickLenfp4;
	int TickLenint;
	int64 Fracint;

	fp4 tmp;
	int64 _hi, _lo;
	asm("
		mul.s	TickLenfp4, _TickLength, freqfp4

		cvt.w.s	tmp, TickLenfp4
		mfc1	TickLenint, tmp

		mul.s	tmp, TickLenfp4, _TickFraction
		mult	TickLenint, _Ticks
		cvt.w.s	tmp, tmp
		mfc1	Fracint, tmp
		mfhi	_hi
		mflo	_lo
		pextlw	_lo, _hi, _lo
		daddu	_lo, _lo, Fracint
		");

	PS2TimeUnion Time1;
	Time1.m_Time64 = _lo;

	CMTime_PS2 Ret( false );
	Ret.m_aTime[0]	= Time1.m_Time32.lo;
	Ret.m_aTime[1]	= Time1.m_Time32.hi;
	
	return Ret;
}

void CMTime_PS2::Pack(uint8 *&_pPtr) const
{
	memcpy( _pPtr, &m_aTime, 8 );
	_pPtr += 8;
}

void CMTime_PS2::Unpack(const uint8 *&_pPtr)
{
	memcpy( &m_aTime, _pPtr, 8 );
	_pPtr += 8;
}

#endif	// DEBUG_PS2_TIME

/*
void PS2WordCopy( void* _pDest, const void* _pSrc, unsigned int _nWords )
{
	register const uint32 *pSrc = (const uint32*)_pSrc;
	register uint32 *pDest = (uint32*)_pDest;
	unsigned int odd = _nWords & 3;
	_nWords	-= odd;
	
	while( _nWords > 0 )
	{
		register uint32 a, b, c, d;
		a	= pSrc[0];
		b	= pSrc[1];
		c	= pSrc[2];
		d	= pSrc[3];
		pDest[0]	= a;
		pDest[1]	= b;
		pDest[2]	= c;
		pDest[3]	= d;
		_nWords	-= 4;
		pDest	+= 4;
		pSrc	+= 4;
	}
	
	if( odd )
	{
		while( odd-- > 0 )
		{
			*pDest++	= *pSrc++;
		}
	}
}

void PS2QWordCopy( uint128* _pDest, uint128* _pSrc, unsigned int _nCount )
{
	unsigned int odd = _nCount & 3;
	_nCount -= odd;
	while( _nCount > 0 )
	{
		register uint128 a, b, c, d;
		
		a	= _pSrc[0];
		b	= _pSrc[1];
		c	= _pSrc[2];
		d	= _pSrc[3];
		_pDest[0]	= a;
		_pDest[1]	= b;
		_pDest[2]	= c;
		_pDest[3]	= d;
		
		_pDest	+= 4;
		_pSrc	+= 4;
		_nCount	-= 4;
	}
	
	if( odd )
	{
		while( odd-- > 0 )
		{
			*_pDest++	= *_pSrc++;
		}
	}
}
*/

#endif	// PLATFORM_PS2

