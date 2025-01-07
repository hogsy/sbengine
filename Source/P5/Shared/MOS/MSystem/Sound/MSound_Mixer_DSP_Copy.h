
class CSC_Mixer_DSP_Copy
{
public:

	enum
	{
		EProcessingType = ESC_Mixer_ProcessingType_InPlace,
		ENumInternal = 0,
		ENumParams = 0,
		ENumLastParams = 0
	};

	static void ProcessFrame(CSC_Mixer_ProcessingInfo *_pInfo, void *_pInternalData, const void *_pParams, void *_pLastParams);
};
