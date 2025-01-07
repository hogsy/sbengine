
#include "PCH.h"

#include "../MSystem.h"

#if defined(PLATFORM_PS2) || defined(PLATFORM_DOLPHIN)
#include "MTextureContainerBinkDummy.h"

/*
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

//reads from the header
static U32 RADLINK MSystem_BinkFileReadHeader( BINKIO PTR4* bio, S32 offset, void* dest, U32 size )
{
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
			BF->File = DNew(CCFile) CCFile();
			BF->File->OpenExt(name, CFILE_READ | CFILE_BINARY, NO_COMPRESSION, NORMAL_COMPRESSION, 0.5, 4, 128*1024);
		}
		catch (CCExceptionFile)
		{
			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
			return 0;
		}
		catch (CCException)
		{
			CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPT);
			return 0;
		}
	}
	
	bio->ReadHeader = MSystem_BinkFileReadHeader;
	bio->ReadFrame = MSystem_BinkFileReadFrame;
	bio->GetBufferSize = MSystem_BinkFileGetBufferSize;
	bio->SetInfo = MSystem_BinkFileSetInfo;
	bio->Idle = MSystem_BinkFileIdle;
	bio->Close = MSystem_BinkFileClose;
	
	return( 1 );
}
*/
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTC_BinkTexture
|__________________________________________________________________________________________________
\*************************************************************************************************/
CTC_BinkTexture::CTC_BinkTexture()
{
	m_hBink = 0;
	m_Width = 0;
	m_Height = 0;
	m_TextureID = 0;
	m_bAutoRestart = true;
	m_bPaused = false;
	m_bDoneFrame = true;
	m_bOnLastFrame = false;
	m_Frames = 0;
	m_bValid = false;
	m_volume = 1;
	m_Mainvolume = 1;
}

CTC_BinkTexture::~CTC_BinkTexture()
{
	Close();
}

void CTC_BinkTexture::Create(CStr _FileName)
{
	if (!CDiskUtil::FileExists(_FileName))
		Error("Create", "Not a valid file: " + _FileName);

	m_FileName = _FileName;

	m_TextureName = CStrF("*VIDEO_%s", _FileName.GetFilenameNoExt().Str());
	m_Width = 1;
	m_Height = 1;
}

bool CTC_BinkTexture::IsOpen()
{
	return ( m_hBink != 0 );
}

void CTC_BinkTexture::Close()
{
	m_hBink = 0;
	SaveState();
}

void CTC_BinkTexture::Open()
{
	MSCOPESHORT(Open);
	
	if (IsOpen())
		Close();

	m_hBink = 1;
	
	m_bDoneFrame = false;

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys)
		m_Mainvolume = pSys->GetEnvironment()->GetValuef("SND_VOLUMESFX", 1.0f, 0);
	else
		m_Mainvolume = 1.0;
	
	
	m_Frames = 0;
	
	m_Width = 256;
	m_Height = 256;

	m_TimeLastVisible = CMTime::GetCPU();
	
	m_bValid = true;
}

void CTC_BinkTexture::SaveState()
{
	if (m_hBink)
	{
		m_bPaused = false;
		m_LastFrame = 0;
		m_bOnLastFrame = true;
	}
}

void CTC_BinkTexture::MakeValid()
{
	if (!IsOpen())
		Open();
}

void CTC_BinkTexture::RestoreState()
{
	if (!IsOpen())
		Open();
	
	if (!IsOpen())
		return;

	m_bDoneFrame = false;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_Bink
|__________________________________________________________________________________________________
\*************************************************************************************************/

CTextureContainer_Video_Bink::CTextureContainer_Video_Bink()
{
	m_CloseTimeOut = 0.4f;
}

CTextureContainer_Video_Bink::~CTextureContainer_Video_Bink()
{
	for(int i = 0; i < m_lspVideos.Len(); i++)
	{
		m_lspVideos[i]->Close();
		if (m_lspVideos[i]->m_TextureID)
			m_pTC->FreeID(m_lspVideos[i]->m_TextureID);
	}
}

void CTextureContainer_Video_Bink::Create(void* _pContext)
{
	m_pTC->EnableTextureClassRefresh(m_iTextureClass);
	
	if (_pContext)
		AddVideo(CStr((const char*)_pContext));
}

int CTextureContainer_Video_Bink::AddVideo(CStr _FileName)
{
	MSCOPE(CTextureContainer_Video_Bink::AddVideo, SYSTEMCORE_TEX_BINK);
	
	spCTC_BinkTexture spVideo = DNew(CTC_BinkTexture) CTC_BinkTexture;
	if (!spVideo) MemError("AddVideo");

	spVideo->Create(_FileName);

//	spVideo->Open();
//	spVideo->Close();

	spVideo->m_TextureID = m_pTC->AllocID(m_iTextureClass, m_lspVideos.Len(), spVideo->m_TextureName.Str());

	return m_lspVideos.Add(spVideo);
}

void CTextureContainer_Video_Bink::CreateFromDirectory(CStr _Path)
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
			if (pRec->m_Ext.CompareNoCase("BIK") == 0)
			{
				CStr FileName = _Path + pRec->m_Name;
				try { AddVideo(FileName); }
				catch(CCException) { LogFile("§cf80WARNING: (CTextureContainer_Video_Bink::CreateFromDirectory) Failure adding video " + FileName); }
			}
		}		
	}
}

void CTextureContainer_Video_Bink::ValidateLocalID(int _iLocal)
{
	if(!m_lspVideos.ValidPos(_iLocal))
		Error("ValidateLocalID", CStrF("Invalid local ID %d", _iLocal));
}

int CTextureContainer_Video_Bink::GetNumLocal()
{
	return m_lspVideos.Len();
}

int CTextureContainer_Video_Bink::GetLocal(const char* _pName)
{
	M_ASSERT(0, "Obsolete");
	return -1;
}

int CTextureContainer_Video_Bink::GetTextureID(int _iLocal)
{
	ValidateLocalID(_iLocal);
	return m_lspVideos[_iLocal]->m_TextureID;
}

int CTextureContainer_Video_Bink::GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps)
{
	MSCOPE(CTextureContainer_Video_Bink::GetTextureDesc, SYSTEMCORE_TEX_BINK);

	ValidateLocalID(_iLocal);
	
	if (m_lspVideos[_iLocal]->m_Width < 2)
	{
		m_lspVideos[_iLocal]->Open();
	}
	_pTargetImg->CreateVirtual(m_lspVideos[_iLocal]->m_Width, m_lspVideos[_iLocal]->m_Height, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);

	_Ret_nMipmaps = 1;
	return 0;
}

void CTextureContainer_Video_Bink::GetTextureProperties(int _iLocal, CTC_TextureProperties& _Properties)
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
	_Properties = Prop;
}

void CTextureContainer_Video_Bink::OnRefresh()
{
	MSCOPE(CTextureContainer_Video_Bink::OnRefresh, SYSTEMCORE_TEX_BINK);
	CMTime Time = CMTime::GetCPU();

	for(int i = 0; i < m_lspVideos.Len(); i++)
	{
		CTC_BinkTexture* pV = m_lspVideos[i];
		if (pV->IsOpen())
		{
			if ((Time - pV->m_TimeLastVisible).GetTime() > m_CloseTimeOut)
			{
				pV->Close();
				m_pTC->MakeDirty(pV->m_TextureID);
			}
			else
			{
				m_pTC->MakeDirty(pV->m_TextureID);
			}
		}
	}
}

void CTextureContainer_Video_Bink::Pause(int _iLocal, bool _Paused)
{
	MSCOPE(CTextureContainer_Video_Bink::Pause, SYSTEMCORE_TEX_BINK);
	ValidateLocalID(_iLocal);
	
	CTC_BinkTexture* pV = m_lspVideos[_iLocal];
	
}

void CTextureContainer_Video_Bink::AutoRestart(int _iLocal, bool _EnableAutoRestart)
{
	ValidateLocalID(_iLocal);

	CTC_BinkTexture* pV = m_lspVideos[_iLocal];

	pV->m_bAutoRestart = _EnableAutoRestart;
}

void CTextureContainer_Video_Bink::Rewind(int _iLocal)
{
	MSCOPE(CTextureContainer_Video_Bink::Rewind, SYSTEMCORE_TEX_BINK);
	ValidateLocalID(_iLocal);
	
	CTC_BinkTexture* pV = m_lspVideos[_iLocal];
		
	if (pV->IsOpen())
	{		
		pV->m_bOnLastFrame = false;
		pV->m_bDoneFrame = false;
		m_pTC->MakeDirty(pV->m_TextureID);
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
	ValidateLocalID(_iLocal);
	
	CTC_BinkTexture* pV = m_lspVideos[_iLocal];
		
	return true;
}

int CTextureContainer_Video_Bink::GetFrame(int _iLocal)
{
	ValidateLocalID(_iLocal);

	CTC_BinkTexture* pV = m_lspVideos[_iLocal];

	if (pV->m_hBink)
	{		
		return 0;
	}
	else
	{
		return pV->m_LastFrame;
	}

}

int CTextureContainer_Video_Bink::GetNumFrames(int _iLocal)
{
	ValidateLocalID(_iLocal);
	
	CTC_BinkTexture* pV = m_lspVideos[_iLocal];
		
	return pV->m_Frames;
}

bool CTextureContainer_Video_Bink::MoveToLastFrame(int _iLocal)
{
	MSCOPE(CTextureContainer_Video_Bink::MoveToLastFrame, SYSTEMCORE_TEX_BINK);
	ValidateLocalID(_iLocal);
	
	CTC_BinkTexture* pV = m_lspVideos[_iLocal];
		
	if (pV->IsOpen())
	{		
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
	ValidateLocalID(_iLocal);
	m_lspVideos[_iLocal]->Close();
}

void CTextureContainer_Video_Bink::SetVolume(int _iLocal, fp4 fpVol)
{
	ValidateLocalID(_iLocal);
	m_lspVideos[_iLocal]->m_volume = fpVol;
}

CStr CTextureContainer_Video_Bink::GetName(int _iLocal)
{ 
	ValidateLocalID(_iLocal);
	return m_lspVideos[_iLocal]->m_TextureName;
};

void CTextureContainer_Video_Bink::BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _ConvertType, int _iStartMip)
{
	MSCOPE(CTextureContainer_Video_Bink::BuildInto, SYSTEMCORE_TEX_BINK);
	ValidateLocalID(_iLocal);
	CTC_BinkTexture* pV = m_lspVideos[_iLocal];

	pV->m_bDoneFrame = true;
	pV->m_bOnLastFrame = true;
}

int CTextureContainer_Video_Bink::GetWidth(int _iLocal)
{
	return 640;
}

int CTextureContainer_Video_Bink::GetHeight(int _iLocal)
{
	return 480;
}

MRTC_IMPLEMENT_DYNAMIC(CTextureContainer_Video_Bink, CTextureContainer_Video);

#endif
