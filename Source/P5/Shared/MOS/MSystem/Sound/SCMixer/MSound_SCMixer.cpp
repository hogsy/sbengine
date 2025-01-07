
#include "PCH.h"
#include "MSound_SCMixer.h"
#include "../MSound_Mixer_DSPEffects.h"
/////////////////////////////////////////////////////////
// Initialization / Destruction

CSoundContext_Mixer::CSoundContext_Mixer()
{
	m_iWC = NULL;
	MACRO_GetRegisterObject(CWaveContext, pWC, "SYSTEM.WAVECONTEXT");
	if (!pWC)
		ThrowError("No wave-context.");

	m_LastReadCategories = 0;
	m_ReloadSequence = 0;
	m_bAutoReloadCategories = false;
	m_bAddedToConsole = 0;
	m_MusicVolume_TotalPrim = 0.0f;
	m_iCurrentVoiceHandle = 0;
	m_DebugStrTempMixer = NULL;
	m_nFreeVoices = 0;
	m_spWaveContext = pWC;
	m_spWCIDInfo = MNew1(CSC_WCIDInfo, m_spWaveContext->GetIDCapacity());
	m_iWC = m_spWaveContext->AddSoundContext( this );

	m_MusicVolume_Total = 1.0f;
	m_MusicVolume_TotalLast = 1.0f;
	m_MusicVolume_System = 1.0f;
	m_MusicVolume_Settings = 1.0f;
	m_MusicVolume_Game = 1.0f;


	m_Listener_Matrix.Unit();
	m_Listener_Velocity = 0.0f;
	m_CoordinateScale = 1.0f;
#ifdef PLATFORM_CONSOLE
	m_LFEScale = 0.0;
#else
	m_LFEScale = 1.0;
#endif

	MACRO_GetSystem;
#ifndef PLATFORM_CONSOLE
	m_bLogUsage = pSys->GetEnvironment()->GetValuei("RESOURCES_LOG") != 0;
#endif

}

CSoundContext_Mixer::~CSoundContext_Mixer()
{
	if (m_bAddedToConsole)
		RemoveFromConsole();

	if( m_iWC != -1 )
		m_spWaveContext->RemoveSoundContext( m_iWC );
	m_spWaveContext = NULL;

	Internal_DestroyCategories();

	CDSPChainTreeIter Iter = m_DSPChainTreeGlobal;
	while (Iter)
	{
		Iter->Destroy(this);
		++Iter;
	}
	Iter = m_DSPChainTreeVoice;
	while (Iter)
	{
		Iter->Destroy(this);
		++Iter;
	}
	m_DSPChainTreeGlobal.f_DeleteAll();
	m_DSPChainTreeVoice.f_DeleteAll();
	m_DSPTree.f_DeleteAll();

	if (m_DebugStrTempMixer)
	{
		for (int i = 0; i < mcs_MaxDebugSounds - 1; ++i)
			delete [] m_DebugStrTempMixer[i];

		delete [] m_DebugStrTempMixer;
	}

}

CSoundContext_Mixer::CSC_DSPChain *CSoundContext_Mixer::Internal_GetGlobalChainByName(CStr _Name)
{
	return Internal_GetGlobalChainByHash(_Name.StrHash());
}

CSoundContext_Mixer::CSC_DSPChain *CSoundContext_Mixer::Internal_GetGlobalChainByHash(uint32 _Name)
{
	return m_DSPChainTreeGlobal.FindEqual(_Name);
}

CSoundContext_Mixer::CSC_DSPChain *CSoundContext_Mixer::Internal_GetVoiceChainByName(CStr _Name)
{
	return Internal_GetVoiceChainByHash(_Name.StrHash());
}

CSoundContext_Mixer::CSC_DSPChain *CSoundContext_Mixer::Internal_GetVoiceChainByHash(uint32 _Name)
{
	return m_DSPChainTreeVoice.FindEqual(_Name);
}

bint CSoundContext_Mixer::Destroy_Everything()
{
	CChannelIter ChnIter = m_Channels;
	while (ChnIter)
	{
		CChannel *pChannel = ChnIter;

		CVoiceIter VoiceIter = pChannel->m_AlloctedVoices;
		while (VoiceIter)
		{
			CVoice *pVoice = VoiceIter;
			++VoiceIter;
			Internal_VoiceDestroy(pVoice);
		}

		++ChnIter;
	}

	for (mint i = 0; i < m_lChannels.Len(); ++i)
	{
		if (m_lChannels[i])
			delete m_lChannels[i];
	}
	m_lChannels.Clear();

	return true;
}


void CSoundContext_Mixer::Create(CStr _Param)
{
	if (!m_bAddedToConsole)
		AddToConsole();
}

void CSoundContext_Mixer::Init(int _MaxVoices)
{
	m_lVoices.SetLen(_MaxVoices);
	Platform_GetInfo(m_PlatformInfo);
	m_LastSetSampleRate = m_PlatformInfo.m_SampleRate;
	m_LastSetSampleRateTraget = m_PlatformInfo.m_SampleRate;
	uint32 nMixerVoices = _MaxVoices + 32;
	m_Mixer.Create(_MaxVoices + 32, m_PlatformInfo.m_nChannels, m_PlatformInfo.m_FrameLength, m_PlatformInfo.m_SampleRate, m_PlatformInfo.m_nProssingThreads); // 32 extra for delayed deletes
	Internal_InitDSP();
	Internal_InitCategories();
	Platform_Init(nMixerVoices);

	for (int i = 0; i < m_PlatformInfo.m_nChannels; ++i)
	{
		m_SpeakerAngles[i] = M_ATan2(m_PlatformInfo.m_Speakers[i].m_Position.k[1], m_PlatformInfo.m_Speakers[i].m_Position.k[0]) * (0.5f/_PI);
	}

	for (int i = 0; i < m_lVoices.Len(); ++i)
	{
		m_FreeVoices.Insert(m_lVoices[i]);
		++m_nFreeVoices;
	}

	m_DualStream.Create(this);


}


int32 CSoundContext_Mixer::Internal_TranslateDSPValue_int32(const CStr &_Value)
{
	if (_Value.Find("Global.") == 0)
	{
		// Global values
		if (_Value == "Global.NumChannels")
			return m_PlatformInfo.m_nChannels;
	}
	return _Value.Val_int();
}

fp32 CSoundContext_Mixer::Internal_TranslateDSPValue_fp32(const CStr &_Value)
{
	return _Value.Val_fp64();
}

//CSoundContext_Mixer::CSC_DSPChainInstance *
CSoundContext_Mixer::CSC_DSPChainInstance *CSoundContext_Mixer::Internal_TranslateDSPValue_Route(const CStr &_Value)
{
	CSC_DSPChain *pRoute = Internal_GetGlobalChainByName(_Value);
	if (!pRoute || !pRoute->m_pGlobalChain)
		ThrowError(CStrF("Could not find route named: %s", _Value.Str()));
	return pRoute->m_pGlobalChain;
}
void CSoundContext_Mixer::Internal_TranslateDSPValue_VolumeMatrix(const CRegistry &_Value, TThinArray< TThinArray< TThinArrayAlign<uint16, sizeof(vec128)> > >&_Volumes)
{
	// Find max number of source voices

	uint32 nChildren = _Value.GetNumChildren();
	uint32 MaxSource = 0;
	uint32 MaxDest = 0;

	for (uint32 i = 0; i < nChildren; ++i)
	{
		CStr Name = _Value.GetChild(i)->GetThisName();
		CStr Type = Name.GetStrSep("_");
		uint32 Source = Name.GetStrSep("_").Val_int();
		uint32 Dest = Name.GetStrSep("_").Val_int();

		MaxSource = Max(MaxSource, Source);
		MaxDest = Max(MaxDest, Dest);
	}

	_Volumes.SetLen(MaxSource+1);

	for (uint32 i = 0; i < nChildren; ++i)
	{
		const CRegistry *pReg = _Value.GetChild(i);
		CStr Name = pReg->GetThisName();
		CStr Type = Name.GetStrSep("_");
		uint32 Source = Name.GetStrSep("_").Val_int();
		uint32 Dest = Name.GetStrSep("_").Val_int();

		TThinArray< TThinArrayAlign<uint16, sizeof(vec128)> > &SourceArray = _Volumes[Source];

		if (SourceArray.Len() == 0)
			SourceArray.SetLen(MaxDest+1);

		TThinArrayAlign<uint16, sizeof(vec128)> &Array = SourceArray[Dest];
		if (Array.Len())
			ThrowError("Duplicate entry");

		uint32 AlignSource = AlignUp(Source, 4);
		uint32 AlignDest = AlignUp(Dest, 4);
		Array.SetLen(AlignSource*AlignDest);

		const char *pParse = pReg->GetThisValue().Str();

		uint32 iArray = 0;
		uint32 iRow = 0;
		uint32 ArraySize = AlignSource*AlignDest;

		while (*pParse && iRow + iArray < ArraySize)
		{
			NScript::ParseWhiteSpace(pParse);
			if (*pParse == ',')
				++pParse;
			NScript::ParseWhiteSpace(pParse);

			bint bDirect = false;
			if (*pParse == 'D' || *pParse == 'd')
			{
				bDirect = true;
				++pParse;
			}

			fp32 Value = NStr::StrToFloatParse(pParse, 0.0f, ",\r\n ");
			if (!bDirect)
				Value = M_Sqrt(Value); // Make correct
			uint16 ValueInt = Clamp(TruncToInt(Value * 65535.0), 0, 65535);
			Array[iRow + iArray++] = ValueInt;
			if (iArray == Source)
			{
				for (int i = Source; i < AlignSource; ++i)
					Array[iRow + i] = 0;
				iRow += AlignSource;
				iArray = 0;
			}
		}
		for (uint32 i = iRow + iArray; i < ArraySize; ++i)
			Array[i] = 0;
	}
	
}


void CSoundContext_Mixer::Internal_DestroyCategories()
{
	DLock(m_SoundCategoriesLock);
	CSoundCategory *pCategory = m_SoundCategories.GetRoot();
	while (pCategory)
	{
		m_SoundCategories.f_Remove(pCategory);
		m_SoundCategoriesName.f_Remove(pCategory);
		m_Pool_SoundCategory.Delete(pCategory);
		pCategory = m_SoundCategories.GetRoot();
	}
}

const char* g_pSoundCategorySpatialization[] = { "none", "3D", "widesound", NULL};

class CFalloffSort
{
public:
	static int Compare(const void *_pContext, CSC_VoiceFalloff::CEntry &First, CSC_VoiceFalloff::CEntry &Second)
	{
		if (First.m_Position > Second.m_Position)
			return 1;
		else if (First.m_Position < Second.m_Position)
			return -1;
		return 0;
	}
};

void CSoundContext_Mixer::Internal_InitCategories()
{
	DLock(m_SoundCategoriesLock);

	MACRO_GetSystem;
	CRegistry_Dynamic Reg;
	CStr FileName = pSys->m_ExePath + "System/Sound/SoundCategories.xrg";
	if (!CDiskUtil::FileExists(FileName))
		return;

	CFileInfo FileInfo = CDiskUtil::FileTimeGet(FileName);
	if (m_LastReadCategories != FileInfo.m_TimeWrite)
	{
		m_LastReadCategories = FileInfo.m_TimeWrite;
		int iReloadSequnce = m_ReloadSequence;
		++m_ReloadSequence;

		Reg.XRG_Read(pSys->m_ExePath + "System/Sound/SoundCategories.xrg");

		// DSPs
		{
			CRegistry *pCategories = Reg.FindChild("SoundCategories");
			if (pCategories)
			{
				uint32 nChildren = pCategories->GetNumChildren();

				for (uint32 i = 0; i < nChildren; ++i)
				{
					CRegistry *pCategory = pCategories->GetChild(i);

					uint32 CategoryID = pCategory->GetThisName().Val_int();
					CStr Name =  pCategory->GetValue("Name");
					CSoundCategory *pNew = m_SoundCategories.FindEqual(CategoryID);
					if (pNew)
					{
						if (pNew->m_ReloadSequence != iReloadSequnce)
							ThrowError(CFStrF("Sound category with id %d already exists", CategoryID));
						m_SoundCategories.f_Remove(pNew);
						m_SoundCategoriesName.f_Remove(pNew);
					}
					pNew = m_SoundCategoriesName.FindEqual(Name);
					if (pNew)
					{
						m_Pool_SoundCategory.Delete(pNew);
						ThrowError(CFStrF("Sound category with name %s already exists", Name.Str()));
					}

					if (!pNew)
						pNew = m_Pool_SoundCategory.New();

					pNew->Clear();
					pNew->m_Name = Name;
					pNew->m_ReloadSequence = m_ReloadSequence;
					pNew->m_Params.m_Category = CategoryID;
					m_SoundCategories.f_Insert(pNew);
					m_SoundCategoriesName.f_Insert(pNew);

					uint32 Override = 0;
					if (pCategory->FindChild("Parent"))
					{
						CStr Parent = pCategory->GetValue("Parent");
						CSoundCategory *pParent = m_SoundCategoriesName.FindEqual(Parent);
						if (pParent)
						{
							pNew->m_DirectFalloff = pParent->m_DirectFalloff;
							pNew->m_ReverbFalloff = pParent->m_ReverbFalloff;
							Override = pParent->m_WhatToOverride;

							pNew->m_Params.m_Priority = pParent->m_Params.m_Priority;
							pNew->m_Params.m_DSPId = pParent->m_Params.m_DSPId;
							pNew->m_Params.m_Pitch = pParent->m_Params.m_Pitch;
							pNew->m_Params.m_Volume = pParent->m_Params.m_Volume;
							pNew->m_Params.m_MinDelay = pParent->m_Params.m_MinDelay;
							pNew->m_Params.m_Spatialization = pParent->m_Params.m_Spatialization;
							pNew->m_Params.m_bDoppler = pParent->m_Params.m_bDoppler;
							pNew->m_Params.m_bFalloff = pParent->m_Params.m_bFalloff;
							pNew->m_Params.m_bLoop = pParent->m_Params.m_bLoop;
							pNew->m_Params.m_bIsMusic = pParent->m_Params.m_bIsMusic;
							pNew->m_Params.m_3DProperties = pParent->m_Params.m_3DProperties;
						}
						else
							ThrowError(CFStrF("In sound category %s: Undefined sound category with name %s specified as parent", Name.Str(), Parent.Str()));
					}
					if (pCategory->FindChild("Priority"))
					{
						pNew->m_Params.m_Priority = pCategory->GetValuei("Priority");
						Override |= ECategoryOverride_Priority;
					}

					if (pCategory->FindChild("DSPChain"))
					{
						CStr DSPChain = pCategory->GetValue("DSPChain");
						CSC_DSPChain *pChain = Internal_GetVoiceChainByName(DSPChain);
						if (pChain)
						{
							pNew->m_Params.m_DSPId = pChain->m_NameHash;
							Override |= ECategoryOverride_DSPID;
						}
						else
							ThrowError(CFStrF("In sound category %s: Undefined dsp chain %s", Name.Str(), DSPChain.Str()));
					}
					if (pCategory->FindChild("Pitch"))
					{
						pNew->m_Params.m_Pitch = pCategory->GetValuef("Pitch");
						Override |= ECategoryOverride_Pitch;
					}
					if (pCategory->FindChild("Volume"))
					{
						pNew->m_Params.m_Volume = pCategory->GetValuef("Volume");
						Override |= ECategoryOverride_Volume;
					}
					if (pCategory->FindChild("MinDelay"))
					{
						pNew->m_Params.m_MinDelay = pCategory->GetValuef("MinDelay");
						Override |= ECategoryOverride_MinDelay;
					}

					if (pCategory->FindChild("Spatialization"))
					{
						CStr StrSpat = pCategory->GetValue("Spatialization");
						int iSpat = StrSpat.TranslateInt(g_pSoundCategorySpatialization);
						if (iSpat >= 0)
						{
							pNew->m_Params.m_Spatialization = iSpat;
							Override |= ECategoryOverride_Spatialization;
						}
						else
						{
							ThrowError(CFStrF("In sound category %s: Unknown spatialization %s", Name.Str(), StrSpat.Str()));
						}
					}

					if (pCategory->FindChild("Doppler"))
					{
						pNew->m_Params.m_bDoppler = pCategory->GetValuei("Doppler") != 0;
						Override |= ECategoryOverride_Doppler;
					}

					if (pCategory->FindChild("Falloff"))
					{
						pNew->m_Params.m_bFalloff = pCategory->GetValuei("Falloff") != 0;
						Override |= ECategoryOverride_Falloff;
					}

					if (pCategory->FindChild("Loop"))
					{
						pNew->m_Params.m_bLoop = pCategory->GetValuei("Loop") != 0;
						Override |= ECategoryOverride_Loop;
					}

					if (pCategory->FindChild("IsMusic"))
					{
						pNew->m_Params.m_bIsMusic = pCategory->GetValuei("IsMusic") != 0;
						Override |= ECategoryOverride_IsMusic;
					}

					if (pCategory->FindChild("MinDist"))
					{
						pNew->m_Params.m_3DProperties.m_MinDist = pCategory->GetValuef("MinDist");
						Override |= ECategoryOverride_MinDist;
					}

					if (pCategory->FindChild("MaxDist"))
					{
						pNew->m_Params.m_3DProperties.m_MaxDist = pCategory->GetValuef("MaxDist");
						Override |= ECategoryOverride_MaxDist;
					}

					CRegistry *pFalloff;
					if ((pFalloff = pCategory->FindChild("DirectFalloff")))
					{
						TArray<CSC_VoiceFalloff::CEntry> TempArray;
						uint32 nFalloff = pFalloff->GetNumChildren();

						if (nFalloff < 2)
						{
							ThrowError(CFStrF("In sound category %s: Direct falloff curve must have at least 2 entries", Name.Str()));
						}
						TempArray.SetLen(nFalloff);
						for (mint j = 0; j < nFalloff; ++j)
						{
							CRegistry *pFallVal = pFalloff->GetChild(j);
							TempArray[j].m_Position = pFallVal->GetThisName().Val_fp64();
							TempArray[j].m_Volume = pFallVal->GetThisValuef();
						}
						TempArray.QSort<CFalloffSort>();

						if (TempArray[0].m_Position != 0.0f)
						{
							ThrowError(CFStrF("In sound category %s: Direct falloff curve must start at position 0.0", Name.Str()));
						}
						if (TempArray[nFalloff-1].m_Position != 1.0f)
						{
							ThrowError(CFStrF("In sound category %s: Direct falloff curve must end at position 1.0", Name.Str()));
						}

						pNew->m_DirectFalloff.m_Values.SetLen(nFalloff);
						for (mint j = 0; j < nFalloff; ++j)
						{
							pNew->m_DirectFalloff.m_Values[j] = TempArray[j];
						}
						Override |= ECategoryOverride_DirectFalloff;
					}
					if ((pFalloff = pCategory->FindChild("ReverbFalloff")))
					{
						TArray<CSC_VoiceFalloff::CEntry> TempArray;
						uint32 nFalloff = pFalloff->GetNumChildren();
						if (nFalloff < 2)
						{
							ThrowError(CFStrF("In sound category %s: Reverb falloff curve must have at least 2 entries", Name.Str()));
						}
						TempArray.SetLen(nFalloff);
						for (mint j = 0; j < nFalloff; ++j)
						{
							CRegistry *pFallVal = pFalloff->GetChild(j);
							TempArray[j].m_Position = pFallVal->GetThisName().Val_fp64();
							TempArray[j].m_Volume = pFallVal->GetThisValuef();
						}
						TempArray.QSort<CFalloffSort>();

						if (TempArray[0].m_Position != 0.0f)
						{
							ThrowError(CFStrF("In sound category %s: Reverb falloff curve must start at position 0.0", Name.Str()));
						}
						if (TempArray[nFalloff-1].m_Position != 1.0f)
						{
							ThrowError(CFStrF("In sound category %s: Reverb falloff curve must end at position 1.0", Name.Str()));
						}

						pNew->m_ReverbFalloff.m_Values.SetLen(nFalloff);

						for (mint j = 0; j < nFalloff; ++j)
						{
							pNew->m_ReverbFalloff.m_Values[j] = TempArray[j];
						}
						Override |= ECategoryOverride_ReverbFalloff;
					}
					pNew->m_WhatToOverride = Override;
				}
			}
		}

		DLinkDS_List(CSoundCategory, m_LinkTemp) ToRemove;
		DIdsTreeAVLAligned_Iterator(CSoundCategory, m_TreeLink, uint32, CSoundCategory::CCompare) Iter = m_SoundCategories;
		while (Iter)
		{
			if (Iter->m_ReloadSequence != m_ReloadSequence)
			{
				ToRemove.Insert(*Iter);
			}
			++Iter;
		}

		CSoundCategory *pToRemove = ToRemove.Pop();
		while (pToRemove)
		{
			m_SoundCategoriesName.f_Remove(pToRemove);
			m_SoundCategories.f_Remove(pToRemove);
			m_Pool_SoundCategory.Delete(pToRemove);
			pToRemove = ToRemove.Pop();
		}

	}
}

const char* g_pDSPTypes[] = { "output", "input", "inputoutput", "inplace" , NULL};
const char* g_pDSPParamTypes[] = { "mint", "vpuptr", "int32", "fp32", "route", "volumematrix", NULL };
const char* g_pDSPParamInterpolateTypes[] = { "linear", "exponent", "log", NULL };

const char* g_pDSPGlobalBind[] = { "numchannels", NULL };
const char* g_pDSPVoiceBind[] = { "numchannels", "3Dvolumematrix", "obstruction", "occlusion", NULL };

void CSoundContext_Mixer::Internal_InitDSP()
{
	m_DSPTree.f_DeleteAll();

	MACRO_GetSystem;
	CRegistry_Dynamic Reg;
	Reg.XRG_Read(pSys->m_ExePath + "System/Sound/DSPConfig2.xrg");

	// DSPs
	{
		CRegistry *pDSPs = Reg.FindChild("DSPs");
		if (pDSPs)
		{
			uint32 nChildren = pDSPs->GetNumChildren();

			for (uint32 i = 0; i < nChildren; ++i)
			{
				CRegistry *pDSPReg = pDSPs->GetChild(i);

				CStr Name = pDSPReg->GetValue("Name");
				if (Name == "")
					ThrowError("Name missing for DSP");

				if (m_DSPTree.FindEqual(Name))
					ThrowError("A DSP with the same name already exists");

				CSC_DSP *pDSP = DNew(CSC_DSP) CSC_DSP;
				pDSP->m_Name = Name;
				pDSP->m_DSPID = pDSPReg->GetValuei("DSPId");			
				CStr DSPType = pDSPReg->GetValue("Type", "output");
				int32 iEntry = DSPType.TranslateInt(g_pDSPTypes);
				if (iEntry < 0)
					ThrowError(CStrF("Unknown DSP type: %s", DSPType.Str()));
				pDSP->m_Type = iEntry;
				pDSP->m_ParamSize = CSC_Mixer_WorkerContext::ms_DSP_NumParams[pDSP->m_DSPID];

				m_DSPTree.f_Insert(pDSP);

				uint32 iParamByte = 0;
				CRegistry *pParamsReg = pDSPReg->FindChild("Params");
				if (pParamsReg)
				{
					uint32 nParams = pParamsReg->GetNumChildren();
					for (uint32 j = 0; j < nParams; ++j)
					{
						CRegistry *pParamReg = pParamsReg->GetChild(j);

						CStr Name = pParamReg->GetThisValue();
						if (pDSP->m_ParamTree.FindEqual(Name))
							ThrowError("A param for this DSP already exists with the same name.");

						CSC_DSP::CParam *pParam = DNew(CSC_DSP::CParam) CSC_DSP::CParam;
						pParam->m_Name = Name;
						CStr DSPValueType = pParamReg->GetValue("Type", "fp32");
						int32 iEntry = DSPValueType.TranslateInt(g_pDSPParamTypes);
						if (iEntry < 0)
							ThrowError(CStrF("Unknown DSP param type: %s", DSPType.Str()));
						pParam->m_Type = iEntry;
						pParam->m_Index = iParamByte;

						CStr DSPInterpolateType = pParamReg->GetValue("InterpolateType", "linear");
						iEntry = DSPInterpolateType.TranslateInt(g_pDSPParamInterpolateTypes);
						if (iEntry < 0)
							ThrowError(CStrF("Unknown DSP param interpolation type: %s", DSPInterpolateType.Str()));
						pParam->m_InterpolationType = iEntry;

						switch (pParam->m_Type)
						{
						case EDSPParamType_mint:
							{
								iParamByte += sizeof(mint);
							}
							break;
						case EDSPParamType_VPUPtr:
							{
								iParamByte += sizeof(CSC_Mixer_WorkerContext::CCustomPtrHolder);
							}
							break;
						case EDSPParamType_int32:
							{
								iParamByte += sizeof(int32);
								pParam->m_Min_int32 = pParamReg->GetValuei("Min", 0);
								pParam->m_Max_int32 = pParamReg->GetValuei("Max", 65536);
								pParam->m_Default_int32 = pParamReg->GetValuei("Default", 0);
							}
							break;
						case EDSPParamType_fp32:
							{
								iParamByte += sizeof(fp32);
								pParam->m_Min_fp32 = pParamReg->GetValuef("Min", 0.0);
								pParam->m_Max_fp32 = pParamReg->GetValuef("Max", 0.0);
								pParam->m_Default_fp32 = pParamReg->GetValuef("Default", 0.0);
							}
							break;
						case EDSPParamType_Route:
							{
								iParamByte += sizeof(CSC_Mixer_WorkerContext::CCustomPtrHolder);
							}
							break;
						case EDSPParamType_VolumeMatrix:
							{
								iParamByte += sizeof(CSC_Mixer_WorkerContext::CCustomPtrHolder);
							}
							break;
						default:
							ThrowError("Unknown param type.");
							break;

						}

						pDSP->m_ParamTree.f_Insert(pParam);
					}
				}
			}
		}
	}

	// Chains

	for (int i = 0; i < 5; ++i)
	{
		// 0 = Global
		// 1 = Voice
		// 2 = Create Global ChainInstances
		// 3 = Global Params
		// 4 = Voice Params

		if (i == 2)
		{
			CDSPChainTreeIter Iter = m_DSPChainTreeGlobal;
			while (Iter)
			{
				Iter->m_pGlobalChain = DNew(CSC_DSPChainInstance) CSC_DSPChainInstance;
				Iter->m_pGlobalChain->m_pDSPChain = Iter;
				CSC_Mixer_DSPChainInstance *pInstance = m_Mixer.DSPChain_CreateIntstance(Iter->m_pMixerDSPChain);
				Iter->m_pGlobalChain->m_pMixerDSPChainInstance = pInstance;
				Iter->m_ChainInstances.Insert(Iter->m_pGlobalChain);
				if (!Iter->m_pGlobalChain->m_pMixerDSPChainInstance)
					ThrowError("Failed to cerate mixer chain instance");

				// If the first element in the chain doesn't take an input we need to run this chain
				if (Iter->m_DSPs[0].m_pDSP->m_Type == EDSPType_Output)
				{
					m_Mixer.DSPChainInstance_AddProcess(Iter->m_pGlobalChain->m_pMixerDSPChainInstance);
				}

				++Iter;
			}

		}
		else
		{

			CRegistry *pChains;
			CDSPChainTree *pChainTree;
			
			if (i == 0 || i == 3)
			{
				pChains = Reg.FindChild("GlobalChains");
				pChainTree = &m_DSPChainTreeGlobal;
			}
			else
			{
				pChains = Reg.FindChild("VoiceChains");
				pChainTree = &m_DSPChainTreeVoice;
			}

			if (pChains)
			{
				uint32 nChains = pChains->GetNumChildren();

				for (uint32 l = 0; l < nChains; ++l)
				{
					CRegistry *pChainReg = pChains->GetChild(l);

					CStr Name = pChainReg->GetValue("Name");
					CSC_DSPChain *pChain;
					if (i < 2)
					{
						if (Name == "")
							ThrowError("Name missing for DSP Chain");

						if (pChainTree->FindEqual(Name.StrHash()))
							ThrowError("A DSP Chain with the same name already exists");

						pChain = DNew(CSC_DSPChain) CSC_DSPChain;
						pChain->m_Name = Name;
						pChain->m_NameHash = Name.StrHash();
						pChainTree->f_Insert(pChain);
						pChain->m_pMixerDSPChain = m_Mixer.DSPChain_Create();

						if (!pChain->m_pMixerDSPChain)
							ThrowError("Failed to create DSPChain in mixer");
					}
					else
					{
						pChain = pChainTree->FindEqual(Name.StrHash());
					}

					CRegistry *pDPSsReg = pChainReg->FindChild("DSPs");
					if (pDPSsReg)
					{
						if (i < 2)
						{
							uint32 nDSPs = pDPSsReg->GetNumChildren();
							pChain->m_DSPs.SetLen(nDSPs);

							// Create DSPs
							for (uint32 j = 0; j < nDSPs; ++j)
							{
								CRegistry *pDSPReg = pDPSsReg->GetChild(j);
								CStr Name = pDSPReg->GetThisValue();
								CSC_DSP *pDSP = Internal_GetDSP(Name);
								if (!pDSP)
									ThrowError(CStrF("Failed to find specified DSP: %s", Name.Str()));

								uint32 iDSP;
								pChain->m_DSPs[j].m_pMixerDSP = m_Mixer.DSP_Create(pChain->m_pMixerDSPChain, pDSP->m_DSPID, iDSP);
								pChain->m_DSPs[j].m_pDSP = pDSP;
								if (!pChain->m_DSPs[j].m_pMixerDSP)
									ThrowError("Failed to create DSP in mixer");

								if (iDSP != j)
									ThrowError("Internal Error 0");
							}

							// Add routes
							for (uint32 j = 0; j < nDSPs; ++j)
							{
								CRegistry *pDSPReg = pDPSsReg->GetChild(j);
								CRegistry *pRoutesReg = pDSPReg->FindChild("RouteTo");
								if (pRoutesReg)
								{
									uint32 nRoutes = pRoutesReg->GetNumChildren();
									for (int k = 0; k < nRoutes; ++k)
									{
										int32 iRoute = pRoutesReg->GetValuei(k);
										if (iRoute < 0 || iRoute >= nDSPs)
											ThrowError("Route ID out of range");

										m_Mixer.DSP_AddRouteTo(pChain->m_DSPs[j].m_pMixerDSP, pChain->m_DSPs[iRoute].m_pMixerDSP);
									}
								}
							}
						}
						else
						{
							uint32 nDSPs = pDPSsReg->GetNumChildren();
							// Get Values
							for (uint32 j = 0; j < nDSPs; ++j)
							{
								CRegistry *pDSPReg = pDPSsReg->GetChild(j);
								CSC_DSP *pDSP = pChain->m_DSPs[j].m_pDSP;
								CRegistry *pValuesReg = pDSPReg->FindChild("Values");
								if (pValuesReg)
								{
									uint32 nValues = pValuesReg->GetNumChildren();
									for (int k = 0; k < nValues; ++k)
									{
										CRegistry *pValueReg = pValuesReg->GetChild(k);
										CStr ValueName = pValueReg->GetThisName();
										CSC_DSP::CParam *pParam = pDSP->GetParam(ValueName);
										if (!pParam)
											ThrowError(CStrF("Param not found for values key: %s", ValueName.Str()));

										int iNew = pChain->m_DefaultParams.Len();
										pChain->m_DefaultParams.SetLen(iNew + 1);
										CSC_DSPChain::CDefaultParams &NewParams = pChain->m_DefaultParams[iNew];
										NewParams.m_iParam = pParam->m_Index;
										NewParams.m_iDSP = j;

										switch (pParam->m_Type)
										{
										case EDSPParamType_mint:
											{
												ThrowError("You cannot set a mint value");
											}
											break;
										case EDSPParamType_int32:
											{
												CParamValue_int32 *pParam = MNew(CParamValue_int32);
												NewParams.m_spValue = pParam;
												pParam->m_Value = Internal_TranslateDSPValue_int32(pValueReg->GetThisValue());
											}
											break;
										case EDSPParamType_fp32:
											{
												CParamValue_fp32 *pParam = MNew(CParamValue_fp32);
												NewParams.m_spValue = pParam;
												pParam->m_Value = Internal_TranslateDSPValue_fp32(pValueReg->GetThisValue());
											}
											break;
										case EDSPParamType_Route:
											{
												CParamValue_Route *pParam = MNew(CParamValue_Route);
												NewParams.m_spValue = pParam;
												pParam->m_Value = Internal_TranslateDSPValue_Route(pValueReg->GetThisValue())->m_pMixerDSPChainInstance;
											}
											break;
										case EDSPParamType_VolumeMatrix:
											{
												CParamValue_VolumeMatrix *pParam = MNew(CParamValue_VolumeMatrix);
												NewParams.m_spValue = pParam;
												Internal_TranslateDSPValue_VolumeMatrix(*pValueReg, pParam->m_Value);
											}
											break;
										default:
											ThrowError("Unknown param type.");
											break;

										}
									}
								}

								CRegistry *pBindsReg = pDSPReg->FindChild("Bind");
								if (pBindsReg)
								{
									uint32 nBounds = pBindsReg->GetNumChildren();
									for (int k = 0; k < nBounds; ++k)
									{
										CRegistry *pBindReg = pBindsReg->GetChild(k);
										CStr BindName = pBindReg->GetThisName();
										CSC_DSP::CParam *pParam = pDSP->GetParam(BindName);
										if (!pParam)
											ThrowError(CStrF("Param not found for bind key: %s", BindName.Str()));

										int iNew = pChain->m_BindingEntries.Len();
										pChain->m_BindingEntries.SetLen(iNew + 1);
										CBindingEntry &BindingEntry = pChain->m_BindingEntries[iNew];
										BindingEntry.m_pChain = pChain;
										BindingEntry.m_iType = pParam->m_Type;
										BindingEntry.m_iDSP = j;
										BindingEntry.m_iParam = pParam->m_Index;

										CStr Bind = pBindReg->GetThisValue();
										CStr BindTo = Bind.GetStrSep(",");
										CStr Mul = Bind.GetStrSep(",");
										CStr Add = Bind.GetStrSep(",");
										CStr BindToCategory = BindTo.GetStrSep(".");
										if (Mul.IsEmpty())
											BindingEntry.m_Mul = 1.0f;
										else
											BindingEntry.m_Mul = Mul.Val_fp64();
										if (Add.IsEmpty())
											BindingEntry.m_Add = 0.0f;
										else
											BindingEntry.m_Add = Add.Val_fp64();

										if (BindToCategory == "Global")
										{
											int iEntry = BindTo.TranslateInt(g_pDSPGlobalBind);
											if (iEntry < 0)
												ThrowError(CStrF("Unknown bind: %s.%s", BindToCategory.Str(), BindTo.Str()));

											BindingEntry.m_iBinding = iEntry;
											m_Bindings[iEntry].m_Entries.Insert(BindingEntry);											
										}
										else if (BindToCategory == "Voice")
										{
											BindingEntry.m_pChain = NULL; // Set to null so we know this is a voice binding
											int iEntry = BindTo.TranslateInt(g_pDSPVoiceBind);
											if (iEntry < 0)
												ThrowError(CStrF("Unknown bind: %s.%s", BindToCategory.Str(), BindTo.Str()));
											pChain->m_VoiceBindings[iEntry].m_Entries.Insert(BindingEntry);											
											BindingEntry.m_iBinding = iEntry;
										}
										else
											ThrowError(CStrF("Unknown binding category: %s", BindToCategory.Str()));
									}
								}
							}
						}
					}
				}
			}
		}
	}

	{
		CDSPChainTreeIter Iter = m_DSPChainTreeGlobal;
		while (Iter)
		{
			// Set params
			Internal_UpdateChainParams(Iter->m_pGlobalChain);
			++Iter;
		}
	}

	// Create
	{
		CDSPChainTreeIter Iter = m_DSPChainTreeGlobal;
		while (Iter)
		{
			CSoundContext_Mixer::CSC_DSPChainInstance *pChainInstance = Iter->m_pGlobalChain;
			CSC_Mixer_DSPChainInstance *pInstance = pChainInstance->m_pMixerDSPChainInstance;
			m_Mixer.DSPChainInstance_RunCreate(pInstance);
			m_Mixer.DSPChainInstance_SetActive(pInstance);
			++Iter;
		}
	}


	
	{
		CSC_DSPChain *pMaster = Internal_GetGlobalChainByName("Master Output");
		if (!pMaster || !pMaster->m_pGlobalChain)
			ThrowError("Could not find master DSP instance");
		m_Mixer.DSPChainInstance_SetMasterInstance(pMaster->m_pGlobalChain->m_pMixerDSPChainInstance);
	}

	{
		m_pDefault2DChain = Internal_GetVoiceChainByName("Default 2D");
		if (!m_pDefault2DChain)
			ThrowError("Could not find default 2D DSP chain");
	}
	{
		m_pDefault3DChain = Internal_GetVoiceChainByName("Default 3D");
		if (!m_pDefault3DChain)
			ThrowError("Could not find default 3D DSP chain");
	}
}

void CSoundContext_Mixer::Internal_UpdateChainParams(CSoundContext_Mixer::CSC_DSPChainInstance *_pChainInstance)
{
	TAP_RCD<CSC_DSPChain::CDefaultParams> pDefaultParams = _pChainInstance->m_pDSPChain->m_DefaultParams;
	for (int i = 0; i < pDefaultParams.Len(); ++i)
	{
		CSC_Mixer_DSPChainInstance *pInstance = _pChainInstance->m_pMixerDSPChainInstance;

		CSC_DSPChain::CDefaultParams &Param = pDefaultParams[i];
		switch (Param.m_spValue->m_Type)
		{
		case EDSPParamType_int32:
			{
				m_Mixer.DSPChainInstance_SetParams(pInstance, Param.m_iDSP, Param.m_iParam, &((CParamValue_int32 *)Param.m_spValue.p)->m_Value, sizeof(fp32));
			}
			break;
		case EDSPParamType_fp32:
			{
				m_Mixer.DSPChainInstance_SetParams(pInstance, Param.m_iDSP, Param.m_iParam, &((CParamValue_fp32 *)Param.m_spValue.p)->m_Value, sizeof(int32));
			}
			break;
		case EDSPParamType_Route:
			{
				m_Mixer.DSPChainInstance_SetParams(pInstance, Param.m_iDSP, Param.m_iParam, &((CParamValue_Route *)Param.m_spValue.p)->m_Value, sizeof(CSC_Mixer_WorkerContext::CCustomPtrHolder));
			}
			break;
		case EDSPParamType_VolumeMatrix:
			{
				// This one is a little special. Only works when num source and dest channels is already set for only this type
				CSC_Mixer_DSP_VolumeMatrix::CParams *pParams = (CSC_Mixer_DSP_VolumeMatrix::CParams *)m_Mixer.DSPChainInstance_GetParamPtr(pInstance, Param.m_iDSP);
				CParamValue_VolumeMatrix * pValue = (CParamValue_VolumeMatrix *)Param.m_spValue.p;

				// Just set a reference to this value because we are constant
				uint32 nMatrix = AlignUp(pParams->m_nSourceChannels, 4) * AlignUp(pParams->m_nDestChannels, 4);
				uint32 nBytes = AlignUp(nMatrix * 2, 16);
				if (!pParams->m_pVolumeMatrix)
				{
					pParams->m_pVolumeMatrix = (vec128 *)CSC_Mixer_WorkerContext::CCustomAllocator::AllocAlign(nBytes, 16);
					CSC_Mixer_DSP_VolumeMatrix::CLastParams *pLastParams = (CSC_Mixer_DSP_VolumeMatrix::CLastParams *)m_Mixer.DSPChainInstance_GetLastParamPtr(pInstance, Param.m_iDSP);
					pLastParams->m_pVolumeMatrix = (vec128 *)CSC_Mixer_WorkerContext::CCustomAllocator::AllocAlign(nBytes, 16);
					if (pLastParams->m_pVolumeMatrix)
					{
						uint32 nVec = nBytes >> 4;
						vec128 *pDst = pLastParams->m_pVolumeMatrix;
						if (pDst)
						{
							vec128 Zero = M_VZero();
							for (uint32 i = 0; i < nVec; ++i)
							{
								pDst[i] = Zero;
							}
						}
					}
				}
				if (pParams->m_pVolumeMatrix)
				{
					DataCopy((vec128*)pParams->m_pVolumeMatrix, (vec128 *)pValue->m_Value[pParams->m_nSourceChannels][pParams->m_nDestChannels].GetBasePtr(), nBytes >> 4);

					uint32 nSource = AlignUp(pParams->m_nSourceChannels, 4);
					uint32 nDest = Min(pParams->m_nDestChannels, m_PlatformInfo.m_nChannels);
					uint16 *pDest = (uint16 *)(vec128 *)pParams->m_pVolumeMatrix;

					for (uint32 i = 0; i < nDest; ++i)
					{
						if (m_PlatformInfo.m_Speakers[i].m_bLFE)
						{
							uint32 iData = i * nSource;
							for (uint32 j = 0; j < nSource; ++j)
							{
								pDest[iData + j] = TruncToInt(pDest[iData + j] * m_LFEScale);
							}

						}
					}

				}
			}
			break;
		}					

	}
}


void CSoundContext_Mixer::KillThreads()
{
	m_DualStream.KillThreads();
}

NThread::CMutual &CSoundContext_Mixer::GetInterfaceLock()
{
	return m_InterfaceLock;
}

/////////////////////////////////////////////////////////
// Channel Management

CSoundContext_Mixer::CChannel *CSoundContext_Mixer::Internal_GetChannel(uint32 _ChannelHandle)
{
	if (!m_lChannels.ValidPos(_ChannelHandle))
		ThrowError("Invalid channel handle");

	CChannel *pChannel = m_lChannels[_ChannelHandle];
	if (!pChannel)
		ThrowError("Invalid channel handle");
	return pChannel;
}


int CSoundContext_Mixer::Chn_Alloc(int _nVoices)
{
	DLock(m_InterfaceLock);
	if (_nVoices > m_nFreeVoices)
	{
		ThrowError("Not enough free voices");
		return -1;
	}

	TAP_RCNRTM<CChannel *> Tap = m_lChannels;
	int iChannel = -1;
	for (int i = 0; i < Tap.Len(); ++i)
	{
		if (Tap[i] == 0)
		{
			iChannel = i;
			break;
		}
	}

	if (iChannel < 0)
	{
		iChannel = m_lChannels.Len();
		m_lChannels.SetLen(iChannel + 1);
	}

	CChannel *pChannel = DNew(CChannel) CChannel;
	m_lChannels[iChannel] = pChannel;
	m_Channels.Insert(pChannel);

	while (_nVoices)
	{
		++pChannel->m_nFreeVoices;
		--m_nFreeVoices;
		--_nVoices;
		CVoice *pVoice = m_FreeVoices.Pop();
		pVoice->m_pChannel = pChannel;
		pChannel->m_FreeVoices.Insert(pVoice);
	}
	pChannel->m_nVoices = pChannel->m_nFreeVoices;

	return iChannel;
}

void CSoundContext_Mixer::Chn_Free(int _hChn)
{
	DLock(m_InterfaceLock);
	CChannel *pChannel = Internal_GetChannel(_hChn);

	// Do cleanup
	{
		CVoice *pVoice = pChannel->m_AlloctedVoices.Pop();
		while (pVoice)
		{
			Internal_VoiceDestroy(pVoice);
			pVoice = pChannel->m_AlloctedVoices.Pop();
		}
		pVoice = pChannel->m_FreeVoices.Pop();
		while (pVoice)
		{
			--pChannel->m_nFreeVoices;
			m_FreeVoices.Insert(pVoice);
			++m_nFreeVoices;
			pVoice = pChannel->m_FreeVoices.Pop();
		}
	}

	// Delete
	delete pChannel;
	m_lChannels[_hChn] = NULL;
}


void CSoundContext_Mixer::Chn_SetVolume(int _hChn, float _Volume)
{
	DLock(m_InterfaceLock);
	CChannel *pChannel = Internal_GetChannel(_hChn);
	pChannel->m_Volume = _Volume;
	CVoiceIter Iter = pChannel->m_AlloctedVoices;
	while (Iter)
	{
		Internal_VoiceUpdateVolume(Iter, true);
		++Iter;
	}
}

fp32 CSoundContext_Mixer::Chn_GetVolume(int _hChn)
{
	DLock(m_InterfaceLock);
	CChannel *pChannel = Internal_GetChannel(_hChn);
	return pChannel->m_Volume;
}

void CSoundContext_Mixer::Chn_SetPitch(int _hChn, float _Pitch)
{
	DLock(m_InterfaceLock);
	CChannel *pChannel = Internal_GetChannel(_hChn);
	pChannel->m_Pitch = _Pitch;
	CVoiceIter Iter = pChannel->m_AlloctedVoices;
	while (Iter)
	{
		Internal_VoiceUpdatePitch(Iter);
		++Iter;
	}
}

void CSoundContext_Mixer::Chn_Play(int _hChn)
{
	DLock(m_InterfaceLock);
	CChannel *pChannel = Internal_GetChannel(_hChn);
	CVoiceIter Iter = pChannel->m_AlloctedVoices;
	while (Iter)
	{
		Internal_VoiceUnpause(Iter, EPauseSlot_Channel);
		++Iter;
	}
}

void CSoundContext_Mixer::Chn_Pause(int _hChn)
{
	DLock(m_InterfaceLock);
	CChannel *pChannel = Internal_GetChannel(_hChn);
	CVoiceIter Iter = pChannel->m_AlloctedVoices;
	while (Iter)
	{
		Internal_VoicePause(Iter, EPauseSlot_Channel);
		++Iter;
	}
}

void CSoundContext_Mixer::Chn_DestroyVoices(int _hChn)
{
	DLock(m_InterfaceLock);
	CChannel *pChannel = Internal_GetChannel(_hChn);
	CVoice *pVoice = pChannel->m_AlloctedVoices.Pop();
	while (pVoice)
	{
		Internal_VoiceDestroy(pVoice);
		pVoice = pChannel->m_AlloctedVoices.Pop();
	}
}

/////////////////////////////////////////////////////////
// Voice Management

void CSoundContext_Mixer::Internal_VoicePause(CVoice *_pVoice, uint32 _PauseFlags)
{
	uint32 WasPaused = _pVoice->m_PauseSlot;

	_pVoice->m_PauseSlot |= _PauseFlags;

	if (_pVoice->m_iMixerVoice == 0xffffffff)
		return;

	if (_pVoice->m_PauseSlot && !WasPaused)
	{
		m_Mixer.Voice_Pause(_pVoice->m_iMixerVoice);
	}
}

void CSoundContext_Mixer::Internal_VoiceUnpause(CVoice *_pVoice, uint32 _PauseFlags)
{
	uint32 WasPaused = _pVoice->m_PauseSlot;

	_pVoice->m_PauseSlot &= ~_PauseFlags;

	if (_pVoice->m_iMixerVoice == 0xffffffff)
		return;

	if (!_pVoice->m_PauseSlot && WasPaused)
	{
		m_Mixer.Voice_Unpause(_pVoice->m_iMixerVoice);
	}
}

void CSoundContext_Mixer::Internal_VoiceUpdatePitch(CVoice *_pVoice)
{
	if (_pVoice->m_iMixerVoice == 0xffffffff)
		return;

	m_Mixer.Voice_SetPitch(_pVoice->m_iMixerVoice, _pVoice->m_Pitch * _pVoice->m_Pitch_3D * _pVoice->m_pChannel->m_Pitch);
}


void CSoundContext_Mixer::Internal_VoiceUpdateVolume(CVoice *_pVoice, bint _bInterpolate)
{
	if (_pVoice->m_iMixerVoice == 0xffffffff)
		return;

	fp32 Volume = _pVoice->m_Volume * _pVoice->m_pChannel->m_Volume * _pVoice->m_Volume_3D;
	if (_pVoice->m_bIsMusic)
		Volume *= m_MusicVolume_Total;
	m_Mixer.Voice_SetVolume(_pVoice->m_iMixerVoice, Volume, _bInterpolate);
}

void CSoundContext_Mixer::Internal_VoiceDestroy(CVoice *_pVoice)
{
	if (_pVoice->m_iMixerVoice != 0xffffffff)
	{
		// Set volume to 0.0f
		m_Mixer.Voice_SetVolume(_pVoice->m_iMixerVoice, 0.0f, true);
		Platform_StopStreamingToMixer(_pVoice->m_iMixerVoice);
		_pVoice->m_iMixerVoice = 0xffffffff;
	}
	m_VoiceHandleTree.f_Remove(_pVoice);
	_pVoice->Clear();
	_pVoice->m_pChannel->m_FreeVoices.Insert(_pVoice);
	++_pVoice->m_pChannel->m_nFreeVoices;
}

CSoundContext_Mixer::CVoice *CSoundContext_Mixer::Internal_GetVoiceFromHandle(uint32 _Handle)
{
	CVoice *pRet = m_VoiceHandleTree.FindEqual(_Handle);

	return pRet;
}



class CVoicePrioSort
{
public:

	M_INLINE static int Compare(void *_pContext, const CSoundContext_Mixer::CVoice *_VB0, const CSoundContext_Mixer::CVoice *_VB1)
	{
		if (_VB0->m_bLooping > _VB1->m_bLooping)
			return 1;
		if (_VB0->m_bLooping < _VB1->m_bLooping)
			return -1;

		if (_VB0->m_Priority < _VB1->m_Priority)
			return -1;
		if (_VB0->m_Priority > _VB1->m_Priority)
			return 1;

		return _VB0->m_StartTime.Compare(_VB1->m_StartTime);
	}

};

CSoundContext_Mixer::CVoice *CSoundContext_Mixer::Internal_VoiceAlloc(uint32 _iChannel, uint32 _Priority, uint32 _Looping, uint32 _WaveID, fp32 _Pitch)
{
	CChannel *pChannel = Internal_GetChannel(_iChannel);

	if (!pChannel->m_nFreeVoices)
	{
		// TODO: Check that voice priority sort is correct

		CVoice *pFirst = pChannel->m_AlloctedVoices.GetFirst();

		// No can do
		if (!pFirst)
			return NULL;
		if ((!_Looping && pFirst->m_bLooping) || pFirst->m_Priority > _Priority)
			return NULL;

		Internal_VoiceDestroy(pFirst);
	}

	CWaveData Data;
	m_spWaveContext->GetWaveLoadedData(_WaveID, Data);

	// Try to allocate mixer voice
	uint32 MixerVoice = m_Mixer.Voice_GetFreeVoice(_Pitch);
	if (MixerVoice == 0xffffffff) // No mixer voices free
		return NULL;

	CVoice* pVoice = pChannel->m_FreeVoices.Pop();
	--pChannel->m_nFreeVoices;

	CMTime Now = CMTime::GetCPU();
	pVoice->m_Priority = _Priority;
	pVoice->m_StartTime = Now;
	pVoice->m_bLooping = _Looping;
	pVoice->m_iMixerVoice = MixerVoice;

	pChannel->m_AlloctedVoices.InsertSorted<CVoicePrioSort>(pVoice);

	pVoice->m_hVoice = ++m_iCurrentVoiceHandle;
	m_VoiceHandleTree.f_Insert(pVoice);

	return pVoice;
}


CSoundContext_Mixer::CVoice *CSoundContext_Mixer::Internal_VoiceCreate(int _hChn, int _WaveID, int _Priority, fp32 _Pitch, fp32 _Volume, bool _bLoop, bool _bStartPaused, fp32 _MinDelay)
{
//	_bLoop = true;
	CVoice* pVoice = Internal_VoiceAlloc(_hChn, _Priority, _bLoop, _WaveID, _Pitch);
	if (!pVoice)
		return NULL;

	pVoice->m_Volume = _Volume;
	pVoice->m_WaveID = _WaveID;
	pVoice->m_PauseSlot = EPauseSlot_Delayed;
	if (_bStartPaused)
		pVoice->m_PauseSlot |= 1;

	return pVoice;
}

void CSoundContext_Mixer::Voice_InitCreateParams(CSC_VoiceCreateParams &_CreateParams, uint32 _Category)
{
	DLock(m_SoundCategoriesLock);
	CSoundCategory *pCategory = m_SoundCategories.FindEqual(_Category);
	if (!pCategory)
	{
		if (_CreateParams.m_Spatialization == SC_SNDSPATIALIZATION_3D)
		{
			pCategory = m_SoundCategories.FindEqual(202);
		}
		else
		{
			pCategory = m_SoundCategories.FindEqual(201);
		}
	}
	if (pCategory)
	{
		_CreateParams.m_Category = _Category;
		uint32 OverrideFlags = pCategory->m_WhatToOverride;
		if (OverrideFlags & ECategoryOverride_Priority)
		{
			_CreateParams.m_Priority = pCategory->m_Params.m_Priority;
		}

		if (OverrideFlags & ECategoryOverride_DSPID)
		{
			_CreateParams.m_DSPId = pCategory->m_Params.m_DSPId;
		}

		if (OverrideFlags & ECategoryOverride_Pitch)
		{
			_CreateParams.m_Pitch = pCategory->m_Params.m_Pitch;
		}

		if (OverrideFlags & ECategoryOverride_Volume)
		{
			_CreateParams.m_Volume = pCategory->m_Params.m_Volume;
		}

		if (OverrideFlags & ECategoryOverride_MinDelay)
		{
			_CreateParams.m_MinDelay = pCategory->m_Params.m_MinDelay;
		}

		if (OverrideFlags & ECategoryOverride_Spatialization)
		{
			_CreateParams.m_Spatialization = pCategory->m_Params.m_Spatialization;
		}

		if (OverrideFlags & ECategoryOverride_Doppler)
		{
			_CreateParams.m_bDoppler = pCategory->m_Params.m_bDoppler;
		}

		if (OverrideFlags & ECategoryOverride_Falloff)
		{
			_CreateParams.m_bFalloff = pCategory->m_Params.m_bFalloff;
		}

		if (OverrideFlags & ECategoryOverride_Loop)
		{
			_CreateParams.m_bLoop = pCategory->m_Params.m_bLoop;
		}

		if (OverrideFlags & ECategoryOverride_IsMusic)
		{
			_CreateParams.m_bIsMusic = pCategory->m_Params.m_bIsMusic;
		}

		if (OverrideFlags & ECategoryOverride_MinDist)
		{
			_CreateParams.m_3DProperties.m_MinDist = pCategory->m_Params.m_3DProperties.m_MinDist;
		}

		if (OverrideFlags & ECategoryOverride_MaxDist)
		{
			_CreateParams.m_3DProperties.m_MaxDist = pCategory->m_Params.m_3DProperties.m_MaxDist;
		}

		if (OverrideFlags & ECategoryOverride_DirectFalloff)
		{
			_CreateParams.m_3DProperties.m_pDirectFalloff = &pCategory->m_DirectFalloff;
		}

		if (OverrideFlags & ECategoryOverride_ReverbFalloff)
		{
			_CreateParams.m_3DProperties.m_pReverbFalloff = &pCategory->m_ReverbFalloff;
		}
	}

}

/*
int CSoundContext_Mixer::Voice_Create(int _hChn, int _WaveID, int _Priority, fp32 _Pitch, fp32 _Volume, bool _bLoop, bool _bStartPaused, fp32 _MinDelay)
{
	DLock(m_InterfaceLock);
	if (_WaveID < 0)
		return -1;
	CWaveData Data;
	m_spWaveContext->GetWaveLoadedData(_WaveID, Data);
	fp32 OriginalSampleRate = Data.GetSampleRate();
	fp32 SampleRate = OriginalSampleRate * _Pitch;
	CSC_DSPChain *pChain = m_pDefault2DChain;
	CSC_Mixer_DSPChainInstance *pChainInstance = m_Mixer.DSPChain_CreateIntstance(pChain->m_pMixerDSPChain);
	if (!pChainInstance)
		return -1;
	CVoice* pVoice = Internal_VoiceCreate(_hChn, _WaveID, _Priority, SampleRate, _Volume, _bLoop, _bStartPaused, _MinDelay);
	if (!pVoice)
	{
		m_Mixer.DSPChainInstance_Destroy(pChainInstance);
		return -1;
	}

	m_Mixer.Voice_SetDSPChain(pVoice->m_iMixerVoice, pChainInstance);
	pVoice->m_DSPChainInstance.m_pDSPChain = pChain;
	pVoice->m_DSPChainInstance.m_pMixerDSPChainInstance = pChainInstance;
	pChain->m_ChainInstances.Insert(pVoice->m_DSPChainInstance);
	pVoice->m_nChannels = Data.GetChannels();
	pVoice->m_OriginalSampleRate = SampleRate;

	Internal_UpdateVoiceBindings(pVoice);
	Internal_UpdateChainParams(&pVoice->m_DSPChainInstance);
	Internal_VoiceUpdateVolume(pVoice, _bLoop);
	Internal_VoiceUpdatePitch(pVoice);

	m_Mixer.DSPChainInstance_RunCreate(pChainInstance);

	fp32 SamplesDelay = Max(_MinDelay, 0.0f) * SampleRate;
	m_Mixer.Voice_Play(pVoice->m_iMixerVoice, TruncToInt(SamplesDelay), true);
	Platform_StartStreamingToMixer(pVoice->m_iMixerVoice, pVoice, _WaveID, pVoice->m_OriginalSampleRate);
	return pVoice->m_hVoice;
}
*/
int CSoundContext_Mixer::Voice_Create(const CSC_VoiceCreateParams &_CreateParams)
{
	DLock(m_InterfaceLock);
	if (_CreateParams.m_WaveID < 0)
		return -1;
	CWaveData Data;
	m_spWaveContext->GetWaveLoadedData(_CreateParams.m_WaveID, Data);
	fp32 OriginalSampleRate = Data.GetSampleRate();
	fp32 SampleRate = OriginalSampleRate * _CreateParams.m_Pitch;
	CSC_DSPChain *pChain;
	if (_CreateParams.m_DSPId == 0)
	{
		if (_CreateParams.m_Spatialization == SC_SNDSPATIALIZATION_3D)
			pChain = m_pDefault3DChain;
		else
			pChain = m_pDefault2DChain;
	}
	else
	{
		pChain = Internal_GetVoiceChainByHash(_CreateParams.m_DSPId);
		
		if (!pChain)
			M_BREAKPOINT; // Chain must exist
	}

	CSC_Mixer_DSPChainInstance *pChainInstance = m_Mixer.DSPChain_CreateIntstance(pChain->m_pMixerDSPChain);
	if (!pChainInstance)
		return -1;
	CVoice* pVoice = Internal_VoiceCreate(_CreateParams.m_hChannel, _CreateParams.m_WaveID, _CreateParams.m_Priority, SampleRate, _CreateParams.m_Volume, _CreateParams.m_bLoop, _CreateParams.m_bStartPaused, _CreateParams.m_MinDelay);
	if (!pVoice)
	{
		m_Mixer.DSPChainInstance_Destroy(pChainInstance);
		return -1;
	}

	m_Mixer.Voice_SetDSPChain(pVoice->m_iMixerVoice, pChainInstance);

	pVoice->m_DSPChainInstance.m_pDSPChain = pChain;
	pVoice->m_DSPChainInstance.m_pMixerDSPChainInstance = pChainInstance;
	pChain->m_ChainInstances.Insert(pVoice->m_DSPChainInstance);

	pVoice->m_nChannels = Data.GetChannels();
	pVoice->m_OriginalSampleRate = SampleRate;
	pVoice->m_Position = _CreateParams.m_3DProperties.m_Position;
	pVoice->m_Velocity = _CreateParams.m_3DProperties.m_Velocity;
	pVoice->m_Orientation = _CreateParams.m_3DProperties.m_Orientation;
	pVoice->m_Occlusion = _CreateParams.m_3DProperties.m_Occlusion;
	pVoice->m_MinDist = _CreateParams.m_3DProperties.m_MinDist;
	pVoice->m_MaxDist = _CreateParams.m_3DProperties.m_MaxDist;
	pVoice->m_pDirectFalloff = _CreateParams.m_3DProperties.m_pDirectFalloff;
	pVoice->m_pReverbFalloff = _CreateParams.m_3DProperties.m_pReverbFalloff;
	pVoice->m_Spatialization = _CreateParams.m_Spatialization;
	pVoice->m_bFalloff = _CreateParams.m_bFalloff;
	pVoice->m_bDoppler = _CreateParams.m_bDoppler;
	pVoice->m_bIsMusic = _CreateParams.m_bIsMusic;

	Internal_UpdateVoiceBindings(pVoice);
	Internal_UpdateChainParams(&pVoice->m_DSPChainInstance);
	Internal_VoiceUpdate3D(pVoice, false);
	Internal_VoiceUpdateVolume(pVoice, _CreateParams.m_bLoop);
	Internal_VoiceUpdatePitch(pVoice);

	m_Mixer.DSPChainInstance_RunCreate(pChainInstance);

	fp32 SamplesDelay = Max(_CreateParams.m_MinDelay, 0.0f) * SampleRate;
	pVoice->m_OriginalSampleRate = SampleRate * _CreateParams.m_Pitch;
	m_Mixer.Voice_Play(pVoice->m_iMixerVoice, TruncToInt(SamplesDelay), true);
	Platform_StartStreamingToMixer(pVoice->m_iMixerVoice, pVoice, _CreateParams.m_WaveID, pVoice->m_OriginalSampleRate);
	return pVoice->m_hVoice;
}

void CSoundContext_Mixer::Voice_Destroy(int _hVoice)
{
	DLock(m_InterfaceLock);
	CVoice* pVoice = Internal_GetVoiceFromHandle(_hVoice);
	if (!pVoice)
		return;
	Internal_VoiceDestroy(pVoice);	
}

bint CSoundContext_Mixer::Internal_VoiceIsPlaying(CVoice *_pVoice)
{
	if (_pVoice->m_iMixerVoice == 0xffffffff)
		return false;
	if (!Platform_IsPlaying(_pVoice->m_iMixerVoice))
		return false;
	bint bIsPlaying = m_Mixer.Voice_IsPlaying(_pVoice->m_iMixerVoice);
	return bIsPlaying;
}

bool CSoundContext_Mixer::Voice_IsPlaying(int _hVoice)
{
	DLock(m_InterfaceLock);
	CVoice* pVoice = Internal_GetVoiceFromHandle(_hVoice);
	if (!pVoice)
		return false;
	return Internal_VoiceIsPlaying(pVoice) != 0;
}

bint CSoundContext_Mixer::Voice_ReadyForPlay(int _hVoice)
{
	DLock(m_InterfaceLock);
	CVoice* pVoice = Internal_GetVoiceFromHandle(_hVoice);
	if (!pVoice)
		return true;

	if (pVoice->m_PauseSlot & EPauseSlot_Delayed)
		return false;
	else
		return true;
}



class CCone
{
public:


};

void CSoundContext_Mixer::Internal_VoiceUpdate3D(CVoice *_pVoice, bint _bUpdateMixer)
{
	if (!_pVoice->m_bFalloff && !_pVoice->m_bDoppler && _pVoice->m_Spatialization == SC_SNDSPATIALIZATION_NONE)
		return;

	CVec3Dfp32 Position;
	CVec3Dfp32 Velocity;
	CVec3Dfp32 Orientation;
	CSC_3DOcclusion Occlusion;
	fp32 MinDist;
	fp32 MaxDist;
	CMat4Dfp32 ListenerMatrix;
	CVec3Dfp32 ListenerVelocity;
	fp32 CoordinateScale;

	Position = _pVoice->m_Position;
	Velocity = _pVoice->m_Velocity;
	Orientation = _pVoice->m_Orientation;
	Occlusion = _pVoice->m_Occlusion;
	MinDist = _pVoice->m_MinDist;
	MaxDist = _pVoice->m_MaxDist;
	ListenerMatrix = m_Listener_Matrix;
	ListenerVelocity = m_Listener_Velocity;
	CoordinateScale = m_CoordinateScale;
	fp32 NewPitch = 1.0f;
	fp32 NewVolume = 1.0f;

	{
		// Unlock while doing the calculations?
//		DUnlock(m_InterfaceLock);
		CVec3Dfp32 ListenerPosition = CVec3Dfp32::GetMatrixRow(ListenerMatrix, 3);
		CMat4Dfp32 ListenerRotationMatrix = ListenerMatrix;
		CMat4Dfp32 ListenerRotationMatrixInv;
		CVec3Dfp32::GetMatrixRow(ListenerRotationMatrix, 3) = CVec3Dfp32(0.0f);
		ListenerRotationMatrix.Normalize3x3();
		ListenerRotationMatrix.Inverse3x3(ListenerRotationMatrixInv);
		CVec3Dfp32 ListenerOrientation = CVec3Dfp32::GetMatrixRow(ListenerMatrix, 0);
		CVec3Dfp32 RelativePos = (ListenerPosition - Position);

		// Folloff calc
		fp32 LinearRolloffDistance = MinDist + (MaxDist-MinDist) * 0.7f;
		fp32 RollOffFactor = 1.0f;
		fp32 Dist = ListenerPosition.Distance(Position);
		fp32 Volume = 1.0f/((Dist / MinDist) * RollOffFactor);
		Volume = Clamp(Volume, 0.0f, 1.0f);
		if (_pVoice->m_pDirectFalloff)
		{
			if (Dist < MinDist)
				Volume = 1.0f;
			else if (Dist >= MaxDist)
				Volume = 0.0f;
			else
			{
				fp32 Pos = (Dist - MinDist) / (MaxDist - MinDist);

				TAP_RCD<CSC_VoiceFalloff::CEntry> Tap = _pVoice->m_pDirectFalloff->m_Values;
				mint iPos = 0;
				for (mint i = Tap.Len()-2; i > 0; --i)
				{
					if (Tap[i].m_Position <= Pos)
					{
						iPos = i;
						break;
					}
				}

				fp32 LrpPos = (Pos - Tap[iPos].m_Position) / (Tap[iPos+1].m_Position - Tap[iPos].m_Position);
				Volume = Tap[iPos].m_Volume * (1.0-LrpPos) + Tap[iPos+1].m_Volume * LrpPos;
			}
		}
		else
		{
			if (Dist > LinearRolloffDistance)
			{
				if (Dist >= MaxDist)
					Volume = 0.0f;
				else
				{
					fp32 LinearFalloff = 1.0 - (Dist - LinearRolloffDistance) / (MaxDist - LinearRolloffDistance);
					Volume *= LinearFalloff;
				}
			}
		}

		// Pitch calc
		CVec3Dfp32 PosNormal = RelativePos.Normalize();
		fp32 SpeedOfSoundInv = 1.0f/340.29f;
		fp32 DotPr = (PosNormal * (ListenerVelocity - Velocity));
		fp32 Doppler = 1.0f - (DotPr * CoordinateScale) * SpeedOfSoundInv;
		NewPitch = Doppler;
		NewVolume = Volume;
		fp32 MaxPitchShift = 8.0f;
		NewPitch = Clamp(NewPitch, 1.0f/MaxPitchShift, MaxPitchShift);
		if (FloatIsNAN(NewPitch))
			NewPitch = 1.0f;
		if (FloatIsNAN(NewVolume))
			NewVolume = 1.0f;

		// Sound positioning calc

		CVec3Dfp32 RelPos = RelativePos * ListenerRotationMatrixInv;
		fp32 Angle = 0.0f;
		if (RelPos.k[0] == 0.0f && RelPos.k[1] == 0.0f)
			Angle = 0.0f;
		else
			Angle = M_ATan2(RelPos.k[1], RelPos.k[0]) * (0.5f/_PI);
		if (Angle < 0.0f)
			Angle += 1.0f;
		if (Angle == 1.0f)
			Angle = 1.0f;
		uint32 nActiveSpeakers = 0;
		fp32 VolumeMatrix[16];
		mint nChannels = m_PlatformInfo.m_nChannels;
		for (int i = 0; i < nChannels; ++i)
		{
			// Reset
			if (m_PlatformInfo.m_Speakers[i].m_bLFE)
				VolumeMatrix[i] = m_LFEScale;
			else
			{
				VolumeMatrix[i] = 0.0f;
				++nActiveSpeakers;
			}
		}
		int32 iLower = -1;
		fp32 BestLower = -2.0f;
		int32 iUpper = -1;
		fp32 BestUpper = 2.0f;
		for (int i = 0; i < nChannels; ++i)
		{
			if (m_PlatformInfo.m_Speakers[i].m_bLFE)
				continue;

			fp32 Diff0 = m_SpeakerAngles[i] - Angle;
			fp32 Diff1 = (m_SpeakerAngles[i] - 1.0) - Angle;
			fp32 Diff2 = (m_SpeakerAngles[i]) - (Angle - 1.0);
			fp32 Diff3 = (m_SpeakerAngles[i] - 1.0) - (Angle - 1.0);

			fp32 DiffLower = Diff0;
			if (Diff1 < 0.0f && Abs(Diff1) < Abs(DiffLower))
				DiffLower = Diff1;
			if (Diff2 < 0.0f && Abs(Diff2) < Abs(DiffLower))
				DiffLower = Diff2;
			if (Diff3 < 0.0f && Abs(Diff3) < Abs(DiffLower))
				DiffLower = Diff3;

			fp32 DiffUpper = Diff0;
			if (Diff1 > 0.0f && Abs(Diff1) < Abs(DiffUpper))
				DiffUpper = Diff1;
			if (Diff2 > 0.0f && Abs(Diff2) < Abs(DiffUpper))
				DiffUpper = Diff2;
			if (Diff3 > 0.0f && Abs(Diff3) < Abs(DiffUpper))
				DiffUpper = Diff3;

			if (DiffLower == 0.0f)
			{
				iLower = i;
				iUpper = i;
				break;
			}
			
			if (DiffLower < 0.0 && (iLower == -1 || DiffLower > BestLower))
			{
				BestLower = DiffLower;
				iLower = i;
			}
			
			if (DiffUpper > 0.0 && (iUpper == -1 || DiffUpper < BestUpper))
			{
				BestUpper = DiffUpper;
				iUpper = i;
			}			
		}
		if (iLower == -1 || iUpper == -1)
		{
			// This happens when an invalid listener is set
			iLower = iUpper = 0;
//			M_TRACEALWAYS("WARING WARING WARING WARING WARING WARING: Invalid Listen3D origin set to sound context\n");
//			M_BREAKPOINT; // Should not happen
		}

		fp32 Diff = (BestUpper - BestLower);
		fp32 DiffInv = 1.0f/Diff;
		VolumeMatrix[iLower] = 1.0f - (-BestLower) * DiffInv;
		if (iLower != iUpper)
			VolumeMatrix[iUpper] = 1.0f - VolumeMatrix[iLower];

		fp32 Length = CVec2Dfp32(RelPos.k[0], RelPos.k[1]).Length();
		fp32 UpAngle = M_ATan2(RelPos.k[2], Length) * (1.0f/(0.5f*_PI));

//		fp32 Upness = RelPos * CVec3Dfp32(0.0f, 0.0f, 1.0f); // Up
		fp32 Upness = M_Fabs(UpAngle); // Up and down

//		fp32 Upness2 = M_Fabs(RelPos * CVec3Dfp32(0.0f, 0.0f, 1.0f)); // Up
//		M_TRACEALWAYS("Upness: %f %f\n", Upness, Upness2);

		// Lrp towards all channels
		fp32 LrpVolume = 1.0f/nActiveSpeakers;
		for (int i = 0; i < nChannels; ++i)
		{
			// Reset
			if (!m_PlatformInfo.m_Speakers[i].m_bLFE)
				VolumeMatrix[i] = M_Sqrt(VolumeMatrix[i] * (1.0f - Upness) + (LrpVolume * Upness));
		}

		uint16 M_ALIGN(16) VolumeMatrix_uint16[16*16];

		uint16 *pMatrix = VolumeMatrix_uint16;
		uint32 nChannelsAlign = AlignUp(nChannels, 4);
		uint32 nSrcChannels = _pVoice->m_nChannels;
		uint32 nSrcChannelsAlign = AlignUp(nSrcChannels, 4);
		
		for (uint32 i = 0; i < nChannels; ++i)
		{
			uint16 Blaha = TruncToInt(Clamp(VolumeMatrix[i] * 65535.0f, 0.0f, 65535.0f));
			for (uint32 j = 0; j < nSrcChannels; ++j)
				VolumeMatrix_uint16[i * nSrcChannelsAlign + j] = Blaha;
			for (uint32 j = nSrcChannels; j < nSrcChannelsAlign; ++j)
				VolumeMatrix_uint16[i * nSrcChannelsAlign + j] = 0;
		}
		for (uint32 i = nChannels; i < nChannelsAlign; ++i)
		{
			for (uint32 j = 0; j < nSrcChannelsAlign; ++j)
				VolumeMatrix_uint16[i * nSrcChannelsAlign + j] = 0;
		}

		if (_pVoice->m_Spatialization != SC_SNDSPATIALIZATION_NONE)
			Internal_VoiceBindingChanged(_pVoice, EDSPBindingVoice_3DVolumeMatrix, VolumeMatrix_uint16);

/*		for (int i = 0; i < nChannels; ++i)
		{
			M_TRACEALWAYS("%.2f ", VolumeMatrix[i]);
		}
		M_TRACEALWAYS("\n");*/
/*		sd = _alcGetSpeakerPosition(cid, i);
		sa = _alVectorDotp(lp, sp, sd) / m;
		sa += 1.0;*/



	}

	if (_pVoice->m_bDoppler)
		_pVoice->m_Pitch_3D = NewPitch;
	if (_pVoice->m_bFalloff)
		_pVoice->m_Volume_3D = NewVolume;
	if (_bUpdateMixer)
	{
		Internal_VoiceUpdateVolume(_pVoice, true);
		Internal_VoiceUpdatePitch(_pVoice);
	}
}

CMTime CSoundContext_Mixer::Voice_GetPlayPos(int _hVoice)
{
	DLock(m_InterfaceLock);
	CVoice* pVoice = Internal_GetVoiceFromHandle(_hVoice);
	if (!pVoice || pVoice->m_iMixerVoice == 0xffffffff)
		return CMTime();

	return m_Mixer.Voice_GetPosition(pVoice->m_iMixerVoice);
}


CMTime CSoundContext_Mixer::SFX_GetLength(CSC_SFXDesc *_pSFXDesc, int _iWave)
{
	DLock(m_InterfaceLock);
	CWaveData Data;
	m_spWaveContext->GetWaveLoadedData(_pSFXDesc->GetWaveId(_iWave), Data);
	fp64 PlayRate = Data.GetSampleRate() * _pSFXDesc->GetPitch();
	fp64 BytesPerSecond = Data.GetSampleSize() * Data.GetChannels() * PlayRate;

	fp64 Time = (Data.GetNumSamples()) / BytesPerSecond;
	int64 Seconds = (int64)Time;
	fp32 Fraction = Time - Seconds;

	return CMTime::CreateFromTicks(Seconds, 1, Fraction);
}


void CSoundContext_Mixer::Internal_UpdateMusicVolume()
{
	fp32 NewVolume = m_MusicVolume_System * m_MusicVolume_Settings * m_MusicVolume_Game;
	fp32 ChangeSpeed = 0.25f;

	ModerateFloat(m_MusicVolume_Total, NewVolume, m_MusicVolume_TotalPrim, ChangeSpeed);
	

	if (!AlmostEqual(m_MusicVolume_TotalLast, m_MusicVolume_Total, 0.001))
	{
		m_MusicVolume_TotalLast = NewVolume;
		CChannelIter ChnIter = m_Channels;
		while (ChnIter)
		{
			CChannel *pChannel = ChnIter;

			CVoiceIter VoiceIter = pChannel->m_AlloctedVoices;
			while (VoiceIter)
			{
				CVoice *pVoice = VoiceIter;
				++VoiceIter;
				if (pVoice->m_bIsMusic)
					Internal_VoiceUpdateVolume(pVoice, true);
			}

			++ChnIter;
		}
	}
}

// Refresh
void CSoundContext_Mixer::Refresh()
{
	DLock(m_InterfaceLock);
	m_DualStream.Refresh();

	CMTime SampleTime;
	CMTime CPUTime;
	m_Mixer.Global_GetTimeStats(SampleTime, CPUTime);

	fp32 Diff = (SampleTime - CPUTime).GetTime();
//	if (Abs(Diff) > 0.0025)
	{
		m_LastSetSampleRate = Clamp(m_LastSetSampleRateTraget + (Diff*m_LastSetSampleRateTraget*0.2f), m_LastSetSampleRateTraget - m_LastSetSampleRateTraget*0.25, m_LastSetSampleRateTraget + m_LastSetSampleRateTraget*0.25);
		m_Mixer.Global_SetSampleRate(m_LastSetSampleRate);
		m_Drift_LastCPUTime = CPUTime;
		m_Drift_LastSampleTime = SampleTime;
	}


	if (m_bAutoReloadCategories)
	{
		Internal_InitCategories();
	}

	CChannelIter ChnIter = m_Channels;
	Internal_UpdateMusicVolume();

	while (ChnIter)
	{
		CChannel *pChannel = ChnIter;

		CVoiceIter VoiceIter = pChannel->m_AlloctedVoices;
		while (VoiceIter)
		{
			CVoice *pVoice = VoiceIter;
			++VoiceIter;

			if (!Internal_VoiceIsPlaying(pVoice))
				Internal_VoiceDestroy(pVoice);
			else if (m_bUpdateAll3D)
				Internal_VoiceUpdate3D(pVoice, true);
		}

		++ChnIter;
	}

	if (m_bUpdateAll3D)
	{
		m_Need3DUpdate.Clear();
		m_bUpdateAll3D = false;
	}
	else
	{
		CVoice *pVoice = m_Need3DUpdate.Pop();
		while (pVoice)
		{
			Internal_VoiceUpdate3D(pVoice, true);
			pVoice = m_Need3DUpdate.Pop();
		}
	}
}

/////////////////////////////////////////////////////////
// Property modification
void CSoundContext_Mixer::Voice_SetPitch(int _hVoice, fp32 _Pitch)
{
	DLock(m_InterfaceLock);
	CVoice* pVoice = Internal_GetVoiceFromHandle(_hVoice);
	if (!pVoice || pVoice->m_iMixerVoice == 0xffffffff)
		return;
	pVoice->m_Pitch = _Pitch;
	Internal_VoiceUpdatePitch(pVoice);
}

void CSoundContext_Mixer::Voice_SetVolume(int _hVoice, fp32 _Volume)
{
	DLock(m_InterfaceLock);
	CVoice* pVoice = Internal_GetVoiceFromHandle(_hVoice);
	if (!pVoice || pVoice->m_iMixerVoice == 0xffffffff)
		return;
	pVoice->m_Volume = _Volume;
	Internal_VoiceUpdateVolume(pVoice, true);
}


void CSoundContext_Mixer::Voice_Unpause(int _hVoice, uint32 _PauseSlot)
{
	DLock(m_InterfaceLock);
	CVoice* pVoice = Internal_GetVoiceFromHandle(_hVoice);
	if (!pVoice || pVoice->m_iMixerVoice == 0xffffffff)
		return;
	Internal_VoiceUnpause(pVoice, _PauseSlot);
}

void CSoundContext_Mixer::Voice_Pause(int _hVoice, uint32 _PauseSlot)
{
	DLock(m_InterfaceLock);
	CVoice* pVoice = Internal_GetVoiceFromHandle(_hVoice);
	if (!pVoice || pVoice->m_iMixerVoice == 0xffffffff)
		return;
	Internal_VoicePause(pVoice, _PauseSlot);
}

/////////////////////////////////////////////////////////
// 3D-Property modification
void CSoundContext_Mixer::Voice3D_SetOrigin(int _hVoice, const CVec3Dfp32& _Pos)
{
	DLock(m_InterfaceLock);
	CVoice* pVoice = Internal_GetVoiceFromHandle(_hVoice);
	if (!pVoice)
		return;
	if (pVoice->m_Position != _Pos)
	{
		pVoice->m_Position = _Pos;
		m_Need3DUpdate.Insert(pVoice);
	}
}

void CSoundContext_Mixer::Voice3D_SetVelocity(int _hVoice, const CVec3Dfp32& _Vel)
{
	DLock(m_InterfaceLock);
	CVoice* pVoice = Internal_GetVoiceFromHandle(_hVoice);
	if (!pVoice)
		return;
	if (pVoice->m_Velocity != _Vel)
	{
		pVoice->m_Velocity = _Vel;
		m_Need3DUpdate.Insert(pVoice);
	}
}

void CSoundContext_Mixer::Voice3D_SetMinMaxDist(int _hVoice, fp32 _MinDist, fp32 _MaxDist)
{
	DLock(m_InterfaceLock);
	CVoice* pVoice = Internal_GetVoiceFromHandle(_hVoice);
	if (!pVoice)
		return;
	if (pVoice->m_MinDist != _MinDist || pVoice->m_MaxDist != _MaxDist)
	{
		pVoice->m_MinDist = _MinDist;
		pVoice->m_MaxDist = _MaxDist;
		m_Need3DUpdate.Insert(pVoice);
	}
}

void CSoundContext_Mixer::Voice3D_SetOcclusion(int _hVoice, const CSC_3DOcclusion &_Prop)
{
	DLock(m_InterfaceLock);
	CVoice* pVoice = Internal_GetVoiceFromHandle(_hVoice);
	if (!pVoice)
		return;
	pVoice->m_Occlusion = _Prop;
	Internal_VoiceBindingChanged(pVoice, EDSPBindingVoice_Obstruction, _Prop.m_Obstruction);
	Internal_VoiceBindingChanged(pVoice, EDSPBindingVoice_Occlusion, _Prop.m_Occlusion);
//	m_Need3DUpdate.Insert(pVoice);
}

void CSoundContext_Mixer::Voice3D_SetOrientation(int _hVoice, const CVec3Dfp32& _Orientation)
{
	DLock(m_InterfaceLock);
	CVoice* pVoice = Internal_GetVoiceFromHandle(_hVoice);
	if (!pVoice)
		return;

	if (pVoice->m_Orientation != _Orientation)
	{
		pVoice->m_Orientation = _Orientation;
		m_Need3DUpdate.Insert(pVoice);
	}
}

/////////////////////////////////////////////////////////
// 3D-Listener modifactions
void CSoundContext_Mixer::Listen3D_SetCoordinateScale(fp32 _Scale)
{
	DLock(m_InterfaceLock);
	if (m_CoordinateScale != _Scale)
	{
		m_CoordinateScale = _Scale;
		m_bUpdateAll3D = true;
	}
}

void CSoundContext_Mixer::Listen3D_SetOrigin(const CMat4Dfp32& _Pos)
{
	DLock(m_InterfaceLock);
	if (!m_Listener_Matrix.AlmostEqual(_Pos, 0.0001f))
	{
#ifdef M_Profile
		if (FloatIsNAN(_Pos.k[0][0]) || FloatIsNAN(_Pos.k[0][1]) || FloatIsNAN(_Pos.k[0][2]) || FloatIsNAN(_Pos.k[0][3]))
		{
			M_TRACEALWAYS("WARING WARING WARING WARING WARING WARING: Invalid Listen3D origin set to sound context\n");
		}
#endif
		m_Listener_Matrix = _Pos;
		m_bUpdateAll3D = true;
	}
}

void CSoundContext_Mixer::Listen3D_SetVelocity(const CVec3Dfp32& _Vel)
{
	DLock(m_InterfaceLock);
	if (m_Listener_Velocity != _Vel)
	{
		m_Listener_Velocity = _Vel;
		m_bUpdateAll3D = true;
	}
}

/////////////////////////////////////////////////////////
// Dual Stream
void CSoundContext_Mixer::MultiStream_Play(const char * *_Files, int _nFiles, bool _FadeIn, bool _bResetVolumes, uint32 _LoopMask)
{
	m_DualStream.MultiStream_Play(_Files, _nFiles, _FadeIn, _bResetVolumes, _LoopMask);
}

void CSoundContext_Mixer::MultiStream_Stop(bool _FadeOut)
{
	m_DualStream.MultiStream_Stop(_FadeOut);
}
void CSoundContext_Mixer::MultiStream_Volumes(fp32 *_Volumes, int _nVolumes, bool _Fade, fp32 *_pFadeSpeeds)
{
	m_DualStream.MultiStream_Volumes(_Volumes, _nVolumes, _Fade, _pFadeSpeeds);
}
void CSoundContext_Mixer::MultiStream_Volume(fp32 _Volume)
{
	m_DualStream.MultiStream_Volume(_Volume);
}
void CSoundContext_Mixer::MultiStream_Pause()
{
	m_DualStream.MultiStream_Pause();
}
void CSoundContext_Mixer::MultiStream_Resume()
{
	m_DualStream.MultiStream_Resume();
}

/////////////////////////////////////////////////////////
// Precache
void *CSoundContext_Mixer::LSD_Get(int16 _WaveID)
{
	return m_LipSyncManager.LSD_Get(_WaveID);
}

fp32 CSoundContext_Mixer::Timecode_Get(int16 _WaveID)
{
	return m_LipSyncManager.Timecode_Get(_WaveID);
}

/////////////////////////////////////////////////////////
// Precache
void CSoundContext_Mixer::Wave_PrecacheFlush()
{
	m_LipSyncManager.Wave_PrecacheFlush();
}

void CSoundContext_Mixer::Wave_PrecacheBegin( int _Count )
{
	m_LipSyncManager.Wave_PrecacheBegin();
}

void CSoundContext_Mixer::Wave_Precache(int _WaveID)
{
	m_LipSyncManager.Wave_Precache(_WaveID, m_spWaveContext);
}

int CSoundContext_Mixer::Wave_GetMemusage(int _WaveID)
{
	return 0;
}

void CSoundContext_Mixer::Wave_PrecacheEnd(uint16 *_pPreCacheOrder, int _nIds)
{
	m_LipSyncManager.Wave_PrecacheEnd(_pPreCacheOrder, _nIds, m_spWaveContext);
}

CWaveContext *CSoundContext_Mixer::Wave_GetContext()
{
	return m_spWaveContext;
}


CSC_WCIDInfo* CSoundContext_Mixer::GetWCIDInfo()
{
	return m_spWCIDInfo;
}

CSC_SFXDesc *CSoundContext_Mixer::GetSFXDesc(const char *_pSFXName)
{
	return m_spWaveContext->GetSFXDesc(_pSFXName);
}

class CDisplayVoiceSort
{
public:
	static int Compare(const void* _pContext, CSoundContext_Mixer::CVoice *_p0, CSoundContext_Mixer::CVoice *_p1)
	{
		if (_p0 < _p1)
			return -1;
		else if (_p0 > _p1)
			return 1;
		return 0;
	}
};

const char **CSoundContext_Mixer::GetDebugStrings()
{


	if (!m_DebugStrTempMixer)
	{
		m_DebugStrTempMixer = DNew(char *) char *[mcs_MaxDebugSounds];
		for (int i = 0; i < mcs_MaxDebugSounds - 1; ++i)
		{
			m_DebugStrTempMixer[i] = DNew(char ) char [256];
			m_DebugStrTempMixer[i][0] = 0; 
		}
		m_DebugStrTempMixer[mcs_MaxDebugSounds - 1] = NULL;
	}
	char **pStrings = m_DebugStrTempMixer;

	CMTime SampleTime;
	CMTime CPUTime;
	m_Mixer.Global_GetTimeStats(SampleTime, CPUTime);
	int CurrentVoice = 0;
	strncpy(pStrings[CurrentVoice++], CFStrF("Mixer nFreeVoices %02d, Free Memory %02d, SampleRate %.2f, Mixer CPU %3.2f%%", 
		m_Mixer.Voice_GetNumFree(), m_Mixer.GetFreeMemory(), m_LastSetSampleRate, m_Mixer.Stat_GetFrameTimePercent()*100.0f
		).Str(), 256);

	strncpy(pStrings[CurrentVoice++], "", 256);

	TArray<CSoundContext_Mixer::CVoice *> TempVoices;
	TempVoices.SetLen(32);
	for (int i = 0; i < m_lChannels.Len(); ++i)
	{
		CChannel *pChannel = m_lChannels[i];

		if (pChannel)
		{
			if (TempVoices.Len() < pChannel->m_nVoices)
				TempVoices.SetLen(pChannel->m_nVoices);

			int TempLen = TempVoices.Len();
			int iVoice = 0;

			strncpy(pStrings[CurrentVoice++], CFStrF("Channel %02d: %d/%d Free Vol %.2f Pitch %.2f", i, pChannel->m_nFreeVoices, pChannel->m_nVoices, pChannel->m_Volume, pChannel->m_Pitch).Str(), 256);

			{
				CVoiceIter Iter = pChannel->m_AlloctedVoices;

				while (Iter)
				{
					CSoundContext_Mixer::CVoice *pVoice = Iter;
					TempVoices[iVoice++] = pVoice;
					++Iter;
				
				}

				Iter = pChannel->m_FreeVoices;
				while (Iter)
				{
					CSoundContext_Mixer::CVoice *pVoice = Iter;
					TempVoices[iVoice++] = pVoice;
					++Iter;
				}
			}

			TempVoices.QSort<CDisplayVoiceSort>(iVoice);

			CFStr TempStr;
			const uint nCol = 4;
			int iStride = (iVoice + nCol-1) / nCol;
			for (int i = 0; i < iStride; ++i)
			{
				TempStr.Clear();
				for (int y = 0; y < nCol; ++y)
				{
					int iTest = y * iStride + i;
					if (iTest < iVoice)
					{
						CSoundContext_Mixer::CVoice *pVoice = TempVoices[iTest];
						if (pVoice->m_iMixerVoice != -1)
						{
							CMTime VoicePos = m_Mixer.Voice_GetPosition(pVoice->m_iMixerVoice);
							uint32 PauseFlags = pVoice->m_PauseSlot;
							bint bLoop = pVoice->m_bLooping;
							CWaveData WaveData;
							m_spWaveContext->GetWaveLoadedData(pVoice->m_WaveID, WaveData);
							fp32 Pitch = pVoice->m_Pitch * pVoice->m_Pitch_3D * pVoice->m_OriginalSampleRate;
							fp32 Volume = pVoice->m_Volume * pVoice->m_Volume_3D;
							if (pVoice->m_bIsMusic)
								Volume *= m_MusicVolume_Total;

							int iLocal;
							CWaveContainer_Plain *pContainer = (CWaveContainer_Plain *)m_spWaveContext->GetWaveContainer(pVoice->m_WaveID, iLocal);
							CStr Name = pContainer->GetName(iLocal);

							uint iCol = y;
							uint TabBase = iCol * 250;
							TempStr += CFStrF(
								"§t%.3d%-20s §t%.3d P:%.02f §t%.3d V:%.2f §t%.3d L:%d §t%.3d Pi:%.2f kHz", 
								TabBase+9, Name.Str(), 
								TabBase+120, VoicePos.GetTime(), 
								TabBase+155, Volume, 
								TabBase+183, bLoop, 
								TabBase+200, Pitch / 1000.0);

/*							if (y % 2 == 0)
							{
								TempStr = CFStrF(
									"§t009%-20s §t120 P:%.02f §t155 V:%.2f §t183 PF:0x%08x §t250 L:%d §t270 Pi:%.2f kHz", 
									Name.Str(), VoicePos.GetTime(), Volume, PauseFlags, bLoop, Pitch / 1000.0
								);
							}
							else
							{
								TempStr += CFStrF(
								"§t329| %-20s §t440 P:%.02f §t475 V:%.2f §t503 PF:0x%08x §t570 L:%d §t590 Pi:%.2f kHz", 
								Name.Str(), VoicePos.GetTime(), Volume, PauseFlags, bLoop, Pitch / 1000.0
								);
							}*/
			
						}
						else
						{
							uint iCol = y;
							uint TabBase = iCol * 250;
							TempStr += CFStrF("§t%.3d-", TabBase+9);

/*							if (y % 2 == 0)
							{
								TempStr = "§t009-";
							}
							else
							{
								TempStr += "§t329| -";
							}*/

						}
					}
				}
				strncpy(pStrings[CurrentVoice++], TempStr.Str(), 256);
				if (CurrentVoice >= 255)
					break;
			}
		
		}
		if (CurrentVoice >= 255)
			break;
	}

	for (int i = CurrentVoice; i < 256 - 1; ++i)
	{
		pStrings[i][0] = 0; 
	}

	return (const char **)pStrings;
}


void CSoundContext_Mixer::Internal_SetBind(uint32 _Type, CSC_Mixer_DSPChainInstance *_pChainInstance, uint32 _iDSP, uint32 _iParam, fp32 _Value)
{
	switch (_Type)
	{
	case EDSPParamType_int32:
		{
			int32 Value = TruncToInt(_Value);
			m_Mixer.DSPChainInstance_SetParams(_pChainInstance, _iDSP, _iParam, &Value, sizeof(int32));
		}
		break;
	case EDSPParamType_fp32:
		{
			m_Mixer.DSPChainInstance_SetParams(_pChainInstance, _iDSP, _iParam, &_Value, sizeof(fp32));
		}
		break;
	}
}

void CSoundContext_Mixer::Internal_SetBind(uint32 _Type, CSC_Mixer_DSPChainInstance *_pChainInstance, uint32 _iDSP, uint32 _iParam, int32 _Value)
{
	switch (_Type)
	{
	case EDSPParamType_int32:
		{
			m_Mixer.DSPChainInstance_SetParams(_pChainInstance, _iDSP, _iParam, &_Value, sizeof(int32));
		}
		break;
	case EDSPParamType_fp32:
		{
			fp32 Value = _Value;
			m_Mixer.DSPChainInstance_SetParams(_pChainInstance, _iDSP, _iParam, &Value, sizeof(fp32));
		}
		break;
	}
}

void CSoundContext_Mixer::Internal_SetBind(uint32 _Type, CSC_Mixer_DSPChainInstance *_pChainInstance, uint32 _iDSP, uint32 _iParam, uint16 *_pVolumeMatrix)
{
	switch (_Type)
	{
	case EDSPParamType_VolumeMatrix:
		{
			// This one is a little special. Only works when num source and dest channels is already set for only this type
			CSC_Mixer_DSP_VolumeMatrix::CParams *pParams = (CSC_Mixer_DSP_VolumeMatrix::CParams *)m_Mixer.DSPChainInstance_GetParamPtr(_pChainInstance, _iDSP);

			// Just set a reference to this value because we are constant
			uint32 nMatrix = AlignUp(pParams->m_nSourceChannels, 4) * AlignUp(pParams->m_nDestChannels, 4);
			uint32 nBytes = AlignUp(nMatrix * 2, 16);
			if (!pParams->m_pVolumeMatrix)
			{
				pParams->m_pVolumeMatrix = (vec128 *)CSC_Mixer_WorkerContext::CCustomAllocator::AllocAlign(nBytes, 16);
				CSC_Mixer_DSP_VolumeMatrix::CLastParams *pLastParams = (CSC_Mixer_DSP_VolumeMatrix::CLastParams *)m_Mixer.DSPChainInstance_GetLastParamPtr(_pChainInstance, _iDSP);
				pLastParams->m_pVolumeMatrix = (vec128 *)CSC_Mixer_WorkerContext::CCustomAllocator::AllocAlign(nBytes, 16);
				if (pLastParams->m_pVolumeMatrix)
				{
					uint32 nVec = nBytes >> 4;
					vec128 *pDst = pLastParams->m_pVolumeMatrix;
					if (pDst)
					{
						vec128 Zero = M_VZero();
						for (uint32 i = 0; i < nVec; ++i)
						{
							pDst[i] = Zero;
						}
					}
				}
			}
			if (pParams->m_pVolumeMatrix)
			{
				DataCopy((vec128*)pParams->m_pVolumeMatrix, (vec128 *)_pVolumeMatrix, nBytes >> 4);
			}
		}
		break;
	}
}

void CSoundContext_Mixer::Internal_GlobalBindingChanged(uint32 _Binding, fp32 _Value)
{
	CBindingEntryIter Iter = m_Bindings[_Binding].m_Entries;

	while (Iter)
	{
		CBindingEntry *pIter = Iter;
		CChainInstanceIter Iter2 = Iter->m_pChain->m_ChainInstances;

		while (Iter2)
		{		
			Internal_SetBind(pIter->m_iType, Iter2->m_pMixerDSPChainInstance, pIter->m_iDSP, pIter->m_iParam, _Value*pIter->m_Mul + pIter->m_Add);
			++Iter2;
		}


		++Iter;
	}
}

void CSoundContext_Mixer::Internal_GlobalBindingChanged(uint32 _Binding, int32 _Value)
{
	CBindingEntryIter Iter = m_Bindings[_Binding].m_Entries;

	while (Iter)
	{
		CBindingEntry *pIter = Iter;
		CChainInstanceIter Iter2 = Iter->m_pChain->m_ChainInstances;

		while (Iter2)
		{
			Internal_SetBind(pIter->m_iType, Iter2->m_pMixerDSPChainInstance, pIter->m_iDSP, pIter->m_iParam, _Value*pIter->m_Mul + pIter->m_Add);
			++Iter2;
		}

		++Iter;
	}
}

void CSoundContext_Mixer::Internal_GlobalBindingChanged(uint32 _Binding, uint16 * _pValue)
{
	CBindingEntryIter Iter = m_Bindings[_Binding].m_Entries;

	while (Iter)
	{
		CBindingEntry *pIter = Iter;
		CChainInstanceIter Iter2 = Iter->m_pChain->m_ChainInstances;

		while (Iter2)
		{
			Internal_SetBind(pIter->m_iType, Iter2->m_pMixerDSPChainInstance, pIter->m_iDSP, pIter->m_iParam, _pValue);
			++Iter2;
		}

		++Iter;
	}
}

void CSoundContext_Mixer::Internal_UpdateVoiceBindings(CVoice *_pVoice)
{
	CSC_DSPChain *pChain = _pVoice->m_DSPChainInstance.m_pDSPChain;
	
	TAP_RCD<CBindingEntry> pBinds = pChain->m_BindingEntries;
	
	for (uint32 i = 0; i < pBinds.Len(); ++i)
	{
		CBindingEntry &Entry = pBinds[i];
		if (Entry.m_pChain)
		{
			// Global bind
			switch (Entry.m_iBinding)
			{
			case EDSPBindingGlobal_NumChannels:
				{
					Internal_SetBind(Entry.m_iType, _pVoice->m_DSPChainInstance.m_pMixerDSPChainInstance, Entry.m_iDSP, Entry.m_iParam, int32(m_PlatformInfo.m_nChannels * Entry.m_Mul + Entry.m_Add));
				}
				break;
			}
		}
		else
		{
			// Voice bind
			switch (Entry.m_iBinding)
			{
			case EDSPBindingVoice_NumChannels:
				Internal_SetBind(Entry.m_iType, _pVoice->m_DSPChainInstance.m_pMixerDSPChainInstance, Entry.m_iDSP, Entry.m_iParam, int32(_pVoice->m_nChannels*Entry.m_Mul + Entry.m_Add));
				break;
			case EDSPBindingVoice_3DVolumeMatrix:
				break;
			case EDSPBindingVoice_Obstruction:
				Internal_SetBind(Entry.m_iType, _pVoice->m_DSPChainInstance.m_pMixerDSPChainInstance, Entry.m_iDSP, Entry.m_iParam, fp32(_pVoice->m_Occlusion.m_Obstruction*Entry.m_Mul + Entry.m_Add));
				break;
			case EDSPBindingVoice_Occlusion:
				Internal_SetBind(Entry.m_iType, _pVoice->m_DSPChainInstance.m_pMixerDSPChainInstance, Entry.m_iDSP, Entry.m_iParam, fp32(_pVoice->m_Occlusion.m_Occlusion*Entry.m_Mul + Entry.m_Add));
				break;
			}
		}

	}
}


void CSoundContext_Mixer::Internal_VoiceBindingChanged(CVoice *_pVoice, uint32 _Binding, fp32 _Value)
{
	CSC_DSPChain *pChain = _pVoice->m_DSPChainInstance.m_pDSPChain;
	CBindingEntryIter Iter = pChain->m_VoiceBindings[_Binding].m_Entries;
	while (Iter)
	{
		CBindingEntry &Entry = *Iter;
		// Voice bind
		Internal_SetBind(Entry.m_iType, _pVoice->m_DSPChainInstance.m_pMixerDSPChainInstance, Entry.m_iDSP, Entry.m_iParam, _Value*Entry.m_Mul + Entry.m_Add);
		++Iter;
	}
}

void CSoundContext_Mixer::Internal_VoiceBindingChanged(CVoice *_pVoice, uint32 _Binding, int32 _Value)
{
	CSC_DSPChain *pChain = _pVoice->m_DSPChainInstance.m_pDSPChain;
	CBindingEntryIter Iter = pChain->m_VoiceBindings[_Binding].m_Entries;
	while (Iter)
	{
		CBindingEntry &Entry = *Iter;
		// Voice bind
		Internal_SetBind(Entry.m_iType, _pVoice->m_DSPChainInstance.m_pMixerDSPChainInstance, Entry.m_iDSP, Entry.m_iParam, _Value*Entry.m_Mul + Entry.m_Add);
		++Iter;
	}
}

void CSoundContext_Mixer::Internal_VoiceBindingChanged(CVoice *_pVoice, uint32 _Binding, uint16 *_pValue)
{
	CSC_DSPChain *pChain = _pVoice->m_DSPChainInstance.m_pDSPChain;
	CBindingEntryIter Iter = pChain->m_VoiceBindings[_Binding].m_Entries;
	while (Iter)
	{
		CBindingEntry &Entry = *Iter;
		// Voice bind
		Internal_SetBind(Entry.m_iType, _pVoice->m_DSPChainInstance.m_pMixerDSPChainInstance, Entry.m_iDSP, Entry.m_iParam, _pValue);
		++Iter;
	}
}

void CSoundContext_Mixer::Con_Snd_Mix_Reload()
{
	Internal_InitCategories();
}

void CSoundContext_Mixer::Con_Snd_Mix_AutoReload(int32 _bAutoReload)
{
	m_bAutoReloadCategories = _bAutoReload;
}

void CSoundContext_Mixer::Register(CScriptRegisterContext &_RegContext)
{

	_RegContext.RegFunction("snd_mix_reload", this, &CSoundContext_Mixer::Con_Snd_Mix_Reload);
	_RegContext.RegFunction("snd_mix_autoreload", this, &CSoundContext_Mixer::Con_Snd_Mix_AutoReload);

}
