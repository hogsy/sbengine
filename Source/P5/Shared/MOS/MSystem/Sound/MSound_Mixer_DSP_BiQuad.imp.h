

bint CSC_Mixer_DSP_BiQuad::InitData(void *_pInternalData, void *_pParams, void *_pLastParams)
{
	CParams *pParams = (CParams *)_pParams;
	CLastParams *pLastParams = (CLastParams *)_pLastParams;
	pParams->m_Type = 0;
	pParams->m_Frequency = 5000.0f;
	pParams->m_Q = 1.0f;
	pParams->m_Gain = 1.0f;

	for (mint i = 0; i < ESCMixer_MaxChannelVectors; ++i)
	{
		pLastParams->m_HistIn_1[i] = M_VZero();
		pLastParams->m_HistIn_2[i] = M_VZero();
		pLastParams->m_HistOut_1[i] = M_VZero();
		pLastParams->m_HistOut_2[i] = M_VZero();
	}

	return true;
}    

void CSC_Mixer_DSP_BiQuad::Create(void *_pInternalData, void *_pParams, void *_pLastParams)
{
	CParams *pParams = (CParams *)_pParams;
	CLastParams *pLastParams = (CLastParams *)_pLastParams;

	CalcCoef(pParams->m_Type, pParams->m_Frequency, pParams->m_Q, pParams->m_Gain, CSC_Mixer_WorkerContext::ms_pThis->m_SampleRate, pLastParams->m_Coef);
}

