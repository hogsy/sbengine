#include "PCH.h"

#include "WObj_SoundVolume.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_PosHistory.h"

#include "MFloat.h"

// -------------------------------------------------------------------
// 
// SoundVolume
// 
// -------------------------------------------------------------------

void CWObject_SoundVolume::CClientData::Apply(const CSoundContext::CFilter *_pFilter)
{
	if(!_pFilter)
	{
		CSoundContext::CFilter Filter;
		Filter.SetDefault();

        Apply(&Filter);
		return;
	}

	m_DecayTime = M_Log(_pFilter->m_DecayTime); 
	m_DecayHFRatio = M_Log(_pFilter->m_DecayHFRatio);
	m_ReflectionsDelay = M_Log(_pFilter->m_ReflectionsDelay);
	m_ReverbDelay = M_Log(_pFilter->m_ReverbDelay);
	m_HFReference = M_Log(_pFilter->m_HFReference);
	m_EnvironmentSize = M_Log(_pFilter->m_EnvironmentSize);
	m_DecayLFRatio = M_Log(_pFilter->m_DecayLFRatio);
	m_EchoTime = M_Log(_pFilter->m_EchoTime);
	m_ModulationTime = M_Log(_pFilter->m_ModulationTime);
	m_LFReference = M_Log(_pFilter->m_LFReference);

	// Volumes
	m_Room = M_Pow(10.0f, _pFilter->m_Room * 0.0005f);
	m_Reflections = M_Pow(10.0f, _pFilter->m_Reflections * 0.0005f);
	m_Reverb = M_Pow(10.0f, _pFilter->m_Reverb * 0.0005f);
	m_RoomHF = M_Pow(10.0f, _pFilter->m_RoomHF * 0.0005f);
	m_RoomLF = M_Pow(10.0f, _pFilter->m_RoomLF * 0.0005f);

	m_RoomRolloffFactor = _pFilter->m_RoomRolloffFactor;
	m_ReverbDelay = _pFilter->m_ReverbDelay;
	m_Diffusion = _pFilter->m_Diffusion;
	m_Density = _pFilter->m_Density;

	m_EchoDepth = _pFilter->m_EchoDepth;
	m_ModulationDepth = _pFilter->m_ModulationDepth;
	m_AirAbsorptionHF = _pFilter->m_AirAbsorptionHF;
}

void CWObject_SoundVolume::CClientData::GetFilterSettings(CSoundContext::CFilter &_Dest)
{
	_Dest.m_Room = m_Room;
	_Dest.m_RoomHF = m_RoomHF;
	_Dest.m_RoomRolloffFactor = m_RoomRolloffFactor;
	_Dest.m_DecayTime = m_DecayTime;
	_Dest.m_DecayHFRatio = m_DecayHFRatio;
	_Dest.m_Reflections = m_Reflections;
	_Dest.m_ReflectionsDelay = m_ReflectionsDelay;
	_Dest.m_Reverb = m_Reverb;
	_Dest.m_ReverbDelay = m_ReverbDelay;
	_Dest.m_Diffusion = m_Diffusion;
	_Dest.m_Density = m_Density;
	_Dest.m_HFReference = m_HFReference;

	_Dest.m_EnvironmentSize = m_EnvironmentSize;
	_Dest.m_DecayLFRatio = m_DecayLFRatio;

	_Dest.m_EchoTime = m_EchoTime;
	_Dest.m_EchoDepth = m_EchoDepth;
	_Dest.m_ModulationTime = m_ModulationTime;
	_Dest.m_ModulationDepth = m_ModulationDepth;
	_Dest.m_AirAbsorptionHF = m_AirAbsorptionHF;
	_Dest.m_LFReference = m_LFReference;

	_Dest.m_RoomLF = m_RoomLF;
}


//
// Main purpose the set the sound object flag and default parameters
//
CWObject_SoundVolume::CWObject_SoundVolume()
{
	// Falloff
	memset(m_Data, 0, sizeof(m_Data));
	m_Data[0*2+1] = (100<<24); // Default falloff modifier
	m_Data[1*2+1] = (100<<24);
	m_Data[2*2+1] = (100<<24);
	m_Data[3*2+1] = (100<<24);

	m_iAnim0 = 128;		// Default falloff
	m_iAnim1 = 0;		// Default min-falloff

	CClientData *pCD = GetClientData();
	if(!pCD)
		Error_static("CWObject_SoundVolume::CWObject_SoundVolume", "Unable to get client data.");

	pCD->Apply(NULL);
	pCD->m_ApplyEffects = 0;
}

// -----------------------------------------------------------

CWObject_SoundVolume::CClientData *CWObject_SoundVolume::GetClientData(CWObject_CoreData* _pObj)
{
	if(!_pObj->m_lspClientObj[CLIENTOBJ_CLIENTDATA])
	{
		_pObj->m_lspClientObj[CLIENTOBJ_CLIENTDATA] = MNew(CClientData);
		if(!_pObj->m_lspClientObj[CLIENTOBJ_CLIENTDATA])
			Error_static("CWObject_GameCore::GetClientData", "Unable to create client objects.");
	}
	
	return (CWObject_SoundVolume::CClientData*)(CReferenceCount*)_pObj->m_lspClientObj[CLIENTOBJ_CLIENTDATA];
}

CWObject_SoundVolume::CClientData *CWObject_SoundVolume::GetClientData() { return GetClientData(this); }
const CWObject_SoundVolume::CClientData *CWObject_SoundVolume::GetClientData(const CWObject_CoreData* _pObj)
{ return GetClientData(const_cast<CWObject_CoreData *>(_pObj)); }

//
//
//
int CWObject_SoundVolume::OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pObj, uint8* _pData, int _Flags) const
{
	const CClientData *pCD = GetClientData(this);
	if(!pCD)
		Error_static("CWObject_SoundVolume::OnCreateClientUpdate", "Unable to pack client update.");
	
	int32 Flags = 0;
	if(_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT)
		Flags = CWO_CLIENTUPDATE_AUTOVAR;
	uint8* pD = _pData;
	pD += CWObject::OnCreateClientUpdate(_iClient, _pClObjInfo, _pObj, _pData, Flags);
	if (pD - _pData == 0)
		return pD - _pData;
	
	pCD->AutoVar_Pack(_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT, pD, m_pWServer->GetMapData());
	
	return pD - _pData;
}

//
//
//
int CWObject_SoundVolume::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	const uint8* pD = &_pData[CWObject::OnClientUpdate(_pObj, _pWClient, _pData, _Flags)];
	if (_pObj->m_iClass == 0 || pD - _pData == 0) return pD - _pData;
	
	if(_pObj->m_bAutoVarDirty)
		GetClientData(_pObj)->AutoVar_Unpack(pD, _pWClient->GetMapData());
	
	return (uint8*)pD - _pData;
}

//
//
//
void CWObject_SoundVolume::OnRefresh()
{
	CWObject::OnRefresh();
	m_DirtyMask |= GetClientData()->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
}

static CSoundContext::CFilter::CMinMax gs_MinMax;

//
// Get the directions and sounds for the object aswell as the falloff parameter
//
void CWObject_SoundVolume::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	CClientData *pCD = GetClientData();
	if(!pCD)
		Error_static("CWObject_SoundVolume::CWObject_SoundVolume", "Unable to get client data.");

	// BSP Model
	switch (_KeyHash)
	{
	case MHASH4('BSPM','ODEL','INDE','X'): // "BSPMODELINDEX"
		{
			CStr ModelName = CStrF("$WORLD:%d", _pKey->GetThisValuei());
	/*
			int XRMode = m_pWServer->Registry_GetGame()->GetValuei("XR_MODE", 0, 0);

			int32 iModel;
			if( XRMode == 1 )
				iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel2(ModelName);
			else if( XRMode == 2 )
				iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel3(ModelName);
			else
				iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
	*/		
			int iModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
			if (!iModel)
				Error("OnEvalKey", "Failed to acquire world-model.");

			if (!m_iModel[0])
				m_iModel[0] = iModel;

			// Setup physics
			CWO_PhysicsState Phys;
			Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_PHYSMODEL, iModel, 0, 0);
			Phys.m_nPrim = 1;

			Phys.m_PhysFlags = OBJECT_PHYSFLAGS_ROTATION;
			Phys.m_ObjectIntersectFlags = 0;
			Phys.m_ObjectFlags = OBJECT_FLAGS_SOUND;
			if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
				ConOutL("§cf80WARNING: Unable to set trigger physics state.");

			// Set bound-box.
			CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
			if (pModel)
			{
				CBox3Dfp32 Box;
				pModel->GetBound_Box(Box);
				m_pWServer->Object_SetVisBox(m_iObject, Box.m_Min, Box.m_Max);
			}
			else
			{
				ConOutL(CStrF("§cf80ERROR: failed getting model! (iModel = %d)", iModel));
			}
			break;
		}

	// Falloff parameter
	case MHASH2('FALL','OFF'): // "FALLOFF"
		{
			m_iAnim0 = _pKey->GetThisValuei();
			return;
			break;
		}
	case MHASH3('MINF','ALLO','FF'): // "MINFALLOFF"
		{
			m_iAnim1 = _pKey->GetThisValuei();
			break;
		}
	case MHASH3('APPL','Y FI','LTER'): // "APPLY FILTER"
		{
			pCD->m_ApplyEffects = _pKey->GetThisValuei();
			break;
		}

	// Filter effects
	case MHASH1('ROOM'): // "ROOM"
		{
			pCD->m_Room = M_Pow(10.0f, gs_MinMax.m_Room.Clamp(_pKey->GetThisValuef()) * 0.0005f);
			break;
		}
	case MHASH2('ROOM','HF'): // "ROOMHF"
		{
			pCD->m_RoomHF = M_Pow(10.0f, gs_MinMax.m_RoomHF.Clamp(_pKey->GetThisValuef()) * 0.0005f);
			break;
		}
	case MHASH5('ROOM','ROLL','OFFF','ACTO','R'): // "ROOMROLLOFFFACTOR"
		{
			pCD->m_RoomRolloffFactor = gs_MinMax.m_RoomRolloffFactor.Clamp(_pKey->GetThisValuef());
			break;
		}
	case MHASH3('DECA','YTIM','E'): // "DECAYTIME"
		{
			pCD->m_DecayTime = M_Log(gs_MinMax.m_DecayTime.Clamp(_pKey->GetThisValuef()));
			break;
		}
	case MHASH3('DECA','YHFR','ADIO'): // "DECAYHFRADIO"
		{
			pCD->m_DecayHFRatio = M_Log(gs_MinMax.m_DecayHFRatio.Clamp(_pKey->GetThisValuef()));
			break;
		}
	case MHASH3('DECA','YHFR','ATIO'): // "DECAYHFRATIO"
		{
			pCD->m_DecayHFRatio = M_Log(gs_MinMax.m_DecayHFRatio.Clamp(_pKey->GetThisValuef()));
			break;
		}
	case MHASH3('REFL','ECTI','ONS'): // "REFLECTIONS"
		{
			pCD->m_Reflections = M_Pow(10.0f, gs_MinMax.m_Reflections.Clamp(_pKey->GetThisValuei()) * 0.0005f);
			break;
		}
	case MHASH4('REFL','ECTI','ONSD','ELAY'): // "REFLECTIONSDELAY"
		{
			pCD->m_ReflectionsDelay = M_Log(gs_MinMax.m_ReflectionsDelay.Clamp(_pKey->GetThisValuef()) + 0.0001f);
			break;
		}
	case MHASH2('REVE','RB'): // "REVERB"
		{
			pCD->m_Reverb = M_Pow(10.0f, gs_MinMax.m_Reverb.Clamp(_pKey->GetThisValuef()) * 0.0005f);
			break;
		}
	case MHASH3('REVE','RBDE','LAY'): // "REVERBDELAY"
		{
			pCD->m_ReverbDelay = M_Log(gs_MinMax.m_ReverbDelay.Clamp(_pKey->GetThisValuef()) + 0.0001f);
			break;
		}
	case MHASH3('DIFF','USIO','N'): // "DIFFUSION"
		{
			pCD->m_Diffusion = gs_MinMax.m_Diffusion.Clamp(_pKey->GetThisValuef());
			break;
		}
	case MHASH2('DENS','ITY'): // "DENSITY"
		{
			pCD->m_Density = gs_MinMax.m_Density.Clamp(_pKey->GetThisValuef());
			break;
		}
	case MHASH3('HFRE','FERE','NCE'): // "HFREFERENCE"
		{
			pCD->m_HFReference = M_Log(gs_MinMax.m_HFReference.Clamp(_pKey->GetThisValuef()));
			break;
		}
	case MHASH4('ENVI','RONM','ENTS','IZE'): // "ENVIRONMENTSIZE"
		{
			pCD->m_EnvironmentSize = M_Log(gs_MinMax.m_EnvironmentSize.Clamp(_pKey->GetThisValuef()));
			break;
		}
	case MHASH3('DECA','YLFR','ATIO'): // "DECAYLFRATIO"
		{
			pCD->m_DecayLFRatio = M_Log(gs_MinMax.m_DecayLFRatio.Clamp(_pKey->GetThisValuef()));
			break;
		}
	case MHASH2('ECHO','TIME'): // "ECHOTIME"
		{
			pCD->m_EchoTime = M_Log(gs_MinMax.m_EchoTime.Clamp(_pKey->GetThisValuef()));
			break;
		}
	case MHASH3('ECHO','DEPT','H'): // "ECHODEPTH"
		{
			pCD->m_EchoDepth = gs_MinMax.m_EchoDepth.Clamp(_pKey->GetThisValuef());
			break;
		}
	case MHASH4('MODU','LATI','ONTI','ME'): // "MODULATIONTIME"
		{
			pCD->m_ModulationTime = M_Log(gs_MinMax.m_ModulationTime.Clamp(_pKey->GetThisValuef()));
			break;
		}
	case MHASH4('MODU','LATI','ONDE','PTH'): // "MODULATIONDEPTH"
		{
			pCD->m_ModulationDepth = gs_MinMax.m_ModulationDepth.Clamp(_pKey->GetThisValuef());
			break;
		}
	case MHASH4('AIRA','BSOR','BSIO','NHF'): // "AIRABSORBSIONHF"
		{
			pCD->m_AirAbsorptionHF = gs_MinMax.m_AirAbsorptionHF.Clamp(_pKey->GetThisValuef());
			break;
		}
	case MHASH3('LFRE','FERE','NCE'): // "LFREFERENCE"
		{
			pCD->m_LFReference = M_Log(gs_MinMax.m_LFReference.Clamp(_pKey->GetThisValuef()));
			break;
		}
	case MHASH2('ROOM','LF'): // "ROOMLF"
		{
			pCD->m_RoomLF = M_Pow(10.0f, gs_MinMax.m_RoomLF.Clamp(_pKey->GetThisValuef()) * 0.0005f);
			break;
		}

	default:
		{
			if(_pKey->GetThisName().Find("FALLOFF") != -1)
			{
				CFStr Str = _pKey->GetThisValue();
				int iVolume = _pKey->GetThisName().Copy(7, 1024).Val_int();

				if (iVolume > 0 && iVolume <= 4)
				{
					iVolume--; // make it zero based
					uint8 Falloff = Str.Val_int();
					m_Data[iVolume*2+1] = (Falloff<<24)|(m_Data[iVolume*2+1]&0xFFFFFF);
				}
				else
				{
					ConOutL(CStrF("WARNING: [CWObject_SoundVolume] incorrect FALLOFF index: %d", iVolume));
				}
			}

			// Sound Direction
			else if(_pKey->GetThisName().Find("DIRECTION") != -1)
			{
				CFStr Str = _pKey->GetThisValue();
				int iVolume = _pKey->GetThisName().Copy(9, 1024).Val_int();

				if (iVolume > 0 && iVolume <= 4)
				{
					iVolume--; // make it zero based

					CVec3Dfp32 Dir; // aquire direction
					Dir.k[0] = (fp32)Str.GetStrSep(",").Val_fp64();
					Dir.k[1] = (fp32)Str.GetStrSep(",").Val_fp64();
					Dir.k[2] = (fp32)Str.GetStrSep(",").Val_fp64();

					m_Data[iVolume*2+1] = Dir.Pack24(1.01f)|(m_Data[iVolume*2+1]&0xFF000000);
				}
				else
				{
					ConOutL(CStrF("WARNING: [CWObject_SoundVolume] incorrect DIRECTION index: %d", iVolume));
				}
			}

			// Sound
			else if(_pKey->GetThisName().Find("SOUND") != -1)
			{
				CFStr Str = _pKey->GetThisValue();
				int iVolume = _pKey->GetThisName().Copy(5, 1024).Val_int();

				if (iVolume > 0 && iVolume <= 4)
				{
					iVolume--; // make it zero based
					m_Data[iVolume*2] = m_pWServer->GetMapData()->GetResourceIndex_Sound(Str);
				}
				else
				{
					ConOutL(CStrF("WARNING: [CWObject_SoundVolume] incorrect SOUND index: %d", iVolume));
				}
				return;
			}
			else
				return CWObject::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

//
//
//
void CWObject_SoundVolume::OnFinishEvalKeys()
{
	CWObject::OnFinishEvalKeys();
	m_pWServer->Object_SetPhysics_ObjectFlags(m_iObject, GetPhysState().m_ObjectFlags | OBJECT_FLAGS_SOUND);
//	m_PhysState.m_ObjectFlags |= OBJECT_FLAGS_SOUND;
}

//
//
//
void CWObject_SoundVolume::OnSpawnWorld()
{
	CWObject::OnSpawnWorld();

	m_DirtyMask |= GetClientData()->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
}

aint CWObject_SoundVolume::OnClientMessage(CWObject_Client *_pObj, CWorld_Client *_pWClient, const CWObject_Message &_Msg)
{
	if(_Msg.m_Msg != 666)
		return 0;

	if(_Msg.m_DataSize != sizeof(CSoundContext::CFilter))
		return 0;

	CClientData *pCD = GetClientData(_pObj);
	if(!pCD)
		return 0;

	if(!pCD->m_ApplyEffects)
		return 0;
	
	pCD->GetFilterSettings(*((CSoundContext::CFilter*)_Msg.m_pData));
	return 1;
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_SoundVolume, CWObject, 0x0100);

