
#include "PCH.h"

#include "../MSystem.h"

#if defined(PLATFORM_XENON)
#include "MTextureContainerWMV.h"

#include "../../shared/mos/RenderContexts/xenon/MRenderXenon_Main.h"


#ifdef _DEBUG
#pragma comment(lib, "Xmediad.lib")
#else
#pragma comment(lib, "Xmedia.lib")
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Callbacks
|__________________________________________________________________________________________________
\*************************************************************************************************/

//#pragma optimize("",off)
//#pragma inline_depth(0)

HRESULT ReadCallback(PVOID _pContext, ULONGLONG _Offset, PVOID _pBuffer, DWORD _nBytesToRead, DWORD *_pnBytesRead)
{
	CCFile *pF = (CCFile*)_pContext;

	//Error check
	if( _Offset + _nBytesToRead > pF->Length() ) 
	{
		if( _Offset >= pF->Length() ) return S_FALSE;
		_nBytesToRead = pF->Length() - _Offset;
	}

	//Seek to position
	if( pF->Pos() != _Offset ) pF->Seek(_Offset);

	//Read data
	pF->Read(_pBuffer,_nBytesToRead);
	(*_pnBytesRead) = _nBytesToRead;

	return S_OK;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTC_WMVTexture
|__________________________________________________________________________________________________
\*************************************************************************************************/

uint32 lTimes[100] = {0};
bool CTC_WMVTexture::CWMV::DoWork()
{
	if( !m_pPlayer ) return false;

	CFrame *pFrame = GetFreeFrame();

	if (pFrame)
	{
		// Start by reading data
		int bBufferReady = true;

		int Width = pFrame->m_Y.GetWidth();
		int Height = pFrame->m_Y.GetHeight();
		void *pData = pFrame->m_Y.Lock();
		int Stride = pFrame->m_Y.GetModulo();
		{
			XMEDIA_VIDEO_FRAME VideoFrame;    
			ZeroMemory( &VideoFrame, sizeof( XMEDIA_VIDEO_FRAME ) );

			VideoFrame.videoFormat = XMEDIA_VIDEO_FORMAT_I420;
			VideoFrame.i420.pvYBuffer = pData;
			VideoFrame.i420.dwYBufferSize = Height * Stride;
			VideoFrame.i420.dwYPitch = Stride;
			VideoFrame.i420.pvUBuffer = m_lU.GetBasePtr();
			VideoFrame.i420.dwUBufferSize = m_lU.Len();
			VideoFrame.i420.dwUPitch = Stride >> 1;
			VideoFrame.i420.pvVBuffer = m_lV.GetBasePtr();
			VideoFrame.i420.dwVBufferSize = m_lV.Len();
			VideoFrame.i420.dwVPitch = Stride >> 1;
			VideoFrame.dwFlags = XMEDIA_PLAY_DISABLE_AV_SYNC;

			HRESULT Res = m_pPlayer->GetNextFrame(&VideoFrame);
			if( FAILED( Res ) || (Res == XMEDIA_W_EOF) )
			{
				m_bEOF = true;
				bBufferReady = false;
			}
			else if( Res == XMEDIA_W_NO_DATA )
			{
				bBufferReady = false;
			}
			else
			{
				/*
				static LONG lTimes[100];
				if( m_nFrames < 100 )
				{
					lTimes[m_nFrames] = VideoFrame.lTimeToPresent;
					if( m_nFrames == 99 )
					{
						m_nFrames = 99;
					}
				}
				*/

				pFrame->m_Time = m_FrameTime * (fp64)m_nFrames + m_SoundOffset;
				if( pFrame->m_Time < m_LastAddedTime )
				{
					pFrame->m_Time = m_LastAddedTime + m_FrameTime;
				}
				m_nFrames += 1;
			}
		}
		pFrame->m_Y.Unlock();

		if( bBufferReady )
		{
			pData = pFrame->m_UV.Lock();
			Width = pFrame->m_UV.GetWidth();
			Height = pFrame->m_UV.GetHeight();
			Stride = pFrame->m_UV.GetModulo();
			for (int y = 0; y < Height; ++y)
			{
				uint8 *pU = m_lU.GetBasePtr() + Width * y;
				uint8 *pV = m_lV.GetBasePtr() + Width * y;
				uint8 *pUV = ((uint8 *)pData) + Stride * y;

				for (int x = 0; x < (Width >> 1); ++x)
				{
					((uint32 *)pUV)[x] = pU[x*2] << 0 | pV[x*2] << 8 | pU[x*2+1] << 16 | pV[x*2+1] << 24;
				}
			}
			pFrame->m_UV.Unlock();
		}

		if (!bBufferReady)
		{
			ReturnFrame(pFrame);
			return true;
		}

		AddFinisherFrame(pFrame);
		return true;
	}

	return false;
}

#ifdef PLATFORM_XENON
#include "XTL.h"
#endif

const char* CTC_WMVTexture::CWMV::Thread_GetName() const
{
	return "WMV unpacker";
}

int CTC_WMVTexture::CWMV::Thread_Main()
{
	//	MRTC_SystemInfo::Thread_SetProcessor(4);

	while (!Thread_IsTerminating() && m_bIsOpen)
	{				
		if (!DoWork())
		{
			if (m_bEOF)
				break;
			if (!m_WorkEvent.WaitTimeout(1.0))
				M_TRACEALWAYS("m_WorkEvent timed out\n");
		}

		if (m_bEOF)
		{
			break;
		}
	}

	{
		M_LOCK(m_Lock);
		m_WorkDoneEvent.Signal();
	}

	return 0;
}

CTC_WMVTexture::CWMV::CFrame *CTC_WMVTexture::CWMV::GetFreeFrame()
{
	M_LOCK(m_Lock);
	for (int i = 0; i < WMVNumFrames; ++i)
	{
		if (m_pFreeFrames[i])
		{
			CFrame *pRet = m_pFreeFrames[i];
			m_pFreeFrames[i] = NULL;
			return pRet;
		}
	}
	return 0;
}

CMTime CTC_WMVTexture::CWMV::GetSoundTime()
{
	if (m_SoundVoices.m_iVoices[0] >= 0)
	{
		if (m_pSC)
		{
			CMTime Pos = m_pSC->Voice_GetPlayPos(m_SoundVoices.m_iVoices[0]);

			return Pos;
		}
		else
			return CMTime::CreateInvalid();
	}

	return CMTime::CreateInvalid();
}


CMTime CTC_WMVTexture::CWMV::GetNextFrameTime()
{
	M_LOCK(m_Lock);

	if (m_pFinishedFrames[m_iFinishedConsume])
	{
		fp64 Scaled = (m_pFinishedFrames[m_iFinishedConsume]->m_Time / m_OriginalFrameTime) * m_FrameTime;
		int64 Ticks = Scaled;
		fp32 Fraction = Scaled - Ticks;

		return CMTime::CreateFromTicks(Ticks, 1, Fraction);
	}
	else if (m_bEOF)
		return CMTime::CreateInvalid();
	else			
	{
		fp64 Scaled = (m_LastAddedTime + m_FrameTime);
		int64 Ticks = Scaled;
		fp32 Fraction = Scaled - Ticks;

		return CMTime::CreateFromTicks(Ticks, 1, Fraction);
	}
}

void CTC_WMVTexture::CWMV::AddFinisherFrame(CFrame *_pFrame)
{
	M_LOCK(m_Lock);
	M_ASSERT(!m_pFinishedFrames[m_iFinishedProduce], "Should not be anything in here");
	m_pFinishedFrames[m_iFinishedProduce] = _pFrame;
	++m_iFinishedProduce;
	if (m_iFinishedProduce >= WMVNumFrames)
		m_iFinishedProduce = 0;

	m_LastAddedTime = (_pFrame->m_Time / m_OriginalFrameTime) * m_FrameTime;

	m_WorkDoneEvent.Signal();
}

CTC_WMVTexture::CWMV::CFrame *CTC_WMVTexture::CWMV::GetFrame(bool _bBlock)
{
	M_LOCK(m_Lock);
	m_WorkDoneEvent.WaitTimeout(0.0);
	CFrame *pFrame = m_pFinishedFrames[m_iFinishedConsume];
	if (pFrame)
	{
		m_pFinishedFrames[m_iFinishedConsume] = NULL;
		++m_iFinishedConsume;
		if (m_iFinishedConsume >= WMVNumFrames)
			m_iFinishedConsume = 0;

		return pFrame;
	}
	else if (!m_bEOF)
	{
		if (!_bBlock)
			return NULL;
		M_UNLOCK(m_Lock);
		if (!m_WorkDoneEvent.WaitTimeout(1.0))
		{
			M_TRACEALWAYS("m_WorkDoneEvent timed out m_bEOF = %d \n", m_bEOF);
		}
	}

	pFrame = m_pFinishedFrames[m_iFinishedConsume];
	if (pFrame)
	{
		m_pFinishedFrames[m_iFinishedConsume] = NULL;
		++m_iFinishedConsume;
		if (m_iFinishedConsume >= WMVNumFrames)
			m_iFinishedConsume = 0;
		return pFrame;
	}

	return 0;
}

void CTC_WMVTexture::CWMV::ReturnFrame(CFrame *_pFrame)
{
	M_LOCK(m_Lock);
	CFrame *pFrame = _pFrame;

	for (int i = 0; i < WMVNumFrames; ++i)
	{
		if (!m_pFreeFrames[i])
		{
			m_pFreeFrames[i] = pFrame;
			m_WorkEvent.Signal();
			return;
		}
	}
}

extern __declspec(thread) g_XOverrideReadWritePhysical;

bool CTC_WMVTexture::CWMV::Init(CSoundContext *_pSC,D3DDevice * _pDev)
{
#ifdef _DEBUG
	// We don't support WMV in debug because DirectX asserts
	return false;
#endif
	//Create WMV player
	{
		XMEDIA_XMV_CREATE_PARAMETERS Param;
		ZeroMemory(&Param,sizeof(XMEDIA_XMV_CREATE_PARAMETERS));
		Param.createType = XMEDIA_CREATE_FROM_USER_IO;
		Param.dwFlags = XMEDIA_CREATE_CPU_AFFINITY;
		Param.createFromUserIo.pfnAudioStreamReadCallback = ReadCallback;
		Param.createFromUserIo.pfnVideoStreamReadCallback = ReadCallback;
		Param.createFromUserIo.pvAudioStreamContext = &m_File;
		Param.createFromUserIo.pvVideoStreamContext = &m_File;
		Param.dwAudioStreamId = XMEDIA_STREAM_ID_DONT_USE;
		Param.dwVideoStreamId = XMEDIA_STREAM_ID_USE_DEFAULT;
		Param.dwAudioDecoderCpu = 1;
		Param.dwVideoDecoderCpu = 3;
		Param.dwAudioRendererCpu = 1;
		Param.dwVideoRendererCpu = 1;

/*
		_pDev = NRenderXenon::CRenderContextXenon::ms_This.m_pDevice;
		NRenderXenon::CRenderContextXenon::ms_This.m_bHoldDevice = true;
		Sleep(200);
		_pDev->AcquireThreadOwnership();
*/
//#ifndef _DEBUG
		g_XOverrideReadWritePhysical = true;
//#endif
		HRESULT Res = XMediaCreateXmvPlayer(_pDev,&Param,&m_pPlayer);
//#ifndef _DEBUG
		g_XOverrideReadWritePhysical = false;
//#endif
		//_pDev->ReleaseThreadOwnership();
		if( FAILED(Res) )
		{
			M_TRACEALWAYS(CStrF("Error reading WMV, Error code %d\n",GetLastError()).Str());
			return false;
		}

		//Get stream info
		m_pPlayer->GetVideoDescriptor(&m_VideoDesc);
	}

	m_bEOF = false;
	m_iFinishedProduce = 0;
	m_iFinishedConsume = 0;
	m_Time = 0;

	m_FrameTime = m_OriginalFrameTime = 1.0f / m_VideoDesc.fFrameRate;

	//Create sound
	CStr FileName = m_File.GetFileName();
	CStr SetFile = FileName.GetPath() + FileName.GetFilenameNoExt() + ".set";
	m_SoundOffset = 0;
	if (CDiskUtil::FileExists(SetFile))
	{
		CRegistry_Dynamic Temp;
		Temp.ReadSimple(SetFile);
		m_FrameTime = m_OriginalFrameTime = 1.0/Temp.GetValuef("FRAMERATE", 1.0/m_FrameTime);
		m_SoundOffset = Temp.GetValuef("SOUNDOFFSET", 0.0);
	}

	m_LastAddedTime = m_SoundOffset + -m_FrameTime;
	m_iCurrentFrame = 0;

	m_bIsOpen = true;

	m_pSC = _pSC;

	//Initialize frame data
	m_nFrames = 0;
	for (int i = 0; i < WMVNumFrames; ++i)
	{
		m_pFreeFrames[i] = DNew(CTC_WMVTexture::CWMV::CFrame) CTC_WMVTexture::CWMV::CFrame;
		m_pFinishedFrames[i] = NULL;
		m_pFreeFrames[i]->m_Y.Create(m_VideoDesc.dwWidth, 
			m_VideoDesc.dwHeight, IMAGE_FORMAT_I8, IMAGE_MEM_SYSTEM);
		m_pFreeFrames[i]->m_UV.Create(m_VideoDesc.dwWidth >> 1, 
			m_VideoDesc.dwHeight >> 1, IMAGE_FORMAT_I8A8, IMAGE_MEM_SYSTEM);
	}

	m_lU.SetLen((m_VideoDesc.dwHeight * m_VideoDesc.dwWidth) >> 2);
	m_lV.SetLen(m_lU.Len());

	//Launch thread
	Thread_Create(NULL, 16384, MRTC_THREAD_PRIO_ABOVENORMAL);

	return true;
}

void CTC_WMVTexture::CWMV::Cleanup()
{
	if (m_bIsOpen)
	{
		m_bIsOpen = false;
		m_WorkEvent.Signal();
		Thread_Destroy();
		if( m_pPlayer )
		{
//#ifndef _DEBUG
			g_XOverrideReadWritePhysical = true;
//#endif
			m_pPlayer->Release();
//#ifndef _DEBUG
			g_XOverrideReadWritePhysical = false;
//#endif
			m_pPlayer = NULL;
		}

		m_lU.Clear();
		m_lV.Clear();

		int nDeleted = 0;

		for (int i = 0; i < WMVNumFrames; ++i)
		{
			if (m_pFreeFrames[i])
			{
				++nDeleted;
				delete m_pFreeFrames[i];
			}
			if (m_pFinishedFrames[i])
			{
				++nDeleted;
				delete m_pFinishedFrames[i];
			}
		}
		M_ASSERT(nDeleted == WMVNumFrames, "");

		if (m_pSC)
			m_SoundVoices.Destroy(m_pSC);

	}		
}

void CTC_WMVTexture::CWMV::ResumeSound()
{
	if (m_bPausedSound)
	{
		if (m_pSC)
		{
			for (int i = 0; i < 6; ++i)
			{
				if (m_SoundVoices.m_iVoices[i] >= 0)
				{
					m_pSC->Voice_Unpause(m_SoundVoices.m_iVoices[i]);
				}
			}
		}

		m_bPausedSound = false;
	}
}

void CTC_WMVTexture::CWMV::PauseSound()
{
	if (!m_bPausedSound)
	{
		if (m_pSC)
		{
			for (int i = 0; i < 6; ++i)
			{
				if (m_SoundVoices.m_iVoices[i] >= 0)
				{
					m_pSC->Voice_Pause(m_SoundVoices.m_iVoices[i]);
				}
			}
		}

		m_bPausedSound = true;
	}
}

void CTC_WMVTexture::CWMV::SetSound(int _ihSound)
{
	m_SoundVoices.Destroy(m_pSC);
	m_SoundVoices.m_iVoices[0] = _ihSound;
	m_SoundVoices.m_iVoicesOwn[0] = false;
	m_bPausedSound = false;
}

void CTC_WMVTexture::CWMV::PlaySound(int _iChannel)
{
	if (m_SoundVoices.m_iVoices[0] >= 0)
		return;
	if (m_pSC && _iChannel >= 0)
	{

		CStr Sound = CStrF("%s", "video_") + m_pTexture->m_FileName.GetFilenameNoExt();

		if (Sound.Len())
		{
			CWaveContext *pWC = m_pSC->Wave_GetContext();

			int WaveId = pWC->GetWaveID(Sound + "ST");

			if (WaveId >= 0)
			{
				MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");

				fp32 Vol = 1.0;
				if (pSys)
					Vol = pSys->GetRegistry()->GetValuef("SND_VIDEO_VOLUME", 1.0);

				m_SoundVoices.Destroy(m_pSC);

				CSC_VoiceCreateParams CreateParams;
				m_pSC->Voice_InitCreateParams(CreateParams, 203);
				CreateParams.m_Volume = Vol;
				CreateParams.m_hChannel = _iChannel;
				CreateParams.m_WaveID = WaveId;
				m_SoundVoices.m_iVoices[0] = m_pSC->Voice_Create(CreateParams);
				m_bPausedSound = false;
			}
		}
	}
}


CTC_WMVTexture::CWMV::CWMV()
{
	m_bIsOpen = false;
	m_pPlayer = NULL;
}

CTC_WMVTexture::CWMV::~CWMV()
{
	Cleanup();

}

CTC_WMVTexture::CTC_WMVTexture()
{
	MAUTOSTRIP(CTC_WMVTexture_ctor, MAUTOSTRIP_VOID);

	m_TextureID[0] = 0;
	m_TextureID[1] = 0;
	m_TextureID[2] = 0;
	m_pDecoder = 0;
	m_pFrame	= 0;
	m_LastFrame = 0;
	m_bOnLastFrame = false;
	m_bBroken = false;
	m_bWasRendered = false;
	m_bClosed = false;
}

//
//
//
CTC_WMVTexture::~CTC_WMVTexture()
{
	MAUTOSTRIP(CTC_WMVTexture_dtor, MAUTOSTRIP_VOID);
	if( m_pFrame )
	{
		m_pDecoder->ReturnFrame(m_pFrame);
		m_pFrame	= 0;
	}

	Close();
}

//
//
//
void CTC_WMVTexture::Create(CStr _FileName)
{
	MAUTOSTRIP(CTC_WMVTexture_Create, MAUTOSTRIP_VOID);
	if (!CDiskUtil::FileExists(_FileName))
		Error("Create", "Not a valid file: " + _FileName);

	m_FileName = _FileName;

	m_TextureName[0] = CStrF("*VIDEO_%s", _FileName.GetFilenameNoExt().Str());
	m_TextureName[1] = CStrF("*VIDEO_%s_y", _FileName.GetFilenameNoExt().Str());
	m_TextureName[2] = CStrF("*VIDEO_%s_uv", _FileName.GetFilenameNoExt().Str());
}

//
//
//
bool CTC_WMVTexture::IsOpen()
{
	MAUTOSTRIP(CTC_WMVTexture_IsOpen, false);
	return m_pDecoder != NULL;
}

//
//
//
void CTC_WMVTexture::Close()
{
	MAUTOSTRIP(CTC_WMVTexture_Close, MAUTOSTRIP_VOID);
	MSCOPE(CTC_WMVTexture::Close, TEXTURECONTAINER_WMV);

	if( m_pFrame )
	{
		m_pDecoder->ReturnFrame(m_pFrame);
		m_pFrame	= NULL;
	}

	if (m_pDecoder)
	{
		delete m_pDecoder;
		m_pDecoder = NULL;
	}

	m_bWasRendered = false;
	m_bClosed = true; // make sure the video isn't reopened automagically (arrgh!)
	m_bOnLastFrame = 0;
}


//
//
//
void CTC_WMVTexture::Open(D3DDevice * _pDev)
{
	MAUTOSTRIP(CTC_WMVTexture_Open, MAUTOSTRIP_VOID);
	MSCOPE(CTC_WMVTexture::Open, TEXTURECONTAINER_WMV);

	if (m_bBroken)
		return;
	// Close existing
	Close();
	m_bClosed = false;

	M_TRY
	{
		m_pDecoder = DNew(CWMV) CWMV;
		m_pDecoder->m_File.OpenExt(m_FileName.Str(), CFILE_READ | CFILE_BINARY | CFILE_NODEFERCLOSE, NO_COMPRESSION, NORMAL_COMPRESSION, 0.5, 4, 128*1024);
	}
	M_CATCH(
		catch (CCExceptionFile)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
		Close();
		m_bBroken = true;
		return;
	}
	)

		MACRO_GetRegisterObject(CSoundContext, pSound, "SYSTEM.SOUND");

	if (!m_pDecoder->Init(pSound,_pDev))
	{
		Close();
		m_bBroken = true;
		return;
	}
	m_pDecoder->m_pTexture = this;

	m_TextureWidth = m_pDecoder->m_VideoDesc.dwWidth;
	m_TextureHeight = m_pDecoder->m_VideoDesc.dwHeight;
	m_FrameTime = m_pDecoder->m_FrameTime;

	//
	m_TimeLastVisible = CMTime::GetCPU();
	m_bOnLastFrame = false;
}

//
//
//

//
// Unsupported functions
//
void CTC_WMVTexture::SaveState()
{
}

void CTC_WMVTexture::RestoreState()
{
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_WMV
|__________________________________________________________________________________________________
\*************************************************************************************************/
CTextureContainer_Video_WMV::CTextureContainer_Video_WMV()
{
	MAUTOSTRIP(CTextureContainer_Video_WMV_ctor, MAUTOSTRIP_VOID);
	m_CloseTimeOut = 0.5f;
	m_iSoundChannel = -1;
	m_pDevice = NULL;
}

//
// Frees all textures and videos
//
CTextureContainer_Video_WMV::~CTextureContainer_Video_WMV()
{
	MAUTOSTRIP(CTextureContainer_Video_WMV_dtor, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_lspVideos.Len(); i++)
	{
		CloseVideo(m_lspVideos[i]);//->Close();
		for( int j = 0; j < 3; j++ )
		{
			if(m_lspVideos[i]->m_TextureID[j])
			{
				m_pTC->FreeID(m_lspVideos[i]->m_TextureID[j]);
			}
		}
	}

	if( m_pDevice )
	{
		m_pDevice->Release();
		m_pDevice = NULL;
	}

	MACRO_GetRegisterObject(CSoundContext, pSound, "SYSTEM.SOUND");
	if (pSound && m_iSoundChannel >= 0)
	{
		pSound->Chn_Free(m_iSoundChannel);
	}
}

//
// Adds a video to the list
//
void CTextureContainer_Video_WMV::Create(void* _pContext)
{
	MAUTOSTRIP(CTextureContainer_Video_WMV_Create, MAUTOSTRIP_VOID);
	m_pTC->EnableTextureClassRefresh(m_iTextureClass);

	if (_pContext)
		AddVideo(CStr((const char*)_pContext));
}

//
// Adds a video to the list
//
int CTextureContainer_Video_WMV::AddVideo(CStr _FileName)
{
	MAUTOSTRIP(CTextureContainer_Video_WMV_AddVideo, 0);
	MSCOPE(CTextureContainer_Video_WMV::AddVideo, TEXTURECONTAINER_WMV);

	// Allocate video
	spCTC_WMVTexture spVideo = MNew(CTC_WMVTexture);
	if (!spVideo)
		MemError("AddVideo");

	// Create and allocate texture id
	spVideo->Create(_FileName);
	spVideo->m_TextureID[0] = m_pTC->AllocID(m_iTextureClass, m_lspVideos.Len() * 3 + 0, spVideo->m_TextureName[0].Str());
	spVideo->m_TextureID[1] = m_pTC->AllocID(m_iTextureClass, m_lspVideos.Len() * 3 + 1, spVideo->m_TextureName[1].Str());
	spVideo->m_TextureID[2] = m_pTC->AllocID(m_iTextureClass, m_lspVideos.Len() * 3 + 2, spVideo->m_TextureName[2].Str());

	// Add to the list and return the local id
	return m_lspVideos.Add(spVideo);
}

//
// Recursive, checks for extension (not used, AddVideo is called from ScanVideo)
//
void CTextureContainer_Video_WMV::CreateFromDirectory(CStr _Path)
{
	MAUTOSTRIP(CTextureContainer_Video_WMV_CreateFromDirectory, MAUTOSTRIP_VOID);
	Create(NULL);

	CDirectoryNode Dir;
	Dir.ReadDirectory(_Path + "*");

	for(int32 i = 0; i < Dir.GetFileCount(); i++)
	{
		CDir_FileRec *pRec = Dir.GetFileRec(i);
		if (pRec->IsDirectory())
		{
			if (pRec->m_Name.Copy(0,1) != ".")
				CreateFromDirectory(_Path + pRec->m_Name + "\\");
		}
		else
		{
			if (pRec->m_Ext.CompareNoCase("wmv") == 0)
			{
				CStr FileName = _Path + pRec->m_Name;
				M_TRY
				{
					AddVideo(FileName);
				}
				M_CATCH(
					catch(CCException)
				{
					LogFile("§cf80WARNING: (CTextureContainer_Video_WMV::CreateFromDirectory) Failure adding video " + FileName);
				}
				)
			}
		}		
	}
}

//
//
//
void CTextureContainer_Video_WMV::ValidateLocalID(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_WMV_ValidateLocalID, MAUTOSTRIP_VOID);
	if(!m_lspVideos.ValidPos(_iLocal))
		Error("ValidateLocalID", CStrF("Invalid local ID %d", _iLocal));
}

//
//
//
int CTextureContainer_Video_WMV::GetNumLocal()
{
	MAUTOSTRIP(CTextureContainer_Video_WMV_GetNumLocal, 0);
	return m_lspVideos.Len();
}

//
//
//
int CTextureContainer_Video_WMV::GetLocal(const char* _pName)
{
	MAUTOSTRIP(CTextureContainer_Video_WMV_GetLocal, 0);
	M_ASSERT(0, "Obsolete");
	return -1;
}

//
//
//
int CTextureContainer_Video_WMV::GetTextureID(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_WMV_GetTextureID, 0);
	int iLocal = _iLocal / 3;
	int iSub = _iLocal - iLocal * 3;
	ValidateLocalID(iLocal);
	CTC_WMVTexture* pVideo = m_lspVideos[iLocal];
	M_LOCK(pVideo->m_TextureLock);
	return pVideo->m_TextureID[iSub];
}

//
//
//
int CTextureContainer_Video_WMV::GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps)
{
	MAUTOSTRIP(CTextureContainer_Video_WMV_GetTextureDesc, 0);
	MSCOPE(CTextureContainer_Video_WMV::GetTextureDesc, TEXTURECONTAINER_WMV);

	int iLocal = _iLocal / 3;
	int iSub = _iLocal - iLocal * 3;
	ValidateLocalID(iLocal);
	CTC_WMVTexture *pVideo = m_lspVideos[iLocal];

	M_LOCK(pVideo->m_TextureLock);

	if (!pVideo->IsOpen() && !pVideo->m_bClosed)
	{
		OpenVideo(pVideo);
	}
	if (!pVideo->IsOpen())
		return 0;

	switch( iSub )
	{
	case 0:
		{
			_pTargetImg->CreateVirtual(pVideo->m_TextureWidth, pVideo->m_TextureHeight, IMAGE_FORMAT_BGRX8, IMAGE_MEM_IMAGE);
			break;
		}
	case 1:
		{
			_pTargetImg->CreateVirtual(pVideo->m_TextureWidth, pVideo->m_TextureHeight, IMAGE_FORMAT_I8, IMAGE_MEM_IMAGE);
			break;
		}
	case 2:
		{
			_pTargetImg->CreateVirtual(pVideo->m_TextureWidth / 2, pVideo->m_TextureHeight / 2, IMAGE_FORMAT_I8A8, IMAGE_MEM_IMAGE);
			break;
		}
	}
	_Ret_nMipmaps = 1;
	return 0;
}

//
// Opens the video if needed
//
int CTextureContainer_Video_WMV::GetWidth(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_WMV_GetWidth, 0);
	int iLocal = _iLocal / 3;
	ValidateLocalID(iLocal);

	CTC_WMVTexture *pVideo = m_lspVideos[iLocal];
	M_LOCK(pVideo->m_TextureLock);

	if (!pVideo->IsOpen() && !pVideo->m_bClosed)
	{
		OpenVideo(pVideo);
	}
	if (!pVideo->IsOpen())
		return 0;

	return pVideo->m_pDecoder->m_VideoDesc.dwWidth;
}

//
// Opens the video if needed
//
int CTextureContainer_Video_WMV::GetHeight(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_WMV_GetHeight, 0);
	int iLocal = _iLocal / 3;
	ValidateLocalID(iLocal);

	CTC_WMVTexture *pVideo = m_lspVideos[iLocal];
	M_LOCK(pVideo->m_TextureLock);
	if (!pVideo->IsOpen() && !pVideo->m_bClosed)
	{
		OpenVideo(pVideo);
	}
	if (!pVideo->IsOpen())
		return 0;

	return pVideo->m_pDecoder->m_VideoDesc.dwHeight;
}

//
//
//
void CTextureContainer_Video_WMV::GetTextureProperties(int _iLocal, CTC_TextureProperties& _Properties)
{
	MAUTOSTRIP(CTextureContainer_Video_WMV_GetTextureProperties, MAUTOSTRIP_VOID);
	int iLocal = _iLocal / 3;
	ValidateLocalID(iLocal);
	CTC_TextureProperties Prop;
	Prop.m_Flags = 
		CTC_TEXTUREFLAGS_CLAMP_U | CTC_TEXTUREFLAGS_CLAMP_V  |
		CTC_TEXTUREFLAGS_NOMIPMAP | CTC_TEXTUREFLAGS_NOPICMIP |
		CTC_TEXTUREFLAGS_HIGHQUALITY | CTC_TEXTUREFLAGS_NOCOMPRESS |
		CTC_TEXTUREFLAGS_PROCEDURAL;
	_Properties = Prop;
}

//
//
//
void CTextureContainer_Video_WMV::OnRefresh()
{

	if (m_iSoundChannel < 0)
	{
		MACRO_GetRegisterObject(CSoundContext, pSound, "SYSTEM.SOUND");
		if (pSound)
			m_iSoundChannel = pSound->Chn_Alloc(1);
	}

	MAUTOSTRIP(CTextureContainer_Video_WMV_OnRefresh, MAUTOSTRIP_VOID);
	MSCOPE(CTextureContainer_Video_WMV::OnRefresh, TEXTURECONTAINER_WMV);
	CMTime Time = CMTime::GetCPU();
	CMTime Now = Time;

	for(int32 i = 0; i < m_lspVideos.Len(); i++)
	{
		CTC_WMVTexture *pVideo = m_lspVideos[i];
		static CTC_WMVTexture *pVideoDebug;
		pVideoDebug = pVideo;
		M_LOCK(pVideo->m_TextureLock);

		if (pVideo->m_bOnLastFrame > 1)
			--pVideo->m_bOnLastFrame;

		if(pVideo->IsOpen())
		{
			//
			fp32 Time1 = (Time - pVideo->m_TimeLastVisible).GetTime();

			// Only autoclose video if no sound is playing on it
			if(pVideo->m_pDecoder->m_SoundVoices.m_iVoicesOwn[0] && Time1 > m_CloseTimeOut && pVideo->m_pFrame && (pVideo->m_bOnLastFrame == 1 || !pVideo->m_pFrame->m_bFrameShown || pVideo->m_pFrame->m_bFrameShown >= 2))//!pVideo->m_pFrame->m_bFrameShown)// || !pVideo->m_bWasRendered)
			{
				if (!pVideo->m_bOnLastFrame)
					pVideo->m_bOnLastFrame = 5;
				else if (pVideo->m_bOnLastFrame == 1)
				{
					CloseVideo(pVideo);//->Close();
					if(pVideo->m_TextureID[0]) m_pTC->MakeDirty(pVideo->m_TextureID[0]);
					if(pVideo->m_TextureID[1]) m_pTC->MakeDirty(pVideo->m_TextureID[1]);
					if(pVideo->m_TextureID[2]) m_pTC->MakeDirty(pVideo->m_TextureID[2]);
				}
			}
			else
			{
				bint bClosed = false;
				if (!pVideo->m_pDecoder->m_SoundVoices.m_iVoicesOwn[0])
				{
					if (!pVideo->m_pDecoder->m_pSC->Voice_IsPlaying(pVideo->m_pDecoder->m_SoundVoices.m_iVoices[0]))
					{
						CloseVideo(pVideo);//->Close();
						if(pVideo->m_TextureID[0]) m_pTC->MakeDirty(pVideo->m_TextureID[0]);
						if(pVideo->m_TextureID[1]) m_pTC->MakeDirty(pVideo->m_TextureID[1]);
						if(pVideo->m_TextureID[2]) m_pTC->MakeDirty(pVideo->m_TextureID[2]);
						bClosed = true;
					}					
				}

				if (!bClosed)
				{

					// Check if new frame should be obtained
					bool bFinishedFrame = false;
					while (1)
					{
						CMTime FrameTime = pVideo->m_pDecoder->GetNextFrameTime();
						if( !FrameTime.IsInvalid() )
						{
							CMTime Time;
							bool bNewFrame = false;
							if (FrameTime.GetTime() < 0.00001f)
							{
								bNewFrame = true;
								pVideo->m_VideoTimeStart = Now;
								pVideo->m_pDecoder->PlaySound(m_iSoundChannel);
							}
							else 
							{
								CMTime SoundTime = pVideo->m_pDecoder->GetSoundTime();

								if (!SoundTime.IsInvalid())
								{
									Time = SoundTime + CMTime::CreateFromSeconds(1/45.0f);
									static int LastTime = 0;
									int Time = SoundTime.GetTime();
									if (LastTime != Time)
									{
										LastTime = Time;
										M_TRACEALWAYS("SoundTime: SoundTime: %f Time: %f Out of sync: %f\n", SoundTime.GetTime(), FrameTime.GetTime(), (SoundTime - FrameTime).GetTime());
									}
								}
								else
								{
									Time = Now - pVideo->m_VideoTimeStart;
								}

								if (Time.Compare(FrameTime) > 0)
									bNewFrame = true;
							}

							if( bNewFrame )
							{
								CTC_WMVTexture::CWMV::CFrame *pOldFrame = pVideo->m_pFrame;
								if (pOldFrame && pOldFrame->m_bFrameShown == 1)
								{
									if(pVideo->m_TextureID[1]) m_pTC->MakeDirty(pVideo->m_TextureID[1]);
									if(pVideo->m_TextureID[2]) m_pTC->MakeDirty(pVideo->m_TextureID[2]);
									break;
								}

								CTC_WMVTexture::CWMV::CFrame *pFrame = pVideo->m_pDecoder->GetFrame(false);

								if (!pFrame)
									break;

								if( pOldFrame )
								{
									pVideo->m_pDecoder->ReturnFrame(pOldFrame);
								}

								pVideo->m_pFrame	= pFrame;

								if( pVideo->m_pFrame )
								{
									pFrame->m_bFrameShown = false;
									pVideo->m_LastFrame = pVideo->m_pFrame->m_iFrame;
									CMTime Gap = Time - FrameTime;

									bFinishedFrame = true;
									/*if (Gap.Compare(pVideo->m_FrameTime * 2) > 0)
									{
									fp32 Slack = Gap.GetTime();

									pVideo->m_pDecoder->m_pSC->Chn_SetPitch(m_iSoundChannel, 0.5);
									//					pVideo->m_pDecoder->PauseSound();
									}
									else if (Gap.Compare(pVideo->m_FrameTime) < 0)
									{
									pVideo->m_pDecoder->m_pSC->Chn_SetPitch(m_iSoundChannel, 1.0);
									//					pVideo->m_pDecoder->ResumeSound();
									}
									*/
									if (Gap.Compare(pVideo->m_FrameTime * 2) > 0)
									{
										fp32 Slack = Gap.GetTime() - pVideo->m_FrameTime * 2;

										fp32 Pitch = (pVideo->m_FrameTime * 10 - Slack) / (pVideo->m_FrameTime * 10);
										Pitch = Max(Pitch, 0.1f);
										Pitch = Min(Pitch, 1.0f);

										if (pVideo->m_pDecoder->m_pSC)
										{
											if (pVideo->m_pDecoder->m_SoundVoices.m_iVoicesOwn[0])
												pVideo->m_pDecoder->m_pSC->Voice_SetPitch(pVideo->m_pDecoder->m_SoundVoices.m_iVoices[0], Pitch);
											else
												pVideo->m_pDecoder->m_pSC->Chn_SetPitch(m_iSoundChannel, Pitch);
										}
										//					pVideo->m_pDecoder->PauseSound();
									}
									else
									{
										if (pVideo->m_pDecoder->m_pSC)
										{
											if (pVideo->m_pDecoder->m_SoundVoices.m_iVoicesOwn[0])
												pVideo->m_pDecoder->m_pSC->Voice_SetPitch(pVideo->m_pDecoder->m_SoundVoices.m_iVoices[0], 1.0);
											else
												pVideo->m_pDecoder->m_pSC->Chn_SetPitch(m_iSoundChannel, 1.0);



										}

									}

									if(pVideo->m_TextureID[0]) m_pTC->MakeDirty(pVideo->m_TextureID[0]);
									if(pVideo->m_TextureID[1]) m_pTC->MakeDirty(pVideo->m_TextureID[1]);
									if(pVideo->m_TextureID[2]) m_pTC->MakeDirty(pVideo->m_TextureID[2]);
								}
							}
							else
								break;
						}
						else
						{
							if (!pVideo->m_bOnLastFrame)
								pVideo->m_bOnLastFrame = 5;
							break;
						}
					}
				}
			}
		}
	}
}

//
//
//
int CTextureContainer_Video_WMV::GetFrame(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_WMV_GetFrame, 0);
	int iLocal = _iLocal / 3;
	ValidateLocalID(iLocal);

	CTC_WMVTexture *pVideo = m_lspVideos[iLocal];
	M_LOCK(pVideo->m_TextureLock);

	if (pVideo)
		return pVideo->m_LastFrame;

	return 0;
}

//
//
//
void CTextureContainer_Video_WMV::Rewind(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_WMV_Rewind, MAUTOSTRIP_VOID);
	MSCOPE(CTextureContainer_Video_WMV::Rewind, TEXTURECONTAINER_WMV);
	int iLocal = _iLocal / 3;
	ValidateLocalID(iLocal);

	CTC_WMVTexture *pVideo = m_lspVideos[iLocal];
	M_LOCK(pVideo->m_TextureLock);

	if(pVideo->IsOpen())
	{
		CloseVideo(pVideo,false);
		OpenVideo(pVideo,false);

		if(pVideo->m_TextureID[0]) m_pTC->MakeDirty(pVideo->m_TextureID[0]);
		if(pVideo->m_TextureID[1]) m_pTC->MakeDirty(pVideo->m_TextureID[1]);
		if(pVideo->m_TextureID[2]) m_pTC->MakeDirty(pVideo->m_TextureID[2]);
	}
	else
	{
		OpenVideo(pVideo);

		if(pVideo->m_TextureID[0]) m_pTC->MakeDirty(pVideo->m_TextureID[0]);
		if(pVideo->m_TextureID[1]) m_pTC->MakeDirty(pVideo->m_TextureID[1]);
		if(pVideo->m_TextureID[2]) m_pTC->MakeDirty(pVideo->m_TextureID[2]);
	}
}

//
//
//
bool CTextureContainer_Video_WMV::IsOnLastFrame(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_WMV_IsOnLastFrame, false);

	int iLocal = _iLocal / 3;

	ValidateLocalID(iLocal);

	CTC_WMVTexture* pVideo = m_lspVideos[iLocal];
	M_LOCK(pVideo->m_TextureLock);

	return pVideo->m_bOnLastFrame != 0;
}


//
//
//
void CTextureContainer_Video_WMV::CloseVideo(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_WMV_CloseVideo, MAUTOSTRIP_VOID);
	int iLocal = _iLocal / 3;

	ValidateLocalID(iLocal);
	CTC_WMVTexture *pVideo = m_lspVideos[iLocal];
	M_LOCK(pVideo->m_TextureLock);
	CloseVideo(pVideo);//->Close();
	if(pVideo->m_TextureID[0]) m_pTC->MakeDirty(pVideo->m_TextureID[0]);
	if(pVideo->m_TextureID[1]) m_pTC->MakeDirty(pVideo->m_TextureID[1]);
	if(pVideo->m_TextureID[2]) m_pTC->MakeDirty(pVideo->m_TextureID[2]);
}

//
//
//
CStr CTextureContainer_Video_WMV::GetName(int _iLocal)
{ 
	MAUTOSTRIP(CTextureContainer_Video_WMV_GetName, CStr());

	int iLocal = _iLocal / 3;
	int iSub = _iLocal - iLocal * 3;

	ValidateLocalID(iLocal);
	CTC_WMVTexture* pVideo = m_lspVideos[iLocal];
	M_LOCK(pVideo->m_TextureLock);
	return pVideo->m_TextureName[iSub];
};

//
//
//
M_INLINE int Clamp(int x, int _Clamp)
{
	if (x < 0)
		return 0;
	return x < _Clamp ? x : _Clamp;
}

void CTextureContainer_Video_WMV::BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _TextureVersion, int _ConvertType, int _iStartMip, uint32 _BuildFlags)
{
	MAUTOSTRIP(CTextureContainer_Video_WMV_BuildInto, MAUTOSTRIP_VOID);
	MSCOPESHORT(CTextureContainer_Video_WMV::BuildInto);

	int iLocal = _iLocal / 3;
	int iSub = _iLocal - iLocal * 3;

	ValidateLocalID(iLocal);

	CTC_WMVTexture *pVideo = m_lspVideos[iLocal];

	M_LOCK(pVideo->m_TextureLock);
	// Update Time
	pVideo->m_TimeLastVisible = CMTime::GetCPU();

	pVideo->m_bWasRendered = true;

	if (!pVideo->IsOpen() && !pVideo->m_bClosed)
	{
		OpenVideo(pVideo);
	}
	if (_BuildFlags & ETCBuildFlags_NewTexture || !pVideo->m_pFrame)
	{
		uint8 *pDest = (uint8*)(*_ppImg)->Lock();
		if( iSub != 2 )
			memset( pDest, 0, (*_ppImg)->GetModulo() * (*_ppImg)->GetHeight());
		else
			memset( pDest, 128, (*_ppImg)->GetModulo() * (*_ppImg)->GetHeight());
		(*_ppImg)->Unlock();
	}

	if( !pVideo->m_pFrame )
	{
		return;
	}


	//	CMTime FrameTime = pVideo->m_pDecoder->GetNextFrameTime();

	//	if (!FrameTime.IsInvalid())
	{
		if (pVideo->m_pFrame)
		{	
			CTC_WMVTexture::CWMV::CFrame *pFrame = pVideo->m_pFrame;

			//			if (pFrame)
			{
				if( iSub == 0 )
				{
					// Drop frame if we are not in sync

					int ImageType = (*_ppImg)->GetFormat();

					switch (ImageType)
					{
					case IMAGE_FORMAT_BGRX8:
					case IMAGE_FORMAT_BGRA8:
#if 1
						//						CMTime Timer;

						{
							//							TMeasure(Timer);
							uint8 *pSrcData = (uint8 *)pFrame->m_Y.Lock();
							uint8 *pSrcDataUV = (uint8 *)pFrame->m_UV.Lock();
							uint8 *pDstData = (uint8 *)(*_ppImg)->Lock();
							int SrcStride = pFrame->m_Y.GetModulo();
							int SrcStrideUV = pFrame->m_UV.GetModulo();
							int DstStride = (*_ppImg)->GetModulo();
							int Width = pVideo->m_pDecoder->m_VideoDesc.dwWidth >> 1;
							int Height = pVideo->m_pDecoder->m_VideoDesc.dwHeight >> 1;

							for (int y = 0; y < Height; ++y)
							{
								uint8 *pScrUV = pSrcDataUV + y * SrcStrideUV;

								uint8 *pScr0 = pSrcData + (y* 2) * SrcStride;
								uint8 *pScr1 = pSrcData + (y*2 + 1) * SrcStride;
								uint8 *pDst0 = pDstData + (y*2) * DstStride;
								uint8 *pDst1 = pDstData + (y*2 + 1) * DstStride;

								for (int x = 0; x < Width; ++x)
								{
									int U = (int)pScrUV[x*2] - 128;
									int V = (int)pScrUV[x*2 + 1] - 128;

									enum 
									{
										EScaleBits = 10,
										EScale = 1 << EScaleBits, 
										EScaleMax = EScale * 255,
										EScale_uvBU = int(EScale * 2.018f),
										EScale_uvGU = int(EScale * 0.391f),
										EScale_uvGV = int(EScale * 0.813f),
										EScale_uvRV = int(EScale * 1.596f),
										EScale_Y = int(EScale * 1.164f),
									};
									int uvB = (U * EScale_uvBU);
									int uvG = -((U * EScale_uvGU))- ((V * EScale_uvGV));
									int uvR = (V * EScale_uvRV);

									int yRGB = (((int)pScr0[x*2] - 16) * EScale_Y);

									pDst0[x*8 + 0] = Clamp(yRGB + uvB, EScaleMax) >> EScaleBits;
									pDst0[x*8 + 1] = Clamp(yRGB + uvG, EScaleMax) >> EScaleBits;
									pDst0[x*8 + 2] = Clamp(yRGB + uvR, EScaleMax) >> EScaleBits;
									pDst0[x*8 + 3] = 0xff;

									yRGB = (((int)pScr0[x*2 + 1] - 16) * EScale_Y);

									pDst0[x*8 + 4] = Clamp(yRGB + uvB, EScaleMax) >> EScaleBits;
									pDst0[x*8 + 5] = Clamp(yRGB + uvG, EScaleMax) >> EScaleBits;
									pDst0[x*8 + 6] = Clamp(yRGB + uvR, EScaleMax) >> EScaleBits;
									pDst0[x*8 + 7] = 0xff;

									yRGB = (((int)pScr1[x*2] - 16) * EScale_Y);

									pDst1[x*8 + 0] = Clamp(yRGB + uvB, EScaleMax) >> EScaleBits;
									pDst1[x*8 + 1] = Clamp(yRGB + uvG, EScaleMax) >> EScaleBits;
									pDst1[x*8 + 2] = Clamp(yRGB + uvR, EScaleMax) >> EScaleBits;
									pDst1[x*8 + 3] = 0xff;

									yRGB = (((int)pScr1[x*2 + 1] - 16) * EScale_Y);

									pDst1[x*8 + 4] = Clamp(yRGB + uvB, EScaleMax) >> EScaleBits;
									pDst1[x*8 + 5] = Clamp(yRGB + uvG, EScaleMax) >> EScaleBits;
									pDst1[x*8 + 6] = Clamp(yRGB + uvR, EScaleMax) >> EScaleBits;
									pDst1[x*8 + 7] = 0xff;
								}
							}

							pFrame->m_UV.Unlock();
							pFrame->m_Y.Unlock();
							(*_ppImg)->Unlock();
						}
						//						M_TRACEALWAYS("Time %f micro seconds\n", Timer.GetTime() * 1000000.0);

#else
						{

							uint8 *pSrcData = (uint8 *)pFrame->m_Y.Lock();
							uint8 *pSrcDataUV = (uint8 *)pFrame->m_UV.Lock();
							CPixel32 *pDstData = (CPixel32 *)(*_ppImg)->Lock();
							int SrcStride = pFrame->m_Y.GetModulo();
							int SrcStrideUV = pFrame->m_UV.GetModulo();
							int DstStride = (*_ppImg)->GetModulo() / 4;
							int Width = m_pDecoder->m_VideoDesc.dwWidth;
							int Height = m_pDecoder->m_VideoDesc.dwHeight;

							for (int y = 0; y < Height; ++y)
							{
								uint8 *pScr = pSrcData + y * SrcStride;
								uint8 *pScrUV = pSrcDataUV + (y >> 1) * SrcStrideUV;
								CPixel32 *pDst = pDstData + y * DstStride;
								for (int x = 0; x < Width; ++x)
								{
									int Y = pScr[x];
									int U = pScrUV[(x & ~1)];
									int V = pScrUV[(x & ~1) + 1];
									int Raw = 1.164f * (Y - 16) + 2.018f * (U - 128);
									pDst[x].B() = Min(Max(Raw, 0), 255);
									Raw = 1.164f * (Y - 16) - 0.813f * (V - 128) - 0.391f * (U - 128);
									pDst[x].G() = Min(Max(Raw, 0), 255);
									Raw = 1.164f * (Y - 16) + 1.596f * (V - 128);
									pDst[x].R() = Min(Max(Raw, 0), 255);
									pDst[x].A() = 0xff;
								}
							}

							pFrame->m_UV.Unlock();
							pFrame->m_Y.Unlock();
							(*_ppImg)->Unlock();
						}
#endif

						pFrame->m_bFrameShown = 2;

						break;
					}		
				}
				else if( iSub == 1 )
				{
					int ImageType = (*_ppImg)->GetFormat();
					if( ImageType == IMAGE_FORMAT_I8 )
					{
						uint8* pSrcData = (uint8*)pFrame->m_Y.Lock();
						if( pSrcData )
						{
							uint8* pDestData = (uint8*)(*_ppImg)->Lock();
							if( pDestData )
							{
								int nSrcMod = pFrame->m_Y.GetModulo();
								int nDestMod = (*_ppImg)->GetModulo();
								if( nSrcMod == nDestMod )
								{
									memcpy( pDestData, pSrcData, pFrame->m_Y.GetSize() );
								}
								else
								{
									int height = Min(pFrame->m_Y.GetHeight(), (*_ppImg)->GetHeight());
									int rowlength = Min( nSrcMod, nDestMod );
									for( int y = 0; y < height; y++ )
									{
										memcpy( pDestData, pSrcData, rowlength );
										pSrcData	+= nSrcMod;
										pDestData	+= nDestMod;
									}
								}
								(*_ppImg)->Unlock();
							}
							pFrame->m_Y.Unlock();
						}
					}
					++pFrame->m_bFrameShown;
				}
				else if( iSub == 2 )
				{
					int ImageType = (*_ppImg)->GetFormat();
					if( ImageType == IMAGE_FORMAT_I8A8 )
					{
						uint8* pSrcData = (uint8*)pFrame->m_UV.Lock();
						if( pSrcData )
						{
							uint8* pDestData = (uint8*)(*_ppImg)->Lock();
							if( pDestData )
							{
								int nSrcMod = pFrame->m_UV.GetModulo();
								int nDestMod = (*_ppImg)->GetModulo();
								if( nSrcMod == nDestMod )
								{
									memcpy( pDestData, pSrcData, pFrame->m_UV.GetSize() );
								}
								else
								{
									int height = Min(pFrame->m_UV.GetHeight(), (*_ppImg)->GetHeight());
									int rowlength = Min( nSrcMod, nDestMod );
									for( int y = 0; y < height; y++ )
									{
										memcpy( pDestData, pSrcData, rowlength );
										pSrcData	+= nSrcMod;
										pDestData	+= nDestMod;
									}
								}
								(*_ppImg)->Unlock();
							}
							pFrame->m_UV.Unlock();
						}
					}
					++pFrame->m_bFrameShown;
				}
				//				else
				{
					// Frame dropped
					//				M_TRACEALWAYS("Frame dropped: %d Time: %f Out of sync: %f\n", pFrame->m_iFrame, pFrame->m_Time, Gap - pVideo->m_FrameTime);
				}

			}

		}
	}
	//	else
	//	{
	//		pVideo->m_bOnLastFrame = true;
	//	}

}

fp32 CTextureContainer_Video_WMV::GetTime(int _iLocal)
{
	int iLocal = _iLocal / 3;
	int iSub = _iLocal - iLocal * 3;
	ValidateLocalID(iLocal);

	CTC_WMVTexture *pVideo = m_lspVideos[iLocal];
	M_LOCK(pVideo->m_TextureLock);
	if (!pVideo->IsOpen())
		return 0;

	return pVideo->m_pDecoder->GetNextFrameTime().GetTime();
	//	return pVideo->m_pDecoder->GetTimeFromStart()/1000.0f;
}

//
// Not Supported functions
//
void CTextureContainer_Video_WMV::AutoRestart(int _iLocal, bool _EnableAutoRestart)
{
}

int CTextureContainer_Video_WMV::GetNumFrames(int _iLocal)
{
	return 1;
}
bool CTextureContainer_Video_WMV::MoveToLastFrame(int _iLocal)
{
	return 0;
}
void CTextureContainer_Video_WMV::SetVolume(int _iLocal, fp32 fpVol)
{}

void CTextureContainer_Video_WMV::Pause(int _iLocal, bool _Paused)
{}

void CTextureContainer_Video_WMV::SetSoundHandle(int _iLocal, int _hSound)
{
	int iLocal = _iLocal / 3;
	int iSub = _iLocal - iLocal * 3;
	ValidateLocalID(iLocal);

	CTC_WMVTexture *pVideo = m_lspVideos[iLocal];
	M_LOCK(pVideo->m_TextureLock);
	if (!pVideo->IsOpen())
	{
		OpenVideo(pVideo);
	}

	if (!pVideo->IsOpen())
		return;

	pVideo->m_pDecoder->SetSound(_hSound);
}

#if _XDK_VER == 4314 

const static mint g_D3DDeviceCommandBufferOffset = 0x00002a3c;
#if (defined(M_RTM) && !defined(M_Profile)) || defined(M_RtmDebug)
const static mint g_D3DDeviceCommandBufferOffsetPresentParams = 0x00003370;
#elif defined(_DEBUG)
const static mint g_D3DDeviceCommandBufferOffsetPresentParams = 0x00003374;
//const static mint g_D3DDeviceCommandBufferOffsetPresentParams = 0x00003734;
#else
const static mint g_D3DDeviceCommandBufferOffsetPresentParams = 0x00003374;
#endif

#elif _XDK_VER == 3529 || _XDK_VER == 4025
const static mint g_D3DDeviceCommandBufferOffset = 0x00002a38;

#if (defined(M_RTM) && !defined(M_Profile)) || defined(M_RtmDebug)
const static mint g_D3DDeviceCommandBufferOffsetPresentParams = 0x00003370;
#elif defined(_DEBUG)
const static mint g_D3DDeviceCommandBufferOffsetPresentParams = 0x00003374;
//const static mint g_D3DDeviceCommandBufferOffsetPresentParams = 0x00003734;
#else
const static mint g_D3DDeviceCommandBufferOffsetPresentParams = 0x00003374;
#endif
#else
#error "Check this for new version of XDK"
		// &((D3D::CDevice *)0)->m_CommandBufferDevice
		// &((D3D::CDevice *)0)->m_PresentParameters	 {BackBufferWidth=??? BackBufferHeight=??? BackBufferFormat=??? ...}	const _D3DPRESENT_PARAMETERS_ *

#endif

//Access to D3D device-create by lazy initialization
__declspec(noinline) D3DDevice * CTextureContainer_Video_WMV::GetDevice()
{
	//Create D3D Device
	if( !m_pDevice )
	{
		Direct3D_CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_COMMAND_BUFFER,NULL,0,NULL,&m_pDevice);

		//Clear command buffer flag
		D3DPRESENT_PARAMETERS *pParams = (D3DPRESENT_PARAMETERS *)((uint8*)m_pDevice + g_D3DDeviceCommandBufferOffsetPresentParams);
		pParams->BackBufferCount = 1;
		pParams->BackBufferWidth = 640;
		pParams->BackBufferHeight = 480;


	}
	if (m_pDevice)
	{
		uint8 *pFlag = (uint8*)m_pDevice + g_D3DDeviceCommandBufferOffset;
		(*pFlag) = false;
	}

	return m_pDevice;
}

void CTextureContainer_Video_WMV::ReturnDevice()
{
//	return;
	//Create D3D Device
	if( m_pDevice )
	{
		uint8 *pFlag = (uint8*)m_pDevice + g_D3DDeviceCommandBufferOffset;
		(*pFlag) = true;
	}
}

void CTextureContainer_Video_WMV::OpenVideo(CTC_WMVTexture * _pVideo,bool _bAffectRef /* = true */)
{
	//If device is not defined, it will get +1 ref automatically
	if( !m_pDevice ) _bAffectRef = false;

	_pVideo->Open(GetDevice());
	ReturnDevice();

	if( !_pVideo->m_pDecoder || !_pVideo->m_pDecoder->m_pPlayer ) _bAffectRef = false;

	/*
	if( _bAffectRef && m_pDevice )
	{
		m_pDevice->AddRef();
	}
	*/
}

void CTextureContainer_Video_WMV::CloseVideo(CTC_WMVTexture * _pVideo,bool _bAffectRef /* = true */)
{
	if( !_pVideo->m_pDecoder || !_pVideo->m_pDecoder->m_pPlayer ) _bAffectRef = false;
	_pVideo->Close();

	/*
	if( _bAffectRef && !m_pDevice->Release() )
	{
		m_pDevice = NULL;
	}
	*/
}

MRTC_IMPLEMENT_DYNAMIC(CTextureContainer_Video_WMV, CTextureContainer_Video);

#endif
