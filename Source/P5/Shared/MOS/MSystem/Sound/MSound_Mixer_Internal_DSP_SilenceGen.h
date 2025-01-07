
class CSC_Mixer_DSP_SilenceGen
{
public:

	class CParams
	{
	public:
		uint32 m_nChannels;
	};
	enum
	{
		EProcessingType = ESC_Mixer_ProcessingType_Out,
		ENumInternal = 0,
		ENumParams = sizeof(CParams),
		ENumLastParams = 0
	};

	static void ProcessFrame(CSC_Mixer_ProcessingInfo *_pInfo, void *_pInternalData, const void *_pParams, void *_pLastParams);
	static bint InitData(void *_pInternalData, void *_pParams, void *_pLastParams);
};
