
void CSC_Mixer_DSP_SilenceGen::ProcessFrame(CSC_Mixer_ProcessingInfo *_pInfo, void *_pInternalData, const void *_pParams, void *_pLastParams)
{
//	M_TRACEALWAYS("CSC_Mixer_DSP_SilenceGen ");
	CParams *pParams = (CParams *)_pParams;

	_pInfo->m_nChannels = pParams->m_nChannels;
	uint32 nDstChannels = (pParams->m_nChannels + 3) >> 2;
	uint32 nLoops = _pInfo->m_nSamples;

	vec128 * M_RESTRICT pDst = (vec128 *)_pInfo->m_pDataOut;
	nLoops *= nDstChannels;
	for (uint32 i = 0; i < nLoops; i += 64)
	{
		M_ZERO128(0, pDst);
		M_ZERO128(128, pDst);
		M_ZERO128(256, pDst);
		M_ZERO128(384, pDst);
		M_ZERO128(512, pDst);
		M_ZERO128(512+128, pDst);
		M_ZERO128(512+256, pDst);
		M_ZERO128(512+384, pDst);
		pDst += 64;
	}
}
