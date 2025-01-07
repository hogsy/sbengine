
#include "PCH.h"

#include "../MSystem.h"

#if defined(PLATFORM_DOLPHIN)
#include "MTextureContainerBink.h"

#include "../../../SDK/BinkSDK/rad.h"
#include "../../../SDK/BinkSDK/bink.h"
//#include "../../../SDK/BinkSDK/radmath.h"

#if defined(__RADNT__) || defined(__RADXBOX__)

#pragma code_seg("BINK")
#pragma data_seg("BINKDATA")

#ifdef PLATFORM_XBOX
#pragma comment(lib, "..\\..\\..\\SDK\\BinkSDK_Xbox\\binkw32.lib") 
#else
#pragma comment(lib, "..\\..\\..\\SDK\\BinkSDK\\binkw32.lib") 
#endif

#ifdef __RADXBOX__

	#include <xtl.h>

#else
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#endif

#endif

#ifdef PLATFORM_DOLPHIN
# include "../../../MCC/MFile_Dolphin.h"
# include "../../MRndrDolphin/DisplayContext.h"
# include "../../MRndrDolphin/RenderContext.h"
#endif


#if defined(PLATFORM_DOLPHIN)
# define BINK_MAXGAIN 32000
#elif defined(PLATFORM_XBOX)
# define BINK_MAXGAIN 6000
#else
# define BINK_MAXGAIN 17500
#endif


static int32 s_nFrameCounter = 0;



// #define SINGLE_FULLSCREEN_BINK

// If you intend to replace the BinkFile functions, then you should probably
//   replace the "rfile.h" below with your own defines and leave everything
//   else alone.  You should only try to replace all of the BinkFile functions
//   completely, if you are going to require more sophisticated streaming 
//   (like off TCP/IP directly, for example).

//#include "rfile.H"


// defined variables that are accessed from the Bink IO structure
typedef struct BINKFILE
{
	CCFile *File;
	U32 FileIOPos;
	U32 FileBufPos;
	U8* BufPos;
	U32 BufEmpty;
	S32 InIdle;
	U8* Buffer;
	U8* BufEnd;
	U8* BufBack;
	U32 DontClose;
	U32 StartFile;
	U32 FileSize;
	U32 Simulate;
	S32 AdjustRate;
} BINKFILE;

#define BF ( ( BINKFILE PTR4 volatile * )bio->iodata )

#ifdef PLATFORM_DOLPHIN
	#define BASEREADSIZE 4096
	static void intelendian(void* dest, U32 size)
	{
		U32 s = (size+3) / 4;
		U32* d = (U32*)dest;
		while (s--)
			*d++ = __lwbrx(d, 0);
	}

#elif defined(__RADMAC__)
	#define BASEREADSIZE ( 32 * 1024 )

	static void intelendian( void* dest, U32 size )
	{
		MAUTOSTRIP(intelendian, MAUTOSTRIP_VOID);
		U32 s = ( size + 3 ) / 4;
		U32 *d = dest;
		while ( s-- )
		{
	#ifdef __RADPPC__
			U32 v = __lwbrx( d, 0 );
			*d++ = v;
	#else
			U32 v = *d;
			*d++ = ( v >> 24 ) | ( ( v >> 8 ) & 0xff00 ) | ( ( v << 8 ) & 0xff0000 ) | ( v << 24 );
	#endif
		}
	}

#else
	#define BASEREADSIZE 4096
	#define intelendian( dest, size )

#endif


#define SUSPEND_CB( bio ) { if ( bio->suspend_callback ) bio->suspend_callback( bio );}
#define RESUME_CB( bio ) { if ( bio->resume_callback ) bio->resume_callback( bio );}
#define TRY_SUSPEND_CB( bio ) ( ( bio->try_suspend_callback == 0 ) ? 0 : bio->try_suspend_callback( bio ) )
#define IDLE_ON_CB( bio ) { if ( bio->idle_on_callback ) bio->idle_on_callback( bio );}

//reads from the header
static U32 RADLINK MSystem_BinkFileReadHeader( BINKIO PTR4* bio, S32 offset, void* dest, U32 size )
{
	MAUTOSTRIP(MSystem_BinkFileReadHeader, 0);
	U32 amt, temp;
	
	SUSPEND_CB( bio );
	
	if ( offset != -1 )
	{
		if ( BF->FileIOPos != (U32) offset )
		{
			BF->File->Seek(offset + BF->StartFile);
			BF->FileIOPos = offset;
		}
	}
	amt = -BF->File->Pos();
	try
	{
		BF->File->Read(dest, size);
	}
	catch (CCExceptionFile)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
		if (!MRTC_GetObjectManager()->InMainThread())
		{
#ifdef PLATFORM_XBOX
			ExitThread(2);
#endif
		}
		else
			throw;
	}
#ifdef M_SUPPORTSTATUSCORRUPT
	catch (CCException)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
		if (!MRTC_GetObjectManager()->InMainThread())
		{
#ifdef PLATFORM_XBOX
			ExitThread(2);
#endif
		}
		else
			throw;
	}
#endif

	amt += BF->File->Pos();
	
	BF->FileIOPos += amt;
	BF->FileBufPos = BF->FileIOPos;
	
	temp = ( BF->FileSize - BF->FileBufPos );
	bio->CurBufSize = ( temp < bio->BufSize ) ? temp : bio->BufSize;
	
	RESUME_CB( bio );
	
	intelendian( dest, amt );
	
	return( amt );
}


static void dosimulate( BINKIO PTR4* bio, U32 amt, U32 timer )
{
	MAUTOSTRIP(dosimulate, MAUTOSTRIP_VOID);
	S32 rt; 
	S64 Temp = amt*1000;
	Temp /= BF->Simulate;
	S32 wait = Temp / 1000;
	
	rt = RADTimerRead();
	
	BF->AdjustRate += ( (S32) wait ) - ( (S32) ( rt - timer ) );
	while ( BF->AdjustRate > 0 )
	{
		do
		{
#ifdef __RADNT__
			Sleep( 0 );
#endif
			wait = RADTimerRead();
		} while ( ( wait - rt ) < BF->AdjustRate );
		BF->AdjustRate -= ( wait - rt );
		rt = wait;
	}
}

//reads a frame (BinkIdle might be running from another thread, so protect against it)
static U32  RADLINK MSystem_BinkFileReadFrame( BINKIO PTR4* bio, 
											  U32 framenum, 
											  S32 offset,
											  void* dest,
											  U32 size )
{
	S32 funcstart = 0;
	U32 amt, tamt = 0;
	U32 timer, timer2;
	U32 cpy;
	void* odest = dest;
	
	if ( bio->ReadError )
		return( 0 );
	
	timer = RADTimerRead();
	
	if ( offset != -1 )
	{
		if ( BF->FileBufPos != (U32) offset )
		{
			
			funcstart = 1;
			SUSPEND_CB( bio );
			
			if ( ( (U32) offset > BF->FileBufPos ) && ( (U32) offset <= BF->FileIOPos ) )
			{
				
				amt = offset-BF->FileBufPos;
				
				BF->FileBufPos = offset;
				BF->BufEmpty += amt;
				bio->CurBufUsed -= amt;
				BF->BufPos += amt;
				if ( BF->BufPos > BF->BufEnd )
					BF->BufPos -= bio->BufSize;
				
			}
			else
			{
				
				BF->File->Seek(offset + BF->StartFile);
				BF->FileIOPos = offset;
				BF->FileBufPos = offset;
				
				BF->BufEmpty = bio->BufSize;
				bio->CurBufUsed = 0;
				BF->BufPos = BF->Buffer;
				BF->BufBack = BF->Buffer;
			}
		}
		
	}
	
	// copy from background buffer
getrest:
	
	cpy = bio->CurBufUsed;
	
	if ( cpy )
	{
		U32 front;
		
		if ( cpy > size )
			cpy = size;
		
		size -= cpy;
		tamt += cpy;
		BF->FileBufPos += cpy;
		
		front = BF->BufEnd - BF->BufPos;
		if ( front <= cpy )
		{
			radmemcpy( dest, BF->BufPos, front );
			dest = ( (U8*) dest ) + front;
			BF->BufPos = BF->Buffer;
			cpy -= front;	  
			LockedAddFunc( (void*) &bio->CurBufUsed, -(S32)front );
			LockedAddFunc( (void*) &BF->BufEmpty, front );
			if ( cpy == 0 )
				goto skipwrap;
		}
		radmemcpy( dest, BF->BufPos, cpy );
		dest = ( (U8*) dest ) + cpy;
		BF->BufPos += cpy;
		LockedAddFunc( (void*) &bio->CurBufUsed, -(S32)cpy );
		LockedAddFunc( (void*) &BF->BufEmpty, cpy );
	}
	
skipwrap:
	
	if ( size )
	{
		if ( funcstart == 0 )
		{
			funcstart = 1;
			SUSPEND_CB( bio );
			goto getrest;
		}
		
		timer2 = RADTimerRead();

		amt = -BF->File->Pos();
		try
		{
			BF->File->Read(dest, size);
		}
		catch (CCExceptionFile)
		{
			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
			if (!MRTC_GetObjectManager()->InMainThread())
			{
	#ifdef PLATFORM_XBOX
				ExitThread(2);
	#endif
			}
			else
				throw;
		}
#ifdef M_SUPPORTSTATUSCORRUPT
		catch (CCException)
		{
			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
			if (!MRTC_GetObjectManager()->InMainThread())
			{
	#ifdef PLATFORM_XBOX
				ExitThread(2);
	#endif
			}
			else
				throw;
		}
#endif
		amt += BF->File->Pos();

		if ( amt < size )
			bio->ReadError = 1;
		
		BF->FileIOPos += amt;
		BF->FileBufPos += amt;
		bio->BytesRead += amt;
		tamt += amt;
		
		if ( BF->Simulate )
			dosimulate( bio, amt, timer2 );
		
		amt = RADTimerRead();
		bio->TotalTime += ( amt - timer2 );
		bio->ForegroundTime += ( amt - timer );
	}
	else
	{
		bio->ForegroundTime += ( RADTimerRead() - timer );
	}
	
	amt = ( BF->FileSize - BF->FileBufPos );
	bio->CurBufSize = ( amt < bio->BufSize ) ? amt : bio->BufSize;
	if ( ( bio->CurBufUsed + BASEREADSIZE ) > bio->CurBufSize )
		bio->CurBufSize = bio->CurBufUsed;
	
	if ( funcstart )
		RESUME_CB( bio );
	
	intelendian( odest, tamt );
	
	return( tamt );
}


//returns the size of the recommended buffer
static U32 RADLINK MSystem_BinkFileGetBufferSize( BINKIO PTR4* bio, U32 size )
{
	MAUTOSTRIP(MSystem_BinkFileGetBufferSize, 0);
	size /= 3;
//	size = 8192;
	size = ( ( size + ( BASEREADSIZE - 1 ) ) / BASEREADSIZE ) * BASEREADSIZE;
	return( size );
}


//sets the address and size of the background buffer
static void RADLINK MSystem_BinkFileSetInfo( BINKIO PTR4* bio, 
											void PTR4* buf,
											U32 size,
											U32 filesize,
											U32 simulate )
{
	SUSPEND_CB( bio );
	
	size = ( size / BASEREADSIZE ) * BASEREADSIZE;
	BF->Buffer = (U8*) buf;
	BF->BufPos = (U8*) buf;
	BF->BufBack = (U8*) buf;
	BF->BufEnd =( (U8*) buf ) + size;
	bio->BufSize = size;
	BF->BufEmpty = size;
	bio->CurBufUsed = 0;
	BF->FileSize = filesize;
	BF->Simulate = simulate;
	
	RESUME_CB( bio );
}


//close the io structure
static void RADLINK MSystem_BinkFileClose( BINKIO PTR4* bio )
{
	MAUTOSTRIP(MSystem_BinkFileClose, MAUTOSTRIP_VOID);
	SUSPEND_CB( bio );
	
	if ( BF->DontClose == 0 )
	{
		if (BF->File)
			delete BF->File;
	}
	
	RESUME_CB( bio );
}


//tells the io system that idle time is occurring (can be called from another thread)
static U32 RADLINK MSystem_BinkFileIdle(BINKIO PTR4* bio)
{
	MAUTOSTRIP(MSystem_BinkFileIdle, 0);
	U32 amt = 0;
	
	U32 temp, timer;
	S32 Working = bio->Working;
	
	if ( bio->ReadError )
		return( 0 );
	
	if ( TRY_SUSPEND_CB( bio ) )
	{
		temp = ( BF->FileSize - BF->FileIOPos );
		
		if ( ( BF->BufEmpty >= BASEREADSIZE ) && ( temp >= BASEREADSIZE ) )
		{
			timer = RADTimerRead();
			
			bio->DoingARead = 1;
			amt = -BF->File->Pos();
			try
			{
				BF->File->Read((void*) BF->BufBack, BASEREADSIZE);
			}
			catch (CCExceptionFile)
			{
				CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
				if (!MRTC_GetObjectManager()->InMainThread())
				{
		#ifdef PLATFORM_XBOX
					ExitThread(2);
		#endif
				}
				else
					throw;
			}
#ifdef M_SUPPORTSTATUSCORRUPT
			catch (CCException)
			{
				CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
				if (!MRTC_GetObjectManager()->InMainThread())
				{
		#ifdef PLATFORM_XBOX
					ExitThread(2);
		#endif
				}
				else
					throw;
			}
#endif
			amt += BF->File->Pos();
			bio->DoingARead = 0;
			if ( amt < BASEREADSIZE )
				bio->ReadError = 1;
			
			bio->BytesRead += amt;
			BF->FileIOPos += amt;
			BF->BufBack += amt;
			if ( BF->BufBack >= BF->BufEnd )
				BF->BufBack = BF->Buffer;
			
			LockedAddFunc( (void*) &BF->BufEmpty, -(S32) amt );
			LockedAddFunc( (void*)&bio->CurBufUsed, amt );
			
			if ( bio->CurBufUsed > bio->BufHighUsed )
				bio->BufHighUsed = bio->CurBufUsed;
			
			if ( BF->Simulate )
				dosimulate( bio, amt, timer );
			
			timer = RADTimerRead() - timer;
			bio->TotalTime += timer;
			if ( ( Working ) || ( bio->Working ) )
				bio->ThreadTime += timer;
			else
				bio->IdleTime += timer;
		}
		else
		{
			// if we can't fill anymore, then set the max size to the current size
			bio->CurBufSize = bio->CurBufUsed;
		}
		
		RESUME_CB( bio );
	}
	else
	{
		// if we're in idle in the background thread, do a sleep to give it more time
		IDLE_ON_CB( bio ); // let the callback run
		amt = (U32)-1;
	}
	
	return( amt );
}


//opens a normal filename into an io structure
RADDEFFUNC S32 RADLINK MSystem_BinkFileOpen( BINKIO PTR4* bio, const char PTR4* name, U32 flags );
RADDEFFUNC S32 RADLINK MSystem_BinkFileOpen( BINKIO PTR4* bio, const char PTR4* name, U32 flags )
{
	MAUTOSTRIP(MSystem_BinkFileOpen, 0);
	radmemset( bio, 0, sizeof( BINKIO ) );
	
	if ( flags & BINKFILEHANDLE )
	{
		M_ASSERT(0, "We dont support this");
		return 0;
	}
	else
	{
		try 
		{
			BF->File = MNew( CCFile );
		#ifdef PLATFORM_DOLPHIN
			BF->File->Open(name, CFILE_READ | CFILE_BINARY); // that cache in OpenExt looked a bit too large..
		#else
			BF->File->OpenExt(name, CFILE_READ | CFILE_BINARY, NO_COMPRESSION, NORMAL_COMPRESSION, 0.5, 4, 128*1024);
		#endif
		}
		catch (CCExceptionFile)
		{
			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
			return 0;
		}
#ifdef M_SUPPORTSTATUSCORRUPT
		catch (CCException)
		{
			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
			return 0;
		}
#endif
	}
	
	bio->ReadHeader = MSystem_BinkFileReadHeader;
	bio->ReadFrame = MSystem_BinkFileReadFrame;
	bio->GetBufferSize = MSystem_BinkFileGetBufferSize;
	bio->SetInfo = MSystem_BinkFileSetInfo;
	bio->Idle = MSystem_BinkFileIdle;
	bio->Close = MSystem_BinkFileClose;
	
	return( 1 );
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

class COSInfoClass
{
public:
	M_INLINE void Init(){}
	M_INLINE bool IsNT(){return true;}
};

static COSInfoClass g_OsInfo;

#endif
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTC_BinkTexture
|__________________________________________________________________________________________________
\*************************************************************************************************/
CTC_BinkTexture::CTC_BinkTexture()
{
	MAUTOSTRIP(CTC_BinkTexture_ctor, MAUTOSTRIP_VOID);
	m_hBink = 0;
	m_Width = 0;
	m_Height = 0;
	m_RealWidth = 0;
	m_RealHeight = 0;
	m_TextureID = 0;
	m_bAutoRestart = true;
	m_bPaused = false;
	m_bDoneFrame = true;
	m_bOnLastFrame = false;
	m_Frames = 0;
	m_bValid = false;
	m_volume = 1;
	m_Mainvolume = 1;
	m_bGlobalPaused = false;
}

CTC_BinkTexture::~CTC_BinkTexture()
{
	MAUTOSTRIP(CTC_BinkTexture_dtor, MAUTOSTRIP_VOID);
	Close();
}

void CTC_BinkTexture::Create(CStr _FileName)
{
	MAUTOSTRIP(CTC_BinkTexture_Create, MAUTOSTRIP_VOID);

	m_FileName = _FileName;
	if (!CDiskUtil::FileExists(m_FileName))
		Error("Create", "Not a valid file: " + m_FileName);

	m_TextureName = CStrF("*VIDEO_%s", _FileName.GetFilenameNoExt().Str());
	m_Width = 1;
	m_Height = 1;
	m_RealWidth = 1;
	m_RealHeight = 1;
}

bool CTC_BinkTexture::IsOpen()
{
	MAUTOSTRIP(CTC_BinkTexture_IsOpen, false);
	return m_hBink != 0;
}


void CTC_BinkTexture::Close()
{
	MAUTOSTRIP(CTC_BinkTexture_Close, MAUTOSTRIP_VOID);
	MSCOPE(CTC_BinkTexture::Close, SYSTEMCORE_TEX_BINK);

	SaveState();
	if (m_hBink)
	{
//		BinkSetSoundOnOff(m_hBink, false);
//		BinkService(m_hBink);
		BinkClose(m_hBink);
		m_hBink = 0;
	}
}

void CTC_BinkTexture::Open()
{
	MAUTOSTRIP(CTC_BinkTexture_Open, MAUTOSTRIP_VOID);
	MSCOPE(CTC_BinkTexture::Open, SYSTEMCORE_TEX_BINK);

	if (IsOpen())
		Close();

	m_bDoneFrame = false;

	uint32 SoundTrack = 0;

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	MACRO_GetRegisterObject(CSoundContext, pSound, "SYSTEM.SOUND");
	if (pSys)
		SoundTrack = pSys->GetEnvironment()->GetValuei("BINK_SOUNDTRACK", 1);

	BinkSetSoundTrack(1, &SoundTrack);

#ifdef PLATFORM_DOLPHIN
	if (!pSound)
		BinkSetSoundTrack(0, NULL);

	BinkSetIO(MSystem_BinkFileOpen);
	m_hBink = BinkOpen(m_FileName.Str(), BINKALPHA | BINKSNDTRACK | BINKIOPROCESSOR);
	if (m_hBink && pSound && !m_hBink->soundon && m_hBink->NumTracks)
	{
		// If we don't get any sound try to open with default sound
		BinkClose(m_hBink);
		BinkSetIO(MSystem_BinkFileOpen);
		m_hBink = BinkOpen(m_FileName.Str(), BINKALPHA | BINKIOPROCESSOR);
	}
#else
	BinkSetIO(MSystem_BinkFileOpen);
	uint32 Flags=0;
/*	if (g_OsInfo.IsNT())
	{
		Flags |= BINKIOPROCESSOR;
	}
	else
	{
	}*/
	m_hBink = BinkOpen(m_FileName.Str(), /*BINKFILEHANDLE*/ BINKALPHA | Flags | BINKSNDTRACK); // 
	if (m_hBink && !m_hBink->soundon && m_hBink->NumTracks)
	{
		// If we don't get any sound try to open with default sound
		BinkClose(m_hBink);
		m_hBink = BinkOpen(m_FileName.Str(), /*BINKFILEHANDLE*/ BINKALPHA | Flags); // 
	}
#endif	

	if (!m_hBink)
		Error("Create", CStrF("BinkOpen failed on %s", m_FileName.Str()));

	m_Mainvolume = 1.0;
	if (pSys)
		m_Mainvolume = pSys->GetOptions()->GetValuef("SND_VOLUMESFX", 1.0f, 0);

	int Volume = Max(0, int(BINK_MAXGAIN * m_volume * m_Mainvolume));
	if (pSound)
		BinkSetVolume(m_hBink, 0, Volume);

	m_Frames = m_hBink->Frames;
	if (m_Frames == 1)
		m_bAutoRestart = false;	//AR-ADD

	BINKSUMMARY Summary;
	FillChar(&Summary, sizeof(Summary), 0);
	BinkGetSummary(m_hBink, &Summary);
	
	m_RealWidth = Summary.Width;
	m_RealHeight = Summary.Height;
	
#if defined(PLATFORM_XBOX) || defined(PLATFORM_DOLPHIN)
	m_Width = Summary.Width;
	m_Height = Summary.Height;
#else
	m_Width = GetGEPow2(Summary.Width);
	m_Height = GetGEPow2(Summary.Height);
#endif
	m_TimeLastVisible = s_nFrameCounter;
	
	m_bValid = true;
}

void CTC_BinkTexture::UpdateVolume()
{
	if (!m_hBink)
		return;

	int Volume;
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys)
	{
		if (pSys->GetEnvironment()->GetValuei("Snd_Use3D", 1))
			Volume = 19600 * m_volume * m_Mainvolume;
		else
			Volume = 32767 * m_volume * m_Mainvolume;
	}
	else
		Volume = 19600 * m_volume * m_Mainvolume;

	if (Volume < 33)
	{
		Volume = 33;
	}

	BinkSetVolume(m_hBink, 0, Volume);
}

void CTC_BinkTexture::SaveState()
{
	MAUTOSTRIP(CTC_BinkTexture_SaveState, MAUTOSTRIP_VOID);
	if (m_hBink)
	{
		m_bPaused = m_hBink->Paused != 0;
		m_LastFrame = m_hBink->FrameNum;
		m_bOnLastFrame = m_hBink->FrameNum == m_hBink->Frames;
	}
}

void CTC_BinkTexture::MakeValid()
{
	MAUTOSTRIP(CTC_BinkTexture_MakeValid, MAUTOSTRIP_VOID);
	if (!IsOpen())
		Open();
}

void CTC_BinkTexture::RestoreState()
{
	MAUTOSTRIP(CTC_BinkTexture_RestoreState, MAUTOSTRIP_VOID);
	if (!IsOpen())
		Open();
	
	if (!IsOpen())
		return;

	BinkGoto(m_hBink, m_LastFrame, 0);
	BinkPause(m_hBink, m_bPaused);
	m_bDoneFrame = false;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_Bink
|__________________________________________________________________________________________________
\*************************************************************************************************/

#if defined(PLATFORM_XBOX) || defined(PLATFORM_DOLPHIN)
void PTR4* RADLINK MallocForBink(u32 bytes);
void PTR4* RADLINK MallocForBink(u32 bytes)
{
	MAUTOSTRIP(MallocForBink, NULL);
	void * FillBytes = DNew(uint8) uint8[bytes];
//	memset(FillBytes, 0, bytes);
	return FillBytes;

}
void RADLINK FreeForBink(void PTR4* ptr);
void RADLINK FreeForBink(void PTR4* ptr)
{
	MAUTOSTRIP(FreeForBink, MAUTOSTRIP_VOID);
	delete[] (uint8*)ptr;
}
#endif

#ifdef PLATFORM_DOLPHIN
static CBlockManager::CBlock* s_apARamBlocks[32];
static int s_nARamBlocks = 0;

void PTR4* RADLINK ARAM_MallocForBink(u32 bytes);
void PTR4* RADLINK ARAM_MallocForBink(u32 bytes)
{
	M_ASSERT(s_nARamBlocks < 32, "Too many ARAM blocks for Bink!");
	CBlockManager::CBlock* pBlock = g_AramManagerSound.Alloc(bytes); //TODO: Check if Misc or Sound should be used..
	if (pBlock)
	{
		s_apARamBlocks[s_nARamBlocks++] = pBlock;
		return pBlock->m_pMemory;
	}
	return NULL;
}

void RADLINK ARAM_FreeForBink(void PTR4* ptr);
void RADLINK ARAM_FreeForBink(void PTR4* ptr)
{
	M_ASSERT(s_nARamBlocks > 0, "No known ARAM blocks for bink left!");
	if (ptr)
	{
		for (int i=0; i<s_nARamBlocks; i++)
		{
			if (s_apARamBlocks[i]->m_pMemory == ptr)
			{
				g_AramManagerSound.Free(s_apARamBlocks[i]);
				s_apARamBlocks[i] = s_apARamBlocks[--s_nARamBlocks];
				return;
			}
		}
		M_ASSERT(false, "ARAM block not found in list of bink blocks!");
	}
}
#endif



CTextureContainer_Video_Bink::CTextureContainer_Video_Bink()
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_ctor, MAUTOSTRIP_VOID);
	m_CloseTimeOut = 0.4f;

#if defined(PLATFORM_XBOX) || defined(PLATFORM_DOLPHIN)
	RADSetMemory(MallocForBink, FreeForBink);
#endif

#if defined(PLATFORM_DOLPHIN)
	MACRO_GetRegisterObject(CSoundContext, pSound, "SYSTEM.SOUND");
	if (pSound)
	{
		static RADARAMCALLBACKS	cb;
		cb.aram_malloc = ARAM_MallocForBink;
		cb.aram_free = ARAM_FreeForBink;		
		BinkSoundUseAX(&cb);
	}
#endif
}

CTextureContainer_Video_Bink::~CTextureContainer_Video_Bink()
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_dtor, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_lspVideos.Len(); i++)
	{
		m_lspVideos[i]->Close();
		if (m_lspVideos[i]->m_TextureID)
			m_pTC->FreeID(m_lspVideos[i]->m_TextureID);
	}
}

void CTextureContainer_Video_Bink::Create(void* _pContext)
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_Create, MAUTOSTRIP_VOID);
	m_pTC->EnableTextureClassRefresh(m_iTextureClass);
	
	if (_pContext)
		AddVideo(CStr((const char*)_pContext));
}

int CTextureContainer_Video_Bink::AddVideo(CStr _FileName)
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_AddVideo, 0);
	MSCOPE(CTextureContainer_Video_Bink::AddVideo, SYSTEMCORE_TEX_BINK);
	
	spCTC_BinkTexture spVideo = MNew( CTC_BinkTexture );
	if (!spVideo) MemError("AddVideo");

	spVideo->Create(_FileName);

//	spVideo->Open();
//	spVideo->Close();

	spVideo->m_TextureID = m_pTC->AllocID(m_iTextureClass, m_lspVideos.Len(), spVideo->m_TextureName.Str());

	return m_lspVideos.Add(spVideo);
}

void CTextureContainer_Video_Bink::CreateFromDirectory(CStr _Path)
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_CreateFromDirectory, MAUTOSTRIP_VOID);
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
			if (pRec->m_Ext.CompareNoCase("BIK") == 0)
			{
				CStr FileName = _Path + pRec->m_Name;
				try { AddVideo(FileName); }
				catch(CCException) { LogFile("§cf80WARNING: (CTextureContainer_Video_Bink::CreateFromDirectory) Failure adding video " + FileName); }
			}
		}		
	}
}

void CTextureContainer_Video_Bink::ValidateLocalID(int _iLocal) const
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_ValidateLocalID, MAUTOSTRIP_VOID);
	if(!m_lspVideos.ValidPos(_iLocal))
		Error("ValidateLocalID", CStrF("Invalid local ID %d", _iLocal));
}

int CTextureContainer_Video_Bink::GetNumLocal()
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_GetNumLocal, 0);
	return m_lspVideos.Len();
}

int CTextureContainer_Video_Bink::GetLocal(const char* _pName)
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_GetLocal, 0);
	M_ASSERT(0, "Obsolete");
	return -1;

/*	for(int i = 0; i < m_lspVideos.Len(); i++)
		if(m_lspVideos[i]->m_TextureName.CompareNoCase(_pName) == 0)
			return i;

	return -1;*/
}

int CTextureContainer_Video_Bink::GetTextureID(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_GetTextureID, 0);
	ValidateLocalID(_iLocal);
	return m_lspVideos[_iLocal]->m_TextureID;
}

int CTextureContainer_Video_Bink::GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps)
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_GetTextureDesc, 0);
	MSCOPE(CTextureContainer_Video_Bink::GetTextureDesc, SYSTEMCORE_TEX_BINK);

	ValidateLocalID(_iLocal);
	
	if (m_lspVideos[_iLocal]->m_Width < 2)
	{
		m_lspVideos[_iLocal]->Open();
	}
	_pTargetImg->CreateVirtual(m_lspVideos[_iLocal]->m_Width, m_lspVideos[_iLocal]->m_Height, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);
//	_pTargetImg->CreateVirtual(m_lspVideos[_iLocal]->m_Width, m_lspVideos[_iLocal]->m_Height, IMAGE_FORMAT_BGRX8, IMAGE_MEM_IMAGE);
	_Ret_nMipmaps = 1;
	return 0;
}

int CTextureContainer_Video_Bink::GetWidth(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_GetWidth, 0);
	ValidateLocalID(_iLocal);
	
	if (m_lspVideos[_iLocal]->m_Width < 2)
	{
		m_lspVideos[_iLocal]->Open();
	}

	return m_lspVideos[_iLocal]->m_RealWidth;
}

int CTextureContainer_Video_Bink::GetHeight(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_GetHeight, 0);
	ValidateLocalID(_iLocal);
	
	if (m_lspVideos[_iLocal]->m_Height < 2)
	{
		m_lspVideos[_iLocal]->Open();
	}

	return m_lspVideos[_iLocal]->m_RealHeight;
}


void CTextureContainer_Video_Bink::GetTextureProperties(int _iLocal, CTC_TextureProperties& _Properties)
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_GetTextureProperties, MAUTOSTRIP_VOID);
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
	_Properties = Prop;
}

void CTextureContainer_Video_Bink::OnRefresh()
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_OnRefresh, MAUTOSTRIP_VOID);
	MSCOPE(CTextureContainer_Video_Bink::OnRefresh, SYSTEMCORE_TEX_BINK);

	for (int i = 0; i < m_lspVideos.Len(); i++)
	{
		CTC_BinkTexture* pV = m_lspVideos[i];
		if (pV->IsOpen())
		{
			if (s_nFrameCounter - pV->m_TimeLastVisible > 0)
			{
//				pV->Close();
				CloseVideo(i);
				m_pTC->MakeDirty(pV->m_TextureID);
			}
			else
			{
				m_pTC->MakeDirty(pV->m_TextureID);
			}
		}
	}
	
	s_nFrameCounter++;
}

void CTextureContainer_Video_Bink::Pause(int _iLocal, bool _Paused)
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_Pause, MAUTOSTRIP_VOID);
	MSCOPE(CTextureContainer_Video_Bink::Pause, SYSTEMCORE_TEX_BINK);
	ValidateLocalID(_iLocal);
	
	CTC_BinkTexture* pV = m_lspVideos[_iLocal];
	
	if (pV->IsOpen())
	{
		BinkPause(pV->m_hBink, _Paused);
	}
	else
	{
		pV->m_bPaused = _Paused;
	}

	if (!_Paused)
	{
		m_pTC->MakeDirty(pV->m_TextureID);
	}

}

void CTextureContainer_Video_Bink::AutoRestart(int _iLocal, bool _EnableAutoRestart)
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_AutoRestart, MAUTOSTRIP_VOID);
	ValidateLocalID(_iLocal);

	CTC_BinkTexture* pV = m_lspVideos[_iLocal];

	pV->m_bAutoRestart = _EnableAutoRestart;
}

void CTextureContainer_Video_Bink::Rewind(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_Rewind, MAUTOSTRIP_VOID);
	MSCOPE(CTextureContainer_Video_Bink::Rewind, SYSTEMCORE_TEX_BINK);
	ValidateLocalID(_iLocal);
	
	CTC_BinkTexture* pV = m_lspVideos[_iLocal];
		
	if (pV->IsOpen())
	{		
		BinkGoto(pV->m_hBink, 1, 0);
		pV->m_bOnLastFrame = false;
		pV->m_bDoneFrame = false;
		m_pTC->MakeDirty(pV->m_TextureID);
		pV->UpdateVolume();
	}
	else
	{
		pV->m_LastFrame = 1;
		pV->m_bOnLastFrame = false;
		pV->m_bDoneFrame = false;
		m_pTC->MakeDirty(pV->m_TextureID);
	}
}

bool CTextureContainer_Video_Bink::IsOnLastFrame(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_IsOnLastFrame, false);
	ValidateLocalID(_iLocal);
	
	CTC_BinkTexture* pV = m_lspVideos[_iLocal];
		
	return pV->m_bOnLastFrame;
}

int CTextureContainer_Video_Bink::GetFrame(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_GetFrame, 0);
	ValidateLocalID(_iLocal);

	CTC_BinkTexture* pV = m_lspVideos[_iLocal];

	if (pV->m_hBink)
	{		
		return pV->m_hBink->FrameNum;
	}
	else
	{
		return pV->m_LastFrame;
	}

}

int CTextureContainer_Video_Bink::GetNumFrames(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_GetNumFrames, 0);
	ValidateLocalID(_iLocal);
	
	CTC_BinkTexture* pV = m_lspVideos[_iLocal];
		
	return pV->m_Frames;
//	return 0;

}


fp4 CTextureContainer_Video_Bink::GetTime(int _iLocal)
{
	ValidateLocalID(_iLocal);
	HBINK hBink = m_lspVideos[_iLocal]->m_hBink;
	if (hBink)
		return hBink->FrameNum / fp4(hBink->FrameRate);
	else
		return 0.0f;
}


bool CTextureContainer_Video_Bink::MoveToLastFrame(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_MoveToLastFrame, false);
	MSCOPE(CTextureContainer_Video_Bink::MoveToLastFrame, SYSTEMCORE_TEX_BINK);
	ValidateLocalID(_iLocal);
	
	CTC_BinkTexture* pV = m_lspVideos[_iLocal];
		
	if (pV->IsOpen())
	{		
//		BinkSetSoundOnOff(pV->m_hBink, false);
		BinkGoto(pV->m_hBink, pV->m_Frames, 0);
		pV->m_bOnLastFrame = true;
		pV->m_bDoneFrame = false;
		m_pTC->MakeDirty(pV->m_TextureID);
	}
	else
	{
		pV->m_LastFrame = pV->m_Frames;
		pV->m_bOnLastFrame = false;
		pV->m_bDoneFrame = false;
		m_pTC->MakeDirty(pV->m_TextureID);
	}

	return 0;
}

void CTextureContainer_Video_Bink::CloseVideo(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_CloseVideo, MAUTOSTRIP_VOID);
	ValidateLocalID(_iLocal);
	m_lspVideos[_iLocal]->Close();

#ifdef PLATFORM_DOLPHIN // enclave beta-hack deluxe
	int TextureID = GetTextureID(_iLocal);
	CDisplayContextDolphin* pDC = CDisplayContextDolphin::Get();
	CRenderContextDolphin* pRC = safe_cast<CRenderContextDolphin>(pDC->GetRenderContext());
	pRC->ReleaseTexture(TextureID);
#endif
}

void CTextureContainer_Video_Bink::SetVolume(int _iLocal, fp4 fpVol)
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_SetVolume, MAUTOSTRIP_VOID);
	ValidateLocalID(_iLocal);
	m_lspVideos[_iLocal]->m_volume = Clamp01(fpVol);

	if (m_lspVideos[_iLocal]->m_hBink)
	{
		m_lspVideos[_iLocal]->UpdateVolume();
	}

}

CStr CTextureContainer_Video_Bink::GetName(int _iLocal)
{ 
	MAUTOSTRIP(CTextureContainer_Video_Bink_GetName, CStr());
	ValidateLocalID(_iLocal);
	return m_lspVideos[_iLocal]->m_TextureName;
};


void CTextureContainer_Video_Bink::SetFrame(int _iLocal, int _Frame)
{
	ValidateLocalID(_iLocal);
	CTC_BinkTexture& Video = *m_lspVideos[_iLocal];
	Video.m_LastFrame = _Frame;
	Video.RestoreState();
}


#ifdef PLATFORM_XBOX
#include "../RndrXbox/MRndrXbox.h"
RADEXPFUNC S32 RADEXPLINK BinkDX8SurfaceType(void* lpD3Ds);
#endif

void CTextureContainer_Video_Bink::BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _ConvertType, int _iStartMip)
{
	MAUTOSTRIP(CTextureContainer_Video_Bink_BuildInto, MAUTOSTRIP_VOID);
	MSCOPESHORT(CTextureContainer_Video_Bink::BuildInto);
	
	bool bLocked = false;
	try
	{
		
		MSCOPE(CTextureContainer_Video_Bink::BuildInto, SYSTEMCORE_TEX_BINK);
		ValidateLocalID(_iLocal);

		
		if (_nMipmaps != 1)
			Error("BuildInto", "nMipmaps != 1");

		//	if (_ppImg[0]->GetPixelSize() != 4)
		//		Error("BuildInto", "Not a 32-bit destination format.");
		
		//	_ppImg[0]->Fill(_ppImg[0]->GetClipRect(), 0xff008000);
		CTC_BinkTexture* pV = m_lspVideos[_iLocal];
		if (!pV->IsOpen())
		{
#ifdef SINGLE_FULLSCREEN_BINK
			if (pV->m_Width >= 512 || pV->m_Height >= 512)
			{
				for(int i = 0; i < m_lspVideos.Len(); i++)
					if (m_lspVideos[i]->IsOpen() && (m_lspVideos[i]->m_Width >= 512 || m_lspVideos[i]->m_Height >= 512))
					{
						m_lspVideos[i]->Close();
					}
			}
#endif
			pV->RestoreState();
		}

		if (!pV->m_hBink)
			return;

		bool CopyAll = !pV->m_bDoneFrame;
		bool DoneFrame = false;

		if (!pV->m_bDoneFrame || !BinkWait(pV->m_hBink))
		{
			DoneFrame = true;
			BinkDoFrame(pV->m_hBink);
			pV->m_bDoneFrame = true;
			pV->m_bOnLastFrame = pV->m_hBink->FrameNum == pV->m_hBink->Frames;
			if (pV->m_bOnLastFrame && !pV->m_bAutoRestart)
				BinkPause(pV->m_hBink, true);

			if (!pV->m_hBink->Paused)
			{
				BinkNextFrame(pV->m_hBink);
			}
		}

		pV->m_TimeLastVisible = s_nFrameCounter;

		CRct Rect(0,0,pV->m_RealWidth, pV->m_RealHeight);
//		_ppImg[0]->Fill(_ppImg[0]->GetClipRect(), 0xff00FF00);
		
#ifdef PLATFORM_XBOX
		void* pPixels = _ppImg[0]->Lock(&Rect);
		bLocked = true;
		int Modulo = _ppImg[0]->GetModulo();
		int Height = _ppImg[0]->GetHeight();

		int Fmt = 0;

		Fmt = BinkDX8SurfaceType((*((CRndrXbox_Image **)_ppImg))->m_Surface.GetRes());

		if (DoneFrame || _ConvertType)
		{
			BinkCopyToBuffer(pV->m_hBink,
				pPixels,
				Modulo,
				Height,
				0,0,
				Fmt | ((_ConvertType || CopyAll) ? BINKCOPYALL  : 0));
		}
		_ppImg[0]->Unlock();

#else
		
		if (DoneFrame || _ConvertType)
		{
			void* pPixels = _ppImg[0]->Lock(&Rect);
			bLocked = true;
			int Modulo = _ppImg[0]->GetModulo();
			int Height = _ppImg[0]->GetHeight();
			
			int Fmt = 0;
			
		switch(_ppImg[0]->GetPixelSize())
		{
		case 2 : Fmt = BINKSURFACE565; break;
		case 3 : 
			{
				switch(_ppImg[0]->GetFormat())
				{
				case IMAGE_FORMAT_RGB8 : Fmt = BINKSURFACE24R; break;
				default : Fmt = BINKSURFACE24;
				}
				break;
			}
		case 4 : 
			{
				switch(_ppImg[0]->GetFormat())
				{
				case IMAGE_FORMAT_RGBA8 : Fmt = BINKSURFACE32RA; break;
				default : Fmt = BINKSURFACE32A;
				}
				break;
			}
		default :
			Error("BuildInto", "Unsupported format: " + _ppImg[0]->IDString());
		}

			BinkCopyToBuffer(pV->m_hBink,
				pPixels,
				Modulo,
				Height,
				0,0,
				Fmt | ((_ConvertType || CopyAll) ? BINKCOPYALL : 0));
#ifdef PRODUCT_ENCLAVE
				if (pV->m_RealHeight == 512)
				{
					memset((uint8 *)pPixels + Modulo * 480, 0, Modulo * 2);
		}
#endif

			_ppImg[0]->Unlock();
		}
#endif

		
	}
	catch (CCExceptionFile)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
		if (bLocked)
			_ppImg[0]->Unlock();
	}
#ifdef M_SUPPORTSTATUSCORRUPT
	catch (CCException)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
		if (bLocked)
			_ppImg[0]->Unlock();
	}
#endif

}


void CTextureContainer_Video_Bink::GlobalPause()
{
	MACRO_GetRegisterObject(CSoundContext, pSound, "SYSTEM.SOUND");
	if (pSound)
	{
		int nV = m_lspVideos.Len();
		for (int i=0; i<nV; i++)
		{
			CTC_BinkTexture& Video = *m_lspVideos[i];
			if (Video.m_hBink && !Video.m_bGlobalPaused && (Video.m_hBink->Paused == 0))
			{
				BinkSetVolume(Video.m_hBink, 0, 0);
				BinkPause(Video.m_hBink, true);
				Video.m_bGlobalPaused = true;
			}
		}
	}
}


void CTextureContainer_Video_Bink::GlobalResume()
{
	MACRO_GetRegisterObject(CSoundContext, pSound, "SYSTEM.SOUND");
	if (pSound)
	{
		int nV = m_lspVideos.Len();
		for (int i=0; i<nV; i++)
		{
			CTC_BinkTexture& Video = *m_lspVideos[i];
			if (Video.m_hBink && Video.m_bGlobalPaused)
			{
				BinkSetVolume(Video.m_hBink, 0, BINK_MAXGAIN * (Video.m_volume * Video.m_Mainvolume));
				BinkPause(Video.m_hBink, false);
				Video.m_bGlobalPaused = false;
			}
		}
	}
}

MRTC_IMPLEMENT_DYNAMIC(CTextureContainer_Video_Bink, CTextureContainer_Video);

#endif
