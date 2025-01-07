#include "PCH.h"

#include "../MSystem.h"
#include "MSound.h"

// -------------------------------------------------------------------
SYSTEMDLLEXPORT spCSoundContext MCreateSoundContext(CStr _ClassName, int _MaxVoices)
{
	MAUTOSTRIP(MCreateSoundContext, NULL);

#ifdef PLATFORM_XBOX1
	spCSoundContext spSC = (CSoundContext*) MRTC_GetObjectManager()->CreateObject("CSoundContext_Xbox");
#elif defined(PLATFORM_XENON)
//	spCSoundContext spSC = (CSoundContext*) MRTC_GetObjectManager()->CreateObject("CSoundContext_Xenon");
//	spCSoundContext spSC = (CSoundContext*) MRTC_GetObjectManager()->CreateObject("CSoundContext_Xenon2");
	spCSoundContext spSC = (CSoundContext*) MRTC_GetObjectManager()->CreateObject("CSoundContext_Xenon3");
#elif defined(PLATFORM_PS3)
	spCSoundContext spSC = (CSoundContext*) MRTC_GetObjectManager()->CreateObject("CSoundContext_PS3");
#else
	spCSoundContext spSC = (CSoundContext*) MRTC_GetObjectManager()->CreateObject(_ClassName);
#endif

	if (spSC != NULL) spSC->Create("");
	if (spSC != NULL) spSC->Init(_MaxVoices);

	return spSC;
}

CSoundContext::CFilter::CMinMax CSoundContext::CFilter::ms_MinMax;

CSoundContext::CFilter::CMinMax::CMinMax()
{

	m_EnvironmentSize.Set(1.0f, 100.0f);
	m_Diffusion.Set(0.0f, 100.0f);
	m_Density.Set(0.0f, 100.0f);
	m_Room.Set(-10000, 0);
	m_RoomLF.Set(-10000, 0);
	m_RoomHF.Set(-10000, 0);

	m_DecayTime.Set(0.1f, 20.0f);
	m_DecayHFRatio.Set(0.1f, 2.0f);
	m_DecayLFRatio.Set(0.1f, 2.0f);
	m_Reflections.Set(-10000, 1000);
	m_ReflectionsDelay.Set(0.0f, 0.3f);
	m_Reverb.Set(-10000, 2000);
	m_ReverbDelay.Set(0.0f, 0.1f);

	m_EchoTime.Set(0.075f, 0.25f);
	m_EchoDepth.Set(0.0f, 1.0f);

	m_ModulationTime.Set(0.04f, 4.0f);
	m_ModulationDepth.Set(0.0f, 1.0f);

	m_AirAbsorptionHF.Set(-100.0f, 0.0f);

	m_HFReference.Set(1000.0f, 20000.0f);
	m_LFReference.Set(20.0f, 1000.0f);

	m_RoomRolloffFactor.Set(0.0f, 10.0f);
}

//
//
//
void CSoundContext::CFilter::SetDefault()
{
	m_Room = -10000;
	m_RoomHF = -10000;
	m_RoomRolloffFactor = 0;
	m_DecayTime = 1.0f;
	m_DecayHFRatio = 0.5f;
	m_Reflections = -10000;
	m_ReflectionsDelay = 0.02f;
	m_Reverb = -10000;
	m_ReverbDelay = 0.04f;
	m_Diffusion = 100;
	m_Density = 100;
	m_HFReference = 5000;

	m_EnvironmentSize = 7.5f;
	m_DecayLFRatio = 1.00f;

	m_EchoTime = 0.25f;
	m_EchoDepth = 0.0f;
	m_ModulationTime = 0.25f;
	m_ModulationDepth = 0.0f;
	m_AirAbsorptionHF = -5.0f;
	m_LFReference = 250.0f;

	m_RoomLF = -10000;
};

CSC_3DOcclusion::CSC_3DOcclusion()
{
	SetDefault();
}


void CSC_3DOcclusion::SetDefault()
{
	m_Occlusion = 0.0f;
	m_Obstruction = 0.0f;
}


//----------------------------------------------------------------
// CWaveContext
//----------------------------------------------------------------

IMPLEMENT_OPERATOR_NEW(CWaveContext);


CWaveContext::CWaveContext(int _IDCapacity) : m_WaveIDHeap(_IDCapacity)
{
	MAUTOSTRIP( CWaveContext_ctor, MAUTOSTRIP_VOID );

	// Alloc an ID and check that it's zero, Index 0 is not to be used for allocations 
	// since it represents 'no Wave'.
	if (m_WaveIDHeap.AllocID() != 0)
		Error("-", "Internal error.");

	m_IDCapacity = _IDCapacity;

#ifdef PLATFORM_CONSOLE
	m_lpWC.SetGrow(32);
#else
	m_lpWC.SetGrow(1024);
#endif
	// Alloc Waveinfo
	m_lWaveIDInfo.SetLen(m_IDCapacity);
	m_WaveIDHeap.Create(m_IDCapacity);
};

CWaveContext::~CWaveContext()
{
	MAUTOSTRIP( CWaveContext_dtor, MAUTOSTRIP_VOID );


	int nTClass = m_lpWC.Len();
	for (int i = 0; i < nTClass; i++) 
		RemoveWaveClass(i);

	m_lpWC.Clear();
	m_lWaveIDInfo.Clear();
};

void CWaveContext::ClearUsage()
{
	ClearWaveIDFlags(CWC_WAVEIDFLAGS_USED);
}

void CWaveContext::LogUsed(CStr _FileName)
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys)
	{
		// Write resourse log
		CCFile ResourceLog;
		
		if (!_FileName.Len())
			_FileName = pSys->m_ExePath + "\\Waves.wul";

		ResourceLog.Open(_FileName, CFILE_WRITE);

		uint32 NumFound = 0;
		ResourceLog.WriteLE(NumFound);
		
		for (int i = 0; i < GetIDCapacity(); ++i)
		{
			if (GetWaveIDFlag(i) & CWC_WAVEIDFLAGS_USED)
			{
				int iLocal;
				CWaveContainer *pWaveContainer = GetWaveContainer(i, iLocal);

				if (CWaveContainer_Plain *pWaveC = TDynamicCast<CWaveContainer_Plain >(pWaveContainer))
				{
					++NumFound;
					CStr Str = pWaveC->GetName(iLocal);
					Str.Write(&ResourceLog);
				}
			}
		}
		// Rewrite num found
		ResourceLog.Seek(0);
		ResourceLog.WriteLE(NumFound);
	}
}

spCSCC_CodecStream CWaveContext::OpenStream(int _WaveID, int _Flags)
{
	MAUTOSTRIP( CWaveContext_OpenStream, NULL );

	spCSCC_CodecStream spRet = MNew(CSCC_CodecStream);
	M_TRY
	{
		OpenWaveFile(_WaveID, spRet->m_StreamFile);
	}
	M_CATCH(
	catch (CCException)
	{
		return NULL;
	}
	)

	CCFile *pFile = &spRet->m_StreamFile;
	spCSCC_Codec _spCodec = NULL;
	
	// Get image info
	CImage TempImage;
	TempImage.ReadHeader(pFile);
	
	if (TempImage.IsCompressed())
	{		
		// Read codec
		char Format[5];
		pFile->Read(Format, 4);
		Format[4] = 0;
		if (strlen(Format) < 3)
			Error("OpenStream", CStrF("Invalid codec: %s", Format));
		
		CFStr ClassName;
#ifdef PLATFORM_WIN_PC
		MACRO_GetSystemEnvironment(pEnv);
		MACRO_GetSystem;
		if(!pEnv)
			Error("OpenStream", "No env");
		
		int bXDF = D_MXDFCREATE;
		int Platform = D_MPLATFORM;
		if (bXDF && Platform != 0)
			ClassName.CaptureFormated("CMSound_CodecXDF_%s", Format);
		else
			ClassName.CaptureFormated("CMSound_Codec_%s", Format);
#else
   		ClassName.CaptureFormated("CMSound_Codec_%s", Format);
#endif
		
		TPtr<CReferenceCount> _spRefCount = MRTC_GetObjectManager()->CreateObject(ClassName);
		if (!_spRefCount)
			Error("OpenStream", "No such codec:" + ClassName);

		// Read the original format
		CSCC_CodecFormat OriginalFormat;
		OriginalFormat.Load(pFile);

		CSCC_Codec* _pCodec = safe_cast<CSCC_Codec >((CReferenceCount *)_spRefCount);
		if (!_pCodec)
			Error("OpenStream", "Error not a codec");

		_pCodec->m_Format = OriginalFormat;
/*
		if (_pCodec->m_Format.m_Data.GetSampleSize() == 0)
			_pCodec->m_Format.m_Data.SetSampleSize(OriginalFormat.m_Data.GetSampleSize());
	
		if (_pCodec->m_Format.m_Data.GetNumSamples() == 0)
			_pCodec->m_Format.m_Data.SetNumSamples(OriginalFormat.m_Data.GetNumSamples());

		if (_pCodec->m_Format.m_Data.GetSampleRate() == 0)
			_pCodec->m_Format.m_Data.SetSampleRate(OriginalFormat.m_Data.GetSampleRate());

		if (_pCodec->m_Format.m_Data.GetChannels() == 0)
			_pCodec->m_Format.m_Data.SetChannels(OriginalFormat.m_Data.GetChannels());
*/

		if (!_pCodec->CreateDecoderInternal(pFile))
			Error("OpenStream", "Error creating codec internal");

		if (!_pCodec->CreateDecoder(_Flags))
			Error("OpenStream", "Error creating codec");

		_spCodec = _pCodec;
///		_spCodec->m_Format.m_Priority = OriginalFormat.m_Priority;
	}
	else
	{
		// Use raw streamer
		CSCC_Codec_RAW *_pCodec = MNew(CSCC_Codec_RAW);
		if (!_pCodec->CreateDecoderInternal(pFile))
			Error("OpenStream", "Error creating codec internal");

		if (!_pCodec->CreateRAW(&TempImage))
			Error("OpenStream", "Error creating codec");

		_spCodec = _pCodec;
	}

	spRet->m_spCodec = _spCodec;

//	if (_spCodec->m_Format.m_nSamples <= 0)
//		Error("CImage::Compress_Sound", "We don't support decompressing dynamic streams here")

	if (_spCodec->m_Format.m_Data.GetChannels() == 0)
		Error("OpenStream", "No number of channels, error");

//	if (_spCodec->m_Format.m_SampleRate <= 0)
//		Error("CImage::Compress_Sound", "No number of channels, error")				

	_spCodec->m_Format.UpdateSize();

	return spRet;				
}


void CWaveContext::GetWaveData(int _WaveID, CWaveData &_WaveData)
{
	MAUTOSTRIP( CWaveContext_GetWaveData, MAUTOSTRIP_VOID );

	spCSCC_CodecStream spStream = OpenStream(_WaveID, 0);

	if (spStream)
		_WaveData = spStream->m_spCodec->m_Format.m_Data;
}

void CWaveContext::GetWaveLoadedData(int _WaveID, CWaveData &_WaveData)
{
	MAUTOSTRIP( CWaveContext_GetWaveLoadedData, MAUTOSTRIP_VOID );

	int iTC = m_lWaveIDInfo[_WaveID].m_iWaveClass;
	int _iLocal = m_lWaveIDInfo[_WaveID].m_iLocal;
	if ((iTC < 0) || (_iLocal < 0)) Error("GetWaveLoadedData", CStrF("Invalid WaveID %d", _WaveID));

	m_lpWC[iTC]->GetWaveData(_iLocal, _WaveData);
}


CSC_SFXDesc *CWaveContext::GetSFXDesc(const char *_pSFXName)
{
	MAUTOSTRIP( CWaveContext_GetSFXDesc, NULL );

	// Not found, search the WCs for it.
	int iSFX = -1;
	for(int iWC = 0; iWC < m_lpWC.Len(); iWC++)
	{
		CWaveContainer_Plain *pPlain = safe_cast<CWaveContainer_Plain >((CWaveContainer *)m_lpWC[iWC]);
		
		if (pPlain)
		{
			iSFX = pPlain->GetSFXDescIndex(_pSFXName);

			if (iSFX != -1)
				return pPlain->GetSFXDesc(iSFX);
		}
	}
	return NULL;
}

int CWaveContext::GetWaveID(const char *_pSFXName)
{
	MAUTOSTRIP( CWaveContext_GetSFXDesc, NULL );

	// Not found, search the WCs for it.
	int iWave = -1;
	for(int iWC = 0; iWC < m_lpWC.Len(); iWC++)
	{
		CWaveContainer_Plain *pPlain = safe_cast<CWaveContainer_Plain >((CWaveContainer *)m_lpWC[iWC]);
		
		if (pPlain)
		{
			iWave = pPlain->GetIndex(_pSFXName);

			if (iWave != -1)
				return pPlain->GetWaveID(iWave);
		}
	}
	return iWave;
}

fp32 CWaveContext::SFX_GetLength(CSC_SFXDesc *_pSFXDesc, int _iWave)
{
	CWaveData Data;
	int WaveID = _pSFXDesc->GetWaveId(_iWave);
	if(WaveID == -1)
		return 0;
	GetWaveLoadedData(WaveID, Data);
	fp64 PlayRate = Data.GetSampleRate() * _pSFXDesc->GetPitch();
	fp64 BytesPerSecond = Data.GetSampleSize() * Data.GetChannels() * PlayRate;
	
	return 2 * Data.GetNumSamples() / BytesPerSecond;
}


void CWaveContext::ClearWaveIDFlags(int _Flags)
{
	MAUTOSTRIP( CWaveContext_ClearWaveIDFlags, MAUTOSTRIP_VOID );

	for (int i = 0; i < m_IDCapacity; i++) 
		m_lWaveIDInfo[i].m_Flags &= ~_Flags;
}

void CWaveContext::AddWaveIDFlag(int _WaveID, int _Flags)
{
	MAUTOSTRIP( CWaveContext_AddWaveIDFlag, MAUTOSTRIP_VOID );

	m_lWaveIDInfo[_WaveID].m_Flags |= _Flags;
}

int CWaveContext::GetWaveIDFlag(int _WaveID)
{
	MAUTOSTRIP( CWaveContext_GetWaveIDFlag, 0 );

	return m_lWaveIDInfo[_WaveID].m_Flags;
}

void CWaveContext::RemoveWaveIDFlag(int _WaveID, int _Flags)
{
	MAUTOSTRIP( CWaveContext_RemoveWaveIDFlag, MAUTOSTRIP_VOID );

	m_lWaveIDInfo[_WaveID].m_Flags &= ~_Flags;
}


int CWaveContext::AllocID(int _iTC, int _iLocal)
{
	MAUTOSTRIP( CWaveContext_AllocID, 0 );

	int ID = m_WaveIDHeap.AllocID();
	if (ID < 0)
		Error("AllocID", "Out of wave IDs.");
	CWC_WaveIDInfo* pID = &m_lWaveIDInfo[ID];
	pID->m_iWaveClass = _iTC;
	pID->m_iLocal = _iLocal;
	return ID;
};

void CWaveContext::FreeID(int _ID)
{
	MAUTOSTRIP( CWaveContext_FreeID, MAUTOSTRIP_VOID );

	if ((_ID < 0) || (_ID >= m_IDCapacity)) return;
	MakeDirty(_ID);
	CWC_WaveIDInfo* pID = &m_lWaveIDInfo[_ID];
	pID->m_iWaveClass = -1;
	pID->m_iLocal = -1;
	m_WaveIDHeap.FreeID(_ID);
};

int CWaveContext::GetWaveDesc(int _WaveID, CWaveform* _pWaveDesc)
{
	MAUTOSTRIP( CWaveContext_GetWaveDesc, 0 );

	int iTC = m_lWaveIDInfo[_WaveID].m_iWaveClass;
	int iL = m_lWaveIDInfo[_WaveID].m_iLocal;
	if ((iTC < 0) || (iL < 0)) Error("GetWaveDesc", CStrF("Invalid WaveID %d", _WaveID));
	return m_lpWC[iTC]->GetWaveDesc(iL, _pWaveDesc);
};

CWaveContainer *CWaveContext::GetWaveContainer(int _WaveID, int &_iLocal)
{
	MAUTOSTRIP( CWaveContext_GetWaveContainer, NULL );

	int iTC = m_lWaveIDInfo[_WaveID].m_iWaveClass;
	_iLocal = m_lWaveIDInfo[_WaveID].m_iLocal;
	if ((iTC < 0) || (_iLocal < 0)) Error("GetWaveDesc", CStrF("Invalid WaveID %d", _WaveID));
	return m_lpWC[iTC];
}

CWaveform* CWaveContext::GetWaveform(int _WaveID)
{
	MAUTOSTRIP( CWaveContext_GetWaveform, NULL );

	int iTC = m_lWaveIDInfo[_WaveID].m_iWaveClass;
	int iL = m_lWaveIDInfo[_WaveID].m_iLocal;
	if ((iTC < 0) || (iL < 0)) Error("GetWaveform", CStrF("Invalid WaveID %d", _WaveID));
	return m_lpWC[iTC]->GetWaveform(iL);
}

void CWaveContext::OpenWaveFile(int _WaveID, CCFile &_File)
{
	MAUTOSTRIP( CWaveContext_OpenWaveFile, MAUTOSTRIP_VOID );

	int iTC = m_lWaveIDInfo[_WaveID].m_iWaveClass;
	int iL = m_lWaveIDInfo[_WaveID].m_iLocal;
	if ((iTC < 0) || (iL < 0)) Error("GetWaveform", CStrF("Invalid WaveID %d", _WaveID));

	CStream_SubFile * NewFile = MNew(CStream_SubFile);

	m_lpWC[iTC]->OpenWaveFile(iL, NewFile);

	_File.Open(NewFile, CFILE_READ|CFILE_BINARY, true);
}

void CWaveContext::GetWaveFileData( int _WaveID, fint& _Pos, fint& _Size )
{
	MAUTOSTRIP( CWaveContext_GetWaveFileData, MAUTOSTRIP_VOID );

	int iTC = m_lWaveIDInfo[_WaveID].m_iWaveClass;
	int iL = m_lWaveIDInfo[_WaveID].m_iLocal;
	if ((iTC < 0) || (iL < 0)) Error("GetWaveform", CStrF("Invalid WaveID %d", _WaveID));

	m_lpWC[iTC]->GetWaveFileData( iL, _Pos, _Size );
}

void CWaveContext::BuildInto(int _WaveID, CWaveform* _pWave)
{
	MAUTOSTRIP( CWaveContext_BuildInto, MAUTOSTRIP_VOID );

	int iTC = m_lWaveIDInfo[_WaveID].m_iWaveClass;
	int iL = m_lWaveIDInfo[_WaveID].m_iLocal;
	if ((iTC < 0) || (iL < 0)) Error("BuildInto", CStrF("Invalid WaveID %d", _WaveID));
	m_lpWC[iTC]->BuildInto(iL, _pWave);
}

bint CWaveContext::IsWaveValid(int _WaveID)
{
	if (_WaveID < 0 || _WaveID >= GetIDCapacity())
		return false;
	int iTC = m_lWaveIDInfo[_WaveID].m_iWaveClass;
	int iL = m_lWaveIDInfo[_WaveID].m_iLocal;
	if ((iTC < 0) || (iL < 0))
		return false;
	return true;
}

CStr CWaveContext::GetWaveName(int _WaveID)
{
	int iTC = m_lWaveIDInfo[_WaveID].m_iWaveClass;
	int iL = m_lWaveIDInfo[_WaveID].m_iLocal;
	if ((iTC < 0) || (iL < 0)) Error("GetWaveName", CStrF("Invalid WaveID %d", _WaveID));
	return m_lpWC[iTC]->GetWaveName(iL);
}

uint32 CWaveContext::GetWaveNameID(int _WaveID)
{
	int iTC = m_lWaveIDInfo[_WaveID].m_iWaveClass;
	int iL = m_lWaveIDInfo[_WaveID].m_iLocal;
	if ((iTC < 0) || (iL < 0)) Error("GetWaveNameID", CStrF("Invalid WaveID %d", _WaveID));
	return m_lpWC[iTC]->GetWaveNameID(iL);
}

void CWaveContext::MakeDirty(int _ID)
{
	MAUTOSTRIP( CWaveContext_MakeDirty, MAUTOSTRIP_VOID );

	int nSC = m_lpSC.Len();
	for(int i = 0; i < nSC; i++)
	{
		if( m_lpSC[i] )
			m_lpSC[i]->GetWCIDInfo()->m_pWCIDInfo[_ID].m_Fresh = FALSE;
	}
}

int CWaveContext::AddSoundContext(CSoundContext* _pSC)
{
	MAUTOSTRIP( CWaveContext_AddSoundContext, NULL );

	int iSC = -1;
	int nSC = m_lpSC.Len();
	for (int i = 0; ((i < nSC) && (iSC < 0)); i++)
		if (m_lpSC[i] == NULL) iSC = i;

	if (iSC < 0) 
		iSC = m_lpSC.Add(_pSC);
	else
		m_lpSC[iSC] = _pSC;

	return iSC;
};

void CWaveContext::RemoveSoundContext(int _iSC)
{
	MAUTOSTRIP( CWaveContext_RemoveSoundContext, MAUTOSTRIP_VOID );

	if(_iSC < m_lpSC.Len())
		m_lpSC[_iSC] = NULL;
};

CSoundContext* CWaveContext::GetSoundContext( int _iSC )
{
	if( ( _iSC < 0 ) || ( _iSC >= m_lpSC.Len() ) )
		return NULL;

	return m_lpSC[_iSC];
}

int CWaveContext::AddWaveClass(CWaveContainer* _pWClass)
{
	MAUTOSTRIP( CWaveContext_AddWaveClass, -1 );

	int iWClass = -1;
	int nTClass = m_lpWC.Len();
	for (int i = 0; ((i < nTClass) && (iWClass < 0)); i++) 
		if (m_lpWC[i] == NULL) iWClass = i;

	if (iWClass < 0) 
		iWClass = m_lpWC.Add(_pWClass);
	else
		m_lpWC[iWClass] = _pWClass;

	return iWClass;
};

void CWaveContext::RemoveWaveClass(int _iWClass)
{
	MAUTOSTRIP( CWaveContext_RemoveWaveClass, MAUTOSTRIP_VOID );

	if (m_lpWC[_iWClass])
	{
		m_lpWC[_iWClass]->m_pWaveContext = NULL;
		m_lpWC[_iWClass] = NULL;
	}
};

void CWaveContext::Precache_Flush()
{
	for( int i = 0; i < m_lpSC.Len(); i++ )
		if( m_lpSC[i] )
			m_lpSC[i]->Wave_PrecacheFlush();
};


void CWaveContext::LSD_Flush()
{
	for( int i = 0; i < m_lpWC.Len(); i++ )
		if( m_lpWC[i] )
			m_lpWC[i]->LSD_Flush();
};

