

bint CSC_Mixer_DSP_Voice::InitData(void *_pInternalData, void *_pParams, void *_pLastParams)
{
	CParams *pParams = (CParams *)_pParams;
	CLastParams *pLastParams = (CLastParams *)_pLastParams;

	pParams->m_pSourceVoice = NULL;
	pParams->m_SingleVolume = 1.0f;

	// Start with 0 volume so looped sounds doesn't "pop"
	pLastParams->m_SingleVolume = 0.0f;
//	pLastParams->m_SingleVolumePrim = 0.0f;
	return true;
}    

