
#include "PCH.h"

#ifdef PLATFORM_WIN_PC

//#pragma optimize("", off)
//#pragma inline_depth(0)

#include "MSound_Vorbis.h"

MRTC_IMPLEMENT(CSoundContext_Vorbis, CSoundContext);

CSoundContext_Vorbis::CSoundContext_Vorbis()
{
	m_Recursion = 0;
	m_DebugStrTempVorbis = NULL;
	m_nCheckList = 0;
}

CSoundContext_Vorbis::~CSoundContext_Vorbis()
{

	KillThreads();

	for (mint i = 0; i < m_lVoices.Len(); ++i)
	{
		m_lVoices[i].Destroy();
	}
	m_lVoices.Clear();

	for (int i = 0; i < m_lpStaicVoices.Len(); i++)
	{
		if (m_lpStaicVoices[i])
			delete m_lpStaicVoices[i];
	}
	m_lpStaicVoices.Clear();

	if (m_DebugStrTempVorbis)
	{
		for (int i = 0; i < mcs_MaxDebugSounds - 1; ++i)
			delete [] m_DebugStrTempVorbis[i];

		delete [] m_DebugStrTempVorbis;
	}


}

bint CSoundContext_Vorbis::Destroy_Everything()
{
	bint bDone = CSuper::Destroy_Everything();

	if (!UpdateVoiceDelete())
		bDone = false;

	return bDone;
}

void CSoundContext_Vorbis::Create(CStr _Params)
{
	CSuper::Create(_Params);
}

void CSoundContext_Vorbis::KillThreads()
{
	for (mint i = 0; i < m_Threads_Decode.Len(); ++i)
	{
		m_Threads_Decode[i]->Thread_Destroy();
		delete m_Threads_Decode[i];
	}
	m_Threads_Decode.SetLen(0);

	m_Thread_SoundStart.Thread_Destroy();
	CSuper::KillThreads();
}

bint CSoundContext_Vorbis::Platform_IsPlaying(uint32 _MixerVoice)
{
	CSCVorbis_Buffer &Voice = m_lVoices[_MixerVoice];
	if (Voice.m_bFailedAlloc)
		return false;
	return true;
}

void CSoundContext_Vorbis::Platform_StartStreamingToMixer(uint32 _MixerVoice, CVoice *_pVoice, uint32 _WaveID, fp32 _SampleRate)
{

	DLock(m_InterfaceLock);
	CStaticVoice_Vorbis *pStaticVoice = GetStaticVoice(_WaveID);

	// Just stream the sound if the voice hasn't been loaded yet
	if (pStaticVoice && pStaticVoice->m_bDecoded != 1)
	{
		DLock(m_Static_ToLoad_Lock);
		if (pStaticVoice->m_bDecoded != 1)
		{
			if (pStaticVoice->m_Link.IsInList())
			{
				// Check if it's still in the list and put it at the top if it is.
				m_Static_ToLoad.Insert(pStaticVoice);
			}

			pStaticVoice = NULL;
		}
	}

	uint32 MixerVoice = _MixerVoice;
	if (MixerVoice == 0xffffffff)
		return;

	if (pStaticVoice)
	{

		CWaveData Data;
		m_spWaveContext->GetWaveLoadedData(_WaveID, Data);
		
		CSCVorbis_Buffer &Voice = m_lVoices[_MixerVoice];
		Voice.Init(this);
			
		Voice.m_Format.m_Data = Data;
		Voice.m_Format.UpdateSize();
		Voice.m_SampleRate = _SampleRate;
		Voice.m_MixerVoice = MixerVoice;
		Voice.m_pVoice = _pVoice;
		Voice.m_bLooping = _pVoice->m_bLooping;
		Voice.m_bDelayed = false;
		Voice.m_WaveID = _WaveID;
		int64 SamplePos = m_Mixer.Voice_GetSamplePos(MixerVoice);
		// Resets samplepos, temp fix
		Voice.Create(pStaticVoice);
		m_Mixer.Voice_SetSamplePos(MixerVoice,SamplePos);
		// Start playing
		Internal_VoiceUnpause(_pVoice, CSoundContext_Mixer::EPauseSlot_Delayed);
	}
	else
	{
		CWaveData Data;
		m_spWaveContext->GetWaveLoadedData(_WaveID, Data);

		CSCVorbis_Buffer &Voice = m_lVoices[_MixerVoice];

		Voice.Init(this);
		Voice.m_Format.m_Data = Data;
		Voice.m_Format.UpdateSize();
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
	}
}

void CSoundContext_Vorbis::Platform_StopStreamingToMixer(uint32 _MixerVoice)
{
	DLock(m_InterfaceLock);
	CSCVorbis_Buffer &Voice = m_lVoices[_MixerVoice];
	Voice.m_pVoice = NULL;
	if (Voice.m_MixerVoice != 0xffffffff)
	{
		m_Mixer.Voice_WantStop(Voice.m_MixerVoice);
	}

	{
		M_LOCK(m_DSLock);
		m_VoicesForDelete.Insert(Voice);
	}
}

void CSoundContext_Vorbis::Platform_Init(uint32 _MaxMixerVoices)
{
	m_lpStaicVoices.SetLen(m_spWaveContext->GetIDCapacity());
	for (int i = 0; i < m_lpStaicVoices.Len(); ++i)
		m_lpStaicVoices[i] = NULL;

	m_lVoices.SetLen(_MaxMixerVoices);
	for (int i = 0; i < _MaxMixerVoices; ++i)
	{
		m_lVoices[i].Init(this);
	}

	{
		m_Thread_SoundStart.m_pThis = this;
		m_Thread_LoadStatic.m_pThis = this;

		if (!m_Thread_SoundStart.Thread_IsCreated())
		{
			m_Thread_SoundStart.Thread_Create(NULL, 65536, MRTC_THREAD_PRIO_HIGHEST);
		}


		if (!m_Thread_LoadStatic.Thread_IsCreated())
		{
			m_Thread_LoadStatic.Thread_Create(NULL, 65536, MRTC_THREAD_PRIO_BELOWNORMAL);
		}

		uint32 nDecodeThreads = 1;
		uint32 nCPU = MRTC_SystemInfo::MRTC_GetSystemInfo().m_nCPU;

		if (nCPU > 2)
		{
			nDecodeThreads = nCPU - 2;
		}		

		m_Threads_Decode.SetLen(nDecodeThreads);

		for (uint32 i = 0; i < nDecodeThreads; ++i)
		{
			m_Threads_Decode[i] = DNew(CDecodeThread) CDecodeThread;
			if (!m_Threads_Decode[i]->Thread_IsCreated())
			{
				m_Threads_Decode[i]->m_pThis = this;
				m_Threads_Decode[i]->Thread_Create(NULL, 65536, MRTC_THREAD_PRIO_HIGHEST);
				m_DecodeEvent.ReportTo(&m_Threads_Decode[i]->m_Event);
			}
		}
	}
}


void CSoundContext_Vorbis::Wave_PrecacheFlush()
{
	DLock(m_InterfaceLock);
	CSuper::Wave_PrecacheFlush();


	{
		DLock(m_Static_ToLoad_Lock);
		m_Static_ToLoad.Clear();
	}

	// Remove Aramvoices
	for (int i = 0; i < m_lpStaicVoices.Len(); i++)
	{
		CStaticVoice_Vorbis *pToDelete = m_lpStaicVoices[i];
		if(pToDelete && !pToDelete->m_RefCount)
		{
			bint bForceDelete = 0;
			{
				DLock(m_Static_ToLoad_Lock);
				while (pToDelete->m_bDecoded == 2)
				{
					DUnlock(m_Static_ToLoad_Lock);
					MRTC_SystemInfo::OS_Sleep(10);
				}
				if (pToDelete->m_bDecoded == 0)
					bForceDelete = true;
			}

			CSC_IDInfo* pInfo = &GetWCIDInfo()->m_pWCIDInfo[pToDelete->m_WaveID];
			if (bForceDelete || (!(m_spWaveContext->GetWaveIDFlag(pToDelete->m_WaveID) & CWC_WAVEIDFLAGS_PRECACHE) || !(pInfo->m_Fresh & 1)))
				delete pToDelete;
		}
	}
}

void CSoundContext_Vorbis::Wave_PrecacheBegin( int _Count )
{
	DLock(m_InterfaceLock);
	CSuper::Wave_PrecacheBegin( _Count );
}

void CSoundContext_Vorbis::Wave_PrecacheEnd(uint16 *_pPreCacheOrder, int _nIds)
{
	DLock(m_InterfaceLock);
	CSuper::Wave_PrecacheEnd(_pPreCacheOrder, _nIds);
}

void CSoundContext_Vorbis::Wave_Precache(int _WaveID)
{
	DLock(m_InterfaceLock);

	MSCOPE(CSoundContext_Vorbis::Wave_Precache, SCXbox);
//	return -1;
	// M_CALLGRAPH;
	CStaticVoice_Vorbis *pStaticVoice = NULL;
	M_TRY
	{
		if (_WaveID < 0)
			return;

#if 1 // Only support streamed sounds for now
		CWaveData WaveData;
		m_spWaveContext->GetWaveLoadedData(_WaveID, WaveData);
		uint32 Flags = WaveData.Get_Flags();

		if (!(WaveData.Get_Flags() & ESC_CWaveDataFlags_Stream))
		{

			pStaticVoice = m_lpStaicVoices[_WaveID];

			if (!pStaticVoice)
			{
				pStaticVoice = DNew(CStaticVoice_Vorbis) CStaticVoice_Vorbis(this, _WaveID);

				{
					DLock(m_Static_ToLoad_Lock);
					m_Static_ToLoad.Insert(pStaticVoice);
					m_Thread_LoadStatic.m_Event.Signal();
				}

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

int CSoundContext_Vorbis::Wave_GetMemusage(int _WaveID)
{
	DLock(m_InterfaceLock);
	CStaticVoice_Vorbis *pStaticVoice = m_lpStaicVoices[_WaveID];

	if (pStaticVoice)
		return (int)(mint)(AlignUp(pStaticVoice->GetLen() * sizeof(fp32), 16) + AlignUp(sizeof(CStaticVoice_Vorbis), 16) + 4 + 8*2);

	return 0;
}
#ifdef PLATFORM_WIN
#include <windows.h>
#endif

const char **CSoundContext_Vorbis::GetDebugStrings()
{
	DLock(m_InterfaceLock);

	if (!m_DebugStrTempVorbis)
	{
		m_DebugStrTempVorbis = DNew(char *) char *[mcs_MaxDebugSounds];
		for (int i = 0; i < mcs_MaxDebugSounds - 1; ++i)
		{
			m_DebugStrTempVorbis[i] = DNew(char ) char [256];
			m_DebugStrTempVorbis[i][0] = 0; 
		}
		m_DebugStrTempVorbis[mcs_MaxDebugSounds - 1] = NULL;
	}


	int CurrentVoice = 0;

#ifdef PLATFORM_WIN
	class CThreadTimes : public CReferenceCount
	{
	public:
		#define mcs_HistorySize 64

		CThreadTimes()
		{
			m_LastUpdate = CMTime::GetCPU();
			m_LastHistory = 0;
		
			for (int i = 0; i < mcs_HistorySize; i++)
			{
				m_History[i] = 0.0;
				m_HistoryTime[i] = 0.0;
				m_HistoryLeft[i] = 0.0;
				m_HistoryRight[i] = 0.0;
			}

			m_HistorySum = 0.0;
			m_HistoryTimeSum = 0.0;

		}
		~CThreadTimes()
		{

		}

		CMTime m_LastUpdate;

		int64 m_LastKernelTime;
		int64 m_LastUserTime;		
		int m_LastHistory;

		fp64 m_ThreadTime;
		fp64 m_ThreadTimeMean;

		fp32 m_UpdatePeriod;

		fp64 m_History[mcs_HistorySize];
		fp64 m_HistorySum;
		fp64 m_HistoryTime[mcs_HistorySize];
		fp64 m_HistoryTimeSum;
		fp64 m_HistoryLeft[mcs_HistorySize];
		fp64 m_HistoryRight[mcs_HistorySize];

		fp64 m_LeftSum;
		fp64 m_RightSum;
		fp64 m_Left;
		fp64 m_Right;

		fp32 GetHighestLeft()
		{
			fp32 Highest = -100;
			for (int i = 0; i < mcs_HistorySize; ++i)
			{
				if (m_HistoryLeft[i] > Highest)
					Highest = m_HistoryLeft[i];
			}
			return Highest;
		}
		fp32 GetHighestRight()
		{
			fp32 Highest = -100;
			for (int i = 0; i < mcs_HistorySize; ++i)
			{
				if (m_HistoryRight[i] > Highest)
					Highest = m_HistoryRight[i];
			}
			return Highest;
		}

		void Update(void *_pThread, fp32 _PeakLeft, fp32 _PeakRight)
		{
			const static fp64 FileTimeFreq = 10000000.0;
			CMTime Current = CMTime::GetCPU();
			m_UpdatePeriod = (Current - m_LastUpdate).GetTime();
			m_LastUpdate = Current;

			int64 CreationTime;
			int64 ExitTime;
			int64 KernelTime;
			int64 UserTime;
			
			GetThreadTimes(_pThread,(FILETIME *)&CreationTime, (FILETIME *)&ExitTime, (FILETIME *)&KernelTime, (FILETIME *)&UserTime);

			m_ThreadTime = (((UserTime - m_LastUserTime) + (KernelTime - m_LastKernelTime)) / FileTimeFreq) / m_UpdatePeriod;

			int Sub = m_LastHistory + 1;
			if (Sub >= mcs_HistorySize)
				Sub = 0;
			m_HistorySum -= m_History[Sub];
			m_HistoryTimeSum -= m_HistoryTime[Sub];
			m_LeftSum -= m_HistoryLeft[Sub];
			m_RightSum -= m_HistoryRight[Sub];
			m_History[m_LastHistory] = ((UserTime - m_LastUserTime) + (KernelTime - m_LastKernelTime)) / FileTimeFreq;
			m_HistoryTime[m_LastHistory] = m_UpdatePeriod;
			m_HistoryLeft[Sub] = _PeakLeft;
			m_HistoryRight[Sub] = _PeakRight;
			m_HistorySum += m_History[m_LastHistory];
			m_HistoryTimeSum += m_HistoryTime[m_LastHistory];
			m_LeftSum += m_HistoryLeft[m_LastHistory];
			m_RightSum += m_HistoryRight[m_LastHistory];

			m_Left = m_LeftSum / (fp64)mcs_HistorySize;
			m_Right = m_RightSum / (fp64)mcs_HistorySize;

			m_LastHistory = Sub;

			m_ThreadTimeMean = m_HistorySum / m_HistoryTimeSum;
			
			m_LastKernelTime = KernelTime;
			m_LastUserTime = UserTime;

		}
	};

	static CThreadTimes SoundStart;
	static CThreadTimes StaticLoad;
	static TThinArray< TPtr<CThreadTimes> > Decode;

	uint32 nDecode = m_Threads_Decode.Len();

	if (Decode.Len() != nDecode)
	{
		uint32 OldLen = Decode.Len();
		Decode.SetLen(nDecode);
		for (mint i = OldLen; i < nDecode; ++i)
		{
			Decode[i] = MNew(CThreadTimes);
		}
	}

	SoundStart.Update(m_Thread_SoundStart.Thread_GetHandle(), 0, 0);
	StaticLoad.Update(m_Thread_LoadStatic.Thread_GetHandle(), 0, 0);

	sprintf(m_DebugStrTempVorbis[CurrentVoice++], "Start Sounds: %03.2lf %%",
		SoundStart.m_ThreadTimeMean * 100.0
		);

	sprintf(m_DebugStrTempVorbis[CurrentVoice++], "Load Staic: %03.2lf %%",
		StaticLoad.m_ThreadTimeMean * 100.0
		);
	for (uint32 i = 0; i < nDecode; ++i)
	{
		Decode[i]->Update(m_Threads_Decode[i]->Thread_GetHandle(), 0, 0);
		sprintf(m_DebugStrTempVorbis[CurrentVoice++], "Decode(%d): %03.2lf %%", i,
			Decode[i]->m_ThreadTimeMean * 100.0
			);

	}
#endif

	const char ** pSuperStrings = CSuper::GetDebugStrings();
	if (pSuperStrings)
	{
		while (*pSuperStrings && m_DebugStrTempVorbis[CurrentVoice])
		{
			strcpy(m_DebugStrTempVorbis[CurrentVoice++], *pSuperStrings);
			++pSuperStrings;
		}
	
	}

	for (int i = CurrentVoice; i < mcs_MaxDebugSounds - 1; ++i)
	{
		m_DebugStrTempVorbis[i][0] = 0; 
	}

	
	return (const char **)m_DebugStrTempVorbis;
}

bool CSoundContext_Vorbis::UpdateVoiceDelete()
{
	bool bEverythingDeleted = true;

	CSCVorbis_BufferDeleteIter Iter = m_VoicesForDelete;
	while (Iter)
	{
		CSCVorbis_Buffer *pBuffer = Iter;
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

void CSoundContext_Vorbis::Refresh()
{
	DLock(m_InterfaceLock);
	if (m_Recursion) 
		return;
	m_Recursion++;

	M_TRY
	{
		{
			DUnlock(m_InterfaceLock);
			UpdateVoiceDelete();
		}
		CSuper::Refresh();
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


void CSoundContext_Vorbis::Thread_LoadStatic()
{
	// Protect against deleting static voices
	CStaticVoice_Vorbis *pStatic = NULL;
	{
		DLock(m_Static_ToLoad_Lock);
		pStatic = m_Static_ToLoad.Pop();
		if (pStatic)
			pStatic->m_bDecoded = 2;
	}
	
	while (pStatic)
	{
		// Guard for this waveid becoming invalid
		try
		{
			DLock(m_spWaveContext->m_Lock);
			if (m_spWaveContext->IsWaveValid(pStatic->m_WaveID))
			{
				spCSCC_CodecStream spStream;
				{
					spStream = m_spWaveContext->OpenStream(pStatic->m_WaveID, 0);
				}

				if (spStream)
				{
					pStatic->LoadVoice(spStream);
					pStatic->m_bDecoded = 1;
				}
				else
					pStatic->m_bDecoded = 0;
			}
			else
				pStatic->m_bDecoded = 0;
		}
		catch (CCException)
		{
			pStatic->m_bDecoded = 0;
		}

		{
			DLock(m_Static_ToLoad_Lock);
			pStatic = m_Static_ToLoad.Pop();
			if (pStatic)
				pStatic->m_bDecoded = 2;
		}
	}

}

void CSoundContext_Vorbis::Thread_StartSounds()
{
	DLock(m_Streams_Start_Lock);
	DLinkD_Iter(CSCVorbis_Buffer, m_DecodeLink) Iter = m_Streams_Start;

	while (Iter)
	{
		CSCVorbis_Buffer *pBuf = Iter;
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
			DLock(m_Streams_Decode_Lock);
			m_Streams_Decode.Insert(pBuf);
			m_DecodeEvent.Signal();
		}
	}
}

void CSoundContext_Vorbis::Thread_Decode()
{
	while (1)
	{

		bint bDidDecode = false;
		// Decode
		CSCVorbis_Buffer *pToDecode = NULL;
		{
			DLock(m_Streams_Decode_Lock);
			pToDecode = m_Streams_Decode.Pop();
			if (pToDecode)
				++pToDecode->m_bDecoding;
		}

		while (pToDecode)
		{
			if (pToDecode->WantDelete())
			{
				DLock(m_Streams_Decode_Lock);
				--pToDecode->m_bDecoding;
				pToDecode = m_Streams_Decode.Pop();
				if (pToDecode)
					++pToDecode->m_bDecoding;
			}
			else
			{
				uint32 DecodeRet = pToDecode->Decode();
				{
					DLock(m_Streams_Decode_Lock);
					--pToDecode->m_bDecoding;
					if (DecodeRet != 1)
					{
					m_Streams_CheckNewPackets.Insert(pToDecode);
						++m_nCheckList;
					}
					if (DecodeRet != 2)
						bDidDecode = true;
					pToDecode = m_Streams_Decode.Pop();
					if (pToDecode)
						++pToDecode->m_bDecoding;
				}
			}

		}

		// Check for new packets
		{
			DLock(m_Streams_Decode_Lock);
			pToDecode = m_Streams_CheckNewPackets.Pop();
			if (pToDecode)
			{
				--m_nCheckList;
				++pToDecode->m_bDecoding;
			}
		}

		int32 nToCheck = m_nCheckList;

		while (pToDecode)
		{
			--nToCheck;
			if (pToDecode->WantDelete())
			{
				DLock(m_Streams_Decode_Lock);
				--pToDecode->m_bDecoding;
				if (nToCheck > 0)
				{
					pToDecode = m_Streams_CheckNewPackets.Pop();
					if (pToDecode)
					{
						--m_nCheckList;
						++pToDecode->m_bDecoding;
					}
				}
				else
					pToDecode = NULL;
			}
			else
			{
				bint bNewPackets = pToDecode->GetNewPackets();

				DLock(m_Streams_Decode_Lock);
				--pToDecode->m_bDecoding;
				if (bNewPackets)
					m_Streams_Decode.Insert(pToDecode);
				else
				{
					m_Streams_CheckNewPackets.Insert(pToDecode);
					++m_nCheckList;
				}
				if (nToCheck > 0)
				{
					pToDecode = m_Streams_CheckNewPackets.Pop();
					if (pToDecode)
					{
						--m_nCheckList;
						++pToDecode->m_bDecoding;
					}
				}
				else
					pToDecode = NULL;
			}

		}

		// Nothing more to do
		if (!bDidDecode)
			break;
	}
}



CStaticVoice_Vorbis *CSoundContext_Vorbis::GetStaticVoice(int _WaveID)
{
	CStaticVoice_Vorbis *pStaticVoice = m_lpStaicVoices[_WaveID];
	CSC_IDInfo* pInfo = &GetWCIDInfo()->m_pWCIDInfo[_WaveID];

	// If not found, try to precache it
	if (!pStaticVoice || !(pInfo->m_Fresh & 1))
		Wave_Precache(_WaveID);

	return m_lpStaicVoices[_WaveID];
}

void CSCVorbis_Buffer::Clear()
{
	m_bFailedAlloc = false;
	m_pVoice = NULL;
	if (m_pStaticVoice)
		--m_pStaticVoice->m_RefCount;
	m_pStaticVoice = NULL;
	m_bDelayed = false;
	m_iWantDelete = 0;
	m_bLooping = NULL;
	m_bDecoding = false;
}

void CSCVorbis_Buffer::Destroy()
{
	M_ASSERT(CanDelete(), "Must be able to delete");

	m_Stream.Destroy();
	Clear();
	if (m_MixerVoice != 0xffffffff)
	{
		m_pSoundContext->m_Mixer.Voice_Stop(m_MixerVoice);
		m_MixerVoice = 0xffffffff;
	}
}

void CSCVorbis_Buffer::Init(class CSoundContext_Vorbis *_pSoundContext)
{
	m_pSoundContext = _pSoundContext;
	m_Stream.m_pParent = this;
	Clear();
}

CSCVorbis_Buffer::~CSCVorbis_Buffer()
{
	Destroy();
}

#if 1

bint CSCVorbis_Buffer::CreateUpdate()
{
	CSCStream *pStream = &m_Stream;
	if (!pStream->m_spCodec)
	{
		spCSCC_CodecStream spStream;
		{
			spStream = m_pSoundContext->m_spWaveContext->OpenStream(m_WaveID, 0);
		}
		m_Stream.Create2();
		m_Stream.m_spCodec = spStream;

		MSCOPE(sspBuf->Create, IGNORE);
		Create(NULL);
	}

	return true;
}

bint CSCVorbis_Buffer::GetNewPackets()
{
#ifdef M_Profile
	if (m_pStaticVoice)
		M_BREAKPOINT;
#endif
	CSCStream *pStream = &m_Stream;
	if (!pStream->m_pMixBuffersUnused)
		return false;
	// Check for Mixer packets that has finished playing and add them to the list of free packets
	{
		while (1)
		{
			CSCStream::CMixerBuffer *pBuf = (CSCStream::CMixerBuffer *)m_pSoundContext->m_Mixer.Voice_PopFinishedPacket(m_MixerVoice);
			if (pBuf)
			{
				DLock(pStream->m_Lock);

				pStream->m_pMixBuffersUnused->Insert(pBuf);
			}
			else
				break;
		}

	}

	{
		DLock(pStream->m_Lock);
		return !pStream->m_pMixBuffersUnused->IsEmpty();
	}
}

bint CSCVorbis_Buffer::CSCStream::CanDelete()
{
	return true;
}

uint32 CSCVorbis_Buffer::Decode()
{

	CSCStream *pStream = &m_Stream;

	if (!pStream->m_pMixBuffersUnused)
		return 1;

	// Get all new packets available
	GetNewPackets();

	CSCStream::CMixerBuffer *pBuffer;
	{
		DLock(pStream->m_Lock);
		pBuffer = (CSCStream::CMixerBuffer *)pStream->m_pMixBuffersUnused->Pop();
	}
	if (pBuffer)
	{
		mint nDecoded = pStream->m_spCodec->m_spCodec->GetData(pBuffer->m_pData, pBuffer->m_nSamples, m_bLooping);
		if (m_bLooping)
		{
			M_ASSERT(nDecoded, "Must be");
		}
		else
		{
			// Sound has finished decoding to end
			if (!nDecoded)
			{
				DLock(pStream->m_Lock);
				pStream->m_pMixBuffersUnused->Push(pBuffer);
				return 1;
			}
		}

		m_pSoundContext->m_Mixer.Voice_SubmitPacket(m_MixerVoice, pBuffer);
		if (m_bDelayed)
		{
			m_bDelayed = false;
			if (m_pVoice)
				m_pSoundContext->Internal_VoiceUnpause(m_pVoice, CSoundContext_Mixer::EPauseSlot_Delayed);
		}

		return 0;
	}
	else
		return 2;

}


bint CSCVorbis_Buffer::CSCStream::Create2()
{

	CStaticVoice_Vorbis *pStaticVoice = m_pParent->m_pStaticVoice;

	if (pStaticVoice)
	{
		CMixerBuffer *pNew = CSC_Mixer::Voice_AllocMixerPacket<CMixerBuffer>(m_pParent->m_Format.m_Data.GetChannels(), m_pParent->m_Format.m_Data.GetNumSamples(), pStaticVoice->GetData());
		if (pNew)
		{
			pNew->m_bCircular = true;
			m_pParent->m_pSoundContext->m_Mixer.Voice_SubmitPacket(m_pParent->m_MixerVoice, pNew);
			m_pStaticPacket = pNew;
		}
		else
		{
			m_pParent->m_bFailedAlloc = true;
		}
//		m_pMixBuffersUnused->Insert(pNew);
	}
	else
	{
		m_pMixBuffersUnused = CSC_Mixer_WorkerContext::New< DLinkAllocatorDS_List(CSC_Mixer_WorkerContext::CSC_Mixer_Packet, m_Link, CSC_Mixer_WorkerContext::CCustomAllocator) >();
		if (m_pMixBuffersUnused)
		// Create three mixbuffers for good ovelapping
		{
			// Buffer 8 frames of samples (4 KB in 48000 KHz case)
			uint32 VoiceSampelRate = m_pParent->m_Format.m_Data.GetSampleRate();
			uint32 nSamplesPerBuffer = VoiceSampelRate / 10; // Each buffer is 100 ms
			nSamplesPerBuffer = AlignUp(nSamplesPerBuffer, 128);

			for (int i = 0; i < 3; ++i)
			{
				CMixerBuffer *pNew = CSC_Mixer::Voice_AllocMixerPacket<CMixerBuffer>(m_pParent->m_Format.m_Data.GetChannels(), nSamplesPerBuffer, (fp32 *)NULL);
				if (pNew)
					m_pMixBuffersUnused->Insert(pNew);
				else
					m_pParent->m_bFailedAlloc = true;
			}
		}
		else 
		{
			m_pParent->m_bFailedAlloc = true;
			return false;
		}
	}
	return true;
}

CSCVorbis_Buffer::CSCStream::~CSCStream()
{
	Destroy();
}

void CSCVorbis_Buffer::CSCStream::Destroy()
{
	if (m_spCodec)
	{
		m_spCodec = NULL;
	}

	if (m_pParent && m_pParent->m_MixerVoice != 0xffffffff)
	{
		m_pParent->m_pSoundContext->m_Mixer.Voice_FlushPackets(m_pParent->m_MixerVoice);

		CSCStream::CMixerBuffer *pBuf = (CSCStream::CMixerBuffer *)m_pParent->m_pSoundContext->m_Mixer.Voice_PopFinishedPacket(m_pParent->m_MixerVoice);
		if (!m_pParent->m_pStaticVoice)
		{
			while (pBuf)
			{
				CSC_Mixer::Voice_FreeMixerPacket(pBuf);
				pBuf = (CSCStream::CMixerBuffer *)m_pParent->m_pSoundContext->m_Mixer.Voice_PopFinishedPacket(m_pParent->m_MixerVoice);
			}
		}
	}
	
	if (m_pParent && m_pParent->m_pStaticVoice)
	{
		CSC_Mixer::Voice_FreeMixerPacket(m_pStaticPacket);
		m_pStaticPacket = NULL;
	}
	else
	{
		if (m_pMixBuffersUnused)
		{
			CSCStream::CMixerBuffer *pBuf = (CSCStream::CMixerBuffer *)m_pMixBuffersUnused->Pop();
			while (pBuf)
			{
				CSC_Mixer::Voice_FreeMixerPacket(pBuf);
				pBuf = (CSCStream::CMixerBuffer *)m_pMixBuffersUnused->Pop();
			}

			CSC_Mixer_WorkerContext::Delete< DLinkAllocatorDS_List(CSC_Mixer_WorkerContext::CSC_Mixer_Packet, m_Link, CSC_Mixer_WorkerContext::CCustomAllocator) >(m_pMixBuffersUnused);
			m_pMixBuffersUnused = NULL;
		}
	}


	Clear();
}



void CSCVorbis_Buffer::Create(CStaticVoice_Vorbis *_pStaticVoice)
{

	if (m_pStaticVoice)
		--m_pStaticVoice->m_RefCount;
	m_pStaticVoice = _pStaticVoice;

	if (m_pStaticVoice)
	{
		++m_pStaticVoice->m_RefCount;
		m_Stream.Create2();
	}
	else
	{

	}
	bint bLooping = false;
	if (m_pVoice)
		bLooping = m_pVoice->m_bLooping;
	m_pSoundContext->m_Mixer.Voice_SetLoop(m_MixerVoice, bLooping, 0, 0, 0);
	m_pSoundContext->m_Mixer.Voice_Setup(m_MixerVoice, m_Format.m_Data.GetChannels(), m_Format.m_Data.GetNumSamples());
}

bool CSCVorbis_Buffer::CanDelete()
{
	if (m_MixerVoice != 0xffffffff && !m_pSoundContext->m_Mixer.Voice_IsStopped(m_MixerVoice))
	{
		return false;
	}
	else if (!m_iWantDelete)
	{
		SetWantDelete();
	}
	{
		DLock(m_pSoundContext->m_Streams_Start_Lock);
		{
			DLock(m_pSoundContext->m_Streams_Decode_Lock);
			{
				if (m_DecodeLink.IsInList())
					return false;				
				if (m_bDecoding)
				{
					return false;
				}
			}
		}
	}

	if (!m_Stream.CanDelete())
		return false;

	if (m_MixerVoice == 0xffffffff)
		return true;

	return true; 
}


CStaticVoice_Vorbis::CStaticVoice_Vorbis(CSoundContext_Vorbis *_pSCD, int _WaveId)
{
	m_WaveID = _WaveId;
	m_pSCD = _pSCD;
	m_pSCD->m_lpStaicVoices[_WaveId] = this;
	m_RefCount = 0;
	m_bDecoded = 0;
}

CStaticVoice_Vorbis::~CStaticVoice_Vorbis()
{
	m_pSCD->m_lpStaicVoices[m_WaveID] = NULL;
}


void CStaticVoice_Vorbis::LoadVoice(spCSCC_CodecStream _spStream)
{
	uint32 nChannels = _spStream->m_spCodec->m_Format.m_Data.GetChannels();
	uint32 nSamples = _spStream->m_spCodec->m_Format.m_Data.GetNumSamples();
	AllocData(nSamples * nChannels + 4); // Alloc 4 extra fp32 because unaligned reads will need to read extra data

	fp32 *pData = GetData();
	if (pData)
	{
		mint nDecoded = _spStream->m_spCodec->GetData(pData, nSamples, false);
		M_ASSERT(nDecoded == nSamples, "Should match");
	}
	
}


#endif // #if 0

#endif // PLATFORM_WIN_PC
