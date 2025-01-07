
/////////////////////////////////////////////////////////////////////////////////////////
// Handles resampling and volume etc

CSC_Mixer_WorkerContext::CMixBinContainer *CSC_Mixer_DSP_Router::MixBuffer(CSC_Mixer_WorkerContext *_pMixer, TCDynamicPtr<CSC_Mixer_WorkerContext::CCustomPtrHolder, CSC_Mixer_WorkerContext::CMixBinContainer> &_pIn0, TCDynamicPtr<CSC_Mixer_WorkerContext::CCustomPtrHolder, CSC_Mixer_WorkerContext::CMixBinContainer> &_pIn1, bint _bCanDestroy1)
{
	// Mix in the buffer with the most number of channels

	CSC_Mixer_WorkerContext::CMixBinContainer *pSrc0 = _pIn0;
	CSC_Mixer_WorkerContext::CMixBinContainer *pSrc1 = _pIn1;

	if (pSrc1->m_nChannels > pSrc0->m_nChannels)
	{
		CSC_Mixer_WorkerContext::CMixBinContainer *pTemp = pSrc1;
		pSrc1 = pSrc0;
		pSrc0 = pTemp;
	}

	uint32 Src0Stride = (pSrc0->m_nChannels + 3) >> 2;
	uint32 Src1Stride = (pSrc1->m_nChannels + 3) >> 2;
	uint32 nSamples = _pMixer->m_FrameLength;

	CSC_Mixer_WorkerContext::CMixBinContainer *pRet = pSrc0;

	CSC_Mixer_WorkerContext::CBixBinCointanerPtr ContainersPtrs[3] = {0};


	uint32 nContainers = 2;
	ContainersPtrs[0].m_pContainer = pSrc0;
	ContainersPtrs[0].m_pPtr = 0;
	ContainersPtrs[0].m_bDiscard = false;
	ContainersPtrs[1].m_pContainer = pSrc1;
	ContainersPtrs[1].m_pPtr = 0;
	ContainersPtrs[1].m_bDiscard = false;


	bint bNewDest = false;
	if (!_bCanDestroy1 && pSrc1 == _pIn1)
	{
		bNewDest = true;
		pRet = _pMixer->GetMixBinContainer();
		ContainersPtrs[2].m_pContainer = pRet;
		ContainersPtrs[2].m_pPtr = 0;
		ContainersPtrs[2].m_bDiscard = true;
		nContainers = 3;
	}

	_pMixer->GetMixBinContainerPtrs(ContainersPtrs, nContainers);

	vec128 * M_RESTRICT pSrc0Samples = ContainersPtrs[0].m_pPtr;
	vec128 * M_RESTRICT pSrc1Samples = ContainersPtrs[1].m_pPtr;
	vec128 * M_RESTRICT pDstSamples;
	if (bNewDest)
		pDstSamples = ContainersPtrs[2].m_pPtr;
	else
		pDstSamples = ContainersPtrs[0].m_pPtr;

	M_PRECACHE128( 0, pSrc0Samples); // Precache
	M_PRECACHE128( 128, pSrc0Samples); // Precache
	M_PRECACHE128( 256, pSrc0Samples); // Precache
	M_PRECACHE128( 384, pSrc0Samples); // Precache
	M_PRECACHE128( 0, pSrc1Samples); // Precache
	M_PRECACHE128( 128, pSrc1Samples); // Precache
	M_PRECACHE128( 256, pSrc1Samples); // Precache
	M_PRECACHE128( 384, pSrc1Samples); // Precache

#define Macro_DoCopy \
			M_PRECACHE128( 512, pSrc0Samples);\
			M_PRECACHE128( 512, pSrc1Samples);\
			vec128 Src10 = pSrc0Samples[0];\
			vec128 Src11 = pSrc0Samples[1];\
			vec128 Src12 = pSrc0Samples[2];\
			vec128 Src13 = pSrc0Samples[3];\
			vec128 Src14 = pSrc0Samples[4];\
			vec128 Src15 = pSrc0Samples[5];\
			vec128 Src16 = pSrc0Samples[6];\
			vec128 Src17 = pSrc0Samples[7];\
			vec128 Src00 = pSrc1Samples[0];\
			vec128 Src01 = pSrc1Samples[1];\
			vec128 Src02 = pSrc1Samples[2];\
			vec128 Src03 = pSrc1Samples[3];\
			vec128 Src04 = pSrc1Samples[4];\
			vec128 Src05 = pSrc1Samples[5];\
			vec128 Src06 = pSrc1Samples[6];\
			vec128 Src07 = pSrc1Samples[7];\
			Src00 = M_VAdd(Src00, Src10);\
			Src01 = M_VAdd(Src01, Src11);\
			Src02 = M_VAdd(Src02, Src12);\
			Src03 = M_VAdd(Src03, Src13);\
			Src04 = M_VAdd(Src04, Src14);\
			Src05 = M_VAdd(Src05, Src15);\
			Src06 = M_VAdd(Src06, Src16);\
			Src07 = M_VAdd(Src07, Src17);\
			pDstSamples[0] = Src00;\
			pDstSamples[1] = Src01;\
			pDstSamples[2] = Src02;\
			pDstSamples[3] = Src03;\
			pDstSamples[4] = Src04;\
			pDstSamples[5] = Src05;\
			pDstSamples[6] = Src06;\
			pDstSamples[7] = Src07;\
			pSrc0Samples += 8;\
			pSrc1Samples += 8;\
			pDstSamples += 8;
	

	// 16 possible combinations
	if (Src1Stride == Src0Stride)
	{
		nSamples = nSamples / 8;
		nSamples *= Src1Stride;
		if (bNewDest)
		{
			for (uint32 i = 0; i < nSamples; ++i)
			{
				M_PREZERO128( 0, pDstSamples); // Precache
				Macro_DoCopy;
			}
		}
		else
		{
			for (uint32 i = 0; i < nSamples; ++i)
			{
				Macro_DoCopy;
			}
		}
	}

	switch (Src0Stride)
	{
	case 1:
		{
			for (uint32 i = 0; i < nSamples; ++i)
			{
				vec128 Src0 = pSrc0Samples[0];
				vec128 Src1 = pSrc1Samples[0];
				pDstSamples[0] = M_VAdd(Src0, Src1);
				pDstSamples += 1;
				pSrc0Samples += 1;
				pSrc1Samples += Src1Stride;
			}
		}
		break;
	case 2:
		{
			if (Src1Stride < Src0Stride)
			{
				for (uint32 i = 0; i < nSamples; ++i)
				{
					vec128 Src00 = pSrc0Samples[0];
					vec128 Src10 = pSrc1Samples[0];
					vec128 Src01 = pSrc0Samples[1];
					pDstSamples[0] = M_VAdd(Src00, Src10);
					pDstSamples[1] = Src01;
					pDstSamples += 2;
					pSrc0Samples += 2;
					pSrc1Samples += Src1Stride;
				}
			}
			else
			{
				for (uint32 i = 0; i < nSamples; ++i)
				{
					vec128 Src00 = pSrc0Samples[0];
					vec128 Src10 = pSrc1Samples[0];
					vec128 Src01 = pSrc0Samples[1];
					vec128 Src11 = pSrc1Samples[1];
					pDstSamples[0] = M_VAdd(Src00, Src10);
					pDstSamples[1] = M_VAdd(Src01, Src11);
					pDstSamples += 2;
					pSrc0Samples += 2;
					pSrc1Samples += Src1Stride;
				}
			}
		}
		break;
	case 3:
		{
			switch (Src1Stride)
			{
			case 1:
				{
					for (uint32 i = 0; i < nSamples; ++i)
					{
						vec128 Src00 = pSrc0Samples[0];
						vec128 Src10 = pSrc1Samples[0];
						vec128 Src01 = pSrc0Samples[1];
						vec128 Src02 = pSrc0Samples[2];
						pDstSamples[0] = M_VAdd(Src00, Src10);
						pDstSamples[1] = Src01;
						pDstSamples[2] = Src02;
						pDstSamples += 3;
						pSrc0Samples += 3;
						pSrc1Samples += Src1Stride;
					}
				}
				break;
			case 2:
				{
					for (uint32 i = 0; i < nSamples; ++i)
					{
						vec128 Src00 = pSrc0Samples[0];
						vec128 Src10 = pSrc1Samples[0];
						vec128 Src01 = pSrc0Samples[1];
						vec128 Src11 = pSrc1Samples[1];
						vec128 Src02 = pSrc0Samples[2];
						pDstSamples[0] = M_VAdd(Src00, Src10);
						pDstSamples[1] = M_VAdd(Src01, Src11);
						pDstSamples[2] = Src02;
						pDstSamples += 3;
						pSrc0Samples += 3;
						pSrc1Samples += Src1Stride;
					}
				}
				break;
			case 4:
				{
					for (uint32 i = 0; i < nSamples; ++i)
					{
						vec128 Src00 = pSrc0Samples[0];
						vec128 Src10 = pSrc1Samples[0];
						vec128 Src01 = pSrc0Samples[1];
						vec128 Src11 = pSrc1Samples[1];
						vec128 Src02 = pSrc0Samples[2];
						vec128 Src12 = pSrc1Samples[2];
						pDstSamples[0] = M_VAdd(Src00, Src10);
						pDstSamples[1] = M_VAdd(Src01, Src11);
						pDstSamples[2] = M_VAdd(Src02, Src12);
						pDstSamples += 3;
						pSrc0Samples += 3;
						pSrc1Samples += Src1Stride;
					}
				}
				break;
			}
		}
		break;
	case 4:
		{
			switch (Src1Stride)
			{
			case 1:
				{
					for (uint32 i = 0; i < nSamples; ++i)
					{
						vec128 Src00 = pSrc0Samples[0];
						vec128 Src10 = pSrc1Samples[0];
						vec128 Src01 = pSrc0Samples[1];
						vec128 Src02 = pSrc0Samples[2];
						vec128 Src03 = pSrc0Samples[3];
						pDstSamples[0] = M_VAdd(Src00, Src10);
						pDstSamples[1] = Src01;
						pDstSamples[2] = Src02;
						pDstSamples[3] = Src03;
						pDstSamples += 4;
						pSrc0Samples += 4;
						pSrc1Samples += Src1Stride;
					}
				}
				break;
			case 2:
				{
					for (uint32 i = 0; i < nSamples; ++i)
					{
						vec128 Src00 = pSrc0Samples[0];
						vec128 Src10 = pSrc1Samples[0];
						vec128 Src01 = pSrc0Samples[1];
						vec128 Src11 = pSrc1Samples[1];
						vec128 Src02 = pSrc0Samples[2];
						vec128 Src03 = pSrc0Samples[3];
						pDstSamples[0] = M_VAdd(Src00, Src10);
						pDstSamples[1] = M_VAdd(Src01, Src11);
						pDstSamples[2] = Src02;
						pDstSamples[3] = Src03;
						pDstSamples += 4;
						pSrc0Samples += 4;
						pSrc1Samples += Src1Stride;
					}
				}
				break;
			case 3:
				{
					for (uint32 i = 0; i < nSamples; ++i)
					{
						vec128 Src00 = pSrc0Samples[0];
						vec128 Src10 = pSrc1Samples[0];
						vec128 Src01 = pSrc0Samples[1];
						vec128 Src11 = pSrc1Samples[1];
						vec128 Src02 = pSrc0Samples[2];
						vec128 Src12 = pSrc1Samples[2];
						vec128 Src03 = pSrc0Samples[3];
						pDstSamples[0] = M_VAdd(Src00, Src10);
						pDstSamples[1] = M_VAdd(Src01, Src11);
						pDstSamples[2] = M_VAdd(Src02, Src12);
						pDstSamples[3] = Src03;
						pDstSamples += 4;
						pSrc0Samples += 4;
						pSrc1Samples += Src1Stride;
					}
				}
				break;
			}
		}
		break;
	}

	if (_bCanDestroy1 && pSrc1 == _pIn1)
	{
		_pMixer->ReturnMixBinContainer(_pIn1);
		_pIn1 = NULL;
	}

	if (pSrc1 == _pIn0)
	{
		_pMixer->ReturnMixBinContainer(_pIn0);
		_pIn0 = NULL;
	}
	return pRet;
}

CSC_Mixer_WorkerContext::CMixBinContainer *CSC_Mixer_DSP_Router::CopyContainer(CSC_Mixer_WorkerContext *_pMixer, CSC_Mixer_WorkerContext::CMixBinContainer *_pContainer)
{
	CSC_Mixer_WorkerContext::CMixBinContainer *pRet = _pMixer->GetMixBinContainer();
	pRet->m_nChannels = _pContainer->m_nChannels;
	uint32 nChannelVectors = (pRet->m_nChannels + 3) >> 2;
	CSC_Mixer_WorkerContext::CBixBinCointanerPtr ContainersPtrs[2];
	ContainersPtrs[0].m_bDiscard = true;
	ContainersPtrs[0].m_pPtr = 0;
	ContainersPtrs[0].m_pContainer = pRet;
	ContainersPtrs[1].m_bDiscard = false;
	ContainersPtrs[1].m_pPtr = 0;
	ContainersPtrs[1].m_pContainer = _pContainer;

	_pMixer->GetMixBinContainerPtrs(ContainersPtrs, 2);

	DataCopy(ContainersPtrs[0].m_pPtr, ContainersPtrs[1].m_pPtr, nChannelVectors * _pMixer->m_FrameLength);
	return pRet;
}

void CSC_Mixer_DSP_Router::PreProcessFrame(void *_pInternalData, const void *_pParams, const void *_pLastParams)
{
	const CParams *pParams = (const CParams *)_pParams;

	for (uint32 i = 0; i < EMaxRoutes; ++i)
	{
		CDSPChainInstanceInternal *pRouteTo = pParams->m_pRouteTo[i];
		if (pRouteTo)
		{
			CSC_Mixer_WorkerContext::CMixBinRef &MixRef = pRouteTo->m_MixBinRef[0];
			++MixRef.m_nToMix;
//				M_TRACEALWAYS("Preprocess %d: CSC_Mixer_DSP_Router\n", MixRef.m_nToMix);
			if (MixRef.m_nToMix == 1)
			{
				// Continue Processing
				CSC_Mixer_WorkerContext::ms_pThis->PreProcessDSPChainInstance(pRouteTo);
			}
		}
	}
}


void CSC_Mixer_DSP_Router::ProcessFrame(CSC_Mixer_ProcessingInfo *_pInfo, void *_pInternalData, const void *_pParams, void *_pLastParams)
{

//	M_TRACEALWAYS("CSC_Mixer_DSP_Router %f ", ((fp32 *)_pInfo->m_pDataIn)[0]);

	const CParams *pParams = (const CParams *)_pParams;
	CSC_Mixer_WorkerContext::CProcessingInfoInternal *pProcessingInfoInternal = (CSC_Mixer_WorkerContext::CProcessingInfoInternal *)_pInfo;
	uint32 nRoutes = 0;
	for (uint32 i = 0; i < EMaxRoutes; ++i)
	{
		CDSPChainInstanceInternal *pRouteTo = pParams->m_pRouteTo[i];
		if (pRouteTo)
		{
			++nRoutes;
		}
	}

	for (uint32 i = 0; i < EMaxRoutes; ++i)
	{
		CDSPChainInstanceInternal *pRouteTo = pParams->m_pRouteTo[i];
		if (pRouteTo)
		{
			--nRoutes;
			CSC_Mixer_WorkerContext::CMixBinRef &MixRef = pRouteTo->m_MixBinRef[0];
			--MixRef.m_nToMix;
//				M_TRACEALWAYS("Process %d: CSC_Mixer_DSP_Router\n", MixRef.m_nToMix);
			if (MixRef.m_nToMix < 0)
				M_BREAKPOINT;

			if (!MixRef.m_pBinContainer)
			{
				if (nRoutes == 0)
				{
					MixRef.m_pBinContainer = pProcessingInfoInternal->m_pContainerIn;
					pProcessingInfoInternal->m_pContainerIn = NULL;
				}
				else
					MixRef.m_pBinContainer = CopyContainer(CSC_Mixer_WorkerContext::ms_pThis, pProcessingInfoInternal->m_pContainerIn);
			}
			else
			{
				MixRef.m_pBinContainer = MixBuffer(CSC_Mixer_WorkerContext::ms_pThis, pRouteTo->m_MixBinRef[0].m_pBinContainer, pProcessingInfoInternal->m_pContainerIn, nRoutes == 0);
				pProcessingInfoInternal->m_pContainerIn = NULL;
			}

			// All have mixed into this buffer
			if (MixRef.m_nToMix == 0)
			{
				// Continue Processing
				CSC_Mixer_WorkerContext::CMixBinContainer *pContainer = pRouteTo->m_MixBinRef[0].m_pBinContainer;
				MixRef.m_pBinContainer = NULL;
				CSC_Mixer_WorkerContext::ms_pThis->ProcessDSPChainInstance(pRouteTo, pContainer);
			}
		}
	}
}
