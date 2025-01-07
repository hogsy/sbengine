



bint CSC_Mixer_DSP_VolumeMatrix::InitData(void *_pInternalData, void *_pParams, void *_pLastParams)
{
	CParams *pParams = (CParams *)_pParams;
	CLastParams *pLastParams = (CLastParams *)_pLastParams;
	pParams->m_nDestChannels = 1;
	pParams->m_nSourceChannels = 1;
	pParams->m_pVolumeMatrix = NULL;

	pLastParams->m_pVolumeMatrix = NULL;
	return true;
}    

void CSC_Mixer_DSP_VolumeMatrix::DestroyData(void *_pInternalData, void *_pParams, void *_pLastParams)
{
	CParams *pParams = (CParams *)_pParams;
	CLastParams *pLastParams = (CLastParams *)_pLastParams;
	if (pParams->m_pVolumeMatrix)
	{
		CSC_Mixer_WorkerContext::CCustomAllocator::Free(pParams->m_pVolumeMatrix);
		pParams->m_pVolumeMatrix = NULL;
	}
	if (pLastParams->m_pVolumeMatrix)
	{
		CSC_Mixer_WorkerContext::CCustomAllocator::Free(pLastParams->m_pVolumeMatrix);
		pLastParams->m_pVolumeMatrix = NULL;
	}
}    

void CSC_Mixer_DSP_VolumeMatrix::CopyData(void *_pParams, void *_pLastParams, void *_pInternal, void *_SourceData, void *_DestData)
{
	CParams *pParams = (CParams *)_pParams;
	CLastParams *pLastParams = (CLastParams *)_pLastParams;
	if (!pLastParams->m_pVolumeMatrix.m_PtrData.m_Offset)
		return;

	uint32 ToCopy = AlignUp(AlignUp(pParams->m_nSourceChannels, 4) * AlignUp(pParams->m_nDestChannels, 4) * 2, 16) >> 4;
	uint32 Offset = pLastParams->m_pVolumeMatrix.m_PtrData.m_Offset;
	vec128 *pDst = (vec128 *)(((mint)_DestData) + Offset);
	vec128 *pSrc = (vec128 *)(((mint)_SourceData) + Offset);
	DataCopy(pDst, pSrc, ToCopy);

}
