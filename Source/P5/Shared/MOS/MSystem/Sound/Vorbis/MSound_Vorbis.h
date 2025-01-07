
#ifndef DInc_MSound_Vorbis_h
#define DInc_MSound_Vorbis_h

#include "../MSound.h"
#include "../MSound_Mixer.h"
#include "../SCMixer/MSound_SCMixer.h"
#include "../../Misc/MPerfGraph.h"

#define DMaxPackets 4

// Enable this for end of buffer debugging
//#define MSND_ENDOFSAMPLEREADDETECT

#ifdef MSND_ENDOFSAMPLEREADDETECT
#include <windows.h>
#endif

// 176 bytes
class CStaticVoice_Vorbis
{
public:
	CStaticVoice_Vorbis(class CSoundContext_Vorbis *_pSCD, int _WaveId);
	~CStaticVoice_Vorbis();

	class CSoundContext_Vorbis *m_pSCD; // 4
	int m_WaveID; // 4
	uint32 m_RefCount;
	uint32 m_bDecoded;
#ifdef MSND_ENDOFSAMPLEREADDETECT

	void *m_pDataAlloc;
	void *m_pData;
	fp32 *GetData()
	{
		return (fp32 *)m_pData;
	}
	mint GetLen()
	{
		return 0;
	}

	void AllocData(mint _nSamples)
	{
		mint OrgSize = _nSamples * sizeof(fp32);
		mint Size = AlignUp(OrgSize, 4096) + 4096;
		m_pDataAlloc = MRTC_SystemInfo::OS_Alloc(Size, 4096);
		m_pData = (((uint8 *)m_pDataAlloc) + Size) - (4096 + OrgSize);

		DWORD OldesProtect = 0;
		if (!VirtualProtect((uint8 *)m_pDataAlloc + Size - 4096, 4096, PAGE_NOACCESS, &OldesProtect))
			M_BREAKPOINT;
	}

#else
	TArray<fp32> m_Data;
	fp32 *GetData()
	{
		return m_Data.GetBasePtr();
	}
	mint GetLen()
	{
		return m_Data.Len();
	}

	void AllocData(mint _nSamples)
	{
		m_Data.SetLen(_nSamples);
	}
#endif


	DLinkD_Link(CStaticVoice_Vorbis, m_Link);

	void LoadVoice(spCSCC_CodecStream _spStream);
};

class CSCVorbis_Buffer
{
public:
	CSCVorbis_Buffer()
	{
		m_pStaticVoice = NULL;
		m_MixerVoice = 0xffffffff;
	}
	CMTime m_StartTime;

	MRTC_CriticalSection m_DatasLock; // 56

	mint m_iWantDelete;

	void SetWantDelete()
	{
		Volatile(m_iWantDelete)++;
	}

	mint WantDelete()
	{
		return Volatile(m_iWantDelete);
	}

	uint32 m_MixerVoice;
	CSoundContext_Mixer::CVoice *m_pVoice;
	fp32 m_SampleRate;

	class CSCStream
	{
	public:
		CSCVorbis_Buffer *m_pParent;
		spCSCC_CodecStream m_spCodec;

		NThread::CMutual m_Lock;

		class CMixerBuffer : public CSC_Mixer_Packet_fp32
		{
		public:
			bint m_bStatic;
			CMixerBuffer(uint32 _nChannels, uint32 _nSamples, fp32 *_pStaticData)
			{
				m_bStatic = _pStaticData != NULL;
				if (m_bStatic)
					m_pData = _pStaticData;
				else
					m_pData = (fp32 *)M_ALLOCALIGN(_nChannels*_nSamples*sizeof(fp32), 128);
				m_nChannels = _nChannels;
				m_nSamples = _nSamples;
			}
			~CMixerBuffer()
			{
				if (!m_bStatic && m_pData)
					MRTC_GetMemoryManager()->Free(m_pData);
			}
			
		};

		union
		{
			DLinkAllocatorDS_List(CSC_Mixer_WorkerContext::CSC_Mixer_Packet, m_Link, CSC_Mixer_WorkerContext::CCustomAllocator) *m_pMixBuffersUnused;
			CMixerBuffer *m_pStaticPacket;
		};
		int64 m_SamplePos;

		bint CanDelete();

		void Clear()
		{
			m_pParent = NULL;
			m_pMixBuffersUnused = NULL;
		}

		CSCStream()
		{
			Clear();
		}
		~CSCStream();

		bint Create2();
		void Destroy();
	};

	uint32 m_WaveID:16;
	uint32 m_bFailedAlloc:1;
	CSCC_CodecFormat m_Format;

	CSCStream m_Stream;

	CStaticVoice_Vorbis *m_pStaticVoice;
	class CSoundContext_Vorbis *m_pSoundContext;

	DLinkD_Link(CSCVorbis_Buffer, m_DecodeLink);
	DLinkD_Link(CSCVorbis_Buffer, m_DeleteLink);

	bint m_bDecoding;
	volatile bint m_bDelayed;
	bint m_bLooping;

	bint CreateUpdate();
	bint GetNewPackets();
	uint32 Decode();

	void Clear();
	void Destroy();
	void Init(class CSoundContext_Vorbis *_pSoundContext);
	~CSCVorbis_Buffer();
	void SetData(void* _pData, int _nSamples, int _SamplePos = 0);
	void Create(CStaticVoice_Vorbis *_pStaticVoice) ;

	bool CanDelete();
};

typedef DLinkD_Iter(CSCVorbis_Buffer, m_DeleteLink) CSCVorbis_BufferDeleteIter;


class CMixerOutputEffect;
// -------------------------------------------------------------------
class CSoundContext_Vorbis : public CSoundContext_Mixer
{
	friend class CStaticVoice_Vorbis;
	friend class CSCVorbis_Buffer;
	friend class CSCVorbis_Buffer::CSCStream;
	friend class CMixerOutputEffect;

	typedef CSoundContext_Mixer CSuper;

	MRTC_DECLARE;

	const static int mcs_MaxDebugSounds = 384;
	char ** m_DebugStrTempVorbis;

	TThinArray<CSCVorbis_Buffer> m_lVoices;
	DLinkD_List(CSCVorbis_Buffer, m_DeleteLink) m_VoicesForDelete;

	MRTC_CriticalSection m_VoicesLock;
	MRTC_CriticalSection m_DSLock;

	int m_bCreated;
	uint32 m_Recursion;

	void *m_pMainThreadID;

	TThinArray<CStaticVoice_Vorbis *> m_lpStaicVoices;

	class CStartSoundThread : public MRTC_Thread
	{
	public:
		CSoundContext_Vorbis *m_pThis;

		NThread::CEventAutoResetReportable m_Event;
		int Thread_Main()
		{
			MRTC_SystemInfo::Thread_SetName("Sound Start");
			MRTC_Thread::m_QuitEvent.ReportTo(&m_Event);
			while (!Thread_IsTerminating())
			{
				m_pThis->Thread_StartSounds();
				m_Event.WaitTimeout(1.0f);
			}
			return 0;
		}
	};

	CStartSoundThread m_Thread_SoundStart;

	class CLoadStaticThread : public MRTC_Thread
	{
	public:
		CSoundContext_Vorbis *m_pThis;

		NThread::CEventAutoResetReportable m_Event;
		int Thread_Main()
		{
			MRTC_SystemInfo::Thread_SetName("Sounds Load Static");
			MRTC_Thread::m_QuitEvent.ReportTo(&m_Event);
			while (!Thread_IsTerminating())
			{
				m_pThis->Thread_LoadStatic();
				m_Event.WaitTimeout(1.0f);
			}
			return 0;
		}
	};

	CLoadStaticThread m_Thread_LoadStatic;

	class CDecodeThread : public MRTC_Thread
	{
	public:
		CSoundContext_Vorbis *m_pThis;

		NThread::CEventAutoResetReportable m_Event;

		int Thread_Main()
		{
			MRTC_SystemInfo::Thread_SetName("Sound Decode");
			MRTC_Thread::m_QuitEvent.ReportTo(&m_Event);
			while (!Thread_IsTerminating())
			{
				m_pThis->Thread_Decode();
				m_Event.WaitTimeout(0.025); // 25 ms to check for packets
			}
			return 0;
		}
	};

	TThinArray<CDecodeThread *> m_Threads_Decode;

	NThread::CMutual m_Static_ToLoad_Lock;
	DLinkD_List(CStaticVoice_Vorbis, m_Link) m_Static_ToLoad;

	NThread::CEventAutoResetReportable m_DecodeEvent;
	NThread::CMutual m_Streams_Decode_Lock;
	DLinkD_List(CSCVorbis_Buffer, m_DecodeLink) m_Streams_Decode;
	DLinkD_List(CSCVorbis_Buffer, m_DecodeLink) m_Streams_CheckNewPackets;
	mint m_nCheckList;

	NThread::CMutual m_Streams_Start_Lock;
	DLinkD_List(CSCVorbis_Buffer, m_DecodeLink) m_Streams_Start;

	

private:

	void Thread_LoadStatic();
	void Thread_Decode();
	void Thread_StartSounds();

	CStaticVoice_Vorbis *GetStaticVoice(int _WaveID);

public:

	CSoundContext_Vorbis();
	~CSoundContext_Vorbis();

	virtual void Create(CStr _Params);
	bint Destroy_Everything();

	virtual void KillThreads();

	virtual void Platform_StartStreamingToMixer(uint32 _MixerVoice, CVoice *_pVoice, uint32 _WaveID, fp32 _SampleRate);
	virtual void Platform_StopStreamingToMixer(uint32 _MixerVoice);
	virtual void Platform_Init(uint32 _MaxMixerVoices);
	bint Platform_IsPlaying(uint32 _MixerVoice);

	// Precache
	virtual void Wave_Precache(int _WaveID);
	virtual void Wave_PrecacheFlush();
	virtual void Wave_PrecacheBegin( int _Count );
	virtual void Wave_PrecacheEnd(uint16 *_pPreCacheOrder, int _nIds);
	virtual	int Wave_GetMemusage(int _WaveID);

	virtual const char **GetDebugStrings();

public:
	bool UpdateVoiceDelete();
	virtual void Refresh();

};

extern CSoundContext_Vorbis *g_XSoundContext;

typedef TPtr<CSoundContext_Vorbis> spCSoundContext_Vorbis;

#endif // DInc_MSound_Vorbis_h
