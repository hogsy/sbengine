
#include "PCH.h"


#if defined(PLATFORM_PS3) || defined(PLATFORM_WIN32_PC)
#include "MMath_Vec128.h"

#include "MSound_PS3.h"
#include "../../MSystem.h"

// -------------------------------------------------------------------
#define MACRO_DSBDESC(Name)	\
	DSBUFFERDESC Name;		\
	FillChar(&Name, sizeof(Name), 0);	\
	Name.dwSize = sizeof(Name);



	static int gs_QualityMatrix[3][3] = {{48, 96, 192},   // 256, 512, 1024  
	{96, 192, 192},  // 512, 1024, 1024
	{96, 192, 384}  // 512, 1024, 2048
							};

// Frame Sizese
	static int gs_FarmeSizeMatrix[3][3] = {{256, 512, 1024},  // 256, 512, 1024  
	{512, 1024, 1024}, // 512, 1024, 1024
	{512, 1024, 2048} // 512, 1024, 2048
							};
							  
	static int gs_InterleaveMatrix[3][3] = {{8, 8, 8},   // 256, 512, 1024  
	{4, 4, 4},	// 512, 1024, 1024
	{4, 4, 4}	// 512, 1024, 2048
};


	static int gs_InterleaveSizeMatrix[3][3] = {{2048, 4096, 8192},   // 256, 512, 1024  
	{2048, 4096, 4096},	// 512, 1024, 1024
	{2048, 4096, 8192}	// 512, 1024, 2048
};

static int gs_nChannelsFromIndex[3] = {1, 2, 6};

// -------------------------------------------------------------------
//  CSCPS3_Buffer
// -------------------------------------------------------------------
#ifdef PLATFORM_PS3


void CSCPS3_Buffer::Clear()
{
	m_MixerVoice = 0xffffffff;
	m_pVoice = NULL;
	m_pStaticVoice = NULL;
	m_bDelayed = false;
	m_iWantDelete = 0;
	m_bLooping = NULL;
}

void CSCPS3_Buffer::Destroy()
{
	M_ASSERT(CanDelete(), "Must be able to delete");
	if (m_MixerVoice != 0xffffffff)
		m_pSoundContext->m_Mixer.Voice_FlushPackets(m_MixerVoice);
	m_Stream.Destroy();
	if (m_MixerVoice != 0xffffffff)
	{
		m_pSoundContext->m_Mixer.Voice_Stop(m_MixerVoice);
		m_MixerVoice = 0xffffffff;
	}
	
	Clear();
}

void CSCPS3_Buffer::Init(class CSoundContext_PS3 *_pSoundContext)
{
	m_pSoundContext = _pSoundContext;
	m_Stream.m_pParent = this;
	Clear();
}

CSCPS3_Buffer::~CSCPS3_Buffer()
{
	Destroy();
}

void CSoundContext_PS3::Thread_StartSounds()
{
	DLock(m_Streams_Start_Lock);
	DLinkD_Iter(CSCPS3_Buffer, m_ReadLink) Iter = m_Streams_Start;

	while (Iter)
	{
		CSCPS3_Buffer *pBuf = Iter;
		++Iter;

		if (pBuf->WantDelete())
		{
			m_Streams_Start.Remove(pBuf);
			continue;
		}

		bint bRet;
		{
			DUnlock(m_Streams_Start_Lock);
			bRet = pBuf->CreateUpdate();
		}
		if (bRet)
		{
			DLock(m_Streams_Read_Lock);
			m_Streams_Read.Insert(pBuf);
			m_Thread_Read.m_Event.Signal();
		}
	}
}

/*
void CSoundContext_PS3::Thread_ReturnBuffers()
{
	DLock(m_Streams_ReturnBuffers_Lock);
	DLinkD_Iter(CSCPS3_Buffer, m_ReturnBuffersLink) Iter = m_Streams_ReturnBuffers;

	while (Iter)
	{
		CSCPS3_Buffer *pBuf = Iter;
		++Iter;

		bint bRet;
		{
//			DUnlock(m_Streams_Start_Lock);
			bRet = pBuf->m_pStream->ReturnBuffers();
		}
		if (bRet)
		{
			DLock(m_Streams_Read_Lock);
			m_Streams_Read.Insert(pBuf);
			m_Thread_Read.m_Event.Signal();
		}
	}

}
*/

void CSoundContext_PS3::Thread_Read()
{
	DLock(m_Streams_Read_Lock);
	DLinkD_Iter(CSCPS3_Buffer, m_ReadLink) Iter = m_Streams_Read;

	while (Iter)
	{
		CSCPS3_Buffer *pBuf = Iter;
		++Iter;

		if (pBuf->WantDelete())
		{
			m_Streams_Read.Remove(pBuf);
			continue;
		}

		bint bRet;
		{
			DUnlock(m_Streams_Read_Lock);
			bRet = pBuf->ReadDataUpdate();
		}
		if (bRet)
		{
			m_Streams_Read.Remove(pBuf);
		}
	}
}

void CSoundContext_PS3::Thread_Submit()
{
	CMTime Timer;

	{
		TMeasure(Timer);
		DLock(m_Streams_Submit_Lock);
		DLinkD_Iter(CSCPS3_Buffer, m_SubmitLink) Iter = m_Streams_Submit;

		while (Iter)
		{
			CSCPS3_Buffer *pBuf = Iter;
			++Iter;

			if (pBuf->WantDelete())
			{
				m_Streams_Submit.Remove(pBuf);
				continue;
			}

			bint bRet;
			{
				DUnlock(m_Streams_Submit_Lock);
				bRet = pBuf->SubmitPackets();
			}
			if (bRet)
			{
				m_Streams_Submit.Remove(pBuf);
			}
		}
	}

	static int iTest = 0;
	if ((iTest % 128) == 0)
	{
//		M_TRACEALWAYS("Submit %f micro\n", Timer.GetTimefp64() * 1000000.0);
	}
	++iTest;

}


bint CSCPS3_Buffer::CreateUpdate()
{
	CSCStream *pStream = &m_Stream;
	if (!pStream->m_spCodec)
	{
		spCSCC_CodecStream spStream;
		{
			spStream = m_pSoundContext->m_spWaveContext->OpenStream(m_WaveID, 0);
		}
		m_Format = spStream->m_spCodec->m_Format;
		spStream->m_spCodec = NULL;
		CCFile *pFile = &(spStream->m_StreamFile);

		uint32 nStreams;
		pFile->ReadLE(nStreams);
		uint32 QualityIndex;
		pFile->ReadLE(QualityIndex);
		uint32 HeaderSize;
		pFile->ReadLE(HeaderSize);

		m_Stream.m_pStreamsData = DNew(CStreamsData_PS3) CStreamsData_PS3;
		m_Stream.m_pStreamsData->m_QualityIndex = QualityIndex;

		m_Stream.m_pStreamsData->m_Streams.SetLen(nStreams);

		m_Stream.m_DataSize = 0;
		for (int i = 0; i < nStreams; ++i)
		{
			CStreamData_PS3 &StreamData = m_Stream.m_pStreamsData->m_Streams[i];
			uint32 Temp;
			pFile->ReadLE(Temp);
			StreamData.m_HeaderDataSize = Temp;
			pFile->ReadLE(Temp);
			StreamData.m_DataSize = Temp;
			m_Stream.m_DataSize += Temp;
			pFile->ReadLE(Temp);
			StreamData.m_ChannelIndex = Temp;
			StreamData.m_Data.SetLen(StreamData.m_HeaderDataSize);
			pFile->Read(StreamData.m_Data.GetBasePtr(), StreamData.m_HeaderDataSize);
		}

		// Align
		fint Pos = pFile->Pos();
		if ((Pos) & (2048 - 1))
		{
			// Align
			fint EndPos = (((Pos) + 2047) & ~2047);
			if (EndPos < Pos)
				EndPos += 2048;
			pFile->Seek(EndPos);
			Pos = EndPos;
		}

		m_Stream.m_DataStartPos = m_Stream.m_BufferPos = Pos;
		m_Stream.m_spCodec = spStream;
		m_Stream.Create();
		Create(NULL);

	}

	return true;
}

bint CSCPS3_Buffer::ReadDataUpdate()
{
	CSCStream *pStream = &m_Stream;
	while (1)
	{
		CSCStream::CSCBuffer *pBuf = pStream->GetFreeBuffer();

		if (pBuf)
		{
			// Wrap stream

			if (pStream->m_BufferPos >= pStream->m_DataSize + pStream->m_DataStartPos)
			{
				if (!m_bLooping)
				{
					return true;
				}

				pStream->m_BufferPos = pStream->m_DataStartPos;
			}

			//fint nBytes = (pStream->m_DataSize + pStream->m_DataStartPos) - pStream->m_BufferPos;
			//nBytes = MinMT(nBytes, pStream->m_SubBufferSize);
			fint nBytes = pStream->m_SubBufferSize; // Always read the whole size
			pBuf->m_Request.SetRequest(pBuf->m_Request.m_pBuffer, pStream->m_BufferPos, nBytes, true);
			pStream->m_spCodec->m_StreamFile.Read(&pBuf->m_Request);
//			M_TRACEALWAYS("0x%08x: Reading %d at %d\n", pStream, nBytes, pStream->m_BufferPos);

			pStream->m_BufferPos += nBytes;


			pStream->QueueToSubmit(pBuf);
			m_pSoundContext->m_Thread_Submit.m_Event.Signal();
			m_pSoundContext->m_Thread_Read.m_Event.Signal(); // Signal that we want one more update

			return false;
		}
		else
			break;
	}

	return true;
}

static void CopyGatherStreams(CSCPS3_Buffer::CSCStream::CMixerBuffer *_pMixBuffer, CSCPS3_Buffer::CSCStream::CSCStream *_pStream, uint32 _StartSample, uint32 _nSamples)
{
    fp32 *pData = _pMixBuffer->m_pData + _StartSample * _pMixBuffer->m_nChannels;
	M_ASSERT(_StartSample + _nSamples <= _pMixBuffer->m_nSamples, "");


	fp32 *pPointers[8];
	uint32 nChannels[8];
	uint32 ToConsume = _nSamples;
	uint32 nStreams = _pStream->m_nStreams;
	uint32 Stride = 0;

	for (int i = 0; i < nStreams; ++i)
	{
		uint32 nChan = gs_nChannelsFromIndex[_pStream->m_pStreamsData->m_Streams[i].m_ChannelIndex];
		nChannels[i] = nChan;
		pPointers[i] = _pStream->m_AtracHandles[i].m_OutputBuffer.GetBasePtr() + _pStream->m_CurrentPacketPosRead*nChan;
		Stride += nChan;
	}

	for (int i = 0; i < ToConsume; ++i)
	{
		for (int s = 0; s < nStreams; ++s)
		{
			uint32 nChn = nChannels[s];
			for (int j = 0; j < nChn; ++j)
			{
				*(pData++) = pPointers[s][i*nChn + j];
			}
		}
	}
}


bint CSCPS3_Buffer::CSCStream::CanDelete()
{
	return true;
}

bint CSCPS3_Buffer::SubmitPackets()
{
	CMTime Now = CMTime::GetCPU();

	// Check if we should start static voices
/*	{
		if (m_pStaticVoice)
		{
			if (m_bDelayed)
			{
				if ((Now - m_StartTime).GetTime() >= m_MinDelay)
				{
					m_bDelayed = false;
					Resume();
					return true;
				}
			}
			return false;
		}
	}*/

	CSCStream *pStream = &m_Stream;

	if (!pStream->m_bCreated)
		return false;

	uint32 nStreams = pStream->m_pStreamsData->m_Streams.Len();;

	bint bDoneSomething = true;
	while (bDoneSomething)
	{
		bDoneSomething = false;
		if (!m_pStaticVoice)
		{
			while (1)
			{
				CSCStream::CSCBuffer *pBuf;
				{
					DLock(pStream->m_Lock);
					pBuf = pStream->m_StreamsToSubmit.GetFirst();
				}

				if (pBuf)
				{
					if (pStream->m_spCodec->m_StreamFile.Read(&pBuf->m_Request))
					{
						uint32 nPackets = 0;

						uint32 StreamStride = pStream->m_pStreamsData->m_Stride;

						uint32 CurrentSubPacketPos = (pBuf->m_Request.m_StartPos - pStream->m_DataStartPos);
						uint32 iPacket = CurrentSubPacketPos / StreamStride;
						nPackets = pBuf->m_Request.m_nBytes / StreamStride;

						uint8 *pSource = (uint8 *)pBuf->m_Request.m_pBuffer;

						bint bDoneSomething = 1;
						while (bDoneSomething)
						{
							bDoneSomething = false;
							uint32 PacketDone = nStreams;
							for (int i = 0; i < nStreams; ++i)
							{
								uint8_t *pWritePtr;
								uint32_t WritableByte;
								uint32_t ReadPos;
								cellAtracGetStreamDataInfo(&pStream->m_AtracHandles[i].m_Handle, &pWritePtr, &WritableByte, &ReadPos);

								if (WritableByte)
								{
									CStreamData_PS3 &Stream = pStream->m_pStreamsData->m_Streams[i];
									uint32 WritePos = pWritePtr - Stream.m_Data.GetBasePtr();
									uint32 StreamPacketSize = gs_InterleaveSizeMatrix[pStream->m_pStreamsData->m_QualityIndex][Stream.m_ChannelIndex];
									uint32 StreamOffset = Stream.m_CurrentInterleavePos;

									uint32 PacketPos = (iPacket + pStream->m_iCurrentSubmitStream) * StreamPacketSize;

									ReadPos -= Stream.m_HeaderDataSize;
									uint32 RealSize = StreamPacketSize;
									RealSize = Min(RealSize, Stream.m_DataSize - PacketPos);

									if ((ReadPos >= PacketPos) && ((ReadPos) < (PacketPos + RealSize)))
									{
										bDoneSomething = true;
										uint32 SubPos = ReadPos - PacketPos;
										uint32 ToRead = Min(WritableByte, RealSize - SubPos);
										memcpy(pWritePtr, pSource + pStream->m_iCurrentSubmitStream * StreamStride + SubPos + StreamOffset, ToRead);
	//									M_TRACEALWAYS("0x%08x: Submitting %d at %d\n", pStream, ToRead, ReadPos);
										cellAtracAddStreamData(&pStream->m_AtracHandles[i].m_Handle, ToRead);
										bDoneSomething = true;
										if (ReadPos + ToRead >= PacketPos + RealSize)
										{
	//										M_TRACEALWAYS("0x%08x: Retiering packet %d with size %d\n", pStream, PacketPos, RealSize);
											--PacketDone;
										}
									}
									else if (ReadPos >= PacketPos + RealSize)
									{
										--PacketDone;
									}
								}
							}

							if (PacketDone)
							{
								if (bDoneSomething)
									continue;
								else
									break;
							}

							bDoneSomething = true;
							++pStream->m_iCurrentSubmitStream;
							if (pStream->m_iCurrentSubmitStream == nPackets)
								break;
						}
						if (pStream->m_iCurrentSubmitStream == nPackets)
						{
							pStream->m_iCurrentSubmitStream = 0;

							if (m_pStaticVoice)
							{
								if (m_bLooping)
								{
									DLock(pStream->m_Lock);
									pStream->m_StreamsToSubmit.Insert(pBuf);
								}
								else
									pStream->ReturnBuffer(pBuf);
							}
							else
							{
								pStream->ReturnBuffer(pBuf);
								{
									DLock(m_pSoundContext->m_Streams_Read_Lock);
									if (!m_ReadLink.IsInList())
										m_pSoundContext->m_Streams_Read.Insert(this);

									m_pSoundContext->m_Thread_Read.m_Event.Signal();
								}
							}
						}
						else
							break;
					}
					else
						break;
				}
				else
					break;
			}
		}


		bint bSubmitted = false;
		// Check for Mixer packets that has finished playing and add them to the list of free packets
		{
			while (1)
			{
				CSCStream::CMixerBuffer *pBuf = (CSCStream::CMixerBuffer *)m_pSoundContext->m_Mixer.Voice_PopFinishedPacket(m_MixerVoice);
				if (pBuf)
				{
					pStream->m_pMixBuffersUnused->Insert(pBuf);
				}
				else
					break;
			}

		}

		// Decode data
		if (!pStream->m_bPendingDataAvailable)
		{
			bint bAvailable = nStreams;
			for (int i = 0; i < nStreams; ++i)
			{
				CStreamData_PS3 &Stream = pStream->m_pStreamsData->m_Streams[i];
				CSCStream::CAtracHandle &Atrac = pStream->m_AtracHandles[i];

				if (Atrac.m_bDecodeDone == 2)
				{
					// Do request
					Atrac.m_bDecodeDone = 0;

					uint32 Finished;

					uint32 NextDecodePos = 0;
					int StatusDecode = cellAtracGetNextDecodePosition(&Atrac.m_Handle, &NextDecodePos);

					int Status = cellAtracDecode(&Atrac.m_Handle, Atrac.m_OutputBuffer.GetBasePtr(), &Atrac.m_nDecodedSamples, &Finished, &Atrac.m_RemainingFrames);

					if (Status == CELL_ATRAC_ERROR_NODATA_IN_BUFFER || Status == CELL_ATRAC_ERROR_ALLDATA_WAS_DECODED)
					{
/*						uint8_t *pWritePtr;
						uint32_t WritableByte;
						uint32_t ReadPos;
						cellAtracGetStreamDataInfo(&pStream->m_AtracHandles[i].m_Handle, &pWritePtr, &WritableByte, &ReadPos);*/

						Atrac.m_bDecodeDone = 2;
					}
					else if (Status)
						M_BREAKPOINT;
					else
					{
						Atrac.m_LastDecodedPos = NextDecodePos + Atrac.m_nDecodedSamples;
						Atrac.m_bDecodeDone = 1;
						bDoneSomething = true;
						--bAvailable;
					}
				}
				else if (Atrac.m_bDecodeDone == 1)
				{
					// Request done
					--bAvailable;
				}
			}
			if (!bAvailable)
			{
				pStream->m_bPendingDataAvailable = true;
				for (int i = 0; i < nStreams; ++i)
				{
					CSCStream::CAtracHandle &Atrac = pStream->m_AtracHandles[i];
					Atrac.m_bDecodeDone = 2;
				}
			}
		}

		// Check for decoded data and put in first mixer packet available. When full submit packet to mixer and start delayed voices voice.
		if (pStream->m_bPendingDataAvailable)
		{
			while (1)
			{
				CSCStream::CMixerBuffer *pBuffer = (CSCStream::CMixerBuffer *)pStream->m_pMixBuffersUnused->GetFirst();
				if (!pBuffer)
					break;

				uint32 nAvailable = 0;
				uint32 nAvailableStart = 0;
				// Maximum of 8 streams
				for (int i = 0; i < nStreams; ++i)
				{
					CStreamData_PS3 &Stream = pStream->m_pStreamsData->m_Streams[i];
					CSCStream::CAtracHandle &Atrac = pStream->m_AtracHandles[i];

					if (i == 0)
						nAvailable = Atrac.m_nDecodedSamples;
					else if (nAvailable != Atrac.m_nDecodedSamples)
						M_BREAKPOINT; // Number of decoded samples not same for all streams
				}

				nAvailableStart = nAvailable;
				nAvailable -= pStream->m_CurrentPacketPosRead;

				uint32 iNeeded = m_Format.m_Data.GetNumSamples() - pStream->m_SamplePos;
				iNeeded = Min(iNeeded, pBuffer->m_nSamples - pStream->m_MixerStreamPos);

				uint32 nToRead = Min(nAvailable, iNeeded);

				CopyGatherStreams(pBuffer, pStream, pStream->m_MixerStreamPos, nToRead);

				bDoneSomething = true;

				pStream->m_CurrentPacketPosRead += nToRead;
				pStream->m_MixerStreamPos += nToRead;
				pStream->m_SamplePos += nToRead;

				bint bWrap = false;
				if (pStream->m_SamplePos >= m_Format.m_Data.GetNumSamples())
				{
					bWrap = true;
					pStream->m_SamplePos = 0;
				}

				if (pStream->m_MixerStreamPos == pBuffer->m_nSamples || bWrap)
				{
					pBuffer = (CSCStream::CMixerBuffer *)pStream->m_pMixBuffersUnused->Pop();
					
#if 0
					for (int i = 0; i < pBuffer->m_nSamples; ++i)
					{
						for (int ch = 0; ch < pBuffer->m_nChannels; ++ch)
						{
							if (ch == 0)
								pBuffer->m_pData[i*pBuffer->m_nChannels + ch] = M_Sin((fp32(i) / 128.0)*M_PI)*0.5;
							else
								pBuffer->m_pData[i*pBuffer->m_nChannels + ch] = M_Sin((fp32(i) / 64.0)*M_PI)*0.7;
						}
					}
#endif
					m_pSoundContext->m_Mixer.Voice_SubmitPacket(m_MixerVoice, pBuffer);
					bSubmitted = true;

					pStream->m_MixerStreamPos = 0;
				}

				if (pStream->m_CurrentPacketPosRead == nAvailableStart)
				{
					pStream->m_CurrentPacketPosRead = 0;
					pStream->m_bPendingDataAvailable = false;
					break;
				}
			}		
			
		}

		if (m_bDelayed && bSubmitted)
		{
			m_bDelayed = false;
			m_pSoundContext->Internal_VoiceUnpause(m_pVoice, CSoundContext_Mixer::EPauseSlot_Delayed);
		}
	}

	return false;
}

uint32 CStreamsData_PS3::GetNumChannels()
{
	uint32 nChannels = 0;
	for (int i = 0; i < m_Streams.Len(); ++i)
	{
		nChannels += gs_nChannelsFromIndex[m_Streams[i].m_ChannelIndex];
	}

	return nChannels;
}

void CSCPS3_Buffer::CSCStream::Create()
{
	m_pMixBuffersUnused = CSC_Mixer_WorkerContext::New< DLinkAllocatorDS_List(CSC_Mixer_WorkerContext::CSC_Mixer_Packet, m_Link, CSC_Mixer_WorkerContext::CCustomAllocator) >();
	m_CurrentPacketPosRead = 0;
	m_bPendingDataAvailable = false;
	m_iCurrentSubmitStream = 0;
	m_MixerStreamPos = 0;
	m_SamplePos = 0;
	if (m_pParent->m_pStaticVoice)
	{
		m_pStreamsData = &m_pParent->m_pStaticVoice->m_StreamsData;
	}

	m_nStreams = m_pStreamsData->m_Streams.Len();

	m_AtracHandles.SetLen(m_nStreams);
	uint32 nChannels = m_pStreamsData->GetNumChannels();

	// Create three mixbuffers for good ovelapping
	{
		// Buffer 8 frames of samples (4 KB in 48000 KHz case)
		uint32 nSamplesPerBuffer = TruncToInt(((m_pParent->m_SampleRate * 512 * 8) / 48000));
		nSamplesPerBuffer += 2047;
		nSamplesPerBuffer /= 2048;
		nSamplesPerBuffer *= 2048; // Must align on 2048 samples
		for (int i = 0; i < 3; ++i)
		{
			CMixerBuffer *pNew = CSC_Mixer::Voice_AllocMixerPacket<CMixerBuffer>(nSamplesPerBuffer * nChannels * sizeof(fp32), nChannels, nSamplesPerBuffer);
			m_pMixBuffersUnused->Insert(pNew);
		}
	}

#if 0
	if (m_pParent->m_pStaticVoice)
	{
		m_pStaticBuffer = _pStaticPacket;
		m_BufferSize = _PacketSize;
		m_SubBufferSize = _PacketSize;
		m_pBufferBlock = NULL;
		for (int i = 0; i < 2; ++i)
		{
			CSCBuffer *pBuf = DNew(CSCBuffer) CSCBuffer();
			pBuf->m_pParent = m_pParent;
			pBuf->m_Request.m_pBuffer = _pStaticPacket;
			pBuf->m_Request.m_nBytes = _PacketSize;
			m_StreamsToSubmit.Insert(pBuf);
		}
	}
	else
#endif
	{
		mint BufferSize = 2048 * 16;

		mint BlockSize = 0;

		for (int i = 0; i < m_nStreams; ++i)
		{
			CStreamData_PS3 &Stream = m_pStreamsData->m_Streams[i];

			Stream.m_CurrentInterleavePos = BlockSize;

			BlockSize += gs_InterleaveSizeMatrix[m_pStreamsData->m_QualityIndex][Stream.m_ChannelIndex];
		}

		m_pStreamsData->m_Stride = BlockSize;

		// Align on whole packet
		BufferSize += (BlockSize - 1);
		BufferSize /= BlockSize;
		BufferSize *= BlockSize;

		if (m_pParent->m_pStaticVoice)
		{
			m_BufferSize = 0;
			m_SubBufferSize = 0;
		}
		else
		{
			mint nBuffers = DMaxPackets;
			m_BufferSize = BufferSize*nBuffers;
			m_SubBufferSize = BufferSize;
			{
				m_StreamBuffer.SetLen(m_BufferSize);
			}
			uint8 *pBuffer = m_StreamBuffer.GetBasePtr();
			for (int i = 0; i < nBuffers; ++i)
			{
				CSCBuffer *pBuf = DNew(CSCBuffer) CSCBuffer();
				pBuf->m_pParent = m_pParent;
				pBuf->m_Request.m_pBuffer = pBuffer;
				m_StreamsFree.Insert(pBuf);
				pBuffer += m_SubBufferSize;
			}
		}

		fint iPos = 0;
		if (!m_pParent->m_pStaticVoice)
			m_spCodec->m_StreamFile.Pos();

		for (int i = 0; i < m_nStreams; ++i)
		{
			CStreamData_PS3 &Stream = m_pStreamsData->m_Streams[i];
			uint32_t NeededBufferSize;
			
			
			int32 status;
			if (m_pParent->m_pStaticVoice)
				status = cellAtracSetDataAndGetMemSize(&m_AtracHandles[i].m_Handle, Stream.m_Data.GetBasePtr(), Stream.m_HeaderDataSize + Stream.m_DataSize, Stream.m_HeaderDataSize + Stream.m_DataSize, &NeededBufferSize);
			else
			{
				uint32 BlockSize2 = gs_InterleaveSizeMatrix[m_pStreamsData->m_QualityIndex][Stream.m_ChannelIndex];
				Stream.m_Data.SetLen(BlockSize2*2);
				m_spCodec->m_StreamFile.Read(Stream.m_Data.GetBasePtr() + Stream.m_HeaderDataSize, BlockSize2);
				status = cellAtracSetDataAndGetMemSize(&m_AtracHandles[i].m_Handle, Stream.m_Data.GetBasePtr(), BlockSize2 + Stream.m_HeaderDataSize, BlockSize2*2, &NeededBufferSize);
			}

			if (status < 0) 
				M_BREAKPOINT;
	
			m_DecoderData.SetLen(NeededBufferSize);
			status = cellAtracCreateDecoder(&m_AtracHandles[i].m_Handle,m_DecoderData.GetBasePtr(),100,100);
			if (status < 0) 
				M_BREAKPOINT;

			uint32 MaxSamples = 0;
			status = cellAtracGetMaxSample(&m_AtracHandles[i].m_Handle, &MaxSamples);
			if (status < 0) 
				M_BREAKPOINT;
			MaxSamples = MaxMT(2048, MaxSamples);

			int32 EndSample = 0;
			int32 LoopStartSample = 0;
			int32 LoopEndSample = 0;
			status = cellAtracGetSoundInfo(&m_AtracHandles[i].m_Handle, &EndSample, &LoopStartSample, &LoopEndSample);
			if (status < 0) 
				M_BREAKPOINT;

			uint32 NextDecodePos = 0;
			status = cellAtracGetNextDecodePosition(&m_AtracHandles[i].m_Handle, &NextDecodePos);
			if (status < 0) 
				M_BREAKPOINT;

			m_AtracHandles[i].m_OutputBuffer.SetLen(MaxSamples * gs_nChannelsFromIndex[Stream.m_ChannelIndex]);

			uint32 nSamples = m_pParent->m_Format.m_Data.GetNumSamples();

//			M_TRACEALWAYS("Blaha needed size %d Max Samples %d nSamples %d = %d\n", NeededBufferSize, MaxSamples, EndSample, nSamples);
		}
		if (!m_pParent->m_pStaticVoice)
			m_spCodec->m_StreamFile.Seek(iPos);
		m_bCreated = true;
	}

}

CSCPS3_Buffer::CSCStream::~CSCStream()
{
	Destroy();
}

void CSCPS3_Buffer::CSCStream::Destroy()
{
	if (m_bCreated)
	{
		for (int i = 0; i < m_AtracHandles.Len(); ++i)
		{
			cellAtracDeleteDecoder(&m_AtracHandles[i].m_Handle);
		}
	}

	if (m_spCodec)
	{
		{
			DLinkD_Iter(CSCBuffer, m_Link) Iter = m_StreamsReading;
			while (Iter)
			{
				while (!Iter->m_Request.Done())
					m_spCodec->m_StreamFile.Read(&Iter->m_Request);
				++Iter;
			}
		}
		{
			DLinkD_Iter(CSCBuffer, m_Link) Iter = m_StreamsToSubmit;
			while (Iter)
			{
				while (!Iter->m_Request.Done())
					m_spCodec->m_StreamFile.Read(&Iter->m_Request);
				++Iter;
			}
		}

		m_spCodec = NULL;
	}

	m_StreamBuffer.Clear();

	if (m_pParent->m_MixerVoice != 0xffffffff)
	{
		CSCStream::CMixerBuffer *pBuf = (CSCStream::CMixerBuffer *)m_pParent->m_pSoundContext->m_Mixer.Voice_PopFinishedPacket(m_pParent->m_MixerVoice);
		while (pBuf)
		{
			CSC_Mixer::Voice_FreeMixerPacket(pBuf);
			pBuf = (CSCStream::CMixerBuffer *)m_pParent->m_pSoundContext->m_Mixer.Voice_PopFinishedPacket(m_pParent->m_MixerVoice);
		}
	}

		CSCStream::CMixerBuffer *pBuf = (CSCStream::CMixerBuffer *)m_pMixBuffersUnused->Pop();
		while (pBuf)
		{
			CSC_Mixer::Voice_FreeMixerPacket(pBuf);
			pBuf = (CSCStream::CMixerBuffer *)m_pMixBuffersUnused->Pop();
		}
	

	m_StreamsFree.DeleteAll();
	m_StreamsReading.DeleteAll();
	m_StreamsToSubmit.DeleteAll();

	if (m_pStreamsData && !m_pParent->m_pStaticVoice)
		delete m_pStreamsData;
	m_pStreamsData = NULL;

	m_AtracHandles.Clear();
	m_DecoderData.Clear();

	if (m_pMixBuffersUnused)
		CSC_Mixer_WorkerContext::Delete< DLinkAllocatorDS_List(CSC_Mixer_WorkerContext::CSC_Mixer_Packet, m_Link, CSC_Mixer_WorkerContext::CCustomAllocator) >(m_pMixBuffersUnused);
	m_bCreated = false;
}

void CSCPS3_Buffer::Create(CStaticVoice_PS3 *_pStaticVoice)
{

	m_pStaticVoice = _pStaticVoice;

	m_pSoundContext->m_Mixer.Voice_Setup(m_MixerVoice, m_Format.m_Data.GetChannels(), m_Format.m_Data.GetNumSamples());

	if (m_pStaticVoice)
	{

		m_Stream.Create();

	}
	else
	{
		M_LOCK(m_pSoundContext->m_DSLock);

	}

}

bool CSCPS3_Buffer::CanDelete()
{
	{
		DLock(m_pSoundContext->m_Streams_Start_Lock);
		{
			DLock(m_pSoundContext->m_Streams_Read_Lock);
			{
				DLock(m_pSoundContext->m_Streams_Submit_Lock);

				if (m_ReadLink.IsInList())
				{
					M_TRACE("Cannot delete: ReadLink\n");
					return false;
				}
				if (m_SubmitLink.IsInList())
				{
					M_TRACE("Cannot delete: SubmitLink\n");
					return false;
				}
			}
		}
	}

	if (!m_Stream.CanDelete())
		return false;

	if (m_MixerVoice == 0xffffffff)
		return true;

	if (!m_pSoundContext->m_Mixer.Voice_IsStopped(m_MixerVoice))
	{
		M_TRACE("Cannot delete: Mixer\n");
		return false;
	}

	
	M_TRACE("Can delete\n");
	return true; 
}
#endif

// -------------------------------------------------------------------
//  CSoundContext_PS3
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CSoundContext_PS3, CSoundContext);

#ifdef PLATFORM_PS3

void CSoundContext_PS3::CreateDevice()
{
	DestroyDevice();

	int err = cellAudioInit();
	if (err != CELL_OK)
		M_BREAKPOINT;

	CellAudioPortParam CellAudioPortParams;

	CellAudioPortParams.attr = 0;
	CellAudioPortParams.nBlock = CELL_AUDIO_BLOCK_8;
	CellAudioPortParams.nChannel = CELL_AUDIO_PORT_8CH;
	CellAudioPortParams.level = 1.0;

	unsigned int PortNumber;
	err = cellAudioPortOpen(&CellAudioPortParams, &PortNumber);
	if (err != CELL_OK)
		M_BREAKPOINT;


	m_PortNumber = PortNumber;

	err = cellAudioGetPortConfig(m_PortNumber, &m_PortConfig);
	if (err != CELL_OK)
		M_BREAKPOINT;
	
	sys_event_queue_attribute_t	queueAttr = { SYS_SYNC_FIFO, SYS_PPU_QUEUE };
#define QUEUE_KEY_BASE	0x0000000095837866UL /* just a temporary value */
#define QUEUE_DEPTH		4

	int tryCount = 0;
	m_AudioSysEventQueueKey = QUEUE_KEY_BASE;
	while (tryCount < 10)
	{
		err = sys_event_queue_create(&m_AudioSysEventQueue, &queueAttr, m_AudioSysEventQueueKey, QUEUE_DEPTH);
		if (err == CELL_OK){
			break;
		}
		m_AudioSysEventQueueKey = QUEUE_KEY_BASE | (rand() & 0x0ffff); /* search unused key */
		tryCount ++;
	}
	if (err != CELL_OK)
		M_BREAKPOINT;

	m_Thread_AudioRingBuffer.m_pThis = this;
	m_Thread_AudioRingBuffer.Thread_Create(NULL, 16384, MRTC_THREAD_PRIO_TIMECRITICAL);
}


#include "sysutil/sysutil_sysparam.h"
void CSoundContext_PS3::Thread_AudioRingBuffer()
{
	M_TRACEALWAYS("Thread_AudioRingBuffer\n");
	int err = cellAudioSetNotifyEventQueue(m_AudioSysEventQueueKey);
	if (err < 0)
		M_BREAKPOINT;

	sys_event_queue_drain(m_AudioSysEventQueue);

	/* start port */
	err = cellAudioPortStart(m_PortNumber);
	if (err < 0)
		M_BREAKPOINT;
	
	mint BlockSize = 8 * CELL_AUDIO_BLOCK_SAMPLES;
	fp32 iCurAngle = 0.0;

	int iNext = 0;

	while (!m_Thread_AudioRingBuffer.Thread_IsTerminating())
	{
		sys_event_t  event;
		err = sys_event_queue_receive(m_AudioSysEventQueue, &event, 4 * 1000 * 1000);
		if (err == ETIMEDOUT)
		{
			continue;
		}
//		M_TRACEALWAYS("ReadIndex: %d\n", );
		uint64 iBlock = *((uint64_t*)m_PortConfig.readIndexAddr);
//		iBlock+=1;
		if (iBlock >= 8)
			iBlock -= 8;
		if (iNext != iBlock)
		{
//			M_TRACEALWAYS("Missed audio frame %d(%d)\n", iBlock, iNext);
			iBlock = *((uint64_t*)m_PortConfig.readIndexAddr);
//			iBlock+=1;
			if (iBlock >= 8)
				iBlock -= 8;
		}
		iNext = iBlock + 1;
		if (iNext >= 8)
			iNext -= 8;

		fp32 *pDest = (fp32 *)m_PortConfig.portAddr;
		pDest += BlockSize * iBlock;

		CSC_Mixer_OuputFrame *pFrame = m_Mixer.StartNewFrame();
		if (pFrame)
		{
			if (pFrame->m_nSamples != CELL_AUDIO_BLOCK_SAMPLES)
				M_BREAKPOINT;
			if (pFrame->m_nChannels != 8)
				M_BREAKPOINT;

			M_IMPORTBARRIER;
			memcpy(pDest, pFrame->m_pData, BlockSize * sizeof(fp32));
//			pDest[0] = 0.0f;
			M_EXPORTBARRIER;

		}
//		else
//			M_TRACEALWAYS("Missed audio frame %d\n", iBlock);

#if 0
		for (int i = 0; i < BlockSize; ++i)
		{
			if (i % 8 == 0)
			{
				pDest[i] = M_Sin(iCurAngle);
				iCurAngle += 0.056;
				if (iCurAngle >	M_PI*2)
					iCurAngle -= M_PI*2;
			}
			else
				pDest[i] = 0.0;
		}
#endif
	}

	err = cellAudioPortStop(m_PortNumber);
	if (err < 0)
		M_BREAKPOINT;

	err = cellAudioRemoveNotifyEventQueue(m_AudioSysEventQueueKey);
	if (err < 0)
		M_BREAKPOINT;

}


void CSoundContext_PS3::DestroyDevice()
{
	if (m_PortNumber != 0xffffffff)
	{
		m_Thread_AudioRingBuffer.Thread_Destroy();
		sys_event_queue_destroy(m_AudioSysEventQueue, 0);
		cellAudioPortClose(m_PortNumber);
		cellAudioQuit();
		m_PortNumber = 0xffffffff;
	}
}


#endif

void CSoundContext_PS3::Wave_PrecacheFlush()
{
	DLock(m_InterfaceLock);
	CSuper::Wave_PrecacheFlush();


	// Remove Aramvoices
	for (int i = 0; i < m_lpStaicVoices.Len(); i++)
	{
		CStaticVoice_PS3 *pToDelete = m_lpStaicVoices[i];
		if(pToDelete && !pToDelete->m_RefCount)
		{
			CSC_IDInfo* pInfo = &GetWCIDInfo()->m_pWCIDInfo[pToDelete->m_WaveID];
			if (!(m_spWaveContext->GetWaveIDFlag(pToDelete->m_WaveID) & CWC_WAVEIDFLAGS_PRECACHE) || !(pInfo->m_Fresh & 1))
				delete pToDelete;
		}
	}
}

void CSoundContext_PS3::Wave_PrecacheBegin( int _Count )
{
	DLock(m_InterfaceLock);
	CSuper::Wave_PrecacheBegin( _Count );
}

void CSoundContext_PS3::Wave_PrecacheEnd(uint16 *_pPreCacheOrder, int _nIds)
{
	DLock(m_InterfaceLock);
	CSuper::Wave_PrecacheEnd(_pPreCacheOrder, _nIds);
}


CStaticVoice_PS3::CStaticVoice_PS3(CSoundContext_PS3 *_pSCD, int _WaveId)
{
	m_WaveID = _WaveId;
	m_pSCD = _pSCD;
	m_pSCD->m_lpStaicVoices[_WaveId] = this;
	m_RefCount = 0;
}

CStaticVoice_PS3::~CStaticVoice_PS3()
{
	m_pSCD->m_lpStaicVoices[m_WaveID] = NULL;
}



void CStaticVoice_PS3::LoadVoice(CCFile *_pFile)
{
	uint32 nStreams;
	_pFile->ReadLE(nStreams);
	uint32 QualityIndex;
	_pFile->ReadLE(QualityIndex);
	uint32 HeaderSize;
	_pFile->ReadLE(HeaderSize);

	m_StreamsData.m_QualityIndex = QualityIndex;

	m_StreamsData.m_Streams.SetLen(nStreams);

	for (int i = 0; i < nStreams; ++i)
	{
		CStreamData_PS3 &StreamData = m_StreamsData.m_Streams[i];
		uint32 Temp;
		_pFile->ReadLE(Temp);
		StreamData.m_HeaderDataSize = Temp;
		_pFile->ReadLE(Temp);
		StreamData.m_DataSize = Temp;
		_pFile->ReadLE(Temp);
		StreamData.m_ChannelIndex = Temp;
		StreamData.m_Data.SetLen(StreamData.m_HeaderDataSize + StreamData.m_DataSize);
		_pFile->Read(StreamData.m_Data.GetBasePtr(), StreamData.m_HeaderDataSize);
	}

	// Align
	fint Pos = _pFile->Pos();
	if ((Pos) & (2048 - 1))
	{
		// Align
		fint EndPos = (((Pos) + 2047) & ~2047);
		if (EndPos < Pos)
			EndPos += 2048;
		_pFile->Seek(EndPos);
	}

	// Uninterleave
	int nStreamsDone = nStreams;
	while (nStreamsDone)
	{
		for (int i = 0; i < nStreams; ++i)
		{
			CStreamData_PS3 &StreamData = m_StreamsData.m_Streams[i];

			uint32 ToRead = gs_InterleaveSizeMatrix[QualityIndex][StreamData.m_ChannelIndex];
			uint32 MaxRead = StreamData.m_DataSize - StreamData.m_CurrentInterleavePos;
			if (MaxRead)
			{
				uint32 CurrentRead = Min(ToRead, MaxRead);

				_pFile->Read(StreamData.m_Data.GetBasePtr() + StreamData.m_HeaderDataSize + StreamData.m_CurrentInterleavePos, CurrentRead);
				StreamData.m_CurrentInterleavePos += CurrentRead;
				if (StreamData.m_CurrentInterleavePos >= StreamData.m_DataSize)
					--nStreamsDone;
				if (ToRead - CurrentRead)
					_pFile->RelSeek(ToRead - CurrentRead); // Skip padding
			}
		}
	}
}

void CSoundContext_PS3::Wave_Precache(int _WaveID)
{
	DLock(m_InterfaceLock);

	MSCOPE(CSoundContext_PS3::Wave_Precache, SCXbox);
//	return -1;
	// M_CALLGRAPH;
	CStaticVoice_PS3 *pStaticVoice = NULL;
	M_TRY
	{
		if (_WaveID < 0)
			return;

#ifndef PLATFORM_CONSOLE
//		if (m_bLogUsage)
//			m_spWaveContext->AddWaveIDFlag(_WaveID, CWC_WAVEIDFLAGS_USED);
#endif

#if 1 // Only support streamed sounds for now
		CWaveData WaveData;
		m_spWaveContext->GetWaveLoadedData(_WaveID, WaveData);
		uint32 Flags = WaveData.Get_Flags();

		if (!(WaveData.Get_Flags() & ESC_CWaveDataFlags_Stream))
		{
//			int iLocal;
//			CWaveContainer_Plain *pWaveC = TDynamicCast<CWaveContainer_Plain >(m_spWaveContext->GetWaveContainer(_WaveID, iLocal));
//			M_TRACE(CStrF("Precahcing Wave: %s\n", pWaveC->GetName(iLocal)));

			pStaticVoice = m_lpStaicVoices[_WaveID];

			if (!pStaticVoice)
			{
				pStaticVoice = DNew(CStaticVoice_PS3) CStaticVoice_PS3(this, _WaveID);

				spCSCC_CodecStream spStream;
				{
					DUnlock(m_InterfaceLock);
					spStream = m_spWaveContext->OpenStream(_WaveID, 0);
				}

				pStaticVoice->LoadVoice(&spStream->m_StreamFile);
			}
		}
#endif
	}
	M_CATCH(
	catch(CCException)
	{
		if (pStaticVoice)
			delete pStaticVoice;
	}
	)
	CSuper::Wave_Precache(_WaveID);
}

int CSoundContext_PS3::Wave_GetMemusage(int _WaveID)
{
	DLock(m_InterfaceLock);
	CStaticVoice_PS3 *pStaticVoice = m_lpStaicVoices[_WaveID];

	if (pStaticVoice)
		return ((pStaticVoice->m_VoiceDataSize + 7) & (~7)) + ((sizeof(CStaticVoice_PS3) + 7) & (~7)) + 4 + 8*2;

	return 0;
}

#ifdef PLATFORM_PS3

CStaticVoice_PS3 *CSoundContext_PS3::GetStaticVoice(int _WaveID)
{
	CStaticVoice_PS3 *pStaticVoice = m_lpStaicVoices[_WaveID];
	CSC_IDInfo* pInfo = &GetWCIDInfo()->m_pWCIDInfo[_WaveID];

	// If not found, try to precache it
	if (!pStaticVoice || !(pInfo->m_Fresh & 1))
		Wave_Precache(_WaveID);

	return m_lpStaicVoices[_WaveID];
}

#endif

CSoundContext_PS3 * g_XSoundContext = NULL;


// -------------------------------------------------------------------
CSoundContext_PS3::CSoundContext_PS3()
{
//	DmSetDataBreakpoint(this, DMBREAK_WRITE, 4);

	g_XSoundContext = this;
	m_Recursion = 0;

//	m_StaticVoiceHash.InitHash(2,(uint32)(&((CStaticVoice_PS3 *)(0x70000000))->m_WaveID) - 0x70000000,2);

	m_DebugStrTemp = NULL;

	m_PortNumber = 0xffffffff;

	m_bCreated = false;
	m_pMainThreadID = MRTC_SystemInfo::OS_GetThreadID();

	MACRO_AddSubSystem(this);

}

void CSoundContext_PS3::CreateAll()
{
	if (m_bCreated)
		return;
	
#ifdef PLATFORM_PS3
	CreateDevice();
#endif

	MultiStream_Resume();

	MACRO_GetSystem;
	// Tell we need to reprecache sounds
	if(pSys) 
		pSys->System_BroadcastMessage(CSS_Msg(CSS_MSG_PRECACHEHINT));

	m_bCreated = true;


}

void CSoundContext_PS3::DestroyAll()
{
	if (!m_bCreated)
		return;

	MultiStream_Pause();

	for (int i = 0; i < m_lpStaicVoices.Len(); i++)
	{
		if (m_lpStaicVoices[i])
			delete m_lpStaicVoices[i];
	}

	m_bCreated = false;
//	m_lspVoices.Clear();
#ifdef PLATFORM_PS3
	DestroyDevice();
#endif
}


void CSoundContext_PS3::KillThreads()
{
	Destroy_Everything();
//	m_Thread_ReturnBuffers.Thread_Destroy();
#ifdef PLATFORM_PS3
	m_Thread_Read.Thread_Destroy();
	m_Thread_SoundStart.Thread_Destroy();
	m_Thread_Submit.Thread_Destroy();
#endif
}

CSoundContext_PS3::~CSoundContext_PS3()
{

	KillThreads();

	MACRO_RemoveSubSystem(this);

	for (int i = 0; i < m_lpStaicVoices.Len(); i++)
	{
		if (m_lpStaicVoices[i])
			delete m_lpStaicVoices[i];
	}
	m_lpStaicVoices.Clear();


#ifdef PLATFORM_PS3
	DestroyDevice();
#endif

	if (m_DebugStrTemp)
	{
		for (int i = 0; i < mcs_MaxDebugSounds - 1; ++i)
			delete [] m_DebugStrTemp[i];

		delete [] m_DebugStrTemp;
	}

	g_XSoundContext = NULL;
#if 0
	if (m_pStaticHeap)
	{
#ifdef PLATFORM_PS3
		XPhysicalFree(m_pStaticHeap);
#else
		MRTC_SystemInfo::OS_Free(m_pStaticHeap);
#endif
	}
#endif
}



void CSoundContext_PS3::Create(CStr _Params)
{
	CSuper::Create(_Params);
}


void CSoundContext_PS3::Platform_GetInfo(CPlatformInfo &_Info)
{
	////////////////////////
	// 5.1 Surround:
	////////////////////////
	//fL        fC      fR//
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//sL        L       sR//
	////////////////////////

	////////////////////////
	// 6.1 Surround:
	////////////////////////
	//fL       fC       fR//
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//sL        L       sR//
	//                    //
	//                    //
	//          rC        //
	////////////////////////


	////////////////////////
	// 7.1 Surround:
	////////////////////////
	//fL       fC       fR//
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//                    //
	//sL        L       sR//
	//                    //
	//                    //
	//     rL       rR    //
	////////////////////////


	_Info.m_nProssingThreads = 1;
	_Info.m_nChannels = 8;
	_Info.m_FrameLength = 256;
	_Info.m_SampleRate = 48000;
	_Info.m_Speakers[0].m_Position = CVec3Dfp32(-1.0f, -1.0f, 0.0f); // Front Left
	_Info.m_Speakers[1].m_Position = CVec3Dfp32(-1.0f, 1.0f, 0.0f); // Front Right
	_Info.m_Speakers[2].m_Position = CVec3Dfp32(0.0f, -1.0f, 0.0f); // Surround Left
	_Info.m_Speakers[3].m_Position = CVec3Dfp32(0.0f, 1.0f, 0.0f); // Surround Right
	_Info.m_Speakers[4].m_Position = CVec3Dfp32(-1.0f, 0.0f, 0.0f); // Front Center
	_Info.m_Speakers[5].m_Position = CVec3Dfp32(0.0f, 0.0f, 0.0f); // LFE
	_Info.m_Speakers[5].m_bLFE = true;
	_Info.m_Speakers[6].m_Position = CVec3Dfp32(1.0f, -0.5f, 0.0f); // Back Left
	_Info.m_Speakers[7].m_Position = CVec3Dfp32(1.0f, 0.5f, 0.0f); // Back Right
}

void CSoundContext_PS3::Platform_StartStreamingToMixer(uint32 _MixerVoice, CVoice *_pVoice, uint32 _WaveID, fp32 _SampleRate)
{
	M_TRY
	{
#ifndef PLATFORM_CONSOLE
		//if (m_bLogUsage)
			//m_spWaveContext->AddWaveIDFlag(_WaveID, CWC_WAVEIDFLAGS_USED);
#endif

#ifdef PLATFORM_PS3
		CStaticVoice_PS3 *pStaticVoice = GetStaticVoice(_WaveID);

		uint32 MixerVoice = _MixerVoice;
		if (MixerVoice == 0xffffffff)
			return;

		if (pStaticVoice)
		{

			CWaveData Data;
			m_spWaveContext->GetWaveLoadedData(_WaveID, Data);
			
			CSCPS3_Buffer &Voice = m_lVoices[_MixerVoice];
			Voice.Init(this);
				
			Voice.m_Format.m_Data = Data;
			Voice.m_Format.UpdateSize();
			Voice.m_SampleRate = _SampleRate;
			Voice.m_MixerVoice = MixerVoice;
			Voice.m_pVoice = _pVoice;
			Voice.m_bLooping = _pVoice->m_bLooping;
			int64 SamplePos = m_Mixer.Voice_GetSamplePos(MixerVoice);
			Voice.Create(pStaticVoice);
			m_Mixer.Voice_SetSamplePos(MixerVoice,SamplePos);
				
			Voice.m_bDelayed = true;
			Voice.m_WaveID = _WaveID;

			Voice.SubmitPackets();
			Voice.SubmitPackets();

			{
				DLock(m_Streams_Submit_Lock);
				m_Streams_Submit.Insert(Voice);
			}
		}
		else
		{

			CWaveData Data;

			m_spWaveContext->GetWaveLoadedData(_WaveID, Data);

			CSCC_CodecFormat IntFormat;
			m_spWaveContext->GetWaveLoadedData(_WaveID, IntFormat.m_Data);
			IntFormat.UpdateSize();

			CSCPS3_Buffer &Voice = m_lVoices[_MixerVoice];
			Voice.Init(this);

			Voice.m_Format = IntFormat;
			Voice.m_SampleRate = _SampleRate;
			Voice.m_MixerVoice = MixerVoice;
			Voice.m_pVoice = _pVoice;
			Voice.m_bLooping = _pVoice->m_bLooping;
			
			Voice.m_WaveID = _WaveID;

			// Always delay
			Voice.m_bDelayed = true;

			{
				DLock(m_Streams_Start_Lock);
				m_Streams_Start.Insert(Voice);
				m_Thread_SoundStart.m_Event.Signal();
			}
			{
				DLock(m_Streams_Submit_Lock);
				m_Streams_Submit.Insert(Voice);
			}
		}
#endif
		
	}
	M_CATCH(
	catch(CCException)
	{
	}
	)
}

void CSoundContext_PS3::Platform_StopStreamingToMixer(uint32 _MixerVoice)
{
#ifdef PLATFORM_PS3
	CSCPS3_Buffer &Voice = m_lVoices[_MixerVoice];
	Voice.SetWantDelete();
	if (Voice.m_MixerVoice != 0xffffffff)
	{
		m_Mixer.Voice_WantStop(Voice.m_MixerVoice);
	}

	{
		M_LOCK(m_DSLock);
		m_VoicesForDelete.Insert(Voice);
	}

#endif
}

void CSoundContext_PS3::Platform_Init(uint32 _MaxMixerVoices)
{
	int nChannels = 8;

	m_lpStaicVoices.SetLen(m_spWaveContext->GetIDCapacity());
	for (int i = 0; i < m_lpStaicVoices.Len(); ++i)
		m_lpStaicVoices[i] = NULL;

#ifdef PLATFORM_PS3
	m_lVoices.SetLen(_MaxMixerVoices);
	for (int i = 0; i < _MaxMixerVoices; ++i)
	{
		m_lVoices[i].Init(this);
	}
#endif
	
	{
		
#ifdef SC_THREADS

#ifdef PLATFORM_PS3
		m_Thread_Submit.m_pThis = this;
		m_Thread_SoundStart.m_pThis = this;
		m_Thread_Read.m_pThis = this;
//		m_Thread_ReturnBuffers.m_pThis = this;

		if (!m_Thread_Submit.Thread_IsCreated())
		{
			M_TRY
			{
				m_Thread_Submit.Thread_Create(NULL, 65536, MRTC_THREAD_PRIO_TIMECRITICAL);
			}
			M_CATCH(
			catch(CCException)
			{
			}
			)
		}

		if (!m_Thread_SoundStart.Thread_IsCreated())
		{
			M_TRY
			{
				m_Thread_SoundStart.Thread_Create(NULL, 65536, MRTC_THREAD_PRIO_HIGHEST);
			}
			M_CATCH(
			catch(CCException)
			{
			}
			)
		}

		if (!m_Thread_Read.Thread_IsCreated())
		{
			M_TRY
			{
				m_Thread_Read.Thread_Create(NULL, 65536, MRTC_THREAD_PRIO_HIGHEST);
			}
			M_CATCH(
			catch(CCException)
			{
			}
			)
		}

/*		if (!m_Thread_ReturnBuffers.Thread_IsCreated())
		{
			M_TRY
			{
				m_Thread_ReturnBuffers.Thread_Create(NULL, 65536, MRTC_THREAD_PRIO_TIMECRITICAL);
			}
			M_CATCH(
			catch(CCException)
			{
			}
			)
		}*/

#endif

		
#endif
	}

	CreateAll();
}



// -------------------------------------------------------------------

#ifdef PLATFORM_PS3
bool CSoundContext_PS3::UpdateVoiceDelete()
{
	bool bEverythingDeleted = true;

	CSCPS3_BufferDeleteIter Iter = m_VoicesForDelete;
	while (Iter)
	{
		CSCPS3_Buffer *pBuffer = Iter;
		++Iter;
		if (pBuffer->CanDelete())
		{
			pBuffer->m_DeleteLink.Unlink();
			pBuffer->Destroy();
		}
		else
			bEverythingDeleted = false;
	}

	return bEverythingDeleted;
}
#endif

// -------------------------------------------------------------------
void CSoundContext_PS3::Refresh()
{
	DLock(m_InterfaceLock);
#ifdef PLATFORM_PS3
	if (!m_bCreated)
		return;
#endif
	if (m_Recursion) return;
	m_Recursion++;

	M_TRY
	{
#ifdef PLATFORM_PS3

		{
			DUnlock(m_InterfaceLock);
			UpdateVoiceDelete();
		}

		CSuper::Refresh();

#endif
	}
	M_CATCH(
	catch(CCException)
	{
		m_Recursion--;
		throw;
	}
	)
	m_Recursion--;


}

// -------------------------------------------------------------------
aint CSoundContext_PS3::OnMessage(const CSS_Msg& _Msg)
{
	return 0;
}

void CSoundContext_PS3::OnRefresh(int _Context)
{
	Refresh();
}

void CSoundContext_PS3::OnBusy(int _Context)
{
	Refresh();
}


//#ifndef M_RTM
const char **CSoundContext_PS3::GetDebugStrings()
{
	DLock(m_InterfaceLock);
#ifdef PLATFORM_PS3
	if (!m_bCreated)
		return 0;
#endif
#ifdef PLATFORM_PS3

	if (!m_DebugStrTemp)
	{
		m_DebugStrTemp = DNew(char *) char *[mcs_MaxDebugSounds];
		for (int i = 0; i < mcs_MaxDebugSounds - 1; ++i)
		{
			m_DebugStrTemp[i] = DNew(char ) char [256];
			m_DebugStrTemp[i][0] = 0; 
		}
		m_DebugStrTemp[mcs_MaxDebugSounds - 1] = NULL;
	}



	int CurrentVoice = 0;


	const char ** pSuperStrings = CSuper::GetDebugStrings();
	if (pSuperStrings)
	{
		while (*pSuperStrings && m_DebugStrTemp[CurrentVoice])
		{
			strcpy(m_DebugStrTemp[CurrentVoice++], *pSuperStrings);
			++pSuperStrings;
		}
	
	}

	for (int i = CurrentVoice; i < mcs_MaxDebugSounds - 1; ++i)
	{
		m_DebugStrTemp[i][0] = 0; 
	}

	
	return (const char **)m_DebugStrTemp;
#else
	return 0;
#endif
}

//#endif


CSCPS3_Heap::CBlock *CSCPS3_Heap::Alloc(mint _Size, mint _Alignment)
{
	DLock(m_Lock);
	if (_Alignment < m_Align)
		_Alignment = m_Align;

	// Align all allocated sizes to keep alignment in heap
	_Size = (_Size + (m_Align - 1)) & (~(m_Align - 1));
	mint OriginalSize = _Size;
	mint AlignmentSearch = _Alignment;

	// First try to allocate a block that is just the right size and see if we are good alignmentwise

RestartSearch:

	CBlockSize *pSizeClass = (CBlockSize *)m_FreeSizeTree.FindSmallestGreaterThanEqual(_Size);
	
	if (!pSizeClass)
	{
		M_BREAKPOINT;
		return NULL;
	}
	
	CBlock * pBlock = pSizeClass->GetBlock(AlignmentSearch);
	if (!pBlock)
	{
		// No block met out alignment requirement, lets restart search with a size that will allow alignment
		if (_Size != OriginalSize)
			AlignmentSearch = m_Align; // The default alignment
		else
			_Size = OriginalSize + _Alignment;
		goto RestartSearch;
	}

	int BlockSize = pSizeClass->m_Size;
	
	RemoveFreeBlock(pBlock);
	m_FreeMemory -= BlockSize;
	

	if (pBlock->GetMemory() & (_Alignment - 1))
	{
		// We need to put a free block between our block and the previous block
		mint MemoryPos = (pBlock->GetMemory() + (_Alignment - 1)) & (~(_Alignment - 1));
		M_ASSERT(MemoryPos != pBlock->GetMemory(), "Duh");
		int SizeLeftPre = MemoryPos - pBlock->GetMemory();
		m_FreeMemory += SizeLeftPre;
		BlockSize -= SizeLeftPre;

		// Pre block
		{
			// Add remainder of block to free blocks
			CBlock * pNewBlock = m_BlockPool.New();
			
			pNewBlock->SetMemory(pBlock->GetMemory());
			pNewBlock->m_pPrevBlock = pBlock->m_pPrevBlock;
			if (pNewBlock->m_pPrevBlock)
			{
				pNewBlock->m_pPrevBlock->m_pNextBlock = pNewBlock;
			}

			pNewBlock->m_pNextBlock = pBlock;
			pBlock->m_pPrevBlock = pNewBlock;
			
			CBlockSize *pSizeClass = (CBlockSize *)m_FreeSizeTree.FindEqual(SizeLeftPre);
			
			if (!pSizeClass)
			{
				pSizeClass = m_BlockSizePool.New();
				pSizeClass->m_Size = SizeLeftPre;
				m_FreeSizeTree.f_Insert(pSizeClass);
			}
			
			pSizeClass->AddBlock(pNewBlock);
		}
		pBlock->SetMemory(MemoryPos);

		
	}

	{
		int SizeLeft = BlockSize - OriginalSize;
		m_FreeMemory += SizeLeft;
		
		if (SizeLeft > 0)
		{
			// Add remainder of block to free blocks
			CBlock * pNewBlock = m_BlockPool.New();
			
			pNewBlock->SetMemory((mint)((uint8 *)pBlock->GetMemory() + OriginalSize));
			pNewBlock->m_pNextBlock = pBlock->m_pNextBlock;
			if (pNewBlock->m_pNextBlock)
			{
				pNewBlock->m_pNextBlock->m_pPrevBlock = pNewBlock;
			}
			pBlock->m_pNextBlock = pNewBlock;
			pNewBlock->m_pPrevBlock = pBlock;
			
			CBlockSize *pSizeClass = (CBlockSize *)m_FreeSizeTree.FindEqual(SizeLeft);
			
			if (!pSizeClass)
			{
				pSizeClass = m_BlockSizePool.New();
				pSizeClass->m_Size = SizeLeft;
				m_FreeSizeTree.f_Insert(pSizeClass);
			}
			
			pSizeClass->AddBlock(pNewBlock);
		}
	}
	
#ifdef M_Profile
	CheckHeap();
#endif
	static int iAlloc = 0;
	++iAlloc;
	return pBlock;
}

void CSCPS3_Heap::Free(CBlock *&_pBlock)
{

#ifdef M_Profile
	// Reset Free Memory
	mint Size = GetBlockSize(_pBlock);
	for (int i = 0; i < Size >> 2; ++i)
	{
		((uint32 *)_pBlock->GetMemory())[i] = 0xf050050f;
	}
#endif

	DLock(m_Lock);
	M_ASSERT(!_pBlock->m_FreeLink.IsInList(), "Must not be free already");
	int BlockSize = GetBlockSize(_pBlock);
	mint pBlockPos = _pBlock->GetMemory();
	CBlock *pFinalBlock = _pBlock;
	CBlock *pPrevBlock = _pBlock->m_pPrevBlock;
	CBlock *pNextBlock = _pBlock->m_pNextBlock;
	bool bDeleteBlock1 = false;
	bool bDeleteBlock2 = false;
	
	m_FreeMemory += BlockSize;
	
	if (pPrevBlock)
	{
		if (pPrevBlock->m_FreeLink.IsInList())
		{
			BlockSize += GetBlockSize(pPrevBlock);
			pBlockPos = pPrevBlock->GetMemory();
			pFinalBlock = pPrevBlock;
			
			RemoveFreeBlock(pPrevBlock);
			
			bDeleteBlock1 = true;
		}
	}
	
	if (pNextBlock)
	{
		if (pNextBlock->m_FreeLink.IsInList())
		{
			BlockSize += GetBlockSize(pNextBlock);
			
			RemoveFreeBlock(pNextBlock);
			
			bDeleteBlock2 = true;
		}
	}
	
	CBlockSize *pSizeClass = GetSizeBlock(BlockSize);
	
	pSizeClass->AddBlock(pFinalBlock);

	if (bDeleteBlock1)
		DeleteBlock(_pBlock);
	if (bDeleteBlock2)
		DeleteBlock(pNextBlock);

	if (m_bAutoDefrag)
		Defrag();

#ifdef M_Profile
	// Reset Free Memory
	CheckHeap();
#endif
	_pBlock = NULL;
}

void CSCPS3_Heap::ResetNextFreeBlock(CBlock *&_pBlock, mint _MaxSize)
{
	DLock(m_Lock);
	mint ResetThisBlock = Min((mint)16, GetBlockSize(_pBlock));
	if (_pBlock->m_pNextBlock && _pBlock->m_pNextBlock->m_FreeLink.IsInList())
	{
		mint BlockSize = GetBlockSize(_pBlock->m_pNextBlock);
		mint Size = Min(BlockSize + ResetThisBlock, _MaxSize);
		for (int i = 0; i < Size >> 2; ++i)
		{
			((uint32 *)(_pBlock->m_pNextBlock->GetMemory() - ResetThisBlock))[i] = 0xf050050f;
		}
	}
	else
	{
		mint Size = GetBlockSize(_pBlock);
		for (int i = (Size - ResetThisBlock) >> 2; i < Size >> 2; ++i)
		{
			((uint32 *)(_pBlock->GetMemory()))[i] = 0xf050050f;
		}
	}
}

void CSCPS3_Heap::FreeMidBlock(CBlock *_pBlock, mint _Memory, mint _SavedSize)
{
	DLock(m_Lock);
	CBlock *pBlock = _pBlock;
	while (pBlock->GetMemory() < _Memory)
		pBlock = pBlock->m_pNextBlock;
	M_ASSERT(pBlock->GetMemory() == _Memory, "Must be so");
	m_MemorySavedMidBlock -= _SavedSize;

	Free(pBlock);
}

CSCPS3_Heap::CBlock *CSCPS3_Heap::ReturnMidBlock(CBlock *_pBlock, mint _Memory, mint _Size)
{
	DLock(m_Lock);
	CBlock *pBlock = _pBlock;
	while (pBlock->m_pNextBlock->GetMemory() < _Memory)
		pBlock = pBlock->m_pNextBlock;

	// Free block

	m_FreeMemory += _Size;
	m_MemorySavedMidBlock += _Size;
	
	CBlock * pNewBlock = m_BlockPool.New();
	
	pNewBlock->SetMemory(_Memory);
	pNewBlock->m_pPrevBlock = pBlock;
	pNewBlock->m_pNextBlock = pBlock->m_pNextBlock;
	if (pNewBlock->m_pNextBlock)
	{
		pNewBlock->m_pNextBlock->m_pPrevBlock = pNewBlock;
	}
	pBlock->m_pNextBlock = pNewBlock;
	
	CBlockSize *pSizeClass = (CBlockSize *)m_FreeSizeTree.FindEqual(_Size);
	
	if (!pSizeClass)
	{
		pSizeClass = m_BlockSizePool.New();
		pSizeClass->m_Size = _Size;
		m_FreeSizeTree.f_Insert(pSizeClass);
	}
	
	pSizeClass->AddBlock(pNewBlock);

	// Next block

	CBlock * pRightBlock = m_BlockPool.New();
	
	pRightBlock->SetMemory(_Memory + _Size);
	pRightBlock->m_pPrevBlock = pNewBlock;
	pRightBlock->m_pNextBlock = pNewBlock->m_pNextBlock;
	if (pRightBlock->m_pNextBlock)
	{
		pRightBlock->m_pNextBlock->m_pPrevBlock = pRightBlock;
	}
	pNewBlock->m_pNextBlock = pRightBlock;

#ifdef M_Profile
	CheckHeap();
#endif

	return pRightBlock;	
}

CSCPS3_Heap::CSCPS3_Heap()
{
	m_pBlockStart = NULL;
	m_pBlockEnd = NULL;
	m_pFirstBlock = NULL;
}

CSCPS3_Heap::~CSCPS3_Heap()
{	
	RemoveFreeBlock(m_pFirstBlock);
	m_BlockPool.Delete(m_pFirstBlock);
}

void CSCPS3_Heap::Create(void *_pMemory, mint _Blocksize, mint _Alignment, bool _bAutoDefrag)
{
	DLock(m_Lock);
	m_bAutoDefrag = _bAutoDefrag;
	m_Align = _Alignment;
	m_pBlockStart = _pMemory;
	m_pBlockEnd = ((uint8 *)m_pBlockStart + _Blocksize);
	
	m_pFirstBlock = m_BlockPool.New();
	
	m_pFirstBlock->SetMemory((mint)_pMemory);
	m_pFirstBlock->m_pNextBlock = NULL;
	m_pFirstBlock->m_pPrevBlock = NULL;
	
	CBlockSize *pSizeClass = m_BlockSizePool.New();
	pSizeClass->m_Size = _Blocksize;
	m_FreeSizeTree.f_Insert(pSizeClass);

	m_FreeMemory = _Blocksize;
	m_MemorySavedMidBlock = 0;

	pSizeClass->AddBlock(m_pFirstBlock);

#ifdef M_Profile
	CheckHeap();
#endif
}

void CSCPS3_Heap::CheckHeap()
{
	return ;
	DLock(m_Lock);
	// Iterate all blocks
	CBlock *pCurrent = m_pFirstBlock;
	bint bLastFree = false;
	uint32 LastMem = 0;
	if (pCurrent)
		bLastFree = !pCurrent->m_FreeLink.IsInList();
	
	while (pCurrent)
	{
		if (pCurrent->m_FreeLink.IsInList())
		{
			if (bLastFree)
				M_BREAKPOINT;

		}

		bLastFree = pCurrent->m_FreeLink.IsInList();

        if (LastMem > pCurrent->GetMemory())
			M_BREAKPOINT;

		LastMem = pCurrent->GetMemory();

		pCurrent = pCurrent->m_pNextBlock;
	}

	DIdsTreeAVLAligned_Iterator(CBlockSize, m_AVLLink, mint, CBlockSize::CCompare) Iter = m_FreeSizeTree;

	while (Iter)
	{
		DLinkDS_Iter(CBlock, m_FreeLink) Iter2 = Iter->m_FreeList;

		mint BlockSize = GetBlockSize(Iter2);
		mint ShouldBeSize = Iter->m_Size;
		if (BlockSize != ShouldBeSize)
			M_BREAKPOINT;


		++Iter;
	}
}

/*

Defrag strategy:

Loop through all blocks and move those with the greatest need for 

*/

void CSCPS3_Heap::Defrag()
{
	DLock(m_Lock);
	// Iterate all blocks
	CBlock *pCurrent = m_pFirstBlock;
	
	while (pCurrent)
	{

		if (pCurrent->m_FreeLink.IsInList() && pCurrent->m_pNextBlock)
		{
			// Move the next block to this location
			CBlock *pBlockToMove = pCurrent->m_pNextBlock;
			
			M_ASSERT(!pBlockToMove->m_FreeLink.IsInList(), "Internal error: Two sequential free blocks.");
			
			if (pBlockToMove->m_pNextBlock && pBlockToMove->m_pNextBlock->m_FreeLink.IsInList())
			{
				CBlock *pNextBlock = pBlockToMove->m_pNextBlock;
				// The block after the nextblock is free, move the original block and make one of the free blocks
				// |1111|22222|333333|
				// |22222|33333333333|
				// 1 = pCurrent, 2=pBlockToMove, 3=pNextBlock

				int Size = GetBlockSize(pBlockToMove);
				int FreeBlockSize = GetBlockSize(pCurrent) + GetBlockSize(pNextBlock);

				MoveBlock((void *)pCurrent->GetMemory(), (void *)pBlockToMove->GetMemory(), Size);
				pBlockToMove->SetMemory(((mint)pCurrent->GetMemory() + Size));
				CBlockSize *pSizeClass = GetSizeBlock(FreeBlockSize);
				pSizeClass->AddBlock(pBlockToMove);

				// Remove free blocks
				RemoveFreeBlock(pCurrent);
				RemoveFreeBlock(pNextBlock);
				DeleteBlock(pNextBlock);
			}
			else 
			{
				// The next two blocks is allocated, only move the block
				// |11111|222222|
				// |222222|11111|
				// 1 = pCurrent, 2=pBlockToMove

				int Size = GetBlockSize(pBlockToMove);
				int FreeBlockSize = GetBlockSize(pCurrent);

				
				MoveBlock((void *)pCurrent->GetMemory(), (void *)pBlockToMove->GetMemory(), Size);
				pBlockToMove->SetMemory(((mint)pCurrent->GetMemory() + Size));
				CBlockSize *pSizeClass = GetSizeBlock(FreeBlockSize);
				pSizeClass->AddBlock(pBlockToMove);

				RemoveFreeBlock(pCurrent);				
			}
		}

		pCurrent = pCurrent->m_pNextBlock;
	}		
	
}



void CSCPS3_Heap::MoveBlock(void *_To, void*_From, mint _Size)
{
	memcpy(_To, _From, _Size);
}

void CSCPS3_Heap::RemoveFreeBlock(CBlock *_pBlock)
{
	DLock(m_Lock);
	if (_pBlock->m_FreeLink.IsAloneInList())
	{				
		CBlockSize *pSizeClass = (CBlockSize *)m_FreeSizeTree.FindEqual(GetBlockSize(_pBlock));
		M_ASSERT(pSizeClass && pSizeClass->m_FreeList.GetFirst() == _pBlock, "Must be so");
		_pBlock->m_FreeLink.Unlink();
		m_FreeSizeTree.f_Remove(pSizeClass);
		m_BlockSizePool.Delete(pSizeClass);					
	}
	else
	{
		_pBlock->m_FreeLink.Unlink();
	}
}

CSCPS3_Heap::CBlockSize *CSCPS3_Heap::GetSizeBlock(mint _BlockSize)
{
	DLock(m_Lock);
	CBlockSize *pSizeClass = m_FreeSizeTree.FindEqual(_BlockSize);

	if (!pSizeClass)
	{
		pSizeClass = m_BlockSizePool.New();
		pSizeClass->m_Size = _BlockSize;
		m_FreeSizeTree.f_Insert(pSizeClass);
	}
	return pSizeClass;
}

void CSCPS3_Heap::DeleteBlock(CBlock *_pBlock)
{
	DLock(m_Lock);
	if (_pBlock->m_pPrevBlock)
		_pBlock->m_pPrevBlock->m_pNextBlock = _pBlock->m_pNextBlock;
	else
		m_pFirstBlock = _pBlock;

	if (_pBlock->m_pNextBlock)
		_pBlock->m_pNextBlock->m_pPrevBlock = _pBlock->m_pPrevBlock;

	m_BlockPool.Delete(_pBlock);
}

mint CSCPS3_Heap::GetBlockSize(CBlock *_pBlock)
{		
	DLock(m_Lock);
	if (_pBlock->m_pNextBlock)
		return (uint8 *)_pBlock->m_pNextBlock->GetMemory() - (uint8 *)_pBlock->GetMemory();
	else
		return (uint8 *)m_pBlockEnd - (uint8 *)_pBlock->GetMemory();
}


#endif
