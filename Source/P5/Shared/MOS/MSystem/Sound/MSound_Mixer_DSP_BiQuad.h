

class CSC_Mixer_DSP_BiQuad
{
public:

	enum EType
	{
		EType_LowPass,
		EType_HighPass,
		EType_BandPass,
		EType_Notch,
		EType_AllPass,
		EType_PeakingEQ,
		EType_ShelvingLow,
		EType_ShelvingHigh,
	};

	class CParams
	{
	public:
		int32 m_Type;
		fp32 m_Frequency;
		fp32 m_Q;
		fp32 m_Gain;
	};

	class CCoef
	{
	public:
		fp32 m_Coef_a1;
		fp32 m_Coef_a2;
		fp32 m_Coef_b0;
		fp32 m_Coef_b1;
		fp32 m_Coef_b2;
	};

	class CLastParams
	{
	public:
		vec128 m_HistIn_1[ESCMixer_MaxChannelVectors];
		vec128 m_HistIn_2[ESCMixer_MaxChannelVectors];
		vec128 m_HistOut_1[ESCMixer_MaxChannelVectors];
		vec128 m_HistOut_2[ESCMixer_MaxChannelVectors];
		CCoef m_Coef;
	};

	enum
	{
		EProcessingType = ESC_Mixer_ProcessingType_InPlace,
		ENumInternal = 0,
		ENumParams = sizeof(CParams),
		ENumLastParams = sizeof(CLastParams)
	};

	static void CalcCoef(int32 _Type, fp32 _Frequency, fp32 _Q, fp32 _Gain, fp32 _SampleFrequency, CCoef &_Coef)
	{
		// fp32 A = M_Pow(10.0f, _Gain / 40.0f);
		fp32 A = _Gain;
		fp32 w0 = (2.0f * _PI * _Frequency) / _SampleFrequency;
		fp32 sn = M_Sin(w0);
		fp32 cs = M_Cos(w0);
		fp32 alpha = sn / (2.0f * _Q);

		fp32 a0;
		fp32 a1;
		fp32 a2;
		fp32 b0;
		fp32 b1;
		fp32 b2;
		switch (_Type)
		{
		default:
		case EType_LowPass:
			{
				a0 = 1 + alpha;
				a1 =  -2.0f*cs;
				a2 =   1.0f - alpha;
				b0 =  (1.0f - cs)/2.0f;
				b1 =   1.0f - cs;
				b2 =  (1.0f - cs)/2.0f;
			}
			break;

		case EType_HighPass:
			{
				a0 = 1 + alpha;
				a1 =  -2.0f*cs;
				a2 =   1.0f - alpha;
				b0 =  (1.0f + cs)/2.0f;
				b1 =   1.0f + cs;
				b2 =  (1.0f + cs)/2.0f;
			}
			break;

		case EType_BandPass:
			{
				b0 = sn/2.0f;
				b1 = 0.0f;
				b2 = -sn/2.0f;
				a0 = 1.0f + alpha;
				a1 = -2.0f*cs;
				a2 = 1.0f - alpha;
			}
			break;

		case EType_Notch:
			{
				b0 = 1.0f;
				b1 = -2.0f*cs;
				b2 = 1.0f;
				a0 = 1.0f + alpha;
				a1 = -2.0f*cs;
				a2 = 1.0f - alpha;
			}
			break;

		case EType_AllPass:
			{
				b0 =   1.0f - alpha;
				b1 =  -2.0f*cs;
				b2 =   1.0f + alpha;
				a0 =   1.0f + alpha;
				a1 =  -2.0f*cs;
				a2 =   1.0f - alpha;
			}
			break;

		case EType_PeakingEQ:
			{
				b0 =   1.0f + alpha*A;
				b1 =  -2.0f*cs;
				b2 =   1.0f - alpha*A;
				a0 =   1.0f + alpha/A;
				a1 =  -2.0f*cs;
				a2 =   1.0f - alpha/A;
			}
			break;

		case EType_ShelvingLow:
			{
				fp32 Temp0 = 2.0f*M_Sqrt(A)*alpha;
				b0 =      A*( (A+1.0f) - (A-1.0f) * cs + Temp0);
				b1 = 2.0f*A*( (A-1.0f) - (A+1.0f) * cs);
				b2 =      A*( (A+1.0f) - (A-1.0f) * cs - Temp0);
				a0 =          (A+1.0f) + (A-1.0f) * cs + Temp0;
				a1 =  -2.0f*( (A-1.0f) + (A+1.0f) * cs);
				a2 =          (A+1.0f) + (A-1.0f) * cs - Temp0;
			}
			break;

		case EType_ShelvingHigh:
			{
				fp32 Temp0 = 2.0f*M_Sqrt(A)*alpha;
				b0 =       A*( (A+1.0f) + (A-1.0f) * cs + Temp0);
				b1 = -2.0f*A*( (A-1.0f) + (A+1.0f) * cs);
				b2 =       A*( (A+1.0f) + (A-1.0f) * cs - Temp0);
				a0 =		    (A+1.0f) - (A-1.0f) * cs + Temp0;
				a1 =    2.0f*( (A-1.0f) - (A+1.0f) * cs);
				a2 =           (A+1.0f) - (A-1.0f) * cs - Temp0;
			}
			break;
		}
		fp32 Inv_a0 = 1.0f / a0;
		_Coef.m_Coef_a1 = -a1 * Inv_a0; // Negate so we can do MAD
		_Coef.m_Coef_a2 = -a2 * Inv_a0; // Negate so we can do MAD
		_Coef.m_Coef_b0 = b0 * Inv_a0;
		_Coef.m_Coef_b1 = b1 * Inv_a0;
		_Coef.m_Coef_b2 = b2 * Inv_a0;
	}
	static void Internal_ProcessChannelVectors(vec128 *_pDst, const vec128 *_pSrc, mint _nSamples, mint _nChannelVec, const CCoef &_StartCoef, const CCoef &_EndCoef,
		vec128 *_pHistoryIn_1, vec128 *_pHistoryIn_2, vec128 *_pHistoryOut_1, vec128 *_pHistoryOut_2);

	static void ProcessFrame(CSC_Mixer_ProcessingInfo *_pInfo, void *_pInternalData, const void *_pParams, void *_pLastParams);
	static bint InitData(void *_pInternalData, void *_pParams, void *_pLastParams);
	static void Create(void *_pInternalData, void *_pParams, void *_pLastParams);

};
