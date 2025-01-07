
#include "PCH.h"

#include "MSound_Mixer.h"

#include "MMath_Vec128.h"
#include "MRTC_VPUManager.h"
#include "MSound_Mixer_DSPEffects.h"

/*M_FORCEINLINE vec128 M_VLdMemU(const void* _pMem)
{
#if defined(PLATFORM_XENON) || defined(PLATFORM_PS3)
      vec128 l=__lvlx((void*) _pMem, 0);

      vec128 r=__lvrx((void*) _pMem, 16);

      vec128 res= M_VOr(r,l);
	  return res;
#else
	return *((vec128 *)_pMem);
#endif
}*/
/*
#ifdef PLATFORM_PS3
#define __stvewx(a1, a2, a3) __builtin_altivec_stvewx ((vector signed int) a1, a3, (void *) a2);
#endif

M_FORCEINLINE void M_VStAny32(vec128 _a, void *_pDest, mint _Offset)
{
#if defined(PLATFORM_XENON) || defined(PLATFORM_PS3)
	__stvewx(_a, _pDest, _Offset);
#else
#endif
}
*/
/*
M_FORCEINLINE vec128 M_VPerm(vec128 _a, vec128 _b, vec128 _Permute)
{
#if defined(PLATFORM_XENON) || defined(PLATFORM_PS3)
	return __vperm(_a, _b, _Permute);
#else
	return M_VZero();
#endif
}

*/
CSC_Mixer::CSC_Mixer()
{
	ms_pThis = this;
	m_FinishedOutputFrame.Construct(0);
	m_StartNewFrame.Construct(0);
//	Thread_Create(NULL, 16384, MRTC_THREAD_PRIO_TIMECRITICAL);
	Thread_Create(NULL, 65536, MRTC_THREAD_PRIO_TIMECRITICAL); // More stack when opts are disabled

	m_WorkContextBlockManagerMemory.SetLen(64*1024); // Only 64 KB Allowed
	m_WorkContextBlockManagerMemoryVPU.SetLen(64*1024);
	m_WorkContextBlockManager.Create("Sound Mixer", 8, 64*1024-8, 8);
	CSC_Mixer_WorkerContext::ms_DataOffset = (mint)m_WorkContextBlockManagerMemory.GetBasePtr();
	SCMixer_VPUAccess_SetOffest((mint)m_WorkContextBlockManagerMemoryVPU.GetBasePtr());
	CSC_Mixer_WorkerContext::ms_pBlockManager = &m_WorkContextBlockManager;
	CSC_Mixer_WorkerContext::ms_pBlockManagerLock = &m_WorkContextBlockManagerLock;

	CSC_Mixer_WorkerContext::ms_pThis = CSC_Mixer_WorkerContext::New<CSC_Mixer_WorkerContext>();

	TCDynamicPtr<CSC_Mixer_WorkerContext::CCustomPtrHolder, CSC_Mixer_WorkerContext> This;
	This = CSC_Mixer_WorkerContext::ms_pThis;
	SCMixer_VPUAccess_SetThis(&This);

	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();

	pWorkContext->m_VoiceStartLocked = 0;
	pWorkContext->m_CurrentFrame = 0;
	pWorkContext->m_StartedFrame = 0;
	pWorkContext->m_SampleRate = 0;
	pWorkContext->m_FrameLength = 0;
	pWorkContext->m_nChannelVectors = 0;
	pWorkContext->m_nProcessingThreads = 0;
	pWorkContext->m_nCreatedContainers = 0;

}

CSC_Mixer::~CSC_Mixer()
{
	CSC_Mixer_WorkerContext::Delete<CSC_Mixer_WorkerContext>(CSC_Mixer_WorkerContext::ms_pThis);
}

CSC_Mixer *CSC_Mixer::ms_pThis;


const char* CSC_Mixer::Thread_GetName() const
{
	return "MSound_Mixer";
}

int CSC_Mixer::Thread_Main()
{
	MRTC_SystemInfo::Thread_SetProcessor(3);
	m_QuitEvent.ReportTo(&m_Event);

	while(!Thread_IsTerminating())
	{
		int iNewFrame = m_StartNewFrame.Exchange(0);

		if (iNewFrame)
		{
			CalculateFrame();
		}

		m_Event.WaitTimeout(0.1);
	}

	return 0;
	
}

uint32 CSC_Mixer::GetVoiceIndex(CSC_Mixer_WorkerContext::CVoice *_pVoice)
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	return _pVoice - pWorkContext->m_Voices.GetBasePtr();
}

void CSC_Mixer::Create(int32 _nMaxVoices, int32 _nChannels, int32 _FrameLength, fp32 _SampleRate, int32 _nProcessingThreads)
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	pWorkContext->m_Voices.SetLen(_nMaxVoices);
	pWorkContext->m_nFreeVoices = _nMaxVoices;

	for (int i = 0; i < _nMaxVoices; ++i)
	{
		pWorkContext->m_VoicesFree.Insert(pWorkContext->m_Voices[i]);
		pWorkContext->m_Voices[i].m_bInvalid = 2;
	}
	pWorkContext->m_CurrentFrame = 0;
	pWorkContext->m_SampleRate = _SampleRate;
	pWorkContext->m_DesiredSampleRate = _SampleRate;
	pWorkContext->m_FrameLength = _FrameLength;
	pWorkContext->m_nChannelVectors = ((_nChannels + 3) >> 2);
	pWorkContext->m_nProcessingThreads = _nProcessingThreads;

	pWorkContext->m_FrameTime = _FrameLength / _SampleRate;
	pWorkContext->m_SampleTime = 1.0f / _SampleRate;

	int NeededOutputData = pWorkContext->m_nChannelVectors * _FrameLength * 2;
	pWorkContext->m_OutputData.SetLen(NeededOutputData);
	pWorkContext->m_OutputFrames[0].m_nChannels = _nChannels;
	pWorkContext->m_OutputFrames[0].m_nSamples = _FrameLength;
	pWorkContext->m_OutputFrames[0].m_pData = pWorkContext->m_OutputData.GetBasePtr();
	pWorkContext->m_OutputFrames[1].m_nChannels = _nChannels;
	pWorkContext->m_OutputFrames[1].m_nSamples = _FrameLength;
	pWorkContext->m_OutputFrames[1].m_pData = pWorkContext->m_OutputData.GetBasePtr() + pWorkContext->m_nChannelVectors * _FrameLength;


};

CSC_Mixer_DSPChain *CSC_Mixer::DSPChain_Create()
{
	DLock(m_MiscLock);
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	CSC_Mixer_WorkerContext::CDSPChainInternal *pNew = pWorkContext->New<CSC_Mixer_WorkerContext::CDSPChainInternal>();
	return pNew;
}

void CSC_Mixer::DSPChain_Destroy(CSC_Mixer_DSPChain *_pChain)
{
	DLock(m_MiscLock);
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	pWorkContext->Delete((CSC_Mixer_WorkerContext::CDSPChainInternal *)_pChain);
}


CSC_Mixer_DSPInstance *CSC_Mixer::DSP_Create(CSC_Mixer_DSPChain *_pChain, uint32 _DSPID, uint32 &_iDSP)
{
	DLock(m_MiscLock);
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	CSC_Mixer_WorkerContext::CDSPChainInternal *pChain = (CSC_Mixer_WorkerContext::CDSPChainInternal *)_pChain;
	CSC_Mixer_WorkerContext::CDSPInstanceInternal *pNew = pWorkContext->New<CSC_Mixer_WorkerContext::CDSPInstanceInternal>();
	pNew->m_DSPID = _DSPID;
	_iDSP = pNew->m_iChainDSP = pChain->m_nDSPInstances++;
	pNew->m_pDSPChain = pChain;
	pChain->m_DSPInstances.Insert(pNew);
	if (CSC_Mixer_WorkerContext::ms_DSP_Func_CopyData[_DSPID])
		pChain->m_DSPInstancesSpecialCopy.Insert(pNew);

	return pNew;
}

void CSC_Mixer::DSP_AddRouteTo(CSC_Mixer_DSPInstance *_pDSP, CSC_Mixer_DSPInstance *_pRouteToDSP)
{
	DLock(m_MiscLock);
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	CSC_Mixer_WorkerContext::CDSPInstanceLink *pLink = pWorkContext->New<CSC_Mixer_WorkerContext::CDSPInstanceLink>();
	pLink->m_pDSPInstance = (CSC_Mixer_WorkerContext::CDSPInstanceInternal *)_pRouteToDSP;
	((CSC_Mixer_WorkerContext::CDSPInstanceInternal *)_pDSP)->m_RouteTo.Insert(pLink);
}

CSC_Mixer_DSPChainInstance *CSC_Mixer::DSPChain_CreateIntstance(CSC_Mixer_DSPChain *_pChain)
{
	DLock(m_MiscLock);
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	CSC_Mixer_WorkerContext::CDSPChainInternal *pChain = (CSC_Mixer_WorkerContext::CDSPChainInternal *)_pChain;
	CSC_Mixer_WorkerContext::CDSPChainInstanceInternal *pNew = pWorkContext->New<CSC_Mixer_WorkerContext::CDSPChainInstanceInternal>();
	if (pNew)
	{
		if (!pNew->SetDSPChain(pChain))
		{
			pWorkContext->Delete(pNew);
			return NULL;
		}
		pWorkContext->m_DSPChainInstancesNonProcess.Insert(pNew);
		return pNew;
	}
	return NULL;
}

void CSC_Mixer::DSPChainInstance_AddProcess(CSC_Mixer_DSPChainInstance * _pChainInstance)
{
	DLock(m_MiscLock);
	CSC_Mixer_WorkerContext::CDSPChainInstanceInternal *pInstance = (CSC_Mixer_WorkerContext::CDSPChainInstanceInternal *)_pChainInstance;
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	pWorkContext->m_DSPChainInstancesProcess.Insert(pInstance);
	pInstance->m_bActive = true;

}


void CSC_Mixer::DSPChainInstance_SetActive(CSC_Mixer_DSPChainInstance * _pChainInstance)
{
	DLock(m_MiscLock);
	CSC_Mixer_WorkerContext::CDSPChainInstanceInternal *pInstance = (CSC_Mixer_WorkerContext::CDSPChainInstanceInternal *)_pChainInstance;
	pInstance->m_bActive = true;
}
void CSC_Mixer::DSPChainInstance_RunCreate(CSC_Mixer_DSPChainInstance * _pChainInstance)
{
	DLock(m_MiscLock);
	CSC_Mixer_WorkerContext::CDSPChainInstanceInternal *pChainInstance = (CSC_Mixer_WorkerContext::CDSPChainInstanceInternal *)_pChainInstance;
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();

	CSC_Mixer_WorkerContext::CDSPChainInternal *pChain = pChainInstance->m_pChain;
	CSC_Mixer_WorkerContext::CDSPInstanceInternalIterator Iter = pChain->m_DSPInstances;
	while (Iter)
	{
		uint32 DSPID = Iter->m_DSPID;
		uint32 iDSP = Iter->m_iChainDSP;
		if (CSC_Mixer_WorkerContext::ms_DSP_Func_Create[DSPID])
			CSC_Mixer_WorkerContext::ms_DSP_Func_Create[DSPID](pChainInstance->GetInternalPtr(iDSP), pChainInstance->GetParamPtr(iDSP), pChainInstance->GetLastParamPtr(iDSP));
		++Iter;
	}
}


void CSC_Mixer::DSPChainInstance_SetMasterInstance(CSC_Mixer_DSPChainInstance * _pChainInstance)
{
	DLock(m_MiscLock);
	CSC_Mixer_WorkerContext::CDSPChainInstanceInternal *pInstance = (CSC_Mixer_WorkerContext::CDSPChainInstanceInternal *)_pChainInstance;
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	pWorkContext->m_pMasterInstance = pInstance;
}


void CSC_Mixer::DSPChainInstance_Destroy(CSC_Mixer_DSPChainInstance * _pChainInstance)
{
	CSC_Mixer_WorkerContext::CDSPChainInstanceInternal *pChainInstance = (CSC_Mixer_WorkerContext::CDSPChainInstanceInternal *)_pChainInstance;
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();

	CSC_Mixer_WorkerContext::CDSPChainInternal *pChain = pChainInstance->m_pChain;
	CSC_Mixer_WorkerContext::CDSPInstanceInternalIterator Iter = pChain->m_DSPInstances;
	while (Iter)
	{
		uint32 DSPID = Iter->m_DSPID;
		uint32 iDSP = Iter->m_iChainDSP;
		if (CSC_Mixer_WorkerContext::ms_DSP_Func_DestroyData[DSPID])
			CSC_Mixer_WorkerContext::ms_DSP_Func_DestroyData[DSPID](pChainInstance->GetInternalPtr(iDSP), pChainInstance->GetParamPtr(iDSP), pChainInstance->GetLastParamPtr(iDSP));
		++Iter;
	}

	pWorkContext->Delete(pChainInstance);
}

void CSC_Mixer::DSPChainInstance_SetParams(CSC_Mixer_DSPChainInstance *_pChainInstance, uint32 _iDSP, uint32 _iParamStart, const void *_pValue, uint32 _nBytes)
{
	CSC_Mixer_WorkerContext::CDSPChainInstanceInternal *pInstance = (CSC_Mixer_WorkerContext::CDSPChainInstanceInternal *)_pChainInstance;
	memcpy((uint8 *)pInstance->GetParamPtr(_iDSP) + _iParamStart, _pValue, _nBytes);
}

void *CSC_Mixer::DSPChainInstance_GetParamPtr(CSC_Mixer_DSPChainInstance *_pChainInstance, uint32 _iDSP)
{
	CSC_Mixer_WorkerContext::CDSPChainInstanceInternal *pInstance = (CSC_Mixer_WorkerContext::CDSPChainInstanceInternal *)_pChainInstance;
	return pInstance->GetParamPtr(_iDSP);
}

void *CSC_Mixer::DSPChainInstance_GetLastParamPtr(CSC_Mixer_DSPChainInstance *_pChainInstance, uint32 _iDSP)
{
	CSC_Mixer_WorkerContext::CDSPChainInstanceInternal *pInstance = (CSC_Mixer_WorkerContext::CDSPChainInstanceInternal *)_pChainInstance;
	return pInstance->GetLastParamPtr(_iDSP);
}


void CSC_Mixer::Voice_WantStop(uint32 _Voice)
{
	DLock(m_MiscLock);
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	CSC_Mixer_WorkerContext::CVoice &Voice = pWorkContext->m_Voices[_Voice];
	Voice.m_bInvalid = 3;
	Voice.m_bDoneStop = false;
	pWorkContext->m_VoicesWaitingToStop.Insert(Voice);
}

bint CSC_Mixer::Voice_IsStopped(uint32 _Voice)
{
	DLock(m_MiscLock);
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	CSC_Mixer_WorkerContext::CVoice &Voice = pWorkContext->m_Voices[_Voice];
	bint bIsInList = Voice.m_Link.IsInList();
	return !bIsInList;
}

void CSC_Mixer::Voice_FlushPackets(uint32 _Voice)
{
	DLock(m_MiscLock);
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();

	// Block if voice is in use
	CSC_Mixer_WorkerContext::CVoice &Voice = pWorkContext->m_Voices[_Voice];
	Voice.m_bInvalid = 3;
	while (Voice.m_Link.IsInList() || Voice.m_LinkDelete.IsInList())
	{
		DUnlock(m_MiscLock);
		MRTC_SystemInfo::OS_Sleep(0);
	}

	{
		DLock(m_VoiceQueueLock);
		if (Voice.m_pCurrentPacket)
			Voice.m_FinishedPackets.Insert(Voice.m_pCurrentPacket);
		Voice.m_pCurrentPacket = NULL;
		while (Voice.m_QueuedPackets.GetFirst())
			Voice.m_FinishedPackets.Insert(Voice.m_QueuedPackets.GetFirst());
	}
}
void CSC_Mixer::Voice_Stop(uint32 _Voice)
{
	DLock(m_MiscLock);
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();

	// Block if voice is in use
	CSC_Mixer_WorkerContext::CVoice &Voice = pWorkContext->m_Voices[_Voice];
	if (Voice.m_bInvalid != 3)
		M_BREAKPOINT;

	Voice.m_bInvalid = 2;

	if (Voice.m_Link.IsInList() || Voice.m_LinkDelete.IsInList())
		M_BREAKPOINT;

	{
		DLock(m_VoiceQueueLock);
		if (Voice.m_pCurrentPacket)
			M_BREAKPOINT;
		Voice.m_pCurrentPacket = NULL;
		if (Voice.m_QueuedPackets.GetFirst())
			M_BREAKPOINT;
	}

	// Destroy instance
	if (Voice.m_pChainInstance)
	{
		DSPChainInstance_Destroy(Voice.m_pChainInstance);
		Voice.m_pChainInstance = NULL;
	}
	else
	{
		M_TRACEALWAYS("No chain instance\n");
	}

	//M_TRACEALWAYS("Freeing voice %d\n", _Voice);
	pWorkContext->m_VoicesFree.Insert(Voice);
	++pWorkContext->m_nFreeVoices;
}

void CSC_Mixer::Global_SetSampleRate(fp32 _SampleRate)
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	pWorkContext->m_DesiredSampleRate = _SampleRate;
}

void CSC_Mixer::Global_GetTimeStats(CMTime &_SampleTime, CMTime &_CPUTime)
{
	{
		DLock(m_TimeLock);
		_SampleTime = m_SampleTime;
		_CPUTime = m_LastFrameStart - m_FirstFrameStart;
	}
}


void CSC_Mixer::CalculateFrame()
{
	DLock(m_CalcLock);
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();

	MRTC_ThreadPoolManager::EvacuateCPU(3);

	CMTime Timer;
	{
		TMeasure(Timer);


		{
			DLock(m_TimeLock);
			pWorkContext->m_StartedFrame = pWorkContext->m_CurrentFrame;
			m_LastFrameStart = CMTime::GetCPU();
			if (m_FirstFrameStart.IsReset())
				m_FirstFrameStart = m_LastFrameStart;
			else
			{
				m_SampleTime += CMTime::CreateFromTicks(pWorkContext->m_FrameLength, pWorkContext->m_SampleTime);
				
				CMTime Time = m_LastFrameStart - m_FirstFrameStart;
				if (Abs((Time-m_SampleTime).GetTime()) > 1.0f)
				{
					m_SampleTime = Time;
				}
			}
		}


		fp32 SampleRate = pWorkContext->m_DesiredSampleRate;
		pWorkContext->m_SampleRate = SampleRate;
		pWorkContext->m_FrameTime = pWorkContext->m_FrameLength / SampleRate;
		pWorkContext->m_SampleTime = 1.0f / SampleRate;
		//////////////////////////////////////////////////////////
		// Start by handling pending deletes and stops

		uint32 bStartVoices = true;
		{
			DLock(m_InterfaceLock);
			bStartVoices = !pWorkContext->m_VoiceStartLocked;
		}
		
		{
			DLock(m_MiscLock);

			CSC_Mixer_WorkerContext::CVoice *pVoice;

			if (bStartVoices)
			{
				pVoice = pWorkContext->m_VoicesWaitingToPlay.Pop();

				while (pVoice)
				{
					if (pVoice->m_SyncTo == 0xffffffff)
					{
						//pVoice->m_FinishedPackets
					}
					else
					{
					}
					//m_DSPChainInstances.Insert(pVoice->m_ChainInstance);
					pWorkContext->m_VoicesPlaying.Insert(pVoice);
					//M_TRACEALWAYS("Start Play %d\n", GetVoiceIndex(pVoice));
					pVoice = pWorkContext->m_VoicesWaitingToPlay.Pop();
				}
			}

			{
				DLinkAllocatorDS_Iter(CSC_Mixer_WorkerContext::CVoice, m_LinkDelete, CSC_Mixer_WorkerContext::CCustomAllocator) Iter = pWorkContext->m_VoicesWaitingToStop;

				while (Iter)
				{
					pVoice = Iter;
					++Iter;

					if (!pVoice->m_bDoneStop)
					{
						pVoice->m_bDoneStop = true;
						continue;
					}
					{
						{
	//						pVoice->m_ChainInstance.ClearDSP();
	//						pVoice->m_ChainInstance.m_nReferences = 0;
		//					M_TRACEALWAYS("Clearing voice %d\n", GetVoiceIndex(pVoice));


							{
								DLock(m_VoiceQueueLock);
								CSC_Mixer_WorkerContext::CSC_Mixer_Packet *pPacket = pVoice->m_QueuedPackets.Pop();
								while (pPacket)
								{
									pVoice->m_FinishedPackets.Insert(pPacket);

									pPacket = pVoice->m_QueuedPackets.Pop();
								}
							}
						}
	//					pVoice->m_ChainInstance.m_Link.Unlink();
					}

					pVoice->m_Link.Unlink();
					// 
					//m_VoicesFree.Insert(pVoice); 
					pVoice = pWorkContext->m_VoicesWaitingToStop.Pop();
					
				}
			}

			{
				CSC_Mixer_WorkerContext::CVoiceIter Iter = pWorkContext->m_VoicesPlaying;

				while (Iter)
				{
					CSC_Mixer_WorkerContext::CVoice *pVoice = Iter;
					++Iter;

					pVoice->m_bPausedProcessing = pVoice->m_bPaused;

					if (pVoice->m_SamplePos < 0)
					{
						fp32 Temp = pVoice->m_CurrentPitch * pWorkContext->m_FrameLength + pVoice->m_SamplesLeft;

						uint32 WholeSamples = TruncToInt(Temp);
						pVoice->m_SamplesLeft += Temp - WholeSamples;
						pVoice->m_SamplePos += WholeSamples;

						// Pause Voice
						if (pVoice->m_SamplePos < 0)
							pVoice->m_bPausedProcessing = true;
					}


					if (!pVoice->m_bPausedProcessing)
					{
						if (pVoice->m_SamplePos >= pVoice->m_nSamples)
						{
							// Loop?
							if (pVoice->m_LoopPoint < 0)
							{
								pVoice->m_bPausedProcessing = true;
							}
						}
					}
				}
			}
			{

				CSC_Mixer_WorkerContext::CChainIter Iter = pWorkContext->m_DSPChainInstancesProcess;
				for (int i = 0; i < 2; ++i)
				{
					while (Iter)
					{
						CSC_Mixer_WorkerContext::CDSPChainInstanceInternal &Instance = *Iter;
						if (Instance.m_bActive)
							Instance.m_bNewlyCreated = false;
						++Iter;
					}
					Iter = pWorkContext->m_DSPChainInstancesNonProcess;
				}
			}


			{
				DLock(m_VoiceQueueLock);
				vec128 *M_RESTRICT pDest = (vec128 *)m_WorkContextBlockManagerMemoryVPU.GetBasePtr();
				vec128 *M_RESTRICT pSrc = (vec128 *)m_WorkContextBlockManagerMemory.GetBasePtr();
				uint32 nLoop = (64*1024) / 128;
				M_PRECACHE128(0, pSrc);
				M_PRECACHE128(128, pSrc);
				M_PRECACHE128(256, pSrc);
				M_PRECACHE128(384, pSrc);

				for (int i = 0; i < nLoop; ++i)
				{
					M_PRECACHE128(512, pSrc);
					M_PREZERO128(0, pDest);
					pDest[0] = pSrc[0];
					pDest[1] = pSrc[1];
					pDest[2] = pSrc[2];
					pDest[3] = pSrc[3];
					pDest[4] = pSrc[4];
					pDest[5] = pSrc[5];
					pDest[6] = pSrc[6];
					pDest[7] = pSrc[7];
					pDest += 8;
					pSrc += 8;
				}

				//memcpy(m_WorkContextBlockManagerMemoryVPU.GetBasePtr(), m_WorkContextBlockManagerMemory.GetBasePtr(), m_WorkContextBlockManagerMemory.ListSize());
			}
		}

		CSC_Mixer_OuputFrame *pOutput = &(pWorkContext->m_OutputFrames[pWorkContext->m_CurrentFrame % 2]);

		M_EXPORTBARRIER;
		TCDynamicPtr<CSC_Mixer_WorkerContext::CCustomPtrHolder, CSC_Mixer_OuputFrame> pOutputPtr;
		pOutputPtr = pOutput;
		SCMixer_VPUAccess_CalculateFrame(&pOutputPtr, m_WorkContextBlockManagerMemoryVPU.GetBasePtr());
		M_IMPORTBARRIER;
		{
			DLock(m_MiscLock);
			{
				{
					DLock(m_VoiceQueueLock);
					CSC_Mixer_WorkerContext::CVoiceIter Iter = pWorkContext->m_VoicesPlaying;

					while (Iter)
					{
						CSC_Mixer_WorkerContext::CVoice *pVoice = Iter;
						++Iter;

						if (pVoice->m_bPausedProcessing)
							continue;

						CSC_Mixer_WorkerContext::CVoiceVPUChange ChangedParams;
						uint32 iVoice = GetVoiceIndex(pVoice);

						SCMixer_VPUAccess_GetVPUChangableForVoice(iVoice, &ChangedParams);

						pVoice->m_pCurrentPacket = ChangedParams.m_pCurrentPacket;
						pVoice->m_CurrentPacketPos = ChangedParams.m_CurrentPacketPos;
						pVoice->m_SamplePos = ChangedParams.m_SamplePos;
						pVoice->m_SamplesLeft = ChangedParams.m_SamplesLeft;
						pVoice->m_LoopSkipCurrent = ChangedParams.m_LoopSkipCurrent;
						pVoice->m_bLoopSkipDone = ChangedParams.m_bLoopSkipDone;

						if (pVoice->m_pCurrentPacket)
							pVoice->m_pCurrentPacket->m_Link.Unlink(); // Unlink

						
						TCDynamicPtr<CSC_Mixer_WorkerContext::CCustomPtrHolder, CSC_Mixer_WorkerContext::CSC_Mixer_Packet> Curr;
						SCMixer_VPUAccess_DequePendingPacket(iVoice, &Curr);
						CSC_Mixer_WorkerContext::CSC_Mixer_Packet *pCurrentPacket = Curr;

						while (pCurrentPacket)
						{
							pVoice->m_FinishedPackets.Insert(pCurrentPacket);
							SCMixer_VPUAccess_DequePendingPacket(iVoice, &Curr);
							pCurrentPacket = Curr;
						}

					}
				}

				CSC_Mixer_WorkerContext::CChainIter Iter = pWorkContext->m_DSPChainInstancesProcess;
				for (int i = 0; i < 2; ++i)
				{
					while (Iter)
					{
						CSC_Mixer_WorkerContext::CDSPChainInstanceInternal &Instance = *Iter;
						if (!Instance.m_bNewlyCreated)
						{
							uint32 CopyLen = (Instance.m_Data.Len() - Instance.m_CopyDataStart + 15) >> 4;
							if (CopyLen)
							{
								uint32 CopyStart = Instance.m_Data.GetArray().m_PtrData.m_Offset + Instance.m_CopyDataStart;
								uint8 *pSrcData = m_WorkContextBlockManagerMemoryVPU.GetBasePtr();
								uint8 *pDstData = m_WorkContextBlockManagerMemory.GetBasePtr();
								vec128 *M_RESTRICT pDest = (vec128 *)(pDstData + CopyStart);
								vec128 *M_RESTRICT pSrc = (vec128 *)(pSrcData + CopyStart);
								DataCopy(pDest, pSrc, CopyLen);

								CSC_Mixer_WorkerContext::CDSPInstanceInternalIteratorSpecailCopy Iter2 = Instance.m_pChain->m_DSPInstancesSpecialCopy;
								while (Iter2)
								{
									CSC_Mixer_WorkerContext::CDSPInstanceInternal &DSPInstance = *Iter2;
									uint32 iDSP = DSPInstance.m_iChainDSP;

									CSC_Mixer_WorkerContext::ms_DSP_Func_CopyData[DSPInstance.m_DSPID](Instance.GetParamPtr(iDSP), Instance.GetLastParamPtr(iDSP), Instance.GetInternalPtr(iDSP), pSrcData, pDstData);
									++Iter2;
								}
							}
						}

						++Iter;
					}
					Iter = pWorkContext->m_DSPChainInstancesNonProcess;
				}
			}
		}

		++pWorkContext->m_CurrentFrame;
		M_EXPORTBARRIER;
		m_FinishedOutputFrame.Exchange((smint)pOutput);
		m_LastFrameTime = (CMTime::GetCPU() - m_LastFrameStart).GetTime();
	}

	MRTC_ThreadPoolManager::RestoreCPU(3);

	static int bDoTrace = true;
	static int iTest = 0;
	fp64 Time = Timer.GetTimefp64() * 1000.0;
	if ((iTest % 128) == 0)
	{
		//M_TRACEALWAYS("Calculate Frame: %f micro\n",  Time * 1000.0);
	}
	++iTest;


}

fp32 CSC_Mixer::Stat_GetFrameTimePercent()
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	return m_LastFrameTime / pWorkContext->m_FrameTime;

}

void CSC_Mixer::Voice_SetDSPChain(uint32 _Voice, CSC_Mixer_DSPChainInstance *_pDSPChainInstance)
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	CSC_Mixer_WorkerContext::CVoice &Voice = pWorkContext->m_Voices[_Voice];

	M_IMPORTBARRIER;

	if (Voice.m_bInvalid == 3)
		return;

	if (Voice.m_bInvalid != 1)
		M_BREAKPOINT; // The voice must be stopped

	CSC_Mixer_WorkerContext::CDSPChainInstanceInternal *pChainInstance = (CSC_Mixer_WorkerContext::CDSPChainInstanceInternal *)_pDSPChainInstance;
	Voice.m_pChainInstance = pChainInstance;
}

void CSC_Mixer::Voice_Setup(uint32 _Voice, uint32 _nChannels, uint64 _NumSamples)
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	CSC_Mixer_WorkerContext::CVoice &Voice = pWorkContext->m_Voices[_Voice];

	M_IMPORTBARRIER;

	if (Voice.m_bInvalid == 3)
		return;

	if (Voice.m_bInvalid != 1)
		M_BREAKPOINT; // The voice must be stopped

	{
		DLock(m_MiscLock);

		Voice.m_nChannels = _nChannels;
		Voice.m_bInvalid = 0;
		Voice.m_SamplePos = 0;
		Voice.m_pCurrentPacket = NULL;
		Voice.m_nSamples = _NumSamples;
		Voice.m_SamplesLeft = 0;
		Voice.m_CurrentPacketPos = 0;

		CSC_Mixer_WorkerContext::CDSPChainInstanceInternal *pChainInstance = (CSC_Mixer_WorkerContext::CDSPChainInstanceInternal *)Voice.m_pChainInstance;

		CSC_Mixer_DSP_Voice::CParams *pParams = (CSC_Mixer_DSP_Voice::CParams *)pChainInstance->GetParamPtr(0);
		pParams->m_pSourceVoice = &Voice;

//		M_TRACEALWAYS("Setup %d\n", _Voice);
//		Voice.m_ChainInstance.CopyPendingData(0);
//		Voice.m_ChainInstance.CopyPendingData(1);
	}
}

void CSC_Mixer::Voice_Play(uint32 _Voice, uint32 _VoiceSyncTo, int64 _SyncOffset, uint32 _bPaused)
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	CSC_Mixer_WorkerContext::CVoice &Voice = pWorkContext->m_Voices[_Voice];
	if (!Voice_IsStopped(_Voice))
		M_BREAKPOINT; // You cannot play a voice that is still running
	{
		DLock(m_MiscLock);
		if (Voice.m_bInvalid == 3)
			return;

		if (Voice.m_bInvalid)
			M_BREAKPOINT; // You cannot play a voice that hasn't had Voice_Setup called on it

		if (Voice.m_Link.IsInList())
			M_BREAKPOINT; // You cannot start an already playing voice

		Voice.m_SyncTo = _VoiceSyncTo;
		Voice.m_SyncSamples = _SyncOffset;
		Voice.m_bPaused = _bPaused;
		Voice.m_pChainInstance->m_bActive = true;
		pWorkContext->m_VoicesWaitingToPlay.Insert(Voice);
//		M_TRACEALWAYS("Play %d\n", _Voice);
	}
}

void CSC_Mixer::Voice_Play(uint32 _Voice, uint32 _nFramesDelay, uint32 _bPaused)
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	CSC_Mixer_WorkerContext::CVoice &Voice = pWorkContext->m_Voices[_Voice];
	{
		DLock(m_MiscLock);
		if (Voice.m_bInvalid == 3)
			return;

		if (!_bPaused)
		{
			if (Voice.m_bInvalid)
				M_BREAKPOINT; // You cannot play a voice that hasn't had Voice_Setup called on it

			if (Voice.m_Link.IsInList())
				M_BREAKPOINT; // You cannot start an already playing voice
		}

		Voice.m_SyncTo = 0xffffffff;
		Voice.m_SamplePos = -int32(_nFramesDelay);
		Voice.m_SyncSamples = _nFramesDelay;
		Voice.m_bPaused = _bPaused;
		Voice.m_pChainInstance->m_bActive = true;
//		M_TRACEALWAYS("Play %d\t", _Voice);
		pWorkContext->m_VoicesWaitingToPlay.Insert(Voice);
	}
}

void CSC_Mixer::Voice_SetLoop(uint32 _Voice, bint _bEnableLoop, uint64 _LoopPoint, uint32 _LoopSkipStart, uint32 _LoopSkipEnd)
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	CSC_Mixer_WorkerContext::CVoice &Voice = pWorkContext->m_Voices[_Voice];
	{
		DLock(m_MiscLock);
		if (_bEnableLoop)
			Voice.m_LoopPoint = _LoopPoint;
		else
			Voice.m_LoopPoint = -1;
		Voice.m_LoopSkipStart = _LoopSkipStart;
		Voice.m_LoopSkipBoth = _LoopSkipStart + _LoopSkipEnd;
	}
}


bint CSC_Mixer::Voice_IsPlaying(uint32 _Voice)
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	// TODO: Handle wrapping logic
	CSC_Mixer_WorkerContext::CVoice &Voice = pWorkContext->m_Voices[_Voice];
	bint bPlaying = true;
	{
		DLock(m_MiscLock);
		if (Voice.m_bInvalid >= 2)
			bPlaying = false;
		if (!Voice.m_Link.IsInList())
			bPlaying = false;
		if (Voice.m_LoopPoint < 0 && Voice.m_SamplePos >= Voice.m_nSamples)
			bPlaying = false;
	}
	return bPlaying;
}

void CSC_Mixer::Voice_Pause(uint32 _Voice)
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	CSC_Mixer_WorkerContext::CVoice &Voice = pWorkContext->m_Voices[_Voice];
	bint bPlaying = true;
	{
		DLock(m_MiscLock);
		Voice.m_bPaused = true;
	}
}

void CSC_Mixer::Voice_Unpause(uint32 _Voice)
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	CSC_Mixer_WorkerContext::CVoice &Voice = pWorkContext->m_Voices[_Voice];
	bint bPlaying = true;
	{
		DLock(m_MiscLock);
		{
			if (Voice.m_bInvalid == 3)
				return;
			if (Voice.m_bInvalid)
				M_BREAKPOINT;
		}

		Voice.m_bPaused = false;
	}
}

bint CSC_Mixer::Voice_IsPaused(uint32 _Voice)
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	CSC_Mixer_WorkerContext::CVoice &Voice = pWorkContext->m_Voices[_Voice];
	bint bPlaying = true;
	{
		DLock(m_MiscLock);
		return Voice.m_bPaused;
	}
}

uint32 CSC_Mixer::Voice_GetNumFree()
{
	DLock(m_MiscLock);
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	return pWorkContext->m_nFreeVoices;
}

uint32 CSC_Mixer::Voice_GetFreeVoice(fp32 _OriginalPitch)
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	CSC_Mixer_WorkerContext::CVoice *pRet;
	{
		DLock(m_MiscLock);
		pRet = pWorkContext->m_VoicesFree.Pop();
		if (pRet)
			--pWorkContext->m_nFreeVoices;
	}

	if (pRet)
	{
//		pRet->m_ChainInstance.SetDSPChain(&pWorkContext->m_VoiceChain);
//		pRet->m_ChainInstance.m_nReferences = 1;
		if (pRet->m_bInvalid != 2)
			M_BREAKPOINT;
		pRet->m_bInvalid = 1;

		pRet->m_nSamples = 1;
		pRet->m_LoopPoint = -1;
		pRet->m_SamplePos = 0;
		pRet->m_OriginalPitch = _OriginalPitch / pWorkContext->m_SampleRate;
		pRet->m_CurrentPitch = pRet->m_OriginalPitch;
		pRet->m_CurrentPitchInv = 1.0f / pRet->m_CurrentPitch;

		pRet->m_LoopSkipCurrent	= 0;
		pRet->m_bLoopSkipDone = false;
		M_EXPORTBARRIER;
		int iVoice = GetVoiceIndex(pRet);;
//		M_TRACEALWAYS("Allocating voice %d\n", iVoice);
		return iVoice;
	}
	else
		return 0xffffffff;
}

void CSC_Mixer::Voice_SetVolume(uint32 _Voice, fp32 _Volume, bint _bInterpolate)
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	DLock(m_MiscLock);
	CSC_Mixer_WorkerContext::CVoice &Voice = pWorkContext->m_Voices[_Voice];
	if (Voice.m_bInvalid == 3)
		return;

	CSC_Mixer_DSP_Voice::CParams *pParams = (CSC_Mixer_DSP_Voice::CParams *)Voice.m_pChainInstance->GetParamPtr(0);
	pParams->m_SingleVolume = _Volume;
	if (!_bInterpolate)
	{
		CSC_Mixer_DSP_Voice::CLastParams *pLastParams = (CSC_Mixer_DSP_Voice::CLastParams *)Voice.m_pChainInstance->GetLastParamPtr(0);
		pLastParams->m_SingleVolume = _Volume;
	}
}

void CSC_Mixer::Voice_SetPitch(uint32 _Voice, fp32 _Pitch)
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	DLock(m_MiscLock);
	CSC_Mixer_WorkerContext::CVoice &Voice = pWorkContext->m_Voices[_Voice];
	if (Voice.m_bInvalid == 3)
		return;

	Voice.m_CurrentPitch = Voice.m_OriginalPitch * _Pitch;
	Voice.m_CurrentPitchInv = 1.0f / Voice.m_CurrentPitch;
}


CMTime CSC_Mixer::Voice_GetPosition(uint32 _Voice)
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	CSC_Mixer_WorkerContext::CVoice &Voice = pWorkContext->m_Voices[_Voice];
	CMTime Offset = CMTime::CreateFromTicks(Voice.m_SamplePos % Voice.m_nSamples, pWorkContext->m_SampleTime / Voice.m_OriginalPitch);

	fp32 TimeOffset = ((CMTime::GetCPU() - m_LastFrameStart)).GetTime();
	
	Offset.Add(CMTime::CreateFromSeconds(TimeOffset * ( Voice.m_OriginalPitch / Voice.m_CurrentPitch)));
	return Offset;
}

int64 CSC_Mixer::Voice_GetSamplePos(uint32 _Voice)
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	CSC_Mixer_WorkerContext::CVoice &Voice = pWorkContext->m_Voices[_Voice];
	return Voice.m_SamplePos;
}

void CSC_Mixer::Voice_SetSamplePos(uint32 _Voice, int64 _SamplePos)
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	CSC_Mixer_WorkerContext::CVoice &Voice = pWorkContext->m_Voices[_Voice];
	Voice.m_SamplePos = _SamplePos;
}


void CSC_Mixer::Voice_SubmitPacket(uint32 _Voice, CSC_Mixer_WorkerContext::CSC_Mixer_Packet *_pPacket)
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	CSC_Mixer_WorkerContext::CVoice &Voice = pWorkContext->m_Voices[_Voice];
	{
		DLock(m_VoiceQueueLock);
		Voice.m_QueuedPackets.Insert(_pPacket);
	}
}

CSC_Mixer_WorkerContext::CSC_Mixer_Packet *CSC_Mixer::Voice_PopFinishedPacket(uint32 _Voice)
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	CSC_Mixer_WorkerContext::CVoice &Voice = pWorkContext->m_Voices[_Voice];
	CSC_Mixer_WorkerContext::CSC_Mixer_Packet *pPacket;
	{
		DLock(m_VoiceQueueLock);
		pPacket = Voice.m_FinishedPackets.Pop();
	}
	return pPacket;
}


CSC_Mixer_OuputFrame *CSC_Mixer::StartNewFrame()
{
	CSC_Mixer_WorkerContext *pWorkContext = GetWorkContext();
	CSC_Mixer_OuputFrame *pOutput = (CSC_Mixer_OuputFrame *)m_FinishedOutputFrame.Exchange(0);
//	M_IMPORTBARRIER;
	m_StartNewFrame.Increase();
	MRTC_SystemInfo::Event_SetSignaled(m_Event.m_pSemaphore);
//	m_Event.Signal();

	if (!pOutput)
	{
//		M_TRACEALWAYS("Missed frame\n");
	}

	return pOutput;
}


void CSC_Mixer_WorkerContext::CDSPChainInstanceInternal::ClearDSP()
{
	m_pChain = NULL;
	m_Data.Clear();
	m_MixBinRef.Clear();
	m_Indices.Clear();
	m_pParamIndices = NULL;
	m_pInternalDataIndices = NULL;
	m_bNewlyCreated = true;
	m_bActive = false;
}

bint CSC_Mixer_WorkerContext::CDSPChainInstanceInternal::SetDSPChain(CDSPChainInternal *_pChain)
{
	m_pChain = _pChain;
	mint nInstances = _pChain->m_nDSPInstances;
	if (!AllocIndices(nInstances))
		return false;
	{
		mint DataPtr = 0;
		{
			CDSPInstanceInternalIterator Iter = _pChain->m_DSPInstances;
			for (mint i = 0; i < nInstances; ++i)
			{
				if (!Iter)
					M_BREAKPOINT; // Not good

				int iDSP = Iter->m_iChainDSP;
				int ParamSize = CSC_Mixer_WorkerContext::ms_DSP_NumParams[Iter->m_DSPID];

				m_pParamIndices[iDSP] = DataPtr; DataPtr += (ParamSize + 15) & (~15);

				++Iter;
			}
		}
		m_CopyDataStart = DataPtr;
		{
			CDSPInstanceInternalIterator Iter = _pChain->m_DSPInstances;
			for (mint i = 0; i < nInstances; ++i)
			{
				if (!Iter)
					M_BREAKPOINT; // Not good

				int iDSP = Iter->m_iChainDSP;

				int LastParamSize = CSC_Mixer_WorkerContext::ms_DSP_NumLastParams[Iter->m_DSPID];
				m_pLastParamIndices[iDSP] = DataPtr; DataPtr += (LastParamSize + 15) & (~15);

				++Iter;
			}
		}
		{
			CDSPInstanceInternalIterator Iter = _pChain->m_DSPInstances;
			for (mint i = 0; i < nInstances; ++i)
			{
				if (!Iter)
					M_BREAKPOINT; // Not good
				int iDSP = Iter->m_iChainDSP;
				int InternalSize = CSC_Mixer_WorkerContext::ms_DSP_NumInternal[Iter->m_DSPID];
				m_pInternalDataIndices[iDSP] = DataPtr; DataPtr += (InternalSize + 15) & (~15);
				++Iter;
			}
		}
		if (!m_Data.SetLen(DataPtr))
			return false;
	}
	{
		CDSPInstanceInternalIterator Iter = _pChain->m_DSPInstances;
		for (mint i = 0; i < nInstances; ++i)
		{
			if (!Iter)
				M_BREAKPOINT; // Not good

			uint32 iDSP = Iter->m_iChainDSP;
			uint32 iDSPID = Iter->m_DSPID;
			if (CSC_Mixer_WorkerContext::ms_DSP_Func_InitData[iDSPID])
			{
				if (!CSC_Mixer_WorkerContext::ms_DSP_Func_InitData[iDSPID](GetInternalPtr(iDSP), GetParamPtr(iDSP), GetLastParamPtr(iDSP)))
					return false;
			}

			++Iter;
		}
	}
	return true;
}

bint CSC_Mixer_WorkerContext::CDSPChainInstanceInternal::AllocIndices(int _nDSPInstances)
{
	if (!m_MixBinRef.SetLen(_nDSPInstances))
		return false;
	if (!m_Indices.SetLen(_nDSPInstances * 3))
		return false;
	m_pParamIndices = m_Indices.GetBasePtr();
	m_pLastParamIndices = m_pParamIndices + _nDSPInstances;
	m_pInternalDataIndices = m_pLastParamIndices + _nDSPInstances;
	return true;
}

mint CSC_Mixer_WorkerContext::ms_DataOffset = 0;
TCDynamicPtr<CSC_Mixer_WorkerContext::CCustomPtrHolder, CSC_Mixer_WorkerContext> CSC_Mixer_WorkerContext::ms_pThis = {0};


#ifndef M_COMPILING_ON_VPU
	// NONVPU stuff here
TCSimpleBlockManager<> *CSC_Mixer_WorkerContext::ms_pBlockManager = NULL;
NThread::CMutual *CSC_Mixer_WorkerContext::ms_pBlockManagerLock = NULL;

#endif

void SCMixer_PPUAccess_GetNewMixBin(void *_pDest)
{
	TCDynamicPtr<CSC_Mixer_WorkerContext::CCustomPtrHolder, CSC_Mixer_WorkerContext::CMixBinContainer> *pDest = (TCDynamicPtr<CSC_Mixer_WorkerContext::CCustomPtrHolder, CSC_Mixer_WorkerContext::CMixBinContainer> *)_pDest;
	*pDest = CSC_Mixer_WorkerContext::New<CSC_Mixer_WorkerContext::CMixBinContainer>(ESCMixer_MaxChannels * CSC_Mixer_WorkerContext::ms_pThis->m_FrameLength);
	(*pDest)->m_nChannels = 0;

	uint8 *pDestMem = (uint8 *)CSC_Mixer::ms_pThis->m_WorkContextBlockManagerMemoryVPU.GetBasePtr();
	uint8 *pSrcMem = (uint8 *)CSC_Mixer::ms_pThis->m_WorkContextBlockManagerMemory.GetBasePtr();

	memcpy(pDestMem + pDest->m_PtrData.m_Offset, pSrcMem + pDest->m_PtrData.m_Offset, sizeof(CSC_Mixer_WorkerContext::CMixBinContainer));

	CSC_Mixer_WorkerContext::ms_pThis->m_FreeContainters.Push(*pDest);
	++CSC_Mixer_WorkerContext::ms_pThis->m_nCreatedContainers;

	M_TRACEALWAYS("Increased number of mixbins to %d. They take %d KiB\n", CSC_Mixer_WorkerContext::ms_pThis->m_nCreatedContainers, (ESCMixer_MaxChannels * CSC_Mixer_WorkerContext::ms_pThis->m_FrameLength * CSC_Mixer_WorkerContext::ms_pThis->m_nCreatedContainers * 4) >> 10);
}

void SCMixer_VPUCallManager(void *_pManagerContext, uint32 _Message, uint32 _Context, uint32 &_Ret0, uint32 &_Ret1)
{
	switch (_Message)
	{
	case ESCMixer_VPUCall_GetNewMixBin:
		{
			TCDynamicPtr<CSC_Mixer_WorkerContext::CCustomPtrHolder, CSC_Mixer_WorkerContext::CMixBinContainer> Dest;
			SCMixer_PPUAccess_GetNewMixBin(&Dest);
			_Ret0 = Dest.m_PtrData.m_Offset;
		}
	}
}

void SCMixer_StartVPUJob(void *_pWorkMemory, void *_pWorkParams)
{
	CVPU_JobDefinition JobDef;
	JobDef.AddSimpleBuffer(0, _pWorkMemory, 64*1024, VPU_IN_BUFFER);
	JobDef.AddSimpleBuffer(1, _pWorkParams, 1, VPU_IN_BUFFER);
	JobDef.SetJob(MHASH3('VPUC','ALCF','RAME'));
	uint32 TaskID = MRTC_ThreadPoolManager::VPU_AddTask(JobDef,VpuSoundContext, true);
	MRTC_ThreadPoolManager::VPU_BlockOnTask(TaskID, VpuSoundContext, SCMixer_VPUCallManager);


}




