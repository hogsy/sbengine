
/////////////////////////////////////////////////////////////////////////////////////////
// Handles resampling and volume etc

class CSC_Mixer_DSP_Router
{
public:

	enum 
	{
		EMaxRoutes = 16 // Route to a maximum of 16 voices
	};
	typedef CSC_Mixer_WorkerContext::CDSPChainInstanceInternal CDSPChainInstanceInternal;

	class CParams
	{
	public:
		TCDynamicPtr<CSC_Mixer_WorkerContext::CCustomPtrHolder, CDSPChainInstanceInternal> m_pRouteTo[EMaxRoutes];
	};
	enum
	{
		EProcessingType = ESC_Mixer_ProcessingType_In,
		ENumInternal = 0,
		ENumParams = sizeof(CParams),
		ENumLastParams = 0
	};

	static CSC_Mixer_WorkerContext::CMixBinContainer *MixBuffer(CSC_Mixer_WorkerContext *_pMixer, TCDynamicPtr<CSC_Mixer_WorkerContext::CCustomPtrHolder, CSC_Mixer_WorkerContext::CMixBinContainer> &_pIn0, TCDynamicPtr<CSC_Mixer_WorkerContext::CCustomPtrHolder, CSC_Mixer_WorkerContext::CMixBinContainer> &_pIn1, bint _bCanDestroy1);
	static CSC_Mixer_WorkerContext::CMixBinContainer *CopyContainer(CSC_Mixer_WorkerContext *_pMixer, CSC_Mixer_WorkerContext::CMixBinContainer *_pContainer);
	static void PreProcessFrame(void *_pInternalData, const void *_pParams, const void *_pLastParams);
	static void ProcessFrame(CSC_Mixer_ProcessingInfo *_pInfo, void *_pInternalData, const void *_pParams, void *_pLastParams);
	static bint InitData(void *_pInternalData, void *_pParams, void *_pLastParams);
};
