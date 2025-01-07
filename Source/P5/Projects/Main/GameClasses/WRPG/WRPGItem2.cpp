#include "PCH.h"

#include "WRPGItem2.h"
#include "WRPGChar2.h"
#include "../WObj_RPG.h"
#include "MFloat.h"
#include "../WObj_Char2.h"
#include "WRPGAmmo.h"
#include "../../Exe/WGameContextMain.h"
#include "../../GameWorld/WServerMod.h"

//-------------------------------------------------------------------
//- CRPG_Object_Item2 ------------------------------------------------
//-------------------------------------------------------------------

const char* CRPG_Object_Item2::ms_FlagsStr[] =
{
	"wallcollision", 
	"noviewmodel", 
	"playerlaserbeam",
	"__equipped__", 
	"rpgmodelflags", 
	"aiming", 
	"playerflashlight", 
	"forcewalk", 

	"unselectable", 
	"autoaim",
	"autoequip", 
	"autoactivate",
	"useless",
	"__activated__",
	"needammo", 
	"needlink", 
	"needequippedlink", 
	"keepaftermission",
	"droponunequip",
	"__removed__",
	"renderquantity",
	"questitem",
	"forceselected",
	"twohanded",
	"consume",
	"__usable__", 
	"nopickup",
	"mergeonly", 
	"weapontargeting",
	"singleshot",
	"removeinstant",
	"nomerge",
	NULL
};

const char* CRPG_Object_Item2::ms_RenderFlagsStr[] =
{
	"laserbeam", 
	NULL
};

//-------------------------------------------------------------------

void CRPG_Object_Item2::OnCreate()
{
	MAUTOSTRIP(CRPG_Object_Item2_OnCreate, MAUTOSTRIP_VOID);
	CRPG_Object::OnCreate();
	
	m_WeaponGroup = -1;
	m_FireTimeout = 0;
	
	m_iSound_Cast = -1;
	m_iSound_End = -1;
	m_iSound_Equip = -1;
	m_iSound_Unequip = -1;
	m_iSound_Pickup = -1;
	m_iSound_Dropped = -1;
	
	m_Flags = 0;
	
	m_NumItems = 0;
	m_MaxItems = -1;

	m_iItemType = 0;

	for(int i = 0; i < RPG_ITEM_MAXLINKS; i++)
		m_lLinkedItemTypes[i] = -1;

	m_iCurLink = 0;
	m_NumLinks = 0;

	m_iClientHelperObject = 0;
	m_iMuzzleAttachPoint = 1;

	m_PendingEquipUpdate = -1;
	
	m_iIconSurface = 0;
	m_iLastActivator = -1;

	m_PickupMsg_iDialogue = 0;
	m_PickupMsg_iDialogueID = 0;
	
	m_AIType = "";
	m_AIUseFOV = 1.0f;
	m_RequiredClearance = 0;
	m_GivenClearanceLevel = 0;

	m_NoiseLevel = 0.0f;
	m_Visibility = 0.0f;

	m_nRumble_Activate = 0;

	m_SaveMask = 0;

	m_AimOffset = CVec3Dfp32(0.0f);
	m_AnimProperty = 0;

	m_pEquippedModel = NULL;

	m_iAnimHold = 0;
	m_iAnimAttack = 0;
	m_iAnimEquip = 0;
	m_iAnimUnequip = 0;
	m_iAnimNoAmmoHold = 0;
	m_iAnimNoAmmo = 0;
	m_iAnimReload = 0;
	m_iAnimReloadCrouch = 0;
}

//-------------------------------------------------------------------


void CRPG_Object_Item2::OnIncludeClass(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CRPG_Object_Item2_OnIncludeClass, MAUTOSTRIP_VOID);
	CRPG_Object::OnIncludeClass(_pReg, _pMapData, _pWServer);

	IncludeModelFromKey("MODEL", _pReg, _pMapData);
	IncludeModelFromKey("ATTACHMODEL", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_CAST", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_HOLD", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_END", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_LEARNED", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_EQUIP", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_UNEQUIP", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_DROPPED", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_PICKUP", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_ANIM0", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_ANIM1", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_ANIM2", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_ANIM3", _pReg, _pMapData);

	if(CRPG_Object::m_bPrecacheForPlayerUse)
		IncludeSurfaceFromKey("ICONSURFACE", _pReg, _pMapData);
	IncludeAnimFromKey("ANIM_HOLD", _pReg, _pMapData);
	IncludeAnimFromKey("ANIM_ATTACK", _pReg, _pMapData);
	IncludeAnimFromKey("ANIM_BLOCK", _pReg, _pMapData);
	IncludeAnimFromKey("ANIM_RELOAD", _pReg, _pMapData);
	IncludeAnimFromKey("ANIM_RELOAD_CROUCH", _pReg, _pMapData);
	IncludeAnimFromKey("ANIM_EQUIP", _pReg, _pMapData);
	IncludeAnimFromKey("ANIM_NOAMMOHOLD", _pReg, _pMapData);
	IncludeAnimFromKey("ANIM_NOAMMO", _pReg, _pMapData);
	if(_pReg)
	{
		for(int i = 0; i < ATTACHMODEL_NUMMODELS; i++)
		{
			CRegistry *pChild = _pReg->FindChild(CFStrF("ATTACHMODEL%i", i));
			if(pChild)
			{
				CFStr Str = pChild->GetThisValue();
				Str.GetStrSep("#");
				_pMapData->GetResourceIndex_Model(Str);
			}
		}

		CRegistry *pChild = _pReg->FindChild("PICKUPMSG");
		if(pChild)
		{
			CFStr St = pChild->GetThisValue();
			_pMapData->GetResourceIndex_Dialogue(St.GetStrSep(":"));
		}
	}
}

int CRPG_Object_Item2::ResolveAnimHandle(const char *_pSequence)
{
	MAUTOSTRIP(CRPG_Object_Item_ResolveAnimHandle, 0);
	CWObject_Message Msg(OBJMSG_GAME_RESOLVEANIMHANDLE);
	Msg.m_pData = (void *)_pSequence;
	return m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());
}

//-------------------------------------------------------------------

bool CRPG_Object_Item2::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CRPG_Object_Item2_OnEvalKey, false);
	
 const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	
	int KeyValuei = KeyValue.Val_int();
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();

	switch (_KeyHash)
	{
	case MHASH4('ITEM','NAME','_DEB','UG'): // "ITEMNAME_DEBUG"
		{
			m_ItemName_Debug = KeyValue;		// For debugging purposes only
			break;
		}

	case MHASH3('WEAP','ONGR','OUP'): // "WEAPONGROUP"
		{
			m_WeaponGroup = KeyValuei;
			break;
		}
	
	case MHASH3('RELO','ADTI','ME'): // "RELOADTIME"
		{
			if(KeyValue.CompareNoCase("anim") == 0)
				m_FireTimeout = -1;
			else
				m_FireTimeout = KeyValuef * SERVER_TICKSPERSECOND;
			break;
		}
	
	case MHASH3('NOIS','ELEV','EL'): // "NOISELEVEL"
		{
			m_NoiseLevel = KeyValuef;
			break;
		}

	case MHASH3('VISI','BILI','TY'): // "VISIBILITY"
		{
			m_Visibility = KeyValuef;
			break;
		}

	case MHASH2('MODE','L'): // "MODEL"
		{
			m_Model.m_iModel[0] = m_pWServer->GetMapData()->GetResourceIndex_Model(KeyValue);
			break;
		}

/*	
	case MHASH3('ATTA','CHMO','DEL'): // "ATTACHMODEL"
		{
			m_Model.m_iModel[1] = m_pWServer->GetMapData()->GetResourceIndex_Model(KeyValue);
			break;
		}
*/

	else if(KeyName.CompareSubStr("ATTACHMODEL") == 0)
	{
		// Attachmodel string format: *attachmodel<index> "<attachpoint>#<model>"
		int Index = KeyName.Copy(11, 1024).Val_int() + 1;
		if(Index >= 1 && Index < ATTACHMODEL_NUMMODELS)
		{
			CStr Str = KeyValue;

			int iAttachPoint = Str.GetStrMSep("#").Val_int();
			Str.Trim();

			if (Str == "")
			{
				Str = KeyValue;
				iAttachPoint = -1;
			}

			int iModel = m_pWServer->GetMapData()->GetResourceIndex_Model(Str);

			if (iModel == 0)
			{
				Str = KeyValue;
				iAttachPoint = -1;
				iModel = m_pWServer->GetMapData()->GetResourceIndex_Model(Str);
			}

			if (iModel > 0)
				m_Model.SetModel(Index, iModel, iAttachPoint);
		}
	}
	
	case MHASH4('ATTA','CHRO','TTRA','CK'): // "ATTACHROTTRACK"
		{
			m_Model.m_iAttachRotTrack = KeyValuei;
			break;
		}
	
	case MHASH3('ATTA','CHPO','INT'): // "ATTACHPOINT"
		{
			m_Model.m_iAttachPoint[0] = KeyValuei;
			break;
		}
	
	case MHASH3('SOUN','D_CA','ST'): // "SOUND_CAST"
		{
			m_iSound_Cast = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	
	case MHASH3('SOUN','D_EN','D'): // "SOUND_END"
		{
			m_iSound_End = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	
/*	else if(_KeyHash == MHASH3('SOUN','D_RE','LOAD'))
		m_iSound_Reload = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);*/
	
	case MHASH3('SOUN','D_EQ','UIP'): // "SOUND_EQUIP"
		{
			m_iSound_Equip = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	
	case MHASH4('SOUN','D_UN','EQUI','P'): // "SOUND_UNEQUIP"
		{
			m_iSound_Unequip = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}

	case MHASH3('SOUN','D_PI','CKUP'): // "SOUND_PICKUP"
		{
			m_iSound_Pickup = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}

	case MHASH4('SOUN','D_DR','OPPE','D'): // "SOUND_DROPPED"
		{
			m_iSound_Dropped = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	
	case MHASH3('ANIM','_HOL','D'): // "ANIM_HOLD"
		{
			m_iAnimHold = ResolveAnimHandle(KeyValue);
			break;
		}
	
	case MHASH3('ANIM','_ATT','ACK'): // "ANIM_ATTACK"
		{
			m_iAnimAttack = ResolveAnimHandle(KeyValue);
			break;
		}

	case MHASH5('ANIM','_REL','OAD_','CROU','CH'): // "ANIM_RELOAD_CROUCH"
		{
			m_iAnimReloadCrouch = ResolveAnimHandle(KeyValue);
			break;
		}

	case MHASH3('ANIM','_REL','OAD'): // "ANIM_RELOAD"
		{
			m_iAnimReload = ResolveAnimHandle(KeyValue);
			break;
		}

	case MHASH3('ANIM','_EQU','IP'): // "ANIM_EQUIP"
		{
			m_iAnimEquip = ResolveAnimHandle(KeyValue);
			break;
		}

	case MHASH3('ANIM','_UNE','QUIP'): // "ANIM_UNEQUIP"
		{
			m_iAnimUnequip = ResolveAnimHandle(KeyValue);
			break;
		}
	
	case MHASH4('ANIM','_NOA','MMOH','OLD'): // "ANIM_NOAMMOHOLD"
		{
			m_iAnimNoAmmoHold = ResolveAnimHandle(KeyValue);
			break;
		}

	case MHASH3('ANIM','_NOA','MMO'): // "ANIM_NOAMMO"
		{
			m_iAnimNoAmmo = ResolveAnimHandle(KeyValue);
			break;
		}

	case MHASH7('ANIM','PROP','ERTY','_EQU','IPPE','DITE','M'): // "ANIMPROPERTY_EQUIPPEDITEM"
		{
			m_AnimProperty = CRPG_Object::GetItemAnimProperty(KeyValue);
			break;
		}

	/* Not used anymore?
	case MHASH3('PICK','UP_O','FS'): // "PICKUP_OFS"
		{
			m_PickupOfs.ParseString(KeyValue);
			break;
		}
	*/

	case MHASH3('AIMO','FFSE','T'): // "AIMOFFSET"
		{
			m_AimOffset.ParseString(KeyValue);
			break;
		}

	case MHASH2('NUMI','TEMS'): // "NUMITEMS"
		{
			m_NumItems = KeyValuei;
			break;
		}

	case MHASH2('MAXI','TEMS'): // "MAXITEMS"
		{
			m_MaxItems = KeyValuei;
			break;
		}

	case MHASH2('FLAG','S'): // "FLAGS"
		{
			m_Flags |= KeyValue.TranslateFlags(ms_FlagsStr);
			break;
		}

	case MHASH4('TEST','_NUM','ITEM','S'): // "TEST_NUMITEMS"
		{
			m_NumItems = KeyValuei;
			break;
		}

	case MHASH4('LINK','EDIT','EMTY','PES'): // "LINKEDITEMTYPES"
		{
			CFStr Links = KeyValue;
			while(Links != "")
				m_lLinkedItemTypes[m_NumLinks++] = Links.GetStrMSep(",;").Val_int();
			break;
		}

	else if(KeyName.CompareSubStr("ICONSURFACE") == 0)
	{
		//if(CRPG_Object::m_bPrecacheForPlayerUse)
		m_iIconSurface = m_pWServer->GetMapData()->GetResourceIndex_Surface(KeyValue);
		CStr Upper = KeyValue.UpperCase();
		
		// Ugly hack to find out if we have the map or not!!!! REMOVE FIXME MOGG
		if (Upper.Find("GUI_ICON_MAP_PA2STORAGE01") != -1)
			m_iIconSurface |= 0x8000;
	}
	
	case MHASH2('ITEM','TYPE'): // "ITEMTYPE"
		{
			m_iItemType = KeyValuei;
			break;
		}

	case MHASH5('MUZZ','LEAT','TACH','POIN','T'): // "MUZZLEATTACHPOINT"
		{
			m_iMuzzleAttachPoint = KeyValuei;
			break;
		}
	
	case MHASH3('PICK','UPMS','G'): // "PICKUPMSG"
		{
			CFStr Index = KeyValue;
			CFStr Dialogue = Index.GetStrSep(":");
			m_PickupMsg_iDialogue = m_pWServer->GetMapData()->GetResourceIndex_Dialogue(Dialogue);
			m_PickupMsg_iDialogueID = Index.Val_int();
			break;
		}
	
	case MHASH2('AI_T','YPE'): // "AI_TYPE"
		{
			m_AIType = KeyValue;
			break;
		}

	case MHASH3('AI_U','SEFO', 'V'): // "AI_USEFOV"
		{
			m_AIUseFOV = KeyValuef / 360.0f;
			break;
		}

	case MHASH5('AI_R','EQUI','REDC','LEAR','ANCE'): // "AI_REQUIREDCLEARANCE"
		{
			m_RequiredClearance = KeyValuei;
			break;
		}

	case MHASH5('AI_G','IVEN','CLEA','RANC','E'): // "AI_GIVENCLEARANCE"
		{
			m_GivenClearanceLevel = KeyValuei;
			break;
		}

	case MHASH4('RUMB','LE_A','CTIV','ATE'): // "RUMBLE_ACTIVATE"
		{
			CFStr St = KeyValue;
			while(St != "")
			{
				if(m_nRumble_Activate >= RPG_ITEM_MAXACTIVATERUMBLE)
					break;
				m_lRumble_Activate[m_nRumble_Activate++] = St.GetStrSep(",");
			}
			break;
		}
	case MHASH4('ANIM','SETS','NEED','ED'): // "ANIMSETSNEEDED"
		{
			m_AnimSetsNeeded = KeyValue;
			break;
		}

	default:
		{
			return CRPG_Object::OnEvalKey(_pKey);
			break;
		}
	}
	
	return true;
}

//-------------------------------------------------------------------

void CRPG_Object_Item2::OnFinishEvalKeys()
{
	MAUTOSTRIP(CRPG_Object_Item2_OnFinishEvalKeys, MAUTOSTRIP_VOID);
	CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(m_pWServer);
	if (pServerMod && m_AnimProperty)
	{
		//pServerMod->AddItemAnimType(m_AnimProperty);
		pServerMod->AddItem(m_AnimSetsNeeded, m_AnimProperty);
	}
}

//-------------------------------------------------------------------

CFStr CRPG_Object_Item2::GetItemName()
{
	MAUTOSTRIP(CRPG_Object_Item2_GetItemName, CFStr());
	CFStr Name;
	if(m_Name.CompareSubStr("pickup") == 0)
		Name = "§LITEM_" + m_Name.Copy(7, 1024);
	else
		Name = "§LITEM_" + m_Name;

//	if(m_Flags & RPG_ITEM_FLAGS_RENDERQUANTITY)
	{
		CFStr Stripped;
		CFStr Part;
		CFStr Name2 = Name;
		while(Name2 != "")
		{
			if(Part != "")
			{
				if(Stripped != "")
					Stripped += "_";
				Stripped += Part;
			}
			Part = Name2.GetStrSep("_");
		}
		int iVal = Part.Val_int();
		if((m_Flags & RPG_ITEM_FLAGS_RENDERQUANTITY && iVal != 0) || Part.CompareSubStr("ammo") == 0)
			Name = Stripped;
			
/*		if(m_NumItems > 0)
			Name += CFStrF("§p0%i§pq", m_NumItems);*/
	}
	return Name;
}

//-------------------------------------------------------------------

CFStr CRPG_Object_Item2::GetItemDesc()
{
	MAUTOSTRIP(CRPG_Object_Item2_GetItemDesc, CFStr());
	CFStr Desc;
	if(m_Name.CompareSubStr("pickup") == 0)
		Desc = "§LITEMDESC_" + m_Name.Copy(7, 1024);
	else
		Desc = "§LITEMDESC_" + m_Name;

//	if(m_Flags & RPG_ITEM_FLAGS_RENDERQUANTITY)
	{
		CFStr Stripped;
		CFStr Part;
		CFStr Name2 = Desc;
		while(Name2 != "")
		{
			if(Part != "")
			{
				if(Stripped != "")
					Stripped += "_";
				Stripped += Part;
			}
			Part = Name2.GetStrSep("_");
		}
		int iVal = Part.Val_int();
		if((m_Flags & RPG_ITEM_FLAGS_RENDERQUANTITY && iVal != 0) || Part.CompareSubStr("ammo") == 0)
			Desc = Stripped;
	}
			
	return Desc;
}

//-------------------------------------------------------------------

bool CRPG_Object_Item2::OnProcess(CRPG_Object *_pRoot, int _iObject)
{
	MAUTOSTRIP(CRPG_Object_Item2_OnProcess, false);

	return CRPG_Object::OnProcess(_pRoot, _iObject);
}

//-------------------------------------------------------------------

bool CRPG_Object_Item2::IsEquippable(CRPG_Object* _pRoot)
{
	MAUTOSTRIP(CRPG_Object_Item2_IsEquippable, false);
	// For now, this function is only based on if there is any ammunition left.

	// First this was NEEDAMMO, then for some reason I changed it to NEEDLINK (can't remember why).
	// But that is not enough. Now it's back to NEEDAMMO.
	if (m_Flags & RPG_ITEM_FLAGS_NEEDAMMO)
	{
		CRPG_Object_Char2* pChar = GetChar(_pRoot);
		if (pChar == NULL)
			return (m_NumItems > 0);

		CRPG_Object_Ammo* pAmmo = NULL;//GetAssociatedAmmo(pChar);
		if (pAmmo == NULL)
			return (m_NumItems > 0);

		return (pAmmo->GetNumTotal() > 0);
	}

	return true;
}

//-------------------------------------------------------------------

bool CRPG_Object_Item2::IsActivatable(CRPG_Object* _pRoot,int _iObject,int _Input)
{
	MAUTOSTRIP(CRPG_Object_Item2_IsActivatable, false);
	return true;
}

//-------------------------------------------------------------------

bool CRPG_Object_Item2::Equip(int _iObject, CRPG_Object* _pRoot, int _iPrecedingUnequipAnim, bool _bInstant, bool _bHold)
{
	MAUTOSTRIP(CRPG_Object_Item2_Equip, false);
	CRPG_Object_Char2 *pChar = GetChar(_pRoot);
	if (pChar == NULL)
		return false;

	if (pChar->Wait() == 0)
	{
		if (!_bInstant && (m_iSound_Equip > 0))
		{
			CWObject *pObj = m_pWServer->Object_Get(_iObject);
			if (pObj)
				m_pWServer->Sound_At(pObj->GetPosition(), m_iSound_Equip, 0);
		}

		// Set weapon hold animation
		PlayWeaponAnim(m_iAnimHold,_iObject,SKELANIM_FLAG_BLEND | SKELANIM_FLAG_LOOPING);

		if ((m_iAnimEquip != -1) && !_bInstant)
		{
			if (_iPrecedingUnequipAnim == m_iAnimEquip)
			{
				// Don't do anything, merely wait the second half of the already playing animation.
				//ApplyWait(_iObject, m_SharedEquipAnimMidTicks, 0, 0);
				//ApplyWait(_iObject, m_NumEquipTicks, 0, 0);
			}
			else
			{
			// URGENT FIXME: Flags was not initialized, I fixed this but this could destroy the current behavior, check with owner of code
				int Flags = 0;
				Flags |= PLAYER_ANIM_ONLYTORSO;
				Flags |= _bHold ? PLAYER_ANIM_HOLDATEND : 0;

				//int AnimDurationTicks = PlayAnimSequence(_iObject, m_iAnimEquip, Flags);
				PlayWeaponAnim(m_iAnimEquip,_iObject, SKELANIM_FLAG_BLEND);
				int AnimDurationTicks = 0;// m_SharedEquipAnimMidTicks;
				ApplyWait(_iObject, AnimDurationTicks, 0, 0);
				if((m_iItemType >> 8) < 2)
					SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL, m_iItemType >> 8, GetGameTick(_iObject));
			}
		}
		else
		{
			if((m_iItemType >> 8) < 2)
				SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL, m_iItemType >> 8, GetGameTick(_iObject));
		}

		return true;
	}
	else
	{
		return false;
	}
}

//-------------------------------------------------------------------

bool CRPG_Object_Item2::Unequip(int _iObject, CRPG_Object* _pRoot, int _iSucceedingEquipAnim, bool _bInstant, bool _bHold)
{
	MAUTOSTRIP(CRPG_Object_Item2_Unequip, false);
	CRPG_Object_Char2 *pChar = GetChar(_pRoot);
	if (pChar == NULL)
		return false;

	// Turn flashlight off when unequipping
	m_Flags &= ~RPG_ITEM_FLAGS_FLASHLIGHT;

	if (pChar->Wait() == 0)
	{
		if (!_bInstant && (m_iSound_Unequip > 0))
		{
			CWObject *pObj = m_pWServer->Object_Get(_iObject);
			if (pObj)
				m_pWServer->Sound_At(pObj->GetPosition(), m_iSound_Unequip, 0);
		}

		// Play unequip anim
		PlayWeaponAnim(m_iAnimUnequip,_iObject,SKELANIM_FLAG_BLEND);

		if ((m_iAnimEquip != -1) && !_bInstant)
		{
			// URGENT FIXME: Flags was not initialized, I fixed this but this could destroy the current behavior, check with owner of code
			int Flags = 0;
			Flags |= PLAYER_ANIM_ONLYTORSO;
			Flags |= _bHold ? PLAYER_ANIM_HOLDATEND : 0;

			//if (_iSucceedingEquipAnim == m_iAnimEquip)
			int ChangeItemDelayTicks = 0;//m_NumUnequipTicks;

			ApplyWait(_iObject, ChangeItemDelayTicks, 0, 0);
			int32 ModelUpdateTick = GetGameTick(_iObject);
			if((m_iItemType >> 8) < 2)
				SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL, m_iItemType >> 8, ModelUpdateTick);
		}
		else
		{
			// Update instantly if there are no succeeding equip anim.
			// If there is a succeeding equip anim, that item will update the models.
			// Update after estimate of equip/unequip animation has ended 
			// (only for "weapons" not unarmed)
			int32 ModelUpdateTick = GetGameTick(_iObject);
			if (_iSucceedingEquipAnim == -1)
				if((m_iItemType >> 8) < 2)
					SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL, m_iItemType >> 8, ModelUpdateTick);
		}

		//If item should be dropped when unequipping, tell object to do so
		//Currently we can only drop weapons
		//This is horribly ugly and propably unsafe, but I don't want to rewrite drop item message to be sufficiently general right now...
		if (m_Flags & RPG_ITEM_FLAGS_DROPONUNEQUIP)
		{
			SendMsg(_iObject, OBJMSG_CHAR_FORCEDROPWEAPON);
		}

		return true;
	}
	else
	{
		return false;
	}
}

//-------------------------------------------------------------------

bool CRPG_Object_Item2::MergeItem(int _iObject, CRPG_Object_Item2 *_pObj)
{
	MAUTOSTRIP(CRPG_Object_Item2_MergeItem, false);
	if((m_Flags & RPG_ITEM_FLAGS_NEEDAMMO) && m_iItemType == _pObj->m_iItemType)
	{
		m_NumItems += _pObj->m_NumItems;
		if(m_MaxItems != -1)
			m_NumItems = Min(m_NumItems, m_MaxItems);
		return true;
	}
	return false;
}

//-------------------------------------------------------------------

int CRPG_Object_Item2::OnPickup(int _iObject, CRPG_Object *_pRoot, bool _bNoSound, int _iSender)
{
	MAUTOSTRIP(CRPG_Object_Item2_OnPickup, MAUTOSTRIP_VOID);
	if(_iObject > 0 && m_iSound_Pickup != -1 && !_bNoSound)
	{
		CWObject *pObj = m_pWServer->Object_Get(_iObject);
		if(pObj)
			m_pWServer->Sound_At(pObj->GetPosition(), m_iSound_Pickup, 0);
	}
	return true;
}

//-------------------------------------------------------------------

bool CRPG_Object_Item2::CanPickup()
{
	// Hmm, anything here...?
	return true;
}

//-------------------------------------------------------------------

void CRPG_Object_Item2::OnDeltaLoad(CCFile* _pFile, CRPG_Object* _pCharacter)
{
	int32 Num;
	_pFile->ReadLE(Num);
	m_NumItems = Num;
}

//-------------------------------------------------------------------

void CRPG_Object_Item2::OnDeltaSave(CCFile* _pFile, CRPG_Object* _pCharacter)
{
	int32 Num = m_NumItems;
	_pFile->WriteLE(Num);
}

//-------------------------------------------------------------------

bool CRPG_Object_Item2::NeedsDeltaSave(CRPG_Object* _pCharacter)
{
	if (m_SaveMask == 0)
		return false;

	return true;
}

uint8 CRPG_Object_Item2::GetRefreshFlags(uint8 _Flags)
{
	MAUTOSTRIP(CRPG_Object_Item2_GetRefreshFlags, 0);
	// These flags are kept from being sent every frame (all this trouble has to do with item changes)
	int Mask = (RPG_ITEM_FLAGS_NORENDERMODEL | RPG_ITEM_FLAGS_EQUIPPED | RPG_ITEM_FLAGS_AIMING);
	
	_Flags = (m_Flags & (~Mask)) | (_Flags & Mask);
	return _Flags;
}

//-------------------------------------------------------------------

bool CRPG_Object_Item2::ApplyWait(int _iObject, int _Ticks, fp32 _NoiseLevel, fp32 _Visibility)
{
	MAUTOSTRIP(CRPG_Object_Item2_ApplyWait, false);
	SendMsg(_iObject, OBJMSG_CHAR_RAISENOISELEVEL, _NoiseLevel);
	SendMsg(_iObject, OBJMSG_CHAR_RAISEVISIBILITY, _Visibility);
	return SendMsg(_iObject, OBJMSG_CHAR_APPLYWAIT, _Ticks) != 0;
}

//-------------------------------------------------------------------

bool CRPG_Object_Item2::SetWait(int _iObject, int _Ticks)
{
	MAUTOSTRIP(CRPG_Object_Item2_SetWait, false);
	return SendMsg(_iObject, OBJMSG_CHAR_SETWAIT, _Ticks) != 0;
}

//-------------------------------------------------------------------

int CRPG_Object_Item2::SendDamage(int _iObject, const CVec3Dfp32& _Pos, int _iSender, int _Damage, int _DamageType, int _SurfaceType, const CVec3Dfp32 *_pForce, const CVec3Dfp32 *_pSplatterDir, CCollisionInfo* _pCInfo, const char* _pDamageEffect, int _StunTicks)
{
	MAUTOSTRIP(CRPG_Object_Item2_SendDamage, 0);
	fp32 DamageBoost = 1.0f;

	if (_Damage > 0)
	{
		if(_pForce)
		{
			CVec3Dfp32 Temp = (*_pForce * DamageBoost);

			CWO_DamageMsg Msg(_Damage * DamageBoost, _DamageType, &_Pos, &Temp, _pSplatterDir, _pCInfo, _SurfaceType, _pDamageEffect, _StunTicks);
			return Msg.Send(_iObject, _iSender, m_pWServer);
		}
		else
		{
			CWO_DamageMsg Msg(_Damage * DamageBoost, _DamageType, &_Pos, NULL, _pSplatterDir, _pCInfo, _SurfaceType, _pDamageEffect, _StunTicks);
			return Msg.Send(_iObject, _iSender, m_pWServer);
		}
	}
	return 0;
}

//-------------------------------------------------------------------

CRPG_Object_Char2* CRPG_Object_Item2::GetChar(CRPG_Object* _pRoot)
{
	MAUTOSTRIP(CRPG_Object_Item2_GetChar, NULL);
	if ((_pRoot == NULL) || (_pRoot->GetType() != TYPE_CHAR))
		return NULL;

	//JK-FIX: Don't do this using TDynamicCast
	CRPG_Object_Char2* pChar = TDynamicCast<CRPG_Object_Char2>(_pRoot);

	return pChar;
}

//-------------------------------------------------------------------

int CRPG_Object_Item2::GetCurLinkItemType()
{
	MAUTOSTRIP(CRPG_Object_Item2_GetCurLinkItemType, 0);
	if(m_NumLinks == 0)
		return -1;

	return m_lLinkedItemTypes[m_iCurLink];
}

//-------------------------------------------------------------------

/*CRPG_Object_Ammo* CRPG_Object_Item2::GetAssociatedAmmo(CRPG_Object *_pRoot)
{
	MAUTOSTRIP(CRPG_Object_Item2_GetAssociatedAmmo, NULL);
	CRPG_Object_Char2 *pChar = GetChar(_pRoot);
	if (pChar == NULL)
		return NULL;

	return pChar->GetAssociatedAmmo(m_iItemType);
}*/

//-------------------------------------------------------------------

CRPG_Object_Item2* CRPG_Object_Item2::GetCurLinkedItem(CRPG_Object *_pRoot)
{
	MAUTOSTRIP(CRPG_Object_Item2_GetCurLinkedItem, NULL);
	CRPG_Object_Char2 *pChar = GetChar(_pRoot);
	if (pChar == NULL)
		return NULL;

	int iType = GetCurLinkItemType();
	
	return pChar->FindItemByType(iType);
}

//-------------------------------------------------------------------

bool CRPG_Object_Item2::HasLinkedItemType(int _ItemType)
{
	MAUTOSTRIP(CRPG_Object_Item2_HasLinkedItemType, false);
	for(int i = 0; i < m_NumLinks; i++)
		if(m_lLinkedItemTypes[i] == _ItemType)
			return true;

	return false;
}

//-------------------------------------------------------------------

bool CRPG_Object_Item2::CheckAmmo(CRPG_Object *_pRoot)
{
	MAUTOSTRIP(CRPG_Object_Item2_CheckAmmo, false);
	if(!(m_Flags & RPG_ITEM_FLAGS_NEEDAMMO))
		return true;

	CRPG_Object_Ammo *pAmmo = NULL;//GetAssociatedAmmo(_pRoot);
	if (pAmmo != NULL)
		return (pAmmo->GetNumLoaded() > 0);

	return (m_NumItems > 0);
}

//-------------------------------------------------------------------

int CRPG_Object_Item2::GetAmmo(CRPG_Object* _pRoot, int _iType)
{
	CRPG_Object_Ammo *pAmmo = NULL;//GetAssociatedAmmo(_pRoot);
	if (pAmmo != NULL)
		return pAmmo->GetNumLoaded();

	return m_NumItems;
}

//-------------------------------------------------------------------

int CRPG_Object_Item2::GetActivateAnim(int _iObject)
{
	MAUTOSTRIP(CRPG_Object_Item2_GetActivateAnim, 0);
	return m_iAnimAttack;
}

//-------------------------------------------------------------------

bool CRPG_Object_Item2::DrawAmmo(int _Num, const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, bool _bApplyFireTimeout)
{
	MAUTOSTRIP(CRPG_Object_Item2_DrawAmmo, false);
	CRPG_Object_Ammo *pAmmo = NULL;//GetAssociatedAmmo(_pRoot);
	if(!pAmmo)
	{
		if(m_NumItems > 0)
		{
			m_NumItems--;
			SAVEMASK_SETBIT(LOADSAVEMASK_NUMITEMS);

			// Flag item as unequipped, or rather consumed.
			// This need to be replicated to the client in RpgMechanics, since it hides, for example, a thrown bomb, from being rendered in hand.
			if (m_Flags & RPG_ITEM_FLAGS_CONSUME)
				m_Flags &= ~RPG_ITEM_FLAGS_EQUIPPED;
		}
	}
	else
	{
		SAVEMASK_SETBIT(LOADSAVEMASK_ASSOCIATEDAMMO);
		if(pAmmo->ConsumeLoaded(1) == 0)
		{
//			Reload(&_Mat, _pRoot, _iObject);
			_bApplyFireTimeout = false;
		}
	}

	if(_bApplyFireTimeout)
	{
		if(m_FireTimeout > 0)
			ApplyWait(_iObject, m_FireTimeout, m_NoiseLevel, m_Visibility);

		//int Ticks = PlayAnimSequence(_iObject, GetActivateAnim(_iObject), PLAYER_ANIM_ONLYTORSO);
		int Ticks = 0;
		PlayWeaponAnim(GetActivateAnim(_iObject),_iObject,SKELANIM_FLAG_BLEND);

		if(m_FireTimeout == -1 && Ticks > 0)
			ApplyWait(_iObject, Ticks, m_NoiseLevel, m_Visibility);
	}

	if ((m_Flags & RPG_ITEM_FLAGS_QUESTITEM) /*&& (m_Flags & RPG_ITEM_FLAGS_RENDERQUANTITY)*/)
		SendMsg(_iObject, OBJMSG_CHAR_UPDATEQUESTICONS);

	return false;
}

//-------------------------------------------------------------------

bool CRPG_Object_Item2::Activate(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	MAUTOSTRIP(CRPG_Object_Item2_Activate, false);
/*	if(!CheckAmmo(_pRoot))
		return Reload(&_Mat, _pRoot, _iObject);		// SAVE: Only affects the GetAssociatedAmmo CRPG_Object_Ammo class*/
	
	int iObj = -1;
	if(OnActivate(_Mat, _pRoot, _iObject, _Input))	// SAVE: Nothing in CRPG_Object_Item2 is affected by this
	{
		if(m_Flags & RPG_ITEM_FLAGS_NEEDAMMO)
			DrawAmmo(1, _Mat, _pRoot, _iObject);	// SAVE: If this object doesn't have an associated ammo class DrawAmmo reduce m_NumItems by one.
		if(m_iSound_Cast != -1)
			m_pWServer->Sound_At(CVec3Dfp32::GetMatrixRow(_Mat, 3), m_iSound_Cast, 0);
	}
	
	return true;
}

//-------------------------------------------------------------------

void CRPG_Object_Item2::Line(const CVec3Dfp32& _Pos0, const CVec3Dfp32& _Pos1, int _Col)
{
	MAUTOSTRIP(CRPG_Object_Item2_Line, MAUTOSTRIP_VOID);
	m_pWServer->Debug_RenderWire(_Pos0, _Pos1, _Col, 5);
}

CMat43fp32 CRPG_Object_Item2::GetAttachOnCharMatrix(CAttachInfo &_AttachInfo)
{
	CMat43fp32 Pos;		
	if(_AttachInfo.m_pSkelInstance->m_lBoneTransform.ValidPos(_AttachInfo.m_iCharRotTrack))
		Pos.CreateFrom(_AttachInfo.m_pSkelInstance->m_lBoneTransform[_AttachInfo.m_iCharRotTrack]);
	else
{
		Pos.Unit();
		return Pos;
	}
	
	const CXR_SkeletonAttachPoint *pPoint = _AttachInfo.m_pCharSkeleton->GetAttachPoint(_AttachInfo.m_iCharAttachPoint);
	if(pPoint)
		(pPoint->m_LocalPos * Pos).SetRow(Pos,3);
	
	return Pos;
}

CMat43fp32 CRPG_Object_Item2::GetItemOnCharMatrix(CAttachInfo &_AttachInfo)
{
	CMat43fp32 Attach = GetAttachOnCharMatrix(_AttachInfo);
	CMat43fp32 Res;
	const CXR_SkeletonAttachPoint *pItemHandle = _AttachInfo.m_pItemSkeleton ? _AttachInfo.m_pItemSkeleton->GetAttachPoint(0) : NULL;
	if(pItemHandle)
{
		CMat43fp32 Handle;
		Handle.Unit();
		(-pItemHandle->m_LocalPos).SetRow(Handle, 3);
		Handle.Multiply(Attach, Res);
	}
	else
		Res = Attach;

	return Res;
}

CMat43fp32 CRPG_Object_Item2::GetAttachOnItemMatrix(CAttachInfo &_AttachInfo)
{
	CMat43fp32 Res = GetItemOnCharMatrix(_AttachInfo);
	const CXR_SkeletonAttachPoint *pAttach = _AttachInfo.m_pItemSkeleton ? _AttachInfo.m_pItemSkeleton->GetAttachPoint(_AttachInfo.m_iItemAttachPoint) : NULL;
	if(pAttach)
		CVec3Dfp32::GetRow(Res, 3) = pAttach->m_LocalPos * Res;

	return Res;
	}

CMat43fp32 CRPG_Object_Item2::GetAttachMatrix(int _iObject, int _iCharRottrack, int _iCharAttachPoint, int _iItemAttachPoint)
{
	CWObject *pObj = m_pWServer->Object_Get(_iObject);
	if(!pObj)
	{
		CMat43fp32 Unit;
		Unit.Unit();
		return Unit;
	}

	CXR_AnimState AnimState;
	CWObject_Message Msg(OBJSYSMSG_GETANIMSTATE);
	Msg.m_pData = &AnimState;
	Msg.m_DataSize = sizeof(AnimState);

	if(!m_pWServer->Message_SendToObject(Msg, _iObject))
		return pObj->GetPositionMatrix();

	CXR_Model *pChar = m_pWServer->GetMapData()->GetResource_Model(pObj->m_iModel[0]);
	CXR_Skeleton *pSkelChar = pChar ? (CXR_Skeleton*)pChar->GetParam(MODEL_PARAM_SKELETON) : NULL;
	if(!AnimState.m_pSkeletonInst || !pSkelChar)
		return pObj->GetPositionMatrix();

	CAttachInfo Info;
	Info.m_iCharAttachPoint = _iCharAttachPoint;
	Info.m_iCharRotTrack = _iCharRottrack;
	Info.m_pCharSkeleton = pSkelChar;
	Info.m_pSkelInstance = AnimState.m_pSkeletonInst;

	if(_iItemAttachPoint == -1)
		return GetAttachOnCharMatrix(Info);
	else
	{
		Info.m_iItemAttachPoint = _iItemAttachPoint;
		CXR_Model *pItem = m_pWServer->GetMapData()->GetResource_Model(m_Model.m_iModel[0]);
		Info.m_pItemSkeleton = pItem ? (CXR_Skeleton*)pItem->GetParam(MODEL_PARAM_SKELETON) : NULL;
		return GetAttachOnItemMatrix(Info);
	}
}
	
CMat43fp32 CRPG_Object_Item2::GetCurAttachOnItemMatrix(int _iObject)
	{
	CMat43fp32 Res = GetAttachMatrix(_iObject, m_Model.m_iAttachRotTrack, m_Model.m_iAttachPoint[0], m_iMuzzleAttachPoint);
	return Res;
	}

bool CRPG_Object_Item2::UpdateAmmoBarInfo(CRPG_Object *_pRoot, uint16 &_Surface1, uint16 &_Surface2, uint8 &_NumTotal, uint8 &_NumLoaded, uint32 &_Extra)
{
	MAUTOSTRIP(CRPG_Object_Item2_UpdateAmmoBarInfo, false);
	CRPG_Object_Char2 *pChar = GetChar(_pRoot);
	if (pChar == NULL)
		return false;
	
	// FIXME
	CRPG_Object_Ammo* pAmmo = NULL;//pChar->GetAssociatedAmmo();
	if (pAmmo == NULL)
		return false;

	_Surface1 = (uint16)pAmmo->m_iAmmoSurface1;
	_Surface2 = (uint16)pAmmo->m_iAmmoSurface2;
	_NumTotal = Min(int(255 * (fp32)pAmmo->GetNumTotal() / (fp32)pAmmo->GetMaxTotal()), 255);
	_NumLoaded = Min(pAmmo->GetNumLoaded(), 255);
	_Extra = 0;
	return true;
}

void CRPG_Object_Item2::TagAnimationsForPrecache(CWAGI_Context* _pContext, CWAGI* _pAgi)
{
	// MovBas+MovBasAss+....
	CStr Needed = m_AnimSetsNeeded;
	while (Needed.Len() > 0)
	{
		CStr SetName = Needed.GetStrSep("+");
		_pAgi->TagAnimSetFromName(_pContext, m_pWServer->GetMapData(), m_pWServer->m_spWData,SetName);
	}
}

//-------------------------------------------------------------------

bool CRPG_Object_Item2::CanUse(int32 _iChar)
{
	return true;
}

//-------------------------------------------------------------------

void CRPG_Object_Item2::UpdateAmmoList(CRPG_Object *_pRoot)
{
	MAUTOSTRIP(CRPG_Object_Item2_UpdateAmmoList, MAUTOSTRIP_VOID);
//	Reload(NULL, _pRoot, false);
}

//-------------------------------------------------------------------

//Get the number of extra magazines. Return -1 if we don't need/use extra magazines.
int CRPG_Object_Item2::ShowNumMagazines()
{
	return -1;
};


//-------------------------------------------------------------------

int CRPG_Object_Item2::GetGameTick(int _iObject)
{
	MAUTOSTRIP(CRPG_Object_Item2_GetGameTick, 0);
	int iTick = SendMsg(_iObject, OBJMSG_CHAR_GETGAMETICK);
	if(!iTick)
		iTick = m_pWServer->GetGameTick();

	return iTick;
}

//-------------------------------------------------------------------

//Returns the speed (in world units per server frame) of any projectiles the item can summon, or
//_FP32_MAX if these projectiles are instantaneous. If the object does not summon projectiles, 0 is returned.
fp32 CRPG_Object_Item2::GetProjectileSpeed()
{
	MAUTOSTRIP(CRPG_Object_Item2_GetProjectileSpeed, 0.0f);
	return 0;
};

//-------------------------------------------------------------------

//-------------------------------------------------------------------
CWO_Damage * CRPG_Object_Item2::GetDamage()
{
	return NULL;
}
//-------------------------------------------------------------------

//-------------------------------------------------------------------
bool CRPG_Object_Item2::IsMeleeWeapon()
{
	return false;
}
//-------------------------------------------------------------------

//-------------------------------------------------------------------
int CRPG_Object_Item2::GetRequiredClearanceLevel()
{
	return m_RequiredClearance;
}
//-------------------------------------------------------------------

int CRPG_Object_Item2::GetGivenClearanceLevel()
{
	return m_GivenClearanceLevel;
}


//-------------------------------------------------------------------
int32 CRPG_Object_Item2::PlayWeaponAnimFromType(int32 _Type, int32 _iObject, uint16 _Flags)
{
	int32 iAnim = -1;
	switch (_Type)
	{
	case RPG_ITEM_ANIM_HOLD: iAnim = m_iAnimHold; break;
	case RPG_ITEM_ANIM_ATTACK: iAnim = m_iAnimAttack; break;
	case RPG_ITEM_ANIM_RELOAD: iAnim = m_iAnimReload; break;
	case RPG_ITEM_ANIM_RELOAD_CROUCH: iAnim = m_iAnimReloadCrouch; break;
	case RPG_ITEM_ANIM_EQUIP: iAnim = m_iAnimEquip; break;
	case RPG_ITEM_ANIM_NOAMMOHOLD: iAnim = m_iAnimEquip; break;
	case RPG_ITEM_ANIM_NOAMMO: iAnim = m_iAnimEquip; break;
	default: break;
	}

	if (iAnim != -1)
		return PlayWeaponAnim(iAnim, _iObject, _Flags);

	return false;
}
// Play animations on the (hopefully) skeleton animated item
int CRPG_Object_Item2::PlayWeaponAnim(int _iAnim, int _iObject, uint16 _Flags)
{
	return SendMsg(_iObject, OBJMSG_CHAR_PLAYWEAPONANIM, _iAnim, _Flags);
}

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Item2, CRPG_Object);