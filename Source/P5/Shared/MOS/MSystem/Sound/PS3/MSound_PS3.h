

#ifndef DInc_MSound_PS3_h
#define DInc_MSound_PS3_h

#include "../MSound.h"
#include "../MSound_Mixer.h"
#include "../SCMixer/MSound_SCMixer.h"

#include "../../Misc/MPerfGraph.h"

#ifdef PLATFORM_PS3
#include <sys/event.h>
#include <cell/audio.h>
#include <cell/codec/libatrac3plus.h>
#endif

#define DMaxPackets 4

class CSCPS3_Heap
{
private:
	class CBlockSize;

public:

	class CBlockInfo
	{
	public:
		virtual mint GetAlignment() pure;

	};

	class CBlock
	{
		friend class CSCPS3_Heap;
		friend class CBlockSize;
	private:
		DLinkDS_Link(CBlock, m_FreeLink);
		CBlock *m_pPrevBlock;
		CBlock *m_pNextBlock; // 8 bytes here
	public:
		uint32 m_Memory0:31; // Aligned on
		uint32 m_Free:1; // Aligned on
		mint GetMemory()
		{
			return m_Memory0 << 1;
		}
		void SetMemory(mint _Memory)
		{
			m_Memory0 = _Memory >> 1;
		}
//		CBlockInfo *m_pBlockInfo;
	};

private:

	TCPool<CBlock> m_BlockPool;

	class CBlockSize
	{
	public:

		static CBlockSize *BlockSizeFromFreeList(void *_pFreeList)
		{
			M_OFFSET(CBlockSize, m_FreeList, Offset);
//			int Offset = (uint8*)&((CBlockSize *)(0))->m_FreeList - (uint8*)((CBlockSize *)(0));

			return (CBlockSize *)((uint8*)_pFreeList - Offset);
		}

		class CCompare
		{
		public:
			DIdsPInlineS static aint Compare(const CBlockSize *_pFirst, const CBlockSize *_pSecond, void *_pContext)
			{
				return (aint)_pFirst->m_Size - (aint)_pSecond->m_Size;
			}

			DIdsPInlineS static aint Compare(const CBlockSize *_pTest, mint _Key, void *_pContext)
			{
				return (aint)_pTest->m_Size - (aint)_Key;
			}
		};

		DAVLAligned_Link(CBlockSize, m_AVLLink, mint, CCompare);

		DLinkDS_List(CBlock, m_FreeLink) m_FreeList;
		mint m_Size;

		CBlockSize()
		{
		}
		~CBlockSize()
		{
			M_ASSERT(m_FreeList.IsEmpty(), "MUust");
		}

		CBlock *GetBlock(mint _Alignment)
		{
			DLinkDS_Iter(CBlock, m_FreeLink) Iter = m_FreeList;
			while (Iter)
			{
				CBlock *pBlock = Iter;

				if (!(pBlock->GetMemory() & (_Alignment - 1)))
				{
					return pBlock;
				}
				++Iter;
			}

			return NULL;
		}

		void AddBlock(CBlock *_pBlock)
		{
//			m_FreeList.InsertSorted(_pBlock);
			m_FreeList.Push(_pBlock);
		}
	};

	TCPool<CBlockSize> m_BlockSizePool;

	DAVLAligned_Tree(CBlockSize, m_AVLLink, mint, CBlockSize::CCompare) m_FreeSizeTree;
	NThread::CMutual m_Lock;

	void *m_pBlockStart;
	void *m_pBlockEnd;
	CBlock *m_pFirstBlock;
	mint m_Align;
	mint m_bAutoDefrag:1;
	mint m_FreeMemory;
	mint m_MemorySavedMidBlock;

	void MoveBlock(void *_To, void*_From, mint _Size);

	void RemoveFreeBlock(CBlock *_pBlock);

	CBlockSize *GetSizeBlock(mint _BlockSize);

	void DeleteBlock(CBlock *_pBlock);

	void CheckHeap();

public:


	CSCPS3_Heap();
	~CSCPS3_Heap();

	mint GetFreeMemory()
	{
		DLock(m_Lock);
		return m_FreeMemory;
	}
	mint GetLargestFreeBlock()
	{
		DLock(m_Lock);
		CBlockSize *pLargest = m_FreeSizeTree.FindLargest();
		if (pLargest)
		{
			return pLargest->m_Size;
		}
		return 0;
	}

	mint GetSavedMidBlock()
	{
		DLock(m_Lock);
		return m_MemorySavedMidBlock;
	}

	void Defrag();

	mint GetBlockSize(CBlock *_pBlock);

	CBlock *Alloc(mint _Size, mint _Alignment);
	void FreeMidBlock(CBlock *_pBlock, mint _Memory, mint _SavedSize);
	void Free(CBlock *&_pBlock);
	void ResetNextFreeBlock(CBlock *&_pBlock, mint _MaxSize);
	CBlock *ReturnMidBlock(CBlock *_pBlock, mint _Memory, mint _Size);
	void Create(void *_pMemory, mint _Blocksize, mint _Alignment, bool _bAutoDefrag);

};

class CStreamData_PS3
{
public:
	CStreamData_PS3()
	{
		m_CurrentInterleavePos = 0;
		m_ChannelIndex = 0;
		m_DataSize = 0;
		m_HeaderDataSize = 0;
	}
	uint32 m_CurrentInterleavePos:24;
	uint32 m_ChannelIndex:4;
	uint32 m_DataSize:24;
	uint32 m_HeaderDataSize:8;
	TThinArray<uint8> m_Data;
};

class CStreamsData_PS3
{
public:
	TThinArray<CStreamData_PS3> m_Streams;
	uint32 m_QualityIndex:3;
	uint32 m_Stride:29;
	uint32 GetNumChannels();
};

// 176 bytes
class CStaticVoice_PS3
{
public:
	CStaticVoice_PS3(class CSoundContext_PS3 *_pSCD, int _WaveId);
	~CStaticVoice_PS3();

	class CSoundContext_PS3 *m_pSCD; // 4
	int m_WaveID; // 4

	CStreamsData_PS3 m_StreamsData;
	uint32 m_VoiceDataSize;
	uint32 m_RefCount;

	void LoadVoice(CCFile *_pFile);
};

#ifdef PLATFORM_PS3
class CSCPS3_Buffer
{
public:
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
		CSCPS3_Buffer *m_pParent;
		spCSCC_CodecStream m_spCodec;
		class CSCBuffer
		{
		public:
			CSCBuffer()
			{
				m_Request.m_pBuffer = NULL;
			}
			
			CAsyncRequest m_Request;
			CSCPS3_Buffer *m_pParent;
			~CSCBuffer()
			{
			}
			mint m_BufferSize;
			DLinkD_Link(CSCBuffer, m_Link);
		};


		NThread::CMutual m_Lock;

		DLinkD_List(CSCBuffer, m_Link) m_StreamsToSubmitToMixer;
		DLinkD_List(CSCBuffer, m_Link) m_StreamsToSubmit;
		DLinkD_List(CSCBuffer, m_Link) m_StreamsReading;

		NThread::CSpinLock m_FreeLock;
		DLinkD_List(CSCBuffer, m_Link) m_StreamsFree;

		class CMixerBuffer : public CSC_Mixer_Packet_fp32
		{
		public:
			CMixerBuffer(uint32 _Size, uint32 _nChannels, uint32 _nSamples)
			{
				m_pData = (fp32 *)M_ALLOC(_Size);
				m_nChannels = _nChannels;
				m_nSamples = _nSamples;
			}
			~CMixerBuffer()
			{
				if (m_pData)
					MRTC_GetMemoryManager()->Free(m_pData);
			}
			
		};

		DLinkAllocatorDS_List(CSC_Mixer_WorkerContext::CSC_Mixer_Packet, m_Link, CSC_Mixer_WorkerContext::CCustomAllocator) *m_pMixBuffersUnused;

		TThinArray<uint8> m_StreamBuffer;
		mint m_BufferSize;
		uint32 m_SubBufferSize:24;
		uint32 m_nStreams:8;
		uint32 m_iCurrentSubmitStream:16;
		uint32 m_MixerStreamPos:16;
		uint32 m_CurrentPacketPosRead:16;
		uint64 m_SamplePos;
		fint m_BufferPos;
		fint m_DataStartPos;
		fint m_DataSize;
		bint m_bPendingDataAvailable;
		bint m_bCreated;

		CStreamsData_PS3 *m_pStreamsData;

		class CAtracHandle
		{
		public:
			CAtracHandle()
			{
				m_bDecodeDone = 2;
				m_nDecodedSamples = 0;
				m_RemainingFrames = 0;
				m_LastDecodedPos = 0;
			}
			CellAtracHandle m_Handle;
			TThinArrayAlign<fp32, 128> m_OutputBuffer;
			uint32 m_bDecodeDone;
			uint32 m_nDecodedSamples;
			uint32 m_LastDecodedPos;
			int32 m_RemainingFrames;
		};

		TThinArray<CAtracHandle> m_AtracHandles;
		TThinArray<uint8> m_DecoderData;


		bint CanDelete();

		CSCStream()
		{
			m_pStreamsData = NULL;
			m_bPendingDataAvailable = false;
			m_CurrentPacketPosRead = 0;
			m_bCreated = false;
			m_pMixBuffersUnused = NULL;
		}
		~CSCStream();

		void Create();
		void Destroy();

		void ReturnBuffer(CSCBuffer *pBuf)
		{
			{
				DLock(m_FreeLock);
				m_StreamsFree.Insert(pBuf);
			}
		}

		CSCBuffer *GetFreeBuffer()
		{
			CSCBuffer *pBuf;
			{
				DLock(m_FreeLock);
				pBuf = m_StreamsFree.Pop();	
			}
			{
				if  (pBuf)
				{
					DLock(m_FreeLock);
					return pBuf;
				}
			}
			return NULL;
		}

		CSCBuffer *GetToSubmit()
		{
			{
				DLock(m_Lock);
				CSCBuffer *pBuf = m_StreamsToSubmit.Pop();
				if  (pBuf)
					return pBuf;
			}
			return NULL;
		}

		void QueueToSubmit(CSCBuffer *_pBuf)
		{
			{
				DLock(m_Lock);
				m_StreamsToSubmit.Insert(_pBuf);
			}
		}

	};

	uint16 m_WaveID;
	CSCC_CodecFormat m_Format;

	CSCStream m_Stream;

	CStaticVoice_PS3 *m_pStaticVoice;
	class CSoundContext_PS3 *m_pSoundContext;

	DLinkD_Link(CSCPS3_Buffer, m_SubmitLink);
	DLinkD_Link(CSCPS3_Buffer, m_ReadLink);
	DLinkD_Link(CSCPS3_Buffer, m_DeleteLink);

	volatile bint m_bDelayed;
	bint m_bLooping;

	bint CreateUpdate();
	bint ReadDataUpdate();
	bint SubmitPackets();

	void Clear();
	void Destroy();
	void Init(class CSoundContext_PS3 *_pSoundContext);
	~CSCPS3_Buffer();
	void SetData(void* _pData, int _nSamples, int _SamplePos = 0);
	void Create(CStaticVoice_PS3 *_pStaticVoice) ;

	bool CanDelete();
};

typedef DLinkD_Iter(CSCPS3_Buffer, m_DeleteLink) CSCPS3_BufferDeleteIter;

#endif

class CMixerOutputEffect;
// -------------------------------------------------------------------
class CSoundContext_PS3 : public CSoundContext_Mixer, public CSubSystem
{
	friend class CStaticVoice_PS3;
	friend class CSCPS3_Buffer;
#ifdef PLATFORM_PS3
	friend class CSCPS3_Buffer::CSCStream;
	friend class CMixerOutputEffect;
#endif

	typedef CSoundContext_Mixer CSuper;

	MRTC_DECLARE;

	const static int mcs_MaxDebugSounds = 384;
	char ** m_DebugStrTemp;

#ifdef PLATFORM_PS3
	TThinArray<CSCPS3_Buffer> m_lVoices;
	DLinkD_List(CSCPS3_Buffer, m_DeleteLink) m_VoicesForDelete;
#endif

	void *m_pStaticHeap;
	CSCPS3_Heap m_StaticVoiceHeap;

	MRTC_CriticalSection m_VoicesLock;
	MRTC_CriticalSection m_DSLock;

	int m_bCreated;
	uint32 m_Recursion;

	uint32 m_PortNumber;
#ifdef PLATFORM_PS3
	CellAudioPortConfig	m_PortConfig;

	sys_event_queue_t	m_AudioSysEventQueue;
	uint64_t			m_AudioSysEventQueueKey;
#endif

	void *m_pMainThreadID;

	TThinArray<CStaticVoice_PS3 *> m_lpStaicVoices;

#ifdef PLATFORM_PS3
	class CStartSoundThread : public MRTC_Thread
	{
	public:
		CSoundContext_PS3 *m_pThis;

		NThread::CEventAutoResetReportable m_Event;
		const char* Thread_GetName() const
		{
			return "Sound Start";
		}
		int Thread_Main()
		{
			MRTC_SystemInfo::Thread_SetName("Sound Start");
			MRTC_Thread::m_QuitEvent.ReportTo(&m_Event);
			while (!Thread_IsTerminating())
			{
				m_pThis->Thread_StartSounds();
				m_Event.WaitTimeout(1.0);
			}
			return 0;
		}
	};

	CStartSoundThread m_Thread_SoundStart;

	class CSubmitThread : public MRTC_Thread
	{
	public:
		CSoundContext_PS3 *m_pThis;

		NThread::CEventAutoResetReportable m_Event;
		const char* Thread_GetName() const
		{
			return "Sound Submit";
		}

		int Thread_Main()
		{
			MRTC_SystemInfo::Thread_SetName("Sound Submit");
			MRTC_Thread::m_QuitEvent.ReportTo(&m_Event);
			while (!Thread_IsTerminating())
			{
				m_pThis->Thread_Submit();
				m_Event.WaitTimeout(0.005f); // 5 ms to enable delayed sounds
			}
			return 0;
		}
	};

	CSubmitThread m_Thread_Submit;
	
	class CReadThread : public MRTC_Thread
	{
	public:
		CSoundContext_PS3 *m_pThis;

		NThread::CEventAutoReset m_Event;

		const char* Thread_GetName() const
		{
			return "Sound Read";
		}
		int Thread_Main()
		{
			MRTC_SystemInfo::Thread_SetName("Sound Read");
//			MRTC_Thread::m_QuitEvent.ReportTo(&m_Event);
			while (!Thread_IsTerminating())
			{
				m_pThis->Thread_Read();
				m_Event.WaitTimeout(0.5f);
			}
			return 0;
		}
	};

	CReadThread m_Thread_Read;

	NThread::CMutual m_Streams_Submit_Lock;
	DLinkD_List(CSCPS3_Buffer, m_SubmitLink) m_Streams_Submit;

	NThread::CMutual m_Streams_Start_Lock;
	DLinkD_List(CSCPS3_Buffer, m_ReadLink) m_Streams_Start;

	NThread::CMutual m_Streams_Read_Lock;
	DLinkD_List(CSCPS3_Buffer, m_ReadLink) m_Streams_Read;


	class CAudioRingBufferThread : public MRTC_Thread
	{
	public:
		CSoundContext_PS3 *m_pThis;

		NThread::CEventAutoResetReportable m_Event;

		const char* Thread_GetName() const
		{
			return "Audio Ring Buffer";
		}

		int Thread_Main()
		{
			MRTC_SystemInfo::Thread_SetName("Audio Ring Buffer");
			
			MRTC_Thread::m_QuitEvent.ReportTo(&m_Event);

			m_pThis->Thread_AudioRingBuffer();

			return 0;
		}
	};

	CAudioRingBufferThread m_Thread_AudioRingBuffer;

#endif
	

private:

	void Thread_Submit();
	void Thread_Read();
	void Thread_StartSounds();
	void Thread_AudioRingBuffer();
#ifdef PLATFORM_PS3
	void CreateDevice();
	void DestroyDevice();

	CStaticVoice_PS3 *GetStaticVoice(int _WaveID);

#endif

	void CreateAll();
	void DestroyAll();

public:

	CSoundContext_PS3();
	~CSoundContext_PS3();
	virtual void Create(CStr _Params);
	virtual void KillThreads();

	virtual void Platform_GetInfo(CPlatformInfo &_Info);
	virtual void Platform_Init(uint32 _MaxMixerVoices);
	virtual void Platform_StartStreamingToMixer(uint32 _MixerVoice, CVoice *_pVoice, uint32 _WaveID, fp32 _SampleRate);
	virtual void Platform_StopStreamingToMixer(uint32 _MixerVoice);


	// Precache
	virtual void Wave_Precache(int _WaveID);
	virtual void Wave_PrecacheFlush();
	virtual void Wave_PrecacheBegin( int _Count );
	virtual void Wave_PrecacheEnd(uint16 *_pPreCacheOrder, int _nIds);
	virtual	int Wave_GetMemusage(int _WaveID);

	virtual const char **GetDebugStrings();

public:
#ifdef PLATFORM_PS3
	bool UpdateVoiceDelete();
#endif
	virtual void Refresh();

	// Overrides from CSubSystem
	virtual aint OnMessage(const CSS_Msg& _Msg);
	virtual void OnRefresh(int _Context);
	virtual void OnBusy(int _Context);
};

extern CSoundContext_PS3 *g_XSoundContext;

typedef TPtr<CSoundContext_PS3> spCSoundContext_PS3;

#endif // DInc_MSound_PS3_h
