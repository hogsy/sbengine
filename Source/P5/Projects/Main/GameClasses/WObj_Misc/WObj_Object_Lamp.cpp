/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_Object_Lamp.cpp

	Author:			Anton Ragnarsson

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_Object_Lamp implementation

	Comments:

	History:		
		051116:		Created File
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_Object.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WDynamicsEngine.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Lights.h"

#define DBG_OUT DO_IF(0) M_TRACE


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Object_Lamp
|
| Data[1] = lamp flags
| Data[5] = light index (16-bit)
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObject_Object_Lamp : public CWObject_Object
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	typedef CWObject_Object parent;

	enum
	{
		FLAGS_LAMP_STATE_ON =			M_Bit(0),
		FLAGS_LAMP_STATE_BROKEN =		M_Bit(1),
		FLAGS_LAMP_MASTER_CONTROLLED =	M_Bit(2),
		FLAGS_LAMP_MODELSHADOW =		M_Bit(3),
	};

public:
	CWO_Light_MessageData	m_LightFlicker;			// What flicker linked light should have
	CWO_Light_MessageData	m_CurrentLightFlicker;	// Save what flicker linked light had before sending ours
	CNameHash m_Light;
	CNameHash m_MasterLight;
	CStr m_ProjectionMap;
	CVec3Dfp32 m_ProjMapOrigin;
	CMessageContainer m_Messages_LightsOn;
	CMessageContainer m_Messages_LightsOff;


	void SendSignalToMasterLight(int _Signal, int _Param = 0);
	void SafeSendToTarget(const CWObject_Message &_Msg, const CNameHash& _Target);

public:
	virtual void OnInitInstance(const aint* _pParam, int _nParam);
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual void OnSpawnWorld2();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);
	virtual void OnDeltaSave(CCFile* _pFile);

	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
};



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Object_Lamp
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Object_Lamp, parent, 0x0100);


void CWObject_Object_Lamp::OnInitInstance(const aint* _pParam, int _nParam)
{
	parent::OnInitInstance(_pParam, _nParam);

	if (_nParam == 2)
	{
		const CWObject_Object* pBaseObj = reinterpret_cast<const CWObject_Object*>(_pParam[0]);
		const CWObject_Object_Lamp* pLamp = safe_cast<const CWObject_Object_Lamp>(pBaseObj);

		// Copy data
		m_Light = pLamp->m_Light;
		m_MasterLight = pLamp->m_MasterLight;
		m_Messages_LightsOn = pLamp->m_Messages_LightsOn;
		m_Messages_LightsOff = pLamp->m_Messages_LightsOff;

		Data(DATA_LAMP_FLAGS)      = pLamp->Data(DATA_LAMP_FLAGS);
		Data(DATA_LAMP_LIGHTINDEX) = pLamp->Data(DATA_LAMP_LIGHTINDEX);

		// Move all children to this object
		for (int iChild; (iChild = pLamp->GetFirstChild()); )
			m_pWServer->Object_AddChild(m_iObject, iChild);
	}
}


void CWObject_Object_Lamp::OnCreate()
{
	parent::OnCreate();
	Data(DATA_LAMP_FLAGS) = FLAGS_LAMP_STATE_ON;
}


void CWObject_Object_Lamp::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	switch (_KeyHash)
	{
	case MHASH2('LIGH','T'): // "LIGHT"
		{
			m_Light = m_pWServer->World_MangleTargetName(KeyValue);
			break;
		}

	case MHASH3('MAST','ERLI','GHT'): // "MASTERLIGHT"
		{
			m_MasterLight = m_pWServer->World_MangleTargetName(KeyValue);
			break;
		}

	case MHASH3('INIT','IALS','TATE'): // "INITIALSTATE"
		{
			bool bOn = (KeyValue.Val_int() == 0);
			Data(DATA_LAMP_FLAGS) = (Data(DATA_LAMP_FLAGS) & ~FLAGS_LAMP_STATE_ON) | (bOn ? FLAGS_LAMP_STATE_ON : 0);
			break;
		}

	case MHASH4('LIGH','T_PR','OJMA','P'): // "LIGHT_PROJMAP"
		{
			m_ProjectionMap = KeyValue;
			break;
		}

	case MHASH5('LIGH','T_PR','OJMA','PORI','GIN'): // "LIGHT_PROJMAPORIGIN"
		{
			m_ProjMapOrigin.ParseString(KeyValue);
			break;
		}
	case MHASH5('LIGH','T_FL','ICKE','RTYP','E'): // LIGHT_FLICKERTYPE
		{
			m_LightFlicker.m_FlickerType = _pKey->GetThisValuei();
			return;
		}
	case MHASH5('LIGH','T_FL','ICKE','RSPR','EAD'): // LIGHT_FLICKERSPREAD
		{
			m_LightFlicker.m_FlickerSpread = _pKey->GetThisValuei();
			return;
		}
	case MHASH5('LIGH','T_FL','ICKE','RFRE','Q'): // LIGHT_FLICKERFREQ
		{
			m_LightFlicker.m_FlickerFreq = _pKey->GetThisValuef();
			return;
		}

	default:
		{
			if (KeyName.CompareSubStr("MESSAGE_LIGHTSON") == 0)
			{
				int iSlot = atoi(KeyName.Str() + 16);
				m_Messages_LightsOn.Add(iSlot, KeyValue, *this);
			}
			else if (KeyName.CompareSubStr("MESSAGE_LIGHTSOFF") == 0)
			{
				int iSlot = atoi(KeyName.Str() + 17);
				m_Messages_LightsOff.Add(iSlot, KeyValue, *this);
			}
			else
				return parent::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}


void CWObject_Object_Lamp::OnFinishEvalKeys()
{
	parent::OnFinishEvalKeys();

	m_Messages_LightsOn.Precache(*this);
	m_Messages_LightsOff.Precache(*this);

	m_Root.m_Flags |= FLAGS_LAMP;
	if (m_Root.m_Flags & FLAGS_MODELSHADOW)
		Data(DATA_LAMP_FLAGS) |= FLAGS_LAMP_MODELSHADOW;

	// Find main lamp trigger, to get aim offset
	CWO_Object_ClientData* pCD = AllocClientData(this);
	CVec3Dfp32 AimOffset(0.0f);
	if(m_Root.m_lTriggers.Len())
	{
		m_Root.m_lTriggers[0].m_PhysPrim.m_BoundBox.GetCenter(AimOffset);
	}

    pCD->m_AimOffset = AimOffset;
	RefreshDirty();
}


void CWObject_Object_Lamp::OnSpawnWorld2()
{
	parent::OnSpawnWorld();

	if (m_Light)
	{
		uint8 Msg = (Data(DATA_LAMP_FLAGS) & FLAGS_LAMP_STATE_ON) ? CWObject_Light::MSG_ON : CWObject_Light::MSG_OFF;
		SafeSendToTarget(CWObject_Message(OBJMSG_IMPULSE, Msg), m_Light);

		CWObject* pObj = m_pWServer->Object_Get(m_pWServer->Selection_GetSingleTarget(m_Light));
		if (pObj)
		{
			Data(DATA_LAMP_LIGHTINDEX) = pObj->OnMessage(CWObject_Message(OBJMSG_LIGHT_GETINDEX));

			if (m_ProjectionMap.Len())
			{ // Set projection map on the light
				CWObject_Message Msg(OBJMSG_LIGHT_INITPROJMAP, mint(m_ProjectionMap.Str()), 0, m_iObject, 0, m_ProjMapOrigin);
				pObj->OnMessage(Msg);
			}
		}
	}

	if (m_MasterLight)
	{
		uint bActive = (Data(DATA_LAMP_FLAGS) & FLAGS_LAMP_STATE_ON) ? 1 : 0;

		M_TRACE("[Object_Lamp %d, %s], sending MSG_SLAVE_REG to %s\n",
			m_iObject, GetName(), m_MasterLight.DbgName().Str());
		SendSignalToMasterLight(CWObject_Light::MSG_SLAVE_REG, bActive);
	}
}


aint CWObject_Object_Lamp::OnMessage(const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJMSG_LIGHT_IMPULSE:
		if (!(m_Root.m_Flags & FLAGS_WAITSPAWN) && !(Data(DATA_LAMP_FLAGS) & FLAGS_LAMP_STATE_BROKEN))
		{
			uint8 Msg = _Msg.m_Param0;
			if (Msg == CWObject_Light::MSG_BREAK)
			{
				if (m_Light)
				{
					SafeSendToTarget(CWObject_Message(OBJMSG_IMPULSE, CWObject_Light::MSG_BREAK), m_Light);

					int iObj = m_pWServer->Selection_GetSingleTarget(m_Light);
					if (iObj > 0)
						m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_BROKEN_LIGHT, iObj), m_pWServer->Game_GetObjectIndex());
				}

				if (Data(DATA_LAMP_FLAGS) & FLAGS_LAMP_STATE_ON)
					SendSignalToMasterLight(CWObject_Light::MSG_SLAVE_OFF, 1);

				Data(DATA_LAMP_FLAGS) |= FLAGS_LAMP_STATE_BROKEN;
			}
			else
			{
				if (Msg == CWObject_Light::MSG_TOGGLE)
					Msg = (Data(DATA_LAMP_FLAGS) & FLAGS_LAMP_STATE_ON) ? CWObject_Light::MSG_OFF : CWObject_Light::MSG_ON;

				if (Msg == CWObject_Light::MSG_ON && !(Data(DATA_LAMP_FLAGS) & FLAGS_LAMP_STATE_ON))
				{
					m_Messages_LightsOn.Send(_Msg.m_iSender, *this);
					Data(DATA_LAMP_FLAGS) |= FLAGS_LAMP_STATE_ON;
					SendSignalToMasterLight(CWObject_Light::MSG_SLAVE_ON);
				}
				else if (Msg == CWObject_Light::MSG_OFF && (Data(DATA_LAMP_FLAGS) & FLAGS_LAMP_STATE_ON))
				{
					m_Messages_LightsOff.Send(_Msg.m_iSender, *this);
					Data(DATA_LAMP_FLAGS) &= ~FLAGS_LAMP_STATE_ON;
					SendSignalToMasterLight(CWObject_Light::MSG_SLAVE_OFF);
				}
				else if (Msg == CWObject_Light::MSG_FLICKER)
				{
					// Flicker lights
					if (_Msg.m_Param1)
					{
						CWObject_Message Msg(OBJMSG_LIGHT_IMPULSE,CWObject_Light::MSG_GETFLICKERDATA);
						Msg.m_DataSize = sizeof(CWO_Light_MessageData);
						Msg.m_pData = &m_CurrentLightFlicker;
						SafeSendToTarget(Msg, m_Light);
						Msg.m_Param0 = CWObject_Light::MSG_FLICKER;
						Msg.m_pData = &m_LightFlicker;
						Msg.m_DataSize = sizeof(CWO_Light_MessageData);
						SafeSendToTarget(Msg,m_Light);
					}
					else
					{
						// Reset to old flicker
						CWObject_Message Msg(OBJMSG_LIGHT_IMPULSE,CWObject_Light::MSG_FLICKER);
						Msg.m_pData = &m_CurrentLightFlicker;;
						Msg.m_DataSize = sizeof(CWO_Light_MessageData);
						SafeSendToTarget(Msg,m_Light);
					}
					return 1;
				}

				SafeSendToTarget(_Msg, m_Light);
			}
		}
		return 1;

	case OBJMSG_DAMAGE:
		{
			const CWO_DamageMsg* pMsg = CWO_DamageMsg::GetSafe(_Msg);
			if (pMsg && pMsg->m_DamageType == DAMAGETYPE_BLACKHOLE)
			{
				if (m_Root.m_lSubParts.Len() > 0 && m_Root.m_lConstraints.Len() > 0)
					CheckSeparation(); // so that object doesn't act as one rigid part

				return 0;  // Lamps ignore damage from black hole
			}
		}
		break;

	case OBJMSG_CHAR_GETAUTOAIMOFFSET:
		{
            CWO_Object_ClientData* pCD = GetClientData(this);
			return pCD->m_AimOffset.m_Value.Pack32(256.0f);
		}
		break;

	case OBJMSG_LIGHT_MORPH:
		if (!(m_Root.m_Flags & FLAGS_WAITSPAWN))
		{
			// pass message on to the light object
			SafeSendToTarget(_Msg, m_Light);
			return 1;
		}
		break;
	case OBJMSG_LIGHT_GETSTATUS:
		{
			if (!m_Light)
				return 0;

			int32 RetVal = 0;
			TSelection<CSelection::SMALL_BUFFER> Selection;
			m_pWServer->Selection_AddTarget(Selection, m_Light);
			const int16 *pSel;
			int nSel = m_pWServer->Selection_Get(Selection, &pSel);
			for (int i = 0; i < nSel; i++)
			{
				CWObject *pObj = m_pWServer->Object_Get(pSel[i]);
				if (pObj->m_iClass == m_iClass)
					ConOutL(CStrF("§cf80WARNING: (GP) Detected a Lamp with same name as Light! (%s)", pObj->GetName()));
				else
				{
					RetVal += m_pWServer->Message_SendToObject(_Msg, pSel[i]);
				}
			}
			return RetVal;
		}
	}

	return parent::OnMessage(_Msg);
}


void CWObject_Object_Lamp::SendSignalToMasterLight(int _Signal, int _Param)
{
	int iMasterLight = m_pWServer->Selection_GetSingleTarget(m_MasterLight);
	CWObject* pMasterLight = m_pWServer->Object_Get(iMasterLight);
	if (pMasterLight)
	{
		if (pMasterLight->m_iClass == m_iClass)
			ConOutL(CStrF("§cf80WARNING: Detected a Lamp with same name as Light! (%s)", pMasterLight->GetName()));
		else
		{
			CWObject_Message Msg;
			Msg.m_iSender = m_iObject;
			Msg.m_Msg = OBJMSG_IMPULSE;
			Msg.m_Param0 = _Signal;
			Msg.m_Param1 = _Param;
			pMasterLight->OnMessage(Msg);
		}
	}
}


void CWObject_Object_Lamp::SafeSendToTarget(const CWObject_Message& _Msg, const CNameHash& _Target)
{
	if (!_Target)
		return;

	TSelection<CSelection::SMALL_BUFFER> Selection;
	m_pWServer->Selection_AddTarget(Selection, _Target);
	const int16 *pSel;
	int nSel = m_pWServer->Selection_Get(Selection, &pSel);
	for (int i = 0; i < nSel; i++)
	{
		CWObject *pObj = m_pWServer->Object_Get(pSel[i]);
		if (pObj->m_iClass == m_iClass)
			ConOutL(CStrF("§cf80WARNING: (GP) Detected a Lamp with same name as Light! (%s)", pObj->GetName()));
		else
			pObj->OnMessage(_Msg);
	}
}


void CWObject_Object_Lamp::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	parent::OnDeltaLoad(_pFile, _Flags);

	if (m_Root.m_Flags & FLAGS_SAVEPARTS)
	{
		m_Light.Read(_pFile);
		m_MasterLight.Read(_pFile);
		m_ProjectionMap.Read(_pFile);
		m_ProjMapOrigin.Read(_pFile);
		m_Messages_LightsOn.Read(_pFile);
		m_Messages_LightsOff.Read(_pFile);
	}

	if (m_Light)
	{
		CWObject* pObj = m_pWServer->Object_Get(m_pWServer->Selection_GetSingleTarget(m_Light));
		if (pObj)
			Data(DATA_LAMP_LIGHTINDEX) = pObj->OnMessage(CWObject_Message(OBJMSG_LIGHT_GETINDEX));
	}
}


void CWObject_Object_Lamp::OnDeltaSave(CCFile* _pFile)
{
	parent::OnDeltaSave(_pFile);

	if (m_Root.m_Flags & FLAGS_SAVEPARTS)
	{
		m_Light.Write(_pFile);
		m_MasterLight.Write(_pFile);
		m_ProjectionMap.Write(_pFile);
		m_ProjMapOrigin.Write(_pFile);
		m_Messages_LightsOn.Write(_pFile);
		m_Messages_LightsOff.Write(_pFile);
	}
}


void CWObject_Object_Lamp::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MSCOPESHORT(CWObject_Object_Lamp::OnClientRender);

	uint32 LampFlags = _pObj->m_Data[DATA_LAMP_FLAGS];
	uint16 LightIndex = _pObj->m_Data[DATA_LAMP_LIGHTINDEX];
	uint16 LightGUID = 0;
	CPixel32 UserColor(0);

	if (LampFlags & FLAGS_LAMP_MASTER_CONTROLLED)
	{
		if ((LampFlags & FLAGS_LAMP_STATE_ON) && !(LampFlags & FLAGS_LAMP_STATE_BROKEN))
			UserColor = -1;
	}
	else if (LightIndex)
	{
		CVec3Dfp32 Intens;
		LightGUID = _pEngine->m_pSceneGraphInstance->SceneGraph_Light_GetGUID(LightIndex);
		_pEngine->m_pSceneGraphInstance->SceneGraph_Light_GetIntensity(LightIndex, Intens);

		UserColor.R() = RoundToInt(Clamp01(Intens[0] * 2.0f) * 255.0f);
		UserColor.G() = RoundToInt(Clamp01(Intens[1] * 2.0f) * 255.0f);
		UserColor.B() = RoundToInt(Clamp01(Intens[2] * 2.0f) * 255.0f);
		UserColor.A() = 255;
	}

	CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
	AnimState.m_Data[1] = UserColor;
	if (!(LampFlags & FLAGS_LAMP_MODELSHADOW))
		AnimState.m_NoShadowLightGUID = LightGUID;

	DoRender(_pObj, _pWClient, _pEngine, AnimState);
}

aint CWObject_Object_Lamp::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJMSG_CHAR_GETAUTOAIMOFFSET:
		{
			CWO_Object_ClientData* pCD = GetClientData(_pObj);
			if(pCD)
				return pCD->m_AimOffset.m_Value.Pack32(256.0f);
		}
		break;
	}

	return parent::OnClientMessage(_pObj, _pWClient, _Msg);
}
