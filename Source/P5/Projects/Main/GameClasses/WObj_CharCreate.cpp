/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Character initialization code
					
	Contents:		OnCreate
					OnEvalKey
					OnFinishEvalKey
					OnInitInstance
					OnIncludeClass
					OnIncludeTemplate
					InitClientObjects
					etc..
\*____________________________________________________________________________________________*/
#include "PCH.h"

#include "WObj_Char.h"

#include "../GameWorld/WServerMod.h"
#include "../../Shared/MOS/Classes/GameContext/WGameContext.h"
#include "../../Shared/MOS/XR/XRBlockNav.h"
#include "../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_Game.h"
#include "../../../Shared/MOS/Classes/GameWorld/WAnimGraph2Instance/Wag2i.h"
#include "../../../Shared/MOS/Classes/GameWorld/WAnimGraph2Instance/Wag2_clientdata.h"
#include "../../../Shared/MOS/Classes/GameWorld/FrontEnd/WFrontEnd.h"

#include "CConstraintSystem.h"
#include "WObj_AI/AICore.h"
#include "WObj_Misc/WObj_ActionCutscene.h"
#include "WObj_Misc/WObj_BlackHole.h"
#include "WObj_Misc/WObj_DarklingSpawn.h"
#include "WObj_Misc/WObj_EffectSystem.h"
#include "WObj_Misc/WObj_TentacleSystem.h"
#include "WRPG/WRPGChar.h"
#include "WRPG/WRPGFist.h"



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CCharDialogueItems
|__________________________________________________________________________________________________
\*************************************************************************************************/
bool CCharDialogueItems::Parse(uint32 _KeyHash, const CStr& _KeyValue)
{
	switch (_KeyHash)
	{
	case MHASH5('APPR','OACH','DIAL','OGUE','ITEM'): // "APPROACHDIALOGUEITEM"   (legacy support)
		m_Approach.Set(_KeyValue, false);
		return true;

	case MHASH6('DIAL','OGUE','ITEM','_APP','ROAC','H'): // "DIALOGUEITEM_APPROACH"
		m_Approach.Set(_KeyValue, true);
		return true;

	case MHASH7('DIAL','OGUE','ITEM','_APP','ROAC','H_SC','ARED'): // "DIALOGUEITEM_APPROACH_SCARED"
		m_ApproachScared.Set(_KeyValue, true);
		return true;

	case MHASH6('DIAL','OGUE','ITEM','_THR','EATE','N'): // "DIALOGUEITEM_THREATEN"
		m_Threaten.Set(_KeyValue, true);
		return true;

	case MHASH5('DIAL','OGUE','ITEM','_IGN','ORE'): // "DIALOGUEITEM_IGNORE"
		m_Ignore.Set(_KeyValue, true);
		return true;

	case MHASH5('DIAL','OGUE','ITEM','_TIM','EOUT'): // "DIALOGUEITEM_TIMEOUT"
		m_Timeout.Set(_KeyValue, true);
		return true;

	case MHASH5('DIAL','OGUE','ITEM','_EXI','T'): // "DIALOGUEITEM_EXIT"
		m_Exit.Set(_KeyValue, true);
		return true;
	}
	return false;
}

void CCharDialogueItems::Override(const CCharDialogueItems& _Other)
{
	if (_Other.m_Approach.IsValid())       m_Approach = _Other.m_Approach;
	if (_Other.m_ApproachScared.IsValid()) m_ApproachScared = _Other.m_ApproachScared;
	if (_Other.m_Threaten.IsValid())       m_Threaten = _Other.m_Threaten;
	if (_Other.m_Ignore.IsValid())         m_Ignore = _Other.m_Ignore;
	if (_Other.m_Timeout.IsValid())        m_Timeout = _Other.m_Timeout;
	if (_Other.m_Exit.IsValid())           m_Exit = _Other.m_Exit;
}

void CCharDialogueItems::Read(CCFile* _pFile)
{
	uint8 FlagsIsPlayer = 0;
	_pFile->ReadLE(FlagsIsPlayer);
	m_Approach.m_bIsPlayer =       (FlagsIsPlayer & M_Bit(0)) != 0;
	m_ApproachScared.m_bIsPlayer = (FlagsIsPlayer & M_Bit(1)) != 0;
	m_Threaten.m_bIsPlayer =       (FlagsIsPlayer & M_Bit(2)) != 0;
	m_Ignore.m_bIsPlayer =         (FlagsIsPlayer & M_Bit(3)) != 0;
	m_Timeout.m_bIsPlayer =        (FlagsIsPlayer & M_Bit(4)) != 0;
	m_Exit.m_bIsPlayer =           (FlagsIsPlayer & M_Bit(5)) != 0;

	_pFile->ReadLE(m_Approach.m_ItemHash);
	_pFile->ReadLE(m_ApproachScared.m_ItemHash);
	_pFile->ReadLE(m_Threaten.m_ItemHash);
	_pFile->ReadLE(m_Ignore.m_ItemHash);
	_pFile->ReadLE(m_Timeout.m_ItemHash);
	_pFile->ReadLE(m_Exit.m_ItemHash);
}

void CCharDialogueItems::Write(CCFile* _pFile) const
{
	uint8 FlagsIsPlayer = 0;
	FlagsIsPlayer |= (m_Approach.m_bIsPlayer       ? M_Bit(0) : 0);
	FlagsIsPlayer |= (m_ApproachScared.m_bIsPlayer ? M_Bit(1) : 0);
	FlagsIsPlayer |= (m_Threaten.m_bIsPlayer       ? M_Bit(2) : 0);
	FlagsIsPlayer |= (m_Ignore.m_bIsPlayer         ? M_Bit(3) : 0);
	FlagsIsPlayer |= (m_Timeout.m_bIsPlayer        ? M_Bit(4) : 0);
	FlagsIsPlayer |= (m_Exit.m_bIsPlayer           ? M_Bit(5) : 0);
	_pFile->WriteLE(FlagsIsPlayer);

	_pFile->WriteLE(m_Approach.m_ItemHash);
	_pFile->WriteLE(m_ApproachScared.m_ItemHash);
	_pFile->WriteLE(m_Threaten.m_ItemHash);
	_pFile->WriteLE(m_Ignore.m_ItemHash);
	_pFile->WriteLE(m_Timeout.m_ItemHash);
	_pFile->WriteLE(m_Exit.m_ItemHash);
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Character
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWO_Character_ClientData* CWObject_Character::GetClientData(CWObject_CoreData* _pObj)
{
	CWO_Character_ClientData* pCD = safe_cast<CWO_Character_ClientData>((CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	return pCD;
}
const CWO_Character_ClientData* CWObject_Character::GetClientData(const CWObject_CoreData* _pObj)
{
	const CWO_Character_ClientData* pCD = safe_cast<const CWO_Character_ClientData>((const CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	return pCD;
}

CXR_SkeletonInstance* CWObject_Character::GetClientSkelInstance(CWO_Character_ClientData* _pCD, int _iSkelInst)
{
	CXR_SkeletonInstance *pSI = _pCD->m_lspSkelInst[_iSkelInst];
	return pSI;
}

CWAG2I_Mirror* CWObject_Character::GetAG2IMirror(CWObject_CoreData* _pObj)
{
	CWAG2I_Mirror* pAGIS = safe_cast<CWAG2I_Mirror>((CReferenceCount*)_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_AGISTATE]);
	return pAGIS;
}
const CWAG2I_Mirror* CWObject_Character::GetAG2IMirror(const CWObject_CoreData* _pObj)
{
	return safe_cast<CWAG2I_Mirror>((CReferenceCount*)((CWObject_CoreData*)_pObj)->m_lspClientObj[PLAYER_CLIENTOBJ_AGISTATE]);
}


int CWObject_Character::InitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	MAUTOSTRIP(CWObject_Character_InitClientObjects, 0);
	MSCOPE(CWObject_Character::InitClientObjects, CHARACTER);

	CWO_Character_ClientData* pCD = GetClientData(_pObj);
	M_ASSERT(pCD, "!");
	if (!pCD)
		return false;

	{
//		pCD->m_Anim_Look.Create(_pObj->GetPositionMatrix());
//		pCD->m_Anim_LastLook.Create(_pObj->GetPositionMatrix());

		// FIXME: UPDATELOOK
//		pCD->m_Anim_Look.Create(pCD->m_Control_Look);
//		pCD->m_Anim_LastLook.Create(pCD->m_Control_Look);

		CMat4Dfp32 LookMat; pCD->m_Control_Look.CreateMatrixFromAngles(0, LookMat);
		pCD->m_Anim_Look.Create(LookMat);
		pCD->m_Anim_LastLook.Create(LookMat);

		for(int i = 0; i < PLAYER_NUMANIMCACHESLOTS; i++)
		{
			MRTC_SAFECREATEOBJECT_NOEX(spSkel, "CXR_SkeletonInstance", CXR_SkeletonInstance);
			pCD->m_lspClientData[i] = spSkel;
			if (!spSkel)
			{
				_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA] = NULL;
				return false;
			}
		}
	}
	
/*	if (!_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_SKELINSTANCE])
	{
		MRTC_SAFECREATEOBJECT_NOEX(spSkel, "CXR_SkeletonInstance", CXR_SkeletonInstance);
		_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_SKELINSTANCE] = spSkel;
	}*/
	if (!_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_AGISTATE])
	{
		_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_AGISTATE] = MNew1(CWAG2I_Mirror,2);
		CWAG2I_Mirror* pPackedAGIState = GetAG2IMirror(_pObj);
		if (pPackedAGIState == NULL)
			return false;
	}
	
	if (!_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]/* ||
		!_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_SKELINSTANCE]*/) return false;
	
	return true;
}

void CWObject_Character::OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	// Check if we need to (re)allocate client data
	CReferenceCount* pData = _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA];
	if (!TDynamicCast<CWO_Character_ClientData>(pData))
	{
		CWO_Character_ClientData* pCD = MNew(CWO_Character_ClientData);
		if (!pCD)
			Error_static("CWObject_Character", "Could not allocate client data!");

		_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA] = pCD;
		pCD->Clear(_pObj, _pWPhysState);
	}

	if (!InitClientObjects(_pObj, _pWPhysState))
		Error_static("CWObject_Character", "InitClientObjects failed");
}




void CWObject_Character::OnCreate()
{
	MAUTOSTRIP(CWObject_Character_OnCreate, MAUTOSTRIP_VOID);

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
/*	m_Look = 0;
	m_Move = 0;
	m_Press = 0;
	m_Released = -1;*/

//	m_ClientFlags |= PLAYER_CLIENTFLAGS_THIRDPERSON; // Added by Mondelore.

	// Default is to render all models.
/*	m_SpawnDuration = 0;
	m_iSpawnAnim = -1;
	m_iSpawnSound = 0;

	m_iTimeoutSound = 0;*/

	m_bHideWhenDead = false;

	//m_TimeoutTick = 0;

	m_iSearch = 0;

	Char_SetPhysType(this, PLAYER_PHYS_VOID);
	pCD->m_iPlayer = -1;

	m_liTeams.Clear();
	m_lTeamFromKeys.Clear();

	m_StatusInfoCountdown = 0;
	m_PlayerInfoCountdown = 0;
	m_GameInfoCountdown = 0;

	m_Player.m_iTemporaryItemType = RPG_ITEMTYPE_UNDEFINED;
	m_Player.m_iTemporaryItemLastTick = -1;
	m_Player.m_iTemporaryItemSender = 0;
	m_Player.m_bSpecialClass = false;
	m_Player.m_LastInfoscreenPress = -1;

	m_bForceReleaseAttack2 = false;
	m_GrabDifficulty = 0;
	m_FightBBSize = 0;

	m_bDisableQuicksave = false;
	m_MountFlags = 0;

	m_LastHurtSoundTick = 0;

	m_MediumResist = 1.0f;
	m_Mass = 75.0f;
	m_Density = 1.05f;

	m_bFirst = true;

	m_ShieldIgnoreDamageEndTick = 0;
	
	//m_RespawnTimeoutTicks = 0;
	m_FallDmgTimeoutTicks = 0;

#ifndef M_RTM
	m_bPhysLog = 0;
	m_NoClipSpeedFactor = 1.0f;
#endif

	m_PhysAttrib.m_StepSize = PLAYER_STEPSIZE;
	m_PhysAttrib.m_Friction = 0.0f;

	pCD->m_AnimOOSBase = MRTC_RAND();

	pCD->m_Phys_Width = 10;
	pCD->m_Phys_Height = 28;
	pCD->m_SwimHeight = 41.0f;
	pCD->m_Phys_HeightCrouch = 16;
	pCD->m_Phys_DeadRadius = 8;
	pCD->m_Phys_SpectatorSize = 8;

	pCD->m_BackPlane = 5000;
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys && pSys->GetEnvironment())
		pCD->m_BackPlane = pSys->GetEnvironment()->GetValuef("VP_BACKPLANE", 5000.0f);

	m_iLastUsed = -1;
	m_LastUsedTick = 0;
	m_LastNotFjuffed = 0;

	m_Player.m_Map_ItemType = -1;
	m_Player.m_Map_Visited = 0;

	m_Flags = 0;
	//m_SpecialGrab = 0;
	m_GrabbedBody = 0;

	m_NextGUIScreen = 0;

	m_DarknessDrain = 0;
	m_AverageLightIntensity = 0.0f;	
	//m_DarknessPowersAvailable = ~0;	// Mask that controls which darkness powers the player can use
	m_bNonPlayerDarknessUser = false;

	// Hmm, do invalid or straight mapping from start?
	for (int32 i = 0; i < 4; i++)
		m_lDarklingMapping[i] = i;

	m_lControlledBots.Clear();
	m_iBotControl = 0;
	m_iController = 0;
	m_spAI = MNew(CAI_Core);
	m_spAI->Init(this, m_pWServer);

	m_iPendingItemModelUpdate[0] = -1;
	m_iPendingItemModelUpdate[1] = -1;
	m_iPendingItemModelUpdate[2] = -1;
	m_iPendingItemModelUpdate[3] = -1;
	
	m_PendingTimeLeapTick = -1;
	m_PendingCutsceneTick = -1;
	m_PendingCutsceneCamera = -1;

	m_LastCamera = GetPositionMatrix();

	m_CachedActivatePosition_Tick = -1;
	m_ActivatePosition = CVec3Dfp32(0.0f);
	m_ActivatePositionTick = -1;

	m_AIAimOffset = 56;
	m_AICrouchAimOffset = 35;

	pCD->m_GameTick = m_pWServer->GetGameTick();
	pCD->m_GameTime = PHYSSTATE_TICKS_TO_TIME(pCD->m_GameTick, m_pWServer);

	m_bAIControl = false;
	m_bResetControlState = false;

	// Reset last used item
	m_LastUsedItem = 0;
	m_LastUsedItemType = -1;
	m_WeaponUnequipTimer = 0;

	m_iStealthActivateTime = -1;
	m_iStealthTimer = -1;

	m_DropItemTick = 0;

	m_DamageEffect = "hitEffect_blood01";

	m_Player.m_SneakTriggerCount = 0;
	m_RaisedNoiseLevel = 0;
	m_RaisedVisibility = 0;

	// Sneak by light
	m_Player.m_bLightSneak = false;
	m_LightIntensity = 1.0f;

	m_iListener = 0;
	m_iSpeaker = 0;
	m_InterruptedItemHash = 0;

	// Reset everything that's got to do with tranquillizer
	m_TranquillizerOutTime = 20*20;			// 20 seconds of downtime when we've been 
	m_TranquillizerIncoming = 0;
	m_TranquillizerLevel = 0;
	m_TranquillizerTickCount = 0;

	m_iSoftSpotCount = 0;
	m_InitialPhysMode = PLAYER_PHYS_STAND;

	CWObject_Message Msg(OBJMSG_CHAR_CLEARREQUIREDCLEARANCELEVEL);
	m_pWServer->Message_SendToObject(Msg, m_iObject);

	m_MinSecurityClearance = 0;
	
	m_MsgContainer.Register(PLAYER_SIMPLEMESSAGE_ONSPAWN, "MSG_SPAWN");
	m_MsgContainer.Register(PLAYER_SIMPLEMESSAGE_ONDIE, "MSG_DIE");
	m_MsgContainer.Register(PLAYER_SIMPLEMESSAGE_ONTAKEDAMAGE, "MSG_TAKEDAMAGE");
	m_MsgContainer.Register(PLAYER_SIMPLEMESSAGE_ONPICKUPDROPWEAPON, "MSG_PICKUPWEAPON");
	m_MsgContainer.Register(PLAYER_SIMPLEMESSAGE_ONPICKUPDROPITEM, "MSG_PICKUPITEM");
	m_OnTakeDamageTypeMask = ~0; //0x7fffffff;
//	Char_SetControlMode(this, PLAYER_CONTROLMODE_ANIMATION);

	// RUNE: New health-system
	uint8 InitialHealth = 8;
	pCD->m_Health = InitialHealth;
	pCD->m_MaxHealth = InitialHealth;

	pCD->m_ThisAimHeight = 0.5f;

	pCD->m_iLaserBeam = m_pWServer->GetMapData()->GetResourceIndex_Model("LaserBeam");
	pCD->m_iLightCone = m_pWServer->GetMapData()->GetResourceIndex_Model("SpotLightVolume");

	pCD->m_SelectItemTimeOutTick = 0;
	pCD->m_iCurSelectedItem = 0;
	pCD->m_iNextValidSelectItem = 0;
	pCD->m_iCurEquippedItem = 0;

	m_spRagdollDropItems[0] = NULL;
	m_spRagdollDropItems[1] = NULL;

	// Gib stuff
	m_lGibInfos.Clear();
	m_GibExplosionOrigin = CVec3Dfp32(0.0f);
	m_GibExplosionParams[0] = 10.0f;	// Force: 10 units per tick speed at ground zero 
	m_GibExplosionParams[1] = 32.0f;	// Radius: Blast tapers off to 0 at 1 meter
	m_GibExplosionParams[2] = 0;		

	m_iDarkness_BlackHole = 0;
	m_iDarkness_Drain_2 = 0;
	m_iDarkness_MouthSmoke = 0;
	m_iDarkness_CreepingDark = 0;

	m_iChainObj = 0;
	m_iLastAutoUseObject = 0;
	m_GrabbedObject.Release();

	pCD->m_SpawnTick = m_pWServer->GetGameTick();
	
	m_GunplayUnsafeTick = m_pWServer->GetGameTick();
	// How long to wait before attempting to gunplay
	m_GunplaySafeDelay = 0;
	m_GunplaySpread = 0;
	m_GunplayReactivateDelay = 0;
	m_LastDarklingSpawnCheck = 0;
	m_iBestDarklingSpawn = 0;
	m_LastButton1Release = -100;

	m_DarknessFlags = DARKNESS_FLAGS_HEART;
	m_SaveParams = 0;

	m_spDarknessLightMeter = NULL;

	m_RagdollHash = MHASH2('DEFA','ULT');
}

void CWObject_Character::OnDestroy()
{
	OnShutOffAllLights();
	if (m_spAI)
	{	//*** NOTE: Should be GUID based ***
		m_spAI->m_KB.Global_RemoveDead(m_iObject);
	}

	CWO_Character_ClientData* pCD = GetClientData(this);

	m_spDarknessLightMeter = NULL;

	if (pCD->m_iDarkness_Tentacles)
		m_pWServer->Object_Destroy(pCD->m_iDarkness_Tentacles);

	if (m_iDarkness_BlackHole)
		m_pWServer->Object_Destroy(m_iDarkness_BlackHole);

    if (m_iDarkness_Drain_2)
		m_pWServer->Object_Destroy(m_iDarkness_Drain_2);

	if (m_iDarkness_MouthSmoke)
		m_pWServer->Object_Destroy(m_iDarkness_MouthSmoke);

	if (m_iDarkness_CreepingDark)
		m_pWServer->Object_Destroy(m_iDarkness_CreepingDark);

	if (m_iChainObj)
		m_pWServer->Object_Destroy(m_iChainObj);

	//Physcluster holds ragdoll, drop the constraints
	if	( m_pPhysCluster )
	{	
		uint32 liConstraints[WOBJECT_MAX_CONSTRAINTS];
		uint16 nConstraints = m_liConnectedToConstraint.Len();
		memcpy(liConstraints,m_liConnectedToConstraint.GetBasePtr(),sizeof(uint) * nConstraints);
		for(uint i = 0;i < nConstraints;i++)
			m_pWServer->Phys_RemoveConstraint( liConstraints[i] );
		CleanPhysCluster();
	}
}

static void NukeLight(int _GUID, CXR_SceneGraphInstance* _pSGI)
{
	int iLight = _pSGI->SceneGraph_Light_GetIndex(_GUID);
	if (iLight)
		_pSGI->SceneGraph_Light_Unlink(iLight);
}

void CWObject_Character::OnShutOffAllLights()
{
	CXR_SceneGraphInstance* pSGI = m_pWServer->World_GetSceneGraphInstance();
	if(!pSGI)
		return;

	NukeLight(m_iObject * 5 + 0x4002, pSGI);
	NukeLight(m_iObject * 5 + 0x4003, pSGI);
}

void CWObject_Character::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	MAUTOSTRIP(CWObject_Character_OnIncludeClass, MAUTOSTRIP_VOID);

// Head gibbing stuff
//	_pWData->GetResourceIndex_Model("Characters\\sever\\sever");
//	_pWData->GetResourceIndex_Model("Characters\\sever\\sever_head");
//	_pWData->GetResourceIndex_Class("hitEffect_CriticalHeadShot");
//	_pWData->GetResourceIndex_Class("weapon_head");
	
	_pWData->GetResourceIndex_Model("Sprite");
	_pWData->GetResourceIndex_Class("ActionCutscenePickup");
	_pWData->GetResourceIndex_Class("DroppableItem");
	_pWData->GetResourceIndex_Class("GibSystem");
	_pWData->GetResourceIndex_Class("CreepingDark");
	_pWData->GetResourceIndex_Class("DarklingSpawn");
	_pWData->GetResourceIndex_Class("LampSwitch");
	_pWData->GetResourceIndex_Class("Television");
	_pWData->GetResourceIndex_Class("UseProxy");
	_pWData->GetResourceIndex_Class("CharPlayer");
	_pWData->GetResourceIndex_Class("CharNPC");
	_pWData->GetResourceIndex_Class("CharDarkling");
	_pWData->GetResourceIndex_Class("Light");
	_pWData->GetResourceIndex_Class("Light2");
	//_pWData->GetResourceIndex_Class("Wind");
	_pWData->GetResourceIndex_Class("EffectSystem");
	_pWData->GetResourceIndex_Class("BlackHole");
	_pWData->GetResourceIndex_Class("SwingDoor");

	_pWData->GetResourceIndex_Model("LaserBeam");
	_pWData->GetResourceIndex_Model("SpotLightVolume");
	_pWData->GetResourceIndex_Registry("Registry\\Map");

	_pWData->GetResourceIndex_Class("hitEffect_blood01");
	_pWData->GetResourceIndex_Class("hitEffect_bloodspurt01");

	//_pWData->GetResourceIndex_Model("Sprite:projmap_laserdot");
	_pWData->GetResourceIndex_Model("Sprite:laserdot");

	// Darkness sounds
	_pWData->GetResourceIndex_Sound("Gam_drk_summon");
	_pWData->GetResourceIndex_Sound("Gam_drk_unsumm");
	_pWData->GetResourceIndex_Sound("Gam_drk_drk01");
	_pWData->GetResourceIndex_Sound("Gam_drk_drk02");
	_pWData->GetResourceIndex_Sound("Gam_drk_over01");
	_pWData->GetResourceIndex_Sound("Gam_drk_over02");
	_pWData->GetResourceIndex_Sound("gam_drk_drain_2d_start");
	_pWData->GetResourceIndex_Sound("gam_drk_drain_2d_stop");
	_pWData->GetResourceIndex_Sound("gam_drk_drain_2d_loop");
	_pWData->GetResourceIndex_Sound("Gam_drk_crp_fail");
	_pWData->GetResourceIndex_Sound("Gam_drk_fail");

	_pWData->GetResourceIndex_Sound("fol_bodyfall_full");
	_pWData->GetResourceIndex_Sound("fol_bodyfall_soft");
	_pWData->GetResourceIndex_Sound("fol_bodyfall_head");
	_pWData->GetResourceIndex_Sound("fol_bodyfall_arm");
	_pWData->GetResourceIndex_Sound("fol_bodyfall_leg");
	
	// Hurt responses
	_pWData->GetResourceIndex_Sound("D_Hurts_Jackie_Reaction_01");
	_pWData->GetResourceIndex_Sound("D_Hurts_Jackie_Breath_01");
	_pWData->GetResourceIndex_Sound("D_Hurts_Darkl_Reaction_01");
	_pWData->GetResourceIndex_Sound("D_Hurts_Darkl_Breath_01");

	//Multiplayer quad sound
	_pWData->GetResourceIndex_Sound("Env_Itm_Mult_Damage_Hit");
	_pWData->GetResourceIndex_Sound("Env_Itm_Mult_Shieldactivate");
	
	// Pager sounds
	_pWData->GetResourceIndex_Sound("Gam_pla_page01");

	// Darkling sounds
	_pWData->GetResourceIndex_Sound("Env_Fire_Sizzleloop01");

	// Ancient weapons
#ifndef M_RTM
	if (!D_MXDFCREATE)
	{
		CRPG_Object::IncludeRPGClass("weapon_Ancient_1", _pWData, _pWServer, true);
		CRPG_Object::IncludeRPGClass("weapon_Ancient_2", _pWData, _pWServer, true);
	}
#endif

	CWObject_TentacleSystem::OnIncludeClass(_pWData, _pWServer);
	CWObject_EffectSystem::OnIncludeClass(_pWData, _pWServer);
	CWObject_BlackHole::OnIncludeClass(_pWData, _pWServer);

	CWO_Burnable::OnIncludeClass(_pWData, _pWServer);
}


void CWObject_Character::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* _pWServer)
{
	MAUTOSTRIP(CWObject_Character_OnIncludeTemplate, MAUTOSTRIP_VOID);
	MSCOPE(CWObject_Character::OnIncludeTemplate, CHARACTER);

	CWObject_RPG::OnIncludeTemplate(_pReg, _pMapData, _pWServer);

	{
		MSCOPE(CAI_Core::OnIncludeTemplate, CHARACTER);
		CAI_Core::OnIncludeTemplate(_pReg, _pMapData);
	}

/*	int iPicmip = PLAYER_PICMIP_CHARACTER;
	if(_pReg->GetValuei("HIGHTEXRES") != 0)
		iPicmip = PLAYER_PICMIP_PLAYER;

	CFStr sModel = _pReg->GetValue("MODEL");
	if(sModel != "")
	{
		// Picmip group hack. This is made to make sure each character has the correct picmip group, so
		// that the player can have a higher texture resolution than other characters.
		// But since this is on the server, it will only work with local clients.
		int iModel = _pMapData->GetResourceIndex_Model(sModel.GetStrMSep(";, "));
		if(iModel > 0)
		{
			CXR_Model *pModel = _pMapData->GetResource_Model(iModel);
			if(pModel)
				pModel->SetParam(CXR_MODEL_PARAM_SETPICMIPGROUP, iPicmip);
		}
	}
*/
	for(int i = 0; i < _pReg->GetNumChildren(); ++i)
	{
		if(_pReg->GetName(i).CompareSubStr("GIBPART") == 0)
		{
			CRegistry *pGib = _pReg->GetChild(i);
			IncludeModelFromKey("MODEL", pGib, _pMapData);
		}
	}

	CRegistry *pAnimGraph2Reg = _pReg->FindChild("ANIMGRAPH2");
	if(pAnimGraph2Reg)
	{
		MSCOPE(GetResourceIndex_AnimGraph, CHARACTER);
		int32 NumChildren = pAnimGraph2Reg->GetNumChildren();
		for (int32 i = 0; i < NumChildren; i++)
		{
			CStr KeyName = pAnimGraph2Reg->GetName(i);
			if (KeyName.CompareSubStr("GRAPH") == 0)
				_pMapData->GetResourceIndex_AnimGraph2(pAnimGraph2Reg->GetValue(i));
			else
				_pMapData->GetResourceIndex_AnimGraph2(KeyName);
		}
	}

	IncludeAnimFromKey("ANIMLIPSYNC", _pReg, _pMapData);

	IncludeClassFromKey("DEATH_EFFECT", _pReg, _pMapData);
	IncludeClassFromKey("DAMAGE_EFFECT", _pReg, _pMapData);

	// Include darkness stuff
	IncludeClassFromKey("DARKNESS_TENTACLES", _pReg, _pMapData);
	IncludeClassFromKey("DARKNESS_VISIONPARTICLES", _pReg, _pMapData);
	IncludeClassFromKey("DARKNESS_DARKLINGSPAWN", _pReg, _pMapData);
	IncludeClassFromKey("DARKNESS_BLACKHOLE", _pReg, _pMapData);
	IncludeClassFromKey("DARKNESS_CREEPINGDARK", _pReg, _pMapData);
	IncludeClassFromKey("DARKNESS_DRAIN_2", _pReg, _pMapData);
	IncludeModelFromKey("DARKNESS_DRAIN_1", _pReg, _pMapData);
	
	// Include mouth smoke model if needed
	if (_pReg)
	{
		CRegistry* pPowerupReg = _pReg->FindChild("DARKNESSPOWERUP");
		if (pPowerupReg)
			_pMapData->GetResourceIndex_Class("fx_mouthsmoke");
	}

	// Include the character that might be spawned from this
	//IncludeClassFromKey("DARKLINGINHABITANT",_pReg,_pMapData);
	// Not the most elegant solution but works (for now...) part 1
	CRegistry *pDarklingInhabitant = _pReg->Find("DARKLINGINHABITANT");
	if (pDarklingInhabitant)
	{
		CMat4Dfp32 Mat;
		Mat.Unit();
		Mat.k[3][2] = -100000;
		int iObj = _pWServer->Object_Create(pDarklingInhabitant->GetThisValue(),Mat);
		CWObject* pObj = _pWServer->Object_Get(iObj);
		if (pObj)
		{	// "Not the most elegant solution but works (for now...)" part 2
			// (Lurigt varre att hitta denna Olle)
			CWObject_Character* pChar = safe_cast<CWObject_Character>(pObj);
			if (pChar)
			{	// Clear the list of teamnames
				pChar->m_lTeamFromKeys.Clear();
			}
			pObj->OnSpawnWorld();
			pObj->OnSpawnWorld2();
		}
		_pWServer->Object_DestroyInstant(iObj);
	}
	CRegistry *pAvailableDarklings = _pReg->Find("AVAILABLEDARKLINGS");
	if (pAvailableDarklings)
	{
		int32 Len = pAvailableDarklings->GetNumChildren();
		for (int32 i = 0; i < Len; i++)
		{
			const CRegistry* pChild = pAvailableDarklings->GetChild(i);
			CMat4Dfp32 Mat;
			Mat.Unit();
			Mat.k[3][2] = -100000;
			if (pChild)
			{
				// For each child create and then remove it (so all anims needed gets precached...)
				int iObj = _pWServer->Object_Create(pChild->GetThisName(),Mat);
				CWObject* pObj = _pWServer->Object_Get(iObj);
				if (pObj)
				{	// "Not the most elegant solution but works (for now...)" part 2
					// (Lurigt varre att hitta denna Olle)
					CWObject_Character* pChar = safe_cast<CWObject_Character>(pObj);
					if (pChar)
					{	// Clear the list of teamnames
						pChar->m_lTeamFromKeys.Clear();
					}
					pObj->OnSpawnWorld();
					pObj->OnSpawnWorld2();
				}
				_pWServer->Object_DestroyInstant(iObj);
				//_pMapData->GetResourceIndex_Class();
				Mat.k[3][2] += 100;
			}
		}
	}

	CRPG_Object::IncludeRPGClassFromKey("DROPITEM", _pReg, _pMapData, _pWServer, true);

	// precache "MAP" surfaces from levelkeys (still used?)
	CRegistry* pLevelKeys_Player = _pWServer->Registry_GetLevelKeys("PLAYER");
	if (pLevelKeys_Player)
	{
		CStr Val = pLevelKeys_Player->GetValue("MAP");
		if (!Val.IsEmpty())
			_pWServer->GetMapData()->GetResourceIndex_Surface(Val.GetStrSep(","));
	}

	CWO_Burnable::OnIncludeTemplate(_pReg, _pMapData, _pWServer);
}

static void PrecacheSurface(CXW_Surface* _pSurf, CXR_Engine* _pEngine)
{
	MAUTOSTRIP(PrecacheSurface, MAUTOSTRIP_VOID);
	CXW_Surface* pSurf = _pSurf->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
	pSurf->InitTextures(false);
	pSurf->SetTextureParam(CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
}

static void PrecacheSurface(const char* _pSurf, CXR_Engine* _pEngine)
{
	MAUTOSTRIP(PrecacheSurface_2, MAUTOSTRIP_VOID);
	CXW_Surface* pSurf = _pEngine->m_pSC->GetSurface(_pSurf);
	if (pSurf)
		PrecacheSurface(pSurf, _pEngine);
}

static void PrecacheTexture(const char* _pTexture, CXR_Engine* _pEngine)
{
	int TxtID = _pEngine->m_pTC->GetTextureID(_pTexture);
	if (TxtID > 0)
		_pEngine->m_pTC->SetTextureParam(TxtID, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
}


void CWObject_Character::OnClientPrecacheClass(CWorld_Client* _pWClient, CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CWObject_Character_OnClientPrecacheClass, MAUTOSTRIP_VOID);
	PrecacheSurface("HoleSkin01_01", _pEngine);
	PrecacheSurface("HoleSkin02_01", _pEngine);
	PrecacheSurface("HoleSkin03_01", _pEngine);
	PrecacheSurface("bruise01", _pEngine);
	PrecacheSurface("bruise02", _pEngine);
	PrecacheSurface("bruise03", _pEngine);
	PrecacheSurface("bruise04", _pEngine);
	PrecacheSurface("bruise05", _pEngine);
	PrecacheSurface("char_chesthole", _pEngine);

	PrecacheSurface("GUI_SBZStamp", _pEngine);
	PrecacheSurface("GUI_HUD_demonarm", _pEngine);
//	PrecacheSurface("GUI_HUD_ancient", _pEngine);
	PrecacheSurface("GUI_HUD_creepingdark", _pEngine);
	PrecacheSurface("GUI_HUD_blackhole", _pEngine);
	PrecacheSurface("GUI_HUD_darkness", _pEngine);
	PrecacheSurface("GUI_quad", _pEngine);
	PrecacheSurface("GUI_shield", _pEngine);
	PrecacheSurface("GUI_healthboost", _pEngine);

	PrecacheSurface("DarkPowSmoke02", _pEngine);
	PrecacheSurface("laserdot", _pEngine);

	PrecacheTexture("dots", _pEngine);
	PrecacheTexture("Retina", _pEngine);
}


void CWObject_Character::OnInitInstance(const aint* _pParam, int _nParam)
{
	M_ASSERT(_nParam == 0, "Players should use CWObject_CharBase");
}

void CWObject_Character::OnSpawnWorld()
{
	MAUTOSTRIP(CWObject_Character_OnSpawnWorld, MAUTOSTRIP_VOID);
	CWObject_Player::OnSpawnWorld();

	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	/*if(pCD && pCD->m_iPlayer != -1)
	{
		int iClient = m_pWServer->Game_GetObject()->Player_GetClient(pCD->m_iPlayer);
		if (iClient >= 0) 
		{
			CRegistry* reg = m_pWServer->Registry_GetClientVar(iClient);
			reg->SetValuei("character_spawncount", 0);
		}
	}*/
	if (!pCD)
		return;

	// Go through each rpg item and use its tag function to tag animations that
	// we must have
	CWorld_ServerCore* pServer = safe_cast<CWorld_ServerCore>(m_pWServer);
	if (pServer->GetServerMode() & SERVER_MODE_SPAWNWORLD)
	{
		CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(m_pWServer);
		CWAG2I_Context AGContext(this,m_pWServer,pCD->m_GameTime);
		TArray<CXRAG2_Impulse> lImpulses;
		// Add "explore"
		lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,AG2_IMPULSEVALUE_WEAPONTYPE_UNARMED));
		lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,AG2_IMPULSEVALUE_WEAPONTYPE_UNARMEDCROUCH));
		// Demoonarm response
		lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_BEHAVIOR,AG2_IMPULSEVALUE_BEHAVIOR_DARKNESS_RESPONSE_KNIFE));
		// Creeping dark impulses
		lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,AG2_IMPULSEVALUE_CREEPINGDARK_USE));
		lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,AG2_IMPULSEVALUE_CREEPINGDARK_RESPONSE_ATTACK_CROUCH));
		lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,AG2_IMPULSEVALUE_CREEPINGDARK_RESPONSE_ATTACK_HIGH));
		// Melee responses
		lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,AG2_IMPULSEVALUE_GAMEPLAY_MELEERESPONSE));

		// LEDGE TEMP FOR RHD
		{
			TSelection<CSelection::LARGE_BUFFER> Selection;
			m_pWServer->Selection_AddClass(Selection, "Ledge");
			const int16* pSel;
			int nSel = m_pWServer->Selection_Get(Selection, &pSel);
			if (nSel)
				lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_LEDGE,0));
		}

		// Add gunkata moves
		TAP_RCD<GunKataMove> lGunKataMoves = m_lGunKataMoves;
		for (int32 i = 0; i < lGunKataMoves.Len(); i++)
			lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,lGunKataMoves[i].m_Impulse));

		CRPG_Object_Char* pChar = Char();
		CRPG_Object_Inventory* pInventory = (pChar ? pChar->GetInventory(RPG_CHAR_INVENTORY_WEAPONS) : NULL);

		if (pCD && pInventory)
		{
			int32 Len = pInventory->GetNumItems();
			for (int32 i = 0; i < Len; i++)
			{
				CRPG_Object_Item* pItem = pInventory->GetItemByIndex(i);
				//Skicka animgraph här istället....
				//ladda hela containrar (antar att man får spara namn i animlistan :/)
				if (pItem)
				{
					pItem->TagAnimationsForPrecache(&AGContext, pCD->m_AnimGraph2.GetAG2I());
				}
			}
		}

		// Only load acs for player
		if (pCD && pCD->m_iPlayer != -1)
		{
			// Load acs animations as well, select all acs classes(?) (or is it enough with base class?)
			// Call taganmforprecache at them as well
			TSelection<CSelection::LARGE_BUFFER> Selection;
			// Select the actioncutscene class(es) (actioncutscene, ladders, ledges, acs pickups)
			m_pWServer->Selection_AddClass(Selection, "ActionCutscene");
			m_pWServer->Selection_AddClass(Selection, "HealthStation");
			m_pWServer->Selection_AddClass(Selection, "LeverActionCutscene");
			m_pWServer->Selection_AddClass(Selection, "ValveActionCutscene");
			m_pWServer->Selection_AddClass(Selection, "Phys_Ladder");
			m_pWServer->Selection_AddClass(Selection, "Ledge");
			m_pWServer->Selection_AddClass(Selection, "ActionCutscenePickup");

			const int16* pSel;
			int nSel = m_pWServer->Selection_Get(Selection, &pSel);

			// Check if there were any actioncutscenes
			if (nSel > 0)
			{
				TArray<int32> liACS;
				for (int i = 0; i < nSel; i++)
				{
					CWObject_ActionCutscene* pACS = safe_cast<CWObject_ActionCutscene>(m_pWServer->Object_Get(pSel[i]));
					if (pACS)
					{
						pACS->TagAnimationsForPrecache(&AGContext, pCD->m_AnimGraph2.GetAG2I(),liACS);

					}
				}
				int32 iACSLen = liACS.Len();
				for (int32 i = 0; i < iACSLen; i++)
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_ACTIONCUTSCENE,liACS[i]));
			}

			// Find normal pickups?!?!? (CWObject_Item)
			Selection.Reset();
			// Select the actioncutscene class(es) (actioncutscene, ladders, ledges, acs pickups)
			m_pWServer->Selection_AddClass(Selection, "Item");

			nSel = m_pWServer->Selection_Get(Selection, &pSel);

			// Check if there were any actioncutscenes
			if (nSel > 0)
			{
				for (int i = 0; i < nSel; i++)
				{
					CWObject_Item* pItem = safe_cast<CWObject_Item>(m_pWServer->Object_Get(pSel[i]));
					if (pItem)
					{
						pItem->TagAnimationsForPrecache(&AGContext, pCD->m_AnimGraph2.GetAG2I());
					}
				}
			}

			// Extra items, that player might pickup or buy
			if (pServerMod)
			{
				const TArray<int32>& lItemAnimTypesNeeded = pServerMod->GetItemAnimTypesNeeded();
				int32 Len = lItemAnimTypesNeeded.Len();
				for (int32 i = 0; i < Len; i++)
				{
					int32 Needed = lItemAnimTypesNeeded[i];
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,Needed));
					// Add for crouch as well
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,Needed+AG2_IMPULSEVALUE_NUMWEAPONTYPES));
				}
			}

			CStr CheatItems = m_Player.m_CheatItems;
			while(CheatItems != "")
			{
				CStr AnimSetsNeeded;
				TArray<int32> lTypes;
				CRPG_Object::IncludeRPGClass(CheatItems.GetStrSep(","), m_pWServer->GetMapData(), m_pWServer, true, &lTypes);
				for (int32 i = 0; i < lTypes.Len(); i++)
				{
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,lTypes[i]));
					lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE,lTypes[i]+AG2_IMPULSEVALUE_NUMWEAPONTYPES));
				}
			}
		}

		{
			// Dialogue animations
			CWRes_Dialogue *pRes = GetDialogueResource(this, m_pWServer);
			if(pRes)
				pRes->TagAnimations(m_pWServer->GetMapData(), pCD->m_AnimGraph2.GetAG2I(), &AGContext);
		}

		// Temporary gameplay add for darkling
		if (pCD->IsDarkling())
			lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_GAMEPLAY,AG2_IMPULSEVALUE_GAMEPLAY_DARKLING_SPAWNPUSHJUMPUP));
		pCD->m_AnimGraph2.GetAG2I()->TagAnimSetFromImpulses(&AGContext,m_pWServer->GetMapData(),m_pWServer->m_spWData,lImpulses);

		// Register AnimList with servermod
		if (pServerMod)
		{
			int32 Len = pCD->m_AnimGraph2.GetAG2I()->GetNumResource_AnimGraph2();
			for (int32 i = 0; i < Len; i++)
				pServerMod->RegisterAnimGraph2(pCD->m_AnimGraph2.GetAG2I()->GetResourceIndex_AnimGraph2(i));
		}
	}


	m_MsgContainer.Precache(m_iObject, m_pWServer);
	int i;
	for(i = 0; i < m_lDialogueImpulses.Len(); i++)
		m_lDialogueImpulses[i].SendPrecache(m_iObject, m_pWServer);

	m_ImpulseContainer.SendPrecache(m_iObject, m_pWServer);

	//Set team indices. If any named teams do not exist, create new team objects
	for (i = 0; i < m_lTeamFromKeys.Len(); i++)
	{
		//Assume team names are unique
		CStr TeamName = m_lTeamFromKeys[i];
		int iTeam = m_pWServer->Selection_GetSingleTarget(TeamName);
		if (iTeam > 0)
		{
			//Team exists, add to own team list
			m_liTeams.Add(iTeam);
		}
		else
		{
			//Team does not exist, create
			iTeam = m_pWServer->Object_Create("Team");
			if (iTeam)
			{
				ConOutL(CStrF("WARNING: Team '%s' does not exist, creating it... (char: %s)", TeamName.Str(), GetName()));
				m_pWServer->Object_SetName(iTeam, TeamName);
				m_liTeams.Add(iTeam);
			}
		}
	}
	//Remove team name list
	m_lTeamFromKeys.Destroy();
	
	//Notify AI of world spawn
	m_spAI->OnSpawnWorld();

	// Use and desc names
	m_UseName = "§LCHAR_NAME_";
	m_UseName += GetTemplateName();
	m_DescName = "§LCHAR_DESC_";
	m_DescName += GetTemplateName();

	// Override player's backplane value with the one set in WorldSky..
	if (pCD->m_iPlayer != -1)
	{
		TSelection<CSelection::SMALL_BUFFER> Selection;
		m_pWServer->Selection_AddClass(Selection, "WorldSky");
		const int16* pSel = NULL;
		uint nSel = m_pWServer->Selection_Get(Selection, &pSel);
		for (uint i = 0; i < nSel; i++)
		{
			CWObject* pSky = m_pWServer->Object_Get(pSel[i]);
			if (pSky && pSky->m_iAnim1 != 0)
			{
				pCD->m_BackPlane = pSky->m_iAnim1;
				break;
			}
		}
	}

	// Init cached transforms to somewhat valid values
	m_bHeadMatUpdated = false;
	m_BaseMat = GetPositionMatrix();
	m_WeaponMat = GetPositionMatrix();
	m_HeadMat = GetPositionMatrix();
	m_WeaponMat.GetRow(3).k[2] += 48.0f;
	m_HeadMat.GetRow(3).k[2] += 48.0f;

}

void CWObject_Character::OnSpawnWorld2()
{
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
	m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
}


void CWObject_Character::SendOnSpawnMessages()
{
	if (!(m_Flags & PLAYER_FLAGS_WAITSPAWN))
		m_MsgContainer.SendMessage(PLAYER_SIMPLEMESSAGE_ONSPAWN, m_iObject, m_iObject, m_pWServer);
}



void CWObject_Character::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Character_OnEvalKey, MAUTOSTRIP_VOID);
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);

//TODO: Remove this - velocities should be [units / second] instead of [units / tick]
#define PHYSSTATE_CONVERTFROM20HZ(x) ((x) * 20.0f * m_pWServer->GetGameTickTime())

	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	int KeyValuei = KeyValue.Val_int();
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();
	
	switch (_KeyHash)
	{
	case MHASH2('PLAY','ERNR'): // "PLAYERNR"
		{
			pCD->m_iPlayer = KeyValuei;
			m_pWServer->Object_SetName(m_iObject, "$PLAYER");
			break;
		}

	case MHASH2('MODE','L'): // "MODEL"
		{
			CWObject_Player::OnEvalKey(_KeyHash, _pKey);

			// Find a matching face-setup file for the given model file
			CFStr Name = CFStrF("FaceSetup/%s.xsa", KeyValue.GetFilenameNoExt().Str());
			CFStr FileName = m_pWServer->GetMapData()->ResolveFileName("ANIM/" + Name);
			if (CDiskUtil::FileExists(FileName))
				pCD->m_iFacialSetup = m_pWServer->GetMapData()->GetResourceIndex_FacialSetup(Name);
			break;
		}
	case MHASH2('BASE','FOV'): // "BASEFOV"
		{
			pCD->m_BaseFov = KeyValuef;
			pCD->m_TargetFov = KeyValuef;
			break;
		}

	case MHASH3('PHYS','_WID','TH'): // "PHYS_WIDTH"
		{
			pCD->m_Phys_Width = KeyValuei;
			break;
		}
	
	case MHASH3('PHYS','_HEI','GHT'): // "PHYS_HEIGHT"
		{
			pCD->m_Phys_Height = KeyValuei;
			break;
		}

	case MHASH4('PHYS','_SWI','M_HE','IGHT'): // "PHYS_SWIM_HEIGHT"
		{
			pCD->m_SwimHeight = KeyValuef;
			break;
		}

	case MHASH5('PHYS','_HEI','GHT_','CROU','CH'): // "PHYS_HEIGHT_CROUCH"
		{
			pCD->m_Phys_HeightCrouch = KeyValuei;
			break;
		}

	case MHASH4('PHYS','_DEA','D_RA','DIUS'): // "PHYS_DEAD_RADIUS"
		{
			pCD->m_Phys_DeadRadius = KeyValuei;
			break;
		}

	case MHASH4('PHYS','_FRI','CTIO','N'): // "PHYS_FRICTION"
		{
			m_PhysAttrib.m_Friction = KeyValuef;
			break;
		}
	
	case MHASH4('PHYS','_STE','PSIZ','E'): // "PHYS_STEPSIZE"
		{
			m_PhysAttrib.m_StepSize = KeyValuef;
			break;
		}
	
	case MHASH3('PHYS','_TYP','E'): // "PHYS_TYPE"
		{
			m_InitialPhysMode = KeyValuei;
			break;
		}
	case MHASH5('MAXT','ARGE','TING','DIST','ANCE'): // "MAXTARGETINGDISTANCE"
		{
			pCD->m_MaxTargetingDistance = KeyValuef;
			break;
		}

	case MHASH2('RAGD','OLL'): // "RAGDOLL"
		{
			if( KeyValue != "0" )
			{
				m_RagdollHash = StringToHash(KeyValue);
				m_Flags |= PLAYER_FLAGS_RAGDOLL;
			}
			break;
		}

/*	
	case MHASH4('TELE','PORT','_EFF','ECT'): // "TELEPORT_EFFECT"
		{
			m_Teleport_Effect = KeyValue;
			m_pWServer->GetMapData()->GetResourceIndex_Class(m_Teleport_Effect);
			break;
		}
	case MHASH4('TELE','PORT','_SOU','ND'): // "TELEPORT_SOUND"
		{
			m_Teleport_iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
*/
/*	
	case MHASH4('SPAW','N_DU','RATI','ON'): // "SPAWN_DURATION"
		{
			m_SpawnDuration = KeyValuef * SERVER_TICKSPERSECOND;
			break;
		}
	case MHASH3('SPAW','N_AN','IM'): // "SPAWN_ANIM"
		{
			CWObject_Message Msg(OBJMSG_GAME_RESOLVEANIMHANDLE);
			Msg.m_pData = (char *)KeyValue;
			m_iSpawnAnim = m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());
			break;
		}
	case MHASH3('SPAW','N_SO','UND'): // "SPAWN_SOUND"
		{
			m_iSpawnSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH5('SPAW','N_EF','FECT','_CEN','TER'): // "SPAWN_EFFECT_CENTER"
		{
			m_SpawnEffectCenter = KeyValue;
			m_pWServer->GetMapData()->GetResourceIndex_Class(m_SpawnEffectCenter);
			break;
		}
	case MHASH5('SPAW','N_EF','FECT','_GRO','UND'): // "SPAWN_EFFECT_GROUND"
		{
			m_SpawnEffectGround = KeyValue;
			m_pWServer->GetMapData()->GetResourceIndex_Class(m_SpawnEffectGround);
			break;
		}

	case MHASH4('TIME','OUT_','SOUN','D'): // "TIMEOUT_SOUND"
		{
			m_iTimeoutSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH6('TIME','OUT_','EFFE','CT_C','ENTE','R'): // "TIMEOUT_EFFECT_CENTER"
		{
			m_TimeoutEffectCenter = KeyValue;
			break;
		}
	case MHASH6('TIME','OUT_','EFFE','CT_G','ROUN','D'): // "TIMEOUT_EFFECT_GROUND"
		{
			m_TimeoutEffectGround = KeyValue;
			break;
		}
*/

	case MHASH5('DAMA','GE_M','SG_T','YPEM','ASK'): // "DAMAGE_MSG_TYPEMASK"
		{	// What damage types will trigger OnTakeDamage
			m_OnTakeDamageTypeMask = (uint32)KeyValue.TranslateFlags(CRPG_Object::ms_DamageTypeStr);
			break;
		}
	case MHASH4('DAMA','GE_E','FFEC','T'): // "DAMAGE_EFFECT"
		{
			m_DamageEffect = KeyValue;
			m_pWServer->GetMapData()->GetResourceIndex_Class(m_DamageEffect);
			break;
		}

	case MHASH3('DEAT','H_EF','FECT'): // "DEATH_EFFECT"
		{
			m_DeathEffect = KeyValue;
			m_pWServer->GetMapData()->GetResourceIndex_Class(m_DeathEffect);
			break;
		}

	case MHASH3('HIDE','WHEN','DEAD'): // "HIDEWHENDEAD"
		{
			m_bHideWhenDead = (KeyValuei != 0);
			break;
		}

/*	
	case MHASH3('BLOO','D_CO','LOR'): // "BLOOD_COLOR"
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if(pCD)
			{
				int32 r, g, b, a;

				CStr Value = KeyValue;
				r = Max(0, Min(255, Value.GetIntSep(",")));
				g = Max(0, Min(255, Value.GetIntSep(",")));
				b = Max(0, Min(255, Value.GetIntSep(",")));
				a = 0xFF;
				pCD->m_BloodColor = (a << 24) | (r << 16) | (g << 8) | (b << 0);
			}
			break;
		}
*/

	case MHASH3('LASE','RBEA','M'): // "LASERBEAM"
		{
			ClientFlags() |= PLAYER_CLIENTFLAGS_LASERBEAM;
			break;
		}

	case MHASH3('STUN','NABL','E'): // "STUNNABLE"
		{
			m_Flags |= PLAYER_FLAGS_STUNNABLE;
			break;
		}

	case MHASH3('FLAS','HLIG','HT'): // "FLASHLIGHT"
		{	// We don't want to evaluate keys without AI_ prefix i ai, 
			// therefore I do it here
			// KeyValuei be 0(off), 1(on) or 2(dynamic, depending on lightmeter)
			if (m_spAI)
			{
				if(KeyValuei == 2)
				{
					if (m_spAI->IsPlayer())
					{	// Player always use the fancy m_UseLightmeter 2
						if (m_spAI->m_UseLightmeter < 2)
						{
							m_spAI->m_UseLightmeter = 2;
						}
					}
					else
						if (m_spAI->m_UseLightmeter < 1)
						{
							m_spAI->m_UseLightmeter = 1;
						}
					m_spAI->m_bFlashlight = true;
				}
				else if(KeyValuei == 1)
				{
					if (!m_spAI->IsPlayer())
					{
						if (m_spAI->m_UseLightmeter > 0)
						{
							m_spAI->m_UseLightmeter = 0;
						}
						m_spAI->m_bFlashlight = false;
					}
					ClientFlags() |= PLAYER_CLIENTFLAGS_FLASHLIGHT;
				}
				else // KeyValuei == 0
				{
					if (!m_spAI->IsPlayer())
					{
						if (m_spAI->m_UseLightmeter > 0)
						{
							m_spAI->m_UseLightmeter = 0;
						}
						m_spAI->m_bFlashlight = false;
					}
					ClientFlags() &= ~PLAYER_CLIENTFLAGS_FLASHLIGHT;
				}
			}
			break;
		}

	case MHASH4('NONI','GHTV','ISIO','N'): // "NONIGHTVISION"
		{
			m_Flags |= PLAYER_FLAGS_NONIGHTVISION;
			break;
		}

	case MHASH3('BACK','PLAN','E'): // "BACKPLANE"
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if(pCD)
				pCD->m_BackPlane = KeyValuef;
			break;
		}

	case MHASH3('SKEL','ETON','TYPE'): // "SKELETONTYPE"
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if(pCD)
				pCD->m_Aim_SkeletonType = KeyValuei;
			break;
		}
	case MHASH2('GIBP','ARTS'): // "GIBPARTS"
		{
			m_lGibInfos.SetLen(_pKey->GetNumChildren());

			for(int g = 0; g < _pKey->GetNumChildren(); ++g)
			{
				const CRegistry* pGibKeys = _pKey->GetChild(g);

				CGibInfo& Gib = m_lGibInfos[g];
				FillChar(&Gib, sizeof(Gib), 0);

				for(int j = 0; j < pGibKeys->GetNumChildren(); ++j)
				{
					if(pGibKeys->GetName(j) == "MODEL")
					{
						Gib.m_iModel = m_pWServer->GetMapData()->GetResourceIndex_Model(pGibKeys->GetValue(j));
					}
					else if(pGibKeys->GetName(j) == "ORIGIN")
					{
						int32 Dest[3];
						pGibKeys->GetValueai(j, 3, Dest);
						Gib.m_Pos.k[0] = Dest[0];
						Gib.m_Pos.k[1] = Dest[1];
						Gib.m_Pos.k[2] = Dest[2];
	//					Gib.m_Pos.ParseString(pGibKeys->GetValue(j));
					}
					else if(pGibKeys->GetName(j) == "ANGLES")
					{
						CVec3Dfp32 Angles; Angles.ParseString(pGibKeys->GetValue(j));
						Angles *= (1.0f/360.0f);
						Gib.m_Angles[0] = RoundToInt(Angles[0] * 256.0f);
						Gib.m_Angles[1] = RoundToInt(Angles[1] * 256.0f);
						Gib.m_Angles[2] = RoundToInt(Angles[2] * 256.0f);
					}
					else if(pGibKeys->GetName(j) == "NODE")
					{
						Gib.m_iNode = pGibKeys->GetValuei(j);
					}
				}
		//		Angles.CreateMatrixFromAngles(0, Gib.m_Pos);
		//		CVec3Dfp32::GetMatrixRow(Gib.m_Pos, 3) = Origo;

			}
			break;
		}
	case MHASH3('GIBE','XPLO','SION'): // "GIBEXPLOSION"
		{
			for(int j = 0; j < _pKey->GetNumChildren(); ++j)
			{
				if(_pKey->GetName(j) == "ORIGIN")
				{
					_pKey->GetValueaf(j, 3, m_GibExplosionOrigin.k);
	//				m_GibExplosionOrigin.ParseString(_pKey->GetValue(j));
				}
				else if(_pKey->GetName(j) == "FORCE")
				{
					m_GibExplosionParams[0] = (fp32)_pKey->GetValuef(j);
				}
				else if(_pKey->GetName(j) == "RADIUS")
				{
					m_GibExplosionParams[1] = (fp32)_pKey->GetValuef(j);
				}
				else if(_pKey->GetName(j) == "NUMPARTS")
				{
					m_GibExplosionParams[2] = (fp32)_pKey->GetValuef(j);
				}
			}
			break;
		}

	case MHASH3('AUTO','DEST','ROY'): // "AUTODESTROY"
		{
			m_Flags |= PLAYER_FLAGS_AUTODESTROY;
			break;
		}

	case MHASH6('CAME','RA_S','TAND','_HEA','D_OF','FSET'): // "CAMERA_STAND_HEAD_OFFSET"
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if(pCD)
			{

				CVec3Dfp32 offset;
				CStr Value = KeyValue;
				offset.k[0] = Value.Getfp64Sep(",");
				offset.k[1] = Value.Getfp64Sep(",");
	//			offset.k[2] = Min((fp32)_Value.Getfp64Sep(","), (fp32)PLAYER_MAX_STAND_HEAD_HEIGHT);
				offset.k[2] = Value.Getfp64Sep(",");
				pCD->m_Camera_StandHeadOffset = offset;
			}
			break;
		}
	case MHASH7('CAME','RA_C','ROUC','H_HE','AD_O','FFSE','T'): // "CAMERA_CROUCH_HEAD_OFFSET"
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if(pCD)
			{
				CVec3Dfp32 offset;
				CStr Value = KeyValue;
				offset.k[0] = Value.Getfp64Sep(",");
				offset.k[1] = Value.Getfp64Sep(",");
	//			offset.k[2] = Min((fp32)_Value.Getfp64Sep(","), (fp32)PLAYER_MAX_CROUCH_HEAD_HEIGHT);
				offset.k[2] = Value.Getfp64Sep(",");
				pCD->m_Camera_CrouchHeadOffset = offset;
			}
			break;
		}
	case MHASH7('CAME','RA_B','EHIN','D_HE','AD_O','FFSE','T'): // "CAMERA_BEHIND_HEAD_OFFSET"
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if(pCD)
			{
				CStr Value = KeyValue;
				CVec3Dfp32 offset;
				offset.k[0] = Value.Getfp64Sep(",");
				offset.k[1] = Value.Getfp64Sep(",");
				offset.k[2] = Value.Getfp64Sep(",");
				pCD->m_Camera_BehindOffset = offset;
			}
			break;
		}
	case MHASH6('CAME','RA_S','HOUL','DER_','OFFS','ET'): // "CAMERA_SHOULDER_OFFSET"
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if(pCD)
			{
				CStr Value = KeyValue;
				CVec3Dfp32 offset;
				offset.k[0] = Value.Getfp64Sep(",");
				offset.k[1] = Value.Getfp64Sep(",");
				offset.k[2] = Value.Getfp64Sep(",");
				pCD->m_Camera_ShoulderOffset = offset;
			}
			break;
		}// Added by Mondelore {END}.
	case MHASH4('AUTO','AIMH','EIGH','T'): // "AUTOAIMHEIGHT"
		{
			// Where on _this_ character should other characters try to auto-aim
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if(pCD)
			{
				fp32 WantedHeight = KeyValuef;
				fp32 Interp = WantedHeight / (GetAbsBoundBox()->m_Max.k[2] - GetAbsBoundBox()->m_Min.k[2]);
				pCD->m_ThisAimHeight = Interp;
			}
			break;
		}
	case MHASH3('AI_A','IMOF','FSET'): // "AI_AIMOFFSET"
		{
			m_AIAimOffset = KeyValuei;
			break;
		}
	case MHASH5('AI_A','IMOF','FSET','_CRO','UCH'): // "AI_AIMOFFSET_CROUCH"
		{
			m_AICrouchAimOffset = KeyValuei;
			break;
		}
/*	case MHASH2('RAGD','OLL'): // "RAGDOLL"
		{
			if (KeyValuei)
			{
				m_Flags |= PLAYER_FLAGS_RAGDOLL;
				if ((KeyValuei != 0) && (KeyValuei < 256))
				{
					m_RagdollSettings.m_SkeletonType = KeyValuei;
				}
				int N = _pKey->GetNumChildren();
				for (int i = 0; i < N; i++)
				{
					const CRegistry* pChild = _pKey->GetChild(i);
					if (pChild)
					{
						const CStr ChildName = _pKey->GetThisName();
						uint32 ChildHash = StringToHash(ChildName);
						CStr ChildValue = _pKey->GetThisValue();
						int ChildValuei = ChildValue.Val_int();
						fp32 ChildValuef = (fp32)ChildValue.Val_fp64();

						if (ChildHash == MHASH3('ITER','ATIO','NS'))
						{
							if ((ChildValuei > 0) && (ChildValuei < 256))
							{
								m_RagdollSettings.m_ConstraintIterations = ChildValuei;
							}
						}
						else if (ChildHash == MHASH3('BLEN','DFRA','MES'))
						{
							if ((ChildValuei > 0) && (ChildValuei < 256))
							{
								m_RagdollSettings.m_BlendFrames = ChildValuei;
							}
						}
						else if (ChildHash == MHASH2('REBO','UND'))
						{
							if ((ChildValuef >= 0.0f) && (ChildValuef <= 1.0f))
							{
								m_RagdollSettings.m_Rebound = ChildValuef;
							}
						}
						else if (ChildHash == MHASH4('STOP','THRE','SHOL','D'))
						{
							if ((ChildValuef >= 0.0f) && (ChildValuef <= 10.0f))
							{
								m_RagdollSettings.m_StopThreshold = ChildValuef;
							}
						}
					}
				}
			}
			else
			{	// *Ragdoll 0
				m_Flags &= ~PLAYER_FLAGS_RAGDOLL;
				m_spRagdoll = NULL;
			}
			break;
		}*/
	case MHASH2('ROLL','BONE'): // "ROLLBONE"
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if(pCD)
				pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_ROLLBONE;
			break;
		}
	case MHASH5('NOCO','LLIS','IONS','UPPO','RTED'): // "NOCOLLISIONSUPPORTED"
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if(pCD)
				pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_NOCOLLISIONSUPPORTED;
			break;
		}

	case MHASH3('NODE','ATHS','OUND'): // "NODEATHSOUND"
		{
			m_Flags |= PLAYER_FLAGS_NODEATHSOUND;
			break;
		}
/*
	case MHASH5('CAME','RA_F','ACE_','OFFS','ET'): // "CAMERA_FACE_OFFSET"
		{ 
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if(pCD)
			{
				CVec3Dfp32 offset;
				offset.k[0] = _Value.Getfp64Sep(",");
				offset.k[1] = _Value.Getfp64Sep(",");
				offset.k[2] = _Value.Getfp64Sep(",");
				pCD->m_Camera_FaceOffset = offset;
			}
			break;
		}
*/

	case MHASH5('CAME','RA_F','ADE_','OFFS','ET'): // "CAMERA_FADE_OFFSET"
	 	{ 
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
			if(pCD)
				pCD->m_Camera_FadeOffset = KeyValuef;
		
			break;
		}
	case MHASH5('SPEE','D_WA','LKMO','DIFI','ER'): // "SPEED_WALKMODIFIER"
		{
			CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
			if(pCD)
			{
				pCD->m_Speed_Forward = PHYSSTATE_CONVERTFROM20HZ(KeyValuef);
			}
			break;
		}
	case MHASH4('SPEE','D_FO','RWAR','D'): // "SPEED_FORWARD"
		{
			CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
			if(pCD)
			{
				pCD->m_Speed_Forward = PHYSSTATE_CONVERTFROM20HZ(KeyValuef);
			}
			break;
		}
	case MHASH4('SPEE','D_SI','DEST','EP'): // "SPEED_SIDESTEP"
		{
			CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
			if(pCD)
			{
				pCD->m_Speed_SideStep = PHYSSTATE_CONVERTFROM20HZ(KeyValuef);
			}
			break;
		}
	case MHASH2('SPEE','D_UP'): // "SPEED_UP"
		{
			CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
			if(pCD)
				pCD->m_Speed_Up = PHYSSTATE_CONVERTFROM20HZ(KeyValuef);
			break;
		}
	
	case MHASH3('SPEE','D_JU','MP'): // "SPEED_JUMP"
		{
			CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
			if(pCD)
				pCD->m_Speed_Jump = PHYSSTATE_CONVERTFROM20HZ(KeyValuef);
			break;
		}

	case MHASH2('FLAG','S'): // "FLAGS"
		{
			m_Flags |= ResolvePlayerFlags(KeyValue);
			break;
		}

	case MHASH3('DEFA','ULTF','LAGS'): // "DEFAULTFLAGS"
		{
			m_Flags |= ResolvePlayerFlags(KeyValue);
			break;
		}

	case MHASH2('DROP','ITEM'): // "DROPITEM"
		{
			bool OldPrecache = CRPG_Object::m_bPrecacheForPlayerUse;
			CRPG_Object::m_bPrecacheForPlayerUse = true;
			m_spDropItem = CRPG_Object::CreateObject(KeyValue, m_pWServer);
			CRPG_Object::m_bPrecacheForPlayerUse = OldPrecache;
			break;
		}

	case MHASH1('MAP'): // "MAP"
		{
			CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
			if(!pCD)
				return;

			CStr St = KeyValue;
			pCD->m_Map_iSurface = m_pWServer->GetMapData()->GetResourceIndex_Surface(St.GetStrSep(","));
			m_Player.m_Map_ItemType = St.GetStrSep(",").Val_int();
			pCD->m_Map_PosLeft = St.GetStrSep(",").Val_int();
			pCD->m_Map_PosTop = St.GetStrSep(",").Val_int();
			pCD->m_Map_PosRight = St.GetStrSep(",").Val_int();
			pCD->m_Map_PosBottom = St.GetStrSep(",").Val_int();
			break;
		}

	case MHASH3('OVER','VIEW','MAP'): // "OVERVIEWMAP"
		{
			CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
			if(!pCD)
				return;

			CFStr St = KeyValue;
			pCD->m_Overviewmap.m_Value = St.GetStrSep(",");
			pCD->m_Overviewmap.MakeDirty();
			int iZone = St.GetStrSep(",").Val_int();
			pCD->m_Overviewmap_Current = (int8)iZone;
			pCD->m_Overviewmap_Visited = pCD->m_Overviewmap_Visited | (1 << iZone);
			break;
		}

/*	
	case MHASH4('DUMM','YSEQ','UENC','E'): // "DUMMYSEQUENCE"
		{
			m_iForceAnim = KeyValuei;
			break;
		}

	case MHASH3('FORC','EANI','M'): // "FORCEANIM"
		{
			CWObject_Message Msg(OBJMSG_GAME_RESOLVEANIMHANDLE);
			Msg.m_pData = (char *)KeyValue;

			m_iForceAnim = m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());
			break;
		}
*/

/*	
	case MHASH3('DEAT','HANI','MS'): // "DEATHANIMS"
		{
			CFStr st = KeyValue;
			int Index = 0;
			while(st != "")
			{
				CWObject_Message Msg(OBJMSG_GAME_RESOLVEANIMHANDLE);
				CStr Anim = st.GetStrSep(",");
				Msg.m_pData = (char *)Anim;

				m_iAnimDeath[Index++] = m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());
			}
			break;
		}
*/

	case MHASH3('ANIM','GRAP','H2'): // "ANIMGRAPH2"
		{
			int32 NumChildren = _pKey->GetNumChildren();
			for (int32 i = 0; i < NumChildren; i++)
			{
				CWAG2I_Context Context(this, m_pWServer, CMTime());
				CStr AGName = _pKey->GetName(i);
				int32 iAGSlot = -1;
				if (AGName.CompareSubStr("GRAPH") == 0)
				{
					CStr Name = AGName;
					AGName = _pKey->GetValue(i);
				
					if (NumChildren > 1)
					{
						CStr Right = Name.RightFrom(5);
						iAGSlot = Right.Val_int() - 1;
					}
				}

				int32 iAG2 = m_pWServer->GetMapData()->GetResourceIndex_AnimGraph2(AGName);
				if (iAGSlot == -1)
					pCD->m_AnimGraph2.GetAG2I()->AddResourceIndex_AnimGraph2(iAG2,AGName);
				else
					pCD->m_AnimGraph2.GetAG2I()->SetResourceIndex_AnimGraph2(iAG2,AGName,iAGSlot);
			}
			break;
		}
	case MHASH3('ANIM','LIPS','YNC'): // "ANIMLIPSYNC"
		{
			int iAnim = m_pWServer->GetMapData()->GetResourceIndex_Anim(KeyValue);
			CWResource* pWRes = m_pWServer->GetMapData()->GetResource(iAnim);
			if (pWRes)
				pCD->m_AnimLipSync = pWRes->m_iRc;//m_pWServer->GetMapData()->GetResourceIndex_Anim(KeyValue);
			break;
		}


/*
	case MHASH2('ANIM','LIST'): // "ANIMLIST"
		{
			CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
			if(pCD && KeyValue != "-")
				pCD->m_iAnimList = m_pWServer->GetMapData()->GetResourceIndex_AnimList(KeyValue);
			break;
		}
*/	
/*	
	case MHASH5('FORC','EANI','MSTA','RTTI','ME'): // "FORCEANIMSTARTTIME"
		{
			m_ForceAnimStartTime = KeyValuef * SERVER_TICKSPERSECOND;
			break;
		}
*/

/*	
	case MHASH3('SOUN','D_JU','MP'): // "SOUND_JUMP"
		{
			CWO_Character_ClientData *pCD = GetClientData(this);
			if(pCD)
				pCD->m_iSoundJump = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH4('SOUN','D_JU','MPVO','C'): // "SOUND_JUMPVOC"
		{
			CWO_Character_ClientData *pCD = GetClientData(this);
			if(pCD)
				pCD->m_iSoundJumpVocal = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH3('SOUN','D_WA','LK'): // "SOUND_WALK"
		{
			CWO_Character_ClientData *pCD = GetClientData(this);
			if(pCD)
				pCD->m_iSoundWalk = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH3('SOUN','D_LA','ND0'): // "SOUND_LAND0"
		{
			m_iSoundLand[0] = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH3('SOUN','D_LA','ND1'): // "SOUND_LAND1"
		{
			m_iSoundLand[1] = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH3('SOUN','D_LA','ND2'): // "SOUND_LAND2"
		{
			m_iSoundLand[2] = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH3('SOUN','D_PA','IN0'): // "SOUND_PAIN0"
		{
			m_iSoundPain[0] = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH3('SOUN','D_PA','IN1'): // "SOUND_PAIN1"
		{
			m_iSoundPain[1] = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH4('SOUN','D_DE','ATH_','FALL'): // "SOUND_DEATH_FALL"
		{
			m_iSoundDeath[0] = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH4('SOUN','D_DE','ATH_','GIB'): // "SOUND_DEATH_GIB"
		{
			m_iSoundDeath[1] = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH4('SOUN','D_DE','ATH_','FIRE'): // "SOUND_DEATH_FIRE"
		{
			m_iSoundDeath[2] = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	case MHASH5('SOUN','D_DE','ATH_','MAGI','C'): // "SOUND_DEATH_MAGIC"
		{
			m_iSoundDeath[3] = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
			break;
		}
	else if(KeyName.CompareSubStr("SOUND_ACTIVATE_ITEMGROUP") == 0)
	{
		int iVal = KeyName.Copy(24, 1024).Val_int();
		if(iVal > 0 && iVal < PLAYER_MAXACTIVATEITEMGROUPSOUNDS)
			m_iSoundActivateItem[iVal] = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
	}
*/
	
	case MHASH2('AI_T','YPE'): // "AI_TYPE"
		{
			// *** Workaround constructions like this works again ***
			// *key KeyValue
			// {
			//		*Subkey SubkeyValue
			// }
			// ***
			if (KeyValue == "")
			{
				int N = _pKey->GetNumChildren();
				for (int i = 0; i < N; i++)
				{
					const CRegistry* pChildKey = _pKey->GetChild(i);
					if ((pChildKey)&&(pChildKey->GetThisValue().CompareNoCase("CLASS_DARKLING") == 0))
					{
						// ***  REMOVE BEFORE CHECKING IN ***
						// _pKey->GetParent()->XRG_Write("What_the_fuck.xrg");
						// ***
						KeyValue = "DARKLING";
						break;
					}
				}
			}
			// ***
			spCAI_Core spAI = CAI_Core::GetAIFromString(KeyValue, m_spAI, m_pWServer, this);
			if (spAI)
			{
				m_spAI = spAI;
			}
			int N = _pKey->GetNumChildren();
			for (int i = 0; i < N; i++)
			{
				const CRegistry* pChildKey = _pKey->GetChild(i);
				if (pChildKey)
				{
					m_spAI->OnEvalKey(pChildKey->GetThisNameHash(), pChildKey);
				}
			}
			break;
		}

	case MHASH3('PHYS','_GHO','ST'): // "PHYS_GHOST"
		{
			if (KeyValuei == 1)
			{
				m_ClientFlags |= PLAYER_CLIENTFLAGS_GHOST;
			};
			break;
		}


	case MHASH3('PHYS','_FLY','ER'): // "PHYS_FLYER"
		{
			if (KeyValuei == 1)
			{
				CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
				if(pCD)
					pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_AIRCRAFT;
				m_ClientFlags |= PLAYER_CLIENTFLAGS_NOGRAVITY | PLAYER_CLIENTFLAGS_WEIGHTLESS;
			};
			break;
		}

	case MHASH3('PHYS','_IMM','UNE'): // "PHYS_IMMUNE"
		{
			if (KeyValuei == 1)
			{
				//Shouldn't be used really, immunity should propably be declared with other phys flags
				CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
				if(pCD)
					pCD->m_Phys_Flags = pCD->m_Phys_Flags | PLAYER_PHYSFLAGS_IMMUNE;
			};
			break;
		}

	case MHASH3('PHYS','_FLA','GS'): // "PHYS_FLAGS"
		{
			CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
			if(pCD)
				pCD->m_Phys_Flags = ResolvePhysFlags(KeyValue);
			break;
		}

	case MHASH4('PHYS','_NOG','RAVI','TY'): // "PHYS_NOGRAVITY"
		{
			if (KeyValuei == 1)
			{
				//Disable gravity, and set weightless flag
			
				m_ClientFlags |= PLAYER_CLIENTFLAGS_NOGRAVITY | PLAYER_CLIENTFLAGS_WEIGHTLESS;
			};
			break;
		}
	
	case MHASH3('NO_S','HADO','W'): // "NO_SHADOW"
		{
			CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
			if (pCD)
			{
				if (KeyValuei == 1)
					pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_NOSHADOW;
				else
					pCD->m_ExtraFlags = pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_NOSHADOW;
			}
			break;
		}

	case MHASH6('MOUN','TEDC','AM_N','OLOO','KUPD','ATE'): // "MOUNTEDCAM_NOLOOKUPDATE"
		{
			CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
			if (pCD)
			{
				if (KeyValuei == 1)
					pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_MOUNTEDCAM_NOLOOKUPDATE;
				else
					pCD->m_ExtraFlags = pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_MOUNTEDCAM_NOLOOKUPDATE;
			}
			break;
		}

	case MHASH4('ABIL','ITY_','STEA','LTH'): // "ABILITY_STEALTH"
		{
			m_iStealthActivateTime = (int)(KeyValuef * m_pWServer->GetGameTicksPerSecond());
			m_iStealthTimer = m_iStealthActivateTime;
			break;
		}
	case MHASH4('DAMA','GE_F','ACTO','R0'): // "DAMAGE_FACTOR0"
		{
			EvalDamageFactor(_pKey,0);
			break;
		}
	case MHASH4('DAMA','GE_F','ACTO','R1'): // "DAMAGE_FACTOR1"
		{
			EvalDamageFactor(_pKey,1);
			break;
		}
	case MHASH4('DAMA','GE_F','ACTO','R2'): // "DAMAGE_FACTOR2"
		{
			EvalDamageFactor(_pKey,2);
			break;
		}
	case MHASH4('DAMA','GE_F','ACTO','R3'): // "DAMAGE_FACTOR3"
		{
			EvalDamageFactor(_pKey,3);
			break;
		}
	case MHASH4('DAMA','GE_F','ACTO','R4'): // "DAMAGE_FACTOR4"
		{
			EvalDamageFactor(_pKey,4);
			break;
		}
	case MHASH4('DAMA','GE_F','ACTO','R5'): // "DAMAGE_FACTOR5"
		{
			EvalDamageFactor(_pKey,5);
			break;
		}
	case MHASH4('DAMA','GE_F','ACTO','R6'): // "DAMAGE_FACTOR6"
		{
			EvalDamageFactor(_pKey,6);
			break;
		}
	case MHASH4('DAMA','GE_F','ACTO','R7'): // "DAMAGE_FACTOR7"
		{
			EvalDamageFactor(_pKey,7);
			break;
		}
	case MHASH4('DAMA','GE_F','ACTO','R8'): // "DAMAGE_FACTOR8"
		{
			EvalDamageFactor(_pKey,8);
			break;
		}
	case MHASH4('DAMA','GE_F','ACTO','R9'): // "DAMAGE_FACTOR9"
		{
			EvalDamageFactor(_pKey,9);
			break;
		}
	case MHASH7('CHAR','_TRA','NQUI','LLIZ','EROU','TTIM','E'): // "CHAR_TRANQUILLIZEROUTTIME"
		{
			m_TranquillizerOutTime = (int32)(KeyValuef * 20.0f);
			break;
		}

	case MHASH4('GRAB','DIFF','ICUL','TY'): // "GRABDIFFICULTY"
		{
			m_GrabDifficulty = KeyValuei;
			break;
		}

	case MHASH3('FIGH','TBBS','IZE'): // "FIGHTBBSIZE"
		{
			m_FightBBSize = (uint8)KeyValuei;
			break;
		}

	case MHASH4('FOCU','SFRA','MEOF','FSET'): // "FOCUSFRAMEOFFSET"
		{
			pCD->m_FocusFrameOffset = (int8)KeyValue.Val_int();
			break;
		}

	case MHASH5('SECU','RITY','CLEA','RANC','E'): // "SECURITYCLEARANCE"
		{
			m_MinSecurityClearance = Min(127, Max(0, KeyValue.Val_int()));
			break;
		}

	case MHASH3('DEBU','GCHA','R'): // "DEBUGCHAR"
		{
			// Just remove the log-error.
			break;
		}

	case MHASH3('MAXD','ARKN','ESS'): // "MAXDARKNESS"
		{
			// Clamp to 100 units
			pCD->m_MaxDarkness = Min(KeyValuei,0x64);
			break;
		}

	case MHASH4('DARK','NESS','POWE','RS'): // "DARKNESSPOWERS"
		{
			pCD->m_DarknessPowersAvailable = ResolveDarknessFlags(KeyValue);
			break;
		}

	case MHASH4('DARK','NESS','LEVE','L'):	// "DARKNESSLEVEL"
		{
			pCD->m_DarknessLevel.SetLevel(Clamp(KeyValuei, 0, 4));
			pCD->m_MaxDarkness = pCD->m_DarknessLevel.GetMaxDarkness();
			break;
		}

	case MHASH6('DARK','NESS','LEVE','L','_','ARM'):	// "DARKNESSLEVEL_ARM"
		{
			pCD->m_DarknessLevel.SetPowerLevel(PLAYER_DARKNESSMODE_POWER_DEMONARM, Clamp(KeyValuei, 0, 1));
			break;
		}

	case MHASH6('NONP','LAYE','RDAR','KNES','SUSE','R'): // "NONPLAYERDARKNESSUSER"
		{
			m_bNonPlayerDarknessUser = (KeyValuei != 0);
			break;
		}

#ifndef M_DEMO
	case MHASH3('CHEA','TITE','MS'): // "CHEATITEMS"
		{
			m_Player.m_CheatItems = KeyValue;
			break;
		}
#endif

	case MHASH3('RPGO','BJEC','T'): // "RPGOBJECT"
		{
			m_spRPGObjectReg = _pKey->Duplicate();
			break;
		}

	case MHASH5('DARK','NESS','_TEN','TACL','ES'): // "DARKNESS_TENTACLES"
		{
			pCD->m_iDarkness_Tentacles = m_pWServer->Object_Create(KeyValue, GetPosition(), m_iObject);
			break;
		}

	case MHASH5('DARK','NESS','_BLA','CKHO','LE'): // "DARKNESS_BLACKHOLE"
		{
			int iNewObj = m_pWServer->Object_Create(KeyValue, GetPosition(), m_iObject);
			m_iDarkness_BlackHole = Max(0, iNewObj);
			break;
		}

	case MHASH6('DARK','NESS','_DAR','KLIN','GSPA','WN'): // "DARKNESS_DARKLINGSPAWN"
		{
			m_Darkness_DarklingSpawn = KeyValue;
			break;
		}

/*	case MHASH6('DARK','NESS','_VIS','IONP','ARTI','CLES'): // "DARKNESS_VISIONPARTICLES"
		{
			pCD->m_iDarkness_VisionParticles = m_pWServer->Object_Create(KeyValue, GetPosition(), m_iObject);
			break;
		}*/

	case MHASH4('DARK','NESS','_DRA','IN_1'): // "DARKNESS_DRAIN_1"
		{
			pCD->m_iDarkness_Drain_1 = m_pWServer->GetMapData()->GetResourceIndex_Model(KeyValue);
			break;
		}

	case MHASH4('DARK','NESS','_DRA','IN_2'): // "DARKNESS_DRAIN_2"
		{
			m_iDarkness_Drain_2 = Max(0, m_pWServer->Object_Create(KeyValue, GetPosition(), m_iObject));
			break;
		}

	case MHASH6('DARK','NESS','_CRE','EPIN','GDAR','K'): // "DARKNESS_CREEPINGDARK"
		{
			m_iDarkness_CreepingDark = Max(0, m_pWServer->Object_Create(KeyValue, GetPosition(), m_iObject));
			break;
		}

	case MHASH3('CHAI','NEFF','ECT'): // "CHAINEFFECT"
		{
			m_iChainObj = m_pWServer->Object_Create(KeyValue, GetPosition(), m_iObject);
			break;
		}

	// Darkness powerups like (darklings, max darkness and Jackies core powers)
	case MHASH4('DARK','NESS','POWE','RUP'): // "DARKNESSPOWERUP"
		{
			int32 Len = _pKey->GetNumChildren();
			for(int32 i = 0; i < Len; i++)
			{
				const CRegistry* pChild = _pKey->GetChild(i);
				if(pChild && pChild->GetThisName() != "")
				{
					SDarknessPowerup DarknessPowerup;
					CStr ChildName = pChild->GetThisName();
					if(ChildName == "DARKLING")
						DarknessPowerup.m_Type = PLAYER_DARKNESSPOWERUP_DARKLING;
					else if(ChildName == "MAXDARKNESS")
						DarknessPowerup.m_Type = PLAYER_DARKNESSPOWERUP_MAXDARKNESS;
					else if(ChildName == "COREPOWER")
						DarknessPowerup.m_Type = PLAYER_DARKNESSPOWERUP_COREPOWER;
					else
						DarknessPowerup.m_Type = 0;

					DarknessPowerup.m_Data = pChild->GetThisValue();
					m_lDarknessPowerups.Add(DarknessPowerup);
				}
			}
			break;
		}
	case MHASH5('AVAI','LABL','EDAR','KLIN','GS'): // "AVAILABLEDARKLINGS"
		{
			// "cheating" I guess way to add darklings
			int32 Len = _pKey->GetNumChildren();
			for (int32 i = 0; i < Len; i++)
			{
				const CRegistry* pChild = _pKey->GetChild(i);
				if (pChild && pChild->GetThisName().Len() > 0)
					Char_AddDarkling(pChild->GetThisName());
			}
			break;
		}
	case MHASH2('GODM','ODE'): // "GODMODE"
		{
			if (Char_CheatsEnabled())
				pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_GODMODE;
			break;
		}
	case MHASH3('OTHE','RWOR','LD'): // "OTHERWORLD"
		{
			if (KeyValuei)
				pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_OTHERWORLD;
			break;
		}

	case MHASH5('FORC','EATT','ACHM','ATRI','X'): // "FORCEATTACHMATRIX"
		{
			pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_FORCEATTACHMATRIX;
			break;
		}
	case MHASH3('FACI','ALSE','TUP'): // "FACIALSETUP"
		{
			pCD->m_iFacialSetup = m_pWServer->GetMapData()->GetResourceIndex_FacialSetup(KeyValue);
			break;
		}
	case MHASH3('ANIM','_VOC','AP'): // "ANIM_VOCAP"
		{
			CStr St = KeyValue;
			while(St != "")
			{
				CStr Anim = St.GetStrSep(",");
				if(Anim != "")
				{
					int iAnim = m_pWServer->GetMapData()->GetResourceIndex_Anim(Anim);
					if(iAnim > 0)
					{
						pCD->m_lAnim_VoCap.Add(iAnim);

						// mark container as fully used.
						//   (TEMP! what we should do is to only load the sequences that are actually used)
						CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(m_pWServer);
						if (pServerMod)
							pServerMod->AddAnimContainerEntry(Anim, 0, iAnim);
					}
				}

			}
			pCD->m_lAnim_VoCap.MakeDirty();
			break;
		}
	case MHASH4('GUNP','LAYS','AFED','ELAY'): // "GUNPLAYSAFEDELAY"
		{
			m_GunplaySafeDelay = (int32)M_Ceil(m_pWServer->GetGameTicksPerSecond() * KeyValuef);
			break;
		}
	case MHASH4('GUNP','LAYS','PREA','D'): // "GUNPLAYSPREAD"
		{
			m_GunplaySpread = (int32)M_Ceil(m_pWServer->GetGameTicksPerSecond() * KeyValuef);
			break;
		}
	case MHASH3('GUNP','LAYT','YPES'): // "GUNPLAYTYPES"
		{
			while (KeyValue.Len())
			{
				CAG2ImpulseValue Val = KeyValue.GetStrSep(",").Val_int();
				if (Val > 0)
					m_lGunPlayTypes.Add(Val);
			}
			break;
		}
	case MHASH6('GUNP','LAYR','EACT','IVAT','EDEL','AY'): // "GUNPLAYREACTIVATEDELAY"
		{
			m_GunplayReactivateDelay = (int32)M_Ceil(m_pWServer->GetGameTicksPerSecond() * KeyValuef);
			break;
		}
	case MHASH3('GUNK','ATAM','OVES'): // "GUNKATAMOVES"
		{
			while (KeyValue.Len())
			{
				CStr StrVal = KeyValue.GetStrSep(",");
				if (StrVal.Len())
				{
					GunKataMove Move;
					Move.m_Impulse = StrVal.GetStrSep(":").Val_int();
					Move.m_TriggerType = StrVal.GetStrSep(":").Val_int();
					Move.m_Direction = StrVal.GetStrSep(":").Val_int();
					Move.m_AnimType = StrVal.GetStrSep(":").Val_int();
					// Might not be set (defaults to 0)
					Move.m_ItemType = StrVal.Val_int();
					m_lGunKataMoves.Add(Move);
				}
			}
			break;
		}
	case MHASH3('SHIE','LD_S','TART'): // "SHIELD_START"
		{
			pCD->m_ShieldStart = KeyValuei;
			break;
		}

	case MHASH2('POST','LOAD'): // "POSTLOAD" - the keys parsed here are supposed to be applied at the end of OnDeltaLoad()
		{
			uint nChild = _pKey->GetNumChildren();
			for (uint i = 0; i < nChild; i++)
			{
				int j = _pKey->FindIndex(CFStrF("%d", i));	// this is a hack to force the order of children (keys are named "0", "1", "2" etc)
				if (j < 0) continue;
				const CRegistry* pGroup = _pKey->GetChild(j);
				CStr Flags = pGroup->GetThisValue();
				if (m_pWServer->World_TestSpawnFlags(Flags))
				{
					// loop over keys to override
					uint nChild2 = pGroup->GetNumChildren();
					for (uint k = 0; k < nChild2; k++)
					{
						uint32 KeyHash = pGroup->GetChild(k)->GetThisNameHash();
						m_PostLoadDialogueItems.Parse(KeyHash, pGroup->GetValue(k));
					}
				}
			}
			break;
		}

	default:
		{
			if (m_DialogueItems.Parse(_KeyHash, KeyValue))
				return;

			{
				CWO_Burnable& Burnable = pCD->m_Burnable.m_Value;
				if (Burnable.OnEvalKey(m_pWServer, _KeyHash, _pKey))
					return;
			}

			if (KeyName.CompareSubStr("TEAM") == 0)
			{
				int iSlot = atoi(KeyName.Str()+4);
				m_lTeamFromKeys.SetMinLen(iSlot + 1);
				m_lTeamFromKeys[iSlot] = m_pWServer->World_MangleTargetName(KeyValue);
			}

			else if(KeyName.CompareSubStr("DIALOGUEIMPULSE") == 0)
			{
				int iIndex = atoi(KeyName.Str()+15);
				m_lDialogueImpulses.SetMinLen(iIndex + 1);
				m_lDialogueImpulses[iIndex].Parse(KeyValue, m_pWServer);
			}

			else if (KeyName.CompareSubStr("SKELETONSCALE") == 0)
				// Skeletonscale is "difference", so take 1 - given scale
				pCD->m_CharSkeletonScale = 1.0f - KeyValuef;

			else if (KeyName.CompareSubStr("CHARGLOBALSCALE") == 0)
				// Global scale
				pCD->m_CharGlobalScale = KeyValuef;

			else if(m_MsgContainer.OnEvalKey(_KeyHash, _pKey, m_pWServer))
				return;

			else if(m_ImpulseContainer.OnEvalKey(_KeyHash, _pKey, m_pWServer))
				return;

			//Let AI or grandparent object evaluate this key
			else if (!m_spAI->OnEvalKey(_KeyHash, _pKey))
				CWObject_RPG::OnEvalKey(_KeyHash, _pKey);

			break;
		}
	}
}
//}}}
void CWObject_Character::EvalDamageFactor(const CRegistry* _pKey,int _iSlot)
{
	if ((!_pKey)||(_iSlot < 0)||(_iSlot > 9))
	{
		return;
	}

	fp32 Factor = 1.0f;
	uint32 DamageTypes = DAMAGETYPE_UNDEFINED;
	uint16 iHitloc =  SHitloc::HITLOC_ALL;
	int16 iSurface = -1;
	int iArmour = 0;
	int N = _pKey->GetNumChildren();
	for(int i = 0; i < N; i++)
	{
		CStr KeyName = _pKey->GetName(i).UpperCase();
		CStr KeyValue = _pKey->GetValue(i);
		if (KeyName == "FACTOR")
		{
			Factor = _pKey->GetValuef(i);
		}
		else if (KeyName == "DAMAGETYPES")
		{
			DamageTypes = (uint32)_pKey->GetValue(i).TranslateFlags(CRPG_Object::ms_DamageTypeStr);
		}
		else if (KeyName == "HITLOCATION")
		{
			iHitloc = 0;
			CStr HitLoc = _pKey->GetValue(i).UpperCase();
			if (HitLoc.Find("ALL") >= 0)
			{
				iHitloc |= SHitloc::HITLOC_ALL;
			}
			else if ((HitLoc.Find("HIGH") >= 0)||(HitLoc.Find("HEAD") >= 0))
			{
				iHitloc |= SHitloc::HITLOC_HIGH;
			}
			else if ((HitLoc.Find("MID") >= 0)||(HitLoc.Find("BODY") >= 0)||(HitLoc.Find("ARM") >= 0))
			{
				iHitloc |= SHitloc::HITLOC_MID;
			}
			else if ((HitLoc.Find("LOW") >= 0)||(HitLoc.Find("LEG") >= 0))
			{
				iHitloc |= SHitloc::HITLOC_LOW;
			}
		}
		else if (KeyName == "SURFACE")
		{
			iSurface =  _pKey->GetValuei(i);
		}
		else if (KeyName == "ARMOUR")
		{
			iArmour =  _pKey->GetValuei(i);
		}
	}
	if (N > 0)
	{
		SetHitlocDamageFactor(_iSlot,Factor,DamageTypes,iHitloc,iSurface,iArmour);
	}
}

//Part of Sammes message testing system
//extern TArray<SMessageCounter> g_slMessages;

// Unspawn
void CWObject_Character::UnspawnCharacter()
{
	MAUTOSTRIP(CWObject_Character_UnspawnCharacter, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_Character::UnspawnCharacter);

	m_Flags |= PLAYER_FLAGS_WAITSPAWN;
	ClientFlags() |= CWO_CLIENTFLAGS_INVISIBLE;
	UpdateVisibilityFlag();
	CWO_PhysicsState Phys;
	Phys.Clear();
	m_pWServer->Object_SetPhysics(m_iObject, Phys);
}


bool CWObject_Character::Char_TrySetPosition(int _PhysMode)
{
	CWO_Character_ClientData* pCD = GetClientData(this);

	CVec3Dfp32 DesiredPos = GetPosition();
	DesiredPos.k[2] += 0.001f;

	// First try to spawn at desired position
	SetPosition(DesiredPos);
	if (Char_SetPhysics(this, m_pWServer, m_pWServer, _PhysMode, false, true))
		return true;

	// Then start adding an offset of increasing size
	const static fp32 lRadius[] = { 4.0f, 16.0f, 64.0f };
	for (uint iRadius = 0; iRadius < 3; iRadius++)
	{
		fp32 Radius = lRadius[iRadius];

		for (int x = -1; x <= 1; x+=2)
			for (int y = -1; y <= 1; y+=2)
				for (int z = -1; z <= 1; z+=2)
				{
					CVec3Dfp32 dPos(fp32(x), fp32(y), fp32(z) * 0.5f);
					dPos *= Radius;

					SetPosition(DesiredPos + dPos);
					if (Char_SetPhysics(this, m_pWServer, m_pWServer, _PhysMode, false, true))
					{
						ConOut(CStrF("Dislocated spawned character %s units (%s, %s, %s)", dPos.GetFilteredString().Str(), GetName(), GetTemplateName(), pCD->m_Character.m_Value.Str()));
						return true;
					}
				}

		if ((iRadius == 1) && (pCD->m_iPlayer != -1))
		{
			// after having failed with a radius of 16 units, let's try nuke all physics objects in our way
			SetPosition(DesiredPos);
			if (Char_SetPhysics(this, m_pWServer, m_pWServer, _PhysMode, true, true))
				return true;
		}
	}
	// we failed :'(
	return false;
}


void CWObject_Character::SpawnCharacter(int _PhysMode, int _SpawnBehavior, bool _bFindPos)
{
	MAUTOSTRIP(CWObject_Character_SpawnCharacter, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_Character::SpawnCharacter);
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
	if (!pCD) return;

//	M_TRACE("[Char %d, %s], spawned\n", m_iObject, GetName());

	if(m_Flags & PLAYER_FLAGS_SPAWNDEAD)
	{
		Char_Die(DAMAGETYPE_UNDEFINED,0);
		_PhysMode = PLAYER_PHYS_DEAD;
	}
	else if(m_Flags & PLAYER_FLAGS_SPAWNCROUCHED)
	{
		_PhysMode = PLAYER_PHYS_CROUCH;
	}

	if(m_Flags & PLAYER_FLAGS_DISABLEIK)
	{
		//pCD->m_PostAnimSystem.DisableFeetIK(true);
		pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_DISABLEHUMANIK;
	}

	if (!_bFindPos)
	{
		CMat4Dfp32 Mat = GetPositionMatrix();
		Mat.GetRow(3).k[2] += 0.001f;
		CWO_PhysicsState Phys = GetPhysState();
		Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(pCD->m_Phys_Width, pCD->m_Phys_Width, pCD->m_Phys_Height), CVec3Dfp32(0,0,pCD->m_Phys_Height));
		Phys.m_nPrim = 1;
		Phys.m_MediumFlags |= XW_MEDIUM_PLAYERSOLID | XW_MEDIUM_DYNAMICSSOLID;
		Phys.m_ObjectFlags |= OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_WORLDTELEPORT | OBJECT_FLAGS_PHYSOBJECT;
		if (pCD->m_iPlayer == -1)
			Phys.m_MediumFlags |= XW_MEDIUM_AISOLID;

		int Ret = m_pWServer->Object_SetPhysics(m_iObject, Phys, Mat);
		if (!Ret)
		{
			return;
		}
	}

	ClientFlags() &= ~CWO_CLIENTFLAGS_INVISIBLE;
	if(m_Flags & PLAYER_FLAGS_NOCROUCH)
	{
		pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_ALWAYS_NOCROUCH;
		ClientFlags() |= PLAYER_CLIENTFLAGS_NOCROUCH;
	}

	if(m_Flags & PLAYER_FLAGS_ALWAYSVISIBLE)
	{
//		ClientFlags() |= CWO_CLIENTFLAGS_LINKINFINITE;		// Svar: Nej   /mh
	}

/*Part of Sammes message testing system
	if(pCD->m_iPlayer != -1)
		g_slMessages.Clear();*/

	if(m_Flags & PLAYER_FLAGS_AIR)
		pCD->m_Phys_Flags = pCD->m_Phys_Flags | PLAYER_PHYSFLAGS_NOCHARACTERCOLL;
	
	if(m_Flags & PLAYER_FLAGS_NOBLINK)
		pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_NOBLINK;
	
	if(pCD->m_iPlayer == -1)
	{
		pCD->m_GameTick = m_pWServer->GetGameTick();
		pCD->m_GameTime = PHYSSTATE_TICKS_TO_TIME(pCD->m_GameTick, m_pWServer);
	}

	bool bOk = Char_TrySetPosition(_PhysMode);

	if (!bOk)
	{
		// We failed to place the character
		if(pCD->m_iPlayer != -1 || (_SpawnBehavior & PLAYER_SPAWNBEHAVIOR_FROMIO))
		{
			ConOutL("§cf80WARNING: Player could not be placed, using 'noclip'. (" + GetPosition().GetString() + ")");
			Char_SetPhysics(this, m_pWServer, m_pWServer, PLAYER_PHYS_NOCLIP, true);
		}
		else
		{
#ifndef PLATFORM_XBOX				
			//Create marker object for unspawned character
			CWObject * pObj = m_pWServer->Object_Get(m_pWServer->Object_Create("Model", GetPosition()));
			if (pObj)
				pObj->m_iModel[0] = m_iModel[0];
#endif
			ConOutL(CStrF("Computer controlled character could not be placed (%s, %s, %s)", GetName(), GetTemplateName(), pCD->m_Character.m_Value.Str()));
			Destroy();
			return;
		}
	}

//	m_SpawnPos = GetPosition();

	Char_SetAnimStance(this, 0);

	// Initialize body direction
	CVec3Dfp32 Look = GetLook(GetPositionMatrix());
	pCD->m_Control_Look[2] = Look[2];
	pCD->m_Anim_BodyAngleZ = Look[2];

	// If we have aquired a wait state during initalization we reset it.
	Char()->Wait() = 0;

	// No point in rotating char visbox.
	m_ClientFlags |= CWO_CLIENTFLAGS_AXISALIGNEDVISBOX;

	// FIXME: Should ALL characters have velocity replication? or just players?
	m_ClientFlags |= CWO_CLIENTFLAGS_MOVEVELOCITY;

	UpdateVisibilityFlag();

	if(m_Flags & PLAYER_FLAGS_WAITSPAWN && !(_SpawnBehavior & PLAYER_SPAWNBEHAVIOR_NOSPAWNMESSAGES))
	{
		m_Flags &= ~PLAYER_FLAGS_WAITSPAWN;
		m_MsgContainer.SendMessage(PLAYER_SIMPLEMESSAGE_ONSPAWN, m_iObject, m_iObject, m_pWServer);
	}

	m_LastCamera = GetPositionMatrix();


	// Don't send OnSpawn messages if player hasn't spawned yet...  (this is done in CWObject_Game::OnClientConnected)
	if (m_pWServer->Game_GetObject()->Player_GetNum())
		SendOnSpawnMessages();

	// Refresh dirty autovars
	m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
} 


void CWObject_Character::OnFinishEvalKeys()
{
	MAUTOSTRIP(CWObject_Character_OnFinishEvalKeys, MAUTOSTRIP_VOID);
	MSCOPE(CWObject_Character::OnFinishEvalKeys, CHARACTER);

	CWObject_Player::OnFinishEvalKeys();

	if (m_lDarknessPowerups.Len() > 0)
		m_iDarkness_MouthSmoke = Max(0, m_pWServer->Object_Create("fx_mouthsmoke", GetPosition(), m_iObject));

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
	if (!pCD) return;

	CRegistry* pLevelKeys_Player = m_pWServer->Registry_GetLevelKeys("PLAYER");
	if (pLevelKeys_Player && (pCD->m_iPlayer >= 0))
	{
		for (int i = 0; i < pLevelKeys_Player->GetNumChildren(); i++)
		{
			const CRegistry* pReg = pLevelKeys_Player->GetChild(i);
			OnEvalKey(pReg->GetThisNameHash(), pReg);
		}
	}

	// Re-order models, so that the one with most complete skeleton (i.e. the head model) gets placed in slot 0
	if (0) { // -- disabled, because checking node-count doesn't work when having cloth on a bodypart (might get more nodes than the head model)
		CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
		CXR_Skeleton* pSkel0 = pModel ? pModel->GetSkeleton() : NULL;
		for (uint i = 1; i < CWO_NUMMODELINDICES; i++)
		{
			int iModel = m_iModel[i];
			pModel = m_pWServer->GetMapData()->GetResource_Model(iModel);
			CXR_Skeleton* pSkel = pModel ? pModel->GetSkeleton() : NULL;
			if (pSkel && (!pSkel0 || pSkel->m_lNodes.Len() > pSkel0->m_lNodes.Len()))
			{
				m_iModel[i] = m_iModel[0];
				m_iModel[0] = iModel;
				pSkel0 = pSkel;
			}
		}
	}

	if(pCD->m_iPlayer != -1)
		CRPG_Object::m_bPrecacheForPlayerUse = true;

	if(m_spRPGObjectReg)
	{
		m_spRPGObject = CRPG_Object::CreateObject(m_spRPGObjectReg, m_pWServer);
		m_spRPGObjectReg = NULL;

		// Try to select suitable guns
		CRPG_Object_Char* pChar = Char();
		if (pChar)
		{
			if (!pChar->GetEquippedItem(AG2_ITEMSLOT_WEAPONS))
				if (!pChar->FindNextItemNotEquippedByAnimType(0,3,0,7))
					Char_SelectItemByType(CRPG_Object_Fist_ItemType);

			// Force animgraph to weaponswitch...
			CRPG_Object_Item* pEquippedItem = pChar->GetFinalSelectedItem(AG2_ITEMSLOT_WEAPONS);
			if (pEquippedItem)
			{
				pCD->m_EquippedItemClass = (uint16)pEquippedItem->m_AnimProperty;
				pCD->m_EquippedItemType = (uint16)pEquippedItem->m_iItemType;
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_ITEMANIMTYPE,pEquippedItem->m_AnimType);
				
				// Hide player gun in the beginnning
				if ((pEquippedItem->m_iItemType != CRPG_Object_Fist_ItemType) && 
					(pCD->m_iPlayer != -1))
				{
					int PreviousWeaponId = pEquippedItem->m_Identifier;
					int PreviousItemType = pEquippedItem->m_iItemType;

					// Find fist item
					CRPG_Object_Item* pMelee = Char()->FindItemByType(CRPG_Object_Fist_ItemType);
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
			}
		}
	}
	
	if(!Char())
	{
		m_spRPGObject = (CRPG_Object_Char *)(CRPG_Object *)CRPG_Object::CreateObject("Char", m_pWServer);
		if(!m_spRPGObject)
			Error("OnFinishEvalKeys", "Could not create default RPG object");
	}

	if(pCD->m_iPlayer != -1)
		CRPG_Object::m_bPrecacheForPlayerUse = false;
	
/*	if(m_iModel[0] == 0)
	{
		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
		ConOutL(CStrF("No model was set for character (%s, %s, %s)", GetName(), GetTemplateName(), pCD ? pCD->m_Character.m_Value.Str() : ""));
		m_iModel[0] = m_pWServer->GetMapData()->GetResourceIndex_Model("CoordSys");
	}*/

	if(m_Flags & PLAYER_FLAGS_NONPUSHABLE)
		pCD->m_ExtraFlags = pCD->m_ExtraFlags | PLAYER_EXTRAFLAGS_NONPUSHABLE;

	{
		//Initialize items
		Char()->UpdateItems();

		int iInitialWeapon = 0;

		if (IsBot() && m_spAI)
		{
			int AIRec = m_spAI->GetRecommendedWeapon();
			if(AIRec != -1)
				iInitialWeapon = AIRec;	
		}

		if (iInitialWeapon != 0)
			SelectItem(iInitialWeapon, true);

		if (!SelectItem(Char()->GetInventory(RPG_CHAR_INVENTORY_ITEMS)->GetSelectedItemType(), true))
			NextItem(RPG_CHAR_INVENTORY_ITEMS, false, true);

		if (!SelectItem(Char()->GetInventory(RPG_CHAR_INVENTORY_ARMOUR)->GetSelectedItemType(), true))
			NextItem(RPG_CHAR_INVENTORY_ARMOUR, false, true);

		if (!SelectItem(Char()->GetInventory(RPG_CHAR_INVENTORY_QUESTITEMS)->GetSelectedItemType(), true))
			NextItem(RPG_CHAR_INVENTORY_QUESTITEMS, false, true);
	}

	if(!(m_Flags & PLAYER_FLAGS_WAITSPAWN))
	{
		//This must be done before spawning, but there's probably some stuff that's set during spawning, so do it again afterwards to be safe
		if (m_spAI)
			m_spAI->Init(this, m_pWServer);

		SpawnCharacter(m_InitialPhysMode);
	}
	else
		ClientFlags() |= CWO_CLIENTFLAGS_INVISIBLE;
	
	if (m_spAI)
		m_spAI->Init(this, m_pWServer);

	m_spAI->OnFinishEvalKeys();

	if(pCD->m_iPlayer != -1)
		Char_UpdateQuestItems();

	if(pCD->m_lAnim_VoCap.Len() == 0)
	{
		CWRes_Dialogue *pDialogue = GetDialogueResource(this, m_pWServer);
		if(pDialogue)
		{
			int iSound = pDialogue->GetSoundIndex(100, m_pWServer->GetMapData());
			if(iSound > 0)
			{
				CStr St = m_pWServer->GetMapData()->GetResourceName(iSound);
				CStr Sound = St.Copy(4, 1024);
				int iFind = Sound.FindReverse("_");
				if(iFind > 0)
				{
					CStr Anim = "VoCap\\" + Sound.Copy(0, iFind);
					CStr Path = m_pWServer->GetMapData()->ResolveFileName("Anim\\" + Anim + ".xsa");
					if(CDiskUtil::FileExists(Path))
					{
						int iAnim = m_pWServer->GetMapData()->GetResourceIndex_Anim(Anim);
						if(iAnim > 0)
						{
							pCD->m_lAnim_VoCap.Add(iAnim);
							pCD->m_lAnim_VoCap.MakeDirty();
						}
					}
				}
			}
		}
	}

	// Refresh animgraph
	CWAG2I_Context AGContext(this, m_pWServer, pCD->m_GameTime);
	CWAG2I* pAGI = pCD->m_AnimGraph2.GetAG2I();
	if (pAGI != NULL)
	{
		pAGI->ClearTokens();
		pAGI->Refresh(&AGContext);
	}

	pCD->m_CharName.m_Value = m_pWServer->Object_GetName(m_iObject);
	pCD->m_CharName.MakeDirty();
}

void CWObject_Character::SetTemplateName(CStr _TemplateName)
{
	CWObject_Player::SetTemplateName(_TemplateName);

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
	if(!pCD)
		return;

	pCD->m_Character.m_Value = (CFStr("§LCHAR_") + GetTemplateName()).Str();
	pCD->m_Character.MakeDirty();
}

int CWObject_Character::ResolvePlayerFlags(CStr _Str)
{
	static const char *FlagsTranslate[] =
	{
		"stunnable", "waitspawn", "spawndead", "autodestroy", "timeleap", "nonpushable", "nocrouch", 
		"noblink", "alwaysvisible", "notargeting", "forcetension", "$$$", "$$$", "$$$", "air", "nogunkata", 
		"spawncrouched", "nodialogueturn", "muzzlevisibility", "usednaweapons", "nonightvision", 
		"ragdoll", "nodeathsound", "noaimassistance", "noautoaim", "autounequipweapons", "dualwieldsupported",
		"projectileinvulnerable", "attachforcetriggers", "disableik", NULL
	};

	return _Str.TranslateFlags(FlagsTranslate);
}

int CWObject_Character::ResolvePhysFlags(CStr _Str)
{
	static const char *FlagsTranslate[] =
	{
		"NoCharColl", "NoWorldColl", "NoProjectileColl", "NoGravity", "Immune", "NoMediumMovement", "Immobile", NULL
	};

	return _Str.TranslateFlags(FlagsTranslate);
}


int CWObject_Character::ResolveDarknessSelection(const int8& _Selection)
{
	int Result = 0;

	switch(_Selection)
	{
		case 0:
			Result = PLAYER_DARKNESSMODE_POWER_CREEPINGDARK;
			break;

		case 1:
			Result = PLAYER_DARKNESSMODE_POWER_DEMONARM;
			break;

		case 2:
			Result = PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS;
			break;

		case 3:
			Result = PLAYER_DARKNESSMODE_POWER_BLACKHOLE;
			break;
	}

	return Result;
}

int CWObject_Character::ResolveDarknessSelectValue(int _DarknessPowerFlag)
{
	switch(_DarknessPowerFlag)
	{
	case PLAYER_DARKNESSMODE_POWER_CREEPINGDARK:
		return 0; 
	case PLAYER_DARKNESSMODE_POWER_DEMONARM:
		return 1; 
	case PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS:
		return 2; 
	case PLAYER_DARKNESSMODE_POWER_BLACKHOLE:
		return 3; 
	}
	return -1;
}


int CWObject_Character::ResolveDarknessFlags(CStr _Str)
{
	static const char* FlagsTranslate[] =
	{
		"CreepingDark",			// PLAYER_DARKNESSMODE_POWER_CREEPINGDARK
		"DemonArm",				// PLAYER_DARKNESSMODE_POWER_DEMONARM
		"AncientWeapons",		// PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS
		"BlackHole",			// PLAYER_DARKNESSMODE_POWER_BLACKHOLE
		"DarknessVision",		// PLAYER_DARKNESSMODE_POWER_DARKNESSVISION
		"DarknessShield",		// PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD
		"$$$",
		"$$$",
		NULL

		//"$$$", "$$$", "$$$", "$$$", 
		//"CreepingDark",		// PLAYER_DARKNESSMODE_POWER_CREEPINGDARK
		//"$$$",
		//"DemonArm",			// PLAYER_DARKNESSMODE_POWER_DEMONARM
		//"AncientWeapons",	// PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS
		//"DarknessShield",	// PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD
		//"DarknessVision",	// PLAYER_DARKNESSMODE_DARKNESSVISION
		//NULL,
	};
	return _Str.TranslateFlags(FlagsTranslate);
}


void CWObject_Character::UpdateVisibilityFlag(bool _bRefresh)
{
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
	if(!pCD)
		return;

	if(_bRefresh)
	{
	
		if(!(m_Flags & PLAYER_FLAGS_MUZZLEVISIBILITY) || (pCD->m_MuzzleFlashTick0 >= pCD->m_GameTick - 3 || pCD->m_MuzzleFlashTick1 >= pCD->m_GameTick - 3 ||
														  pCD->m_LightningLightTick0 >= pCD->m_GameTick - 28 || pCD->m_LightningLightTick1 >= pCD->m_GameTick - 28))
			return;

		// Flash is finished, no visibility-flag required
		m_Flags &= ~PLAYER_FLAGS_MUZZLEVISIBILITY;
	}

	int iCM = Char_GetControlMode(this);
	if(pCD->m_iPlayer != -1 ||
	   m_ClientFlags & PLAYER_CLIENTFLAGS_FLASHLIGHT ||
	   pCD->m_Item0_Flags & RPG_ITEM_FLAGS_FLASHLIGHT ||
	   m_Flags & PLAYER_FLAGS_MUZZLEVISIBILITY ||
	   m_ClientFlags & PLAYER_CLIENTFLAGS_DIALOGUE ||
	   iCM == PLAYER_CONTROLMODE_ACTIONCUTSCENE ||
	   iCM == PLAYER_CONTROLMODE_LADDER ||
	   iCM == PLAYER_CONTROLMODE_HANGRAIL ||
	   iCM == PLAYER_CONTROLMODE_LEDGE ||
	   iCM == PLAYER_CONTROLMODE_LEDGE2)
		ClientFlags() |= CWO_CLIENTFLAGS_VISIBILITY;
	else
		ClientFlags() &= ~CWO_CLIENTFLAGS_VISIBILITY;
}

void CWObject_Character::UpdateVisibility(int *_lpExtraModels, int _nExtraModels)
{
	CBox3Dfp32 VisBox;
	VisBox.m_Min = 0;
	VisBox.m_Max = 0;
	bool bShadowCaster = false;
	int nModels = CWO_NUMMODELINDICES + _nExtraModels;

	for(int i = 0; i < nModels; i++)
	{
		CXR_Model* pM;
		if(i < CWO_NUMMODELINDICES)
			pM = m_pWServer->GetMapData()->GetResource_Model(m_iModel[i]);
		else
			pM = m_pWServer->GetMapData()->GetResource_Model(_lpExtraModels[i - CWO_NUMMODELINDICES]);

		if (pM)
		{
			// Update vis box.
			CBox3Dfp32 Box;
			pM->GetBound_Box(Box);
			fp32 Width = ((Box.m_Max[0] - Box.m_Min[0]) + (Box.m_Max[1] - Box.m_Min[1])) / 3;
			Box.m_Max[0] = Width;
			Box.m_Max[1] = Width;
			Box.m_Min[0] = -Width;
			Box.m_Min[1] = -Width;
			VisBox.Expand(Box);

			if (pM->GetParam(CXR_MODEL_PARAM_ISSHADOWCASTER))
				bShadowCaster = true;
		}
	}

	if(bShadowCaster && !(m_ClientFlags & CWO_CLIENTFLAGS_SHADOWCASTER))
		ClientFlags() |= CWO_CLIENTFLAGS_SHADOWCASTER;
	else if(!bShadowCaster && (m_ClientFlags & CWO_CLIENTFLAGS_SHADOWCASTER))
		ClientFlags() &= ~CWO_CLIENTFLAGS_SHADOWCASTER;

	m_Flags &= ~PLAYER_FLAGS_VISBOXEXPANDED;

	CBox3Dfp32 CurBox;
	GetVisBoundBox(CurBox);
	if(CurBox.m_Max != VisBox.m_Max || CurBox.m_Min != VisBox.m_Min)
		m_pWServer->Object_SetVisBox(m_iObject, VisBox.m_Min, VisBox.m_Max);
}

