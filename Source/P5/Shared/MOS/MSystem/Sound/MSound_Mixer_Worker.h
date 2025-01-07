

#include "../../../MCC/VPUShared/MCC_VPUShared.h"
#include "../../../MCC/VPUShared/MDA_VPUShared.h"


#ifdef M_COMPILING_ON_VPU
namespace NVPU
{
#endif

enum
{
	ESCMixer_PacketType_None = 0,
	ESCMixer_PacketType_fp32,
	ESCMixer_PacketType_int16,
	ESCMixer_MaxChannels = 16, // Max number of channels in an mixbuffer (must be multiple of 4)
	ESCMixer_MaxChannelVectors = ESCMixer_MaxChannels >> 2,

};
class CSC_Mixer_OuputFrame
{
public:
	vec128 *m_pData; // Data Channels are interleaved and unused channels in a vector is 0.0
	uint32 m_nChannels;
	uint32 m_nSamples;
};

class CSC_Mixer_ProcessingInfo
{
public:
	uint32 m_nChannels:8;
	uint32 m_nSamples:24;
	vec128 *m_pDataIn;
	vec128 *m_pDataOut;
};

enum
{
	ESC_Mixer_ProcessingType_In = 0,
	ESC_Mixer_ProcessingType_Out,
	ESC_Mixer_ProcessingType_InOut,
	ESC_Mixer_ProcessingType_InPlace,
};

enum
{
	ESC_Mixer_DSP_Master = 0,
	ESC_Mixer_DSP_Voice	= 1,
	ESC_Mixer_DSP_Router = 2,
	ESC_Mixer_DSP_SilenceGen = 3,
	ESC_Mixer_DSP_VolumeMatrix = 4,
	ESC_Mixer_DSP_Reverb = 5,
	ESC_Mixer_DSP_Volume = 6,
	ESC_Mixer_DSP_Copy = 7,
	ESC_Mixer_DSP_BiQuad = 8,
	ESC_Mixer_DSP_Max,
};




class CSC_Mixer_DSPChain
{
public:
};

class CSC_Mixer_DSPInstance
{
public:
};

class CSC_Mixer_DSPChainInstance
{
public:
};





typedef void (FSCMixer_DSPFunc_ProcessFrame)(CSC_Mixer_ProcessingInfo *_pInfo, void *_pInternalData, const void *_pParams, void *_pLastParams);
typedef void (FSCMixer_DSPFunc_PreProcessFrame)(void *_pInternalData, const void *_pParams, const void *_pLastParams);
typedef bint (FSCMixer_DSPFunc_InitData)(void *_pInternal, void *_pParams, void *_pLastParams);
typedef void (FSCMixer_DSPFunc_DestroyData)(void *_pInternal, void *_pParams, void *_pLastParams);
typedef void (FSCMixer_DSPFunc_CopyData)(void *_pParams, void *_pLastParams, void *_pInternal, void *_SourceData, void *_DestData);
typedef void (FSCMixer_DSPFunc_Create)(void *_pInternal, void *_pParams, void *_pLastParams);

/*
#if defined(TARGET_PS3_SPU) || defined(SN_TARGET_PS3_SPU)
#define M_COMPILING_ON_VPU
#endif*/
class CSC_Mixer_WorkerContext_DMAHelper;
class CSC_Mixer_WorkerContext
{
public:

	typedef uint16 COffset;
	typedef int16 CSignedOffset;
	
	static uint32 ms_DSP_NumParams[ESC_Mixer_DSP_Max];
	static uint32 ms_DSP_NumLastParams[ESC_Mixer_DSP_Max];
	static uint32 ms_DSP_NumInternal[ESC_Mixer_DSP_Max];
	static uint32 ms_DSP_ProcessingType[ESC_Mixer_DSP_Max];
	static FSCMixer_DSPFunc_ProcessFrame *ms_DSP_Func_ProcessFrame[ESC_Mixer_DSP_Max];
	static FSCMixer_DSPFunc_PreProcessFrame *ms_DSP_Func_PreProcessFrame[ESC_Mixer_DSP_Max];
	static FSCMixer_DSPFunc_InitData *ms_DSP_Func_InitData[ESC_Mixer_DSP_Max];
	static FSCMixer_DSPFunc_DestroyData *ms_DSP_Func_DestroyData[ESC_Mixer_DSP_Max];
	static FSCMixer_DSPFunc_CopyData *ms_DSP_Func_CopyData[ESC_Mixer_DSP_Max];
	static FSCMixer_DSPFunc_Create *ms_DSP_Func_Create[ESC_Mixer_DSP_Max];

	static mint ms_DataOffset;

	class CCustomPtrHolder
	{
	public:
		COffset m_Offset;

		DIdsPInlineS void * Get() const
		{
//			if (m_Offset > 64*1024)
//				M_BREAKPOINT;
			if ((m_Offset & (~uint32(3))) == 0)
				return (void *)(mint)(m_Offset & 3);
			return (void *)(CSC_Mixer_WorkerContext::ms_DataOffset + m_Offset);
		}

		DIdsPInlineS void Set(void *_pAddress)
		{
			mint Address = (mint)_pAddress;
			mint Offset;
			
			if ((Address & (~mint(3))) == 0)
				Offset = Address & 3;
			else
				Offset = Address - ms_DataOffset;

#ifdef M_Profile
			if (Offset > 64*1024)
				M_BREAKPOINT;
#endif
			m_Offset = Offset;
		}

		DIdsPInlineS void Set(CCustomPtrHolder &_Address)
		{
			m_Offset = _Address.m_Offset;
//			if (m_Offset > 64*1024)
//				M_BREAKPOINT;
		}        
	};

#ifdef M_COMPILING_ON_VPU
	// On VPU we only have access to context 0

	class CCustomAllocator : public DIdsDefaultAllocator
	{
	public:
		typedef CCustomPtrHolder CPtrHolder;
	};

#else

	~CSC_Mixer_WorkerContext()
	{
		m_FreeContainters.DeleteAll();
	}


	class CCustomAllocator
	{
	public:

		DIdsPInlineS static mint StaticAddresses()
		{
			return 0;
		}

		typedef CCustomPtrHolder CPtrHolder;
		
		DIdsPInlineS static mint GranularityAlloc()
		{
			return 4;
		}

		DIdsPInlineS static mint GranularityCommit()
		{
			return 4096;
		}

		DIdsPInlineS static mint GranularityProtect()
		{
			return 4096;
		}

		DIdsPInlineS static mint Size(void *_pBlock)
		{
			DLock(*CSC_Mixer_WorkerContext::ms_pBlockManagerLock);
			TCSimpleBlockManager<>::CBlock *pBlock = CSC_Mixer_WorkerContext::ms_pBlockManager->GetBlockFromAddress((mint)_pBlock - CSC_Mixer_WorkerContext::ms_DataOffset);
			return CSC_Mixer_WorkerContext::ms_pBlockManager->GetBlockSize(pBlock);
		}

		DIdsPInlineS static bint CanCommit()
		{
			return false;
		}

		DIdsPInlineS static bint CanProtect()
		{
			return false;
		}

		DIdsPInlineS static bint CanFail()
		{
			return true;
		}

		DIdsPInlineS static void Protect(void *_pMem, mint _Size, auint _Protect)
		{

		}

		DIdsPInlineS static void *AllocDebug(mint _Size, mint _Location, const ch8 *_pFile, aint _Line, aint _Flags = 0)
		{
			return Alloc(_Size, _Location);
		}

		DIdsPInlineS static void *AllocAlignDebug(mint _Size, mint _Align, mint _Location, const ch8 *_pFile, aint _Line, aint _Flags = 0)
		{
			return AllocAlign(_Size, _Align, _Location);
		}

		DIdsPInlineS static void *AllocAlign(mint _Size, mint _Align, mint _Location = 0)
		{
			DLock(*CSC_Mixer_WorkerContext::ms_pBlockManagerLock);
			TCSimpleBlockManager<>::CBlock *pBlock = CSC_Mixer_WorkerContext::ms_pBlockManager->Alloc(_Size, _Align);
			if (pBlock)
				return (void *)(CSC_Mixer_WorkerContext::ms_DataOffset +  pBlock->GetMemory());
			return NULL;
		}

		DIdsPInlineS static void *Alloc(mint _Size, mint _Location = 0)
		{
			DLock(*CSC_Mixer_WorkerContext::ms_pBlockManagerLock);
			TCSimpleBlockManager<>::CBlock *pBlock = CSC_Mixer_WorkerContext::ms_pBlockManager->Alloc(_Size, 1);
			if (pBlock)
				return (void *)(CSC_Mixer_WorkerContext::ms_DataOffset +  pBlock->GetMemory());
			return NULL;
		}

		DIdsPInlineS static void Free(void *_pBlock, mint _Size = 0, mint _Location = 0)
		{
			DLock(*CSC_Mixer_WorkerContext::ms_pBlockManagerLock);
			TCSimpleBlockManager<>::CBlock *pBlock = CSC_Mixer_WorkerContext::ms_pBlockManager->GetBlockFromAddress((mint)_pBlock - CSC_Mixer_WorkerContext::ms_DataOffset);
			return CSC_Mixer_WorkerContext::ms_pBlockManager->Free(pBlock);
		}

		DIdsPInlineS static void *Realloc(void *_pMem, mint _Size, mint _Location = 0, mint _OldSize = 0, mint _OldLocation = 0)
		{
			M_BREAKPOINT;
			return NULL;
		}

		DIdsPInlineS static void *ReallocDebug(void *_pMem, mint _Size, mint _Location, mint _OldSize, mint _OldLocation, const ch8 *_pFile, aint _Line, aint _Flags = 0)
		{
			M_BREAKPOINT;
			return NULL;
		}

		DIdsPInlineS static void *Resize(void *_pMem, mint _Size, mint _Location = 0, mint _OldSize = 0, mint _OldLocation = 0)
		{
			M_BREAKPOINT;
			return NULL;
		}

		DIdsPInlineS static void *ResizeDebug(void *_pMem, mint _Size, mint _Location, mint _OldSize, mint _OldLocation, const ch8 *_pFile, aint _Line, aint _Flags = 0)
		{
			M_BREAKPOINT;
			return NULL;
		}

		DIdsPInlineS static void *AllocNoCommit(mint _Size, mint _Location = 0)
		{
			return Alloc(_Size, _Location);
		}

		DIdsPInlineS static void Commit(void *_pMem, mint _Size)
		{
		}

		DIdsPInlineS static void Decommit(void *_pMem, mint _Size)
		{
		}

	};

	template <typename t_CClass>
	static t_CClass *New()
	{
		DLock(*CSC_Mixer_WorkerContext::ms_pBlockManagerLock);
		TCSimpleBlockManager<>::CBlock *pBlock = ms_pBlockManager->Alloc(sizeof(t_CClass), M_ALIGNMENTOF(t_CClass));

		if (pBlock)
		{
			void *pAlloc = (void *)(ms_DataOffset + (mint)pBlock->GetMemory());

			return new(pAlloc) t_CClass;
		}
		else
		{
			return NULL;
		}
	}

	template <typename t_CClass, typename t_CArg0>
	static t_CClass *New(t_CArg0 _Arg0)
	{
		DLock(*CSC_Mixer_WorkerContext::ms_pBlockManagerLock);
		TCSimpleBlockManager<>::CBlock *pBlock = ms_pBlockManager->Alloc(sizeof(t_CClass), M_ALIGNMENTOF(t_CClass));

		if (pBlock)
		{
			void *pAlloc = (void *)(ms_DataOffset + (mint)pBlock->GetMemory());

			return new(pAlloc) t_CClass(_Arg0);
		}
		else 
		{
			return NULL;
		}
	}

	template <typename t_CClass, typename t_CArg0, typename t_CArg1>
	static t_CClass *New(t_CArg0 _Arg0, t_CArg1 _Arg1)
	{
		DLock(*CSC_Mixer_WorkerContext::ms_pBlockManagerLock);
		TCSimpleBlockManager<>::CBlock *pBlock = ms_pBlockManager->Alloc(sizeof(t_CClass), M_ALIGNMENTOF(t_CClass));

		if (pBlock)
		{
			void *pAlloc = (void *)(ms_DataOffset + (mint)pBlock->GetMemory());

			return new(pAlloc) t_CClass(_Arg0, _Arg1);
		}
		else 
		{
			return NULL;
		}
	}

	template <typename t_CClass, typename t_CArg0, typename t_CArg1, typename t_CArg2>
	static t_CClass *New(t_CArg0 _Arg0, t_CArg1 _Arg1, t_CArg2 _Arg2)
	{
		DLock(*CSC_Mixer_WorkerContext::ms_pBlockManagerLock);
		TCSimpleBlockManager<>::CBlock *pBlock = ms_pBlockManager->Alloc(sizeof(t_CClass), M_ALIGNMENTOF(t_CClass));

		if (pBlock)
		{
			void *pAlloc = (void *)(ms_DataOffset + (mint)pBlock->GetMemory());

			return new(pAlloc) t_CClass(_Arg0, _Arg1, _Arg2);
		}
		else 
		{
			return NULL;
		}
	}

	template <typename t_CClass, typename t_CArg0, typename t_CArg1, typename t_CArg2, typename t_CArg3>
	static t_CClass *New(t_CArg0 _Arg0, t_CArg1 _Arg1, t_CArg2 _Arg2, t_CArg3 _Arg3)
	{
		DLock(*CSC_Mixer_WorkerContext::ms_pBlockManagerLock);
		TCSimpleBlockManager<>::CBlock *pBlock = ms_pBlockManager->Alloc(sizeof(t_CClass), M_ALIGNMENTOF(t_CClass));

		if (pBlock)
		{
			void *pAlloc = (void *)(ms_DataOffset + (mint)pBlock->GetMemory());

			return new(pAlloc) t_CClass(_Arg0, _Arg1, _Arg2, _Arg3);
		}
		else 
		{
			return NULL;
		}
	}

	template <typename t_CClass, typename t_CArg0, typename t_CArg1, typename t_CArg2, typename t_CArg3, typename t_CArg4>
	static t_CClass *New(t_CArg0 _Arg0, t_CArg1 _Arg1, t_CArg2 _Arg2, t_CArg3 _Arg3, t_CArg4 _Arg4)
	{
		DLock(*CSC_Mixer_WorkerContext::ms_pBlockManagerLock);
		TCSimpleBlockManager<>::CBlock *pBlock = ms_pBlockManager->Alloc(sizeof(t_CClass), M_ALIGNMENTOF(t_CClass));

		if (pBlock)
		{
			void *pAlloc = (void *)(ms_DataOffset + (mint)pBlock->GetMemory());

			return new(pAlloc) t_CClass(_Arg0, _Arg1, _Arg2, _Arg3, _Arg4);
		}
		else 
		{
			return NULL;
		}
	}

	template <typename t_CClass>
	static void Delete(t_CClass *_pToDelete)
	{
		if (_pToDelete)
		{
			_pToDelete->~t_CClass();
			{
				DLock(*CSC_Mixer_WorkerContext::ms_pBlockManagerLock);
				TCSimpleBlockManager<>::CBlock *pBlock = ms_pBlockManager->GetBlockFromAddress((mint)_pToDelete - CSC_Mixer_WorkerContext::ms_DataOffset);
				ms_pBlockManager->Free(pBlock);
			}
		}
	}


#endif


	class CSC_Mixer_Packet
	{
	public:
		CSC_Mixer_Packet()
		{
			m_PacketType = ESCMixer_PacketType_None;
			m_nChannels = 0;
			m_nSamples = 0;
			m_bCircular = 0;
		}
		DLinkAllocatorDS_Link(CSC_Mixer_Packet, m_Link, CCustomAllocator);
		uint32 m_PacketType:2;
		uint32 m_nChannels:5;
		uint32 m_bCircular:1;
		uint32 m_nSamples:25;
	};

	static TCDynamicPtr<CCustomPtrHolder, CSC_Mixer_WorkerContext> ms_pThis;

	enum
	{
		EMaxExtraFramesToConsume = 8, // This is the number of frames that
	};

	// Align locks on cacheline boundaries

	class CMixBin
	{
		friend class CSC_Mixer_WorkerContext;
		friend class CSC_Mixer_WorkerContext_DMAHelper;
	protected:
		vec128 *m_pData0;
	public:
		uint32 m_nChannels;
	};

	class CDSPInstanceInternal;
	class CDSPChainInternal;
	class CDSPChainInstanceInternal;


	// Flags
	uint32 m_VoiceStartLocked;

	CSC_Mixer_OuputFrame m_OutputFrames[2];
	uint64 m_CurrentFrame;
	uint64 m_StartedFrame;
	fp32 m_SampleRate;
	fp32 m_DesiredSampleRate;
	fp32 m_FrameTime;
	fp32 m_SampleTime;
	uint32 m_FrameLength;
	uint32 m_nChannelVectors;
	uint32 m_nProcessingThreads;

	TThinArrayAlign<vec128, 128> m_OutputData;

	class M_ALIGN(16) CMixBinContainer : public CMixBin
	{
	public:
		CMixBinContainer(uint32 _nData)
		{
			m_Data.SetLen(_nData);
			m_pData0 = m_Data.GetBasePtr();
		}

		TThinArrayAlign<vec128, 128> m_Data;
		DLinkAllocatorDS_Link(CMixBinContainer, m_Link, CCustomAllocator);
	};

	class CProcessingInfoInternal : public CSC_Mixer_ProcessingInfo
	{
	public:
		TCDynamicPtr<CCustomPtrHolder, CMixBinContainer> m_pContainerIn;
		TCDynamicPtr<CCustomPtrHolder, CMixBinContainer> m_pContainerOut;
	};


	DLinkAllocatorDS_List(CMixBinContainer, m_Link, CCustomAllocator) m_FreeContainters;
	uint32 m_nCreatedContainers;

	class CBixBinCointanerPtr
	{
	public:
		CMixBinContainer *m_pContainer;
		vec128 *m_pPtr;
		bint m_bDiscard;
	};
	void GetMixBinContainerPtrs(CBixBinCointanerPtr *_pContainers, mint _nContainers);

	CMixBinContainer *GetMixBinContainer();

	void ReturnMixBinContainer(CMixBinContainer *_pContainer);

	class CDSPInstanceLink
	{
	public:
		DLinkAllocatorDS_Link(CDSPInstanceLink, m_Link, CCustomAllocator);
		TCDynamicPtr<CCustomPtrHolder, CDSPInstanceInternal> m_pDSPInstance;
	};

	typedef DLinkAllocatorDS_Iter(CDSPInstanceLink, m_Link, CCustomAllocator) CDSPInstanceLinkIter;

	class CDSPInstanceInternal : public CSC_Mixer_DSPInstance
	{
	public:
#ifdef M_COMPILING_ON_VPU
	private:
		CDSPInstanceInternal() {}
	public:
#else
		~CDSPInstanceInternal()
		{
			m_RouteTo.DeleteAll();
		}
#endif

		DLinkAllocatorDS_Link(CDSPInstanceInternal, m_Link, CCustomAllocator);
		DLinkAllocatorDS_Link(CDSPInstanceInternal, m_LinkSpecialCopy, CCustomAllocator);
		DLinkAllocatorDS_List(CDSPInstanceLink, m_Link, CCustomAllocator) m_RouteTo;
		TCDynamicPtr<CCustomPtrHolder, CDSPChainInternal> m_pDSPChain;
		COffset m_DSPID;
		COffset m_iChainDSP;
	};

	typedef DLinkAllocatorDS_Iter(CDSPInstanceInternal, m_Link, CCustomAllocator) CDSPInstanceInternalIterator;
	typedef DLinkAllocatorDS_Iter(CDSPInstanceInternal, m_LinkSpecialCopy, CCustomAllocator) CDSPInstanceInternalIteratorSpecailCopy;

	class CDSPChainInternal : public CSC_Mixer_DSPChain
	{
	public:
		CDSPChainInternal()
		{
			m_nDSPInstances = 0;
		}
#ifndef M_COMPILING_ON_VPU
		~CDSPChainInternal()
		{
			m_DSPInstances.DeleteAll();
		}
#endif
		DLinkAllocatorDS_Link(CDSPChainInternal, m_Link, CCustomAllocator);
		DLinkAllocatorDS_List(CDSPInstanceInternal, m_Link, CCustomAllocator) m_DSPInstances;
		DLinkAllocatorDS_List(CDSPInstanceInternal, m_LinkSpecialCopy, CCustomAllocator) m_DSPInstancesSpecialCopy;
		COffset m_nDSPInstances;
	};


	DLinkAllocatorDS_List(CDSPChainInternal, m_Link, CCustomAllocator) m_DSPChains;


	class CMixBinRef
	{
	public:
#ifdef M_COMPILING_ON_VPU
	private:
		CMixBinRef() {}
	public:
#else
		CMixBinRef()
		{
			m_pBinContainer = 0;
			m_nToMix = 0;
		}
		~CMixBinRef()
		{
			if (m_pBinContainer || m_nToMix)
				M_BREAKPOINT; // Something went wrong
		}
#endif

		TCDynamicPtr<CCustomPtrHolder, CMixBinContainer> m_pBinContainer;
		CSignedOffset m_nToMix;
	};

	class CDSPChainInstanceInternal : public CSC_Mixer_DSPChainInstance
	{
	public:

#ifdef M_COMPILING_ON_VPU
	private:
		CDSPChainInstanceInternal() {}
	public:
#else
		CDSPChainInstanceInternal()
		{
			m_nReferences = 0;
			m_pChain = 0;
			ClearDSP();
		}

		~CDSPChainInstanceInternal()
		{
			if (m_nReferences > 0)
			{
				M_BREAKPOINT; // Not good
			}
			ClearDSP();
		}
#endif

		DLinkAllocatorD_Link(CDSPChainInstanceInternal, m_Link, CCustomAllocator);
		TCDynamicPtr<CCustomPtrHolder, CDSPChainInternal> m_pChain;
		TThinArrayAlign<uint8, 16, 0, CCustomAllocator> m_Data;
		TThinArray<CMixBinRef, CCustomAllocator> m_MixBinRef;
		TThinArray<uint16, CCustomAllocator> m_Indices;
		TCDynamicPtr<CCustomPtrHolder, uint16> m_pParamIndices;
		TCDynamicPtr<CCustomPtrHolder, uint16> m_pLastParamIndices;
		TCDynamicPtr<CCustomPtrHolder, uint16> m_pInternalDataIndices;
		COffset m_nReferences;
		COffset m_CopyDataStart;
		COffset m_bNewlyCreated:1;
		COffset m_bActive:1;

#ifndef M_COMPILING_ON_VPU
		void ClearDSP();
		bint SetDSPChain(CDSPChainInternal *_pChain);
		bint AllocIndices(int _nDSPInstances);
#endif

		void *GetInternalPtr(uint32 _iDSP)
		{
			return m_Data.GetBasePtr() + m_pInternalDataIndices[_iDSP];
		}

		void *GetParamPtr(uint32 _iDSP)
		{
			return m_Data.GetBasePtr() + m_pParamIndices[_iDSP];
		}

		void *GetLastParamPtr(uint32 _iDSP)
		{
			return m_Data.GetBasePtr() + m_pLastParamIndices[_iDSP];
		}
	};
	DLinkAllocatorD_List(CDSPChainInstanceInternal, m_Link, CCustomAllocator) m_DSPChainInstancesProcess;
	DLinkAllocatorD_List(CDSPChainInstanceInternal, m_Link, CCustomAllocator) m_DSPChainInstancesNonProcess;
	typedef DLinkAllocatorD_Iter(CDSPChainInstanceInternal, m_Link, CCustomAllocator) CChainIter;
	TCDynamicPtr<CCustomPtrHolder, CDSPChainInstanceInternal> m_pMasterInstance;
	 
//	DLinkDS_List(CDSPChainInstanceInternal, m_Link) m_DSPChainInstances;

/*	CDSPChainInternal m_MasterChain;
	CDSPChainInstanceInternal m_MasterChainInstance;
	CDSPInstanceInternal m_MasterDSPInstance;

	CDSPInstanceInternal m_VoiceDSPInstanceResample;
	CDSPInstanceInternal m_VoiceDSPInstanceOutput;
	CDSPChainInternal m_VoiceChain;

	CDSPInstanceInternal m_SilenceGenDSPInstance;
	CDSPInstanceInternal m_SilenceGenDSPInstanceRouter;
	CDSPChainInternal m_SilenceGenChain;
	CDSPChainInstanceInternal m_SilenceGenChainInstance;
	*/

	class CVoice
	{
	public:
#ifdef M_COMPILING_ON_VPU
#else
		CVoice()
		{
			 m_pCurrentPacket = NULL;
			 m_pChainInstance = NULL;
		}
#endif

//		CDSPChainInstanceInternal m_ChainInstance;

		DLinkAllocatorDS_Link(CVoice, m_Link, CCustomAllocator);
		DLinkAllocatorDS_Link(CVoice, m_LinkDelete, CCustomAllocator);
		DLinkAllocatorDS_List(CSC_Mixer_Packet, m_Link, CCustomAllocator) m_QueuedPackets;
		DLinkAllocatorDS_List(CSC_Mixer_Packet, m_Link, CCustomAllocator) m_FinishedPackets;
		DLinkAllocatorDS_List(CSC_Mixer_Packet, m_Link, CCustomAllocator) m_PendingFinishedPackets;
		TCDynamicPtr<CCustomPtrHolder, CSC_Mixer_Packet> m_pCurrentPacket;
		TCDynamicPtr<CCustomPtrHolder, CDSPChainInstanceInternal> m_pChainInstance;
		uint32 m_CurrentPacketPos;
		fp32 m_OriginalPitch;
		fp32 m_CurrentPitch;
		fp32 m_CurrentPitchInv;
		fp32 m_SamplesLeft;
		uint64 m_StartFrame;
		int64 m_nSamples;
		int64 m_LoopPoint;
		int64 m_SamplePos; // Can be negative if voice is delayed
		int32 m_SyncSamples;
		uint32 m_SyncTo;
		uint32 m_LoopSkipStart:16;
		uint32 m_LoopSkipBoth:16;
		uint32 m_LoopSkipCurrent:16;
		uint32 m_bLoopSkipDone:5;
		uint32 m_nChannels:5;
		uint32 m_bDoneStop:1;
		uint32 m_bPaused;
		uint32 m_bInvalid;
		uint32 m_bPausedProcessing;
	};

	DLinkAllocatorDS_List(CVoice, m_Link, CCustomAllocator) m_VoicesPlaying;
	DLinkAllocatorDS_List(CVoice, m_Link, CCustomAllocator) m_VoicesWaitingToPlay;
	DLinkAllocatorDS_List(CVoice, m_Link, CCustomAllocator) m_VoicesFree;
	uint32 m_nFreeVoices;

	typedef DLinkAllocatorDS_Iter(CVoice, m_Link, CCustomAllocator) CVoiceIter;

	DLinkAllocatorDS_List(CVoice, m_LinkDelete, CCustomAllocator) m_VoicesWaitingToStop;

	TThinArray<CVoice, CCustomAllocator> m_Voices;

#ifdef M_COMPILING_ON_VPU
	void PreProcessDSPChainInstance(CDSPChainInstanceInternal *_pChainInstance);

	void PreProcessDSPInstance(CDSPChainInstanceInternal *_pChainInstance, CDSPInstanceInternal *_pDSPInstance);

	void ProcessDSPChainInstance(CDSPChainInstanceInternal *_pChainInstance, CMixBinContainer *_pMixBin);

	void ProcessDSPInstance(CDSPChainInstanceInternal *_pChainInstance, CDSPInstanceInternal *_pDSPInstance, CMixBinContainer *_pMixBin);
	
	void CalculateFrame(CSC_Mixer_OuputFrame *_pOutput);
#endif


#ifndef M_COMPILING_ON_VPU
	// NONVPU stuff here
	static TCSimpleBlockManager<> *ms_pBlockManager;
	static NThread::CMutual *ms_pBlockManagerLock;

#endif


	class CVoiceVPUChange
	{
	public:
		TCDynamicPtr<CCustomPtrHolder, CSC_Mixer_Packet> m_pCurrentPacket;
		uint32 m_CurrentPacketPos;
		int64 m_SamplePos;
		fp32 m_SamplesLeft;
		uint32 m_LoopSkipCurrent;
		uint32 m_bLoopSkipDone;
	};

	class M_ALIGN(16) CVPU_WorkParams
	{
	public:
		TCDynamicPtr<CCustomPtrHolder, CSC_Mixer_WorkerContext> m_pThis;
		TCDynamicPtr<CCustomPtrHolder, CSC_Mixer_OuputFrame> m_pOutput;

	};

#if defined(PLATFORM_SPU) || defined(PLATFORM_PS3)
	CVPU_WorkParams m_WorkParams;
#endif
};


class CSC_Mixer_Packet_fp32 : public CSC_Mixer_WorkerContext::CSC_Mixer_Packet
{
public:
	CSC_Mixer_Packet_fp32()
	{
		m_PacketType = ESCMixer_PacketType_fp32;
	}
	fp32 *m_pData;
};

class CSC_Mixer_Packet_int16 : public CSC_Mixer_WorkerContext::CSC_Mixer_Packet
{
public:
	CSC_Mixer_Packet_int16()
	{
		m_PacketType = ESCMixer_PacketType_int16;
	}
	int16 *m_pData;
};

#ifdef M_COMPILING_ON_VPU
}
#endif

enum
{
	ESCMixer_VPUCall_GetNewMixBin = 2,
};

void SCMixer_VPUAccess_GetVPUChangableForVoice(uint32 _iVoice, void *_pDest);
void SCMixer_VPUAccess_DequePendingPacket(uint32 _iVoice, void *_pDest);
void SCMixer_VPUAccess_CalculateFrame(void *_pOutput, void *_pWorkMemory);
void SCMixer_VPUAccess_SetOffest(mint _Offset);
void SCMixer_VPUAccess_SetThis(void *_pPtr);

void SCMixer_PPUAccess_GetNewMixBin(void *_pDest);
