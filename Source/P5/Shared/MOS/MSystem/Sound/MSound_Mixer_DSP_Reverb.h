
class CSC_Mixer_DSP_Reverb
{
public:

	class CParams
	{
	public:
		// Constant params
		uint32 m_nDestinationChannels;
		fp32 m_MaxDelay;

		// Dynamic params
		fp32 m_Wet;
		fp32 m_Gain;
		fp32 m_RoomSize;
		fp32 m_Damp;
		fp32 m_Width;
		fp32 m_Spread;
	};

	enum
	{
		EMaxChannels = 8
	};
	class CInternal
	{
	public:
		uint32 m_nDestinationChannels;
		fp32 m_MaxDelay;

		uint32 m_BufferSize;
		void *m_pDynamicMemory;

		class CSecondOrderIIR
		{
		public:
//			fp32 m_Coef_a0;
			fp32 m_Coef_a1;
			fp32 m_Coef_a2;
			fp32 m_Coef_b0;
			fp32 m_Coef_b1;
			fp32 m_Coef_b2;

			fp32 m_HistIn_1;
			fp32 m_HistIn_2;

			fp32 m_HistOut_1;
			fp32 m_HistOut_2;

			void SetCoef(fp32 _a0, fp32 _a1, fp32 _a2, fp32 _b0, fp32 _b1, fp32 _b2)
			{
				fp32 Inv_a0 = 1.0f / _a0;
				m_Coef_a1 = _a1 * Inv_a0;
				m_Coef_a2 = _a2 * Inv_a0;
				m_Coef_b0 = _b0 * Inv_a0;
				m_Coef_b1 = _b1 * Inv_a0;
				m_Coef_b2 = _b2 * Inv_a0;
			}

			fp32 Calculate(fp32 _In)
			{
				fp32 Output = m_Coef_b0 * _In + m_Coef_b1*m_HistIn_1 + m_Coef_b2*m_HistIn_2 - m_Coef_a1*m_HistOut_1 - m_Coef_a2*m_HistOut_2;

				m_HistIn_2 = m_HistIn_1;
				m_HistIn_1 = _In;

				m_HistOut_2 = m_HistOut_1;
				m_HistOut_1 = Output;

				return Output;			
			}
		};

		class CDelayLine
		{
		public:
			fp32 *m_pBuffer;
			uint32 m_nSamples;
			uint32 m_iBuffer;
			void Init()
			{
				m_pBuffer = 0;
				m_nSamples = 0;
				m_iBuffer = 0;
			}
		};

		class CFilter_Comb : public CDelayLine
		{
		public:
			fp32 m_FilterStore;
			void Init()
			{
				CDelayLine::Init();
				m_FilterStore = 0.0f;
			}

			fp32 Calculate(fp32 _In, fp32 _Damp0, fp32 _Damp1, fp32 _Feedback)
			{
				fp32 Out;
				uint32 iBuf = m_iBuffer;
				Out = m_pBuffer[iBuf];
				fp32 FilterStore = (Out*_Damp1) + (m_FilterStore*_Damp0);

				m_pBuffer[iBuf] = _In + (FilterStore * _Feedback);
				m_FilterStore = FilterStore;

				++iBuf;
				if (iBuf == m_nSamples)
					iBuf = 0;
				m_iBuffer = iBuf;

				return Out;
			}
		};

		CFilter_Comb m_TestCombs[3];
		
		class CFilter_DelayAllpass : public CDelayLine
		{
		public:
			void Init()
			{
				CDelayLine::Init();
			}
			fp32 Calculate(fp32 _In, fp32 _Feedback)
			{
				float Out;
				float BufOut;
				
				uint32 iBuf = m_iBuffer;
				BufOut = m_pBuffer[iBuf];

				Out = -_In + BufOut;
				m_pBuffer[iBuf] = _In + (BufOut*_Feedback);

				++iBuf;
				if (iBuf == m_nSamples)
					iBuf = 0;
				m_iBuffer = iBuf;

				return Out;
			}
		};

		CFilter_DelayAllpass m_TestDelayAllPass;

		class CFilter_LowPass : public CSecondOrderIIR
		{
		public:
			void CalcParams(fp32 _Frequency, fp32 _Gain, fp32 _Q, fp32 _SampleFrequency)
			{
//				fp32 A = M_Pow(10.0f, _Gain / 40.0f);
				fp32 w0 = (2.0f * _PI * _Frequency) / _SampleFrequency;
				fp32 sn = M_Sin(w0);
				fp32 cs = M_Cos(w0);
				fp32 alpha = sn / (2.0f * _Q);

				fp32 a0 = 1 + alpha;
				fp32 a1 =  -2.0f*cs;
				fp32 a2 =   1.0f - alpha;
				fp32 b0 =  (1.0f - cs)/2.0f;
				fp32 b1 =   1.0f - cs;
				fp32 b2 =  (1.0f - cs)/2.0f;
				SetCoef(a0, a1, a2, b0, b1, b2);
			}
		};

		class CFilter_HighPass : public CSecondOrderIIR
		{
		public:
			void CalcParams(fp32 _Frequency, fp32 _Gain, fp32 _Q, fp32 _SampleFrequency)
			{
//				fp32 A = M_Pow(10.0f, _Gain / 40.0f);
				fp32 w0 = (2.0f * _PI * _Frequency) / _SampleFrequency;
				fp32 sn = M_Sin(w0);
				fp32 cs = M_Cos(w0);
				fp32 alpha = sn / (2.0f * _Q);

				fp32 a0 = 1 + alpha;
				fp32 a1 =  -2.0f*cs;
				fp32 a2 =   1.0f - alpha;
				fp32 b0 =  (1.0f + cs)/2.0f;
				fp32 b1 =   1.0f + cs;
				fp32 b2 =  (1.0f + cs)/2.0f;
				SetCoef(a0, a1, a2, b0, b1, b2);
			}
		};

		class CFilter_BandPass : public CSecondOrderIIR
		{
		public:
			void CalcParams(fp32 _Frequency, fp32 _Gain, fp32 _Q, fp32 _SampleFrequency)
			{
//				fp32 A = M_Pow(10.0f, _Gain / 40.0f);
				fp32 w0 = (2.0f * _PI * _Frequency) / _SampleFrequency;
				fp32 sn = M_Sin(w0);
				fp32 cs = M_Cos(w0);
				fp32 alpha = sn / (2.0f * _Q);

				fp32 b0 = sn/2.0f;
				fp32 b1 = 0.0f;
				fp32 b2 = -sn/2.0f;
				fp32 a0 = 1.0f + alpha;
				fp32 a1 = -2.0f*cs;
				fp32 a2 = 1.0f - alpha;

				SetCoef(a0, a1, a2, b0, b1, b2);
			}
		};


		class CFilter_Notch : public CSecondOrderIIR
		{
		public:
			void CalcParams(fp32 _Frequency, fp32 _Gain, fp32 _Q, fp32 _SampleFrequency)
			{
//				fp32 A = M_Pow(10.0f, _Gain / 40.0f);
				fp32 w0 = (2.0f * _PI * _Frequency) / _SampleFrequency;
				fp32 sn = M_Sin(w0);
				fp32 cs = M_Cos(w0);
				fp32 alpha = sn / (2.0f * _Q);

				fp32 b0 = 1.0f;
				fp32 b1 = -2.0f*cs;
				fp32 b2 = 1.0f;
				fp32 a0 = 1.0f + alpha;
				fp32 a1 = -2.0f*cs;
				fp32 a2 = 1.0f - alpha;

				SetCoef(a0, a1, a2, b0, b1, b2);
			}
		};

		class CFilter_AllPass : public CSecondOrderIIR
		{
		public:
			void CalcParams(fp32 _Frequency, fp32 _Gain, fp32 _Q, fp32 _SampleFrequency)
			{
//				fp32 A = M_Pow(10.0f, _Gain / 40.0f);
				fp32 w0 = (2.0f * _PI * _Frequency) / _SampleFrequency;
				fp32 sn = M_Sin(w0);
				fp32 cs = M_Cos(w0);
				fp32 alpha = sn / (2.0f * _Q);

				fp32 b0 =   1.0f - alpha;
				fp32 b1 =  -2.0f*cs;
				fp32 b2 =   1.0f + alpha;
				fp32 a0 =   1.0f + alpha;
				fp32 a1 =  -2.0f*cs;
				fp32 a2 =   1.0f - alpha;

				SetCoef(a0, a1, a2, b0, b1, b2);
			}
		};

		class CFilter_PeakingEQ : public CSecondOrderIIR
		{
		public:
			void CalcParams(fp32 _Frequency, fp32 _Gain, fp32 _Q, fp32 _SampleFrequency)
			{
				fp32 A = M_Pow(10.0f, _Gain / 40.0f);
				fp32 w0 = (2.0f * _PI * _Frequency) / _SampleFrequency;
				fp32 sn = M_Sin(w0);
				fp32 cs = M_Cos(w0);
				fp32 alpha = sn / (2.0f * _Q);

				fp32 b0 =   1.0f + alpha*A;
				fp32 b1 =  -2.0f*cs;
				fp32 b2 =   1.0f - alpha*A;
				fp32 a0 =   1.0f + alpha/A;
				fp32 a1 =  -2.0f*cs;
				fp32 a2 =   1.0f - alpha/A;

				SetCoef(a0, a1, a2, b0, b1, b2);
			}
		};

		class CFilter_ShelvingLow : public CSecondOrderIIR
		{
		public:
			void CalcParams(fp32 _Frequency, fp32 _Gain, fp32 _Q, fp32 _SampleFrequency)
			{
				fp32 A = M_Pow(10.0f, _Gain / 40.0f);
				fp32 w0 = (2.0f * _PI * _Frequency) / _SampleFrequency;
				fp32 sn = M_Sin(w0);
				fp32 cs = M_Cos(w0);
				fp32 alpha = sn / (2.0f * _Q);
				fp32 Temp0 = 2.0f*M_Sqrt(A)*alpha;

				fp32 b0 =      A*( (A+1.0f) - (A-1.0f) * cs + Temp0);
				fp32 b1 = 2.0f*A*( (A-1.0f) - (A+1.0f) * cs);
				fp32 b2 =      A*( (A+1.0f) - (A-1.0f) * cs - Temp0);
				fp32 a0 =          (A+1.0f) + (A-1.0f) * cs + Temp0;
				fp32 a1 =  -2.0f*( (A-1.0f) + (A+1.0f) * cs);
				fp32 a2 =          (A+1.0f) + (A-1.0f) * cs - Temp0;

				SetCoef(a0, a1, a2, b0, b1, b2);
			}
		};


		class CFilter_ShelvingHigh : public CSecondOrderIIR
		{
		public:
			void CalcParams(fp32 _Frequency, fp32 _Gain, fp32 _Q, fp32 _SampleFrequency)
			{
				fp32 A = M_Pow(10.0f, _Gain / 40.0f);
				fp32 w0 = (2.0f * _PI * _Frequency) / _SampleFrequency;
				fp32 sn = M_Sin(w0);
				fp32 cs = M_Cos(w0);
				fp32 alpha = sn / (2.0f * _Q);
				fp32 Temp0 = 2.0f*M_Sqrt(A)*alpha;

				fp32 b0 =       A*( (A+1.0f) + (A-1.0f) * cs + Temp0);
				fp32 b1 = -2.0f*A*( (A-1.0f) + (A+1.0f) * cs);
				fp32 b2 =       A*( (A+1.0f) + (A-1.0f) * cs - Temp0);
				fp32 a0 =		    (A+1.0f) - (A-1.0f) * cs + Temp0;
				fp32 a1 =    2.0f*( (A-1.0f) - (A+1.0f) * cs);
				fp32 a2 =           (A+1.0f) - (A-1.0f) * cs - Temp0;

				SetCoef(a0, a1, a2, b0, b1, b2);
			}
		};

	};

	class CLastParams
	{
	public:
	};

	enum
	{
		EProcessingType = ESC_Mixer_ProcessingType_InOut,
		ENumInternal = sizeof(CInternal),
		ENumParams = sizeof(CParams),
		ENumLastParams = sizeof(CLastParams)
	};

	static void ProcessFrame(CSC_Mixer_ProcessingInfo *_pInfo, void *_pInternalData, const void *_pParams, void *_pLastParams);
	static bint InitData(void *_pInternalData, void *_pParams, void *_pLastParams);
	static void DestroyData(void *_pInternalData, void *_pParams, void *_pLastParams);
	static void Create(void *_pInternalData, void *_pParams, void *_pLastParams);

};

