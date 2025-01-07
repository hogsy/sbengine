
class CSC_Mixer_DSP_Master
{
public:


	class CParams
	{
	public:
		CSC_Mixer_OuputFrame *m_pOutputFrame;
	};
	enum
	{
		EProcessingType = ESC_Mixer_ProcessingType_In,
		ENumInternal = 0,
		ENumParams = sizeof(CParams),
		ENumLastParams = 0
	};

	static void ProcessFrame(CSC_Mixer_ProcessingInfo *_pInfo, void *_pInternalData, const void *_pParams, void *_pLastParams);
	static void PreProcessFrame(void *_pInternalData, const void *_pParams, const void *_pLastParams) {}
	static bint InitData(void *_pInternalData, void *_pParams, void *_pLastParams);
};
