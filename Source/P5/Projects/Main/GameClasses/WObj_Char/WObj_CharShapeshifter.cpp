/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			Character class for shapeshifter(multiplayer) characters.

Author:			Roger Mattsson

Copyright:		2005 Starbreeze Studios AB

Contents:		CWObject_CharShapeshifter

Comments:

History:		
051117:		Created file
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_CharShapeshifter.h"
#include "../wrpg/WRPGChar.h"
#include "../wrpg/WRPGItem.h"
#include "../wrpg/WRPGFist.h"
#include "../WObj_Game/WObj_GameMod.h"
#include "../../GameWorld/WClientMod_Defines.h"

#include "../CConstraintSystem.h"

//#pragma optimize("", off)
//#pragma inline_depth(0)

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_CharShapeshifter, CWObject_CharDarkling, 0x0100);

CWObject_CharShapeshifter::CWObject_CharShapeshifter()
{
	for(int i = 0; i < 2; i++)
	{
		m_PhysWidth[i] = 0;
		m_PhysHeight[i] = 0;
		m_PhysHeightCrouched[i] = 0;
		m_DeadRadius[i] = 0;
		m_PhysStepsize[i] = 0.0f;
		m_PhysFriction[i] = 0.0f;
		m_SpeedForward[i] = 0.0f;
		m_SpeedSidestep[i] = 0.0f;
		m_SpeedUp[i] = 0.0f;
		m_SpeedJump[i] = 0.0f;
	}

	m_Fov[0] = 70;
	m_Fov[1] = 115;

	m_SelfRemoveTick = 0;
	m_SelfRemoveFlags = 0;
	m_WeaponAtShapeshift = 0;
}


const CWO_CharShapeshifter_ClientData& CWObject_CharShapeshifter::GetClientData(const CWObject_CoreData* _pObj)
{
	M_ASSERT(_pObj, "[CWO_CharShapeshifter_ClientData] Bad this-pointer!");
	const CWO_CharShapeshifter_ClientData* pCD = safe_cast<const CWO_CharShapeshifter_ClientData>((const CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	M_ASSERT(pCD, "[CWO_CharShapeshifter_ClientData] No clientdata?!");
	return *pCD;
}


CWO_CharShapeshifter_ClientData& CWObject_CharShapeshifter::GetClientData(CWObject_CoreData* _pObj)
{
	M_ASSERT(_pObj, "[CWO_CharShapeshifter_ClientData] Bad this-pointer!");
	CWO_CharShapeshifter_ClientData* pCD = safe_cast<CWO_CharShapeshifter_ClientData>((CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	M_ASSERT(pCD, "[CWO_CharShapeshifter_ClientData] No clientdata?!");
	return *pCD;
}

void CWObject_CharShapeshifter::OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	// Check if we need to (re)allocate client data
	CReferenceCount* pData = _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA];
	CWO_CharShapeshifter_ClientData* pCD = TDynamicCast<CWO_CharShapeshifter_ClientData>(pData);

	if (!pCD || pCD->m_pObj != _pObj || pCD->m_pWPhysState != _pWPhysState)
	{
		pCD = MNew(CWO_CharShapeshifter_ClientData);
		if (!pCD)
			Error_static("CWObject_CharShapeshifter", "Could not allocate client data!");

		_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA] = pCD;
		pCD->Clear(_pObj, _pWPhysState);
	}

	if (!InitClientObjects(_pObj, _pWPhysState))
		Error_static("CWObject_CharShapeshifter", "InitClientObjects failed");
}

void CWObject_CharShapeshifter::OnInitInstance(const aint* _pParam, int _nParam)
{
	CWO_CharShapeshifter_ClientData& CD = GetClientData();
	if (_pParam && _nParam > 0)
		CD.m_iPlayer = (int16)_pParam[0];
}

bool CWObject_CharShapeshifter::ToggleDarklingHuman(bool _bForce, bool _bSpawnShape)
{
	CWO_CharShapeshifter_ClientData& CD = GetClientData();
	
	if(CD.m_LastTransformTick + RoundToInt(CD.m_pWPhysState->GetGameTicksPerSecond() * 1.0f) > CD.m_pWPhysState->GetGameTick() && !_bForce)
		return false;

	// Spawn a dying shape of last type (maybe some more effects on it to hide popping of darkling/human model?)
	if (_bSpawnShape)
	{
		// Effect
		//m_pWServer->Object_Create("Shapeshift_SmokeBlast", GetPositionMatrix(), m_iObject);

		int iObjDup = m_pWServer->Object_Create(GetTemplateName(), GetPositionMatrix());
		CWObject_CharShapeshifter* pObjDup = safe_cast<CWObject_CharShapeshifter>(m_pWServer->Object_Get(iObjDup));
		if (pObjDup)
		{
			pObjDup->m_SelfRemoveFlags = SHAPESHIFTER_REMOVE_FLAGS_REMOVE;

			// Copy client data
			CWO_CharShapeshifter_ClientData& CDDup = CWObject_CharShapeshifter::GetClientData(pObjDup);
			if (!CD.m_IsHuman)
			{
				pObjDup->ToggleDarklingHuman(true, false);
				pObjDup->iModel(0) = m_iModel[0];
				pObjDup->iModel(1) = m_iModel[1];
				pObjDup->iModel(2) = m_iModel[2];

				// No ragdoll mode, just flag for removal
				pObjDup->m_SelfRemoveTick = m_pWServer->GetGameTick() + TruncToInt(m_pWServer->GetGameTicksPerSecond() * 2.0f);
			}
			else
			{
				// Make sure ragdoll finish before trying to remove object
				pObjDup->m_SelfRemoveFlags |= SHAPESHIFTER_REMOVE_FLAGS_CHECK;
			}

			// Copy client data and reset some of the values
			CDDup.Copy(CD);
			CDDup.m_LastTransformTick = CD.m_LastTransformTick;
			CDDup.m_iPlayer = -1;
			
			// Set position, and phys flags
			CDDup.m_Phys_Flags = CDDup.m_Phys_Flags | PLAYER_PHYSFLAGS_NOPROJECTILECOLL | PLAYER_PHYSFLAGS_NOCHARACTERCOLL | PLAYER_PHYSFLAGS_NOANIMPHYS;
			m_pWServer->Object_SetPositionNoIntersection(iObjDup, GetPositionMatrix());
						
			// Kill duplicate so it will either blow up for darkling type or enter ragdoll mode for humans
			CWO_Damage Damage;
			Damage.m_DamageType = DAMAGETYPE_UNDEFINED;
			Damage.m_DeliverDelay = 0;
			Damage.m_Damage = Char()->Health().m_Value;
			Damage.Send(iObjDup, 0, m_pWServer, GetPosition(), NULL);
		}		
	}

	CD.m_IsHuman = !CD.m_IsHuman;
	CD.m_IsHuman.MakeDirty();

	CD.m_LastTransformTick = CD.m_pWPhysState->GetGameTick();

	if(CD.m_IsHuman)
	{	//Set model to human
		m_Flags |= PLAYER_FLAGS_RAGDOLL;
		Data(RPG_DATAINDEX_DIALOGUEID) = m_Dialogues[0];

		m_pWServer->Net_ClientCommand(CD.m_iPlayer, "cl_darknessvision(0)");

		CD.m_Aim_SkeletonType = 1;
		CD.m_Aim_SkeletonType.MakeDirty();

		iModel(0) = CD.m_iHumanModel[0];
		iModel(1) = CD.m_iHumanModel[1];
		iModel(2) = CD.m_iHumanModel[2];

		CD.m_TargetFov = m_Fov[0];
		CD.m_FovTimeScale = 100.0f;
		CD.m_FovTime = m_pWServer->GetGameTick() - 1;

		CD.m_Phys_Width = m_PhysWidth[0];
		CD.m_Phys_Height = m_PhysHeight[0];
		CD.m_Phys_HeightCrouch = m_PhysHeightCrouched[0];
		CD.m_Phys_DeadRadius = m_DeadRadius[0];
		m_PhysAttrib.m_Friction = m_PhysFriction[0];
		m_PhysAttrib.m_StepSize = m_PhysStepsize[0];
		CD.m_Speed_Forward = m_SpeedForward[0];
		CD.m_Speed_SideStep = m_SpeedSidestep[0];
		CD.m_Speed_Up = m_SpeedUp[0];
		CD.m_Speed_Jump = m_SpeedJump[0];
		if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_SPEED)
		{
			fp32 speed = CD.m_SpeedBoostMultiplier;
			CD.m_Speed_Forward.m_Value *= speed;
			CD.m_Speed_SideStep.m_Value *= speed;
			CD.m_Speed_Up.m_Value *= speed;
			CD.m_Speed_Jump.m_Value *= speed;
		}
		CD.m_Phys_DeadRadius.MakeDirty();

		Char_SetPhysics(this, m_pWServer, m_pWServer, Char_GetPhysType(this), false, true);

		CWAG2I_Context Context(this, CD.m_pWPhysState, CD.m_GameTime);
		CD.m_AnimGraph2.GetAG2I()->RestartAG(&Context, 0);

		if(!m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_CANDARKLINGSUSEWEAPONS), m_pWServer->Game_GetObjectIndex()))
		{
			CRPG_Object_Inventory* pWeaponInv = Char()->GetInventory(AG2_ITEMSLOT_WEAPONS);
			int32 iCurEquipped = pWeaponInv->GetFinalSelectedItemIndex();
			CRPG_Object_Item* pCurItem = pWeaponInv->FindFinalSelectedItem();//Char()->GetFinalSelectedItem(AG2_ITEMSLOT_WEAPONS);

			if(CD.m_SelectItemTimeOutTick == 0 && pCurItem && pCurItem->m_iItemType == 30)
			{
				CRPG_Object_Item* pNextItem = NULL;
				pNextItem = pWeaponInv->GetItemByIndex(m_WeaponAtShapeshift);
				if(!pNextItem)
				{	//This happens when the weapon we had was removed because it was out of ammo, equip any weapon we have
					for(int i = 0; i < pWeaponInv->GetNumItems(); i++)
					{
						CRPG_Object_Item* pWeapon = pWeaponInv->GetItemByIndex(i);
						CFStr name = pWeapon->GetItemName();
						if(name.Find("_DARKLING") == -1)
						{
							pNextItem = pWeapon;
							break;
						}
					}
				}
				if (pNextItem)
					SelectItemByIdentifier(pNextItem->m_iItemType >> 8,pNextItem->m_Identifier, true);
				CD.m_WeaponUnequipTick = 0;
			}

			CD.m_Disable = CD.m_Disable & ~PLAYER_DISABLE_SWITCHWEAPON;
			CD.m_Disable.MakeDirty();

			int NumGuns = 0;
			CMat4Dfp32 Mat = GetPositionMatrix();
			for(int i = 0; i < pWeaponInv->GetNumItems(); i++)
			{
				CRPG_Object_Item* pWeapon = pWeaponInv->GetItemByIndex(i);
				pWeapon->m_Flags2 &= ~RPG_ITEM_FLAGS2_TAGGEDFORRELOAD;
				// Deactivate (trigger might have been pressed mid change)
				pWeapon->Deactivate(Mat,Char(),m_iObject,0);
				if(NumGuns >= 2 && pWeapon->m_iItemType == 1)
					pWeapon->m_Flags2 |= RPG_ITEM_FLAGS2_THROWAWAYONEMPTY;	//Now we can throw them
				if(pWeapon->m_iItemType == 1)
					NumGuns++;
			}

			//Disable all darkling weapons, enable the rest
			for(int i = 0; i < pWeaponInv->GetNumItems(); i++)
			{
				CRPG_Object_Item* pWeapon = pWeaponInv->GetItemByIndex(i);
				CFStr name = pWeapon->GetItemName();
				if(name.Find("_DARKLING") != -1)
					pWeapon->m_Flags2 = pWeapon->m_Flags2 | RPG_ITEM_FLAGS2_NOTSELECTABLE;
				else
					pWeapon->m_Flags2 = pWeapon->m_Flags2 & ~RPG_ITEM_FLAGS2_NOTSELECTABLE;
			}
			CD.m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED,false);
			CD.m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUEDDUALWIELD,false);
		}
	}
	else
	{	//set model to darkling
		m_Flags &= ~PLAYER_FLAGS_RAGDOLL;
		Data(RPG_DATAINDEX_DIALOGUEID) = m_Dialogues[1];

		m_pWServer->Net_ClientCommand(CD.m_iPlayer, "cl_darknessvision(1)");

		CD.m_Aim_SkeletonType = 4;
		CD.m_Aim_SkeletonType.MakeDirty();

		iModel(0) = CD.m_iDarklingModel;
		iModel(1) = -1;
		iModel(2) = -1;

		CD.m_TargetFov = m_Fov[1];
		CD.m_FovTimeScale = 100.0f;
		CD.m_FovTime = m_pWServer->GetGameTick() - 1;

		CD.m_Phys_Width = m_PhysWidth[1];
		CD.m_Phys_Height = m_PhysHeight[1];
		CD.m_Phys_HeightCrouch = m_PhysHeightCrouched[1];
		CD.m_Phys_DeadRadius = m_DeadRadius[1];
		m_PhysAttrib.m_Friction = m_PhysFriction[1];
		m_PhysAttrib.m_StepSize = m_PhysStepsize[1];
		CD.m_Speed_Forward = m_SpeedForward[1];
		CD.m_Speed_SideStep = m_SpeedSidestep[1];
		CD.m_Speed_Up = m_SpeedUp[1];
		CD.m_Speed_Jump = m_SpeedJump[1];
		if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_SPEED)
		{
			fp32 speed = CD.m_SpeedBoostMultiplier;
			CD.m_Speed_Forward.m_Value *= speed;
			CD.m_Speed_SideStep.m_Value *= speed;
			CD.m_Speed_Up.m_Value *= speed;
			CD.m_Speed_Jump.m_Value *= speed;
		}
		CD.m_Phys_DeadRadius.MakeDirty();

		Char_SetPhysics(this, m_pWServer, m_pWServer, Char_GetPhysType(this), false, true);
		
		CWAG2I_Context Context(this, CD.m_pWPhysState, CD.m_GameTime);
		CD.m_AnimGraph2.GetAG2I()->RestartAG(&Context, 1);

		CRPG_Object_Inventory* pWeaponInv = Char()->GetInventory(AG2_ITEMSLOT_WEAPONS);
		m_WeaponAtShapeshift = pWeaponInv->GetFinalSelectedItemIndex();

//		CMat4Dfp32 Pos = m_pWServer->Object_GetPositionMatrix(m_iObject);
//		CVec3Dfp32 Gravity = CD.m_Gravity;
//		Pos.GetRow(3) += (-Gravity) * 20.0f;
/*		if(!m_pWServer->Object_SetPosition_World(m_iObject, Pos))
		{
			CWObject *pObj = m_pWServer->Object_Get(m_iObject);
			CMat4Dfp32 LastPos = pObj->GetLastPositionMatrix();

			LastPos.GetRow(3) += (-Gravity) * 20.0f;
			if(!m_pWServer->Object_SetPosition_World(m_iObject, LastPos))
			{
				M_TRACEALWAYS("Failed to set position twice, forcing it\n");
				m_pWServer->Object_SetPositionNoIntersection(m_iObject, Pos);
			}
		}*/

		CMat4Dfp32 Pos = GetPositionMatrix();
		Pos.k[3][2] += 16.0f;
		m_pWServer->Object_SetPositionNoIntersection(m_iObject, Pos);
		
		if(!m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_CANDARKLINGSUSEWEAPONS), m_pWServer->Game_GetObjectIndex()))
		{
			CRPG_Object_Item* pMelee = Char()->FindItemByType(30);
			if (pMelee)
				SelectItemByIdentifier(pMelee->m_iItemType >> 8,pMelee->m_Identifier, true);

			CD.m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED,false);
			CD.m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUEDDUALWIELD,false);

			CD.m_Disable = CD.m_Disable | PLAYER_DISABLE_SWITCHWEAPON;
			CD.m_Disable.MakeDirty();
		}
		else
		{
/*			//disable all non darkling weapons and enable the darkling weapons
			CRPG_Object_Inventory* pWeaponInv = Char()->GetInventory(AG2_ITEMSLOT_WEAPONS);
			for(int i = 0; i < pWeaponInv->GetNumItems(); i++)
			{
				CRPG_Object_Item* pWeapon = pWeaponInv->GetItemByIndex(i);
				CFStr name = pWeapon->GetItemName();
				if(name.Find("_DARKLING") != -1)
					pWeapon->m_Flags2 = pWeapon->m_Flags2 & ~RPG_ITEM_FLAGS2_NOTSELECTABLE;
				else
					pWeapon->m_Flags2 = pWeapon->m_Flags2 | RPG_ITEM_FLAGS2_NOTSELECTABLE;
			}*/

			//Make sure we got a valid weapon equipped
/*				CRPG_Object_Item* pCurItem = pWeaponInv->FindFinalSelectedItem();
			if(pCurItem->m_Name.Find("WEAPON_DARKLING") != -1)
			{
				for(int i = 0; i < pWeaponInv->GetNumItems(); i++)
				{
					CRPG_Object_Item* pNextItem = pWeaponInv->GetItemByIndex(i);
					CFStr name = pNextItem->GetItemName();
					if(name.Find("WEAPON_DARKLING") == -1)
					{
						CWAG2I_Context AGContext(this, m_pWServer, CD.m_GameTime);
						CD.m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER, pNextItem->m_Identifier);
						if (CD.m_AnimGraph2.SendAttackImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION, AG2_IMPULSEVALUE_ITEMACTION_UNEQUIP)))
							CD.m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED, true);
						CD.m_WeaponUnequipTick = 0;
						break;
					}
				}
			}*/
		}
	}
	return true;
}

void CWObject_CharShapeshifter::RemoveAllDarklingsWeapons(void)
{
	CWO_CharShapeshifter_ClientData& CD = GetClientData();
	if(!m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_CANDARKLINGSUSEWEAPONS), m_pWServer->Game_GetObjectIndex()))
	{
		CRPG_Object_Inventory* pWeaponInv = Char()->GetInventory(AG2_ITEMSLOT_WEAPONS);
		CRPG_Object_Item* pCurItem = pWeaponInv->FindFinalSelectedItem();//Char()->GetFinalSelectedItem(AG2_ITEMSLOT_WEAPONS);

		if(CD.m_SelectItemTimeOutTick == 0 && (pCurItem && pCurItem->m_iItemType == 30) && m_LastUsedItem != 0)
		{
			CRPG_Object_Item* pNextItem = pWeaponInv->FindItemByIdentifier(m_LastUsedItem);
			if (pNextItem)
			{
				CWAG2I_Context AGContext(this, m_pWServer, CD.m_GameTime);
				CD.m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER, pNextItem->m_Identifier);
				if (CD.m_AnimGraph2.SendAttackImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION, AG2_IMPULSEVALUE_ITEMACTION_UNEQUIP)))
					CD.m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED, true);
				CD.m_WeaponUnequipTick = 0;
			}
		}
		if(!pCurItem)
		{
			CRPG_Object_Item* pNextItem = NULL;
			for(int i = 0; i < pWeaponInv->GetNumItems(); i++)
			{
				CRPG_Object_Item* pWeapon = pWeaponInv->GetItemByIndex(i);
				CFStr name = pWeapon->GetItemName();
				if(name.Find("_DARKLING") == -1)
					pNextItem = pWeapon;
			}
			if(pNextItem)
				SelectItemByIdentifier(pNextItem->m_iItemType >> 8, pNextItem->m_Identifier, true);
			CD.m_WeaponUnequipTick = 0;
		}

		CD.m_Disable = CD.m_Disable & ~PLAYER_DISABLE_SWITCHWEAPON;
		CD.m_Disable.MakeDirty();

		//Disable all darkling weapons, enable the rest
		for(int i = 0; i < pWeaponInv->GetNumItems(); i++)
		{
			CRPG_Object_Item* pWeapon = pWeaponInv->GetItemByIndex(i);
			CFStr name = pWeapon->GetItemName();
			if(name.Find("_DARKLING") != -1)
				pWeapon->m_Flags2 = pWeapon->m_Flags2 | RPG_ITEM_FLAGS2_NOTSELECTABLE;
			else
				pWeapon->m_Flags2 = pWeapon->m_Flags2 & ~RPG_ITEM_FLAGS2_NOTSELECTABLE;
		}
	}
}

void CWObject_CharShapeshifter::EquipSomething(void)
{
	CWO_CharShapeshifter_ClientData& CD = GetClientData();
	CRPG_Object_Inventory* pWeaponInv = Char()->GetInventory(AG2_ITEMSLOT_WEAPONS);

	CRPG_Object_Item* pNextItem = NULL;
	for(int i = 0; i < pWeaponInv->GetNumItems(); i++)
	{
		CRPG_Object_Item* pWeapon = pWeaponInv->GetItemByIndex(i);
		CFStr name = pWeapon->GetItemName();
		if(name.Find("_DARKLING") == -1)
			pNextItem = pWeapon;
	}
	if(pNextItem)
		SelectItemByIdentifier(pNextItem->m_iItemType >> 8, pNextItem->m_Identifier, true);
	CD.m_WeaponUnequipTick = 0;
}

void CWObject_CharShapeshifter::ResetSpeeds(void)
{
	CWO_CharShapeshifter_ClientData& CD = GetClientData();
	if(CD.m_IsHuman)
	{
		CD.m_Speed_Forward = m_SpeedForward[0];
		CD.m_Speed_SideStep = m_SpeedSidestep[0];
		CD.m_Speed_Up = m_SpeedUp[0];
		CD.m_Speed_Jump = m_SpeedJump[0];
	}
	else
	{
		CD.m_Speed_Forward = m_SpeedForward[1];
		CD.m_Speed_SideStep = m_SpeedSidestep[10];
		CD.m_Speed_Up = m_SpeedUp[1];
		CD.m_Speed_Jump = m_SpeedJump[1];
	}
}

void CWObject_CharShapeshifter::OnRefresh()
{
	CWO_CharShapeshifter_ClientData& CD = GetClientData();

	if (m_SelfRemoveFlags & SHAPESHIFTER_REMOVE_FLAGS_CHECK)
	{
		bool bRemove = true;
#ifdef INCLUDE_OLD_RAGDOLL
		if (m_Flags & PLAYER_FLAGS_RAGDOLL && m_spRagdoll)
			bRemove = m_spRagdoll->IsStopped();
#endif

		if (bRemove)
		{
			m_SelfRemoveTick = m_pWServer->GetGameTick() + TruncToInt(m_pWServer->GetGameTicksPerSecond() * 2.0f);
			m_SelfRemoveFlags &= ~SHAPESHIFTER_REMOVE_FLAGS_CHECK;
			m_SelfRemoveFlags |= SHAPESHIFTER_REMOVE_FLAGS_REMOVE;
		}
	}
	else if (m_SelfRemoveFlags & SHAPESHIFTER_REMOVE_FLAGS_REMOVE && m_pWServer->GetGameTick() > m_SelfRemoveTick)
		m_pWServer->Object_Destroy(m_iObject);

	if(CD.m_IsHuman)
		parent_human::OnRefresh();
	else
		parent_darkling::OnRefresh();
}

void CWObject_CharShapeshifter::OnPress()
{
	CWO_CharShapeshifter_ClientData& CD = GetClientData();

	if((CD.m_Control_Press & CONTROLBITS_BUTTON4) && !(CD.m_Control_LastPress & CONTROLBITS_BUTTON4))
		m_pWServer->Net_ClientCommand(CD.m_iPlayer, "mp_setmultiplayerstatus(1)");

	if(!(CD.m_Control_Press & CONTROLBITS_BUTTON4) && (CD.m_Control_LastPress & CONTROLBITS_BUTTON4))
		m_pWServer->Net_ClientCommand(CD.m_iPlayer, "mp_setmultiplayerstatus(0)");

	if(CD.m_IsHuman)
		parent_human::OnPress();
	else
		parent_darkling::OnPress();
}

void CWObject_CharShapeshifter::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	CWO_CharShapeshifter_ClientData& CD = GetClientData();

	//TODO: Remove this - velocities should be [units / second] instead of [units / tick]
#define PHYSSTATE_CONVERTFROM20HZ(x) ((x) * 20.0f * m_pWServer->GetGameTickTime())

	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	int KeyValuei = KeyValue.Val_int();
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();

	switch (_KeyHash)
	{
	case MHASH4('MODE','LDAR','KLIN','G'): // "MODELDARKLING"
		{
			CD.m_iDarklingModel = m_pWServer->GetMapData()->GetResourceIndex_Model(CStrF("Characters\\Darklings\\%s", KeyValue.Str()));
			break;
		}
	case MHASH2('MODE','L'): // "MODEL"
		{
			CD.m_iHumanModel[0] = m_pWServer->GetMapData()->GetResourceIndex_Model(KeyValue);
			parent_darkling::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	case MHASH2('MODE','L1'): // "MODEL1"
		{
			CD.m_iHumanModel[1] = m_pWServer->GetMapData()->GetResourceIndex_Model(KeyValue);
			parent_darkling::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	case MHASH2('MODE','L2'): // "MODEL2"
		{
			CD.m_iHumanModel[2] = m_pWServer->GetMapData()->GetResourceIndex_Model(KeyValue);
			parent_darkling::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	case MHASH3('PHYS','_WID','TH'): // "PHYS_WIDTH"
		{
			m_PhysWidth[0] = CD.m_Phys_Width = KeyValuei;
			break;
		}
	case MHASH3('PHYS','_HEI','GHT'): // "PHYS_HEIGHT"
		{
			m_PhysHeight[0] = CD.m_Phys_Height = KeyValuei;
			break;
		}
	case MHASH5('PHYS','_HEI','GHT_','CROU','CH'): // "PHYS_HEIGHT_CROUCH"
		{
			m_PhysHeightCrouched[0] = CD.m_Phys_HeightCrouch = KeyValuei;
			break;
		}
	case MHASH4('PHYS','_DEA','D_RA','DIUS'): // "PHYS_DEAD_RADIUS"
		{
			m_DeadRadius[0] = CD.m_Phys_DeadRadius = KeyValuei;
			break;
		}
	case MHASH4('PHYS','_FRI','CTIO','N'): // "PHYS_FRICTION"
		{
			m_PhysFriction[0] = m_PhysAttrib.m_Friction = KeyValuef;
			break;
		}
	case MHASH4('PHYS','_STE','PSIZ','E'): // "PHYS_STEPSIZE"
		{
			m_PhysStepsize[0] = m_PhysAttrib.m_StepSize = KeyValuef;
			break;
		}
	case MHASH4('SPEE','D_FO','RWAR','D'): // "SPEED_FORWARD"
		{
			m_SpeedForward[0] = CD.m_Speed_Forward = PHYSSTATE_CONVERTFROM20HZ(KeyValuef);
			break;
		}
	case MHASH4('SPEE','D_SI','DEST','EP'): // "SPEED_SIDESTEP"
		{
			m_SpeedSidestep[0] = CD.m_Speed_SideStep = PHYSSTATE_CONVERTFROM20HZ(KeyValuef);
			break;
		}
	case MHASH2('SPEE','D_UP'): // "SPEED_UP"
		{
			m_SpeedUp[0] = CD.m_Speed_Up = PHYSSTATE_CONVERTFROM20HZ(KeyValuef);
			break;
		}
	case MHASH3('SPEE','D_JU','MP'): // "SPEED_JUMP"
		{
			m_SpeedJump[0] = CD.m_Speed_Jump = PHYSSTATE_CONVERTFROM20HZ(KeyValuef);
			break;
		}
	case MHASH2('DIAL','OGUE'): // "DIALOGUE"
		{
			parent_human::OnEvalKey(_KeyHash, _pKey);
			m_Dialogues[0] = Data(RPG_DATAINDEX_DIALOGUEID);
			break;
		}
	case MHASH5('DARK','LING','_DIA','LOGU','E'):
		{
			m_Dialogues[1] = IncludeDialogue(_pKey->GetThisValue(), m_pWServer->GetMapData());
			break;
		}
	case MHASH5('DARK','LING','_PHY','S_WI','DTH'): // "DARKLING_PHYS_WIDTH"
		{
			m_PhysWidth[1] = KeyValuei;
			break;
		}
	case MHASH5('DARK','LING','_PHY','S_HE','IGHT'): // "DARKLING_PHYS_HEIGHT"
		{
			m_PhysHeight[1] = KeyValuei;
			break;
		}
	case MHASH7('DARK','LING','_PHY','S_HE','IGHT','_CRO','UCH'): // "DARKLING_PHYS_HEIGHT_CROUCH"
		{
			m_PhysHeightCrouched[1] = KeyValuei;
			break;
		}
	case MHASH7('DARK','LING','_PHY','S_DE','AD_R','ADIU','S'): // "DARKLING_PHYS_DEAD_RADIUS"
		{
			m_DeadRadius[1] = KeyValuei;
			break;
		}
	case MHASH6('DARK','LING','_PHY','S_FR','ICTI','ON'): // "DARKLING_PHYS_FRICTION"
		{
			m_PhysFriction[1] = KeyValuef;
			break;
		}
	case MHASH6('DARK','LING','_PHY','S_ST','EPSI','ZE'): // "DARKLING_PHYS_STEPSIZE"
		{
			m_PhysStepsize[1] = KeyValuef;
			break;
		}
	case MHASH6('DARK','LING','_SPE','ED_F','ORWA','RD'): // "DARKLING_SPEED_FORWARD"
		{
			m_SpeedForward[1] = PHYSSTATE_CONVERTFROM20HZ(KeyValuef);
			break;
		}
	case MHASH6('DARK','LING','_SPE','ED_S','IDES','TEP'): // "DARKLING_SPEED_SIDESTEP"
		{
			m_SpeedSidestep[1] = PHYSSTATE_CONVERTFROM20HZ(KeyValuef);
			break;
		}
	case MHASH5('DARK','LING','_SPE','ED_U','P'): // "DARKLING_SPEED_UP"
		{
			m_SpeedUp[1] = PHYSSTATE_CONVERTFROM20HZ(KeyValuef);
			break;
		}
	case MHASH5('DARK','LING','_SPE','ED_J','UMP'): // "DARKLING_SPEED_JUMP"
		{
			m_SpeedJump[1] = PHYSSTATE_CONVERTFROM20HZ(KeyValuef);
			break;
		}
	default:
		{
			parent_darkling::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_CharShapeshifter::OnFinishEvalKeys(void)
{
	CWO_CharShapeshifter_ClientData& CD = GetClientData();

	if(CD.m_IsHuman)
		parent_human::OnFinishEvalKeys();
	else
		parent_darkling::OnFinishEvalKeys();

	m_Flags |= PLAYER_FLAGS_NOAIMASSISTANCE;
}

void CWObject_CharShapeshifter::SpawnCharacter(int _PhysMode, int _SpawnBehavior)
{
	CWO_CharShapeshifter_ClientData& CD = GetClientData();

	if(CD.m_IsHuman)
		parent_human::SpawnCharacter(_PhysMode, _SpawnBehavior);
	else
		parent_darkling::SpawnCharacter(_PhysMode, _SpawnBehavior);

	m_pWServer->MessageQueue_SendToObject(CWObject_Message(OBJMSG_CHAR_CHECK_WEAPON_FOR_MP), m_iObject);
}

void CWObject_CharShapeshifter::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT(CWObject_CharShapeshifter::OnClientRefresh);
	CWO_CharShapeshifter_ClientData& CD = GetClientData(_pObj);

	if(CD.m_IsHuman)
		parent_human::OnClientRefresh(_pObj, _pWClient);
	else
		parent_darkling::OnClientRefresh(_pObj, _pWClient);
}

aint CWObject_CharShapeshifter::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	CWO_CharShapeshifter_ClientData& CD = GetClientData(_pObj);

	if(CD.m_IsHuman)
		return parent_human::OnClientMessage(_pObj, _pWClient, _Msg);
	else
		return parent_darkling::OnClientMessage(_pObj, _pWClient, _Msg);
}

int CWObject_CharShapeshifter::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	if(_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA])
	{
		CWO_CharShapeshifter_ClientData& CD = GetClientData(_pObj);

		int8 bOld = CD.m_IsHuman;

		int x = CWObject_Character::OnClientUpdate(_pObj, _pWClient, _pData, _Flags);

		if(bOld != CD.m_IsHuman)
			CD.m_lspSkelInst[0] = NULL;

		return x;	
	}
	
	return CWObject_Character::OnClientUpdate(_pObj, _pWClient, _pData, _Flags);
}

aint CWObject_CharShapeshifter::OnMessage(const CWObject_Message& _Msg)
{
	CWO_CharShapeshifter_ClientData& CD = CWObject_CharShapeshifter::GetClientData();
	
	if(CD.m_IsHuman)
	{
		switch(_Msg.m_Msg)
		{
		case OBJMSG_CHAR_CHECK_WEAPON_FOR_MP:
			{
				return 1;
				CRPG_Object_Inventory* pWeaponInv = Char()->GetInventory(AG2_ITEMSLOT_WEAPONS);
				CRPG_Object_Item* pCurItem = pWeaponInv->FindFinalSelectedItem();

				CWO_CharShapeshifter_ClientData& CD = GetClientData();

				if(!m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_CANDARKLINGSUSEWEAPONS), m_pWServer->Game_GetObjectIndex()))
				{
					if(pCurItem->m_Name.Find("_DARKLING") != -1)
					{
						for(int i = 0; i < pWeaponInv->GetNumItems(); i++)
						{
							CRPG_Object_Item* pNextItem = pWeaponInv->GetItemByIndex(i);
							CFStr name = pNextItem->GetItemName();
							if(name.Find("_DARKLING") == -1)
							{
								CWAG2I_Context AGContext(this, m_pWServer, CD.m_GameTime);
								CD.m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER, pNextItem->m_Identifier);
								if (CD.m_AnimGraph2.SendAttackImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION, AG2_IMPULSEVALUE_ITEMACTION_UNEQUIP)))
									CD.m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED, true);
								CD.m_WeaponUnequipTick = 0;
								break;
							}
						}
					}

					for(int i = 0; i < pWeaponInv->GetNumItems(); i++)
					{
						CRPG_Object_Item* pWeapon = pWeaponInv->GetItemByIndex(i);
						CFStr name = pWeapon->GetItemName();
						if(name.Find("_DARKLING") != -1)
							pWeapon->m_Flags2 = pWeapon->m_Flags2 | RPG_ITEM_FLAGS2_NOTSELECTABLE;
						else
							pWeapon->m_Flags2 = pWeapon->m_Flags2 & ~RPG_ITEM_FLAGS2_NOTSELECTABLE;
					}
				}
				else
				{
					if(pCurItem->m_Name.Find("WEAPON_DARKLING") != -1)
					{
						for(int i = 0; i < pWeaponInv->GetNumItems(); i++)
						{
							CRPG_Object_Item* pNextItem = pWeaponInv->GetItemByIndex(i);
							CFStr name = pNextItem->GetItemName();
							if(name.Find("WEAPON_DARKLING") == -1)
							{
								CWAG2I_Context AGContext(this, m_pWServer, CD.m_GameTime);
								CD.m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER, pNextItem->m_Identifier);
								if (CD.m_AnimGraph2.SendAttackImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION, AG2_IMPULSEVALUE_ITEMACTION_UNEQUIP)))
									CD.m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED, true);
								CD.m_WeaponUnequipTick = 0;
								break;
							}
						}
					}

					for(int i = 0; i < pWeaponInv->GetNumItems(); i++)
					{
						CRPG_Object_Item* pWeapon = pWeaponInv->GetItemByIndex(i);
						CFStr name = pWeapon->GetItemName();
						if(name.Find("_DARKLING") != -1)
							pWeapon->m_Flags2 = pWeapon->m_Flags2 | RPG_ITEM_FLAGS2_NOTSELECTABLE;
						else
							pWeapon->m_Flags2 = pWeapon->m_Flags2 & ~RPG_ITEM_FLAGS2_NOTSELECTABLE;
					}
				}
			}
			break;
		}
		return parent_human::OnMessage(_Msg);
	}
	else
	{
		switch(_Msg.m_Msg) 
		{
		case OBJMSG_AIQUERY_GETACTIVATEPOSITION:
		    {
				CMat4Dfp32& Pos = *(CMat4Dfp32*)_Msg.m_pData;
				Pos = GetPositionMatrix();
/*				GetClientData().GetCamera(Camera, 0.0f, &Camera);
				CRPG_Object_Inventory* pWeaponInv = Char()->GetInventory(AG2_ITEMSLOT_WEAPONS);
				CRPG_Object_Item* pCurItem = pWeaponInv->FindFinalSelectedItem();
				if(pCurItem->m_Name.Find("WEAPON_DARKLING") != -1)
				{	//Using darkling melee weapon, set pos to head or seomthing
					Camera.k[3][0] = m_Pos.k[3][0];
					Camera.k[3][1] = m_Pos.k[3][1];
					Camera.k[3][2] = m_Pos.k[3][2];
				}*/
		    }
			return 1;
		default:
			return parent_darkling::OnMessage(_Msg);
		}
	}
}

void CWObject_CharShapeshifter::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	CWO_CharShapeshifter_ClientData& CD = GetClientData(_pObj);

	if(CD.m_IsHuman)
		parent_human::OnClientRender(_pObj, _pWClient, _pEngine, _ParentMat);
	else
		parent_darkling::OnClientRender(_pObj, _pWClient, _pEngine, _ParentMat);
}

int CWObject_CharShapeshifter::OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo)
{
	CWO_CharShapeshifter_ClientData& CD = GetClientData(_pObj);

	if(CD.m_IsHuman)
		return parent_human::OnPhysicsEvent(_pObj, _pObjOther, _pPhysState, _Event, _Time, _dTime, _pMat, _pCollisionInfo);
	else
		return parent_darkling::OnPhysicsEvent(_pObj, _pObjOther, _pPhysState, _Event, _Time, _dTime, _pMat, _pCollisionInfo);
}










