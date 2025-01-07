
#include "PCH.h"

#include "../MSystem.h"

#if defined(PLATFORM_XBOX1)
#include "MTextureContainerXMV.h"
#if defined(_DEBUG)
	#pragma comment(lib, "Xmv.lib")
#else
	#pragma comment(lib, "Xmv.lib")
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTC_XMVTexture
|__________________________________________________________________________________________________
\*************************************************************************************************/
CTC_XMVTexture::CTC_XMVTexture()
{
	MAUTOSTRIP(CTC_XMVTexture_ctor, MAUTOSTRIP_VOID);

	m_TextureID = 0;

	m_VideoDesc.Width = 0;
	m_VideoDesc.Height = 0;
	m_LastFrame = 0;
	m_pDecoder = NULL;
	m_bOnLastFrame = false;
	m_CloseTimeoutDelay = 0;
	m_pInFile = NULL;
	m_bBroken = false;
	m_pPackets[0] = NULL;
	m_pPackets[1] = NULL;
}

//
//
//
CTC_XMVTexture::~CTC_XMVTexture()
{
	MAUTOSTRIP(CTC_XMVTexture_dtor, MAUTOSTRIP_VOID);
	Close();
}

//
//
//
void CTC_XMVTexture::Create(CStr _FileName)
{
	MAUTOSTRIP(CTC_XMVTexture_Create, MAUTOSTRIP_VOID);
	if (!CDiskUtil::FileExists(_FileName))
		Error("Create", "Not a valid file: " + _FileName);

	m_FileName = _FileName;

	m_TextureName = CStrF("*VIDEO_%s", _FileName.GetFilenameNoExt().Str());
	m_VideoDesc.Width = 1;
	m_VideoDesc.Height = 1;
}

//
//
//
bool CTC_XMVTexture::IsOpen()
{
	MAUTOSTRIP(CTC_XMVTexture_IsOpen, false);
	return m_pDecoder != NULL;
}

//
//
//
void CTC_XMVTexture::Close()
{
	MAUTOSTRIP(CTC_XMVTexture_Close, MAUTOSTRIP_VOID);
	MSCOPE(CTC_XMVTexture::Close, SYSTEMCORE_TEX_BINK);

	if (m_pDecoder)
	{
		m_pDecoder->CloseDecoder();
		m_pDecoder = NULL;
	}

	while (!m_PendingRequest.Done())
	{
		try
		{
			m_pInFile->Read(&m_PendingRequest);
		}
		catch (CCExceptionFile)
		{
		}
	}

	if (m_pInFile)
	{
		delete m_pInFile;
		m_pInFile = NULL;
	}

	m_PendingRequest.SetRequest(0,0,0,0);

	if (m_pPackets[0])
	{
		XPhysicalFree(m_pPackets[0]);
		XPhysicalFree(m_pPackets[1]);
		m_pPackets[0] = NULL;
		m_pPackets[1] = NULL;
	}
	m_bOnLastFrame = false;
	m_VideoDesc.Width = 1;
	m_VideoDesc.Height = 1;
}


HRESULT CALLBACK CTC_XMVTexture::GetNextPacket(DWORD _Context, void **_ppPacket, DWORD *_pOffsetToNextPacket)
{

	CTC_XMVTexture *pThis = (CTC_XMVTexture *)_Context;

	if (!pThis->m_PendingRequest.Done())
		pThis->m_pInFile->Read(&pThis->m_PendingRequest);

	if (pThis->m_PendingRequest.Done())
	{
		pThis->m_PendingPacket = pThis->m_DecodingPacket;
		pThis->m_DecodingPacket = pThis->m_LoadingPacket;
		pThis->m_LoadingPacket = -1;
		*_ppPacket = pThis->m_pPackets[pThis->m_DecodingPacket];
		uint32 *PacketSize = (uint32 *)*_ppPacket;
		*_pOffsetToNextPacket = pThis->m_DecodingPacketSize;
		pThis->m_DecodingPacketSize = *PacketSize;
	}
	else
	{
		*_ppPacket = NULL;
		*_pOffsetToNextPacket = 0;
	}
	return S_OK;
}

HRESULT CALLBACK CTC_XMVTexture::ReleasePreviousPacket(DWORD _Context, LONGLONG _NextReadByteOffset, DWORD _NextPacketSize)
{

	CTC_XMVTexture *pThis = (CTC_XMVTexture *)_Context;

	if (_NextPacketSize)
	{
		pThis->m_LoadingPacket = pThis->m_PendingPacket;
		pThis->m_PendingPacket = -1;

		if (_NextPacketSize > pThis->m_MaxPacketSize)
			return 1;
		pThis->m_PendingRequest.SetRequest(pThis->m_pPackets[pThis->m_LoadingPacket], _NextReadByteOffset, _NextPacketSize, false);
		pThis->m_pInFile->Read(&pThis->m_PendingRequest); // Queu up the next packet
	}

	return S_OK;
}

//
//
//
void CTC_XMVTexture::Open()
{
	MAUTOSTRIP(CTC_XMVTexture_Open, MAUTOSTRIP_VOID);
	MSCOPE(CTC_XMVTexture::Open, SYSTEMCORE_TEX_BINK);

	if (m_bBroken)
		return;
	// Close existing
	Close();

	m_pInFile = MNew(CCFile);

	try
	{
		m_pInFile->OpenExt(m_FileName.Str(), CFILE_READ | CFILE_BINARY | CFILE_NODEFERCLOSE, NO_COMPRESSION, NORMAL_COMPRESSION, 0.5, 4, 128*1024);
		uint32 Data[3];
		m_pInFile->Read(Data, sizeof(Data));
		m_pInFile->Seek(0);

		uint32 NextPacketSize = Data[0];
		uint32 ThisPacketSize = Data[1];
		uint32 MaxPacketSize = Data[2];

		m_MaxPacketSize = MaxPacketSize;

		m_pPackets[0] = XPhysicalAlloc(MaxPacketSize, MAXULONG_PTR, 4096, PAGE_READWRITE); // PAGE_WRITECOMBINE
		m_pPackets[1] = XPhysicalAlloc(MaxPacketSize, MAXULONG_PTR, 4096, PAGE_READWRITE); // PAGE_WRITECOMBINE

		m_pInFile->Read(m_pPackets[0], ThisPacketSize);

		m_DecodingPacket = 1;
		m_PendingPacket = -1;
		m_LoadingPacket = 0;
		m_DecodingPacketSize = ThisPacketSize;

		// Create Decoder and check that all went well
		int Err = XMVDecoder_CreateDecoderForPackets(0, m_pPackets[0], (DWORD)this, GetNextPacket, ReleasePreviousPacket, &m_pDecoder);
		if (Err != S_OK)
			return;

	}
	catch (CCExceptionFile)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
		Close();
		m_bBroken = true;
		return;
	}

//	if(XMVDecoder_CreateDecoderForFile(0, m_FileName.Str(), &m_pDecoder) != S_OK)
//		return;

	// Get video description
	m_pDecoder->GetVideoDescriptor(&m_VideoDesc);

	/// Enable audio stream if we have any
	if(m_VideoDesc.AudioStreamCount > 0)
	{

		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		fp32 ValueSfx = pSys->GetOptions()->GetValuef("SND_VOLUMESFX", 1.0f);

		fp64 mB;		// Millibell
		if (ValueSfx >= 1.0f)
		{
			ValueSfx = 1.0f;
			mB = 0;
		}
		else
		{
			if (ValueSfx <= 0.0f)
				mB = -10000.0;
			else
				mB = 2000.0*log10(ValueSfx);
			if (mB < -10000.0) mB = -10000.0;
		}

		int32 lStreams[5] = {-1, -1, -1, -1, -1};
		int32 nStreams = 0;

		if(m_VideoDesc.AudioStreamCount > 1 && (XC_AUDIO_FLAGS_BASIC(XGetAudioFlags()) == XC_AUDIO_FLAGS_SURROUND))
		{
			// surround support
			m_pDecoder->EnableAudioStream(1, 0, NULL, NULL);
			m_pDecoder->EnableAudioStream(2, 0, NULL, NULL);
			m_pDecoder->EnableAudioStream(3, 0, NULL, NULL);
			lStreams[nStreams++] = 1;
			lStreams[nStreams++] = 2;
			lStreams[nStreams++] = 3;
		}
		else
		{
			if ((XC_AUDIO_FLAGS_BASIC(XGetAudioFlags()) == XC_AUDIO_FLAGS_SURROUND))
			{
				DSMIXBINVOLUMEPAIR Temp[6] = {
					{DSMIXBIN_FRONT_LEFT, DSBVOLUME_MAX},   
					{DSMIXBIN_FRONT_RIGHT, DSBVOLUME_MAX},  
					{DSMIXBIN_FXSEND_5, -600}, 
					{DSMIXBIN_FXSEND_5, -600}, 
					{DSMIXBIN_BACK_LEFT, DSBVOLUME_MAX},
					{DSMIXBIN_BACK_RIGHT, DSBVOLUME_MAX}};

				DSMIXBINS MixBins;

				MixBins.dwMixBinCount = 6;
				MixBins.lpMixBinVolumePairs = Temp;

				m_pDecoder->EnableAudioStream(0, 0, &MixBins, NULL);
			}
			else
			{
				m_pDecoder->EnableAudioStream(0, 0, NULL, NULL);
			}
			lStreams[nStreams++] = 0;
		}

		for(int32 i = 0; i < nStreams; i++)
		{
			if(lStreams[i] != -1)
			{
				IDirectSoundStream *pStream;
				m_pDecoder->GetAudioStream(lStreams[i], &pStream);
				if(pStream)
				{
					pStream->SetVolume(mB);
					pStream->Release();
				}
			}
		}
	}

	//
	m_TimeLastVisible = CMTime::GetCPU();
	m_bOnLastFrame = false;
}

//
//
//
void CTC_XMVTexture::MakeValid()
{
	MAUTOSTRIP(CTC_XMVTexture_MakeValid, MAUTOSTRIP_VOID);
	if (!IsOpen())
		Open();
}

//
// Unsupported functions
//
void CTC_XMVTexture::SaveState()
{}
	
void CTC_XMVTexture::RestoreState()
{}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_XMV
|__________________________________________________________________________________________________
\*************************************************************************************************/
CTextureContainer_Video_XMV::CTextureContainer_Video_XMV()
{
	MAUTOSTRIP(CTextureContainer_Video_XMV_ctor, MAUTOSTRIP_VOID);
	m_CloseTimeOut = 0.4f;
}

//
// Frees all textures and videos
//
CTextureContainer_Video_XMV::~CTextureContainer_Video_XMV()
{
	MAUTOSTRIP(CTextureContainer_Video_XMV_dtor, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_lspVideos.Len(); i++)
	{
		m_lspVideos[i]->Close();
		if (m_lspVideos[i]->m_TextureID)
			m_pTC->FreeID(m_lspVideos[i]->m_TextureID);
	}
}

//
// Adds a video to the lsit
//
void CTextureContainer_Video_XMV::Create(void* _pContext)
{
	MAUTOSTRIP(CTextureContainer_Video_XMV_Create, MAUTOSTRIP_VOID);
	m_pTC->EnableTextureClassRefresh(m_iTextureClass);
	
	if (_pContext)
		AddVideo(CStr((const char*)_pContext));
}

//
// Adds a video to the lsit
//
int CTextureContainer_Video_XMV::AddVideo(CStr _FileName)
{
	MAUTOSTRIP(CTextureContainer_Video_XMV_AddVideo, 0);
	MSCOPE(CTextureContainer_Video_XMV::AddVideo, SYSTEMCORE_TEX_BINK);

	// Allocate video
	spCTC_XMVTexture spVideo = MNew(CTC_XMVTexture);
	if (!spVideo)
		MemError("AddVideo");

	// Create and allocate texture id
	spVideo->Create(_FileName);
	spVideo->m_TextureID = m_pTC->AllocID(m_iTextureClass, m_lspVideos.Len(), spVideo->m_TextureName.Str());

	// Add to the list and return the local id
	return m_lspVideos.Add(spVideo);
}

//
// Recursive, checks for extention (not used, AddVideo is called from ScanVideo)
//
void CTextureContainer_Video_XMV::CreateFromDirectory(CStr _Path)
{
	MAUTOSTRIP(CTextureContainer_Video_XMV_CreateFromDirectory, MAUTOSTRIP_VOID);
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
			if (pRec->m_Ext.CompareNoCase("XMV") == 0)
			{
				CStr FileName = _Path + pRec->m_Name;
				try
				{
					AddVideo(FileName);
				}
				catch(CCException)
				{
					LogFile("§cf80WARNING: (CTextureContainer_Video_XMV::CreateFromDirectory) Failure adding video " + FileName);
				}
			}
		}		
	}
}

//
//
//
void CTextureContainer_Video_XMV::ValidateLocalID(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_XMV_ValidateLocalID, MAUTOSTRIP_VOID);
	if(!m_lspVideos.ValidPos(_iLocal))
		Error("ValidateLocalID", CStrF("Invalid local ID %d", _iLocal));
}

//
//
//
int CTextureContainer_Video_XMV::GetNumLocal()
{
	MAUTOSTRIP(CTextureContainer_Video_XMV_GetNumLocal, 0);
	return m_lspVideos.Len();
}

//
//
//
int CTextureContainer_Video_XMV::GetLocal(const char* _pName)
{
	MAUTOSTRIP(CTextureContainer_Video_XMV_GetLocal, 0);
	M_ASSERT(0, "Obsolete");
	return -1;
}

//
//
//
int CTextureContainer_Video_XMV::GetTextureID(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_XMV_GetTextureID, 0);
	ValidateLocalID(_iLocal);
	return m_lspVideos[_iLocal]->m_TextureID;
}

//
//
//
int CTextureContainer_Video_XMV::GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps)
{
	MAUTOSTRIP(CTextureContainer_Video_XMV_GetTextureDesc, 0);
	MSCOPE(CTextureContainer_Video_XMV::GetTextureDesc, SYSTEMCORE_TEX_BINK);

	ValidateLocalID(_iLocal);
	
	if (m_lspVideos[_iLocal]->m_VideoDesc.Width < 2)
		m_lspVideos[_iLocal]->Open();

	_pTargetImg->CreateVirtual(m_lspVideos[_iLocal]->m_VideoDesc.Width, m_lspVideos[_iLocal]->m_VideoDesc.Height, IMAGE_FORMAT_BGRX8, IMAGE_MEM_IMAGE);
	_Ret_nMipmaps = 1;
	return 0;
}

//
// Opens the video if needed
//
int CTextureContainer_Video_XMV::GetWidth(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_XMV_GetWidth, 0);
	ValidateLocalID(_iLocal);
	
	if (m_lspVideos[_iLocal]->m_VideoDesc.Width < 2)
		m_lspVideos[_iLocal]->Open();

	return m_lspVideos[_iLocal]->m_VideoDesc.Width;
}

//
// Opens the video if needed
//
int CTextureContainer_Video_XMV::GetHeight(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_XMV_GetHeight, 0);
	ValidateLocalID(_iLocal);
	
	if (m_lspVideos[_iLocal]->m_VideoDesc.Height < 2)
		m_lspVideos[_iLocal]->Open();

	return m_lspVideos[_iLocal]->m_VideoDesc.Height;
}

//
//
//
void CTextureContainer_Video_XMV::GetTextureProperties(int _iLocal, CTC_TextureProperties& _Properties)
{
	MAUTOSTRIP(CTextureContainer_Video_XMV_GetTextureProperties, MAUTOSTRIP_VOID);
	ValidateLocalID(_iLocal);
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
void CTextureContainer_Video_XMV::OnRefresh()
{
	MAUTOSTRIP(CTextureContainer_Video_XMV_OnRefresh, MAUTOSTRIP_VOID);
	MSCOPE(CTextureContainer_Video_XMV::OnRefresh, SYSTEMCORE_TEX_BINK);
	CMTime Time = CMTime::GetCPU();

	for(int32 i = 0; i < m_lspVideos.Len(); i++)
	{
		CTC_XMVTexture *pVideo = m_lspVideos[i];
		if(pVideo->IsOpen())
		{
			//
			fp32 Time1 = (Time - pVideo->m_TimeLastVisible).GetTime();
			if(Time1 > m_CloseTimeOut)
			{
				if ((pVideo->m_CloseTimeoutDelay++) >= 2)
					pVideo->Close();
			}
			else
			{
				pVideo->m_CloseTimeoutDelay = 0;
			}

			m_pTC->MakeDirty(pVideo->m_TextureID);
		}
	}
}

//
//
//
int CTextureContainer_Video_XMV::GetFrame(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_XMV_GetFrame, 0);
	ValidateLocalID(_iLocal);

	CTC_XMVTexture *pVideo = m_lspVideos[_iLocal];

	if (pVideo)
		return pVideo->m_LastFrame;

	return 0;
}

//
//
//
void CTextureContainer_Video_XMV::Rewind(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_XMV_Rewind, MAUTOSTRIP_VOID);
	MSCOPE(CTextureContainer_Video_XMV::Rewind, SYSTEMCORE_TEX_BINK);
	ValidateLocalID(_iLocal);
	
	CTC_XMVTexture *pVideo = m_lspVideos[_iLocal];
		
	if(pVideo->IsOpen())
	{
		pVideo->Close();
		pVideo->Open();
		m_pTC->MakeDirty(pVideo->m_TextureID);
	}
	else
	{
		pVideo->Open();
		m_pTC->MakeDirty(pVideo->m_TextureID);
	}
}

//
//
//
bool CTextureContainer_Video_XMV::IsOnLastFrame(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_XMV_IsOnLastFrame, false);
	ValidateLocalID(_iLocal);
	
	CTC_XMVTexture* pV = m_lspVideos[_iLocal];
		
	return pV->m_bOnLastFrame;
}


//
//
//
void CTextureContainer_Video_XMV::CloseVideo(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_XMV_CloseVideo, MAUTOSTRIP_VOID);
	ValidateLocalID(_iLocal);
	m_lspVideos[_iLocal]->Close();
}

//
//
//
CStr CTextureContainer_Video_XMV::GetName(int _iLocal)
{ 
	MAUTOSTRIP(CTextureContainer_Video_XMV_GetName, CStr());
	ValidateLocalID(_iLocal);
	return m_lspVideos[_iLocal]->m_TextureName;
};

// We must break the abstraction becouse we need to get the D3DSurface so we can decode the frame direcly into it
#ifdef PLATFORM_XBOX1
	#include "../RndrXbox/MRndrXbox.h"
#endif

//
//
//
void CTextureContainer_Video_XMV::BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _ConvertType, int _iStartMip)
{
	MAUTOSTRIP(CTextureContainer_Video_XMV_BuildInto, MAUTOSTRIP_VOID);
	MSCOPESHORT(CTextureContainer_Video_XMV::BuildInto);

	ValidateLocalID(_iLocal);
	
	CTC_XMVTexture *pVideo = m_lspVideos[_iLocal];

	// Update Time
	pVideo->m_TimeLastVisible = CMTime::GetCPU();

	if (!pVideo->IsOpen())
		return;

	// Get next frame
	XMVRESULT Result;
	uint32 FrameNumber = 0;
	try
	{
		pVideo->m_pDecoder->GetNextFrame((*((CRndrXbox_Image **)_ppImg))->m_Surface.GetRes(), &Result, &FrameNumber);
	}
	catch (CCExceptionFile)
	{
		CDiskUtil::AddCorrupt(DISKUTIL_STATUS_CORRUPTFILE);
		pVideo->Close();
		pVideo->m_bBroken = true;
	}

	if (Result == XMV_NOFRAME && pVideo->m_LastFrame == 0) 
	{
		D3DDevice_BlockUntilVerticalBlank(); // Fix buuuuuuuuuuuuuuuuuuuuggggggggggggggggggyyyyyyyyyyyyyyyyyyy fuckin MS code ??
	}
	// Handle result
	if(Result == XMV_ENDOFFILE)
		pVideo->m_bOnLastFrame = true;
	
	if(Result == XMV_NEWFRAME)
		pVideo->m_LastFrame = FrameNumber;

}

fp32 CTextureContainer_Video_XMV::GetTime(int _iLocal)
{
	ValidateLocalID(_iLocal);
	
	CTC_XMVTexture *pVideo = m_lspVideos[_iLocal];
	if (!pVideo->IsOpen())
		return 0;
	return pVideo->m_pDecoder->GetTimeFromStart()/1000.0f;
}

//
// Not Supported functions
//
void CTextureContainer_Video_XMV::AutoRestart(int _iLocal, bool _EnableAutoRestart)
{}
int CTextureContainer_Video_XMV::GetNumFrames(int _iLocal)
{
	return 1;
}
bool CTextureContainer_Video_XMV::MoveToLastFrame(int _iLocal)
{
	return 0;
}
void CTextureContainer_Video_XMV::SetVolume(int _iLocal, fp32 fpVol)
{}

void CTextureContainer_Video_XMV::Pause(int _iLocal, bool _Paused)
{}

MRTC_IMPLEMENT_DYNAMIC(CTextureContainer_Video_XMV, CTextureContainer_Video);

#endif
