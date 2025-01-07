
void CSC_Mixer_DSP_Volume::ProcessFrame(CSC_Mixer_ProcessingInfo *_pInfo, void *_pInternalData, const void *_pParams, void *_pLastParams)
{

//	M_TRACEALWAYS("CSC_Mixer_DSP_Volume %f ", ((fp32 *)_pInfo->m_pDataIn)[0]);

	CParams *pParams = (CParams *)_pParams;
	CLastParams *pLastParams = (CLastParams *)_pLastParams;

	vec128 CurrentVolume = M_VLdScalar(pLastParams->m_Volume);
	vec128 VolumeDelta = M_VMul(M_VSub(M_VLdScalar(pParams->m_Volume), CurrentVolume), M_VRcp(M_VCnv_i32_f32(M_VLdScalar_u32(_pInfo->m_nSamples))));
	
	vec128 * M_RESTRICT pSrcDst = (vec128 *)_pInfo->m_pDataIn;

	uint32 nChannelVec = ((AlignUp(_pInfo->m_nChannels, 4)) >> 2);
	uint32 nSamples = _pInfo->m_nSamples;
	pLastParams->m_Volume = pParams->m_Volume;

	M_PRECACHE128( 0, pSrcDst); // Precache
	M_PRECACHE128( 128, pSrcDst); // Precache
	M_PRECACHE128( 256, pSrcDst); // Precache
	M_PRECACHE128( 384, pSrcDst); // Precache

#define Macro_DoCopy1 \
			M_PRECACHE128( 512, pSrcDst);\
			vec128 Src00 = pSrcDst[0];\
			vec128 Src01 = pSrcDst[1];\
			vec128 Src02 = pSrcDst[2];\
			vec128 Src03 = pSrcDst[3];\
			vec128 Src04 = pSrcDst[4];\
			vec128 Src05 = pSrcDst[5];\
			vec128 Src06 = pSrcDst[6];\
			vec128 Src07 = pSrcDst[7];\
			Src00 = M_VMul(Src00, CurrentVolume);\
			CurrentVolume = M_VAdd(CurrentVolume, VolumeDelta);\
			Src01 = M_VMul(Src01, CurrentVolume);\
			CurrentVolume = M_VAdd(CurrentVolume, VolumeDelta);\
			Src02 = M_VMul(Src02, CurrentVolume);\
			CurrentVolume = M_VAdd(CurrentVolume, VolumeDelta);\
			Src03 = M_VMul(Src03, CurrentVolume);\
			CurrentVolume = M_VAdd(CurrentVolume, VolumeDelta);\
			Src04 = M_VMul(Src04, CurrentVolume);\
			CurrentVolume = M_VAdd(CurrentVolume, VolumeDelta);\
			Src05 = M_VMul(Src05, CurrentVolume);\
			CurrentVolume = M_VAdd(CurrentVolume, VolumeDelta);\
			Src06 = M_VMul(Src06, CurrentVolume);\
			CurrentVolume = M_VAdd(CurrentVolume, VolumeDelta);\
			Src07 = M_VMul(Src07, CurrentVolume);\
			CurrentVolume = M_VAdd(CurrentVolume, VolumeDelta);\
			pSrcDst[0] = Src00;\
			pSrcDst[1] = Src01;\
			pSrcDst[2] = Src02;\
			pSrcDst[3] = Src03;\
			pSrcDst[4] = Src04;\
			pSrcDst[5] = Src05;\
			pSrcDst[6] = Src06;\
			pSrcDst[7] = Src07;\
			pSrcDst += 8;\
	

#define Macro_DoCopy2 \
			M_PRECACHE128( 512, pSrcDst);\
			vec128 Src00 = pSrcDst[0];\
			vec128 Src01 = pSrcDst[1];\
			vec128 Src02 = pSrcDst[2];\
			vec128 Src03 = pSrcDst[3];\
			vec128 Src04 = pSrcDst[4];\
			vec128 Src05 = pSrcDst[5];\
			vec128 Src06 = pSrcDst[6];\
			vec128 Src07 = pSrcDst[7];\
			Src00 = M_VMul(Src00, CurrentVolume);\
			Src01 = M_VMul(Src01, CurrentVolume);\
			CurrentVolume = M_VAdd(CurrentVolume, VolumeDelta);\
			Src02 = M_VMul(Src02, CurrentVolume);\
			Src03 = M_VMul(Src03, CurrentVolume);\
			CurrentVolume = M_VAdd(CurrentVolume, VolumeDelta);\
			Src04 = M_VMul(Src04, CurrentVolume);\
			Src05 = M_VMul(Src05, CurrentVolume);\
			CurrentVolume = M_VAdd(CurrentVolume, VolumeDelta);\
			Src06 = M_VMul(Src06, CurrentVolume);\
			Src07 = M_VMul(Src07, CurrentVolume);\
			CurrentVolume = M_VAdd(CurrentVolume, VolumeDelta);\
			pSrcDst[0] = Src00;\
			pSrcDst[1] = Src01;\
			pSrcDst[2] = Src02;\
			pSrcDst[3] = Src03;\
			pSrcDst[4] = Src04;\
			pSrcDst[5] = Src05;\
			pSrcDst[6] = Src06;\
			pSrcDst[7] = Src07;\
			pSrcDst += 8;\

#define Macro_DoCopy3 \
			M_PRECACHE128( 512, pSrcDst);\
			vec128 Src00 = pSrcDst[0];\
			vec128 Src01 = pSrcDst[1];\
			vec128 Src02 = pSrcDst[2];\
			vec128 Src03 = pSrcDst[3];\
			vec128 Src04 = pSrcDst[4];\
			vec128 Src05 = pSrcDst[5];\
			vec128 Src06 = pSrcDst[6];\
			vec128 Src07 = pSrcDst[7];\
			vec128 Src08 = pSrcDst[8];\
			vec128 Src09 = pSrcDst[9];\
			vec128 Src10 = pSrcDst[10];\
			vec128 Src11 = pSrcDst[11];\
			Src00 = M_VMul(Src00, CurrentVolume);\
			Src01 = M_VMul(Src01, CurrentVolume);\
			Src02 = M_VMul(Src02, CurrentVolume);\
			CurrentVolume = M_VAdd(CurrentVolume, VolumeDelta);\
			Src03 = M_VMul(Src03, CurrentVolume);\
			Src04 = M_VMul(Src04, CurrentVolume);\
			Src05 = M_VMul(Src05, CurrentVolume);\
			CurrentVolume = M_VAdd(CurrentVolume, VolumeDelta);\
			Src06 = M_VMul(Src06, CurrentVolume);\
			Src07 = M_VMul(Src07, CurrentVolume);\
			Src08 = M_VMul(Src08, CurrentVolume);\
			CurrentVolume = M_VAdd(CurrentVolume, VolumeDelta);\
			Src09 = M_VMul(Src09, CurrentVolume);\
			Src10 = M_VMul(Src10, CurrentVolume);\
			Src11 = M_VMul(Src11, CurrentVolume);\
			CurrentVolume = M_VAdd(CurrentVolume, VolumeDelta);\
			pSrcDst[0] = Src00;\
			pSrcDst[1] = Src01;\
			pSrcDst[2] = Src02;\
			pSrcDst[3] = Src03;\
			pSrcDst[4] = Src04;\
			pSrcDst[5] = Src05;\
			pSrcDst[6] = Src06;\
			pSrcDst[7] = Src07;\
			pSrcDst[8] = Src08;\
			pSrcDst[9] = Src09;\
			pSrcDst[10] = Src10;\
			pSrcDst[11] = Src11;\
			pSrcDst += 12;\
	
#define Macro_DoCopy4 \
			M_PRECACHE128( 512, pSrcDst);\
			vec128 Src00 = pSrcDst[0];\
			vec128 Src01 = pSrcDst[1];\
			vec128 Src02 = pSrcDst[2];\
			vec128 Src03 = pSrcDst[3];\
			vec128 Src04 = pSrcDst[4];\
			vec128 Src05 = pSrcDst[5];\
			vec128 Src06 = pSrcDst[6];\
			vec128 Src07 = pSrcDst[7];\
			Src00 = M_VMul(Src00, CurrentVolume);\
			Src01 = M_VMul(Src01, CurrentVolume);\
			Src02 = M_VMul(Src02, CurrentVolume);\
			Src03 = M_VMul(Src03, CurrentVolume);\
			CurrentVolume = M_VAdd(CurrentVolume, VolumeDelta);\
			Src04 = M_VMul(Src04, CurrentVolume);\
			Src05 = M_VMul(Src05, CurrentVolume);\
			Src06 = M_VMul(Src06, CurrentVolume);\
			Src07 = M_VMul(Src07, CurrentVolume);\
			CurrentVolume = M_VAdd(CurrentVolume, VolumeDelta);\
			pSrcDst[0] = Src00;\
			pSrcDst[1] = Src01;\
			pSrcDst[2] = Src02;\
			pSrcDst[3] = Src03;\
			pSrcDst[4] = Src04;\
			pSrcDst[5] = Src05;\
			pSrcDst[6] = Src06;\
			pSrcDst[7] = Src07;\
			pSrcDst += 8;\

	switch (nChannelVec)
	{
	case 1:
		{
			for (uint32 i = 0; i < nSamples; i+=8)
			{
				Macro_DoCopy1;
			}
		}
		break;
	case 2:
		{
			for (uint32 i = 0; i < nSamples; i+=4)
			{
				Macro_DoCopy2;
			}
		}
		break;
	case 3:
		{
			for (uint32 i = 0; i < nSamples; i+=4)
			{
				Macro_DoCopy3;
			}
		}
		break;
	case 4:
		{
			for (uint32 i = 0; i < nSamples; i+=2)
			{
				Macro_DoCopy4;
			}
		}
		break;
	}

#undef Macro_DoCopy1
#undef Macro_DoCopy2
#undef Macro_DoCopy3
#undef Macro_DoCopy4

}

