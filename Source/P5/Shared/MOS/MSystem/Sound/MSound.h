
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030606:		Added Comments
\*_____________________________________________________________________________________________*/


#ifndef _INC_MSOUND
#define _INC_MSOUND

#include "../Raster/MImage.h"
#include "../Misc/MConsole.h"

#define CSC_SFXDESC_VERSION 0x0106
#define CSC_SFXDESC_VERSION_VER6 0x0106
#define CSC_SFXDESC_VERSION_VER5 0x0105
#define CSC_SFXDESC_VERSION_VER4 0x0104
#define CSC_SFXDESC_VERSION_VER3 0x0103
#define CSC_SFXDESC_VERSION_VER2 0x0102
#define SC_THREADS

#ifdef PLATFORM_CONSOLE
#ifndef M_Profile
	#define USE_HASHED_SFXDESC
	#define USE_HASHED_WAVENAME
#endif
#endif

enum ESC_CWaveDataFlags
{
	ESC_CWaveDataFlags_Uninterleaved = 0x1,
	ESC_CWaveDataFlags_Stream = 0x2
//	ESC_CWaveData_ = 0x2
//	ESC_CWaveData_ = 0x4

};

class CWaveData
{
public:
	uint32 m_BitsData2;
//	uint32 m_nSamples:31;
//	uint32 m_bCanDecode:1;

/*	class CBits
	{
	public:
		uint32 m_SampleRate:18; //256k Max
		uint32 m_SampleSize:4;
		uint32 m_nChannels:4;
		uint32 m_Flags:6;
	};*/

	uint32 m_BitsData;
	
	M_INLINE CWaveData() :
		m_BitsData2(0),
		m_BitsData(0)		
	{
	}

	M_INLINE uint32 GetNumSamples()  const
	{
		return m_BitsData2 & (~(1<<31));
	}
	M_INLINE void SetNumSamples(uint32 _Value)
	{
		m_BitsData2 &= (1<<31);
		m_BitsData2 |= _Value & (~(1<<31));;
	}

	M_INLINE bool GetCanDecode()  const
	{
		return !(m_BitsData2 & (1<<31));
	}
	M_INLINE void SetCanDecode(bool _Value)
	{
		m_BitsData2 &= ~(1<<31);
		m_BitsData2 |= (!_Value) << 31;
	}

	M_INLINE uint32 GetSampleRate()  const
	{
		return (m_BitsData & (((1<<18) - 1) << 14)) >> 14;
	}
	M_INLINE void SetSampleRate(uint32 _Value)
	{
		m_BitsData &= ~(((1<<18) - 1) << 14); 
		m_BitsData |= (_Value & ((1<<18) - 1)) << 14;
	}

	M_INLINE uint32 GetSampleSize() const
	{
		return (m_BitsData & (((1<<4) - 1) << 10)) >> 10;
	}
	M_INLINE void SetSampleSize(uint32 _Value)
	{
		m_BitsData &= ~(((1<<4) - 1) << 10); 
		m_BitsData |= (_Value & ((1<<4) - 1)) << 10;
	}

	M_INLINE uint32 GetChannels()  const
	{
		return (m_BitsData & (((1<<4) - 1) << 6)) >> 6;
	}
	M_INLINE void SetChannels(uint32 _Value)
	{
		m_BitsData &= ~(((1<<4) - 1) << 6); 
		m_BitsData |= (_Value & ((1<<4) - 1)) << 6;
	}

	M_INLINE uint32 Get_Flags()  const
	{
		return (m_BitsData & (((1<<6) - 1) << 0)) >> 0;
	}
	M_INLINE void Set_Flags(uint32 _Value)
	{
		m_BitsData &= ~(((1<<6) - 1) << 0); 
		m_BitsData |= (_Value & ((1<<6) - 1)) << 0;
	}

	void ByteSwap()
	{
#if CPU_BIGENDIAN
		ByteSwap_uint32(m_BitsData2);
		ByteSwap_uint32(m_BitsData);
#endif
	}

	void Write(CCFile *_pFile)
	{
		ByteSwap();
		_pFile->Write(this, sizeof(*this));
		ByteSwap();
	}

	void Read(CCFile *_pFile)
	{
		_pFile->Read(this, sizeof(*this));
		ByteSwap();
	}
};


// -------------------------------------------------------------------
//  CWaveform
// -------------------------------------------------------------------
/*
NOTE:
	- A wavetable is an image.
	- The width is the number of channels.
	- The height is the number of samples.
	- The modulo is the size of a sample.
	- 8 bit samples use the IMAGE_FORMAT_I8 format.
	- 16 bit samples use the IMAGE_FORMAT_I16 format.
*/
#define WAVE_FORMAT_8BIT	IMAGE_FORMAT_I8
#define WAVE_FORMAT_16BIT	IMAGE_FORMAT_I16

class SYSTEMDLLEXPORT CWaveform : public CImage
{
public:
	int32 m_Rate;
	int32 m_LoopStart;
	int32 m_LoopEnd;
	uint8 m_SpeakerAssignment[16];
	uint8 m_ChannelAssignments[16];


	CWaveform()
	{
		m_Rate = -1;
		m_LoopStart = -1;
		m_LoopEnd = -1;
		for (int i = 0; i < 16; ++i)
		{
			m_SpeakerAssignment[i] = 0xFF;
			m_ChannelAssignments[i] = i / 2;
		}
	};


	void WT_Create(int _nSamples, int _nChannels, int _Format);

	inline int WT_GetSampleCount() { return m_Height; };
	inline int WT_GetChnCount() { return m_Width; };
	inline int WT_GetBitDepth() { return m_Pixelsize*8; };
	inline int WT_GetSampleSize() { return m_Pixelsize*m_Width; };
	inline int WT_GetBufferSize() { return m_Pixelsize*m_Width*m_Height; };
	inline int WT_GetSampleRate() { return m_Rate; };

	bool WT_CaptureWAV(void* _pData, int _Size);
	void WT_ReadWAV(CStr _Filename);
	void WT_WriteWAV();

	void WT_CompressToVorbis();

	void WT_ReadRAW(CCFile* _pFile, int _Size, int _Format = WAVE_FORMAT_8BIT, int _nChn = 1, bool _bUnsigned = false);
	void WT_ReadRAW(CStr _Filename, int _Format = WAVE_FORMAT_8BIT, int _nChn = 1, bool _bUnsigned = false);
	void WT_WriteRAW(CCFile* _pFile);
	void WT_WriteRAW(CStr _Filename);

	void WT_Read(CStr _Filename, int _Flags = 0);	// 'Extension sensitive'.

	void WT_Add(int _iStart, int _Count, int _Value, bool _bWrap);
	void WT_Add(int _iStart, int _Count, fp32 _Value, bool _bWrap);
	void WT_SignFlip();
};

typedef TPtr<CWaveform> spCWaveform;

// -------------------------------------------------------------------
//  CSnd_WaveContainer
// -------------------------------------------------------------------
/*class CSoundSystem;

class CSnd_Container : public CReferenceCount
{
	CSoundSystem* m_pSndSys;
public:
	
}

typedef TPtr<CSnd_Container> spCSnd_Container;

*/

enum
{
	CWC_WAVEIDFLAGS_PRECACHE = 0x01
	,CWC_WAVEIDFLAGS_USED		= 0x02
};

// -------------------------------------------------------------------
class CWC_WaveIDInfo			// 4 Bytes, 64k st.txt. => 256kb
{
public:

	int16 m_iWaveClass:12;			// Reg. wave constructor class.
	int16 m_Flags:4;				// Flags

	int16 m_iLocal;				// WaveClass local waveform index

	CWC_WaveIDInfo()
	{
		m_iWaveClass = -1;
		m_iLocal = -1;
		m_Flags = 0;
	}
};

//----------------------------------------------------------------
class CWaveContext;

class CWaveContainer : public CReferenceCount
{
protected:
	friend class CWaveContext;
	int m_iWaveClass;
	CWaveContext* m_pWaveContext;

public:
	MACRO_OPERATOR_TPTR(CWaveContainer);

	CWaveContainer();
	~CWaveContainer();

	virtual int GetWaveCount() pure;
	virtual int GetWaveID(int _iLocal) pure;
	virtual int GetWaveDesc(int _iLocal, CWaveform* _pTargetWave) pure;
	virtual CWaveform* GetWaveform(int _iLocal) pure;
	virtual void OpenWaveFile(int _iLocal, CStream_SubFile* NewFile) pure;
	virtual void BuildInto(int _iLocal, CWaveform* _pWave) pure;
	virtual void GetWaveData(int _iLocal, CWaveData &_WaveData) pure;
	virtual int Compress(const char *_pCodecName, float _Quality) pure;
	virtual void GetWaveFileData( int _iLocal, fint& _Pos, fint& _Size ) pure;
	virtual CStr GetWaveName(int _iLocal) pure;
	virtual uint32 GetWaveNameID(int _WaveID) pure;

	virtual const char *GetContainerSortName() {return "";} // Must return a unique name if container does any file io

	virtual bool GetLSD(int _LocalWaveID, void *_pData, int &_rSize) pure;
	virtual bool GetLSDSize(int _LocalWaveID, int &_rSize) pure;
	virtual void LSD_Flush() pure;
};

typedef TPtr<CWaveContainer> spCWaveContainer;

// -------------------------------------------------------------------
struct CSC_IDInfo
{
	int16 m_SCID;					// RC ID used by this texture
	int16 m_Fresh;					// Need to be rebuilt?

	CSC_IDInfo()
	{
		m_SCID = -1;
		m_Fresh = 0;
	}
};

class CSC_WCIDInfo : public CReferenceCount
{
public:
	CSC_IDInfo* m_pWCIDInfo;		// Parallell array to m_pTxtInfo

	CSC_WCIDInfo(int _IDCapacity)
	{
		m_pWCIDInfo = DNew(CSC_IDInfo) CSC_IDInfo[_IDCapacity];
		if (!m_pWCIDInfo) MemError("-");
	}

	~CSC_WCIDInfo()
	{
		if (m_pWCIDInfo != NULL) { delete[] m_pWCIDInfo; m_pWCIDInfo= NULL; }
	}
};

//----------------------------------------------------------------
class CSoundContext;

#include "MSound_Codec.h"

// -------------------------------------------------------------------
//  CSC_SFXDesc
//
// Tells how a sound should be played. Contains diffrent sounds for randomization
// and materials
//
// FIXME: To dynamic, can cause memory fragmentation
//
// -------------------------------------------------------------------
#if 0
class SYSTEMDLLEXPORT CSC_SFXDesc
{
public:

	uint32 m_NameHash;
#ifndef USE_HASHED_SFXDESC
	CStr m_SoundName;
#endif

	uint32 m_Priority:6;
	uint32 m_ContainFlags:18;
	uint32 m_nTemplates:6;
	enum
	{
		EContain_Waves = M_Bit(0),
		EContain_MaterialWaves = M_Bit(1),
		EContain_Attn = M_Bit(2),
		EContain_Pitch = M_Bit(3),
		EContain_Volume = M_Bit(4),
		EContain_DirectCurve = M_Bit(2),
		EContain_LFECurve = M_Bit(2),
		EContain_ReverbCurve = M_Bit(2),
		EContain_DirectCurveLPF = M_Bit(2),
		EContain_ReverbCurveLPF = M_Bit(2),
		EContain_EffectData = M_Bit(5),
	};

#if 0
	// Optional

	// m_nTemplates
	uint32 TemplateHash;
	uint32 TemplateHash;
	uint32 TemplateHash;
	uint32 TemplateHash;

	// EContain_Waves
	uint8 nWaves; // = 4
	uint8 iLastWave;
	int16 iWave;
	int16 iWave;
	int16 iWave;
	int16 iWave;

	// EContain_MaterialWaves
	uint16 nChunkSize; // = ?
	uint16 nMaterial; // = 3
	//
	uint8 iMaterial;
	uint8 iMaterial;
	uint8 iMaterial;
	// Align here to 16 bit
	int16 iOffset;
	int16 iOffset;
	int16 iOffset;

	uint8 nWave; // = 3
	uint8 iLastWave;
	int16 iWave;
	int16 iWave;
	int16 iWave;

	// EContain_Attn
	uint16 AttnMinDist;
	uint16 AttnMaxDist;

	// EContain_Pitch
	uint8 Pitch;
	uint8 PitchRandAmp;

	// EContain_Volume
	uint8 Volume;
	uint8 VolumeRandAmp;

	// EContain_EffectData
	uint8 iEffectChain;
	uint8 nExtra;
	// Align here to 64 bit;
	uint64 Overridden;
	fp32 Extra0;
	fp32 Extra1;
	fp32 Extra2;
	fp32 Extra3;
	fp32 Extra4;

#endif


	class CCompare
	{
	public:
		DIdsPInlineS static aint Compare(const CSymbol *_pFirst, const CSymbol *_pSecond, void *_pContext)
		{
			if (_pFirst->m_NameHash > _pSecond->m_NameHash)
				return 1;
			else (_pFirst->m_NameHash < _pSecond->m_NameHash)
				return -1;
			return 0;
		}

		DIdsPInlineS static aint Compare(const CSymbol *_pTest, uint32 _Key, void *_pContext)
		{
			if (_pFirst->m_NameHash > _Key)
				return 1;
			else (_pFirst->m_NameHash < _Key)
				return -1;
			return 0;
		}
	};

	DIdsTreeAVLAligned_Link(CSC_SFXDesc, m_AVLLink, uint32, CCompare);

	// Returns a wave id from a local id.
	int16 GetWaveId(int16 _iLocal);
	int16 GetPlayWaveId(uint8 _iMaterial = 0);
	
	int16 GetNumWaves();

	// Enumeration
	int32 EnumWaves(int16 *_paWaves, int32 _MaxWaves);

	// Attribute getting and setting
	int32 GetPriority() const {return m_Datas.m_Priority;}
	void SetPriority(int32 _Value) {m_Datas.m_Priority = _Value;}

	fp32 GetPitch() const {return m_Datas.m_Pitch / 32.0;} // Values form 0.0 to 8.0
	void SetPitch(fp32 _Value) {m_Datas.m_Pitch = int(_Value * 32.0);}

	fp32 GetPitchRandAmp() const {return m_Datas.m_PitchRandAmp / 32.0;} // Values form 0.0 to 8.0
	void SetPitchRandAmp(fp32 _Value) {m_Datas.m_PitchRandAmp = int(_Value * 32.0);}

	fp32 GetVolume() const {return m_Datas.m_Volume / 255.0;} // Values form 0.0 to 1.0
	void SetVolume(fp32 _Value) {m_Datas.m_Volume = int(_Value * 255.0);}

	fp32 GetVolumeRandAmp() const {return m_Datas.m_VolumeRandAmp / 255.0;} // Values form 0.0 to 1.0
	void SetVolumeRandAmp(fp32 _Value) {m_Datas.m_VolumeRandAmp = int(_Value * 255.0);}


	fp32 GetAttnMinDist() const {return m_Datas.m_AttnMinDist * 16.0;} 
	void SetAttnMinDist(fp32 _Value) {m_Datas.m_AttnMinDist = int(_Value / 16.0);}

	fp32 GetAttnMaxDist() const {return m_Datas.m_AttnMaxDist * 16.0;}
	void SetAttnMaxDist(fp32 _Value) {m_Datas.m_AttnMaxDist = int(_Value / 16.0);}

	// Duplication
	void operator= (CSC_SFXDesc& _s);

	// Load and Save function
	void Read(CCFile* _pFile, class CWaveContainer_Plain *_pWaveContainer);
	void Write(CCFile* _pFile, class CWaveContainer_Plain *_pWaveContainer);
};
#endif

class SYSTEMDLLEXPORT CWaveContext : public CReferenceCount
{
	TArray<CWaveContainer*> m_lpWC;
	TArray<CWC_WaveIDInfo> m_lWaveIDInfo;
	int m_IDCapacity;
	CIDHeap m_WaveIDHeap;

	TArray<CSoundContext*> m_lpSC;

	int AllocRCID(int _iRC, int _tnr);
	void FreeRCID(int _iRC, int _ID);

public:
	NThread::CMutual m_Lock;

	DECLARE_OPERATOR_NEW

	CWaveContext(int _IDCapacity);
	~CWaveContext();

	class CSC_SFXDesc *GetSFXDesc(const char *_pSFXName);
	int GetWaveID(const char *_pSFXName);
	fp32 SFX_GetLength(CSC_SFXDesc *_pSFXDesc, int _iWave);
	
	CWaveContainer *GetWaveContainer(int _WaveID, int &_iLocal);
	spCSCC_CodecStream OpenStream(int _WaveID, int _Flags);
	void GetWaveData(int _WaveID, CWaveData &_WaveData);
	void GetWaveLoadedData(int _WaveID, CWaveData &_WaveData);

	int GetIDCapacity() { return m_IDCapacity; };
	int AllocID(int _iTC, int _iLocal);
	void FreeID(int _ID);

	int GetWaveDesc(int _WaveID, CWaveform* _pTargetWave);
	CWaveform* GetWaveform(int _WaveID);
	void BuildInto(int _WaveID, CWaveform* _pWave);

	bint IsWaveValid(int _WaveID);

	CStr GetWaveName(int _WaveID);
	uint32 GetWaveNameID(int _WaveID);

	void OpenWaveFile(int _WaveID, CCFile &_File); // Returns the length of the file
	void GetWaveFileData( int _WaveID, fint& _Pos, fint& _Size );

	void MakeDirty(int _ID);

	void LogUsed(CStr _FileName);
	void ClearUsage();

	int AddSoundContext(CSoundContext* _pSC);
	void RemoveSoundContext(int _iSC);
	CSoundContext* GetSoundContext( int _iSC );

	void ClearWaveIDFlags(int _Flags);
	void AddWaveIDFlag(int _WaveID, int _Flags);
	int GetWaveIDFlag(int _WaveID);
	void RemoveWaveIDFlag(int _WaveID, int _Flags);

	int AddWaveClass(CWaveContainer* _pTClass);
	void RemoveWaveClass(int _iTClass);

	// Does a Wave_PrecacheFlush on all CSoundContext's that are registered (and containers)
	void Precache_Flush();
	void LSD_Flush();
};

typedef TPtr<CWaveContext> spCWaveContext;

// -------------------------------------------------------------------
//  CSC_SFXDesc
// -------------------------------------------------------------------
/*
class SYSTEMDLLEXPORT CSC_SFXDescWave
{
public:
	int32 m_iLocalWave;

	CSC_SFXDescWave();

	void Read(CCFile* _pFile, class CWaveContainer_Plain *_pWaveContainer, const char *_pSFXDescName);
	void Write(CCFile* _pFile, class CWaveContainer_Plain *_pWaveContainer);
};
*/

// -------------------------------------------------------------------
//  CSC_SFXDesc
//
// Tells how a sound should be played. Contains diffrent sounds for randomization
// and materials
//
// FIXME: Too dynamic, can cause memory fragmentation
//
// -------------------------------------------------------------------
class CWaveContainer_Plain;
class SYSTEMDLLEXPORT CSC_SFXDesc : public CReferenceCount
{
public:
	// 
	enum
	{
		ENONE=0,
		ENORMAL=1,
		EMATERIAL=2,


		MAXSOUNDLENGTH=24,
	};

	// Name of this descriptor
	// Wave container where the waves are located
	#ifdef USE_HASHED_SFXDESC
		uint32 m_SoundIndex;
		M_FORCEINLINE uint32 GetNameHash() const { return m_SoundIndex; }
	#else
		CStr m_SoundName;
		M_FORCEINLINE uint32 GetNameHash() const { return m_SoundName.StrHash(); }
	#endif


	CWaveContainer_Plain* m_pWaveContainer;

	// Normal mode parameters class
	class CNormalParams
	{
	public:
		CNormalParams()
		{
			m_LastSound = -1;
		}


		int16 m_LastSound;
		TThinArray<int16> m_lWaves;
		int16 GetRandomSound();
	};

	// Material mode parameters class
	class CMaterialParams
	{
	public:
		class CMaterialWaveHolder
		{
		public:
			uint8 m_iMaterial;
			TThinArray<int16> m_lWaves;

			void AddWave(int16 _WaveID);
		};
	
		static const int32 m_HolderHashSize=4;

		TThinArray<CMaterialWaveHolder*> m_alMaterialWaveHolders[m_HolderHashSize];

		~CMaterialParams()
		{
			// Delete all material wave holders and clear the list
			for(int32 h = 0; h < m_HolderHashSize; h++)
			{
				for(int32 i = 0; i < m_alMaterialWaveHolders[h].Len(); i++)
					delete m_alMaterialWaveHolders[h][i];

				m_alMaterialWaveHolders[h].Clear();
			}
		}

		CMaterialWaveHolder *GetMaterial(uint8 _ID, int &_HashPlace);
		CMaterialWaveHolder *CreateMaterial(uint8 _ID);
		CMaterialWaveHolder *GetOrCreateMaterial(uint8 _ID);
		
		void AddWave(uint8 _MaterialID, int16 _WaveID); // Creates the material if nessesery
	};

	// Pointer to hold the current parameters
	void *m_pParams;

	// Data about how the sound should be.
	class CData
	{
		friend class CSC_SFXDesc;
	public:
		uint32 m_Mode:2;
		uint32 m_AttnMinDist:12;		// Closer than this it doesn't get louder
		uint32 m_AttnMaxDist:12;		// At this distance there's no sound
		uint32 m_Priority:6;
		uint8 m_Pitch;
		uint8 m_PitchRandAmp;
		uint8 m_Volume;
		uint8 m_VolumeRandAmp;
		uint16 m_Category;
	};

	CData m_Datas;

	// The constructor
	CSC_SFXDesc();
	~CSC_SFXDesc()
	{
		SetMode(ENONE); // Clear everything
	}

	void SetWaveContatiner(CWaveContainer_Plain *_pWaveContainer);

	// Mode and params
	void SetMode(int8 _Mode)
	{
		if(m_Datas.m_Mode == _Mode)
			return;
		
		// Remove old params
		if(m_pParams)
		{
			if(m_Datas.m_Mode == ENORMAL)
				delete (CNormalParams*)m_pParams;
			else if(m_Datas.m_Mode == EMATERIAL)
				delete (CMaterialParams*)m_pParams;
			else
				Error("SetMode", "Parameters exist but no destructor for this mode");


			m_pParams = NULL;
		}

		// Create new params
		if(_Mode == ENORMAL)
			m_pParams = DNew(CNormalParams) CNormalParams;
		else if(_Mode == EMATERIAL)
			m_pParams = DNew(CMaterialParams) CMaterialParams;

		// Set mode
		m_Datas.m_Mode = _Mode;
	}
	M_INLINE int8 GetMode() { return m_Datas.m_Mode; };

	M_INLINE CNormalParams *GetNormalParams() { return (CNormalParams*)m_pParams; };
	//M_INLINE CWaveParams *GetWaveParams() { return (CWaveParams*)m_pParams; };
	M_INLINE CMaterialParams *GetMaterialParams() { return (CMaterialParams*)m_pParams; };

	// Returns a wave id from a local id.
	int16 GetWaveId(int16 _iLocal);
	int16 GetContainerWaveId(int16 _iLocal);
	int16 GetPlayWaveId(uint8 _iMaterial = 0);
	
	int16 GetNumWaves();

	// Enumeration
	int32 EnumWaves(int16 *_paWaves, int32 _MaxWaves);

	// Attribute getting and setting
	int32 GetPriority() const {return m_Datas.m_Priority;}
	void SetPriority(int32 _Value) {m_Datas.m_Priority = _Value;}

	fp32 GetPitch() const {return m_Datas.m_Pitch / 32.0;} // Values form 0.0 to 8.0
	void SetPitch(fp32 _Value) {m_Datas.m_Pitch = int(_Value * 32.0);}

	fp32 GetPitchRandAmp() const {return m_Datas.m_PitchRandAmp / 32.0;} // Values form 0.0 to 8.0
	void SetPitchRandAmp(fp32 _Value) {m_Datas.m_PitchRandAmp = int(_Value * 32.0);}

	fp32 GetVolume() const {return m_Datas.m_Volume / 255.0;} // Values form 0.0 to 1.0
	void SetVolume(fp32 _Value) {m_Datas.m_Volume = int(_Value * 255.0);}

	fp32 GetVolumeRandAmp() const {return m_Datas.m_VolumeRandAmp / 255.0;} // Values form 0.0 to 1.0
	void SetVolumeRandAmp(fp32 _Value) {m_Datas.m_VolumeRandAmp = int(_Value * 255.0);}


	fp32 GetAttnMinDist() const {return m_Datas.m_AttnMinDist * 16.0;} 
	void SetAttnMinDist(fp32 _Value) {m_Datas.m_AttnMinDist = int(_Value / 16.0);}

	fp32 GetAttnMaxDist() const {return m_Datas.m_AttnMaxDist * 16.0;}
	void SetAttnMaxDist(fp32 _Value) {m_Datas.m_AttnMaxDist = int(_Value / 16.0);}

	uint16 GetCategory() const {return m_Datas.m_Category;}
	void SetCategory(uint8 _Category) {m_Datas.m_Category = _Category;}

	// Duplication
	void operator= (CSC_SFXDesc& _s);

	// Load and Save function
	void Read(CCFile* _pFile, class CWaveContainer_Plain *_pWaveContainer);
	void Write(CCFile* _pFile, class CWaveContainer_Plain *_pWaveContainer);
};

// -------------------------------------------------------------------
//
//  CWaveContainer_Plain
//
// -------------------------------------------------------------------
class SYSTEMDLLEXPORT CWC_Waveform : public CReferenceCount
{
public:
	enum
	{
		MAXNAMELENGTH=24, // Warning, changing this will change the wavecontainer format, proceed with caution.
	};

	int m_WaveID;
#ifdef USE_HASHED_WAVENAME
	uint32 m_NameID;
#else
	CStr m_Name;
	//char m_Name[MAXNAMELENGTH];
#endif

#ifdef PLATFORM_CONSOLE	
#ifdef M_Profile
	spCWaveform m_spWave;
#endif
#else
	spCWaveform m_spWave;
#endif


	int32 m_FilePos;
	int32 m_SubLength;

	void operator= (const CWC_Waveform& _s)
	{
		m_WaveID = _s.m_WaveID;
#ifdef USE_HASHED_WAVENAME
		m_NameID = _s.m_NameID;
#else
		m_Name = _s.m_Name;
#endif
#if !defined(PLATFORM_CONSOLE) || defined(M_Profile)
		m_spWave = _s.m_spWave;
#endif
		m_FilePos = _s.m_FilePos;
		m_SubLength = _s.m_SubLength;
	}


	DECLARE_OPERATOR_NEW

	CWC_Waveform();
	void Read(CDataFile* _pDFile, bool _bDynamic = false);
	void Write(CDataFile* _pDFile);

	void WriteData(CCFile* _pFile);
	void WriteInfo(CCFile* _pFile);

	void ReadInfo(CCFile* _pFile, bool _bDynamic = false);
	void ReadData(CCFile* _pFile);

	void ReadDynamic(const CStr& _Filename);
};

typedef TPtr<CWC_Waveform> spCWC_Waveform;

// -------------------------------------------------------------------
//
// 
//
// -------------------------------------------------------------------
class SYSTEMDLLEXPORT CWaveContainer_Plain : public CWaveContainer
{
	int m_bDynamic;

	TArray<CWC_Waveform> m_lWaves;
	TThinArray<CWaveData> m_lWaveData;

	TArray<CSC_SFXDesc> m_lSFXDesc;
	TArray<uint16> m_lSFXDescOrder;
	int m_UsedSFXDesc;
	CStr m_Path;

	// lip data
	int32 m_LipDataFileOffset;
	int32 m_LipDataRefCount;
	TThinArray<uint32> m_lLipIndexData;

	void LoadIndexData();
	void UnloadIndexData();

	void LoadLSD(CCFile *_pFile, int32 _WaveCount, int32 _Start);

private:

//#ifndef PLATFORM_CONSOLE
	// used when compiling wave containers
	struct SLipDataSet
	{
		void *m_pData;
		int32 m_DataSize;
		int16 m_LocalWaveID;
		fp32 m_TimeCode;
	};
	TArray<SLipDataSet> m_lLipDataSets;
//#endif


#ifdef USE_HASHED_WAVENAME
	class CIDHash*   m_pHash;
#else
	CStringHashConst m_Hash;
#endif
	int m_bHashHash;

#ifdef	PLATFORM_PS2
	fint	m_ContainerDiscLocation;
#endif	// PLATFORM_PS2

	void CreateHash();
	void DestroyHash();

public:

	DECLARE_OPERATOR_NEW

	CWaveContainer_Plain();
	~CWaveContainer_Plain();

	int GetSFXCount();
	int GetWaveCount();
	int GetIndex(CStr _Name);
	int GetIndex(uint32 _NameID);
	const char *GetName(int _iLocal);

	CStr GetWaveName(int _iLocal);
	uint32 GetWaveNameID(int _iLocal);

	void AddSFXDesc(CSC_SFXDesc &_SFXDesc);

	const char *GetContainerSortName() {return m_Path.Str();}

	//
//	TArray<int16> m_lSFXDescWaves;
//	uint16 AllocSFXDescWave(uint16 _Num);
//	void SFXDescWaveTrim();

	// Overrides:
	virtual int GetWaveID(int _iLocal);
	virtual int GetWaveDesc(int _iLocal, CWaveform* _pTargetWave);
	virtual CWaveform* GetWaveform(int _iLocal);
	virtual void OpenWaveFile(int _iLocal, CStream_SubFile* NewFile);
	virtual void BuildInto(int _iLocal, CWaveform* _pWave);
	virtual void GetWaveData(int _iLocal, CWaveData &_WaveData);
	virtual void GetWaveFileData( int _iLocal, fint& _Pos, fint& _Size );

	float GetWaveSampleRate(int _iLocal);
	CStr GetFileName(){return m_Path;};

	virtual bool GetLSD(int _LocalWaveID, void* _rpData, int &_rSize);
	virtual bool GetLSDSize(int _LocalWaveID, int &_rSize);
	virtual void LSD_Flush();

	// -------------------------------------------------------------------
	int AddRAW(CStr _Filename, CStr _Name, int _Fmt = WAVE_FORMAT_8BIT, int _nChn = 1, bool _bUnsigned = false);
	int AddWaveTable(spCWC_Waveform _spWave);
	int AddWaveTable(spCWaveform _spWave, CStr _Name);
	int AddWaveTable(CStr _Filename, CStr _Name, const char *_pCodecName, float _Priority, float _Quality, float _LipSyncSampleRate = 0.0, int _Flags = 0);
	int Compress(const char *_pCodecName, float _Quality);
//	int AddFromWaveList(CDataFile* _pDFile);
	int AddFromWaveInfo(CDataFile* _pDFile);
	void AddXWC(CStr _Filename, bool _bDynamic);
#ifndef PLATFORM_CONSOLE
	void FilterWaves(TArray<CStr> *_pValidWaves);
	void AddFiltered(CWaveContainer_Plain *_pSrcXWC, TArray<CStr> *_pAllowed, int _Flags = 3);
#endif // PLATFORM_CONSOLE

	void AddFromScript(CStr _Filename);

//	void WriteWaveList(CDataFile* _pDFile);
	void WriteWaveData(CDataFile* _pDFile);
	void WriteWaveInfo(CDataFile* _pDFile);
	void WriteWaveDesc(CDataFile* _pDFile);
	void WriteSFXDescs(CDataFile* _pDFile);
	void WriteLSD(CDataFile* _pDFile);
	void WriteXWC(CStr _Filename);
	void WriteXWCStrippedData(CStr _Filename);

	int16 GetLocalWaveID(const char* _WaveName);
	CWC_Waveform *GetWaveform(const char* _WaveName);
	int GetSFXDescIndex(const char* _SoundName);
	CSC_SFXDesc* GetSFXDesc(int _iSound);

	#ifndef PLATFORM_CONSOLE
		// MikeW: Only used by XWC_Static. Required for partial sound XWC updates.
		// NOTE: Takes ownership of _pData!!
		void AddLipDataSet(int _iLocalWaveID, fp32 _TimeCode, int32 _DataSize, void * _pData);
		bool GetLipDataSet(int _iLocalWaveID, fp32 &_oTimeCode, int32 &_oDataSize, void **_opData);
	#endif

};

typedef TPtr<CWaveContainer_Plain> spCWaveContainer_Plain;

// -------------------------------------------------------------------
//
//  CSoundContext
//
// -------------------------------------------------------------------

class CSC_3DOcclusion
{
public:
	CSC_3DOcclusion();
	void SetDefault();
	fp32 m_Occlusion; // 1.0 = fully wet occlusion
	fp32 m_Obstruction; // 1.0  = fully wet obstruction
};

class CSC_VoiceFalloff
{
public:
	class CEntry
	{
	public:
		fp32 m_Position;
		fp32 m_Volume;
	};
	TThinArray<CEntry> m_Values;
};

class CSC_VoiceCreate3DProperties
{
public:
	void SetDefault()
	{
		m_Position = 0.0f;
		m_Velocity = 0.0f;
		m_MinDist = 0.0f;
		m_MaxDist = 0.0f;
		m_Orientation = 0.0f;
		m_Occlusion.SetDefault();
		m_pDirectFalloff = NULL;
		m_pReverbFalloff = NULL;
	}

	CVec3Dfp32 m_Position;
	CVec3Dfp32 m_Velocity;
	CVec3Dfp32 m_Orientation;
	CSC_3DOcclusion m_Occlusion;
	fp32 m_MinDist;
	fp32 m_MaxDist;

	// Set the falloff curve, min dist will become ineffective
	CSC_VoiceFalloff *m_pDirectFalloff;
	CSC_VoiceFalloff *m_pReverbFalloff;
};

enum
{
	SC_SNDSPATIALIZATION_NONE,
	SC_SNDSPATIALIZATION_3D,
	SC_SNDSPATIALIZATION_WIDESOUND,
};

class CSC_VoiceCreateParams
{
public:
	CSC_VoiceCreateParams()
	{
		Clear();
	}
	void Clear()
	{
		m_hChannel = -1;
		m_WaveID = -1;
		m_Pitch = 1.0f;
		m_Priority = 0;
		m_Volume = 1.0f;
		m_MinDelay = 0.0f;
		m_bLoop = false;
		m_bStartPaused = false;
		m_bIsMusic = false;
		m_bDoppler = false;
		m_bFalloff = false;
		m_DSPId = 0; // Signifies default chain (2d choosen for )
		m_Spatialization = SC_SNDSPATIALIZATION_NONE;
		m_3DProperties.SetDefault();
	}

	int32 m_hChannel;
	int32 m_WaveID;
	int32 m_Priority;
	uint32 m_DSPId;
	fp32 m_Pitch;
	fp32 m_Volume;
	fp32 m_MinDelay;
	uint32 m_Category:16;
	uint32 m_Spatialization:4;
	uint32 m_bDoppler:1;
	uint32 m_bFalloff:1;
	uint32 m_bLoop:1;
	uint32 m_bStartPaused:1;
	uint32 m_bIsMusic:1;

	CSC_VoiceCreate3DProperties m_3DProperties;

};


class CSC_VoiceCreateParams3D : public CSC_VoiceCreateParams
{
public:
	CSC_VoiceCreateParams3D()
	{
		m_Spatialization = SC_SNDSPATIALIZATION_3D;
		m_bFalloff = true;
		m_bDoppler = true;
	}
};


#if 0

class CSC_Category
{
public:

	CSC_Category()
	{
		m_2DRPGVolume = 1.0;
		m_2DGuiCubeVolume = 1.0;
		m_2DCutsceneVolume = 1.0;
		m_2DGameVolume = 1.0;
		m_2DCGPlaySoundVolume = 1.0;
		m_2DCGPlaySoundSfxVolVolume = 1.0;
		m_2DVideoVolume = 1.0;

		m_3DVideoVolume = 1.0;
		m_3DRPGVolume = 1.0;
		m_3DGameVolume = 1.0;
		m_3DGameLoopingVolume = 1.0;

		for (int i = 0; i < 256; ++i)
			m_MixCategories[i] = 1.0;
		for (int i = 0; i < 256; ++i)
			m_2DSoundNess[i] = 0;
	}

	fp32 m_2DRPGVolume;
	fp32 m_2DGuiCubeVolume;
	fp32 m_2DCutsceneVolume;
	fp32 m_2DGameVolume;
	fp32 m_2DCGPlaySoundVolume;
	fp32 m_2DCGPlaySoundSfxVolVolume;
	fp32 m_2DVideoVolume;

	fp32 m_3DVideoVolume;
	fp32 m_3DRPGVolume;
	fp32 m_3DGameVolume;
	fp32 m_3DGameLoopingVolume;

	fp32 m_MixCategories[256];
	int32 m_2DSoundNess[256];

	void ReadSettings(CRegistry *_pRegistry);
};

#endif


// -------------------------------------------------------------------
class SYSTEMDLLEXPORT CSoundContext : public CConsoleClient
{
public:
	virtual void Create(CStr _Param) {};

	virtual void Init(int _MaxVoices) pure;
	virtual void KillThreads() pure;

	virtual NThread::CMutual &GetInterfaceLock() pure;

	// Access function for CWaveContext
	virtual CSC_WCIDInfo* GetWCIDInfo() pure;
	virtual CSC_SFXDesc *GetSFXDesc(const char *_pSFXName) pure;

	// Channel management
	virtual int Chn_Alloc(int _nVoices = 1) pure;
	virtual void Chn_Free(int _hChn) pure;
	virtual void Chn_SetVolume(int _hChn, float _Volume) pure;
	virtual fp32 Chn_GetVolume(int _hChn) pure;
	virtual void Chn_SetPitch(int _hChn, float _Pitch) pure;
	virtual void Chn_Play(int _hChn) pure;
	virtual void Chn_Pause(int _hChn) pure;
	virtual void Chn_DestroyVoices(int _hChn) pure;

	// Lip Sync Data
	virtual void *LSD_Get(int16 _WaveID) pure;
	virtual fp32 Timecode_Get(int16 _WaveID) pure;

	// Voice creation
	virtual void Voice_InitCreateParams(CSC_VoiceCreateParams &_CreateParams, uint32 _Category) pure;
	
	virtual int Voice_Create(const CSC_VoiceCreateParams &_CreateParams) pure;

//	virtual int Voice_Create3D(int _hChn, int _WaveID, int _Priority, fp32 _Pitch, fp32 _Volume, const CSC_3DProperties& _Properties, bool _bLoop = false, bool _bStartPaused = false, fp32 _MinDelay = 0.0f) pure;

	virtual void Voice_Destroy(int _hVoice) pure;
	virtual bool Voice_IsPlaying(int _hVoice) pure;

	// Voice data
	virtual CMTime Voice_GetPlayPos(int _hVoice) pure;

	virtual CMTime SFX_GetLength(CSC_SFXDesc *_pSFXDesc, int _iWave) pure;
	
	// Property modification
	virtual void Voice_SetPitch(int _hVoice, fp32 _Pitch) pure;
	virtual void Voice_SetVolume(int _hVoice, fp32 _Volume) pure;
	virtual void Voice_Unpause(int _hVoice, uint32 _PauseSlot = 1) pure;
	virtual void Voice_Pause(int _hVoice, uint32 _PauseSlot = 1) pure;

	virtual bint Voice_ReadyForPlay(int _hVoice) pure;

	// 3D-Property modification
	virtual void Voice3D_SetOrigin(int _hVoice, const CVec3Dfp32& _Pos) pure;
	virtual void Voice3D_SetOrientation(int _hVoice, const CVec3Dfp32& _Orientation) {}
	virtual void Voice3D_SetVelocity(int _hVoice, const CVec3Dfp32& _Vel) pure;
	virtual void Voice3D_SetMinMaxDist(int _hVoice, fp32 _MinDist, fp32 _MaxDist) pure;
	virtual void Voice3D_SetOcclusion(int _hVoice, const CSC_3DOcclusion &_Prop) pure;
	
	// Precache
	virtual void Wave_PrecacheFlush() pure;
	virtual void Wave_PrecacheBegin( int _Count ) pure;
	virtual void Wave_Precache(int _WaveID) pure;
	virtual int Wave_GetMemusage(int _WaveID){ return 0;}
	virtual void Wave_PrecacheEnd(uint16 *_pPreCacheOrder, int _nIds) pure;
	virtual CWaveContext *Wave_GetContext() pure;

	// 3D-Listener modifactions
	virtual void Listen3D_SetCoordinateScale(fp32 _Scale) pure;
	virtual void Listen3D_SetOrigin(const CMat4Dfp32& _Pos) pure;
	virtual void Listen3D_SetVelocity(const CVec3Dfp32& _Vel) pure;

	// Music
	virtual void Music_SetSystemVolume(fp32 _Volume) pure; // Set by system when player overrides game music from console
	virtual void Music_SetSettingsVolume(fp32 _Volume) pure; // Volume that player sets for music
	virtual void Music_SetGameVolume(fp32 _Volume) pure; // Volume that can be set from gameplay code to disable music

	// Filter
	class CFilter
	{
	public:
		void SetDefault();

		template <typename t_CType>
		class TCMinMax
		{
		public:
			t_CType m_Min;
			t_CType m_Max;

			void Set(t_CType _Min, t_CType _Max)
			{
				m_Min = _Min;
				m_Max = _Max;
			}

			t_CType Clamp(t_CType _In)
			{
				return ::Clamp(_In, m_Min, m_Max);
			}
		};

		fp32 m_RoomRolloffFactor;
		fp32 m_DecayTime;
		fp32 m_DecayHFRatio;
		fp32 m_ReflectionsDelay;
		fp32 m_ReverbDelay;
		fp32 m_Diffusion;
		fp32 m_Density;
		fp32 m_HFReference;

		fp32 m_Room;
		fp32 m_RoomHF;
		fp32 m_Reflections;
		fp32 m_Reverb;

		// PC Parameters

		fp32 m_EnvironmentSize;
		fp32 m_DecayLFRatio;

		fp32 m_EchoTime;
		fp32 m_EchoDepth;
		fp32 m_ModulationTime;
		fp32 m_ModulationDepth;
		fp32 m_AirAbsorptionHF;
		fp32 m_LFReference;

		fp32 m_RoomLF;

		class CMinMax
		{
		public:
			CMinMax();

			TCMinMax<fp32> m_RoomRolloffFactor;
			TCMinMax<fp32> m_DecayTime;
			TCMinMax<fp32> m_DecayHFRatio;
			TCMinMax<fp32> m_ReflectionsDelay;
			TCMinMax<fp32> m_ReverbDelay;
			TCMinMax<fp32> m_Diffusion;
			TCMinMax<fp32> m_Density;
			TCMinMax<fp32> m_HFReference;

			TCMinMax<fp32> m_Room;
			TCMinMax<fp32> m_RoomHF;
			TCMinMax<fp32> m_Reflections;
			TCMinMax<fp32> m_Reverb;

			// PC Parameters

			TCMinMax<fp32> m_EnvironmentSize;
			TCMinMax<fp32> m_DecayLFRatio;

			TCMinMax<fp32> m_EchoTime;
			TCMinMax<fp32> m_EchoDepth;
			TCMinMax<fp32> m_ModulationTime;
			TCMinMax<fp32> m_ModulationDepth;
			TCMinMax<fp32> m_AirAbsorptionHF;
			TCMinMax<fp32> m_LFReference;

			TCMinMax<fp32> m_RoomLF;
		};

		static CMinMax ms_MinMax;
	};

	// Dual Stream
	virtual void MultiStream_Play(const char * *_Files, int _nFiles, bool _FadeIn, bool _bResetVolumes, uint32 _LoopMask = 0xffffffff) pure;
	virtual void MultiStream_Stop(bool _FadeOut) pure;
	virtual void MultiStream_Volumes(fp32 *_Volumes, int _nVolumes, bool _Fade, fp32 *_pFadeSpeeds) pure;
	virtual void MultiStream_Volume(fp32 _Volume) pure;
	virtual void MultiStream_Pause() pure;
	virtual void MultiStream_Resume() pure;

	// Refresh
	virtual void Refresh() pure;

	// FIXME, remove from rtm at rtm
//#ifndef M_RTM
	virtual const char **GetDebugStrings() {return NULL;};
//#endif

	MACRO_OPERATOR_TPTR(CSoundContext);
};

typedef TPtr<CSoundContext> spCSoundContext;

// -------------------------------------------------------------------
SYSTEMDLLEXPORT spCSoundContext MCreateSoundContext(CStr _ClassName, int _MaxVoices);


class CPrecacheWaveCompare
{
public:
	static int Compare(CWaveContext* _pContext, uint16 _i0, uint16 _i1)
	{
		MAUTOSTRIP(RegistryCompare, 0);
		int iLocalFirst;
		int iLocalSecond;
		CWaveContainer *pTCFirst = _pContext->GetWaveContainer(_i0, iLocalFirst);
		CWaveContainer *pTCSecond = _pContext->GetWaveContainer(_i1, iLocalSecond);
		if (pTCFirst != pTCSecond)
		{
			const char *pFirstStr = pTCFirst->GetContainerSortName();
			const char *pSecondStr = pTCSecond->GetContainerSortName();
			if (pFirstStr != pSecondStr)
			{
				int iCmp = CStrBase::stricmp(pFirstStr, pSecondStr);
				if (iCmp != 0)
					return iCmp;
			}
		}

		if (iLocalFirst > iLocalSecond)
			return 1;
		else if (iLocalFirst < iLocalSecond)
			return -1;

		return 0;
	}
};



#endif
