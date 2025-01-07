
#include "PCH.h"
#include "WDataRes_Sound.h"
#include "WMapData.h"
#include "WPhysState.h"
#include "WAnimGraph2Instance/WAG2I.h"
#include "WAnimGraph2Instance/WAG2_ClientData.h"
#include "WObjects/WObj_AnimUtils.h"

MRTC_IMPLEMENT_DYNAMIC(CWRes_Sound, CWResource);
MRTC_IMPLEMENT_DYNAMIC(CWRes_Wave, CWResource);
MRTC_IMPLEMENT_DYNAMIC(CWRes_Dialogue, CWResource);

#define DIALOGUEPARSE_BUFSIZE 1024*128

#define IsDigit(_c) (_c >= 0x30 && _c <= 0x39)

//#define DebugLog(s) LogFile(s)
#define DebugLog(s)

//Implement hashtable
uint32 CWRes_Dialogue::ms_lHashTable[128] = { 0 };


template<class T>
T SafeRead(const void* _pBuff, uint32 _Offset, T& _ReadToHere)
{
#ifdef PLATFORM_CONSOLE
	_ReadToHere = *((const T *)((const uint8*)_pBuff + _Offset));
#else
	int Endian = D_MBIGENDIAN;
	if (Endian) // Xenon is big endian
	{
		register uint8* pRead = &((uint8*)_pBuff)[_Offset];
		int nBytes = sizeof(T);
		_ReadToHere = 0;
		int DstShift = (nBytes - 1) * 8;
		while(nBytes > 0)
		{
			_ReadToHere |= (*pRead) << DstShift;
			DstShift -= 8;
			pRead++;
			nBytes--;
		};
	}
	else
	{
		register uint8* pRead = &((uint8*)_pBuff)[_Offset];
		int nBytes = sizeof(T);
		_ReadToHere = 0;
		int DstShift = 0;
		while(nBytes > 0)
		{
			_ReadToHere |= (*pRead) << DstShift;
			DstShift += 8;
			pRead++;
			nBytes--;
		};
	}
#endif

	return _ReadToHere;
};

template<class T>
void SafeWrite(void* _pBuff, uint32 _Offset, T _StoreMe)
{
#ifdef PLATFORM_CONSOLE
	*((T *)((uint8*)_pBuff + _Offset)) = _StoreMe;
#else
	if(_Offset >= DIALOGUEPARSE_BUFSIZE + sizeof(_StoreMe))
		Error_static("CWRes_Dialogue::SafeWrite", "Buffer overrun");

	int Endian = D_MBIGENDIAN;
	if (Endian) // Xenon is big endian
	{
		register uint8* pStore = &((uint8*)_pBuff)[_Offset];
		int nBytes = sizeof(T);
		int Shift = (nBytes - 1) * 8;
		while(nBytes > 0)
		{
			*pStore = uint8((_StoreMe >> Shift) & 0xff);
			Shift -= 8;
			pStore++;
			nBytes--;
		};
	}
	else
	{
		register uint8* pStore = &((uint8*)_pBuff)[_Offset];
		int nBytes = sizeof(T);
		while(nBytes > 0)
		{
			*pStore = uint8(_StoreMe & 0xff);
			_StoreMe >>= 8;
			pStore++;
			nBytes--;
		};
	}
#endif
};



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWRes_Sound
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWRes_Sound::CWRes_Sound()
{
	MAUTOSTRIP(CWRes_Sound_ctor, MAUTOSTRIP_VOID);
	//m_iWC = -1;
	m_iSFX = -1;
}

CSC_SFXDesc* CWRes_Sound::GetSound()
{
	MAUTOSTRIP(CWRes_Sound_GetSound, NULL);
	return m_spWC->GetSFXDesc(m_iSFX);
}

bool CWRes_Sound::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_Sound_Create, false);
	MSCOPE(CWRes_Sound::Create, RES_SOUND);

	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass)) return false;

	m_pWData = _pWData;
	m_Name = _pName;

	return true;
	//		Error("Create", "Undefined sound: " + _Name);
}

void CWRes_Sound::OnLoad()
{
	MAUTOSTRIP(CWRes_Sound_OnLoad, MAUTOSTRIP_VOID);
	MSCOPE(CWRes_Sound::OnLoad, RES_SOUND);

	if (IsLoaded()) return;

	DebugLog("(CWRes_Sound::Create) " + _pName);

	TArray<spCWaveContainer_Plain> lspWC = m_pWData->GetWCList();

	// Not found, search the WCs for it.
	int iSFX = -1;
	{
		// Moved inside scope to make sure it's deleted before any other allocations occur
		CStr Name = m_Name.CopyFrom(4);
		for(int iWC = 0; iWC < lspWC.Len(); iWC++)
		{
			iSFX = lspWC[iWC]->GetSFXDescIndex(Name);
			if (iSFX >= 0)
			{
				//m_iWC = iWC;
				m_iSFX = iSFX;
				m_spWC = lspWC[iWC];
				DebugLog("(CWRes_Sound::Create) Done.");
				break;
			}
		}
	}

	if (iSFX == -1)
		ConOutL("§cf80WARNING: Undefined sound: " + m_Name);

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys->GetEnvironment()->GetValue("rs_preload_sound", "1").Val_int() == 0)
		return;

	M_TRY
	{
		CSC_SFXDesc *pDesc = GetSound();
		if (pDesc)
		{
			MACRO_GetRegisterObject(CWaveContext, pWC, "SYSTEM.WAVECONTEXT");
			if (!pWC)
				Error("OnLoad", "Could not get wavecontext");

			const int ArraySize = 256; // should be more then needed
			int16 aWaves[ArraySize];
			int32 NumWaves = pDesc->EnumWaves(aWaves, ArraySize);

			for(int32 i = 0; i < NumWaves; i++)
				pWC->AddWaveIDFlag(pDesc->GetWaveId(aWaves[i]), CWC_WAVEIDFLAGS_PRECACHE);
			//				m_spWC->GetWaveform(pDesc->m_lWaves[i].m_iLocalWave);
		}
	}
	M_CATCH(
		catch(CCException)
	{
		ConOutL("§cf80WARNING: (CWRes_Sound::OnLoad) Exception while initializing sound " + m_Name);
	}
	)

}

void CWRes_Sound::OnPrecache(class CXR_Engine* _pEngine)
{
	M_TRY
	{
		CSC_SFXDesc *pDesc = GetSound();
		if (pDesc)
		{
			MACRO_GetRegisterObject(CWaveContext, pWC, "SYSTEM.WAVECONTEXT");
			if (!pWC)
				Error("OnPrecache", "Could not get wavecontext");

			const int ArraySize = 256; // should be more then needed
			int16 aWaves[ArraySize];
			int32 NumWaves = pDesc->EnumWaves(aWaves, ArraySize);

			for(int32 i = 0; i < NumWaves; i++)
				pWC->AddWaveIDFlag(pDesc->GetWaveId(aWaves[i]), CWC_WAVEIDFLAGS_PRECACHE);
			//				m_spWC->GetWaveform(pDesc->m_lWaves[i].m_iLocalWave);
		}
	}
	M_CATCH(
		catch(CCException)
	{
		ConOutL("§cf80WARNING: (CWRes_Sound::OnLoad) Exception while precaching sound " + m_Name);
	}
	)
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWRes_Wave
|__________________________________________________________________________________________________
\*************************************************************************************************/
int32 CWRes_Wave::m_CurrentID = 0;

CWRes_Wave::CWRes_Wave()
{
	MAUTOSTRIP(CWRes_Wave_ctor, MAUTOSTRIP_VOID);
	m_SoundDesc.SetPriority(10);
	m_SoundDesc.SetPitch(1);
	m_SoundDesc.SetPitchRandAmp(0);
	m_SoundDesc.SetVolume(1);
	m_SoundDesc.SetVolumeRandAmp(0);
	m_SoundDesc.SetAttnMaxDist(512);
	m_SoundDesc.SetAttnMinDist(64);
	m_SoundDesc.SetCategory(204);

#ifdef USE_HASHED_SFXDESC
	m_SoundDesc.m_SoundIndex = 0;
#else
	//m_SoundDesc.m_SoundName = ""
#endif
	//m_iWC = -1;
	//m_WaveID = -1;
}

//
// Returns the created sound descriptor
//
CSC_SFXDesc* CWRes_Wave::GetSound()
{
	MAUTOSTRIP(CWRes_Sound_GetSound, NULL);

	// Safty check so we don't return anything funky
	//if(m_SoundDesc.GetMode() != CSC_SFXDesc::EWAVE)
	//	return NULL;

	if(m_SoundDesc.m_Datas.m_Pitch <= 0)
	{
		ConOutL(CStrF("§cf80WARNING: Wave resource has pitch of 0, '%s'", m_Name.Str()));
		return NULL;
	}

	return &m_SoundDesc;
}

//
// Creates the resource
//
bool CWRes_Wave::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_Wave_Create, false);
	MSCOPE(CWRes_Sound::Create, RES_WAVE);

	// Create resource
	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass))
		return false;

	m_pWData = _pWData;
	m_Name = _pName;

	return true;
}

void CWRes_Wave::OnLoad()
{
	MAUTOSTRIP(CWRes_Wave_OnLoad, MAUTOSTRIP_VOID);
	MSCOPE(CWRes_Sound::OnLoad, RES_WAVE);

	// What need is there to load it if it's already loaded ?
	if(IsLoaded())
		return;

	// Parse the string
	CStr WholeString = m_Name.CopyFrom(4); // Skip the "WAV:" part
	CFStr Name = WholeString.GetStrSep(":").Str()+1;
	CFStr VolumeStr,PitchStr;

	int32 Volume,Pitch,Min,Max;
	int32 VolumeRand=0,PitchRand=0;
	int32 VolumeCategory = 0;

	VolumeStr = WholeString.GetStrSep(",");
	PitchStr = WholeString.GetStrSep(",");
	Min = WholeString.GetStrSep(",").Val_int();
	Max = WholeString.GetStrSep(",").Val_int();
	VolumeCategory = WholeString.GetStrSep(",").Val_int();

	//
	if(VolumeStr.Find("-") != -1)
	{
		int32 Min = VolumeStr.GetStrSep("-").Val_int();
		int32 Max = VolumeStr.GetStrSep("-").Val_int();

		if(Min > Max)
		{
			int32 Temp = Max;
			Max = Min;
			Min = Temp;
		}

		Volume = Min;
		VolumeRand = (Max-Min);
	}
	else
		Volume = VolumeStr.Val_int();

	//
	if(PitchStr.Find("-") != -1)
	{
		int32 Min = PitchStr.GetStrSep("-").Val_int();
		int32 Max = PitchStr.GetStrSep("-").Val_int();

		if(Min > Max)
		{
			int32 Temp = Max;
			Max = Min;
			Min = Temp;
		}

		Pitch = Min;
		PitchRand = (Max-Min);
	}
	else
		Pitch = PitchStr.Val_int();

	// Extract the names
	TArray<CStr> lNames;
	while(1)
	{
		CStr ThisName = Name.GetStrSep(";");
		if(ThisName.Len() <= 0)
			break;

		lNames.Add(ThisName);
	}

	if(!lNames.Len())
		return;

	// Search the wave containers for the sound
	TArray<spCWaveContainer_Plain> lspWC = m_pWData->GetWCList();
	for(int32 index = 0; index < lspWC.Len(); index++)
	{
		// When we search for the wave
		int16 iWaveLocal = lspWC[index]->GetLocalWaveID(lNames[0]);
		if(iWaveLocal != -1)
		{
			// Create sound desc (Sets the WaveContainer)
			m_SoundDesc.SetWaveContatiner(lspWC[index]);

			CStr Name;
			Name.CaptureFormated("autosfx%.8x",m_CurrentID++); // Exact 16 bytes

#ifdef USE_HASHED_SFXDESC
			m_SoundDesc.m_SoundIndex = StringToHash(Name);
#else
			m_SoundDesc.m_SoundName = Name;
#endif

			m_SoundDesc.SetMode(CSC_SFXDesc::ENORMAL);

			// Get ids
			TArray<int16> lLocalIDs;
			for(int32 i = 0; i < lNames.Len(); i++)
			{
				int16 iWaveLocal = lspWC[index]->GetLocalWaveID(lNames[i]);

				if(iWaveLocal >= 0 )
					lLocalIDs.Add(iWaveLocal);
			}

			if(lLocalIDs.Len())
			{
				// Transfer the ids
				m_SoundDesc.GetNormalParams()->m_lWaves.SetLen(lLocalIDs.Len());
				for(int32 i = 0; i < lLocalIDs.Len(); i++)
					m_SoundDesc.GetNormalParams()->m_lWaves[i] = lLocalIDs[i];
			}

			// Clamp pitch
			if(Pitch > 400)
				Pitch = 400;

			// Apply parameters
			if(Volume != 0)
				m_SoundDesc.SetVolume(Volume/100.0f);
			if(Pitch != 0)
				m_SoundDesc.SetPitch(Pitch/100.0f);
			if(Min != 0)
				m_SoundDesc.SetAttnMinDist(Min);
			if(Max != 0)
				m_SoundDesc.SetAttnMaxDist(Max);
			if (VolumeCategory != 0)
				m_SoundDesc.SetCategory(VolumeCategory);

			m_SoundDesc.SetVolumeRandAmp(VolumeRand/100.0f);
			m_SoundDesc.SetPitchRandAmp(PitchRand/100.0f);
			

			m_spWC = lspWC[index];
			break;
		}
	}
}

void CWRes_Wave::OnPrecache(class CXR_Engine* _pEngine)
{
	M_TRY
	{
		const int ArraySize = 256; // should be more then needed
		int16 WaveIds[ArraySize];
		int32 nWaves = m_SoundDesc.EnumWaves(WaveIds, ArraySize);		

		MACRO_GetRegisterObject(CWaveContext, pWC, "SYSTEM.WAVECONTEXT");
		if (!pWC)
			Error("OnPrecache", "Could not get wavecontext");

		for(int32 i = 0; i < nWaves; i++)
			pWC->AddWaveIDFlag(m_SoundDesc.GetWaveId(WaveIds[i]), CWC_WAVEIDFLAGS_PRECACHE);
	}
	M_CATCH(
		catch(CCException)
	{
		ConOutL("§cf80WARNING: (CWRes_Sound::OnLoad) Exception while precaching sound " + m_Name);
	}
	)
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWRes_Dialogue
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWRes_Dialogue::CWRes_Dialogue()
{
	MAUTOSTRIP(CWRes_Dialogue_ctor, MAUTOSTRIP_VOID);

	m_SubtitleRange = 576;
	m_AttnMaxDist = 688;
	m_AttnMinDist = 64;
	m_bTaggedAnimations = false;
	m_bSpecialVoice = false;

	//Initialize hashtable
	if( ms_lHashTable[0] == ms_lHashTable[1] )
	{
		for(uint8 i = 0; i < 128; i++)
		{
			CFStr Number = CFStrF("%d", i);
			ms_lHashTable[i] = StringToHash(Number.Str());
		}
	}
}

void CWRes_Dialogue::Read_XCD(const char* _pFileName, CMapData* _pMapData)
{
	MSCOPE(CRegistryCompiled::Read_XCR, RES_REGISTRY);
	MAUTOSTRIP(CWRes_Dialogue_Read_XCD, MAUTOSTRIP_VOID);
	CDataFile DFile;
	DFile.Open(_pFileName);
	Read(&DFile);
	DFile.Close();

	UpdateSoundIndexes(_pMapData);
	m_lSoundNames.Clear();
}

void CWRes_Dialogue::Read(CDataFile* _pDFile)
{
	MSCOPE(CRegistryCompiled::Read, RES_REGISTRY);
	MAUTOSTRIP(CWRes_Dialogue_Read, MAUTOSTRIP_VOID);
	CCFile* pFile = _pDFile->GetFile();

	//XCD_ATTRIB
	{
		if (!_pDFile->GetNext("XCD_ATTRIB"))
			Error("Read", "XCD_ATTRIB not found.");
		_pDFile->GetFile()->ReadLE(m_SubtitleRange);
		_pDFile->GetFile()->ReadLE(m_AttnMaxDist);
		_pDFile->GetFile()->ReadLE(m_AttnMinDist);
	}
	// XCD_INDEXES
	{
		if (!_pDFile->GetNext("XCD_INDEXES"))
			Error("Read", "XCD_INDEXES not found.");

		m_lDialogueIndexes.SetLen(_pDFile->GetUserData());
		pFile->Read(m_lDialogueIndexes.GetBasePtr(), m_lDialogueIndexes.ListSize());
#ifdef CPU_BIGENDIAN
		int nIndidies = m_lDialogueIndexes.Len();
		for (int i = 0; i < nIndidies; ++i)
		{
			m_lDialogueIndexes[i].SwapLE();
		}
#endif
	}

	// XCD_DATA
	{
		if (!_pDFile->GetNext("XCD_DATA"))
			Error("Read", "XCD_DATA not found.");

		m_lDialogueData.SetLen(_pDFile->GetUserData());
		pFile->Read(m_lDialogueData.GetBasePtr(), m_lDialogueData.ListSize());
	}
	// XCD_SOUNDNAMES
	{
		if (!_pDFile->GetNext("XCD_SOUNDNAMES"))
			Error("Read", "XCD_SOUNDNAMES not found.");
		m_lSoundNames.SetLen(_pDFile->GetUserData());
		for(int i = 0; i<m_lSoundNames.Len(); ++i)
			m_lSoundNames[i].Read(_pDFile->GetFile());
	}
}

void CWRes_Dialogue::UpdateSoundIndexes(CMapData* _pMapData)
{
	// Set the sounds from the list of sound names
	if (_pMapData)
	{
		TAP<const CStr> pSoundNames = m_lSoundNames;
		for (uint i = 0; i < pSoundNames.Len(); ++i)
		{
			CFStr Sound = pSoundNames[i];
			if (!Sound.IsEmpty() && m_lDialogueIndexes[i].m_Size)
			{
				int16 iSound = (int16)m_pWData->GetResourceIndex(CFStr("SND:") + Sound, WRESOURCE_CLASS_SOUND, _pMapData);
				int16 *pBuf = (int16 *)(m_lDialogueData.GetBasePtr() + m_lDialogueIndexes[i].m_StartPos);
				SafeWrite(pBuf, 0, iSound);
			}
		}
	}
}

#ifndef PLATFORM_CONSOLE
void CWRes_Dialogue::Write(CDataFile* _pDFile)
{
	MAUTOSTRIP(CWRes_Dialogue_Write, MAUTOSTRIP_VOID);
	CCFile* pFile = _pDFile->GetFile();
	try
	{
		// XCD_ATTRIB
		{
			_pDFile->BeginEntry("XCD_ATTRIB");
			_pDFile->GetFile()->WriteLE(m_SubtitleRange);
			_pDFile->GetFile()->WriteLE(m_AttnMaxDist);
			_pDFile->GetFile()->WriteLE(m_AttnMinDist);
			_pDFile->EndEntry(0, DIALOGUE_COMPILED_VERSION);
		}
		// XCD_INDEXES
		{
			_pDFile->BeginEntry("XCD_INDEXES");
#ifdef CPU_BIGENDIAN
			int nIndidies = m_lDialogueIndexes.Len();
			for (int i = 0; i < nIndidies; ++i)
			{
				m_lDialogueIndexes[i].SwapLE();
			}
#endif
			pFile->Write(m_lDialogueIndexes.GetBasePtr(), m_lDialogueIndexes.ListSize());
#ifdef CPU_BIGENDIAN
			int nIndidies = m_lDialogueIndexes.Len();
			for (int i = 0; i < nIndidies; ++i)
			{
				m_lDialogueIndexes[i].SwapLE();
			}
#endif
			_pDFile->EndEntry(m_lDialogueIndexes.Len(), DIALOGUE_COMPILED_VERSION);
		}

		// XCR_STROFFSETS
		{
			_pDFile->BeginEntry("XCD_DATA");
			pFile->Write(m_lDialogueData.GetBasePtr(), m_lDialogueData.ListSize());
			_pDFile->EndEntry(m_lDialogueData.Len(), DIALOGUE_COMPILED_VERSION);
		}
		// XCD_SOUNDNAMES
		{
			_pDFile->BeginEntry("XCD_SOUNDNAMES");
			for(int i = 0; i<m_lSoundNames.Len(); ++i)
				m_lSoundNames[i].Write(_pDFile->GetFile());
			_pDFile->EndEntry(m_lSoundNames.Len(), DIALOGUE_COMPILED_VERSION);
		}
	}
	catch(CCException)
	{
		throw;
	}
}

void CWRes_Dialogue::Write_XCD(const char* _pFileName)
{
	MAUTOSTRIP(CWRes_Dialogue_Write_XCD, MAUTOSTRIP_VOID);
	CDataFile DFile;
	DFile.Create(_pFileName);
	Write(&DFile);
	DFile.Close();
}

#endif


bool CWRes_Dialogue::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_Dialogue_Create, false);
	//	MSCOPE(CWRes_Dialogue::Create, RES_SOUND);

	m_pWData = _pWData;

	if(!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass))
		return false;

	// make sure that we got atleast "DLG:x" in our name
	int Len = CStrBase::StrLen(_pName);
	if(Len < 5)
		return false;

	// cut away the "DLG:" part of the name
	CStr Name = &_pName[4];

	//
	m_lDialogueIndexes.SetLen(0);
	m_lDialogueData.SetLen(0);
	TArray<spCRegistry> lspHierarchy;

#ifndef M_RTM
	m_Base = "";
#endif

	if (_pWData->m_DialogContainer.GetEntry(Name))
	{
		{
			spCCFile spFile = MNew(CCFile);
			_pWData->m_DialogContainer.GetFile(*spFile, Name);

			CDataFile DFile;
			DFile.Open(spFile, 0);
			Read(&DFile);
			DFile.Close();
		}

//#ifndef PLATFORM_CONSOLE
		_pWData->m_DialogContainer.CloseFile();
//#endif

		// Set the sounds from the list of sound names
		if(_pMapData)
		{
			CFStr Sound;
			for(int i= 0; i < m_lSoundNames.Len(); ++i)
			{
				Sound = m_lSoundNames[i];
				if(Sound != "" && m_lDialogueIndexes[i].m_Size)
				{
					int16 iSound = (int16)m_pWData->GetResourceIndex(CFStr("SND:") + Sound, WRESOURCE_CLASS_SOUND, _pMapData);
					int16 *pBuf = (int16 *)(m_lDialogueData.GetBasePtr() + m_lDialogueIndexes[i].m_StartPos);
					SafeWrite(pBuf, 0, iSound);
				}
			}
		}
		m_lSoundNames.Clear();
	}
	else
	{
		CStr FileName = _pWData->ResolveFileName("DIALOGUES\\" + Name + ".XCD");
		if(CDiskUtil::FileExists(FileName))
			// Check if there is a content compiled version first
			Read_XCD(FileName,_pMapData);
		else
		{
			while(Name != "")
			{		
				FileName = _pWData->ResolveFileName("DIALOGUES\\" + Name + ".XRG");
				if(!CDiskUtil::FileExists(FileName))
				{
					ConOutL(CStrF("§cf80WARNING: Dialogue %s does not exist", FileName.Str()));
					return false;
				}

				TArray<CStr> lDefines;
				MACRO_GetSystemRegistry(pSysReg);
				CRegistry* pRegDef = (pSysReg) ? pSysReg->GetDir("GAMECONTEXT\\REGISTRYDEFINES") : NULL;
				if (pRegDef)
				{
					for(int i = 0; i < pRegDef->GetNumChildren(); i++)
						lDefines.Add(pRegDef->GetName(i));
				}

				spCRegistry spReg = REGISTRY_CREATE;
				spReg->XRG_Read(FileName, lDefines);
				if(spReg->GetNumChildren() < 1 || spReg->GetName(0) != "DIALOGUE")
				{
					ConOutL(CStrF("§cf80WARNING: File %s is an invalid Dialogue", FileName.Str()));
					return false;
				}

#ifndef M_RTM
				Name = spReg->GetValue(0);
				if(m_Base == "")
					m_Base = Name;
#endif
				lspHierarchy.Add(spReg->GetChild(0));
				Name = spReg->GetValue(0);
			}

#ifndef M_RTM
			if(lspHierarchy.Len() == 2)
				m_spParent = lspHierarchy[1]->Duplicate();
#endif

			spCRegistry spReg = lspHierarchy[lspHierarchy.Len() - 1];
			for(int h = lspHierarchy.Len() - 2; h >= 0; h--)
			{
				spReg->CopyDir(lspHierarchy[h]);
#ifndef M_RTM
				if(h == 1)
					m_spParent = spReg->Duplicate();
#endif
			}

			CreateFromRegistry(spReg, Name, _pMapData);
		}
	}

	return true;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Class:			CTempDialogue

Comments:		Small class to handle sortable Dialogue ID:s		
\*____________________________________________________________________*/
struct CTempDialogue 
{
	TArray<char> m_lArray;
	uint32 m_Hash;

#ifndef M_RTM
	CStr m_Name;
	CStr m_Sound;

	bool * m_pbHasErrors;
#endif

	CTempDialogue() { };

	CTempDialogue(const CTempDialogue &_Copy) 
	{ 
		m_lArray = _Copy.m_lArray;
		m_Hash = _Copy.m_Hash;
#ifndef M_RTM
		m_Name = _Copy.m_Name;
#endif
	};

	int Compare(const CTempDialogue &_Element) const
	{
#ifndef M_RTM
		if(m_Hash > _Element.m_Hash) return 1;
		if(m_Hash < _Element.m_Hash) return -1;

		//Identical hash values! danger!
		if (m_Name.CompareNoCase(_Element.m_Name) != 0)
		{
			(*m_pbHasErrors) = true;
			LogFile(CStrF("ERROR: Dialogue string hash conflict detected: '%s - '%s'", m_Name.Str(), _Element.m_Name.Str()));
		}
		else if (m_Sound.CompareNoCase(_Element.m_Sound) != 0)
		{
			(*m_pbHasErrors) = true;
			LogFile(CStrF("ERROR: Dialogue string ID duplicate detected: '%s' - '%s', '%s'",m_Name.Str(), m_Sound.Str(),_Element.m_Sound.Str()));
		}
		else
		{
			//an entry is compared to itself...?
		}
		return 0;
#else
		//We don't need to bother with equal values when creating final
		return (m_Hash > _Element.m_Hash) ? 1 : -1;
#endif
	}
};

void CWRes_Dialogue::CreateFromRegistry(CRegistry *_pReg, const char* _pName, CMapData* _pMapData, bool _bCompile)
{
	//Set the "Has errors" parameter
#ifndef M_RTM
	bool bHasErrors = false;
#endif

	// TArray<TThinArray<char> > lDialogueItems; // Only use to store data during the parsing process
	TArray_Sortable<CTempDialogue>	lDialogueItems;
	lDialogueItems.SetLen(_pReg->GetNumChildren());	//Default length will be guaranteed to hold all dialogue items

	uint32 nDialogueItems = 0; //The *real* amount of items

	MSCOPESHORT(CWRes_Dialogue::CreateFromRegistry);

	for(int i = 0; i < _pReg->GetNumChildren(); i++)
	{
		CRegistry *pItem = _pReg->GetChild(i);
		CStr Name = pItem->GetThisName();

		if(Name == "SUBTITLE_RANGE")
			m_SubtitleRange = pItem->GetThisValuei();
		else if(Name == "ATTENUATION")
		{
			CStr Val = pItem->GetThisValue();
			m_AttnMaxDist = Val.GetStrSep(",").Val_fp64();
			m_AttnMinDist = Val.GetStrSep(",").Val_fp64();
		}
		else
		{
			/*  Digit-only, discarded when hashing-method passed
			int nChar = Name.Len();
			int c;
			for(c = 0; c < nChar; c++)
			if(!IsDigit(Name[c]))
			break;

			if(c == nChar)
			{

			// Name only consists of digits
			int Index = Name.Val_int();
			if(Index < 0 || Index > 1024)
			Error("Create", CStrF("Dialogue %s contained and invalid index (%i)", _pName, Index));
			if(Index >= lDialogueItems.Len())
			lDialogueItems.SetLen(Index + 1);

			if(lDialogueItems[Index].Len() != 0)
			Error("Create", CStrF("Dialogue %s contained more than one item with index %i", _pName, Index));

			//*/
			int c;
			for(c = 0; c < Name.Len(); c++)
				if(IsDigit(Name[c]) || ((Name[c]>='A')&&(Name[c]<='Z'))|| ((Name[c]>='a')&&(Name[c]<='z')) )
					break;

			//Discard Ogier-keys
			if( Name.SubStr(0,4).UpperCase() == "OGR_" )
			{
				c = Name.Len();
			}

			if( c < Name.Len() )
			{
				//*
				CTempDialogue &NewDialogue = lDialogueItems[nDialogueItems];
				NewDialogue.m_lArray.Clear();
				NewDialogue.m_Hash = StringToHash(Name.Str());
				//*/
				{
					MSCOPE(CWRes_Dialogue::Parse, RES_SOUND);
					RESOURCE_MEMSCOPE;
					CFStr Sound = Parse(pItem, _pMapData, NewDialogue.m_lArray, _bCompile);
					m_MemUsed -= RESOURCE_MEMDELTA;
#ifndef M_RTM
					NewDialogue.m_Name = Name;
					NewDialogue.m_Sound = Sound;
					NewDialogue.m_pbHasErrors = &bHasErrors;
#endif
					nDialogueItems++;
				}
			}  // Digit-only scope close
		}
	}

	//Sort array
	lDialogueItems.QuickSetLen(nDialogueItems);
	lDialogueItems.Sort();

	int j;

#ifndef M_RTM
	m_lSoundNames.SetLen(nDialogueItems);
	for(j = 0; j < nDialogueItems; j++)
	{
		m_lSoundNames[j] = lDialogueItems[j].m_Sound;
	}

	//See if any hashing conflicts were reported... Not a very informative report, but this is very seldom a problem
	if( bHasErrors )
	{
		Error("Create", CStrF("String hash conflict in dialogue %s", _pName));
	}
#endif

	// Transfer the data from the temporary storing area
	m_lDialogueIndexes.Clear();
	m_lDialogueData.Clear();
	int TotalDataSize = 0;
	for(j = 0; j < nDialogueItems; j++) 
		TotalDataSize += lDialogueItems[j].m_lArray.Len();
	m_lDialogueIndexes.SetLen(nDialogueItems);
	m_lDialogueData.SetLen(TotalDataSize);

	int Pos = 0;
	for(j = 0; j < nDialogueItems; ++j) 
	{
		//if( lDialogueItems[j].array )
		if(lDialogueItems[j].m_lArray.Len())
		{
			memcpy(m_lDialogueData.GetBasePtr()+Pos, lDialogueItems[j].m_lArray.GetBasePtr(), lDialogueItems[j].m_lArray.Len());
			m_lDialogueIndexes[j].m_StartPos = Pos;
			m_lDialogueIndexes[j].m_Size = lDialogueItems[j].m_lArray.Len();
			m_lDialogueIndexes[j].m_Hash = lDialogueItems[j].m_Hash;
			Pos += lDialogueItems[j].m_lArray.Len();
		}
		else
		{
			m_lDialogueIndexes[j].m_StartPos = 0;
			m_lDialogueIndexes[j].m_Size = 0;
			m_lDialogueIndexes[j].m_Hash = 0;
		}
	}
}

int CWRes_Dialogue::AddHeader(int _Flags, int _Type, fp32 _Time, int8 *_pBuf, int &_Pos)
{
	if(_Pos >= DIALOGUEPARSE_BUFSIZE + 4)
		Error("CWRes_Dialogue::AddString", "Buffer overrun");

	int OrgPos = _Pos;
	_pBuf[_Pos++] = 0; // Size is set later
	_pBuf[_Pos++] = (_Flags << 5) | _Type;

	SafeWrite(_pBuf, _Pos, uint16(RoundToInt(_Time * 256)));
	_Pos += 2;
	return OrgPos;
}

void CWRes_Dialogue::AddString(const char *_pSt, int8 *_pBuf, int &_Pos)
{
	int Len = 0;
	if(_pSt != NULL)
	{
		Len = MinMT((mint)248, strlen(_pSt));

		if(_Pos >= DIALOGUEPARSE_BUFSIZE + Len + 1)
			Error("CWRes_Dialogue::AddString", "Buffer overrun");

		memcpy(_pBuf + _Pos, _pSt, Len);
	}
	_Pos += Len;
	_pBuf[_Pos++] = 0;
}

void CWRes_Dialogue::AddUnicodeString(const wchar *_pSt, int8 *_pBuf, int &_Pos)
{
	int Len = 0;
#ifdef PLATFORM_CONSOLE
	if(_pSt != NULL)
	{
		Len = MinMT((mint)248, CStrBase::wcslen(_pSt));
		memcpy(_pBuf + _Pos, _pSt, Len * sizeof(wchar));
	}
#else
	int Endian = D_MBIGENDIAN;
	if (Endian)
	{
		if(_pSt != NULL)
		{
			Len = MinMT((mint)248, CStrBase::wcslen(_pSt));
			if(_Pos >= DIALOGUEPARSE_BUFSIZE + Len + sizeof(wchar))
				Error("CWRes_Dialogue::AddString", "Buffer overrun");

			memcpy(_pBuf + _Pos, _pSt, Len * sizeof(wchar));
			for (int i = 0; i < Len; ++i)
			{
				Swap_uint16(((uint16 *)(_pBuf + _Pos))[i]);
			}
		}
	}
	else
	{
		if(_pSt != NULL)
		{
			Len = MinMT((mint)248, CStrBase::wcslen(_pSt));
			if(_Pos >= DIALOGUEPARSE_BUFSIZE + Len + sizeof(wchar))
				Error("CWRes_Dialogue::AddString", "Buffer overrun");

			memcpy(_pBuf + _Pos, _pSt, Len * sizeof(wchar));
		}
	}
#endif
	_Pos += Len * sizeof(wchar);
	_pBuf[_Pos++] = 0;
	_pBuf[_Pos++] = 0;
}
void CWRes_Dialogue::AddTailer(int _OrgPos, int8 *_pBuf, int &_Pos)
{
	if(_Pos & 1)
		_pBuf[_Pos++] = 0;
	_pBuf[_OrgPos] = (_Pos - _OrgPos) >> 1;
}

CFStr CWRes_Dialogue::Parse(CRegistry *_pReg, CMapData *_pMapData,TArray<char> &_lDialogueItem, bool _bCompile)
{
	int8 Buf[DIALOGUEPARSE_BUFSIZE];
	int Pos = 0;
	void* pBuf = (void*)Buf;

	int iSound = 0;
	CFStr Sound = _pReg->GetThisValue();
	if(Sound == "")
		Sound = _pReg->GetValue("SOUND");
	if(!_bCompile)
	{
		if(Sound != "")
			iSound = m_pWData->GetResourceIndex(CFStr("SND:") + Sound, WRESOURCE_CLASS_SOUND, _pMapData);
	}

	// When compiling a XCD we write a placeholder at the sound place
	SafeWrite(pBuf, Pos, int16(iSound));
	Pos += 2;
	int16 *pnEvents = (int16 *)(((mint)Buf) + Pos);
	Pos += 2;

	int16 nEvents = 0;

	for(int i = 0; i < _pReg->GetNumChildren(); i++)
	{
		CFStr Name = _pReg->GetName(i);
		int EventFlags = 0;

		if (Name[0] == 'S' || Name[0] == 's')
		{
			EventFlags |= EVENTFLAGS_SAMPLELENGTHRELATIVE;
			Name = Name.Copy(1, 1024);
		}
		else if (Name.CompareNoCase("SPECIALVOICE") == 0)
		{
			nEvents++;
			int OrgPos = AddHeader(EventFlags, EVENTTYPE_SPECIALVOICE, 0.0f, Buf, Pos);
			AddTailer(OrgPos, Buf, Pos);
		}
		else if (Name.CompareNoCase("LIGHT") == 0)
		{
			uint8 light_type;
			CStr Val = _pReg->GetValue("LIGHT").UpperCase();
			if(Val == "OFF")
				light_type = LIGHT_OFF;
			else if(Val == "FADE")
				light_type = LIGHT_FADE;
			else
				light_type = LIGHT_ON;

			int OrgPos = AddHeader(EventFlags, EVENTTYPE_LIGHT, 0.0f, Buf, Pos);
			nEvents++;
			SafeWrite(Buf, Pos, light_type); Pos += 1;
			AddTailer(OrgPos, Buf, Pos);
		}
		else if (Name.CompareNoCase("ANIM") == 0)
		{
			uint32 AnimNameHash = _pReg->GetValue(i).StrHash();
			nEvents++;
			int OrgPos = AddHeader(EventFlags, EVENTTYPE_CUSTOM_ANIM, 0.0f, Buf, Pos);
			SafeWrite(Buf, Pos, AnimNameHash); Pos += 4;
			AddTailer(OrgPos, Buf, Pos);
		}
		else if(Name.CompareNoCase("MOVINGHOLD") == 0)
		{
			nEvents++;
			int OrgPos = AddHeader(EventFlags, EVENTTYPE_MOVINGHOLD, 0.0f, Buf, Pos);
			SafeWrite(Buf, Pos, int32(_pReg->GetValue(i).Val_int())); Pos += 4;
			AddTailer(OrgPos, Buf, Pos);
		}
		else if(Name.CompareNoCase("ANIMFLAGS") == 0)
		{
			nEvents++;
			int OrgPos = AddHeader(EventFlags, EVENTTYPE_ANIMFLAGS, 0.0f, Buf, Pos);
			uint16 Flags = CWO_AnimUtils::TranslateDialogueAnimFlags(_pReg->GetValue(i));
			SafeWrite(Buf, Pos, Flags); Pos += 2;
			AddTailer(OrgPos, Buf, Pos);
		}

		fp32 Time = Name.Val_fp64();

		CStr St;
		CRegistry *pChild = _pReg->GetChild(i);
		int iSubtitle = pChild->FindIndex("SUBTITLE");
		if(iSubtitle != -1)
		{
			St = pChild->GetValue("SUBTITLE");

			static const char* lpFlags[] = { "idle", "interactive", "ai", "cutscene", NULL };
			int SubFlags = pChild->GetValue("SUBTITLEFLAG").TranslateFlags(lpFlags);

			nEvents++;
			int OrgPos = AddHeader(EventFlags, EVENTTYPE_SUBTITLE, Time, Buf, Pos);
			SafeWrite(Buf, Pos, uint16(SubFlags)); Pos += 2;
			AddUnicodeString(St.Unicode(), Buf, Pos);
			AddTailer(OrgPos, Buf, Pos);
		}

		int iChoice = pChild->FindIndex("CHOICE");
		if(iChoice != -1)
		{
			St = pChild->GetValue("CHOICE");
			nEvents++;
			int OrgPos = AddHeader(EventFlags, EVENTTYPE_CHOICE, Time, Buf, Pos);
			AddUnicodeString(St.Unicode(), Buf, Pos);
			AddTailer(OrgPos, Buf, Pos);
		}

		int iNoCamera = pChild->FindIndex("NOCAMERA");
		if(iNoCamera != -1)
		{
			St = pChild->GetValue("NOCAMERA");
			nEvents++;
			int OrgPos = AddHeader(EventFlags, EVENTTYPE_NOCAMERA, Time, Buf, Pos);
			AddUnicodeString(St.Unicode(), Buf, Pos);
			AddTailer(OrgPos, Buf, Pos);
		}

		int iUsers = pChild->FindIndex("USERS");
		if(iUsers != -1)
		{
			St = pChild->GetValue("USERS");
			nEvents++;
			int OrgPos = AddHeader(EventFlags, EVENTTYPE_USERS, Time, Buf, Pos);
			AddUnicodeString(St.Unicode(), Buf, Pos);
			AddTailer(OrgPos, Buf, Pos);
		}

		St = pChild->GetValue("LINK");
		if(St != "")
		{
			nEvents++;
			int OrgPos = AddHeader(EventFlags, EVENTTYPE_LINK, Time, Buf, Pos);
			AddString(St, Buf, Pos);
			AddTailer(OrgPos, Buf, Pos);
		}

		St = pChild->GetValue("RANDOMLINK");
		if(St != "")
		{
			nEvents++;
			int OrgPos = AddHeader(EventFlags, EVENTTYPE_RANDOMLINK, Time, Buf, Pos);
			AddString(St, Buf, Pos);
			AddTailer(OrgPos, Buf, Pos);
		}

		St = pChild->GetValue("PRIORITY");
		if(St != "")
		{
			nEvents++;
			int OrgPos = AddHeader(EventFlags, EVENTTYPE_PRIORITY, Time, Buf, Pos);
			SafeWrite(Buf, Pos, int8(St.Val_int())); Pos += 1;
			AddTailer(OrgPos, Buf, Pos);
		}

		St = pChild->GetValue("LISTENER");
		if(St != "")
		{
			nEvents++;
			int OrgPos = AddHeader(EventFlags, EVENTTYPE_LISTENER, Time, Buf, Pos);
			CStr Target = St.GetStrSep(",");
			static const char *FlagsTranslate[] =
			{
				"KeepBehaviourSpeaker", "KeepBehaviourListener", "NoLookListener", "NoLookSpeaker", "NoCrowd", "StopBehaviourCrowd", NULL
			};
			int Flags = St.TranslateFlags(FlagsTranslate);
			SafeWrite(Buf, Pos, int8(Flags)); Pos += 1;
			AddString(Target, Buf, Pos);
			AddTailer(OrgPos, Buf, Pos);
		}

		St = pChild->GetValue("CAMERA");
		if(St != "")
		{
			nEvents++;
			int OrgPos = AddHeader(EventFlags, EVENTTYPE_CAMERA, Time, Buf, Pos);
			uint16 CamMode = St.GetStrSep(",").Val_int() << 12;
			uint16 CamParm = RoundToInt(St.GetStrSep(",").Val_fp64() * 16);
			if (CamParm & 0xf000)
			{
				ConOutL(CStr("ERROR IN CAMERATOKEN! Camera mode parameter exceed 255"));
				CamParm &= 0xfff;
			}

			SafeWrite(Buf, Pos, uint16(CamMode | CamParm)); Pos += 2;
			SafeWrite(Buf, Pos, int16(RoundToInt(St.GetStrSep(",").Val_fp64() * 256))); Pos += 2;	// FOV delta, Fixed point 8.8
			SafeWrite(Buf, Pos, int16(RoundToInt(St.GetStrSep(",").Val_fp64() * 256))); Pos += 2;	// Speaker X offset, Fixed point 8.8
			SafeWrite(Buf, Pos, int16(RoundToInt(St.GetStrSep(",").Val_fp64() * 256))); Pos += 2;	// Speaker Y offset, Fixed point 8.8
			SafeWrite(Buf, Pos, int16(RoundToInt(St.GetStrSep(",").Val_fp64() * 256))); Pos += 2;	// Speaker Z offset, Fixed point 8.8
			SafeWrite(Buf, Pos, int16(RoundToInt(St.GetStrSep(",").Val_fp64() * 256))); Pos += 2;	// Listener X offset, Fixed point 8.8
			SafeWrite(Buf, Pos, int16(RoundToInt(St.GetStrSep(",").Val_fp64() * 256))); Pos += 2;	// Listener Y offset, Fixed point 8.8
			SafeWrite(Buf, Pos, int16(RoundToInt(St.GetStrSep(",").Val_fp64() * 256))); Pos += 2;	// Listener Z offset, Fixed point 8.8
			SafeWrite(Buf, Pos, int16(RoundToInt(St.GetStrSep(",").Val_fp64() * 256))); Pos += 2;	// Camera shake modifier. default 0.15f
			SafeWrite(Buf, Pos, int16(RoundToInt(St.GetStrSep(",").Val_fp64() * 256))); Pos += 2;	// Camera shake anim speed. default 0.55f
			CStr Camera = St.GetStrSep(",");
			AddString(Camera, Buf, Pos);
			SafeWrite(Buf, Pos, int16(St.GetStrSep(",").Val_int())); Pos += 2;
			AddTailer(OrgPos, Buf, Pos);
		}

		St = pChild->GetValue("MOVETOKEN");
		if(St != "")
		{
			nEvents++;
			int OrgPos = AddHeader(EventFlags, EVENTTYPE_MOVETOKEN, Time, Buf, Pos);
			uint8 iToken = St.GetStrSep(",").Val_int();
			SafeWrite(Buf, Pos, iToken); Pos += 1;
			AddString(St, Buf, Pos);
			AddTailer(OrgPos, Buf, Pos);
		}

		for(int8 p = 0; p < 6; p++)
		{
			St = pChild->GetValue(CStrF("SETANIMPROPERTY%i", p));
			if(St != "")
			{
				nEvents++;
				int OrgPos = AddHeader(EventFlags, EVENTTYPE_SETANIMPROPERTY, Time, Buf, Pos);
				int iProp = St.GetStrSep(",").Val_int();
				SafeWrite(Buf, Pos, iProp); Pos += 4;
				fp32 Val = St.GetStrSep(",").Val_fp64();
				int i = *(int *)&Val;
				SafeWrite(Buf, Pos, i); Pos += 4;
				AddTailer(OrgPos, Buf, Pos);
			}
		}

		St = pChild->GetValue("SETANIMPROPERTIES");
		if(St != "")
		{
			nEvents++;
			int OrgPos = AddHeader(EventFlags, EVENTTYPE_SETANIMPROPERTIES, Time, Buf, Pos);
			uint8 *pnProperties = (uint8 *)Buf + Pos;
			Pos += 1;
			int nProperties = 0;
			while(St != "")
			{
				int32 p = St.GetStrSep("=").Val_int();
				fp32 Val = St.GetStrSep(",").Val_fp64();
				SafeWrite(Buf, Pos, p); Pos += 4;
				int i = *(int *)&Val;
				SafeWrite(Buf, Pos, i); Pos += 4;
				nProperties++;
			}
			*pnProperties = nProperties;
			AddTailer(OrgPos, Buf, Pos);
		}

		St = pChild->GetValue("IMPULSE");
		if(St != "")
		{
			nEvents++;
			int OrgPos = AddHeader(EventFlags, EVENTTYPE_IMPULSE, Time, Buf, Pos);
			SafeWrite(Buf, Pos, int16(St.Val_int())); Pos += 2;
			AddTailer(OrgPos, Buf, Pos);
		}

		St = pChild->GetValue("SETHOLD");
		if(St != "")
		{
			nEvents++;
			int OrgPos = AddHeader(EventFlags, EVENTTYPE_SETHOLD, Time, Buf, Pos);
			uint32 Anim = StringToHash(St.GetStrSep(","));
			fp32 Begin = St.GetStrSep(",").Val_fp64();
			fp32 End = St.GetStrSep(",").Val_fp64();
			SafeWrite(Buf, Pos, Anim); Pos += 4;
			SafeWrite(Buf, Pos, int32(Begin * 1000.0f)); Pos += 4;
			SafeWrite(Buf, Pos, int32(End * 1000.0f)); Pos += 4;
			AddTailer(OrgPos, Buf, Pos);
		}

		St = pChild->GetValue("MESSAGE");
		if(St != "")
		{
			nEvents++;
			int OrgPos = AddHeader(EventFlags, EVENTTYPE_MESSAGE, Time, Buf, Pos);
			AddString(St, Buf, Pos);
			AddTailer(OrgPos, Buf, Pos);
		}

		static const char* const lpSetItemEvents[] = { 
			"SETITEM_APPROACH", "SETITEM_APPROACHSCARED", "SETITEM_THREATEN", "SETITEM_IGNORE", "SETITEM_TIMEOUT", "SETITEM_EXIT" };
		for (uint j = 0; j < NUMDIALOGUEITEMS; j++)
		{
			const char* pKeyName = lpSetItemEvents[j];
			St = pChild->GetValue(pKeyName);
			if (j == 0 && St == "")
			{
				CStr s = pChild->GetValue("SETAPPROACHITEM"); // legacy support
				if (s != "")
					St = "#" + s;
			}

			if (St != "")
			{
				nEvents++;
				uint EventType = EVENTTYPE_SETITEM_APPROACH + j;
				int OrgPos = AddHeader(EventFlags, EventType, Time, Buf, Pos);

				AddString(St, Buf, Pos);
				AddTailer(OrgPos, Buf, Pos);
			}
		}
	}

	SafeWrite(pnEvents, 0, nEvents);

	_lDialogueItem.SetLen(Pos);
	memcpy(_lDialogueItem.GetBasePtr(), Buf, Pos);

	return Sound;
}

void CWRes_Dialogue::PreCache(CMapData* _pMapData)
{
	MAUTOSTRIP(CWRes_Dialogue_PreCache, MAUTOSTRIP_VOID);
	for(int i = 0; i < m_lDialogueIndexes.Len(); i++)
		GetSoundIndexBuf(GetDialogueItemAtPos(i), _pMapData);

	// Clear this, since after this there will probably be a new level load
	m_bTaggedAnimations = false;
}

void CWRes_Dialogue::TagAnimations(CMapData* _pMapData, CWAG2I *_pWAGI, CWAG2I_Context *_pContext)
{
	//	if(m_bTaggedAnimations) // Turn this on for optimizations (but it may break something) -JA
	//		return;

	const int lDialogueTypes[] = { AG2_IMPULSEVALUE_DIALOG_BASE, AG2_IMPULSEVALUE_DIALOG_GESTURE, AG2_IMPULSEVALUE_DIALOG_FACIAL, AG2_IMPULSEVALUE_DIALOG_FACIALGESTURE };
	const int nTypes = sizeof(lDialogueTypes) / 4;
	TArray<CXRAG2_Impulse> llImpulses[nTypes];
	TArray<int32> llActionVals[nTypes];
	for(int i = 0; i < nTypes; i++)
		llImpulses[i].SetGrow(256);

	TAP<SDialogueItem> lDialogueIndexes = m_lDialogueIndexes;
	for(int i = 0; i < lDialogueIndexes.Len(); i++)
	{
		if(lDialogueIndexes[i].m_Size > 0)
		{
			SDialogueItem *pItem = &lDialogueIndexes[i];
			const char *pBuf = (char *)(m_lDialogueData.GetBasePtr() + m_lDialogueIndexes[i].m_StartPos);

			pBuf += 2;
			uint16 tmp_u16;
			uint8 tmp_u8;

			int nEvents = SafeRead(pBuf, 0, tmp_u16);
			pBuf += 2;

			int i = 0;
			for(i = 0; i < nEvents; i++)
			{
				const char *pOrg = pBuf;
				int Size = SafeRead(pBuf, 0, tmp_u8); pBuf++;
				int t = SafeRead(pBuf, 0, tmp_u8); pBuf++;
				int Type = t & 31;
				if(Type == EVENTTYPE_SETANIMPROPERTY)
				{
					uint32 iProperty;
					pBuf += 2; //????
					SafeRead(pBuf, 0, iProperty); pBuf += 4;

					int32 iValue;
					SafeRead(pBuf, 0, iValue); pBuf += 4;

					if((iProperty >> AG2_PACKEDANIMPROPERTY_TYPESHIFT) == AG2_PACKEDANIMPROPERTY_TYPE_DIALOGUE)
					{
						int iToken = (iProperty >> AG2_PACKEDANIMPROPERTY_TOKENSHIFT) & AG2_PACKEDANIMPROPERTY_TOKENMASK;
						int iSlot = -1;
						switch(iToken)
						{
						case AG2_TOKEN_DIALOG_BASE: iSlot = 0; break;
						case AG2_TOKEN_DIALOG_GESTURE: iSlot = 1; break;
						case AG2_TOKEN_DIALOG_FACIAL: iSlot = 2; break;
						case AG2_TOKEN_DIALOG_FACIALGESTURE: iSlot = 3; break;
						case AG2_TOKEN_DIALOG_FACIALGESTURE + 1: iSlot = 3; break;
						}

						if(iSlot != -1)
						{
							TAP<CXRAG2_Impulse> lImpulses = llImpulses[iSlot];
							CXRAG2_Impulse Impulse;
							Impulse.m_ImpulseType = iProperty & AG2_PACKEDANIMPROPERTY_SENDIMPULSE_IMPULSETYPEMASK;
							Impulse.m_ImpulseValue = RoundToInt(*(fp32 *)&iValue);
							int i;
							for(i = 0; i < lImpulses.Len(); i++)
							{
								if(lImpulses[i].m_ImpulseValue == Impulse.m_ImpulseValue)
									break;
							}

							if(i == lImpulses.Len())
							{
								llImpulses[iSlot].Add(Impulse);
								llActionVals[iSlot].Add(Impulse.m_ImpulseValue);
							}
						}
					}
				}

				pBuf = pOrg + (Size << 1);
			}
		}
	}

	// Need this pile for exit animations
	TArray<CXRAG2_Impulse> lExitReaction;
	TArray<CXRAG2_Impulse> lExitBlock;
	lExitReaction.Add(CXRAG2_Impulse(AG2_CLD_IMPULSETYPE_DIALOG,AG2_IMPULSEVALUE_DIALOGCONTROL_TERMINATE));
	lExitBlock.Add(CXRAG2_Impulse(AG2_CLD_IMPULSETYPE_DIALOG, 0));
	for(int i = 0; i < nTypes; i++)
		if(llImpulses[i].Len() > 0)
		{
			_pWAGI->TagAnimSetFromBlockReaction(_pContext,_pContext->m_pWPhysState->GetMapData(),_pContext->m_pWPhysState->GetWorldData(),
												CXRAG2_Impulse(AG2_CLD_IMPULSETYPE_DIALOG, lDialogueTypes[i]), llImpulses[i], true);
			lExitBlock[0] = (CXRAG2_Impulse(AG2_CLD_IMPULSETYPE_DIALOG, lDialogueTypes[i]));
			// Precache exit animations as well
			_pWAGI->TagAnimSetFromBlockReactionSwitchState(_pContext,_pContext->m_pWPhysState->GetMapData(),_pContext->m_pWPhysState->GetWorldData(),
				lExitBlock, lExitReaction, llActionVals[i]);
		}

	m_bTaggedAnimations = true;
}

int CWRes_Dialogue::GetNumItems() const
{
	MAUTOSTRIP(CWRes_Dialogue_GetNumItems, 0);
	return 1024;// m_lDialogueIndexes.Len();
}

//Get dialogue item from an integer index (old)
const char *CWRes_Dialogue::GetDialogueItem(int _Index) const
{
	MAUTOSTRIP(CWRes_Dialogue_GetDialogueItem, NULL);

	/* Discarded, numeric-only solution
	if(_Index >= 0 && _Index < m_lDialogueIndexes.Len() && m_lDialogueIndexes[_Index].m_Size)
	return m_lDialogueData.GetBasePtr() + m_lDialogueIndexes[_Index].m_StartPos;
	return NULL;
	//*/
	return GetHashDialogueItem(IntToHash(_Index));
}

//Get dialogue item from a string
const char *CWRes_Dialogue::GetHashDialogueItem(const char * _pName) const
{
	MAUTOSTRIP(CWRes_Dialogue_GetHashDialogueItem, NULL);
	return GetHashDialogueItem(StringToHash(_pName));
}

//Get dialogue item from a hash value
const char *CWRes_Dialogue::GetHashDialogueItem(uint32 _Hash) const
{
	MAUTOSTRIP(CWRes_Dialogue_GetHashDialogueItem, NULL);

	uint16 iDialogue = GetHashPosition(_Hash);
	return GetDialogueItemAtPos(iDialogue);
}

//Get hash position in list
uint16 CWRes_Dialogue::GetHashPosition(uint32 _Hash) const
{
	MAUTOSTRIP(CWRes_Dialogue_GetHashPosition, NULL);
	int32 iCurrent,Offset;
	TAP<const SDialogueItem> pItems = m_lDialogueIndexes;
	iCurrent = Offset = pItems.Len() >> 1;
	Offset += !(Offset & 0x01);

	//Binary-search to find the correct dialogue
	while(Offset > 1)
	{
		Offset = (Offset + 1) >> 1;

		uint32 ThisHash = pItems[iCurrent].m_Hash;
		if(_Hash == ThisHash)
		{
			return iCurrent;
		}
		else
		{
			iCurrent += (_Hash > ThisHash) ? Offset : -Offset;
			iCurrent = Clamp(iCurrent, 0, pItems.Len()-1);
		}
	}

	//Check once more...
	uint32 ThisHash = m_lDialogueIndexes[iCurrent].m_Hash;
	if(_Hash == ThisHash)
	{
		return iCurrent;
	}

	return pItems.Len();
}


#ifndef M_RTM
/*
void CWRes_Dialogue::SetDialogueItem(int _Index, const CDialogueItem &_Item)
{
MAUTOSTRIP(CWRes_Dialogue_SetDialogueItem, MAUTOSTRIP_VOID);
if(_Index < 0)
return;

m_lDialogueItems.SetLen(Max(_Index + 1, m_lDialogueItems.Len()));

bool bDefault = m_lDialogueItems[_Index].m_bDefault;
if(_Item.m_Text != m_lDialogueItems[_Index].m_Text)
bDefault = false;
if(_Item.m_Sound != m_lDialogueItems[_Index].m_Sound)
bDefault = false;
if(_Item.m_SubtitleDuration != m_lDialogueItems[_Index].m_SubtitleDuration)
bDefault = false;

m_lDialogueItems[_Index] = _Item;
m_lDialogueItems[_Index].m_bDefault = bDefault;
}

void CWRes_Dialogue::Write(CStr _Path)
{
MAUTOSTRIP(CWRes_Dialogue_Write, MAUTOSTRIP_VOID);
spCRegistry spWriteReg = REGISTRY_CREATE;
if (!spWriteReg)
MemError("Write");
spWriteReg->SetThisKey("dialogue", m_Base);
for(int i = 0; i < m_lDialogueItems.Len(); i++)
{
if(!m_lDialogueItems[i].m_bDefault && !m_lDialogueItems[i].IsEmpty())
{
spCRegistry spReg = m_lDialogueItems[i].GetReg();
if(!spReg)
return;
spReg->RenameThisKey(CStrF("%i", i));
spWriteReg->AddReg(spReg);
}
}
spWriteReg->XRG_Write(_Path);
}
*/
#endif

bool CWRes_Dialogue::Refresh(const CMTime& _Time, fp32 _dTime, CDialogueInstance &_Instance, CDialogueToken *_pToken, CRefreshRes *_pRes)
{
	fp32 Duration = (_Time - _Instance.m_StartTime).GetTime();
	const char *pBuf = GetHashDialogueItem(_Instance.m_DialogueItemHash);
	if(!pBuf)
	{
		_Instance.m_Subtitle = "";
		_Instance.m_Choice = "";
		_Instance.m_DialogueItemHash = 0;
		if(_pRes)
		{
			_pRes->m_Events |= 1 << EVENTTYPE_LINK;
			_pRes->m_pLink = (const char *)-1;
		}
		return true;
	}

	pBuf += 2;
	uint16 tmp_u16;
	int8 tmp_s8;
	uint8 tmp_u8;

	int nEvents = SafeRead(pBuf, 0, tmp_u16);
	pBuf += 2;
	int i = 0;
	for(i = 0; i < nEvents; i++)
	{
		Duration = (_Time - _Instance.m_StartTime).GetTime();
		const char *pOrg = pBuf;
		int Size = SafeRead(pBuf, 0, tmp_u8); pBuf++;
		int t = SafeRead(pBuf, 0, tmp_u8); pBuf++;
		int Type = t & 31;
		int Flags = t >> 5;
		fp32 EventTime = fp32(SafeRead(pBuf, 0, tmp_u16)) * (1.0f / 256.0f);
		pBuf += 2;
		if (Flags & EVENTFLAGS_SAMPLELENGTHRELATIVE)
			EventTime += _Instance.m_SampleLength;

		if(Type == EVENTTYPE_SUBTITLE)
		{
			if (!(Duration < EventTime))
			{
				if (EventTime >= (Duration - _dTime))
				{
					if(_pRes)
						_pRes->m_Events |= 1 << EVENTTYPE_SUBTITLE;
					uint16 Tmp;
					_Instance.m_SubtitleFlags = SafeRead(pBuf, 0, Tmp);
					pBuf = pOrg + (Size << 1);
					_Instance.m_Subtitle = "";
					continue;
				}
			}
/*			EventTime += 0.2f;
			if (Duration < EventTime)
			{
				pBuf = pOrg + (Size << 1);
				continue;
			}*/

			Duration -= 0.2f;
			Duration = Max(Duration, 0.0f);
			if (Duration < EventTime)
			{
				pBuf = pOrg + (Size << 1);
				continue;
			}
		}

		if (Duration < EventTime)
			break;
		if (EventTime >= (Duration - _dTime))
		{
			switch(Type)
			{
			case EVENTTYPE_SUBTITLE:
				if(_pRes)
					_pRes->m_Events |= 1 << EVENTTYPE_SUBTITLE;
				uint16 Tmp;
				_Instance.m_SubtitleFlags = SafeRead(pBuf, 0, Tmp);
				_Instance.m_Subtitle = CStr((wchar*)(pBuf + 2));
				break;
				/*			case EVENTTYPE_CHOICE:
				if(_pRes)
				_pRes->m_Events |= 1 << EVENTTYPE_CHOICE;
				_Instance.m_Choice = CStr((wchar*)pBuf);
				break;*/
			case EVENTTYPE_CAMERA:
				if(_pToken /*&& !(_Instance.m_Flags & DIALOGUEFLAGS_PAUSED)*/)
				{
					void* pRead = (void*)pBuf;
					_pToken->m_StartGameTime = _Time;

					int Pos = 0;
					uint16 Tmp;
					int16 TmpInt;
					uint16 Camera = SafeRead(pRead, Pos, Tmp); Pos += 2;
					_pToken->m_CameraMode = Camera >> 12;
					_pToken->m_CameraModeParameter = int(fp32((Camera&0xfff) << 4) / 256.0f);
					_pToken->m_FOV = fp32(SafeRead(pRead, Pos, Tmp)) / 256.0f; Pos += 2;
					//_pToken->m_dFOV = fp32(SafeRead(pRead, Pos, Tmp)) / 256.0f; Pos += 2;
					_pToken->m_CameraSpeakerOffset.k[0] = fp32(SafeRead(pRead, Pos, TmpInt)) / 256.0f; Pos += 2;
					_pToken->m_CameraSpeakerOffset.k[1] = fp32(SafeRead(pRead, Pos, TmpInt)) / 256.0f; Pos += 2;
					_pToken->m_CameraSpeakerOffset.k[2] = fp32(SafeRead(pRead, Pos, TmpInt)) / 256.0f; Pos += 2;
					_pToken->m_CameraListenerOffset.k[0] = fp32(SafeRead(pRead, Pos, TmpInt)) / 256.0f; Pos += 2;
					_pToken->m_CameraListenerOffset.k[1] = fp32(SafeRead(pRead, Pos, TmpInt)) / 256.0f; Pos += 2;
					_pToken->m_CameraListenerOffset.k[2] = fp32(SafeRead(pRead, Pos, TmpInt)) / 256.0f; Pos += 2;

					fp32 temp = (fp32(SafeRead(pRead, Pos, TmpInt)) / 256.0f) / 100.0f;	Pos += 2;
					if(temp != 0.0f)
						_pToken->m_CameraShakeVal = Max(Min(temp, 1.0f), 0.0f);
									
					temp = (fp32(SafeRead(pRead, Pos, TmpInt)) / 256.0f) / 100.0f; Pos += 2;
					if(temp != 0.0f)
						_pToken->m_CameraShakeSpeed = Max(temp, 0.0f);
					
					_pToken->m_Camera_Scripted = CStr(pBuf + Pos);
					Pos += _pToken->m_Camera_Scripted.Len() + 1;
					SafeRead(pRead, Pos, _pToken->m_CameraTelephoneScript); Pos += 2;
				}
				break;
			case EVENTTYPE_NOCAMERA:
				if(_pRes && !(_Instance.m_Flags & DIALOGUEFLAGS_PAUSED))
				{
					_pRes->m_Events |= 1 << EVENTTYPE_NOCAMERA;
					_pRes->m_pLink = pBuf;
				}
				break;
			case EVENTTYPE_LINK:
				if(_pRes && !(_Instance.m_Flags & DIALOGUEFLAGS_PAUSED))
				{
					_pRes->m_Events |= 1 << EVENTTYPE_LINK;
					_pRes->m_pLink = pBuf;
				}
				break;
			case EVENTTYPE_SPECIALVOICE:
				if(_pRes && !(_Instance.m_Flags & DIALOGUEFLAGS_PAUSED))
				{
					_pRes->m_Events |= 1 << EVENTTYPE_SPECIALVOICE;
				}
				break;
			case EVENTTYPE_RANDOMLINK:
				if(_pRes && !(_Instance.m_Flags & DIALOGUEFLAGS_PAUSED))
				{
					_pRes->m_Events |= 1 << EVENTTYPE_RANDOMLINK;
					_pRes->m_pLink = pBuf;
				}
				break;
			case EVENTTYPE_MOVETOKEN:
				if(_pRes && !(_Instance.m_Flags & DIALOGUEFLAGS_PAUSED))
				{
					_pRes->m_Events |= 1 << EVENTTYPE_MOVETOKEN;
					_pRes->m_iToken = SafeRead(pBuf, 0, tmp_s8); pBuf += 1;
					_pRes->m_pAction = pBuf;
				}
				break;
			case EVENTTYPE_SETANIMPROPERTY:
				if(_pRes && !(_Instance.m_Flags & DIALOGUEFLAGS_PAUSED))
				{
					_pRes->m_Events |= 1 << EVENTTYPE_SETANIMPROPERTIES;
					int32 tmp_i;
					CDialoguePropertyChange Change;
					Change.m_Property = SafeRead(pBuf, 0, tmp_i); pBuf += 4;
					int i2 = SafeRead(pBuf, 0, tmp_i); pBuf += 4;
					Change.m_Value = *(fp32 *)&i2;
					_pRes->m_lPropertyChanges.SetLen(_pRes->m_lPropertyChanges.Len() + 1);
					_pRes->m_lPropertyChanges[_pRes->m_lPropertyChanges.Len() - 1] = Change;
				}
				break;
#if defined(PLATFORM_WIN32) && !defined(M_RTM)
			case EVENTTYPE_SETANIMPROPERTIES:
				if(_pRes && !(_Instance.m_Flags & DIALOGUEFLAGS_PAUSED))
				{
					int Pos = 0;
					uint8 nProp;
					SafeRead(pBuf, 0, nProp); pBuf += 1;
					_pRes->m_lPropertyChanges.SetLen(nProp);
					for(int i = 0; i < nProp; i++)
					{
						SafeRead(pBuf, Pos, _pRes->m_lPropertyChanges[i].m_Property); Pos += 4;

						int32 tmp_i;
						int i2 = SafeRead(pBuf, Pos, tmp_i); pBuf += 4;
						_pRes->m_lPropertyChanges[i].m_Value = *(fp32 *)&i2;
					}
					_pRes->m_Events |= 1 << EVENTTYPE_SETANIMPROPERTIES;
				}
				break;
#endif
			case EVENTTYPE_MESSAGE:
				if(_pRes && !(_Instance.m_Flags & DIALOGUEFLAGS_PAUSED))
				{
					_pRes->m_Events |= 1 << EVENTTYPE_MESSAGE;
					_pRes->m_pMessage = pBuf;
				}
				break;
			case EVENTTYPE_SETHOLD:
				if(_pRes && !(_Instance.m_Flags & DIALOGUEFLAGS_PAUSED))
				{
					_pRes->m_Events |= 1 << EVENTTYPE_SETHOLD;
					int32 tmp_i;
					_pRes->m_SetHoldAnim = SafeRead(pBuf, 0, tmp_i); pBuf += 4;
					_pRes->m_SetHoldBegin = fp32(SafeRead(pBuf, 0, tmp_i)) / 1000.0f; pBuf += 4;
					_pRes->m_SetHoldEnd = fp32(SafeRead(pBuf, 0, tmp_i)) / 1000.0f; pBuf += 4;
				}
				break;
			case EVENTTYPE_IMPULSE:
				if(_pRes && !(_Instance.m_Flags & DIALOGUEFLAGS_PAUSED))
				{
					if(!(_pRes->m_Events & 1 << EVENTTYPE_IMPULSE))
						_pRes->m_Impulse = 0;
					_pRes->m_Events |= 1 << EVENTTYPE_IMPULSE;
					_pRes->m_Impulse |= 1 << (SafeRead(pBuf, 0, tmp_u16));
				}
				break;
			case EVENTTYPE_SETITEM_APPROACH:
			case EVENTTYPE_SETITEM_APPROACH_SCARED:
			case EVENTTYPE_SETITEM_THREATEN:
			case EVENTTYPE_SETITEM_IGNORE:
			case EVENTTYPE_SETITEM_TIMEOUT:
			case EVENTTYPE_SETITEM_EXIT:
				if(_pRes && !(_Instance.m_Flags & DIALOGUEFLAGS_PAUSED))
				{
					uint iItem = Type - EVENTTYPE_SETITEM_APPROACH;
					M_ASSERT(iItem < NUMDIALOGUEITEMS, "!");
					_pRes->m_Events |= 1 << Type;
					_pRes->m_pItems[iItem] = pBuf;
				}
				break;
			}
		}
		pBuf = pOrg + (Size << 1);
	}

	if(i == nEvents && Duration > _Instance.m_SampleLength)
	{
		_Instance.m_Subtitle = "";
		_Instance.m_Choice = "";
		_Instance.m_DialogueItemHash = 0;
		if(_pRes && !(_pRes->m_Events & (1 << EVENTTYPE_LINK)) && !(_pRes->m_Events & (1 << EVENTTYPE_RANDOMLINK)))
		{
			_pRes->m_Events |= 1 << EVENTTYPE_LINK;
			_pRes->m_pLink = (const char *)-1;
		}
		return true;
	}
	return false;
}


/*
bool CWRes_Dialogue::IsQuickSound(int _iDialogue)
{
uint16 i = GetHashPosition(IntToHash(_iDialogue));
if(i < m_lDialogueIndexes.Len())
return m_lDialogueIndexes[i].m_Size == 4;
else
return true;
}*/


bool CWRes_Dialogue::IsQuickSound_Hash(uint32 _ItemHash) const
{
	uint16 i = GetHashPosition(_ItemHash);
	if(i < m_lDialogueIndexes.Len())
		return m_lDialogueIndexes[i].m_Size == 4;
	else
		return true;
}


bool CWRes_Dialogue::IsQuickSound(int _iDialogue)  const
{
	return IsQuickSound_Hash(IntToHash(_iDialogue));
}


bool CWRes_Dialogue::HasLinkBuf(const char* _pBuf) const
{
	if(!_pBuf)
		return false;

	_pBuf += 2;
	uint16 tmp_u16;
	uint8 tmp_u8;

	int nEvents = SafeRead(_pBuf, 0, tmp_u16);
	_pBuf += 2;

	int i = 0;
	for(i = 0; i < nEvents; i++)
	{
		const char *pOrg = _pBuf;
		int Size = SafeRead(_pBuf, 0, tmp_u8); _pBuf++;
		int t = SafeRead(_pBuf, 0, tmp_u8); _pBuf++;
		int Type = t & 31;
		if(Type == EVENTTYPE_LINK || Type == EVENTTYPE_RANDOMLINK)
			return true;
		_pBuf = pOrg + (Size << 1);
	}
	return false;
}

/*bool CWRes_Dialogue::HasLink(int _iDialogue)
{
return HasLinkBuf(GetDialogueItem(_iDialogue));
}*/

bool CWRes_Dialogue::HasLink(int _iDialogue) const
{
	uint32 Hash = IntToHash(_iDialogue);
	return HasLink_Hash(Hash);
}


bool CWRes_Dialogue::HasLink_Hash(uint32 _iDialogue) const
{
	return HasLinkBuf(GetHashDialogueItem(_iDialogue));
}


int CWRes_Dialogue::GetSoundIndexBuf(const char * _pBuf, CMapData *_pMapData, uint16 *_piType) const
{
	const int16 *pBuf = (const int16 *)_pBuf;
	if(!pBuf)
		return 0;

	int16 iSound;
	SafeRead(pBuf, 0, iSound);
	if(iSound == 0)
		return 0;

	CWResource *pRes = _pMapData->m_spWData->GetResource(iSound);
	if(!pRes || pRes->GetClass() != WRESOURCE_CLASS_SOUND)
		return 0;

	if(_piType)
	{
		uint16 iType = 0;
		const char *pSub = FindEventBuf(_pBuf, EVENTTYPE_SUBTITLE);
		if(pSub)
			SafeRead(pSub, 0, iType);

		*_piType = iType;
	}

	return _pMapData->GetResourceIndex(pRes->GetName(), WRESOURCE_CLASS_SOUND);
}


/*
int CWRes_Dialogue::GetSoundIndex(int _iItem, CMapData *_pMapData, int *_piType)
{
return GetSoundIndexBuf(GetDialogueItem(_iItem),_pMapData,_piType);
}*/


int CWRes_Dialogue::GetSoundIndex(int _iItem, CMapData* _pMapData, uint16* _piType) const
{
	return GetSoundIndex_Hash(IntToHash(_iItem), _pMapData, _piType);
}


int CWRes_Dialogue::GetSoundIndex_Hash(uint32 _ItemHash, CMapData* _pMapData, uint16* _piType) const
{
	return GetSoundIndexBuf(GetHashDialogueItem(_ItemHash), _pMapData, _piType);
}


fp32 CWRes_Dialogue::GetSampleLengthBuf(const char* _pBuf) const
{
	if(!_pBuf)
		return 0;

	int16 iIndex;
	SafeRead(_pBuf, 0, iIndex);
	if(iIndex != 0)
	{
		CWResource *pRes = m_pWData->GetResource(iIndex);
		if(pRes && pRes->GetClass() == WRESOURCE_CLASS_SOUND)
		{
			CWRes_Sound *pSoundRes = (CWRes_Sound *)pRes;
			if(pSoundRes)
			{
				CSC_SFXDesc *pSound = pSoundRes->GetSound();
				if(pSound)
				{
					MACRO_GetRegisterObject(CWaveContext, pWC, "SYSTEM.WAVECONTEXT");
					if(pWC)
					{
						fp32 Duration = pWC->SFX_GetLength(pSound, 0) + 0.4f;
						return Duration;
					}
				}
			}
		}
	}

	// Didn't find sample. Find the last timed subtitle entry and a few additional seconds
	_pBuf += 2;

	uint16 tmp_u16;
	uint8 tmp_u8;

	int nEvents = SafeRead(_pBuf, 0, tmp_u16);
	_pBuf += 2;
	fp32 Time = -1.0f;

	for(int i = 0; i < nEvents; i++)
	{
		const char *pOrg = _pBuf;
		int Size = SafeRead(_pBuf, 0, tmp_u8); _pBuf++;
		int t = SafeRead(_pBuf, 0, tmp_u8); *_pBuf++;
		int Type = t & 31;
		int Flags = t >> 5;

		if(!(Flags & EVENTFLAGS_SAMPLELENGTHRELATIVE))
		{
			switch(Type)
			{
			case EVENTTYPE_SUBTITLE:
				Time = SafeRead(_pBuf, 0, tmp_u16) * (1.0f / 256.0f);
				_pBuf += 4;
				int Len = CStr((wchar*)_pBuf).Len();
				if (Len > 0)
					Time += Max(Len * 18, 40) * (1.0f / 256.0f);
				break;
			}
		}
		_pBuf = pOrg + (Size << 1);
	}

	return Max(0.0f, Time);
}

/*
fp32 CWRes_Dialogue::GetSampleLength(int _iItem)
{
return GetSampleLengthBuf(GetDialogueItem(_iItem));
}*/


fp32 CWRes_Dialogue::GetSampleLength(int _iItem) const
{
	return GetSampleLength_Hash(IntToHash(_iItem));
}


fp32 CWRes_Dialogue::GetSampleLength_Hash(uint32 _ItemHash) const
{
	return GetSampleLengthBuf(GetHashDialogueItem(_ItemHash));
}


uint8 CWRes_Dialogue::GetLightType(uint32 _ItemHash) const
{
	uint iDialogue = GetHashPosition(_ItemHash);
	const char *pBuf = GetDialogueItemAtPos(iDialogue);

	if (!pBuf)
		return LIGHT_OFF;

	pBuf += 2;

	uint16 tmp_u16;
	uint8 tmp_u8;

	int nEvents = SafeRead(pBuf, 0, tmp_u16);

	pBuf += 2;

	for (int i = 0; i < nEvents; i++)
	{
		const char *pOrg = pBuf;
		int Size = SafeRead(pBuf, 0, tmp_u8); pBuf++;
		int t = SafeRead(pBuf, 0, tmp_u8); pBuf++;
		int Type = t & 31;
		int Flags = t >> 5;
		fp32 EventTime = fp32(SafeRead(pBuf, 0, tmp_u16)) * (1.0f / 256.0f);
		pBuf += 2;

		if (Type == EVENTTYPE_LIGHT)
		{
			uint8 light_type;
			SafeRead(pBuf, 0, light_type); pBuf++;
			return light_type;
		}
		pBuf = pOrg + (Size << 1);
	}
	return LIGHT_FADE;
}


const char* CWRes_Dialogue::FindEventBuf(const char* _pBuf, int _Event, fp32* _pTime /* = NULL */, int* _pFlags /* = NULL */) const
{
	if (!_pBuf)
		return "";

	fp32 SampleLength = GetSampleLengthBuf(_pBuf);
	_pBuf += 2;

	uint16 tmp_u16;
	uint8 tmp_u8;

	int nEvents = SafeRead(_pBuf, 0, tmp_u16);

	_pBuf += 2;
	//	fp32 Time = 0;

	for (int i = 0; i < nEvents; i++)
	{
		const char *pOrg = _pBuf;
		int Size = SafeRead(_pBuf, 0, tmp_u8); _pBuf++;
		int t = SafeRead(_pBuf, 0, tmp_u8); _pBuf++;
		int Type = t & 31;
		int Flags = t >> 5;
		fp32 EventTime = fp32(SafeRead(_pBuf, 0, tmp_u16)) * (1.0f / 256.0f);
		_pBuf += 2;
		if (Flags & EVENTFLAGS_SAMPLELENGTHRELATIVE)
			EventTime += SampleLength;

		if (Type == _Event)
		{
			if (_pFlags)
				*_pFlags = Flags;
			if (_pTime)
				*_pTime = EventTime;
			return _pBuf;
		}
		_pBuf = pOrg + (Size << 1);
	}
	return NULL;
}

/*
const char *CWRes_Dialogue::FindEvent(int _iItem, int _Event, fp32* _pTime, int *_pFlags)
{
return FindEventBuf(GetDialogueItem(_iItem),_Event,_pTime,_pFlags);
}*/

const char *CWRes_Dialogue::FindEvent(int _iItem, int _Event, fp32* _pTime, int *_pFlags) const
{
	uint32 Hash = IntToHash(_iItem);
	return FindEvent_Hash(Hash, _Event, _pTime, _pFlags);
}

const char *CWRes_Dialogue::FindEvent_Hash(uint32 _ItemHash, int _Event, fp32* _pTime, int *_pFlags) const
{
	return FindEventBuf(GetHashDialogueItem(_ItemHash), _Event, _pTime, _pFlags);
}


int CWRes_Dialogue::GetPriorityBuf(const char * _pBuf) const
{
	if( _pBuf == NULL )
		return PRIORITY_DEFAULT;

	const char *pPrio = FindEventBuf(_pBuf, EVENTTYPE_PRIORITY);
	if(pPrio)
		return *(uint8 *)pPrio;

	const char *pSub = FindEventBuf(_pBuf, EVENTTYPE_SUBTITLE);
	if(pSub)
	{
		uint16 Flags;
		SafeRead(pSub, 0, Flags);
		switch(Flags & SUBTITLE_TYPE_MASK)
		{
		case SUBTITLE_TYPE_IDLE: return PRIORITY_IDLE;
		case SUBTITLE_TYPE_INTERACTIVE: return PRIORITY_DEFAULT;
		case SUBTITLE_TYPE_AI: return PRIORITY_DEFAULT;
		case SUBTITLE_TYPE_CUTSCENE: return PRIORITY_DEFAULT;
		case SUBTITLE_TYPE_CUTSCENEKEEPENDANIM: return PRIORITY_DEFAULT;
		}
	}
	return PRIORITY_DEFAULT;

}


int CWRes_Dialogue::GetPriority_Hash(uint32 _iDialogue) const
{
	return GetPriorityBuf(GetHashDialogueItem(_iDialogue));
}


int CWRes_Dialogue::GetPriority(int _iDialogue) const
{
	return GetPriorityBuf(GetDialogueItem(_iDialogue));
}


int CWRes_Dialogue::GetMovingHold(uint32 _ItemHash) const
{
	int32 iMovingHold = 1;
	const char* pEventData = FindEvent_Hash(_ItemHash, EVENTTYPE_MOVINGHOLD);
	if (pEventData)
		SafeRead(pEventData, 0, iMovingHold);
	return iMovingHold;
}


uint32 CWRes_Dialogue::GetCustomAnim(uint32 _ItemHash) const
{
	uint32 AnimNameHash = 0;
	const char* pEventData = FindEvent_Hash(_ItemHash, EVENTTYPE_CUSTOM_ANIM);
	if (pEventData)
		SafeRead(pEventData, 0, AnimNameHash);
	return AnimNameHash;
}

uint16 CWRes_Dialogue::GetDialogueAnimFlags(uint32 _ItemHash) const
{
	uint16 AnimNameHash = 0;
	const char* pEventData = FindEvent_Hash(_ItemHash, EVENTTYPE_ANIMFLAGS);
	if (pEventData)
		SafeRead(pEventData, 0, AnimNameHash);
	return AnimNameHash;
}

#ifndef M_RTM

CStr CWRes_Dialogue::Ogr_GetDescription(int _iItem)
{
	CDialogueInstance Instance;
	Instance.m_DialogueItemHash = IntToHash(_iItem);
	Instance.m_StartTime = CMTime();
	Instance.m_SampleLength = 32767;
	Refresh(CMTime(), 0.05f, Instance);
	CStr Desc;
	if(Instance.m_Subtitle != "")
		Desc += Instance.m_Subtitle.Ansi() + " ";
	CStr Sound = Ogr_GetSound(_iItem);
	if(Sound != "")
		Desc += "(" + Sound + ")";
	return Desc;
}

CStr CWRes_Dialogue::Ogr_GetDescription_Hash(uint32 _ItemHash)
{
	CDialogueInstance Instance;
	Instance.m_DialogueItemHash = _ItemHash;
	Instance.m_StartTime = CMTime();
	Instance.m_SampleLength = 32767;
	Refresh(CMTime(), 0.05f, Instance);
	CStr Desc = Instance.m_Subtitle;
	CStr Sound = Ogr_GetSound_Hash(_ItemHash);
	if(Desc != "")
		Desc += " ";
	if(Sound != "")
		Desc += "(" + Sound + ")";
	return Desc;
}

CStr CWRes_Dialogue::Ogr_GetSound(int _iItem)
{
	uint16 i = GetHashPosition(IntToHash(_iItem));

	if( i < m_lDialogueIndexes.Len() )
		return m_lSoundNames[i];
	else
		return "";
}

CStr CWRes_Dialogue::Ogr_GetSound_Hash(uint32 _iItem)
{
	uint16 i = GetHashPosition(_iItem);

	if( i < m_lDialogueIndexes.Len() )
		return m_lSoundNames[i];
	else
		return "";
}

CStr CWRes_Dialogue::Ogr_GetFirstSound()
{
	for(int i = m_lSoundNames.Len() - 1; i >= 0; i--)
		if(m_lSoundNames[i] != "")
			return m_lSoundNames[i];

	return "";
}

CStr CWRes_Dialogue::Ogr_Link(int _iItem, fp32& _Duration)
{
	fp32 Time = 0;
	const char *pBuf = FindEvent(_iItem, EVENTTYPE_LINK, &Time);
	if (!pBuf)
	{
		_Duration = GetSampleLength(_iItem);
		return "";
	}
	_Duration = Time;
	return pBuf;
}

CStr CWRes_Dialogue::Ogr_Link_Hash(uint32 _iItem, fp32& _Duration)
{
	fp32 Time = 0;
	const char *pBuf = FindEvent_Hash(_iItem, EVENTTYPE_LINK, &Time);
	if (!pBuf)
	{
		_Duration = GetSampleLength_Hash(_iItem);
		return "";
	}
	_Duration = Time;
	return pBuf;
}

void CWRes_Dialogue::Ogr_GetEvents(int _iItem, TArray<fp32> &_lDurations)
{
	const char *pBuf = GetDialogueItem(_iItem);
	if(!pBuf)
		return;

	pBuf += 2;

	uint16 tmp_u16;
	uint8 tmp_u8;

	int nEvents = SafeRead(pBuf, 0, tmp_u16);
	pBuf += 2;
	//	fp32 Time = 0;

	fp32 SampleLength = GetSampleLength(_iItem);

	for(int i = 0; i < nEvents; i++)
	{
		const char *pOrg = pBuf;
		int Size = SafeRead(pBuf, 0, tmp_u8); pBuf++;
		int t = SafeRead(pBuf, 0, tmp_u8); pBuf++;
		int Flags = t >> 5;
		int EventTime = int(fp32(SafeRead(pBuf, 0, tmp_u16)) * (1.0f / 256.0f));
		pBuf += 2;
		if (Flags & EVENTFLAGS_SAMPLELENGTHRELATIVE)
			EventTime = int(EventTime + SampleLength);

		_lDurations.Add(EventTime);
		pBuf = pOrg + (Size << 1);
	}
}

void CWRes_Dialogue::Ogr_GetEvents_Hash(uint32 _iItem, TArray<fp32> &_lDurations)
{
	const char *pBuf = GetHashDialogueItem(_iItem);
	if(!pBuf)
		return;

	pBuf += 2;

	uint16 tmp_u16;
	uint8 tmp_u8;

	int nEvents = SafeRead(pBuf, 0, tmp_u16);
	pBuf += 2;
	//	fp32 Time = 0;

	fp32 SampleLength = GetSampleLength_Hash(_iItem);

	for(int i = 0; i < nEvents; i++)
	{
		const char *pOrg = pBuf;
		int Size = SafeRead(pBuf, 0, tmp_u8); pBuf++;
		int t = SafeRead(pBuf, 0, tmp_u8); pBuf++;
		int Flags = t >> 5;
		fp32 EventTime = fp32(SafeRead(pBuf, 0, tmp_u16)) * (1.0f / 256.0f);
		pBuf += 2;
		if (Flags & EVENTFLAGS_SAMPLELENGTHRELATIVE)
			EventTime = int(EventTime + SampleLength);

		_lDurations.Add(EventTime);
		pBuf = pOrg + (Size << 1);
	}
}


void CWRes_Dialogue::Ogr_Recreate(CRegistry *_pReg, CMapData *_pMapData)
{
	spCRegistry spReg;
	if(m_spParent)
	{
		spReg = m_spParent->Duplicate();
		spReg->CopyDir(_pReg);
	}
	else
		spReg = _pReg;
	CreateFromRegistry(spReg, NULL, _pMapData);
}

bool CWRes_Dialogue::Ogr_GetDialogueItemHashAtPos(uint _Pos, uint32 &oHashValue)
{
	if (_Pos >= m_lDialogueIndexes.Len())
		return false;
	else
	{
		oHashValue = m_lDialogueIndexes[_Pos].m_Hash;
		return true;
	}
}


#endif
