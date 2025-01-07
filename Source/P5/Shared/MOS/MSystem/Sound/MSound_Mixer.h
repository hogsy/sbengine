
#ifndef DInc_MSound_Mixer_h
#define DInc_MSound_Mixer_h

#include "MSound_Core.h"

class CSC_Mixer_WorkerContext;
#include "MSound_Mixer_Worker.h"


class CSC_Mixer_DSP_Voice;
class CSC_Mixer : public MRTC_Thread
{
	friend class CSC_Mixer_DSP_Voice;
	friend class CSC_Mixer_DSP_Router;
	friend class CSC_Mixer_WorkerContext;
public:


	static CSC_Mixer *ms_pThis;
	CSC_Mixer();
	~CSC_Mixer();

	void Create(int32 _nMaxVoices, int32 _nChannels, int32 _FrameLength, fp32 _SampleRate, int32 _nProcessingThreads);

	// Safe to call from interrupts
	CSC_Mixer_OuputFrame *StartNewFrame(); // Returns old calculated frame if available. This data will be valid until the next time StartNewFrame is called


	mint GetFreeMemory()
	{
		DLock(m_WorkContextBlockManagerLock);
		return m_WorkContextBlockManager.GetFreeMemory();
	}

	////////////////////////////////////////////////
	// Voice interface
	void Voice_Setup(uint32 _Voice, uint32 _nChannels, uint64 _NumSamples);
	void Voice_SetDSPChain(uint32 _Voice, CSC_Mixer_DSPChainInstance *_pDSPChainInstance);
	void Voice_WantStop(uint32 _Voice);
	bint Voice_IsStopped(uint32 _Voice);
	void Voice_Stop(uint32 _Voice); // Blocks if the voice is still playing!
	void Voice_FlushPackets(uint32 _Voice); // Blocks if the voice is still playing!

	uint32 Voice_GetFreeVoice(fp32 _OriginalPitch);
	uint32 Voice_GetNumFree();
	
	// Used to sync voice starts
	void Voice_StartPlayLock();
	void Voice_StartPlayUnlock();

	void Voice_SetLoop(uint32 _Voice, bint _bEnableLoop, uint64 _LoopPoint, uint32 _LoopSkipStart, uint32 _LoopSkipEnd);
	void Voice_Play(uint32 _Voice, uint32 _nSamplesDelay, uint32 _bPaused);
	// Used to sync to an already started voice
	void Voice_Play(uint32 _Voice, uint32 _VoiceSyncTo, int64 _SyncOffset, uint32 _bPaused);
	bint Voice_IsPlaying(uint32 _Voice);

	void Voice_Pause(uint32 _Voice);
	void Voice_Unpause(uint32 _Voice);
	bint Voice_IsPaused(uint32 _Voice);

	void Voice_Sync(uint32 _Voice, uint32 _VoiceSyncTo, int64 _SyncOffset);
	void Voice_SetVolume(uint32 _Voice, fp32 _Volume, bint _bInterpolate);
	void Voice_SetPitch(uint32 _Voice, fp32 _Pitch); // Sample rate modifier Samplrate = OriginalSamplaRate * _Pitch

	uint64 Voice_GetFramePosition(uint32 _Voice);
	CMTime Voice_GetPosition(uint32 _Voice);
	int64 Voice_GetSamplePos(uint32 _Voice);
	void Voice_SetSamplePos(uint32 _Voice, int64 _SamplePos);

	template <typename t_CPacket>
	static t_CPacket *Voice_AllocMixerPacket()
	{
		return CSC_Mixer_WorkerContext::New<t_CPacket>();
	}

	template <typename t_CPacket, typename t_0>
	static t_CPacket *Voice_AllocMixerPacket(t_0 _0)
	{
		return CSC_Mixer_WorkerContext::New<t_CPacket>(_0);
	}
	template <typename t_CPacket, typename t_0, typename t_1>
	static t_CPacket *Voice_AllocMixerPacket(t_0 _0, t_1 _1)
	{
		return CSC_Mixer_WorkerContext::New<t_CPacket>(_0, _1);
	}
	template <typename t_CPacket, typename t_0, typename t_1, typename t_2>
	static t_CPacket *Voice_AllocMixerPacket(t_0 _0, t_1 _1, t_2 _2)
	{
		return CSC_Mixer_WorkerContext::New<t_CPacket>(_0, _1, _2);
	}
	template <typename t_CPacket, typename t_0, typename t_1, typename t_2, typename t_3>
	static t_CPacket *Voice_AllocMixerPacket(t_0 _0, t_1 _1, t_2 _2, t_3 _3)
	{
		return CSC_Mixer_WorkerContext::New<t_CPacket>(_0, _1, _2, _3);
	}
	template <typename t_CPacket, typename t_0, typename t_1, typename t_2, typename t_3, typename t_4>
	static t_CPacket *Voice_AllocMixerPacket(t_0 _0, t_1 _1, t_2 _2, t_3 _3, t_4 _4)
	{
		return CSC_Mixer_WorkerContext::New<t_CPacket>(_0, _1, _2, _3, _4);
	}

	template <typename t_CPacket>
	static void Voice_FreeMixerPacket(t_CPacket *_pPacket)
	{
		CSC_Mixer_WorkerContext::Delete<t_CPacket>(_pPacket);
	}


	void Voice_SubmitPacket(uint32 _Voice, CSC_Mixer_WorkerContext::CSC_Mixer_Packet *_pPacket);
	void Voice_RemoveUnplayedPackets(uint32 _Voice);
	CSC_Mixer_WorkerContext::CSC_Mixer_Packet *Voice_PopFinishedPacket(uint32 _Voice);

	////////////////////////////////////////////////
	// DSP interface

	// DSP index 0 is the master buffer
	CSC_Mixer_DSPChain *DSPChain_Create();
	void DSPChain_Destroy(CSC_Mixer_DSPChain *_pChain); // Blocks
	CSC_Mixer_DSPInstance *DSP_Create(CSC_Mixer_DSPChain *_pChain, uint32 _DSPID, uint32 &_iDSP);
	void DSP_AddRouteTo(CSC_Mixer_DSPInstance *_pDSP, CSC_Mixer_DSPInstance *_pRouteToDSP);

	CSC_Mixer_DSPChainInstance *DSPChain_CreateIntstance(CSC_Mixer_DSPChain *_pChain);
	void DSPChainInstance_Destroy(CSC_Mixer_DSPChainInstance * _pChainInstance); // You have to guarantee that no voices are playing with this instance before destroying it, it will breakpoint otherwise
	void DSPChainInstance_SetParams(CSC_Mixer_DSPChainInstance *_pChainInstance, uint32 _iDSP, uint32 _iParamStart, const void *_pValue, uint32 _nBytes);
	void *DSPChainInstance_GetParamPtr(CSC_Mixer_DSPChainInstance *_pChainInstance, uint32 _iDSP);
	void *DSPChainInstance_GetLastParamPtr(CSC_Mixer_DSPChainInstance *_pChainInstance, uint32 _iDSP);
	void DSPChainInstance_AddProcess(CSC_Mixer_DSPChainInstance * _pChainInstance);
	void DSPChainInstance_SetActive(CSC_Mixer_DSPChainInstance * _pChainInstance);
	void DSPChainInstance_RunCreate(CSC_Mixer_DSPChainInstance * _pChainInstance);

	void DSPChainInstance_SetMasterInstance(CSC_Mixer_DSPChainInstance * _pChainInstance);

	TThinArrayAlign<uint8, 128> m_WorkContextBlockManagerMemory;
	TThinArrayAlign<uint8, 128> m_WorkContextBlockManagerMemoryVPU;

	void Global_SetSampleRate(fp32 _SampleRate);
	void Global_GetTimeStats(CMTime &_SampleTime, CMTime &_CPUTime);

	fp32 Stat_GetFrameTimePercent();
protected:

	TCSimpleBlockManager<> m_WorkContextBlockManager;

	enum
	{
		EMaxExtraFramesToConsume = 8, // This is the number of frames that
	};

	// Align locks on cacheline boundaries
	NThread::CEventAutoResetReportable M_ALIGN(M_ATOMICALIGNMENT) m_Event;
	NThread::CMutual M_ALIGN(M_ATOMICALIGNMENT) m_VoiceQueueLock;
	NThread::CMutual M_ALIGN(M_ATOMICALIGNMENT) m_InterfaceLock;
	NThread::CMutual M_ALIGN(M_ATOMICALIGNMENT) m_MiscLock;
	NThread::CSpinLock M_ALIGN(M_ATOMICALIGNMENT) m_MixBinContainerLock;
	NThread::CMutual M_ALIGN(M_ATOMICALIGNMENT) m_CalcLock;
	NThread::CSpinLock M_ALIGN(M_ATOMICALIGNMENT) m_TimeLock;
	NThread::CAtomicInt M_ALIGN(M_ATOMICALIGNMENT) m_StartNewFrame;
	NThread::CAtomicSmint M_ALIGN(M_ATOMICALIGNMENT) m_FinishedOutputFrame;
	NThread::CMutual M_ALIGN(M_ATOMICALIGNMENT) m_WorkContextBlockManagerLock;

	CSC_Mixer_WorkerContext *GetWorkContext()
	{
		return CSC_Mixer_WorkerContext::ms_pThis;
	}

	CMTime m_LastFrameStart;
	CMTime m_FirstFrameStart;
	CMTime m_SampleTime;
	fp32 m_LastFrameTime;

	void CalculateFrame();

	uint32 GetVoiceIndex(CSC_Mixer_WorkerContext::CVoice *_pVoice);

	const char* Thread_GetName() const;
	int Thread_Main();

};


#endif // DInc_MSound_Mixer_h
