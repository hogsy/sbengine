/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			
					
	Author:			Olle Rosenquist
					
	Copyright:		Copyright O3 Games AB 2002
					
	Contents:		
					
	Comments:		RPG object pickups with extra functionality
					
	History:		
		021018:		Created File
\*____________________________________________________________________________________________*/

#include "PCH.h"
#include "WObj_ActionCutscenePickup.h"
#include "../WObj_Char.h"
#include "../WObj_RPG.h"
#include "../WObj_Player.h"
#include "../WObj_Weapons/WObj_Spells.h"
#include "../WRPG/WRPGSpell.h"
#include "../WRPG/WRPGChar.h"
#include "../../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_Game.h"

#include "../../../../Shared/MOS/Classes/GameWorld/WDataRes_Sound.h"
#include "../../../../Shared/MOS/Classes/GameWorld/Client/WClient_Core.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Initializes the pickup
						
	Comments:		
\*____________________________________________________________________*/
void CWObject_ActionCutscenePickup::OnCreate()
{
	CWObject_ActionCutscenePickupParent::OnCreate();
	CACSClientData* pCD = GetACSClientData(this);
	m_iItemRepickCountdown = 0;
	m_IntersectNotifyFlags = OBJECT_FLAGS_PLAYER;
//	m_AnimTime = MRTC_RAND() % 255;
	m_nPhysIdleTicks = 0; 
	pCD->SetCutsceneFlags(ACTIONCUTSCENE_OPERATION_DISABLEPHYSICS | 
		ACTIONCUTSCENE_OPERATION_USETHIRDPERSON | 
		ACTIONCUTSCENE_OPERATION_PLAYSUCCESSANIMATION |
		ACTIONCUTSCENE_OPERATION_TRIGGERSUCCESS |
		ACTIONCUTSCENE_FLAGS_PAUSEALLAI);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		?
						
	Parameters:		
		_pWData:		description
		_pWServer:		World server
			
	Comments:		
\*____________________________________________________________________*/
void CWObject_ActionCutscenePickup::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	CWObject_ActionCutscenePickupParent::OnIncludeClass(_pWData, _pWServer);
}
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Configures the actioncutscene from registry keys
						
	Parameters:		
		_pKey:		Registry key
			
	Returns:		Whether a match to the keys were made or not
						
	Comments:		
\*____________________________________________________________________*/
void CWObject_ActionCutscenePickup::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	
	switch (_KeyHash)
	{
	case MHASH3('RPGO','BJEC','T'): // "RPGOBJECT"
		{
			CRPG_Object::IncludeRPGClass(KeyValue, m_pWServer->GetMapData(), m_pWServer, true);
			SetItem(_pKey->GetThisValue());
			break;
		}
	default:
		{
			if(KeyName.CompareSubStr("MSG_ONPICKUP") == 0)
			{
				int iSlot = atoi(KeyName.Str()+12);
				m_lMsg_TriggerSuccess.SetLen(Max(iSlot+1, m_lMsg_TriggerSuccess.Len()));
				m_lMsg_TriggerSuccess[iSlot].Parse(KeyValue, m_pWServer);
			}
			else
				CWObject_ActionCutscenePickupParent::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Sets the dummy item by name
						
	Parameters:		
		_pName:			Name of dummy item
			
	Returns:		Whether the operation was ok or not
						
	Comments:		
\*____________________________________________________________________*/
bool CWObject_ActionCutscenePickup::SetItem(const char *_pName)
{
	m_ItemTemplate = _pName;
	spCRPG_Object spObj = CRPG_Object::CreateObject(_pName, m_pWServer);
	
	return SetItem( (CRPG_Object_Item *)(CRPG_Object *)spObj );
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Sets the dummy item object
						
	Parameters:		
		_pObj:			Dummy item
			
	Returns:		Whether the operation was ok or not
						
	Comments:		
\*____________________________________________________________________*/
bool CWObject_ActionCutscenePickup::SetItem(CRPG_Object_Item *_pObj)
{
	if (!_pObj)
		return false;
	m_ItemTemplate = (char *)_pObj->m_Name;
	m_spDummyObject = _pObj;
	m_Param = 0x8000 | m_spDummyObject->GetAmmo(NULL);
	if(m_spDummyObject)
	{
		iModel(0) = m_spDummyObject->m_Model.m_iModel[0];
		if (m_spDummyObject->m_bNoPlayerPickup || m_spDummyObject->GetAmmo(NULL) == 0)
			ClientFlags() |= CLIENTFLAGS_PICKUPNOFLASH;
		return true;
	}
	else
		return false;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Message handling routine
						
	Parameters:		
		_Msg:			The message to handle
			
	Returns:		Return value depends on the message
						
	Comments:		
\*____________________________________________________________________*/
aint CWObject_ActionCutscenePickup::OnMessage(const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJSYSMSG_NOTIFY_INTERSECTION:
		{
			if (m_pWServer->GetGameTick() < m_iItemRepickCountdown)
				return false;

			if(m_spDummyObject && !IsDestroyed())
			{
				// Check if the item can be merged, if so auto pick it up.......
				bool bCanPickupUniqueGun = ((m_spDummyObject->m_Flags2 & RPG_ITEM_FLAGS2_UNIQUE) &&	m_spDummyObject->CanPickup());
				if (!(m_spDummyObject->m_Flags & RPG_ITEM_FLAGS_NOMERGE) || bCanPickupUniqueGun)
				{
					// Check if the item can be merged, if so auto pick it up.......
					m_iItemRepickCountdown = m_pWServer->GetGameTick() + ITEM_REPICKCOUNTDOWN;
					// Send message to char, asking if item exists
					if (m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_CANPICKUPITEM,m_spDummyObject->m_iItemType),_Msg.m_Param0) ||
						bCanPickupUniqueGun)
					{
						CWObject_Message Msg(OBJMSG_RPG_AVAILABLEPICKUP, (m_spDummyObject->m_iItemType), aint((CRPG_Object_Item *)(CReferenceCount *)m_spDummyObject));
						Msg.m_Reason = 1; //Don't autoequip this item. 
	 					Msg.m_pData = (char *)m_ItemTemplate;
						Msg.m_DataSize = m_ItemTemplate.GetMax();
						Msg.m_iSender = m_iObject;


						if(m_pWServer->Message_SendToObject(Msg, _Msg.m_Param0))
						{
							for(int i = 0; i < m_lMsg_TriggerSuccess.Len(); i++)
								m_lMsg_TriggerSuccess[i].SendMessage(m_iObject, _Msg.m_Param0, m_pWServer);
							Destroy();
							return true;
						}
					}
				}
			}
			return false;
		}
	case OBJMSG_ACTIONCUTSCENE_DOTRIGGERMIDDLE:
		{
			if (m_pWServer->GetGameTick() < m_iItemRepickCountdown)
				return false;

			m_iItemRepickCountdown = m_pWServer->GetGameTick() + ITEM_REPICKCOUNTDOWN;
			CACSClientData* pCD = GetACSClientData(this);
			// Pickup item
			if(!(pCD->m_CutsceneFlags & ACTIONCUTSCENE_FLAGS_WAITSPAWN))
			{
				if(m_spDummyObject && !IsDestroyed())
				{
					CWObject_Message Msg(OBJMSG_RPG_AVAILABLEPICKUP, (m_spDummyObject->m_iItemType), aint((CRPG_Object_Item *)(CReferenceCount *)m_spDummyObject));
					Msg.m_pData = (char *)m_ItemTemplate;
					Msg.m_DataSize = m_ItemTemplate.GetMax();
					Msg.m_iSender = m_iObject;

					if(m_pWServer->Message_SendToObject(Msg, _Msg.m_iSender))
					{
						for(int i = 0; i < m_lMsg_TriggerSuccess.Len(); i++)
							m_lMsg_TriggerSuccess[i].SendMessage(m_iObject, _Msg.m_iSender, m_pWServer);
					}

					if (!(m_spDummyObject->m_Flags & RPG_ITEM_FLAGS_NOPICKUP))
						iModel(0) = 0;

					return 0;
				}
			}
			// Let parent handle this as well
			break;
		}
	case OBJMSG_RPG_AVAILABLEPICKUP:
		{
			CACSClientData* pCD = GetACSClientData(this);
			if(!(pCD->m_CutsceneFlags & ACTIONCUTSCENE_FLAGS_WAITSPAWN))
			{
				if(m_spDummyObject && !IsDestroyed() && !(m_spDummyObject->m_Flags & RPG_ITEM_FLAGS_NOPICKUP))
				{
					CWObject_Message Msg(OBJMSG_RPG_AVAILABLEPICKUP, (m_spDummyObject->m_iItemType), aint((CRPG_Object_Item *)(CReferenceCount *)m_spDummyObject));
					Msg.m_pData = (char *)m_ItemTemplate;
					Msg.m_DataSize = m_ItemTemplate.GetMax();
					Msg.m_iSender = m_iObject;
					
					if(m_pWServer->Message_SendToObject(Msg, _Msg.m_Param0))
					{
						for(int i = 0; i < m_lMsg_TriggerSuccess.Len(); i++)
							m_lMsg_TriggerSuccess[i].SendMessage(m_iObject, _Msg.m_Param0, m_pWServer);
						Destroy();
					}

					/*if (!(m_spDummyObject->m_Flags & RPG_ITEM_FLAGS_NOPICKUP))
						iModel(0) = 0;*/
		
					return 0;
				}
			}
			return false;
		}
	case OBJMSG_ACTIONCUTSCENE_GETPICKUPITEMTYPE:
		{
			if(m_spDummyObject && !IsDestroyed())
			{
				if (_Msg.m_pData && _Msg.m_DataSize >= sizeof(int32))
				{
					*(int32*)_Msg.m_pData = m_spDummyObject->m_AnimProperty;
				}
				return m_spDummyObject->m_iItemType;
			}
			break;
		}
	case OBJMSG_ACTIONCUTSCENE_ACTIVATE:
		{
			return DoActionSuccess(_Msg.m_iSender);
		}
	case OBJMSG_RPG_REPLACEPICKUP:
		{
			for(int i = 0; i < m_lMsg_TriggerSuccess.Len(); i++)
				m_lMsg_TriggerSuccess[i].SendMessage(m_iObject, _Msg.m_Param0, m_pWServer);

 			m_lMsg_TriggerSuccess.SetLen(_Msg.m_Param0);
			CWO_SimpleMessage *pMsg = (CWO_SimpleMessage *)_Msg.m_Param1;
			for(int i = 0; i < _Msg.m_Param0; i++)
				m_lMsg_TriggerSuccess[i] = pMsg[i];

			return SetItem((CRPG_Object_Item *)_Msg.m_pData);
		}
	case OBJMSG_RPG_SETTEMPPICKUPSURFACE:
			m_CreationGameTick = m_pWServer->GetGameTick();
			m_CreationGameTickFraction = 0;
			Data(0) = _Msg.m_Param0;
			Data(1) = m_pWServer->GetGameTick() + _Msg.m_Param1;
			return 1;

	case OBJMSG_RPG_ISPICKUP:
		{
			/*if(m_Flags & ACTIONCUTSCENE_FLAGS_WAITSPAWN)
				return false;
			else
				return true;*/
			return true;
		}
	case OBJMSG_ACTIONCUTSCENE_ISACTIONCUTSCENE:
		{
			return true;
		}
	case OBJMSG_ACTIONCUTSCENE_CANACTIVATE:
		{
			return true;
		}
	/*case OBJMSG_RPG_GETPICKUPANIM:
		{
			return m_iSuccessAnimation;
		}*/
	case OBJMSG_GAME_SPAWN:
		{
			if(!(m_Flags & ACTIONCUTSCENE_FLAGS_WAITSPAWN))
				return 0;
			m_Flags &= ~ACTIONCUTSCENE_FLAGS_WAITSPAWN;
			ClientFlags() &= ~CWO_CLIENTFLAGS_INVISIBLE;
			m_pWServer->Phys_InsertPosition(m_iObject, this);
			m_pWServer->Object_SetDirty(m_iObject, CWO_DIRTYMASK_COREMASK);

			return true;
		}
	case OBJMSG_ACTIONCUTSCENE_GETTYPE:
		{
			return ACTIONCUTSCENE_TYPE_PICKUP;
		}

	case OBJMSG_CHAR_GETUSENAME:
		{
			CFStr *pSt = (CFStr *)_Msg.m_pData;
			if(!pSt || !m_spDummyObject)
				return 0;

			*pSt = m_spDummyObject->GetItemName();
			return 1;
		}
	case OBJMSG_CHAR_GETDESCNAME:
		{
			CFStr *pSt = (CFStr *)_Msg.m_pData;
			if(!pSt || !m_spDummyObject)
				return 0;

			*pSt = m_spDummyObject->GetItemDesc();
			return 1;
		}
	case OBJMSG_CHAR_SETUSENAME:
		{
			if(!_Msg.m_pData || !m_spDummyObject)
				return 0;

			m_spDummyObject->m_Name = (const char*)_Msg.m_pData;
			return 1;
		}
	case OBJMSG_CHAR_SETDESCNAME:
		{
			if(!_Msg.m_pData || !m_spDummyObject)
				return 0;

			m_spDummyObject->m_Name = (const char*)_Msg.m_pData;
			return 1;
		}
	case OBJMSG_ACTIONCUTSCENEPICKUP_SETDEFAULTS:
		{
			// Set some default stuffs
			CACSClientData* pCD = GetACSClientData(this);
			m_ActionCutsceneTypeSuccess = ACTIONCUTSCENE_TYPE_PICKUPITEMGROUND;
			pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_OPERATION_DISABLEPHYSICS | 
				ACTIONCUTSCENE_OPERATION_USETHIRDPERSON | 
				ACTIONCUTSCENE_OPERATION_PLAYSUCCESSANIMATION);
			return 1;
		}
	default:
		break;
	};

	return CWObject_ActionCutscenePickupParent::OnMessage(_Msg);
}

bool CWObject_ActionCutscenePickup::CanActivate(int32 _iChar)
{
	// Check if dummyitem is assault, if so then we can't activate now can we...
	/*if (m_spDummyObject && (m_spDummyObject->m_AnimProperty == AG_EQUIPPEDITEMCLASS_ASSAULT))
	{
		return (m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_USEDNAWEAPONS,2),_iChar) == 1);
	}*/

	return true;
}

void CWObject_ActionCutscenePickup::TagAnimationsForPrecache(CWAG2I_Context* _pContext, CWAG2I* _pAGI, TArray<int32>& _liACS)
{
	if (m_spDummyObject)
		m_spDummyObject->TagAnimationsForPrecache(_pContext, _pAGI);
	CWObject_ActionCutscene::TagAnimationsForPrecache(_pContext,_pAGI, _liACS);
}

int32 CWObject_ActionCutscenePickup::DetermineACSType(const CVec3Dfp32& _Charpos, bool _bCrouch)
{
	// Just temporary type for now
	// Check height diff from char to acs
	// Should do bb check to see that nothing is in the way...
	fp32 Height = GetPosition().k[2] - _Charpos.k[2];
	if (Height > 20.0f && Height < 50.0f)
	{
		if (_bCrouch)
			return ACTIONCUTSCENE_TYPE_CROUCH_PICKUPITEMTABLE;
		else
			return ACTIONCUTSCENE_TYPE_PICKUPITEMTABLE;
	}
	else if (Height > -2.0f)
	{
		if (_bCrouch)
			return ACTIONCUTSCENE_TYPE_CROUCH_PICKUPITEMGROUND;
		else
			return ACTIONCUTSCENE_TYPE_PICKUPITEMGROUND;
	}

	return 0;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		In this instance DoActionSuccess puts together an
					animation with the actions neccessary for picking
					up this object
						
	Parameters:		
		_iCharacter:	Which character is picking up the object
			
	Returns:		Whether adding the animation was successful or not
						
	Comments:		
\*____________________________________________________________________*/
bool CWObject_ActionCutscenePickup::DoActionSuccess(int _iCharacter)
{
	// If the animation isn't to be played, return.. (maybe should change this)
	// First test if we can merge item, if we can, just pick it up directly
	// Don't know if weapon merge handles max ammo, if not check that as well!
	/*if (!(m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_PLAYSUCCESSANIMATION) || 
		(m_ActionCutsceneTypeSuccess == ACTIONCUTSCENE_TYPE_UNDEFINED))*/
	CACSClientData* pCDACS = GetACSClientData(this);
	CWObject* pObj = m_pWServer->Object_Get(_iCharacter);
	CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
	if (!m_spDummyObject || !pCD)
		return false;

	// Must detemine what kind of pickup animation to use (crouch/stand, low high)
	m_ActionCutsceneTypeSuccess = DetermineACSType(pObj->GetPosition(),(CWObject_Character::Char_GetPhysType(pObj) == PLAYER_PHYS_CROUCH));

	// Check if the item should picked up
	/*if (!m_spDummyObject->CanUse(_iCharacter) || m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_CANPICKUPITEM,m_spDummyObject->m_iItemType),_iCharacter) ||
		!(m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_PLAYSUCCESSANIMATION) || (m_ActionCutsceneTypeSuccess == 0))*/
	if (m_spDummyObject)
	{
		if (m_spDummyObject->m_Flags & RPG_ITEM_FLAGS_NOPICKUP)
			return false;
		CWObject_Message Msg(OBJMSG_RPG_AVAILABLEPICKUP, (m_spDummyObject->m_iItemType), aint((CRPG_Object_Item *)(CReferenceCount *)m_spDummyObject));
		Msg.m_pData = (char *)m_ItemTemplate;
		Msg.m_DataSize = m_ItemTemplate.GetMax();
		Msg.m_iSender = m_iObject;

		if(m_pWServer->Message_SendToObject(Msg, _iCharacter))
		{
			for(int i = 0; i < m_lMsg_TriggerSuccess.Len(); i++)
				m_lMsg_TriggerSuccess[i].SendMessage(m_iObject, _iCharacter, m_pWServer);
			//m_Msg_OnPickup.SendMessage(m_iObject, _Msg.m_Param0, m_pWServer);
			Destroy();
			return true;
		}
		return false;
	}

	// Don't want to autopickup several version of it
	m_IntersectNotifyFlags = 0;

	// Enter actioncutscene controlmode
	CWObject_Character::Char_SetControlMode(pObj, PLAYER_CONTROLMODE_ACTIONCUTSCENE);
	((CWObject_Character *)pObj)->UpdateVisibilityFlag();

	// Set action cutscene id to controlmode param0
	pCD->m_ControlMode_Param0 = (fp32)m_iObject;

	// Set cutscenetype to param2
	pCD->m_ControlMode_Param2 = (fp32)m_ActionCutsceneTypeSuccess;

	if (pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_FLAGS_HASDEFINEDDIRECTION)
	{
		// Set defined direction to param1
		pCD->m_ControlMode_Param1 = m_DefinedDirection;
	}

	// Ok determine what type of cutscene we're having and deal with positioning accordingly
	// Only the hatch animations should be able to move around (and thus use the old positioning)
	// Filter out success actions and set to param4
	pCD->m_ControlMode_Param4 = ((pCDACS->m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_SUCCESSMASK) & 
		~(ACTIONCUTSCENE_OPERATION_SETSTARTPOSMASK|ACTIONCUTSCENE_OPERATION_SETENDPOSMASK)) | 
		ACTIONCUTSCENE_FLAGS_ISSUCCESS | ACTIONCUTSCENE_FLAGS_USEFLOATINGSTART;

	// Make sure the camera will be updated when the ACS is active
	ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

	// Start activation timer
	if (m_MinActivationTime > 0.0f)
		m_MinActivationTimer = m_pWServer->GetGameTick() + 
		(int)(m_MinActivationTime * m_pWServer->GetGameTicksPerSecond());
	else
		m_MinActivationTimer = 0;

	//	*attachrottrack 23 // Right hand
	//	*attachpoint 7 // Right hand

	// Set item model on character, and kill local model
	if (m_spDummyObject)
	{
		// Set extra item
		// OBJMSG_CHAR_SETEXTRAITEM <- creates object and casts it away, seems unecessary
		// must set attachpoint and stuff anyways...
		pCD->m_Item2_Model.UpdateModel(m_spDummyObject->m_Model);
		pCD->m_Item2_Model.MakeDirty();
		pCD->m_Item2_Model.m_iAttachPoint[0] = 7;
		pCD->m_Item2_Model.m_iAttachRotTrack = 23;

		// Clear our own model (in the actioncutscene)
		iModel(0) = 0;
	}

	return true;

	// OLD STUFF!!!
	// Hmm, ok we need the same functionality on the new animation system thingy
	/*CWObject* pObj = m_pWServer->Object_Get(_iCharacter);
	CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);
	if (!pCD)
		return false;
	
	// Enter actioncutscene controlmode
	CWObject_Character::Char_SetControlMode(pObj, PLAYER_CONTROLMODE_ACTIONCUTSCENE);

	// Set action cutscene id to controlmode param0
	pCD->m_ControlMode_Param0 = (fp32)m_iObject;
	// Filter out success actions and set to param4
	pCD->m_ControlMode_Param4 = (m_CutsceneFlags & ACTIONCUTSCENE_OPERATION_SUCCESSMASK);
	// Set cutscenetype to param2
	pCD->m_ControlMode_Param2 = (fp32)m_ActionCutsceneTypeSuccess;

	// Make sure the ledge camera will be updated when the ACS is active
	ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

	return true;*/
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Renders the pickup model
						
	Parameters:		
		_pObj:			Pickup object
		_pWClient:		World client
		_pEngine:		Renderer
			
	Comments:		
\*____________________________________________________________________*/
void CWObject_ActionCutscenePickup::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	if(_pObj->m_ClientFlags & CLIENTFLAGS_ATTACHED)
	{
/*		int iParent = _pWClient->Object_GetParent(_pObj->m_iObject);
		CWObject_Client *pChar = _pWClient->Object_Get(iParent);
		if(pChar)
		{
			// if pChar, och sedan vadå?  /mh   :)
		}*/
	}
	else
	{
		CMat4Dfp32 MatIP;
		fp32 IPTime = _pWClient->GetRenderTickFrac();

		Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);

		int iModel = _pObj->m_iModel[0];
		CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(iModel);
		if (pModel)
		{
			uint32 RenderFlags = 0;
			CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
			//AnimState.m_AnimTime1 = AnimState.m_AnimTime0;
			//AnimState.m_AnimTime0.Reset();
			if (!(_pObj->m_ClientFlags & CLIENTFLAGS_PICKUPNOFLASH))
			{
				CXW_Surface* pOrgSurf = _pEngine->m_pSC->GetSurface("pickupglow");
				if(pOrgSurf)
				{
					CXW_Surface* pSurf = pOrgSurf->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
					if (pSurf)
					{
						AnimState.m_lpSurfaces[0] = pSurf;
						RenderFlags |= CXR_MODEL_ONRENDERFLAGS_SURF0_ADD;
					}
				}
			}
			if(_pObj->m_Data[1] != 0 && _pObj->m_Data[1] >= _pWClient->GetGameTick())
			{
				CXW_Surface* pSurf = _pWClient->GetMapData()->GetResource_Surface(_pObj->m_Data[0]);
				if (pSurf)
				{
					AnimState.m_lpSurfaces[0] = pSurf;
					RenderFlags |= CXR_MODEL_ONRENDERFLAGS_SURF0_ADD;
				}
			}

			_pEngine->Render_AddModel(pModel, MatIP, AnimState, XR_MODEL_STANDARD, RenderFlags);
		}
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Updates the pickup
						
	Comments:		
\*____________________________________________________________________*/
void CWObject_ActionCutscenePickup::OnRefresh()
{
	CWObject_ActionCutscenePickupParent::OnRefresh();
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		?
						
	Comments:		
\*____________________________________________________________________*/
void CWObject_ActionCutscenePickup::OnSpawnWorld()
{
	// Disable physics on default
	CACSClientData* pCD = GetACSClientData(this);
	pCD->SetCutsceneFlags(pCD->m_CutsceneFlags | ACTIONCUTSCENE_OPERATION_DISABLEPHYSICS);
	CWObject_ActionCutscenePickupParent::OnSpawnWorld();

	CWO_PhysicsState PhysState(GetPhysState());
	PhysState.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(8.0f, 8.0f, 8.0f), 0);
	PhysState.m_nPrim = 1;
	m_IntersectNotifyFlags = OBJECT_FLAGS_PLAYER;
	//m_iItemRepickCountdown = 1.0f * SERVER_TICKSPERSECOND;

	if (!m_pWServer->Object_SetPhysics(m_iObject, PhysState))
	{
		m_pWServer->Object_Destroy(m_iObject);
		return;
	}
}

bool CWObject_ActionCutscenePickup::OnEndACS(int _iCharacter)
{
	CWObject_ActionCutscene::OnEndACS(_iCharacter);

	CWObject_CoreData* pObj = m_pWServer->Object_GetCD(_iCharacter);
	CWO_Character_ClientData* pCD = (pObj ? CWObject_Character::GetClientData(pObj) : NULL);

	if (pCD)
	{
		// Reset the extra item
		pCD->m_Item2_Model.Clear();
		pCD->m_Item2_Model.MakeDirty();
	}

	// Kill ourselves
	if(m_spDummyObject && !(m_spDummyObject->m_Flags & RPG_ITEM_FLAGS_NOPICKUP))
		Destroy();

	return true;
}

void CWObject_ActionCutscenePickup::OnDeltaSave(CCFile* _pFile)
{
	CWObject_ActionCutscenePickupParent::OnDeltaSave(_pFile);
	// Save the dummyitem
	uint8 Save = 0;
	if (m_spDummyObject)
	{
		Save = 1;
		_pFile->WriteLE(Save);
		// Mmmkay then, guess we first have to write template name and then the delta save
		// for the item?
		m_ItemTemplate.Write(_pFile);
		//m_spDummyObject->OnDeltaSave(_pFile);
	}
	else
	{
		_pFile->WriteLE(Save);
	}

	//A pickup created after world spawn must save success messages if any,
	//since these can be set for a dropitem
 	if (!m_bOriginallySpawned)
	{
		uint8 nMsgsSuccess = m_lMsg_TriggerSuccess.Len();
		_pFile->WriteLE(nMsgsSuccess);
		for (int i = 0; i < nMsgsSuccess; i++)
		{
			m_lMsg_TriggerSuccess[i].OnDeltaSave(_pFile);
		}
	}
}

void CWObject_ActionCutscenePickup::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	CWObject_ActionCutscenePickupParent::OnDeltaLoad(_pFile, _Flags);
	uint8 Load = 0;
	_pFile->ReadLE(Load);
	if (Load)
	{
		CFStr Temp;
		Temp.Read(_pFile);
		SetItem(Temp.GetStr());
		/*if (m_spDummyObject)
		m_spDummyObject->OnDeltaLoad(_pFile);*/
	}

	//A pickup created after world spawn must load success messages if any,
	//since these can be set for a dropitem
	if (!m_bOriginallySpawned)
	{
		uint8 nMsgsSuccess = 0;
		_pFile->ReadLE(nMsgsSuccess);
		m_lMsg_TriggerSuccess.Clear();
		m_lMsg_TriggerSuccess.SetLen(nMsgsSuccess);
		for (int i = 0; i < nMsgsSuccess; i++)
		{
			m_lMsg_TriggerSuccess[i].OnDeltaLoad(_pFile);
		}
	}
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_ActionCutscenePickup, CWObject_ActionCutscene, 0x0100);



#include "../CConstraintSystem.h"
#include "../CConstraintSystemClient.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:		Updates the dropped item, using ragdoll technique

Comments:		
\*____________________________________________________________________*/
void CWObject_DroppableItem::OnRefresh()
{
	CWObject_DroppableItemParent::OnRefresh();
	if(m_spRagdoll)
	{
		CMat4Dfp32 Mat = GetPositionMatrix();
		m_spRagdoll->Animate(m_pWServer->GetGameTick(), Mat);
		CMat4Dfp32 Mat2;
		m_spRagdoll->GetPosition(Mat2);
		if(m_spRagdoll->IsStopped())
			m_spRagdoll = NULL;

		// Check if item has fallen outside the world
		CWObject* pWorldSpawn = m_pWServer->Object_Get( m_pWServer->Object_GetWorldspawnIndex() );
		if (pWorldSpawn)
		{
			CBox3Dfp32 WorldBound = *pWorldSpawn->GetAbsBoundBox();
			WorldBound.Grow(8.0f);
			if (!GetAbsBoundBox()->IsInside(WorldBound))
			{
				ConOutL("§cf80WARNING: Removing dropitem because it is outside the world..");
				m_pWServer->Object_Destroy(m_iObject);
				return;
			}
		}

		m_pWServer->Object_SetPosition(m_iObject, Mat2);
		// Remove "pickupable" status if out of ammo
		if (m_spDummyObject && (m_spDummyObject->m_Flags2 & RPG_ITEM_FLAGS2_THROWAWAYONEMPTY) && 
			!m_spDummyObject->GetAmmo(NULL) && (GetPhysState().m_ObjectFlags != 0))
		{
			m_pWServer->Object_SetPhysics_ObjectFlags(m_iObject, 0);
		}
	}

	// If our weapon is empty and a player isn't looking, remove ourselves
	if (m_spDummyObject && m_spDummyObject->GetAmmoDraw() && !m_spDummyObject->GetAmmo(NULL))
	{
		CVec3Dfp32 OurPos = GetPosition();
		// Ok then, check if players are looking
		CWObject_Game *pGame = m_pWServer->Game_GetObject();
		for (int i = 0; i < pGame->Player_GetNum(); i++)
		{
			CWObject* pPlayer = pGame->Player_GetObject(i);
			if(pPlayer)
			{
				CVec3Dfp32 Dir = OurPos - pPlayer->GetPosition();
				// if player isn't looking away, return
				if (Dir * pPlayer->GetLocalPositionMatrix().GetRow(0) > 0.0f)
					return;
			}
		}

		// Remove ourselves
		m_pWServer->Object_Destroy(m_iObject);
	}
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:		Initializes the ragdoll for the dropped item.

Comments:		
\*____________________________________________________________________*/
void CWObject_DroppableItem::OnFinishEvalKeys()
{
	CWObject_DroppableItemParent::OnFinishEvalKeys();
	ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;

}
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:		Initializes the ragdoll for the dropped item.

Comments:		
\*____________________________________________________________________*/
void CWObject_DroppableItem::OnCreate()
{
	CWObject_DroppableItemParent::OnCreate();
	m_spRagdoll = NULL;
}


aint CWObject_DroppableItem::OnMessage(const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{	
		case OBJMSG_RPG_SETRIGIDBODY:
		{
			m_spRagdoll = (CConstraintRigidObject *) _Msg.m_Param0;
			if (m_spRagdoll)
				m_spRagdoll->Init(m_iObject, this, m_pWServer);
			return 1;
		}
	}

	return CWObject_DroppableItemParent::OnMessage(_Msg);
}

void CWObject_DroppableItem::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_DroppableItem, CWObject_DroppableItemParent, 0x0100);




