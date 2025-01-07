
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// m_pVolumeMatrix is uint16
// 1

void CSC_Mixer_DSP_VolumeMatrix::ProcessFrame(CSC_Mixer_ProcessingInfo *_pInfo, void *_pInternalData, const void *_pParams, void *_pLastParams)
{

//	M_TRACEALWAYS("CSC_Mixer_DSP_VolumeMatrix %f ", ((fp32 *)_pInfo->m_pDataIn)[0]);

	CParams *pParams = (CParams *)_pParams;
	CLastParams *pLastParams = (CLastParams *)_pLastParams;

	uint32 nDstChannels = pParams->m_nDestChannels;
	uint32 nSrcChannels = _pInfo->m_nChannels;
	_pInfo->m_nChannels = nDstChannels;
	uint32 nLoops = _pInfo->m_nSamples;

	if (nSrcChannels != pParams->m_nSourceChannels)
		M_BREAKPOINT;

	vec128 * M_RESTRICT pSrc = (vec128 *)_pInfo->m_pDataIn;
	vec128 * M_RESTRICT pDst = (vec128 *)_pInfo->m_pDataOut;
	vec128 * pVolumes = pParams->m_pVolumeMatrix;
	vec128 * pLastVolumes = pLastParams->m_pVolumeMatrix;
	if (!pVolumes)
	{
		_pInfo->m_nChannels = nSrcChannels;
		DataCopy(_pInfo->m_pDataOut, _pInfo->m_pDataOut, (AlignUp(nSrcChannels, 4) * _pInfo->m_nSamples) >> 2);
		return;
	}
	if (!pLastVolumes)
	{
		pLastVolumes = pVolumes;
	}

	vec128 MulInt16 = M_VConst(1.0f/65535.0f, 1.0f/65535.0f, 1.0f/65535.0f, 1.0f/65535.0f);

	vec128 SamplesInv = M_VRcp(M_VCnv_i32_f32(M_VLdScalar_u32(_pInfo->m_nSamples)));

	uint32 nDstVec = (nDstChannels + 3) >> 2;
	uint32 nSrcVec = (nSrcChannels + 3) >> 2;

	M_PRECACHE128( 0, pSrc); // Precache
	M_PRECACHE128( 128, pSrc); // Precache
	M_PRECACHE128( 256, pSrc); // Precache
	M_PRECACHE128( 384, pSrc); // Precache
	uint32 DoPrezero = 0;
	if (nSrcVec != 1)
	{
		M_PREZERO128( 0, pDst); // Precache
	}


	switch (nSrcVec)
	{
	case 1:
		{
			switch (nDstVec)
			{
			case 1:
				{
					vec128 * M_RESTRICT pVol = pVolumes;
					vec128 * M_RESTRICT pLastVol = pLastVolumes;

					vec128 VolS00 = M_VLd(pLastVol);
					vec128 VolS01 = M_VLd(pLastVol+1);

					vec128 NVolS00 = M_VLd(pVol);
					vec128 NVolS01 = M_VLd(pVol+1);

					pLastVol[0] = NVolS00;
					pLastVol[1] = NVolS01;

					vec128 Vol00 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS00)), MulInt16);
					vec128 Vol01 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS00)), MulInt16);
					vec128 Vol02 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS01)), MulInt16);
					vec128 Vol03 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS01)), MulInt16);

					vec128 DVol00 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(NVolS00)), MulInt16), Vol00), SamplesInv);
					vec128 DVol01 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(NVolS00)), MulInt16), Vol01), SamplesInv);
					vec128 DVol02 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(NVolS01)), MulInt16), Vol02), SamplesInv);
					vec128 DVol03 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(NVolS01)), MulInt16), Vol03), SamplesInv);

					for (uint32 i = 0; i < nLoops; i+=8)
					{
						M_PRECACHE128( 512, pSrc); // Precache
						M_PREZERO128( 0, pDst); // Zero out cacheline

						vec128 Src0 = M_VLd(pSrc);
						vec128 Src1 = M_VLd(pSrc+1);
						vec128 Src2 = M_VLd(pSrc+2);
						vec128 Src3 = M_VLd(pSrc+3);
						vec128 Src4 = M_VLd(pSrc+4);
						vec128 Src5 = M_VLd(pSrc+5);
						vec128 Src6 = M_VLd(pSrc+6);
						vec128 Src7 = M_VLd(pSrc+7);

						pDst[0] = M_VDp4x4(Src0, Vol00, Src0, Vol01, Src0, Vol02, Src0, Vol03);
						Vol00 = M_VAdd(Vol00, DVol00);
						Vol01 = M_VAdd(Vol01, DVol01);
						Vol02 = M_VAdd(Vol02, DVol02);
						Vol03 = M_VAdd(Vol03, DVol03);


						pDst[1] = M_VDp4x4(Src1, Vol00, Src1, Vol01, Src1, Vol02, Src1, Vol03);
						Vol00 = M_VAdd(Vol00, DVol00);
						Vol01 = M_VAdd(Vol01, DVol01);
						Vol02 = M_VAdd(Vol02, DVol02);
						Vol03 = M_VAdd(Vol03, DVol03);

						pDst[2] = M_VDp4x4(Src2, Vol00, Src2, Vol01, Src2, Vol02, Src2, Vol03);
						Vol00 = M_VAdd(Vol00, DVol00);
						Vol01 = M_VAdd(Vol01, DVol01);
						Vol02 = M_VAdd(Vol02, DVol02);
						Vol03 = M_VAdd(Vol03, DVol03);

						pDst[3] = M_VDp4x4(Src3, Vol00, Src3, Vol01, Src3, Vol02, Src3, Vol03);
						Vol00 = M_VAdd(Vol00, DVol00);
						Vol01 = M_VAdd(Vol01, DVol01);
						Vol02 = M_VAdd(Vol02, DVol02);
						Vol03 = M_VAdd(Vol03, DVol03);

						pDst[4] = M_VDp4x4(Src4, Vol00, Src4, Vol01, Src4, Vol02, Src4, Vol03);
						Vol00 = M_VAdd(Vol00, DVol00);
						Vol01 = M_VAdd(Vol01, DVol01);
						Vol02 = M_VAdd(Vol02, DVol02);
						Vol03 = M_VAdd(Vol03, DVol03);

						pDst[5] = M_VDp4x4(Src5, Vol00, Src5, Vol01, Src5, Vol02, Src5, Vol03);
						Vol00 = M_VAdd(Vol00, DVol00);
						Vol01 = M_VAdd(Vol01, DVol01);
						Vol02 = M_VAdd(Vol02, DVol02);
						Vol03 = M_VAdd(Vol03, DVol03);

						pDst[6] = M_VDp4x4(Src6, Vol00, Src6, Vol01, Src6, Vol02, Src6, Vol03);
						Vol00 = M_VAdd(Vol00, DVol00);
						Vol01 = M_VAdd(Vol01, DVol01);
						Vol02 = M_VAdd(Vol02, DVol02);
						Vol03 = M_VAdd(Vol03, DVol03);

						pDst[7] = M_VDp4x4(Src7, Vol00, Src7, Vol01, Src7, Vol02, Src7, Vol03);
						Vol00 = M_VAdd(Vol00, DVol00);
						Vol01 = M_VAdd(Vol01, DVol01);
						Vol02 = M_VAdd(Vol02, DVol02);
						Vol03 = M_VAdd(Vol03, DVol03);

						pDst += 8;
						pSrc += 8;
					}
				}
				break;
			case 2:
				{
					vec128 * M_RESTRICT pVol = pVolumes;
					vec128 * M_RESTRICT pLastVol = pLastVolumes;


					vec128 VolS00 = M_VLd(pLastVol);
					vec128 VolS01 = M_VLd(pLastVol+1);
					vec128 VolS10 = M_VLd(pLastVol+2);
					vec128 VolS11 = M_VLd(pLastVol+3);

					vec128 NVolS00 = M_VLd(pVol);
					vec128 NVolS01 = M_VLd(pVol+1);
					vec128 NVolS10 = M_VLd(pVol+2);
					vec128 NVolS11 = M_VLd(pVol+3);

					pLastVol[0] = NVolS00;
					pLastVol[1] = NVolS01;
					pLastVol[2] = NVolS10;
					pLastVol[3] = NVolS11;

					vec128 Vol00 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS00)), MulInt16);
					vec128 Vol01 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS00)), MulInt16);
					vec128 Vol02 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS01)), MulInt16);
					vec128 Vol03 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS01)), MulInt16);

					vec128 Vol10 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS10)), MulInt16);
					vec128 Vol11 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS10)), MulInt16);
					vec128 Vol12 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS11)), MulInt16);
					vec128 Vol13 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS11)), MulInt16);

					vec128 DVol00 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(NVolS00)), MulInt16), Vol00), SamplesInv);
					vec128 DVol01 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(NVolS00)), MulInt16), Vol01), SamplesInv);
					vec128 DVol02 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(NVolS01)), MulInt16), Vol02), SamplesInv);
					vec128 DVol03 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(NVolS01)), MulInt16), Vol03), SamplesInv);

					vec128 DVol10 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(NVolS10)), MulInt16), Vol10), SamplesInv);
					vec128 DVol11 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(NVolS10)), MulInt16), Vol11), SamplesInv);
					vec128 DVol12 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(NVolS11)), MulInt16), Vol12), SamplesInv);
					vec128 DVol13 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(NVolS11)), MulInt16), Vol13), SamplesInv);

					for (uint32 i = 0; i < nLoops; i+=8)
					{
						M_PRECACHE128( 512, pSrc); // Precache

						for (uint32 i = 0; i < 2; ++i)
						{
							vec128 Src0 = M_VLd(pSrc);
							vec128 Src1 = M_VLd(pSrc+1);
							vec128 Src2 = M_VLd(pSrc+2);
							vec128 Src3 = M_VLd(pSrc+3);
							M_PREZERO128( 0, pDst); // Zero out cacheline

							pDst[0] = M_VDp4x4(Src0, Vol00, Src0, Vol01, Src0, Vol02, Src0, Vol03);
							pDst[1] = M_VDp4x4(Src0, Vol10, Src0, Vol11, Src0, Vol12, Src0, Vol13);
							Vol00 = M_VAdd(Vol00, DVol00);
							Vol01 = M_VAdd(Vol01, DVol01);
							Vol02 = M_VAdd(Vol02, DVol02);
							Vol03 = M_VAdd(Vol03, DVol03);
							Vol10 = M_VAdd(Vol10, DVol10);
							Vol11 = M_VAdd(Vol11, DVol11);
							Vol12 = M_VAdd(Vol12, DVol12);
							Vol13 = M_VAdd(Vol13, DVol13);

							pDst[2] = M_VDp4x4(Src1, Vol00, Src1, Vol01, Src1, Vol02, Src1, Vol03);
							pDst[3] = M_VDp4x4(Src1, Vol10, Src1, Vol11, Src1, Vol12, Src1, Vol13);
							Vol00 = M_VAdd(Vol00, DVol00);
							Vol01 = M_VAdd(Vol01, DVol01);
							Vol02 = M_VAdd(Vol02, DVol02);
							Vol03 = M_VAdd(Vol03, DVol03);
							Vol10 = M_VAdd(Vol10, DVol10);
							Vol11 = M_VAdd(Vol11, DVol11);
							Vol12 = M_VAdd(Vol12, DVol12);
							Vol13 = M_VAdd(Vol13, DVol13);

							pDst[4] = M_VDp4x4(Src2, Vol00, Src2, Vol01, Src2, Vol02, Src2, Vol03);
							pDst[5] = M_VDp4x4(Src2, Vol10, Src2, Vol11, Src2, Vol12, Src2, Vol13);
							Vol00 = M_VAdd(Vol00, DVol00);
							Vol01 = M_VAdd(Vol01, DVol01);
							Vol02 = M_VAdd(Vol02, DVol02);
							Vol03 = M_VAdd(Vol03, DVol03);
							Vol10 = M_VAdd(Vol10, DVol10);
							Vol11 = M_VAdd(Vol11, DVol11);
							Vol12 = M_VAdd(Vol12, DVol12);
							Vol13 = M_VAdd(Vol13, DVol13);

							pDst[6] = M_VDp4x4(Src3, Vol00, Src3, Vol01, Src3, Vol02, Src3, Vol03);
							pDst[7] = M_VDp4x4(Src3, Vol10, Src3, Vol11, Src3, Vol12, Src3, Vol13);
							Vol00 = M_VAdd(Vol00, DVol00);
							Vol01 = M_VAdd(Vol01, DVol01);
							Vol02 = M_VAdd(Vol02, DVol02);
							Vol03 = M_VAdd(Vol03, DVol03);
							Vol10 = M_VAdd(Vol10, DVol10);
							Vol11 = M_VAdd(Vol11, DVol11);
							Vol12 = M_VAdd(Vol12, DVol12);
							Vol13 = M_VAdd(Vol13, DVol13);


							pDst += 8;
							pSrc += 4;
						}
					}
				}
				break;
			case 3:
				{
					vec128 * M_RESTRICT pVol = pVolumes;
					vec128 * M_RESTRICT pLastVol = pLastVolumes;
					vec128 VolS00 = M_VLd(pLastVol);
					vec128 VolS01 = M_VLd(pLastVol+1);
					vec128 VolS10 = M_VLd(pLastVol+2);
					vec128 VolS11 = M_VLd(pLastVol+3);
					vec128 VolS20 = M_VLd(pLastVol+4);
					vec128 VolS21 = M_VLd(pLastVol+5);

					vec128 NVolS00 = M_VLd(pVol);
					vec128 NVolS01 = M_VLd(pVol+1);
					vec128 NVolS10 = M_VLd(pVol+2);
					vec128 NVolS11 = M_VLd(pVol+3);
					vec128 NVolS20 = M_VLd(pVol+4);
					vec128 NVolS21 = M_VLd(pVol+5);

					pLastVol[0] = NVolS00;
					pLastVol[1] = NVolS01;
					pLastVol[2] = NVolS10;
					pLastVol[3] = NVolS11;
					pLastVol[4] = NVolS20;
					pLastVol[5] = NVolS21;

					vec128 Vol00 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS00)), MulInt16);
					vec128 Vol01 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS00)), MulInt16);
					vec128 Vol02 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS01)), MulInt16);
					vec128 Vol03 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS01)), MulInt16);

					vec128 Vol10 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS10)), MulInt16);
					vec128 Vol11 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS10)), MulInt16);
					vec128 Vol12 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS11)), MulInt16);
					vec128 Vol13 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS11)), MulInt16);

					vec128 Vol20 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS20)), MulInt16);
					vec128 Vol21 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS20)), MulInt16);
					vec128 Vol22 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS21)), MulInt16);
					vec128 Vol23 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS21)), MulInt16);

					vec128 DVol00 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(NVolS00)), MulInt16), Vol00), SamplesInv);
					vec128 DVol01 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(NVolS00)), MulInt16), Vol01), SamplesInv);
					vec128 DVol02 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(NVolS01)), MulInt16), Vol02), SamplesInv);
					vec128 DVol03 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(NVolS01)), MulInt16), Vol03), SamplesInv);

					vec128 DVol10 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(NVolS10)), MulInt16), Vol10), SamplesInv);
					vec128 DVol11 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(NVolS10)), MulInt16), Vol11), SamplesInv);
					vec128 DVol12 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(NVolS11)), MulInt16), Vol12), SamplesInv);
					vec128 DVol13 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(NVolS11)), MulInt16), Vol13), SamplesInv);

					vec128 DVol20 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(NVolS20)), MulInt16), Vol20), SamplesInv);
					vec128 DVol21 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(NVolS20)), MulInt16), Vol21), SamplesInv);
					vec128 DVol22 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(NVolS21)), MulInt16), Vol22), SamplesInv);
					vec128 DVol23 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(NVolS21)), MulInt16), Vol23), SamplesInv);

					for (uint32 i = 0; i < nLoops; i+=8)
					{
						M_PRECACHE128( 512, pSrc); // Precache
						M_PREZERO128( 0, pDst); // Zero out cacheline
						M_PREZERO128( 128, pDst); // Zero out cacheline
						M_PREZERO128( 256, pDst); // Zero out cacheline

						vec128 Src0 = M_VLd(pSrc);
						vec128 Src1 = M_VLd(pSrc+1);
						vec128 Src2 = M_VLd(pSrc+2);
						vec128 Src3 = M_VLd(pSrc+3);
						vec128 Src4 = M_VLd(pSrc+4);
						vec128 Src5 = M_VLd(pSrc+5);
						vec128 Src6 = M_VLd(pSrc+6);
						vec128 Src7 = M_VLd(pSrc+7);

						pDst[0 ] = M_VDp4x4(Src0, Vol00, Src0, Vol01, Src0, Vol02, Src0, Vol03);
						pDst[1 ] = M_VDp4x4(Src0, Vol10, Src0, Vol11, Src0, Vol12, Src0, Vol13);
						pDst[2 ] = M_VDp4x4(Src0, Vol20, Src0, Vol21, Src0, Vol22, Src0, Vol23);
						Vol00 = M_VAdd(Vol00, DVol00);
						Vol01 = M_VAdd(Vol01, DVol01);
						Vol02 = M_VAdd(Vol02, DVol02);
						Vol03 = M_VAdd(Vol03, DVol03);
						Vol10 = M_VAdd(Vol10, DVol10);
						Vol11 = M_VAdd(Vol11, DVol11);
						Vol12 = M_VAdd(Vol12, DVol12);
						Vol13 = M_VAdd(Vol13, DVol13);
						Vol20 = M_VAdd(Vol20, DVol20);
						Vol21 = M_VAdd(Vol21, DVol21);
						Vol22 = M_VAdd(Vol22, DVol22);
						Vol23 = M_VAdd(Vol23, DVol23);

						pDst[3 ] = M_VDp4x4(Src1, Vol00, Src1, Vol01, Src1, Vol02, Src1, Vol03);
						pDst[4 ] = M_VDp4x4(Src1, Vol10, Src1, Vol11, Src1, Vol12, Src1, Vol13);
						pDst[5 ] = M_VDp4x4(Src1, Vol20, Src1, Vol21, Src1, Vol22, Src1, Vol23);
						Vol00 = M_VAdd(Vol00, DVol00);
						Vol01 = M_VAdd(Vol01, DVol01);
						Vol02 = M_VAdd(Vol02, DVol02);
						Vol03 = M_VAdd(Vol03, DVol03);
						Vol10 = M_VAdd(Vol10, DVol10);
						Vol11 = M_VAdd(Vol11, DVol11);
						Vol12 = M_VAdd(Vol12, DVol12);
						Vol13 = M_VAdd(Vol13, DVol13);
						Vol20 = M_VAdd(Vol20, DVol20);
						Vol21 = M_VAdd(Vol21, DVol21);
						Vol22 = M_VAdd(Vol22, DVol22);
						Vol23 = M_VAdd(Vol23, DVol23);

						pDst[6 ] = M_VDp4x4(Src2, Vol00, Src2, Vol01, Src2, Vol02, Src2, Vol03);
						pDst[7 ] = M_VDp4x4(Src2, Vol10, Src2, Vol11, Src2, Vol12, Src2, Vol13);
						pDst[8 ] = M_VDp4x4(Src2, Vol20, Src2, Vol21, Src2, Vol22, Src2, Vol23);
						pDst[9 ] = M_VDp4x4(Src3, Vol00, Src3, Vol01, Src3, Vol02, Src3, Vol03);
						pDst[10] = M_VDp4x4(Src3, Vol10, Src3, Vol11, Src3, Vol12, Src3, Vol13);
						pDst[11] = M_VDp4x4(Src3, Vol20, Src3, Vol21, Src3, Vol22, Src3, Vol23);
						Vol00 = M_VAdd(Vol00, DVol00);
						Vol01 = M_VAdd(Vol01, DVol01);
						Vol02 = M_VAdd(Vol02, DVol02);
						Vol03 = M_VAdd(Vol03, DVol03);
						Vol10 = M_VAdd(Vol10, DVol10);
						Vol11 = M_VAdd(Vol11, DVol11);
						Vol12 = M_VAdd(Vol12, DVol12);
						Vol13 = M_VAdd(Vol13, DVol13);
						Vol20 = M_VAdd(Vol20, DVol20);
						Vol21 = M_VAdd(Vol21, DVol21);
						Vol22 = M_VAdd(Vol22, DVol22);
						Vol23 = M_VAdd(Vol23, DVol23);

						pDst[12] = M_VDp4x4(Src4, Vol00, Src4, Vol01, Src4, Vol02, Src4, Vol03);
						pDst[13] = M_VDp4x4(Src4, Vol10, Src4, Vol11, Src4, Vol12, Src4, Vol13);
						pDst[14] = M_VDp4x4(Src4, Vol20, Src4, Vol21, Src4, Vol22, Src4, Vol23);
						pDst[15] = M_VDp4x4(Src5, Vol00, Src5, Vol01, Src5, Vol02, Src5, Vol03);
						pDst[16] = M_VDp4x4(Src5, Vol10, Src5, Vol11, Src5, Vol12, Src5, Vol13);
						pDst[17] = M_VDp4x4(Src5, Vol20, Src5, Vol21, Src5, Vol22, Src5, Vol23);
						Vol00 = M_VAdd(Vol00, DVol00);
						Vol01 = M_VAdd(Vol01, DVol01);
						Vol02 = M_VAdd(Vol02, DVol02);
						Vol03 = M_VAdd(Vol03, DVol03);
						Vol10 = M_VAdd(Vol10, DVol10);
						Vol11 = M_VAdd(Vol11, DVol11);
						Vol12 = M_VAdd(Vol12, DVol12);
						Vol13 = M_VAdd(Vol13, DVol13);
						Vol20 = M_VAdd(Vol20, DVol20);
						Vol21 = M_VAdd(Vol21, DVol21);
						Vol22 = M_VAdd(Vol22, DVol22);
						Vol23 = M_VAdd(Vol23, DVol23);

						pDst[18] = M_VDp4x4(Src6, Vol00, Src6, Vol01, Src6, Vol02, Src6, Vol03);
						pDst[19] = M_VDp4x4(Src6, Vol10, Src6, Vol11, Src6, Vol12, Src6, Vol13);
						pDst[20] = M_VDp4x4(Src6, Vol20, Src6, Vol21, Src6, Vol22, Src6, Vol23);
						pDst[21] = M_VDp4x4(Src7, Vol00, Src7, Vol01, Src7, Vol02, Src7, Vol03);
						pDst[22] = M_VDp4x4(Src7, Vol10, Src7, Vol11, Src7, Vol12, Src7, Vol13);
						pDst[23] = M_VDp4x4(Src7, Vol20, Src7, Vol21, Src7, Vol22, Src7, Vol23);
						Vol00 = M_VAdd(Vol00, DVol00);
						Vol01 = M_VAdd(Vol01, DVol01);
						Vol02 = M_VAdd(Vol02, DVol02);
						Vol03 = M_VAdd(Vol03, DVol03);
						Vol10 = M_VAdd(Vol10, DVol10);
						Vol11 = M_VAdd(Vol11, DVol11);
						Vol12 = M_VAdd(Vol12, DVol12);
						Vol13 = M_VAdd(Vol13, DVol13);
						Vol20 = M_VAdd(Vol20, DVol20);
						Vol21 = M_VAdd(Vol21, DVol21);
						Vol22 = M_VAdd(Vol22, DVol22);
						Vol23 = M_VAdd(Vol23, DVol23);

						pDst += 24;
						pSrc += 8;
					}
				}
				break;
			case 4:
				{
					vec128 * M_RESTRICT pVol = pVolumes;
					vec128 * M_RESTRICT pLastVol = pLastVolumes;
					vec128 VolS00 = M_VLd(pLastVol);
					vec128 VolS01 = M_VLd(pLastVol+1);
					vec128 VolS10 = M_VLd(pLastVol+2);
					vec128 VolS11 = M_VLd(pLastVol+3);
					vec128 VolS20 = M_VLd(pLastVol+4);
					vec128 VolS21 = M_VLd(pLastVol+5);
					vec128 VolS30 = M_VLd(pLastVol+6);
					vec128 VolS31 = M_VLd(pLastVol+7);

					vec128 NVolS00 = M_VLd(pVol);
					vec128 NVolS01 = M_VLd(pVol+1);
					vec128 NVolS10 = M_VLd(pVol+2);
					vec128 NVolS11 = M_VLd(pVol+3);
					vec128 NVolS20 = M_VLd(pVol+4);
					vec128 NVolS21 = M_VLd(pVol+5);
					vec128 NVolS30 = M_VLd(pVol+6);
					vec128 NVolS31 = M_VLd(pVol+7);

					pLastVol[0] = NVolS00;
					pLastVol[1] = NVolS01;
					pLastVol[2] = NVolS10;
					pLastVol[3] = NVolS11;
					pLastVol[4] = NVolS20;
					pLastVol[5] = NVolS21;
					pLastVol[6] = NVolS30;
					pLastVol[7] = NVolS31;

					vec128 Vol00 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS00)), MulInt16);
					vec128 Vol01 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS00)), MulInt16);
					vec128 Vol02 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS01)), MulInt16);
					vec128 Vol03 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS01)), MulInt16);

					vec128 Vol10 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS10)), MulInt16);
					vec128 Vol11 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS10)), MulInt16);
					vec128 Vol12 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS11)), MulInt16);
					vec128 Vol13 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS11)), MulInt16);

					vec128 Vol20 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS20)), MulInt16);
					vec128 Vol21 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS20)), MulInt16);
					vec128 Vol22 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS21)), MulInt16);
					vec128 Vol23 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS21)), MulInt16);

					vec128 Vol30 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS30)), MulInt16);
					vec128 Vol31 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS30)), MulInt16);
					vec128 Vol32 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS31)), MulInt16);
					vec128 Vol33 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS31)), MulInt16);

					vec128 DVol00 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(NVolS00)), MulInt16), Vol00), SamplesInv);
					vec128 DVol01 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(NVolS00)), MulInt16), Vol01), SamplesInv);
					vec128 DVol02 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(NVolS01)), MulInt16), Vol02), SamplesInv);
					vec128 DVol03 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(NVolS01)), MulInt16), Vol03), SamplesInv);

					vec128 DVol10 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(NVolS10)), MulInt16), Vol10), SamplesInv);
					vec128 DVol11 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(NVolS10)), MulInt16), Vol11), SamplesInv);
					vec128 DVol12 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(NVolS11)), MulInt16), Vol12), SamplesInv);
					vec128 DVol13 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(NVolS11)), MulInt16), Vol13), SamplesInv);

					vec128 DVol20 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(NVolS20)), MulInt16), Vol20), SamplesInv);
					vec128 DVol21 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(NVolS20)), MulInt16), Vol21), SamplesInv);
					vec128 DVol22 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(NVolS21)), MulInt16), Vol22), SamplesInv);
					vec128 DVol23 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(NVolS21)), MulInt16), Vol23), SamplesInv);

					vec128 DVol30 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(NVolS30)), MulInt16), Vol30), SamplesInv);
					vec128 DVol31 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(NVolS30)), MulInt16), Vol31), SamplesInv);
					vec128 DVol32 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(NVolS31)), MulInt16), Vol32), SamplesInv);
					vec128 DVol33 = M_VMul(M_VSub(M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(NVolS31)), MulInt16), Vol33), SamplesInv);

					for (uint32 i = 0; i < nLoops; i+=8)
					{
						M_PRECACHE128( 512, pSrc); // Precache

						for (uint32 i = 0; i < 4; ++i)
						{
							vec128 Src0 = M_VLd(pSrc);
							vec128 Src1 = M_VLd(pSrc+1);
							M_PREZERO128( 0, pDst); // Zero out cacheline

							pDst[0] = M_VDp4x4(Src0, Vol00, Src0, Vol01, Src0, Vol02, Src0, Vol03);
							pDst[1] = M_VDp4x4(Src0, Vol10, Src0, Vol11, Src0, Vol12, Src0, Vol13);
							pDst[2] = M_VDp4x4(Src0, Vol20, Src0, Vol21, Src0, Vol22, Src0, Vol23);
							pDst[3] = M_VDp4x4(Src0, Vol30, Src0, Vol31, Src0, Vol32, Src0, Vol33);

							pDst[4] = M_VDp4x4(Src1, Vol00, Src1, Vol01, Src1, Vol02, Src1, Vol03);
							pDst[5] = M_VDp4x4(Src1, Vol10, Src1, Vol11, Src1, Vol12, Src1, Vol13);
							pDst[6] = M_VDp4x4(Src1, Vol20, Src1, Vol21, Src1, Vol22, Src1, Vol23);
							pDst[7] = M_VDp4x4(Src1, Vol30, Src1, Vol31, Src1, Vol32, Src1, Vol33);

							Vol00 = M_VAdd(Vol00, DVol00);
							Vol01 = M_VAdd(Vol01, DVol01);
							Vol02 = M_VAdd(Vol02, DVol02);
							Vol03 = M_VAdd(Vol03, DVol03);
							Vol10 = M_VAdd(Vol10, DVol10);
							Vol11 = M_VAdd(Vol11, DVol11);
							Vol12 = M_VAdd(Vol12, DVol12);
							Vol13 = M_VAdd(Vol13, DVol13);
							Vol20 = M_VAdd(Vol20, DVol20);
							Vol21 = M_VAdd(Vol21, DVol21);
							Vol22 = M_VAdd(Vol22, DVol22);
							Vol23 = M_VAdd(Vol23, DVol23);
							Vol30 = M_VAdd(Vol30, DVol30);
							Vol31 = M_VAdd(Vol31, DVol31);
							Vol32 = M_VAdd(Vol32, DVol32);
							Vol33 = M_VAdd(Vol33, DVol33);

							pDst += 8;
							pSrc += 2;
						}
					}
				}
				break;
			}
			
		}
		break;
		// No support for volume interpolation for these onward
	case 2:
		{
			for (uint32 i = 0; i < nLoops; i+=8)
			{
				M_PRECACHE128( 512, pSrc); // Precache

				for (uint32 i = 0; i < 4; ++i)
				{
					vec128 Src0 = M_VLd(pSrc);
					vec128 Src1 = M_VLd(pSrc + 1);

					vec128 * M_RESTRICT pVol = pVolumes;
					// Loop in the interest of code size
					for (uint32 i = 0; i < nDstVec; ++i)
					{
						if (DoPrezero == 8)
						{
							M_PREZERO128( 0, pDst); // Precache
							DoPrezero = 0;
						}
						++DoPrezero;
						vec128 VolS0 = M_VLd(pVol); 
						vec128 VolS1 = M_VLd(pVol+1); 
						vec128 VolS2 = M_VLd(pVol+2); 
						vec128 VolS3 = M_VLd(pVol+3); 
						vec128 Vol0 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS0)), MulInt16);
						vec128 Vol1 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS0)), MulInt16);
						vec128 Vol2 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS1)), MulInt16);
						vec128 Vol3 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS1)), MulInt16);
						vec128 Vol4 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS2)), MulInt16);
						vec128 Vol5 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS2)), MulInt16);
						vec128 Vol6 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS3)), MulInt16);
						vec128 Vol7 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS3)), MulInt16);
						vec128 Temp0 = M_VAdd(M_VDp4(Src0, Vol0), M_VDp4(Src1, Vol1));
						vec128 Temp1 = M_VAdd(M_VDp4(Src0, Vol2), M_VDp4(Src1, Vol3));
						vec128 Temp2 = M_VAdd(M_VDp4(Src0, Vol4), M_VDp4(Src1, Vol5));
						vec128 Temp3 = M_VAdd(M_VDp4(Src0, Vol6), M_VDp4(Src1, Vol7));
						*pDst = M_VScalarToPacked(Temp0, Temp1, Temp2, Temp3);
						++pDst;
						pVol += 4;
					}

					pSrc += 2;
				}
			}
		}
		break;
	case 3:
		{

			mint LastPrecache = (mint)pSrc + 384;
			for (uint32 i = 0; i < nLoops; i+=8)
			{
				mint Now = AlignDown((mint)pSrc, 128);
				if (LastPrecache != Now)
				{
					LastPrecache = Now;
					M_PRECACHE128( 512, pSrc); // Precache
				}

				for (uint32 i = 0; i < 2; ++i)
				{
					vec128 Src0 = M_VLd(pSrc);
					vec128 Src1 = M_VLd(pSrc + 1);
					vec128 Src2 = M_VLd(pSrc + 2);

					vec128 * M_RESTRICT pVol = pVolumes;
					// Loop in the interest of code size
					for (uint32 i = 0; i < nDstVec; ++i)
					{
						if (DoPrezero == 8)
						{
							M_PREZERO128( 0, pDst); // Precache
							DoPrezero = 0;
						}
						++DoPrezero;
						vec128 VolS0 = M_VLd(pVol); 
						vec128 VolS1 = M_VLd(pVol + 1);
						vec128 VolS2 = M_VLd(pVol + 2);
						vec128 VolS3 = M_VLd(pVol + 3);
						vec128 VolS4 = M_VLd(pVol + 4);
						vec128 VolS5 = M_VLd(pVol + 5);
						vec128 Vol0 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS0)), MulInt16);
						vec128 Vol1 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS0)), MulInt16);
						vec128 Vol2 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS1)), MulInt16);
						vec128 Vol3 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS1)), MulInt16);
						vec128 Vol4 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS2)), MulInt16);
						vec128 Vol5 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS2)), MulInt16);
						vec128 Vol6 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS3)), MulInt16);
						vec128 Vol7 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS3)), MulInt16);
						vec128 Vol8 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS4)), MulInt16);
						vec128 Vol9	= M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS4)), MulInt16);
						vec128 Vol10 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS5)), MulInt16);
						vec128 Vol11 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS5)), MulInt16);
						vec128 Temp0 = M_VAdd(M_VAdd(M_VDp4(Src0, Vol0), M_VDp4(Src1, Vol1)), M_VDp4(Src2, Vol2));
						vec128 Temp1 = M_VAdd(M_VAdd(M_VDp4(Src0, Vol3), M_VDp4(Src1, Vol4)), M_VDp4(Src2, Vol5));
						vec128 Temp2 = M_VAdd(M_VAdd(M_VDp4(Src0, Vol6), M_VDp4(Src1, Vol7)), M_VDp4(Src2, Vol8));
						vec128 Temp3 = M_VAdd(M_VAdd(M_VDp4(Src0, Vol9), M_VDp4(Src1, Vol10)), M_VDp4(Src2, Vol11));

						*pDst = M_VScalarToPacked(Temp0, Temp1, Temp2, Temp3);
						++pDst;
						pVol += 6;
					}

					pSrc += 3;
				}
			}
		}
		break;
	case 4:
		{
			for (uint32 i = 0; i < nLoops; i+=8)
			{
				M_PRECACHE128( 512, pSrc); // Precache
				for (uint32 i = 0; i < 2; ++i)
				{
					vec128 Src0 = M_VLd(pSrc);
					vec128 Src1 = M_VLd(pSrc + 1);
					vec128 Src2 = M_VLd(pSrc + 2);
					vec128 Src3 = M_VLd(pSrc + 3);

					vec128 * M_RESTRICT pVol = pVolumes;
					// Loop in the interest of code size
					for (uint32 i = 0; i < nDstVec; ++i)
					{
						if (DoPrezero == 8)
						{
							M_PREZERO128( 0, pDst); // Precache
							DoPrezero = 0;
						}
						++DoPrezero;
						vec128 VolS0 = M_VLd(pVol); 
						vec128 VolS1 = M_VLd(pVol + 1);
						vec128 VolS2 = M_VLd(pVol + 2);
						vec128 VolS3 = M_VLd(pVol + 3);
						vec128 VolS4 = M_VLd(pVol + 4);
						vec128 VolS5 = M_VLd(pVol + 5);
						vec128 VolS6 = M_VLd(pVol + 6);
						vec128 VolS7 = M_VLd(pVol + 7);
						vec128 Vol0 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS0)), MulInt16);
						vec128 Vol1 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS0)), MulInt16);
						vec128 Vol2 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS1)), MulInt16);
						vec128 Vol3 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS1)), MulInt16);
						vec128 Vol4 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS2)), MulInt16);
						vec128 Vol5 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS2)), MulInt16);
						vec128 Vol6 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS3)), MulInt16);
						vec128 Vol7 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS3)), MulInt16);
						vec128 Vol8 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS4)), MulInt16);
						vec128 Vol9 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS4)), MulInt16);
						vec128 Vol10 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS5)), MulInt16);
						vec128 Vol11 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS5)), MulInt16);
						vec128 Vol12 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS6)), MulInt16);
						vec128 Vol13 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS6)), MulInt16);
						vec128 Vol14 = M_VMul(M_VCnv_i32_f32(M_VCnvL_u16_u32(VolS7)), MulInt16);
						vec128 Vol15 = M_VMul(M_VCnv_i32_f32(M_VCnvH_u16_u32(VolS7)), MulInt16);
						vec128 Temp0 = M_VAdd(M_VAdd(M_VDp4(Src0, Vol0), M_VDp4(Src1, Vol1)), M_VAdd(M_VDp4(Src2, Vol2), M_VDp4(Src3, Vol3)));
						vec128 Temp1 = M_VAdd(M_VAdd(M_VDp4(Src0, Vol4), M_VDp4(Src1, Vol5)), M_VAdd(M_VDp4(Src2, Vol6), M_VDp4(Src3, Vol7)));
						vec128 Temp2 = M_VAdd(M_VAdd(M_VDp4(Src0, Vol8), M_VDp4(Src1, Vol9)), M_VAdd(M_VDp4(Src2, Vol10), M_VDp4(Src3, Vol11)));
						vec128 Temp3 = M_VAdd(M_VAdd(M_VDp4(Src0, Vol12), M_VDp4(Src1, Vol13)), M_VAdd(M_VDp4(Src2, Vol14), M_VDp4(Src3, Vol15)));
						*pDst = M_VScalarToPacked(Temp0, Temp1, Temp2, Temp3);
						++pDst;
						pVol += 8;
					}

					pSrc += 4;
				}
			}
		}
		break;
	}
}

//#pragma optimize("", on)
//#pragma inline_depth()
