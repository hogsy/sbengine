
//#pragma optimize("", off)
//#pragma inline_depth(0)

bint CSC_Mixer_DSP_Reverb::InitData(void *_pInternalData, void *_pParams, void *_pLastParams)
{
	CParams *pParams = (CParams *)_pParams;
	CLastParams *pLastParams = (CLastParams *)_pLastParams;
	CInternal *pInternal = (CInternal *)_pInternalData;
	pParams->m_nDestinationChannels = 1;
	pParams->m_MaxDelay = 0.5f;
	// Dynamic params
	pParams->m_Wet = 1.0f;
	pParams->m_Gain = 0.015f;
	pParams->m_RoomSize = 0.5f;
	pParams->m_Damp = 0.5f;
	pParams->m_Width = 1.0f;
	pParams->m_Spread = 0.00052f;

	pInternal->m_nDestinationChannels = 1;
	pInternal->m_MaxDelay = 0.5f;
	pInternal->m_BufferSize = 0;
	pInternal->m_pDynamicMemory = 0;

	return true;
}    

void CSC_Mixer_DSP_Reverb::DestroyData(void *_pInternalData, void *_pParams, void *_pLastParams)
{
	CParams *pParams = (CParams *)_pParams;
	CLastParams *pLastParams = (CLastParams *)_pLastParams;
	CInternal *pInternal = (CInternal *)_pInternalData;
	if (pInternal->m_pDynamicMemory)
		MRTC_MemFree(pInternal->m_pDynamicMemory);
}    

void CSC_Mixer_DSP_Reverb::Create(void *_pInternalData, void *_pParams, void *_pLastParams)
{
	CParams *pParams = (CParams *)_pParams;
	CLastParams *pLastParams = (CLastParams *)_pLastParams;
	CInternal *pInternal = (CInternal *)_pInternalData;

	pInternal->m_MaxDelay = Clamp(pParams->m_MaxDelay, 0.01f, 0.036f);
	pInternal->m_nDestinationChannels = Clamp(pParams->m_nDestinationChannels, 1, uint32(EMaxChannels));
	fp32 SampleRate = CSC_Mixer_WorkerContext::ms_pThis->m_SampleRate;
	uint32 nMaxSamples = uint32(SampleRate * pInternal->m_MaxDelay);
	uint32 nChannels = pInternal->m_nDestinationChannels;
	mint nNeededBytes = 512*1024;
	uint32 BytesPerBuffer = AlignUp(sizeof(fp32) * nMaxSamples, 128);

	pInternal->m_pDynamicMemory = M_ALLOCALIGN(nNeededBytes, 128);
	uint8 *pBytes = (uint8 *)pInternal->m_pDynamicMemory;
	memset(pBytes, 0, nNeededBytes);

	pInternal->m_TestCombs[0].Init();
	pInternal->m_TestCombs[0].m_nSamples = 1226;
	pInternal->m_TestCombs[0].m_pBuffer = (fp32 *)pBytes; pBytes += 1226 * sizeof(fp32);

	pInternal->m_TestCombs[1].Init();
	pInternal->m_TestCombs[1].m_nSamples = 1337;
	pInternal->m_TestCombs[1].m_pBuffer = (fp32 *)pBytes; pBytes += 1337 * sizeof(fp32);

	pInternal->m_TestCombs[2].Init();
	pInternal->m_TestCombs[2].m_nSamples = 1466;
	pInternal->m_TestCombs[2].m_pBuffer = (fp32 *)pBytes; pBytes += 1466 * sizeof(fp32);

	pInternal->m_TestDelayAllPass.Init();
	pInternal->m_TestDelayAllPass.m_nSamples = 1024;
	pInternal->m_TestDelayAllPass.m_pBuffer = (fp32 *)pBytes; pBytes += 1024 * sizeof(fp32);

/*
	nNeededBytes += ENumCombs * nChannels * BytesPerBuffer;
	nNeededBytes += ENumAllpass * nChannels * BytesPerBuffer;
	pInternal->m_pDynamicMemory = M_ALLOCALIGN(nNeededBytes, 128);
	uint8 *pBytes = (uint8 *)pInternal->m_pDynamicMemory;
	memset(pBytes, 0, nNeededBytes);

	for (mint i = 0; i < ENumCombs; ++i)
	{
		for (mint j = 0; j < nChannels; ++j)
		{
			pInternal->m_Combs[i][j].m_pBuffer = (fp32 *)pBytes;
			pBytes += BytesPerBuffer;
		}
	}
	for (mint i = 0; i < ENumAllpass; ++i)
	{
		for (mint j = 0; j < nChannels; ++j)
		{
			pInternal->m_AllPass[i][j].m_pBuffer = (fp32 *)pBytes;
			pBytes += BytesPerBuffer;
		}
	}
*/
}
