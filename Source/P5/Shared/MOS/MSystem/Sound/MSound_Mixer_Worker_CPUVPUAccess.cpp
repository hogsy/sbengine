

#define M_COMPILING_ON_VPU

#include "MSound_Mixer_Worker.h"
#include "MRTC_StringHash.h"


#ifdef PLATFORM_XENON
#include "xtl.h"
#include "tracerecording.h"
#pragma comment( lib, "tracerecording.lib" )
#endif


void SCMixer_VPUAccess_GetVPUChangableForVoice(uint32 _iVoice, void *_pDest)
{
	NVPU::CSC_Mixer_WorkerContext::CVoiceVPUChange *pDest = (NVPU::CSC_Mixer_WorkerContext::CVoiceVPUChange *)_pDest;
	NVPU::CSC_Mixer_WorkerContext *pThis = NVPU::CSC_Mixer_WorkerContext::ms_pThis;

	NVPU::CSC_Mixer_WorkerContext::CVoice &Voice = pThis->m_Voices[_iVoice];

	pDest->m_pCurrentPacket = Voice.m_pCurrentPacket;
	pDest->m_CurrentPacketPos = Voice.m_CurrentPacketPos;
	pDest->m_SamplePos = Voice.m_SamplePos;
	pDest->m_SamplesLeft = Voice.m_SamplesLeft;
	pDest->m_LoopSkipCurrent = Voice.m_LoopSkipCurrent;
	pDest->m_bLoopSkipDone = Voice.m_bLoopSkipDone;

}

void SCMixer_VPUAccess_DequePendingPacket(uint32 _iVoice, void *_pDest)
{
	TCDynamicPtr<NVPU::CSC_Mixer_WorkerContext::CCustomPtrHolder, NVPU::CSC_Mixer_WorkerContext::CSC_Mixer_Packet> *pDest = (TCDynamicPtr<NVPU::CSC_Mixer_WorkerContext::CCustomPtrHolder, NVPU::CSC_Mixer_WorkerContext::CSC_Mixer_Packet> *)_pDest;
	NVPU::CSC_Mixer_WorkerContext *pThis = NVPU::CSC_Mixer_WorkerContext::ms_pThis;

	NVPU::CSC_Mixer_WorkerContext::CVoice &Voice = pThis->m_Voices[_iVoice];

	*pDest = Voice.m_PendingFinishedPackets.Pop();
}


void SCMixer_StartVPUJob(void *_pWorkMemory, void *_pWorkParams);

void SCMixer_VPUAccess_CalculateFrame(void *_pOutput, void *_pWorkMemory)
{
	NVPU::CSC_Mixer_WorkerContext *pThis = NVPU::CSC_Mixer_WorkerContext::ms_pThis;
#ifdef PLATFORM_XENON
	static bint bTraceRecord = false;
	if (bTraceRecord)
		XTraceStartRecording( "cache:\\MixerCalcFrame.bin" );
#endif
	TCDynamicPtr<NVPU::CSC_Mixer_WorkerContext::CCustomPtrHolder, NVPU::CSC_Mixer_OuputFrame> *pOutput = (TCDynamicPtr<NVPU::CSC_Mixer_WorkerContext::CCustomPtrHolder, NVPU::CSC_Mixer_OuputFrame> *)_pOutput;

#ifdef PLATFORM_PS3
	
//	NVPU::CSC_Mixer_WorkerContext::CVPU_WorkParams Params;
	pThis->m_WorkParams.m_pThis = pThis;
	pThis->m_WorkParams.m_pOutput = *pOutput;
	SCMixer_StartVPUJob(_pWorkMemory, &pThis->m_WorkParams);
#else
	pThis->CalculateFrame(*pOutput);
#endif

#ifdef PLATFORM_XENON
	if (bTraceRecord)
	{
		XTraceStopRecording();
		bTraceRecord = false;
	}
#endif

}

void SCMixer_VPUAccess_SetOffest(mint _Offset)
{
	NVPU::CSC_Mixer_WorkerContext::ms_DataOffset = _Offset;
}

void SCMixer_VPUAccess_SetThis(void *_pPtr)
{
	TCDynamicPtr<NVPU::CSC_Mixer_WorkerContext::CCustomPtrHolder, NVPU::CSC_Mixer_WorkerContext> *pSrc = (TCDynamicPtr<NVPU::CSC_Mixer_WorkerContext::CCustomPtrHolder, NVPU::CSC_Mixer_WorkerContext> *)_pPtr;
	NVPU::CSC_Mixer_WorkerContext::ms_pThis = *pSrc;
}


#ifdef PLATFORM_PS3
mint NVPU::CSC_Mixer_WorkerContext::ms_DataOffset = 0;
TCDynamicPtr<NVPU::CSC_Mixer_WorkerContext::CCustomPtrHolder, NVPU::CSC_Mixer_WorkerContext> NVPU::CSC_Mixer_WorkerContext::ms_pThis = {0};
#endif
