#include "PCH.h"

#include "../MSystem.h"
#include "MSound.h"

// -------------------------------------------------------------------
//  CSC_SFXDesc
// -------------------------------------------------------------------
int16 CSC_SFXDesc::CNormalParams::GetRandomSound()
{
	if(m_lWaves.Len() > 1)
	{
		int16 Sound = Min((int32)(m_lWaves.Len()-1), (int32)(Random*(fp32)(m_lWaves.Len()))); 
		
		if(Sound == m_LastSound)
		{
			Sound++;
			Sound %= m_lWaves.Len();
		}

		m_LastSound = Sound;
		return Sound;
	}

	return 0;
}

//
void CSC_SFXDesc::CMaterialParams::CMaterialWaveHolder::AddWave(int16 _WaveID)
{
	// Increase size and add wave
	int32 Len = m_lWaves.Len();
	m_lWaves.SetLen(Len+1);
	m_lWaves[Len] = _WaveID;
}

//
//
//

//
//
//
CSC_SFXDesc::CMaterialParams::CMaterialWaveHolder *CSC_SFXDesc::CMaterialParams::GetMaterial(uint8 _ID, int &_HashPlace)
{
	int32 HashIndex = _ID%m_HolderHashSize;
	
	// Search the holder for the material
	for(int32 i = 0; i < m_alMaterialWaveHolders[HashIndex].Len(); i++)
		if(m_alMaterialWaveHolders[HashIndex][i]->m_iMaterial == _ID)
		{
			_HashPlace = i;
			return m_alMaterialWaveHolders[HashIndex][i];
		}

	return NULL;
}

//
//
//
CSC_SFXDesc::CMaterialParams::CMaterialWaveHolder *CSC_SFXDesc::CMaterialParams::CreateMaterial(uint8 _ID)
{
	int32 HashIndex = _ID%m_HolderHashSize;

	// Create material
	CMaterialWaveHolder *pMaterialWaveHolder = DNew(CMaterialWaveHolder) CMaterialWaveHolder;

	// Add to the list
	int32 Len = m_alMaterialWaveHolders[HashIndex].Len();
	m_alMaterialWaveHolders[HashIndex].SetLen(Len+1);
	m_alMaterialWaveHolders[HashIndex][Len] = pMaterialWaveHolder;

	// Set Material
	pMaterialWaveHolder->m_iMaterial = _ID;

	// Return the ew object
	return pMaterialWaveHolder;
}

//
//
//
CSC_SFXDesc::CMaterialParams::CMaterialWaveHolder *CSC_SFXDesc::CMaterialParams::GetOrCreateMaterial(uint8 _ID)
{
	if(!_ID)
		return NULL;

	int _HashPlace;
	CMaterialWaveHolder *pMaterialWaveHolder = GetMaterial(_ID, _HashPlace);
	
	if(!pMaterialWaveHolder)
		return CreateMaterial(_ID);

	return pMaterialWaveHolder;
}

//
//
//
void CSC_SFXDesc::CMaterialParams::AddWave(uint8 _MaterialID, int16 _WaveID)
{
	if(!_MaterialID || _WaveID < 0)
		return;

	GetOrCreateMaterial(_MaterialID)->AddWave(_WaveID);
}

//
// Just sets the mode to normal
//
CSC_SFXDesc::CSC_SFXDesc()
{
	// Init Mode
	m_Datas.m_Mode = ENONE;
	m_Datas.m_Category = 0;
	m_pParams = NULL;
	m_pWaveContainer = NULL;
	
}

//
// 
//
//
//
//
void CSC_SFXDesc::SetWaveContatiner(class CWaveContainer_Plain *_pWaveContainer)
{
	MAUTOSTRIP( CSC_SFXDesc_SetWaveContatiner, MAUTOSTRIP_VOID);
	m_pWaveContainer = _pWaveContainer;
}

//
//
//
int16 CSC_SFXDesc::GetWaveId(int16 _iLocal)
{
	MAUTOSTRIP( CSC_SFXDesc_GetWaveId, 0);

	if(GetMode() == ENORMAL)
	{
		if(GetNormalParams()->m_lWaves.Len() <= _iLocal)
			return -1;
		return m_pWaveContainer->GetWaveID(GetNormalParams()->m_lWaves[_iLocal]);
	}
	if(GetMode() == EMATERIAL)
	{
		uint8 HashID = (_iLocal>>6)&0x3;
		int8 Index = (_iLocal)&0x3F;
		int8 LocalID = _iLocal>>8;

		if(HashID >= CMaterialParams::m_HolderHashSize)
			return -1;

		if(Index >= GetMaterialParams()->m_alMaterialWaveHolders[HashID].Len())
			return -1;

		CMaterialParams::CMaterialWaveHolder *pHolder = GetMaterialParams()->m_alMaterialWaveHolders[HashID][Index];
		
		if(!pHolder)
			return -1;

		if(pHolder->m_lWaves.Len() <= LocalID)
			return -1;

		return m_pWaveContainer->GetWaveID(pHolder->m_lWaves[LocalID]);
	}

	return -1;
}

//
//
//
int16 CSC_SFXDesc::GetContainerWaveId(int16 _iLocal)
{
	MAUTOSTRIP(CSC_SFXDesc_GetContainerWaveId, 0);

	if(GetMode() == ENORMAL)
	{
		if(GetNormalParams()->m_lWaves.Len() <= _iLocal)
			return -1;
		return GetNormalParams()->m_lWaves[_iLocal];
	}
	if(GetMode() == EMATERIAL)
	{
		uint8 HashID = (_iLocal>>6)&0x3;
		int8 Index = (_iLocal)&0x3F;
		int8 LocalID = _iLocal>>8;

		if(HashID >= CMaterialParams::m_HolderHashSize)
			return -1;

		if(Index >= GetMaterialParams()->m_alMaterialWaveHolders[HashID].Len())
			return -1;

		CMaterialParams::CMaterialWaveHolder *pHolder = GetMaterialParams()->m_alMaterialWaveHolders[HashID][Index];
		
		if(!pHolder)
			return -1;

		if(pHolder->m_lWaves.Len() <= LocalID)
			return -1;

		return pHolder->m_lWaves[LocalID];
	}

	return -1;
}

//
//
//
int16 CSC_SFXDesc::GetNumWaves()
{
	MAUTOSTRIP( CSC_SFXDesc_GetNumWaves, 0);

	if(GetMode() == ENORMAL)
		return GetNormalParams()->m_lWaves.Len();
	if(GetMode() == EMATERIAL)
	{
		CMaterialParams *pParams = GetMaterialParams();
		int32 Count = 0;

		for(int32 h = 0; h < pParams->m_HolderHashSize; h++)
			for(int32 i = 0; i < pParams->m_alMaterialWaveHolders[h].Len(); i++)
				Count += pParams->m_alMaterialWaveHolders[h][i]->m_lWaves.Len();

		return Count;
	}

	return 0;
}

//
//
//
int32 CSC_SFXDesc::EnumWaves(int16 *_paWaves, int32 _MaxWaves)
{
	if(_MaxWaves <= 0)
		return 0;

	int32 Current = 0;

	if(GetMode() == ENORMAL)
	{
		CNormalParams *pParams = GetNormalParams();
		while(Current < pParams->m_lWaves.Len() && Current < _MaxWaves)
		{
			_paWaves[Current] = Current;
			++Current;
		}

		return Current;
	}

	if(GetMode() == EMATERIAL)
	{
		CMaterialParams *pParams = GetMaterialParams();
//		int32 Count = 0;

		for(int32 h = 0; h < CMaterialParams::m_HolderHashSize; h++)
			for(int32 i = 0; i < pParams->m_alMaterialWaveHolders[h].Len(); i++)
			{
				CMaterialParams::CMaterialWaveHolder *pHolder = pParams->m_alMaterialWaveHolders[h][i];

				for(int32 w = 0; w < pHolder->m_lWaves.Len(); w++)
				{
					// this should be a function
//					int16 iLocal = (pHolder->m_lWaves.Len() > 1) ? Min((int32)(pHolder->m_lWaves.Len()-1), (int32)(Random*(fp32)(pHolder->m_lWaves.Len()))) : 0;
					int16 Ret = (w<<8) | ((pHolder->m_iMaterial%4)<<6) | i;

					_paWaves[Current++] = Ret;

					if(Current == _MaxWaves)
						return Current;
				}
			}

		return Current;
	}

	return 0;
}

int16 CSC_SFXDesc::GetPlayWaveId(uint8 _iMaterial)
{
	if(GetMode() == ENORMAL)
	{
		CNormalParams *pParams = GetNormalParams();
		return pParams->GetRandomSound();
	}
	else if(GetMode() == EMATERIAL)
	{
		int HashPlace = 0; 
		CMaterialParams::CMaterialWaveHolder *pHolder = GetMaterialParams()->GetMaterial(_iMaterial, HashPlace);

		M_ASSERT(HashPlace >= 0 && HashPlace < 63, "Too many materials");
		
		// if we didn't found the holder for the material. try the base10 of the material id. it should contain the fallback sound
		if(!pHolder)
		{
			_iMaterial = _iMaterial / 10 * 10;
			pHolder = GetMaterialParams()->GetMaterial(_iMaterial, HashPlace);
		}

		if(!pHolder)
		{
			CMaterialParams *pParams = GetMaterialParams();
			if(!pParams)
				return -1;

			for(int i = 0; i < pParams->m_HolderHashSize; i++)
				if(pParams->m_alMaterialWaveHolders[i].Len())
					pHolder = pParams->m_alMaterialWaveHolders[i][0];
		}

		if(!pHolder)
			return -1;

		int16 iLocal = (pHolder->m_lWaves.Len() > 1) ? Min((int32)(pHolder->m_lWaves.Len()-1), (int32)(Random*(fp32)(pHolder->m_lWaves.Len()))) : 0;
		int16 Ret = (iLocal<<8) | ((_iMaterial%4)<<6);
		int16 Ret2 = HashPlace;
		Ret |= Ret2;
		return Ret;
	}
	return -1;
}

//
//
//
void CSC_SFXDesc::operator= (CSC_SFXDesc& _s)
{
	MAUTOSTRIP( CSC_SFXDesc_operator_equal, MAUTOSTRIP_VOID);

	// Set Mode
	SetMode(_s.GetMode());

	// Copy parameter options
	if(GetMode() == ENORMAL)
	{
		GetNormalParams()->m_lWaves.SetLen(_s.GetNormalParams()->m_lWaves.Len());

		for(int32 i = 0; i < GetNormalParams()->m_lWaves.Len(); i++)
			GetNormalParams()->m_lWaves[i] = _s.GetNormalParams()->m_lWaves[i];
	}
	else if (GetMode() == EMATERIAL)
	{
		// Get parameters
		CMaterialParams *pParams = _s.GetMaterialParams();
		CMaterialParams *pParamsDest = GetMaterialParams();

		// Copy materials
		for(int32 h = 0; h < CMaterialParams::m_HolderHashSize; h++)
		{
			pParamsDest->m_alMaterialWaveHolders[h].SetLen(pParams->m_alMaterialWaveHolders[h].Len());

			for(int32 m = 0; m < pParams->m_alMaterialWaveHolders[h].Len(); m++)
			{
				CMaterialParams::CMaterialWaveHolder *pHolder = pParams->m_alMaterialWaveHolders[h][m];
				CMaterialParams::CMaterialWaveHolder *pHolderDest = DNew(CMaterialParams::CMaterialWaveHolder) CMaterialParams::CMaterialWaveHolder;
				pParamsDest->m_alMaterialWaveHolders[h][m] = pHolderDest;

				pHolderDest->m_lWaves.SetLen(pHolder->m_lWaves.Len());
				pHolderDest->m_iMaterial = pHolder->m_iMaterial;

				// Write wave names for the material
				for(int32 w = 0; w < pHolder->m_lWaves.Len(); w++)
					pHolderDest->m_lWaves[w] = pHolder->m_lWaves[w];
			}
		}
	}
	else
		Error("operator=", "Can't copy SFXDesc in this mode");

	// Copy name
	#ifdef USE_HASHED_SFXDESC
		m_SoundIndex = _s.m_SoundIndex;
	#else
		m_SoundName = _s.m_SoundName;
	#endif

	SetWaveContatiner(m_pWaveContainer);

	// Copy data
	SetPriority(_s.GetPriority());
	SetPitch(_s.GetPitch());
	SetPitchRandAmp(_s.GetPitchRandAmp());
	SetVolume(_s.GetVolume());
	SetVolumeRandAmp(_s.GetVolumeRandAmp());
	SetAttnMaxDist(_s.GetAttnMaxDist());
	SetAttnMinDist(_s.GetAttnMinDist());
	SetCategory(_s.GetCategory());
	
}

//
//
//

//
// Load a descriptor from a file
//
void CSC_SFXDesc::Read(CCFile* _pFile, class CWaveContainer_Plain *_pWaveContainer)
{
	MAUTOSTRIP( CSC_SFXDesc_Read, MAUTOSTRIP_VOID);
	MSCOPE(CSC_SFXDesc::Read, IGNORE);

	SetWaveContatiner(_pWaveContainer);

	uint32 Ver = 0x0000;
	_pFile->ReadLE(Ver);

	if (Ver > CSC_SFXDESC_VERSION)
	{
		Error("CSC_SFXDesc", CStrF("Unsupported SFX-Description version (%.4x)", Ver));
	}
	else if (Ver >= 0x0103)
	{
		// Read Sound Name
		CStr TempStr;
		TempStr.Read(_pFile);

		#ifdef USE_HASHED_SFXDESC
			m_SoundIndex = StringToHash(TempStr);
		#else
			m_SoundName = TempStr;
		#endif
		
		// Read mode
		uint8 CurrentMode;
		_pFile->ReadLE(CurrentMode);

		if(CurrentMode != ENORMAL && CurrentMode != EMATERIAL)
			CurrentMode = CurrentMode;

		SetMode(CurrentMode);

		if(CurrentMode == ENORMAL)
		{
			// Read wave names
			int16 NumWaves;
			_pFile->ReadLE(NumWaves);
			GetNormalParams()->m_lWaves.SetLen(NumWaves);
			
			int32 Count = 0;
			for(int32 i = 0; i < NumWaves; i++)
			{
				CStr TempStr;
				TempStr.Read(_pFile);
				#ifdef USE_HASHED_WAVENAME
					int32 iLocalWave = _pWaveContainer->GetIndex(StringToHash(TempStr));
				#else
					int32 iLocalWave = _pWaveContainer->GetIndex(TempStr);
				#endif

				int32 iTemp = (iLocalWave >= 0) ? _pWaveContainer->GetWaveID(iLocalWave) : -1;
				
				if(iTemp < 0)
					ConOutL(CStr("§cf80WARNING: Sound references undefined waveform ."));
				else
					GetNormalParams()->m_lWaves[Count++] = iLocalWave;
			}

			// Optimize List size
			GetNormalParams()->m_lWaves.SetLen(Count);
		}
		else if(CurrentMode == EMATERIAL)
		{
			// Get parameters
			CMaterialParams *pParams = GetMaterialParams();

			// Calculate the total number of materials
			int16 NumMaterials;
			_pFile->ReadLE(NumMaterials);

			// Read materials
			for(int32 m = 0; m < NumMaterials; m++)
			{
				uint8 MaterialID;
				int16 NumWaves;
				_pFile->ReadLE(MaterialID);
				_pFile->ReadLE(NumWaves);

				CMaterialParams::CMaterialWaveHolder *pHolder = pParams->GetOrCreateMaterial(MaterialID);
				for(int32 w = 0; w < NumWaves; w++)
				{
					CStr TempStr;
					TempStr.Read(_pFile);

					#ifdef USE_HASHED_WAVENAME
						int32 iLocalWave = _pWaveContainer->GetIndex(StringToHash(TempStr));
					#else
						int32 iLocalWave = _pWaveContainer->GetIndex(TempStr);
					#endif

					int32 iTemp = (iLocalWave >= 0) ? _pWaveContainer->GetWaveID(iLocalWave) : -1;
					
					if(iTemp < 0)
						ConOutL(CStr("§cf80WARNING: Sound references undefined waveform."));
					else
						pHolder->AddWave(iLocalWave);
				}
			}
		}
		else
			Error("(CSC_SFXDesc::Read)", "Strange mode requested to sfxdesc, bailing.");



		if (Ver == CSC_SFXDESC_VERSION_VER3)
		{
			// Read Data
			uint16 Temp;
			_pFile->ReadLE(Temp);
			m_Datas.m_AttnMaxDist = Temp;
			_pFile->ReadLE(Temp);
			m_Datas.m_AttnMinDist = Temp;
			uint8 Temp8;
			_pFile->ReadLE(Temp8);
			m_Datas.m_Priority = Temp8;
			_pFile->ReadLE(m_Datas.m_Pitch);
			_pFile->ReadLE(m_Datas.m_PitchRandAmp);
			_pFile->ReadLE(m_Datas.m_Volume);
			_pFile->ReadLE(m_Datas.m_VolumeRandAmp);
		}
		else
		{
			uint32 Data1;
			_pFile->ReadLE(Data1);
			m_Datas.m_AttnMinDist = (Data1 >> 2) & ((1 << 12) - 1);
			m_Datas.m_AttnMaxDist = (Data1 >> 14) & ((1 << 12) - 1);
			m_Datas.m_Priority = (Data1 >> 26) & ((1 << 6) - 1);

			_pFile->ReadLE(m_Datas.m_Pitch);
			_pFile->ReadLE(m_Datas.m_PitchRandAmp);
			_pFile->ReadLE(m_Datas.m_Volume);
			_pFile->ReadLE(m_Datas.m_VolumeRandAmp);
		}

		if (Ver >= 0x0105)
		{
			if (Ver >= 0x0106)
				_pFile->ReadLE(m_Datas.m_Category);
			else
			{
				uint8 Category;
				_pFile->ReadLE(Category);
				m_Datas.m_Category = Category;				
			}
		}

	}
	else
	{
		switch(Ver)
		{
		case CSC_SFXDESC_VERSION_VER2:  // Old Version
			{
				// Set mode
				SetMode(ENORMAL);

				// Read Sound Name
				CStr TempStr;
				TempStr.Read(_pFile);

				#ifdef USE_HASHED_SFXDESC
					m_SoundIndex = StringToHash(TempStr);
				#else
					m_SoundName = TempStr;
				#endif

				// Read wave names
				int16 NumWaves;
				_pFile->ReadLE(NumWaves);
				GetNormalParams()->m_lWaves.SetLen(NumWaves);
				
				int32 Count = 0;
				for(int32 i = 0; i < NumWaves; i++)
				{
					int32 m_iLocalWave = _pWaveContainer->GetIndex(TempStr);
					int32 iTemp = (m_iLocalWave >= 0) ? _pWaveContainer->GetWaveID(m_iLocalWave) : -1;
					
					if(iTemp < 0)
						ConOutL(CStrF("§cf80WARNING: Sound references undefined waveform '%s'.", TempStr.Str()));
					else
						GetNormalParams()->m_lWaves[Count++] = m_iLocalWave;
				}

				// Optimize List size
				GetNormalParams()->m_lWaves.SetLen(Count);

				// Read Data
				int16 Crap16;
				int8 Crap8;
				uint16 Temp;
				_pFile->ReadLE(Temp);
				m_Datas.m_AttnMaxDist = Temp;

				_pFile->ReadLE(Temp);
				m_Datas.m_AttnMinDist = Temp;
				
				_pFile->ReadLE(Crap16);
				_pFile->ReadLE(Crap16);

				_pFile->ReadLE(Crap8);
				m_Datas.m_Priority = Crap8;
				_pFile->ReadLE(m_Datas.m_Pitch);
				_pFile->ReadLE(m_Datas.m_PitchRandAmp);
				_pFile->ReadLE(m_Datas.m_Volume);
				_pFile->ReadLE(m_Datas.m_VolumeRandAmp);

				_pFile->ReadLE(Crap8);
				_pFile->ReadLE(Crap8);
			}
			break;
		default:
			Error("CSC_SFXDesc", CStrF("Unsupported SFX-Description version (%.4x)", Ver));
		}
	}
}

//
// Writes the descriptor to a file
//
void CSC_SFXDesc::Write(CCFile* _pFile, class CWaveContainer_Plain *_pWaveContainer)
{
#ifdef USE_HASHED_SFXDESC
	Error("CSC_SFXDesc", "Can't save sound description that has a diffrent mode than normal or material");
	return;
#else

	MAUTOSTRIP( CSC_SFXDesc_Write, MAUTOSTRIP_VOID);

	if(GetMode() != ENORMAL && GetMode() != EMATERIAL)
	{
		Error("CSC_SFXDesc", "Can't save sound description that has a diffrent mode than normal or material");
		return;
	}

	// Write Version
	uint32 Ver = CSC_SFXDESC_VERSION;
	_pFile->WriteLE(Ver);

	// Write name of description
	m_SoundName.Write(_pFile);

	// Write mode
	uint8 CurrentMode = GetMode();
	_pFile->WriteLE(CurrentMode);

	if(CurrentMode == ENORMAL)
	{
		// Normal mode
		int16 NumWaves = (int16)GetNormalParams()->m_lWaves.Len();
		_pFile->WriteLE(NumWaves);
		
		for(int i = 0; i < GetNormalParams()->m_lWaves.Len(); i++)
		{
			/*
			#ifdef USE_HASHED_WAVENAME
				int16 id = GetNormalParams()->m_lWaves[i];
				if(id == -1)
					ConOutL(CStrF("§cf80WARNING: Waveform with no good id.", id));

				uint32 nameid = _pWaveContainer->GetNameID(id);
				_pFile->WriteLE(nameid);
				
			#else
			*/
			CStr TempStr;
			TempStr = _pWaveContainer->GetName(GetNormalParams()->m_lWaves[i]);
			TempStr.Write(_pFile);
			//#endif
		}
	}
	else if(CurrentMode == EMATERIAL)
	{
		// Get parameters
		CMaterialParams *pParams = GetMaterialParams();

		// Calculate the total number of materials
		int16 NumMaterials = 0;
		for(int32 i = 0; i < CMaterialParams::m_HolderHashSize; i++)
			NumMaterials += pParams->m_alMaterialWaveHolders[i].Len();

		_pFile->WriteLE(NumMaterials);

		// Write materials
		for(int32 h = 0; h < CMaterialParams::m_HolderHashSize; h++)
		{
			for(int32 m = 0; m < pParams->m_alMaterialWaveHolders[h].Len(); m++)
			{
				CMaterialParams::CMaterialWaveHolder *pHolder = pParams->m_alMaterialWaveHolders[h][m];

				// Write material ID and num waves for that material
				_pFile->WriteLE((uint8)pHolder->m_iMaterial);
				_pFile->WriteLE((int16)pHolder->m_lWaves.Len());
				
				// Write wave names for the material
				for(int32 w = 0; w < pHolder->m_lWaves.Len(); w++)
				{
					/*
					#ifdef USE_HASHED_WAVENAME
						//int16 id = m_pWaveContainer->GetWaveID(GetNormalParams()->m_lWaves[i]);
						uint32 nameid = _pWaveContainer->GetNameID(pHolder->m_lWaves[w]);
						_pFile->WriteLE(nameid);

						//_pFile->WriteLE(_pWaveContainer->GetNameID(m_pWaveContainer->GetWaveID(GetNormalParams()->m_lWaves[i])));
					#else
					*/
					CStr TempStr;
					TempStr = _pWaveContainer->GetName(pHolder->m_lWaves[w]);
					TempStr.Write(_pFile);
                    //#endif
				}
			}
		}
	}
	uint32 Data1 = m_Datas.m_Mode | m_Datas.m_AttnMinDist<<2 | m_Datas.m_AttnMaxDist<<14 | m_Datas.m_Priority << 26;
	_pFile->WriteLE(Data1);
	_pFile->WriteLE(m_Datas.m_Pitch);
	_pFile->WriteLE(m_Datas.m_PitchRandAmp);
	_pFile->WriteLE(m_Datas.m_Volume);
	_pFile->WriteLE(m_Datas.m_VolumeRandAmp);
	_pFile->WriteLE(m_Datas.m_Category);
#endif
}
