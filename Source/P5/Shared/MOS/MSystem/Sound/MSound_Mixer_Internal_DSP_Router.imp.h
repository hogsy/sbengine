
bint CSC_Mixer_DSP_Router::InitData(void *_pInternalData, void *_pParams, void *_pLastParams)
{
	CParams *pParams = (CParams *)_pParams;
	for (uint32 i = 0; i < EMaxRoutes; ++i)
	{
		pParams->m_pRouteTo[i] = NULL;
	}
	return true;
}    
