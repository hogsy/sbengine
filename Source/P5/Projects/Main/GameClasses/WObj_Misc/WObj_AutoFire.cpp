#include "PCH.h"


class CWObject_AutoFire : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;
protected:
	TPtr<CRPG_Object_Item> m_spWeapon;
	// How big of an angle we're gonna autofire in
	fp32	m_ConeDot;
	fp32	m_ActivationRange;
	// Target we're shooting for
	int32	m_TargetHash;
	int16	m_iTarget;
	int16	m_Flags;
	bool	m_bReEvalTarget;
public:
	enum
	{
		AUTOFIRE_FLAGS_WAITSPAWN = M_Bit(0),
	};

	class CWO_AutoFire_ClientData : public CReferenceCount
	{
	public:
		TPtr<CXR_ModelInstance> m_spMuzzleFlash;
	};
	
	virtual void OnCreate()
	{
		CWObject::OnCreate();
		m_ConeDot = M_Cos(15.0f * _PI2/360.0f);
		m_ActivationRange = 1000.0f;
		m_Flags = 0;
		m_bReEvalTarget = false;
	}

	static void OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
	{
		// Check if we need to (re)allocate client data
		CReferenceCount* pData = _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA];
		CWO_AutoFire_ClientData* pCD = TDynamicCast<CWO_AutoFire_ClientData>(pData);

		// Allocate clientdata
		if (!pCD)
		{
			pCD = MNew(CWO_AutoFire_ClientData);
			if (!pCD)
				Error_static("CWObject_Turret", "Could not allocate client data!");

			_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA] = pCD;
		}
	}
	static const CWO_AutoFire_ClientData& GetClientData(const CWObject_CoreData* _pObj)
	{
		const CReferenceCount* pData = _pObj->m_lspClientObj[0];
		M_ASSERT(pData, "Who deleted my client data?!");
		return *safe_cast<const CWO_AutoFire_ClientData>(pData);
	}
	static CWO_AutoFire_ClientData& GetClientData(CWObject_CoreData* _pObj)
	{
		CReferenceCount* pData = _pObj->m_lspClientObj[0];
		M_ASSERT(pData, "Who deleted my client data?!");
		return *safe_cast<CWO_AutoFire_ClientData>(pData);
	}

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
	{
		CStr Value = _pKey->GetThisValue();

		switch (_KeyHash)
		{
		case MHASH3('TURR','ETFL','AGS'): // "TURRETFLAGS"
			{
				static const char *FlagsTranslate[] =
				{
					"waitspawn",NULL
				};

				m_Flags = m_Flags | Value.TranslateFlags(FlagsTranslate);
				break;
			}
		case MHASH2('WEAP','ON'): // "WEAPON"
			{
				spCRPG_Object spObj = CRPG_Object::CreateObject(Value,m_pWServer);
				if (spObj != NULL && TDynamicCast<CRPG_Object_Item >((CRPG_Object *)spObj) != NULL)
				{
					spCRegistry spReg = CRPG_Object::GetEvaledRegistry(Value, m_pWServer);
					spObj->OnIncludeClass(spReg, m_pWServer->GetMapData(),m_pWServer);
					m_spWeapon = (CRPG_Object_Item *)((CRPG_Object *)spObj);
					if (m_spWeapon)
					{
						m_spWeapon->m_Flags2 |= RPG_ITEM_FLAGS2_NOAMMODRAW;
					}
				}
				break;
			}
		case MHASH3('ATTA','CKAN','GLE'): // "ATTACKANGLE"
			{
				fp32 Val;
				_pKey->GetThisValueaf(1,&Val);

				m_ConeDot = M_Cos(Val * _PI2 / 360.0f);
				break;
			}
		case MHASH4('ACTI','VATI','ONRA','NGE'): // "ACTIVATIONRANGE"
			{
				fp32 Val;
				_pKey->GetThisValueaf(1,&Val);

				m_ActivationRange = Val;
				break;
			}
		case MHASH2('TARG','ET'): // "TARGET"
			{
				m_TargetHash = StringToHash(Value);
				if (Value == "$player")
					m_bReEvalTarget = true;
				break;
			}
		default:
			CWObject::OnEvalKey(_KeyHash,_pKey);
		};
	}

	virtual void OnFinishEvalKeys()
	{
		CWObject::OnFinishEvalKeys();
		// No refresh if we have no weapon
		if ((m_Flags & AUTOFIRE_FLAGS_WAITSPAWN) || !m_spWeapon)
			ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;

		// Set the effect
		if ((m_spWeapon)&&(m_spWeapon->m_Model.m_ExtraModels.m_lEffects.Len() > 0))
		{
			Data(0) = m_spWeapon->m_Model.m_ExtraModels.m_lEffects[0].m_iModel;
		}
	}

	virtual void OnSpawnWorld()
	{
		CWObject::OnSpawnWorld();
		m_iTarget = m_pWServer->Selection_GetSingleTarget(m_TargetHash);
	}

	virtual aint OnMessage(const CWObject_Message &_Msg)
	{
		if (_Msg.m_Msg == OBJMSG_IMPULSE)
		{
			if (_Msg.m_Param0 == 0)
				ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
			else if (m_spWeapon)
				ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

			return 1;
		}
		else
			return CWObject::OnMessage(_Msg);
	}
	virtual void OnRefresh()
	{
		CWObject::OnRefresh();
		// Reeval target if it's the player
		if (m_bReEvalTarget)
		{
			m_bReEvalTarget = false;
			m_iTarget = m_pWServer->Selection_GetSingleTarget(m_TargetHash);
		}

		CWObject_CoreData* pObj = m_pWServer->Object_GetCD(m_iTarget);
		if (m_spWeapon && (m_pWServer->GetGameTick() - m_spWeapon->m_iExtraActivationWait) > 0 && pObj)
		{
			//m_pWServer->Phys_InsertPosition(m_iObject,this);
			CMat4Dfp32 Mat = GetPositionMatrix();
			CVec3Dfp32 CenterPos;
			pObj->GetAbsBoundBox()->GetCenter(CenterPos);
			CVec3Dfp32 Dir = CenterPos - Mat.GetRow(3);
			fp32 LenSqr = Dir.LengthSqr();
			Dir.Normalize();
			//m_pWServer->Debug_RenderWire(GetPosition(),GetPosition() + Mat.GetRow(0) * 150.0f,0xff7f0000);
			//m_pWServer->Debug_RenderWire(GetPosition(),GetPosition() + Dir * 150.0f,0xff00007f);
			fp32 Dot = Dir * Mat.GetRow(0);
			if (Sqr(m_ActivationRange) > LenSqr && Dot >= m_ConeDot)
			{
				// Yay, we can fire, 
				Dir.SetRow(Mat,0);
				Mat.RecreateMatrix(0,2);
				m_spWeapon->Activate(Mat,NULL,m_iObject,0);
				// set the muzzleeffect
				Data(1) = m_pWServer->GetGameTick() + 1;
				Data(2) = TruncToInt(m_pWServer->GetGameTicksPerSecond() * 2.0f);
			}
		}
	}

	static int OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
	{
		int32 iOldModel = _pObj->m_Data[0];
		int RetVal = CWObject::OnClientUpdate(_pObj,_pWClient,_pData,_Flags);
		if (!(_pWClient->GetClientMode() & WCLIENT_MODE_MIRROR) && iOldModel != _pObj->m_Data[0])
		{
			CWO_AutoFire_ClientData& CD = GetClientData(_pObj);
			CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_Data[0]);
			if (pModel)
			{
				CD.m_spMuzzleFlash = pModel->CreateModelInstance();
				if (CD.m_spMuzzleFlash != NULL)
					CD.m_spMuzzleFlash->Create(pModel, CXR_ModelInstanceContext(NULL, NULL));
			}
			else
				CD.m_spMuzzleFlash = NULL;
		}
		return RetVal;
	}

	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
	{
		CWO_AutoFire_ClientData& CD = GetClientData(_pObj);
		fp32 IPTime = _pWClient->GetRenderTickFrac();
		CMat4Dfp32 MatIP, ParentMat;
		Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);
		if (_pObj->m_Data[0] != 0)
		{
			CMTime Time = _pWClient->GetRenderTime();
			CXR_AnimState AnimState;
			CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_Data[0]);
			if(pModel->GetParam(CXR_MODEL_PARAM_TIMEMODE) == CXR_MODEL_TIMEMODE_CONTINUOUS)
			{
				AnimState.m_AnimTime0 = Time;
				AnimState.m_AnimTime1.Reset();
			}
			else
			{
				CMTime AnimTime;
				CMTime StartTime = CMTime::CreateFromTicks(_pObj->m_Data[1],_pWClient->GetGameTickTime());
				if (Time.Compare(StartTime) < 0)
					AnimState.m_AnimTime0 = CMTime::CreateFromSeconds(0.0f);
				else
					AnimState.m_AnimTime0 = Time - StartTime;

				AnimState.m_AnimTime1 = Time;

				// If we're over duration, just return...
				if (AnimState.m_AnimTime0.Compare(CMTime::CreateFromTicks(_pObj->m_Data[2],_pWClient->GetGameTickTime())) > 0)
					return;
			}

			AnimState.m_iObject = _pObj->m_iObject;
			//AnimState.m_pContext = (CXR_Skeleton*)_pBaseModel->GetParam(MODEL_PARAM_SKELETON);
			AnimState.m_Anim0 = (2 << 8) | 3;
			AnimState.m_pModelInstance = CD.m_spMuzzleFlash;
			AnimState.m_Anim0 = 0;//m_AC_RandSeed & 0x7fff;
			AnimState.m_Data[3] = 0;//m_ModelFlags[i];
			_pEngine->Render_AddModel(pModel, MatIP, AnimState);
		}
	}

	
};

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_AutoFire, CWObject, 0x0100);
