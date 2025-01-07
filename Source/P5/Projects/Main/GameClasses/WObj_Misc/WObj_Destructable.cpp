#include "PCH.h"
#include "WObj_Destructable.h"

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Destructable, CWObject_Explosion, 0x0100);

void CWObject_Destructable::OnCreate()
{
	CWObject_Explosion::OnCreate();

	m_Hitpoints = 1;
	m_DamageImmunity = 0;
}

void CWObject_Destructable::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr Val = _pKey->GetThisValue();
	switch (_KeyHash)
	{
	case MHASH2('MODE','L'): // "MODEL"
		{
			m_Data[3] = m_pWServer->GetMapData()->GetResourceIndex_Model(Val);
			break;
		}

	case MHASH2('MODE','L3'): // "MODEL3"
		{
			CRegistry_Dynamic Reg;
			Reg.SetThisKey("MODEL", Val);
			CWObject_Explosion::OnEvalKey(Reg.GetThisNameHash(), &Reg);
			break;
		}

	case MHASH4('FIRS','TMOD','ELDE','LAY'): // "FIRSTMODELDELAY"
		{
			m_Data[4] = (int32)(Val.Val_int() * m_pWServer->GetGameTicksPerSecond());
			break;
		}

	case MHASH3('HITP','OINT','S'): // "HITPOINTS"
		{
			m_Hitpoints = Val.Val_int();
			break;
		}

	case MHASH4('DAMA','GE_I','MMUN','ITY'): // "DAMAGE_IMMUNITY"
		{
			m_DamageImmunity = (uint32)Val.TranslateFlags(CRPG_Object::ms_DamageTypeStr);
			break;
		}
	default:
		{
			CWObject_Explosion::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_Destructable::UpdateVisibility(int *_lpExtraModels, int _nExtraModels)
{
	if(m_iAnim0 == 0)
	{
		// Call parent UpdateVisibility with pre-explosion model only
		int i;
		int16 liModel[CWO_NUMMODELINDICES];
		for(i = 0; i < CWO_NUMMODELINDICES; i++)
		{
			liModel[i] = m_iModel[i];
			m_iModel[i] = 0;
		}

		int Model = m_Data[3];
		CWObject_Explosion::UpdateVisibility(&Model, 1);
		for(i = 0; i < CWO_NUMMODELINDICES; i++)
			m_iModel[i] = liModel[i];
	}
	else
		CWObject_Explosion::UpdateVisibility();
}

aint CWObject_Destructable::OnMessage(const CWObject_Message &_Msg)
{
	if(_Msg.m_Msg == OBJMSG_DAMAGE)
	{
		const CWO_DamageMsg *pMsg = CWO_DamageMsg::GetSafe(_Msg);
		if(pMsg)
		{
			if (!(m_DamageImmunity & pMsg->m_DamageType))
			{
				m_Hitpoints -= pMsg->m_Damage;
				if(m_Hitpoints < 0)
					SpawnExplosion(_Msg.m_iSender);
			}
		}
		return 1;
	}
	else if(_Msg.m_Msg == OBJMSG_RADIALSHOCKWAVE)
	{
		const CWO_ShockwaveMsg *pMsg = CWO_ShockwaveMsg::GetSafe(_Msg);
		if(pMsg)
		{	
			if (!(m_DamageImmunity & pMsg->m_DamageType))
			{
				CVec3Dfp32 Dim = GetPhysState().m_Prim[0].GetDim();
				fp32 h = Dim.k[2] * 2.0f;
				fp32 r = Min(Dim.k[0], Dim.k[1]);
				CVec3Dfp32 TracePos = GetPosition();

				CWO_DamageMsg Msg;
				if(pMsg->GetTracedDamage(m_iObject, TracePos, r, h, m_pWServer, Msg))
				{
					m_Hitpoints -= Msg.m_Damage;
					if(m_Hitpoints < 0)
						SpawnExplosion(_Msg.m_iSender);
					return 1;
				}
			}
		}
		return 1;
	}

	else
		return CWObject_Explosion::OnMessage(_Msg);
}

void CWObject_Destructable::SpawnExplosion(int _iSender, const aint* _pParam, int _nParam)
{
	CWO_PhysicsState Phys;
	Phys.m_nPrim = 0;
	m_pWServer->Object_SetPhysics(m_iObject, Phys);

	CWObject_Explosion::SpawnExplosion(_iSender, _pParam, _nParam);
}

void CWObject_Destructable::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	int32 Tick = _pObj->GetAnimTick(_pWClient) + (int)_pWClient->GetRenderTickFrac();
	if(_pObj->m_Data[3] != 0 && (_pObj->m_iAnim0 == 0 || _pObj->m_Data[4] > Tick))
	{
		CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_Data[3]);
		if(pModel)
		{
			CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
			AnimState.m_pModelInstance = GetClientData(_pObj)->m_spInstance1;
			AnimState.m_iObject = _pObj->m_iObject;
			_pEngine->Render_AddModel(pModel, _pObj->GetPositionMatrix(), AnimState);
		}
	}
	CWObject_Explosion::OnClientRender(_pObj, _pWClient, _pEngine, _ParentMat);
}

void CWObject_Destructable::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	CWObject_DestructableParent::OnDeltaLoad(_pFile,_Flags);
	// Hmrs, save some stuff
	_pFile->ReadLE(m_Data[3]);
	_pFile->ReadLE(m_Data[4]);
	_pFile->ReadLE(m_iAnim0);
	int16 nPrim = 0;
	_pFile->ReadLE(nPrim);
	if (!nPrim)
	{
		CWO_PhysicsState Phys;
		Phys.m_nPrim = 0;
		m_pWServer->Object_SetPhysics(m_iObject, Phys);
	}
	int32 CFlags;
	_pFile->ReadLE(CFlags);
	ClientFlags() = CFlags;
	_pFile->ReadLE(m_CreationGameTick);
}

void CWObject_Destructable::OnDeltaSave(CCFile* _pFile)
{
	CWObject_DestructableParent::OnDeltaSave(_pFile);
	_pFile->WriteLE(m_Data[3]);
	_pFile->WriteLE(m_Data[4]);
	_pFile->WriteLE(m_iAnim0);
	_pFile->WriteLE(GetPhysState().m_nPrim);
	// Flags
	_pFile->WriteLE(ClientFlags());
	_pFile->WriteLE(m_CreationGameTick);
}


class CWObject_Mine : public CWObject_Destructable
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	void OnCreate()
	{
		CWObject_Destructable::OnCreate();
		
		ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
		m_IntersectNotifyFlags = OBJECT_FLAGS_CHARACTER;
		Data(5) = m_pWServer->GetMapData()->GetResourceIndex_Surface("nvmine");
	}

	static void OnIncludeClass(CMapData *_pWData, CWorld_Server *_pWServer)
	{
		CWObject_Destructable::OnIncludeClass(_pWData, _pWServer);

		_pWData->GetResourceIndex_Surface("nvmine");
	}

	aint OnMessage(const CWObject_Message& _Msg)
	{
		switch(_Msg.m_Msg)
		{
		case OBJSYSMSG_NOTIFY_INTERSECTION:
		case OBJMSG_TRIGGER_INTERSECTION:
			SpawnExplosion(m_iObject);
			return 0;
		}
		return CWObject_Destructable::OnMessage(_Msg);
	}

	static void OnClientRefresh(CWObject_Client *_pObj, CWorld_Client *_pWClient)
	{
		if(_pObj->m_iAnim0 == 0 && _pObj->m_ClientData[0] == 0 && _pObj->m_Data[5] != 0)
		{
			CXR_WallmarkDesc WMD;
			WMD.m_SurfaceID = _pWClient->GetMapData()->GetResource_SurfaceID(_pObj->m_Data[5]);
			WMD.m_Size = 32;

			CMat4Dfp32 Mat;
			Mat.UnitNot3x3();
			CVec3Dfp32(0, 1, 0).SetRow(Mat, 0);
			CVec3Dfp32(-1, 0, 0).SetRow(Mat, 1);
			CVec3Dfp32(0, 0, 1).SetRow(Mat, 2);
			_pObj->GetPosition().SetRow(Mat, 3);

			_pObj->m_ClientData[0] = _pWClient->Wallmark_Create(WMD, Mat, 4, XR_WALLMARK_NEVERDESTROY);
		}
		else if(_pObj->m_iAnim0 == 1 && _pObj->m_ClientData[0] != 0)
		{
			_pWClient->Wallmark_Destroy(_pObj->m_ClientData[0]);
			_pObj->m_ClientData[0] = 0;
		}

		return CWObject_Destructable::OnClientRefresh(_pObj, _pWClient);
	}
};

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Mine, CWObject_Destructable, 0x0100);
