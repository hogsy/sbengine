#include "PCH.h"
#include "WObj_HealthStation.h"
#include "../WObj_CharMsg.h"
#include "../WRPG/WRPGChar.h"

#define HEALTHSTATION_MAXLOAD 4
#define HEALTHSTATION_DATASLOT 7

#define HEALTHSTATION_CARTRIDEID 0x123

//
//
//
void CWObject_HealthStation::OnCreate()
{
	CWObject_ActionCutscene::OnCreate();
	CACSClientData* pCD = GetACSClientData(this);

	m_MaxLoad = HEALTHSTATION_MAXLOAD;
	m_Load = -1;
	m_RefillMsgDelay = 0;
	m_ActivationTick = -1;
	m_Type = HEALTHSTATIONTYPE_SMALL;
	m_ActionCutsceneTypeSuccess = ACTIONCUTSCENE_TYPE_USEMEDSTATION;
	pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_OPERATION_PLAYSUCCESSANIMATION);
}

//
//
//
void CWObject_HealthStation::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	
	int KeyValuei = _pKey->GetThisValuei();
	
	switch (_KeyHash)
	{
	case MHASH6('HEAL','THST','ATIO','N_MA','XLOA','D'): // "HEALTHSTATION_MAXLOAD"
		{
			m_MaxLoad = KeyValuei;
			break;
		}

	case MHASH5('HEAL','THST','ATIO','N_LO','AD'): // "HEALTHSTATION_LOAD"
		{
			m_Load = KeyValuei;
			break;
		}

	case MHASH5('HEAL','THST','ATIO','N_DE','LAY'): // "HEALTHSTATION_DELAY"
		{
			m_RefillMsgDelay = (uint16)(_pKey->GetThisValuef() * m_pWServer->GetGameTicksPerSecond());
			break;
		}

	case MHASH5('HEAL','THST','ATIO','N_TY','PE'): // "HEALTHSTATION_TYPE"
		{
			m_Type = KeyValuei;
			break;
		}

	case MHASH3('REFI','LLCA','MERA'): // "REFILLCAMERA"
		{
			m_CamConfigStr = _pKey->GetThisValue();
			break;
		}

	default:
		{
			if (KeyName.Find("MSG_TRIGGERREFILL") != -1)
			{
				m_lMsg_TriggerRefill.SetLen(m_lMsg_TriggerRefill.Len() + 1);
				m_lMsg_TriggerRefill[m_lMsg_TriggerRefill.Len()-1].Parse(_pKey->GetThisValue(), m_pWServer);
			}
			else
				CWObject_ActionCutscene::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

spCActionCutsceneCamera CWObject_HealthStation::GetActionCutsceneCamera(int _iObject, int _iCharacter, int _Specific)
{
	CWObject_CoreData* pObj = m_pWServer->Object_GetCD(_iCharacter);
	CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
	if (pCD)
	{
		int32 ACSType = (int32)pCD->m_ControlMode_Param2;
		if (ACSType == ACTIONCUTSCENE_TYPE_RECHARGEMEDSTATION)
		{
			spCActionCutsceneCamera spCamera = MNew(CActionCutsceneCamera);

			if (spCamera != NULL)
			{
				spCamera->SetServer(m_pWServer);
				spCamera->OnCreate();
				spCamera->ConfigureCamera(m_RefillCamera);
				return spCamera;
			}
		}
	}

	return CWObject_ActionCutscene::GetActionCutsceneCamera(_iObject,_iCharacter,_Specific);
}

void CWObject_HealthStation::TagAnimationsForPrecache(CWAGI_Context* _pContext, CWAGI* _pAGI, TArray<int32>& _liACS)
{
	_liACS.Add(ACTIONCUTSCENE_TYPE_USEMEDSTATION);
	_liACS.Add(ACTIONCUTSCENE_TYPE_RECHARGEMEDSTATION);
	CWObject_ActionCutscene::TagAnimationsForPrecache(_pContext,_pAGI,_liACS);
}

void CWObject_HealthStation::OnSpawnWorld()
{
	if (m_CamConfigStr.Len() > 0)
		CActionCutsceneCamera::MakeConfigBlockFromString(m_RefillCamera,m_CamConfigStr, m_pWServer);
	m_CamConfigStr.Clear();
	CWObject_ActionCutscene::OnSpawnWorld();

	for (int32 i = 0; i < m_lMsg_TriggerRefill.Len(); i++)
		m_lMsg_TriggerRefill[i].SendPrecache(m_iObject, m_pWServer);
}
//
//
//
void CWObject_HealthStation::OnFinishEvalKeys()
{
	CWObject_ActionCutscene::OnFinishEvalKeys();

	// Fill it up
	if (m_Load == -1)
		m_Load = m_MaxLoad;

	if (m_MaxLoad != HEALTHSTATION_MAXLOAD)
		ConOut(CStrF("WARNING: Healthstation max load have been changed from 4 to %i, the textureanimation will not work! Consider yourself warned.", m_MaxLoad));

	// Only refresh object when active
	ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;

	Data(HEALTHSTATION_DATASLOT) = m_Load;
}

//
//
//
void CWObject_HealthStation::GiveHealth()
{
	int8 Lacking = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETMAXHEALTH), m_iActivator) - m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETHEALTH), m_iActivator);
	int Fill = MinMT((Lacking+15)/16, m_Load);
	m_Load -= Fill;
	m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ADDHEALTH, Fill*16), m_iActivator);
}

//
//
//
void CWObject_HealthStation::GivePlupp()
{
	m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_INCREASEMAXHEALTH, 2), m_iActivator);

	int32 MaxHealth = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETMAXHEALTH), m_iActivator);
	int32 CurrentHealth = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETHEALTH), m_iActivator);
	int8 Lacking = MaxHealth - CurrentHealth;
	m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ADDHEALTH, Lacking), m_iActivator);
	m_Load = 0;
}

//
//
//
void CWObject_HealthStation::OnRefresh()
{
	CWObject_ActionCutscene::OnRefresh();

	if (m_ActivationTick != -1)
	{
		// Make sure health HUD is visible
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_DISPLAYHEALTH), m_iActivator);

		// Should we send health to the character this refresh?
		// Not working atm
		/*int RunningTick = m_pWServer->GetGameTick() - m_ActivationTick;
		if((!m_bHaveGivenHealth) && (RunningTick >= m_RefillMsgDelay))
		{
			m_bHaveGivenHealth = true;
			switch(m_Type)
			{
				case HEALTHSTATIONTYPE_SMALL: GiveHealth(); break;
				//case HEALTHSTATIONTYPE_BIG: GivePlupp();  break; // by request of junkan
			}
		}*/

		/*if (m_Load == 0)
		{
			// Depleted
			// This healthstation (actioncutscene) should not be activated again
			OnMessage(CWObject_Message(OBJMSG_IMPULSE, ACTIONCUTSCENE_IMPULSE_LOCK));
		}*/

		Data(HEALTHSTATION_DATASLOT) = m_Load;
	}
}

//
//
//
CRPG_Object_Inventory *CWObject_HealthStation::GetActivatorInventory(int16 _iActivator)
{
	//
	CWObject *pObj;
	CWObject_Interface_AI *pAIInterface;

	if(!(pObj = m_pWServer->Object_Get(_iActivator)))
		return NULL;

	if(!(pAIInterface = pObj->GetInterface_AI()))
		return NULL;

	return pAIInterface->AI_GetRPGInventory(RPG_CHAR_INVENTORY_ITEMS);
}

//
//
//
CRPG_Object_Item *CWObject_HealthStation::GetActivatorCartItem(int16 _iActivator)
{
	CRPG_Object_Inventory *pInventory = GetActivatorInventory(_iActivator);
	if(!pInventory)
		return false;

	for(int32 i = 0; i < pInventory->GetNumItems(); i++)
	{
		CRPG_Object_Item *pItem = pInventory->GetItemByIndex(i);
		if(pItem && pItem->m_iItemType == HEALTHSTATION_CARTRIDEID)
			return pItem;
	}

	return NULL;
}

//
//
//
int32 CWObject_HealthStation::GetNumActivatorCarts(int16 _iActivator)
{
	CRPG_Object_Item *pItem = GetActivatorCartItem(_iActivator);
	if(!pItem)
		return 0;

	return pItem->m_NumItems;
}

//
//
//
void CWObject_HealthStation::UseActivatorCart(int16 _iActivator)
{
	CRPG_Object_Item *pItem = GetActivatorCartItem(_iActivator);
	if(!pItem)
		return;

	if(pItem->m_NumItems)
		pItem->m_NumItems--;

	if(!pItem->m_NumItems)
	{
		CRPG_Object_Inventory *pInventory = GetActivatorInventory(_iActivator);
		if(!pInventory)
			return;

		pInventory->RemoveItemByType(0, HEALTHSTATION_CARTRIDEID);
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_UPDATEQUESTICONS),_iActivator);
	}
}


//
//
//
void CWObject_HealthStation::OnActivateMessage(int16 _iActivator)
{
	m_iActivator = _iActivator;
	m_ActivationTick = m_pWServer->GetGameTick();
	m_bHaveGivenHealth = false;

	// Make sure the ledge camera will be updated when the ACS is active
	ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
}

//
//
//
void CWObject_HealthStation::OnEndMessage()
{
	m_iActivator = 0;
	m_ActivationTick = -1;
}

//
//
//
aint CWObject_HealthStation::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
		case OBJMSG_ACTIONCUTSCENE_ACTIVATE:
			if (_Msg.m_iSender != -1)
			{
				int bActionCutsceneStarted = CWObject_ActionCutscene::OnMessage(_Msg);
				if (bActionCutsceneStarted)
				{
					OnActivateMessage(_Msg.m_iSender);
					return true;
				};
			}
			return false;

		case OBJMSG_ACTIONCUTSCENE_ONENDACS:
			OnEndMessage();
			break;	// Does not return here, should perform baseclass OBJMSG_ACTIONCUTSCENE_ONENDACS
		case OBJMSG_ACTIONCUTSCENE_DOTRIGGERMIDDLE:
			{
				// Give health and send on to base class
				if(!m_bHaveGivenHealth)
				{
					m_bHaveGivenHealth = true;
					switch(m_Type)
					{
					case HEALTHSTATIONTYPE_SMALL: GiveHealth(); break;
					case HEALTHSTATIONTYPE_BIG: /*GivePlupp(); */ break; // by request of junkan
				}
				break;
			}
		case OBJMSG_ACTIONCUTSCENE_DOTRIGGERREFILL:
			{
				// Send refill messages
				//....
				for (int32 i = 0; i < m_lMsg_TriggerRefill.Len(); i++)
					m_lMsg_TriggerRefill[i].SendMessage(m_iObject, _Msg.m_iSender, m_pWServer);
				CWObject_CoreData* pObj = m_pWServer->Object_GetCD(_Msg.m_iSender);
				CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
				if (pCD)
				{
					int32 ACSType = (int32)pCD->m_ControlMode_Param2;
					if (ACSType == ACTIONCUTSCENE_TYPE_RECHARGEMEDSTATION)
					{
						UseActivatorCart(_Msg.m_iSender);
						m_Load = m_MaxLoad;
					}
				}
				break;
			}
		}
	}

	return CWObject_ActionCutscene::OnMessage(_Msg);
}

bool CWObject_HealthStation::DoActionSuccess(int _iCharacter)
{
	CACSClientData* pCDACS = GetACSClientData(this);
	// If the animation isn't to be played, return.. (maybe should change this)
	if (!(pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_PLAYSUCCESSANIMATION) || 
		(m_ActionCutsceneTypeSuccess == ACTIONCUTSCENE_TYPE_UNDEFINED))
	{
		return false;
	}

	int32 ACSType = m_ActionCutsceneTypeSuccess;
	if (m_Load == 0)
	{
		// Depleted, use recharage instead
		if (GetNumActivatorCarts(_iCharacter) > 0)
			ACSType = ACTIONCUTSCENE_TYPE_RECHARGEMEDSTATION;
		else
			return false;
	}
	else
	{
		// Send impulse 1 to the base class when successful
		OnImpulse(1, _iCharacter);
	}
	
	// Hmm, ok we need the same functionality on the new animation system thingy
	CWObject* pObj = m_pWServer->Object_Get(_iCharacter);
	CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
	
	if (!pCD)
		return false;

	if (m_ActionCutsceneTypeSuccess == ACTIONCUTSCENE_TYPE_JUSTACTIVATE)
	{
		SetStartPosition(_iCharacter);

		// Send the success trigger messages 
		if (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESS)
		{
			// Send the trigger message
			int32 Len = m_lMsg_TriggerSuccess.Len();
			for (int32 i = 0; i < Len; i++)
			{
				if (m_lMsg_TriggerSuccess[i].IsValid())
					m_lMsg_TriggerSuccess[i].SendMessage(m_iObject, _iCharacter, m_pWServer);
			}
		}

		// If the acs should be disabled, just remove it
		if (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_DISABLEONSUCCESS)
		{
			//m_pWServer->Object_Destroy(m_iObject);
			pCDACS->SetDisabled(true);
		}
	}
	else
	{
		// Enter actioncutscene controlmode
		CWObject_Character::Char_SetControlMode(pObj, PLAYER_CONTROLMODE_ACTIONCUTSCENE);
		((CWObject_Character *)pObj)->UpdateVisibilityFlag();

		// Set action cutscene id to controlmode param0
		pCD->m_ControlMode_Param0 = (fp32)m_iObject;
		
		// Set cutscenetype to param2
		pCD->m_ControlMode_Param2 = (fp32)ACSType;

		if (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_FLAGS_HASDEFINEDDIRECTION)
		{
			// Set defined direction to param1
			pCD->m_ControlMode_Param1 = m_DefinedDirection;
		}

		// Ok determine what type of cutscene we're having and deal with positioning accordingly
		// Only the hatch animations should be able to move around (and thus use the old positioning)
		// Filter out success actions and set to param4
		if (m_ActionCutsceneTypeSuccess > ACTIONCUTSCENE_TYPE_HATCHOUTHORIZONTAL)
		{
			int32 Flags = ((pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_SUCCESSMASK) & 
				~(ACTIONCUTSCENE_OPERATION_SETSTARTPOSMASK|ACTIONCUTSCENE_OPERATION_SETENDPOSMASK)) | 
				ACTIONCUTSCENE_FLAGS_ISSUCCESS | ACTIONCUTSCENE_FLAGS_USEFLOATINGSTART;
			if (m_ActionCutsceneTypeSuccess == ACTIONCUTSCENE_TYPE_USEHEALTHSTATION)
				Flags |= ACTIONCUTSCENE_OPERATION_USEPERFECTPOS;
			pCD->m_ControlMode_Param4 = Flags;
		}
		else
		{
			pCD->m_ControlMode_Param4 = (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_SUCCESSMASK) | 
				ACTIONCUTSCENE_FLAGS_ISSUCCESS;
		}
		

		// Make sure the camera will be updated when the ACS is active
		ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

		// Start activation timer
		if (m_MinActivationTime > 0.0f)
			m_MinActivationTimer = m_pWServer->GetGameTick() + 
				(int)(m_MinActivationTime * m_pWServer->GetGameTicksPerSecond());
		else
			m_MinActivationTimer = 0;
	}

	return true;
}

void CWObject_HealthStation::OnDeltaSave(CCFile* _pFile)
{
	CWObject_ActionCutscene::OnDeltaSave(_pFile);
	_pFile->WriteLE(m_Load);
}

//
//
//
void CWObject_HealthStation::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	CWObject_ActionCutscene::OnDeltaLoad(_pFile, _Flags);
	_pFile->ReadLE(m_Load);
	Data(HEALTHSTATION_DATASLOT) = m_Load;
}

//
//
//
void CWObject_HealthStation::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Engine_Path_OnClientRender, MAUTOSTRIP_VOID);
	if((_pObj->m_ClientFlags & CLIENTFLAGS_INVISWHENSTOPPED) && !(_pObj->m_ClientFlags & (CLIENTFLAGS_RUN | CLIENTFLAGS_PROPELLED)))
		return;
	if (_pObj->m_ClientFlags & CLIENTFLAGS_DEACTIVATED)
		return;

	bool bMat = false;
	CMat4Dfp32 Mat;

	int Load = _pObj->m_Data[HEALTHSTATION_DATASLOT];
	int HealthLeft = HEALTHSTATION_MAXLOAD-Load;
	//int Frame = HealthLeft;
	fp32 Time = HealthLeft + 0.1f;
//	for(int i = 0; i < CWO_NUMMODELINDICES; i++)
	int i = 0;
	{
		CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
		if(pModel)
		{
			if(!bMat)
			{
//				Time = GetTime(_pObj, _pWClient, _pWClient->GetGameTick() + _pWClient->GetRenderTickFrac());
				Mat = ((CWObject_Attach *)_pObj)->GetRenderMatrix(_pWClient, CMTime(), 0, 0);
				bMat = true;
			}

			CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient, i);

/*			if (pModel->GetParam(CXR_MODEL_PARAM_NEEDPATHDATA))
				AnimState.m_pspClientData = &(_pObj->m_lspClientObj[1]);
			else
				AnimState.m_pspClientData = &(_pObj->m_lspClientObj[2]);*/


			AnimState.m_AnimTime0 = CMTime::CreateFromSeconds(Time);
			AnimState.m_Data[3] = ~(_pObj->m_Data[3]);

			switch((_pObj->m_ClientFlags >> CLIENTFLAGS_LIGHTINGSHIFT) & 3)
			{
			case 0: AnimState.m_AnimAttr0 = Time; break;
			case 1: AnimState.m_AnimAttr0 = 0; break;
			case 2: AnimState.m_AnimAttr0 = 1; break;
			}

			_pEngine->Render_AddModel(pModel, Mat, AnimState);
		}
	}
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_HealthStation, CWObject_ActionCutscene, 0x0100);
