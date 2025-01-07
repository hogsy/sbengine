#include "PCH.h"

#include "WObj_Char.h"
#include "WRPG/WRPGChar.h"
#include "WObj_AI/AICore.h"
#include "WObj_Game/WObj_GameMod.h"
#include "WObj_Misc/WObj_ActionCutscene.h"
#include "../GameWorld/WClientMod_Defines.h"
#include "CConstraintSystem.h"
#include "WObj_Misc/WObj_TelePhone.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Character file and network IO.
					
	Contents:		OnLoad
					OnSave
					OnCreateClientUpdate
					OnClientUpdate
					OnClientLoad
					OnClientSave

\*____________________________________________________________________________________________*/

void CWObject_Character::OnLoad(CCFile* _pFile)
{
	CWObject_RPG::OnLoad(_pFile);
}

void CWObject_Character::OnSave(CCFile* _pFile)
{
	CWObject_RPG::OnSave(_pFile);
}

//
// Player character format
//
// CMat4Dfp32							Position
// for i=0 to RPG_CHAR_NUMCHILDREN		(4)
// {
//   int								Number of objects in inventory i
//   for o=0 to NumberObj				Loop through all objects in inventory i
//   {
//     CFStr							Class name of object o
//     [unknown]						Class specific object data
//   }
// }
//

class CBitFieldReader
{
public:
	TArray<bool> m_lData;
	int m_CurrentBit;

	CBitFieldReader() : m_CurrentBit(0) {}

	void AdM_Bit(bool _bit)
	{
		m_lData.Add(_bit);
	}

	bool GetBit()
	{
		return m_lData[m_CurrentBit++];
	}
};

#define MACRO_BITFIELD_DECLARE(_id) CBitFieldReader Bitfield##_id;
#define MACRO_BITFIELD_READ(_id) Bitfield_Read(Bitfield##_id.m_lData, _pFile)
#define MACRO_BITFIELD_WRITE(_id) Bitfield_Write(Bitfield##_id.m_lData, _pFile)
#define MACRO_BITFIELD_REAM_Bit(_id, _var) _var = (uint8)Bitfield##_id.GetBit()
#define MACRO_BITFIELD_WRITEBIT(_id, _data) Bitfield##_id.AdM_Bit(_data==1?true:false)

// Please replace this badly written bitfield stuff. Also, macros like this shouldn't be needed for the IO.
//#define MACRO_BITFIELD_DECLARE(_id) TArray<bool> Bitfield##_id; int BitfieldRead##_id = 0;
//#define MACRO_BITFIELD_WRITEBIT(_id, _data) Bitfield##_id.Add(_data==1?true:false)
//#define MACRO_BITFIELD_WRITE(_id) Bitfield_Write(Bitfield##_id, _pFile)
//#define MACRO_BITFIELD_REAM_Bit(_id, _var) _var = (uint8)Bitfield##_id[BitfieldRead##_id++];
//#define MACRO_BITFIELD_READ(_id) Bitfield_Read(Bitfield##_id, _pFile)

#define MACRO_WRITE(_type, _data) {_type TempApa = _data; _pFile->WriteLE(TempApa);}
#define MACRO_READ(_type, _var) {_type TempApa; _pFile->ReadLE(TempApa); _var = TempApa; }

#ifdef _DEBUG
#define MACRO_WRITEVERIFY {uint32 Apa = 0x81920467; _pFile->WriteLE(Apa);}
#define MACRO_READVERIFY {uint32 Apa; _pFile->ReadLE(Apa); M_ASSERT(Apa == 0x81920467, CStrF("Load/save mismatch in file '%s' on line %i", __FILE__, __LINE__)); };
#else
#define MACRO_WRITEVERIFY
#define MACRO_READVERIFY
#endif


//#define VERBOSE

#define VERBOSE_OUT(_Message)
#ifdef VERBOSE
#ifdef _DEBUG
	#define VERBOSE_OUT(_Message) ConOutL(_Message)
#endif	// _DEBUG
#endif	// VERBOSE

void Bitfield_Write(TArray<bool>& _Bitfield, CCFile* _pFile)
{
	int32 nBits = _Bitfield.Len();
	int32 nIntsToWrite = (nBits / 32)+1;
	M_ASSERT(nIntsToWrite<256, "Bitfield depleted. This will not happen. You are not reading this message.");

	MACRO_WRITE(uint8, nIntsToWrite);
	int32 iBit = 0;
	int32 iInt;
	for (iInt=0; iInt<nIntsToWrite; iInt++)
	{
		uint32 Bitfield = 0;
		for (int iLocalBit=0; iLocalBit<32; iLocalBit++)
		{
			bool bTemp = false;
			if (_Bitfield.ValidPos(iBit))
				bTemp = _Bitfield[iBit];
			iBit++;
			Bitfield |= bTemp << iLocalBit;
		}

		MACRO_WRITE(uint32, Bitfield);
	}
}

void Bitfield_Read(TArray<bool>& _Bitfield, CCFile* _pFile)
{
	_Bitfield.Clear();

	uint8 nIntsToRead;
//	int32 iBit = 0;
	int32 iInt;
	MACRO_READ(uint8, nIntsToRead);
	for (iInt=0; iInt<nIntsToRead; iInt++)
	{
		uint32 Bitfield;
		MACRO_READ(uint32, Bitfield);
		for (int iLocalBit=0; iLocalBit<32; iLocalBit++)
		{
			bool bBit = (Bitfield >> iLocalBit) == 1?true:false;
			_Bitfield.Add(bBit);
		}
	}
}



#define MACRO_FLAGCHANGED_TOTRUE(_previous, _loaded, _flag) (((_previous & _flag) != _flag) && ((_loaded & _flag) == _flag))
#define MACRO_FLAGCHANGED_TOFALSE(_previous, _loaded, _flag) (((_previous & _flag) == _flag) && ((_loaded & _flag) != _flag))

//Optional save data flags. Currently only used in CWObject_Character::OnDeltaLoad/Save but
//we should expand this to include all character member classes' optional save data as well
enum 
{
	PLAYER_OPTIONALSAVEFLAGS_DIALOGUEINSTANCE	= M_Bit(0),
	PLAYER_OPTIONALSAVEFLAGS_OWNER				= M_Bit(1),
	PLAYER_OPTIONALSAVEFLAGS_EXTRAITEM			= M_Bit(2),
	PLAYER_OPTIONALSAVEFLAGS_INPUTENTITIES		= M_Bit(3),
	PLAYER_OPTIONALSAVEFLAGS_ANIMEVENTLISTENER	= M_Bit(4),
	PLAYER_OPTIONALSAVEFLAGS_CAMERAUTIL			= M_Bit(5),
	PLAYER_OPTIONALSAVEFLAGS_PARENT				= M_Bit(6),
};

void CWObject_Character::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	CWO_Character_ClientData *pCD = GetClientData(this);
	if (!pCD)
		return;

	// Load name, might have changed
	CStr Name;
	Name = _pFile->Readln();
	if (Name != GetName())
		m_pWServer->Object_SetName(m_iObject,Name);

	MACRO_READVERIFY;

	if(pCD->m_iPlayer != -1)
	{
		int16 UDMoney, MaxHealth;
		MACRO_READ(int32, UDMoney);
		MACRO_READ(int32, MaxHealth);
		if(_Flags & INFOPLAYERSTART_FLAGS_CLEARPLAYER)
		{
			CRPG_Object_Item *pItem = Char()->FindItemByType(0x124);
			if(pItem)
				pItem->m_NumItems = UDMoney;

			Char()->MaxHealth() = MaxHealth;
			Char()->Health() = MaxHealth;
			Char_UpdateQuestItems();
			return;
		}
	}

	//Check which optional data we need to save
	int8 OptionalSaveFlags = 0;
	MACRO_READ(int8, OptionalSaveFlags);

	// Parent
	if (OptionalSaveFlags & PLAYER_OPTIONALSAVEFLAGS_PARENT)
	{
		int32 iObjParentGUID;
		MACRO_READ(int32,iObjParentGUID);
		MACRO_READ(int8,m_iParentAttach);
		// parent away!
		m_pWServer->Object_AddChild(m_pWServer->Object_GetIndex(iObjParentGUID), m_iObject);
	}

	//Should we load dialogue instance stuff?
	if (OptionalSaveFlags & PLAYER_OPTIONALSAVEFLAGS_DIALOGUEINSTANCE)
	{
		int32 StartTime1000;
		MACRO_READ(uint32, pCD->m_DialogueInstance.m_DialogueItemHash);
		MACRO_READ(int32, StartTime1000);
		MACRO_READ(fp32, pCD->m_DialogueInstance.m_SampleLength);
		pCD->m_DialogueInstance.m_StartTime = CMTime::CreateFromTicks(StartTime1000, 1000.0f);
	}
	//Should we load owner?
	if (OptionalSaveFlags & PLAYER_OPTIONALSAVEFLAGS_OWNER)
	{
		MACRO_READ(uint16, m_iOwner);
	}
	//Should we load extra item?
	if (OptionalSaveFlags & PLAYER_OPTIONALSAVEFLAGS_EXTRAITEM)
	{
		//Load everything that's changed in CAutoVar_AttachModel::UpdateModel.
		int8 nModels = 0;
		MACRO_READ(int8, nModels);
		for(int i = 0; i < nModels; i++)
		{
			CStr Model = _pFile->Readln();
			pCD->m_Item2_Model.m_iModel[i] = m_pWServer->GetMapData()->GetResourceIndex(Model);
			MACRO_READ(uint8, pCD->m_Item2_Model.m_iAttachPoint[i]);
			MACRO_READ(int8, pCD->m_Item2_Model.m_ModelFlags[i]);
		}

		MACRO_READ(uint8, pCD->m_Item2_Model.m_Flags);
		MACRO_READ(uint8, pCD->m_Item2_Model.m_iAttachRotTrack);

		//Make dirty to force update
		pCD->m_Item2_Model.MakeDirty();
	}
	// Should we load inputentities that's listening to us?
	if (OptionalSaveFlags & PLAYER_OPTIONALSAVEFLAGS_INPUTENTITIES)
	{
		int8 nEntities = 0;
		MACRO_READ(int8,nEntities);
		for (int32 i = 0; i < nEntities; i++)
		{
			int16 iInputEntity = 0;
			MACRO_READ(int16, iInputEntity);
			if (iInputEntity != 0)
				m_liInputEntities.Add(iInputEntity);
		}
	}
	if (OptionalSaveFlags & PLAYER_OPTIONALSAVEFLAGS_ANIMEVENTLISTENER)
	{
		int8 nEntities = 0;
		MACRO_READ(int8,nEntities);
		for (int32 i = 0; i < nEntities; i++)
		{
			int16 iAnimEventListener = 0;
			MACRO_READ(int16, iAnimEventListener);
			if (iAnimEventListener != 0)
				m_liAnimEventEntities.Add(iAnimEventListener);
		}
	}

	// Read and parse flags
	int32 Flags;
	MACRO_READ(int32, Flags);

	// Is this character waitspawn at gameworld spawn and but not waitspawn when loading savefile we spawn the character
	if(MACRO_FLAGCHANGED_TOFALSE(m_Flags, Flags, PLAYER_FLAGS_WAITSPAWN))
		SpawnCharacter(PLAYER_PHYS_STAND, PLAYER_SPAWNBEHAVIOR_NOSPAWNMESSAGES | PLAYER_SPAWNBEHAVIOR_FROMIO);

	// Allow level to decide NoNightvision flag
	m_Flags = (Flags & (~PLAYER_FLAGS_NONIGHTVISION)) | (m_Flags & PLAYER_FLAGS_NONIGHTVISION);


	// Read and parse other flags
	MACRO_READ(int32, m_ClientFlags);
	m_ClientFlags &= ~PLAYER_CLIENTFLAGS_CUTSCENE;
	m_ClientFlags &= ~PLAYER_CLIENTFLAGS_DIALOGUE;
	MACRO_READ(uint16, pCD->m_Phys_Flags);
	MACRO_READ(int8, pCD->m_Disable);

	/*CStr Model = _pFile->Readln();
	m_iModel[0] = m_pWServer->GetMapData()->GetResourceIndex(Model);*/

	MACRO_READ(int32, m_Data[PLAYER_DATAINDEX_CONTROL]);

	uint8 PhysType;
	MACRO_READ(uint8, PhysType);
	if((PhysType == 1 && m_InitialPhysMode == 2) || Char_GetPhysType(this) == 2)
		PhysType = 2;
	Char_SetPhysics(this, m_pWServer, m_pWServer, PhysType,false,true);
	if(PhysType == 2)
	{
		pCD->m_Control_Press |= CONTROLBITS_CROUCH;
		CNetMsg Msg(PLAYER_NETMSG_SETCROUCHINPUT);
		m_pWServer->NetMsg_SendToClass(Msg, m_iClass);
	}
	
	if (pCD->m_iPlayer == -1)
	{
		// AI dudes
		MACRO_READVERIFY;

		if (OptionalSaveFlags & PLAYER_OPTIONALSAVEFLAGS_PARENT)
		{
			InternalState_GetPositionMatrix().Read(_pFile);
			InternalState_GetLocalPositionMatrix().Read(_pFile);
		}
		else
		{
			InternalState_GetLocalPositionMatrix().Read(_pFile);
			m_pWServer->Phys_InsertPosition(m_iObject, this);
		}

		MACRO_READVERIFY;
		m_spAI->OnDeltaLoad(_pFile);
	}
	else
	{
		MACRO_READVERIFY;
		MACRO_BITFIELD_DECLARE(Player);
		MACRO_BITFIELD_READ(Player);
		uint8 bNV;
		MACRO_BITFIELD_REAM_Bit(Player, bNV);
		pCD->m_bNightvisionEnabled = pCD->m_bNightvisionEnabled | bNV;

		if (OptionalSaveFlags & PLAYER_OPTIONALSAVEFLAGS_PARENT)
		{
			// Read pos and localpos
			InternalState_GetPositionMatrix().Read(_pFile);
			InternalState_GetLocalPositionMatrix().Read(_pFile);
		}

		MACRO_READ(int32, pCD->m_Overviewmap_Visited);
		pCD->m_DarknessLevel.OnDeltaLoad(_pFile);

		pCD->m_Overviewmap_Visited = pCD->m_Overviewmap_Visited | (1 << pCD->m_Overviewmap_Current);
		
		// Load completed missions
		m_CompletedMissions.Read(_pFile);

		// load godmode
		int8 bGodMode;
		MACRO_READ(int8,bGodMode);
		if (bGodMode && Char_CheatsEnabled())
			 pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_GODMODE;
		// Camerautil
		if (OptionalSaveFlags & PLAYER_OPTIONALSAVEFLAGS_CAMERAUTIL)
		{
			pCD->m_CameraUtil.OnDeltaLoad(_pFile);
			int16 iCamObj = pCD->m_CameraUtil.GetMountedCameraObject();
			if (iCamObj)
			{
				CWObject_CoreData* pOriginObj = m_pWServer->Object_GetCD(iCamObj);
				if (!pOriginObj || !(pOriginObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER))
					pCD->m_CameraUtil.Clear();
			}
		}

		MACRO_READ(int16,pCD->m_iFocusFrameObject);
	}
	// Read dialog choices
	int16 nChoices;
	MACRO_READ(int16,nChoices);
	pCD->m_liDialogueChoice.SetLen(nChoices);
	for (int32 i = 0; i < nChoices; i++)
	{
		MACRO_READ(uint32,pCD->m_liDialogueChoice[i].m_ItemHash);
		int8 IsPlayer;
		MACRO_READ(int8,IsPlayer);
		pCD->m_liDialogueChoice[i].m_bIsPlayer = IsPlayer != 0;
	}
	// Mounted on both player and ai's
	MACRO_READ(int8,pCD->m_MountedMode);
	MACRO_READ(int32,pCD->m_MountedStartTick);
	int32 GUID_MountedObject;
	MACRO_READ(int32,GUID_MountedObject);
	pCD->m_iMountedObject = m_pWServer->Object_GetIndex(GUID_MountedObject);
	if (GUID_MountedObject)
	{
		MACRO_READ(int8,m_MountFlags);
		if (!pCD->m_iMountedObject)
		{
			// Mounted object not found, dismount
			pCD->m_MountedMode = 0;
			m_MountFlags = 0;
		}
	}

	MACRO_READ(uint8, m_DarknessFlags);

	MACRO_READVERIFY;

	MACRO_READ(int32, m_Data[RPG_DATAINDEX_CURDIALOGUEINDEX]);
	MACRO_READ(int32, m_Data[RPG_DATAINDEX_CURDIALOGUEITEMTICK]);

	MACRO_READ(uint16, m_Param);

	m_DialogueItems.Read(_pFile);

//	MACRO_READ(int32, m_SpawnDuration);
	MACRO_READ(int32, m_Flags);
	MACRO_READ(int32, m_iStealthTimer);
	MACRO_READ(int32, m_TranquillizerLevel);
	MACRO_READ(int32, m_TranquillizerTickCount);

	MACRO_READVERIFY;
	CWAG2I_Context Context(this,m_pWServer,pCD->m_GameTime);
	pCD->m_AnimGraph2.GetAG2I()->Read(&Context,_pFile);

	if(pCD->m_iPlayer != -1)
	{
		// Reset player ag (????)
		pCD->m_AnimGraph2.GetAG2I()->ClearTokens();
		pCD->m_AnimGraph2.GetAG2I()->Refresh(&Context);
	}

	// RPG Object
	int32 Health;
	MACRO_READ(int32, Health);
	if(Health < 16)
		Health = 16;
	else
	{
		int Frac = Health & 15;
		if(Frac != 0)
			Health += 16 - Frac;
	}
	Char()->Health() = Health;

	MACRO_READ(int32, Char()->MaxHealth());

	MACRO_READVERIFY;

	//
	MACRO_BITFIELD_DECLARE(Flags);
	MACRO_BITFIELD_READ(Flags);

	MACRO_READVERIFY;
	LoadInventory(_pFile);
	MACRO_READVERIFY;

	if(pCD->m_iPlayer != -1)
	{
		// If we are currently in a riotguard, save those states...
		CWObject_ActionCutsceneRiot::CharLoad(m_pWServer, _pFile, m_iObject);
		MACRO_READVERIFY;
		// Save weapon identifiers
		MACRO_READ(int32,m_LastUsedItem);
		MACRO_READ(int32, m_LastUsedItemType);

		// Make sure we don't have weapons equipped
		CRPG_Object_Item* pEquippedItem = Char()->GetFinalSelectedItem(AG2_ITEMSLOT_WEAPONS);
		if (pEquippedItem)
		{
			pCD->m_EquippedItemClass = (uint16)pEquippedItem->m_AnimProperty;
			pCD->m_EquippedItemType = (uint16)pEquippedItem->m_iItemType;
			pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_ITEMANIMTYPE,pEquippedItem->m_AnimType);

			// Hide player gun in the beginnning
			if ((pEquippedItem->m_iItemType != 0) && 
				(pEquippedItem->m_iItemType != RPG_ITEMTYPE_ANCIENTWEAPONS))
			{
				int PreviousWeaponId = pEquippedItem->m_Identifier;
				int PreviousItemType = pEquippedItem->m_iItemType;

				// Find fist item
				CRPG_Object_Item* pMelee = Char()->FindItemByType(0);
				if (pMelee)
				{
					pCD->m_EquippedItemClass = (uint16)pMelee->m_AnimProperty;
					pCD->m_EquippedItemType = (uint16)pMelee->m_iItemType;
					pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_ITEMANIMTYPE,pMelee->m_AnimType);
					int ItemIndex = Char()->GetInventory(AG2_ITEMSLOT_WEAPONS)->FindItemIndexByIdentifier(pMelee->m_Identifier);
					pCD->m_iCurSelectedItem = ItemIndex;
					m_LastUsedItem = PreviousWeaponId;
					m_LastUsedItemType = PreviousItemType;
					m_pWServer->Phys_Message_SendToObject(CWObject_Message(OBGMSG_CHAR_SWITCHITEMBYIDENTIFIER, 0, pMelee->m_Identifier),m_iObject);
					m_WeaponUnequipTimer = 0;
				}
			}
			CWAG2I_Context Context(this,m_pWServer,pCD->m_GameTime);
			pCD->m_AnimGraph2.UpdateImpulseState(&Context);
		}

		// Load current darkness powers
		uint8 Temp;
		MACRO_READ(uint8,Temp);
		pCD->m_Darkness = Temp;
		MACRO_READ(uint8,Temp);
		pCD->m_MaxDarkness = Temp;
		// Save powershield and ancientweapons
		uint16 Temp2;
		MACRO_READ(uint16, Temp2);
		pCD->m_DarknessSelectionMode = Temp2 & (PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD | PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS);
		
		MACRO_READ(uint8, Temp);
		pCD->m_DarknessPowersAvailable = Temp;

		// Read available darklings?
		int8 Len;
		MACRO_READ(int8,Len);
		m_lAvailableDarklings.SetLen(Len);
		for (int32 i = 0; i < Len; i++)
		{
			m_lAvailableDarklings[i].m_DarklingType = _pFile->Readln();
			MACRO_READ(int32,m_lAvailableDarklings[i].m_AddedTick);
		}
		MACRO_READVERIFY;

		int iObj = m_pWServer->Selection_GetSingleTarget("TELEPHONEREG");
		if(iObj)
		{
			CWObject_TelephoneRegistry *pTele = safe_cast<CWObject_TelephoneRegistry>(m_pWServer->Object_Get(iObj));
			uint8 Numbers = 0;
			MACRO_READ(uint8, Numbers);
			for(int i = 0; i < Numbers; i++)
			{
				CStr PhoneNumber;
				PhoneNumber = _pFile->Readln();

				CNameHash NameHash;
				NameHash.Read(_pFile);
				pTele->AddPhoneNumber(PhoneNumber, NameHash);
			}
		}
		else
		{
			uint8 Numbers = 0;
			MACRO_READ(uint8, Numbers);
		}
		MACRO_READVERIFY
	}

	// Read m_nTracks
	int32 nTracks;
	MACRO_READ(int32, nTracks);
	if (nTracks > 0)
	{
		CMat4Dfp32 Mat;
		Mat.Read(_pFile);
		OverridePositionMatrix(Mat);
	
		CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
		M_ASSERT((pCD != NULL),CStr("Ragdoll load failed to get pCD"));
		Char_RagdollDeltaRead(_pFile,pCD);
		/*
		CXR_AnimState Anim;
		CXR_Skeleton* pSkel;
		CXR_SkeletonInstance* pSkelInstance;
		GetEvaluatedPhysSkeleton(pSkelInstance, pSkel, Anim);
        if (!m_spRagdoll)
		{
			m_spRagdoll = MNew(CConstraintSystem);
		}

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
		m_spRagdoll->SetOrgMat(Mat);
		m_spRagdoll->Init(m_iObject, this, m_pWServer, pCD, &pCD->m_RagdollClient, 0);
		m_spRagdoll->Setup(&m_RagdollSettings);
		m_RagdollSettings.m_pSkelInstance = NULL;
		for(uint i = 0; i < CWO_NUMMODELINDICES; i++)
			m_RagdollSettings.m_lpClothSkeleton[i] = NULL;
		m_spRagdoll->Activate(true);

		m_spRagdoll->OnDeltaLoad(_pFile);
		m_spRagdoll->Simulate();
		m_spRagdoll->Animate(pCD->m_GameTick, Mat);
		pCD->m_RagdollClient.MakeDirty();
		*/
		
		//m_spRagdoll->SetState(CConstraintSystem::READY);
	}

	MACRO_READVERIFY;
	//Load last position (this is only done for characters, due to AI issues)
	//Set "last rotation" as current rotation
 	CVec3Dfp32 LastPos;
	LastPos.Read(_pFile);
	InternalState_GetLastPositionMatrix() = GetPositionMatrix();
	LastPos.SetRow(InternalState_GetLastPositionMatrix(), 3);
	MACRO_READVERIFY;

	pCD->m_Burnable.Read(_pFile, m_pWServer->GetMapData());
	pCD->m_Burnable.MakeDirty();
	MACRO_READVERIFY;

	if(pCD->m_iPlayer != -1)
		Char_UpdateQuestItems();


	//If we're owned by someone, notify owner of load 
	if (OptionalSaveFlags & PLAYER_OPTIONALSAVEFLAGS_OWNER)	
	{
		CWObject_Message Msg(OBJMSG_SPAWNER_ONLOADOWNED);
		Msg.m_iSender = m_iObject;
		m_pWServer->Message_SendToObject(Msg, m_iOwner);
	}

	// Update dirtymask
	m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
}

void CWObject_Character::OnDeltaSave(CCFile* _pFile)
{
	CWO_Character_ClientData *pCD = GetClientData(this);
	if (!pCD)
		return;

	// Save name, might have changed from spawn time (darklings among other things)
	_pFile->Writeln(GetName());
	MACRO_WRITEVERIFY;

	if(pCD->m_iPlayer != -1)
	{
		MACRO_WRITE(int32, pCD->m_nUDMoney);
		MACRO_WRITE(int32, Char()->MaxHealth());
	}

	//Check which optional data we need to save
	int8 OptionalSaveFlags = 0;
	{
		if (pCD->m_DialogueInstance.IsValid())
			OptionalSaveFlags |= PLAYER_OPTIONALSAVEFLAGS_DIALOGUEINSTANCE;
		//Apparently owner default is -1 even though owner is uint16 : 15 :P
		if (m_iOwner != (((uint16)~0) >> 2))
			OptionalSaveFlags |= PLAYER_OPTIONALSAVEFLAGS_OWNER;
		if (pCD->m_Item2_Model.IsValid())
			OptionalSaveFlags |= PLAYER_OPTIONALSAVEFLAGS_EXTRAITEM;
		if (m_liInputEntities.Len())
			OptionalSaveFlags |= PLAYER_OPTIONALSAVEFLAGS_INPUTENTITIES;
		if (m_liAnimEventEntities.Len())
			OptionalSaveFlags |= PLAYER_OPTIONALSAVEFLAGS_ANIMEVENTLISTENER;
		if (pCD->m_CameraUtil.IsActive())
			OptionalSaveFlags |= PLAYER_OPTIONALSAVEFLAGS_CAMERAUTIL;
		if (GetParent() != 0)
			OptionalSaveFlags |= PLAYER_OPTIONALSAVEFLAGS_PARENT;
		//TODO:
		//OptionalSaveFlags |= m_spAI->GetOptionalSaveFlags();
		//OptionalSaveFlags |= pCD->m_AnimGraph->GetOptionalSaveFlags();
		//...etc. It would propably be a good idea to do a "optionalsaveflag-pass"
		//on all character members later when/if we need to reduce save-size
	}
	MACRO_WRITE(int8, OptionalSaveFlags);
	// Parent
	if (OptionalSaveFlags & PLAYER_OPTIONALSAVEFLAGS_PARENT)
	{
		int32 iParentGUID = m_pWServer->Object_GetGUID(GetParent());
		MACRO_WRITE(int32,iParentGUID);
		MACRO_WRITE(int8,m_iParentAttach);
	}

	//Should we save dialogue instance stuff?
 	if (OptionalSaveFlags & PLAYER_OPTIONALSAVEFLAGS_DIALOGUEINSTANCE)
	{
		MACRO_WRITE(uint32, pCD->m_DialogueInstance.m_DialogueItemHash);
		MACRO_WRITE(int32, pCD->m_DialogueInstance.m_StartTime.GetNumTicks(1000.0f));
		MACRO_WRITE(fp32, pCD->m_DialogueInstance.m_SampleLength);
	}
	//Should we save owner?
	if (OptionalSaveFlags & PLAYER_OPTIONALSAVEFLAGS_OWNER)
	{
		MACRO_WRITE(uint16, m_iOwner);
	}
	//Should we save extra attach item?
	if (OptionalSaveFlags & PLAYER_OPTIONALSAVEFLAGS_EXTRAITEM)
	{
		//Save everything that's changed in CAutoVar_AttachModel::UpdateModel.
		int8 nModels = 0;
		int i;
		for(i = ATTACHMODEL_NUMMODELS - 1; i >= 0; i--)
		{
			if (pCD->m_Item2_Model.m_iModel[i] != 0)
			{
				nModels = i + 1;
				break;
			}
		}
		MACRO_WRITE(int8, nModels);
        
		for(i = 0; i < nModels; i++)
		{
			//Save model resource name to be safe.
			CStr Model = m_pWServer->GetMapData()->GetResourceName(pCD->m_Item2_Model.m_iModel[i]);
			_pFile->Writeln(Model);
			MACRO_WRITE(uint8, pCD->m_Item2_Model.m_iAttachPoint[i]);
			MACRO_WRITE(int8, pCD->m_Item2_Model.m_ModelFlags[i]);
		}

		MACRO_WRITE(uint8, pCD->m_Item2_Model.m_Flags);
		MACRO_WRITE(uint8, pCD->m_Item2_Model.m_iAttachRotTrack);
	}

	// Should we load inputentities that's listening to us?
	if (OptionalSaveFlags & PLAYER_OPTIONALSAVEFLAGS_INPUTENTITIES)
	{
		int8 nEntities = m_liInputEntities.Len();
		MACRO_WRITE(int8,nEntities);
		for (int32 i = 0; i < nEntities; i++)
		{
			MACRO_WRITE(int16, m_liInputEntities[i]);
		}
	}
	if (OptionalSaveFlags & PLAYER_OPTIONALSAVEFLAGS_ANIMEVENTLISTENER)
	{
		int8 nEntities = m_liAnimEventEntities.Len();
		MACRO_WRITE(int8,nEntities);
		for (int32 i = 0; i < nEntities; i++)
		{
			MACRO_WRITE(int16, m_liAnimEventEntities[i]);
		}
	}

	MACRO_WRITE(int32, m_Flags);
	MACRO_WRITE(int32, m_ClientFlags);
	MACRO_WRITE(int16, pCD->m_Phys_Flags);
	MACRO_WRITE(int8, pCD->m_Disable);

	/*CStr Model = m_pWServer->GetMapData()->GetResourceName(m_iModel[0]);
	_pFile->Writeln(Model);*/

	MACRO_WRITE(int32, m_Data[PLAYER_DATAINDEX_CONTROL]);
	MACRO_WRITE(int8, Char_GetPhysType(this));

	if (pCD->m_iPlayer == -1)
	{
		//Stuff that should only be saved on AI characters
		MACRO_WRITEVERIFY;

		CMat4Dfp32 Mat = GetPositionMatrix();
		Mat.Write(_pFile);
		
		if (OptionalSaveFlags & PLAYER_OPTIONALSAVEFLAGS_PARENT)
		{
			// Save localpos as well when parented
			Mat = GetLocalPositionMatrix();
			Mat.Write(_pFile);
		}

		MACRO_WRITEVERIFY;
		m_spAI->OnDeltaSave(_pFile);
	}
	else
	{
		// Only save on player character
		MACRO_WRITEVERIFY;
		MACRO_BITFIELD_DECLARE(Player);
		MACRO_BITFIELD_WRITEBIT(Player, pCD->m_bNightvisionEnabled);
		MACRO_BITFIELD_WRITE(Player);

		if (OptionalSaveFlags & PLAYER_OPTIONALSAVEFLAGS_PARENT)
		{
			// Save pos and localpos
			GetPositionMatrix().Write(_pFile);
			GetLocalPositionMatrix().Write(_pFile);
		}

		MACRO_WRITE(int32, pCD->m_Overviewmap_Visited);
		pCD->m_DarknessLevel.OnDeltaSave(_pFile);

		// Save completed missions
		m_CompletedMissions.Write(_pFile);

		// Save godmode
		int8 bGodMode = (pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_GODMODE) != 0;
		MACRO_WRITE(int8, bGodMode);
		// Save camerautil
		if (OptionalSaveFlags & PLAYER_OPTIONALSAVEFLAGS_CAMERAUTIL)
			pCD->m_CameraUtil.OnDeltaSave(_pFile);
	
		MACRO_WRITE(int16,pCD->m_iFocusFrameObject);
	}

	// Write dialog choices if any
	int16 nChoices = pCD->m_liDialogueChoice.Len();
	MACRO_WRITE(int16,nChoices);
	for (int32 i = 0; i < nChoices; i++)
	{
		MACRO_WRITE(uint32,pCD->m_liDialogueChoice[i].m_ItemHash);
		int8 IsPlayer = pCD->m_liDialogueChoice[i].m_bIsPlayer;
		MACRO_WRITE(int8,IsPlayer);
	}

	// Mounted on both player and ai's
	MACRO_WRITE(int8,pCD->m_MountedMode);
	MACRO_WRITE(int32,pCD->m_MountedStartTick);
	int32 GUID_MountedObject = m_pWServer->Object_GetGUID(pCD->m_iMountedObject);
	MACRO_WRITE(int32,GUID_MountedObject);
	if (pCD->m_iMountedObject != 0)
		MACRO_WRITE(int8,m_MountFlags);

	MACRO_WRITE(uint8, m_DarknessFlags);

	MACRO_WRITEVERIFY;

	MACRO_WRITE(int32, m_Data[RPG_DATAINDEX_CURDIALOGUEINDEX]);
	MACRO_WRITE(int32, m_Data[RPG_DATAINDEX_CURDIALOGUEITEMTICK]);

	MACRO_WRITE(uint16, m_Param);

	m_DialogueItems.Write(_pFile);

//	MACRO_WRITE(int32, m_SpawnDuration);
	MACRO_WRITE(int32, m_Flags);
	MACRO_WRITE(int32, m_iStealthTimer);
	MACRO_WRITE(int32, m_TranquillizerLevel);
	MACRO_WRITE(int32, m_TranquillizerTickCount);

	MACRO_WRITEVERIFY;
	pCD->m_AnimGraph2.GetAG2I()->Write(_pFile);

	// RPG Object
	MACRO_WRITE(int32, Char()->Health());
	MACRO_WRITE(int32, Char()->MaxHealth());
	//MACRO_WRITE(int32, Char()->m_LastDamageTick);

	MACRO_WRITEVERIFY;

	MACRO_BITFIELD_DECLARE(Flags);
	MACRO_BITFIELD_WRITE(Flags); 

	MACRO_WRITEVERIFY;
	SaveInventory(_pFile);
	MACRO_WRITEVERIFY;

	if(pCD->m_iPlayer != -1)
	{
		// If we are currently in a riotguard, save those states...
		CWObject_ActionCutsceneRiot::CharSave(m_pWServer, _pFile, m_iObject,pCD);
		MACRO_WRITEVERIFY;

		// Save weapon identifiers
		MACRO_WRITE(int32, m_LastUsedItem);
		MACRO_WRITE(int32, m_LastUsedItemType);

		// Save current darkness powers
		MACRO_WRITE(uint8,pCD->m_Darkness.m_Value);
		MACRO_WRITE(uint8,pCD->m_MaxDarkness.m_Value);
		// Save powershield and ancientweapons
		MACRO_WRITE(uint16, pCD->m_DarknessSelectionMode.m_Value);
		MACRO_WRITE(uint8, pCD->m_DarknessPowersAvailable.m_Value);

		// Write available darklingtypes that the player can spawn?
		int8 Len = m_lAvailableDarklings.Len();
		MACRO_WRITE(int8,Len);
		for (int32 i = 0; i < Len; i++)
		{
			_pFile->Writeln(m_lAvailableDarklings[i].m_DarklingType);
			MACRO_WRITE(int32,m_lAvailableDarklings[i].m_AddedTick);
		}
		MACRO_WRITEVERIFY;

		int iObj = m_pWServer->Selection_GetSingleTarget("TELEPHONEREG");
		if(iObj)
		{
			CWObject_TelephoneRegistry *pTele = safe_cast<CWObject_TelephoneRegistry>(m_pWServer->Object_Get(iObj));
			MACRO_WRITE(uint8, pTele->m_lPhoneBook.Len());
			for(int i = 0; i < pTele->m_lPhoneBook.Len(); i++)
			{
				_pFile->Writeln(pTele->m_lPhoneBook[i].m_PhoneNumber);
				pTele->m_lPhoneBook[i].m_DialogLink.Write(_pFile);
			}
		}
		else
		{
			uint8 Numbers = 0;
			MACRO_WRITE(uint8, Numbers);
		}
		MACRO_WRITEVERIFY
	}

	if ((m_Flags & PLAYER_FLAGS_RAGDOLL) && ((Char_GetPhysType(this) == PLAYER_PHYS_DEAD)||(pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_RAGDOLLACTIVE))
		&& (m_pPhysCluster))
	{
		// Save m_nTracks
		MACRO_WRITE(int32,1);
		CMat4Dfp32 Mat = GetPositionMatrix();
		Mat.Write(_pFile);
		
		// m_spRagdoll->OnDeltaSave(_pFile);
		
		Char_RagdollDeltaWrite(_pFile);
	}
	else
	{
		int32 Temp = 0;
		MACRO_WRITE(int32,Temp);
	}

	MACRO_WRITEVERIFY;
	//Save last position (this is only done for characters, due to AI issues). Position only, no rotation.
	GetLastPosition().Write(_pFile);
	MACRO_WRITEVERIFY;

	pCD->m_Burnable.Write(_pFile, m_pWServer->GetMapData());
	MACRO_WRITEVERIFY;
}


void CWObject_Character::OnFinishDeltaLoad()
{
	CWObject_Player::OnFinishDeltaLoad();

	// Override loaded dialogye items
	m_DialogueItems.Override(m_PostLoadDialogueItems);
}


void CWObject_Character::SaveInventory(CCFile* _pFile)
{
	CWO_Character_ClientData *pCD = GetClientData(this);
	if (pCD)
	{
		if (pCD->m_iPlayer != -1)
		{
			//
			// This is the player.
			// This inventory can be changed during play,
			// therefore we need to do more extensive save
			//
			SaveInventory_Player(_pFile);
		} else {
			//
			// This is an AI character.
			// Since AI character inventories aren't allowed to
			// change we just do OnDeltaLoad/Save for each object.
			//
			SaveInventory_AI(_pFile);
		}
	}
}

void CWObject_Character::SaveInventory_PlayerHelper(CCFile* _pFile, bool _bNoTrans, bool _bOnlyTrans, bool _bPrecache)
{
	CRPG_Object_Char* pChar = Char();
	uint32 lnItems[RPG_CHAR_NUMCHILDREN];
	uint32 lnRealItems[RPG_CHAR_NUMCHILDREN];
	// Save names
	for(int i = 0; i < RPG_CHAR_NUMCHILDREN; i++)
	{
		CRPG_Object_Inventory* pInv = pChar->GetInventory(i);
		lnItems[i] = pChar->GetNumItems(i);
		lnRealItems[i] = _bOnlyTrans ? pChar->NumTranscendingItems(i) : (_bNoTrans ? lnItems[i] - pChar->NumTranscendingItems(i) : lnItems[i]);
		_pFile->WriteLE(&lnRealItems[i], 1);
		for(int j = 0; j < lnItems[i]; j++)
		{
			CRPG_Object_Item *pItem = pInv->GetItemByIndex(j);
			if (pItem)
			{
				if (_bOnlyTrans && !(pItem->m_Flags2 & RPG_ITEM_FLAGS2_TRANSCENDIVENTORY))
					continue;
				if (_bOnlyTrans || (!_bNoTrans || !(pItem->m_Flags2 & RPG_ITEM_FLAGS2_TRANSCENDIVENTORY)))
				{
					CFStr ItemName = pItem->GetDesc(0);
					VERBOSE_OUT(CStrF("Inventory item %i on player is a %s.", j, ItemName.GetStr()));
					_pFile->Writeln(ItemName.GetStr());
					if (_bPrecache)
					{
						CWObject_Message Msg(OBJMSG_GAMEP4_SAVEPRECACHEINFO, PRECACHEINFO_RPG);
						Msg.m_pData = ItemName.GetStr();
						m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());
					}
				}
			}
		}
	}

	// Save object specific data
	for(int i = 0; i < RPG_CHAR_NUMCHILDREN; i++)
	{
		CRPG_Object_Inventory* pInv = pChar->GetInventory(i);
		int32 EquippedItemType = pInv->GetFinalSelectedItemType();
		_pFile->WriteLE(&EquippedItemType, 1);
		for(int j = 0; j < lnItems[i]; j++)
		{
			CRPG_Object_Item *pItem = pInv->GetItemByIndex(j);
			if (_bOnlyTrans && !(pItem->m_Flags2 & RPG_ITEM_FLAGS2_TRANSCENDIVENTORY))
				continue;
			if (_bOnlyTrans || (!_bNoTrans || !(pItem->m_Flags2 & RPG_ITEM_FLAGS2_TRANSCENDIVENTORY)))
				pItem->OnDeltaSave(_pFile, pChar);		
		}
		// Save inventory itemcounter
		int32 ItemCounter = pInv->GetItemCounter();
		_pFile->WriteLE(ItemCounter);
	}
}

void CWObject_Character::SaveInventory_Player(CCFile* _pFile)
{
	//
	// Save entire contents of inventory
	//
	VERBOSE_OUT(CStr("Saving inventory on player objekt."));
	CRPG_Object_Char* pChar = Char();
	// If a certain param is set on the player, save current inventory in the "saveforlater" bin
	if (m_SaveParams == PLAYER_SAVEPARAM_SAVETOBIN)
	{
		// Oki then, save all objects to the "bin" and remove them all from the real inventory
		// except the mighty fist
		CStream_Memory MemoryStream;
		CCFile TempFile;
		MemoryStream.Open(m_lSavedInventory, CFILE_READ | CFILE_BINARY);
		TempFile.Open(&MemoryStream, CFILE_WRITE | CFILE_BINARY);
		// Save inventory, no transcending items and no precache
		SaveInventory_PlayerHelper(&TempFile,true,false,false);

		TempFile.Close();
		MemoryStream.Close();

		// TEMP REMOVE
		int32 SavedInvLen = m_lSavedInventory.Len();

		// Empty inventory from everything but transcending items
		for(int i = 0; i < RPG_CHAR_NUMCHILDREN; i++)
		{
			uint32 nItems = pChar->GetNumItems(i);
			CRPG_Object_Inventory* pInv = pChar->GetInventory(i);
			for(int j = 0; j < nItems; j++)
			{
				CRPG_Object_Item *pItem = pInv->GetItemByIndex(j);
				if (pItem && !(pItem->m_Flags2 & RPG_ITEM_FLAGS2_TRANSCENDIVENTORY))
				{
					pInv->DelChild(j);
					j--;
					nItems--;
				}
			}
		}
		// Process
		m_spRPGObject->Process(NULL, m_iObject);
	}
	else if (m_SaveParams == PLAYER_SAVEPARAM_LOADFROMBIN)
	{
		// Oki then, use the bin instead of current inventory, might need to save some items that
		// have been collected during our travels though
		int8 Flags = 0;
		// Go through inventories, check for "transcending" items
		Flags |= pChar->HasTranscendingItems() ? PLAYER_INVENTORYSAVEFLAG_HASTRANSCENDINGITEMS : 0;
		Flags |= !m_lSavedInventory.Len() ? PLAYER_INVENTORYSAVEFLAG_NONORMALINVENTORY : 0;
		
		_pFile->WriteLE(Flags);
		if (!(Flags & PLAYER_INVENTORYSAVEFLAG_NONORMALINVENTORY))
			_pFile->WriteLE(m_lSavedInventory.GetBasePtr(),m_lSavedInventory.Len());

		// Add extra items that we might wanna save
		if (Flags & PLAYER_INVENTORYSAVEFLAG_HASTRANSCENDINGITEMS)
		{
			// Save transcending items only, and precache them
			SaveInventory_PlayerHelper(_pFile, false, true,true);
		}

		// Send precache info for the saved items that we are now bringing back
		if (!(Flags & PLAYER_INVENTORYSAVEFLAG_NONORMALINVENTORY))
		{
			CStream_Memory MemoryStream;
			CCFile TempFile;
			MemoryStream.Open(m_lSavedInventory, CFILE_READ | CFILE_BINARY);
			TempFile.Open(&MemoryStream, CFILE_READ | CFILE_BINARY);
			for(int i = 0; i < RPG_CHAR_NUMCHILDREN; i++)
			{
				uint32 nItems;
				TempFile.ReadLE(&nItems, 1);
				for(int j = 0; j < nItems; j++)
				{
					CStr ItemName = TempFile.Readln();
					CWObject_Message Msg(OBJMSG_GAMEP4_SAVEPRECACHEINFO, PRECACHEINFO_RPG);
					Msg.m_pData = ItemName.GetStr();
					m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());
				}
			}
		}

		return;
	}

	int8 Flags = m_lSavedInventory.Len() ? PLAYER_INVENTORYSAVEFLAG_HASSAVEDINVENTORY : 0;
	_pFile->WriteLE(Flags);
	// Save extra inventory
	if (Flags & PLAYER_INVENTORYSAVEFLAG_HASSAVEDINVENTORY)
	{
		int32 Size = m_lSavedInventory.Len();
		_pFile->WriteLE(Size);
		_pFile->WriteLE(m_lSavedInventory.GetBasePtr(),Size);
	}

	// Save normal inventory, precache items
	SaveInventory_PlayerHelper(_pFile, false, false, true);
}

void CWObject_Character::SaveInventory_AI(CCFile* _pFile)
{
	int i, j;
	uint32 SaveMask = 0;
	uint32 nItems = 0;
	uint32 iItem = 0;

	VERBOSE_OUT(CStrF("Saving inventory for AI objekt %i.", m_iObject));
	// First enumerate which items to save
	for(i=0; i<RPG_CHAR_NUMCHILDREN; i++)
	{
		uint32 nLocalItems = Char()->GetNumItems(i);
		nItems += nLocalItems;
		for(j=0; j<nLocalItems; j++)
		{
			CRPG_Object_Item *pItem = Char()->GetInventory(i)->GetItemByIndex(j);
			if (pItem != NULL)
			{
				if (pItem->NeedsDeltaSave(Char()) == true)
				{
					SaveMask |= (1 << iItem);
				}
				iItem++;
			}
		}
	}

	M_ASSERT(nItems <= 32, "ERROR: Bitfield depleted. Save is broken. Fixelifix!");

	// Savie savie

	// Write header
	_pFile->WriteLE(&nItems, 1);
	_pFile->WriteLE(&SaveMask, 1);

	// Loop through all objects and see if it should be saved or not, obey SaveMask
	if (SaveMask)
	{
		iItem = 0;
		for(i=0; i<RPG_CHAR_NUMCHILDREN; i++)
		{
			uint32 nLocalItems = Char()->GetNumItems(i);
			for(j=0; j<nLocalItems; j++)
			{
				CRPG_Object_Item *pItem = Char()->GetInventory(i)->GetItemByIndex(j);
				if (pItem != NULL)
				{
					if ((SaveMask & (1 << iItem)) == (1 << iItem))
					{
						VERBOSE_OUT(CStrF("Inventory item %i on IA %i is of type %s.", j, m_iObject, typeid(*pItem).name()));
						pItem->OnDeltaSave(_pFile, Char());
					}

					iItem++;
				}
			}
		}
	}
}

void CWObject_Character::LoadInventory(CCFile* _pFile)
{
	CWO_Character_ClientData *pCD = GetClientData(this);
	if (pCD)
	{
		if (pCD->m_iPlayer != -1)
		{
			//
			// This is the player.
			// This inventory can be changed during play,
			// therefore we need to do more extensive save
			//
			LoadInventory_Player(_pFile);
		} else {
			//
			// This is an AI character.
			// Since AI character inventories aren't allowed to
			// change we just do OnDeltaLoad/Save for each object
			//
			LoadInventory_AI(_pFile);
		}
	}
}

void CWObject_Character::LoadInventory_PlayerHelper(CCFile* _pFile, bool _bRemoveItems)
{
	CRPG_Object_Char* pChar = Char();
	uint32 lnItems[RPG_CHAR_NUMCHILDREN];
	TArray<TPtr<CRPG_Object_Item> > lspItems[RPG_CHAR_NUMCHILDREN];
	for(uint i = 0; i < RPG_CHAR_NUMCHILDREN; i++)
	{
		//
		// Clear what's in this inventory first
		//
		if (_bRemoveItems)
		{
			uint nItems = pChar->GetInventory(i)->GetNumItems();
			for (uint iItem = 0; iItem < nItems; iItem++)
				pChar->GetInventory(i)->RemoveItemByIndex(0, 0);
		}

		_pFile->ReadLE(&lnItems[i], 1);
		lspItems[i].SetLen(lnItems[i]);
		for(int j = 0; j < lnItems[i]; j++)
		{
			CStr ItemName = _pFile->Readln();
			spCRPG_Object spObj = pChar->CreateObject(ItemName);
			if (!spObj)
				continue;
			lspItems[i][j] = (CRPG_Object_Item *)((CRPG_Object *)spObj);


			VERBOSE_OUT(CStrF("Inventory item %i on player is a %s.", j, ItemName.GetStr()));

		}
	}

	for(uint i = 0; i < RPG_CHAR_NUMCHILDREN; i++)
	{
		CRPG_Object_Inventory* pInventory = pChar->GetInventory(i);
		int32 EquippedItemType;
		_pFile->ReadLE(&EquippedItemType, 1);
		for(int j = 0; j < lnItems[i]; j++)
		{
			// Delta load before stuffing in an inventory
			lspItems[i][j]->OnDeltaLoad(_pFile, pChar);

			int Res = lspItems[i][j]->OnPickup(m_iObject, pChar, true, 0, true);
			if(Res == 0)
				continue;
			else if(Res == 2)
				continue;

			int iSlot = lspItems[i][j]->m_iItemType >> 8;
			//New unique item
			pInventory->AddItem(m_iObject, lspItems[i][j]);
		}

		// Set inventory itemcounter
		int32 ItemCounter;
		MACRO_READ(int32,ItemCounter);
		pChar->GetInventory(i)->SetItemCounter(ItemCounter);
		// Don't select item for dual wield inventory
		if (i != 2)
			pInventory->SelectItem(EquippedItemType, pChar, true);
	}
}

void CWObject_Character::LoadInventory_Player(CCFile* _pFile)
{
	//
	// Read inventory
	//
	int8 Flags;
	_pFile->ReadLE(Flags);
	if (Flags & PLAYER_INVENTORYSAVEFLAG_HASSAVEDINVENTORY)
	{
		int32 Size;
		_pFile->ReadLE(Size);
		m_lSavedInventory.SetLen(Size);
		_pFile->ReadLE(m_lSavedInventory.GetBasePtr(),Size);
	}
	CRPG_Object_Char* pChar = Char();
	if (!(Flags & PLAYER_INVENTORYSAVEFLAG_NONORMALINVENTORY))
		LoadInventory_PlayerHelper(_pFile, true);

	// Load "transcending items"
	if (Flags & PLAYER_INVENTORYSAVEFLAG_HASTRANSCENDINGITEMS)
		LoadInventory_PlayerHelper(_pFile, false);

	{
		// Ugly code (tm). Fix for Workpass revisited with dead guard in door problem
		if(m_pWServer->m_WorldName.CompareNoCase("pa2_workpass") == 0 && Char()->FindItemByType(0x193) != NULL)
		{
			// We are in workpass and have the bomb
			m_pWServer->Message_SendToTarget(CWObject_Message(OBJMSG_IMPULSE, 1), "d_wentroom2");
			m_pWServer->Message_SendToTarget(CWObject_Message(OBJMSG_IMPULSE, 2), "d_wentroom2");
			m_pWServer->Message_SendToTarget(CWObject_Message(OBJMSG_IMPULSE, -1), "d_wentroom2");
		}
	}

}

void CWObject_Character::LoadInventory_AI(CCFile* _pFile)
{
	int i, j;
	uint32 LoadMask;
	uint32 nExpectedItems = 0;
	uint32 nItems;
	for(i=0; i<RPG_CHAR_NUMCHILDREN; i++)
	{
		nExpectedItems += Char()->GetNumItems(i);
	}

	// Loadiloadi
	_pFile->ReadLE(&nItems, 1);
	_pFile->ReadLE(&LoadMask, 1);

	// Loop through all objects and see if it should be loaded or not
	uint32 iItem = 0;
	for(i=0; i<RPG_CHAR_NUMCHILDREN; i++)
	{
		uint32 nLocalItems = Char()->GetNumItems(i);
		for(j=0; j<nLocalItems; j++)
		{
			CRPG_Object_Item *pItem = Char()->GetInventory(i)->GetItemByIndex(j);
			if (pItem != NULL)
			{
				if ((LoadMask & (1 << iItem)) == (1 << iItem))
				{
					VERBOSE_OUT(CStrF("Inventory item %i on AI %i is of type %s.", j, m_iObject, typeid(*pItem).name()));
					pItem->OnDeltaLoad(_pFile, Char());
				}

				iItem++;
			}
		}
	}
}

int CWObject_Character::OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const
{
	const CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	if (!pCD) Error("OnCreateClientUpdate", "Unable to create client update.");

	int Flags = CWO_CLIENTUPDATE_EXTRADATA; // Used for Animgraph
	if(_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT)
		Flags |= CWO_CLIENTUPDATE_AUTOVAR;

	uint8 *pD = _pData;
	pD += CWObject_RPG::OnCreateClientUpdate(_iClient, _pClObjInfo, _pOld, pD, Flags);
	int ClFlags = _pClObjInfo->m_ClientFlags | m_ClientFlags;

	// Replicate press & release only if this object is replicated with high precision,
	// i.e, it is probably beeing sent to a client that is playing this character.
	if (ClFlags & CWO_CLIENTFLAGS_HIGHPRECISION)
	{
		PTR_PUTINT16(pD, pCD->m_Control_Press);
		PTR_PUTINT16(pD, pCD->m_Control_Released);
	}

	// AGMERGE
	// AGI State Sync
	{
	
		if (pCD != NULL)
		{
			// First byte in ag pack will be flags, the last 2 bits will be used by inventory info
			uint8& SharedFlags = pD[0];
			{
				MSCOPESHORT(PackNew);
//				uint8* pDataBefore = pD;
				int8 AGPackType = (m_pWServer->GetPlayMode() == SERVER_PLAYMODE_DEMO ? WAG2I_CLIENTUPDATE_NORMAL : WAG2I_CLIENTUPDATE_DIRECTACCESS);
				// pAG2IMirror will be NULL on first update 
				// (client mirror doesn't exist at that point yet)
				static const CWAG2I DummyAGI;
				const CWAG2I_Mirror* pAG2IMirror = GetAG2IMirror(_pOld);
				const CWAG2I* pAG2IMirrorChar = pAG2IMirror ? pAG2IMirror->GetWAG2I(0) : &DummyAGI;
				pD += pCD->m_AnimGraph2.GetAG2I()->OnCreateClientUpdate2(pD, pAG2IMirrorChar, AGPackType);
				{
					const CWAG2I* pAG2IMirrorWeap = pAG2IMirror ? pAG2IMirror->GetWAG2I(1) : &DummyAGI;
					pD += pCD->m_WeaponAG2.GetAG2I()->OnCreateClientUpdate2(pD, pAG2IMirrorWeap,AGPackType);
				}
				//NewTotal += pD - pDataBefore;
				//NewMax = Max(NewMax,pD - pDataBefore);
			}
			// Inventory info update (only for players)
			uint8* pBefore = pD;
			const CWO_Character_ClientData* pCDMirror = GetClientData(_pOld);
			if (pCDMirror && pCD->m_iPlayer != -1)
				pD += pCD->m_InventoryInfo.OnCreateClientUpdate2(pD,pCDMirror->m_InventoryInfo, SharedFlags);
		}
		else
			ConOutL("ERROR: Can't create client AGIState update!!!");
	}

	int AFlags = 0;
//	if(pCD->m_iPlayer != -1 && pCD->m_SneakHud > 0)
	pCD->AutoVar_Pack(_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT, pD, m_pWServer->GetMapData(), AFlags);
//	const_cast<CWS_ClientObjInfo *>(_pClObjInfo)->m_DirtyMask &= ~(-1 << CWO_DIRTYMASK_USERSHIFT);

	return pD - _pData;
}

int CWObject_Character::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	MSCOPESHORT(CWObject_Character::OnClientUpdate);
	const uint8 *pD = _pData;
	pD += CWObject_RPG::OnClientUpdate(_pObj, _pWClient, pD, _Flags);

	if (_pObj->m_iClass == 0 || (pD - _pData == 0))
		return pD - _pData;

	CWO_Character_ClientData *pCD = GetClientData(_pObj);
	if (!pCD) Error_static("CWObject_Character::OnCreateClientUpdate", "Unable to unpack client update.");

	if (_pObj->m_ClientFlags & CWO_CLIENTFLAGS_HIGHPRECISION)
	{
		PTR_GETINT16(pD, pCD->m_Control_Press);
		PTR_GETINT16(pD, pCD->m_Control_Released);
	}

	pCD->m_GameTick = _pWClient->GetGameTick() + Char_GetGameTickDiff(_pObj);
	pCD->m_GameTime = PHYSSTATE_TICKS_TO_TIME(pCD->m_GameTick, _pWClient);

	pCD->m_Anim_BodyAngleZ = _pObj->m_iAnim1/65536.0f;

	// AGMERGE
	// AGI State Sync
	{
		{
			uint8 SharedFlags = pD[0];
			MSCOPESHORT(UnpackNew);
			CWAG2I_Context AG2IContext(_pObj, _pWClient, pCD->m_GameTime);
			CWAG2I_Mirror* pAG2IMirror = GetAG2IMirror(_pObj);
			CWAG2I* pAG2IMirrorChar = pAG2IMirror->GetWAG2I(0);
			CWAG2I* pAG2I = ((_pWClient->GetClientMode() & WCLIENT_MODE_MIRROR) == 0) ? pCD->m_AnimGraph2.GetAG2I() : NULL;
			pD += CWAG2I::OnClientUpdate2(&AG2IContext, pAG2IMirrorChar, pAG2I, pD);
			{
				CWAG2I* pWeaponAG = ((_pWClient->GetClientMode() & WCLIENT_MODE_MIRROR) == 0) ? pCD->m_WeaponAG2.GetAG2I() : NULL;
				CWAG2I* pAG2IMirrorWeapon = pAG2IMirror->GetWAG2I(1);
				pD += CWAG2I::OnClientUpdate2(&AG2IContext, pAG2IMirrorWeapon, pWeaponAG, pD);
				if (pWeaponAG)
					pWeaponAG->RefreshPredictionMisses(&AG2IContext);
			}
			// Inventory info update
			pD += pCD->m_InventoryInfo.OnClientUpdate(pD,SharedFlags);
		}
	}

	if(_pObj->m_bAutoVarDirty)
	{
		int Flags = 0;
//		if(pCD->m_iPlayer != -1)
//			Flags = 0x1000;
		if (pCD->AutoVar_Unpack(pD, _pWClient->GetMapData(), Flags))
			pCD->m_Burnable.m_Value.OnClientUpdate(_pObj, _pWClient);
	}
	
	pCD->m_Control_Look_Wanted = GetLook(_pObj->GetPositionMatrix());
	pCD->Char_UpdateLook(0.0f); // FIXME: UPDATELOOK

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
/*
	// Did we teleport?
	if (pCD->m_LastTeleportTick == _pWClient->GetGameTick())
	{
		const CMat4Dfp32& OldPos = _pObj->GetLastPositionMatrix();
		CMat4Dfp32 NewPos; // Need to calc new pos from localpos (m_Pos is not set yet)
		_pObj->GetLocalPositionMatrix().Multiply(_pWClient->Object_GetPositionMatrix(_pObj->GetParent()), NewPos);

//		M_TRACE("client teleport, tick: %d\n", _pWClient->GetGameTick());
//		M_TRACE(" - old pos: %s\n", OldPos.GetRow(3).GetString().Str());
//		M_TRACE(" - new pos: %s\n", NewPos.GetRow(3).GetString().Str());

		// Relocate cloth to new position
		for (int i = 0; i < pCD->m_lCharacterCloth.Len(); i++)
			pCD->m_lCharacterCloth[i].ResetCloth(
	}
*/

	return (uint8*)pD - _pData;
}

void CWObject_Character::OnClientLoad(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	CWObject_RPG::OnClientLoad(_pObj, _pWorld, _pFile, _pWData, _Flags);

	CWO_Character_ClientData *pCD = GetClientData(_pObj);
	pCD->AutoVar_Read(_pFile, _pWData);
	pCD->m_InventoryInfo.Read(_pFile);

/*
#ifndef ANIMGRAPH2_ENABLED
	//GetPackedAGIState(_pObj)->Read(_pFile);
#endif
*/
}

void CWObject_Character::OnClientSave(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	CWObject_RPG::OnClientSave(_pObj, _pWorld, _pFile, _pWData, _Flags);

	CWO_Character_ClientData *pCD = GetClientData(_pObj);
	pCD->AutoVar_Write(_pFile, _pWData);
	pCD->m_InventoryInfo.Write(_pFile);

/*
#ifndef ANIMGRAPH2_ENABLED
	//GetPackedAGIState(_pObj)->Write(_pFile);
#endif
*/
}

