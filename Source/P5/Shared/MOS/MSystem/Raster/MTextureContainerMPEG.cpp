#include "PCH.h"
#include "../MSystem.h"


#if defined(PLATFORM_PS2)

#include <eekernel.h>
#include <libmpeg.h>
#include <sif.h>
#include <sifdev.h>
#include <sifrpc.h>
#include <libsdr.h>
#include <sdrcmd.h>

#define	_MTEXTURECONTAINERMPEG_INCLUDESUBSYS
#include "MTextureContainerMPEG.h"
#undef	_MTEXTURECONTAINERMPEG_INCLUDESUBSYS



#define MAX_WIDTH		640
#define MAX_HEIGHT		512
#define MAX_MBX         (MAX_WIDTH/16)
#define MAX_MBY         (MAX_HEIGHT/16)
#define ZERO_BUFF_SIZE  (0x800)
#define MPEG_BUFF_SIZE  (SCE_MPEG_BUFFER_SIZE(MAX_WIDTH, MAX_HEIGHT))
#define DEMUX_BUFF_SIZE (192 * 1024)
#define IOP_BUFF_SIZE   (12288 * 2) // 512 * 48
#define AUDIO_BUFF_SIZE (IOP_BUFF_SIZE * 16)
#define READCHUNKSIZE	(16*1024) // (16*1024)
#define NUMREADCHUNKS	16
#define MUXBUFFSIZE		READCHUNKSIZE * NUMREADCHUNKS
#define DMACHUNKSIZE    4096          // bytes


#define UNCMASK 0x0fffffff
#define UNCBASE 0x20000000
#define DmaAddr(val) \
    ((void*)((unsigned int)(val) & UNCMASK))
#define UncachedAddr(val) \
    ((void*)(((unsigned int)(val) & UNCMASK) | UNCBASE))


template <class T>
T MAX( T a, T b )
{
	return (a>b)?a:b;
}

template <class T>
T MIN( T a, T b )
{
	return (a<b)?a:b;
}

int IopAlloc( uint32 _Size );
int IopAlloc( uint32 _Size )
{
	void *ret = sceSifAllocIopHeap(_Size);
	M_ASSERT( ret != 0, "CMpegStream::IopAlloc(..) Cannot allocate IOP memory\n" );
	return( int( ret ) );
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMpegASyncReader
|__________________________________________________________________________________________________
\*************************************************************************************************/
CMpegASyncReader	g_ASyncReader;
CMpegFilePreCache	g_PreCache;

void RotateThreads();
void RotateThreads()
{
	RotateThreadReadyQueue( 1 );
	
	asm __volatile__
	(
		"addi $2, $0, 1"
		"addi $1, $0, 100"
		"sll  $1, $1, 3"
		"loop:"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"nop"
		"sub  $1, $1, $2"
		"bne  $0, $1, loop"
		:
		:
		: "$1", "$2"
	);
	
}

fp64		g_BytesRead	= 0;
fp64		g_StartTime = 0;
fp64		g_ReadTime	= 0;

// --- CMpegASyncReaderStreamRequest --------------------------------------------------------------
CMpegASyncReaderStreamRequest::CMpegASyncReaderStreamRequest( void *_pBuffer, int _FilePos, uint32 _Size )
{
	m_Request.SetRequest( _pBuffer, _FilePos, _Size, false );
	m_pNext = NULL;
	m_pPrev = NULL;
	m_Size	= _Size;
}

// --- CMpegASyncReaderStreamRequest --------------------------------------------------------------
CMpegASyncReaderStreamRequest::~CMpegASyncReaderStreamRequest()
{
}

// --- CMpegASyncReaderStream ---------------------------------------------------------------------
#define RINGBUFFER_CHUNKSIZE	16384
#define RINGBUFFER_NUMCHUNKS	16
#define RINGBUFFER_SIZE			(RINGBUFFER_CHUNKSIZE * RINGBUFFER_NUMCHUNKS)

CMpegASyncReaderStream::CMpegASyncReaderStream( CCFile *_pFile, uint32 _FileSize )
{
	m_pFile			= _pFile;
	m_pRingBuffer	= (uint8*)MRTC_GetMemoryManager()->AllocAlign( RINGBUFFER_SIZE, 64 );
	m_pRequests		= NULL;
	m_GetPtr		= 0;
	m_PutPtr		= 0;
	m_SetPtr		= 0;
	m_DataWritten	= 0;
	m_DataRequested	= 0;
	m_bStopped		= true;
	m_FilePos		= 0;
	m_RequestPos	= 0;
	m_FileStartPos	= m_pFile ? m_pFile->Pos() : 0;
	m_FileSize		= _FileSize;
	m_pNext			= NULL;
	m_pPrev			= NULL;
}

CMpegASyncReaderStream::~CMpegASyncReaderStream()
{
	m_CriticalSection.Lock();

	DeleteAllRequests();
	MRTC_GetMemoryManager()->Free( m_pRingBuffer );

	m_CriticalSection.Unlock();
}

void CMpegASyncReaderStream::DeleteAllRequests()
{
	while( m_pRequests )
		RemoveRequest( m_pRequests );
}

void CMpegASyncReaderStream::Start()
{
	m_CriticalSection.Lock();
	m_bStopped = false;
	m_CriticalSection.Unlock();
}

void CMpegASyncReaderStream::Stop()
{
	m_CriticalSection.Lock();
	m_bStopped = true;
	m_CriticalSection.Unlock();
}

void CMpegASyncReaderStream::Rewind()
{
	m_CriticalSection.Lock();

	DeleteAllRequests();

	m_GetPtr			= 0;
	if( m_pFile ) 	
	{	// only do this if not using precached data
		m_pFile->Seek( 0 );
		m_DataWritten	= 0;
		m_DataRequested	= 0;
		m_PutPtr		= 0;
		m_SetPtr		= 0;
		m_bStopped		= false;
	}
	m_FilePos		= 0;
	m_RequestPos	= 0;
	m_FileStartPos	= m_pFile ? m_pFile->Pos() : 0;

	m_CriticalSection.Unlock();
}


void CMpegASyncReaderStream::Process()
{
	m_CriticalSection.Lock();
	
	// if stream is not stopped, check if there is space in ringbuffer for a new read request
	if( !m_bStopped )
	{
		uint32	DataFree;
		bool	bContinue = true;
		
		while( bContinue )
		{
			DataFree = RINGBUFFER_SIZE - ( m_DataWritten + m_DataRequested );

			if( DataFree >= RINGBUFFER_CHUNKSIZE )
			{
				int32 ReadLen = m_FileSize - m_RequestPos;
				if( ReadLen > RINGBUFFER_CHUNKSIZE )	
					ReadLen = RINGBUFFER_CHUNKSIZE;
				else									
					m_bStopped = true;

				AddRequest( m_pRingBuffer + m_PutPtr, m_FileStartPos + m_RequestPos, ReadLen );
				m_DataRequested	+= ReadLen;
				m_RequestPos 	+= ReadLen;
				m_PutPtr		 = (m_PutPtr + ReadLen) % RINGBUFFER_SIZE;

				bContinue = !m_bStopped;			
			}	
			else bContinue = false;
		}
	}

	// remove finished requests
	CMpegASyncReaderStreamRequest *pRequest = m_pRequests;
	while( pRequest )
	{
		CMpegASyncReaderStreamRequest *pNextRequest = pRequest->m_pNext;

		if( !pRequest->m_Request.Done() )
		{
			m_pFile->Read( &pRequest->m_Request );

			if( !pRequest->m_Request.Done() )
				break;
		}

		if( pRequest->m_Request.Done() )
		{
			// this request is done, move fileptr and remove from list
			uint32 DataSize = pRequest->m_Request.m_nBytes;
			
			m_FilePos			+= DataSize;
			m_SetPtr	 		 = (m_SetPtr + DataSize) % RINGBUFFER_SIZE;
			m_DataWritten		+= DataSize;
			m_DataRequested		-= DataSize;
			RemoveRequest( pRequest );
		}
		pRequest = pNextRequest;
	}
	m_CriticalSection.Unlock();
}

void CMpegASyncReaderStream::GetAvailRingBufferData( uint8 *&_pRingTop, uint32 &_RingSize, uint8 *&_pStart, uint8 *&_pEnd, uint32 &_Size )
{
	m_CriticalSection.Lock();

	_pRingTop	= m_pRingBuffer;
	_RingSize	= RINGBUFFER_SIZE;
	_pStart 	= m_pRingBuffer + m_GetPtr;
	_pEnd		= m_pRingBuffer + m_SetPtr;
	_Size		= m_DataWritten;

	m_CriticalSection.Unlock();
}

void CMpegASyncReaderStream::MoveGetPtr( uint32 _Len )
{
	m_CriticalSection.Lock();
	
	m_DataWritten	-= _Len;
	m_GetPtr 		 = ( m_GetPtr + _Len ) % RINGBUFFER_SIZE;

	m_CriticalSection.Unlock();
}

void CMpegASyncReaderStream::AddRequest( void *_pBuffer, int _FilePos, uint32 _Size )
{
	CMpegASyncReaderStreamRequest *pNewRequest = DNew(CMpegASyncReaderStreamRequest) CMpegASyncReaderStreamRequest( _pBuffer, _FilePos, _Size );
	
	if( !m_pRequests )
	{
		m_pRequests = pNewRequest;
	}
	else
	{
		// link last 
		CMpegASyncReaderStreamRequest *pRequest	= m_pRequests;
		CMpegASyncReaderStreamRequest *pLastRequest;
		while( pRequest )
		{
			pLastRequest = pRequest;
			pRequest = pRequest->m_pNext;
		}
		pNewRequest->m_pPrev = pLastRequest;
		pNewRequest->m_pNext = NULL;
		pLastRequest->m_pNext = pNewRequest;
	}
	
	m_pFile->Read( &pNewRequest->m_Request );
}

void CMpegASyncReaderStream::RemoveRequest( CMpegASyncReaderStreamRequest *_pRequest )
{
	if( m_pRequests == _pRequest )
	{
		if( _pRequest->m_pNext )
		{
			_pRequest->m_pNext->m_pPrev = NULL;
			m_pRequests					= _pRequest->m_pNext;		
		}
		else
		{
			m_pRequests = NULL;
		}
	}
	else
	{
		_pRequest->m_pPrev->m_pNext = _pRequest->m_pNext;
		if( _pRequest->m_pNext )
		{
			_pRequest->m_pNext->m_pPrev = _pRequest->m_pPrev;
		}
	}
	delete _pRequest;
}

void CMpegASyncReaderStream::Dump()
{
	uint32 nRequests = 0;
	CMpegASyncReaderStreamRequest *pRequest = m_pRequests;
	while( pRequest )
	{
		nRequests++;
		pRequest = pRequest->m_pNext;
	}

	scePrintf
	( 
		"OPEN STREAM: % 40s STATUS = % 10s NUMREQ = % 4d FILE = % 7d\n", 
		m_pFile->GetFileName().Str(), 
		m_bStopped ? "STOPPED" : "RUNNING",
		nRequests,
		m_pFile->Length()
	);
}


void CMpegASyncReaderStream::SetPreCachedData( uint8 *_pData, uint32 _Size )
{
	m_CriticalSection.Lock();

	DeleteAllRequests();
	
	memcpy( m_pRingBuffer, _pData, _Size );

	m_GetPtr		= 0;
	m_PutPtr		= _Size % RINGBUFFER_SIZE;
	m_SetPtr		= _Size % RINGBUFFER_SIZE;
	m_DataWritten	= _Size;
	m_DataRequested	= 0;
	m_FilePos		= _Size;
	m_RequestPos	= _Size;
	if( m_pFile ) m_pFile->Seek( _Size );
	m_FileStartPos	= 0;

	if( !m_pFile )	m_bStopped = true;			// no more read request if all file data is in precache

	m_CriticalSection.Unlock();
}

// --- CMpegASyncReader ---------------------------------------------------------------------------

CMpegASyncReader::CMpegASyncReader()
{
	// cyclic list of streams
	m_pStreams	= NULL;
	// no reader thread yet
	m_ThreadId 	= -1;
}

CMpegASyncReader::~CMpegASyncReader()
{
	m_CriticalSection.Lock();
	
	CMpegASyncReaderStream *pDeleteStream = m_pStreams;
	if( pDeleteStream )
	{
		do
		{
			CMpegASyncReaderStream *pNextStream = pDeleteStream->m_pNext;
			delete pDeleteStream;
			pDeleteStream = pNextStream;
			
		} while( pDeleteStream != m_pStreams );
	}
	m_CriticalSection.Unlock();
}

void CMpegASyncReader::InsertStream( CMpegASyncReaderStream *_pStream )
{
	M_ASSERT( _pStream, "CMpegASyncReader::OpenStream, Stream is NULL" );
	
	m_CriticalSection.Lock();
	if( m_pStreams == NULL )
	{
		_pStream->m_pPrev = _pStream;
		_pStream->m_pNext = _pStream;
		m_pStreams 		  = _pStream;

		CreateReaderThread();
		StartThread( m_ThreadId, this );
	}
	else
	{
		_pStream->m_pPrev = m_pStreams->m_pPrev;
		_pStream->m_pNext = m_pStreams;
		
		m_pStreams->m_pPrev->m_pNext	= _pStream;
		m_pStreams->m_pPrev 			= _pStream;
		m_pStreams = _pStream;
	}
	m_CriticalSection.Unlock();
}

void CMpegASyncReader::RemoveStream( CMpegASyncReaderStream *_pStream )
{
	M_ASSERT( _pStream, "CMpegASyncReader::CloseStream, Stream is NULL" );

	m_CriticalSection.Lock();


	if( _pStream->m_pPrev == NULL && _pStream->m_pNext == NULL )
	{
		scePrintf( "    Stream not linked\n" );
	}

	if( _pStream->m_pNext == _pStream && _pStream->m_pPrev == _pStream )
	{	// only one in list
		m_pStreams = NULL;
		DeleteReaderThread();
	}
	else
	{
		if( m_pStreams == _pStream )
			m_pStreams = _pStream->m_pNext;
	
		_pStream->m_pPrev->m_pNext	= _pStream->m_pNext;
		_pStream->m_pNext->m_pPrev	= _pStream->m_pPrev;
	}
	m_CriticalSection.Unlock();
}

void CMpegASyncReader::CreateReaderThread()
{
	M_ASSERT( m_ThreadId == -1, "CMpegASyncReader::CreateReaderThread, Reader thread already created\n" );

	#define READERTHREAD_STACKSIZE	(16*1024)
	m_pThreadStack = (uint8*)MRTC_GetMemoryManager()->AllocAlign( READERTHREAD_STACKSIZE, 16 );
	memset( &m_TP, 0, sizeof( ThreadParam ) );

    m_TP.entry			= (void (*)(void*))CMpegASyncReader::ASyncReaderMain;
    m_TP.stack			= m_pThreadStack;
    m_TP.stackSize		= READERTHREAD_STACKSIZE;
    m_TP.initPriority	= 1;
    m_TP.gpReg			= &_gp;
    m_TP.option			= 0;

	m_ThreadId = CreateThread( &m_TP );
	#undef READERTHREAD_STACKSIZE
}


void CMpegASyncReader::DeleteReaderThread()
{
	M_ASSERT( m_ThreadId != -1, "CMpegASyncReader::DeleteReaderThread, no reader thread\n" );

	TerminateThread( m_ThreadId );
	DeleteThread( m_ThreadId );
	m_ThreadId = -1;
	
	MRTC_GetMemoryManager()->Free( m_pThreadStack );
}

uint32 g_NReaderThreadCalls 		= 0;
uint32 g_NReaderThreadCallsDemux	= 0;
bool   g_bDemux						= false;

void CMpegASyncReader::ASyncReaderMain( void* _pEntry )
{
	CMpegASyncReader *pReader = (CMpegASyncReader*)_pEntry;

	while( 1 )
	{
		g_NReaderThreadCalls++;
		if( g_bDemux )
			g_NReaderThreadCallsDemux++;
		
	
		pReader->m_CriticalSection.Lock();

		// add read requests from all open streams
		
		CMpegASyncReaderStream *pStream = pReader->m_pStreams;
		if( pStream )
		{
			do
			{
				pStream->Process();
				pStream = pStream->m_pNext;
			}
			while( pStream != pReader->m_pStreams );
		}	
		pReader->m_CriticalSection.Unlock();

		RotateThreads();
	}
}

// --- CMpegFilePreCacheData ----------...--------------------------------------------------------
#define PRECACHEDATA_MAXSIZE	(RINGBUFFER_SIZE/2) //  - RINGBUFFER_CHUNKSIZE)


CMpegFilePreCacheData::CMpegFilePreCacheData( const CStr &_FileName )
{
	m_FileName	= _FileName;
	m_pFile		= NULL;
	m_pData		= NULL;
	m_Size		= 0;
	m_pNext		= NULL;
	m_pPrev		= NULL;
}

CMpegFilePreCacheData::~CMpegFilePreCacheData()
{
	if( m_pData ) MRTC_GetMemoryManager()->Free( m_pData );
}

void CMpegFilePreCacheData::Read( bool _bPreCache )
{
	try
	{
		m_pFile = MNew(CCFile);
		m_pFile->OpenExt
		( 
			m_FileName, 
			CFILE_READ | CFILE_BINARY, 
			NO_COMPRESSION, 
			NORMAL_COMPRESSION, 
			0.5, 
			16, 
			32*1024 // 128*1024
		);
	}
	catch (CCExceptionFile)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
		m_pFile = NULL;
		return;
	}
#ifdef M_SUPPORTSTATUSCORRUPT
	catch (CCException)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
		m_pFile = NULL;
		return;
	}
#endif
	
	// Get File Size
	m_pFile->SeekToEnd();
	m_FileSize = m_pFile->Pos();
	m_pFile->Seek(0);
	
	if( m_FileSize < 128*1024 )
		_bPreCache = true;
	
	// read MAX( FileSize, PRECACEDATA_MAXSIZE )
	if( _bPreCache )
	{
	
		m_Size = m_FileSize;
		if( m_Size > PRECACHEDATA_MAXSIZE ) m_Size = PRECACHEDATA_MAXSIZE;
		
		m_pData = (uint8*)MRTC_GetMemoryManager()->AllocAlign( m_Size, 64 );
		m_pFile->Read( m_pData, m_Size );
	}
	
	m_pFile->Close();
	
	delete m_pFile;
}

bool CMpegFilePreCacheData::NameMatch( const CStr &_MatchName )
{
	return( m_FileName == _MatchName );
}

// --- CMpegFilePreCache ----------------------------------------------------------------------
CMpegFilePreCache::CMpegFilePreCache()
{
	m_pPreCached = NULL;
}

CMpegFilePreCache::~CMpegFilePreCache()
{
	Purge();
}

void CMpegFilePreCache::Purge()
{
	while( m_pPreCached ) 
		RemoveData( m_pPreCached );
}

void CMpegFilePreCache::PreCache( const CStr &_FileName, bool _bPreCacheData )
{
	CMpegFilePreCacheData *pNewData = DNew(CMpegFilePreCacheData) CMpegFilePreCacheData( _FileName );
	pNewData->Read( _bPreCacheData );
	InsertData( pNewData );
}

void CMpegFilePreCache::InsertData( CMpegFilePreCacheData *_pData )
{
	// link first
	_pData->m_pNext	= m_pPreCached;
	if( m_pPreCached ) m_pPreCached->m_pPrev = _pData;
	m_pPreCached = _pData;
}

void CMpegFilePreCache::RemoveData( CMpegFilePreCacheData *_pData )
{
	if( _pData == m_pPreCached )
	{	// head of list
		if( !_pData->m_pNext )
		{	// only one in list
			m_pPreCached = NULL;
		}
		else
		{	// more nodes in list
			_pData->m_pNext->m_pPrev = NULL;
			m_pPreCached = _pData->m_pNext;
		}
	}
	else
	{
		_pData->m_pPrev->m_pNext =	_pData->m_pNext;
		if( _pData->m_pNext )		_pData->m_pNext->m_pPrev = _pData->m_pPrev;
	}
	delete _pData;
}

CMpegFilePreCacheData *CMpegFilePreCache::Find( const CStr &_FindName )
{
	CMpegFilePreCacheData *pData = m_pPreCached;
	while( pData )
	{
		if( pData->NameMatch( _FindName ) )
		{
			return( pData );
		}
		pData = pData->m_pNext;
	}
	return( NULL );
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMpegAudioDec
|__________________________________________________________________________________________________
\*************************************************************************************************/
const uint32 CMpegAudioDec::ms_cHdrSize =	sizeof( CMpegAudioDec::SpuStreamHeader ) + 
											sizeof( CMpegAudioDec::SpuStreamBody );


#define AUTODMA_CH			0
#define AU_HEADER_SIZE		40
#define UNIT_SIZE			1024
#define PRESET_VALUE(count)	(count)


CMpegAudioDec::CMpegAudioDec()
{
	m_pCurrentStream = NULL;
}

CMpegAudioDec::~CMpegAudioDec()
{
}

void CMpegAudioDec::Create
(
	uint8	*_pBuff,
	int32	 _BuffSize,
	int32	 _IopBuff,
	int32	 _IopBuffSize,
	uint8	*_pZeroBuff,
	int32	 _IopZeroBuff,
	int32	 _ZeroBuffSize
)
{
	m_State			= AU_STATE_INIT;
	m_HdrCount		= 0;
	m_Data			= _pBuff;
	m_Put			= 0;
	m_Count			= 0;
	m_Size			= _BuffSize;
	m_TotalBytes	= 0;
	m_TotalBytesSent= 0;

	m_IopBuffSize	= _IopBuffSize;
	m_IopLastPos	= 0;
	m_IopPausePos	= 0;

	//
	// Audio data buffer on IOP
	//
	m_IopBuff		= _IopBuff;

	//
	// Zero data buffer on IOP
	//
	m_IopZero		= _IopZeroBuff;
	m_IopZeroSize	= _ZeroBuffSize;

	// send zero data to IOP
	memset (_pZeroBuff, 0, _ZeroBuffSize);
	SendToIop(m_IopZero, _pZeroBuff, _ZeroBuffSize);

	//
	// Change master volume
	//
//	SetMasterVolume(0x3fff);
}

void CMpegAudioDec::Delete()
{
//	this->SetMasterVolume(0);
}

void CMpegAudioDec::Pause()
{
	m_State = AU_STATE_PAUSE;

	SetInputVolume(0);
	
	//
	//	stop DMA and save the position to be played
	//
	int32 ret = sceSdRemote( 1, rSdBlockTrans, AUTODMA_CH, SD_TRANS_MODE_STOP, NULL, 0) & 0x00FFFFFF;

	m_IopPausePos = ret - m_IopBuff;

	//
	//	clear SPU buffer
	//
    sceSdRemote( 1, rSdVoiceTrans, AUTODMA_CH, SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA,
				 m_IopZero, 0x4000, m_IopZeroSize );
}

void CMpegAudioDec::Resume()
{
	SetInputVolume(0x7fff);

	sceSdRemote( 1, rSdBlockTrans, AUTODMA_CH,
				 (SD_TRANS_MODE_WRITE_FROM | SD_BLOCK_LOOP),
				 m_IopBuff, (m_IopBuffSize/UNIT_SIZE)*UNIT_SIZE,
				 m_IopBuff + m_IopPausePos );


	m_State = AU_STATE_PLAY;
}

void CMpegAudioDec::Start()
{
	Resume();
}

void CMpegAudioDec::Reset()
{
	//
	// Stop audio
	//
	Pause();

	m_State			= AU_STATE_INIT;
	m_HdrCount		= 0;
	m_Put			= 0;
	m_Count			= 0;
	m_TotalBytes	= 0;
	m_TotalBytesSent= 0;
	m_IopLastPos	= 0;
	m_IopPausePos	= 0;
}

void CMpegAudioDec::BeginPut
(
	uint8	**_Ptr0,
	int32	 *_Len0,
	uint8	**_Ptr1,
	int32	 *_Len1
)
{
	//
	// return ADS header area when (state == AU_STATE_INIT)
	//
	if (m_State == AU_STATE_INIT) 
	{
		*_Ptr0 = (uint8*)&m_Sshd + m_HdrCount;
		*_Len0 = ms_cHdrSize - m_HdrCount;
		*_Ptr1 = (uint8*)m_Data;
		*_Len1 = m_Size;
		return;
	}

	//
	// Return the empty areas
	//
	int32 len = m_Size - m_Count;

	if (m_Size - m_Put >= len) 
	{ 
		// area0
		*_Ptr0 = m_Data + m_Put;
		*_Len0 = len;
		*_Ptr1 = NULL;
		*_Len1 = 0;
	} 
	else 
	{			    
		// area0 + area1
		*_Ptr0 = m_Data + m_Put;
		*_Len0 = m_Size - m_Put;
		*_Ptr1 = m_Data;
		*_Len1 = len - (m_Size - m_Put);
	}
}


void CMpegAudioDec::EndPut( int32 _Size )
{
	if (m_State == AU_STATE_INIT) 
	{
		int hdr_add = MIN( int(_Size), int(ms_cHdrSize - m_HdrCount));
		m_HdrCount += hdr_add;

		if (m_HdrCount >= ms_cHdrSize) 
		{
			m_State = AU_STATE_PRESET;
 /*
			scePrintf("-------- audio information --------------------\n");
			scePrintf("[%c%c%c%c]\n"
			"header size:                            %d\n"
			"type(0:PCM big, 1:PCM little, 2:ADPCM): %d\n"
			"sampling rate:                          %dHz\n"
			"channels:                               %d\n"
			"interleave size:                        %d\n"
			"interleave start block address:         %d\n"
			"interleave end block address:           %d\n",
			m_Sshd.m_Id[0],
			m_Sshd.m_Id[1],
			m_Sshd.m_Id[2],
			m_Sshd.m_Id[3],
			m_Sshd.m_Size,
			m_Sshd.m_Type,
			m_Sshd.m_Rate,
			m_Sshd.m_Ch,
			m_Sshd.m_InterSize,
			m_Sshd.m_LoopStart,
			m_Sshd.m_LoopEnd
			);

			scePrintf("[%c%c%c%c]\n"
			"data size:                              %d\n",
			m_Ssbd.m_Id[0],
			m_Ssbd.m_Id[1],
			m_Ssbd.m_Id[2],
			m_Ssbd.m_Id[3],
			m_Ssbd.m_Size
			);
*/
		}
		_Size -= hdr_add;
	}
	m_Put = (m_Put + _Size) % m_Size;
	m_Count += _Size;
	m_TotalBytes += _Size;
}


bool CMpegAudioDec::IsPreset()
{
    return( m_TotalBytesSent >= PRESET_VALUE(m_IopBuffSize) );
}


int32 CMpegAudioDec::SendToIop()
{
	int pd0, pd1, d0, d1;
	u_char *ps0, *ps1;
	int s0, s1;
	int count_sent = 0;
	int countAdj;
	int pos = 0;

	switch (m_State) 
	{
	case AU_STATE_INIT:
		return 0;
		break;

	case AU_STATE_PRESET:
		pd0	= m_IopBuff + (m_TotalBytesSent) % m_IopBuffSize;
		d0	= m_IopBuffSize - m_TotalBytesSent;
		pd1	= 0;
		d1	= 0;
		break;

	case AU_STATE_PLAY:
		pos = ((sceSdRemote(1, rSdBlockTransStatus, AUTODMA_CH) & 0x00FFFFFF) - m_IopBuff);
		IopGetArea(&pd0, &d0, &pd1, &d1, pos);
		break;

	case AU_STATE_PAUSE:
		return( 0 );
		break;
	}

	ps0 = m_Data + (m_Put - m_Count + m_Size) % m_Size;
	ps1 = m_Data;

	// adjust to UNIT_SIZE boundary
	countAdj = (m_Count / UNIT_SIZE) * UNIT_SIZE;

	s0 = MIN(m_Data + m_Size - ps0, countAdj);
	s1 = countAdj - s0;

	if (d0 + d1 >= UNIT_SIZE && s0 + s1 >= UNIT_SIZE)
	{
		count_sent = SendToIop2area(pd0, d0, pd1, d1, ps0, s0, ps1, s1);
	}

	m_Count -= count_sent;

	m_TotalBytesSent += count_sent;
	m_IopLastPos = (m_IopLastPos + count_sent) % m_IopBuffSize;
	return( count_sent );
}

void CMpegAudioDec::IopGetArea( int32 *_pD0, int32 *_D0, int32 *_pD1, int32 *_D1, int32 _Pos )
{
	int len = (_Pos + m_IopBuffSize - m_IopLastPos - UNIT_SIZE) % m_IopBuffSize;

	int diff = _Pos - m_IopLastPos;

	// corrects overrun bug in previous version
	if (diff >= 0 && diff < UNIT_SIZE) 
	{
		*_pD0 = m_IopBuff;
		*_D0  = 0;
		*_pD1 = m_IopBuff;
		*_D1  = 0;
		return;
	}

	// adjust to UNIT_SIZE boundary
	len = (len / UNIT_SIZE) * UNIT_SIZE;

	if (m_IopBuffSize -  m_IopLastPos >= len) 
	{
		// area0
		*_pD0	= m_IopBuff + m_IopLastPos;
		*_D0	= len;
		*_pD1	= 0;
		*_D1	= 0;
	} 
	else 
	{
		// area0 + area1
		*_pD0	= m_IopBuff + m_IopLastPos;
		*_D0	= m_IopBuffSize - m_IopLastPos;
		*_pD1	= m_IopBuff;
		*_D1	= len - (m_IopBuffSize - m_IopLastPos);
	}
}

int32 CMpegAudioDec::SendToIop2area
(
	int32	 _pD0, 
	int32	 _D0, 
	int32	 _pD1, 
	int32	 _D1, 
	uint8	*_pS0, 
	int32	 _S0, 
	uint8	*_pS1, 
	int32	 _S1
)
{
	if (_D0 + _D1 < _S0 + _S1) 
	{
		int diff = (_S0 + _S1) - (_D0 + _D1);
		if (diff >= _S1) 
		{
			_S0 -= (diff - _S1);
			_S1 = 0;
		} 
		else 
		{
			_S1 -= diff;
		}
	}

	//
	// (d0 + d1 >= s0 + s1)
	//
	if (_S0 >= _D0) 
	{
		SendToIop(_pD0,				(uint8*)_pS0,		_D0);
		SendToIop(_pD1,				(uint8*)_pS0 + _D0,	_S0 - _D0);
		SendToIop(_pD1 + _S0 - _D0,	(uint8*)_pS1,		_S1);
	} 
	else 
	{ 
		// s0 < d0
		if (_S1 >= _D0 - _S0) 
		{
			SendToIop(_pD0,			_pS0,				_S0);
			SendToIop(_pD0 + _S0,	_pS1,				_D0 - _S0);
			SendToIop(_pD1,			_pS1 + _D0 - _S0,	_S1 - (_D0 - _S0));
		} 
		else 
		{ 
			// s1 < d0 - s0
			SendToIop(_pD0,			_pS0,		_S0);
			SendToIop(_pD0 + _S0,	_pS1,		_S1);
		}
	}
	return( _S0 + _S1 );
}

int32 CMpegAudioDec::SendToIop( int32 _Dst, uint8 *_Src, int32 _Size )
{
	sceSifDmaData transData;
	int did;

	if (_Size <= 0) 
	{
		return 0;
	}

	transData.data = (u_int)_Src;
	transData.addr = (u_int)_Dst;
	transData.size = _Size;
	transData.mode = 0; // caution
	FlushCache(0);

	did = sceSifSetDma( &transData, 1 );

	while (sceSifDmaStat(did) >= 0)
	{
	}

	return( _Size );
}

void CMpegAudioDec::SetMasterVolume( uint32 _Volume )
{
	int i;
	for( i = 0; i < 2; i++ ) 
	{
		sceSdRemote( 1, rSdSetParam, i | SD_P_MVOLL, _Volume );
		sceSdRemote( 1, rSdSetParam, i | SD_P_MVOLR, _Volume );
	}
}

void CMpegAudioDec::SetInputVolume( uint32 _Volume )
{
    sceSdRemote( 1, rSdSetParam, AUTODMA_CH | SD_P_BVOLL, _Volume );
    sceSdRemote( 1, rSdSetParam, AUTODMA_CH | SD_P_BVOLR, _Volume );
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMpegStream
|__________________________________________________________________________________________________
\*************************************************************************************************/
CMpegAudioDec	CMpegStream::ms_AudioDec;
int32			CMpegStream::ms_ReferenceCount = 0;

CMpegStream::CMpegStream( const CStr &_FileName, bool _bPlayAudio, uint32 _Width, uint32 _Height, uint32 _nFrames, float _FramesPerSecond )
{
	m_FileName = _FileName;

	CMpegFilePreCacheData *pPreCached = g_PreCache.Find( m_FileName );

	m_pFile = NULL;

	bool 	bCreateFile = !pPreCached || (pPreCached->GetFileSize() > pPreCached->GetSize());
	uint32 	FileSize;

	if( bCreateFile )
	{
		// open file if not enough precahce data
		try
		{
			m_pFile = MNew(CCFile);
			m_pFile->OpenExt
			( 
				_FileName, 
				CFILE_READ | CFILE_BINARY, 
				NO_COMPRESSION, 
				NORMAL_COMPRESSION, 
				0.5, 
				16, 
				32*1024 // 128*1024
			);
			
			if( !pPreCached )
			{	// not precached, so we do not know filsize yet (this happens when decodeinfo is called
				m_pFile->SeekToEnd();
				FileSize = m_pFile->Pos();
				m_pFile->Seek( 0 );
			}
		}
		catch (CCExceptionFile)
		{
			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
			return;
		}
#ifdef M_SUPPORTSTATUSCORRUPT
		catch (CCException)
		{
			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
			return;
		}
#endif
	}

	m_pStream = DNew(CMpegASyncReaderStream) CMpegASyncReaderStream( m_pFile, pPreCached ? pPreCached->GetFileSize() : FileSize );
	if( pPreCached )
	{
		m_pStream->SetPreCachedData( pPreCached->GetData(), pPreCached->GetSize() ); 
	}
	
	if( !_Width	)	_Width	= MAX_WIDTH;
	if( !_Height )  _Height	= MAX_HEIGHT;
	
	m_Width					= _Width;
	m_Height				= _Height;
	m_FramesPerSecond		= _FramesPerSecond;


	g_ASyncReader.InsertStream( m_pStream );
	if( m_pFile ) 
	{
		m_pStream->Start();
		RotateThreads();							// this will add new requests
	}

	m_pMpegBuff			= (uint8*)MRTC_GetMemoryManager()->AllocAlign( SCE_MPEG_BUFFER_SIZE( _Width, _Height ), 64 );
	m_mpegBuffSize		= SCE_MPEG_BUFFER_SIZE( _Width, _Height );
	m_pDemuxBuff		= (uint8*)MRTC_GetMemoryManager()->AllocAlign( DEMUX_BUFF_SIZE, 64 );
	m_DemuxBuffSize		= DEMUX_BUFF_SIZE;
	m_pDemuxBuffPast	= ((uint8*)m_pDemuxBuff) + 16;
	m_pDemuxBuffPresent = ((uint8*)m_pDemuxBuff);
	m_pDemuxBuffFuture	= ((uint8*)m_pDemuxBuff) + 16;

	m_bPlayAudio		= _bPlayAudio;
	m_Time.Reset();
	m_CurrentFrame		= 0;
	m_bDmaStarted		= false;
	m_nFrames			= _nFrames;
	m_bNFramesValid		= _nFrames ? true : false;
	m_bAutoRewind		= false;
	m_bWHValid			= false;
	m_State				= ePREBUFFER;

	if (++ms_ReferenceCount == 1)
	{
		ChangeThreadPriority( GetThreadId(), 1 );
		sceSdRemoteInit();
		sceMpegInit();
	}

	if (m_bPlayAudio)
	{
		m_pAudioBuff		= (uint8*)MRTC_GetMemoryManager()->AllocAlign( AUDIO_BUFF_SIZE, 64 );
		m_AudioBuffSize		= AUDIO_BUFF_SIZE;
		m_IopBuff			= IopAlloc( IOP_BUFF_SIZE );
		m_IopBuffSize		= IOP_BUFF_SIZE;
		m_pZeroBuff			= (uint8*)MRTC_GetMemoryManager()->AllocAlign( ZERO_BUFF_SIZE, 64 );  
		m_IopZeroBuff		= IopAlloc( ZERO_BUFF_SIZE );
		m_bAudioBufferFull	= false;

		CMpegStream *pCurrentStreamPlaying = (CMpegStream*)ms_AudioDec.GetCurrentStream();
		if( pCurrentStreamPlaying )
			pCurrentStreamPlaying->m_bPlayAudio = false;

		if (ms_AudioDec.GetCurrentStream() != NULL )
		{
			ms_AudioDec.Reset();
		}

		ms_AudioDec.Create
		(
			 m_pAudioBuff,	m_AudioBuffSize,
			 m_IopBuff,		m_IopBuffSize,
			 m_pZeroBuff,	m_IopZeroBuff, ZERO_BUFF_SIZE
		);

		ms_AudioDec.SetCurrentStream( this );

	}
	else
	{
		m_pAudioBuff = NULL;
		m_pZeroBuff = NULL;
		m_IopBuff = -1;
		m_IopZeroBuff = -1;
	}

	sceMpegCreate(&m_mp, m_pMpegBuff, m_mpegBuffSize);

	sceMpegAddStrCallback(&m_mp, sceMpegStrM2V, 0, (sceMpegCallback)CMpegStream::CBVideo, this );
	sceMpegAddStrCallback(&m_mp, sceMpegStrPCM, 0, (sceMpegCallback)CMpegStream::CBAudio, this );
	sceMpegAddCallback(&m_mp, sceMpegCbNodata,		CBNodata,		this );
	sceMpegAddCallback(&m_mp, sceMpegCbBackground,	CBBackground,	this );
	sceMpegAddCallback(&m_mp, sceMpegCbError,		CBError,		this );
	
}

CMpegStream::~CMpegStream()
{
	g_ASyncReader.RemoveStream( m_pStream );
	delete m_pFile;
	delete m_pStream;

	sceMpegDelete(&m_mp);

	MRTC_GetMemoryManager()->Free( m_pMpegBuff );
	MRTC_GetMemoryManager()->Free( m_pDemuxBuff );

	if( --ms_ReferenceCount == 0)
	{
	}

	if (m_bPlayAudio)
	{
		ms_AudioDec.Reset();
		ms_AudioDec.Delete();
		ms_AudioDec.SetCurrentStream( NULL );
	}

	if (m_pAudioBuff) MRTC_GetMemoryManager()->Free( m_pAudioBuff );
	if (m_pZeroBuff) MRTC_GetMemoryManager()->Free( m_pZeroBuff );
	if (m_bPlayAudio)
	{
		if (m_IopBuff != -1) sceSifFreeIopHeap((void*)m_IopBuff);
		if (m_IopZeroBuff != -1) sceSifFreeIopHeap((void*)m_IopZeroBuff);
	}	
}

void CMpegStream::SetVolume( float _Volume )
{
	m_Volume = _Volume;
}

void CMpegStream::SetAutoRewind( bool _bAutoRewind )
{
	m_bAutoRewind = _bAutoRewind;
}

void CMpegStream::SetState( CMpegStream::eState _State )
{
	if (_State == ePREBUFFER)
	{
		if (m_State != ePREBUFFER)
		{
			//
			//	rewind
			//
			m_pStream->Rewind();
			
			sceMpegReset(&m_mp);
			m_pDemuxBuffPast	= ((uint8*)m_pDemuxBuff) + 16;
			m_pDemuxBuffPresent	= ((uint8*)m_pDemuxBuff);
			m_pDemuxBuffFuture	= ((uint8*)m_pDemuxBuff) + 16;
			m_CurrentFrame		= 0;
			m_bDmaStarted		= false;
			m_Time.Reset();								// make sure to restart timestamp counting
			m_bWHValid			= false;

			m_State 			= ePREBUFFER;
			
			if( m_bPlayAudio )
				ms_AudioDec.Reset();
		}
	}
	else if (_State == ePLAY)
	{
		// unpause audio if previous was pause
		sceMpegSetDecodeMode( &m_mp, SCE_MPEG_DECODE_ALL, SCE_MPEG_DECODE_ALL, SCE_MPEG_DECODE_ALL );
		
		if( m_pFile ) m_pStream->Start();
		
		if( m_bPlayAudio && m_State == ePAUSE )
		{
			ms_AudioDec.Resume();
		}
		m_State = _State;
	}
	else if (_State == ePAUSE)
	{
		m_State = _State;
		if( m_bPlayAudio )
			ms_AudioDec.Pause();
	}
	else if (_State == eSEEKEND)
	{
//		sceMpegSetDecodeMode( &m_mp, SCE_MPEG_DECODE_ALL, SCE_MPEG_DECODE_ALL, SCE_MPEG_DECODE_ALL );
		m_State = _State;
		if( m_pFile ) m_pStream->Start();
		if( m_bPlayAudio )
		{
			ms_AudioDec.Reset();
			m_bPlayAudio = false;
		}
	}
	else if (_State == eEND )
	{
		m_pStream->Stop();
		m_State = _State;

		if( m_bPlayAudio )
			ms_AudioDec.Reset();
	}

}

void CMpegStream::Demux()
{
	// demultiplex from ringbuffer
	uint8	*pRingTop;
	uint32	 RingSize;
	uint8	*pStart;
	uint8	*pEnd;
	uint32	 Size;

	m_pStream->GetAvailRingBufferData( pRingTop, RingSize, pStart, pEnd, Size );

	m_ProcBytes	= 1;

	if( Size )
	{
		m_bAnyCbCall		= false;
		uint32 ProcBytes	= sceMpegDemuxPssRing( &m_mp, pStart, Size, pRingTop, RingSize );
		m_ProcBytes 		= ProcBytes;
		
		m_pStream->MoveGetPtr( ProcBytes );
	}
}

void CMpegStream::SendAudio()
{
	if( m_bPlayAudio )
	{
		ms_AudioDec.SendToIop();
		if (ms_AudioDec.IsPreset())
			ms_AudioDec.Start();

		int32 Proc = (ms_AudioDec.NumBytesInBuffer() * 100) / AUDIO_BUFF_SIZE;
	}
}

void CMpegStream::DecodeNextFrame( uint8 *_pPixels, uint32 _nBufSize, fp32 _DeltaTime )
{
	if( m_State == eEND || m_State == ePAUSE )
	{
		return;
	}

	if( m_State == ePREBUFFER )
	{
		SetState( ePLAY );
	}
	
	RotateThreads();

	FlushCache( 0 );

	// fps correction code, because we are controlled by how fast the PCM stream is playing, finetune FPS so that the buffer does not over / underflow
	if( m_bPlayAudio )
	{
		uint32 BufferFullProc = ( ms_AudioDec.NumBytesInBuffer() * 100 ) / AUDIO_BUFF_SIZE;
		if		( BufferFullProc > 40 )	m_FramesPerSecond -= 0.001f;			// we are decoding too fast
		else if	( BufferFullProc < 10 ) m_FramesPerSecond += 0.001f;			// we are decoding too slow
	}

	uint32 nFramesToDecode = 1;

	if( m_bWHValid && m_State != eSEEKEND )
	{
		CMTime DeltaTime = CMTime::CreateFromSeconds( _DeltaTime );
		m_Time.Add( DeltaTime );
	
		fp32 PreferedTime = 1.f / fp32( m_FramesPerSecond );

		if( m_Time.Compare( CMTime::CreateFromSeconds((PreferedTime*0.5f))) < 0 )
		{
			// ahead
			nFramesToDecode = 0;
		}
		else if( m_Time.Compare( CMTime::CreateFromSeconds( (PreferedTime*400) ) ) > 0 )
		{
			// behind
			nFramesToDecode = 2;
			//m_Time -= PreferedTime * 2;
			m_Time.Reset();
		}
		else
		{
			// on time
			CMTime PrefTime = CMTime::CreateFromSeconds( -PreferedTime );
			m_Time.Add( PrefTime );	// on time
		}
	}

	bool bDeletePixels = false;
	if( !_pPixels )
	{
		_nBufSize = MAX_MBX * MAX_MBY * sizeof(sceIpuRGB32);
		_pPixels = (uint8*)MRTC_GetMemoryManager()->AllocAlign(_nBufSize, 64 );
		bDeletePixels = true;
		nFramesToDecode = 1;
	}


	//
	//	restart this dma
	//
	if( m_bDmaStarted )
		sceIpuRestartDMA( &m_DmaEnv );

	bool bContinue = true;

	while( bContinue )
	{
		RotateThreads();
		
		for( uint32 i = 0; i < nFramesToDecode; i++ )
		{
			m_bDmaStarted = true;
			
			sceIpuSETTH( 0x10, 0x5 );

			if( m_State == eSEEKEND )
			{
				if( m_bNFramesValid && ( m_mp.frameCount < ( int32(m_nFrames) - 32 ) ) )
				{
					sceMpegSetDecodeMode( &m_mp, SCE_MPEG_DECODE_ALL, 0, 0 );
				}
				else
				{
					sceMpegSetDecodeMode( &m_mp, SCE_MPEG_DECODE_ALL, SCE_MPEG_DECODE_ALL, SCE_MPEG_DECODE_ALL );
				}
			}

			sceMpegGetPicture( &m_mp, (sceIpuRGB32*)_pPixels, _nBufSize / sizeof(sceIpuRGB32) );
			m_CurrentFrame++;


			if (m_mp.frameCount == 0 && m_bWHValid == false ) // m_Width == 0 && m_Height == 0 ) 
			{
				m_Width		= m_mp.width;
				m_Height	= m_mp.height;
			}
			m_bWHValid	= true;
			
			if( sceMpegIsEnd( &m_mp ))
			{
				if( !m_bNFramesValid)
				{
					m_nFrames 		= m_CurrentFrame;
					m_bNFramesValid = true;
				}

				if (m_bAutoRewind)
				{
					SetState( ePREBUFFER );
				}
				else
				{
					SetState( eEND );
				}
				break;
			}
		}
		if( m_State != eSEEKEND )
			bContinue = false;
	}
	
	//
	//	stop this DMA
	//
	if( m_bDmaStarted )
		sceIpuStopDMA( &m_DmaEnv );
	
	if( bDeletePixels )
	{
		MRTC_GetMemoryManager()->Free( _pPixels );
	}
	else
	{
		SendAudio();
	}
}

int CMpegStream::CBVideo( sceMpeg *mp, sceMpegCbDataStr *cbstr, void *data )
{
	CMpegStream *pMpegStream = (CMpegStream*)data;
	pMpegStream->m_bAnyCbCall	= true;

	//
	//	copy from cbstr into demux ring
	//
	int32 availSpace;
	int32 spill;

	availSpace = pMpegStream->m_pDemuxBuffPresent - pMpegStream->m_pDemuxBuffPast;					// processed data
	if (availSpace < 0)
	{
		availSpace += pMpegStream->m_DemuxBuffSize;								// ring buffer
	}

	if ((availSpace == 0) && (pMpegStream->m_pDemuxBuffPresent == pMpegStream->m_pDemuxBuffFuture))
	{
		availSpace = pMpegStream->m_DemuxBuffSize;								// special case - buffer is empty, not full
	}

	if (availSpace < cbstr->len) 
	{
		//
		// buffer full, stop demultiplex
		//
		return( 0 );
	}

	spill = (pMpegStream->m_pDemuxBuffPast - ((uint8*)pMpegStream->m_pDemuxBuff)) + cbstr->len - pMpegStream->m_DemuxBuffSize;

	if (spill > 0) 
	{
		memcpy(UncachedAddr(pMpegStream->m_pDemuxBuffPast), cbstr->data, cbstr->len - spill);
		memcpy(UncachedAddr(pMpegStream->m_pDemuxBuff), cbstr->data + (cbstr->len - spill), spill);
		pMpegStream->m_pDemuxBuffPast = ((uint8*)pMpegStream->m_pDemuxBuff) + spill;
	}
	else 
	{
		memcpy(UncachedAddr(pMpegStream->m_pDemuxBuffPast), cbstr->data, cbstr->len);
		pMpegStream->m_pDemuxBuffPast += cbstr->len;
	}

	//
	//	video buffer not full, continue
	//
	return( 1 );
}

int CMpegStream::CBAudio( sceMpeg *mp, sceMpegCbDataStr *cbstr, void *data )
{
	CMpegStream *pMpegStream = (CMpegStream*)data;

	pMpegStream->m_bAnyCbCall	= true;
	pMpegStream->m_bHasAudio	= true;

	if( !pMpegStream->m_bPlayAudio )
		return( 1 );

	uint8 *pd0, *pd1;
	int32 d0, d1;
	int32 spill;

	ms_AudioDec.BeginPut(&pd0, &d0, &pd1, &d1);

	cbstr->len  -= 4;  // first four bytes are sub-stream ID
	cbstr->data += 4;

	if (cbstr->len > (d0 + d1)) 
	{
		// scePrintf( "audio buffer full\n" );
		return 0;
	}
	
	spill = cbstr->len - d0;

	if (spill > 0) 
	{
		memcpy(pd0, cbstr->data, cbstr->len - spill);
		memcpy(pd1, cbstr->data + (cbstr->len - spill), spill);
	}
	else 
	{
		memcpy(pd0, cbstr->data, cbstr->len);
	}

	ms_AudioDec.EndPut( cbstr->len);

	return( 1 );
}

int CMpegStream::CBNodata( sceMpeg *mp, sceMpegCbData *cbstr, void *data )
{
	CMpegStream *pMpegStream = (CMpegStream*)data;

	int dmaSize;
	int availData;

	pMpegStream->m_pDemuxBuffPresent = pMpegStream->m_pDemuxBuffFuture;

	availData = pMpegStream->m_pDemuxBuffPast - pMpegStream->m_pDemuxBuffFuture;    // data ready for processing
	if (availData < 0) availData += pMpegStream->m_DemuxBuffSize;					// ring buffer

	while (availData < DMACHUNKSIZE) 
	{
		//
		//	switch to reader thread
		//
		g_bDemux = true;
		RotateThreads();
		g_bDemux = false;
		pMpegStream->Demux();

		//
		//	This will happen when Audio Buffer overflows also,
		//
		if( pMpegStream->m_ProcBytes == 0 && pMpegStream->m_bAnyCbCall == false ) 
		{
			// scePrintf( "stream end: % 40s\n", pMpegStream->m_FileName.Str() );
			//
			//  More data cannot be obtained from the demultiplexer so this must
			//  be the end of the stream. Round up the data to ensure there's no
			//  remainder from the quadword-multiple transfer, otherwise the
			//  decoder won't receive the MPEG end sequence code.
			// 
			availData += 15;
			break;
		}

		availData = pMpegStream->m_pDemuxBuffPast - pMpegStream->m_pDemuxBuffFuture;	// data ready for processing
		if (availData < 0) availData += pMpegStream->m_DemuxBuffSize;					// ring buffer
	}

	dmaSize = availData & ~0xf;
	if (dmaSize > DMACHUNKSIZE) dmaSize = DMACHUNKSIZE;

	if ((pMpegStream->m_pDemuxBuffPresent + dmaSize) > (pMpegStream->m_pDemuxBuff + pMpegStream->m_DemuxBuffSize))
	dmaSize = pMpegStream->m_pDemuxBuff + pMpegStream->m_DemuxBuffSize - pMpegStream->m_pDemuxBuffPresent;  

	//  DMA must have stopped so these fields are safe to write to
//	while( (*D4_CHCR ) & (1<<8) ) ;	// but wait anyways!

	FlushCache( 0 );

	*D4_QWC  = dmaSize / 16;
	*D4_MADR = (int) pMpegStream->m_pDemuxBuffPresent;
	*D4_CHCR = 1 | 1 << 8;											// start, normal, from memory

	pMpegStream->m_pDemuxBuffFuture = pMpegStream->m_pDemuxBuffPresent + dmaSize;

	if( pMpegStream->m_pDemuxBuffFuture >= (pMpegStream->m_pDemuxBuff + pMpegStream->m_DemuxBuffSize))  // ring buffer
	pMpegStream->m_pDemuxBuffFuture -= pMpegStream->m_DemuxBuffSize;  
 	
	return( 1 );
}

int CMpegStream::CBBackground( sceMpeg *mp, sceMpegCbData *cbstr, void *data )
{
	return( 0 );
}

int CMpegStream::CBError( sceMpeg *mp, sceMpegCbData *cbstr, void *data )
{
	CMpegStream *pMpegStream = (CMpegStream*)data;

	sceMpegCbDataError *cberr= (sceMpegCbDataError *) cbstr;
	scePrintf("Mpeg stream internal error: '%s' in: % 25s\n", cberr->errMessage, pMpegStream->m_FileName.Str() );  

	return 1;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTC_MpegTexture
|__________________________________________________________________________________________________
\*************************************************************************************************/
CTC_MpegTexture::CTC_MpegTexture()
{
	m_pMpegStream	= NULL;
	m_bOnLastFrame	= false;
	m_bPaused		= false;
	m_bAutoRestart	= false;
	m_bHasAudio		= false;
	m_bReferenced	= false;
}

CTC_MpegTexture::~CTC_MpegTexture()
{
	Close();
}

void CTC_MpegTexture::Create( const CStr &_FileName )
{
	MSCOPESHORT(CTC_MpegTexture::Create);
	if (!CDiskUtil::FileExists(_FileName))
		Error("Create", "Not a valid file: " + _FileName);

	m_FileName = _FileName;
	m_TextureName = CStrF("*VIDEO_%s", _FileName.GetFilenameNoExt().Str());

	ReadInfoFile( _FileName );

	CStr msg = CStrF("Mpeg stream %s has dimensions %dx%d", m_FileName.Str(), m_Width, m_Height);

	if( m_bHasAudio )
		msg += " Audio";

	if( m_Width & 0xF ) msg += " Width Error.";
	if( m_Height & 0xF ) msg += " Height Error.";

	// ConOut( msg );
}

void CTC_MpegTexture::ReadInfoFile( const CStr &_FileName )
{
	MSCOPESHORT(CTC_MpegTexture::ReadInfoFile);
	class InfoFile
	{
	public:
		InfoFile( const uint8 *_pInfo )
		{
			m_pChar = _pInfo;

			m_bError = false;
			
			m_Width = GetValue( "width" );
			m_Height = GetValue( "height" );
			m_bAudio = GetValue( "audio" ) == 1 ? true : false;
			m_bAlpha = GetValue( "alpha" ) == 1 ? true : false;
			m_Frames = GetValue( "frames" );
			m_LastFramePtr = GetValue( "lastframeptr" );
		}
		
		~InfoFile()
		{
		}
		
		bool IsAlpha()
		{
			return( *m_pChar >= '0' && *m_pChar <= '9' );
		}
		
		bool IsChar()
		{
			return( (*m_pChar >= 'a' && *m_pChar <= 'z') || (*m_pChar >= 'A' && *m_pChar <= 'Z') );
		}
		
		bool IsEnd()
		{
			return( *m_pChar == 0 );
		}

		uint32 GetValue( const char *_pszTag )
		{
			while( !IsAlpha() && !IsChar() && !IsEnd() ) m_pChar++;
			if( IsEnd() || IsAlpha() )
			{
				scePrintf( "premature end\n" );
				m_bError = true;
				return( 0 );
			}
			for( uint32 i = 0; i < strlen( _pszTag ); i++ )
			{
				if( _pszTag[ i ] != *m_pChar )
				{
					scePrintf( "wrong tag\n" );
					m_bError = true;
					return( 0 );
				}
				m_pChar++;
			}
			while( !IsAlpha() && !IsChar() && !IsEnd() ) m_pChar++;
			if( IsEnd() || IsChar() )
			{
				scePrintf( "error after tag\n" );
				m_bError = true;
				return( 0 );
			}
			const uint8 *pNumStart = m_pChar;
			while( IsAlpha() ) m_pChar++;
			const uint8 *pNumEnd = m_pChar - 1;
			
			if( uint32(pNumEnd - pNumStart) > 32 )
			{
				scePrintf( "num to large!\n" );
				m_bError = true;
				return( 0 );
			}
			char tmp[ 32 ];
			uint32 i = 0;
			for( i = 0; i < (pNumEnd - pNumStart)+1; i++ )
			{
				tmp[i] = pNumStart[i];
			}
			tmp[i] = 0;
			
			uint32 Num = atoi( tmp );
			return( Num );
		}

		const uint8 *m_pChar;

		bool 	m_bError;

		uint32	m_Width;
		uint32	m_Height;
		bool	m_bAudio;
		bool	m_bAlpha;
		uint32	m_Frames;
		uint32	m_LastFramePtr; 
	};

	CCFile *pInfoFile = NULL;
	try
	{
		pInfoFile = MNew(CCFile);
		pInfoFile->OpenExt
		(
			_FileName.GetPath() + _FileName.GetFilenameNoExt() + ".txt",
			CFILE_READ | CFILE_BINARY, 
			NO_COMPRESSION, 
			NORMAL_COMPRESSION, 
			0.5, 
			4, 
			32 * 1024 // 128*1024
		);
	}
	catch (CCExceptionFile)
	{
		DecodeInfo();
		return;
	}
	catch (CCException)
	{
		DecodeInfo();
		return;
	}
	

	uint8 *pFileData = DNew(uint8) uint8[ pInfoFile->Length() ];
	pInfoFile->Read( pFileData, pInfoFile->Length() );
	delete pInfoFile;
	
	InfoFile Info( pFileData );
	delete [] pFileData;
	if( Info.m_bError )
	{
		scePrintf( "Error reading movie description file: %s\n", _FileName.Str() );
		DecodeInfo();
		return;
	}
	
	m_Width = Info.m_Width;
	m_Height = Info.m_Height;
	m_bHasAudio = Info.m_bAudio;
	m_nFrames = Info.m_Frames;
	m_bAlpha = Info.m_bAlpha;
}

void CTC_MpegTexture::DecodeInfo()
{
	MSCOPESHORT(CTC_MpegTexture::DecodeInfo);
	scePrintf( "Please wait, decoding info from %s\n", m_FileName.Str() );

	//
	// count frames of m2v file (M2V file must be in same directory as PSS file)
	//
	int frames = 0;
	
	scePrintf( "    counting frames" );
	CCFile *pFile;
	try
	{
		pFile = MNew(CCFile);
		pFile->OpenExt
		(
			m_FileName.GetPath() + m_FileName.GetFilenameNoExt() + ".m2v",
			CFILE_READ | CFILE_BINARY, 
			NO_COMPRESSION, 
			NORMAL_COMPRESSION, 
			0.5, 
			4, 
			32 * 1024
		);
		uint32 Length = pFile->Length();
		
		int ch;
		int state = 0;

		uint8 *pReadBuf = DNew(uint8) uint8[ 32 * 1024 ];
		uint8 *pPos = pReadBuf + 32 * 1024;
		
		bool wasInSlice = false;
		uint32 i = 0;
		uint32 a = 10;
		while ( i < Length ) 
		{
			if( pPos == pReadBuf + 32 * 1024 )
			{
				if( a == 10 )
				{
					scePrintf( "." );
					a = 0;
				}
				pFile->Read( pReadBuf, 32 * 1024 );
				pPos = pReadBuf;
			}
			ch = *pPos++;
			i++;
		
			if (ch == 0 && state == 0)
				state = 1;
			else if (ch == 0 && state == 1)
				state = 2;
			else if (ch == 1 && state == 2)
				state = 3;
			else if (state == 3) 
			{
				bool isInSlice = (ch >= 0x01 && ch <= 0xAF);
				if (wasInSlice && !isInSlice)
				{
					// inc frame counted
					++frames;
				}
				wasInSlice = isInSlice;
				state = 0;
			}
			else
				state = 0;
		}
		delete pReadBuf;
		delete pFile;
		
		scePrintf( "\n    Frames: %d\n", frames );
		
	}
	catch (CCExceptionFile)
	{
		scePrintf( "\nCould not find %s.m2v, framecount incorrect!\n", m_FileName.GetFilenameNoExt().Str() );
	}
	catch (CCException)
	{
		scePrintf( "\nCould not find %s.m2v, framecount incorrect!\n", m_FileName.GetFilenameNoExt().Str() );
	}
	


	//
	//	get width and height from the stream (Requires open, decode of one picture and then close)
	//
	scePrintf( "    Decode first frame\n" );
	
	m_pMpegStream = DNew(CMpegStream) CMpegStream( m_FileName, false );
/*
	while( !m_pMpegStream->GetStreamInfo().m_bWHValid )
	{
		m_pMpegStream->DecodeNextFrame( NULL );
	}
*/
	while( !m_pMpegStream->m_bWHValid )
		m_pMpegStream->DecodeNextFrame( NULL );
	
	
/*
	m_Width		= m_pMpegStream->GetStreamInfo().m_Width;
	m_Height	= m_pMpegStream->GetStreamInfo().m_Height;
	m_bHasAudio = m_pMpegStream->GetStreamInfo().m_bHasAudio;
	m_nFrames	= frames;
*/
	m_Width		= m_pMpegStream->m_Width;
	m_Height	= m_pMpegStream->m_Height;
	m_bHasAudio = m_pMpegStream->m_bHasAudio;
	m_nFrames	= frames;

	scePrintf( "    width     %d\n", m_Width );
	scePrintf( "    height    %d\n", m_Height );
	scePrintf( "    audio     %s\n", m_bHasAudio ? "true" : "false" );
	
	delete m_pMpegStream;
	m_pMpegStream = NULL;
	
	SaveInfoFile();
}

void CTC_MpegTexture::SaveInfoFile()
{
	MSCOPESHORT(CTC_MpegTexture::SaveInfoFile);
	CCFile *pInfoFile = NULL;
	try
	{
		pInfoFile = MNew(CCFile);
		pInfoFile->OpenExt
		(
			m_FileName.GetPath() + m_FileName.GetFilenameNoExt() + ".txt",
			CFILE_WRITE | CFILE_BINARY, 
			NO_COMPRESSION, 
			NORMAL_COMPRESSION, 
			0.5, 
			4, 
			32 * 1024
		);
	
		char *pTmp = DNew(char) char[ 32 * 1024 ];
		for( uint32 i = 0; i < 32 * 1024; i++ ) pTmp[ i ] = 0;
		sprintf
		( 
			pTmp, 
			"width                %d\n"
			"height               %d\n"
			"audio                %d\n"
			"alpha                %d\n"
			"frames               %d\n"
			"lastframeptr         %d\n",
			m_Width,
			m_Height,
			(m_bHasAudio == true ? 1 : 0 ),
			0,
			m_nFrames,
			0
		);
		
		pInfoFile->Write( pTmp, 32 * 1024 ); // strlen( pTmp ) + 1 );
		
		delete pTmp;
	
	}
	catch (CCExceptionFile)
	{
		return;
	}
	catch (CCException)
	{
		return;
	}

	delete pInfoFile;	

}
void CTC_MpegTexture::SaveState()
{
	if( IsOpen() )
	{
		m_bPaused		= m_pMpegStream->m_State == CMpegStream::ePAUSE;
		m_bOnLastFrame	= m_pMpegStream->m_State == CMpegStream::eEND;
	}
}

void CTC_MpegTexture::RestoreState()
{
	if( !IsOpen() )
		Open();

	if( !IsOpen() )
		return;

	if( m_bPaused )
	{
		m_pMpegStream->SetState( CMpegStream::ePAUSE );
	}
}

bool CTC_MpegTexture::IsOpen()
{
	return( m_pMpegStream != NULL );
}

void CTC_MpegTexture::Close()
{
	SaveState();
	delete m_pMpegStream;
	m_pMpegStream = NULL;
}

void CTC_MpegTexture::Open()
{
	MSCOPESHORT( CTC_MpegTexture::Open );
	if( IsOpen() )
		Close();

	m_pMpegStream = DNew(CMpegStream) CMpegStream( m_FileName, m_bHasAudio, m_Width, m_Height, m_nFrames );

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys)	m_Mainvolume = pSys->GetEnvironment()->GetValuef("SND_VOLUMESFX", 1.0f, 0);
	else		m_Mainvolume = 1.0;
	m_pMpegStream->SetVolume( m_Mainvolume );
	m_pMpegStream->SetAutoRewind( m_bAutoRestart );

	m_bOnLastFrame	= false;
	m_bPaused		= false;
	m_bAutoRestart	= false;
	//m_bHasAudio	= false;
	m_bReferenced	= false;

//	m_TimeLastVisible = CMTime::GetCPU();
	m_TimeLastVisible.Snapshot();
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_Video_Mpeg
|__________________________________________________________________________________________________
\*************************************************************************************************/
CTextureContainer_Video_Mpeg::CTextureContainer_Video_Mpeg()
{
	m_CloseTimeOut = 10;
}


CTextureContainer_Video_Mpeg::~CTextureContainer_Video_Mpeg()
{
	for(int i = 0; i < m_lspVideos.Len(); i++)
	{
		m_lspVideos[i]->Close();
		if (m_lspVideos[i]->m_TextureID)
			m_pTC->FreeID(m_lspVideos[i]->m_TextureID);
	}
}

void CTextureContainer_Video_Mpeg::Create(void* _pContext)
{
	MSCOPE( CTextureContainer_Video_Mpeg::Create, SYSTEMCORE_TEX_MPEG );
	m_pTC->EnableTextureClassRefresh(m_iTextureClass);

	if (_pContext)
		AddVideo(CStr((const char*)_pContext));

}


int CTextureContainer_Video_Mpeg::AddVideo(CStr _FileName)
{
	MSCOPE(CTextureContainer_Video_Mpeg::AddVideo, SYSTEMCORE_TEX_MPEG);

	spCTC_MpegTexture spVideo = MNew(CTC_MpegTexture);
	if (!spVideo) MemError("AddVideo");

	spVideo->Create(_FileName);

	spVideo->m_TextureID = m_pTC->AllocID(m_iTextureClass, m_lspVideos.Len(), spVideo->m_TextureName.Str());

	return m_lspVideos.Add(spVideo);
}


void CTextureContainer_Video_Mpeg::CreateFromDirectory(CStr _Path)
{
	Create(NULL);

	CDirectoryNode Dir;
	Dir.ReadDirectory(_Path + "*");

	for(int i = 0; i < Dir.GetFileCount(); i++)
	{
		CDir_FileRec* pRec  = Dir.GetFileRec(i);
		if (pRec->IsDirectory())
		{
			if (pRec->m_Name.Copy(0,1) != ".")
				CreateFromDirectory(_Path + pRec->m_Name + "\\");
		}
		else
		{
			if (pRec->m_Ext.CompareNoCase("PSS") == 0)
			{
				CStr FileName = _Path + pRec->m_Name;
				try 
				{ 
					AddVideo(FileName); 
				}
				catch(CCException) 
				{ 
					LogFile("§cf80WARNING: (CTextureContainer_Video_Mpeg::CreateFromDirectory) Failure adding video " + FileName); 
				}
			}
		}		
	}
}



void CTextureContainer_Video_Mpeg::ValidateLocalID(int _iLocal)
{
	if(!m_lspVideos.ValidPos(_iLocal))
		Error("ValidateLocalID", CStrF("Invalid local ID %d", _iLocal));
}


int CTextureContainer_Video_Mpeg::GetNumLocal()
{
	return m_lspVideos.Len();
}


int CTextureContainer_Video_Mpeg::GetLocal(const char* _pName)
{
	M_ASSERT(0, "Obsolete");
	return -1;
}


int CTextureContainer_Video_Mpeg::GetTextureID(int _iLocal)
{
	ValidateLocalID(_iLocal);
	return m_lspVideos[_iLocal]->m_TextureID;
}

int CTextureContainer_Video_Mpeg::GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps)
{
	MSCOPE(CTextureContainer_Video_Mpeg::GetTextureDesc, SYSTEMCORE_TEX_MPEG);

	// if no precached data, precache all
	if( g_PreCache.Empty() )
	{
		uint32 nLen = m_lspVideos.Len();
		for( uint32 i = 0; i < nLen; i++ )
		{
			CTC_MpegTexture* pV = m_lspVideos[ i ];
			if( pV )
			{ 
				g_PreCache.PreCache( pV->m_FileName );
			}	
		}
	}


	ValidateLocalID(_iLocal);

	CTC_MpegTexture* pV = m_lspVideos[_iLocal];

	if( !pV->m_bOnLastFrame )
	{	// only open if not on last frame
		if (pV->IsOpen() == false )
		{
			pV->Open();
		}
	}

	_pTargetImg->CreateVirtual
	(
		pV->m_Width,
		pV->m_Height,
		IMAGE_FORMAT_BGRA8, 
		IMAGE_MEM_IMAGE
	);
	
	pV->m_bReferenced = true;

	_Ret_nMipmaps = 1;
	return 0;
}

void CTextureContainer_Video_Mpeg::GetTextureProperties(int _iLocal, CTC_TextureProperties& _Properties)
{
	ValidateLocalID(_iLocal);
	CTC_TextureProperties Prop;
	Prop.m_Flags = 
		CTC_TEXTUREFLAGS_CLAMP_U | 
		CTC_TEXTUREFLAGS_CLAMP_V | 
		CTC_TEXTUREFLAGS_NOMIPMAP | 
		CTC_TEXTUREFLAGS_NOPICMIP | 
		CTC_TEXTUREFLAGS_HIGHQUALITY | 
		CTC_TEXTUREFLAGS_NOCOMPRESS	|
		CTC_TEXTUREFLAGS_PROCEDURAL;
	Prop.m_iPicMipGroup = 16;
	_Properties = Prop;
}


void CTextureContainer_Video_Mpeg::OnRefresh()
{
	MSCOPE(CTextureContainer_Video_Mpeg::OnRefresh, SYSTEMCORE_TEX_MPEG);
//	CMTime Time = CMTime::GetCPU();
	CMTime Time;
	Time.Snapshot();

	bool bAnyOpen = false;

	// check open movies and make them dirty
	for(int i = 0; i < m_lspVideos.Len(); i++)
	{
		CTC_MpegTexture* pV = m_lspVideos[i];

		if( pV->m_bReferenced == true ) 
			bAnyOpen = true;

		if( pV->m_bOnLastFrame == true )
		{
			if( pV->m_bReferenced == false )
			{
				m_pTC->MakeDirty(pV->m_TextureID);
				pV->m_bOnLastFrame = false;
			}
		}
		pV->m_bReferenced = false;
	
		
		if (pV->IsOpen())
		{
			if( (Time.GetCycles() - pV->m_TimeLastVisible.GetCycles()) > m_CloseTimeOut )
			{
				pV->Close();
			}
			m_pTC->MakeDirty(pV->m_TextureID);
		}	
	}
	
	// check if anu open movie has timed out
	
	// should we purge texture cache
	static uint32 nPurgeTimeOut = 0;
	if( !bAnyOpen )
	{
		if( ++nPurgeTimeOut == 10 )
		{
			g_PreCache.Purge();
			nPurgeTimeOut = 0;
		}
	}
	else 
	{
		nPurgeTimeOut = 0;
	}		
	
}

void CTextureContainer_Video_Mpeg::Pause(int _iLocal, bool _Paused)
{
	MSCOPE(CTextureContainer_Video_Mpeg::Pause, SYSTEMCORE_TEX_MPEG);
	ValidateLocalID(_iLocal);
	
	CTC_MpegTexture* pV = m_lspVideos[_iLocal];

	pV->m_bPaused = _Paused;
}

void CTextureContainer_Video_Mpeg::AutoRestart(int _iLocal, bool _EnableAutoRestart)
{
	ValidateLocalID(_iLocal);

	CTC_MpegTexture* pV = m_lspVideos[_iLocal];

	pV->m_bAutoRestart = _EnableAutoRestart;
}

void CTextureContainer_Video_Mpeg::Rewind(int _iLocal)
{
	MSCOPE(CTextureContainer_Video_Mpeg::Rewind, SYSTEMCORE_TEX_MPEG);
	ValidateLocalID(_iLocal);
	
	CTC_MpegTexture* pV = m_lspVideos[_iLocal];
		
	if (pV->IsOpen())
	{		
		if (pV->m_pMpegStream->m_CurrentFrame > 1)
		{
			pV->m_pMpegStream->SetState( CMpegStream::ePREBUFFER );
			m_pTC->MakeDirty(pV->m_TextureID);
		}

	}
}

bool CTextureContainer_Video_Mpeg::IsOnLastFrame(int _iLocal)
{
	ValidateLocalID(_iLocal);
	
	CTC_MpegTexture* pV = m_lspVideos[_iLocal];
	
	return pV->m_bOnLastFrame;
}

int CTextureContainer_Video_Mpeg::GetFrame(int _iLocal)
{
	ValidateLocalID(_iLocal);
	CTC_MpegTexture* pV = m_lspVideos[_iLocal];

	if (pV->IsOpen() )
		return( pV->m_pMpegStream->m_CurrentFrame );
	else
		return( 0 );
}


int CTextureContainer_Video_Mpeg::GetNumFrames(int _iLocal)
{
	ValidateLocalID(_iLocal);
	
	CTC_MpegTexture* pV = m_lspVideos[_iLocal];

	return( pV->m_nFrames );
}


bool CTextureContainer_Video_Mpeg::MoveToLastFrame(int _iLocal)
{
	MSCOPE(CTextureContainer_Video_Mpeg::MoveToLastFrame, SYSTEMCORE_TEX_MPEG);
	ValidateLocalID(_iLocal);
	
	CTC_MpegTexture* pV = m_lspVideos[_iLocal];

	if (pV->IsOpen())
	{
		pV->m_pMpegStream->SetState( CMpegStream::eSEEKEND );
		m_pTC->MakeDirty(pV->m_TextureID);
	}
	else
	{
		pV->Open();
		pV->m_pMpegStream->SetState( CMpegStream::eSEEKEND );
		m_pTC->MakeDirty(pV->m_TextureID);
	}

	return 0;
}

void CTextureContainer_Video_Mpeg::CloseVideo(int _iLocal)
{
	ValidateLocalID(_iLocal);
	CTC_MpegTexture *pV = m_lspVideos[_iLocal];
	pV->Close();
	pV->m_bOnLastFrame = true;
}


void CTextureContainer_Video_Mpeg::SetVolume(int _iLocal, fp32 fpVol)
{
	ValidateLocalID(_iLocal);
	CTC_MpegTexture* pV = m_lspVideos[_iLocal];
	
	if (pV->IsOpen())
	{
		pV->m_pMpegStream->SetVolume( fpVol * pV->m_Mainvolume );
	}
}


CStr CTextureContainer_Video_Mpeg::GetName(int _iLocal)
{ 
	ValidateLocalID(_iLocal);
	return m_lspVideos[_iLocal]->m_TextureName;
};


void CTextureContainer_Video_Mpeg::BuildInto
(
	int			_iLocal, 
	CImage**	_ppImg, 
	int			_nMipmaps, 
	int			_ConvertType, 
	int			_iStartMip
)
{
	try
	{
		MSCOPE(CTextureContainer_Video_Mpeg::BuildInto, SYSTEMCORE_TEX_MPEG);
		ValidateLocalID(_iLocal);

		if (_nMipmaps != 1)
			Error("BuildInto", "nMipmaps != 1");


		CTC_MpegTexture* pV = m_lspVideos[_iLocal];


		//
		//	decoder one frame
		//
		if( !pV->m_pMpegStream )
		{
		}
		else
		{		
			uint8 *pPixels = (uint8*)_ppImg[0]->Lock();

			if( !pPixels )
			{
				if( _ppImg[0]->GetWidth() != pV->m_Width || _ppImg[0]->GetHeight() != pV->m_Width )
				{
					M_ASSERT
					( 
						0, 
						"Mpeg decoder Fatal Error: can not decode to a texture smaller then source stream."
						"Probable cause: Texture size has been reduced to fit in Vram!"
						"Check BuildTexture2D"
						"\n" 
					);
				}
			}

			uint32 nBufSize = _ppImg[0]->GetModulo() * _ppImg[0]->GetHeight();
//			CMTime Time = CMTime::GetCPU();
			CMTime Time;
			Time.Snapshot();
			fp32 DeltaTime = Time.GetCycles() - pV->m_TimeLastVisible.GetCycles();
			pV->m_TimeLastVisible = Time;
			pV->m_pMpegStream->DecodeNextFrame( pPixels, nBufSize, DeltaTime );
			_ppImg[0]->Unlock();
			pV->m_bOnLastFrame = pV->m_pMpegStream->m_State == CMpegStream::eEND;
		}

		
		if( pV->m_bOnLastFrame )
		{	// end reached, close
			pV->Close();
		}
	}
	catch (CCExceptionFile)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
	}
#ifdef M_SUPPORTSTATUSCORRUPT
	catch (CCException)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
	}
#endif
}


int CTextureContainer_Video_Mpeg::GetWidth(int _iLocal)
{
	CTC_MpegTexture* pV = m_lspVideos[_iLocal];

	int Width = pV->m_Width;

	return (int( float(Width) / float( GetGEPow2(Width) ) * float(Width) ));
}

int CTextureContainer_Video_Mpeg::GetHeight(int _iLocal)
{
	CTC_MpegTexture* pV = m_lspVideos[_iLocal];

	int Height = pV->m_Height;

	return (int( float(Height) / float( GetGEPow2(Height) ) * float(Height) ));
}


fp32 CTextureContainer_Video_Mpeg::GetTime(int _iLocal)
{
	ValidateLocalID(_iLocal);
	CTC_MpegTexture *pV	= m_lspVideos[_iLocal];
	CMpegStream *pS		= pV->m_pMpegStream;

	fp32 ret;

	if( pS && pS->m_bNFramesValid )
		ret = ( fp32(pS->m_CurrentFrame) / pS->m_FramesPerSecond );
	else
		ret = 0.0f;

	return( ret );
}



MRTC_IMPLEMENT_DYNAMIC(CTextureContainer_Video_Mpeg, CTextureContainer_Video);

#endif // defined(PLATFORM_PS2)
