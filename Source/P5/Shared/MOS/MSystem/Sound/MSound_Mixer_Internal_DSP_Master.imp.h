
bint CSC_Mixer_DSP_Master::InitData(void *_pInternalData, void *_pParams, void *_pLastParams)
{
	CParams *pParams = (CParams *)_pParams;
	pParams->m_pOutputFrame = NULL;
	return true;
}    
