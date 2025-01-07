#include "PCH.h"
#include "../Models/WModel_EffectSystem.h"
#include "WObj_EffectSystem.h"

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_EffectSystem, CWObject_Model, 0x0100);


enum
{
	class_CharNPC =			MHASH5('CWOb','ject','_','Char','NPC'),
	class_CharPlayer =		MHASH6('CWOb','ject','_','Char','Play','er'),
	class_CharDarkling =	MHASH6('CWOb','ject','_','Char','Dark','ling'),
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_EffectSystem
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CWO_EffectSystem_ClientData::Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	m_pObj = _pObj;
	m_pWPhysState = _pWPhysState;
	m_pWServer = m_pWPhysState->IsServer() ? safe_cast<CWorld_Server>(m_pWPhysState) : NULL;
	m_pWClient = m_pWPhysState->IsClient() ? safe_cast<CWorld_Client>(m_pWPhysState) : NULL;

	m_LightGUID = 0;
	m_TimeScale = 1.0f;
	m_iTarget = 0;
	m_Flags = 0;
	m_iOwner = 0;
	m_iBoneAttach = 0;

	m_StartTick = 0;
	m_EndTick = 0;

	for(int i = 0; i < 3; i++)
		m_pModels[i] = NULL;

	m_AttachPoint.m_Value.m_iNode = 0xffff;
}


void CWO_EffectSystem_ClientData::SetTimeScale(fp32 _TimeScale)
{
	m_TimeScale = _TimeScale;
}


void CWO_EffectSystem_ClientData::SetWaitSpawn(bool _bWaitSpawn)
{
	m_Flags = (_bWaitSpawn) ? (m_Flags | FXOBJ_FLAGS_WAITSPAWN) : (m_Flags & ~FXOBJ_FLAGS_WAITSPAWN);
}


bool CWO_EffectSystem_ClientData::PauseSystem(int _PauseTick, fp32 _PauseFrac)
{
	if(!(m_Flags & FXOBJ_FLAGS_TIMEPAUSED))
	{
		m_Flags = m_Flags | FXOBJ_FLAGS_TIMEPAUSED;
		SetAnimTick(_PauseTick, _PauseFrac, true);
		return true;
	}

	return false;
}


bool CWO_EffectSystem_ClientData::UnpauseSystem()
{
	if(m_Flags & FXOBJ_FLAGS_TIMEPAUSED)
	{
		m_Flags = m_Flags & ~FXOBJ_FLAGS_TIMEPAUSED;
		SetAnimTick(m_pObj->m_CreationGameTick, m_pObj->m_CreationGameTickFraction);
		return true;
	}

	return false;
}


void CWO_EffectSystem_ClientData::SetAnimTick(int _Tick, fp32 _Frac, bool _bAbsolute)
{
	if(m_pWServer)
	{
		if(_bAbsolute)
		{
			m_pObj->m_CreationGameTick = _Tick;
			m_pObj->m_CreationGameTickFraction = _Frac;
		}
		else
			m_pObj->SetAnimTick(m_pWServer, _Tick, _Frac);
	}
}


int CWO_EffectSystem_ClientData::GetAnimTick(CWorld_Client* _pWClient)
{
	if(m_Flags & FXOBJ_FLAGS_TIMEPAUSED)
		return m_pObj->m_CreationGameTick;

	if(_pWClient)
		return m_pObj->GetAnimTick(_pWClient);
	if(m_pWServer)
		return m_pObj->GetAnimTick(m_pWServer);
	else if(m_pWClient)
		return m_pObj->GetAnimTick(m_pWClient);

	return 0;
}


fp32 CWO_EffectSystem_ClientData::GetAnimFrac()
{
	return m_pObj->GetAnimTickFraction();
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_EffectSystem
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWObject_EffectSystem::CWObject_EffectSystem()
{
	m_OwnerNameHash = 0;
	m_SelfDestruct = 0;
}


const CWO_EffectSystem_ClientData& CWObject_EffectSystem::GetClientData(const CWObject_CoreData* _pObj)
{
	const CReferenceCount* pData = _pObj->m_lspClientObj[0];
	M_ASSERT(pData, "Who deleted my client data?!");
	return *safe_cast<const CWO_EffectSystem_ClientData>(pData);
}


CWO_EffectSystem_ClientData& CWObject_EffectSystem::GetClientData(CWObject_CoreData* _pObj)
{
	CReferenceCount* pData = _pObj->m_lspClientObj[0];
	M_ASSERT(pData, "Who deleted my client data?!");
	return *safe_cast<CWO_EffectSystem_ClientData>(pData);
}


void CWObject_EffectSystem::OnCreate()
{
	CWObject_EffectSystemParent::OnCreate();

	CWO_EffectSystem_ClientData& CD = GetClientData(this);
	CD.m_iOwner = m_iOwner;

	if (m_iOwner)
		m_bNoSave = true;
}

void CWObject_EffectSystem::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	CWO_EffectSystem_ClientData& CD = GetClientData(this);
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	
	switch(_KeyHash)
	{
	case MHASH2('FXMO','DEL'): // "FXMODEL"
	case MHASH2('FXMO','DEL0'): // "FXMODEL0"
		{
			Model_Set(0, CStr("EffectSystem:") + KeyValue);
		}
		break;

	case MHASH2('FXMO','DEL1'): // "FXMODEL1"
		{
			Model_Set(1, CStr("EffectSystem:") + KeyValue);
		}
		break;

	case MHASH2('FXMO','DEL2'): // "FXMODEL2"
		{
			Model_Set(2, CStr("EffectSystem:") + KeyValue);
		}
		break;
	
	case MHASH4('SELF','_DES','TRUC','T'): // "SELF_DESTRUCT"
		{
			CD.m_Flags = CD.m_Flags | ((KeyValue.Val_int()) ? FXOBJ_FLAGS_SELFDESTRUCT : 0);
		}
		break;

	case MHASH3('BONE','_ATT','ACH'): // "BONE_ATTACH"
		{
			CD.m_Flags = CD.m_Flags | ((KeyValue.Val_int()) ? FXOBJ_FLAGS_BONEATTACH : 0);
			CD.m_iBoneAttach = (uint16)KeyValue.Val_int();
		}
		break;

	case MHASH3('ATTA','CHPO','INT'): // "ATTACHPOINT"
		{
			CD.m_AttachPoint.m_Value.Parse(KeyValue);
			CD.m_AttachPoint.MakeDirty();
		}
		break;

	case MHASH2('NO_C','ULL'): // "NO_CULL"
		{
			CD.m_Flags = CD.m_Flags | ((KeyValue.Val_int()) ? FXOBJ_FLAGS_NOCULL : 0);
		}
		break;

	case MHASH1('FADE'):
		{
			CD.m_Flags = CD.m_Flags | ((KeyValue.Val_int()) ? FXOBJ_FLAGS_FADE : 0);
		}
		break;

	case MHASH2('FX_O','WNER'):
		{
			CStr OwnerName = m_pWServer->World_MangleTargetName(KeyValue);
			m_OwnerNameHash = StringToHash(OwnerName.Str());
		}

	default:
		{
			CWObject_EffectSystemParent::OnEvalKey(_KeyHash, _pKey);
		}
		break;
	}
}


void CWObject_EffectSystem::OnFinishEvalKeys()
{
	CWObject_EffectSystemParent::OnFinishEvalKeys();

	// Determine client flags visibility
	ClientFlags() &= ~CWO_CLIENTFLAGS_VISIBILITY;
	for(uint8 i = 0; i < 3; i++)
	{
		CXR_Model_EffectSystem* pFXSystem = (CXR_Model_EffectSystem*)m_pWServer->GetMapData()->GetResource_Model(m_iModel[i]);
		if(pFXSystem && pFXSystem->NeedOnRenderVis())
		{
			ClientFlags() |= CWO_CLIENTFLAGS_VISIBILITY;
			break;
		}
	}

	SetAnimTick(m_pWServer, 0, 0);

	m_DirtyMask |= GetClientData(this).AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
}


void CWObject_EffectSystem::OnSpawnWorld()
{
	CWObject_EffectSystemParent::OnSpawnWorld();
	
	// Set owner if one hasn't already been assigned
	if (m_OwnerNameHash)
	{
		CWO_EffectSystem_ClientData& CD = GetClientData(this);

		int iOwner = m_pWServer->Selection_GetSingleTarget(m_OwnerNameHash);
		if (iOwner != m_iObject)
		{
			m_iOwner = iOwner;
			CD.m_iOwner = m_iOwner;
		}
		else
			m_OwnerNameHash = 0;
	}

	if (m_iOwner)
		m_bNoSave = true;
}


void CWObject_EffectSystem::OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	// Check if we need to (re)allocate client data
	CReferenceCount* pData = _pObj->m_lspClientObj[0];
	CWO_EffectSystem_ClientData* pCD = TDynamicCast<CWO_EffectSystem_ClientData>(pData);

	// Allocate clientdata
	if (!pCD || pCD->m_pObj != _pObj || pCD->m_pWPhysState != _pWPhysState)
	{
		pCD = MNew(CWO_EffectSystem_ClientData);
		if (!pCD)
			Error_static("CWObject_EffectSystem", "Could not allocate client data!");

		_pObj->m_lspClientObj[0] = pCD;
		pCD->Clear(_pObj, _pWPhysState);
	}
}


void CWObject_EffectSystem::SendImpulse(CWObject_EffectSystem* _pEffectSystem, uint _Impulse, const CStr& _String)
{
	_pEffectSystem->Impulse(_Impulse, _String.Str());
}


void CWObject_EffectSystem::Impulse(uint _Impulse, const char* _pString)
{
	{
		CWO_EffectSystem_ClientData& CD = GetClientData(this);
		CStr StringData = (_pString) ? _pString : "0";

		switch(_Impulse)
		{
		case FXIMPULSETYPE_TIMESCALE:
			{
				CD.m_TimeScale = (fp32)StringData.Val_fp64();
				break;
			}

		case FXIMPULSETYPE_ANIMTIME:
			{
				fp32 AnimTime = ((fp32)StringData.Val_fp64()) * m_pWServer->GetGameTickTime();
				int32 CompleteTick = TruncToInt(AnimTime);
				SetAnimTick(m_pWServer, CompleteTick, AnimTime - CompleteTick);
				break;
			}

		case FXIMPULSETYPE_PAUSETIME:
			{
				if(StringData.Val_int())
					CD.PauseSystem(GetAnimTick(m_pWServer), 0.0f);
				else
					CD.UnpauseSystem();
				break;
			}

		case FXIMPULSETYPE_WAITSPAWN:
			{
				CD.SetWaitSpawn(StringData.Val_int() ? true : false);
				break;
			}

		case FXIMPULSETYPE_TIMECONTROL:
			{
				const fp32 TimeScale = (fp32)StringData.GetStrSep(",").Val_fp64();
				const fp32 AnimTime = (fp32)StringData.GetStrSep(",").Val_fp64();
				const int32 CompleteTick = TruncToInt(AnimTime);

				CD.SetTimeScale(TimeScale);
				SetAnimTick(m_pWServer, CompleteTick, AnimTime - CompleteTick);
				break;
			}

		case FXIMPULSETYPE_SETTARGET_FROMNAME:
			{
				CD.m_iTarget = m_pWServer->Selection_GetSingleTarget(StringData);
				break;
			}

		case FXIMPULSETYPE_SETTARGET_FROMGUID:
			{
				CD.m_iTarget = m_pWServer->Object_GetIndex(StringData.Val_int());
				break;
			}

		case FXIMPULSETYPE_SETTARGET_FROMID:
			{
				CD.m_iTarget = StringData.Val_int();
				break;
			}

		case FXIMPULSETYPE_SETOWNER:
			{
				CStr OwnerName = m_pWServer->World_MangleTargetName(StringData);
				m_OwnerNameHash = StringToHash(OwnerName.Str());
				if (m_OwnerNameHash)
				{
					int iOwner = m_pWServer->Selection_GetSingleTarget(m_OwnerNameHash);
					if (iOwner != m_iObject)
					{
						m_iOwner = iOwner;
						CD.m_iOwner = m_iOwner;

						if (m_iOwner)
							m_bNoSave = true;
					}
				}

				break;
			}
		}
	}
}


aint CWObject_EffectSystem::OnMessage(const CWObject_Message& _Msg)
{
	CWO_EffectSystem_ClientData& CD = GetClientData(this);

	switch(_Msg.m_Msg)
	{
	case OBJMSG_EFFECTSYSTEM_IMPULSE:
		{
			Impulse(_Msg.m_Param0, (const char*)_Msg.m_pData);
		}
		return 1;

	case OBJMSG_EFFECTSYSTEM_SETFADE:
		{
			switch(_Msg.m_Param0)
			{
			case FXFADE_IN:
				{
					uint8 Flags = (CD.m_Flags | FXOBJ_FLAGS_FADEIN) & ~FXOBJ_FLAGS_FADEOUT;
					if (CD.m_Flags.m_Value != Flags)
						CD.m_Flags = Flags;
					break;
				}

			case FXFADE_OUT:
				{
					uint8 Flags = (CD.m_Flags | FXOBJ_FLAGS_FADEOUT) & ~FXOBJ_FLAGS_FADEIN;
					if (CD.m_Flags.m_Value != Flags)
						CD.m_Flags = Flags;
					break;
				}

			default:
				{
					uint8 Flags = CD.m_Flags & ~(FXOBJ_FLAGS_FADEOUT | FXOBJ_FLAGS_FADEIN);
					if (CD.m_Flags.m_Value != Flags)
						CD.m_Flags = Flags;
					break;
				}
			}				
		}
		return 1;

	case OBJMSG_EFFECTSYSTEM_SELFDESTRUCT:
		{
			m_SelfDestruct = m_pWServer->GetGameTick() + TruncToInt(fp32(_Msg.m_Param0 / 1000) * m_pWServer->GetGameTicksPerSecond());
		}
		return 1;

		/*
		//case OBJMSG_EFFECTSYSTEM_SETTIMECONTROL:
		case OBJMSG_EFFECTSYSTEM_SETTIMESCALE:
		{
			fp32 TimeScale = 0.0f;
			if(_Msg.m_Param0)
				TimeScale = 1.0f / *((fp32*)_Msg.m_Param0);

			CD.m_TimeScale = TimeScale;

			SetAnimTick(m_pWServer, (int32)_Msg.m_Param1, 0.0f);
			CD.m_Flags = CD.m_Flags & ~FXOBJ_FLAGS_TIMEPAUSED;
		}
		return 1;

		case OBJMSG_EFFECTSYSTEM_PAUSETIME:
		{
			fp32 TimeFrac = 0.0f;
			if(_Msg.m_Param0)
				TimeFrac = MaxMT(0.0f, *((fp32*)_Msg.m_Param0));

            SetAnimTick(m_pWServer, (int32)(m_pWServer->GetGameTick() - _Msg.m_Param1), 0.0f);
			CD.m_Flags = CD.m_Flags | FXOBJ_FLAGS_TIMEPAUSED;
		}
		return 1;

		case OBJMSG_EFFECTSYSTEM_WAITSPAWN:
		{
			if (_Msg.m_Param0 == 0)
				CD.m_Flags = CD.m_Flags & ~FXOBJ_FLAGS_WAITSPAWN;
			else if (_Msg.m_Param0 == 1)
				CD.m_Flags = CD.m_Flags | FXOBJ_FLAGS_WAITSPAWN;
		}
		return 1;
		*/
	}

	return CWObject_EffectSystemParent::OnMessage(_Msg);
}


aint CWObject_EffectSystem::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	CWO_EffectSystem_ClientData& CD = GetClientData(_pObj);

	switch(_Msg.m_Msg)
	{
		case OBJMSG_EFFECTSYSTEM_EXCLUDEOWNER:
		{
			CD.m_LightGUID = (uint16)_Msg.m_Param0;
			return 1;
		}

		case OBJMSG_EFFECTSYSTEM_GETLIGHTGUID:
		{
			if(_Msg.m_Param0)
				*(uint16*)_Msg.m_Param0 = CD.m_LightGUID;

			return 1;
		}
	}

	return CWObject_EffectSystemParent::OnClientMessage(_pObj, _pWClient, _Msg);
}


void CWObject_EffectSystem::OnRefresh()
{
	CWO_EffectSystem_ClientData& CD = GetClientData(this);

	m_DirtyMask |= CD.AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
	if(CD.m_Flags & FXOBJ_FLAGS_WAITSPAWN)
		return;

	// Self destruct
	if (m_SelfDestruct != 0 && m_pWServer->GetGameTick() >= m_SelfDestruct)
		m_pWServer->Object_Destroy(m_iObject);

	// Self destruct, test if it's time to destroy ourself.
	if(CD.m_Flags & FXOBJ_FLAGS_SELFDESTRUCT)
	{
		const int32 AnimTick = GetAnimTick(m_pWServer);
		bool bDestroy = true;
		//bool bHasModel = false;
		for(uint8 i = 0; i < 3; i++)
		{
			CXR_Model* pModel = (m_iModel[i]) ? m_pWServer->GetMapData()->GetResource_Model(m_iModel[i]) : NULL;
			if(pModel)
			{
				const fp32 ModelDuration = *(fp32*)pModel->GetParam(CXR_MODEL_PARAM_ANIM);
				const fp32 TicksPerSec = m_pWServer->GetGameTicksPerSecond();
				const int32 DestroyTick = TruncToInt(ModelDuration * TicksPerSec)+1;
				if(DestroyTick >= 0 && AnimTick < DestroyTick)
				{
					bDestroy = false;
					break;
				}
			}
		}

		if(bDestroy)
			m_pWServer->Object_Destroy(m_iObject);
	}

	// Handle owner stuff
	CWObject* pOwner = (m_iOwner) ? m_pWServer->Object_Get(m_iOwner) : NULL;
	if(pOwner)
	{
		m_pWServer->Object_SetPositionNoIntersection(m_iObject, pOwner->GetPositionMatrix());
	}
}


int CWObject_EffectSystem::OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const
{
	int Flags = CWO_CLIENTUPDATE_EXTRADATA;
	if(_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT)
		Flags |= CWO_CLIENTUPDATE_AUTOVAR;

	uint8* pD = _pData;
	pD += CWObject_EffectSystemParent::OnCreateClientUpdate(_iClient, _pClObjInfo, _pOld, _pData, Flags);
	if((pD - _pData) == 0)
		return 0;

	const CWO_EffectSystem_ClientData& CD = GetClientData(this);
	CD.AutoVar_Pack((_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT), pD, m_pWServer->GetMapData());
	
	return(pD - _pData);
}


int CWObject_EffectSystem::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	CBox3Dfp32 BBox;
	_pObj->GetVisBoundBox(BBox);

	int16 liModels[CWO_NUMMODELINDICES];
	for (uint i = 0; i < CWO_NUMMODELINDICES; i++)
		liModels[i] = _pObj->m_iModel[i];

	const uint8* pD = _pData;
	pD += CWObject_EffectSystemParent::OnClientUpdate(_pObj, _pWClient, _pData, _Flags);
	if(_pObj->m_iClass == 0 || (pD - _pData) == 0)
		return 0;

	CWO_EffectSystem_ClientData& CD = GetClientData(_pObj);

	if (_pObj->m_bAutoVarDirty)
	{
		CD.AutoVar_Unpack(pD, _pWClient->GetMapData(), 0);

		for(uint i = 0; i < CWO_NUMMODELINDICES; i++)
		{
			if (liModels[i] != _pObj->m_iModel[i])
			{
				CD.m_pModels[i] = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
				if (_pObj->m_iModel[i] && !CD.m_pModels[i])
				{
					M_TRACEALWAYS("Error retriving model resource (%d) in CWObject_EffectSystem! But why!!!!?!!\n", _pObj->m_iModel[i]);
				}
			}
		}

		if (CD.m_Flags & FXOBJ_FLAGS_FADE)
		{
			int32 GameTick = _pWClient->GetGameTick();
			int32 GameTicksPerSec = (int32)TruncToInt(_pWClient->GetGameTicksPerSecond());

			for(uint8 i = 0; i < 3; i++)
			{
				if (CD.m_Flags & FXOBJ_FLAGS_FADEOUT)
				{
					// Turned off nearby
					if(GameTick > CD.m_EndTick + GameTicksPerSec)
						CD.m_EndTick = GameTick;
					else
						CD.m_EndTick = GameTick;
				}
				else if(CD.m_Flags & FXOBJ_FLAGS_FADEIN)
				{
					int32 DiffVal = GameTick - CD.m_EndTick;
					if(DiffVal > GameTicksPerSec)
						CD.m_StartTick = GameTick;
                    //else
					//	CD.m_StartTick -= DiffVal;
				}
				else
				{
					CD.m_StartTick = 0;
					CD.m_EndTick = 0;
				}
			}
		}
	}

	_pWClient->Object_SetVisBox(_pObj->m_iObject, BBox.m_Min, BBox.m_Max);

	return (pD - _pData);
}


void CWObject_EffectSystem::IncludeEffectFromKey(const CStr Key, CRegistry *pReg, CMapData *_pMapData)
{
	MAUTOSTRIP(CWObject_EffectSystem_IncludeEffectFromKey, MAUTOSTRIP_VOID);
	if(pReg)
	{
		CRegistry *pChild = pReg->FindChild(Key);
		if(pChild)
			_pMapData->GetResourceIndex_Model(CStr("EffectSystem:") + pChild->GetThisValue());
	}
}


void CWObject_EffectSystem::OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer)
{
	IncludeEffectFromKey("FXMODEL", _pReg, _pMapData);
	IncludeEffectFromKey("FXMODEL0", _pReg, _pMapData);
	IncludeEffectFromKey("FXMODEL1", _pReg, _pMapData);
	IncludeEffectFromKey("FXMODEL2", _pReg, _pMapData);
}


void CWObject_EffectSystem::Model_Set(uint8 _iPos, const char* _pValue)
{
	iModel(_iPos) = m_pWServer->GetMapData()->GetResourceIndex_Model(_pValue);
}


void CWObject_EffectSystem::OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MSCOPESHORT(CWObject_EffectSystem::OnClientRenderVis);
	// TODO: AnimTime just looks plain stupid and ugly. Remove this and replace with an animstate pherhaps.

	// Add effect systems for on client render vis
	CWO_EffectSystem_ClientData& CD = GetClientData(_pObj);
	
	if(CD.m_Flags & FXOBJ_FLAGS_WAITSPAWN)
		return;

	const int32 AnimTick = CD.GetAnimTick(_pWClient);
	const fp32 IPTime = (CD.m_Flags & FXOBJ_FLAGS_TIMEPAUSED) ? CD.GetAnimFrac() : _pWClient->GetRenderTickFrac();
	const CMat4Dfp32& Pos = _pObj->GetPositionMatrix();
	const fp32 TimeScale = CD.m_TimeScale;
	
	CXR_SceneGraphInstance* pSGI = _pWClient->World_GetSceneGraphInstance();
	fp32 AnimTime = 0;
	
    for(uint8 i = 0; i < 3; i++)
	{
		//CXR_Model* pModel = (_pObj->m_iModel[i] != 0) ? _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]) : NULL;
		CXR_Model* pModel = CD.m_pModels[i];
		if(pModel)
		{
			if(pModel->GetParam(CXR_MODEL_PARAM_TIMEMODE) == CXR_MODEL_TIMEMODE_CONTINUOUS)
				AnimTime = _pWClient->GetGameTime().GetTime() + _pWClient->GetInterpolateTime();
			else
			{
				//AnimTime = (_pWClient->GetGameTickTime() * AnimTick) + IPTime;
				//AnimTime = (((_pWClient->GetGameTickTime() * TimeScale) * AnimTick) + (IPTime * _pWClient->GetGameTickTime() * TimeScale));
				AnimTime = CMTime::CreateFromTicks(AnimTick, _pWClient->GetGameTickTime() * TimeScale, IPTime).GetTime();

				// Paused time
				//if(CD.m_Flags & FXOBJ_FLAGS_TIMEPAUSE)

				//if(CD.m_PauseFrac >= 0)
				//	AnimTime = CMTime::CreateFromTicks(CD.m_PauseTick, _pWClient->GetGameTickTime(), CD.m_PauseFrac).GetTime();

				//if(CD.m_Flags & FXOBJ_FLAGS_TIMEPAUSED)
				//	AnimTime = CMTime::CreateFromTicks(_pWClient->GetGameTick() - AnimTick, _pWClient->GetGameTickTime(), _pObj->GetAnimTickFraction()).GetTime();
			}

			CXR_Model_EffectSystem::OnClientRenderVis(_pWClient, (CXR_Model_EffectSystem*)pModel, pSGI, AnimTime, Pos, _pObj->m_iObject);
		}
	}
}


void CWObject_EffectSystem::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	CWO_EffectSystem_ClientData& CD = GetClientData(_pObj);

	if(CD.m_Flags & FXOBJ_FLAGS_WAITSPAWN)
		return;

	CMat4Dfp32 Pos = _pObj->GetPositionMatrix();
	const int32 AnimTick = CD.GetAnimTick(_pWClient); //_pObj->GetAnimTick(_pWClient);
	const fp32 TimeScale = CD.m_TimeScale;
	
	int OnRenderFlags = 0;

	CXR_AnimState AnimState;
	AnimState.m_AnimAttr0 = (CD.m_Flags & FXOBJ_FLAGS_TIMEPAUSED) ? CD.GetAnimFrac() : _pWClient->GetRenderTickFrac(); //GetInterpolateTime();	// [0...1]
	
	if(CD.m_Flags & FXOBJ_FLAGS_NOCULL)
		OnRenderFlags |= CXR_MODEL_ONRENDERFLAGS_NOCULL;

	int32 GameTick = _pWClient->GetGameTick();
	fp32 GameTickTime = _pWClient->GetGameTickTime();
	fp32 RenderTickFrac = _pWClient->GetRenderTickFrac();
	int32 GameTicksPerSec = (int32)TruncToInt(_pWClient->GetGameTicksPerSecond());

	// Add effect systems for render
	for(uint8 i = 0; i < 3; i++)
	{
		//CXR_Model* pModel = (_pObj->m_iModel[i] != 0) ? _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]) : NULL;
		CXR_Model* pModel = CD.m_pModels[i];
		if(pModel)
		{
			//if((CD.m_Flags & FXOBJ_FLAGS_FADE) && !(CD.m_EndTick[i] != 0 && ((_pWClient->GetGameTick() - CD.m_EndTick[i]) < _pWClient->GetGameTicksPerSecond())))
			//	continue;

			AnimState.m_AnimTime0 = CMTime::CreateFromTicks(AnimTick, GameTickTime * TimeScale, AnimState.m_AnimAttr0);
			if(pModel->GetParam(CXR_MODEL_PARAM_TIMEMODE) == CXR_MODEL_TIMEMODE_CONTINUOUS)
				AnimState.m_AnimTime1.Reset();
			else
				AnimState.m_AnimTime1 = CMTime::CreateFromTicks(GameTick, GameTickTime * TimeScale, AnimState.m_AnimAttr0);

			if (CD.m_Flags & FXOBJ_FLAGS_FADE)
			{
				if(CD.m_StartTick == 0 && CD.m_EndTick == 0)
					continue;

				Pos.Unit3x3();
				AnimState.m_AnimTime0 = CMTime::CreateFromTicks(GameTick - CD.m_StartTick, GameTickTime, RenderTickFrac);

				if (CD.m_Flags & FXOBJ_FLAGS_FADEOUT)
				{
					int32 EndTick = Max(GameTick - CD.m_StartTick, (CD.m_EndTick - CD.m_StartTick) + GameTicksPerSec);
					AnimState.m_AnimTime1 = CMTime::CreateFromTicks(EndTick, GameTickTime, -RenderTickFrac);
				}
				else
				{
					int32 EndTick = Max(GameTick - CD.m_StartTick, (CD.m_EndTick - CD.m_StartTick) + GameTicksPerSec);
					AnimState.m_AnimTime1 = CMTime::CreateFromTicks(GameTick, GameTickTime, RenderTickFrac);
					/*
					int32 EndTick = Max((int32)0,
					_pWClient->GetGameTick() - CD.m_StartTick + 
					(_pWClient->GetGameTick() - CD.m_EndTick));// * 2 - CD.m_Offset[i]);
					AnimState.m_AnimTime1 = CD.m_EndTick > 0 ? CMTime::CreateFromTicks(EndTick,_pWClient->GetGameTickTime(),_pWClient->GetRenderTickFrac()) : CMTime::CreateInvalid();
					*/
				}
			}
			
			// Setup skeleton instance if needed
			if(CD.m_Flags & FXOBJ_FLAGS_BONEATTACH || CD.m_AttachPoint.m_Value.m_iNode != 0xffff)
			{
				CWObject_CoreData* pOwner = _pWClient->Object_GetCD(CD.m_iOwner);
				if(pOwner && (pOwner->IsClass(class_CharNPC) || pOwner->IsClass(class_CharPlayer) || pOwner->IsClass(class_CharDarkling)))
				{
					if (CD.m_AttachPoint.m_Value.m_iNode != 0xffff)
						SetupAttachPoint(CD.m_AttachPoint.m_Value, Pos, pOwner, _pWClient, RenderTickFrac);
					else
						SetupBoneAttachData(pOwner, _pWClient, AnimState, CD.m_iBoneAttach, RenderTickFrac);
				}
			}

			// Run RenderClient on effect objects
			AnimState.m_pModelInstance = _pObj->m_lModelInstances[i];
			CXR_Model_EffectSystem::OnClientRender((CXR_Model_EffectSystem*)pModel, _pWClient, AnimState.m_AnimTime0, Pos);

			// Add model for rendering
			_pEngine->Render_AddModel(pModel, Pos, AnimState, XR_MODEL_STANDARD, OnRenderFlags);
		}
	}
}


void CWObject_EffectSystem::ShaderUseAnimTime1(CXR_AnimState& _AnimState, bool _bUseAnimTime1)
{
	if (_bUseAnimTime1)
		_AnimState.m_Data[FXANIM_DATA_FLAGS] &= ~FXANIM_FLAGS_ANIMTIME0;
	else
		_AnimState.m_Data[FXANIM_DATA_FLAGS] |= FXANIM_FLAGS_ANIMTIME0;
}


bool CWObject_EffectSystem::EvaluateOwnerSkeleton(CWObject_CoreData* _pObj, CWorld_Client* _pWClient, CXR_Skeleton*& _pSkeleton, CXR_SkeletonInstance*& _pSkeletonInstance, fp32 _IPTime, CMat4Dfp32* _pBasePos)
{
	bool bResult = false;
	if (_pObj && _pWClient)
	{
		CXR_AnimState TempAnimState;
		CXR_Skeleton* pSkel = NULL;
		CXR_SkeletonInstance* pSkelInst = NULL;

		if (_pBasePos)
			bResult = CWObject_Character::GetEvaluatedPhysSkeleton(_pObj, _pWClient, pSkelInst, pSkel, TempAnimState, _IPTime, _pBasePos);
		else
		{
			CMat4Dfp32 BasePos;
			Interpolate2(_pObj->GetLocalPositionMatrix(), _pObj->GetPositionMatrix(), BasePos, _IPTime);
			bResult = CWObject_Character::GetEvaluatedPhysSkeleton(_pObj, _pWClient, pSkelInst, pSkel, TempAnimState, _IPTime, &BasePos);
		}

		_pSkeleton = pSkel;
		_pSkeletonInstance = pSkelInst;
	}

	return bResult;
}


bool CWObject_EffectSystem::SetupAttachPoint(const CFXAttachPoint& _AttachPoint, CMat4Dfp32& _ResultPos, CWObject_CoreData* _pObj, CWorld_Client* _pWClient, fp32 _IPTime, CMat4Dfp32* _pBasePos)
{
	CXR_Skeleton* pSkel = NULL;
	CXR_SkeletonInstance* pSkelInst = NULL;
	if (EvaluateOwnerSkeleton(_pObj, _pWClient, pSkel, pSkelInst, _IPTime, _pBasePos))
	{
		int iRotTrack = _AttachPoint.m_iNode;
		const CMat4Dfp32& BoneTransform = pSkelInst->m_pBoneTransform[iRotTrack];
		_AttachPoint.m_LocalPos.Multiply(BoneTransform, _ResultPos);
		return true;
	}

	return false;
}


bool CWObject_EffectSystem::SetupBoneAttachData(CWObject_CoreData* _pObj, CWorld_Client* _pWClient, CXR_AnimState& _AnimState, uint8 _iBoneAttach, fp32 _IPTime, CMat4Dfp32* _pBasePos)
{
	// Setup animstate
	CXR_Skeleton* pSkel = NULL;
	CXR_SkeletonInstance* pSkelInst = NULL;
	if (EvaluateOwnerSkeleton(_pObj, _pWClient, pSkel, pSkelInst, _IPTime, _pBasePos))
	{
		_AnimState.m_pSkeletonInst = pSkelInst;
		_AnimState.m_Data[FXANIM_DATA_FLAGS] |= FXANIM_FLAGS_USESKELETON;
		_AnimState.m_Data[FXANIM_DATA_FLAGS] &= ~FXANIM_FLAGS_USEBOXES;
		_AnimState.m_Data[FXANIM_DATA_SKELETONTYPE] = _iBoneAttach;
		_AnimState.m_Data[FXANIM_DATA_SKELETON] = (aint)pSkel;

		return true;
	}

	return false;
}


