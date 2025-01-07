
void CSC_Mixer_DSP_BiQuad::Internal_ProcessChannelVectors(vec128 *_pDst, const vec128 *_pSrc, mint _nSamples, mint _nChannelVec, const CCoef &_StartCoef, const CCoef &_EndCoef,
		vec128 *_pHistoryIn_1, vec128 *_pHistoryIn_2, vec128 *_pHistoryOut_1, vec128 *_pHistoryOut_2)
{

	uint32 nChannelVec = _nChannelVec;
	uint32 nSamples = _nSamples;

	vec128 SamplesInv = M_VRcp(M_VLdScalar(fp32(nSamples)));
	vec128 Coefa1 = M_VLdScalar(_StartCoef.m_Coef_a1);
	vec128 Coefa2 = M_VLdScalar(_StartCoef.m_Coef_a2);
	vec128 Coefb0 = M_VLdScalar(_StartCoef.m_Coef_b0);
	vec128 Coefb1 = M_VLdScalar(_StartCoef.m_Coef_b1);
	vec128 Coefb2 = M_VLdScalar(_StartCoef.m_Coef_b2);
	vec128 Coefa1Delta = M_VMul(M_VSub(M_VLdScalar(_EndCoef.m_Coef_a1), Coefa1), SamplesInv);
	vec128 Coefa2Delta = M_VMul(M_VSub(M_VLdScalar(_EndCoef.m_Coef_a2), Coefa2), SamplesInv);
	vec128 Coefb0Delta = M_VMul(M_VSub(M_VLdScalar(_EndCoef.m_Coef_b0), Coefb0), SamplesInv);
	vec128 Coefb1Delta = M_VMul(M_VSub(M_VLdScalar(_EndCoef.m_Coef_b1), Coefb1), SamplesInv);
	vec128 Coefb2Delta = M_VMul(M_VSub(M_VLdScalar(_EndCoef.m_Coef_b2), Coefb2), SamplesInv);

/*				fp32 Output = m_Coef_b0 * _In + m_Coef_b1*m_HistIn_1 + m_Coef_b2*m_HistIn_2 - m_Coef_a1*m_HistOut_1 - m_Coef_a2*m_HistOut_2;

				m_HistIn_2 = m_HistIn_1;
				m_HistIn_1 = _In;

				m_HistOut_2 = m_HistOut_1;
				m_HistOut_1 = Output;*/

	const vec128 * M_RESTRICT pSrc = _pSrc;
	vec128 * M_RESTRICT pDst = _pDst;
		
	M_PRECACHE128( 0, pSrc); // Precache
	M_PRECACHE128( 128, pSrc); // Precache
	M_PRECACHE128( 256, pSrc); // Precache
	M_PRECACHE128( 384, pSrc); // Precache

	switch (nChannelVec)
	{
	case 1:
		{
			vec128 HistIn1_0 = _pHistoryIn_1[0];
			vec128 HistIn2_0 = _pHistoryIn_2[0];
			vec128 HistOut1_0 = _pHistoryOut_1[0];
			vec128 HistOut2_0 = _pHistoryOut_2[0];

			for (uint32 i = 0; i < nSamples; i+=8)
			{
				vec128 Src00 = pSrc[0];
				vec128 Src01 = pSrc[1];
				vec128 Src02 = pSrc[2];
				vec128 Src03 = pSrc[3];
				vec128 Src04 = pSrc[4];
				vec128 Src05 = pSrc[5];
				vec128 Src06 = pSrc[6];
				vec128 Src07 = pSrc[7];

				vec128 Out00 = M_VMul(Coefb0, Src00);
				Out00 = M_VMAdd(Coefb1, HistIn1_0, Out00);
				Out00 = M_VMAdd(Coefb2, HistIn2_0, Out00);
				Out00 = M_VMAdd(Coefa1, HistOut1_0, Out00);
				Out00 = M_VMAdd(Coefa2, HistOut2_0, Out00);
				HistIn2_0 = HistIn1_0;
				HistIn1_0 = Src00;
				HistOut2_0 = HistOut1_0;
				HistOut1_0 = Out00;
				Coefa1 = M_VAdd(Coefa1, Coefa1Delta);
				Coefa2 = M_VAdd(Coefa2, Coefa2Delta);
				Coefb0 = M_VAdd(Coefb0, Coefb0Delta);
				Coefb1 = M_VAdd(Coefb1, Coefb1Delta);
				Coefb2 = M_VAdd(Coefb2, Coefb2Delta);

				vec128 Out01 = M_VMul(Coefb0, Src01);
				Out01 = M_VMAdd(Coefb1, HistIn1_0, Out01);
				Out01 = M_VMAdd(Coefb2, HistIn2_0, Out01);
				Out01 = M_VMAdd(Coefa1, HistOut1_0, Out01);
				Out01 = M_VMAdd(Coefa2, HistOut2_0, Out01);
				HistIn2_0 = HistIn1_0;
				HistIn1_0 = Src01;
				HistOut2_0 = HistOut1_0;
				HistOut1_0 = Out01;
				Coefa1 = M_VAdd(Coefa1, Coefa1Delta);
				Coefa2 = M_VAdd(Coefa2, Coefa2Delta);
				Coefb0 = M_VAdd(Coefb0, Coefb0Delta);
				Coefb1 = M_VAdd(Coefb1, Coefb1Delta);
				Coefb2 = M_VAdd(Coefb2, Coefb2Delta);

				vec128 Out02 = M_VMul(Coefb0, Src02);
				Out02 = M_VMAdd(Coefb1, HistIn1_0, Out02);
				Out02 = M_VMAdd(Coefb2, HistIn2_0, Out02);
				Out02 = M_VMAdd(Coefa1, HistOut1_0, Out02);
				Out02 = M_VMAdd(Coefa2, HistOut2_0, Out02);
				HistIn2_0 = HistIn1_0;
				HistIn1_0 = Src02;
				HistOut2_0 = HistOut1_0;
				HistOut1_0 = Out02;
				Coefa1 = M_VAdd(Coefa1, Coefa1Delta);
				Coefa2 = M_VAdd(Coefa2, Coefa2Delta);
				Coefb0 = M_VAdd(Coefb0, Coefb0Delta);
				Coefb1 = M_VAdd(Coefb1, Coefb1Delta);
				Coefb2 = M_VAdd(Coefb2, Coefb2Delta);

				vec128 Out03 = M_VMul(Coefb0, Src03);
				Out03 = M_VMAdd(Coefb1, HistIn1_0, Out03);
				Out03 = M_VMAdd(Coefb2, HistIn2_0, Out03);
				Out03 = M_VMAdd(Coefa1, HistOut1_0, Out03);
				Out03 = M_VMAdd(Coefa2, HistOut2_0, Out03);
				HistIn2_0 = HistIn1_0;
				HistIn1_0 = Src03;
				HistOut2_0 = HistOut1_0;
				HistOut1_0 = Out03;
				Coefa1 = M_VAdd(Coefa1, Coefa1Delta);
				Coefa2 = M_VAdd(Coefa2, Coefa2Delta);
				Coefb0 = M_VAdd(Coefb0, Coefb0Delta);
				Coefb1 = M_VAdd(Coefb1, Coefb1Delta);
				Coefb2 = M_VAdd(Coefb2, Coefb2Delta);

				vec128 Out04 = M_VMul(Coefb0, Src04);
				Out04 = M_VMAdd(Coefb1, HistIn1_0, Out04);
				Out04 = M_VMAdd(Coefb2, HistIn2_0, Out04);
				Out04 = M_VMAdd(Coefa1, HistOut1_0, Out04);
				Out04 = M_VMAdd(Coefa2, HistOut2_0, Out04);
				HistIn2_0 = HistIn1_0;
				HistIn1_0 = Src04;
				HistOut2_0 = HistOut1_0;
				HistOut1_0 = Out04;
				Coefa1 = M_VAdd(Coefa1, Coefa1Delta);
				Coefa2 = M_VAdd(Coefa2, Coefa2Delta);
				Coefb0 = M_VAdd(Coefb0, Coefb0Delta);
				Coefb1 = M_VAdd(Coefb1, Coefb1Delta);
				Coefb2 = M_VAdd(Coefb2, Coefb2Delta);

				vec128 Out05 = M_VMul(Coefb0, Src05);
				Out05 = M_VMAdd(Coefb1, HistIn1_0, Out05);
				Out05 = M_VMAdd(Coefb2, HistIn2_0, Out05);
				Out05 = M_VMAdd(Coefa1, HistOut1_0, Out05);
				Out05 = M_VMAdd(Coefa2, HistOut2_0, Out05);
				HistIn2_0 = HistIn1_0;
				HistIn1_0 = Src05;
				HistOut2_0 = HistOut1_0;
				HistOut1_0 = Out05;
				Coefa1 = M_VAdd(Coefa1, Coefa1Delta);
				Coefa2 = M_VAdd(Coefa2, Coefa2Delta);
				Coefb0 = M_VAdd(Coefb0, Coefb0Delta);
				Coefb1 = M_VAdd(Coefb1, Coefb1Delta);
				Coefb2 = M_VAdd(Coefb2, Coefb2Delta);

				vec128 Out06 = M_VMul(Coefb0, Src06);
				Out06 = M_VMAdd(Coefb1, HistIn1_0, Out06);
				Out06 = M_VMAdd(Coefb2, HistIn2_0, Out06);
				Out06 = M_VMAdd(Coefa1, HistOut1_0, Out06);
				Out06 = M_VMAdd(Coefa2, HistOut2_0, Out06);
				HistIn2_0 = HistIn1_0;
				HistIn1_0 = Src06;
				HistOut2_0 = HistOut1_0;
				HistOut1_0 = Out06;
				Coefa1 = M_VAdd(Coefa1, Coefa1Delta);
				Coefa2 = M_VAdd(Coefa2, Coefa2Delta);
				Coefb0 = M_VAdd(Coefb0, Coefb0Delta);
				Coefb1 = M_VAdd(Coefb1, Coefb1Delta);
				Coefb2 = M_VAdd(Coefb2, Coefb2Delta);

				vec128 Out07 = M_VMul(Coefb0, Src07);
				Out07 = M_VMAdd(Coefb1, HistIn1_0, Out07);
				Out07 = M_VMAdd(Coefb2, HistIn2_0, Out07);
				Out07 = M_VMAdd(Coefa1, HistOut1_0, Out07);
				Out07 = M_VMAdd(Coefa2, HistOut2_0, Out07);
				HistIn2_0 = HistIn1_0;
				HistIn1_0 = Src07;
				HistOut2_0 = HistOut1_0;
				HistOut1_0 = Out07;
				Coefa1 = M_VAdd(Coefa1, Coefa1Delta);
				Coefa2 = M_VAdd(Coefa2, Coefa2Delta);
				Coefb0 = M_VAdd(Coefb0, Coefb0Delta);
				Coefb1 = M_VAdd(Coefb1, Coefb1Delta);
				Coefb2 = M_VAdd(Coefb2, Coefb2Delta);

				pDst[0] = Out00;
				pDst[1] = Out01;
				pDst[2] = Out02;
				pDst[3] = Out03;
				pDst[4] = Out04;
				pDst[5] = Out05;
				pDst[6] = Out06;
				pDst[7] = Out07;
				pDst += 8;
				pSrc += 8;
			}

			_pHistoryIn_1[0] = HistIn1_0;
			_pHistoryIn_2[0] = HistIn2_0;
			_pHistoryOut_1[0] = HistOut1_0;
			_pHistoryOut_2[0] = HistOut2_0;
		}
		break;
	case 2:
		{
			vec128 HistIn1_0 = _pHistoryIn_1[0];
			vec128 HistIn2_0 = _pHistoryIn_2[0];
			vec128 HistOut1_0 = _pHistoryOut_1[0];
			vec128 HistOut2_0 = _pHistoryOut_2[0];
			vec128 HistIn1_1 = _pHistoryIn_1[1];
			vec128 HistIn2_1 = _pHistoryIn_2[1];
			vec128 HistOut1_1 = _pHistoryOut_1[1];
			vec128 HistOut2_1 = _pHistoryOut_2[1];

			for (uint32 i = 0; i < nSamples; i+=4)
			{
				vec128 Src00 = pSrc[0];
				vec128 Src01 = pSrc[1];
				vec128 Src02 = pSrc[2];
				vec128 Src03 = pSrc[3];
				vec128 Src04 = pSrc[4];
				vec128 Src05 = pSrc[5];
				vec128 Src06 = pSrc[6];
				vec128 Src07 = pSrc[7];

				vec128 Out00 = M_VMul(Coefb0, Src00);
				Out00 = M_VMAdd(Coefb1, HistIn1_0, Out00);
				Out00 = M_VMAdd(Coefb2, HistIn2_0, Out00);
				Out00 = M_VMAdd(Coefa1, HistOut1_0, Out00);
				Out00 = M_VMAdd(Coefa2, HistOut2_0, Out00);
				HistIn2_0 = HistIn1_0;
				HistIn1_0 = Src00;
				HistOut2_0 = HistOut1_0;
				HistOut1_0 = Out00;
				vec128 Out01 = M_VMul(Coefb0, Src01);
				Out01 = M_VMAdd(Coefb1, HistIn1_1, Out01);
				Out01 = M_VMAdd(Coefb2, HistIn2_1, Out01);
				Out01 = M_VMAdd(Coefa1, HistOut1_1, Out01);
				Out01 = M_VMAdd(Coefa2, HistOut2_1, Out01);
				HistIn2_1 = HistIn1_1;
				HistIn1_1 = Src01;
				HistOut2_1 = HistOut1_1;
				HistOut1_1 = Out01;

				Coefa1 = M_VAdd(Coefa1, Coefa1Delta);
				Coefa2 = M_VAdd(Coefa2, Coefa2Delta);
				Coefb0 = M_VAdd(Coefb0, Coefb0Delta);
				Coefb1 = M_VAdd(Coefb1, Coefb1Delta);
				Coefb2 = M_VAdd(Coefb2, Coefb2Delta);

				vec128 Out02 = M_VMul(Coefb0, Src02);
				Out02 = M_VMAdd(Coefb1, HistIn1_0, Out02);
				Out02 = M_VMAdd(Coefb2, HistIn2_0, Out02);
				Out02 = M_VMAdd(Coefa1, HistOut1_0, Out02);
				Out02 = M_VMAdd(Coefa2, HistOut2_0, Out02);
				HistIn2_0 = HistIn1_0;
				HistIn1_0 = Src02;
				HistOut2_0 = HistOut1_0;
				HistOut1_0 = Out02;
				vec128 Out03 = M_VMul(Coefb0, Src03);
				Out03 = M_VMAdd(Coefb1, HistIn1_1, Out03);
				Out03 = M_VMAdd(Coefb2, HistIn2_1, Out03);
				Out03 = M_VMAdd(Coefa1, HistOut1_1, Out03);
				Out03 = M_VMAdd(Coefa2, HistOut2_1, Out03);
				HistIn2_1 = HistIn1_1;
				HistIn1_1 = Src03;
				HistOut2_1 = HistOut1_1;
				HistOut1_1 = Out03;

				Coefa1 = M_VAdd(Coefa1, Coefa1Delta);
				Coefa2 = M_VAdd(Coefa2, Coefa2Delta);
				Coefb0 = M_VAdd(Coefb0, Coefb0Delta);
				Coefb1 = M_VAdd(Coefb1, Coefb1Delta);
				Coefb2 = M_VAdd(Coefb2, Coefb2Delta);

				vec128 Out04 = M_VMul(Coefb0, Src04);
				Out04 = M_VMAdd(Coefb1, HistIn1_0, Out04);
				Out04 = M_VMAdd(Coefb2, HistIn2_0, Out04);
				Out04 = M_VMAdd(Coefa1, HistOut1_0, Out04);
				Out04 = M_VMAdd(Coefa2, HistOut2_0, Out04);
				HistIn2_0 = HistIn1_0;
				HistIn1_0 = Src04;
				HistOut2_0 = HistOut1_0;
				HistOut1_0 = Out04;
				vec128 Out05 = M_VMul(Coefb0, Src05);
				Out05 = M_VMAdd(Coefb1, HistIn1_1, Out05);
				Out05 = M_VMAdd(Coefb2, HistIn2_1, Out05);
				Out05 = M_VMAdd(Coefa1, HistOut1_1, Out05);
				Out05 = M_VMAdd(Coefa2, HistOut2_1, Out05);
				HistIn2_1 = HistIn1_1;
				HistIn1_1 = Src05;
				HistOut2_1 = HistOut1_1;
				HistOut1_1 = Out05;

				Coefa1 = M_VAdd(Coefa1, Coefa1Delta);
				Coefa2 = M_VAdd(Coefa2, Coefa2Delta);
				Coefb0 = M_VAdd(Coefb0, Coefb0Delta);
				Coefb1 = M_VAdd(Coefb1, Coefb1Delta);
				Coefb2 = M_VAdd(Coefb2, Coefb2Delta);

				vec128 Out06 = M_VMul(Coefb0, Src06);
				Out06 = M_VMAdd(Coefb1, HistIn1_0, Out06);
				Out06 = M_VMAdd(Coefb2, HistIn2_0, Out06);
				Out06 = M_VMAdd(Coefa1, HistOut1_0, Out06);
				Out06 = M_VMAdd(Coefa2, HistOut2_0, Out06);
				HistIn2_0 = HistIn1_0;
				HistIn1_0 = Src06;
				HistOut2_0 = HistOut1_0;
				HistOut1_0 = Out06;
				vec128 Out07 = M_VMul(Coefb0, Src07);
				Out07 = M_VMAdd(Coefb1, HistIn1_1, Out07);
				Out07 = M_VMAdd(Coefb2, HistIn2_1, Out07);
				Out07 = M_VMAdd(Coefa1, HistOut1_1, Out07);
				Out07 = M_VMAdd(Coefa2, HistOut2_1, Out07);
				HistIn2_1 = HistIn1_1;
				HistIn1_1 = Src07;
				HistOut2_1 = HistOut1_1;
				HistOut1_1 = Out07;

				Coefa1 = M_VAdd(Coefa1, Coefa1Delta);
				Coefa2 = M_VAdd(Coefa2, Coefa2Delta);
				Coefb0 = M_VAdd(Coefb0, Coefb0Delta);
				Coefb1 = M_VAdd(Coefb1, Coefb1Delta);
				Coefb2 = M_VAdd(Coefb2, Coefb2Delta);

				pDst[0] = Out00;
				pDst[1] = Out01;
				pDst[2] = Out02;
				pDst[3] = Out03;
				pDst[4] = Out04;
				pDst[5] = Out05;
				pDst[6] = Out06;
				pDst[7] = Out07;
				pDst += 8;
				pSrc += 8;
			}

			_pHistoryIn_1[0] = HistIn1_0;
			_pHistoryIn_2[0] = HistIn2_0;
			_pHistoryOut_1[0] = HistOut1_0;
			_pHistoryOut_2[0] = HistOut2_0;
			_pHistoryIn_1[1] = HistIn1_1;
			_pHistoryIn_2[1] = HistIn2_1;
			_pHistoryOut_1[1] = HistOut1_1;
			_pHistoryOut_2[1] = HistOut2_1;
		}
		break;
	case 3:
		{
			vec128 HistIn1_0 = _pHistoryIn_1[0];
			vec128 HistIn2_0 = _pHistoryIn_2[0];
			vec128 HistOut1_0 = _pHistoryOut_1[0];
			vec128 HistOut2_0 = _pHistoryOut_2[0];
			vec128 HistIn1_1 = _pHistoryIn_1[1];
			vec128 HistIn2_1 = _pHistoryIn_2[1];
			vec128 HistOut1_1 = _pHistoryOut_1[1];
			vec128 HistOut2_1 = _pHistoryOut_2[1];
			vec128 HistIn1_2 = _pHistoryIn_1[2];
			vec128 HistIn2_2 = _pHistoryIn_2[2];
			vec128 HistOut1_2 = _pHistoryOut_1[2];
			vec128 HistOut2_2 = _pHistoryOut_2[2];

			for (uint32 i = 0; i < nSamples; i+=4)
			{
				vec128 Src00 = pSrc[0];
				vec128 Src01 = pSrc[1];
				vec128 Src02 = pSrc[2];
				vec128 Src03 = pSrc[3];
				vec128 Src04 = pSrc[4];
				vec128 Src05 = pSrc[5];
				vec128 Src06 = pSrc[6];
				vec128 Src07 = pSrc[7];
				vec128 Src08 = pSrc[8];
				vec128 Src09 = pSrc[9];
				vec128 Src10 = pSrc[10];
				vec128 Src11 = pSrc[11];

				vec128 Out00 = M_VMul(Coefb0, Src00);
				Out00 = M_VMAdd(Coefb1, HistIn1_0, Out00);
				Out00 = M_VMAdd(Coefb2, HistIn2_0, Out00);
				Out00 = M_VMAdd(Coefa1, HistOut1_0, Out00);
				Out00 = M_VMAdd(Coefa2, HistOut2_0, Out00);
				HistIn2_0 = HistIn1_0;
				HistIn1_0 = Src00;
				HistOut2_0 = HistOut1_0;
				HistOut1_0 = Out00;
				vec128 Out01 = M_VMul(Coefb0, Src01);
				Out01 = M_VMAdd(Coefb1, HistIn1_1, Out01);
				Out01 = M_VMAdd(Coefb2, HistIn2_1, Out01);
				Out01 = M_VMAdd(Coefa1, HistOut1_1, Out01);
				Out01 = M_VMAdd(Coefa2, HistOut2_1, Out01);
				HistIn2_1 = HistIn1_1;
				HistIn1_1 = Src01;
				HistOut2_1 = HistOut1_1;
				HistOut1_1 = Out01;
				vec128 Out02 = M_VMul(Coefb0, Src02);
				Out02 = M_VMAdd(Coefb1, HistIn1_2, Out02);
				Out02 = M_VMAdd(Coefb2, HistIn2_2, Out02);
				Out02 = M_VMAdd(Coefa1, HistOut1_2, Out02);
				Out02 = M_VMAdd(Coefa2, HistOut2_2, Out02);
				HistIn2_2 = HistIn1_2;
				HistIn1_2 = Src02;
				HistOut2_2 = HistOut1_2;
				HistOut1_2 = Out02;

				Coefa1 = M_VAdd(Coefa1, Coefa1Delta);
				Coefa2 = M_VAdd(Coefa2, Coefa2Delta);
				Coefb0 = M_VAdd(Coefb0, Coefb0Delta);
				Coefb1 = M_VAdd(Coefb1, Coefb1Delta);
				Coefb2 = M_VAdd(Coefb2, Coefb2Delta);

				vec128 Out03 = M_VMul(Coefb0, Src03);
				Out03 = M_VMAdd(Coefb1, HistIn1_0, Out03);
				Out03 = M_VMAdd(Coefb2, HistIn2_0, Out03);
				Out03 = M_VMAdd(Coefa1, HistOut1_0, Out03);
				Out03 = M_VMAdd(Coefa2, HistOut2_0, Out03);
				HistIn2_0 = HistIn1_0;
				HistIn1_0 = Src03;
				HistOut2_0 = HistOut1_0;
				HistOut1_0 = Out03;
				vec128 Out04 = M_VMul(Coefb0, Src04);
				Out04 = M_VMAdd(Coefb1, HistIn1_1, Out04);
				Out04 = M_VMAdd(Coefb2, HistIn2_1, Out04);
				Out04 = M_VMAdd(Coefa1, HistOut1_1, Out04);
				Out04 = M_VMAdd(Coefa2, HistOut2_1, Out04);
				HistIn2_1 = HistIn1_1;
				HistIn1_1 = Src04;
				HistOut2_1 = HistOut1_1;
				HistOut1_1 = Out04;
				vec128 Out05 = M_VMul(Coefb0, Src05);
				Out05 = M_VMAdd(Coefb1, HistIn1_2, Out05);
				Out05 = M_VMAdd(Coefb2, HistIn2_2, Out05);
				Out05 = M_VMAdd(Coefa1, HistOut1_2, Out05);
				Out05 = M_VMAdd(Coefa2, HistOut2_2, Out05);
				HistIn2_2 = HistIn1_2;
				HistIn1_2 = Src05;
				HistOut2_2 = HistOut1_2;
				HistOut1_2 = Out05;

				Coefa1 = M_VAdd(Coefa1, Coefa1Delta);
				Coefa2 = M_VAdd(Coefa2, Coefa2Delta);
				Coefb0 = M_VAdd(Coefb0, Coefb0Delta);
				Coefb1 = M_VAdd(Coefb1, Coefb1Delta);
				Coefb2 = M_VAdd(Coefb2, Coefb2Delta);

				vec128 Out06 = M_VMul(Coefb0, Src06);
				Out06 = M_VMAdd(Coefb1, HistIn1_0, Out06);
				Out06 = M_VMAdd(Coefb2, HistIn2_0, Out06);
				Out06 = M_VMAdd(Coefa1, HistOut1_0, Out06);
				Out06 = M_VMAdd(Coefa2, HistOut2_0, Out06);
				HistIn2_0 = HistIn1_0;
				HistIn1_0 = Src06;
				HistOut2_0 = HistOut1_0;
				HistOut1_0 = Out06;
				vec128 Out07 = M_VMul(Coefb0, Src07);
				Out07 = M_VMAdd(Coefb1, HistIn1_1, Out07);
				Out07 = M_VMAdd(Coefb2, HistIn2_1, Out07);
				Out07 = M_VMAdd(Coefa1, HistOut1_1, Out07);
				Out07 = M_VMAdd(Coefa2, HistOut2_1, Out07);
				HistIn2_1 = HistIn1_1;
				HistIn1_1 = Src07;
				HistOut2_1 = HistOut1_1;
				HistOut1_1 = Out07;
				vec128 Out08 = M_VMul(Coefb0, Src08);
				Out08 = M_VMAdd(Coefb1, HistIn1_2, Out08);
				Out08 = M_VMAdd(Coefb2, HistIn2_2, Out08);
				Out08 = M_VMAdd(Coefa1, HistOut1_2, Out08);
				Out08 = M_VMAdd(Coefa2, HistOut2_2, Out08);
				HistIn2_2 = HistIn1_2;
				HistIn1_2 = Src08;
				HistOut2_2 = HistOut1_2;
				HistOut1_2 = Out08;

				Coefa1 = M_VAdd(Coefa1, Coefa1Delta);
				Coefa2 = M_VAdd(Coefa2, Coefa2Delta);
				Coefb0 = M_VAdd(Coefb0, Coefb0Delta);
				Coefb1 = M_VAdd(Coefb1, Coefb1Delta);
				Coefb2 = M_VAdd(Coefb2, Coefb2Delta);

				vec128 Out09 = M_VMul(Coefb0, Src09);
				Out09 = M_VMAdd(Coefb1, HistIn1_0, Out09);
				Out09 = M_VMAdd(Coefb2, HistIn2_0, Out09);
				Out09 = M_VMAdd(Coefa1, HistOut1_0, Out09);
				Out09 = M_VMAdd(Coefa2, HistOut2_0, Out09);
				HistIn2_0 = HistIn1_0;
				HistIn1_0 = Src09;
				HistOut2_0 = HistOut1_0;
				HistOut1_0 = Out09;
				vec128 Out10 = M_VMul(Coefb0, Src10);
				Out10 = M_VMAdd(Coefb1, HistIn1_1, Out10);
				Out10 = M_VMAdd(Coefb2, HistIn2_1, Out10);
				Out10 = M_VMAdd(Coefa1, HistOut1_1, Out10);
				Out10 = M_VMAdd(Coefa2, HistOut2_1, Out10);
				HistIn2_1 = HistIn1_1;
				HistIn1_1 = Src10;
				HistOut2_1 = HistOut1_1;
				HistOut1_1 = Out10;
				vec128 Out11 = M_VMul(Coefb0, Src11);
				Out11 = M_VMAdd(Coefb1, HistIn1_2, Out11);
				Out11 = M_VMAdd(Coefb2, HistIn2_2, Out11);
				Out11 = M_VMAdd(Coefa1, HistOut1_2, Out11);
				Out11 = M_VMAdd(Coefa2, HistOut2_2, Out11);
				HistIn2_2 = HistIn1_2;
				HistIn1_2 = Src11;
				HistOut2_2 = HistOut1_2;
				HistOut1_2 = Out11;

				Coefa1 = M_VAdd(Coefa1, Coefa1Delta);
				Coefa2 = M_VAdd(Coefa2, Coefa2Delta);
				Coefb0 = M_VAdd(Coefb0, Coefb0Delta);
				Coefb1 = M_VAdd(Coefb1, Coefb1Delta);
				Coefb2 = M_VAdd(Coefb2, Coefb2Delta);

				pDst[0] = Out00;
				pDst[1] = Out01;
				pDst[2] = Out02;
				pDst[3] = Out03;
				pDst[4] = Out04;
				pDst[5] = Out05;
				pDst[6] = Out06;
				pDst[7] = Out07;
				pDst[8] = Out08;
				pDst[9] = Out09;
				pDst[10] = Out10;
				pDst[11] = Out11;
				pDst += 12;
				pSrc += 12;
			}

			_pHistoryIn_1[0] = HistIn1_0;
			_pHistoryIn_2[0] = HistIn2_0;
			_pHistoryOut_1[0] = HistOut1_0;
			_pHistoryOut_2[0] = HistOut2_0;
			_pHistoryIn_1[1] = HistIn1_1;
			_pHistoryIn_2[1] = HistIn2_1;
			_pHistoryOut_1[1] = HistOut1_1;
			_pHistoryOut_2[1] = HistOut2_1;
			_pHistoryIn_1[2] = HistIn1_2;
			_pHistoryIn_2[2] = HistIn2_2;
			_pHistoryOut_1[2] = HistOut1_2;
			_pHistoryOut_2[2] = HistOut2_2;
		}
		break;
	case 4:
		{
			vec128 HistIn1_0 = _pHistoryIn_1[0];
			vec128 HistIn2_0 = _pHistoryIn_2[0];
			vec128 HistOut1_0 = _pHistoryOut_1[0];
			vec128 HistOut2_0 = _pHistoryOut_2[0];
			vec128 HistIn1_1 = _pHistoryIn_1[1];
			vec128 HistIn2_1 = _pHistoryIn_2[1];
			vec128 HistOut1_1 = _pHistoryOut_1[1];
			vec128 HistOut2_1 = _pHistoryOut_2[1];
			vec128 HistIn1_2 = _pHistoryIn_1[2];
			vec128 HistIn2_2 = _pHistoryIn_2[2];
			vec128 HistOut1_2 = _pHistoryOut_1[2];
			vec128 HistOut2_2 = _pHistoryOut_2[2];
			vec128 HistIn1_3 = _pHistoryIn_1[3];
			vec128 HistIn2_3 = _pHistoryIn_2[3];
			vec128 HistOut1_3 = _pHistoryOut_1[3];
			vec128 HistOut2_3 = _pHistoryOut_2[3];

			for (uint32 i = 0; i < nSamples; i+=2)
			{
				vec128 Src00 = pSrc[0];
				vec128 Src01 = pSrc[1];
				vec128 Src02 = pSrc[2];
				vec128 Src03 = pSrc[3];
				vec128 Src04 = pSrc[4];
				vec128 Src05 = pSrc[5];
				vec128 Src06 = pSrc[6];
				vec128 Src07 = pSrc[7];

				vec128 Out00 = M_VMul(Coefb0, Src00);
				Out00 = M_VMAdd(Coefb1, HistIn1_0, Out00);
				Out00 = M_VMAdd(Coefb2, HistIn2_0, Out00);
				Out00 = M_VMAdd(Coefa1, HistOut1_0, Out00);
				Out00 = M_VMAdd(Coefa2, HistOut2_0, Out00);
				HistIn2_0 = HistIn1_0;
				HistIn1_0 = Src00;
				HistOut2_0 = HistOut1_0;
				HistOut1_0 = Out00;
				vec128 Out01 = M_VMul(Coefb0, Src01);
				Out01 = M_VMAdd(Coefb1, HistIn1_1, Out01);
				Out01 = M_VMAdd(Coefb2, HistIn2_1, Out01);
				Out01 = M_VMAdd(Coefa1, HistOut1_1, Out01);
				Out01 = M_VMAdd(Coefa2, HistOut2_1, Out01);
				HistIn2_1 = HistIn1_1;
				HistIn1_1 = Src01;
				HistOut2_1 = HistOut1_1;
				HistOut1_1 = Out01;
				vec128 Out02 = M_VMul(Coefb0, Src02);
				Out02 = M_VMAdd(Coefb1, HistIn1_2, Out02);
				Out02 = M_VMAdd(Coefb2, HistIn2_2, Out02);
				Out02 = M_VMAdd(Coefa1, HistOut1_2, Out02);
				Out02 = M_VMAdd(Coefa2, HistOut2_2, Out02);
				HistIn2_2 = HistIn1_2;
				HistIn1_2 = Src02;
				HistOut2_2 = HistOut1_2;
				HistOut1_2 = Out02;
				vec128 Out03 = M_VMul(Coefb0, Src03);
				Out03 = M_VMAdd(Coefb1, HistIn1_3, Out03);
				Out03 = M_VMAdd(Coefb2, HistIn2_3, Out03);
				Out03 = M_VMAdd(Coefa1, HistOut1_3, Out03);
				Out03 = M_VMAdd(Coefa2, HistOut2_3, Out03);
				HistIn2_3 = HistIn1_3;
				HistIn1_3 = Src03;
				HistOut2_3 = HistOut1_3;
				HistOut1_3 = Out03;

				Coefa1 = M_VAdd(Coefa1, Coefa1Delta);
				Coefa2 = M_VAdd(Coefa2, Coefa2Delta);
				Coefb0 = M_VAdd(Coefb0, Coefb0Delta);
				Coefb1 = M_VAdd(Coefb1, Coefb1Delta);
				Coefb2 = M_VAdd(Coefb2, Coefb2Delta);

				vec128 Out04 = M_VMul(Coefb0, Src04);
				Out04 = M_VMAdd(Coefb1, HistIn1_0, Out04);
				Out04 = M_VMAdd(Coefb2, HistIn2_0, Out04);
				Out04 = M_VMAdd(Coefa1, HistOut1_0, Out04);
				Out04 = M_VMAdd(Coefa2, HistOut2_0, Out04);
				HistIn2_0 = HistIn1_0;
				HistIn1_0 = Src04;
				HistOut2_0 = HistOut1_0;
				HistOut1_0 = Out04;
				vec128 Out05 = M_VMul(Coefb0, Src05);
				Out05 = M_VMAdd(Coefb1, HistIn1_1, Out05);
				Out05 = M_VMAdd(Coefb2, HistIn2_1, Out05);
				Out05 = M_VMAdd(Coefa1, HistOut1_1, Out05);
				Out05 = M_VMAdd(Coefa2, HistOut2_1, Out05);
				HistIn2_1 = HistIn1_1;
				HistIn1_1 = Src05;
				HistOut2_1 = HistOut1_1;
				HistOut1_1 = Out05;
				vec128 Out06 = M_VMul(Coefb0, Src06);
				Out06 = M_VMAdd(Coefb1, HistIn1_2, Out06);
				Out06 = M_VMAdd(Coefb2, HistIn2_2, Out06);
				Out06 = M_VMAdd(Coefa1, HistOut1_2, Out06);
				Out06 = M_VMAdd(Coefa2, HistOut2_2, Out06);
				HistIn2_2 = HistIn1_2;
				HistIn1_2 = Src06;
				HistOut2_2 = HistOut1_2;
				HistOut1_2 = Out06;
				vec128 Out07 = M_VMul(Coefb0, Src07);
				Out07 = M_VMAdd(Coefb1, HistIn1_3, Out07);
				Out07 = M_VMAdd(Coefb2, HistIn2_3, Out07);
				Out07 = M_VMAdd(Coefa1, HistOut1_3, Out07);
				Out07 = M_VMAdd(Coefa2, HistOut2_3, Out07);
				HistIn2_3 = HistIn1_3;
				HistIn1_3 = Src07;
				HistOut2_3 = HistOut1_3;
				HistOut1_3 = Out07;

				Coefa1 = M_VAdd(Coefa1, Coefa1Delta);
				Coefa2 = M_VAdd(Coefa2, Coefa2Delta);
				Coefb0 = M_VAdd(Coefb0, Coefb0Delta);
				Coefb1 = M_VAdd(Coefb1, Coefb1Delta);
				Coefb2 = M_VAdd(Coefb2, Coefb2Delta);

				pDst[0] = Out00;
				pDst[1] = Out01;
				pDst[2] = Out02;
				pDst[3] = Out03;
				pDst[4] = Out04;
				pDst[5] = Out05;
				pDst[6] = Out06;
				pDst[7] = Out07;
				pDst += 8;
				pSrc += 8;
			}

			_pHistoryIn_1[0] = HistIn1_0;
			_pHistoryIn_2[0] = HistIn2_0;
			_pHistoryOut_1[0] = HistOut1_0;
			_pHistoryOut_2[0] = HistOut2_0;
			_pHistoryIn_1[1] = HistIn1_1;
			_pHistoryIn_2[1] = HistIn2_1;
			_pHistoryOut_1[1] = HistOut1_1;
			_pHistoryOut_2[1] = HistOut2_1;
			_pHistoryIn_1[2] = HistIn1_2;
			_pHistoryIn_2[2] = HistIn2_2;
			_pHistoryOut_1[2] = HistOut1_2;
			_pHistoryOut_2[2] = HistOut2_2;
			_pHistoryIn_1[3] = HistIn1_3;
			_pHistoryIn_2[3] = HistIn2_3;
			_pHistoryOut_1[3] = HistOut1_3;
			_pHistoryOut_2[3] = HistOut2_3;
		}
		break;
	}
}


void CSC_Mixer_DSP_BiQuad::ProcessFrame(CSC_Mixer_ProcessingInfo *_pInfo, void *_pInternalData, const void *_pParams, void *_pLastParams)
{

//	M_TRACEALWAYS("CSC_Mixer_DSP_BiQuad %f ", ((fp32 *)_pInfo->m_pDataIn)[0]);

	CParams *pParams = (CParams *)_pParams;
	CLastParams *pLastParams = (CLastParams *)_pLastParams;

	CCoef NewCoef;
	CalcCoef(pParams->m_Type, pParams->m_Frequency, pParams->m_Q, pParams->m_Gain, CSC_Mixer_WorkerContext::ms_pThis->m_SampleRate, NewCoef);

	vec128 * pSrcDst = (vec128 *)_pInfo->m_pDataIn;
	uint32 nChannelVec = ((AlignUp(_pInfo->m_nChannels, 4)) >> 2);
	uint32 nSamples = _pInfo->m_nSamples;

	Internal_ProcessChannelVectors(pSrcDst, pSrcDst, nSamples, nChannelVec, pLastParams->m_Coef, NewCoef,
		pLastParams->m_HistIn_1, pLastParams->m_HistIn_2, pLastParams->m_HistOut_1, pLastParams->m_HistOut_2);

	pLastParams->m_Coef = NewCoef;

}

