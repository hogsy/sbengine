

bint CSC_Mixer_DSP_Volume::InitData(void *_pInternalData, void *_pParams, void *_pLastParams)
{
	CParams *pParams = (CParams *)_pParams;
	CLastParams *pLastParams = (CLastParams *)_pLastParams;
	pParams->m_Volume = 1.0f;
	pLastParams->m_Volume = 1.0f;
	return true;
}    

void CSC_Mixer_DSP_Volume::Create(void *_pInternalData, void *_pParams, void *_pLastParams)
{
	CParams *pParams = (CParams *)_pParams;
	CLastParams *pLastParams = (CLastParams *)_pLastParams;
	pLastParams->m_Volume = pParams->m_Volume;
}

