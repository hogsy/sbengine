

bint CSC_Mixer_DSP_SilenceGen::InitData(void *_pInternalData, void *_pParams, void *_pLastParams)
{
	CParams *pParams = (CParams *)_pParams;
	pParams->m_nChannels = 2;
	return true;
}    
