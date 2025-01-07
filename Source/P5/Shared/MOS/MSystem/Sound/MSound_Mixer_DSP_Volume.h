
class CSC_Mixer_DSP_Volume
{
public:

	class CParams
	{
	public:
		fp32 m_Volume;
	};

	class CLastParams
	{
	public:
		fp32 m_Volume;
	};

	enum
	{
		EProcessingType = ESC_Mixer_ProcessingType_InPlace,
		ENumInternal = 0,
		ENumParams = sizeof(CParams),
		ENumLastParams = sizeof(CLastParams)
	};

	static void ProcessFrame(CSC_Mixer_ProcessingInfo *_pInfo, void *_pInternalData, const void *_pParams, void *_pLastParams);
	static bint InitData(void *_pInternalData, void *_pParams, void *_pLastParams);
	static void Create(void *_pInternalData, void *_pParams, void *_pLastParams);

};
