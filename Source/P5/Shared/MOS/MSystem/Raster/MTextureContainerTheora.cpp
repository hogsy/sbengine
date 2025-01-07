
#include "PCH.h"

#include "../MSystem.h"

#if defined(PLATFORM_WIN_PC) || defined(PLATFORM_XBOX)
#include "MTextureContainerTheora.h"
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTC_TheoraTexture
|__________________________________________________________________________________________________
\*************************************************************************************************/

bool CTC_TheoraTexture::CTheora::DoWork()
{
	CFrame *pFrame = GetFreeFrame();
	if (pFrame)
	{
		// Start by reading data
		int bBufferReady = false;
		static CMTime Timer;
		static int nFrames = 0;
		while(1)
		{
			{
				TMeasure(Timer);
				while(!bBufferReady)
				{
					/* theora is one in, one out... */
					if(ogg_stream_packetout(&m_OggStreamState, &m_OggPacket) > 0)
					{

				//		if(m_OggPacket.granulepos >= 0)
				//		{
				//			theora_decode_ctl(m_pTheoraDecodeContext,OC_DECCTL_SET_GRANPOS,&m_OggPacket.granulepos,sizeof(m_OggPacket.granulepos));
				//		}

						int64 Pos;
						theora_decode_packetin(m_pTheoraDecodeContext, &m_OggPacket, &Pos);
#ifdef	CPU_X86
						__asm emms;
#endif

//						int64 Pos = m_pTheoraDecodeContext-> .granulepos;
						fp8 Time = theora_granule_time(m_pTheoraDecodeContext, Pos);
						m_Time = Time;
						pFrame->m_Time = m_SoundOffset + Time;
						if (pFrame->m_Time < m_LastAddedTime)
						{
							pFrame->m_Time = m_LastAddedTime + m_FrameTime;
						}
						pFrame->m_iFrame = m_iCurrentFrame++;
						bBufferReady=1;
					}
					else
						break;
				}
			}
	//		MRTC_SystemInfo::OS_Trace("Decode(%d) %s\n", pFrame->m_iFrame, TString("", Timer).Str());				

			if(!bBufferReady && m_File.EndOfFile())
			{
				m_bEOF = true;
				break;
			}

			if(bBufferReady)
				/* dumpvideo frame, and get new one */
			{
				theora_ycbcr_buffer yuv;
				theora_decode_ycbcr_out(m_pTheoraDecodeContext,yuv);


				//M_ASSERT(yuv.y_stride == pFrame->m_Y.GetWidth() && yuv.y_height == pFrame->m_Y.GetHeight(), "Dimension missmatch");
				//M_ASSERT(yuv.uv_stride == pFrame->m_UV.GetWidth() && yuv.uv_height == pFrame->m_UV.GetHeight(), "Dimension missmatch");

				{
					TMeasure(Timer);

					int PlaneMap[] = {0, 1, 2};
                
					{
						int Width = pFrame->m_Y.GetWidth();
						int Height = pFrame->m_Y.GetHeight();
						void *pData = pFrame->m_Y.Lock();
						int Stride = pFrame->m_Y.GetModulo();
						for (int y = 0; y < Height; ++y)
						{
							uint8 *pY = (uint8 *)yuv[PlaneMap[0]].data + yuv[PlaneMap[0]].ystride * y;
							uint8 *pDest = ((uint8 *)pData) + Stride * y;
							memcpy(pDest, pY, Width);
						}
						pFrame->m_Y.Unlock();
					}
					{
						int Width = pFrame->m_UV.GetWidth();
						int Height = pFrame->m_UV.GetHeight();
						void *pData = pFrame->m_UV.Lock();
						int Stride = pFrame->m_UV.GetModulo();
						for (int y = 0; y < Height; ++y)
						{
							uint8 *pU = (uint8 *)yuv[PlaneMap[1]].data + yuv[PlaneMap[1]].ystride * y;
							uint8 *pV = (uint8 *)yuv[PlaneMap[2]].data + yuv[PlaneMap[2]].ystride * y;
							uint8 *pUV = ((uint8 *)pData) + Stride * y;

							for (int x = 0; x < (Width >> 1); ++x)
							{

#ifdef CPU_LITTLEENDIAN
//								uint16 u = ((uint16 *)pU)[x];
//								uint16 v = ((uint16 *)pV)[x];
//								((uint32 *)pUV)[x] = (u&0xff00) << 8 | (u&0xff) >> 0 | (v&0xff00) << 16 | (v&0xff) << 8;

								((uint32 *)pUV)[x] = pU[x*2] << 0 | pV[x*2] << 8 | pU[x*2+1] << 16 | pV[x*2+1] << 24;

/*								uint32 u = ((uint16 *)pU)[x];
								uint32 v = ((uint16 *)pV)[x];
								((uint32 *)pUV)[x*2]	= (u&0xff000000) >> 8 | (u&0x00ff0000) >> 16			| (v&0xff000000) << 0| (v&0x00ff0000) >> 8;
								((uint32 *)pUV)[x*2+1]	= (u&0xff00) << 8 | (u&0xff) >> 0			| (v&0xff00) << 16 | (v&0xff) << 8;*/

//								((uint16 *)pUV)[x] = pU[x] << 8 | pV[x];
								
/*								uint32 Forced; 
								uint8 *pForced = (uint8 *)&Forced;
								pForced[0] = pU[x*2];
								pForced[1] = pV[x*2];
								pForced[2] = pU[x*2+1];
								pForced[3] = pV[x*2+1];
								((uint32 *)pUV)[x] = Forced;

//								if (((uint32 *)pUV)[x] != Forced)
//									M_BREAKPOINT;*/
#else
								((uint32 *)pUV)[x] = pU[x*2] << 0 | pV[x*2] << 8 | pU[x*2+1] << 16 | pV[x*2+1] << 24;
//								((uint32 *)pUV)[x] = pU[x*2] << 8 | pV[x*2] << 0 | pU[x*2+1] << 24 | pV[x*2+1] << 16;
//								((uint32 *)pUV)[x] = pU[x*2] << 0 | pV[x*2] << 8 | pU[x*2+1] << 16 | pV[x*2+1] << 24;
//								((uint32 *)pUV)[x] = 0x7f7f7f7f;
//								((uint32 *)pUV)[x] = pU[x*2] << 24 | pV[x*2] << 16| pU[x*2+1] << 8 | pV[x*2+1] << 0;
//#error "Implement this"
#endif
//								pUV[x*2] = pU[x];
//								pUV[x*2+1] = pV[x];
							}
						}
						pFrame->m_UV.Unlock();
					}
				}

//				MRTC_SystemInfo::OS_Trace("Convert(%d) %s\n", pFrame->m_iFrame, TString("", Timer).Str());				
//				Timer.Reset();

				break;
			}
			else
			{
				/* no data yet for somebody.  Grab another page */
				int ret=BufferData();
				while(ogg_sync_pageout(&m_OggState, &m_OggPage) > 0)
					QueuePage();
			}
		}

		if (m_bEOF)
		{
			MRTC_SystemInfo::OS_Trace("%s %d frames (%f fps)\n", TString("Theora time", Timer).Str(), nFrames + 1, 1.0 / (Timer.GetTime() / fp4(nFrames +1)));				
			nFrames = 0;
			Timer.Reset();
		}

		if (!bBufferReady)
		{
			ReturnFrame(pFrame);
			return true;
		}


		++nFrames;
		AddFinisherFrame(pFrame);
		return true;
	}

	return false;
}

#ifdef PLATFORM_XENON
#include "XTL.h"
#endif

int CTC_TheoraTexture::CTheora::Thread_Main()
{
	MRTC_SystemInfo::Thread_SetName("Threora unpacker");
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

CTC_TheoraTexture::CTheora::CFrame *CTC_TheoraTexture::CTheora::GetFreeFrame()
{
	M_LOCK(m_Lock);
	for (int i = 0; i < ENumFrames; ++i)
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

CMTime CTC_TheoraTexture::CTheora::GetSoundTime()
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


CMTime CTC_TheoraTexture::CTheora::GetNextFrameTime()
{
	M_LOCK(m_Lock);

	if (m_pFinishedFrames[m_iFinishedConsume])
	{
		fp8 Scaled = (m_pFinishedFrames[m_iFinishedConsume]->m_Time / m_OriginalFrameTime) * m_FrameTime;
		int64 Ticks = Scaled;
		fp4 Fraction = Scaled - Ticks;

		return CMTime::CreateFromTicks(Ticks, 1, Fraction);
	}
	else if (m_bEOF)
		return CMTime::CreateInvalid();
	else			
	{
		fp8 Scaled = (m_LastAddedTime + m_FrameTime);
		int64 Ticks = Scaled;
		fp4 Fraction = Scaled - Ticks;

		return CMTime::CreateFromTicks(Ticks, 1, Fraction);
	}
}

void CTC_TheoraTexture::CTheora::AddFinisherFrame(CFrame *_pFrame)
{
	M_LOCK(m_Lock);
	M_ASSERT(!m_pFinishedFrames[m_iFinishedProduce], "Should not be anything in here");
	m_pFinishedFrames[m_iFinishedProduce] = _pFrame;
	++m_iFinishedProduce;
	if (m_iFinishedProduce >= ENumFrames)
		m_iFinishedProduce = 0;

	m_LastAddedTime = (_pFrame->m_Time / m_OriginalFrameTime) * m_FrameTime;

	m_WorkDoneEvent.Signal();
}

CTC_TheoraTexture::CTheora::CFrame *CTC_TheoraTexture::CTheora::GetFrame(bool _bBlock)
{
	M_LOCK(m_Lock);
	m_WorkDoneEvent.WaitTimeout(0.0);
	CFrame *pFrame = m_pFinishedFrames[m_iFinishedConsume];
	if (pFrame)
	{
		m_pFinishedFrames[m_iFinishedConsume] = NULL;
		++m_iFinishedConsume;
		if (m_iFinishedConsume >= ENumFrames)
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
		if (m_iFinishedConsume >= ENumFrames)
			m_iFinishedConsume = 0;
		return pFrame;
	}

	return 0;
}

void CTC_TheoraTexture::CTheora::ReturnFrame(CFrame *_pFrame)
{
	M_LOCK(m_Lock);
	CFrame *pFrame = _pFrame;

	for (int i = 0; i < ENumFrames; ++i)
	{
		if (!m_pFreeFrames[i])
		{
			m_pFreeFrames[i] = pFrame;
			m_WorkEvent.Signal();
			return;
		}
	}
}
/*
void CTC_TheoraTexture::CTheora::ReturnFrame()
{
	M_LOCK(m_Lock);
	ReturnFrame(m_pFinishedFrames[m_iFinishedConsume]);
	m_pFinishedFrames[m_iFinishedConsume] = NULL;
	++m_iFinishedConsume;
	if (m_iFinishedConsume >= ENumFrames)
		m_iFinishedConsume = 0;
}
*/
int CTC_TheoraTexture::CTheora::BufferData()
{
	char *pBuffer=ogg_sync_buffer(&m_OggState,16384);
	int Bytes = Min(16384, (int)(m_File.Length() - m_File.Pos()));
	m_File.Read(pBuffer, Bytes);
	ogg_sync_wrote(&m_OggState,Bytes);
	return(Bytes);
}

void CTC_TheoraTexture::CTheora::QueuePage()
{
	if (m_TheoraPage)
		ogg_stream_pagein(&m_OggStreamState, &m_OggPage);
}

bool CTC_TheoraTexture::CTheora::Init(CSoundContext *_pSC)
{
	ogg_sync_init(&m_OggState);
	theora_comment_init(&m_TheoraComment);
	theora_info_init(&m_TheoraInfo);
	m_TheoraPage = 0;

	int bProcessingHeaders = true;
	theora_setup_info	*pTheoraSetupInfo = NULL;


	int bState = 0;
	while (!bState)
	{
		int Ret = BufferData();
		if (!Ret)
			break;

		while(ogg_sync_pageout(&m_OggState,&m_OggPage) > 0)
		{
			ogg_stream_state test;

			/* is this a mandated initial header? If not, stop parsing */
			if(!ogg_page_bos(&m_OggPage))
			{
				/* don't leak the page; get it into the appropriate stream */
				QueuePage();
				bState=1;
				break;
			}

			ogg_stream_init(&test,ogg_page_serialno(&m_OggPage));
			ogg_stream_pagein(&test,&m_OggPage);
			ogg_stream_packetpeek(&test,&m_OggPacket);

			/* identify the codec: try theora */
			bProcessingHeaders = theora_decode_headerin(&m_TheoraInfo,&m_TheoraComment,&pTheoraSetupInfo,&m_OggPacket);
			if(!m_TheoraPage && bProcessingHeaders >= 0)
			{
				/* it is theora */
				memcpy(&m_OggStreamState,&test,sizeof(test));
				m_TheoraPage=1;
				if(bProcessingHeaders)
					ogg_stream_packetout(&m_OggStreamState,NULL);
 			}
			else
			{
				/* whatever it is, we don't care about it */
				ogg_stream_clear(&test);
			}
		}
		/* fall through to non-bos page parsing */
	}

	/* we're expecting more header packets. */
	while (m_TheoraPage && bProcessingHeaders)
	{
		int ret;

		/* look for further theora headers */
		while(bProcessingHeaders && (ret = ogg_stream_packetpeek(&m_OggStreamState,&m_OggPacket)))
		{
			if (ret<0)
				continue;
			/*
			{
				M_TRACEALWAYS("Error parsing Theora stream headers; corrupt stream?\n");
				theora_info_clear(&m_TheoraInfo);
				theora_comment_clear(&m_TheoraComment);
				ogg_sync_clear(&m_OggState);
				return false;
			}*/
		    bProcessingHeaders = theora_decode_headerin(&m_TheoraInfo,&m_TheoraComment,&pTheoraSetupInfo,&m_OggPacket);
			if(bProcessingHeaders<0)
			{
				M_TRACEALWAYS("Error parsing Theora stream headers; corrupt stream?\n");
				theora_info_clear(&m_TheoraInfo);
				theora_comment_clear(&m_TheoraComment);
				ogg_sync_clear(&m_OggState);
				return false;
			}
			else if (bProcessingHeaders > 0)
			{
				/*Advance past the successfully processed header.*/
				ogg_stream_packetout(&m_OggStreamState,NULL);
			}
			m_TheoraPage++;
//			if (m_TheoraPage == 3)
//				break;
		}

		/*Stop now so we don't fail if there aren't enough pages in a short
		stream.*/
		if(!(m_TheoraPage&&bProcessingHeaders))
			break;

		/* The header pages/packets will arrive before anything else we
		care about, or the stream is not obeying spec */

		if(ogg_sync_pageout(&m_OggState,&m_OggPage)>0)
		{
			QueuePage(); /* demux into the appropriate stream */
		}
		else
		{
			int ret=BufferData(); /* someone needs more data */
			if (ret == 0)
			{
				M_TRACEALWAYS("End of file while searching for codec headers.\n");
				theora_info_clear(&m_TheoraInfo);
				theora_comment_clear(&m_TheoraComment);
				ogg_sync_clear(&m_OggState);
				return false;
			}
		}
	}

	/* and now we have it all.  initialize decoders */
	if(m_TheoraPage)
	{
		m_pTheoraDecodeContext = theora_decode_alloc(&m_TheoraInfo, pTheoraSetupInfo);
		M_TRACEALWAYS("Ogg logical stream %x is Theora %dx%d %.02f fps video\nEncoded frame content is %dx%d with %dx%d offset\n",
			m_OggStreamState.serialno, m_TheoraInfo.pic_width,m_TheoraInfo.pic_height, (double)m_TheoraInfo.fps_numerator/m_TheoraInfo.fps_denominator,
			m_TheoraInfo.frame_width, m_TheoraInfo.frame_height, m_TheoraInfo.pic_x, m_TheoraInfo.pic_y);
		theora_setup_free(pTheoraSetupInfo);
	}
	else
	{
		theora_setup_free(pTheoraSetupInfo);
		/* tear down the partial theora setup */
		theora_info_clear(&m_TheoraInfo);
		theora_comment_clear(&m_TheoraComment);
		ogg_sync_clear(&m_OggState);

		return false;
	}

	// queue any remaining pages from data we buffered but that did not contain headers
	while(ogg_sync_pageout(&m_OggState, &m_OggPage) > 0)
	{
		QueuePage();
	}

	int Level = 0;
	theora_decode_ctl(m_pTheoraDecodeContext,OC_DECCTL_SET_PPLEVEL,&Level,sizeof(Level));



	m_bEOF = false;
	m_iFinishedProduce = 0;
	m_iFinishedConsume = 0;
	m_Time = 0;

	m_OriginalFrameTime = m_FrameTime = (fp8)m_TheoraInfo.fps_denominator / (fp8)m_TheoraInfo.fps_numerator;


	CStr FileName = m_File.GetFileName();
	CStr SetFile = FileName.GetPath() + FileName.GetFilenameNoExt() + ".set";
	m_SoundOffset = 0;
	if (CDiskUtil::FileExists(SetFile))
	{
		CRegistry_Dynamic Temp;
		Temp.ReadSimple(SetFile);
		m_FrameTime = 1.0/Temp.GetValuef("FRAMERATE", 1.0/m_FrameTime);
		m_SoundOffset = Temp.GetValuef("SOUNDOFFSET", 0.0);
	}

	m_LastAddedTime = m_SoundOffset + -m_FrameTime;
	m_iCurrentFrame = 0;

	for (int i = 0; i < ENumFrames; ++i)
	{
		m_pFreeFrames[i] = DNew(CFrame) CFrame;
		m_pFinishedFrames[i] = NULL;
		m_pFreeFrames[i]->m_Y.Create(m_TheoraInfo.pic_width, m_TheoraInfo.pic_height, IMAGE_FORMAT_I8, IMAGE_MEM_SYSTEM);
		m_pFreeFrames[i]->m_UV.Create(m_TheoraInfo.pic_width/2, m_TheoraInfo.pic_height/2, IMAGE_FORMAT_I8A8, IMAGE_MEM_SYSTEM);
	}
	m_bIsOpen = true;
	Thread_Create(NULL, 16384, MRTC_THREAD_PRIO_ABOVENORMAL);

	m_pSC = _pSC;

	return true;
}

void CTC_TheoraTexture::CTheora::Cleanup()
{
	if (m_bIsOpen)
	{
		m_bIsOpen = false;
		m_WorkEvent.Signal();
		Thread_Destroy();
		ogg_stream_clear(&m_OggStreamState);
	    theora_decode_free(m_pTheoraDecodeContext);
		theora_comment_clear(&m_TheoraComment);
		theora_info_clear(&m_TheoraInfo);

		ogg_sync_clear(&m_OggState);
		int nDeleted = 0;

		for (int i = 0; i < ENumFrames; ++i)
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
		M_ASSERT(nDeleted == ENumFrames, "");

		if (m_pSC)
			m_SoundVoices.Destroy(m_pSC);

	}		
}

void CTC_TheoraTexture::CTheora::ResumeSound()
{
	if (m_bPausedSound)
	{
		if (m_pSC)
		{
			for (int i = 0; i < 6; ++i)
			{
				if (m_SoundVoices.m_iVoices[i] >= 0)
				{
					m_pSC->Voice_Play(m_SoundVoices.m_iVoices[i]);
				}
			}
		}

		m_bPausedSound = false;
	}
}

void CTC_TheoraTexture::CTheora::PauseSound()
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

void CTC_TheoraTexture::CTheora::SetSound(int _ihSound)
{
	m_SoundVoices.Destroy(m_pSC);
	m_SoundVoices.m_iVoices[0] = _ihSound;
	m_SoundVoices.m_iVoicesOwn[0] = false;
	m_bPausedSound = false;
}

void CTC_TheoraTexture::CTheora::PlaySound(int _iChannel)
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

				fp4 Vol = 1.0;
				if (pSys)
					Vol = pSys->GetRegistry()->GetValuef("SND_VIDEO_VOLUME", 1.0);

				m_SoundVoices.Destroy(m_pSC);
                m_SoundVoices.m_iVoices[0] = m_pSC->Voice_Create(_iChannel, WaveId, 0, 1.0, Vol);
				m_bPausedSound = false;
			}
		}
	}
}


CTC_TheoraTexture::CTheora::CTheora()
{
	m_bIsOpen = false;
}

CTC_TheoraTexture::CTheora::~CTheora()
{
	Cleanup();

}

CTC_TheoraTexture::CTC_TheoraTexture()
{
	MAUTOSTRIP(CTC_TheoraTexture_ctor, MAUTOSTRIP_VOID);

	m_TextureID[0] = 0;
	m_TextureID[1] = 0;
	m_TextureID[2] = 0;
	m_pDecoder = 0;
	m_pFrame	= 0;
	m_LastFrame = 0;
	m_bOnLastFrame = false;
	m_CloseTimeoutDelay = 0;
	m_bBroken = false;
	m_bWasRendered = false;

}

//
//
//
CTC_TheoraTexture::~CTC_TheoraTexture()
{
	MAUTOSTRIP(CTC_TheoraTexture_dtor, MAUTOSTRIP_VOID);
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
void CTC_TheoraTexture::Create(CStr _FileName)
{
	MAUTOSTRIP(CTC_TheoraTexture_Create, MAUTOSTRIP_VOID);
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
bool CTC_TheoraTexture::IsOpen()
{
	MAUTOSTRIP(CTC_TheoraTexture_IsOpen, false);
	return m_pDecoder != NULL;
}

//
//
//
void CTC_TheoraTexture::Close()
{
	MAUTOSTRIP(CTC_TheoraTexture_Close, MAUTOSTRIP_VOID);
	MSCOPE(CTC_TheoraTexture::Close, SYSTEMCORE_TEX_BINK);

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

	m_bOnLastFrame = false;
}


//
//
//
void CTC_TheoraTexture::Open()
{
	MAUTOSTRIP(CTC_TheoraTexture_Open, MAUTOSTRIP_VOID);
	MSCOPE(CTC_TheoraTexture::Open, SYSTEMCORE_TEX_BINK);

	if (m_bBroken)
		return;
	// Close existing
	Close();

	M_TRY
	{
		m_pDecoder = DNew(CTheora) CTheora;
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

	if (!m_pDecoder->Init(pSound))
	{
		Close();
		m_bBroken = true;
		return;
	}
	m_pDecoder->m_pTexture = this;

#ifdef PLATFORM_XENON
	m_TextureWidth = m_pDecoder->m_TheoraInfo.frame_width;
	m_TextureHeight = m_pDecoder->m_TheoraInfo.frame_height;
#else
	m_TextureWidth = GetGEPow2(m_pDecoder->m_TheoraInfo.frame_width);
	m_TextureHeight = GetGEPow2(m_pDecoder->m_TheoraInfo.frame_height);
#endif
	m_FrameTime = m_pDecoder->m_FrameTime;

	//
	m_TimeLastVisible = CMTime::GetCPU();
	m_bOnLastFrame = false;
	m_CloseTimeoutDelay	= 0;
}

//
//
//
void CTC_TheoraTexture::MakeValid()
{
	MAUTOSTRIP(CTC_TheoraTexture_MakeValid, MAUTOSTRIP_VOID);
	if (!IsOpen())
		Open();
}

//
// Unsupported functions
//
void CTC_TheoraTexture::SaveState()
{
}
	
void CTC_TheoraTexture::RestoreState()
{
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_XMV
|__________________________________________________________________________________________________
\*************************************************************************************************/
CTextureContainer_Video_Theora::CTextureContainer_Video_Theora()
{
	MAUTOSTRIP(CTextureContainer_Video_Theora_ctor, MAUTOSTRIP_VOID);
	m_CloseTimeOut = 0.5f;
	m_iSoundChannel = -1;
}

//
// Frees all textures and videos
//
CTextureContainer_Video_Theora::~CTextureContainer_Video_Theora()
{
	MAUTOSTRIP(CTextureContainer_Video_Theora_dtor, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_lspVideos.Len(); i++)
	{
		m_lspVideos[i]->Close();
		for( int j = 0; j < 3; j++ )
		{
			if(m_lspVideos[i]->m_TextureID[j])
			{
				m_pTC->FreeID(m_lspVideos[i]->m_TextureID[j]);
			}
		}
	}

	MACRO_GetRegisterObject(CSoundContext, pSound, "SYSTEM.SOUND");
	if (pSound && m_iSoundChannel >= 0)
	{
		pSound->Chn_Free(m_iSoundChannel);
	}

}

//
// Adds a video to the lsit
//
void CTextureContainer_Video_Theora::Create(void* _pContext)
{
	MAUTOSTRIP(CTextureContainer_Video_Theora_Create, MAUTOSTRIP_VOID);
	m_pTC->EnableTextureClassRefresh(m_iTextureClass);

	if (_pContext)
		AddVideo(CStr((const char*)_pContext));
}

//
// Adds a video to the lsit
//
int CTextureContainer_Video_Theora::AddVideo(CStr _FileName)
{
	MAUTOSTRIP(CTextureContainer_Video_Theora_AddVideo, 0);
	MSCOPE(CTextureContainer_Video_Theora::AddVideo, SYSTEMCORE_TEX_BINK);

	// Allocate video
	spCTC_TheoraTexture spVideo = MNew(CTC_TheoraTexture);
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
// Recursive, checks for extention (not used, AddVideo is called from ScanVideo)
//
void CTextureContainer_Video_Theora::CreateFromDirectory(CStr _Path)
{
	MAUTOSTRIP(CTextureContainer_Video_Theora_CreateFromDirectory, MAUTOSTRIP_VOID);
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
			if (pRec->m_Ext.CompareNoCase("ogg") == 0)
			{
				CStr FileName = _Path + pRec->m_Name;
				M_TRY
				{
					AddVideo(FileName);
				}
				M_CATCH(
				catch(CCException)
				{
					LogFile("§cf80WARNING: (CTextureContainer_Video_Theora::CreateFromDirectory) Failure adding video " + FileName);
				}
				)
			}
		}		
	}
}

//
//
//
void CTextureContainer_Video_Theora::ValidateLocalID(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_Theora_ValidateLocalID, MAUTOSTRIP_VOID);
	if(!m_lspVideos.ValidPos(_iLocal))
		Error("ValidateLocalID", CStrF("Invalid local ID %d", _iLocal));
}

//
//
//
int CTextureContainer_Video_Theora::GetNumLocal()
{
	MAUTOSTRIP(CTextureContainer_Video_Theora_GetNumLocal, 0);
	return m_lspVideos.Len();
}

//
//
//
int CTextureContainer_Video_Theora::GetLocal(const char* _pName)
{
	MAUTOSTRIP(CTextureContainer_Video_Theora_GetLocal, 0);
	M_ASSERT(0, "Obsolete");
	return -1;
}

//
//
//
int CTextureContainer_Video_Theora::GetTextureID(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_Theora_GetTextureID, 0);
	int iLocal = _iLocal / 3;
	int iSub = _iLocal - iLocal * 3;
	ValidateLocalID(iLocal);
	CTC_TheoraTexture* pVideo = m_lspVideos[iLocal];
	M_LOCK(pVideo->m_TextureLock);
	return pVideo->m_TextureID[iSub];
}

//
//
//
int CTextureContainer_Video_Theora::GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps)
{
	MAUTOSTRIP(CTextureContainer_Video_Theora_GetTextureDesc, 0);
	MSCOPE(CTextureContainer_Video_Theora::GetTextureDesc, SYSTEMCORE_TEX_BINK);

	int iLocal = _iLocal / 3;
	int iSub = _iLocal - iLocal * 3;
	ValidateLocalID(iLocal);
	CTC_TheoraTexture *pVideo = m_lspVideos[iLocal];

	M_LOCK(pVideo->m_TextureLock);
	
	if (!pVideo->IsOpen())
		pVideo->Open();
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
int CTextureContainer_Video_Theora::GetWidth(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_Theora_GetWidth, 0);
	int iLocal = _iLocal / 3;
	ValidateLocalID(iLocal);

	CTC_TheoraTexture *pVideo = m_lspVideos[iLocal];
	M_LOCK(pVideo->m_TextureLock);
	
	if (!pVideo->IsOpen())
		pVideo->Open();
	if (!pVideo->IsOpen())
		return 0;

	return pVideo->m_pDecoder->m_TheoraInfo.frame_width;
}

//
// Opens the video if needed
//
int CTextureContainer_Video_Theora::GetHeight(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_Theora_GetHeight, 0);
	int iLocal = _iLocal / 3;
	ValidateLocalID(iLocal);
	
	CTC_TheoraTexture *pVideo = m_lspVideos[iLocal];
	M_LOCK(pVideo->m_TextureLock);
	if (!pVideo->IsOpen())
		pVideo->Open();
	if (!pVideo->IsOpen())
		return 0;

	return pVideo->m_pDecoder->m_TheoraInfo.frame_height;
}

//
//
//
void CTextureContainer_Video_Theora::GetTextureProperties(int _iLocal, CTC_TextureProperties& _Properties)
{
	MAUTOSTRIP(CTextureContainer_Video_Theora_GetTextureProperties, MAUTOSTRIP_VOID);
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
void CTextureContainer_Video_Theora::OnRefresh()
{

	if (m_iSoundChannel < 0)
	{
		MACRO_GetRegisterObject(CSoundContext, pSound, "SYSTEM.SOUND");
		if (pSound)
			m_iSoundChannel = pSound->Chn_Alloc(1);
	}

	MAUTOSTRIP(CTextureContainer_Video_Theora_OnRefresh, MAUTOSTRIP_VOID);
	MSCOPE(CTextureContainer_Video_Theora::OnRefresh, SYSTEMCORE_TEX_BINK);
	CMTime Time = CMTime::GetCPU();
	CMTime Now = Time;

	for(int32 i = 0; i < m_lspVideos.Len(); i++)
	{
		CTC_TheoraTexture *pVideo = m_lspVideos[i];
		M_LOCK(pVideo->m_TextureLock);

		if (pVideo->m_bOnLastFrame > 1)
			--pVideo->m_bOnLastFrame;

		if(pVideo->IsOpen())
		{
			//
			fp4 Time1 = (Time - pVideo->m_TimeLastVisible).GetTime();
			// Only autoclose video if no sound is playing on it
			if(pVideo->m_pDecoder->m_SoundVoices.m_iVoicesOwn[0] && Time1 > m_CloseTimeOut && pVideo->m_pFrame && !pVideo->m_pFrame->m_bFrameShown)// || !pVideo->m_bWasRendered)
			{
				if ((pVideo->m_CloseTimeoutDelay++) >= 5)
				{
					pVideo->Close();
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
						pVideo->Close();
						if(pVideo->m_TextureID[0]) m_pTC->MakeDirty(pVideo->m_TextureID[0]);
						if(pVideo->m_TextureID[1]) m_pTC->MakeDirty(pVideo->m_TextureID[1]);
						if(pVideo->m_TextureID[2]) m_pTC->MakeDirty(pVideo->m_TextureID[2]);
						bClosed = true;
					}					
				}

				if (!bClosed)
				{
					pVideo->m_CloseTimeoutDelay = 0;

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
								CTC_TheoraTexture::CTheora::CFrame *pOldFrame = pVideo->m_pFrame;
								if (pOldFrame && pOldFrame->m_bFrameShown == 1)
									break;

								CTC_TheoraTexture::CTheora::CFrame *pFrame = pVideo->m_pDecoder->GetFrame(false);

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
										fp4 Slack = Gap.GetTime();

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
										fp4 Slack = Gap.GetTime() - pVideo->m_FrameTime * 2;

										fp4 Pitch = (pVideo->m_FrameTime * 10 - Slack) / (pVideo->m_FrameTime * 10);
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
int CTextureContainer_Video_Theora::GetFrame(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_Theora_GetFrame, 0);
	int iLocal = _iLocal / 3;
	ValidateLocalID(iLocal);

	CTC_TheoraTexture *pVideo = m_lspVideos[iLocal];
	M_LOCK(pVideo->m_TextureLock);

	if (pVideo)
		return pVideo->m_LastFrame;

	return 0;
}

//
//
//
void CTextureContainer_Video_Theora::Rewind(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_Theora_Rewind, MAUTOSTRIP_VOID);
	MSCOPE(CTextureContainer_Video_Theora::Rewind, SYSTEMCORE_TEX_BINK);
	int iLocal = _iLocal / 3;
	ValidateLocalID(iLocal);
	
	CTC_TheoraTexture *pVideo = m_lspVideos[iLocal];
	M_LOCK(pVideo->m_TextureLock);
		
	if(pVideo->IsOpen())
	{
		pVideo->Close();
		pVideo->Open();
		if(pVideo->m_TextureID[0]) m_pTC->MakeDirty(pVideo->m_TextureID[0]);
		if(pVideo->m_TextureID[1]) m_pTC->MakeDirty(pVideo->m_TextureID[1]);
		if(pVideo->m_TextureID[2]) m_pTC->MakeDirty(pVideo->m_TextureID[2]);
	}
	else
	{
		pVideo->Open();
		if(pVideo->m_TextureID[0]) m_pTC->MakeDirty(pVideo->m_TextureID[0]);
		if(pVideo->m_TextureID[1]) m_pTC->MakeDirty(pVideo->m_TextureID[1]);
		if(pVideo->m_TextureID[2]) m_pTC->MakeDirty(pVideo->m_TextureID[2]);
	}
}

//
//
//
bool CTextureContainer_Video_Theora::IsOnLastFrame(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_Theora_IsOnLastFrame, false);

	int iLocal = _iLocal / 3;

	ValidateLocalID(iLocal);
	
	CTC_TheoraTexture* pVideo = m_lspVideos[iLocal];
	M_LOCK(pVideo->m_TextureLock);
		
	return pVideo->m_bOnLastFrame == 1;
}


//
//
//
void CTextureContainer_Video_Theora::CloseVideo(int _iLocal)
{
	MAUTOSTRIP(CTextureContainer_Video_Theora_CloseVideo, MAUTOSTRIP_VOID);
	int iLocal = _iLocal / 3;

	ValidateLocalID(iLocal);
	CTC_TheoraTexture *pVideo = m_lspVideos[iLocal];
	M_LOCK(pVideo->m_TextureLock);
	pVideo->Close();
	if(pVideo->m_TextureID[0]) m_pTC->MakeDirty(pVideo->m_TextureID[0]);
	if(pVideo->m_TextureID[1]) m_pTC->MakeDirty(pVideo->m_TextureID[1]);
	if(pVideo->m_TextureID[2]) m_pTC->MakeDirty(pVideo->m_TextureID[2]);
}

//
//
//
CStr CTextureContainer_Video_Theora::GetName(int _iLocal)
{ 
	MAUTOSTRIP(CTextureContainer_Video_Theora_GetName, CStr());

	int iLocal = _iLocal / 3;
	int iSub = _iLocal - iLocal * 3;

	ValidateLocalID(iLocal);
	CTC_TheoraTexture* pVideo = m_lspVideos[iLocal];
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

void CTextureContainer_Video_Theora::BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _TextureVersion, int _ConvertType, int _iStartMip, uint32 _BuildFlags)
{
	MAUTOSTRIP(CTextureContainer_Video_Theora_BuildInto, MAUTOSTRIP_VOID);
	MSCOPESHORT(CTextureContainer_Video_Theora::BuildInto);

	int iLocal = _iLocal / 3;
	int iSub = _iLocal - iLocal * 3;

	ValidateLocalID(iLocal);
	
	CTC_TheoraTexture *pVideo = m_lspVideos[iLocal];

	M_LOCK(pVideo->m_TextureLock);
	// Update Time
	pVideo->m_TimeLastVisible = CMTime::GetCPU();

	pVideo->m_bWasRendered = true;

	if (!pVideo->IsOpen())
	{
		pVideo->Open();
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
			CTC_TheoraTexture::CTheora::CFrame *pFrame = pVideo->m_pFrame;

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
							int Width = pVideo->m_pDecoder->m_TheoraInfo.frame_width >> 1;
							int Height = pVideo->m_pDecoder->m_TheoraInfo.frame_height >> 1;

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
							int Width = pVideo->m_pDecoder->m_TheoraInfo.frame_width;
							int Height = pVideo->m_pDecoder->m_TheoraInfo.frame_height;

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

fp4 CTextureContainer_Video_Theora::GetTime(int _iLocal)
{
	int iLocal = _iLocal / 3;
	int iSub = _iLocal - iLocal * 3;
	ValidateLocalID(iLocal);
	
	CTC_TheoraTexture *pVideo = m_lspVideos[iLocal];
	M_LOCK(pVideo->m_TextureLock);
	if (!pVideo->IsOpen())
		return 0;

	return pVideo->m_pDecoder->GetNextFrameTime().GetTime();
//	return pVideo->m_pDecoder->GetTimeFromStart()/1000.0f;
}

//
// Not Supported functions
//
void CTextureContainer_Video_Theora::AutoRestart(int _iLocal, bool _EnableAutoRestart)
{
}

int CTextureContainer_Video_Theora::GetNumFrames(int _iLocal)
{
	return 1;
}
bool CTextureContainer_Video_Theora::MoveToLastFrame(int _iLocal)
{
	return 0;
}
void CTextureContainer_Video_Theora::SetVolume(int _iLocal, fp4 fpVol)
{}

void CTextureContainer_Video_Theora::Pause(int _iLocal, bool _Paused)
{}

void CTextureContainer_Video_Theora::SetSoundHandle(int _iLocal, int _hSound)
{
	int iLocal = _iLocal / 3;
	int iSub = _iLocal - iLocal * 3;
	ValidateLocalID(iLocal);
	
	CTC_TheoraTexture *pVideo = m_lspVideos[iLocal];
	M_LOCK(pVideo->m_TextureLock);
	if (!pVideo->IsOpen())
	{
		pVideo->Open();
	}
	if (!pVideo->IsOpen())
		return;

	pVideo->m_pDecoder->SetSound(_hSound);
}



MRTC_IMPLEMENT_DYNAMIC(CTextureContainer_Video_Theora, CTextureContainer_Video);

#endif
