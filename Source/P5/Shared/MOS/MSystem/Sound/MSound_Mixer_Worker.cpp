
#define M_COMPILING_ON_VPU
#define USE_SPU_PRINTF

#include "../../../MCC/VPU/VPU_Platform.h"
#ifdef PLATFORM_SPU
#include "../../../MCC/VPU/MRTC_VPU.h"
#define PLATFORM_DMA
#endif
#include "../../../MCC/MMath_vec128.h"
#include "MSound_Mixer_Worker.h"

M_INLINE static int TruncToInt( fp32 _Val )
{
	return (int)_Val;
}

M_INLINE static int TruncToInt( fp64 _Val )
{
	return (int)_Val;
}

M_INLINE static int RoundToInt( fp32 _Val )
{
	return (int)(( _Val < 0.0f )? ( _Val - 0.5f ) : ( _Val + 0.5f ));
}

M_INLINE static int RoundToInt(fp64 _Val)
{
	return (_Val < 0.0) ? int(_Val-0.5) : int(_Val + 0.5);
};

#ifdef M_COMPILING_ON_VPU
namespace NVPU
{
#endif

	enum
	{
		EPacketCacheSize = 4096
	};
	void *g_pPacketCache;

#include "MSound_Mixer_DSPEffects.h"

#include "MSound_Mixer_Internal_DSP_Master.impvpu.h"
#include "MSound_Mixer_Internal_DSP_Voice.impvpu.h"
#include "MSound_Mixer_Internal_DSP_Router.impvpu.h"
#include "MSound_Mixer_Internal_DSP_SilenceGen.impvpu.h"
#include "MSound_Mixer_Internal_DSP_VolumeMatrix.impvpu.h"
#include "MSound_Mixer_DSP_Volume.impvpu.h"
#include "MSound_Mixer_DSP_Copy.impvpu.h"
#include "MSound_Mixer_DSP_BiQuad.impvpu.h"
#include "MSound_Mixer_DSP_Reverb.impvpu.h"

/*
	ESC_Mixer_DSP_Master = 0,
	ESC_Mixer_DSP_Voice,
	ESC_Mixer_DSP_Router,
	ESC_Mixer_DSP_Max,
	*/

mint CSC_Mixer_WorkerContext::ms_DataOffset = 0;
TCDynamicPtr<CSC_Mixer_WorkerContext::CCustomPtrHolder, CSC_Mixer_WorkerContext> CSC_Mixer_WorkerContext::ms_pThis = {0};

uint32 CSC_Mixer_WorkerContext::ms_DSP_NumParams[ESC_Mixer_DSP_Max] = 
{
	CSC_Mixer_DSP_Master::ENumParams,
	CSC_Mixer_DSP_Voice::ENumParams,
	CSC_Mixer_DSP_Router::ENumParams,
	CSC_Mixer_DSP_SilenceGen::ENumParams,
	CSC_Mixer_DSP_VolumeMatrix::ENumParams,
	CSC_Mixer_DSP_Reverb::ENumParams,
	CSC_Mixer_DSP_Volume::ENumParams,
	CSC_Mixer_DSP_Copy::ENumParams,
	CSC_Mixer_DSP_BiQuad::ENumParams,
};

uint32 CSC_Mixer_WorkerContext::ms_DSP_NumLastParams[ESC_Mixer_DSP_Max] = 
{
	CSC_Mixer_DSP_Master::ENumLastParams,
	CSC_Mixer_DSP_Voice::ENumLastParams,
	CSC_Mixer_DSP_Router::ENumLastParams,
	CSC_Mixer_DSP_SilenceGen::ENumLastParams,
	CSC_Mixer_DSP_VolumeMatrix::ENumLastParams,
	CSC_Mixer_DSP_Reverb::ENumLastParams,
	CSC_Mixer_DSP_Volume::ENumLastParams,
	CSC_Mixer_DSP_Copy::ENumLastParams,
	CSC_Mixer_DSP_BiQuad::ENumLastParams,
};

uint32 CSC_Mixer_WorkerContext::ms_DSP_NumInternal[ESC_Mixer_DSP_Max] = 
{
	CSC_Mixer_DSP_Master::ENumInternal,
	CSC_Mixer_DSP_Voice::ENumInternal,
	CSC_Mixer_DSP_Router::ENumInternal,
	CSC_Mixer_DSP_SilenceGen::ENumInternal,
	CSC_Mixer_DSP_VolumeMatrix::ENumInternal,
	CSC_Mixer_DSP_Reverb::ENumInternal,
	CSC_Mixer_DSP_Volume::ENumInternal,
	CSC_Mixer_DSP_Copy::ENumInternal,
	CSC_Mixer_DSP_BiQuad::ENumInternal,
};
uint32 CSC_Mixer_WorkerContext::ms_DSP_ProcessingType[ESC_Mixer_DSP_Max] = 
{
	CSC_Mixer_DSP_Master::EProcessingType,
	CSC_Mixer_DSP_Voice::EProcessingType,
	CSC_Mixer_DSP_Router::EProcessingType,
	CSC_Mixer_DSP_SilenceGen::EProcessingType,
	CSC_Mixer_DSP_VolumeMatrix::EProcessingType,
	CSC_Mixer_DSP_Reverb::EProcessingType,
	CSC_Mixer_DSP_Volume::EProcessingType,
	CSC_Mixer_DSP_Copy::EProcessingType,
	CSC_Mixer_DSP_BiQuad::EProcessingType,
};
FSCMixer_DSPFunc_ProcessFrame *CSC_Mixer_WorkerContext::ms_DSP_Func_ProcessFrame[ESC_Mixer_DSP_Max] = 
{
	CSC_Mixer_DSP_Master::ProcessFrame,
	CSC_Mixer_DSP_Voice::ProcessFrame,
	CSC_Mixer_DSP_Router::ProcessFrame,
	CSC_Mixer_DSP_SilenceGen::ProcessFrame,
	CSC_Mixer_DSP_VolumeMatrix::ProcessFrame,
	CSC_Mixer_DSP_Reverb::ProcessFrame,
	CSC_Mixer_DSP_Volume::ProcessFrame,
	CSC_Mixer_DSP_Copy::ProcessFrame,
	CSC_Mixer_DSP_BiQuad::ProcessFrame,
};
FSCMixer_DSPFunc_PreProcessFrame *CSC_Mixer_WorkerContext::ms_DSP_Func_PreProcessFrame[ESC_Mixer_DSP_Max] = 
{
	CSC_Mixer_DSP_Master::PreProcessFrame,
	NULL,
	CSC_Mixer_DSP_Router::PreProcessFrame,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};



void CSC_Mixer_WorkerContext::PreProcessDSPInstance(CDSPChainInstanceInternal *_pChainInstance, CDSPInstanceInternal *_pDSPInstance)
{
	uint32 iDSP = _pDSPInstance->m_iChainDSP;
	
	uint32 DSPID = _pDSPInstance->m_DSPID;
	if (CSC_Mixer_WorkerContext::ms_DSP_Func_PreProcessFrame[DSPID])
		CSC_Mixer_WorkerContext::ms_DSP_Func_PreProcessFrame[DSPID](_pChainInstance->GetInternalPtr(iDSP), _pChainInstance->GetParamPtr(iDSP), _pChainInstance->GetLastParamPtr(iDSP));
	
	uint32 ProcessingType = CSC_Mixer_WorkerContext::ms_DSP_ProcessingType[DSPID];

	if (ProcessingType != ESC_Mixer_ProcessingType_In)
	{
		CDSPInstanceLinkIter Iter = _pDSPInstance->m_RouteTo;
		while (Iter)
		{
			CDSPInstanceInternal *pIterInstance = Iter->m_pDSPInstance;
			uint32 iIterDSP = pIterInstance->m_iChainDSP;
			CMixBinRef &MixBinRef = _pChainInstance->m_MixBinRef[iIterDSP];
			++MixBinRef.m_nToMix;
//			M_TRACEALWAYS("PreProcess %d: PreProcessDSPInstance\n", MixBinRef.m_nToMix);

			if (MixBinRef.m_nToMix == 1)
				PreProcessDSPInstance(_pChainInstance, pIterInstance);

			++Iter;
		}
	}

}

void CSC_Mixer_WorkerContext::PreProcessDSPChainInstance(CDSPChainInstanceInternal *_pChainInstance)
{
	CDSPChainInternal *pChainInstance = _pChainInstance->m_pChain;
	// Start with first in list
	PreProcessDSPInstance(_pChainInstance, pChainInstance->m_DSPInstances.GetFirst());
}

void CSC_Mixer_WorkerContext::ProcessDSPInstance(CDSPChainInstanceInternal *_pChainInstance, CDSPInstanceInternal *_pDSPInstance, CMixBinContainer *_pMixBin)
{
	uint32 iDSP = _pDSPInstance->m_iChainDSP;
	
	CProcessingInfoInternal ProcessingInfo;
	ProcessingInfo.m_pContainerIn = _pMixBin;
	ProcessingInfo.m_pDataOut = NULL;
	ProcessingInfo.m_pContainerOut = NULL;
	uint32 DSPID = _pDSPInstance->m_DSPID;
	uint32 ProcessingType = CSC_Mixer_WorkerContext::ms_DSP_ProcessingType[DSPID];

	switch(ProcessingType)
	{
	case ESC_Mixer_ProcessingType_Out:
	case ESC_Mixer_ProcessingType_InOut:
		{
			ProcessingInfo.m_pContainerOut = GetMixBinContainer();
			if (ProcessingType == ESC_Mixer_ProcessingType_Out)
				break;
		}
	case ESC_Mixer_ProcessingType_In:
	case ESC_Mixer_ProcessingType_InPlace:
		{
			if (!ProcessingInfo.m_pContainerIn)
				M_BREAKPOINT; // We need a container to come in here or we will be playing garbage

		}
		break;
	}

	ProcessingInfo.m_nSamples = m_FrameLength;
	CSC_Mixer_WorkerContext::CBixBinCointanerPtr ContainersPtrs[2];
	uint32 nContainers = 0;
	if (ProcessingInfo.m_pContainerIn)
	{
		ProcessingInfo.m_nChannels = ProcessingInfo.m_pContainerIn->m_nChannels;
		ContainersPtrs[nContainers].m_bDiscard = false;
		ContainersPtrs[nContainers].m_pContainer = ProcessingInfo.m_pContainerIn;
		++nContainers;
	}
	else
	{
		ProcessingInfo.m_nChannels = 0;
	}

	if (ProcessingInfo.m_pContainerOut)
	{
		ContainersPtrs[nContainers].m_bDiscard = true;
		ContainersPtrs[nContainers].m_pContainer = ProcessingInfo.m_pContainerOut;
		++nContainers;
	}

	GetMixBinContainerPtrs(ContainersPtrs, nContainers);
	nContainers = 0;
	if (ProcessingInfo.m_pContainerIn)
		ProcessingInfo.m_pDataIn = ContainersPtrs[nContainers++].m_pPtr;

	if (ProcessingInfo.m_pContainerOut)
		ProcessingInfo.m_pDataOut = ContainersPtrs[nContainers++].m_pPtr;

	if (CSC_Mixer_WorkerContext::ms_DSP_Func_ProcessFrame[DSPID])
		CSC_Mixer_WorkerContext::ms_DSP_Func_ProcessFrame[DSPID](&ProcessingInfo, _pChainInstance->GetInternalPtr(iDSP), _pChainInstance->GetParamPtr(iDSP), _pChainInstance->GetLastParamPtr(iDSP));

	TCDynamicPtr<CSC_Mixer_WorkerContext::CCustomPtrHolder, CSC_Mixer_WorkerContext::CMixBinContainer> pProcessContainer;
	pProcessContainer = NULL;
	switch(ProcessingType)
	{
	case ESC_Mixer_ProcessingType_Out:
		pProcessContainer = ProcessingInfo.m_pContainerOut;
		break;
	case ESC_Mixer_ProcessingType_InOut:
		if (ProcessingInfo.m_pContainerIn)
			ReturnMixBinContainer(ProcessingInfo.m_pContainerIn);
		pProcessContainer = ProcessingInfo.m_pContainerOut;
		break;
	case ESC_Mixer_ProcessingType_In:
		if (ProcessingInfo.m_pContainerIn)
			ReturnMixBinContainer(ProcessingInfo.m_pContainerIn);
		break;
	case ESC_Mixer_ProcessingType_InPlace:
		pProcessContainer = ProcessingInfo.m_pContainerIn;
		break;
	}
	

	if (pProcessContainer)
	{
		pProcessContainer->m_nChannels = ProcessingInfo.m_nChannels;

		CDSPInstanceLinkIter Iter = _pDSPInstance->m_RouteTo;
		while (Iter)
		{
			CDSPInstanceInternal *pIterInstance = Iter->m_pDSPInstance;
			++Iter;

			bint bLast = !Iter;
			uint32 iIterDSP = pIterInstance->m_iChainDSP;
			CMixBinRef &MixBinRef = _pChainInstance->m_MixBinRef[iIterDSP];
			--MixBinRef.m_nToMix;
//			M_TRACEALWAYS("Process %d: ProcessDSPInstance\n", MixBinRef.m_nToMix);
			if (MixBinRef.m_nToMix < 0)
				M_BREAKPOINT;

			if (!MixBinRef.m_pBinContainer)
			{
				if (bLast)
				{
					// If we are the last buffer just
					MixBinRef.m_pBinContainer = pProcessContainer;
					pProcessContainer = NULL;
				}
				else
					MixBinRef.m_pBinContainer = CSC_Mixer_DSP_Router::CopyContainer(this, pProcessContainer);
			}
			else
			{
				CSC_Mixer_WorkerContext::CMixBinContainer *pResult = CSC_Mixer_DSP_Router::MixBuffer(this, MixBinRef.m_pBinContainer, pProcessContainer, bLast);
				MixBinRef.m_pBinContainer = pResult;
				pProcessContainer = NULL;
			}

			// All have mixed into this buffer
			if (MixBinRef.m_nToMix == 0)
			{
				// Continue Processing
				CSC_Mixer_WorkerContext::CMixBinContainer *pContainer = MixBinRef.m_pBinContainer;
				MixBinRef.m_pBinContainer = NULL;
				ProcessDSPInstance(_pChainInstance, pIterInstance, pContainer);
			}

		}

		if (pProcessContainer)
		{
			ReturnMixBinContainer(pProcessContainer);
		}
	}
	else
	{
		if (ProcessingType != ESC_Mixer_ProcessingType_In)
			M_BREAKPOINT;
	}
}


void CSC_Mixer_WorkerContext::ProcessDSPChainInstance(CDSPChainInstanceInternal *_pChainInstance, CMixBinContainer *_pMixBin)
{
	CDSPChainInternal *pChainInstance = _pChainInstance->m_pChain;
	// Start with first in list
	ProcessDSPInstance(_pChainInstance, pChainInstance->m_DSPInstances.GetFirst(), _pMixBin);
}


CSC_Mixer_WorkerContext::CMixBinContainer *CSC_Mixer_WorkerContext::GetMixBinContainer()
{
	CSC_Mixer_WorkerContext::CMixBinContainer *pRet;
	{
		pRet = m_FreeContainters.Pop();
	}

    if (!pRet)
	{
#ifdef M_COMPILING_ON_VPU
		TCDynamicPtr<CSC_Mixer_WorkerContext::CCustomPtrHolder, CSC_Mixer_WorkerContext::CMixBinContainer> Dest;
		SCMixer_PPUAccess_GetNewMixBin(&Dest);
		return Dest;
#else
		pRet = New<CMixBinContainer>(ESCMixer_MaxChannels * m_FrameLength);
		pRet->m_nChannels = 0;
		++m_nCreatedContainers;
		M_TRACEALWAYS("Increased number of mixbins to %d. They take %d KiB\n", m_nCreatedContainers, (ESCMixer_MaxChannels * m_FrameLength * m_nCreatedContainers * 4) >> 10);
#endif
	}
	return pRet;
}

#ifdef PLATFORM_SPU
class CSC_Mixer_WorkerContext_DMAHelper
{
public:
	enum
	{
		ENumCacheEntries = 3,
	};

	class CMixBitCacheEntry
	{
	public:
		CSC_Mixer_WorkerContext::CMixBinContainer *m_pCachedContainer;
		vec128 *m_pCacheAddr;
	};

	CMixBitCacheEntry m_MixBinCacheEntries[ENumCacheEntries];
	mint m_CacheEntrySize;

	CMixBitCacheEntry *FindCacheEntry(CSC_Mixer_WorkerContext::CMixBinContainer *_pContainer)
	{
		for (int i = 0; i < ENumCacheEntries; ++i)
		{
			if (m_MixBinCacheEntries[i].m_pCachedContainer == _pContainer)
			{
				return m_MixBinCacheEntries + i;
			}
		}
		return NULL;
	}

	void FreeContainer(CSC_Mixer_WorkerContext::CMixBinContainer *_pContainer)
	{
		CMixBitCacheEntry *pCacheEntry = FindCacheEntry(_pContainer);
		if (pCacheEntry)
		{
			pCacheEntry->m_pCachedContainer = NULL;
		}				
	}

	void FlushContainer(uint32 _iContainer)
	{
		MRTC_System_VPU::OS_DMATransferToSys((mint)m_MixBinCacheEntries[_iContainer].m_pCachedContainer->m_pData0, m_MixBinCacheEntries[_iContainer].m_pCacheAddr, m_CacheEntrySize);
		m_MixBinCacheEntries[_iContainer].m_pCachedContainer = NULL;
	}

	void Init(uint32 _nSamples)
	{
		m_CacheEntrySize = _nSamples*ESCMixer_MaxChannelVectors*16;
		for (int i = 0; i < ENumCacheEntries; ++i)
		{
			m_MixBinCacheEntries[i].m_pCachedContainer = NULL;
			m_MixBinCacheEntries[i].m_pCacheAddr = (vec128 *)MRTC_System_VPU::OS_LocalHeapAlloc(m_CacheEntrySize);
		}
	}
};
CSC_Mixer_WorkerContext_DMAHelper g_DMAHelper;
#endif

void CSC_Mixer_WorkerContext::ReturnMixBinContainer(CMixBinContainer *_pContainer)
{
#ifdef PLATFORM_SPU
	g_DMAHelper.FreeContainer(_pContainer);
#endif
	m_FreeContainters.Push(_pContainer);
}

void CSC_Mixer_WorkerContext::GetMixBinContainerPtrs(CBixBinCointanerPtr *_pContainers, mint _nContainers)
{
#ifdef PLATFORM_SPU
	M_ASSERT(_nContainers <= CSC_Mixer_WorkerContext_DMAHelper::ENumCacheEntries, "!");

	uint32 Tagged[CSC_Mixer_WorkerContext_DMAHelper::ENumCacheEntries] = {0};
	uint32 TaggedIndex[CSC_Mixer_WorkerContext_DMAHelper::ENumCacheEntries] = {0};
	uint32 nFree = 0;
	uint32 nNeeded = _nContainers;
	for (mint i = 0; i < CSC_Mixer_WorkerContext_DMAHelper::ENumCacheEntries; ++i)
	{
		if (g_DMAHelper.m_MixBinCacheEntries[i].m_pCachedContainer)
		{
			for (mint j = 0; j < _nContainers; ++j)
			{
				if (g_DMAHelper.m_MixBinCacheEntries[i].m_pCachedContainer == _pContainers[j].m_pContainer)
				{
					Tagged[i] = 1;
					TaggedIndex[j] = 1;
					_pContainers[j].m_pPtr = g_DMAHelper.m_MixBinCacheEntries[i].m_pCacheAddr;
					--nNeeded;
					break;
				}
			}
		}
		else
			++nFree;
	}
	// Free up containers that are needed
	if (nFree < nNeeded)
	{
		for (mint i = 0; i < CSC_Mixer_WorkerContext_DMAHelper::ENumCacheEntries; ++i)
		{
			if (g_DMAHelper.m_MixBinCacheEntries[i].m_pCachedContainer && !Tagged[i])
			{
				g_DMAHelper.FlushContainer(i);
				++nFree;
				if (nFree >= nNeeded)
					break;
			}
		}
	}
	// Fill containers
	if (nNeeded)
	{
		int iCurrent = 0;
		for (mint i = 0; i < CSC_Mixer_WorkerContext_DMAHelper::ENumCacheEntries; ++i)
		{
			CSC_Mixer_WorkerContext_DMAHelper::CMixBitCacheEntry &Entry = g_DMAHelper.m_MixBinCacheEntries[i];
			if (!Entry.m_pCachedContainer)
			{
				while (TaggedIndex[iCurrent])
					++iCurrent;

				CBixBinCointanerPtr &ContainerPtr = _pContainers[iCurrent];

				Entry.m_pCachedContainer = ContainerPtr.m_pContainer;
				ContainerPtr.m_pPtr = Entry.m_pCacheAddr;
				if (!ContainerPtr.m_bDiscard)
				{
					MRTC_System_VPU::OS_DMATransferFromSys(Entry.m_pCacheAddr, (mint)ContainerPtr.m_pContainer->m_pData0, g_DMAHelper.m_CacheEntrySize);
				}
				++iCurrent;
				--nNeeded;
				if (!nNeeded)
					break; // Done
			}
		}
	}
#else
	for (int i = 0; i < _nContainers; ++i)
	{
		_pContainers[i].m_pPtr = _pContainers[i].m_pContainer->m_pData0;
	}
#endif
}

void CSC_Mixer_WorkerContext::CalculateFrame(CSC_Mixer_OuputFrame *_pOutput)
{
	CSC_Mixer_OuputFrame *pOutput = _pOutput;

	CSC_Mixer_DSP_Master::CParams *pDSPParams = (CSC_Mixer_DSP_Master::CParams *)m_pMasterInstance->GetParamPtr(0);
	pDSPParams->m_pOutputFrame = pOutput;

	// Preprocess
	{
		CVoiceIter Iter = m_VoicesPlaying;

		while (Iter)
		{
			CVoice *pVoice = Iter;
			++Iter;

			if (pVoice->m_bPausedProcessing)
				continue;

			PreProcessDSPChainInstance(pVoice->m_pChainInstance);
		}
	}
	{
		CChainIter Iter = m_DSPChainInstancesProcess;
		while (Iter)
		{
			PreProcessDSPChainInstance(Iter);
			++Iter;
		}
	}
	// Process
	{

		CVoiceIter Iter = m_VoicesPlaying;
		while (Iter)
		{
			CVoice *pVoice = Iter;
			++Iter;

			if (pVoice->m_bPausedProcessing)
				continue;

			ProcessDSPChainInstance(pVoice->m_pChainInstance, NULL);
		}
	}
	{
		CChainIter Iter = m_DSPChainInstancesProcess;
		while (Iter)
		{
			ProcessDSPChainInstance(Iter, NULL);
			++Iter;
		}
	}

//	M_TRACEALWAYS("CalcFrame\n");

}


#ifdef M_COMPILING_ON_VPU
}
#endif


#ifdef PLATFORM_SPU


mint g_SysBufferAddr = 0;
void SCMixer_PPUAccess_GetNewMixBin(void *_pDest)
{

	uint32 RetOffset = 0;
	uint32 Dummy = 0;
	CVPU_WorkQueue::CallPPU(ESCMixer_VPUCall_GetNewMixBin, 0, RetOffset, Dummy);

	TCDynamicPtr<NVPU::CSC_Mixer_WorkerContext::CCustomPtrHolder, NVPU::CSC_Mixer_WorkerContext::CMixBinContainer> *pDest = (TCDynamicPtr<NVPU::CSC_Mixer_WorkerContext::CCustomPtrHolder, NVPU::CSC_Mixer_WorkerContext::CMixBinContainer> *)_pDest;
	pDest->m_PtrData.m_Offset = RetOffset;

	MRTC_System_VPU::OS_DMATransferFromSys((void *)(NVPU::CSC_Mixer_WorkerContext::ms_DataOffset + RetOffset), g_SysBufferAddr + RetOffset, sizeof(NVPU::CSC_Mixer_WorkerContext::CMixBinContainer));
}

extern "C" char stack_size[];

uint32 VPU_CalculateFrame(CVPU_JobInfo& _JobInfo)
{
	g_StackSize = 4*1024;

	CVPU_SimpleBuffer<uint8> InBuffer(_JobInfo.GetSimpleBuffer(0));
	CVPU_SimpleBuffer<NVPU::CSC_Mixer_WorkerContext::CVPU_WorkParams> WorkerParams(_JobInfo.GetSimpleBuffer(1));

	uint32 Size;
	mint BufferAddr = (mint)InBuffer.GetBuffer(Size);
	g_SysBufferAddr = InBuffer.GetSysAddr();

	NVPU::CSC_Mixer_WorkerContext::CVPU_WorkParams *pParams = WorkerParams.GetBuffer(Size);

	NVPU::CSC_Mixer_WorkerContext::ms_DataOffset = BufferAddr;
	NVPU::CSC_Mixer_WorkerContext::ms_pThis = pParams->m_pThis;

	NVPU::CSC_Mixer_WorkerContext *pThis = pParams->m_pThis;

#ifdef PLATFORM_SPU
	NVPU::g_DMAHelper.Init(pThis->m_FrameLength);
	NVPU::g_pPacketCache = MRTC_System_VPU::OS_LocalHeapAlloc(NVPU::EPacketCacheSize);
#endif

//	spu_printf("CalcFrame\n");

	pThis->CalculateFrame(pParams->m_pOutput);

	MRTC_System_VPU::OS_DMATransferToSys(InBuffer.GetSysAddr(), (void *)BufferAddr, InBuffer.GetElementCount());

	return 0;
}

MVPU_MAIN

MVPU_WORKER_BEGIN
MVPU_WORKER_DECLARE(MHASH3('VPUC','ALCF','RAME'),VPU_CalculateFrame);
MVPU_WORKER_END

#endif

