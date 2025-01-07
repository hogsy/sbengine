
/////////////////////////////////////////////////////////////////////////////////////////
// Handles resampling and volume etc

// Todo: Add optimized code for SampleRate = 0.5 case and nChannels == 1 or 2

class CSC_Mixer_DSP_Voice
{
public:
	typedef CSC_Mixer_WorkerContext::CVoice CVoice;

	class CParams
	{
	public:
		TCDynamicPtr<CSC_Mixer_WorkerContext::CCustomPtrHolder, CVoice> m_pSourceVoice;
		fp32 m_SingleVolume;
	};

	class CLastParams
	{
	public:
		fp32 m_SingleVolume;
//		fp32 m_SingleVolumePrim;
	};

	enum
	{
		EProcessingType = ESC_Mixer_ProcessingType_Out,
		ENumInternal = 0,
		ENumParams = sizeof(CParams),
		ENumLastParams = sizeof(CLastParams)
	};

	static void ProcessFrame(CSC_Mixer_ProcessingInfo *_pInfo, void *_pInternalData, const void *_pParams, void *_pLastParams);
	static bint InitData(void *_pInternalData, void *_pParams, void *_pLastParams);
};

