
void CSC_Mixer_DSP_Master::ProcessFrame(CSC_Mixer_ProcessingInfo *_pInfo, void *_pInternalData, const void *_pParams, void *_pLastParams)
{
	CParams *pParams = (CParams *)_pParams;
	M_ASSERT(_pInfo->m_nSamples == pParams->m_pOutputFrame->m_nSamples, "Samples missmatch");
	M_ASSERT((_pInfo->m_nSamples & 7) == 0, "Samples must me aligned on 8 samples");
//	M_TRACEALWAYS("CSC_Mixer_DSP_Master %f ", ((fp32 *)_pInfo->m_pDataIn)[0]);

	uint32 nDstChannels = (pParams->m_pOutputFrame->m_nChannels + 3) >> 2;
	uint32 nSrcChannels = (_pInfo->m_nChannels + 3) >> 2;
	uint32 nLoops = _pInfo->m_nSamples;
	vec128 * M_RESTRICT pSrc = (vec128 *)_pInfo->m_pDataIn;
#ifdef PLATFORM_DMA
	vec128 * M_RESTRICT pDst = (vec128 *)_pInfo->m_pDataIn;
#else
	vec128 * M_RESTRICT pDst = (vec128 *)pParams->m_pOutputFrame->m_pData;
#endif
	// Source and test the same ptr
	M_ASSERT(nSrcChannels >= nDstChannels, "The silence generator should take care of this");
#ifdef PLATFORM_DMA
	if (nSrcChannels > nDstChannels)
#endif
	{
		switch (nDstChannels)
		{
		case 1:
			{
				for (uint32 i = 0; i < nLoops; i += 8)
				{
					M_PREZERO128(0, pDst);
					pDst[0] = *pSrc;
					pSrc += nSrcChannels;
					pDst[1] = *pSrc;
					pSrc += nSrcChannels;
					pDst[2] = *pSrc;
					pSrc += nSrcChannels;
					pDst[3] = *pSrc;
					pSrc += nSrcChannels;
					pDst[4] = *pSrc;
					pSrc += nSrcChannels;
					pDst[5] = *pSrc;
					pSrc += nSrcChannels;
					pDst[6] = *pSrc;
					pSrc += nSrcChannels;
					pDst[7] = *pSrc;
					pSrc += nSrcChannels;
					pDst += 8;
				}
			}
			break;
		case 2:
			{
				for (uint32 i = 0; i < nLoops; i += 4)
				{
					M_PREZERO128(0, pDst);
					pDst[0] = pSrc[0];
					pDst[1] = pSrc[1];
					pDst += 2;
					pSrc += nSrcChannels;
					pDst[0] = pSrc[0];
					pDst[1] = pSrc[1];
					pDst += 2;
					pSrc += nSrcChannels;
					pDst[0] = pSrc[0];
					pDst[1] = pSrc[1];
					pDst += 2;
					pSrc += nSrcChannels;
					pDst[0] = pSrc[0];
					pDst[1] = pSrc[1];
					pDst += 2;
					pSrc += nSrcChannels;
				}
			}
			break;
		case 3:
			{
				for (uint32 i = 0; i < nLoops; i += 8)
				{
					M_PREZERO128(0, pDst);
					M_PREZERO128(128, pDst);
					M_PREZERO128(256, pDst);
					pDst[0] = pSrc[0];
					pDst[1] = pSrc[1];
					pDst[2] = pSrc[2];
					pDst += 3;
					pSrc += nSrcChannels;
					pDst[0] = pSrc[0];
					pDst[1] = pSrc[1];
					pDst[2] = pSrc[2];
					pDst += 3;
					pSrc += nSrcChannels;
					pDst[0] = pSrc[0];
					pDst[1] = pSrc[1];
					pDst[2] = pSrc[2];
					pDst += 3;
					pSrc += nSrcChannels;
					pDst[0] = pSrc[0];
					pDst[1] = pSrc[1];
					pDst[2] = pSrc[2];
					pDst += 3;
					pSrc += nSrcChannels;
					pDst[0] = pSrc[0];
					pDst[1] = pSrc[1];
					pDst[2] = pSrc[2];
					pDst += 3;
					pSrc += nSrcChannels;
					pDst[0] = pSrc[0];
					pDst[1] = pSrc[1];
					pDst[2] = pSrc[2];
					pDst += 3;
					pSrc += nSrcChannels;
					pDst[0] = pSrc[0];
					pDst[1] = pSrc[1];
					pDst[2] = pSrc[2];
					pDst += 3;
					pSrc += nSrcChannels;
					pDst[0] = pSrc[0];
					pDst[1] = pSrc[1];
					pDst[2] = pSrc[2];
					pDst += 3;
					pSrc += nSrcChannels;
				}
			}
			break;
		case 4:
			{
				for (uint32 i = 0; i < nLoops; i += 4)
				{
					M_PREZERO128(0, pDst);
					M_PREZERO128(128, pDst);
					pDst[0] = pSrc[0];
					pDst[1] = pSrc[1];
					pDst[2] = pSrc[2];
					pDst[3] = pSrc[3];
					pDst += 4;
					pSrc += nSrcChannels;
					pDst[0] = pSrc[0];
					pDst[1] = pSrc[1];
					pDst[2] = pSrc[2];
					pDst[3] = pSrc[3];
					pDst += 4;
					pSrc += nSrcChannels;
					pDst[0] = pSrc[0];
					pDst[1] = pSrc[1];
					pDst[2] = pSrc[2];
					pDst[3] = pSrc[3];
					pDst += 4;
					pSrc += nSrcChannels;
					pDst[0] = pSrc[0];
					pDst[1] = pSrc[1];
					pDst[2] = pSrc[2];
					pDst[3] = pSrc[3];
					pDst += 4;
					pSrc += nSrcChannels;
				}
			}
			break;
		}
	}
#ifdef PLATFORM_DMA
	mint OutputBufferSize = pParams->m_pOutputFrame->m_nChannels*pParams->m_pOutputFrame->m_nSamples * sizeof(fp32);
	MRTC_System_VPU::OS_DMATransferToSys((mint)pParams->m_pOutputFrame->m_pData, _pInfo->m_pDataIn, OutputBufferSize);
#endif
}
