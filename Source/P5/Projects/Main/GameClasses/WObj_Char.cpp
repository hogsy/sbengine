/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Character execution.
					
	Contents:		OnRefresh
					OnClientRefresh
					OnPumpControl
					etc..
\*____________________________________________________________________________________________*/
#include "PCH.h"

#include "WObj_Char.h"
#include "../GameWorld/WClientMod_Defines.h"
#include "WObj_Game/WObj_GameMod.h"

#include "WRPG/WRPGChar.h"
#include "WRPG/WRPGFist.h"
#include "WObj_AI/AICore.h"
#include "../../Shared/MOS/XR/XRBlockNav.h"
#include "WObj_Misc/WObj_Ledge.h"
#include "CConstraintSystem.h"
#include "WObj_Misc/WObj_TentacleSystem.h"
#include "WObj_Misc/WObj_CreepingDark.h"
#include "WObj_Misc/WObj_InputEntity.h"
#include "WObj_Misc/WObj_Turret.h"
#include "WObj_Misc/WObj_DarklingSpawn.h"
#include "WObj_Misc/WObj_EffectSystem.h"
//#include "WObj_Sys/WObj_Wind.h" - Disabled
#include "../../../Shared/MOS/Classes/GameWorld/WAnimGraph2Instance/WAG2I.h"
#include "../../../Shared/MOS/Classes/GameWorld/WAnimGraph2Instance/WAG2_ClientData.h"
#include "../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Lights.h"
#include "../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_AutoVar.h"
#include "WObj_Char/WObj_CharDarkling.h"
#include "WObj_Char/WObj_CharShapeshifter.h"

#include "../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_PhysCluster.h"


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Character
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Character, CWObject_Player, 0x0100);

bool CWObject_Character::GetEvaluatedSkeleton(CWObject_CoreData *_pObj, CWorld_PhysState* _pWPhysState, CXR_SkeletonInstance *&_pInstance, CXR_Skeleton *&_pSkeleton, CXR_AnimState& _Anim)
{
	MAUTOSTRIP(CWObject_Character_GetEvaluatedSkeleton, false);
	CXR_Model* pModel = _pWPhysState->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
	if(!pModel)
		return false;
	CXR_Skeleton* pSkel = pModel->GetSkeleton();
	if(!pSkel)
		return false;
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if(!pCD)
		return false;

	OnGetAnimState(_pObj, _pWPhysState, pSkel, 0, _pObj->GetPositionMatrix(), 0, _Anim);

	_pInstance = GetClientSkelInstance(pCD, 0);
	if(!_pInstance)
		return false;

	_pSkeleton = pSkel;
	return true;
}

bool CWObject_Character::GetEvaluatedSkeleton(CXR_SkeletonInstance *&_pInstance, CXR_Skeleton *&_pSkeleton, CXR_AnimState& _Anim)
{
	MAUTOSTRIP(CWObject_Character_GetEvaluatedSkeleton_2, false);
	return GetEvaluatedSkeleton(this, m_pWServer, _pInstance, _pSkeleton, _Anim);
}

bool CWObject_Character::GetEvaluatedPhysSkeleton(CWObject_CoreData *_pObj, CWorld_PhysState* _pWPhysState, CXR_SkeletonInstance *&_pInstance, CXR_Skeleton *&_pSkeleton, CXR_AnimState& _Anim, fp32 _IPTime, const CMat4Dfp32* _pCharPos)
{
	MAUTOSTRIP(CWObject_Character_GetEvaluatedSkeleton, false);
	CXR_Model* pModel = _pWPhysState->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
	if(!pModel)
		return false;
	CXR_Skeleton* pSkel = pModel->GetPhysSkeleton();
	if(!pSkel)
		return false;

	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if(!pCD)
		return false;

	CMat4Dfp32 TempMat;
	if(pCD->m_RenderAttached != 0)
	{
		CWObject_CoreData* pObj = _pWPhysState->Object_GetCD(pCD->m_RenderAttached);
		if(pObj)
		{
			_pCharPos = &TempMat;
			if (_pWPhysState->IsServer())
			{
				CMTime Time = pCD->m_GameTime;
				CWObject_Message Msg(OBJMSG_HOOK_GETTIME);
				Msg.m_pData = &Time;
				Msg.m_DataSize = sizeof(CMTime);
				_pWPhysState->Phys_Message_SendToObject(Msg, pCD->m_RenderAttached);
				Msg = CWObject_Message(OBJMSG_HOOK_GETRENDERMATRIX, (aint)&Time,(aint)&TempMat);
				//			fp32 Time = 0;
				//			CWObject_Message Msg(OBJMSG_HOOK_GETRENDERMATRIX, (int)&Time, (int)&MatIP);
				if(!_pWPhysState->Phys_Message_SendToObject(Msg, pCD->m_RenderAttached))
					TempMat = pObj->GetPositionMatrix();
			}
			else
			{
				CWObject_Message Msg(OBJMSG_HOOK_GETCURRENTMATRIX, (aint)&TempMat);
				//			fp32 Time = 0;
				//			CWObject_Message Msg(OBJMSG_HOOK_GETRENDERMATRIX, (int)&Time, (int)&MatIP);
				if(!_pWPhysState->Phys_Message_SendToObject(Msg, pCD->m_RenderAttached))
					TempMat = pObj->GetPositionMatrix();
			}
		}
	}

	if (_pCharPos)
		OnGetAnimState(_pObj, _pWPhysState, pSkel, 1, *_pCharPos, _IPTime, _Anim);
	else
		OnGetAnimState(_pObj, _pWPhysState, pSkel, 1, _pObj->GetPositionMatrix(), _IPTime, _Anim);

	_pInstance = GetClientSkelInstance(pCD, 1);
	if(!_pInstance)
		return false;

	_pSkeleton = pSkel;
	return true;
}

bool CWObject_Character::GetEvaluatedPhysSkeleton(CXR_SkeletonInstance *&_pInstance, CXR_Skeleton *&_pSkeleton, CXR_AnimState& _Anim, fp32 _IPTime, const CMat4Dfp32* _pCharPos)
{
	MAUTOSTRIP(CWObject_Character_GetEvaluatedSkeleton_2, false);
	return GetEvaluatedPhysSkeleton(this, m_pWServer, _pInstance, _pSkeleton, _Anim, _IPTime, _pCharPos);
}

void CWObject_Character::OnPress()
{
	MAUTOSTRIP(CWObject_Character_OnPress, MAUTOSTRIP_VOID);
	
	MSCOPE(CWObject_Character::OnPress, CHARACTER);
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if (!pCD) return;

	int ControlPress = pCD->m_Control_Press; //  | pCD->m_Control_Press_Intermediate
	int ControlLastPress = pCD->m_Control_LastPress;
	bool bIsMP = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), m_pWServer->Game_GetObjectIndex()) ? true : false;

	if (Char_GetControlMode(this) == PLAYER_CONTROLMODE_LEDGE2)
	{
		CWAG2I_Context AGContext(this,m_pWServer,CMTime::CreateFromTicks(pCD->m_GameTick,m_pWServer->GetGameTickTime(),pCD->m_PredictFrameFrac));
		CWObject_Ledge::OnPress(&AGContext,pCD,pCD->m_Control_Press,pCD->m_Control_LastPress);
		pCD->m_Control_LastPress = ControlPress;
		return;
	}

	// Multiplayer respawn?
	if (Char_GetPhysType(this) == PLAYER_PHYS_DEAD)
	{
		if(ControlPress & CONTROLBITS_PRIMARY || ControlPress & CONTROLBITS_SECONDARY)
		{
			if(bIsMP)
			{
				CWObject_Message Msg(OBJMSG_GAME_RESPAWN);
				Msg.m_iSender = pCD->m_iPlayer;
				m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());
			}
		}
		return;
	}

	// Skip dialogue?
	bool bIsPressingDrainButton = (pCD->m_Control_Press & CONTROLBITS_BUTTON5) != 0;
	bool bPressedUseButton = (pCD->m_Control_Press & CONTROLBITS_BUTTON0 && !(pCD->m_Control_LastPress & CONTROLBITS_BUTTON0));
	bool bPressedJumpButton = (pCD->m_Control_Press & CONTROLBITS_JUMP && !(pCD->m_Control_LastPress & CONTROLBITS_JUMP));
	if ((pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_DIALOGUE) && !bIsPressingDrainButton && (bPressedUseButton || bPressedJumpButton))
	{
		bool bHaveChoices = (m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETCHOICES,0,0,m_iObject), pCD->m_iFocusFrameObject) > 0);
		if (pCD->m_iFocusFrameObject != -1 && !bHaveChoices)
		{
			Char_SkipDialogue();
		}
	}

	// Redirect input to turret?
	if (pCD->m_iMountedObject && (pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_TURRET))
	{
		m_pWServer->Phys_Message_SendToObject(CWObject_Message(CWObject_Turret::OBJMSG_TURRET_HANDLEINPUT,ControlPress,ControlLastPress),pCD->m_iMountedObject);
		pCD->m_Control_LastPress = ControlPress;
		return;
	}

	bool bPlayerView = Char_IsPlayerViewControlled(this);
	if (pCD->m_DarknessPowersAvailable && ((pCD->m_iPlayer != -1) || m_bNonPlayerDarknessUser))
	{
		if (bPlayerView)
			Char_CheckDarknessActivation(ControlPress,ControlLastPress,pCD->m_Control_Move);
		// Update creepingdark turnspeed
		if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK)
		{
			CWObject_CreepingDark::SetMoveSpeed(m_pWServer, pCD->m_iCreepingDark,pCD->m_Control_Move[0], pCD->m_Control_Move[1]);
			if (pCD->m_Control_Hold & CONTROLBITS_BUTTON2 || pCD->m_Control_Press & CONTROLBITS_BUTTON5)
				CWObject_CreepingDark::SetWallClimb(m_pWServer, pCD->m_iCreepingDark, true);	
			else
				CWObject_CreepingDark::SetWallClimb(m_pWServer, pCD->m_iCreepingDark, false);	
			if(ControlPress & CONTROLBITS_BUTTON5 && !(ControlLastPress & CONTROLBITS_BUTTON5))
				m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CREEPINGDARK_DEVOUR), pCD->m_iCreepingDark);
		}
		else if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DEMONARM)
		{
			CWObject* pTentacleObj = m_pWServer->Object_Get(pCD->m_iDarkness_Tentacles);
			if (pTentacleObj)// && pCD->m_Control_Move.LengthSqr() > 0.15f)
			{
				CWObject_TentacleSystem& TentacleSystem = *safe_cast<CWObject_TentacleSystem>(pTentacleObj);
				TentacleSystem.UpdateArmControl(pCD->m_Control_Move);
			}
		}
	}

	if (pCD->m_iPlayer != -1)
	{
		// Need char
		CWAG2I_Context AGContext(this,m_pWServer,pCD->m_GameTime);
		CRPG_Object_Inventory * pWeaponInv = Char()->GetInventory(AG2_ITEMSLOT_WEAPONS);
		int32 iCurEquipped = pWeaponInv->GetFinalSelectedItemIndex();
		CRPG_Object_Item* pCurItem = pWeaponInv->FindFinalSelectedItem();//Char()->GetFinalSelectedItem(AG2_ITEMSLOT_WEAPONS);
		bool bIsPressLeft = ((ControlPress & CONTROLBITS_DPAD_LEFT) && !(ControlLastPress & CONTROLBITS_DPAD_LEFT));
		bool bIsPressRight = ((ControlPress & CONTROLBITS_DPAD_RIGHT) && !(ControlLastPress & CONTROLBITS_DPAD_RIGHT));
		bool bIsPressDown = ((ControlPress & CONTROLBITS_DPAD_DOWN) && !(ControlLastPress & CONTROLBITS_DPAD_DOWN));
	//	bool bIsPressUp = ((ControlPress & CONTROLBITS_DPAD_UP) && !(ControlLastPress & CONTROLBITS_DPAD_UP));
		bool bIsSpawningDarkling = (ControlPress & CONTROLBITS_BUTTON1) && (m_iBestDarklingSpawn > 0);

		if (!m_GrabbedObject.IsActive() && pCurItem && !(pCurItem->m_Flags2 & RPG_ITEM_FLAGS2_NOTSELECTABLE) && !bIsSpawningDarkling &&
			!(pCD->m_Disable & PLAYER_DISABLE_SWITCHWEAPON) && !(ControlPress & CONTROLBITS_BUTTON1 && m_iBestDarklingSpawn))
		{
			//
			// Use left/right to cycle through the following weapon categories:  0-Unequipped, 1-Pistols, 2-Shotguns, 3-AK47
			if (bIsPressLeft || bIsPressRight)
			{
				// Deactivate ancientweapons when switching to guns
				if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS)
					Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS, true, false);

				Char_EquipNewItem(pCurItem,bIsPressLeft);
			}
			if (bIsPressDown)
			{
				// Equip unarmed
				CRPG_Object_Item* pMelee = Char()->FindItemByType(CRPG_Object_Fist_ItemType);
				if (pMelee)
				{
					int PreviousWeaponId = pCurItem->m_Identifier;
					int PreviousItemType = pCurItem->m_iItemType;
					int ItemIndex = Char()->GetInventory(0)->FindItemIndexByIdentifier(pMelee->m_Identifier);
					int ItemIndexPrev = Char()->GetInventory(0)->FindItemIndexByIdentifier(pCurItem->m_Identifier);
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
		}

		if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), m_pWServer->Game_GetObjectIndex()) && (ControlPress & CONTROLBITS_BUTTON0) && !(ControlLastPress & CONTROLBITS_BUTTON0))
		{
			if (pCurItem && !(pCurItem->m_Flags2 & RPG_ITEM_FLAGS2_THROWAWAYONEMPTY) && pCurItem->GetMagazines())
			{
				CRPG_Object_Summon *pWeap = TDynamicCast<CRPG_Object_Summon>(pCurItem);
				bool bReload = true;
				if(pWeap && pWeap->m_AmmoLoad == pWeap->m_MaxAmmo)
					bReload = false;
				if(bReload)
				{
					pCD->m_AnimGraph2.SendAttackImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION, AG2_IMPULSEVALUE_ITEMACTION_RELOAD));
				}
			}
		}
	}

	if (!m_GrabbedObject.IsActive() && !(pCD->m_Disable & PLAYER_DISABLE_ATTACK) && 
		!(pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_BLOCKACTIVE) && 
		Char_GetPhysType(this) != PLAYER_PHYS_DEAD &&
		Char_GetControlMode(this) != PLAYER_CONTROLMODE_ACTIONCUTSCENE)
	{
		// In creeping dark mode, do attack
		if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK)
		{
			// Send creeping dark attack
			if ((ControlPress & CONTROLBITS_PRIMARY) && !(ControlLastPress & CONTROLBITS_PRIMARY))
			{
				m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CREEPINGDARK_DOATTACK),pCD->m_iCreepingDark);
			}
		}
		else
		{
			if((ControlPress & CONTROLBITS_PRIMARY && !(ControlLastPress & CONTROLBITS_PRIMARY) && pCD->m_PostAnimSystem.GetSafeToFirePrim()) ||
				(ControlPress & CONTROLBITS_SECONDARY && !(ControlLastPress & CONTROLBITS_SECONDARY) && pCD->m_PostAnimSystem.GetSafeToFireSec()))
			{
				SetGunplayUnsafe(pCD->m_GameTick);
				// Check if any weapon is currently active if so activate it, otherwise we equip the
				// last active weapon
				// Get equippeditem
				CRPG_Object_Item* pItem = Char()->GetFinalSelectedItem(0);
				if (pCD->m_SelectItemTimeOutTick == 0 && pItem && pItem->m_iItemType == RPG_ITEMTYPE_FIST && 
					!(pCD->m_Disable & PLAYER_DISABLE_SWITCHWEAPON))
				{

					CRPG_Object_Item* pLastItem = Char()->FindItemByIdentifier(m_LastUsedItem,AG2_ITEMSLOT_WEAPONS);
					if (!pLastItem || pLastItem->m_iItemType == 7 || (pLastItem->m_Flags2 & RPG_ITEM_FLAGS2_PERMANENT))
					{
						// Find a good candidate for last used item (gun)
						CRPG_Object_Item* pGun = Char()->FindNextItemForReloadByType(pItem->m_Identifier,1, pLastItem);
						if (pGun)
							m_LastUsedItem = pGun->m_Identifier;
						else if (pLastItem && pLastItem->m_iItemType == 7)
						{
							pLastItem = NULL;
							m_LastUsedItem = 0;
							m_LastUsedItemType = -1;
						}
					}
					// Check if we have a last selected item type, otherwise I'll guess we have to 
					// use fists
					if (m_LastUsedItem && Char()->SelectItemByIdentifier(m_LastUsedItem,0,true,true))
					{
						ResetWeaponUnequipTimer();
						// Make animgraph quickequip...
						CRPG_Object_Item* pEquippedItem = Char()->GetFinalSelectedItem(0);
						int32 ItemClass = pCD->m_EquippedItemClass;
						if (pEquippedItem && (ItemClass != pEquippedItem->m_AnimProperty))
						{
							if (pEquippedItem->m_iItemType == 1)
							{
								CRPG_Object_Summon* pSummon = safe_cast<CRPG_Object_Summon>(pEquippedItem);
								Char()->GetInventory(AG2_ITEMSLOT_WEAPONS)->ReloadItemFromOthers(pSummon);
							}
							// Item might have been removed
							pCD->m_EquippedItemClass = (uint16)pEquippedItem->m_AnimProperty;
							pCD->m_EquippedItemType = (uint16)pEquippedItem->m_iItemType;
							pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER,pEquippedItem->m_Identifier);
							pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_ITEMANIMTYPE,pEquippedItem->m_AnimType);
							CWAG2I_Context Context(this,m_pWServer,pCD->m_GameTime);
							pCD->m_AnimGraph2.UpdateImpulseState(&Context);
							
							// Show weapon icon
							pCD->m_Pickup_iIcon = (uint16)pEquippedItem->m_iIconSurface;
							pCD->m_Pickup_StartTick = pCD->m_GameTick;
							pCD->m_Pickup_Magazine_Num = (int8)(Max(-1, Min(127, pEquippedItem->ShowNumMagazines())));
						}
					}
					pCD->m_WeaponUnequipTick = 0;
				}
			}

			//  TEMP REMOVE, send impulse to ag2 about to fire the weapon (reload?)
			CWAG2I_Context Context(this,m_pWServer,CMTime::CreateFromTicks(pCD->m_GameTick,m_pWServer->GetGameTickTime(),pCD->m_PredictFrameFrac));
			CRPG_Object_Char* pChar = Char();
			CRPG_Object_Item* pEquippedItem = pChar->GetEquippedItem(AG2_ITEMSLOT_WEAPONS);
			bool bNoAmmoPrimary = false;
			bool bNoAmmoSecondary = false;
			if (pEquippedItem)
			{
				bool bActivated = false;
				if (ControlPress & CONTROLBITS_PRIMARY)
				{
					pCD->m_WeaponStatus0.m_AmmoLoad = pEquippedItem->GetAmmo(pChar);
					if (pCD->m_WeaponStatus0.OkToFire(pCD,m_pWServer) && pEquippedItem && pEquippedItem->IsActivatable(pChar,m_iObject) && 
						pEquippedItem->m_iExtraActivationWait < pCD->m_GameTick && pCD->m_PostAnimSystem.GetSafeToFirePrim())
					{
						bActivated = true;
						if (!(pEquippedItem->m_Flags2 & RPG_ITEM_FLAGS2_DRAINSDARKNESS))
						{
							// Check if we have a victim present, if so kill him good 
							int8 FocusType = (pCD->m_FocusFrameType & SELECTION_MASK_TYPEINVALID);
							int32 iFocusObj = pCD->m_iFocusFrameObject;
							if (FocusType && FocusType != SELECTION_CHAR)
							{
								// Try to find a char instead
								Char_FindBestOpponent(m_pWServer,this,iFocusObj);
							}
							// Do Gunkata
							if (iFocusObj != -1)
								Char_DoGunKata(pEquippedItem->m_AnimType, iFocusObj);
						}
						// Move to atleast this tick
						pEquippedItem->m_iExtraActivationWait = pCD->m_GameTick;;
						//CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_DIALOG,AG2_IMPULSEVALUE_DIALOG_INIT)
						if (pCD->m_AnimGraph2.SendAttackImpulse(&Context,CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_PRIMARYATTACK),AG2_TOKEN_MAIN))
						pCD->m_WeaponStatus0.Fire(pCD, (pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_GODMODE) != 0);
						pCD->m_WeaponStatus0.m_Flags |= CAutoVar_WeaponStatus::WEAPONSTATUS_TRIGGERPRESSED;
						// Reset analog
						pCD->m_Analog0 = 0;
						// Signal "need reset"
						pCD->m_AnalogMode = pCD->m_AnalogMode | M_Bit(0);
						if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), m_pWServer->Game_GetObjectIndex()))
						{
							CWO_CharShapeshifter_ClientData& CD = CWObject_CharShapeshifter::GetClientData(this);
							if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_DAMAGE)
								m_pWServer->Sound_On(m_iObject, m_pWServer->GetMapData()->GetResourceIndex_Sound("Env_Itm_Mult_Damage_Hit"), WCLIENT_ATTENUATION_3D);
						}
					}
				}
				bNoAmmoPrimary = !bActivated && ((pEquippedItem->m_Flags2 & RPG_ITEM_FLAGS2_DRAINSDARKNESS) ? ((pEquippedItem->m_Flags2 & RPG_ITEM_FLAGS2_DRAINSDARKNESS) && (pCD->m_Darkness < pEquippedItem->GetAmmoDraw())) : !(pEquippedItem->GetAmmo(Char()) || pEquippedItem->GetMagazines()));
			}
			// Check dual wield item
			CRPG_Object_Item* pEquippedItemDualWield = pChar->GetEquippedItem(AG2_ITEMSLOT_DUALWIELD);
			if (pEquippedItemDualWield)
			{
				bool bActivated = false;
				if (ControlPress & CONTROLBITS_SECONDARY)
				{
					pCD->m_WeaponStatus1.m_AmmoLoad = pEquippedItemDualWield->GetAmmo(pChar);
					if (pCD->m_WeaponStatus1.OkToFire(pCD,m_pWServer) && pEquippedItemDualWield->IsActivatable(pChar,m_iObject) && 
						pEquippedItemDualWield->m_iExtraActivationWait < pCD->m_GameTick && pCD->m_PostAnimSystem.GetSafeToFireSec())
					{
						bActivated = true;
						if (!(pEquippedItemDualWield->m_Flags2 & RPG_ITEM_FLAGS2_DRAINSDARKNESS))
						{
							// Check if we have a victim present, if so kill him good 
							int8 FocusType = (pCD->m_FocusFrameType & SELECTION_MASK_TYPEINVALID);
							int32 iFocusObj = pCD->m_iFocusFrameObject;
							if (FocusType && FocusType != SELECTION_CHAR)
							{
								// Try to find a char instead
								Char_FindBestOpponent(m_pWServer,this,iFocusObj);
							}
							// Do Gunkata
							if (iFocusObj != -1)
								Char_DoGunKata(pEquippedItemDualWield->m_AnimType, iFocusObj);
						}
						// Move to atleast this tick
						pEquippedItemDualWield->m_iExtraActivationWait = pCD->m_GameTick;
						CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_SECONDARYATTACK);
						//CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_DIALOG,AG2_IMPULSEVALUE_DIALOG_INIT);
						if (pCD->m_AnimGraph2.SendAttackImpulse(&Context,Impulse,AG2_TOKEN_MAIN))
						pCD->m_WeaponStatus1.Fire(pCD, (pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_GODMODE) != 0);
						pCD->m_WeaponStatus1.m_Flags |= CAutoVar_WeaponStatus::WEAPONSTATUS_TRIGGERPRESSED;
						// Reset analog
						pCD->m_Analog1 = 0;
						// Signal "need reset"
						pCD->m_AnalogMode = pCD->m_AnalogMode | M_Bit(1);
						if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), m_pWServer->Game_GetObjectIndex()))
						{
							CWO_CharShapeshifter_ClientData& CD = CWObject_CharShapeshifter::GetClientData(this);
							if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_DAMAGE)
								m_pWServer->Sound_On(m_iObject, m_pWServer->GetMapData()->GetResourceIndex_Sound("Env_Itm_Mult_Damage_Hit"), WCLIENT_ATTENUATION_3D);
						}
					}
				}
				bNoAmmoSecondary = !bActivated && ((pEquippedItemDualWield->m_Flags2 & RPG_ITEM_FLAGS2_DRAINSDARKNESS) ? ((pEquippedItemDualWield->m_Flags2 & RPG_ITEM_FLAGS2_DRAINSDARKNESS) && (pCD->m_Darkness < pEquippedItemDualWield->GetAmmoDraw())) : !(pEquippedItemDualWield->GetAmmo(Char()) || pEquippedItemDualWield->GetMagazines()));
			}

			bool bAncientSafe = true;
			if (pEquippedItem && pEquippedItem->m_Flags2 & RPG_ITEM_FLAGS2_DRAINSDARKNESS)
			{
				if (bNoAmmoPrimary)
				{
					// Out of ammo, check if there's more guns with ammo
					CRPG_Object_Item* pNextItem = pChar->FindNextItemNotEquippedByGroupType(pEquippedItem->m_Identifier,AG2_IMPULSEVALUE_GROUPTYPE_GUN,AG2_ITEMSLOT_WEAPONS,7,true,pEquippedItem);
					if (pNextItem)
						bAncientSafe = false;
				}
			}

			if (!((pCD->m_AnimGraph2.GetStateFlagsLoToken3() | pCD->m_AnimGraph2.GetStateFlagsLoToken2()) & 
				(AG2_STATEFLAG_EQUIPPING|AG2_STATEFLAG_RELOADING)) && !(pCD->m_Disable & PLAYER_DISABLE_SWITCHWEAPON))
			{
				bool bDidMelee = false;
				//Char_EquipNewItem(pEquippedItem,false,true);
				if ((ControlPress & CONTROLBITS_SECONDARY) && !(ControlLastPress & CONTROLBITS_SECONDARY) && 
					(bNoAmmoSecondary || !pEquippedItemDualWield) && bAncientSafe)
				{
					if (pCD->m_AnimGraph2.SendAttackImpulse(&Context,CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION,AG2_IMPULSEVALUE_ITEMACTION_MELEEPRIMARY),AG2_TOKEN_MAIN))
					{
						bDidMelee = true;
						int8 FocusType = (pCD->m_FocusFrameType & SELECTION_MASK_TYPEINVALID);
						int32 iFocusObj = pCD->m_iFocusFrameObject;
						if (FocusType && FocusType != SELECTION_CHAR)
						{
							// Try to find a char instead
							Char_FindBestOpponent(m_pWServer,this,iFocusObj);
						}
						if (iFocusObj != -1)
						{
							pCD->m_iFightingCharacter = (m_pWServer->Object_GetPosition(iFocusObj) - GetPosition()).LengthSqr() < Sqr(35.0f) ? iFocusObj : -1;
							/*CWObject_CoreData* pObj = m_pWServer->Object_GetCD(iFocusObj);
							CWO_Character_ClientData* pCDTarget = pObj ? GetClientData(pObj) : NULL;
							if (pCDTarget)
							{
							// Make sure he's sufficiently turned towards us
							CWAG2I_Context TargetContext(pObj,m_pWServer,Context.m_GameTime);
							pCDTarget->m_AnimGraph2.GetAG2I()->SendImpulse(&TargetContext,CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,AG2_IMPULSEVALUE_GAMEPLAY_MELEERESPONSE),AG2_TOKEN_MAIN);
							}*/
						}
					}
				}
				if (!bDidMelee)
				{
					CMat4Dfp32 Mat;
					GetActivatePosition(Mat);
					bool bDidntFjuff = true;
					// Activate "click" sound
					if (bNoAmmoPrimary && (ControlPress & CONTROLBITS_PRIMARY) && (!(ControlLastPress & CONTROLBITS_PRIMARY) || (pEquippedItem->m_Flags2 & RPG_ITEM_FLAGS2_AUTOFJUFF)))
					{
						pChar->ActivateItem(AG2_ITEMSLOT_WEAPONS, Mat, m_iObject, 1, true);
						bDidntFjuff = false;
					}
					if (pEquippedItemDualWield && bNoAmmoSecondary && (ControlPress & CONTROLBITS_SECONDARY) && (!(ControlLastPress & CONTROLBITS_SECONDARY) || (pEquippedItemDualWield->m_Flags2 & RPG_ITEM_FLAGS2_AUTOFJUFF)))
					{
						pChar->ActivateItem(AG2_ITEMSLOT_DUALWIELD, Mat, m_iObject, 1, true);
						bDidntFjuff = false;
					}
					if (bDidntFjuff)
						m_LastNotFjuffed = pCD->m_GameTick;
					// When out of darknessjuice and have tried to activate for a second, deactivate darkness guns
					if (!bAncientSafe && (pEquippedItem->m_Flags2 & RPG_ITEM_FLAGS2_DRAINSDARKNESS) && 
						bNoAmmoPrimary && bNoAmmoSecondary && ((pCD->m_GameTick - m_LastNotFjuffed) > m_pWServer->GetGameTicksPerSecond())						)
					{
						Char_ActivateDarknessPower(PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS, true, false);
					}
				}
			}
		}
	}

	// Hmm not sure this should be here...?
	if(ControlLastPress & CONTROLBITS_PRIMARY && !(ControlPress & CONTROLBITS_PRIMARY))
	{
		//if ((!IsBot()) || (Char_GetControlMode(pCD->m_pObj) != PLAYER_CONTROLMODE_ANIMATION))
		{	// See above, deactivations may require animations as well (bows etc)
			CMat4Dfp32 Mat;
			GetActivatePosition(Mat);
			Char()->DeactivateItem(0, Mat, m_iObject, CONTROLBITS_PRIMARY);
			//Char()->DeactivateItem(2, Mat, m_iObject, CONTROLBITS_PRIMARY);
		}
		pCD->m_WeaponStatus0.m_Flags &= ~CAutoVar_WeaponStatus::WEAPONSTATUS_TRIGGERPRESSED;
	}
	if(ControlLastPress & CONTROLBITS_SECONDARY && !(ControlPress & CONTROLBITS_SECONDARY))
	{
		//if ((!IsBot()) || (Char_GetControlMode(pCD->m_pObj) != PLAYER_CONTROLMODE_ANIMATION))
		{	// See above, deactivations may require animations as well (bows etc)
			CMat4Dfp32 Mat;
			GetActivatePosition(Mat);
			Char()->DeactivateItem(2, Mat, m_iObject, CONTROLBITS_PRIMARY);
		}
		pCD->m_WeaponStatus1.m_Flags &= ~CAutoVar_WeaponStatus::WEAPONSTATUS_TRIGGERPRESSED;
	}

#ifdef WADDITIONALCAMERACONTROL
	if (pCD->m_iPlayer != -1 && (!(pCD->m_Control_LastPressAdditional & CONTROLBITS_BUTTON4) && pCD->m_Control_PressAdditional & CONTROLBITS_BUTTON4))
	{
		pCD->m_bAdditionalCameraControl = !pCD->m_bAdditionalCameraControl;
		if (pCD->m_bAdditionalCameraControl)
		{
			pCD->m_AdditionalCameraPos = GetPosition();
			pCD->m_AdditionalCameraPos.m_Value[2] += 60.0f;
			pCD->m_AdditionalCameraPos.MakeDirty();
			pCD->m_AdditionalCameraControlY = pCD->m_Control_Look_Wanted[1];
			pCD->m_AdditionalCameraControlZ = pCD->m_Control_Look_Wanted[2];
		}
	}
	pCD->m_Control_LastPressAdditional = pCD->m_Control_PressAdditional;
#endif

	if(pCD->m_iPlayer == -1 || bPlayerView)
	{
		if(ControlPress & CONTROLBITS_BUTTON0 && !(pCD->m_Disable & PLAYER_DISABLE_ACTIVATE) && bPlayerView)
		{
			if(pCD->m_iFightingCharacter == -1 && !(m_ClientFlags & PLAYER_CLIENTFLAGS_DIALOGUE) &&
				!pCD->m_Phys_bInAir && (Char_GetControlMode(this) == PLAYER_CONTROLMODE_FREE) && 
				!pCD->m_RelAnimPos.HasQueued(pCD->m_GameTick))
			{
				int iBest = -1;
				if ((pCD->m_FocusFrameType & SELECTION_MASK_TYPEINVALID) == SELECTION_CHAR)
				{
					// If the focusframe selection is a character, use that instead
					iBest = (int)pCD->m_iFocusFrameObject;	
				}

				// Try to find a dead char to devour
				if (!(ControlLastPress & CONTROLBITS_BUTTON0))// temp fix. don't devour and grab at the same time..
				{
					if (m_GrabbedObject.IsActive())
					{
						// Release currently grabbed object
						CWObject* pObj = m_pWServer->Object_Get(m_GrabbedObject.m_iTargetObj);

						CVec3Dfp32 Move = pCD->m_Control_Move;
						if (pObj)
						{
							if (Move.k[0] > 0.0f)
							{
								// throw object
								const CMat4Dfp32& MatLook = GetPositionMatrix();
								CVec3Dfp32 Force = MatLook.GetRow(0) * Move.k[0];
								fp32 Mass = pObj->GetMass();
								Force *= Mass * 100.0f; //tweakme
								m_pWServer->Phys_AddImpulse(m_GrabbedObject.m_iTargetObj, m_GrabbedObject.m_Pid[0].m_LastPos, Force);
							}
						}
						m_GrabbedObject.Release();
					}
					else
					{
						// TEMP
						// Find and activate stuff... (only if no effect anim is playing)
						int32 iSel = -1, iCloseSel = -1;
						int8 SelType = 0;
						bool bCurrentNoPrio = false;
						CWObject_Character::Char_FindStuff(m_pWServer,this,pCD,iSel,
							iCloseSel, SelType,bCurrentNoPrio);


						if (iSel != -1)
						{
							if (0) // disabled for X06 -- (SelType & SELECTION_MASK_TYPE) == SELECTION_GRABBABLE_OBJECT)
							{
								// Only grab when unarmed
								if (pCD->m_AnimGraph2.GetPropertyInt(PROPERTY_INT_STANCESTANDING) == AG2_IMPULSEVALUE_WEAPONTYPE_UNARMED)
								{
									// Grab selected object
									m_GrabbedObject.Init(*m_pWServer, iSel, GetPositionMatrix().GetRow(0));

									CWObject* pObj = m_pWServer->Object_Get(iSel);
									if (pObj)
										pObj->m_iOwner = m_iObject;
									iBest = -1;
								}
							}
							else
							{
								CWObject_Character::Char_ActivateStuff(m_pWServer,this,pCD,
									iSel,SelType);
							}
						}
						else
						{
							// If no other objects were found, try to find a ledge instead
							//ConOut("Grabbing ledge");
							CWObject_Character::Char_GrabLedge(m_pWServer,this,pCD);
						}
					}
				}

				if (iBest != -1)
				{
					// Never allow to enter many screens at once
					CWObject* pTarget = m_pWServer->Object_Get(iBest);
					CWO_Character_ClientData* pCDTarget = (pTarget ? GetClientData(pTarget) : NULL);
					//if(!(pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_NOUSEMASK) && m_Player.m_LastInfoscreenPress < int(m_pWServer->GetGameTick()) - 5 
					//	&& m_PendingCutsceneTick == -1 && (pCDTarget ? !pCDTarget->m_AnimGraph2.GetPropertyBool(PROPERTY_BOOL_ISSLEEPING) : false) 
					//	&& !(pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_DIALOGUE))
						
					if(!(pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_NOUSEMASK) && m_Player.m_LastInfoscreenPress < int(m_pWServer->GetGameTick()) - 5 
						&& m_PendingCutsceneTick == -1 && (pCDTarget ? !pCDTarget->m_AnimGraph2.GetPropertyBool(PROPERTY_BOOL_ISSLEEPING) : false)) 
					{
						//M_TRACEALWAYS(CStrF("%i. OnUse %s", m_pWServer->GetGameTick(), pTarget->GetName()));
						m_Player.m_LastInfoscreenPress = m_pWServer->GetGameTick();
						CWObject_Message Msg(OBJMSG_CHAR_USE);
						Msg.m_iSender = m_iObject;

						CWObject_Character *pChar = CWObject_Character::IsCharacter(iBest, m_pWServer);
						if(pChar)
						{
							//fp32 DirCheck = -((GetPositionMatrix().GetRow(0) * pChar->GetPositionMatrix().GetRow(0)));
							//if(DirCheck > 0.2f)
							//{
								pCD->m_3PI_FocusObject = iBest;
								pCD->m_3PI_Mode = THIRDPERSONINTERACTIVE_MODE_DIALOGUE;
								pChar->Char_SetListener(m_iObject);
								m_iLastAutoUseObject = iBest;
								m_pWServer->Message_SendToObject(Msg, iBest);
							//}
							//else
							//{
							//	m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_DIALOGUE_SPEAKER, m_iObject), iBest);
							//}
						}
						else
						{
							m_iLastAutoUseObject = iBest;
							m_pWServer->Message_SendToObject(Msg, iBest);
						}
						// Drop dragged body
						m_pWServer->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETGRABBEDBODY,0), m_iObject);
					}
				}
				/*else if(pCD->m_FocusFrameType == SELECTIONNONE && m_Player.m_LastInfoscreenPress < int(m_pWServer->GetGameTick()) - 5)
					pCD->m_HealthHudStart = pCD->m_GameTick;*/
			}
		}

		if(ControlPress & CONTROLBITS_BUTTON3 && !(ControlLastPress & CONTROLBITS_BUTTON3))
		{
			CRPG_Object_Item* pEquippedItem = Char()->GetEquippedItem(AG2_ITEMSLOT_WEAPONS);
			if (pEquippedItem && !(pEquippedItem->m_Flags2 & RPG_ITEM_FLAGS2_THROWAWAYONEMPTY) && pEquippedItem->GetMagazines() 
				&& pEquippedItem->NeedReload(m_iObject,true))
			{
				CWAG2I_Context AGContext(this, m_pWServer, pCD->m_GameTime);
				pCD->m_AnimGraph2.SendAttackImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION, AG2_IMPULSEVALUE_ITEMACTION_RELOAD));
			}
		}

		if(ControlPress & CONTROLBITS_BUTTON1 && !(ControlLastPress & CONTROLBITS_BUTTON1))
		{
			// On player this is moved to animgraph
			if (pCD->m_iPlayer == -1)
				if (!(Char_IsSwimming() || Char_IsClimbing() || Char_IsFighting(this)))
					NextItem(RPG_CHAR_INVENTORY_WEAPONS, false);
		}
		
		if(pCD->m_iPlayer != -1 && !bIsMP)
		{
			if ((ControlPress & CONTROLBITS_BUTTON4) && !(Char_IsFighting(this)))
			{
				// Never allow to enter many screens at once
				if(m_Player.m_LastInfoscreenPress < int(m_pWServer->GetGameTick()) - 5 && m_PendingCutsceneTick == -1)
				{
					m_Player.m_LastInfoscreenPress = m_pWServer->GetGameTick();
					
					// Open up the mission journal window
					//Char_ShowMissionJournal();

					// changed to pretty much normal gui system
					Char_UpdateQuestItems();
					bool bGotoDarkness = false;

					// check where to go: Prio. 1 - missions, 2 darkness powers, 3 darklings	
					// check if any new darklings
					if(pCD->m_InventoryInfo.m_iGUITaggedItem == -1)
					{
						if(pCD->m_GUITaggedNewDarknessPower)
							bGotoDarkness = true;
						else
						{
							for(int i = 0; i < m_lAvailableDarklings.Len(); i++)
							{
								if(m_lAvailableDarklings[i].m_bTaggedForFocus)
								{
									bGotoDarkness = true;
									break;
								}
							}
						}
					}
					
					if(bGotoDarkness)
						ConExecute("removepadlock(); cg_rootmenu(\"Darklings\")");
					else
						ConExecute("removepadlock(); cg_rootmenu(\"Mission_Item\")");		
				}
			}

			/*
			if(ControlPress & CONTROLBITS_BUTTON3 && !(ControlLastPress & CONTROLBITS_BUTTON3))
			{
				bool bReload = true;
#ifdef PLATFORM_CONSOLE
				if(Char_IsPlayerView(this) && m_PendingCutsceneTick == -1 && m_Player.m_LastInfoscreenPress < int(m_pWServer->GetGameTick()) - 5)
				{
					// Never allow to enter many screens at once
					if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_SHOWPENDINGGAMEMSG), m_pWServer->Game_GetObjectIndex()))
					{
						m_Player.m_LastInfoscreenPress = m_pWServer->GetGameTick() + 10; // 6 ticks before pause
						bReload = false;
					}
				}
#endif

				if(bReload)
				{
					CRPG_Object_Item *pItem = Char()->GetEquippedItem(0);
					if(pItem)
						pItem->ForceReload(m_iObject);
				}	
			}
			*/

			/*
			if(ControlPress & CONTROLBITS_BUTTON4 && !(ControlLastPress & CONTROLBITS_BUTTON4))
			{
				CRPG_Object_Item *pItem = Char()->GetEquippedItem(0);
				if(pItem)
					pItem->CycleMode(m_iObject);

				pCD->m_PannLampa = !pCD->m_PannLampa;
			}
			*/
/*
			if(ControlPress & CONTROLBITS_BUTTON5 && !(ControlLastPress & CONTROLBITS_BUTTON5))
			{
				int NewTarget = 0;
				switch(pCD->m_Zoom.m_Target)
				{
				case 0: NewTarget = 170; break;
				//case 85: NewTarget = 170; break;
				case 170: NewTarget = 255; break;
				}
				if(pCD->m_Zoom.Set(NewTarget, pCD->m_GameTick + 2, 0.25f))
					pCD->m_Zoom.MakeDirty();
			}
*/
		}
	}

	pCD->m_Control_LastPress = ControlPress;
}

#ifndef M_RTM
void CWObject_Character::AI_GetControlFrame(CControlFrame& _Ctrl)
{
	MAUTOSTRIP(CWObject_Character_AI_GetControlFrame, MAUTOSTRIP_VOID);
	//Bot control flags are set by controller. If controller is dead or non-existent
	//clear control flags and controller.
	CWObject_Character * pController = NULL;
	if ( !m_iController || 
		 !(pController = (CWObject_Character*)m_pWServer->Object_Get(m_iController)) ||
		 (Char_GetPhysType(pController) == PLAYER_PHYS_DEAD) )
	{
		m_iController = 0;
		m_iBotControl = 0; 
	};
	
	//Take actions
	m_spAI->OnBotControl(_Ctrl, m_iBotControl, pController);

	//Clear bot control...wrong way to do it...will fix...
	m_iBotControl = 0;
}
#endif

int CWObject_Character::OnPumpControl()
{
	MAUTOSTRIP(CWObject_Character_OnPumpControl, 0);
	MSCOPE(CWObject_Character::OnPumpControl, CHARACTER);

	// Get clientdata
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if (!pCD) return 1;

/*	CBox3Dfp32 SelectBound;
	if (!(pCD->m_Phys_IdleTicks > PLAYER_PHYS_IDLETICKS))
	{
		SelectBound = *GetAbsBoundBox();
		CBox3Dfp32 Bound2(SelectBound);

		Bound2.m_Min += GetMoveVelocity();
		Bound2.m_Max += GetMoveVelocity();
		Bound2.m_Min -= 16.0f;
		Bound2.m_Max += 16.0f;
		SelectBound.Expand(Bound2);

		TSelection<CSelection::LARGE_BUFFER> Selection;
		m_pWServer->Selection_AddBoundBox(selection->m_iPhysSelection, 
			GetPhysState().m_ObjectFlags | 
			GetPhysState().m_ObjectIntersectFlags | 
			m_IntersectNotifyFlags, 
			SelectBound.m_Min, SelectBound.m_Max);
		m_pWServer->Object_DisableLinkage(m_iObject);
	}*/

	CVec3Dfp32 StartPos = GetPosition();	// Logging purpose only.
	pCD->m_Control_Look_Wanted = GetLook(GetPositionMatrix());
	pCD->Char_UpdateLook(0.0f); // FIXME: UPDATELOOK
	CVec3Dfp32 OldLookWanted = pCD->m_Control_Look_Wanted;

	int CmdIDStart = 0x7fff;
	int CmdIDEnd = 0x7fff;
	const CControlFrame* pCtrl = NULL;
	
	CWO_Player* pP = NULL;
	if(pCD->m_iPlayer != -1)
	{
		CWObject_Game *pGame = m_pWServer->Game_GetObject();
		pP = pGame->Player_Get(pCD->m_iPlayer);
		pCtrl = (pP && pGame->Player_GetClient(pCD->m_iPlayer) >= 0) ? pGame->Player_GetControlFrame(pCD->m_iPlayer) : NULL;

		// Poll client control and set end control serial.
		if (pCtrl && pP)
		{
			int CmdIDEnd = 0x7fff;
			int Pos = pCtrl->GetCmdStartPos();
			while(Pos < pCtrl->m_MsgSize)
			{
				CCmd Cmd;
				pCtrl->GetCmd(Pos, Cmd);
				CmdIDEnd = Cmd.m_ID;
			}
			if (CmdIDEnd != 0x7fff)
				pP->m_ServerControlSerial = CmdIDEnd;
		}
		
		if (pCtrl && m_bAIControl)
		{
			int iObj = pGame->m_iObject;
			CWObject* pObj = m_pWServer->Object_Get(iObj);
			if (pObj)
			{
				int Pos = pCtrl->GetCmdStartPos();
				while(Pos < pCtrl->m_MsgSize)
				{
					CCmd Cmd;
					pCtrl->GetCmd(Pos, Cmd);

					if (Cmd.m_Cmd == CONTROL_CMD)
					{
						if (Cmd.m_Data[0] == CMD_SKIPINGAMECUTSCENE)
						{
							Char_SkipCutscene();
						}
					}

/*					CWObject_Message Msg(OBJMSG_GAME_CONTROL, pCD->m_iPlayer, m_iObject);
					Msg.m_pData = &Cmd;
					Msg.m_DataSize = sizeof(Cmd);

					pObj->OnMessage(Msg);*/
				}	
			}
		}
	}

	CControlFrame Tmp;

#ifndef M_RTM
	int bAIEnable = m_pWServer->Registry_GetServer()->GetValuei("CHAR_AI", 1);
	int bMoveEnable = m_pWServer->Registry_GetServer()->GetValuei("CHAR_MOVE", 1);
#else
	int bAIEnable = true;
	int bMoveEnable = true;
#endif

	//Set any suitable pause/unpause stuff based on player state here
	if (!IsBot())
	{
		//Dialogue and normal cutscene pause is set when entering/exiting these states

		//Check for action cutscene. This should be sufficient to check every few frames.
		//We could do ACS pause/unpause in ACS code instead but Olle wasn't quite sure about 
		//how they could start and end, so this is less likely to cause bugs :)
		if (m_pWServer->GetGameTick() % 5 == 0)
		{
			int ControlMode = Char_GetControlMode(this);
			bool bACSPause;
			switch (ControlMode)
			{
			//Actioncutscene modes:
			case PLAYER_CONTROLMODE_ACTIONCUTSCENE:
			case PLAYER_CONTROLMODE_LADDER:
			case PLAYER_CONTROLMODE_HANGRAIL:
			case PLAYER_CONTROLMODE_LEDGE:
			case PLAYER_CONTROLMODE_LEDGE2:
				{
					//ACS, check if current ACS wants us to pause all AI
					int iACS = pCD->m_ControlMode_Param0;
					bACSPause = (m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENE_SHOULDPAUSEALLAI), iACS) != 0);
				}
				break;
			//Non-ACS modes
			default:
                bACSPause = false;
			}

			//Pause/Unpause accordingly
			if (bACSPause)
				PauseAllAI(CAI_ResourceHandler::PAUSE_ACS);
			else
				UnpauseAllAI(CAI_ResourceHandler::PAUSE_ACS);
		}
	}


	if(!m_bAIControl && pCD->m_PendingControlFrame.m_MsgSize > 0)
		pCtrl = &pCD->m_PendingControlFrame;
	else if(bAIEnable)
	{
		if(m_bAIControl || IsBot())
		{
			//This is where a bot-character can recieve commands from the AI

#ifndef M_RTM
			if (m_iController)
			{
				//Refreshing and getting controlframe from bot controlled by other character
				m_spAI->OnRefresh(true);
				AI_GetControlFrame(Tmp);
				pCtrl = &Tmp;
			}
			else
#endif	
			{
				//Normal AI refresh and controlframe retrieval
				m_spAI->OnRefresh();
				pCtrl = m_spAI->GetControlFrame();
			};
		}
		else
		{
			//Refresh player AI (passive)
			m_spAI->OnRefresh(true);
		}
	}

	if (m_bResetControlState)
	{
		//Make sure control state is reset before processing the current control...FIX

		m_bResetControlState = false;
	};

	if (pCtrl)
	{
		if (pP && !pP->m_bCtrlValid)
		{
			if (pCD->m_PhysSelection!=NULL)
			{
				m_pWServer->Object_EnableLinkage(m_iObject);
				delete pCD->m_PhysSelection;
				pCD->m_PhysSelection=NULL;
			}
			return 1;
		}

//		int nUserVel = 1;

		CVec3Dfp32 Vel = 0;

		pCD->m_Control_Released &= (~pCD->m_Control_Press) | PLAYER_CONTROLBITS_NOAUTORELEASE;
//		pCD->ControlSnapshot();
		pCD->m_PredictFrameFrac = 0;

		// Send 1 input block to connected input entities for the entire frame, or 
		// send after each command...? (otherwise we might miss a button press..)

		int nMove = 0;
		int OldTime = 0;
		int Pos = pCtrl->GetCmdStartPos();
		while(Pos < pCtrl->m_MsgSize)
		{
//			int LastPress = pCD->m_Control_Press;
//			CVec3Dfp32 LastMove = pCD->m_Control_Move;

			CCmd Cmd;
			int Pos2 = Pos;
			pCtrl->GetCmd(Pos2, Cmd);

			if (CmdIDStart == 0x7fff) CmdIDStart = Cmd.m_ID;
			CmdIDEnd = Cmd.m_ID;
			int Time = Cmd.m_dTime;

//			if ((Cmd.m_Cmd == CONTROL_MOVE && pCD->m_Control_Move != LastMove && (Params.m_Flags & 1)) ||
//				(Cmd.m_Cmd == CONTROL_STATEBITS && pCD->m_Control_Press != LastPress))

			bool bControlCmd = Phys_ControlAffectsMovement(*pCtrl, Pos, this, pCD);
			if (bControlCmd)
			{
				if (Time - OldTime > 0)
				{
					fp32 dT = fp32(Time - OldTime) * (1.0f/ 128.0f);
	#ifdef LOG_AUTHORING
	ConOutL(CStrF("Authoring ID %d, dTime %d, Ctrl %d, Size %d, Move %s, Look %s, Vel %s", Cmd.m_ID, Cmd.m_dTime, Cmd.m_Cmd, Cmd.m_Size, (char*)pCD->m_Control_Move.GetString() , (char*)pCD->m_Control_Look.GetString(), (char*)Vel.GetString() ));
	#endif
					if (bMoveEnable) pCD->Phys_Move(*pCD->m_PhysSelection, dT, Vel, false);
					nMove++;
					OldTime = Time;
				}
	#ifdef LOG_AUTHORING
				else
	ConOutL(CStrF("Authoring ID %d, dTime %d, Ctrl %d, Size %d, Move %s, Look %s", Cmd.m_ID, Cmd.m_dTime, Cmd.m_Cmd, Cmd.m_Size, (char*)pCD->m_Control_Move.GetString() , (char*)pCD->m_Control_Look.GetString() ));
	#endif
			}

			pCD->m_PredictFrameFrac = fp32(Time) * (1.0f/ 128.0f);

			if (!Char_ProcessControl(*pCtrl, Pos))
			{
				if (pCD->m_PhysSelection!=NULL)
				{
					m_pWServer->Object_EnableLinkage(m_iObject);
					delete pCD->m_PhysSelection;
					pCD->m_PhysSelection=NULL;
				}
				return 0;
			}
			// For now send status message of input on after each control command
			if (bControlCmd && m_liInputEntities.Len())
			{
				CWObject_Message InputUpdateMsg(CWObject_InputEntity::OBJECT_INPUTENTITY_MESSAGE_UPDATEINPUT,pCD->m_Control_Press,pCD->m_Control_LastPress,-1,0,pCD->m_Control_Move, pCD->m_Control_Look_Wanted);
				for (int32 i = 0; i < m_liInputEntities.Len(); i++)
					m_pWServer->Message_SendToObject(InputUpdateMsg,m_liInputEntities[i]);
			}

			{
				OnPress();
				pCD->m_Control_Released &= (~pCD->m_Control_Press) | PLAYER_CONTROLBITS_NOAUTORELEASE;
			}
		}

//		if (pP) pP->m_ServerControlSerial = CmdIDEnd;

		int Time = 128;
		if (Time - OldTime > 0)
		{
			fp32 dT = fp32(Time - OldTime) * (1.0f/128.0f);
			if (bMoveEnable) pCD->Phys_Move(*pCD->m_PhysSelection, dT, Vel, false);
			nMove++;
//			OnPress(Params);
//			m_Released &= ~m_Press;
		}

		pCD->m_PredictFrameFrac = fp32(Time) * (1.0f/128.0f);
//		ConOut(CStrF("(CWObject_Character::OnPumpControl) nUserVel %d", nUserVel));
	}
	else
	{
		pCD->m_Control_Move = 0;
		pCD->m_Control_Press = 0;

		CVec3Dfp32 Vel = 0;

		// FIXME: MEMOPT
		// Expired
		//pCD->ControlSnapshot();

		if (bMoveEnable) pCD->Phys_Move(*pCD->m_PhysSelection, 1.0f, Vel, false);
	}

	if(!(m_Flags & PLAYER_FLAGS_NOAIMASSISTANCE) && 
		pCD->m_iPlayer != -1 && 
		!InNoClipMode(CWObject_Character::Char_GetPhysType(this)) &&
		pCD->m_Item0_Flags & RPG_ITEM_FLAGS_AIMING &&
		pCD->m_Control_Press & CONTROLBITS_AIMASSISTANCE
		&& pCD->m_AutoAimVal != 0.0f)
	{
		Aim_Refresh(this, m_pWServer);
	}
	else
		pCD->m_AimTarget = 0;

	pCD->Char_UpdateLook(1.0f);
	
	// Set orientation according to the look.
	if ((!(m_ClientFlags & PLAYER_CLIENTFLAGS_NOLOOK) || pCD->m_RelAnimPos.HasDirection())&& 
		!(pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOLOOK) && 
		!(Char_GetControlMode(this) == PLAYER_CONTROLMODE_LEDGE2) &&
		!(Char_GetControlMode(this) == PLAYER_CONTROLMODE_HANGRAIL) &&
		(OldLookWanted.DistanceSqr(pCD->m_Control_Look_Wanted) > Sqr(0.00001f)))
	{
		CMat4Dfp32 Mat;
		pCD->m_Control_Look_Wanted.CreateMatrixFromAngles(0, Mat);
		m_pWServer->Object_SetRotation(m_iObject, Mat);
	}

	if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_CREEPINGCAM)
	{
		CWObject_CoreData* pCreep = m_pWServer->Object_GetCD(pCD->m_iCreepingDark);
		CWObject_CreepingDark::CWObject_SetCreepOrientation(m_pWServer, pCreep, pCD->m_Creep_Control_Look_Wanted.m_Value);
		pCD->m_Creep_Control_Look_Wanted.MakeDirty();
	}

	{
//		CMat4Dfp32 Mat = GetPositionMatrix();
		//JK-NOTE: Don't do this big nasty! 3,3 does not exist in a 43 matrix!
//		Mat.k[3][3] += 16.0f;
//		m_pWServer->Debug_RenderMatrix(Mat, 3.0, true, 0xff007f00, 0xff000000, 0xff000000);
//		_pWPhysState->Debug_RenderMatrix(MatLook, 1.0, true, 0xff007f00, 0xff000000, 0xff000000);
	}
		

#ifdef LOG_AUTHORING
	ConOutL(CStrF("ServerMove CmdID %d->%d, Look %s, %s to %s", CmdIDStart, CmdIDEnd, (char*)pCD->m_Control_Look.GetString(), (char*)StartPos.GetString(), (char*)GetPosition().GetString()));
#endif

	if (pCD->m_PhysSelection!=NULL)
	{
/*		CBox3Dfp32 Bound = *GetAbsBoundBox();
		if (!Bound.IsCovering(SelectBound))
		{
			M_TRACEALWAYS("Character %d moved outside selection bound: %s is outside %s\n", m_iObject, Bound.GetString().Str(), SelectBound.GetString().Str() );
		}
*/
		m_pWServer->Object_EnableLinkage(m_iObject);
		delete pCD->m_PhysSelection;
		pCD->m_PhysSelection=NULL;
	}
	return 1;
}

// Returns true if char is in darkness
bool CWObject_Character::InDarkness()
{
	if (m_spAI)
	{	// We supply 0 to GetLightIntensity() as we DO want our own flashlight to be taken into account
		if (m_spAI->GetLightIntensity(0) > LIGHTMETER_SNEAK_THRESHOLD)
		{
			// We're in sneak approved darkness
			return(true);
		}
		else
		{
			return(false);
		}
	}
	else
	{	// Glow in the dark unimportant spear carrying extra :)
		return(false);
	}
}

bool CWObject_Character::HasHeart()
{
	return (m_DarknessFlags & DARKNESS_FLAGS_HEART) ? true : false;
}

void CWObject_Character::SetHeart(bool _bHasHeart)
{
	if (_bHasHeart)
		m_DarknessFlags |= DARKNESS_FLAGS_HEART;
	else
		m_DarknessFlags &= ~DARKNESS_FLAGS_HEART;
}

void CWObject_Character::OnRefreshSkeletonMats()
{
	if (!m_bHeadMatUpdated)
	{
		CXR_Skeleton* pSkel;
		CXR_SkeletonInstance* pSkelInstance;					
		CXR_AnimState AnimState;

		CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
		if(!pCD)
			return;

		if (CWObject_Character::Char_GetPhysType(this) == PLAYER_PHYS_NOCLIP)
			return;

		if (GetEvaluatedPhysSkeleton(pSkelInstance, pSkel, AnimState))
		{
			// Head
			if (pSkelInstance->m_nBoneTransform >= PLAYER_ROTTRACK_HEAD)
			{
				const CMat4Dfp32& Mat = pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_HEAD];
				CVec3Dfp32 tmp;
				tmp = pSkel->m_lNodes[PLAYER_ROTTRACK_HEAD].m_LocalCenter;

				const CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
				if (pCD->m_iPlayer != -1)
				{
					m_HeadMat = GetPositionMatrix();
					tmp +=  CVec3Dfp32(3.5f, -1.0f, 2.5f); // tweak offset for "camera" position
				}
				else
					m_HeadMat = Mat;

				if (!(Mat.k[0][0] > -1.1f && Mat.k[0][0] < 1.1f))
					tmp *= GetPositionMatrix(); // fallback for non-existing (broken) head-track
				else
					tmp *= Mat;

				m_HeadMat.GetRow(3) = tmp;
			}

			//Weapon
			CRPG_Object_Item* pEquippedItem = Char()->GetFinalSelectedItem(0);
			if (pEquippedItem && pEquippedItem->m_iItemType != RPG_ITEMTYPE_FIST)
			{
				CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(pCD->m_Item0_Model.m_iModel[0]);
				if (pModel)
				{
					CMat4Dfp32 Mat;
					if (pCD->m_Item0_Model.GetModel0_RenderMatrix(pSkelInstance, pSkel, pModel, pCD->m_Item0_Model.m_iAttachRotTrack, pCD->m_Item0_Model.m_iAttachPoint[0], Mat))
					{
						CXR_Skeleton* pWeaponSkel = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
						const CXR_SkeletonAttachPoint* pAttach = pWeaponSkel ? pWeaponSkel->GetAttachPoint(1) : NULL;
						if (pAttach)
							Mat.GetRow(3) = pAttach->m_LocalPos.GetRow(3) * Mat;
						m_WeaponMat = Mat;
					}
				}
			}
		}
		m_bHeadMatUpdated = true; 
	}
}

void CWObject_Character::UpdateDarknessLightMorph(CWO_Character_ClientData* _pCD)
{
	//if(_pCD->m_DarknessSelectionMode & (PLAYER_DARKNESSMODE_DARKNESSVISION | PLAYER_DARKNESSMODE_POWER_DEVOUR_DARKLING | PLAYER_DARKNESSMODE_POWER_DRAIN))
	if(_pCD->m_DarknessSelectionMode & (PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD))
    {
		TSelection<CSelection::LARGE_BUFFER> Selection;
		m_pWServer->Selection_AddClass(Selection, "Light");
		m_pWServer->Selection_AddClass(Selection, "Light2");
		
		const int16* piObjects;
		int32 nObjects = m_pWServer->Selection_Get(Selection, &piObjects);

		int16* piLights = m_lFlickerLights.GetBasePtr();
		const int& nLights = m_lFlickerLights.Len();
		
		CWO_Light_MessageData LightData;
		LightData.m_FlickerType = CLightIntens::ESine;//ERandPulse;

		TArray<int16> liNewFlicker;
		for(int i = 0; i < nObjects; i++)
		{
			CWObject* pObj = m_pWServer->Object_Get(piObjects[i]);
			
			// Check if light is out of range we skip it
			if((pObj->GetPosition() - GetPosition()).LengthSqr() > Sqr(512))
				continue;

			// If we found light we check it as so, and checks next light
			int iObject;
			for(iObject = 0; iObject < nLights; iObject++)
			{
				if(piObjects[i] == piLights[iObject])
				{
					liNewFlicker.Add(piObjects[i]);
					piLights[iObject] = -1;
					break;
				}
			}

			// Check if light passed our little validation
			if(iObject >= nLights)
			{
				CWObject_Light* pLight = safe_cast<CWObject_Light>(pObj);

				const uint8 Type = pLight->GetFlickerType(pObj);
				const uint8 Broken = pLight->IsBroken(pObj);
				
				// Make sure light isn't already flickering or broken
				if(!Type && !Broken)
				{
					liNewFlicker.Add(piObjects[i]);

					CWObject_Message Msg(OBJMSG_LIGHT_IMPULSE, CWObject_Light::MSG_FLICKER);
					Msg.m_pData = &LightData;

					if (_pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD)
					{
						LightData.m_FlickerFreq = fp32(2 + (MRTC_RAND() % 3));
						LightData.m_FlickerSpread = 40 + (MRTC_RAND() % 40);
					}
                    
					m_pWServer->Message_SendToObject(Msg, pObj->m_iObject);
				}
			}
		}

		LightData.m_FlickerType = CLightIntens::ENone;
		LightData.m_FlickerSpread = 0;
		LightData.m_FlickerFreq = 0;

		// Before copying new data to our previous list we restore the lights
		// That needs to be restored
		for(int i = 0; i < nLights; i++)
		{
			if (piLights[i] > 0)
			{
				CWObject_Message Msg(OBJMSG_LIGHT_IMPULSE, CWObject_Light::MSG_FLICKER);
				Msg.m_pData = &LightData;
				m_pWServer->Message_SendToObject(Msg, piLights[i]);
			}
		}

		// Copy new selection
		const int& nNewFlicker = liNewFlicker.Len();
		const int16* piNewFlicker = liNewFlicker.GetBasePtr();
		m_lFlickerLights.SetLen(nNewFlicker);
		piLights = m_lFlickerLights.GetBasePtr();

		memcpy(piLights, piNewFlicker, sizeof(int16)*nNewFlicker);

	}
	else
	{
		// When power is turned off we should remember to restore everything, (because I didn't at first try =)
		const int& nLights = m_lFlickerLights.Len();
		if(nLights > 0)
		{
			CWO_Light_MessageData LightData;
			LightData.m_FlickerType = CLightIntens::ENone;
			LightData.m_FlickerSpread = 0;
			LightData.m_FlickerFreq = 0.0f;

			CWObject_Message Msg(OBJMSG_LIGHT_IMPULSE, CWObject_Light::MSG_FLICKER);
			Msg.m_pData = &LightData;

			const int16* piLights = m_lFlickerLights.GetBasePtr();
			for(int i = 0; i < nLights; i++)
			{
				CWObject* pObj = m_pWServer->Object_Get(piLights[i]);
				m_pWServer->Message_SendToObject(Msg, pObj->m_iObject);
			}

			m_lFlickerLights.Clear();
		}
	}
}

void CWObject_Character::OnRefresh()
{
	MAUTOSTRIP(CWObject_Character_OnRefresh, MAUTOSTRIP_VOID);
	if(m_Flags & PLAYER_FLAGS_WAITSPAWN)
		return;

	MSCOPE(CWObject_Character::OnRefresh, CHARACTER);

	// assure that we got clientdata
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if(!pCD)
		return;

	CWO_Burnable& Burnable = pCD->m_Burnable.m_Value;
	if (Burnable.OnRefresh(this, m_pWServer))
		pCD->m_Burnable.MakeDirty();

	m_pWServer->Debug_RenderText(GetPosition() + CVec3Dfp32(0,0,56), GetName(), 0xff007f00, m_pWServer->GetGameTickTime(), true);

	// A bit of a hack, if characters are lockedtoparent they won't trigger triggers, so force set to same position
	if ((m_Flags & PLAYER_FLAGS_ATTACHFORCETRIGGERS) && (GetParent() != 0))
	{
		//m_pWServer->Object_SetPhysics_DoNotify(m_iObject,GetPhysState(),GetPositionMatrix());
		//m_pWServer->Phys_SetPosition(-1,m_iObject,GetPositionMatrix());
		//m_pWServer->Object_SetPosition(m_iObject,GetLocalPosition());
		m_pWServer->Object_MovePhysical(m_iObject);
		//m_pWServer->Phys_IntersectWorld(-1,GetPhysState(),GetPositionMatrix(),GetPositionMatrix(),m_iObject,NULL,m_IntersectNotifyFlags);
			 //(_pcs, pObj->m_PhysState, WOrigin, WDest, _iObj, _pCollisionInfo, NotifyFlags, iSel1, iSel2);
	}

	//--------------------------------------------------------------------------------
	// IK needs traces to be done here, instead of via OnAnimGetState due to 
	// MultiTreading problems with PhysStates intersections
	// is it a darkling?
	bool bIsDead = (Char_GetPhysType(this) == PLAYER_PHYS_DEAD);
	bool bIsMP = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), m_pWServer->Game_GetObjectIndex()) ? true : false;
	if (!bIsDead)
	{
		if(pCD->IsDarkling()) // is darkling
		{
			pCD->m_PostAnimSystem.GatherFeetCollisionData(CPostAnimSystem::FEET_TYPE_DARKLING_QUADRO, m_pWServer, 1.0f);
		}
		else
		{
			if((pCD->m_iPlayer != -1) && !(pCD->m_AnimGraph2.GetStateFlagsHiCombined() & AG2_STATEFLAGHI_IKSYSTEM_IGNOREHANDS)) // is player and "ignore" not set
			{
				if(pCD->m_PostAnimSystem.GetWeaponHoldType() == 0)
				{
					TSelection<CSelection::SMALL_BUFFER> Selection;
					pCD->m_PostAnimSystem.GatherWeaponWallCollisionData(-1, m_pWServer, 0, &Selection);
				}
				else
					pCD->m_PostAnimSystem.GatherWeaponWallCollisionData(-1, m_pWServer);

				pCD->m_PostAnimSystem.GatherAimCollisionData(m_pWServer, pCD->m_AimTarget, true);
			}

			fp32 TestScale = pCD->m_CharGlobalScale;
			if(pCD->m_iPlayer == -1)
			{
				if((pCD->m_GameTick - GetLastTickInClientPVS()) < 10)
					pCD->m_PostAnimSystem.SetFeetSetupStatus(false);

				pCD->m_PostAnimSystem.GatherFeetCollisionData(CPostAnimSystem::FEET_TYPE_HUMAN_BIPED, m_pWServer, pCD->m_CharGlobalScale);					
				pCD->m_PostAnimSystem.GatherAimCollisionData(m_pWServer, pCD->m_AimTarget, false);
			}				
		}
	}

	if (pCD->m_iPlayer != -1 && !bIsDead)
	{
		uint8 HitEffectFlags = pCD->m_HitEffect_Flags.m_Value;
		if (HitEffectFlags & PLAYER_HITEFFECT_FLAGS_DAMAGE)
		{
			if (pCD->m_HitEffect_Duration.m_Value <= m_pWServer->GetGameTick() && (pCD->m_Health >= 16 || bIsMP))
				pCD->m_HitEffect_Flags = HitEffectFlags & ~PLAYER_HITEFFECT_FLAGS_DAMAGE;
		}

		// Recive health after cooldown
		if (!bIsMP && pCD->m_Health < pCD->m_MaxHealth && (m_ShieldIgnoreDamageEndTick < pCD->m_GameTick) && (Char()->m_LastDamageTick + 5.0f * m_pWServer->GetGameTicksPerSecond() < pCD->m_GameTick))
		{
			Char()->ReceiveHealth(1);
			pCD->m_Health = Char()->Health();
		}
	}
	

#ifndef	M_RTM
	// UGLY HACK
	{
		int bRemoveCharachters = m_pWServer->Registry_GetServer()->GetValuei("KMA_REMOVECHARS", 0);

		// make sure that it's the local player
		if(bRemoveCharachters && pCD->m_iPlayer == 0)
		{
			// clear flag
			m_pWServer->Registry_GetServer()->SetValuei("KMA_REMOVECHARS", 0);

			TSelection<CSelection::LARGE_BUFFER> Selection;
			m_pWServer->Selection_AddOnFlagsSet(Selection, OBJECT_FLAGS_CHARACTER);

			const int16 *pObjects;
			int32 nObjects = m_pWServer->Selection_Get(Selection, &pObjects);

			int16 iClosest = -1;
			fp32 ClosestDistance = 100000;
			for(int32 i = 0; i < nObjects; i++)
			{
				if(pObjects[i] == m_iObject) // skip real player
					continue;

				CWObject *pObj = m_pWServer->Object_Get(pObjects[i]);
				if(!pObj)
					continue;

				fp32 Distance = (pObj->GetPosition()-GetPosition()).Length();
				if(Distance < ClosestDistance)
				{
					ClosestDistance = Distance;
					iClosest = pObjects[i];
				}
			}

			// destroy charachters
			for(int32 i = 0; i < nObjects; i++)
			{
				if(pObjects[i] == m_iObject) // skip real player
					continue;

				if(pObjects[i] == iClosest) // skip closest player
					continue;

				m_pWServer->Object_Destroy(pObjects[i]);
			}

		}
	}
#endif

	// Get wind forces for cloth objects
	/* - Disabled,
	{
		CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
		CXR_Skeleton* pSkel = (pModel) ? pModel->GetPhysSkeleton() : NULL;
		if(pModel && pSkel && pSkel->m_lCloth.Len() > 0)
		{
			CWObject_Wind::CreateSelection(m_pWServer, CVec3Dfp32::GetMatrixRow(GetPositionMatrix(), 3), 64.0f);
			pCD->m_WindForce = CWObject_Wind::GetForceSumAll(m_pWServer, CVec3Dfp32::GetMatrixRow(GetPositionMatrix(), 3));
			CWObject_Wind::DestroySelection();
		}
	}
	*/

	// Light morphing done due to darkness powers
	UpdateDarknessLightMorph(pCD);

	// Clear raised noise level and visibility every game-tick
	// m_RaisedNoiseLevel = 0;
	m_RaisedVisibility = 0;
	pCD->m_GameTime = PHYSSTATE_TICKS_TO_TIME(pCD->m_GameTick, m_pWServer);
	if(pCD->m_iPlayer == -1)
	{
		pCD->m_GameTick = m_pWServer->GetGameTick();
		Char_SetGameTickDiff(0);

		if(!m_spAI->OnActivationCheck())
		{
			if(!m_bFirst)
			{
				if (pCD->m_DialogueInstance.IsValid())
				{
					CWRes_Dialogue::CRefreshRes Res;
					OnRefresh_Dialogue(this, m_pWServer, 0, 0, &Res);
					EvalDialogueLink(Res);
				}
				// Might need to force refresh ag
				if (pCD->m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_FORCEREFRESH)
				{
					CWAG2I_Context AG2Context(this, m_pWServer, CMTime::CreateFromTicks(pCD->m_GameTick,m_pWServer->GetGameTickTime()), m_pWServer->GetGameTickTime());
					if (!m_DirtyMask && (pCD->m_AnimGraph2.GetAG2I()->Refresh(&AG2Context) & AG2I_TOKENREFRESHFLAGS_PERFORMEDACTION))
					{
						// Make this object dirty
						m_pWServer->Object_SetDirty(m_iObject,CWO_DIRTYMASK_GENERAL);
					}
				}

				m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
				return;
			}
		}
	}
	else
	{
		pCD->m_GameTick += 1;
		int Diff = pCD->m_GameTick - m_pWServer->GetGameTick();
//		if(Diff > 0 || Diff < -64)
		if(Diff > 10 || Diff < -4)
		{
			//ConOut("§cf80WARNING: Resetting Async player gametick to server gametick");
			pCD->m_GameTick = m_pWServer->GetGameTick();
		}
		Char_SetGameTickDiff(pCD->m_GameTick - m_pWServer->GetGameTick());

		// Check respawn
		CWObject_ActionCutsceneRiot::CheckRespawn(m_pWServer, m_iObject);
	}

	if ((pCD->m_DarknessSelectionMode || pCD->m_DarknessPowersAvailable) && ((pCD->m_iPlayer != -1) || m_bNonPlayerDarknessUser))
	{
		// Refresh darkness stuff
		int32 PrevDarkness = pCD->m_Darkness;
		int32 PrevHealth = pCD->m_Health;
		Char_RefreshDarkness();
		if ((PrevDarkness != pCD->m_Darkness) || (PrevHealth != pCD->m_Health))
			pCD->m_HealthHudStart = m_pWServer->GetGameTick();
	}

	// Fade in smoke when dead or stunned
	if (m_iDarkness_MouthSmoke)
	{
		CAI_Core* pAI = AI_GetAI();
		bool bFadeIn = (Char_GetPhysType(this) == PLAYER_PHYS_DEAD) | (pAI && !pAI->IsConscious());
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_EFFECTSYSTEM_SETFADE, (bFadeIn) ? 1 : 2), m_iDarkness_MouthSmoke);
	}

	// m_bFirst is a fix for the Vatar Activation Check Animation problem
	if(m_bFirst)
		m_bFirst = false;
	
	// Moved to fix dead AI bug
	pCD->m_bIsPredicting = false;
	pCD->m_bIsClientRefresh =  false;
	pCD->m_bIsServerRefresh = true;

	if ((pCD->m_iPlayer == -1) && (Char_GetPhysType(this) == PLAYER_PHYS_DEAD))
	{
		OnPumpControl();
		
		if ((pCD->m_DestroyTick > 0) && (pCD->m_DestroyTick <= pCD->m_GameTick))
			Destroy();

		m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;

		// AGMerge
		// FIXME: Temp fix. since client anim will ont refresh AGI otherwise.
		if (pCD != NULL)
		{
			Client_Anim(this, m_pWServer, pCD, 4);
			pCD->OnRefresh(this, m_pWServer, pCD->m_GameTime);
		}

		if(m_DropItemTick != 0)
		{
			if(m_DropItemTick <= pCD->m_GameTick)
			{
				Char_DeathDropItem();
				m_DropItemTick = 0;
			}
			// Collect information for the ragdoll on the drop item, so that it will have a nice speed when starting
			else
			{
				if (m_spRagdollDropItems[0])
				{
					CRPG_Object_Item *pItem = Char()->GetEquippedItem(0);
					if(pItem &&  !(pItem->m_Flags & RPG_ITEM_FLAGS_NOPICKUP))
					{
						CXR_AnimState Anim;
						CXR_Skeleton* pSkel;
						CXR_SkeletonInstance* pSkelInstance;					
						if(GetEvaluatedPhysSkeleton(pSkelInstance, pSkel, Anim))
						{
							CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);

							CMat4Dfp32 Mat;
							CXR_Model* pModel;
							CMTime GameTime = CMTime::CreateFromTicks(pCD->m_GameTick, m_pWServer->GetGameTickTime(), pCD->m_PredictFrameFrac);
							if(pCD->m_Item0_Model.GetModel0_RenderInfo(m_pWServer->GetMapData(), NULL, pSkelInstance, pSkel, GameTime, 
								Anim, Mat, pModel, m_pWServer))
								m_spRagdollDropItems[0]->Animate(m_pWServer->GetGameTick(), Mat);
						}
					}
				}
				if (m_spRagdollDropItems[1])
				{
					CRPG_Object_Item *pItem = Char()->GetEquippedItem(RPG_CHAR_INVENTORY_ARMOUR);
					if(pItem && !(pItem->m_Flags & RPG_ITEM_FLAGS_NOPICKUP))
					{
						CXR_AnimState Anim;
						CXR_Skeleton* pSkel;
						CXR_SkeletonInstance* pSkelInstance;					
						if(GetEvaluatedPhysSkeleton(pSkelInstance, pSkel, Anim))
						{
							CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);

							CMat4Dfp32 Mat;
							CXR_Model* pModel;
							CMTime GameTime = CMTime::CreateFromTicks(pCD->m_GameTick, m_pWServer->GetGameTickTime(), pCD->m_PredictFrameFrac);
							if(pCD->m_Item1_Model.GetModel0_RenderInfo(m_pWServer->GetMapData(), NULL, pSkelInstance, pSkel, GameTime, 
								Anim, Mat, pModel, m_pWServer))
								m_spRagdollDropItems[1]->Animate(m_pWServer->GetGameTick(), Mat);
						}
					}
				}
			}
		}
#ifdef INCLUDE_OLD_RAGDOLL
		if(m_spRagdoll)
		{
			CMat4Dfp32 Mat = GetPositionMatrix();
			m_spRagdoll->Animate(pCD->m_GameTick, Mat);
			if (m_spRagdoll->m_IdleTicks <= 3)
				pCD->m_RagdollClient.MakeDirty();

			// Remove animgraph information (should only be done once)
			if (m_spRagdoll->GetState() == CConstraintSystem::READY || 
				m_spRagdoll->GetState() == CConstraintSystem::STOPPED)
				pCD->m_AnimGraph2.GetAG2I()->DisableAll();
		}
#endif // INCLUDE_OLD_RAGDOLL
		if( m_pPhysCluster )
		{
			Char_RagdollToCD(pCD);
		}

		m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
		return;
	}

	if ((pCD->m_iPlayer != -1) && (Char_GetPhysType(this) == PLAYER_PHYS_DEAD) && m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), m_pWServer->Game_GetObjectIndex()))
	{
#ifdef INCLUDE_OLD_RAGDOLL
		if(m_spRagdoll)
		{
			CMat4Dfp32 Mat = GetPositionMatrix();
			m_spRagdoll->Animate(pCD->m_GameTick, Mat);

			if (m_spRagdoll->m_IdleTicks <= 3)
				pCD->m_RagdollClient.MakeDirty();

			// Remove animgraph information (should only be done once)
			if (m_spRagdoll->GetState() == CConstraintSystem::READY || 
				m_spRagdoll->GetState() == CConstraintSystem::STOPPED)
				pCD->m_AnimGraph2.GetAG2I()->DisableAll();
		}
#endif // INCLUDE_OLD_RAGDOLL
		if (m_pPhysCluster)
		{
			Char_RagdollToCD(pCD);
		}

		m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
	}

	//AR-NOTE: Quick hack to enable ragdoll on living characters
	if ((pCD->m_iPlayer == -1) && (pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_RAGDOLLACTIVE))
	{
#ifdef INCLUDE_OLD_RAGDOLL
		if( m_spRagdoll )
		{
			CMat4Dfp32 Mat = GetPositionMatrix();
			m_spRagdoll->Animate(pCD->m_GameTick, Mat);
			if (m_spRagdoll->m_IdleTicks <= 3)
				pCD->m_RagdollClient.MakeDirty();

			// Remove animgraph information (should only be done once)
			if ((m_spRagdoll->GetState() == CConstraintSystem::READY || 
				m_spRagdoll->GetState() == CConstraintSystem::STOPPED) && 
				(!(pCD->m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_BERAGDOLL)))
				pCD->m_AnimGraph2.GetAG2I()->DisableAll();
		}
#endif
		if (m_pPhysCluster)
		{
			Char_RagdollToCD(pCD);
		}
		m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
	}

#ifndef M_RTM
	int bRefreshEnable = m_pWServer->Registry_GetServer()->GetValuei("CHAR_REFRESH", -1);
#else
	int bRefreshEnable = -1;
#endif

	if (bRefreshEnable & 1)
		OnRefresh_ServerPredicted(pCD, this, m_pWServer);
	if (bRefreshEnable & 2)
		OnRefresh_ServerOnly(pCD);

	if(pCD->m_iPlayer != -1)
	{
		if(Char_IsPlayerView(this))
		{
			if((Char_GetPhysType(this) == PLAYER_PHYS_CROUCH) || Char_DarknessIsTintSound(pCD))
			{
				if(pCD->m_CrouchLevel.Set(255, pCD->m_GameTick, 0))
				{
//					ConExecute("sv_set (\"timescale\",\"0.75\")");
					pCD->m_CrouchLevel.MakeDirty();
//					Sound(m_pWServer->GetMapData()->GetResourceIndex_Sound("gui_sneak_on"));
					//m_pWServer->Sound_Global(m_pWServer->GetMapData()->GetResourceIndex_Sound("gui_sneak_on"));
				}
			}
			else
			{
				if(pCD->m_CrouchLevel.Set(0, pCD->m_GameTick, 0))
				{
//					ConExecute("sv_set (\"timescale\",\"1.0\")");
					pCD->m_CrouchLevel.MakeDirty();
//					Sound(m_pWServer->GetMapData()->GetResourceIndex_Sound("gui_sneak_off"));
					//m_pWServer->Sound_Global(m_pWServer->GetMapData()->GetResourceIndex_Sound("gui_sneak_off"));
				}
			}
		}
		else if(pCD->m_CrouchLevel.Set(0, pCD->m_GameTick, 0))
		{
//			ConExecute("sv_set (\"timescale\",\"1.0\")");
			pCD->m_CrouchLevel.MakeDirty();
		}

#ifdef INCLUDE_OLD_RAGDOLL

		if(m_GrabbedBody)
		{
			CWObject *pObj = m_pWServer->Object_Get(m_GrabbedBody);
			if(pObj)
			{
				CWObject_Character *pChar = safe_cast<CWObject_Character>(pObj);
				if(pChar->m_spRagdoll)
				{
					// Find attachpoint in left/right hand..
					/*CMat4Dfp32 Mat;
					{
						CXR_AnimState Anim;
						OnGetAnimState(this, m_pWServer, GetPositionMatrix(), 0.0f, Anim, NULL, 0, 8);
						CXR_SkeletonInstance* pSkelInst = Anim.m_pSkeletonInst;
						CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
						CXR_Skeleton* pSkel = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
						CVec3Dfp32 vHandle(0);
						const CXR_SkeletonAttachPoint* pGunHandle = (pSkel) ? pSkel->GetAttachPoint(0) : NULL;
						if(pGunHandle)
							vHandle = -pGunHandle->m_LocalPos;

						CMat43fp32 Handle;
						Handle.Unit();
						vHandle.SetMatrixRow(Handle, 3);

						CMat43fp32 Pos = pSkelInst->m_lBoneTransform[PLAYER_ROTTRACK_RHAND];

						CVec3Dfp32 v(0);
						if(pSkel)
						{
							const CXR_SkeletonAttachPoint* pHand = pSkel->GetAttachPoint(0);
							if (pHand) v = pHand->m_LocalPos;
						}
						v *= Pos;
						v.SetMatrixRow(Pos, 3);

						Handle.Multiply(Pos, Mat);
					}*/

					CVec3Dfp32 Pos = GetPosition();
					Pos.k[2] += 10;
					CVec3Dfp32 Forward = CVec3Dfp32::GetMatrixRow(GetPositionMatrix(),0);
					Forward.k[2] = 0.0f;
					Forward.Normalize();
					Pos += Forward * 10.0f;
					pChar->m_spRagdoll->SetPendingBonepos(PLAYER_ROTTRACK_RHAND, Pos);
				}
			}
		}
#endif // INCLUDE_OLD_RAGDOLL
		
		if ((m_ClientFlags & PLAYER_CLIENTFLAGS_CUTSCENE) &&
			m_PendingTimeLeapTick == pCD->m_GameTick)
		{
			Char_BeginTimeLeap();
		}

		bool bTrigger = false;
		if ((IsPlayer())&&!(Char_GetPhysType(this) == PLAYER_PHYS_NOCLIP)&& pCD->m_Cutscene_Camera == 0)
		{	// We supply 0 as flashlight owner as we DO want our own flash to be taken into account
			m_LightIntensity = m_spAI->GetLightIntensity(0);
		}

		if(!(Char_GetPhysType(this) == PLAYER_PHYS_NOCLIP) && Char_GetPhysType(this) == PLAYER_PHYS_CROUCH && pCD->m_Cutscene_Camera == 0)
		{
			bTrigger = m_LightIntensity < LIGHTMETER_SNEAK_THRESHOLD && m_LightIntensity >= 0.0f;
			if(GetMoveVelocity().LengthSqr() > Sqr(SNEAK_MAXMOVE_THRESHOLD)) // don't trigger sneak if we are moving too fast
				bTrigger = false;
		}

//		const fp32 SneakEnterDelay = 0.0f; // 0.0 = disabled
		if(bTrigger)
		{
//			if(m_LightSneak.m_EnterTime < m_pWServer->GetGameTime())
			{
				if(!m_Player.m_bLightSneak)
				{
					m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ENTERSNEAK), m_iObject);
					m_Player.m_bLightSneak = true;
				}
			}
		}
		else
		{
//			m_LightSneak.m_EnterTime = m_pWServer->GetGameTime()+SneakEnterDelay;

			if(m_Player.m_bLightSneak)
			{
				m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_LEAVESNEAK), m_iObject);
				m_Player.m_bLightSneak = false;
			}
		}

		// Refresh Fov morphing (if needed)
		if (pCD->m_BaseFov != pCD->m_TargetFov)
		{
			fp32 FovTime = pCD->m_FovTime;
			fp32 CurrTime = m_pWServer->GetGameTick();

			if (CurrTime > (FovTime + m_pWServer->GetGameTicksPerSecond() / pCD->m_FovTimeScale))
			{
				pCD->m_BaseFov = pCD->m_TargetFov;
			}
		}

		
		if(GetVelocity().m_Move.LengthSqr() > 5*5)
		{
			Char()->m_LastDamageTick = MaxMT(pCD->m_GameTick - Char()->m_HealthRegenerationDelay / 2, Char()->m_LastDamageTick);
			/*if((Char()->Health() & 15) != 0)
				pCD->m_HealthHudStart = pCD->m_GameTick;*/
			if(pCD->m_Zoom.m_Target != 0)
			{
				pCD->m_Zoom.Set(0, pCD->m_GameTick + 2, 0.25f);
				pCD->m_Zoom.MakeDirty();
			}
		}

		// Check health hud stuff
		{
//			uint32 HealthHudStart = pCD->m_HealthHudStart;
			uint32 ShouldBeOn = false;

			// Sort in order of importance
			if (Char_IsFighting(this))
				ShouldBeOn = true;

			// Tension higher than GET_TENSION_TENSION_HOSTILE (38 > 255 * 0.10f)
			if (pCD->m_Tension > 38)
			{
				ShouldBeOn = true;
				// Reset weapon unequip timer
				SetWeaponUnequipTimer();
			}

			// If you are dead the health meter should not be showing...
			if (Char_GetPhysType(this) == PLAYER_PHYS_DEAD)
				ShouldBeOn = false;

			if (ShouldBeOn)
				pCD->m_HealthHudStart = m_pWServer->GetGameTick();
		}
	}
	else
	{
		// Check if we're in water, if so kill ourselves
		const int MaxSamples = 2;
		int nMediums = 2;
		CXR_MediumDesc Mediums[MaxSamples];
		CVec3Dfp32 MediumV[MaxSamples];

		// if we're up the the head (approx) then drown
		MediumV[0] = GetPosition();
		MediumV[0].k[2] = GetAbsBoundBox()->m_Min[2]; //Min(fp32( + pCD->m_SwimHeight), GetAbsBoundBox()->m_Max[2]);
		MediumV[1] = GetPosition();
		MediumV[1].k[2] = GetAbsBoundBox()->m_Max[2]-15.0f;

		if (pCD->m_PhysSelection!=NULL)
			m_pWServer->Phys_GetMediums(*pCD->m_PhysSelection, MediumV, nMediums, Mediums);
		else
			m_pWServer->Phys_GetMediums(MediumV, nMediums, Mediums);

		if ((Mediums[0].m_MediumFlags & XW_MEDIUM_LIQUID) && (Mediums[1].m_MediumFlags & XW_MEDIUM_LIQUID))
		{
			// Kill youself
			Char_Die(0,0);
			//CWO_DamageMsg Msg(1000000,DAMAGETYPE_PHYSICS);
			//Physics_Damage(Msg,m_iObject);
		}
		// Spawn Behaviour
/*		if (m_SpawnDuration > 0)
		{
			m_SpawnDuration--;
			if (m_SpawnDuration == 0)
			{
				Char()->ApplyWait(0);
				ClientFlags() &= ~PLAYER_CLIENTFLAGS_NOMOVE;
				ClientFlags() &= ~PLAYER_CLIENTFLAGS_NOLOOK;
			}
		}*/
	}


	if (m_FallDmgTimeoutTicks) m_FallDmgTimeoutTicks--;

	pCD->m_bIsServerRefresh = false;
/*	if(pCD->m_iPlayer != -1)
	{
		if(pCD->m_iMountedObject != 0)
		{
			// Make sure mounted object is in sync with player
			CWObject *pObj = m_pWServer->Object_Get(pCD->m_iMountedObject);
			if(pObj)
			{
				pObj->OnRefresh();
				m_pWServer->Object_SetDirty(pObj->m_iObject, pObj->m_DirtyMask);
				
				if(pCD->m_iMountedObject != 0)
					// Still mounted? Make sure it won't run it's own OnRefresh
					pObj->SetNextRefresh(m_pWServer->GetGameTick() + 1000);
			}
		}
	}*/

	/*int MedType = pCD->m_FeetMediumType; 
	int PrevMedType = pCD->m_PreviousFeetMediumType;*/
	
	if(pCD->m_LiquidTickCountDown > 0)
		Char()->ApplyWait(1);
	if(CWObject_Character::Char_GetControlMode(this) == PLAYER_CONTROLMODE_LADDER)
		Char()->ApplyWait(1);


	/*if(MedType & XW_MEDIUM_LIQUID)
	{
		if(!(PrevMedType & XW_MEDIUM_LIQUID))
		{
			CMat4Dfp32 Pos = GetPositionMatrix();
			CVec3Dfp32 Feet = GetPosition();
			// Create the splash at the position of the feet and move it up with the velocity of the object. Thus moving it up one tick
			// so that the effect will be in the right place.
			Feet[2] = GetAbsBoundBox()->m_Min[2] - GetMoveVelocity()[2];
			
			Feet.SetMatrixRow(Pos,3);
			//m_pWServer->Object_Create("firebottlewall", Pos);
		}
	}*/

	// Update ledgeclimboncounter stuff
	if ((pCD->m_LedgeClimbOnCounter - 1) >= 0)
		pCD->m_LedgeClimbOnCounter--;

	// Should only do this when almost standing still...?
	// The player only, when looking at usable stuff, show focus frame
	if (Char_IsPlayerViewControlled(this) && (pCD->m_iPlayer != -1))
	{
		// Should health blob be shown?
		if ((pCD->m_Control_Press & CONTROLBITS_BUTTON0) && !(pCD->m_Control_Press_LastPressAGRefresh & CONTROLBITS_BUTTON0) && !(pCD->m_Disable & PLAYER_DISABLE_ACTIVATE) &&
			(pCD->m_FocusFrameType == SELECTION_NONE && m_Player.m_LastInfoscreenPress < int(m_pWServer->GetGameTick()) - 5))
			pCD->m_HealthHudStart = pCD->m_GameTick;
		// Ok, do a hard check (very direction dependant) looking for objects to use/pickup
		// maybe not ladders and stuff?
		// The best object found (if any), will be shown in the hint window
//		int32 iPrev = pCD->m_iFocusFrameObject;
		int32 iSel = -1;
		int8 SelType = 0;
		int32 iCloseSel = -1;
		bool bCurrentNoPrio = false;
		Char_FindStuff(m_pWServer,this,pCD,iSel,iCloseSel,SelType,bCurrentNoPrio,FINDSTUFF_SELECTIONMODE_FOCUSFRAME | FINDSTUFF_SELECTIONMODE_DEVOURING);
		//CFStr OldText = pCD->m_FocusFrameText;
		Char_ShowInFocusFrame(SelType, iSel);
	}

	// Check if a movetoken should be performed
	//pCD->m_RelAnimPos.CheckForMoveTokens(this, pCD,m_pWServer);
	pCD->m_RelAnimPos.OnRefresh();


	// Well then, should move back to original position
	int32 CameraMode = pCD->m_ActionCutSceneCameraMode;
	if (((pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_MASK) == THIRDPERSONINTERACTIVE_MODE_NONE) &&	// don't offset the camera in interactive mode
	    (((CameraMode & CActionCutsceneCamera::ACS_CAMERAMODE_ACTIVE) && 
	      (pCD->m_ExtraFlags & (PLAYER_EXTRAFLAGS_THIRDPERSON))) ||
	      (Char_GetControlMode(this) == PLAYER_CONTROLMODE_LEAN)) )
	{
		pCD->m_ActionCutSceneCameraOffsetX = pCD->m_ActionCutSceneCameraOffsetX - 
			pCD->m_ActionCutSceneCameraOffsetX * 0.05f;
		pCD->m_ActionCutSceneCameraOffsetY = pCD->m_ActionCutSceneCameraOffsetY - 
			pCD->m_ActionCutSceneCameraOffsetY * 0.05f;
	}
	else if (!(pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_TURRET))
	{
		if (pCD->m_ActionCutSceneCameraOffsetX != 0.0f)
			pCD->m_ActionCutSceneCameraOffsetX = 0.0f;
		if (pCD->m_ActionCutSceneCameraOffsetY != 0.0f)
			pCD->m_ActionCutSceneCameraOffsetY = 0.0f;
	}

	// Can we be affected by tranquillizer?
	if (m_Flags & PLAYER_FLAGS_STUNNABLE)
	{
		// First check if we are already sleeping. In that case we might want to wake up now
		if (m_TranquillizerTickCount > 0)
		{
			m_TranquillizerTickCount--;
			if (m_TranquillizerTickCount == 0)
			{
				// Wake up
				m_TranquillizerIncoming = 0;
				m_TranquillizerLevel = 0;
				CWObject_Message Msg(OBJMSG_SCRIPTIMPULSE, CAI_Core::IMPULSE_RELEASE);
				OnMessage(Msg);
			}
		}
		else
		{
			// Not asleep, that's nice. Check if we might need to go to sleep soon
			m_TranquillizerLevel += m_TranquillizerIncoming;
			if (m_TranquillizerLevel >= 100)
			{
				// Sleeping beauty
				m_TranquillizerTickCount = m_TranquillizerOutTime;

				CWObject_Message Msg(OBJMSG_SCRIPTIMPULSE, CAI_Core::IMPULSE_PAUSE);
				OnMessage(Msg);
			}
		}
	}
	
	CRPG_Object_Item* pEquippedItem = Char()->GetFinalSelectedItem(0);
	if (!pEquippedItem)
	{
		if (pCD->m_iPlayer != -1)
		{
			// Select melee if nothing else exists
			Char()->GetInventory(0)->SelectItem(0, Char(), true);
			pEquippedItem = Char()->GetFinalSelectedItem(0);
			int32 ItemClass = pCD->m_EquippedItemClass;
			if (pEquippedItem && (ItemClass != pEquippedItem->m_AnimProperty))
			{
				pCD->m_EquippedItemClass = (uint16)pEquippedItem->m_AnimProperty;
				pCD->m_EquippedItemType = (uint16)pEquippedItem->m_iItemType;
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_ITEMANIMTYPE,pEquippedItem->m_AnimType);
				CWAG2I_Context Context(this,m_pWServer,pCD->m_GameTime);
				pCD->m_AnimGraph2.UpdateImpulseState(&Context);
			}
		}
	}
	else
	{
		pCD->m_EquippedItemClass = (uint16)pEquippedItem->m_AnimProperty;
		pCD->m_EquippedItemType = (uint16)pEquippedItem->m_iItemType;
		pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_ITEMANIMTYPE,pEquippedItem->m_AnimType);
	}

	//Players update weapon selection stuff here
	/*if (pCD->m_iPlayer != -1)
	{
		M_ASSERT(Char() && Char()->GetInventory(AG_ITEMSLOT_WEAPONS), "No weapon inventory!");

		//Update currently equipped item if necessary
		CRPG_Object_Inventory * pWeaponInv = Char()->GetInventory(AG_ITEMSLOT_WEAPONS);
		int iCurEquipped = pWeaponInv->GetFinalSelectedItemIndex();
		if (pCD->m_iCurEquippedItem != iCurEquipped)
			pCD->m_iCurEquippedItem = iCurEquipped;

		//Are we currently selecting? (don't select weapons if we're in ancient weapon mode)
		if ((pCD->m_SelectItemTimeOutTick > pCD->m_GameTick) && !(pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS))
		{
			//Selecting. If currently selected item is same index as next valid then update next valid
			//This occurs if we pressed select button last tick or if we only have one weapon
			if (pCD->m_iCurSelectedItem == pCD->m_iNextValidSelectItem)
			{
				pCD->m_iNextValidSelectItem = pWeaponInv->GetNextValidSelectedItemIndex(pCD->m_iCurSelectedItem);

				//If we're not already displaying the proper icon, do so
				CRPG_Object_Item * pCurSelectedItem	= pWeaponInv->GetItemByIndex(pCD->m_iCurSelectedItem);			
				if (pCurSelectedItem && 
					((uint16)pCurSelectedItem->m_iIconSurface != pCD->m_Pickup_iIcon))
				{
					pCD->m_Pickup_iIcon = (uint16)pCurSelectedItem->m_iIconSurface;
					pCD->m_Pickup_StartTick = pCD->m_GameTick;
					pCD->m_Pickup_Magazine_Num = (int8)(Max(-1, Min(127, pCurSelectedItem->ShowNumMagazines())));
				}
				//Otherwise, fix icon display time if it's wrong (allow slight error)
				else if (pCurSelectedItem && pCD->m_SelectItemTimeOutTick - 5 > pCD->m_Pickup_StartTick + (PLAYER_WEAPONSELECT_DURATION * SERVER_TICKSPERSECOND))
				{
					pCD->m_Pickup_StartTick = Max(0, (int)(pCD->m_SelectItemTimeOutTick - (PLAYER_WEAPONSELECT_DURATION * SERVER_TICKSPERSECOND)));
					pCD->m_Pickup_Magazine_Num = pCurSelectedItem->ShowNumMagazines();
				}
			}
		}
		else
		{
			//Not selecting. Reset current and next select items (check first to avoid updating autovars needlessly)
			if (pCD->m_iCurSelectedItem != iCurEquipped)
				pCD->m_iCurSelectedItem = iCurEquipped;
			int iNextSelectable = pWeaponInv->GetNextValidSelectedItemIndex(iCurEquipped);
			if (pCD->m_iNextValidSelectItem != iNextSelectable)
				pCD->m_iNextValidSelectItem = iNextSelectable;
		}
	}*/

	bool bSleeping = pCD->m_AnimGraph2.GetPropertyBool(PROPERTY_BOOL_ISSLEEPING);
	bool bValidDialogue = pCD->m_DialogueInstance.IsValid();
	if (bSleeping && bValidDialogue && (pCD->m_DialogueInstance.m_Priority != 255))
	{ 
		//ConOutL("KILLING VOICE OUTSIDE");
		Char_DestroyDialogue();
	}

	// Update Third Person Interactive mode
	if (pCD->m_iPlayer != -1)
		Char_UpdateThirdPersonInteractive(*pCD);

	// Check if were´re in third person view, if so, set visibility flag
	/*uint8 Mode = (pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_MASK);
	bool b3PI = (Mode != THIRDPERSONINTERACTIVE_MODE_NONE);
	if(b3PI)
	{
		//ClientFlags() |= CWO_CLIENTFLAGS_VISIBILITY;
		M_TRACE("*** In b3PI mode: ***, \n");
	}*/

	pCD->OnRefresh(this, m_pWServer, pCD->m_GameTime);

	if(pCD->m_DestroyTick > 0 && pCD->m_GameTick > pCD->m_DestroyTick)
		Destroy();

	m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;

	if (Char_UpdatePannlampa(m_pWServer, this))
		ClientFlags() |= CWO_CLIENTFLAGS_VISIBILITY;

	if (!pCD->IsDarkling())
	{
		//
		// Get a body transform that doesn't include look-angles.
		// NOTE: This is a big hack that only works for "normal" humanoids
		// However, I haven't found any engine support for fetching this matrix correctly, so..
		//
		const CMat4Dfp32& PosMatrix=GetPositionMatrix();
		m_BaseMat.r[0]=PosMatrix.r[0];
		m_BaseMat.r[2]=M_VConst(0,0,1.0f,0);
		m_BaseMat.RecreateMatrix<2, 0>();
		m_BaseMat.r[3]=PosMatrix.r[3];
		//	m_pWServer->Debug_RenderMatrix(PosMatrix, 0.0f, false);
		//	m_pWServer->Debug_RenderMatrix(m_BaseTransform, 0.0f, false);
	}


	// Store weapon and head matrices
	m_WeaponMat = GetPositionMatrix();		// dummy default
	m_WeaponMat.GetRow(3).k[2] += 48.0f;	// --
	m_HeadMat = GetPositionMatrix();		// dummy default
	m_HeadMat.GetRow(3).k[2] += 48.0f;		// --
	m_bHeadMatUpdated = false;

	/*
	CXR_Skeleton* pSkel;
	CXR_SkeletonInstance* pSkelInstance;					
	CXR_AnimState AnimState;
	if (GetEvaluatedPhysSkeleton(pSkelInstance, pSkel, AnimState))
	{
		// Head
		if (pSkelInstance->m_nBoneTransform >= PLAYER_ROTTRACK_HEAD)
		{
			const CMat43fp32& Mat = pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_HEAD];
			CVec3Dfp32 tmp;
			tmp = pSkel->m_lNodes[PLAYER_ROTTRACK_HEAD].m_LocalCenter;

			const CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if (pCD->m_iPlayer != -1)
				tmp +=  CVec3Dfp32(3.5f, -1.0f, 2.5f); // tweak offset for "camera" position

			if (!(Mat.k[0][0] > -1.1f && Mat.k[0][0] < 1.1f))
				tmp *= GetPositionMatrix(); // fallback for non-existing (broken) head-track
			else
				tmp *= Mat;
			m_HeadMat = GetPositionMatrix();
			m_HeadMat.GetRow(3) = tmp;
		}

		//Weapon
		CRPG_Object_Item* pEquippedItem = Char()->GetFinalSelectedItem(0);
		if (pEquippedItem && pEquippedItem->m_iItemType != RPG_ITEMTYPE_FIST)
		{
			CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(pCD->m_Item0_Model.m_iModel[0]);
			if (pModel)
			{
				CMat4Dfp32 Mat;
				if (pCD->m_Item0_Model.GetModel0_RenderMatrix(pSkelInstance, pSkel, pModel, pCD->m_Item0_Model.m_iAttachRotTrack, pCD->m_Item0_Model.m_iAttachPoint[0], Mat))
				{
					CXR_Skeleton* pWeaponSkel = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
					const CXR_SkeletonAttachPoint* pAttach = pWeaponSkel ? pWeaponSkel->GetAttachPoint(1) : NULL;
					if (pAttach)
						Mat.GetRow(3) = pAttach->m_LocalPos.GetRow(3) * Mat;
					m_WeaponMat = Mat;
				}
			}
		}
	}
	*/

	/*
	if (pEquippedItem && pEquippedItem->m_iItemType != RPG_ITEMTYPE_FIST)
	{
		//CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
		CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(pCD->m_Item0_Model.m_iModel[0]);
		if (pModel)
		{
			CXR_Skeleton* pSkel;
			CXR_SkeletonInstance* pSkelInstance;					
			CXR_AnimState AnimState;
			if (GetEvaluatedPhysSkeleton(pSkelInstance, pSkel, AnimState))
			{
				CMat4Dfp32 Mat;
				if (pCD->m_Item0_Model.GetModel0_RenderMatrix(pSkelInstance, pSkel, pModel, pCD->m_Item0_Model.m_iAttachRotTrack, pCD->m_Item0_Model.m_iAttachPoint[0], Mat))
				{
					CXR_Skeleton* pWeaponSkel = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
					const CXR_SkeletonAttachPoint* pAttach = pWeaponSkel ? pWeaponSkel->GetAttachPoint(1) : NULL;
					if (pAttach)
						Mat.GetRow(3) = pAttach->m_LocalPos.GetRow(3) * Mat;

					m_WeaponMat = Mat;
				}
			}
		}
	}
	*/

	{ // Check if controler is active or idle and update timestap accordingly
		const bool bSticks = (pCD->m_Control_Move.LengthSqr() > 0.0005f); // | (pCD->m_Control_DeltaLook.Length() > 0.001f);//(pCD->m_Control_DeltaLook.LengthSqr() > 0.000005f);				
		if(bSticks || (pCD->m_Control_Press & ~CONTROLBITS_AIMASSISTANCE) != 0)
			pCD->m_Control_IdleTicks = m_pWServer->GetGameTick();
	}

	// Update impulsestate for animgraph
	CWAG2I_Context AG2Context(this, m_pWServer, CMTime::CreateFromTicks(pCD->m_GameTick,m_pWServer->GetGameTickTime()),m_pWServer->GetGameTickTime());
	pCD->m_AnimGraph2.GetAG2I()->UpdateImpulseState(&AG2Context);
	// Check for gameplay anim events
	pCD->m_AnimGraph2.GetAG2I()->CheckAnimEvents(&AG2Context, OBJMSG_CHAR_ONANIMEVENT, ANIM_EVENT_MASK_GAMEPLAY|ANIM_EVENT_MASK_AISCRIPT|ANIM_EVENT_MASK_EFFECT|ANIM_EVENT_MASK_DIALOGUEIMPULSE|ANIM_EVENT_MASK_ANIMIMPULSES);
	/*if (pCD->m_iPlayer != -1)
		ConOutL(CStrF("Checking Events: %f Tick: %d",AG2Context.m_GameTime.GetTime(),pCD->m_GameTick));*/
	pCD->m_WeaponAG2.GetAG2I()->CheckAnimEvents(&AG2Context, OBJMSG_CHAR_ONANIMEVENT, ANIM_EVENT_MASK_EFFECT,false);

	// Gunplay test
	if (pCD->m_iPlayer != -1 && !pCD->m_AnimGraph2.GetPropertyBool(PROPERTY_BOOL_GUNPLAYDISABLED) && 
		pCD->m_AnimGraph2.GetPropertyFloat(PROPERTY_FLOAT_MOVEVELOCITY) < 0.8f)
	{
		//....
		int32 Len = m_lGunPlayTypes.Len();
		int32 Diff = pCD->m_GameTick - m_GunplayUnsafeTick - m_GunplaySafeDelay;
		if (Len > 0 && Diff > 0)
		{
			if ((fp32)Diff * Random > (fp32)m_GunplaySpread * Random)
			{
				m_GunplayUnsafeTick = pCD->m_GameTick + m_GunplayReactivateDelay;
				int32 GunplayType = m_lGunPlayTypes[((int)(((fp32)Len) * Random)) % Len];
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_GUNPLAY,GunplayType);
				pCD->m_AnimGraph2.SendAttackImpulse(&AG2Context,CXRAG2_Impulse(AG2_IMPULSETYPE_GUNPLAY,Random > 0.5f ? AG2_IMPULSEVALUE_GUNPLAY_RIGHT : AG2_IMPULSEVALUE_GUNPLAY_LEFT),0);
			}
		}
	}

	pCD->m_Control_Press_Intermediate = 0;

#ifdef WDEBUGCONTROL_ENABLED
	// CFStr TargetObj = m_pWServer->Registry_GetServer()->GetValue("dbgctrlobj");
	// int iDebugControlObject = m_pWServer->Selection_GetSingleTarget(TargetObj);

	CFStr TargetObj = m_pWServer->Registry_GetServer()->GetValue("agdbgobj");
	int iDebugControlObject = TargetObj.Val_int();
	if (iDebugControlObject <= 0)
		iDebugControlObject = TargetObj.Val_int();
	pCD->m_iDebugControlObject = iDebugControlObject;
	if (pCD->m_iPlayer != -1 && iDebugControlObject)
	{
		// If it's not a character, don't debug it
		CWObject_CoreData* pObj = m_pWServer->Object_GetCD(pCD->m_iDebugControlObject);
		CWObject_Character* pChar = TDynamicCast<CWObject_Character>(pObj);
		CWO_Character_ClientData* pCDDebugControl = pChar ? GetClientData(pChar) : NULL;
		if (pCDDebugControl)
		{
			pCD->m_DebugControlMoveX = pCDDebugControl->m_Control_Move[0];
			pCD->m_DebugControlMoveY = pCDDebugControl->m_Control_Move[1];
			pCD->m_DebugControlLookH = pCDDebugControl->m_Control_Look_Wanted[2];
			pCD->m_DebugControlLookV = pCDDebugControl->m_Control_Look_Wanted[1];
			pCD->m_DebugControlDLookH = pCDDebugControl->m_Control_DeltaLook[2];
			pCD->m_DebugControlDLookV = pCDDebugControl->m_Control_DeltaLook[1];
			pCD->m_DebugControlPress = pCDDebugControl->m_Control_Press;
		}
		else
		{
			// Not a character, remove from registry
			m_pWServer->Registry_GetServer()->DeleteKey("agdbgobj");
		}
	}
#endif

	// Update auto-use object
	if (m_iLastAutoUseObject)
	{
		CWObject* pObj = m_pWServer->Object_Get(m_iLastAutoUseObject);
		fp32 DSqr = pObj ? pObj->GetPosition().DistanceSqr(GetPosition()) : 100000.0f;
		if (DSqr > Sqr(64.0f))
			m_iLastAutoUseObject = 0;
	}

	// Update position of GrabObject-constraint
	if (m_GrabbedObject.IsActive())
	{
		const CMat4Dfp32& PosMat = GetPositionMatrix();
		CVec3Dfp32 Movement =  m_BaseMat.GetRow(0) * (pCD->m_Control_Move.k[0] * (fp32)pCD->m_Speed_Forward);
		          Movement += m_BaseMat.GetRow(1) * (pCD->m_Control_Move.k[1] * (fp32)pCD->m_Speed_SideStep);

		CVec3Dfp32 NewWorldPos = PosMat.GetRow(3);
		NewWorldPos.k[2] += 50.0f;
		NewWorldPos += PosMat.GetRow(0) * 32.0f;
		NewWorldPos += Movement * 5.0f;
		if (!m_GrabbedObject.Update(*m_pWServer, NewWorldPos, PosMat.GetRow(0)))
			m_GrabbedObject.Release();
	}
}


bool CWObject_Character::IsControllerIdle(const int _nTicks)
{
	CWO_Character_ClientData* pCD = GetClientData(this);
	if(m_pWServer->GetGameTick() - (pCD->m_Control_IdleTicks+1) > 0)
	{
		// If less than zero, we just says it's true
		if(_nTicks < 0)
			return true;

		// Check if a certain amount of time has passed since controler was released. In ticks!
		if(pCD->m_Control_IdleTicks - m_pWServer->GetGameTick() >= _nTicks)
			return true;
	}

	return false;
}

bool CWObject_Character::Char_UpdatePannlampa(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj)
{

	CWO_Character_ClientData *pCD = GetClientData(_pObj);
	CXR_SceneGraphInstance* pSGI = _pWPhys->World_GetSceneGraphInstance();
	if (!pSGI || !pCD || (pCD->m_iPlayer == -1))
		return false;

	if (pCD->m_PannLampa)
	{
		CXR_AnimState Anim;
		CXR_Skeleton* pSkel;
		CXR_SkeletonInstance* pSkelInst;
		const CMat4Dfp32& CharPos = _pObj->GetPositionMatrix();
		if(!GetEvaluatedPhysSkeleton(_pObj, _pWPhys, pSkelInst, pSkel, Anim, pCD->m_PredictFrameFrac, &CharPos))
			return false;

		// Get pos of a head bone..
		CMat4Dfp32 Trans;
		bool bOk = false;
		if(PLAYER_ROTTRACK_HEAD< pSkelInst->m_nBoneTransform)
		{
			Trans = pSkelInst->GetBoneTransform(PLAYER_ROTTRACK_HEAD);
			bOk = true;
		}

		// Add a pannlampa
		CMat4Dfp32 LightPos = _pObj->GetPositionMatrix();
		if (bOk)
		{
			CVec3Dfp32 Point(0,0,60);
			Point = (Point * Trans) + CVec3Dfp32::GetMatrixRow(LightPos,0) * 5;
			Point.SetRow(LightPos,3);
		}
		else
		{
			CVec3Dfp32::GetRow(LightPos,3).k[2] += 60.0f;
		}

		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");

		CRegistry* pOptionsRegistry = pSys ? pSys->GetOptions() : NULL;
		fp32 Range = (pOptionsRegistry ? pOptionsRegistry->GetValuef("PANNLAMPARANGE",1024.0f) : 1024.0f);

		CXR_Light Light(LightPos, CVec3Dfp32(((pCD->m_FlashLightColor.m_Value >> 16) & 0xff) / 255.0f,
			((pCD->m_FlashLightColor.m_Value >> 8) & 0xff) / 255.0f,
			((pCD->m_FlashLightColor.m_Value >> 0) & 0xff) / 255.0f) * 2,
			Range, 0, CXR_LIGHTTYPE_POINT);
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
		Light.m_LightGUID = _pObj->m_iObject * 5 + 0x4004;
		Light.m_iLight = 0;
		if (pCD->m_iPlayer == -1)
			Light.m_Flags |= CXR_LIGHT_NOSPECULAR;

		pSGI->SceneGraph_Light_LinkDynamic(Light);
		return true;
	}
	return false;
}
// This is mainly intented for summoned companions.
/*void CWObject_Character::OnTimeout()
{
	MAUTOSTRIP(CWObject_Character_OnTimeout, MAUTOSTRIP_VOID);
	ConOut(CStrF("Character %s(%d) timed out!", GetName(), m_iObject));

	// Stop character from moving, so that effects don't look miss placed.
	ClientFlags() |= PLAYER_CLIENTFLAGS_NOMOVE;

	// 
//	ClientFlags() |= PLAYER_CLIENTFLAGS_DEAD;
	Char_Die(0, 0, 0);

	// Create center timeout effect.
	if (m_TimeoutEffectCenter != "")
	{		
//		CMat4Dfp32 EffectMatrixCentered = GetPositionMatrix();
		CMat4Dfp32 EffectMatrixCentered;
		EffectMatrixCentered.Unit3x3();
		CVec3Dfp32 Center;
		GetAbsBoundBox()->GetCenter(Center);
		Center.SetMatrixRow(EffectMatrixCentered, 3);
		int iObj = m_pWServer->Object_Create(m_TimeoutEffectCenter, EffectMatrixCentered, m_iObject);
	}

	// Create floor timeout effect.
	if (m_TimeoutEffectGround != "")
	{
//		CMat4Dfp32 EffectMatrixGround = GetPositionMatrix();
		CMat4Dfp32 EffectMatrixGround;
		EffectMatrixGround.Unit3x3();
		GetPosition().SetMatrixRow(EffectMatrixGround, 3);
		int iObj = m_pWServer->Object_Create(m_TimeoutEffectGround, EffectMatrixGround, m_iObject);
	}

	// Play Timeout Sound
	Sound(m_iTimeoutSound);

	// Fade character
	//m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETFADE, 0), m_iObject);
	
	// Destroy character after 1 second.
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if (pCD != NULL)
		pCD->m_DestroyTick = pCD->m_GameTick + int(1.0f * SERVER_TICKSPERSECOND);
}*/

void CWObject_Character::OnRefresh_ServerOnly(CWO_Character_ClientData*_pCD)
{
	MAUTOSTRIP(CWObject_Character_OnRefresh_ServerOnly, MAUTOSTRIP_VOID);
	MSCOPE(CWObject_Character::OnRefresh_ServerOnly, CHARACTER);

	// Not dead?
	if (!(Char_GetPhysType(this) == PLAYER_PHYS_DEAD))
	{
		/*if ((m_TimeoutTick > 0) && (_pCD->m_GameTick >= m_TimeoutTick))
		{
			m_TimeoutTick = 0;
			OnTimeout();
		}*/

		if (!(Char_GetPhysType(this) == PLAYER_PHYS_NOCLIP))
		{
			const CWO_PhysicsState& Phys = GetPhysState();
			bool bForce = !(!(_pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOCHARACTERCOLL) ? 
				(m_ClientFlags & PLAYER_CLIENTFLAGS_GHOST || _pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOWORLDCOLL) ||
				(Phys.m_ObjectFlags & OBJECT_FLAGS_PHYSOBJECT) : true);
			/*if ((!(_pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOPHYSFLAG) && !(Phys.m_ObjectIntersectFlags & OBJECT_FLAGS_ANIMPHYS)) ||
				((_pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOPHYSFLAG) && (Phys.m_ObjectIntersectFlags & OBJECT_FLAGS_ANIMPHYS)))
				bForce = true;*/

			if ((_pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOPHYSFLAG) || (_pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOANIMPHYS))
			{
				if (Phys.m_ObjectIntersectFlags & OBJECT_FLAGS_ANIMPHYS)
					bForce = true;
			}
			else
			{
				if (!(Phys.m_ObjectIntersectFlags & OBJECT_FLAGS_ANIMPHYS))
					bForce = true;
			}

			if ((_pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOCHARCOLL) || (_pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOCHARACTERCOLL))
			{
				/*if (!(_pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOCHARACTERCOLL))
					_pCD->m_Phys_Flags = _pCD->m_Phys_Flags | PLAYER_PHYSFLAGS_NOCHARACTERCOLL;*/
				if (Phys.m_ObjectFlags & OBJECT_FLAGS_PHYSOBJECT)
					bForce = true;
			}
			else
			{
				/*if (_pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOCHARACTERCOLL)
					_pCD->m_Phys_Flags = _pCD->m_Phys_Flags & ~PLAYER_PHYSFLAGS_NOCHARACTERCOLL;*/
				if (!(Phys.m_ObjectFlags & OBJECT_FLAGS_PHYSOBJECT) && !(_pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOCHARACTERCOLL))
					bForce = true;
			}

			if ((_pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_NOCOLLISIONSUPPORTED) && Char_GetPhysType(this) != PLAYER_PHYS_DEAD)
			{
				if ((_pCD->m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_NOCOLLISION) ||
					(_pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_INVISIBLE))
				{
					// Set
					bForce = bForce || (Phys.m_ObjectIntersectFlags & OBJECT_FLAGS_PHYSOBJECT) != 0;
				}
				else
				{
					bForce = bForce || !(Phys.m_ObjectIntersectFlags & OBJECT_FLAGS_PHYSOBJECT);
				}
			}

			if (IsBot())
			{
				//Maintain crouch until AI tells us to stand
				Char_SetPhysics(this, m_pWServer, m_pWServer, (Char_GetPhysType(this) == PLAYER_PHYS_CROUCH) ? PLAYER_PHYS_CROUCH : PLAYER_PHYS_STAND,false,bForce);
			}
			else
			{
				//Players revert to stand unless crouch control bit is pressed
				// Force stand physics if in a heavyguard
				int32 PhysType;
				{
					// Try with crouched if we need to force set physics...
					if (_pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_FORCESETPHYS)
						PhysType = PLAYER_PHYS_CROUCH;
					else
						PhysType = (_pCD->m_Control_Press & CONTROLBITS_CROUCH) ? PLAYER_PHYS_CROUCH : PLAYER_PHYS_STAND;
				}
				
				Char_SetPhysics(this, m_pWServer, m_pWServer, PhysType,false,bForce);
			}

			// Expand vis boundbox
			if (_pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_EXPANDVISBOX)
			{
				// Expand bbox
				if (!(m_Flags & PLAYER_FLAGS_VISBOXEXPANDED))
				{
					CBox3Dfp32 CurBox;
					GetVisBoundBox(CurBox);
					fp32 Width = M_Fabs(_pCD->m_AnimGraph2.GetOffsetY());
					CBox3Dfp32 NewBox;
					NewBox.m_Min = CVec3Dfp32(-Width,-Width,0.0f);
					NewBox.m_Max = CVec3Dfp32(Width,Width,0.0f);
					NewBox.Expand(CurBox);
					m_Flags |= PLAYER_FLAGS_VISBOXEXPANDED;
					if(CurBox.m_Max != NewBox.m_Max || CurBox.m_Min != NewBox.m_Min)
						m_pWServer->Object_SetVisBox(m_iObject, NewBox.m_Min, NewBox.m_Max);
				}
			}
			else if (m_Flags & PLAYER_FLAGS_VISBOXEXPANDED)
			{
				// Reset to original bbox
				UpdateVisibility();
			}
		}

		OnPress();
	}
	// Not used anymore
	/*else
	{
		if (m_RespawnTimeoutTicks)
			m_RespawnTimeoutTicks--;
		else
		{
			if (_pCD->m_Control_Press & CONTROLBITS_PRIMARY)
			{
				m_pWServer->Message_SendToTarget(CWObject_Message(OBJMSG_GAME_RESPAWN, _pCD->m_iPlayer), (char*)WSERVER_GAMEOBJNAME);
				return;
			}
		}
	}*/
	
	// update weaponzoomstates
	if(_pCD)
	{
		bool bOKToZoom = true;
		uint8 Mode = (_pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_MASK);
		if((Mode & THIRDPERSONINTERACTIVE_MODE_ACS) || (Mode & THIRDPERSONINTERACTIVE_MODE_DIALOGUE))
			bOKToZoom = false;
		else if(!_pCD->m_PostAnimSystem.GetOKToWeaponZoom())
			bOKToZoom = false;
		else if(Char_GetControlMode(this) == PLAYER_CONTROLMODE_ANIMATION)
			bOKToZoom = false;
		else
		{
			CWObject* pTentacleObj = m_pWServer->Object_Get(_pCD->m_iDarkness_Tentacles);
			CWObject_TentacleSystem* pTentacleSystem = pTentacleObj ? safe_cast<CWObject_TentacleSystem>(pTentacleObj) : NULL;
			if(pTentacleSystem && (pTentacleSystem->IsDevouring() || pTentacleSystem->IsCreepingDark()))
				bOKToZoom = false;
		}

		if(!bOKToZoom)
		{
			if(_pCD->m_WeaponZoomState != WEAPON_ZOOMSTATE_OFF && _pCD->m_WeaponZoomState != WEAPON_ZOOMSTATE_OUT)
				ToggleWeaponZoom();
		}
		
		if(_pCD->m_WeaponZoomState != WEAPON_ZOOMSTATE_OFF && _pCD->m_WeaponZoomState != WEAPON_ZOOMSTATE_ON)
		{
			fp32 ZoomVal = ((fp32)(m_pWServer->GetGameTick() - _pCD->m_WeaponZoomStartTick) / (m_pWServer->GetGameTicksPerSecond() * 0.1f));
			if(ZoomVal > 1.0f)
			{
				switch(_pCD->m_WeaponZoomState )
				{
				case WEAPON_ZOOMSTATE_IN:
					_pCD->m_WeaponZoomState = WEAPON_ZOOMSTATE_ON;
					break;
				case WEAPON_ZOOMSTATE_OUT:
					_pCD->m_WeaponZoomState = WEAPON_ZOOMSTATE_OFF;
					break;
				default:
					break;
				}
			}
		}
	}

	{
		Client_Anim(this, m_pWServer, _pCD, 4);
		m_iAnim1 = (int16)(_pCD->m_Anim_BodyAngleZ*65536.0f);

/*ConOutL(CStrF("SERVER (Tick %.4d) %.2f, %.2f, %s", 
_pCD->m_GameTick, _pCD->m_Anim_LastBodyAngleZ, _pCD->m_Anim_BodyAngleZ,
GetLook(GetPositionMatrix()).GetString().Str()
));*/
	}

	if (!IsBot())
		OnRefresh_Predicted_Target(this, m_pWServer);


	if (!(Char_GetPhysType(this) == PLAYER_PHYS_DEAD))
	{
		// Alive
		m_DirtyMask |= CWO_DIRTYMASK_GENERAL;
	}
	else
	{
		// Dead
		CAxisRotfp32 Rot = GetVelocity().m_Rot;
		Rot.m_Angle *= 0.90f;
		if (Rot.m_Angle > 0.01f) 
			Rot.m_Angle = 0.01f;
		else if (Rot.m_Angle < -0.01f) 
			Rot.m_Angle = -0.01f;
		m_pWServer->Object_SetRotVelocity(m_iObject, Rot);
	}

	{

		// Debug-render skeleton
	/*	{
			CXR_AnimState Anim;
			CXR_Skeleton* pSkel;
			CXR_SkeletonInstance* pSkelInstance;					
			if(GetEvaluatedSkeleton(pSkelInstance, pSkel, Anim))
			{
				CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);

				m_pWServer->Debug_RenderSkeleton(pSkel, Anim.m_pSkeletonInst, 0xff20407f, 10.0f, true);

			}
		}*/
	}

	{
		TProfileDef(TPump); 
		{
			TMeasureProfile(TPump);
			if (!OnPumpControl())
			{
				m_DirtyMask |= _pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
				return;
			}
		}
		_pCD->m_PendingControlFrame.m_MsgSize = 0;
	}

#ifndef M_RTM
	int bMechanics = m_pWServer->Registry_GetServer()->GetValuei("CHAR_MECHANICS", 1);
	if (bMechanics)
#endif
		OnRefresh_Mechanics();

//	OnShutOffAllLights();
	OnRefresh_WeaponLights(this, m_pWServer, GetPositionMatrix(), 0, NULL);
	UpdateVisibilityFlag(true);
	
	if(_pCD->m_iPlayer != -1)
	{
		if(m_ClientFlags & PLAYER_CLIENTFLAGS_PLAYERSPEAK)
		{
			if (!_pCD->m_DialogueInstance.IsValid())
			{
				// Player has stopped talking.
				ClientFlags() &= ~(PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK | PLAYER_CLIENTFLAGS_PLAYERSPEAK);
			}
		}

		if(_pCD->m_iDialogueTargetObject != 0)
		{
			// Abort dialogue under certain conditions
			int lTest[3];
			int nTest = 2;
			lTest[0] = m_iObject;
			lTest[1] = _pCD->m_iDialogueTargetObject;
			lTest[2] = _pCD->m_PlayerDialogueToken.m_iLastSpeaker;
			if(lTest[2] != 0 && lTest[2] != lTest[1] && lTest[2] != lTest[0])
				nTest++;

			bool bSpeaking = false;
			int i;
			for(i = 0; i < nTest; i++)
			{
				CWObject_Character *pObj = TDynamicCast<CWObject_Character>(m_pWServer->Object_Get(lTest[i]));
				CWO_Character_ClientData* pCDDialog = (pObj ? GetClientData(pObj) : NULL);
				if(pObj && pCDDialog)
				{
					if (pCDDialog->m_DialogueInstance.IsValid())
						bSpeaking = true;
					if((CWObject_Character::Char_GetPhysType(pObj) == PLAYER_PHYS_DEAD || pCDDialog->m_AnimGraph2.GetPropertyBool(PROPERTY_BOOL_ISSLEEPING)))
					{
						Char_EndDialogue();
						pObj->Char_DestroyDialogue();
						break;
					}
				}
			}
			
			/* FJ-NOTE: Removed: This ends dialogue prematurely..
			if(i == nTest && !bSpeaking && m_liDialogueChoices.Len() == 0)
				Char_EndDialogue();*/
		}

		if(m_Flags & PLAYER_FLAGS_TIMELEAP && Char_GetPhysType(this) == PLAYER_PHYS_DEAD)
			Char_EndTimeLeap();
	}
	// Refresh dialogue
	CWRes_Dialogue::CRefreshRes Res;
	OnRefresh_Dialogue(this, m_pWServer, 0, 0, &Res);
	EvalDialogueLink(Res);
	RefreshInteractiveDialogue();

	//Refresh abilities. We should fix proper ability handling later...
	if (!(Char_GetPhysType(this) == PLAYER_PHYS_DEAD))
		OnRefresh_Stealth();
}

extern fp32 AngleAdjust(fp32 _AngleA, fp32 _AngleB);
void CWObject_Character::OnRefresh_ServerPredicted_Extras(CWO_Character_ClientData *_pCD, CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	MAUTOSTRIP(CWObject_Character_OnRefresh_ServerPredicted_Extras, MAUTOSTRIP_VOID);
	MSCOPE(CWObject_Character::OnRefresh_ServerPredicted_Extras, CHARACTER);

	// ===========================================================================================
	// Decrease PainSoundDelay Counter (This is only needed on the server side, right?)
	if (_pCD->m_PainSoundDelay > 0)
	{
//		ConOut(CStrF("OnRefresh: PainSoundDelay = %d (decrease)", _pCD->m_PainSoundDelay));
		_pCD->m_PainSoundDelay = _pCD->m_PainSoundDelay - 1;
	}
	else
	{
		_pCD->m_PainSoundDelay = 0;
	}

	// All moderate speeds are adjusted for 20hz, adjust for dynamic rate
	fp32 ModerateSpeedAdjustment = 20.0f * _pWPhysState->GetGameTickTime();

	// If turn correction should be applied use this (should only be used when moving
	// The wanted angle is the control stick angle (in the beginning atleast
	// later on when backwards running comes in we want that too don't we
	// so the wanted angle should be created somewhere else and then used here or something..
	int32 TokenFlags = _pCD->m_AnimGraph2.GetStateFlagsLo();
	if ((TokenFlags & AG2_STATEFLAG_USETURNCORRECTION) && !_pCD->m_AnimGraph2.GetPropertyBool(PROPERTY_BOOL_DISABLETURNCORRECTION))
	{
		int32 AnimPhysMoveType = (int32)_pCD->m_AnimGraph2.GetAnimPhysMoveType();
		//ConOutL(CStrF("PhysMoveType: %d",AnimPhysMoveType));
		AdjustTurnCorrection(_pObj, _pCD, AnimPhysMoveType,_pWPhysState);
		/*if (_pCD->m_iPlayer == -1)
			ConOut(CStrF("Before: %f After: %f", Before, After));*/
	}
	else
	{
		AdjustTurnCorrection(_pObj, _pCD, ANIMPHYSMOVETYPE_RESET,_pWPhysState);
	}

	// ===========================================================================================
	// Player stuff
	if (_pCD->m_iPlayer != -1 || ((_pCD->m_iPlayer != -1) && (Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD)))
	{
		if (_pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_CREEPINGCAM)
		{
			CWObject_CoreData* pCreep = _pWPhysState->Object_GetCD(_pCD->m_iCreepingDark);
			if (pCreep)
				CWObject_CreepingDark::UpdateOrientation(_pWPhysState, pCreep, _pWPhysState->GetGameTickTime());
		}
		// Zoom
		_pCD->m_LastZoom = _pCD->m_CurZoom;
		_pCD->m_CameraUtil.UpdateOffsetVars();

		bool bMounted = _pCD->m_iMountedObject != 0;
		if (InCutscene(_pObj) || bMounted)
			_pCD->m_CurZoom = 1.0f;
		else
			_pCD->m_CurZoom = (fp32)_pCD->m_MaxZoom;
		
		// HealthHud
		uint32 GameTick = _pCD->m_GameTick;
		fp32 WantedHealthHudFade = 0.0f;
		if (_pCD->m_HealthHudStart != 0)
			WantedHealthHudFade = ((_pCD->m_HealthHudStart + 115) > GameTick) ? 1.0f : 0.0f;

		if(_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), _pWPhysState->Game_GetObjectIndex()))
			WantedHealthHudFade = 1.0f;

		_pCD->m_LastHealthHudFade = _pCD->m_HealthHudFade;
		WantedHealthHudFade *= 100.0f;
		_pCD->m_HealthHudFade *= 100.0f;
		_pCD->m_HealthHudFadeChange *= 100.0f;
		Moderatef(_pCD->m_HealthHudFade, WantedHealthHudFade, _pCD->m_HealthHudFadeChange, (int)(PLAYER_TARGETHUD_FADESPEED*ModerateSpeedAdjustment));
		_pCD->m_HealthHudFade *= (1.0f/100.0f);
		_pCD->m_HealthHudFadeChange *= (1.0f/100.0f);

		// Update darkness visibility (nv)
		_pCD->m_LastDarknessVisibility = _pCD->m_CurrentDarknessVisibility;
		_pCD->m_CurrentDarknessVisibility = LERP(_pCD->m_CurrentDarknessVisibility,(_pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSVISION ? 255 : 0),0.1f);
		//WantedHealthHudFade = ((fp32)_pCD->m_DarknessVisibility) * (100.0f/255.0f);
		/*WantedHealthHudFade = ((fp32)(_pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_DARKNESSVISION) != 0)*100.0f;
		_pCD->m_LastDarknessVisibility = _pCD->m_CurrentDarknessVisibility;
		_pCD->m_CurrentDarknessVisibility *= 100.0f;
		_pCD->m_DarknessVisibilityChange *= 100.0f;
		Moderatef(_pCD->m_CurrentDarknessVisibility, WantedHealthHudFade, _pCD->m_DarknessVisibilityChange, 100);
		_pCD->m_CurrentDarknessVisibility *= 1.0f/100.0f;
		_pCD->m_DarknessVisibilityChange *= 1.0f/100.0f;*/

		int32 ControlMode = Char_GetControlMode(_pObj);
		int32 CameraMode = _pCD->m_ActionCutSceneCameraMode;
		if (((_pCD->m_ExtraFlags & (PLAYER_EXTRAFLAGS_THIRDPERSON | PLAYER_EXTRAFLAGS_FORCETHIRPERSON)) != 0)
			|| (CameraMode & CActionCutsceneCamera::ACS_CAMERAMODE_ACTIVE))
		{
			// Update 3'rd person camera
			if (CameraMode & CActionCutsceneCamera::ACS_CAMERAMODE_ACTIVE)
			{
				CActionCutsceneCamera ActionCam;
				ActionCam.GetCamVars().ImportFromPCD(_pCD, _pObj->m_iObject);
				ActionCam.OnClientRefresh(ActionCam.GetCamVars(),_pWPhysState, _pCD);
			}
			else
			{
				// "Normal" (enclave) 3'rd person camera
				Camera_OnRefresh(_pObj, _pWPhysState);
			}
		}
		else if (ControlMode !=  PLAYER_CONTROLMODE_LEAN)
		{
			// Reset actioncutscene camera variables since they shouldn't interfere next time
			// we use them
			CamVars Temp;
			Temp.SetToPCD(_pCD);
			Temp.UpdateModVars(_pCD);
		}


		int32 iFightChar = (_pCD->m_iFightingCharacter != -1 ? _pCD->m_iFightingCharacter : 
				((_pCD->m_FocusFrameType & SELECTION_MASK_TYPEINVALID) == SELECTION_CHAR ? _pCD->m_iFocusFrameObject : -1));
		if (iFightChar == -1)
			iFightChar = (_pCD->m_iCloseObject ? _pCD->m_iCloseObject : -1);

		if ((iFightChar != -1) && 
			!_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ISDEAD),
			iFightChar))
		{
			if (TokenFlags & AG2_STATEFLAG_APPLYFIGHTCORRECTION)
			{
				CVec3Dfp32 Dir;
				if (_pCD->m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_EXACTSTARTPOSITION)
				{
					//CMat4Dfp32 StartPos = _pCD->m_AnimGraph2.GetDestination();
					CMat4Dfp32 StartPos = _pCD->m_DialoguePlayerPos;
					_pCD->m_AnimGraph2.SetDestination(StartPos);
					if (StartPos.GetRow(3) != CVec3Dfp32(_FP32_MAX))
						Dir = StartPos.GetRow(0);
					else
						Dir = _pObj->GetPositionMatrix().GetRow(0);
				}
				else
				{
					CVec3Dfp32 OpponentPos = _pWPhysState->Object_GetPosition(iFightChar);
					Dir = OpponentPos - _pObj->GetPosition();
					Dir.Normalize();
				}
				// Want to look at opponents torso, or rather keep the character in frame all the time
				// (keeping the opponents head at the top of the screen
				//fp32 HeightDiff = 6.0f;
				//fp32 YVal = M_ATan(HeightDiff/Distance)*(1.0f / _PI);

				// Ok, start moderating towards opponent
				fp32 WantedFightAngleX,WantedFightAngleY;

				// Ah yes we must moderate towards the closest angle as well...
				WantedFightAngleX = 1.0f - CVec3Dfp32::AngleFromVector(Dir[0], Dir[1]);
				
				//WantedFightAngleY = -0.080f + YVal;//-0.025f;
				WantedFightAngleY = 0.0f;//-0.025f;
				
				WantedFightAngleX += AngleAdjust(_pCD->m_Control_Look_Wanted[2], WantedFightAngleX);
				WantedFightAngleY += AngleAdjust(_pCD->m_Control_Look_Wanted[1], WantedFightAngleY);
				
				_pCD->m_FightLastAngleX = _pCD->m_FightTargetAngleX;
				WantedFightAngleX *= 100.0f;
				_pCD->m_FightTargetAngleX *= 100.0f;
				_pCD->m_FightAngleXChange *= 100.0f;
				Moderatef(_pCD->m_FightTargetAngleX, WantedFightAngleX, _pCD->m_FightAngleXChange, (int)(220*ModerateSpeedAdjustment));
				_pCD->m_FightTargetAngleX *= (1.0f/100.0f);
				_pCD->m_FightAngleXChange *= (1.0f/100.0f);

				_pCD->m_FightLastAngleY = _pCD->m_FightTargetAngleY;
				WantedFightAngleY *= 100.0f;
				_pCD->m_FightTargetAngleY *= 100.0f;
				_pCD->m_FightAngleYChange *= 100.0f;
				Moderatef(_pCD->m_FightTargetAngleY, WantedFightAngleY, _pCD->m_FightAngleYChange, (int)(220*ModerateSpeedAdjustment));
				_pCD->m_FightTargetAngleY *= (1.0f/100.0f);
				_pCD->m_FightAngleYChange *= (1.0f/100.0f);
			}

			/*if ((Char_GetControlMode(_pObj) == PLAYER_CONTROLMODE_FIGHTING) && 
				(_pCD->m_FightModeFlag & PLAYER_FIGHTMODEFLAG_SNEAK))
			{
				// Break neck distance
				Dir = (OpponentPos - _pObj->GetPosition());
				fp32 TargetValue = Dir.k[2];
				_pCD->m_BreakNeckLastHeight = _pCD->m_BreakNeckTargetHeight;
				TargetValue *= 100.0f;
				_pCD->m_BreakNeckTargetHeight *= 100.0f;
				_pCD->m_BreakNeckHeightChange *= 100.0f;
				Moderatef(_pCD->m_BreakNeckTargetHeight, TargetValue, _pCD->m_BreakNeckHeightChange, (int)(220*ModerateSpeedAdjustment));
				_pCD->m_BreakNeckTargetHeight *= (1.0f/100.0f);
				_pCD->m_BreakNeckHeightChange *= (1.0f/100.0f);
			}*/
		}
		else if (ControlMode != PLAYER_CONTROLMODE_LEAN)
		{
			// Always update towards the real angle...
			_pCD->m_FightLastAngleX = _pCD->m_Control_Look_Wanted[2];
			_pCD->m_FightTargetAngleX = _pCD->m_Control_Look_Wanted[2];
			_pCD->m_FightAngleXChange = 0.0f;

			_pCD->m_FightLastAngleY = _pCD->m_Control_Look_Wanted[1];
			_pCD->m_FightTargetAngleY = _pCD->m_Control_Look_Wanted[1];
			_pCD->m_FightAngleYChange = 0.0f;
		}

		if (ControlMode == PLAYER_CONTROLMODE_LEAN)
		{
			// Lean stuff
			// Update angle offsets
			//ConOut(CStrF("Massa update: %f", _pWPhys->GetGameTime()));
			fp32 WantedAngleOffset = _pCD->m_ActionCutSceneCameraOffsetX * 100;
			_pCD->m_ACSLastAngleOffsetX = _pCD->m_ACSTargetAngleOffsetX;
			_pCD->m_ACSTargetAngleOffsetX *= 100.0f;
			_pCD->m_ACSAngleOffsetXChange *= 100.0f;
			Moderatef(_pCD->m_ACSTargetAngleOffsetX, WantedAngleOffset, _pCD->m_ACSAngleOffsetXChange, 
				(int)(PLAYER_TARGETHUD_FADESPEED*ModerateSpeedAdjustment));
			_pCD->m_ACSTargetAngleOffsetX *= (1.0f/100.0f);
			_pCD->m_ACSAngleOffsetXChange *= (1.0f/100.0f);

			WantedAngleOffset = _pCD->m_ActionCutSceneCameraOffsetY * 100;
			_pCD->m_ACSLastAngleOffsetY = _pCD->m_ACSTargetAngleOffsetY;
			_pCD->m_ACSTargetAngleOffsetY *= 100.0f;
			_pCD->m_ACSAngleOffsetYChange *= 100.0f;
			Moderatef(_pCD->m_ACSTargetAngleOffsetY, WantedAngleOffset, _pCD->m_ACSAngleOffsetYChange, 
				(int)(PLAYER_TARGETHUD_FADESPEED*ModerateSpeedAdjustment));
			_pCD->m_ACSTargetAngleOffsetY *= (1.0f/100.0f);
			_pCD->m_ACSAngleOffsetYChange *= (1.0f/100.0f);

			// Base direction
			fp32 WantedAngleX = _pCD->m_ACSLastTargetDistance;
			// Ah yes we must moderate towards the closest angle as well...
			WantedAngleX += AngleAdjust(_pCD->m_Control_Look_Wanted[2], WantedAngleX);

			//ConOut(CStrF("WantedAngleX: %f",WantedAngleX));

			_pCD->m_FightLastAngleX = _pCD->m_FightTargetAngleX;
			WantedAngleX *= 100.0f;
			_pCD->m_FightTargetAngleX *= 100.0f;
			_pCD->m_FightAngleXChange *= 100.0f;
			Moderatef(_pCD->m_FightTargetAngleX, WantedAngleX, _pCD->m_FightAngleXChange, (int)(220*ModerateSpeedAdjustment));
			_pCD->m_FightTargetAngleX *= (1.0f/100.0f);
			_pCD->m_FightAngleXChange *= (1.0f/100.0f);
		}
	}
}


void CWObject_Character::OnRefresh_ServerPredicted(CWO_Character_ClientData *_pCD, CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	MAUTOSTRIP(CWObject_Character_OnRefresh_ServerPredicted, MAUTOSTRIP_VOID);
	// Run on server and on predicted players.

	// Added by Mondelore.
	OnRefresh_ServerPredicted_Extras(_pCD, _pObj, _pWPhysState);
	_pCD->m_CameraUtil.Refresh(_pWPhysState, _pCD);
	_pCD->UpdateButtonPressTicks();

	// Count frames we have been flying.
	if (_pCD->m_Phys_bInAir)
	{
		if (_pCD->m_Phys_nInAirFrames < 127)
			_pCD->m_Phys_nInAirFrames++;
	}
	else
		_pCD->m_Phys_nInAirFrames = 0;

	// Count down auto tilt delay.
	if (_pCD->m_Control_AutoTiltCountDown > 0)
		_pCD->m_Control_AutoTiltCountDown--;

	switch(Char_GetControlMode(_pObj))
	{
	case PLAYER_CONTROLMODE_FORCEMOVE :
		{
			_pCD->m_ControlMode_Param3 = _pCD->m_ControlMode_Param3 - _pWPhysState->GetGameTickTime();
			if (_pCD->m_ControlMode_Param3 <= 0.0f)
			{
				Char_SetControlMode(_pObj, PLAYER_CONTROLMODE_FREE);
				_pCD->m_ControlMode_Param3 = 0;
			}
		}
		break;
	case PLAYER_CONTROLMODE_LADDER :
		{
			// Since it is a uint 8, and it isn't necesssary to keep further track. We limit it to be max 200.
			if(_pCD->m_Anim_ClimbTickCounter < 200)
				_pCD->m_Anim_ClimbTickCounter = _pCD->m_Anim_ClimbTickCounter+1;
		}
		break;

	default :;
	}
}

// Run from OnClientRefresh, but only in demo playback
void CWObject_Character::OnClientRefresh_DemoPlayback(CWO_Character_ClientData *_pCD, CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	MAUTOSTRIP(CWObject_Character_OnClientRefresh_DemoPlayback, MAUTOSTRIP_VOID);
	OnRefresh_ServerPredicted_Extras(_pCD, _pObj, _pWPhysState);
}

void CWObject_Character::OnClientRefresh_Mirror(CWO_Character_ClientData *_pCD, CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_Character_OnClientRefresh_Mirror, MAUTOSTRIP_VOID);
}


void CWObject_Character::OnClientRefresh_TrueClient(CWO_Character_ClientData *_pCD, CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_Character_OnClientRefresh_TrueClient, MAUTOSTRIP_VOID);
//	M_CALLGRAPH;

	MSCOPE(CWObject_Character::OnClientRefresh_TrueClient, OCR_TrueClientMem);

//	const CRegistry *pReg = _pWClient->Registry_GetUser();

	_pCD->m_GameTick += 1;
	_pCD->m_GameTime = PHYSSTATE_TICKS_TO_TIME(_pCD->m_GameTick, _pWClient);
	if(_pCD->m_bIsPredicting)
	{
		//Char_SetGameTickDiff(_pCD->m_GameTick - _pWClient->GetGameTick());
		_pObj->m_Data[PLAYER_DATAINDEX_CONTROL] &= 0xffffff00;
		_pObj->m_Data[PLAYER_DATAINDEX_CONTROL] |= (_pCD->m_GameTick - _pWClient->GetGameTick()) & 0xff;
	}

	// Added 12/2-04 to fix ledge camera
#ifdef WCLIENT_FIXEDRATE
	OnRefresh_ServerPredicted(_pCD, _pObj, _pWClient);
#endif

	_pCD->OnClientRefresh(_pObj, _pWClient, _pCD->m_GameTime);
	Client_Anim(_pObj, _pWClient, _pCD, 16);

	CWRes_Dialogue::CRefreshRes Res;
	OnRefresh_Dialogue(_pObj, _pWClient, 0, 0, &Res);
	EvalDialogueLink_Client(_pObj, _pWClient, Res);

	Char_EvolveClientEffects(_pObj, _pWClient);
	OnRefresh_Predicted_Target(_pObj, _pWClient);

	_pCD->m_Choices = "";
//	if (_pCD->m_3PI_FocusObject != 0 && (_pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_STATE_MASK) != THIRDPERSONINTERACTIVE_STATE_LEAVING)
	if (_pCD->m_iFocusFrameObject > 0)
	{
		CWObject_Message Msg(OBJMSG_CHAR_GETCHOICES);
		Msg.m_pData = &_pCD->m_Choices;
		Msg.m_iSender = _pObj->m_iObject;
		_pCD->m_nChoices = _pWClient->ClientMessage_SendToObject(Msg, _pCD->m_iFocusFrameObject);

		// Override if choice is telephone with available telephone numbers to be selected directly
		if (_pCD->m_Choices.CompareNoCase("§LACS_TELEPHONE") == 0)
		{
			if (!_pCD->m_DialogueInstance.IsValid())
			{
				_pCD->m_Choices = "";
				_pCD->m_nChoices = 0;

				int nItems = _pCD->m_InventoryInfo.m_lInventory.Len();
				for (int i = 0; i < nItems; i++)
				{
					if(_pCD->m_InventoryInfo.m_lInventory[i].m_Flags & CWO_Inventory_Info::INVENTORYINFO_TYPE_PHONEBOOK)
					{
						// Get the phonenumber from the itemdescription
						CStr TemStr = Localize_Str(_pCD->m_InventoryInfo.m_lInventory[i].m_ItemDescription);
						int iFinder = TemStr.Find("555");
						if(iFinder != -1)
						{
							TemStr = TemStr.DelTo(iFinder-1);
							TemStr = TemStr.DelFrom(10); // 8 +1 for "-" and +1 for " "
							TemStr = TemStr.Del(7,1);
							TemStr = TemStr.Del(3,1);

							TemStr = "§LPHONENUMBER_OWNER_" + TemStr;
							TemStr = Localize_Str(TemStr);

							_pCD->m_Choices += TemStr;
							_pCD->m_Choices += ";";
							_pCD->m_nChoices++;
						}
						
					}
				}

				_pCD->m_Choices += Localize_Str("§LGUI_DIAL_MANUALLY");
				_pCD->m_Choices += ";";
				_pCD->m_nChoices++;
			}
			else
			{
				_pCD->m_Choices = "";
				_pCD->m_nChoices = 0;
			}
		}
	}
	else
	{
		_pCD->m_iLastFocusFrameObject = -1;
		_pCD->m_nChoices = 0;
		_pCD->m_iCurChoice = 0;
	}

	if(_pCD->m_iPlayer != -1)
	{
		// PLAYER ONLY CODE

//		OnRefresh_SoundVolumes(_pObj, _pWClient);

		int nItems = Min(_pCD->m_MaxHealth >> 3, PLAYER_MAXHEALTH_HUD);
		int Health = _pCD->m_Health;
		int iSlot = Health >> 4;
		for(int i = 0; i < nItems; i++)
		{
			int Fraction;
			if(i < iSlot)
				Fraction = 15;
			else if(i > iSlot)
				Fraction = 0;
			else
				Fraction = Health - i * 16;

			_pCD->m_HealthHudItem[i].SetFraction(Fraction);
		}

		if(_pCD->m_bDoOneTimeEffects && _pCD->m_CrouchLevel.Get(_pCD->m_GameTick, _pCD->m_PredictFrameFrac) > 0.8f)
		{
			// Find closes characters
			TSelection<CSelection::LARGE_BUFFER> Selection;
			CWO_PhysicsState PhysState;
			PhysState.m_Prim[0].Create(OBJECT_PRIMTYPE_SPHERE, -1, CVec3Dfp32(255, 0, 0), 0);
			PhysState.m_nPrim = 1;
			PhysState.m_ObjectFlags = 0;
			PhysState.m_MediumFlags = 0;
			PhysState.m_ObjectIntersectFlags = OBJECT_FLAGS_CHARACTER;
			PhysState.m_iExclude = _pObj->m_iObject;
			_pWClient->Selection_AddIntersection(Selection, _pObj->GetPosition(), PhysState);
			const int16 *liSel;
			int nSel = _pWClient->Selection_Get(Selection, &liSel);
			const int nMaxHeartbeats = 2;
			CWObject_Client *lpObj[nMaxHeartbeats];
			fp32 lDistance[nMaxHeartbeats];
			int nObj = 0;
			int iMaxDinstance = 0;
			for(int i = 0; i < nSel; i++)
			{
				CWObject_Client *pObj = _pWClient->Object_Get(liSel[i]);
				if(pObj)
				{
					fp32 Distance = (_pObj->GetPosition() - pObj->GetPosition()).LengthSqr();
					if(nObj < nMaxHeartbeats)
					{
						lpObj[nObj] = pObj;
						lDistance[nObj]= Distance;
						if(Distance > lDistance[iMaxDinstance])
							iMaxDinstance = nObj;
						nObj++;
					}
					else if(Distance < lDistance[iMaxDinstance])
					{
						lDistance[iMaxDinstance] = Distance;
						lpObj[iMaxDinstance] = pObj;
						for(int j = 0; j < nMaxHeartbeats; j++)
							if(lDistance[j] > lDistance[iMaxDinstance])
								iMaxDinstance = j;
					}
				}
			}
			for(int j = 0; j < nObj; j++)
				GetHeartBeat(lpObj[j], _pWClient, _pCD->m_GameTime, true);

		}
	}
}


//
// Checks for sound volumes around the player and applies sounds accordingly
//
void CWObject_Character::OnRefresh_SoundVolumes(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	// Get client data

/*	wtf was this placed here?   -mh

	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if(!pCD)
		return;

	CMat4Dfp32 CameraMatrix;
	_pWClient->Render_GetLastRenderCamera(CameraMatrix);
	_pWClient->Sound_UpdateSoundVolumes(CVec3Dfp32::GetMatrixRow(CameraMatrix, 3));
	_pWClient->Sound_UpdateTrackSounds();
*/
}



// 
//
//
void CWObject_Character::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_Character_OnClientRefresh, MAUTOSTRIP_VOID);
	MSCOPE(CWObject_Character::OnClientRefresh, CHARACTER);
	
	CWO_Character_ClientData *pCD= CWObject_Character::GetClientData(_pObj);
	if (!pCD) Error_static("CWObject_Character::OnClientRefresh", "NULL ClientData");

	if (_pWClient->GetRecMode() == 2)
		OnClientRefresh_DemoPlayback(pCD, _pObj, _pWClient);

	// Adjust camera effects
	const fp32 GameTickTime = _pWClient->GetGameTickTime();

	pCD->m_CreepingLensLast = pCD->m_CreepingLens;
	pCD->m_AncientLensLast = pCD->m_AncientLens;

	pCD->m_CreepingLens = (fp32)LERP(pCD->m_CreepingLens, (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK) ? 1.0f : 0.0f, GameTickTime*2.0f);
	pCD->m_AncientLens = (fp32)LERP(pCD->m_AncientLens, (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS) ? 1.0f : 0.0f, GameTickTime*2.0f);
	pCD->m_Disorientation_Flash = (fp32)LERP(pCD->m_Disorientation_Flash, 0, GameTickTime*6.0f);

	bool bShield = (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD) != 0;
	bool bHitEffectDamage = (pCD->m_HitEffect_Flags & PLAYER_HITEFFECT_FLAGS_DAMAGE) != 0;
	fp32 HitEffectTarget = bHitEffectDamage ? 1.0f : 0.0f;
	fp32 LerpMul = (bHitEffectDamage) ? 3.0f : 1.0f;
	fp32 HitEffectLerp = LerpMul * GameTickTime * ((bShield || bHitEffectDamage) ? 8.0f : 4.0f);
	fp32 HitEffect_LastTime = pCD->m_HitEffect_Time;
	pCD->m_HitEffect_LastTime = HitEffect_LastTime;
	pCD->m_HitEffect_Time = (fp32)LERP(HitEffect_LastTime, HitEffectTarget, HitEffectLerp);

	if (pCD->m_RenderAttached != 0)
	{
		CMat4Dfp32 Mat;
		CWObject_Message Msg(OBJMSG_HOOK_GETCURRENTMATRIX, (aint)&Mat, pCD->m_iAttachJoint);
		if(_pWClient->ClientMessage_SendToObject(Msg, pCD->m_RenderAttached))
		{	
			CMat4Dfp32 MatCurrent = _pObj->GetPositionMatrix();
			Mat.GetRow(3).SetMatrixRow(MatCurrent,3);
			_pWClient->Object_ForcePosition_World(_pObj->m_iObject, MatCurrent);
		}
	}

	
	//--------------------------------------------------------------------------------
	// IK needs traces to be done here, instead of via OnAnimGetState due to 
	// MultiTreading problems with PhysStates intersections
	// is it a darkling?

	if(Char_GetPhysType(_pObj) != PLAYER_PHYS_DEAD )
	{
		if(pCD->IsDarkling()) // is darkling
		{
			pCD->m_PostAnimSystem.GatherFeetCollisionData(CPostAnimSystem::FEET_TYPE_DARKLING_QUADRO, _pWClient, 1.0f);
		}
		else
		{
			// Moved to CPostianimSystem::EvalHandsonWeapon.
			if((pCD->m_iPlayer != -1) && !(pCD->m_AnimGraph2.GetStateFlagsHiCombined()& AG2_STATEFLAGHI_IKSYSTEM_IGNOREHANDS)) // is player and "ignore" not set
			{
				if(pCD->m_PostAnimSystem.GetWeaponHoldType() == 0)
				{
					TSelection<CSelection::SMALL_BUFFER> Selection;
					pCD->m_PostAnimSystem.GatherWeaponWallCollisionData(-1, _pWClient, 0, &Selection);
				}

				//pCD->m_PostAnimSystem.GatherAimCollisionData(_pWClient, pCD->m_AimTarget, true);
			}
			
			fp32 TestScale = pCD->m_CharGlobalScale;
			if(pCD->m_iPlayer == -1)
			{
				pCD->m_PostAnimSystem.GatherFeetCollisionData(CPostAnimSystem::FEET_TYPE_HUMAN_BIPED, _pWClient, pCD->m_CharGlobalScale);
				pCD->m_PostAnimSystem.GatherAimCollisionData(_pWClient, pCD->m_AimTarget, false);
			}
		}
	}
	
	if (!IsMirror(_pObj, _pWClient))
	{
		int bOneTimeOld = pCD->m_bDoOneTimeEffects;
		pCD->m_bDoOneTimeEffects = 1;
		if (!(Char_GetPhysType(_pObj) == PLAYER_PHYS_NOCLIP))
		{
			// Evolves from tick base forward 1 tick
			CMTime Time = CMTime::CreateFromTicks(pCD->m_GameTick, _pWClient->GetGameTickTime(), 0.0f);
			CWAG2I_Context AG2IContext(_pObj, _pWClient, Time);
			/*if (pCD->m_iPlayer != -1)
				AG2IContext.m_TimeSpan = pCD->m_PredictFrameFrac * _pWClient->GetGameTickTime();*/
			CWorld_ClientCore *pWClientCore = safe_cast<CWorld_ClientCore>(_pWClient);
			if (pWClientCore && pWClientCore->m_spSound != NULL && 
				pCD->m_VoCap.IsActiveFullLayer(pCD->m_AnimGraph2.GetPropertyInt(PROPERTY_INT_VOCAPINFO) & AG2_VOCAP_FLAG_IDLE))
			{
				// Get event layers from vocap instead
				CEventLayer Layers[8];
				CAG2TokenID IDS[8];
				memset(IDS,0,sizeof(IDS));
				int32 nLayers = 8;
				bint bIdle = pCD->m_AnimGraph2.GetPropertyInt(PROPERTY_INT_VOCAPINFO) & AG2_VOCAP_FLAG_IDLE;
				if (pCD->m_VoCap.GetEventLayers(pWClientCore->m_spSound, _pObj->m_ClientData[0], Time, bIdle, Layers, nLayers))
				{
					// Set timespan to new_time - old_time
					fp32 dt = pCD->m_VoCap.m_spAnimQueue->m_EventTime - Layers[0].m_Layer.m_Time;
					dt = Clamp(dt, 0.0f, 1.0f);
					AG2IContext.m_TimeSpan = Max(0.0f, dt);
					uint EventMask = ANIM_EVENT_MASK_EFFECT | ANIM_EVENT_MASK_SOUND | ANIM_EVENT_MASK_GAMEPLAY;
					pCD->m_AnimGraph2.GetAG2I()->CheckAnimEvents(&AG2IContext, Layers, IDS, nLayers, OBJMSG_CHAR_ONANIMEVENT, EventMask);
				}
			}
			else
			{
				pCD->m_AnimGraph2.GetAG2I()->CheckAnimEvents(&AG2IContext, OBJMSG_CHAR_ONANIMEVENT);
				// Check events on guns as well
				uint EventMask = ANIM_EVENT_MASK_EFFECT | ANIM_EVENT_MASK_SOUND | ANIM_EVENT_MASK_GAMEPLAY;
				if (pCD->m_iPlayer != -1)
					EventMask |= ANIM_EVENT_MASK_IK;
				pCD->m_WeaponAG2.GetAG2I()->CheckAnimEvents(&AG2IContext,OBJMSG_CHAR_ONANIMEVENT,EventMask,false);
			}
		}
		pCD->m_bDoOneTimeEffects = bOneTimeOld;
	}
	
	int LocalPlayer = IsLocalPlayer(_pObj, _pWClient);
	// Set if we're local player
	pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_LOCALPLAYER,LocalPlayer);
#ifndef WCLIENT_FIXEDRATE
	if ((_pWClient->GetRecMode() != 2) &&
		LocalPlayer &&
		!(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_SERVERCONTROL))
		return;
#endif

//	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pObj);
//	if (!pCD) Error_static("CWObject_Character::OnClientRefresh", "NULL ClientData");

	pCD->m_bIsPredicting = false;
	pCD->m_bIsClientRefresh =  true;
	pCD->m_bIsServerRefresh = false;
	pCD->m_bDoOneTimeEffects = !LocalPlayer;

	if (IsMirror(_pObj, _pWClient))
		OnClientRefresh_Mirror(pCD, _pObj, _pWClient);
	else
		OnClientRefresh_TrueClient(pCD, _pObj, _pWClient);

	// FIXME: AGI Refresh NoPredict
	if (_pWClient->Player_GetLocalObject() != _pObj->m_iObject || _pWClient->GetRecMode() == 2)
	{
		CWAG2I_Context AG2IContext(_pObj, _pWClient, pCD->m_GameTime);
		pCD->m_AnimGraph2.GetAG2I()->RefreshPredictionMisses(&AG2IContext);
	}

	pCD->m_bIsClientRefresh =  false;
	pCD->m_bDoOneTimeEffects = false;
}

CVec3Dfp32 CWObject_Character::GetAdjustedMovement(const CWO_Character_ClientData* _pCD)
{
	MAUTOSTRIP(CWObject_Character_GetAdjustedMovement, CVec3Dfp32());
	CVec3Dfp32 ControlMove = _pCD->m_Control_Move;

	if (_pCD->m_WantedZoomSpeed > 0.0f)
		ControlMove[0] = 0;

	for (int i = 0; i < 3; i++)
	{
		if (ControlMove[i] > 0.0f)
		{
			ControlMove[i] = Max(0.0f, ControlMove[i] + (fp32)_pCD->m_ControlMove_Offset);
		}
		else if (ControlMove[i] < 0.0f)
		{
			ControlMove[i] = Min(0.0f, ControlMove[i] - (fp32)_pCD->m_ControlMove_Offset);
		}
		ControlMove[i] *= _pCD->m_ControlMove_Scale;
	}
	return ControlMove;
}
/*
static fp32 GetHeading(const CVec3Dfp32& _Player, const CVec3Dfp32& _Target, fp32 _CurrentHeading)
{
	//Get angle in radians
	CVec3Dfp32 RelPos = _Player - _Target;
	RelPos[2] = 0; 
	RelPos.Normalize();
	fp32 Angle = M_ACos(RelPos[0]);
	if (RelPos[1] > 0) 
		Angle = -Angle;

	//Convert angle to fractions
	Angle *= (1.0f/_PI2);

	// _CurrentHeading = gamla vinkeln
	// Angle = nya vinkeln

//	if((_CurrentHeading >= 0.25f)
//	&& (Angle <= -0.25f))
//	{
//		fp32 Count = _CurrentHeading;
//		while(Count >= 0.25f)
//		{
//			Count -= 1.0f;
//			Angle += 1.0f;
//		}
//	}
//	else
//	if((_CurrentHeading < -0.25f)
//	&& (Angle >= 0.25f))
//	{
//		fp32 Count = _CurrentHeading;
//		while(Count < -0.25f)
//		{
//			Count += 1.0f;
//			Angle -= 1.0f;
//		}
//	}


	return Angle;
}

// Note that this doesn't work as GetPitch() in WObj_CharMecahnics.cpp
static fp32 GetPitch(const CVec3Dfp32& _Player, const CVec3Dfp32& _Target)
{
	//Get angle in radians
	CVec3Dfp32 RelPos = _Player - _Target;
	RelPos.Normalize();
	fp32 Angle = M_ASin(RelPos[2]);

	//Convert angle to fractions
	Angle *= (1.0f/_PI2);

	return Angle;
}
*/
#define AUTOAIM_MINSTRENGTH  0.8f
#define AUTOAIM_MAXSTRENGTH  6.0f


void CWObject_Character::Char_UpdateLook(CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD, CWorld_PhysState* _pWPhysState, fp32 _FrameFrac)
{
	MSCOPESHORT(CWObject_Character::Char_UpdateLook);
	MAUTOSTRIP(CWObject_Character_Char_UpdateLook, MAUTOSTRIP_VOID);
	fp32 MaxTilt = PLAYER_CAMERAMAXTILT;
	
	// Don't update when nolook flag is set
//	if (_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOLOOK)
//		return;

	int32 BlendInStartCache = _pCD->m_AnimCamBlendInStart;
	if (_pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_TAGUSEANIMCAMERA)
	{
		if (!_pCD->m_AnimCamBlendInStart)
			_pCD->m_AnimCamBlendInStart = _pCD->m_GameTick + 2;
	}
	else if (_pCD->m_AnimCamBlendOutStart < _pCD->m_AnimCamBlendInStart)
	{
		_pCD->m_AnimCamBlendOutStart = _pCD->m_GameTick + 2;
	}
	if (_pCD->m_AnimCamBlendInStart > _pCD->m_AnimCamBlendOutStart)
		_pCD->m_AnimCamBlendOutStart = 0;
	else if (_pCD->m_AnimCamBlendOutStart < _pCD->m_GameTick && (_pCD->m_GameTick - _pCD->m_AnimCamBlendOutStart) > 13)
		_pCD->m_AnimCamBlendInStart = 0;

	const int PhysType = Char_GetPhysType(_pObj);

	if (InNoClipMode(PhysType) || (_pCD->m_iPlayer == -1))
		MaxTilt = 0.249f;

	int ControlMode = Char_GetControlMode(_pObj);
	int32 iFightChar = (_pCD->m_iFightingCharacter != -1 ? _pCD->m_iFightingCharacter : 
				((_pCD->m_FocusFrameType & SELECTION_MASK_TYPEINVALID) == SELECTION_CHAR ? _pCD->m_iFocusFrameObject : -1));

	if (iFightChar == -1)
		iFightChar = (_pCD->m_iCloseObject ? _pCD->m_iCloseObject : -1);

	// Use some crap, like camera and stuff....
	if (ControlMode == PLAYER_CONTROLMODE_LEAN)
	{
		// Ah, we must set wanted control look to the old control look (seems that Look_Wanted
		// is used mostly yes?)
		fp32 BaseX = LERP(_pCD->m_FightLastAngleX, _pCD->m_FightTargetAngleX, _FrameFrac);
		_pCD->m_Control_Look_Wanted.k[2] = /*_pCD->m_ACSLastTargetDistance*/BaseX + LERP(_pCD->m_ACSLastAngleOffsetX, _pCD->m_ACSTargetAngleOffsetX, _FrameFrac);
		_pCD->m_Control_Look_Wanted.k[1] = LERP(_pCD->m_ACSLastAngleOffsetY, _pCD->m_ACSTargetAngleOffsetY, _FrameFrac);
		_pCD->m_Control_Look = _pCD->m_Control_Look_Wanted;
		return;
	}
	else if ((_pCD->m_iPlayer != -1) && (iFightChar != -1) && 
		(_pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_APPLYFIGHTCORRECTION) && 
		!_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ISDEAD),iFightChar))
	{
		// Only look on opponent if he's alive...
		_pCD->m_Control_Look_Wanted[2] = LERP(_pCD->m_FightLastAngleX, _pCD->m_FightTargetAngleX, _FrameFrac);
		_pCD->m_Control_Look_Wanted[1] = LERP(_pCD->m_FightLastAngleY, _pCD->m_FightTargetAngleY, _FrameFrac);
		_pCD->m_Control_Look = _pCD->m_Control_Look_Wanted;

		return;
	}
	else if (ControlMode == PLAYER_CONTROLMODE_LADDER)
	{
		// Set direction as the opposite of ladder/ledge direction
		int iLadder = (int)_pCD->m_ControlMode_Param0;
			
		CVec3Dfp32 LadderNormal;
		if (_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETNORMAL,
			0,0,_pObj->m_iObject, 0,0,0,&LadderNormal, sizeof(CVec3Dfp32)), iLadder))
		{
			LadderNormal = -LadderNormal;
			_pCD->m_Control_Look_Wanted = GetLook(LadderNormal);
			_pCD->m_Control_Look = _pCD->m_Control_Look_Wanted;
		}
	}
	else if ((ControlMode == PLAYER_CONTROLMODE_LEDGE2) ||(ControlMode == PLAYER_CONTROLMODE_HANGRAIL))
	{
		CVec3Dfp32 Direction = CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(),0);
		_pCD->m_Control_Look = _pCD->m_Control_Look_Wanted = CWObject_Character::GetLook(Direction);
	}
	else if (ControlMode == PLAYER_CONTROLMODE_ANIMSYNC)
	{
		_pCD->m_Control_Look.k[2] = LERP(_pCD->m_Control_Look.k[2],_pCD->m_RelAnimPos.GetControlLookX(),_FrameFrac);
		_pCD->m_Control_Look.k[1] = LERP(_pCD->m_Control_Look.k[1],0.0f,_FrameFrac);//0.0f;
		_pCD->m_Control_Look.k[0] = 0.0f;
		_pCD->m_Control_Look_Wanted = _pCD->m_Control_Look;
	}
	if (PhysType != PLAYER_PHYS_NOCLIP && _pCD->m_iMountedObject != 0 && _pCD->m_iPlayer != -1 && 
		!(_pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_TURRET))
	{
		// Limit amount we can look
		CWObject_CoreData* pMounted = _pWPhysState->Object_GetCD(_pCD->m_iMountedObject);
		CWO_Character_ClientData* pCDMounted = (pMounted ? GetClientData(pMounted) : NULL);
		if (pCDMounted)
		{
			fp32 Diff = _pCD->m_Control_Look_Wanted[2] - pCDMounted->m_Control_Look_Wanted[2];
			Diff = (Diff > 0.5f ? Diff - 1.0f : (Diff < -0.5f ? Diff + 1.0f : Diff));
			if (Diff  > 0.0f)
				Diff = Min(pCDMounted->m_AnimGraph2.GetMaxLookAngleZ(),Diff);
			else
				Diff = Max(-pCDMounted->m_AnimGraph2.GetMaxLookAngleZ(),Diff);

			fp32 NewVal = pCDMounted->m_Control_Look_Wanted[2] + Diff;
			NewVal = (NewVal > 1.0f ? NewVal - 1.0f : (NewVal < 0.0f ? NewVal + 1.0f : NewVal));
			//ConOut(CStrF("NewVal: %f Diff: %f Mounted: %f Self: %f", NewVal, Diff, pCDMounted->m_Control_Look_Wanted[2],_pCD->m_Control_Look_Wanted[2]));
			_pCD->m_Control_Look_Wanted[2] = NewVal;

			Diff = _pCD->m_Control_Look_Wanted[1] - pCDMounted->m_Control_Look_Wanted[1];
			if (Diff  > 0.0f)
				Diff = Min(pCDMounted->m_AnimGraph2.GetMaxLookAngleY(),Diff);
			else
				Diff = Max(-pCDMounted->m_AnimGraph2.GetMaxLookAngleY(),Diff);

			//ConOut(CStrF("NewVal: %f Diff: %f Mounted: %f Self: %f", NewVal, Diff, pCDMounted->m_Control_Look_Wanted[2],_pCD->m_Control_Look_Wanted[2]));
			_pCD->m_Control_Look_Wanted[1] = pCDMounted->m_Control_Look_Wanted[1] + Diff;
		}
	}

/*	else if (ControlMode == PLAYER_CONTROLMODE_HANGRAIL)
	{
		int iHangRail = (int)_pCD->m_ControlMode_Param0;
		CVec3Dfp32 Pos1,Pos2,HangRailDir;

		_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION1,
			0,0,_pObj->m_iObject, 0,0,0,&Pos1, sizeof(CVec3Dfp32)), iHangRail);
		if (_pWPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION2,
			0,0,_pObj->m_iObject, 0,0,0,&Pos2, sizeof(CVec3Dfp32)), iHangRail))
		{
			HangRailDir = Pos2 - Pos1;
			//HangRailDir.Normalize();
			//HangRailDir = -HangRailDir;
			_pCD->m_Control_Look_Wanted = GetLook(HangRailDir);
			_pCD->m_Control_Look = _pCD->m_Control_Look_Wanted;
		}
	}
	else if ((ControlMode == PLAYER_CONTROLMODE_LEDGE) || (ControlMode == PLAYER_CONTROLMODE_LEDGE2))
	{
		CVec3Dfp32 LedgeData[3];
		CWObject_Message LedgeMsg(OBJMSG_LEDGE_GETACTIVELEDGEINFO,0,0,0,0,0,0,LedgeData,sizeof(LedgeData));
		int iLedge = (int)_pCD->m_ControlMode_Param0;
		if (_pWPhysState->Phys_Message_SendToObject(LedgeMsg,iLedge))
		{
			LedgeData[2] = -LedgeData[2];
			_pCD->m_Control_Look_Wanted = GetLook(LedgeData[2]);
			_pCD->m_Control_Look = _pCD->m_Control_Look_Wanted;
		}
	}*/

	_pCD->m_Control_Look_Wanted[1] = Min(MaxTilt, Max(-MaxTilt, _pCD->m_Control_Look_Wanted[1]));

	// Forced tilt
	fp32 ForcedTiltBlend = LERP(_pCD->m_LastForcedTiltBlend, _pCD->m_ForcedTiltBlend, _FrameFrac);
	fp32 ForcedTilt = LERP(_pCD->m_LastForcedTilt, _pCD->m_ForcedTilt, _FrameFrac);
	if((!AssistCamera(_pCD)) || InNoClipMode(PhysType) || _pCD->m_iPlayer == -1)
		ForcedTiltBlend = 0.0f;
	fp32 Tilt = LERP(_pCD->m_Control_Look_Wanted[1], ForcedTilt, ForcedTiltBlend);

	// Forced look
	CVec2Dfp32 ForcedLook = LERP(_pCD->m_LastForcedLook, _pCD->m_ForcedLook, _FrameFrac);
	fp32 ForcedLookBlend = LERP(_pCD->m_LastForcedLookBlend, _pCD->m_ForcedLookBlend, _FrameFrac);
	if (InNoClipMode(PhysType))
		ForcedLookBlend = 0.0f;
	Tilt = LERP(Tilt, ForcedLook[0], ForcedLookBlend);
	if ((_pCD->m_Control_Look_Wanted[2] - ForcedLook[1]) > 0.5f)
		ForcedLook[1] += 1.0f;
	if ((_pCD->m_Control_Look_Wanted[2] - ForcedLook[1]) < -0.5f)
		ForcedLook[1] -= 1.0f;
	fp32 Turn = LERP(_pCD->m_Control_Look_Wanted[2], ForcedLook[1], ForcedLookBlend);

	_pCD->m_Control_Look[1] = Min(MaxTilt, Max(-MaxTilt, Tilt));
	_pCD->m_Control_Look[2] = Turn;

	if (IsForcedLook(_pCD) && (ForcedLookBlend >= 0.0f))
		_pCD->m_Control_Look_Wanted = _pCD->m_Control_Look;
	
	if(PhysType == PLAYER_PHYS_DEAD)
		_pCD->m_Control_Look = 0;

	// Meh, reset some crap
	_pCD->m_FightLastAngleX = _pCD->m_FightTargetAngleX = _pCD->m_Control_Look_Wanted[2];
	_pCD->m_FightAngleXChange = 0.0f;

	_pCD->m_FightLastAngleY = _pCD->m_FightTargetAngleY = _pCD->m_Control_Look_Wanted[1];
	_pCD->m_FightAngleYChange = 0.0f;





	// SAMME: Lock-target. Disabled until a better solution is found!
	/*int Offset = (_pWPhysState->IsClient())?1:0;
	if (_pCD->m_AimTarget && _pCD->m_iPlayer != -1 && 
		(_pCD->m_AimTargetStartTick <= _pWPhysState->GetGameTick()+Offset ||
		 _pCD->m_AimReleasingTarget && _pCD->m_AimTargetStartTick >= _pWPhysState->GetGameTick()+Offset))
	{
		fp32 SpeedPercent = _pWPhysState->Object_GetVelocity(_pObj->m_iObject).Length()/_pCD->m_Speed_Forward;
		CMTime CurTime = CMTime::CreateFromTicks(_pCD->m_GameTick, SERVER_TIMEPERFRAME, _FrameFrac);
		fp32 DeltaTime = Min(Max((CurTime-_pCD->m_AutoAim_LastUpdateTime).GetTime(),0.0f),SERVER_TIMEPERFRAME);

		fp32 TimeSinceLastLookChange = Max((CurTime-_pCD->m_AutoAim_LastLookMoveTime).GetTime(),0.0f);
		if(DeltaTime > 0.0f && SpeedPercent > 0.1f)// && TimeSinceLastLookChange > SERVER_TIMEPERFRAME)
		{
			CVec3Dfp32 PlayerPos = _pObj->GetPosition();
			CVec3Dfp32 TargetPos = _pWPhysState->Object_GetPosition(_pCD->m_AimTarget);

			fp32 HeightDiff = _pObj->GetAbsBoundBox()->m_Max.k[2] - _pObj->GetAbsBoundBox()->m_Min.k[2];
			fp32 HeightOffset = HeightDiff * _pCD->m_ThisAimHeight;
			PlayerPos.k[2] += HeightOffset;

			CWObject_CoreData *pAimObject = _pWPhysState->Object_GetCD(_pCD->m_AimTarget);
			if(pAimObject)
			{
				CWO_Character_ClientData *pCD = GetClientData(pAimObject);
				if(pCD)
				{
					fp32 HeightDiff = pAimObject->GetAbsBoundBox()->m_Max.k[2] - pAimObject->GetAbsBoundBox()->m_Min.k[2];
					fp32 HeightOffset = HeightDiff * pCD->m_ThisAimHeight;
					TargetPos.k[2] += HeightOffset;
				}
			}

			CVec3Dfp32 MoveDir = _pObj->GetMoveVelocity();

			MoveDir.Normalize();

			CVec3Dfp32 LockTargetLook = 0;
			CVec3Dfp32 RelPos = TargetPos - PlayerPos;
			fp32 TargetDistance = RelPos.Length();
			RelPos = RelPos/TargetDistance; 

			CVec3Dfp32 Fwd = CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(),0);

			CVec3Dfp32 RelPosRot90 = RelPos;
			fp32 Temp = RelPosRot90[1];
			RelPosRot90[1] = RelPosRot90[0];
			RelPosRot90[0] = -Temp;
			RelPosRot90[2] = 0;
			RelPosRot90.Normalize();
			fp32 DotProductMove = RelPosRot90 * MoveDir;
			fp32 DotProductFwd = RelPosRot90 * Fwd;
			// Ouch, cannot explain this in a simple way. But I'll try.
			// We only autoaim when both the movement vector and the look vector is on the same side of the 
			// plane defined by the direction to the target and (0,0,1). This means that we're heading away from the 
			// target. Should not be done if we're moving straight ahead though.
			if(DotProductFwd * DotProductMove > 0 && Fwd * MoveDir < 0.95f) 
			{
				MSCOPE(CWObject_Character:: FollowTarget, CHARACTER);
				//Convert angle to fractions

				LockTargetLook.k[0] = 0.0f;
				LockTargetLook.k[1] = GetPitch(TargetPos, PlayerPos);
				LockTargetLook.k[2] = GetHeading(TargetPos, PlayerPos, _pCD->m_Control_Look.k[2]);
				CMat4Dfp32 a, b, c;
				const CVec3Dfp32& Prev = _pCD->m_Control_Look;

				LockTargetLook.CreateMatrixFromAngles(0, a);
				Prev.CreateMatrixFromAngles(0, b);

				CVec3Dfp32 TarFwd = CVec3Dfp32::GetMatrixRow(a,0);
				CVec3Dfp32 Fwd = CVec3Dfp32::GetMatrixRow(b,0);

			//	fp32 Dist = Fwd.Distance(TarFwd);
				///				fp32 DistComp = Max(0.50f - Dist, 0.0f);
				fp32 DotDir = Fwd * TarFwd;

				fp32 MaxAngle = TARGET_CLOSEANGLE + (TargetDistance/TARGET_MAXDIST) * (TARGET_FARANGLE-TARGET_CLOSEANGLE);
				fp32 DistComp = Max((DotDir - MaxAngle)/(1.0f - MaxAngle) - 0.1f, 0.0f); 

				fp32 DistCorr = M_Pow((TARGET_MAXDIST-TargetDistance)/TARGET_MAXDIST,3);
				fp32 AutoaimPull = AUTOAIM_MINSTRENGTH + DistCorr * (AUTOAIM_MAXSTRENGTH-AUTOAIM_MINSTRENGTH);

				CQuatfp32 Q1, Q2, QD;
				Q1.Create(b);
				Q2.Create(a);
				Q1.Interpolate(Q2, QD, AutoaimPull*SpeedPercent*DeltaTime*DistComp);
				QD.CreateMatrix(c);

				_pCD->m_AutoAim_LastUpdateTime = CurTime;

				CVec3Dfp32 Res;
				Res.CreateAnglesFromMatrix(0, c);
				if(Res[1] < -0.5f)
					Res[1] = Res[1] + 1.0f;
				_pCD->m_Control_Look = Res;

				_pCD->m_Control_Look_Wanted = Res;
				int iTar = _pCD->m_AimTarget;
				//if(_pWPhysState->IsServer())
			//		ConOutL(CStrF("(Server) Time: %d Look[2]: %f", _pWPhysState->GetGameTick(), _pCD->m_Control_Look[2]) + PlayerPos.GetString() + " " + TargetPos.GetString());
				//if(_pWPhysState->IsClient() && ((CWorld_Client *)_pWPhysState)->GetClientMode() & WCLIENT_MODE_PRIMARY)
				//	ConOutL(CStrF("(Client) Time: %d Look[2]: %f", _pWPhysState->GetGameTick(), _pCD->m_Control_Look[2]) + PlayerPos.GetString() + " " + TargetPos.GetString());
				//		if(_pWPhysState->IsClient() && ((CWorld_Client *)_pWPhysState)->GetClientMode() & WCLIENT_MODE_PRIMARY)
				//	ConOutL(CStrF("CLIENT (%f) Speed %f Dt %f CT: %f LT: %f",_pWPhysState->GetGameTime().GetTime(),SpeedPercent,DeltaTime,CurTime.GetTime(),_pCD->m_AutoAim_LastUpdateTime.GetTime())+_pCD->m_Control_Look_Wanted.GetString());	
				//	else if(_pWPhysState->IsServer())
				//	ConOutL(CStrF("SERVER (%f) Speed %f Dt %f CT: %f LT: %f",_pWPhysState->GetGameTime().GetTime(),SpeedPercent,DeltaTime,CurTime.GetTime(),_pCD->m_AutoAim_LastUpdateTime.GetTime())+_pCD->m_Control_Look_Wanted.GetString());	
				//	
			}
		}
	}*/
}

int CWObject_Character::Char_ProcessControl(const CControlFrame& _Msg, int& _Pos)
{
	MSCOPESHORT(CWObject_Character::Char_ProcessControl);
	MAUTOSTRIP(CWObject_Character_Char_ProcessControl, 0);
	int& Pos = _Pos;
	{
		int SavePos = Pos;

		CWorld_Server* pWServer = m_pWServer;
		int GUIDThis = pWServer->Object_GetGUID(m_iObject);

/*		if (!pWPhysState || !pWPhysState->Object_GetCD(iObjThis))
		{
			ConOutL("Nu är skiten krickad.");
			return 0;
		}*/

		CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
		if (!pCD) return 0;

		CCmd Cmd;
		_Msg.GetCmd(Pos, Cmd);
		int iObj = m_pWServer->Game_GetObjectIndex();
		CWObject_Message Msg(OBJMSG_GAME_CONTROL, pCD->m_iPlayer, m_iObject);

		Msg.m_pData = &Cmd;
		Msg.m_DataSize = sizeof(Cmd);
		if (m_pWServer->Message_SendToObject(Msg, iObj)) 
		{
			if (!pWServer->Object_GetWithGUID(GUIDThis)) return 0;
			return 1;
		}
		
		if (!pWServer->Object_GetWithGUID(GUIDThis)) return 0;

		if (IsDestroyed())
		{
			ConOutL("(CWObject_Character::Char_ProcessControl) Object is destroyed.");
			return 0;
		}

		Pos = SavePos;
/*		if(pCD->m_iMountedObject != 0)
		{
			CWObject_Message Msg(OBJMSG_CHAR_PROCESSCONTROL, m_iObject, _Pos);
			Msg.m_pData = (void *)&_Msg;
			if(m_pWServer->Message_SendToObject(Msg, pCD->m_iMountedObject))
			{
				Pos += Cmd.m_Size + 4;
				return 1;
			}
		}*/

		Pos = SavePos;

		pCD = CWObject_Character::GetClientData(this);
		if (!pCD) return 0;

		int Ctrl;
		int Size;
		int dTime;
//		int ID = _Msg.m_Data[Pos+3];
		_Msg.GetCmd(Pos, Ctrl, Size, dTime);
		if (Pos + Size > _Msg.m_MsgSize) return 1;
		SavePos = Pos;

#ifdef LOG_SERVERAUTHORING_CMD
		ConOutL(CStrF("    SERVER ID %d, Pos %d, CMD %d, dTime %d", ID, Pos, Ctrl, dTime));
#endif

//		ConOutL(CStrF("(CWObject_Character::Char_ProcessImpulse) %d, %d, %d", ID, dTime, Ctrl));
//		ConOut(CStrF("(CWObject_Character::Char_ProcessImpulse) %d, %d, %d, %d", Cmd, d0, d1, d2));

		switch(Ctrl)
		{
			case CONTROL_LOOK:
				if (Size == 6)
				{
					// Break if we're not suppose to look around
					if(Char_CanLook(this))
					{
						CVec3Dfp32 dLook = _Msg.GetVecInt16_Max32768(Pos);
						//				ConOutL("(CWObject_Character::Char_ProcessControl) dLook " + dLook.GetString());
						dLook *= (1.0f / 65536);
						pCD->Char_ProcessControl_Look(dLook);
					}
				}
				break;
			
			case CONTROL_MOVE:
				if (Size == 6)
				{
					CVec3Dfp32 Move = _Msg.GetVecInt16_Max32768(Pos) * (1.0f / 256.0f);
					pCD->Char_ProcessControl_Move(Move);
				}
				else
				{
					pCD->m_MoveDir = 0;
					pCD->m_Control_Move = 0;
				}
				break;

			case CONTROL_ANALOG:
				if (Size == 2)
				{
					int8 Channel = _Msg.GetInt8(Pos);
					uint8 v = _Msg.GetInt8(Pos);
					switch(Channel)
					{
						case 0:
							{
								if (pCD->m_AnalogMode & M_Bit(0))
								{
									pCD->m_Analog0 =  pCD->m_AnalogMode & M_Bit(0) ? Min(v,pCD->m_Analog0.m_Value) : v;
									if (v <= 5)
										pCD->m_AnalogMode = pCD->m_AnalogMode & ~M_Bit(0);
									break;
								}
								pCD->m_Analog0 = v;
								break;
							}
						case 1:
							{
								if (pCD->m_AnalogMode & M_Bit(1))
								{
									pCD->m_Analog1 =  pCD->m_AnalogMode & M_Bit(1) ? Min(v,pCD->m_Analog1.m_Value) : v;
									if (v <= 5)
										pCD->m_AnalogMode = pCD->m_AnalogMode & ~M_Bit(1);
									break;
								}
								pCD->m_Analog1 = v;
								break;
							}
						case 2: pCD->m_Analog2 = v; break;
						case 3: pCD->m_Analog3 = v; break;
						default:
							M_ASSERT(false, "Erroneous channel");	// Look for MAX_ANALOGCHANNELS in WClientP4.h
					}
				}
				break;
#ifdef WADDITIONALCAMERACONTROL
			case CONTROL_LOOKADDITIONAL:
				if (Size == 6)
				{
					MSCOPESHORT(CWObject_Character::Char_ProcessControl__CONTROL_LOOKADDITIONAL);
					CVec3Dfp32 dLook = _Msg.GetVecInt16_Max32768(Pos);
					//				ConOutL("(CWObject_Character::Char_ProcessControl) dLook " + dLook.GetString());
					dLook *= (1.0f / 65536);

					// In noclip mode
					pCD->m_AdditionalCameraControlY = Min(0.249f, Max(-0.249f, pCD->m_AdditionalCameraControlY + dLook[1]));
					pCD->m_AdditionalCameraControlZ = M_FMod((fp32)pCD->m_AdditionalCameraControlZ + dLook[2] + 2.0f, 1.0f);

					//Char_UpdateLook(this, pCD, m_pWServer, pCD->m_PredictFrameFrac);
				}
				break;
			case CONTROL_MOVEADDITIONAL:
				{
					MSCOPESHORT(CWObject_Character::Char_ProcessControl__CONTROL_MOVEADDITIONAL);
					if (Size == 6)
					{
						pCD->m_Control_MoveAdditional = _Msg.GetVecInt16_Max32768(Pos) * (1.0f / 256.0f);
					}
					else
					{
						pCD->m_Control_MoveAdditional = 0.0f;
					}
				}
				break;
			case CONTROL_STATEBITSADDITIONAL:
				if (Size == 4)
				{
					pCD->m_Control_PressAdditional = _Msg.GetInt32(Pos);
					//				m_Released |= ~m_Press;
					pCD->m_Control_ReleasedAdditional |= (~pCD->m_Control_Press) & (~PLAYER_CONTROLBITS_NOAUTORELEASE);
					pCD->m_Control_ReleasedAdditional &= (~pCD->m_Control_Press) | PLAYER_CONTROLBITS_NOAUTORELEASE;
					//				ConOutL(CStrF("Press %.8x", m_Press));
				}
				break;
#endif
			case CONTROL_ADDITEM:
				{
					if (Char_CheatsEnabled())
					{
						m_Flags |= PLAYER_FLAGS_USEDNAWEAPONS;

						char Item[256];
						_Msg.GetData(Item, Size, Pos);
						Item[Size] = 0;

						Char_PickupItem(false, false, Item, m_iObject);
					}
				}
				break;

			case CONTROL_SELECTITEM:
				{
					int32 ItemType = _Msg.GetInt32(Pos);
					int32 ControlMode = Char_GetControlMode(this);
					bool bCanSwitch = !(pCD->m_AnimGraph2.GetStateFlagsLoCombined() & AG2_STATEFLAG_EQUIPPING);
					bCanSwitch = (bCanSwitch && !pCD->m_RelAnimPos.HasQueued(pCD->m_GameTick) && (ControlMode == PLAYER_CONTROLMODE_FREE || ControlMode == PLAYER_CONTROLMODE_CARRYBODY || pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_BLOCKACTIVE));
					if (!bCanSwitch)
						break;
					//ConOut(CStrF("Selecting item: %d",ItemType));
					Char()->SelectItem(ItemType,true,true);
					CRPG_Object_Item* pEquippedItem = Char()->GetFinalSelectedItem(0);
					int32 ItemClass = pCD->m_EquippedItemClass;
					if (pEquippedItem && (ItemClass != pEquippedItem->m_AnimProperty))
					{
						pCD->m_EquippedItemClass = (uint16)pEquippedItem->m_AnimProperty;
						pCD->m_EquippedItemType = (uint16)pEquippedItem->m_iItemType;
						pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_ITEMANIMTYPE,pEquippedItem->m_AnimType);
						
						// Update weapon
						CWAG2I_Context Context(this,m_pWServer,pCD->m_GameTime);
						pCD->m_AnimGraph2.UpdateImpulseState(&Context);
						// Show weapon icon
						pCD->m_Pickup_iIcon = (uint16)pEquippedItem->m_iIconSurface;
						pCD->m_Pickup_StartTick = pCD->m_GameTick;
						pCD->m_Pickup_Magazine_Num = (int8)(Max(-1, Min(127, pEquippedItem->ShowNumMagazines())));
					}
					break;
				}

			case CONTROL_SELECTDIALOGUECHOICE:
				{
					int8 Choice = _Msg.GetInt8(Pos);
					Char_SelectDialogueChoice(Choice);
				}
				break;

			case CONTROL_SELECTTELEPHONECHOICE:
				{
					int8 Choice = _Msg.GetInt8(Pos);
					//Char_SelectTelephoneChoice(Choice);
					m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENE_TELEPHONE_SELECTCHOICE,Choice),(int)pCD->m_ControlMode_Param0);
					break;
				}

			case CONTROL_DIALTELEPHONENUMBER:
				{
					int8 Choice = _Msg.GetInt8(Pos);
				
					char *testchar = (char*)&Cmd.m_Data[0];
					testchar[Size] = 0;
					
					CWObject_Message Msg;
					Msg.m_pData = testchar;
					
					Msg.m_Msg = OBJMSG_TELEPHONE_DIAL;
					Msg.m_Param0 = Choice;
					Msg.m_DataSize = Size;
					
					m_pWServer->Message_SendToObject(Msg, (int)pCD->m_iFocusFrameObject);
				
					break;
				}

#ifndef M_RTM
			case CONTROL_SUMMON:
				{
					if (Char_CheatsEnabled())
					{
						char PlayerClass[256];
						_Msg.GetData(PlayerClass, Size, Pos);
						PlayerClass[Size] = 0;

						CStr Class = CStrF("Player_%s", PlayerClass);

//						m_pWServer->NetMsg_SendToObject(CNetMsg(0), m_iObject);
						CMat4Dfp32 Pos(GetPositionMatrix());
						CVec3Dfp32::GetMatrixRow(Pos, 3).Combine(CVec3Dfp32::GetMatrixRow(Pos, 0), 96, CVec3Dfp32::GetMatrixRow(Pos, 3));
						Pos.k[3][2] += 32;
						int iSpawn = m_pWServer->Object_Create(Class, Pos, m_iObject);
						if (iSpawn > 0)
						{
							//Summon succeded
//								CWObject_Character* pSpawn = (CWObject_Character*)(m_pWServer->Object_Get(iSpawn));
//								if(pSpawn)
//									pSpawn->m_iController = m_iObject;	
						} 
						else
						{
							// Summon without player_ prefix
							int iSpawn = m_pWServer->Object_Create(PlayerClass, Pos, m_iObject);
							if(!iSpawn)
								//Summon failed
								m_pWServer->Net_ConOut(CStrF("Failed to summon %s", GetTemplateName()));
						}
					}
					else
						m_pWServer->Net_ConOut(CStr("Summon is cheat protected."), m_pWServer->Game_GetObject()->Player_GetClient(pCD->m_iPlayer));
				}
				break;

			case CONTROL_ACTIVATEITEM:
				{
					if (CWObject_Character::Char_GetControlMode(this) != PLAYER_CONTROLMODE_LADDER)
					{
						char ItemType[256];
						_Msg.GetData(ItemType, Size, Pos);
						int iItemType = CFStr(ItemType).Val_int();
						ForceActivateItem(iItemType);
					}
				}
				break;

			case CONTROL_SETAPPROACHITEM:
				{
					if (Char_CheatsEnabled())
					{
						char Target[256];
						_Msg.GetData(Target, Size, Pos);
						Target[Size - 1] = 0;
						CStr str = Target;
						CWObject_Message Msg(OBJMSG_CHAR_SETDIALOGUEITEM_APPROACH);
						Msg.m_Param0 = 0;
						CStr Char = str.GetStrSep(":");
						Msg.m_pData = (void *)str.Str();
						m_pWServer->Message_SendToTarget(Msg, Char);
					}
				}
				break;

			case CONTROL_DEBUGIMPULSE:
				{
					if (Char_CheatsEnabled())
					{
						int Impulse = _Msg.GetInt32(Pos);

						char Target[256];
						_Msg.GetData(Target, Size - 4, Pos);
						Target[Size - 4] = 0;

						m_pWServer->Message_SendToTarget(CWObject_Message(OBJMSG_IMPULSE, Impulse, 0, m_iObject), Target);
					}
					else
						m_pWServer->Net_ConOut(CStr("DebugImpulse is cheat protected."), m_pWServer->Game_GetObject()->Player_GetClient(pCD->m_iPlayer));
				}
				break;

#endif
			case CONTROL_DEBUGMSG:
				{
					if (Char_CheatsEnabled())
					{
						char Buf[256];
						_Msg.GetData(Buf, Size, Pos);
						Buf[Size] = 0;
						CWO_SimpleMessage Msg;
						Msg.Parse(Buf, m_pWServer);
						Msg.SendMessage(m_iObject, m_iObject, m_pWServer);
					}
					else
						m_pWServer->Net_ConOut(CStr("DebugMsg is cheat protected."), m_pWServer->Game_GetObject()->Player_GetClient(pCD->m_iPlayer));
				}
				break;

			case CONTROL_STATEBITS:
				if (Size == 4)
				{
					pCD->m_Control_Press = _Msg.GetInt32(Pos);
					pCD->m_Control_Press_Intermediate |= pCD->m_Control_Press;
	//				m_Released |= ~m_Press;
					pCD->m_Control_Released |= (~pCD->m_Control_Press) & (~PLAYER_CONTROLBITS_NOAUTORELEASE);
					pCD->m_Control_Released &= (~pCD->m_Control_Press) | PLAYER_CONTROLBITS_NOAUTORELEASE;
	//				ConOutL(CStrF("Press %.8x", m_Press));
				}
				break;

			case CONTROL_CMD:
				{
					MSCOPESHORT(CWObject_Character::Char_ProcessControl__CONTROL_CMD);

					if (Size < 0) break;
					int Cmd = _Msg.GetInt8(Pos);
					int d0 = (Size > 1) ? _Msg.GetInt8(Pos) : 0;
//					int d1 = (Size > 2) ? _Msg.GetInt8(Pos) : 0;
//					int d2 = (Size > 3) ? _Msg.GetInt8(Pos) : 0;
					if(Size > 2) _Msg.GetInt8(Pos);
					if(Size > 3) _Msg.GetInt8(Pos);


					switch(Cmd)
					{					
						case CMD_NOCLIP2:
							Phys_ToggleNoClip(PLAYER_PHYS_NOCLIP2);
							break;

						case CMD_NOCLIP:
							Phys_ToggleNoClip(PLAYER_PHYS_NOCLIP);
							break;

#ifndef M_RTM
						case CMD_PHYSLOG:
							// PHYSICS LOG
							m_bPhysLog ^= true;
							break;
#endif

						case CMD_GODMODE:
							{
								CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
								if (!pCD) break;

								if (Char_CheatsEnabled())
								{
									uint32 Flags = pCD->m_ExtraFlags;
									if(Flags & PLAYER_EXTRAFLAGS_GODMODE)
									{
										ConOut("GOD MODE is now OFF");
										Flags &= ~PLAYER_EXTRAFLAGS_GODMODE;
									}
									else
									{
										ConOut("GOD MODE is now ON");
										Flags |= PLAYER_EXTRAFLAGS_GODMODE;
									}
									pCD->m_ExtraFlags = Flags;
								}
								else
								{
									CWO_Player* pP = m_pWServer->Game_GetObject()->Player_Get(pCD->m_iPlayer);
									if (pP) m_pWServer->Net_ConOut(CStrF("%s %s", (char*)pP->m_Name, "tried to cheat."));
								}
							}
							break;

						case CMD_TOGGLEDARKLINGHUMAN:
							{
								CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
								if (!pCD) 
									break;

								if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), m_pWServer->Game_GetObjectIndex()))
								{
									CWObject_Message Msg(OBJMSG_GAME_TOGGLEDARKLINGHUMAN);
									Msg.m_iSender = pCD->m_iPlayer;
									m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());
								}
							}
							break;

						case CMD_TOGGLEWEAPONZOOM:
							{
								if(pCD->m_PostAnimSystem.GetOKToWeaponZoom())
									ToggleWeaponZoom();
							}
							break;

						case CMD_DUMPAUTOVARLOG:
							{
#ifdef AUTOVAR_LOG
								DumpAutoVarLog();
#endif
							}
							break;
						case CMD_NOCLIPSPEED:
							{
#ifndef M_RTM
								// This is such a hack... :)
								fp32 CurrentSpeedFactor = m_NoClipSpeedFactor;
								if (d0 == 1)
								{
									m_NoClipSpeedFactor *= 2.0f;
									if (m_NoClipSpeedFactor > 64.0f)
										m_NoClipSpeedFactor = 64.0f;
								}
								else if (d0 == -1)
								{
									m_NoClipSpeedFactor /= 2.0f;
									if (m_NoClipSpeedFactor < 1.0f /64.0f)
										m_NoClipSpeedFactor = 1.0f /64.0f;
								}
								else
								{
									m_NoClipSpeedFactor = 1.0f;
								}
								// Set move scale if in noclip
								if (Char_GetPhysType(this) == PLAYER_PHYS_NOCLIP)
								{
									pCD->m_ControlMove_Scale = pCD->m_ControlMove_Scale * m_NoClipSpeedFactor / CurrentSpeedFactor;
								}
#endif
							}
							break;

						case CMD_GIVEALL:
							{
								CStr St = m_Player.m_CheatItems;
								while(St != "")
								{
									CStr Item = St.GetStrSep(",");
									Char_PickupItem(false, false, Item, m_iObject);
								}
								pCD->m_Disable = 0;
								
								// Don't give max level
								//pCD->m_DarknessDevour = 100;			// Set 100 devoured corpses
								//pCD->m_MaxDarkness = 100;				// Set max darkness

								pCD->m_Darkness = pCD->m_MaxDarkness;	// Fill up darkness max
								pCD->m_DarknessPowersAvailable = pCD->m_DarknessPowersAvailable | PLAYER_DARKNESSMODE_POWERMASK;
								pCD->m_DarknessPowers = pCD->m_DarknessPowersAvailable;
																
								// Find all darkling spawn points and add all the darklings in them to the player
/*								TSelection<CSelection::LARGE_BUFFER> Selection;
								m_pWServer->Selection_AddClass(Selection,"darklingspawn");
								const int16* pList = NULL;
								int NumSel = m_pWServer->Selection_Get(Selection,&pList);
								for (int32 i = 0; i < NumSel; i++)
								{
									CWObject_DarklingSpawn* pSpawn = safe_cast<CWObject_DarklingSpawn>(m_pWServer->Object_Get(pList[i]));
									if (pSpawn)
										Char_AddDarkling(pSpawn->GetDarklingSpawnType());

								}*/

								// Give ancient weapons to player, (if we not already have them)
								CRPG_Object_Item* pAncient = Char()->FindItemByType(RPG_ITEMTYPE_ANCIENTWEAPONS);
								if (!pAncient)
								{
									Char_PickupItem(false, false, "weapon_Ancient_1", m_iObject);
									Char_PickupItem(false, false, "weapon_Ancient_2", m_iObject);
								}

								Char_AddDarkling("ai_darkling_berserker");
								Char_AddDarkling("ai_darkling_gunner");
								Char_AddDarkling("ai_darkling_lightkiller");
								Char_AddDarkling("ai_darkling_kamikaze");
								ConOut("Give All!");
							}
							break;

						case CMD_GIVE_DARKNESSLEVEL1:
						case CMD_GIVE_DARKNESSLEVEL2:
						case CMD_GIVE_DARKNESSLEVEL3:
						case CMD_GIVE_DARKNESSLEVEL4:
						case CMD_GIVE_DARKNESSLEVEL5:
							{
								int Level = Cmd - CMD_GIVE_DARKNESSLEVEL1;
								pCD->m_DarknessLevel.SetLevel(Level);
								pCD->m_MaxDarkness = pCD->m_DarknessLevel.GetMaxDarkness();
								pCD->m_Darkness = pCD->m_MaxDarkness;

								ConOut(CStrF("Darkness level %d", Level + 1));
							}
							break;

						case CMD_GIVE_DEMONARMLEVEL1:
						case CMD_GIVE_DEMONARMLEVEL2:
							{
								uint DemonArmLevel = Cmd - CMD_GIVE_DEMONARMLEVEL1;
								if (DemonArmLevel)
								{
									ConOut("Setting strong demon arm");
									pCD->m_DarknessLevel.SetPowerLevel(PLAYER_DARKNESSMODE_POWER_DEMONARM, 1);
								}
								else
								{
									ConOut("Setting weak demon arm");
									pCD->m_DarknessLevel.SetPowerLevel(PLAYER_DARKNESSMODE_POWER_DEMONARM, 0);
								}
							}
							break;

						case CMD_NEXTWEAPON:
							if (!(Char_IsSwimming() || Char_IsClimbing()))
								NextItem(RPG_CHAR_INVENTORY_WEAPONS, false);
							break;

						case CMD_PREVWEAPON:
							if (!(Char_IsSwimming() || Char_IsClimbing()))
								NextItem(RPG_CHAR_INVENTORY_WEAPONS, true);
							break;

						case CMD_NEXTITEM:
							if (!(Char_IsSwimming() || Char_IsClimbing()))
								NextItem(RPG_CHAR_INVENTORY_ITEMS, false);
							break;

						case CMD_PREVITEM:
							if (!(Char_IsSwimming() || Char_IsClimbing()))
								NextItem(RPG_CHAR_INVENTORY_ITEMS, true);
							break;
							
/*						case CMD_NEXTDIALOGUEITEM:
							Char_NextDialogueItem(1);
							break;

						case CMD_PREVDIALOGUEITEM:
							Char_NextDialogueItem(0);
							break;
							
						case CMD_SELECTDIALOGUEITEM:
							Char_SelectDialogueItem();
							break;*/

						case CMD_ENDDIALOGUE:
							if(m_ClientFlags & PLAYER_CLIENTFLAGS_DIALOGUE)
								Char_EndDialogue();
							break;

						case CMD_CLOSECLIENTWINDOW:
							{
								CWObject_Message Msg(OBJMSG_GAME_SETCLIENTWINDOW);
								Msg.m_iSender = m_iObject;
								m_pWServer->Message_SendToTarget(Msg, (char*)WSERVER_GAMEOBJNAME);

								break;
							}
						case CMD_SELFSUMMON:
							// Summon a bot of characters own type, with character as owner.
							// Character can command any summoned bots by issuing the bot 
							// commands (102 and upwards a bit). 
							m_pWServer->NetMsg_SendToObject(CNetMsg(0), m_iObject);

							if (Char_CheatsEnabled())
							{
								CMat4Dfp32 Pos(GetPositionMatrix());
								CVec3Dfp32::GetMatrixRow(Pos, 3).Combine(CVec3Dfp32::GetMatrixRow(Pos, 0), 96, CVec3Dfp32::GetMatrixRow(Pos, 3));
								Pos.k[3][2] += 32;
								int iSpawn = m_pWServer->Object_Create(GetTemplateName(), Pos, m_iObject);
								if (iSpawn)
								{
									//Summon succeded
									m_lControlledBots.Add(iSpawn);

									CWObject_Character* pSpawn = (CWObject_Character*)(m_pWServer->Object_Get(iSpawn));
									if(pSpawn)
									{
										pSpawn->m_iController = m_iObject;	
									};
								} 
								else
									//Summon failed
									m_pWServer->Net_ConOut(CStrF("Failed to summon %s", GetTemplateName()));

							}
							else
								m_pWServer->Net_ConOut(CStr("Summon is cheat protected."), m_pWServer->Game_GetObject()->Player_GetClient(pCD->m_iPlayer));
							break;
						case CMD_KILL:
							if (!(Char_GetPhysType(this) == PLAYER_PHYS_NOCLIP) && 
								!(m_ClientFlags & PLAYER_CLIENTFLAGS_SPECTATOR))
							{
								CWO_DamageMsg Msg(10000);
								Physics_Damage(Msg, 0);
							}
							break;

						case CMD_SKIPINGAMECUTSCENE:
							Char_SkipCutscene();
							break;
						case CMD_TOGGLENIGHTVISION:
							if(!(m_Flags & PLAYER_FLAGS_NONIGHTVISION))
							{
								// Don't toggle nightvision when fighting
								if (Char_GetControlMode(this) != PLAYER_CONTROLMODE_FIGHTING)
								{
									// Toggle nightvision
									uint8 bNightvisionEnabled = pCD->m_bNightvisionEnabled;

									pCD->m_bNightvisionEnabled = !bNightvisionEnabled;
									/*if(pCD->m_bNightvisionEnabled)
										m_pWServer->Sound_Global(m_pWServer->GetMapData()->GetResourceIndex_Sound("gui_night_on"));
									else
										m_pWServer->Sound_Global(m_pWServer->GetMapData()->GetResourceIndex_Sound("gui_night_off"));*/
								}
							}

							break;
						case CMD_TOGGLEAIMASSISTANCE:
							m_Flags ^= PLAYER_FLAGS_NOAIMASSISTANCE;
							break;
						case CMD_HELPBUTTON:
							if(Char_IsPlayerView(this) && m_PendingCutsceneTick == -1 && m_Player.m_LastInfoscreenPress < int(m_pWServer->GetGameTick()) - 5)
							{
								// Never allow to enter many screens at once
								if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_SHOWPENDINGGAMEMSG), m_pWServer->Game_GetObjectIndex()))
									m_Player.m_LastInfoscreenPress = m_pWServer->GetGameTick() + 10; // 6 ticks before pause
							}
							break;
						case CMD_CYCLECAMERA:
							{
								if(pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_FORCETHIRPERSON)
									pCD->m_ExtraFlags = pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_FORCETHIRPERSON;
								else
									pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_FORCETHIRPERSON;
							}
							break;

#ifndef M_RTM
						case CMD_SETAGDEBUGFLAGS:
							{
								//Change how animgraph debug info (state transitions etc) is printed 
								MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
								CRegistry* pReg = pSys ? pSys->GetEnvironment() : NULL;
								if (pReg)
								{
									static const int s_lFlagCombos[6] = 
									{
										//All
										AG2I_DEBUGFLAGS_ENTERSTATE_SERVER | AG2I_DEBUGFLAGS_ENTERSTATE_CLIENT | AG2I_DEBUGFLAGS_ENTERSTATE_PLAYER | AG2I_DEBUGFLAGS_ENTERSTATE_NONPLAYERS,
										//Player on both server and client
										AG2I_DEBUGFLAGS_ENTERSTATE_SERVER | AG2I_DEBUGFLAGS_ENTERSTATE_CLIENT | AG2I_DEBUGFLAGS_ENTERSTATE_PLAYER,
										//Player on server
										AG2I_DEBUGFLAGS_ENTERSTATE_SERVER | AG2I_DEBUGFLAGS_ENTERSTATE_PLAYER, 
										//Player on client
										AG2I_DEBUGFLAGS_ENTERSTATE_CLIENT | AG2I_DEBUGFLAGS_ENTERSTATE_PLAYER, 
										//Bots on server
										AG2I_DEBUGFLAGS_ENTERSTATE_SERVER | AG2I_DEBUGFLAGS_ENTERSTATE_NONPLAYERS, 
										//Nothing
										0,
									};
									static const char * s_lDesc[6] = {"EVERYONE on SERVER/CLIENT","PLAYER on SERVER/CLIENT","PLAYER on SERVER","PLAYER on CLIENT","BOTS on SERVER","NOTHING"};
									ConOutL(CStrF("Displaying animgraph debug info for %s", s_lDesc[d0]));
									pReg->SetValuei("AGI_DEBUG_FLAGS", s_lFlagCombos[d0]);
									if (m_pWServer && m_pWServer->Registry_GetServer())
										m_pWServer->Registry_GetServer()->SetValuei("AGI_DEBUG_FLAGS", s_lFlagCombos[d0]);
								}
							}
							break;

						case CMD_PATHTEST :
							{
								ConOut("PathTest...");

								CXR_BlockNavSearcher* pNav = m_pWServer->Path_GetBlockNavSearcher();
								if (!pNav) break;

								if (m_iSearch)
								{
									pNav->Search_Destroy(m_iSearch);
									m_iSearch = NULL;
								}

								CMat4Dfp32 m;
								GetActivatePosition(m);

								CCollisionInfo Info;
								bool Res = m_pWServer->Phys_IntersectLine(CVec3Dfp32::GetMatrixRow(m, 3), 
																		  CVec3Dfp32::GetMatrixRow(m, 3) + CVec3Dfp32::GetMatrixRow(m, 0) * 1000,
																		  0,
																		  OBJECT_FLAGS_PHYSMODEL,
																		  XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID,
																		  &Info);
								if(Res && Info.m_bIsValid)
								{
									CVec3Dfp32 Pos = Info.m_Pos + CVec3Dfp32(0,0,8);
									ConOut("Hit " + Info.m_Pos.GetString());
									m_pWServer->Object_Create("CoordinateSystem", Info.m_Pos);
									CXBN_SearchParams Params(GetPosition() + CVec3Dfp32(0,0,4), 
															 Pos, 
															 CVec3Dint16(24, 24, 32),
															 48);

									m_iSearch = pNav->Search_Create(Params);
									if (!m_iSearch)
										ConOut("Could not create search instance.");
									else
										ConOut(CStrF("New search instance: %d", m_iSearch));


								}
								else
									ConOut("NoHit");
							}
							break;

						//////////////////////////////////////////////////////////////////////
						// The following are commands that allow the player to control 
						// bots. Issuing a command will toggle the corresponding bot control 
						// flag.
						//////////////////////////////////////////////////////////////////////
						case CMD_GIVEORDER: //Give conrolled bots an order
							{
								GiveOrder(d0);
							};
							break;
						case CMD_SETPATHDESTINATION://Sets bots' path destination, and makes all bots try to go there
							{	
								//Intersect the sight line with any solids to find path destination
								CMat4Dfp32 m;
								GetActivatePosition(m);
								CCollisionInfo Info;
								bool Res = m_pWServer->Phys_IntersectLine(CVec3Dfp32::GetMatrixRow(m, 3), 
																		  CVec3Dfp32::GetMatrixRow(m, 3) + CVec3Dfp32::GetMatrixRow(m, 0) * 1000,
																		  0,
																		  OBJECT_FLAGS_PHYSMODEL,
																		  XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID,
																		  &Info);
								if(Res && Info.m_bIsValid)
								{
									//Successful intersection, set path destination
									m_spAI->SetPathDestination(Info.m_Pos + CVec3Dfp32(0,0,4));

									//Set all bots to follow the path
									CWObject_Character * pBot;
									for (int i = 0; i < m_lControlledBots.Len(); i++)
									{	
										if ((pBot = IsCharacter(m_pWServer->Object_Get(m_lControlledBots[i]))))
											pBot->m_iBotControl |= CAI_Core::BOT_FOLLOW_PATH;
									};

									//Feedback...
									ConOutL("Hit " + Info.m_Pos.GetString());
									m_pWServer->Object_Create("CoordinateSystem", Info.m_Pos);
								}					
								else
									ConOutL("NoHit");
							};
							break;
						case CMD_PAUSEBOT://Get control of bot by aiming at it
							{	
								//Intersect the sight line with any characters to find target
								CMat4Dfp32 m;
								GetActivatePosition(m);
								CCollisionInfo Info;
								bool Res = m_pWServer->Phys_IntersectLine(CVec3Dfp32::GetMatrixRow(m, 3), 
																		  CVec3Dfp32::GetMatrixRow(m, 3) + CVec3Dfp32::GetMatrixRow(m, 0) * 1000,
																		  0,
																		  OBJECT_FLAGS_CHARACTER,
																		  0,
																		  &Info,
																		  m_iObject);
								if(Res && Info.m_bIsValid)
								{
									//Successful intersection, check if bot 
									CWObject_Character * pBot = IsCharacter(m_pWServer->Object_Get(Info.m_iObject));
									if ( pBot->IsBot())
									{
										pBot->m_iController = m_iObject;
										m_lControlledBots.Add((int)Info.m_iObject);
										
										//Feedback...
//										ConOutL(RPG_GetMouseOverIdentification(0) + " is taking control of " + pBot->RPG_GetMouseOverIdentification(0));
									}
									else 
										ConOutL("Can't take control of that");
								}					
								else
									ConOutL("NoHit");
							};
							break;
						case CMD_RELEASEALLBOTS://Release all bots we control
							{	
								CWObject_Character * pBot;
								for (int i = 0; i < m_lControlledBots.Len(); i++)
								{	
									if ((pBot = IsCharacter(m_pWServer->Object_Get(m_lControlledBots[i]))))
										pBot->m_iController = NULL;
								};

								//Reset all bots orders and clear list of controlled bots
								GiveOrder(NULL);
								m_lControlledBots.Clear();

								ConOutL("Emancipation! Equal rights for bots!");
							};
							break;
						case CMD_AIDEBUG: //Give debug command
							{
								m_spAI->SetDebugMsg(d0);
							};
							break;
						case CMD_SETTESTCOORDINATES: //Give coordinate for test stuff
							{
								//Intersect the sight line with any solids to find coordinate
								CMat4Dfp32 m;
								GetActivatePosition(m);
								CCollisionInfo Info;
								bool Res = m_pWServer->Phys_IntersectLine(CVec3Dfp32::GetMatrixRow(m, 3), 
																		  CVec3Dfp32::GetMatrixRow(m, 3) + CVec3Dfp32::GetMatrixRow(m, 0) * 1000,
																		  0,
																		  OBJECT_FLAGS_PHYSMODEL,
																		  XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID,
																		  &Info);
								if(Res && Info.m_bIsValid)
								{
									//Successful intersection, set coordinate of all controlled bots
									CWObject_Character * pBot;
									for (int i = 0; i < m_lControlledBots.Len(); i++)
									{	
										if ((pBot = IsCharacter(m_pWServer->Object_Get(m_lControlledBots[i]))))
											pBot->m_spAI->BotCoord = Info.m_Pos;
									};

									//Feedback...
									ConOutL("Hit " + Info.m_Pos.GetString());
									m_pWServer->Object_Create("CoordinateSystem", Info.m_Pos);
								}					
								else
									ConOutL("NoHit");
							};
							break;

						case CMD_DEBUG_RENDERTEST:
							{
							}
							break;

						case CMD_DEBUG_FOV:
							{
								fp32 f = pCD->m_BaseFov;
								f += 5;
								if (f > 120)
									f = 50;

								pCD->m_BaseFov = f;
								pCD->m_TargetFov = f;

								ConOut(CStrF("New base FOV is: %.1f", f));
							}
							break;

						case CMD_DEBUG_SCALE:
							{
								fp32 s = pCD->m_CharGlobalScale;
								s += 0.10f;
								if (s > 1.31f)
									s = 0.70f;
								pCD->m_CharGlobalScale = s;

								ConOut(CStrF("New character scale is: %.2f", s));
							}
							break;

						case 125 : 
							{
								if(d0)
									m_Flags |= PLAYER_FLAGS_NONIGHTVISION;
								else
									m_Flags &= ~PLAYER_FLAGS_NONIGHTVISION;
							}
							break;

						case 126 : 
							{
								if(d0)
									m_Flags |= PLAYER_FLAGS_USEDNAWEAPONS;
								else
									m_Flags &= ~PLAYER_FLAGS_USEDNAWEAPONS;
							}
							break;
						////////////////////////////////////////////////////////////////////////
#endif //!M_RTM
						case 127 :
							{
//								int a = 1;
							}
							break;

						case -1 :
							{
								// Stepping in freezetime
							}
							break;

						default:
							ConOut(CStrF("(CWObject_Character::Char_ProcessControl) Unknown CONTROL_CMD: %d", Cmd));
							break;
					}
				}
				break;

			default:
				ConOut(CStrF("(CWObject_Character::Char_ProcessControl) Unknown control: %d", Ctrl));
				break;
		}


		Pos = SavePos + Size;
	}
	return 1;
}

void CWObject_Character::ToggleWeaponZoom()
{
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if(!pCD)
		return;

	fp32 ZoomVal = 0;
	int TicksIntoTransition = 0;
	int TotalTransitionTicks = 0;
	int ModTicks = 0;

	switch(pCD->m_WeaponZoomState)
	{
	case WEAPON_ZOOMSTATE_ON:
		pCD->m_WeaponZoomState = WEAPON_ZOOMSTATE_OUT;
		pCD->m_WeaponZoomStartTick = m_pWServer->GetGameTick();
		break;

	case WEAPON_ZOOMSTATE_IN:
		pCD->m_WeaponZoomState = WEAPON_ZOOMSTATE_OUT;
		TicksIntoTransition = (m_pWServer->GetGameTick() - pCD->m_WeaponZoomStartTick);
		TotalTransitionTicks = TruncToInt(m_pWServer->GetGameTicksPerSecond() * 0.1f); 
		ModTicks = TotalTransitionTicks - TicksIntoTransition;
		pCD->m_WeaponZoomStartTick = m_pWServer->GetGameTick() - ModTicks;
		break;

	case WEAPON_ZOOMSTATE_OFF:
		pCD->m_WeaponZoomState = WEAPON_ZOOMSTATE_IN;
		pCD->m_WeaponZoomStartTick = m_pWServer->GetGameTick();
		break;

	case WEAPON_ZOOMSTATE_OUT:
		pCD->m_WeaponZoomState = WEAPON_ZOOMSTATE_IN;
		TicksIntoTransition = (m_pWServer->GetGameTick() - pCD->m_WeaponZoomStartTick);
		TotalTransitionTicks = TruncToInt(m_pWServer->GetGameTicksPerSecond() * 0.1f); 
		ModTicks = TotalTransitionTicks - TicksIntoTransition;
		pCD->m_WeaponZoomStartTick = m_pWServer->GetGameTick() - ModTicks;
		break;

	default:
		break;
	}
}
