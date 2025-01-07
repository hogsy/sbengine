
#include "../MSound_Mixer.h"

class SYSTEMDLLEXPORT CSoundContext_Mixer : public CSoundContext
{
public:
	NThread::CMutual M_ALIGN(128) m_InterfaceLock;

	class CSC_DSPChainInstance;
	class CSC_DSPChain;
	class CVoice;

	const static int mcs_MaxDebugSounds = 384;
	char ** m_DebugStrTempMixer;

	enum 
	{
		EPauseSlot_Channel = M_Bit(31),
		EPauseSlot_Delayed = M_Bit(30),
		// Lower bits is reserved for client
	};

	CSC_Mixer m_Mixer;


	// Sound categories
	enum
	{
		ECategoryOverride_Priority = M_Bit(0),
		ECategoryOverride_DSPID = M_Bit(1),
		ECategoryOverride_Pitch = M_Bit(2),
		ECategoryOverride_Volume = M_Bit(3),
		ECategoryOverride_MinDelay = M_Bit(4),
		ECategoryOverride_Spatialization = M_Bit(5),
		ECategoryOverride_Doppler = M_Bit(6),
		ECategoryOverride_Falloff = M_Bit(7),
		ECategoryOverride_Loop = M_Bit(8),
		ECategoryOverride_IsMusic = M_Bit(9),
		ECategoryOverride_MinDist = M_Bit(10),
		ECategoryOverride_MaxDist = M_Bit(11),
		ECategoryOverride_DirectFalloff = M_Bit(12),
		ECategoryOverride_ReverbFalloff = M_Bit(13),
	};

	class CSoundCategory
	{
	public:
		CSoundCategory()
		{
			Clear();
		}
		CSC_VoiceCreateParams m_Params;
		uint32 m_WhatToOverride;
		uint32 m_ReloadSequence;
		CSC_VoiceFalloff m_DirectFalloff;
		CSC_VoiceFalloff m_ReverbFalloff;
		CStr m_Name;

		void Clear()
		{
			m_WhatToOverride = 0;
			m_ReloadSequence = 0;
			m_Params.Clear();
			m_DirectFalloff.m_Values.Clear();
			m_ReverbFalloff.m_Values.Clear();
			m_Name.Clear();
		}

		class CCompare
		{
		public:
			static aint Compare(CSoundCategory *_pFirst, CSoundCategory *_pSecond, void *_pContext)
			{
				if (_pFirst->m_Params.m_Category > _pSecond->m_Params.m_Category)
					return 1;
				else if (_pFirst->m_Params.m_Category < _pSecond->m_Params.m_Category)
					return -1;
				return 0;
			}

			static aint Compare(CSoundCategory *_pFirst, uint32 _Key, void *_pContext)
			{
				if (_pFirst->m_Params.m_Category > _Key)
					return 1;
				else if (_pFirst->m_Params.m_Category < _Key)
					return -1;
				return 0;
			}
		};

		DIdsTreeAVLAligned_Link(CSoundCategory, m_TreeLink, uint32, CCompare);

		class CCompareName
		{
		public:
			static aint Compare(CSoundCategory *_pFirst, CSoundCategory *_pSecond, void *_pContext)
			{
				return _pFirst->m_Name.CompareNoCase(_pSecond->m_Name);
			}

			static aint Compare(CSoundCategory *_pFirst, const CStr &_Key, void *_pContext)
			{
				return _pFirst->m_Name.CompareNoCase(_Key);
			}
		};

		DIdsTreeAVLAligned_Link(CSoundCategory, m_TreeLinkName, CStr, CCompareName);
		DLinkDS_Link(CSoundCategory, m_LinkTemp);
	};

	TCPool<CSoundCategory> m_Pool_SoundCategory;
	DIdsTreeAVLAligned_Tree(CSoundCategory, m_TreeLink, uint32, CSoundCategory::CCompare) m_SoundCategories;
	DIdsTreeAVLAligned_Tree(CSoundCategory, m_TreeLinkName, CStr, CSoundCategory::CCompareName) m_SoundCategoriesName;
	NThread::CMutual m_SoundCategoriesLock;

	///////////////////////////////////////////////////////////////////
	// DSP types


	enum EDSPBindingGlobal
	{
		EDSPBindingGlobal_NumChannels,
		EDSPBindingGlobal_Max
	};

	enum EDSPBindingVoice
	{
		EDSPBindingVoice_NumChannels,
		EDSPBindingVoice_3DVolumeMatrix,
		EDSPBindingVoice_Obstruction,
		EDSPBindingVoice_Occlusion,
		EDSPBindingVoice_Max
	};

	class CBindingEntry
	{
	public:
		DLinkDS_Link(CBindingEntry, m_Link);
		CSC_DSPChain *m_pChain;
		uint32 m_iType:4;
		uint32 m_iDSP:4;
		uint32 m_iBinding:8;
		uint32 m_iParam:16;
		fp32 m_Mul;
		fp32 m_Add;
		void operator =(CBindingEntry &_Other)
		{
			m_pChain = _Other.m_pChain;
			m_iType = _Other.m_iType;
			m_iDSP = _Other.m_iDSP;
			m_iBinding = _Other.m_iBinding;
			m_iParam = _Other.m_iParam;
			m_Mul = _Other.m_Mul;
			m_Add = _Other.m_Add;
			
			// Link
			m_Link.m_Link.SetPrev(_Other.m_Link.m_Link.GetPrev());
			m_Link.m_Link.SetNext(_Other.m_Link.m_Link.GetNext());
			_Other.m_Link.m_Link.GetPrev()->SetNext(&m_Link.m_Link);
			if (!_Other.m_Link.m_Link.GetNext ()->IsListLink())
				_Other.m_Link.m_Link.GetNext()->SetPrev(&m_Link.m_Link);
			_Other.m_Link.m_Link.Construct();
		}
	};

	typedef DLinkDS_Iter(CBindingEntry, m_Link) CBindingEntryIter;

	class CBinding
	{
	public:
		DLinkDS_List(CBindingEntry, m_Link) m_Entries;
	};

	CBinding m_Bindings[EDSPBindingGlobal_Max];

	
	void Internal_SetBind(uint32 _Type, CSC_Mixer_DSPChainInstance *_pChainInstance, uint32 _iDSP, uint32 _iParam, fp32 _Value);
	void Internal_SetBind(uint32 _Type, CSC_Mixer_DSPChainInstance *_pChainInstance, uint32 _iDSP, uint32 _iParam, int32 _Value);
	void Internal_SetBind(uint32 _Type, CSC_Mixer_DSPChainInstance *_pChainInstance, uint32 _iDSP, uint32 _iParam, uint16 *_pVolumeMatrix);
	void Internal_GlobalBindingChanged(uint32 _Binding, fp32 _Value); // EDSPBindingGlobal
	void Internal_GlobalBindingChanged(uint32 _Binding, int32 _Value); // EDSPBindingGlobal
	void Internal_GlobalBindingChanged(uint32 _Binding, uint16 *_pValue); // EDSPBindingGlobal

	void Internal_UpdateVoiceBindings(CVoice *_pVoice);

	void Internal_VoiceBindingChanged(CVoice *_pVoice, uint32 _Binding, fp32 _Value);
	void Internal_VoiceBindingChanged(CVoice *_pVoice, uint32 _Binding, int32 _Value);
	void Internal_VoiceBindingChanged(CVoice *_pVoice, uint32 _Binding, uint16 *_pValue);

	enum EDSPType
	{
		EDSPType_Output,
		EDSPType_Input,
		EDSPType_InputOutput,
	};

	enum EDSPParamType
	{
		EDSPParamType_mint,
		EDSPParamType_VPUPtr,
		EDSPParamType_int32,
		EDSPParamType_fp32,
		EDSPParamType_Route,
		EDSPParamType_VolumeMatrix,
	};

	enum EDSPParamInterpolationType
	{
		EDSPParamInterpolationType_Linear,
		EDSPParamInterpolationType_Exponent,
		EDSPParamInterpolationType_Log
	};

	class CSC_DSP
	{
	public:
		CStr m_Name;
		uint32 m_DSPID;
		uint32 m_Type;
		uint32 m_ParamSize;

		CSC_DSP()
		{
			m_ParamSize = 0;
		}

		~CSC_DSP()
		{
			m_ParamTree.f_DeleteAll();
		}

		class CParam
		{
		public:
			CParam()
			{
				m_InterpolationType = EDSPParamInterpolationType_Linear;
			}

			CStr m_Name;
			uint32 m_Type;
			uint32 m_Index;
			uint32 m_InterpolationType;
			union
			{
				int32 m_Min_int32;
				fp32 m_Min_fp32;
			};
			union
			{
				int32 m_Max_int32;
				fp32 m_Max_fp32;
			};
			union
			{
				int32 m_Default_int32;
				fp32 m_Default_fp32;
			};

			class CCompare
			{
			public:
				DIdsPInlineS static aint Compare(const CParam *_pFirst, const CParam *_pSecond, void *_pContext)
				{
					return _pFirst->m_Name.CompareNoCase(_pSecond->m_Name);
				}

				DIdsPInlineS static aint Compare(const CParam *_pTest, const CStr &_Key, void *_pContext)
				{
					return _pTest->m_Name.CompareNoCase(_Key);
				}
			};
			DAVLAligned_Link(CParam, m_TreeLink, CStr, CCompare);

		};


		class CCompare
		{
		public:
			DIdsPInlineS static aint Compare(const CSC_DSP *_pFirst, const CSC_DSP *_pSecond, void *_pContext)
			{
				return _pFirst->m_Name.CompareNoCase(_pSecond->m_Name);
			}

			DIdsPInlineS static aint Compare(const CSC_DSP *_pTest, const CStr &_Key, void *_pContext)
			{
				return _pTest->m_Name.CompareNoCase(_Key);
			}
		};
		DAVLAligned_Link(CSC_DSP, m_TreeLink, CStr, CCompare);

		DAVLAligned_Tree(CParam, m_TreeLink, CStr, CParam::CCompare) m_ParamTree;

		CParam *GetParam(const CStr&_Name)
		{
			return m_ParamTree.FindEqual(_Name);
		}
	};

	DAVLAligned_Tree(CSC_DSP, m_TreeLink, CStr, CSC_DSP::CCompare) m_DSPTree;

	CSC_DSP *Internal_GetDSP(const CStr &_Name)
	{
		return m_DSPTree.FindEqual(_Name);
	}

	class CParamValue : public CReferenceCount
	{
	public:
		uint32 m_Type;
		virtual ~CParamValue()
		{

		}		
	};

	typedef TPtr<CParamValue> spCParamValue;

	class CParamValue_int32 : public CParamValue
	{
	public:
		CParamValue_int32()
		{
			m_Type = EDSPParamType_int32;
		}
		int32 m_Value;
	};

	class CParamValue_fp32 : public CParamValue
	{
	public:
		CParamValue_fp32()
		{
			m_Type = EDSPParamType_fp32;
		}
		fp32 m_Value;
	};


	class CParamValue_Route : public CParamValue
	{
	public:
		CParamValue_Route()
		{
			m_Type = EDSPParamType_Route;
		}
		TCDynamicPtr<CSC_Mixer_WorkerContext::CCustomPtrHolder, CSC_Mixer_DSPChainInstance> m_Value;
	};

	class CParamValue_VolumeMatrix : public CParamValue
	{
	public:
		CParamValue_VolumeMatrix()
		{
			m_Type = EDSPParamType_VolumeMatrix;
		}
		TThinArray< TThinArray< TThinArrayAlign<uint16, sizeof(vec128)> > > m_Value;
	};


	class CSC_DSPChainInstance
	{
	public:
		CSC_DSPChainInstance()
		{
			m_pMixerDSPChainInstance = NULL;
			m_pDSPChain = NULL;
		}
		~CSC_DSPChainInstance()
		{
		}

		CSC_DSPChain *m_pDSPChain;
		CSC_Mixer_DSPChainInstance *m_pMixerDSPChainInstance;

		DLinkDS_Link(CSC_DSPChainInstance, m_Link);

		void Destroy(CSoundContext_Mixer *_pSC)
		{
			if (m_pMixerDSPChainInstance)
				_pSC->m_Mixer.DSPChainInstance_Destroy(m_pMixerDSPChainInstance);
			m_pMixerDSPChainInstance = NULL;
		}
	};

	typedef DLinkDS_Iter(CSC_DSPChainInstance, m_Link) CChainInstanceIter;


	class CSC_DSPChain
	{
	public:
		CSC_DSPChain()
		{
			m_pGlobalChain = NULL;
			m_pMixerDSPChain = NULL;
		}
		~CSC_DSPChain()
		{
			if (m_pGlobalChain)
				delete m_pGlobalChain;

			m_BindingEntries.Clear();
			m_ChainInstances.DeleteAll();
		}
		CSC_DSPChainInstance *m_pGlobalChain;
		CStr m_Name;
		uint32 m_NameHash;
		CSC_Mixer_DSPChain *m_pMixerDSPChain;
		DLinkDS_List(CSC_DSPChainInstance, m_Link) m_ChainInstances;

		TThinArray<CBindingEntry> m_BindingEntries;
		CBinding m_VoiceBindings[EDSPBindingVoice_Max];

		class CDSPInfo
		{
		public:
			CSC_Mixer_DSPInstance *m_pMixerDSP;
			CSC_DSP *m_pDSP;

		};
		TThinArray<CDSPInfo> m_DSPs;
		
		class CDefaultParams
		{
		public:
			uint32 m_iDSP:16;
			uint32 m_iParam:16;
			spCParamValue m_spValue;
		};

		TThinArray<CDefaultParams> m_DefaultParams;

		class CCompare
		{
		public:
			DIdsPInlineS static aint Compare(const CSC_DSPChain *_pFirst, const CSC_DSPChain *_pSecond, void *_pContext)
			{
				if (_pFirst->m_NameHash > _pSecond->m_NameHash)
					return 1;
				else if (_pFirst->m_NameHash < _pSecond->m_NameHash)
					return -1;
				return 0;
			}

			DIdsPInlineS static aint Compare(const CSC_DSPChain *_pTest, uint32 _Hash, void *_pContext)
			{
				if (_pTest->m_NameHash > _Hash)
					return 1;
				else if (_pTest->m_NameHash < _Hash)
					return -1;
				return 0;
			}
		};
		DAVLAligned_Link(CSC_DSPChain, m_TreeLink, uint32, CCompare);

		void Destroy(CSoundContext_Mixer *_pSC)
		{
			m_DefaultParams.Clear();
			m_BindingEntries.Clear();
			if (m_pGlobalChain)
			{
				m_pGlobalChain->Destroy(_pSC);
				m_pGlobalChain = NULL;
			}
			if (m_pMixerDSPChain)
				_pSC->m_Mixer.DSPChain_Destroy(m_pMixerDSPChain);
			m_pMixerDSPChain = NULL;
		}
	};


	///////////////////////////////////////////////////////////////////////////////////////

	class CChannel;
	class CVoice
	{
	public:
		CVoice()
		{
			m_iMixerVoice = 0xffffffff;
			m_pChannel = NULL;
			Clear();
		}

		void Clear()
		{
			m_DSPChainInstance.m_pDSPChain = NULL;
			m_DSPChainInstance.m_pMixerDSPChainInstance = NULL;
			m_DSPChainInstance.m_Link.Unlink();

			m_Link3DUpdate.Unlink();

			m_PauseSlot = 0;
			m_Volume = 1.0f;
			m_Volume_3D = 1.0f;
			m_Pitch = 1.0f;
			m_Pitch_3D = 1.0f;
			m_hVoice = 0;
			m_Priority = 0;
			m_WaveID = -1;
			m_OriginalSampleRate = 1.0f;
			m_bLooping = false;
			m_StartTime.Reset();
		}

		class CCompare
		{
		public:
			DIdsPInlineS static aint Compare(const CVoice *_pFirst, const CVoice *_pSecond, void *_pContext)
			{
				if (_pFirst->m_hVoice < _pSecond->m_hVoice)
					return -1;
				if (_pFirst->m_hVoice > _pSecond->m_hVoice)
					return 1;
				return 0;
			}

			DIdsPInlineS static aint Compare(const CVoice *_pTest, uint32 _Key, void *_pContext)
			{
				if (_pTest->m_hVoice < _Key)
					return -1;
				if (_pTest->m_hVoice > _Key)
					return 1;
				return 0;
			}
		};

		DAVLAligned_Link(CVoice, m_VoiceHandleLink, uint32, CCompare);
		DLinkDS_Link(CVoice, m_LinkChannel);

		CSC_DSPChainInstance m_DSPChainInstance;
		CMTime m_StartTime;
		CChannel *m_pChannel;
		fp32 m_OriginalSampleRate;
		fp32 m_Pitch;
		fp32 m_Pitch_3D;
		fp32 m_Volume;
		fp32 m_Volume_3D;
		uint32 m_hVoice;
		uint32 m_iMixerVoice;
		uint32 m_PauseSlot;
		uint32 m_Priority;
		uint32 m_bLooping;
		uint32 m_WaveID:16;
		uint32 m_nChannels:5;
		uint32 m_Spatialization:4;
		uint32 m_bFalloff:1;
		uint32 m_bDoppler:1;
		uint32 m_bIsMusic:1;

		// 3D Properties

		DLinkD_Link(CVoice, m_Link3DUpdate);

		CVec3Dfp32 m_Position;
		CVec3Dfp32 m_Velocity;
		CVec3Dfp32 m_Orientation;
		CSC_3DOcclusion m_Occlusion;
		fp32 m_MinDist;
		fp32 m_MaxDist;
		CSC_VoiceFalloff *m_pDirectFalloff;
		CSC_VoiceFalloff *m_pReverbFalloff;
//		CQuatfp32 m_Orientation;

	};

	class CChannel
	{
	public:
		CChannel()
		{
			m_Volume = 1.0f;
			m_Pitch = 1.0f;
			m_nFreeVoices = 0;
			m_nVoices = 0;
		}

		fp32 m_Volume;
		fp32 m_Pitch;
		mint m_nFreeVoices;
		mint m_nVoices;
		DLinkDS_List(CVoice, m_LinkChannel) m_FreeVoices;
		DLinkDS_List(CVoice, m_LinkChannel) m_AlloctedVoices;
		DLinkDS_Link(CChannel, m_Link);
	};


	DLinkDS_List(CVoice, m_LinkChannel) m_FreeVoices; // Voices not allocated to any channel
	typedef DLinkDS_Iter(CVoice, m_LinkChannel) CVoiceIter;
	mint m_nFreeVoices;

	DLinkDS_List(CChannel, m_Link) m_Channels; // Voices not allocated to any channel
	typedef DLinkDS_Iter(CChannel, m_Link) CChannelIter;

	DAVLAligned_Tree(CVoice, m_VoiceHandleLink, uint32, CVoice::CCompare) m_VoiceHandleTree;

	TThinArray<CChannel *> m_lChannels;
	TThinArray<CVoice> m_lVoices;

	TPtr<CWaveContext> m_spWaveContext;
	TPtr<CSC_WCIDInfo> m_spWCIDInfo;	// WaveID info
	int32 m_iWC;
	uint32 m_bAddedToConsole;
	uint32 m_iCurrentVoiceHandle;

	fp32 m_LastSetSampleRate;
	fp32 m_LastSetSampleRateTraget;
	CMTime m_Drift_LastCPUTime;
	CMTime m_Drift_LastSampleTime;

	fp32 m_LFEScale;
	fp32 m_MusicVolume_TotalPrim;
	fp32 m_MusicVolume_Total;
	fp32 m_MusicVolume_TotalLast;
	fp32 m_MusicVolume_System;
	fp32 m_MusicVolume_Settings;
	fp32 m_MusicVolume_Game;

	void Music_SetSystemVolume(fp32 _Volume)
	{
		m_MusicVolume_System = _Volume;
	}
	void Music_SetSettingsVolume(fp32 _Volume)
	{
		m_MusicVolume_Settings = _Volume;
	}
	void Music_SetGameVolume(fp32 _Volume)
	{
		m_MusicVolume_Game = _Volume;
	}

	void Internal_UpdateMusicVolume();

	// 3D

	CMat4Dfp32 m_Listener_Matrix;
	CVec3Dfp32 m_Listener_Velocity;
	fp32 m_CoordinateScale;
	int32 m_bUpdateAll3D;

	DLinkD_List(CVoice, m_Link3DUpdate) m_Need3DUpdate;
	typedef DLinkDS_Iter(CVoice, m_LinkChannel) CNeed3DUpdateIter;

	// External interfaces
	CSC_LipSyncManager m_LipSyncManager;
	CSC_DualStream m_DualStream;

#ifndef PLATFORM_CONSOLE
	int m_bLogUsage;
#endif

	typedef DAVLAligned_Tree(CSC_DSPChain, m_TreeLink, uint32, CSC_DSPChain::CCompare) CDSPChainTree;
	typedef DIdsTreeAVLAligned_Iterator(CSC_DSPChain, m_TreeLink, uint32, CSC_DSPChain::CCompare) CDSPChainTreeIter;
	CDSPChainTree m_DSPChainTreeGlobal;
	CDSPChainTree m_DSPChainTreeVoice;

	CSC_DSPChain *Internal_GetGlobalChainByName(CStr _Name);
	CSC_DSPChain *Internal_GetGlobalChainByHash(uint32 _Name);
	CSC_DSPChain *Internal_GetVoiceChainByName(CStr _Name);
	CSC_DSPChain *Internal_GetVoiceChainByHash(uint32 _Name);

	CSC_DSPChain *m_pDefault2DChain;
	CSC_DSPChain *m_pDefault3DChain;

	CSoundContext_Mixer();
	~CSoundContext_Mixer();

	int32 Internal_TranslateDSPValue_int32(const CStr &_Value);
	fp32 Internal_TranslateDSPValue_fp32(const CStr &_Value);
	CSoundContext_Mixer::CSC_DSPChainInstance *Internal_TranslateDSPValue_Route(const CStr &_Value);
	void Internal_TranslateDSPValue_VolumeMatrix(const CRegistry &_Value, TThinArray< TThinArray< TThinArrayAlign<uint16, sizeof(vec128)> > > &_Volumes);

	void Internal_VoicePause(CVoice *_pVoice, uint32 _PauseFlags);
	void Internal_VoiceUnpause(CVoice *_pVoice, uint32 _PauseFlags);
	void Internal_VoiceUpdatePitch(CVoice *_pVoice);
	void Internal_VoiceUpdateVolume(CVoice *_pVoice, bint _bInterpolate);
	void Internal_VoiceDestroy(CVoice *_pVoice);
	void Internal_VoiceUpdate3D(CVoice *_pVoice, bint _bUpdateMixer);

	void Internal_VoiceUpdateBindings(CVoice *_pVoice, bint _bUpdateMixer);

	bint Internal_VoiceIsPlaying(CVoice *_pVoice);
	CChannel *Internal_GetChannel(uint32 _ChannelHandle);

	CVoice *Internal_VoiceAlloc(uint32 _iChannel, uint32 _Priority, uint32 _Looping, uint32 _WaveID, fp32 _Pitch);

	CVoice *Internal_VoiceCreate(int _hChn, int _WaveID, int _Priority, fp32 _Pitch, fp32 _Volume, bool _bLoop, bool _bStartPaused, fp32 _MinDelay);
	CVoice *Internal_GetVoiceFromHandle(uint32 _Handle);

	void Internal_UpdateChainParams(CSoundContext_Mixer::CSC_DSPChainInstance *_pChainInstance);

	void Internal_InitDSP();
	void Internal_InitCategories();
	void Internal_DestroyCategories();
	

	virtual bint Destroy_Everything();

	//////////////////////////
	// Platform Interface

	class CPlatformInfo
	{
	public:
		uint32 m_nProssingThreads;  // For mixer
		uint32 m_nChannels;			// Number of physical output channels
		uint32 m_FrameLength;		// Must be power of 2
		fp32 m_SampleRate;			// Physical sample frequency

		class CSpeaker
		{
		public:
			CSpeaker()
			{
				m_Position = 0.0f;
				m_bLFE = false;
			}
			CVec3Dfp32 m_Position;
			bint m_bLFE;
		};
		CSpeaker m_Speakers[ESCMixer_MaxChannels];
	};

	fp32 m_SpeakerAngles[ESCMixer_MaxChannels];
	CPlatformInfo m_PlatformInfo;

	virtual void Platform_GetInfo(CPlatformInfo &_Info) pure;
	virtual void Platform_Init(uint32 _MaxMixerVoices) pure;
	virtual void Platform_StartStreamingToMixer(uint32 _MixerVoice, CVoice *_pVoice, uint32 _WaveID, fp32 _SampleRate) pure; // When first packet is submitted they voice must be unpaused with slot EPauseSlot_Delayed
	virtual void Platform_StopStreamingToMixer(uint32 _MixerVoice) pure; // The platform must stop the mixer voice
	virtual bint Platform_IsPlaying(uint32 _MixerVoice) {return true;}

	//////////////////////////
	// Interface

	void Create(CStr _Param);
	void Init(int _MaxVoices);
	void KillThreads();

	NThread::CMutual &GetInterfaceLock();

	// Access function for CWaveContext
	CSC_WCIDInfo* GetWCIDInfo();
	CSC_SFXDesc *GetSFXDesc(const char *_pSFXName);

	const char **GetDebugStrings();

	// Channel management
	int Chn_Alloc(int _nVoices = 1);
	void Chn_Free(int _hChn);
	void Chn_SetVolume(int _hChn, float _Volume);
	fp32 Chn_GetVolume(int _hChn);
	void Chn_SetPitch(int _hChn, float _Pitch);
	void Chn_Play(int _hChn);
	void Chn_Pause(int _hChn);
	void Chn_DestroyVoices(int _hChn);

	// Lip Sync Data
	void *LSD_Get(int16 _WaveID);
	fp32 Timecode_Get(int16 _WaveID);

	// Voice creation
	int Voice_Create(const CSC_VoiceCreateParams &_CreateParams);
	void Voice_InitCreateParams(CSC_VoiceCreateParams &_CreateParams, uint32 _Category);

//	int Voice_Create(int _hChn, int _WaveID, int _Priority = 0, fp32 _Pitch = 1.0f, fp32 _Volume = 1.0f, bool _bLoop = false, bool _bStartPaused = false, fp32 _MinDelay = 0.0f);
//	int Voice_Create3D(int _hChn, int _WaveID, int _Priority, fp32 _Pitch, fp32 _Volume, const CSC_3DProperties& _Properties, bool _bLoop = false, bool _bStartPaused = false, fp32 _MinDelay = 0.0f);

	void Voice_Destroy(int _hVoice);
	bool Voice_IsPlaying(int _hVoice);

	// Voice data
	CMTime Voice_GetPlayPos(int _hVoice);

	CMTime SFX_GetLength(CSC_SFXDesc *_pSFXDesc, int _iWave);
	
	// Property modification
	void Voice_SetPitch(int _hVoice, fp32 _Pitch);
	void Voice_SetVolume(int _hVoice, fp32 _Volume);
	void Voice_Unpause(int _hVoice, uint32 _PauseSlot = 1);
	void Voice_Pause(int _hVoice, uint32 _PauseSlot = 1);
	bint Voice_ReadyForPlay(int _hVoice);

	// 3D-Property modification
	void Voice3D_SetOrigin(int _hVoice, const CVec3Dfp32& _Pos);
	void Voice3D_SetOrientation(int _hVoice, const CVec3Dfp32& _Orientation);
	void Voice3D_SetVelocity(int _hVoice, const CVec3Dfp32& _Vel);
	void Voice3D_SetMinMaxDist(int _hVoice, fp32 _MinDist, fp32 _MaxDist);
	void Voice3D_SetOcclusion(int _hVoice, const CSC_3DOcclusion &_Prop);

	// Precache
	void Wave_PrecacheFlush();
	void Wave_PrecacheBegin( int _Count );
	void Wave_Precache(int _WaveID);
	int Wave_GetMemusage(int _WaveID);
	void Wave_PrecacheEnd(uint16 *_pPreCacheOrder, int _nIds);
	CWaveContext *Wave_GetContext();

	// 3D-Listener modifactions
	void Listen3D_SetCoordinateScale(fp32 _Scale);
	void Listen3D_SetOrigin(const CMat4Dfp32& _Pos);
	void Listen3D_SetVelocity(const CVec3Dfp32& _Vel);

	// Dual Stream
	void MultiStream_Play(const char * *_Files, int _nFiles, bool _FadeIn, bool _bResetVolumes, uint32 _LoopMask = 0xffffffff);
	void MultiStream_Stop(bool _FadeOut);
	void MultiStream_Volumes(fp32 *_Volumes, int _nVolumes, bool _Fade, fp32 *_pFadeSpeeds);
	void MultiStream_Volume(fp32 _Volume);
	void MultiStream_Pause();
	void MultiStream_Resume();

	// Refresh
	void Refresh() pure;

	void Con_Snd_Mix_Reload();
	void Con_Snd_Mix_AutoReload(int32 _bAutoReload);

	int64 m_LastReadCategories;
	bint m_bAutoReloadCategories;
	uint32 m_ReloadSequence;

	void Register(CScriptRegisterContext &_RegContext);

};

