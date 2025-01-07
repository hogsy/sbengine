#include "PCH.h"

#include "WRPGSpell.h"
#include "WRPGChar.h"
#include "../WObj_RPG.h"
#include "../WObj_Weapons/WObj_Spells.h"
#include "../WObj_Weapons/WObj_ClientDebris.h"
#include "MFloat.h"
#include "../WObj_Char.h"
#include "WRPGAmmo.h"
#include "../../Exe/WGameContextMain.h"
#include "../../GameWorld/WServerMod.h"
#include "../WObj_Misc/WObj_Shell.h"
#include "../WObj_Char/WObj_CharShapeshifter.h"

//-------------------------------------------------------------------
//- CRPG_Object_Item ------------------------------------------------
//-------------------------------------------------------------------

const char* CRPG_Object_Item::ms_FlagsStr[] =
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

const char* CRPG_Object_Item::ms_FlagsStr2[] =
{
	"unique", 
	"throwawayonempty",
	"nolastthrowaway",
	"notselectable",
	"cloneattachitem",
	"nomuzzle",
	"noammodraw",
	"__internal_use_0x80__",
	"forceshellemit",
	"grabbableobject",
	"forceequipright",
	"forceequipleft",
	"permanent",
	"drainsdarkness",
	"autofjuff",
	"transcendinventory",
	NULL
};

const char* CRPG_Object_Item::ms_RenderFlagsStr[] =
{
	"laserbeam", 
	NULL
};

//-------------------------------------------------------------------

void CRPG_Object_Item::OnCreate()
{
	MAUTOSTRIP(CRPG_Object_Item_OnCreate, MAUTOSTRIP_VOID);
	CRPG_Object::OnCreate();
	
	m_Price = 0;
	m_WeaponGroup = -1;
	m_FireTimeout = 0;
	
	m_iSkill = -1;
	m_iSound_Cast = -1;
	m_iSound_Cast_Player = -1;
	m_iSound_Cast_LeftItem_Player = -1;
	m_iSound_Learned = -1;
	m_iSound_Hold = -1;
	m_iSound_End = -1;
	m_iSound_Equip = -1;
	m_iSound_Unequip = -1;
	m_iSound_Pickup = -1;
	m_iSound_Dropped = -1;
	m_iSound_Anim[0] = 0;
	m_iSound_Anim[1] = 0;
	m_iSound_Anim[2] = 0;
	m_iSound_Anim[3] = 0;
	
	m_Flags = 0;
	m_Flags2 = 0;

	
	m_NumItems = 0;
	m_MaxItems = -1;

	m_AmmoDraw = 1;
	m_iExtraActivationWait = 0;

	//m_SharedEquipAnimMidTicks = 0.4f * SERVER_TICKSPERSECOND;
	m_NumUnequipTicks = 0;
	m_NumEquipTicks = 0;

	// Not too sure about this one..
	//m_iAnimBlock = -1;

	m_iSpellIndex = 0;
	m_iItemType = 0;
	m_AnimType = -1;
	m_GroupType = -1;
	m_AnimGripType = -1;
	m_iFirstAttachPoint = -1;

	for(int i = 0; i < RPG_ITEM_MAXLINKS; i++)
		m_lLinkedItemTypes[i] = -1;

	m_iCurLink = 0;
	m_NumLinks = 0;
	m_iPhysModel = 0;

	m_Identifier = 0;
	m_LastEquipped = 0;

	m_iClientHelperObject = 0;
	m_iMuzzleAttachPoint = 1;

	m_MaxAmmo = 0;
	m_AmmoLoad = 0;
	m_ShotsFired = 0;

	m_PendingEquipUpdate = -1;

	m_iAnimGraph = 0;
	
	m_iIconSurface = 0;
	m_iActivationSurface = 0;
	m_JournalImage = 0;
	m_iLastActivator = -1;

	m_PickupMsg_iDialogue = 0;
	m_PickupMsg_iDialogueID = 0;

	m_FocusFrameOffset = 0;

	m_DropItemOffset = 0;
	
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

	m_bUnequipWhenEmpty = false;
	m_bNoPlayerPickup = false;
	m_bReplaceOnSameType = false;
	m_bNoLoadedAmmoMergeOnPickup = false;

	m_iShellType = -1;
	m_iShellAttach = -1;
}

//-------------------------------------------------------------------


void CRPG_Object_Item::OnIncludeClass(const CRegistry *_pReg, CMapData *_pMapData, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CRPG_Object_Item_OnIncludeClass, MAUTOSTRIP_VOID);
	CRPG_Object::OnIncludeClass(_pReg, _pMapData, _pWServer);

	IncludeModelFromKey("MODEL", _pReg, _pMapData);
	const CRegistry* pChildReg = _pReg->FindChild("PHYSMODEL");
	if (pChildReg)
		_pMapData->GetResourceIndex_BSPModel(pChildReg->GetThisValue());
	IncludeModelFromKey("ATTACHMODEL", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_CAST", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_CAST_PLAYER", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_CAST_PLAYER_LEFT", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_CAST_PLAYER_RIGHT", _pReg, _pMapData);
	IncludeSoundFromKey("SOUND_CAST_LEFTITEM_PLAYER", _pReg, _pMapData);
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

	IncludeShellTypeFromKey("SUMMON_SHELL_TYPE", _pReg, _pMapData);
	
	if(CRPG_Object::m_bPrecacheForPlayerUse)
	{
		IncludeSurfaceFromKey("ICONSURFACE", _pReg, _pMapData);
		IncludeSurfaceFromKey("ACTIVATIONSURFACE", _pReg, _pMapData);
	}
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
		const CRegistry *pAnimGraph2Reg = _pReg->FindChild("ANIMGRAPH2");
		if(pAnimGraph2Reg)
		{
			CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(_pWServer);
			int32 iAnimGraph = _pMapData->GetResourceIndex_AnimGraph2(pAnimGraph2Reg->GetThisValue());
			CWRes_AnimGraph2* pAnimGraph2Res = _pMapData->GetResource_AnimGraph2(iAnimGraph);
			CXRAG2* pAnimGraph = (pAnimGraph2Res ? pAnimGraph2Res->GetAnimGraph() : NULL);
			if (pAnimGraph)
			{
				// Go through all animation layers
				int32 NumAnimLayers = pAnimGraph->GetNumAnimLayers();
				for (int32 i = 0; i < NumAnimLayers; i++)
				{
					const CXRAG2_AnimLayer* pLayer = pAnimGraph->GetAnimLayer(i);
					const CXRAG2_AnimNames* pAnimName = pAnimGraph->GetAnimName(pLayer->GetAnimIndex());
					M_ASSERT(pAnimName, "CWAG2I::TagAnimSetFromImpulses Invalid AnimName");

					CStr ContainerName = pAnimGraph->GetAnimContainerName(pAnimName->m_iContainerName);
					//ConOutL(CStrF("Adding animation: %s:%d",ContainerName.Str(),pAnimName->m_iAnimSeq));
					pServerMod->AddAnimContainerEntry(ContainerName, pAnimName->m_iAnimSeq);
				}
			}
		}

		const CRegistry *pEffectListReg = _pReg->FindChild("EFFECTLIST");
		if (pEffectListReg)
		{
			int32 NumChildren = pEffectListReg->GetNumChildren();
			for (int32 i = 0; i < NumChildren; i++)
			{
				const CRegistry* pChild = pEffectListReg->GetChild(i);
				CStr Effect = pChild->GetThisValue();
				Effect.GetStrSep("#");
				_pMapData->GetResourceIndex_Model(Effect);
			}
		}

		for(int i = 0; i < ATTACHMODEL_NUMMODELS; i++)
		{
			const CRegistry *pChild = _pReg->FindChild(CFStrF("ATTACHMODEL%i", i));
			if(pChild)
			{
				CFStr Str = pChild->GetThisValue();
				Str.GetStrSep("#");
				_pMapData->GetResourceIndex_Model(Str);
			}
		}

		const CRegistry *pChild = _pReg->FindChild("PICKUPMSG");
		if(pChild)
		{
			CFStr St = pChild->GetThisValue();
			_pMapData->GetResourceIndex_Dialogue(St.GetStrSep(":"));
		}
	}
}

//-------------------------------------------------------------------

bool CRPG_Object_Item::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CRPG_Object_Item_OnEvalKey, false);
	
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	
	int KeyValuei = KeyValue.Val_int();
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();
	
	switch (_KeyHash)
	{
	case MHASH2('PRIC','E'): // "PRICE"
		{
			m_Price = (int)KeyValuef;
			break;
		}

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
				m_FireTimeout = (int)(KeyValuef * m_pWServer->GetGameTicksPerSecond());
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

	case MHASH3('PHYS','MODE','L'): // "MODEL"
		{
			m_iPhysModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(KeyValue);
			break;
		}

/*	
	case MHASH3('ATTA','CHMO','DEL'): // "ATTACHMODEL"
		{
			m_Model.m_iModel[1] = m_pWServer->GetMapData()->GetResourceIndex_Model(KeyValue);
			break;
		}
*/

	case MHASH3('ANIM','GRAP','H2'): // "ANIMGRAPH2"
		{
			m_iAnimGraph = m_pWServer->GetMapData()->GetResourceIndex_AnimGraph2(KeyValue);
			break;
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
	case MHASH5('FIRS','T','ATTA','CHPO','INT'): // "FIRSTATTACHPOINT"
		{
			m_iFirstAttachPoint = KeyValuei;
			break;
		}
	
	case MHASH3('SOUN','D_CA','ST'): // "SOUND_CAST"
		{
			m_iSound_Cast = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}

	case MHASH5('SOUN','D_CA','ST_P','LAYE','R'): // "SOUND_CAST_PLAYER"
		{
			m_iSound_Cast_Player = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}

	case MHASH6('SOUN','D_CA','ST_P','LAYE','R_RI','GHT'): // "SOUND_CAST_PLAYER_RIGHT"
		{
			m_iSound_Cast_Player = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}

	case MHASH6('SOUN','D_CA','ST_P','LAYE','R_LE','FT'): // "SOUND_CAST_PLAYER_LEFT"
		{
			m_iSound_Cast_LeftItem_Player = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}

	case MHASH3('SOUN','D_HO','LD'): // "SOUND_HOLD"
		{
			m_iSound_Hold = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	
	case MHASH3('SOUN','D_EN','D'): // "SOUND_END"
		{
			m_iSound_End = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	
	case MHASH4('SOUN','D_LE','ARNE','D'): // "SOUND_LEARNED"
		{
			m_iSound_Learned = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	
/*	
	case MHASH3('SOUN','D_RE','LOAD'): // "SOUND_RELOAD"
		{
			m_iSound_Reload = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
*/
	
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

	case MHASH4('NUME','QUIP','TICK','S'): // "NUMEQUIPTICKS"
		{
			m_NumUnequipTicks = KeyValuei;
			break;
		}
	case MHASH4('NUMU','NEQU','IPTI','CKS'): // "NUMUNEQUIPTICKS"
		{
			m_NumUnequipTicks = KeyValuei;
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

	case MHASH3('SPEL','LIND','EX'): // "SPELLINDEX"
		{
			m_iSpellIndex = KeyValuei;
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

	case MHASH2('AMMO','DRAW'): // "AMMODRAW"
		{
			m_AmmoDraw = KeyValuei;
			break;
		}

	case MHASH2('FLAG','S'): // "FLAGS"
		{
			m_Flags |= KeyValue.TranslateFlags(ms_FlagsStr);
			break;
		}
	case MHASH2('FLAG','S2'): // "FLAGS2"
		{
			m_Flags2 |= KeyValue.TranslateFlags(ms_FlagsStr2);
			break;
		}

	case MHASH2('SKIL','L'): // "SKILL"
		{
			m_iSkill = KeyValue.TranslateInt(CRPG_Object_Char::ms_SkillStr);
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
	
	case MHASH2('ITEM','TYPE'): // "ITEMTYPE"
		{
			m_iItemType = KeyValuei;
			break;
		}

	case MHASH2('ANIM','TYPE'): // "ANIMTYPE"
		{
			m_AnimType = KeyValuei;
			break;
		}
	case MHASH3('GROU','PTYP','E'): // "GROUPTYPE"
		{
			m_GroupType = KeyValuei;
			break;
		}

	case MHASH3('ANIM','GRIP','TYPE'): // "ANIMGRIPTYPE"
		{
			m_AnimGripType = KeyValuei;
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
	
/*	
	case MHASH1('AMMO'): // "AMMO"
		{
			CStr Ammo = KeyValue;
			m_NumAmmo = 0;
			while(Ammo != "")
			{
				m_lAmmoType[m_NumAmmo] = Ammo.GetStrSep(",");
				m_lDefaultAmmo[m_NumAmmo] = Ammo.GetStrSep(",").Val_int();
				if(m_lAmmoType[m_NumAmmo] != "")
				{
					m_NumAmmo++;
					if(m_NumAmmo >= RPG_NUMAMMO)
						Error("CRPG_Object_Item::OnEvalKey", "Too many ammo-types");
				}
			}
			break;
		}
*/
	
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
	case MHASH4('FOCU','SFRA','MEOF','FSET'): // "FOCUSFRAMEOFFSET"
		{
			m_FocusFrameOffset = (int8)KeyValuei;
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
	case MHASH4('UNEQ','UIPW','HENE','MPTY'): // "UNEQUIPWHENEMPTY"
		{
			m_bUnequipWhenEmpty = KeyValuei != 0;
			break;
		}
	case MHASH4('DROP','ITEM','OFFS','ET'): // "DROPITEMOFFSET"
		{
			m_DropItemOffset = KeyValuei;
			break;
		}
	case MHASH4('NOPL','AYER','PICK','UP'): // "NOPLAYERPICKUP"
		{
			m_bNoPlayerPickup = KeyValuei != 0;
			break;
		}
	case MHASH5('REPL','ACEO','NSAM','ETYP','E'): // "REPLACEONSAMETYPE"
		{
			m_bReplaceOnSameType = KeyValuei != 0;
			break;
		}
	case MHASH7('NOLO','ADED','AMMO','MERG','EONP','ICKU','P'): // "NOLOADEDAMMOMERGEONPICKUP"
		{
			m_bNoLoadedAmmoMergeOnPickup = KeyValue != 0;
			break;
		}
	case MHASH5('SUMM','ON_S','HELL','_TYP','E'): // "SUMMON_SHELL_TYPE"
		{
			m_iShellType = KeyValue.Val_int();
			CWO_ShellManager* pShellMgr = (CWO_ShellManager*)m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETSHELLMANAGER), m_pWServer->Game_GetObjectIndex());
			if(pShellMgr) pShellMgr->IncludeShellType(m_iShellType, m_pWServer);
			break;
		}
	case MHASH5('SUMM','ON_S','HELL','_ATT','ACH'): // "SUMMON_SHELL_ATTACH"
		{
			m_iShellAttach = KeyValue.Val_int();
			break;
		}
	case MHASH3('EFFE','CTLI','ST'): // "EFFECTLIST"
		{
			m_Model.m_ExtraModels.Clear();
			int32 NumChildren = _pKey->GetNumChildren();
			m_Model.m_ExtraModels.SetLen(NumChildren);
			for (int32 i = 0; i < NumChildren; i++)
			{
				const CRegistry* pChild = _pKey->GetChild(i);

				// Effects might not be ordered in reg the same they were written
				//int32 iEffect = pChild->GetThisName().GetStrSep("EFFECT").Val_int();
				CStr ChildName = pChild->GetThisName();
				if(ChildName.Find("EFFECT") >= 0)
				{
					int32 iEffect = ChildName.CopyFrom(6).Val_int();
					if (iEffect >= 0 && iEffect < NumChildren)
					{
						CStr Effect = pChild->GetThisValue();
						int iAttach = Effect.GetStrSep("#").Val_int();
						int iModel = m_pWServer->GetMapData()->GetResourceIndex_Model(Effect);
			//			ConOutL(CStrF("[%s] Effect %d, Attach:%d, Model:%d (%s)", GetItemName().Str(), iEffect, iAttach, iModel, pChild->GetThisValue().Str()));
						m_Model.m_ExtraModels.m_lEffects[iEffect].m_iAttachPoint = iAttach;
						m_Model.m_ExtraModels.m_lEffects[iEffect].m_iModel = iModel;
						m_Model.m_ExtraModels.m_liExtraModel[iEffect] = 0;
					}
					else
					{
						ConOutL(CStrF("ERROR: Effect index out of range! (%d, %d)", iEffect, NumChildren));
					}
				}
				else
				{
					ConOutL(CStrF("ERROR: Non-effect found in EFFECTLIST: '%s'", ChildName.Str()));
				}
			}
			break;
		}
	case MHASH2('NOLA','SER'): // "NOLASER"
		{
			m_Flags &= ~RPG_ITEM_FLAGS_LASERBEAM;
			break;
		}
	case MHASH3('JOUR', 'NALI', 'MAGE'):
		{
			MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
			if(pTC)
			{
				m_JournalImage = pTC->GetTextureID(KeyValue);
				if(m_JournalImage)
					pTC->SetTextureParam(m_JournalImage, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
			}
			break;
		}
	default:
		{
			if(KeyName.CompareSubStr("ATTACHMODEL") == 0)
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
			else if(KeyName.CompareSubStr("ICONSURFACE") == 0)
			{
				//if(CRPG_Object::m_bPrecacheForPlayerUse)
				m_iIconSurface = m_pWServer->GetMapData()->GetResourceIndex_Surface(KeyValue);
				CStr Upper = KeyValue.UpperCase();

				// Ugly hack to find out if we have the map or not!!!! REMOVE FIXME MOGG
				if (Upper.Find("GUI_ICON_MAP_PA2STORAGE01") != -1)
					m_iIconSurface |= 0x8000;
			}
			else if (KeyName.CompareSubStr("ACTIVATIONSURFACE") == 0)
			{
				m_iActivationSurface = m_pWServer->GetMapData()->GetResourceIndex_Surface(KeyValue);
			}
			else if(KeyName.CompareSubStr("SOUND_ANIM") == 0)
			{
				int Index = KeyName.Copy(10, 1024).Val_int() - 1;
				if(Index >= 0 && Index < RPG_ITEM_NUMANIMSOUNDS)
					m_iSound_Anim[Index] = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			}
			else
				return CRPG_Object::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
	
	return true;
}

//-------------------------------------------------------------------

void CRPG_Object_Item::OnFinishEvalKeys()
{
	MAUTOSTRIP(CRPG_Object_Item_OnFinishEvalKeys, MAUTOSTRIP_VOID);
	CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(m_pWServer);
	if (pServerMod)
	{
		pServerMod->AddAnimTypeItem(m_AnimType);
	}

	// If animtype hasn't been set, use itemtype
	if (m_AnimType == -1)
		m_AnimType = m_iItemType;
	if (m_GroupType == -1)
		m_GroupType = m_iItemType;
}

//-------------------------------------------------------------------

CFStr CRPG_Object_Item::GetItemName() const
{
	MAUTOSTRIP(CRPG_Object_Item_GetItemName, CFStr());
	CFStr Name;
	if(m_Name.CompareSubStr("pickup") == 0)
		Name = CFStr("§LITEM_") + CFStr(m_Name.Str() + 7);
	else
		Name = CFStr("§LITEM_") + CFStr(m_Name.Str());

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

CFStr CRPG_Object_Item::GetItemDesc() const
{
	MAUTOSTRIP(CRPG_Object_Item_GetItemDesc, CFStr());
	CFStr Desc;
	
	// If description is set, return that instead
	if (m_ItemDescription.Len())
		return m_ItemDescription;

	if(m_Name.CompareSubStr("pickup") == 0)
		Desc = CFStr("§LITEMDESC_") + CFStr(m_Name.Str() + 7);
	else
		Desc = CFStr("§LITEMDESC_") + CFStr(m_Name.Str());

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

bool CRPG_Object_Item::Block(bool _bAutoBlock, const CMat4Dfp32 &, CRPG_Object *_pRoot, int _iObject, int _iSender)
{
	MAUTOSTRIP(CRPG_Object_Item_Block, false);
	/*if(m_iAnimBlock > 0)
	{
		if(_pRoot->GetType() == CRPG_Object::TYPE_CHAR && ((CRPG_Object_Char *)_pRoot)->Wait() == 0)
		{
			int Ticks = PlayAnimSequence(_iObject, m_iAnimEquip, PLAYER_ANIM_ONLYTORSO);
			ApplyWait(_iObject, Ticks, m_NoiseLevel, m_Visibility);
		}
	}*/
	return false;
}

//-------------------------------------------------------------------

bool CRPG_Object_Item::OnProcess(CRPG_Object *_pRoot, int _iObject)
{
	MAUTOSTRIP(CRPG_Object_Item_OnProcess, false);
/*	if(m_Flags & RPG_ITEM_FLAGS_AUTORELOAD)
	{
		CRPG_Object_Ammo *pAmmo = GetAssociatedAmmo(_pRoot);
		if (pAmmo != NULL)
		{
			if(pAmmo->GetNumLoaded() == 0 && _pRoot->GetType() == CRPG_Object::TYPE_CHAR)
			{
				CWObject *pObj = m_pWServer->Object_Get(_iObject);
				if(((CRPG_Object_Char *)_pRoot)->Wait() == 0)
					Reload(&_Mat, _pRoot, _iObject);
			}
		}
	}*/
	
/*	
	if(m_PendingEquipUpdate != -1)
	{
		if(m_PendingEquipUpdate == 0)
		{
			if(_pRoot->GetType() == CRPG_Object::TYPE_CHAR)
			{
				if(((CRPG_Object_Char *)_pRoot)->Wait() == 0)
					Equip(_iObject, _pRoot);
			}
			else
				m_PendingEquipUpdate = -1;
		}
		else
		{
			int iTick = GetGameTick(_iObject);
			if(iTick >= m_PendingEquipUpdate)
			{
				m_PendingEquipUpdate = -1;
				if((m_iItemType >> 8) == 0)
					// This is a weapon
					SetAnimStance(_iObject, m_iAnimStance);
				
				SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL, m_iItemType >> 8);
			}
		}
	}
*/

	return CRPG_Object::OnProcess(_pRoot, _iObject);
}

//-------------------------------------------------------------------

bool CRPG_Object_Item::IsEquippable(CRPG_Object* _pRoot)
{
	MAUTOSTRIP(CRPG_Object_Item_IsEquippable, false);
	// For now, this function is only based on if there is any ammunition left.

	// First this was NEEDAMMO, then for some reason I changed it to NEEDLINK (can't remember why).
	// But that is not enough. Now it's back to NEEDAMMO.
	if (m_Flags & RPG_ITEM_FLAGS_NEEDAMMO)
	{
		CRPG_Object_Char* pChar = GetChar(_pRoot);
		if (pChar == NULL)
			return (m_NumItems > 0);

		if (pChar->m_bInsideEquipGUI)
			return true;

		CRPG_Object_Ammo* pAmmo = GetAssociatedAmmo(pChar);
		if (pAmmo == NULL)
			return (m_NumItems > 0);

		return (pAmmo->GetNumTotal() > 0);
	}

/*	if (m_Flags & RPG_ITEM_FLAGS_RENDERQUANTITY)
		return (m_NumItems > 0);*/

	return true;
}

//-------------------------------------------------------------------

bool CRPG_Object_Item::IsActivatable(CRPG_Object* _pRoot,int _iObject,int _Input)
{
	MAUTOSTRIP(CRPG_Object_Item_IsActivatable, false);
	if(m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), m_pWServer->Game_GetObjectIndex()))
	{
		CWObject_CharShapeshifter* pPlayerObj = safe_cast<CWObject_CharShapeshifter>(m_pWServer->Object_Get(_iObject));
		CWO_CharShapeshifter_ClientData& CD = pPlayerObj->GetClientData();
		if(CD.IsDarkling() && CD.m_State != DARKLING_STATE_NORMAL)
			return false;	
	}

	return true;
}

//-------------------------------------------------------------------

bool CRPG_Object_Item::Equip(int _iObject, CRPG_Object* _pRoot, bool _bInstant, bool _bHold)
{
	MAUTOSTRIP(CRPG_Object_Item_Equip, false);
	CRPG_Object_Char *pChar = GetChar(_pRoot);
	if (pChar == NULL)
		return false;

	int32 GameTick = GetGameTick(_iObject);
	m_LastEquipped = GameTick;
	// Meh, set animgraph to character animgraph instance
	CAutoVar_WeaponStatus Status;
	Status.m_CurrentTimeOut = m_iExtraActivationWait > GameTick ? m_iExtraActivationWait : 0;
	Status.m_FireTimeOut = m_FireTimeout;
	int16 iSound = m_iSound_Cast_Player > 0 ? m_iSound_Cast_Player : m_iSound_Cast;
	if (m_Flags2 & RPG_ITEM_FLAGS2_IAMACLONE)
	{
		iSound = m_iSound_Cast_LeftItem_Player > 0 ? m_iSound_Cast_LeftItem_Player : iSound;
	}
	Status.m_iSoundActivatePlayer = iSound;
	Status.m_AmmoLoad = m_Flags2 & RPG_ITEM_FLAGS2_DRAINSDARKNESS ? GetAmmoDraw() : GetAmmo(_pRoot,0);
	Status.m_Flags = Status.m_iSoundActivatePlayer != -1 ? CAutoVar_WeaponStatus::WEAPONSTATUS_HASSOUND : 0;
	Status.m_Flags |= m_Flags & RPG_ITEM_FLAGS_SINGLESHOT ? CAutoVar_WeaponStatus::WEAPONSTATUS_SINGLESHOT : 0;
	Status.m_Flags |= m_Flags2 & RPG_ITEM_FLAGS2_DRAINSDARKNESS ? CAutoVar_WeaponStatus::WEAPONSTATUS_DARKNESSDRAIN : 0;
	Status.m_Flags |= IsMeleeWeapon() ? CAutoVar_WeaponStatus::WEAPONSTATUS_MELEEWEAPON : 0;
	m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETWEAPONAG2,m_iAnimGraph, m_iItemType >> 8 > 0 ? 1: 0,-1,0,0,0,&Status,sizeof(CAutoVar_WeaponStatus)),_iObject);

	if (pChar->Wait() == 0)
	{
		if (!_bInstant && (m_iSound_Equip > 0))
		{
			CWObject *pObj = m_pWServer->Object_Get(_iObject);
			if (pObj)
				m_pWServer->Sound_At(pObj->GetPosition(), m_iSound_Equip, 0);
		}

		/*if ((m_iAnimEquip != -1) && !_bInstant)
		{
			if (_iPrecedingUnequipAnim == m_iAnimEquip)
			{
				// Don't do anything, merely wait the second half of the already playing animation.
				//ApplyWait(_iObject, m_SharedEquipAnimMidTicks, 0, 0);
				ApplyWait(_iObject, m_NumEquipTicks, 0, 0);
			}
			else
			{
			// URGENT FIXME: Flags was not initialized, I fixed this but this could destroy the current behavior, check with owner of code
				int Flags = 0;
				Flags |= PLAYER_ANIM_ONLYTORSO;
				Flags |= _bHold ? PLAYER_ANIM_HOLDATEND : 0;

				//int AnimDurationTicks = PlayAnimSequence(_iObject, m_iAnimEquip, Flags);
				PlayWeaponAnim(m_iAnimEquip,_iObject,SKELANIM_FLAG_BLEND);
				int AnimDurationTicks = m_NumEquipTicks;// m_SharedEquipAnimMidTicks;
				ApplyWait(_iObject, AnimDurationTicks, 0, 0);
				if((m_iItemType >> 8) < 2)
					SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL, m_iItemType >> 8, GetGameTick(_iObject) + m_NumEquipTicks);
			}
		}
		else*/
		{
			uint iSlot = (m_iItemType >> 8);
			if ((iSlot < 2) || (m_Flags2 & RPG_ITEM_FLAGS2_CLONEATTACHITEM))
			{
				SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL, m_iItemType >> 8, GameTick, m_AnimGripType);
			}
		}

		return true;
	}
	else
	{
		return false;
	}

/*
	if (pChar->Wait() > 0)
		m_PendingEquipUpdate = 0;
	else
	{
		m_Flags |= RPG_ITEM_FLAGS_EQUIPPED;
		if (m_iSound_Equip != -1)
		{
			CWObject *pObj = m_pWServer->Object_Get(_iObject);
			if(pObj)
				m_pWServer->Sound_At(pObj->GetPosition(), m_iSound_Equip, 0);
		}

		int Ticks = PlayAnimSequence(_iObject, m_iAnimEquip, true);
	
		ApplyWait(_iObject, Ticks);

		SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL, m_iItemType >> 8, GetGameTick(_iObject) + UPDATEITEMMODELTICK);
		m_PendingEquipUpdate = -1;
	}
	return true;
*/	
}

//-------------------------------------------------------------------

bool CRPG_Object_Item::Unequip(int _iObject, CRPG_Object* _pRoot, bool _bInstant, bool _bHold)
{
	MAUTOSTRIP(CRPG_Object_Item_Unequip, false);
	CRPG_Object_Char *pChar = GetChar(_pRoot);
	if (pChar == NULL)
		return false;

	// Turn flashlight off when unequipping
	m_Flags &= ~RPG_ITEM_FLAGS_FLASHLIGHT;

	//CRPG_Object_Ammo* pAmmo = GetAssociatedAmmo(_pRoot);
	//if ((pAmmo != NULL) && (m_Flags & RPG_ITEM_FLAGS_UNLOADONUNEQUIP))
	//	pAmmo->Unload();
	// Reset ag
	m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETWEAPONAG2, 0, m_iItemType >> 8 > 0 ? 1: 0),_iObject);

	if (pChar->Wait() == 0)
	{
		if (!_bInstant && (m_iSound_Unequip > 0))
		{
			CWObject *pObj = m_pWServer->Object_Get(_iObject);
			if (pObj)
				m_pWServer->Sound_At(pObj->GetPosition(), m_iSound_Unequip, 0);
		}

		/*if ((m_iAnimEquip != -1) && !_bInstant)
		{
			// URGENT FIXME: Flags was not initialized, I fixed this but this could destroy the current behavior, check with owner of code
			int Flags = 0;
			Flags |= PLAYER_ANIM_ONLYTORSO;
			Flags |= _bHold ? PLAYER_ANIM_HOLDATEND : 0;

			//if (_iSucceedingEquipAnim == m_iAnimEquip)
			int ChangeItemDelayTicks = 0;//m_NumUnequipTicks;

			ApplyWait(_iObject, ChangeItemDelayTicks, 0, 0);
			int32 ModelUpdateTick = GetGameTick(_iObject) + (m_Flags & RPG_ITEM_FLAGS_REMOVEINSTANT ? 0 : m_NumUnequipTicks);
			if((m_iItemType >> 8) < 2)
				SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL, m_iItemType >> 8, ModelUpdateTick);
		}
		else*/
		{
			// Update instantly if there are no succeeding equip anim.
			// If there is a succeeding equip anim, that item will update the models.
			// Update after estimate of equip/unequip animation has ended 
			// (only for "weapons" not unarmed)
			int32 ModelUpdateTick = GetGameTick(_iObject) + (m_Flags & RPG_ITEM_FLAGS_REMOVEINSTANT ? 0 : m_NumUnequipTicks);
			if((m_iItemType >> 8) < 2)
				SendMsg(_iObject, OBJMSG_CHAR_UPDATEITEMMODEL, m_iItemType >> 8, ModelUpdateTick);
		}

		//If item should be dropped when unequipping, tell object to do so
		//Currently we can only drop weapons
		//This is horribly ugly and propably unsafe, but I don't want to rewrite drop item message to be sufficiently general right now...
		if ((m_Flags & RPG_ITEM_FLAGS_DROPONUNEQUIP) &&
			((m_iItemType >> 8) == RPG_CHAR_INVENTORY_WEAPONS))
		{
			// Should check if this item is currenlty the "hold down" item
			// ie just lowering the weapon in wait to be used again
			if (!(SendMsg(_iObject, OBJMSG_CHAR_GETLASTWEAPON) == m_Identifier))
				SendMsg(_iObject, OBJMSG_CHAR_FORCEDROPWEAPON);
		}

		return true;
	}
	else
	{
		return false;
	}

/*
	if (m_Flags & RPG_ITEM_FLAGS_EQUIPPED)
	{
		m_Flags &= ~RPG_ITEM_FLAGS_EQUIPPED;

		if(pChar->Wait() == 0)
		{
			if(m_iSound_Unequip != -1)
			{
				CWObject *pObj = m_pWServer->Object_Get(_iObject);
				if(pObj)
					m_pWServer->Sound_At(pObj->GetPosition(), m_iSound_Unequip, 0);
			}
			int Ticks = PlayAnimSequence(_iObject, m_iAnimEquip, true);
			ApplyWait(_iObject, Ticks);
		}

		m_PendingEquipUpdate = -1;
	}

	return true;
*/
}

//-------------------------------------------------------------------

bool CRPG_Object_Item::MergeItem(int _iObject, CRPG_Object_Item *_pObj)
{
	MAUTOSTRIP(CRPG_Object_Item_MergeItem, false);
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

int CRPG_Object_Item::OnPickup(int _iObject, CRPG_Object *_pRoot, bool _bNoSound, int _iSender, bool _bNoPickupIcon)
{
	MAUTOSTRIP(CRPG_Object_Item_OnPickup, MAUTOSTRIP_VOID);
	if(_iObject > 0 && m_iSound_Pickup != -1 && !_bNoSound)
	{
		CWObject *pObj = m_pWServer->Object_Get(_iObject);
		if(pObj)
			m_pWServer->Sound_At(pObj->GetPosition(), m_iSound_Pickup, 0);
	}
	return true;
}

//-------------------------------------------------------------------

bool CRPG_Object_Item::CanPickup()
{
	// Hmm, anything here...?
	return true;
}

//-------------------------------------------------------------------

void CRPG_Object_Item::OnDeltaLoad(CCFile* _pFile, CRPG_Object* _pCharacter)
{
	int32 Num;
	_pFile->ReadLE(Num);
	m_NumItems = Num;
	_pFile->ReadLE(Num);
	m_Identifier = Num;

	// Do we need to read in item description?
	uint8 Flags = 0;
	_pFile->ReadLE(Flags);
	if(Flags & 1)
	{
		m_ItemDescription = _pFile->Readln();
		M_TRACEALWAYS("ItemDescription loaded: %s\n", m_ItemDescription.GetStr());
	}
	if (Flags & 2)
	{
		// In in armor inventory (dual wield) modify stuff
		m_Model.m_iAttachRotTrack = 23;
		m_Model.m_iAttachPoint[0] = 1;
		m_iItemType |= 0x200;
		m_Flags2 |= RPG_ITEM_FLAGS2_IAMACLONE;
	}
}

//-------------------------------------------------------------------

void CRPG_Object_Item::OnDeltaSave(CCFile* _pFile, CRPG_Object* _pCharacter)
{
	int32 Num = m_NumItems;
	_pFile->WriteLE(Num);
	Num = m_Identifier;
	_pFile->WriteLE(Num);

	// Do we need to write down item description?
	uint8 Flags = (m_ItemDescription.Len()) ? 1 : 0;
	if (m_iItemType >> 8 == 2)
	{
		// I'm in armor
		Flags |= 2;
	}
	_pFile->WriteLE(Flags);
	if(Flags & 1)
	{
		_pFile->Writeln(m_ItemDescription.GetStr());
		M_TRACEALWAYS("ItemDescription saved: %s\n", m_ItemDescription.GetStr());
	}
}

//-------------------------------------------------------------------

bool CRPG_Object_Item::NeedsDeltaSave(CRPG_Object* _pCharacter)
{
	if (m_SaveMask == 0)
		return false;

	return true;
}

uint8 CRPG_Object_Item::GetRefreshFlags(uint8 _Flags)
{
	MAUTOSTRIP(CRPG_Object_Item_GetRefreshFlags, 0);
	// These flags are kept from being sent every frame (all this trouble has to do with item changes)
	int Mask = (RPG_ITEM_FLAGS_NORENDERMODEL | RPG_ITEM_FLAGS_EQUIPPED | RPG_ITEM_FLAGS_AIMING);
	
	_Flags = (m_Flags & (~Mask)) | (_Flags & Mask);
	return _Flags;
}

//-------------------------------------------------------------------

bool CRPG_Object_Item::ApplyWait(int _iObject, int _Ticks, fp32 _NoiseLevel, fp32 _Visibility)
{
	MAUTOSTRIP(CRPG_Object_Item_ApplyWait, false);
	SendMsg(_iObject, OBJMSG_CHAR_RAISENOISELEVEL, (int)_NoiseLevel);
	SendMsg(_iObject, OBJMSG_CHAR_RAISEVISIBILITY, (int)_Visibility);
	int32 iCurrentTick = GetGameTick(_iObject);
	if (_Ticks == -1)
		m_iExtraActivationWait = iCurrentTick + 10000; // Lock
	else if (_Ticks == 0)
		m_iExtraActivationWait = 0; // Unlock
	else if( _Ticks < -1)
		m_iExtraActivationWait = iCurrentTick - _Ticks; // Set
	else
	{
		if (m_iExtraActivationWait > iCurrentTick)
			m_iExtraActivationWait += _Ticks;// Increase
		else
			m_iExtraActivationWait = iCurrentTick + _Ticks;
	}
	// Since we can have dualwield, change apply wait...?
	//return SendMsg(_iObject, OBJMSG_CHAR_APPLYWAIT, _Ticks) != 0;
	return true;
}

//-------------------------------------------------------------------

bool CRPG_Object_Item::SetWait(int _iObject, int _Ticks)
{
	MAUTOSTRIP(CRPG_Object_Item_SetWait, false);
	return SendMsg(_iObject, OBJMSG_CHAR_SETWAIT, _Ticks) != 0;
}

//-------------------------------------------------------------------

int CRPG_Object_Item::PlayAnimSequence(int _iObject, int _iAnim, int _Flags)
{
	MAUTOSTRIP(CRPG_Object_Item_PlayAnimSequence_2, 0);
	//if(_iAnim != -1)
	//	return SendMsg(_iObject, OBJMSG_CHAR_PLAYANIMSEQUENCE, _iAnim, _Flags);
	return 0;
}

//-------------------------------------------------------------------

void CRPG_Object_Item::SetAnimStance(int _iObject, int _iAnim)
{
	MAUTOSTRIP(CRPG_Object_Item_SetAnimStance, MAUTOSTRIP_VOID);
	SendMsg(_iObject, OBJMSG_CHAR_SETANIMSTANCE, _iAnim);
}

//-------------------------------------------------------------------

int CRPG_Object_Item::SendDamage(int _iObject, const CVec3Dfp32& _Pos, int _iSender, int _Damage, uint32 _DamageType, int _SurfaceType, const CVec3Dfp32 *_pForce, const CVec3Dfp32 *_pSplatterDir, CCollisionInfo* _pCInfo, const char* _pDamageEffect, int _StunTicks)
{
	MAUTOSTRIP(CRPG_Object_Item_SendDamage, 0);
	fp32 DamageBoost = 1.0f;
	if (m_iSkill != -1)
	{
		int Res = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETDAMAGEBOOST, m_iSkill), _iSender);
		if(Res != 0)
			DamageBoost = fp32(Res) / 256;
	}

	if (_Damage > 0)
	{
		if(_pForce)
		{
			CVec3Dfp32 Temp = (*_pForce * DamageBoost);

			CWO_DamageMsg Msg((int)(_Damage * DamageBoost), _DamageType, &_Pos, &Temp, _pSplatterDir, _pCInfo, _SurfaceType, _pDamageEffect, _StunTicks);
			return Msg.Send(_iObject, _iSender, m_pWServer);
		}
		else
		{
			CWO_DamageMsg Msg((int)(_Damage * DamageBoost), _DamageType, &_Pos, NULL, _pSplatterDir, _pCInfo, _SurfaceType, _pDamageEffect, _StunTicks);
			return Msg.Send(_iObject, _iSender, m_pWServer);
		}
	}
	return 0;
}

//-------------------------------------------------------------------

CRPG_Object_Char* CRPG_Object_Item::GetChar(CRPG_Object* _pRoot)
{
	MAUTOSTRIP(CRPG_Object_Item_GetChar, NULL);
	if ((_pRoot == NULL) || (_pRoot->GetType() != TYPE_CHAR))
		return NULL;

	//JK-FIX: Don't do this using TDynamicCast
	CRPG_Object_Char* pChar = TDynamicCast<CRPG_Object_Char>(_pRoot);

	return pChar;
}

//-------------------------------------------------------------------

int CRPG_Object_Item::GetCurLinkItemType()
{
	MAUTOSTRIP(CRPG_Object_Item_GetCurLinkItemType, 0);
	if(m_NumLinks == 0)
		return -1;

	return m_lLinkedItemTypes[m_iCurLink];
}

//-------------------------------------------------------------------

CRPG_Object_Ammo* CRPG_Object_Item::GetAssociatedAmmo(CRPG_Object *_pRoot)
{
	MAUTOSTRIP(CRPG_Object_Item_GetAssociatedAmmo, NULL);
	CRPG_Object_Char *pChar = GetChar(_pRoot);
	if (pChar == NULL)
		return NULL;

	return pChar->GetAssociatedAmmo(m_iItemType);
}

//-------------------------------------------------------------------

CRPG_Object_Item* CRPG_Object_Item::GetCurLinkedItem(CRPG_Object *_pRoot)
{
	MAUTOSTRIP(CRPG_Object_Item_GetCurLinkedItem, NULL);
	CRPG_Object_Char *pChar = GetChar(_pRoot);
	if (pChar == NULL)
		return NULL;

	int iType = GetCurLinkItemType();
	
	return pChar->FindItemByType(iType);
}

//-------------------------------------------------------------------

bool CRPG_Object_Item::HasLinkedItemType(int _ItemType)
{
	MAUTOSTRIP(CRPG_Object_Item_HasLinkedItemType, false);
	for(int i = 0; i < m_NumLinks; i++)
		if(m_lLinkedItemTypes[i] == _ItemType)
			return true;

	return false;
}

//-------------------------------------------------------------------
/*
bool CRPG_Object_Item::Reload(const CMat4Dfp32 *_Mat, CRPG_Object *_pRoot, int _iObject, bool _bApplyAnim)
{
	MAUTOSTRIP(CRPG_Object_Item_Reload, false);
	CRPG_Object_Ammo *pAmmo = GetAssociatedAmmo(_pRoot);
	if (pAmmo == NULL)
		return (m_NumItems > 0);

	if (pAmmo->RefillMagazine() > 0)
	{
		if(_bApplyAnim)
		{
			if(m_iSound_Reload != -1 && _Mat)				
				m_pWServer->Sound_At(CVec3Dfp32::GetMatrixRow(*_Mat, 3), m_iSound_Reload, 0);

			int Ticks = 0;
			int iAnim = 0;

			PlayWeaponAnim(m_iAnimReload,CMTime::CreateFromTicks(GetGameTick(_iObject), SERVER_TIMEPERFRAME),_iObject,SKELANIM_FLAG_BLEND);
				
			ApplyWait(_iObject, Ticks + 6, 0, 0);
		}

		SAVEMASK_SETBIT(LOADSAVEMASK_ASSOCIATEDAMMO);	// Associatedammo have been changed, save to memorycard
		return true;
	}

	return false;
}
*/
//-------------------------------------------------------------------

bool CRPG_Object_Item::CheckAmmo(CRPG_Object *_pRoot)
{
	MAUTOSTRIP(CRPG_Object_Item_CheckAmmo, false);
	if(!(m_Flags & RPG_ITEM_FLAGS_NEEDAMMO))
		return true;

	CRPG_Object_Ammo *pAmmo = GetAssociatedAmmo(_pRoot);
	if (pAmmo != NULL)
		return (pAmmo->GetNumLoaded() > 0);

	return (m_NumItems > 0);
}

//-------------------------------------------------------------------

int CRPG_Object_Item::GetAmmo(CRPG_Object* _pRoot, int _iType)
{
	CRPG_Object_Ammo *pAmmo = GetAssociatedAmmo(_pRoot);
	if (pAmmo != NULL)
		return pAmmo->GetNumLoaded();

	return m_NumItems;
}

int CRPG_Object_Item::GetMagazines(void)
{
	return 0;
}

//-------------------------------------------------------------------

bool CRPG_Object_Item::DrawAmmo(int _Num, const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, bool _bApplyFireTimeout)
{
	MAUTOSTRIP(CRPG_Object_Item_DrawAmmo, false);
	CRPG_Object_Ammo *pAmmo = GetAssociatedAmmo(_pRoot);
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
		if(m_FireTimeout == -1 && Ticks > 0)
			ApplyWait(_iObject, Ticks, m_NoiseLevel, m_Visibility);
	}

	if ((m_Flags & RPG_ITEM_FLAGS_QUESTITEM) /*&& (m_Flags & RPG_ITEM_FLAGS_RENDERQUANTITY)*/)
		SendMsg(_iObject, OBJMSG_CHAR_UPDATEQUESTICONS);

	if(m_Flags2 & RPG_ITEM_FLAGS2_FORCESHELLEMIT && (m_iShellType >= 0))
	{
		// Force a shell to emit when weapon goes off
		CWO_ShellManager* pShellMgr = (CWO_ShellManager*)m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETSHELLMANAGER), m_pWServer->Game_GetObjectIndex());
		if(pShellMgr)
		{
			CWO_Shell_SpawnData SpawnData;
			GetShellSpawnData(_iObject, &SpawnData);
			pShellMgr->SpawnShell(_iObject, m_iShellType, SpawnData);
		}
	}

	return false;
}

//-------------------------------------------------------------------

bool CRPG_Object_Item::Activate(const CMat4Dfp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	MAUTOSTRIP(CRPG_Object_Item_Activate, false);
/*	if(!CheckAmmo(_pRoot))
		return Reload(&_Mat, _pRoot, _iObject);		// SAVE: Only affects the GetAssociatedAmmo CRPG_Object_Ammo class*/
	
//	int iObj = -1;
	if(OnActivate(_Mat, _pRoot, _iObject, _Input))	// SAVE: Nothing in CRPG_Object_Item is affected by this
	{
		if(m_Flags & RPG_ITEM_FLAGS_NEEDAMMO)
			DrawAmmo(m_AmmoDraw, _Mat, _pRoot, _iObject);	// SAVE: If this object doesn't have an associated ammo class DrawAmmo reduce m_NumItems by one.
		if(m_iSound_Cast != -1)
			m_pWServer->Sound_At(CVec3Dfp32::GetMatrixRow(_Mat, 3), m_iSound_Cast, 0);
	}
	
	return true;
}

//-------------------------------------------------------------------

int CRPG_Object_Item::ResolveAnimHandle(const char *_pSequence)
{
	MAUTOSTRIP(CRPG_Object_Item_ResolveAnimHandle, 0);
	CWObject_Message Msg(OBJMSG_GAME_RESOLVEANIMHANDLE);
	Msg.m_pData = (void *)_pSequence;
	return m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());
}

//-------------------------------------------------------------------

void CRPG_Object_Item::Line(const CVec3Dfp32& _Pos0, const CVec3Dfp32& _Pos1, int _Col)
{
	MAUTOSTRIP(CRPG_Object_Item_Line, MAUTOSTRIP_VOID);
	m_pWServer->Debug_RenderWire(_Pos0, _Pos1, _Col, 5);
}

void CRPG_Object_Item::GetShellSpawnData(int _iObject, CWO_Shell_SpawnData* _pSpawnData)
{
	_pSpawnData->m_AttachMat = GetCurAttachOnItemMatrix(_iObject, m_iShellAttach);
}

CMat4Dfp32 CRPG_Object_Item::GetAttachOnCharMatrix(CAttachInfo &_AttachInfo)
{
	int iRotTrack = _AttachInfo.m_iCharRotTrack;

	CMat4Dfp32 Pos;
	const CXR_SkeletonAttachPoint* pPoint = _AttachInfo.m_pCharSkeleton->GetAttachPoint(_AttachInfo.m_iCharAttachPoint);
	if (pPoint)
	{
		// This is a hack for old data... 
		if (iRotTrack == 22 && pPoint->m_iNode == 0 && pPoint->m_LocalPos.GetRow(3).LengthSqr() > 0.01f)
		{
			Pos = _AttachInfo.m_pSkelInstance->m_pBoneTransform[22];
			Pos.GetRow(3) = pPoint->m_LocalPos.GetRow(3) * _AttachInfo.m_pSkelInstance->m_pBoneTransform[16];
		}
		else
		{
			if (pPoint->m_iNode)
				iRotTrack = pPoint->m_iNode;
			const CMat4Dfp32& BoneTransform = _AttachInfo.m_pSkelInstance->m_pBoneTransform[iRotTrack];
			pPoint->m_LocalPos.Multiply(BoneTransform, Pos);
		}
	}
	else
	{
		Pos = _AttachInfo.m_pSkelInstance->m_pBoneTransform[iRotTrack];
		Pos.GetRow(3) = CVec3Dfp32(0) * Pos;
	}
	return Pos;
}

CMat4Dfp32 CRPG_Object_Item::GetItemOnCharMatrix(CAttachInfo &_AttachInfo)
{
	CMat4Dfp32 Attach = GetAttachOnCharMatrix(_AttachInfo);
	CMat4Dfp32 Res;
	const CXR_SkeletonAttachPoint *pItemHandle = _AttachInfo.m_pItemSkeleton ? _AttachInfo.m_pItemSkeleton->GetAttachPoint(0) : NULL;
	if(pItemHandle)
	{
		CMat4Dfp32 Handle;
		pItemHandle->m_LocalPos.InverseOrthogonal(Handle);
		Handle.Multiply(Attach, Res);
	}
	else
		Res = Attach;

	return Res;
}

CMat4Dfp32 CRPG_Object_Item::GetAttachOnItemMatrix(CAttachInfo &_AttachInfo)
{
	CMat4Dfp32 Res = GetItemOnCharMatrix(_AttachInfo);
	const CXR_SkeletonAttachPoint *pAttach = _AttachInfo.m_pItemSkeleton ? _AttachInfo.m_pItemSkeleton->GetAttachPoint(_AttachInfo.m_iItemAttachPoint) : NULL;
	if (pAttach)
	{
		CMat4Dfp32 Tmp;
		pAttach->m_LocalPos.Multiply(Res, Tmp);
		return Tmp;
	}
	else if (_AttachInfo.m_pSkelInstance && _AttachInfo.m_pCharSkeleton)
	{
		CVec3Dfp32 Temp = _AttachInfo.m_pCharSkeleton->m_lNodes[_AttachInfo.m_iCharRotTrack].m_LocalCenter;
		CMat4Dfp32 Trans;
		Trans = _AttachInfo.m_pSkelInstance->m_pBoneTransform[_AttachInfo.m_iCharRotTrack];
		Temp *= Trans;
		Res.GetRow(3) = Temp;
	}
	return Res;
}

CMat4Dfp32 CRPG_Object_Item::GetAttachMatrix(int _iObject, int _iCharRottrack, int _iCharAttachPoint, int _iItemAttachPoint)
{
	CWObject *pObj = m_pWServer->Object_Get(_iObject);
	if(!pObj)
	{
		CMat4Dfp32 Unit;
		Unit.Unit();
		return Unit;
	}

	CXR_AnimState AnimState;
	CWObject_Message Msg(OBJSYSMSG_GETPHYSANIMSTATE);
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
	
CMat4Dfp32 CRPG_Object_Item::GetCurAttachOnItemMatrix(int _iObject)
{
	CMat4Dfp32 Res = GetAttachMatrix(_iObject, m_Model.m_iAttachRotTrack, m_Model.m_iAttachPoint[0], m_iMuzzleAttachPoint);
	return Res;
}

CMat4Dfp32 CRPG_Object_Item::GetCurAttachOnItemMatrix(int _iObject, int _iAttachPoint)
{
	CMat4Dfp32 Res = GetAttachMatrix(_iObject, m_Model.m_iAttachRotTrack, m_Model.m_iAttachPoint[0], _iAttachPoint);
	return Res;
}

bool CRPG_Object_Item::UpdateAmmoBarInfo(CRPG_Object *_pRoot, uint16 &_Surface1, uint16 &_Surface2, uint8 &_NumTotal, uint8 &_NumLoaded, uint32 &_Extra)
{
	MAUTOSTRIP(CRPG_Object_Item_UpdateAmmoBarInfo, false);
	CRPG_Object_Char *pChar = GetChar(_pRoot);
	if (pChar == NULL)
		return false;
	
	CRPG_Object_Ammo* pAmmo = pChar->GetAssociatedAmmo();
	if (pAmmo == NULL)
		return false;

	_Surface1 = (uint16)pAmmo->m_iAmmoSurface1;
	_Surface2 = (uint16)pAmmo->m_iAmmoSurface2;
	_NumTotal = Min(int(255 * (fp32)pAmmo->GetNumTotal() / (fp32)pAmmo->GetMaxTotal()), 255);
	_NumLoaded = Min(pAmmo->GetNumLoaded(), 255);
	_Extra = 0;
	return true;
}

void CRPG_Object_Item::TagAnimationsForPrecache(CWAG2I_Context* _pContext, CWAG2I* _pAgi)
{
	// From itemtype
	TArray<CXRAG2_Impulse> lImpulses;
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,m_AnimType));
	// Crouched
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,m_AnimType + AG2_IMPULSEVALUE_NUMWEAPONTYPES));
		
	_pAgi->TagAnimSetFromImpulses(_pContext, m_pWServer->GetMapData(), m_pWServer->m_spWData,lImpulses);
}

//-------------------------------------------------------------------

bool CRPG_Object_Item::CanUse(int32 _iChar)
{
	return true;
}

//-------------------------------------------------------------------

void CRPG_Object_Item::UpdateAmmoList(CRPG_Object *_pRoot)
{
	MAUTOSTRIP(CRPG_Object_Item_UpdateAmmoList, MAUTOSTRIP_VOID);
//	Reload(NULL, _pRoot, false);
}

//-------------------------------------------------------------------

//Get the number of extra magazines. Return -1 if we don't need/use extra magazines.
int CRPG_Object_Item::ShowNumMagazines()
{
	return -1;
};


//-------------------------------------------------------------------

int CRPG_Object_Item::GetGameTick(int _iObject)
{
	MAUTOSTRIP(CRPG_Object_Item_GetGameTick, 0);
	int iTick = SendMsg(_iObject, OBJMSG_CHAR_GETGAMETICK);
	if(!iTick)
		iTick = m_pWServer->GetGameTick();

	return iTick;
}

//-------------------------------------------------------------------

//Returns the speed (in world units per server frame) of any projectiles the item can summon, or
//_FP32_MAX if these projectiles are instantaneous. If the object does not summon projectiles, 0 is returned.
fp32 CRPG_Object_Item::GetProjectileSpeed()
{
	MAUTOSTRIP(CRPG_Object_Item_GetProjectileSpeed, 0.0f);
	return 0;
};

//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Returns the color of the muzzle, if weapon has no muzzle flame, color is zero
CVec3Duint8 CRPG_Object_Item::GetMuzzleColor()
{
	return CVec3Duint8(0);
}

//-------------------------------------------------------------------
CWO_Damage * CRPG_Object_Item::GetDamage()
{
	return NULL;
}
//-------------------------------------------------------------------

//-------------------------------------------------------------------
bool CRPG_Object_Item::IsMeleeWeapon()
{
	return false;
}
//-------------------------------------------------------------------

//-------------------------------------------------------------------
int CRPG_Object_Item::GetRequiredClearanceLevel()
{
	return m_RequiredClearance;
}
//-------------------------------------------------------------------

int CRPG_Object_Item::GetGivenClearanceLevel()
{
	return m_GivenClearanceLevel;
}


//-------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Item, CRPG_Object);

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CRPG_Object_Collectible

	Comments:		
\*____________________________________________________________________*/

class CRPG_Object_Collectible : public CRPG_Object_Item
{
	MRTC_DECLARE;

public:
	int m_ID;
	virtual bool OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
	{
		CStr Key = _pKey->GetThisName();
		if(Key == "COLLECTIBLEID")
			m_ID = _pKey->GetThisValuei();
		
		else
			return CRPG_Object_Item::OnEvalKey(_KeyHash, _pKey);
		
		return true;
	}

	virtual int OnPickup(int _iObject, CRPG_Object *_pRoot, bool _bNoSound, int _iSender, bool _bNoPickupIcon)
	{
		CRPG_Object_Item::OnPickup(_iObject, _pRoot, _bNoSound, _iSender, _bNoPickupIcon);

		CWObject_Message Msg(OBJMSG_GAME_SHOWGAMEMSG, 0, 0, _iSender, 1);
		CStr St = CStrF("§LITEM_COLLECTIBLE|$%i,192,256,§C888§LCOLLECTIBLE_STATUS§p0%i§pq||", m_iIconSurface, m_ID + 1);
		CStr St2 = St;

		MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
		M_ASSERT(pGame != NULL, "!");
		CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
		TArray<CExtraContentHandler::CContent *> lpContent;
		if(pGameMod->m_ExtraContent.AddKeyBit(m_ID, &lpContent))
		{
			int j = 0;
			for(int i = 0; i < lpContent.Len(); i++)
			{
				if(j > 2 || (j == 2 && i == lpContent.Len() - 1))
				{
					Msg.m_pData = (void*)St.Str();
					m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());
					St = St2;
					j = 0;
				}
				St += "§Lcollectible_unlock§p0§C884" + lpContent[i]->m_Name + "§C888§pq||";
				j++;
			}
			if(lpContent.Len() > 1)
				St += "§Lcollectible_multiple";
			else if(lpContent.Len() > 0)
				St += "§Lcollectible_single";
			ConExecute("saveprofile()");
		}
		else
			St += "§Lcollectible_dup";

		Msg.m_pData = (void*)St.Str();
		m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());
		SendMsg(_iObject, OBJMSG_CHAR_DESTROYITEM, 0, 0, _iSender, 0, 0, 0, (void *)m_Name.Str());

		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETNEXTGUISCREEN, 3), _iObject);
		return 2;
	}

	virtual CFStr GetItemName()
	{
		return "§LITEM_COLLECTIBLE";
	}

	virtual bool MergeItem(int _iObject, CRPG_Object_Item *_pObj)
	{
		return true;
	}
};

MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_Collectible, CRPG_Object_Item);
