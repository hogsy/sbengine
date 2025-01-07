#include "PCH.h"
#include "WObj_Telephone.h"
#include "../WObj_Char.h"
#include "../../GameWorld/WFrontEndMod.h"
#include "../../Exe/WGameContextMain.h"

#define STR_TELEPHONE_DIALING "§LACS_TELEPHONE_DIALING"
#define STR_TELEPHONE_NORMAL "§LACS_TELEPHONE"

void CWObject_Telephone::OnCreate()
{
	CWObject_TelephoneParent::OnCreate();

	m_DialingNumber = -1;
	m_LastDialTone = 0;
	m_bRunPreDial = false;
	m_NextDialAtThisTick = 0;
	m_NumbersLeftToDial = 9;
	m_iActiveChar = 0;
}

void CWObject_Telephone::OnSpawnWorld()
{
	CWObject_TelephoneParent::OnSpawnWorld();

	CACSClientData* pCD = GetACSClientData(this);
	// Find telephone registry (should only be one)
	int iTeleReg = m_pWServer->Selection_GetSingleTarget("TELEPHONEREG");
	m_spGlobalRegistry = TDynamicCast<CWObject_TelephoneRegistry>(m_pWServer->Object_Get(iTeleReg));
	if (!m_spGlobalRegistry)
	{
		ConOutL("WARNING NO TELEPHONE REGISTRY FOUND");
	}
	else
	{
		pCD->SetTelephoneRegistry(iTeleReg);
		m_pWServer->Object_SetDirty(m_iObject,1);
	}

	m_lPhoneSounds[PHONESOUND_DIALTONE_0]		= m_pWServer->GetMapData()->GetResourceIndex_Sound("env_phone_dialtone_0");
	m_lPhoneSounds[PHONESOUND_DIALTONE_1]		= m_pWServer->GetMapData()->GetResourceIndex_Sound("env_phone_dialtone_1");
	m_lPhoneSounds[PHONESOUND_DIALTONE_2]		= m_pWServer->GetMapData()->GetResourceIndex_Sound("env_phone_dialtone_2");
	m_lPhoneSounds[PHONESOUND_DIALTONE_3]		= m_pWServer->GetMapData()->GetResourceIndex_Sound("env_phone_dialtone_3");
	m_lPhoneSounds[PHONESOUND_DIALTONE_4]		= m_pWServer->GetMapData()->GetResourceIndex_Sound("env_phone_dialtone_4");
	m_lPhoneSounds[PHONESOUND_DIALTONE_5]		= m_pWServer->GetMapData()->GetResourceIndex_Sound("env_phone_dialtone_5");
	m_lPhoneSounds[PHONESOUND_DIALTONE_6]		= m_pWServer->GetMapData()->GetResourceIndex_Sound("env_phone_dialtone_6");
	m_lPhoneSounds[PHONESOUND_DIALTONE_7]		= m_pWServer->GetMapData()->GetResourceIndex_Sound("env_phone_dialtone_7");
	m_lPhoneSounds[PHONESOUND_DIALTONE_8]		= m_pWServer->GetMapData()->GetResourceIndex_Sound("env_phone_dialtone_8");
	m_lPhoneSounds[PHONESOUND_DIALTONE_9]		= m_pWServer->GetMapData()->GetResourceIndex_Sound("env_phone_dialtone_9");

	m_lPhoneSounds[PHONESOUND_BUSY]				= m_pWServer->GetMapData()->GetResourceIndex_Sound("env_phone_sign_01");
	m_lPhoneSounds[PHONESOUND_HANGUP]			= m_pWServer->GetMapData()->GetResourceIndex_Sound("env_phone_sign_02");
	m_lPhoneSounds[PHONESOUND_PREDIALTONE]		= m_pWServer->GetMapData()->GetResourceIndex_Sound("env_phone_sign_03");
	m_lPhoneSounds[PHONESOUND_PICKUPOTHEREND]	= m_pWServer->GetMapData()->GetResourceIndex_Sound("env_phone_sign_04");
	m_lPhoneSounds[PHONESOUND_RINGING]			= m_pWServer->GetMapData()->GetResourceIndex_Sound("env_phone_sign_06");

}

void CWObject_Telephone::OnRefresh()
{
	CWObject_TelephoneParent::OnRefresh();

	if(m_bRunPreDial)
		DialNumber();
}

void CWObject_Telephone::DialNumber()
{
	CACSClientData* pCD = GetACSClientData(this);

	if(m_NumbersLeftToDial == 9)
	{
		m_NextDialAtThisTick = m_pWServer->GetGameTick() + TruncToInt(m_pWServer->GetGameTicksPerSecond() * 0.70f);
		CVec3Dfp32 Pos = GetPosition();
		iSound(0) = m_lPhoneSounds[PHONESOUND_PREDIALTONE];

		m_NumbersLeftToDial--;
		return;
	}
	else if(m_NumbersLeftToDial == 8)
	{
		if(m_NextDialAtThisTick < m_pWServer->GetGameTick())
		{
			m_NextDialAtThisTick = m_pWServer->GetGameTick() + TruncToInt(m_pWServer->GetGameTicksPerSecond() * 0.23f);
		}
		else
			return;
	}

	if(m_NumbersLeftToDial > 0)
	{
		if(m_NextDialAtThisTick < m_pWServer->GetGameTick() || m_NumbersLeftToDial == 8) // ready to dial another number
		{
			CStr NumStr =  pCD->m_CurrentNumber.DelFrom(8 - (m_NumbersLeftToDial-1));

			if(m_NumbersLeftToDial < 8)
				NumStr = NumStr.DelTo(8 - (m_NumbersLeftToDial+1));

			iSound(0) = 0;
			CVec3Dfp32 Pos = GetPosition();
			m_pWServer->Sound_At(Pos, m_lPhoneSounds[atoi(NumStr.Str())], WCLIENT_ATTENUATION_3D); // temp 0, insert real numbers here
			m_NumbersLeftToDial--;
			m_NextDialAtThisTick = m_pWServer->GetGameTick() + TruncToInt(m_pWServer->GetGameTicksPerSecond() * 0.23f);
		}
	}

	if(m_NumbersLeftToDial < 1 && m_NumbersLeftToDial > -2)
	{
		if(m_NextDialAtThisTick < m_pWServer->GetGameTick())
		{
			m_NextDialAtThisTick = m_pWServer->GetGameTick() + TruncToInt(m_pWServer->GetGameTicksPerSecond() * 2.0f);
			m_NumbersLeftToDial--;
			CVec3Dfp32 Pos = GetPosition();
			m_pWServer->Sound_At(Pos, m_lPhoneSounds[PHONESOUND_RINGING], WCLIENT_ATTENUATION_3D); // temp 0, insert real numbers here
		}
	}
	else if(m_DialingNumber == -1) // no number
	{
		// call a random wrong number here, or just let ring
		//DoActionSuccess(m_iActiveChar);
		m_NumbersLeftToDial = 0;
		//m_bRunPreDial = false;
		//OnEndACS(m_iActiveChar);
	}
	else if(m_NumbersLeftToDial == -2)
	{
		if(m_NextDialAtThisTick < m_pWServer->GetGameTick())
		{
			m_NextDialAtThisTick = m_pWServer->GetGameTick() + TruncToInt(m_pWServer->GetGameTicksPerSecond() * 0.2f);
			m_NumbersLeftToDial--;
			CVec3Dfp32 Pos = GetPosition();
			m_pWServer->Sound_At(Pos, m_lPhoneSounds[PHONESOUND_PICKUPOTHEREND], WCLIENT_ATTENUATION_3D); // temp 0, insert real numbers here
		}

	}
	else if(m_NumbersLeftToDial == -3)
	{
		if(m_NextDialAtThisTick < m_pWServer->GetGameTick())
		{
			if (pCD)
			{
				pCD->SetChoiceString(STR_TELEPHONE_DIALING);
				m_pWServer->Object_SetDirty(m_iObject,1);
			}

			for(int32 i = 0; i < m_lMsg_StartRing.Len(); i++)
				m_lMsg_StartRing[i].SendMessage(m_iObject, m_iActiveChar, m_pWServer);

			DoActionSuccess(m_iActiveChar);
			m_bRunPreDial = false;
			OnEndACS(m_iActiveChar);
		}
	}
}

void CWObject_Telephone::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	if(KeyName.Find("MSG_STARTRING") != -1)
	{
		m_lMsg_StartRing.SetLen(m_lMsg_StartRing.Len() + 1);
		m_lMsg_StartRing[m_lMsg_StartRing.Len() - 1].Parse(KeyValue, m_pWServer);
	}
	else if(KeyName.Find("MSG_STOPRING") != -1)
	{
		m_lMsg_StopRing.SetLen(m_lMsg_StopRing.Len() + 1);
		m_lMsg_StopRing[m_lMsg_StopRing.Len()-1].Parse(KeyValue, m_pWServer);
	}
	else
		CWObject_TelephoneParent::OnEvalKey(_KeyHash, _pKey);
}

void CWObject_Telephone::OnFinishEvalKeys()
{
	CWObject_TelephoneParent::OnFinishEvalKeys();

	if (m_UseName.Len() == 0)
		m_UseName = STR_TELEPHONE_NORMAL;

	CACSClientData* pCD = GetACSClientData(this);
	if (pCD)
	{
		pCD->SetChoiceString(m_UseName);
		m_pWServer->Object_SetDirty(m_iObject,1);
	}
}

aint CWObject_Telephone::OnMessage(const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJMSG_ACTIONCUTSCENE_TELEPHONE_SELECTCHOICE:
		MoveSelection(_Msg.m_Param0);
		break;

	case OBJMSG_ACTIONCUTSCENE_ISTELEPHONE:
		return m_spGlobalRegistry ? m_spGlobalRegistry->m_iObject : -1;

	case OBJMSG_TELEPHONE_DIAL:
		{
			// Dial a number on ourself
			if (_Msg.m_Param0 == 1)
			{
				// Turn dialing off
				m_DialingNumber = -1;
				ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
				// Set choice string
				CACSClientData* pCD = GetACSClientData(this);
				if (pCD)
				{
					pCD->SetChoiceString(m_UseName);
					m_pWServer->Object_SetDirty(m_iObject,1);
				}

				for(int32 i = 0; i < m_lMsg_StopRing.Len(); i++)
					m_lMsg_StopRing[i].SendMessage(m_iObject, _Msg.m_iSender, m_pWServer);
			}
			else if(_Msg.m_Param0 == 2)
			{
				// Answer phone
				DoActionSuccess(_Msg.m_iSender);
			}
			else
			{
				// Get number from message, check so it exists
				CStr PhoneNumber = (char*)_Msg.m_pData;
				m_DialingNumber = m_spGlobalRegistry ? m_spGlobalRegistry->FindPhoneNumber(PhoneNumber) : -1;
				// Start refreshing
				if (m_DialingNumber != -1)
				{
					CACSClientData* pCD = GetACSClientData(this);
					pCD->m_CurrentNumber = PhoneNumber;
					m_bRunPreDial = true;
					m_iActiveChar = _Msg.m_iSender;
					m_NumbersLeftToDial = 9;

					ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
				}
			}
		}
		break;

	case OBJMSG_CHAR_USE:
		{
			CACSClientData* pCD = GetACSClientData(this);
			if (pCD && pCD->m_iTelephoneRegistry)
			{
				CWObject* pObj = m_pWServer->Object_Get(pCD->m_iTelephoneRegistry);
				CWO_Character_ClientData* pCharCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
				if (pCharCD && pCharCD->m_liDialogueChoice.Len())
					return pObj->OnMessage(_Msg);
			}
			return CWObject_TelephoneParent::OnMessage(_Msg);
		}

	default:
		return CWObject_TelephoneParent::OnMessage(_Msg);
	}
	return 1;
}

aint CWObject_Telephone::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJMSG_CHAR_GETCHOICES:
		{
			// If we can't activate this acs, don't set any choices?
			if (!CanActivateClient(_pWClient, _pObj, _Msg.m_iSender))
				return 0;

			if(_pObj->m_iAnim2 != 0)
				return _pWClient->ClientMessage_SendToObject(_Msg, _pObj->m_iAnim2);

			// Check if we have actual dialogue choices
			CACSClientData* pCD = GetACSClientData(_pObj);
			if (pCD->m_iTelephoneRegistry)
			{
				int nDialogueChoices = _pWClient->ClientMessage_SendToObject(_Msg, pCD->m_iTelephoneRegistry);
				if (nDialogueChoices)
					return nDialogueChoices;
			}

			CStr *pSt = (CStr *)_Msg.m_pData;
			if (pSt)
				*pSt = pCD->m_ChoiceString;
			return 1;
		}
	case OBJMSG_ACTIONCUTSCENE_CANACTIVATE:
		{
			// Check if the actioncutscene can be activated (ie character holds correct items..)
			// and such
			// Assuming sender is the character id
			if (_Msg.m_iSender != -1)
				return CanActivateClient(_pWClient, _pObj, _Msg.m_iSender);

			return 0;
		}
	default:
		return CWObject_TelephoneParent::OnClientMessage(_pObj, _pWClient, _Msg);
	}
}

void CWObject_Telephone::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	CWObject_TelephoneParent::OnIncludeClass(_pWData, _pWServer);
	_pWData->GetResourceIndex_Sound("env_phone_02");
}

void CWObject_Telephone::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	CWObject_TelephoneParent::OnIncludeTemplate(_pReg, _pMapData, _pWServer);
}

void CWObject_Telephone::MoveSelection(int32 _iChoice)
{
	enum Types
	{
		LEFT	= 1,
		RIGHT	= 2,
		UP		= 3,
		DOWN	= 4,
		SELECT	= 5,
		QUIT	= 6,
	};
	CACSClientData* pCD = GetACSClientData(this);

	if(((pCD->m_CurrentNumber.Len() >= 5 && pCD->m_CurrentNumber.Find("555") != 0) || pCD->m_CurrentNumber.Len() == 8) && _iChoice != QUIT)
		return;

	bool bSetDirty = true;
	bool bSetSelection = true;
	bool bPushedButton = false;
	CVec2Dint32 Selection = pCD->m_CurrentSelection;
	switch (_iChoice)
	{
	case LEFT:
		{
			Selection.k[0] = Max((int32)0,pCD->m_CurrentSelection.k[0] - 1);
			break;
		}
	case RIGHT:
		{
			Selection.k[0] = Min((int32)2,pCD->m_CurrentSelection.k[0] + 1);
			break;
		}
	case UP:
		{
			Selection.k[1] = Max((int32)0,pCD->m_CurrentSelection.k[1] - 1);
			break;
		}
	case DOWN:
		{
			Selection.k[1] = Min((int32)3,pCD->m_CurrentSelection.k[1] + 1);
			break;
		}
	case SELECT:
		{
			iSound(0) = 0; // quit predial tone

			// Add number to current string
			bPushedButton = true;
			bSetSelection = false;
			const char Number[] = { '1','2','3','4','5','6','7','8','9','*','0','#'};
			int Index = pCD->m_CurrentSelection.k[1] * 3 + pCD->m_CurrentSelection.k[0];
			if (Index < sizeof(Number))
			{
				//if(Index == 9 && pCD->m_CurrentNumber == "555")
				//	pCD->m_CurrentNumber = "";

				// Update number
				pCD->SetCurrentNumber(pCD->m_CurrentNumber + CStr(Number[Index]));

				// play correct sound
				CVec3Dfp32 Pos = GetPosition();
				Index = Min(Index, 9);
				m_pWServer->Sound_At(Pos, m_lPhoneSounds[Index], 0);
			}

			break;
		}
	case QUIT:
		{
			// Exit acs
			m_NumbersLeftToDial = 0;
			m_bRunPreDial = false;

			iSound(0) = 0; // quit predial tone
			OnEndACS(m_iActiveChar);
			bSetDirty = false;
			break;
		}
	default:
		{
			iSound(0) = 0; // quit predial tone
			bSetDirty = false;
			break;
		}
	}

	if (bSetDirty)
	{
		m_pWServer->Object_SetDirty(m_iObject,1);
		m_DirtyMask |= 1 << CWO_DIRTYMASK_USERSHIFT;
	}

	if (bSetSelection)
		pCD->SetCurrentSelection(Selection);

	if (bPushedButton)
	{
		// Stop looping sound (if active)
		if (m_iSound[0] != 0)
		{
			m_iSound[0] = 0;
			m_DirtyMask |= CWO_DIRTYMASK_SOUND;
		}
	}

	//ConOut(CStr("CurrentSelection: [%d][%d] Number: %s",pCD->m_CurrentSelection[0],pCD->m_CurrentSelection[1],pCD->m_CurrentNumber.GetStr()));

	// Check for telephone number match...
	if (pCD->m_CurrentNumber.Len() >= 3)
	{
		//pCD->m_CurrentNumber[0] == '*'

		if(pCD->m_CurrentNumber.Len() >= 5)
		{
			if(pCD->m_CurrentNumber.Find("555") != 0)// || ) // did the dialer mean to use the 555 code or not?
			{
				CStr Tmp = "555" + pCD->m_CurrentNumber;
				if(m_spGlobalRegistry)
				{
					// Check if we found a match for number
					m_DialingNumber = m_spGlobalRegistry ? m_spGlobalRegistry->FindPhoneNumber(Tmp) : -1;

					// Start refreshing
					m_bRunPreDial = true;
					ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
					m_NumbersLeftToDial = 0;
				}
			}
			else if(pCD->m_CurrentNumber.Len() >= 8)
			{
				if(m_spGlobalRegistry)
				{
					// Check if we found a match for number
					m_DialingNumber = m_spGlobalRegistry ? m_spGlobalRegistry->FindPhoneNumber(pCD->m_CurrentNumber) : -1;

					// Start refreshing
					m_bRunPreDial = true;
					ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
					m_NumbersLeftToDial = 0;
				}
			}
		}

		//OnEndACS(m_iActiveChar);
	}
}

bool CWObject_Telephone::CanActivate(int _iCharacter)
{
	if (m_spGlobalRegistry)
	{
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_spGlobalRegistry);
		if (pCD && pCD->m_DialogueInstance.IsValid())
			return false;
	}

	return CWObject_TelephoneParent::CanActivate(_iCharacter);
}

bool CWObject_Telephone::CanActivateClient(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, int _iCharacter)
{
	CACSClientData* pCDACS = GetACSClientData(_pObj);
	if (pCDACS->m_iTelephoneRegistry)
	{
		CWObject_CoreData* pObj = _pWPhys->Object_GetCD(pCDACS->m_iTelephoneRegistry);
		CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
		if (pCD && pCD->m_DialogueInstance.IsValid())
			return false;
	}

	return CWObject_TelephoneParent::CanActivateClient(_pWPhys, _pObj, _iCharacter);
}

bool CWObject_Telephone::DoActionSuccess(int _iCharacter)
{
	// If the telephone reg is currently talking, return
	if (m_spGlobalRegistry)
	{
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(m_spGlobalRegistry);
		if (pCD && pCD->m_DialogueInstance.IsValid())
			return false;
	}

	if (m_DialingNumber != -1)
	{
		// Call the number, do nothing else
		bool bResult = m_spGlobalRegistry->CallNumber(m_DialingNumber, _iCharacter, GetPosition());
		// Stop dialing tone
		ClientFlags() |= CWO_CLIENTFLAGS_NOREFRESH;
		m_DialingNumber = -1;
		CACSClientData* pCD = GetACSClientData(this);
		if (pCD)
			pCD->SetChoiceString(m_UseName);

		for(int32 i = 0; i < m_lMsg_StopRing.Len(); i++)
			m_lMsg_StopRing[i].SendMessage(m_iObject, _iCharacter, m_pWServer);

		return bResult;
	}
	else
	{
		// Send impulse 1 to the base class when successful
		OnImpulse(1, _iCharacter);
		CACSClientData *pCD = GetACSClientData(this);

		// Hmm, ok we need the same functionality on the new animation system thingy
		CWObject* pObjChar = m_pWServer->Object_Get(_iCharacter);
		CWO_Character_ClientData* pCDChar = (pObjChar ? CWObject_Character::GetClientData(pObjChar) : NULL);

		if (!pCDChar)
			return false;

		m_iActiveChar = _iCharacter;

		// Clear
		//pCD->Clear();
		pCD->SetChoiceString(m_UseName);
		//pCD->SetDirtyAll();
		m_pWServer->Object_SetDirty(m_iObject,1);
		m_DirtyMask |= 1 << CWO_DIRTYMASK_USERSHIFT;
		// Start client window
		char *pWndName = "telephone";
		CWObject_Message Msg(OBJMSG_GAME_SETCLIENTWINDOW, aint(pWndName),
			m_pWServer->Game_GetObject()->Player_GetClient(pCDChar->m_iPlayer));

		Msg.m_iSender = m_iObject;
		m_pWServer->Message_SendToTarget(Msg, (char*)WSERVER_GAMEOBJNAME);

		pCD->m_ActionRetryCountdown = m_pWServer->GetGameTick() + m_ActionRetryCountdownValue;
		// Enter actioncutscene controlmode
		CWObject_Character::Char_SetControlMode(pObjChar, PLAYER_CONTROLMODE_ACTIONCUTSCENE);
		((CWObject_Character *)pObjChar)->UpdateVisibilityFlag();

		// Set action cutscene id to controlmode param0
		pCDChar->m_ControlMode_Param0 = (fp32)m_iObject;

		// Set cutscenetype to param2
		pCDChar->m_ControlMode_Param2 = ACTIONCUTSCENE_TYPE_TELEPHONE;

		// Ok determine what type of cutscene we're having and deal with positioning accordingly
		// Only the hatch animations should be able to move around (and thus use the old positioning)
		// Filter out success actions and set to param4
		int32 Flags = ((pCD->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_SUCCESSMASK) & 
			~(ACTIONCUTSCENE_OPERATION_SETSTARTPOSMASK/*|ACTIONCUTSCENE_OPERATION_SETENDPOSMASK*/)) | 
			ACTIONCUTSCENE_FLAGS_ISSUCCESS | ACTIONCUTSCENE_FLAGS_USEFLOATINGSTART;
		if (m_ActionCutsceneTypeSuccess == ACTIONCUTSCENE_TYPE_USEHEALTHSTATION)
			Flags |= ACTIONCUTSCENE_OPERATION_USEPERFECTPOS;
		pCDChar->m_ControlMode_Param4 = Flags;

		// Make sure the camera will be updated when the ACS is active
		ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

		// Start activation timer
		if (m_MinActivationTime > 0.0f)
			m_MinActivationTimer = m_pWServer->GetGameTick() + 
			(int)(m_MinActivationTime * m_pWServer->GetGameTicksPerSecond());
		else
			m_MinActivationTimer = 0;

		// Play sound idle sound
		//m_iSound[0] = m_iSoundWaiting;
		//m_pWServer->Sound_On(m_iObject, m_iSound[0], 2);
		m_DirtyMask |= CWO_DIRTYMASK_SOUND;

		pCD->SetCurrentNumber("");
		pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_FLAGS_TELEPHONEACTIVE);

		iSound(0) = m_lPhoneSounds[PHONESOUND_PREDIALTONE];

		return true;
	}
}

bool CWObject_Telephone::OnEndACS(int _iCharacter)
{
 	CWObject_TelephoneParent::OnEndACS(_iCharacter);
	CWObject* pObjChar = m_pWServer->Object_Get(_iCharacter);
	CWO_Character_ClientData* pCDChar = (pObjChar ? CWObject_Character::GetClientData(pObjChar) : NULL);
	if (!pCDChar)
		return false;

	// Clear client window
	CWObject_Message Msg(OBJMSG_GAME_SETCLIENTWINDOW);
	Msg.m_iSender = m_iObject;
	m_pWServer->Message_SendToTarget(Msg, (char*)WSERVER_GAMEOBJNAME);

	// Send unarmed weapon impulse
	CWAG2I_Context AGContext(pObjChar,m_pWServer,pCDChar->m_GameTime);
	pCDChar->m_AnimGraph2.GetAG2I()->SendImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,AG2_IMPULSEVALUE_WEAPONTYPE_UNARMED),0);

	// Stop looping sound
	if (m_iSound[0] != 0)
	{
		m_iSound[0] = 0;
		m_DirtyMask |= CWO_DIRTYMASK_SOUND;
	}

	if (m_DialingNumber != -1)
		ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

	// Reset selection
	CACSClientData* pCDACS = GetACSClientData(this);
	pCDACS->SetCurrentNumber("");
	pCDACS->SetCurrentSelection(CVec2Dint32(0,0));
	pCDACS->SetCutsceneFlags(pCDACS->m_CutsceneFlags & ~ACTIONCUTSCENE_FLAGS_TELEPHONEACTIVE);

	m_pWServer->Object_SetDirty(m_iObject,1);

	return true;
}

/*int CWObject_Telephone::OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pObj, uint8* _pData, int _Flags) const
{
CACSClientData* pCD = const_cast<CACSClientData*>(GetACSClientData(this));
if(!pCD)
Error_static("CWObject_Telephone::OnCreateClientUpdate", "Unable to pack client update.");

int Flags = CWO_CLIENTUPDATE_EXTRADATA;
if(_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT)
Flags = CWO_CLIENTUPDATE_AUTOVAR;
uint8* pD = _pData;
pD += CWObject_TelephoneParent::OnCreateClientUpdate(_iClient, _pClObjInfo, _pObj, _pData, Flags);
if (pD - _pData == 0)
return pD - _pData;

pD += pCD->OnCreateClientUpdate(pD);

return (uint8*)pD - _pData;
}

int CWObject_Telephone::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
MAUTOSTRIP(CWObject_Telephone_OnClientUpdate, 0);
MSCOPESHORT(CWObject_Telephone::OnClientUpdate);

const uint8* pD = &_pData[CWObject_TelephoneParent::OnClientUpdate(_pObj, _pWClient, _pData, _Flags)];
if (_pObj->m_iClass == 0 || pD - _pData == 0) return pD - _pData;

CACSClientData *pCD = GetACSClientData(_pObj);

pD += pCD->OnClientUpdate(pD);

return (uint8*)pD - _pData;
}*/

void CWObject_Telephone::GetCurrentNumber(CWorld_PhysState* _pWPhys, int32 _iObject, CStr& _Number)
{
	CWObject_CoreData* pObj = _pWPhys->Object_GetCD(_iObject);
	CACSClientData* pCD = (pObj ? GetACSClientData(pObj) : NULL);
	if (pCD)
	{
		_Number = pCD->m_CurrentNumber;
	}
}

void CWObject_Telephone::GetCurrentSelection(CWorld_PhysState* _pWPhys, int32 _iObject, CVec2Dint32& _Selection)
{
	CWObject_CoreData* pObj = _pWPhys->Object_GetCD(_iObject);
	CACSClientData* pCD = (pObj ? GetACSClientData(pObj) : NULL);
	if (pCD)
	{
		_Selection = pCD->m_CurrentSelection;
	}
}

void CWObject_Telephone::OnDeltaSave(CCFile* _pFile)
{
	CWObject_TelephoneParent::OnDeltaSave(_pFile);
	_pFile->WriteLE(m_DialingNumber);
	_pFile->WriteLE(m_LastDialTone);
	// Save clientflags
	_pFile->WriteLE(m_ClientFlags);
}
void CWObject_Telephone::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	CWObject_TelephoneParent::OnDeltaLoad(_pFile, _Flags);
	_pFile->ReadLE(m_DialingNumber);
	_pFile->ReadLE(m_LastDialTone);
	int32 Temp;
	_pFile->ReadLE(Temp);
	ClientFlags() = Temp;
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Telephone, CWObject_TelephoneParent, 0x0100);



void CWObject_TelephoneRegistry::OnCreate()
{
	// Hmm, not too much here yet
	CWObject_TelephoneRegistryParent::OnCreate();
	// Set 0 physics
	CWO_PhysicsState Phys;
	Phys.Clear();
	m_pWServer->Object_SetPhysics(m_iObject, Phys);
	m_pWServer->Object_SetName(m_iObject, "TELEPHONEREG");
	ClientFlags() |= PLAYER_CLIENTFLAGS_NOGRAVITY;

	CWO_Character_ClientData *pCD = GetClientData(this);
	pCD->m_Phys_Flags = pCD->m_Phys_Flags | PLAYER_PHYSFLAGS_NOWORLDCOLL;
}

void CWObject_TelephoneRegistry::OnSpawnWorld()
{
	// Not much here either yet
	CWObject_TelephoneRegistryParent::OnSpawnWorld();
	// Set our name
	Char_SetPhysType(this,PLAYER_PHYS_NOCLIP);
}

// Should only be one instance of this class
void CWObject_TelephoneRegistry::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	MAUTOSTRIP(CWObject_TelephoneRegistry_OnIncludeTemplate, MAUTOSTRIP_VOID);
	MSCOPE(CWObject_TelephoneRegistry::OnIncludeTemplate, WOBJECT_TelephoneRegistry);
	CWObject_TelephoneRegistryParent::OnIncludeTemplate(_pReg, _pMapData, _pWServer);

	if(_pReg)
	{
		CRegistry *pDialogue = _pReg->FindChild("dialogue");
		if(pDialogue)
		{
			CWObject_RPG::IncludeDialogue(pDialogue->GetThisValue(), _pMapData);
		}
	}
}

void CWObject_TelephoneRegistry::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{

	MAUTOSTRIP(CWObject_TelephoneRegistry_OnEvalKey, MAUTOSTRIP_VOID);
	switch (_KeyHash)
	{
	case MHASH3('PHON','EBOO','K'): // "PHONEBOOK"
		{
			int32 Len = _pKey->GetNumChildren();
			for (int32 i = 0; i < Len; i++)
			{
				const CRegistry* pChild = _pKey->GetChild(i);
				if (pChild)
				{
					/*CStr DialogItem = pChild->GetThisName();
					CStr ChildName = DialogItem.GetStrSep(":");
					int32 iDialogItem = DialogItem.Val_int();
					// Needed?
					int32 iDialog = CWObject_RPG::IncludeDialogue(ChildName, m_pWServer->GetMapData());
					if (iDialog <= 0)
					{
					ConOut(CStr("CWObject_TelephoneRegistry WARNING DIALOG: %s NOT FOUND",pChild->GetThisName()));
					}
					AddPhoneNumber(pChild->GetThisValue(),iDialogItem);//iDialog);*/

					//AddPhoneNumber(pChild->GetThisName(), pChild->GetThisValue().Val_int());
					AddPhoneNumber(pChild->GetThisName(), pChild->GetThisValue());
				}
			}
			break;
		}
	default:
		{
			CWObject_TelephoneRegistryParent::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

aint CWObject_TelephoneRegistry::OnMessage(const CWObject_Message& _Msg)
{
	if (_Msg.m_Msg == OBJMSG_PHONEBOOK_CHANGEDIALOGCHOICE)
	{
		// Change dialog choice of given number
		if (_Msg.m_pData)
		{
			CStr CombData = (const char*)_Msg.m_pData;
			CStr PhoneNumber = CombData.GetStrSep(",");
			CStr DialogeLink = CombData.GetStrSep(",");
			return ChangeDialog(PhoneNumber, DialogeLink);
		}
		return false;
	}
	else if (_Msg.m_Msg == OBJMSG_CHAR_CANBEDROPKILLED)
	{
		return false;
	}
	else
	{
		return CWObject_TelephoneRegistryParent::OnMessage(_Msg);
	}
}

aint CWObject_TelephoneRegistry::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	if(_Msg.m_Msg == OBJMSG_ACTIONCUTSCENE_ISTELEPHONE)
		return 1;
	return CWObject_TelephoneRegistryParent::OnClientMessage(_pObj, _pWClient, _Msg);
}

bool CWObject_TelephoneRegistry::AddPhoneNumber(const CStr& _PhoneNumber, const CStr& _DialogLink)
{
	if (_DialogLink.IsEmpty())
		return false;

	if (_PhoneNumber.Find("DEFAULT") != -1)
	{
		// Default numbers
		m_lWrongNumbers.Add(CDialogLink(_PhoneNumber, _DialogLink));
		return true;
	}
	else if (FindPhoneNumber(_PhoneNumber) == -1)
	{
		// Add a new phone entry
		m_lPhoneBook.Add(CDialogLink(_PhoneNumber, _DialogLink));
		return true;
	}

	return false;
}

void CWObject_TelephoneRegistry::AddPhoneNumber(const CStr& _PhoneNumber, const CNameHash _Hash)
{
	for(int i = 0; i < m_lPhoneBook.Len(); i++)
	{
		if(m_lPhoneBook[i].m_PhoneNumber == _PhoneNumber)
		{
			if(m_lPhoneBook[i].m_DialogLink != _Hash)
				m_lPhoneBook[i].m_DialogLink = _Hash;
			return;
		}
	}

	m_lPhoneBook.Add(CDialogLink(_PhoneNumber, _Hash));
}

int CWObject_TelephoneRegistry::FindPhoneNumber(const CStr& _PhoneNumber)
{
	int32 Len = m_lPhoneBook.Len();

	for (int32 i = 0; i < Len; i++)
	{
		if (m_lPhoneBook[i].m_PhoneNumber == _PhoneNumber)
			return i;
		//return  m_lPhoneBook[i].m_DialogLink;
	}

	return -1;
}

/* not used
int CWObject_TelephoneRegistry::FindPhoneNum(const CStr& _PhoneNumber)
{
	int32 Len = m_lPhoneBook.Len();

	int32 itr;
	for(itr = 0; itr < Len; itr++)
	{
		if(m_lPhoneBook[itr].m_PhoneNumber == _PhoneNumber)
		{
			const char *str = _PhoneNumber.GetStr();
			int dlink = m_lPhoneBook[itr].m_DialogLink;
			return  m_lPhoneBook[itr].m_DialogLink;
		}
	}

	return -1;
}
*/

/* not used?
bool CWObject_TelephoneRegistry::CallWrongNumber(const CStr& _Number, int32 _iChar, const CVec3Dfp32& _CallPos)
{
	if (m_lWrongNumbers.Len() <= 0)
		return false;

	// Call a random number in the "wrong number" phonebook
	int32 iNumber = StringToHash(_Number) % m_lWrongNumbers.Len();

	// Set our own position to the given one (so dialog camera will be reasonable correct, same for
	// sound), just to be sure, set 0 physics as well
	CWO_PhysicsState Phys;
	Phys.Clear();
	m_pWServer->Object_SetPhysics(m_iObject,Phys);
	m_pWServer->Object_SetPosition(m_iObject,_CallPos);

	// Ok, found a call match start dialog (conout for now..)
	//ConOut(CStr("Found a match for number: %s DialogLink: %d",_Number.GetStr(),m_lPhoneBook[iNumber].m_DialogLink));
	CWObject_Message Msg(OBJMSG_CHAR_BEGINDIALOGUE, m_lWrongNumbers[iNumber].m_DialogLink);
	Msg.m_iSender = m_iObject;
	Msg.m_Reason = 1;
	m_pWServer->Message_SendToObject(Msg, _iChar);//<--?? who to send to...
	PlayDialogue(m_lWrongNumbers[iNumber].m_DialogLink, 0, 0);
	CWObject_CoreData* pTarget = m_pWServer->Object_GetCD(m_iListener);
	if(pTarget)
	{
		CMat4Dfp32 CallPos;
		(-pTarget->GetPositionMatrix().GetRow(0)).SetRow(CallPos,0);
		CVec3Dfp32(0.0f,0.0f,1.0f).SetRow(CallPos,2);
		CallPos.RecreateMatrix(0,2);
		_CallPos.SetRow(CallPos,3);
		m_pWServer->Object_SetPosition(m_iObject, CallPos);
	}
	return true;
}
*/

// _iNumber is the dialogue link
bool CWObject_TelephoneRegistry::CallNumber(int32 _iNumber, int32 _iChar, const CVec3Dfp32& _CallPos)
{
	if (!m_lPhoneBook.ValidPos(_iNumber))
		return false;

	CWO_Character_ClientData *pCD = GetClientData(this);
	pCD->m_pCurrentDialogueToken = NULL;

	// Set our own position to the given one (so dialog camera will be reasonable correct, same for
	// sound), just to be sure, set 0 physics as well
	CWO_PhysicsState Phys;
	Phys.Clear();
	m_pWServer->Object_SetPhysics(m_iObject,Phys);
	m_pWServer->Object_SetPosition(m_iObject, _CallPos);

	// Ok, found a call match start dialog (conout for now..)
	//ConOut(CStr("Found a match for number: %s DialogLink: %d",_Number.GetStr(),m_lPhoneBook[iNumber].m_DialogLink));
	const CDialogLink& Link = m_lPhoneBook[_iNumber];
	ConOutL(CStrF("[Telephone] Calling '%s' (%s)...", Link.m_PhoneNumber.Str(), Link.m_DialogLink.DbgName().Str())); 
	CWObject_Message Msg(OBJMSG_CHAR_BEGINDIALOGUE, Link.m_DialogLink);
	Msg.m_iSender = m_iObject;
	Msg.m_Reason = 1;
	m_pWServer->Message_SendToObject(Msg, _iChar);//<--?? who to send to...*/
	PlayDialogue_Hash(m_lPhoneBook[_iNumber].m_DialogLink, 0, 0);
	CWObject_CoreData* pTarget = m_pWServer->Object_GetCD(m_iListener);
	if(pTarget)
	{
		CMat4Dfp32 CallPos;

//		CallPos.Unit();
//		(-pTarget->GetPositionMatrix().GetRow(0)).SetRow(CallPos,0);
//		CVec3Dfp32(0.0f,0.0f,1.0f).SetRow(CallPos,2);
//		CallPos.RecreateMatrix(0,2);
//		_CallPos.SetRow(CallPos,3);

		CallPos.r[0] = M_VNeg(pTarget->GetPositionMatrix().r[0]);
		CallPos.r[2] = M_VConst(0,0,1.0f,0);	
		CallPos.RecreateMatrix<0,2>();
		CallPos.r[3] = M_VLd_P3_Slow(&_CallPos);
		m_pWServer->Object_SetPosition(m_iObject, CallPos);

		CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(pTarget);
		pCD->m_3PI_Mode = THIRDPERSONINTERACTIVE_MODE_DIALOGUE;
		if(!pCD->m_3PI_LightFadeStart == THIRDPERSONINTERACTIVE_LIGHT_STATE_FADE_IN )
		{
			pCD->m_3PI_LightFadeStart = m_pWServer->GetGameTick();
			pCD->m_3PI_LightState = THIRDPERSONINTERACTIVE_LIGHT_STATE_FADE_IN;
		}
	}

	return true;
}

bool CWObject_TelephoneRegistry::CallNumber(const CStr& _Number, int32 _iChar, const CVec3Dfp32& _CallPos)
{
	return CallNumber(FindPhoneNumber(_Number), _iChar, _CallPos);
}

bool CWObject_TelephoneRegistry::ChangeDialog(const CStr& _PhoneNumber, const CStr& _DialogLink)
{
	// Change the dialogue item of a phone number
	int iNumber = FindPhoneNumber(_PhoneNumber);
	if (iNumber != -1)
	{
		m_lPhoneBook[iNumber].m_DialogLink = _DialogLink;
		return true;
	}
	ConOutL(CStrF("WARNING: ChangeDialog, number '%s' was not found (update ignored)", _PhoneNumber.Str()));
	return false;
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_TelephoneRegistry, CWObject_TelephoneRegistryParent, 0x0100);


void GetTexCoords(CVec2Dfp32* pTV, char _Num)
{
	fp32 XStart,YStart,XEnd,YEnd;
	switch (_Num)
	{
	case '0':
		{
			XStart = 0.25f;
			YStart = 0.75f;
			break;
		}
	case '1':
		{
			XStart = 0.0f;
			YStart = 0.0f;
			break;
		}
	case '2':
		{
			XStart = 0.25f;
			YStart = 0.0f;
			break;
		}
	case '3':
		{
			XStart = 0.5f;
			YStart = 0.0f;
			break;
		}
	case '4':
		{
			XStart = 0.0f;
			YStart = 0.25f;
			break;
		}
	case '5':
		{
			XStart = 0.25f;
			YStart = 0.25f;
			break;
		}
	case '6':
		{
			XStart = 0.5f;
			YStart = 0.25f;
			break;
		}
	case '7':
		{
			XStart = 0.0f;
			YStart = 0.5f;
			break;
		}
	case '8':
		{
			XStart = 0.25f;
			YStart = 0.5f;
			break;
		}
	case '9':
		{
			XStart = 0.5f;
			YStart = 0.5f;
			break;
		}
	case '#':
		{
			XStart = 0.5f;
			YStart = 0.75f;
			break;
		}
	case '*':
		{
			XStart = 0.0f;
			YStart = 0.75f;
			break;
		}
	case 'M':
		{
			XStart = 0.75f;
			YStart = 0.0f;
			break;
		}
	default:
		{
			XStart = 0.0f;
			YStart = 0.0f;
			break;
		}
	}

	XEnd = XStart + 0.25f;
	YEnd = YStart + 0.25f;
	pTV[0][0] = XStart;
	pTV[0][1] = YStart;

	pTV[1][0] = XEnd;
	pTV[1][1] = YStart;

	pTV[2][0] = XEnd;
	pTV[2][1] = YEnd;

	pTV[3][0] = XStart;
	pTV[3][1] = YEnd;
}


void RenderSquare(CRC_Util2D *_pUtil2D, const CClipRect& _Clip, const CRct& _Rect, char _Key)
{
	MAUTOSTRIP(CRC_Util2D_Rect, MAUTOSTRIP_VOID);

	int32 _Color = 0x00ffffff;//CPixel32((50/2),(50/2),(50/2),255);

	CXR_VBManager* pVBM = _pUtil2D->GetVBM();
	if (!_Clip.Visible(_Rect) || !pVBM) return;

	_pUtil2D->SetTexture("SPECIAL_NUMPAD");
	CRct Rect = _Rect;
	Rect += _Clip;

	Rect.p0.x = Max( Rect.p0.x, _Clip.clip.p0.x );
	Rect.p1.x = Min( Rect.p1.x, _Clip.clip.p1.x );
	Rect.p0.y = Max( Rect.p0.y, _Clip.clip.p0.y );
	Rect.p1.y = Min( Rect.p1.y, _Clip.clip.p1.y );

	//	CVec3Dfp32 Verts[4];
	CVec3Dfp32 Verts3D[4];
	CVec2Dfp32 TVerts[4];
	CPixel32 Colors[4];
	CVec3Dfp32 *pV = Verts3D;
	CVec2Dfp32 *pTV = TVerts;
	CPixel32 *pC = Colors;

	pV = pVBM->Alloc_V3(4);
	pTV = pVBM->Alloc_V2(4);
	pC = pVBM->Alloc_CPixel32(4);

	if(!pV || !pTV || !pC)
		return;

	pV[0].k[0] = Rect.p0.x;		pV[0].k[1] = Rect.p0.y;		pV[0].k[2] = 0;
	pV[1].k[0] = Rect.p1.x;		pV[1].k[1] = Rect.p0.y;		pV[1].k[2] = 0;
	pV[2].k[0] = Rect.p1.x;		pV[2].k[1] = Rect.p1.y;		pV[2].k[2] = 0;
	pV[3].k[0] = Rect.p0.x;		pV[3].k[1] = Rect.p1.y;		pV[3].k[2] = 0;
	CVec3Dfp32::MultiplyMatrix(pV, pV, _pUtil2D->GetTransform(), 4);

	//	fp32 xs = 1.0f/fp32(640);
	//	fp32 ys = 1.0f/fp32(480);
	//	int txtox = 0;//_pUtil2D->m_CurTextureOrigo.x;
	//	int txtoy = 0;//_pUtil2D->m_CurTextureOrigo.y;
	GetTexCoords(pTV,_Key);
	/*pTV[0][0] = 0.0f;
	pTV[0][1] = 0.0f;
	pTV[1][0] = 1.0f;
	pTV[1][1] = 0.0f;
	pTV[2][0] = 1.0f;
	pTV[2][1] = 1.0f;
	pTV[3][0] = 0.0f;
	pTV[3][1] = 1.0f;*/
	pC[0] = _Color;
	pC[1] = _Color;
	pC[2] = _Color;
	pC[3] = _Color;

	CXR_VertexBuffer *pVB = pVBM->Alloc_VB();
	if(pVB)
	{
		static uint16 ms_DualTringle[6] = { 0,1,2,0,2,3 };
		pVB->m_Priority = 1.0f;
		if (!pVB->AllocVBChain(pVBM, false))
			return;
		pVB->Geometry_VertexArray(pV, 4, true);
		pVB->Geometry_TVertexArray(pTV, 0);
		pVB->Render_IndexedTriangles(ms_DualTringle, 2);

		/*if(_pUtil2D->m_pSurf)
		{
		pVB->Geometry_ColorArray(pC);
		CXR_Util::Render_Surface(0, _pUtil2D->m_pSurf, _pUtil2D->m_pSurfKeyFrame, NULL, _pUtil2D->m_pVBM, NULL, NULL, (CMat4Dfp32*)NULL, pVB, _pUtil2D->m_VBPriority, 0.0001f);
		}
		else*/
		{
			//pVB->Geometry_Color(_Color);
			pVB->m_pAttrib = pVBM->Alloc_Attrib();
			if(pVB->m_pAttrib)
			{
				pVB->m_pAttrib->SetDefault();
				int32 TextureID = _pUtil2D->GetTC()->GetTextureID("SPECIAL_NUMPAD");
				pVB->m_pAttrib->Attrib_TextureID(0, TextureID);
				pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
				pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
				pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL);
				pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
				pVBM->AddVB(pVB);
				//					if(m_bAutoFlush)
				//						m_pVBM->Flush(m_pCurRC, 0);
			}
		}
	}
}

void CWObject_Telephone::RenderPad(CWorld_Client* _pWClient, CRC_Util2D *_pUtil2D, CWObject_CoreData* pObj, CWO_Character_ClientData* _pCD)
{
	if ((CWObject_Character::Char_GetControlMode(pObj) != PLAYER_CONTROLMODE_ACTIONCUTSCENE) ||
		(int)_pCD->m_ControlMode_Param2 != ACTIONCUTSCENE_TYPE_TELEPHONE)
		return;


	CWObject_CoreData* pTele = _pWClient->Object_GetCD((int)_pCD->m_ControlMode_Param0);
	if (pTele)
	{
		CACSClientData* pCD = GetACSClientData(pTele);
		if (pCD && pCD->m_TelePhoneActivationTick > (_pWClient->GetGameTick() - 2))
			return;
	}

	CClipRect Clip(0, 0, 640, 480);
	// Test (for telephone render in the future...)
	int32 PadSizeX = 30;
	int32 PadSizeY = 40;
	int32 StartX = (640 - PadSizeX * 3) / 2;
	int32 StartY = (480 - PadSizeY * 4) / 2;

	char Chars[] = {'1','2','3','4','5','6','7','8','9','*','0','#'};
	for (int32 y = 0; y < 4; y++)
	{
		for (int32 i = 0; i < 3; i++)
		{
			int32 CoordX = StartX + (i % 3) * PadSizeX;
			int32 CoordY = StartY + (y % 4) * PadSizeY;
			char Key = Chars[y*3 + i];
			RenderSquare(_pUtil2D,Clip,CRct(CoordX,CoordY , CoordX + PadSizeX, CoordY + PadSizeY),Key);
		}
	}
	// Render current selection
	CVec2Dint32 CurrentSelection = 0;
	CWObject_Telephone::GetCurrentSelection(_pWClient,(int)_pCD->m_ControlMode_Param0,CurrentSelection);
	int32 CoordX = StartX + CurrentSelection.k[0] * PadSizeX;
	int32 CoordY = StartY + CurrentSelection.k[1] * PadSizeY;
	RenderSquare(_pUtil2D,Clip,CRct(CoordX,CoordY , CoordX + PadSizeX, CoordY + PadSizeY),'M');

	// Render current phone number
	CStr PhoneNumber;

	CWObject_Telephone::GetCurrentNumber(_pWClient,(int)_pCD->m_ControlMode_Param0,PhoneNumber);

	PadSizeX = 20;
	StartY = (480 - PadSizeY * 4) / 2 - 30;
	PadSizeY = 20;
	StartX = ((640 - PadSizeX * PhoneNumber.Len()) / 2) + 45;

	/*
	int32 Len = PhoneNumber.Len();
	for (int32 i = 0; i < Len; i++)
	{
	int32 CoordX = StartX + i * PadSizeX;
	int32 CoordY = StartY;
	char Key = PhoneNumber.GetStr()[i];
	RenderSquare(_pUtil2D,Clip,CRct(CoordX,CoordY , CoordX + PadSizeX, CoordY + PadSizeY),Key);
	}
	*/
	//
	fp32 FontSize = 16;
	CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("HEADINGS"));
	//	_pUtil2D->Text_DrawFormatted(Clip, pFont,  PhoneNumber, StartX, StartY, Style, 0xffffffff, 0xffffffff, 0xff000000, 120, 100, false);

	CPixel32 PixColor(255, 255, 255, 255);
	CPixel32 PixColorBlack(0, 0, 0, 255);
	CPixel32 PixColorBlackOutline(0, 0, 0, 105);

	_pUtil2D->Text_DrawFormatted(Clip, pFont, PhoneNumber, StartX, StartY, 0, PixColorBlackOutline, PixColorBlackOutline, PixColorBlackOutline, Clip.GetWidth(), Clip.GetHeight(), true, 0, (FontSize / pFont->GetOriginalSize()));
	_pUtil2D->Text_DrawFormatted(Clip, pFont, PhoneNumber, StartX, StartY, 0, PixColor, PixColor, PixColorBlack, Clip.GetWidth(), Clip.GetHeight(), false, 0, (FontSize / pFont->GetOriginalSize()));

	// render skip button
	if(pFont)
	{
		CRC_Viewport* pVP = _pUtil2D->GetVBM()->Viewport_Get();
		CRct VRect = pVP->GetViewRect();
		fp32 Aspect = _pUtil2D->GetRC()->GetDC()->GetScreenAspect() * _pUtil2D->GetRC()->GetDC()->GetPixelAspect();
		CVec2Dfp32 old_scale = _pUtil2D->GetCoordinateScale();

		_pUtil2D->SetCoordinateScale(CVec2Dfp32(VRect.GetWidth() / 640.0f , VRect.GetHeight() / 480.0f));
		
		CRct Rect;
		fp32 ButtonScale = 0.6f;
		fp32 ButtonYAdjust = 2.0f * ButtonScale;
		Rect.p0.x = 640 - TruncToInt((64 + 64*ButtonScale) * ButtonScale);
#ifdef PLATFORM_CONSOLE
		Rect.p0.x -= TruncToInt(640 * 0.15f / 4.0f);
#endif
		Rect.p1.x = Rect.p0.x+ TruncToInt(64*ButtonScale);
		Rect.p0.y = TruncToInt(32 * Aspect);
#ifdef PLATFORM_CONSOLE
		Rect.p0.y += TruncToInt(480 * 0.15f / 4.0f);
#endif

		int PictureWidthBase =  20;
		_pUtil2D->DrawTexture(Clip, Rect.p0, "GUI_Button_B", 0xFFFFFFFF,  CVec2Dfp32(1.0f/ButtonScale,1.0f/(ButtonScale*Aspect)));

		CStr Text = "§Z16§LMENU_CUTSCENEQUIT";
		int Style = WSTYLE_TEXT_WORDWRAP | WSTYLE_TEXT_SHADOW;
		wchar wText[1024];
		Localize_Str(Text, wText, 1023);
		fp32 w = Min(pFont->GetWidth(16, wText), 120.0f);
		fp32 FontSize = 16;
		CPnt Pos = Rect.p0;
		Pos.y += TruncToInt(ButtonYAdjust*Aspect);
		Pos.x = TruncToInt(Rect.p0.x -6.0f);
		Pos.x -= TruncToInt(w);
		_pUtil2D->SetFontScale(1.0f, Aspect);

		int TextColorM = CPixel32(255,255,255, 255);
		int TextColorH = CPixel32(255,255,255, 255);;
		int TextColorD = 0xff000000;
		_pUtil2D->Text_DrawFormatted(Clip, pFont, Text, Pos.x, Pos.y, Style, TextColorM, TextColorH, TextColorD, 280, 50, false);

		_pUtil2D->SetCoordinateScale(old_scale);

	}
}


// Telephone pager object
class CRPG_Object_Pager : public CRPG_Object_Item
{
	MRTC_DECLARE;
protected:
	CStr m_TelephoneNumber;
public:

	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
	{
		CStr Key = _pKey->GetThisName();
		if(Key == "PAGERNUMBER")
			m_TelephoneNumber = _pKey->GetThisValue();
		//DialogueItem
		else
			return CRPG_Object_Item::OnEvalKey(_KeyHash, _pKey);

		return true;
	}

	virtual int OnPickup(int _iObject, CRPG_Object *_pRoot, bool _bNoSound, int _iSender, bool _bNoPickupIcon = false)
	{
		CRPG_Object_Item::OnPickup(_iObject, _pRoot, _bNoSound, _iSender, _bNoPickupIcon);
		if (!_bNoPickupIcon)
		{
			CWObject_Message PagerMsg(OBJMSG_CHAR_SETPAGERNUMBER);
			PagerMsg.m_pData = m_TelephoneNumber.GetStr();
			m_pWServer->Message_SendToObject(PagerMsg, _iObject);
		}
		// Return 0 so this gets removed
		return 0;
	}

	virtual CFStr GetItemName()
	{
		return m_TelephoneNumber;
	}
};

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Pager, CRPG_Object_Item);
