#include "PCH.h"

#include "WObj_Char.h"

#include "../GameWorld/WClientMod_Defines.h"
#include "../../../Shared/MOS/Classes/GameWorld/WDataRes_Sound.h"
#include "../../Shared/MOS/XR/XRBlockNav.h"

#include "../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_Game.h"
#include "WRPG/WRPGFist.h"
#include "WObj_Game/WObj_GameCore.h"

#include "WRPG/WRPGChar.h"
#include "WRPG/WRPGWeapon.h"
#include "WRPG/WRPGInitParams.h"

#include "WObj_AI/AICore.h"
#include "WObj_AI/AI_ResourceHandler.h"
#include "WObj_AI/WObj_Aux/WObj_Team.h"

#include "WObj_Misc/WObj_Ledge.h"

#include "CConstraintSystem.h"
#include "WObj_Misc/WObj_CreepingDark.h"
#include "WObj_Misc/WObj_TentacleSystem.h"
#include "WObj_Misc/WObj_EffectSystem.h"
#include "WObj_Misc/WObj_BlackHole.h"
#include "WObj_Misc/WObj_DarklingSpawn.h"
#include "WObj_Misc/WObj_Object.h"
#include "WObj_Misc/WObj_SwingDoor.h"
#include "WObj_Char/WObj_CharShapeshifter.h"

#include "../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_PhysCluster.h"

#ifdef PLATFORM_DOLPHIN
#include "../../../Shared/MOS/MRndrDolphin/DisplayContext.h"
#endif

//-------------------------------------------------------------------
enum
{
	class_CharPlayer =			MHASH6('CWOb','ject','_','Char','Play','er'),
	class_CharNPC =				MHASH5('CWOb','ject','_','Char','NPC'),
	class_CharNPC_P6 =			MHASH6('CWOb','ject','_','Char','NPC','_P6'),
	class_Object_Lamp =			MHASH6('CWOb','ject','_','Obje','ct_','Lamp'),
	class_LampSwitch =			MHASH6('CWOb','ject','_','Lamp','Swit','ch'),
	class_Television =			MHASH6('CWOb','ject','_','Tele','visi','on'),
	class_UseProxy =			MHASH5('CWOb','ject','_','UseP','roxy'),
	class_DarklingSpawn =		MHASH7('CWOb','ject','_','Dark','ling','Spaw','n'),
	class_Telephoneregistry =	MHASH8('CWOb','ject','_','Tele','phon','ereg','istr','y'),
};

//-------------------------------------------------------------------
static int16 GetObjFlags(CWObject_CoreData* _pObj)
{
	MAUTOSTRIP(GetObjFlags, 0)
	if (_pObj == NULL)
		return 0;

	return _pObj->GetPhysState().m_ObjectFlags;
}
/*
static int16 GetObjFlags(int _iObj, CWorld_PhysState* _pWPhysState)
{
	MAUTOSTRIP(GetObjFlags_2, 0);
	if (_pWPhysState == NULL)
		return 0;

	return GetObjFlags(_pWPhysState->Object_GetCD(_iObj));
}
*/
//-------------------------------------------------------------------


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Character gameplay mechanics, a.k.a "sophögen"
\*____________________________________________________________________________________________*/

int CWObject_Character::Char_CheatsEnabled()
{
	MAUTOSTRIP(CWObject_Character_Char_CheatsEnabled, 0);
	return 1;
//	return m_pWServer->Registry_GetServer()->GetValuei("CHEATS", 1);
}

int32 CWObject_Character::Team_Get(int32 _i)
{
	MAUTOSTRIP(CWObject_Character_Team_Get, 0);
	if (m_liTeams.ValidPos(_i))
		return m_liTeams[_i];
	else
		return 0;
};

int32 CWObject_Character::Team_GetNum()
{
	return m_liTeams.Len();
}

// Adds team _iID unless already added
// Returns list index to added team
int32 CWObject_Character::Team_Add(int32 _iID)
{
	MAUTOSTRIP(CWObject_Character_Team_Add, 0);

	//Check teams
	int NTeam = Team_GetNum();
	for (int i = 0; i < NTeam; i++)
	{	
		if (_iID == Team_Get(i))
		{	//Jupp, we belong to team
			return i;
		}
	}	
	m_liTeams.Add(_iID);
	return NTeam;
};

bool CWObject_Character::Team_Remove(int32 _iID)
{
	MAUTOSTRIP(CWObject_Character_Team_Remove, 0);

	//Check teams
	int NTeam = Team_GetNum();
	// We loop backwards in the (unlikely) event that we may want to remove
	// multiple instances of the same team.
	for (int i = NTeam-1; i >= 0; i--)
	{
		if (_iID == Team_Get(i))
		{	//Jupp, we belong to team
			m_liTeams.Del(i);
			return true;
		}
	}

	return false;
};

int CWObject_Character::Char_AutoActivate()
{
	return m_pWServer->Registry_GetServer()->GetValuei("AUTOACTIVATE", 1);
}

// true if item was removed.
bool CWObject_Character::Char_RemoveItem(const char *_pName)
{
	MAUTOSTRIP(CWObject_Character_Char_RemoveItem, false);
	for(int i = 0; i < RPG_CHAR_NUMCHILDREN; i++)
	{
		int nItems = Char()->GetNumItems(i);
		for(int j = 0; j < nItems; j++)
		{
			CRPG_Object_Item *pItem = Char()->GetInventory(i)->GetItemByIndex(j);
			if ((pItem != NULL) && (pItem->m_Name.CompareNoCase(_pName) == 0))
			{
				if (!Char()->GetInventory(i)->RemoveItemByName(m_iObject, _pName))
					if (!Char()->CycleSelectedItem(i, true))
						if (i == 0) // Only set default stance for weapons, not for when you sell items/ammo or such.
							Char_SetAnimStance(this, 0); // Set some default unarmed stance.
				return true;
			}

		}
	}

	return false;
}

/*void CWObject_Character::CreateTeleportEffect()
{
	MAUTOSTRIP(CWObject_Character_CreateTeleportEffect, MAUTOSTRIP_VOID);
	if (m_Teleport_Effect != "")
	{
		CMat4Dfp32 EffectMatrix = GetPositionMatrix();
		CVec3Dfp32(0, 0, 1).SetMatrixRow(EffectMatrix, 2);
		EffectMatrix.RecreateMatrix(0, 2);
		GetCharacterCenter(this, m_pWServer, false, 0.25f).SetMatrixRow(EffectMatrix, 3);
		int iTeleportEffectObj = m_pWServer->Object_Create(m_Teleport_Effect, EffectMatrix, m_iObject);
	}

	if (m_Teleport_iSound > 0)
		Sound(m_Teleport_iSound);
}*/

bool CWObject_Character::Char_SelectItem(CRPG_Object_Item* _pItem)
{
	MAUTOSTRIP(CWObject_Character_Char_SelectItem, false);
	if (!_pItem)
		return false;

	if(_pItem->m_iItemType & 0x100)
	{
		CRPG_Object_Inventory *pInv = Char()->GetInventory(0);
		for(int i = 0; i < pInv->GetNumItems(); i++)
		{
			CRPG_Object_Item *pWeapon = pInv->GetItemByIndex(i);
			for(int j = 0; j < pWeapon->m_NumLinks; j++)
				if(pWeapon->m_lLinkedItemTypes[j] == _pItem->m_iItemType)
					return SelectItem(pWeapon->m_iItemType, true);				
		}
		return false;
	}
	else
	{
		if (!SelectItemByIdentifier(_pItem->m_iItemType >> 8,_pItem->m_Identifier, true))
			return SelectItem(_pItem->m_iItemType);

		return true;
	}
}

bool CWObject_Character::Char_SelectItemByName(const char *_pName)
{
	return Char_SelectItem(Char()->FindItemByName(_pName));
}

bool CWObject_Character::Char_SelectItemByType(int _ItemType)
{
	return Char_SelectItem(Char()->FindItemByType(_ItemType));
}

bool CWObject_Character::Char_SelectItemByAnimType(int _AnimType, int _iSlot)
{
	return Char_SelectItem(Char()->FindItemByAnimType(_AnimType,_iSlot));
}

bool CWObject_Character::Char_SelectItemByIndex(int _iSlot, int _iIndex)
{
	CRPG_Object_Inventory *pInv = Char()->GetInventory(_iSlot);
	CRPG_Object_Item *pItem = (pInv) ? pInv->GetItemByIndex(_iIndex) : NULL;
	return Char_SelectItem(pItem);
};

void CWObject_Character::Char_RemoveEmptyItems(int _iSlot)
{
	// Ok then, remove empty items from slot
	CRPG_Object_Inventory *pInv = Char()->GetInventory(_iSlot);
	if (pInv)
		pInv->RemoveEmptyItems();

	m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_FORCEDROPWEAPON),m_iObject);

}

extern bool g_bSkippable;

bool CWObject_Character::Char_BeginTimeLeap(bool _bDestroySounds)
{
	m_PendingTimeLeapTick = -1;
	g_bSkippable = false;
	if(!(m_Flags & PLAYER_FLAGS_TIMELEAP))
	{
		if(_bDestroySounds)
			ConExecute("timeleap(1)");
		else
			ConExecute("timeleap(2)");
		m_Flags |= PLAYER_FLAGS_TIMELEAP;
		//M_TRACEALWAYS(CStrF("%i. BeginTimeLeap\n", m_pWServer->GetGameTick()));
		return true;
	}
	else
		return false;
}

bool CWObject_Character::Char_EndTimeLeap()
{
	g_bSkippable = false;
	ConExecute("timeleap(0)");
	if(m_Flags & PLAYER_FLAGS_TIMELEAP)
	{
		//M_TRACEALWAYS(CStrF("%i. EndTimeLeap\n", m_pWServer->GetGameTick()));
		m_Flags &= ~PLAYER_FLAGS_TIMELEAP;
		CWO_Character_ClientData* pCD = GetClientData(this);
		// Set diff
		Char_SetGameTickDiff(0);
		CWAG2I_Context AGContext(this,m_pWServer,CMTime::CreateFromTicks(pCD->m_GameTick,m_pWServer->GetGameTickTime()));
		pCD->m_AnimGraph2.GetAG2I()->EndTimeLeap(&AGContext);
		return true;
	}
	else
		return false;
}

void CWObject_Character::Char_RemoveDropWeapon(int32 _CurrentIdentifier)
{
	// Check for other weapons that might have to be dropped
	CRPG_Object_Inventory* pInventory = Char()->GetInventory(RPG_CHAR_INVENTORY_WEAPONS);
	for (int i = 0; i < pInventory->GetNumItems(); i++)
	{
		CRPG_Object_Item *pItem = pInventory->GetItemByIndex(i);
		if ((pItem != NULL) && (pItem->m_Flags & RPG_ITEM_FLAGS_DROPONUNEQUIP) && 
			!(pItem->m_Flags2 & RPG_ITEM_FLAGS2_PERMANENT) && 
			(pItem->m_Identifier != _CurrentIdentifier) && (pItem->m_Identifier != m_LastUsedItem))
		{
			m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_FORCEDROPWEAPON,
				pItem->m_Identifier),m_iObject);
			i--;
		}
	}
}

bool CWObject_Character::Char_DropItem(int _iSlot)
{
	MAUTOSTRIP(CWObject_Character_Char_DropItem, false);
/*	CWO_Character_ClientData *pCD = GetClientData(this);
	int ItemIndex = Char()->GetCurItemIndex(_iSlot);
	CRPG_Object_Item *pItem = Char()->GetCurItem(_iSlot);
	CXR_Model *pChar = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
	if(pCD && m_pItem[_iSlot] && pItem && pChar)
	{
		// Drop items
		CXR_AnimState Anim;
		if(OnGetAnimState(this, m_pWServer, GetPositionMatrix(), 0, Anim))
		{
			CXR_Skeleton *pSkel = (CXR_Skeleton*)pChar->GetParam(MODEL_PARAM_SKELETON);
			if(pItem->m_Model.IsValid() && pSkel)
			{
				CXR_Model *pModel = m_pWServer->GetMapData()->GetResource_Model(pItem->m_Model.m_iModel[0]);
				if(pModel)
				{
					CMat4Dfp32 Mat;
					if(CRPG_Object_Item::GetItemRenderMatrix(Anim.m_pSkeletonInst, pSkel, pModel, pItem->m_Model.m_iAttachRotTrack, pItem->m_Model.m_iAttachPoint[0], pItem->m_Model.m_AttachAngles, Mat))
					{
						m_pItem[_iSlot]->DetachCharacter(this, pItem, Mat);
						SelectItem(_iSlot, -1, true);
						Char()->GetInventory(_iSlot)->DelChild(ItemIndex);

						int iObj = m_pWServer->Object_Create("Item", GetPositionMatrix(), m_iObject);
						m_pItem[_iSlot] = (CWObject_Item *)m_pWServer->Object_Get(iObj);
						if(!m_pItem[_iSlot])
							Error("CWObject_Character::Char_DropItem", "Could not create CWObject_Item");
						m_pItem[_iSlot]->AttachCharacter(this, _iSlot);
						return true;
					}
				}
			}
		}
	}*/

	return false;
}

void CWObject_Character::Char_DeathDropItem()
{
	CRPG_Object_Item *pItem = Char()->GetEquippedItem(0);
	if(!pItem || pItem->m_Flags & RPG_ITEM_FLAGS_NOPICKUP)
		return;

	TPtr<CRPG_Object_Item> spItem = pItem->GetForcedPickup();
	if (spItem)
		pItem = spItem;

	CXR_AnimState Anim;
	CXR_Skeleton* pSkel;
	CXR_SkeletonInstance* pSkelInstance;					
	if(!GetEvaluatedPhysSkeleton(pSkelInstance, pSkel, Anim))
		return;

	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if(!pCD)
		return;

	// *** Please remove me after PB ships ***
	// We detect that the item to drop is a knuckleduster and hijack track 44
	/*bool bKnuckleduster = false;
	if ((pCD->m_Item0_Model.m_iAttachRotTrack == 44) && (pCD->m_Item0_Model.m_iAttachPoint[0] == 0))
	{
		pCD->m_Item0_Model.m_iAttachRotTrack = 22;
		pCD->m_Item0_Model.m_iAttachPoint[0] = 6;
		bKnuckleduster = true;
	}
	// *** Please remove me after PB ships ****/

	CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
	CMat4Dfp32 Mat;
	CMTime GameTime = CMTime::CreateFromTicks(pCD->m_GameTick, m_pWServer->GetGameTickTime(), pCD->m_PredictFrameFrac);
	/*if(pCD->m_Item0_Model.GetModel0_RenderInfo(m_pWServer->GetMapData(), NULL, pSkelInstance, pSkel, GameTime, 
		Anim, Mat, pModel, m_pWServer))*/
	if (m_spRagdollDropItems[0])
	{
		m_spRagdollDropItems[0]->GetPosition(Mat);
		int iObj = m_pWServer->Object_Create("Item", Mat);
		if(iObj > 0)
		{
			m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENEPICKUP_SETDEFAULTS), iObj);
			int nMessages;
			const CWO_SimpleMessage* pMsg = m_MsgContainer.GetMessages(PLAYER_SIMPLEMESSAGE_ONPICKUPDROPWEAPON, nMessages);
			CWObject_Message Msg(OBJMSG_RPG_REPLACEPICKUP, nMessages, aint(pMsg));
			Msg.m_pData = (CRPG_Object *)pItem;
			m_pWServer->Message_SendToObject(Msg, iObj);

			/*CConstraintRigidObject* pDropItem = m_spRagdollDropItems[0];
			m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_RPG_SETRIGIDBODY, (aint)pDropItem), iObj); 
			if (pDropItem)
				pDropItem->SetSingleCollisionSound(pItem->m_iSound_Dropped);*/
			Char()->RemoveItemByIdentifier(m_iObject,pItem->m_Identifier,0,true);
		}
	}
	CRPG_Object_Item *pItem1 = Char()->GetEquippedItem(1);
	if (!pItem1)
		pItem1 = (CRPG_Object_Item *)(CRPG_Object*)m_spDropItem;
	if(pItem1 && !(pItem1->m_Flags & RPG_ITEM_FLAGS_NOPICKUP))
	{
		TPtr<CRPG_Object_Item> spItem1 = pItem1->GetForcedPickup();
		if (spItem1)
			pItem1 = spItem1;
		// secondary item
		/*if(pCD->m_Item1_Model.GetModel0_RenderInfo(m_pWServer->GetMapData(), NULL, pSkelInstance, pSkel, GameTime, 
			Anim, Mat, pModel, m_pWServer))*/
		if (m_spRagdollDropItems[1])
		{
			m_spRagdollDropItems[1]->GetPosition(Mat);
			int iObj = m_pWServer->Object_Create("Item", Mat);
			if(iObj > 0)
			{
				m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENEPICKUP_SETDEFAULTS), iObj);
				int nMessages;
				const CWO_SimpleMessage* pMsg = m_MsgContainer.GetMessages(PLAYER_SIMPLEMESSAGE_ONPICKUPDROPITEM, nMessages);
				CWObject_Message Msg(OBJMSG_RPG_REPLACEPICKUP, nMessages, aint(pMsg));
				Msg.m_pData = (CRPG_Object *)pItem1;
				m_pWServer->Message_SendToObject(Msg, iObj);
				
				/*CConstraintRigidObject* pDropItem = m_spRagdollDropItems[1];
				m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_RPG_SETRIGIDBODY, (aint)pDropItem), iObj); 
				if (pDropItem)
					pDropItem->SetSingleCollisionSound(pItem->m_iSound_Dropped);*/
				Char()->RemoveItemByIdentifier(m_iObject,pItem1->m_Identifier,1,true);
			}
		}
		pCD->m_Item1_Model.Clear();
		pCD->m_Item1_Model.MakeDirty();
	}

	/*// *** Please remove me after PB ships ***
	if (bKnuckleduster)
	{
		pCD->m_Item0_Model.m_iAttachRotTrack = 44;
		pCD->m_Item0_Model.m_iAttachPoint[0] = 0;
		bKnuckleduster = true;
	}
	// *** Please remove me after PB ships ****/

	pCD->m_Item0_Model.Clear();
	pCD->m_Item0_Model.MakeDirty();
}

CRPG_Object* CWObject_Character::CreateWorldUseRPGObject(const char *_pRPGClass)
{
	MAUTOSTRIP(CWObject_Character_CreateWorldUseRPGObject, NULL);
	CRPG_Object_Item* pItem = Char()->GetInventory(RPG_CHAR_INVENTORY_ITEMS)->FindItemByName(_pRPGClass);
	if (pItem == NULL)
	{
		spCRPG_Object spObject = CRPG_Object::CreateObject(_pRPGClass, m_pWServer);
		if (spObject == NULL)
			Error("CRPG_Object_Character::OnUseInformation", CStrF("Could not create usable RPGObject %s", _pRPGClass));

		pItem = safe_cast<CRPG_Object_Item>((CRPG_Object*)spObject);
		if (pItem == NULL)
			Error("CRPG_Object_Character::OnUseInformation", CStrF("Usable RPGObject %s was not of type type CRPG_Object_Item", _pRPGClass));

		Char()->GetInventory(RPG_CHAR_INVENTORY_ITEMS)->AddChild(spObject);
		Char()->UpdateItems();
	}

	if (!Char()->IsSelected(pItem->m_iItemType))
	{		
		if (Char()->Wait() == 0)
		{
			Char()->SelectItem(pItem->m_iItemType, true, false, SELECTIONMETHOD_FORCE);
		}
	}

	return pItem;
}

int CWObject_Character::Char_Mount(const char *_pTarget, int _Flags)
{
	MAUTOSTRIP(CWObject_Character_Char_Mount, false);
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if(!pCD)
		return 0;

	if(pCD->m_iMountedObject == 0)
	{
		int iTarget = m_pWServer->Selection_GetSingleTarget(_pTarget);
		if(iTarget <= 0)
			return 0;

		pCD->m_MountedStartTick = pCD->m_GameTick + 1;
		if (_Flags & 4)
			pCD->m_MountedMode = pCD->m_MountedMode | PLAYER_MOUNTEDMODE_FLAG_BLENDANIM;
			
		//pCD->m_MountedMode = _Flags;
		if (_Flags & 2)
		{
			// Make this character invisible and turn off physics
			pCD->m_MountedMode = pCD->m_MountedMode | PLAYER_MOUNTEDMODE_FLAG_INVISIBLE;
		}

		m_MountFlags = PLAYER_MOUNTEDMODE_FLAG_ORIGINAL;
		pCD->m_MountedMode = pCD->m_MountedMode | PLAYER_MOUNTEDMODE_FLAG_ORIGINAL;

		ClientFlags() |= PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOCROUCH;
		pCD->m_iMountedObject = iTarget;
		int RetVal = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_REQUESTMOUNT, pCD->m_MountedStartTick, (pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_BLENDANIM) | (_Flags & PLAYER_MOUNTEDMODE_FLAG_RENDERHEAD), m_iObject), iTarget);
		if(!RetVal)
		{
			ClientFlags() &= ~PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK | PLAYER_CLIENTFLAGS_NOCROUCH;
			pCD->m_iMountedObject = 0;
			pCD->m_MountedMode = 0;
		}
		else
		{
			if (RetVal == 2)
			{
				pCD->m_MountedMode = pCD->m_MountedMode | PLAYER_MOUNTEDMODE_FLAG_TURRET;
				ClientFlags() |= PLAYER_CLIENTFLAGS_NOLOOK;
				pCD->m_ActionCutSceneCameraOffsetY = 0.0f;
				pCD->m_ActionCutSceneCameraOffsetX = 0.0f;
				if (m_spAI)
				{
					m_spAI->OnImpulse(CAI_Core::IMPULSE_TURRET_MOUNT,m_iObject,iTarget);
				}
			}

			m_OrgMountPos = m_pWServer->Object_GetPositionMatrix(pCD->m_iMountedObject);
			pCD->m_Control_Press = 0;
			pCD->m_Control_Move = 0;
			pCD->m_Control_Look = 0;
			return true;
		}
	}
	else if(_pTarget == NULL)
	{
		CWObject_CoreData* pMountedObj = m_pWServer->Object_GetCD(pCD->m_iMountedObject);
		if (_Flags & 8)
		{
			// Wait a bit before releasing mount mode, blendout
			pCD->m_MountedMode = pCD->m_MountedMode | PLAYER_MOUNTEDMODE_FLAG_BLENDANIM|PLAYER_MOUNTEDMODE_FLAG_BLENDOUTANIM;
			pCD->m_MountedStartTick = pCD->m_GameTick + 2;
			int RetVal = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_REQUESTMOUNT, pCD->m_MountedStartTick, pCD->m_MountedMode & (PLAYER_MOUNTEDMODE_FLAG_BLENDANIM|PLAYER_MOUNTEDMODE_FLAG_BLENDOUTANIM), m_iObject), pCD->m_iMountedObject);
		}
		else if (!pMountedObj || m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_FORCERELEASEMOUNT, 0, 0, m_iObject), pCD->m_iMountedObject))
		{
			ClientFlags() &= ~(PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK| (m_Flags & PLAYER_FLAGS_NOCROUCH ? 0 : PLAYER_CLIENTFLAGS_NOCROUCH));

			// Clear mounted camera if it's on this object
			if (pCD->m_CameraUtil.GetMountedCameraObject() == pCD->m_iMountedObject)
			{
				pCD->m_CameraUtil.Clear();
				pCD->m_CameraUtil.MakeDirty();
			}

			// Release object from player sync
			/*CWObject *pObj = m_pWServer->Object_Get(pCD->m_iMountedObject);
			if(pObj)
				pObj->SetNextRefresh(m_pWServer->GetGameTick() + 1);*/
			if (pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_TURRET)
			{
				CWAG2I_Context Context(this,m_pWServer,CMTime::CreateFromTicks(pCD->m_GameTick,m_pWServer->GetGameTickTime()));
				pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&Context,CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,0),0);
			}
			if (m_spAI)
			{
				m_spAI->OnImpulse(CAI_Core::IMPULSE_TURRET_MOUNT,m_iObject,0)	;
			}
			pCD->m_iMountedObject = 0;
			pCD->m_MountedMode = 0;
		}
		return true;
	}

	// Modify mountedflags?
	int iTarget = m_pWServer->Selection_GetSingleTarget(_pTarget);
	if (iTarget == pCD->m_iMountedObject)
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_REQUESTMOUNT, pCD->m_MountedStartTick, (pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_BLENDANIM) | (_Flags & PLAYER_MOUNTEDMODE_FLAG_RENDERHEAD), m_iObject), iTarget);

	return false;
}

CVec3Dfp32 CWObject_Character::GetCharacterCenter(CWObject_CoreData* _pChar, CWorld_PhysState* _pWPhysState, bool _bEvalSkeleton, fp32 _CenterBias)
{
	MAUTOSTRIP(CWObject_Character_GetCharacterCenter, CVec3Dfp32());
	CVec3Dfp32 Center; 

	if (_bEvalSkeleton)
	{
//		fp32 Extrapolate = 0;
		CXR_AnimState AnimState;
		CWObject_Message Msg(OBJSYSMSG_GETANIMSTATE);
		Msg.m_pData = &AnimState;
		Msg.m_DataSize = sizeof(AnimState);
//		Msg.m_Param0 = *(int *)&Extrapolate;
		Msg.m_VecParam0[0] = 1;

		if (_pWPhysState == NULL)
			return GetCharacterCenter(_pChar, NULL, false);

		if(!_pWPhysState->Phys_Message_SendToObject(Msg, _pChar->m_iObject))
			return GetCharacterCenter(_pChar, NULL, false);

		if(AnimState.m_pSkeletonInst == NULL)
			return GetCharacterCenter(_pChar, NULL, false);

		CXR_Model *pModel = _pWPhysState->GetMapData()->GetResource_Model(_pChar->m_iModel[0]);
		if(pModel == NULL)
			return GetCharacterCenter(_pChar, NULL, false);

		int iBone = 8;
		const CMat4Dfp32& BoneWorldMatrix = AnimState.m_pSkeletonInst->m_pBoneTransform[iBone];

		// True behövs tydligen (vet inte varför) när man skall sätta saker i origin på ett ben, utan att ha nån worldpos innan.
		if (true) // (False funkar minst för sniper & crossbow(!aimassist) på berserker & lich) (for attaching models)
		{
			CXR_Skeleton *pSkeleton = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
			const CXR_SkeletonNode& Node = pSkeleton->m_lNodes[iBone];
			Center = Node.m_LocalCenter;
			Center *= BoneWorldMatrix;
//			Center.SetMatrixRow(BoneWorldMatrix, 3);
//			_pWPhysState->Debug_RenderMatrix(BoneWorldMatrix, 10);
		}

//		CXR_Skeleton *pSkeleton = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
//		_pWPhysState->Debug_RenderSkeleton(pSkeleton, AnimState.m_pSkeletonInst, 0xFF00FF00, 5);
	}
	else
	{
		const CBox3Dfp32* BoundBox = _pChar->GetAbsBoundBox();

		// BoundBox is snapped to 8 units, means it usually are 8 units heighter than it should be, where 4 units bias a truer middle.
		// The we add one extra unit just to make if more centered about the chest =).
		fp32 ZOffset = 4.0f + 1.0f;
		Center.k[0] = (BoundBox->m_Max.k[0] + BoundBox->m_Min.k[0]) * 0.5f;
		Center.k[1] = (BoundBox->m_Max.k[1] + BoundBox->m_Min.k[1]) * 0.5f;
		Center.k[2] = LERP(BoundBox->m_Min.k[2], BoundBox->m_Max.k[2], 0.5f + _CenterBias) + ZOffset;
/*
		if (_pWPhysState != NULL)
		{
			_pWPhysState->Debug_RenderAABB(*BoundBox, 0xFFFFFFFF, 20);
			_pWPhysState->Debug_RenderWire(0, Center, 0xFFFFFFFF, 20);
		}
*/
	}

	return Center;
}

CVec3Dfp32 CWObject_Character::Char_GetMouthPos(CWObject_CoreData *_pObj)
{
	MAUTOSTRIP(CWObject_Character_Char_GetMouthPos, CVec3Dfp32());
	CVec3Dfp32 Pos = _pObj->GetPosition();

	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if(!pCD)
		return Pos;

	CXR_AnimState Anim;
	CXR_Skeleton* pSkel;
	CXR_SkeletonInstance* pSkelInst;
	if (GetEvaluatedPhysSkeleton(_pObj, pCD->m_pWPhysState, pSkelInst, pSkel, Anim, pCD->m_PredictFrameFrac) &&
		(PLAYER_ROTTRACK_HEAD < pSkel->m_lNodes.Len()))
	{

		Pos = pSkel->m_lNodes[PLAYER_ROTTRACK_HEAD].m_LocalCenter;
		Pos *= pSkelInst->GetBoneTransform(PLAYER_ROTTRACK_HEAD);
	}
	else// Estimate mouth position
		Pos[2] += (pCD->m_Phys_Height * 2) * 0.85f;

	return Pos;
}

bool CWObject_Character::GetActivatePosition(CMat4Dfp32& _Mat, bool _bPreciseAutoAim, uint32 _DamageType)
{
	MAUTOSTRIP(CWObject_Character_GetActivatePosition, false);
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if(!pCD)
		return false;

	if (m_CachedActivatePosition_Tick == pCD->m_GameTick &&
	    m_CachedActivatePosition_bPrecise == _bPreciseAutoAim &&
	    m_CachedActivatePosition_iDamageType == _DamageType)
	{
		_Mat = m_CachedActivatePosition_Mat;
		return true;
	}
	bool bIsControlled = (pCD->m_RenderAttached != 0 && (pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_FORCEATTACHMATRIX)) || (pCD->m_iMountedObject > 0);

	if ((m_bAIControl || IsBot()) && !bIsControlled)
	{
		m_spAI->GetActivatePosition(&_Mat);
	}
	else
	{
		MSCOPE(CWObject_Character::GetActivatePosition, CHARACTER);
		if(pCD->m_RenderAttached != 0)
		{
			CWObject *pObj = m_pWServer->Object_Get(pCD->m_RenderAttached);
			if(pObj)
			{
				CMTime Time = pCD->m_GameTime;
				CWObject_Message Msg(OBJMSG_HOOK_GETTIME);
				Msg.m_pData = &Time;
				Msg.m_DataSize = sizeof(CMTime);
				m_pWServer->Phys_Message_SendToObject(Msg, pCD->m_RenderAttached);
				Msg = CWObject_Message(OBJMSG_HOOK_GETRENDERMATRIX, (aint)&Time, (aint)&_Mat);
				//			fp32 Time = 0;
				//			CWObject_Message Msg(OBJMSG_HOOK_GETRENDERMATRIX, (int)&Time, (int)&MatIP);
				if(!m_pWServer->Message_SendToObject(Msg, pCD->m_RenderAttached))
					_Mat = pObj->GetPositionMatrix();
			}
			else
				_Mat = GetPositionMatrix();
		}
		else
		{
			_Mat = GetPositionMatrix();
		}
		/*_Mat.RotX_x_M(-0.25f);
		_Mat.RotY_x_M(0.25f);*/

		Camera_Get(m_pWServer, &_Mat, this, 0);
		_Mat.RotY_x_M(-0.25f);
		_Mat.RotX_x_M(0.25f);
		
		bool bValid = false;
		if (pCD->Target_GetObject() != 0)
		{
			CVec3Dfp32 AimOrigin = CVec3Dfp32::GetMatrixRow(_Mat, 3);
			
			if (_bPreciseAutoAim)
			{
				int iTarget = pCD->Target_GetObject();
				CWObject* pTarget = m_pWServer->Object_Get(iTarget);
				CWObject_Character* pTargetChar = safe_cast<CWObject_Character>(pTarget);
				if (pTargetChar != NULL)
				{
					if (AssistAim(pCD) && (pCD->m_CrosshairInfo & PLAYER_CROSSHAIR_INSIDE_TARGETRING))
					{
						CVec3Dfp32 TargetPos = GetCharacterCenter(pTarget, m_pWServer, true, 0.25f);
						
						bool bAimAssistBetter = true;
						
						CCollisionInfo CInfo;
						CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
						int objects = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
						int mediums = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID;
						
//						_pWPhysState->Debug_RenderWire(AimOrigin, TargetPos, 0xFFFF0000, 10);
//						_pWPhysState->Debug_RenderVertex(TargetPos, 0xFFFFFFFF, 10);
						
						bool bHit = m_pWServer->Phys_IntersectLine(AimOrigin, TargetPos, 0, objects, mediums, &CInfo, m_iObject);
						
						if (bHit && (iTarget == CInfo.m_iObject) && (GetObjFlags(pTargetChar) & OBJECT_FLAGS_CHARACTER))
						{
							if (pCD->m_FreeAimedTargetSurfaceType & 128)
							{
								int HitLoc = GetHitlocation(&CInfo);
								int AssistAimedTargetSurfaceType = CInfo.m_SurfaceType;
								int FreeAimedTargetSurfaceType = pCD->m_FreeAimedTargetSurfaceType & ~128;
								// NOTE: We do not need to use tweak damage as all values will scale the same
								fp32 AssistedAimDamageFactor = pTargetChar->GetHitlocDamageFactor(_DamageType,HitLoc,AssistAimedTargetSurfaceType);
								fp32 FreeAimDamageFactor = pTargetChar->GetHitlocDamageFactor(_DamageType,HitLoc,FreeAimedTargetSurfaceType);
								bAimAssistBetter = AssistedAimDamageFactor > FreeAimDamageFactor;
							}
						}
						else if (CInfo.m_iObject != iTarget)
						{
							bAimAssistBetter = false;
						}
						
						if (bAimAssistBetter)
						{
							CVec3Dfp32 TargetVector = TargetPos - AimOrigin;
							CVec3Dfp32 TargetDir = TargetVector; TargetDir.Normalize();
							CVec2Dfp32 AimAngles2D = pCD->Target_GetAnglesForVector(TargetDir);
							CVec3Dfp32 AimAngles3D(0, AimAngles2D[0], AimAngles2D[1]);
							AimAngles3D.CreateMatrixFromAngles(0, _Mat);
							AimOrigin.SetMatrixRow(_Mat, 3);
						} // else free aim is better, return CameraAimMatrix unmodified.
						
						// Return either free aim or accurate target center.
						bValid = true;
					}
				}
			}
			if(!bValid)
			{
				// Return unaccurate target center.

				// FIXME: Removed these during SP cruch since it obviously didn't work very well - JA
//				pCD->Target_GetAimMatrix(_Mat);
//				AimOrigin.SetMatrixRow(_Mat, 3);
			}
		} // else no target, return free aim.
	}

	m_CachedActivatePosition_Tick = pCD->m_GameTick;
	m_CachedActivatePosition_bPrecise = _bPreciseAutoAim;
	m_CachedActivatePosition_iDamageType = _DamageType;
	m_CachedActivatePosition_Mat = _Mat;

	return true;	
}

int CWObject_Character::Physics_Kill(uint32 _DamageType, int _iSender)
{
	MAUTOSTRIP(CWObject_Character_Physics_Kill, 0);
	if (_iSender != m_iObject)
	{
		Char()->Health() = -999;
		Char()->Pain() = 50;
		Char_Die(_DamageType, _iSender);
		return 1;
	}
	return 0;
}

int CWObject_Character::Physics_Shockwave(const CWO_ShockwaveMsg &_DX, int _iSender)
{
	MAUTOSTRIP(CWObject_Character_Physics_Shockwave, 0);
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
	if((m_Flags & PLAYER_FLAGS_WAITSPAWN) || !pCD || (pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMUNE))
		return 0;

	fp32 h = fp32(GetPhysState().m_Prim[0].m_DimZ) * 2.0f;
	CVec3Dfp32 TracePos = GetPosition();
	TracePos[2] += h*0.5f;
	
	CVec3Dfp32 Vel;
	CWO_DamageMsg Msg;
	if(_DX.GetTracedDamage(m_iObject, TracePos, pCD->m_Phys_Width * _SQRT2, h, m_pWServer, Msg))
		return Physics_Damage(Msg, _iSender);
	return 0;
	
/*	CCollisionInfo CInfo;
	if(!m_pWServer->Phys_IntersectLine(TracePos, Pos, 0, OBJECT_FLAGS_PHYSMODEL, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &CInfo, m_iObject))
	{
		CVec3Dfp32 v = TracePos - Pos;

		fp32 r = Max(0.0f, Length2(v[0], v[1]) - pCD->m_Phys_Width*fp32(_SQRT2));
		fp32 dh = 0;
		if (Pos[2] < GetPosition()[2])
			dh = GetPosition()[2] - Pos[2];
		else if(Pos[2] > GetPosition()[2] + h)
			dh = Pos[2] - (GetPosition()[2] + h);

		v.Normalize();

		fp32 d = Length2(dh, r);
		if (d < _DX.m_SplashRadius)
		{
			fp32 i = (1.0f - (d / _DX.m_SplashRadius)) * _DX.m_SplashForce;
			fp32 Force = (1.0f - (d / _DX.m_SplashRadius)) * _DX.m_SplashForce * GetDamageFactor(PLAYER_HITLOCATION_SKIN, _DX.m_DamageType);
			if(!(pCD->m_Phys_Flags & (PLAYER_PHYSFLAGS_IMMUNE | PLAYER_PHYSFLAGS_IMMOBILE)))
				m_pWServer->Object_AddVelocity(m_iObject, v * Force * 0.35f);

			if (m_iObject == _iSender)
				i *= PLAYER_SELFSPLASH_SCALE;

			Damage += i;
		}
	}*/
}

int CWObject_Character::Char_DarknessIsTintSound(CWO_Character_ClientData* _pCD)
{
	// Add powers as needed
	return (_pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD);
}

int CWObject_Character::Char_DarknessIsTintView(CWO_Character_ClientData* _pCD)
{
	// Add powers as needed
	return (_pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD);
}


int CWObject_Character::Char_RefreshDarknessPowers(CWO_Character_ClientData* _pCD, fp32 /*_Visibility*/)
{
//	bool bDarkEnough = (_pCD->m_DarknessVisibility > 0);
	bool bDarkEnough = true;	//TEMP -- possible to use darkness anytime, anywhere
	bool bGodMode = (_pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_GODMODE) != 0;

	// Changed else if's to if's since code was unable to reach other test and got stuck at certain states
	// when ancient weapons was active
	int DrainCost = 0;

//	if (_pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS)
//	{
//		// Send unequip impulse if weaponselect is still queued
//		CWAG2I_Context AGContext(this,m_pWServer,_pCD->m_GameTime);
//		if (_pCD->m_AnimGraph2.GetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED))
//			_pCD->m_AnimGraph2.SendAttackImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_UNEQUIP));
//
//		// Don't deactivate on timeout anymore, remove when out of darkness
//		//if(m_pWServer->GetGameTime().Compare(m_AncientTimeout) > 0)
//		if (_pCD->m_Darkness == 0)
//			Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS,true,false);
//	}
	//else
	if (_pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK)
	{
		// Check if we have reached end 
		CWObject_CoreData* pObj = m_pWServer->Object_GetCD(_pCD->m_iCreepingDark);
		if (pObj)
		{
			// Never turn off creepingdark automatically... Let them be creeping dark active for as long as they want...
			// Like, scouting out the area for example.
			//if ((CWObject_CreepingDark::GetStartTick(pObj) + m_pWServer->GetGameTicksPerSecond() * 30.0f) < m_pWServer->GetGameTick() && 
			//	!CWObject_CreepingDark::BackTrackActive(pObj))
			//	m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CREEPINGDARK_BACKTRACK),pObj->m_iObject);

			if (CWObject_CreepingDark::BackTrackReachedEnd(pObj) && ((CWObject_CreepingDark::GetEndTick(pObj) + 5) < m_pWServer->GetGameTick()))
			{
				Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_CREEPINGDARK,true,false);
				if (!_pCD->m_Darkness.m_Value)
				{
					Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD, true, false);
					Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSVISION, true, false);
				}
			}
			else if (!bGodMode)
			{
				if (!CWObject_CreepingDark::BackTrackActive(pObj))
				{
					DrainCost = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CREEPINGDARK_GETDARKNESSCOST), pObj->m_iObject);
					if (_pCD->m_Darkness.m_Value < DrainCost)
						m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CREEPINGDARK_BACKTRACK),pObj->m_iObject);
				}
			}
		}
	}
	//else
	if (_pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DEMONARM)
	{
		// Deactivate the arm when walking in light or when out of juice, OR if we aren't holding darkness button down
		if ((!bDarkEnough || (!_pCD->m_Darkness && !bGodMode)) || !(_pCD->m_Control_Press & CONTROLBITS_BUTTON2))
		{
			Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DEMONARM, true, false);
		}
		else if (_pCD->m_Darkness > 0 && !bGodMode)
		{
			if (!(_pCD->m_GameTick % TruncToInt(m_pWServer->GetGameTicksPerSecond())))
				DrainCost = 1;
		}
	}

	if (_pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_BLACKHOLE)
	{
		// Continue to drain darkness while blackhole is runnning
		CWObject_BlackHole* pBlackHole = (m_iDarkness_BlackHole) ? safe_cast<CWObject_BlackHole>(m_pWServer->Object_Get(m_iDarkness_BlackHole)) : NULL;
		if (pBlackHole)
		{
			if (pBlackHole->IsActive())
			{
				if (_pCD->m_Darkness.m_Value > 0)
				{
					DrainCost = _pCD->m_Darkness.m_Value - pBlackHole->GetDarkness(_pCD->m_MaxDarkness.m_Value, m_pWServer->GetGameTick());

					// Add force until out of juice
					if (_pCD->m_Darkness > DrainCost)
						pBlackHole->AddForce();
					else
					{
						pBlackHole->SetExplode(false);
						pBlackHole->Deactivate();
					}
				}
			}
			else
			{
				// If blackhole has finished its buisness, reset our power flag
				_pCD->m_DarknessSelectionMode = _pCD->m_DarknessSelectionMode & ~PLAYER_DARKNESSMODE_POWER_BLACKHOLE;
			}
		}
	}

	// Deactive ancient weapons when juice is depleted
	/*if (_pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS)
	{
		if (_pCD->m_Darkness == 0)
			Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS, true, false);
	}*/

	// Set player darkness mode
	CAI_Core* pAI = AI_GetAI();
	if (pAI)
	{
		if ((_pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_AIDARKNESSMASK) != 0)
		{
			pAI->SetPlayerDarkness(_pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_AIDARKNESSMASK);
			if (_pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK)
			{
				CWObject_CoreData* pObj = m_pWServer->Object_GetCD(_pCD->m_iCreepingDark);
				if (pObj)
				{
					pAI->SetDarknessPos(pObj->GetPosition());
				}
			}
			else if (_pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DEMONARM)
			{
				CVec3Dfp32 Pos = Char_GetDarknessPosition();
				if (VALID_POS(Pos))
					pAI->SetDarknessPos(Pos);
				// Don't clear if false, we still want bots to see the old pos until expired 
			}
		}
		else
		{
			pAI->SetPlayerDarkness(0);
			// Don't clear, we still want bots to see the old pos until expired 
			// pAI->ClearDarknessPos();
		}
	}
		

	// Remove darklingspawned flag if present
	if (_pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_DARKLINGSPAWNED)
		_pCD->m_DarknessSelectionMode = _pCD->m_DarknessSelectionMode & ~PLAYER_DARKNESSMODE_DARKLINGSPAWNED;

	return DrainCost;
}

CVec3Dfp32 CWObject_Character::Char_GetDarknessPosition()
{
	CWO_Character_ClientData* pCD = GetClientData(this);
	// Only supports (and assumes) main tentacle position
	CWObject* pObj = m_pWServer->Object_Get(pCD->m_iDarkness_Tentacles);
	if (pObj)
	{
		CWObject_TentacleSystem& TentacleSystem = *safe_cast<CWObject_TentacleSystem>(pObj);
		CVec3Dfp32 Pos;
		if (TentacleSystem.GetMainTentaclePos(&Pos))
			return Pos;
	}
	return CVec3Dfp32(_FP32_MAX);
}


void CWObject_Character::Char_RefreshDarkness()
{
	CWO_Character_ClientData* pCD = GetClientData(this);
	if (!pCD)
		return;

	if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), m_pWServer->Game_GetObjectIndex()))
		return;

	fp32 Visibility = m_AverageLightIntensity;
	bool bDarkEnough = (pCD->m_DarknessVisibility > 0);		// TEMP - always dark enough!

	if(!(pCD->m_DarknessPowersAvailable & PLAYER_DARKNESSMODE_COREPOWERMASK))
	{
		pCD->m_DarknessSelectedPower = -1;
		pCD->m_DarknessLastSelectedPower = -1;
	}
	else if(pCD->m_DarknessSelectedPower < 0)
	{
		pCD->m_DarknessSelectedPower = 0;
		pCD->m_DarknessLastSelectedPower = 0;
	}

	// Remove any drain cost
	int nDrain = Char_RefreshDarknessPowers(pCD, Visibility);
	pCD->m_Darkness = Max(0, (int)pCD->m_Darkness - nDrain);

	// Update darkness amount every tick
	if ((pCD->m_GameTick % 1) == 0 && (pCD->m_GameTick > 2) && (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_NOREGENMASK) == 0)
	{
		int Step = pCD->m_MaxDarkness / 4;

		// Drain shield (health) when in light
		/*
		if (!bDarkEnough && ((pCD->m_GameTick % 8) == 0))
		{
			// Drain health a bit
			if (Char()->Health() > 20)
			{
				Char()->ReceiveHealth(-1);
				pCD->m_Health = Char()->Health();
			}
		}
		*/

		// Update which powers can be activated
		pCD->m_DarknessPowers = pCD->m_DarknessPowers | pCD->m_DarknessPowersAvailable;
	}

	// Deactivate darkness mode if out of juice
	if ((pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD) && !pCD->m_Darkness && 
		!(pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS))
	{
		Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD, true, false);
		Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSVISION, true, false);
	}

	// Make darkness speak around characters with powers inside them (only check once every odd second)
	if (!(pCD->m_GameTick % TruncToInt(m_pWServer->GetGameTicksPerSecond() * 2.0f)))
	{
		TSelection<CSelection::MEDIUM_BUFFER> Selection;
		m_pWServer->Selection_AddBoundSphere(Selection, OBJECT_FLAGS_CHARACTER, GetPosition(), 196.0f);
		const int16* pSel = NULL;
		uint nSel = m_pWServer->Selection_Get(Selection,&pSel);
		bool bDarknessVoice = true;
		for (uint i = 0; i < nSel; i++)
		{
			CWObject_Character* pChar = safe_cast<CWObject_Character>(m_pWServer->Object_Get(pSel[i]));
			if(pChar && !(pChar->m_DarknessFlags & DARKNESS_FLAGS_VOICE))
			{
				int nDarknessPowers = pChar->m_lDarknessPowerups.Len();
				if (nDarknessPowers)
				{
					if (bDarknessVoice)
					{
						static int iLastDialogue = -1;
						int iDialogue = MRTC_GetRand()->GenRand32() % 10;
						if (iDialogue == iLastDialogue)
							iDialogue = (iDialogue + 1) % 10;
						iLastDialogue = iDialogue;

						switch (iDialogue)
						{
						case 0:
							PlayDialogue_Hash(PLAYER_DIALOGUE_DARKNESS_POWER01);
							break;
						case 1:
							PlayDialogue_Hash(PLAYER_DIALOGUE_DARKNESS_POWER02);
							break;
						case 2:
							PlayDialogue_Hash(PLAYER_DIALOGUE_DARKNESS_POWER03);
							break;
						case 3:
							PlayDialogue_Hash(PLAYER_DIALOGUE_DARKNESS_POWER04);
							break;
						case 4:
							PlayDialogue_Hash(PLAYER_DIALOGUE_DARKNESS_POWER05);
							break;
						case 5:
							PlayDialogue_Hash(PLAYER_DIALOGUE_DARKNESS_POWER06);
							break;
						case 6:
							PlayDialogue_Hash(PLAYER_DIALOGUE_DARKNESS_POWER07);
							break;
						case 7:
							PlayDialogue_Hash(PLAYER_DIALOGUE_DARKNESS_POWER08);
							break;
						case 8:
							PlayDialogue_Hash(PLAYER_DIALOGUE_DARKNESS_POWER09);
							break;
						case 9:
							PlayDialogue_Hash(PLAYER_DIALOGUE_DARKNESS_POWER10);
							break;
						}
					}

					// Voice tagged
					pChar->m_DarknessFlags |= DARKNESS_FLAGS_VOICE;
				}
			}
		}
	}
}

#define PLAYER_DARKNESSMODE_DRAINMAXRANGE (384.0f)
//#define PLAYER_DARKNESSMODE_BLACKHOLEUNITCOST (0.25f)
//#define DARKNESSPOWER_DEBUG
#ifdef DARKNESSPOWER_DEBUG
#define DARKNESSDEBUG(s) {ConOut(s); M_TRACEALWAYS((s + "\n").Str());}
#define DARKNESSSTUFF(s) { s }
#else
#define DARKNESSDEBUG(s)
#define DARKNESSSTUFF(s)
#endif


void CWObject_Character::CheckDarklingSpawnPoints(const CVec3Dfp32& _SelectPos, fp32 _Radius, int16& _iBest)
{
	TSelection<CSelection::LARGE_BUFFER> Selection;
	m_pWServer->Selection_AddBoundSphere(Selection, OBJECT_FLAGS_PICKUP, _SelectPos, _Radius);
	m_pWServer->Selection_RemoveOnNotClass(Selection,"DarklingSpawn");

	CWObject_Message Msg(OBJMSG_DARKLINGSPAWN_ISCLOSESPAWN);
	Msg.m_iSender = m_iObject;
	Msg.m_VecParam0 = _SelectPos;
	const CVec3Dfp32 Dir = GetPositionMatrix().GetRow(0);
	fp32 DotBest = -1.0f;
	fp32 ScoreBest = -1.0f;
	const int16* pSel = NULL;
	int32 nSel = m_pWServer->Selection_Get(Selection,&pSel);
	fp32 RcpRadius = 1.0f / _Radius;
	for (int32 i = 0; i < nSel; i++)
	{
		// Make sure there's a free line of sight ?
		CVec3Dfp32 ObjPos = m_pWServer->Object_GetPosition(pSel[i]);
		if (m_pWServer->Phys_Message_SendToObject(Msg,pSel[i]) && // Check if usable (returns 1)
			!m_pWServer->Phys_IntersectLine(_SelectPos, ObjPos, 
			OBJECT_FLAGS_PROJECTILE, OBJECT_FLAGS_WORLD, 
			XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, NULL, m_iObject))
		{
			// Check which is the best one
			CVec3Dfp32 DirToSpawn = ObjPos - _SelectPos;
			DirToSpawn.Normalize();
			fp32 Dot = DirToSpawn * Dir;
			
			// Consider length to object also
			fp32 Length = (ObjPos - _SelectPos).Length();
			fp32 Score = Dot + (1.0f - (Length * RcpRadius));
			if (Score > ScoreBest)
			{
				ScoreBest = Score;
				_iBest = pSel[i];
			}
		}
	}
}



void CWObject_Character::Char_CheckDarknessActivation(int _ControlPress, int _ControlLastPress, const CVec3Dfp32& _ControlMove)
{
	MSCOPESHORT(CWObject_Character::Char_CheckDarknessActivation);
	CWO_Character_ClientData* pCD = GetClientData(this);

	// Ignore darkness stuff in multiplayer?
	if (m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), m_pWServer->Game_GetObjectIndex()))
		return;

	CWObject* pTentacleObj = m_pWServer->Object_Get(pCD->m_iDarkness_Tentacles);
	CWObject_TentacleSystem* pTentacleSystem = pTentacleObj ? safe_cast<CWObject_TentacleSystem>(pTentacleObj) : NULL;


	uint8 PrevDarkness = pCD->m_Darkness;
	uint8 PrevHealth = pCD->m_Health;
	bool bForceHud = false;
	// If light is above certain level, don't activate any darkness powers (can only be activated in darkness)
	bool bDarkEnough = (pCD->m_DarknessVisibility > 0);
	bool bDarknessPowersDisabled = (pCD->m_Disable & PLAYER_DISABLE_DARKNESSPOWERS) != 0;
	bool bIsInDarknessMode = (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD) != 0;
	bool bCanEnterDarknessMode = !bDarknessPowersDisabled && (pCD->m_DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD) != 0 && (pCD->m_Darkness > 0);

	// POWERS (should perhaps be in clientdata so it can be displayed when rendering
	bool bPowerSwitchUp = ((_ControlPress & CONTROLBITS_DPAD_UP) && !(_ControlLastPress & CONTROLBITS_DPAD_UP));
	bool bPowerSwitchDown = false;// -- disabled. this button is for unequip...     ((_ControlPress & CONTROLBITS_DPAD_DOWN) && !(_ControlLastPress & CONTROLBITS_DPAD_DOWN));

	// Cycle to next darkness power? 
	// If we add side effects of selecting power, be sure to change the handling of OBJMSG_SELECTDARKNESSPOWER as well. 
	// This code and that should really be broken out nicely instead...
	if ((bPowerSwitchUp || bPowerSwitchDown))
	{
		// Deactivate ancient weapons if we try to select any other.  
		if (pCD->m_DarknessSelectionMode.m_Value & PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS)
			Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS, true, false);
		if (pCD->m_Darkness > 0 && !(pCD->m_DarknessSelectionMode.m_Value & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD))
		{
			Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD, true, true);
			Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSVISION, true, true);
		}
	}
	if ((bPowerSwitchUp || bPowerSwitchDown) && !(pCD->m_DarknessSelectionMode.m_Value & PLAYER_DARKNESSMODE_ACTIVATEPOWERMASK) &&
		!(_ControlPress & CONTROLBITS_BUTTON1 && m_iBestDarklingSpawn))
	{
		bForceHud = true;
		int NumPowersavailable = 0;
		int iSelectPower = (bPowerSwitchUp) ? 1 : -1;
		if(pCD->m_DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK)
			NumPowersavailable++;
		if(pCD->m_DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_DEMONARM)
			NumPowersavailable++;
		if(pCD->m_DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_BLACKHOLE)
			NumPowersavailable++;
		if(pCD->m_DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS)
			NumPowersavailable++;
		pCD->m_DarknessLastSelectedPower = pCD->m_DarknessSelectedPower;

		if(NumPowersavailable > 1)
		{
			uint16 ResolvePower = 0;
			int SelectedPower = pCD->m_DarknessSelectedPower;
			do {
				SelectedPower = (SelectedPower + iSelectPower + PLAYER_DARKNESSMODE_SELECTION_NUMPOWERS) % PLAYER_DARKNESSMODE_SELECTION_NUMPOWERS;
				ResolvePower = ResolveDarknessSelection(SelectedPower);
			} while(/*ResolvePower == PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS ||*/ !(ResolvePower & pCD->m_DarknessPowersAvailable));
			pCD->m_DarknessSelectedPower = SelectedPower;
			//ConOut(CStrF("Selected power: %d",SelectedPower));
		}
		else if(NumPowersavailable)
		{
			if(pCD->m_DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK)
				pCD->m_DarknessSelectedPower = 0;
			if(pCD->m_DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_DEMONARM)
				pCD->m_DarknessSelectedPower = 1;
			if(pCD->m_DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_BLACKHOLE)
				pCD->m_DarknessSelectedPower = 3;
			if(pCD->m_DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS)
				pCD->m_DarknessSelectedPower = 2;
		}
	}

	bool bPrevDrainState = (pCD->m_DarknessSelectionMode.m_Value & PLAYER_DARKNESSMODE_POWER_DRAIN) != 0;
	pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode.m_Value & ~PLAYER_DARKNESSMODE_POWER_DRAIN;


	// Check Darkness mode ("Darkness Shield") toggle
	if (pCD->m_Control_Click & CONTROLBITS_BUTTON5)
	{
		pCD->m_Control_Click &= ~CONTROLBITS_BUTTON5;	// TODO: this should be handled automatically, but currently m_Control_Click is updated from OnRefresh() while OnPress() is called for every input, so multiple OnPress() can occur before m_Control_Click gets updated (i.e. cleared) again

		// Don't allow turning on/off powers while a power is in use.
		uint DarknessSelectionModeMasked = pCD->m_DarknessSelectionMode & (PLAYER_DARKNESSMODE_POWERMASK | PLAYER_DARKNESSMODE_ACTIVATEPOWERMASK);
		if ((DarknessSelectionModeMasked & ~(PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD | PLAYER_DARKNESSMODE_POWER_DARKNESSVISION)) == 0)
		{
			if (!bIsInDarknessMode && bCanEnterDarknessMode)
			{
				// Activate darkness mode
				M_TRACE("[%d], Activate darkness mode\n", pCD->m_GameTick);
				Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD, true, true);
				Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSVISION, true, true);
				bIsInDarknessMode = true;
				bForceHud = true;
			}
			else if (bIsInDarknessMode)
			{
				// Deactivate darkness mode
				M_TRACE("[%d], Deactivate darkness mode\n", pCD->m_GameTick);
				Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD, true, false);
				Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSVISION, true, false);
				bIsInDarknessMode = false;
				//bForceHud = true;
			}
		}
	}

	//Creeping Dark jump, disabled, is disabled because of problems with landing on phys object
/*	if((pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_ACTIVATEPOWERMASK) & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK)
	{
		if(pCD->m_Control_Press & CONTROLBITS_JUMP && !(pCD->m_Control_LastPress & CONTROLBITS_JUMP))
		{
			m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CREEPINGDARK_JUMP), pCD->m_iCreepingDark);
		}
	}*/

	// Check Darkness drain
	bool bPowerDrain = true;
	if (!bDarknessPowersDisabled && (pCD->m_Control_Hold & CONTROLBITS_BUTTON5))
	{
		bForceHud = true; // Show health hud

		// Check for things to devour before rechargin darkness power...
		if (!(pCD->m_Control_LastHold & CONTROLBITS_BUTTON5) && !((pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_ACTIVATEPOWERMASK) & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK))
		{
			pCD->m_Control_LastHold |= CONTROLBITS_BUTTON5; // TODO: this should be handled automatically, but currently m_Control_LastHold is updated from OnRefresh() while OnPress() is called for every input, so multiple OnPress() can occur before m_Control_LastHold gets updated

			ConOut("Trying to devour (will start draining next tick...)");
			int iBest = -1;
			int32 iSel = -1, iCloseSel = -1;
			int8 SelType = 0;
			bool bCurrentNoPrio = false; 
			// Traceforward to find a good position to look for stuff at
			// Find spawn pos
			// Determine spawn position
			const CMat4Dfp32& Mat = GetPositionMatrix();
			CMat4Dfp32 SpawnEffectMat;
			const CVec3Dfp32& Dir = Mat.GetRow(0);
			CVec3Dfp32 StartPos = Mat.GetRow(3) + CVec3Dfp32(0,0,54);
			CVec3Dfp32 SpawnPos = StartPos + Dir * 512.0f; // Max range: 16 meters
			SpawnEffectMat.GetRow(0) = Mat.GetRow(1);

			CCollisionInfo CInfo;

			bool bHit = m_pWServer->Phys_IntersectLine(
				StartPos, SpawnPos, OBJECT_FLAGS_CHARACTER, 
				OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL, 
				XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &CInfo, m_iObject);


			CVec3Dfp32 FindStuffPos;
			CVec3Dfp32* pFindStuffPos = NULL;
			if (bHit && CInfo.m_bIsValid && ((CInfo.m_Pos - Mat.GetRow(3)).LengthSqr() < Sqr(200.0f)))
			{
				// Find a good Findstuff pos withing a certain range..
				FindStuffPos = CInfo.m_Pos - Dir * 5.0f;
				pFindStuffPos = &FindStuffPos;
			}

			// Check for dead people
			if (Char_FindStuff(m_pWServer,this,pCD,iSel,iCloseSel,SelType,bCurrentNoPrio,FINDSTUFF_SELECTIONMODE_DEVOURING,-1.0f,0,pFindStuffPos))
			{
				bool bCrouch = (Char_GetPhysType(this) == PLAYER_PHYS_CROUCH);

				bool bIsChar = (SelType & SELECTION_MASK_TYPEINVALID) == SELECTION_CHAR;
				bool bIsDead = (SelType & SELECTION_MASK_TYPEINVALID) == SELECTION_DEADCHAR;
				bool bIsStunned = false;
				if (bIsChar && !bIsDead)
				{
					CWObject_Character* pTarget = safe_cast<CWObject_Character>(m_pWServer->Object_Get(iSel));
					CAI_Core* pAI = pTarget->AI_GetAI();
					bIsStunned = (pAI) ? !pAI->IsConscious() : false;
				}

				if ((SelType & SELECTION_MASK_TYPEINVALID) == SELECTION_USABLEOBJECT && !(pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_NOUSEMASK))
				{
					m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_USE), iSel);
					iBest = -1; // so the code below won't try to handle it
					bPowerDrain = false;
				}
			}
		}

		bool bOldDarkEnough = bDarkEnough;
		bDarkEnough = true;
		fp32 LightIntensity = 1.0f;
		if (bPowerDrain && !(pCD->m_DarknessSelectionMode.m_Value & PLAYER_DARKNESSMODE_NOREGENMASK))
		{
			// Only drain if needed
			if (pCD->m_Darkness < pCD->m_MaxDarkness || Char()->Health() < Char()->MaxHealth())
				LightIntensity = m_spAI->GetLightIntensity(m_iObject);
			/* Old draining code, remove ?
			{
				// If we are not standing in a dark enough area, shoot out a ray and check if we are pointing at a dark enough area
				if(m_spAI->GetLightIntensity(m_iObject) <= 0.3f)
				{
					bDarkEnough = true;
				}
				else
				{
					CXR_AnimState Anim;
					CXR_Skeleton* pSkel = NULL;
					CXR_SkeletonInstance* pSkelInstance = NULL;
					if(m_spAI && GetEvaluatedPhysSkeleton(pSkelInstance, pSkel, Anim) && pSkelInstance && PLAYER_ROTTRACK_CAMERA < pSkelInstance->m_nBoneTransform)
					{
						const CVec3Dfp32 CamPos = pSkel->m_lNodes[PLAYER_ROTTRACK_CAMERA].m_LocalCenter * pSkelInstance->GetBoneTransform(PLAYER_ROTTRACK_CAMERA);
						const CVec3Dfp32 TargetPos = CamPos + GetPositionMatrix().GetRow(0) * PLAYER_DARKNESSMODE_DRAINMAXRANGE;
						const int ObjectFlags = CXR_COLLISIONTYPE_PROJECTILE;
						const int IntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
						const int MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID;
						CCollisionInfo CInfo;
						const bool bHit = m_pWServer->Phys_IntersectLine(CamPos, TargetPos, ObjectFlags, IntersectFlags, MediumFlags, &CInfo, m_iObject);
						if(bHit && CInfo.m_bIsValid)
						{
							// Create a temporary light meter and measure the point where we collided with the ray
							CWObj_ObjLightMeter TempLightMeter;
							const CVec3Dfp32 MeasurePos = CInfo.m_Pos + CInfo.m_Plane.n;
							TempLightMeter.Init(m_pWServer, 0);
							TempLightMeter.Measure(m_pWServer, true, MeasurePos, MeasurePos, 4.0f, 4.0f, 10, true, m_iObject);
							if(TempLightMeter.GetIntensity(m_iObject) <= 0.3f)
								bDarkEnough = true;
						}
					}
				}
			}
			*/

			// If we been hit, drain isn't allowed in 2 seconds (otherwise we could run around and just drain)
			bool bCanDrain = (Char()->m_HealthRegenerationDelay < m_pWServer->GetGameTick());
			if(bCanDrain && !(pCD->m_Disable & PLAYER_DISABLE_DARKNESSPOWERS))
			{
				// First charge darkness, then recharge health
				if (pCD->m_Darkness < pCD->m_MaxDarkness)
				{
					m_DarknessDrain += MaxMT(0.01f, (1.0f - (LightIntensity*0.8f)) / (0.1f * m_pWServer->GetGameTicksPerSecond()));
					int ChargeDarkness = TruncToInt(m_DarknessDrain);
					if (ChargeDarkness)
					{
						m_DarknessDrain -= fp32(ChargeDarkness);
						pCD->m_Darkness = Min(uint(pCD->m_MaxDarkness.m_Value), uint(pCD->m_Darkness.m_Value) + ChargeDarkness);
					}
				}
				else if (Char()->Health() < Char()->MaxHealth())
				{
					m_DarknessDrain += MaxMT(0.01f, (1.0f - LightIntensity) / (0.3f * m_pWServer->GetGameTicksPerSecond()));
					int ChargeHealth = TruncToInt(m_DarknessDrain);
					if (ChargeHealth)
					{
						m_DarknessDrain -= fp32(ChargeHealth);
						Char()->ReceiveHealth(ChargeHealth);
						pCD->m_Health = Char()->Health();
					}
				}
				else
				{
					// Don't allow restart of drain in 5 seconds after darkness and health fills up to max
					if (pCD->m_Darkness == pCD->m_MaxDarkness && pCD->m_Health == pCD->m_MaxHealth)
						Char()->m_HealthRegenerationDelay = m_pWServer->GetGameTick() + TruncToInt(m_pWServer->GetGameTicksPerSecond() * 5.0f);
				}

				pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode.m_Value | PLAYER_DARKNESSMODE_POWER_DRAIN;

				if (!(pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSVISION))
				{
					Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSVISION, true, true);
					Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD, true, true);
				}
			}
		}
		bDarkEnough = bOldDarkEnough;
	}
	if (!bPowerDrain)
		pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode.m_Value & ~PLAYER_DARKNESSMODE_POWER_DRAIN;

	// Play start/stop loop sound for darkness draining
	bool bCurDrainState = (pCD->m_DarknessSelectionMode.m_Value & PLAYER_DARKNESSMODE_POWER_DRAIN) != 0;
	if ((bPrevDrainState && !bCurDrainState) || (!bPrevDrainState && bCurDrainState))
	{
		// Play start/end drain sound
		int iDrainSoundInOut = m_pWServer->GetMapData()->GetResourceIndex_Sound((bPrevDrainState) ? "gam_drk_drain_2d_stop" : "gam_drk_drain_2d_start");
		int iDrainLoopSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("gam_drk_drain_2d_loop");
		
		if (iDrainSoundInOut)
			m_pWServer->Sound_At(GetPosition(), iDrainSoundInOut, WCLIENT_ATTENUATION_3D);
	
		if (bPrevDrainState && iDrainLoopSound)
			m_pWServer->Sound_Off(m_iObject, iDrainLoopSound);
		else if (!bPrevDrainState && iDrainLoopSound)
			m_pWServer->Sound_On(m_iObject, iDrainLoopSound, WCLIENT_ATTENUATION_FORCE_3D_LOOP);

		// Fade in out effect
		/* REMOVED, SAVED FOR REFERENCE
		CWObject_Message MsgDrain(OBJMSG_EFFECTSYSTEM_SETFADE, (bPrevDrainState) ? 2 : 1);
		m_pWServer->Message_SendToObject(MsgDrain, m_iDarkness_Drain_2);
		*/

		if (bCurDrainState)
		{
			Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD, true, true);
			Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSVISION, true, true);
		}
	}


	// Every 0.2 secs or so check for darklings
	if ((pCD->m_GameTick - m_LastDarklingSpawnCheck) > (int)(m_pWServer->GetGameTicksPerSecond() * 0.2f))
	{
		int OldDarklingsSpawn = m_iBestDarklingSpawn;
		m_iBestDarklingSpawn = 0;
		CVec3Dfp32 SelectPos = GetPosition();
		if(pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK)
			SelectPos = m_pWServer->Object_GetPosition(pCD->m_iCreepingDark);
		SelectPos.k[2] += 30.0f;
		m_LastDarklingSpawnCheck = pCD->m_GameTick;
		
		// Update spawn points
		CheckDarklingSpawnPoints(SelectPos, 500.0f, m_iBestDarklingSpawn);
		if(m_iBestDarklingSpawn != OldDarklingsSpawn)
			pCD->m_DarklingSpawnRenderChoices = 0;
	}


	// Check for double b1 click
	if (!(_ControlPress & CONTROLBITS_BUTTON1) && (_ControlLastPress & CONTROLBITS_BUTTON1))
		m_LastButton1Release = pCD->m_GameTick;


	aint bSpawned = 0;
	if (m_iBestDarklingSpawn)
	{
		const uint SpawnButtonMask = (CONTROLBITS_JUMP | CONTROLBITS_BUTTON3 | CONTROLBITS_BUTTON0 | CONTROLBITS_BUTTON1);
		if ((_ControlPress & CONTROLBITS_BUTTON5) && (_ControlPress & SpawnButtonMask) && !(_ControlLastPress & SpawnButtonMask))
		{
			if (!pCD->m_DarklingSpawnRenderChoices)
			{
				CWObject_Message Msg(OBJMSG_CANUSE, m_iObject);
				int Ret = m_pWServer->Message_SendToObject(Msg, m_iBestDarklingSpawn);
				if(Ret)
				{
					pCD->m_DarklingSpawnRenderChoices = Ret;
					pCD->m_DarklingSpawnRenderChoicesStartTick = m_pWServer->GetGameTick();
				}
				else
				{
					pCD->m_DarklingSpawnRenderChoicesStartTick = 0;
				}
			}

			// Spawn the darkling?
			uint ChoicesMask = pCD->m_DarklingSpawnRenderChoices;
			if (ChoicesMask)
			{
				CWObject_Message Msg(OBJMSG_USE, m_iObject);

				switch (_ControlPress & SpawnButtonMask)
				{
				case CONTROLBITS_JUMP:		Msg.m_Param0 = CWObject_DarklingSpawn::DARKLINGSPAWN_FLAGS2_BERSERKER; break;
				case CONTROLBITS_BUTTON3:	Msg.m_Param0 = CWObject_DarklingSpawn::DARKLINGSPAWN_FLAGS2_KAMIKAZE; break;
				case CONTROLBITS_BUTTON0:	Msg.m_Param0 = CWObject_DarklingSpawn::DARKLINGSPAWN_FLAGS2_GUNNER; break;
				case CONTROLBITS_BUTTON1:	Msg.m_Param0 = CWObject_DarklingSpawn::DARKLINGSPAWN_FLAGS2_LIGHTKILLER; break;
				}

				if (Msg.m_Param0)
					bSpawned = m_pWServer->Message_SendToObject(Msg, m_iBestDarklingSpawn);
			}
		}
		else
			pCD->m_DarklingSpawnRenderChoicesStartTick = 0;
	}
	else 
		pCD->m_DarklingSpawnRenderChoicesStartTick = 0;

	if (!bSpawned && (_ControlPress & CONTROLBITS_BUTTON1) && !(_ControlLastPress & CONTROLBITS_BUTTON1))
	{
		bool bCanJump = true;
		int32 TimeSinceLastRelease = pCD->m_GameTick - m_LastButton1Release;
		CMat4Dfp32 SpawnEffectMat;
		if (TimeSinceLastRelease < (int32)m_pWServer->GetGameTicksPerSecond() * 0.5f)
		{	// Wee, got double click, tell darkling to go to player (fp32max tells the ai that)
			SpawnEffectMat.GetRow(3) = CVec3Dfp32(_FP32_MAX,_FP32_MAX,_FP32_MAX);
		}
		else
		{
			// Redirect spawned darklings
			const CMat4Dfp32& Mat = GetPositionMatrix();
			const CVec3Dfp32& Dir = Mat.GetRow(0);
			CVec3Dfp32 StartPos = Mat.GetRow(3) + CVec3Dfp32(0,0,54);
			CVec3Dfp32 SpawnPos = StartPos + Dir * 512.0f; // Max range: 16 meters
			SpawnEffectMat.GetRow(0) = Mat.GetRow(1);
			
			CCollisionInfo CInfo;
			bool bHit = m_pWServer->Phys_IntersectLine(
				StartPos, SpawnPos, OBJECT_FLAGS_CHARACTER, 
				OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL, 
				XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &CInfo, m_iObject);


			if (bHit && CInfo.m_bIsValid)
			{	// Check for Object
				CWObject* pObj = m_pWServer->Object_Get(CInfo.m_iObject);
				if ((pObj)&&(MRTC_ISKINDOF(pObj,CWObject_SwingDoor)))
				{	// Should we check for locked? (Maybe but if we don't then pathfinder will solve it for us)
					if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_SWINGDOOR_CANDARKLINGOPEN), CInfo.m_iObject))
					{
						bCanJump = false;
						StartPos = CInfo.m_Pos + Dir * 32.0f;
						bHit = m_pWServer->Phys_IntersectLine(
							StartPos, SpawnPos, OBJECT_FLAGS_CHARACTER, 
							OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL, 
							XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &CInfo, m_iObject);
						if ((!bHit)||(!CInfo.m_bIsValid)||(!(pObj = m_pWServer->Object_Get(CInfo.m_iObject))))
						{
							pObj = NULL;
						}
					}
					else
						pObj = NULL;
				}

				if (pObj)
				{
					uint16 ObjFlags = pObj->GetPhysState().m_ObjectFlags;
					if (!(ObjFlags & OBJECT_FLAGS_OBJECT))
					{	// OK; it's not an object. If it's a door then we shall continue unless

						SpawnEffectMat.GetRow(2) = CInfo.m_Plane.n;
						SpawnEffectMat.RecreateMatrix(2, 0);
						SpawnEffectMat.GetRow(3) = CInfo.m_Pos;
					}
					else
					{	// We tell the Darkling ai to give the player the finger with this
						SpawnEffectMat.GetRow(2) = CVec3Dfp32(_FP32_MAX,_FP32_MAX,_FP32_MAX);
					}
				}
				else
				{	// We tell the Darkling ai to give the player the finger with this
					SpawnEffectMat.GetRow(2) = CVec3Dfp32(_FP32_MAX,_FP32_MAX,_FP32_MAX);
				}
			}
			else
			{
				SpawnEffectMat.GetRow(2) = -Dir;
				SpawnEffectMat.RecreateMatrix(2, 0);
				SpawnEffectMat.GetRow(3) = SpawnPos;
			}
		}

		// Find all darklings
		const int16* pSel = NULL;
		TSelection<CSelection::LARGE_BUFFER> Selection;
		// Select only pickups for now...
		m_pWServer->Selection_AddClass(Selection,CStr("CharDarkling"));
		int32 nSel = m_pWServer->Selection_Get(Selection, &pSel);
		CWObject_Message Msg(OBJMSG_AIEFFECT_RETARGET);
		Msg.m_iSender = m_iObject;
		Msg.m_Param0 = bCanJump ? 1 : 0;
		Msg.m_VecParam0 = SpawnEffectMat.GetRow(3);
		Msg.m_VecParam1 = SpawnEffectMat.GetRow(2);
		for (int i = 0; i < nSel; i++)
		{
			int32 iObj =  pSel[i];
			CWObject* pObj = m_pWServer->Object_Get(iObj);
			if (pObj)
			{
				pObj->OnMessage(Msg);
			}
		}
	}

	// Activate/Deactivate selected Darkness power
	if (!(pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DRAIN))
	{
		bool bDarknessPower_Click = (pCD->m_Control_Click & CONTROLBITS_BUTTON2) != 0;
		bool bDarknessPower_Press = (_ControlPress & CONTROLBITS_BUTTON2) && !(_ControlLastPress & CONTROLBITS_BUTTON2) != 0;
		uint CurrPower = pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_ACTIVATEPOWERMASK;
		if (!bDarknessPowersDisabled)
		{
			if ((CurrPower && bDarknessPower_Click) || (!CurrPower && bDarknessPower_Press))
			{
				bForceHud = true; // Show health hud

				// Check if we can activate a power
				if (CurrPower)
				{
					// Deactivate current power
					if (CurrPower & PLAYER_DARKNESSMODE_POWER_BLACKHOLE)
						Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_BLACKHOLE, true, false);

					if (CurrPower & PLAYER_DARKNESSMODE_POWER_DEMONARM)
						Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DEMONARM, true, false);

					if (CurrPower & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK)
						m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CREEPINGDARK_BACKTRACK), pCD->m_iCreepingDark);

					if (CurrPower & PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS)
						Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS, true, false);
				}
				else
				{
					pCD->ResetButtonPressTicks(CONTROLBITS_BUTTON2); // TODO: shouldn't be needed..?

					// If not already in Darkness mode, enter Darkness mode and ignore activation (player has to press button again)
					if (!bIsInDarknessMode && bCanEnterDarknessMode)
					{
						Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD, true, true);
						Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSVISION, true, true);
						bIsInDarknessMode = true;
						bForceHud = true;
					}
					//else --("2K x06 revert": auto-enable but don't require an extra button press to activate power)
					{
						// Activate currently selected darkness power
						int8 SelectedPower = pCD->m_DarknessSelectedPower;
						bool bActivated = (SelectedPower < 0) ? false : Char_ActivateDarknessPower(ResolveDarknessSelection(SelectedPower), bDarkEnough, true);
						DARKNESSDEBUG(CStrF("Activating: %d %s", (SelectedPower < 0) ? "None" : ResolveDarknessSelection(SelectedPower),bActivated ? "Ok" : "Error"));
						pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode | PLAYER_DARKNESSMODE_STICKSELECTED | PLAYER_DARKNESSMODE_STICKMOVED;
						if (bActivated)
							pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode | PLAYER_DARKNESSMODE_ACTIVATED;
					}
				}
			}
		}
	}

/*
	// If we have full health, get the snakes out
	if (!bDarknessPowersDisabled && (Char()->Health() == Char()->MaxHealth()))
	{
		// Activate "Darkness Vision" if available
		if ((pCD->m_DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_DARKNESSVISION) && 
			!(pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSVISION))
		{
			pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode | PLAYER_DARKNESSMODE_POWER_DARKNESSVISION;
			bForceHud = true;
		}

		// Activate "Darkness Shield" if available
		if ((pCD->m_DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD) && 
		   !(pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD))
		{
			pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode | PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD;
			bForceHud = true;

			CWObject* pTentacleObj = m_pWServer->Object_Get(m_iDarkness_Tentacles);
			if (pTentacleObj)
			{
				CWObject_TentacleSystem& TentacleSystem = *safe_cast<CWObject_TentacleSystem>(pTentacleObj);
				if (!TentacleSystem.IsDevouring())
					TentacleSystem.ActivateDemonHeads(true);
			}

			const char* pSoundName = "Gam_drk_drk01";
			int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(pSoundName);
			if (iSound > 0)
				m_pWServer->Sound_At(GetPosition(), iSound, WCLIENT_ATTENUATION_3D);
		}
	}
	else if (bDarknessPowersDisabled || (Char()->Health() <= PLAYER_DARKNESSMODE_DARKNESSVISIONSHIELDLIMIT))
	{
		// Turn off "Darkness Vision"
		if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSVISION)
		{
			pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode & ~PLAYER_DARKNESSMODE_POWER_DARKNESSVISION;
		}

		// Turn off "Darkness Shield"
		if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD)
		{
			pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode & ~PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD;

			const char* pSoundName = "Gam_drk_drk02";
			int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(pSoundName);
			if (iSound > 0)
				m_pWServer->Sound_At(GetPosition(), iSound, WCLIENT_ATTENUATION_3D);

			//bForceHud = true;
			CWObject* pTentacleObj = m_pWServer->Object_Get(m_iDarkness_Tentacles);
			if (pTentacleObj)
			{
				CWObject_TentacleSystem& TentacleSystem = *safe_cast<CWObject_TentacleSystem>(pTentacleObj);
				if (!TentacleSystem.IsDevouring() && !TentacleSystem.IsCreepingDark())
					TentacleSystem.ActivateDemonHeads(false);
			}	
		}
	}
*/

	// Show amount of darkness left
	if (bForceHud || (PrevDarkness != pCD->m_Darkness) || (PrevHealth != pCD->m_Health))
		pCD->m_HealthHudStart = pCD->m_GameTick;
}


void CWObject_Character::Char_TurnOffDarknessPowers()
{
	CWO_Character_ClientData* pCD = GetClientData(this);
	if (!pCD)
		return;

	// Deactivate darkness shield
	if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD)
		Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD, true, false);

	// Deactivate darknessvision
	if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSVISION)
		Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSVISION, true, false);

	// Deactivate demonarm
	if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DEMONARM)
		Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DEMONARM, true, false);

	// Backtrack creeping dark power
	if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK)
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CREEPINGDARK_BACKTRACK),pCD->m_iCreepingDark);

	// Deactivate blackhole
	if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_BLACKHOLE)
		Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_BLACKHOLE, true, false);

	// Deactivate ancient weapons
	if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS)
		Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS, true, false);

	// Clear ai darkness
	CAI_Core* pAI = AI_GetAI();
	if (pAI)
		pAI->SetPlayerDarkness(0);

	CWObject* pTentacles = m_pWServer->Object_Get(pCD->m_iDarkness_Tentacles);
	if (pTentacles)
	{
		CWObject_TentacleSystem& TentacleSystem = *safe_cast<CWObject_TentacleSystem>(pTentacles);
		TentacleSystem.ActivateDemonHeads(false);
	}

	// Kill any extra stuff in selection mode (like drain and devour etc.)
	pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode & ~PLAYER_DARKNESSMODE_POWER_STATE_MASK ;
}


bool CWObject_Character::Char_ActivateDarknessPower(int32 _Power, bool _bDarkEnough, bool _bActivate)
{
	CWO_Character_ClientData* pCD = GetClientData(this);

	// Check if player is able to activate selected power
	if (_bActivate && ((pCD->m_Disable & PLAYER_DISABLE_DARKNESSPOWERS) || !(_Power & pCD->m_DarknessPowers)))
	{
		Char_DarknessFlashBar( CPixel32(255,255,255,255) );
		return false;
	}

	CWObject* pTentacleObj = m_pWServer->Object_Get(pCD->m_iDarkness_Tentacles);

	int DarknessDrain = 0;
	bool bActivated = false;
	bool bActive = (pCD->m_DarknessSelectionMode & _Power) != 0;
	switch (_Power)
	{
	case PLAYER_DARKNESSMODE_POWER_CREEPINGDARK:
		{
			// Don't activate if in air
			if (pCD->m_Phys_bInAir && pCD->m_Phys_nInAirFrames > 2)	//Make sure we have been in the air for at least 3 frames
				break;
			if(!_bActivate || (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK))
			{
				if (pTentacleObj)
				{
					CWObject_TentacleSystem& TentacleSystem = *safe_cast<CWObject_TentacleSystem>(pTentacleObj);
					TentacleSystem.StopCreepingDark();

					// make sure snakes are out
					if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD)
						TentacleSystem.ActivateDemonHeads(true);
				}

				// If active turn power off (or start backtrack??)
				// Ok, reached end of line for creepingdark, turn it off
				pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode & ~(PLAYER_DARKNESSMODE_POWER_CREEPINGDARK | PLAYER_DARKNESSMODE_CREEPINGCAM);
				// Remove immobile flag
				pCD->m_Phys_Flags = pCD->m_Phys_Flags & ~PLAYER_PHYSFLAGS_IMMOBILE;
				// Destroy object
				m_pWServer->Object_Destroy(pCD->m_iCreepingDark);
				pCD->m_iCreepingDark = 0;

				// Turn off creeping dark effect
				CWObject_Message MsgDrain(OBJMSG_EFFECTSYSTEM_SETFADE, FXFADE_OUT);
				m_pWServer->Message_SendToObject(MsgDrain, m_iDarkness_CreepingDark);

				// Unlock from gameplay state
				CWAG2I_Context Context(this,m_pWServer,pCD->m_GameTime);
				pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&Context,CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,0),0);
			}
			else if (pCD->m_Darkness != 0 && (pCD->m_DarknessPowers & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK))
			{
				// Activate creeping dark
				// Get real camera position here (temp!)
				CMat4Dfp32 Pos = GetPositionMatrix();
				// "Flatten" the look
				CVec3Dfp32::GetMatrixRow(Pos,0).k[2] = 0.0f;
				Pos.RecreateMatrix(0,2);
				Camera_Get_FirstPerson(m_pWServer,&Pos,this,0.0f);
				// Find position to start
				//Camera_Get(m_pWServer, &Pos, this, 0.0f);
				Pos.RotY_x_M(-0.25f);
				Pos.RotX_x_M(0.25f);

				if(Char_GetPhysType(this) == PLAYER_PHYS_CROUCH)
					Pos.k[3][2] -= 25.0f;

				if (pTentacleObj)
				{
					CWObject_TentacleSystem& TentacleSystem = *safe_cast<CWObject_TentacleSystem>(pTentacleObj);
					if (!TentacleSystem.StartCreepingDark(Pos.GetRow(3)))
					{
						int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("Gam_drk_crp_fail");
						if(iSound > 0)
							m_pWServer->Sound_On(m_iObject, iSound, WCLIENT_ATTENUATION_3D);
						break;
					}
				}

				int iObject = m_pWServer->Object_Create("CreepingDark",Pos);
				if (iObject == -1)
				{
					// Failed to create creeping dark, don't activate
					break;
				}

				pCD->m_iCreepingDark = iObject;
				pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode | PLAYER_DARKNESSMODE_POWER_CREEPINGDARK | PLAYER_DARKNESSMODE_CREEPINGCAM;
				m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CREEPINGDARK_SETSTARTINGCHARACTER,m_iObject),iObject);
				//ConOut(CStrF("StartPos: %s",m_pWServer->Object_GetPositionMatrix(iObject).GetString().GetStr()));
				// Set camera parameters
				pCD->m_Creep_Control_Look_Wanted.m_Value.k[0] = 0.0f;//pCD->m_Control_Look_Wanted.k[1];
				pCD->m_Creep_Control_Look_Wanted.m_Value.k[1] = 0.0f;//pCD->m_Control_Look_Wanted.k[2];
				pCD->m_Creep_Control_Look_Wanted.MakeDirty();
				pCD->m_Creep_Orientation.m_Value.Create(Pos);
				pCD->m_Creep_PrevOrientation.m_Value = pCD->m_Creep_Orientation.m_Value;

				// Lock player so he can't move
				pCD->m_Phys_Flags = pCD->m_Phys_Flags | PLAYER_PHYSFLAGS_IMMOBILE;
				bActivated = true;

				// Set start parameters??
				//CWObject_CreepingDark::SetStartParameters();

				// Send creepingdark anim thing to player
				CWAG2I_Context Context(this,m_pWServer,CMTime::CreateFromTicks(pCD->m_GameTick,m_pWServer->GetGameTickTime()));
				pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&Context,CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,AG2_IMPULSEVALUE_CREEPINGDARK_USE),0);

				pCD->m_DarknessPowers = pCD->m_DarknessPowers & ~PLAYER_DARKNESSMODE_POWER_CREEPINGDARK;
				
				// Turn on creeping dark effect
				CWObject_Message MsgDrain(OBJMSG_EFFECTSYSTEM_SETFADE, FXFADE_IN);
				m_pWServer->Message_SendToObject(MsgDrain, m_iDarkness_CreepingDark);
			}
			break;
		}
	case PLAYER_DARKNESSMODE_POWER_BLACKHOLE:
		{
	/*		if (!_bActivate || (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_LEFT) || 
				(pCD->m_Darkness < DARKNESS_POWER_ACTIVATIONCOST_LEFT))
			{
				// If active turn power off
				if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_LEFT)
				{
					pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode & ~PLAYER_DARKNESSMODE_POWER_LEFT;
				}
			}
			else if (_bDarkEnough)
			{
				ConOut("Activating right darkness");
				pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode | PLAYER_DARKNESSMODE_POWER_LEFT;
				pCD->m_Darkness = pCD->m_Darkness - DARKNESS_POWER_ACTIVATIONCOST_LEFT;
				bActivated = true;
			}*/
			/*if(pCD->m_Darkness != 0 && pCD->m_DarknessPowers & PLAYER_DARKNESSMODE_POWER_BLACKHOLE)
			{
				pCD->m_DarknessPowers = pCD->m_DarknessPowers & ~PLAYER_DARKNESSMODE_POWER_BLACKHOLE;
				//pCD->m_Darkness = pCD->m_MaxDarkness / 2;
				// Darknesscost is half of max darkness
				DarknessDrain = pCD->m_MaxDarkness / PLAYER_DARKNESSMODE_DRAINCOST_BLACKHOLE;
			}
	*/
			CWObject_BlackHole* pBlackHole = safe_cast<CWObject_BlackHole>( m_pWServer->Object_Get(m_iDarkness_BlackHole) );
			if (!pBlackHole)
			{
				ConOut("ERROR: Player has no blackhole system (fix the template)");
				return false;
			}

			if (_bActivate)
			{
				//int AtivationCost = TruncToInt(pBlackHole->GetActivationCost());
				// Make sure a hole isn't active and that we are fully charged
				if (pBlackHole->IsActive() || pCD->m_Darkness.m_Value != pCD->m_MaxDarkness.m_Value)
					break;

				ConOutL("Activating black hole");
				pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode | PLAYER_DARKNESSMODE_POWER_BLACKHOLE;
				pCD->m_DarknessPowers = pCD->m_DarknessPowers & ~PLAYER_DARKNESSMODE_POWER_BLACKHOLE;
				Char_ActivateBlackHole();
				bActivated = true;
			}
			else
			{
				ConOutL("Deactivating black hole");
				pBlackHole->SetExplode(true);
				pBlackHole->Deactivate();

				// Drain what's left of the juice
				pCD->m_Darkness = 0;
			}

			break;
		}
	case PLAYER_DARKNESSMODE_POWER_DEMONARM:
		{
			// Tentacle demon arm (testing testing!)
			if (!pTentacleObj)
			{
				ConOut("ERROR: Player has no tentacle system (fix the template)");
				return false;
			}
			CWObject_TentacleSystem& TentacleSystem = *safe_cast<CWObject_TentacleSystem>(pTentacleObj);

			if (!_bActivate || (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DEMONARM))
			{
				// Deactivate demon arm
				TentacleSystem.ReleaseObject(&pCD->m_Control_Move);
				pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode & ~PLAYER_DARKNESSMODE_POWER_DEMONARM;
				pCD->m_LookSpeedDamper = 0;
			}
			else if (_bActivate && (pCD->m_Darkness >= PLAYER_DARKNESS_DRAINCOST_ACTIVATE_DEMONARM) && (pCD->m_DarknessPowers & PLAYER_DARKNESSMODE_POWER_DEMONARM))
			{
				uint SelectType = TENTACLE_SELECTION_ALL;
				CWObject* pTarget = TentacleSystem.SelectTarget(&SelectType);
				if (pTarget)
				{
					DarknessDrain = PLAYER_DARKNESS_DRAINCOST_ACTIVATE_DEMONARM;

					// Find amount of darkness to take...
					bool bHoldingObject = true;
					CWObject_Character* pChar = TDynamicCast<CWObject_Character>(pTarget);
					if (pChar)
					{
						ConOut("Activating demon arm (char)");
//						int nMaxDamage = pCD->m_Darkness / 4;			// temp - for e3 demo 2006
//						int nMaxDamage = pCD->m_MaxDarkness / 2;		// temp - for e3 demo 2006
						int nMaxDamage = 255;
						int nHealth = pChar->Char()->Health();
						if (SelectType == TENTACLE_SELECTION_CHARACTERITEM)
						{
							TentacleSystem.GrabCharacterObject(*pChar, 30.0f);
						}
						/*else if (nMaxDamage < nHealth)
						{ // not enough power to kill enemy. just send damage
							TentacleSystem.BreakObject(*pTarget, nMaxDamage);
						}*/
						else
						{ // enough power to kill, let's grab! (will kill char)
							TentacleSystem.GrabObject(*pChar, 80.0f, nMaxDamage);
						}

						pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode | PLAYER_DARKNESSMODE_POWER_DEMONARM;
					}
					else if (((SelectType == TENTACLE_SELECTION_OBJECT) || SelectType == TENTACLE_SELECTION_SWINGDOOR) && (pTarget->GetMass() >= 10.0f))
					{
						// Calculate how much we're able to lift
						fp32 x = fp32(pCD->m_MaxDarkness) * (1.0f / 255.0f);
						x = Clamp((x - 0.75f) * 4.0f, 0.0f, 1.0f);
						fp32 GrabPower = 100.0f + Sqr(x) * 3400.0f;		// 100 kg at 75% maxdarkness, 3500 kg at 100% maxdarkness
						
/*
				M_TRACE("Mass = %.1f, GrabPower = %.1f\n", pTarget->GetMass(), GrabPower);
						DarknessDrain = int(pCD->m_MaxDarkness * (0.33f * M_Sqrt(pTarget->GetMass() / GrabPower)));
				M_TRACE("DarknessDrain = %d, m_Darkness = %d\n", DarknessDrain, (int)pCD->m_Darkness);
						if (DarknessDrain > pCD->m_Darkness)
						{
							GrabPower *= fp32(pCD->m_Darkness) / fp32(DarknessDrain);
				M_TRACE(" - reduced grabpower: %.1f\n", GrabPower);
							DarknessDrain = pCD->m_Darkness;
						}
*/
						// E3 demo 2006 - max power!
						GrabPower = 3000.0f;

						uint DemonArmLevel = pCD->m_DarknessLevel.GetPowerLevel(PLAYER_DARKNESSMODE_POWER_DEMONARM);
						fp32 TargetMass = pTarget->GetMass();
						if (!DemonArmLevel && (TargetMass > 750.0f))
						{
							ConOut(CStrF("Activating demon arm (push object) Force: %.2f, Mass: %.2f", TargetMass * 0.21f, TargetMass));
							TentacleSystem.PushObject(*pTarget, TargetMass * 0.21f);
						}
						else
						{
							// Slow down look speed by 50..90%   (1000 kg -> 75% speed decrease)
							fp32 Damp = Clamp(0.5f + TargetMass * (0.25f / 1000.0f), 0.0f, 0.9f);
							pCD->m_LookSpeedDamper = RoundToInt(Damp * 255);

							ConOut(CStrF("Activating demon arm (grab object), Mass: %.2f, Look speed damp: %d%%", TargetMass, int(Damp * 100)));
							TentacleSystem.GrabObject(*pTarget, GrabPower);
						}

						pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode | PLAYER_DARKNESSMODE_POWER_DEMONARM;
					}
					else
					{
						ConOut("Activating demon arm (break object)");
						int nMaxDamage = pCD->m_Darkness / 4;
						TentacleSystem.BreakObject(*pTarget, nMaxDamage);
					}
					
					pCD->m_DarknessPowers = pCD->m_DarknessPowers & ~PLAYER_DARKNESSMODE_POWER_DEMONARM;
					bActivated = true;
				}
				else if (TentacleSystem.IsIdle())
				{
					// Couldn't find anything..  Just send the arm out and bring it back
					const CMat4Dfp32& Mat = GetPositionMatrix();
					const CVec3Dfp32& Dir = CVec3Dfp32::GetRow(Mat, 0);
					CVec3Dfp32 StartPos = CVec3Dfp32::GetRow(Mat, 3) + CVec3Dfp32(0,0,54);

					CVec3Dfp32 EndPos = StartPos + Dir * 128.0f;
					CCollisionInfo CInfo;
					bool bHit = m_pWServer->Phys_IntersectLine(
						StartPos, EndPos, OBJECT_FLAGS_PROJECTILE, 
						OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL, 
						XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS, &CInfo, m_iObject);

					if (bHit && CInfo.m_bIsValid)
					{
						EndPos = CInfo.m_Pos;
					}

					TentacleSystem.GetNothing(EndPos);
					bActivated = true;

					// Darkness drain for just shooting out 1 / 8th? (if we got here we should always have enough darkness)
					//DarknessDrain = pCD->m_MaxDarkness / PLAYER_DARKNESSMODE_DRAINCOST_DEMONARM;

					// Since this doesn't set the darkness selection, we must tell the AI manually
					// TODO: perhaps this should set darkness selection
					CAI_Core* pAI = AI_GetAI();
					if (pAI)
					{
						pAI->SetPlayerDarkness(pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_AIDARKNESSMASK);
						pAI->SetDarknessPos(EndPos);
					}
				}
			}
			break;
		}
	
	case PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS:
		{
			// Shield Changed to ancient weapons...
			CWAG2I_Context AGContext(this,m_pWServer,pCD->m_GameTime);
			if(!_bActivate || (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS))
			{
				// If active turn power off
				if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS)
				{
					pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode & ~PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS;
					// Select normal gun again instead of the ancient weapon
					CRPG_Object_Item* pItem = Char()->GetFinalSelectedItem(0);
					if (!pItem || (pItem && pItem->m_iItemType != RPG_ITEMTYPE_GUN))
					{
						CRPG_Object_Item* pGun = Char()->FindItemByType(RPG_ITEMTYPE_GUN);
						if (!pGun)
							pGun = Char()->FindItemByType(RPG_ITEMTYPE_FIST);
						if (pGun)
						{
							int ItemIndex = Char()->GetInventory(0)->FindItemIndexByIdentifier(pGun->m_Identifier);
							pCD->m_iCurSelectedItem = ItemIndex;
							//Switch weapon in animgraph
							pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER,pGun->m_Identifier);
							if (pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_UNEQUIP),0))
								pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED,true);
							m_WeaponUnequipTimer = 0;
						}
					}
				}
			}
			else if (pCD->m_Darkness > 0 && (pCD->m_DarknessPowers & PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS))
			{
				DARKNESSDEBUG(CStr("Activating Ancient Weapons"));

				// Switch gun to "ancient"
				CRPG_Object_Item* pItem = Char()->GetFinalSelectedItem(0);
				if (!pItem || (pItem && pItem->m_iItemType != RPG_ITEMTYPE_ANCIENTWEAPONS))
				{
					CRPG_Object_Item* pAncient = Char()->FindItemByType(RPG_ITEMTYPE_ANCIENTWEAPONS);
					if (pAncient && pAncient->m_Flags2 & RPG_ITEM_FLAGS2_FORCEEQUIPLEFT)
					{
						// Find another one
						pAncient = Char()->FindNextItemByType(pAncient->m_Identifier, pAncient->m_iItemType,true);
					}
					if (pAncient)
					{
						int ItemIndex = Char()->GetInventory(0)->FindItemIndexByIdentifier(pAncient->m_Identifier);
						pCD->m_iCurSelectedItem = ItemIndex;
						//Switch weapon in animgraph
						//Switch weapon in animgraph
						pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER,pAncient->m_Identifier);
						if (pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_UNEQUIP),0))
							pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED,true);
						m_WeaponUnequipTimer = 0;
						pCD->m_DarknessPowers = pCD->m_DarknessPowers & ~PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS;
						pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode | PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS;
						bActivated = true;
						DarknessDrain = 0;
					}
				}
			}
			break;
		}

	case PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD:
		{
			CWObject* pTentacleObj = m_pWServer->Object_Get(pCD->m_iDarkness_Tentacles);
			CWObject_TentacleSystem* pTentacleSystem = pTentacleObj ? safe_cast<CWObject_TentacleSystem>(pTentacleObj) : NULL;

			if (_bActivate)
			{
				if (!(pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD))
				{
					ConOutL("Activating Darkness shield");
					pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode | PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD;
					bActivated = true;

					const char* pSoundName = "Gam_drk_drk01";
					int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(pSoundName);
					if (iSound > 0)
						m_pWServer->Sound_At(GetPosition(), iSound, WCLIENT_ATTENUATION_2D);
				}

				// Get the snakes out
				if (pTentacleSystem && !pTentacleSystem->IsDevouring())
					pTentacleSystem->ActivateDemonHeads(true);
			}
			else
			{
				if ((pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DRAIN) == 0)
				{
					ConOutL("Deactivating Darkness shield");
					pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode & ~PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD;

					const char* pSoundName = "Gam_drk_drk02";
					int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(pSoundName);
					if (iSound > 0)
						m_pWServer->Sound_At(GetPosition(), iSound, WCLIENT_ATTENUATION_2D);

					// Put the snakes back in
					if (pTentacleSystem && !pTentacleSystem->IsDevouring() && !pTentacleSystem->IsCreepingDark())
						pTentacleSystem->ActivateDemonHeads(false);
				}
			}
			break;
		}

	case PLAYER_DARKNESSMODE_POWER_DARKNESSVISION:
		{
			if (_bActivate)
			{
				pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode | PLAYER_DARKNESSMODE_POWER_DARKNESSVISION;
				bActivated = true;
			}
			else
				pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode & ~PLAYER_DARKNESSMODE_POWER_DARKNESSVISION;
		}
		break;


	default:
		return false;
	}

	if (_bActivate && !bActivated)
	{
		// Not enough power
		int32 Color = !_bDarkEnough ? CPixel32(255,30,30,0) : CPixel32(255,255,255,255);
		Char_DarknessFlashBar(Color);

		// Play fail sound
		if (!bActive)
		{
			int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("Gam_drk_fail");
			if (iSound != 0)
				m_pWServer->Sound_On(m_iObject, iSound, WCLIENT_ATTENUATION_2D);
		}
	}

	if(bActivated)
		pCD->m_Darkness = Max(int(pCD->m_Darkness) - DarknessDrain,0);

	return bActivated;
}


void CWObject_Character::Char_ActivateBlackHole()
{
	CWO_Character_ClientData* pCD = GetClientData(this);

	CWObject_BlackHole* pBlackHole = (m_iDarkness_BlackHole) ? (CWObject_BlackHole*)m_pWServer->Object_Get(m_iDarkness_BlackHole) : NULL;
	if(pBlackHole)
	{
		const CMat4Dfp32& Mat = GetPositionMatrix();
		const CVec3Dfp32& Dir = Mat.GetRow(0);
		CVec3Dfp32 StartPos = Mat.GetRow(3) + CVec3Dfp32(0,0,54);
		CVec3Dfp32 EndPos = StartPos + Dir * 512.0f; // Max range: 16 meters
		CCollisionInfo CInfo;

		bool bHit = m_pWServer->Phys_IntersectLine(StartPos, EndPos, 0,
			OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &CInfo, m_iObject);

		CVec3Dfp32 SpawnPos;
		if(bHit && CInfo.m_bIsValid)
		{
			// Can be VERY close to player ... Or maybe not ?
			//fp32 Len = (CInfo.m_Pos - StartPos).Length();
			//if(Len < 16.0f)
			//	return;

			// Activate at collision point
			SpawnPos = CInfo.m_Pos + CInfo.m_Plane.n * 32.0f;
		}
		else
		{
			// Didn't hit anything, activate at max range
			SpawnPos = EndPos;
		}

		uint DarknessLevel = (pCD) ? pCD->m_DarknessLevel.GetLevel() : 0;
		pBlackHole->Activate(SpawnPos, DarknessLevel);
		if (m_spAI)
		{
			m_spAI->SetPlayerDarkness(pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_AIDARKNESSMASK);
			m_spAI->SetDarknessPos(SpawnPos);
		}
	}
}

bool CWObject_Character::Char_BeRagDoll(bool _bEnable)
{
	CWO_Character_ClientData* pCD = GetClientData(this);
	if (_bEnable)
	{
		if (m_Flags & PLAYER_FLAGS_RAGDOLL)
		{
#ifdef INCLUDE_OLD_RAGDOLL
			CConstraintSystem* pRagdoll = m_spRagdoll;
			if (!pRagdoll)
				pRagdoll = m_spRagdoll = MNew(CConstraintSystem);

			if (!pRagdoll->IsInited())
			{
				// To be sure we have a skeletoninstance when creating the ragdoll
				CXR_AnimState Anim;
				CXR_Skeleton* pSkel;
				CXR_SkeletonInstance* pSkelInstance;
				if (GetEvaluatedPhysSkeleton(pSkelInstance, pSkel, Anim))
				{
					CXR_Model* lpModels[CWO_NUMMODELINDICES];
					for (uint i = 0; i < CWO_NUMMODELINDICES; i++)
					{
						m_RagdollSettings.m_lpClothSkeleton[i] = NULL;
						if (m_iModel[i] > 0)
						{
							lpModels[i] = m_pWServer->GetMapData()->GetResource_Model(m_iModel[i]);
							if(lpModels[i])
								m_RagdollSettings.m_lpClothSkeleton[i] = lpModels[i]->GetSkeleton();
						}
					}

					m_RagdollSettings.m_pSkelInstance = pSkelInstance;
					m_RagdollSettings.m_pSkeleton = pSkel;
					pRagdoll->SetOrgMat(GetPositionMatrix());
					pRagdoll->Init(m_iObject, this, m_pWServer, pCD, &pCD->m_RagdollClient);
					pRagdoll->Setup(&m_RagdollSettings);
					for(uint i = 0; i < CWO_NUMMODELINDICES; i++)
						m_RagdollSettings.m_lpClothSkeleton[i] = NULL;
					pRagdoll->Activate(true);
					pRagdoll->AddImpulse(CConstraintSystem::BODY_ALL,pCD->m_AnimGraph2.m_PhysImpulse);
					pCD->m_AnimGraph2.m_PhysImpulse = 0.0f;
					m_RagdollSettings.m_pSkelInstance = NULL;
				}
			}
			pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_RAGDOLLACTIVE;
#endif // INCLUDE_OLD_RAGDOLL

			if( !m_pPhysCluster )
			{
				Char_ActivateRagdoll(pCD);
			}
			pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_RAGDOLLACTIVE;

			if (pCD->m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_EXITRAGDOLL)
			{
				// Add Phys flags again
				if (pCD->m_Phys_Flags & (PLAYER_PHYSFLAGS_NOGRAVITY | PLAYER_PHYSFLAGS_NOCHARACTERCOLL | PLAYER_PHYSFLAGS_NOWORLDCOLL))
				{
					ClientFlags() = m_ClientFlags & ~PLAYER_CLIENTFLAGS_NOGRAVITY;
					pCD->m_Phys_Flags = pCD->m_Phys_Flags & ~(PLAYER_PHYSFLAGS_NOGRAVITY | PLAYER_PHYSFLAGS_NOCHARACTERCOLL | PLAYER_PHYSFLAGS_NOWORLDCOLL);
					Char_SetPhysics(this,m_pWServer,m_pWServer,(Char_GetPhysType(this) == PLAYER_PHYS_CROUCH) ? PLAYER_PHYS_CROUCH : PLAYER_PHYS_STAND,false,true);
				}
			}
			// Drop item
			CRPG_Object_Item* pEquipped = Char()->GetFinalSelectedItem(RPG_CHAR_INVENTORY_WEAPONS);
			if (pEquipped && pEquipped->m_AnimType == AG2_IMPULSEVALUE_WEAPONTYPE_RIFLE)
			{
				// Drop current weapon and force select a gun
				CWObject_Message Msg(OBJMSG_CHAR_FORCEDROPWEAPON,0,0);
				OnCharForceDropWeapon(Msg,pCD);
				ForceSelectItem(1);
				if (m_spAI)
					m_spAI->m_Weapon.OnRefresh();
			}
		}
	}
	/*else
	{
		// Remove ragdoll if we have fully blended in...?
		CWO_Character_ClientData* pCD = GetClientData(this);
		if (pCD->m_RagdollClient.m_State != CConstraintSystemClient::NOTREADY)
		{
			m_spRagdoll->SetState(CConstraintSystemClient::NOTREADY);
			pCD->m_RagdollClient.MakeDirty();
			pCD->m_ExtraFlags = pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_RAGDOLLACTIVE;
			pCD->m_Phys_Flags = pCD->m_Phys_Flags & 
				~(PLAYER_PHYSFLAGS_NOGRAVITY | PLAYER_PHYSFLAGS_NOCHARACTERCOLL | PLAYER_PHYSFLAGS_NOWORLDCOLL);
			// Remove ragdoll
			m_spRagdoll = NULL;
		}	
	}*/
	return true;
}

// Check if we can do gunkata (client)
bool CWObject_Character::Char_CheckGunKata(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, int32 _Type, int32 _iTarget)
{
	int iClassCharNPC = _pWPhys->GetMapData()->GetResourceIndex_Class("CharNPC");	
	// Check so target is a character
	CWObject_CoreData* pTarget = _pWPhys->Object_GetCD(_iTarget);
	if (!pTarget || !(pTarget->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER))
		return false;

	CWO_Character_ClientData* pCDPlayer = GetClientData(_pObj);
	CWO_Character_ClientData* pCDVictim = GetClientData(pTarget);
	if (!pCDVictim || pTarget->m_iClass != iClassCharNPC || pCDVictim->m_iPlayer != -1 ||
		(pCDVictim->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMUNE) || (Char_GetPhysType(pTarget) == PLAYER_PHYS_DEAD))
		return false;

	// Don't do gunkata while equipping/unequipping
	if (((pCDVictim->m_AnimGraph2.GetStateFlagsLoToken2() | pCDPlayer->m_AnimGraph2.GetStateFlagsLoCombined()) & AG2_STATEFLAG_EQUIPPING) ||
		((pCDVictim->m_AnimGraph2.GetStateFlagsLoCombined() | pCDPlayer->m_AnimGraph2.GetStateFlagsLoCombined()) & AG2_STATEFLAG_HURTACTIVE))
		return false;

	// Check so that target is in "normal" explore mode
	spCWAG2I spAGI = pCDVictim->m_AnimGraph2.GetAG2I();
	if (!spAGI->GetNumTokens())
		return false;

	const CWAG2I_Token* pToken = spAGI->GetToken(0);
	CAG2AnimGraphID iAnimGraph = pToken ? pToken->GetAnimGraphIndex() : -1;
	CXRAG2* pAnimGraph = iAnimGraph != -1 ? pCDVictim->m_AnimGraph2.GetAG2I()->GetAnimGraph(iAnimGraph) : NULL;
	CAG2ReactionIndex iReaction = pAnimGraph ? pAnimGraph->GetMatchingReaction(pToken->GetGraphBlock(),CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,0)) : -1;
	if (iReaction == -1)
		return false;

	return true;
}

// Do some cool gunkata move
bool CWObject_Character::Char_DoGunKata(int32 _AnimType, int32 _iTarget)
{

	/*int32 Type;
	CVec3Dfp32 Pos1,Pos2;
	int32 NumObjs = m_spAI->GetInterestingObjects(Type, Pos1, Pos2);
	if (NumGunKataTypes == 0 || (Type == CAI_Core::INTERESTING_ENEMY && NumObjs > 1))
		return false;*/
	int iClassCharNPC = m_pWServer->GetMapData()->GetResourceIndex_Class("CharNPC");	
	CWObject_Character* pTarget = IsCharacter(_iTarget,m_pWServer);
	CWO_Character_ClientData* pCDPlayer = GetClientData(this);
	CWO_Character_ClientData* pCDVictim = pTarget ? GetClientData(pTarget) : NULL;
	if (!pCDVictim || pTarget->m_iClass != iClassCharNPC || pCDVictim->m_iPlayer != -1 ||
		(pCDVictim->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMUNE) || (Char_GetPhysType(pTarget) == PLAYER_PHYS_DEAD) ||
		(pTarget->m_Flags & PLAYER_FLAGS_NOGUNKATA) || pCDPlayer->m_iMountedObject != 0 || pCDVictim->m_iMountedObject != 0)
		return false;

	// Don't do gunkata while equipping/unequipping
	if (((pCDVictim->m_AnimGraph2.GetStateFlagsLoToken2() | pCDPlayer->m_AnimGraph2.GetStateFlagsLoCombined()) & AG2_STATEFLAG_EQUIPPING) ||
		((pCDVictim->m_AnimGraph2.GetStateFlagsLoCombined() | pCDPlayer->m_AnimGraph2.GetStateFlagsLoCombined()) & AG2_STATEFLAG_HURTACTIVE))
		return false;

	// Check so that target is in "normal" explore mode
	const CWAG2I_Token* pToken = pCDVictim->m_AnimGraph2.GetAG2I()->GetToken(0);
	CAG2AnimGraphID iAnimGraph = pToken ? pToken->GetAnimGraphIndex() : -1;
	CXRAG2* pAnimGraph = iAnimGraph != -1 ? pCDVictim->m_AnimGraph2.GetAG2I()->GetAnimGraph(iAnimGraph) : NULL;
	CAG2ReactionIndex iReaction = pAnimGraph ? pAnimGraph->GetMatchingReaction(pToken->GetGraphBlock(),CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,0)) : -1;
	if (iReaction == -1)
		return false;
	
	CWObject_Character* pCharTarget = (CWObject_Character*)pTarget;
	// Can't kill undead people (yet...)
	/*if (pCharTarget->m_spAI->m_CharacterClass == CAI_Core::CLASS_UNDEAD)
		return false;*/

	CRPG_Object_Item* pFinalItem = Char()->GetFinalSelectedItem(AG2_ITEMSLOT_WEAPONS);
	if (!pFinalItem)
		return false;

	// Check so we don't have anything inbetween us (quick test for now..)
	CVec3Dfp32 MidPoint,MidPointTarget;
	pTarget->GetAbsBoundBox()->GetCenter(MidPointTarget);
	GetAbsBoundBox()->GetCenter(MidPoint);
	if (m_pWServer->Phys_IntersectLine(MidPoint,MidPointTarget,OBJECT_FLAGS_PROJECTILE,OBJECT_FLAGS_PHYSOBJECT|OBJECT_FLAGS_PHYSMODEL|OBJECT_FLAGS_WORLD,XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS,pTarget->m_iObject,m_iObject))
		return false;

	CVec3Dfp32 FlatDir,FlatTargetDir;
	FlatDir = GetPositionMatrix().GetRow(0);
	FlatDir.k[2] = 0.0f;
	FlatDir.Normalize();
	FlatTargetDir = pTarget->GetPositionMatrix().GetRow(0);
	FlatTargetDir.k[2] = 0.0f;
	FlatTargetDir.Normalize();
	int8 LookDir = ((FlatDir * FlatTargetDir) < 0.0f) ? 1 : 0;

	TArray<CAG2ImpulseValue> lPossibleMoves;
	switch (_AnimType)
	{
	case AG2_IMPULSEVALUE_WEAPONTYPE_GUN:
		{
			TAP_RCD<GunKataMove> lGunKataMoves = m_lGunKataMoves;
			for (int32 i = 0; i < lGunKataMoves.Len(); i++)
			{
				bool bItemOk = lGunKataMoves[i].m_ItemType != 0 ? pFinalItem->m_iItemType == lGunKataMoves[i].m_ItemType : true;
				if (bItemOk && lGunKataMoves[i].m_AnimType == 3 && lGunKataMoves[i].m_Direction == LookDir)
				{
					lPossibleMoves.Add(lGunKataMoves[i].m_Impulse);
				}
			}

			break;
		}
	case AG2_IMPULSEVALUE_WEAPONTYPE_RIFLE:
		{
			TAP_RCD<GunKataMove> lGunKataMoves = m_lGunKataMoves;
			for (int32 i = 0; i < lGunKataMoves.Len(); i++)
			{
				bool bItemOk = lGunKataMoves[i].m_ItemType != 0 ? pFinalItem->m_iItemType == lGunKataMoves[i].m_ItemType : true;
				if (bItemOk && lGunKataMoves[i].m_AnimType == 6 && lGunKataMoves[i].m_Direction == LookDir)
					lPossibleMoves.Add(lGunKataMoves[i].m_Impulse);
			}
			
			break;
		}
	default:
		{
			return false;
		}
	};
	TAP_RCD<CAG2ImpulseValue> lPMoves = lPossibleMoves;
	// Check if we have any moves at all to use
	if (!lPossibleMoves.Len())
		return false;
	CAG2ImpulseValue GKMove = lPMoves[((int)(((fp32)lPMoves.Len()) * Random)) % lPMoves.Len()];
	// Set no death sound, they will be in the animation instead
	pCharTarget->m_Flags |= PLAYER_FLAGS_NODEATHSOUND;

	CMat4Dfp32 MatPlayer,MatVictim;
	CVec3Dfp32 PosPlayer = GetPosition();
	CVec3Dfp32 PosTarget = pTarget->GetPosition();
	CVec3Dfp32 Dir = PosTarget - PosPlayer;
	fp32 Length = Dir.Length();
	if (Length > 60.0f || (Abs(PosPlayer.k[2] - PosTarget.k[2]) > 6.0f))
		return false;

	SetGunplayUnsafe((int32)(m_pWServer->GetGameTick() + m_pWServer->GetGameTicksPerSecond() * 5.0f));
	// Ok then, place characters 40 units from eachother exactly in line (for now..)
	// ok then, to find the perfect positions, start the gun kata move and check for constants
	// Hardcoded type for now... (5050)...
	int32 Tick = Max(pCDPlayer->m_GameTick,pCDVictim->m_GameTick);
	CWAG2I_Context ContextPlayer(this,m_pWServer,CMTime::CreateFromTicks(Tick,m_pWServer->GetGameTickTime()));
	CWAG2I_Context ContextVictim(pTarget,m_pWServer,CMTime::CreateFromTicks(Tick,m_pWServer->GetGameTickTime()));
	CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,GKMove);
	pCDPlayer->m_AnimGraph2.GetAG2I()->SendImpulse(&ContextPlayer,Impulse,0);
	pCDVictim->m_AnimGraph2.GetAG2I()->SendImpulse(&ContextVictim,Impulse,0);
	// Set physics on victim
	if (pCDVictim->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOCOLLISION)
		Char_SetPhysics(pTarget,m_pWServer,m_pWServer,Char_GetPhysType(pTarget),false,true);
	// Raise our noiselevel for the Gunkata gun discharge
	if (m_spAI)
	{
		// if (pFinalItem) Not needed, checked further up
		CWObject_Message Msg(OBJMSG_CHAR_RAISENOISELEVEL);
		Msg.m_Param0 = TruncToInt(pFinalItem->m_NoiseLevel);	// Typical gun loudness
		OnMessage(Msg);
		Msg.m_Msg = OBJMSG_CHAR_RAISEVISIBILITY;
		Msg.m_Param0 = TruncToInt(pFinalItem->m_Visibility);	// Typical gun visibility
		OnMessage(Msg);
		m_spAI->m_KB.Global_ReportDeath(m_iObject,_iTarget,PosTarget,SDead::DEATHCAUSE_SHOT);
	}
	if (pTarget->m_spAI)
	{
		// Turn off ai facials
		pTarget->m_spAI->m_DeviceFacial.Pause(true);
		// Kill the facials
		pCDVictim->m_AnimGraph2.GetAG2I()->SendImpulse(&ContextVictim,CXRAG2_Impulse(AG2_IMPULSETYPE_FACIAL,AG2_IMPULSEVALUE_FACIAL_TERMINATE),AG2_TOKEN_FACIAL);
	}
	// Set fight char so we know who to sync anim time with
	pCDVictim->m_iFightingCharacter = m_iObject;
	pCDPlayer->m_iFightingCharacter = pTarget->m_iObject;
	// Get distance from state constant
	fp32 HalfDistanceX = pCDPlayer->m_AnimGraph2.GetOffsetX();
	fp32 HalfDistanceY = pCDPlayer->m_AnimGraph2.GetOffsetY();
	if (HalfDistanceX <= 0.0f)
		HalfDistanceX = 20.0f;

	//CVec3Dfp32 PosMid = PosPlayer + Dir * 0.5f;
	// Normalize
	Length = 1.0f / Length;
	Dir *= Length;

	MatPlayer.GetRow(0) = Dir;
	MatPlayer.GetRow(2) = CVec3Dfp32(0.0f,0.0f,1.0f);
	MatPlayer.RecreateMatrix(0,2);
	MatPlayer.GetRow(3) = PosPlayer;//PosMid - Dir * HalfDistanceX + MatPlayer.GetRow(1) * HalfDistanceY;

	MatVictim.GetRow(0) = LookDir ? -Dir : Dir;
	MatVictim.GetRow(2) = CVec3Dfp32(0.0f,0.0f,1.0f);
	MatVictim.RecreateMatrix(0,2);
	MatVictim.GetRow(3) = PosPlayer + Dir * HalfDistanceX * 2.0f + MatVictim.GetRow(1) * HalfDistanceY * 2.0f;
	
	// Set the startpositions to the animgraphs

	pCDPlayer->m_AnimGraph2.SetDestination(MatPlayer);
	// Borrow dialogueplayerpos to get position to client
	pCDPlayer->m_DialoguePlayerPos = MatPlayer;
	pCDVictim->m_AnimGraph2.SetDestinationLock(MatVictim);
	pCDVictim->m_DialoguePlayerPos = MatVictim;
	Char_SetControlMode(this,PLAYER_CONTROLMODE_ANIMATION);
	Char_SetControlMode(pTarget,PLAYER_CONTROLMODE_ANIMATION);

	// Ok, that should be it
	return true;
}


#ifdef NEVER // function isn't used anymore
void CWObject_Character::Char_SpawnDarkling(int _ControlPress, int _ControlLastPress)
{
	CWO_Character_ClientData* pCD = GetClientData(this);
	CWObject_EffectSystem* pSpawnEffect = (m_iDarkness_DarklingSpawn > 0) ? (CWObject_EffectSystem*)m_pWServer->Object_Get(m_iDarkness_DarklingSpawn) : NULL;

	// Check if any spawn-buttons are pressed
	const uint DPadMask = CONTROLBITS_DPAD_UP | CONTROLBITS_DPAD_DOWN | CONTROLBITS_DPAD_LEFT | CONTROLBITS_DPAD_RIGHT;
	if ((_ControlPress & DPadMask) == 0)
	{
		// Release spawn object for self destruction
		if(pSpawnEffect)
		{
			// Forward effect to start on fade out/dying time
			const fp32 TickPerSec = m_pWServer->GetGameTicksPerSecond();
			pSpawnEffect->SetAnimTick(m_pWServer, TruncToInt(TickPerSec + (((255 - pCD->m_DarklingSpawn.Get(pCD->m_GameTick, 0.0f)) * 0.00390625f) * TickPerSec)), 0);
			m_iDarkness_DarklingSpawn = 0;
		}

		// Reset "spawn level"
		if (pCD->m_DarklingSpawn.Set(0, pCD->m_GameTick-20, 0.0f))
			pCD->m_DarklingSpawn.MakeDirty();

		return;
	}

	// Check which Darkling to spawn
	int iMapping = -1;
	if (_ControlPress & CONTROLBITS_DPAD_UP)
		iMapping = PLAYER_DARKNESS_DARKLING_ARRAYMAP_UP;
	else if (_ControlPress & CONTROLBITS_DPAD_DOWN)
		iMapping = PLAYER_DARKNESS_DARKLING_ARRAYMAP_DOWN;
	else if (_ControlPress & CONTROLBITS_DPAD_LEFT)
		iMapping = PLAYER_DARKNESS_DARKLING_ARRAYMAP_LEFT;
	else if (_ControlPress & CONTROLBITS_DPAD_RIGHT)
		iMapping = PLAYER_DARKNESS_DARKLING_ARRAYMAP_RIGHT;
	M_ASSERT(iMapping >= 0, "!");

	int32 DarklingType = m_lDarklingMapping[iMapping];
	if (DarklingType == -1 || !m_lAvailableDarklings.ValidPos(DarklingType))
	{
		if (pCD->m_DarklingSpawn.Set(0, pCD->m_GameTick-20, 0.0f))
			pCD->m_DarklingSpawn.MakeDirty();
		if(pSpawnEffect)
			pSpawnEffect->SetAnimTick(m_pWServer, 0, 0);
		return;
	}

	// Determine spawn position
	const CMat4Dfp32& Mat = GetPositionMatrix();
	CMat4Dfp32 SpawnEffectMat;
	const CVec3Dfp32& Dir = Mat.GetRow(0);
	CVec3Dfp32 StartPos = Mat.GetRow(3) + CVec3Dfp32(0,0,54);
	CVec3Dfp32 SpawnPos = StartPos + Dir * 512.0f; // Max range: 16 meters
	SpawnEffectMat.GetRow(0) = Mat.GetRow(1);
	
	CCollisionInfo CInfo;
	
	bool bHit = m_pWServer->Phys_IntersectLine(
					StartPos, SpawnPos, OBJECT_FLAGS_CHARACTER, 
					OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL, 
					XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &CInfo, m_iObject);
	if (bHit && CInfo.m_bIsValid)
	{
		fp32 Len = (CInfo.m_Pos - StartPos).Length();
		if (Len < 16.0f)
		{
			if (pCD->m_DarklingSpawn.Set(0, pCD->m_GameTick, 0.0f))
				pCD->m_DarklingSpawn.MakeDirty();
			if(pSpawnEffect)
				pSpawnEffect->SetAnimTick(m_pWServer, 0, 0);
			return;
		}
		SpawnPos = StartPos + Dir * (Len - 1.0f);
		
		SpawnEffectMat.GetRow(2) = CInfo.m_Plane.n;
		SpawnEffectMat.RecreateMatrix(2, 0);
		SpawnEffectMat.GetRow(3) = CInfo.m_Pos;
		
		//WallMarkPos.UnitNot3x3();
		//WallMarkPos.GetRow(2) = -Dir;
		//WallMarkPos.GetRow(0) = Mat.GetRow(1);
		//WallMarkPos.RecreateMatrix(2, 0);
		//WallMarkPos.GetRow(3) = CInfo.m_Pos;
	}
	else
	{
		SpawnEffectMat.GetRow(2) = -Dir;
		SpawnEffectMat.RecreateMatrix(2, 0);
		SpawnEffectMat.GetRow(3) = SpawnPos;
		//WallMarkPos.Unit();
		//WallMarkPos.GetRow(3) = SpawnPos;
	}

	// Collide to find a valid spawn position
	CMat4Dfp32 SpawnMat;
	SpawnMat.GetRow(2) = -Dir;
	CWO_PhysicsState TmpPhys = GetPhysState();
	TmpPhys.m_Prim[0].Create(OBJECT_PRIMTYPE_SPHERE, -1, 16.0f, 0.0f);
	CMat4Dfp32 p0;
	p0.Unit();
	p0.GetRow(3) = SpawnPos - (Dir * 8.0f);
	for (uint i = 0; i < 4; i++)
	{
		m_pWServer->Debug_RenderSphere(p0, 16.0f, 0xff0000ff, 0.0f, false);
		CInfo.Clear();
		bool bHit = m_pWServer->Phys_IntersectWorld((CSelection *) NULL, TmpPhys, p0, p0, m_iObject, &CInfo);
		if (bHit && CInfo.m_bIsValid)
		{
			if (i == 0)
				SpawnMat.GetRow(2) = CInfo.m_Plane.n; // store first normal
			p0.GetRow(3) = CInfo.m_Pos + (CInfo.m_Plane.n * 16.1f);
		}
		else break;
	}
	SpawnMat.GetRow(3) = p0.GetRow(3);//.GetRow(2) = CInfo.m_Plane.n;
	SpawnMat.GetRow(3) -= SpawnMat.GetRow(2) * 16.0f;
	if (M_Fabs(SpawnMat.GetRow(2) * Dir) < 0.1f)
		SpawnMat.GetRow(0) = Mat.GetRow(1);
	else
		SpawnMat.GetRow(0) = -Dir;
	SpawnMat.RecreateMatrix(2, 0);

	// Check so there's not one spawned already, or just check if we are ready for a new spawn
 	CFStr Name = CFStrF("DARKTYPE_%d", DarklingType);
	int iObj = m_pWServer->Selection_GetSingleTarget(Name);
	if (iObj > 0)
	{
		if ((_ControlLastPress & DPadMask) == 0) // Must release DPad before allowed to unspawn
		{
			// If one is spawned - tell it to get new target
			CWObject_Character* pDarkling = TDynamicCast<CWObject_Character>(m_pWServer->Object_Get(iObj));//CWObject_Character::IsCharacter(iObj, m_pWServer);
			if (pDarkling)
			{
				if (0)
				{ // kill it -- remove after milestone 8b
					//uint nHealth = pDarkling->Char()->Health();
					//pDarkling->Physics_Damage(CWO_DamageMsg((fp32)nHealth), iObj);
					m_pWServer->Object_Destroy(iObj);
					Sound(m_pWServer->GetMapData()->GetResourceIndex_Sound("Gam_drk_unsumm"));
				}
				else
				{ // tell it to retarget -- enable after milestone 8b
					// What IS this code anyway? Seems like we insta-redirect a newly spawned darkling?
					CWObject_Message Msg(OBJMSG_AIEFFECT_RETARGET);
					Msg.m_Param0 = 0;	// No jump
					Msg.m_VecParam0 = SpawnEffectMat.GetRow(3);
					Msg.m_VecParam1 = SpawnEffectMat.GetRow(2);
					pDarkling->OnMessage(Msg);
				}
			}
		}
		// Reset "spawn level"
		if (pCD->m_DarklingSpawn.Set(0, pCD->m_GameTick-20, 0.0f))
			pCD->m_DarklingSpawn.MakeDirty();

		return;
	}

	// Check so powerlevels are ok
/*	if (pCD->m_Darkness < DARKNESS_DARKLING_ACTIVATIONCOST)
	{
		if (pCD->m_DarklingSpawn.Set(0, pCD->m_GameTick-20, 0.0f))
			pCD->m_DarklingSpawn.MakeDirty();

		return;
	}

	// Meassure light intensity at spawn position
	fp32 Light = 1.0f;
	if (m_spAI && m_spAI->m_spLightMeter)	// TODO: Don't use the player's lightmeter for this, it will invalide the internal "cache" of the lightmeter
	{
		m_spAI->m_spLightMeter->Measure(SpawnPos, SpawnPos, 8.0f, 8.0f, 10, true);
		Light = m_spAI->m_spLightMeter->GetIntensity(0);
	}*/
	fp32 Light = 0.0f; // Temp - allow darklings to be spawned anywhere

	if (Light < 0.4f) // TWEAK THIS VALUE!
	{
		if (pCD->m_DarklingSpawn.Set(255, pCD->m_GameTick+2, 0.25f))
			pCD->m_DarklingSpawn.MakeDirty();
	}
	else
	{
		if (pCD->m_DarklingSpawn.Set(64, pCD->m_GameTick+2, 0.25f))
			pCD->m_DarklingSpawn.MakeDirty();
	}

	uint8 SpawnLevel = pCD->m_DarklingSpawn.Get(pCD->m_GameTick, 0.0f);

  #ifndef M_RTM
	CBox3Dfp32 box(SpawnPos - CVec3Dfp32(SpawnLevel*(16.0f/255.0f)), SpawnPos + CVec3Dfp32(SpawnLevel*(16.0f/255.0f)));
	uint32 Color = CPixel32((int)(Light*255), (int)((1-Light)*255), 0, 255);
	m_pWServer->Debug_RenderAABB(box, Color, false);
  #endif

	// Update spawn effect position and animation times
	{
		if(pSpawnEffect)
		{
			// Set anim time to be correct with spawn level
			const fp32 TickPerSec = m_pWServer->GetGameTicksPerSecond();
			if(Light < 0.4f)
				pSpawnEffect->SetAnimTick(m_pWServer, TruncToInt((SpawnLevel * 0.00390625f) * TickPerSec), 0);
			else
			{
				// Ugly hack to remove darkling light flashing!
				if(SpawnLevel >= 62 && SpawnLevel <= 66)
					pSpawnEffect->SetAnimTick(m_pWServer, TruncToInt((TickPerSec*2.0f - 1) - 0.25f * TickPerSec), 0);
				else
					pSpawnEffect->SetAnimTick(m_pWServer, TruncToInt((TickPerSec*2.0f - 1) - (SpawnLevel * 0.00390625f) * TickPerSec), 0);
			}

			if(m_iDarkness_DarklingSpawn > 0)
				m_pWServer->Object_SetPosition(m_iDarkness_DarklingSpawn, SpawnEffectMat);
		}
		else
		{
			m_iDarkness_DarklingSpawn = m_pWServer->Object_Create(m_Darkness_DarklingSpawn, SpawnEffectMat, m_iObject);
			pSpawnEffect = (CWObject_EffectSystem*)m_pWServer->Object_Get(m_iDarkness_DarklingSpawn);
			if(pSpawnEffect)
			{
				pSpawnEffect->SetAnimTick(m_pWServer, 0, 0);
				m_pWServer->Object_SetPosition(m_iDarkness_DarklingSpawn, SpawnEffectMat);
			}
		}
	}

	if (SpawnLevel == 255)
	{
		// Ok, it's time to spawn a darkling
		iObj = m_pWServer->Object_Create(m_lAvailableDarklings[DarklingType].m_DarklingType, SpawnMat);
		m_pWServer->Object_SetName(iObj, Name);

		CWObject_CharDarkling* pDarkling = (CWObject_CharDarkling*)m_pWServer->Object_Get(iObj);
		if (pDarkling)
			pDarkling->Char_PrepareSpawn();

		// Take some darkness
		//pCD->m_Darkness = pCD->m_Darkness - DARKNESS_DARKLING_ACTIVATIONCOST;
		// Add darkling spawned flag
		pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode | PLAYER_DARKNESSMODE_DARKLINGSPAWNED;
		// Make spawn sound
		Sound(m_pWServer->GetMapData()->GetResourceIndex_Sound("Gam_drk_summon"));

		// Release spawn object for self destruction
		if(pSpawnEffect)
		{
			pSpawnEffect->SetAnimTick(m_pWServer, (int)m_pWServer->GetGameTicksPerSecond(), 0);
			m_iDarkness_DarklingSpawn = 0;
		}

		if (pCD->m_DarklingSpawn.Set(0, pCD->m_GameTick-20, 0.0f))
			pCD->m_DarklingSpawn.MakeDirty();
	}
}
#endif//NEVER

int CWObject_Character::Char_ExtractDarkling(const CMat4Dfp32& _ExtractMat, const CStr& _DarklingType)
{
	CFStr Name = CFStrF("DARKTYPE_EXTRACT_%s", _DarklingType.Str());
	int iObj = m_pWServer->Object_Create(_DarklingType, _ExtractMat);
	m_pWServer->Object_SetName(iObj, _DarklingType);
	return iObj;
}

int CWObject_Character::Char_DarknessShieldHit(int _Damage)
{
	MSCOPESHORT(CWObject_Character::Char_DarknessShieldHit);

	CWO_Character_ClientData* pCD = GetClientData(this);
	if (!pCD)
		return _Damage;

	// Scale damage according to health, since that's what the projectiles have been targeted for.
	int Darkness = pCD->m_Darkness.m_Value;
	if (Darkness > 0)
	{
		int MaxHealth = pCD->m_MaxHealth.m_Value;
		fp32 DamageScale = 20.0f / (fp32(MaxHealth));
		int DarknessDamage = MaxMT(1, TruncToInt(Ceil(fp32(_Damage) * DamageScale)));

		// Deal darkness damage and remove shield if depleted
		pCD->m_Darkness = Clamp(Darkness - DarknessDamage, 0, 0x64);
		if (DarknessDamage >= Darkness)
		{
			Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD, false, false);
			Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSVISION, false, false);

			// Make player invincible for a short period after shield is gone.
			m_ShieldIgnoreDamageEndTick = pCD->m_GameTick + TruncToInt(m_pWServer->GetGameTicksPerSecond() * 5.0f);
		}

		// Don't deal any damage to player at all when shield is damage
		_Damage = 0;
	}
	
	// Calculate amount of darkness damage (TODO: Make it easier to tweak?)
	/* Old shield damage code
	int ShieldDamage = MaxMT(MinMT(_Damage, 1), int(_Damage * 0.9f));
	uint MaxHealth = pCD->m_MaxHealth.m_Value;
	fp32 DarknessScale = fp32(pCD->m_MaxDarkness.m_Value) / (fp32(MaxHealth) * 0.25f);
	int DarknessDamage = TruncToInt(DarknessScale * fp32(ShieldDamage));
	uint Darkness = pCD->m_Darkness.m_Value;
	if(DarknessDamage < Darkness)
	{
		// Reduce amount of health damage and remove darkness
		_Damage -= ShieldDamage;
		pCD->m_Darkness = Darkness - DarknessDamage;
	}
	else
	{
		// Calculate new damage
		//_Damage = (_Damage - ShieldDamage) + TruncToInt(MaxMT(fp32(DarknessDamage - Darkness), 0.0f) / DarknessScale);
		_Damage = 0;
		
		// Make player invincible for a short period
		m_ShieldIgnoreDamageEndTick = pCD->m_GameTick + TruncToInt(m_pWServer->GetGameTicksPerSecond() * 5.0f);

		// Remove remaning darkness and deactivate shield
		pCD->m_Darkness = 0;
		Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD, false, false);
		Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSVISION, false, false);
	}

	// Make sure we don't damage player lower that to 1/8th of his max health when shield is active
	uint LowestHealth = (MaxHealth / 8);
	uint Health = pCD->m_Health.m_Value;
	int PostHealth = MaxMT(10, (Health - _Damage));
	_Damage = (Health - PostHealth);
	*/
	
	// Return damage
	return _Damage;
}

bool CWObject_Character::Char_DevourTarget(int _iTarget)
{
	// Handle devouring of a target
	CWObject_Character* pTarget = safe_cast<CWObject_Character>(m_pWServer->Object_Get(_iTarget));
	CWO_Character_ClientData* pCD = GetClientData(this);
	CWObject* pTentacleObj = (pCD) ? m_pWServer->Object_Get(pCD->m_iDarkness_Tentacles) : NULL;
	if (!pTentacleObj || !pTarget)
		return false;

	// Get darkling inhabitant if any
	CStr DarklingInhabitant;
	CWObject_Message Msg(OBJMSG_CHAR_GETINHABITINGDARKLING);
	Msg.m_pData = &DarklingInhabitant;
	m_pWServer->Message_SendToObject(Msg, _iTarget);

	// Send tentacles down for devouring
	CWObject_TentacleSystem& TentacleSystem = *safe_cast<CWObject_TentacleSystem>(pTentacleObj);
	const bool bDevourDarkling = ((DarklingInhabitant != "") ? !Char_IsDarklingAvailable(DarklingInhabitant) : false);
	if (!TentacleSystem.GrabAndDevour(*pTarget, bDevourDarkling))
		return false;

	// Make user immobile, tentacles will release when they are about to finnish
	pCD->m_Phys_Flags = pCD->m_Phys_Flags | PLAYER_PHYSFLAGS_IMMOBILE;

	// Remove heart
	pTarget->SetHeart(false);

	// Set devouring-flag, to prevent other power from being activated while devouring
	pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode | PLAYER_DARKNESSMODE_POWER_DEVOUR;

	// Let other characters react when using darkness powers
	if (m_spAI)
	{
		m_spAI->SetPlayerDarkness(pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_AIDARKNESSMASK);
		m_spAI->SetDarknessPos(pTarget->GetPosition());
	}

	// No internall check for devouring possibilities
	// Fade out and self destruct any mouth smoke character might have in 4 seconds
	if (pTarget->m_iDarkness_MouthSmoke != 0)
	{
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_EFFECTSYSTEM_SETFADE, FXFADE_OUT), pTarget->m_iDarkness_MouthSmoke);
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_EFFECTSYSTEM_SELFDESTRUCT, 4000), pTarget->m_iDarkness_MouthSmoke);
		pTarget->m_iDarkness_MouthSmoke = 0;
	}

	return true;
}

bool CWObject_Character::Char_DevourTarget_PreFinish()
{
	// Handle pre finish of devouring, when the tentacles rip out the heart
	CWO_Character_ClientData* pCD = GetClientData(this);
	if (!pCD)
		return false;

	pCD->m_Phys_Flags = pCD->m_Phys_Flags & ~PLAYER_PHYSFLAGS_IMMOBILE;
	return true;
}

bool CWObject_Character::Char_DevourTarget_Finish(int _iTarget)
{
	// Handle finish of devouring, when tentacle animation have finished
	CWO_Character_ClientData* pCD = GetClientData(this);
	if (!pCD)
		return false;

	//pCD->m_DarknessLevel.AddHeart();
	pCD->m_MaxDarkness = pCD->m_DarknessLevel.GetMaxDarkness();

	// Check if the target has any powerups
	Char_AddDarknessPowerups(_iTarget, pCD);

	pCD->m_DarknessSelectionMode = pCD->m_DarknessSelectionMode & ~PLAYER_DARKNESSMODE_POWER_DEVOUR;
	return true;
}


void CWObject_Character::Char_AddDarknessPowerups(const int32& _iFromCorpseObj, CWO_Character_ClientData* _pToCD)
{
	CWObject_Character* pCorpse = safe_cast<CWObject_Character>(m_pWServer->Object_Get(_iFromCorpseObj));
	if(pCorpse)
	{
		const int nPowerups = pCorpse->m_lDarknessPowerups.Len();
		const SDarknessPowerup* pPowerups = pCorpse->m_lDarknessPowerups.GetBasePtr();
		for(int i = 0; i < nPowerups; i++)
		{
			const uint8& Type = pPowerups[i].m_Type;
			const CStr& Data = pPowerups[i].m_Data;

			switch(Type)
			{
			// Add darkling
			case PLAYER_DARKNESSPOWERUP_DARKLING:
				{
					Char_AddDarkling(Data, true);
					break;
				}

			// Add max darkness
			case PLAYER_DARKNESSPOWERUP_MAXDARKNESS:
				{
					_pToCD->m_MaxDarkness = MinMT(_pToCD->m_MaxDarkness + Data.Val_int(), 0xff);
					break;
				}

			// Add core power
			case PLAYER_DARKNESSPOWERUP_COREPOWER:
				{
					uint AddDarknessPowers = ResolveDarknessFlags(Data);
					Char_AddDarknessPowerups_ShowInfoScreen((AddDarknessPowers & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK), "§LDARKNESS_NEWPOWER|§LDARKNESS_CREEPINGDARK");
					Char_AddDarknessPowerups_ShowInfoScreen((AddDarknessPowers & PLAYER_DARKNESSMODE_POWER_DEMONARM), "§LDARKNESS_NEWPOWER|§LDARKNESS_DEMONARM");
					Char_AddDarknessPowerups_ShowInfoScreen((AddDarknessPowers & PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS), "§LDARKNESS_NEWPOWER|§LDARKNESS_ANCIENTWEAPONS");
					Char_AddDarknessPowerups_ShowInfoScreen((AddDarknessPowers & PLAYER_DARKNESSMODE_POWER_BLACKHOLE), "§LDARKNESS_NEWPOWER|§LDARKNESS_BLACKHOLE");
					Char_AddDarknessPowerups_ShowInfoScreen((AddDarknessPowers & PLAYER_DARKNESSMODE_POWER_DARKNESSVISION), "§LDARKNESS_NEWPOWER|§LDARKNESS_DARKNESSVISION");
					Char_AddDarknessPowerups_ShowInfoScreen((AddDarknessPowers & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD), "§LDARKNESS_NEWPOWER|§LDARKNESS_DARKNESSHIELD");
					_pToCD->m_DarknessPowersAvailable = _pToCD->m_DarknessPowersAvailable | ResolveDarknessFlags(Data);
					break;
				}
			}
		}
	}
}

void CWObject_Character::Char_AddDarknessPowerups_ShowInfoScreen(int _bShow, const char* _pStr)
{
	if (_bShow)
	{
		CWObject_Message Msg(OBJMSG_GAME_SHOWGAMEMSG, 2000, 0, m_iObject, 1, 0, 0, (void*)_pStr);
		m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());
	}
}

bool CWObject_Character::Char_IsDarklingAvailable(const CStr& _Type)
{
	if(_Type != "")
	{
		CStr Type = _Type.LowerCase();
		int32 Len = m_lAvailableDarklings.Len();

		for(int32 i = 0; i < Len; i++)
		{
			if(m_lAvailableDarklings[i].m_DarklingType.CompareSubStr(_Type) == 0)
				return true;
		}
	}
	
	return false;
}

CStr CWObject_Character::Char_GetSpawnDarkling(const CStr& _Type)
{
	CFStr Choices[16];
	uint32 nRandoms = 0;

	if(_Type != "")
	{
		CStr Type = _Type.LowerCase();
		int32 Len = m_lAvailableDarklings.Len();

		for(uint32 i = 0; i < Len; i++)
		{
			if(m_lAvailableDarklings[i].m_DarklingType.CompareSubStr(_Type) == 0 && nRandoms < 16)
			{
				Choices[nRandoms] = m_lAvailableDarklings[i].m_DarklingType;
				nRandoms++;
			}
		}
	}

	if(nRandoms)
	{
		uint iRandom = MRTC_RAND() % nRandoms;
		return Choices[iRandom];
	}
	CStr Empty;
	return Empty;
}

bool CWObject_Character::Char_AddDarkling(const CStr& _Type, bool _bSendInfoMsg)
{
	CWO_Character_ClientData* pCD = GetClientData(this);
	M_ASSERT(pCD,"No char clientdata!");
	// Add a darkling of the given type
	CStr Type = _Type.LowerCase();

	// Check so we don't have this type already
	//int32 Len = m_lAvailableDarklings.Len();
	//bool bOk = true;
	//for (int32 i = 0; i < Len; i++)
	//{
	//	if (m_lAvailableDarklings[i].m_DarklingType == Type)
	//	{
	//		bOk = false;
	//		break;
	//	}
	//}

	bool bOk = !Char_IsDarklingAvailable(Type);

	// Darkling not found, add it to the list
	if (bOk)
	{

		stDarklingType DarkType;
		DarkType.m_DarklingType = Type;
		DarkType.m_bTaggedForFocus = _bSendInfoMsg;
		DarkType.m_AddedTick = pCD->m_GameTick;
		m_lAvailableDarklings.Add(DarkType);
		// Give some message that a new darkling has been found
		// (so the user can recofigure to dispatch new darkling types)
		//ConOut(CStrF("Added darkling: %s",Type.GetStr()));
		Char_AddDarknessPowerups_ShowInfoScreen(_bSendInfoMsg ? 1 : 0, CStrF("§LDARKNESS_NEWDARKLING|§L%s",Type.Str()).GetStr());
		
		// Send message to all darklingspawnpoints that player has gotten a new type
		TSelection<CSelection::LARGE_BUFFER> Selection;
		m_pWServer->Selection_AddClass(Selection,"darklingspawn");
		CWObject_Message Msg(OBJMSG_DARKLINGSPAWN_DOSPAWNCHECK,1,0,m_iObject);
		Msg.m_pData = Type.GetStr();
		const int16* pList = NULL;
		int NumSel = m_pWServer->Selection_Get(Selection,&pList);
		for (int32 i = 0; i < NumSel; i++)
			m_pWServer->Message_SendToObject(Msg,pList[i]);
	}

	return bOk;
}

void CWObject_Character::Char_DarknessFlashBar(int32 _Color)
{
	CWO_Character_ClientData* pCD = GetClientData(this);
	if (!pCD)
		return;

	pCD->m_DarknessBarFlashTick = pCD->m_GameTick;
	pCD->m_DarknessBarFlashColor = _Color;
	// Show health hud
	pCD->m_HealthHudStart = m_pWServer->GetGameTick();
}

void CWObject_Character::Char_HealthWobble(fp32 _Amount)
{
	CNetMsg NetMsg(PLAYER_NETMSG_HEALTHWOBBLE);
	NetMsg.Addfp32(_Amount);
	m_pWServer->NetMsg_SendToObject(NetMsg, m_iObject);
}

int CWObject_Character::Physics_Damage(const CWO_DamageMsg &_DX, int _iSender)
{
	MAUTOSTRIP(CWObject_Character_Physics_Damage, 0);
	MSCOPESHORT(CWObject_Character::Physics_Damage);
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
	if((m_Flags & PLAYER_FLAGS_WAITSPAWN) || !pCD)
		return 0;

	bool bIsDead = Char_GetPhysType(this) == PLAYER_PHYS_DEAD;
	bool bIsP6 = false;
	bool bIsMP = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), m_pWServer->Game_GetObjectIndex()) ? true : false;
	CWObject_Message Msg(OBJMSG_GAME_GETIS_P6);
	if(m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex()))
		bIsP6 = true;
	if (_DX.m_DamageType & DAMAGETYPE_DISORIENT_MASK)
	{
		// Handle any disorientation damage types such as flashbangs etc
		if (_DX.m_DamageType & DAMAGETYPE_FLASH)
		{
			/*
			fp32 Intensity = (m_spAI) ? m_spAI->GetLightIntensity(0) : 0.0f;
			fp32 LightMapIntensity = (m_spAI) ? m_spAI->GetLightmapIntensity() : 0.0f;

			// Ops, we just got flashed. No cover available here!
			if(Intensity >= 0.4f)
			{
				// Send flash disorientation to object.
				CNetMsg Msg(PLAYER_NETMSG_DISORIENTATION_FLASH);
				m_pWServer->NetMsg_SendToObject(Msg, m_iObject);
			}
			*/

			// Setup a fake point light to use as flashbang measurement light
			if(m_spAI && m_spAI->m_spLightMeter)
			{
				const fp32 Height = m_spAI->GetCurHeight();
				const fp32 Width = m_spAI->GetBaseSize();
				const CVec3Dfp32 BoxPos = m_spAI->GetBasePos();
				const CVec3Dfp32 MeasurePos = (m_spAI->IsPlayer()) ? BoxPos + CVec3Dfp32(Width * (Random-0.5f) * 0.5f, Width * (Random-0.5f) * 0.5f, Height * 0.5f) :
																	BoxPos + CVec3Dfp32(0, 0, 6);
				CXR_Light FakePointLight(_DX.m_Position, CVec3Dfp32(1,1,1), (fp32)_DX.m_Damage, 0, CXR_LIGHTTYPE_POINT);
				const fp32 Intensity = m_spAI->m_spLightMeter->MeasureFakePointLight(BoxPos, MeasurePos, Width, Height, FakePointLight, true, m_iObject, _iSender);

				if(Intensity > 0.4f)
				{
					// Send flash disorientation to object.
					CNetMsg Msg(PLAYER_NETMSG_DISORIENTATION_FLASH);
					m_pWServer->NetMsg_SendToObject(Msg, m_iObject);
				}
			}
		}
	}
	else
	{
		fp32 OrgDamage = _DX.m_Damage;
		int Damage = _DX.m_Damage;
	//	fp32 Force = 1.0f;

		Char_HealthWobble(OrgDamage*0.05f);

		// Put character on fire?
		if (_DX.m_DamageType & DAMAGETYPE_FIRE)
		{
			if (pCD->m_Burnable.m_Value.SetBurning(true, m_pWServer))
				pCD->m_Burnable.MakeDirty();
		}

		if(_iSender > 0)
		{
	/*		if(_iSender == m_iObject)
			{
				Damage = 0;
				Force *= PLAYER_SELFSPLASH_SCALE;
			}*/

	/* Hit with friendly fire
			else
			{
				//No friendly fire damage... kinda deprecated, since team concept is fuzzier in PB
				CWObject *pSender = m_pWServer->Object_Get(_iSender);
				if(pSender && pSender->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER)
				{
					for (int i = 0; i < Team_GetNum(); i++)
					{
						if (m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_TEAM_BELONGTOTEAM, Team_Get(i)), _iSender))
						{
							//Friendly fire! Stop damage but send message to AI in any case
							if (IsBot())
								m_spAI->OnMessage(CWObject_Message(OBJMSG_GAME_DAMAGE, 0, _DX.m_DamageType, _iSender));
							return 0;
						}
					}
				}
			}
	*/
		}

		if (pCD->m_iPlayer != -1)
		{
			CWObject* pTentacleObj = m_pWServer->Object_Get(pCD->m_iDarkness_Tentacles);
			if ((pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD) && pTentacleObj)
			{
				CWObject_TentacleSystem& TentacleSystem = *safe_cast<CWObject_TentacleSystem>(pTentacleObj);
				TentacleSystem.DoHurtResponse();
			}

			if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK)
			{
				CWObject_CoreData* pObj = m_pWServer->Object_GetCD(pCD->m_iCreepingDark);
				if(pObj)
					m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CREEPINGDARK_BACKTRACK), pObj->m_iObject);
			}
		}

		bool bRicochet = false;
		int SurfaceType = _DX.m_SurfaceType;
		int HitLoc = SHitloc::HITLOC_ALL;
		if(Damage > 0)
		{	// How do we know what kind of damage it was?
			if (_DX.m_pCInfo)
			{
				HitLoc = GetHitlocation(_DX.m_pCInfo);
			}
			fp32 DamageFactor = GetHitlocDamageFactor(_DX.m_DamageType, HitLoc, SurfaceType, _DX.m_Damage, _iSender);
			Damage = RoundToInt(Damage * DamageFactor);
			if ((DamageFactor > 0.0f) && (Damage < 1))
			{
				Damage = 1;
			}

			if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), m_pWServer->Game_GetObjectIndex()))
			{
				CWObject_Message Msg;
				Msg.m_Msg = OBJMSG_GAME_GETPLAYER;
				Msg.m_Param0 = m_iObject;
				//CWObject_CharShapeshifter* pPlayerObj = safe_cast<CWObject_CharShapeshifter>(m_pWServer->Object_Get(m_iObject));
				CWO_CharShapeshifter_ClientData* pCDShape = safe_cast<CWO_CharShapeshifter_ClientData>(pCD);
				if(pCDShape && pCDShape->m_ActiveBoosts & MP_BOOST_ACTIVE_SHIELD)
				{
					CWObject *pSender = m_pWServer->Object_Get(_iSender);
					CWObject_Character *pChar = TDynamicCast<CWObject_Character>(pSender);
					if(pChar)
					{
						CWO_Character_ClientData *pCDSender = CWObject_Character::GetClientData(pChar);
						if(pCDSender && pCDSender->IsDarkling())
						{
							CWO_CharDarkling_ClientData *pDarklingCD = pCDSender->IsDarkling();
							if(pDarklingCD->m_State >= DARKLING_STATE_JUMP_INIT && pDarklingCD->m_State <= DARKLING_STATE_JUMP_LAND)
							{
								int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("Env_Itm_Mult_Shieldactivate");
								if(iSound > 0)
									m_pWServer->Sound_On(m_iObject, iSound, WCLIENT_ATTENUATION_3D);
								pCDShape->m_ActiveBoosts.m_Value &= ~MP_BOOST_ACTIVE_SHIELD;
								Damage = 0;

								pCD->m_HitEffect_Flags = pCD->m_HitEffect_Flags | PLAYER_HITEFFECT_FLAGS_DAMAGE | PLAYER_HITEFFECT_FLAGS_BLUR | PLAYER_HITEFFECT_FLAGS_SOUND;
								pCD->m_HitEffect_Duration = pCD->m_pWPhysState->GetGameTick() + TruncToInt(pCD->m_pWPhysState->GetGameTicksPerSecond() * 2.0f);
								pCD->m_HitEffect_iObject = _iSender;
							}
						}
					}
				}
			}
		}

		// Reduce damage further if friendly fire
		CAI_Core* pAI = AI_GetAI();
		if (pAI)
		{
			CAI_AgentInfo* pAttacker = pAI->m_KB.GetAgentInfo(_iSender);
			if ((pAttacker) && (pAttacker->GetCurRelation() <= CAI_AgentInfo::FRIENDLY))
			{
				Damage = (int)(Damage * pAI->m_FriendlyFireFactor);
			}
		}
		


		// Here we tell the animgraph that it got hit
		// we do not send the original damage though
		CWO_DamageMsg curDmg = _DX;
 		curDmg.m_Damage = Damage;

		//Send message to characters AI that someone (tried to) hurt it.
		if (IsBot() && !bIsDead)
		{	// Send damage message to ai
			if (!(pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMUNE))
			{
				m_spAI->OnTakeDamage(_iSender,Damage,_DX.m_DamageType);
			}
		}

		if (!bIsDead)
		{
			//if ((m_OnTakeDamageTypeMask == 0x7fffffff)||(m_OnTakeDamageTypeMask & _DX.m_DamageType))
			if ((m_OnTakeDamageTypeMask == ~0)||(m_OnTakeDamageTypeMask & _DX.m_DamageType))
			{
				m_MsgContainer.SendMessage(PLAYER_SIMPLEMESSAGE_ONTAKEDAMAGE, m_iObject, m_iObject, m_pWServer);
			}
		}

		//Immune bots aren't affected by damage
		if (pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMUNE)
			return 0;

		if(pCD->m_iPlayer != -1)
		{
			if (Damage > 0)
			{
				if (_DX.m_DamageType & DAMAGETYPE_MELEE_MASK)
				{	// Make some noise
					m_pWServer->Message_SendToObject(
						CWObject_Message(OBJMSG_CHAR_RAISENOISELEVEL,99),m_iObject);
				}
				else if (_DX.m_DamageType & (DAMAGETYPE_PISTOL | DAMAGETYPE_RIFLE | DAMAGETYPE_AP | DAMAGETYPE_BLAST))
				{	// Make a loud noise
					m_pWServer->Message_SendToObject(
						CWObject_Message(OBJMSG_CHAR_RAISENOISELEVEL,200),m_iObject);
				}
			}
			
			if(Damage > 8)
			{
				CNetMsg Msg(PLAYER_NETMSG_RUMBLE);
				if(Damage > 16)
					Msg.AddStr("DamageFrom_ShotGun01");
				else
					Msg.AddStr("DamageFrom_ASR01");

				m_pWServer->NetMsg_SendToObject(Msg, m_iObject);
			}

			bool bDarknessShield = (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD) != 0;
			fp32 HitEffectDuration = 0.45f;
			uint HitEffectRenderMask = bDarknessShield ? PLAYER_HITEFFECT_FLAGS_BLUR : PLAYER_HITEFFECT_RENDERMASK;
			pCD->m_HitEffect_Flags = pCD->m_HitEffect_Flags | PLAYER_HITEFFECT_FLAGS_DAMAGE | HitEffectRenderMask | PLAYER_HITEFFECT_FLAGS_SOUND;
			pCD->m_HitEffect_Duration = pCD->m_pWPhysState->GetGameTick() + TruncToInt(pCD->m_pWPhysState->GetGameTicksPerSecond() * HitEffectDuration);
			if (pCD->m_HitEffect_iObject.m_Value != _iSender)
				pCD->m_HitEffect_iObject = _iSender;

			{
				if(pCD->m_Health > pCD->m_ShieldStart)
				{
					bool bIsDarkling = pCD->IsDarkling() ? true : false;
					m_pWServer->Sound_On(m_iObject, m_pWServer->GetMapData()->GetResourceIndex_Sound(bIsDarkling ? "D_Hurts_Darkl_Reaction_01" : "D_Hurts_Jackie_Reaction_01"), 0, 0, 1.0f, pCD->m_iPlayer);
				}
			}
		}

		if(Damage > 0 && !bIsDead)
		{
			// If we're attached, send damage to the one who's attached to this char (player)
			if (pCD->m_iMountedObject != 0 && !(m_MountFlags & PLAYER_MOUNTEDMODE_FLAG_ORIGINAL))
			{
				CWO_DamageMsg DX;
				DX.m_Damage = _DX.m_Damage;
				DX.m_DamageType = _DX.m_DamageType;
				DX.m_SurfaceType = _DX.m_SurfaceType;
				DX.m_StunTicks = _DX.m_StunTicks;
				DX.m_bPositionValid = _DX.m_bPositionValid;
				DX.m_bForceValid = _DX.m_bForceValid;;
				DX.m_bSplattDirValid = _DX.m_bSplattDirValid;
				DX.m_Position = _DX.m_Position;
				DX.m_Force = _DX.m_Force;
				DX.m_SplattDir = _DX.m_SplattDir;
				DX.m_pDamageEffect = _DX.m_pDamageEffect;
				DX.m_pCInfo = _DX.m_pCInfo;

				CWObject_Message Msg;
				Msg.m_Msg = OBJMSG_DAMAGE;
				Msg.m_pData = &DX;
				Msg.m_DataSize = sizeof(CWO_DamageMsg);
				m_pWServer->Message_SendToObject(Msg,pCD->m_iMountedObject);
				// Get health
				Msg.m_Msg = OBJMSG_CHAR_GETHEALTH;
				int32 Health = m_pWServer->Message_SendToObject(Msg,pCD->m_iMountedObject);
				Char()->Health() = Max((int32)1,Health);
			}
			else
			{
				if(pCD->m_iPlayer != -1)
				{
					if(m_ShieldIgnoreDamageEndTick < pCD->m_GameTick)
					{
						// If we have darkness shield activated						
						if (pCD->m_DarknessSelectionMode & (PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD | PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS))
							Damage = Char_DarknessShieldHit(Damage);

						int health = Char()->Health();
						bool bDarklingAttack = false;
						bool bJumpAttack = false;
						CWObject *pSender = m_pWServer->Object_Get(_iSender);
						CWObject_Character *pChar = TDynamicCast<CWObject_Character>(pSender);
						if(pChar)
						{
							CWO_Character_ClientData *pCDSender = CWObject_Character::GetClientData(pChar);
							CWO_CharDarkling_ClientData *pDarklingCD = pCDSender->IsDarkling();
							if(pCDSender && pDarklingCD)
							{
								bDarklingAttack = true;
								if(pDarklingCD->m_State >= DARKLING_STATE_JUMP_INIT && pDarklingCD->m_State <= DARKLING_STATE_JUMP_LAND)
									bJumpAttack = true;
							}
						}

						CWO_CharDarkling_ClientData *pDarklingCD = pCD->IsDarkling();
						if(!pDarklingCD)
						{
							if(!bDarklingAttack)
							{	// Apply damage
								if(health - Damage <= 1 && !bIsMP)
								{
									Damage = Max(health - 1, 1);
									m_ShieldIgnoreDamageEndTick = pCD->m_GameTick + TruncToInt(1.0f * m_pWServer->GetGameTicksPerSecond());
								}
							}
							else if(health - Damage <= 1 && !bJumpAttack)
								Damage = Max(health - 1, 1);
						}
						else
						{
							if(pDarklingCD->m_State >= DARKLING_STATE_JUMP_INIT && pDarklingCD->m_State <= DARKLING_STATE_JUMP_LAND)
								Damage = TruncToInt(Damage * 1.8f);
							else
								Damage = TruncToInt(Damage * 1.2f);
						}
						Char()->ApplyDamage(Damage);
					}
					else	//Even if we are immune, we still want to update the LastDamageTick, so the regeneration won't kick in as easy when we are taking fire
						Char()->m_LastDamageTick = pCD->m_GameTick;	
				}
				else
					Char()->ApplyDamage(Damage);
			}

			if(pCD->m_iPlayer == -1)
			{
				if(_DX.m_StunTicks != -1)
					Char_Stun(_DX.m_StunTicks);
				else if(Damage > 10)
					Char_Stun((int)(0.5f * m_pWServer->GetGameTicksPerSecond()));
			}
			else
			{
				CNetMsg Msg(PLAYER_NETMSG_FLASHSCREEN);
				Msg.AddInt8(Min(Damage, 255));
				m_pWServer->NetMsg_SendToObject(Msg, m_iObject);
			}
		}

		// No push if darkness shield is active
		int bForceValid = (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD) ? 0 : _DX.m_bForceValid;
		if (bForceValid && !(m_Flags & PLAYER_FLAGS_NONPUSHABLE) && !(pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMOBILE))
		{
			CVec3Dfp32 ForceDir = _DX.m_Force;
			ForceDir.Normalize();
	//		int bCrouch = Char_GetPhysType(this) == PLAYER_PHYS_CROUCH;
	//		fp32 d = ForceDir * CVec3Dfp32::GetRow(GetPositionMatrix(), 0);

			if(_DX.m_Force.LengthSqr() > 0 && !bIsP6)
			{
				CVec3Dfp32 Force = _DX.m_Force;
				if(pCD->m_iPlayer != -1)
				{
					CVec3Dfp32 Vel = m_pWServer->Object_GetVelocity(m_iObject);
					Vel += Force;
					Vel[0] = MaxMT(Min(Vel[0], 20.0f), -20.0f);
					Vel[1] = MaxMT(Min(Vel[1], 20.0f), -20.0f);
					Vel[2] = MaxMT(Min(Vel[2], 8.0f), -8.0f);
					m_pWServer->Object_SetVelocity(m_iObject, Vel);
				}
				else
					Physics_AddVelocityImpulse(Force);
			}
		}

		int bBlood = true;
		{ 
			MSCOPESHORT(GAME_BLOOD);
			MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
			if(pSys && pSys->GetOptions())
				bBlood = pSys->GetOptions()->GetValuei("GAME_BLOOD", 1);
		}

		if (Damage > 0 && bBlood && (pCD->m_iPlayer == -1))
		{
			const char* pDamageEffect = NULL;
			if (m_DamageEffect != "")
				pDamageEffect = m_DamageEffect;

			if (_DX.m_pDamageEffect != NULL && CFStr(_DX.m_pDamageEffect) != "")
				pDamageEffect = _DX.m_pDamageEffect;

			if (pDamageEffect != NULL && _DX.m_bPositionValid)
			{
				CMat4Dfp32 DamageEffectMatrix;
				DamageEffectMatrix.Unit();

				if (_DX.m_bSplattDirValid)
					_DX.m_SplattDir.SetMatrixRow(DamageEffectMatrix, 0);
				else if (_DX.m_bForceValid)
					_DX.m_Force.SetMatrixRow(DamageEffectMatrix, 0);
				else
					CVec3Dfp32(1, 0, 0).SetMatrixRow(DamageEffectMatrix, 0);
				CVec3Dfp32(0, 0, 1).SetMatrixRow(DamageEffectMatrix, 2);
				DamageEffectMatrix.RecreateMatrix(0, 2);
				_DX.m_Position.SetMatrixRow(DamageEffectMatrix, 3);

				CRPG_InitParams InitParams;
				InitParams.m_pCInfo = _DX.m_pCInfo;
				const int nParam = 1;
				aint pParam[nParam] = { (aint)&InitParams };

				m_pWServer->Object_Create(pDamageEffect, DamageEffectMatrix, m_iObject, pParam, nParam);
			}

			int SkeletonType = pCD->m_Aim_SkeletonType;
			if (SkeletonType == SKELETONTYPE_NORMAL || SkeletonType == SKELETONTYPE_BUTCHER)
			{
				// Send decal msg
				if (_DX.m_pCInfo && pCD->m_iPlayer == -1)
				{
	/*					if(pCD->m_iPlayer != -1)
						ConOutL(CStrF("Decal %i",_DX.m_pCInfo->m_LocalNode));*/
					CVec3Dfp32 Normal(_DX.m_pCInfo->m_Plane.n);
					Normal.MultiplyMatrix3x3(_DX.m_pCInfo->m_LocalNodePos);

					// TODO: Rotate normal to unanimated model space.

					Normal *= 127.0f;

					CNetMsg Msg(PLAYER_NETMSG_DECAL);
					Msg.AddVecInt16_Max32768(_DX.m_pCInfo->m_LocalPos);
					Msg.AddVecInt8_Max128(Normal);
					Msg.AddInt8(_DX.m_pCInfo->m_LocalNode);
					Msg.AddInt8(TruncToInt(Random * 2.999f));
					Msg.AddInt8(8);
					Msg.AddInt16(0);
					m_pWServer->NetMsg_SendToObject(Msg, m_iObject);
				}
			}
		}
		
		if (Char()->Health() <= 0 && !bIsDead) 
		{
			// No deathsound when headshotting
			if (!(m_Flags & PLAYER_FLAGS_NODEATHSOUND))
			{
				if (HitLoc == SHitloc::HITLOC_HIGH)
					PlayDialogue_Hash(MHASH1('28'));
				else
					PlayDialogue_Hash(MHASH1('27'));
			}

			if(!(pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_GODMODE))
			{
				Char_Die(_DX.m_DamageType, _iSender);
				if( (_DX.m_bPositionValid) && (_DX.m_bForceValid) && (_DX.m_Force != CVec3Dfp32(0)) )
				{
#ifdef INCLUDE_OLD_RAGDOLL
					if( m_spRagdoll != NULL )
						m_spRagdoll->AddPendingImpulse(_DX.m_Position, _DX.m_Force);
#endif // INCLUDE_OLD_RAGDOLL
					if( m_pPhysCluster != NULL )
					{
						uint32 iBest = m_pPhysCluster->QuickGetClosest(_DX.m_Position);
					//	m_pWServer->Phys_AddImpulse(m_pPhysCluster->m_lObjects[iBest].m_pRB,
					//		_DX.m_Position,_DX.m_Force);
						m_pWServer->Phys_AddForce(m_pPhysCluster->m_lObjects[iBest].m_pRB,
							_DX.m_Force * RAGDOLL_FORCE_MULTIPLIER);
					}
				}
	
				if (m_DeathEffect != "" && bBlood)
				{
					CMat4Dfp32 DamageEffectMatrix;
					DamageEffectMatrix.Unit();

					if (_DX.m_bSplattDirValid)
						_DX.m_SplattDir.SetMatrixRow(DamageEffectMatrix, 0);
					else if (_DX.m_bForceValid)
						_DX.m_Force.SetMatrixRow(DamageEffectMatrix, 0);
					else
						CVec3Dfp32(1, 0, 0).SetMatrixRow(DamageEffectMatrix, 0);
					CVec3Dfp32(0, 0, 1).SetMatrixRow(DamageEffectMatrix, 2);
					DamageEffectMatrix.RecreateMatrix(0, 2);
					(GetPosition() + CVec3Dfp32(0, 0, pCD->m_Phys_Height)).SetRow(DamageEffectMatrix, 3);

					m_pWServer->Object_Create(m_DeathEffect, DamageEffectMatrix);
					//m_pWServer->Object_AddChild(m_iObject, iDeathEffect);
				}

				return 2;
			}
			else
				Char()->Health() = Char()->MaxHealth();
		}
		else if ((Damage > 0) && !(pCD->m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_EXITRAGDOLL) && (_DX.m_bPositionValid) && (_DX.m_bForceValid) && (_DX.m_Force != CVec3Dfp32(0)))
		{
#ifdef INCLUDE_OLD_RAGDOLL
			if( m_spRagdoll != NULL )
				m_spRagdoll->AddPendingImpulse(_DX.m_Position, _DX.m_Force);
#endif // INCLUDE_OLD_RAGDOLL			
			if( m_pPhysCluster != NULL )
			{
				uint32 iBest = m_pPhysCluster->QuickGetClosest(_DX.m_Position);
			//	m_pWServer->Phys_AddImpulse(m_pPhysCluster->m_lObjects[iBest].m_pRB,
			//		_DX.m_Position,_DX.m_Force);
				m_pWServer->Phys_AddForce(m_pPhysCluster->m_lObjects[iBest].m_pRB,
					_DX.m_Force * RAGDOLL_FORCE_MULTIPLIER);
			}
		}

		if(Damage > 0 && !bIsDead)
		{
			// PainSoundDelay counter is updated in WObj_Char.cpp, ServerPredicted_Extras().
			if (pCD->m_PainSoundDelay <= 0)
			{
				if (Char()->Pain() > 15)
					PlayDialogue_Hash(PLAYER_DIALOGUE_PAIN1);
				else
					PlayDialogue_Hash(PLAYER_DIALOGUE_PAIN0);

				pCD->m_PainSoundDelay = (int)(PLAYER_PAINSOUNDDELAY * m_pWServer->GetGameTicksPerSecond());

				//ConOut(CStrF("Damage: PainSoundDelay = %d (RESET)", pCD->m_PainSoundDelay));
			}
			else
			{
				//ConOut(CStrF("Damage: PainSoundDelay = %d", pCD->m_PainSoundDelay));
			}

			// Set health hud to active
			pCD->m_HealthHudStart = m_pWServer->GetGameTick();
		}

		if (_DX.m_DamageType & DAMAGETYPE_TRANQUILLIZER)
		{
#ifdef INCLUDE_OLD_RAGDOLL
			if( m_spRagdoll )
				m_spRagdoll->AddImpulse(CConstraintSystem::BODY_ALL,CVec3Dfp32((Random-0.5f) * 5.0f,(Random-0.5f) * 5.0f,5.0f));
			else 
#endif // INCLUDE_OLD_RAGDOLL
			if( m_pPhysCluster )
			{
				TAP<CWPhys_ClusterObject> pPO = m_pPhysCluster->m_lObjects;
				for(uint i = 0;i < pPO.Len();i++)
				{
					CVec3Dfp32 Force = CVec3Dfp32((Random-0.5f) * 5.0f,(Random-0.5f) * 5.0f,5.0f);
					CVec3Dfp32 Pos = pPO[i].m_Transform.GetRow(3) + CVec3Dfp32((Random-0.5f) * 5.0f,(Random-0.5f) * 5.0f,5.0f);
					m_pWServer->Phys_AddImpulse(pPO[i].m_pRB,Pos,Force * RAGDOLL_FORCE_MULTIPLIER);
				}
			}
		}

		if (bRicochet)
			return DAMAGE_RETURN_RICOCHET;
	}

	return 1;
}

int CWObject_Character::Char_Stun(int _Ticks)
{
	MAUTOSTRIP(CWObject_Character_Char_Stun, 0);
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
	if(pCD)
	{
		pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_STUNNED;
		int Stun = Max(_Ticks - Char()->Wait(), 0);
		if(Stun)
			Char()->ApplyWait(Stun);
	}
	return true;
}

bool CWObject_Character::Char_DoBloodAtAttach(int32 _iRotTrack, int32 _iAttach, const CVec3Dfp32& _Direction, const CVec3Dfp32& _Offset, int32 _Count, const char* _pEffect)
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	int bBlood = true;
	if(pSys && pSys->GetOptions())
		bBlood = pSys->GetOptions()->GetValuei("GAME_BLOOD", 1);

	if (bBlood)
	{
		const char* pDamageEffect = _pEffect;
		if (!pDamageEffect)
			pDamageEffect = m_DamageEffect;

		if (pDamageEffect != NULL)
		{
			CMat4Dfp32 DamageEffectMatrix;
			DamageEffectMatrix.Unit();
			_Direction.SetMatrixRow(DamageEffectMatrix, 0);
			CVec3Dfp32(0, 0, 1).SetMatrixRow(DamageEffectMatrix, 2);
			DamageEffectMatrix.RecreateMatrix(0, 2);
			// Find position from skeleton
			// Find skeleton and evalutate current headoffset
			CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
			//CXR_Model* pModel1 = m_pWServer->GetMapData()->GetResource_Model(m_iModel[1]);
			//CXR_Skeleton *pSkel1 = pModel1 ? pModel1->GetSkeleton() : NULL;
			CXR_Skeleton *pSkel = pModel ? pModel->GetSkeleton() : NULL;

			/*CXR_Skeleton *pSkel = pSkel1;
			if(!pSkel || (pSkel0 && pSkel0->m_lNodes.Len() > pSkel->m_lNodes.Len()))
				pSkel = pSkel0;*/

			CXR_SkeletonInstance* pSkelInst = NULL;
			CXR_AnimState Anim;
			if (!GetEvaluatedPhysSkeleton(pSkelInst,pSkel,Anim))
				return false;

			CVec3Dfp32 Pos;
			if(pSkelInst && (_iRotTrack >= 0) && (_iRotTrack < pSkelInst->m_nBoneTransform))
			{
				const CMat4Dfp32& Trans = pSkelInst->GetBoneTransform(_iRotTrack);
				Pos = pSkel->m_lNodes[_iRotTrack].m_LocalCenter;
				Pos *= Trans;
				if(_iAttach != -1)
				{
					const CXR_SkeletonAttachPoint *pAttach = pSkel->GetAttachPoint(_iAttach);
					if(pAttach)
						Pos = pAttach->m_LocalPos.GetRow(3) * (Trans);
				}
			}
			Pos += _Offset;

			//_pWClient->Debug_RenderSkeleton(pSkel,Anim.m_pSkeletonInst);
			//return Pos.k[2] + HEAD_NECK_OFFSET - CVec3Dfp32::GetMatrixRow(_Position,3).k[2];

			Pos.SetMatrixRow(DamageEffectMatrix, 3);

			CRPG_InitParams InitParams;
			//?
			//InitParams.m_pCInfo = _DX.m_pCInfo;
//			const int nParam = 0;
			//int32 pParam[nParam] = { (int32)&InitParams };*/

			m_pWServer->Debug_RenderVertex(Pos,0xff7f7f00,20.0f,false);
			m_pWServer->Debug_RenderWire(Pos,Pos + _Direction * 20.0f,0xff7f7f00,20.0f,false);
			for (int32 i = 0; i < _Count; i++)
				m_pWServer->Object_Create(pDamageEffect/*"coordsys","hitEffect_bloodspurt01"*/, DamageEffectMatrix, m_iObject);//, pParam, nParam);
		}
	}

	return true;
}

/*int CWObject_Character::Char_ApplyDamageSimple(int _Damage, int _StunTicks, int _iSender, int _MinHealth)
{
	if (_Damage > 0)
	{
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);

		if (IsBot())
			m_spAI->OnMessage(CWObject_Message(OBJMSG_GAME_DAMAGE,_Damage,DAMAGETYPE_UNDEFINED,_iSender));

		//Immune bots aren't affected by damage
		if (m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETIMMUNE, 0, 0, m_iObject), m_iObject))
			return -1;

		if(_Damage > 0)
		{
			Char()->ApplyDamage(_Damage);
			
			// Stun a little
			if(pCD->m_iPlayer == -1)
			{
				if(_StunTicks != -1)
					Char_Stun(_StunTicks);
				else if(_Damage > 10)
					Char_Stun(0.5f * SERVER_TICKSPERSECOND);
			}

			if (Char()->Health() <= 0) 
			{
				if(!(pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_GODMODE))
				{
					// If the character should be dead, set health to zero and return 0
					Char()->Health() = _MinHealth;
					
					return 0;
				}
				else
				{
					// If character is god, grant him extra health
					Char()->Health() = Char()->MaxHealth();
				}
			}
		}

		if(pCD->m_iPlayer != -1)
		{
			CNetMsg Msg(PLAYER_NETMSG_FLASHSCREEN);
			Msg.AddInt8(Min(_Damage, 255));
			m_pWServer->NetMsg_SendToObject(Msg, m_iObject);
		}
	}

	return Char()->Health();
}*/

int CWObject_Character::Physics_Preintersection(int _iObject, CCollisionInfo *_pInfo)
{
	MAUTOSTRIP(CWObject_Character_Physics_Preintersection, 0);
	MSCOPESHORT(CWObject_Character::Physics_Preintersection);
	// Called from OnPhysicsEvent for all CWO_PHYSEVENT_IMPACT events.

//	if (m_FallDmgTimeoutTicks)
//		return SERVER_PHYS_DEFAULTHANDLER;

	CWObject *pObj = m_pWServer->Object_Get(_iObject);
	if(pObj && (pObj->GetPhysState().m_ObjectFlags & (OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PLAYERPHYSMODEL)))
	{	
		if (_pInfo && _pInfo->m_bIsValid)
		{
			CVec3Dfp32 Vel = m_pWServer->Object_GetVelocity(m_iObject);
// Changed 2002-04-18 /mh
//			fp32 ImpactVel = -(Vel * _pInfo->m_Plane.n) / _pInfo->m_Plane.n.Length();

			fp32 VelMag = Vel.Length();

			fp32 ImpactVel = 0;
			if (VelMag > 0.01f)
			{
				fp32 VelProj = M_Fabs(Vel * _pInfo->m_Plane.n);

				CVec3Dfp32 VelTang;
				Vel.Combine(_pInfo->m_Plane.n, VelProj, VelTang);
				fp32 VelTangMag = VelTang.Length();

				ImpactVel = 0.25f * (0.5f + 0.5f*M_Fabs((Vel * _pInfo->m_Plane.n) / VelMag)) * VelTangMag;
				ImpactVel += VelProj;
			}

			// FIXME: This should be predicted, but it can't be done until it's possible to run this only the first prediction.
			/*if (ImpactVel > PLAYER_FALLTILTHEIGHT)
				Char_AddTilt(CVec3Dfp32(1,0,0), 0.003f*fp32(Min(30.0f, ImpactVel * PLAYER_FALLTILTSCALE)));*/

			if (ImpactVel > PLAYER_FALLVELOCITYSOUND0)
			{
//				ConOutL(CStrF("Frame %d, Vel %f", m_pWServer->GetFrame(), ImpactVel));
				uint32 DlgItemHash = PLAYER_DIALOGUE_LAND;
/*				if (ImpactVel > PLAYER_FALLVELOCITYSOUND1)
					DlgItemHash = PLAYER_DIALOGUE_LAND1;
				else
					DlgItemHash = PLAYER_DIALOGUE_LAND0;*/

				m_FallDmgTimeoutTicks = 3;

				// Only accept damage when colliding to ground, not walls or ceiling.
// Removed 2002-04-18 /mh
// Added again 2004-03-10 /JA
				if (_pInfo->m_Plane.n.k[2] > 0.6)

				{
					CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
					if(pCD && pCD->m_iPlayer != -1 && (Char_GetControlMode(this) != PLAYER_CONTROLMODE_ANIMSYNC))
					{
						// Fall damage
						if (ImpactVel > PLAYER_FALLDAMAGE_MINVEL && m_iSoftSpotCount == 0 && pCD->m_iPlayer != -1)
						{
							Char()->Health();

							CWO_DamageMsg Msg(8, DAMAGETYPE_FALL);
							Msg.Send(m_iObject, 0, m_pWServer);
							if(Char_GetPhysType(this) == PLAYER_PHYS_DEAD)
								return SERVER_PHYS_DEFAULTHANDLER;
							// Make some noise
							m_pWServer->Message_SendToObject(
								CWObject_Message(OBJMSG_CHAR_RAISENOISELEVEL,99),m_iObject);
						}

						UpdateGroundMaterial(this, m_pWServer);
						PlayDialogue_Hash(DlgItemHash, DIALOGUEFLAGS_PLAYERFORCE2D, pCD->m_GroundMaterial);
					}

#ifdef PLAYER_JUMP_PREINTERSECT
					// Jump
					if (((m_Press & CONTROLBITS_JUMP) & m_Released) &&
						!(pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMOBILE))
					{
						ConOut(CStrF("Preintersect Jump: Press %.8x, Release %.8x", m_Press, m_Released));

						m_Released &= ~CONTROLBITS_JUMP;
						CVec3Dfp32 VRet = 0;

						CVec3Dfp32 CurV = pObj->GetMoveVelocity();
						fp32 vz = -Min(0.0f, CurV[2]);
	//#PHYSFIX:
						float fSpeed_Jump = m_Speed_Jump/* * Char()->JumpSpeed()*/;
						VRet[2] = Max(vz + fSpeed_Jump, fSpeed_Jump);
						CurV[2] = VRet[2];

						m_pWServer->Object_SetVelocity(m_iObject, VRet);
					}
#endif
				}
			}
		}
	}
	return SERVER_PHYS_DEFAULTHANDLER;
}

void CWObject_Character::Physics_AddVelocityImpulse(const CVec3Dfp32& _Impulse)
{
	int32 ControlMode = Char_GetControlMode(this);
	if (ControlMode == PLAYER_CONTROLMODE_ANIMATION)
	{
		CWO_Character_ClientData* pCD = GetClientData(this);
		// Might be problems in scripts with add (ex trigger adding it 8x/sec
		//pCD->m_AnimGraph2.AddPhysImpulse(_Impulse);
		pCD->m_AnimGraph2.SetPhysImpulse(_Impulse);
	}
	else
	{
		// Might be problems in scripts with add (ex trigger adding it 8x/sec
		//m_pWServer->Object_AddVelocity(m_iObject, _Impulse);
		m_pWServer->Object_SetVelocity(m_iObject, _Impulse);
	}
}

int CWObject_Character::Game_KilledPlayer(int _iPlayer, uint32 _DamageType)
{
	MAUTOSTRIP(CWObject_Character_Game_KilledPlayer, 0);
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
	if (!pCD) return 0;

	if(pCD->m_iPlayer < 0)
		return 0;

	CWO_Player* pP = m_pWServer->Game_GetObject()->Player_Get(pCD->m_iPlayer);
	CWO_Player* pPKilled = m_pWServer->Game_GetObject()->Player_Get(_iPlayer);

	bool bKilledSelf = (_iPlayer == pCD->m_iPlayer);
	if (bKilledSelf)
	{
/*
		switch(_DamageType)
		{
		case DAMAGE_FIRE:
			m_pWServer->Net_ConOut(CStrF("%s died in the flames.", (char*)pP->m_Name));
			break;

		default:
*/
		m_pWServer->Net_ConOut(CStrF("%s %s", (char*)pP->m_Name, "died."));
			/*
			break;
		}
		*/
	}
	else if (pP && pPKilled)
	{
//		switch(_DamageType)
//		{
//		default:
			m_pWServer->Net_ConOut(CStrF("%s was killed by %s.", (char*)pPKilled->m_Name, (char*)pP->m_Name));
//			break;
//		}
	}

	return 1;
}


bool CWObject_Character::CanUse(int _iUser)
{
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if (!pCD) 
		return false;

	// Can only use if in correct controlmode (the 2 main ones, free and animation controlled)
	int32 ControlMode = Char_GetControlMode(this);
	if (ControlMode != PLAYER_CONTROLMODE_FREE && ControlMode != PLAYER_CONTROLMODE_ANIMATION)
		return false;

	if (pCD->m_iPlayer != -1)  //You can't use playable characters
		return false;

	if (pCD->m_iFightingCharacter != -1)
		return false;

	if (m_pWServer->GetGameTick() - m_LastUsedTick < m_pWServer->GetGameTicksPerSecond() * 2)
		return false;

//	if (m_spAI->IsHostileOrWorse(_iUser))
	if (m_spAI->IsEnemy(_iUser))
		return false;

	return true; // basic checks ok
}


int CWObject_Character::OnUse(int _iUser, int _Param)
{
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if (!pCD) return 0;

	CAI_Core* pAI = AI_GetAI();
	bool bHasHeartLeft = HasHeart();
	bool bIsDead = Char_GetPhysType(this) == PLAYER_PHYS_DEAD;
	bool bIsConscious = (pAI && pAI->IsConscious());
	bool bIsStunned = ((pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_STUNNED) != 0);
	bool bDevourOk = (bHasHeartLeft && (bIsDead || !bIsConscious || bIsStunned));
	if (!CanUse(_iUser) && !bDevourOk)
		return 0;

	CWObject *pObj = m_pWServer->Object_Get(_iUser);
	if(!pObj)
		return 0;

	// Check if we are trying to devour the character
	if (bDevourOk)
	{
		// Tell activator to devour this object
		return m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_DEVOURTARGET, m_iObject), pObj->m_iObject);
	}


	if (_Param < 0)
	{ // SelectDialogueChoice
		uint8 nChoice = -(1 + _Param);
		if (pCD->m_liDialogueChoice.Len() > nChoice)
		{
			CWObject_Character *pChar = TDynamicCast<CWObject_Character>(pObj);
			if(pChar)
			{
				pChar->ClientFlags() |= PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK | PLAYER_CLIENTFLAGS_PLAYERSPEAK;
				Char_ActivateDialogueItem(pCD->m_liDialogueChoice[nChoice], _iUser); 
				m_LastUsedTick = m_pWServer->GetGameTick();
			}
			pCD->m_liDialogueChoice.SetLen(0);
			pCD->m_liDialogueChoice.MakeDirty();
		}
	
		return 0;
	}
	
	CDialogueLink ApproachItem = Char_GetDialogueApproachItem();
	if (ApproachItem.IsValid())
	{
		Char_ActivateDialogueItem(ApproachItem, _iUser);
		m_LastUsedTick = m_pWServer->GetGameTick();
	}

	return 1;
}

bool CWObject_Character::Char_PickupItem(int _bSelect, int _bInstant, const char *_pName, int _iSender, bool _bNoPickupIcon, bool _bDontAutoEquip)
{
	MAUTOSTRIP(CWObject_Character_Char_PickupItem, false);
	MSCOPESHORT(CWObject_Character::Char_PickupItem);
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if(!pCD)
		return false;

	SetWeaponUnequipTimer();

	bool bRemovePickup = false;

	TPtr<CRPG_Object_Item> spDropping;

	if(pCD->m_iPlayer != -1)
		CRPG_Object::m_bPrecacheForPlayerUse = true;
	CRPG_Object_Item* pPickedItem = Char()->PickupItem(_pName, /*!_bInstant*/_bSelect == 0, _bSelect != 0, &spDropping, m_iObject, _bInstant != 0, _iSender,pCD->m_iPlayer != -1, _bNoPickupIcon);
	CRPG_Object::m_bPrecacheForPlayerUse = false;
	if(pPickedItem == (CRPG_Object_Item *)0x1) // Just remove 
		return true;
	if (pPickedItem != NULL)
	{
		if(pPickedItem->m_Flags & RPG_ITEM_FLAGS_NOPICKUP)
			return true;

		//Abort any ongoing weapon selection
		pCD->m_SelectItemTimeOutTick = 0;

		if (_iSender > 0)
		{
			if (spDropping && (spDropping->m_iItemType != 0)) // Picked item replaces an old item of the same itemtype. Old item is droppet.
			{
				int Ammo = 0;
				CWObject *pObj = m_pWServer->Object_Get(_iSender);
				if(pObj && pObj->m_Param & 0x8000)
					Ammo = pObj->m_Param & 0x7fff;
				CWObject_Message Msg(OBJMSG_RPG_REPLACEPICKUP, 0, (int)(2.0f * m_pWServer->GetGameTicksPerSecond()));
				Msg.m_pData = (CRPG_Object_Item *)spDropping;
				m_pWServer->Message_SendToObject(Msg, _iSender);
				// Set ammo
				if (Ammo > 0)
					pPickedItem->SetAmmo(Ammo);
			}
			else // Taken. Remove the pickup.
			{
				/*CWObject *pObj = m_pWServer->Object_Get(_iSender);
				if(pObj && (pObj->m_Param & 0x8000) && !pPickedItem->m_bNoLoadedAmmoMergeOnPickup)
					pPickedItem->SetAmmo(pObj->m_Param & 0x7fff);*/
				bRemovePickup = true;
			}
//				m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_RPG_REMOVEPICKUP), _iSender);
		}

		Char()->UpdateItems();

		//Autoequip item if instant flag is set or item should be autoequipped 
		//and current item is not an autoequip item
		bool bAutoEquip = false;
	 	if (!_bDontAutoEquip)
 		{
			if (_bInstant)
			{
				//Always autoeuip
				bAutoEquip = true;
			}
			else if (pPickedItem->m_Flags & RPG_ITEM_FLAGS_DROPONUNEQUIP)
			{
				//Items that can only be in inventory if equipped must always be autoequipped when possible
				bAutoEquip = true;
			}
			else if (pPickedItem->m_Flags & RPG_ITEM_FLAGS_AUTOEQUIP)
			{
				//Autoequip items are only autoequipped if current equipped item isn't an autoequip item
				CRPG_Object_Item * pCurEquipped = Char()->GetFinalSelectedItem(pPickedItem->m_iItemType >> 8);
                if (!pCurEquipped || !(pCurEquipped->m_Flags & RPG_ITEM_FLAGS_AUTOEQUIP))
				{
					bAutoEquip = true;
				}
			}
		}
		if (bAutoEquip)
		{
			bool bAGOk = false;
			if (!(pPickedItem->m_iItemType >> 8) && !(pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_EQUIPPING) &&
				!pCD->m_AnimGraph2.GetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED) && pCD->m_iPlayer != -1)
			{
				// Reequip on right hand if a gun, otherwise unequip...?
				// Set identifier of next weapon and unequip weapon ("reload" for dual guns?)
				CRPG_Object_Item * pCurEquipped = Char()->GetFinalSelectedItem(AG2_ITEMSLOT_WEAPONS);
				if (pCurEquipped)
				{
					CWAG2I_Context AGContext(this,m_pWServer,pCD->m_GameTime);
					pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER,pPickedItem->m_Identifier);
					if (pCD->m_AnimGraph2.SendAttackImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,pCurEquipped->m_AnimType == RPG_ITEMTYPE_GUN ? AG2_IMPULSEVALUE_ITEMACTION_RELOAD : AG2_IMPULSEVALUE_ITEMACTION_UNEQUIP)))
						pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED,true);
					//m_LastUsedItem = pPickedItem->m_Identifier;
					//m_LastUsedItemType = pPickedItem->m_iItemType;
					bAGOk = true;
				}
			}

			if (!bAGOk)
			{
				// Force equip
				SelectItemByIdentifier(pPickedItem->m_iItemType >> 8,pPickedItem->m_Identifier, _bInstant != 0);
			}

			// Force player AG to weaponswitch if current weapon isn't the same as the one we got
			/*if ((pCD->m_iPlayer != -1) && !(m_ClientFlags & PLAYER_CLIENTFLAGS_DIALOGUE))
			{
				CRPG_Object_Item* pEquippedItem = Char()->GetFinalSelectedItem(0);
				int32 ItemClass = pCD->m_EquippedItemClass;
				if (pEquippedItem && (ItemClass != pEquippedItem->m_AnimProperty))
				{
					CWAG2I_Context AGContext(this,m_pWServer,pCD->m_GameTime);
					pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER,pEquippedItem->m_Identifier);
					if (pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_UNEQUIP),0))
						pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED,true);
				}
			}*/
			// Reset last weapon
			m_LastUsedItem = 0;
			m_LastUsedItemType = -1;
		}

		if(pCD->m_iPlayer != -1 && !_bNoPickupIcon)
		{
			pCD->m_Pickup_iIcon = (uint16)pPickedItem->m_iIconSurface;
			pCD->m_Pickup_StartTick = pCD->m_GameTick;
			pCD->m_Pickup_Magazine_Num = (int8)(Max(-1, Min(127, pPickedItem->ShowNumMagazines())));
			m_NextGUIScreen = 1;
		}
		Char_UpdateQuestItems();
	}
	
	return bRemovePickup;
}

void CWObject_Character::Char_EquipNewItem(CRPG_Object_Item* _pCurItem, bool _bIsPressLeft, bool _bOnlyWeaponsWithAmmo)
{
	CWO_Character_ClientData* pCD = GetClientData(this);
	CWAG2I_Context AGContext(this,m_pWServer,pCD->m_GameTime);
	enum { EUnequipped = 0, EPistols = 1, ERifles= 2};

	// First, find out the current type
	uint iCurrCategory = _bIsPressLeft ? EPistols : ERifles;

/*	int32 Count = 0;
	equiptryagain:
	Count++;
	if (_bOnlyWeaponsWithAmmo && iCurrCategory == EUnequipped)
	{
		// No dice if nothing's found after 4 tries, equip fist..
		if (Count <= 4)
			goto equiptryagain;
	}*/

	// Next, equip the new type;
	/*if (iCurrCategory == EUnequipped)
	{
		CRPG_Object_Item* pMelee = Char()->FindItemByType(CRPG_Object_Fist_ItemType);
		if (pMelee)
		{
			int PreviousWeaponId = _pCurItem->m_Identifier;
			int PreviousItemType = _pCurItem->m_iItemType;
			int ItemIndex = Char()->GetInventory(0)->FindItemIndexByIdentifier(pMelee->m_Identifier);
			int ItemIndexPrev = Char()->GetInventory(0)->FindItemIndexByIdentifier(_pCurItem->m_Identifier);
			pCD->m_iCurSelectedItem = ItemIndex;
			// Set current selected item
			// Select fist (go through ag and stuff...)
			if (pCD->m_iCurSelectedItem != ItemIndexPrev)
			{
				//Switch weapon
				CRPG_Object_Item* pItem = Char()->GetItemByIndex(0,pCD->m_iCurSelectedItem);
				if (pItem)
				{
					m_LastUsedItem = PreviousWeaponId;
					m_LastUsedItemType = PreviousItemType;

					// AG2 version
					pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER,pItem->m_Identifier);
					if (pCD->m_AnimGraph2.SendAttackImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_UNEQUIP)))
						pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED,true);
				}
			}
			pCD->m_WeaponUnequipTick = m_pWServer->GetGameTick();
			m_WeaponUnequipTimer = 0;
		}
	}
	else */if (iCurrCategory == EPistols)
	{
		// Ok then, Find our next guns...?
		CRPG_Object_Item* pNextItem = Char()->FindNextItemNotEquippedByGroupType(_pCurItem->m_Identifier,AG2_IMPULSEVALUE_GROUPTYPE_GUN,AG2_ITEMSLOT_WEAPONS,7,_bOnlyWeaponsWithAmmo,_pCurItem);
		if (pNextItem)
		{
			if (_pCurItem->m_AnimType != AG2_IMPULSEVALUE_WEAPONTYPE_GUN)
			{
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER,pNextItem->m_Identifier);
				if (pCD->m_AnimGraph2.SendAttackImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_UNEQUIP)))
					pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED,true);
				pCD->m_WeaponUnequipTick = 0;
			}
			else
			{
				// Already have a gun, "reequip a new one"
				pNextItem->m_Flags2 |= RPG_ITEM_FLAGS2_TAGGEDFORRELOAD;
				m_bLastChangeRight = !m_bLastChangeRight;
				pCD->m_WeaponUnequipTick = 0;
				ConOutL(CStrF("Reequipping: %d Tick: %d",m_bLastChangeRight,pCD->m_GameTick));
				if (m_bLastChangeRight)
				{
					pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER,pNextItem->m_Identifier);
					if (pCD->m_AnimGraph2.SendAttackImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_REEQUIPPRIMARY)))
						pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED,true);
				}
				else
				{	
					pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIERDUALWIELD,pNextItem->m_Identifier);
					if (pCD->m_AnimGraph2.SendAttackImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_REEQUIPSECONDARY)))
						pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUEDDUALWIELD,true);
				}
			}
		}
	}
	else if (iCurrCategory == ERifles)
	{
		// Ok then, find next shotgun
		CRPG_Object_Item* pNextItem = Char()->FindNextItemNotEquippedByAnimType(_pCurItem->m_Identifier,AG2_IMPULSEVALUE_WEAPONTYPE_RIFLE,AG2_ITEMSLOT_WEAPONS,-1,_pCurItem,true);
		if (pNextItem)
		{
			pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER,pNextItem->m_Identifier);
			if (pCD->m_AnimGraph2.SendAttackImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_UNEQUIP)))
				pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED,true);
			pCD->m_WeaponUnequipTick = 0;
		}
	}
}

bool CWObject_Character::Char_UpdateItemDescription(const char *_pName, const char* _pNewDesc, int16 _iNewIcon)
{
	// Find item in inventory and update it's description text
	CRPG_Object_Item* pItem = Char()->FindItemByName(_pName);
	if (!pItem)
	{
		// Add the item REMEMBER TO REMOVE ME WHEN GOLD PLZ K THX
		pItem = Char()->PickupItem(_pName,true,false);
		//return false;
	}
	if (!pItem)
		return false;

	// Set itemdesc
	pItem->m_ItemDescription = _pNewDesc;
	// Change icon
	if (_iNewIcon > 0 )
		pItem->m_JournalImage = _iNewIcon;
		//pItem->m_iIconSurface = _iNewIcon;

	// Set a new identifier so it's updated to the client
	Char()->UpdateItemIdentifier(pItem);
	Char_UpdateQuestItems();

	// when a missionitem is updated we need to tag that as a new item for the journal
	int iSlot = pItem->m_iItemType >> 8;
	CRPG_Object_Inventory* pInventory = Char()->GetInventory(iSlot);
	if(pInventory)
		pInventory->m_GUITaggedItemID = pItem->GetItemIdentifier();

	return true;
}

bool CWObject_Character::Char_AvailablePickup(const CWObject_Message &_Msg)
{
	MAUTOSTRIP(CWObject_Character_Char_AvailablePickup, false);
	CRPG_Object_Item *pDummy = (CRPG_Object_Item *)_Msg.m_Param1;
	if(pDummy && pDummy->m_Flags & RPG_ITEM_FLAGS_AUTOACTIVATE)
	{
		CMat4Dfp32 Mat;
		GetActivatePosition(Mat);
		pDummy->OnPickup(m_iObject, NULL, false, _Msg.m_iSender);
	 	if(Char()->QuickActivateItem(pDummy, Mat, m_iObject, 0))
			return true;
	}

	char *pName = (char *)_Msg.m_pData;
	if(!pName)
		return false;

	return Char_PickupItem(false, false, pName, _Msg.m_iSender, false, (_Msg.m_Reason == 1));
}

#ifdef _DEBUG
extern void DrawCircle(CWorld_PhysState* _pWPhys, CVec3Dfp32 _StartPos, int _Res, fp32 _Size, int _Dir,
			   int _Color = 0xffffffff);
enum
{
	CIRCLEDIR_XY,
	CIRCLEDIR_XZ,
	CIRCLEDIR_YZ,
};
#endif

#define ACTION_SELECTIONRADIUS (40.0f)
#define MAXPICKUPDISTANCE (70.0f)
#define CHARSELECT_MAXHEIGHTDIFF (20.0f)
bool CWObject_Character::Char_FindStuff(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, 
										CWO_Character_ClientData* _pCD, int32& _iSel, int32& _iCloseSel,
										int8& _SelType, bool& _bCurrentNoPrio, uint8 _SelectionMode, fp32 _SelectRadius, int32 _SelectType, CVec3Dfp32* _pSelectPos)
{
	MSCOPESHORT(CWObject_Character::Char_FindStuff);
	if (!_pWPhys || !_pCD || !_pObj)
		return false;

	{
		bool bReturn = false;
		switch (CWObject_Character::Char_GetControlMode(_pObj))
		{
		//case PLAYER_CONTROLMODE_FREE:
		case PLAYER_CONTROLMODE_LADDER:
		case PLAYER_CONTROLMODE_FORCEMOVE:
		//case PLAYER_CONTROLMODE_FIGHTING:
		case PLAYER_CONTROLMODE_HANGRAIL:
		//case PLAYER_CONTROLMODE_CARRYBODY:
		case PLAYER_CONTROLMODE_LEAN:
		case PLAYER_CONTROLMODE_LEDGE:
		//case PLAYER_CONTROLMODE_ANIMCONTROLLED:
		//case PLAYER_CONTROLMODE_ANIMATION:
		case PLAYER_CONTROLMODE_ACTIONCUTSCENE:
		case PLAYER_CONTROLMODE_EVENT:
		case PLAYER_CONTROLMODE_LEDGE2:
			{
				bReturn = true;
			}
		default:
			break;
		}

		// Fist make sure that we're not talking to anybody
		if (bReturn || (_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_DIALOGUE))
			return false;
	}

	// Check if we've already found stuff
	int FocusFrameType = _pCD->m_FocusFrameType & SELECTION_MASK_TYPEINVALID;
	bool bFocusFrame = _SelectionMode & FINDSTUFF_SELECTIONMODE_FOCUSFRAME;

	if (!bFocusFrame && (_pCD->m_iFocusFrameObject != -1) && 
		(!(_SelectionMode & FINDSTUFF_SELECTIONMODE_DEVOURING) || 
		(FocusFrameType == SELECTION_DEADCHAR)))
	{
		if (((_SelectType & OBJECT_FLAGS_CHARACTER) && ((FocusFrameType == SELECTION_CHAR) || 
			(FocusFrameType == SELECTION_STEALTHTARGET))) || 
			(!(_SelectType & OBJECT_FLAGS_CHARACTER) && ((FocusFrameType != SELECTION_CHAR) || 
			(FocusFrameType != SELECTION_STEALTHTARGET))))
		{
			_iSel = _pCD->m_iFocusFrameObject;
			_SelType = _pCD->m_FocusFrameType;

			return true;
		}
	}

//	bool bCrouch = (Char_GetPhysType(_pObj) == PLAYER_PHYS_CROUCH);

	CVec3Dfp32 CharDir = CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(),0);
	CVec3Dfp32 CharDirFlat = CharDir;
	CharDirFlat.k[2] = 0.0f;
	CharDirFlat.Normalize();
	const CVec3Dfp32 CharPos = _pObj->GetPosition();
	fp32 Radius = (_SelectRadius != -1 ? _SelectRadius : ACTION_SELECTIONRADIUS);
	fp32 HeightOffset = (Char_GetPhysType(_pObj) == PLAYER_PHYS_CROUCH ? (uint8)_pCD->m_Phys_HeightCrouch : (uint8)_pCD->m_Phys_Height); 
	CVec3Dfp32 SelectPosition = _pSelectPos ? *_pSelectPos : CharPos + CharDir * Radius * 0.75f + CVec3Dfp32(0,0,HeightOffset);
	//_pWPhys->Debug_RenderVertex(SelectPosition);
	int nSel = 0;
	const int16* pSel = NULL;

	TSelection<CSelection::LARGE_BUFFER> Selection;
	// Select only pickups for now...
	int32 SelectFlags = (_SelectType != 0 ? _SelectType : OBJECT_FLAGS_PICKUP | OBJECT_FLAGS_CHARACTER);
	_pWPhys->Selection_AddBoundSphere(Selection, SelectFlags, SelectPosition, Radius);
	// Add player bounding box to selection, so we can pickup what we're standing on
	const CBox3Dfp32* pBox = _pObj->GetAbsBoundBox();
	_pWPhys->Selection_AddBoundBox(Selection, SelectFlags,pBox->m_Min,pBox->m_Max);
	nSel = _pWPhys->Selection_Get(Selection, &pSel);
	// Reset close selection
	_iCloseSel = 0;
	int32 iCloseSelForce = 0;

	/*int32 DrawColor = (nSel > 1 ? 0xff00ff00 : 0xffff0000);
	DrawCircle(_pWPhys,SelectPosition,20,Radius,CIRCLEDIR_XY,DrawColor);
	DrawCircle(_pWPhys,SelectPosition,20,Radius,CIRCLEDIR_XZ,DrawColor);
	DrawCircle(_pWPhys,SelectPosition,20,Radius,CIRCLEDIR_YZ,DrawColor);*/

//	bool bAlreadyFighting = false;
	
	// Check if there were any pickups (or other stuff)....
	if (nSel > 0)
	{
		int32 iBest = -1;
		bool bCurrentNoPrio = true;
		fp32 BestDot = 0.0f;
//		int LadderType = 0;
		int8 Selection = SELECTION_NONE;

		// Is this direction (Positionmatrix 0 I mean), it seems to be....
		CVec3Dfp32 Direction = CharDir;
		//Direction.k[2] = 0.0f;
		//Direction.Normalize();

		for(int i = 0; i < nSel; i++)
		{
			// Don't select ourselves...
			if ((pSel[i] == _pObj->m_iObject))
				continue;

			// Got a pickup/fight/ladder/ledge/whatever, lets see it we can use it
			CWObject_CoreData* pObject = _pWPhys->Object_GetCD(pSel[i]);
			bool bIsObject = (pObject->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_OBJECT) != 0;
			bool bIsChar = pObject->IsClass(class_CharPlayer) || pObject->IsClass(class_CharNPC) || pObject->IsClass(class_Telephoneregistry) || pObject->IsClass(class_CharNPC_P6);
			// If it's a character it has priority at this time
			if (bIsChar)
			{
				bool bIsDead = _pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ISDEAD),pSel[i]) != 0;
				bool bHasHeartLeft = false;
				if(pObject->IsClass(class_CharNPC))
				{
					CWObject_Character* pChar = (CWObject_Character*)pObject;
					if (pChar->HasHeart())
						bHasHeartLeft = true;
				}

				CVec3Dfp32 MyPos = CharPos;
				CVec3Dfp32 Opponent = pObject->GetPosition();
				fp32 HeightDiff = M_Fabs(Opponent.k[2] - CharPos.k[2]);

				CXR_SkeletonInstance* pOtherSkelInst = NULL;
				CXR_Skeleton* pOtherSkel = NULL;
				CXR_AnimState Anim;
				if (GetEvaluatedPhysSkeleton(pObject, _pWPhys, pOtherSkelInst, pOtherSkel, Anim) 
					&& (pOtherSkelInst->m_nBoneTransform >= PLAYER_ROTTRACK_HEAD))
				{
					if (_SelectionMode & FINDSTUFF_SELECTIONMODE_DEVOURING)
					{
						Opponent = pOtherSkel->m_lNodes[PLAYER_ROTTRACK_SPINE].m_LocalCenter;
						Opponent *= pOtherSkelInst->m_pBoneTransform[PLAYER_ROTTRACK_SPINE];
					}
					else
					{
						Opponent = pOtherSkel->m_lNodes[PLAYER_ROTTRACK_HEAD].m_LocalCenter;
						Opponent *= pOtherSkelInst->m_pBoneTransform[PLAYER_ROTTRACK_HEAD];
					}
					MyPos = CharPos + Camera_GetHeadPos(_pObj); // yuck!
				}

				// Make sure height difference isn't too much either
				fp32 Length = (Opponent - MyPos).Length();
				CVec3Dfp32 DirToObj = (Opponent - MyPos).Normalize();
				fp32 Dot = (DirToObj * Direction);

				// Instead of using focusframe when close, if we don't find something else
				// this will be it
				_iCloseSel = pSel[i];
				
				if (bFocusFrame && !pObject->IsClass(class_Telephoneregistry))
				{
					// Extra test for "hard check", ie must look almost directly at object
					if ((bIsDead || Dot < 0.6) && ((_SelectionMode & FINDSTUFF_SELECTIONMODE_DEVOURING) == 0))
						Dot = -1;
				}

				//AR-HACK -- make sure people with dialogue gets priority over other things
				bool bHaveChoices = (_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETCHOICES,0,0,_pObj->m_iObject), pSel[i]) > 0);
				if (bHaveChoices)
					Dot += 1.1f;

				//AR-HACK2 -- make sure dead people gets priority when devouring
				if (bIsDead && (_SelectionMode & FINDSTUFF_SELECTIONMODE_DEVOURING))
					Dot += 1.2f;

				if (((Dot > BestDot || bCurrentNoPrio) && (Length < 80.0f) && 
					((HeightDiff < CHARSELECT_MAXHEIGHTDIFF) || pObject->IsClass(class_Telephoneregistry))))
				{
					// Make a trace to make sure the other person is not behind glass or 
					// some other stuff, a wall for example
					CCollisionInfo CInfo;
					CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
					CInfo.SetReturnValues(0);

					int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
					int32 ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;// | OBJECT_FLAGS_PLAYERPHYSMODEL;
					int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
					int32 iExclude = _pObj->m_iObject;

					CVec3Dfp32 Center, CenterOpponent;
					_pObj->GetAbsBoundBox()->GetCenter(Center);
					pObject->GetAbsBoundBox()->GetCenter(CenterOpponent);
					{ // Get closest point on opponents boundbox
						fp32 d = pObject->GetAbsBoundBox()->GetMinDistance(Center);
						CenterOpponent = Center + (CenterOpponent - Center).SetLength(d);
					}

					bool bHit = _pWPhys->Phys_IntersectLine(Center, CenterOpponent, OwnFlags, ObjectFlags, MediumFlags, &CInfo, iExclude);

					// mode against this character
//					CWObject_CoreData* pObj = (CInfo.m_bIsValid ? _pWPhys->Object_GetCD(CInfo.m_iObject) : NULL);

/*#ifndef M_RTM
					int32 Color = (!bHit || (bHit && pObj && (pSel[i] == CInfo.m_iObject)) ? 0xff00ff00 : 0xffff0000);
					_pWPhys->Debug_RenderWire(Center, CenterOpponent, Color, 60.0f);
#endif*/
					if (!bHit || (bHit && (pSel[i] == CInfo.m_iObject)) || (bHit && bHaveChoices))
					//	(pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER))))
					{
						if (!bIsDead)
						{
							// Found a suitable candidate
							iBest = pSel[i];
							BestDot = Dot;
							bCurrentNoPrio = false;
							iCloseSelForce = iBest;

							Selection = SELECTION_CHAR;
						}
						else if (bIsDead && (((_SelectionMode & FINDSTUFF_SELECTIONMODE_DEVOURING) && bHasHeartLeft) || 
							(CWObject_Character::Char_GetPhysType(_pObj) == PLAYER_PHYS_CROUCH)))
						{
							// Pickup character perhaps... 
							iBest = pSel[i];
							BestDot = Dot;
							bCurrentNoPrio = false;
							Selection = SELECTION_DEADCHAR;
						}
					}
				}
			}
			else if (bIsObject || 
			         pObject->IsClass(class_LampSwitch) || pObject->IsClass(class_UseProxy) || 
			         pObject->IsClass(class_DarklingSpawn)) // add other objects that can handle OBJMSG_USE here...
			{
				CVec3Dfp32 ObjPos;
				pObject->GetAbsBoundBox()->GetCenter(ObjPos);

				const CVec3Dfp32& ObjDir = CVec3Dfp32::GetRow(pObject->GetPositionMatrix(), 0);
				fp32 DirCheck = -(ObjDir * Direction);
				if (DirCheck > 0.1f || pObject->IsClass(class_DarklingSpawn) || bIsObject)
				{
					CVec3Dfp32 HeadPos = CharPos + CVec3Dfp32(0, 0, HeightOffset);
					CVec3Dfp32 CharToObject = ObjPos - HeadPos;
					fp32 Length = CharToObject.Length();
					CharToObject *= (1.0f / Length);
					fp32 Dot = CharToObject * Direction;
					if ((Dot > BestDot || bCurrentNoPrio) && ((Length < 64.0f) || pObject->IsClass(class_DarklingSpawn)))
					{
						CCollisionInfo CInfo;
						bool bHit = _pWPhys->Phys_IntersectLine(HeadPos, ObjPos, 
							OBJECT_FLAGS_PROJECTILE, OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL, 
							XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS, &CInfo, _pObj->m_iObject);

						if (!bHit || (bHit && (pSel[i] == CInfo.m_iObject)))
						{
							if (pObject->IsClass(class_UseProxy))
							{
								if(pObject->m_iAnim2 != 0)
								{
									CWObject_CoreData *pReal = _pWPhys->Object_GetCD(pObject->m_iAnim2);
									if (pReal->IsClass(class_CharPlayer) || pReal->IsClass(class_CharNPC))
										Selection = SELECTION_CHAR;
									else
										Selection = SELECTION_ACTIONCUTSCENE;

									Selection |= SELECTION_FLAG_PROXY;
									BestDot = Dot;
									bCurrentNoPrio = false;
									iBest = pObject->m_iAnim2;
								}
							}
							else if (pObject->IsClass(class_DarklingSpawn))
							{
								BestDot = Dot;
								bCurrentNoPrio = false;
								iBest = pSel[i];
								Selection = SELECTION_DARKLINGSPAWN;
							}
							else if (bIsObject && !pObject->IsClass(class_Television))
							{
								CWObject_Message Msg(OBJMSG_RPG_ISPICKUP);
								int8 Type = SELECTION_GRABBABLE_OBJECT;
								if (_pWPhys->Phys_Message_SendToObject(Msg, pSel[i]))
								{
									// It's a pickup
									Type = SELECTION_PICKUP;
								}
								const CVec3Dfp32* pAttachPoints = NULL;
								Msg.m_Msg = OBJMSG_OBJECT_GET_GRABATTACHPOINTS;
								Msg.m_pData = &pAttachPoints;
								uint nPoints = _pWPhys->Phys_Message_SendToObject(Msg, pSel[i]);
								if (nPoints)
								{ // only grab phys-objects that have attachpoints
									BestDot = Dot;
									bCurrentNoPrio = false;
									iBest = pSel[i];
									Selection = Type;
								}
							}
							else
							{
								BestDot = Dot;
								bCurrentNoPrio = false;
								iBest = pSel[i];
								Selection = SELECTION_USABLEOBJECT;
							}
						}
					}
				}
			}
			else
			{
				bool bIsPickup = _pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_RPG_ISPICKUP,0,0,
					_pObj->m_iObject),pSel[i]) != 0;	

				int ACSType = _pWPhys->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_ACTIONCUTSCENE_GETTYPE,0,0,	_pObj->m_iObject),pSel[i]);

				bool bIsWaitSpawned = _pWPhys->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_ACTIONCUTSCENE_ISWAITSPAWNED),pSel[i]) != 0;
				if ((pObject && !bIsWaitSpawned && (ACSType == ACTIONCUTSCENE_TYPE_RIOT)))
				{
					// Add height so we look from the head
					CVec3Dfp32 HeadOffset = ((CWObject_Character::Char_GetPhysType(_pObj) == PLAYER_PHYS_CROUCH) ? 
						CVec3Dfp32(0, 0, 31) : CVec3Dfp32(0, 0, 62));

					CVec3Dfp32 CharHeadPos = CharPos + HeadOffset;
					CVec3Dfp32 ItemPosition = pObject->GetPosition();
					ItemPosition.k[2] += 40.0f;
					CVec3Dfp32 ItemDirection = ItemPosition - CharHeadPos;
					fp32 Length = ItemDirection.Length();
					ItemDirection = ItemDirection / Length;

					fp32 DirValue = ItemDirection * CharDir; //(Length * Length);
					// Check so that the item is in the correct x-y, direction   
					//ItemDirection.k[2] = 0.0f;
					ItemDirection.Normalize();
					fp32 Dot = ItemDirection * Direction;

					// Extra test for "hard check", ie must look almost directly at object
					if (bFocusFrame && DirValue < 0.6f)
						Dot = -1;

					bool bNoPrio = false;
					int32 ItemAnimType = 0;
					int32 ItemType = _pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENE_GETPICKUPITEMTYPE,0,0,0,0,0,0,&ItemAnimType,sizeof(ItemAnimType)),pSel[i]);	
					bNoPrio = !_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_CANPICKUPITEM,ItemType),pSel[i]);

					// If this is noprio and we already have a prio continue with other items
					if (bNoPrio && !bCurrentNoPrio)
						continue;
					// Make us select the one we are looking closest at (dirvalue)
					if (((!bNoPrio && (BestDot == 0.0f||bCurrentNoPrio)) || 
						((Dot > 0.5f) && ((DirValue > BestDot)||bCurrentNoPrio))) && Length < MAXPICKUPDISTANCE)
					{
						// Make a trace to make sure the ACS isn't behind anything
						// some other stuff, a wall for example
						CCollisionInfo CInfo;
						CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
						CInfo.SetReturnValues(0);

						int32 OwnFlags = 0;
						int32 ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;
						int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
						int32 iExclude = _pObj->m_iObject;

						//Trace destination is center of objects bounding box...
						CVec3Dfp32 CenterObject;
						pObject->GetAbsBoundBox()->GetCenter(CenterObject);
						//...moved slightly towards head position to hanlde near wall/floor objects etc
						CVec3Dfp32 Dir = CenterObject - CharHeadPos;
						Dir.Normalize();
						CenterObject -= Dir * 2.0f;
						//CenterObject -= CVec3Dfp32::GetMatrixRow(pObject->GetPositionMatrix(),0);

						bool bHit = _pWPhys->Phys_IntersectLine(CharHeadPos, CenterObject, OwnFlags, 
							ObjectFlags, MediumFlags, &CInfo, iExclude);
						if (!bHit || (bHit && CInfo.m_iObject == pObject->m_iObject))
						{
							// Found a suitable candidate
							iBest = pSel[i];
							BestDot = DirValue;
							bCurrentNoPrio = bNoPrio;
							Selection = SELECTION_ACTIONCUTSCENE;
						}
					}
				}
				else if ((pObject && !bIsWaitSpawned && (bIsPickup || (ACSType == ACTIONCUTSCENE_TYPE_ACS))))
				{
					// Add height so we look from the head
					CVec3Dfp32 HeadOffset = ((CWObject_Character::Char_GetPhysType(_pObj) == PLAYER_PHYS_CROUCH) ? 
						CVec3Dfp32(0, 0, 31) : CVec3Dfp32(0, 0, 62));

					CVec3Dfp32 CharHeadPos = CharPos + HeadOffset;
					CVec3Dfp32 ItemPosition = pObject->GetPosition();
					CVec3Dfp32 ItemDirection = ItemPosition - CharHeadPos;
					fp32 Length = ItemDirection.Length();
					ItemDirection = ItemDirection / Length;

					fp32 DirValue = ItemDirection * CharDir; //(Length * Length);
					// Check so that the item is in the correct x-y, direction   
					//ItemDirection.k[2] = 0.0f;
					ItemDirection.Normalize();
					fp32 Dot = ItemDirection * Direction;

					// Extra test for "hard check", ie must look almost directly at object
					if (bFocusFrame && DirValue < 0.6f)
						Dot = -1;

					bool bNoPrio = false;
					int32 ItemAnimType = 0;
					_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENE_GETPICKUPITEMTYPE,0,0,0,0,0,0,&ItemAnimType,sizeof(ItemAnimType)),pSel[i]);

					// If this is noprio and we already have a prio continue with other items
					if (bNoPrio && !bCurrentNoPrio)
						continue;
					// Make us select the one we are looking closest at (dirvalue)
					if (((!bNoPrio && (BestDot == 0.0f||bCurrentNoPrio)) || 
						((Dot > 0.5f) && ((DirValue > BestDot)||bCurrentNoPrio))) && Length < 100.0f)
					{
						// Make a trace to make sure the ACS isn't behind anything
						// some other stuff, a wall for example
		 				CCollisionInfo CInfo;
						CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
						CInfo.SetReturnValues(0);

						int32 OwnFlags = 0;
						int32 ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;
						int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
						int32 iExclude = _pObj->m_iObject;

						//Trace destination is center of objects bounding box...
						CVec3Dfp32 CenterObject;
						pObject->GetAbsBoundBox()->GetCenter(CenterObject);
						//...moved slightly towards head position to hanlde near wall/floor objects etc
						CVec3Dfp32 Dir = CenterObject - CharHeadPos;
						Dir.Normalize();
						CenterObject -= Dir * 2.0f;
						//CenterObject -= CVec3Dfp32::GetMatrixRow(pObject->GetPositionMatrix(),0);

						bool bHit = _pWPhys->Phys_IntersectLine(CharHeadPos, CenterObject, OwnFlags, 
							ObjectFlags, MediumFlags, &CInfo, iExclude);

						if (!bHit || (bHit && CInfo.m_iObject == pObject->m_iObject))
						{
#ifndef M_RTM
							_pWPhys->Debug_RenderWire(CharHeadPos, CenterObject, 0xff00ff00, 1.0f);
#endif

							// Found a suitable candidate
							iBest = pSel[i];
							BestDot = DirValue + (bIsPickup ? 0.4f : 0.0f);
							bCurrentNoPrio = bNoPrio;
							if (bCurrentNoPrio)
								bCurrentNoPrio = !bIsPickup;

							if (ACSType == ACTIONCUTSCENE_TYPE_ACS)// || (ACSType == ACTIONCUTSCENE_TYPE_PICKUP))
							{
								// Check if the actioncutscene can be activated
								if (_pWPhys->Phys_Message_SendToObject(
									CWObject_Message(OBJMSG_ACTIONCUTSCENE_CANACTIVATE,2,0,
									_pObj->m_iObject),pSel[i]))
								{
									Selection = SELECTION_ACTIONCUTSCENE;
								}
								else
								{
									Selection = SELECTION_ACTIONCUTSCENELOCKED;
								}
							}
							else if (bIsPickup || (ACSType == ACTIONCUTSCENE_TYPE_PICKUP))
								Selection = SELECTION_PICKUP;
							else if (CWObject_Character::Char_GetPhysType(_pObj) == PLAYER_PHYS_CROUCH)
								Selection = SELECTION_DEADCHAR; // Dead person
						}
#ifndef M_RTM
						else
						{
							_pWPhys->Debug_RenderWire(CharHeadPos, CenterObject, 0xffff0000, 60.0f);
						}
#endif
					}
				}
				else if ((Selection != SELECTION_ACTIONCUTSCENE) && 
					(Selection != SELECTION_ACTIONCUTSCENELOCKED) &&
					(Selection != SELECTION_PICKUP) && !bIsWaitSpawned)// && !bFocusFrame)
				{
					switch (ACSType)
					{
					case ACTIONCUTSCENE_TYPE_LADDER: 
						{
							// Make sure the character is "close" and facing the ladder before making him climb
							CVec3Dfp32 LadderNormal;
							CWObject_Message Msg(OBJMSG_LADDER_GETNORMAL, 0,0, _pObj->m_iObject, 0, 0, 
								0, &LadderNormal, sizeof(CVec3Dfp32));

							// Make sure we actually got a ladder
							if (_pWPhys->Phys_Message_SendToObject(Msg, pSel[i]))
							{
								CVec3Dfp32 Pos1, Pos2;

								_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION1,
									0,0,_pObj->m_iObject, 0,0,0,&Pos1, sizeof(CVec3Dfp32)), pSel[i]);
								_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION2,
									0,0,_pObj->m_iObject, 0,0,0,&Pos2, sizeof(CVec3Dfp32)), pSel[i]);

//								int EndPoint = _pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_FINDENDPOINT,0,
//									_pCD->m_Phys_Height,_pObj->m_iObject, 0,CharPos), pSel[i]);

								CVec3Dfp32 LadderDir;

								LadderDir = Pos2 - Pos1;
								LadderDir.Normalize();

								CVec3Dfp32 CharBottom = _pObj->GetPosition();
								CVec3Dfp32 PointOnLadder = Pos1 + LadderDir * ((CharBottom - Pos1) * LadderDir);
								int Len = (int)(PointOnLadder - CharBottom).LengthSqr();
								fp32 OverTop = (PointOnLadder - Pos2).k[2];

								// Make sure we are sufficiently close to the ladder and facing it 
								// from the right direction
								if (((OverTop < -10.0f) && ((Len < (50.0f * 50.0f)) && (CharDirFlat * LadderNormal < -0.5f))) || 
									((OverTop > -10.0f) && (Len < (50.0f * 50.0f)) && (CharDirFlat * LadderNormal > 0.5f)))
								{
									// Make a trace to make sure the other person is not behind glass or 
									// some other stuff, a wall for example
									CCollisionInfo CInfo;
									CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
									CInfo.SetReturnValues(0);

									int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
									int32 ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;
									int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
									int32 iExclude = _pObj->m_iObject;

									CVec3Dfp32 CharCenter = _pObj->GetPosition();
									CharCenter.k[2] += 20.0f;

									// Offset a little
									fp32 LadderDot = (PointOnLadder - Pos1) * LadderDir;
									PointOnLadder += LadderNormal;
									if (LadderDot < 0)
										PointOnLadder.k[2] = Pos1.k[2];
									bool bHit = _pWPhys->Phys_IntersectLine(CharCenter, PointOnLadder, 
										OwnFlags, ObjectFlags, MediumFlags, &CInfo, iExclude);
									/*{
									// REMOVE 
									CVec3Dfp32 Dir = CharCenter - PointOnLadder;
									Dir.Normalize();
									if (bHit)
									_pWPhys->Debug_RenderWire(PointOnLadder,PointOnLadder + Dir * 100,0xffff0000,10.0f);
									else
									_pWPhys->Debug_RenderWire(PointOnLadder,PointOnLadder + Dir * 100,0xff00ff00,10.0f);
									}*/

									if (!bHit)
									{
										BestDot = Max(LadderDot,-LadderDot);
										bCurrentNoPrio = false;
										iBest = pSel[i];
										Selection = SELECTION_LADDER;
									}
								}
							}
							break;
						}
					case ACTIONCUTSCENE_TYPE_HANGRAIL:
						{
							CVec3Dfp32 Pos1, Pos2, HangRailNormal;

							_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION1,
								0,0,_pObj->m_iObject, 0,0,0,&Pos1, sizeof(CVec3Dfp32)), pSel[i]);
							_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION2,
								0,0,_pObj->m_iObject, 0,0,0,&Pos2, sizeof(CVec3Dfp32)), pSel[i]);
							_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETNORMAL, 
								0,0, _pObj->m_iObject, 0, 0, 0, &HangRailNormal, sizeof(CVec3Dfp32)), pSel[i]);

							CVec3Dfp32 HangRailDir;

							HangRailDir = Pos2 - Pos1;
							HangRailDir.Normalize();

							fp32 LadderOffset;
							*(int32*)&LadderOffset = _pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_FINDENDPOINTOFFSET,
								0,0,_pObj->m_iObject, 0,CharPos), pSel[i]);

							CVec3Dfp32 PointOnHangRail = Pos1 + HangRailDir * ((CharPos - Pos1) * HangRailDir);
							fp32 Len = (PointOnHangRail - CharPos).LengthSqr();
							fp32 Dot = M_Fabs(CharDirFlat * HangRailDir);

							// Make sure we're hanging under the hangrail as well
							fp32 HeightDiff = PointOnHangRail.k[2] - CharPos.k[2];

							// Make sure we are sufficiently close to the hangrail
							if ((Len < (110.0f * 110.0f)) && 
								(HeightDiff > _pCD->m_Phys_Height) && (M_Fabs(LadderOffset) < 40.0f))
							{
								// Found a suitable candidate
								BestDot = Dot;
								bCurrentNoPrio = false;
								iBest = pSel[i];
								Selection = SELECTION_HANGRAIL;
							}
							break;
						}
					default:
						break;
					}
				}
			}
		}

		_iCloseSel = (iCloseSelForce ? iCloseSelForce : _iCloseSel);
		_iSel = iBest;
		_bCurrentNoPrio = bCurrentNoPrio;

		_SelType = Selection;
		if(BestDot <= 0.0f)
			_SelType |= SELECTION_FLAG_INVALID;
		//_SelType = (BestDot > 0.0f ? Selection : Selection + SELECTION_BADOFFSET);
	}

	return (_iSel != -1);
}

bool CWObject_Character::Char_FindBestOpponent(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, int32& _iSel, fp32 _SelectRadius)
{
	MSCOPESHORT(CWObject_Character::Char_FindStuff);
	CWO_Character_ClientData* pCD = GetClientData(_pObj);

	CVec3Dfp32 CharDir = CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(),0);
	CVec3Dfp32 CharDirFlat = CharDir;
	CharDirFlat.k[2] = 0.0f;
	CharDirFlat.Normalize();
	const CVec3Dfp32 CharPos = _pObj->GetPosition();
	fp32 Radius = (_SelectRadius != -1 ? _SelectRadius : ACTION_SELECTIONRADIUS);
	fp32 HeightOffset = (Char_GetPhysType(_pObj) == PLAYER_PHYS_CROUCH ? (uint8)pCD->m_Phys_HeightCrouch : (uint8)pCD->m_Phys_Height); 
	CVec3Dfp32 SelectPosition = CharPos + CharDir * Radius * 0.75f + CVec3Dfp32(0,0,HeightOffset);
	//_pWPhys->Debug_RenderVertex(SelectPosition);
	int nSel = 0;
	const int16* pSel = NULL;

	TSelection<CSelection::LARGE_BUFFER> Selection;
	// Select only pickups for now...
	int32 SelectFlags = OBJECT_FLAGS_CHARACTER;
	_pWPhys->Selection_AddBoundSphere(Selection, SelectFlags, SelectPosition, Radius);
	nSel = _pWPhys->Selection_Get(Selection, &pSel);
	// Reset close selection
	int32 iCloseSelForce = 0;
	int iClassCharPlayer = _pWPhys->GetMapData()->GetResourceIndex_Class("CharPlayer");
	int iClassCharNPC = _pWPhys->GetMapData()->GetResourceIndex_Class("CharNPC");	

	//	bool bAlreadyFighting = false;

	// Check if there were any pickups (or other stuff)....
	if (nSel > 0)
	{
		int32 iBest = -1;
		bool bCurrentNoPrio = true;
		fp32 BestDot = 0.0f;
		//		int LadderType = 0;
		int8 Selection = SELECTION_NONE;

		// Is this direction (Positionmatrix 0 I mean), it seems to be....
		CVec3Dfp32 Direction = CharDir;
		//Direction.k[2] = 0.0f;
		//Direction.Normalize();

		CVec3Dfp32 PlayerHead = Camera_GetHeadPos(_pObj); 
		CVec3Dfp32 Center;
		_pObj->GetAbsBoundBox()->GetCenter(Center);

		for(int i = 0; i < nSel; i++)
		{
			// Don't select ourselves...
			if ((pSel[i] == _pObj->m_iObject))
				continue;

			// Got a pickup/fight/ladder/ledge/whatever, lets see it we can use it
			CWObject_CoreData* pObject = _pWPhys->Object_GetCD(pSel[i]);
			bool bIsChar = pObject->IsClass(class_CharPlayer) || pObject->IsClass(class_CharNPC);
			// If it's a character it has priority at this time
			if (bIsChar)
			{
				if (Char_GetPhysType(pObject) == PLAYER_PHYS_DEAD)
					continue;

				CVec3Dfp32 MyPos = CharPos;		
				CVec3Dfp32 Opponent = pObject->GetPosition();
				fp32 HeightDiff = M_Fabs(Opponent.k[2] - CharPos.k[2]);

				CXR_SkeletonInstance* pOtherSkelInst = NULL;
				CXR_Skeleton* pOtherSkel = NULL;
				CXR_AnimState Anim;
				if (GetEvaluatedPhysSkeleton(pObject, _pWPhys, pOtherSkelInst, pOtherSkel, Anim) 
					&& (pOtherSkelInst->m_nBoneTransform >= PLAYER_ROTTRACK_HEAD))
				{
					Opponent = pOtherSkel->m_lNodes[PLAYER_ROTTRACK_HEAD].m_LocalCenter;
					Opponent *= pOtherSkelInst->m_pBoneTransform[PLAYER_ROTTRACK_HEAD];
					MyPos += PlayerHead;
				}

				// Make sure height difference isn't too much either
				fp32 Length = (Opponent - MyPos).Length();
				CVec3Dfp32 DirToObj = (Opponent - MyPos).Normalize();
				fp32 Dot = (DirToObj * Direction);

				if (((Dot > BestDot || bCurrentNoPrio) && (Length < 80.0f) && 
					((HeightDiff < CHARSELECT_MAXHEIGHTDIFF))))
				{
					// Make a trace to make sure the other person is not behind glass or 
					// some other stuff, a wall for example
					CCollisionInfo CInfo;
					CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
					CInfo.SetReturnValues(0);

					int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
					int32 ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;// | OBJECT_FLAGS_PLAYERPHYSMODEL;
					int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
					int32 iExclude = _pObj->m_iObject;

					CVec3Dfp32 CenterOpponent;
					pObject->GetAbsBoundBox()->GetCenter(CenterOpponent);
					{ // Get closest point on opponents boundbox
						fp32 d = pObject->GetAbsBoundBox()->GetMinDistance(Center);
						CenterOpponent = Center + (CenterOpponent - Center).SetLength(d);
					}

					bool bHit = _pWPhys->Phys_IntersectLine(Center, CenterOpponent, OwnFlags, ObjectFlags, MediumFlags, &CInfo, iExclude);

					if (!bHit || (bHit && (pSel[i] == CInfo.m_iObject)))
					{
						// Found a suitable candidate
						iBest = pSel[i];
						BestDot = Dot;
						bCurrentNoPrio = false;
						iCloseSelForce = iBest;
					}
				}
			}
		}

		_iSel = iBest;
	}

	return (_iSel != -1);
}

bool CWObject_Character::Char_ShowInFocusFrame(int8 _SelType, int _iObj)
{
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if(!pCD)
		return false;
	
	CFStr UseText;// = "Pickup moj";
	CFStr DescText;// = "Hallååå dääär det är jag som är Bengt!";
	switch (_SelType & ~SELECTION_FLAG_PROXY)
	{
	case SELECTION_DEADCHAR:
	case SELECTION_CHAR:
		{
			CWObject_Character* pChar = TDynamicCast<CWObject_Character>(m_pWServer->Object_Get(_iObj));
			CWO_Character_ClientData* pCharCD = (pChar) ? CWObject_Character::GetClientData(pChar) : NULL;
			CAI_Core* pAI = (pChar) ? pChar->AI_GetAI() : NULL;
			bool bHasHeartLeft = (pChar && pChar->HasHeart());
			bool bIsDead = _SelType == SELECTION_DEADCHAR;
			bool bIsConscious = (pAI && pAI->IsConscious());
			bool bIsStunned = (pCharCD && (pCharCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_STUNNED) != 0);
			bool bIsDevouring = (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DEVOUR) != 0;
			if (!bIsDevouring && bHasHeartLeft && (bIsDead || !bIsConscious || bIsStunned))
			{
				UseText = "§LACS_DEVOUR";
				break;
			}
			else if (m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETAIPRIORITYCLASS),_iObj) > CAI_Action::PRIO_ALERT)
				break;
		}
	case SELECTION_PICKUP:
	case SELECTION_PICKUP + SELECTION_FLAG_INVALID:
	case SELECTION_ACTIONCUTSCENELOCKED:
	//case SELECTIONISACTIONCUTSCENELOCKED_BAD:
	//case SELECTIONISDEADCHAR_BAD:
	//case SELECTIONISCHAR_BAD:
		{
			if (!m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENE_CANACTIVATE,2,0,m_iObject), _iObj))
			{
				// Check if there's any description text
				CWObject_Message Msg = CWObject_Message(OBJMSG_CHAR_GETDESCNAME);
				Msg.m_iSender = m_iObject;
				Msg.m_pData = (void*)&DescText;
				m_pWServer->Message_SendToObject(Msg, _iObj);
				Msg = CWObject_Message(OBJMSG_CHAR_GETFOCUSOFFSET);
				pCD->m_FocusFrameOffset = (int8)m_pWServer->Message_SendToObject(Msg, _iObj);
			}
		}
	case SELECTION_ACTIONCUTSCENE:
	//case SELECTIONISACTIONCUTSCENE_BAD:
		{
			CWObject_Message Msg(OBJMSG_CHAR_GETUSENAME);
			Msg.m_pData = (void *)&UseText;
			if(!m_pWServer->Message_SendToObject(Msg, _iObj))
			{
				_SelType = 0;
				_iObj = -1;
				pCD->m_FocusFrameOffset = 0;
			}
			else
			{
				Msg = CWObject_Message(OBJMSG_CHAR_GETFOCUSOFFSET);
				pCD->m_FocusFrameOffset = (int8)m_pWServer->Message_SendToObject(Msg, _iObj);
			}
		}
		break;
	case SELECTION_LADDER:
	//case SELECTIONISLADDER_BAD:
		UseText = "§LACS_LADDER"; break;
	case SELECTION_LEDGE:
	//case SELECTIONISLEDGE_BAD:
		UseText = "§LACS_LEDGE"; break;
	case SELECTION_HANGRAIL:
	//case SELECTIONISHANGRAIL_BAD:
		UseText = "§LACS_HANGRAIL"; break;
	case SELECTION_NONE:
	default:
		_SelType = 0;
		_iObj = -1;
	};

	pCD->m_FocusFrameType = _SelType;
	pCD->m_iFocusFrameObject = (int32)_iObj;
	if(UseText != (CFStr)pCD->m_FocusFrameUseText)
	{
		pCD->m_FocusFrameUseText.m_Value = UseText;
		pCD->m_FocusFrameUseText.MakeDirty();
	}
	if(DescText != (CFStr)pCD->m_FocusFrameDescText)
	{
		pCD->m_FocusFrameDescText.m_Value = DescText;
		pCD->m_FocusFrameDescText.MakeDirty();
	}
	
	return _iObj != 0;
}

bool CWObject_Character::Char_ActivateStuff(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, 
											CWO_Character_ClientData* _pCD, const int32& _iSel, 
											const int8& _SelType)
{
	MSCOPESHORT(CWObject_Character::Char_ActivateStuff);
	// Ok, check if an item was found, if so add "pickupitem" fist action
	if (_iSel != -1)
	{
		switch (_SelType & SELECTION_MASK_TYPE)
		{
		case SELECTION_ACTIONCUTSCENELOCKED:
		case SELECTION_ACTIONCUTSCENE:
			{
				// Hmm, ok then send a message to the action cutscene and hope for the best
				_pWPhys->Phys_Message_SendToObject(
					CWObject_Message(OBJMSG_ACTIONCUTSCENE_ACTIVATE,0,0,_pObj->m_iObject), 
					_iSel);
				break;
			}
		case SELECTION_PICKUP:
			{
				// Just use standard "enclave" pickup (for now...)
				_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_RPG_AVAILABLEPICKUP, 
					_pObj->m_iObject, 0, _iSel), _iSel);
				break;
			}
		case SELECTION_LADDER:
		case SELECTION_HANGRAIL:
			{
				CWObject_Phys_Ladder::GrabLadder(_iSel, _SelType, _pWPhys,_pObj, _pCD);
				break;
			}
		/*case SELECTION_DEADCHAR:
			{
				// Pickup dead character.....
				// Should switch controlmode (so animgraph can see it)
				Char_SetControlMode(_pObj,PLAYER_CONTROLMODE_CARRYBODY);
				// Pickup the body as well, set carrybody info somewhere
				_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETGRABBEDBODY,
					_iSel), _pObj->m_iObject);
				break;
			}*/

		case SELECTION_USABLEOBJECT:
			_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_USE), _iSel);
			break;

		default:
			{
				return false;
			}
		};
	}

	return true;
}

bool CWObject_Character::Char_GrabLedge(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj,
										CWO_Character_ClientData* _pCD)
{
	// Hmm, didn't find anything, so lets try to find a ledge instead yes?
	CVec3Dfp32 Direction = CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(),0);
	fp32 Range = 60.0f;//LEDGE_DEFAULTACTIVATIONRANGE;
	fp32 Width = (fp32)_pCD->m_Phys_Width * 2.0f;
	int iBestLedge,BestLedgeType;
	fp32 LedgePos;
	int iLedge = CWObject_Ledge::FindBestLedge(_pWPhys, _pObj->GetPosition(), _pObj->m_iObject, 
		Direction, Range, _pCD->m_Phys_bInAir != 0,Width,iBestLedge,BestLedgeType,LedgePos);
	
	if (iLedge != -1)
	{
		// Found a ledge, now grab it
		CWObject_Message LedgeMsg(OBJMSG_LEDGE_GRABLEDGE,BestLedgeType,iBestLedge,
			0,0,CVec3Dfp32(LedgePos,0,0),0,_pObj);
		_pWPhys->Phys_Message_SendToObject(LedgeMsg,iLedge);
		_pWPhys->Phys_Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENE_ACTIVATE,0,0,
			_pObj->m_iObject), iLedge);
		
		return true;
	}

	return false;
}

// What we need to do here is to find an opponent and smack him around a bit
#define FIGHT_DIRECTIONSTRICTNESS (0.5f)
#define FIGHT_DISTANCESTRICTNESS (55.0f)
/*#define DAMAGE_BASIC (8)
#define DAMAGE_MIDDLE (16)
#define DAMAGE_SPECIAL_UNARMED (24)
#define DAMAGE_SPECIAL_SHANK (32)
#define DAMAGE_SPECIAL_CLUB (32)*/

int CWObject_Character::OnUseInformation(const CWObject_Message &_Msg)
{
	MAUTOSTRIP(CWObject_Character_OnUseInformation, 0);
//	CRPG_Object *pObj = CreateWorldUseRPGObject((const char *)_Msg.m_pData);
	const char* pName = (const char *)_Msg.m_pData;
	if (pName == NULL)
		pName = "use_generic";

	CRPG_Object_Item* pUseItem = Char()->GetInventory(RPG_CHAR_INVENTORY_ITEMS)->FindItemByName(pName);
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if (pUseItem == NULL || !pCD)
		return 0;

	m_Player.m_iTemporaryItemLastTick = pCD->m_GameTick;
	m_Player.m_iTemporaryItemType = pUseItem->m_iItemType;
	m_Player.m_iTemporaryItemSender = _Msg.m_iSender;

	return 1;
}

void CWObject_Character::Char_UpdateQuestItems()
{
	MAUTOSTRIP(CWObject_Character_Char_UpdateQuestItems, MAUTOSTRIP_VOID);
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if(!pCD)
		return;

	bool bFoundMap = false;

//	int nQuestItems = 0;
	int nInv = Char()->GetNumChildren();
	int i;
	int iItCntr = 0;
	pCD->m_InventoryInfo.Clear();
	for(i = 0; i < nInv; i++)
	{
		CRPG_Object_Inventory *pInv = Char()->GetInventory(i);
		for(int j = 0; j < pInv->GetNumItems(); j++)
		{
			CRPG_Object_Item *pItem = pInv->GetItemByIndex(j);

			if(pItem->m_iItemType == m_Player.m_Map_ItemType)
				bFoundMap = true;

			/// WTF???
			//pItem = Char()->FindItemByType(pItem->m_iItemType);

			if(pItem && (pItem->m_Flags & RPG_ITEM_FLAGS_QUESTITEM))
			{
				// now this was one way of doing it, lets instead try to parse out "555" from description
				//if(pItem->GetItemName().Find("PHONENUMBER") != -1)
				//	pCD->m_InventoryInfo.AddItem(pItem, pCD->m_GameTick, CWO_Inventory_Info::INVENTORYINFO_TYPE_PHONEBOOK);
				if(pItem->GetMaxAmmo())
				{
					// check if can dualwield
					if((pItem->m_Flags2 & RPG_ITEM_FLAGS2_CLONEATTACHITEM)) // or "is-dual-wieldable"
					{
						pCD->m_InventoryInfo.AddItem(pItem,pCD->m_GameTick, CWO_Inventory_Info::INVENTORYINFO_SUBTYPE_WEAPON_GUN);
					}
					else
					{
						pCD->m_InventoryInfo.AddItem(pItem,pCD->m_GameTick, CWO_Inventory_Info::INVENTORYINFO_SUBTYPE_WEAPON_RIFLE);
					}
				}
				else
				{
					int Identifier = pItem->GetItemIdentifier();
					if(Identifier == pInv->m_GUITaggedItemID)
					{
						pCD->m_InventoryInfo.m_iGUITaggedItem = pInv->m_GUITaggedItemID;
						pInv->m_GUITaggedItemID = -1;
					}

					CStr CheckString = pItem->GetItemDesc();
					CheckString = CheckString.UpperCase();
					CheckString = Localize_Str(CheckString).Ansi();
					if(CheckString.Find("555") != -1)
						pCD->m_InventoryInfo.AddItem(pItem, pCD->m_GameTick, CWO_Inventory_Info::INVENTORYINFO_TYPE_PHONEBOOK);
					else
						pCD->m_InventoryInfo.AddItem(pItem,pCD->m_GameTick, CWO_Inventory_Info::INVENTORYINFO_TYPE_INVENTORY);
					//pCD->m_InventoryInfo.AddItem(pItem,pCD->m_GameTick, i == 3 ? CWO_Inventory_Info::INVENTORYINFO_TYPE_PHONEBOOK : CWO_Inventory_Info::INVENTORYINFO_TYPE_INVENTORY);	
				}
				iItCntr++;
			}
			// Phonebook items in RPG_CHAR_INVENTORY_QUESTITEMS
			/*if ((i == RPG_CHAR_INVENTORY_QUESTITEMS) && pItem)
			{
				pCD->m_InventoryInfo.AddItem(pItem,pCD->m_GameTick,CWO_Inventory_Info::INVENTORYINFO_TYPE_PHONEBOOK);
			}*/
		}
	}

	if(bFoundMap)
		pCD->m_Map_iSurface = pCD->m_Map_iSurface | 0x8000;
	else
		pCD->m_Map_iSurface = pCD->m_Map_iSurface & (~0x8000);

/*	if(nQuestItems < 1) pCD->m_Pickup_iQuestIcon0 = 0;
	if(nQuestItems < 2) pCD->m_Pickup_iQuestIcon1 = 0;
	if(nQuestItems < 3) pCD->m_Pickup_iQuestIcon2 = 0;
	if(nQuestItems < 4) pCD->m_Pickup_iQuestIcon3 = 0;
	if(nQuestItems < 5) pCD->m_Pickup_iQuestIcon4 = 0;
	if(nQuestItems < 6) pCD->m_Pickup_iQuestIcon5 = 0;*/
}

//Part of Sammes message testing system
/*
TArray<SMessageCounter> g_slMessages;
void PrintMessageLog(int _iTick)
{
	TArray<SMessageCounter> SortedLog;
	for(int i = 0; i <g_slMessages.Len(); ++i)
	{
		int Count = g_slMessages[i].m_ServerCount + g_slMessages[i].m_ClientCount;
		int j;
		for(j = 0; j <SortedLog.Len(); ++j)
		{
			if(SortedLog[j].m_ServerCount + SortedLog[j].m_ClientCount < Count)
			{
				break;
			}
		}
		SortedLog.Insert(j, g_slMessages[i]);
	}
	ConOutL("=========================MESSAGE LOG=================================");
	ConOutL(CStr("NAME\t\t\tID\t\t\tServerCalls(PerTick)\t\tClientCalls(PerTick)"));		
	for(int j = 0; j <SortedLog.Len(); ++j)
	{
		fp32 SCallsPerTick = fp32(SortedLog[j].m_ServerCount)/fp32(_iTick); 
		fp32 CCallsPerTick = fp32(SortedLog[j].m_ClientCount)/fp32(_iTick); 
		ConOutL(CStrF("\t\t\t0x%X\t\t\t%d(%f)\t\t\t%d(%f)",SortedLog[j].m_iMsg,SortedLog[j].m_ServerCount,SCallsPerTick,SortedLog[j].m_ClientCount,CCallsPerTick));		
	}
	ConOutL("=====================END MESSAGE LOG=================================");
}
*/
void CWObject_Character::Char_Die(uint32 _Method, int _iKilledBy, int _iDeathAnim)
{
	MAUTOSTRIP(CWObject_Character_Char_Die, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_Character::Char_Die);

	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if (!pCD) return;

/*	if(pCD->m_iPlayer != -1)
		PrintMessageLog(m_pWServer->GetGameTick());*/
	if (Char_GetPhysType(this) == PLAYER_PHYS_DEAD) return;

	// Notify weapons
	Char()->OnCharDeath(m_iObject);

	// Make sure the AG updates for atleast another tick
	pCD->m_AnimGraph2.RegisterImportantAGEvent();
	CWAG2I_Context Context(this,m_pWServer,pCD->m_GameTime);
	pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&Context,CXRAG2_Impulse(AG2_IMPULSETYPE_RESPONSE,AG2_IMPULSEVALUE_RESPONSE_DEAD),0);

	if(pCD->m_iMountedObject)
		Char_Mount(NULL, 0);

	Char_SetPhysics(this, m_pWServer, m_pWServer, PLAYER_PHYS_DEAD);
	OnShutOffAllLights();

	m_DropItemTick = pCD->m_GameTick + 2;

	CXR_AnimState Anim;
	CXR_Skeleton* pSkel = NULL; 
	CXR_SkeletonInstance* pSkelInstance = NULL;					

	CRPG_Object_Item *pItem = Char()->GetEquippedItem(0);
	if(pItem &&  !(pItem->m_Flags & RPG_ITEM_FLAGS_NOPICKUP))
	{
		m_DropItemTick += pItem->GetDropItemOffset();
		CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
		CMat4Dfp32 Mat;
		CMTime GameTime = CMTime::CreateFromTicks(pCD->m_GameTick, m_pWServer->GetGameTickTime(), pCD->m_PredictFrameFrac);
		if(GetEvaluatedPhysSkeleton(pSkelInstance, pSkel, Anim))
		{
			if(pCD->m_Item0_Model.IsValid() && pCD->m_Item0_Model.GetModel0_RenderInfo(m_pWServer->GetMapData(), NULL, pSkelInstance, pSkel, GameTime, 
				Anim, Mat, pModel, m_pWServer))
			{
				m_spRagdollDropItems[0] = MNew(CConstraintRigidObject);
				m_spRagdollDropItems[0]->SetOrgMat(Mat);
				CXR_Model *pItemModel = m_pWServer->GetMapData()->GetResource_Model(pCD->m_Item0_Model.m_iModel[0]);
				SConstraintSystemSettings Settings;
				Settings.m_SkeletonType	= SConstraintSystemSettings::RAGDOLL_NONE;
				Settings.m_pSkelInstance = pSkelInstance;
				m_spRagdollDropItems[0]->SetCollisionSound(pItem->m_iSound_Dropped,false);
				m_spRagdollDropItems[0]->StartCollecting(m_pWServer, pItemModel, &Settings);
				m_spRagdollDropItems[0]->Activate(true);
				m_spRagdollDropItems[0]->CollectFrame(false,Mat);
				m_spRagdollDropItems[0]->Animate(m_pWServer->GetGameTick(), Mat);
			}
		}
		pItem = Char()->GetEquippedItem(1);
		if(pItem && !(pItem->m_Flags & RPG_ITEM_FLAGS_NOPICKUP))
		{
			if(pCD->m_Item0_Model.IsValid() && pCD->m_Item0_Model.GetModel0_RenderInfo(m_pWServer->GetMapData(), NULL, pSkelInstance, pSkel, GameTime, 
				Anim, Mat, pModel, m_pWServer))
			{
				m_spRagdollDropItems[1] = MNew(CConstraintRigidObject);
				m_spRagdollDropItems[1]->SetOrgMat(Mat);
				CXR_Model *pItemModel = m_pWServer->GetMapData()->GetResource_Model(pCD->m_Item1_Model.m_iModel[0]);
				SConstraintSystemSettings Settings;
				Settings.m_SkeletonType	= SConstraintSystemSettings::RAGDOLL_NONE;
				Settings.m_pSkelInstance = pSkelInstance;
				m_spRagdollDropItems[1]->SetCollisionSound(pItem->m_iSound_Dropped,false);
				m_spRagdollDropItems[1]->StartCollecting(m_pWServer, pItemModel, &Settings);
				m_spRagdollDropItems[1]->Activate(true);
				m_spRagdollDropItems[1]->CollectFrame(false,Mat);
				m_spRagdollDropItems[1]->Animate(m_pWServer->GetGameTick(), Mat);
			}
		}
	}

	if (m_lGibInfos.Len() && !pSkel && !pSkelInstance)
		GetEvaluatedPhysSkeleton(pSkelInstance, pSkel, Anim);

	//Char_DeathDropItem();

	if(m_spDropItem)
	{
		bool bOk = false;
		if (!m_spRagdollDropItems[1])
		{
			CRPG_Object_Item* pItem = (CRPG_Object_Item*)(CRPG_Object*)m_spDropItem;
			CMTime GameTime = CMTime::CreateFromTicks(pCD->m_GameTick, m_pWServer->GetGameTickTime(), pCD->m_PredictFrameFrac);
			CMat4Dfp32 Mat;
			CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
			if(pCD->m_Item0_Model.IsValid() && pCD->m_Item0_Model.GetModel0_RenderInfo(m_pWServer->GetMapData(), NULL, pSkelInstance, pSkel, GameTime, 
				Anim, Mat, pModel, m_pWServer))
			{
				(Mat.GetRow(3) + CVec3Dfp32(1.0f,2.0f,2.5f)).SetMatrixRow(Mat,3);
				m_spRagdollDropItems[1] = MNew(CConstraintRigidObject);
				m_spRagdollDropItems[1]->SetOrgMat(Mat);
				CXR_Model *pItemModel = m_pWServer->GetMapData()->GetResource_Model(pItem->m_Model.m_iModel[0]);
				SConstraintSystemSettings Settings;
				Settings.m_SkeletonType	= SConstraintSystemSettings::RAGDOLL_NONE;
				Settings.m_pSkelInstance = pSkelInstance;
				m_spRagdollDropItems[1]->SetCollisionSound(pItem->m_iSound_Dropped,false);
				m_spRagdollDropItems[1]->StartCollecting(m_pWServer, pItemModel, &Settings);
				m_spRagdollDropItems[1]->Activate(true);
				m_spRagdollDropItems[1]->CollectFrame(false,Mat);
				m_spRagdollDropItems[1]->Animate(m_pWServer->GetGameTick(), Mat);
				bOk = true;
			}
		}
		if (!bOk)
		{
			CMat4Dfp32 Mat = GetPositionMatrix();
			Mat.k[3][2] += 16;
			int iObj = m_pWServer->Object_Create("ActionCutscenePickup", Mat);
			if(iObj > 0)
			{
				m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENEPICKUP_SETDEFAULTS),iObj);

				int nMessages;
				const CWO_SimpleMessage* pMsg = m_MsgContainer.GetMessages(PLAYER_SIMPLEMESSAGE_ONPICKUPDROPITEM, nMessages);
				CWObject_Message Msg(OBJMSG_RPG_REPLACEPICKUP, nMessages, aint(pMsg));
				Msg.m_pData = (CRPG_Object *)m_spDropItem;
				m_pWServer->Message_SendToObject(Msg, iObj);	
			}
		}
	}

	// Clear weapon animation effects
	{
		for (int iModel = 0; iModel < 4; iModel++)
		{
			pCD->SetItemModelFlag(0, iModel, 0);
			pCD->SetItemModelFlag(1, iModel, 0);
		}

		pCD->m_Item0_Model.AC_SetTarget(CMTime(), 0, CMTime());
		pCD->m_Item1_Model.AC_SetTarget(CMTime(), 0, CMTime());
	}

	if(m_Flags & PLAYER_FLAGS_RAGDOLL)
	{
#if 0
		if (!(m_spRagdoll && m_spRagdoll->IsActive()))
		{
			// To be sure we have a skeletoninstance when creating the ragdoll
			CXR_AnimState Anim;
			CXR_Skeleton* pSkel;
			CXR_SkeletonInstance* pSkelInstance;					
			if(GetEvaluatedPhysSkeleton(pSkelInstance, pSkel, Anim))
			{
                CXR_Model* lpModels[CWO_NUMMODELINDICES];
				for (uint i = 0; i < CWO_NUMMODELINDICES; i++)
				{
					m_RagdollSettings.m_lpClothSkeleton[i] = NULL;
					if (m_iModel[i] > 0)
					{
						lpModels[i] = m_pWServer->GetMapData()->GetResource_Model(m_iModel[i]);
						if(lpModels[i])
							m_RagdollSettings.m_lpClothSkeleton[i] = lpModels[i]->GetSkeleton();
					}
				}

				if (!m_spRagdoll)
				{
					m_spRagdoll = MNew(CConstraintSystem);
				}
				m_RagdollSettings.m_pSkelInstance = pSkelInstance;
				m_RagdollSettings.m_pSkeleton = pSkel;

				CVec3Dfp32 Start, End;
				Start = pSkel->m_lNodes[PLAYER_ROTTRACK_ROOT].m_LocalCenter * pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_ROOT];
				End = Start;
				Start.k[2] += 30.0f;
				End.k[2] -= 100.0f;

				CCollisionInfo CInfo;
				int32 ObjectFlags = 0;
				int32 ObjectIntersectionFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;
				int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;

				bool bHit = m_pWServer->Phys_IntersectLine(Start, End, ObjectFlags, ObjectIntersectionFlags, MediumFlags, &CInfo, m_iObject);
				if(bHit)
				{
					int32 iMaterial = CInfo.m_SurfaceType;
					if(iMaterial == 0 && CInfo.m_pSurface && CInfo.m_pSurface->GetBaseFrame())
						iMaterial = CInfo.m_pSurface->GetBaseFrame()->m_MaterialType;
					m_spRagdoll->SetMaterialSound(iMaterial, this, m_pWServer);
				}
				else
					m_spRagdoll->SetMaterialSound(10, this, m_pWServer);

				m_spRagdoll->SetOrgMat(GetPositionMatrix());
				m_spRagdoll->Init(m_iObject, this, m_pWServer, pCD, &pCD->m_RagdollClient);
				m_spRagdoll->Setup(&m_RagdollSettings);
				m_spRagdoll->Activate(true);
				m_RagdollSettings.m_pSkelInstance = NULL;
				for(uint i = 0; i < CWO_NUMMODELINDICES; i++)
					m_RagdollSettings.m_lpClothSkeleton[i] = NULL;
			}
		}
#else
		if( !m_pPhysCluster )
			Char_ActivateRagdoll(pCD);
#endif
		pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_RAGDOLLACTIVE;
	}

	pCD->m_DeathTick = pCD->m_GameTick;
	
	if (IsBot())
	{	// Must tell the ai we killed him!
		m_spAI->OnTakeDamage(_iKilledBy,Char()->MaxHealth()+1,_Method);
		// When should the corpse vanish?
		if (m_Flags & PLAYER_FLAGS_AUTODESTROY)
		{
			pCD->m_DestroyTick = pCD->m_GameTick + int(CORPSE_DURATION * m_pWServer->GetGameTicksPerSecond());
		}
	}
	
	m_pWServer->Message_SendToTarget(CWObject_Message(OBJMSG_GAME_CHARACTERKILLED, m_iObject, 0 + (_Method << 16), _iKilledBy), (char*)WSERVER_GAMEOBJNAME);
	//m_RespawnTimeoutTicks = 30;
 	
	ClientFlags() |= (PLAYER_CLIENTFLAGS_NOLOOK | PLAYER_CLIENTFLAGS_NOMOVE);

	//Remove any static sounds
	for (int i = 0;  i < CWO_NUMSOUNDINDICES; i++)
		m_iSound[i] = 0;

	// Set turncorrection to zero
	pCD->m_TurnCorrectionLastAngle = pCD->m_TurnCorrectionTargetAngle;
	pCD->m_TurnCorrectionAngleChange = 0.0f;

	if(pCD->m_iPlayer != -1)
	{
		// Reset pending cutscenes
		m_PendingCutsceneCamera = -1;
		m_PendingCutsceneTick = -1;
		m_PendingTimeLeapTick = -1;
		m_PendingCutsceneType = -1;
		Char_EndTimeLeap();
	}

	m_MsgContainer.SendMessage(PLAYER_SIMPLEMESSAGE_ONDIE, m_iObject, m_iObject, m_pWServer);

	// Gib the motherfucker!!!
	if(m_lGibInfos.Len() && pSkelInstance)
	{
		// Remove all models
		int i;
		for(i = 0; i < CWO_NUMMODELINDICES; ++i)
			m_iModel[i] = 0;
		CMat4Dfp32 Pos = GetPositionMatrix();
		CMat4Dfp32 GibPos;
		int iGibSystem = m_pWServer->Object_Create("GibSystem", Pos);
		CWObject* pGibSystem = m_pWServer->Object_Get(iGibSystem);
		if (pGibSystem)
		{
			
/*			// School-book example of retarded code:

			CWObject_Message Msg(OBJMSG_GIB_ADDPART);
			int nToSpawn = m_GibExplosionParams[2];
			int nInfo = m_lGibInfos.Len();
			if(nToSpawn == 0)
				nToSpawn = nInfo;
			for(i = 0; i < nInfo; ++i)
			{
				if(Random < (fp32)nToSpawn / (nInfo - i))
				{
					m_lGibInfos[i].m_Pos.Multiply(Pos, GibPos);
					Msg.m_Param0 = m_lGibInfos[i].m_iModel;
					Msg.m_Param1 = (int)&GibPos;
					m_pWServer->Message_SendToObject(Msg, iGibSystem);
					nToSpawn--;
				}
			}
			// Finally, send the explosion params
			CWObject_Message Msg2(OBJMSG_GIB_EXPLOSION);
			Msg2.m_VecParam0 = m_GibExplosionOrigin;
			Msg2.m_VecParam1 = m_GibExplosionParams;
			m_pWServer->Message_SendToObject(Msg2,iGibSystem);
*/

			const int MaxGib = 32;
			CGibInfo lGibAnimated[MaxGib];

			int nGibs = Min(MaxGib, m_lGibInfos.Len());
			const CGibInfo* pGibs = m_lGibInfos.GetBasePtr();

			CMat4Dfp32 GibSystemInv;
			Pos.InverseOrthogonal(GibSystemInv);

			for(int i = 0; i < nGibs; i++)
			{
				lGibAnimated[i].m_iModel = pGibs[i].m_iModel;
				CMat43fp32 Mat;
				CVec3Dfp32 Angles(fp32(pGibs[i].m_Angles[0]) / 256.0f, fp32(pGibs[i].m_Angles[1]) / 256.0f, fp32(pGibs[i].m_Angles[2]) / 256.0f);
				Angles.CreateMatrixFromAngles(0, Mat);
				Mat.k[3][0] = pGibs[i].m_Pos[0];
				Mat.k[3][1] = pGibs[i].m_Pos[1];
				Mat.k[3][2] = pGibs[i].m_Pos[2];

				if((pGibs[i].m_iNode != (uint8)~0) && (pGibs[i].m_iNode < pSkelInstance->m_nBoneTransform))
				{
					CMat43fp32 Temp;
					Mat.Multiply(pSkelInstance->m_pBoneTransform[pGibs[i].m_iNode], Temp);
//					pSkelInstance->m_pBoneTransform[pGibs[i].m_iNode].Multiply(Mat, Temp);
					Temp.Multiply(GibSystemInv, Mat);
				}

				// .. do anim stuff ..

				Angles.CreateAnglesFromMatrix(0, Mat);
				lGibAnimated[i].m_Angles[0] = RoundToInt(Angles[0] * 256.0f);
				lGibAnimated[i].m_Angles[1] = RoundToInt(Angles[1] * 256.0f);
				lGibAnimated[i].m_Angles[2] = RoundToInt(Angles[2] * 256.0f);
				lGibAnimated[i].m_Pos[0] = (int16)Mat.k[3][0];
				lGibAnimated[i].m_Pos[1] = (int16)Mat.k[3][1];
				lGibAnimated[i].m_Pos[2] = (int16)Mat.k[3][2];
			}


			CWObject_Message Msg(OBJMSG_GIB_INITIALIZE);
			Msg.m_pData = lGibAnimated;
			Msg.m_DataSize = sizeof(CGibInfo) * m_lGibInfos.Len();
			Msg.m_Param0 = m_lGibInfos.Len();
			Msg.m_VecParam0 = m_GibExplosionOrigin;
			Msg.m_VecParam1 = m_GibExplosionParams;
			pGibSystem->OnMessage(Msg);
		}

		//Let AI know it died in the usual fashion
		if (m_spAI)
		{
			m_spAI->OnDie();
		}	
		Destroy();
	}
	else if (m_bHideWhenDead)
	{
		//Let AI know it died in the usual fashion
		if (m_spAI)
		{
			m_spAI->OnDie();
		}
		Destroy();
	}

	m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
}

bool CWObject_Character::SelectItem(int _iItemType, bool _bInstant)
{
	MAUTOSTRIP(CWObject_Character_SelectItem, false);
	return Char()->SelectItem(_iItemType, true, true, SELECTIONMETHOD_NORMAL, _bInstant);
}

bool CWObject_Character::SelectItemByIdentifier(int _iSlot, int _Identifier, bool _bInstant)
{
	MAUTOSTRIP(CWObject_Character_SelectItemByIdentifier, false);
	return Char()->SelectItemByIdentifier(_Identifier, _iSlot,true, true, SELECTIONMETHOD_NORMAL, _bInstant);
}

bool CWObject_Character::ForceSelectItem(int _iItemType)
{
	MAUTOSTRIP(CWObject_Character_ForceSelectItem, false);
	return Char()->SelectItem(_iItemType, true, true, SELECTIONMETHOD_FORCE);
}

bool CWObject_Character::ForceActivateItem(int _iItemType)
{
	MAUTOSTRIP(CWObject_Character_ForceActivateItem, false);
	return Char()->SelectItem(_iItemType, true, true, SELECTIONMETHOD_FORCEACTIVATE);
}

void CWObject_Character::NextItem(int _iSlot, bool _bReverse, bool _bInstant)
{
	MAUTOSTRIP(CWObject_Character_NextItem, MAUTOSTRIP_VOID);
	Char()->CycleSelectedItem(_iSlot, _bReverse, true, _bInstant);
}

int CWObject_Character::OnUpdateItemModel(int _iSlot, int _iTick)
{
	MAUTOSTRIP(CWObject_Character_OnUpdateItemModel, 0);
	MSCOPESHORT(CWObject_Character::OnUpdateItemModel);
	// Update item model from highest estimate (probaly unequip animation)
	if(_iTick != -1)
	{
		if (m_iPendingItemModelUpdate[_iSlot] < _iTick)
			m_iPendingItemModelUpdate[_iSlot] = _iTick;
		return true;
	}

	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if(!pCD)
		return false;

	CRPG_Object_Char* pChar = Char();
	if (pChar != NULL)
	{
		CRPG_Object_Inventory* pInventory = pChar->GetInventory(_iSlot);
		if (pInventory != NULL)
		{
			pInventory->OnUpdateModel();
		}
	}

	CRPG_Object_Item *pItem = Char()->GetFinalSelectedItem(_iSlot);
	if(pItem)
	{
		uint32 MuzzleLightColor = 0;
		uint32 MuzzleLightRange = 0;
		bool bHasLightningLight = pItem->HasLightningLight();
		pItem->GetMuzzleProperties(MuzzleLightColor, MuzzleLightRange);
		

		if(_iSlot == 0)
		{
//			pCD->m_Item0_Model.SCopyFrom(&pCD->m_Item0_Model, &pItem->m_Model);
			pCD->m_Item0_Model.UpdateModel(pItem->m_Model);
			if (pItem->m_iFirstAttachPoint != -1)
				pCD->m_Item0_Model.m_iAttachPoint[0] = pItem->m_iFirstAttachPoint;
			pCD->m_Item0_Model.MakeDirty();
			pCD->m_Item0_Flags = (uint8)pItem->m_Flags;
			pCD->m_Item0_AnimSound0 = (uint16)pItem->m_iSound_Anim[0];
			pCD->m_Item0_AnimSound1 = (uint16)pItem->m_iSound_Anim[1];
			pCD->m_Item0_AnimSound2 = (uint16)pItem->m_iSound_Anim[2];
			pCD->m_Item0_AnimSound3 = (uint16)pItem->m_iSound_Anim[3];
			pCD->m_Item0_Icon = (uint16)pItem->m_iIconSurface;

			// Weapons can have flashlights, and should therefor update the visibilty flag
			UpdateVisibilityFlag();

			//Char_SetAnimStance(this, pItem->m_iAnimStance);
			pCD->m_MuzzleLightColor0 = MuzzleLightColor;
			pCD->m_MuzzleLightRange0 = MuzzleLightRange;
			pCD->m_LightningLight = pCD->m_LightningLight & ~M_Bit(0);
			pCD->m_LightningLight = pCD->m_LightningLight | ((bHasLightningLight) ? M_Bit(0) : 0);
		}
		else
		{
			//pCD->m_Item1_Model.SCopyFrom(&pCD->m_Item1_Model, &pItem->m_Model);
			pCD->m_Item1_Model.UpdateModel(pItem->m_Model);
			if (pItem->m_iFirstAttachPoint != -1)
				pCD->m_Item0_Model.m_iAttachPoint[0] = pItem->m_iFirstAttachPoint;
			pCD->m_Item1_Model.MakeDirty();
			pCD->m_Item1_Flags = (uint8)pItem->m_Flags;
			pCD->m_Item1_AnimSound0 = (uint16)pItem->m_iSound_Anim[0];
			pCD->m_Item1_AnimSound1 = (uint16)pItem->m_iSound_Anim[1];
			pCD->m_Item1_AnimSound2 = (uint16)pItem->m_iSound_Anim[2];
			pCD->m_Item1_AnimSound3 = (uint16)pItem->m_iSound_Anim[3];
			pCD->m_Item1_Icon = (uint16)pItem->m_iIconSurface;

			pCD->m_MuzzleLightColor1 = MuzzleLightColor;
			pCD->m_MuzzleLightRange1 = MuzzleLightRange;
			pCD->m_LightningLight = pCD->m_LightningLight & ~M_Bit(1);
			pCD->m_LightningLight = pCD->m_LightningLight | ((bHasLightningLight) ? M_Bit(1) : 0);
		}
	}
	else
	{
		if(_iSlot == 0)
		{
//			ConOut("Clearing Slot0");
			pCD->m_Item0_Model.Clear();
			pCD->m_Item0_Model.MakeDirty();

			pCD->m_Item0_Flags = 0;
			pCD->m_Item0_Icon = 0;
		}
		else
		{
			CRPG_Object_Item *pItem = Char()->GetFinalSelectedItem(AG2_ITEMSLOT_DUALWIELD);
			if (!pItem)
			{
	//			ConOut("Clearing Slot1");
				pCD->m_Item1_Model.Clear();
				pCD->m_Item1_Model.MakeDirty();

				pCD->m_Item1_Flags = 0;
				pCD->m_Item1_Icon = 0;
			}
		}
	}
	
	return true;
}

/*void CWObject_Character::Char_AddTilt(const CVec3Dfp32& _Axis, fp32 _Angle)
{
	MAUTOSTRIP(CWObject_Character_Char_AddTilt, MAUTOSTRIP_VOID);
	CAxisRotfp32 AxisRot(_Axis, _Angle);
	CMat4Dfp32 Mat;
	CVec3Dfp32 Angles;
	AxisRot.CreateMatrix(Mat);
	Angles.CreateAnglesFromMatrix(0, Mat);

	CNetMsg Msg(PLAYER_NETMSG_TILT);
	Msg.AddInt8(Angles[0]*1024);
	Msg.AddInt8(Angles[1]*1024);
	Msg.AddInt8(Angles[2]*1024);
	m_pWServer->NetMsg_SendToObject(Msg, m_iObject);
}*/

void CWObject_Character::Char_SpawnCutscene(int _CameraObject, bool _bInstant, int _Type)
{
	MAUTOSTRIP(CWObject_Character_Char_SpawnCutscene, MAUTOSTRIP_VOID);
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if (!pCD)
		return;

	if(Char_GetPhysType(this) == PLAYER_PHYS_DEAD)
		return;

	int EFlags = pCD->m_ExtraFlags;
	if(_Type & PLAYER_CUTSCENE_NOBORDER)
		pCD->m_ExtraFlags = EFlags | PLAYER_EXTRAFLAGS_CSNOBORDER;
	else
		pCD->m_ExtraFlags = EFlags & ~PLAYER_EXTRAFLAGS_CSNOBORDER;

	if(!(_Type & PLAYER_CUTSCENE_NOSKIP))
	{
		char *pWndName = "ingame_cutscene";

		{
			CWObject *pCam = m_pWServer->Object_Get(_CameraObject);
			if(pCam && CFStr(pCam->GetName()) == "ENDCAM1" && m_pWServer->m_WorldName == "pa3_finalbattle")
			{
				pWndName = "ingame_cutscene_outtro";
				MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
				if(pSys)
				{
					pSys->GetRegistry()->SetValuei("OPTG\\GAME_COMMENTARY", 1);
					ConExecute("option_update()");
				}
			}
		}

		CWObject_Message Msg(OBJMSG_GAME_SETCLIENTWINDOW, aint(pWndName), m_pWServer->Game_GetObject()->Player_GetClient(pCD->m_iPlayer));
		Msg.m_iSender = m_iObject;
		m_pWServer->Message_SendToTarget(Msg, (char*)WSERVER_GAMEOBJNAME);
	}
	Char_SetCutsceneCamera(_CameraObject);

	if(!_bInstant)
	{
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_FADESCREEN, -500), m_pWServer->Game_GetObjectIndex());
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_FADESOUND, -500), m_pWServer->Game_GetObjectIndex());
	}

	{
		TSelection<CSelection::LARGE_BUFFER> Selection;
		m_pWServer->Selection_AddAll(Selection);
		const int16 *pSel;
		int nSel = m_pWServer->Selection_Get(Selection, &pSel);
		CWObject_Message Msg(OBJMSG_GAME_NOTIFYCUTSCENE);
		for(int i = 0; i < nSel; i++)
			m_pWServer->Message_SendToObject(Msg, pSel[i]);
	}
}

bool CWObject_Character::Char_BeginCutscene(int _CameraObject, bool _bInstant, int _Type)
{
	MAUTOSTRIP(CWObject_Character_Char_BeginCutscene, false);
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if (!pCD)
		return false;

	//AO: Don't run cutscenes on non-players (should this be handled differently?)
	if (pCD->m_iPlayer == -1)
		return false;

	if(Char_GetPhysType(this) == PLAYER_PHYS_DEAD)
		return false;
		
	//End dialogue, if any. This might have averse effects, but hopefully 
	//not more so than allowing dialogue run parallell to cutscene
	if (ClientFlags() & PLAYER_CLIENTFLAGS_DIALOGUE)
	{
		Char_EndDialogue();
	}

	// Drop draggedbody (if any)
	m_pWServer->Phys_Message_SendToObject(
		CWObject_Message(OBJMSG_CHAR_SETGRABBEDBODY,0), m_iObject);
	// Deactivate weapons
	CMat4Dfp32 Mat;
	GetActivatePosition(Mat);
	Char()->DeactivateItems(0,Mat,m_iObject,1);
	Char()->DeactivateItems(0,Mat,m_iObject,2);

	if(pCD->m_iMountedObject != 0)
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_FORCERELEASEMOUNT), pCD->m_iMountedObject);
	
	if(_bInstant)
	{
		Char_SpawnCutscene(_CameraObject, _bInstant, _Type);
		m_PendingCutsceneCamera = -1;
		m_PendingCutsceneTick = -1;
		m_PendingTimeLeapTick = -1;
		m_PendingCutsceneType = -1;
	}
	else if(m_PendingCutsceneTick != -1 && m_PendingCutsceneTick > pCD->m_GameTick)
	{	
		m_PendingCutsceneCamera = _CameraObject;
		m_PendingCutsceneType = _Type;
	}
	else if(m_ClientFlags & PLAYER_CLIENTFLAGS_CUTSCENE)
	{
		int EFlags = pCD->m_ExtraFlags;
		if(_Type & PLAYER_CUTSCENE_NOBORDER)
			pCD->m_ExtraFlags = EFlags | PLAYER_EXTRAFLAGS_CSNOBORDER;
		else
			pCD->m_ExtraFlags = EFlags & ~PLAYER_EXTRAFLAGS_CSNOBORDER;
		if(!(_Type & PLAYER_CUTSCENE_NOSKIP))
		{
			char *pWndName = "ingame_cutscene";
			CWObject_Message Msg(OBJMSG_GAME_SETCLIENTWINDOW, aint(pWndName), m_pWServer->Game_GetObject()->Player_GetClient(pCD->m_iPlayer));
			Msg.m_iSender = m_iObject;
			m_pWServer->Message_SendToTarget(Msg, (char*)WSERVER_GAMEOBJNAME);
		}

		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_FADESCREEN, -500), m_pWServer->Game_GetObjectIndex());
		Char_SetCutsceneCamera(_CameraObject);
	}
	else
	{
		uint32 Fade = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETSCREENFADE), m_pWServer->Game_GetObjectIndex());
		if((Fade >> 24) < 200 && _CameraObject > 0)
		{
			m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_FADESCREEN, 300), m_pWServer->Game_GetObjectIndex());
			m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_FADESOUND, 300), m_pWServer->Game_GetObjectIndex());
			m_PendingCutsceneTick = pCD->m_GameTick + int(0.3f * m_pWServer->GetGameTicksPerSecond()) + 2; // Marigin for fade
			m_PendingCutsceneCamera = _CameraObject;
			m_PendingCutsceneType = _Type;

		}
		else	
			Char_SpawnCutscene(_CameraObject, false, _Type);
	}
	
	//AI controlled players should not set "nomove" and "nolook" flags
	if (!m_bAIControl)
		ClientFlags() |= PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK;
	
	PauseAllAI(CAI_ResourceHandler::PAUSE_CUTSCENE);
	
	return true;
}

bool CWObject_Character::Char_SetCutsceneCamera(int _CameraObject)
{
	MAUTOSTRIP(CWObject_Character_Char_SetCutsceneCamera, false);
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if (!pCD)
		return false;

	pCD->m_Cutscene_Camera = 0;
	if(_CameraObject > 0)
	{
		pCD->m_Cutscene_Camera = (uint16)_CameraObject;
		ClientFlags() |= PLAYER_CLIENTFLAGS_CUTSCENE;
		ConExecute("cl_darknessvision(0)");
	}
	else
	{
		ClientFlags() &= ~PLAYER_CLIENTFLAGS_CUTSCENE;
	}
	
	
	return true;
}

bool CWObject_Character::Char_SkipCutscene()
{
	MAUTOSTRIP(CWObject_Character_Char_SkipCutscene, false);
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if (!pCD)
		return false;

	if(m_ClientFlags & PLAYER_CLIENTFLAGS_CUTSCENE && m_PendingTimeLeapTick == -1 && !(m_Flags & PLAYER_FLAGS_TIMELEAP))
	{
//		int iGameState = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETGAMESTATE), m_pWServer->Game_GetObjectIndex());
//		if(iGameState == CWObject_Mod::MOD_GAMESTATE_GAMEON ||
//		   iGameState == CWObject_GameCampaign::CAMPAIGN_GAMESTATE_CUTSCENE)
		{
			m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_FADESCREEN, 300), m_pWServer->Game_GetObjectIndex());
			m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_FADESOUND, 300), m_pWServer->Game_GetObjectIndex());
			m_PendingTimeLeapTick = pCD->m_GameTick + int(0.3f * m_pWServer->GetGameTicksPerSecond()) + 2;
			CWObject_GameCore::m_sbRenderSkipText = true;
			return true;
		}
	}
	return false;
}

bool CWObject_Character::Char_EndCutscene()
{
	MAUTOSTRIP(CWObject_Character_Char_EndCutscene, false);
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if (!pCD)
		return false;

//	if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETGAMESTATE), m_pWServer->Game_GetObjectIndex()) != CWObject_Mod::MOD_GAMESTATE_GAMEON)
//		return false;

	m_iLastAutoUseObject = 0;

	// Reset cutscene FOV so the next cutscene won't start with fov45 or something
	pCD->m_CutsceneFOV = 90;

	Char_EndTimeLeap();
	if(Char_GetPhysType(this) != PLAYER_PHYS_DEAD)
	{
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_FADESCREEN, -500), m_pWServer->Game_GetObjectIndex());
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_FADESOUND, -500), m_pWServer->Game_GetObjectIndex());
	}
	m_PendingTimeLeapTick = -1;
	CWObject_GameCore::m_sbRenderSkipText = false;
	// Force player to weaponswitch
	//CWAG2I_Context Context(this,m_pWServer,pCD->m_GameTime);
	/*pCD->m_AnimGraph2.GetAG2I()->ClearTokens();
	pCD->m_AnimGraph2.GetAG2I()->Refresh(&Context);*/

	if(m_ClientFlags & PLAYER_CLIENTFLAGS_CUTSCENE)
	{
		UnpauseAllAI(CAI_ResourceHandler::PAUSE_CUTSCENE);
		
		ClientFlags() &= ~(PLAYER_CLIENTFLAGS_CUTSCENE | PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK);

		CWObject_Message Msg(OBJMSG_GAME_SETCLIENTWINDOW);
		Msg.m_iSender = m_iObject;
		m_pWServer->Message_SendToTarget(Msg, (char*)WSERVER_GAMEOBJNAME);

		Char_SetCutsceneCamera(0);

		return true;
	}
	else
		return false;
}

bool CWObject_Character::Char_ShowMissionJournal()
{
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if (!pCD)
		return false;

	char *pWndName = NULL;
	switch(m_NextGUIScreen)
	{
	case 1: pWndName = "mission_inventory"; break;
	case 2: pWndName = "mission_map"; break;
	case 3: pWndName = "mission_collectibles"; break;
	default: pWndName = "mission_objectives"; break;
	}
	CWObject_Message Msg(OBJMSG_GAME_SETCLIENTWINDOW, aint(pWndName),
		m_pWServer->Game_GetObject()->Player_GetClient(pCD->m_iPlayer));

	Msg.m_iSender = m_iObject;
	m_pWServer->Message_SendToTarget(Msg, (char*)WSERVER_GAMEOBJNAME);
	m_NextGUIScreen = 0;

	return true;
}

bool CWObject_Character::Char_EndShowMissionJournal()
{
	CWObject_Message Msg(OBJMSG_GAME_SETCLIENTWINDOW);
	Msg.m_iSender = m_iObject;
	m_pWServer->Message_SendToTarget(Msg, (char*)WSERVER_GAMEOBJNAME);

	return true;
}

bool CWObject_Character::Char_ShowMissionMap()
{
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if (!pCD)
		return false;

	char *pWndName = "mission_map";
	CWObject_Message Msg(OBJMSG_GAME_SETCLIENTWINDOW, aint(pWndName), 
		m_pWServer->Game_GetObject()->Player_GetClient(pCD->m_iPlayer));
	Msg.m_iSender = m_iObject;
	m_pWServer->Message_SendToTarget(Msg, (char*)WSERVER_GAMEOBJNAME);

	return true;
}

bool CWObject_Character::Char_EndShowMissionMap()
{
	CWObject_Message Msg(OBJMSG_GAME_SETCLIENTWINDOW);
	Msg.m_iSender = m_iObject;
	m_pWServer->Message_SendToTarget(Msg, (char*)WSERVER_GAMEOBJNAME);

	return true;
}

CVec3Dfp32 CWObject_Character::Target_GetObjectVector(CWObject_CoreData* _pObjSelf, int _iObj, CWorld_PhysState* _pWPhysState)
{
	MAUTOSTRIP(CWObject_Character_Target_GetObjectVector, CVec3Dfp32());
	CWObject_CoreData* pObj = _pWPhysState->Object_GetCD(_iObj);
	if (!pObj) return 0;

	CVec3Dfp32 TargetPos;
	CVec3Dfp32 BoxMin;

	pObj->GetAbsBoundBox()->GetCenter(TargetPos);
	TargetPos[2] = LERP(pObj->GetAbsBoundBox()->m_Min[2], pObj->GetAbsBoundBox()->m_Max[2], 0.75f);
	return TargetPos - (_pObjSelf->GetPosition() + Camera_GetHeadPos(_pObjSelf));
}

// Checks line of sight from a given position to another. Sets the _HitPos param to the 
// position that a hit occurred, or the _To param if there was no hit. 
bool CWObject_Character::Target_CheckLOS(const CVec3Dfp32& _From, const CVec3Dfp32& _To, CWorld_PhysState* _pWPhysState, CVec3Dfp32* _HitPos)
{
	MAUTOSTRIP(CWObject_Character_Target_CheckLOS, false);
	// The following setting works, so far.
	int objects = OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
	int mediums = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID;

	// The following two settings does not work.
//	int objects = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
//	int mediums = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID;

//	int objects = OBJECT_FLAGS_PHYSOBJECT;
//	int mediums = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID;
	
	CCollisionInfo CInfo;
	bool bHit = _pWPhysState->Phys_IntersectLine(_From, _To, 0, objects, mediums, (_HitPos) ? &CInfo : NULL);

	if (_HitPos) // Should we set hit position?
	{
		if(bHit && CInfo.m_bIsValid)
			*_HitPos = CInfo.m_Pos; // We hit something, set position.
		else
			*_HitPos = _To; // We didn't hit or couldn't get valid hit info.
	}

	return !bHit; //Succeed if we DIDN'T hit anything (i.e. nothing occludes the line of sight).
};


fp32 CWObject_Character::Target_SelectionFitness(fp32 _CosV, fp32 _DistSqr)
{
	MAUTOSTRIP(CWObject_Character_Target_SelectionFitness, 0.0f);
	return 2 * 1000*1000 - (1 - _CosV) * _DistSqr;
};


#define TARGET_USE_AI_CHAR_LIST


class CPrioChar
{
public:
	CWObject_Character * m_pChar;
	int m_iPrio;

	CPrioChar(CWObject_Character * _pChar = NULL, int _iPrio = 0)
	{
		m_pChar = _pChar;
		m_iPrio = _iPrio;
	};

	int GetPriority() const
	{
		return m_iPrio;
	};
};

fp32 CWObject_Character::GetMaxTargetingDistance(CWObject_CoreData* _pObj)
{
	MAUTOSTRIP(CWObject_Character_GetMaxTargetingDistance, 0.0f);
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if (pCD != NULL) 
		return pCD->m_MaxTargetingDistance;

	return OBJECT_MAXTARGETINGDISTANCE_DEFAULT;
}

fp32 CWObject_Character::GetTargetRingRadius(CWObject_CoreData* _pObj)
{
	MAUTOSTRIP(CWObject_Character_GetTargetRingRadius, 0.0f);
	const CBox3Dfp32* pBBox = _pObj->GetAbsBoundBox();
	if (pBBox == NULL)
		return 0;

	fp32 Radius = (pBBox->m_Max - pBBox->m_Min).Length() / 4.0f;
	return Radius;
}

void CWObject_Character::OnRefresh_Target()
{
/*	MAUTOSTRIP(CWObject_Character_OnRefresh_Server_Target, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_Character::OnRefresh_Target);

	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if(!pCD) 
		return;

	if (!Char_IsPlayer(pCD, this))
		return;
	
	uint8 CrosshairInfo = PLAYER_NOT_AIMING;
	if ((Char_GetPhysType(this) == PLAYER_PHYS_DEAD) || (Char_GetPhysType(this) == PLAYER_PHYS_NOCLIP))
	{
		pCD->m_CrosshairInfo = CrosshairInfo;
		return;
	}
	
//	MSCOPE(CWObject_Character::OnRefresh_Server_Target, CHARACTER);
	
	int iOldTarget = pCD->Target_GetObject();
	bool HadTarget = (iOldTarget != 0);

	CRPG_Object_Char* pChar = Char();
	CRPG_Object_Weapon* pWeapon = NULL;
	if (pChar != NULL)
		pWeapon = safe_cast<CRPG_Object_Weapon>(pChar->GetEquippedItem(RPG_CHAR_INVENTORY_WEAPONS));

	if ((pWeapon != NULL) && 
		((pWeapon->m_Flags & RPG_ITEM_FLAGS_AUTOAIM) ||
		 (pWeapon->m_Flags & RPG_ITEM_FLAGS_AIMING)))
	{
		CrosshairInfo = PLAYER_CROSSHAIR_VISIBLE;
	}

	CMat4Dfp32 AimMatrixCamera; GetCameraAimMatrix(this, m_pWServer, &AimMatrixCamera, true);
	CVec3Dfp32 AimOriginCamera = CVec3Dfp32::GetRow(AimMatrixCamera, 3);

//	CMat4Dfp32 AimMatrix; GetCameraAimMatrix(this, m_pWServer, &AimMatrix, false);
//	CVec3Dfp32 AimOrigin = CVec3Dfp32::GetRow(AimMatrix, 3);
	CMat4Dfp32 AimMatrix = GetPositionMatrix();
	CVec3Dfp32 AimOrigin = GetCharacterCenter(this, m_pWServer);
	CVec3Dfp32 AimDir = CVec3Dfp32::GetRow(AimMatrix, 0);
	CVec3Dfp32 AimUp = CVec3Dfp32::GetRow(AimMatrix, 2);

//	if (AssistAim(this))
//	{ // Remove vertical component.
//		AimDir -= AimUp * (AimDir * AimUp);
//	}

	bool RingTargeted = false; // Whether aim has been detected to be inside any target ring so far.
	CWObject* pBestTarget = NULL;
	fp32 BestTargetDistance = _FP32_MAX;
	fp32 BestAimDistance = _FP32_MAX;
	fp32 BestTargetScore = _FP32_MAX;
	fp32 BestTargetRingRadius = _FP32_MAX;
	bool bBestNoTargetRing = false;
	CVec3Dfp32 BestTargetDir;

	fp32 MaxTargetAimDistance = 100.0f;
	fp32 MaxTargetNormalizedAimDistance = 0.5f; // 1.0f == 45 degrees.
	fp32 TargetDistanceWeight = 0.1f;
	fp32 AimDistanceWeight = 1.0f;
//	fp32 AngleLimit = cos(30.0f / 180.0f * _PI);

	
	if( !Char_IsClimbing() && !Char_IsSwimming())
	{
		CSimpleIntList * plAgents = m_spAI->GetAgents(); 
		for(int i = 0; i < plAgents->Length(); i++)
		{

			int iObj = plAgents->Get(i);

			// Dont' consider self.
			if ((iObj == m_iObject))
				continue;

			bool bNoTargetRing = false;

			//Below code is deprecated; target everyone for now
			//CWObject* pEnemy;
			//pEnemy = m_spAI->IsEnemy(iObj);
			//if (pEnemy == NULL)
			//	continue; // Skip companions.
			//bNoTargetRing = true;

			//CWObject_Character* pTarget = IsCharacter(pEnemy);
			CWObject_Character* pTarget = IsCharacter(iObj, m_pWServer);
			if (!pTarget)
				continue;

			if (pTarget->m_Flags & PLAYER_FLAGS_NOTARGETING)
				bNoTargetRing = true;

			// Get target aim ring radius.
			fp32 TargetRingErrorScale = 1.9f; // Used to compensate for visual error, or something =).
			fp32 TargetRingRadius = GetTargetRingRadius(pTarget);
			fp32 Speed = Min(10.0f, Max(1.0f, GetMoveVelocity().Length() * ( 1.0f / 4.0f )));
			TargetRingRadius /= Speed;

			if (bNoTargetRing)
				TargetRingRadius = 0;

			// Get crosshair radius distance to target aim ring center.
			CVec3Dfp32 TargetPos = GetCharacterCenter(pTarget, m_pWServer);

			CVec3Dfp32 TargetVector = TargetPos - AimOrigin;
			CVec3Dfp32 TargetVectorCamera = TargetPos - AimOriginCamera;
			fp32 TargetDistance = TargetVector * AimDir;
	//		fp32 TargetDistance = TargetVector.Length();
			
			if (TargetDistance < 0.0f)
				continue;

			fp32 MaxTargetDistance = GetMaxTargetingDistance(pTarget);

			if (pWeapon != NULL)
				MaxTargetDistance = Min(MaxTargetDistance, pWeapon->m_Range);
			
			fp32 MaxTargetDistanceOut = MaxTargetDistance + 5; // Distance needed to loose a target.
			fp32 MaxTargetDistanceIn = MaxTargetDistance - 5; // Distance needed to gain a target.

			// This is set to zero, since it's not the correct way to solve the problem (and makes it impossible to see close target's healthbar in melee).
			const fp32 MinTargetDistance = 1; // To avoid targeting close but to the side characters, which makes the player turn ugly.

			// To reduce flickering on/off when standing almost exactly on the limit distance.
			if (HadTarget)
			{
				if (TargetDistance > MaxTargetDistanceOut)
					continue;
			}
			else
			{
				if (TargetDistance > MaxTargetDistanceIn)
					continue;
			}

			if (TargetDistance < MinTargetDistance)
				continue;

			CVec3Dfp32 AimDistanceVector = (TargetPos - AimOriginCamera) - AimDir * (TargetVectorCamera * AimDir);

			if (AssistAim(pCD) && false)
			{ // Remove vertical component.
				AimDistanceVector -= AimUp * (AimDistanceVector * AimUp);
			}

			fp32 AimDistance = AimDistanceVector.Length() / TargetRingErrorScale;

			if (TargetDistance < 100)
				MaxTargetNormalizedAimDistance *= 1.0f + (1.0f - TargetDistance * ( 1.0f / 100.0f )) * 3.0f;

			fp32 TargetScore = 0.0f;

			bool BetterTarget = false;
			bool RingMaybyTargeted = false;

			if ((AimDistance < TargetRingRadius) && ((AimDistance / TargetDistance) < MaxTargetNormalizedAimDistance))
			{
				if ((TargetDistance < BestTargetDistance) || (!RingTargeted))
					BetterTarget = true;
				RingMaybyTargeted = true;
			}
			else if (!RingTargeted)
			{
				if (AimDistance > MaxTargetAimDistance)
					continue;

				// This should prevend targeting enemies outside a certain angle.
				if ((AimDistance / TargetDistance) > MaxTargetNormalizedAimDistance)
					continue;

				TargetScore = TargetDistance * TargetDistanceWeight + AimDistance / TargetDistance * AimDistanceWeight;
				if (TargetScore < BestTargetScore)
					BetterTarget = true;
			}

			if (BetterTarget)
			{
				int objects = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
				int mediums = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID;

				CCollisionInfo CInfo;
				CInfo.m_CollisionType = CXR_COLLISIONTYPE_PHYSICS; // Use boundbox, i.e. don't evaluate character skeleton.

	//			m_pWServer->Debug_RenderWire(0, TargetPos, 0x80FFFFFF, 20);

				bool bHit = m_pWServer->Phys_IntersectLine(AimOriginCamera, TargetPos, 0, objects, mediums, &CInfo, m_iObject);

				if (bHit && CInfo.m_bIsValid && (CInfo.m_iObject == iObj))
				{
	//				m_pWServer->Debug_RenderWire(AimOriginCamera, CInfo.m_Pos, 0xFF00FF00, 20);

					CVec3Dfp32 TargetDir = TargetVector; TargetDir.Normalize();
					if (RingMaybyTargeted)
						RingTargeted = true;

					pBestTarget = pTarget;
					BestTargetDistance = TargetDistance;
					BestAimDistance = AimDistance;
					BestTargetDir = TargetDir;
					BestTargetRingRadius = TargetRingRadius;
					BestTargetScore = TargetScore;
					bBestNoTargetRing = bNoTargetRing;
				}
				else
				{
	//				m_pWServer->Debug_RenderWire(AimOriginCamera, TargetPos, 0xFFFF0000, 20);
				}
			}
		}
	}

	if (pBestTarget != NULL)
		// Add margin for predicted player running backwards (i.e. predicted distance may grow).
		//pCD->m_MaxWeaponRange = (uint16)BestTargetDistance + 100.0f;
		pCD->m_MaxWeaponRange = (uint16)BestTargetDistance * 1.5f;
	else
		if (pWeapon != NULL)
			pCD->m_MaxWeaponRange = (uint16)pWeapon->m_Range * 1.5f;
		else
			pCD->m_MaxWeaponRange = 500;

	{
		uint8 SurfaceType = 0;

		int Objects = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
		int Mediums = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID;
		CCollisionInfo CInfo;
		fp32 Distance = pCD->m_MaxWeaponRange + 200.0f; // Add 200 to adjust for behind offset distance.
		
		CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
		if (m_pWServer->Phys_IntersectLine(AimOriginCamera, AimOriginCamera + AimDir * Distance, 0, Objects, Mediums, &CInfo, m_iObject))
		{
			Distance *= CInfo.m_Time;
			CWObject *pObj = m_pWServer->Object_Get(CInfo.m_iObject);
			if (IsCharacter(pObj) && Distance < GetMaxTargetingDistance(pObj)) // Don't allow body part or targeting for objects out of range.
			{
				if (pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER)
				{
					SurfaceType = CInfo.m_SurfaceType | 128;
				}
				else if (m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_ALLOWAUTOTARGET), CInfo.m_iObject))
				{
//					CrosshairInfo = PLAYER_CROSSHAIR_VISIBLE;
					pBestTarget = pObj;
				}
			}
		}

		pCD->m_FreeAimedTargetSurfaceType = SurfaceType;
	}

	bool AllowLock = false;
	if ((pChar != NULL) && (pWeapon != NULL))
		AllowLock = (pWeapon->m_Flags & RPG_ITEM_FLAGS_AUTOAIM) != 0;

	if (AllowLock)
		pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_LOCKAIM;
	else
		pCD->m_ExtraFlags = pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_LOCKAIM;

	//pCD->Target_SetAccuracy(Accuracy);
	if (pBestTarget != NULL)
	{
		pCD->Target_SetObject(pBestTarget->m_iObject);
	}
	else
	{
		pCD->Target_SetObject(0);
	}

	pCD->m_CrosshairInfo = CrosshairInfo;*/
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void CWObject_Character::OnRefresh_Predicted_VerifyTarget(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	MAUTOSTRIP(CWObject_Character_OnRefresh_ServerPredicted_VerifyTarget, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_Character::OnRefresh_Predicted_VerifyTarget);
	CWO_Character_ClientData *pCD = GetClientData(_pObj);
	if(!pCD) 
		return;

	if (pCD->m_iPlayer == -1)
		return;

	int iTarget = pCD->Target_GetObject();

	if (iTarget == 0)
	{
		// Don't clear the target id here, because it's needed until the hud has faded away completely.
		pCD->m_TargetHudState = 0;
	}
	else
	{
		pCD->m_HudTargetID = (uint16)iTarget;
		pCD->m_TargetHudState = 255;
	}

	if (iTarget == 0)
		return;
	
	CWObject_CoreData* pTarget = _pWPhysState->Object_GetCD(iTarget);
	if (pTarget == NULL)
	{
//		pCD->m_InsideRingTime = 0.0f;
//		pCD->Target_SetAccuracy(255.0f);
		pCD->Target_SetObject(0);
		return;
	}

	CMat4Dfp32 AimMatrixCamera; GetCameraAimMatrix(_pObj, _pWPhysState, &AimMatrixCamera, true);
	CVec3Dfp32 AimOriginCamera = CVec3Dfp32::GetRow(AimMatrixCamera, 3);

//	CMat4Dfp32 AimMatrix; GetCameraAimMatrix(this, m_pWServer, &AimMatrix, false);
//	CVec3Dfp32 AimOrigin = CVec3Dfp32::GetRow(AimMatrix, 3);
	CMat4Dfp32 AimMatrix = _pObj->GetPositionMatrix();
	CVec3Dfp32 AimOrigin = GetCharacterCenter(_pObj, _pWPhysState);
	CVec3Dfp32 AimDir = CVec3Dfp32::GetRow(AimMatrix, 0);
	CVec3Dfp32 AimUp = CVec3Dfp32::GetRow(AimMatrix, 2);

	fp32 MaxTargetDistance = GetMaxTargetingDistance(pTarget);

	if (pCD != NULL)
		MaxTargetDistance = Min(MaxTargetDistance, (fp32)pCD->m_MaxWeaponRange);

	fp32 MaxTargetDistanceOut = MaxTargetDistance + 5; // Distance needed to loose a target.
//	fp32 MaxTargetDistanceIn = MaxTargetDistance - 5; // Distance needed to gain a target.
//	fp32 MaxTargetAimDistance = 100.0f;
	fp32 MaxTargetNormalizedAimDistance = 0.5f; // 1.0f == 45 degrees.
//	fp32 TargetDistanceWeight = 0.1f;
//	fp32 AimDistanceWeight = 1.0f;
//	fp32 Accuracy;

	// Get target aim ring radius.
	fp32 TargetRingErrorScale = 1.9f; // Used to compensate for visual error, or something =).
	fp32 TargetRingRadius = GetTargetRingRadius(pTarget);
	fp32 Speed = Min(10.0f, Max(1.0f, _pObj->GetMoveVelocity().Length() * ( 1.0f / 4.0f )));
	TargetRingRadius /= Speed;

	// Get crosshair radius distance to target aim ring center.
	CVec3Dfp32 TargetPos = GetCharacterCenter(pTarget);

	CVec3Dfp32 TargetVector = TargetPos - AimOrigin;
	CVec3Dfp32 TargetVectorCamera = TargetPos - AimOriginCamera;
	fp32 TargetDistance = TargetVector * AimDir;
//	fp32 TargetDistance = TargetVector.Length();

	if ((TargetDistance < 0.0f) || (TargetDistance > MaxTargetDistanceOut))
	{
//		pCD->m_InsideRingTime = 0.0f;
//		pCD->Target_SetAccuracy(255.0f);
		pCD->Target_SetObject(0);
		return;
	}

	CVec3Dfp32 AimDistanceVector = (TargetPos - AimOriginCamera) - AimDir * (TargetVectorCamera * AimDir);

	if (AssistAim(pCD) && false)
	{ // Remove vertical component.
		AimDistanceVector -= AimUp * (AimDistanceVector * AimUp);
	}

	fp32 AimDistance = AimDistanceVector.Length() / TargetRingErrorScale;

	bool TargetInLOS = false;

	if (AimDistance < TargetRingRadius)
	{
		int objects = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
		int mediums = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID;

		CCollisionInfo CInfo;
		CInfo.m_CollisionType = CXR_COLLISIONTYPE_PHYSICS;

		bool bHit = _pWPhysState->Phys_IntersectLine(AimOriginCamera, TargetPos, 0, objects, mediums, &CInfo, _pObj->m_iObject);

		if (bHit && CInfo.m_bIsValid && (CInfo.m_iObject == iTarget))
		{
			TargetInLOS = true;
		}
	}

//	bool AllowLock = (pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_LOCKAIM) != 0;

	if (TargetDistance < 100)
		MaxTargetNormalizedAimDistance *= 1.0f + (1.0f - TargetDistance * ( 1.0f / 100.0f )) * 3.0f;

/*	// This normalized stuff should prevent targeting enemies outside a certain angle.
	if ((AimDistance < TargetRingRadius) && ((AimDistance / TargetDistance) < MaxTargetNormalizedAimDistance) && TargetInLOS && AllowLock)
	{
		pCD->m_CrosshairInfo = uint8(pCD->m_CrosshairInfo) | PLAYER_CROSSHAIR_INSIDE_TARGETRING;
//		Accuracy = 128.0f + 127.0f * pCD->m_InsideRingTime / PLAYER_TARGETRING_LOCKTIME;
		Accuracy = 255.0f * pCD->m_InsideRingTime / PLAYER_TARGETRING_LOCKTIME;

//		Accuracy = 128.0f + 127.0f * (1.0f - (AimDistance / TargetRingRadius));
	}
	else
	{
//		pCD->m_InsideRingTime = 0.0f;

		// This normalized stuff should prevent targeting enemies outside a certain angle.
		if ((AimDistance < MaxTargetAimDistance) && ((AimDistance / TargetDistance) < MaxTargetNormalizedAimDistance))
		{
			pCD->m_CrosshairInfo = uint32(pCD->m_CrosshairInfo) & ~PLAYER_CROSSHAIR_INSIDE_TARGETRING;
			Accuracy = 255.0f;
		}
		else
		{
			pCD->Target_SetObject(0);
		}
	}

//	ConOut(CStrF("SPVT: InsideRingTime = %f", (fp32)pCD->m_InsideRingTime));*/

//	pCD->Target_SetAccuracy(Accuracy);
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void CWObject_Character::OnRefresh_Predicted_Target(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	MAUTOSTRIP(CWObject_Character_OnRefresh_ServerPredicted_Target, MAUTOSTRIP_VOID);
	MSCOPE(CWObject_Character::OnRefresh_ServerPredicted_Target, CHARACTER);
	
	CWO_Character_ClientData *pCD = GetClientData(_pObj);
	if (!pCD) return;

	if (pCD->m_iPlayer == -1)
		return;

	OnRefresh_Predicted_VerifyTarget(_pObj, _pWPhysState);

	pCD->m_LastForcedLook = pCD->m_ForcedLook;
	pCD->m_LastForcedLookBlend = pCD->m_ForcedLookBlend;

	pCD->m_LastForcedTilt = pCD->m_ForcedTilt;
	pCD->m_LastForcedTiltBlend = pCD->m_ForcedTiltBlend;
	pCD->m_Target_LastAimAngles = pCD->Target_GetAimAngles();

	CMat4Dfp32 AimMatrix; GetCameraAimMatrix(_pObj, _pWPhysState, &AimMatrix, true);

	CVec3Dfp32 AimOrigin = CVec3Dfp32::GetRow(AimMatrix, 3);
	CVec3Dfp32 AimDir = CVec3Dfp32::GetRow(AimMatrix, 0);
	CVec2Dfp32 WantedAimAngles, AimAngles;

//	ConOutL(CStrF("Obj = %X, AimDir = <%f, %f, %f>", _pObj, AimDir[0], AimDir[1], AimDir[2]));
//	ConOutL(CStrF("AimOrigin = <%f, %f, %f>", AimOrigin[0], AimOrigin[1], AimOrigin[2]));

//	_pWPhysState->Debug_RenderWire(AimOrigin, AimOrigin + AimDir * 500.0f, 0x20002000, 10.0f);

	if (pCD->m_CrosshairInfo & PLAYER_CROSSHAIR_VISIBLE)
	{
		CWObject_CoreData* pTarget = _pWPhysState->Object_GetCD(pCD->Target_GetObject());
		if (AssistAim(pCD) && (pTarget != NULL) && (pCD->m_CrosshairInfo & PLAYER_CROSSHAIR_INSIDE_TARGETRING))
		{
			CVec3Dfp32 TargetPos = GetCharacterCenter(pTarget, NULL, false, 0.25f);
			CVec3Dfp32 TargetVector = TargetPos - AimOrigin;
			CVec3Dfp32 TargetDir = TargetVector; TargetDir.Normalize();
			WantedAimAngles = pCD->Target_GetAnglesForVector(TargetDir);

//			_pWPhysState->Debug_RenderWire(AimOrigin, TargetPos, 0xFFFFFFFF, 10.0f);
		}
		else
		{
			WantedAimAngles = pCD->Target_GetAnglesForVector(AimDir);
		}
	}
	else
	{
		WantedAimAngles = pCD->Target_GetAnglesForVector(AimDir);
	}

	if (pCD->m_bForceLookOnTarget == false)
	{
		pCD->m_ForcedLook = WantedAimAngles;
		pCD->m_LastForcedLook = WantedAimAngles;
	}

	// Force Look On Target (a.k.a. shield locking)
	pCD->m_bForceLookOnTarget = false;
	fp32 WantedForcedLookBlend = 0;
/*	
	if ((_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_BLOCKING) != 0)
	{
		CWObject_CoreData* pTarget = _pWPhysState->Object_GetCD(pCD->Target_GetObject());
		if (pTarget != NULL)
		{
			CVec2Dfp32 WantedForcedLook;
			CVec3Dfp32 TargetPos = GetCharacterCenter(pTarget);
			CVec3Dfp32 TargetVector = TargetPos - AimOrigin;
			CVec3Dfp32 TargetDir = TargetVector; TargetDir.Normalize();
			WantedForcedLook = pCD->Target_GetAnglesForVector(TargetDir);

			if (WantedForcedLook[1] > 1.0f)
			{
				WantedForcedLook[1] -= 1.0f;
				pCD->m_ForcedLook[1] -= 1.0f;
			}
			if (WantedForcedLook[1] < 0.0f)
			{
				WantedForcedLook[1] += 1.0f;
				pCD->m_ForcedLook[1] += 1.0f;
				}stForcedLook[1] -= 1.0f;
				if (pCD->m_LastForcedLook[1] < 0.0f)
				pCD->m_LastForcedLook[1] += 1.0f;
				if (pCD->m_ForcedLook[1] > 1.0f)
				pCD->m_ForcedLook[1] -= 1.0f;
				if (pCD->m_ForcedLook[1] < 0.0f)
				pCD->m_ForcedLook[1] += 1.0f;
				
				  WantedForcedLook = pCD->Target_WrapAngles(WantedForcedLook, pCD->m_LastForcedLook);

			if (pCD->m_LastForcedLook[1] > 1.0f)
				pCD->m_La
			pCD->m_ForcedLook *= 100.0f;
			pCD->m_ForcedLookChange *= 100.0f;
			WantedForcedLook *= 100.0f;
			Moderatef(pCD->m_ForcedLook[0], WantedForcedLook[0], pCD->m_ForcedLookChange[0], 100);
			Moderatef(pCD->m_ForcedLook[1], WantedForcedLook[1], pCD->m_ForcedLookChange[1], 100);
			pCD->m_ForcedLook *= 1.0f / 100.0f;
			pCD->m_ForcedLookChange *= 1.0f / 100.0f;
			//WantedForcedLook *= 1.0f / 100.0f;

			if (false)
				ConOutL(CStrF("ForcedTurn = %f, LastForcedTurn = %f", pCD->m_ForcedLook[1], pCD->m_LastForcedLook[1]));

			pCD->m_bForceLookOnTarget = false;
			WantedForcedLookBlend = 1.0f;
			WantedAimAngles = pCD->m_ForcedLook;
		}
	}
*/

	WantedForcedLookBlend *= 100.0f;
	pCD->m_ForcedLookBlend *= 100.0f;
	pCD->m_ForcedLookBlendChange *= 100.0f;
	Moderatef(pCD->m_ForcedLookBlend, WantedForcedLookBlend, pCD->m_ForcedLookBlendChange, 200);
	pCD->m_ForcedLookBlend *= ( 1.0f / 100.0f );
	pCD->m_ForcedLookBlendChange *= ( 1.0f / 100.0f );

	CVec3Dfp32 WantedAimDir = pCD->Target_GetVectorForAngles(WantedAimAngles);

	WantedAimAngles = pCD->Target_WrapAngles(WantedAimAngles, pCD->m_Target_LastAimAngles);
	AimAngles = pCD->m_Target_LastAimAngles;

	if (pCD->m_CrosshairInfo & PLAYER_CROSSHAIR_VISIBLE)
	{
		if (AssistAim(pCD))
		{
			AimAngles *= 100.0f;
			pCD->m_Target_AimAnglesChange *= 100.0f;
			WantedAimAngles *= 100.0f;
			Moderatef(AimAngles[0], WantedAimAngles[0], pCD->m_Target_AimAnglesChange[0], PLAYER_AIMASSISTCHANGEY);
			AimAngles *= ( 1.0f / 100.0f );
			pCD->m_Target_AimAnglesChange *= ( 1.0f / 100.0f );
			WantedAimAngles *= ( 1.0f / 100.0f );

			AimAngles[1] = WantedAimAngles[1];
			pCD->m_Target_AimAnglesChange[1] = 0;
		}
		else
		{
			AimAngles[0] = WantedAimAngles[0];
			pCD->m_Target_AimAnglesChange[0] = 0;

			AimAngles[1] = WantedAimAngles[1];
			pCD->m_Target_AimAnglesChange[1] = 0;
		}

		pCD->Target_SetAimAngles(AimAngles);
//		pCD->Target_SetAimAnglesVel(AimAngles - pCD->m_Target_LastAimAngles);
		pCD->Target_SetAimAnglesVel(0);

		AimDir = pCD->Target_GetVectorForAngles(AimAngles);

//		_pWPhysState->Debug_RenderVector(AimOrigin, WantedAimDir * 100.0f, 0xFF00FF00, 10.0f);
//		_pWPhysState->Debug_RenderVector(AimOrigin, AimDir * 100.0f, 0xFFFF0000, 10.0f);
	}
	else
	{
		AimAngles = WantedAimAngles;
		pCD->m_Target_AimAnglesChange = 0;
		pCD->m_ForcedTilt = 0.0f;
		pCD->m_ForcedTiltBlend = 0.0f;
		pCD->m_ForcedTiltBlendChange = 0.0f;
		pCD->Target_SetAimAngles(AimAngles);
//		pCD->Target_SetAimAnglesVel(AimAngles - pCD->m_Target_LastAimAngles);
		pCD->Target_SetAimAnglesVel(0);

		AimDir = pCD->Target_GetVectorForAngles(AimAngles);

//		_pWPhysState->Debug_RenderVector(AimOrigin, WantedAimDir * 100.0f, 0xFF00FF00, 10.0f);
//		_pWPhysState->Debug_RenderVector(AimOrigin, AimDir * 100.0f, 0xFFFF0000, 10.0f);

		return;
	}

//	ConOutL(CStrF("Obj = %X, WAA = %f, AA = %f, LAA = %f, AAC = %f", _pObj, WantedAimAngles.k[0], AimAngles.k[0], pCD->m_Target_LastAimAngles.k[0], pCD->m_Target_AimAnglesChange.k[0]));
//	ConOutL(CStrF("AimAngles = <%f, %f>, LastAimAngles = <%f, %f>", AimAngles.k[0], AimAngles.k[1], pCD->m_Target_LastAimAngles.k[0], pCD->m_Target_LastAimAngles.k[1]));

	fp32 WantedForcedTiltBlend;

	pCD->m_ForcedTilt = AimAngles[0];
	if (AssistAim(pCD) && (pCD->m_CrosshairInfo & PLAYER_CROSSHAIR_INSIDE_TARGETRING))
	{
		WantedForcedTiltBlend = 1.0f;
	}
	else
	{
		WantedForcedTiltBlend = 0.0f;
	}

	// Wrap target tilt angle.
	if ((pCD->m_LastForcedTilt - pCD->m_ForcedTilt) > 0.5f)
		pCD->m_ForcedTilt += 1.0f;
	if ((pCD->m_LastForcedTilt - pCD->m_ForcedTilt) < -0.5f)
		pCD->m_ForcedTilt -= 1.0f;

	WantedForcedTiltBlend *= 100.0f;
	pCD->m_ForcedTiltBlend *= 100.0f;
	pCD->m_ForcedTiltBlendChange *= 100.0f;
	Moderatef(pCD->m_ForcedTiltBlend, WantedForcedTiltBlend, pCD->m_ForcedTiltBlendChange, 200);
	pCD->m_ForcedTiltBlend *= ( 1.0f / 100.0f );
	pCD->m_ForcedTiltBlendChange *= ( 1.0f / 100.0f );
}


void CWObject_Character::OnRefresh_Mechanics()
{
	MAUTOSTRIP(CWObject_Character_OnRefresh_Mechanics, MAUTOSTRIP_VOID);
	MSCOPE(CWObject_Character::OnRefresh_Mechanics, CHARACTER);

	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if (!pCD) return;

	// Added by Mondelore {START}.
/*	if (Char() && Char()->GetCurItem(0) && 
		((Char()->GetCurItem(0)->m_Flags & RPG_ITEM_FLAGS_AUTOAIM) ||
		 (Char()->GetCurItem(0)->m_Flags & RPG_ITEM_FLAGS_AIMING)))*/
	{
		// EXPIRED: Special Rotation
//		pCD->m_Camera_SpecialOffset = CVec3Dfp32(0, 0, 0);
//		pCD->m_Camera_SpecialRotation = CVec3Dfp32(0, 0, 0);
	}
/*	else
	{
		// These values will tilt the non aiming weapon modes down a bit.
		pCD->m_Camera_SpecialOffset = CVec3Dfp32(0, -20.0f, +25.0f);
		pCD->m_Camera_SpecialRotation = CVec3Dfp32(-0.35f, 0, 0);
	}*/

	// Remove reloadflags when not reloading anymore
	if (!(pCD->m_AnimGraph2.GetStateFlagsLoCombined() & AG2_STATEFLAG_RELOADING))
		Char()->ClearReloadFlags(AG2_ITEMSLOT_WEAPONS);

	uint32 extraflags = pCD->m_ExtraFlags;

	if(pCD->m_iPlayer != -1)
	{
		// Defaults
		int assistcamera = 0;
		int assistaim = 1;
		int cameramode = PLAYER_CAMERAMODE_NORMAL;

		{
			int iClient = m_pWServer->Game_GetObject()->Player_GetClient(pCD->m_iPlayer);
			if (iClient >= 0)
			{
				CRegistry* Reg = m_pWServer->Registry_GetClientVar(iClient);
				if (Reg != NULL)
				{
					assistcamera = Reg->GetValuei("control_camera_assist", assistcamera, 0);
					assistaim = Reg->GetValuei("control_target_assist", assistaim, 0);
					cameramode = Reg->GetValuei("control_cameramode", cameramode, 0);
				}
			}
		}

//		if(pCD->m_Health <= pCD->m_ShieldStart && !(Char_GetPhysType(this) == PLAYER_PHYS_DEAD))
		if(pCD->m_HitEffect_Flags & PLAYER_HITEFFECT_FLAGS_DAMAGE && pCD->m_HitEffect_Flags & PLAYER_HITEFFECT_FLAGS_SOUND && Char_GetPhysType(this) != PLAYER_PHYS_DEAD)
		{
			fp32 HealthP = 1.0f - fp32(pCD->m_Health.m_Value) / fp32(pCD->m_MaxHealth.m_Value);
			fp32 RepeatTime = 1.5f - HealthP * 0.75f;
			fp32 Volume = 0.8f + HealthP * 0.4f;
			if((pCD->m_GameTick - m_LastHurtSoundTick) * m_pWServer->GetGameTickTime() >= RepeatTime)
			{
				bool bIsDarkling = pCD->IsDarkling() ? true : false;
				m_pWServer->Sound_On(m_iObject, m_pWServer->GetMapData()->GetResourceIndex_Sound(bIsDarkling ? "D_Hurts_Darkl_Breath_01" : "D_Hurts_Jackie_Breath_01"), WCLIENT_ATTENUATION_2D_CUTSCENE, 0, Volume, pCD->m_iPlayer);
				m_LastHurtSoundTick = pCD->m_GameTick;
				CNetMsg Msg(PLAYER_NETMSG_RUMBLE);
				if(HealthP < 0.3f)
					Msg.AddStr("DamageBreath1");	
				else if(HealthP < 0.6f)
					Msg.AddStr("DamageBreath2");	 
				else
					Msg.AddStr("DamageBreath3");	

				m_pWServer->NetMsg_SendToObject(Msg, m_iObject);
			}
		}

		if (assistcamera)
			extraflags |= PLAYER_EXTRAFLAGS_ASSISTCAMERA;
		else
		extraflags &= ~PLAYER_EXTRAFLAGS_ASSISTCAMERA;

		if (assistaim)
			extraflags |= PLAYER_EXTRAFLAGS_ASSISTAIM;
		else
			extraflags &= ~PLAYER_EXTRAFLAGS_ASSISTAIM;

		extraflags &= ~PLAYER_EXTRAFLAGS_CAMERAMODE_MASK;
		extraflags |= cameramode << PLAYER_EXTRAFLAGS_CAMERAMODE_SHIFT;

		// If weapon has timed out, select "fists" (or equal) and set weapon id (don't take down
		// ancient weapons)
		// Try to select fists
		const int PhysType = CWObject_Character::Char_GetPhysType(this);
		CRPG_Object_Item* pItem = Char()->GetFinalSelectedItem(AG2_ITEMSLOT_WEAPONS);
		if (!InNoClipMode(PhysType) && (m_Flags & PLAYER_FLAGS_AUTOUNEQUIPWEAPONS) && pItem && 
			(pItem->m_iItemType != CRPG_Object_Fist_ItemType && 
			pItem->m_iItemType != RPG_ITEMTYPE_ANCIENTWEAPONS))
		{
			// Check if we need a reload
			CWAG2I_Context AGContext(this,m_pWServer,pCD->m_GameTime);
			bool bHasResetTimer = false;
			if (pItem->NeedReload(m_iObject) && !(pItem->m_Flags2 & RPG_ITEM_FLAGS2_THROWAWAYONEMPTY))
			{
				CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_RELOAD);
				bHasResetTimer = pCD->m_AnimGraph2.SendAttackImpulse(&AGContext,Impulse);	
				// Send impulse to weapon as well
				pCD->m_WeaponAG2.UpdateItemProperties(Char(),pItem,this,pCD);
				pCD->m_WeaponAG2.GetAG2I()->SendImpulse(&AGContext, Impulse, 0);
				pCD->m_WeaponHUDStartTick = m_pWServer->GetGameTick();
			}
			else if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), m_pWServer->Game_GetObjectIndex()) && !pItem->GetAmmo(Char()) &&
				!pCD->m_AnimGraph2.GetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED))
			{
				CRPG_Object_Inventory * pWeaponInv = Char()->GetInventory(AG2_ITEMSLOT_WEAPONS);
				int32 iCurEquipped = pWeaponInv->GetFinalSelectedItemIndex();
				CRPG_Object_Item* pCurItem = pWeaponInv->FindFinalSelectedItem();

				if(pCurItem->m_iItemType != 30)
				{
					int PreviousWeaponId = pCurItem->m_Identifier;
					int PreviousItemType = pCurItem->m_iItemType;

					TThinArray<uint8> ValidWeapons;
					ValidWeapons.SetLen(pWeaponInv->GetNumItems());
					uint8 iCurValidWeapon = 0;
					bool bGunAdded = false;
					for(int i = 0; i < pWeaponInv->GetNumItems(); i++)
					{
						if(i != iCurEquipped)
						{
							CRPG_Object_Item* pWeapon = pWeaponInv->GetItemByIndex(i);
							if(pWeapon->m_iItemType != 30 && (pWeapon->GetAmmo(Char()) || pWeapon->GetMagazines()))	//No darkling attack and no empty weapons
							{
								if(pWeapon->m_iItemType == 1)
								{
									if(bGunAdded)
										continue;
									else
										bGunAdded = true;
								}
								
								ValidWeapons[iCurValidWeapon] = i;
								iCurValidWeapon++;
							}
						}
					}

					if(iCurValidWeapon)
					{
						if(iCurValidWeapon > 1)
						{
							for(int i = 0; i < iCurValidWeapon; i++)
							{
								CRPG_Object_Item* pWeapon = pWeaponInv->GetItemByIndex(ValidWeapons[i]);
								if(pWeapon->m_iItemType != 1)
								{
									pCD->m_iCurSelectedItem = ValidWeapons[i];
									break;
								}
							}
						}
						else 
							pCD->m_iCurSelectedItem = ValidWeapons[0];

						CRPG_Object_Item* pItem = Char()->GetItemByIndex(0, pCD->m_iCurSelectedItem);
						if (pItem)
						{
							m_LastUsedItem = PreviousWeaponId;
							m_LastUsedItemType = PreviousItemType;

							// AG2 version
							pItem->m_Flags2 |= RPG_ITEM_FLAGS2_TAGGEDFORRELOAD;
							CWAG2I_Context AGContext(this, m_pWServer, pCD->m_GameTime);
							pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER,pItem->m_Identifier);
							if (pCD->m_AnimGraph2.SendAttackImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_RELOAD)))
								pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED,true);
						}
						pCD->m_WeaponUnequipTick = m_pWServer->GetGameTick();
						m_WeaponUnequipTimer = 0;
					}
				}
			}
		
			// Reset unequip timer while equipping/unequipping/reloading
			if (!bHasResetTimer && Char_IsPlayerViewControlled(this) && (pCD->m_AnimGraph2.GetStateFlagsLoToken2() & AG2_STATEFLAG_EQUIPPING))
			{
				SetWeaponUnequipTimer();
				bHasResetTimer = true;
			}

			if (bHasResetTimer && (m_WeaponUnequipTimer > 0) && (m_WeaponUnequipTimer < pCD->m_GameTick))
			{
				int PreviousWeaponId = pItem->m_Identifier;
				int PreviousItemType = pItem->m_iItemType;

				// Find fist item
				CRPG_Object_Item* pMelee = Char()->FindItemByType(CRPG_Object_Fist_ItemType);
				if (pMelee)
				{
					int ItemIndex = Char()->GetInventory(AG2_ITEMSLOT_WEAPONS)->FindItemIndexByIdentifier(pMelee->m_Identifier);
					int ItemIndexPrev = Char()->GetInventory(AG2_ITEMSLOT_WEAPONS)->FindItemIndexByIdentifier(pItem->m_Identifier);
					pCD->m_iCurSelectedItem = ItemIndex;
					// Set current selected item
					// Select fist (go through ag and stuff...)
					if (pCD->m_iCurSelectedItem != ItemIndexPrev)
					{
						//Switch weapon
						CRPG_Object_Item* pItem = Char()->GetItemByIndex(0,pCD->m_iCurSelectedItem);
						if (pItem)
						{
							// Set identifier of next weapon and unequip weapon
							pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER,pItem->m_Identifier);
							pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED,true);
							pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_UNEQUIP),0);

							m_LastUsedItem = PreviousWeaponId;
							m_LastUsedItemType = PreviousItemType;
							pCD->m_WeaponUnequipTick = m_pWServer->GetGameTick();
						}
					}
					m_WeaponUnequipTimer = 0;
				}
			}
			else if (m_WeaponUnequipTimer == 0)
			{
				// Weapon unequip timer needs to be set
				SetWeaponUnequipTimer(true);
			}
			else if (/*(pItem->m_Flags2 & RPG_ITEM_FLAGS2_THROWAWAYONEMPTY) &&*/ !pItem->GetAmmo(Char()) && !pItem->GetMagazines() &&
				!(pCD->m_AnimGraph2.GetStateFlagsLoToken2() & (AG2_STATEFLAG_EQUIPPING|AG2_STATEFLAG_RELOADING)))
			{
				// Might take a while until we have thrown away the item...
				// Ok, maybe we have run out of ammo, if so throw the weapon away
				int32 ItemType = pItem->m_iItemType;
				//m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_FORCEDROPWEAPON),m_iObject);

				// Select item of similar type
				CRPG_Object_Item* pNextItem = Char()->FindNextItemNotEquippedByAnimType(pItem->m_Identifier,pItem->m_AnimType,AG2_ITEMSLOT_WEAPONS,-1,pItem,true);
				//CRPG_Object_Item* pNextItem = Char()->FindNextItemForReloadByType(pItem->m_Identifier, ItemType,pItem);
				// Don't throw away if we can't find an item of the same type
				/*if (!pNextItem)
					pNextItem = Char()->FindItemByType(CRPG_Object_Fist_ItemType);*/
				if (pNextItem/* && !pCD->m_AnimGraph2.GetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED)*/)
				{
					pCD->m_WeaponHUDStartTick = m_pWServer->GetGameTick();
					pNextItem->m_Flags2 |= RPG_ITEM_FLAGS2_TAGGEDFORRELOAD;
					// If we don't get in here something's very wrong
					int ItemIndex = Char()->GetInventory(AG2_ITEMSLOT_WEAPONS)->FindItemIndexByIdentifier(pNextItem->m_Identifier);
					pCD->m_iCurSelectedItem = ItemIndex;
					// Set identifier of next weapon and unequip weapon ("reload" for dual guns?)
					CWAG2I_Context AGContext(this,m_pWServer,pCD->m_GameTime);
					pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER,pNextItem->m_Identifier);
					CXRAG2_Impulse Impulse = pItem->m_AnimType == AG2_IMPULSEVALUE_WEAPONTYPE_RIFLE ? CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_UNEQUIP) : CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_RELOAD);
					if (pCD->m_AnimGraph2.SendAttackImpulse(&AGContext, Impulse))
						pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED,true);
					m_LastUsedItem = pItem->m_Identifier;
					m_LastUsedItemType = pItem->m_iItemType;
					m_WeaponUnequipTimer = 0;
				}
			}

			// Reload secondary
			CRPG_Object_Item* pItemDualWield = Char()->GetFinalSelectedItem(AG2_ITEMSLOT_DUALWIELD);
			if (pItemDualWield && ((pItemDualWield->m_Flags2 & RPG_ITEM_FLAGS2_THROWAWAYONEMPTY) && !pItemDualWield->GetAmmo(Char()) && 
				!(pCD->m_AnimGraph2.GetStateFlagsLoToken3() & (AG2_STATEFLAG_EQUIPPING|AG2_STATEFLAG_RELOADING))))
			{
				// Might take a while until we have thrown away the item...
				// Ok, maybe we have run out of ammo, if so throw the weapon away
				int32 ItemType = pItemDualWield->m_iItemType;
				//m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_FORCEDROPWEAPON),m_iObject);

				// Select item of similar type
				CRPG_Object_Item* pNextItem = Char()->FindNextItemForReloadByType(pItemDualWield->m_Identifier, ItemType & ~0x200,pItemDualWield);
				// Don't throw away if we can't find an item of the same type
				/*if (!pNextItem)
				pNextItem = Char()->FindItemByType(CRPG_Object_Fist_ItemType);*/
				if (pNextItem/* && !pCD->m_AnimGraph2.GetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUEDDUALWIELD)*/)
				{
					pCD->m_WeaponHUDStartTick = m_pWServer->GetGameTick();
					pNextItem->m_Flags2 |= RPG_ITEM_FLAGS2_TAGGEDFORRELOAD;
					// If we don't get in here something's very wrong
					//Char()->GetInventory(AG2_ITEMSLOT_WEAPONS)->FindItemIndexByIdentifier(pNextItem->m_Identifier);
					// Set identifier of next weapon and unequip weapon ("reload" for dual guns?)
					CWAG2I_Context AGContext(this,m_pWServer,pCD->m_GameTime);
					pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIERDUALWIELD,pNextItem->m_Identifier);
					if (pCD->m_AnimGraph2.SendAttackImpulseDualWield(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_RELOADSECONDARY)))
						pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUEDDUALWIELD,true);
					m_WeaponUnequipTimer = 0;
				}
			}
		}
	}

	pCD->m_ExtraFlags = extraflags;
	// Added by Mondelore {END}.

	// No health regeneration, only if that darkness power is activated?
	if(!(Char_GetPhysType(this) == PLAYER_PHYS_DEAD))
	{
		// Replication NOT needed for players only.
		if(Char()->Health() <= 0)
			Char_Die(DAMAGETYPE_UNDEFINED, m_iObject);

//		CMat4Dfp32 Mat = GetPositionMatrix();
//		CVec3Dfp32::GetMatrixRow(Mat, 3) += Char_GetCameraPos(this);
//		CMat4Dfp32 Mat;
//		GetActivatePosition(Mat);
		m_spRPGObject->Process(NULL, m_iObject);

		// Refresh weapon animgraph
		CWAG2I_Context AG2Context(this,m_pWServer,CMTime::CreateFromTicks(pCD->m_GameTick,m_pWServer->GetGameTickTime()));
		CWAG2I* pAG2I = pCD->m_WeaponAG2.GetAG2I();
		if (pAG2I)
		{
			// Weapon in hand 1
			pCD->m_WeaponAG2.UpdateItemProperties(Char(),Char()->GetEquippedItem(AG2_ITEMSLOT_WEAPONS),this,pCD);
			pAG2I->RefreshToken(&AG2Context,TOKEN_WEAPON1,0);
			pCD->m_WeaponAG2.UpdateItemProperties(Char(),Char()->GetEquippedItem(AG2_ITEMSLOT_DUALWIELD),this,pCD);
			pAG2I->RefreshToken(&AG2Context,TOKEN_WEAPON2,1);
			// Weapon in hand 2
			/*pCD->m_WeaponAG2.UpdateItemProperties(Char(),Char()->GetEquippedItem(0));
			pAG2I->RefreshToken(&AG2Context,TOKEN_WEAPON2);*/
		}

	/*	CRPG_Object_Char* pChar = Char();
		if (pChar->m_HealthRegenerationDelay != 0)
		{
			int Frac = (pChar->Health() & 15);
			if(Frac != 0)
			{
				Frac = 16 - Frac;
				int TickDiff = m_pWServer->GetGameTick() - pChar->m_LastDamageTick;
				if (TickDiff >= pChar->m_HealthRegenerationDelay)
					pChar->ReceiveHealth(Frac);
			}
		}*/
	}

	int Health = Char()->Health();
	pCD->m_Health = (int)Min(Health, 255);
	int MaxHealth = Char()->MaxHealth();
	pCD->m_MaxHealth = (int)Min(MaxHealth, 255);

	if(pCD->m_iPlayer != -1)
	{
		// More stuff should be moved in here.
		bool bIsMP = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), m_pWServer->Game_GetObjectIndex()) ? true : false;
		if (Char_GetPhysType(this) == PLAYER_PHYS_DEAD)
			pCD->m_Tension = 0;
		else if (m_Flags & PLAYER_FLAGS_FORCETENSION)
			pCD->m_Tension = 255;
		else if(bIsMP)
			pCD->m_Tension = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETTENSION, m_iObject), m_pWServer->Game_GetObjectIndex());
		else
			pCD->m_Tension = (int)(m_spAI->GetTension() * 255);

		bool bWait = Char()->Wait() > 0;
		if(bWait ^ ((m_ClientFlags & PLAYER_CLIENTFLAGS_WAIT) != 0))
			ClientFlags() ^= PLAYER_CLIENTFLAGS_WAIT;

		if(m_PendingCutsceneTick != -1 && m_PendingCutsceneTick == pCD->m_GameTick)
		{
			Char_SpawnCutscene(m_PendingCutsceneCamera, false, m_PendingCutsceneType);
			m_PendingCutsceneTick = -1;
			m_PendingCutsceneType = -1;
		}

		CRPG_Object_Item *pItem = Char()->FindItemByType(0x124);
		if(pItem)
			pCD->m_nUDMoney = pItem->m_NumItems;
		pItem = Char()->FindItemByType(0x125);
		if(pItem)
			pCD->m_nMoth = pItem->m_NumItems;
	}
	else
	{
		if(pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_STUNNED && Char()->Wait() == 0)
			pCD->m_ExtraFlags = pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_STUNNED;
	}

	{
		MSCOPESHORT("POSSIBLE_REMOVAL");
		{
			// Check pending item model updates
			for(int i = 0; i < 4; i++)
				if(m_iPendingItemModelUpdate[i] != -1)
					if(m_iPendingItemModelUpdate[i] <= pCD->m_GameTick)
					{
						OnUpdateItemModel(i);
						m_iPendingItemModelUpdate[i] = -1;
					}
		}

		const int nItems = 2;
		CRPG_Object_Item *pSelectedItems[nItems];
		pSelectedItems[0] = Char()->GetFinalSelectedItem(RPG_CHAR_INVENTORY_WEAPONS);
		pSelectedItems[1] = Char()->GetFinalSelectedItem(RPG_CHAR_INVENTORY_ARMOUR);
		CRPG_Object_Item* pEquippedItems[nItems];
		pEquippedItems[0] = Char()->GetEquippedItem(RPG_CHAR_INVENTORY_WEAPONS);
		pEquippedItems[1] = Char()->GetEquippedItem(RPG_CHAR_INVENTORY_ARMOUR);

		int TempItemSustantion = 2;
		if((m_Player.m_iTemporaryItemType != RPG_ITEMTYPE_UNDEFINED) && (m_Player.m_iTemporaryItemLastTick < (pCD->m_GameTick - TempItemSustantion)))
			m_Player.m_iTemporaryItemType = RPG_ITEMTYPE_UNDEFINED;
		
		if (pSelectedItems[0])
		{
/*			{
				uint16 Surface1, Surface2;
				uint8 NumTotal, NumLoaded;
				uint32 Extra;
				if(pSelectedItems[0]->UpdateAmmoBarInfo(Char(), Surface1, Surface2, NumTotal, NumLoaded, Extra))
				{
					pCD->m_Ammo_Surface1 = Surface1;
					pCD->m_Ammo_Surface2 = Surface2;
					pCD->m_Ammo_NumTotal = NumTotal;
					pCD->m_Ammo_NumLoaded = NumLoaded;
					pCD->m_Ammo_Extra = Extra;
				}
				else
				{
					pCD->m_Ammo_Surface1 = 0;
				}
			}*/
			
/*			if (pSelectedItems[0]->m_Flags & RPG_ITEM_FLAGS_USEABLE)
				pCD->m_Item0_Icon = (uint16)pSelectedItems[0]->m_iIconSurface;
			else
				pCD->m_Item0_Icon = (uint16)pSelectedItems[0]->m_iIconSurface | PLAYER_ICONFLAGS_DISABLED;*/
			
			pCD->m_Item0_Num = (uint16)pSelectedItems[0]->GetAmmo(Char());
			pCD->m_Pickup_Magazine_Num = (uint16)pSelectedItems[0]->GetAmmo(Char(), 1);
			if(pCD->m_Pickup_Magazine_Num == 0)
			{
				int nItems = Char()->GetNumItems(0);
				int iType = pSelectedItems[0]->m_iItemType;
				int nGuns = 0;
				for(int i = 0; i < nItems; i++)
				{
					CRPG_Object_Item *pItem = Char()->GetItemByIndex(0, i);
					if(pItem->m_iItemType == iType)
						nGuns++;
				}
				pCD->m_Pickup_Magazine_Num = nGuns - 1;

			}
//			pCD->m_Item0_Flags = (uint8)pSelectedItems[0]->GetRefreshFlags(pCD->m_Item0_Flags);
			if (pEquippedItems[0] != NULL)
			{
				// This should only be updated when updating model (which is done by UpdateItemModels).
				// So, if we don't update it here then the flag set by UpdateItemModels will remain.
				int Mask = RPG_ITEM_FLAGS_NORENDERMODEL | RPG_ITEM_FLAGS_EQUIPPED;

				// We need to update equipped flag for consumed ammo/items (such as bombs)
				// since they are hidden when unequipped (thrown).
				if (pEquippedItems[0]->m_Flags & RPG_ITEM_FLAGS_CONSUME)
					Mask &= ~RPG_ITEM_FLAGS_EQUIPPED;

				pCD->m_Item0_Flags = (pCD->m_Item0_Flags & Mask) | ((uint8)pEquippedItems[0]->m_Flags & ~Mask);
			}

/*			pCD->m_Item0_Flags = pCD->m_Item0_Flags & ~RPG_ITEM_FLAGS_RENDERQUANTITY;
			pCD->m_Item0_Flags = pCD->m_Item0_Flags | (pSelectedItems[0]->m_Flags & RPG_ITEM_FLAGS_RENDERQUANTITY);*/
		}
		else
		{
//			pCD->m_Item0_Icon = 0;

			if (pEquippedItems[0] != NULL)
			{
				// This should only be updated when updating model (which is done by UpdateItemModels).
				// So, if we don't update it here then the flag set by UpdateItemModels will remain.
				int Mask = RPG_ITEM_FLAGS_NORENDERMODEL | RPG_ITEM_FLAGS_EQUIPPED;

				// We need to update equipped flag for consumed ammo/items (such as bombs)
				// since they are hidden when unequipped (thrown).
				if (pEquippedItems[0]->m_Flags & RPG_ITEM_FLAGS_CONSUME)
					Mask &= ~RPG_ITEM_FLAGS_EQUIPPED;

				pCD->m_Item0_Flags = (pCD->m_Item0_Flags & Mask) | ((uint8)pEquippedItems[0]->m_Flags & ~Mask);
			}
			else
			{
				// Can't remember why we set this to zero. Mayby because we don't have no selected nor equipped item to have flags for.
				//pCD->m_Item0_Flags = 0;
			}
		}
		if (!pSelectedItems[1] && !pEquippedItems[1])
		{
			pSelectedItems[1] = Char()->GetFinalSelectedItem(RPG_CHAR_INVENTORY_ITEMS);
			pEquippedItems[1] = Char()->GetEquippedItem(RPG_CHAR_INVENTORY_ITEMS);
		}

		if (pSelectedItems[1])
		{
/*			if (m_iTemporaryItemType != RPG_ITEMTYPE_UNDEFINED)
			{
				CRPG_Object_Item *pTempItem = Char()->GetInventory(RPG_CHAR_INVENTORY_ITEMS)->FindItemByType(m_iTemporaryItemType);
				if (pTempItem)
					pCD->m_Item1_Icon = (uint16)pTempItem->m_iIconSurface | PLAYER_ICONFLAGS_HIGHLITE;
			}
			else
			{
				if (pSelectedItems[1]->m_Flags & RPG_ITEM_FLAGS_USEABLE)
					pCD->m_Item1_Icon = (uint16)pSelectedItems[1]->m_iIconSurface;
				else
					pCD->m_Item1_Icon = (uint16)pSelectedItems[1]->m_iIconSurface | PLAYER_ICONFLAGS_DISABLED;
			}*/

			pCD->m_Item1_Num = (uint16)pSelectedItems[1]->GetAmmo(Char());
//			pCD->m_Item1_Flags = (uint8)pSelectedItems[1]->GetRefreshFlags(pCD->m_Item1_Flags);
			if (pEquippedItems[1] != NULL)
			{
				// This should only be updated when updating model (which is done by UpdateItemModels).
				// So, if we don't update it here then the flag set by UpdateItemModels will remain.
				int Mask = RPG_ITEM_FLAGS_NORENDERMODEL | RPG_ITEM_FLAGS_EQUIPPED;

				// We need to update equipped flag for consumed ammo/items (such as bombs)
				// since they are hidden when unequipped (thrown).
				if (pEquippedItems[1]->m_Flags & RPG_ITEM_FLAGS_CONSUME)
					Mask &= ~RPG_ITEM_FLAGS_EQUIPPED;

				pCD->m_Item1_Flags = (pCD->m_Item1_Flags & Mask) | ((uint8)pEquippedItems[1]->m_Flags & ~Mask);
			}

//			if (m_Player.m_iTemporaryItemType != RPG_ITEMTYPE_UNDEFINED)
//				pCD->m_Item1_Flags = pCD->m_Item1_Flags & ~RPG_ITEM_FLAGS_CANBLOCK;
			
/*			pCD->m_Item1_Flags = pCD->m_Item1_Flags & ~RPG_ITEM_FLAGS_RENDERQUANTITY;
			pCD->m_Item1_Flags = pCD->m_Item1_Flags | (pSelectedItems[1]->m_Flags & RPG_ITEM_FLAGS_RENDERQUANTITY);*/
		}
		else
		{
/*			if (m_iTemporaryItemType != RPG_ITEMTYPE_UNDEFINED)
			{
				CRPG_Object_Item *pTempItem = Char()->GetInventory(RPG_CHAR_INVENTORY_ITEMS)->FindItemByType(m_iTemporaryItemType);
				if (pTempItem)
					pCD->m_Item1_Icon = (uint16)pTempItem->m_iIconSurface | PLAYER_ICONFLAGS_HIGHLITE;
			}
			else
				pCD->m_Item1_Icon = 0;*/

			if (pEquippedItems[1] != NULL)
			{
				// This should only be updated when updating model (which is done by UpdateItemModels).
				// So, if we don't update it here then the flag set by UpdateItemModels will remain.
				int Mask = RPG_ITEM_FLAGS_NORENDERMODEL | RPG_ITEM_FLAGS_EQUIPPED;

				// We need to update equipped flag for consumed ammo/items (such as bombs)
				// since they are hidden when unequipped (thrown).
				if (pEquippedItems[1]->m_Flags & RPG_ITEM_FLAGS_CONSUME)
					Mask &= ~RPG_ITEM_FLAGS_EQUIPPED;

				pCD->m_Item1_Flags = (pCD->m_Item1_Flags & Mask) | ((uint8)pEquippedItems[1]->m_Flags & ~Mask);
			}
			else
			{
				// Can't remember why we set this to zero. Mayby because we don't have no selected nor equipped item to have flags for.
				pCD->m_Item1_Flags = 0;
			}

//			if (m_Player.m_iTemporaryItemType != RPG_ITEMTYPE_UNDEFINED)
//				pCD->m_Item1_Flags = pCD->m_Item1_Flags & ~RPG_ITEM_FLAGS_CANBLOCK;
		}
	}

	if(pCD->m_DemonHeadTargetFade > -1)
	{
		fp32 Target = (fp32)pCD->m_DemonHeadTargetFade;
		if(pCD->m_DemonHeadCurrentFade < Target)
		{
			pCD->m_DemonHeadCurrentFade = pCD->m_DemonHeadCurrentFade + m_pWServer->GetGameTickTime() * 2.0f;
			pCD->m_DemonHeadCurrentFade = Min(pCD->m_DemonHeadCurrentFade.m_Value, Target);
		}
		else if(pCD->m_DemonHeadCurrentFade > Target)
		{
			pCD->m_DemonHeadCurrentFade = pCD->m_DemonHeadCurrentFade - m_pWServer->GetGameTickTime() * 2.0f;
			pCD->m_DemonHeadCurrentFade = Max(pCD->m_DemonHeadCurrentFade.m_Value, Target);
		}
	}

	/*int target = m_pWServer->Registry_GetServer()->GetValuei("CHAR_TARGET", 1);

	// Get target. Bots don't use autotargeting, and always has max accuracy
	if (IsBot())
	{
		//Bot set accuracy by themselves
		//pCD->Target_SetAccuracy(255);
	}
	else if (target)
	{
		OnRefresh_Target();
	};*/
}

//Checks if the given object is a character, and returns a character-pointer if so, else NULL
CWObject_Character * CWObject_Character::IsCharacter(CWObject_CoreData * _pObj)
{
	MAUTOSTRIP(CWObject_Character_IsCharacter1, NULL);

	//Check objects physstate's object flags
	if ( _pObj && 
		 (&(_pObj->GetPhysState())) &&
		 ((_pObj->GetPhysState()).m_ObjectFlags & OBJECT_FLAGS_CHARACTER) )
		return (CWObject_Character*)_pObj;
	else
		return NULL;
};


//Checks if the given server index points to a charactre on the given server, and returns a 
//character-pointer if so, else NULL
CWObject_Character * CWObject_Character::IsCharacter(int iObj, CWorld_PhysState * _pPhysstate)
{
	MAUTOSTRIP(CWObject_Character_IsCharacter2, NULL);
	if (_pPhysstate)
		return IsCharacter(_pPhysstate->Object_GetCD(iObj));
	else
		return NULL;
};



//Checks if the given character is AI-controlled at the moment
bool CWObject_Character::IsBot()
{
	MAUTOSTRIP(CWObject_Character_IsBot, false);
	if(!this)
		return false;

	if (!m_pWServer)
		return false;
	
	const CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if (!pCD) return false;
	
	return (pCD->m_iPlayer == -1/* || m_pWServer->Game_GetObject()->Player_GetClient(pCD->m_iPlayer) < 0*/);
};


//Checks if the character has a corresponding player
bool CWObject_Character::IsPlayer()
{
	MAUTOSTRIP(CWObject_Character_IsPlayer, false);
	const CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if (!pCD) return false;

	return pCD->m_iPlayer != -1;
};


//Gives an order to all controlled bots
#ifndef M_RTM

void CWObject_Character::GiveOrder(int _Order)
{
	MAUTOSTRIP(CWObject_Character_GiveOrder, MAUTOSTRIP_VOID);
	if (!m_pWServer || (m_lControlledBots.Len() == 0))
		return;

	CWObject_Character * pBot;
	for (int i = 0; i < m_lControlledBots.Len(); i++)
	{	
		if ((pBot = IsCharacter(m_pWServer->Object_Get(m_lControlledBots[i]))))
			if (_Order)
				pBot->m_iBotControl |= 1 << (_Order - 1);
			else
				//Reset order is NULL
				pBot->m_iBotControl = NULL;
	};
};

#endif

// Get visibility factor of character. 
// _bLightsOut indicate if the level should be complete darkness (argh,om
// _bLightsOut är satt till true så låtsas vi att hela kartan är helt mörk)
fp32 CWObject_Character::GetVisibility(bool _bLightsOut,int32 _PerceptionFlags)
{
	CWO_Character_ClientData* pCD = GetClientData(this);
	if (!pCD)
		return 0.0f;

	if (_PerceptionFlags & PERC_NIGHTVISION_FLAG)
	{	// Night vision sees everything
		return(1.0f);
	}

	//Default value (1.0) is for a character standing still in good lighting conditions
	//A moving character will have a slightly higher value while a guy in a Predator suit
	//would have visibility 0.1 or lower. A guy carrying a flare might have a value of 
	//around 20. Completely invisible is of course 0.

	fp32 ret = 1.0f;

	if (_bLightsOut)
	{
		ret = 0.0f;		// Lights are out, Riddick is invisible
	}
	else
	{	// Standard lights in this level
		if (m_Player.m_SneakTriggerCount > 0)
		{	// Player is in sneak trigger so if he isn't running
			// he should basically be invisible
			ret = 0.0f;
		}
		else
		{
			if (m_LightIntensity > 1.0f)
			{
				ret = 1.0f;
			}
			else
			{
				ret = m_LightIntensity;
			}
		}
	}

	// Has the visibility been raised by the item being used?
	if (m_RaisedVisibility > ret)
	{
		ret = m_RaisedVisibility;
	}

	return ret;
};


//Get noise factor of character.
//NOTE The reason for the somewhat weird looking code nelow is similarity with GetVisibility above and
// the possibility thet there may be other factors in CWObject_Character that would add to noise
fp32 CWObject_Character::GetNoise()
{
	fp32 ret = 0;

	// Has the noiselevel been raised by the item being used?
	if (m_RaisedNoiseLevel > ret)
	{
		ret = m_RaisedNoiseLevel;
		m_RaisedNoiseLevel = 0;
	}
	
	return ret;
};



//Refresh stealth mode stuff
void CWObject_Character::OnRefresh_Stealth()
{
	MAUTOSTRIP(CWObject_Character_OnStealthRefresh, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_Character::OnRefresh_Stealth);
	CWO_Character_ClientData * pCD = CWObject_Character::GetClientData(this);
	if (!pCD)
		return;

	//Are we capable of stealth mode?
	if (m_iStealthActivateTime > 0)
	{
		//Are we in stealth mode?
		if (pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_STEALTH)
		{
			//Exit stealth mode if we're attacking, using item, jumping or moving faster than 4 units
			if ((pCD->m_Control_Press & (CONTROLBITS_PRIMARY | CONTROLBITS_SECONDARY | CONTROLBITS_JUMP)) ||
				(GetPosition().DistanceSqr(GetLastPosition()) > Sqr(4)))
			{
				pCD->m_ExtraFlags = pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_STEALTH;
				m_iStealthTimer = m_iStealthActivateTime;
			};
		}
		else 
		{
			//Check if we have violated stealth requirements, and must reset timer
			if ((pCD->m_Control_Press & (CONTROLBITS_PRIMARY | CONTROLBITS_SECONDARY | CONTROLBITS_JUMP)) ||
				(GetPosition().DistanceSqr(GetLastPosition()) > Sqr(0.1f)))
			{
				//We moved or used an item. Reset timer.
				m_iStealthTimer = m_iStealthActivateTime;
			}
			//Only count down timer and give the chance to enter stealth mode every 10 frames
			else if ((pCD->m_GameTick % 10) == 0)
			{
				//Check if anyone has spotted us, and if so is close or have a LOS
				bool bStealth = true;
				CAI_AgentInfo * pInfo;
				for (int i = 0; i < m_spAI->m_KB.NumAgentInfo(); i++)
				{
					pInfo = m_spAI->m_KB.IterateAgentInfo(i);
					if (pInfo &&
						pInfo->GetObject() &&
						(pInfo->GetRelation() >= CAI_AgentInfo::ENEMY) &&
						(pInfo->GetAwarenessOfUs() >= CAI_AgentInfo::SPOTTED) &&
						((pInfo->GetObject()->GetPosition().DistanceSqr(GetPosition()) < 400) ||
						 pInfo->InLOS()))
					{
						//Enemy which breaks constraints have been found!
						bStealth = false;
						break;
					};
				};

				if (bStealth)
				{
					//Stealth requirements fulfilled!
					if (m_iStealthTimer <= 0)
					{
						//Enter stealth mode
						pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_STEALTH;
					}
					else
					{
						//Count down timer
						m_iStealthTimer -= 10;
					};
				}
				else
				{
					//Stealth requirments not fulfilled. Reset timer.
					m_iStealthTimer = m_iStealthActivateTime;
				};
			};
		}
	}
	//Are we in stealth mode even though we shouldn't be able to?
	else if (pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_STEALTH)
	{
		pCD->m_ExtraFlags = pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_STEALTH;
	};
};


int CWObject_Character::GetHitlocation(const CCollisionInfo* _pCInfo)
{
	uint8 iBone = _pCInfo->m_LocalNode;
	switch (iBone)
	{
	// low
	case PLAYER_ROTTRACK_RLEG:
	case PLAYER_ROTTRACK_RKNEE:
	case PLAYER_ROTTRACK_RFOOT:
	case PLAYER_ROTTRACK_LLEG:
	case PLAYER_ROTTRACK_LKNEE:
	case PLAYER_ROTTRACK_LFOOT:
		return SHitloc::HITLOC_LOW;

	// mid
	case PLAYER_ROTTRACK_SPINE:
	case PLAYER_ROTTRACK_TORSO:
	case PLAYER_ROTTRACK_RSHOULDER:
	case PLAYER_ROTTRACK_RARM:
	case PLAYER_ROTTRACK_RELBOW:
	case PLAYER_ROTTRACK_SPINE2:
	case PLAYER_ROTTRACK_RHAND:
	case PLAYER_ROTTRACK_LSHOULDER:
	case PLAYER_ROTTRACK_LARM:
	case PLAYER_ROTTRACK_LELBOW:
	case PLAYER_ROTTRACK_LHAND:
	case PLAYER_ROTTRACK_RHANDATTACH:
	case PLAYER_ROTTRACK_LHANDATTACH:
		return SHitloc::HITLOC_MID;

	// high
	case PLAYER_ROTTRACK_NECK:
	case PLAYER_ROTTRACK_HEAD:
	case PLAYER_ROTTRACK_JAW:
	case PLAYER_ROTTRACK_CAMERA:
		return SHitloc::HITLOC_HIGH;

	// fallback
	default:
		return SHitloc::HITLOC_MID;
	}
}


// Returns the damage factor from a hit of a certain (single) _DamageType. DAMAGETYPE_UNDEFINED works as all types
fp32 CWObject_Character::GetHitlocDamageFactor(uint32 _DamageType, int _HitLoc,int _SurfaceType, int _Damage, int _iSender)
{
	MAUTOSTRIP(CWObject_Character_GetDamageFactor, 0.0f);
	MSCOPESHORT(CWObject_Character::GetHitlocDamageFactor);

	// If surface type is metal/metal plate (60/61) (on swat shield for example) then we have no damage
	if (_SurfaceType == 61 || _SurfaceType == 60)
		return 0.0f;

	if ((_SurfaceType == -1) && (_DamageType == 0))
	{
		return(1.0f);
	}

//	bool bFoundHitloc = false;
//	bool bFoundSurface = false;
	fp32 Tweak = 1.0f;
	if (IsPlayer())
	{
		if (_DamageType & DAMAGETYPE_MELEE_MASK)
		{
			Tweak = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_TWEAK_DAMAGE_MELEE, _iSender),m_pWServer->Game_GetObjectIndex()) / 1024.0f;
		}
		else	// Yes DAMAGETYPE_UNDEFINED will be treated as ranged
		{
 			Tweak = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_TWEAK_DAMAGE_RANGED, _iSender),m_pWServer->Game_GetObjectIndex()) / 1024.0f;
		}
	}

	int HitLocCount = m_lHitlocations.Len();
	for (int iLoc = 0; iLoc < HitLocCount; iLoc++)
	{
		if (m_lHitlocations[iLoc].m_Factor < 0.0f)
		{	// m_Factor < 0.0f indicates an noninitialized hitloc
			continue;
		}
		if ((_DamageType != DAMAGETYPE_UNDEFINED)&&(!(_DamageType & m_lHitlocations[iLoc].m_DamageTypes)))
		{	// All damagetypes are valid for generic damage.	
			continue;
		}
		if ((_HitLoc != SHitloc::HITLOC_ALL)&&(!(_HitLoc & m_lHitlocations[iLoc].m_Hitloc)))
		{	// All hitloc are valid for generic damage.	
			continue;
		}
		if ((_SurfaceType != -1)&&(m_lHitlocations[iLoc].m_SurfaceType != -1)&&(_SurfaceType != m_lHitlocations[iLoc].m_SurfaceType))
		{	// Surface type mismatch	
			continue;
		}
		
		int Armor = m_lHitlocations[iLoc].m_Armour;
		fp32 Factor = m_lHitlocations[iLoc].m_Factor;
		if ((_Damage > 0) && (Armor > 0))
		{
			if (_Damage > Armor)
			{
				return(Tweak * Factor * (1.0f - Armor / _Damage));
			}
			else
			{	// Armor absorbed the hit completely
				return(0.0f);
			}
		}
		else
		{
			return(Tweak * Factor);
		}
	}

	return (Tweak * 1.0f);
}

/*
// Returns the damage factor based on _SurfaceType(hitlocation), _DamageType [and _Damage]
// Method finds the first matching surface that matches _DamageType OR the first matching surface
// if _DamageType == 0. If the optional _Damage > 0 and the surface also has an amrmor value > 0
// then the damage factor will be calculated as if Factor * (Damage - Armor) was used.
fp32 CWObject_Character::GetDamageFactor(int _SurfaceType, uint32 _DamageType, int _Damage,int* _pFlags)
{
	MAUTOSTRIP(CWObject_Character_GetDamageFactor, 0.0f);

	if ((_SurfaceType == -1) && (_DamageType == 0))
	{
		return(1.0f);
	}

	fp32 Tweak = 1.0f;
	uint32 DamageType = _DamageType;

	if (IsPlayer())
	{
		if (DamageType & DAMAGETYPE_MELEE_MASK)
		{
			Tweak = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_TWEAK_DAMAGE_MELEE),m_pWServer->Game_GetObjectIndex()) / 1024.0f;
		}
		else
		{
			Tweak = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_TWEAK_DAMAGE_RANGED),m_pWServer->Game_GetObjectIndex()) / 1024.0f;
		}
	}

 	for (int iLoc = 0; iLoc < m_lArmorSurfaces.Len(); iLoc++)
	{
		if (((_SurfaceType == -1)||(_SurfaceType == m_lArmorSurfaces[iLoc].m_SurfaceType))&&
			((DamageType == 0)||(DamageType & m_lArmorSurfaces[iLoc].m_DamageTypes)))
		{
			int Armor = m_lArmorSurfaces[iLoc].m_Armor;
			fp32 Factor = m_lArmorSurfaces[iLoc].m_Factor;
			if ((_Damage > 0) && (Armor > 0))
			{
				if (_Damage > Armor)
				{
					return(Tweak * Factor * (1.0f - Armor / _Damage));
				}
				else
				{	// Armor absorbed the hit completely
					return(0.0f);
				}
			}
			else
			{
				return(Tweak * Factor);
			}
		}
	}

	return (Tweak * 1.0f);
}
*/

// Multiply damage factor for the given hit location and damage type(s) to the given value.
// (As value originally is 1.0 this will practically be the same as setting the value)
void CWObject_Character::SetHitlocDamageFactor(int _iSlot, fp32 _Factor, uint32 _DamageTypes, int _HitLoc, int _SurfaceType, int _Armour)	
{
	if (_iSlot >= m_lHitlocations.Len())
	{
		for (int i = m_lHitlocations.Len(); i <= _iSlot; i++)
		{
			SHitloc newLoc;
			newLoc.m_Factor = -1.0f;
			m_lHitlocations.Add(newLoc);
		}
	}
	
	// ***
//	int N = m_lHitlocations.Len();
	// ***

	// (re)set the slot
	m_lHitlocations[_iSlot].m_Factor = _Factor;
	m_lHitlocations[_iSlot].m_DamageTypes = _DamageTypes;
	m_lHitlocations[_iSlot].m_Hitloc = _HitLoc;
	m_lHitlocations[_iSlot].m_SurfaceType = _SurfaceType;
	m_lHitlocations[_iSlot].m_Armour = _Armour;
}

static fp32 GetHeading(const CVec3Dfp32& _Player, const CVec3Dfp32& _Target)
{
	//Get angle in radians
	CVec3Dfp32 RelPos = _Player - _Target;
	RelPos[2] = 0; 
	RelPos.Normalize();
	fp32 Angle = M_ACos(RelPos[0]);
	if (RelPos[1] > 0) 
		Angle = -Angle;

	//Convert angle to fractions
	Angle *= ( 1.0f / _PI2 );

	return Angle;
}
/*
static fp32 GetPitch(const CVec3Dfp32& _Player, const CVec3Dfp32& _Target)
{
	//Get angle in radians
	CVec3Dfp32 RelPos = _Player - _Target;
	RelPos.Normalize();
	fp32 Angle = M_ASin(RelPos[2]);

	Angle *= ( 1.0f / _PI2 );
	return Angle;
}
*/
fp32 GetFOVFactor(const CVec3Dfp32& _Pos, const CVec3Dfp32& _Target, fp32 _Heading, fp32 _Pitch)
{
	// Falculate FOV for heading
	fp32 Heading = GetHeading(_Pos, _Target);
	Heading -= _Heading;
	while(Heading < 0.0f) Heading += 1.0f;
	Heading = (0.5f - M_Fabs(Heading - 0.5f)) * 2.0f;		// Now factFOV should be 1.0f when enemy is dead-on, descending do 0 when enemy is behind
//	if (Heading < 0.9f) Heading = 0.0f;					// Enemy to far off screen.
	if (Heading < 0.8f) Heading = 0.0f;					// Enemy to far off screen.

/*
	// Only 2D for now
	fp32 Cone = M_Cos(_Pitch) * 0.4f;
	fp32 Pitch = -GetPitch(_Pos, _Target);
	if((Pitch > -_Pitch)
	&& (Pitch < _Pitch))
	{
		Pitch = 1.0f;
	} else {
		Pitch = 0.0f;
	}
*/

	fp32 FOVFactor = Heading;// * Pitch;
	return FOVFactor;
}

class CWO_SortEntry
{
public:
	fp32 Factor;
	int16 iObject;
};

bool CWObject_Character::Aim_IsValidTarget(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, int _iTarget, fp32 &_Angle)
{
	CWObject_CoreData *pTarget = _pWPhysState->Object_GetCD(_iTarget);
	if (!pTarget)
		return false;
	else
	{
		bool bChar = (pTarget->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER) != 0;
		if(bChar)
		{
			CWO_Character_ClientData *pCD = GetClientData(pTarget);
			if((pCD && ((pCD->m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_BERAGDOLL))) || pCD->m_iPlayer != -1)
				return false;
		}

		bool bObjectHasAutoAim = false;
		if (_pWPhysState->IsServer())
		{
			// We only got this information on the server so we can only check it here. (sometimes this will cause a little jerk)
			if(bChar)
			{
				if (((CWObject_Character *)pTarget)->m_Flags & PLAYER_FLAGS_NOAUTOAIMATARGET)
					return false;
			}	
			
			
			CWObject_Interface_AI* pAI = ((CWObject*)pTarget)->GetInterface_AI();
			if (bChar && !(pAI && pAI->AI_IsAlive()))
				return false;

			CWorld_Server* pWServer = safe_cast<CWorld_Server>(_pWPhysState);
			CWObject_Object* pObjObj = pWServer ? TDynamicCast<CWObject_Object>(pWServer->Object_Get(_iTarget)) : NULL;
			if(pObjObj && ((pObjObj->GetFlags() & CWObject_Object::FLAGS_AUTOAIM)))
				bObjectHasAutoAim = true;
		}

		{
			CWO_Character_ClientData *pCDPlayer = GetClientData(_pObj);
			CVec3Dfp32 TargetPos;
			fp32 UseTargetMaxDist = TARGET_MAXDIST;
			fp32 UseTargetCloseAngle = TARGET_CLOSEANGLE;
			fp32 UseTargetFarAngle = TARGET_FARANGLE;

			// Check if the object is a lamp
			bool bObject = (pTarget->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_OBJECT) != 0;
			bool bIsLamp = pTarget->IsClass(class_Object_Lamp);
			
			//bool bLocked = true;
			if(bObject)
			{
				if(!bIsLamp && !bObjectHasAutoAim)
					return false;
				else if(bIsLamp)
				{
					// check if broken
					uint32 LampFlags = pTarget->m_Data[1]; // DATA_LAMP_FLAGS = 1
					if(LampFlags & M_Bit(1)) // FLAGS_LAMP_STATE_BROKEN = M_Bit(1)
						return false;
				}
				
				if(_iTarget != pCDPlayer->m_AimTarget)
				{
					UseTargetFarAngle = TARGET_FARANGLE_LAMP_LOCKTRESH;
					UseTargetCloseAngle = TARGET_CLOSEANGLE_LAMP_LOCKTRESH;
					//bLocked = false;
				}
				else
				{
					UseTargetCloseAngle = TARGET_FARANGLE_LAMP;
					UseTargetFarAngle = TARGET_FARANGLE_LAMP;
				}

				UseTargetMaxDist = TARGET_MAXDIST_LAMP;				
			}
			else if(_iTarget != pCDPlayer->m_AimTarget)
			{
				UseTargetFarAngle = TARGET_FARANGLE_LOCKTRESH;
				UseTargetCloseAngle = TARGET_CLOSEANGLE_LOCKTRESH;
				//bLocked = false;
			}
			
			aint Ret =_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETAUTOAIMOFFSET), _iTarget);
			if(Ret)
			{
				TargetPos = CVec3Dfp32().Unpack32(Ret, 256.0f);
				TargetPos *= pTarget->GetPositionMatrix();
			}
			else
			{
				TargetPos = pTarget->GetPosition();
			}

			if(bChar && (Char_GetPhysType(pTarget) != PLAYER_PHYS_CROUCH))
			{
				TargetPos.k[2] += 10.0f;
			}

			// Check if the target is a valid target still
			// Pos is look-pos. I get it from the postanim system since i know that one is at least correct 
			//CWO_Character_ClientData *pCDPlayer = GetClientData(_pObj);
			CVec3Dfp32 Pos = pCDPlayer->m_PostAnimSystem.GetCameraLookPos();
			CVec3Dfp32 Fwd = pCDPlayer->m_PostAnimSystem.GetCameraLookDir();
			CVec3Dfp32 Dir = TargetPos - Pos;
			fp32 TargetDistance = Dir.Length();
			Dir = Dir / TargetDistance;

			if( TargetDistance > UseTargetMaxDist)
				return false;
			else
			{
				_Angle = Dir * Fwd;
				fp32 MaxAngle = UseTargetCloseAngle + (TargetDistance/UseTargetMaxDist) * (UseTargetFarAngle-UseTargetCloseAngle);
				fp32 tmp = 1.0f - MaxAngle;
				MaxAngle += tmp * (1.0f-pCDPlayer->m_AutoAimVal);

				//--------------------------------------------------------------------------------------------
				// debug
				/*{
					fp32 ActAngle = M_ACos(_Angle);
					fp32 ActMaxAngle = M_ACos(MaxAngle);
					fp32 AutoTargetRadius;
					AutoTargetRadius = M_ASin(ActMaxAngle) * TargetDistance;
					CMat4Dfp32 TargetSphere;
					TargetSphere.Unit();
					TargetSphere.GetRow(0) = Dir;
					TargetSphere.RecreateMatrix(0,2);
					TargetSphere.GetRow(3) = TargetPos;

					_pWPhysState->Debug_RenderWire(Pos, (Pos+(Fwd*2048.0f)), 0x0ffffffff);
					_pWPhysState->Debug_RenderSphere(TargetSphere, AutoTargetRadius, bLocked ? 0xFFFF0000 : 0x0FF00FF00);
				}*/
				//--------------------------------------------------------------------------------------------

				if(_Angle  < MaxAngle)
					return false;
				else
				{
					// Check to make sure nothing is obstructing the line of aim from us to our target
					CCollisionInfo CInfo;
					CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
					int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
					int32 ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
					int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
					
					CVec3Dfp32 TraceStart = Pos + (Fwd*10.0f);

					if(bChar)
					{
						CWO_Character_ClientData *pCD = GetClientData(pTarget);
						// check against head
						TargetPos = pCD->m_PostAnimSystem.GetCameraLookPos();
						if(_pWPhysState->Phys_IntersectLine(TraceStart, TargetPos, OwnFlags, ObjectFlags, MediumFlags, &CInfo, _iTarget, true))
							return false;
					}
					else
					{
						if(_pWPhysState->Phys_IntersectLine(TraceStart, TargetPos, OwnFlags, ObjectFlags, MediumFlags, &CInfo, _iTarget, true))
							return false;
					}
				}
			}
		}


	}
	return true;
}


#define MAXOBJECTS  20
int16 CWObject_Character::Aim_FindTarget(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, int16* _pExcludeObjectList, int _ExcludeListLength)
{
	MSCOPE(CWObject_Character::Aim_FindTarget, CHARACTER);
	const int16* pSelectionList;
	int nObjects;
	TSelection<CSelection::LARGE_BUFFER> Selection;

	uint AimAtThese = OBJECT_FLAGS_CHARACTER;
	AimAtThese |= OBJECT_FLAGS_OBJECT;
	//AimAtThese |= OBJECT_FLAGS_PICKUP;

	_pWPhysState->Selection_AddBoundSphere(Selection, AimAtThese, _pObj->GetPosition() + CVec3Dfp32(0,0,54), TARGET_MAXDIST);
	_pWPhysState->Selection_RemoveOnNotFlagsSet(Selection, AimAtThese);
	_pWPhysState->Selection_RemoveObjects(Selection, _pExcludeObjectList, _ExcludeListLength);
	nObjects = _pWPhysState->Selection_Get(Selection, &pSelectionList);


	// Now we have a list of potential targets
	int16 iReturnObject = 0;
	int iObject;
	fp32 MaxFactor = 0.0f;
	fp32 TargetAngle;
	bool bFoundHumanTarget = false;
	for (iObject=0; iObject<nObjects; iObject++)
	{
		int16 iTargetObj = pSelectionList[iObject];
		CWObject_CoreData *pTarget = _pWPhysState->Object_GetCD(iTargetObj);

		bool bIsObject = (pTarget->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_OBJECT) != 0;
		if(!bIsObject || (bIsObject && !bFoundHumanTarget))
		{
			if (iTargetObj != _pObj->m_iObject && Aim_IsValidTarget(_pObj, _pWPhysState, iTargetObj, TargetAngle))
			{
				// first human target. discard all objects MaxFactor
				if(!bFoundHumanTarget && (pTarget->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER) != 0) 
				{
					bFoundHumanTarget = true;
					MaxFactor = 0.0f;
				}

				if( TargetAngle > MaxFactor )
				{
					MaxFactor = TargetAngle;
					iReturnObject = iTargetObj;
				}
			}
		}
	}
	
	return iReturnObject;
}

void CWObject_Character::Aim_GetMatrix(CMat4Dfp32& _Matrix)
{
	CWO_Character_ClientData *pCD = GetClientData(this);
	if (!pCD || pCD->m_AimTarget == 0)
		return;

	const CVec3Dfp32& Player = GetPosition();
	const CVec3Dfp32& Target = m_pWServer->Object_GetPosition(pCD->m_AimTarget);

	_Matrix.Unit();
	Player.SetMatrixRow(_Matrix, 3);

	CVec3Dfp32 Dir = Target - Player;
	Dir.SetMatrixRow(_Matrix, 0);
	_Matrix.RecreateMatrix(0, 2);
}

void CWObject_Character::Aim_Refresh(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	MSCOPE(CWObject_Character::Aim_Refresh, CHARACTER);

	CWO_Character_ClientData *pCD = GetClientData(_pObj);
	if (!pCD)
		return;
	fp32 Rubbish;


	if(pCD->m_AimReleasingTarget && pCD->m_AimTargetStartTick <= _pWPhysState->GetGameTick())
	{
		pCD->m_AimReleasingTarget = false;
//		int iTarget = pCD->m_AimTarget;
		pCD->m_AimTarget = 0;
	}
	if(!pCD->m_AimReleasingTarget && pCD->m_AimTarget && !Aim_IsValidTarget(_pObj, _pWPhysState, pCD->m_AimTarget, Rubbish))
	{
		pCD->m_AimReleasingTarget = true;
		pCD->m_AimTargetStartTick = _pWPhysState->GetGameTick()+2; // Release target in two ticks
//		int iTarget = pCD->m_AimTarget;
	}
	else if (!pCD->m_AimTarget)
	{
		pCD->m_AimTarget = Aim_FindTarget(_pObj, _pWPhysState, NULL, 0);
		if(pCD->m_AimTarget)
		{ 
			pCD->m_AimTargetStartTick = _pWPhysState->GetGameTick()+2; // Start using target in two ticks
//			int iTarget = pCD->m_AimTarget; 
		}
	}

	if(pCD->m_WeaponZoomState == WEAPON_ZOOMSTATE_IN)
	{
		fp32 Diff = _pWPhysState->GetGameTick() - pCD->m_WeaponZoomStartTick;
		fp32 ZoomDuration = 0.1f * _pWPhysState->GetGameTicksPerSecond();
		fp32 ZoomMod = ((fp32)Diff / ZoomDuration);
		ZoomMod = Min(ZoomMod, 1.0f);
		ZoomMod = Max(ZoomMod, 0.0f);
		ZoomMod = M_Sin((ZoomMod*_PI*0.25f));

		CVec3Dfp32 LookAtPos;
		LookAtPos = pCD->m_PostAnimSystem.GetAimTargetPos();	
		CVec3Dfp32 Direction = (LookAtPos - pCD->m_PostAnimSystem.GetCameraLookPos()).Normalize();
		CVec2Dfp32 DirectionAngles = pCD->Target_GetAnglesForVector(Direction);
		pCD->m_Control_Look_Wanted.k[2] = DirectionAngles.k[1];
		pCD->m_Control_Look_Wanted.k[1] = DirectionAngles.k[0];		
	}
}

fp32 CWObject_Character::GetHeartBeat(CWObject_CoreData *_pObj, CWorld_Client* _pWClient, const CMTime& _Time, bool _bRefresh)
{
	CWO_Character_ClientData *pCD = GetClientData(_pObj);
	if(!pCD)
		return 0;

	if(pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOWORLDCOLL)
		return 0;

	static fp32 sHeartBeat[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.3f, 0.7f, 0, 0.2f, 0.3f, 0.25f, 0.20f, 0.15f, 0.10f, 0.05f, 0 };
	CMTime Time = _Time + CMTime::CreateFromSeconds(fp32(_pObj->m_iObject ^ 0x2EB5) * 0.05f);
	fp32 fTime0 = Time.GetTimeModulusScaled(20.0f, 20.0f);
	int iTime0 = TruncToInt(fTime0);

	if(_bRefresh)
	{
		if(iTime0 == 8)
		{
			Char_PlayDialogue_Hash(_pObj, _pWClient, PLAYER_DIALOGUE_HEARTBEAT01);
		}
		else if(iTime0 == 12)
		{
			Char_PlayDialogue_Hash(_pObj, _pWClient, PLAYER_DIALOGUE_HEARTBEAT02);
		}
		return 0;
	}
	else
	{
		fp32 TimeFrac = fTime0 - iTime0;
		int iTime1 = iTime0 + 1;
		if(iTime1 >= 20)
			iTime1 = 0;
		return sHeartBeat[iTime0] + (sHeartBeat[iTime1] - sHeartBeat[iTime0]) * TimeFrac;
	}
}

bool CWObject_Character::OnRefresh_WeaponLights_Model(CWO_Character_ClientData* pCD, const uint8& _iAttachModel, const int& _iLight, CWorld_PhysState* _pWPhysState,
													  CXR_SceneGraphInstance* _pSGI, CXR_SkeletonInstance* _pSkelInstance, CXR_Skeleton* _pSkel, const CMTime& _GameTime,
													  CXR_AnimState& _AnimState, CMat4Dfp32* _pMat, const fp32& _IPTime, const bool& _bMuzzleFlash, const bool& _bFlashlight, const int& _iLightningLight)
{
	if(!_bMuzzleFlash && !_bFlashlight && !_iLightningLight)
		return false;

	// Fetch correct item model
    CXR_Model* pModel;
	const bool bValid = (_iAttachModel == 0) ? 
		pCD->m_Item0_Model.GetModel0_RenderInfo(_pWPhysState->GetMapData(), NULL, _pSkelInstance, _pSkel, _GameTime, _AnimState, *_pMat, pModel, _pWPhysState) :
		pCD->m_Item1_Model.GetModel0_RenderInfo(_pWPhysState->GetMapData(), NULL, _pSkelInstance, _pSkel, _GameTime, _AnimState, *_pMat, pModel, _pWPhysState);

	if(bValid)
	{
		int MuzzleFlashTick = 0;
		int32 MuzzleLightColor = 0;
		int32 MuzzleLightRange = 0;
		int32 LightningTick = 0;

		if(_iAttachModel == 0)
		{
			LightningTick = pCD->m_LightningLightTick0;
			MuzzleFlashTick = pCD->m_MuzzleFlashTick0;
			MuzzleLightColor = pCD->m_MuzzleLightColor0;
			MuzzleLightRange = pCD->m_MuzzleLightRange0;
		}
		else
		{
			LightningTick = pCD->m_LightningLightTick1;
			MuzzleFlashTick = pCD->m_MuzzleFlashTick1;
			MuzzleLightColor = pCD->m_MuzzleLightColor1;
			MuzzleLightRange = pCD->m_MuzzleLightRange1;
		}

		if(_bMuzzleFlash)
		{
			CMat4Dfp32 FlashMat = *_pMat;
			CXR_Skeleton* pSkelItem = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
			if(pSkelItem)
			{
				const CXR_SkeletonAttachPoint *pAttach = pSkelItem->GetAttachPoint(1);
				if(pAttach)
					CVec3Dfp32::GetRow(FlashMat, 3) = pAttach->m_LocalPos.GetRow(3) * (FlashMat);
			}
			
			// Drag flash matrix a little bit above and "inwards" to get a nice lighting.
			if(_iAttachModel == 0)
				FlashMat.GetRow(3) += (FlashMat.GetRow(2) * 5) + (FlashMat.GetRow(1) * 5);
			else
				FlashMat.GetRow(3) += (FlashMat.GetRow(2) * 5) + (-FlashMat.GetRow(1) * 5);
			
			fp32 Duration = fp32(int(pCD->m_GameTick) - MuzzleFlashTick) + _IPTime - 0.5f;
			Duration = MinMT(MaxMT(Duration, 0.0f), 3.0f);
			fp32 Amp = ((3.0f - Duration) / 3.0f) * 0.01f;
			if(Amp > 0 && Duration > 0)
			{
				CXR_Light Light(FlashMat, CVec3Dfp32(((MuzzleLightColor >> 16) & 0xff),
													((MuzzleLightColor >>  8) & 0xff),
													((MuzzleLightColor >>  0) & 0xff)) * Amp,
													MuzzleLightRange, 0, CXR_LIGHTTYPE_POINT);
				Light.m_LightGUID = _iLight;
				Light.m_iLight = 0;
				Light.m_Flags = CXR_LIGHT_NOSHADOWS | CXR_LIGHT_NOSPECULAR;
				_pSGI->SceneGraph_Light_LinkDynamic(Light);
			}
			else
			{
				// Unlink light
				const int iLight = _pSGI->SceneGraph_Light_GetIndex(_iLight);
				if(iLight)
					_pSGI->SceneGraph_Light_Unlink(iLight);
			}
		}

		// Setup extra lights (Darkness lightning light)
		if(_iLightningLight)
		{
			CMat4Dfp32 LightningMat = *_pMat;
			CXR_Skeleton* pSkelItem = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
			if(pSkelItem)
			{
				const CXR_SkeletonAttachPoint* pAttach = pSkelItem->GetAttachPoint(5);
				if(pAttach)
					CVec3Dfp32::GetRow(LightningMat, 3) = pAttach->m_LocalPos.GetRow(3) * (LightningMat);
			}

			// Drag lightning matrix a little bit above and "inwards" to get a nice lighting.
			//if(_iAttachModel == 0)
			//	LightningMat.GetRow(3) += (LightningMat.GetRow(2) * 5) + (LightningMat.GetRow(1) * 5);
			//else
			//	LightningMat.GetRow(3) += (LightningMat.GetRow(2) * 5) + (-LightningMat.GetRow(1) * 5);

			int32 Tick = (pCD->m_GameTick - LightningTick);
			fp32 Time = Clamp((fp32(Tick) * _pWPhysState->GetGameTickTime()) * 5.0f, 0.0f, 1.0f);
			fp32 ColorScale = M_Sin(_PI * Time);
			if(ColorScale > 0)
			{
				CXR_Light Light(LightningMat, CVec3Dfp32(1.0f, 1.0f, 0.2f) * ColorScale, 8, 0, CXR_LIGHTTYPE_POINT);
				Light.m_LightGUID = _iLightningLight;
				Light.m_iLight = 0;
				Light.m_Flags = CXR_LIGHT_NOSHADOWS | CXR_LIGHT_NOSPECULAR;
				_pSGI->SceneGraph_Light_LinkDynamic(Light);
			}
			else
			{
				// Unlink
				const int iLight = _pSGI->SceneGraph_Light_GetIndex(_iLightningLight);
				if(iLight)
					_pSGI->SceneGraph_Light_Unlink(iLight);
			}
		}

		// Willbo:	Flashlight dual wield is not really correct. It do add two lights if both items has one, but it only uses the color and range
		//			from the "primary" item.
		if(_bFlashlight)
		{
			CXR_Skeleton* pSkelItem = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
			if(pSkelItem)
			{
				const CXR_SkeletonAttachPoint *pAttach = pSkelItem->GetAttachPoint(3);
				if(pAttach)
					CVec3Dfp32::GetRow(*_pMat, 3) = pAttach->m_LocalPos.GetRow(3) * (*_pMat);
				else
					CVec3Dfp32::GetRow(*_pMat, 3) += CVec3Dfp32::GetRow(*_pMat, 0) * 25.0f;
			}

			CMat4Dfp32 LightPos = *_pMat;
			CVec3Dfp32::GetRow(LightPos, 3) += CVec3Dfp32::GetRow(*_pMat, 0) * -3.0f;
			CXR_Light Light(LightPos, CVec3Dfp32(((pCD->m_FlashLightColor.m_Value >> 16) & 0xff) / 255.0f,
							((pCD->m_FlashLightColor.m_Value >> 8) & 0xff) / 255.0f,
							((pCD->m_FlashLightColor.m_Value >> 0) & 0xff) / 255.0f) * 2,
							pCD->m_FlashLightRange, 0, CXR_LIGHTTYPE_SPOT);
			if(pCD->m_iPlayer != -1)
			{
				Light.m_SpotHeight = M_Tan(50 * (_PI / 180.0f ));
				Light.m_SpotWidth = M_Tan(50 * (_PI / 180.0f ));
			}
			else
			{
				Light.m_SpotHeight = M_Tan(30 * (_PI / 180.0f ));
				Light.m_SpotWidth = M_Tan(30 * (_PI / 180.0f ));
			}
			Light.m_LightGUID = _iLight + 1;
			Light.m_iLight = 0;
			if (pCD->m_iPlayer == -1)
				Light.m_Flags |= CXR_LIGHT_NOSPECULAR;
			_pSGI->SceneGraph_Light_LinkDynamic(Light);

			return true;
		}
	}

	return false;
}

bool CWObject_Character::OnRefresh_WeaponLights(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, const CMat4Dfp32 &_CharPos, fp32 _IPTime, CMat4Dfp32 *_pMat)
{
	CWO_Character_ClientData *pCD = GetClientData(_pObj);
	if(!pCD)
		return false;

	CXR_SceneGraphInstance* pSGI = _pWPhysState->World_GetSceneGraphInstance();
	if (!pSGI)
		return false;

//	int bFlashLightSet = 0;
//	int bMuzzleFlashSet = 0;
	bool bRet = false;

	const bool bFlashLight = (_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_FLASHLIGHT || pCD->m_Item0_Flags & RPG_ITEM_FLAGS_FLASHLIGHT) && !(pCD->m_ActionCutSceneCameraMode & CActionCutsceneCamera::ACS_CAMERAMODE_ACTIVE) && !(pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOITEMRENDER);
	const bool bMuzzleFlash0 = pCD->m_MuzzleFlashTick0 > pCD->m_GameTick - 4;
	const bool bMuzzleFlash1 = pCD->m_MuzzleFlashTick1 > pCD->m_GameTick - 4;
	const bool bLightning0 = pCD->m_LightningLightTick0 > pCD->m_GameTick - 28;
	const bool bLightning1 = pCD->m_LightningLightTick1 > pCD->m_GameTick - 28;
	
	CXR_AnimState Anim;
	CXR_Skeleton* pSkel = NULL;
	CXR_SkeletonInstance* pSkelInstance = NULL;
	
	const int iBaseIndex = _pObj->m_iObject * 5;
	int iLightning0 = (bLightning0 ? (iBaseIndex + 0x4006) : 0);
	int iLightning1 = (bLightning1 ? (iBaseIndex + 0x4007) : 0);
	if((!bFlashLight && !bMuzzleFlash0 && !bMuzzleFlash1 && !bLightning0 && !bLightning1) || (!GetEvaluatedPhysSkeleton(_pObj, _pWPhysState, pSkelInstance, pSkel, Anim, _IPTime, &_CharPos)))
	{
		// Either we have no lights, or something went wrong, so time to unlink lights
		int iIndex = iBaseIndex + 0x4002;
		const int nNum = iIndex + 6;
		for(; iIndex < nNum; iIndex++)
		{
			const int iLight = pSGI->SceneGraph_Light_GetIndex(iIndex);
			if(iLight)
				pSGI->SceneGraph_Light_Unlink(iLight);
		}

		iLightning0 = 0;
		iLightning1 = 0;
	}

	// Try to update weapon lights
	{
		CMat4Dfp32 Mat;
		if(!_pMat)
			_pMat = &Mat;

		//CXR_Model* pModel;
		CMTime GameTime = CMTime::CreateFromTicks(pCD->m_GameTick, _pWPhysState->GetGameTickTime(), pCD->m_PredictFrameFrac);
		
		// Update weapon lights
		bRet |= OnRefresh_WeaponLights_Model(pCD, 0, iBaseIndex + 0x4002, _pWPhysState, pSGI, pSkelInstance, pSkel, GameTime, Anim, _pMat, _IPTime, bMuzzleFlash0, bFlashLight, iLightning0);
		bRet |= OnRefresh_WeaponLights_Model(pCD, 1, iBaseIndex + 0x4004, _pWPhysState, pSGI, pSkelInstance, pSkel, GameTime, Anim, _pMat, _IPTime, bMuzzleFlash1, bFlashLight, iLightning1);

		/*
		if(pCD->m_Item0_Model.GetModel0_RenderInfo(_pWPhysState->GetMapData(), NULL, pSkelInstance, pSkel, GameTime, 
			Anim, *_pMat, pModel, _pWPhysState))
		{
			if(bMuzzleFlash)
			{
				CMat4Dfp32 FlashMat = *_pMat;
				CXR_Skeleton* pSkelItem = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
				if(pSkelItem)
				{
					const CXR_SkeletonAttachPoint *pAttach = pSkelItem->GetAttachPoint(1);
					if(pAttach)
						CVec3Dfp32::GetRow(FlashMat, 3) = pAttach->m_LocalPos.GetRow(3) * (FlashMat);
				}
				CVec3Dfp32::GetRow(FlashMat, 3) += CVec3Dfp32::GetRow(FlashMat, 2) * 5 + CVec3Dfp32::GetRow(FlashMat, 1) * 5;
				fp32 Duration = fp32(int(pCD->m_GameTick) - int(pCD->m_MuzzleFlashTick)) + _IPTime - 0.5f;
				Duration = MinMT(MaxMT(Duration, 0.0f), 3.0f);
	//			fp32 Amp = M_Sin(_PI * Duration / 3.0f) * 0.01f;
				fp32 Amp = ((3.0f - Duration) / 3.0f) * 0.01f;
				if(Amp > 0 && Duration > 0)
				{
					CXR_Light Light(FlashMat, CVec3Dfp32(((pCD->m_MuzzleLightColor.m_Value >> 16) & 0xff),
														((pCD->m_MuzzleLightColor.m_Value >>  8) & 0xff),
														((pCD->m_MuzzleLightColor.m_Value >>  0) & 0xff)) * Amp,
														pCD->m_MuzzleLightRange, 0, CXR_LIGHTTYPE_POINT);
					//CXR_Light Light(FlashMat, CVec3Dfp32(203, 131, 134) * Amp, 128, 0, CXR_LIGHTTYPE_POINT);
					Light.m_LightGUID = _pObj->m_iObject * 5 + 0x4002;
					Light.m_iLight = 0;
					Light.m_Flags = CXR_LIGHT_NOSHADOWS | CXR_LIGHT_NOSPECULAR;
					pSGI->SceneGraph_Light_LinkDynamic(Light);
					bMuzzleFlashSet = 1;
				}
			}
			if(bFlashLight)
			{
				CXR_Skeleton* pSkelItem = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
				if(pSkelItem)
				{
					const CXR_SkeletonAttachPoint *pAttach = pSkelItem->GetAttachPoint(3);
					if(pAttach)
						CVec3Dfp32::GetRow(*_pMat, 3) = pAttach->m_LocalPos.GetRow(3) * (*_pMat);
					else
						CVec3Dfp32::GetRow(*_pMat, 3) += CVec3Dfp32::GetRow(*_pMat, 0) * 25.0f;
				}
	//			_pWPhysState->Debug_RenderMatrix(*_pMat, 0.05f, false);

				CMat4Dfp32 LightPos = *_pMat;
				CVec3Dfp32::GetRow(LightPos, 3) += CVec3Dfp32::GetRow(*_pMat, 0) * -3.0f;
//				CVec3Dfp32::GetRow(LightPos, 3) += CVec3Dfp32::GetRow(*_pMat, 1) * 2.0f;
				CXR_Light Light(LightPos, CVec3Dfp32(((pCD->m_FlashLightColor.m_Value >> 16) & 0xff) / 255.0f,
								((pCD->m_FlashLightColor.m_Value >> 8) & 0xff) / 255.0f,
								((pCD->m_FlashLightColor.m_Value >> 0) & 0xff) / 255.0f) * 2,
								pCD->m_FlashLightRange, 0, CXR_LIGHTTYPE_SPOT);
				if(pCD->m_iPlayer != -1)
				{
					Light.m_SpotHeight = M_Tan(50 * (_PI / 180.0f ));
					Light.m_SpotWidth = M_Tan(50 * (_PI / 180.0f ));
				}
				else
				{
					Light.m_SpotHeight = M_Tan(30 * (_PI / 180.0f ));
					Light.m_SpotWidth = M_Tan(30 * (_PI / 180.0f ));
				}
				Light.m_LightGUID = _pObj->m_iObject * 5 + 0x4003;
				Light.m_iLight = 0;
				if (pCD->m_iPlayer == -1)
					Light.m_Flags |= CXR_LIGHT_NOSPECULAR;
				pSGI->SceneGraph_Light_LinkDynamic(Light);
				bFlashLightSet = 1;

				bRet = true;
			}
		}
		*/
	}

	/*
Done:
	if (!bMuzzleFlashSet)
	{
		int iLight = pSGI->SceneGraph_Light_GetIndex(_pObj->m_iObject * 5 + 0x4002);
		if (iLight)
			pSGI->SceneGraph_Light_Unlink(iLight);
	}
	if (!bFlashLightSet)
	{
		int iLight = pSGI->SceneGraph_Light_GetIndex(_pObj->m_iObject * 5 + 0x4003);
		if (iLight)
			pSGI->SceneGraph_Light_Unlink(iLight);
	}
	*/


	return bRet;
}

void CWObject_Character::Char_SetFootstep(CWObject_CoreData* _pObj, CWorld_Client* _pWClient, int _iFoot)
{
	return; // not finished yet

	CXR_AnimState Anim;
	CXR_Skeleton* pSkel;
	CXR_SkeletonInstance* pSkelInstance;					
	if(!GetEvaluatedPhysSkeleton(_pObj, _pWClient, pSkelInstance, pSkel, Anim))
		return;

	CRPG_Object_Item::CAttachInfo Info;
	Info.m_pSkelInstance = Anim.m_pSkeletonInst;
	Info.m_pCharSkeleton = pSkel;

//	CMat4Dfp32 *pRot = pSkelInstance->m_pBoneTransform;
	if(pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_RFOOT].k[3][2] > pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_LFOOT].k[3][2])
		_iFoot = 1;
//	ConOut(CStrF("%i  %f %f", _iFoot, pSkelInstance->m_lBoneTransform[PLAYER_ROTTRACK_RFOOT].k[3][2], pSkelInstance->m_lBoneTransform[PLAYER_ROTTRACK_LFOOT].k[3][2]));

	Info.m_iCharRotTrack = _iFoot ? PLAYER_ROTTRACK_RFOOT : PLAYER_ROTTRACK_LFOOT;
	Info.m_iCharAttachPoint = _iFoot ? 2 : 3;

	CMat4Dfp32 Mat = CRPG_Object_Item::GetAttachOnCharMatrix(Info);
	_pWClient->Debug_RenderMatrix(Mat, 10.0f, true);

	CMat4Dfp32 Pos = Mat;
	CVec3Dfp32(0, 0, 1).SetRow(Pos, 2);
	Pos.RecreateMatrix(2, 0);
//	Pos.M_x_RotZ(-0.25f);
	Pos.RotZ_x_M(0.25f);
	_pWClient->Debug_RenderSkeleton(pSkel, pSkelInstance, 0xff007f3f, 10.0f);
	//ConOut(CVec3Dfp32::GetRow(Pos, 3).GetFilteredString());
/*	CVec3Dfp32(0, 1, 0).SetRow(Mat, 0);
	CVec3Dfp32(-1, 0, 0).SetRow(Mat, 1);
	CVec3Dfp32(0, 0, 1).SetRow(Mat, 2);*/

	_pWClient->Wallmark_Create(Pos, 10, 4, _pWClient->GetRenderTime(), "footprint02", 0);
}

bool CWObject_Character::Char_IsPlayerViewControlled(CWObject_CoreData* _pObj)
{
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if(!pCD)
		return false;

	if(/*pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_THIRDPERSON ||*/
		Char_GetControlMode(_pObj) == PLAYER_CONTROLMODE_ACTIONCUTSCENE ||
		Char_GetControlMode(_pObj) == PLAYER_CONTROLMODE_LADDER ||
		Char_GetControlMode(_pObj) == PLAYER_CONTROLMODE_ANIMSYNC ||
		pCD->m_RelAnimPos.HasQueued(pCD->m_GameTick) ||
		_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_DIALOGUE ||
		_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_CUTSCENE ||
		_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_PLAYERSPEAK ||
		Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD ||
		pCD->m_Disable & PLAYER_DISABLE_TRIGGER)
		return false;

	return true;
}

bool CWObject_Character::Char_IsPlayerView(CWObject_CoreData* _pObj)
{
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if(!pCD)
		return false;

	if(pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_THIRDPERSON ||
		Char_GetControlMode(_pObj) == PLAYER_CONTROLMODE_ACTIONCUTSCENE ||
		Char_GetControlMode(_pObj) == PLAYER_CONTROLMODE_LADDER ||
		_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_DIALOGUE ||
		_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_CUTSCENE ||
		Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD ||
		(pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_MASK) != THIRDPERSONINTERACTIVE_MODE_NONE)
		return false;
	
	return true;
}

bool CWObject_Character::Char_CanLook(CWObject_CoreData* _pObj)
{
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	
	if(((pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_MASK) != THIRDPERSONINTERACTIVE_MODE_DIALOGUE) &&
		(pCD->m_AnimGraph2.GetStateFlagsLo() & (AG2_STATEFLAG_NOLOOK|AG2_STATEFLAG_BEHAVIORACTIVE) ||
	  _pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOLOOK) && 
	  !(pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_CREEPINGCAM) &&
	  !(pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_TURRET) &&
	  !(pCD->m_CameraUtil.IsActive(CAMERAUTIL_MODE_MOUNTEDCAM)))

		return false;
	return true;
}

void CWObject_Character::SetWeaponUnequipTimer(bool _bReset, fp32 _Seconds)
{
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if(!pCD)
		return;

	// Set gunplay unsafe
	SetGunplayUnsafe(pCD->m_GameTick);

	int nTicks = int(m_pWServer->GetGameTicksPerSecond() * _Seconds);
	if (_bReset || m_WeaponUnequipTimer != 0)
		m_WeaponUnequipTimer = pCD->m_GameTick + nTicks;
}

void CWObject_Character::ResetWeaponUnequipTimer()
{
	m_WeaponUnequipTimer = 0;
}


static void GetCharBonePos(CWorld_PhysState& _WPhysState, CWObject_Character& _Char, int _RotTrack, CVec3Dfp32& _Pos, CVec3Dfp32& _Dir)
{
	CVec3Dfp32 Pos(0,0,0), Dir(1,0,0);
	CXR_AnimState Anim;
	CXR_Skeleton* pSkel;
	CXR_SkeletonInstance* pSkelInstance;					
	if (CWObject_Character::GetEvaluatedPhysSkeleton(&_Char, &_WPhysState, pSkelInstance, pSkel, Anim) &&
		(_RotTrack < pSkel->m_lNodes.Len()))
	{
		const CMat4Dfp32& Mat = pSkelInstance->m_pBoneTransform[_RotTrack];
		Pos = pSkel->m_lNodes[_RotTrack].m_LocalCenter;
		Pos *= Mat;
		Dir = Mat.GetRow(0);

		// THIS IS A TEMP HACK - The new Jackie model have a camera track local pos at (0,0,56)-something,
		// but we still use old animations, where the camera track local pos was (0,0,0).
		// Therefore the correct code gives a position at (0,0,112)-something..
		if (_RotTrack == PLAYER_ROTTRACK_CAMERA && (Pos.k[2] - _Char.GetPosition().k[2]) > 70.0f)
			Pos = CVec3Dfp32(0,0,-5) * Mat;
	}
	else
	{
		Pos = _Char.GetPosition();
		Dir = _Char.GetPositionMatrix().GetRow(0);
	}

	_Pos = Pos;
	_Dir = Dir;
}

static M_INLINE fp32 Dot2(const CVec3Dfp32& a, const CVec3Dfp32& b)
{
	return (a.k[0] * b.k[0] + a.k[1] * b.k[1]) / (M_Sqrt(Sqr(a.k[0])+Sqr(a.k[1])) * M_Sqrt(Sqr(b.k[0])+Sqr(b.k[1])));
}



/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:		Updates the state of "Third Person Interactive".
					It will check when the camera should start moving
					towards 3rd person mode, and when the camera should
					move back again.

	Parameters:	
		_CD:		The client data

	Comments:		This function is mainly an experiment, and it's one 
					big mess. Once we've got it working, it needs to
					be rewritten!
\*____________________________________________________________________*/
void CWObject_Character::Char_UpdateThirdPersonInteractive(CWO_Character_ClientData& _CD)
{
	enum { e_Nothing, e_InDialogue, e_InACS, e_Focus_Dialogue, e_Focus_ACS };
	int8 eFocus = e_Nothing;
	int32 iFocusObj = -1;
	CWObject* pObj = NULL;
	CWObject_Character* pChar = NULL;

	uint8 Mode = (_CD.m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_MASK);
	uint8 ControlMode = CWObject_Character::Char_GetControlMode(this);
	bool b3PI = (Mode != THIRDPERSONINTERACTIVE_MODE_NONE);
	bool bIsMoving = (_CD.m_Control_Move.LengthSqr() > 0.1f);
	bool bIsRotating = (_CD.m_Control_DeltaLook.LengthSqr() > 1e-5f);
	bool bHaveChoices = false;
	bool bIsTelePhone = false;
	bool bSpeaking = (m_ClientFlags & PLAYER_CLIENTFLAGS_PLAYERSPEAK) != 0;
	
	if(bSpeaking)
		bIsMoving = false;

	static fp32 fCaptureTime = 0.0f;
	static fp32 fDeltaTime = 0.0f;

	iFocusObj = _CD.m_iFocusFrameObject;
	pObj = m_pWServer->Object_Get(iFocusObj);
	if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENE_ISTELEPHONE), iFocusObj) != -1 ||
		(int)_CD.m_ControlMode_Param2 == ACTIONCUTSCENE_TYPE_TELEPHONE)
	{
		bIsTelePhone = true;
	}

	if (b3PI)
	{

		if (ControlMode == PLAYER_CONTROLMODE_ACTIONCUTSCENE || ControlMode == PLAYER_CONTROLMODE_LADDER)
		{
			iFocusObj = _CD.m_3PI_FocusObject;//(int32)_CD.m_ControlMode_Param0;
			pObj = m_pWServer->Object_Get(iFocusObj);
			if (pObj)
				eFocus = e_InACS;

			bHaveChoices = true;
			bIsRotating = false;
			bIsMoving = false;
		}
		else if (Mode == THIRDPERSONINTERACTIVE_MODE_ACS)
		{
			iFocusObj = _CD.m_3PI_FocusObject;
			pObj = m_pWServer->Object_Get(iFocusObj);
			if (pObj)
			{ 
				if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENE_CANACTIVATE,2,0,m_iObject), iFocusObj) != 0)
					eFocus = e_Focus_ACS;
			}

			bHaveChoices = true;
		}
		else if (Mode == THIRDPERSONINTERACTIVE_MODE_DIALOGUE)
		{
			iFocusObj = _CD.m_3PI_FocusObject;
			pObj = pChar = CWObject_Character::IsCharacter(iFocusObj, m_pWServer);
			if (pChar)
				eFocus = e_Focus_Dialogue;
			else if(bIsTelePhone)
			{
				pObj = m_pWServer->Object_Get(iFocusObj);
				bIsTelePhone = true;
				eFocus = e_Focus_ACS;
			}

			bHaveChoices = (m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETCHOICES,0,0,m_iObject), iFocusObj) > 0)
				|| (ClientFlags() & PLAYER_CLIENTFLAGS_PLAYERSPEAK) != 0
				|| (_CD.m_3PI_Mode & THIRDPERSONINTERACTIVE_STATE_ENTERING);
		}
	}
	else
	{
		/*if (((_CD.m_FocusFrameType & SELECTION_MASK_TYPEINVALID) == SELECTION_ACTIONCUTSCENELOCKED) || ((_CD.m_FocusFrameType & SELECTION_MASK_TYPEINVALID) == SELECTION_ACTIONCUTSCENE))
		{
		// Check if the locked acs is a telephone (conversation on-going....)
		iFocusObj = _CD.m_iFocusFrameObject;
		pObj = m_pWServer->Object_Get(iFocusObj);
		int iTeleReg = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENE_ISTELEPHONE), iFocusObj);
		if (iTeleReg != -1)
		pObj = pChar = CWObject_Character::IsCharacter(iTeleReg, m_pWServer);
		if (pChar)
		{
		// Start dialogue 3pi
		bHaveChoices = (m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETCHOICES,0,0,m_iObject), pChar->m_iObject) > 0)
		|| (ClientFlags() & PLAYER_CLIENTFLAGS_PLAYERSPEAK) != 0;
		if (bHaveChoices)
		{
		iFocusObj = iTeleReg;
		eFocus = e_Focus_Dialogue;
		}
		else if ((pChar->Char_GetDialogueApproachItem().m_iHash != 0) && pChar->CanUse(m_iObject))
		{
		CWO_Character_ClientData* pCharCD = GetClientData(pChar);
		if ((pCharCD && pCharCD->m_DialogueInstance.m_iDialogueItem == 0) && (pChar->m_iListener == 0 || pChar->m_iListener == m_iObject))
		{
		iFocusObj = pChar->m_iObject;
		eFocus = e_Focus_Dialogue;
		}
		else
		{
		eFocus = e_Nothing;
		}
		}
		}
		}
		else */if (ControlMode == PLAYER_CONTROLMODE_ACTIONCUTSCENE || ControlMode == PLAYER_CONTROLMODE_LADDER)
		{
			iFocusObj = (int32)_CD.m_ControlMode_Param0;
			pObj = m_pWServer->Object_Get(iFocusObj);
			if ((pObj && m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENE_USE3PI), iFocusObj)) || bIsTelePhone)
				eFocus = e_InACS;

			bHaveChoices = true;
		}
		else if ((_CD.m_FocusFrameType & SELECTION_MASK_TYPEINVALID) == SELECTION_CHAR)
		{
			pObj = pChar = CWObject_Character::IsCharacter(_CD.m_iFocusFrameObject, m_pWServer);
			if (pChar) 
			{
				bHaveChoices = (m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETCHOICES,0,0,m_iObject), _CD.m_iFocusFrameObject) > 0)
					|| (ClientFlags() & PLAYER_CLIENTFLAGS_PLAYERSPEAK) != 0;
				if (bHaveChoices)
				{
					iFocusObj = _CD.m_iFocusFrameObject;
					eFocus = e_Focus_Dialogue;
				}
				else if (pChar->Char_GetDialogueApproachItem().IsValid() && pChar->CanUse(m_iObject))
				{
					CWO_Character_ClientData* pCharCD = GetClientData(pChar);
					M_ASSERTHANDLER(pCharCD, "All characters must have clientdata!!", return);

					fp32 TimeLeft = 0.0f;
					if (pCharCD->m_DialogueInstance.IsValid())
						TimeLeft = (pCharCD->m_DialogueInstance.m_StartTime - pCharCD->m_GameTime).GetTime() + pCharCD->m_DialogueInstance.m_SampleLength;

					if (TimeLeft <= 0.0f && (pChar->m_iListener == 0 || pChar->m_iListener == m_iObject))
					{
						iFocusObj = _CD.m_iFocusFrameObject;
						eFocus = e_Focus_Dialogue;
					}
					else
					{
						eFocus = e_Nothing;
					}
				}
			}
		}
		else if (((_CD.m_FocusFrameType & SELECTION_MASK_TYPEINVALID) == SELECTION_ACTIONCUTSCENE) || ((_CD.m_FocusFrameType & SELECTION_MASK_TYPEINVALID) == SELECTION_LADDER))
		{
			iFocusObj = _CD.m_iFocusFrameObject;
			pObj = m_pWServer->Object_Get(iFocusObj);
			if (pObj)
			{
				bool bCanActivate = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENE_CANACTIVATE,2,0,m_iObject), iFocusObj) != 0;
				bool bUse3PI = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENE_USE3PI), iFocusObj) != 0;
				if (bCanActivate && bUse3PI)
					eFocus = e_Focus_ACS;
			}
		}
	}

	if (!pObj)
		eFocus = e_Nothing;

	fp32 FocusAmount = 0.0f;	// 1: perfect, 0.5: threshold, 0: nothing
	bool bEnoughFocus = false;

	if (eFocus != e_Nothing)
	{
		// Get camera position & direction
		CVec3Dfp32 CameraPos, CameraDir;
		GetCharBonePos(*m_pWServer, *this, PLAYER_ROTTRACK_CAMERA, CameraPos, CameraDir);
		CameraPos -= GetPositionMatrix().GetRow(1) * 3;	// prototype hack!

		CameraPos[2] -= 5.0f;													// prototype hack!
		CameraDir = GetPositionMatrix().GetRow(0); // TODO: remove?

		// Get hot-spot position & direction
		CVec3Dfp32 ObjPos(0, 0, 0), ObjDir(0, 1, 0);
		if (eFocus == e_InDialogue || eFocus == e_Focus_Dialogue)
		{
			GetCharBonePos(*m_pWServer, *pChar, PLAYER_ROTTRACK_HEAD, ObjPos, ObjDir);
		}
		else if (eFocus == e_InACS || eFocus == e_Focus_ACS)
		{
			ObjPos = pObj->GetPosition();
			ObjDir = pObj->GetPositionMatrix().GetRow(0);

			CWObject_Message Msg(OBJMSG_ACTIONCUTSCENE_GETFOCUSPOS);
			Msg.m_VecParam0 = CameraPos;
			Msg.m_pData = &ObjPos;
			Msg.m_DataSize = sizeof(CVec3Dfp32);
			int d = m_pWServer->Message_SendToObject(Msg,pObj->m_iObject);
			if (d == -1)
				ObjDir *= -1.0f;
		}

		CVec3Dfp32 CamToObj = ObjPos - CameraPos;
		fp32 DistanceToObject = CamToObj.Length(); 
		fp32 ClosestDistanceToLine = 10000.0f;
		fp32 Proj = CamToObj * CameraDir;
		if (Proj > 6.0f)
			ClosestDistanceToLine = (CameraPos + CameraDir * Proj - ObjPos).Length();
		CamToObj *= (1.0f / DistanceToObject); //Normalize()

		CVec3Dfp32 CamToObjZ = CamToObj;
		CamToObjZ.k[2] = pObj->GetPosition().k[2] - GetPosition().k[2];
		DistanceToObject = Min(DistanceToObject, CamToObjZ.Length());

		//		fp32 DirCheck1 = Dot2(CameraDir, CamToObj);
		fp32 DirCheck2 = -Dot2(CameraDir, ObjDir);

		// -- spam!! -- M_TRACE("DistanceToObject, %f\n", DistanceToObject);
		//m_pWServer->Debug_RenderWire(CameraPos, ObjPos, 0xff7f7f7f, 0.05f);
		//m_pWServer->Debug_RenderWire(CameraPos, CameraPos + CameraDir * 32, 0xff7f7f7f, 0.05f);

		if (b3PI)
		{
			if (eFocus == e_InDialogue || eFocus == e_Focus_Dialogue)
			{
				// Disable proxy for now
				/*if ((ClientFlags() & PLAYER_CLIENTFLAGS_PLAYERSPEAK) || (_CD.m_FocusFrameType & SELECTION_FLAG_PROXY))
				//if ((ClientFlags() & PLAYER_CLIENTFLAGS_PLAYERSPEAK) && (!(_CD.m_FocusFrameType & SELECTION_FLAG_PROXY)))
				{
				FocusAmount = 1.0f;
				bEnoughFocus = true;
				}
				else //if(!(_CD.m_FocusFrameType & SELECTION_FLAG_PROXY))*/
				{
					FocusAmount = Max(0.0f, 1.0f - ClosestDistanceToLine * (0.5f/18.0f));
					//bEnoughFocus = (DistanceToObject < 58.0f) && (FocusAmount > 0.0f) && (DirCheck2 > 0.2f);// && !bIsMoving;
					bEnoughFocus = (DistanceToObject < 58.0f) && (FocusAmount > 0.0f) && (DirCheck2 > 0.2f) && !bIsMoving;
					//bEnoughFocus = (DistanceToObject < 58.0f) && (FocusAmount > 0.0f) && !bIsMoving;

					//if((FocusAmount > 0.4f) && (DistanceToObject < 68.0f))
					//if((FocusAmount > 0.4f) && (DistanceToObject < 87.0f) && bHaveChoices && !bIsMoving)
					if((FocusAmount > 0.4f) && bHaveChoices && !bIsMoving)
					{
						if(fCaptureTime < 0.001f)
							fCaptureTime = m_pWServer->GetGameTime().GetTime();

						fDeltaTime = (m_pWServer->GetGameTime().GetTime() - fCaptureTime);

						if((fDeltaTime) > 0.6f)
						{
							FocusAmount = 1.0f;
							bEnoughFocus = true;
						}
					}
				}
			}
			else if (eFocus == e_Focus_ACS)
			{
				FocusAmount = Max(0.0f, 1.0f - ClosestDistanceToLine * (0.5f/18.0f));
				bEnoughFocus = (DistanceToObject < 48.0f) && (FocusAmount > 0.0f) && (DirCheck2 > 0.2f) && !bIsMoving && !bIsTelePhone;
			}
			else if (eFocus == e_InACS)
			{
				FocusAmount = 1.0f;
				bEnoughFocus = true;
			}
		}
		else
		{
			if (eFocus == e_InDialogue || eFocus == e_Focus_Dialogue)
			{

				/*if(_CD.m_FocusFrameType & SELECTION_FLAG_PROXY)
				{
				// -- Currently disabled, If proxy object, just keep first-person camera for now --
				FocusAmount = 1.0f;
				bEnoughFocus = true;
				}
				else //if(!(_CD.m_FocusFrameType & SELECTION_FLAG_PROXY))*/
				{
					//					uint16 iType = _CD.m_FocusFrameType;

					FocusAmount = Max(0.0f, 1.0f - ClosestDistanceToLine * (0.5f/14.0f));
					//bEnoughFocus = (DistanceToObject < 38.0f) && (FocusAmount > 0.5f) && (DirCheck2 >= -0.4f) && !bIsMoving;
					bEnoughFocus = (DistanceToObject < 58.0f) && (FocusAmount > 0.5f) && (DirCheck2 >= 0.2f) && !bIsMoving;
					//bEnoughFocus = (DistanceToObject < 58.0f) && (FocusAmount > 0.5f) && !bIsMoving;

					if(bHaveChoices && !bIsMoving)
					{
						if(fCaptureTime < 0.001f)
							fCaptureTime = m_pWServer->GetGameTime().GetTime();

						fDeltaTime = (m_pWServer->GetGameTime().GetTime() - fCaptureTime);

						if((fDeltaTime) > 0.6f)
						{
							FocusAmount = 1.0f;
							bEnoughFocus = true;
						}
					}
				}
			}
			else if (eFocus == e_Focus_ACS)
			{
				FocusAmount = Max(0.0f, 1.0f - ClosestDistanceToLine * (0.5f/14.0f));
				bEnoughFocus = (DistanceToObject < 48.0f) && (FocusAmount > 0.5f) /*&& (DirCheck2 > 0.2f)*/ && !bIsMoving && !bIsTelePhone;
			}
			else if (eFocus == e_InACS)
			{
				FocusAmount = 1.0f;
				bEnoughFocus = true;
				_CD.m_3PI_FocusTicks = 100; //test
			}
		}
	}

	if (!b3PI)
	{
		if (bEnoughFocus)
		{
			_CD.m_3PI_FocusObject = iFocusObj;

			if (eFocus == e_InDialogue || eFocus == e_Focus_Dialogue)
			{
				//				_CD.m_3PI_LightFadeStart = m_pWServer->GetGameTick();
				//				_CD.m_3PI_LightFadeDuration = RoundToInt(2.0f * m_pWServer->GetGameTicksPerSecond());
				//				_CD.m_3PI_LightState = THIRDPERSONINTERACTIVE_LIGHT_STATE_FADE_IN;

				// Auto-Use?
				bool bWait = (pChar->m_Flags & PLAYER_FLAGS_DIALOGUE_WAIT) != 0;
				
				//if(!_CD.m_Go3rdpOnTick)
				//	_CD.m_Go3rdpOnTick = m_pWServer->GetGameTick() + 60;

				if (!bHaveChoices && !bSpeaking && !bWait && (m_iLastAutoUseObject != iFocusObj))
				{
					//_CD.m_3PI_Mode = THIRDPERSONINTERACTIVE_MODE_DIALOGUE;
					pChar->Char_SetListener(m_iObject,PLAYER_DIALOGUE_AIFLAGS_KEEPBEHAVIOURSPEAKER);
					m_iLastAutoUseObject = iFocusObj;
					pChar->OnUse(m_iObject, 0);
					_CD.m_Go3rdpOnTick = -1;
				}
				else if(!bWait)
				{
					if((bHaveChoices && _CD.m_Go3rdpOnTick /*&& _CD.m_Go3rdpOnTick < m_pWServer->GetGameTick()*/ || bSpeaking) && _CD.m_iMountedObject == 0)
					{
						_CD.m_3PI_CameraSlide.Set(0, _CD.m_GameTick-10, 0.0f);
						_CD.m_3PI_Mode = THIRDPERSONINTERACTIVE_MODE_DIALOGUE;
						
						//pChar = CWObject_Character::IsCharacter(m_iObject, m_pWServer);
						Char_MakePlayerLookAtSpeaker(this, pChar);
					}
					Char_TurnDialogueLightOn(_CD, true);
					if (pChar)
						pChar->Char_SetListener(m_iObject,PLAYER_DIALOGUE_AIFLAGS_KEEPBEHAVIOURSPEAKER);
				}
			}
			else if (eFocus == e_InACS || eFocus == e_Focus_ACS)
			{
				_CD.m_3PI_Mode = THIRDPERSONINTERACTIVE_MODE_ACS;
				_CD.m_ControlMode_Param0 = iFocusObj;

				CActionCutsceneCamera* pCamera = NULL;
				CCameraUtil* pCamUtil = &_CD.m_CameraUtil;
				CWObject_Message msg(OBJMSG_ACTIONCUTSCENE_GETFOCUSCUTSCENECAMERA, 0, 0, m_iObject, 0, 0, 0, pCamUtil);
				if (m_pWServer->Phys_Message_SendToObject(msg, iFocusObj) && pCamera != NULL)
				{
					pCamera->InteractiveMode_Start(_CD);
					_CD.m_CameraUtil.MakeDirty();
				}
			}
		}
		else
		{
			if (m_iLastAutoUseObject != iFocusObj)
				Char_TurnDialogueLightOn(_CD, bHaveChoices);
			
			_CD.m_Go3rdpOnTick = 0;
		}
	}
	else
	{	
		if(bEnoughFocus && bHaveChoices)// && (_CD.m_3PI_Mode != THIRDPERSONINTERACTIVE_STATE_LEAVING))
		{
			_CD.m_Go3rdpOnTick = 0;

			Char_TurnDialogueLightOn(_CD,true);
			
			// Turn on light
			if(!(ClientFlags() & CWO_CLIENTFLAGS_VISIBILITY))
			{
				ClientFlags() |= CWO_CLIENTFLAGS_VISIBILITY;
				//UpdateVisibilityFlag(true);
			}

			// Slide camera
			_CD.m_3PI_Mode = Mode | THIRDPERSONINTERACTIVE_STATE_ENTERING;
			//_CD.m_3PI_Mode.MakeDirty();

			//if(_CD.m_3PI_CameraSlide.Set(255, _CD.m_GameTick+2, 0.25f))
			if (_CD.m_3PI_CameraSlide.Set(255, _CD.m_GameTick-10, 0.0f))	// TEMP -- instant switch to 3PI-camera
			{
				// This is the first frame of the third person camera, capture reference matrix..
				_CD.m_bFirstFrameIn3PI = 1;
				_CD.m_bFirstFrameIn3PI.MakeDirty();
				_CD.m_3PI_CameraSlide.MakeDirty();

				pChar = CWObject_Character::IsCharacter(m_iObject, m_pWServer);

				// Make player look at speaker
				if(pChar && pChar->IsPlayer())
					Char_MakePlayerLookAtSpeaker(pChar, CWObject_Character::IsCharacter(iFocusObj, m_pWServer));
			}

		}
		else
		{
			if(ClientFlags() & CWO_CLIENTFLAGS_VISIBILITY)
			{
				// Turn off light
				ClientFlags() &= ~CWO_CLIENTFLAGS_VISIBILITY;
				UpdateVisibilityFlag();
			}

			//if (_CD.m_3PI_CameraSlide.Set(0, _CD.m_GameTick+2, 0.25f))
			if (_CD.m_3PI_CameraSlide.Set(0, _CD.m_GameTick-10, 0.0f))	// TEMP -- instant switch to normal camera
			{
				_CD.m_3PI_CameraSlide.MakeDirty();
				fCaptureTime = 0.0f;
				fDeltaTime = 0.0f;
			}

			//if (_CD.m_3PI_CameraSlide.Get(_CD.m_GameTick, 0.0f) == 0)
			{
				//if (!bEnoughFocus)
				{

					if(!bHaveChoices && !bEnoughFocus)
						Char_TurnDialogueLightOn(_CD, false);
					
					_CD.m_3PI_FocusObject = -1;
					_CD.m_3PI_Mode = THIRDPERSONINTERACTIVE_MODE_NONE;
					_CD.m_3PI_FocusTicks = 0;

					if (Mode == THIRDPERSONINTERACTIVE_MODE_ACS)
					{
						CActionCutsceneCamera* pCamera = NULL;
						CWObject_Message msg(OBJMSG_ACTIONCUTSCENE_GETACTIVECUTSCENECAMERA, 0, 0, m_iObject, 0, 0, 0, &pCamera);
						if (m_pWServer->Phys_Message_SendToObject(msg, iFocusObj) && pCamera != NULL)
						{
							pCamera->InteractiveMode_Stop(_CD);
						}
						else
						{
							CActionCutsceneCamera ActionCam;
							ActionCam.GetCamVars().ImportFromPCD(&_CD, m_iObject);
							ActionCam.InteractiveMode_Stop(_CD);
						}
						// Send "leavefocus" messages
						m_pWServer->Phys_Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENE_LEAVEFOCUSCUTSCENECAMERA),iFocusObj);
					}
				}
			}
		}
	}
}

void CWObject_Character::Char_MakePlayerLookAtSpeaker(CWObject_Character* _pPlayer, CWObject_Character* _pSpeaker)
{
	if(_pPlayer && _pSpeaker)
	{
		CMat4Dfp32 PlayerPosition = _pPlayer->GetPositionMatrix();
		CVec3Dfp32 PlayerPos,SpeakerPos,Dir;
		GetCharBonePos(*m_pWServer, *_pPlayer, PLAYER_ROTTRACK_HEAD, PlayerPos, Dir);
		GetCharBonePos(*m_pWServer, *_pSpeaker, PLAYER_ROTTRACK_HEAD, SpeakerPos, Dir);
		CVec3Dfp32 Direction = (SpeakerPos - PlayerPos).Normalize();
		Direction.SetMatrixRow(PlayerPosition, 0);
		PlayerPosition.RecreateMatrix(2, 0);
		_pPlayer->SetPosition(PlayerPosition);
	}
}
#define FADELIGHT_IN_TIME 2.5f
#define FADELIGHT_OUT_TIME 2.5f
void CWObject_Character::Char_TurnDialogueLightOn(CWO_Character_ClientData& _CD, bool _bOn)
{
	if(_CD.m_iPlayer == -1)
		return;

	int TicksIntoFade;
	int TotFadeTicks;
	fp32 FadeVal;

	int ModTicks = 0;
	if(_bOn)
	{
		switch(_CD.m_3PI_LightState)
		{
		case THIRDPERSONINTERACTIVE_LIGHT_STATE_FADE_IN:
			{
				FadeVal = ((fp32)(m_pWServer->GetGameTick() - _CD.m_3PI_LightFadeStart) / (m_pWServer->GetGameTicksPerSecond() * FADELIGHT_IN_TIME));
				if(FadeVal > 1.0f)
				{
					_CD.m_3PI_LightState = THIRDPERSONINTERACTIVE_LIGHT_STATE_ON;
					_CD.m_3PI_LightState.MakeDirty();
				}
			}
			break;

		case THIRDPERSONINTERACTIVE_LIGHT_STATE_ON:
			break;

		case THIRDPERSONINTERACTIVE_LIGHT_STATE_FADE_OUT:
			{
				_CD.m_3PI_LightState = THIRDPERSONINTERACTIVE_LIGHT_STATE_FADE_IN;
				_CD.m_3PI_LightState.MakeDirty();
				
				// smooth transition between fadeout and fadein
				TicksIntoFade = (m_pWServer->GetGameTick() - _CD.m_3PI_LightFadeStart);
				TotFadeTicks = TruncToInt(m_pWServer->GetGameTicksPerSecond() * FADELIGHT_IN_TIME); 

				ModTicks = TotFadeTicks - TicksIntoFade;

				_CD.m_3PI_LightFadeStart = m_pWServer->GetGameTick() - ModTicks;
				_CD.m_3PI_LightFadeStart.MakeDirty();
			}
			break;

		case THIRDPERSONINTERACTIVE_LIGHT_STATE_OFF:
			{
				_CD.m_3PI_LightState = THIRDPERSONINTERACTIVE_LIGHT_STATE_FADE_IN;
				_CD.m_3PI_LightState.MakeDirty();

				_CD.m_3PI_LightFadeStart = m_pWServer->GetGameTick();
				_CD.m_3PI_LightFadeStart.MakeDirty();
			}
		    break;
		
		default:
		    break;		
		}
	}
	else
	{
		switch(_CD.m_3PI_LightState)
		{
		case THIRDPERSONINTERACTIVE_LIGHT_STATE_FADE_IN:
			{
				_CD.m_3PI_LightState = THIRDPERSONINTERACTIVE_LIGHT_STATE_FADE_OUT;
				_CD.m_3PI_LightState.MakeDirty();

				// smooth transition between fadein and fadeout
				
				TicksIntoFade = (m_pWServer->GetGameTick() - _CD.m_3PI_LightFadeStart);
				TotFadeTicks = TruncToInt(m_pWServer->GetGameTicksPerSecond() * FADELIGHT_OUT_TIME); 

				ModTicks = TotFadeTicks - TicksIntoFade;

				_CD.m_3PI_LightFadeStart = m_pWServer->GetGameTick() - ModTicks;
				_CD.m_3PI_LightFadeStart.MakeDirty();
			}
			break;

		case THIRDPERSONINTERACTIVE_LIGHT_STATE_ON:
			{
				_CD.m_3PI_LightState = THIRDPERSONINTERACTIVE_LIGHT_STATE_FADE_OUT;
				_CD.m_3PI_LightState.MakeDirty();

				_CD.m_3PI_LightFadeStart = m_pWServer->GetGameTick();
				_CD.m_3PI_LightFadeStart.MakeDirty();
			}
			break;

		case THIRDPERSONINTERACTIVE_LIGHT_STATE_FADE_OUT:
			{
				FadeVal = ((fp32)(m_pWServer->GetGameTick() - _CD.m_3PI_LightFadeStart) / (m_pWServer->GetGameTicksPerSecond() * FADELIGHT_OUT_TIME));
				if(FadeVal > 1.0f)
				{
					_CD.m_3PI_LightState = THIRDPERSONINTERACTIVE_LIGHT_STATE_OFF;
					_CD.m_3PI_LightState.MakeDirty();
				}
			}
			break;

		case THIRDPERSONINTERACTIVE_LIGHT_STATE_OFF:
			break;

		default:
			break;
		}		
	}
};

void CWObject_Character::Char_CreateNeckBlood()
{
	// Make sure we don't get to bloody if not wanted
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	int bBlood = (pSys && pSys->GetOptions()) ? pSys->GetOptions()->GetValuei("GAME_BLOOD", 1) : 1;
	if (!bBlood)
		return;

	CMat4Dfp32 NeckMat;
	CXR_AnimState Anim;
	CXR_Skeleton* pSkel;
	CXR_SkeletonInstance* pSkelInstance;
	CWorld_PhysState* pWPhysState = safe_cast<CWorld_PhysState>(m_pWServer);
	if (CWObject_Character::GetEvaluatedPhysSkeleton(this, pWPhysState, pSkelInstance, pSkel, Anim))
	{
		NeckMat = pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_NECK];
		NeckMat.GetRow(3) = (pSkel->m_lNodes[PLAYER_ROTTRACK_NECK].m_LocalCenter * NeckMat);

		/* Fix this,
		CWO_Character_ClientData* pCD = GetClientData(this);
		int16 iHeadModel = m_pWServer->GetMapData()->GetResourceIndex_Model("Characters\\sever\\sever_head");
		CXR_Model* pHeadModel = m_pWServer->GetMapData()->GetResource_Model(iHeadModel);
		if (pHeadModel && pCD)
		{
			CMTime GameTime = CMTime::CreateFromTicks(pCD->m_GameTick, m_pWServer->GetGameTickTime(), pCD->m_PredictFrameFrac);
			{
				{
					m_spRagdollDropItems[2] = MNew(CConstraintRigidObject);
					m_spRagdollDropItems[2]->SetOrgMat(NeckMat);

					SConstraintSystemSettings Settings;
					Settings.m_SkeletonType	= SConstraintSystemSettings::RAGDOLL_NONE;
					Settings.m_pSkelInstance = pSkelInstance;
					m_spRagdollDropItems[2]->SetCollisionSound(0,false);
					m_spRagdollDropItems[2]->StartCollecting(m_pWServer, pHeadModel, &Settings);
					m_spRagdollDropItems[2]->Activate(true);
					m_spRagdollDropItems[2]->CollectFrame(false,NeckMat);
					m_spRagdollDropItems[2]->Animate(m_pWServer->GetGameTick(), NeckMat);

					// Add pending impulse
					m_spRagdollDropItems[2]->AddPendingImpulse(GetPosition(), CVec3Dfp32(0,0,2.0f) - CVec3Dfp32::GetMatrixRow(GetPositionMatrix(),1));
				}
			}

			spCRPG_Object spItem = Char()->CreateObject("weapon_head", m_pWServer);
			CRPG_Object* pItem = (CRPG_Object *)spItem;
			if (pItem && m_spRagdollDropItems[2])
			{
				int iHeadObj = m_pWServer->Object_Create("Item", NeckMat);
				if(iHeadObj > 0)
				{
					m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENEPICKUP_SETDEFAULTS), iHeadObj);
					int nMessages;
					const CWO_SimpleMessage* pMsg = m_MsgContainer.GetMessages(PLAYER_SIMPLEMESSAGE_ONPICKUPDROPWEAPON, nMessages);
					CWObject_Message Msg(OBJMSG_RPG_REPLACEPICKUP, nMessages, aint(pMsg));
					Msg.m_pData = (CRPG_Object *)pItem;
					m_pWServer->Message_SendToObject(Msg, iHeadObj);
				}
			}
		}
		*/

		// Create detailed head blown off blood stuffs
		{
			CWO_OnIntersectLineContext Ctx;
			Ctx.m_pObj = this;
			Ctx.m_pPhysState = pWPhysState;
			Ctx.m_p0 = NeckMat.GetRow(3) + (NeckMat.GetRow(2) * 8.0f);
			Ctx.m_p1 = NeckMat.GetRow(3) - (NeckMat.GetRow(2) * 8.0f);
			Ctx.m_ObjectFlags = (OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER);
			Ctx.m_ObjectIntersectionFlags = 0;
			Ctx.m_MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;

			CCollisionInfo CInfo;
			CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
			CInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_SURFACE | CXR_COLLISIONRETURNVALUE_LOCALPOSITION);
			bool bHit = CWObject_Character::OnIntersectLine(Ctx, &CInfo);
			if (bHit && CInfo.m_bIsValid)
			{
				// Create decal around neck
				int8 iDecal = TruncToInt(Random * 2.999f);
				CVec3Dfp32 Normal(CInfo.m_Plane.n);
				Normal.MultiplyMatrix3x3(CInfo.m_LocalNodePos);

				// TODO: Rotate normal to unanimated model space.
				Normal *= 127.0f;

				CNetMsg DecalMsg(PLAYER_NETMSG_DECAL);
				DecalMsg.AddVecInt16_Max32768(CInfo.m_LocalPos);
				DecalMsg.AddVecInt8_Max128(Normal);
				DecalMsg.AddInt8(CInfo.m_LocalNode);
				DecalMsg.AddInt8(iDecal);
				DecalMsg.AddInt8(6);
				DecalMsg.AddInt16(90);
				m_pWServer->NetMsg_SendToObject(DecalMsg, m_iObject);

				// Create blood effect
				CRPG_InitParams InitParams;
				InitParams.m_pCInfo = &CInfo;
				const int nParam = 1;
				aint pParam[nParam] = { (aint)&InitParams };
				m_pWServer->Object_Create("hitEffect_CriticalHeadShot", NeckMat, m_iObject, pParam, nParam);
			}
		}
	}
}
