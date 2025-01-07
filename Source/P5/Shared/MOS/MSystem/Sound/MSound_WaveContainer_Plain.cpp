#include "PCH.h"

#include "../MSystem.h"
#include "MSound.h"
#include "LipSync/LipSync.h"

class CSortWCNonHashed
{
public:
	static int Compare(TAP_RCNRTM<CSC_SFXDesc> *_pContext, uint16 _i0, uint16 _i1)
	{
#ifdef USE_HASHED_SFXDESC
		return 0;
#else
//		const char *p0 = (*_pContext)[_i0]->m_SoundName.Str();
//		const char *p1 = (*_pContext)[_i1]->m_SoundName.Str();
		return (*_pContext)[_i0].m_SoundName.CompareNoCase((*_pContext)[_i1].m_SoundName);
#endif
	}
};
class CSortWCHashed
{
public:
	static int Compare(TAP_RCNRTM<CSC_SFXDesc> *_pContext, uint16 _i0, uint16 _i1)
	{
#ifdef USE_HASHED_SFXDESC
		uint32 SoundID0 = (*_pContext)[_i0].m_SoundIndex;
		uint32 SoundID1 = (*_pContext)[_i1].m_SoundIndex;
#else
		uint32 SoundID0 = StringToHash((*_pContext)[_i0].m_SoundName.Str());
		uint32 SoundID1 = StringToHash((*_pContext)[_i1].m_SoundName.Str());
#endif
		if (SoundID0 > SoundID1)
			return 1;
		else if (SoundID0 < SoundID1)
			return -1;
		return 0;
	}
};


// -------------------------------------------------------------------
//  CWaveContainer
// -------------------------------------------------------------------
CWaveContainer::CWaveContainer()
{
	MAUTOSTRIP( CWaveContainer_ctor, MAUTOSTRIP_VOID );

	m_pWaveContext = NULL;
	m_iWaveClass = -1;

	// Get WaveContext
	MACRO_GetRegisterObject(CWaveContext, pWC, "SYSTEM.WAVECONTEXT");
	if (!pWC) Error("-", "No wave-context object.");
	m_pWaveContext = pWC;

	// Register container
	m_iWaveClass = m_pWaveContext->AddWaveClass(this);
}

CWaveContainer::~CWaveContainer()
{
	MAUTOSTRIP( CWaveContainer_dtor, MAUTOSTRIP_VOID );

	if (m_iWaveClass >= 0)
	{
		if (m_pWaveContext) 
			m_pWaveContext->RemoveWaveClass(m_iWaveClass);
	}

	m_pWaveContext = NULL;
	m_iWaveClass = -1;
}

// -------------------------------------------------------------------
//  CWaveContainer_Plain
// -------------------------------------------------------------------

#ifdef USE_HASHED_WAVENAME
	//TODO: share this class with texturecontext..
	struct CIDHashElement { uint32 m_Value; };

	class CIDHash : public THash<int16, CIDHashElement>
	{
		int GetHashIndex(uint32 _Value) const { return _Value & 1023; }

	public:
		void Create(int _nIndices)
		{
			THash<int16, CIDHashElement>::Create(_nIndices, 1024, false);
		}

		void Insert(int _Index, uint32 _Value)
		{
			THash<int16, CIDHashElement>::Remove(_Index);
			THash<int16, CIDHashElement>::Insert(_Index, GetHashIndex(_Value));
			CHashIDInfo& IDInfo = m_pIDInfo[_Index];
			IDInfo.m_Value = _Value;
		}

		int GetIndex(uint32 _Value) const
		{
			int ID = m_pHash[GetHashIndex(_Value)];
			while (ID != -1)
			{
				if (m_pIDInfo[ID].m_Value == _Value)
					return ID;
				ID = m_pIDInfo[ID].m_iNext;
			}
			return -1;
		}
	};
#endif



IMPLEMENT_OPERATOR_NEW(CWaveContainer_Plain);


//
//
//
CWaveContainer_Plain::CWaveContainer_Plain()
{
	MAUTOSTRIP( CWaveContainer_Plain_ctor, MAUTOSTRIP_VOID);

	m_bDynamic = false;
	m_UsedSFXDesc = 0;
	m_lWaves.SetGrow(64);
	m_lSFXDesc.SetGrow(64);
	m_bHashHash = false;

	// lip
	//m_lLipIndexData = NULL;
	m_LipDataFileOffset = 0;
	m_LipDataRefCount = 0;
	m_LipDataFileOffset = 0;

#ifdef USE_HASHED_WAVENAME
	m_pHash = NULL;
#endif
}

//
///
//
CWaveContainer_Plain::~CWaveContainer_Plain()
{
	MAUTOSTRIP( CWaveContainer_Plain_dtor, MAUTOSTRIP_VOID);

	DLock(m_pWaveContext->m_Lock);

	if (m_pWaveContext && m_iWaveClass >= 0)
	{
		for(int i = 0; i < m_lWaves.Len(); i++)
			if (m_lWaves[i].m_WaveID > 0)
				m_pWaveContext->FreeID(m_lWaves[i].m_WaveID);
	}
#ifdef USE_HASHED_WAVENAME
	delete m_pHash;
#endif

	#ifndef PLATFORM_CONSOLE
	for(int32 i = 0; i < m_lLipDataSets.Len(); i++)
		if(m_lLipDataSets[i].m_pData)
		{
			delete [] m_lLipDataSets[i].m_pData;
			m_lLipDataSets[i].m_pData = NULL;
		}
	#endif

	m_lWaves.Clear();
	m_lWaveData.Clear();
	m_lSFXDesc.Clear();
	m_lSFXDescOrder.Clear();
	m_lLipIndexData.Clear();
	m_lLipDataSets.Clear();

}

//
//
//
void CWaveContainer_Plain::CreateHash()
{
	MAUTOSTRIP( CWaveContainer_Plain_CreateHash, MAUTOSTRIP_VOID);

	MSCOPESHORT(CWaveContainer_Plain::CreateHash);
	int nWaves = m_lWaves.Len();

#ifdef USE_HASHED_WAVENAME
	M_ASSERT(!m_pHash, "!");
	m_pHash = MNew( CIDHash );
	m_pHash->Create(nWaves);
	for (int iW=0; iW<nWaves; iW++)
		m_pHash->Insert(iW, m_lWaves[iW].m_NameID);
#else
	m_Hash.Create(nWaves, false, 10);
	for(int iW = 0; iW < nWaves; iW++)
		m_Hash.Insert(iW, m_lWaves[iW].m_Name);
#endif

	m_bHashHash = true;
}

//
//
//
void CWaveContainer_Plain::DestroyHash()
{
	MAUTOSTRIP( CWaveContainer_Plain_DestroyHash, MAUTOSTRIP_VOID);

	MSCOPESHORT(CWaveContainer_Plain::DestroyHash);
	m_bHashHash = false;

#ifdef USE_HASHED_WAVENAME
	if (m_pHash)
		m_pHash->Clear();
	delete m_pHash;
	m_pHash = NULL;
#else
	m_Hash.Clear();
#endif
}

//
//
//
int CWaveContainer_Plain::GetIndex(uint32 _NameID)
{
	MAUTOSTRIP( CWaveContainer_Plain_GetIndex, -1 );
	MSCOPESHORT(CWaveContainer_Plain::GetIndex);

	if (m_bHashHash)
	{
#ifdef USE_HASHED_WAVENAME
		return m_pHash->GetIndex(_NameID);
#else
		Error("CWaveContainer_Plain::GetIndex", "USE_HASHED_WAVENAME not defined");
#endif
	}
	else
	{
		int nWaves = m_lWaves.Len();
#ifdef USE_HASHED_WAVENAME
		for (int iW=0; iW<nWaves; iW++)
			if (m_lWaves[iW].m_NameID == _NameID)
				return iW;
#else
		Error("CWaveContainer_Plain::GetIndex", "USE_HASHED_WAVENAME not defined");
#endif
	}

	return -1;
}

//
//
//
int CWaveContainer_Plain::GetIndex(CStr _Name)
{
	MAUTOSTRIP( CWaveContainer_Plain_GetIndex, -1 );
	MSCOPESHORT(CWaveContainer_Plain::GetIndex);

	if (m_bHashHash)
	{
#ifdef USE_HASHED_WAVENAME
		return m_pHash->GetIndex(StringToHash(_Name));
#else
		return m_Hash.GetIndex(_Name);
#endif
	}
	else
	{
		int nWaves = m_lWaves.Len();
#ifdef USE_HASHED_WAVENAME
		uint32 NameID = StringToHash(_Name);
		for (int iW=0; iW<nWaves; iW++)
			if (m_lWaves[iW].m_NameID == NameID)
				return iW;
#else
		_Name = _Name.UpperCase();
		for(int iW = 0; iW < nWaves; iW++)
			if (CStrBase::stricmp(m_lWaves[iW].m_Name.Str(), _Name) == 0) return iW;
#endif
	}

	return -1;
}

//
//
//
int CWaveContainer_Plain::GetWaveCount()
{
	MAUTOSTRIP( CWaveContainer_Plain_GetWaveCount, 0 );
	return m_lWaves.Len();
}

//
//
//
int CWaveContainer_Plain::GetSFXCount()
{
	MAUTOSTRIP( CWaveContainer_Plain_GetSFXCount, 0 );
	return m_lSFXDesc.Len();
}

//
//
//
const char *CWaveContainer_Plain::GetName(int _iLocal)
{
	MAUTOSTRIP( CWaveContainer_Plain_GetName, NULL );
#ifdef USE_HASHED_WAVENAME
//	return CFStrF("%08x", m_NameID);
//#	ifdef PLATFORM_DOLPHIN
		return "Undefined";
//#	else
//		Error("GetName", "Can't get name with 'USE_HASHED_WAVENAME' defined!");
//		return NULL;
//#	endif
#else
	if (!m_lWaves.ValidPos(_iLocal))
		Error("GetName", CStrF("Invalid local ID. (%d/%d)", _iLocal, m_lWaves.Len()));

	return m_lWaves[_iLocal].m_Name;
#endif
}

CStr CWaveContainer_Plain::GetWaveName(int _iLocal)
{
	MAUTOSTRIP( CWaveContainer_Plain_GetName, NULL );
#ifdef USE_HASHED_WAVENAME
	if (!m_lWaves.ValidPos(_iLocal))
		Error("GetName", CStrF("Invalid local ID. (%d/%d)", _iLocal, m_lWaves.Len()));
	return CFStrF("%08x", m_lWaves[_iLocal].m_NameID);
//#	ifdef PLATFORM_DOLPHIN
//		return "Undefined";
//#	else
//		Error("GetName", "Can't get name with 'USE_HASHED_WAVENAME' defined!");
//		return NULL;
//#	endif
#else
	if (!m_lWaves.ValidPos(_iLocal))
		Error("GetName", CStrF("Invalid local ID. (%d/%d)", _iLocal, m_lWaves.Len()));

	return CFStrF("%08x", StringToHash(m_lWaves[_iLocal].m_Name));
#endif
}
//
//
//
uint32 CWaveContainer_Plain::GetWaveNameID(int _iLocal)
{
	MAUTOSTRIP( CWaveContainer_Plain_GetName, NULL );
#ifdef USE_HASHED_WAVENAME
	return m_lWaves[_iLocal].m_NameID;
#else
	return StringToHash(m_lWaves[_iLocal].m_Name);
#endif
	return 0;
}



// -------------------------------------------------------------------
//
//
//
int CWaveContainer_Plain::GetWaveID(int _iLocal)
{
	MAUTOSTRIP( CWaveContainer_Plain_GetWaveID, 0 );

	if (!m_lWaves.ValidPos(_iLocal))
		Error("GetWaveID", CStrF("Invalid local ID. (%d/%d)", _iLocal, m_lWaves.Len()));

	return m_lWaves[_iLocal].m_WaveID;
}

//
//
//
int CWaveContainer_Plain::GetWaveDesc(int _iLocal, CWaveform* _pTargetWave)
{
	MAUTOSTRIP( CWaveContainer_Plain_GetWaveDesc, 0 );
#ifdef PLATFORM_CONSOLE
	Error_static("CWaveContainer_Plain::GetWaveDesc", "Not supported!");
	return 0;
#else
	if (!m_lWaves.ValidPos(_iLocal))
		Error("GetWaveDesc", CStrF("Invalid local ID. (%d/%d)", _iLocal, m_lWaves.Len()));

	m_lWaves[_iLocal].ReadDynamic(m_Path);
	CWaveform* pW = m_lWaves[_iLocal].m_spWave;
	if (!pW) Error("GetWaveDesc", "Tried to access unloaded dynamic waveform.");

	_pTargetWave->CreateVirtual(pW->GetWidth(), pW->GetHeight(), pW->GetFormat(), pW->GetMemModel());
	return 0;
#endif
}

//
//
//
CWaveform* CWaveContainer_Plain::GetWaveform(int _iLocal)
{
	MAUTOSTRIP( CWaveContainer_Plain_GetWaveform, NULL );
#ifdef PLATFORM_CONSOLE
	Error_static("CWaveContainer_Plain::GetWaveform", "Not supported!");
	return 0;
#else
	if (!m_lWaves.ValidPos(_iLocal))
		Error("GetWaveform", CStrF("Invalid local ID. (%d/%d)", _iLocal, m_lWaves.Len()));

	m_lWaves[_iLocal].ReadDynamic(m_Path);
	return m_lWaves[_iLocal].m_spWave;
#endif
}

//
//
//
void CWaveContainer_Plain::OpenWaveFile(int _iLocal, CStream_SubFile* NewFile)
{
	MAUTOSTRIP( CWaveContainer_Plain_OpenWaveFile, MAUTOSTRIP_VOID);

	if (!m_lWaves.ValidPos(_iLocal))
		Error("GetWaveform", CStrF("Invalid local ID. (%d/%d)", _iLocal, m_lWaves.Len()));

	M_ASSERT(m_lWaves[_iLocal].m_SubLength, "Has to have a value");

	spCCFile spNewFileReal = MNew(CCFile);

/*	if (m_lWaves[_iLocal].m_spWave)
	{
		void *BitMap = m_lWaves[_iLocal].m_spWave->LockCompressed();
		
		spNewFileReal->Open(BitMap, m_lWaves[_iLocal].m_spWave->GetSize(), m_lWaves[_iLocal].m_spWave->GetSize(), CFILE_READ | CFILE_BINARY);
		NewFile->Open(spNewFileReal, 0, m_lWaves[_iLocal].m_spWave->GetSize());

		m_lWaves[_iLocal].m_spWave->Unlock();
	}
	else
	{*/
#if 	defined(PLATFORM_DOLPHIN)
		spNewFileReal->OpenExt(m_Path, CFILE_READ | CFILE_BINARY, NO_COMPRESSION, NORMAL_COMPRESSION, 1, 1, 32*1024);
#elif	defined(PLATFORM_PS2)
		spNewFileReal->OpenExt(m_Path, CFILE_READ | CFILE_BINARY, NO_COMPRESSION, NORMAL_COMPRESSION, 1, 1, 32*1024);
#elif	defined(PLATFORM_XBOX)
//		if (m_Path.Left(2).CompareNoCase("d:"))
//			spNewFileReal->OpenExt(m_Path, CFILE_READ | CFILE_BINARY, NO_COMPRESSION, NORMAL_COMPRESSION, 1, 1, 73728*2);
//		else
//			spNewFileReal->OpenExt(m_Path, CFILE_READ | CFILE_BINARY, NO_COMPRESSION, NORMAL_COMPRESSION, 1, 1, 73728*2);
			spNewFileReal->OpenExt(m_Path, CFILE_READ | CFILE_BINARY, NO_COMPRESSION, NORMAL_COMPRESSION, 2, 1, 2048);
#else
		spNewFileReal->OpenExt(m_Path, CFILE_READ | CFILE_BINARY, NO_COMPRESSION, NORMAL_COMPRESSION, 1, 1, 32*1024);
#endif
		NewFile->Open(spNewFileReal, m_lWaves[_iLocal].m_FilePos, m_lWaves[_iLocal].m_SubLength);
//	}

}

void CWaveContainer_Plain::GetWaveFileData( int _iLocal, fint& _Pos, fint& _Size )
{
	MAUTOSTRIP( CWaveContainer_Plain_GetWaveFileData, MAUTOSTRIP_VOID );
#ifdef	PLATFORM_PS2
	_Pos	= ( m_lWaves[_iLocal].m_FilePos >> 11 ) + m_ContainerDiscLocation;
#else
	_Pos	= m_lWaves[_iLocal].m_FilePos;
#endif
	_Size	= m_lWaves[_iLocal].m_SubLength;
}

void CWaveContainer_Plain::BuildInto(int _iLocal, CWaveform* _pWave)
{
	MAUTOSTRIP( CWaveContainer_Plain_BuildInto, MAUTOSTRIP_VOID);
#ifdef PLATFORM_CONSOLE
	Error_static("CWaveContainer_Plain::BuildInto", "Not supported!");
#else
	if (!m_lWaves.ValidPos(_iLocal))
		Error("BuildInto", CStrF("Invalid local ID. (%d/%d)", _iLocal, m_lWaves.Len()));

	m_lWaves[_iLocal].ReadDynamic(m_Path);
	if (!m_lWaves[_iLocal].m_spWave) Error("BuildInto", "Tried to access unloaded dynamic waveform.");

	_pWave->Create(*m_lWaves[_iLocal].m_spWave);
	_pWave->Blt(_pWave->GetClipRect(), * (CWaveform*) m_lWaves[_iLocal].m_spWave, 0, CPnt(0,0)); 
#endif
}

// -------------------------------------------------------------------

//
//
//
int CWaveContainer_Plain::AddRAW(CStr _Filename, CStr _Name, int _Fmt, int _nChn, bool _bUnsigned)
{
	MAUTOSTRIP( CWaveContainer_Plain_AddRaw, m_lWaves.Len() );

	spCWaveform spW = MNew(CWaveform);
	spW->WT_ReadRAW(_Filename, _Fmt, _nChn, _bUnsigned);
	return AddWaveTable(spW, _Name);
}

//
//
//
int CWaveContainer_Plain::AddWaveTable(spCWC_Waveform _spWave)
{
	MAUTOSTRIP( CWaveContainer_Plain_AddWaveTable__1, m_lWaves.Len() );

	/*
	if(m_pLipData)
	{
		M_TRACEALWAYS("CWaveContainer_Plain: Adding stuff to the wave table will ruin the lipsyncdata. Zonking it.\n");
		delete [] m_pLipData;
		m_pLipData = NULL;
	}
	*/

	_spWave->m_WaveID = m_pWaveContext->AllocID(m_iWaveClass, m_lWaves.Len());
	return m_lWaves.Add(*_spWave);
}

//
//
//
int CWaveContainer_Plain::AddWaveTable(spCWaveform _spWave, CStr _Name)
{
	MAUTOSTRIP( CWaveContainer_Plain_AddWaveTable__2, m_lWaves.Len() );
#ifdef PLATFORM_CONSOLE
	Error_static("CWaveContainer_Plain::AddWaveTable", "Not supported!");
	return 0;
#else
	spCWC_Waveform spW = MNew(CWC_Waveform);
	spW->m_spWave = _spWave;
#ifdef USE_HASHED_WAVENAME
	spW->m_NameID = StringToHash(_Name);
#else
	spW->m_Name = _Name;
#endif
	return AddWaveTable(spW);
#endif
}

//
//
//
int CWaveContainer_Plain::Compress(const char *_pCodecName, float _Quality)
{
	MAUTOSTRIP( CWaveContainer_Plain_Compress,  1);
#ifdef PLATFORM_CONSOLE
	Error_static("CWaveContainer_Plain::Compress", "Not supported!");
	return 0;
#else
	for(int iWave = 0; iWave < m_lWaves.Len(); iWave++)
	{
		//m_lWaves[iWave].WriteData(pFile);
		CWaveform Temp;
		CWaveform *Temp2 = MNew(CWaveform);
		m_lWaves[iWave].m_spWave->Decompress(&Temp);
		Temp.CompressSound(_pCodecName, Temp2, 1.0, _Quality, 0);
		m_lWaves[iWave].m_spWave = Temp2;
	}

	return 1;
#endif	
}



#pragma pack(push, 1)
struct broadcast_audio_extension 
{
    char description[256]; // ASCII : Description of the sound sequence
    char originator[32]; // ASCII : Name of the originator
    char originatorreference[32]; // ASCII : Reference of the originator
    char originationdate[10]; // ASCII : yyyy-mm-dd
    char originationtime[8]; ///ASCII : hh-mm-ss
    uint64 TimeReference; // First sample count since midnight
    uint32 Version; // Version of the BWF; unsigned binary number
    uint8 UMID[64]; // Binary byte 0 of SMPTE UMID
    uint8 reserved[188]; // 190 bytes, reserved for future use, set to "NULL" (188 because of Wavelab)
};
#pragma pack(pop)


//
//
//
bint FindChunk(CCFile *pFile, const ch8 *_pChunk)
{
	while (!pFile->EndOfFile())
	{
		fint LastPos = pFile->Pos();
   		char ChunkID[5];
		pFile->Read(ChunkID, 4);
		ChunkID[4] = 0;
		if (strcmp(ChunkID, _pChunk) == 0)
		{
			return true;
		}
		uint32 ChuckSize;
		pFile->ReadLE(ChuckSize);
		pFile->RelSeek(ChuckSize);
		if (pFile->Pos() == LastPos)
			break;
      }
	return false;
}

int CWaveContainer_Plain::AddWaveTable(CStr _Filename, CStr _Name, const char *_pCodecName, float _Priority, float _Quality, float _LipSyncSampleRate, int _Flags)
{
	MAUTOSTRIP( CWaveContainer_Plain_AddWaveTable__3, m_lWaves.Len() );
#ifdef PLATFORM_CONSOLE
	Error_static("CWaveContainer_Plain::AddWaveTable", "Not supported!");
	return 0;
#else
	fp32 TimeCode = 0.0f;
	spCWaveform spWave = MNew(CWaveform);
	if (!spWave) MemError("AddWaveTable");

	CStr DefaultFile = _Filename;

	CWaveform WaveTemp;

	if (_Filename.GetFilenameExtenstion().CompareNoCase("xrs") == 0)
	{
		CRegistry_Dynamic Reg;
		CStr Path = _Filename.GetPath();
		CStr FileNamePath = Path + "\\" + _Filename.GetFilenameNoExt();
		Reg.XRG_Read(_Filename);
		WaveTemp.m_LoopStart = spWave->m_LoopStart = Reg.GetValuei("loopstart", -1);
		WaveTemp.m_LoopEnd = spWave->m_LoopEnd = Reg.GetValuei("loopend", -1);
		CRegistry *pWaves = Reg.FindChild("waves");
        if (!pWaves)
			Error_static(M_FUNCTION, "No waves information found");

		if (pWaves->GetNumChildren() < 1)
			Error_static(M_FUNCTION, "No waves found");

		for (int i = 0; i < pWaves->GetNumChildren(); ++i)
		{
			CStr Temp = pWaves->GetValue(i);
			while (Temp != "")
			{
				CStr Name = Temp.GetStrSep(",");
				uint32 Speaker = NStr::StrToInt(Temp.GetStrSep(",").Str(), -1);
				int iChannelsBefore = WaveTemp.WT_GetChnCount();
				if (DefaultFile == "")
					DefaultFile = FileNamePath + Name;
				WaveTemp.WT_Read(FileNamePath + Name, 1);
				int iChannelsAfter = WaveTemp.WT_GetChnCount();
				for (int j = iChannelsBefore; j < iChannelsAfter; ++j)
				{
					WaveTemp.m_ChannelAssignments[j] = spWave->m_ChannelAssignments[j] = i;
					WaveTemp.m_SpeakerAssignment[j] = spWave->m_SpeakerAssignment[j] = Speaker;
				}
			}
		}
		if (WaveTemp.WT_GetChnCount() > 16)
			Error_static(M_FUNCTION, "Only 16 channels per waveform supported");
		spWave->m_Rate = WaveTemp.m_Rate;
	}
	else
	{
		WaveTemp.WT_Read(_Filename, 0);
		spWave->m_Rate = WaveTemp.m_Rate;
	}

	if (WaveTemp.WT_GetSampleCount() < 1)
		ThrowError("Sound samplecount must be 1 sample or greater");

	if (!_pCodecName || CStrBase::stricmp(_pCodecName, "RAW") != 0)
	{
		WaveTemp.CompressSound(_pCodecName, spWave, _Priority, _Quality, _Flags);

	}
	else
	{
		WaveTemp.Duplicate(spWave);
	}

	// Find Timecode data
	{
		CCFile File;
		File.Open(DefaultFile, CFILE_READ);
		uint8 Temp[4];
		File.Read(Temp, 4);
		File.RelSeek(8); // Seek to chunks

		if (memcmp(Temp, "RIFF", 4) == 0)
		{
			// We have a wave file
#if 0 // Disable bext
			fint EndPos = File.Length();
			for (fint i = 4; i < EndPos-4; ++i)
			{
				File.Seek(i);
				File.Read(Temp, 4);
				if (memcmp(Temp, "bext", 4) == 0)
				{
					uint32 Size;
					File.ReadLE(Size);
					if (File.Pos() + sizeof(broadcast_audio_extension ) <= EndPos)
					{
						broadcast_audio_extension Ext;

						File.Read(&Ext, sizeof(Ext));

						fp64 SampleRate = spWave->WT_GetSampleRate();
						fp64 Seconds = fp64(Ext.TimeReference) / SampleRate;
						fp64 Hour = 60.0*60.0;

						TimeCode = Seconds - Hour;
					}
				}
			}
#else // Use vegas 
			if (FindChunk(&File, "LIST"))
			{
				File.RelSeek(8);
				while (1)
				{
					if (FindChunk(&File, "labl"))
					{
						uint32 Size;
						File.ReadLE(Size);
						uint32 CueID;
						File.ReadLE(CueID);
						Size -= 4;
						CStr Temp;
						File.Read(Temp.GetBuffer(Size + 1), Size);
						Temp.GetStr()[Size] = 0;
						if (Temp.CompareSubStr("TC:") == 0)
						{
							CStr Number = Temp.CopyFrom(3);
							uint64 SamplesSinceMidnight = NStr::StrToInt(Number.Str(), uint64(0));
							
							fp64 SampleRate = 48000.0;
							fp64 Seconds = fp64(SamplesSinceMidnight) / SampleRate;
//							fp64 Hour = 60.0*60.0;

							TimeCode = Seconds;
							break;
						}
					}
					else
						break;
				}
			}
#endif
		}
	}

	

	//
	SLipDataSet DataSet;
	DataSet.m_DataSize = 0;
	DataSet.m_LocalWaveID = 0;
	DataSet.m_pData = NULL;

	DataSet.m_TimeCode = TimeCode;
	bool GotDataSet = false;

	if(_LipSyncSampleRate > 0.5f)
	{
		static NThread::CMutual Lock;
		DLock(Lock);
		NLipSync::CAnalyser *pAnalyser = NLipSync::GetAnalyser();
		if(pAnalyser)
		{
			NLipSync::CAnalysis *pAnalysis = pAnalyser->GetAnalysis(DefaultFile, _LipSyncSampleRate);
			
			if(pAnalysis)
			{
				NLipSync::CreateCompressedData(pAnalysis, DataSet.m_pData, DataSet.m_DataSize);
				GotDataSet = true;
				delete pAnalysis;
			}
		}
		
	}

	spCWC_Waveform spW = MNew(CWC_Waveform);
	spW->m_spWave = spWave;

#ifdef USE_HASHED_WAVENAME
	spW->m_NameID = StringToHash(_Name);
#else
	spW->m_Name = _Name;
#endif
	
	if(GotDataSet || !AlmostEqual(TimeCode, 0.0f, 0.001f))
	{
		DataSet.m_LocalWaveID = AddWaveTable(spW);
		m_lLipDataSets.Add(DataSet);
		return DataSet.m_LocalWaveID;
	}

	return AddWaveTable(spW);		
#endif
}

/*
int CWaveContainer_Plain::AddFromWaveList(CDataFile* _pDFile)
{
	CCFile* pFile = _pDFile->GetFile();

	int iFirstWave = m_lWaves.Len();
	CStr NodeName = _pDFile->GetNext();
	while(NodeName != "")
	{
		if (NodeName == "WAVEFORM")
		{
			_pDFile->PushPosition();
			if (_pDFile->GetSubDir())
			{
				spCWC_Waveform spW = DNew(CWC_Waveform) CWC_Waveform;
				if (spW == NULL) MemError("Read");
				spW->Read(_pDFile, m_bDynamic != 0);
				AddWaveTable(spW);
				_pDFile->GetParent();
			}
			_pDFile->PopPosition();
		}
		NodeName = _pDFile->GetNext();
	}
	return iFirstWave;
}
*/

//
//
//
int CWaveContainer_Plain::AddFromWaveInfo(CDataFile* _pDFile)
{
	MAUTOSTRIP( CWaveContainer_Plain_AddFromWaveInfo, m_lWaves.Len() );

	int nWaves = _pDFile->GetUserData();
	CCFile* pFile = _pDFile->GetFile();

	int iFirstWave = m_lWaves.Len();

	while(nWaves)
	{
		spCWC_Waveform spW = MNew(CWC_Waveform);

		if (spW == NULL) 
			MemError("Read");

		spW->ReadInfo(pFile, m_bDynamic != 0);
		
		AddWaveTable(spW);

		--nWaves;
 	}
	return iFirstWave;
}

#ifndef PLATFORM_CONSOLE
void CWaveContainer_Plain::AddFiltered(CWaveContainer_Plain *_pSrcXWC, TArray<CStr> *_pAllowed, int _Flags)
{
#ifdef M_RTM
	return;
#else

	if(_pSrcXWC->m_LipDataFileOffset && !_pSrcXWC->m_lLipDataSets.Len())
	{
		CCFile File;
		File.Open(_pSrcXWC->m_Path, CFILE_READ|CFILE_BINARY);
		File.Seek(_pSrcXWC->m_LipDataFileOffset);
		_pSrcXWC->LoadLSD(&File, _pSrcXWC->GetWaveCount(), 0);
		File.Close();
	}

	DestroyHash();
	
	CStringHashConst Hash;
	if (_pAllowed)
	{
		Hash.Create(_pAllowed->Len());

		for (int i = 0; i < _pAllowed->Len(); ++i)
		{
			Hash.Insert(i, (*_pAllowed)[i]);
		}
	}
	else
		Hash.Create(1);

	TArray<int> WaveTranslate;
	WaveTranslate.SetLen(_pSrcXWC->m_lWaves.Len());

	M_ASSERT(_pSrcXWC->m_lWaves.Len() == _pSrcXWC->m_lWaveData.Len(),"");

	int CurrentWave = m_lWaves.Len();
	m_lWaves.SetLen(CurrentWave + _pSrcXWC->m_lWaves.Len());
	m_lWaveData.SetLen(CurrentWave + _pSrcXWC->m_lWaves.Len());

	for (int i = 0; i < WaveTranslate.Len(); ++i)
	{
		
		int iHash = Hash.GetIndex(_pSrcXWC->m_lWaves[i].m_Name);

		if (!_pAllowed || iHash >= 0 )
		{
			bool bStreamed = (_pSrcXWC->m_lWaveData[i].Get_Flags() & ESC_CWaveDataFlags_Stream) != 0;
			bool bAdd = false;
			
			if ((_Flags & 1) && !bStreamed)
				bAdd = true;

			if ((_Flags & 2) && bStreamed)
				bAdd = true;

			if (bAdd)
			{
				WaveTranslate[i] = CurrentWave;
				m_lWaves[CurrentWave] = _pSrcXWC->m_lWaves[i];
				m_lWaveData[CurrentWave] = _pSrcXWC->m_lWaveData[i];
				m_lWaves[CurrentWave].m_WaveID = m_pWaveContext->AllocID(m_iWaveClass, CurrentWave);
				++CurrentWave;
			}
			else
				WaveTranslate[i] = -1;
		}
		else
		{
			WaveTranslate[i] = -1;
		}

	}

	m_lWaves.SetLen(CurrentWave);
	m_lWaveData.SetLen(CurrentWave);

//	m_lSFXDesc.SetLen(0);
	int UsedSFXDesc = 0;
	int CurrentSFXDesc = 0;
	
	for (int i = 0; i < _pSrcXWC->m_lSFXDesc.Len(); ++i)
	{
		CSC_SFXDesc Desc;
		Desc = _pSrcXWC->m_lSFXDesc[i];
		CSC_SFXDesc *pSrcDesc = &(_pSrcXWC->m_lSFXDesc[i]);

		int Mode = Desc.GetMode();
		bool bAdd = false;

		if (Mode == CSC_SFXDesc::ENORMAL)
		{
			CSC_SFXDesc::CNormalParams *pParams = Desc.GetNormalParams();
			TThinArray<int16> lNew;
			lNew.SetLen(pParams->m_lWaves.Len());
			int iCurrent = 0;
			for (int i = 0; i < pParams->m_lWaves.Len(); ++i)
			{
				if (WaveTranslate[pParams->m_lWaves[i]] >= 0)
				{
					lNew[iCurrent] = WaveTranslate[pParams->m_lWaves[i]];
					++iCurrent;
				}
			}
			lNew.SetLen(iCurrent);
			pParams->m_lWaves = lNew;
			bAdd = iCurrent != 0;
		}
		else if (Mode == CSC_SFXDesc::EMATERIAL)
		{
			Desc.SetMode(CSC_SFXDesc::ENONE);
			Desc.SetMode(CSC_SFXDesc::EMATERIAL);
			CSC_SFXDesc::CMaterialParams *pParams = Desc.GetMaterialParams();
			CSC_SFXDesc::CMaterialParams *pParamsSrc = pSrcDesc->GetMaterialParams();
			for (int i = 0; i < pParamsSrc->m_HolderHashSize; ++i)
			{
				if (pParamsSrc->m_alMaterialWaveHolders[i].Len())
				{
					for (int h = 0; h < pParamsSrc->m_alMaterialWaveHolders[i].Len(); ++h)
					{
						for (int j = 0; j < pParamsSrc->m_alMaterialWaveHolders[i][h]->m_lWaves.Len(); ++j)
						{
							if (WaveTranslate[pParamsSrc->m_alMaterialWaveHolders[i][h]->m_lWaves[j]] >= 0)
							{
								pParams->AddWave(pParamsSrc->m_alMaterialWaveHolders[i][h]->m_iMaterial, WaveTranslate[pParamsSrc->m_alMaterialWaveHolders[i][h]->m_lWaves[j]]);
								bAdd = true;
							}							
						}
					}
				}
			}
		}
		else
		{
			Error("AddFiltered", "Material mode error");
		}

		if (bAdd)
		{
			AddSFXDesc(Desc);
		}

	}

	for (int i = 0; i < _pSrcXWC->m_lLipDataSets.Len(); ++i)
	{
		int32 LocalWaveID = _pSrcXWC->m_lLipDataSets[i].m_LocalWaveID;
		if (WaveTranslate[LocalWaveID] >= 0)
		{
			SLipDataSet DataSet;
			DataSet.m_DataSize = _pSrcXWC->m_lLipDataSets[i].m_DataSize;
			if (DataSet.m_DataSize)
				DataSet.m_pData = DNew(uint8) uint8[DataSet.m_DataSize];
			else
				DataSet.m_pData = NULL;
			DataSet.m_TimeCode = _pSrcXWC->m_lLipDataSets[i].m_TimeCode;
			if (DataSet.m_pData)
				memcpy(DataSet.m_pData, _pSrcXWC->m_lLipDataSets[i].m_pData, DataSet.m_DataSize);
			DataSet.m_LocalWaveID = WaveTranslate[LocalWaveID];
			m_lLipDataSets.Add(DataSet);
		}
	}
#endif
}

//
//
//
void CWaveContainer_Plain::FilterWaves(TArray<CStr> *_pValidWaves)
{
	Error_static("CWaveContainer_Plain::FilterWaves", "Not supported");
#if 0
	if (!_pValidWaves)
		return;

	DestroyHash();
	
	CStringHashConst Hash;
	Hash.Create(_pValidWaves->Len());

	for (int i = 0; i < _pValidWaves->Len(); ++i)
	{
		Hash.Insert(i, (*_pValidWaves)[i]);
	}


	TArray<int> WaveTranslate;
	WaveTranslate.SetLen(m_lWaves.Len());

	M_ASSERT(m_lWaves.Len() == m_lWaveData.Len(),"");

	int CurrentWave = 0;
	TArray<CWC_Waveform> Waves;
	TThinArray<CWaveData> WaveData;
	Waves.SetLen(m_lWaves.Len());
	WaveData.SetLen(m_lWaves.Len());

	for (int i = 0; i < WaveTranslate.Len(); ++i)
	{
		int iHash = Hash.GetIndex(m_lWaves[i].m_Name);

		if (m_lWaves[i].m_WaveID > 0)
			m_pWaveContext->FreeID(m_lWaves[i].m_WaveID);

		if (iHash >= 0)
		{
			WaveTranslate[i] = CurrentWave;
			Waves[CurrentWave] = m_lWaves[i];
			WaveData[CurrentWave] = m_lWaveData[i];
			Waves[CurrentWave].m_WaveID = m_pWaveContext->AllocID(m_iWaveClass, CurrentWave);
			++CurrentWave;
		}
		else
		{
			WaveTranslate[i] = -1;
		}

	}
	Waves.SetLen(CurrentWave);
	WaveData.SetLen(CurrentWave);
	TArray<CSC_SFXDesc> SFXDesc;
	TArray<int16> SFXDescWaves;
	int UsedSFXDesc = 0;
	int CurrentSFXDesc = 0;
	SFXDescWaves.SetLen(m_lSFXDescWaves.Len());
	SFXDesc.SetLen(m_lSFXDesc.Len());
	
	for (int i = 0; i < m_lSFXDesc.Len(); ++i)
	{
		CSC_SFXDesc Desc;
		Desc = m_lSFXDesc[i];
		Desc.m_iWavesStart = UsedSFXDesc;
		Desc.m_iNumWaves = 0;

		for (int j = 0; j < m_lSFXDesc[i].m_iNumWaves; ++j)
		{
			int LastLocal = m_lSFXDescWaves[m_lSFXDesc[i].m_iWavesStart + j].m_iLocalWave;
			if (WaveTranslate[LastLocal] >= 0)
			{
				SFXDescWaves[UsedSFXDesc++].m_iLocalWave = WaveTranslate[LastLocal];
				++Desc.m_iNumWaves;
			}
		}

		if (Desc.m_iNumWaves)
		{
			SFXDesc[CurrentSFXDesc++] = Desc;
		}
	}
	SFXDescWaves.SetLen(UsedSFXDesc);
	SFXDesc.SetLen(CurrentSFXDesc);

	m_lWaves = Waves;
	m_lWaveData = WaveData;
	m_lSFXDesc = SFXDesc;
	m_lSFXDescWaves = SFXDescWaves;
	m_UsedSFXDesc = UsedSFXDesc;
#endif
}
#endif //PLATFORM_CONSOLE

//
//
//
void CWaveContainer_Plain::AddXWC(CStr _Filename, bool _bDynamic)
{
	MAUTOSTRIP( CWaveContainer_Plain_AddXWC, MAUTOSTRIP_VOID);

	int32 CurrentWaves = GetWaveCount();

//	M_CALLGRAPH;
	CStr PreviousPath = m_Path;
	m_Path.Capture(_Filename.Str());
	m_bDynamic = _bDynamic;

#ifdef	PLATFORM_PS2
	extern class MRTC_PS2Metafile *PS2_Metafile;
	m_ContainerDiscLocation	= MRTC_SystemInfo::OS_FilePosition( _Filename.GetStr() );
#endif	// PLATFORM_PS2

	CDataFile DFile;
	DFile.Open(m_Path);

	CCFile* pF = DFile.GetFile();

	const char * pFirstEntry;
	int Version = 1;
	pFirstEntry = DFile.GetNext();
	pFirstEntry = DFile.GetNext();
	if (strcmp(pFirstEntry, "VERSION") == 0)
	{
		Version = DFile.GetUserData();
	}
	
	switch(Version)
	{
	case 1:
	case 2:
		Error("AddXWC", CStrF("File '%s' has an old wavecontainerformat which is not supported.", m_Path.GetStr()));
		/*
		
		DFile.PushPosition();

		if (strcmp(pFirstEntry, "WAVELIST")) Error("AddXWC", "Not a valid XWC file.");
		if (!DFile.GetSubDir()) Error("AddXWC", "Not a valid XWC file.");
		AddFromWaveList(&DFile);
		DFile.GetParent();
		DFile.PopPosition();
		
		if (DFile.GetNext("SFXDESC"))
		{
			int Start = m_lSFXDesc.Len();
			m_lSFXDesc.SetLen(Start + DFile.GetUserData());
			for(int i = Start; i < m_lSFXDesc.Len(); i++)
			{
				m_lSFXDesc[i].Read(pF, this);
				
			}
		}
		else
			LogFile("No sound-fx descriptions in file.");
			*/
		break;
	case 3:
		if (!DFile.GetNext("WAVEDATA")) 
			Error("AddXWC", CStrF("File '%s' is not a valid XWC file (missing WAVEDATA).", m_Path.GetStr()));
	default:

		if (!DFile.GetNext("WAVEINFO")) 
			Error("AddXWC", CStrF("File '%s' is not a valid XWC file (missing WAVEINFO).", m_Path.GetStr()));

		{
			MSCOPESHORT(AddFromWaveInfo);
			AddFromWaveInfo(&DFile);
		}

		if (!DFile.GetNext("WAVEDESC")) 
			Error("AddXWC", CStrF("File '%s' is not a valid XWC file (missing WAVEDESC).", m_Path.GetStr()));

		int32 WavesInThis = DFile.GetUserData();
		m_lWaveData.SetLen(DFile.GetUserData());
		DFile.GetFile()->Read(m_lWaveData.GetBasePtr(), sizeof(CWaveData) * m_lWaveData.Len());

#ifdef CPU_BIGENDIAN
		for (int i = 0; i < m_lWaveData.Len(); ++i)
		{
			m_lWaveData[i].ByteSwap();
		}
#endif
		
		CreateHash();

		if (DFile.GetNext("SFXDESC"))
		{

			MSCOPESHORT(SFXDESC);
			int Start = m_lSFXDesc.Len();
			m_lSFXDesc.SetLen(Start + DFile.GetUserData());
			for(int i = Start; i < m_lSFXDesc.Len(); i++)
			{
				m_lSFXDesc[i].Read(pF, this);
				
				/*			CSC_SFXDesc *pDesc = &m_lSFXDesc[i];
				//	LogFile(CStrF("SFX %d, %s", i, m_lSFXDesc[i].m_SoundName));
				for(int iWave = 0; iWave < m_lSFXDesc[i].m_lWaves.Len(); iWave++)
				{
				CSC_SFXDescWave *pWaveDesc = &m_lSFXDesc[i].m_lWaves[iWave];
				//				m_lSFXDesc[i].m_lWaves[iWave].m_iLocalWave = GetIndex(&m_lSFXDesc[i].m_lWaves[iWave].m_WaveName[0]);
				
				  int iTemp = GetIndex(&m_lSFXDesc[i].m_lWaves[iWave].m_WaveName[0]);
				  m_lSFXDesc[i].m_lWaves[iWave].m_WaveID = (iTemp >= 0) ? GetWaveID(iTemp) : 0;
				  
					if (m_lSFXDesc[i].m_lWaves[iWave].m_WaveID < 0)
					ConOutL(CStrF("§cf80WARNING: Sound '%s' references undefined waveform '%s'.", &m_lSFXDesc[i].m_SoundName[0], &m_lSFXDesc[i].m_lWaves[iWave].m_WaveName[0]));
			}*/
			}

			{ MSCOPESHORT(AddFromWaveInfo);
				m_lWaves.OptimizeMemory();
			}
		}
		else
			LogFile("No sound-fx descriptions in file.");

#ifdef USE_HASHED_SFXDESC

		{
			int Len = m_lSFXDesc.Len();
			m_lSFXDescOrder.SetLen(Len);
			for (int i = 0; i < Len; ++i)
			{
				m_lSFXDescOrder[i] = i;
			}
			TAP_RCNRTM<CSC_SFXDesc> Tap(m_lSFXDesc);
			m_lSFXDescOrder.QSort<CSortWCHashed>(&Tap);
		}
#else
		{
			int Len = m_lSFXDesc.Len();
			m_lSFXDescOrder.SetLen(Len);
			for (int i = 0; i < Len; ++i)
			{
				m_lSFXDescOrder[i] = i;
			}
			TAP_RCNRTM<CSC_SFXDesc> Tap(m_lSFXDesc);
			m_lSFXDescOrder.QSort<CSortWCNonHashed>(&Tap);
//			for (int i = 0; i < Len; ++i)
//			{
//				M_TRACEALWAYS("%s\n", m_lSFXDesc[m_lSFXDescOrder[i]].m_SoundName.Str());
//			}
		}
#endif

		

		// optional lipsync section
		if(DFile.GetNext("LIPSYNC")) 
		{
			if(CurrentWaves != 0)
			{
				// Not the first

				// load first if needed
				if(m_LipDataFileOffset)
				{
					CCFile File;
					File.Open(PreviousPath, CFILE_READ|CFILE_BINARY);
					File.Seek(m_LipDataFileOffset);
					LoadLSD(&File, CurrentWaves, 0);
					File.Close();
					m_LipDataFileOffset = 0;
				}

				// load this one
				CCFile *pFile = DFile.GetFile();
				m_LipDataFileOffset = pFile->Pos();
				LoadLSD(pFile, WavesInThis, CurrentWaves);
				m_LipDataFileOffset = 0;
			}
			else
			{
				CCFile *pFile = DFile.GetFile();
				m_LipDataFileOffset = pFile->Pos();
			}

			/*
			for(int32 i = 0; i < GetWaveCount(); i++)
			{
				void *pData;
				int32 Size;
				LoadLipSyncData(i, Size, pData);

				if(pData)
				{
					this->
				}
			}

			pFile->Read(m_pLipData, Size);

			uint32 Add = (uint32)m_pLipData+GetWaveCount()*sizeof(void*);
			uint32 *pLipPtrs = (uint32 *)m_pLipData;
			for(int32 i = 0; i < GetWaveCount(); i++)
				if(pLipPtrs[i])
					pLipPtrs[i] += Add;
					*/
		}

		DestroyHash();
	}
	//LogFile("(CWaveContainer_Plain::AddXWC) " + _Filename + CStrF(", nWaves %d, nSFXDesc %d", m_lWaves.Len(), m_lSFXDesc.Len()));

	DFile.Close();
}

void CWaveContainer_Plain::LoadLSD(CCFile *pFile, int32 _WaveCount, int32 _Start)
{
	uint32 *pIndex = DNew(uint32) uint32[_WaveCount];
	pFile->Read(pIndex, _WaveCount*sizeof(int32));

	if(_Start < 0)
		_Start = GetWaveCount();

	for(int32 i = 0; i < _WaveCount; i++)
	{
		// skip
		if(pIndex[i] == 0xFFFFFFFF)
			continue;

		SLipDataSet DataSet;
		pFile->ReadLE(DataSet.m_DataSize);
		pFile->ReadLE(DataSet.m_TimeCode);
		if (DataSet.m_DataSize)
		{
			DataSet.m_pData = DNew(uint8) uint8[DataSet.m_DataSize];
			pFile->Read(DataSet.m_pData, DataSet.m_DataSize);
		}
		else
			DataSet.m_pData = NULL;
		DataSet.m_LocalWaveID = _Start+i;
		m_lLipDataSets.Add(DataSet);
	}

	delete [] pIndex;
}

void CWaveContainer_Plain::LoadIndexData()
{
	if(!m_LipDataFileOffset)
		return;

	int32 Size = GetWaveCount();
	m_lLipIndexData.SetLen(Size);
	CCFile File;
	File.Open(m_Path, CFILE_READ|CFILE_BINARY);
	File.Seek(m_LipDataFileOffset);
	File.ReadLE(m_lLipIndexData.GetBasePtr(), Size);
	File.Close();
}

void CWaveContainer_Plain::UnloadIndexData()
{
	m_lLipIndexData.SetLen(0);
}

// _pData has to point to a valid chunk of memory that's large enough for LSD data
bool CWaveContainer_Plain::GetLSD(int _LocalWaveID, void* _pData, int &_rSize)
{
	// reset
	_rSize = 0;

	if(!m_LipDataFileOffset)
		return false;

	// load index data if needed
	if(m_lLipIndexData.Len() <= 0)
		LoadIndexData();

	//
	uint32 iLipData = m_lLipIndexData[_LocalWaveID];
	if(iLipData == 0xFFFFFFFF)
		return false;

	// open
	CCFile File;
	File.Open(m_Path, CFILE_READ|CFILE_BINARY);

	// seek to data
	int32 Offset = m_LipDataFileOffset + GetWaveCount() * sizeof(int32) + iLipData;
	File.Seek(Offset);

	// read data
	int32 Size;
	File.ReadLE(Size);
	File.Read(_pData, Size + sizeof(fp32));

	// clean up
	File.Close();

	_rSize	= Size + sizeof(fp32);

	// UnloadIndexData() // ??
	return true;
}

bool CWaveContainer_Plain::GetLSDSize(int _LocalWaveID, int &_rSize)
{
	// reset
	_rSize = 0;

	if(!m_LipDataFileOffset)
		return false;

	// load index data if needed
	if(m_lLipIndexData.Len() <= 0)
		LoadIndexData();

	//
	uint32 iLipData = m_lLipIndexData[_LocalWaveID];
	if(iLipData == 0xFFFFFFFF)
		return false;

	// open
	CCFile File;
	File.Open(m_Path, CFILE_READ|CFILE_BINARY);

	// seek to data
	int32 Offset = m_LipDataFileOffset + GetWaveCount() * sizeof(int32) + iLipData;
	File.Seek(Offset);

	// read data
	int32 Size;
	File.ReadLE(Size);

	// clean up
	File.Close();

	_rSize	= Size + sizeof(fp32);

	// UnloadIndexData() // ??
	return true;
}



//
//
//
void CWaveContainer_Plain::GetWaveData(int _iLocal, CWaveData &_WaveData)
{
	MAUTOSTRIP( CWaveContainer_Plain_GetWaveData, MAUTOSTRIP_VOID);
	_WaveData = m_lWaveData[_iLocal];
}

/*
void CWaveContainer_Plain::WriteWaveList(CDataFile* _pDFile)
{
	CCFile* pFile = _pDFile->GetFile();

	for(int iWave = 0; iWave < m_lWaves.Len(); iWave++)
	{
		_pDFile->BeginEntry("WAVEFORM");
		_pDFile->EndEntry(0);

		_pDFile->BeginSubDir();
		m_lWaves[iWave].Write(_pDFile);
		_pDFile->EndSubDir();
	}
}
*/

//
//
//
void CWaveContainer_Plain::WriteLSD(CDataFile* _pDFile)
{
	CCFile *pFile = _pDFile->GetFile();
	for(int32 i = 0; i < GetWaveCount(); i++)
	{
		uint32 Offset = 0xFFFFFFFF;
		uint32 Current = 0;
		//uint8 *pCurrent = NULL;
		for(int32 l = 0; l < m_lLipDataSets.Len(); l++)
		{
			if(m_lLipDataSets[l].m_LocalWaveID == i)
			{
				Offset = Current;
				break;
			}

			Current += sizeof(m_lLipDataSets[l].m_DataSize);
			Current += sizeof(m_lLipDataSets[l].m_TimeCode);
			Current += m_lLipDataSets[l].m_DataSize;
		}

		pFile->WriteLE(Offset);
	}

	for(int32 l = 0; l < m_lLipDataSets.Len(); l++)
	{
		pFile->WriteLE(m_lLipDataSets[l].m_DataSize);
		pFile->WriteLE(m_lLipDataSets[l].m_TimeCode);
		if (m_lLipDataSets[l].m_pData)
			pFile->Write(m_lLipDataSets[l].m_pData, m_lLipDataSets[l].m_DataSize);
	}
}

//
//
//
void CWaveContainer_Plain::WriteWaveData(CDataFile* _pDFile)
{
	MAUTOSTRIP( CWaveContainer_Plain_WriteWaveData, MAUTOSTRIP_VOID);
	CCFile* pFile = _pDFile->GetFile();
	for(int iWave = 0; iWave < m_lWaves.Len(); iWave++)
	{
		m_lWaves[iWave].WriteData(pFile);
	}
}

//
//
//
void CWaveContainer_Plain::WriteWaveDesc(CDataFile* _pDFile)
{
	MAUTOSTRIP( CWaveContainer_Plain_WriteWaveDesc, MAUTOSTRIP_VOID);

	CCFile* pFile = _pDFile->GetFile();

	for(int iWave = 0; iWave < m_lWaves.Len(); iWave++)
	{
		CWaveData WaveData;
		m_pWaveContext->GetWaveData(m_lWaves[iWave].m_WaveID, WaveData);
		WaveData.Write(pFile);
	}
}


//
//
//
void CWaveContainer_Plain::WriteWaveInfo(CDataFile* _pDFile)
{
	MAUTOSTRIP( CWaveContainer_Plain_WriteWaveInfo, MAUTOSTRIP_VOID);

	CCFile* pFile = _pDFile->GetFile();

	for(int iWave = 0; iWave < m_lWaves.Len(); iWave++)
		m_lWaves[iWave].WriteInfo(pFile);
}

//
//
//
void CWaveContainer_Plain::WriteSFXDescs(CDataFile* _pDFile)
{
	MAUTOSTRIP( CWaveContainer_Plain_WriteSFXDescs, MAUTOSTRIP_VOID);

	CCFile* pFile = _pDFile->GetFile();

	for(int i = 0; i < m_lSFXDesc.Len(); i++)
		m_lSFXDesc[i].Write(pFile, this);

	
}


//
//
//

void CWaveContainer_Plain::WriteXWCStrippedData(CStr _Filename)
{
	MAUTOSTRIP( CWaveContainer_Plain_WriteXWC, MAUTOSTRIP_VOID);

	if(m_LipDataFileOffset && !m_lLipDataSets.Len())
	{
		CCFile File;
		File.Open(m_Path, CFILE_READ|CFILE_BINARY);
		File.Seek(m_LipDataFileOffset);
		LoadLSD(&File, GetWaveCount(), 0);
		File.Close();
	}

	m_Path = _Filename;

	CDataFile DataFile;
	DataFile.Create(_Filename, 2);
	DataFile.BeginEntry("VERSION");
	int Version = 4;
	DataFile.EndEntry(Version);

	DataFile.BeginEntry("WAVEINFO");
	WriteWaveInfo(&DataFile);
	DataFile.EndEntry(GetWaveCount());

	DataFile.BeginEntry("WAVEDESC");
	DataFile.GetFile()->Write(m_lWaveData.GetBasePtr(), sizeof(CWaveData) * m_lWaveData.Len());
	DataFile.EndEntry(GetWaveCount());

	DataFile.BeginEntry("SFXDESC");
	WriteSFXDescs(&DataFile);
	DataFile.EndEntry(m_lSFXDesc.Len());

	// save lipsync data if we have compiled it (should be moved to it's own function)
	#ifndef PLATFORM_CONSOLE
	if(m_lLipDataSets.Len())
	{
		DataFile.BeginEntry("LIPSYNC");
		WriteLSD(&DataFile);
		DataFile.EndEntry(0);
	}
	#endif
	DataFile.Close();
}

void CWaveContainer_Plain::WriteXWC(CStr _Filename)
{
	MAUTOSTRIP( CWaveContainer_Plain_WriteXWC, MAUTOSTRIP_VOID);

	if(m_LipDataFileOffset && !m_lLipDataSets.Len())
	{
		CCFile File;
		File.Open(m_Path, CFILE_READ|CFILE_BINARY);
		File.Seek(m_LipDataFileOffset);
		LoadLSD(&File, GetWaveCount(), 0);
		File.Close();
	}


	m_Path = _Filename + ".Tmp";

	// Hack hach
	// Write everything to a temp file so we can read it laaaaater
	{
		CDataFile DataFile;
		DataFile.Create(m_Path, 2);
		DataFile.BeginEntry("VERSION");
		int Version = 3;
		DataFile.EndEntry(Version);

		DataFile.BeginEntry("WAVEDATA");
//JK-TODO: The alignment must be 2k re-enable this
//		if( Platform == 2 )
		DataFile.GetFile()->Align( 2048 );
		WriteWaveData(&DataFile);
		DataFile.EndEntry(GetWaveCount());

		DataFile.Close();
	}

	CDataFile DataFile;
	DataFile.Create(_Filename, 2);
	DataFile.BeginEntry("VERSION");
	int Version = 3;
	DataFile.EndEntry(Version);

	DataFile.BeginEntry("WAVEDATA");
//JK-TODO: The alignment must be 2k re-enable this
//	if( Platform == 2 )
	DataFile.GetFile()->Align( 2048 );
	WriteWaveData(&DataFile);
	DataFile.EndEntry(GetWaveCount());

	DataFile.BeginEntry("WAVEINFO");
	WriteWaveInfo(&DataFile);
	DataFile.EndEntry(GetWaveCount());

	DataFile.BeginEntry("WAVEDESC");
	WriteWaveDesc(&DataFile);
	DataFile.EndEntry(GetWaveCount());

	DataFile.BeginEntry("SFXDESC");
	WriteSFXDescs(&DataFile);
	DataFile.EndEntry(m_lSFXDesc.Len());

	// save lipsync data if we have compiled it (should be moved to it's own function)
	#ifndef PLATFORM_CONSOLE
	if(m_lLipDataSets.Len())
	{
		DataFile.BeginEntry("LIPSYNC");
		WriteLSD(&DataFile);
		DataFile.EndEntry(0);
	}
	#endif
	DataFile.Close();

	CDiskUtil::DelFile(m_Path);
}

// -------------------------------------------------------------------

//
//
//
CWC_Waveform *CWaveContainer_Plain::GetWaveform(const char* _pWaveName)
{
	MAUTOSTRIP(CWaveContainer_Plain_GetWaveform, NULL );
	
	int16 LocalID = GetLocalWaveID(_pWaveName);

	if(LocalID >= 0)
		return m_lWaves.GetBasePtr() + LocalID;

	return NULL;
}

//
//
//
int16 CWaveContainer_Plain::GetLocalWaveID(const char *_pWaveName)
{
	MAUTOSTRIP(CWaveContainer_Plain_GetWaveform, NULL );

	#ifdef USE_HASHED_WAVENAME
		uint32 Index = StringToHash(_pWaveName);
	#endif

	CWC_Waveform *pWaveform = m_lWaves.GetBasePtr();
	int32 NumWaves = m_lWaves.Len();
	for(int32 i = 0; i < NumWaves; i++)
	{
		#ifdef USE_HASHED_WAVENAME
			if (pWaveform[i].m_NameID == Index)
				return (int16)i;
		#else
			if (CStrBase::CompareNoCase(pWaveform[i].m_Name, _pWaveName) == 0)
				return (int16)i;
		#endif
	}

	return -1;
}

//
//
//
class CSortWCNonHashedBinarySearch
{
public:
	static int Compare(TAP_RCNRTM<CSC_SFXDesc> *_pContext, uint16 _i0, const char *_pName)
	{
#ifdef USE_HASHED_SFXDESC
		return 0;
#else
		return (*_pContext)[_i0].m_SoundName.CompareNoCase(_pName);
#endif
	}
};

class CSortWCHashedBinarySearch
{
public:
	static int Compare(TAP_RCNRTM<CSC_SFXDesc> *_pContext, uint16 _i0, uint32 _SoundIndex)
	{
#ifdef USE_HASHED_SFXDESC
		uint32 SoundID0 = (*_pContext)[_i0].m_SoundIndex;
		uint32 SoundID1 = _SoundIndex;
#else
		uint32 SoundID0 = StringToHash((*_pContext)[_i0].m_SoundName.Str());
		uint32 SoundID1 = _SoundIndex;
#endif
		if (SoundID0 > SoundID1)
			return 1;
		else if (SoundID0 < SoundID1)
			return -1;
		return 0;
	}
};

int CWaveContainer_Plain::GetSFXDescIndex(const char* _SoundName)
{
	MAUTOSTRIP( CWaveContainer_Plain_GetSFXDescIndex, -1 );

	if (m_lSFXDescOrder.Len())
	{
		TAP_RCNRTM<CSC_SFXDesc> Tap(m_lSFXDesc);
#ifdef USE_HASHED_SFXDESC
		uint32 SoundID = StringToHash(_SoundName);
		int iFind = m_lSFXDescOrder.BinarySearch<CSortWCHashedBinarySearch>(SoundID, &Tap);
		if (iFind >= 0)
			return m_lSFXDescOrder[iFind];
		else
			return -1;
#else
		int iFind = m_lSFXDescOrder.BinarySearch<CSortWCNonHashedBinarySearch>(_SoundName, &Tap);
		if (iFind >= 0)
			return m_lSFXDescOrder[iFind];
		else
			return -1;
#endif
	}
	else
	{
		CSC_SFXDesc* pDesc = m_lSFXDesc.GetBasePtr();
		int nDesc = m_lSFXDesc.Len();
		#ifdef USE_HASHED_SFXDESC
			uint32 SoundID = StringToHash(_SoundName);
			for(int i = 0; i < nDesc; i++)
				if (pDesc[i].m_SoundIndex == SoundID)
					return i;
		#else
			for(int i = 0; i < nDesc; i++)
				if (CStrBase::CompareNoCase(pDesc[i].m_SoundName, _SoundName) == 0)
					return i;
		#endif
		return -1;
	}
}

//
//
//
CSC_SFXDesc *CWaveContainer_Plain::GetSFXDesc(int _iSound)
{
	MAUTOSTRIP( CWaveContainer_Plain_GetSFXDesc, NULL);
	if ((_iSound < 0) || (_iSound >= m_lSFXDesc.Len()))
		return NULL;
	return &m_lSFXDesc[_iSound];
}

void CWaveContainer_Plain::AddSFXDesc(CSC_SFXDesc &_SFXDesc)
{
	m_lSFXDesc.Add(_SFXDesc);
}


//
//
//
fp32 CWaveContainer_Plain::GetWaveSampleRate(int _iLocal)
{
	MAUTOSTRIP( CWaveContainer_Plain_GetWaveSampleRate, 22050.0f );
#ifdef PLATFORM_CONSOLE
	Error_static("CWaveContainer_Plain::GetWaveSampleRate", "Not supported!");
	return 0;
#else
	if (_iLocal < 0)
		return 22050.0f;

	if (m_lWaves[_iLocal].m_WaveID >= 0)
		return m_lWaves[_iLocal].m_spWave->WT_GetSampleRate();
	else
		return 22050.0f;
#endif
}

void CWaveContainer_Plain::LSD_Flush()
{
	UnloadIndexData();
}

#ifndef PLATFORM_CONSOLE
// MikeW: Only used by XWC_Static. Required for partial sound XWC updates.
void CWaveContainer_Plain::AddLipDataSet(int _iLocalWaveID, fp32 _TimeCode, int32 _DataSize, void * _pData)
{
	SLipDataSet DataSet;
	DataSet.m_LocalWaveID = _iLocalWaveID;
	DataSet.m_TimeCode = _TimeCode;
	DataSet.m_DataSize = _DataSize;
	DataSet.m_pData = _pData;
	m_lLipDataSets.Add(DataSet);
}

bool CWaveContainer_Plain::GetLipDataSet(int _iLocalWaveID, fp32 &_oTimeCode, int32 &_oDataSize, void **_oppData)
{
	int nDataSets = m_lLipDataSets.Len();
	for (int i = 0; i < nDataSets; i++)
	{
		if (m_lLipDataSets[i].m_LocalWaveID == _iLocalWaveID) {
			_oTimeCode = m_lLipDataSets[i].m_TimeCode;
			_oDataSize = m_lLipDataSets[i].m_DataSize;
			if (_oppData)
				*_oppData = m_lLipDataSets[i].m_pData;
			return true;
		}
	}
	return false;
}

#endif
