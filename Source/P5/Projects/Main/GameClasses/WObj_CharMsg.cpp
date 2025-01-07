#include "PCH.h"

#include "WObj_Char.h"
#include "../GameWorld/WClientMod.h"
#include "../GameWorld/WServerMod.h"
#include "../../../Shared/MOS/Classes/GameWorld/Client/WClient_Core.h"
#include "../../../Shared/MOS/Classes/GameWorld/WDataRes_Sound.h"
#include "../../../Shared/MOS/xr/xrcustommodel.h"
#include "WObj_AI/AICore.h"
#include "WObj_Weapons/WObj_Spells.h"
#include "WRPG/WRPGChar.h"
#include "WRPG/WRPGAmmo.h"
#include "WRPG/WRPGFist.h"
#include "WObj_Sys/WObj_Trigger.h"
#include "WObj_AI/WObj_Aux/WObj_Team.h"
#include "WObj_Game/WObj_GameMod.h"
#include "CConstraintSystem.h"
#include "WObj_Misc/WObj_ActionCutscene.h"
#include "WObj_Misc/WObj_CreepingDark.h"
#include "WObj_Misc/WObj_InputEntity.h"
#include "WObj_Misc/WObj_AnimEventListener.h"
#include "WObj_Misc/WObj_ChainEffect.h"
#include "WObj_Misc/WObj_Shell.h"
#include "WObj_Misc/WObj_Object.h"
#include "WObj_Misc/WObj_TentacleSystem.h"

#include "Models/WModel_EffectSystem.h"

#include "../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_PhysCluster.h"

//Part of Sammes message testing system
/*
extern TArray<SMessageCounter> g_slMessages;

void AddMessage(const CWObject_Message& _Msg, bool _bServer)
{
	for(int i = 0; i < g_slMessages.Len(); ++i)
	{
		if(g_slMessages[i].m_iMsg == _Msg.m_Msg)
		{
			if(_bServer)
				g_slMessages[i].m_ServerCount++;
			else
				g_slMessages[i].m_ClientCount++;
			return;
		}
	}

	SMessageCounter Msg;
	Msg.m_iMsg = _Msg.m_Msg;
	Msg.m_ServerCount= _bServer;
	Msg.m_ClientCount= !_bServer;
	g_slMessages.Add(Msg);
}
*/


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Character message parsers
					
	Contents:		OnMessage
					OnClientMessage
					OnClientNetMsg
\*____________________________________________________________________________________________*/

aint CWObject_Character::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_Character_OnMessage, 0);

//	AddMessage(_Msg, true); //Part of Sammes message testing system

	//Let AI catch any messages first
	if (m_spAI->OnMessage(_Msg))
		//Message handled by AI!
		return true;
	
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	/* The messages are now orderd so that the most freqeunt are placed first in the case-switch. When putting a new message into
		the system. A bit down a end comment is put. After that the messages can be in any order. */
	switch(_Msg.m_Msg)
	{
	case OBJSYSMSG_GETCAMERA:
		{
			if(!pCD)
				return 0;

			if (_Msg.m_DataSize != sizeof(CMat4Dfp32)) return 0;
			CMat4Dfp32& Camera = *(CMat4Dfp32*)_Msg.m_pData;

			int iCameraObj = 0;
			if(m_ClientFlags & PLAYER_CLIENTFLAGS_DIALOGUE)
			{
				/*
				// Dialogue mode
				// Get camera matrix and return
				iCameraObj = 0;
				int iTarget = pCD->m_iDialogueTargetObject;
				GetDialogueCamera(Camera, 0.0f, NULL, 0, 0, this, m_pWServer->Object_Get(iTarget));
				return 1;
				*/
			}
			if(pCD->m_Cutscene_Camera != 0)
			{
				iCameraObj = (uint16)pCD->m_Cutscene_Camera;

				// Force update this variable. This can possible fix some bugs related to 
				// the camera not updating correctly when having cutscenes in the beginning of the level.
				pCD->m_Cutscene_Camera.MakeDirty();
			}
			else if (pCD->m_CameraUtil.IsActive(CAMERAUTIL_MODE_MOUNTEDCAM))
			{
				if (pCD->m_CameraUtil.GetCamera(m_pWServer, Camera, this, pCD, 1.0f))
					return 1;
			}
			else if (pCD->m_iMountedObject && (m_MountFlags & PLAYER_MOUNTEDMODE_FLAG_ORIGINAL))
			{
				iCameraObj = (uint16)pCD->m_iMountedObject;
			}

			/*			if(pCD->m_iMountedObject)
			iCameraObj = (uint16)pCD->m_iMountedObject;*/

			if(iCameraObj > 0)
			{
				if(m_pWServer->Message_SendToObject(_Msg, iCameraObj))
				{
					Camera.RotX_x_M(-0.25f);
					Camera.RotY_x_M(0.25f);
					return 1;
				}
			}

			if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_CREEPINGCAM)
			{
				CWObject_CoreData* pCreep = m_pWServer->Object_GetCD(pCD->m_iCreepingDark);
				if (pCreep)
				{
					Camera = pCreep->GetPositionMatrix();
					return 1;
				}
			}


			Camera = GetPositionMatrix();
			CVec3Dfp32::GetMatrixRow(Camera, 3) += Camera_GetHeadPos(this);

			//Update last camera pos
			m_LastCamera = Camera;

			return 1;
		}
	case OBJMSG_CHAR_CANACTIVATEITEM:
		{
			CRPG_Object_Char* pChar = Char();
			CRPG_Object_Item* pEquippedItem = pChar->GetEquippedItem(_Msg.m_Param0);
			if (pEquippedItem)
				return pEquippedItem->IsActivatable(pChar,m_iObject);

			return false;
		}
	case OBJMSG_CHAR_SETMOUNTEDLOOK:
		{
			return OnCharSetMountedLook(_Msg, pCD);
		}
	case OBJMSG_CHAR_SETANIMPHYS:
		{
			if (_Msg.m_Param0)
			{
				pCD->m_Phys_Flags = pCD->m_Phys_Flags | PLAYER_PHYSFLAGS_NOANIMPHYS;
			}
			else
			{
				pCD->m_Phys_Flags = pCD->m_Phys_Flags & ~PLAYER_PHYSFLAGS_NOANIMPHYS;
			}
			return true;
		}
	case OBJMSG_CHAR_LISTENTOME:
		{
			// Target in
			if (!_Msg.m_pData)
				return 0;
			
			// Target
			int iTarget = m_pWServer->Selection_GetSingleTarget((char*)_Msg.m_pData);
			return m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_LISTENTOME,_Msg.m_Param0,m_iObject,m_iObject),iTarget);
		}
	case OBJMSG_CHAR_SETMOUNTEDCAMERA:
		{
			return OnCharSetMountedCamera(_Msg, pCD);
		}
	case OBJMSG_CHAR_UPDATEITEMDESC:
		{
			return OnCharUpdateItemDesc(_Msg, pCD);
		}
	case OBJMSG_CHAR_SETMISSIONCOMPLETED:
		{
			CStr ItemType = (char*)_Msg.m_pData;
			CRPG_Object_Item* pItem = Char()->FindItemByName(ItemType);
			if (pItem)
			{
				if (Char()->RemoveItemByName(m_iObject,pItem->m_iItemType >> 8,ItemType,true))
				{
					m_CompletedMissions.AddMission(ItemType);
				}
				Char_UpdateQuestItems();
			}
			return true;
		}

	case OBJMSG_CHAR_SETFLAGS_SCRIPT: // "SetPlayerFlags"
		{
			uint32 Flags = ResolvePlayerFlags((const char*)_Msg.m_pData);
			if (_Msg.m_Param0 == 0)
			{
				m_Flags |= Flags;	// Add flags
			}
			else if (_Msg.m_Param0 == 1)
			{
				m_Flags &= ~Flags;	// Remove flags
			}
		}
		break;

	case OBJMSG_CHAR_CANPICKUPITEM:
		{
			return true;
			/*if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), m_pWServer->Game_GetObjectIndex()))
				return true;

			// First check if we have a weapon at all
			if (Char()->FindItemByType(_Msg.m_Param0))
				return Char()->CanPickup(_Msg.m_Param0);
			return false;*/
		}
	case OBJMSG_AIQUERY_RPGATTRIBUTE:
		{
			MSCOPESHORT(MESSAGEGROUP_AI);
			if (Char())
			{
				switch (_Msg.m_Param0)
				{
				case AIQUERY_RPGATTRIBUTE_HEALTH:
					{
						return Char()->Health();
					}
				case AIQUERY_RPGATTRIBUTE_MAXHEALTH:
					{
						return Char()->MaxHealth();
					}
				case AIQUERY_RPGATTRIBUTE_WAIT:
					{
						return Char()->Wait();
					}
				case AIQUERY_RPGATTRIBUTE_MANA:
					{
						if (Char())
						{
							CRPG_Object_Ammo *pAmmo = Char()->GetAssociatedAmmo();
							if(!pAmmo)
								return 0;
							return pAmmo->m_NumItems;
						}
						else
							return 0;
					}
				case AIQUERY_RPGATTRIBUTE_MAXMANA:
					{
						if (Char() && Char()->GetAssociatedAmmo())
						{
							return Char()->GetAssociatedAmmo()->GetMaxTotal();
						}
						else
							return 0;
					}
				case AIQUERY_RPGATTRIBUTE_AMMO:
					{
						if (Char())
						{
							CRPG_Object_Ammo *pAmmo = Char()->GetAssociatedAmmo(_Msg.m_Param1);
							if(!pAmmo)
								//Assume this means that weapon doesn't draw ammo... maybe wrong...
								return -1;
							else 
								return pAmmo->GetNumTotal();
						}
						else
							return 0;
					}
				case AIQUERY_RPGATTRIBUTE_LOADEDAMMO:
					{
						if (Char())
						{
							CRPG_Object_Ammo *pAmmo = Char()->GetAssociatedAmmo(_Msg.m_Param1);
							if(!pAmmo)
								//Assume this means that weapon doesn't draw ammo... maybe wrong...
								return -1;
							else
								return pAmmo->GetNumLoaded();
						}
						else
							return 0;
					}
				}
			}
		}
		return 0;

	case OBJMSG_CHAR_GETGAMETICK:
		{
			if (pCD)
				return pCD->m_GameTick;
			return 0;
		}
	case OBJMSG_CHAR_HASAUTOAIMTARGET:
		{
			if(!pCD)
				return 0;
			return pCD->m_AimTarget;
		}
	case OBJMSG_CHAR_GETAUTOAIMOFFSET:
		{
			if(!pCD)
				return 0;

			CVec3Dfp32 Offset = 0.0f;
			Offset.k[2] = (GetAbsBoundBox()->m_Max.k[2] - GetAbsBoundBox()->m_Min.k[2]) * pCD->m_ThisAimHeight;
			return Offset.Pack32(256.0f);
		}


	case OBJMSG_TEAM_NUMTEAMS:
		{
			MSCOPESHORT(MESSAGEGROUP_TEAM);
			return Team_GetNum();
		}

	case OBJSYSMSG_REQUESTFORCESYNC:
		{
//			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if(/*m_bAIControl && */m_Flags & PLAYER_FLAGS_TIMELEAP)
				return 1;
			else
				return 0;
		}
	case OBJMSG_TEAM_GETTEAM:
		{
			MSCOPESHORT(MESSAGEGROUP_TEAM);
			return Team_Get(_Msg.m_Param0);
		}

	case OBJMSG_CHAR_RAISEVISIBILITY:
		if (m_RaisedVisibility < _Msg.m_Param0)
			m_RaisedVisibility = _Msg.m_Param0;
		return 1;

	case OBJMSG_CHAR_SETITEMANIMTARGET:
		{
			if (pCD == NULL)
				return 0;

			bool bPlayer = pCD->m_iPlayer != -1;

			CMTime Timestamp = pCD->m_GameTime + CMTime::CreateFromTicks(bPlayer ?  3 : 0, m_pWServer->GetGameTickTime());
			if(_Msg.m_Reason == 0)
			{
				pCD->m_Item0_Model.AC_SetTarget(CMTime::CreateFromSeconds(*(fp32 *)&_Msg.m_Param0), *(fp32 *)&_Msg.m_Param1, Timestamp);
				pCD->m_Item0_Model.m_AC_RandSeed = MRTC_RAND();
				pCD->m_Item0_Model.MakeDirty();

				if(_Msg.m_Param1 != 0)
				{
					pCD->m_MuzzleFlashTick0 = pCD->m_GameTick + 2;
					if(pCD->m_LightningLight & M_Bit(0))
						pCD->m_LightningLightTick0 = pCD->m_GameTick + 2;
					m_Flags |= PLAYER_FLAGS_MUZZLEVISIBILITY;
					UpdateVisibilityFlag();
				}
			}
			else
			{
				pCD->m_Item1_Model.AC_SetTarget(CMTime::CreateFromSeconds(*(fp32 *)&_Msg.m_Param0), *(fp32 *)&_Msg.m_Param1, Timestamp);
				pCD->m_Item1_Model.m_AC_RandSeed = MRTC_RAND();
				pCD->m_Item1_Model.MakeDirty();

				if(_Msg.m_Param1 != 0)
				{
					pCD->m_MuzzleFlashTick1 = pCD->m_GameTick + 2;
					if(pCD->m_LightningLight & M_Bit(1))
						pCD->m_LightningLightTick1 = pCD->m_GameTick + 2;
					m_Flags |= PLAYER_FLAGS_MUZZLEVISIBILITY;
					UpdateVisibilityFlag();
				}
			}

			return 1;
		}

	case OBJMSG_CHAR_ISDEAD:
		if(Char_GetPhysType(this) == PLAYER_PHYS_DEAD)
			return 1;
		else
			return 0;

	case OBJMSG_CHAR_NEVERTRIGGER:
		{
			if (!pCD)
				return 0;
			if(_Msg.m_Param0 == 2)
				return (pCD->m_Disable & PLAYER_DISABLE_TRIGGER) != 0;
			else if(_Msg.m_Param0 == 1)
			{
				// Remove after Riddick GM, use OBJMSG_CHAR_DISABLE instead
				pCD->m_Disable = pCD->m_Disable | PLAYER_DISABLE_TRIGGER | PLAYER_DISABLE_ACTIVATE;
				return 1;
			}
			else
			{
				// Remove after Riddick GM, use OBJMSG_CHAR_DISABLE instead
				pCD->m_Disable = pCD->m_Disable & ~(PLAYER_DISABLE_TRIGGER | PLAYER_DISABLE_ACTIVATE);
				return 1;
			}
		}
	case OBJMSG_CHAR_DISABLE:
		{
			if (!pCD)
				return 0;
			static const char *FlagsTranslate[] =
			{
				"Trigger", "Activate", "Attack", "Move", "Look", "Grab", "Darknesspowers", "SwitchWeapon",NULL
			};
			pCD->m_Disable = CStrBase::TranslateFlags((char*)_Msg.m_pData, FlagsTranslate);
			if (pCD->m_Disable & PLAYER_DISABLE_MOVE)
				pCD->m_Phys_Flags = pCD->m_Phys_Flags | PLAYER_PHYSFLAGS_IMMOBILE;
			else
				pCD->m_Phys_Flags = pCD->m_Phys_Flags & ~PLAYER_PHYSFLAGS_IMMOBILE;

			if (pCD->m_Disable & PLAYER_DISABLE_DARKNESSPOWERS)
			{
				// Make sure all powers are turned off
				Char_TurnOffDarknessPowers();
			}
		}
		return 1;

	case OBJMSG_CHAR_EQUIPITEMTYPE: 
		{
			if (_Msg.m_Param0 == EQUIPITEM_SPECIALTYPE_INVALID)
				return 0;

			bool bRes = false;
			int iSlot;
			int ItemType;
			if (_Msg.m_Param0 == EQUIPITEM_SPECIALTYPE_USESAMEASOTHER)
			{
				CWObject * pOther = AI_GetSingleTarget(_Msg.m_Param1,(char *)_Msg.m_pData,_Msg.m_iSender);
				if (!pOther)
					return 0;
				
				iSlot = AG2_ITEMSLOT_WEAPONS;
				ItemType = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETPENDINGEQUIPITEMTYPE), pOther->m_iObject);
			}
			else
			{
				iSlot = _Msg.m_Param0 >> 8;
				ItemType = _Msg.m_Param0;
			}


			if (iSlot == AG2_ITEMSLOT_WEAPONS)
			{
				if (ItemType == 7)
				{
					// Ancient weapons and shield
					if (pCD->m_Darkness > 0)
					{
						Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD, true, true);
						Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSVISION, true, true);
						pCD->m_DarknessSelectedPower = PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS;
						return Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS, true, true);
					}
					return false;
				}
				else
				{
					// Deactivate ancient weapons if they are selected
					if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS)
						Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS, true, false);
				}

				CRPG_Object_Inventory * pWeaponInv = Char()->GetInventory(AG2_ITEMSLOT_WEAPONS);
				int32 iCurEquipped = pWeaponInv->GetFinalSelectedItemIndex();
				CRPG_Object_Item* pCurItem = pWeaponInv->FindFinalSelectedItem();//Char()->GetFinalSelectedItem(AG2_ITEMSLOT_WEAPONS);
				int PreviousWeaponId = 0;
				int PreviousItemType = 0;
				int ItemIndexPrev = -1;
				int ExtraFlags2 = 0;
				if (pCurItem)
				{
					// Holster gun
					ExtraFlags2 = pCurItem->m_Flags2 & RPG_ITEM_FLAGS2_NOAMMODRAW;
					PreviousWeaponId = pCurItem->m_Identifier;
					PreviousItemType = pCurItem->m_iItemType;
					ItemIndexPrev = Char()->GetInventory(0)->FindItemIndexByIdentifier(pCurItem->m_Identifier);
				}
				// Find item to equip
				CRPG_Object_Item* pItemToEquip = Char()->FindItemByType(ItemType);
				if (pItemToEquip)
				{
					pItemToEquip->m_Flags2 |= ExtraFlags2;
					int ItemIndex = Char()->GetInventory(0)->FindItemIndexByIdentifier(pItemToEquip->m_Identifier);
					pCD->m_iCurSelectedItem = ItemIndex;
					// Set current selected item
					// Select fist (go through ag and stuff...)
					if (pCD->m_iCurSelectedItem != ItemIndexPrev)
					{
						//Switch weapon
						m_LastUsedItem = PreviousWeaponId;
						m_LastUsedItemType = PreviousItemType;

						// AG2 version
						CWAG2I_Context AGContext(this,m_pWServer,pCD->m_GameTime);
						pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER,pItemToEquip->m_Identifier);
						if (pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_UNEQUIP),0))
						{
							pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED,true);
						}
						else
						{
							SelectItemByIdentifier(0,pItemToEquip->m_Identifier,true);
						}
					}
					pCD->m_WeaponUnequipTick = m_pWServer->GetGameTick();
					m_WeaponUnequipTimer = 0;
				}
			}
			else
				bRes = Char_SelectItemByType(ItemType);

			return bRes;
		}
	
	case OBJMSG_CHAR_GETDISABLE:
		{
			return (pCD ? pCD->m_Disable : 0);
		}
	case OBJMSG_CHAR_GETAIPRIORITYCLASS:
		{
			if (m_spAI)
				return m_spAI->GetCurrentPriorityClass();
			return 0;
		}

	case OBJMSG_CHAR_ACTIVATEITEM:
		{
			return OnCharActivateItem(_Msg, pCD);
		}

	case OBJMSG_CHAR_SETEXTRAITEM:
		{
			return OnCharSetExtraItem(_Msg, pCD);
		}

	case OBJMSG_CHAR_RELOADITEM:
		{
			CRPG_Object_Item *pItem = Char()->GetEquippedItem(_Msg.m_Param0);
			if(pItem)
			{
				//ConOutL(CStrF("Activating WeaponAG: %f Tick: %d",Context.m_GameTime.GetTime(),_pCD->m_GameTick));
				// Check anim events directly
				//_pCD->m_WeaponAG2.GetAG2I()->CheckAnimEvents(&Context, OBJMSG_CHAR_ONANIMEVENT, ANIM_EVENT_MASK_EFFECT);
				pItem->Reload(m_iObject);
				// Update weapon status
				if (_Msg.m_Param0)
				{
					pCD->m_WeaponStatus1.m_AmmoLoad = pItem->GetAmmo(Char());
					pCD->m_WeaponStatus1.MakeDirty();
				}
				else
				{
					pCD->m_WeaponStatus0.m_AmmoLoad = pItem->GetAmmo(Char());
					pCD->m_WeaponStatus0.MakeDirty();
				}

				return true;
			}
			else
				return false;
		}

	case OBJMSG_CHAR_RAISENOISELEVEL:
		if (m_RaisedNoiseLevel < _Msg.m_Param0)
			m_RaisedNoiseLevel = _Msg.m_Param0;
		return 1;

	case OBJMSG_CHAR_APPLYWAIT:
		{
			Char()->ApplyWait(_Msg.m_Param0);
			return 1;
		}
	case OBJMSG_CHAR_GETTARGET:
		{
			if (pCD == NULL)
				return 0;

			return pCD->Target_GetObject();
		}
	case OBJSYSMSG_GETANIMSTATE :
		{
			CXR_AnimState Anim;
			if (!_Msg.m_pData) return 0;
			if (_Msg.m_DataSize != sizeof(Anim)) return 0;

			CXR_Skeleton* pSkel;
			CXR_SkeletonInstance* pSkelInstance;					
			if(!GetEvaluatedSkeleton(pSkelInstance, pSkel, Anim))
				return 0;

			*(CXR_AnimState*)_Msg.m_pData = Anim;
			return 1;
		}

	case OBJSYSMSG_GETPHYSANIMSTATE :
		{
			CXR_AnimState Anim;
			if (!_Msg.m_pData) return 0;
			if (_Msg.m_DataSize != sizeof(Anim)) return 0;

			CXR_Skeleton* pSkel;
			CXR_SkeletonInstance* pSkelInstance;					
			if(!GetEvaluatedPhysSkeleton(pSkelInstance, pSkel, Anim))
				return 0;

			*(CXR_AnimState*)_Msg.m_pData = Anim;
			return 1;
		}

	case OBJMSG_CHAR_GETREQUIREDCLEARANCELEVEL:
		return m_RequiredClearanceLevel;

	case OBJMSG_CHAR_GETGIVENCLEARANCELEVEL:
		{
			int GivenClearanceLevel = m_MinSecurityClearance;
			for(int i = 0; i < RPG_CHAR_NUMCHILDREN; i++)
			{
				CRPG_Object_Inventory *pInv = Char()->GetInventory(i);
				for(int j = pInv->GetNumItems() - 1; j >= 0; j--)
				{
					int cl = pInv->GetItemByIndex(j)->m_GivenClearanceLevel;
					if (cl > GivenClearanceLevel)
						GivenClearanceLevel = cl;
				};
			}
			return GivenClearanceLevel;
		}
	
	case OBJMSG_CHAR_SETIDLESTANCE:
		{
			if (pCD)
			{
				pCD->m_Anim_IdleStance = (int8)_Msg.m_Param0;
				return 1;
			}
			else
				return 0;
		}

	case OBJMSG_CHAR_ISINFIGHTMODE:
		{
			if (pCD)
				return pCD->m_iFightingCharacter;
			return -1;
		}
	case OBJMSG_RPG_SPEAK_OLD:
		{
			if(_Msg.m_pData)
			{
				if(CStrBase::CompareNoCase((char *)_Msg.m_pData, "nooverride") == 0)
				{
					if (pCD && pCD->m_DialogueInstance.IsValid())
						return 0;
				}
			}
			return CWObject_RPG::OnMessage(_Msg);
		}
	case OBJMSG_RPG_SPEAK:
		{
			if(_Msg.m_pData)
			{
				if(CStrBase::CompareNoCase((char *)_Msg.m_pData, "nooverride") == 0)
				{
					if (pCD && pCD->m_DialogueInstance.IsValid())
						return 0;
				}
			}
			return CWObject_RPG::OnMessage(_Msg);
		}



/*********************************************************************************************
 * END MESSAGE ORDERING
 *********************************************************************************************/

	case OBJMSG_CHAR_GETCHOICES: // On Server
		{
			if (!pCD)
				return 0;
			
			return pCD->m_liDialogueChoice.Len();
		}
		return 0;

	case OBJMSG_CHAR_GETLOOKDIRANDPOSITION:
		{
			if (!_Msg.m_pData || _Msg.m_DataSize < 2*sizeof(CVec3Dfp32))
				return 0;
			CVec3Dfp32* pVec = (CVec3Dfp32*)_Msg.m_pData;
			CVec3Dfp32 tmp(0,0,0);
			CMat4Dfp32 Mat;
			if(pCD->m_RenderAttached != 0)
			{
				CWObject *pObj = m_pWServer->Object_Get(pCD->m_RenderAttached);
				if(pObj)
				{
					CMTime Time = pCD->m_GameTime;
					CWObject_Message Msg(OBJMSG_HOOK_GETRENDERMATRIX, (aint)&Time,(aint)&Mat);
					//			fp32 Time = 0;
					//			CWObject_Message Msg(OBJMSG_HOOK_GETRENDERMATRIX, (int)&Time, (int)&MatIP);
					if(!m_pWServer->Message_SendToObject(Msg, pCD->m_RenderAttached))
						Mat = pObj->GetPositionMatrix();
				}
				else
					Mat = GetPositionMatrix();
			}
			else
			{
				Mat = GetPositionMatrix();
			}

			Camera_Get(m_pWServer, &Mat, this, 0);
			Mat.RotY_x_M(-0.25f);
			Mat.RotX_x_M(0.25f);
			pVec[0] = Mat.GetRow(3);
			pVec[1] = Mat.GetRow(0);
			return 1;

			/*if (CWObject_Character::GetEvaluatedPhysSkeleton(this, m_pWServer, pSkelInstance, pSkel, Anim))
			{
				const CMat4Dfp32& Mat = pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_HEAD];
				tmp = pSkel->m_lNodes[PLAYER_ROTTRACK_HEAD].m_LocalCenter;
				tmp *= Mat;
				// Put pos in 0 and lookdir in 1
				pVec[0] = tmp;
				pVec[1] = GetPositionMatrix().GetRow(0);
				CWO_Character_ClientData* pCD = GetClientData(this);
				if (pCD->m_iMountedObject != 0)
				{
					CVec3Dfp32 Body = GetLook(pVec[1]);
					CWObject_CoreData* pObj = m_pWServer->Object_GetCD(pCD->m_iMountedObject);
					CWO_Character_ClientData* pCDOther = (pObj ? GetClientData(pObj) : NULL);
					if (pCDOther)
					{
						Body[2] -= pCDOther->m_TurnCorrectionTargetAngle;
						Body[1] -= pCDOther->m_TurnCorrectionTargetAngleVert;
					}
					CMat4Dfp32 TempMat;
					Body.CreateMatrixFromAngles(0, TempMat);
					pVec[1] = TempMat.GetRow(0);
				}
				return 1;
			}
			return 0;*/
		}

	case OBJMSG_CHAR_SETCURRENTSURVEILLANCECAMERA:
		{
			if (!pCD)
				return 0;

			int16 iObject = _Msg.m_Param0;
			pCD->m_iSurveillanceCamera = iObject;
			ConOut(CStrF("Set surveillancecamera to %i", iObject));
		}
		return 1;

	case OBJMSG_CHAR_BLUREFFECT:
		{
			if(pCD->m_iPlayer != -1)
			{
				CFStr BlurData = (char *)_Msg.m_pData;
				uint8 Type = BlurData.GetStrSep(",").Val_int();
				fp32 Time = BlurData.GetStrSep(",").Val_fp64();
				uint8 Dir = BlurData.GetStrSep(",").Val_int();
				uint8 Sound = BlurData.GetStrSep(",").Val_int();

				uint8 Flags = 0;
				Flags |= (Dir << 5);
				Flags |= (Type) ? PLAYER_HITEFFECT_FLAGS_DAMAGE : 0;
				Flags |= (Sound) ? PLAYER_HITEFFECT_FLAGS_SOUND : 0;
				pCD->m_HitEffect_Flags = (Flags | PLAYER_HITEFFECT_RENDERMASK | PLAYER_HITEFFECT_FLAGS_DAMAGE);
				pCD->m_HitEffect_Duration = m_pWServer->GetGameTick() + TruncToInt(Time * m_pWServer->GetGameTicksPerSecond());
				if (pCD->m_HitEffect_iObject.m_Value != 0)
					pCD->m_HitEffect_iObject = 0;
			}
			else
				ConOutL(CStrF("§cf80WARNING: %s is not a player, can't set blur effect on NPCs", GetName()));
		}
		return 1;

	case OBJMSG_CHAR_SETOPACITY:
		{
			int Alpha = _Msg.m_Param0;
			if (Alpha >= 0)
			{
				int32 Tick = m_pWServer->GetGameTick();
				if (_Msg.m_pData)
					Tick = 1;
				// Set opacity value - will control surface time
				if (pCD->m_Opacity.Set(1 + Clamp(Alpha, 0, 254), Tick, 0.0f))
					pCD->m_Opacity.MakeDirty();
			}
			else // -1
			{
				// Disable completely - will make the surface time animate as usual
				if (pCD->m_Opacity.Set(0, 0, 0.0f))
					pCD->m_Opacity.MakeDirty();
			}
		}
		return 1;

	case OBJMSG_CHAR_ENTERSNEAK:
		{
			if (!pCD)
				return 0;

			m_Player.m_SneakTriggerCount++;
			if(m_Player.m_SneakTriggerCount > 0)
			{
				if(pCD->m_SneakLevel.Set(255, pCD->m_GameTick, 0.0f))
					pCD->m_SneakLevel.MakeDirty();
			}
//			ConOutL(CStrF("Entering new sneak trigger. Currently in %i sneak triggers.", m_SneakTriggerCount));
		}
		return 1;

	case OBJMSG_CHAR_LEAVESNEAK:
		{
			//ConOutL(CStr("OnMessage: Toggle sneak"));
			if (!pCD)
				return 0;

			if (!m_Player.m_SneakTriggerCount)
			{
				ConOutL("WARNING: Trying to leave sneak zone when not in a sneak zone (typ)");
				return 0;
			};

			m_Player.m_SneakTriggerCount--;
			if(m_Player.m_SneakTriggerCount == 0)
			{
				if(pCD->m_SneakLevel.Set(0, pCD->m_GameTick, 0.0f))
					pCD->m_SneakLevel.MakeDirty();
			}
//			ConOutL(CStrF("Exiting a sneak trigger. Currently in %i sneak triggers.", m_SneakTriggerCount));
		}
		return 1;

	case OBJMSG_CHAR_ENTERSOFTSPOT:
		m_iSoftSpotCount++;
		return 1;

	case OBJMSG_CHAR_LEAVESOFTSPOT:
		m_iSoftSpotCount--;
		return 1;


	case OBJMSG_CHAR_RAISEREQUIREDCLEARANCELEVEL:
		if (m_RequiredClearanceLevel < _Msg.m_Param0)
			m_RequiredClearanceLevel = _Msg.m_Param0;
		return 1;

	case OBJMSG_CHAR_CLEARREQUIREDCLEARANCELEVEL:
		m_RequiredClearanceLevel = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETMINIMUMCLEARANCELEVEL), m_pWServer->Game_GetObjectIndex());
		return 1;


	case OBJMSG_CHAR_STUN:
		{
			if(pCD)
			{
				pCD->m_ElectroShockTick = pCD->m_GameTick;
				if(_Msg.m_Param0)
				{
					// ElectroAnim=1,OnlyEffect=2,ForceStun=4
					if(_Msg.m_Param0 & 4)
						m_TranquillizerIncoming = 5; // tranq-gun strength
				}
				else if (m_Flags & PLAYER_FLAGS_STUNNABLE)
				{
					pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_ISSLEEPING,true);
					m_TranquillizerLevel = 101;
					m_TranquillizerIncoming += _Msg.m_Param1;
				}

				/*if(pCD->m_DialogueInstance.m_iDialogueItem != 0)
				{ 
					pCD->m_DialogueInstance.Reset(0, 0, 0, 0);
					ConOutL("KILLING VOICE MSG");
					CNetMsg Msg(PLAYER_NETMSG_KILLVOICE);
					m_pWServer->NetMsg_SendToObject(Msg, m_iObject);
				}*/

				return 1;
			}

			return 0;
/*
			int HitLocation = _Msg.m_Param1;
			CWObject_Message Msg(OBJMSG_SCRIPTIMPULSE, CAI_Core::IMPULSE_PAUSE);
			return OnMessage(Msg);
*/
		}

	case OBJMSG_CHAR_DRAG:
		{
			//if (m_spRagdoll)
			if( m_pPhysCluster )
			{
				//Param0 decides "drag mode": 
				CWObject * pDragger = NULL;
				switch (_Msg.m_Param0)
				{
				case 0:
					{
						//Drag to named object
						pDragger = AI_GetSingleTarget(_Msg.m_Param0,(char *)_Msg.m_pData,_Msg.m_iSender);
						if (!pDragger) return 0;
						break;
					}
				}
				CVec3Dfp32 Pos = (pDragger) ? pDragger->GetPosition() : _Msg.m_VecParam0;
				
				//Always drag by right hand for now. Woe unto he who tries to change this :)
				// m_spRagdoll->SetPendingBonepos(PLAYER_ROTTRACK_RHAND, Pos);

				//New way, still sucks, but anyway...
				{
					CWPhys_ClusterObject &PCO = (m_pPhysCluster->m_lObjects.Len() > 7) ?
						m_pPhysCluster->m_lObjects[7] : m_pPhysCluster->m_lObjects[0];

					//Counter gravity
					m_pWServer->Phys_AddForce(PCO.m_pRB, CVec3Dfp32(0, 0, 9.81f*GetMass())); // Cancel out gravity
					PCO.m_Velocity.m_Rot.m_Angle = 0;

					//Add force to adjust position
					fp32 Mass = PCO.m_pRB->GetMass();
					CVec3Dfp32 AvgForce = (Pos - PCO.m_Transform.GetRow(3)) * Mass;
					m_pWServer->Phys_AddForce(PCO.m_pRB,AvgForce);
					if( AvgForce.LengthSqr() < Sqr(GetMass() * 10.0f) )
					{
						CVec3Dfp32 Force = AvgForce * (1.0f / Mass);
						TAP<CWPhys_ClusterObject> pPCO = m_pPhysCluster->m_lObjects;
						for(uint i = 1; i < pPCO.Len();i++)
						{
							m_pWServer->Phys_AddForce(pPCO[i].m_pRB, Force * pPCO[i].m_pRB->m_Mass);
							pPCO[i].m_Velocity.m_Rot.m_Angle *= 0.5f;
							pPCO[i].m_Velocity.m_Move *= 0.7f;
						}
					}
				}
				return 1;
			}
			return 0;
		}

	case OBJMSG_CHAR_PLAYANIM:
		{
			return OnCharPlayAnim(_Msg, pCD);
		}
	case OBJMSG_CHAR_ONANIMEVENT:
		{
			OnCharAnimEvent(_Msg, pCD, (CXR_Anim_DataKey *)_Msg.m_Param0);
			return 1;
		}
	case OBJMSG_CHAR_HIDEMODEL:
		{
			if(pCD)
			{
				switch (_Msg.m_Param0)
				{
				case 0:
					{
						// Hidemodel off
						pCD->m_ExtraFlags = pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_HIDDEN;
						break;
					}
				case 1:
					{
						// Hidemodel on
						pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_HIDDEN;
						break;
					}
				case 2:
					{
						// No render head
						pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_NORENDERHEAD;
						break;
					}
				case 3:
					{
						// Render head again
						pCD->m_ExtraFlags = pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_NORENDERHEAD;
						break;
					}
				default:
					break;
				}
			}

			// Make sure the autovar are sent instantly, even if OnRefresh already has run
			m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;

			return 1;
		}

	case OBJMSG_CHAR_SETFLASHLIGHTCOLOR:
		{
			if(pCD)
			{
				pCD->m_FlashLightColor = _Msg.m_Param0;
				pCD->m_FlashLightRange = _Msg.m_Param1;
			}
			return 1;
		}

		/*
	case OBJMSG_CHAR_SETMUZZLELIGHTCOLOR:
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if(pCD)
			{
				pCD->m_MuzzleLightColor = _Msg.m_Param0;
				pCD->m_MuzzleLightRange = _Msg.m_Param1;
			}
			return 1;
		}
		*/

	case OBJMSG_CHAR_SETANIMSTANCE:
		{
			Char_SetAnimStance(this, _Msg.m_Param0);
			return 1;
		}

	case OBJMSG_CHAR_SETANIMSOUND:
		{
			if(pCD)
			{
				if(_Msg.m_Param0 == 5)
					pCD->m_AnimSound5 = _Msg.m_Param1;
			}
			return 1;
		}
		
	case OBJMSG_CHAR_PROCESSCONTROL:
		{
			// I am most probably a boar/wolf
			
			CControlFrame *pFrame = (CControlFrame *)_Msg.m_pData;
			int Pos = _Msg.m_Param1;
			
			if(!pCD)
				return 0;

			CCmd Cmd;
			pFrame->GetCmd(Pos, Cmd);
			pCD->m_PendingControlFrame.AddCmd(Cmd);

			return 1;
		}


	case OBJMSG_CHAR_SETCLIENTFLAGS: 
		{
			ClientFlags() = (m_ClientFlags | _Msg.m_Param0) & ~_Msg.m_Param1;
			return 1;
		}
	case OBJMSG_CHAR_GETIMMUNE:
		{
			return IsImmune();
		}		

	case OBJMSG_PHYSICS_KILL:
		{

			return Physics_Kill(_Msg.m_Param0, _Msg.m_iSender);
		}

	case OBJMSG_DAMAGE:
		{
			const CWO_DamageMsg *pMsg = CWO_DamageMsg::GetSafe(_Msg);
			if(pMsg)
				return Physics_Damage(*pMsg, _Msg.m_iSender);
		}
	case OBJMSG_RADIALSHOCKWAVE:
		{
			const CWO_ShockwaveMsg *pMsg = CWO_ShockwaveMsg::GetSafe(_Msg);
			if(pMsg)
				return Physics_Shockwave(*pMsg, _Msg.m_iSender);
		}
		
	case OBJMSG_CHAR_SETWAIT:
		{
			Char()->Wait() = _Msg.m_Param0;
			return 1;
		}

	case OBJMSG_CHAR_ADDHEALTH:
		{
			int Res = Char()->ReceiveHealth(_Msg.m_Param0);
			bint bGodMode = (pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_GODMODE);
			if ((Char()->Health() <= 0) && !bGodMode)
				Char_Die(DAMAGETYPE_UNDEFINED, _Msg.m_iSender);
			return Res;
		}

	case OBJMSG_CHAR_INCREASEMAXHEALTH:
		{
			Char()->IncreaseMaxHealth(_Msg.m_Param0*16);
			return 1;
		}

	case OBJMSG_CHAR_SETMAXHEALTH:
		{
			Char()->MaxHealth() = _Msg.m_Param0;
			return 1;
		}

	case OBJMSG_CHAR_GETTENSION:
		{
			if(!pCD)
				return 0;

			if(_Msg.m_Param0 != 0)
				*(fp32 *)_Msg.m_Param0 = pCD->m_SoundLevel.Get(pCD->m_GameTick, 0.0f) / 255.0f;

			if(_Msg.m_DataSize == sizeof(fp32) && pCD)
			{
				fp32 *pTension = (fp32 *)_Msg.m_pData;
				*pTension = MinMT(MaxMT(fp32(pCD->m_Tension) / 255.0f, 0.0f), 1.0f);
				return 1;
			}
			else
				return 0;
		}

		/*	case OBJMSG_CHAR_SETNPCBAR:
		{
			CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
			if (pCD && _Msg.m_pData)
				pCD->m_iNPCBar = m_pWServer->Selection_GetSingleTarget((char *)_Msg.m_pData);
			
			return 1;
		}*/


	case OBJMSG_CHAR_UPDATEITEMMODEL:
		{
			// Set grip type
			if (!_Msg.m_Param0)
				pCD->m_AnimGripTypeRight = _Msg.m_iSender;
			else
				pCD->m_AnimGripTypeLeft = _Msg.m_iSender;
			return OnUpdateItemModel(_Msg.m_Param0, _Msg.m_Param1);
		}

	/*case OBJMSG_CHAR_PLAYWEAPONANIM:
		{
			CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
			if (pCD == NULL)
				return 0;

			pCD->m_Item0_Model.SetSkelAnim(_Msg.m_Param0, pCD->m_GameTime, _Msg.m_Param1);
			pCD->m_Item0_Model.MakeDirty();
			return 1;
		}*/
	case OBJMSG_CHAR_SETWEAPONAG2:
		{
			// Clear old resources and set the new one
			CWAG2I_Context AG2Context(this,m_pWServer,pCD->m_GameTime);
			pCD->m_WeaponAG2.GetAG2I()->ClearSessionInfo(_Msg.m_Param1);
			pCD->m_WeaponAG2.SetInitialProperties(&AG2Context);
			if (_Msg.m_Param0 > 0)
				pCD->m_WeaponAG2.GetAG2I()->SetResourceIndex_AnimGraph2(_Msg.m_Param0,"",_Msg.m_Param1);

			if (_Msg.m_pData && _Msg.m_DataSize == sizeof(CAutoVar_WeaponStatus))
			{
				if (_Msg.m_Param1)
				{
					int8 OldFlags = pCD->m_WeaponStatus1.m_Flags & CAutoVar_WeaponStatus::WEAPONSTATUS_TRIGGERPRESSED;
					pCD->m_WeaponStatus1.CopyFrom(*(CAutoVar_WeaponStatus*)_Msg.m_pData);
					pCD->m_WeaponStatus1.m_Flags |= OldFlags;
					pCD->m_WeaponStatus1.MakeDirty();
				}
				else
				{
					int8 OldFlags = pCD->m_WeaponStatus0.m_Flags & CAutoVar_WeaponStatus::WEAPONSTATUS_TRIGGERPRESSED;
					pCD->m_WeaponStatus0.CopyFrom(*(CAutoVar_WeaponStatus*)_Msg.m_pData);
					pCD->m_WeaponStatus0.m_Flags |= OldFlags;
					pCD->m_WeaponStatus0.MakeDirty();
				}
			}
			else
			{
				// Clear it
				if (_Msg.m_Param1)
				{
					int8 OldFlags = pCD->m_WeaponStatus1.m_Flags & CAutoVar_WeaponStatus::WEAPONSTATUS_TRIGGERPRESSED;
					pCD->m_WeaponStatus1.Clear();
					pCD->m_WeaponStatus1.m_Flags |= OldFlags;
					pCD->m_WeaponStatus1.MakeDirty();
				}
				else
				{
					int8 OldFlags = pCD->m_WeaponStatus0.m_Flags & CAutoVar_WeaponStatus::WEAPONSTATUS_TRIGGERPRESSED;
					pCD->m_WeaponStatus0.Clear();
					pCD->m_WeaponStatus0.m_Flags |= OldFlags;
					pCD->m_WeaponStatus0.MakeDirty();
				}
			}
			
			return true;
		}
	case OBJMSG_CHAR_GETANIMLAYERFROMONLYANIMTYPE:
		{
			// Ok then, this will get animation layers that match given "onlyanimtype"
			// NOTE: Atm it will also give every other layers that do not use "value compare"
			CMTime Time = CMTime::CreateFromTicks(m_pWServer->GetGameTick(),m_pWServer->GetGameTickTime());
			CWAG2I_Context Context(this,m_pWServer,Time);
			int NumLayers = _Msg.m_DataSize;
			pCD->m_AnimGraph2.GetAG2I()->GetValueCompareLayers(&Context,(CXR_AnimLayer*)_Msg.m_pData,NumLayers, _Msg.m_Param0);
			return NumLayers;
		}

	case OBJMSG_CHAR_SETITEMMODELFLAG:
		{
			if (pCD == NULL)
				return 0;

			for (int iModel = 0; iModel < 4; iModel++)
				pCD->SetItemModelFlag(_Msg.m_Param0, iModel, _Msg.m_Param1);

			return 1;
		}

	case OBJMSG_CHAR_UPDATEQUESTICONS:
		{
			Char_UpdateQuestItems();
			return 1;
		}

	case OBJMSG_CHAR_PICKUPITEM:
		{
			//	return Char_PickupItem(_Msg.m_Param0, (char *)_Msg.m_pData, _Msg.m_iSender);
			return Char_PickupItem(_Msg.m_Param1, _Msg.m_Param0, (char *)_Msg.m_pData, _Msg.m_iSender);
		}

	case OBJMSG_CHAR_REMOVEITEM:
		{
			return Char_RemoveItem((char *)_Msg.m_pData);
		}
		
	case OBJMSG_CHAR_SELECTITEMBYNAME:
		{
			return Char_SelectItemByName((char *)_Msg.m_pData);
		}

	case OBJMSG_CHAR_SELECTITEMBYTYPE:
		{
			return Char_SelectItemByType(_Msg.m_Param0);
		}
	case OBJMSG_CHAR_GETEQUIPPEDITEMTYPE:
		{
			CRPG_Object_Item* pEquippedItem = _Msg.m_Param0 ? Char()->GetFinalSelectedItem(AG2_ITEMSLOT_WEAPONS) : Char()->GetEquippedItem(AG2_ITEMSLOT_WEAPONS);
			if (pEquippedItem)
				return pEquippedItem->m_iItemType;

			return 0;
		}
	case OBJMSG_CHAR_GETPENDINGEQUIPITEMTYPE:
		{
			int PendingWeaponID = pCD->m_AnimGraph2.GetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER);
			CRPG_Object_Item* pPendingEquipItem = Char()->FindItemByIdentifier(PendingWeaponID, _Msg.m_Param0);
			if (!pPendingEquipItem)
			{
				// No item pending equip, use currently equipped item
				pPendingEquipItem = Char()->GetEquippedItem(_Msg.m_Param0);
			}
			if (pPendingEquipItem)
			{
				return pPendingEquipItem->m_iItemType;
			}

			return 0;
		}
	case OBJMSG_CHAR_GETEQUIPPEDANIMTYPE:
		{
			CRPG_Object_Item* pEquippedItem = _Msg.m_Param0 ? Char()->GetFinalSelectedItem(AG2_ITEMSLOT_WEAPONS) : Char()->GetEquippedItem(AG2_ITEMSLOT_WEAPONS);
			if (pEquippedItem)
				return pEquippedItem->m_AnimType;

			return 0;
		}
	case OBJMSG_CHAR_USEINFORMATION:
		return OnUseInformation(_Msg);

	case OBJMSG_CHAR_BEGINCUTSCENE:
		return Char_BeginCutscene(_Msg.m_iSender, false, _Msg.m_Param0);

	case OBJMSG_CHAR_ENDCUTSCENE:
		return Char_EndCutscene();

		// Dialogue
	case OBJMSG_CHAR_BEGINDIALOGUE:
		{
			if(_Msg.m_pData)
			{
				int iSpeaker = m_pWServer->Selection_GetSingleTarget((char*)_Msg.m_pData);
				if (iSpeaker > 0)
				{
					if(_Msg.m_Reason == 0)
					{
						CWObject_Character *pChar = TDynamicCast<CWObject_Character>(m_pWServer->Object_Get(iSpeaker));
						// Abort speaker
						if(pChar && pChar->m_iSpeaker)
							m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_DESTROYCAUSUALDIALOGUE, CAI_Action::PRIO_FORCED), pChar->m_iSpeaker);
					}

					return Char_BeginDialogue(iSpeaker, _Msg.m_Param0);
				}
			}
			else if(_Msg.m_iSender != 0)
				return Char_BeginDialogue(_Msg.m_iSender, _Msg.m_Param0);

			return 0;
		}

	case OBJMSG_CHAR_DESTROYCAUSUALDIALOGUE:
		{
//			if(_Msg.m_Param0 >= CAI_Action::PRIO_ALERT && !(m_ClientFlags & PLAYER_CLIENTFLAGS_DIALOGUE))
			if(pCD && _Msg.m_Param0 > pCD->m_DialogueInstance.m_Priority && !(m_ClientFlags & PLAYER_CLIENTFLAGS_DIALOGUE))
			{
				if (pCD && pCD->m_DialogueInstance.IsValid())
				{
					CWRes_Dialogue* pDialogue = GetDialogueResource(this, m_pWServer);
					if (pDialogue && pDialogue->FindEvent_Hash(pCD->m_DialogueInstance.m_DialogueItemHash, CWRes_Dialogue::EVENTTYPE_LINK))
						m_InterruptedItemHash = pCD->m_DialogueInstance.m_DialogueItemHash; // Remember the line that was interrupted
					
					Char_DestroyDialogue();
				}
			}

			return 1;
		}
		
	case OBJMSG_CHAR_SETDIALOGUECHOICES:
		return Char_SetDialogueChoices((const char *)_Msg.m_Param0, _Msg.m_iSender, _Msg.m_Param1);

	case OBJMSG_CHAR_SETDIALOGUETOKENHOLDER:
		{
			CNetMsg Msg(PLAYER_NETMSG_SETTOKENHOLDER);
			Msg.AddInt16((int16)_Msg.m_Param0);
			Msg.AddInt16((int16)_Msg.m_iSender);
			m_pWServer->NetMsg_SendToObject(Msg, m_iObject);
			
			// Make sure we have ended the timeleap
			Char_EndTimeLeap();
			
			if(!Char_SetDialogueTokenHolder(this, m_pWServer, _Msg.m_Param0, _Msg.m_iSender))
				Char_EndDialogue();

			return 1;
		}

	case OBJMSG_CHAR_USE:
		return OnUse(_Msg.m_iSender, _Msg.m_Param0);


	case OBJMSG_CHAR_USEDNAWEAPONS:
		if(_Msg.m_Param0 == 2)
			return m_Flags & PLAYER_FLAGS_USEDNAWEAPONS ? 1 : 0;
		else if(_Msg.m_Param0 == 1)
			m_Flags |= PLAYER_FLAGS_USEDNAWEAPONS;
		else
			m_Flags &= ~PLAYER_FLAGS_USEDNAWEAPONS;
		return 1;

	case OBJMSG_CHAR_IMMUNE:
		{
			if (pCD)
			{
				if (_Msg.m_Param0 == 1)
					pCD->m_Phys_Flags = pCD->m_Phys_Flags | PLAYER_PHYSFLAGS_IMMUNE;
				else
					pCD->m_Phys_Flags = pCD->m_Phys_Flags & ~PLAYER_PHYSFLAGS_IMMUNE;
			}
		};
		return 1;

	case OBJMSG_CHAR_IMMOBILE:
		{
			if (pCD)
			{
				ConOutL("Msg OBJMSG_CHAR_IMMOBILE is old, use OBJMSG_CHAR_DISABLE (Disable) instead");
				 // Immobile flag should probably be moved to m_Disable (as Disable move)
				if (_Msg.m_Param0 == 1)
					pCD->m_Phys_Flags = pCD->m_Phys_Flags | PLAYER_PHYSFLAGS_IMMOBILE;
				else
					pCD->m_Phys_Flags = pCD->m_Phys_Flags & ~PLAYER_PHYSFLAGS_IMMOBILE;
			}
		};
		return 1;

	case OBJMSG_CHAR_PUSH:
		{	// Bots velocity is set to the given amount, in the direction of the pusher/sender object
			// const CWObject_Message& _Msg
			// _pData is the direction object, if not found we use the sender instead
			// Param0 is the force/velocity  (old format)
			const static char* s_lPushOptions[] = { "AutoForce", "Precision", NULL };
			CFStr Params = (const char*)_Msg.m_pData;
			CFStr DirObj = Params.GetStrSep(",");
			uint Options = Params.GetStrSep(",").TranslateFlags(s_lPushOptions);
			fp32 Force2 = Params.GetStrSep(",").Val_fp64();

			CWObject* pObj = AI_GetSingleTarget(_Msg.m_Param1, DirObj.Str(), _Msg.m_iSender);
			if (!pObj)
				pObj = m_pWServer->Object_Get(_Msg.m_iSender);

			M_ASSERT(pCD, "No clientdata!");
			if (pObj && !(pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMOBILE) && !(IsPlayer()))
			{
				CVec3Dfp32 PushVel = pObj->GetPositionMatrix().GetRow(0);
				if (Options & M_Bit(0))
				{ // set force based on speed of sender
					CWObject* pSender = m_pWServer->Object_Get(_Msg.m_iSender);
					if (!pSender) return 0;
					fp32 Velocity = pSender->GetLastPosition().Distance(pSender->GetPosition());
					PushVel *= Velocity * Force2;		// Force is used as velocity modifier when "AutoForce" flag is set
				}
				else if (Options & M_Bit(1))
				{ // use force specified in message (float precision)
					PushVel *= Force2;
				}
				else
				{ // use force specified in message (integer precision)
					PushVel *= (fp32)_Msg.m_Param0;
				}

#ifdef INCLUDE_OLD_RAGDOLL
				if (m_spRagdoll)
					m_spRagdoll->AddImpulse(CConstraintSystem::BODY_ALL, PushVel);
				else
#endif // INCLUDE_OLD_RAGDOLL
					Physics_AddVelocityImpulse(PushVel);
			}
		}
		return 1;

/*	case OBJMSG_CHAR_SETANIMTIMESCALE:
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if (pCD)
			{
				pCD->m_Item0_TimeScale = ((fp32)_Msg.m_Param0 / 65536.0f);
				return 1;
			}
			else
				return 0;
		}*/

	case OBJMSG_CHAR_STRAIGHTENUP:
		{
			if (pCD)
			{
				pCD->m_StraightenUpTick = pCD->m_GameTick+3;
				/*
				if(_Msg.m_Param0)
					pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_NOTURNANIMATION;
				else
					pCD->m_ExtraFlags = pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_NOTURNANIMATION;
				*/
				return 1;
			}
			else
				return 0;
		}

	case OBJMSG_CHAR_SHAKECAMERA:
		{
			return OnCharShakeCamera(_Msg, pCD);
		}

	case OBJMSG_CHAR_RUMBLE:
		{
			if(_Msg.m_pData)
			{
				CNetMsg Msg(PLAYER_NETMSG_RUMBLE);
				Msg.AddStr((char *)_Msg.m_pData);
				m_pWServer->NetMsg_SendToObject(Msg, m_iObject);
				return 1;
			}
			return 0;
		}

	case OBJMSG_CHAR_SETDIALOGUEITEM_APPROACH_OLD:
		{
			m_DialogueItems.m_Approach.Set( CFStrF("%i", Abs(_Msg.m_Param0)), (_Msg.m_Param0 > 0) );
			return 1;
		}

	case OBJMSG_CHAR_SETDIALOGUEITEM_APPROACH:
		{
			m_DialogueItems.m_Approach.Set((const char*)_Msg.m_pData, (_Msg.m_Param0 == 0));
			return 1;
		}

	case OBJMSG_CHAR_SETDIALOGUEITEM_APPROACH2:
		{
			m_DialogueItems.m_ApproachScared.Set((const char*)_Msg.m_pData, (_Msg.m_Param0 == 0));
			return 1;
		}
		
	case OBJMSG_CHAR_SETDIALOGUEITEM_THREATEN:
		{
			m_DialogueItems.m_Threaten.Set((const char*)_Msg.m_pData, (_Msg.m_Param0 == 0));
			return 1;
		}

	case OBJMSG_CHAR_SETDIALOGUEITEM_IGNORE:
		{
			m_DialogueItems.m_Ignore.Set((const char*)_Msg.m_pData, (_Msg.m_Param0 == 0));
			return 1;
		}
	
	case OBJMSG_CHAR_SETDIALOGUEITEM_TIMEOUT:
		{
			m_DialogueItems.m_Timeout.Set((const char*)_Msg.m_pData, (_Msg.m_Param0 == 0));
			return 1;
		}

	case OBJMSG_CHAR_SETDIALOGUEITEM_EXIT:
		{
			m_DialogueItems.m_Exit.Set((const char*)_Msg.m_pData, (_Msg.m_Param0 == 0));
			return 1;
		}

	case OBJMSG_CHAR_SETPAGERNUMBER:
		{
			// Set pager number and start tick
			if (!_Msg.m_pData)
				return 0;

			pCD->m_PagerStartTick = pCD->m_GameTick;
			pCD->m_PagerPhoneNumber.m_Value = (char*)_Msg.m_pData;
			pCD->m_PagerPhoneNumber.MakeDirty();
			// Play pager sound?
			Sound(m_pWServer->GetMapData()->GetResourceIndex_Sound("Gam_pla_page01"));
			
			// Add pager number to address book...
			
			return 1;
		}
	case OBJMSG_CHAR_ADDAVAILABLEDARKLING:
		{
			if (_Msg.m_pData)
			{
				CStr DarklingType = (char*)_Msg.m_pData;
				return Char_AddDarkling(DarklingType);
			}
			return 0;
		}
	case OBJMSG_CHAR_DARKLINGAVAILABLE:
		{
			if (_Msg.m_pData)
				return Char_IsDarklingAvailable((char*)_Msg.m_pData);
			return 0;
		}

	case OBJMSG_CHAR_MODIFYDARKNESSPOWERS:
		{
			int PowerFlags = ResolveDarknessFlags((const char*)_Msg.m_pData);
			if (_Msg.m_Param0 == 0)
			{ // Add powers
				pCD->m_DarknessPowersAvailable = pCD->m_DarknessPowersAvailable | PowerFlags;
				pCD->m_DarknessPowersBackup = pCD->m_DarknessPowersAvailable;
				//M_TRACEALWAYS("[MODIFYDARKNESSPOWERS], Enabled: %s\n", (const char*)_Msg.m_pData);
			}
			else
			{ // Remove powers
				pCD->m_DarknessPowersAvailable = pCD->m_DarknessPowersAvailable & ~PowerFlags;
				//M_TRACEALWAYS("[MODIFYDARKNESSPOWERS], Disabled: %s\n", (const char*)_Msg.m_pData);
			}
			//M_TRACEALWAYS("- available: %x\n", (int)pCD->m_DarknessPowersAvailable);
			pCD->m_DarknessPowers = pCD->m_DarknessPowers & pCD->m_DarknessPowersAvailable;
			return 1;
		}
	case OBJMSG_CHAR_HASDARKNESSPOWER:
		{
			int PowerFlags = ResolveDarknessFlags((const char*)_Msg.m_pData);
			switch (_Msg.m_Param0)
			{
			case 0:
				return ((pCD->m_DarknessPowersAvailable & PowerFlags) == PowerFlags) ? 1 : 0;

			case 1:
				return ((pCD->m_DarknessPowersAvailable & PowerFlags) != 0) ? 1 : 0;
			}
			
			return 0;
		}

	case OBJMSG_CHAR_GETDARKNESS:
		{
			if (!pCD)
				return 0;
			return aint(pCD->m_Darkness.m_Value);
		}

	case OBJMSG_CHAR_SELECTDARKNESSPOWER:
		{
			int Power = ResolveDarknessFlags((const char*)_Msg.m_pData);
			int SelectValue = ResolveDarknessSelectValue(Power); 
			if ((pCD->m_DarknessPowersAvailable & Power) && (SelectValue != -1))
			{
				// We've got power, select it if not already selected!
				if (SelectValue != pCD->m_DarknessSelectedPower)
				{
					// Check if we must turn off any active darkness powers first.
					if (~Power & pCD->m_DarknessSelectionMode.m_Value & 
						(PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS | PLAYER_DARKNESSMODE_POWER_CREEPINGDARK | 
						PLAYER_DARKNESSMODE_POWER_BLACKHOLE | PLAYER_DARKNESSMODE_POWER_DEMONARM))
					{
						// Turn off all darkness powers to be on the safe side, then reactivate shield/vision if appropriate
						Char_TurnOffDarknessPowers();
						if (pCD->m_Darkness > 0)
						{
							Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD, true, true);
							Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSVISION, true, true);
						}
					}

					pCD->m_DarknessLastSelectedPower = pCD->m_DarknessSelectedPower;
					pCD->m_DarknessSelectedPower = SelectValue;
				}
				return 1;
			}
			return 0;
		}
	case OBJMSG_CHAR_ISDARKNESSPOWERACTIVE:
		{
			int Power = ResolveDarknessFlags((const char*)_Msg.m_pData);
			if (pCD->m_DarknessSelectionMode & Power)
				return 1;
			else
				return 0;
		}
	case OBJMSG_CHAR_ADDDARKNESSJUICE:
		{
			// Add darkness
			int Val = Max(0, Min((int)pCD->m_MaxDarkness.m_Value,(int)(pCD->m_Darkness + _Msg.m_Param0)));
			pCD->m_Darkness = (uint8)Val;
			
			// Break out ol' hugin and Munin if so specified
			if ((pCD->m_Darkness > 0) && (CStr((const char*)_Msg.m_pData) == "1"))
			{
				Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD, true, true);
				Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_DARKNESSVISION, true, true);
			}
			return 1;
		}

	case OBJMSG_CHAR_FORCEDARKNESSPOWER:
		{
			// Force start/stop darkness power
			pCD->m_DarknessPowers = pCD->m_DarknessPowers | pCD->m_DarknessPowersAvailable;
			int32 PowerFlags = ResolveDarknessFlags((const char*)_Msg.m_pData);
			if (_Msg.m_Param0)
			{
				return Char_ActivateDarknessPower(PowerFlags,true,_Msg.m_Param0 != 0);
			}
			else
			{
				if (PowerFlags & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK)
				{
					CWObject_CoreData* pObj = m_pWServer->Object_GetCD(pCD->m_iCreepingDark);
					if (pObj)
						m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CREEPINGDARK_BACKTRACK),pObj->m_iObject);
				}
				else if (PowerFlags & (PLAYER_DARKNESSMODE_POWER_DEMONARM|PLAYER_DARKNESSMODE_POWER_BLACKHOLE))
				{
					Char_ActivateDarknessPower(PowerFlags,true,_Msg.m_Param0 != 0);
				}
			}
			return 1;
		}

	case OBJMSG_CHAR_LOCKTOPARENT:
		{
			// 0: Lock Move+Look
			// 1: Lock Move
			// 2: Release (unlink)
			if (_Msg.m_Param0 == 0 || _Msg.m_Param0 == 2)
			{
				CFStr Joint = (const char*)_Msg.m_pData;
				CFStr Parent = Joint.GetStrSep("/");

				// Find parent
				int iParent = m_pWServer->Selection_GetSingleTarget(Parent);
				if (iParent <= 0)
					return 0;

				// Link to parent
				m_pWServer->Object_AddChild(iParent, m_iObject);

				// Lock character (this might be overkill...)
				pCD->m_Phys_Flags = pCD->m_Phys_Flags | PLAYER_PHYSFLAGS_IMMOBILE;
				ClientFlags() |= PLAYER_CLIENTFLAGS_NOMOVE;
				if (_Msg.m_Param0 == 0)
					ClientFlags() |= PLAYER_CLIENTFLAGS_NOLOOK;
	
				// Set joint attach if given
				m_iParentAttach = -1;
				if (Joint.Len())
					m_iParentAttach = Joint.GetStrSep(",").Val_int();

				pCD->m_Phys_IdleTicks = PLAYER_PHYS_IDLETICKS + 1;

			//	M_TRACE("T: %d - [Char %s, %d], now locked to parent: %s\n", m_pWServer->GetGameTick(), 
			//		GetName(), m_iObject, (const char*)_Msg.m_pData);
			}
			else
			{
				// Unlink from parent
				m_pWServer->Object_AddChild(0, m_iObject);

				// Release character
				pCD->m_Phys_Flags = pCD->m_Phys_Flags & ~PLAYER_PHYSFLAGS_IMMOBILE;
				ClientFlags() &= ~(PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK);

			//	M_TRACE("T: %d - [Char %s, %d], now unlocked from parent\n", m_pWServer->GetGameTick(), 
			//		GetName(), m_iObject);
			}
			return 1;
		}
	case OBJMSG_CHAR_ANIMIMPULSE:
		{
			return OnCharAnimImpulse(_Msg, pCD,pCD->m_AnimGraph2.GetAG2I());
		}
	case OBJMSG_CHAR_DEMONHEAD_ANIMIMPULSE:
		{
			if (pCD->m_iDarkness_Tentacles)
				return m_pWServer->Message_SendToObject(_Msg, pCD->m_iDarkness_Tentacles);
			return 0;
		}
	case OBJMSG_CHAR_WEAPONANIMIMPULSE:
		{
			return OnCharAnimImpulse(_Msg, pCD,pCD->m_WeaponAG2.GetAG2I());
		}
	case OBJMSG_CHAR_WEAPONANIMIMPULSESECONDARY:
		{
			return OnCharAnimImpulse(_Msg, pCD,pCD->m_WeaponAG2.GetAG2I(),1);
		}
	case OBJMSG_CHAR_SETSAVEPARAMS:
		{
			// modify saveflags
			m_SaveParams = _Msg.m_Param0;
			return 1;
		}

	case OBJMSG_CHAR_GETDAMAGEBOOST:
		{
			int iBoost = Char()->GetDamageBoost(_Msg.m_Param0);
//			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
//			if (pCD && pCD->m_iPlayer != -1)
//				return iBoost * 2;
//			else 
			return iBoost;
		}
	case OBJMSG_CHAR_GHOST:
		{
			if (_Msg.m_Param0 == 1)
			{
				m_ClientFlags |= PLAYER_CLIENTFLAGS_GHOST;
				if (!(m_Flags & PLAYER_FLAGS_WAITSPAWN))
					return Char_SetPhysics(this, m_pWServer, m_pWServer, Char_GetPhysType(this), false, true);
			}
			else
			{
				m_ClientFlags &= ~PLAYER_CLIENTFLAGS_GHOST;
				if (!(m_Flags & PLAYER_FLAGS_WAITSPAWN))
					return Char_SetPhysics(this, m_pWServer, m_pWServer, Char_GetPhysType(this), false, true);
			}
			return 1;
		}	

	case OBJMSG_CHAR_ISVALIDATTACH:
		{
//			int iBone = _Msg.m_Param0;
//			int iAttachType = _Msg.m_Param1;

			if(pCD && (pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_NOATTACH))
				return 0;
			
#ifndef M_DISABLE_TODELETE

			if (iAttachType == CWObject_AttachModel::AttachType_Leathal)
			{
				int iHeadBone = 11; // FIXME: This is probably nicely defined somewhere. Find that nice constant and use it instead.
				if ((iBone == iHeadBone) || (iBone >= 20))
					return 0;
			}
#endif

			return 1;
		}

	case OBJMSG_CHAR_ADDITEM:
		{
			if(_Msg.m_pData)
				return Char_PickupItem(false, false, (char *)_Msg.m_pData, _Msg.m_iSender);
			else
				return 0;
		}

	case OBJMSG_CHAR_MOVETOKEN:
		{
			if (pCD == NULL)
				return 0;

			ConOutL(CStrF("OBJMSG_CHAR_MOVETOKEN: OLD INACTIVE MESSAGE, DON'T USE (%s)",(char*)_Msg.m_pData));
			return 0;
			//return MoveToken(this,m_pWServer,pCD,_Msg.m_Param0,CStr((char*)_Msg.m_pData),CMTime(),0.0f,20);
		}

	case OBJMSG_CHAR_SETCUTSCENEFOV:
		{
			// Set camera fov to the given fov
			int FovToSet;
			if(_Msg.m_Param0 == 0)
				FovToSet = 90;
			else
				FovToSet = _Msg.m_Param0;
			if (pCD)
			{
				pCD->m_CutsceneFOV = (uint8)FovToSet;

				// Make sure the autovar are sent instantly, even if OnRefresh already has run
				m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;

				return true;
			}

			return false;
		}
	case OBJMSG_CHAR_FOVOVERRIDE:
		{
			CStr FOVStr;
			if (_Msg.m_pData)
				FOVStr = (char*)_Msg.m_pData;
			uint8 FOV = _Msg.m_Param0;
			if (!FOV)
			{
				pCD->m_FovOverridePrevValue = pCD->m_FovOverride;
				// Reset to normal fov
				if (FOVStr.Len())
				{
					pCD->m_FovOverrideStartTick = m_pWServer->GetGameTick() + 2;
					pCD->m_FovOverrideBlendTime = (int8)(FOVStr.Val_fp64() * (fp32)m_pWServer->GetGameTicksPerSecond());
					pCD->m_FovOverridePrevValue = pCD->m_FovOverride;
					pCD->m_FovOverride = pCD->m_BaseFov;
				}
				else
				{
					// Just reset
					pCD->m_FovOverrideStartTick = 0;
					pCD->m_FovOverrideBlendTime = 0;
					pCD->m_FovOverridePrevValue = pCD->m_FovOverride = 0;
				}
			}
			else if (!FOVStr.Len())
			{
				pCD->m_FovOverridePrevValue = pCD->m_FovOverride = FOV;
				// Just set fov
				pCD->m_FovOverrideStartTick = m_pWServer->GetGameTick();
				pCD->m_FovOverrideBlendTime = 0;
			}
			else
			{
				pCD->m_FovOverridePrevValue = pCD->m_FovOverride ? pCD->m_FovOverride : (int8)M_Ceil(pCD->m_BaseFov);
				pCD->m_FovOverride = FOV;
				// Blend the fov
				pCD->m_FovOverrideStartTick = m_pWServer->GetGameTick() + 2;
				pCD->m_FovOverrideBlendTime = (int8)(FOVStr.Val_fp64() * (fp32)m_pWServer->GetGameTicksPerSecond());
			}

			return 1;
		}
	case OBJMSG_CHAR_SETITEMANIMTYPETOPARAM:
		{
			CRPG_Object_Item* pEquippedItem = _Msg.m_Param0 ? Char()->GetFinalSelectedItem(AG2_ITEMSLOT_WEAPONS) : Char()->GetEquippedItem(AG2_ITEMSLOT_WEAPONS);
			uint16 Param = 0;
			if (pEquippedItem)
			{
				if (!_Msg.m_Param0)
					Param = pEquippedItem->m_AnimType;
				else
					Param = pEquippedItem->m_iItemType;
			}
			m_Param = Param;
			
			return 1;
		}
	case OBJMSG_CHAR_CLONEITEMS:
		{
			// Clone items to target object
			if (!_Msg.m_pData)
				return 0;
			int32 iTarget = m_pWServer->Selection_GetSingleTarget((const char*)_Msg.m_pData);
			CWObject_Character* pChar = IsCharacter(iTarget, m_pWServer);
			if (!pChar)
				return 0;
			pChar->m_spRPGObject = Char()->Duplicate();
			pChar->Char()->UpdateItemIdentifiers();
			// Set equipped item and stuff
			CRPG_Object_Item* pItem = Char()->GetEquippedItem(RPG_CHAR_INVENTORY_WEAPONS);
			if (pItem)
			{
				if (_Msg.m_Param0 == 0)
					pChar->Char()->SelectItem(pItem->m_iItemType, true, true,SELECTIONMETHOD_FORCE, true);
				else
					pChar->Char()->SelectItem(pItem->m_iItemType, true, true,SELECTIONMETHOD_NORMAL, true);
			}
			
			pItem = Char()->GetEquippedItem(RPG_CHAR_INVENTORY_ARMOUR);
			if (pItem && pItem->m_Flags2 & RPG_ITEM_FLAGS2_CLONEATTACHITEM)
			{
				CRPG_Object_Inventory* pInv = pChar->Char()->GetInventory(RPG_CHAR_INVENTORY_ARMOUR);
				CRPG_Object_Inventory* pWeapInv = pChar->Char()->GetInventory(RPG_CHAR_INVENTORY_WEAPONS);
				CRPG_Object_Item* pOtherWeapon = pChar->Char()->GetFinalSelectedItem(AG2_ITEMSLOT_WEAPONS);
				TPtr<CRPG_Object_Summon> spOtherItem = (CRPG_Object_Summon*)pChar->Char()->FindItemByName(pItem->m_Name);
				if (pInv && pWeapInv && pOtherWeapon && spOtherItem && pOtherWeapon != spOtherItem)
				{
					pWeapInv->RemoveItemByIdentifier(iTarget, spOtherItem->m_Identifier, true);
					spOtherItem->m_Flags &= ~RPG_ITEM_FLAGS_REMOVED;
					spOtherItem->m_Model.m_iAttachRotTrack = 23;
					spOtherItem->m_Model.m_iAttachPoint[0] = 1;
					//spItem->m_FireTimeout = 0;
					spOtherItem->m_iItemType |= 0x200;
					spOtherItem->m_Flags2 |= RPG_ITEM_FLAGS2_IAMACLONE;
					pInv->AddItem(iTarget, spOtherItem);
					if (_Msg.m_Param0 == 0)
					{
						pInv->ForceSelectItem(spOtherItem->m_iItemType|0x200, pChar->Char(), true);
						pInv->ForceSetEquipped(iTarget);
					}
				}
			}
			return 1;
		}
	case OBJMSG_CHAR_MISSIONCOMPLETED:
		{
			// Check if the mission has been completed
			if (!_Msg.m_pData)
				return 0;

			return m_CompletedMissions.MissionExists((char*)_Msg.m_pData);
		}
	case OBJMSG_CHAR_SETAMMODRAW:
		{
			bool bDrawAmmo = (_Msg.m_Param0 == 1);
			CRPG_Object_Inventory* pWeaponInventory = Char()->GetInventory(RPG_CHAR_INVENTORY_WEAPONS);
			if (pWeaponInventory)
			{
				CRPG_Object_Item * pWeapon;
				for (int i = 0; i < pWeaponInventory->GetNumItems(); i++)
				{
					pWeapon = pWeaponInventory->GetItemByIndex(i);
					if (pWeapon)
					{
						if (bDrawAmmo)
							pWeapon->m_Flags2 &= ~RPG_ITEM_FLAGS2_NOAMMODRAW;
						else
							pWeapon->m_Flags2 |= RPG_ITEM_FLAGS2_NOAMMODRAW;
					}
				}
			}
			if (m_Flags & PLAYER_FLAGS_DUALWIELDSUPPORTED)
			{
				// Left hand weapons are in armour inventory
				pWeaponInventory = Char()->GetInventory(RPG_CHAR_INVENTORY_ARMOUR);
				if (pWeaponInventory)
				{
					CRPG_Object_Item * pWeapon;
					for (int i = 0; i < pWeaponInventory->GetNumItems(); i++)
					{
						pWeapon = pWeaponInventory->GetItemByIndex(i);
						if (pWeapon)
						{
							if (bDrawAmmo)
								pWeapon->m_Flags2 &= ~RPG_ITEM_FLAGS2_NOAMMODRAW;
							else
								pWeapon->m_Flags2 |= RPG_ITEM_FLAGS2_NOAMMODRAW;
						}
					}
				}
			}
			return 1;
		}

	case OBJMSG_CHAR_GETITEM:
		return aint(Char()->FindItemByType(_Msg.m_Param0));
	case OBJMSG_CHAR_GETITEMBYINDEX:
		return aint(Char()->GetItemByIndex(AG2_ITEMSLOT_WEAPONS,_Msg.m_Param0));
	
	case OBJMSG_CHAR_GETITEMFROMSTRING:
		{
			if(_Msg.m_DataSize == sizeof(CStr *))
			{
				CStr *pSt = (CStr *)_Msg.m_pData;

				return aint(Char()->FindItemByName(pSt->GetStr()));
			}

			return NULL;
		}

	case OBJMSG_CHAR_APPLYDAMAGE:
		{
			Char()->ApplyDamage(_Msg.m_Param0);
		}
		return 1;
	case OBJMSG_CHAR_SETBODYANGLEZ:
		{
			if (pCD)
			{
				pCD->m_Anim_BodyAngleZ = *((fp32*)_Msg.m_pData);
				return 1;
			}
			else 
				return 0;
		}
	case OBJMSG_CHAR_SETGRAVITY:
		{
			if (_Msg.m_Param0)
				//Turn gravity on
				m_ClientFlags &= ~PLAYER_CLIENTFLAGS_NOGRAVITY;
			else 
				//Turn garvity off
				m_ClientFlags |= PLAYER_CLIENTFLAGS_NOGRAVITY;
			return 1;
		}
	case OBJMSG_CHAR_MORPHFOV:
		{
			// Morph fov
			pCD->m_TargetFov = *(fp32*)&_Msg.m_Param0;
			pCD->m_FovTimeScale = *(fp32*)&_Msg.m_Param1;
			pCD->m_FovTime = m_pWServer->GetGameTick() + 1;

			return 1;
		}

	case OBJMSG_CHAR_GETFIGHTMODERADIUS:
		{
			return (aint)FIGHTMODE_INITIATERADIUS;
		}

/*
	case OBJMSG_CHAR_GETIDLESTANCE:
		{
			if (pCD)
			{
				return pCD->m_Anim_IdleStance;
			}
			else
				return 0;
		}
*/

	case OBJMSG_CHAR_KILLPLAYER:
		{
			if(!(pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_GODMODE))
			{
				if (_Msg.m_Param1)
					Char_Die(0, _Msg.m_iSender, _Msg.m_Param0);
				else
					Char_Die(0, _Msg.m_iSender);
				
				/*if (pCD->m_iPlayer != -1)
				{
					// Oh no, the player died, set thirdperson camera...?
					pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_THIRDPERSON;
				}*/

				return 1;
			}

			return 0;
		}
	case OBJMSG_CHAR_RENDERATTACHED:
		{
			if(pCD)
			{
				CFStr Str = (const char *)_Msg.m_pData;
				CNameHash TargetNameHash( Str.GetStrSep("/") );
				int iAttachObj = m_pWServer->Selection_GetSingleTarget(TargetNameHash);
				int iJoint = Str.GetStrSep(",").Val_int();
				iJoint = Max(0, iJoint);

			//	M_TRACE("T: %d - [Char %s, %d], now renderattached to: %s (joint: %d)\n", m_pWServer->GetGameTick(), 
			//		GetName(), m_iObject, iAttachObj > 0 ? TargetNameHash.DebugStr().Str() : "<none>", iJoint);

				if (iJoint)
				{
					//AR-NOTE: not exactly sure when we *don't* want 'forceattachmatrix',
					//         but since it's off by default I only enable it when attached
					//         to specific joints. if the non-force mode is needed, we should
					//         probably provide a checkbox in the message...
					pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_FORCEATTACHMATRIX;
				}

				// Make sure the autovar are sent instantly, even if OnRefresh already has run
				pCD->m_RenderAttached = iAttachObj;
				pCD->m_iAttachJoint = iJoint;
				m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
			}
			return 1;
		}
	case OBJMSG_CHAR_PICKFIGHT:
		{
			// Someone wants to fight with you....
			return false;
		}
	case OBJMSG_CHAR_GETCONTROLMODE:
		{
			return Char_GetControlMode(this);
		}
	case OBJMSG_CHAR_GETHEALTH:
		{
			// return the characters health
			return Char()->Health();
		}
	case OBJMSG_CHAR_SETHEALTH:
		{
			// Set char health
			int32 Health = Min(Max((int)_Msg.m_Param0,0),(int)Char()->MaxHealth());
			Char()->Health() = Health;
			return 1;
		}
	case OBJMSG_CHAR_GETMAXHEALTH:
		{
			// return the characters max health
			return Char()->MaxHealth();
		}
	case OBJMSG_CHAR_ISPLAYER:
		{
			// return the characters health
			if (pCD)
			{
				return  (pCD->m_iPlayer != -1);
			}

			return 0;
		}
	case OBJMSG_CHAR_GETUSENAME:
		{
			CFStr *pSt = (CFStr *)_Msg.m_pData;
			if(!pSt)
				return 0;

			*pSt = m_UseName;
			return 1;
		}
	case OBJMSG_CHAR_GETDESCNAME:
		{
			CFStr *pSt = (CFStr *)_Msg.m_pData;
			if(!pSt)
				return 0;

			*pSt = m_DescName;
			return 1;
		}
	case OBJMSG_CHAR_GETFOCUSOFFSET:
		{
			return (aint)(pCD ? pCD->m_FocusFrameOffset : 64.0f);
		}
	case OBJMSG_CHAR_SETFOCUSFRAMEOFFSET:
		{
			if (pCD)
			{
				pCD->m_FocusFrameOffset = (int8)_Msg.m_Param0;
				return true;
			}

			return false;
		}
	case OBJMSG_CHAR_GETFIGHTCHARACTER:
		{
			int32 iBest = -1; 
//			fp32 BestDot = 0.0f;
//			bool bAlreadyFighting = false;
			CharGetFightCharacter(this, pCD, m_pWServer, iBest, 50.0f);

			return iBest;
		}
	/*case OBJMSG_CHAR_GETFIGHTPOSITION:
		{
			if ((_Msg.m_pData != NULL) && (_Msg.m_DataSize >= sizeof(CVec3Dfp32)))
			{
				CWO_Character_ClientData *pCD = GetClientData(this);
				*((CVec3Dfp32*)_Msg.m_pData) = pCD->m_FCharMovement.GetNextTarget(this);

				return true;
			}

			return false;
		}*/
	case OBJMSG_CHAR_FIGHTMODEFLAG:
		{
			if (pCD)
				return pCD->m_FightModeFlag;
			else
				return 0;
		}

	
	case OBJMSG_CHAR_CANBEDROPKILLED:
		{	// Perhaps a special damage factor here?
			if (GetHitlocDamageFactor(DAMAGETYPE_FALL) > 0.0f)
			{
				return(true);
			}
			else
			{
				return(false);
			}
		}

	case OBJMSG_CHAR_GETDAMAGEFACTOR:
		{	
			// *** How do we send the hitlocation? (skip surface?)
			// Check all bits of damagefactor(_Msg.m_Param1) at hitlocation (_Msg.m_Param0)
			fp32 bestFactor = GetHitlocDamageFactor(_Msg.m_Param1,SHitloc::HITLOC_ALL,_Msg.m_Param0);
			int rValue = (int)(bestFactor * 100);
			// We return the best damagefactor times 100 to be useful for measuring attack validity
			return rValue;
		}

	case OBJMSG_CHAR_NOISE:
		{
			fp32* pNoise = (fp32*)_Msg.m_pData;
			if (pNoise)
				*pNoise = GetNoise();
			return 1;
		}
	case OBJMSG_CHAR_VISIBILITY:
		{
			int LightsOut = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETLIGHTSOUT), m_pWServer->Game_GetObjectIndex());
			fp32* pVisibility = (fp32*)_Msg.m_pData;
			if (pVisibility)
				*pVisibility = GetVisibility((LightsOut==1)?true:false);
			return 1;
		}

	case OBJMSG_CHAR_DEACTIVATEITEM:
		{
			CMat4Dfp32 Mat;
			GetActivatePosition(Mat);
			return Char()->DeactivateItem(_Msg.m_Param0, Mat, m_iObject, _Msg.m_Param1);
		}

	case OBJMSG_CHAR_GETEQUIPPEDITEM:
		{
			CRPG_Object_Char* pChar = Char();
			return (aint)(pChar ? pChar->GetEquippedItem(_Msg.m_Param0) : NULL);
		}
	case OBJMSG_CHAR_RESETHEALTHREGENTIME:
		{
			if(pCD)
				Char()->m_LastDamageTick = pCD->m_GameTick;
			return 1;
		}
	case OBJMSG_CHAR_RESETWEAPONUNEQUIPTIMEOUT:
		{
			SetWeaponUnequipTimer();
			return 1;
		}
	case OBJMSG_CHAR_RESETLASTWEAPON:
		{
			m_LastUsedItem = 0;
			m_LastUsedItemType = -1;
			return 1;
		}
	case OBJMSG_CHAR_GETLASTWEAPON:
		{
			return m_LastUsedItem;
		}
	case OBJMSG_CHAR_DEVOURTARGET:
		{
			// Devour target
			return Char_DevourTarget(_Msg.m_Param0);
		}
	case OBJMSG_CHAR_DEVOURTARGET_PREFINISH:
		{
			// Pre finish devouring
			return Char_DevourTarget_PreFinish();
		}
	case OBJMSG_CHAR_DEVOURTARGET_FINISH:
		{
			// Finish devouring
			return Char_DevourTarget_Finish(_Msg.m_Param0);
		}
	case OBJMSG_CHAR_GETINHABITINGDARKLING:
		{
			// Return first darkling type
			CStr* pStr = (CStr*)_Msg.m_pData;
			if(pStr)
			{
				int nPowerups = m_lDarknessPowerups.Len();
				SDarknessPowerup* pPowerups = m_lDarknessPowerups.GetBasePtr();
				for(int i = 0; i < nPowerups; i++)
				{
					if(pPowerups[i].m_Type == PLAYER_DARKNESSPOWERUP_DARKLING)
					{
						*pStr = pPowerups[i].m_Data;
						return 1;
					}
				}
			}
			return 0;
		}
	case OBJMSG_CHAR_DRAWDARKNESSAMOUNT:
		{
			if (_Msg.m_Param0 != 0)
			{
				uint8 Before = pCD->m_Darkness;
				pCD->m_Darkness = (uint8)Max((aint)0, Min((aint)pCD->m_MaxDarkness, (aint)pCD->m_Darkness - (aint)_Msg.m_Param0));
				if (Before != pCD->m_Darkness)
					pCD->m_HealthHudStart = pCD->m_GameTick;
			}
			return pCD->m_Darkness;
		}
	/*case OBGMSG_CHAR_PLAYSKELANIMONITEM:
		{
			CRPG_Object_Char* pChar = Char();
			CRPG_Object_Item* pEquippedItem = pChar->GetEquippedItem(_Msg.m_Param0);
			CWO_Character_ClientData* pCD = GetClientData(this);
			if (pEquippedItem && pCD)
				return pEquippedItem->PlayWeaponAnimFromType(_Msg.m_Param1,m_iObject,_Msg.m_Reason);
			return false;
		}*/

	case OBGMSG_CHAR_NEXTWEAPON:
		{
			NextItem(RPG_CHAR_INVENTORY_WEAPONS, false,true);
			// Update equipped item prop for AG
			CRPG_Object_Item* pEquippedItem = Char()->GetFinalSelectedItem(AG2_ITEMSLOT_WEAPONS);
			if (pCD && pEquippedItem && (pCD->m_EquippedItemClass != (uint16)pEquippedItem->m_AnimProperty))
			{
				pCD->m_EquippedItemClass = (uint16)pEquippedItem->m_AnimProperty;
				pCD->m_EquippedItemType = (uint16)pEquippedItem->m_iItemType;
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_ITEMANIMTYPE,pEquippedItem->m_AnimType);
				return true;
			}
			return false;
		}

	case OBGMSG_CHAR_SWITCHITEMBYINDEX:
		{
			SetWeaponUnequipTimer();
			bool bRes = Char_SelectItemByIndex(_Msg.m_Param0, _Msg.m_Param1);
			if (_Msg.m_Param0 == AG2_ITEMSLOT_WEAPONS)
			{
				// Update equipped item prop for AG
				CRPG_Object_Item* pEquippedItem = Char()->GetFinalSelectedItem(0);
				ConOut(CStrF("Switching to: %d Type: %d",pEquippedItem->m_Identifier, pEquippedItem->m_iItemType));
				if (pCD && pEquippedItem && (pCD->m_EquippedItemClass != (uint16)pEquippedItem->m_AnimProperty))
				{
					pCD->m_EquippedItemClass = (uint16)pEquippedItem->m_AnimProperty;
					pCD->m_EquippedItemType = (uint16)pEquippedItem->m_iItemType;
					pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_ITEMANIMTYPE,pEquippedItem->m_AnimType);
					return true;
				}
				else
					return false;
			}
			return bRes;
		}
	case OBGMSG_CHAR_SWITCHITEMBYIDENTIFIER:
		{
			SetWeaponUnequipTimer();
			// Force drop current item (if any)...
			CRPG_Object_Item* pItem = Char()->GetEquippedItem(_Msg.m_Param0);
			if (pItem && (pItem->m_Flags2 & RPG_ITEM_FLAGS2_THROWAWAYONEMPTY) && !pItem->GetAmmo(Char()))
			{
				if (pItem->m_Flags2 & RPG_ITEM_FLAGS2_PERMANENT)
				{
					// "Magically" Fill up with ammo
					pItem->SetAmmo(pItem->GetMaxAmmo());
				}
				// Only throw item away if we have another one
				CRPG_Object_Item* pNextItem = Char()->FindNextItemByType(pItem->m_Identifier, pItem->m_iItemType & ~0x200);
				if (pNextItem)
				{
					OnMessage(CWObject_Message(OBJMSG_CHAR_FORCEDROPWEAPON,_Msg.m_Param0));
					// Change last weapon (so we can autoequip stuff again...)
					m_LastUsedItem = pNextItem->m_Identifier;
					m_LastUsedItemType = pNextItem->m_iItemType;
				}
			}

			// Check for any weapons that should be thrown away
			Char_RemoveDropWeapon(_Msg.m_Param1);

			// Select next item
			bool bRes = false;
			CRPG_Object_Inventory* pWeapInv = Char()->GetInventory(RPG_CHAR_INVENTORY_WEAPONS);
			if (_Msg.m_Param0 == AG2_ITEMSLOT_DUALWIELD)
			{
				CRPG_Object_Inventory* pDualInv = Char()->GetInventory(RPG_CHAR_INVENTORY_ARMOUR);
				TPtr<CRPG_Object_Summon> spItem = (CRPG_Object_Summon*)pWeapInv->FindItemByIdentifier(_Msg.m_Param1);
				if (spItem)
				{
					if (_Msg.m_DataSize < 0)
					{
						// Put old item back in the "original" inventory
						TPtr<CRPG_Object_Summon> spOldItem = (CRPG_Object_Summon*)pDualInv->GetEquippedItem();
						if (spOldItem && (spOldItem->m_Flags2 & RPG_ITEM_FLAGS2_CLONEATTACHITEM))
						{
							pDualInv->RemoveItemByIdentifier(m_iObject, spOldItem->m_Identifier, true);
							spOldItem->m_Flags &= ~(RPG_ITEM_FLAGS_REMOVED | RPG_ITEM_FLAGS_EQUIPPED);
							spOldItem->m_Model.m_iAttachRotTrack = spItem->m_Model.m_iAttachRotTrack;
							spOldItem->m_Model.m_iAttachPoint[0] = spItem->m_Model.m_iAttachPoint[0];
							spOldItem->m_FireTimeout = spItem->m_FireTimeout;
							spOldItem->m_iItemType &= ~0x200;
							spOldItem->m_Flags2 &= ~RPG_ITEM_FLAGS2_IAMACLONE;
							// Reset identifier
							spOldItem->m_Identifier = 0;
							// Add back to weapons inventory
							pWeapInv->AddItem(m_iObject, spOldItem);
						}
					}
					pWeapInv->RemoveItemByIdentifier(m_iObject, spItem->m_Identifier,true);
					spItem->m_Flags &= ~RPG_ITEM_FLAGS_REMOVED;
					spItem->m_Model.m_iAttachRotTrack = 23;
					spItem->m_Model.m_iAttachPoint[0] = 1;
					spItem->m_iItemType |= 0x200;
					spItem->m_Flags2 |= RPG_ITEM_FLAGS2_IAMACLONE;
					pDualInv->AddItem(m_iObject, spItem);
					pDualInv->ForceSelectItem(spItem->m_iItemType, Char(), true);
					pDualInv->ForceSetEquipped(m_iObject);
					// Set clone identifier to "main" weapon
					CRPG_Object_Item* pItem = Char()->GetFinalSelectedItem(AG2_ITEMSLOT_WEAPONS);
					if (pItem)
						pItem->SetCloneIdentifier(spItem->m_Identifier);
				}
			}
			else
			{
				int32 Identifier = _Msg.m_Param1;
				CRPG_Object_Item* pItem = Char()->FindItemByIdentifier(Identifier,_Msg.m_Param0);
				if (pItem && pItem->m_Flags2 & RPG_ITEM_FLAGS2_FORCEEQUIPLEFT)
				{
					pItem = Char()->FindNextItemByType(pItem->m_Identifier, pItem->m_iItemType,true);
					Identifier = pItem ? pItem->m_Identifier : Identifier;
				}
				if (_Msg.m_DataSize < 0)
				{
					pWeapInv->ForceSetUnequipped(m_iObject);
					pWeapInv->SelectItemByIdentifier(Identifier,Char(),true);;
					pWeapInv->ForceSetEquipped(m_iObject);
				}
				else
					bRes = SelectItemByIdentifier(_Msg.m_Param0, Identifier);

				/*CRPG_Object_Item* pSelectedItem = Char()->GetFinalSelectedItem(AG2_ITEMSLOT_WEAPONS);
				if (_Msg.m_Reason && pItem && pSelectedItem && pSelectedItem->m_AnimType == pItem->m_AnimType)
				{
					// Send equip impulse
					CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_EQUIP);
					CWAG2I_Context Context(this,m_pWServer,pCD->m_GameTime);
					pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&Context,Impulse, 0);
				}*/
			}

			if (_Msg.m_Param0 == AG2_ITEMSLOT_WEAPONS || _Msg.m_Param0 == AG2_ITEMSLOT_DUALWIELD)
			{
				// Equip on weaponag
				CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_EQUIP);
				CWAG2I_Context Context(this,m_pWServer,pCD->m_GameTime);
				int32 iToken = _Msg.m_Param0 == AG2_ITEMSLOT_DUALWIELD ? TOKEN_WEAPON2 : TOKEN_WEAPON1;
				pCD->m_WeaponAG2.GetAG2I()->SendImpulse(&Context,Impulse, iToken);

				// Update equipped item prop for AG
				CRPG_Object_Item* pEquippedItem = Char()->GetFinalSelectedItem(_Msg.m_Param0);
				//ConOut(CStrF("Switching to: %d Type: %d",pEquippedItem->m_Identifier, pEquippedItem->m_iItemType));
				if (pCD && pEquippedItem)// && (pCD->m_EquippedItemClass != (uint16)pEquippedItem->m_AnimProperty))
				{
					pCD->m_EquippedItemClass = (uint16)pEquippedItem->m_AnimProperty;
					pCD->m_EquippedItemType = (uint16)pEquippedItem->m_iItemType;
					pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_ITEMANIMTYPE,pEquippedItem->m_AnimType);

					// msg.m_iSender has number of ticks of wait to put on the new weapon
					if (_Msg.m_iSender > 0)
					{
						pEquippedItem->m_iExtraActivationWait = pCD->m_GameTick + _Msg.m_iSender;
						// Update weapon status
						if (_Msg.m_Param0)
							pCD->m_WeaponStatus1.m_CurrentTimeOut = pEquippedItem->m_iExtraActivationWait;
						else
							pCD->m_WeaponStatus0.m_CurrentTimeOut = pEquippedItem->m_iExtraActivationWait;
					}

					SetGunplayUnsafe(pCD->m_GameTick + _Msg.m_iSender);

					if (_Msg.m_Param0 == AG2_ITEMSLOT_WEAPONS)
					{
						CRPG_Object_Item* pDualWield = Char()->GetFinalSelectedItem(AG2_ITEMSLOT_DUALWIELD);
						if (pDualWield)
							pEquippedItem->SetCloneIdentifier(pDualWield->m_Identifier);
					}

					return true;
				}
				else
					return false;
			}
			return bRes;
		}
	case OBJMSG_CHAR_GETEQUIPPEDITEM_ANIMPROPERTY:
		{
			CRPG_Object_Char* pChar = Char();
			CRPG_Object_Item* pEquippedItem = pChar->GetEquippedItem(_Msg.m_Param0);
			if (pEquippedItem)
				return pEquippedItem->m_AnimProperty;

			return false;
		}

	case OBJMSG_CHAR_ANIMCONTROL:
		{
			if (_Msg.m_Param0)
			{
				//Turn on anim control
				if (Char_GetControlMode(this) != PLAYER_CONTROLMODE_ANIMATION)
					Char_SetControlMode(this, PLAYER_CONTROLMODE_ANIMATION);
			}
			else
			{
				//Switch off anim control
				if (Char_GetControlMode(this) == PLAYER_CONTROLMODE_ANIMATION)
					Char_SetControlMode(this, PLAYER_CONTROLMODE_FREE);
			}
			return 1;
		}
	case OBJMSG_CHAR_DISPLAYHEALTH:
		{
			if (pCD)
				pCD->m_HealthHudStart = m_pWServer->GetGameTick();
			return 1;
		}
	case OBJMSG_CHAR_SETAIMINGMODE: // script
		{
			pCD->m_ForcedAimingMode = (uint8)_Msg.m_Param0;
			return 1;
		}

	case OBJMSG_CHAR_SETCOLLISIONMODE: // script
		{
			if (!pCD)
				return 0;

			if(_Msg.m_pData)
				pCD->m_Phys_Flags = (uint16)atoi((const char *)_Msg.m_pData);
			else
				pCD->m_Phys_Flags = (uint16)_Msg.m_Param0;
			return Char_SetPhysics(this, m_pWServer, m_pWServer, Char_GetPhysType(this), false, true);
		}


	case OBJMSG_CHAR_SETPHYSFLAGS: // script
		{
			if (!pCD)
				return 0;

			pCD->m_Phys_Flags = ResolvePhysFlags((char *)_Msg.m_pData);
			int iType = Char_GetPhysType(this);
			if(iType == PLAYER_PHYS_VOID)
				iType = PLAYER_PHYS_STAND;
			return Char_SetPhysics(this, m_pWServer, m_pWServer, iType, false, true);
		}

	case OBJMSG_DEPTHFOG_SET: // script
		{
			if(!pCD)
				return 0;

			CWObject_WorldSky::CDepthFog Fog;
			Fog.Parse((const char *)_Msg.m_pData);
			pCD->m_DepthFog.Set(Fog, pCD->m_GameTick, 0.0f);
			pCD->m_DepthFog.MakeDirty();
			pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_OVERRIDEFOG;
			return 1;
		}

	case OBJMSG_CHAR_SETSPECIALGRAB: // script
		{
			if(!pCD)
				return 0;
			pCD->m_SpecialGrab = _Msg.m_Param0;
			return 1;
		}

	case OBJMSG_CHAR_GETSPECIALGRAB:
		{
			if (pCD)
			{
				if (pCD->m_AnimGraph2.GetPropertyBool(PROPERTY_BOOL_ISSLEEPING))
					return 3;
				return pCD->m_SpecialGrab;
			}
			return 0;
		}

	case OBJMSG_CHAR_SETGRABBEDBODY:
		{	// If m_GrabbedBody != 0 and _Msg.m_Param0 == 0 we dropped the body
			// Report a dragged dead and let the knowledgbase sort it out
			CAI_Core* pAI = AI_GetAI();
			if (pAI)
			{
				if ((_Msg.m_Param0 == 0) && (m_GrabbedBody != 0))
				{	// Currently dragged body dropped
					if (pAI->m_pServer)
					{
						CWObject* pObj = pAI->m_pServer->Object_Get(m_GrabbedBody);
						if (pObj)
						{
							CVec3Dfp32 Pos = pObj->GetPosition();
							pAI->m_KB.Global_ReportDeath(m_iObject,m_GrabbedBody,Pos,SDead::DEATHCAUSE_MOVED);
						}
					}
				}
				else if (_Msg.m_Param0 != 0)
				{	// We started dragging a body
					pAI->m_KB.Global_ReportDeath(m_iObject,_Msg.m_Param0,CVec3Dfp32(_FP32_MAX),SDead::DEATHCAUSE_MOVED);
				}
			}
			m_GrabbedBody = _Msg.m_Param0;
			// If we dropped the body, stop playing sound
			if(!m_GrabbedBody && m_iSound[1] != 0)
			{
				m_iSound[1] = 0;
				m_iSound[1] |= CWO_DIRTYMASK_SOUND;
			}
			return 1;
		}
	case OBJMSG_CHAR_GETGRABBEDBODY:
		{
			return m_GrabbedBody;
		}

	case OBJMSG_CHAR_GETGRABBEDOBJECT:
		{
			// Get index of currently grabbed object, if any. Note that only one object is returned, even if it might be possible to grab several objects.
			if(pCD && pCD->m_DarknessPowersAvailable)
			{
				// Have we grabbed anything with the schnabel?
				int DemonArmGrabee = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETGRABBEDOBJECT), pCD->m_iDarkness_Tentacles);
				if (DemonArmGrabee)
					return DemonArmGrabee;
			}
			// Have we grabbed anything otherwise? (This is untested; can't grab stuff right now)
			return m_GrabbedObject.m_iTargetObj;
		}

	case OBJMSG_CHAR_SETLASERBEAM: // script
		if(_Msg.m_Param0 == 0)
			ClientFlags() &= ~PLAYER_CLIENTFLAGS_LASERBEAM;
		else
			ClientFlags() |= PLAYER_CLIENTFLAGS_LASERBEAM;
		return 1;

	case OBJMSG_CHAR_SETFLASHLIGHT: // script
		// m_Param0 be 0(off), 1(on) or 2(dynamic, depending on lightmeter)
		if (m_spAI)
		{
			if(_Msg.m_Param0 == 2)
			{
				if (m_spAI->m_UseLightmeter < 1)
				{
					m_spAI->m_UseLightmeter = 1;
				}
				m_spAI->m_bFlashlight = true;
			}
			else if(_Msg.m_Param0 == 1)
			{
				if (m_spAI->m_UseLightmeter > 0)
				{
					m_spAI->m_UseLightmeter = 0;
				}
				m_spAI->m_bFlashlight = false;
				ClientFlags() |= PLAYER_CLIENTFLAGS_FLASHLIGHT;
			}
			else // _Msg.m_Param0 == 0
			{
				if (m_spAI->m_UseLightmeter > 0)
				{
					m_spAI->m_UseLightmeter = 0;
				}
				m_spAI->m_bFlashlight = false;
				ClientFlags() &= ~PLAYER_CLIENTFLAGS_FLASHLIGHT;
			}
		}
		else
		{
			if(_Msg.m_Param0 == 0)
				ClientFlags() &= ~PLAYER_CLIENTFLAGS_FLASHLIGHT;
			else
			{
				ClientFlags() |= PLAYER_CLIENTFLAGS_FLASHLIGHT;
			}
		}
		
		UpdateVisibilityFlag();
		return 1;

	case OBJMSG_CHAR_AICONTROL: // script
		{
			//Did we just toggle the AI control flag?
			if ((!m_bAIControl && (_Msg.m_Param0 == 1)) ||
				(m_bAIControl && (_Msg.m_Param0 != 1)))
				m_bResetControlState = true;

			m_bAIControl = (_Msg.m_Param0 == 1);
			if (m_bAIControl)
			{
				//Set server control (i.e. no prediction)
				m_ClientFlags |= PLAYER_CLIENTFLAGS_SERVERCONTROL;

				//Reset AI scripts
				if (m_spAI)
					m_spAI->m_Script.Clear();

				//Reset NOLOOK and NOMOVE if we're in a cutscene, and still alive
				if ((m_ClientFlags & PLAYER_CLIENTFLAGS_CUTSCENE) &&
					!(Char_GetPhysType(this) == PLAYER_PHYS_DEAD))
					m_ClientFlags &= ~(PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK);
			}
			else
			{
				//Release server control
				m_ClientFlags &= ~PLAYER_CLIENTFLAGS_SERVERCONTROL;

				//Reset AI scripts
				if (m_spAI)
					m_spAI->m_Script.Clear();

				//Set NOMOVE and NOLOOK if we're in a cutscene
				if (m_ClientFlags & PLAYER_CLIENTFLAGS_CUTSCENE)
					m_ClientFlags |= (PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK);
			}
		};
		return 1;

	case OBJMSG_CHAR_DESTROYITEM: // script
		{
			if(_Msg.m_pData)
			{
				char *pItemName = (char *)_Msg.m_pData;
				if(CStrBase::CompareNoCase("all", pItemName) == 0)
				{
					for(int i = 0; i < 2; i++)
					{
						CRPG_Object_Inventory *pInv = Char()->GetInventory(i);
						for(int j = pInv->GetNumItems() - 1; j >= 0; j--)
							Char()->RemoveItemByName(m_iObject, i, pInv->GetItemByIndex(j)->m_Name, _Msg.m_Param1 != 0);
					}
					Char_UpdateQuestItems();
					return 1;
				}
				else
				{
					CRPG_Object_Item *pItem = Char()->FindItemByName(pItemName);
					if(!pItem)
						ConOutL(CStr("Could not destroy item ") + pItemName);
					else
					{
						int iItemType = pItem->m_iItemType;
						if(_Msg.m_Param0 > 0)
							pItem->m_NumItems = Max(0, int(pItem->m_NumItems - _Msg.m_Param0));
						else
							Char()->RemoveItemByType(m_iObject, pItem->m_iItemType, _Msg.m_Param1 != 0);
						OnUpdateItemModel(iItemType >> 8, -1);
						Char_UpdateQuestItems();
						return 1;
					}
				}
			}
			return 0;
		}

	case OBJMSG_CHAR_GETNUMITEMS:
		{
			CRPG_Object_Item *pItem = NULL;
			if(_Msg.m_pData)
				pItem = Char()->FindItemByName((char *)_Msg.m_pData);
			else
				pItem = Char()->FindItemByType(_Msg.m_Param0);

			if(!pItem)
				return 0;
			return pItem->GetAmmo(Char());
		}

	case OBJMSG_CHAR_GETHEALTHPERCENT: // script
		{
			int MaxHealth = Char()->MaxHealth();
			int Health = Char()->Health();
			return 100 * Health / MaxHealth;
		}

	case OBJMSG_CHAR_SETUSENAME: // script
		{
			if(!_Msg.m_pData)
				return 0;

			m_UseName = (const char*)_Msg.m_pData;
			return 1;
		}

	case OBJMSG_CHAR_SETDESCNAME: // script
		{
			if(!_Msg.m_pData)
				return 0;

			m_DescName = (const char*)_Msg.m_pData;
			return 1;
		}
	case OBJMSG_HOOK_GETCURRENTMATRIX:
		{
			//CMTime *pTime = (CMTime *)_Msg.m_Param0;
			CMat4Dfp32 *pMatrix = (CMat4Dfp32 *)_Msg.m_Param0;

			*pMatrix = GetPositionMatrix();
			if (_Msg.m_iSender != -1 && _Msg.m_pData)
			{
				CXR_AnimState AnimState;
				struct SkelProp
				{
					CXR_Skeleton* m_pSkel;
					CXR_SkeletonInstance* m_pSkelInst;
				};
				struct SkelProp* pSkelProp = (struct SkelProp*)_Msg.m_pData;
				GetEvaluatedPhysSkeleton(pSkelProp->m_pSkelInst,pSkelProp->m_pSkel,AnimState,0.0f,pMatrix);
				/*if (pSkelInst)
				{
					pMatrix->GetRow(0) = pSkelInst->m_pBoneTransform[_Msg.m_iSender].GetRow(0);
					pMatrix->GetRow(1) = pSkelInst->m_pBoneTransform[_Msg.m_iSender].GetRow(1);
					pMatrix->GetRow(2) = pSkelInst->m_pBoneTransform[_Msg.m_iSender].GetRow(2);
					pMatrix->GetRow(3) = pSkelInst->m_pBoneTransform[_Msg.m_iSender].GetRow(3);
				}*/
			}
			else
			{
				CXR_AnimState AnimState;
				CXR_Skeleton* pSkel;
				CXR_SkeletonInstance* pSkelInstance;
				GetEvaluatedPhysSkeleton(this, m_pWServer, pSkelInstance,pSkel,AnimState,0.0f,pMatrix);
				if (pSkelInstance)
				{
					uint iNode = _Msg.m_Param1;
					if (iNode && (iNode < pSkel->m_lNodes.Len()))
					{
						CVec3Dfp32 LocalPos = pSkel->m_lNodes[iNode].m_LocalCenter;
						*pMatrix = pSkelInstance->m_pBoneTransform[iNode];
						pMatrix->GetRow(3) = LocalPos * (*pMatrix);
					}
					else if (pSkelInstance->m_nBoneTransform)
					{
						// Get root position and set that as the matrix pos
						if (pSkelInstance->m_nBoneTransform > PLAYER_ROTTRACK_CAMERA)
							*pMatrix = pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_CAMERA];
						pMatrix->GetRow(3) = pSkelInstance->m_pBoneTransform[0].GetRow(3);
					}
				}
			}
			return 1;
		}
	case OBJMSG_HOOK_GETRENDERMATRIX:
		{
			//CWO_Character_ClientData* pCD = GetClientData(_pObj);
			//CMTime Time = CMTime::CreateFromTicks(pCD->m_GameTick,_pWClient->GetGameTickTime(),pCD->m_PredictFrameFrac);
			CMat4Dfp32* pMatrix = (CMat4Dfp32*)_Msg.m_Param1;

			*pMatrix = GetPositionMatrix();
			if (_Msg.m_iSender != -1 && _Msg.m_pData)
			{
				CXR_AnimState AnimState;
				struct SkelProp
				{
					CXR_Skeleton* m_pSkel;
					CXR_SkeletonInstance* m_pSkelInst;
				};
				struct SkelProp* pSkelProp = (struct SkelProp*)_Msg.m_pData;
				GetEvaluatedPhysSkeleton(pSkelProp->m_pSkelInst,pSkelProp->m_pSkel,AnimState,0.0f,pMatrix);
				/*if (pSkelInst)
				{
				pMatrix->GetRow(0) = pSkelInst->m_pBoneTransform[_Msg.m_iSender].GetRow(0);
				pMatrix->GetRow(1) = pSkelInst->m_pBoneTransform[_Msg.m_iSender].GetRow(1);
				pMatrix->GetRow(2) = pSkelInst->m_pBoneTransform[_Msg.m_iSender].GetRow(2);
				pMatrix->GetRow(3) = pSkelInst->m_pBoneTransform[_Msg.m_iSender].GetRow(3);
				}*/
			}
			else
			{
				CXR_AnimState AnimState;
				CXR_Skeleton* pSkel;
				CXR_SkeletonInstance* pSkelInstance;
				GetEvaluatedPhysSkeleton(this, m_pWServer, pSkelInstance,pSkel,AnimState,0.0f,pMatrix);
				if (pSkelInstance)
				{
					uint iNode = _Msg.m_Param0;
					if (iNode && (iNode < pSkel->m_lNodes.Len()))
					{
						CVec3Dfp32 LocalPos = pSkel->m_lNodes[iNode].m_LocalCenter;
						*pMatrix = pSkelInstance->m_pBoneTransform[iNode];
						pMatrix->GetRow(3) = LocalPos * (*pMatrix);
					}
					else if (pSkelInstance->m_nBoneTransform)
					{
						// Get root position and set that as the matrix pos
						if (pSkelInstance->m_nBoneTransform > PLAYER_ROTTRACK_CAMERA)
							*pMatrix = pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_CAMERA];
						pMatrix->GetRow(3) = pSkelInstance->m_pBoneTransform[0].GetRow(3);
					}
				}
			}
			return 1;
		}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	case OBJMSG_TEAM_BELONGTOTEAM:
		{
			MSCOPESHORT(MESSAGEGROUP_TEAM);
			//Check own teams
			for (int i = 0; i < Team_GetNum(); i++)
			{	
				if (_Msg.m_Param0 == Team_Get(i))
				{
					//Jupp, we belong to team
					return 1;
				}
			}	

			//Don't belong to team
			return 0;
		}
	case OBJMSG_TEAM_JOINTEAM:
		{
			MSCOPESHORT(MESSAGEGROUP_TEAM);
			if (_Msg.m_Param0)
			{
				Team_Add(_Msg.m_Param0);
				return 1;
			}
			else
			{
				return 0;
			}
			
		}
	case OBJMSG_TEAM_LEAVETEAM:
		{
			MSCOPESHORT(MESSAGEGROUP_TEAM);
			if (_Msg.m_Param0)
			{
				if (Team_Remove(_Msg.m_Param0))
				{
					return 1;
				}
			}
			return 0;
		}
	////////////////////////////////////////////////////////////////////////////////////////////////////////

	// put this in interface_ai 
	case OBJMSG_CHAR_SETFLAGS:
		{
			m_Flags = (m_Flags | _Msg.m_Param0) & ~_Msg.m_Param1;
			return 1;
		}
	case OBJMSG_CHAR_GETFLAGS:
		{
			return m_Flags;
		}

	case OBJMSG_AIQUERY_ISALIVE:
		return CWObject_Character::AI_IsAlive();

	case OBJMSG_AIQUERY_STEALTHMODE:
		{
			MSCOPESHORT(MESSAGEGROUP_AI);
			if (pCD && (pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_STEALTH))
				return 1;
			else 
				return 0;
		}

	case OBJMSG_AIQUERY_GETBODYANGLEZ:
		{
			MSCOPESHORT(MESSAGEGROUP_AI);
			fp32* pAngle = (fp32*)_Msg.m_pData;
			if (pCD)
			{
				*pAngle = pCD->m_Anim_BodyAngleZ;
				return 1;
			}
			else
				return 0;
		}


	case OBJMSG_AIQUERY_GETVULNERABLEPOS:
		{
			MSCOPESHORT(MESSAGEGROUP_AI);
			//Get position above bounding box center at height of face attachpoint 
			//or activate position if no face attachpoint can be found
			CVec3Dfp32 * pPos = (CVec3Dfp32 *)(_Msg.m_pData);
			
			CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
			CXR_Skeleton* pSkel;
			const CXR_SkeletonAttachPoint * pPoint;
			if (pModel &&
				(pSkel = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON)) &&
				(pPoint = pSkel->GetAttachPoint(PLAYER_ATTACHPOINT_FACE)))
			{
				*pPos = pPoint->m_LocalPos.GetRow(3);
				(*pPos)[0] = 0;
				(*pPos)[1] = 0;
				*pPos = *pPos + GetPosition();
			}
			else
			{
				//Couldn't get face attach point, use point at 3/4 of height
				if (pCD)
					*pPos = GetPosition() + CVec3Dfp32(0, 0, pCD->m_Phys_Height * 0.75f);
				else
					*pPos = GetPosition() + CVec3Dfp32(0, 0, 56 * 0.75f);
			}
			return 1;
		}


	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	case OBJMSG_RPG_AVAILABLEPICKUP:
		{
			return Char_AvailablePickup(_Msg);
		}

/*	case OBJMSG_RPG_GETITEMMATRIX:
		{
			if(!_Msg.m_pData)
				return 0;
			if(_Msg.m_DataSize != sizeof(CMat4Dfp32))
				return 0;

			CRPG_Object_Item *pItem = Char()->GetEquippedItem(_Msg.m_Param0);
			CXR_Model *pChar = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
			if(pItem && pChar)
			{
				CXR_AnimState Anim;
				if(OnGetAnimState(this, m_pWServer, GetPositionMatrix(), 0, Anim))
				{
					CXR_Skeleton *pSkel = (CXR_Skeleton*)pChar->GetParam(MODEL_PARAM_SKELETON);
					if(pItem->m_Model.IsValid() && pSkel)
					{
						CXR_Model *pModel = m_pWServer->GetMapData()->GetResource_Model(pItem->m_Model.m_iModel[0]);
						if(pModel)
						{
							CMat4Dfp32 *pMat = (CMat4Dfp32 *)_Msg.m_pData;
							if(CRPG_Object_Item::GetItemRenderMatrix(Anim.m_pSkeletonInst, pSkel, pModel, pItem->m_Model.m_iAttachRotTrack, pItem->m_Model.m_iAttachPoint[0], *pMat))
								return 1;
						}
					}
				}
			}
			return 0;
		}*/
	case OBJMSG_RPG_GETITEMMODEL:
		{
			CRPG_Object_Item *pItem = Char()->GetEquippedItem(_Msg.m_Param0);
			return pItem->m_Model.m_iModel[0];
		}
		
	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	case OBJSYSMSG_PRECACHEMESSAGE:
		{
			aint retval = OnCharPrecacheMessage(_Msg, pCD);
			if(retval)
				return retval;
			break;
		}

	case OBJSYSMSG_TELEPORT:
		{
			return OnCharTeleport(_Msg, pCD);
		}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case OBJMSG_TELEPORTCONTROLLER_TELEPORTOBJECT:
		{
			//Get object position to teleport to
			CWObject * pObj = m_pWServer->Object_Get(m_pWServer->Selection_GetSingleTarget((char *)_Msg.m_pData));
			if (pObj &&
				m_pWServer->Object_SetPosition(m_iObject, pObj->GetPositionMatrix()))
			{
				if (pCD)
				{
					//Force body angle to align with look as well...
					CVec3Dfp32 VFwd = CVec3Dfp32::GetMatrixRow(GetPositionMatrix(), 0);
					fp32 Heading = 1.0f - CVec3Dfp32::AngleFromVector(VFwd[0], VFwd[1]);
					pCD->m_Anim_BodyAngleZ = Heading;
				};

				return 1;
			}
			else
				return 0;
		}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case OBJMSG_GAME_FORCEENDTIMELEAP:
		{
			Char_EndTimeLeap();
			return 1;
		}

	case OBJMSG_GAME_NOTIFYCUTSCENE:
		{
			/*if(m_TimeoutTick != 0)
				Destroy();*/
			return 1;
		}

	case OBJMSG_GAME_SPAWN:
		if((m_Flags & PLAYER_FLAGS_WAITSPAWN)&&(_Msg.m_Param0 <= 0))
		{
			if (_Msg.m_Param0 == 0)
			{	// Try to find an eligible pos
				SpawnCharacter(m_InitialPhysMode,0,true);
			}
			else
			{	// Fail early if the pos is not available
				SpawnCharacter(m_InitialPhysMode,0,false);
			}
			return 1;
		}
		else if (!(m_Flags & PLAYER_FLAGS_WAITSPAWN)&&(_Msg.m_Param0 > 0))
		{
			UnspawnCharacter();
			if (m_spAI)
			{
				m_spAI->OnMessage(CWObject_Message(_Msg));
			}
			return 1;
		}
		return 0;
		
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case OBJMSG_IMPULSE:
		{
			//Old hardcoded test stuff
			//CWO_Character_ClientData *pCD = GetClientData(this);
			//if (pCD)
			//	if(pCD->m_iPlayer != -1)
			//	{
			//		CWObject_Message Msg(OBJMSG_ACTIONCUTSCENE_ACTIVATE);
			//		Msg.m_iSender = m_iObject;
			//		m_pWServer->Message_SendToTarget(Msg, "AC_Drill");
			//		return 1;
			//	}
			if(CStrBase::CompareNoCase("AI_WB_Sara_01", GetTemplateName()) == 0)
			{
				if(pCD)
					pCD->m_Anim_FreezeTick = pCD->m_GameTick;
			}

			return m_ImpulseContainer.OnImpulse(_Msg.m_Param0, m_iObject, _Msg.m_iSender, m_pWServer);
		}

	case OBJMSG_CHAR_RESPAWN:
		{
			if (!pCD)
				return 0;

			if(m_Param == 0)
				m_Param = Char()->MaxHealth();
			CWObject_Message Msg = _Msg;
			Msg.m_Param0 = pCD->m_iPlayer;
			Msg.m_iSender = m_iObject;
			// Might be changed in respawn message
			CWorld_Server* pWServer = m_pWServer;
			int iObj = pWServer->Message_SendToObject(Msg, pWServer->Game_GetObjectIndex());
			if(iObj > 0)
			{
				CWObject_Character *pChar = TDynamicCast<CWObject_Character>(pWServer->Object_Get(iObj));
				pCD = CWObject_Character::GetClientData(pChar);
				if(pChar && pCD)
				{
					int SkeletonType = pCD->m_Aim_SkeletonType;
					if ((SkeletonType == SKELETONTYPE_NORMAL || SkeletonType == SKELETONTYPE_BUTCHER) && m_Param > 0)
					{
						pChar->Char()->MaxHealth() = m_Param;
						pChar->Char()->Health() = m_Param;
						m_Param = 0;
					}
				}
				return iObj;
			}

			return 1;
		}
	case OBJMSG_PLAYER_GETSPECIALCLASS:
		{	
			if(m_Player.m_bSpecialClass)
				return (aint)GetTemplateName();
			else
				return 0;
		}

	case OBJMSG_PLAYER_GETPHYSTYPE:
		return Char_GetPhysType(this);

	case OBJMSG_CHAR_REQUESTMOUNT:
		{
			if (!pCD)
				return 0;

			pCD->m_iMountedObject = _Msg.m_iSender;
			pCD->m_MountedStartTick = _Msg.m_Param0;
			pCD->m_MountedMode = /*pCD->m_MountedMode |*/ (uint8)_Msg.m_Param1;
			return 1;
		}

	case OBJMSG_CHAR_FORCERELEASEMOUNT:
		{
			if (!pCD)
				return 0;

			pCD->m_iMountedObject = 0;
			return 1;
		}

	case OBJMSG_CHAR_MOUNT:
		return Char_Mount((const char *)_Msg.m_pData, _Msg.m_Param0);

	case OBJMSG_CHAR_UPDATEMAGAZINE:
		{
			if (!pCD)
				return 0;

			pCD->m_Pickup_iIcon = (uint16)_Msg.m_Param0;
			pCD->m_Pickup_Magazine_Num = (int8)(Max(-1, Min(127, (int)_Msg.m_Param1)));
			pCD->m_Pickup_StartTick = pCD->m_GameTick;
			Char_UpdateQuestItems();
			return 1;
		}

	case OBJMSG_CHAR_FORCEDROPWEAPON:
		{
			return OnCharForceDropWeapon(_Msg, pCD);
		}
	case OBJMSG_CHAR_CREATEGRABBABLEITEMOBJECT:
		{
			return OnCharCreateGrabbableItemObject(_Msg, pCD);
		}
	case OBJMSG_CHAR_GETGRABDIFFICULTY:
		{
			return m_GrabDifficulty;
		}

	case OBJMSG_CHAR_SETNIGHTVISION:
		{
/*			if(m_Flags & PLAYER_FLAGS_NONIGHTVISION && _Msg.m_Param0 == 1)
				m_Flags &= ~PLAYER_FLAGS_NONIGHTVISION;
			else if(!(m_Flags & PLAYER_FLAGS_NONIGHTVISION) && _Msg.m_Param0 == 0)
				m_Flags |= PLAYER_FLAGS_NONIGHTVISION;
			else
				return 0;*/
			if(_Msg.m_Param0)
				ConExecute("cl_darknessvision(1)");
			else
				ConExecute("cl_darknessvision(0)");
			return 1;
		}
	case OBJMSG_CHAR_PUSHRAGDOLL:
		{
#ifdef INCLUDE_OLD_RAGDOLL
			if (m_spRagdoll)
			{
				m_spRagdoll->AddImpulse(CConstraintSystem::BODY_ALL,_Msg.m_VecParam1);
				// m_spRagdoll->AddPendingImpulse(_Msg.m_VecParam0, _Msg.m_VecParam1);
				return 1;
			}
#endif // INCLUDE_OLD_RAGDOLL
			if( m_pPhysCluster )
			{
				m_pPhysCluster->AddForceToAll(m_pWServer,_Msg.m_VecParam1 * RAGDOLL_FORCE_MULTIPLIER);
				return 1;
			}
			return 0;
		}
	case OBJMSG_CHAR_SETRAGDOLL:
		{
			// Remove ragdoll if we have fully blended in...?
			CWO_Character_ClientData* pCD = GetClientData(this);
			if (!_Msg.m_Param0)
			{			
				//if (pCD->m_RagdollClient.m_State != CConstraintSystemClient::NOTREADY)
				if (pCD->m_RagdollClientData.m_State)
				{
					//m_spRagdoll->SetState(CConstraintSystemClient::NOTREADY);
					//pCD->m_RagdollClient.MakeDirty();
					pCD->m_ExtraFlags = pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_RAGDOLLACTIVE;
					pCD->m_Phys_Flags = pCD->m_Phys_Flags & 
						~(PLAYER_PHYSFLAGS_NOGRAVITY | PLAYER_PHYSFLAGS_NOCHARACTERCOLL | PLAYER_PHYSFLAGS_NOWORLDCOLL);
					// Remove ragdoll
					//m_spRagdoll = NULL;
					CleanPhysCluster();
					pCD->m_RagdollClientData.m_State = 0;
					pCD->m_RagdollClientData.MakeDirty();
				}
			}
			else if (_Msg.m_Param0 == 2)
			{
				// Add Phys flags again
				if (pCD->m_Phys_Flags & (PLAYER_PHYSFLAGS_NOGRAVITY | PLAYER_PHYSFLAGS_NOCHARACTERCOLL | PLAYER_PHYSFLAGS_NOWORLDCOLL))
				{
					ClientFlags() = m_ClientFlags & ~PLAYER_CLIENTFLAGS_NOGRAVITY;
					pCD->m_Phys_Flags = pCD->m_Phys_Flags & ~(PLAYER_PHYSFLAGS_NOGRAVITY | PLAYER_PHYSFLAGS_NOCHARACTERCOLL | PLAYER_PHYSFLAGS_NOWORLDCOLL);
					Char_SetPhysics(this,m_pWServer,m_pWServer,(Char_GetPhysType(this) == PLAYER_PHYS_CROUCH) ? PLAYER_PHYS_CROUCH : PLAYER_PHYS_STAND,false,true);
				}
			}
			else if (_Msg.m_Param0 == 3)
			{
				// Setposition according to how the ragdoll looks like atm (head in the right direction..)
				CXR_SkeletonInstance* pSkelInst = NULL;
				CXR_Skeleton* pSkel = NULL;
				CXR_AnimState AnimState;
				GetEvaluatedPhysSkeleton(pSkelInst,pSkel,AnimState);
				// Align to 1
				if (pSkelInst && pSkelInst->m_nBoneTransform > 1)
				{
					CMat4Dfp32 NewPos;
					NewPos.GetRow(2) = CVec3Dfp32(0.0f,0.0f,1.0f);
					NewPos.GetRow(0) = pSkelInst->m_pBoneTransform[1].GetRow(2);
					NewPos.RecreateMatrix(2,0);
					NewPos.UnitNot3x3();
					GetPosition().SetMatrixRow(NewPos,3);
					m_pWServer->Object_SetPosition(m_iObject,NewPos);
				}
			}
			else
			{
				Char_BeRagDoll(true);
			}
			return 1;
		}

	case OBJMSG_CHAR_SETNEXTGUISCREEN:
		{
			m_NextGUIScreen = _Msg.m_Param0;
			return 1;
		}

	case OBJSYSMSG_PLAYER_CANSAVE:
		{
			// Basically should be same as trying to enter dialog mode?
			if (!pCD || pCD->m_iMountedObject || pCD->m_iCreepingDark)
				return false;

			uint32 Fade = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETSCREENFADE), m_pWServer->Game_GetObjectIndex());
			return (((Fade >> 24) < 20) && Char_IsPlayerViewControlled(this) && (m_PendingCutsceneTick == -1) && !m_bDisableQuicksave);
		}

	case OBJMSG_CHAR_SETDROPITEM:
		{
			if(_Msg.m_pData == NULL)
				m_spDropItem = NULL;
			else
				m_spDropItem = CRPG_Object::CreateObject((char *)_Msg.m_pData, m_pWServer);
			return 1;
		}
	case OBJMSG_CHAR_SETCAMERAEFFECTS:
		{
			return OnCharSetCameraEffects(_Msg, pCD);
		}
	case OBJMSG_CHAR_DISABLEQUICKSAVE:
		{
			m_bDisableQuicksave = _Msg.m_Param0 != 0;
			return 1;
		}

	case OBJMSG_CHAR_CHAIN:
		{
			if(m_iChainObj)
			{
	//			CWObject_ChainEffect* pChain = safe_cast<CWObject_ChainEffect>(m_pWServer->Object_Get(m_iChainObj));
	//			CWO_ChainEffect_ClientData& ChainCD = pChain->GetClientData(pChain);
				
				if(_Msg.m_Param0 == 0)
				{
					// Send an expand message to attached chain
					CWObject_Message Msg(OBJMSG_CHAIN_EXPAND, 1);
					m_pWServer->Message_SendToObject(Msg, m_iChainObj);
				}
				else if(_Msg.m_Param0 == 1)
				{
					// Send an shrink message to attached chain
					CWObject_Message Msg(OBJMSG_CHAIN_SHRINK, 1);
					m_pWServer->Message_SendToObject(Msg, m_iChainObj);
				}
				else if(_Msg.m_Param0 == 2)
				{
					CVec3Dfp32 Dir = CVec3Dfp32::GetMatrixRow(GetPositionMatrix(), 2);

					CWObject_Message Msg(OBJMSG_CHAIN_IMPULSE, -1, aint(&Dir));
					m_pWServer->Message_SendToObject(Msg, m_iChainObj);
				}
				else
				{
					//CVec3Dfp32 Dir = CVec3Dfp32::GetMatrixRow(GetPositionMatrix(), 2);
					CVec3Dfp32 Origin = CVec3Dfp32(0);

					CWObject_Message Msg(OBJMSG_CHAIN_LENGTHTO, 0, aint(&Origin));
					m_pWServer->Message_SendToObject(Msg, m_iChainObj);
				}

				// Activate or deactive chain effect,
	//			if(_Msg.m_Param0 != 0)
	//				ChainCD.m_Flags = (ChainCD.m_Flags | CHAIN_STATE_ROLLOUT | CHAIN_STATE_ACTIVE) & ~(CHAIN_STATE_ROLLIN);
	//			else
	//				ChainCD.m_Flags = (ChainCD.m_Flags | CHAIN_STATE_ROLLIN) & ~(CHAIN_STATE_ROLLOUT | CHAIN_STATE_ACTIVE);
			}

			return 1;
		}

	case CWObject_InputEntity::OBJECT_INPUTENTITY_MESSAGE_ATTACHME:
		{
			if (_Msg.m_Param0)
			{
				// Add to list of recipients
				bool bOk = true;
				for (int32 i = 0; i < m_liInputEntities.Len(); i++)
				{
					if (m_liInputEntities[i] == _Msg.m_iSender)
					{
						bOk = false;
						break;
					}
				}
				if (bOk)
					m_liInputEntities.Add(_Msg.m_iSender);
			}
			else
			{
				// Remove from list of recipients
				for (int32 i = 0; i < m_liInputEntities.Len(); i++)
				{
					if (m_liInputEntities[i] == _Msg.m_iSender)
					{
						m_liInputEntities.Del(i);
						break;
					}
				}
			}
			return 1;
		}
	case CWObject_AnimEventListener::OBJECT_ANIMEVENTLISTENER_MESSAGE_ATTACHME:
		{
			if (_Msg.m_Param0)
			{
				// Add to list of recipients
				bool bOk = true;
				for (int32 i = 0; i < m_liAnimEventEntities.Len(); i++)
				{
					if (m_liAnimEventEntities[i] == _Msg.m_iSender)
					{
						bOk = false;
						break;
					}
				}
				if (bOk)
					m_liAnimEventEntities.Add(_Msg.m_iSender);
			}
			else
			{
				// Remove from list of recipients
				for (int32 i = 0; i < m_liAnimEventEntities.Len(); i++)
				{
					if (m_liAnimEventEntities[i] == _Msg.m_iSender)
					{
						m_liAnimEventEntities.Del(i);
						break;
					}
				}
			}
			return 1;
		}
	case OBJMSG_OBJECT_ISBREAKING:
		{
			// Are we grabbing the destroyed object?
			if (_Msg.m_iSender == m_GrabbedObject.m_iTargetObj)
			{
/* -- not done yet --
				m_GrabbedObject.Release();

				uint nNewObjects = _Msg.m_Param0;
				const uint16* piNewObjects = (const uint16*)_Msg.m_Param1;

				CVec3Dfp32 CurrPos = m_GrabbedObject.m_LastPos;
				uint iBestObj = 0;
				fp32 Best = 1e10f;
				for (uint i = 0; i < nNewObjects; i++)
				{
					int iNewObj = piNewObjects[i];
					CWObject* pObj = m_pWServer->Object_Get(iNewObj);
					CVec3Dfp32 ObjCenter;
					pObj->GetAbsBoundBox()->GetCenter(ObjCenter);
					fp32 d = CurrPos.DistanceSqr(ObjCenter);
					iBestObj = (d < Best) ? iNewObj : iBestObj;
					Best = Min(d, Best);
				}
				if (iBestObj)
					m_GrabbedObject.Init(*m_pWServer, iBestObj);
*/
			}
			else if (pCD->m_iDarkness_Tentacles)
			{
				// Well, perhaps the demon arm is grabbing the destroyed object?
				return m_pWServer->Message_SendToObject(_Msg, pCD->m_iDarkness_Tentacles);
			}
			// wtf?
			return 0;
		}
	case OBJMSG_CHAR_SETHUGINCUTSCENEFADE:
		{
			pCD->m_DemonHeadTargetFade = _Msg.m_Param0;
			return 1;
		}
	case OBJMSG_CHAR_GETDEMONARMREACH:
		{
			if(pCD && pCD->m_DarknessPowersAvailable)
			{
				return m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETDEMONARMREACH), pCD->m_iDarkness_Tentacles);
			}
			else
			{
				return 0;
			}
		}
	case OBJMSG_CHAR_GETDARKNESSPOSITION:
		{
			if(_Msg.m_DataSize == sizeof(CVec3Dfp32))
			{
				CVec3Dfp32 * pPos = (CVec3Dfp32 *)_Msg.m_pData;
				*pPos = Char_GetDarknessPosition();
				return 1;
			}
			return 0;
		}
	case OBJMSG_CHAR_GETHITLOCATION:
		{
			if (_Msg.m_pData && _Msg.m_DataSize == sizeof(CCollisionInfo))
			{
				CCollisionInfo* pCInfo = (CCollisionInfo*)_Msg.m_pData;
				return GetHitlocation(pCInfo);
			}
			return 0;
		}
	case OBJMSG_CHAR_CRITICALHEADSHOT:
		{
			// Blow head off. (disabled)
			CWO_Character_ClientData* pCD = GetClientData(this);
			bool bGib = (_Msg.m_Param0 || Char()->Health().m_Value <= 0);
			if (false && bGib && pCD && pCD->m_HeadGibbed == 0)
			{
				// Try not to be to violent
				MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
				int bBlood = (pSys && pSys->GetOptions()) ? pSys->GetOptions()->GetValuei("GAME_BLOOD", 1) : 1;
				if (!bBlood)
					return 0;

				pCD->m_HeadGibbed = m_pWServer->GetMapData()->GetResourceIndex_Model("Characters\\sever\\sever");
				Char_CreateNeckBlood();
				return 1;
			}
			return 0;
		}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// THESE MESSAGES DON'T HAVE ANY RELATIONS IN THE CODE
	//


	/*
	case OBJMSG_CHAR_GETANIMFROMHANDLE:
		{
			return int(GetAnimSequence(_Msg.m_Param0));
		
		}
		*/

	/*
	case OBJMSG_SPELLS_GETACTIVATEPOSITION: 
		{
			if (_Msg.m_DataSize == sizeof(CMat4Dfp32) && _Msg.m_pData)
			{
				GetActivatePosition(*(CMat4Dfp32*)_Msg.m_pData);
				return true;
			}
			else
				return false;
		}
		*/

	//case OBJMSG_CHAR_STUN:
	//	return Char_Stun(_Msg.m_Param0);

	/*
	case OBJMSG_OBJANIM_TRIGGER:
		{
			m_iForceAnim = _Msg.m_Param0;
			return 1;
		}
		*/

		/*
	case OBJMSG_CHAR_ISEQUIPPED:
		{
			const CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
			if (pCD)
			{
				if(_Msg.m_Param0 == 0)
					return (pCD->m_Item0_Flags & RPG_ITEM_FLAGS_EQUIPPED) != 0;
				else if(_Msg.m_Param1 == 0)
					return (pCD->m_Item1_Flags & RPG_ITEM_FLAGS_EQUIPPED) != 0;
			}
			return 0;
		}
		*/

	//case OBJMSG_CHAR_SETCUTSCENECAMERA:
	//	return Char_SetCutsceneCamera(_Msg.m_Param0);

	// case OBJMSG_CHAR_ENDDIALOGUE:
	//	return Char_EndDialogue();
	/*
	case OBJMSG_CHAR_SETTIMEOUT:
		{
			if (_Msg.m_Param0 > 0)
			{
				CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
				if (pCD)
				{
					m_TimeoutTick = pCD->m_GameTick + _Msg.m_Param0;
					return 1;
				}
			}
		}
		return 0;
		*/

	/*
	case OBJMSG_CHAR_SETZOOM:
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if (pCD)
			{
				pCD->m_WantedZoomSpeed = ((fp32)_Msg.m_Param0 / 65536.0f);
				pCD->m_MaxZoom = ((fp32)_Msg.m_Param1 / 65536.0f);
				return 1;
			}
			else
				return 0;
		}*/

		/*
	case OBJMSG_CHAR_ADJUSTMOVEMENT:
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if (pCD)
			{
				pCD->m_ControlMove_Offset = ((fp32)_Msg.m_Param0 / 65536.0f);
				pCD->m_ControlMove_Scale = ((fp32)_Msg.m_Param1 / 65536.0f);
				return 1;
			}
			else
				return 0;
		}*/

	//case OBJMSG_CHAR_SETTILT:
	//	{
	//		return 0;
	//	}
		/*
	case OBJMSG_CHAR_AIRCRAFT:
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if(pCD)
			{
				if (_Msg.m_Param0 == 1)
				{
					pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_AIRCRAFT;
					m_ClientFlags = m_ClientFlags | PLAYER_CLIENTFLAGS_NOGRAVITY;
					m_bWeightless = true;
				}
				else
				{
					pCD->m_ExtraFlags = pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_AIRCRAFT;
					m_ClientFlags = m_ClientFlags & ~PLAYER_CLIENTFLAGS_NOGRAVITY;
					m_bWeightless = false;
				}
					
				return 1;
			}
			return 0;
		}*/
		/*
	case OBJMSG_CHAR_SWITCH_CHAR: // script
		{
			//Forward message to game with appropriate modifications
			CWObject_Message Msg = _Msg;
			Msg.m_Msg = OBJMSG_GAME_SWITCH_CHAR;
			Msg.m_Param0 = m_pWServer->Game_GetObject()->Player_GetWithObject(m_iObject)->m_iPlayer;
			Msg.m_Param1 = _Msg.m_Param0;
			return m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());
		};*/
		/*
	case OBJMSG_CHAR_DAMAGEFACTOR:
		{
			//Set damage tolerance for the given hit location (param0) and damage type (param1) 
			//to the given value (fp32 pointer in data)
			SetDamageFactor(0,_Msg.m_Param0, _Msg.m_Param1, *((fp32*)_Msg.m_pData));
			return 1;
		};*/
		/*
	case OBJMSG_CHAR_SETBACKPLANE:
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if(pCD)
				pCD->m_BackPlane = (fp32)_Msg.m_Param0;
			return 1;
		}*/
		
	/*case OBJMSG_CHAR_GETDEFENSE:
		{
			CCollisionInfo *pInfo = (CCollisionInfo *)_Msg.m_Param0;
			return GetDamageFactor(pInfo->m_SurfaceType, 0) * 100 + 1;
		}*/

		/*
	case OBJMSG_CHAR_FREEZEANIM:
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if (pCD)
				pCD->m_Anim_FreezeTick = pCD->m_GameTick;
			return 1;
		}*/

		/*
	case OBJMSG_CHAR_UNFREEZEANIM:
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if (pCD)
				pCD->m_Anim_FreezeTick = 0;
			return 1;
		}*/
	/*
	case OBJMSG_CHAR_SETDEATHANIM:
		m_iForceDeathAnim = _Msg.m_Param0;
		return 1;
		*/
/*
	case OBJMSG_CHAR_FIGHTSETDIRECTION:
		{
			CWO_Character_ClientData *pCD = GetClientData(this);

			//pCD->m_FightingCharPos = _Msg.m_VecParam0;
			//pCD->m_BaseFightDirection = CVec3Dfp32::GetMatrixRow(GetPositionMatrix(),0);
			//pCD->m_FightDirTime = m_pWServer->GetGameTick() + 1;
			pCD->m_FightPosTime = m_pWServer->GetGameTick() + 1;
			CMat4Dfp32 PosMat = GetPositionMatrix();
			_Msg.m_VecParam0.SetMatrixRow(PosMat,0);
			SetPosition(PosMat);
			
			return 1;
		}*/
		/*
	case OBJMSG_CHAR_APPLYDAMAGESIMPLE:
		{
			return Char_ApplyDamageSimple(_Msg.m_Param0, _Msg.m_Param1, _Msg.m_iSender, _Msg.m_Reason);
		}
		*/
		/*
	case OBJMSG_CHAR_SETDODGETIME:
		{
			CWO_Character_ClientData *pCD = GetClientData(this);
		
			if (pCD)
			{
				pCD->m_DodgeTime = *(fp32*)&_Msg.m_Param0;
				
				return 1;
			}
			
			return 0;
		}*/
		/*
	case OBJMSG_CHAR_FIGHTINGEACHOTHER:
		{
			CWO_Character_ClientData *pCD = GetClientData(this);

			if (pCD)
			{
				// Check if we are fighting with the one sending the message
				int iFightingChar = pCD->m_iFightingCharacter;
				
				return (iFightingChar == _Msg.m_iSender);
			}
			return false;
		}*/
		/*
	case OBJMSG_CHAR_SETFIGHTMODEFLAG:
		{
			CWO_Character_ClientData *pCD = GetClientData(this);

			if (pCD)
			{
				// Check if we are fighting with the one sending the message
				pCD->m_FightModeFlag = (uint8)_Msg.m_Param0;
				
				return 1;
			}
			return false;
		}*/
		/*
	case OBJMSG_CHAR_GETWANTEDBEHAVIOR:
		{
			CWO_Character_ClientData *pCD = GetClientData(this);
			
			return (pCD ? pCD->GetWantedBehavior() : false);
		}*/
		/*
	case OBJMSG_CHAR_SETWANTEDBEHAVIOR:
		{
			CWO_Character_ClientData *pCD = GetClientData(this);
			
			if (pCD)
			{
				pCD->SetWantedBehavior(_Msg.m_Param0);
				return true;
			}

			return false;
		}*/
/*
	case OBJMSG_CHAR_SETCONTROLMODE:
		{
			Char_SetControlMode(this, _Msg.m_Param0);

			return true;
		}
		*/
/*
	case OBJMSG_CHAR_FIGHTDEADAWHILE:
		{
			// Fist check so that the character actually is dead....
			// Then check that the death animation has played for a while.....
			CWO_Character_ClientData *pCD = GetClientData(this);

			if(pCD && (Char_GetPhysType(this) == PLAYER_PHYS_DEAD))
			{
				fp32 Time = fp32(pCD->m_GameTick - m_AnimTime) * SERVER_TIMEPERFRAME;
				
				if (Time > 3.0f)
				{
					return true;
				}
			}

			return false;
		}*/

		/*
	case OBJMSG_AIEFFECT_SETMOVEMODE:
		{
			CWO_Character_ClientData * pCD = CWObject_Character::GetClientData(this);
			if (pCD)
				pCD->SetAIMoveMode(_Msg.m_Param0);
			return 1;
		}*/

	//
	// 
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// MESSAGES THAT SHOULD BE REMOVED! 
	//
	case OBJMSG_CHAR_ISBOT:
		return IsBot();

	case OBJMSG_AIQUERY_GETACTIVATEPOSITION:
		CWObject_Character::AI_GetActivateMat(*(CMat4Dfp32*)_Msg.m_pData);
		return 1;

	case OBJMSG_AIQUERY_GETAI:
		*(CAI_Core**)(_Msg.m_pData) = CWObject_Character::AI_GetAI();
		return (*(CAI_Core**)(_Msg.m_pData) != NULL);

	case OBJMSG_AIQUERY_GETMAXSPEED:
		*(fp32*)_Msg.m_pData = CWObject_Character::AI_GetMaxSpeed(_Msg.m_Param0);
		return 1;

	case OBJMSG_AIQUERY_GETRPGITEM:
		*(CRPG_Object_Item**)(_Msg.m_pData) = CWObject_Character::AI_GetRPGItem(_Msg.m_Param0);
		return (*(CRPG_Object_Item **)(_Msg.m_pData) != NULL);

	case OBJMSG_AIQUERY_GETRPGINVENTORY:
		*(CRPG_Object_Inventory**)(_Msg.m_pData) = CWObject_Character::AI_GetRPGInventory(_Msg.m_Param0);
		return (*(CRPG_Object_Inventory**)(_Msg.m_pData) != NULL);

	//
	// 
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef M_RTM
	case OBJSYSMSG_GETDEBUGSTRING:
		if(_Msg.m_DataSize == sizeof(CStr *))
		{
			CStr *pSt = (CStr *)_Msg.m_pData;
			switch(_Msg.m_Param0)
			{
			case 0:
				// Standard info
				CWObject_RPG::OnMessage(_Msg);
				if(m_Flags & PLAYER_FLAGS_WAITSPAWN)
					*pSt += "Waiting for spawn";
				else if(Char_GetPhysType(this) == PLAYER_PHYS_DEAD)
					*pSt += "Dead";
				else
					*pSt += m_spAI->GetDebugString();
				return 1;
			
			case 1:
				if(pCD && pCD->m_iPlayer != -1)
				{
					*pSt = CStrF("Health: %i|Noice: %i|Visibility: %i", int(pCD->m_Health), m_RaisedNoiseLevel, m_RaisedVisibility);
					return 1;
				}
				return 0;
			}

			return 0;
		}
#endif
	}

	return CWObject_RPG::OnMessage(_Msg);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
aint CWObject_Character::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
//Part of Sammes message testing system
//	AddMessage(_Msg, false);

	MAUTOSTRIP(CWObject_Character_OnClientMessage, 0);
	/* The messages are now orderd so that the most freqeunt are placed first in the case-switch. When putting a new message into
	the system. A bit down a end comment is put. After that the messages can be in any order. */
	switch(_Msg.m_Msg)
	{
	case OBJMSG_CHAR_GETAUTOAIMOFFSET:
		{
			CWO_Character_ClientData* pCD = GetClientData(_pObj);
			if(!pCD)
				return 0;

			CVec3Dfp32 Offset = 0.0f;
			Offset.k[2] = (_pObj->GetAbsBoundBox()->m_Max.k[2] - _pObj->GetAbsBoundBox()->m_Min.k[2]) * pCD->m_ThisAimHeight;
			return Offset.Pack32(256.0f);
		}
	case OBJMSG_CHAR_GETNVINTENSITY:
		{
			CWO_Character_ClientData* pCD = GetClientData(_pObj);
			if(!pCD)
				return 0;
			if (!_Msg.m_pData)
				return 0;

			CWO_CameraEffects& NVInfo = *(CWO_CameraEffects*)_Msg.m_pData;

			fp32 PredictFrameFrac = pCD->m_PredictFrameFrac;
			fp32 GameTickTime = _pWClient->GetGameTickTime();
			CMTime GameTime = CMTime::CreateFromTicks(pCD->m_GameTick, GameTickTime, PredictFrameFrac);
			{
				if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSVISION)
				{
					NVInfo.m_Nightvision = 1.0f;
					NVInfo.m_LightIntensity = 1.0f;
				}
				else
				{
					NVInfo.m_Nightvision = 0.0f;
					NVInfo.m_LightIntensity = 0.0f;
				}

				if(!pCD->m_ScreenFlashGameTime.IsReset())
				{
					fp32 Duration = (GameTime - pCD->m_ScreenFlashGameTime).GetTime();
					if(Duration < 0.5)
					{
						int Alpha = RoundToInt((0.5 - Duration) * 255.0f);
						NVInfo.m_AlphaBlend = CPixel32(128, 0, 0, Alpha);
					}
				}
			}

			if(Char_IsPlayerView(_pObj))
			{
			//	NVInfo.m_Sneak = fp32(pCD->m_SneakLevel.Get(pCD->m_GameTick, pCD->m_PredictFrameFrac)) * ( 1.0f / 255 );
				NVInfo.m_Crouch = fp32(pCD->m_CrouchLevel.Get(pCD->m_GameTick, pCD->m_PredictFrameFrac)) * ( 1.0f / 255 );
			}
			NVInfo.m_Tension = fp32(pCD->m_Tension) / 255;

			NVInfo.m_DOF_Amount = fp32(pCD->m_DOF_Amount) / 255;
			NVInfo.m_DOF_Distance = pCD->m_DOF_Distance;
			if(pCD->m_DOF_Object != 0)
			{
				CMat4Dfp32 Cam, VMat;
				_pWClient->Render_GetLastRenderCamera(Cam);
				CVec3Dfp32 Pos = _pWClient->Object_GetPosition(pCD->m_DOF_Object);
				CWObject_Client *pObj = _pWClient->Object_Get(pCD->m_DOF_Object);
				if(pObj)
					pObj->GetAbsBoundBox()->GetCenter(Pos);

				Cam.InverseOrthogonal(VMat);
				Pos *= VMat;
				
				NVInfo.m_DOF_Distance = Pos[2];
				//ConOut(CStrF("%f", Pos[2]));
			}
			NVInfo.m_MotionBlur_Amount = fp32(pCD->m_MotionBlur_Amount) / 255;
			NVInfo.m_MotionBlur_Fuzzyness = fp32(pCD->m_MotionBlur_Fuzzyness) / 255;
			NVInfo.m_GlowMotionBlur = fp32(pCD->m_GlowMotionBlur) / 255;

			// Hit effect defaults
			bool bDarknessShield = (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD) != 0;

			// Custom radial blur filters
			static const char* s_pRadialBlurFilter0 = "XREngine_RadialBlurHurt";
			static const char* s_pRadialBlurFilter1 = "XREngine_RadialBlurInvert";
			NVInfo.m_pRadialBlurFilter = NULL;

#ifndef M_RTM
			// Hurt effect testing/tweaking
			int iHurtEffect = 1;
			MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
			if (pSys)
				iHurtEffect = pSys->GetEnvironment()->GetValuei("CL_HURTEFFECT", 1);
			if (iHurtEffect == 2)
			{
				NVInfo.m_pRadialBlurFilter = s_pRadialBlurFilter0;

				NVInfo.m_RadialBlur = 1.0f;
				NVInfo.m_RadialBlurColorIntensity = 0.0f;
				NVInfo.m_RadialBlurAffection = 0.3f;
				NVInfo.m_RadialBlurColorScale = 1.0f;
				NVInfo.m_RadialBlurCenter = 0.5f;

				NVInfo.m_UVExtra = CVec2Dfp32(0.5f, 0.0f);
				NVInfo.AddRadialBlurFilterParam(CVec4Dfp32(0.002f, 0.9f, 10.0f, 1.0f));
			}
			else
#endif
			{
				// Fetch darkness voice amount
				fp32 DarknessVoice = Clamp01(pCD->m_DarknessVoiceEffect - ((!pCD->m_DarknessVoiceUse.m_Value) ? (GameTickTime * PredictFrameFrac) : 0.0f));
				if (!AlmostEqual(DarknessVoice, 0.0f, 0.001f))
				{
					// Run darkness voice effect
					NVInfo.m_pRadialBlurFilter = s_pRadialBlurFilter1;
					NVInfo.m_RadialBlurColorIntensity = CFXSysUtil::LerpMT(NVInfo.m_RadialBlurColorIntensity, 0.0f, DarknessVoice);
					NVInfo.m_RadialBlurColorScale.Lerp(CVec3Dfp32(1.0f), DarknessVoice, NVInfo.m_RadialBlurColorScale);
					NVInfo.m_RadialBlurAffection.Lerp(CVec3Dfp32(0.6f), DarknessVoice, NVInfo.m_RadialBlurAffection);
					NVInfo.m_RadialBlurMode = CRC_TEXENVMODE_MULTIPLY;
					NVInfo.m_RadialBlur = DarknessVoice;
					if(pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_OTHERWORLD)
						NVInfo.m_RadialBlur *= 0.75f;
				}
				else
				{
					const fp32 Hurt = (fp32)LERP(pCD->m_HitEffect_LastTime, pCD->m_HitEffect_Time, PredictFrameFrac);
					const bool bHurt = (Hurt > 0.0001f);
					const uint HurtFlags = pCD->m_HitEffect_Flags.m_Value;
					const bool bDarknessShield = (pCD->m_DarknessSelectionMode.m_Value & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD) != 0;
					if (bHurt && HurtFlags & PLAYER_HITEFFECT_FLAGS_STREAK)
					{
						// Switch radial blur filter
						NVInfo.m_pRadialBlurFilter = s_pRadialBlurFilter0;
						NVInfo.m_RadialBlurMode = CRC_TEXENVMODE_COMBINE_ADD;
						
						// Setup the 'No risk of dying' values
						fp32 Amount = 1.0f;
						fp32 ColorIntensity = 0.8f;
						CVec3Dfp32 ColorScale(1.1f, 0.95f, 0.95f);
						CVec3Dfp32 Affection(0.25f);
						CVec2Dfp32 UVCenter(0.5f);

						// Switch setting depening on health left
						if (bDarknessShield)
						{
							// Darkness shield active, interpolate seperately
							Amount = 0.4f;
							ColorIntensity = 0.0f;
							Affection = 0.3f;
							CVec3Dfp32(1,1,1.5f).Lerp(CVec3Dfp32(1,1,1.25f), fp32(pCD->m_Darkness.m_Value) / fp32(pCD->m_MaxDarkness.m_Value), ColorScale);
							NVInfo.m_UVExtra = CVec2Dfp32(0.5f, 0.0f);
							NVInfo.AddRadialBlurFilterParam(CVec4Dfp32(0.01f, 0.9f, 10.0f, 0.0f));
						}
						else
						{
							// The lower health we have the more intense the values will become
							fp32 HealthLerp = fp32(pCD->m_Health.m_Value) / fp32(pCD->m_MaxHealth.m_Value);

							ColorIntensity = CFXSysUtil::LerpMT(0.0f, ColorIntensity, HealthLerp);
							CVec3Dfp32(1.6f, 0.6f, 0.6f).Lerp(ColorScale, HealthLerp, ColorScale);
							CVec3Dfp32(0.6f, 0.6f, 0.6f).Lerp(Affection, HealthLerp, Affection);
							NVInfo.m_UVExtra = CVec2Dfp32(0.5f, 0.0f);
							NVInfo.AddRadialBlurFilterParam(CVec4Dfp32(CFXSysUtil::LerpMT(0.0025f, 0.1f, HealthLerp), 0.9f, 10.0f, 0.0f));
						}

						CWObject_CoreData* pObj = _pWClient->Object_Get(pCD->m_HitEffect_iObject);
						if(pObj)
						{
							// Get object position, and if it's a character, fetch the mouth position
							CVec3Dfp32 Pos = pObj->GetPosition();
							if (CWObject_Character::IsCharacter(pObj))
								Pos = CWObject_Character::Char_GetMouthPos(pObj);

							// Get VMat and viewport
							CRC_Viewport VP;
							CMat4Dfp32 Cam, VMat;
							_pWClient->Render_GetLastRenderCamera(Cam);
							_pWClient->Render_GetLastRenderViewport(VP);
							Cam.InverseOrthogonal(VMat);
							
							// Calculate uv center for the object
							UVCenter = CXR_Util::VBM_Convert3DTo2DPosition(&VP, Pos, VMat, true);
							UVCenter.k[0] = -0.25f + (Clamp01(UVCenter.k[0]) * 1.5f);
							UVCenter.k[1] = 1.0f - Clamp01(UVCenter.k[1]);

							CVec3Dfp32 Dir = (Pos - Cam.GetRow(3)).Normalize();
							fp32 Dot = Dir * Cam.GetRow(2);
							if (Dot < 0.4f)
							{
								UVCenter.k[1] = 0.5f;
								if (Dot >= 0.2f)
								{
									fp32 p = (Dot - 0.2f) * 5.0f;
									UVCenter.k[1] = 1.0f - (p * UVCenter.k[1] + (1.0f - p) * 0.5f);
								}

								//Target is outside screen
								if(Dir * (-Cam.GetRow(0)) < 0.0f)
								{
									UVCenter.k[0] = 1.25f;
									if(Dot <= -0.9f)
									{
										fp32 p = (1.0f + Dot) * 10.0f;
										UVCenter.k[0] = 1.25f * p;
									}
								}
								else
								{
									UVCenter.k[0] = -0.25f;
									if(Dot <= -0.9f)
									{
										fp32 p = (1.0f + Dot) * 10.0f;
										UVCenter.k[0] = -0.25f * p;
									}
								}
							}

							//UVCenter.k[0] = Clamp01(UVCenter.k[0]) - Clamp01(UVCenter.k[0] - 1.0f);
							//UVCenter.k[1] = 1.0f - Clamp01(UVCenter.k[1]) - Clamp01(UVCenter.k[1] - 1.0f);
						}

						// Interpolate values
						NVInfo.m_RadialBlur = Hurt * Amount;
						NVInfo.m_RadialBlurColorIntensity = CFXSysUtil::LerpMT(NVInfo.m_RadialBlurColorIntensity, 0.0f, Hurt); //(fp32)LERP(0.0f, NVInfo.m_RadialBlurColorIntensity, Hurt);
						NVInfo.m_RadialBlurAffection.Lerp(Affection, Hurt, NVInfo.m_RadialBlurAffection);
						NVInfo.m_RadialBlurColorScale.Lerp(ColorScale, Hurt, NVInfo.m_RadialBlurColorScale);
						NVInfo.m_RadialBlurCenter.Lerp(UVCenter, Hurt, NVInfo.m_RadialBlurCenter);
					}
					else if(pCD->m_WeaponZoomState != WEAPON_ZOOMSTATE_OFF)
					{
						// weapon focus-zoom
						fp32 ZoomMod = 0.0f;
						fp32 ZoomDuration;
						int32 Diff;
						switch(pCD->m_WeaponZoomState)
						{
						case WEAPON_ZOOMSTATE_ON:
							ZoomMod = 1.0f;
							break;

						case WEAPON_ZOOMSTATE_IN:
							Diff = _pWClient->GetGameTick() - pCD->m_WeaponZoomStartTick;
							ZoomDuration = 0.1f * _pWClient->GetGameTicksPerSecond();
							ZoomMod = ((fp32)Diff / ZoomDuration);
							ZoomMod = Min(ZoomMod, 1.0f);
							ZoomMod = Max(ZoomMod, 0.0f);
							ZoomMod = M_Sin((ZoomMod*_PI*0.25f));
							break;

						case WEAPON_ZOOMSTATE_OFF:
							break;

						case WEAPON_ZOOMSTATE_OUT:
							Diff = _pWClient->GetGameTick() - pCD->m_WeaponZoomStartTick;
							ZoomDuration = 0.1f * _pWClient->GetGameTicksPerSecond();
							ZoomMod = ((fp32)Diff / ZoomDuration);
							ZoomMod = Min(ZoomMod, 1.0f);
							ZoomMod = Max(ZoomMod, 0.0f);
							ZoomMod = M_Sin(ZoomMod*_PI*0.25f);
							ZoomMod = 1.0f-ZoomMod;
							break;

						default:
							break;
						}

						
						NVInfo.m_pRadialBlurFilter = s_pRadialBlurFilter0;

						NVInfo.m_RadialBlur = ZoomMod * 0.2f;
						NVInfo.m_RadialBlurMode = CRC_TEXENVMODE_COMBINE_ADD;
						NVInfo.m_RadialBlurColorIntensity = 0.75f;
						NVInfo.m_RadialBlurAffection = 1.0f;
						NVInfo.m_RadialBlurColorScale = 1.0f * ZoomMod;
						NVInfo.AddRadialBlurFilterParam(CVec4Dfp32(0.53f * ZoomMod, 0.9f, 10.0f, 0.0f));
					}
				}
			}

			// Set nv params
			NVInfo.m_OtherWorld = (pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_OTHERWORLD) ? 1.0f : 0.0f;
			NVInfo.m_CreepingDark = LERP(pCD->m_CreepingLensLast, pCD->m_CreepingLens, PredictFrameFrac);
			NVInfo.m_Nightvision = MaxMT(NVInfo.m_CreepingDark, NVInfo.m_Nightvision);

			return 1;
		}

	case OBJMSG_CHAR_POSTPROCESSOVERRIDE:
		{
			CWO_Character_ClientData* pCD = GetClientData(_pObj);
			if(!pCD || !_Msg.m_pData)
				return 0;

			CXR_Engine_PostProcessParams *M_RESTRICT pPPP = (CXR_Engine_PostProcessParams*)_Msg.m_pData;
			if(pCD->m_Disorientation_Flash > 0)
			{
				const fp32 Flash = pCD->m_Disorientation_Flash;
				
				// Set predefined parameters for glow override.
				const CVec4Dfp32 GlowBias(-0.1, -0.1, -0.1, 0.0f);
				const CVec3Dfp32 GlowScale(25, 25, 25);
				const CVec3Dfp32 GlowGamma(1, 1, 1);

				// Setup overriden parameters
				pPPP->m_GlowAttenuationExp = LERP(pPPP->m_GlowAttenuationExp, 2, Flash);
				pPPP->m_GlowBias.Lerp(GlowBias, Flash, pPPP->m_GlowBias);
				pPPP->m_GlowScale.Lerp(GlowScale, Flash, pPPP->m_GlowScale);
				pPPP->m_GlowGamma.Lerp(GlowGamma, Flash, pPPP->m_GlowGamma);
			}

			// Switch glow filter if necessary...
			static const char* s_pGlowFilter = "XREngine_GaussClampedHurt";
			pPPP->m_pGlowFilter = NULL;

#ifndef M_RTM
			// Testing
			int iHurtEffect = 1;
			MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
			if (pSys)
				iHurtEffect = pSys->GetEnvironment()->GetValuei("CL_HURTEFFECT", 1);
			if (iHurtEffect == 2)
			{
				pPPP->m_pGlowFilter = s_pGlowFilter;
				pPPP->m_GlowAttenuationExp = 2.0f;
				pPPP->m_GlowBias = CVec4Dfp32(-0.1f, -0.1f, -0.1f, 0.00125f);
				pPPP->m_GlowScale = 4.0f;
				pPPP->m_GlowGamma = 1.0f;
			}
			else
#endif
			{
				const fp32 Hurt = (fp32)LERP(pCD->m_HitEffect_LastTime, pCD->m_HitEffect_Time, pCD->m_PredictFrameFrac);
				const bool bHurtEffect = (Hurt > 0.0001f);
				const uint8 HitEffectFlags = pCD->m_HitEffect_Flags.m_Value;
				const bool bDarknessShield = (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD) != 0;
				if (bHurtEffect && HitEffectFlags & PLAYER_HITEFFECT_FLAGS_BLUR)
				{
					// Switch glow filter
					pPPP->m_pGlowFilter = s_pGlowFilter;

					// Setup the No risk of dying range
					fp32 Bias = -0.1f;
					fp32 HurtBias = 0.08f;
					fp32 Scale = 2.5f;
					fp32 Exp = 16.0f;
					CVec3Dfp32 Gamma = 1.0f;

					if (bDarknessShield)
					{
						Scale = 4.0f;
						Exp = 12.0f;
						HurtBias = 0.01f;
						Gamma = CVec3Dfp32(1.4f, 1.4f, 1.0f);
					}
					else
					{
						// The lower health we have the more redish the screen will become.
						fp32 HealthLerp = fp32(pCD->m_Health.m_Value) / fp32(pCD->m_MaxHealth.m_Value);

						HurtBias = CFXSysUtil::LerpMT(0.00125f, HurtBias, HealthLerp);
						Scale = CFXSysUtil::LerpMT(6.0f, Scale, HealthLerp);
						Exp = CFXSysUtil::LerpMT(8.0f, Exp, HealthLerp);
						CVec3Dfp32(1.0f, 1.2f, 1.2f).Lerp(Gamma, HealthLerp, Gamma);
					}

					// Interpolate values
					pPPP->m_GlowAttenuationExp = (fp32)LERP(pPPP->m_GlowAttenuationExp, Exp, Hurt);
					pPPP->m_GlowScale.Lerp(CVec3Dfp32(Scale, Scale, Scale), Hurt, pPPP->m_GlowScale);
					pPPP->m_GlowGamma.Lerp(Gamma, Hurt, pPPP->m_GlowGamma);
					pPPP->m_GlowBias.Lerp(CVec4Dfp32(Bias, Bias, Bias, HurtBias), Hurt, pPPP->m_GlowBias);
				}
			}

			return 1;
		}

	case OBJMSG_CHAR_GETDARKNESS:
		{
			CWO_Character_ClientData* pCD = GetClientData(_pObj);
			if(pCD)
				return (aint)pCD->m_Darkness;
			return 0;
		}

	case OBJMSG_CHAR_GETDARKNESSPOWER:
		{
			CWO_Character_ClientData* pCD = GetClientData(_pObj);
			if(pCD)
				return (aint)pCD->m_DarknessSelectedPower;
			return -1;
		}

	case OBJMSG_CHAR_GETLASTDARKNESSPOWER:
		{
			CWO_Character_ClientData* pCD = GetClientData(_pObj);
			if(pCD)
				return (aint)pCD->m_DarknessLastSelectedPower;
			return -1;
		}

	case OBJMSG_CHAR_GETMAXDARKNESS:
		{
			CWO_Character_ClientData* pCD = GetClientData(_pObj);
			if (pCD)
				return (aint)pCD->m_MaxDarkness.m_Value;
			return -1;
		}

	case OBJSYSMSG_GETCAMERA:
		return OnGetCamera(_pObj, _pWClient, _Msg);

	case OBJSYSMSG_GETVIEWPORT:
		{
			if (_Msg.m_DataSize != sizeof(CRC_Viewport)) return 0;
			CRC_Viewport& VP = *(CRC_Viewport*)_Msg.m_pData;

//			CWObject_Client* pObj = _pObj;

			_pObj = Player_GetLastValidPrediction(_pObj);
			CWO_Character_ClientData *pCD = GetClientData(_pObj);
			if(!pCD)
				return 0;

			fp32 IPTime = pCD->m_PredictFrameFrac;
			fp32 FOV = pCD->m_BaseFov;
			bool bDialogue3PI = ((pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_MASK) == THIRDPERSONINTERACTIVE_MODE_DIALOGUE);

#ifdef WADDITIONALCAMERACONTROL
			if (pCD->m_bAdditionalCameraControl)
			{
				FOV = 90.0f;
			}
			else
#endif
			if (_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_CUTSCENE)
			{
				int iCameraObj = (uint16)pCD->m_Cutscene_Camera;
				FOV = _pWClient->ClientMessage_SendToObject(CWObject_Message(OBJSYSMSG_GETFOV),iCameraObj);
				if(!FOV)
					FOV = (fp32)pCD->m_CutsceneFOV;
			}
			else if (((pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_MASK) != THIRDPERSONINTERACTIVE_MODE_NONE) &&
				((int32)pCD->m_ControlMode_Param2 == ACTIONCUTSCENE_TYPE_PERISCOPE))
				FOV = pCD->m_ControlMode_Param3;

			else if(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_DIALOGUE)
				FOV = GetDialogueCameraFOV_Client(pCD, _pWClient, IPTime);
			
			else if (bDialogue3PI)
			{
				fp32 Zoom = fp32(pCD->m_Zoom.Get(pCD->m_GameTick, IPTime)) * (1.0f / 255.0f);
				FOV = pCD->m_BaseFov * (1.0f - 0.7f*Zoom);

				fp32 FOV2 = GetDialogueCameraFOV_Client(pCD, _pWClient, IPTime);
				fp32 SlideTime = pCD->m_3PI_CameraSlide.Get(pCD->m_GameTick, IPTime) * (1.0f / 255.0f);
				FOV += (FOV2 - FOV) * SlideTime;
			}
			else if (pCD->m_FovOverrideStartTick)
			{
				fp32 CurFov = FOV;
				fp32 Target;
				fp32 BlendTime;
				/*if (pCD->m_FovOverrideBlendTime < 0)
				{
					Target = pCD->m_BaseFov;
					BlendTime = -pCD->m_FovOverrideBlendTime;
					CurFov = pCD->m_FovOverride;
				}
				else*/
				{
					CurFov = pCD->m_FovOverridePrevValue;
					Target = pCD->m_FovOverride;
					BlendTime = pCD->m_FovOverrideBlendTime;
				}
				fp32 Duration = fp32(int(pCD->m_GameTick - pCD->m_FovOverrideStartTick)) + IPTime;
				if(Duration >= BlendTime)
					FOV = Target;
				else if (Duration > 0)
					FOV = CurFov + (Target - CurFov) * (M_Sin(((Duration / BlendTime) - 0.5f)*_PI)*0.5f + 0.5f);
				else
					FOV = CurFov;
			}
			else if(Char_IsPlayerView(_pObj))
			{
				// Zoom
				fp32 Zoom = fp32(pCD->m_Zoom.Get(pCD->m_GameTick, IPTime)) * (1.0f / 255.0f);
				FOV = pCD->m_BaseFov * (1.0f - 0.7f*Zoom);

				fp32 CrouchLevel = fp32(pCD->m_CrouchLevel.Get(pCD->m_GameTick, pCD->m_PredictFrameFrac)) * (1.0f / 255.0f);
				CrouchLevel = 1.0f - ((1.0f - CrouchLevel) * (1.0f - CrouchLevel));
				//FOV += CrouchLevel * 20;   AR-NOTE: Disabled extra FOV when crouching
				
				// weapon focus-zoom
				fp32 ZoomMod = 0.0f;
				fp32 ZoomDuration;
				int32 Diff;
				switch(pCD->m_WeaponZoomState)
				{
				case WEAPON_ZOOMSTATE_ON:
					ZoomMod = 1.0f;
					break;

				case WEAPON_ZOOMSTATE_IN:
					Diff = _pWClient->GetGameTick() - pCD->m_WeaponZoomStartTick;
					ZoomDuration = 0.1f * _pWClient->GetGameTicksPerSecond();
					ZoomMod = ((fp32)Diff / ZoomDuration);
					ZoomMod = Min(ZoomMod, 1.0f);
					ZoomMod = Max(ZoomMod, 0.0f);
					ZoomMod = M_Sin((ZoomMod*_PI*0.25f));
					break;

				case WEAPON_ZOOMSTATE_OFF:
				    break;

				case WEAPON_ZOOMSTATE_OUT:
					Diff = _pWClient->GetGameTick() - pCD->m_WeaponZoomStartTick;
					ZoomDuration = 0.1f * _pWClient->GetGameTicksPerSecond();
					ZoomMod = ((fp32)Diff / ZoomDuration);
					ZoomMod = Min(ZoomMod, 1.0f);
					ZoomMod = Max(ZoomMod, 0.0f);
					ZoomMod = M_Sin(ZoomMod*_PI*0.25f);
					ZoomMod = 1.0f-ZoomMod;
				    break;

				default:
				    break;
				}

				FOV += (-30.0f * ZoomMod);
			}

#ifndef M_RTM
			{
				// Fov test hack
				CRegistry *pUserReg = _pWClient->Registry_GetUser();
				CRegistry *pFOV = pUserReg->FindChild("FORCEFOV");
				if(pFOV && pFOV->GetThisValuef() != -1)
					FOV = pFOV->GetThisValuef();
			}
#endif

			VP.SetFOV(FOV);
			VP.SetBackPlane(pCD->m_BackPlane);
			//if(pCD->m_BackPlane > 5000)
				VP.SetFrontPlane(1.8f);
			//else
			//	VP.SetFrontPlane(1.5f);
			return 1;
		}
	case OBJMSG_CHAR_ONANIMEVENT:
		{
			CWO_Character_ClientData* pCD = GetClientData(_pObj);
			CXR_Anim_DataKey* pKey = (CXR_Anim_DataKey*)_Msg.m_Param0;
			if (pKey && pCD)
			{
				if (pKey->m_Type == ANIM_EVENT_TYPE_DIALOGUE && pCD->m_bDoOneTimeEffects)
				{
					CWRes_Dialogue* pDialogue = GetDialogueResource(_pObj, _pWClient);
					if (!pDialogue)
						return 0;

					int iDialogue = pKey->m_Param;
					bool bNeedGroundMaterial = CFStr28(pKey->Data()).Val_int() != 0;
					bool bFootStepSound = (iDialogue >= PLAYER_FOOTSTEP_STARTINDEX && iDialogue <= PLAYER_FOOTSTEP_ENDINDEX);

					if (bFootStepSound)
					{
						// Footstepsounds
						bNeedGroundMaterial = true;
						CWObject_Client *pFirst = _pWClient->Object_GetFirstCopy(_pObj->m_iObject);
						CWO_Character_ClientData *pCDFirst = GetClientData(pFirst);
						if(pCDFirst->m_LastFootstepTick < int(pCD->m_GameTick) - 6 || iDialogue != pCDFirst->m_LastFootStepType)
						{
							pCDFirst->m_LastFootstepTick = pCD->m_GameTick;
							pCDFirst->m_LastFootStepType = iDialogue;
						}
						else
						{
							return 0;
						}
						//Char_SetFootstep(_pObj, _pWClient, 0);
					}

					uint32 DialogueItemHash = pDialogue->IntToHash(iDialogue);
					int AttnType = (_pWClient->Player_GetLocalObject() == _pObj->m_iObject) ? WCLIENT_ATTENUATION_2D : WCLIENT_ATTENUATION_3D;

					Char_PlayDialogue_Hash(_pObj, _pWClient, DialogueItemHash, AttnType, bNeedGroundMaterial);
				}
				else if (pKey->m_Type == ANIM_EVENT_TYPE_SOUND && pCD->m_bDoOneTimeEffects)
				{
					int iSound = _pWClient->GetMapData()->GetResourceIndex_Sound(pKey->Data());
					if (iSound > 0)
					{
						int AttnType = WCLIENT_ATTENUATION_3D;
						if (_pWClient->Player_GetLocalObject() == _pObj->m_iObject)
							AttnType = WCLIENT_ATTENUATION_2D;

						if(pKey->m_Param)
						{
							CVec3Dfp32 Start = _pObj->GetPosition() + CVec3Dfp32(0, 0, 1);
							CVec3Dfp32 Stop = _pObj->GetPosition() - CVec3Dfp32(0, 0, 10);
							CCollisionInfo CInfo;
							CInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_SURFACE);
							if(_pWClient->Phys_IntersectLine(Start, Stop, 0, OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_PHYSMODEL, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS | XW_MEDIUM_CAMERASOLID, &CInfo, _pObj->m_iObject))
							{
								int iMaterial = CInfo.m_SurfaceType;
								if(iMaterial == 0 && CInfo.m_pSurface && CInfo.m_pSurface->GetBaseFrame())
									iMaterial = CInfo.m_pSurface->GetBaseFrame()->m_MaterialType;

								if(iMaterial != 0)
									pCD->m_GroundMaterial = iMaterial;
							}
						}

						_pWClient->Sound_At(WCLIENT_CHANNEL_SFX, _pObj->GetPosition(), iSound, AttnType, pCD->m_GroundMaterial);
					}
				}
				else if(pKey->m_Type == ANIM_EVENT_TYPE_IK)
				{	
					CWObject_CoreData* pObjFirst = _pWClient->Object_GetFirstCopy(_pObj->m_iObject);
					CWO_Character_ClientData *pCDFirst = pObjFirst ? GetClientData(pObjFirst) : NULL;
					if(pCDFirst)
					{
						switch(pKey->m_Param)
						{
						case ANIM_EVENT_TYPE_IK_LEFT_FOOT_DOWN:
							pCDFirst->m_aLegIKIsDirty[1] = true;
							break;

						case ANIM_EVENT_TYPE_IK_LEFT_FOOT_UP:
							pCDFirst->m_aLegIKIsDirty[1] = false;
							break;

						case ANIM_EVENT_TYPE_IK_RIGHT_FOOT_DOWN:
							pCDFirst->m_aLegIKIsDirty[0] = true;
							break;

						case ANIM_EVENT_TYPE_IK_RIGHT_FOOT_UP:
							pCDFirst->m_aLegIKIsDirty[0] = false;
							break;

						case ANIM_EVENT_TYPE_IK_LEFT_FRONT_FOOT_DOWN:
							break;

						case ANIM_EVENT_TYPE_IK_LEFT_FRONT_FOOT_UP:
							break;

						case ANIM_EVENT_TYPE_IK_RIGHT_FRONT_FOOT_DOWN:
							break;

						case ANIM_EVENT_TYPE_IK_RIGHT_FRONT_FOOT_UP:
							break;
						case ANIM_EVENT_TYPE_IK_RECOIL:
							{
								CStr KeyData(pKey->Data());
								int32 Strength = KeyData.Val_int();
								pCDFirst->m_PostAnimSystem.InitRecoil(_Msg.m_Reason == TOKEN_WEAPON2, Strength, pCDFirst->m_GameTick);
								break;
							}
						}
					}
				}
				else if (pKey->m_Type == ANIM_EVENT_TYPE_EFFECT)
				{
					// Do effect things.. (how to differentiate between gun ag and normal ag?)
					//...
					CMTime EventTime = (_Msg.m_pData && _Msg.m_DataSize >= sizeof(CMTime) ? *(CMTime*)_Msg.m_pData : pCD->m_GameTime);
					CStr KeyData(pKey->Data());
					if (pKey->m_Param == AG2_ANIMEVENT_EFFECTTYPE_NEWWEAPONEFFECT)
					{
						int32 iEffect = KeyData.Val_int();
						CMTime EventTime = (_Msg.m_pData && _Msg.m_DataSize >= sizeof(CMTime) ? *(CMTime*)_Msg.m_pData : CMTime::CreateFromTicks(pCD->m_GameTick,_pWClient->GetGameTickTime()));
						//	Add 1 ticks to player
						if (pCD->m_iPlayer != -1)
						{
							EventTime += CMTime::CreateFromTicks(1,_pWClient->GetGameTickTime());
						}

						if (_Msg.m_Reason == TOKEN_WEAPON1) 
						{
							// Set Model and anim times
							//ConOutL(CStrF("Setting extramodel1: %d:%f Time: %f, Seed(%d)",pCD->m_Item0_Model.m_ExtraModels.m_lEffects[iEffect].m_iModel,EventTime.GetTime(),CMTime::CreateFromTicks(pCD->m_GameTick,_pWClient->GetGameTickTime()).GetTime(), pCD->m_GameTick));
							pCD->m_Item0_Model.m_ExtraModels.SetExtraModel(_pWClient->GetMapData(),EventTime,2.0f,iEffect,pCD->m_GameTick);
						}
						else if (_Msg.m_Reason == TOKEN_WEAPON2)
						{
							//ConOutL(CStrF("Setting extramodel2: %d:%f Time: %f, Seed(%d)",pCD->m_Item1_Model.m_ExtraModels.m_lEffects[iEffect].m_iModel,EventTime.GetTime(),CMTime::CreateFromTicks(pCD->m_GameTick,_pWClient->GetGameTickTime()).GetTime(), pCD->m_GameTick));
							pCD->m_Item1_Model.m_ExtraModels.SetExtraModel(_pWClient->GetMapData(),EventTime,2.0f,iEffect,pCD->m_GameTick);
						}
					}
					else if (pKey->m_Param == AG2_ANIMEVENT_EFFECTTYPE_DISABLEANIMCAM)
					{
						pCD->m_AnimGraph2.SetStateFlagsLo(pCD->m_AnimGraph2.GetStateFlagsLo() & ~AG2_STATEFLAG_TAGUSEANIMCAMERA);
					}
				}
				else if (pKey->m_Type == ANIM_EVENT_TYPE_ITEMOCCLUSIONMASK)
				{
					// Set item occlusionmask
					CAutoVar_AttachModel* pModel;
					if (pKey->m_Param == 1)
						pModel = &pCD->m_Item1_Model;
					else if (pKey->m_Param == 2)
						pModel = &pCD->m_Item2_Model;
					else
						pModel = &pCD->m_Item0_Model;

					CStr KeyData(pKey->Data());
					pModel->m_SurfaceOcclusionMask = KeyData.GetStrSep(",").Val_int();
					pModel->m_SurfaceShadowOcclusionMask = KeyData.Val_int();
				}
				else if (pKey->m_Type == ANIM_EVENT_TYPE_GAMEPLAY)
				{
					switch (pKey->m_Param)
					{
					case AG2_ANIMEVENT_GAMEPLAY_WEAPONATTACH:
						{
							CFStr Data = pKey->Data();
							int iAttachPoint = Data.GetStrSep(",").Val_int();
							if (iAttachPoint >= 0 && iAttachPoint < 10)
							{
								int iItem = Data.Val_int();
								if (iItem == 0)
									pCD->m_Item0_Model.m_iAttachPoint[0] = iAttachPoint;
								else
									pCD->m_Item1_Model.m_iAttachPoint[0] = iAttachPoint;
							}
							break;
						}
					}
				}
				/*else if (pKey->m_Type == ANIM_EVENT_TYPE_ANIMIMPULSE)
				{
					CWObject_Message Msg(OBJMSG_CHAR_ANIMIMPULSE,6);
					Msg.m_pData = pKey->Data();
					Msg.m_DataSize = pKey->DataSize();
					OnCharAnimImpulse(Msg,pCD,pCD->m_AnimGraph2.GetAG2I());
				}
				else if (pKey->m_Type == ANIM_EVENT_TYPE_WEAPONIMPULSE)
				{
					CWObject_Message Msg(OBJMSG_CHAR_ANIMIMPULSE,6);
					Msg.m_pData = pKey->Data();
					Msg.m_DataSize = pKey->DataSize();
					OnCharAnimImpulse(Msg,pCD,pCD->m_WeaponAG2.GetAG2I());
				}*/
/*				if(pKey->m_EventParams[1] != 0)
				{
					pCD->m_Item0_Model.EvalAnimKey(0, pKey, pCD->m_GameTick);
					pCD->m_Item1_Model.EvalAnimKey(1, pKey, pCD->m_GameTick);
					pCD->m_Item2_Model.EvalAnimKey(2, pKey, pCD->m_GameTick);

				}*/
			}
			return 1;
		}
	case OBJMSG_CHAR_GETANIMLAYERFROMONLYANIMTYPE:
		{
			// Ok then, this will get animation layers that match given "onlyanimtype"
			// NOTE: Atm it will also give every other layers that do not use "value compare"
			CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
			CMTime Time = CMTime::CreateFromTicks(_pWClient->GetGameTick(),_pWClient->GetGameTickTime(),_pWClient->GetRenderTickFrac());
			CWAG2I_Context Context(_pObj,_pWClient,Time);
			int NumLayers = _Msg.m_DataSize;
			pCD->m_AnimGraph2.GetAG2I()->GetValueCompareLayers(&Context,(CXR_AnimLayer*)_Msg.m_pData,NumLayers, _Msg.m_Param0);
			return NumLayers;
		}
	case OBJMSG_CHAR_GETTENSION:
		{
			CWO_Character_ClientData* pCD = GetClientData(_pObj);
			if(!pCD)
				return 0;

			if(_Msg.m_Param0 != 0)
			{
				*(fp32 *)_Msg.m_Param0 = pCD->m_SoundLevel.Get(pCD->m_GameTick, 0.0f) / 255.0f;
			}

			if(_Msg.m_DataSize == sizeof(fp32) && pCD)
			{
				fp32 *pTension = (fp32 *)_Msg.m_pData;
				*pTension = MinMT(MaxMT(fp32(pCD->m_Tension) / 255.0f, 0.0f), 1.0f);
				return 1;
			}
			else
				return 0;
		}

	case OBJSYSMSG_PHYSICS_PREINTERSECTION: 
		return OnClientPreIntersection(_pObj, _pWClient, _Msg.m_Param0, (CCollisionInfo *)_Msg.m_pData);

	case OBJMSG_GAME_RENDERSTATUSBAR:
		{
			OnClientRenderStatusBar(_pObj, _pWClient, (CXR_Engine *)_Msg.m_Param0, (CRC_Util2D *)_Msg.m_Param1);
			return 1;
		}

	case OBJMSG_CHAR_ISDEAD:
		if(Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD)
			return 1;
		else
			return 0;

	case OBJMSG_CHAR_CLIENTINPUTOVERRIDE:
		{
			CWO_Character_ClientData *pCD = GetClientData(_pObj);
			if (!pCD)
				return 0;

			if(pCD->m_nChoices == 0)
				return 0;

			if(_Msg.m_Param0 == CONTROLBITS_DPAD_UP)
			{
				pCD->m_iCurChoice--;
				if(pCD->m_iCurChoice != -1)
					pCD->m_ScrollUpTimer = 0.001f;

				pCD->m_iCurChoice = Max(pCD->m_iCurChoice, 0);
				pCD->m_ScrollDownTimer = 0.0f;
			}
			else if(_Msg.m_Param0 == CONTROLBITS_DPAD_DOWN)
			{
				pCD->m_iCurChoice++;
				if(pCD->m_iCurChoice != pCD->m_nChoices )
					pCD->m_ScrollDownTimer = 0.001f;

				pCD->m_iCurChoice = Min(pCD->m_iCurChoice, pCD->m_nChoices - 1);
				pCD->m_ScrollUpTimer = 0.0f;
				
			}
			else if(_Msg.m_Param0 == CONTROLBITS_BUTTON0)
			{
				// if not in any telephone situation, or "dial manually" is called, run normal select dialogue

				CStr Check1 = pCD->m_Choices.Ansi();
				// check if is in phone mode
				bool IsPhone = false;
				int ChekcIfPhone = Check1.Find("(");
				if(ChekcIfPhone != -1)
					ChekcIfPhone = Check1.Find(")");
					if(ChekcIfPhone != -1)
						IsPhone = true;
				
				if(!IsPhone)
				{
					// No really, not everything is a character...
					CWObject_Client *pFocusFrameObj = _pWClient->Object_Get(pCD->m_iFocusFrameObject);
					if (CWObject_Character::IsCharacter(pFocusFrameObj))
					{
						CWO_Character_ClientData *pFocusFrameCD = pFocusFrameObj ? GetClientData(pFocusFrameObj) :NULL;
						if(pFocusFrameCD && pFocusFrameCD->m_liDialogueChoice.Len() > pCD->m_iCurChoice)
						{
							uint32 ItemHash = pFocusFrameCD->m_liDialogueChoice[pCD->m_iCurChoice].m_ItemHash;
							pCD->m_lUsedChoices.Add(ItemHash);
						}
					}

					ConExecute(CStrF("cl_selectdialoguechoice(%i)", pCD->m_iCurChoice));	
					pCD->m_iLastFocusFrameObject = -1; // this forces a "check for first non-gray choise in OBJMSG_CHAR_GETCHOICES
				}
				else
				{
					if(pCD->m_iCurChoice == (pCD->m_nChoices-1))
					{
						ConExecute(CStrF("cl_selectdialoguechoice(%i)", pCD->m_iCurChoice));	
					}
					else
					{
						// compare current selected string to inventory  to get the correct iterator, and parse out the
						// phone number in the item description
						CStr Choice = pCD->m_Choices.Ansi();
						for(int i = 0; i < pCD->m_iCurChoice; i++)
							Choice.GetStrSep(";");

						int iPos = Choice.Find(";");
						if(iPos != -1)
							Choice = Choice.DelFrom(iPos);

						int iNumItems = pCD->m_InventoryInfo.m_lInventory.Len();
						CStr CheckString;
						for(int i = 0; i < iNumItems; i++)
						{
							if(pCD->m_InventoryInfo.m_lInventory[i].m_Flags & CWO_Inventory_Info::INVENTORYINFO_TYPE_PHONEBOOK)
							{
								Choice = Choice.UpperCase();
								// Get the phonenumber from the itemdescription

								CStr ThePhoneNumber;
								CheckString = Localize_Str(pCD->m_InventoryInfo.m_lInventory[i].m_ItemDescription);
								int iFinder = CheckString.Find("555");
								if(iFinder != -1)
								{
									CheckString = CheckString.DelTo(iFinder-1);
									CheckString = CheckString.DelFrom(10); // 8 +1 for "-" and +1 for " "
									CheckString = CheckString.Del(7,1);
									CheckString = CheckString.Del(3,1);
									ThePhoneNumber = CheckString.Ansi();

									CheckString = "§LPHONENUMBER_OWNER_" + CheckString;
									CheckString = Localize_Str(CheckString);
									CheckString = CheckString.UpperCase();

									if(CheckString.Ansi() == Choice)
									{
										const char * sendThis = (char*)ThePhoneNumber.Str();
										ConExecute(CStrF("cmdstr(%i, \"%s\")", CONTROL_DIALTELEPHONENUMBER, sendThis));
										break;
									}
								}
							}
						}
					}
				}


				pCD->m_iCurChoice = 0;
			}
			else
				return 0;

			return 1;
		}

/*********************************************************************************************
 * END MESSAGE ORDERING
 *********************************************************************************************/

	case OBJMSG_CHAR_GETCHOICES: // On Client
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pObj);
			if (!pCD)
				return 0;

			CStr *pSt = (CStr *)_Msg.m_pData;
			int nChoices = pCD->m_liDialogueChoice.Len();
			if(nChoices && pSt)
			{
				CWObject_Client *pPlayer = _pWClient->Object_Get(_Msg.m_iSender);
				if(pPlayer && (pPlayer->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER) != 0)
				{
					// get first nogray choice and set that as selected
					CWO_Character_ClientData *pPlayerCD = GetClientData(pPlayer);
					if(pPlayerCD && pPlayerCD->m_iLastFocusFrameObject != pPlayerCD->m_iFocusFrameObject)
					{
						// only once
						pPlayerCD->m_iLastFocusFrameObject = pPlayerCD->m_iFocusFrameObject;

						uint32 ItemHash = pCD->m_liDialogueChoice[pPlayerCD->m_iCurChoice].m_ItemHash;
						uint32 UsedItemHash;
						bool bIsGreyedOut = false;
						for(int i = 0; i < nChoices; i++)
						{
							bIsGreyedOut = false;
							ItemHash = pCD->m_liDialogueChoice[i].m_ItemHash;
							// check this choice against all used choices
							for(int j = 0; j < pPlayerCD->m_lUsedChoices.Len(); j++)
							{
								UsedItemHash = pPlayerCD->m_lUsedChoices[j];
								if(ItemHash == UsedItemHash)
									bIsGreyedOut = true;		
							}
							if(!bIsGreyedOut)
							{
								pPlayerCD->m_iCurChoice = i;
								break;
							}
						}
					}
				}

				CWRes_Dialogue *pDialogue = GetDialogueResource(pPlayer, _pWClient);
				if(pDialogue)
				{
					for (int i = 0; i < nChoices; i++)
					{
						uint32 ItemHash = pCD->m_liDialogueChoice[i].m_ItemHash;
						CStr Choice = (wchar *)pDialogue->FindEvent_Hash(ItemHash, CWRes_Dialogue::EVENTTYPE_CHOICE);
						if (Choice == "")
						{ // Subtitle fallback
							const wchar* pSub = (wchar *)pDialogue->FindEvent_Hash(ItemHash, CWRes_Dialogue::EVENTTYPE_SUBTITLE);
							if (pSub)
								Choice = pSub + 1;// skip flags
						}
						if(i != 0)
							*pSt += ";";
						*pSt += Choice;
					}
				}
			}
			else
			{
				CWObject_Client *pPlayer = _pWClient->Object_Get(_Msg.m_iSender);
				if (pPlayer && (pPlayer->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER) != 0)
				{
					CWO_Character_ClientData *pPlayerCD = GetClientData(pPlayer);
					if (pPlayerCD->m_FocusFrameUseText == "§LACS_DEVOUR")
					{
						*pSt = pPlayerCD->m_FocusFrameUseText;
						nChoices = 1;
					}
				}
			}
			return nChoices;
		}
		return 0;

	case OBJMSG_CHAR_CSHASBORDER:
		{
			CWO_Character_ClientData* pCD = GetClientData(_pObj);
			if(!pCD)
				return 0;
			int EFlags = pCD->m_ExtraFlags;
			return !(EFlags & PLAYER_EXTRAFLAGS_CSNOBORDER);

		}
	case OBJMSG_CHAR_HASAUTOAIMTARGET:
		{
			CWO_Character_ClientData* pCD = GetClientData(_pObj);
			if(!pCD)
				return 0;
			return pCD->m_AimTarget;

		}
	case OBJMSG_HOOK_GETCURRENTMATRIX:
		{
			CMat4Dfp32 *pMatrix = (CMat4Dfp32 *)_Msg.m_Param0;
			bool bLocalPlayer = (_pObj->m_iObject == _pWClient->Player_GetLocalObject());
			bool bPredicted = false;

			if (bLocalPlayer)
			{
#ifdef WCLIENT_FIXEDRATE
				int bNoPredict = true;
#else
				CRegistry* pUserReg = _pWClient->Registry_GetUser();
				int bNoPredict = (pUserReg) ? pUserReg->GetValuei("NOPREDICT") : 0;
#endif
				if (!bNoPredict)
				{
					CWC_Player* pPlayer = _pWClient->Player_GetLocal();
					if (pPlayer == NULL) return 0;

					if (_pObj->GetNext() != NULL)
					{
						_pObj = Player_GetLastValidPrediction(_pObj);
						bPredicted = true;
					}
				}
			}
			fp32 IPTime = 0.0f;
			CVec3Dfp32 ExtraOffset(0.0f,0.0f,0.0f);
			if (bLocalPlayer && bPredicted)
			{
				*pMatrix = _pObj->GetPositionMatrix();
				CWObject_CoreData* pObjFirst = _pWClient->Object_GetFirstCopy(_pObj->m_iObject);
				if (pObjFirst)
				{
					CWO_Character_ClientData *pCDFirst = GetClientData(pObjFirst);
					IPTime = pCDFirst->m_PredictFrameFrac;
					fp32 IPPredMiss = 1.0f - Clamp01(fp32(_pWClient->GetGameTick() - pCDFirst->m_PredMiss_Tick) + _pWClient->GetRenderTickFrac());
					CVec3Dfp32 PredMissAdd;
					pCDFirst->m_PredMiss_dPos.Scale(IPPredMiss, PredMissAdd);

					if (pCDFirst->m_LastTeleportTick != pCDFirst->m_GameTick)
						pMatrix->GetRow(3) += PredMissAdd;
					ExtraOffset = pCDFirst->m_Camera_CharacterOffset;
				}
			}
			else
			{
				IPTime = _pWClient->GetRenderTickFrac() / (_pWClient->GetModeratedFramePeriod() * _pWClient->GetGameTicksPerSecond());
				Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), *pMatrix, IPTime);
			}
			if (_Msg.m_iSender != -1 && _Msg.m_pData)
			{
				CXR_AnimState AnimState;
				struct SkelProp
				{
					CXR_Skeleton* m_pSkel;
					CXR_SkeletonInstance* m_pSkelInst;
				};
				struct SkelProp* pSkelProp = (struct SkelProp*)_Msg.m_pData;
				GetEvaluatedPhysSkeleton(_pObj, _pWClient, pSkelProp->m_pSkelInst,pSkelProp->m_pSkel,AnimState,_pWClient->GetRenderTickFrac(),pMatrix);
				/*if (pSkelInst)
				{
				pMatrix->GetRow(0) = pSkelInst->m_pBoneTransform[_Msg.m_iSender].GetRow(0);
				pMatrix->GetRow(1) = pSkelInst->m_pBoneTransform[_Msg.m_iSender].GetRow(1);
				pMatrix->GetRow(2) = pSkelInst->m_pBoneTransform[_Msg.m_iSender].GetRow(2);
				pMatrix->GetRow(3) = pSkelInst->m_pBoneTransform[_Msg.m_iSender].GetRow(3);
				}*/
			}
			else
			{
				CXR_AnimState AnimState;
				CXR_Skeleton* pSkel;
				CXR_SkeletonInstance* pSkelInstance;
				GetEvaluatedPhysSkeleton(_pObj, _pWClient, pSkelInstance,pSkel,AnimState,IPTime,pMatrix);
				if (pSkelInstance)
				{
					uint iNode = _Msg.m_Param1;
					if (iNode && (iNode < pSkel->m_lNodes.Len()))
					{
						CVec3Dfp32 LocalPos = pSkel->m_lNodes[iNode].m_LocalCenter;
						*pMatrix = pSkelInstance->m_pBoneTransform[iNode];
						pMatrix->GetRow(3) = LocalPos * (*pMatrix) + ExtraOffset;
					}
					else if (pSkelInstance->m_nBoneTransform)
					{
						// Get root position and set that as the matrix pos
						if (pSkelInstance->m_nBoneTransform > PLAYER_ROTTRACK_CAMERA)
							*pMatrix = pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_CAMERA];
						pMatrix->GetRow(3) = pSkelInstance->m_pBoneTransform[0].GetRow(3) + ExtraOffset;
					}
				}
			}
			return 1;
		}
	case OBJMSG_HOOK_GETRENDERMATRIX:
		{
			//CWO_Character_ClientData* pCD = GetClientData(_pObj);
			//CMTime Time = CMTime::CreateFromTicks(pCD->m_GameTick,_pWClient->GetGameTickTime(),pCD->m_PredictFrameFrac);
			CMat4Dfp32* pMatrix = (CMat4Dfp32*)_Msg.m_Param1;

			// Copied from CharRender.cpp\OnClientRender().

			bool bLocalPlayer = (_pObj->m_iObject == _pWClient->Player_GetLocalObject());
			bool bPredicted = false;

			if (bLocalPlayer)
			{
#ifdef WCLIENT_FIXEDRATE
				int bNoPredict = true;
#else
				CRegistry* pUserReg = _pWClient->Registry_GetUser();
				int bNoPredict = (pUserReg) ? pUserReg->GetValuei("NOPREDICT") : 0;
#endif
				if (!bNoPredict)
				{
					CWC_Player* pPlayer = _pWClient->Player_GetLocal();
					if (pPlayer == NULL) return 0;

					if (_pObj->GetNext() != NULL)
					{
						_pObj = Player_GetLastValidPrediction(_pObj);
						bPredicted = true;
					}
				}
			}

			fp32 IPTime = 0.0f;
			CVec3Dfp32 ExtraOffset(0.0f,0.0f,0.0f);
			if (bLocalPlayer && bPredicted)
			{
				*pMatrix = _pObj->GetPositionMatrix();
				CWObject_CoreData* pObjFirst = _pWClient->Object_GetFirstCopy(_pObj->m_iObject);
				if (pObjFirst)
				{
					CWO_Character_ClientData *pCDFirst = GetClientData(pObjFirst);
					IPTime = pCDFirst->m_PredictFrameFrac;
					fp32 IPPredMiss = 1.0f - Clamp01(fp32(_pWClient->GetGameTick() - pCDFirst->m_PredMiss_Tick) + _pWClient->GetRenderTickFrac());
					CVec3Dfp32 PredMissAdd;
					pCDFirst->m_PredMiss_dPos.Scale(IPPredMiss, PredMissAdd);

					if (pCDFirst->m_LastTeleportTick != pCDFirst->m_GameTick)
						pMatrix->GetRow(3) += PredMissAdd;
					ExtraOffset = pCDFirst->m_Camera_CharacterOffset;
				}
			}
			else
			{
				IPTime = _pWClient->GetRenderTickFrac() / (_pWClient->GetModeratedFramePeriod() * _pWClient->GetGameTicksPerSecond());
				Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), *pMatrix, IPTime);
			}
			if (_Msg.m_iSender != -1 && _Msg.m_pData)
			{
				CXR_AnimState AnimState;
				struct SkelProp
				{
					CXR_Skeleton* m_pSkel;
					CXR_SkeletonInstance* m_pSkelInst;
				};
				struct SkelProp* pSkelProp = (struct SkelProp*)_Msg.m_pData;
				GetEvaluatedPhysSkeleton(_pObj, _pWClient, pSkelProp->m_pSkelInst,pSkelProp->m_pSkel,AnimState,_pWClient->GetRenderTickFrac(),pMatrix);
				/*if (pSkelInst)
				{
				pMatrix->GetRow(0) = pSkelInst->m_pBoneTransform[_Msg.m_iSender].GetRow(0);
				pMatrix->GetRow(1) = pSkelInst->m_pBoneTransform[_Msg.m_iSender].GetRow(1);
				pMatrix->GetRow(2) = pSkelInst->m_pBoneTransform[_Msg.m_iSender].GetRow(2);
				pMatrix->GetRow(3) = pSkelInst->m_pBoneTransform[_Msg.m_iSender].GetRow(3);
				}*/
			}
			else
			{
				CXR_AnimState AnimState;
				CXR_Skeleton* pSkel;
				CXR_SkeletonInstance* pSkelInstance;
				GetEvaluatedPhysSkeleton(_pObj, _pWClient, pSkelInstance,pSkel,AnimState,IPTime,pMatrix);
				if (pSkelInstance && pSkelInstance->m_nBoneTransform)
				{
					// Get root position and set that as the matrix pos
					if (pSkelInstance->m_nBoneTransform > PLAYER_ROTTRACK_CAMERA)
						*pMatrix = pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_CAMERA];
					pMatrix->GetRow(3) = pSkelInstance->m_pBoneTransform[0].GetRow(3) + ExtraOffset;
				}
			}
			return 1;
		}
	case OBJMSG_CHAR_GETRENDERMATRIX:
		{
//			fp32 TickFrac = _Msg.m_VecParam0[0];
			CMat4Dfp32* pMatrix = (CMat4Dfp32*)_Msg.m_pData;

			// Copied from CharRender.cpp\OnClientRender().

			bool bLocalPlayer = (_pObj->m_iObject == _pWClient->Player_GetLocalObject());

			if (bLocalPlayer)
			{
#ifdef WCLIENT_FIXEDRATE
				int bNoPredict = true;
#else
				CRegistry* pUserReg = _pWClient->Registry_GetUser();
				int bNoPredict = (pUserReg) ? pUserReg->GetValuei("NOPREDICT") : 0;
#endif
				if (!bNoPredict)
				{
					CWC_Player* pPlayer = _pWClient->Player_GetLocal();
					if (pPlayer == NULL) return 0;

					if (_pObj->GetNext() != NULL)
						_pObj = Player_GetLastValidPrediction(_pObj);
				}
			}

			*pMatrix = _pObj->GetPositionMatrix();
			return 1;
		}
		/*
	case OBJMSG_CHAR_GETANIMFROMHANDLE: // depricated
		{
			return int(GetAnimSequence(_pObj, _pWClient, _Msg.m_Param0));
		}*/

	case OBJMSG_DEPTHFOG_GET:
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pObj);
			if(!pCD)
				return 0;

			if(!(pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_OVERRIDEFOG))
				return 0;

			CWObject_WorldSky::CDepthFog *pFog = (CWObject_WorldSky::CDepthFog *)_Msg.m_pData;
			*pFog = pCD->m_DepthFog.Get(pCD->m_GameTick, pCD->m_PredictFrameFrac);
			return 1;
		}
	case OBJMSG_CHAR_GETSPECIALGRAB:
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pObj);
			if (pCD)
			{
				if (pCD->m_AnimGraph2.GetPropertyBool(PROPERTY_BOOL_ISSLEEPING))
					return 3;
				return pCD->m_SpecialGrab;
			}
			return 0;
		}

	case OBJMSG_GAMEP4_DESTROYVOICES:
		{
			Voice_Stop(_pObj, _pWClient);
			CWO_Character_ClientData *pCD = GetClientData(_pObj);
			if(pCD)
			{
				pCD->m_DialogueInstance.Reset(CMTime(), 0, 0.0f, 0);
				pCD->m_VoCap.StopVoice();
			}
			return 1;
		}
		
		/*
	case OBJMSG_CHAR_GETEVOLVETRAIL:
		{
			return 0;
//			CWO_Character_ClientData *pCD = GetClientData(_pObj);
//			if(!pCD)
//				return 0;
//			return (pCD->m_Item0_Flags & RPG_ITEM_FLAGS_EVOLVECLIENTTRAIL) | (pCD->m_Item1_Flags & RPG_ITEM_FLAGS_EVOLVECLIENTTRAIL);
		}
		*/

	/*
	case OBJMSG_GAME_GETTARGETHUDDATA: // depricated
		{
			CWO_Character_ClientData* pCD = GetClientData(_pObj);
			if (!pCD)
				return 0;

			CTargetHudData* pTargetHudData = (CTargetHudData*)_Msg.m_Param0;
			if (!pTargetHudData)
				return 0;
			
			pTargetHudData->m_pName = pCD->m_Character.m_Value.Str();
			pTargetHudData->m_Health = pCD->m_Health;

			return 1;
		}*/

	case OBJMSG_CHAR_GETMAPINFO:
		{
			CWO_Character_ClientData* pCD = GetClientData(_pObj);
			if (!pCD)
				return 0;

//			if((pCD->m_Map_iSurface & 0x8000) == 0)
//				return 0;

			CWO_MapInfo *pInfo = (CWO_MapInfo *)_Msg.m_pData;
			pInfo->m_iSurface = pCD->m_Map_iSurface;
			pInfo->m_TopLeft = CVec2Dfp32(pCD->m_Map_PosLeft, pCD->m_Map_PosTop);
			pInfo->m_BottomRight = CVec2Dfp32(pCD->m_Map_PosRight, pCD->m_Map_PosBottom);
			pInfo->m_OverviewMap = pCD->m_Overviewmap;
			pInfo->m_OverviewMap_Visited = (int)pCD->m_Overviewmap_Visited;
			pInfo->m_OverviewMap_Current = (int)pCD->m_Overviewmap_Current;
			return 1;
		}

	case OBJMSG_CHAR_PROCESSCONTROL:
		{
			// I am most probably a boar/wolf

			return 1;
/*			CControlFrame *pFrame = (CControlFrame *)_Msg.m_pData;
			int Pos = _Msg.m_Param1;;
			int *pPos = (int *)_Msg.m_Param1;

			CCmd Cmd;
			pFrame->GetCmd(*pPos, Cmd);
			return 1;*/
		}

	case OBJSYSMSG_GETANIMSTATE :
		{
			CXR_AnimState Anim;
			if (!_Msg.m_pData) return 0;
			if (_Msg.m_DataSize != sizeof(Anim)) return 0;

			CWO_Character_ClientData* pCD = GetClientData(_pObj);
			if (!pCD) return 0;

			CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
			CXR_Skeleton* pSkel = (pModel) ? pModel->GetSkeleton() : NULL;

			fp32 IPTime = (pCD->m_bIsValidPrediction) ? pCD->m_PredictFrameFrac : _Msg.m_VecParam0[0];
			if (!OnGetAnimState(_pObj, _pWClient, pSkel, 0, _pObj->GetPositionMatrix(), IPTime, Anim)) return 0;
			*(CXR_AnimState*)_Msg.m_pData = Anim;
			return 1;
		}

	case OBJSYSMSG_GETPHYSANIMSTATE :
		{
			CXR_AnimState Anim;
			if (!_Msg.m_pData) return 0;
			if (_Msg.m_DataSize != sizeof(Anim)) return 0;

			CWO_Character_ClientData* pCD = GetClientData(_pObj);
			if (!pCD) return 0;

			CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
			CXR_Skeleton* pSkel = (pModel) ? pModel->GetPhysSkeleton() : NULL;

			fp32 IPTime = (pCD->m_bIsValidPrediction) ? pCD->m_PredictFrameFrac : _Msg.m_VecParam0[0];
			if (!OnGetAnimState(_pObj, _pWClient, pSkel, 1, _pObj->GetPositionMatrix(), IPTime, Anim)) return 0;
			*(CXR_AnimState*)_Msg.m_pData = Anim;
			return 1;
		}

	case OBJMSG_RPG_ACTIVATEDIALOGUEITEM:
		{
			CWO_Character_ClientData* pCD = GetClientData(_pObj);
			if (!pCD) return 0;

			CWRes_Dialogue::CRefreshRes Res;
			//OnRefresh_Dialogue(_pObj, _pWClient, _Msg.m_Param0, _Msg.m_Param1, &Res);
			OnRefresh_Dialogue_Hash(_pObj, _pWClient, _Msg.m_Param0, _Msg.m_Param1, &Res);
			EvalDialogueLink_Client(_pObj, _pWClient, Res);

			// Update occlusion
			{
				CVec3Dfp32 MouthPos = Char_GetMouthPos(_pObj);
				CMat4Dfp32 Mat;
				_pWClient->Render_GetLastRenderCamera(Mat);
				uint8 Res = _pWClient->Phys_IntersectLine(CVec3Dfp32::GetRow(Mat, 3), MouthPos, 0, OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, NULL);
				pCD->m_Occlusion = (Res << 8) | Res;
			}

			return CWObject_RPG::OnClientMessage(_pObj, _pWClient, _Msg);
		}

	case OBJMSG_GAME_REQUESTSUBTITLE:
		{
			CWO_Character_ClientData* pCD = GetClientData(_pObj);
			if(!pCD)
				return 0;

			if(_Msg.m_Param0 != 0)
				*((int *)_Msg.m_Param0) = pCD->m_nUDMoney;
				
			return aint(&pCD->m_DialogueInstance);
		}
/*
	case OBJMSG_CHAR_FIGHTINGEACHOTHER: // depricated
		{
			CWO_Character_ClientData *pCD = GetClientData(_pObj);

			if (pCD)
			{
				// Check if we are fighting with the one sending the message
				int iFightingChar = pCD->m_iFightingCharacter;
				
				return (iFightingChar == _Msg.m_iSender);
			}
			return false;
		}
		*/
	case OBJMSG_CHAR_GETMOUNTCAMERA:
		{
			CWO_Character_ClientData* pCD = GetClientData(_pObj);
			CMat4Dfp32& Camera = *(CMat4Dfp32*)_Msg.m_pData;

			// if this char is currently attached
			bool bOk = false;
			fp32 IPTime = _pWClient->GetRenderTickFrac();
			if(pCD->m_RenderAttached != 0)
			{	
				CWObject_Client *pAttachObj = _pWClient->Object_Get(pCD->m_RenderAttached);
				if(pAttachObj)
				{
					CWObject_Message Msg(OBJMSG_HOOK_GETCURRENTMATRIX, (aint)&Camera, pCD->m_iAttachJoint);
					bOk = _pWClient->ClientMessage_SendToObject(Msg, pCD->m_RenderAttached) != 0;
				}
			}

			if (!bOk)
			{
				
				CMat4Dfp32 Pos, Res;
				Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), Pos, IPTime);
				if (_Msg.m_Param0)
				{
					Camera = Pos;
				}
				else
				{
					Camera.RotY_x_M(-0.25f);
					Camera.RotX_x_M(0.25f);
					CVec3Dfp32::GetRow(Pos, 3).SetRow(Camera, 3);
				}
			}
		
			Camera_Get(_pWClient, &Camera, _pObj, IPTime);
			return 1;
		}

	/*case OBJMSG_CHAR_GETFIGHTPOSITION:
		{
			CWO_Character_ClientData *pCD = GetClientData(_pObj);

			if ((_Msg.m_pData != NULL) && (_Msg.m_DataSize >= sizeof(CVec3Dfp32)))
			{
				*((CVec3Dfp32*)_Msg.m_pData) = pCD->m_FCharMovement.GetNextTarget(_pObj);

				return true;
			}

			return false;
		}*/
	/*case OBJMSG_CHAR_GETWANTEDBEHAVIOR:
		{
			CWO_Character_ClientData *pCD = GetClientData(_pObj);
			
			return (pCD ? pCD->GetWantedBehavior() : false);
		}*/
		/*
	case OBJMSG_CHAR_SETWANTEDBEHAVIOR:
		{
			CWO_Character_ClientData *pCD = GetClientData(_pObj);
			if (pCD)
			{
				pCD->SetWantedBehavior(_Msg.m_Param0);
				return true;
			}

			return false;
		}*/

	case OBJMSG_CHAR_GETQUESTITEMNAME:
		{
			CWO_Character_ClientData *pCD = GetClientData(_pObj);
			if (!pCD)
				return 0;
			
			CFStr *pStr = (CFStr*)_Msg.m_pData;
			if(_Msg.m_Param0 < pCD->m_InventoryInfo.m_lInventory.Len())
			{
				*pStr = pCD->m_InventoryInfo.m_lInventory[_Msg.m_Param0].m_ItemName;
				return 1;
			}
			else
			{
				*pStr = "";
				return 0;
			}
		}
	case OBJMSG_CHAR_GETQUESTITEMDESC:
		{
			CWO_Character_ClientData *pCD = GetClientData(_pObj);
			if (!pCD)
				return 0;
			
			CFStr *pStr = (CFStr*)_Msg.m_pData;
			if(_Msg.m_Param0 < pCD->m_InventoryInfo.m_lInventory.Len())
			{
				*pStr = pCD->m_InventoryInfo.m_lInventory[_Msg.m_Param0].m_ItemDescription;
				return 1;
			}
			else
			{
				*pStr = "";
				return 0;
			}
		}

	case OBJMSG_CHAR_GETQUESTICON:
		{
			CWO_Character_ClientData *pCD = GetClientData(_pObj);
			if (!pCD)
				return 0;

			if(_Msg.m_Param0 < pCD->m_InventoryInfo.m_lInventory.Len())
				return pCD->m_InventoryInfo.m_lInventory[_Msg.m_Param0].m_iMissionSurface;
			else
				return 0;
		}
	case OBJMSG_CHAR_GETQUESTITEMQUANTITY:
		{
			CWO_Character_ClientData *pCD = GetClientData(_pObj);
			if (!pCD)
				return 0;

			return pCD->m_InventoryInfo.m_lInventory[_Msg.m_Param0].m_NumItems;
		}

	case OBJMSG_CHAR_ISINFIGHTMODE:
		{
			CWO_Character_ClientData *pCD = GetClientData(_pObj);
			
			if (pCD)
				return pCD->m_iFightingCharacter;
			return -1;
		}

	case OBJMSG_CHAR_NEXTDIALOGUECHOICE:
		{
			M_ASSERT(false, "deprecated!");
			return 0;
		}
	case OBJMSG_CHAR_SELECTDIALOGUECHOICE:
		{
			M_ASSERT(false, "deprecated!");
			return 0;
		}
	case OBJMSG_CHAR_TELEPHONESELECT:
		{
			ConExecute(CStrF("cl_selecttelephonechoice(%i)", _Msg.m_Param0));
			return 1;
		}

#if 1
	case OBJMSG_CHAR_ISDEBUGHUDVISIBLE:
		{
			CWO_Character_ClientData* pCD = GetClientData(_pObj);
			if (pCD)
				return (pCD->m_Debug_ShowHud) ? 1 : 0;

			return 0;
		}

	case OBJMSG_CHAR_SHOWDEBUGHUD:
		{
			CWO_Character_ClientData* pCD = GetClientData(_pObj);
			if (pCD)
				pCD->m_Debug_ShowHud = (_Msg.m_Param0) ? true : false;

			return 1;
		}
#endif

	default :
		return CWObject_RPG::OnClientMessage(_pObj, _pWClient, _Msg);
	}
}


void CWObject_Character::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWObject_Character_OnClientNetMsg, MAUTOSTRIP_VOID);
	CWO_Character_ClientData *pCD = (_pObj) ? GetClientData(_pObj) : NULL;

	switch(_Msg.m_MsgType)
	{
/*	case PLAYER_NETMSG_SPAWNBLOOD:
		{
			// No object needed.
			if (_pWClient->GetClientMode() == WCLIENT_MODE_MIRROR) return;

#ifdef OLDBLOOD
			int P = 0;
			CVec3Dfp32 Pos = _Msg.GetVecInt16_Max32768(P);
			
			int Dmg = (uint8)_Msg.GetInt8(P);

			int iObj = _pWClient->ClientObject_Create("GibBlood", Pos);

			CWObject_CoreData* pObj = _pWClient->Object_Get(iObj);
			if (pObj)
			{
				pObj->m_iAnim0 = 0x08 + Min(Dmg >> 2, 20);
				pObj->m_iAnim1 = 0x02;
			}
#else
			int i = 0;
			CVec3Dfp32 Pos = _Msg.GetVecInt16_Max32768(i);
			CVec3Dfp32 Force = _Msg.GetVecInt16_Max32768(i);
			int Damage = (uint8)_Msg.GetInt8(i);

			CMat4Dfp32 Mat;
			Mat.Unit();
			(-Force).SetMatrixRow(Mat, 0);
			CVec3Dfp32(0, 1, 0).SetMatrixRow(Mat, 1);
			Mat.RecreateMatrix(0, 1);
			Pos.SetMatrixRow(Mat, 3);

			char DamageEffect[255];
			_Msg.GetStr(i, DamageEffect);

			int iObj = _pWClient->ClientObject_Create(DamageEffect, Mat);
			CWObject_CoreData* pObj = _pWClient->Object_Get(iObj);

			if (pObj)
			{
				pObj->m_iAnim0 = Force.Length();
				pObj->m_iAnim1 = Damage;
				
				if (pCD)
					pObj->m_Data[0] = pCD->m_BloodColor;
				else
					pObj->m_Data[0] = 0;
			}
#endif

			break;
		}

	case PLAYER_NETMSG_TILT :
		{
			// Requires object
			if (_pWClient->GetClientMode() == WCLIENT_MODE_MIRROR) return;
			if (!_pObj) return;
			if (!pCD) return;

			int P = 0;
			CVec3Dfp32 Angles;
			Angles[0] = fp32(_Msg.GetInt8(P)) / 1024;
			Angles[1] = fp32(_Msg.GetInt8(P)) / 1024;
			Angles[2] = fp32(_Msg.GetInt8(P)) / 1024;

			CMat4Dfp32 Mat;
			Angles.CreateMatrixFromAngles(0, Mat);
			CAxisRotfp32 AxisRot;
			AxisRot.Create(Mat);
//			AxisRot.Create(Angles[0], Angles[1], Angles[2]
			pCD->AddTilt(AxisRot.m_Axis, AxisRot.m_Angle);
		}
		break;*/

	case PLAYER_NETMSG_RUMBLE:
		{
			if(_pObj->m_iObject == _pWClient->Player_GetLocalObject())
			{
				MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
				if(pSys && pSys->GetOptions()->GetValuei("GAME_VIBRATION", 1, 1))
				{
					int Pos = 0;
					CStr Envelope = _Msg.GetStr(Pos);
//					if(_pWClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_GETGAMESTATE), _pWClient->Game_GetObjectIndex()) == CWObject_Mod::MOD_GAMESTATE_GAMEON)
					{
#ifdef PLATFORM_CONSOLE
						pSys->m_spInput->AppendEnvelope(0, Envelope);
#else
						//ConOut(CStr("Rumble: ") + Envelope);
#endif
					}
				}
			}
		}
		break;

	case PLAYER_NETMSG_SETTOKENHOLDER:
		{
			int Pos = 0;
			int iHolder = _Msg.GetInt16(Pos);
			int iSender = _Msg.GetInt16(Pos);
			Char_SetDialogueTokenHolder(_pObj, _pWClient, iHolder, iSender);
		}
		break;

	case PLAYER_NETMSG_SETDIALOGUECHOICES:
		{
			int Pos = 0;
			int nItems = _Msg.GetInt8(Pos);
			uint16 Buf[64];
			_Msg.GetData(Buf, nItems * 2, Pos);
			Char_SetDialogueChoices_Client(_pObj, _pWClient, Buf, nItems);
		}
		break;

	case PLAYER_NETMSG_CLEARTOKEN:
		if(pCD)
			pCD->m_PlayerDialogueToken.Clear();
		break;

	case PLAYER_NETMSG_HEALTHWOBBLE:
		{
			int Pos = 0;
			fp32 Amount = _Msg.Getfp32(Pos);
			if(pCD)
				pCD->m_HealthWobble += Amount;
		}
		break;

	case PLAYER_NETMSG_FLASHSCREEN:
		{
			if(pCD)
				pCD->m_ScreenFlashGameTime = CMTime::CreateFromTicks(pCD->m_GameTick, _pWClient->GetGameTickTime(), pCD->m_PredictFrameFrac);
		}
		break;

	case PLAYER_NETMSG_KILLVOICE:
		Voice_Stop(_pObj, _pWClient);
		if(pCD)
		{
			pCD->m_DialogueInstance.Reset(CMTime(), 0, 0.0f, 0);
			pCD->m_VoCap.StopVoice();
			/*memset(pCD->m_AnimGraph2.m_Dialogue_Once, 0, sizeof(pCD->m_AnimGraph2.m_Dialogue_Once));
			memset(pCD->m_AnimGraph2.m_Dialogue_Cur, 0, sizeof(pCD->m_AnimGraph2.m_Dialogue_Cur));*/
		}
		break;

	case PLAYER_NETMSG_RESET_POSTANIMSYSTEM:
		{
			if(pCD)
			{
				pCD->m_PostAnimSystem.ResetValues();
			}
		}
		break;

	case PLAYER_NETMSG_DECAL :
		{
			MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
			if (!pSC)
				break;

			int Pos = 0;
			CVec3Dfp32 LocalPos = _Msg.GetVecInt16_Max32768(Pos);
			CVec3Dfp32 Normal = _Msg.GetVecInt8_Max128(Pos);
			uint8 iNode = _Msg.GetInt8(Pos);
			uint8 iDecal = _Msg.GetInt8(Pos);
			uint8 Size = _Msg.GetInt8(Pos);
			int16 RotateDeg = _Msg.GetInt16(Pos);
			Normal.Normalize();

			CVec3Dfp32 BN;
			if (M_Fabs(Normal[0]) < 0.1f)
				BN = CVec3Dfp32(1,0,0);
			else
				BN = CVec3Dfp32(0,0,1);

			CMat4Dfp32 Origin;
			Origin.Unit();
			BN.SetRow(Origin, 0);
			Normal.SetRow(Origin, 2);
			Origin.RecreateMatrix(2, 0);
			LocalPos.SetRow(Origin, 3);

			// Rotate
			if (RotateDeg)
			{
				fp32 Rotate = (1.0f / 360.0f) * RotateDeg;
				Origin.RotZ_x_M(Rotate);
			}

			static const char* lpDecalSurf[] = { "holeskin01_01", "holeskin02_01", "holeskin03_01", "bruise01", "bruise02", "bruise03", "bruise04", "bruise05", "char_chesthole"  };
			int SurfaceID = pSC->GetSurfaceID(lpDecalSurf[iDecal]);

			CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
			if( pModel)
			{
				CXR_WallmarkDesc WMD;
				WMD.m_GUID = _pObj->m_iObject;
				WMD.m_iNode = iNode;
				WMD.m_Size = Size;
				WMD.m_SurfaceID = SurfaceID;		// <-- fix
				_pWClient->Wallmark_Create(pModel, WMD, Origin, 8.0f, 0);
			}

			pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[1]);
			if( pModel)
			{
				CXR_WallmarkDesc WMD;
				WMD.m_GUID = _pObj->m_iObject + 0x2000;
				WMD.m_iNode = iNode;
				WMD.m_Size = Size;
				WMD.m_SurfaceID = SurfaceID;		// <-- fix
				_pWClient->Wallmark_Create(pModel, WMD, Origin, 8.0f, 0);
			}

			pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[2]);
			if( pModel)
			{
				CXR_WallmarkDesc WMD;
				WMD.m_GUID = _pObj->m_iObject + 0x4000;
				WMD.m_iNode = iNode;
				WMD.m_Size = Size;
				WMD.m_SurfaceID = SurfaceID;		// <-- fix
				_pWClient->Wallmark_Create(pModel, WMD, Origin, 8.0f, 0);
			}
		}
		break;

	case NETMSG_PLAYDIALOGUE:
		{
			// Ugly code to handle the problem of time skip. The message is received just as a timeskip has ended,
			// and the player object GameTick is still from the beginning of the time-skip
			// To solve this, the GameTick is force-updated, so that the dialogue code updates correctly
			int Pos = 0;
			int Tick = _Msg.GetInt32(Pos);
			if(pCD && (pCD->m_GameTick - Tick) > 4)
			{
				pCD->m_GameTick = Tick;
				pCD->m_GameTime = PHYSSTATE_TICKS_TO_TIME(pCD->m_GameTick, _pWClient);
			}

			CWObject_RPG::OnClientNetMsg(_pObj, _pWClient, _Msg);
		}
		break;
	case PLAYER_NETMSG_SETCROUCHINPUT:
		{
			CWClient_Mod *pMod = safe_cast<CWClient_Mod>(_pWClient);
			if(!(pMod->m_Control_Press & CONTROLBITS_CROUCH))
				pMod->m_Control_Press |= CONTROLBITS_CROUCH;
		}
		
	case PLAYER_NETMSG_DISORIENTATION_FLASH:
		{
			if(pCD)
				pCD->m_Disorientation_Flash = 1.0f;
		}
		break;

	case PLAYER_NETMSG_DISORIENTATION_BLACKHOLE:
		{
			if(pCD)
			{
				pCD->m_Disorientation_Flash = MinMT((pCD->m_Disorientation_Flash * 2.0f) + 0.01f, 0.4f);
				//pCD->m_Disorientation_Flash = MinMT(pCD->m_Disorientation_Flash + 0.175f, 0.2f);
			}
		}
		break;

	case PLAYER_NETMSG_PLAYSOUND_REPEAT:
		{
			CMTime tNow = CMTime::GetCPU();//_pWClient->GetGameTime();
			fp32 TimeToLast = (pCD->m_PlayAhead_LastCreated - tNow).GetTime();

			if (_Msg.m_MsgSize > 0)
			{
				int Pos = 0;
				int16 iSound = _Msg.GetInt16(Pos);

				fp32 TickTime = _pWClient->GetGameTickTime();
				fp32 RepeatTime = _Msg.GetInt16(Pos) * TickTime;

				bool bLocalPlayer = (_pObj->m_iObject == _pWClient->Player_GetLocalObject());
				uint AttnType = bLocalPlayer ? WCLIENT_ATTENUATION_2D : WCLIENT_ATTENUATION_3D;
				CVec3Dfp32 Position = _pObj->GetPosition() + CVec3Dfp32(0,0,50);

				if (!(TimeToLast >= 0.0f && TimeToLast < 5.0f))
				{ // No play-ahead sounds created, so do it now
					//fp32 Delay = 0.0f;
					fp32 Delay = Max(0.0f,RepeatTime - (tNow - pCD->m_PlayAhead_LastCreated).GetTime());
					for (uint8 i = 0; i < 3; i++)	// TODO: Tweak num sounds to pre-start ?
					{
						pCD->m_lhPlayAheadVoices[i] = _pWClient->Sound_At(0, Position, iSound, AttnType, 0, 1.0f, 0.0f, Delay);
						Delay += RepeatTime;
					}
					pCD->m_PlayAhead_LastCreated = tNow + CMTime::CreateFromSeconds(Delay-RepeatTime);
				}
				else
				{ // Add one more sound
					fp32 Delay = TimeToLast + RepeatTime;
					// We need the absolute time from last time a sound was created + the repeat time
					Delay = Max(0.0f,RepeatTime - (tNow - pCD->m_PlayAhead_LastCreated).GetTime());
					pCD->m_lhPlayAheadVoices[0] = pCD->m_lhPlayAheadVoices[1];
					pCD->m_lhPlayAheadVoices[1] = pCD->m_lhPlayAheadVoices[2];
					pCD->m_lhPlayAheadVoices[2] = _pWClient->Sound_At(0, Position, iSound, AttnType, 0, 1.0f, 0.0f, Delay);
					// The last created sound was at current time + delay
					pCD->m_PlayAhead_LastCreated = tNow + CMTime::CreateFromSeconds(Delay);//CMTime::CreateFromSeconds(RepeatTime);
				}
			}
			else
			{ // Remove all pre-created sounds
				for (uint i = 1; i < 3; i++)
				{
					if (pCD->m_lhPlayAheadVoices[i])
					{
						_pWClient->Sound_SetVolume(pCD->m_lhPlayAheadVoices[i], 0.0f);
						pCD->m_lhPlayAheadVoices[i] = 0;
					}
				}
				pCD->m_PlayAhead_LastCreated = tNow;
			}
		}
		break;

	default :
		CWObject_RPG::OnClientNetMsg(_pObj, _pWClient, _Msg);
	}
}

bool CWObject_Character::AI_IsSpawned()
{
	if (!(m_Flags & PLAYER_FLAGS_WAITSPAWN))
		return true;
	else
		return false;
};

//was: OBJMSG_AIQUERY_ISALIVE
bool CWObject_Character::AI_IsAlive()
{
	//Character counts as "alive" if not dead or no-clipping
	if (!(Char_GetPhysType(this) == PLAYER_PHYS_DEAD) &&
		!(Char_GetPhysType(this) == PLAYER_PHYS_NOCLIP) &&
		!(m_Flags & PLAYER_FLAGS_WAITSPAWN))
		return true;
	else
		return false;
}


//was: OBJMSG_AIQUERY_GETAI
CAI_Core* CWObject_Character::AI_GetAI()
{
	MSCOPESHORT(CWObject_Character::AI_GetAI);
	return m_spAI;
}


//was: OBJMSG_AIQUERY_GETRPGITEM
CRPG_Object_Item* CWObject_Character::AI_GetRPGItem(int _EquipPlace)
{
	MSCOPESHORT(CWObject_Character::AI_GetRPGItem);
	CRPG_Object_Char* pChar = Char();
	if (pChar)
		return pChar->GetEquippedItem(_EquipPlace);
	return NULL;
}


//was: OBJMSG_AIQUERY_GETRPGINVENTORY
CRPG_Object_Inventory* CWObject_Character::AI_GetRPGInventory(int _iSlot)
{
	MSCOPESHORT(CWObject_Character::AI_GetRPGInventory);
	CRPG_Object_Char* pChar = Char();
	if (pChar)
		return pChar->GetInventory(_iSlot);
	return NULL;
}

//was: OBJMSG_AIQUERY_GETMAXSPEED
fp32 CWObject_Character::AI_GetMaxSpeed(int _Speed)
{
	MSCOPESHORT(CWObject_Character::AI_GetMaxSpeed);
	const CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	M_ASSERT(pCD, "Must exist here. Code error!");

	switch (_Speed)
	{
	case AIQUERY_GETMAXSPEED_FORWARD:
		return pCD->m_Speed_Forward;

	case AIQUERY_GETMAXSPEED_SIDESTEP:
		return pCD->m_Speed_SideStep;

	case AIQUERY_GETMAXSPEED_JUMP:
		return pCD->m_Speed_Jump;

	case AIQUERY_GETMAXSPEED_UP:
	case AIQUERY_GETMAXSPEED_DOWN:
		return pCD->m_Speed_Up;

	case AIQUERY_GETMAXSPEED_WALK:
		return 1.2; //Hardcoding! FIX (won't matter when we get animation-controlled physics though...)

	default:
		return pCD->m_Speed_Forward;
	}
}
/*
void CWObject_Character::AI_GetLookMatrix(CMat4Dfp32& _RetValue)
{
	MSCOPESHORT(CWObject_Character::AI_GetLookMatrix);

	//Uh... the below scope seems completely useless. We only return position anyway...
	CMat4Dfp32 Mat = GetPositionMatrix();
	CWO_Character_ClientData *pCD = GetClientData(this);
	if (pCD && !IsBot())
	{
		pCD->Target_GetAimMatrix(Mat);
		CVec3Dfp32::GetRow(Mat, 3) = GetPosition();
	}

	//Use template values instead of Camera_GetHeadPos
	fp32 AimOffset; //Z-offset from base
	int PhysType = Char_GetPhysType(this);
	if (PhysType == PLAYER_PHYS_NOCLIP)
		AimOffset = 0;
	else if (PhysType == PLAYER_PHYS_CROUCH)
		AimOffset = m_AICrouchAimOffset;
	else
		AimOffset = m_AIAimOffset;
	CVec3Dfp32::GetMatrixRow(Mat,3) += CVec3Dfp32(0, 0, AimOffset);
	_RetValue = Mat;
};

void CWObject_Character::AI_GetAimPosition(CVec3Dfp32& _RetValue)
{
	MSCOPESHORT(CWObject_Character::AI_GetAimPosition);
	//const CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
	//M_ASSERT(pCD, "Must exist here. Code error!");

	//CVec3Dfp32* Pos = (CVec3Dfp32*)_Msg.m_pData;

	//Uh... the below scope seems completely useless. We only return position anyway...
	CMat4Dfp32 Mat = GetPositionMatrix();
	CWO_Character_ClientData *pCD = GetClientData(this);
	if (pCD && !IsBot())
	{
		pCD->Target_GetAimMatrix(Mat);
		CVec3Dfp32::GetRow(Mat, 3) = GetPosition();
	}

	//Use template values instead of Camera_GetHeadPos
	fp32 AimOffset; //Z-offset from base
	int PhysType = Char_GetPhysType(this);
	if (PhysType == PLAYER_PHYS_NOCLIP)
		AimOffset = 0;
	else if (PhysType == PLAYER_PHYS_CROUCH)
		AimOffset = m_AICrouchAimOffset;
	else
		AimOffset = m_AIAimOffset;
	_RetValue = CVec3Dfp32::GetMatrixRow(Mat, 3) + CVec3Dfp32(0, 0, AimOffset);
//	_RetValue = CVec3Dfp32::GetMatrixRow(Mat, 3) + Camera_GetHeadPos(this);
//	_RetValue = GetPosition() + Camera_GetHeadPos(this);
}
*/
/*
//MSCOPESHORT(MESSAGEGROUP_AI);
			//Aim position is "camera position" for bots or aim matrix for players
			CVec3Dfp32* Pos = (CVec3Dfp32*)_Msg.m_pData;

			CMat4Dfp32 Mat = GetPositionMatrix();
			CWO_Character_ClientData *pCD = GetClientData(this);
			if (pCD && !IsBot())
			{
				pCD->Target_GetAimMatrix(Mat);
				CVec3Dfp32::GetRow(Mat, 3) = GetPosition();
			}

			*Pos = CVec3Dfp32::GetMatrixRow(Mat, 3) + Camera_GetHeadPos(this);
			*/

//was: OBJMSG_CHAR_ISBOT
bool CWObject_Character::AI_IsBot()
{
	MSCOPESHORT(CWObject_Character::AI_IsBot);
	return IsBot();
}

//was: OBJMSG_CHAR_ISWEIGHTLESS
bool CWObject_Character::AI_IsWeightLess()
{
	return (m_ClientFlags & PLAYER_CLIENTFLAGS_WEIGHTLESS) != 0;
}


//Get physics influencing flags (immune, immobile etc)
uint16 CWObject_Character::AI_GetPhysFlags()
{
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	M_ASSERT(pCD, "No clientdata!");
	return pCD->m_Phys_Flags;
} 


//Get the number of teams the agent belongs to and add the team indices to the given list
uint CWObject_Character::AI_GetTeams(uint16* _piResult, uint _MaxElem) const
{
	TArrayPtr<const uint16> piTeams = m_liTeams;
	uint n = Min(_MaxElem, (uint)piTeams.Len());

	for (uint i = 0; i < n; i++)
		_piResult[i] = piTeams[i];
	return n;
}


//Return value of given rpg attribute, with given params, see enum in ai_def.h.
int CWObject_Character::AI_GetRPGAttribute(int _Attribute, int _Param1)
{
	MSCOPESHORT(CWObject_Character::AI_GetRPGAttribute);
	CRPG_Object_Char* pChar = Char();
	if (pChar)
	{
		switch (_Attribute)
		{
		case AIQUERY_RPGATTRIBUTE_HEALTH:
			{
				return pChar->Health();
			}
		case AIQUERY_RPGATTRIBUTE_MAXHEALTH:
			{
				return pChar->MaxHealth();
			}
		case AIQUERY_RPGATTRIBUTE_WAIT:
			{
				return pChar->Wait();
			}
		case AIQUERY_RPGATTRIBUTE_MANA:
			{
				if (pChar)
				{
					CRPG_Object_Ammo *pAmmo = pChar->GetAssociatedAmmo();
					if(!pAmmo)
						return 0;
					return pAmmo->m_NumItems;
				}
				else
					return 0;
			}
		case AIQUERY_RPGATTRIBUTE_MAXMANA:
			{
				if (pChar && pChar->GetAssociatedAmmo())
				{
					return pChar->GetAssociatedAmmo()->GetMaxTotal();
				}
				else
					return 0;
			}
		case AIQUERY_RPGATTRIBUTE_AMMO:
			{
				if (pChar)
				{
					CRPG_Object_Ammo *pAmmo = pChar->GetAssociatedAmmo(_Param1);
					if(!pAmmo)
						//Assume this means that weapon doesn't draw ammo... maybe wrong...
						return -1;
					else 
						return pAmmo->GetNumTotal();
				}
				else
					return 0;
			}
		case AIQUERY_RPGATTRIBUTE_LOADEDAMMO:
			{
				if (pChar)
				{
					CRPG_Object_Ammo *pAmmo = pChar->GetAssociatedAmmo(_Param1);
					if(!pAmmo)
						//Assume this means that weapon doesn't draw ammo... maybe wrong...
						return -1;
					else
						return pAmmo->GetNumLoaded();
				}
				else
					return 0;
			}
		}
	}
	return 0;
}

//Return position [matrix] of camera last frame
CMat4Dfp32 CWObject_Character::AI_GetLastCamera()
{
	return m_LastCamera;
};			
CVec3Dfp32 CWObject_Character::AI_GetLastCameraPos()
{
	return CVec3Dfp32::GetRow(m_LastCamera, 3);
};		

//Get controlmode (PLAYER_CONTROLMODE_XXX)
int CWObject_Character::AI_GetControlMode()
{
	return Char_GetControlMode(this);
};

//Set animation controlmode on/off
void CWObject_Character::AI_SetAnimControlMode(bool _bOn)
{
	CWO_Character_ClientData* pCD = GetClientData(this);
	if (pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_DONTTOUCHCONTROLMODE)
		return;
 	if (_bOn)
	{
		//Turn on anim control
		if (Char_GetControlMode(this) != PLAYER_CONTROLMODE_ANIMATION)
			Char_SetControlMode(this, PLAYER_CONTROLMODE_ANIMATION);
	}
	else
	{
		//Switch off anim control
		if (Char_GetControlMode(this) == PLAYER_CONTROLMODE_ANIMATION)
			Char_SetControlMode(this, PLAYER_CONTROLMODE_FREE);
	}
};


// Same as CAI_Core::GetSingleTarget
CWObject* CWObject_Character::AI_GetSingleTarget(int _iObject, CStr _pName, int _iSender)
{
	CWObject* pObject = NULL;

	//If we have reserved name always use the corresponding object
	int iSpecialTarget = CWO_SimpleMessage::ResolveSpecialTargetName(_pName);
	if (iSpecialTarget >= 0)
	{
		if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_ACTIVATOR)
		{
			//Use activator, which should be the supplied onject ID
			return m_pWServer->Object_Get(_iObject);
		}
		else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_THIS)
		{
			//Use sender
			return m_pWServer->Object_Get(_iSender);
		}
		else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_PLAYER)
		{
			//Find closest player to sending object (if any)
			fp32 DistSqr;
			fp32 MinDistSqr = _FP32_MAX;
			CWObject * pChar;
			CWObject * pMinChar = NULL;
			CWObject_Game *pGame = m_pWServer->Game_GetObject();
			for (int i = 0; i < pGame->Player_GetNum(); i++)
			{
				if ((pChar = pGame->Player_GetObject(i)) &&											
					((DistSqr = GetPosition().DistanceSqr(pChar->GetPosition())) < MinDistSqr))	
					{
						pMinChar = pChar;
						MinDistSqr = DistSqr;
					}
			};
			return pMinChar;
		}
		else if (iSpecialTarget == CWO_SimpleMessage::SPECIAL_TARGET_ALLPLAYERS)
		{
			//Get random player
			CWObject * pChar;
			CWObject_Game *pGame = m_pWServer->Game_GetObject();
			int iRnd = (int)(Random * 0.999f * pGame->Player_GetNum());
			for (int i = 0; i < pGame->Player_GetNum(); i++)
			{
				if ((pChar = pGame->Player_GetObject((i + iRnd) % pGame->Player_GetNum())))
				{
					return pChar;
				};
			};
			return NULL;
		}
		else
		{
			//Can't handle this name
			return NULL;
		};
	}
	//Otherwise use target name if this is valid
	else if (_pName.Str())
	{
		//Find all objects with the given name
		const int16* piObj;
		int nObj;

		//Set up selection
		TSelection<CSelection::LARGE_BUFFER> Selection;

		//Add objects
		m_pWServer->Selection_AddTarget(Selection, _pName);
		nObj = m_pWServer->Selection_Get(Selection, &piObj);

		//Shuffle through objects until best random candidate is found
		if (nObj == 0)
			return NULL;
		else if (nObj == 1)
			return m_pWServer->Object_Get(piObj[0]);
		else
		{
			//More than one object, settle for the first valid character that is found by a randomized
			//search or the first valid object otherwise

			//Set up check array
			TArray<bool> lbChecked;
			lbChecked.QuickSetLen(nObj);
			int i;
			for (i = 0; i < nObj; i++) lbChecked[i] = false;

			//Start searching
			CWObject * pChar = NULL;
			int Rnd;
			int j = 0;
			for (i = nObj; i > 0; i--)
			{
				//Find random non-checked object index
				Rnd = (int)(Random * i * 0.9999f);
				j = 0;
				while (j <= Rnd)
				{
					if (lbChecked[j])
						Rnd++;
					j++;
				};

				//Check if we've got a valid character
				if ((m_spAI) && (pChar = m_spAI->IsValidAgent(piObj[Rnd])))
				{
					//First valid character found, we're done!
					return pChar;
				}
				else
				{
					return pChar;
				}

				//Should we get the object?
				if (!pObject)
					pObject = m_pWServer->Object_Get(piObj[Rnd]);
			};
			return pObject;
		};
	}
	//Try given object ID
	else 
		return m_pWServer->Object_Get(_iObject);
};

int CWObject_Character::AI_GetActivationState()
{
	if( m_spAI )
		return m_spAI->m_ActivationState;

	return 0;
}

void CWObject_Character::AI_SetActivationState(int _State)
{
	if( m_spAI )
		m_spAI->m_ActivationState = _State;
}


// Get character position matrix, but without the look-angles merged into it..
void CWObject_Character::AI_GetBaseMat(CMat4Dfp32& _RetValue)
{
	_RetValue = m_BaseMat;
}

CVec3Dfp32 CWObject_Character::AI_GetBasePos()
{
	return m_BaseMat.GetRow(3);
}


// Position + direction of weapon muzzle 
void CWObject_Character::AI_GetWeaponMat(CMat4Dfp32& _RetValue)
{
	if (!m_bHeadMatUpdated)
	{
		OnRefreshSkeletonMats();
	}
	_RetValue = m_WeaponMat;
}

CVec3Dfp32 CWObject_Character::AI_GetWeaponPos()
{
	if (!m_bHeadMatUpdated)
	{
		OnRefreshSkeletonMats();
	}
	return m_WeaponMat.GetRow(3);
}


// Get position of eyes + direction of look
void CWObject_Character::AI_GetHeadMat(CMat4Dfp32& _RetValue)
{
	if (!m_bHeadMatUpdated)
	{
		OnRefreshSkeletonMats();
	}
	_RetValue = m_HeadMat;
}

CVec3Dfp32 CWObject_Character::AI_GetHeadPos()
{
	if (!m_bHeadMatUpdated)
	{
		OnRefreshSkeletonMats();
	}
	return(m_HeadMat.GetRow(3));

	/*
	CXR_AnimState Anim;
	CXR_Skeleton* pSkel;
	CXR_SkeletonInstance* pSkelInstance;
	if (CWObject_Character::GetEvaluatedPhysSkeleton((CWObject_Character*)this, m_pWServer, pSkelInstance, pSkel, Anim))
	{
		if (pSkelInstance->m_nBoneTransform >= PLAYER_ROTTRACK_HEAD)
		{
			const CMat4Dfp32& Mat = pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_HEAD];
			CVec3Dfp32 tmp;
			tmp = pSkel->m_lNodes[PLAYER_ROTTRACK_HEAD].m_LocalCenter;

			const CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if (pCD->m_iPlayer != -1)
				tmp +=  CVec3Dfp32(3.5f, -1.0f, 2.5f); // tweak offset for "camera" position

			if (!(Mat.k[0][0] > -1.1f && Mat.k[0][0] < 1.1f))
				tmp *= GetPositionMatrix(); // fallback for non-existing (broken) head-track
			else
				tmp *= Mat;
			return tmp;
		}
	}
	// crappy fallback:
	return Camera_GetHeadPos(this);
	*/
}


// Get activate position of agent
void CWObject_Character::AI_GetActivateMat(CMat4Dfp32& _RetValue)
{
	GetActivatePosition(_RetValue);
}

CVec3Dfp32 CWObject_Character::AI_GetActivatePos()
{
	CMat4Dfp32 Tmp;  // this is stupid
	GetActivatePosition(Tmp);
	return Tmp.GetRow(3);
}


/*
//was: OBJMSG_AIQUERY_GETVULNERABLEPOS
void CWObject_Character::AI_GetVulnerablePos(CVec3Dfp32& _RetValue)
{
	MSCOPESHORT(CWObject_Character::AI_GetVulnerablePos);
	//Get position above bounding box center at height of face attachpoint 
	//or activate position if no face attachpoint can be found
	
	CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
	CXR_Skeleton* pSkel;
	const CXR_SkeletonAttachPoint * pPoint;
	if (pModel &&
		(pSkel = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON)) &&
		(pPoint = pSkel->GetAttachPoint(PLAYER_ATTACHPOINT_FACE)))
	{
		_RetValue = GetPosition();
		_RetValue.k[2] += pPoint->m_LocalPos.k[2];
	}
	else
	{
		//Couldn't get face attach point, use point at 3/4 of height
		CWO_Character_ClientData * pCD = CWObject_Character::GetClientData(this);
		M_ASSERT(pCD, "Should exist here. Fix code error!");
		if (pCD)
			_RetValue = GetPosition() + CVec3Dfp32(0, 0, pCD->m_Phys_Height * 0.75f);
	}
}

//was: OBJMSG_CHAR_NOISE
fp32 CWObject_Character::AI_GetNoise()
{
	MSCOPESHORT(CWObject_Character::AI_GetNoise);
	//Default value (1.0f) is for a character standing still (without holding his breath
	//or otherwise trying to stay noiseless). A running character would therefore have a 
	//value of around 50 and firing a gun would be much louder still. Completely noiseless
	//is of course 0.

	return 75.0f;
}

//was: OBJMSG_CHAR_VISIBILITY
fp32 CWObject_Character::AI_GetVisibility()
{
	MSCOPESHORT(CWObject_Character::AI_GetVisibility);
	//Default value (1.0f) is for a character standing still in good lighting conditions
	//A moving character will have a slightly higher value while a guy in a Predator suit
	//would have visibility 0.1f or lower. A guy carrying a flare might have a value of 
	//around 20. Completely invisible is of course 0.
	
	//The below is a hack. Fix properly.
	fp32 Res = 1.0f;

	//Visibility is modified by speed (easier to spot moving objects)...
	fp32 SpeedSqr = (GetPosition()).DistanceSqr(GetLastPosition());
	Res *= (SpeedSqr < 1*1) ? 1 : ((SpeedSqr < 4*4) ? 2 : 5);

	//If a character has recently used a weapon or item (or switched such) the "report"
	//Causes him to become more visible. This should be specified per each weapon...
	if (Char() && false)//Char()->Wait())
		Res *= 2;

	//Should check how dark/foggy etc it is at character's position 
	//and modify visibility accordingly

	//	ConOut(CStrF("Character visibility: %f", Res));
	return Res;
}

//was: OBJMSG_CHAR_GETCONTROLMODE
int CWObject_Character::AI_GetControlMode()
{
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if (pCD == NULL)
		return 0;

	CWAGI_Context AGIContext(this, m_pWServer, pCD->GetGameTime(), pCD->GetTimeSpan());

	if (pCD->StateGroup_IsInBowMode(&AGIContext))
		return AI_CONTROLMODE_BOW;

	if (pCD->StateGroup_IsInFightMode(&AGIContext))
	{
		if (pCD->GetTargetID() != 0)
			return AI_CONTROLMODE_FIGHT;
		else
			return AI_CONTROLMODE_EXPLORE;
	}

	return AI_CONTROLMODE_EVENT;
}

//Get AI rank
int CWObject_Character::AI_GetRank()
{
	if (m_spAI)
		return m_spAI->m_Rank;
	else
		return CAI_Core::MIN_RANK;
};



//Notifies object of preferred movement mode.
void CWObject_Character::AI_SetAnimGraphMode(uint8 _Mode)
{
	CWO_Character_ClientData * pCD = CWObject_Character::GetClientData(this);
	if (pCD)
	{
		pCD->SetAIMode(_Mode);
	}
}

	
*/

void CWObject_Character::OnCharAnimEvent(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD, class CXR_Anim_DataKey* _pKey)
{
	if (!_pKey || !_pCD)
		return;

	switch (_pKey->m_Type)
	{
	case ANIM_EVENT_TYPE_GAMEPLAY:
		switch (_pKey->m_Param)
		{
		case AG2_ANIMEVENT_GAMEPLAY_WEAPON_FIRE:
			// Quick'n'dirty I guess?
			OnMessage(CWObject_Message(OBJMSG_CHAR_ACTIVATEITEM));
			break;

		case AG2_ANIMEVENT_GAMEPLAY_WEAPON_RELOAD:
			OnMessage(CWObject_Message(OBJMSG_CHAR_RELOADITEM));
			break;

		case AG2_ANIMEVENT_GAMEPLAY_WEAPONATTACH:
			{
				CFStr Data = _pKey->Data();
				int iAttachPoint = Data.GetStrSep(",").Val_int();
				if (iAttachPoint >= 0 && iAttachPoint < 10)
				{
					int iItem = Data.Val_int();
					if (iItem == 0)
					{
						_pCD->m_Item0_Model.m_iAttachPoint[0] = iAttachPoint;
						_pCD->m_Item0_Model.MakeDirty();
					}
					else
					{
						_pCD->m_Item1_Model.m_iAttachPoint[0] = iAttachPoint;
						_pCD->m_Item1_Model.MakeDirty();
					}
				}
				break;
			}
		case AG2_ANIMEVENT_GAMEPLAY_WEAPON_FIRESECONDARY:
			// Quick'n'dirty I guess?
			OnMessage(CWObject_Message(OBJMSG_CHAR_ACTIVATEITEM,2));
			break;

		case AG2_ANIMEVENT_GAMEPLAY_WEAPON_KILLYOURSELF:
			// Send message to ai to kill himself
			m_spAI->OnMessage(CWObject_Message(OBJMSG_CHAR_ANIMEVENT_DEATH));
			break;

		case AG2_ANIMEVENT_GAMEPLAY_WEAPON_FAKEPRIMARY:
			// Quick'n'dirty I guess?
			OnMessage(CWObject_Message(OBJMSG_CHAR_ACTIVATEITEM,0,3));
			break;

		case AG2_ANIMEVENT_GAMEPLAY_WEAPON_FAKESECONDARY:
			OnMessage(CWObject_Message(OBJMSG_CHAR_ACTIVATEITEM,2,3));
			break;

		case AG2_ANIMEVENT_GAMEPLAY_BLOODATROTTRACK:
			{
				CStr KeyData = _pKey->Data();
				int iRotTrack = KeyData.GetStrSep(",").Val_int();
				CVec3Dfp32 Direction = GetPositionMatrix().GetRow(0);
				CVec3Dfp32 Offset(0.0f);
				Offset.ParseString(KeyData);
				Offset.MultiplyMatrix3x3(GetPositionMatrix());
				Char_DoBloodAtAttach(iRotTrack,-1,Direction,Offset);
				//ConOut(CStrF("Tick: %d iRot: %d",m_pWServer->GetGameTick(),iRotTrack));
				break;
			}

		case AG2_ANIMEVENT_GAMEPLAY_WEAPON_FAKEPRIMARYNOSOUND:
			// Quick'n'dirty I guess?
			OnMessage(CWObject_Message(OBJMSG_CHAR_ACTIVATEITEM,0,5));
			break;

		case AG2_ANIMEVENT_GAMEPLAY_WEAPON_FAKESECONDARYNOSOUND:
			OnMessage(CWObject_Message(OBJMSG_CHAR_ACTIVATEITEM,2,5));
			break;

		case AG2_ANIMEVENT_GAMEPLAY_WEAPON_FORCEDROPPRIMARY:
			OnMessage(CWObject_Message(OBJMSG_CHAR_FORCEDROPWEAPON,0));
			break;

		case AG2_ANIMEVENT_GAMEPLAY_WEAPON_FORCEDROPSECONDARY:
			OnMessage(CWObject_Message(OBJMSG_CHAR_FORCEDROPWEAPON,1));
			break;

		case AG2_ANIMEVENT_GAMEPLAY_WEAPON_FORCEMAKEOBJECTPRIMARY:
			OnMessage(CWObject_Message(OBJMSG_CHAR_CREATEGRABBABLEITEMOBJECT,0,0,-1,1));
			break;

		case AG2_ANIMEVENT_GAMEPLAY_WEAPON_FORCEMAKEOBJECTSECONDARY:
			OnMessage(CWObject_Message(OBJMSG_CHAR_CREATEGRABBABLEITEMOBJECT,0,1,-1,1));
			break;

		case AG2_ANIMEVENT_GAMEPLAY_WEAPON_BERAGDOLL:
			Char_BeRagDoll(true);
			break;

		case AG2_ANIMEVENT_GAMEPLAY_WEAPON_MELEE:
			OnMessage(CWObject_Message(OBJMSG_CHAR_ACTIVATEITEM,0,2));
			break;

		default:
			break;
		}
		break;
	//end ANIM_EVENT_TYPE_GAMEPLAY

	case ANIM_EVENT_TYPE_EFFECT:
		{
			CMTime EventTime = (_Msg.m_pData && _Msg.m_DataSize >= sizeof(CMTime) ? *(CMTime*)_Msg.m_pData : _pCD->m_GameTime);
			CStr KeyData(_pKey->Data());
			if (_pKey->m_Param == ANIM_EVENT_EFFECTTYPE_WEAPON)
			{
			}
			else if (_pKey->m_Param == 1)
			{
				KeyData.MakeUpperCase();
				CStr EffectStr = KeyData.GetStrSep(",");

				// Do we want to emit a shell ?
				//ConOut(CStrF("Emit shell! Tick: %d Time: %f", _pCD->m_GameTick,CMTime::CreateFromTicks(_pCD->m_GameTick,m_pWServer->GetGameTickTime()).GetTime()));
				if(EffectStr == "EMITSHELL")
				{
					CWO_ShellManager* pShellMgr = (CWO_ShellManager*)m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETSHELLMANAGER), m_pWServer->Game_GetObjectIndex());
					if (_Msg.m_Reason == TOKEN_WEAPON1 && pShellMgr)
					{
						CWO_Shell_SpawnData SpawnData;
						CRPG_Object_Item* pItem = Char()->GetEquippedItem(0);
						if(pItem)
						{
							pItem->GetShellSpawnData(m_iObject, &SpawnData);
							pShellMgr->SpawnShell(m_iObject, pItem->m_iShellType, SpawnData);
						}
					}
					else if(_Msg.m_Reason == TOKEN_WEAPON2 && pShellMgr)
					{
						CWO_Shell_SpawnData SpawnData;
						CRPG_Object_Item* pItem = Char()->GetEquippedItem(2);
						if(pItem)
						{
							pItem->GetShellSpawnData(m_iObject, &SpawnData);
							pShellMgr->SpawnShell(m_iObject, pItem->m_iShellType, SpawnData);
						}
					}
				}
				else if (EffectStr == "ACTIVATEWEAPON")
				{
					// Activate specific weapon in the inventory
					CStr Item = KeyData.GetStrSep(",");
					int32 ItemSlot = KeyData.GetStrSep(",").Val_int();
					CRPG_Object_Item* pCurItem = Char()->GetEquippedItem(ItemSlot);
					CRPG_Object_Item* pTargetItem = Char()->GetInventory(ItemSlot)->FindItemByName(Item);
					if (pCurItem && pTargetItem)
					{
						if (pCurItem != pTargetItem)
						{
							// Force activate and then change back to the other item
							ForceActivateItem(pTargetItem->m_iItemType);
							ForceSelectItem(pCurItem->m_iItemType);
						}
						else
						{
							// Just activate item...
							OnMessage(CWObject_Message(OBJMSG_CHAR_ACTIVATEITEM,ItemSlot));
						}
					}
				}
			}
		}
		break;
	//end ANIM_EVENT_TYPE_EFFECT

	case ANIM_EVENT_TYPE_AISCRIPT:
		{
			// Send event messages to the aiscript listener
			CWObject_Message Msg(CWObject_AnimEventListener::OBJECT_ANIMEVENTLISTENER_MESSAGE_ONEVENT,_pKey->m_Param);
			for (int32 i = 0; i < m_liAnimEventEntities.Len(); i++)
			{
				m_pWServer->Message_SendToObject(Msg,m_liAnimEventEntities[i]);
			}
			// Also send the event to the ai (via char)
			m_spAI->OnMessage(Msg);
		}
		break;
	//end ANIM_EVENT_TYPE_AISCRIPT

	case ANIM_EVENT_TYPE_SETSOUND:
		{
			CWObject_Message Msg(OBJSYSMSG_SETSOUND, _pKey->m_Param);
			Msg.m_pData = _pKey->Data();
			Msg.m_DataSize = _pKey->DataSize();
			OnMessage(Msg);
		}
		break;
	case ANIM_EVENT_TYPE_DIALOGUEIMPULSE:
		{
			Char_SendDialogueImpulse(_pKey->m_Param);
			break;
		}
	case ANIM_EVENT_TYPE_ANIMIMPULSE:
		{
			CWObject_Message Msg(OBJMSG_CHAR_ANIMIMPULSE,6);
			Msg.m_pData = _pKey->Data();
			Msg.m_DataSize = _pKey->DataSize();
			OnCharAnimImpulse(Msg,_pCD,_pCD->m_AnimGraph2.GetAG2I());
			break;
		}
	case ANIM_EVENT_TYPE_WEAPONIMPULSE:
		{
			CWObject_Message Msg(OBJMSG_CHAR_ANIMIMPULSE,6);
			Msg.m_pData = _pKey->Data();
			Msg.m_DataSize = _pKey->DataSize();
			OnCharAnimImpulse(Msg,_pCD,_pCD->m_WeaponAG2.GetAG2I());
			break;
		}
	//end ANIM_EVENT_TYPE_SETSOUND 
	}
}

aint CWObject_Character::OnCharForceDropWeapon(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD)
{
	// Should only be done if char is "almost" dead (ie breakneck...)
	if (!_pCD)
		return false;

	//ConOut("FORCE DROPPING WEAPON");

	CRPG_Object_Item *pItem = NULL;
	//CRPG_Object_Item *pItem2 = NULL;

	/*if (_Msg.m_Param != 0)
		pItem = Char()->FindItemByIdentifier(_Msg.m_Param0,_Msg.m_Param1);
	else*/
	pItem = Char()->GetEquippedItem(_Msg.m_Param0);

	// Copied from Char_Die
	if (pItem && !(pItem->m_Flags2 & RPG_ITEM_FLAGS2_PERMANENT) && !(pItem->m_Flags & RPG_ITEM_FLAGS_NOPICKUP))// && (pItem->m_AnimProperty > 1))
	{
		/*if (pItem->m_Flags2 & RPG_ITEM_FLAGS2_CLONEATTACHITEM)
		{
			// Find cloned item
			pItem2 = Char()->GetEquippedItem(AG2_ITEMSLOT_DUALWIELD);
		}*/
		CXR_AnimState Anim;
		CXR_Skeleton* pSkel;
		CXR_SkeletonInstance* pSkelInstance;					
		if(GetEvaluatedPhysSkeleton(pSkelInstance, pSkel, Anim))
		{
			CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
			if (!pModel)
				return false;

			CMat4Dfp32 Mat;
			CMTime GameTime = CMTime::CreateFromTicks(_pCD->m_GameTick, m_pWServer->GetGameTickTime(), _pCD->m_PredictFrameFrac);
			if (!_Msg.m_Param0)
			{
				if(_pCD->m_Item0_Model.IsValid() && _pCD->m_Item0_Model.GetModel0_RenderInfo(m_pWServer->GetMapData(), NULL, pSkelInstance, pSkel, GameTime, 
					Anim, Mat, pModel, m_pWServer))
				{
					m_spRagdollDropItems[0] = MNew(CConstraintRigidObject);
					m_spRagdollDropItems[0]->SetOrgMat(Mat);
					/*if (_pCD->m_iPlayer != -1)
					{
						CVec3Dfp32& Pos = CVec3Dfp32::GetMatrixRow(Mat,3);
						Pos.k[2] += 5.0f;
					}*/
					CXR_Model *pItemModel = m_pWServer->GetMapData()->GetResource_Model(_pCD->m_Item0_Model.m_iModel[0]);
					SConstraintSystemSettings Settings;
					Settings.m_SkeletonType	= SConstraintSystemSettings::RAGDOLL_NONE;
					m_spRagdollDropItems[0]->SetCollisionSound(pItem->m_iSound_Dropped,false);
					m_spRagdollDropItems[0]->StartCollecting(m_pWServer, pItemModel, &Settings);
					m_spRagdollDropItems[0]->Activate(true);
					m_spRagdollDropItems[0]->Animate(m_pWServer->GetGameTick(), Mat);
					if (_pCD->m_iPlayer != -1)
					{
						CVec3Dfp32 ImpulsePos = GetPosition();
						ImpulsePos.k[2] += 30.0f;
						m_spRagdollDropItems[0]->AddPendingImpulse(GetPosition(), CVec3Dfp32(0,0,2.0f) -CVec3Dfp32::GetMatrixRow(GetPositionMatrix(),1));
					}
				}

				int iObj = m_pWServer->Object_Create("DroppableItem", Mat);
				if(iObj > 0)
				{
					m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENEPICKUP_SETDEFAULTS), iObj);

					int nMessages;
					const CWO_SimpleMessage* pMsg = m_MsgContainer.GetMessages(PLAYER_SIMPLEMESSAGE_ONPICKUPDROPWEAPON, nMessages);
					CWObject_Message Msg(OBJMSG_RPG_REPLACEPICKUP, nMessages, aint(pMsg));
					Msg.m_pData = (CRPG_Object *)pItem;
					m_pWServer->Message_SendToObject(Msg, iObj);
					m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_RPG_SETRIGIDBODY, (aint)((CConstraintRigidObject*) m_spRagdollDropItems[0])), iObj); 
					CWObject *pObj = m_pWServer->Object_Get(iObj);
					if(pObj)
						pObj->m_Param = pItem->GetAmmo(Char()) | 0x8000;
				}
				Char()->RemoveItemByIdentifier(m_iObject,pItem->m_Identifier,_Msg.m_Param0);
				_pCD->m_Item0_Model.Clear();
				_pCD->m_Item0_Model.MakeDirty();
			}
			else
			{
				if(_pCD->m_Item1_Model.IsValid() && _pCD->m_Item1_Model.GetModel0_RenderInfo(m_pWServer->GetMapData(), NULL, pSkelInstance, pSkel, GameTime, 
					Anim, Mat, pModel, m_pWServer))
				{
					m_spRagdollDropItems[1] = MNew(CConstraintRigidObject);
					m_spRagdollDropItems[1]->SetOrgMat(Mat);
					/*if (_pCD->m_iPlayer != -1)
					{
						CVec3Dfp32& Pos = CVec3Dfp32::GetMatrixRow(Mat,3);
						Pos.k[2] += 5.0f;
					}*/
					CXR_Model *pItemModel = m_pWServer->GetMapData()->GetResource_Model(_pCD->m_Item1_Model.m_iModel[0]);
					SConstraintSystemSettings Settings;
					Settings.m_SkeletonType	= SConstraintSystemSettings::RAGDOLL_NONE;
					m_spRagdollDropItems[1]->SetCollisionSound(pItem->m_iSound_Dropped,false);
					m_spRagdollDropItems[1]->StartCollecting(m_pWServer, pItemModel, &Settings);
					m_spRagdollDropItems[1]->Activate(true);
					m_spRagdollDropItems[1]->Animate(m_pWServer->GetGameTick(), Mat);
					if (_pCD->m_iPlayer != -1)
					{
						CVec3Dfp32 ImpulsePos = GetPosition();
						ImpulsePos.k[2] += 30.0f;
						m_spRagdollDropItems[1]->AddPendingImpulse(GetPosition(), CVec3Dfp32(0,0,2.0f) -CVec3Dfp32::GetMatrixRow(GetPositionMatrix(),1));
					}
				}

				int iObj = m_pWServer->Object_Create("DroppableItem", Mat);
				if(iObj > 0)
				{
					m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENEPICKUP_SETDEFAULTS), iObj);

					int nMessages;
					const CWO_SimpleMessage* pMsg = m_MsgContainer.GetMessages(PLAYER_SIMPLEMESSAGE_ONPICKUPDROPWEAPON, nMessages);
					CWObject_Message Msg(OBJMSG_RPG_REPLACEPICKUP, nMessages, aint(pMsg));
					Msg.m_pData = (CRPG_Object *)pItem;
					m_pWServer->Message_SendToObject(Msg, iObj);
					m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_RPG_SETRIGIDBODY, (aint)((CConstraintRigidObject*) m_spRagdollDropItems[1])), iObj); 
					CWObject *pObj = m_pWServer->Object_Get(iObj);
					if(pObj)
						pObj->m_Param = pItem->GetAmmo(Char()) | 0x8000;
				}

				Char()->RemoveItemByIdentifier(m_iObject,pItem->m_Identifier,RPG_CHAR_INVENTORY_ARMOUR);
				_pCD->m_Item1_Model.Clear();
				_pCD->m_Item1_Model.MakeDirty();
			}
		}
		
		//Char_SetPhysics(this, m_pWServer, m_pWServer, PLAYER_PHYS_DEAD);
	}
	return true;
}

aint CWObject_Character::OnCharCreateGrabbableItemObject(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD)
{
	// Create an object from item (just the model thingy...)
	if (!_pCD)
		return false;

	//ConOut("FORCE DROPPING WEAPON");

	CRPG_Object_Item *pItem = NULL;
	//CRPG_Object_Item *pItem2 = NULL;

	if (_Msg.m_Param0 != 0)
		pItem = Char()->FindItemByIdentifier(_Msg.m_Param0,_Msg.m_Param1);
	else
		pItem = Char()->GetEquippedItem(_Msg.m_Param1);
	
	aint iObj = 0;

	// Copied from Char_Die
	if (pItem &&  !(pItem->m_Flags & RPG_ITEM_FLAGS_NOPICKUP))// && (pItem->m_AnimProperty > 1))
	{
		CXR_AnimState Anim;
		CXR_Skeleton* pSkel;
		CXR_SkeletonInstance* pSkelInstance;					
		if(GetEvaluatedPhysSkeleton(pSkelInstance, pSkel, Anim))
		{
			CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
			if (!pModel)
				return false;

			CMat4Dfp32 Mat;
			CMTime GameTime = CMTime::CreateFromTicks(_pCD->m_GameTick, m_pWServer->GetGameTickTime(), _pCD->m_PredictFrameFrac);
			CAutoVar_AttachModel* pAttachModel = !_Msg.m_Param1 ? (CAutoVar_AttachModel*)&_pCD->m_Item0_Model : (CAutoVar_AttachModel*)&_pCD->m_Item1_Model;
			if(pAttachModel->IsValid() && pAttachModel->GetModel0_RenderInfo(m_pWServer->GetMapData(), NULL, pSkelInstance, pSkel, GameTime, 
				Anim, Mat, pModel, m_pWServer))
			{
				CWObject_Object::SPart Part;
				Part.m_iModel[0] = pAttachModel->m_iModel[0];
				// Mass must be over 30 if we are going to grab it...
				Part.m_Mass = 31.0f;
				//Part.m_PhysPrim.m_iPhysModel = pAttachModel->m_iModel[0];
				Part.m_PhysPrim.m_iPhysModel_Dynamics = pItem->m_iPhysModel;//pAttachModel->m_iModel[0];
				Part.m_PhysPrim.m_BoundBox.m_Max = CVec3Dfp32(2.0f,2.0f,2.0f);
				Part.m_PhysPrim.m_BoundBox.m_Min = CVec3Dfp32(-2.0f,-2.0f,-2.0f);
				Part.m_lAttach_DemonArm.Add(CVec3Dfp32(0.0f,0.0f,0.0f));
				
				aint lParams[2];
				lParams[0] = NULL;
				lParams[1] = (aint)&Part;
				iObj = m_pWServer->Object_Create("Object",Mat,-1,lParams,2);
				//CWObject* pObj = m_pWServer->Object_Get(iObj);
				//if (pObj)
				//{
					
				//}
				// Send "grabbed item" impulse to the animgraph
				if (_Msg.m_Reason == 0)
				{
					CWAG2I_Context Context(this,m_pWServer,CMTime::CreateFromTicks(_pCD->m_GameTick,m_pWServer->GetGameTickTime()));
					_pCD->m_AnimGraph2.GetAG2I()->SendImpulse(&Context,CXRAG2_Impulse(AG2_IMPULSETYPE_RESPONSE,AG2_IMPULSEVALUE_RESPONSE_GRABBEDITEM),0);
				}
			}
			Char()->RemoveItemByIdentifier(m_iObject,pItem->m_Identifier,_Msg.m_Param1);
			pAttachModel->Clear();
			if (!_Msg.m_Param1)
				_pCD->m_Item0_Model.MakeDirty();
			else
				_pCD->m_Item1_Model.MakeDirty();
		}
	}
	return iObj;
}

aint CWObject_Character::OnCharPrecacheMessage(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD)
{
	if(_Msg.m_DataSize == sizeof(CWObject_Message))
	{
		CWObject_Message *pMsg = (CWObject_Message *)_Msg.m_pData;
		if(pMsg->m_Msg == OBJMSG_CHAR_PLAYANIM)
		{
			CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(m_pWServer);
			if (!pServerMod)
				return 0;
			CFStr Animation = (char*)pMsg->m_pData;
			CFStr Container = Animation.GetStrSep(":");
			int32 iSeq = Animation.Val_int();
			//ConOutL(CStrF("Adding animation: %s:%d",ContainerName.Str(),pAnimName->m_iAnimSeq));
			pServerMod->AddAnimContainerEntry(Container, iSeq);
			return 1;
		}

		else if(pMsg->m_Msg == OBJMSG_CHAR_ADDITEM)
		{
			TArray<int32> lItemAnimTypesNeeded;
			CRPG_Object::IncludeRPGClass((char *)pMsg->m_pData, m_pWServer->GetMapData(), m_pWServer, true, &lItemAnimTypesNeeded);

			// Items added at server, get last one added and assume it's the item created
			CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(m_pWServer);
			CWAG2I_Context AGContext(this,m_pWServer,_pCD->m_GameTime);
			if (pServerMod && lItemAnimTypesNeeded.Len())
			{
				TArray<CXRAG2_Impulse> lImpulses;
				//pServerMod->AddItemAnimType(m_AnimProperty);
				for (int32 i = 0; i < lItemAnimTypesNeeded.Len(); i++)
				{
					int32 Needed = lItemAnimTypesNeeded[i];
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,Needed));
					// Add for crouch as well
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,Needed+AG2_IMPULSEVALUE_NUMWEAPONTYPES));
					pServerMod->AddAnimTypeItem(Needed);
				}
				_pCD->m_AnimGraph2.GetAG2I()->TagAnimSetFromImpulses(&AGContext,m_pWServer->GetMapData(),m_pWServer->m_spWData,lImpulses);
				
			}
			/*if (AnimSetsNeeded != "")
			{

				CWO_Character_ClientData* _pCD = CWObject_Character::GetClientData(this);
				if (_pCD)
				{
					CWAGI_Context Context(this, m_pWServer,_pCD->m_GameTime,SERVER_TIMEPERFRAME);
					while (AnimSetsNeeded != "")
					{
						CStr SetName = AnimSetsNeeded.GetStrSep("+");
						_pCD->m_AnimGraph.GetAGI()->TagAnimSetFromName(&Context, m_pWServer->GetMapData(), m_pWServer->m_spWData,SetName);
					}
				}
			}*/

			return 1;
		}

		else if(pMsg->m_Msg == OBJMSG_CHAR_RESPAWN)
			m_pWServer->GetMapData()->GetResourceIndex_Class((const char *)pMsg->m_pData);

		/*else if (pMsg->m_Msg == OBJMSG_CHAR_MOVETOKEN)
		{
			// Ok then, tag animations from targetstate
			CWO_Character_ClientData* _pCD = CWObject_Character::GetClientData(this);
			if (_pCD)
			{
				CWAGI_Context Context(this, m_pWServer,_pCD->m_GameTime);
				_pCD->m_AnimGraph.GetAGI()->TagAnimSetFromTargetState(&Context,m_pWServer->GetMapData(),m_pWServer->m_spWData,CStr((char*)pMsg->m_pData));
			}
		}*/
		else if(pMsg->m_Msg == OBJMSG_CHAR_SETEXTRAITEM || pMsg->m_Msg == OBJMSG_CHAR_SETDROPITEM)
		{
			if(!pMsg->m_pData)
				return 0;
			CRPG_Object::IncludeRPGClass((char *)pMsg->m_pData, m_pWServer->GetMapData(), m_pWServer, true);
		}
		else if (pMsg->m_Msg == OBJMSG_CHAR_ADDAVAILABLEDARKLING)
		{
			// Precache darkling
			m_pWServer->GetMapData()->GetResourceIndex_Class((char*)pMsg->m_pData);
		}
		else if (pMsg->m_Msg == OBJMSG_CHAR_ANIMIMPULSE)
		{
			if (!_pCD)
				return 0;

			TArray<CXRAG2_Impulse> lImpulse;
			if (pMsg->m_Param0 == 6 || pMsg->m_Param0 == 7)
			{
				// New version
				CXRAG2_Impulse Impulse;
				CFStr AnimState = (char *)pMsg->m_pData;
				Impulse.m_ImpulseType = AnimState.GetStrSep(",").Val_int();
				Impulse.m_ImpulseValue = AnimState.GetStrSep(",").Val_int();
				lImpulse.Add(Impulse);
				CStr Val = AnimState.GetStrSep(",");
				if (Val.Len())
				{
					Impulse.m_ImpulseType = Val.Val_int();
					Impulse.m_ImpulseValue = AnimState.GetStrSep(",").Val_int();
					lImpulse.Add(Impulse);
				}
			}
			else
			{
				CXRAG2_Impulse Impulse;
				Impulse.m_ImpulseType = pMsg->m_Param0;
				CFStr Token = (char *)pMsg->m_pData;
				Impulse.m_ImpulseValue = Token.GetStrSep(",").Val_int();
				lImpulse.Add(Impulse);
			}
			
			CWAG2I_Context AGContext(this, m_pWServer,_pCD->m_GameTime,m_pWServer->GetGameTickTime());
			_pCD->m_AnimGraph2.GetAG2I()->TagAnimSetFromImpulses(&AGContext,m_pWServer->GetMapData(),m_pWServer->m_spWData,lImpulse);
		}
		else if (pMsg->m_Msg == OBJMSG_CHAR_DEMONHEAD_ANIMIMPULSE)
		{
			CWObject* pTentacleObj = m_pWServer->Object_Get(_pCD->m_iDarkness_Tentacles);
			if (pTentacleObj)
			{
				CWObject_TentacleSystem& TentacleSystem = *safe_cast<CWObject_TentacleSystem>(pTentacleObj);
				TentacleSystem.OnPrecacheMessage(_Msg);
			}
		}
		else if (pMsg->m_Msg == OBJMSG_CHAR_UPDATEITEMDESC)
		{
			// Precache new icon surface (and the item)
			CStr NewIcon = (char*)pMsg->m_pData;
			CStr ItemType = NewIcon.GetStrSep(","); 
			CStr NewDesc = NewIcon.GetStrSep(",");
			if (NewIcon.Len() && NewIcon != "0")
				m_pWServer->GetMapData()->GetResourceIndex_Surface(NewIcon);
			TArray<int32> lItemAnimTypesNeeded;
			CRPG_Object::IncludeRPGClass(ItemType, m_pWServer->GetMapData(), m_pWServer, true, &lItemAnimTypesNeeded);

			// Items added at server, get last one added and assume it's the item created
			CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(m_pWServer);
			if (pServerMod && lItemAnimTypesNeeded.Len())
			{
				//pServerMod->AddItemAnimType(m_AnimProperty);
				for (int32 i = 0; i < lItemAnimTypesNeeded.Len(); i++)
					pServerMod->AddAnimTypeItem(lItemAnimTypesNeeded[i]);
			}
		}
		else if (pMsg->m_Msg == OBJMSG_CHAR_MOUNT)
		{	// Precache turret anims
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if (pCD)
			{	// Tell AG we use iBehaviour as well
				CWAG2I_Context Context(this,m_pWServer,pCD->m_GameTime);
				TArray<CXRAG2_Impulse> lImpulses;
				lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,AG2_IMPULSEVALUE_GAMEPLAY_TURRET_STANDING));
				lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,AG2_IMPULSEVALUE_GAMEPLAY_TURRET_LOW));
				pCD->m_AnimGraph2.GetAG2I()->TagAnimSetFromImpulses(&Context,m_pWServer->GetMapData(),m_pWServer->m_spWData,lImpulses);
			}
		}
	}

	return 0;
}

aint CWObject_Character::OnCharTeleport(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD)
{
	const char* pTargetName = NULL;
	CVec3Dfp32 OldPos = GetPosition();

	//Get object position to teleport to
	CMat4Dfp32 Mat;
	if(_Msg.m_Param0 == 0)
	{
		CWObject * pObj = m_pWServer->Object_Get(m_pWServer->Selection_GetSingleTarget((char *)_Msg.m_pData));
		if (!pObj)
			return 0;

		pTargetName = pObj->GetName();
		Mat = pObj->GetPositionMatrix();
		Mat.k[3][2] += 0.01f;
	}
	else if(_Msg.m_Param0 == 1)
	{
		Mat.Unit();
		CVec3Dfp32::GetRow(Mat, 3).ParseString((char *)_Msg.m_pData);
	}
	else if(_Msg.m_Param0 == 2)
	{
		Mat.Unit();
		CVec3Dfp32::GetRow(Mat, 3) = _Msg.m_VecParam0;
	}
	else if(_Msg.m_Param0 == 3)
	{
		Mat = *(CMat4Dfp32 *)_Msg.m_pData;
	}
	else if(_Msg.m_Param0 == 4)
	{
		CWObject *pObj = m_pWServer->Object_Get(m_pWServer->Selection_GetSingleTarget((char *)_Msg.m_pData));
		if (!pObj)
			return 0;

		Mat = GetPositionMatrix();
		CVec3Dfp32::GetRow(Mat, 3) = pObj->GetPosition();
		Mat.k[3][2] += 0.01f;
	}
	else if (_Msg.m_Param0 == 5)
	{
		// Teleport to target but ignore XY-rotation
		CWObject* pObj = m_pWServer->Object_Get( m_pWServer->Selection_GetSingleTarget((char *)_Msg.m_pData) );
		if (!pObj)
			return 0;

		CVec3Dfp32 Angles1, Angles2;
		Angles1.CreateAnglesFromMatrix(0, GetPositionMatrix());
		Angles2.CreateAnglesFromMatrix(0, pObj->GetPositionMatrix());
		Angles1.k[2] = Angles2.k[2];
		Angles1.CreateMatrixFromAngles(0, Mat);
		Mat.GetRow(3) = pObj->GetPosition();
		Mat.k[3][2] += 0.01f;
	}
	else if (_Msg.m_Param0 == 8)
	{ // Delta movement
		CVec3Dfp32 Offset;
		Offset.ParseString((const char*)_Msg.m_pData);

		Mat = GetPositionMatrix();
		Mat.GetRow(3) += Offset;

		_pCD->m_LastTeleportTick = _pCD->m_GameTick;
		return m_pWServer->Object_SetPosition_World(m_iObject, Mat) ? 1 : 0;
	}
	else if (_Msg.m_Param0 == 9)
	{ // Delta movement, relative target
		CStr Target = (const char*)_Msg.m_pData;
		CStr OffsetStr = Target.GetStrSep(":");
		CVec3Dfp32 Offset;
		Offset.ParseString(OffsetStr);

		int iTarget = m_pWServer->Selection_GetSingleTarget(Target);
		CWObject_CoreData* pTarget = m_pWServer->Object_Get(iTarget);
		if (!pTarget)
			return 0;	

		// Offset relative forward target position
		Mat = pTarget->GetPositionMatrix();
		CVec3Dfp32 TargetLook = Mat.GetRow(0);
		TargetLook.k[2] = 0.0f;
		TargetLook.SetRow(Mat,0);
		Mat.RecreateMatrix(0,1);
		CVec3Dfp32 TargetPos = Mat.GetRow(3);
		Mat.GetRow(3) = CVec3Dfp32(0.0f,0.0f,0.0f);
		Offset = Offset * Mat + TargetPos;
		Offset.SetMatrixRow(Mat,3);
		// Look at target
		TargetLook = pTarget->GetPosition() - Offset;
		TargetLook.SetMatrixRow(Mat,0);
		Mat.RecreateMatrix(0,2);

		_pCD->m_LastTeleportTick = _pCD->m_GameTick;
	}

	CCollisionInfo Info;
	if(m_pWServer->Phys_IntersectWorld((CSelection *) NULL, GetPhysState(), Mat, Mat, m_iObject, &Info))
	{
		CWObject *pObj = m_pWServer->Object_Get(Info.m_iObject);
		if (pObj)
			ConOutL(CStrF("Failed to teleport Character (%i, %s, %s) to position %s, because of object (%i, %s, %s, Class: %s, Position: %s", 
				m_iObject, GetName(), GetTemplateName(), 
				Mat.GetRow(3).GetString().Str(),
				pObj->m_iObject, pObj->GetName(), pObj->GetTemplateName(),
				pObj->MRTC_GetRuntimeClass()->m_ClassName,
				pObj->GetPosition().GetString().Str()		));
		return 0;
	}

//			m_pWServer->Debug_RenderWire(GetPosition(), CVec3Dfp32::GetRow(Mat, 3), 0xff7f7f7f, 20.0f);

//			M_TRACE("server teleport, tick: %d\n", m_pWServer->GetGameTick());
//			M_TRACE(" - old pos: %s\n", GetPosition().GetString().Str());
//			M_TRACE(" - new pos: %s\n", Mat.GetRow(3).GetString().Str());

	if(!m_pWServer->Object_SetPosition_World(m_iObject, Mat))
	{
		ConOutL(CStrF("Failed to teleport Character (%i, %s)", m_iObject, GetName()));
		return 0;
	}
	if (!_pCD)
		return 0;

//	M_TRACE("T: %d - [Char %s, %d], teleported from %s to %s (%s)\n", m_pWServer->GetGameTick(), 
//		GetName(), m_iObject, OldPos.GetString().Str(), GetPosition().GetString().Str(), pTargetName ? pTargetName : "");

	_pCD->m_Control_Look_Wanted = GetLook(GetPositionMatrix());
	_pCD->m_Control_Look = _pCD->m_Control_Look_Wanted;
	_pCD->m_LastTeleportTick = m_pWServer->GetGameTick();
	_pCD->m_Phys_IdleTicks = 0;
	// Make sure the autovar are sent instantly, even if OnRefresh already has run
	m_DirtyMask |= _pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
	SetVelocity(0);
	return 1;
}

aint CWObject_Character::OnCharActivateItem(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD)
{
	SetWeaponUnequipTimer();
	CMat4Dfp32 Mat;
	GetActivatePosition(Mat);
	if(_pCD)
		Char()->m_LastDamageTick = _pCD->m_GameTick;
	CRPG_Object_Item* pEquippedItem = Char()->GetFinalSelectedItem(_Msg.m_Param0);
	if (!pEquippedItem)
		return false;

	// Send impulse to weapon as well
	int32 iToken = _Msg.m_Param0 == 2 ? TOKEN_WEAPON2 : TOKEN_WEAPON1;
	CRPG_Object_Char* pRoot = Char();
	{
		if (_Msg.m_Param1 != 2 && pEquippedItem->IsActivatable(Char(),m_iObject))
		{
			CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_PRIMARYATTACK);
			//CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_DIALOG,AG2_IMPULSEVALUE_DIALOG_INIT);
			CWAG2I_Context Context(this,m_pWServer,CMTime::CreateFromTicks(_pCD->m_GameTick,m_pWServer->GetGameTickTime(),_pCD->m_PredictFrameFrac),m_pWServer->GetGameTickTime());
			_pCD->m_WeaponAG2.UpdateItemProperties(pRoot,pEquippedItem,this,_pCD);
			_pCD->m_WeaponAG2.GetAG2I()->SendImpulse(&Context,Impulse, iToken);
			//ConOutL(CStrF("Activating WeaponAG: %f Tick: %d",Context.m_GameTime.GetTime(),_pCD->m_GameTick));
			// Check anim events directly
			//_pCD->m_WeaponAG2.GetAG2I()->CheckAnimEvents(&Context, OBJMSG_CHAR_ONANIMEVENT, ANIM_EVENT_MASK_EFFECT);
		}
	}

	bool bStatus = pRoot->ActivateItem(_Msg.m_Param0, Mat, m_iObject, (_pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_GODMODE ? 4 : _Msg.m_Param1), _Msg.m_Reason != 0);
	// Update weapon status incase it gets copied to client
	if (iToken)
	{
		if (!(_pCD->m_WeaponStatus1.m_Flags & CAutoVar_WeaponStatus::WEAPONSTATUS_DARKNESSDRAIN))
			_pCD->m_WeaponStatus1.m_AmmoLoad = pEquippedItem->GetAmmo(pRoot);
	}
	else
	{
		if (!(_pCD->m_WeaponStatus0.m_Flags & CAutoVar_WeaponStatus::WEAPONSTATUS_DARKNESSDRAIN))
			_pCD->m_WeaponStatus0.m_AmmoLoad = pEquippedItem->GetAmmo(pRoot);
	}

	if (pEquippedItem && pEquippedItem->IsUnequip())
	{
		// Selectitemtype 0 (barehands) if ammo is empty
		pRoot->SelectItem(0,true,true);
		CRPG_Object_Item* pItem = pRoot->GetFinalSelectedItem(AG2_ITEMSLOT_WEAPONS);
		int32 ItemClass = _pCD->m_EquippedItemClass;
		if (pEquippedItem && (ItemClass != pItem->m_AnimProperty))
		{
			_pCD->m_EquippedItemClass = (uint16)pItem->m_AnimProperty;
			_pCD->m_EquippedItemType = (uint16)pItem->m_iItemType;
			_pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_ITEMANIMTYPE,pItem->m_AnimType);
		}
		// Remove the item
		pRoot->RemoveItemByType(m_iObject,pEquippedItem->m_iItemType);
	}
	return bStatus;
}

aint CWObject_Character::OnCharSetExtraItem(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD)
{
	if (_pCD == NULL)
		return 0;

	const char* St = (const char*)_Msg.m_pData;
	if(!St || !CStrBase::CompareNoCase(St, ""))
	{
		_pCD->m_Item2_Model.Clear();
		_pCD->m_Item2_Model.MakeDirty();
		return 1;
	}
	spCRPG_Object spObj = CRPG_Object::CreateObject(St, m_pWServer);
	if(!spObj)
		return 0;

	CRPG_Object_Item *pItem = TDynamicCast<CRPG_Object_Item>((CRPG_Object *)spObj);
	if(!pItem)
		return 0;

	_pCD->m_Item2_Model.UpdateModel(pItem->m_Model);
	_pCD->m_Item2_Model.MakeDirty();
	m_DirtyMask |= _pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
	return 1;
}

aint CWObject_Character::OnCharPlayAnim(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD)
{
	if(_pCD)
	{
		CFStr St = (char *)_Msg.m_pData;
		CFStr Container = St.GetStrSep(":");
		int iAnim = 0;
		CXR_Anim_Base *pAnim = NULL;
		if ((!Container.IsEmpty())&&(Container != "0"))
		{
			CFStr Name = "ANM:" + Container;
			iAnim = m_pWServer->GetWorldData()->ResourceExistsPartial(Name.UpperCase());
			//iAnim = m_pWServer->GetMapData()->GetResourceIndex_Anim(Container);
			CWResource* pWRes = m_pWServer->GetWorldData()->GetResource(iAnim);
			CWRes_Anim* pAnimRes = (pWRes && (pWRes->GetClass() == WRESOURCE_CLASS_XSA)) ? (CWRes_Anim*)(const CWResource*)pWRes : NULL;
			pAnim = pAnimRes ? pAnimRes->GetAnim() : NULL;
		}
		int iSeq = St.Val_int();
		vec128 Move = M_VConst(0,0,0,1.0f);
		CQuatfp32 Rot;
		Rot.Unit();
		if(iAnim != 0)
		{
			// Set "destination"
			CMat4Dfp32 OrgPos = GetPositionMatrix();
			if (pAnim)
			{
				CXR_Anim_SequenceData *pSeq = pAnim->GetSequence(iSeq);
				if(pSeq)
					pSeq->EvalTrack0(CMTime(), Move, Rot);
			}
			CMat4Dfp32 RotMat,Temp;
			Move= M_VNeg(Move);
			Move= M_VMul(Move, M_VLdScalar(_pCD->m_CharGlobalScale * (1.0f - _pCD->m_CharSkeletonScale)));
			Move= M_VSetW1(Move);
			Rot.Inverse();
			Rot.CreateMatrix(RotMat);
			OrgPos.Multiply3x3(RotMat,Temp);
			Move = M_VMulMat4x3(Move, OrgPos);
			OrgPos = Temp;
			OrgPos.r[3] = Move;
			_pCD->m_AnimGraph2.SetDestinationLock(OrgPos);
			CMTime Time;
			if (_Msg.m_Reason == 'CT')
				Time = *((CMTime*)(void*)_Msg.m_Param1);
			else
				Time = m_pWServer->GetGameTime() - CMTime::CreateFromSeconds(fp32(_Msg.m_Param0) / 1000);
			
			if (!_Msg.m_Reason || _Msg.m_Reason == 'CT')
				_pCD->m_AnimGraph2.GetAG2I()->SetOverlayAnim(iAnim, iSeq, Time);
			else
				_pCD->m_AnimGraph2.GetAG2I()->SetOverlayAnimLipSync(iAnim, iSeq,(_Msg.m_Param1 != 0 ? _Msg.m_Param1 : PLAYER_ROTTRACK_NECK));
		}
		else
		{
			CMat4Dfp32 Dest;
			Dest.GetRow(3) = CVec3Dfp32(_FP32_MAX);
			_pCD->m_AnimGraph2.ClearDestinationLock();
			_pCD->m_AnimGraph2.SetDestination(Dest);
			// Clear overlay
			if (!_Msg.m_Reason)
				_pCD->m_AnimGraph2.GetAG2I()->SetOverlayAnim(0, 0, CMTime::CreateFromSeconds(0.0f));
			else
				_pCD->m_AnimGraph2.GetAG2I()->SetOverlayAnimLipSync(0, 0,0);
		}
	}
	
	// Make sure the autovar are sent instantly, even if OnRefresh already has run
	m_DirtyMask |= _pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
	return 1;
}

aint CWObject_Character::OnCharShakeCamera(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD)
{
	if (_pCD)
	{
		fp32 OldAmplitude = _pCD->m_CameraShake_Amplitude;
		uint32 OldStartTick = _pCD->m_CameraShake_StartTick;
		uint32 OldElapsedTicks = _pCD->m_GameTick - OldStartTick;
		uint32 OldDurationTicks = _pCD->m_CameraShake_DurationTicks;
		uint32 OldPeekTick = (uint32)((fp32)OldDurationTicks * PLAYER_CAMERASHAKE_BLENDOUTDURATION);

		bool bNoRumble = false;
		if ((_Msg.m_Param0 != 0) && (_Msg.m_DataSize == sizeof(CWO_CameraShake)))
		{
			CWO_CameraShake* pCS = (CWO_CameraShake*)_Msg.m_pData;
			_pCD->m_CameraShake_Amplitude = pCS->m_Magnitude;
			_pCD->m_CameraShake_DurationTicks = (uint32)(pCS->m_Duration * m_pWServer->GetGameTicksPerSecond());
			_pCD->m_CameraShake_Speed = pCS->m_Speed;
			bNoRumble = pCS->m_bNoRumble;
		}
		else
		{
			fp32 Range = 0.0f;
			if (_Msg.m_pData != NULL)
			{
				CStr Params((char*)_Msg.m_pData);
				_pCD->m_CameraShake_Amplitude = (fp32)Params.GetStrMSep(",/").Val_fp64();
				_pCD->m_CameraShake_DurationTicks = (uint32)(m_pWServer->GetGameTicksPerSecond() * (fp32)Params.GetStrMSep(",/").Val_fp64());
				_pCD->m_CameraShake_Speed = (fp32)Params.GetStrMSep(",/").Val_fp64();
				Range = (fp32)Params.GetStrMSep(",").Val_fp64();
			}
			else
			{
				// Defaults used in nodetype.txt
				_pCD->m_CameraShake_Amplitude = 1.0f;
				_pCD->m_CameraShake_DurationTicks = (uint32)(2.0f * m_pWServer->GetGameTicksPerSecond());
				_pCD->m_CameraShake_Speed = 5.0f;
			}

			// Apply falloff
			CWObject* pSender = m_pWServer->Object_Get(_Msg.m_iSender);
			if (pSender)
			{
				if (Range < 1.0f) Range = 1024.0f; // Default range
				fp32 Distance = GetPosition().Distance(pSender->GetPosition()) / Range;
				if (Distance > 1.0f)
				{
					bNoRumble = true;
					_pCD->m_CameraShake_Amplitude = 0.0f;
				}
				else if (Distance > 0.5f)
				{ // apply linear falloff from 0.5 to 1.0
					_pCD->m_CameraShake_Amplitude = _pCD->m_CameraShake_Amplitude * 2*(1.0f - Distance);	
				}
			}
		}

		if(_pCD->m_CameraShake_Amplitude > 5 && !bNoRumble)
		{
			CWObject_Message Msg(OBJMSG_CHAR_RUMBLE);

			if(_pCD->m_CameraShake_Amplitude > 15)
				Msg.m_pData = (void*)"CS3";
			else if(_pCD->m_CameraShake_Amplitude > 10)
				Msg.m_pData = (void*)"CS2";
			else
				Msg.m_pData = (void*)"CS1";

			OnMessage(Msg);
		}

		_pCD->m_CameraShake_StartTick = _pCD->m_GameTick;

		bool bHigher = (_pCD->m_CameraShake_Amplitude > OldAmplitude);
		bool bEqualHigh = (_pCD->m_CameraShake_Amplitude == OldAmplitude);
		bool bLonger = (_pCD->m_CameraShake_DurationTicks > OldDurationTicks);
		bool bEqualLong = (_pCD->m_CameraShake_DurationTicks == OldDurationTicks);
		bool bAfterPeek = (_pCD->m_CameraShake_StartTick > OldPeekTick);
		
		if ((OldDurationTicks > 0) && (OldElapsedTicks < OldDurationTicks))
		{
			if (bHigher || 
				(bEqualHigh && (bLonger || 
								(bEqualLong && bAfterPeek))))
			{
				fp32 OldTime = (fp32)OldElapsedTicks * m_pWServer->GetGameTickTime();
				fp32 OldDuration = (fp32)OldDurationTicks * m_pWServer->GetGameTickTime();
				fp32 OldBlendInTime = OldDuration * PLAYER_CAMERASHAKE_BLENDINDURATION;
				fp32 OldBlendOutTime = OldDuration * PLAYER_CAMERASHAKE_BLENDOUTDURATION;
				fp32 Amplitude = GetFade(OldTime, OldDuration, OldBlendInTime, OldBlendOutTime) * OldAmplitude;
				
				fp32 NewAmplitude = (fp32)_pCD->m_CameraShake_Amplitude;
				fp32 NewDuration = (fp32)_pCD->m_CameraShake_DurationTicks * m_pWServer->GetGameTickTime();
				fp32 NewBlendInTime = NewDuration * PLAYER_CAMERASHAKE_BLENDINDURATION;
				fp32 NewTime = (Amplitude / NewAmplitude) * NewBlendInTime;
				uint32 NewElapsedTicks = (uint32)(NewTime * m_pWServer->GetGameTicksPerSecond());

				_pCD->m_CameraShake_StartTick = _pCD->m_CameraShake_StartTick - NewElapsedTicks;
			}
			else
			{
				_pCD->m_CameraShake_Amplitude = OldAmplitude;
				_pCD->m_CameraShake_DurationTicks = OldDurationTicks;
				_pCD->m_CameraShake_StartTick = OldStartTick;
			}
		}

		return 1;
	}
	else
		return 0;
}

aint CWObject_Character::OnCharAnimImpulse(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD, CWAG2I* _pWAG2I, CAG2TokenID _iToken)
{
	// Send impulse to animation system
	// Param0 = impulse type, impulsevalue and token in string format "impulseval","token"
	if (!_Msg.m_pData)
		return 0;
	CXRAG2_Impulse Impulse;
	CWAG2I_Context AGContext(this, m_pWServer,_pCD->m_GameTime,m_pWServer->GetGameTickTime());
	if (_Msg.m_Param0 == 6 || _Msg.m_Param0 == 7)
	{
		// New version
		CFStr AnimState = (char *)_Msg.m_pData;
		Impulse.m_ImpulseType = AnimState.GetStrSep(",").Val_int();
		Impulse.m_ImpulseValue = AnimState.GetStrSep(",").Val_int();
		bool bRes = _pWAG2I->SendImpulse(&AGContext,Impulse,_iToken);
		CStr Val = AnimState.GetStrSep(",");
		if (Val.Len())
		{
			Impulse.m_ImpulseType = Val.Val_int();
			Impulse.m_ImpulseValue = AnimState.GetStrSep(",").Val_int();
			bRes = _pWAG2I->SendImpulse(&AGContext,Impulse,_iToken);
		}
		_pWAG2I->Refresh(&AGContext);
		return bRes;
	}
	else
	{
		Impulse.m_ImpulseType = _Msg.m_Param0;
		CFStr Token = (char *)_Msg.m_pData;
		Impulse.m_ImpulseValue = Token.GetStrSep(",").Val_int();
		_iToken = Token.Val_int();
		return _pWAG2I->SendImpulse(&AGContext,Impulse, _iToken);
	}
}

aint CWObject_Character::OnCharSetCameraEffects(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD)
{
	if (_pCD)
	{
		CFStr St = (char *)_Msg.m_pData;
		switch(_Msg.m_Param0)
		{
		case 0: // MotionBlur
			_pCD->m_MotionBlur_Amount = (uint8)(St.GetStrSep(",").Val_fp64() * 255);
			_pCD->m_MotionBlur_Fuzzyness = (uint8)(St.GetStrSep(",").Val_fp64() * 255);
			break;
		case 1: // GlowMotionBlur
			_pCD->m_GlowMotionBlur = (uint8)(St.GetStrSep(",").Val_fp64() * 255);
			break;
		case 2: // DOF
			_pCD->m_DOF_Object = 0;
			_pCD->m_DOF_Distance = (uint16)(St.GetStrSep(",").Val_fp64());
			_pCD->m_DOF_Amount = (uint8)(St.GetStrSep(",").Val_fp64() * 255);
			break;
		case 3: // DOF on Object
			_pCD->m_DOF_Object = m_pWServer->Selection_GetSingleTarget(St.GetStrSep(","));
			_pCD->m_DOF_Distance = 0;
			_pCD->m_DOF_Amount = (uint8)(St.GetStrSep(",").Val_fp64() * 255);
			break;
		}
	}
	return 1;
}

aint CWObject_Character::OnCharSetMountedLook(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD)
{
	if (_pCD->m_RenderAttached != 0 && _pCD->m_iMountedObject != 0)
	{
		CWObject_CoreData* pObj = m_pWServer->Object_GetCD(_pCD->m_iMountedObject);
		CWO_Character_ClientData* pCDOther = (pObj ? GetClientData(pObj) : NULL);
		if (pCDOther)
		{
			CStr Str = (char*)_Msg.m_pData;
			fp32 MaxOffset =  ((fp32)(_pCD->m_AnimGraph2.GetMaxBodyOffset())) / 360.0f;
			fp32 NewAngle = -(fp32)(Str.GetStrSep(",").Val_int()) / 360.0f;
			if (NewAngle < -MaxOffset)
				NewAngle = -MaxOffset;
			else if (NewAngle > MaxOffset)
				NewAngle = MaxOffset;
			pCDOther->m_TurnCorrectionTargetAngle = NewAngle;

			NewAngle = -(fp32)(Str.GetStrSep(",").Val_int()) / 360.0f;
			if (NewAngle > 0.2f)
				NewAngle = 0.2f;
			else if (NewAngle < -0.2f)
				NewAngle = -0.2f;
			pCDOther->m_TurnCorrectionTargetAngleVert = NewAngle;
		}
	}
	return true;
}

aint CWObject_Character::OnCharSetMountedCamera(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD)
{
	if (!_Msg.m_pData)
		return false;

	// Param0 = type of mode (follow/free etc)
	// Str = {target,lookfreedom,blendtime} (blendtime if camera has mountedcam set already...)
	CStr ConfStr = (char*)_Msg.m_pData;
	int16 iTarget = m_pWServer->Selection_GetSingleTarget(ConfStr.GetStrSep(","));
	int MaxLookFreedomZ = ConfStr.GetStrSep(",").Val_int();
	int MaxLookFreedomY = ConfStr.GetStrSep(",").Val_int();
	fp32 Blend = ConfStr.GetStrSep(",").Val_fp64();
	int MinLookFreedomZ = MaxLookFreedomZ;
	int MinLookFreedomY = MaxLookFreedomY;
	uint8 DecreaseRate = 10;
	if (ConfStr.Len())
	{
		MinLookFreedomZ = ConfStr.GetStrSep(",").Val_int();
		if (MinLookFreedomZ < 0)
			MinLookFreedomZ = -MinLookFreedomZ;
		MinLookFreedomY = ConfStr.GetStrSep(",").Val_int();
		if (MinLookFreedomY < 0)
			MinLookFreedomY = -MinLookFreedomY;
	}
	if (ConfStr.Len())
	{
		DecreaseRate = Min((uint8)100,(uint8)ConfStr.Val_int());
	}

	if (iTarget > 0)
	{
		_pCD->m_CameraUtil.SetMountedCamera(m_pWServer,_pCD, iTarget,_Msg.m_Param0,MinLookFreedomZ,MinLookFreedomY,MaxLookFreedomZ,MaxLookFreedomY,Blend,DecreaseRate);
		_pCD->m_CameraUtil.MakeDirty();
	}
	else
	{
		// Clear stuff
		_pCD->m_CameraUtil.Clear();
		_pCD->m_CameraUtil.MakeDirty();
	}
	return true;
}

aint CWObject_Character::OnCharUpdateItemDesc(const CWObject_Message& _Msg, class CWO_Character_ClientData* _pCD)
{
	CStr NewIcon = (char*)_Msg.m_pData;
	CStr ItemType = NewIcon.GetStrSep(","); 
	CStr NewDesc = NewIcon.GetStrSep(",");
	int16 Icon = 0;
	if (NewIcon.Len() && NewIcon != "0")
	{
		//Icon = m_pWServer->GetMapData()->GetResourceIndex_Surface(NewIcon);
		MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
		if(pTC)
		{
			Icon = pTC->GetTextureID(NewIcon);
			if(Icon)
				pTC->SetTextureParam(Icon, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
		}
	}
	return Char_UpdateItemDescription(ItemType,NewDesc,Icon);
}
