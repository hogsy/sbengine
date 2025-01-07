
class CSC_Mixer_DSP_VolumeMatrix
{
public:

	class CParams
	{
	public:
		uint32 m_nSourceChannels; // Cannot be changed
		uint32 m_nDestChannels; // Cannot be changed
		TCDynamicPtr<CSC_Mixer_WorkerContext::CCustomPtrHolder, vec128> m_pVolumeMatrix; // Pointer to a volume matrix
	};

	class CLastParams
	{
	public:
		TCDynamicPtr<CSC_Mixer_WorkerContext::CCustomPtrHolder, vec128> m_pVolumeMatrix; // Pointer to a volume matrix
	};

	enum
	{
		EProcessingType = ESC_Mixer_ProcessingType_InOut,
		ENumInternal = 0,
		ENumParams = sizeof(CParams),
		ENumLastParams = sizeof(CLastParams)
	};

	static void ProcessFrame(CSC_Mixer_ProcessingInfo *_pInfo, void *_pInternalData, const void *_pParams, void *_pLastParams);
	static bint InitData(void *_pInternalData, void *_pParams, void *_pLastParams);
	static void DestroyData(void *_pInternalData, void *_pParams, void *_pLastParams);
	static void CopyData(void *_pParams, void *_pLastParams, void *_pInternal, void *_SourceData, void *_DestData);

};
