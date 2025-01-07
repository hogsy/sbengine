
#ifndef _INC_LIPSYNC
#define _INC_LIPSYNC

#if defined(M_RTM) || defined(PLATFORM_CONSOLE)
	#define LIPSYNC_NOANALYSER
#else
	//#define LIPSYNC_NOANALYSER
	//#define LIPSYNC_TALKBACK
	//#define LIPSYNC_ANNOSOFT
	//#define LIPSYNC_FACEFX
	#define LIPSYNC_TALKBACK_AND_FACEFX	// Uses FaceFX if data is available, otherwise Talkback.
#endif

namespace NLipSync
{
	enum
	{
		NUMSPEECHTARGETS=15
	};

	// Used for fast bit counts
	const int8 m_aPrecomputedBits_uint8[] = {	0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
												1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
												1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
												2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
												1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
												2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
												2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
												3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8};

	inline int32 CountBits_uint8(uint8 _Value)
	{
		return m_aPrecomputedBits_uint8[_Value];
	}

	inline int32 CountBits_uint16(uint16 _Value)
	{
		return m_aPrecomputedBits_uint8[_Value>>8]+m_aPrecomputedBits_uint8[_Value&0xFF];
	}

	//
	//
	//
	class CAnalysis
	{
	public:
		virtual ~CAnalysis() {}
		virtual int32 GetFrameRate() pure;
		virtual int32 GetFirstFrame() pure;
		virtual int32 GetLastFrame() pure;
		virtual fp32 GetSpeechTargetValueAtFrame(int32 _Track, int32 _Frame) pure;
	};

	//
	//
	//
	class CAnalyser
	{
	public:
		virtual ~CAnalyser() {}
		virtual CAnalysis *GetAnalysis(const char *_pFilename, int32 _FrameRate) pure;
	};

	//
	//
	//
	CAnalyser *GetAnalyser();

	#ifdef LIPSYNC_TALKBACK_AND_FACEFX
		CAnalyser *GetTalkbackAnalyser();
		CAnalyser *GetFaceFXAnalyser();
	#endif

	//
	//
	//
	void CreateCompressedData(CAnalysis *_pAnalysis, void *&_rpData, int32 &_rSize);

	//
	//
	//
	struct SUncompressedFrame
	{
		uint16 m_Mask;
		uint8 m_Values[NUMSPEECHTARGETS];
	};

	//
	//
	//
	struct SFrame
	{
		uint16 m_Mask;
		uint16 m_StartIndex;

		// inline for speed
		inline int32 GetNumTracks() { return CountBits_uint16(m_Mask); }
		inline int32 GetIndexShift(int32 _TrackIndex) { return CountBits_uint16(m_Mask&((1<<_TrackIndex)-1)); }
	};

	//
	//
	//
	class SYSTEMDLLEXPORT CData
	{
	public:
		class CMemoryBlock : public CReferenceCount
		{
		public:
			uint8 *m_pMem;
			CMemoryBlock(mint _Size) { m_pMem = DNew(uint8) uint8[_Size]; }
			~CMemoryBlock() { if(m_pMem) delete [] m_pMem; }
		};
		
		TPtr<CMemoryBlock> m_spMemoryBlock;

		SFrame *m_pFrames;
		
		uint8 *m_pNumbers;
		int32 m_NumNumbers;

		uint8 m_FrameRate;
		int8 m_FirstFrame;
		uint8 m_SpeechTargets;

		uint16 m_NumFrames;

		CData();
		~CData();

		int32 GetNumTracks(int32 _FrameNumber);
		uint8 GetTrackValue(int32 _FrameNumber, int32 _TrackIndex);

		void ReadData(void *_pData);
		void Clear();
	};
};

class SYSTEMDLLEXPORT CLipSync
{
public:
	NLipSync::CData m_Data;

	int16 m_WaveID;
	int16 m_CurrentWaveID;
	int m_hVoice;

	fp32 m_LastValue[16];

	void Clear();
	void SetVoice(int _WaveID, int _hVoice);
	void CopyFrom(const CLipSync &_LipSync);
	void Eval(CSoundContext* _pSound, int _iAnimResource, class CMapData* _pMapData, class CXR_AnimLayer* _pLayers, int& _nLayers, const int* _pRottracks, int _nRottracks, fp32 _Scale);
	fp32 GetBlendValues(CSoundContext* _pSound, int &_num);
};

#endif // _INC_LIPSYNC
